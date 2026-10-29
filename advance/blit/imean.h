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

#ifndef __IMEAN_H
#define __IMEAN_H

#include "icommon.h"

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

/***************************************************************************/
/* internal_mean */

enum MEAN_MASK {
	MEAN_MASK_H_0,
	MEAN_MASK_H_1,
	MEAN_MASK_MAX
};

static uint32 mean_mask[MEAN_MASK_MAX];

static uint32 expand_nibble(unsigned bytes_per_pixel, unsigned v)
{
	switch (bytes_per_pixel) {
	case 1 :
		return v | v << 8 | v << 16 | v << 24;
	case 2 :
		return v | v << 16;
	case 4 :
		return v;
	default :
		return 0;
	}
}

static void internal_mean_set(const struct video_pipeline_target_struct* target)
{
	adv_pixel rgb_h = rgb_highmask_make_from_def(target->color_def);
	adv_pixel rgb_m = rgb_wholemask_make_from_def(target->color_def);

	unsigned bytes_per_pixel = target->bytes_per_pixel;

	mean_mask[MEAN_MASK_H_0] = expand_nibble(bytes_per_pixel, (~rgb_h) & rgb_m);
	mean_mask[MEAN_MASK_H_1] = mean_mask[MEAN_MASK_H_0];
}

/**
 * Compute the mean of two series of rgb pixels.
 * This function compute (a + b) / 2 for any rgb nibble, using the
 * the formula (a + b) / 2 = ((a ^ b) >> 1) + (a & b).
 * To extend this formula to a serie of packed nibbles the formula is
 * implemented as (((v0 ^ v1) >> 1) & MASK) + (v0 & v1) where MASK
 * is used to clear the high bit of all the packed nibbles.
 */
static inline uint32 internal_mean_value(uint32 v0, uint32 v1)
{
	return (((v0 ^ v1) >> 1) & mean_mask[MEAN_MASK_H_0]) + (v0 & v1);
}

/***************************************************************************/
/* Compute the mean of dst and src and store the result in dst */

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_mean64_vert_self_sse2(void* dst, const void* src, unsigned count)
{
	const __m128i mask = _mm_loadl_epi64((const __m128i*)mean_mask);
	/* Repeat the packed mask; shifts remain independent within each 64-bit lane. */
	const __m128i mask128 = _mm_unpacklo_epi64(mask, mask);
	while (count >= 2) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_loadu_si128((const __m128i*)dst);
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		PADD(src, 16);
		PADD(dst, 16);
		count -= 2;
	}
	if (count) {
		__m128i a = _mm_loadl_epi64((const __m128i*)src);
		__m128i b = _mm_loadl_epi64((const __m128i*)dst);
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
	}
}

static inline void internal_mean8_vert_self_sse2(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 8;
	unsigned done = count - rest;
	internal_mean64_vert_self_sse2(dst, src, count / 8);
	while (rest) {
		dst[done] = internal_mean_value(dst[done], src[done]);
		++done;
		--rest;
	}
}

static inline void internal_mean16_vert_self_sse2(uint16* dst, const uint16* src, unsigned count)
{
	unsigned rest = count % 4;
	unsigned done = count - rest;
	internal_mean64_vert_self_sse2(dst, src, count / 4);
	while (rest) {
		dst[done] = internal_mean_value(dst[done], src[done]);
		++done;
		--rest;
	}
}

