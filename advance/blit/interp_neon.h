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

#ifndef __INTERP_NEON_H
#define __INTERP_NEON_H

#include "interp.h"

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>

static inline int interp_16_any_neon(uint16x8_t mask)
{
#if defined(CONFIG_NEON)
	return vmaxvq_u16(mask) != 0;
#else
	uint16x4_t p = vorr_u16(vget_low_u16(mask), vget_high_u16(mask));
	return (vget_lane_u16(p, 0) | vget_lane_u16(p, 1) | vget_lane_u16(p, 2) | vget_lane_u16(p, 3)) != 0;
#endif
}

static inline int interp_32_any_neon(uint32x4_t mask)
{
#if defined(CONFIG_NEON)
	return vmaxvq_u32(mask) != 0;
#else
	uint32x2_t p = vorr_u32(vget_low_u32(mask), vget_high_u32(mask));
	return (vget_lane_u32(p, 0) | vget_lane_u32(p, 1)) != 0;
#endif
}

static inline void interp_16_store2_neon(interp_uint16* dst, uint16x8_t a, uint16x8_t b)
{
#if defined(CONFIG_NEON)
	vst1q_u16(dst, vzip1q_u16(a, b));
	vst1q_u16(dst + 8, vzip2q_u16(a, b));
#else
	uint16x8x2_t pair;
	pair.val[0] = a;
	pair.val[1] = b;
	vst2q_u16(dst, pair);
#endif
}

static inline void interp_32_store2_neon(interp_uint32* dst, uint32x4_t a, uint32x4_t b)
{
#if defined(CONFIG_NEON)
	vst1q_u32(dst, vzip1q_u32(a, b));
	vst1q_u32(dst + 4, vzip2q_u32(a, b));
#else
	uint32x4x2_t pair;
	pair.val[0] = a;
	pair.val[1] = b;
	vst2q_u32(dst, pair);
#endif
}

static inline uint16x8_t interp_16_565_dist_neon(uint16x8_t a, uint16x8_t b)
{
	const uint16x8_t m31 = vdupq_n_u16(31);
	const uint16x8_t mg = vdupq_n_u16(63);
	uint16x8_t r = vabdq_u16(vshrq_n_u16(a, 11), vshrq_n_u16(b, 11));
	uint16x8_t g = vabdq_u16(vandq_u16(vshrq_n_u16(a, 5), mg), vandq_u16(vshrq_n_u16(b, 5), mg));
	uint16x8_t bl = vabdq_u16(vandq_u16(a, m31), vandq_u16(b, m31));
	uint16x8_t rg = vaddq_u16(r, g);
	return vaddq_u16(vaddq_u16(vshlq_n_u16(rg, 1), rg), bl);
}

static inline uint16x8_t interp_16_565_dist3_neon(uint16x8_t a, uint16x8_t b, uint16x8_t c)
{
	return vaddq_u16(interp_16_565_dist_neon(a, b), interp_16_565_dist_neon(b, c));
}

static inline uint16x8_t interp_16_565_11_neon(uint16x8_t a, uint16x8_t b)
{
	return vaddq_u16(vandq_u16(vshrq_n_u16(veorq_u16(a, b), 1), vdupq_n_u16(0x7BEF)), vandq_u16(a, b));
}

static inline uint16x8_t interp_16_565_31_neon(uint16x8_t a, uint16x8_t b)
{
	return interp_16_565_11_neon(a, interp_16_565_11_neon(a, b));
}

static inline uint16x8_t interp_16_565_71_neon(uint16x8_t a, uint16x8_t b)
{
	const uint16x8_t m31 = vdupq_n_u16(31);
	const uint16x8_t mg = vdupq_n_u16(63);
	uint16x8_t ra = vshrq_n_u16(a, 11);
	uint16x8_t rb = vshrq_n_u16(b, 11);
	uint16x8_t ga = vandq_u16(vshrq_n_u16(a, 5), mg);
	uint16x8_t gb = vandq_u16(vshrq_n_u16(b, 5), mg);
	uint16x8_t ba = vandq_u16(a, m31);
	uint16x8_t bb = vandq_u16(b, m31);
	uint16x8_t r = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ra, 7), rb), 3);
	uint16x8_t g = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ga, 7), gb), 3);
	uint16x8_t bl = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ba, 7), bb), 3);
	return vorrq_u16(vorrq_u16(vshlq_n_u16(r, 11), vshlq_n_u16(g, 5)), bl);
}

