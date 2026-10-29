/*
 * This file is part of the Scale2x project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004, 2008 Andrea Mazzoleni
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
 */

#include "portable.h"

#include "scale2x.h"

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

/***************************************************************************/
/* Scale2x NEON implementation */

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

/*
 * Apply the Scale2x effect at a single row.
 * This function must be called only by the other scale2x functions.
 *
 * Considering the pixel map :
 *
 *      ABC (src0)
 *      DEF (src1)
 *      GHI (src2)
 *
 * this functions compute 2 new pixels in substitution of the source pixel E
 * like this map :
 *
 *      ab (dst)
 *
 * with these variables :
 *
 *      &current -> E
 *      &current_left -> D
 *      &current_right -> F
 *      &current_upper -> B
 *      &current_lower -> H
 *
 *      %0 -> current_upper
 *      %1 -> current
 *      %2 -> current_lower
 *      %3 -> dst
 *      %4 -> counter
 *
 *      %xmm0 -> *current_left
 *      %xmm1 -> *current_next
 *      %xmm2 -> tmp0
 *      %xmm3 -> tmp1
 *      %xmm4 -> tmp2
 *      %xmm5 -> tmp3
 *      %xmm6 -> *current_upper
 *      %xmm7 -> *current
 */
static inline void scale2x_8_neon_center(scale2x_uint8* dst, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && ((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]))) ? src1[1] : src1[0];

#if defined(CONFIG_NEON)
	while (count - x > 16) {
		uint8x16_t b = vld1q_u8(src0 + x);
		uint8x16_t h = vld1q_u8(src2 + x);
		uint8x16_t d = vld1q_u8(src1 + x - 1);
		uint8x16_t f = vld1q_u8(src1 + x + 1);
		uint8x16_t e = vld1q_u8(src1 + x);
		uint8x16_t active = vandq_u8(vmvnq_u8(vceqq_u8(b, h)), vmvnq_u8(vceqq_u8(d, f)));
		uint8x16x2_t pixel;
		pixel.val[0] = vbslq_u8(vandq_u8(active, vorrq_u8(vandq_u8(vceqq_u8(d, b), vmvnq_u8(vceqq_u8(e, vld1q_u8(src2 + x - 1)))), vandq_u8(vceqq_u8(d, h), vmvnq_u8(vceqq_u8(e, vld1q_u8(src0 + x - 1)))))), d, e);
		pixel.val[1] = vbslq_u8(vandq_u8(active, vorrq_u8(vandq_u8(vceqq_u8(f, b), vmvnq_u8(vceqq_u8(e, vld1q_u8(src2 + x + 1)))), vandq_u8(vceqq_u8(f, h), vmvnq_u8(vceqq_u8(e, vld1q_u8(src0 + x + 1)))))), f, e);
		vst2q_u8(dst + 2 * x, pixel);
		x += 16;
	}
#endif
#if defined(CONFIG_NEON32)
	while (count - x > 8) {
		uint8x8_t b = vld1_u8(src0 + x);
		uint8x8_t h = vld1_u8(src2 + x);
		uint8x8_t d = vld1_u8(src1 + x - 1);
		uint8x8_t f = vld1_u8(src1 + x + 1);
		uint8x8_t e = vld1_u8(src1 + x);
		uint8x8_t active = vand_u8(vmvn_u8(vceq_u8(b, h)), vmvn_u8(vceq_u8(d, f)));
		uint8x8x2_t pixel;
		pixel.val[0] = vbsl_u8(vand_u8(active, vorr_u8(vand_u8(vceq_u8(d, b), vmvn_u8(vceq_u8(e, vld1_u8(src2 + x - 1)))), vand_u8(vceq_u8(d, h), vmvn_u8(vceq_u8(e, vld1_u8(src0 + x - 1)))))), d, e);
		pixel.val[1] = vbsl_u8(vand_u8(active, vorr_u8(vand_u8(vceq_u8(f, b), vmvn_u8(vceq_u8(e, vld1_u8(src2 + x + 1)))), vand_u8(vceq_u8(f, h), vmvn_u8(vceq_u8(e, vld1_u8(src0 + x + 1)))))), f, e);
		vst2_u8(dst + 2 * x, pixel);
		x += 8;
	}
