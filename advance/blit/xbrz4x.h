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

#ifndef __XBRZ4X_H
#define __XBRZ4X_H

#include "xbrz2x.h"

/*
 * Blend the bottom-right corner of the rotated 3x3 source kernel.
 * N0..N15 refer to the row-major 4x4 output block.
 * Only modified output pixels are passed to the macro.
 * The adjacent corner flags protect small features and L shapes.
 * The steep direction threshold is 12/5 (2.4).
 * Blending coefficients and order follow xBRZ 1.9.
 */
#define XBRZ4(fmt, B, C, D, E, F, G, H, I, TR, BL, BR, N3, N7, N10, N11, N12, N13, N14, N15) \
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
				if (_shallow && _steep) { \
					(N13) = interp_ ## fmt ## _31(_px, (N13)); \
					(N7) = interp_ ## fmt ## _31(_px, (N7)); \
					(N12) = interp_ ## fmt ## _31((N12), _px); \
					(N3) = interp_ ## fmt ## _31((N3), _px); \
					(N10) = interp_ ## fmt ## _53((N10), _px); /* approximation of 1/3 */ \
					(N15) = _px; \
					(N14) = _px; \
					(N11) = _px; \
				} else if (_shallow) { \
					(N12) = interp_ ## fmt ## _31((N12), _px); \
					(N10) = interp_ ## fmt ## _31((N10), _px); \
					(N13) = interp_ ## fmt ## _31(_px, (N13)); \
					(N11) = interp_ ## fmt ## _31(_px, (N11)); \
					(N14) = _px; \
					(N15) = _px; \
				} else if (_steep) { \
					(N3) = interp_ ## fmt ## _31((N3), _px); \
					(N10) = interp_ ## fmt ## _31((N10), _px); \
					(N7) = interp_ ## fmt ## _31(_px, (N7)); \
					(N14) = interp_ ## fmt ## _31(_px, (N14)); \
					(N11) = _px; \
					(N15) = _px; \
				} else { \
					(N14) = interp_ ## fmt ## _11((N14), _px); \
					(N11) = interp_ ## fmt ## _11((N11), _px); \
					(N15) = _px; \
				} \
			} else { \
				(N15) = interp_ ## fmt ## _53(_px, (N15)); /* approximation of 68/100 */ \
				(N14) = interp_ ## fmt ## _71((N14), _px); /* approximation of 9/100 */ \
				(N11) = interp_ ## fmt ## _71((N11), _px); /* approximation of 9/100 */ \
			} \
		} \
	} while (0)

void xbrz4x_16_565_def(interp_uint16* dst0, interp_uint16* dst1, interp_uint16* dst2, interp_uint16* dst3, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz4x_16_555_def(interp_uint16* dst0, interp_uint16* dst1, interp_uint16* dst2, interp_uint16* dst3, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz4x_32_def(interp_uint32* dst0, interp_uint32* dst1, interp_uint32* dst2, interp_uint32* dst3, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbrz4x_yuy2_def(interp_uint32* dst0, interp_uint32* dst1, interp_uint32* dst2, interp_uint32* dst3, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbrz4x_16_565_sse41(interp_uint16* dst0, interp_uint16* dst1, interp_uint16* dst2, interp_uint16* dst3, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz4x_16_555_sse41(interp_uint16* dst0, interp_uint16* dst1, interp_uint16* dst2, interp_uint16* dst3, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz4x_32_sse41(interp_uint32* dst0, interp_uint32* dst1, interp_uint32* dst2, interp_uint32* dst3, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbrz4x_16_565_neon(interp_uint16* dst0, interp_uint16* dst1, interp_uint16* dst2, interp_uint16* dst3, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz4x_16_555_neon(interp_uint16* dst0, interp_uint16* dst1, interp_uint16* dst2, interp_uint16* dst3, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbrz4x_32_neon(interp_uint32* dst0, interp_uint32* dst1, interp_uint32* dst2, interp_uint32* dst3, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);

#endif
