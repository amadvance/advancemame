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

#include "xbr4x.h"

#ifdef CONFIG_X86

#include "interp_sse.h"

#define XBR_SSE(fmt, eq, gt, add, shl, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3, N4, N5, N6, N7, N8, N9, N10, N11, N12, N13, N14, N15) \
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
				__m128i _leftup = _mm_and_si128(_mm_and_si128(_ex2, _ex3), _mm_and_si128(eq(_ke, _mm_setzero_si128()), eq(_ki, _mm_setzero_si128()))); \
				__m128i _left = _mm_andnot_si128(_leftup, _mm_andnot_si128(gt(shl(_ke, 1), _ki), _ex3)); \
				__m128i _up = _mm_andnot_si128(_mm_or_si128(_leftup, _left), _mm_andnot_si128(gt(shl(_ki, 1), _ke), _ex2)); \
				__m128i _dia = _mm_andnot_si128(_mm_or_si128(_mm_or_si128(_leftup, _left), _up), _active); \
				__m128i _px = _mm_blendv_epi8((PF), (PH), gt(interp_ ## fmt ## _dist_sse((PE), (PF)), interp_ ## fmt ## _dist_sse((PE), (PH)))); \
				__m128i _lup13 = interp_ ## fmt ## _31_sse(_px, (N13)); \
				__m128i _lup12 = interp_ ## fmt ## _31_sse((N12), _px); \
				(N7) = _mm_blendv_epi8((N7), _lup13, _leftup); \
				(N13) = _mm_blendv_epi8((N13), _lup13, _leftup); \
				(N3) = _mm_blendv_epi8((N3), _lup12, _leftup); \
				(N10) = _mm_blendv_epi8((N10), _lup12, _leftup); \
				(N12) = _mm_blendv_epi8((N12), _lup12, _leftup); \
				(N11) = _mm_blendv_epi8((N11), _px, _leftup); \
				(N14) = _mm_blendv_epi8((N14), _px, _leftup); \
				(N15) = _mm_blendv_epi8((N15), _px, _leftup); \
				(N11) = _mm_blendv_epi8((N11), interp_ ## fmt ## _31_sse(_px, (N11)), _left); \
				(N13) = _mm_blendv_epi8((N13), interp_ ## fmt ## _31_sse(_px, (N13)), _left); \
				(N10) = _mm_blendv_epi8((N10), interp_ ## fmt ## _31_sse((N10), _px), _left); \
				(N12) = _mm_blendv_epi8((N12), interp_ ## fmt ## _31_sse((N12), _px), _left); \
				(N14) = _mm_blendv_epi8((N14), _px, _left); \
				(N15) = _mm_blendv_epi8((N15), _px, _left); \
				(N14) = _mm_blendv_epi8((N14), interp_ ## fmt ## _31_sse(_px, (N14)), _up); \
				(N7) = _mm_blendv_epi8((N7), interp_ ## fmt ## _31_sse(_px, (N7)), _up); \
				(N10) = _mm_blendv_epi8((N10), interp_ ## fmt ## _31_sse((N10), _px), _up); \
				(N3) = _mm_blendv_epi8((N3), interp_ ## fmt ## _31_sse((N3), _px), _up); \
				(N11) = _mm_blendv_epi8((N11), _px, _up); \
				(N15) = _mm_blendv_epi8((N15), _px, _up); \
				(N11) = _mm_blendv_epi8((N11), interp_ ## fmt ## _11_sse((N11), _px), _dia); \
				(N14) = _mm_blendv_epi8((N14), interp_ ## fmt ## _11_sse((N14), _px), _dia); \
				(N15) = _mm_blendv_epi8((N15), _px, _dia); \
			} \
		} \
	} while (0)

static inline void xbr4_565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count, unsigned x)
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
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
	XBR4(16_565, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
	XBR4(16_565, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
	XBR4(16_565, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	XBR4(16_565, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
	dst0[4 * x + 0] = E0;
	dst0[4 * x + 1] = E1;
	dst0[4 * x + 2] = E2;
	dst0[4 * x + 3] = E3;
	dst1[4 * x + 0] = E4;
	dst1[4 * x + 1] = E5;
	dst1[4 * x + 2] = E6;
	dst1[4 * x + 3] = E7;
	dst2[4 * x + 0] = E8;
	dst2[4 * x + 1] = E9;
	dst2[4 * x + 2] = E10;
	dst2[4 * x + 3] = E11;
	dst3[4 * x + 0] = E12;
	dst3[4 * x + 1] = E13;
	dst3[4 * x + 2] = E14;
	dst3[4 * x + 3] = E15;
}

__attribute__((target("sse4.1"))) void xbr4x_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	if (count == 0)
		return;
	for (; x < count && x < 2; ++x)
		xbr4_565_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

	/* 8 source pixels per block; access range x-2 to x+9. */
	for (; count - x >= 10; x += 8) {
#define LD4(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		__m128i A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		__m128i D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		__m128i G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		__m128i G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_SSE(16_565, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_16_565_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_565_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_565_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_565_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}
	for (; x < count; ++x)
		xbr4_565_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

static inline void xbr4_555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count, unsigned x)
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
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
	XBR4(16_555, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
	XBR4(16_555, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
	XBR4(16_555, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	XBR4(16_555, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
	dst0[4 * x + 0] = E0;
	dst0[4 * x + 1] = E1;
	dst0[4 * x + 2] = E2;
	dst0[4 * x + 3] = E3;
	dst1[4 * x + 0] = E4;
	dst1[4 * x + 1] = E5;
	dst1[4 * x + 2] = E6;
	dst1[4 * x + 3] = E7;
	dst2[4 * x + 0] = E8;
	dst2[4 * x + 1] = E9;
	dst2[4 * x + 2] = E10;
	dst2[4 * x + 3] = E11;
	dst3[4 * x + 0] = E12;
	dst3[4 * x + 1] = E13;
	dst3[4 * x + 2] = E14;
	dst3[4 * x + 3] = E15;
}

__attribute__((target("sse4.1"))) void xbr4x_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	if (count == 0)
		return;
	for (; x < count && x < 2; ++x)
		xbr4_555_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

	/* 8 source pixels per block; access range x-2 to x+9. */
	for (; count - x >= 10; x += 8) {
#define LD4(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		__m128i A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		__m128i D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		__m128i G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		__m128i G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_SSE(16_555, _mm_cmpeq_epi16, _mm_cmpgt_epi16, _mm_add_epi16, _mm_slli_epi16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_16_555_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_555_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_555_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_555_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}
	for (; x < count; ++x)
		xbr4_555_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

static inline void xbr4_32_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count, unsigned x)
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
	interp_uint32 E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
	XBR4(32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
	XBR4(32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
	XBR4(32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	XBR4(32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
	dst0[4 * x + 0] = E0;
	dst0[4 * x + 1] = E1;
	dst0[4 * x + 2] = E2;
	dst0[4 * x + 3] = E3;
	dst1[4 * x + 0] = E4;
	dst1[4 * x + 1] = E5;
	dst1[4 * x + 2] = E6;
	dst1[4 * x + 3] = E7;
	dst2[4 * x + 0] = E8;
	dst2[4 * x + 1] = E9;
	dst2[4 * x + 2] = E10;
	dst2[4 * x + 3] = E11;
	dst3[4 * x + 0] = E12;
	dst3[4 * x + 1] = E13;
	dst3[4 * x + 2] = E14;
	dst3[4 * x + 3] = E15;
}

__attribute__((target("sse4.1"))) void xbr4x_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;
	if (count == 0)
		return;
	for (; x < count && x < 2; ++x)
		xbr4_32_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

	/* 4 source pixels per block; access range x-2 to x+5. */
	for (; count - x >= 6; x += 4) {
#define LD4(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		__m128i A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		__m128i D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		__m128i G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		__m128i G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_SSE(32, _mm_cmpeq_epi32, _mm_cmpgt_epi32, _mm_add_epi32, _mm_slli_epi32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_32_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_32_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_32_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_32_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}
	for (; x < count; ++x)
		xbr4_32_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

#endif

