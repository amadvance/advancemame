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

#include "scale4k.h"

#ifdef CONFIG_X86

#include "interp_sse.h"

/***************************************************************************/
/* Scale4k SSE4.1 implementation */

/*
 * Vectorized equivalent of one scalar SCALE4K rotation. Preserve the
 * square/horizontal/vertical/diagonal priority and the no-star condition.
 * Each output argument is a modifiable vector expression.
 */
#define SCALE4K_SSE(fmt, eq, A, B, C, D, E, F, G, H, I, N0, N1, N2, N3, N4, N5, N6, N7, N8, N9, N10, N11, N12, N13, N14, N15) \
	do { \
		__m128i _gate = _mm_andnot_si128(eq((B), (E)), eq((D), (B))); \
		if (!_mm_testz_si128(_gate, _gate)) { \
			__m128i _block = _mm_and_si128(eq((B), (C)), eq((D), (G))); \
			__m128i _square = _mm_andnot_si128(eq((A), (E)), _mm_and_si128(_gate, _block)); \
			__m128i _other = _mm_andnot_si128(_block, _gate); \
			__m128i _horizontal = _mm_and_si128(_other, _mm_andnot_si128(eq((C), (F)), eq((B), (C)))); \
			__m128i _remaining = _mm_andnot_si128(_horizontal, _other); \
			__m128i _vertical = _mm_and_si128(_remaining, _mm_andnot_si128(eq((G), (H)), eq((D), (G)))); \
			__m128i _diagonal = _mm_andnot_si128(_vertical, _remaining); \
			__m128i _sq_h = _mm_or_si128(_square, _horizontal); \
			__m128i _sq_v = _mm_or_si128(_square, _vertical); \
			__m128i _h_v = _mm_or_si128(_horizontal, _vertical); \
			__m128i _old0 = (N0), _old1 = (N1), _old2 = (N2), _old3 = (N3); \
			__m128i _old4 = (N4), _old5 = (N5), _old8 = (N8), _old12 = (N12); \
			(N0) = _mm_blendv_epi8(_old0, (D), _mm_or_si128(_square, _other)); \
			__m128i _new1 = _mm_blendv_epi8(_old1, (D), _sq_h); \
			_new1 = _mm_blendv_epi8(_new1, interp_ ## fmt ## _31_sse((D), _old1), _vertical); \
			(N1) = _mm_blendv_epi8(_new1, interp_ ## fmt ## _11_sse((D), _old1), _diagonal); \
			(N2) = _mm_blendv_epi8(_old2, interp_ ## fmt ## _31_sse((D), _old2), _sq_h); \
			(N3) = _mm_blendv_epi8(_old3, interp_ ## fmt ## _31_sse(_old3, (D)), _sq_h); \
			__m128i _new4 = _mm_blendv_epi8(_old4, (D), _sq_v); \
			_new4 = _mm_blendv_epi8(_new4, interp_ ## fmt ## _31_sse((D), _old4), _horizontal); \
			(N4) = _mm_blendv_epi8(_new4, interp_ ## fmt ## _11_sse((D), _old4), _diagonal); \
			__m128i _new5 = _mm_blendv_epi8(_old5, interp_ ## fmt ## _11_sse((D), _old5), _square); \
			(N5) = _mm_blendv_epi8(_new5, interp_ ## fmt ## _31_sse(_old5, (D)), _h_v); \
			(N8) = _mm_blendv_epi8(_old8, interp_ ## fmt ## _31_sse((D), _old8), _sq_v); \
			(N12) = _mm_blendv_epi8(_old12, interp_ ## fmt ## _31_sse(_old12, (D)), _sq_v); \
		} \
	} while (0)

static inline void scale4k_16_565_sse_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E;
	interp_uint16 E4 = E, E5 = E, E6 = E, E7 = E;
	interp_uint16 E8 = E, E9 = E, E10 = E, E11 = E;
	interp_uint16 E12 = E, E13 = E, E14 = E, E15 = E;

	SCALE4K(16_565, A, B, C, D, E, F, G, H, I,
		E0, E1, E2, E3, E4, E5, E6, E7,
		E8, E9, E10, E11, E12, E13, E14, E15);
	SCALE4K(16_565, G, D, A, H, E, B, I, F, C,
		E12, E8, E4, E0, E13, E9, E5, E1,
		E14, E10, E6, E2, E15, E11, E7, E3);
	SCALE4K(16_565, I, H, G, F, E, D, C, B, A,
		E15, E14, E13, E12, E11, E10, E9, E8,
		E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE4K(16_565, C, F, I, B, E, H, A, D, G,
		E3, E7, E11, E15, E2, E6, E10, E14,
		E1, E5, E9, E13, E0, E4, E8, E12);

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

__attribute__((target("sse4.1"))) void scale4k_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First pixel uses a clamped left neighbor. */
	scale4k_16_565_sse_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, count, x++);

	/* 8 source pixels per block; leave the last pixel for the scalar tail. */
	for (; count - x > 8; x += 8) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E;
		__m128i E4 = E, E5 = E, E6 = E, E7 = E;
		__m128i E8 = E, E9 = E, E10 = E, E11 = E;
		__m128i E12 = E, E13 = E, E14 = E, E15 = E;

		SCALE4K_SSE(16_565, _mm_cmpeq_epi16, A, B, C, D, E, F, G, H, I,
			E0, E1, E2, E3, E4, E5, E6, E7,
			E8, E9, E10, E11, E12, E13, E14, E15);
		SCALE4K_SSE(16_565, _mm_cmpeq_epi16, G, D, A, H, E, B, I, F, C,
			E12, E8, E4, E0, E13, E9, E5, E1,
			E14, E10, E6, E2, E15, E11, E7, E3);
		SCALE4K_SSE(16_565, _mm_cmpeq_epi16, I, H, G, F, E, D, C, B, A,
			E15, E14, E13, E12, E11, E10, E9, E8,
			E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE4K_SSE(16_565, _mm_cmpeq_epi16, C, F, I, B, E, H, A, D, G,
			E3, E7, E11, E15, E2, E6, E10, E14,
			E1, E5, E9, E13, E0, E4, E8, E12);

		/* Four packed stores per destination row, without scratch buffers. */
		interp_16_565_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_565_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_565_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_565_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}

	/* Scalar remainder, including the clamped right border. */
	for (; x < count; ++x)
		scale4k_16_565_sse_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, count, x);
}

static inline void scale4k_16_555_sse_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint16 A = src0[left], B = src0[x], C = src0[right];
	interp_uint16 D = src1[left], E = src1[x], F = src1[right];
	interp_uint16 G = src2[left], H = src2[x], I = src2[right];
	interp_uint16 E0 = E, E1 = E, E2 = E, E3 = E;
	interp_uint16 E4 = E, E5 = E, E6 = E, E7 = E;
	interp_uint16 E8 = E, E9 = E, E10 = E, E11 = E;
	interp_uint16 E12 = E, E13 = E, E14 = E, E15 = E;

	SCALE4K(16_555, A, B, C, D, E, F, G, H, I,
		E0, E1, E2, E3, E4, E5, E6, E7,
		E8, E9, E10, E11, E12, E13, E14, E15);
	SCALE4K(16_555, G, D, A, H, E, B, I, F, C,
		E12, E8, E4, E0, E13, E9, E5, E1,
		E14, E10, E6, E2, E15, E11, E7, E3);
	SCALE4K(16_555, I, H, G, F, E, D, C, B, A,
		E15, E14, E13, E12, E11, E10, E9, E8,
		E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE4K(16_555, C, F, I, B, E, H, A, D, G,
		E3, E7, E11, E15, E2, E6, E10, E14,
		E1, E5, E9, E13, E0, E4, E8, E12);

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

__attribute__((target("sse4.1"))) void scale4k_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2, interp_uint16* restrict dst3,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First pixel uses a clamped left neighbor. */
	scale4k_16_555_sse_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, count, x++);

	/* 8 source pixels per block; leave the last pixel for the scalar tail. */
	for (; count - x > 8; x += 8) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E;
		__m128i E4 = E, E5 = E, E6 = E, E7 = E;
		__m128i E8 = E, E9 = E, E10 = E, E11 = E;
		__m128i E12 = E, E13 = E, E14 = E, E15 = E;

		SCALE4K_SSE(16_555, _mm_cmpeq_epi16, A, B, C, D, E, F, G, H, I,
			E0, E1, E2, E3, E4, E5, E6, E7,
			E8, E9, E10, E11, E12, E13, E14, E15);
		SCALE4K_SSE(16_555, _mm_cmpeq_epi16, G, D, A, H, E, B, I, F, C,
			E12, E8, E4, E0, E13, E9, E5, E1,
			E14, E10, E6, E2, E15, E11, E7, E3);
		SCALE4K_SSE(16_555, _mm_cmpeq_epi16, I, H, G, F, E, D, C, B, A,
			E15, E14, E13, E12, E11, E10, E9, E8,
			E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE4K_SSE(16_555, _mm_cmpeq_epi16, C, F, I, B, E, H, A, D, G,
			E3, E7, E11, E15, E2, E6, E10, E14,
			E1, E5, E9, E13, E0, E4, E8, E12);

		/* Four packed stores per destination row, without scratch buffers. */
		interp_16_555_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_16_555_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_16_555_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_16_555_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}

	/* Scalar remainder, including the clamped right border. */
	for (; x < count; ++x)
		scale4k_16_555_sse_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, count, x);
}

static inline void scale4k_32_sse_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
	unsigned right = count - x > 1 ? x + 1 : x;
	interp_uint32 A = src0[left], B = src0[x], C = src0[right];
	interp_uint32 D = src1[left], E = src1[x], F = src1[right];
	interp_uint32 G = src2[left], H = src2[x], I = src2[right];
	interp_uint32 E0 = E, E1 = E, E2 = E, E3 = E;
	interp_uint32 E4 = E, E5 = E, E6 = E, E7 = E;
	interp_uint32 E8 = E, E9 = E, E10 = E, E11 = E;
	interp_uint32 E12 = E, E13 = E, E14 = E, E15 = E;

	SCALE4K(32, A, B, C, D, E, F, G, H, I,
		E0, E1, E2, E3, E4, E5, E6, E7,
		E8, E9, E10, E11, E12, E13, E14, E15);
	SCALE4K(32, G, D, A, H, E, B, I, F, C,
		E12, E8, E4, E0, E13, E9, E5, E1,
		E14, E10, E6, E2, E15, E11, E7, E3);
	SCALE4K(32, I, H, G, F, E, D, C, B, A,
		E15, E14, E13, E12, E11, E10, E9, E8,
		E7, E6, E5, E4, E3, E2, E1, E0);
	SCALE4K(32, C, F, I, B, E, H, A, D, G,
		E3, E7, E11, E15, E2, E6, E10, E14,
		E1, E5, E9, E13, E0, E4, E8, E12);

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

__attribute__((target("sse4.1"))) void scale4k_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	interp_uint32* restrict dst2, interp_uint32* restrict dst3,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First pixel uses a clamped left neighbor. */
	scale4k_32_sse_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, count, x++);

	/* 4 source pixels per block; leave the last pixel for the scalar tail. */
	for (; count - x > 4; x += 4) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E;
		__m128i E4 = E, E5 = E, E6 = E, E7 = E;
		__m128i E8 = E, E9 = E, E10 = E, E11 = E;
		__m128i E12 = E, E13 = E, E14 = E, E15 = E;

		SCALE4K_SSE(32, _mm_cmpeq_epi32, A, B, C, D, E, F, G, H, I,
			E0, E1, E2, E3, E4, E5, E6, E7,
			E8, E9, E10, E11, E12, E13, E14, E15);
		SCALE4K_SSE(32, _mm_cmpeq_epi32, G, D, A, H, E, B, I, F, C,
			E12, E8, E4, E0, E13, E9, E5, E1,
			E14, E10, E6, E2, E15, E11, E7, E3);
		SCALE4K_SSE(32, _mm_cmpeq_epi32, I, H, G, F, E, D, C, B, A,
			E15, E14, E13, E12, E11, E10, E9, E8,
			E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE4K_SSE(32, _mm_cmpeq_epi32, C, F, I, B, E, H, A, D, G,
			E3, E7, E11, E15, E2, E6, E10, E14,
			E1, E5, E9, E13, E0, E4, E8, E12);

		/* Four packed stores per destination row, without scratch buffers. */
		interp_32_store4_sse(dst0 + 4 * x, E0, E1, E2, E3);
		interp_32_store4_sse(dst1 + 4 * x, E4, E5, E6, E7);
		interp_32_store4_sse(dst2 + 4 * x, E8, E9, E10, E11);
		interp_32_store4_sse(dst3 + 4 * x, E12, E13, E14, E15);
	}

	/* Scalar remainder, including the clamped right border. */
	for (; x < count; ++x)
		scale4k_32_sse_edge_pixel(dst0, dst1, dst2, dst3, src0, src1, src2, count, x);
}

#endif

