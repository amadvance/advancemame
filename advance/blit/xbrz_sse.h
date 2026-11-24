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

#ifndef __XBRZ_SSE_H
#define __XBRZ_SSE_H

#include "xbrz2x.h"

#ifdef CONFIG_X86

#include "interp_sse.h"

/**
 * Parts shared by the xBRZ 2x, 3x and 4x SSE4.1 implementations.
 *
 * The 16 bit formats process eight pixels in 16-bit lanes, the 32 bit format
 * four pixels in 32-bit lanes. Every mask is a full lane of ones or zeros,
 * so it is used directly by _mm_blendv_epi8().
 */

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_set_sse(int v)
{
	return _mm_set1_epi16(v);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_eq_sse(__m128i a, __m128i b)
{
	return _mm_cmpeq_epi16(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_gt_sse(__m128i a, __m128i b)
{
	return _mm_cmpgt_epi16(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_add_sse(__m128i a, __m128i b)
{
	return _mm_add_epi16(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_max_sse(__m128i a, __m128i b)
{
	return _mm_max_epi16(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_min_sse(__m128i a, __m128i b)
{
	return _mm_min_epi16(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_mul4_sse(__m128i a)
{
	return _mm_slli_epi16(a, 2);
}

/* 12 * dist <= 12 * 313 and 5 * dist <= 5 * 313 always fit a signed 16-bit lane */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_mul5_sse(__m128i a)
{
	return _mm_mullo_epi16(a, _mm_set1_epi16(5));
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_mul12_sse(__m128i a)
{
	return _mm_mullo_epi16(a, _mm_set1_epi16(12));
}

/**
 * Mask of the lanes where the dominant test 18 * dmin < 5 * dmax fails.
 *
 * In the 16 bit formats dist <= 313 (RGB565) or 310 (RGB555), so the
 * classification scores reach 2504 and 18 * 2504 = 45072 overflows a signed
 * 16-bit lane. It still fits an unsigned 16-bit lane (5 * 2504 = 12520 too),
 * so the products are exact and they are compared as unsigned with
 * max_epu16. This costs one instruction more than a signed compare, while
 * widening to 32 bits would double the registers and the work of the
 * whole classification.
 */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_nodom_sse(__m128i dmin, __m128i dmax)
{
	__m128i a = _mm_mullo_epi16(dmin, _mm_set1_epi16(18));
	__m128i b = _mm_mullo_epi16(dmax, _mm_set1_epi16(5));
	return _mm_cmpeq_epi16(_mm_max_epu16(a, b), a);
}

/* move every lane one pixel right, shifting in the last lane of the previous block */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_prev_sse(__m128i cur, __m128i prev)
{
	return _mm_alignr_epi8(cur, prev, 14);
}

/**
 * Check if no pixel of the block can have any blending flag, before
 * classifying it. The four right flags of a pixel all require PE != PF,
 * and the only other flags are the left ones carried in the last lane of
 * PTL and PBL. This is the early exit with all the flags XBRZ_NONE, taken
 * before computing the distances, which makes flat areas almost free.
 */
__attribute__((target("sse4.1"))) static inline int xbrz_16_flat_sse(__m128i eq_ef, __m128i ptl, __m128i pbl)
{
	__m128i carry = _mm_and_si128(_mm_or_si128(ptl, pbl), _mm_setr_epi16(0, 0, 0, 0, 0, 0, 0, -1));
	return _mm_testc_si128(_mm_andnot_si128(carry, eq_ef), _mm_set1_epi32(-1));
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_flag_sse(int v)
{
	return _mm_set1_epi16(v ? -1 : 0);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_565_load_sse(const interp_uint16* p)
{
	return _mm_loadu_si128((const __m128i*)(const void*)p);
}

/* the C implementation clears the unused bit, which affects the equality tests */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_16_555_load_sse(const interp_uint16* p)
{
	return _mm_and_si128(_mm_loadu_si128((const __m128i*)(const void*)p), _mm_set1_epi16(0x7FFF));
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_set_sse(int v)
{
	return _mm_set1_epi32(v);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_eq_sse(__m128i a, __m128i b)
{
	return _mm_cmpeq_epi32(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_gt_sse(__m128i a, __m128i b)
{
	return _mm_cmpgt_epi32(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_add_sse(__m128i a, __m128i b)
{
	return _mm_add_epi32(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_max_sse(__m128i a, __m128i b)
{
	return _mm_max_epi32(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_min_sse(__m128i a, __m128i b)
{
	return _mm_min_epi32(a, b);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_mul4_sse(__m128i a)
{
	return _mm_slli_epi32(a, 2);
}

/* shifts and adds, since _mm_mullo_epi32 is slow on many CPUs */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_mul5_sse(__m128i a)
{
	return _mm_add_epi32(_mm_slli_epi32(a, 2), a);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_mul12_sse(__m128i a)
{
	return _mm_add_epi32(_mm_slli_epi32(a, 3), _mm_slli_epi32(a, 2));
}

/* scores are at most 8 * 2550, and 18 times them easily fit a 32-bit lane */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_nodom_sse(__m128i dmin, __m128i dmax)
{
	__m128i a = _mm_add_epi32(_mm_slli_epi32(dmin, 4), _mm_slli_epi32(dmin, 1));
	__m128i b = _mm_add_epi32(_mm_slli_epi32(dmax, 2), dmax);
	return _mm_cmpeq_epi32(_mm_max_epi32(a, b), a);
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_prev_sse(__m128i cur, __m128i prev)
{
	return _mm_alignr_epi8(cur, prev, 12);
}

__attribute__((target("sse4.1"))) static inline int xbrz_32_flat_sse(__m128i eq_ef, __m128i ptl, __m128i pbl)
{
	__m128i carry = _mm_and_si128(_mm_or_si128(ptl, pbl), _mm_setr_epi32(0, 0, 0, -1));
	return _mm_testc_si128(_mm_andnot_si128(carry, eq_ef), _mm_set1_epi32(-1));
}

__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_flag_sse(int v)
{
	return _mm_set1_epi32(v ? -1 : 0);
}

/* the C implementation clears the alpha channel, which affects the equality tests */
__attribute__((target("sse4.1"))) static inline __m128i xbrz_32_load_sse(const interp_uint32* p)
{
	return _mm_and_si128(_mm_loadu_si128((const __m128i*)(const void*)p), _mm_set1_epi32(0xFFFFFF));
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
 * lane right with alignr, taking the last lane from the previous block
 * saved in PTL, PTLD, PBL and PBLD. This is faster than classifying the
 * two left vertices again from loads shifted by one column, which doubles
 * the classification work and also needs the column x - 2.
 *
 * The early exit of XBRZ_PREPROCESS when F, G, J, K form two pairs of equal
 * pixels is omitted. Every flag also requires its pixel to differ from its
 * two neighbors in the vertex, and this already fails in that case.
 *
 * EQ_EF is the mask of PE == PF, already computed for xbrz_*_flat_sse().
 * The distances DEA, DEC, DEG and DEI are outputs, as they are reused by
 * the blending. Each flag is a pair of masks: X for X != XBRZ_NONE and XD
 * for X == XBRZ_DOMINANT.
 */
#define XBRZ_CLASSIFY_SSE(fmt, w, B1, C1, PA, PB, PC, C4, PD, PE, PF, F4, PG, PH, PI, I4, H5, I5, EQ_EF, DEA, DEC, DEG, DEI, TL, TLD, TR, TRD, BL, BLD, BR, BRD, PTL, PTLD, PBL, PBLD) \
	do { \
		__m128i _eq_be = xbrz_ ## w ## _eq_sse((PB), (PE)); \
		__m128i _eq_cf = xbrz_ ## w ## _eq_sse((PC), (PF)); \
		__m128i _eq_eh = xbrz_ ## w ## _eq_sse((PE), (PH)); \
		__m128i _eq_fi = xbrz_ ## w ## _eq_sse((PF), (PI)); \
		__m128i _dbf = interp_ ## fmt ## _dist_sse((PB), (PF)); \
		__m128i _dfh = interp_ ## fmt ## _dist_sse((PF), (PH)); \
		__m128i _d0, _d1, _nd, _ntl, _ntld, _nbl, _nbld; \
		(DEA) = interp_ ## fmt ## _dist_sse((PE), (PA)); \
		(DEC) = interp_ ## fmt ## _dist_sse((PE), (PC)); \
		(DEG) = interp_ ## fmt ## _dist_sse((PE), (PG)); \
		(DEI) = interp_ ## fmt ## _dist_sse((PE), (PI)); \
		/* top vertex with F = PB, G = PC, J = PE, K = PF */ \
		_d0 = xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse(interp_ ## fmt ## _dist_sse((PD), (PB)), interp_ ## fmt ## _dist_sse((PB), (C1))), \
				xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse(_dfh, interp_ ## fmt ## _dist_sse((PF), (C4))), xbrz_ ## w ## _mul4_sse(DEC))); \
		_d1 = xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse((DEA), (DEI)), \
				xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse(interp_ ## fmt ## _dist_sse((B1), (PC)), interp_ ## fmt ## _dist_sse((PC), (F4))), xbrz_ ## w ## _mul4_sse(_dbf))); \
		_nd = xbrz_ ## w ## _nodom_sse(xbrz_ ## w ## _min_sse(_d0, _d1), xbrz_ ## w ## _max_sse(_d0, _d1)); \
		/* top.bl is the TR of PE: J != F, J != K and d1 < d0 */ \
		(TR) = _mm_andnot_si128(_mm_or_si128(_eq_be, (EQ_EF)), xbrz_ ## w ## _gt_sse(_d0, _d1)); \
		(TRD) = _mm_andnot_si128(_nd, (TR)); \
		/* top.br is the TL of the next pixel: K != G, K != J and d0 < d1 */ \
		_ntl = _mm_andnot_si128(_mm_or_si128(_eq_cf, (EQ_EF)), xbrz_ ## w ## _gt_sse(_d1, _d0)); \
		_ntld = _mm_andnot_si128(_nd, _ntl); \
		/* bottom vertex with F = PE, G = PF, J = PH, K = PI */ \
		_d0 = xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse((DEG), (DEC)), \
				xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse(interp_ ## fmt ## _dist_sse((H5), (PI)), interp_ ## fmt ## _dist_sse((PI), (F4))), xbrz_ ## w ## _mul4_sse(_dfh))); \
		_d1 = xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse(interp_ ## fmt ## _dist_sse((PD), (PH)), interp_ ## fmt ## _dist_sse((PH), (I5))), \
				xbrz_ ## w ## _add_sse(xbrz_ ## w ## _add_sse(_dbf, interp_ ## fmt ## _dist_sse((PF), (I4))), xbrz_ ## w ## _mul4_sse(DEI))); \
		_nd = xbrz_ ## w ## _nodom_sse(xbrz_ ## w ## _min_sse(_d0, _d1), xbrz_ ## w ## _max_sse(_d0, _d1)); \
		/* bot.tl is the BR of PE: F != G, F != J and d0 < d1 */ \
		(BR) = _mm_andnot_si128(_mm_or_si128((EQ_EF), _eq_eh), xbrz_ ## w ## _gt_sse(_d1, _d0)); \
		(BRD) = _mm_andnot_si128(_nd, (BR)); \
		/* bot.tr is the BL of the next pixel: G != F, G != K and d1 < d0 */ \
		_nbl = _mm_andnot_si128(_mm_or_si128((EQ_EF), _eq_fi), xbrz_ ## w ## _gt_sse(_d0, _d1)); \
		_nbld = _mm_andnot_si128(_nd, _nbl); \
		/* the left vertices are the right ones of the previous pixel */ \
		(TL) = xbrz_ ## w ## _prev_sse(_ntl, (PTL)); \
		(TLD) = xbrz_ ## w ## _prev_sse(_ntld, (PTLD)); \
		(BL) = xbrz_ ## w ## _prev_sse(_nbl, (PBL)); \
		(BLD) = xbrz_ ## w ## _prev_sse(_nbld, (PBLD)); \
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
#define XBRZ_LINE_SSE(fmt, w, B, C, D, E, F, G, H, I, DEF, DEH, DEG, DEC, DEI, DGH, DHI, DIF, DFC, TR, BL, BR, BRD, LINE, PX, SHALLOW, STEEP) \
	do { \
		const __m128i _eq = xbrz_ ## w ## _set_sse(XBRZ_EQUAL_ ## fmt - 1); \
		__m128i _ring = xbrz_ ## w ## _max_sse(xbrz_ ## w ## _max_sse((DGH), (DHI)), xbrz_ ## w ## _max_sse((DIF), (DFC))); \
		__m128i _no = _mm_or_si128(_mm_and_si128((TR), xbrz_ ## w ## _gt_sse((DEG), _eq)), _mm_and_si128((BL), xbrz_ ## w ## _gt_sse((DEC), _eq))); \
		__m128i _fg = interp_ ## fmt ## _dist_sse((F), (G)); \
		__m128i _hc = interp_ ## fmt ## _dist_sse((H), (C)); \
		__m128i _fg12 = xbrz_ ## w ## _mul12_sse(_fg); \
		__m128i _hc12 = xbrz_ ## w ## _mul12_sse(_hc); \
		__m128i _fg5 = xbrz_ ## w ## _mul5_sse(_fg); \
		__m128i _hc5 = xbrz_ ## w ## _mul5_sse(_hc); \
		/* the adjacent corner flags protect small features and L shapes */ \
		_no = _mm_or_si128(_no, _mm_andnot_si128(xbrz_ ## w ## _gt_sse(_ring, _eq), xbrz_ ## w ## _gt_sse((DEI), _eq))); \
		(LINE) = _mm_or_si128((BRD), _mm_andnot_si128(_no, (BR))); \
		(PX) = _mm_blendv_epi8((F), (H), xbrz_ ## w ## _gt_sse((DEF), (DEH))); \
		(SHALLOW) = _mm_andnot_si128(_mm_or_si128(xbrz_ ## w ## _gt_sse(_fg12, _hc5), \
				_mm_or_si128(xbrz_ ## w ## _eq_sse((E), (G)), xbrz_ ## w ## _eq_sse((D), (G)))), (LINE)); \
		(STEEP) = _mm_andnot_si128(_mm_or_si128(xbrz_ ## w ## _gt_sse(_hc12, _fg5), \
				_mm_or_si128(xbrz_ ## w ## _eq_sse((E), (C)), xbrz_ ## w ## _eq_sse((B), (C)))), (LINE)); \
	} while (0)

/**
 * Distances used only by the blending: the four orthogonal neighbors of PE
 * and the eight edges of the ring around PE. They are computed after the
 * early exit, and only once for the four rotations.
 */
#define XBRZ_DIST_SSE(fmt, PA, PB, PC, PD, PE, PF, PG, PH, PI, DEB, DED, DEF, DEH, DAB, DBC, DCF, DFI, DHI, DGH, DDG, DAD) \
	do { \
		(DEB) = interp_ ## fmt ## _dist_sse((PE), (PB)); \
		(DED) = interp_ ## fmt ## _dist_sse((PE), (PD)); \
		(DEF) = interp_ ## fmt ## _dist_sse((PE), (PF)); \
		(DEH) = interp_ ## fmt ## _dist_sse((PE), (PH)); \
		(DAB) = interp_ ## fmt ## _dist_sse((PA), (PB)); \
		(DBC) = interp_ ## fmt ## _dist_sse((PB), (PC)); \
		(DCF) = interp_ ## fmt ## _dist_sse((PC), (PF)); \
		(DFI) = interp_ ## fmt ## _dist_sse((PF), (PI)); \
		(DHI) = interp_ ## fmt ## _dist_sse((PH), (PI)); \
		(DGH) = interp_ ## fmt ## _dist_sse((PG), (PH)); \
		(DDG) = interp_ ## fmt ## _dist_sse((PD), (PG)); \
		(DAD) = interp_ ## fmt ## _dist_sse((PA), (PD)); \
	} while (0)

/**
 * Initialize the state carried between SIMD blocks with the right vertices
 * of the pixel x - 1, i.e. the left vertices of the first pixel of the block.
 * Only the last lane is used by the alignr of XBRZ_CLASSIFY_SSE.
 */
#define XBRZ_START_SSE(fmt, w, MASK, src0, src1, src2, src3, src4, count, x, PTL, PTLD, PBL, PBLD) \
	do { \
		unsigned _sxm2 = (x) >= 2 ? (x) - 2 : 0; \
		unsigned _sxm1 = (x) >= 1 ? (x) - 1 : 0; \
		unsigned _sxp1 = (count) - (x) >= 2 ? (x) + 1 : (count) - 1; \
		unsigned _stl, _sbl; \
		XBRZ_EDGE_LEFT(fmt, MASK, src0, src1, src2, src3, src4, _sxm2, _sxm1, x, _sxp1, _stl, _sbl); \
		(PTL) = xbrz_ ## w ## _flag_sse(_stl != XBRZ_NONE); \
		(PTLD) = xbrz_ ## w ## _flag_sse(_stl == XBRZ_DOMINANT); \
		(PBL) = xbrz_ ## w ## _flag_sse(_sbl != XBRZ_NONE); \
		(PBLD) = xbrz_ ## w ## _flag_sse(_sbl == XBRZ_DOMINANT); \
	} while (0)

#endif

#endif

