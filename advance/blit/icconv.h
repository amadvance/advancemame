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

#ifndef __ICONV_H
#define __ICONV_H

#include "icommon.h"

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

/***************************************************************************/
/* internal conv */

#if defined(CONFIG_X86)
static uint32 bgra8888tobgr332_mask[] = {
	0x000000E0, 0x000000E0, 0x000000E0, 0x000000E0, /* r */
	0x0000001C, 0x0000001C, 0x0000001C, 0x0000001C, /* g */
	0x00000003, 0x00000003, 0x00000003, 0x00000003 /* b */
};

static inline __attribute__((target("sse2"))) void internal_convbgra8888tobgr332_sse2(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 16;

	{
		__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
		count >>= 4;
		if (count) {
			xmm5 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgr332_mask + 0));
			xmm6 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgr332_mask + 16));
			xmm7 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgr332_mask + 32));
			do {
				xmm0 = _mm_load_si128((const __m128i*)((const uint8*)src + 0));
				xmm1 = xmm0;
				xmm2 = xmm0;
				xmm0 = _mm_srli_epi64(xmm0, 16);
				xmm1 = _mm_srli_epi64(xmm1, 11);
				xmm2 = _mm_srli_epi64(xmm2, 6);
				xmm0 = _mm_and_si128(xmm0, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm0 = _mm_or_si128(xmm0, xmm1);
				xmm3 = _mm_load_si128((const __m128i*)((const uint8*)src + 16));
				xmm1 = xmm3;
				xmm2 = xmm3;
				xmm3 = _mm_srli_epi64(xmm3, 16);
				xmm1 = _mm_srli_epi64(xmm1, 11);
				xmm2 = _mm_srli_epi64(xmm2, 6);
				xmm3 = _mm_and_si128(xmm3, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm3 = _mm_or_si128(xmm3, xmm1);
				xmm0 = _mm_packus_epi16(xmm0, xmm3);
				xmm4 = _mm_load_si128((const __m128i*)((const uint8*)src + 32));
				xmm1 = xmm4;
				xmm2 = xmm4;
				xmm4 = _mm_srli_epi64(xmm4, 16);
				xmm1 = _mm_srli_epi64(xmm1, 11);
				xmm2 = _mm_srli_epi64(xmm2, 6);
				xmm4 = _mm_and_si128(xmm4, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm4 = _mm_or_si128(xmm4, xmm1);
				xmm3 = _mm_load_si128((const __m128i*)((const uint8*)src + 48));
				xmm1 = xmm3;
				xmm2 = xmm3;
				xmm3 = _mm_srli_epi64(xmm3, 16);
				xmm1 = _mm_srli_epi64(xmm1, 11);
				xmm2 = _mm_srli_epi64(xmm2, 6);
				xmm3 = _mm_and_si128(xmm3, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm3 = _mm_or_si128(xmm3, xmm1);
				xmm4 = _mm_packus_epi16(xmm4, xmm3);
				xmm0 = _mm_packus_epi16(xmm0, xmm4);
				_mm_storeu_si128((__m128i*)((const uint8*)dst + 0), xmm0);
				PADD(src, 64);
				PADD(dst, 16);
				--count;
			} while (count);
		}
	}

	if (rest) {
		const uint32* src32 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src32[0] >> (8 - 2)) & 0x03)
				| ((src32[0] >> (16 - 3 - 2)) & 0x1C)
				| ((src32[0] >> (24 - 3 - 3 - 2)) & 0xE0);
			++src32;
			++dst8;
			--rest;
		} while (rest);
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_convbgra8888tobgr332_neon(void* dst, const void* src, unsigned count)
{
	const uint32x4_t mask0 = vdupq_n_u32(3);
	const uint32x4_t mask1 = vdupq_n_u32(0x1c);
	const uint32x4_t mask2 = vdupq_n_u32(0xe0);
	unsigned rest = count % 8;
	const uint32* input = src;
	uint8* output = dst;
	while (count >= 8) {
		uint32x4_t p = vld1q_u32(input);
		uint16x4_t lo = vmovn_u32(vorrq_u32(vorrq_u32(vandq_u32(vshrq_n_u32(p, 6), mask0), vandq_u32(vshrq_n_u32(p, 11), mask1)), vandq_u32(vshrq_n_u32(p, 16), mask2)));
		p = vld1q_u32(input + 4);
		uint16x8_t pixel = vcombine_u16(lo, vmovn_u32(vorrq_u32(vorrq_u32(vandq_u32(vshrq_n_u32(p, 6), mask0), vandq_u32(vshrq_n_u32(p, 11), mask1)), vandq_u32(vshrq_n_u32(p, 16), mask2))));
		vst1_u8(output, vmovn_u16(pixel));
		input += 8;
		output += 8;
		count -= 8;
	}
	src = input;
	dst = output;
	if (rest) {
		const uint32* src32 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src32[0] >> (8 - 2)) & 0x03)
				| ((src32[0] >> (16 - 3 - 2)) & 0x1C)
				| ((src32[0] >> (24 - 3 - 3 - 2)) & 0xE0);
			++src32;
			++dst8;
			--rest;
		} while (rest);
	}
}

