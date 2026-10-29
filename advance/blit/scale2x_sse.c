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

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

/***************************************************************************/
/* Scale2x SSE2 implementation */

#if defined(CONFIG_X86)

/*
 * Center row of Scale2x3/Scale2x4. The outer pixels use clamped neighbors;
 * vector loads are restricted to the interior of all three source rows.
 */
static inline __attribute__((target("sse2"))) void scale2x_8_sse2_center(scale2x_uint8* dst, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && ((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]))) ? src1[1] : src1[0];

	while (count - x > 16) {
		__m128i b = _mm_loadu_si128((const __m128i*)(src0 + x));
		__m128i h = _mm_loadu_si128((const __m128i*)(src2 + x));
		__m128i d = _mm_loadu_si128((const __m128i*)(src1 + x - 1));
		__m128i f = _mm_loadu_si128((const __m128i*)(src1 + x + 1));
		__m128i e = _mm_loadu_si128((const __m128i*)(src1 + x));
		__m128i active = _mm_or_si128(_mm_cmpeq_epi8(b, h), _mm_cmpeq_epi8(d, f));
		__m128i left = _mm_or_si128(
			_mm_andnot_si128(_mm_cmpeq_epi8(e, _mm_loadu_si128((const __m128i*)(src2 + x - 1))), _mm_cmpeq_epi8(d, b)),
			_mm_andnot_si128(_mm_cmpeq_epi8(e, _mm_loadu_si128((const __m128i*)(src0 + x - 1))), _mm_cmpeq_epi8(d, h)));
		__m128i right = _mm_or_si128(
			_mm_andnot_si128(_mm_cmpeq_epi8(e, _mm_loadu_si128((const __m128i*)(src2 + x + 1))), _mm_cmpeq_epi8(f, b)),
			_mm_andnot_si128(_mm_cmpeq_epi8(e, _mm_loadu_si128((const __m128i*)(src0 + x + 1))), _mm_cmpeq_epi8(f, h)));
		left = _mm_andnot_si128(active, left);
		right = _mm_andnot_si128(active, right);
		left = _mm_or_si128(_mm_and_si128(left, d), _mm_andnot_si128(left, e));
		right = _mm_or_si128(_mm_and_si128(right, f), _mm_andnot_si128(right, e));
		_mm_storeu_si128((__m128i*)(dst + 2 * x), _mm_unpacklo_epi8(left, right));
		_mm_storeu_si128((__m128i*)(dst + 2 * x + 16), _mm_unpackhi_epi8(left, right));
		x += 16;
	}
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

static inline __attribute__((target("sse2"))) void scale2x_16_sse2_center(scale2x_uint16* dst, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && ((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]))) ? src1[1] : src1[0];

	while (count - x > 8) {
		__m128i b = _mm_loadu_si128((const __m128i*)(src0 + x));
		__m128i h = _mm_loadu_si128((const __m128i*)(src2 + x));
		__m128i d = _mm_loadu_si128((const __m128i*)(src1 + x - 1));
		__m128i f = _mm_loadu_si128((const __m128i*)(src1 + x + 1));
		__m128i e = _mm_loadu_si128((const __m128i*)(src1 + x));
		__m128i active = _mm_or_si128(_mm_cmpeq_epi16(b, h), _mm_cmpeq_epi16(d, f));
		__m128i left = _mm_or_si128(
			_mm_andnot_si128(_mm_cmpeq_epi16(e, _mm_loadu_si128((const __m128i*)(src2 + x - 1))), _mm_cmpeq_epi16(d, b)),
			_mm_andnot_si128(_mm_cmpeq_epi16(e, _mm_loadu_si128((const __m128i*)(src0 + x - 1))), _mm_cmpeq_epi16(d, h)));
		__m128i right = _mm_or_si128(
			_mm_andnot_si128(_mm_cmpeq_epi16(e, _mm_loadu_si128((const __m128i*)(src2 + x + 1))), _mm_cmpeq_epi16(f, b)),
			_mm_andnot_si128(_mm_cmpeq_epi16(e, _mm_loadu_si128((const __m128i*)(src0 + x + 1))), _mm_cmpeq_epi16(f, h)));
		left = _mm_andnot_si128(active, left);
		right = _mm_andnot_si128(active, right);
		left = _mm_or_si128(_mm_and_si128(left, d), _mm_andnot_si128(left, e));
		right = _mm_or_si128(_mm_and_si128(right, f), _mm_andnot_si128(right, e));
		_mm_storeu_si128((__m128i*)(dst + 2 * x), _mm_unpacklo_epi16(left, right));
		_mm_storeu_si128((__m128i*)(dst + 2 * x + 8), _mm_unpackhi_epi16(left, right));
		x += 8;
	}
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

