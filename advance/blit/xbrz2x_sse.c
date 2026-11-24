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

#include "xbrz2x.h"

#ifdef CONFIG_X86

#include "xbrz_sse.h"

/**
 * SIMD version of XBRZ2. Each condition is a lane mask and each
 * conditional assignment a blend. The only branch is the early exit
 * when no lane has BR != XBRZ_NONE.
 */
#define XBRZ_SSE(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, N1, N2, N3) \
	do { \
		if (!_mm_testz_si128((BR), (BR))) { \
			__m128i _line, _px, _shallow, _steep, _n3; \
			XBRZ_LINE_SSE(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, _line, _px, _shallow, _steep); \
			_n3 = _mm_blendv_epi8(interp_ ## fmt ## _31_sse((N3), _px), interp_ ## fmt ## _11_sse((N3), _px), _line); /* approximation of 21/100 */ \
			_n3 = _mm_blendv_epi8(_n3, interp_ ## fmt ## _31_sse(_px, (N3)), _mm_or_si128(_shallow, _steep)); \
			_n3 = _mm_blendv_epi8(_n3, interp_ ## fmt ## _71_sse(_px, (N3)), _mm_and_si128(_shallow, _steep)); /* approximation of 5/6 */ \
			(N3) = _mm_blendv_epi8((N3), _n3, (BR)); \
			(N2) = _mm_blendv_epi8((N2), interp_ ## fmt ## _31_sse((N2), _px), _shallow); \
			(N1) = _mm_blendv_epi8((N1), interp_ ## fmt ## _31_sse((N1), _px), _steep); \
		} \
	} while (0)

/**
 * Classify and blend one block. The C order of the rotations BR, TR, TL, BL
 * is kept, since rotations can modify the same output pixel.
 * The only branches are the early exits when all the flags are XBRZ_NONE.
 */
#define XBRZ2X_SSE(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, PTL, PTLD, PBL, PBLD, E0, E1, E2, E3) \
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
				XBRZ_SSE(fmt, w, PB, PC, PD, PE, PF, PG, PH, PI, dEF, dEH, dEG, dEC, dEI, dGH, dHI, dFI, dCF, tr, bl, br, brd, E1, E2, E3); \
				XBRZ_SSE(fmt, w, PD, PA, PH, PE, PB, PI, PF, PC, dEB, dEF, dEI, dEA, dEC, dFI, dCF, dBC, dAB, tl, br, tr, trd, E0, E3, E1); \
				XBRZ_SSE(fmt, w, PH, PG, PF, PE, PD, PC, PB, PA, dED, dEB, dEC, dEG, dEA, dBC, dAB, dAD, dDG, bl, tr, tl, tld, E2, E1, E0); \
				XBRZ_SSE(fmt, w, PF, PI, PB, PE, PH, PA, PD, PG, dEH, dED, dEA, dEI, dEG, dAD, dDG, dGH, dHI, br, tl, bl, bld, E3, E0, E2); \
			} \
		} \
	} while (0)

static inline void xbrz2_565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, const interp_uint16* src3,
	const interp_uint16* src4, unsigned count, unsigned x)
{
	interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint16 E0, E1, E2, E3;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(16_565, 0xFFFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = PE;
	XBRZ2(16_565, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E1, E2, E3);
	XBRZ2(16_565, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E3, E1);
	XBRZ2(16_565, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E2, E1, E0);
	XBRZ2(16_565, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void xbrz2x_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	__m128i ptl, ptld, pbl, pbld;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz2_565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* the SIMD loop carries the right vertices of the previous pixel */
	if (count - x >= 10)
		XBRZ_START_SSE(16_565, 16, 0xFFFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

	/* x>=1, and count-x>=10 ensures all loads, including the +2 offset for
	 * the final lane, remain in the source row (at most x+9).
	 */
	for (; count - x >= 10; x += 8) {
#define LD565(row, off) xbrz_16_565_load_sse((row) + x + (off))
		__m128i B1 = LD565(src0, 0), C1 = LD565(src0, 1);
		__m128i PA = LD565(src1, -1), PB = LD565(src1, 0), PC = LD565(src1, 1), C4 = LD565(src1, 2);
		__m128i PD = LD565(src2, -1), PE = LD565(src2, 0), PF = LD565(src2, 1), F4 = LD565(src2, 2);
		__m128i PG = LD565(src3, -1), PH = LD565(src3, 0), PI = LD565(src3, 1), I4 = LD565(src3, 2);
		__m128i H5 = LD565(src4, 0), I5 = LD565(src4, 1);
#undef LD565
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBRZ2X_SSE(16_565, 16, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3);

		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), _mm_unpacklo_epi16(E0, E1));
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 8), _mm_unpackhi_epi16(E0, E1));
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), _mm_unpacklo_epi16(E2, E3));
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 8), _mm_unpackhi_epi16(E2, E3));
	}

	for (; x < count; ++x)
		xbrz2_565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

