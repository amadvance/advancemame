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

#include <assert.h>

#include "scale3k.h"

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

#include "interp_neon.h"

/***************************************************************************/
/* Scale3k NEON: 128-bit AArch64 / 64-bit ARMv7, one output row per pass. */

/**
 * Separate border/center passes write one complete destination row at a time.
 * This layout was selected from performance measurements on Raspberry Pi 3/4,
 * particularly with OpenMP enabled. The same layout can be slower on x86;
 * keep this choice specific to the NEON implementation.
 */
#define SCALE3K_NEON(fmt, vec, any, eq, vand, vor, vbic, select, A, B, C, D, E, F, G, H, I, N0, N1, N2, N3, N4, N5, N6, N7, N8) \
	do { \
		vec _gate = vbic(eq((D), (B)), eq((B), (E))); \
		if (any(_gate)) { \
			vec _block = vand(eq((B), (C)), eq((D), (G))); \
			vec _square = vbic(vand(_gate, _block), eq((A), (E))); \
			vec _other = vbic(_gate, _block); \
			vec _horizontal = vand(_other, vbic(eq((B), (C)), eq((C), (F)))); \
			vec _remaining = vbic(_other, _horizontal); \
			vec _vertical = vand(_remaining, vbic(eq((D), (G)), eq((G), (H)))); \
			vec _diagonal = vbic(_remaining, _vertical); \
			vec _old0 = (N0), _old1 = (N1), _old2 = (N2), _old3 = (N3), _old6 = (N6); \
			(N0) = select(_square, interp_ ## fmt ## _31_neon((D), _old0), \
					select(vor(_horizontal, _vertical), (D), \
					select(_diagonal, interp_ ## fmt ## _71_neon((D), _old0), _old0))); \
			(N1) = select(vor(_square, _horizontal), interp_ ## fmt ## _31_neon((D), _old1), \
					select(_vertical, interp_ ## fmt ## _31_neon(_old1, (D)), \
					select(_diagonal, interp_ ## fmt ## _71_neon(_old1, (D)), _old1))); \
			(N2) = select(vor(_square, _horizontal), interp_ ## fmt ## _31_neon(_old2, (D)), _old2); \
			(N3) = select(vor(_square, _vertical), interp_ ## fmt ## _31_neon((D), _old3), \
					select(_horizontal, interp_ ## fmt ## _31_neon(_old3, (D)), \
					select(_diagonal, interp_ ## fmt ## _71_neon(_old3, (D)), _old3))); \
			(N6) = select(vor(_square, _vertical), interp_ ## fmt ## _31_neon(_old6, (D)), _old6); \
		} \
	} while (0)

static inline void scale3k_16_565_border_pixel(interp_uint16* restrict dst,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, unsigned count, unsigned x)
{
	unsigned left = x > 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
	interp_uint16 E5 = E, E6 = E, E7 = E, E8 = E;

	SCALE3K(16_565, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
	SCALE3K(16_565, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
	SCALE3K(16_565, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE3K(16_565, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

	dst[3 * x + 0] = E0;
	dst[3 * x + 1] = E1;
	dst[3 * x + 2] = E2;
}

static inline void scale3k_16_565_border_neon(interp_uint16* restrict dst,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	unsigned x = 0;

	scale3k_16_565_border_pixel(dst, src0, src1, src2, count, x++);

#if defined(CONFIG_NEON)
	/* 8 source pixels per vector, with a scalar right border. */
	for (; count - x > 8; x += 8) {
		uint16x8_t A = vld1q_u16(src0 + x - 1);
		uint16x8_t B = vld1q_u16(src0 + x);
		uint16x8_t C = vld1q_u16(src0 + x + 1);
		uint16x8_t D = vld1q_u16(src1 + x - 1);
		uint16x8_t E = vld1q_u16(src1 + x);
		uint16x8_t F = vld1q_u16(src1 + x + 1);
		uint16x8_t G = vld1q_u16(src2 + x - 1);
		uint16x8_t H = vld1q_u16(src2 + x);
		uint16x8_t I = vld1q_u16(src2 + x + 1);
		uint16x8_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x8_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_store3_neon(dst + 3 * x, E0, E1, E2);
	}
#elif defined(CONFIG_NEON32)
	/* 4 source pixels per vector, with a scalar right border. */
	for (; count - x > 4; x += 4) {
		uint16x4_t A = vld1_u16(src0 + x - 1);
		uint16x4_t B = vld1_u16(src0 + x);
		uint16x4_t C = vld1_u16(src0 + x + 1);
		uint16x4_t D = vld1_u16(src1 + x - 1);
		uint16x4_t E = vld1_u16(src1 + x);
		uint16x4_t F = vld1_u16(src1 + x + 1);
		uint16x4_t G = vld1_u16(src2 + x - 1);
		uint16x4_t H = vld1_u16(src2 + x);
		uint16x4_t I = vld1_u16(src2 + x + 1);
		uint16x4_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x4_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_64_store3_neon(dst + 3 * x, E0, E1, E2);
	}
#endif

	for (; x < count; ++x)
		scale3k_16_565_border_pixel(dst, src0, src1, src2, count, x);
}

static inline void scale3k_16_565_center_pixel(interp_uint16* restrict dst,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, unsigned count, unsigned x)
{
	unsigned left = x > 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
	interp_uint16 E5 = E, E6 = E, E7 = E, E8 = E;

	SCALE3K(16_565, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
	SCALE3K(16_565, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
	SCALE3K(16_565, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE3K(16_565, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

	dst[3 * x + 0] = E3;
	dst[3 * x + 1] = E4;
	dst[3 * x + 2] = E5;
}

static inline void scale3k_16_565_center_neon(interp_uint16* restrict dst,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	unsigned x = 0;

	scale3k_16_565_center_pixel(dst, src0, src1, src2, count, x++);

#if defined(CONFIG_NEON)
	/* 8 source pixels per vector, with a scalar right border. */
	for (; count - x > 8; x += 8) {
		uint16x8_t A = vld1q_u16(src0 + x - 1);
		uint16x8_t B = vld1q_u16(src0 + x);
		uint16x8_t C = vld1q_u16(src0 + x + 1);
		uint16x8_t D = vld1q_u16(src1 + x - 1);
		uint16x8_t E = vld1q_u16(src1 + x);
		uint16x8_t F = vld1q_u16(src1 + x + 1);
		uint16x8_t G = vld1q_u16(src2 + x - 1);
		uint16x8_t H = vld1q_u16(src2 + x);
		uint16x8_t I = vld1q_u16(src2 + x + 1);
		uint16x8_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x8_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_565, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_store3_neon(dst + 3 * x, E3, E4, E5);
	}
#elif defined(CONFIG_NEON32)
	/* 4 source pixels per vector, with a scalar right border. */
	for (; count - x > 4; x += 4) {
		uint16x4_t A = vld1_u16(src0 + x - 1);
		uint16x4_t B = vld1_u16(src0 + x);
		uint16x4_t C = vld1_u16(src0 + x + 1);
		uint16x4_t D = vld1_u16(src1 + x - 1);
		uint16x4_t E = vld1_u16(src1 + x);
		uint16x4_t F = vld1_u16(src1 + x + 1);
		uint16x4_t G = vld1_u16(src2 + x - 1);
		uint16x4_t H = vld1_u16(src2 + x);
		uint16x4_t I = vld1_u16(src2 + x + 1);
		uint16x4_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x4_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_565_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_64_store3_neon(dst + 3 * x, E3, E4, E5);
	}
#endif

	for (; x < count; ++x)
		scale3k_16_565_center_pixel(dst, src0, src1, src2, count, x);
}

void scale3k_16_565_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	assert(count >= 2);

	/* Write one complete output row at a time, like Scale2x3. */
	scale3k_16_565_border_neon(dst0, src0, src1, src2, count);
	scale3k_16_565_center_neon(dst1, src0, src1, src2, count);
	scale3k_16_565_border_neon(dst2, src2, src1, src0, count);
}

static inline void scale3k_16_555_border_pixel(interp_uint16* restrict dst,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, unsigned count, unsigned x)
{
	unsigned left = x > 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
	interp_uint16 E5 = E, E6 = E, E7 = E, E8 = E;

	SCALE3K(16_555, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
	SCALE3K(16_555, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
	SCALE3K(16_555, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE3K(16_555, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

	dst[3 * x + 0] = E0;
	dst[3 * x + 1] = E1;
	dst[3 * x + 2] = E2;
}

static inline void scale3k_16_555_border_neon(interp_uint16* restrict dst,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	unsigned x = 0;

	scale3k_16_555_border_pixel(dst, src0, src1, src2, count, x++);

#if defined(CONFIG_NEON)
	/* 8 source pixels per vector, with a scalar right border. */
	for (; count - x > 8; x += 8) {
		uint16x8_t A = vld1q_u16(src0 + x - 1);
		uint16x8_t B = vld1q_u16(src0 + x);
		uint16x8_t C = vld1q_u16(src0 + x + 1);
		uint16x8_t D = vld1q_u16(src1 + x - 1);
		uint16x8_t E = vld1q_u16(src1 + x);
		uint16x8_t F = vld1q_u16(src1 + x + 1);
		uint16x8_t G = vld1q_u16(src2 + x - 1);
		uint16x8_t H = vld1q_u16(src2 + x);
		uint16x8_t I = vld1q_u16(src2 + x + 1);
		uint16x8_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x8_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_store3_neon(dst + 3 * x, E0, E1, E2);
	}
#elif defined(CONFIG_NEON32)
	/* 4 source pixels per vector, with a scalar right border. */
	for (; count - x > 4; x += 4) {
		uint16x4_t A = vld1_u16(src0 + x - 1);
		uint16x4_t B = vld1_u16(src0 + x);
		uint16x4_t C = vld1_u16(src0 + x + 1);
		uint16x4_t D = vld1_u16(src1 + x - 1);
		uint16x4_t E = vld1_u16(src1 + x);
		uint16x4_t F = vld1_u16(src1 + x + 1);
		uint16x4_t G = vld1_u16(src2 + x - 1);
		uint16x4_t H = vld1_u16(src2 + x);
		uint16x4_t I = vld1_u16(src2 + x + 1);
		uint16x4_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x4_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_64_store3_neon(dst + 3 * x, E0, E1, E2);
	}
#endif

	for (; x < count; ++x)
		scale3k_16_555_border_pixel(dst, src0, src1, src2, count, x);
}

static inline void scale3k_16_555_center_pixel(interp_uint16* restrict dst,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, unsigned count, unsigned x)
{
	unsigned left = x > 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
	interp_uint16 E5 = E, E6 = E, E7 = E, E8 = E;

	SCALE3K(16_555, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
	SCALE3K(16_555, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
	SCALE3K(16_555, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE3K(16_555, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

	dst[3 * x + 0] = E3;
	dst[3 * x + 1] = E4;
	dst[3 * x + 2] = E5;
}

static inline void scale3k_16_555_center_neon(interp_uint16* restrict dst,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	unsigned x = 0;

	scale3k_16_555_center_pixel(dst, src0, src1, src2, count, x++);

#if defined(CONFIG_NEON)
	/* 8 source pixels per vector, with a scalar right border. */
	for (; count - x > 8; x += 8) {
		uint16x8_t A = vld1q_u16(src0 + x - 1);
		uint16x8_t B = vld1q_u16(src0 + x);
		uint16x8_t C = vld1q_u16(src0 + x + 1);
		uint16x8_t D = vld1q_u16(src1 + x - 1);
		uint16x8_t E = vld1q_u16(src1 + x);
		uint16x8_t F = vld1q_u16(src1 + x + 1);
		uint16x8_t G = vld1q_u16(src2 + x - 1);
		uint16x8_t H = vld1q_u16(src2 + x);
		uint16x8_t I = vld1q_u16(src2 + x + 1);
		uint16x8_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x8_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_555, uint16x8_t, interp_16_any_neon, vceqq_u16, vandq_u16, vorrq_u16, vbicq_u16, vbslq_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_store3_neon(dst + 3 * x, E3, E4, E5);
	}
#elif defined(CONFIG_NEON32)
	/* 4 source pixels per vector, with a scalar right border. */
	for (; count - x > 4; x += 4) {
		uint16x4_t A = vld1_u16(src0 + x - 1);
		uint16x4_t B = vld1_u16(src0 + x);
		uint16x4_t C = vld1_u16(src0 + x + 1);
		uint16x4_t D = vld1_u16(src1 + x - 1);
		uint16x4_t E = vld1_u16(src1 + x);
		uint16x4_t F = vld1_u16(src1 + x + 1);
		uint16x4_t G = vld1_u16(src2 + x - 1);
		uint16x4_t H = vld1_u16(src2 + x);
		uint16x4_t I = vld1_u16(src2 + x + 1);
		uint16x4_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint16x4_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(16_555_64, uint16x4_t, interp_16_64_any_neon, vceq_u16, vand_u16, vorr_u16, vbic_u16, vbsl_u16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_16_64_store3_neon(dst + 3 * x, E3, E4, E5);
	}
#endif

	for (; x < count; ++x)
		scale3k_16_555_center_pixel(dst, src0, src1, src2, count, x);
}

void scale3k_16_555_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, unsigned count)
{
	assert(count >= 2);

	/* Write one complete output row at a time, like Scale2x3. */
	scale3k_16_555_border_neon(dst0, src0, src1, src2, count);
	scale3k_16_555_center_neon(dst1, src0, src1, src2, count);
	scale3k_16_555_border_neon(dst2, src2, src1, src0, count);
}

static inline void scale3k_32_border_pixel(interp_uint32* restrict dst,
	const interp_uint32* src0, const interp_uint32* src1,
	const interp_uint32* src2, unsigned count, unsigned x)
{
	unsigned left = x > 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint32 A = src0[left], B = src0[x], C = src0[right];
	interp_uint32 D = src1[left], E = src1[x], F = src1[right];
	interp_uint32 G = src2[left], H = src2[x], I = src2[right];
	interp_uint32 E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
	interp_uint32 E5 = E, E6 = E, E7 = E, E8 = E;

	SCALE3K(32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
	SCALE3K(32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
	SCALE3K(32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE3K(32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

	dst[3 * x + 0] = E0;
	dst[3 * x + 1] = E1;
	dst[3 * x + 2] = E2;
}

static inline void scale3k_32_border_neon(interp_uint32* restrict dst,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, unsigned count)
{
	unsigned x = 0;

	scale3k_32_border_pixel(dst, src0, src1, src2, count, x++);

#if defined(CONFIG_NEON)
	/* 4 source pixels per vector, with a scalar right border. */
	for (; count - x > 4; x += 4) {
		uint32x4_t A = vld1q_u32(src0 + x - 1);
		uint32x4_t B = vld1q_u32(src0 + x);
		uint32x4_t C = vld1q_u32(src0 + x + 1);
		uint32x4_t D = vld1q_u32(src1 + x - 1);
		uint32x4_t E = vld1q_u32(src1 + x);
		uint32x4_t F = vld1q_u32(src1 + x + 1);
		uint32x4_t G = vld1q_u32(src2 + x - 1);
		uint32x4_t H = vld1q_u32(src2 + x);
		uint32x4_t I = vld1q_u32(src2 + x + 1);
		uint32x4_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint32x4_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_32_store3_neon(dst + 3 * x, E0, E1, E2);
	}
#elif defined(CONFIG_NEON32)
	/* 2 source pixels per vector, with a scalar right border. */
	for (; count - x > 2; x += 2) {
		uint32x2_t A = vld1_u32(src0 + x - 1);
		uint32x2_t B = vld1_u32(src0 + x);
		uint32x2_t C = vld1_u32(src0 + x + 1);
		uint32x2_t D = vld1_u32(src1 + x - 1);
		uint32x2_t E = vld1_u32(src1 + x);
		uint32x2_t F = vld1_u32(src1 + x + 1);
		uint32x2_t G = vld1_u32(src2 + x - 1);
		uint32x2_t H = vld1_u32(src2 + x);
		uint32x2_t I = vld1_u32(src2 + x + 1);
		uint32x2_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint32x2_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_32_64_store3_neon(dst + 3 * x, E0, E1, E2);
	}
#endif

	for (; x < count; ++x)
		scale3k_32_border_pixel(dst, src0, src1, src2, count, x);
}

static inline void scale3k_32_center_pixel(interp_uint32* restrict dst,
	const interp_uint32* src0, const interp_uint32* src1,
	const interp_uint32* src2, unsigned count, unsigned x)
{
	unsigned left = x > 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint32 A = src0[left], B = src0[x], C = src0[right];
	interp_uint32 D = src1[left], E = src1[x], F = src1[right];
	interp_uint32 G = src2[left], H = src2[x], I = src2[right];
	interp_uint32 E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
	interp_uint32 E5 = E, E6 = E, E7 = E, E8 = E;

	SCALE3K(32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
	SCALE3K(32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
	SCALE3K(32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE3K(32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

	dst[3 * x + 0] = E3;
	dst[3 * x + 1] = E4;
	dst[3 * x + 2] = E5;
}

static inline void scale3k_32_center_neon(interp_uint32* restrict dst,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, unsigned count)
{
	unsigned x = 0;

	scale3k_32_center_pixel(dst, src0, src1, src2, count, x++);

#if defined(CONFIG_NEON)
	/* 4 source pixels per vector, with a scalar right border. */
	for (; count - x > 4; x += 4) {
		uint32x4_t A = vld1q_u32(src0 + x - 1);
		uint32x4_t B = vld1q_u32(src0 + x);
		uint32x4_t C = vld1q_u32(src0 + x + 1);
		uint32x4_t D = vld1q_u32(src1 + x - 1);
		uint32x4_t E = vld1q_u32(src1 + x);
		uint32x4_t F = vld1q_u32(src1 + x + 1);
		uint32x4_t G = vld1q_u32(src2 + x - 1);
		uint32x4_t H = vld1q_u32(src2 + x);
		uint32x4_t I = vld1q_u32(src2 + x + 1);
		uint32x4_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint32x4_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(32, uint32x4_t, interp_32_any_neon, vceqq_u32, vandq_u32, vorrq_u32, vbicq_u32, vbslq_u32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_32_store3_neon(dst + 3 * x, E3, E4, E5);
	}
#elif defined(CONFIG_NEON32)
	/* 2 source pixels per vector, with a scalar right border. */
	for (; count - x > 2; x += 2) {
		uint32x2_t A = vld1_u32(src0 + x - 1);
		uint32x2_t B = vld1_u32(src0 + x);
		uint32x2_t C = vld1_u32(src0 + x + 1);
		uint32x2_t D = vld1_u32(src1 + x - 1);
		uint32x2_t E = vld1_u32(src1 + x);
		uint32x2_t F = vld1_u32(src1 + x + 1);
		uint32x2_t G = vld1_u32(src2 + x - 1);
		uint32x2_t H = vld1_u32(src2 + x);
		uint32x2_t I = vld1_u32(src2 + x + 1);
		uint32x2_t E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		uint32x2_t E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_NEON(32_64, uint32x2_t, interp_32_64_any_neon, vceq_u32, vand_u32, vorr_u32, vbic_u32, vbsl_u32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		interp_32_64_store3_neon(dst + 3 * x, E3, E4, E5);
	}
#endif

	for (; x < count; ++x)
		scale3k_32_center_pixel(dst, src0, src1, src2, count, x);
}

void scale3k_32_neon(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	interp_uint32* restrict dst2,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, unsigned count)
{
	assert(count >= 2);

	/* Write one complete output row at a time, like Scale2x3. */
	scale3k_32_border_neon(dst0, src0, src1, src2, count);
	scale3k_32_center_neon(dst1, src0, src1, src2, count);
	scale3k_32_border_neon(dst2, src2, src1, src0, count);
}

#endif

