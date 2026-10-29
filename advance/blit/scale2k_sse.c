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

#include "portable.h"

#include "scale2k.h"

#ifdef CONFIG_X86

#include "interp_sse.h"

/***************************************************************************/
/* Scale2k SSE4.1 implementation */

/*
 * Vector equivalent of one scalar SCALE2K rotation. All masks are disjoint:
 * square-block (with no-star), horizontal, vertical and pure diagonal.
 * The square-block branch takes priority even if the no-star test fails.
 * The four rotations are applied sequentially to preserve their effects.
 */
#define SCALE2K_SSE(fmt, eq, A, B, C, D, E, F, G, H, I, N0, N1, N2, N3) \
	do { \
		__m128i _gate = _mm_andnot_si128(eq((B), (E)), eq((D), (B))); \
		if (!_mm_testz_si128(_gate, _gate)) { \
			__m128i _block = _mm_and_si128(eq((B), (C)), eq((D), (G))); \
			__m128i _other = _mm_andnot_si128(_block, _gate); \
			__m128i _square = _mm_andnot_si128(eq((A), (E)), _mm_and_si128(_gate, _block)); \
			__m128i _horizontal = _mm_and_si128(_other, _mm_andnot_si128(eq((C), (F)), eq((B), (C)))); \
			__m128i _remaining = _mm_andnot_si128(_horizontal, _other); \
			__m128i _vertical = _mm_and_si128(_remaining, _mm_andnot_si128(eq((G), (H)), eq((D), (G)))); \
			__m128i _diagonal = _mm_andnot_si128(_vertical, _remaining); \
			__m128i _weighted = _mm_or_si128(_mm_or_si128(_square, _horizontal), _vertical); \
			__m128i _old0 = (N0), _old1 = (N1), _old2 = (N2); \
			__m128i _new0 = _mm_blendv_epi8(_old0, interp_ ## fmt ## _31_sse((D), _old0), _weighted); \
			(N0) = _mm_blendv_epi8(_new0, interp_ ## fmt ## _11_sse(_old0, (D)), _diagonal); \
			(N1) = _mm_blendv_epi8(_old1, interp_ ## fmt ## _31_sse(_old1, (D)), _mm_or_si128(_square, _horizontal)); \
			(N2) = _mm_blendv_epi8(_old2, interp_ ## fmt ## _31_sse(_old2, (D)), _mm_or_si128(_square, _vertical)); \
		} \
	} while (0)

static inline void scale2k_16_565_sse_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E;

	SCALE2K(16_565, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3);
	SCALE2K(16_565, G, D, A, H, E, B, I, F, C, E2, E0, E3, E1);
	SCALE2K(16_565, I, H, G, F, E, D, C, B, A, E3, E2, E1, E0);
	SCALE2K(16_565, C, F, I, B, E, H, A, D, G, E1, E3, E0, E2);

	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void scale2k_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First source pixel has a clamped left neighbor. */
	scale2k_16_565_sse_edge_pixel(dst0, dst1, src0, src1, src2, count, x++);

	/* 8 source pixels per SIMD block; the last lane needs a right neighbor. */
	for (; count - x > 8; x += 8) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E;

		SCALE2K_SSE(16_565, _mm_cmpeq_epi16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3);
		SCALE2K_SSE(16_565, _mm_cmpeq_epi16, G, D, A, H, E, B, I, F, C, E2, E0, E3, E1);
		SCALE2K_SSE(16_565, _mm_cmpeq_epi16, I, H, G, F, E, D, C, B, A, E3, E2, E1, E0);
		SCALE2K_SSE(16_565, _mm_cmpeq_epi16, C, F, I, B, E, H, A, D, G, E1, E3, E0, E2);

		/* Interleaved output stores, with no scratch buffer. */
		__m128i top_lo = _mm_unpacklo_epi16(E0, E1);
		__m128i top_hi = _mm_unpackhi_epi16(E0, E1);
		__m128i bot_lo = _mm_unpacklo_epi16(E2, E3);
		__m128i bot_hi = _mm_unpackhi_epi16(E2, E3);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), top_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 8), top_hi);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), bot_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 8), bot_hi);
	}

	/* Trailing pixels, including the right border, use the scalar rule. */
	for (; x < count; ++x)
		scale2k_16_565_sse_edge_pixel(dst0, dst1, src0, src1, src2, count, x);
}

