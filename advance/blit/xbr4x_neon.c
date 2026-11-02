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

#include "xbr4x.h"

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

#include "interp_neon.h"

#define XBR_NEON(fmt, vec, any, eq, gt, le, ge, add, shl, vand, vor, bic, inv, select, zero, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3, N4, N5, N6, N7, N8, N9, N10, N11, N12, N13, N14, N15) \
	do { \
		vec _gate = inv(vor(eq((PE), (PH)), eq((PE), (PF)))); \
		if (any(_gate)) { \
			vec _e = interp_ ## fmt ## _score_neon((PC), (PE), (PG), (H5), (PI), (F4), (PH), (PF)); \
			vec _i = interp_ ## fmt ## _score_neon((PD), (PH), (I5), (I4), (PF), (PB), (PE), (PI)); \
			vec _active = vand(_gate, gt(_i, _e)); \
			if (any(_active)) { \
				vec _ke = interp_ ## fmt ## _dist_neon((PF), (PG)); \
				vec _ki = interp_ ## fmt ## _dist_neon((PH), (PC)); \
				vec _ex2 = bic(_active, vor(eq((PE), (PC)), eq((PB), (PC)))); \
				vec _ex3 = bic(_active, vor(eq((PE), (PG)), eq((PD), (PG)))); \
				vec _leftup = vand(vand(_ex2, _ex3), vand(eq(_ke, zero(0)), eq(_ki, zero(0)))); \
				vec _left = bic(vand(_ex3, le(shl(_ke, 1), _ki)), _leftup); \
				vec _up = bic(vand(_ex2, ge(_ke, shl(_ki, 1))), vor(_leftup, _left)); \
				vec _dia = bic(_active, vor(vor(_leftup, _left), _up)); \
				vec _px = select(gt(interp_ ## fmt ## _dist_neon((PE), (PF)), interp_ ## fmt ## _dist_neon((PE), (PH))), (PH), (PF)); \
				vec _lup13 = interp_ ## fmt ## _31_neon(_px, (N13)); \
				vec _lup12 = interp_ ## fmt ## _31_neon((N12), _px); \
				(N7) = select(_leftup, _lup13, (N7)); \
				(N13) = select(_leftup, _lup13, (N13)); \
				(N3) = select(_leftup, _lup12, (N3)); \
				(N10) = select(_leftup, _lup12, (N10)); \
				(N12) = select(_leftup, _lup12, (N12)); \
				(N11) = select(_leftup, _px, (N11)); \
				(N14) = select(_leftup, _px, (N14)); \
				(N15) = select(_leftup, _px, (N15)); \
				(N11) = select(_left, interp_ ## fmt ## _31_neon(_px, (N11)), (N11)); \
				(N13) = select(_left, interp_ ## fmt ## _31_neon(_px, (N13)), (N13)); \
				(N10) = select(_left, interp_ ## fmt ## _31_neon((N10), _px), (N10)); \
				(N12) = select(_left, interp_ ## fmt ## _31_neon((N12), _px), (N12)); \
				(N14) = select(_left, _px, (N14)); \
				(N15) = select(_left, _px, (N15)); \
				(N14) = select(_up, interp_ ## fmt ## _31_neon(_px, (N14)), (N14)); \
				(N7) = select(_up, interp_ ## fmt ## _31_neon(_px, (N7)), (N7)); \
				(N10) = select(_up, interp_ ## fmt ## _31_neon((N10), _px), (N10)); \
				(N3) = select(_up, interp_ ## fmt ## _31_neon((N3), _px), (N3)); \
				(N11) = select(_up, _px, (N11)); \
				(N15) = select(_up, _px, (N15)); \
				(N11) = select(_dia, interp_ ## fmt ## _11_neon((N11), _px), (N11)); \
				(N14) = select(_dia, interp_ ## fmt ## _11_neon((N14), _px), (N14)); \
				(N15) = select(_dia, _px, (N15)); \
			} \
		} \
	} while (0)

static inline void xbr4_565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count, unsigned x)
{
	unsigned xm2 = x >= 2 ? x - 2 : 0;
	unsigned xm1 = x >= 1 ? x - 1 : 0;
	unsigned xp1 = count - x >= 2 ? x + 1 : count - 1;
	unsigned xp2 = count - x >= 3 ? x + 2 : count - 1;
	interp_uint16 A1 = src0[xm1], B1 = src0[x], C1 = src0[xp1];
	interp_uint16 A0 = src1[xm2], PA = src1[xm1], PB = src1[x], PC = src1[xp1], C4 = src1[xp2];
	interp_uint16 D0 = src2[xm2], PD = src2[xm1], PE = src2[x], PF = src2[xp1], F4 = src2[xp2];
	interp_uint16 G0 = src3[xm2], PG = src3[xm1], PH = src3[x], PI = src3[xp1], I4 = src3[xp2];
	interp_uint16 G5 = src4[xm1], H5 = src4[x], I5 = src4[xp1];
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
	XBR4(16_565, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
	XBR4(16_565, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
	XBR4(16_565, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	XBR4(16_565, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
	dst0[4 * x + 0] = E0;
	dst0[4 * x + 1] = E1;
	dst0[4 * x + 2] = E2;
	dst0[4 * x + 3] = E3;
	dst1[4 * x + 0] = E4;
	dst1[4 * x + 1] = E5;
	dst1[4 * x + 2] = E6;
	dst1[4 * x + 3] = E7;
	dst2[4 * x + 0] = E8;
	dst2[4 * x + 1] = E9;
	dst2[4 * x + 2] = E10;
	dst2[4 * x + 3] = E11;
	dst3[4 * x + 0] = E12;
	dst3[4 * x + 1] = E13;
	dst3[4 * x + 2] = E14;
	dst3[4 * x + 3] = E15;
}

static inline void xbr4_555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count, unsigned x)
{
	unsigned xm2 = x >= 2 ? x - 2 : 0;
	unsigned xm1 = x >= 1 ? x - 1 : 0;
	unsigned xp1 = count - x >= 2 ? x + 1 : count - 1;
	unsigned xp2 = count - x >= 3 ? x + 2 : count - 1;
	interp_uint16 A1 = src0[xm1], B1 = src0[x], C1 = src0[xp1];
	interp_uint16 A0 = src1[xm2], PA = src1[xm1], PB = src1[x], PC = src1[xp1], C4 = src1[xp2];
	interp_uint16 D0 = src2[xm2], PD = src2[xm1], PE = src2[x], PF = src2[xp1], F4 = src2[xp2];
	interp_uint16 G0 = src3[xm2], PG = src3[xm1], PH = src3[x], PI = src3[xp1], I4 = src3[xp2];
	interp_uint16 G5 = src4[xm1], H5 = src4[x], I5 = src4[xp1];
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
	XBR4(16_555, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
	XBR4(16_555, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
	XBR4(16_555, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	XBR4(16_555, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
	dst0[4 * x + 0] = E0;
	dst0[4 * x + 1] = E1;
	dst0[4 * x + 2] = E2;
	dst0[4 * x + 3] = E3;
	dst1[4 * x + 0] = E4;
	dst1[4 * x + 1] = E5;
	dst1[4 * x + 2] = E6;
	dst1[4 * x + 3] = E7;
	dst2[4 * x + 0] = E8;
	dst2[4 * x + 1] = E9;
	dst2[4 * x + 2] = E10;
	dst2[4 * x + 3] = E11;
	dst3[4 * x + 0] = E12;
	dst3[4 * x + 1] = E13;
	dst3[4 * x + 2] = E14;
	dst3[4 * x + 3] = E15;
}

static inline void xbr4_32_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count, unsigned x)
{
	unsigned xm2 = x >= 2 ? x - 2 : 0;
	unsigned xm1 = x >= 1 ? x - 1 : 0;
	unsigned xp1 = count - x >= 2 ? x + 1 : count - 1;
	unsigned xp2 = count - x >= 3 ? x + 2 : count - 1;
	interp_uint32 A1 = src0[xm1], B1 = src0[x], C1 = src0[xp1];
	interp_uint32 A0 = src1[xm2], PA = src1[xm1], PB = src1[x], PC = src1[xp1], C4 = src1[xp2];
	interp_uint32 D0 = src2[xm2], PD = src2[xm1], PE = src2[x], PF = src2[xp1], F4 = src2[xp2];
	interp_uint32 G0 = src3[xm2], PG = src3[xm1], PH = src3[x], PI = src3[xp1], I4 = src3[xp2];
	interp_uint32 G5 = src4[xm1], H5 = src4[x], I5 = src4[xp1];
	interp_uint32 E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
	XBR4(32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
	XBR4(32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
	XBR4(32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	XBR4(32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
	dst0[4 * x + 0] = E0;
	dst0[4 * x + 1] = E1;
	dst0[4 * x + 2] = E2;
	dst0[4 * x + 3] = E3;
	dst1[4 * x + 0] = E4;
	dst1[4 * x + 1] = E5;
	dst1[4 * x + 2] = E6;
	dst1[4 * x + 3] = E7;
	dst2[4 * x + 0] = E8;
	dst2[4 * x + 1] = E9;
	dst2[4 * x + 2] = E10;
	dst2[4 * x + 3] = E11;
	dst3[4 * x + 0] = E12;
	dst3[4 * x + 1] = E13;
	dst3[4 * x + 2] = E14;
	dst3[4 * x + 3] = E15;
}

void xbr4x_16_565_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	if (count == 0)
		return;
	for (; x < count && x < 2; ++x)
		xbr4_565_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

#if defined(CONFIG_NEON)
	/* 8 pixels per block; access range x-2 to x+9. */
	for (; count - x >= 10; x += 8) {
#define LD4(row, off) vld1q_u16((row) + x + (off))
		uint16x8_t A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		uint16x8_t A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		uint16x8_t D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		uint16x8_t G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		uint16x8_t G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		uint16x8_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_16_store4_neon(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_store4_neon(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_store4_neon(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_store4_neon(dst3 + 4 * x, E12, E13, E14, E15);
	}
#elif defined(CONFIG_NEON32)
	/* 4 pixels per block; access range x-2 to x+5. */
	for (; count - x >= 6; x += 4) {
#define LD4(row, off) vld1_u16((row) + x + (off))
		uint16x4_t A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		uint16x4_t A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		uint16x4_t D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		uint16x4_t G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		uint16x4_t G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		uint16x4_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_16_64_store4_neon(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_64_store4_neon(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_64_store4_neon(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_64_store4_neon(dst3 + 4 * x, E12, E13, E14, E15);
	}
#endif

	for (; x < count; ++x)
		xbr4_565_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

void xbr4x_16_555_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1, interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	const interp_uint16* restrict src3, const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;
	if (count == 0)
		return;
	for (; x < count && x < 2; ++x)
		xbr4_555_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

#if defined(CONFIG_NEON)
	/* 8 pixels per block; access range x-2 to x+9. */
	for (; count - x >= 10; x += 8) {
#define LD4(row, off) vld1q_u16((row) + x + (off))
		uint16x8_t A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		uint16x8_t A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		uint16x8_t D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		uint16x8_t G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		uint16x8_t G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		uint16x8_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vcgtq_u16, vcleq_u16, vcgeq_u16, vaddq_u16, vshlq_n_u16, vandq_u16, vorrq_u16, vbicq_u16, vmvnq_u16, vbslq_u16, vdupq_n_u16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_16_store4_neon(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_store4_neon(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_store4_neon(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_store4_neon(dst3 + 4 * x, E12, E13, E14, E15);
	}
#elif defined(CONFIG_NEON32)
	/* 4 pixels per block; access range x-2 to x+5. */
	for (; count - x >= 6; x += 4) {
#define LD4(row, off) vld1_u16((row) + x + (off))
		uint16x4_t A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		uint16x4_t A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		uint16x4_t D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		uint16x4_t G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		uint16x4_t G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		uint16x4_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vcgt_u16, vcle_u16, vcge_u16, vadd_u16, vshl_n_u16, vand_u16, vorr_u16, vbic_u16, vmvn_u16, vbsl_u16, vdup_n_u16, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_16_64_store4_neon(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_64_store4_neon(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_64_store4_neon(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_64_store4_neon(dst3 + 4 * x, E12, E13, E14, E15);
	}
#endif

	for (; x < count; ++x)
		xbr4_555_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

void xbr4x_32_neon(interp_uint32* restrict dst0, interp_uint32* restrict dst1, interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	const interp_uint32* restrict src3, const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;
	if (count == 0)
		return;
	for (; x < count && x < 2; ++x)
		xbr4_32_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);

#if defined(CONFIG_NEON)
	/* 4 pixels per block; access range x-2 to x+5. */
	for (; count - x >= 6; x += 4) {
#define LD4(row, off) vld1q_u32((row) + x + (off))
		uint32x4_t A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		uint32x4_t A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		uint32x4_t D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		uint32x4_t G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		uint32x4_t G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		uint32x4_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vcgtq_u32, vcleq_u32, vcgeq_u32, vaddq_u32, vshlq_n_u32, vandq_u32, vorrq_u32, vbicq_u32, vmvnq_u32, vbslq_u32, vdupq_n_u32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vcgtq_u32, vcleq_u32, vcgeq_u32, vaddq_u32, vshlq_n_u32, vandq_u32, vorrq_u32, vbicq_u32, vmvnq_u32, vbslq_u32, vdupq_n_u32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vcgtq_u32, vcleq_u32, vcgeq_u32, vaddq_u32, vshlq_n_u32, vandq_u32, vorrq_u32, vbicq_u32, vmvnq_u32, vbslq_u32, vdupq_n_u32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vcgtq_u32, vcleq_u32, vcgeq_u32, vaddq_u32, vshlq_n_u32, vandq_u32, vorrq_u32, vbicq_u32, vmvnq_u32, vbslq_u32, vdupq_n_u32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_32_store4_neon(dst0 + 4 * x, E0, E1, E2, E3);
		interp_32_store4_neon(dst1 + 4 * x, E4, E5, E6, E7);
		interp_32_store4_neon(dst2 + 4 * x, E8, E9, E10, E11);
		interp_32_store4_neon(dst3 + 4 * x, E12, E13, E14, E15);
	}
#elif defined(CONFIG_NEON32)
	/* 2 pixels per block; access range x-2 to x+3. */
	for (; count - x >= 4; x += 2) {
#define LD4(row, off) vld1_u32((row) + x + (off))
		uint32x2_t A1 = LD4(src0, -1), B1 = LD4(src0, 0), C1 = LD4(src0, 1);
		uint32x2_t A0 = LD4(src1, -2), PA = LD4(src1, -1), PB = LD4(src1, 0), PC = LD4(src1, 1), C4 = LD4(src1, 2);
		uint32x2_t D0 = LD4(src2, -2), PD = LD4(src2, -1), PE = LD4(src2, 0), PF = LD4(src2, 1), F4 = LD4(src2, 2);
		uint32x2_t G0 = LD4(src3, -2), PG = LD4(src3, -1), PH = LD4(src3, 0), PI = LD4(src3, 1), I4 = LD4(src3, 2);
		uint32x2_t G5 = LD4(src4, -1), H5 = LD4(src4, 0), I5 = LD4(src4, 1);
#undef LD4
		uint32x2_t E0 = PE, E1 = PE, E2 = PE, E3 = PE, E4 = PE, E5 = PE, E6 = PE, E7 = PE, E8 = PE, E9 = PE, E10 = PE, E11 = PE, E12 = PE, E13 = PE, E14 = PE, E15 = PE;
		XBR_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vcgt_u32, vcle_u32, vcge_u32, vadd_u32, vshl_n_u32, vand_u32, vorr_u32, vbic_u32, vmvn_u32, vbsl_u32, vdup_n_u32, PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10, E11, E12, E13, E14, E15);
		XBR_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vcgt_u32, vcle_u32, vcge_u32, vadd_u32, vshl_n_u32, vand_u32, vorr_u32, vbic_u32, vmvn_u32, vbsl_u32, vdup_n_u32, PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E12, E8, E4, E0, E13, E9, E5, E1, E14, E10, E6, E2, E15, E11, E7, E3);
		XBR_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vcgt_u32, vcle_u32, vcge_u32, vadd_u32, vshl_n_u32, vand_u32, vorr_u32, vbic_u32, vmvn_u32, vbsl_u32, vdup_n_u32, PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E15, E14, E13, E12, E11, E10, E9, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		XBR_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vcgt_u32, vcle_u32, vcge_u32, vadd_u32, vshl_n_u32, vand_u32, vorr_u32, vbic_u32, vmvn_u32, vbsl_u32, vdup_n_u32, PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E3, E7, E11, E15, E2, E6, E10, E14, E1, E5, E9, E13, E0, E4, E8, E12);
		interp_32_64_store4_neon(dst0 + 4 * x, E0, E1, E2, E3);
		interp_32_64_store4_neon(dst1 + 4 * x, E4, E5, E6, E7);
		interp_32_64_store4_neon(dst2 + 4 * x, E8, E9, E10, E11);
		interp_32_64_store4_neon(dst3 + 4 * x, E12, E13, E14, E15);
	}
#endif

	for (; x < count; ++x)
		xbr4_32_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, src3, src4, count, x);
}

#endif /* CONFIG_NEON || CONFIG_NEON32 */
