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

#include "xbrz3x.h"

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

#include "xbrz_neon.h"

/**
 * SIMD version of XBRZ3. Each condition is a lane mask and each
 * conditional assignment a blend. The only branch is the early exit
 * when no lane has BR != XBRZ_NONE.
 * When both SHALLOW and STEEP are set, the later blend wins, matching
 * the "shallow && steep" case of the C implementation.
 */
#define XBRZ_NEON(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, N2, N5, N6, N7, N8) \
	do { \
		if (xbrz_ ## w ## _any_neon((BR))) { \
			xbrz_ ## w ## _neon_t _line, _px, _shallow, _steep, _n7, _n5, _n8; \
			XBRZ_LINE_NEON(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, _line, _px, _shallow, _steep); \
			_n7 = xbrz_ ## w ## _sel_neon(_line, interp_ ## fmt ## _71_neon((N7), _px), (N7)); \
			_n7 = xbrz_ ## w ## _sel_neon(_steep, interp_ ## fmt ## _31_neon((N7), _px), _n7); \
			_n7 = xbrz_ ## w ## _sel_neon(_shallow, interp_ ## fmt ## _31_neon(_px, (N7)), _n7); \
			_n5 = xbrz_ ## w ## _sel_neon(_line, interp_ ## fmt ## _71_neon((N5), _px), (N5)); \
			_n5 = xbrz_ ## w ## _sel_neon(_shallow, interp_ ## fmt ## _31_neon((N5), _px), _n5); \
			_n5 = xbrz_ ## w ## _sel_neon(_steep, interp_ ## fmt ## _31_neon(_px, (N5)), _n5); \
			_n8 = xbrz_ ## w ## _sel_neon(_line, interp_ ## fmt ## _71_neon(_px, (N8)), interp_ ## fmt ## _11_neon((N8), _px)); /* approximation of 45/100 */ \
			_n8 = xbrz_ ## w ## _sel_neon(xbrz_ ## w ## _or_neon(_shallow, _steep), _px, _n8); \
			(N8) = xbrz_ ## w ## _sel_neon((BR), _n8, (N8)); \
			(N7) = _n7; \
			(N5) = _n5; \
			(N6) = xbrz_ ## w ## _sel_neon(_shallow, interp_ ## fmt ## _31_neon((N6), _px), (N6)); \
			(N2) = xbrz_ ## w ## _sel_neon(_steep, interp_ ## fmt ## _31_neon((N2), _px), (N2)); \
		} \
	} while (0)

/**
 * Classify and blend one block. The C order of the rotations BR, TR, TL, BL
 * is kept, since rotations can modify the same output pixel.
 * The only branches are the early exits when all the flags are XBRZ_NONE.
 */
#define XBRZ3X_NEON(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, PTL, PTLD, PBL, PBLD, E0, E1, E2, E3, E4, E5, E6, E7, E8) \
	do { \
		xbrz_ ## w ## _neon_t _eq_ef = xbrz_ ## w ## _eq_neon((PE), (PF)); \
		if (xbrz_ ## w ## _flat_neon(_eq_ef, (PTL), (PBL))) { \
			/* all the flags are XBRZ_NONE, and so the next carried ones */ \
			(PTL) = (PTLD) = (PBL) = (PBLD) = xbrz_ ## w ## _set_neon(0); \
		} else { \
			xbrz_ ## w ## _neon_t dEA, dEC, dEG, dEI, tl, tld, tr, trd, bl, bld, br, brd, _any; \
			XBRZ_CLASSIFY_NEON(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, _eq_ef, dEA, dEC, dEG, dEI, tl, tld, tr, trd, bl, bld, br, brd, PTL, PTLD, PBL, PBLD); \
			_any = xbrz_ ## w ## _or_neon(xbrz_ ## w ## _or_neon(tl, tr), xbrz_ ## w ## _or_neon(bl, br)); \
			if (xbrz_ ## w ## _any_neon(_any)) { \
				xbrz_ ## w ## _neon_t dEB, dED, dEF, dEH, dAB, dBC, dCF, dFI, dHI, dGH, dDG, dAD; \
				XBRZ_DIST_NEON(fmt, PA, PB, PC, PD, PE, PF, PG, PH, PI, dEB, dED, dEF, dEH, dAB, dBC, dCF, dFI, dHI, dGH, dDG, dAD); \
				XBRZ_NEON(fmt, w, PB, PC, PD, PE, PF, PG, PH, PI, dEF, dEH, dEG, dEC, dEI, dGH, dHI, dFI, dCF, tr, bl, br, brd, E2, E5, E6, E7, E8); \
				XBRZ_NEON(fmt, w, PD, PA, PH, PE, PB, PI, PF, PC, dEB, dEF, dEI, dEA, dEC, dFI, dCF, dBC, dAB, tl, br, tr, trd, E0, E1, E8, E5, E2); \
				XBRZ_NEON(fmt, w, PH, PG, PF, PE, PD, PC, PB, PA, dED, dEB, dEC, dEG, dEA, dBC, dAB, dAD, dDG, bl, tr, tl, tld, E6, E3, E2, E1, E0); \
				XBRZ_NEON(fmt, w, PF, PI, PB, PE, PH, PA, PD, PG, dEH, dED, dEA, dEI, dEG, dAD, dDG, dGH, dHI, br, tl, bl, bld, E8, E7, E0, E3, E6); \
			} \
		} \
	} while (0)