#endif
	while (x + 1 < count) {
		if (src0[x] != src2[x] && src1[x - 1] != src1[x + 1]) {
			dst[2 * x] = ((src1[x - 1] == src0[x] && src1[x] != src2[x - 1]) || (src1[x - 1] == src2[x] && src1[x] != src0[x - 1])) ? src1[x - 1] : src1[x];
			dst[2 * x + 1] = ((src1[x + 1] == src0[x] && src1[x] != src2[x + 1]) || (src1[x + 1] == src2[x] && src1[x] != src0[x + 1])) ? src1[x + 1] : src1[x];
		} else {
			dst[2 * x] = dst[2 * x + 1] = src1[x];
		}
		++x;
	}

	/* last pixel: the right neighbor is the pixel itself. */
	dst[2 * x] = (src0[x] != src2[x] && src1[x - 1] != src1[x] && ((src1[x - 1] == src0[x] && src1[x] != src2[x - 1]) || (src1[x - 1] == src2[x] && src1[x] != src0[x - 1]))) ? src1[x - 1] : src1[x];
	dst[2 * x + 1] = src1[x];
}

static inline void scale2x_8_neon_border(scale2x_uint8* dst, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && src1[1] == src0[0]) ? src1[1] : src1[0];

#if defined(CONFIG_NEON)
	while (count - x > 16) {
		uint8x16_t b = vld1q_u8(src0 + x);
		uint8x16_t h = vld1q_u8(src2 + x);
		uint8x16_t d = vld1q_u8(src1 + x - 1);
		uint8x16_t f = vld1q_u8(src1 + x + 1);
		uint8x16_t e = vld1q_u8(src1 + x);
		uint8x16_t active = vandq_u8(vmvnq_u8(vceqq_u8(b, h)), vmvnq_u8(vceqq_u8(d, f)));
		uint8x16x2_t pixel;
		pixel.val[0] = vbslq_u8(vandq_u8(active, vceqq_u8(d, b)), d, e);
		pixel.val[1] = vbslq_u8(vandq_u8(active, vceqq_u8(f, b)), f, e);
		vst2q_u8(dst + 2 * x, pixel);
		x += 16;
	}
#endif
#if defined(CONFIG_NEON32)
	while (count - x > 8) {
		uint8x8_t b = vld1_u8(src0 + x);
		uint8x8_t h = vld1_u8(src2 + x);
		uint8x8_t d = vld1_u8(src1 + x - 1);
		uint8x8_t f = vld1_u8(src1 + x + 1);
		uint8x8_t e = vld1_u8(src1 + x);
		uint8x8_t active = vand_u8(vmvn_u8(vceq_u8(b, h)), vmvn_u8(vceq_u8(d, f)));
		uint8x8x2_t pixel;
		pixel.val[0] = vbsl_u8(vand_u8(active, vceq_u8(d, b)), d, e);
		pixel.val[1] = vbsl_u8(vand_u8(active, vceq_u8(f, b)), f, e);
		vst2_u8(dst + 2 * x, pixel);
		x += 8;
	}
#endif
	while (x + 1 < count) {
		if (src0[x] != src2[x] && src1[x - 1] != src1[x + 1]) {
			dst[2 * x] = src1[x - 1] == src0[x] ? src1[x - 1] : src1[x];
			dst[2 * x + 1] = src1[x + 1] == src0[x] ? src1[x + 1] : src1[x];
		} else {
			dst[2 * x] = dst[2 * x + 1] = src1[x];
		}
		++x;
	}

	/* last pixel: the right neighbor is the pixel itself. */
	dst[2 * x] = (src0[x] != src2[x] && src1[x - 1] != src1[x] && src1[x - 1] == src0[x]) ? src1[x - 1] : src1[x];
	dst[2 * x + 1] = src1[x];
}

