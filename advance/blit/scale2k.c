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

/***************************************************************************/
/* Scale2k C implementation */

void scale2k_16_565_def(interp_uint16* restrict dst0, interp_uint16* restrict dst1, const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2, unsigned count)
{
	unsigned i;

	for (i = 0; i < count; ++i) {
		interp_uint16 c[9];
		interp_uint16 e[4];

		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i > 0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i < count - 1) {
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

		SCALE2K(16_565, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], e[0], e[1], e[2], e[3]);
		SCALE2K(16_565, c[6], c[3], c[0], c[7], c[4], c[1], c[8], c[5], c[2], e[2], e[0], e[3], e[1]);
		SCALE2K(16_565, c[8], c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0], e[3], e[2], e[1], e[0]);
		SCALE2K(16_565, c[2], c[5], c[8], c[1], c[4], c[7], c[0], c[3], c[6], e[1], e[3], e[0], e[2]);

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

void scale2k_16_555_def(interp_uint16* restrict dst0, interp_uint16* restrict dst1, const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2, unsigned count)
{
	unsigned i;

	for (i = 0; i < count; ++i) {
		interp_uint16 c[9];
		interp_uint16 e[4];

		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i > 0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i < count - 1) {
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

		SCALE2K(16_555, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], e[0], e[1], e[2], e[3]);
		SCALE2K(16_555, c[6], c[3], c[0], c[7], c[4], c[1], c[8], c[5], c[2], e[2], e[0], e[3], e[1]);
		SCALE2K(16_555, c[8], c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0], e[3], e[2], e[1], e[0]);
		SCALE2K(16_555, c[2], c[5], c[8], c[1], c[4], c[7], c[0], c[3], c[6], e[1], e[3], e[0], e[2]);

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

void scale2k_32_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, unsigned count)
{
	unsigned i;

	for (i = 0; i < count; ++i) {
		interp_uint32 c[9];
		interp_uint32 e[4];

		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i > 0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i < count - 1) {
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

		SCALE2K(32, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], e[0], e[1], e[2], e[3]);
		SCALE2K(32, c[6], c[3], c[0], c[7], c[4], c[1], c[8], c[5], c[2], e[2], e[0], e[3], e[1]);
		SCALE2K(32, c[8], c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0], e[3], e[2], e[1], e[0]);
		SCALE2K(32, c[2], c[5], c[8], c[1], c[4], c[7], c[0], c[3], c[6], e[1], e[3], e[0], e[2]);

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

void scale2k_yuy2_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, unsigned count)
{
	unsigned i;

	for (i = 0; i < count; ++i) {
		interp_uint32 c[9];
		interp_uint32 e[4];

		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i > 0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i < count - 1) {
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

		SCALE2K(yuy2, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], e[0], e[1], e[2], e[3]);
		SCALE2K(yuy2, c[6], c[3], c[0], c[7], c[4], c[1], c[8], c[5], c[2], e[2], e[0], e[3], e[1]);
		SCALE2K(yuy2, c[8], c[7], c[6], c[5], c[4], c[3], c[2], c[1], c[0], e[3], e[2], e[1], e[0]);
		SCALE2K(yuy2, c[2], c[5], c[8], c[1], c[4], c[7], c[0], c[3], c[6], e[1], e[3], e[0], e[2]);

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