static inline void xbrz3_565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4,
	unsigned count, unsigned x)
{
	interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint16 E0, E1, E2, E3, E4, E5, E6, E7, E8;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(16_565, 0xFFFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = E4 = E5 = E6 = E7 = E8 = PE;
	XBRZ3(16_565, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E2, E5, E6, E7, E8);
	XBRZ3(16_565, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E1, E8, E5, E2);
	XBRZ3(16_565, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E6, E3, E2, E1, E0);
	XBRZ3(16_565, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E8, E7, E0, E3, E6);
	dst0[3 * x + 0] = E0;
	dst0[3 * x + 1] = E1;
	dst0[3 * x + 2] = E2;
	dst1[3 * x + 0] = E3;
	dst1[3 * x + 1] = E4;
	dst1[3 * x + 2] = E5;
	dst2[3 * x + 0] = E6;
	dst2[3 * x + 1] = E7;
	dst2[3 * x + 2] = E8;
}

void xbrz3x_16_565_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz3_565_edge_pixel(dst0, dst1, dst2, src0, src1, src2, src3, src4, count, x);

	/* 8 source pixels per block using 128-bit vectors, with offsets [-1, +9] */
	if (count - x >= 10) {
		xbrz_16_neon_t ptl, ptld, pbl, pbld;

		/* the SIMD loop carries the right vertices of the previous pixel */
		XBRZ_START_NEON(16_565, 16, 0xFFFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

		for (; count - x >= 10; x += 8) {
#define LD(row, off) xbrz_16_565_load_neon((row) + x + (off))
			xbrz_16_neon_t B1 = LD(src0, 0), C1 = LD(src0, 1);
			xbrz_16_neon_t PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
			xbrz_16_neon_t PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
			xbrz_16_neon_t PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
			xbrz_16_neon_t H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
			xbrz_16_neon_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE;
			xbrz_16_neon_t E8 = PE;
			XBRZ3X_NEON(16_565, 16, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3, E4, E5, E6, E7, E8);
			interp_16_store3_neon(dst0 + 3 * x, E0, E1, E2);
			interp_16_store3_neon(dst1 + 3 * x, E3, E4, E5);
			interp_16_store3_neon(dst2 + 3 * x, E6, E7, E8);
		}
	}

	/* remaining pixels with a clamped right neighborhood */
	for (; x < count; ++x)
		xbrz3_565_edge_pixel(dst0, dst1, dst2, src0, src1, src2, src3, src4, count, x);
}

static inline void xbrz3_555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4,
	unsigned count, unsigned x)
{
	interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint16 E0, E1, E2, E3, E4, E5, E6, E7, E8;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(16_555, 0x7FFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = E4 = E5 = E6 = E7 = E8 = PE;
	XBRZ3(16_555, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E2, E5, E6, E7, E8);
	XBRZ3(16_555, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E1, E8, E5, E2);
	XBRZ3(16_555, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E6, E3, E2, E1, E0);
	XBRZ3(16_555, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E8, E7, E0, E3, E6);
	dst0[3 * x + 0] = E0;
	dst0[3 * x + 1] = E1;
	dst0[3 * x + 2] = E2;
	dst1[3 * x + 0] = E3;
	dst1[3 * x + 1] = E4;
	dst1[3 * x + 2] = E5;
	dst2[3 * x + 0] = E6;
	dst2[3 * x + 1] = E7;
	dst2[3 * x + 2] = E8;
}

void xbrz3x_16_555_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz3_555_edge_pixel(dst0, dst1, dst2, src0, src1, src2, src3, src4, count, x);

	/* 8 source pixels per block using 128-bit vectors, with offsets [-1, +9] */
	if (count - x >= 10) {
		xbrz_16_neon_t ptl, ptld, pbl, pbld;

		/* the SIMD loop carries the right vertices of the previous pixel */
		XBRZ_START_NEON(16_555, 16, 0x7FFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

		for (; count - x >= 10; x += 8) {
#define LD(row, off) xbrz_16_555_load_neon((row) + x + (off))
			xbrz_16_neon_t B1 = LD(src0, 0), C1 = LD(src0, 1);
			xbrz_16_neon_t PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
			xbrz_16_neon_t PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
			xbrz_16_neon_t PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
			xbrz_16_neon_t H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
			xbrz_16_neon_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE;
			xbrz_16_neon_t E8 = PE;
			XBRZ3X_NEON(16_555, 16, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3, E4, E5, E6, E7, E8);
			interp_16_store3_neon(dst0 + 3 * x, E0, E1, E2);
			interp_16_store3_neon(dst1 + 3 * x, E3, E4, E5);
			interp_16_store3_neon(dst2 + 3 * x, E6, E7, E8);
		}
	}

	/* remaining pixels with a clamped right neighborhood */
	for (; x < count; ++x)
		xbrz3_555_edge_pixel(dst0, dst1, dst2, src0, src1, src2, src3, src4, count, x);
}

static inline void xbrz3_32_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2,
	const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4,
	unsigned count, unsigned x)
{
	interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, PI;
	interp_uint32 E0, E1, E2, E3, E4, E5, E6, E7, E8;
	unsigned tl, tr, bl, br;

	XBRZ_EDGE_CLASSIFY(32, 0xFFFFFFU, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, tl, tr, bl, br);
	E0 = E1 = E2 = E3 = E4 = E5 = E6 = E7 = E8 = PE;
	XBRZ3(32, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E2, E5, E6, E7, E8);
	XBRZ3(32, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E0, E1, E8, E5, E2);
	XBRZ3(32, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E6, E3, E2, E1, E0);
	XBRZ3(32, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E8, E7, E0, E3, E6);
	dst0[3 * x + 0] = E0;
	dst0[3 * x + 1] = E1;
	dst0[3 * x + 2] = E2;
	dst1[3 * x + 0] = E3;
	dst1[3 * x + 1] = E4;
	dst1[3 * x + 2] = E5;
	dst2[3 * x + 0] = E6;
	dst2[3 * x + 1] = E7;
	dst2[3 * x + 2] = E8;
}