static inline uint16x8_t interp_16_555_dist_neon(uint16x8_t a, uint16x8_t b)
{
	const uint16x8_t m31 = vdupq_n_u16(31);
	const uint16x8_t mg = vdupq_n_u16(31);
	uint16x8_t r = vabdq_u16(vandq_u16(vshrq_n_u16(a, 10), m31), vandq_u16(vshrq_n_u16(b, 10), m31));
	uint16x8_t g = vabdq_u16(vandq_u16(vshrq_n_u16(a, 5), mg), vandq_u16(vshrq_n_u16(b, 5), mg));
	uint16x8_t bl = vabdq_u16(vandq_u16(a, m31), vandq_u16(b, m31));
	return vaddq_u16(vaddq_u16(vaddq_u16(vshlq_n_u16(r, 1), r), vaddq_u16(vshlq_n_u16(g, 2), vshlq_n_u16(g, 1))), bl);
}

static inline uint16x8_t interp_16_555_dist3_neon(uint16x8_t a, uint16x8_t b, uint16x8_t c)
{
	return vaddq_u16(interp_16_555_dist_neon(a, b), interp_16_555_dist_neon(b, c));
}

static inline uint16x8_t interp_16_555_11_neon(uint16x8_t a, uint16x8_t b)
{
	return vaddq_u16(vandq_u16(vshrq_n_u16(veorq_u16(a, b), 1), vdupq_n_u16(0x3DEF)), vandq_u16(a, b));
}

static inline uint16x8_t interp_16_555_31_neon(uint16x8_t a, uint16x8_t b)
{
	return interp_16_555_11_neon(a, interp_16_555_11_neon(a, b));
}

static inline uint16x8_t interp_16_555_71_neon(uint16x8_t a, uint16x8_t b)
{
	const uint16x8_t m31 = vdupq_n_u16(31);
	const uint16x8_t mg = vdupq_n_u16(31);
	uint16x8_t ra = vandq_u16(vshrq_n_u16(a, 10), m31);
	uint16x8_t rb = vandq_u16(vshrq_n_u16(b, 10), m31);
	uint16x8_t ga = vandq_u16(vshrq_n_u16(a, 5), mg);
	uint16x8_t gb = vandq_u16(vshrq_n_u16(b, 5), mg);
	uint16x8_t ba = vandq_u16(a, m31);
	uint16x8_t bb = vandq_u16(b, m31);
	uint16x8_t r = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ra, 7), rb), 3);
	uint16x8_t g = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ga, 7), gb), 3);
	uint16x8_t bl = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ba, 7), bb), 3);
	return vorrq_u16(vorrq_u16(vshlq_n_u16(r, 10), vshlq_n_u16(g, 5)), bl);
}

static inline uint32x4_t interp_32_dist_neon(uint32x4_t a, uint32x4_t b)
{
	const uint8x16_t weights = vreinterpretq_u8_u32(vdupq_n_u32(0x00030601U));
	uint8x16_t d = vabdq_u8(vreinterpretq_u8_u32(a), vreinterpretq_u8_u32(b));
	uint16x8_t lo = vmull_u8(vget_low_u8(d), vget_low_u8(weights));
	uint16x8_t hi = vmull_u8(vget_high_u8(d), vget_high_u8(weights));
	uint16x4_t s0 = vpadd_u16(vget_low_u16(lo), vget_high_u16(lo));
	uint16x4_t s1 = vpadd_u16(vget_low_u16(hi), vget_high_u16(hi));
	return vcombine_u32(vpaddl_u16(s0), vpaddl_u16(s1));
}

static inline uint32x4_t interp_32_dist3_neon(uint32x4_t a, uint32x4_t b, uint32x4_t c)
{
	return vaddq_u32(interp_32_dist_neon(a, b), interp_32_dist_neon(b, c));
}

/* Common edge scores for XBR2, XBR3 and XBR4. */
static inline uint16x8_t interp_16_565_score_neon(uint16x8_t a, uint16x8_t b, uint16x8_t c, uint16x8_t d, uint16x8_t e, uint16x8_t f, uint16x8_t g, uint16x8_t h)
{
	uint16x8_t x = vaddq_u16(interp_16_565_dist3_neon(a, b, c), interp_16_565_dist3_neon(d, e, f));
	uint16x8_t y = vshlq_n_u16(interp_16_565_dist_neon(g, h), 2);
	return vaddq_u16(x, y);
}

