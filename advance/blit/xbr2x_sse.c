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

#include "xbr2x.h"

#ifdef CONFIG_X86

#include <smmintrin.h>

/*
 * All distances and their sums fit in signed 16-bit lanes:
 * dist <= 2504, dist3 <= 5008, and e/i <= 20032.
 */
__attribute__((target("sse4.1"))) static inline __m128i xbr565_dist_v(__m128i a, __m128i b)
{
	const __m128i m31 = _mm_set1_epi16(31);
	const __m128i m63 = _mm_set1_epi16(63);
	__m128i r = _mm_abs_epi16(_mm_sub_epi16(_mm_srli_epi16(a, 11), _mm_srli_epi16(b, 11)));
	__m128i g = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(_mm_srli_epi16(a, 5), m63), _mm_and_si128(_mm_srli_epi16(b, 5), m63)));
	__m128i bl = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(a, m31), _mm_and_si128(b, m31)));
	__m128i rg = _mm_add_epi16(r, g);
	return _mm_slli_epi16(_mm_add_epi16(_mm_add_epi16(_mm_slli_epi16(rg, 1), rg), bl), 3);
}

__attribute__((target("sse4.1"))) static inline __m128i xbr565_dist3_v(__m128i a, __m128i b, __m128i c)
{
	return _mm_add_epi16(xbr565_dist_v(a, b), xbr565_dist_v(b, c));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr565_11_v(__m128i a, __m128i b)
{
	return _mm_add_epi16(_mm_and_si128(_mm_srli_epi16(_mm_xor_si128(a, b), 1), _mm_set1_epi16(0x7BEF)), _mm_and_si128(a, b));
}

/*
 * Weighted interpolation on 5/6/5 channels independently; never overflows
 * a signed 16-bit lane. Only the final shifted packed RGB565 is returned.
 */
__attribute__((target("sse4.1"))) static inline __m128i xbr565_weight_v(__m128i a, __m128i b, int wa, int shift)
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

__attribute__((target("sse4.1"))) static inline __m128i xbr565_31_v(__m128i a, __m128i b)
{
	return xbr565_11_v(a, xbr565_11_v(a, b));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr565_71_v(__m128i a, __m128i b)
{
	return xbr565_weight_v(a, b, 7, 3);
}

/*
 * XBR decisions are made independently per 16-bit pixel lane. The four
 * rotations are applied in scalar order because their E registers overlap.
 */
#define XBR565_V(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		__m128i _gate = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi16((PE), (PH)), _mm_cmpeq_epi16((PE), (PF))), _mm_set1_epi16(-1)); \
		if (!_mm_testz_si128(_gate, _gate)) { \
			__m128i _e = _mm_add_epi16(_mm_add_epi16(xbr565_dist3_v((PC), (PE), (PG)), xbr565_dist3_v((H5), (PI), (F4))), \
					_mm_slli_epi16(xbr565_dist_v((PH), (PF)), 2)); \
			__m128i _i = _mm_add_epi16(_mm_add_epi16(xbr565_dist3_v((PD), (PH), (I5)), xbr565_dist3_v((I4), (PF), (PB))), \
					_mm_slli_epi16(xbr565_dist_v((PE), (PI)), 2)); \
			__m128i _active = _mm_and_si128(_gate, _mm_cmpgt_epi16(_i, _e)); \
			if (!_mm_testz_si128(_active, _active)) { \
				__m128i _ke = xbr565_dist_v((PF), (PG)); \
				__m128i _ki = xbr565_dist_v((PH), (PC)); \
				__m128i _ex2 = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi16((PE), (PC)), _mm_cmpeq_epi16((PB), (PC))), _active); \
				__m128i _ex3 = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi16((PE), (PG)), _mm_cmpeq_epi16((PD), (PG))), _active); \
				__m128i _both = _mm_and_si128(_ex2, _ex3); \
				__m128i _leftup = _mm_and_si128(_both, _mm_and_si128(_mm_cmpeq_epi16(_ke, _mm_setzero_si128()), \
						_mm_cmpeq_epi16(_ki, _mm_setzero_si128()))); \
				__m128i _left = _mm_andnot_si128(_leftup, _mm_andnot_si128(_mm_cmpgt_epi16(_mm_slli_epi16(_ke, 1), _ki), _ex3)); \
				__m128i _up = _mm_andnot_si128(_mm_or_si128(_leftup, _left), \
						_mm_andnot_si128(_mm_cmpgt_epi16(_mm_slli_epi16(_ki, 1), _ke), _ex2)); \
				__m128i _dia = _mm_andnot_si128(_mm_or_si128(_mm_or_si128(_leftup, _left), _up), _active); \
				__m128i _dpf = xbr565_dist_v((PE), (PF)); \
				__m128i _dph = xbr565_dist_v((PE), (PH)); \
				__m128i _px = _mm_blendv_epi8((PF), (PH), _mm_cmpgt_epi16(_dpf, _dph)); \
				__m128i _e3 = (N3), _e1 = (N1), _e2 = (N2); \
				__m128i _side = _mm_or_si128(_left, _up); \
				_e3 = _mm_blendv_epi8(_e3, xbr565_71_v(_px, _e3), _leftup); \
				_e3 = _mm_blendv_epi8(_e3, xbr565_31_v(_px, (N3)), _side); \
				_e3 = _mm_blendv_epi8(_e3, xbr565_11_v((N3), _px), _dia); \
				__m128i _t2 = xbr565_31_v(_e2, _px); \
				__m128i _t1 = xbr565_31_v(_e1, _px); \
				(N3) = _e3; \
				(N2) = _mm_blendv_epi8(_e2, _t2, _mm_or_si128(_leftup, _left)); \
				(N1) = _mm_blendv_epi8(_mm_blendv_epi8(_e1, _t2, _leftup), _t1, _up); \
			} \
		} \
} while (0)

