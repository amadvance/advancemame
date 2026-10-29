/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2012 Andrea Mazzoleni
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

#ifndef __XBR2X_H
#define __XBR2X_H

#include "interp.h"

/*
 * This effect is a derivation of the XBR v3.5 effect made by Hillian for libretro
 *
 * https://github.com/libretro/common-shaders/blob/master/xbr/shaders/legacy/2xbr-v3.5a.cg
 *
 * It uses the "rounded" variant and it's a bit faster than the other variants.
 * These variants differs only by a single line.
 *
 * Flavor "square"
 *   if ((e<i)  && ( !eq(PF,PB) && !eq(PF,PC) || !eq(PH,PD) && !eq(PH,PG) || eq(PE,PI) && (!eq(PF,F4) && !eq(PF,I4) || !eq(PH,H5) && !eq(PH,I5)) || eq(PE,PG) || eq(PE,PC)) )
 * Flavor "semirounded"
 *   if ((e<i)  && ( !eq(PF,PB) && !eq(PH,PD) || eq(PE,PI) && (!eq(PF,I4) && !eq(PH,I5)) || eq(PE,PG) || eq(PE,PC)) )
 * Flavor "rounded"
 *   if (e<i)
 *
 * We also use a color distance based exclusively on the pixel luminance, mostly ignoring any chroma information.
 */

/*
           A1 B1 C1
        A0 PA PB PC C4
        D0 PD PE PF F4
        G0 PG PH PI I4
           G5 H5 I5

        N0 N1
        N2 N3
 */

/* Level 2 interpolation cases are expanded directly in this scalar rotation.
 * Output arguments are modifiable pixel expressions.
 */
#define XBR2(fmt, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) \
	do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned _e = interp_ ## fmt ## _dist3((PC), (PE), (PG)) + interp_ ## fmt ## _dist3((H5), (PI), (F4)) + 4 * interp_ ## fmt ## _dist((PH), (PF)); \
			unsigned _i = interp_ ## fmt ## _dist3((PD), (PH), (I5)) + interp_ ## fmt ## _dist3((I4), (PF), (PB)) + 4 * interp_ ## fmt ## _dist((PE), (PI)); \
			if (_e < _i) { \
				int _ex2 = (PE) != (PC) && (PB) != (PC); \
				int _ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned _ke = interp_ ## fmt ## _dist((PF), (PG)); \
				unsigned _ki = interp_ ## fmt ## _dist((PH), (PC)); \
				interp_uint ## fmt _px = interp_ ## fmt ## _dist((PE), (PF)) <= interp_ ## fmt ## _dist((PE), (PH)) ? (PF) : (PH); \
				if (_ke == 0 && _ki == 0 && _ex3 && _ex2) { \
					(N3) = interp_ ## fmt ## _71(_px, (N3)); \
					(N1) = (N2) = interp_ ## fmt ## _31((N2), _px); \
				} else if (2 * _ke <= _ki && _ex3) { \
					(N3) = interp_ ## fmt ## _31(_px, (N3)); \
					(N2) = interp_ ## fmt ## _31((N2), _px); \
				} else if (_ke >= 2 * _ki && _ex2) { \
					(N3) = interp_ ## fmt ## _31(_px, (N3)); \
					(N1) = interp_ ## fmt ## _31((N1), _px); \
				} else { \
					(N3) = interp_ ## fmt ## _11((N3), _px); \
				} \
			} \
		} \
	} while (0)

void xbr2x_16_565_def(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbr2x_16_555_def(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbr2x_32_def(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbr2x_yuy2_def(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbr2x_16_565_sse41(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbr2x_16_555_sse41(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbr2x_32_sse41(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);
void xbr2x_16_565_neon(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbr2x_16_555_neon(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, const interp_uint16* src3, const interp_uint16* src4, unsigned count);
void xbr2x_32_neon(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, const interp_uint32* src3, const interp_uint32* src4, unsigned count);

#endif