static inline void internal_mean32_vert_self_sse2(uint32* dst, const uint32* src, unsigned count)
{
	unsigned rest = count % 2;
	unsigned done = count - rest;
	internal_mean64_vert_self_sse2(dst, src, count / 2);
	while (rest) {
		dst[done] = internal_mean_value(dst[done], src[done]);
		++done;
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_mean64_vert_self_neon(void* dst, const void* src, unsigned count)
{
	const uint32x2_t mask = vld1_u32(mean_mask);
	while (count) {
		uint32x2_t a = vld1_u32(src);
		uint32x2_t b = vld1_u32(dst);
		vst1_u32(dst, vadd_u32(vand_u32(vshr_n_u32(veor_u32(a, b), 1), mask), vand_u32(a, b)));
		PADD(src, 8);
		PADD(dst, 8);
		--count;
	}
}

static inline void internal_mean8_vert_self_neon(uint8* dst, const uint8* src, unsigned count)
{
	const uint8x16_t mask = vdupq_n_u8((uint8)mean_mask[MEAN_MASK_H_0]);
	while (count >= 16) {
		uint8x16_t a = vld1q_u8(src);
		uint8x16_t b = vld1q_u8(dst);
		vst1q_u8(dst, vaddq_u8(vandq_u8(vshrq_n_u8(veorq_u8(a, b), 1), mask), vandq_u8(a, b)));
		src += 16;
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst = internal_mean_value(*dst, *src++);
		++dst;
		--count;
	}
}

static inline void internal_mean16_vert_self_neon(uint16* dst, const uint16* src, unsigned count)
{
	const uint16x8_t mask = vdupq_n_u16((uint16)mean_mask[MEAN_MASK_H_0]);
	while (count >= 8) {
		uint16x8_t a = vld1q_u16(src);
		uint16x8_t b = vld1q_u16(dst);
		vst1q_u16(dst, vaddq_u16(vandq_u16(vshrq_n_u16(veorq_u16(a, b), 1), mask), vandq_u16(a, b)));
		src += 8;
		dst += 8;
		count -= 8;
	}
	while (count) {
		*dst = internal_mean_value(*dst, *src++);
		++dst;
		--count;
	}
}

static inline void internal_mean32_vert_self_neon(uint32* dst, const uint32* src, unsigned count)
{
	const uint32x4_t mask = vdupq_n_u32((uint32)mean_mask[MEAN_MASK_H_0]);
	while (count >= 4) {
		uint32x4_t a = vld1q_u32(src);
		uint32x4_t b = vld1q_u32(dst);
		vst1q_u32(dst, vaddq_u32(vandq_u32(vshrq_n_u32(veorq_u32(a, b), 1), mask), vandq_u32(a, b)));
		src += 4;
		dst += 4;
		count -= 4;
	}
	while (count) {
		*dst = internal_mean_value(*dst, *src++);
		++dst;
		--count;
	}
}

#endif

static inline void internal_mean32_vert_self_def(uint32* dst32, const uint32* src32, unsigned count)
{
	while (count) {
		dst32[0] = internal_mean_value(dst32[0], src32[0]);
		++src32;
		++dst32;
		--count;
	}
}

static inline void internal_mean8_vert_self_def(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 4;
	unsigned done = count - rest;
	internal_mean32_vert_self_def((uint32*)dst, (const uint32*)src, count / 4);
	while (rest) {
		dst[done] = internal_mean_value(dst[done], src[done]);
		++done;
		--rest;
	}
}

static inline void internal_mean16_vert_self_def(uint16* dst, const uint16* src, unsigned count)
{
	unsigned rest = count % 2;
	unsigned done = count - rest;
	internal_mean32_vert_self_def((uint32*)dst, (const uint32*)src, count / 2);
	while (rest) {
		dst[done] = internal_mean_value(dst[done], src[done]);
		++done;
		--rest;
	}
}

static inline void internal_mean8_vert_self_step(uint8* dst8, const uint8* src8, unsigned count, int step1)
{
	while (count) {
		dst8[0] = internal_mean_value(dst8[0], src8[0]);
		src8 += step1;
		++dst8;
		--count;
	}
}

static inline void internal_mean16_vert_self_step(uint16* dst16, const uint16* src16, unsigned count, int step1)
{
	while (count) {
		dst16[0] = internal_mean_value(dst16[0], src16[0]);
		PADD(src16, step1);
		++dst16;
		--count;
	}
}

static inline void internal_mean32_vert_self_step(uint32* dst32, const uint32* src32, unsigned count, int step1)
{
	while (count) {
		dst32[0] = internal_mean_value(dst32[0], src32[0]);
		PADD(src32, step1);
		++dst32;
		--count;
	}
}

/***************************************************************************/
/* Compute the mean of src and src+1 and store the result in dst */

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_mean8_horz_next_step1_sse2(uint8* dst, const uint8* src, unsigned count)
{
	const __m128i mask64 = _mm_loadl_epi64((const __m128i*)mean_mask);
	const __m128i mask128 = _mm_unpacklo_epi64(mask64, mask64);
	const __m128i edge64 = _mm_set_epi32(0, 0, 0xff000000, 0);
	const __m128i edge128 = _mm_slli_si128(edge64, 8);

	/* The shifted load includes the next pixel, so keep the last vector separate. */
	while (count > 16) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_loadu_si128((const __m128i*)(src + 1));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		src += 16;
		dst += 16;
		count -= 16;
	}
	if (count == 16) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_or_si128(_mm_srli_si128(a, 1), _mm_and_si128(a, edge128));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		return;
	}
	/* A bounded 64-bit tail avoids scalar work for most of the remaining pixels. */
	if (count >= 8) {
		__m128i a = _mm_loadl_epi64((const __m128i*)src);
		__m128i b;
		if (count > 8)
			b = _mm_loadl_epi64((const __m128i*)(src + 1));
		else
			b = _mm_or_si128(_mm_srli_epi64(a, 8), _mm_and_si128(a, edge64));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask64);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		src += 8;
		dst += 8;
		count -= 8;
	}
	while (count > 1) {
		*dst++ = internal_mean_value(src[0], src[1]);
		++src;
		--count;
	}
	if (count)
		*dst = *src;
}