/* Scalar border implementation, duplicated here so this kernel does not
 * depend on the global RGB16 masks or overwrite previously written pixels.
 */
static inline int xbr565_dist_s(interp_uint16 a, interp_uint16 b)
{
	int r = (a >> 11) - (b >> 11);
	int g = ((a >> 5) & 63) - ((b >> 5) & 63);
	int bl = (a & 31) - (b & 31);
	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (bl < 0) bl = -bl;
	return (3 * (r + g) + bl) * 8;
}

static inline int xbr565_dist3_s(interp_uint16 a, interp_uint16 b, interp_uint16 c)
{
	return xbr565_dist_s(a, b) + xbr565_dist_s(b, c);
}

static inline interp_uint16 xbr565_11_s(interp_uint16 a, interp_uint16 b)
{
	return (((a ^ b) >> 1) & 0x7BEF) + (a & b);
}

static inline interp_uint16 xbr565_weight_s(interp_uint16 a, interp_uint16 b, unsigned wa, unsigned shift)
{
	unsigned r = (wa * (a >> 11) + (b >> 11)) >> shift;
	unsigned g = (wa * ((a >> 5) & 63) + ((b >> 5) & 63)) >> shift;
	unsigned bl = (wa * (a & 31) + (b & 31)) >> shift;
	return (interp_uint16)((r << 11) | (g << 5) | bl);
}

static inline interp_uint16 xbr565_31_s(interp_uint16 a, interp_uint16 b)
{
	return xbr565_11_s(a, xbr565_11_s(a, b));
}

static inline interp_uint16 xbr565_71_s(interp_uint16 a, interp_uint16 b)
{
	return xbr565_weight_s(a, b, 7, 3);
}

#define XBR565_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned _e = xbr565_dist3_s((PC), (PE), (PG)) + xbr565_dist3_s((H5), (PI), (F4)) + 4 * xbr565_dist_s((PH), (PF)); \
			unsigned _i = xbr565_dist3_s((PD), (PH), (I5)) + xbr565_dist3_s((I4), (PF), (PB)) + 4 * xbr565_dist_s((PE), (PI)); \
			if (_e < _i) { \
				int _ex2 = (PE) != (PC) && (PB) != (PC); \
				int _ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned _ke = xbr565_dist_s((PF), (PG)); \
				unsigned _ki = xbr565_dist_s((PH), (PC)); \
				interp_uint16 _px = xbr565_dist_s((PE), (PF)) <= xbr565_dist_s((PE), (PH)) ? (PF) : (PH); \
				if (_ke == 0 && _ki == 0 && _ex3 && _ex2) { \
					(N3) = xbr565_71_s(_px, (N3)); \
					(N1) = (N2) = xbr565_31_s((N2), _px); \
				} else if (2 * _ke <= _ki && _ex3) { \
					(N3) = xbr565_31_s(_px, (N3)); \
					(N2) = xbr565_31_s((N2), _px); \
				} else if (_ke >= 2 * _ki && _ex2) { \
					(N3) = xbr565_31_s(_px, (N3)); \
					(N1) = xbr565_31_s((N1), _px); \
				} else { \
					(N3) = xbr565_11_s((N3), _px); \
				} \
			} \
		} \
} while (0)