static inline uint16x8_t interp_16_555_score_neon(uint16x8_t a, uint16x8_t b, uint16x8_t c, uint16x8_t d, uint16x8_t e, uint16x8_t f, uint16x8_t g, uint16x8_t h)
{
	uint16x8_t x = vaddq_u16(interp_16_555_dist3_neon(a, b, c), interp_16_555_dist3_neon(d, e, f));
	uint16x8_t y = vshlq_n_u16(interp_16_555_dist_neon(g, h), 2);
	return vaddq_u16(x, y);
}

/*
 * RGB32: dist(a,b) + dist(b,c) + dist(d,e) + dist(e,f) + 4*dist(g,h).
 * Bytes are B, G, R, ignored alpha; weights are 1, 6, 3, 0.
 * Accumulate all five byte-wise weighted distances in 16-bit lanes and
 * reduce once at the end. The maximum partial lane is 255*6*8=12240,
 * safely within unsigned and signed 16-bit range.
 * Works with ARMv7 NEON and AArch64 NEON.
 */
static inline uint32x4_t interp_32_score_neon(uint32x4_t a, uint32x4_t b, uint32x4_t c, uint32x4_t d, uint32x4_t e, uint32x4_t f, uint32x4_t g, uint32x4_t h)
{
	const uint8x8_t w = vreinterpret_u8_u32(vdup_n_u32(0x00030601U));
	const uint8x8_t w4 = vreinterpret_u8_u32(vdup_n_u32(0x000C1804U));
	uint8x16_t x0 = vabdq_u8(vreinterpretq_u8_u32(a), vreinterpretq_u8_u32(b));
	uint8x16_t x1 = vabdq_u8(vreinterpretq_u8_u32(b), vreinterpretq_u8_u32(c));
	uint8x16_t x2 = vabdq_u8(vreinterpretq_u8_u32(d), vreinterpretq_u8_u32(e));
	uint8x16_t x3 = vabdq_u8(vreinterpretq_u8_u32(e), vreinterpretq_u8_u32(f));
	uint8x16_t x4 = vabdq_u8(vreinterpretq_u8_u32(g), vreinterpretq_u8_u32(h));

	uint16x8_t lo = vmull_u8(vget_low_u8(x0), w);
	lo = vmlal_u8(lo, vget_low_u8(x1), w);
	lo = vmlal_u8(lo, vget_low_u8(x2), w);
	lo = vmlal_u8(lo, vget_low_u8(x3), w);
	lo = vmlal_u8(lo, vget_low_u8(x4), w4);

	uint16x8_t hi = vmull_u8(vget_high_u8(x0), w);
	hi = vmlal_u8(hi, vget_high_u8(x1), w);
	hi = vmlal_u8(hi, vget_high_u8(x2), w);
	hi = vmlal_u8(hi, vget_high_u8(x3), w);
	hi = vmlal_u8(hi, vget_high_u8(x4), w4);

	uint16x4_t p0 = vpadd_u16(vget_low_u16(lo), vget_high_u16(lo));
	uint16x4_t p1 = vpadd_u16(vget_low_u16(hi), vget_high_u16(hi));
	return vcombine_u32(vpaddl_u16(p0), vpaddl_u16(p1));
}

static inline uint32x4_t interp_32_11_neon(uint32x4_t a, uint32x4_t b)
{
	return vreinterpretq_u32_u8(vhaddq_u8(vreinterpretq_u8_u32(a), vreinterpretq_u8_u32(b)));
}

static inline uint32x4_t interp_32_31_neon(uint32x4_t a, uint32x4_t b)
{
	return interp_32_11_neon(a, interp_32_11_neon(a, b));
}

static inline uint32x4_t interp_32_71_neon(uint32x4_t a, uint32x4_t b)
{
	uint8x16_t x = vreinterpretq_u8_u32(a);
	uint8x16_t y = vreinterpretq_u8_u32(b);

	return vreinterpretq_u32_u8(vhaddq_u8(x, vhaddq_u8(x, vhaddq_u8(x, y))));
}

