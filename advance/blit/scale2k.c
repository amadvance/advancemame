/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2008 Andrea Mazzoleni
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

#include "scale2k.h"

#include <assert.h>

/***************************************************************************/
/* Scale2k C implementation */

/*
	ABC
	DEF
	GHI

	E0E1
	E2E3
*/

#define SCALE2K_R0(A,B,C,D,E,F,G,H,I,E0,E1,E2,E3) \
	if (B != E && D != E) { \
		if (D == B) { \
			/* diagonal */ \
			if (B == C && D == G) { \
				/* square block */ \
			} else if (B == C) { \
				/* horizontal slope */ \
				E0 = interp_31(D, E0); \
				E1 = interp_13(D, E1); \
			} else if (D == G) { \
				/* vertical slope */ \
				E0 = interp_31(D, E0); \
				E2 = interp_13(D, E2); \
			} else { \
				/* pure diagonal */ \
				E0 = interp_11(E0,D); \
			} \
		} \
	}

#define SCALE2K(A,B,C,D,E,F,G,H,I,E0,E1,E2,E3) \
	if (B != E && D != E) { \
		if (D == B) { \
			/* diagonal */ \
			if (B == C && D == G) { \
				/* square block */ \
				if (A != E) { \
					/* no star */ \
					E0 = interp_31(D, E0); \
					E1 = interp_13(D, E1); \
					E2 = interp_13(D, E2); \
				} \
			} else if (B == C && C != F) { \
				/* horizontal slope */ \
				E0 = interp_31(D, E0); \
				E1 = interp_13(D, E1); \
			} else if (D == G && G != H) { \
				/* vertical slope */ \
				E0 = interp_31(D, E0); \
				E2 = interp_13(D, E2); \
			} else { \
				/* pure diagonal */ \
				E0 = interp_11(E0,D); \
			} \
		} \
	}

#define interp_31(A,B) interp_16_31(A, B)
#define interp_13(A,B) interp_16_31(B, A)
#define interp_11(A,B) interp_16_11(A, B)

void scale2k_16_def(interp_uint16* restrict dst0, interp_uint16* restrict dst1, const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		interp_uint16 c[9];
		interp_uint16 e[4];
		
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		e[0] = c[4];
		e[1] = c[4];
		e[2] = c[4];
		e[3] = c[4];

		SCALE2K(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],e[0],e[1],e[2],e[3]);
		SCALE2K(c[6],c[3],c[0],c[7],c[4],c[1],c[8],c[5],c[2],e[2],e[0],e[3],e[1]);
		SCALE2K(c[8],c[7],c[6],c[5],c[4],c[3],c[2],c[1],c[0],e[3],e[2],e[1],e[0]);
		SCALE2K(c[2],c[5],c[8],c[1],c[4],c[7],c[0],c[3],c[6],e[1],e[3],e[0],e[2]);
		
		dst0[0] = e[0];
		dst0[1] = e[1];
		dst1[0] = e[2];
		dst1[1] = e[3];

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 2;
		dst1 += 2;
	}
}

#undef interp_31
#undef interp_13
#undef interp_11

#define interp_31(A,B) interp_32_31(A, B)
#define interp_13(A,B) interp_32_31(B, A)
#define interp_11(A,B) interp_32_11(A, B)

void scale2k_32_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		interp_uint32 c[9];
		interp_uint32 e[4];
		
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		e[0] = c[4];
		e[1] = c[4];
		e[2] = c[4];
		e[3] = c[4];

		SCALE2K(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],e[0],e[1],e[2],e[3]);
		SCALE2K(c[6],c[3],c[0],c[7],c[4],c[1],c[8],c[5],c[2],e[2],e[0],e[3],e[1]);
		SCALE2K(c[8],c[7],c[6],c[5],c[4],c[3],c[2],c[1],c[0],e[3],e[2],e[1],e[0]);
		SCALE2K(c[2],c[5],c[8],c[1],c[4],c[7],c[0],c[3],c[6],e[1],e[3],e[0],e[2]);
		
		dst0[0] = e[0];
		dst0[1] = e[1];
		dst1[0] = e[2];
		dst1[1] = e[3];

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 2;
		dst1 += 2;
	}
}

#undef interp_31
#undef interp_13
#undef interp_11

#define interp_31(A,B) interp_yuy2_31(A, B)
#define interp_13(A,B) interp_yuy2_31(B, A)
#define interp_11(A,B) interp_yuy2_11(A, B)

void scale2k_yuy2_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, unsigned count)
{
	unsigned i;

	for(i=0;i<count;++i) {
		interp_uint32 c[9];
		interp_uint32 e[4];
		
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		e[0] = c[4];
		e[1] = c[4];
		e[2] = c[4];
		e[3] = c[4];

		SCALE2K(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7],c[8],e[0],e[1],e[2],e[3]);
		SCALE2K(c[6],c[3],c[0],c[7],c[4],c[1],c[8],c[5],c[2],e[2],e[0],e[3],e[1]);
		SCALE2K(c[8],c[7],c[6],c[5],c[4],c[3],c[2],c[1],c[0],e[3],e[2],e[1],e[0]);
		SCALE2K(c[2],c[5],c[8],c[1],c[4],c[7],c[0],c[3],c[6],e[1],e[3],e[0],e[2]);
		
		dst0[0] = e[0];
		dst0[1] = e[1];
		dst1[0] = e[2];
		dst1[1] = e[3];

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 2;
		dst1 += 2;
	}
}

