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

#include "portable.h"

#include "xbr3x.h"

#include <assert.h>

/***************************************************************************/
/* xbr3x C implementation */

/*
 * This effect is a derivation of the XBR effect made by Hillian for the Kagi Fusion plugin.
 * We use a partially semplified algorithm to gain speed, but without loosing too much quality.
 * We also use a color distance based exclusively on the pixel luminance, mostly ignoring any chroma information.
 */

/*
	   A1 B1 C1
	A0 PA PB PC C4
	D0 PD PE PF F4
	G0 PG PH PI I4
	   G5 H5 I5

	N0 N1 N2
	N3 N4 N5
	N6 N7 N8
*/

#define XBR(type, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3, N4, N5, N6, N7, N8) \
	if (PE!=PH && PE!=PF) {\
		unsigned e = df3(PC,PE,PG) + df3(H5,PI,F4) + 4*df(PH,PF); \
		unsigned i = df3(PD,PH,I5) + df3(I4,PF,PB) + 4*df(PE,PI); \
		if (e<i) { \
			int ex2 = PE!=PC && PB!=PC; \
			int ex3 = PE!=PG && PD!=PG; \
			unsigned ke = df(PF,PG); \
			unsigned ki = df(PH,PC); \
			type px = df(PE,PF) <= df(PE,PH) ? PF : PH; \
			if (ke == 0 && ki == 0 && ex3 && ex2) { \
				LEFT_UP_2_3X(N7, N5, N6, N2, N8, px); \
			} else if (2*ke <= ki && ex3) { \
				LEFT_2_3X(N7, N5, N6, N8, px); \
			} else if (ke >= 2*ki && ex2)  { \
				UP_2_3X(N5, N7, N2, N8, px); \
			} else { \
				DIA_3X(N8, N5, N7, px); \
			} \
		} \
	}

#define LEFT_UP_2_3X(N7, N5, N6, N2, N8, PIXEL) \
	E[N5] = E[N7] = interp_16_31(PIXEL, E[N7]); \
	E[N2] = E[N6] = interp_16_31(E[N6], PIXEL); \
	E[N8] = PIXEL;

#define LEFT_2_3X(N7, N5, N6, N8, PIXEL)\
	E[N7] = interp_16_31(PIXEL, E[N7]); \
	E[N5] = interp_16_31(E[N5], PIXEL); \
	E[N6] = interp_16_31(E[N6], PIXEL); \
	E[N8] = PIXEL;

#define UP_2_3X(N5, N7, N2, N8, PIXEL)\
	E[N5] = interp_16_31(PIXEL, E[N5]); \
	E[N7] = interp_16_31(E[N7], PIXEL); \
	E[N2] = interp_16_31(E[N2], PIXEL); \
	E[N8] = PIXEL;

#define DIA_3X(N8, N5, N7, PIXEL)\
	E[N8] = interp_16_71(PIXEL, E[N8]); \
	E[N5] = interp_16_71(E[N5], PIXEL); \
	E[N7] = interp_16_71(E[N7], PIXEL);

#define df(A, B) interp_16_dist(A, B)
#define df3(A, B, C) interp_16_dist3(A, B, C)

