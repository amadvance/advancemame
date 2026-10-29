/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2004, 2008 Andrea Mazzoleni
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

#ifndef __SCALE2K_H
#define __SCALE2K_H

#include "interp.h"

/*
 *       A B C
 *       D E F
 *       G H I
 *
 *       E0 E1
 *       E2 E3
 *
 * Apply one of the four rotated Scale2k rules.
 * Output arguments must be modifiable pixel expressions.
 */
#define SCALE2K(fmt, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3) \
	do { \
		if ((D) == (B) && (B) != (E)) { \
			/* diagonal */ \
			if ((B) == (C) && (D) == (G)) { \
				/* square block */ \
				if ((A) != (E)) { \
					/* no star */ \
					(E0) = interp_ ## fmt ## _31((D), (E0)); \
					(E1) = interp_ ## fmt ## _31((E1), (D)); \
					(E2) = interp_ ## fmt ## _31((E2), (D)); \
				} \
			} else if ((B) == (C) && (C) != (F)) { \
				/* horizontal slope */ \
				(E0) = interp_ ## fmt ## _31((D), (E0)); \
				(E1) = interp_ ## fmt ## _31((E1), (D)); \
			} else if ((D) == (G) && (G) != (H)) { \
				/* vertical slope */ \
				(E0) = interp_ ## fmt ## _31((D), (E0)); \
				(E2) = interp_ ## fmt ## _31((E2), (D)); \
			} else { \
				/* pure diagonal */ \
				(E0) = interp_ ## fmt ## _11((E0), (D)); \
			} \
		} \
	} while (0)

void scale2k_16_565_def(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, unsigned count);
void scale2k_16_555_def(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, unsigned count);
void scale2k_32_def(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, unsigned count);
void scale2k_yuy2_def(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, unsigned count);
void scale2k_16_565_sse41(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, unsigned count);
void scale2k_16_555_sse41(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, unsigned count);
void scale2k_32_sse41(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, unsigned count);
void scale2k_16_565_neon(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, unsigned count);
void scale2k_16_555_neon(interp_uint16* dst0, interp_uint16* dst1, const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2, unsigned count);
void scale2k_32_neon(interp_uint32* dst0, interp_uint32* dst1, const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2, unsigned count);

#endif