static inline void xbr565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, const interp_uint16* src3,
	const interp_uint16* src4, unsigned count, unsigned x)
{
	unsigned xm2 = x >= 2 ? x - 2 : 0;
	unsigned xm1 = x >= 1 ? x - 1 : 0;
	unsigned xp1 = count - x >= 2 ? x + 1 : count - 1;
	unsigned xp2 = count - x >= 3 ? x + 2 : count - 1;

	interp_uint16 A1 = src0[xm1], B1 = src0[x], C1 = src0[xp1];
	interp_uint16 A0 = src1[xm2], PA = src1[xm1], PB = src1[x], PC = src1[xp1], C4 = src1[xp2];
	interp_uint16 D0 = src2[xm2], PD = src2[xm1], PE = src2[x], PF = src2[xp1], F4 = src2[xp2];
	interp_uint16 G0 = src3[xm2], PG = src3[xm1], PH = src3[x], PI = src3[xp1], I4 = src3[xp2];
	interp_uint16 G5 = src4[xm1], H5 = src4[x], I5 = src4[xp1];
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE;
	XBR565_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR565_S(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR565_S(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR565_S(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1")))
void xbr2x_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* Handle the two leftmost pixels without any output scratch memory. */
	for (; x < count && x < 2; ++x)
		xbr565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* x>=2, and count-x>=10 ensures all loads, including the +2 offset for
	 * the final lane, remain in the source row (at most x+9).
	 */
	for (; count - x >= 10; x += 8) {
#define LD565(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A1 = LD565(src0, -1), B1 = LD565(src0, 0), C1 = LD565(src0, 1);
		__m128i A0 = LD565(src1, -2), PA = LD565(src1, -1), PB = LD565(src1, 0), PC = LD565(src1, 1), C4 = LD565(src1, 2);
		__m128i D0 = LD565(src2, -2), PD = LD565(src2, -1), PE = LD565(src2, 0), PF = LD565(src2, 1), F4 = LD565(src2, 2);
		__m128i G0 = LD565(src3, -2), PG = LD565(src3, -1), PH = LD565(src3, 0), PI = LD565(src3, 1), I4 = LD565(src3, 2);
		__m128i G5 = LD565(src4, -1), H5 = LD565(src4, 0), I5 = LD565(src4, 1);
#undef LD565
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBR565_V(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR565_V(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR565_V(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR565_V(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

		__m128i top_lo = _mm_unpacklo_epi16(E0, E1);
		__m128i top_hi = _mm_unpackhi_epi16(E0, E1);
		__m128i bot_lo = _mm_unpacklo_epi16(E2, E3);
		__m128i bot_hi = _mm_unpackhi_epi16(E2, E3);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), top_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 8), top_hi);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), bot_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 8), bot_hi);
	}

	for (; x < count; ++x)
		xbr565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

/* All distances and their sums fit in signed 16-bit lanes:
 * dist <= 2480, dist3 <= 4960, and e/i <= 19840.
 */
__attribute__((target("sse4.1"))) static inline __m128i xbr555_dist_v(__m128i a, __m128i b)
{
	const __m128i m31 = _mm_set1_epi16(31);
	__m128i r = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(_mm_srli_epi16(a, 10), m31), _mm_and_si128(_mm_srli_epi16(b, 10), m31)));
	__m128i g = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(_mm_srli_epi16(a, 5), m31), _mm_and_si128(_mm_srli_epi16(b, 5), m31)));
	__m128i bl = _mm_abs_epi16(_mm_sub_epi16(_mm_and_si128(a, m31), _mm_and_si128(b, m31)));
	__m128i r3 = _mm_add_epi16(_mm_slli_epi16(r, 1), r);
	__m128i g6 = _mm_add_epi16(_mm_slli_epi16(g, 2), _mm_slli_epi16(g, 1));
	return _mm_slli_epi16(_mm_add_epi16(_mm_add_epi16(r3, g6), bl), 3);
}