static inline __attribute__((target("sse2"))) void internal_mean16_horz_next_step2_sse2(uint16* dst, const uint16* src, unsigned count)
{
	const __m128i mask64 = _mm_loadl_epi64((const __m128i*)mean_mask);
	const __m128i mask128 = _mm_unpacklo_epi64(mask64, mask64);
	const __m128i edge64 = _mm_set_epi32(0, 0, 0xffff0000, 0);
	const __m128i edge128 = _mm_slli_si128(edge64, 8);

	/* The shifted load includes the next pixel, so keep the last vector separate. */
	while (count > 8) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_loadu_si128((const __m128i*)(src + 1));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		src += 8;
		dst += 8;
		count -= 8;
	}
	if (count == 8) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_or_si128(_mm_srli_si128(a, 2), _mm_and_si128(a, edge128));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		return;
	}
	/* A bounded 64-bit tail avoids scalar work for most of the remaining pixels. */
	if (count >= 4) {
		__m128i a = _mm_loadl_epi64((const __m128i*)src);
		__m128i b;
		if (count > 4)
			b = _mm_loadl_epi64((const __m128i*)(src + 1));
		else
			b = _mm_or_si128(_mm_srli_epi64(a, 16), _mm_and_si128(a, edge64));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask64);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		src += 4;
		dst += 4;
		count -= 4;
	}
	while (count > 1) {
		*dst++ = internal_mean_value(src[0], src[1]);
		++src;
		--count;
	}
	if (count)
		*dst = *src;
}

