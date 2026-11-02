/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2026 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#ifndef INTERP_SSE_H
#define INTERP_SSE_H

#ifdef CONFIG_X86

#include <smmintrin.h>

/*
 * Uses distances only for relative comparisons and zero checks.
 * Omit the common factor of 8 used by the scalar implementation,
 * since it does not affect any of these decisions.
 *
 * All distances and their sums fit in signed 16-bit lanes:
 * RGB565: dist <= 313, dist3 <= 626, e/i <= 2504.
 */
__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_dist_sse(__m128i a, __m128i b)
{
	const __m128i m31 = _mm_set1_epi16(31);
	const __m128i m63 = _mm_set1_epi16(63);
	__m128i r = _mm_abs_epi16(_mm_sub_epi16(_mm_srli_epi16(a, 11), _mm_srli_epi16(b, 11)));
	__m128i g = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(_mm_srli_epi16(a, 5), m63), _mm_and_si128(_mm_srli_epi16(b, 5), m63)));
	__m128i bl = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(a, m31), _mm_and_si128(b, m31)));
	__m128i rg = _mm_add_epi16(r, g);
	return _mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(rg, 1), rg), bl);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_dist3_sse(__m128i a, __m128i b, __m128i c)
{
	return _mm_add_epi16(interp_16_565_dist_sse(a, b), interp_16_565_dist_sse(b, c));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_score_sse(__m128i a, __m128i b, __m128i c, __m128i d, __m128i e, __m128i f, __m128i g, __m128i h)
{
	__m128i x = _mm_add_epi16(interp_16_565_dist3_sse(a, b, c), interp_16_565_dist3_sse(d, e, f));
	__m128i y = _mm_slli_epi16(interp_16_565_dist_sse(g, h), 2);
	return _mm_add_epi16(x, y);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_11_sse(__m128i a, __m128i b)
{
	return _mm_add_epi16(_mm_and_si128(_mm_srli_epi16(_mm_xor_si128(a, b), 1), _mm_set1_epi16(0x7BEF)), _mm_and_si128(a, b));
}

/*
 * Weighted interpolation on 5/6/5 channels independently; never overflows
 * a signed 16-bit lane. Only the final shifted packed RGB565 is returned.
 */
__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_weight_sse(__m128i a, __m128i b, int wa, int shift)
{
	const __m128i m31 = _mm_set1_epi16(31);
	const __m128i m63 = _mm_set1_epi16(63);
	__m128i ra = _mm_srli_epi16(a, 11), rb = _mm_srli_epi16(b, 11);
	__m128i ga = _mm_and_si128(_mm_srli_epi16(a, 5), m63);
	__m128i gb = _mm_and_si128(_mm_srli_epi16(b, 5), m63);
	__m128i ba = _mm_and_si128(a, m31), bb = _mm_and_si128(b, m31);
	__m128i w = _mm_set1_epi16((short)wa);
	__m128i r = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(ra, w), rb), shift);
	__m128i g = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(ga, w), gb), shift);
	__m128i bl = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(ba, w), bb), shift);
	return _mm_or_si128(_mm_or_si128(_mm_slli_epi16(r, 11), _mm_slli_epi16(g, 5)), bl);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_31_sse(__m128i a, __m128i b)
{
	return interp_16_565_11_sse(a, interp_16_565_11_sse(a, b));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_565_71_sse(__m128i a, __m128i b)
{
	return interp_16_565_weight_sse(a, b, 7, 3);
}

/*
 * Uses distances only for relative comparisons and zero checks.
 * Omit the common factor of 8 used by the scalar implementation,
 * since it does not affect any of these decisions.
 *
 * All distances and their sums fit in signed 16-bit lanes:
 * RGB555: dist <= 310, dist3 <= 620, e/i <= 2480.
 */
__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_dist_sse(__m128i a, __m128i b)
{
	const __m128i m31 = _mm_set1_epi16(31);
	__m128i r = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(_mm_srli_epi16(a, 10), m31), _mm_and_si128(_mm_srli_epi16(b, 10), m31)));
	__m128i g = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(_mm_srli_epi16(a, 5), m31), _mm_and_si128(_mm_srli_epi16(b, 5), m31)));
	__m128i bl = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(a, m31), _mm_and_si128(b, m31)));
	__m128i r3 = _mm_add_epi16(_mm_slli_epi16(r, 1), r);
	__m128i g6 = _mm_add_epi16(_mm_slli_epi16(g, 2), _mm_slli_epi16(g, 1));
	return _mm_add_epi16(_mm_add_epi16(r3, g6), bl);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_dist3_sse(__m128i a, __m128i b, __m128i c)
{
	return _mm_add_epi16(interp_16_555_dist_sse(a, b), interp_16_555_dist_sse(b, c));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_score_sse(__m128i a, __m128i b, __m128i c, __m128i d, __m128i e, __m128i f, __m128i g, __m128i h)
{
	__m128i x = _mm_add_epi16(interp_16_555_dist3_sse(a, b, c), interp_16_555_dist3_sse(d, e, f));
	__m128i y = _mm_slli_epi16(interp_16_555_dist_sse(g, h), 2);
	return _mm_add_epi16(x, y);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_11_sse(__m128i a, __m128i b)
{
	return _mm_add_epi16(_mm_and_si128(_mm_srli_epi16(_mm_xor_si128(a, b), 1), _mm_set1_epi16(0x3DEF)), _mm_and_si128(a, b));
}

/*
 * Weighted interpolation on 5/5/5 channels independently; never overflows
 * a signed 16-bit lane. Only the final shifted packed RGB555 is returned.
 */
__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_weight_sse(__m128i a, __m128i b, int wa, int shift)
{
	const __m128i m31 = _mm_set1_epi16(31);
	__m128i ra = _mm_and_si128(_mm_srli_epi16(a, 10), m31);
	__m128i rb = _mm_and_si128(_mm_srli_epi16(b, 10), m31);
	__m128i ga = _mm_and_si128(_mm_srli_epi16(a, 5), m31);
	__m128i gb = _mm_and_si128(_mm_srli_epi16(b, 5), m31);
	__m128i ba = _mm_and_si128(a, m31), bb = _mm_and_si128(b, m31);
	__m128i w = _mm_set1_epi16((short)wa);
	__m128i r = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(ra, w), rb), shift);
	__m128i g = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(ga, w), gb), shift);
	__m128i bl = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(ba, w), bb), shift);
	return _mm_or_si128(_mm_or_si128(_mm_slli_epi16(r, 10), _mm_slli_epi16(g, 5)), bl);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_31_sse(__m128i a, __m128i b)
{
	return interp_16_555_11_sse(a, interp_16_555_11_sse(a, b));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_16_555_71_sse(__m128i a, __m128i b)
{
	return interp_16_555_weight_sse(a, b, 7, 3);
}

/*
 * Partial RGB32 distance:
 *   low 16 bits: B + 6 * G
 *   high 16 bits: 3 * R
 *
 * Two signed 16-bit lanes per pixel.
 */
__attribute__((target("sse4.1"))) static inline __m128i interp_32_dist16_sse(__m128i a, __m128i b)
{
	const __m128i weights = _mm_set1_epi32(0x00030601);
	__m128i d = _mm_sub_epi8(_mm_max_epu8(a, b), _mm_min_epu8(a, b));
	return _mm_maddubs_epi16(d, weights);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_32_dist_sse(__m128i a, __m128i b)
{
	return _mm_madd_epi16(interp_32_dist16_sse(a, b), _mm_set1_epi16(1));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_32_dist3_sse(__m128i a, __m128i b, __m128i c)
{
	__m128i x = interp_32_dist16_sse(a, b);
	__m128i y = interp_32_dist16_sse(b, c);
	return _mm_madd_epi16(_mm_add_epi16(x, y), _mm_set1_epi16(1));
}

/*
 * dist(a,b) + dist(b,c) + dist(d,e) + dist(e,f) + 4 * dist(g,h)
 *
 * Accumulate in 16-bit lanes and reduce to 32 bits only once.
 */
__attribute__((target("sse4.1"))) static inline __m128i interp_32_score_sse(__m128i a, __m128i b, __m128i c, __m128i d, __m128i e, __m128i f, __m128i g, __m128i h)
{
	__m128i x0 = interp_32_dist16_sse(a, b);
	__m128i x1 = interp_32_dist16_sse(b, c);
	__m128i x2 = interp_32_dist16_sse(d, e);
	__m128i x3 = interp_32_dist16_sse(e, f);
	__m128i x4 = interp_32_dist16_sse(g, h);
	__m128i s0 = _mm_add_epi16(x0, x1);
	__m128i s1 = _mm_add_epi16(x2, x3);
	__m128i s2 = _mm_slli_epi16(x4, 2);
	__m128i sum = _mm_add_epi16(_mm_add_epi16(s0, s1), s2);
	return _mm_madd_epi16(sum, _mm_set1_epi16(1));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_32_11_sse(__m128i a, __m128i b)
{
	return _mm_avg_epu8(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i interp_32_31_sse(__m128i a, __m128i b)
{
	const __m128i w = _mm_set1_epi16(0x0103);
	__m128i lo = _mm_maddubs_epi16(_mm_unpacklo_epi8(a, b), w);
	__m128i hi = _mm_maddubs_epi16(_mm_unpackhi_epi8(a, b), w);
	return _mm_packus_epi16(_mm_srli_epi16(lo, 2), _mm_srli_epi16(hi, 2));
}

__attribute__((target("sse4.1"))) static inline __m128i interp_32_71_sse(__m128i a, __m128i b)
{
	const __m128i w = _mm_set1_epi16(0x0107);
	__m128i lo = _mm_maddubs_epi16(_mm_unpacklo_epi8(a, b), w);
	__m128i hi = _mm_maddubs_epi16(_mm_unpackhi_epi8(a, b), w);
	return _mm_packus_epi16(_mm_srli_epi16(lo, 3), _mm_srli_epi16(hi, 3));
}

/* Three packed output pixels per source pixel, exactly three 128-bit stores. */
__attribute__((target("sse4.1"))) static inline void interp_16_565_store3_sse(interp_uint16* dst, __m128i e0, __m128i e1, __m128i e2)
{
	__m128i r0 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8(0, 1, (-128), (-128), (-128), (-128), 2, 3, (-128), (-128), (-128), (-128), 4, 5, (-128), (-128))), _mm_shuffle_epi8(e1, _mm_setr_epi8((-128), (-128), 0, 1, (-128), (-128), (-128), (-128), 2, 3, (-128), (-128), (-128), (-128), 4, 5))), _mm_shuffle_epi8(e2, _mm_setr_epi8((-128), (-128), (-128), (-128), 0, 1, (-128), (-128), (-128), (-128), 2, 3, (-128), (-128), (-128), (-128))));
	_mm_storeu_si128((__m128i*)(void*)(dst + 0), r0);
	__m128i r1 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8((-128), (-128), 6, 7, (-128), (-128), (-128), (-128), 8, 9, (-128), (-128), (-128), (-128), 10, 11)), _mm_shuffle_epi8(e1, _mm_setr_epi8((-128), (-128), (-128), (-128), 6, 7, (-128), (-128), (-128), (-128), 8, 9, (-128), (-128), (-128), (-128)))), _mm_shuffle_epi8(e2, _mm_setr_epi8(4, 5, (-128), (-128), (-128), (-128), 6, 7, (-128), (-128), (-128), (-128), 8, 9, (-128), (-128))));
	_mm_storeu_si128((__m128i*)(void*)(dst + 8), r1);
	__m128i r2 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8((-128), (-128), (-128), (-128), 12, 13, (-128), (-128), (-128), (-128), 14, 15, (-128), (-128), (-128), (-128))), _mm_shuffle_epi8(e1, _mm_setr_epi8(10, 11, (-128), (-128), (-128), (-128), 12, 13, (-128), (-128), (-128), (-128), 14, 15, (-128), (-128)))), _mm_shuffle_epi8(e2, _mm_setr_epi8((-128), (-128), 10, 11, (-128), (-128), (-128), (-128), 12, 13, (-128), (-128), (-128), (-128), 14, 15)));
	_mm_storeu_si128((__m128i*)(void*)(dst + 16), r2);
}

