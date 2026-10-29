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

#include "xbr2x.h"

#ifdef CONFIG_NEON

#include <arm_neon.h>

/*
 * Maximum single distance: 2504; maximum e/i: 20032.
 * All the distances and weighted sums fit in unsigned 16-bit lanes.
 */
static inline uint16x8_t xbr565_neon_dist(uint16x8_t a, uint16x8_t b)
{
	const uint16x8_t m31 = vdupq_n_u16(31);
	const uint16x8_t mg = vdupq_n_u16(63);
	uint16x8_t r = vabdq_u16(vshrq_n_u16(a, 11), vshrq_n_u16(b, 11));
	uint16x8_t g = vabdq_u16(vandq_u16(vshrq_n_u16(a, 5), mg), vandq_u16(vshrq_n_u16(b, 5), mg));
	uint16x8_t bl = vabdq_u16(vandq_u16(a, m31), vandq_u16(b, m31));
	return vshlq_n_u16(vaddq_u16(vaddq_u16(vshlq_n_u16(vaddq_u16(r, g), 1), vaddq_u16(r, g)), bl), 3);
}

static inline uint16x8_t xbr565_neon_dist3(uint16x8_t a, uint16x8_t b, uint16x8_t c)
{
	return vaddq_u16(xbr565_neon_dist(a, b), xbr565_neon_dist(b, c));
}

static inline uint16x8_t xbr565_neon_11(uint16x8_t a, uint16x8_t b)
{
	return vaddq_u16(vandq_u16(vshrq_n_u16(veorq_u16(a, b), 1), vdupq_n_u16(0x7BEF)), vandq_u16(a, b));
}

/*
 * The weights and shifts are compile-time constants. Component arithmetic
 * stays in 16-bit lanes, then channels are repacked without saturation.
 */
#define XBR565_NEON_WEIGHT_FUNCTION(name, weight, shift) \
	static inline uint16x8_t name(uint16x8_t a, uint16x8_t b) \
	{ \
		const uint16x8_t m31 = vdupq_n_u16(31); \
		const uint16x8_t mg = vdupq_n_u16(63); \
		uint16x8_t ra = vshrq_n_u16(a, 11); \
		uint16x8_t rb = vshrq_n_u16(b, 11); \
		uint16x8_t ga = vandq_u16(vshrq_n_u16(a, 5), mg); \
		uint16x8_t gb = vandq_u16(vshrq_n_u16(b, 5), mg); \
		uint16x8_t ba = vandq_u16(a, m31); \
		uint16x8_t bb = vandq_u16(b, m31); \
		uint16x8_t r = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ra, weight), rb), shift); \
		uint16x8_t g = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ga, weight), gb), shift); \
		uint16x8_t bl = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ba, weight), bb), shift); \
		return vorrq_u16(vorrq_u16(vshlq_n_u16(r, 11), vshlq_n_u16(g, 5)), bl); \
	}

static inline uint16x8_t xbr565_neon_31(uint16x8_t a, uint16x8_t b)
{
	return xbr565_neon_11(a, xbr565_neon_11(a, b));
}

XBR565_NEON_WEIGHT_FUNCTION(xbr565_neon_71, 7, 3)