#endif

static inline void internal_convbgra8888tobgr332_def(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 4;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8 - 2)) & 0x03)
			| ((src32[0] >> (16 - 3 - 2)) & 0x1C)
			| ((src32[0] >> (24 - 3 - 3 - 2)) & 0xE0)
			| ((src32[1] << -(8 - 2 - 8)) & 0x0300)
			| ((src32[1] >> (16 - 3 - 2 - 8)) & 0x1C00)
			| ((src32[1] >> (24 - 3 - 3 - 2 - 8)) & 0xE000)
			| ((src32[2] << -(8 - 2 - 16)) & 0x030000)
			| ((src32[2] << -(16 - 3 - 2 - 16)) & 0x1C0000)
			| ((src32[2] >> (24 - 3 - 3 - 2 - 16)) & 0xE00000)
			| ((src32[3] << -(8 - 2 - 24)) & 0x03000000)
			| ((src32[3] << -(16 - 3 - 2 - 24)) & 0x1C000000)
			| ((src32[3] << -(24 - 3 - 3 - 2 - 24)) & 0xE0000000);
#else
		*dst32++ = ((src32[3] >> (8 - 2)) & 0x03)
			| ((src32[3] >> (16 - 3 - 2)) & 0x1C)
			| ((src32[3] >> (24 - 3 - 3 - 2)) & 0xE0)
			| ((src32[2] << -(8 - 2 - 8)) & 0x0300)
			| ((src32[2] >> (16 - 3 - 2 - 8)) & 0x1C00)
			| ((src32[2] >> (24 - 3 - 3 - 2 - 8)) & 0xE000)
			| ((src32[1] << -(8 - 2 - 16)) & 0x030000)
			| ((src32[1] << -(16 - 3 - 2 - 16)) & 0x1C0000)
			| ((src32[1] >> (24 - 3 - 3 - 2 - 16)) & 0xE00000)
			| ((src32[0] << -(8 - 2 - 24)) & 0x03000000)
			| ((src32[0] << -(16 - 3 - 2 - 24)) & 0x1C000000)
			| ((src32[0] << -(24 - 3 - 3 - 2 - 24)) & 0xE0000000);