/* Three packed output pixels per source pixel, exactly three 128-bit stores. */
__attribute__((target("sse4.1"))) static inline void interp_16_555_store3_sse(interp_uint16* dst, __m128i e0, __m128i e1, __m128i e2)
{
	__m128i r0 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8(0, 1, (-128), (-128), (-128), (-128), 2, 3, (-128), (-128), (-128), (-128), 4, 5, (-128), (-128))), _mm_shuffle_epi8(e1, _mm_setr_epi8((-128), (-128), 0, 1, (-128), (-128), (-128), (-128), 2, 3, (-128), (-128), (-128), (-128), 4, 5))), _mm_shuffle_epi8(e2, _mm_setr_epi8((-128), (-128), (-128), (-128), 0, 1, (-128), (-128), (-128), (-128), 2, 3, (-128), (-128), (-128), (-128))));
	_mm_storeu_si128((__m128i*)(void*)(dst + 0), r0);
	__m128i r1 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8((-128), (-128), 6, 7, (-128), (-128), (-128), (-128), 8, 9, (-128), (-128), (-128), (-128), 10, 11)), _mm_shuffle_epi8(e1, _mm_setr_epi8((-128), (-128), (-128), (-128), 6, 7, (-128), (-128), (-128), (-128), 8, 9, (-128), (-128), (-128), (-128)))), _mm_shuffle_epi8(e2, _mm_setr_epi8(4, 5, (-128), (-128), (-128), (-128), 6, 7, (-128), (-128), (-128), (-128), 8, 9, (-128), (-128))));
	_mm_storeu_si128((__m128i*)(void*)(dst + 8), r1);
	__m128i r2 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8((-128), (-128), (-128), (-128), 12, 13, (-128), (-128), (-128), (-128), 14, 15, (-128), (-128), (-128), (-128))), _mm_shuffle_epi8(e1, _mm_setr_epi8(10, 11, (-128), (-128), (-128), (-128), 12, 13, (-128), (-128), (-128), (-128), 14, 15, (-128), (-128)))), _mm_shuffle_epi8(e2, _mm_setr_epi8((-128), (-128), 10, 11, (-128), (-128), (-128), (-128), 12, 13, (-128), (-128), (-128), (-128), 14, 15)));
	_mm_storeu_si128((__m128i*)(void*)(dst + 16), r2);
}