void xbrz3x_32_neon(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;

	/* the leftmost pixel has a clamped left neighborhood */
	for (; x < count && x < 1; ++x)
		xbrz3_32_edge_pixel(dst0, dst1, dst2, src0, src1, src2, src3, src4, count, x);

	/* 4 source pixels per block using 128-bit vectors, with offsets [-1, +5] */
	if (count - x >= 6) {
		xbrz_32_neon_t ptl, ptld, pbl, pbld;

		/* the SIMD loop carries the right vertices of the previous pixel */
		XBRZ_START_NEON(32, 32, 0xFFFFFFU, src0, src1, src2, src3, src4, count, x, ptl, ptld, pbl, pbld);

		for (; count - x >= 6; x += 4) {
#define LD(row, off) xbrz_32_load_neon((row) + x + (off))
			xbrz_32_neon_t B1 = LD(src0, 0), C1 = LD(src0, 1);
			xbrz_32_neon_t PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
			xbrz_32_neon_t PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
			xbrz_32_neon_t PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
			xbrz_32_neon_t H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
			xbrz_32_neon_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE;
			xbrz_32_neon_t E8 = PE;
			XBRZ3X_NEON(32, 32, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, ptl, ptld, pbl, pbld, E0, E1, E2, E3, E4, E5, E6, E7, E8);
			interp_32_store3_neon(dst0 + 3 * x, E0, E1, E2);
			interp_32_store3_neon(dst1 + 3 * x, E3, E4, E5);
			interp_32_store3_neon(dst2 + 3 * x, E6, E7, E8);
		}
	}

	/* remaining pixels with a clamped right neighborhood */
	for (; x < count; ++x)
		xbrz3_32_edge_pixel(dst0, dst1, dst2, src0, src1, src2, src3, src4, count, x);
}

#endif

