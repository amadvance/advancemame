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

#ifndef __XBRZ2X_H
#define __XBRZ2X_H

#include "interp.h"

/* color equivalence thresholds in the native Advance distance units */
#define XBRZ_EQUAL_16_565 37
#define XBRZ_EQUAL_16_555 37
#define XBRZ_EQUAL_32 300
#define XBRZ_EQUAL_yuy2 30

enum xbrz_blend {
	XBRZ_NONE,
	XBRZ_NORMAL,
	XBRZ_DOMINANT
};

/* corner order: top-left, top-right, bottom-left, bottom-right. */
struct xbrz_corner {
	unsigned tl, tr, bl, br;
};

/*
 * This effect is a derivation of the xBRZ 1.9 effect made by Zenju
 *
 * https://sourceforge.net/projects/xbrz/
 *
 * We use a color distance based exclusively on the pixel luminance, mostly ignoring any chroma information.
 *
 * To use the fast interp_*_11, 31, 53 and 71 functions, the original
 * blending weights of the front pixel are approximated to multiples of 1/8:
 *
 *   original  used   function        error   scale
 *   1/4       1/4    31(N, px)       0       2x 3x 4x
 *   3/4       3/4    31(px, N)       0       2x 3x 4x
 *   1/2       1/2    11(N, px)       0       3x 4x
 *   1/8       1/8    71(N, px)       0       3x
 *   7/8       7/8    71(px, N)       0       3x
 *   1/3       3/8    53(N, px)       +0.042  4x
 *   5/6       7/8    71(px, N)       +0.042  2x
 *   21/100    1/4    31(N, px)       +0.040  2x
 *   45/100    1/2    11(N, px)       +0.050  3x
 *   68/100    5/8    53(px, N)       -0.055  4x
 *   9/100     1/8    71(N, px)       +0.035  4x
 */

/*
 *          . B C .
 *          E F G H
 *          I J K L
 *          . N O .
 *
 * Classify the four pixels F, G, J, K meeting at a shared vertex.
 * The direction bias is 4 and the dominant threshold is 18/5 (3.6).
 * Use unsigned scores: 18 * 8 * 2550 fits in 32 bits, not in 16.
 * R is an output struct xbrz_corner; all pixel arguments are read-only.
 */
#define XBRZ_PREPROCESS(fmt, B, C, E, F, G, H, I, J, K, L, N, O, R) \
	do { \
		(R).tl = (R).tr = (R).bl = (R).br = XBRZ_NONE; \
		if (!(((F) == (G) && (J) == (K)) || ((F) == (J) && (G) == (K)))) { \
			unsigned _d0 = interp_ ## fmt ## _dist3((I), (F), (C)) + interp_ ## fmt ## _dist3((N), (K), (H)) + 4 * interp_ ## fmt ## _dist((J), (G)); \
			unsigned _d1 = interp_ ## fmt ## _dist3((E), (J), (O)) + interp_ ## fmt ## _dist3((B), (G), (L)) + 4 * interp_ ## fmt ## _dist((F), (K)); \
			if (_d0 < _d1) { \
				unsigned _blend = 18 * _d0 < 5 * _d1 ? XBRZ_DOMINANT : XBRZ_NORMAL; \
				if ((F) != (G) && (F) != (J)) \
					(R).tl = _blend; \
				if ((K) != (G) && (K) != (J)) \
					(R).br = _blend; \
			} else if (_d1 < _d0) { \
				unsigned _blend = 18 * _d1 < 5 * _d0 ? XBRZ_DOMINANT : XBRZ_NORMAL; \
				if ((G) != (F) && (G) != (K)) \
					(R).tr = _blend; \
				if ((J) != (F) && (J) != (K)) \
					(R).bl = _blend; \
			} \
		} \
	} while (0)