static inline __attribute__((target("sse2"))) void internal_mean32_horz_next_step4_sse2(uint32* dst, const uint32* src, unsigned count)
{
	const __m128i mask64 = _mm_loadl_epi64((const __m128i*)mean_mask);
	const __m128i mask128 = _mm_unpacklo_epi64(mask64, mask64);
	const __m128i edge64 = _mm_set_epi32(0, 0, 0xffffffff, 0);
	const __m128i edge128 = _mm_slli_si128(edge64, 8);

	/* The shifted load includes the next pixel, so keep the last vector separate. */
	while (count > 4) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_loadu_si128((const __m128i*)(src + 1));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		src += 4;
		dst += 4;
		count -= 4;
	}
	if (count == 4) {
		__m128i a = _mm_loadu_si128((const __m128i*)src);
		__m128i b = _mm_or_si128(_mm_srli_si128(a, 4), _mm_and_si128(a, edge128));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask128);
		_mm_storeu_si128((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		return;
	}
	/* A bounded 64-bit tail avoids scalar work for most of the remaining pixels. */
	if (count >= 2) {
		__m128i a = _mm_loadl_epi64((const __m128i*)src);
		__m128i b;
		if (count > 2)
			b = _mm_loadl_epi64((const __m128i*)(src + 1));
		else
			b = _mm_or_si128(_mm_srli_epi64(a, 32), _mm_and_si128(a, edge64));
		__m128i half = _mm_and_si128(_mm_srli_epi64(_mm_xor_si128(a, b), 1), mask64);
		_mm_storel_epi64((__m128i*)dst, _mm_add_epi32(half, _mm_and_si128(a, b)));
		src += 2;
		dst += 2;
		count -= 2;
	}
	while (count > 1) {
		*dst++ = internal_mean_value(src[0], src[1]);
		++src;
		--count;
	}
	if (count)
		*dst = *src;
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_mean8_horz_next_step1_neon(uint8* dst, const uint8* src, unsigned count)
{
	const uint8x16_t mask = vdupq_n_u8((uint8)mean_mask[MEAN_MASK_H_0]);
	while (count >= 16) {
		uint8 last = src[15];
		uint8x16_t a = vld1q_u8(src);
		uint8x16_t b = count > 16 ? vld1q_u8(src + 1) : vextq_u8(a, a, 1);
		vst1q_u8(dst, vaddq_u8(vandq_u8(vshrq_n_u8(veorq_u8(a, b), 1), mask), vandq_u8(a, b)));
		if (count == 16)
			dst[15] = last;
		src += 16;
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = count > 1 ? internal_mean_value(src[0], src[1]) : src[0];
		++src;
		--count;
	}
}

static inline void internal_mean16_horz_next_step2_neon(uint16* dst, const uint16* src, unsigned count)
{
	const uint16x8_t mask = vdupq_n_u16((uint16)mean_mask[MEAN_MASK_H_0]);
	while (count >= 8) {
		uint16 last = src[7];
		uint16x8_t a = vld1q_u16(src);
		uint16x8_t b = count > 8 ? vld1q_u16(src + 1) : vextq_u16(a, a, 1);
		vst1q_u16(dst, vaddq_u16(vandq_u16(vshrq_n_u16(veorq_u16(a, b), 1), mask), vandq_u16(a, b)));
		if (count == 8)
			dst[7] = last;
		src += 8;
		dst += 8;
		count -= 8;
	}
	while (count) {
		*dst++ = count > 1 ? internal_mean_value(src[0], src[1]) : src[0];
		++src;
		--count;
	}
}

static inline void internal_mean32_horz_next_step4_neon(uint32* dst, const uint32* src, unsigned count)
{
	const uint32x4_t mask = vdupq_n_u32((uint32)mean_mask[MEAN_MASK_H_0]);
	while (count >= 4) {
		uint32 last = src[3];
		uint32x4_t a = vld1q_u32(src);
		uint32x4_t b = count > 4 ? vld1q_u32(src + 1) : vextq_u32(a, a, 1);
		vst1q_u32(dst, vaddq_u32(vandq_u32(vshrq_n_u32(veorq_u32(a, b), 1), mask), vandq_u32(a, b)));
		if (count == 4)
			dst[3] = last;
		src += 4;
		dst += 4;
		count -= 4;
	}
	while (count) {
		*dst++ = count > 1 ? internal_mean_value(src[0], src[1]) : src[0];
		++src;
		--count;
	}
}

#endif

static inline void internal_mean8_horz_next_step1_def(uint8* dst8, const uint8* src8, unsigned count)
{
	while (count >= 4) {
		uint32 current;
		uint32 adjacent;
		memcpy(&current, src8, sizeof(current));
#ifdef USE_LSB
		adjacent = (current >> 8) | (count > 4
			? (uint32)src8[4] << 24 : current & 0xff000000);
#else
		adjacent = (current << 8) | (count > 4
			? src8[4] : current & 0xff);
#endif
		current = internal_mean_value(current, adjacent);
		memcpy(dst8, &current, sizeof(current));
		src8 += 4;
		dst8 += 4;
		count -= 4;
	}
	while (count > 1) {
		*dst8++ = internal_mean_value(src8[0], src8[1]);
		++src8;
		--count;
	}
	if (count)
		*dst8 = *src8;
}

static inline void internal_mean16_horz_next_step2_def(uint16* dst16, const uint16* src16, unsigned count)
{
	while (count >= 2) {
		uint32 current;
		uint32 adjacent;
		memcpy(&current, src16, sizeof(current));
#ifdef USE_LSB
		adjacent = (current >> 16) | (count > 2
			? (uint32)src16[2] << 16 : current & 0xffff0000);
#else
		adjacent = (current << 16) | (count > 2
			? src16[2] : current & 0xffff);
#endif
		current = internal_mean_value(current, adjacent);
		memcpy(dst16, &current, sizeof(current));
		src16 += 2;
		dst16 += 2;
		count -= 2;
	}
	while (count > 1) {
		*dst16++ = internal_mean_value(src16[0], src16[1]);
		++src16;
		--count;
	}
	if (count)
		*dst16 = *src16;
}

static inline void internal_mean32_horz_next_step4_def(uint32* dst32, const uint32* src32, unsigned count)
{
	if (count) {
		--count;
		while (count) {
			*dst32 = internal_mean_value(src32[0], src32[1]);
			++dst32;
			++src32;
			--count;
		}
		*dst32 = *src32;
	}
}

static inline void internal_mean8_horz_next_step(uint8* dst8, const uint8* src8, unsigned count, int step)
{
	if (count) {
		--count;
		while (count) {
			*dst8 = internal_mean_value(P8DER0(src8), P8DER(src8, step));
			dst8 += 1;
			PADD(src8, step);
			--count;
		}
		*dst8 = *src8;
	}
}

static inline void internal_mean16_horz_next_step(uint16* dst16, const uint16* src16, unsigned count, int step)
{
	if (count) {
		--count;
		while (count) {
			*dst16 = internal_mean_value(P16DER0(src16), P16DER(src16, step));
			++dst16;
			PADD(src16, step);
			--count;
		}
		*dst16 = *src16;
	}
}

static inline void internal_mean32_horz_next_step(uint32* dst32, const uint32* src32, unsigned count, int step)
{
	if (count) {
		--count;
		while (count) {
			*dst32 = internal_mean_value(P32DER0(src32), P32DER(src32, step));
			++dst32;
			PADD(src32, step);
			--count;
		}
		*dst32 = *src32;
	}
}

#endif

