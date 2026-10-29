/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2008 Andrea Mazzoleni
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

#ifndef __IRGB_H
#define __IRGB_H

#include "icommon.h"

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

/***************************************************************************/
/* internal_rgb_raw */

#if defined(CONFIG_X86)

static inline __attribute__((target("sse2"))) void internal_rgb_raw128_012carry_sse2(void* dst, const void* src, const void* mask, const void* carry, unsigned count)
{
	const __m128i mask0 = _mm_loadu_si128((const __m128i*)((const uint8*)mask + 0));
	const __m128i mask1 = _mm_loadu_si128((const __m128i*)((const uint8*)mask + 16));
	const __m128i mask2 = _mm_loadu_si128((const __m128i*)((const uint8*)mask + 32));
	const __m128i carry_mask = _mm_loadu_si128((const __m128i*)carry);

	while (count) {
		__m128i pixel = _mm_loadu_si128((const __m128i*)src);
		__m128i part0 = _mm_and_si128(pixel, mask0);
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), mask1);
		__m128i part2 = _mm_and_si128(_mm_srli_epi64(pixel, 2), mask2);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(part0, part1), part2), _mm_and_si128(_mm_and_si128(pixel, _mm_srli_epi64(pixel, 1)), carry_mask)));
		PADD(src, 16);
		PADD(dst, 16);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw128_01_sse2(void* dst, const void* src, const void* mask, unsigned count)
{
	const __m128i mask0 = _mm_loadu_si128((const __m128i*)((const uint8*)mask + 0));
	const __m128i mask1 = _mm_loadu_si128((const __m128i*)((const uint8*)mask + 16));

	while (count) {
		__m128i pixel = _mm_loadu_si128((const __m128i*)src);
		__m128i part0 = _mm_and_si128(pixel, mask0);
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), mask1);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(part0, part1));
		PADD(src, 16);
		PADD(dst, 16);
		--count;
	}
}

#define internal_rgb_raw128_12carry_sse2 internal_rgb_raw128_012carry_sse2
#define internal_rgb_raw128_1_sse2 internal_rgb_raw128_01_sse2
#define internal_rgb_raw128_2_sse2 internal_rgb_raw128_01_sse2

static inline __attribute__((target("sse2"))) void internal_rgb_raw64_012carry_sse2(void* dst, const void* src, const void* mask, const void* carry, unsigned count)
{
	const __m128i mask0 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 0));
	const __m128i mask1 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 8));
	const __m128i mask2 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 16));
	const __m128i carry_mask = _mm_loadl_epi64((const __m128i*)carry);

	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part0 = _mm_and_si128(pixel, mask0);
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), mask1);
		__m128i part2 = _mm_and_si128(_mm_srli_epi64(pixel, 2), mask2);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(part0, part1), part2), _mm_and_si128(_mm_and_si128(pixel, _mm_srli_epi64(pixel, 1)), carry_mask)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw64_01_sse2(void* dst, const void* src, const void* mask, unsigned count)
{
	const __m128i mask0 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 0));
	const __m128i mask1 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 8));

	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part0 = _mm_and_si128(pixel, mask0);
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), mask1);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(part0, part1));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw64_02_sse2(void* dst, const void* src, const void* mask, unsigned count)
{
	const __m128i mask0 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 0));
	const __m128i mask2 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 16));

	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part0 = _mm_and_si128(pixel, mask0);
		__m128i part2 = _mm_and_si128(_mm_srli_epi64(pixel, 2), mask2);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(part0, part2));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw64_12carry_sse2(void* dst, const void* src, const void* mask, void* carry, unsigned count)
{
	const __m128i mask1 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 8));
	const __m128i mask2 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 16));
	const __m128i carry_mask = _mm_loadl_epi64((const __m128i*)carry);

	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), mask1);
		__m128i part2 = _mm_and_si128(_mm_srli_epi64(pixel, 2), mask2);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(_mm_add_epi32(part1, part2), _mm_and_si128(_mm_and_si128(pixel, _mm_srli_epi64(pixel, 1)), carry_mask)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw64_1_sse2(void* dst, const void* src, const void* mask, unsigned count)
{
	const __m128i mask1 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 8));

	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), mask1);
		_mm_storel_epi64((__m128i*)dst, part1);
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw64_2_sse2(void* dst, const void* src, const void* mask, unsigned count)
{
	const __m128i mask2 = _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 16));

	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part2 = _mm_and_si128(_mm_srli_epi64(pixel, 2), mask2);
		_mm_storel_epi64((__m128i*)dst, part2);
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline __attribute__((target("sse2"))) void internal_rgb_raw64x3_012_sse2(void* dst, const void* src, const void* mask, unsigned count)
{
	unsigned phase = 0;
	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		__m128i part0 = _mm_and_si128(pixel, _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 0 + phase * 8)));
		__m128i part1 = _mm_and_si128(_mm_srli_epi64(pixel, 1), _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 24 + phase * 8)));
		__m128i part2 = _mm_and_si128(_mm_srli_epi64(pixel, 2), _mm_loadl_epi64((const __m128i*)((const uint8*)mask + 48 + phase * 8)));
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(_mm_add_epi32(part0, part1), part2));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
		if (++phase == 3)
			phase = 0;
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_raw128_012carry_neon(void* dst, const void* src, const void* mask, const void* carry, unsigned count)
{
	if (!count)
		return;
	const uint32x4_t mask0 = vld1q_u32((const uint32*)mask + 0);
	const uint32x4_t mask1 = vld1q_u32((const uint32*)mask + 4);
	const uint32x4_t mask2 = vld1q_u32((const uint32*)mask + 8);
	const uint32x4_t correction = vld1q_u32(carry);
	while (count) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vaddq_u32(vaddq_u32(vaddq_u32(vandq_u32(pixel, mask0), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1)), mask1)), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 2)), mask2)), vandq_u32(vandq_u32(pixel, vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1))), correction)));
		PADD(src, 16);
		PADD(dst, 16);
		--count;
	}
}

static inline void internal_rgb_raw128_01_neon(void* dst, const void* src, const void* mask, unsigned count)
{
	if (!count)
		return;
	const uint32x4_t mask0 = vld1q_u32((const uint32*)mask + 0);
	const uint32x4_t mask1 = vld1q_u32((const uint32*)mask + 4);
	while (count) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vaddq_u32(vandq_u32(pixel, mask0), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1)), mask1)));
		PADD(src, 16);
		PADD(dst, 16);
		--count;
	}
}

#define internal_rgb_raw128_12carry_neon internal_rgb_raw128_012carry_neon
#define internal_rgb_raw128_1_neon internal_rgb_raw128_01_neon
#define internal_rgb_raw128_2_neon internal_rgb_raw128_01_neon

