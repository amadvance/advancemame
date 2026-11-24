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

/***************************************************************************/
/* xBRZ4x C implementation */

/*
 * Load the neighboring pixels directly from the five source lines.
 * Reuse only the two right vertex classifications as the left ones of
 * the next pixel. Keep the xBRZ rotation order: BR, TR, TL, BL.
 */

void xbrz4x_16_565_def(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3, const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2, const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned i;
	unsigned tl = XBRZ_NONE, bl = XBRZ_NONE;

	for (i = 0; i < count; ++i) {
		interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
		interp_uint16 B1, C1, C4, F4, I4, H5, I5;
		interp_uint16 E[16];
		struct xbrz_corner top, bot;
		unsigned tr, br;

		/* first column */
		if (i > 0) {
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
		} else {
			PA = src1[0];
			PD = src2[0];
			PG = src3[0];
		}

		/* central */
		B1 = src0[0];
		PB = src1[0];
		PE = src2[0];
		PH = src3[0];
		H5 = src4[0];

		/* last two columns */
		if (count - i > 2) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			PI = src3[1];
			I5 = src4[1];

			C4 = src1[2];
			F4 = src2[2];
			I4 = src3[2];
		} else if (count - i > 1) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			PI = src3[1];
			I5 = src4[1];

			C4 = src1[1];
			F4 = src2[1];
			I4 = src3[1];
		} else {
			C1 = src0[0];
			PC = src1[0];
			PF = src2[0];
			PI = src3[0];
			I5 = src4[0];

			C4 = src1[0];
			F4 = src2[0];
			I4 = src3[0];
		}

		/* classify the upper-right and lower-right vertices */
		XBRZ_PREPROCESS(16_565, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PH, PI, top);
		XBRZ_PREPROCESS(16_565, PB, PC, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, bot);
		tr = top.bl;
		br = bot.tl;

		/* default pixels */
		E[0] = E[1] = E[2] = E[3] = PE;
		E[4] = E[5] = E[6] = E[7] = PE;
		E[8] = E[9] = E[10] = E[11] = PE;
		E[12] = E[13] = E[14] = E[15] = PE;

		XBRZ4(16_565, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E[3], E[7], E[10], E[11], E[12], E[13], E[14], E[15]);
		XBRZ4(16_565, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E[0], E[1], E[6], E[2], E[15], E[11], E[7], E[3]);
		XBRZ4(16_565, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E[12], E[8], E[5], E[4], E[3], E[2], E[1], E[0]);
		XBRZ4(16_565, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E[15], E[14], E[9], E[13], E[0], E[4], E[8], E[12]);

		/* copy resulting pixels into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst0[3] = E[3];
		dst1[0] = E[4];
		dst1[1] = E[5];
		dst1[2] = E[6];
		dst1[3] = E[7];
		dst2[0] = E[8];
		dst2[1] = E[9];
		dst2[2] = E[10];
		dst2[3] = E[11];
		dst3[0] = E[12];
		dst3[1] = E[13];
		dst3[2] = E[14];
		dst3[3] = E[15];

		tl = top.br;
		bl = bot.tr;

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 4;
		dst1 += 4;
		dst2 += 4;
		dst3 += 4;
	}
}

void xbrz4x_16_555_def(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3, const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2, const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned i;
	unsigned tl = XBRZ_NONE, bl = XBRZ_NONE;

	for (i = 0; i < count; ++i) {
		interp_uint16 PA, PB, PC, PD, PE, PF, PG, PH, PI;
		interp_uint16 B1, C1, C4, F4, I4, H5, I5;
		interp_uint16 E[16];
		struct xbrz_corner top, bot;
		unsigned tr, br;

		/* first column */
		if (i > 0) {
			PA = src1[-1] & 0x7FFFU;
			PD = src2[-1] & 0x7FFFU;
			PG = src3[-1] & 0x7FFFU;
		} else {
			PA = src1[0] & 0x7FFFU;
			PD = src2[0] & 0x7FFFU;
			PG = src3[0] & 0x7FFFU;
		}

		/* central */
		B1 = src0[0] & 0x7FFFU;
		PB = src1[0] & 0x7FFFU;
		PE = src2[0] & 0x7FFFU;
		PH = src3[0] & 0x7FFFU;
		H5 = src4[0] & 0x7FFFU;

		/* last two columns */
		if (count - i > 2) {
			C1 = src0[1] & 0x7FFFU;
			PC = src1[1] & 0x7FFFU;
			PF = src2[1] & 0x7FFFU;
			PI = src3[1] & 0x7FFFU;
			I5 = src4[1] & 0x7FFFU;

			C4 = src1[2] & 0x7FFFU;
			F4 = src2[2] & 0x7FFFU;
			I4 = src3[2] & 0x7FFFU;
		} else if (count - i > 1) {
			C1 = src0[1] & 0x7FFFU;
			PC = src1[1] & 0x7FFFU;
			PF = src2[1] & 0x7FFFU;
			PI = src3[1] & 0x7FFFU;
			I5 = src4[1] & 0x7FFFU;

			C4 = src1[1] & 0x7FFFU;
			F4 = src2[1] & 0x7FFFU;
			I4 = src3[1] & 0x7FFFU;
		} else {
			C1 = src0[0] & 0x7FFFU;
			PC = src1[0] & 0x7FFFU;
			PF = src2[0] & 0x7FFFU;
			PI = src3[0] & 0x7FFFU;
			I5 = src4[0] & 0x7FFFU;

			C4 = src1[0] & 0x7FFFU;
			F4 = src2[0] & 0x7FFFU;
			I4 = src3[0] & 0x7FFFU;
		}

		/* classify the upper-right and lower-right vertices */
		XBRZ_PREPROCESS(16_555, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PH, PI, top);
		XBRZ_PREPROCESS(16_555, PB, PC, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, bot);
		tr = top.bl;
		br = bot.tl;

		/* default pixels */
		E[0] = E[1] = E[2] = E[3] = PE;
		E[4] = E[5] = E[6] = E[7] = PE;
		E[8] = E[9] = E[10] = E[11] = PE;
		E[12] = E[13] = E[14] = E[15] = PE;

		XBRZ4(16_555, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E[3], E[7], E[10], E[11], E[12], E[13], E[14], E[15]);
		XBRZ4(16_555, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E[0], E[1], E[6], E[2], E[15], E[11], E[7], E[3]);
		XBRZ4(16_555, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E[12], E[8], E[5], E[4], E[3], E[2], E[1], E[0]);
		XBRZ4(16_555, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E[15], E[14], E[9], E[13], E[0], E[4], E[8], E[12]);

		/* copy resulting pixels into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst0[3] = E[3];
		dst1[0] = E[4];
		dst1[1] = E[5];
		dst1[2] = E[6];
		dst1[3] = E[7];
		dst2[0] = E[8];
		dst2[1] = E[9];
		dst2[2] = E[10];
		dst2[3] = E[11];
		dst3[0] = E[12];
		dst3[1] = E[13];
		dst3[2] = E[14];
		dst3[3] = E[15];

		tl = top.br;
		bl = bot.tr;

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 4;
		dst1 += 4;
		dst2 += 4;
		dst3 += 4;
	}
}

void xbrz4x_32_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned i;
	unsigned tl = XBRZ_NONE, bl = XBRZ_NONE;

	for (i = 0; i < count; ++i) {
		interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, PI;
		interp_uint32 B1, C1, C4, F4, I4, H5, I5;
		interp_uint32 E[16];
		struct xbrz_corner top, bot;
		unsigned tr, br;

		/* first column */
		if (i > 0) {
			PA = src1[-1] & 0xFFFFFFU;
			PD = src2[-1] & 0xFFFFFFU;
			PG = src3[-1] & 0xFFFFFFU;
		} else {
			PA = src1[0] & 0xFFFFFFU;
			PD = src2[0] & 0xFFFFFFU;
			PG = src3[0] & 0xFFFFFFU;
		}

		/* central */
		B1 = src0[0] & 0xFFFFFFU;
		PB = src1[0] & 0xFFFFFFU;
		PE = src2[0] & 0xFFFFFFU;
		PH = src3[0] & 0xFFFFFFU;
		H5 = src4[0] & 0xFFFFFFU;

		/* last two columns */
		if (count - i > 2) {
			C1 = src0[1] & 0xFFFFFFU;
			PC = src1[1] & 0xFFFFFFU;
			PF = src2[1] & 0xFFFFFFU;
			PI = src3[1] & 0xFFFFFFU;
			I5 = src4[1] & 0xFFFFFFU;

			C4 = src1[2] & 0xFFFFFFU;
			F4 = src2[2] & 0xFFFFFFU;
			I4 = src3[2] & 0xFFFFFFU;
		} else if (count - i > 1) {
			C1 = src0[1] & 0xFFFFFFU;
			PC = src1[1] & 0xFFFFFFU;
			PF = src2[1] & 0xFFFFFFU;
			PI = src3[1] & 0xFFFFFFU;
			I5 = src4[1] & 0xFFFFFFU;

			C4 = src1[1] & 0xFFFFFFU;
			F4 = src2[1] & 0xFFFFFFU;
			I4 = src3[1] & 0xFFFFFFU;
		} else {
			C1 = src0[0] & 0xFFFFFFU;
			PC = src1[0] & 0xFFFFFFU;
			PF = src2[0] & 0xFFFFFFU;
			PI = src3[0] & 0xFFFFFFU;
			I5 = src4[0] & 0xFFFFFFU;

			C4 = src1[0] & 0xFFFFFFU;
			F4 = src2[0] & 0xFFFFFFU;
			I4 = src3[0] & 0xFFFFFFU;
		}

		/* classify the upper-right and lower-right vertices */
		XBRZ_PREPROCESS(32, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PH, PI, top);
		XBRZ_PREPROCESS(32, PB, PC, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, bot);
		tr = top.bl;
		br = bot.tl;

		/* default pixels */
		E[0] = E[1] = E[2] = E[3] = PE;
		E[4] = E[5] = E[6] = E[7] = PE;
		E[8] = E[9] = E[10] = E[11] = PE;
		E[12] = E[13] = E[14] = E[15] = PE;

		XBRZ4(32, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E[3], E[7], E[10], E[11], E[12], E[13], E[14], E[15]);
		XBRZ4(32, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E[0], E[1], E[6], E[2], E[15], E[11], E[7], E[3]);
		XBRZ4(32, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E[12], E[8], E[5], E[4], E[3], E[2], E[1], E[0]);
		XBRZ4(32, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E[15], E[14], E[9], E[13], E[0], E[4], E[8], E[12]);

		/* copy resulting pixels into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst0[3] = E[3];
		dst1[0] = E[4];
		dst1[1] = E[5];
		dst1[2] = E[6];
		dst1[3] = E[7];
		dst2[0] = E[8];
		dst2[1] = E[9];
		dst2[2] = E[10];
		dst2[3] = E[11];
		dst3[0] = E[12];
		dst3[1] = E[13];
		dst3[2] = E[14];
		dst3[3] = E[15];

		tl = top.br;
		bl = bot.tr;

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 4;
		dst1 += 4;
		dst2 += 4;
		dst3 += 4;
	}
}

void xbrz4x_yuy2_def(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3, const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2, const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned i;
	unsigned tl = XBRZ_NONE, bl = XBRZ_NONE;

	for (i = 0; i < count; ++i) {
		interp_uint32 PA, PB, PC, PD, PE, PF, PG, PH, PI;
		interp_uint32 B1, C1, C4, F4, I4, H5, I5;
		interp_uint32 E[16];
		struct xbrz_corner top, bot;
		unsigned tr, br;

		/* first column */
		if (i > 0) {
			PA = src1[-1];
			PD = src2[-1];
			PG = src3[-1];
		} else {
			PA = src1[0];
			PD = src2[0];
			PG = src3[0];
		}

		/* central */
		B1 = src0[0];
		PB = src1[0];
		PE = src2[0];
		PH = src3[0];
		H5 = src4[0];

		/* last two columns */
		if (count - i > 2) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			PI = src3[1];
			I5 = src4[1];

			C4 = src1[2];
			F4 = src2[2];
			I4 = src3[2];
		} else if (count - i > 1) {
			C1 = src0[1];
			PC = src1[1];
			PF = src2[1];
			PI = src3[1];
			I5 = src4[1];

			C4 = src1[1];
			F4 = src2[1];
			I4 = src3[1];
		} else {
			C1 = src0[0];
			PC = src1[0];
			PF = src2[0];
			PI = src3[0];
			I5 = src4[0];

			C4 = src1[0];
			F4 = src2[0];
			I4 = src3[0];
		}

		/* classify the upper-right and lower-right vertices */
		XBRZ_PREPROCESS(yuy2, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PH, PI, top);
		XBRZ_PREPROCESS(yuy2, PB, PC, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, bot);
		tr = top.bl;
		br = bot.tl;

		/* default pixels */
		E[0] = E[1] = E[2] = E[3] = PE;
		E[4] = E[5] = E[6] = E[7] = PE;
		E[8] = E[9] = E[10] = E[11] = PE;
		E[12] = E[13] = E[14] = E[15] = PE;

		XBRZ4(yuy2, PB, PC, PD, PE, PF, PG, PH, PI, tr, bl, br, E[3], E[7], E[10], E[11], E[12], E[13], E[14], E[15]);
		XBRZ4(yuy2, PD, PA, PH, PE, PB, PI, PF, PC, tl, br, tr, E[0], E[1], E[6], E[2], E[15], E[11], E[7], E[3]);
		XBRZ4(yuy2, PH, PG, PF, PE, PD, PC, PB, PA, bl, tr, tl, E[12], E[8], E[5], E[4], E[3], E[2], E[1], E[0]);
		XBRZ4(yuy2, PF, PI, PB, PE, PH, PA, PD, PG, br, tl, bl, E[15], E[14], E[9], E[13], E[0], E[4], E[8], E[12]);

		/* copy resulting pixels into dst */
		dst0[0] = E[0];
		dst0[1] = E[1];
		dst0[2] = E[2];
		dst0[3] = E[3];
		dst1[0] = E[4];
		dst1[1] = E[5];
		dst1[2] = E[6];
		dst1[3] = E[7];
		dst2[0] = E[8];
		dst2[1] = E[9];
		dst2[2] = E[10];
		dst2[3] = E[11];
		dst3[0] = E[12];
		dst3[1] = E[13];
		dst3[2] = E[14];
		dst3[3] = E[15];

		tl = top.br;
		bl = bot.tr;

		src0 += 1;
		src1 += 1;
		src2 += 1;
		src3 += 1;
		src4 += 1;
		dst0 += 4;
		dst1 += 4;
		dst2 += 4;
		dst3 += 4;
	}
}