/* Three packed output pixels per source pixel, exactly three 128-bit stores. */
__attribute__((target("sse4.1"))) static inline void interp_32_store3_sse(interp_uint32* dst, __m128i e0, __m128i e1, __m128i e2)
{
	__m128i r0 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8(0, 1, 2, 3, (-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128), 4, 5, 6, 7)), _mm_shuffle_epi8(e1, _mm_setr_epi8((-128), (-128), (-128), (-128), 0, 1, 2, 3, (-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128)))), _mm_shuffle_epi8(e2, _mm_setr_epi8((-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128), 0, 1, 2, 3, (-128), (-128), (-128), (-128))));
	_mm_storeu_si128((__m128i*)(void*)(dst + 0), r0);
	__m128i r1 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8((-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128), 8, 9, 10, 11, (-128), (-128), (-128), (-128))), _mm_shuffle_epi8(e1, _mm_setr_epi8(4, 5, 6, 7, (-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128), 8, 9, 10, 11))), _mm_shuffle_epi8(e2, _mm_setr_epi8((-128), (-128), (-128), (-128), 4, 5, 6, 7, (-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128))));
	_mm_storeu_si128((__m128i*)(void*)(dst + 4), r1);
	__m128i r2 = _mm_or_si128(_mm_or_si128(_mm_shuffle_epi8(e0, _mm_setr_epi8((-128), (-128), (-128), (-128), 12, 13, 14, 15, (-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128))), _mm_shuffle_epi8(e1, _mm_setr_epi8((-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128), 12, 13, 14, 15, (-128), (-128), (-128), (-128)))), _mm_shuffle_epi8(e2, _mm_setr_epi8(8, 9, 10, 11, (-128), (-128), (-128), (-128), (-128), (-128), (-128), (-128), 12, 13, 14, 15)));
	_mm_storeu_si128((__m128i*)(void*)(dst + 8), r2);
}