static inline void internal_rgb_raw64_012carry_neon(void* dst, const void* src, const void* mask, const void* carry, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask0 = vld1_u32((const uint32*)mask + 0);
	const uint32x2_t mask1 = vld1_u32((const uint32*)mask + 2);
	const uint32x2_t mask2 = vld1_u32((const uint32*)mask + 4);
	const uint32x2_t correction = vld1_u32(carry);
#if defined(CONFIG_NEON)
	/* AArch64 handles two repeated mask blocks per iteration. */
	const uint32x4_t pair_mask0 = vcombine_u32(mask0, mask0);
	const uint32x4_t pair_mask1 = vcombine_u32(mask1, mask1);
	const uint32x4_t pair_mask2 = vcombine_u32(mask2, mask2);
	const uint32x4_t pair_correction = vcombine_u32(correction, correction);
	while (count >= 2) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vaddq_u32(vaddq_u32(vaddq_u32(vandq_u32(pixel, pair_mask0), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1)), pair_mask1)), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 2)), pair_mask2)), vandq_u32(vandq_u32(pixel, vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1))), pair_correction)));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
#endif
	while (count) {
		uint32x2_t pixel = vld1_u32(src);
		vst1_u32(dst, vadd_u32(vadd_u32(vadd_u32(vand_u32(pixel, mask0), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask1)), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask2)), vand_u32(vand_u32(pixel, vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1))), correction)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_rgb_raw64_01_neon(void* dst, const void* src, const void* mask, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask0 = vld1_u32((const uint32*)mask + 0);
	const uint32x2_t mask1 = vld1_u32((const uint32*)mask + 2);
#if defined(CONFIG_NEON)
	/* AArch64 handles two repeated mask blocks per iteration. */
	const uint32x4_t pair_mask0 = vcombine_u32(mask0, mask0);
	const uint32x4_t pair_mask1 = vcombine_u32(mask1, mask1);
	while (count >= 2) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vaddq_u32(vandq_u32(pixel, pair_mask0), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1)), pair_mask1)));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
#endif
	while (count) {
		uint32x2_t pixel = vld1_u32(src);
		vst1_u32(dst, vadd_u32(vand_u32(pixel, mask0), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask1)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_rgb_raw64_02_neon(void* dst, const void* src, const void* mask, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask0 = vld1_u32((const uint32*)mask + 0);
	const uint32x2_t mask2 = vld1_u32((const uint32*)mask + 4);
#if defined(CONFIG_NEON)
	/* AArch64 handles two repeated mask blocks per iteration. */
	const uint32x4_t pair_mask0 = vcombine_u32(mask0, mask0);
	const uint32x4_t pair_mask2 = vcombine_u32(mask2, mask2);
	while (count >= 2) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vaddq_u32(vandq_u32(pixel, pair_mask0), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 2)), pair_mask2)));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
#endif
	while (count) {
		uint32x2_t pixel = vld1_u32(src);
		vst1_u32(dst, vadd_u32(vand_u32(pixel, mask0), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask2)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_rgb_raw64_12carry_neon(void* dst, const void* src, const void* mask, void* carry, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask1 = vld1_u32((const uint32*)mask + 2);
	const uint32x2_t mask2 = vld1_u32((const uint32*)mask + 4);
	const uint32x2_t correction = vld1_u32(carry);
#if defined(CONFIG_NEON)
	/* AArch64 handles two repeated mask blocks per iteration. */
	const uint32x4_t pair_mask1 = vcombine_u32(mask1, mask1);
	const uint32x4_t pair_mask2 = vcombine_u32(mask2, mask2);
	const uint32x4_t pair_correction = vcombine_u32(correction, correction);
	while (count >= 2) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vaddq_u32(vaddq_u32(vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1)), pair_mask1), vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 2)), pair_mask2)), vandq_u32(vandq_u32(pixel, vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1))), pair_correction)));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
#endif
	while (count) {
		uint32x2_t pixel = vld1_u32(src);
		vst1_u32(dst, vadd_u32(vadd_u32(vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask1), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask2)), vand_u32(vand_u32(pixel, vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1))), correction)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_rgb_raw64_1_neon(void* dst, const void* src, const void* mask, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask1 = vld1_u32((const uint32*)mask + 2);
#if defined(CONFIG_NEON)
	/* AArch64 handles two repeated mask blocks per iteration. */
	const uint32x4_t pair_mask1 = vcombine_u32(mask1, mask1);
	while (count >= 2) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 1)), pair_mask1));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
#endif
	while (count) {
		uint32x2_t pixel = vld1_u32(src);
		vst1_u32(dst, vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask1));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_rgb_raw64_2_neon(void* dst, const void* src, const void* mask, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask2 = vld1_u32((const uint32*)mask + 4);
#if defined(CONFIG_NEON)
	/* AArch64 handles two repeated mask blocks per iteration. */
	const uint32x4_t pair_mask2 = vcombine_u32(mask2, mask2);
	while (count >= 2) {
		uint32x4_t pixel = vld1q_u32(src);
		vst1q_u32(dst, vandq_u32(vreinterpretq_u32_u64(vshrq_n_u64(vreinterpretq_u64_u32(pixel), 2)), pair_mask2));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
#endif
	while (count) {
		uint32x2_t pixel = vld1_u32(src);
		vst1_u32(dst, vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask2));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_rgb_raw64x3_012_neon(void* dst, const void* src, const void* mask, unsigned count)
{
	if (!count)
		return;
	const uint32x2_t mask00 = vld1_u32((const uint32*)mask + 0);
	const uint32x2_t mask01 = vld1_u32((const uint32*)mask + 6);
	const uint32x2_t mask02 = vld1_u32((const uint32*)mask + 12);
	const uint32x2_t mask10 = vld1_u32((const uint32*)mask + 2);
	const uint32x2_t mask11 = vld1_u32((const uint32*)mask + 8);
	const uint32x2_t mask12 = vld1_u32((const uint32*)mask + 14);
	const uint32x2_t mask20 = vld1_u32((const uint32*)mask + 4);
	const uint32x2_t mask21 = vld1_u32((const uint32*)mask + 10);
	const uint32x2_t mask22 = vld1_u32((const uint32*)mask + 16);
	for (;;) {
		{
			uint32x2_t pixel = vld1_u32(src);
			vst1_u32(dst, vadd_u32(vadd_u32(vand_u32(pixel, mask00), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask01)), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask02)));
		}
		PADD(src, 8);
		PADD(dst, 8);
		if (!--count)
			break;
		{
			uint32x2_t pixel = vld1_u32(src);
			vst1_u32(dst, vadd_u32(vadd_u32(vand_u32(pixel, mask10), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask11)), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask12)));
		}
		PADD(src, 8);
		PADD(dst, 8);
		if (!--count)
			break;
		{
			uint32x2_t pixel = vld1_u32(src);
			vst1_u32(dst, vadd_u32(vadd_u32(vand_u32(pixel, mask20), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 1)), mask21)), vand_u32(vreinterpret_u32_u64(vshr_n_u64(vreinterpret_u64_u32(pixel), 2)), mask22)));
		}
		PADD(src, 8);
		PADD(dst, 8);
		if (!--count)
			break;
	}
}

