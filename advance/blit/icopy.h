/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2008 Andrea Mazzoleni
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

#ifndef __ICOPY_H
#define __ICOPY_H

#include "icommon.h"

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

/***************************************************************************/
/* internal copy */

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_copy8_sse2(uint8* dst, const uint8* src, unsigned count)
{
	while (count >= 16) {
		__m128i pixel = _mm_loadu_si128((const __m128i*)src);
		_mm_storeu_si128((__m128i*)dst, pixel);
		src += 16;
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = *src++;
		--count;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy8_neon(uint8* dst, const uint8* src, unsigned count)
{
	while (count >= 32) {
		uint8x16_t a = vld1q_u8(src);
		uint8x16_t b = vld1q_u8(src + 16);
		vst1q_u8(dst, a);
		vst1q_u8(dst + 16, b);
		src += 32;
		dst += 32;
		count -= 32;
	}
	if (count >= 16) {
		vst1q_u8(dst, vld1q_u8(src));
		src += 16;
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = *src++;
		--count;
	}
}

#endif

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_copy8_step2_sse2(uint8* dst, const uint8* src, unsigned count)
{
	const __m128i mask = _mm_set1_epi16(0xff);
	while (count >= 16) {
		__m128i lo = _mm_and_si128(_mm_load_si128((const __m128i*)src), mask);
		__m128i hi = _mm_and_si128(_mm_load_si128((const __m128i*)(src + 16)), mask);
		_mm_storeu_si128((__m128i*)dst, _mm_packus_epi16(lo, hi));
		src += 32;
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = *src;
		src += 2;
		--count;
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy8_step2_neon(uint8* dst, const uint8* src, unsigned count)
{
	while (count >= 16) {
		uint8x16x2_t pixel = vld2q_u8(src);
		vst1q_u8(dst, pixel.val[0]);
		src += 32;
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = *src;
		src += 2;
		--count;
	}
}

#endif

static inline void internal_copy8_def(uint8* dst, const uint8* src, unsigned count)
{
	memcpy(dst, src, count);
}

static inline void internal_copy8_step2_def(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		src += 2;
		--count;
	}
}

#if defined(CONFIG_X86)
static inline void internal_copy16_sse2(uint16* dst, const uint16* src, unsigned count)
{
	internal_copy8_sse2((uint8*)dst, (uint8*)src, 2 * count);
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy16_neon(uint16* dst, const uint16* src, unsigned count)
{
	internal_copy8_neon((uint8*)dst, (uint8*)src, 2 * count);
}

#endif

static inline void internal_copy16_def(uint16* dst, const uint16* src, unsigned count)
{
	internal_copy8_def((uint8*)dst, (uint8*)src, 2 * count);
}

#if defined(CONFIG_X86)
static inline void internal_copy32_sse2(uint32* dst, const uint32* src, unsigned count)
{
	internal_copy8_sse2((uint8*)dst, (uint8*)src, 4 * count);
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy32_neon(uint32* dst, const uint32* src, unsigned count)
{
	internal_copy8_neon((uint8*)dst, (uint8*)src, 4 * count);
}

#endif

static inline void internal_copy32_def(uint32* dst, const uint32* src, unsigned count)
{
	internal_copy8_def((uint8*)dst, (uint8*)src, 4 * count);
}

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_copy8_step_sse2(uint8* dst, const uint8* src, unsigned count, int step)
{
	while (count >= 16) {
		__m128i pixel = _mm_set_epi8(P8DER(src, 15 * step), P8DER(src, 14 * step), P8DER(src, 13 * step), P8DER(src, 12 * step), P8DER(src, 11 * step), P8DER(src, 10 * step), P8DER(src, 9 * step), P8DER(src, 8 * step), P8DER(src, 7 * step), P8DER(src, 6 * step), P8DER(src, 5 * step), P8DER(src, 4 * step), P8DER(src, 3 * step), P8DER(src, 2 * step), P8DER(src, 1 * step), P8DER(src, 0 * step));
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 16 * step);
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = *src;
		PADD(src, step);
		--count;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy8_step_neon(uint8* dst, const uint8* src, unsigned count, int step)
{
	while (count >= 4) {
		uint8 a = P8DER(src, 0);
		uint8 b = P8DER(src, step);
		uint8 c = P8DER(src, 2 * step);
		uint8 d = P8DER(src, 3 * step);
		dst[0] = a; dst[1] = b; dst[2] = c; dst[3] = d;
		PADD(src, 4 * step);
		dst += 4;
		count -= 4;
	}
	while (count) {
		*dst++ = *src;
		PADD(src, step);
		--count;
	}
}

#endif

static inline void internal_copy8_step_def(uint8* dst, const uint8* src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--count;
	}
}

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_copy16_step_sse2(uint16* dst, const uint16* src, unsigned count, int step)
{
	while (count >= 8) {
		__m128i pixel = _mm_set_epi16(P16DER(src, 7 * step), P16DER(src, 6 * step), P16DER(src, 5 * step), P16DER(src, 4 * step), P16DER(src, 3 * step), P16DER(src, 2 * step), P16DER(src, 1 * step), P16DER(src, 0 * step));
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 8 * step);
		dst += 8;
		count -= 8;
	}
	while (count) {
		*dst++ = *src;
		PADD(src, step);
		--count;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy16_step_neon(uint16* dst, const uint16* src, unsigned count, int step)
{
	while (count >= 4) {
		uint16 a = P16DER(src, 0);
		uint16 b = P16DER(src, step);
		uint16 c = P16DER(src, 2 * step);
		uint16 d = P16DER(src, 3 * step);
		dst[0] = a; dst[1] = b; dst[2] = c; dst[3] = d;
		PADD(src, 4 * step);
		dst += 4;
		count -= 4;
	}
	while (count) {
		*dst++ = *src;
		PADD(src, step);
		--count;
	}
}

#endif

static inline void internal_copy16_step_def(uint16* dst, const uint16* src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--count;
	}
}

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_copy32_step_sse2(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count >= 4) {
		__m128i pixel = _mm_set_epi32(P32DER(src, 3 * step), P32DER(src, 2 * step), P32DER(src, 1 * step), P32DER(src, 0 * step));
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 4 * step);
		dst += 4;
		count -= 4;
	}
	while (count) {
		*dst++ = *src;
		PADD(src, step);
		--count;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_copy32_step_neon(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count >= 4) {
		uint32 a = P32DER(src, 0);
		uint32 b = P32DER(src, step);
		uint32 c = P32DER(src, 2 * step);
		uint32 d = P32DER(src, 3 * step);
		dst[0] = a; dst[1] = b; dst[2] = c; dst[3] = d;
		PADD(src, 4 * step);
		dst += 4;
		count -= 4;
	}
	while (count) {
		*dst++ = *src;
		PADD(src, step);
		--count;
	}
}

#endif

static inline void internal_copy32_step_def(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--count;
	}
}

/***************************************************************************/
/* internal fill */

/* Set optimized for small counts > 0 */
static inline void internal_fill8(uint8* dst, unsigned src, unsigned count)
{
	assert(count > 0);

	do {
		*dst++ = src;
		--count;
	} while (count);
}

/* Set optimized for small counts > 0 */
static inline void internal_fill16(uint16* dst, unsigned src, unsigned count)
{
	assert(count > 0);

	do {
		*dst++ = src;
		--count;
	} while (count);
}

/* Set optimized for small counts > 0 */
static inline void internal_fill32(uint32* dst, unsigned src, unsigned count)
{
	assert(count > 0);

	do {
		*dst++ = src;
		--count;
	} while (count);
}

/***************************************************************************/
/* internal zero */

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_zero8_sse2(uint8* dst, unsigned count)
{
	while (count >= 16) {
		_mm_storeu_si128((__m128i*)dst, _mm_setzero_si128());
		dst += 16;
		count -= 16;
	}
	while (count) {
		*dst++ = 0;
		--count;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_zero8_neon(uint8* dst, unsigned count)
{
	const uint8x16_t zero = vdupq_n_u8(0);
	while (count >= 16) {
		vst1q_u8(dst, zero);
		dst += 16;
		count -= 16;
	}
	while (count--) *dst++ = 0;
}

#endif

static inline void internal_zero8_def(uint8* dst, unsigned count)
{
	memset(dst, 0, count);
}

#if defined(CONFIG_X86)
static inline void internal_zero16_sse2(uint16* dst, unsigned count)
{
	internal_zero8_sse2((uint8*)dst, 2 * count);
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_zero16_neon(uint16* dst, unsigned count)
{
	internal_zero8_neon((uint8*)dst, 2 * count);
}

#endif

static inline void internal_zero16_def(uint16* dst, unsigned count)
{
	internal_zero8_def((uint8*)dst, 2 * count);
}

#if defined(CONFIG_X86)
static inline void internal_zero32_sse2(uint32* dst, unsigned count)
{
	internal_zero8_sse2((uint8*)dst, 4 * count);
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_zero32_neon(uint32* dst, unsigned count)
{
	internal_zero8_neon((uint8*)dst, 4 * count);
}

#endif

static inline void internal_zero32_def(uint32* dst, unsigned count)
{
	internal_zero8_def((uint8*)dst, 4 * count);
}

#endif