__attribute__((target("sse4.1"))) static inline void interp_16_565_store4_sse(interp_uint16* dst, __m128i e0, __m128i e1, __m128i e2, __m128i e3)
{
	__m128i al = _mm_unpacklo_epi16(e0, e1), ah = _mm_unpackhi_epi16(e0, e1);
	__m128i bl = _mm_unpacklo_epi16(e2, e3), bh = _mm_unpackhi_epi16(e2, e3);
	_mm_storeu_si128((__m128i*)(void*)(dst + 0), _mm_unpacklo_epi32(al, bl));
	_mm_storeu_si128((__m128i*)(void*)(dst + 8), _mm_unpackhi_epi32(al, bl));
	_mm_storeu_si128((__m128i*)(void*)(dst + 16), _mm_unpacklo_epi32(ah, bh));
	_mm_storeu_si128((__m128i*)(void*)(dst + 24), _mm_unpackhi_epi32(ah, bh));
}

__attribute__((target("sse4.1"))) static inline void interp_16_555_store4_sse(interp_uint16* dst, __m128i e0, __m128i e1, __m128i e2, __m128i e3)
{
	__m128i al = _mm_unpacklo_epi16(e0, e1), ah = _mm_unpackhi_epi16(e0, e1);
	__m128i bl = _mm_unpacklo_epi16(e2, e3), bh = _mm_unpackhi_epi16(e2, e3);
	_mm_storeu_si128((__m128i*)(void*)(dst + 0), _mm_unpacklo_epi32(al, bl));
	_mm_storeu_si128((__m128i*)(void*)(dst + 8), _mm_unpackhi_epi32(al, bl));
	_mm_storeu_si128((__m128i*)(void*)(dst + 16), _mm_unpacklo_epi32(ah, bh));
	_mm_storeu_si128((__m128i*)(void*)(dst + 24), _mm_unpackhi_epi32(ah, bh));
}

__attribute__((target("sse4.1"))) static inline void interp_32_store4_sse(interp_uint32* dst, __m128i e0, __m128i e1, __m128i e2, __m128i e3)
{
	__m128i al = _mm_unpacklo_epi32(e0, e1), ah = _mm_unpackhi_epi32(e0, e1);
	__m128i bl = _mm_unpacklo_epi32(e2, e3), bh = _mm_unpackhi_epi32(e2, e3);
	_mm_storeu_si128((__m128i*)(void*)(dst + 0), _mm_unpacklo_epi64(al, bl));
	_mm_storeu_si128((__m128i*)(void*)(dst + 4), _mm_unpackhi_epi64(al, bl));
	_mm_storeu_si128((__m128i*)(void*)(dst + 8), _mm_unpacklo_epi64(ah, bh));
	_mm_storeu_si128((__m128i*)(void*)(dst + 12), _mm_unpackhi_epi64(ah, bh));
}

#endif

#endif