static inline void scale2x_16_neon_center(scale2x_uint16* dst, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && ((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]))) ? src1[1] : src1[0];

#if defined(CONFIG_NEON)
	while (count - x > 8) {
		uint16x8_t b = vld1q_u16(src0 + x);
		uint16x8_t h = vld1q_u16(src2 + x);
		uint16x8_t d = vld1q_u16(src1 + x - 1);
		uint16x8_t f = vld1q_u16(src1 + x + 1);
		uint16x8_t e = vld1q_u16(src1 + x);
		uint16x8_t active = vandq_u16(vmvnq_u16(vceqq_u16(b, h)), vmvnq_u16(vceqq_u16(d, f)));
		uint16x8x2_t pixel;
		pixel.val[0] = vbslq_u16(vandq_u16(active, vorrq_u16(vandq_u16(vceqq_u16(d, b), vmvnq_u16(vceqq_u16(e, vld1q_u16(src2 + x - 1)))), vandq_u16(vceqq_u16(d, h), vmvnq_u16(vceqq_u16(e, vld1q_u16(src0 + x - 1)))))), d, e);
		pixel.val[1] = vbslq_u16(vandq_u16(active, vorrq_u16(vandq_u16(vceqq_u16(f, b), vmvnq_u16(vceqq_u16(e, vld1q_u16(src2 + x + 1)))), vandq_u16(vceqq_u16(f, h), vmvnq_u16(vceqq_u16(e, vld1q_u16(src0 + x + 1)))))), f, e);
		vst2q_u16(dst + 2 * x, pixel);
		x += 8;
	}
#endif
#if defined(CONFIG_NEON32)
	while (count - x > 4) {
		uint16x4_t b = vld1_u16(src0 + x);
		uint16x4_t h = vld1_u16(src2 + x);
		uint16x4_t d = vld1_u16(src1 + x - 1);
		uint16x4_t f = vld1_u16(src1 + x + 1);
		uint16x4_t e = vld1_u16(src1 + x);
		uint16x4_t active = vand_u16(vmvn_u16(vceq_u16(b, h)), vmvn_u16(vceq_u16(d, f)));
		uint16x4x2_t pixel;
		pixel.val[0] = vbsl_u16(vand_u16(active, vorr_u16(vand_u16(vceq_u16(d, b), vmvn_u16(vceq_u16(e, vld1_u16(src2 + x - 1)))), vand_u16(vceq_u16(d, h), vmvn_u16(vceq_u16(e, vld1_u16(src0 + x - 1)))))), d, e);
		pixel.val[1] = vbsl_u16(vand_u16(active, vorr_u16(vand_u16(vceq_u16(f, b), vmvn_u16(vceq_u16(e, vld1_u16(src2 + x + 1)))), vand_u16(vceq_u16(f, h), vmvn_u16(vceq_u16(e, vld1_u16(src0 + x + 1)))))), f, e);
		vst2_u16(dst + 2 * x, pixel);
		x += 4;
	}
#endif
	while (x + 1 < count) {
		if (src0[x] != src2[x] && src1[x - 1] != src1[x + 1]) {
			dst[2 * x] = ((src1[x - 1] == src0[x] && src1[x] != src2[x - 1]) || (src1[x - 1] == src2[x] && src1[x] != src0[x - 1])) ? src1[x - 1] : src1[x];
			dst[2 * x + 1] = ((src1[x + 1] == src0[x] && src1[x] != src2[x + 1]) || (src1[x + 1] == src2[x] && src1[x] != src0[x + 1])) ? src1[x + 1] : src1[x];
		} else {
			dst[2 * x] = dst[2 * x + 1] = src1[x];
		}
		++x;
	}

	/* last pixel: the right neighbor is the pixel itself. */
	dst[2 * x] = (src0[x] != src2[x] && src1[x - 1] != src1[x] && ((src1[x - 1] == src0[x] && src1[x] != src2[x - 1]) || (src1[x - 1] == src2[x] && src1[x] != src0[x - 1]))) ? src1[x - 1] : src1[x];
	dst[2 * x + 1] = src1[x];
}