static inline void xbrz2_555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, const interp_uint16* src3,
	const interp_uint16* src4, unsigned count, unsigned x)
{
	interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint16 E0, E1, E2, E3;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(16_555, 0x7FFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = PE;
	XBRZ2(16_555, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E1, E2, E3);
	XBRZ2(16_555, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E3, E1);
	XBRZ2(16_555, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E2, E1, E0);
	XBRZ2(16_555, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void xbrz2x_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	__m128i ptl, ptld, pbl, pbld;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz2_555_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* the SIMD loop carries the right vertices of the previous pixel */
	if (count - x >= 10)
		XBRZ_START_SSE(16_555, 16, 0x7FFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

	/* x>=1, and count-x>=10 ensures all loads, including the +2 offset for
	 * the final lane, remain in the source row (at most x+9).
	 */
	for (; count - x >= 10; x += 8) {
#define LD555(row, off) xbrz_16_555_load_sse((row) + x + (off))
		__m128i B1 = LD555(src0, 0), C1 = LD555(src0, 1);
		__m128i PA = LD555(src1, -1), PB = LD555(src1, 0), PC = LD555(src1, 1), C4 = LD555(src1, 2);
		__m128i PD = LD555(src2, -1), PE = LD555(src2, 0), PF = LD555(src2, 1), F4 = LD555(src2, 2);
		__m128i PG = LD555(src3, -1), PH = LD555(src3, 0), PI = LD555(src3, 1), I4 = LD555(src3, 2);
		__m128i H5 = LD555(src4, 0), I5 = LD555(src4, 1);
#undef LD555
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBRZ2X_SSE(16_555, 16, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3);

		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), _mm_unpacklo_epi16(E0, E1));
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 8), _mm_unpackhi_epi16(E0, E1));
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), _mm_unpacklo_epi16(E2, E3));
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 8), _mm_unpackhi_epi16(E2, E3));
	}

	for (; x < count; ++x)
		xbrz2_555_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

static inline void xbrz2_32_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	const interp_uint32* src0, const interp_uint32* src1,
	const interp_uint32* src2, const interp_uint32* src3,
	const interp_uint32* src4, unsigned count, unsigned x)
{
	interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint32 E0, E1, E2, E3;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(32, 0xFFFFFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = PE;
	XBRZ2(32, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E1, E2, E3);
	XBRZ2(32, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E3, E1);
	XBRZ2(32, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E2, E1, E0);
	XBRZ2(32, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

__attribute__((target("sse4.1"))) void xbrz2x_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, const interp_uint32* restrict src3,
	const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;
	__m128i ptl, ptld, pbl, pbld;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz2_32_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* the SIMD loop carries the right vertices of the previous pixel */
	if (count - x >= 6)
		XBRZ_START_SSE(32, 32, 0xFFFFFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

	/* Interior: four independent input pixels per 128-bit SIMD vector.
	 * count - x >= 6 guarantees valid offsets [-1, +5] in all five rows.
	 */
	for (; count - x >= 6; x += 4) {
#define LD(row, off) xbrz_32_load_sse((row) + x + (off))
		__m128i B1 = LD(src0, 0), C1 = LD(src0, 1);
		__m128i PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
		__m128i PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
		__m128i PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
		__m128i H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
		__m128i E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBRZ2X_SSE(32, 32, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3);

		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x), _mm_unpacklo_epi32(E0, E1));
		_mm_storeu_si128((__m128i*)(void*)(dst0 + 2 * x + 4), _mm_unpackhi_epi32(E0, E1));
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x), _mm_unpacklo_epi32(E2, E3));
		_mm_storeu_si128((__m128i*)(void*)(dst1 + 2 * x + 4), _mm_unpackhi_epi32(E2, E3));
	}

	/* remaining pixels with a clamped right neighborhood */
	for (; x < count; ++x)
		xbrz2_32_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

#endif