static inline void scale2k_16_555_sse_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E;

	SCALE2K(16_555, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3);
	SCALE2K(16_555, G, D, A, H, E, B, I, F, C, E2, E0, E3, E1);
	SCALE2K(16_555, I, H, G, F, E, D, C, B, A, E3, E2, E1, E0);
	SCALE2K(16_555, C, F, I, B, E, H, A, D, G, E1, E3, E0, E2);

	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void scale2k_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First source pixel has a clamped left neighbor. */
	scale2k_16_555_sse_edge_pixel(dst0, dst1, src0, src1, src2, count, x++);

	/* 8 source pixels per SIMD block; the last lane needs a right neighbor. */
	for (; count - x > 8; x += 8) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E;

		SCALE2K_SSE(16_555, _mm_cmpeq_epi16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3);
		SCALE2K_SSE(16_555, _mm_cmpeq_epi16, G, D, A, H, E, B, I, F, C, E2, E0, E3, E1);
		SCALE2K_SSE(16_555, _mm_cmpeq_epi16, I, H, G, F, E, D, C, B, A, E3, E2, E1, E0);
		SCALE2K_SSE(16_555, _mm_cmpeq_epi16, C, F, I, B, E, H, A, D, G, E1, E3, E0, E2);

		/* Interleaved output stores, with no scratch buffer. */
		__m128i top_lo = _mm_unpacklo_epi16(E0, E1);
		__m128i top_hi = _mm_unpackhi_epi16(E0, E1);
		__m128i bot_lo = _mm_unpacklo_epi16(E2, E3);
		__m128i bot_hi = _mm_unpackhi_epi16(E2, E3);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), top_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 8), top_hi);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), bot_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 8), bot_hi);
	}

	/* Trailing pixels, including the right border, use the scalar rule. */
	for (; x < count; ++x)
		scale2k_16_555_sse_edge_pixel(dst0, dst1, src0, src1, src2, count, x);
}

static inline void scale2k_32_sse_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint32 A = src0[left], B = src0[x], C = src0[right];
	interp_uint32 D = src1[left], E = src1[x], F = src1[right];
	interp_uint32 G = src2[left], H = src2[x], I = src2[right];
	interp_uint32 E0 = E, E1 = E, E2 = E, E3 = E;

	SCALE2K(32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3);
	SCALE2K(32, G, D, A, H, E, B, I, F, C, E2, E0, E3, E1);
	SCALE2K(32, I, H, G, F, E, D, C, B, A, E3, E2, E1, E0);
	SCALE2K(32, C, F, I, B, E, H, A, D, G, E1, E3, E0, E2);

	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void scale2k_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First source pixel has a clamped left neighbor. */
	scale2k_32_sse_edge_pixel(dst0, dst1, src0, src1, src2, count, x++);

	/* 4 source pixels per SIMD block; the last lane needs a right neighbor. */
	for (; count - x > 4; x += 4) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E;

		SCALE2K_SSE(32, _mm_cmpeq_epi32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3);
		SCALE2K_SSE(32, _mm_cmpeq_epi32, G, D, A, H, E, B, I, F, C, E2, E0, E3, E1);
		SCALE2K_SSE(32, _mm_cmpeq_epi32, I, H, G, F, E, D, C, B, A, E3, E2, E1, E0);
		SCALE2K_SSE(32, _mm_cmpeq_epi32, C, F, I, B, E, H, A, D, G, E1, E3, E0, E2);

		/* Interleaved output stores, with no scratch buffer. */
		__m128i top_lo = _mm_unpacklo_epi32(E0, E1);
		__m128i top_hi = _mm_unpackhi_epi32(E0, E1);
		__m128i bot_lo = _mm_unpacklo_epi32(E2, E3);
		__m128i bot_hi = _mm_unpackhi_epi32(E2, E3);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), top_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 4), top_hi);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), bot_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 4), bot_hi);
	}

	/* Trailing pixels, including the right border, use the scalar rule. */
	for (; x < count; ++x)
		scale2k_32_sse_edge_pixel(dst0, dst1, src0, src1, src2, count, x);
}

#endif