static inline void scale2x_16_neon_border(scale2x_uint16* dst, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && src1[1] == src0[0]) ? src1[1] : src1[0];

#if defined(CONFIG_NEON)
	while (count - x > 8) {
		uint16x8_t b = vld1q_u16(src0 + x);
		uint16x8_t h = vld1q_u16(src2 + x);
		uint16x8_t d = vld1q_u16(src1 + x - 1);
		uint16x8_t f = vld1q_u16(src1 + x + 1);
		uint16x8_t e = vld1q_u16(src1 + x);
		uint16x8_t active = vandq_u16(vmvnq_u16(vceqq_u16(b, h)), vmvnq_u16(vceqq_u16(d, f)));
		uint16x8x2_t pixel;
		pixel.val[0] = vbslq_u16(vandq_u16(active, vceqq_u16(d, b)), d, e);
		pixel.val[1] = vbslq_u16(vandq_u16(active, vceqq_u16(f, b)), f, e);
		vst2q_u16(dst + 2 * x, pixel);
		x += 8;
	}
#endif
#if defined(CONFIG_NEON32)
	while (count - x > 4) {
		uint16x4_t b = vld1_u16(src0 + x);
		uint16x4_t h = vld1_u16(src2 + x);
		uint16x4_t d = vld1_u16(src1 + x - 1);
		uint16x4_t f = vld1_u16(src1 + x + 1);
		uint16x4_t e = vld1_u16(src1 + x);
		uint16x4_t active = vand_u16(vmvn_u16(vceq_u16(b, h)), vmvn_u16(vceq_u16(d, f)));
		uint16x4x2_t pixel;
		pixel.val[0] = vbsl_u16(vand_u16(active, vceq_u16(d, b)), d, e);
		pixel.val[1] = vbsl_u16(vand_u16(active, vceq_u16(f, b)), f, e);
		vst2_u16(dst + 2 * x, pixel);
		x += 4;
	}
#endif
	while (x + 1 < count) {
		if (src0[x] != src2[x] && src1[x - 1] != src1[x + 1]) {
			dst[2 * x] = src1[x - 1] == src0[x] ? src1[x - 1] : src1[x];
			dst[2 * x + 1] = src1[x + 1] == src0[x] ? src1[x + 1] : src1[x];
		} else {
			dst[2 * x] = dst[2 * x + 1] = src1[x];
		}
		++x;
	}

	/* last pixel: the right neighbor is the pixel itself. */
	dst[2 * x] = (src0[x] != src2[x] && src1[x - 1] != src1[x] && src1[x - 1] == src0[x]) ? src1[x - 1] : src1[x];
	dst[2 * x + 1] = src1[x];
}

static inline void scale2x_32_neon_center(scale2x_uint32* dst, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && ((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]))) ? src1[1] : src1[0];

#if defined(CONFIG_NEON)
	while (count - x > 4) {
		uint32x4_t b = vld1q_u32(src0 + x);
		uint32x4_t h = vld1q_u32(src2 + x);
		uint32x4_t d = vld1q_u32(src1 + x - 1);
		uint32x4_t f = vld1q_u32(src1 + x + 1);
		uint32x4_t e = vld1q_u32(src1 + x);
		uint32x4_t active = vandq_u32(vmvnq_u32(vceqq_u32(b, h)), vmvnq_u32(vceqq_u32(d, f)));
		uint32x4x2_t pixel;
		pixel.val[0] = vbslq_u32(vandq_u32(active, vorrq_u32(vandq_u32(vceqq_u32(d, b), vmvnq_u32(vceqq_u32(e, vld1q_u32(src2 + x - 1)))), vandq_u32(vceqq_u32(d, h), vmvnq_u32(vceqq_u32(e, vld1q_u32(src0 + x - 1)))))), d, e);
		pixel.val[1] = vbslq_u32(vandq_u32(active, vorrq_u32(vandq_u32(vceqq_u32(f, b), vmvnq_u32(vceqq_u32(e, vld1q_u32(src2 + x + 1)))), vandq_u32(vceqq_u32(f, h), vmvnq_u32(vceqq_u32(e, vld1q_u32(src0 + x + 1)))))), f, e);
		vst2q_u32(dst + 2 * x, pixel);
		x += 4;
	}