/*
 *       A B C       N0 N1
 *       D E F  -->  N2 N3
 *       G H I
 *
 * Blend the bottom-right corner of a rotated kernel.
 * The adjacent corner flags protect small features and L shapes.
 * The steep direction threshold is 12/5 (2.4).
 * N1, N2 and N3 are modifiable pixel expressions.
 */
#define XBRZ2(fmt, B, C, D, E, F, G, H, I, TR, BL, BR, N1, N2, N3) \
	do { \
		if ((BR) != XBRZ_NONE) { \
			int _line = (BR) == XBRZ_DOMINANT; \
			interp_uint ## fmt _px; \
			if (!_line) { \
				_line = !((TR) != XBRZ_NONE && interp_ ## fmt ## _dist((E), (G)) >= XBRZ_EQUAL_ ## fmt) \
				        && !((BL) != XBRZ_NONE && interp_ ## fmt ## _dist((E), (C)) >= XBRZ_EQUAL_ ## fmt) \
				        && !(interp_ ## fmt ## _dist((E), (I)) >= XBRZ_EQUAL_ ## fmt \
				             && interp_ ## fmt ## _dist((G), (H)) < XBRZ_EQUAL_ ## fmt \
				             && interp_ ## fmt ## _dist((H), (I)) < XBRZ_EQUAL_ ## fmt \
				             && interp_ ## fmt ## _dist((I), (F)) < XBRZ_EQUAL_ ## fmt \
				             && interp_ ## fmt ## _dist((F), (C)) < XBRZ_EQUAL_ ## fmt); \
			} \
			_px = interp_ ## fmt ## _dist((E), (F)) <= interp_ ## fmt ## _dist((E), (H)) ? (F) : (H); \
			if (_line) { \
				unsigned _fg = interp_ ## fmt ## _dist((F), (G)); \
				unsigned _hc = interp_ ## fmt ## _dist((H), (C)); \
				int _shallow = 12 * _fg <= 5 * _hc && (E) != (G) && (D) != (G); \
				int _steep = 12 * _hc <= 5 * _fg && (E) != (C) && (B) != (C); \
				if (_shallow) \
					(N2) = interp_ ## fmt ## _31((N2), _px); \
				if (_steep) \
					(N1) = interp_ ## fmt ## _31((N1), _px); \
				if (_shallow && _steep) \
					(N3) = interp_ ## fmt ## _71(_px, (N3)); /* approximation of 5/6 */ \
				else if (_shallow || _steep) \
					(N3) = interp_ ## fmt ## _31(_px, (N3)); \
				else \
					(N3) = interp_ ## fmt ## _11((N3), _px); \
			} else { \
				(N3) = interp_ ## fmt ## _31((N3), _px); /* approximation of 21/100 */ \
			} \
		} \
	} while (0)

/* clamped columns of the scalar edge pixels */
#define XBRZ_EDGE_COLUMNS(count, x, XM2, XM1, XP1, XP2) \
	do { \
		(XM2) = (x) >= 2 ? (x) - 2 : 0; \
		(XM1) = (x) >= 1 ? (x) - 1 : 0; \
		(XP1) = (count) - (x) >= 2 ? (x) + 1 : (count) - 1; \
		(XP2) = (count) - (x) >= 3 ? (x) + 2 : (count) - 1; \
	} while (0)

/**
 * Classify the two left vertices of the pixel x from scratch, as the
 * right vertices of the pixel x - 1, without any state of the previous pixel.
 * With clamped columns this gives the same result of the C implementation,
 * also at x = 0, where the C implementation sets XBRZ_NONE: the two equal
 * columns -1 and 0 fail all the flags.
 * MASK clears the unused bits as the C implementation does.
 */
#define XBRZ_EDGE_LEFT(fmt, MASK, src0, src1, src2, src3, src4, XM2, XM1, X, XP1, TL, BL) \
	do { \
		interp_uint ## fmt _la1 = (src0)[XM1] & (MASK), _lb1 = (src0)[X] & (MASK); \
		interp_uint ## fmt _la0 = (src1)[XM2] & (MASK), _lpa = (src1)[XM1] & (MASK), _lpb = (src1)[X] & (MASK), _lpc = (src1)[XP1] & (MASK); \
		interp_uint ## fmt _ld0 = (src2)[XM2] & (MASK), _lpd = (src2)[XM1] & (MASK), _lpe = (src2)[X] & (MASK), _lpf = (src2)[XP1] & (MASK); \
		interp_uint ## fmt _lg0 = (src3)[XM2] & (MASK), _lpg = (src3)[XM1] & (MASK), _lph = (src3)[X] & (MASK), _lpi = (src3)[XP1] & (MASK); \
		interp_uint ## fmt _lg5 = (src4)[XM1] & (MASK), _lh5 = (src4)[X] & (MASK); \
		struct xbrz_corner _llt, _llb; \
		XBRZ_PREPROCESS(fmt, _la1, _lb1, _la0, _lpa, _lpb, _lpc, _ld0, _lpd, _lpe, _lpf, _lpg, _lph, _llt); \
		XBRZ_PREPROCESS(fmt, _lpa, _lpb, _ld0, _lpd, _lpe, _lpf, _lg0, _lpg, _lph, _lpi, _lg5, _lh5, _llb); \
		(TL) = _llt.br; \
		(BL) = _llb.tr; \
	} while (0)

/**
 * Load the clamped neighborhood of the scalar edge pixel x and classify
 * its four vertices from scratch.
 */
#define XBRZ_EDGE_CLASSIFY(fmt, MASK, src0, src1, src2, src3, src4, count, x, PA, PB, PC, PD, PE, PF, PG, PH, PI, TL, TR, BL, BR) \
	do { \
		unsigned _xm2, _xm1, _xp1, _xp2; \
		struct xbrz_corner _top, _bot; \
		interp_uint ## fmt _b1, _c1, _c4, _f4, _i4, _h5, _i5; \
		XBRZ_EDGE_COLUMNS(count, x, _xm2, _xm1, _xp1, _xp2); \
		_b1 = (src0)[x] & (MASK); \
		_c1 = (src0)[_xp1] & (MASK); \
		(PA) = (src1)[_xm1] & (MASK); \
		(PB) = (src1)[x] & (MASK); \
		(PC) = (src1)[_xp1] & (MASK); \
		_c4 = (src1)[_xp2] & (MASK); \
		(PD) = (src2)[_xm1] & (MASK); \
		(PE) = (src2)[x] & (MASK); \
		(PF) = (src2)[_xp1] & (MASK); \
		_f4 = (src2)[_xp2] & (MASK); \
		(PG) = (src3)[_xm1] & (MASK); \
		(PH) = (src3)[x] & (MASK); \
		(PI) = (src3)[_xp1] & (MASK); \
		_i4 = (src3)[_xp2] & (MASK); \
		_h5 = (src4)[x] & (MASK); \
		_i5 = (src4)[_xp1] & (MASK); \
		XBRZ_EDGE_LEFT(fmt, MASK, src0, src1, src2, src3, src4, _xm2, _xm1, x, _xp1, TL, BL); \
		XBRZ_PREPROCESS(fmt, _b1, _c1, (PA), (PB), (PC), _c4, (PD), (PE), (PF), _f4, (PH), (PI), _top); \
		XBRZ_PREPROCESS(fmt, (PB), (PC), (PD), (PE), (PF), _f4, (PG), (PH), (PI), _i4, _h5, _i5, _bot); \
		(TR) = _top.bl; \
		(BR) = _bot.tl; \
	} while (0)

void xbrz2x_16_565_def(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz2x_16_555_def(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz2x_32_def(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbrz2x_yuy2_def(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbrz2x_16_565_sse41(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz2x_16_555_sse41(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz2x_32_sse41(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbrz2x_16_565_neon(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz2x_16_555_neon(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz2x_32_neon(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);

#endif
