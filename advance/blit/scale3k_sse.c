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

#include "scale3k.h"

#ifdef CONFIG_X86

#include "interp_sse.h"

/***************************************************************************/
/* Scale3k SSE4.1 implementation */

/*
 * Vector equivalent of one scalar SCALE3K rotation. Branch masks are
 * mutually exclusive. The square-block branch takes precedence even when
 * its no-star condition is false. Rotations update the same output vectors
 * in the same order as the scalar implementation.
 *
 * Output arguments are modifiable vector expressions.
 */
#define SCALE3K_SSE(fmt, eq, A, B, C, D, E, F, G, H, I, N0, N1, N2, N3, N4, N5, N6, N7, N8) \
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
			__m128i _square_horz = _mm_or_si128(_square, _horizontal); \
			__m128i _square_vert = _mm_or_si128(_square, _vertical); \
			__m128i _horiz_vert = _mm_or_si128(_horizontal, _vertical); \
			__m128i _old0 = (N0), _old1 = (N1), _old2 = (N2), _old3 = (N3), _old6 = (N6); \
			__m128i _new0 = _mm_blendv_epi8(_old0, interp_ ## fmt ## _31_sse((D), _old0), _square); \
			_new0 = _mm_blendv_epi8(_new0, (D), _horiz_vert); \
			(N0) = _mm_blendv_epi8(_new0, interp_ ## fmt ## _71_sse((D), _old0), _diagonal); \
			__m128i _new1 = _mm_blendv_epi8(_old1, interp_ ## fmt ## _31_sse((D), _old1), _square_horz); \
			_new1 = _mm_blendv_epi8(_new1, interp_ ## fmt ## _31_sse(_old1, (D)), _vertical); \
			(N1) = _mm_blendv_epi8(_new1, interp_ ## fmt ## _71_sse(_old1, (D)), _diagonal); \
			(N2) = _mm_blendv_epi8(_old2, interp_ ## fmt ## _31_sse(_old2, (D)), _square_horz); \
			__m128i _new3 = _mm_blendv_epi8(_old3, interp_ ## fmt ## _31_sse((D), _old3), _square_vert); \
			_new3 = _mm_blendv_epi8(_new3, interp_ ## fmt ## _31_sse(_old3, (D)), _horizontal); \
			(N3) = _mm_blendv_epi8(_new3, interp_ ## fmt ## _71_sse(_old3, (D)), _diagonal); \
			(N6) = _mm_blendv_epi8(_old6, interp_ ## fmt ## _31_sse(_old6, (D)), _square_vert); \
		} \
	} while (0)

static inline void scale3k_16_565_sse_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
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

	dst0[3 * x + 0] = E0;
	dst0[3 * x + 1] = E1;
	dst0[3 * x + 2] = E2;
	dst1[3 * x + 0] = E3;
	dst1[3 * x + 1] = E4;
	dst1[3 * x + 2] = E5;
	dst2[3 * x + 0] = E6;
	dst2[3 * x + 1] = E7;
	dst2[3 * x + 2] = E8;
}

__attribute__((target("sse4.1"))) void scale3k_16_565_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First pixel uses a clamped left neighbor. */
	scale3k_16_565_sse_edge_pixel(dst0, dst1, dst2, src0, src1, src2, count, x++);

	/* 8 source pixels per SIMD block; the last lane needs a right neighbor. */
	for (; count - x > 8; x += 8) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		__m128i E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_SSE(16_565, _mm_cmpeq_epi16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_SSE(16_565, _mm_cmpeq_epi16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_SSE(16_565, _mm_cmpeq_epi16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_SSE(16_565, _mm_cmpeq_epi16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		/* Three sequential packed stores per destination row, no scratch buffer. */
		interp_16_565_store3_sse(dst0 + 3 * x, E0, E1, E2);
		interp_16_565_store3_sse(dst1 + 3 * x, E3, E4, E5);
		interp_16_565_store3_sse(dst2 + 3 * x, E6, E7, E8);
	}

	/* Scalar tail, including the right border. */
	for (; x < count; ++x)
		scale3k_16_565_sse_edge_pixel(dst0, dst1, dst2, src0, src1, src2, count, x);
}

static inline void scale3k_16_555_sse_edge_pixel(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2,
	const interp_uint16* src0, const interp_uint16* src1, const interp_uint16* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
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

	dst0[3 * x + 0] = E0;
	dst0[3 * x + 1] = E1;
	dst0[3 * x + 2] = E2;
	dst1[3 * x + 0] = E3;
	dst1[3 * x + 1] = E4;
	dst1[3 * x + 2] = E5;
	dst2[3 * x + 0] = E6;
	dst2[3 * x + 1] = E7;
	dst2[3 * x + 2] = E8;
}

__attribute__((target("sse4.1"))) void scale3k_16_555_sse41(interp_uint16* restrict dst0, interp_uint16* restrict dst1,
	interp_uint16* restrict dst2,
	const interp_uint16* restrict src0, const interp_uint16* restrict src1, const interp_uint16* restrict src2,
	unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First pixel uses a clamped left neighbor. */
	scale3k_16_555_sse_edge_pixel(dst0, dst1, dst2, src0, src1, src2, count, x++);

	/* 8 source pixels per SIMD block; the last lane needs a right neighbor. */
	for (; count - x > 8; x += 8) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		__m128i E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_SSE(16_555, _mm_cmpeq_epi16, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_SSE(16_555, _mm_cmpeq_epi16, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_SSE(16_555, _mm_cmpeq_epi16, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_SSE(16_555, _mm_cmpeq_epi16, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		/* Three sequential packed stores per destination row, no scratch buffer. */
		interp_16_555_store3_sse(dst0 + 3 * x, E0, E1, E2);
		interp_16_555_store3_sse(dst1 + 3 * x, E3, E4, E5);
		interp_16_555_store3_sse(dst2 + 3 * x, E6, E7, E8);
	}

	/* Scalar tail, including the right border. */
	for (; x < count; ++x)
		scale3k_16_555_sse_edge_pixel(dst0, dst1, dst2, src0, src1, src2, count, x);
}

static inline void scale3k_32_sse_edge_pixel(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	interp_uint32* restrict dst2,
	const interp_uint32* src0, const interp_uint32* src1, const interp_uint32* src2,
	unsigned count, unsigned x)
{
	unsigned left = x != 0 ? x - 1 : 0;
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

	dst0[3 * x + 0] = E0;
	dst0[3 * x + 1] = E1;
	dst0[3 * x + 2] = E2;
	dst1[3 * x + 0] = E3;
	dst1[3 * x + 1] = E4;
	dst1[3 * x + 2] = E5;
	dst2[3 * x + 0] = E6;
	dst2[3 * x + 1] = E7;
	dst2[3 * x + 2] = E8;
}

__attribute__((target("sse4.1"))) void scale3k_32_sse41(interp_uint32* restrict dst0, interp_uint32* restrict dst1,
	interp_uint32* restrict dst2,
	const interp_uint32* restrict src0, const interp_uint32* restrict src1, const interp_uint32* restrict src2,
	unsigned count)
{
	unsigned x = 0;

	if (count == 0)
		return;

	/* First pixel uses a clamped left neighbor. */
	scale3k_32_sse_edge_pixel(dst0, dst1, dst2, src0, src1, src2, count, x++);

	/* 4 source pixels per SIMD block; the last lane needs a right neighbor. */
	for (; count - x > 4; x += 4) {
#define LD(row, off) _mm_loadu_si128((const __m128i*)(const void*)((row) + x + (off)))
		__m128i A = LD(src0, -1), B = LD(src0, 0), C = LD(src0, 1);
		__m128i D = LD(src1, -1), E = LD(src1, 0), F = LD(src1, 1);
		__m128i G = LD(src2, -1), H = LD(src2, 0), I = LD(src2, 1);
#undef LD
		__m128i E0 = E, E1 = E, E2 = E, E3 = E, E4 = E;
		__m128i E5 = E, E6 = E, E7 = E, E8 = E;

		SCALE3K_SSE(32, _mm_cmpeq_epi32, A, B, C, D, E, F, G, H, I, E0, E1, E2, E3, E4, E5, E6, E7, E8);
		SCALE3K_SSE(32, _mm_cmpeq_epi32, G, D, A, H, E, B, I, F, C, E6, E3, E0, E7, E4, E1, E8, E5, E2);
		SCALE3K_SSE(32, _mm_cmpeq_epi32, I, H, G, F, E, D, C, B, A, E8, E7, E6, E5, E4, E3, E2, E1, E0);
		SCALE3K_SSE(32, _mm_cmpeq_epi32, C, F, I, B, E, H, A, D, G, E2, E5, E8, E1, E4, E7, E0, E3, E6);

		/* Three sequential packed stores per destination row, no scratch buffer. */
		interp_32_store3_sse(dst0 + 3 * x, E0, E1, E2);
		interp_32_store3_sse(dst1 + 3 * x, E3, E4, E5);
		interp_32_store3_sse(dst2 + 3 * x, E6, E7, E8);
	}

	/* Scalar tail, including the right border. */
	for (; x < count; ++x)
		scale3k_32_sse_edge_pixel(dst0, dst1, dst2, src0, src1, src2, count, x);
}

#endif