#define XBR565_NEON(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		uint16x8_t _gate = vmvnq_u16(vorrq_u16(vceqq_u16((PE), (PH)), vceqq_u16((PE), (PF)))); \
		if (vmaxvq_u16(_gate) != 0) { \
			uint16x8_t _e = vaddq_u16(vaddq_u16(xbr565_neon_dist3((PC), (PE), (PG)), \
					xbr565_neon_dist3((H5), (PI), (F4))), \
					vshlq_n_u16(xbr565_neon_dist((PH), (PF)), 2)); \
			uint16x8_t _i = vaddq_u16(vaddq_u16(xbr565_neon_dist3((PD), (PH), (I5)), \
					xbr565_neon_dist3((I4), (PF), (PB))), \
					vshlq_n_u16(xbr565_neon_dist((PE), (PI)), 2)); \
			uint16x8_t _active = vandq_u16(_gate, vcgtq_u16(_i, _e)); \
			if (vmaxvq_u16(_active) != 0) { \
				uint16x8_t _ke = xbr565_neon_dist((PF), (PG)); \
				uint16x8_t _ki = xbr565_neon_dist((PH), (PC)); \
				uint16x8_t _ex2 = vbicq_u16(_active, vorrq_u16(vceqq_u16((PE), (PC)), vceqq_u16((PB), (PC)))); \
				uint16x8_t _ex3 = vbicq_u16(_active, vorrq_u16(vceqq_u16((PE), (PG)), vceqq_u16((PD), (PG)))); \
				uint16x8_t _leftup = vandq_u16(vandq_u16(_ex2, _ex3), \
						vandq_u16(vceqq_u16(_ke, vdupq_n_u16(0)), vceqq_u16(_ki, vdupq_n_u16(0)))); \
				uint16x8_t _left = vbicq_u16(vandq_u16(_ex3, vcleq_u16(vshlq_n_u16(_ke, 1), _ki)), _leftup); \
				uint16x8_t _up = vbicq_u16(vandq_u16(_ex2, vcgeq_u16(_ke, vshlq_n_u16(_ki, 1))), vorrq_u16(_leftup, _left)); \
				uint16x8_t _dia = vbicq_u16(_active, vorrq_u16(vorrq_u16(_leftup, _left), _up)); \
				uint16x8_t _px = vbslq_u16(vcgtq_u16(xbr565_neon_dist((PE), (PF)), xbr565_neon_dist((PE), (PH))), (PH), (PF)); \
				uint16x8_t _e3 = (N3), _e1 = (N1), _e2 = (N2); \
				uint16x8_t _n3 = vbslq_u16(_leftup, xbr565_neon_71(_px, _e3), _e3); \
				_n3 = vbslq_u16(vorrq_u16(_left, _up), xbr565_neon_31(_px, _e3), _n3); \
				_n3 = vbslq_u16(_dia, xbr565_neon_11(_e3, _px), _n3); \
				uint16x8_t _t2 = xbr565_neon_31(_e2, _px); \
				uint16x8_t _t1 = xbr565_neon_31(_e1, _px); \
				(N3) = _n3; \
				(N2) = vbslq_u16(vorrq_u16(_leftup, _left), _t2, _e2); \
				(N1) = vbslq_u16(_up, _t1, vbslq_u16(_leftup, _t2, _e1)); \
			} \
		} \
} while (0)

/*
 * Scalar border implementation, duplicated here so this kernel does not
 * depend on the global RGB16 masks or overwrite previously written pixels.
 */
static inline int xbr565_dist_s(interp_uint16 a, interp_uint16 b)
{
	int r = (a >> 11) - (b >> 11);
	int g = ((a >> 5) & 63) - ((b >> 5) & 63);
	int bl = (a & 31) - (b & 31);
	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (bl < 0) bl = -bl;
	return (3 * (r + g) + bl) * 8;
}

static inline int xbr565_dist3_s(interp_uint16 a, interp_uint16 b, interp_uint16 c)
{
	return xbr565_dist_s(a, b) + xbr565_dist_s(b, c);
}

static inline interp_uint16 xbr565_11_s(interp_uint16 a, interp_uint16 b)
{
	return (((a ^ b) >> 1) & 0x7BEF) + (a & b);
}

static inline interp_uint16 xbr565_weight_s(interp_uint16 a, interp_uint16 b, unsigned wa, unsigned shift)
{
	unsigned r = (wa * (a >> 11) + (b >> 11)) >> shift;
	unsigned g = (wa * ((a >> 5) & 63) + ((b >> 5) & 63)) >> shift;
	unsigned bl = (wa * (a & 31) + (b & 31)) >> shift;
	return (interp_uint16)((r << 11) | (g << 5) | bl);
}

static inline interp_uint16 xbr565_31_s(interp_uint16 a, interp_uint16 b)
{
	return xbr565_11_s(a, xbr565_11_s(a, b));
}

static inline interp_uint16 xbr565_71_s(interp_uint16 a, interp_uint16 b)
{
	return xbr565_weight_s(a, b, 7, 3);
}

