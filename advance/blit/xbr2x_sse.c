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

#include "interp_sse.h"

#define XBR_SSE(fmt, eq, gt, add, shl, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) \
	do { \
		__m128i _gate = _mm_andnot_si128(_mm_or_si128(eq((PE), (PH)), eq((PE), (PF))), _mm_set1_epi32(-1)); \
		if (!_mm_testz_si128(_gate, _gate)) { \
			__m128i _e = interp_ ## fmt ## _score_sse((PC), (PE), (PG), (H5), (PI), (F4), (PH), (PF)); \
			__m128i _i = interp_ ## fmt ## _score_sse((PD), (PH), (I5), (I4), (PF), (PB), (PE), (PI)); \
			__m128i _active = _mm_and_si128(_gate, gt(_i, _e)); \
			if (!_mm_testz_si128(_active, _active)) { \
				__m128i _ke = interp_ ## fmt ## _dist_sse((PF), (PG)); \
				__m128i _ki = interp_ ## fmt ## _dist_sse((PH), (PC)); \
				__m128i _ex2 = _mm_andnot_si128(_mm_or_si128(eq((PE), (PC)), eq((PB), (PC))), _active); \
				__m128i _ex3 = _mm_andnot_si128(_mm_or_si128(eq((PE), (PG)), eq((PD), (PG))), _active); \
				__m128i _both = _mm_and_si128(_ex2, _ex3); \
				__m128i _leftup = _mm_and_si128(_both, _mm_and_si128(eq(_ke, _mm_setzero_si128()), \
						eq(_ki, _mm_setzero_si128()))); \
				__m128i _left = _mm_andnot_si128(_leftup, _mm_andnot_si128(gt(shl(_ke, 1), _ki), _ex3)); \
				__m128i _up = _mm_andnot_si128(_mm_or_si128(_leftup, _left), \
						_mm_andnot_si128(gt(shl(_ki, 1), _ke), _ex2)); \
				__m128i _dia = _mm_andnot_si128(_mm_or_si128(_mm_or_si128(_leftup, _left), _up), _active); \
				__m128i _dpf = interp_ ## fmt ## _dist_sse((PE), (PF)); \
				__m128i _dph = interp_ ## fmt ## _dist_sse((PE), (PH)); \
				__m128i _px = _mm_blendv_epi8((PF), (PH), gt(_dpf, _dph)); \
				__m128i _e3 = (N3), _e1 = (N1), _e2 = (N2); \
				__m128i _side = _mm_or_si128(_left, _up); \
				_e3 = _mm_blendv_epi8(_e3, interp_ ## fmt ## _71_sse(_px, _e3), _leftup); \
				_e3 = _mm_blendv_epi8(_e3, interp_ ## fmt ## _31_sse(_px, (N3)), _side); \
				_e3 = _mm_blendv_epi8(_e3, interp_ ## fmt ## _11_sse((N3), _px), _dia); \
				__m128i _t2 = interp_ ## fmt ## _31_sse(_e2, _px); \
				__m128i _t1 = interp_ ## fmt ## _31_sse(_e1, _px); \
				(N3) = _e3; \
				(N2) = _mm_blendv_epi8(_e2, _t2, _mm_or_si128(_leftup, _left)); \
				(N1) = _mm_blendv_epi8(_mm_blendv_epi8(_e1, _t2, _leftup), _t1, _up); \
			} \
		} \
	} while (0)

static inline void xbr2_565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
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
	XBR2(16_565, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR2(16_565, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR2(16_565, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR2(16_565, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void xbr2x_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* Handle the two leftmost pixels without any output scratch memory. */
	for (; x < count && x < 2; ++x)
		xbr2_565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

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
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

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
		xbr2_565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

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
	XBR2(16_555, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR2(16_555, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR2(16_555, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR2(16_555, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void xbr2x_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
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
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

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
	XBR2(32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR2(32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR2(32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR2(32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

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

		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

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