#endif

/* TODO Carry version not implemented */
#define internal_rgb_raw32_012carry_def internal_rgb_raw32_012_def

static inline void internal_rgb_raw32_012_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_01_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

/* TODO Carry version not implemented */
#define internal_rgb_raw32_12carry_def internal_rgb_raw32_12_def

static inline void internal_rgb_raw32_12_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_1_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 >> 1) & mask[2];
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_2_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 >> 2) & mask[4];
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_02_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32x2_012_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[3]) + ((v1 >> 2) & mask[5]);
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x2_01_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[3]);
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]);
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x2_12_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		v1 = ((v1 >> 1) & mask[3]) + ((v1 >> 2) & mask[5]);
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x2_1_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = (v0 >> 1) & mask[2];
		v1 = (v1 >> 1) & mask[3];
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 >> 1) & mask[2];
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x3_012_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 3) {
		unsigned v0, v1, v2;
		v0 = src32[0];
		v1 = src32[1];
		v2 = src32[2];

		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[6]) + ((v0 >> 2) & mask[12]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[7]) + ((v1 >> 2) & mask[13]);
		v2 = (v2 & mask[2]) + ((v2 >> 1) & mask[8]) + ((v2 >> 2) & mask[14]);

		dst32[0] = v0;
		dst32[1] = v1;
		dst32[2] = v2;

		dst32 += 3;
		src32 += 3;
		count -= 3;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[6]) + ((v0 >> 2) & mask[12]);
		dst32[0] = v0;
	} else if (count == 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];

		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[6]) + ((v0 >> 2) & mask[12]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[7]) + ((v1 >> 2) & mask[13]);

		dst32[0] = v0;
		dst32[1] = v1;
	}
}

/***************************************************************************/
/* rgb_compute */

static unsigned rgb_raw_pixel_mask_compute(const struct video_pipeline_target_struct* target, unsigned shift, unsigned sub_mask)
{
	unsigned v;
	unsigned red_mask;
	unsigned green_mask;
	unsigned blue_mask;
	int red_shift;
	int green_shift;
	int blue_shift;
	adv_pixel low_mask;

	union adv_color_def_union def;

	def.ordinal = target->color_def;

	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	v = 0;

	/* red */
	if (sub_mask & 0x4)
		v |= red_mask & (red_mask >> shift);

	/* green */
	if (sub_mask & 0x2)
		v |= green_mask & (green_mask >> shift);

	/* blue */
	if (sub_mask & 0x1)
		v |= blue_mask & (blue_mask >> shift);

	return v;
}

static unsigned byte_raw_pixel_mask_compute(const struct video_pipeline_target_struct* target, unsigned shift, unsigned sub_mask)
{
	unsigned v;

	v = 0;

	if (sub_mask & 0x1)
		v |= 0xFFU & (0xFFU >> shift);

	if (sub_mask & 0x2)
		v |= 0xFF00U & (0xFF00U >> shift);

	if (sub_mask & 0x4)
		v |= 0xFF0000U & (0xFF0000U >> shift);

	if (sub_mask & 0x8)
		v |= 0xFF000000U & (0xFF000000U >> shift);

	return v;
}

static unsigned rgb_raw_carry_mask_compute(const struct video_pipeline_target_struct* target, unsigned sub_mask)
{
	unsigned v;
	unsigned red_mask;
	unsigned green_mask;
	unsigned blue_mask;
	int red_shift;
	int green_shift;
	int blue_shift;
	adv_pixel low_mask;

	union adv_color_def_union def;

	def.ordinal = target->color_def;

	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	low_mask = rgb_lowmask_make_from_def(target->color_def);

	v = 0;

	/* red */
	if (sub_mask & 0x4)
		v |= low_mask & red_mask;

	/* green */
	if (sub_mask & 0x2)
		v |= low_mask & green_mask;

	/* blue */
	if (sub_mask & 0x1)
		v |= low_mask & blue_mask;

	return v;
}

static void rgb_raw_mask4_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask >> 12);
	unsigned pixel1 = rgb_raw_pixel_mask_compute(target, shift, mask >> 8);
	unsigned pixel2 = rgb_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel3 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel2, pixel3);
		dst[1] = dst[0];
		dst[2] = dst[0];
		dst[3] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = cpu_uint32_make_uint16(pixel2, pixel3);
		dst[2] = dst[0];
		dst[3] = dst[1];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		dst[2] = pixel2;
		dst[3] = pixel3;
		break;
	default :
		assert(0);
	}
}

static void rgb_raw_carry4_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned mask)
{
	unsigned pixel0 = rgb_raw_carry_mask_compute(target, mask >> 12);
	unsigned pixel1 = rgb_raw_carry_mask_compute(target, mask >> 8);
	unsigned pixel2 = rgb_raw_carry_mask_compute(target, mask >> 4);
	unsigned pixel3 = rgb_raw_carry_mask_compute(target, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel2, pixel3);
		dst[1] = dst[0];
		dst[2] = dst[0];
		dst[3] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = cpu_uint32_make_uint16(pixel2, pixel3);
		dst[2] = dst[0];
		dst[3] = dst[1];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		dst[2] = pixel2;
		dst[3] = pixel3;
		break;
	default :
		assert(0);
	}
}

static void rgb_raw_mask3_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask >> 8);
	unsigned pixel1 = rgb_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel2 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel2, pixel0);
		dst[1] = cpu_uint32_make_uint8(pixel1, pixel2, pixel0, pixel1);
		dst[2] = cpu_uint32_make_uint8(pixel2, pixel0, pixel1, pixel2);
		dst[3] = dst[0];
		dst[4] = dst[1];
		dst[5] = dst[2];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = cpu_uint32_make_uint16(pixel2, pixel0);
		dst[2] = cpu_uint32_make_uint16(pixel1, pixel2);
		dst[3] = dst[0];
		dst[4] = dst[1];
		dst[5] = dst[2];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		dst[2] = pixel2;
		dst[3] = dst[0];
		dst[4] = dst[1];
		dst[5] = dst[2];
		break;
	default :
		assert(0);
	}
}

static void byte_raw_mask3_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = byte_raw_pixel_mask_compute(target, shift, mask >> 8);
	unsigned pixel1 = byte_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel2 = byte_raw_pixel_mask_compute(target, shift, mask);

	dst[0] = pixel0;
	dst[1] = pixel1;
	dst[2] = pixel2;
	dst[3] = dst[0];
	dst[4] = dst[1];
	dst[5] = dst[2];
}

static void rgb_raw_mask2_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel1 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		break;
	default :
		assert(0);
	}
}