#endif
		src32 += 4;
		--count;
	}
	if (rest) {
		const uint32* tail_src = (const uint32*)src32;
		uint8* tail_dst = (uint8*)dst32;
		while (rest) {
			uint32 v = *tail_src++;
			*tail_dst++ = ((v >> 6) & 3) | ((v >> 11) & 0x1c) | ((v >> 16) & 0xe0);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static uint32 bgra8888tobgr565_mask[] = {
	0x00F80000, 0x00F80000, 0x00F80000, 0x00F80000, /* r << 8 */
	0x000007E0, 0x000007E0, 0x000007E0, 0x000007E0, /* g */
	0x0000001F, 0x0000001F, 0x0000001F, 0x0000001F /* b */
};

static inline __attribute__((target("sse2"))) void internal_convbgra8888tobgr565_sse2(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;

	{
		__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;
		/* Each iteration consumes eight pixels (the old shift by 32 was invalid). */
		count >>= 3;
		if (count) {
			xmm5 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgr565_mask + 0));
			xmm6 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgr565_mask + 16));
			xmm7 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgr565_mask + 32));
			do {
				xmm0 = _mm_load_si128((const __m128i*)((const uint8*)src + 0));
				xmm1 = xmm0;
				xmm2 = xmm0;
				xmm1 = _mm_srli_epi32(xmm1, 5);
				xmm2 = _mm_srli_epi32(xmm2, 3);
				xmm0 = _mm_and_si128(xmm0, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm3 = _mm_load_si128((const __m128i*)((const uint8*)src + 16));
				xmm4 = xmm3;
				xmm2 = xmm3;
				xmm4 = _mm_srli_epi32(xmm4, 5);
				xmm2 = _mm_srli_epi32(xmm2, 3);
				xmm3 = _mm_and_si128(xmm3, xmm5);
				xmm4 = _mm_and_si128(xmm4, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm4 = _mm_or_si128(xmm4, xmm2);
				xmm0 = _mm_packus_epi16(xmm0, xmm3);
				xmm1 = _mm_packs_epi32(xmm1, xmm4);
				xmm0 = _mm_or_si128(xmm0, xmm1);
				_mm_storeu_si128((__m128i*)((const uint8*)dst + 0), xmm0);
				PADD(src, 32);
				PADD(dst, 16);
				--count;
			} while (count);
		}
	}

	if (rest) {
		const uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8 - 5)) & 0x001F)
				| ((src32[0] >> (16 - 5 - 6)) & 0x07E0)
				| ((src32[0] >> (24 - 5 - 6 - 5)) & 0xF800);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_convbgra8888tobgr565_neon(void* dst, const void* src, unsigned count)
{
	const uint32x4_t mask0 = vdupq_n_u32(0x1f);
	const uint32x4_t mask1 = vdupq_n_u32(0x7e0);
	const uint32x4_t mask2 = vdupq_n_u32(0xf800);
	unsigned rest = count % 8;
	const uint32* input = src;
	uint16* output = dst;
	while (count >= 8) {
		uint32x4_t p = vld1q_u32(input);
		uint16x4_t lo = vmovn_u32(vorrq_u32(vorrq_u32(vandq_u32(vshrq_n_u32(p, 3), mask0), vandq_u32(vshrq_n_u32(p, 5), mask1)), vandq_u32(vshrq_n_u32(p, 8), mask2)));
		p = vld1q_u32(input + 4);
		uint16x8_t pixel = vcombine_u16(lo, vmovn_u32(vorrq_u32(vorrq_u32(vandq_u32(vshrq_n_u32(p, 3), mask0), vandq_u32(vshrq_n_u32(p, 5), mask1)), vandq_u32(vshrq_n_u32(p, 8), mask2))));
		vst1q_u16(output, pixel);
		input += 8;
		output += 8;
		count -= 8;
	}
	src = input;
	dst = output;
	if (rest) {
		const uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8 - 5)) & 0x001F)
				| ((src32[0] >> (16 - 5 - 6)) & 0x07E0)
				| ((src32[0] >> (24 - 5 - 6 - 5)) & 0xF800);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}

#endif

static inline void internal_convbgra8888tobgr565_def(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8 - 5)) & 0x001F)
			| ((src32[0] >> (16 - 5 - 6)) & 0x07E0)
			| ((src32[0] >> (24 - 5 - 6 - 5)) & 0xF800)
			| ((src32[1] << -(8 - 5 - 16)) & 0x001F0000)
			| ((src32[1] << -(16 - 5 - 6 - 16)) & 0x07E00000)
			| ((src32[1] << -(24 - 5 - 6 - 5 - 16)) & 0xF8000000);
#else
		*dst32++ = ((src32[1] >> (8 - 5)) & 0x001F)
			| ((src32[1] >> (16 - 5 - 6)) & 0x07E0)
			| ((src32[1] >> (24 - 5 - 6 - 5)) & 0xF800)
			| ((src32[0] << -(8 - 5 - 16)) & 0x001F0000)
			| ((src32[0] << -(16 - 5 - 6 - 16)) & 0x07E00000)
			| ((src32[0] << -(24 - 5 - 6 - 5 - 16)) & 0xF8000000);