#define XBR565_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned _e = xbr565_dist3_s((PC), (PE), (PG)) + xbr565_dist3_s((H5), (PI), (F4)) + 4 * xbr565_dist_s((PH), (PF)); \
			unsigned _i = xbr565_dist3_s((PD), (PH), (I5)) + xbr565_dist3_s((I4), (PF), (PB)) + 4 * xbr565_dist_s((PE), (PI)); \
			if (_e < _i) { \
				int _ex2 = (PE) != (PC) && (PB) != (PC); \
				int _ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned _ke = xbr565_dist_s((PF), (PG)); \
				unsigned _ki = xbr565_dist_s((PH), (PC)); \
				interp_uint16 _px = xbr565_dist_s((PE), (PF)) <= xbr565_dist_s((PE), (PH)) ? (PF) : (PH); \
				if (_ke == 0 && _ki == 0 && _ex3 && _ex2) { \
					(N3) = xbr565_71_s(_px, (N3)); \
					(N1) = (N2) = xbr565_31_s((N2), _px); \
				} else if (2 * _ke <= _ki && _ex3) { \
					(N3) = xbr565_31_s(_px, (N3)); \
					(N2) = xbr565_31_s((N2), _px); \
				} else if (_ke >= 2 * _ki && _ex2) { \
					(N3) = xbr565_31_s(_px, (N3)); \
					(N1) = xbr565_31_s((N1), _px); \
				} else { \
					(N3) = xbr565_11_s((N3), _px); \
				} \
			} \
		} \
} while (0)