__attribute__((target("sse4.1"))) static inline __m128i xbr555_dist3_v(__m128i a, __m128i b, __m128i c)
{
	return _mm_add_epi16(xbr555_dist_v(a, b), xbr555_dist_v(b, c));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr555_11_v(__m128i a, __m128i b)
{
	return _mm_add_epi16(_mm_and_si128(_mm_srli_epi16(_mm_xor_si128(a, b), 1), _mm_set1_epi16(0x3DEF)), _mm_and_si128(a, b));
}

/* Weighted interpolation on 5/5/5 channels independently; never overflows
 * a signed 16-bit lane. Only the final shifted packed RGB555 is returned.
 */
__attribute__((target("sse4.1"))) static inline __m128i xbr555_weight_v(__m128i a, __m128i b, int wa, int shift)
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

__attribute__((target("sse4.1"))) static inline __m128i xbr555_31_v(__m128i a, __m128i b)
{
	return xbr555_11_v(a, xbr555_11_v(a, b));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr555_71_v(__m128i a, __m128i b)
{
	return xbr555_weight_v(a, b, 7, 3);
}

/*
 * XBR decisions are made independently per 16-bit pixel lane. The four
 * rotations are applied in scalar order because their E registers overlap.
 */
#define XBR555_V(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		__m128i _gate = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi16((PE), (PH)), _mm_cmpeq_epi16((PE), (PF))), _mm_set1_epi16(-1)); \
		if (!_mm_testz_si128(_gate, _gate)) { \
			__m128i _e = _mm_add_epi16(_mm_add_epi16(xbr555_dist3_v((PC), (PE), (PG)), xbr555_dist3_v((H5), (PI), (F4))), \
					_mm_slli_epi16(xbr555_dist_v((PH), (PF)), 2)); \
			__m128i _i = _mm_add_epi16(_mm_add_epi16(xbr555_dist3_v((PD), (PH), (I5)), xbr555_dist3_v((I4), (PF), (PB))), \
					_mm_slli_epi16(xbr555_dist_v((PE), (PI)), 2)); \
			__m128i _active = _mm_and_si128(_gate, _mm_cmpgt_epi16(_i, _e)); \
			if (!_mm_testz_si128(_active, _active)) { \
				__m128i _ke = xbr555_dist_v((PF), (PG)); \
				__m128i _ki = xbr555_dist_v((PH), (PC)); \
				__m128i _ex2 = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi16((PE), (PC)), _mm_cmpeq_epi16((PB), (PC))), _active); \
				__m128i _ex3 = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi16((PE), (PG)), _mm_cmpeq_epi16((PD), (PG))), _active); \
				__m128i _both = _mm_and_si128(_ex2, _ex3); \
				__m128i _leftup = _mm_and_si128(_both, _mm_and_si128(_mm_cmpeq_epi16(_ke, _mm_setzero_si128()), \
						_mm_cmpeq_epi16(_ki, _mm_setzero_si128()))); \
				__m128i _left = _mm_andnot_si128(_leftup, _mm_andnot_si128(_mm_cmpgt_epi16(_mm_slli_epi16(_ke, 1), _ki), _ex3)); \
				__m128i _up = _mm_andnot_si128(_mm_or_si128(_leftup, _left), \
						_mm_andnot_si128(_mm_cmpgt_epi16(_mm_slli_epi16(_ki, 1), _ke), _ex2)); \
				__m128i _dia = _mm_andnot_si128(_mm_or_si128(_mm_or_si128(_leftup, _left), _up), _active); \
				__m128i _dpf = xbr555_dist_v((PE), (PF)); \
				__m128i _dph = xbr555_dist_v((PE), (PH)); \
				__m128i _px = _mm_blendv_epi8((PF), (PH), _mm_cmpgt_epi16(_dpf, _dph)); \
				__m128i _e3 = (N3), _e1 = (N1), _e2 = (N2); \
				__m128i _side = _mm_or_si128(_left, _up); \
				_e3 = _mm_blendv_epi8(_e3, xbr555_71_v(_px, _e3), _leftup); \
				_e3 = _mm_blendv_epi8(_e3, xbr555_31_v(_px, (N3)), _side); \
				_e3 = _mm_blendv_epi8(_e3, xbr555_11_v((N3), _px), _dia); \
				__m128i _t2 = xbr555_31_v(_e2, _px); \
				__m128i _t1 = xbr555_31_v(_e1, _px); \
				(N3) = _e3; \
				(N2) = _mm_blendv_epi8(_e2, _t2, _mm_or_si128(_leftup, _left)); \
				(N1) = _mm_blendv_epi8(_mm_blendv_epi8(_e1, _t2, _leftup), _t1, _up); \
			} \
		} \
} while (0)