static inline void interp_16_store3_neon(interp_uint16* dst, uint16x8_t a, uint16x8_t b, uint16x8_t c)
{
	uint16x8x3_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	vst3q_u16(dst, pixels);
}

static inline void interp_32_store3_neon(interp_uint32* dst, uint32x4_t a, uint32x4_t b, uint32x4_t c)
{
	uint32x4x3_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	vst3q_u32(dst, pixels);
}

static inline void interp_16_store4_neon(interp_uint16* dst, uint16x8_t a, uint16x8_t b, uint16x8_t c, uint16x8_t d)
{
	uint16x8x4_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	pixels.val[3] = d;
	vst4q_u16(dst, pixels);
}

static inline void interp_32_store4_neon(interp_uint32* dst, uint32x4_t a, uint32x4_t b, uint32x4_t c, uint32x4_t d)
{
	uint32x4x4_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	pixels.val[3] = d;
	vst4q_u32(dst, pixels);
}

static inline int interp_16_64_any_neon(uint16x4_t mask)
{
	return (vget_lane_u16(mask, 0) | vget_lane_u16(mask, 1) | vget_lane_u16(mask, 2) | vget_lane_u16(mask, 3)) != 0;
}

static inline int interp_32_64_any_neon(uint32x2_t mask)
{
	return (vget_lane_u32(mask, 0) | vget_lane_u32(mask, 1)) != 0;
}

static inline void interp_16_64_store2_neon(interp_uint16* dst, uint16x4_t a, uint16x4_t b)
{
	uint16x4x2_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	vst2_u16(dst, pixels);
}

static inline void interp_32_64_store2_neon(interp_uint32* dst, uint32x2_t a, uint32x2_t b)
{
	uint32x2x2_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	vst2_u32(dst, pixels);
}

static inline void interp_16_64_store3_neon(interp_uint16* dst, uint16x4_t a, uint16x4_t b, uint16x4_t c)
{
	uint16x4x3_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	vst3_u16(dst, pixels);
}

static inline void interp_32_64_store3_neon(interp_uint32* dst, uint32x2_t a, uint32x2_t b, uint32x2_t c)
{
	uint32x2x3_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	vst3_u32(dst, pixels);
}

static inline void interp_16_64_store4_neon(interp_uint16* dst, uint16x4_t a, uint16x4_t b, uint16x4_t c, uint16x4_t d)
{
	uint16x4x4_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	pixels.val[3] = d;
	vst4_u16(dst, pixels);
}

static inline void interp_32_64_store4_neon(interp_uint32* dst, uint32x2_t a, uint32x2_t b, uint32x2_t c, uint32x2_t d)
{
	uint32x2x4_t pixels;
	pixels.val[0] = a;
	pixels.val[1] = b;
	pixels.val[2] = c;
	pixels.val[3] = d;
	vst4_u32(dst, pixels);
}

static inline uint16x4_t interp_16_565_64_dist_neon(uint16x4_t a, uint16x4_t b)
{
	const uint16x4_t m31 = vdup_n_u16(31);
	const uint16x4_t mg = vdup_n_u16(63);
	uint16x4_t r = vabd_u16(vshr_n_u16(a, 11), vshr_n_u16(b, 11));
	uint16x4_t g = vabd_u16(vand_u16(vshr_n_u16(a, 5), mg), vand_u16(vshr_n_u16(b, 5), mg));
	uint16x4_t bl = vabd_u16(vand_u16(a, m31), vand_u16(b, m31));
	uint16x4_t rg = vadd_u16(r, g);
	return vadd_u16(vadd_u16(vshl_n_u16(rg, 1), rg), bl);
}

static inline uint16x4_t interp_16_565_64_dist3_neon(uint16x4_t a, uint16x4_t b, uint16x4_t c)
{
	return vadd_u16(interp_16_565_64_dist_neon(a, b), interp_16_565_64_dist_neon(b, c));
}

static inline uint16x4_t interp_16_565_64_11_neon(uint16x4_t a, uint16x4_t b)
{
	return vadd_u16(vand_u16(vshr_n_u16(veor_u16(a, b), 1), vdup_n_u16(0x7BEF)), vand_u16(a, b));
}