#endif
		src32 += 2;
		--count;
	}
	if (rest) {
		const uint32* tail_src = (const uint32*)src32;
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			uint32 v = *tail_src++;
			*tail_dst++ = ((v >> 3) & 0x1f) | ((v >> 5) & 0x7e0) | ((v >> 8) & 0xf800);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static uint32 bgra8888tobgra5551_mask[] = {
	0x00007C00, 0x00007C00, 0x00007C00, 0x00007C00, /* r */
	0x000003E0, 0x000003E0, 0x000003E0, 0x000003E0, /* g */
	0x0000001F, 0x0000001F, 0x0000001F, 0x0000001F /* b */
};

static inline __attribute__((target("sse2"))) void internal_convbgra8888tobgra5551_sse2(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;

	{
		__m128i xmm0, xmm1, xmm2, xmm3, xmm5, xmm6, xmm7;
		count >>= 3;
		if (count) {
			xmm5 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgra5551_mask + 0));
			xmm6 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgra5551_mask + 16));
			xmm7 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra8888tobgra5551_mask + 32));
			do {
				xmm0 = _mm_load_si128((const __m128i*)((const uint8*)src + 0));
				xmm1 = xmm0;
				xmm2 = xmm0;
				xmm0 = _mm_srli_epi32(xmm0, 9);
				xmm1 = _mm_srli_epi32(xmm1, 6);
				xmm2 = _mm_srli_epi32(xmm2, 3);
				xmm0 = _mm_and_si128(xmm0, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm0 = _mm_or_si128(xmm0, xmm1);
				xmm3 = _mm_load_si128((const __m128i*)((const uint8*)src + 16));
				xmm1 = xmm3;
				xmm2 = xmm3;
				xmm3 = _mm_srli_epi32(xmm3, 9);
				xmm1 = _mm_srli_epi32(xmm1, 6);
				xmm2 = _mm_srli_epi32(xmm2, 3);
				xmm3 = _mm_and_si128(xmm3, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm3 = _mm_or_si128(xmm3, xmm1);
				xmm0 = _mm_packs_epi32(xmm0, xmm3);
				_mm_storeu_si128((__m128i*)((const uint8*)dst + 0), xmm0);
				PADD(src, 32);
				PADD(dst, 16);
				--count;
			} while (count);
		}
	}

	if (rest) {
		const uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8 - 5)) & 0x001F)
				| ((src32[0] >> (16 - 5 - 5)) & 0x03E0)
				| ((src32[0] >> (24 - 5 - 5 - 5)) & 0x7C00);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_convbgra8888tobgra5551_neon(void* dst, const void* src, unsigned count)
{
	const uint32x4_t mask0 = vdupq_n_u32(0x1f);
	const uint32x4_t mask1 = vdupq_n_u32(0x3e0);
	const uint32x4_t mask2 = vdupq_n_u32(0x7c00);
	unsigned rest = count % 8;
	const uint32* input = src;
	uint16* output = dst;
	while (count >= 8) {
		uint32x4_t p = vld1q_u32(input);
		uint16x4_t lo = vmovn_u32(vorrq_u32(vorrq_u32(vandq_u32(vshrq_n_u32(p, 3), mask0), vandq_u32(vshrq_n_u32(p, 6), mask1)), vandq_u32(vshrq_n_u32(p, 9), mask2)));
		p = vld1q_u32(input + 4);
		uint16x8_t pixel = vcombine_u16(lo, vmovn_u32(vorrq_u32(vorrq_u32(vandq_u32(vshrq_n_u32(p, 3), mask0), vandq_u32(vshrq_n_u32(p, 6), mask1)), vandq_u32(vshrq_n_u32(p, 9), mask2))));
		vst1q_u16(output, pixel);
		input += 8;
		output += 8;
		count -= 8;
	}
	src = input;
	dst = output;
	if (rest) {
		const uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8 - 5)) & 0x001F)
				| ((src32[0] >> (16 - 5 - 5)) & 0x03E0)
				| ((src32[0] >> (24 - 5 - 5 - 5)) & 0x7C00);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}

#endif

static inline void internal_convbgra8888tobgra5551_def(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8 - 5)) & 0x001F)
			| ((src32[0] >> (16 - 5 - 5)) & 0x03E0)
			| ((src32[0] >> (24 - 5 - 5 - 5)) & 0x7C00)
			| ((src32[1] << -(8 - 5 - 16)) & 0x001F0000)
			| ((src32[1] << -(16 - 5 - 5 - 16)) & 0x03E00000)
			| ((src32[1] << -(24 - 5 - 5 - 5 - 16)) & 0x7C000000);
#else
		*dst32++ = ((src32[1] >> (8 - 5)) & 0x001F)
			| ((src32[1] >> (16 - 5 - 5)) & 0x03E0)
			| ((src32[1] >> (24 - 5 - 5 - 5)) & 0x7C00)
			| ((src32[0] << -(8 - 5 - 16)) & 0x001F0000)
			| ((src32[0] << -(16 - 5 - 5 - 16)) & 0x03E00000)
			| ((src32[0] << -(24 - 5 - 5 - 5 - 16)) & 0x7C000000);