/*
 * Scalar border implementation, duplicated here so this kernel does not
 * depend on the global RGB16 masks or overwrite previously written pixels.
 */
static inline int xbr555_dist_s(interp_uint16 a, interp_uint16 b)
{
	int r = ((a >> 10) & 31) - ((b >> 10) & 31);
	int g = ((a >> 5) & 31) - ((b >> 5) & 31);
	int bl = (a & 31) - (b & 31);
	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (bl < 0) bl = -bl;
	return (3 * r + 6 * g + bl) * 8;
}

static inline int xbr555_dist3_s(interp_uint16 a, interp_uint16 b, interp_uint16 c)
{
	return xbr555_dist_s(a, b) + xbr555_dist_s(b, c);
}

static inline interp_uint16 xbr555_11_s(interp_uint16 a, interp_uint16 b)
{
	return (((a ^ b) >> 1) & 0x3DEF) + (a & b);
}

static inline interp_uint16 xbr555_weight_s(interp_uint16 a, interp_uint16 b, unsigned wa, unsigned shift)
{
	unsigned r = (wa * ((a >> 10) & 31) + ((b >> 10) & 31)) >> shift;
	unsigned g = (wa * ((a >> 5) & 31) + ((b >> 5) & 31)) >> shift;
	unsigned bl = (wa * (a & 31) + (b & 31)) >> shift;
	return (interp_uint16)((r << 10) | (g << 5) | bl);
}

static inline interp_uint16 xbr555_31_s(interp_uint16 a, interp_uint16 b)
{
	return xbr555_11_s(a, xbr555_11_s(a, b));
}

static inline interp_uint16 xbr555_71_s(interp_uint16 a, interp_uint16 b)
{
	return xbr555_weight_s(a, b, 7, 3);
}

#define XBR555_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned _e = xbr555_dist3_s((PC), (PE), (PG)) + xbr555_dist3_s((H5), (PI), (F4)) + 4 * xbr555_dist_s((PH), (PF)); \
			unsigned _i = xbr555_dist3_s((PD), (PH), (I5)) + xbr555_dist3_s((I4), (PF), (PB)) + 4 * xbr555_dist_s((PE), (PI)); \
			if (_e < _i) { \
				int _ex2 = (PE) != (PC) && (PB) != (PC); \
				int _ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned _ke = xbr555_dist_s((PF), (PG)); \
				unsigned _ki = xbr555_dist_s((PH), (PC)); \
				interp_uint16 _px = xbr555_dist_s((PE), (PF)) <= xbr555_dist_s((PE), (PH)) ? (PF) : (PH); \
				if (_ke == 0 && _ki == 0 && _ex3 && _ex2) { \
					(N3) = xbr555_71_s(_px, (N3)); \
					(N1) = (N2) = xbr555_31_s((N2), _px); \
				} else if (2 * _ke <= _ki && _ex3) { \
					(N3) = xbr555_31_s(_px, (N3)); \
					(N2) = xbr555_31_s((N2), _px); \
				} else if (_ke >= 2 * _ki && _ex2) { \
					(N3) = xbr555_31_s(_px, (N3)); \
					(N1) = xbr555_31_s((N1), _px); \
				} else { \
					(N3) = xbr555_11_s((N3), _px); \
				} \
			} \
		} \
} while (0)