static inline __attribute__((target("sse2"))) void scale2x_32_sse2_center(scale2x_uint32* dst, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && ((src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]))) ? src1[1] : src1[0];

	while (count - x > 4) {
		__m128i b = _mm_loadu_si128((const __m128i*)(src0 + x));
		__m128i h = _mm_loadu_si128((const __m128i*)(src2 + x));
		__m128i d = _mm_loadu_si128((const __m128i*)(src1 + x - 1));
		__m128i f = _mm_loadu_si128((const __m128i*)(src1 + x + 1));
		__m128i e = _mm_loadu_si128((const __m128i*)(src1 + x));
		__m128i active = _mm_or_si128(_mm_cmpeq_epi32(b, h), _mm_cmpeq_epi32(d, f));
		__m128i left = _mm_or_si128(
			_mm_andnot_si128(_mm_cmpeq_epi32(e, _mm_loadu_si128((const __m128i*)(src2 + x - 1))), _mm_cmpeq_epi32(d, b)),
			_mm_andnot_si128(_mm_cmpeq_epi32(e, _mm_loadu_si128((const __m128i*)(src0 + x - 1))), _mm_cmpeq_epi32(d, h)));
		__m128i right = _mm_or_si128(
			_mm_andnot_si128(_mm_cmpeq_epi32(e, _mm_loadu_si128((const __m128i*)(src2 + x + 1))), _mm_cmpeq_epi32(f, b)),
			_mm_andnot_si128(_mm_cmpeq_epi32(e, _mm_loadu_si128((const __m128i*)(src0 + x + 1))), _mm_cmpeq_epi32(f, h)));
		left = _mm_andnot_si128(active, left);
		right = _mm_andnot_si128(active, right);
		left = _mm_or_si128(_mm_and_si128(left, d), _mm_andnot_si128(left, e));
		right = _mm_or_si128(_mm_and_si128(right, f), _mm_andnot_si128(right, e));
		_mm_storeu_si128((__m128i*)(dst + 2 * x), _mm_unpacklo_epi32(left, right));
		_mm_storeu_si128((__m128i*)(dst + 2 * x + 4), _mm_unpackhi_epi32(left, right));
		x += 4;
	}
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
static inline __attribute__((target("sse2"))) void scale2x_8_sse2_border(scale2x_uint8* dst, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && src1[1] == src0[0]) ? src1[1] : src1[0];

	/* Keep both horizontal neighbors inside the source row. */
	while (count - x > 16) {
		__m128i b = _mm_loadu_si128((const __m128i*)(src0 + x));
		__m128i h = _mm_loadu_si128((const __m128i*)(src2 + x));
		__m128i d = _mm_loadu_si128((const __m128i*)(src1 + x - 1));
		__m128i f = _mm_loadu_si128((const __m128i*)(src1 + x + 1));
		__m128i e = _mm_loadu_si128((const __m128i*)(src1 + x));
		__m128i inactive = _mm_or_si128(_mm_cmpeq_epi8(b, h), _mm_cmpeq_epi8(d, f));
		__m128i left = _mm_andnot_si128(inactive, _mm_cmpeq_epi8(d, b));
		__m128i right = _mm_andnot_si128(inactive, _mm_cmpeq_epi8(f, b));
		left = _mm_or_si128(_mm_and_si128(left, b), _mm_andnot_si128(left, e));
		right = _mm_or_si128(_mm_and_si128(right, b), _mm_andnot_si128(right, e));
		_mm_storeu_si128((__m128i*)(dst + 2 * x), _mm_unpacklo_epi8(left, right));
		_mm_storeu_si128((__m128i*)(dst + 2 * x + 16), _mm_unpackhi_epi8(left, right));
		x += 16;
	}
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