static inline uint16x4_t interp_16_565_64_31_neon(uint16x4_t a, uint16x4_t b)
{
	return interp_16_565_64_11_neon(a, interp_16_565_64_11_neon(a, b));
}

static inline uint16x4_t interp_16_565_64_71_neon(uint16x4_t a, uint16x4_t b)
{
	const uint16x4_t m31 = vdup_n_u16(31);
	const uint16x4_t mg = vdup_n_u16(63);
	uint16x4_t ra = vshr_n_u16(a, 11);
	uint16x4_t rb = vshr_n_u16(b, 11);
	uint16x4_t ga = vand_u16(vshr_n_u16(a, 5), mg);
	uint16x4_t gb = vand_u16(vshr_n_u16(b, 5), mg);
	uint16x4_t ba = vand_u16(a, m31);
	uint16x4_t bb = vand_u16(b, m31);
	uint16x4_t r = vshr_n_u16(vadd_u16(vmul_n_u16(ra, 7), rb), 3);
	uint16x4_t g = vshr_n_u16(vadd_u16(vmul_n_u16(ga, 7), gb), 3);
	uint16x4_t bl = vshr_n_u16(vadd_u16(vmul_n_u16(ba, 7), bb), 3);
	return vorr_u16(vorr_u16(vshl_n_u16(r, 11), vshl_n_u16(g, 5)), bl);
}

static inline uint16x4_t interp_16_555_64_dist_neon(uint16x4_t a, uint16x4_t b)
{
	const uint16x4_t m31 = vdup_n_u16(31);
	const uint16x4_t mg = vdup_n_u16(31);
	uint16x4_t r = vabd_u16(vand_u16(vshr_n_u16(a, 10), m31), vand_u16(vshr_n_u16(b, 10), m31));
	uint16x4_t g = vabd_u16(vand_u16(vshr_n_u16(a, 5), mg), vand_u16(vshr_n_u16(b, 5), mg));
	uint16x4_t bl = vabd_u16(vand_u16(a, m31), vand_u16(b, m31));
	return vadd_u16(vadd_u16(vadd_u16(vshl_n_u16(r, 1), r), vadd_u16(vshl_n_u16(g, 2), vshl_n_u16(g, 1))), bl);
}

static inline uint16x4_t interp_16_555_64_dist3_neon(uint16x4_t a, uint16x4_t b, uint16x4_t c)
{
	return vadd_u16(interp_16_555_64_dist_neon(a, b), interp_16_555_64_dist_neon(b, c));
}

static inline uint16x4_t interp_16_555_64_11_neon(uint16x4_t a, uint16x4_t b)
{
	return vadd_u16(vand_u16(vshr_n_u16(veor_u16(a, b), 1), vdup_n_u16(0x3DEF)), vand_u16(a, b));
}

static inline uint16x4_t interp_16_555_64_31_neon(uint16x4_t a, uint16x4_t b)
{
	return interp_16_555_64_11_neon(a, interp_16_555_64_11_neon(a, b));
}

static inline uint16x4_t interp_16_555_64_71_neon(uint16x4_t a, uint16x4_t b)
{
	const uint16x4_t m31 = vdup_n_u16(31);
	const uint16x4_t mg = vdup_n_u16(31);
	uint16x4_t ra = vand_u16(vshr_n_u16(a, 10), m31);
	uint16x4_t rb = vand_u16(vshr_n_u16(b, 10), m31);
	uint16x4_t ga = vand_u16(vshr_n_u16(a, 5), mg);
	uint16x4_t gb = vand_u16(vshr_n_u16(b, 5), mg);
	uint16x4_t ba = vand_u16(a, m31);
	uint16x4_t bb = vand_u16(b, m31);
	uint16x4_t r = vshr_n_u16(vadd_u16(vmul_n_u16(ra, 7), rb), 3);
	uint16x4_t g = vshr_n_u16(vadd_u16(vmul_n_u16(ga, 7), gb), 3);
	uint16x4_t bl = vshr_n_u16(vadd_u16(vmul_n_u16(ba, 7), bb), 3);
	return vorr_u16(vorr_u16(vshl_n_u16(r, 10), vshl_n_u16(g, 5)), bl);
}

