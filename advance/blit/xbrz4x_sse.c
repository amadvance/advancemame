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

#include "xbrz4x.h"

#ifdef CONFIG_X86

#include "xbrz_sse.h"

/**
 * SIMD version of XBRZ4. Each condition is a lane mask and each
 * conditional assignment a blend. The only branch is the early exit
 * when no lane has BR != XBRZ_NONE.
 * When both SHALLOW and STEEP are set, the later blend wins, matching
 * the "shallow && steep" case of the C implementation.
 */
#define XBRZ_SSE(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, N3, N7, N10, N11, N12, N13, N14, N15) \
	do { \
		if (!_mm_testz_si128((BR), (BR))) { \
			__m128i _line, _px, _shallow, _steep, _n10, _n11, _n14, _n15; \
			XBRZ_LINE_SSE(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, _line, _px, _shallow, _steep); \
			_n10 = _mm_blendv_epi8((N10), interp_ ## fmt ## _31_sse((N10), _px), _mm_or_si128(_shallow, _steep)); \
			_n10 = _mm_blendv_epi8(_n10, interp_ ## fmt ## _53_sse((N10), _px), _mm_and_si128(_shallow, _steep)); /* approximation of 1/3 */ \
			_n15 = _mm_blendv_epi8(interp_ ## fmt ## _53_sse(_px, (N15)), _px, _line); /* approximation of 68/100 */ \
			_n14 = _mm_blendv_epi8(interp_ ## fmt ## _71_sse((N14), _px), interp_ ## fmt ## _11_sse((N14), _px), _line); /* approximation of 9/100 */ \
			_n14 = _mm_blendv_epi8(_n14, interp_ ## fmt ## _31_sse(_px, (N14)), _steep); \
			_n14 = _mm_blendv_epi8(_n14, _px, _shallow); \
			_n11 = _mm_blendv_epi8(interp_ ## fmt ## _71_sse((N11), _px), interp_ ## fmt ## _11_sse((N11), _px), _line); /* approximation of 9/100 */ \
			_n11 = _mm_blendv_epi8(_n11, interp_ ## fmt ## _31_sse(_px, (N11)), _shallow); \
			_n11 = _mm_blendv_epi8(_n11, _px, _steep); \
			(N15) = _mm_blendv_epi8((N15), _n15, (BR)); \
			(N14) = _mm_blendv_epi8((N14), _n14, (BR)); \
			(N11) = _mm_blendv_epi8((N11), _n11, (BR)); \
			(N10) = _n10; \
			(N13) = _mm_blendv_epi8((N13), interp_ ## fmt ## _31_sse(_px, (N13)), _shallow); \
			(N12) = _mm_blendv_epi8((N12), interp_ ## fmt ## _31_sse((N12), _px), _shallow); \
			(N7) = _mm_blendv_epi8((N7), interp_ ## fmt ## _31_sse(_px, (N7)), _steep); \
			(N3) = _mm_blendv_epi8((N3), interp_ ## fmt ## _31_sse((N3), _px), _steep); \
		} \
	} while (0)

/**
 * Classify and blend one block. The C order of the rotations BR, TR, TL, BL
 * is kept, since rotations can modify the same output pixel.
 * The only branches are the early exits when all the flags are XBRZ_NONE.
 */
#define XBRZ4X_SSE(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, PTL, PTLD, PBL, PBLD, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15) \
	do { \
		__m128i _eq_ef = xbrz_ ## w ## _eq_sse((PE), (PF)); \
		if (xbrz_ ## w ## _flat_sse(_eq_ef, (PTL), (PBL))) { \
			/* all the flags are XBRZ_NONE, and so the next carried ones */ \
			(PTL) = (PTLD) = (PBL) = (PBLD) = _mm_setzero_si128(); \
		} else { \
			__m128i dEA, dEC, dEG, dEI, tl, tld, tr, trd, bl, bld, br, brd, _any; \
			XBRZ_CLASSIFY_SSE(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, _eq_ef, dEA, dEC, dEG, dEI, tl, tld, tr, trd, bl, bld, br, brd, PTL, PTLD, PBL, PBLD); \
			_any = _mm_or_si128(_mm_or_si128(tl, tr), _mm_or_si128(bl, br)); \
			if (!_mm_testz_si128(_any, _any)) { \
				__m128i dEB, dED, dEF, dEH, dAB, dBC, dCF, dFI, dHI, dGH, dDG, dAD; \
				XBRZ_DIST_SSE(fmt, PA, PB, PC, PD, PE, PF, PG, PH, PI, dEB, dED, dEF, dEH, dAB, dBC, dCF, dFI, dHI, dGH, dDG, dAD); \
				XBRZ_SSE(fmt, w, PB, PC, PD, PE, PF, PG, PH, PI, dEF, dEH, dEG, dEC, dEI, dGH, dHI, dFI, dCF, tr, bl, br, brd, E3, E7, E10, E11, E12, E13, E14, E15); \
				XBRZ_SSE(fmt, w, PD, PA, PH, PE, PB, PI, PF, PC, dEB, dEF, dEI, dEA, dEC, dFI, dCF, dBC, dAB, tl, br, tr, trd, E0, E1, E6, E2, E15, E11, E7, E3); \
				XBRZ_SSE(fmt, w, PH, PG, PF, PE, PD, PC, PB, PA, dED, dEB, dEC, dEG, dEA, dBC, dAB, dAD, dDG, bl, tr, tl, tld, E12, E8, E5, E4, E3, E2, E1, E0); \
				XBRZ_SSE(fmt, w, PF, PI, PB, PE, PH, PA, PD, PG, dEH, dED, dEA, dEI, dEG, dAD, dDG, dGH, dHI, br, tl, bl, bld, E15, E14, E9, E13, E0, E4, E8, E12); \
			} \
		} \
	} while (0)

static inline void xbrz4_565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4,
	unsigned count, unsigned x)
{
	interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint16 E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(16_565, 0xFFFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = E4 = E5 = E6 = E7 = PE;
	E8 = E9 = E10 = E11 = E12 = E13 = E14 = E15 = PE;
	XBRZ4(16_565, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E3, E7, E10, E11, E12, E13, E14, E15);
	XBRZ4(16_565, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E1, E6, E2, E15, E11, E7, E3);
	XBRZ4(16_565, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E12, E8, E5, E4, E3, E2, E1, E0);
	XBRZ4(16_565, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E15, E14, E9, E13, E0, E4, E8, E12);
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

__attribute__((target("sse4.1"))) void xbrz4x_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	__m128i ptl, ptld, pbl, pbld;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz4_565_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

	/* the SIMD loop carries the right vertices of the previous pixel */
	if (count - x >= 10)
		XBRZ_START_SSE(16_565, 16, 0xFFFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

	/* SIMD loads require one pixel of padding on the left and two on the right */
	for (; count - x >= 10; x += 8) {
#define LD4(row, off) xbrz_16_565_load_sse((row) + x + (off))
		__m128i B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		__m128i PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		__m128i PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		__m128i PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		__m128i H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE;
		__m128i E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBRZ4X_SSE(16_565, 16, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		interp_16_565_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_565_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_565_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_565_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}

	for (; x < count; ++x)
		xbrz4_565_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

static inline void xbrz4_555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4,
	unsigned count, unsigned x)
{
	interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint16 E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(16_555, 0x7FFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = E4 = E5 = E6 = E7 = PE;
	E8 = E9 = E10 = E11 = E12 = E13 = E14 = E15 = PE;
	XBRZ4(16_555, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E3, E7, E10, E11, E12, E13, E14, E15);
	XBRZ4(16_555, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E1, E6, E2, E15, E11, E7, E3);
	XBRZ4(16_555, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E12, E8, E5, E4, E3, E2, E1, E0);
	XBRZ4(16_555, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E15, E14, E9, E13, E0, E4, E8, E12);
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

__attribute__((target("sse4.1"))) void xbrz4x_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	__m128i ptl, ptld, pbl, pbld;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz4_555_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

	/* the SIMD loop carries the right vertices of the previous pixel */
	if (count - x >= 10)
		XBRZ_START_SSE(16_555, 16, 0x7FFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

	/* SIMD loads require one pixel of padding on the left and two on the right */
	for (; count - x >= 10; x += 8) {
#define LD4(row, off) xbrz_16_555_load_sse((row) + x + (off))
		__m128i B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		__m128i PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		__m128i PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		__m128i PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		__m128i H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE;
		__m128i E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBRZ4X_SSE(16_555, 16, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		interp_16_555_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_555_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_555_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_555_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}

	for (; x < count; ++x)
		xbrz4_555_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

static inline void xbrz4_32_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4,
	unsigned count, unsigned x)
{
	interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint32 E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(32, 0xFFFFFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = E4 = E5 = E6 = E7 = PE;
	E8 = E9 = E10 = E11 = E12 = E13 = E14 = E15 = PE;
	XBRZ4(32, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E3, E7, E10, E11, E12, E13, E14, E15);
	XBRZ4(32, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E1, E6, E2, E15, E11, E7, E3);
	XBRZ4(32, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E12, E8, E5, E4, E3, E2, E1, E0);
	XBRZ4(32, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E15, E14, E9, E13, E0, E4, E8, E12);
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

__attribute__((target("sse4.1"))) void xbrz4x_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;
	__m128i ptl, ptld, pbl, pbld;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz4_32_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

	/* the SIMD loop carries the right vertices of the previous pixel */
	if (count - x >= 6)
		XBRZ_START_SSE(32, 32, 0xFFFFFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

	/* count - x >= 6 guarantees valid offsets [-1, +5] in all five rows */
	for (; count - x >= 6; x += 4) {
#define LD(row, off) xbrz_32_load_sse((row) + x + (off))
		__m128i B1 = LD(src0, 0), C1 = LD(src0, 1);
		__m128i PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
		__m128i PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
		__m128i PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
		__m128i H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE;
		__m128i E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBRZ4X_SSE(32, 32, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		interp_32_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_32_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_32_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_32_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}

	for (; x < count; ++x)
		xbrz4_32_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

#endif