static void byte_raw_mask2_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = byte_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel1 = byte_raw_pixel_mask_compute(target, shift, mask);

	dst[0] = pixel0;
	dst[1] = pixel1;
}

static void rgb_raw_carry2_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned mask)
{
	unsigned pixel0 = rgb_raw_carry_mask_compute(target, mask >> 4);
	unsigned pixel1 = rgb_raw_carry_mask_compute(target, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		break;
	default :
		assert(0);
	}
}

static void rgb_raw_mask1_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel0, pixel0, pixel0);
		dst[1] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel0);
		dst[1] = dst[0];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = dst[0];
		break;
	default :
		assert(0);
	}
}

static void byte_raw_mask1_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = byte_raw_pixel_mask_compute(target, shift, mask);

	dst[0] = pixel0;
	dst[1] = dst[0];
}

/***************************************************************************/
/* internal_rgb_triad16pix */

/*
   This mask is applyed at the image:

   .... (type 0)
   GBR. (type 1)
   GBR.
   GBR.
   .... (type 0)
   .GBR (type 2)
   .GBR
   .GBR

   . = red:75%, green:75%, blue:75%
   G = red:75%, green:100%, blue:75%
   B = red:75%, green:75%, blue:100%
   R = red:100%, green:75%, blue:75%
 */

enum RGB_TRIAD16PIX_MASK {
	RGB_TRIAD16PIX_MASK_0_0_0, /* type 0 shift 0 addr 0 */
	RGB_TRIAD16PIX_MASK_0_0_1, /* type 0 shift 0 addr 1 */
	RGB_TRIAD16PIX_MASK_0_0_2, /* type 0 shift 0 addr 2 */
	RGB_TRIAD16PIX_MASK_0_0_3, /* type 0 shift 0 addr 3 */
	RGB_TRIAD16PIX_MASK_0_1_0, /* type 0 shift 1 addr 0 */
	RGB_TRIAD16PIX_MASK_0_1_1, /* type 0 shift 1 addr 1 */
	RGB_TRIAD16PIX_MASK_0_1_2, /* type 0 shift 1 addr 2 */
	RGB_TRIAD16PIX_MASK_0_1_3, /* type 0 shift 1 addr 3 */
	RGB_TRIAD16PIX_MASK_0_2_0, /* ... */
	RGB_TRIAD16PIX_MASK_0_2_1,
	RGB_TRIAD16PIX_MASK_0_2_2, /* ... */
	RGB_TRIAD16PIX_MASK_0_2_3,
	RGB_TRIAD16PIX_MASK_0_c_0, /* type 0 carry addr 0 */
	RGB_TRIAD16PIX_MASK_0_c_1, /* type 0 carry addr 1 */
	RGB_TRIAD16PIX_MASK_0_c_2, /* type 0 carry addr 2 */
	RGB_TRIAD16PIX_MASK_0_c_3, /* type 0 carry addr 3 */
	RGB_TRIAD16PIX_MASK_1_0_0,
	RGB_TRIAD16PIX_MASK_1_0_1,
	RGB_TRIAD16PIX_MASK_1_0_2,
	RGB_TRIAD16PIX_MASK_1_0_3,
	RGB_TRIAD16PIX_MASK_1_1_0,
	RGB_TRIAD16PIX_MASK_1_1_1,
	RGB_TRIAD16PIX_MASK_1_1_2,
	RGB_TRIAD16PIX_MASK_1_1_3,
	RGB_TRIAD16PIX_MASK_1_2_0,
	RGB_TRIAD16PIX_MASK_1_2_1,
	RGB_TRIAD16PIX_MASK_1_2_2,
	RGB_TRIAD16PIX_MASK_1_2_3,
	RGB_TRIAD16PIX_MASK_1_c_0,
	RGB_TRIAD16PIX_MASK_1_c_1,
	RGB_TRIAD16PIX_MASK_1_c_2,
	RGB_TRIAD16PIX_MASK_1_c_3,
	RGB_TRIAD16PIX_MASK_2_0_0,
	RGB_TRIAD16PIX_MASK_2_0_1,
	RGB_TRIAD16PIX_MASK_2_0_2,
	RGB_TRIAD16PIX_MASK_2_0_3,
	RGB_TRIAD16PIX_MASK_2_1_0,
	RGB_TRIAD16PIX_MASK_2_1_1,
	RGB_TRIAD16PIX_MASK_2_1_2,
	RGB_TRIAD16PIX_MASK_2_1_3,
	RGB_TRIAD16PIX_MASK_2_2_0,
	RGB_TRIAD16PIX_MASK_2_2_1,
	RGB_TRIAD16PIX_MASK_2_2_2,
	RGB_TRIAD16PIX_MASK_2_2_3,
	RGB_TRIAD16PIX_MASK_2_c_0,
	RGB_TRIAD16PIX_MASK_2_c_1,
	RGB_TRIAD16PIX_MASK_2_c_2,
	RGB_TRIAD16PIX_MASK_2_c_3,
	RGB_TRIAD16PIX_MASK_MAX
};

static void internal_rgb_triad16pix_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	/* type 0 */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_0_0_0, 0, 0x0000); /* factor 2^0 = 100% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_0_1_0, 1, 0x7777); /* factor 2^-1 = 50% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_0_2_0, 2, 0x7777); /* factor 2^-2 = 25% */
	rgb_raw_carry4_compute(target, data + RGB_TRIAD16PIX_MASK_0_c_0, 0x7777); /* carry */

	/* type 1 */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_1_0_0, 0, 0x2140); /* factor 2^0 = 100% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_1_1_0, 1, 0x5637); /* factor 2^-1 = 50% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_1_2_0, 2, 0x5637); /* factor 2^-2 = 25% */
	rgb_raw_carry4_compute(target, data + RGB_TRIAD16PIX_MASK_1_c_0, 0x5637); /* carry */

	/* type 2 */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_2_0_0, 0, 0x0214); /* factor 2^0 = 100% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_2_1_0, 1, 0x7563); /* factor 2^-1 = 50% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_2_2_0, 2, 0x7563); /* factor 2^-2 = 25% */
	rgb_raw_carry4_compute(target, data + RGB_TRIAD16PIX_MASK_2_c_0, 0x7563); /* carry */
}

#if defined(CONFIG_X86)