#endif
#if defined(CONFIG_NEON32)
	while (count - x > 2) {
		uint32x2_t b = vld1_u32(src0 + x);
		uint32x2_t h = vld1_u32(src2 + x);
		uint32x2_t d = vld1_u32(src1 + x - 1);
		uint32x2_t f = vld1_u32(src1 + x + 1);
		uint32x2_t e = vld1_u32(src1 + x);
		uint32x2_t active = vand_u32(vmvn_u32(vceq_u32(b, h)), vmvn_u32(vceq_u32(d, f)));
		uint32x2x2_t pixel;
		pixel.val[0] = vbsl_u32(vand_u32(active, vorr_u32(vand_u32(vceq_u32(d, b), vmvn_u32(vceq_u32(e, vld1_u32(src2 + x - 1)))), vand_u32(vceq_u32(d, h), vmvn_u32(vceq_u32(e, vld1_u32(src0 + x - 1)))))), d, e);
		pixel.val[1] = vbsl_u32(vand_u32(active, vorr_u32(vand_u32(vceq_u32(f, b), vmvn_u32(vceq_u32(e, vld1_u32(src2 + x + 1)))), vand_u32(vceq_u32(f, h), vmvn_u32(vceq_u32(e, vld1_u32(src0 + x + 1)))))), f, e);
		vst2_u32(dst + 2 * x, pixel);
		x += 2;
	}
#endif
	while (x + 1 < count) {
		if (src0[x] != src2[x] && src1[x - 1] != src1[x + 1]) {
			dst[2 * x] = ((src1[x - 1] == src0[x] && src1[x] != src2[x - 1]) || (src1[x - 1] == src2[x] && src1[x] != src0[x - 1])) ? src1[x - 1] : src1[x];
			dst[2 * x + 1] = ((src1[x + 1] == src0[x] && src1[x] != src2[x + 1]) || (src1[x + 1] == src2[x] && src1[x] != src0[x + 1])) ? src1[x + 1] : src1[x];
		} else {
			dst[2 * x] = dst[2 * x + 1] = src1[x];
		}
		++x;
	}

	/* last pixel: the right neighbor is the pixel itself. */
	dst[2 * x] = (src0[x] != src2[x] && src1[x - 1] != src1[x] && ((src1[x - 1] == src0[x] && src1[x] != src2[x - 1]) || (src1[x - 1] == src2[x] && src1[x] != src0[x - 1]))) ? src1[x - 1] : src1[x];
	dst[2 * x + 1] = src1[x];
}

static inline void scale2x_32_neon_border(scale2x_uint32* dst, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && src1[1] == src0[0]) ? src1[1] : src1[0];

#if defined(CONFIG_NEON)
	while (count - x > 4) {
		uint32x4_t b = vld1q_u32(src0 + x);
		uint32x4_t h = vld1q_u32(src2 + x);
		uint32x4_t d = vld1q_u32(src1 + x - 1);
		uint32x4_t f = vld1q_u32(src1 + x + 1);
		uint32x4_t e = vld1q_u32(src1 + x);
		uint32x4_t active = vandq_u32(vmvnq_u32(vceqq_u32(b, h)), vmvnq_u32(vceqq_u32(d, f)));
		uint32x4x2_t pixel;
		pixel.val[0] = vbslq_u32(vandq_u32(active, vceqq_u32(d, b)), d, e);
		pixel.val[1] = vbslq_u32(vandq_u32(active, vceqq_u32(f, b)), f, e);
		vst2q_u32(dst + 2 * x, pixel);
		x += 4;
	}