#endif
		src32 += 2;
		--count;
	}
	if (rest) {
		const uint32* tail_src = (const uint32*)src32;
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			uint32 v = *tail_src++;
			*tail_dst++ = ((v >> 3) & 0x1f) | ((v >> 6) & 0x3e0) | ((v >> 9) & 0x7c00);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static uint32 bgra5551tobgr332_mask[] = {
	0x00E000E0, 0x00E000E0, 0x00E000E0, 0x00E000E0, /* r */
	0x001C001C, 0x001C001C, 0x001C001C, 0x001C001C, /* g */
	0x00030003, 0x00030003, 0x00030003, 0x00030003 /* b */
};

static inline __attribute__((target("sse2"))) void internal_convbgra5551tobgr332_sse2(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 16;

	{
		__m128i xmm0, xmm1, xmm2, xmm3, xmm5, xmm6, xmm7;
		count >>= 4;
		if (count) {
			xmm5 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra5551tobgr332_mask + 0));
			xmm6 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra5551tobgr332_mask + 16));
			xmm7 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra5551tobgr332_mask + 32));
			do {
				xmm0 = _mm_load_si128((const __m128i*)((const uint8*)src + 0));
				xmm1 = xmm0;
				xmm2 = xmm0;
				xmm0 = _mm_srli_epi32(xmm0, 7);
				xmm1 = _mm_srli_epi32(xmm1, 5);
				xmm2 = _mm_srli_epi32(xmm2, 3);
				xmm0 = _mm_and_si128(xmm0, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm0 = _mm_or_si128(xmm0, xmm1);
				xmm3 = _mm_load_si128((const __m128i*)((const uint8*)src + 16));
				xmm1 = xmm3;
				xmm2 = xmm3;
				xmm3 = _mm_srli_epi32(xmm3, 7);
				xmm1 = _mm_srli_epi32(xmm1, 5);
				xmm2 = _mm_srli_epi32(xmm2, 3);
				xmm3 = _mm_and_si128(xmm3, xmm5);
				xmm1 = _mm_and_si128(xmm1, xmm6);
				xmm2 = _mm_and_si128(xmm2, xmm7);
				xmm1 = _mm_or_si128(xmm1, xmm2);
				xmm3 = _mm_or_si128(xmm3, xmm1);
				xmm0 = _mm_packus_epi16(xmm0, xmm3);
				_mm_storeu_si128((__m128i*)((const uint8*)dst + 0), xmm0);
				PADD(src, 32);
				PADD(dst, 16);
				--count;
			} while (count);
		}
	}

	if (rest) {
		const uint16* src16 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src16[0] >> (5 - 2)) & 0x03)
				| ((src16[0] >> (10 - 3 - 2)) & 0x1C)
				| ((src16[0] >> (15 - 3 - 3 - 2)) & 0xE0);
			++src16;
			++dst8;
			--rest;
		} while (rest);
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_convbgra5551tobgr332_neon(void* dst, const void* src, unsigned count)
{
	const uint16x8_t mask0 = vdupq_n_u16(3);
	const uint16x8_t mask1 = vdupq_n_u16(0x1c);
	const uint16x8_t mask2 = vdupq_n_u16(0xe0);
	unsigned rest = count % 8;
	const uint16* input = src;
	uint8* output = dst;
	while (count >= 8) {
		uint16x8_t p = vld1q_u16(input);
		vst1_u8(output, vmovn_u16(vorrq_u16(vorrq_u16(vandq_u16(vshrq_n_u16(p, 3), mask0), vandq_u16(vshrq_n_u16(p, 5), mask1)), vandq_u16(vshrq_n_u16(p, 7), mask2))));
		input += 8;
		output += 8;
		count -= 8;
	}
	src = input;
	dst = output;
	if (rest) {
		const uint16* src16 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src16[0] >> (5 - 2)) & 0x03)
				| ((src16[0] >> (10 - 3 - 2)) & 0x1C)
				| ((src16[0] >> (15 - 3 - 3 - 2)) & 0xE0);
			++src16;
			++dst8;
			--rest;
		} while (rest);
	}
}

#endif