static inline void xbr555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, const interp_uint16* src3,
	const interp_uint16* src4, unsigned count, unsigned x)
{
	unsigned xm2 = x >= 2 ? x - 2 : 0;
	unsigned xm1 = x >= 1 ? x - 1 : 0;
	unsigned xp1 = count - x >= 2 ? x + 1 : count - 1;
	unsigned xp2 = count - x >= 3 ? x + 2 : count - 1;

	interp_uint16 A1 = src0[xm1], B1 = src0[x], C1 = src0[xp1];
	interp_uint16 A0 = src1[xm2], PA = src1[xm1], PB = src1[x], PC = src1[xp1], C4 = src1[xp2];
	interp_uint16 D0 = src2[xm2], PD = src2[xm1], PE = src2[x], PF = src2[xp1], F4 = src2[xp2];
	interp_uint16 G0 = src3[xm2], PG = src3[xm1], PH = src3[x], PI = src3[xp1], I4 = src3[xp2];
	interp_uint16 G5 = src4[xm1], H5 = src4[x], I5 = src4[xp1];
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE;
	XBR555_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR555_S(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR555_S(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR555_S(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1")))
void xbr2x_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* Handle the two leftmost pixels without any output scratch memory. */
	for (; x < count && x < 2; ++x)
		xbr555_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* x>=2, and count-x>=10 ensures all loads, including the +2 offset for
	 * the final lane, remain in the source row (at most x+9).
	 */
	for (; count - x >= 10; x += 8) {
#define LD555(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A1 = LD555(src0, -1), B1 = LD555(src0, 0), C1 = LD555(src0, 1);
		__m128i A0 = LD555(src1, -2), PA = LD555(src1, -1), PB = LD555(src1, 0), PC = LD555(src1, 1), C4 = LD555(src1, 2);
		__m128i D0 = LD555(src2, -2), PD = LD555(src2, -1), PE = LD555(src2, 0), PF = LD555(src2, 1), F4 = LD555(src2, 2);
		__m128i G0 = LD555(src3, -2), PG = LD555(src3, -1), PH = LD555(src3, 0), PI = LD555(src3, 1), I4 = LD555(src3, 2);
		__m128i G5 = LD555(src4, -1), H5 = LD555(src4, 0), I5 = LD555(src4, 1);
#undef LD555
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBR555_V(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR555_V(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR555_V(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR555_V(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

		__m128i top_lo = _mm_unpacklo_epi16(E0, E1);
		__m128i top_hi = _mm_unpackhi_epi16(E0, E1);
		__m128i bot_lo = _mm_unpacklo_epi16(E2, E3);
		__m128i bot_hi = _mm_unpackhi_epi16(E2, E3);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), top_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 8), top_hi);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), bot_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 8), bot_hi);
	}

	for (; x < count; ++x)
		xbr555_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

__attribute__((target("sse4.1"))) static inline __m128i xbr32_sse_dist(__m128i a, __m128i b)
{
	const __m128i byte_mask = _mm_set1_epi32(255);
	__m128i r = _mm_abs_epi32(_mm_sub_epi32(_mm_and_si128(_mm_srli_epi32(a, 16), byte_mask), _mm_and_si128(_mm_srli_epi32(b, 16), byte_mask)));
	__m128i g = _mm_abs_epi32(_mm_sub_epi32(_mm_and_si128(_mm_srli_epi32(a, 8), byte_mask), _mm_and_si128(_mm_srli_epi32(b, 8), byte_mask)));
	__m128i bl = _mm_abs_epi32(_mm_sub_epi32(_mm_and_si128(a, byte_mask), _mm_and_si128(b, byte_mask)));

	/* 3*r + 6*g + bl, maximum 2550 per lane. */
	r = _mm_add_epi32(_mm_slli_epi32(r, 1), r);
	g = _mm_add_epi32(_mm_slli_epi32(g, 2), _mm_slli_epi32(g, 1));
	return _mm_add_epi32(_mm_add_epi32(r, g), bl);
}

__attribute__((target("sse4.1"))) static inline __m128i xbr32_sse_dist3(__m128i a, __m128i b, __m128i c)
{
	return _mm_add_epi32(xbr32_sse_dist(a, b), xbr32_sse_dist(b, c));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr32_sse_11(__m128i a, __m128i b)
{
	const __m128i mask = _mm_set1_epi32((int)0xFF7F7F7FU);
	return _mm_add_epi32(_mm_and_si128(_mm_srli_epi32(_mm_xor_si128(a, b), 1), mask),
		       _mm_and_si128(a, b));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr32_sse_31(__m128i a, __m128i b)
{
	return xbr32_sse_11(a, xbr32_sse_11(a, b));
}

__attribute__((target("sse4.1"))) static inline __m128i xbr32_sse_71(__m128i a, __m128i b)
{
	const __m128i rbmask = _mm_set1_epi32(0x00FF00FF);
	const __m128i gmask = _mm_set1_epi32(0x0000FF00);
	__m128i ar = _mm_and_si128(a, rbmask);
	__m128i br = _mm_and_si128(b, rbmask);
	__m128i ag = _mm_and_si128(a, gmask);
	__m128i bg = _mm_and_si128(b, gmask);
	__m128i rb = _mm_and_si128(_mm_srli_epi32(_mm_add_epi32(_mm_sub_epi32(_mm_slli_epi32(ar, 3), ar), br), 3), rbmask);
	__m128i g = _mm_and_si128(_mm_srli_epi32(_mm_add_epi32(_mm_sub_epi32(_mm_slli_epi32(ag, 3), ag), bg), 3), gmask);
	return _mm_or_si128(rb, g);
}

#define XBR32_SSE(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		__m128i _eq_ph = _mm_cmpeq_epi32((PE), (PH)); \
		__m128i _eq_pf = _mm_cmpeq_epi32((PE), (PF)); \
		__m128i _gate = _mm_andnot_si128(_mm_or_si128(_eq_ph, _eq_pf), _mm_set1_epi32(-1)); \
		if (!_mm_testz_si128(_gate, _gate)) { \
			__m128i _e = _mm_add_epi32(_mm_add_epi32(xbr32_sse_dist3((PC), (PE), (PG)), \
					xbr32_sse_dist3((H5), (PI), (F4))), \
					_mm_slli_epi32(xbr32_sse_dist((PH), (PF)), 2)); \
			__m128i _i = _mm_add_epi32(_mm_add_epi32(xbr32_sse_dist3((PD), (PH), (I5)), \
					xbr32_sse_dist3((I4), (PF), (PB))), \
					_mm_slli_epi32(xbr32_sse_dist((PE), (PI)), 2)); \
			__m128i _active = _mm_and_si128(_gate, _mm_cmpgt_epi32(_i, _e)); \
			if (!_mm_testz_si128(_active, _active)) { \
				const __m128i _zero = _mm_setzero_si128(); \
				__m128i _ke = xbr32_sse_dist((PF), (PG)); \
				__m128i _ki = xbr32_sse_dist((PH), (PC)); \
				__m128i _ex2 = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi32((PE), (PC)), \
						_mm_cmpeq_epi32((PB), (PC))), _active); \
				__m128i _ex3 = _mm_andnot_si128(_mm_or_si128(_mm_cmpeq_epi32((PE), (PG)), \
						_mm_cmpeq_epi32((PD), (PG))), _active); \
				__m128i _both = _mm_and_si128(_ex2, _ex3); \
				__m128i _leftup = _mm_and_si128(_both, _mm_and_si128( \
							_mm_cmpeq_epi32(_ke, _zero), _mm_cmpeq_epi32(_ki, _zero))); \
				__m128i _left = _mm_andnot_si128(_leftup, _mm_andnot_si128( \
							_mm_cmpgt_epi32(_mm_slli_epi32(_ke, 1), _ki), _ex3)); \
				__m128i _up = _mm_andnot_si128(_mm_or_si128(_leftup, _left), \
						_mm_andnot_si128(_mm_cmpgt_epi32(_mm_slli_epi32(_ki, 1), _ke), _ex2)); \
				__m128i _dia = _mm_andnot_si128(_mm_or_si128(_mm_or_si128(_leftup, _left), _up), _active); \
				__m128i _dpf = xbr32_sse_dist((PE), (PF)); \
				__m128i _dph = xbr32_sse_dist((PE), (PH)); \
				__m128i _px = _mm_blendv_epi8((PF), (PH), _mm_cmpgt_epi32(_dpf, _dph)); \
				__m128i _e3 = (N3); \
				__m128i _e1 = (N1); \
				__m128i _e2 = (N2); \
				__m128i _side = _mm_or_si128(_left, _up); \
				_e3 = _mm_blendv_epi8(_e3, xbr32_sse_71(_px, _e3), _leftup); \
				_e3 = _mm_blendv_epi8(_e3, xbr32_sse_31(_px, (N3)), _side); \
				_e3 = _mm_blendv_epi8(_e3, xbr32_sse_11((N3), _px), _dia); \
				__m128i _t2 = xbr32_sse_31(_e2, _px); \
				__m128i _t1 = xbr32_sse_31(_e1, _px); \
				(N3) = _e3; \
				(N2) = _mm_blendv_epi8(_e2, _t2, _mm_or_si128(_leftup, _left)); \
				(N1) = _mm_blendv_epi8(_mm_blendv_epi8(_e1, _t2, _leftup), _t1, _up); \
			} \
		} \
} while (0)

/*
 * SIMD decisions and interpolation. The scalar reference remains available
 * independently as xbr2x_32_def().
 */

/*
 * Scalar handling for the first two pixels and the final partial SIMD block.
 * The source is always indexed relative to the FULL row, so that clamping
 * happens at the real image boundary. Outputs are written exactly once.
 */
#define XBR32_EDGE(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned e = interp_32_dist3((PC), (PE), (PG)) + \
				interp_32_dist3((H5), (PI), (F4)) + \
				4 * interp_32_dist((PH), (PF)); \
			unsigned d = interp_32_dist3((PD), (PH), (I5)) + \
				interp_32_dist3((I4), (PF), (PB)) + \
				4 * interp_32_dist((PE), (PI)); \
			if (e < d) { \
				int ex2 = (PE) != (PC) && (PB) != (PC); \
				int ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned ke = interp_32_dist((PF), (PG)); \
				unsigned ki = interp_32_dist((PH), (PC)); \
				interp_uint32 px = interp_32_dist((PE), (PF)) <= interp_32_dist((PE), (PH)) ? (PF) : (PH); \
				if (ke == 0 && ki == 0 && ex3 && ex2) { \
					(N3) = interp_32_71(px, (N3)); \
					(N1) = (N2) = interp_32_31((N2), px); \
				} else if (2 * ke <= ki && ex3) { \
					(N3) = interp_32_31(px, (N3)); \
					(N2) = interp_32_31((N2), px); \
				} else if (ke >= 2 * ki && ex2) { \
					(N3) = interp_32_31(px, (N3)); \
					(N1) = interp_32_31((N1), px); \
				} else { \
					(N3) = interp_32_11((N3), px); \
				} \
			} \
		} \
} while (0)