static inline void internal_rgb_triad16pix8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 16;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 16);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 16);
		break;
	default :
		internal_rgb_raw128_012carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 16);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 16);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 16;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 16);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 16);
		break;
	default :
		internal_rgb_raw128_01_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 16);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 16);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad16pix16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 8);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 8);
		break;
	default :
		internal_rgb_raw128_012carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 8);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 8);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 8);
		break;
	default :
		internal_rgb_raw128_01_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 8);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad16pix32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 4);
		break;
	default :
		internal_rgb_raw128_012carry_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 4);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw128_01_sse2(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_triad16pix8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 16;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 16);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 16);
		break;
	default :
		internal_rgb_raw128_012carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 16);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 16);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 16;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_neon(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 16);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_neon(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 16);
		break;
	default :
		internal_rgb_raw128_01_neon(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 16);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 16);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad16pix16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 8);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 8);
		break;
	default :
		internal_rgb_raw128_012carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 8);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_neon(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 8);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_neon(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 8);
		break;
	default :
		internal_rgb_raw128_01_neon(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 8);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad16pix32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 4);
		break;
	default :
		internal_rgb_raw128_012carry_neon(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 4);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_neon(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_neon(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw128_01_neon(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[16]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_triad16pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32_12carry_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad16pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2 ? count % 4 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_12_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 2);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_012_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 2);
		break;
	default :
		internal_rgb_raw32x2_012_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 2);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad16pix16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong16pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2 ? count % 4 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_1_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 2);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 2);
		break;
	default :
		internal_rgb_raw32x2_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 2);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong16pix16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad16pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_12_def(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_012_def(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count);
		break;
	default :
		internal_rgb_raw32x2_012_def(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_1_def(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_01_def(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count);
		break;
	default :
		internal_rgb_raw32x2_01_def(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count);
		break;
	}
}

/***************************************************************************/
/* internal_rgb_triad6pix */

/*
   This mask is applyed at the image:

   BR (type 0)
   BG (type 1)
   RG (type 2)
   RB (type 3)
   GB (type 4)
   GR (type 5)

   G = red:75%, green:100%, blue:75%
   B = red:75%, green:75%, blue:100%
   R = red:100%, green:75%, blue:75%
 */

enum RGB_TRIAD6PIX_MASK {
	RGB_TRIAD6PIX_MASK_0_0_0, /* type 0 shift 0 addr 0 */
	RGB_TRIAD6PIX_MASK_0_0_1, /* type 0 shift 0 addr 1 */
	RGB_TRIAD6PIX_MASK_0_1_0, /* type 0 shift 1 addr 0 */
	RGB_TRIAD6PIX_MASK_0_1_1, /* type 0 shift 1 addr 1 */
	RGB_TRIAD6PIX_MASK_0_2_0, /* ... */
	RGB_TRIAD6PIX_MASK_0_2_1,
	RGB_TRIAD6PIX_MASK_0_c_0,
	RGB_TRIAD6PIX_MASK_0_c_1,
	RGB_TRIAD6PIX_MASK_1_0_0,
	RGB_TRIAD6PIX_MASK_1_0_1,
	RGB_TRIAD6PIX_MASK_1_1_0,
	RGB_TRIAD6PIX_MASK_1_1_1,
	RGB_TRIAD6PIX_MASK_1_2_0,
	RGB_TRIAD6PIX_MASK_1_2_1,
	RGB_TRIAD6PIX_MASK_1_c_0,
	RGB_TRIAD6PIX_MASK_1_c_1,
	RGB_TRIAD6PIX_MASK_2_0_0,
	RGB_TRIAD6PIX_MASK_2_0_1,
	RGB_TRIAD6PIX_MASK_2_1_0,
	RGB_TRIAD6PIX_MASK_2_1_1,
	RGB_TRIAD6PIX_MASK_2_2_0,
	RGB_TRIAD6PIX_MASK_2_2_1,
	RGB_TRIAD6PIX_MASK_2_c_0,
	RGB_TRIAD6PIX_MASK_2_c_1,
	RGB_TRIAD6PIX_MASK_3_0_0,
	RGB_TRIAD6PIX_MASK_3_0_1,
	RGB_TRIAD6PIX_MASK_3_1_0,
	RGB_TRIAD6PIX_MASK_3_1_1,
	RGB_TRIAD6PIX_MASK_3_2_0,
	RGB_TRIAD6PIX_MASK_3_2_1,
	RGB_TRIAD6PIX_MASK_3_c_0,
	RGB_TRIAD6PIX_MASK_3_c_1,
	RGB_TRIAD6PIX_MASK_4_0_0,
	RGB_TRIAD6PIX_MASK_4_0_1,
	RGB_TRIAD6PIX_MASK_4_1_0,
	RGB_TRIAD6PIX_MASK_4_1_1,
	RGB_TRIAD6PIX_MASK_4_2_0,
	RGB_TRIAD6PIX_MASK_4_2_1,
	RGB_TRIAD6PIX_MASK_4_c_0,
	RGB_TRIAD6PIX_MASK_4_c_1,
	RGB_TRIAD6PIX_MASK_5_0_0,
	RGB_TRIAD6PIX_MASK_5_0_1,
	RGB_TRIAD6PIX_MASK_5_1_0,
	RGB_TRIAD6PIX_MASK_5_1_1,
	RGB_TRIAD6PIX_MASK_5_2_0,
	RGB_TRIAD6PIX_MASK_5_2_1,
	RGB_TRIAD6PIX_MASK_5_c_0,
	RGB_TRIAD6PIX_MASK_5_c_1,
	RGB_TRIAD6PIX_MASK_MAX
};

static void internal_rgb_triad6pix_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	/* type 0 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_0_0_0, 0, 0x14); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_0_1_0, 1, 0x63); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_0_2_0, 2, 0x63); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_0_c_0, 0x63); /* carry */

	/* type 1 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_1_0_0, 0, 0x12); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_1_1_0, 1, 0x65); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_1_2_0, 2, 0x65); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_1_c_0, 0x65); /* carry */

	/* type 2 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_2_0_0, 0, 0x42); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_2_1_0, 1, 0x35); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_2_2_0, 2, 0x35); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_2_c_0, 0x35); /* carry */

	/* type 3 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_3_0_0, 0, 0x41); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_3_1_0, 1, 0x36); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_3_2_0, 2, 0x36); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_3_c_0, 0x36); /* carry */

	/* type 4 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_4_0_0, 0, 0x21); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_4_1_0, 1, 0x56); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_4_2_0, 2, 0x56); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_4_c_0, 0x56); /* carry */

	/* type 5 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_5_0_0, 0, 0x24); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_5_1_0, 1, 0x53); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_5_2_0, 2, 0x53); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_5_c_0, 0x53); /* carry */
}

#if defined(CONFIG_X86)