static inline void internal_convbgra5551tobgr332_def(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 4;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src16[0] >> (5 - 2)) & 0x03)
			| ((src16[0] >> (10 - 3 - 2)) & 0x1C)
			| ((src16[0] >> (15 - 3 - 3 - 2)) & 0xE0)
			| ((src16[1] << -(5 - 2 - 8)) & 0x0300)
			| ((src16[1] << -(10 - 3 - 2 - 8)) & 0x1C00)
			| ((src16[1] << -(15 - 3 - 3 - 2 - 8)) & 0xE000)
			| ((src16[2] << -(5 - 2 - 16)) & 0x030000)
			| ((src16[2] << -(10 - 3 - 2 - 16)) & 0x1C0000)
			| ((src16[2] << -(15 - 3 - 3 - 2 - 16)) & 0xE00000)
			| ((src16[3] << -(5 - 2 - 24)) & 0x03000000)
			| ((src16[3] << -(10 - 3 - 2 - 24)) & 0x1C000000)
			| ((src16[3] << -(15 - 3 - 3 - 2 - 24)) & 0xE0000000);
#else
		*dst32++ = ((src16[3] >> (5 - 2)) & 0x03)
			| ((src16[3] >> (10 - 3 - 2)) & 0x1C)
			| ((src16[3] >> (15 - 3 - 3 - 2)) & 0xE0)
			| ((src16[2] << -(5 - 2 - 8)) & 0x0300)
			| ((src16[2] << -(10 - 3 - 2 - 8)) & 0x1C00)
			| ((src16[2] << -(15 - 3 - 3 - 2 - 8)) & 0xE000)
			| ((src16[1] << -(5 - 2 - 16)) & 0x030000)
			| ((src16[1] << -(10 - 3 - 2 - 16)) & 0x1C0000)
			| ((src16[1] << -(15 - 3 - 3 - 2 - 16)) & 0xE00000)
			| ((src16[0] << -(5 - 2 - 24)) & 0x03000000)
			| ((src16[0] << -(10 - 3 - 2 - 24)) & 0x1C000000)
			| ((src16[0] << -(15 - 3 - 3 - 2 - 24)) & 0xE0000000);