static inline __attribute__((target("sse2"))) void scale2x_16_sse2_border(scale2x_uint16* dst, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && src1[1] == src0[0]) ? src1[1] : src1[0];

	/* Keep both horizontal neighbors inside the source row. */
	while (count - x > 8) {
		__m128i b = _mm_loadu_si128((const __m128i*)(src0 + x));
		__m128i h = _mm_loadu_si128((const __m128i*)(src2 + x));
		__m128i d = _mm_loadu_si128((const __m128i*)(src1 + x - 1));
		__m128i f = _mm_loadu_si128((const __m128i*)(src1 + x + 1));
		__m128i e = _mm_loadu_si128((const __m128i*)(src1 + x));
		__m128i inactive = _mm_or_si128(_mm_cmpeq_epi16(b, h), _mm_cmpeq_epi16(d, f));
		__m128i left = _mm_andnot_si128(inactive, _mm_cmpeq_epi16(d, b));
		__m128i right = _mm_andnot_si128(inactive, _mm_cmpeq_epi16(f, b));
		left = _mm_or_si128(_mm_and_si128(left, b), _mm_andnot_si128(left, e));
		right = _mm_or_si128(_mm_and_si128(right, b), _mm_andnot_si128(right, e));
		_mm_storeu_si128((__m128i*)(dst + 2 * x), _mm_unpacklo_epi16(left, right));
		_mm_storeu_si128((__m128i*)(dst + 2 * x + 8), _mm_unpackhi_epi16(left, right));
		x += 8;
	}
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