#endif
#if defined(CONFIG_NEON32)
	while (count - x > 2) {
		uint32x2_t b = vld1_u32(src0 + x);
		uint32x2_t h = vld1_u32(src2 + x);
		uint32x2_t d = vld1_u32(src1 + x - 1);
		uint32x2_t f = vld1_u32(src1 + x + 1);
		uint32x2_t e = vld1_u32(src1 + x);
		uint32x2_t active = vand_u32(vmvn_u32(vceq_u32(b, h)), vmvn_u32(vceq_u32(d, f)));
		uint32x2x2_t pixel;
		pixel.val[0] = vbsl_u32(vand_u32(active, vceq_u32(d, b)), d, e);
		pixel.val[1] = vbsl_u32(vand_u32(active, vceq_u32(f, b)), f, e);
		vst2_u32(dst + 2 * x, pixel);
		x += 2;
	}
#endif
	while (x + 1 < count) {
		if (src0[x] != src2[x] && src1[x - 1] != src1[x + 1]) {
			dst[2 * x] = src1[x - 1] == src0[x] ? src1[x - 1] : src1[x];
			dst[2 * x + 1] = src1[x + 1] == src0[x] ? src1[x + 1] : src1[x];
		} else {
			dst[2 * x] = dst[2 * x + 1] = src1[x];
		}
		++x;
	}

	/* last pixel: the right neighbor is the pixel itself. */
	dst[2 * x] = (src0[x] != src2[x] && src1[x - 1] != src1[x] && src1[x - 1] == src0[x]) ? src1[x - 1] : src1[x];
	dst[2 * x + 1] = src1[x];
}

/**
 * Scale by a factor of 2 a row of pixels of 8 bits.
 * This is a very fast SSE2 implementation.
 * The implementation uses a combination of cmp/and/not operations to
 * completly remove the need of conditional jumps. This trick give the
 * major speed improvement.
 * Also, using the 16 bytes SSE2 registers more than one pixel are computed
 * at the same time.
 * Before calling this function you must ensure that the currenct CPU supports
 * the SSE2 instruction set.
 * The pixels over the left and right borders are assumed of the same color of
 * the pixels on the border.
 * Note that the implementation is optimized to write data sequentially to
 * maximize the bandwidth on video memory.
 * \param src0 Pointer at the first pixel of the previous row.
 * \param src1 Pointer at the first pixel of the current row.
 * \param src2 Pointer at the first pixel of the next row.
 * \param count Length in pixels of the src0, src1 and src2 rows. It must
 * be at least 16 and a multiple of 8.
 * \param dst0 First destination row, double length in pixels.
 * \param dst1 Second destination row, double length in pixels.
 */
