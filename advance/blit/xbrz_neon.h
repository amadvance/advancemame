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

#ifndef __XBRZ_NEON_H
#define __XBRZ_NEON_H

#include "xbrz2x.h"

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

#include "interp_neon.h"

/**
 * Parts shared by the xBRZ 2x, 3x and 4x NEON implementations, for both
 * AArch64 (CONFIG_NEON) and ARMv7 (CONFIG_NEON32).
 *
 * The lane width w selects the vector type and the helpers: 16 processes
 * eight 16 bit pixels and 32 four 32 bit pixels in 128-bit vectors.
 *
 * ARMv7 also uses 128-bit vectors, although it has only 16 quad registers
 * and the blending spills many of them. With 64-bit vectors there are
 * twice the blocks, and each block pays the early exit tests, which move
 * a NEON result to the core registers and stall the pipeline. On a
 * Raspberry Pi 4 in 32 bit mode, with the scaling split on three threads,
 * 128-bit vectors are up to 25% faster in advmame, while 64-bit ones are
 * faster only in a single thread on heavily detailed images.
 *
 * Every mask is a full lane of ones or zeros, so it is used directly by vbsl.
 * All the values are unsigned: distances and scores never overflow their
 * lanes, as 18 * 2504 = 45072 still fits an unsigned 16-bit lane in the
 * dominant test of the 16 bit formats (see xbrz_sse.h).
 */

/**
 * Generic helpers. Q is q for the 128-bit intrinsics and T their lane type.
 */