__attribute__((target("sse4.1"))) static inline void xbr32_edge_pixel(interp_uint32* restrict dst0,
	interp_uint32* restrict dst1,
	const interp_uint32* src0,
	const interp_uint32* src1,
	const interp_uint32* src2,
	const interp_uint32* src3,
	const interp_uint32* src4,
	unsigned count, unsigned x)
{
	unsigned xm2 = x >= 2 ? x - 2 : 0;
	unsigned xm1 = x >= 1 ? x - 1 : 0;
	unsigned xp1 = count - x >= 2 ? x + 1 : count - 1;
	unsigned xp2 = count - x >= 3 ? x + 2 : count - 1;

	interp_uint32 A1 = src0[xm1], B1 = src0[x], C1 = src0[xp1];
	interp_uint32 A0 = src1[xm2], PA = src1[xm1], PB = src1[x], PC = src1[xp1], C4 = src1[xp2];
	interp_uint32 D0 = src2[xm2], PD = src2[xm1], PE = src2[x], PF = src2[xp1], F4 = src2[xp2];
	interp_uint32 G0 = src3[xm2], PG = src3[xm1], PH = src3[x], PI = src3[xp1], I4 = src3[xp2];
	interp_uint32 G5 = src4[xm1], H5 = src4[x], I5 = src4[xp1];

	interp_uint32 E0 = PE, E1 = PE, E2 = PE, E3 = PE;
	XBR32_EDGE(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR32_EDGE(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR32_EDGE(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR32_EDGE(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

/*
 * One source pixel becomes four output pixels. Every output location
 * receives exactly one store. No temporary output arrays, no overwriting.
 */
__attribute__((target("sse4.1"))) void xbr2x_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, const interp_uint32* restrict src3,
	const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* First two pixels require a clamped left neighborhood. */
	for (; x < count && x < 2; ++x)
		xbr32_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* Interior: four independent input pixels per 128-bit SIMD vector.
	 * count - x >= 6 guarantees valid offsets [-2, +5] in all five rows.
	 */
	for (; count - x >= 6; x += 4) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A1 = LD(src0, -1), B1 = LD(src0, 0), C1 = LD(src0, 1);
		__m128i A0 = LD(src1, -2), PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
		__m128i D0 = LD(src2, -2), PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
		__m128i G0 = LD(src3, -2), PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
		__m128i G5 = LD(src4, -1), H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE;

		XBR32_SSE(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR32_SSE(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR32_SSE(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR32_SSE(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

		__m128i top_lo = _mm_unpacklo_epi32(E0, E1);
		__m128i top_hi = _mm_unpackhi_epi32(E0, E1);
		__m128i bot_lo = _mm_unpacklo_epi32(E2, E3);
		__m128i bot_hi = _mm_unpackhi_epi32(E2, E3);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), top_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 4), top_hi);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), bot_lo);
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 4), bot_hi);
	}

	/* Remaining zero to five pixels: directly written, no scratch buffers. */
	for (; x < count; ++x)
		xbr32_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}
#endif