static inline __attribute__((target("sse2"))) void scale2x_32_sse2_border(scale2x_uint32* dst, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	unsigned x = 1;
	assert(count >= 2);

	/* first pixel: the left neighbor is the pixel itself. */
	dst[0] = src1[0];
	dst[1] = (src0[0] != src2[0] && src1[0] != src1[1] && src1[1] == src0[0]) ? src1[1] : src1[0];

	/* Keep both horizontal neighbors inside the source row. */
	while (count - x > 4) {
		__m128i b = _mm_loadu_si128((const __m128i*)(src0 + x));
		__m128i h = _mm_loadu_si128((const __m128i*)(src2 + x));
		__m128i d = _mm_loadu_si128((const __m128i*)(src1 + x - 1));
		__m128i f = _mm_loadu_si128((const __m128i*)(src1 + x + 1));
		__m128i e = _mm_loadu_si128((const __m128i*)(src1 + x));
		__m128i inactive = _mm_or_si128(_mm_cmpeq_epi32(b, h), _mm_cmpeq_epi32(d, f));
		__m128i left = _mm_andnot_si128(inactive, _mm_cmpeq_epi32(d, b));
		__m128i right = _mm_andnot_si128(inactive, _mm_cmpeq_epi32(f, b));
		left = _mm_or_si128(_mm_and_si128(left, b), _mm_andnot_si128(left, e));
		right = _mm_or_si128(_mm_and_si128(right, b), _mm_andnot_si128(right, e));
		_mm_storeu_si128((__m128i*)(dst + 2 * x), _mm_unpacklo_epi32(left, right));
		_mm_storeu_si128((__m128i*)(dst + 2 * x + 4), _mm_unpackhi_epi32(left, right));
		x += 4;
	}
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
 * \param count Length in pixels of the src0, src1 and src2 rows. It must be at least 2.
 * \param dst0 First destination row, double length in pixels.
 * \param dst1 Second destination row, double length in pixels.
 */
void scale2x_8_sse2(scale2x_uint8* dst0, scale2x_uint8* dst1, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	scale2x_8_sse2_border(dst0, src0, src1, src2, count);
	scale2x_8_sse2_border(dst1, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2 a row of pixels of 16 bits.
 * This function operates like scale2x_8_sse2() but for 16 bits pixels.
 * \param src0 Pointer at the first pixel of the previous row.
 * \param src1 Pointer at the first pixel of the current row.
 * \param src2 Pointer at the first pixel of the next row.
 * \param count Length in pixels of the src0, src1 and src2 rows. It must be at least 2.
 * \param dst0 First destination row, double length in pixels.
 * \param dst1 Second destination row, double length in pixels.
 */
void scale2x_16_sse2(scale2x_uint16* dst0, scale2x_uint16* dst1, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	scale2x_16_sse2_border(dst0, src0, src1, src2, count);
	scale2x_16_sse2_border(dst1, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2 a row of pixels of 32 bits.
 * This function operates like scale2x_8_sse2() but for 32 bits pixels.
 * \param src0 Pointer at the first pixel of the previous row.
 * \param src1 Pointer at the first pixel of the current row.
 * \param src2 Pointer at the first pixel of the next row.
 * \param count Length in pixels of the src0, src1 and src2 rows. It must be at least 2.
 * \param dst0 First destination row, double length in pixels.
 * \param dst1 Second destination row, double length in pixels.
 */
void scale2x_32_sse2(scale2x_uint32* dst0, scale2x_uint32* dst1, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	scale2x_32_sse2_border(dst0, src0, src1, src2, count);
	scale2x_32_sse2_border(dst1, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x3 a row of pixels of 8 bits.
 * This function operates like scale2x_8_sse2() but with an expansion
 * factor of 2x3 instead of 2x2.
 */
void scale2x3_8_sse2(scale2x_uint8* dst0, scale2x_uint8* dst1, scale2x_uint8* dst2, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	scale2x_8_sse2_border(dst0, src0, src1, src2, count);
	scale2x_8_sse2_center(dst1, src0, src1, src2, count);
	scale2x_8_sse2_border(dst2, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x3 a row of pixels of 16 bits.
 * This function operates like scale2x_16_sse2() but with an expansion
 * factor of 2x3 instead of 2x2.
 */
void scale2x3_16_sse2(scale2x_uint16* dst0, scale2x_uint16* dst1, scale2x_uint16* dst2, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	scale2x_16_sse2_border(dst0, src0, src1, src2, count);
	scale2x_16_sse2_center(dst1, src0, src1, src2, count);
	scale2x_16_sse2_border(dst2, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x3 a row of pixels of 32 bits.
 * This function operates like scale2x_32_sse2() but with an expansion
 * factor of 2x3 instead of 2x2.
 */
void scale2x3_32_sse2(scale2x_uint32* dst0, scale2x_uint32* dst1, scale2x_uint32* dst2, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	scale2x_32_sse2_border(dst0, src0, src1, src2, count);
	scale2x_32_sse2_center(dst1, src0, src1, src2, count);
	scale2x_32_sse2_border(dst2, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x4 a row of pixels of 8 bits.
 * This function operates like scale2x_8_sse2() but with an expansion
 * factor of 2x4 instead of 2x2.
 */
void scale2x4_8_sse2(scale2x_uint8* dst0, scale2x_uint8* dst1, scale2x_uint8* dst2, scale2x_uint8* dst3, const scale2x_uint8* src0, const scale2x_uint8* src1, const scale2x_uint8* src2, unsigned count)
{
	scale2x_8_sse2_border(dst0, src0, src1, src2, count);
	scale2x_8_sse2_center(dst1, src0, src1, src2, count);
	scale2x_8_sse2_center(dst2, src0, src1, src2, count);
	scale2x_8_sse2_border(dst3, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x4 a row of pixels of 16 bits.
 * This function operates like scale2x_16_sse2() but with an expansion
 * factor of 2x4 instead of 2x2.
 */
void scale2x4_16_sse2(scale2x_uint16* dst0, scale2x_uint16* dst1, scale2x_uint16* dst2, scale2x_uint16* dst3, const scale2x_uint16* src0, const scale2x_uint16* src1, const scale2x_uint16* src2, unsigned count)
{
	scale2x_16_sse2_border(dst0, src0, src1, src2, count);
	scale2x_16_sse2_center(dst1, src0, src1, src2, count);
	scale2x_16_sse2_center(dst2, src0, src1, src2, count);
	scale2x_16_sse2_border(dst3, src2, src1, src0, count);
}

/**
 * Scale by a factor of 2x4 a row of pixels of 32 bits.
 * This function operates like scale2x_32_sse2() but with an expansion
 * factor of 2x4 instead of 2x2.
 */
void scale2x4_32_sse2(scale2x_uint32* dst0, scale2x_uint32* dst1, scale2x_uint32* dst2, scale2x_uint32* dst3, const scale2x_uint32* src0, const scale2x_uint32* src1, const scale2x_uint32* src2, unsigned count)
{
	scale2x_32_sse2_border(dst0, src0, src1, src2, count);
	scale2x_32_sse2_center(dst1, src0, src1, src2, count);
	scale2x_32_sse2_center(dst2, src0, src1, src2, count);
	scale2x_32_sse2_border(dst3, src2, src1, src0, count);
}

#endif