#define XBRZ_NEON_SPECIALIZE(w, Q, T) \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _set_neon(unsigned v) \
	{ \
		return vdup ## Q ## _n_ ## T(v); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _eq_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vceq ## Q ## _ ## T(a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _gt_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vcgt ## Q ## _ ## T(a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _add_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vadd ## Q ## _ ## T(a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _max_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vmax ## Q ## _ ## T(a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _min_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vmin ## Q ## _ ## T(a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _or_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vorr ## Q ## _ ## T(a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _and_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vand ## Q ## _ ## T(a, b); \
	} \
	/* a & ~b */ \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _bic_neon(xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vbic ## Q ## _ ## T(a, b); \
	} \
	/* m ? a : b */ \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _sel_neon(xbrz_ ## w ## _neon_t m, xbrz_ ## w ## _neon_t a, xbrz_ ## w ## _neon_t b) \
	{ \
		return vbsl ## Q ## _ ## T(m, a, b); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _mul4_neon(xbrz_ ## w ## _neon_t a) \
	{ \
		return vshl ## Q ## _n_ ## T(a, 2); \
	} \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _flag_neon(int v) \
	{ \
		return vcgt ## Q ## _ ## T(vdup ## Q ## _n_ ## T(v != 0), vdup ## Q ## _n_ ## T(0)); \
	} \
	/* mask of the lanes where the dominant test 18 * dmin < 5 * dmax fails */ \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _nodom_neon(xbrz_ ## w ## _neon_t dmin, xbrz_ ## w ## _neon_t dmax) \
	{ \
		return vcge ## Q ## _ ## T(xbrz_ ## w ## _mul18_neon(dmin), xbrz_ ## w ## _mul5_neon(dmax)); \
	} \
	/* move every lane one pixel right, shifting in the last lane of the previous block */ \
	static inline xbrz_ ## w ## _neon_t xbrz_ ## w ## _prev_neon(xbrz_ ## w ## _neon_t cur, xbrz_ ## w ## _neon_t prev) \
	{ \
		return vext ## Q ## _ ## T(prev, cur, xbrz_ ## w ## _LANES - 1); \
	} \
	/** \
	 * Check if no pixel of the block can have any blending flag, before \
	 * classifying it. The four right flags of a pixel all require PE != PF, \
	 * and the only other flags are the left ones carried in the last lane of \
	 * PTL and PBL. This is the early exit with all the flags XBRZ_NONE, taken \
	 * before computing the distances, which makes flat areas almost free. \
	 */ \
	static inline int xbrz_ ## w ## _flat_neon(xbrz_ ## w ## _neon_t eq_ef, xbrz_ ## w ## _neon_t ptl, xbrz_ ## w ## _neon_t pbl) \
	{ \
		xbrz_ ## w ## _neon_t carry = vand ## Q ## _ ## T(vorr ## Q ## _ ## T(ptl, pbl), xbrz_ ## w ## _last_neon()); \
		return xbrz_ ## w ## _all_neon(vbic ## Q ## _ ## T(eq_ef, carry)); \
	}

typedef uint16x8_t xbrz_16_neon_t;
typedef uint32x4_t xbrz_32_neon_t;

#define xbrz_16_LANES 8
#define xbrz_32_LANES 4

#if defined(CONFIG_NEON)
static inline int xbrz_16_any_neon(uint16x8_t m)
{
	return vmaxvq_u16(m) != 0;
}

static inline int xbrz_16_all_neon(uint16x8_t m)
{
	return vminvq_u16(m) != 0;
}

static inline int xbrz_32_any_neon(uint32x4_t m)
{
	return vmaxvq_u32(m) != 0;
}

static inline int xbrz_32_all_neon(uint32x4_t m)
{
	return vminvq_u32(m) != 0;
}
#else
/* ARMv7 has no horizontal reductions: fold to 64 bits and move once to the core registers */
static inline int xbrz_16_any_neon(uint16x8_t m)
{
	return vget_lane_u64(vreinterpret_u64_u16(vorr_u16(vget_low_u16(m), vget_high_u16(m))), 0) != 0;
}

static inline int xbrz_16_all_neon(uint16x8_t m)
{
	return vget_lane_u64(vreinterpret_u64_u16(vand_u16(vget_low_u16(m), vget_high_u16(m))), 0) == 0xFFFFFFFFFFFFFFFFULL;
}

static inline int xbrz_32_any_neon(uint32x4_t m)
{
	return vget_lane_u64(vreinterpret_u64_u32(vorr_u32(vget_low_u32(m), vget_high_u32(m))), 0) != 0;
}

static inline int xbrz_32_all_neon(uint32x4_t m)
{
	return vget_lane_u64(vreinterpret_u64_u32(vand_u32(vget_low_u32(m), vget_high_u32(m))), 0) == 0xFFFFFFFFFFFFFFFFULL;
}
#endif

static inline uint16x8_t xbrz_16_last_neon(void)
{
	return vcombine_u16(vcreate_u16(0), vcreate_u16(0xFFFF000000000000ULL));
}

/* 12 * dist, 5 * dist and 18 * score always fit an unsigned 16-bit lane */
static inline uint16x8_t xbrz_16_mul5_neon(uint16x8_t a)
{
	return vmulq_n_u16(a, 5);
}

static inline uint16x8_t xbrz_16_mul12_neon(uint16x8_t a)
{
	return vmulq_n_u16(a, 12);
}

static inline uint16x8_t xbrz_16_mul18_neon(uint16x8_t a)
{
	return vmulq_n_u16(a, 18);
}

static inline uint32x4_t xbrz_32_last_neon(void)
{
	return vcombine_u32(vcreate_u32(0), vcreate_u32(0xFFFFFFFF00000000ULL));
}

/* shifts and adds, since 32-bit multiplications are slower */
static inline uint32x4_t xbrz_32_mul5_neon(uint32x4_t a)
{
	return vaddq_u32(vshlq_n_u32(a, 2), a);
}

static inline uint32x4_t xbrz_32_mul12_neon(uint32x4_t a)
{
	return vaddq_u32(vshlq_n_u32(a, 3), vshlq_n_u32(a, 2));
}

static inline uint32x4_t xbrz_32_mul18_neon(uint32x4_t a)
{
	return vaddq_u32(vshlq_n_u32(a, 4), vshlq_n_u32(a, 1));
}

XBRZ_NEON_SPECIALIZE(16, q, u16)
XBRZ_NEON_SPECIALIZE(32, q, u32)

static inline uint16x8_t xbrz_16_565_load_neon(const interp_uint16* p)
{
	return vld1q_u16(p);
}

/* the C implementation clears the unused bit, which affects the equality tests */
static inline uint16x8_t xbrz_16_555_load_neon(const interp_uint16* p)
{
	return vandq_u16(vld1q_u16(p), vdupq_n_u16(0x7FFF));
}

/* the C implementation clears the alpha channel, which affects the equality tests */
static inline uint32x4_t xbrz_32_load_neon(const interp_uint32* p)
{
	return vandq_u32(vld1q_u32(p), vdupq_n_u32(0xFFFFFF));
}

/**
 *          . B1 C1  .
 *         PA PB PC C4
 *         PD PE PF F4
 *         PG PH PI I4
 *          . H5 I5  .
 *
 * SIMD version of the two XBRZ_PREPROCESS of the C implementation, which
 * classify the upper-right (top) and lower-right (bottom) vertices of PE.
 *
 * The C implementation reuses top.br and bot.tr of the previous pixel as
 * its TL and BL. Lanes are independent, so the same values are moved one
 * lane right with vext, taking the last lane from the previous block
 * saved in PTL, PTLD, PBL and PBLD. This is faster than classifying the
 * two left vertices again from loads shifted by one column, which doubles
 * the classification work and also needs the column x - 2.
 *
 * The early exit of XBRZ_PREPROCESS when F, G, J, K form two pairs of equal
 * pixels is omitted. Every flag also requires its pixel to differ from its
 * two neighbors in the vertex, and this already fails in that case.
 *
 * EQ_EF is the mask of PE == PF, already computed for xbrz_*_flat_neon().
 * The distances DEA, DEC, DEG and DEI are outputs, as they are reused by
 * the blending. Each flag is a pair of masks: X for X != XBRZ_NONE and XD
 * for X == XBRZ_DOMINANT.
 */
#define XBRZ_CLASSIFY_NEON(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, EQ_EF, DEA, DEC, DEG, DEI, TL, TLD, TR, TRD, BL, BLD, BR, BRD, PTL, PTLD, PBL, PBLD) \
	do { \
		xbrz_ ## w ## _neon_t _eq_be = xbrz_ ## w ## _eq_neon((PB), (PE)); \
		xbrz_ ## w ## _neon_t _eq_cf = xbrz_ ## w ## _eq_neon((PC), (PF)); \
		xbrz_ ## w ## _neon_t _eq_eh = xbrz_ ## w ## _eq_neon((PE), (PH)); \
		xbrz_ ## w ## _neon_t _eq_fi = xbrz_ ## w ## _eq_neon((PF), (PI)); \
		xbrz_ ## w ## _neon_t _dbf = interp_ ## fmt ## _dist_neon((PB), (PF)); \
		xbrz_ ## w ## _neon_t _dfh = interp_ ## fmt ## _dist_neon((PF), (PH)); \
		xbrz_ ## w ## _neon_t _d0, _d1, _nd, _ntl, _ntld, _nbl, _nbld; \
		(DEA) = interp_ ## fmt ## _dist_neon((PE), (PA)); \
		(DEC) = interp_ ## fmt ## _dist_neon((PE), (PC)); \
		(DEG) = interp_ ## fmt ## _dist_neon((PE), (PG)); \
		(DEI) = interp_ ## fmt ## _dist_neon((PE), (PI)); \
		/* top vertex with F = PB, G = PC, J = PE, K = PF */ \
		_d0 = xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon(interp_ ## fmt ## _dist_neon((PD), (PB)), interp_ ## fmt ## _dist_neon((PB), (C1))), \
				xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon(_dfh, interp_ ## fmt ## _dist_neon((PF), (C4))), xbrz_ ## w ## _mul4_neon(DEC))); \
		_d1 = xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon((DEA), (DEI)), \
				xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon(interp_ ## fmt ## _dist_neon((B1), (PC)), interp_ ## fmt ## _dist_neon((PC), (F4))), xbrz_ ## w ## _mul4_neon(_dbf))); \
		_nd = xbrz_ ## w ## _nodom_neon(xbrz_ ## w ## _min_neon(_d0, _d1), xbrz_ ## w ## _max_neon(_d0, _d1)); \
		/* top.bl is the TR of PE: J != F, J != K and d1 < d0 */ \
		(TR) = xbrz_ ## w ## _bic_neon(xbrz_ ## w ## _gt_neon(_d0, _d1), xbrz_ ## w ## _or_neon(_eq_be, (EQ_EF))); \
		(TRD) = xbrz_ ## w ## _bic_neon((TR), _nd); \
		/* top.br is the TL of the next pixel: K != G, K != J and d0 < d1 */ \
		_ntl = xbrz_ ## w ## _bic_neon(xbrz_ ## w ## _gt_neon(_d1, _d0), xbrz_ ## w ## _or_neon(_eq_cf, (EQ_EF))); \
		_ntld = xbrz_ ## w ## _bic_neon(_ntl, _nd); \
		/* bottom vertex with F = PE, G = PF, J = PH, K = PI */ \
		_d0 = xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon((DEG), (DEC)), \
				xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon(interp_ ## fmt ## _dist_neon((H5), (PI)), interp_ ## fmt ## _dist_neon((PI), (F4))), xbrz_ ## w ## _mul4_neon(_dfh))); \
		_d1 = xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon(interp_ ## fmt ## _dist_neon((PD), (PH)), interp_ ## fmt ## _dist_neon((PH), (I5))), \
				xbrz_ ## w ## _add_neon(xbrz_ ## w ## _add_neon(_dbf, interp_ ## fmt ## _dist_neon((PF), (I4))), xbrz_ ## w ## _mul4_neon(DEI))); \
		_nd = xbrz_ ## w ## _nodom_neon(xbrz_ ## w ## _min_neon(_d0, _d1), xbrz_ ## w ## _max_neon(_d0, _d1)); \
		/* bot.tl is the BR of PE: F != G, F != J and d0 < d1 */ \
		(BR) = xbrz_ ## w ## _bic_neon(xbrz_ ## w ## _gt_neon(_d1, _d0), xbrz_ ## w ## _or_neon((EQ_EF), _eq_eh)); \
		(BRD) = xbrz_ ## w ## _bic_neon((BR), _nd); \
		/* bot.tr is the BL of the next pixel: G != F, G != K and d1 < d0 */ \
		_nbl = xbrz_ ## w ## _bic_neon(xbrz_ ## w ## _gt_neon(_d0, _d1), xbrz_ ## w ## _or_neon((EQ_EF), _eq_fi)); \
		_nbld = xbrz_ ## w ## _bic_neon(_nbl, _nd); \
		/* the left vertices are the right ones of the previous pixel */ \
		(TL) = xbrz_ ## w ## _prev_neon(_ntl, (PTL)); \
		(TLD) = xbrz_ ## w ## _prev_neon(_ntld, (PTLD)); \
		(BL) = xbrz_ ## w ## _prev_neon(_nbl, (PBL)); \
		(BLD) = xbrz_ ## w ## _prev_neon(_nbld, (PBLD)); \
		(PTL) = _ntl; \
		(PTLD) = _ntld; \
		(PBL) = _nbl; \
		(PBLD) = _nbld; \
	} while (0)

/**
 *       A B C
 *       D E F
 *       G H I
 *
 * SIMD version of the decision part of XBRZ2, XBRZ3 and XBRZ4, which is
 * the same for all the scales. Compute the blending pixel PX and the masks
 * LINE, SHALLOW and STEEP, all limited to the lanes with BR != XBRZ_NONE.
 * The distances are passed precomputed, as each of them is shared by two
 * rotations or with the classification.
 */
#define XBRZ_LINE_NEON(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, LINE, PX, SHALLOW, STEEP) \
	do { \
		const xbrz_ ## w ## _neon_t _eq = xbrz_ ## w ## _set_neon(XBRZ_EQUAL_ ## fmt - 1); \
		xbrz_ ## w ## _neon_t _ring = xbrz_ ## w ## _max_neon(xbrz_ ## w ## _max_neon((DGH), (DHI)), xbrz_ ## w ## _max_neon((DIF), (DFC))); \
		xbrz_ ## w ## _neon_t _no = xbrz_ ## w ## _or_neon(xbrz_ ## w ## _and_neon((TR), xbrz_ ## w ## _gt_neon((DEG), _eq)), xbrz_ ## w ## _and_neon((BL), xbrz_ ## w ## _gt_neon((DEC), _eq))); \
		xbrz_ ## w ## _neon_t _fg = interp_ ## fmt ## _dist_neon((F), (G)); \
		xbrz_ ## w ## _neon_t _hc = interp_ ## fmt ## _dist_neon((H), (C)); \
		xbrz_ ## w ## _neon_t _fg12 = xbrz_ ## w ## _mul12_neon(_fg); \
		xbrz_ ## w ## _neon_t _hc12 = xbrz_ ## w ## _mul12_neon(_hc); \
		xbrz_ ## w ## _neon_t _fg5 = xbrz_ ## w ## _mul5_neon(_fg); \
		xbrz_ ## w ## _neon_t _hc5 = xbrz_ ## w ## _mul5_neon(_hc); \
		/* the adjacent corner flags protect small features and L shapes */ \
		_no = xbrz_ ## w ## _or_neon(_no, xbrz_ ## w ## _bic_neon(xbrz_ ## w ## _gt_neon((DEI), _eq), xbrz_ ## w ## _gt_neon(_ring, _eq))); \
		(LINE) = xbrz_ ## w ## _or_neon((BRD), xbrz_ ## w ## _bic_neon((BR), _no)); \
		(PX) = xbrz_ ## w ## _sel_neon(xbrz_ ## w ## _gt_neon((DEF), (DEH)), (H), (F)); \
		(SHALLOW) = xbrz_ ## w ## _bic_neon((LINE), xbrz_ ## w ## _or_neon(xbrz_ ## w ## _gt_neon(_fg12, _hc5), \
				xbrz_ ## w ## _or_neon(xbrz_ ## w ## _eq_neon((E), (G)), xbrz_ ## w ## _eq_neon((D), (G))))); \
		(STEEP) = xbrz_ ## w ## _bic_neon((LINE), xbrz_ ## w ## _or_neon(xbrz_ ## w ## _gt_neon(_hc12, _fg5), \
				xbrz_ ## w ## _or_neon(xbrz_ ## w ## _eq_neon((E), (C)), xbrz_ ## w ## _eq_neon((B), (C))))); \
	} while (0)

/**
 * Distances used only by the blending: the four orthogonal neighbors of PE
 * and the eight edges of the ring around PE. They are computed after the
 * early exit, and only once for the four rotations.
 */
#define XBRZ_DIST_NEON(fmt, PA, PB, PC, PD, PE, PF, PG, PH, PI, DEB, DED, DEF, DEH, DAB, DBC, DCF, DFI, DHI, DGH, DDG, DAD) \
	do { \
		(DEB) = interp_ ## fmt ## _dist_neon((PE), (PB)); \
		(DED) = interp_ ## fmt ## _dist_neon((PE), (PD)); \
		(DEF) = interp_ ## fmt ## _dist_neon((PE), (PF)); \
		(DEH) = interp_ ## fmt ## _dist_neon((PE), (PH)); \
		(DAB) = interp_ ## fmt ## _dist_neon((PA), (PB)); \
		(DBC) = interp_ ## fmt ## _dist_neon((PB), (PC)); \
		(DCF) = interp_ ## fmt ## _dist_neon((PC), (PF)); \
		(DFI) = interp_ ## fmt ## _dist_neon((PF), (PI)); \
		(DHI) = interp_ ## fmt ## _dist_neon((PH), (PI)); \
		(DGH) = interp_ ## fmt ## _dist_neon((PG), (PH)); \
		(DDG) = interp_ ## fmt ## _dist_neon((PD), (PG)); \
		(DAD) = interp_ ## fmt ## _dist_neon((PA), (PD)); \
	} while (0)

/**
 * Initialize the state carried between SIMD blocks with the right vertices
 * of the pixel x - 1, i.e. the left vertices of the first pixel of the block.
 * Only the last lane is used by the vext of XBRZ_CLASSIFY_NEON.
 * sfmt is the scalar format of XBRZ_EDGE_LEFT.
 */
#define XBRZ_START_NEON(sfmt, w, MASK, src0, src1, src2, src3, src4, count, x, PTL, PTLD, PBL, PBLD) \
	do { \
		unsigned _sxm2 = (x) >= 2 ? (x) - 2 : 0; \
		unsigned _sxm1 = (x) >= 1 ? (x) - 1 : 0; \
		unsigned _sxp1 = (count) - (x) >= 2 ? (x) + 1 : (count) - 1; \
		unsigned _stl, _sbl; \
		XBRZ_EDGE_LEFT(sfmt, MASK, src0, src1, src2, src3, src4, _sxm2, _sxm1, x, _sxp1, _stl, _sbl); \
		(PTL) = xbrz_ ## w ## _flag_neon(_stl != XBRZ_NONE); \
		(PTLD) = xbrz_ ## w ## _flag_neon(_stl == XBRZ_DOMINANT); \
		(PBL) = xbrz_ ## w ## _flag_neon(_sbl != XBRZ_NONE); \
		(PBLD) = xbrz_ ## w ## _flag_neon(_sbl == XBRZ_DOMINANT); \
	} while (0)

#endif

#endif