static inline uint32x2_t interp_32_64_dist_neon(uint32x2_t a, uint32x2_t b)
{
	const uint8x8_t weights = vreinterpret_u8_u32(vdup_n_u32(0x00030601U));
	uint8x8_t d = vabd_u8(vreinterpret_u8_u32(a), vreinterpret_u8_u32(b));
	uint16x8_t w = vmull_u8(d, weights);
	uint16x4_t s = vpadd_u16(vget_low_u16(w), vget_high_u16(w));
	return vpaddl_u16(s);
}

static inline uint32x2_t interp_32_64_dist3_neon(uint32x2_t a, uint32x2_t b, uint32x2_t c)
{
	return vadd_u32(interp_32_64_dist_neon(a, b), interp_32_64_dist_neon(b, c));
}

/* ARMv7: XBR edge scores for 64-bit input/output vectors. */
static inline uint16x4_t interp_16_565_64_score_neon(uint16x4_t a, uint16x4_t b, uint16x4_t c, uint16x4_t d, uint16x4_t e, uint16x4_t f, uint16x4_t g, uint16x4_t h)
{
	uint16x4_t x = vadd_u16(interp_16_565_64_dist3_neon(a, b, c), interp_16_565_64_dist3_neon(d, e, f));
	return vadd_u16(x, vshl_n_u16(interp_16_565_64_dist_neon(g, h), 2));
}

static inline uint16x4_t interp_16_555_64_score_neon(uint16x4_t a, uint16x4_t b, uint16x4_t c, uint16x4_t d, uint16x4_t e, uint16x4_t f, uint16x4_t g, uint16x4_t h)
{
	uint16x4_t x = vadd_u16(interp_16_555_64_dist3_neon(a, b, c), interp_16_555_64_dist3_neon(d, e, f));
	return vadd_u16(x, vshl_n_u16(interp_16_555_64_dist_neon(g, h), 2));
}

/* Accumulate all five RGB32 distances at 16-bit precision, then reduce. */
static inline uint32x2_t interp_32_64_score_neon(uint32x2_t a, uint32x2_t b, uint32x2_t c, uint32x2_t d, uint32x2_t e, uint32x2_t f, uint32x2_t g, uint32x2_t h)
{
	const uint8x8_t w = vreinterpret_u8_u32(vdup_n_u32(0x00030601U));
	const uint8x8_t w4 = vreinterpret_u8_u32(vdup_n_u32(0x000C1804U));
	uint8x8_t x0 = vabd_u8(vreinterpret_u8_u32(a), vreinterpret_u8_u32(b));
	uint8x8_t x1 = vabd_u8(vreinterpret_u8_u32(b), vreinterpret_u8_u32(c));
	uint8x8_t x2 = vabd_u8(vreinterpret_u8_u32(d), vreinterpret_u8_u32(e));
	uint8x8_t x3 = vabd_u8(vreinterpret_u8_u32(e), vreinterpret_u8_u32(f));
	uint8x8_t x4 = vabd_u8(vreinterpret_u8_u32(g), vreinterpret_u8_u32(h));

	uint16x8_t sum = vmull_u8(x0, w);
	sum = vmlal_u8(sum, x1, w);
	sum = vmlal_u8(sum, x2, w);
	sum = vmlal_u8(sum, x3, w);
	sum = vmlal_u8(sum, x4, w4);

	uint16x4_t pairs = vpadd_u16(vget_low_u16(sum), vget_high_u16(sum));
	return vpaddl_u16(pairs);
}

static inline uint32x2_t interp_32_64_11_neon(uint32x2_t a, uint32x2_t b)
{
	const uint32x2_t mask = vdup_n_u32(0xFF7F7F7FU);
	return vadd_u32(vand_u32(vshr_n_u32(veor_u32(a, b), 1), mask), vand_u32(a, b));
}

static inline uint32x2_t interp_32_64_31_neon(uint32x2_t a, uint32x2_t b)
{
	return interp_32_64_11_neon(a, interp_32_64_11_neon(a, b));
}

static inline uint32x2_t interp_32_64_71_neon(uint32x2_t a, uint32x2_t b)
{
	uint8x8_t x = vreinterpret_u8_u32(a);
	uint8x8_t y = vreinterpret_u8_u32(b);
	uint8x8_t w = vdup_n_u8(7);
	uint16x8_t sum = vaddw_u8(vmull_u8(x, w), y);
	return vreinterpret_u32_u8(vshrn_n_u16(sum, 3));
}

#endif
#endif