static inline void xbr565_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, const interp_uint16* src3,
	const interp_uint16* src4, unsigned count, unsigned x)
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
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE;
	XBR565_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR565_S(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR565_S(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR565_S(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

void xbr2x_16_565_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	for (; x < count && x < 2; ++x)
		xbr565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* Eight source pixels per SIMD block. The widest source access is x+9. */
	for (; count - x >= 10; x += 8) {
#define LD565(row, off) vld1q_u16((row) + x + (off))
		uint16x8_t A1 = LD565(src0, -1), B1 = LD565(src0, 0), C1 = LD565(src0, 1);
		uint16x8_t A0 = LD565(src1, -2), PA = LD565(src1, -1), PB = LD565(src1, 0), PC = LD565(src1, 1), C4 = LD565(src1, 2);
		uint16x8_t D0 = LD565(src2, -2), PD = LD565(src2, -1), PE = LD565(src2, 0), PF = LD565(src2, 1), F4 = LD565(src2, 2);
		uint16x8_t G0 = LD565(src3, -2), PG = LD565(src3, -1), PH = LD565(src3, 0), PI = LD565(src3, 1), I4 = LD565(src3, 2);
		uint16x8_t G5 = LD565(src4, -1), H5 = LD565(src4, 0), I5 = LD565(src4, 1);
#undef LD565
		uint16x8_t E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBR565_NEON(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR565_NEON(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR565_NEON(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR565_NEON(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

		/* Four 128-bit stores; each resulting destination pixel is written once. */
		vst1q_u16(dst0 + 2 * x, vzip1q_u16(E0, E1));
		vst1q_u16(dst0 + 2 * x + 8, vzip2q_u16(E0, E1));
		vst1q_u16(dst1 + 2 * x, vzip1q_u16(E2, E3));
		vst1q_u16(dst1 + 2 * x + 8, vzip2q_u16(E2, E3));
	}

	for (; x < count; ++x)
		xbr565_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

/*
 * Maximum single distance: 2480; maximum e/i: 19840.
 * All the distances and weighted sums fit in unsigned 16-bit lanes.
 */
static inline uint16x8_t xbr555_neon_dist(uint16x8_t a, uint16x8_t b)
{
	const uint16x8_t m31 = vdupq_n_u16(31);
	const uint16x8_t mg = vdupq_n_u16(31);
	uint16x8_t r = vabdq_u16(vandq_u16(vshrq_n_u16(a, 10), m31), vandq_u16(vshrq_n_u16(b, 10), m31));
	uint16x8_t g = vabdq_u16(vandq_u16(vshrq_n_u16(a, 5), mg), vandq_u16(vshrq_n_u16(b, 5), mg));
	uint16x8_t bl = vabdq_u16(vandq_u16(a, m31), vandq_u16(b, m31));
	return vshlq_n_u16(vaddq_u16(vaddq_u16(vaddq_u16(vshlq_n_u16(r, 1), r), vaddq_u16(vshlq_n_u16(g, 2), vshlq_n_u16(g, 1))), bl), 3);
}

static inline uint16x8_t xbr555_neon_dist3(uint16x8_t a, uint16x8_t b, uint16x8_t c)
{
	return vaddq_u16(xbr555_neon_dist(a, b), xbr555_neon_dist(b, c));
}

static inline uint16x8_t xbr555_neon_11(uint16x8_t a, uint16x8_t b)
{
	return vaddq_u16(vandq_u16(vshrq_n_u16(veorq_u16(a, b), 1), vdupq_n_u16(0x3DEF)), vandq_u16(a, b));
}

/*
 * The weights and shifts are compile-time constants. Component arithmetic
 * stays in 16-bit lanes, then channels are repacked without saturation.
 */
#define XBR555_NEON_WEIGHT_FUNCTION(name, weight, shift) \
	static inline uint16x8_t name(uint16x8_t a, uint16x8_t b) \
	{ \
		const uint16x8_t m31 = vdupq_n_u16(31); \
		const uint16x8_t mg = vdupq_n_u16(31); \
		uint16x8_t ra = vandq_u16(vshrq_n_u16(a, 10), m31); \
		uint16x8_t rb = vandq_u16(vshrq_n_u16(b, 10), m31); \
		uint16x8_t ga = vandq_u16(vshrq_n_u16(a, 5), mg); \
		uint16x8_t gb = vandq_u16(vshrq_n_u16(b, 5), mg); \
		uint16x8_t ba = vandq_u16(a, m31); \
		uint16x8_t bb = vandq_u16(b, m31); \
		uint16x8_t r = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ra, weight), rb), shift); \
		uint16x8_t g = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ga, weight), gb), shift); \
		uint16x8_t bl = vshrq_n_u16(vaddq_u16(vmulq_n_u16(ba, weight), bb), shift); \
		return vorrq_u16(vorrq_u16(vshlq_n_u16(r, 10), vshlq_n_u16(g, 5)), bl); \
	}

static inline uint16x8_t xbr555_neon_31(uint16x8_t a, uint16x8_t b)
{
	return xbr555_neon_11(a, xbr555_neon_11(a, b));
}

XBR555_NEON_WEIGHT_FUNCTION(xbr555_neon_71, 7, 3)

#define XBR555_NEON(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		uint16x8_t _gate = vmvnq_u16(vorrq_u16(vceqq_u16((PE), (PH)), vceqq_u16((PE), (PF)))); \
		if (vmaxvq_u16(_gate) != 0) { \
			uint16x8_t _e = vaddq_u16(vaddq_u16(xbr555_neon_dist3((PC), (PE), (PG)), \
					xbr555_neon_dist3((H5), (PI), (F4))), \
					vshlq_n_u16(xbr555_neon_dist((PH), (PF)), 2)); \
			uint16x8_t _i = vaddq_u16(vaddq_u16(xbr555_neon_dist3((PD), (PH), (I5)), \
					xbr555_neon_dist3((I4), (PF), (PB))), \
					vshlq_n_u16(xbr555_neon_dist((PE), (PI)), 2)); \
			uint16x8_t _active = vandq_u16(_gate, vcgtq_u16(_i, _e)); \
			if (vmaxvq_u16(_active) != 0) { \
				uint16x8_t _ke = xbr555_neon_dist((PF), (PG)); \
				uint16x8_t _ki = xbr555_neon_dist((PH), (PC)); \
				uint16x8_t _ex2 = vbicq_u16(_active, vorrq_u16(vceqq_u16((PE), (PC)), vceqq_u16((PB), (PC)))); \
				uint16x8_t _ex3 = vbicq_u16(_active, vorrq_u16(vceqq_u16((PE), (PG)), vceqq_u16((PD), (PG)))); \
				uint16x8_t _leftup = vandq_u16(vandq_u16(_ex2, _ex3), \
						vandq_u16(vceqq_u16(_ke, vdupq_n_u16(0)), vceqq_u16(_ki, vdupq_n_u16(0)))); \
				uint16x8_t _left = vbicq_u16(vandq_u16(_ex3, vcleq_u16(vshlq_n_u16(_ke, 1), _ki)), _leftup); \
				uint16x8_t _up = vbicq_u16(vandq_u16(_ex2, vcgeq_u16(_ke, vshlq_n_u16(_ki, 1))), vorrq_u16(_leftup, _left)); \
				uint16x8_t _dia = vbicq_u16(_active, vorrq_u16(vorrq_u16(_leftup, _left), _up)); \
				uint16x8_t _px = vbslq_u16(vcgtq_u16(xbr555_neon_dist((PE), (PF)), xbr555_neon_dist((PE), (PH))), (PH), (PF)); \
				uint16x8_t _e3 = (N3), _e1 = (N1), _e2 = (N2); \
				uint16x8_t _n3 = vbslq_u16(_leftup, xbr555_neon_71(_px, _e3), _e3); \
				_n3 = vbslq_u16(vorrq_u16(_left, _up), xbr555_neon_31(_px, _e3), _n3); \
				_n3 = vbslq_u16(_dia, xbr555_neon_11(_e3, _px), _n3); \
				uint16x8_t _t2 = xbr555_neon_31(_e2, _px); \
				uint16x8_t _t1 = xbr555_neon_31(_e1, _px); \
				(N3) = _n3; \
				(N2) = vbslq_u16(vorrq_u16(_leftup, _left), _t2, _e2); \
				(N1) = vbslq_u16(_up, _t1, vbslq_u16(_leftup, _t2, _e1)); \
			} \
		} \
} while (0)