void xbr3x_16_def(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2, const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, xPI;
		interp_uint16 A0, D0, G0, A1, B1, C1, C4, F4, I4, G5, H5, I5;
		interp_uint16 E[9];

		/* first two columns */
		if (i>1) {
			A1 = src0[-1];
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
			G5 = src4[-1];
		
			A0 = src1[-2];
			D0 = src2[-2];
			G0 = src3[-2];
		} else if (i>0) {
			A1 = src0[-1];
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
			G5 = src4[-1];
			
			A0 = src1[-1];
			D0 = src2[-1];
			G0 = src3[-1];
		} else {
			A1 = src0[0];
			PA = src1[0];
			PD = src2[0];
			PG = src3[0];
			G5 = src4[0];
			
			A0 = src1[0];
			D0 = src2[0];
			G0 = src3[0];
		}

		/* central */
		B1 = src0[0];
		PB = src1[0];
		PE = src2[0];
		PH = src3[0];
		H5 = src4[0];

		/* last two columns */
		if (i+2<count) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			xPI = src3[1];
			I5 = src4[1];

			C4 = src1[2];
			F4 = src2[2];
			I4 = src3[2];
		} else if (i+1<count) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			xPI = src3[1];
			I5 = src4[1];

			C4 = src1[1];
			F4 = src2[1];
			I4 = src3[1];
		} else {
			C1 = src0[0];
			PC = src1[0];
			PF = src2[0];
			xPI = src3[0];
			I5 = src4[0];

			C4 = src1[0];
			F4 = src2[0];
			I4 = src3[0];
		}

		/* default pixels */
		E[0] = PE;
		E[1] = PE;
		E[2] = PE;
		E[3] = PE;
		E[4] = PE;
		E[5] = PE;
		E[6] = PE;
		E[7] = PE;
		E[8] = PE;

		XBR(interp_uint16, PE, xPI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, 0, 1, 2, 3, 4, 5, 6, 7, 8);
		XBR(interp_uint16, PE, PC, PF, PB, xPI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, 6, 3, 0, 7, 4, 1, 8, 5, 2);
		XBR(interp_uint16, PE, PA, PB, PD, PC, PG, PF, PH, xPI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		XBR(interp_uint16, PE, PG, PD, PH, PA, xPI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, 2, 5, 8, 1, 4, 7, 0, 3, 6);

		/* copy resulting pixel into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst1[0] = E[3];
		dst1[1] = E[4];
		dst1[2] = E[5];
		dst2[0] = E[6];
		dst2[1] = E[7];
		dst2[2] = E[8];

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
	}
}

#undef LEFT_UP_2_3X
#undef LEFT_2_3X
#undef UP_2_3X
#undef DIA_3X
#undef df
#undef df3

#define LEFT_UP_2_3X(N7, N5, N6, N2, N8, PIXEL) \
	E[N5] = E[N7] = interp_32_31(PIXEL, E[N7]); \
	E[N2] = E[N6] = interp_32_31(E[N6], PIXEL); \
	E[N8] = PIXEL;

#define LEFT_2_3X(N7, N5, N6, N8, PIXEL)\
	E[N7] = interp_32_31(PIXEL, E[N7]); \
	E[N5] = interp_32_31(E[N5], PIXEL); \
	E[N6] = interp_32_31(E[N6], PIXEL); \
	E[N8] = PIXEL;

#define UP_2_3X(N5, N7, N2, N8, PIXEL)\
	E[N5] = interp_32_31(PIXEL, E[N5]); \
	E[N7] = interp_32_31(E[N7], PIXEL); \
	E[N2] = interp_32_31(E[N2], PIXEL); \
	E[N8] = PIXEL;

#define DIA_3X(N8, N5, N7, PIXEL)\
	E[N8] = interp_32_71(PIXEL, E[N8]); \
	E[N5] = interp_32_71(E[N5], PIXEL); \
	E[N7] = interp_32_71(E[N7], PIXEL);

#define df(A, B) interp_32_dist(A, B)
#define df3(A, B, C) interp_32_dist3(A, B, C)

void xbr3x_32_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, xPI;
		interp_uint32 A0, D0, G0, A1, B1, C1, C4, F4, I4, G5, H5, I5;
		interp_uint32 E[9];

		/* first two columns */
		if (i>1) {
			A1 = src0[-1];
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
			G5 = src4[-1];
		
			A0 = src1[-2];
			D0 = src2[-2];
			G0 = src3[-2];
		} else if (i>0) {
			A1 = src0[-1];
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
			G5 = src4[-1];
			
			A0 = src1[-1];
			D0 = src2[-1];
			G0 = src3[-1];
		} else {
			A1 = src0[0];
			PA = src1[0];
			PD = src2[0];
			PG = src3[0];
			G5 = src4[0];
			
			A0 = src1[0];
			D0 = src2[0];
			G0 = src3[0];
		}

		/* central */
		B1 = src0[0];
		PB = src1[0];
		PE = src2[0];
		PH = src3[0];
		H5 = src4[0];

		/* last two columns */
		if (i+2<count) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			xPI = src3[1];
			I5 = src4[1];

			C4 = src1[2];
			F4 = src2[2];
			I4 = src3[2];
		} else if (i+1<count) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			xPI = src3[1];
			I5 = src4[1];

			C4 = src1[1];
			F4 = src2[1];
			I4 = src3[1];
		} else {
			C1 = src0[0];
			PC = src1[0];
			PF = src2[0];
			xPI = src3[0];
			I5 = src4[0];

			C4 = src1[0];
			F4 = src2[0];
			I4 = src3[0];
		}

		/* default pixels */
		E[0] = PE;
		E[1] = PE;
		E[2] = PE;
		E[3] = PE;
		E[4] = PE;
		E[5] = PE;
		E[6] = PE;
		E[7] = PE;
		E[8] = PE;

		XBR(interp_uint32, PE, xPI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, 0, 1, 2, 3, 4, 5, 6, 7, 8);
		XBR(interp_uint32, PE, PC, PF, PB, xPI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, 6, 3, 0, 7, 4, 1, 8, 5, 2);
		XBR(interp_uint32, PE, PA, PB, PD, PC, PG, PF, PH, xPI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		XBR(interp_uint32, PE, PG, PD, PH, PA, xPI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, 2, 5, 8, 1, 4, 7, 0, 3, 6);

		/* copy resulting pixel into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst1[0] = E[3];
		dst1[1] = E[4];
		dst1[2] = E[5];
		dst2[0] = E[6];
		dst2[1] = E[7];
		dst2[2] = E[8];

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
	}
}

#undef LEFT_UP_2_3X
#undef LEFT_2_3X
#undef UP_2_3X
#undef DIA_3X
#undef df
#undef df3

#define LEFT_UP_2_3X(N7, N5, N6, N2, N8, PIXEL) \
	E[N5] = E[N7] = interp_yuy2_31(PIXEL, E[N7]); \
	E[N2] = E[N6] = interp_yuy2_31(E[N6], PIXEL); \
	E[N8] = PIXEL;

#define LEFT_2_3X(N7, N5, N6, N8, PIXEL)\
	E[N7] = interp_yuy2_31(PIXEL, E[N7]); \
	E[N5] = interp_yuy2_31(E[N5], PIXEL); \
	E[N6] = interp_yuy2_31(E[N6], PIXEL); \
	E[N8] = PIXEL;

#define UP_2_3X(N5, N7, N2, N8, PIXEL)\
	E[N5] = interp_yuy2_31(PIXEL, E[N5]); \
	E[N7] = interp_yuy2_31(E[N7], PIXEL); \
	E[N2] = interp_yuy2_31(E[N2], PIXEL); \
	E[N8] = PIXEL;

#define DIA_3X(N8, N5, N7, PIXEL)\
	E[N8] = interp_yuy2_71(PIXEL, E[N8]); \
	E[N5] = interp_yuy2_71(E[N5], PIXEL); \
	E[N7] = interp_yuy2_71(E[N7], PIXEL);

#define df(A, B) interp_yuy2_dist(A, B)
#define df3(A, B, C) interp_yuy2_dist3(A, B, C)

void xbr3x_yuy2_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, xPI;
		interp_uint32 A0, D0, G0, A1, B1, C1, C4, F4, I4, G5, H5, I5;
		interp_uint32 E[9];

		/* first two columns */
		if (i>1) {
			A1 = src0[-1];
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
			G5 = src4[-1];
		
			A0 = src1[-2];
			D0 = src2[-2];
			G0 = src3[-2];
		} else if (i>0) {
			A1 = src0[-1];
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
			G5 = src4[-1];
			
			A0 = src1[-1];
			D0 = src2[-1];
			G0 = src3[-1];
		} else {
			A1 = src0[0];
			PA = src1[0];
			PD = src2[0];
			PG = src3[0];
			G5 = src4[0];
			
			A0 = src1[0];
			D0 = src2[0];
			G0 = src3[0];
		}

		/* central */
		B1 = src0[0];
		PB = src1[0];
		PE = src2[0];
		PH = src3[0];
		H5 = src4[0];

		/* last two columns */
		if (i+2<count) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			xPI = src3[1];
			I5 = src4[1];

			C4 = src1[2];
			F4 = src2[2];
			I4 = src3[2];
		} else if (i+1<count) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			xPI = src3[1];
			I5 = src4[1];

			C4 = src1[1];
			F4 = src2[1];
			I4 = src3[1];
		} else {
			C1 = src0[0];
			PC = src1[0];
			PF = src2[0];
			xPI = src3[0];
			I5 = src4[0];

			C4 = src1[0];
			F4 = src2[0];
			I4 = src3[0];
		}

		/* default pixels */
		E[0] = PE;
		E[1] = PE;
		E[2] = PE;
		E[3] = PE;
		E[4] = PE;
		E[5] = PE;
		E[6] = PE;
		E[7] = PE;
		E[8] = PE;

		XBR(interp_uint32, PE, xPI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, 0, 1, 2, 3, 4, 5, 6, 7, 8);
		XBR(interp_uint32, PE, PC, PF, PB, xPI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, 6, 3, 0, 7, 4, 1, 8, 5, 2);
		XBR(interp_uint32, PE, PA, PB, PD, PC, PG, PF, PH, xPI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		XBR(interp_uint32, PE, PG, PD, PH, PA, xPI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, 2, 5, 8, 1, 4, 7, 0, 3, 6);

		/* copy resulting pixel into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst1[0] = E[3];
		dst1[1] = E[4];
		dst1[2] = E[5];
		dst2[0] = E[6];
		dst2[1] = E[7];
		dst2[2] = E[8];

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
	}
}