static inline void internal_rgb_triad6pix8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_sse2(dst, src, mask, carry, count / 8);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_sse2(dst, src, mask, count / 8);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad6pix16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_sse2(dst, src, mask, carry, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_sse2(dst, src, mask, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad6pix32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_sse2(dst, src, mask, carry, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_sse2(dst, src, mask, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_triad6pix8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_neon(dst, src, mask, carry, count / 8);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_neon(dst, src, mask, count / 8);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad6pix16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_neon(dst, src, mask, carry, count / 4);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_neon(dst, src, mask, count / 4);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad6pix32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_neon(dst, src, mask, carry, count / 2);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_neon(dst, src, mask, count / 2);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_triad6pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad6pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad6pix16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong6pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong6pix16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad6pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_012carry_def(dst, src, mask, count);
}

static inline void internal_rgb_triadstrong6pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_01_def(dst, src, mask, count);
}

/***************************************************************************/
/* internal_rgb_triad3pix */

/*
   This mask is applyed at the image:

   RB (type 0)
   GR (type 1)
   BG (type 2)

   R = red:100%, green:75%, blue:75%
   G = red:75%, green:100%, blue:75%
   B = red:75%, green:75%, blue:100%
 */

enum RGB_TRIAD3PIX_MASK {
	RGB_TRIAD3PIX_MASK_0_0_0, /* type 0 shift 0 addr 0 */
	RGB_TRIAD3PIX_MASK_0_0_1, /* type 0 shift 0 addr 1 */
	RGB_TRIAD3PIX_MASK_0_1_0, /* type 0 shift 1 addr 0 */
	RGB_TRIAD3PIX_MASK_0_1_1, /* type 0 shift 1 addr 1 */
	RGB_TRIAD3PIX_MASK_0_2_0, /* ... */
	RGB_TRIAD3PIX_MASK_0_2_1,
	RGB_TRIAD3PIX_MASK_0_c_0,
	RGB_TRIAD3PIX_MASK_0_c_1,
	RGB_TRIAD3PIX_MASK_1_0_0,
	RGB_TRIAD3PIX_MASK_1_0_1,
	RGB_TRIAD3PIX_MASK_1_1_0,
	RGB_TRIAD3PIX_MASK_1_1_1,
	RGB_TRIAD3PIX_MASK_1_2_0,
	RGB_TRIAD3PIX_MASK_1_2_1,
	RGB_TRIAD3PIX_MASK_1_c_0,
	RGB_TRIAD3PIX_MASK_1_c_1,
	RGB_TRIAD3PIX_MASK_2_0_0,
	RGB_TRIAD3PIX_MASK_2_0_1,
	RGB_TRIAD3PIX_MASK_2_1_0,
	RGB_TRIAD3PIX_MASK_2_1_1,
	RGB_TRIAD3PIX_MASK_2_2_0,
	RGB_TRIAD3PIX_MASK_2_2_1,
	RGB_TRIAD3PIX_MASK_2_c_0,
	RGB_TRIAD3PIX_MASK_2_c_1,
	RGB_TRIAD3PIX_MASK_MAX
};

static void internal_rgb_triad3pix_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	/* type 0 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_0_0_0, 0, 0x41); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_0_1_0, 1, 0x36); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_0_2_0, 2, 0x36); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD3PIX_MASK_0_c_0, 0x36); /* carry */

	/* type 1 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_1_0_0, 0, 0x24); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_1_1_0, 1, 0x53); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_1_2_0, 2, 0x53); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD3PIX_MASK_1_c_0, 0x53); /* carry */

	/* type 2 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_2_0_0, 0, 0x12); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_2_1_0, 1, 0x65); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_2_2_0, 2, 0x65); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD3PIX_MASK_2_c_0, 0x65); /* carry */
}

#if defined(CONFIG_X86)

static inline void internal_rgb_triad3pix8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_sse2(dst, src, mask, carry, count / 8);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw64_01_sse2(dst, src, mask, count / 8);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad3pix16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_sse2(dst, src, mask, carry, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw64_01_sse2(dst, src, mask, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad3pix32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_sse2(dst, src, mask, carry, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw64_01_sse2(dst, src, mask, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_triad3pix8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_neon(dst, src, mask, carry, count / 8);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw64_01_neon(dst, src, mask, count / 8);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad3pix16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_neon(dst, src, mask, carry, count / 4);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw64_01_neon(dst, src, mask, count / 4);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad3pix32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_neon(dst, src, mask, carry, count / 2);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw64_01_neon(dst, src, mask, count / 2);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_triad3pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad3pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triad3pix16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triadstrong3pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_triadstrong3pix16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_triad3pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_012carry_def(dst, src, mask, count);
}

static inline void internal_rgb_triadstrong3pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_01_def(dst, src, mask, count);
}

/***************************************************************************/
/* internal_rgb_scandouble */

enum RGB_SCANDOUBLE_MASK {
	RGB_SCANDOUBLE_MASK_0_0_0,
	RGB_SCANDOUBLE_MASK_0_0_1,
	RGB_SCANDOUBLE_MASK_0_1_0,
	RGB_SCANDOUBLE_MASK_0_1_1,
	RGB_SCANDOUBLE_MASK_1_0_0,
	RGB_SCANDOUBLE_MASK_1_0_1,
	RGB_SCANDOUBLE_MASK_1_1_0,
	RGB_SCANDOUBLE_MASK_1_1_1,
	RGB_SCANDOUBLE_MASK_MAX
};

static void internal_rgb_scandouble_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_0_0, 0, 0x7); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */

		/* type 1 */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_0_0, 0, 0x0); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_1_0, 1, 0x7); /* factor 2^-1 = 50% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_0_0, 0, 0xF); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */

		/* type 1 */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_0_0, 0, 0xA); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_1_0, 1, 0x5); /* factor 2^-1 = 50% */
	}
}

#if defined(CONFIG_X86)

static inline void internal_rgb_scandouble8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		internal_rgb_raw64_1_sse2(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 8);
	} else {
		internal_copy8_sse2(dst, src, count);
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandouble16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		internal_rgb_raw64_1_sse2(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 4);
	} else {
		internal_copy16_sse2(dst, src, count);
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandouble32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		if (data[RGB_SCANDOUBLE_MASK_1_0_0] == 0)
			internal_rgb_raw64_1_sse2(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
		else
			internal_rgb_raw64_01_sse2(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
	} else {
		internal_copy32_sse2(dst, src, count);
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_scandouble8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	if (line % 2 == 0) {
		internal_copy8_neon(dst, src, count);
		return;
	}
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		internal_rgb_raw64_1_neon(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 8);
	} else {
		internal_copy8_neon(dst, src, count);
	}

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandouble16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	if (line % 2 == 0) {
		internal_copy16_neon(dst, src, count);
		return;
	}
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		internal_rgb_raw64_1_neon(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 4);
	} else {
		internal_copy16_neon(dst, src, count);
	}

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandouble32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	if (line % 2 == 0) {
		internal_copy32_neon(dst, src, count);
		return;
	}
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		if (data[RGB_SCANDOUBLE_MASK_1_0_0] == 0)
			internal_rgb_raw64_1_neon(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
		else
			internal_rgb_raw64_01_neon(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
	} else {
		internal_copy32_neon(dst, src, count);
	}

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_scandouble8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 4);
	} else {
		internal_copy8_def(dst, src, count);
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandouble16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	if (line % 2) {
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
	} else {
		internal_copy16_def(dst, src, count);
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandouble16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandouble32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		if (data[RGB_SCANDOUBLE_MASK_1_0_0] == 0)
			internal_rgb_raw32_1_def(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count);
		else
			internal_rgb_raw32_01_def(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count);
	} else {
		internal_copy32_def(dst, src, count);
	}
}