void scale2x_8_neon(scale2x_uint8* dst0, scale2x_uint8* dst1, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	scale2x_8_neon_border(dst0, src0, src1, src2, count);
	scale2x_8_neon_border(dst1, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2 a row of pixels of 16 bits.
 * This function operates like scale2x_8_neon() but for 16 bits pixels.
 * \param src0 Pointer at the first pixel of the previous row.
 * \param src1 Pointer at the first pixel of the current row.
 * \param src2 Pointer at the first pixel of the next row.
 * \param count Length in pixels of the src0, src1 and src2 rows. It must
 * be at least 8 and a multiple of 4.
 * \param dst0 First destination row, double length in pixels.
 * \param dst1 Second destination row, double length in pixels.
 */
void scale2x_16_neon(scale2x_uint16* dst0, scale2x_uint16* dst1, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	scale2x_16_neon_border(dst0, src0, src1, src2, count);
	scale2x_16_neon_border(dst1, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2 a row of pixels of 32 bits.
 * This function operates like scale2x_8_neon() but for 32 bits pixels.
 * \param src0 Pointer at the first pixel of the previous row.
 * \param src1 Pointer at the first pixel of the current row.
 * \param src2 Pointer at the first pixel of the next row.
 * \param count Length in pixels of the src0, src1 and src2 rows. It must
 * be at least 4 and a multiple of 2.
 * \param dst0 First destination row, double length in pixels.
 * \param dst1 Second destination row, double length in pixels.
 */
void scale2x_32_neon(scale2x_uint32* dst0, scale2x_uint32* dst1, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	scale2x_32_neon_border(dst0, src0, src1, src2, count);
	scale2x_32_neon_border(dst1, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x3 a row of pixels of 8 bits.
 * This function operates like scale2x_8_neon() but with an expansion
 * factor of 2x3 instead of 2x2.
 */
void scale2x3_8_neon(scale2x_uint8* dst0, scale2x_uint8* dst1, scale2x_uint8* dst2, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	scale2x_8_neon_border(dst0, src0, src1, src2, count);
	scale2x_8_neon_center(dst1, src0, src1, src2, count);
	scale2x_8_neon_border(dst2, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x3 a row of pixels of 16 bits.
 * This function operates like scale2x_16_neon() but with an expansion
 * factor of 2x3 instead of 2x2.
 */
void scale2x3_16_neon(scale2x_uint16* dst0, scale2x_uint16* dst1, scale2x_uint16* dst2, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	scale2x_16_neon_border(dst0, src0, src1, src2, count);
	scale2x_16_neon_center(dst1, src0, src1, src2, count);
	scale2x_16_neon_border(dst2, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x3 a row of pixels of 32 bits.
 * This function operates like scale2x_32_neon() but with an expansion
 * factor of 2x3 instead of 2x2.
 */
void scale2x3_32_neon(scale2x_uint32* dst0, scale2x_uint32* dst1, scale2x_uint32* dst2, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	scale2x_32_neon_border(dst0, src0, src1, src2, count);
	scale2x_32_neon_center(dst1, src0, src1, src2, count);
	scale2x_32_neon_border(dst2, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x4 a row of pixels of 8 bits.
 * This function operates like scale2x_8_neon() but with an expansion
 * factor of 2x4 instead of 2x2.
 */
void scale2x4_8_neon(scale2x_uint8* dst0, scale2x_uint8* dst1, scale2x_uint8* dst2, scale2x_uint8* dst3, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	scale2x_8_neon_border(dst0, src0, src1, src2, count);
	scale2x_8_neon_center(dst1, src0, src1, src2, count);
	scale2x_8_neon_center(dst2, src0, src1, src2, count);
	scale2x_8_neon_border(dst3, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x4 a row of pixels of 16 bits.
 * This function operates like scale2x_16_neon() but with an expansion
 * factor of 2x4 instead of 2x2.
 */
void scale2x4_16_neon(scale2x_uint16* dst0, scale2x_uint16* dst1, scale2x_uint16* dst2, scale2x_uint16* dst3, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	scale2x_16_neon_border(dst0, src0, src1, src2, count);
	scale2x_16_neon_center(dst1, src0, src1, src2, count);
	scale2x_16_neon_center(dst2, src0, src1, src2, count);
	scale2x_16_neon_border(dst3, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x4 a row of pixels of 32 bits.
 * This function operates like scale2x_32_neon() but with an expansion
 * factor of 2x4 instead of 2x2.
 */
void scale2x4_32_neon(scale2x_uint32* dst0, scale2x_uint32* dst1, scale2x_uint32* dst2, scale2x_uint32* dst3, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	scale2x_32_neon_border(dst0, src0, src1, src2, count);
	scale2x_32_neon_center(dst1, src0, src1, src2, count);
	scale2x_32_neon_center(dst2, src0, src1, src2, count);
	scale2x_32_neon_border(dst3, src2, src1, src0, count);
}

#endif