/*
 * Scalar border implementation, duplicated here so this kernel does not
 * depend on the global RGB16 masks or overwrite previously written pixels.
 */
static inline int xbr555_dist_s(interp_uint16 a, interp_uint16 b)
{
	int r = ((a >> 10) & 31) - ((b >> 10) & 31);
	int g = ((a >> 5) & 31) - ((b >> 5) & 31);
	int bl = (a & 31) - (b & 31);
	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (bl < 0) bl = -bl;
	return (3 * r + 6 * g + bl) * 8;
}

static inline int xbr555_dist3_s(interp_uint16 a, interp_uint16 b, interp_uint16 c)
{
	return xbr555_dist_s(a, b) + xbr555_dist_s(b, c);
}

static inline interp_uint16 xbr555_11_s(interp_uint16 a, interp_uint16 b)
{
	return (((a ^ b) >> 1) & 0x3DEF) + (a & b);
}

static inline interp_uint16 xbr555_weight_s(interp_uint16 a, interp_uint16 b, unsigned wa, unsigned shift)
{
	unsigned r = (wa * ((a >> 10) & 31) + ((b >> 10) & 31)) >> shift;
	unsigned g = (wa * ((a >> 5) & 31) + ((b >> 5) & 31)) >> shift;
	unsigned bl = (wa * (a & 31) + (b & 31)) >> shift;
	return (interp_uint16)((r << 10) | (g << 5) | bl);
}

static inline interp_uint16 xbr555_31_s(interp_uint16 a, interp_uint16 b)
{
	return xbr555_11_s(a, xbr555_11_s(a, b));
}

static inline interp_uint16 xbr555_71_s(interp_uint16 a, interp_uint16 b)
{
	return xbr555_weight_s(a, b, 7, 3);
}

#define XBR555_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned _e = xbr555_dist3_s((PC), (PE), (PG)) + xbr555_dist3_s((H5), (PI), (F4)) + 4 * xbr555_dist_s((PH), (PF)); \
			unsigned _i = xbr555_dist3_s((PD), (PH), (I5)) + xbr555_dist3_s((I4), (PF), (PB)) + 4 * xbr555_dist_s((PE), (PI)); \
			if (_e < _i) { \
				int _ex2 = (PE) != (PC) && (PB) != (PC); \
				int _ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned _ke = xbr555_dist_s((PF), (PG)); \
				unsigned _ki = xbr555_dist_s((PH), (PC)); \
				interp_uint16 _px = xbr555_dist_s((PE), (PF)) <= xbr555_dist_s((PE), (PH)) ? (PF) : (PH); \
				if (_ke == 0 && _ki == 0 && _ex3 && _ex2) { \
					(N3) = xbr555_71_s(_px, (N3)); \
					(N1) = (N2) = xbr555_31_s((N2), _px); \
				} else if (2 * _ke <= _ki && _ex3) { \
					(N3) = xbr555_31_s(_px, (N3)); \
					(N2) = xbr555_31_s((N2), _px); \
				} else if (_ke >= 2 * _ki && _ex2) { \
					(N3) = xbr555_31_s(_px, (N3)); \
					(N1) = xbr555_31_s((N1), _px); \
				} else { \
					(N3) = xbr555_11_s((N3), _px); \
				} \
			} \
		} \
} while (0)