/***************************************************************************/
/* internal_rgb_scandoublevert */

enum RGB_SCANDOUBLEVERT_MASK {
	RGB_SCANDOUBLEVERT_MASK_0_0_0,
	RGB_SCANDOUBLEVERT_MASK_0_0_1,
	RGB_SCANDOUBLEVERT_MASK_0_1_0,
	RGB_SCANDOUBLEVERT_MASK_0_1_1,
	RGB_SCANDOUBLEVERT_MASK_MAX
};

static void internal_rgb_scandoublevert_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, 0, 0x70); /* factor 2^0 = 100% */
		rgb_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_1_0, 1, 0x07); /* factor 2^-1 = 50% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, 0, 0xFA); /* factor 2^0 = 100% */
		byte_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_1_0, 1, 0x05); /* factor 2^-1 = 50% */
	}
}

#if defined(CONFIG_X86)

static inline void internal_rgb_scandoublevert8_sse2(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64_01_sse2(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 8);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert8_sse2((uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandoublevert16_sse2(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64_01_sse2(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert16_sse2((uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandoublevert32_sse2(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64_01_sse2(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert32_sse2((uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_scandoublevert8_neon(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64_01_neon(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 8);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert8_neon((uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandoublevert16_neon(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64_01_neon(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 4);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert16_neon((uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandoublevert32_neon(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64_01_neon(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 2);

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert32_neon((uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_scandoublevert8_def(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert8_def((uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandoublevert16_def(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scandoublevert16_def((uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scandoublevert32_def(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32_01_def(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count);
}

/***************************************************************************/
/* internal_rgb_scantriple */

enum RGB_SCANTRIPLE_MASK {
	RGB_SCANTRIPLE_MASK_0_0_0,
	RGB_SCANTRIPLE_MASK_0_0_1,
	RGB_SCANTRIPLE_MASK_0_1_0,
	RGB_SCANTRIPLE_MASK_0_1_1,
	RGB_SCANTRIPLE_MASK_0_2_0,
	RGB_SCANTRIPLE_MASK_0_2_1,
	RGB_SCANTRIPLE_MASK_1_0_0,
	RGB_SCANTRIPLE_MASK_1_0_1,
	RGB_SCANTRIPLE_MASK_1_1_0,
	RGB_SCANTRIPLE_MASK_1_1_1,
	RGB_SCANTRIPLE_MASK_1_2_0,
	RGB_SCANTRIPLE_MASK_1_2_1,
	RGB_SCANTRIPLE_MASK_2_0_0,
	RGB_SCANTRIPLE_MASK_2_0_1,
	RGB_SCANTRIPLE_MASK_2_1_0,
	RGB_SCANTRIPLE_MASK_2_1_1,
	RGB_SCANTRIPLE_MASK_2_2_0,
	RGB_SCANTRIPLE_MASK_2_2_1,
	RGB_SCANTRIPLE_MASK_MAX
};

static void internal_rgb_scantriple_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_0_0, 0, 0x7); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 1 */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_0_0, 0, 0x0); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_1_0, 1, 0x7); /* factor 2^-1 = 50% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 2 */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_0_0, 0, 0x0); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_2_0, 2, 0x7); /* factor 2^-2 = 25% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_0_0, 0, 0xF); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 1 */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_0_0, 0, 0xA); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_1_0, 1, 0x5); /* factor 2^-1 = 50% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 2 */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_0_0, 0, 0xA); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_2_0, 2, 0x5); /* factor 2^-2 = 25% */
	}
}

#if defined(CONFIG_X86)

static inline void internal_rgb_scantriple8_sse2(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy8_sse2(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw64_1_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 8);
		break;
	case 2 :
		internal_rgb_raw64_2_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 8);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple8_sse2(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriple16_sse2(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy16_sse2(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw64_1_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 4);
		break;
	case 2 :
		internal_rgb_raw64_2_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 4);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple16_sse2(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriple32_sse2(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy32_sse2(dst, src, count);
		break;
	case 1 :
		if (data[RGB_SCANTRIPLE_MASK_1_0_0] == 0)
			internal_rgb_raw64_1_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		else
			internal_rgb_raw64_01_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		break;
	case 2 :
		if (data[RGB_SCANTRIPLE_MASK_2_0_0] == 0)
			internal_rgb_raw64_2_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		else
			internal_rgb_raw64_02_sse2(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple32_sse2(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_scantriple8_neon(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	if (line % 3 == 0) {
		internal_copy8_neon(dst, src, count);
		return;
	}
	unsigned rest = count % 8;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy8_neon(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw64_1_neon(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 8);
		break;
	case 2 :
		internal_rgb_raw64_2_neon(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 8);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple8_neon(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 8);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriple16_neon(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	if (line % 3 == 0) {
		internal_copy16_neon(dst, src, count);
		return;
	}
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy16_neon(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw64_1_neon(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 4);
		break;
	case 2 :
		internal_rgb_raw64_2_neon(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 4);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple16_neon(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriple32_neon(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	if (line % 3 == 0) {
		internal_copy32_neon(dst, src, count);
		return;
	}
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy32_neon(dst, src, count);
		break;
	case 1 :
		if (data[RGB_SCANTRIPLE_MASK_1_0_0] == 0)
			internal_rgb_raw64_1_neon(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		else
			internal_rgb_raw64_01_neon(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		break;
	case 2 :
		if (data[RGB_SCANTRIPLE_MASK_2_0_0] == 0)
			internal_rgb_raw64_2_neon(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		else
			internal_rgb_raw64_02_neon(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		break;
	}

	if (rest) {
		union { uint32 align; uint8 bytes[8]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple32_neon(line, (uint32*)output.bytes, (const uint32*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_scantriple8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy8_def(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 4);
		break;
	case 2 :
		internal_rgb_raw32_2_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 4);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple8_def(line, (uint8*)output.bytes, (const uint8*)input.bytes, data, 4);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriple16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2;
	unsigned bulk = count - rest;
	count = bulk;

	switch (line % 3) {
	case 0 :
		internal_copy16_def(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		break;
	case 2 :
		internal_rgb_raw32_2_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		break;
	}
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[4]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriple16_def(line, (uint16*)output.bytes, (const uint16*)input.bytes, data, 2);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriple32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy32_def(dst, src, count);
		break;
	case 1 :
		if (data[RGB_SCANTRIPLE_MASK_1_0_0] == 0)
			internal_rgb_raw32_1_def(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count);
		else
			internal_rgb_raw32_01_def(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count);
		break;
	case 2 :
		if (data[RGB_SCANTRIPLE_MASK_2_0_0] == 0)
			internal_rgb_raw32_2_def(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count);
		else
			internal_rgb_raw32_02_def(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count);
		break;
	}
}

/***************************************************************************/
/* internal_rgb_scantriplevert */

enum RGB_SCANTRIPLEVERT_MASK {
	RGB_SCANTRIPLEVERT_MASK_0_0_0,
	RGB_SCANTRIPLEVERT_MASK_0_0_1,
	RGB_SCANTRIPLEVERT_MASK_0_0_2,
	RGB_SCANTRIPLEVERT_MASK_0_0_3,
	RGB_SCANTRIPLEVERT_MASK_0_0_4,
	RGB_SCANTRIPLEVERT_MASK_0_0_5,
	RGB_SCANTRIPLEVERT_MASK_0_1_0,
	RGB_SCANTRIPLEVERT_MASK_0_1_1,
	RGB_SCANTRIPLEVERT_MASK_0_1_2,
	RGB_SCANTRIPLEVERT_MASK_0_1_3,
	RGB_SCANTRIPLEVERT_MASK_0_1_4,
	RGB_SCANTRIPLEVERT_MASK_0_1_5,
	RGB_SCANTRIPLEVERT_MASK_0_2_0,
	RGB_SCANTRIPLEVERT_MASK_0_2_1,
	RGB_SCANTRIPLEVERT_MASK_0_2_2,
	RGB_SCANTRIPLEVERT_MASK_0_2_3,
	RGB_SCANTRIPLEVERT_MASK_0_2_4,
	RGB_SCANTRIPLEVERT_MASK_0_2_5,
	RGB_SCANTRIPLEVERT_MASK_MAX
};

static void internal_rgb_scantriplevert_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, 0, 0x700); /* factor 2^0 = 100% */
		rgb_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_1_0, 1, 0x070); /* factor 2^-1 = 50% */
		rgb_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_2_0, 2, 0x007); /* factor 2^-2 = 25% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, 0, 0xFAA); /* factor 2^0 = 100% */
		byte_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_1_0, 1, 0x050); /* factor 2^-1 = 50% */
		byte_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_2_0, 2, 0x005); /* factor 2^-2 = 25% */
	}
}

#if defined(CONFIG_X86)

static inline void internal_rgb_scantriplevert8_sse2(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 8 ? count % 24 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64x3_012_sse2(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 8);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[24]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert8_sse2((uint8*)output.bytes, (const uint8*)input.bytes, data, 24);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriplevert16_sse2(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4 ? count % 12 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64x3_012_sse2(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[24]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert16_sse2((uint16*)output.bytes, (const uint16*)input.bytes, data, 12);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriplevert32_sse2(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2 ? count % 6 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64x3_012_sse2(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[24]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert32_sse2((uint32*)output.bytes, (const uint32*)input.bytes, data, 6);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_scantriplevert8_neon(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 24;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64x3_012_neon(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 8);

	if (rest) {
		union { uint32 align; uint8 bytes[24]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert8_neon((uint8*)output.bytes, (const uint8*)input.bytes, data, 24);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriplevert16_neon(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 12;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64x3_012_neon(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 4);

	if (rest) {
		union { uint32 align; uint8 bytes[24]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert16_neon((uint16*)output.bytes, (const uint16*)input.bytes, data, 12);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriplevert32_neon(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 6;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw64x3_012_neon(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 2);

	if (rest) {
		union { uint32 align; uint8 bytes[24]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert32_neon((uint32*)output.bytes, (const uint32*)input.bytes, data, 6);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

#endif

static inline void internal_rgb_scantriplevert8_def(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 4 ? count % 12 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw32x3_012_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 4);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[12]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert8_def((uint8*)output.bytes, (const uint8*)input.bytes, data, 12);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriplevert16_def(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	unsigned rest = count % 2 ? count % 6 : 0;
	unsigned bulk = count - rest;
	count = bulk;

	internal_rgb_raw32x3_012_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 2);
	if (rest) {
		/* Restart only at a complete mask period; copy no padding back to the row. */
		union { uint32 align; uint8 bytes[12]; } input, output;
		memset(input.bytes, 0, sizeof(input.bytes));
		memcpy(input.bytes, src + bulk, rest * sizeof(*src));
		internal_rgb_scantriplevert16_def((uint16*)output.bytes, (const uint16*)input.bytes, data, 6);
		memcpy(dst + bulk, output.bytes, rest * sizeof(*dst));
	}
}

static inline void internal_rgb_scantriplevert32_def(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32x3_012_def(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count);
}

/***************************************************************************/
/* internal_rgb_skipdouble */

static void internal_rgb_skipdouble_set(const struct video_pipeline_target_struct* target)
{
}

#if defined(CONFIG_X86)

static inline void internal_rgb_skipdouble8_sse2(unsigned line, uint8* dst, const uint8* src, unsigned count)
{
	if (line % 2) {
		internal_zero8_sse2(dst, count);
	} else {
		internal_copy8_sse2(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble16_sse2(unsigned line, uint16* dst, const uint16* src, unsigned count)
{
	if (line % 2) {
		internal_zero16_sse2(dst, count);
	} else {
		internal_copy16_sse2(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble32_sse2(unsigned line, uint32* dst, const uint32* src, unsigned count)
{
	if (line % 2) {
		internal_zero32_sse2(dst, count);
	} else {
		internal_copy32_sse2(dst, src, count);
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_rgb_skipdouble8_neon(unsigned line, uint8* dst, const uint8* src, unsigned count)
{

	if (line % 2) {
		internal_zero8_neon(dst, count);
	} else {
		internal_copy8_neon(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble16_neon(unsigned line, uint16* dst, const uint16* src, unsigned count)
{

	if (line % 2) {
		internal_zero16_neon(dst, count);
	} else {
		internal_copy16_neon(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble32_neon(unsigned line, uint32* dst, const uint32* src, unsigned count)
{

	if (line % 2) {
		internal_zero32_neon(dst, count);
	} else {
		internal_copy32_neon(dst, src, count);
	}
}

#endif

static inline void internal_rgb_skipdouble8_def(unsigned line, uint8* dst, const uint8* src, unsigned count)
{
	if (line % 2) {
		internal_zero8_def(dst, count);
	} else {
		internal_copy8_def(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble16_def(unsigned line, uint16* dst, const uint16* src, unsigned count)
{
	if (line % 2) {
		internal_zero16_def(dst, count);
	} else {
		internal_copy16_def(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble32_def(unsigned line, uint32* dst, const uint32* src, unsigned count)
{
	if (line % 2) {
		internal_zero32_def(dst, count);
	} else {
		internal_copy32_def(dst, src, count);
	}
}

#endif