#endif
		src16 += 4;
		--count;
	}
	if (rest) {
		const uint16* tail_src = (const uint16*)src16;
		uint8* tail_dst = (uint8*)dst32;
		while (rest) {
			uint16 v = *tail_src++;
			*tail_dst++ = ((v >> 3) & 3) | ((v >> 5) & 0x1c) | ((v >> 7) & 0xe0);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static inline __attribute__((target("sse2"))) void internal_convbgra5551tobgr565_sse2(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;

	{
		const __m128i mask = _mm_set1_epi16(-32); /* 0xFFE0 per pixel */
		__m128i xmm0;
		count >>= 3;
		if (count) {
			do {
				xmm0 = _mm_load_si128((const __m128i*)((const uint8*)src + 0));
				xmm0 = _mm_add_epi16(xmm0, _mm_and_si128(xmm0, mask));
				_mm_storeu_si128((__m128i*)((const uint8*)dst + 0), xmm0);
				PADD(src, 16);
				PADD(dst, 16);
				--count;
			} while (count);
		}
	}

	if (rest) {
		const uint16* src16 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = src16[0] + (src16[0] & 0xFFE0);
			++src16;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_convbgra5551tobgr565_neon(void* dst, const void* src, unsigned count)
{
	const uint16x8_t mask = vdupq_n_u16(0x001f);
	unsigned rest = count % 8;
	const uint16* input = src;
	uint16* output = dst;
	while (count >= 8) {
		uint16x8_t p = vld1q_u16(input);
		vst1q_u16(output, vaddq_u16(p, vbicq_u16(p, mask)));
		input += 8;
		output += 8;
		count -= 8;
	}
	src = input;
	dst = output;
	if (rest) {
		const uint16* src16 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = src16[0] + (src16[0] & 0xFFE0);
			++src16;
			++dst16;
			--rest;
		} while (rest);
	}
}

#endif

static inline void internal_convbgra5551tobgr565_def(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;
	while (count) {
		*dst32++ = (src32[0] & 0x001F001F)
			| ((src32[0] << 1) & 0xFFC0FFC0);
		src32 += 1;
		--count;
	}
	if (rest) {
		const uint16* tail_src = (const uint16*)src32;
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			uint16 v = *tail_src++;
			*tail_dst++ = (v & 0x1f) | ((v << 1) & 0xffc0);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static uint32 bgra5551tobgra8888_mask[] = {
	0x000000F8, 0x000000F8, 0x000000F8, 0x000000F8, /* r */
	0x0000F800, 0x0000F800, 0x0000F800, 0x0000F800, /* g */
	0x00F80000, 0x00F80000, 0x00F80000, 0x00F80000 /* b */
};

static inline __attribute__((target("sse2"))) void internal_convbgra5551tobgra8888_sse2(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;

	{
		__m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5;
		count >>= 2;
		if (count) {
			xmm3 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra5551tobgra8888_mask + 0));
			xmm4 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra5551tobgra8888_mask + 16));
			xmm5 = _mm_loadu_si128((const __m128i*)((const uint8*)bgra5551tobgra8888_mask + 32));
			do {
				xmm0 = _mm_loadl_epi64((const __m128i*)((const uint8*)src + 0));
				xmm0 = _mm_unpacklo_epi16(xmm0, xmm0);
				xmm1 = xmm0;
				xmm2 = xmm0;
				xmm0 = _mm_slli_epi32(xmm0, 3);
				xmm1 = _mm_slli_epi32(xmm1, 6);
				xmm2 = _mm_slli_epi32(xmm2, 9);
				xmm0 = _mm_and_si128(xmm0, xmm3);
				xmm1 = _mm_and_si128(xmm1, xmm4);
				xmm2 = _mm_and_si128(xmm2, xmm5);
				xmm0 = _mm_or_si128(xmm0, xmm1);
				xmm0 = _mm_or_si128(xmm0, xmm2);
				_mm_storeu_si128((__m128i*)((const uint8*)dst + 0), xmm0);
				PADD(src, 8);
				PADD(dst, 16);
				--count;
			} while (count);
		}
	}

	if (rest) {
		const uint16* src16 = src;
		uint32* dst32 = dst;
		do {
			*dst32 = ((src16[0] << 3) & 0x000000F8)
				| ((src16[0] << 6) & 0x0000F800)
				| ((src16[0] << 9) & 0x00F80000);
			++src16;
			++dst32;
			--rest;
		} while (rest);
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)

static inline void internal_convbgra5551tobgra8888_neon(void* dst, const void* src, unsigned count)
{
	const uint32x4_t mask0 = vdupq_n_u32(0xf8);
	const uint32x4_t mask1 = vdupq_n_u32(0xf800);
	const uint32x4_t mask2 = vdupq_n_u32(0xf80000);
	unsigned rest = count % 4;
	const uint16* input = src;
	uint32* output = dst;
	while (count >= 4) {
		uint32x4_t p = vmovl_u16(vld1_u16(input));
		vst1q_u32(output, vorrq_u32(vorrq_u32(vandq_u32(vshlq_n_u32(p, 3), mask0), vandq_u32(vshlq_n_u32(p, 6), mask1)), vandq_u32(vshlq_n_u32(p, 9), mask2)));
		input += 4;
		output += 4;
		count -= 4;
	}
	src = input;
	dst = output;
	if (rest) {
		const uint16* src16 = src;
		uint32* dst32 = dst;
		do {
			*dst32 = ((src16[0] << 3) & 0x000000F8)
				| ((src16[0] << 6) & 0x0000F800)
				| ((src16[0] << 9) & 0x00F80000);
			++src16;
			++dst32;
			--rest;
		} while (rest);
	}
}

#endif

static inline void internal_convbgra5551tobgra8888_def(void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src16[0] << 3) & 0x000000F8)
			| ((src16[0] << 6) & 0x0000F800)
			| ((src16[0] << 9) & 0x00F80000);
		src16 += 1;
		--count;
	}
}

#if defined(CONFIG_X86)
/*
        Y =  0.299  R + 0.587  G + 0.114  B
        U = -0.1687 R - 0.3313 G + 0.5    B + 128
        V =  0.5    R - 0.4187 G - 0.0813 B + 128

        Y = (76*R + 150*G + 29*B) >> 8
        U = (-43*R - 84*G + 128*B) >> 8 + 128
        V = (128*R - 107*G - 20*B) >> 8 + 128
 */

static uint32 bgra8888toyuy2_coeff[] = {
	/*uuuuyyyy    vvvvyyyy */
	0x0080001d, 0xffec001d, /* b */
	0xffac0096, 0xff950096, /* g */
	0xffd5004c, 0x0080004c, /* r */
	0x80000000, 0x80000000  /* add */
};