static inline void xbr555_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* src0, const interp_uint16* src1,
	const interp_uint16* src2, const interp_uint16* src3,
	const interp_uint16* src4, unsigned count, unsigned x)
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
	interp_uint16 E0 = PE, E1 = PE, E2 = PE, E3 = PE;
	XBR555_S(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR555_S(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR555_S(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR555_S(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);
	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

void xbr2x_16_555_neon(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1,
	const interp_uint16* restrict src2, const interp_uint16* restrict src3,
	const interp_uint16* restrict src4, unsigned count)
{
	unsigned x = 0;

	for (; x < count && x < 2; ++x)
		xbr555_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* Eight source pixels per SIMD block. The widest source access is x+9. */
	for (; count - x >= 10; x += 8) {
#define LD555(row, off) vld1q_u16((row) + x + (off))
		uint16x8_t A1 = LD555(src0, -1), B1 = LD555(src0, 0), C1 = LD555(src0, 1);
		uint16x8_t A0 = LD555(src1, -2), PA = LD555(src1, -1), PB = LD555(src1, 0), PC = LD555(src1, 1), C4 = LD555(src1, 2);
		uint16x8_t D0 = LD555(src2, -2), PD = LD555(src2, -1), PE = LD555(src2, 0), PF = LD555(src2, 1), F4 = LD555(src2, 2);
		uint16x8_t G0 = LD555(src3, -2), PG = LD555(src3, -1), PH = LD555(src3, 0), PI = LD555(src3, 1), I4 = LD555(src3, 2);
		uint16x8_t G5 = LD555(src4, -1), H5 = LD555(src4, 0), I5 = LD555(src4, 1);
#undef LD555
		uint16x8_t E0 = PE, E1 = PE, E2 = PE, E3 = PE;
		XBR555_NEON(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR555_NEON(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR555_NEON(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR555_NEON(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

		/* Four 128-bit stores; each resulting destination pixel is written once. */
		vst1q_u16(dst0 + 2 * x, vzip1q_u16(E0, E1));
		vst1q_u16(dst0 + 2 * x + 8, vzip2q_u16(E0, E1));
		vst1q_u16(dst1 + 2 * x, vzip1q_u16(E2, E3));
		vst1q_u16(dst1 + 2 * x + 8, vzip2q_u16(E2, E3));
	}

	for (; x < count; ++x)
		xbr555_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

static inline uint32x4_t xbr32_neon_dist(uint32x4_t a, uint32x4_t b)
{
	const uint32x4_t m = vdupq_n_u32(255);
	uint32x4_t r = vabdq_u32(vandq_u32(vshrq_n_u32(a, 16), m), vandq_u32(vshrq_n_u32(b, 16), m));
	uint32x4_t g = vabdq_u32(vandq_u32(vshrq_n_u32(a, 8), m), vandq_u32(vshrq_n_u32(b, 8), m));
	uint32x4_t bl = vabdq_u32(vandq_u32(a, m), vandq_u32(b, m));
	/* Max distance is 2550 per lane. */
	r = vaddq_u32(vshlq_n_u32(r, 1), r);
	g = vaddq_u32(vshlq_n_u32(g, 2), vshlq_n_u32(g, 1));
	return vaddq_u32(vaddq_u32(r, g), bl);
}

static inline uint32x4_t xbr32_neon_dist3(uint32x4_t a, uint32x4_t b, uint32x4_t c)
{
	return vaddq_u32(xbr32_neon_dist(a, b), xbr32_neon_dist(b, c));
}

static inline uint32x4_t xbr32_neon_11(uint32x4_t a, uint32x4_t b)
{
	const uint32x4_t mask = vdupq_n_u32(0xFF7F7F7FU);
	return vaddq_u32(vandq_u32(vshrq_n_u32(veorq_u32(a, b), 1), mask), vandq_u32(a, b));
}

static inline uint32x4_t xbr32_neon_31(uint32x4_t a, uint32x4_t b)
{
	return xbr32_neon_11(a, xbr32_neon_11(a, b));
}

static inline uint32x4_t xbr32_neon_71(uint32x4_t a, uint32x4_t b)
{
	const uint32x4_t rbmask = vdupq_n_u32(0x00FF00FFU);
	const uint32x4_t gmask = vdupq_n_u32(0x0000FF00U);
	uint32x4_t ar = vandq_u32(a, rbmask), br = vandq_u32(b, rbmask);
	uint32x4_t ag = vandq_u32(a, gmask), bg = vandq_u32(b, gmask);
	uint32x4_t rb = vandq_u32(vshrq_n_u32(vaddq_u32(vsubq_u32(vshlq_n_u32(ar, 3), ar), br), 3), rbmask);
	uint32x4_t g = vandq_u32(vshrq_n_u32(vaddq_u32(vsubq_u32(vshlq_n_u32(ag, 3), ag), bg), 3), gmask);
	return vorrq_u32(rb, g);
}

#define XBR32_NEON(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		uint32x4_t _gate = vmvnq_u32(vorrq_u32(vceqq_u32((PE), (PH)), vceqq_u32((PE), (PF)))); \
		if (vmaxvq_u32(_gate) != 0) { \
			uint32x4_t _e = vaddq_u32(vaddq_u32(xbr32_neon_dist3((PC), (PE), (PG)), \
					xbr32_neon_dist3((H5), (PI), (F4))), \
					vshlq_n_u32(xbr32_neon_dist((PH), (PF)), 2)); \
			uint32x4_t _i = vaddq_u32(vaddq_u32(xbr32_neon_dist3((PD), (PH), (I5)), \
					xbr32_neon_dist3((I4), (PF), (PB))), \
					vshlq_n_u32(xbr32_neon_dist((PE), (PI)), 2)); \
			uint32x4_t _active = vandq_u32(_gate, vcgtq_u32(_i, _e)); \
			if (vmaxvq_u32(_active) != 0) { \
				const uint32x4_t _zero = vdupq_n_u32(0); \
				uint32x4_t _ke = xbr32_neon_dist((PF), (PG)); \
				uint32x4_t _ki = xbr32_neon_dist((PH), (PC)); \
				uint32x4_t _ex2 = vbicq_u32(_active, vorrq_u32(vceqq_u32((PE), (PC)), vceqq_u32((PB), (PC)))); \
				uint32x4_t _ex3 = vbicq_u32(_active, vorrq_u32(vceqq_u32((PE), (PG)), vceqq_u32((PD), (PG)))); \
				uint32x4_t _both = vandq_u32(_ex2, _ex3); \
				uint32x4_t _leftup = vandq_u32(_both, vandq_u32(vceqq_u32(_ke, _zero), vceqq_u32(_ki, _zero))); \
				uint32x4_t _left = vbicq_u32(vandq_u32(_ex3, vcleq_u32(vshlq_n_u32(_ke, 1), _ki)), _leftup); \
				uint32x4_t _up = vbicq_u32(vandq_u32(_ex2, vcgeq_u32(_ke, vshlq_n_u32(_ki, 1))), vorrq_u32(_leftup, _left)); \
				uint32x4_t _dia = vbicq_u32(_active, vorrq_u32(vorrq_u32(_leftup, _left), _up)); \
				uint32x4_t _px = vbslq_u32(vcgtq_u32(xbr32_neon_dist((PE), (PF)), xbr32_neon_dist((PE), (PH))), (PH), (PF)); \
				uint32x4_t _e3 = (N3), _e1 = (N1), _e2 = (N2); \
				uint32x4_t _side = vorrq_u32(_left, _up); \
				uint32x4_t _n3 = vbslq_u32(_leftup, xbr32_neon_71(_px, _e3), _e3); \
				_n3 = vbslq_u32(_side, xbr32_neon_31(_px, _e3), _n3); \
				_n3 = vbslq_u32(_dia, xbr32_neon_11(_e3, _px), _n3); \
				uint32x4_t _t2 = xbr32_neon_31(_e2, _px); \
				uint32x4_t _t1 = xbr32_neon_31(_e1, _px); \
				(N3) = _n3; \
				(N2) = vbslq_u32(vorrq_u32(_leftup, _left), _t2, _e2); \
				(N1) = vbslq_u32(_up, _t1, vbslq_u32(_leftup, _t2, _e1)); \
			} \
		} \
} while (0)

/* Scalar path for boundary pixels: source coordinates clamped to the full row. */
#define XBR32_NEON_EDGE(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, N0, N1, N2, N3) do { \
		if ((PE) != (PH) && (PE) != (PF)) { \
			unsigned e = interp_32_dist3((PC), (PE), (PG)) + interp_32_dist3((H5), (PI), (F4)) + 4 * interp_32_dist((PH), (PF)); \
			unsigned i = interp_32_dist3((PD), (PH), (I5)) + interp_32_dist3((I4), (PF), (PB)) + 4 * interp_32_dist((PE), (PI)); \
			if (e < i) { \
				int ex2 = (PE) != (PC) && (PB) != (PC); \
				int ex3 = (PE) != (PG) && (PD) != (PG); \
				unsigned ke = interp_32_dist((PF), (PG)); \
				unsigned ki = interp_32_dist((PH), (PC)); \
				interp_uint32 px = interp_32_dist((PE), (PF)) <= interp_32_dist((PE), (PH)) ? (PF) : (PH); \
				if (ke == 0 && ki == 0 && ex3 && ex2) { \
					(N3) = interp_32_71(px, (N3)); \
					(N1) = (N2) = interp_32_31((N2), px); \
				} else if (2 * ke <= ki && ex3) { \
					(N3) = interp_32_31(px, (N3)); \
					(N2) = interp_32_31((N2), px); \
				} else if (ke >= 2 * ki && ex2) { \
					(N3) = interp_32_31(px, (N3)); \
					(N1) = interp_32_31((N1), px); \
				} else { \
					(N3) = interp_32_11((N3), px); \
				} \
			} \
		} \
} while (0)

static inline void xbr32_neon_edge_pixel(interp_uint32* restrict dst0,
	interp_uint32* restrict dst1,
	const interp_uint32* src0,
	const interp_uint32* src1,
	const interp_uint32* src2,
	const interp_uint32* src3,
	const interp_uint32* src4,
	unsigned count, unsigned x)
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

	interp_uint32 E0 = PE, E1 = PE, E2 = PE, E3 = PE;
	XBR32_NEON_EDGE(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
	XBR32_NEON_EDGE(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
	XBR32_NEON_EDGE(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
	XBR32_NEON_EDGE(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

	dst0[2 * x] = E0;
	dst0[2 * x + 1] = E1;
	dst1[2 * x] = E2;
	dst1[2 * x + 1] = E3;
}

void xbr2x_32_neon(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1,
	const interp_uint32* restrict src2, const interp_uint32* restrict src3,
	const interp_uint32* restrict src4, unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* Left border. No vector may load outside the actual input row. */
	for (; x < count && x < 2; ++x)
		xbr32_neon_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);

	/* Four source pixels per iteration. Valid columns are x-2 through x+5. */
	for (; count - x >= 6; x += 4) {
#define LD(row, off) vld1q_u32((row) + x + (off))
		uint32x4_t A1 = LD(src0, -1), B1 = LD(src0, 0), C1 = LD(src0, 1);
		uint32x4_t A0 = LD(src1, -2), PA = LD(src1, -1), PB = LD(src1, 0), PC = LD(src1, 1), C4 = LD(src1, 2);
		uint32x4_t D0 = LD(src2, -2), PD = LD(src2, -1), PE = LD(src2, 0), PF = LD(src2, 1), F4 = LD(src2, 2);
		uint32x4_t G0 = LD(src3, -2), PG = LD(src3, -1), PH = LD(src3, 0), PI = LD(src3, 1), I4 = LD(src3, 2);
		uint32x4_t G5 = LD(src4, -1), H5 = LD(src4, 0), I5 = LD(src4, 1);
#undef LD
		uint32x4_t E0 = PE, E1 = PE, E2 = PE, E3 = PE;

		XBR32_NEON(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, E0, E1, E2, E3);
		XBR32_NEON(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, E2, E0, E3, E1);
		XBR32_NEON(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, E3, E2, E1, E0);
		XBR32_NEON(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, E1, E3, E0, E2);

		/* Two 128-bit stores per output row, one write per resulting pixel. */
		vst1q_u32(dst0 + 2 * x, vzip1q_u32(E0, E1));
		vst1q_u32(dst0 + 2 * x + 4, vzip2q_u32(E0, E1));
		vst1q_u32(dst1 + 2 * x, vzip1q_u32(E2, E3));
		vst1q_u32(dst1 + 2 * x + 4, vzip2q_u32(E2, E3));
	}

	/* Right border and tail: direct writes, no scratch buffers. */
	for (; x < count; ++x)
		xbr32_neon_edge_pixel(dst0, dst1, src0, src1, src2, src3, src4, count, x);
}

#endif