static inline __attribute__((target("sse2"))) __m128i pixel_convbgra8888toyuy2_value_sse2(const void* src)
{
	uint32 pixel;
	memcpy(&pixel, src, sizeof(pixel));
	__m128i b = _mm_set1_epi16(pixel & 0xff);
	__m128i g = _mm_set1_epi16((pixel >> 8) & 0xff);
	__m128i r = _mm_set1_epi16((pixel >> 16) & 0xff);
	b = _mm_mullo_epi16(b, _mm_loadl_epi64((const __m128i*)bgra8888toyuy2_coeff));
	g = _mm_mullo_epi16(g, _mm_loadl_epi64((const __m128i*)(bgra8888toyuy2_coeff + 2)));
	r = _mm_mullo_epi16(r, _mm_loadl_epi64((const __m128i*)(bgra8888toyuy2_coeff + 4)));
	r = _mm_add_epi16(r, _mm_loadl_epi64((const __m128i*)(bgra8888toyuy2_coeff + 6)));
	return _mm_srli_epi16(_mm_add_epi16(_mm_add_epi16(b, g), r), 8);
}

static inline __attribute__((target("sse2"))) void pixel_convbgra8888toyuy2_sse2(void* dst, const void* src0, const void* src1)
{
	__m128i a = pixel_convbgra8888toyuy2_value_sse2(src0);
	__m128i b = pixel_convbgra8888toyuy2_value_sse2(src1);
	a = _mm_packus_epi16(a, a);
	b = _mm_packus_epi16(b, b);
	_mm_storel_epi64((__m128i*)dst, _mm_unpacklo_epi32(a, b));
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
/*
        Y =  0.299  R + 0.587  G + 0.114  B
        U = -0.1687 R - 0.3313 G + 0.5    B + 128
        V =  0.5    R - 0.4187 G - 0.0813 B + 128

        Y = (76*R + 150*G + 29*B) >> 8
        U = (-43*R - 84*G + 128*B) >> 8 + 128
        V = (128*R - 107*G - 20*B) >> 8 + 128
 */

static inline uint8x16_t pixel_convbgra8888toyuy2_value_neon(const void* src)
{
	uint32 pixel;
	memcpy(&pixel, src, sizeof(pixel));
	/* Four lanes are Y, U, Y, V; arithmetic intentionally wraps at 16 bits. */
	const uint16_t bc[4] = { 29, 128, 29, (uint16_t)-20 };
	const uint16_t gc[4] = { 150, (uint16_t)-84, 150, (uint16_t)-107 };
	const uint16_t rc[4] = { 76, (uint16_t)-43, 76, 128 };
	const uint16_t bias[4] = { 0, 0x8000, 0, 0x8000 };
	uint16x4_t value = vmul_n_u16(vld1_u16(bc), pixel & 0xff);
	value = vmla_n_u16(value, vld1_u16(gc), (pixel >> 8) & 0xff);
	value = vmla_n_u16(value, vld1_u16(rc), (pixel >> 16) & 0xff);
	value = vshr_n_u16(vadd_u16(value, vld1_u16(bias)), 8);
	return vreinterpretq_u8_u16(vcombine_u16(value, vdup_n_u16(0)));
}

static inline void pixel_convbgra8888toyuy2_neon(void* dst, const void* src0, const void* src1)
{
	uint16x4_t a = vget_low_u16(vreinterpretq_u16_u8(pixel_convbgra8888toyuy2_value_neon(src0)));
	uint16x4_t b = vget_low_u16(vreinterpretq_u16_u8(pixel_convbgra8888toyuy2_value_neon(src1)));
	vst1_u8(dst, vmovn_u16(vcombine_u16(a, b)));
}

#endif

static inline void pixel_convbgra8888toyuy2_def(void* dst, const void* src)
{
	const uint8* src8 = (const uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int r, g, b;
	int y, u, v;

#ifdef USE_LSB
	b = src8[0];
	g = src8[1];
	r = src8[2];
#else
	b = src8[3];
	g = src8[2];
	r = src8[1];
#endif

/*
      Y =  0.299  R + 0.587  G + 0.114  B
      U = -0.1687 R - 0.3313 G + 0.5    B + 128
      V =  0.5    R - 0.4187 G - 0.0813 B + 128
 */
	y = ((19595 * r + 38469 * g + 7471 * b) >> 16);
	u = ((-11055 * r - 21712 * g + 32768 * b) >> 16) + 128;
	v = ((32768 * r - 27439 * g - 5328 * b) >> 16) + 128;

	dst8[0] = y;
	dst8[1] = u;
	dst8[2] = y;
	dst8[3] = v;
}

#endif

