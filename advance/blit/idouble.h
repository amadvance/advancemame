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

#ifndef __IDOUBLE_H
#define __IDOUBLE_H

#include "icommon.h"

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

/***************************************************************************/
/* internal double */

#ifdef CONFIG_X86
static inline __attribute__((target("sse2"))) void internal_double8_sse2(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 8;
	count /= 8;
	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		_mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi8(pixel, pixel));
		src += 8;
		dst += 16;
		--count;
	}
	while (rest) {
		dst[0] = dst[1] = *src++;
		dst += 2;
		--rest;
	}
}

static inline __attribute__((target("sse2"))) void internal_double16_sse2(uint16* dst, const uint16* src, unsigned count)
{
	unsigned rest = count % 4;
	count /= 4;
	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		_mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi16(pixel, pixel));
		src += 4;
		dst += 8;
		--count;
	}
	while (rest) {
		dst[0] = dst[1] = *src++;
		dst += 2;
		--rest;
	}
}

static inline __attribute__((target("sse2"))) void internal_double32_sse2(uint32* dst, const uint32* src, unsigned count)
{
	unsigned rest = count % 2;
	count /= 2;
	while (count) {
		__m128i pixel = _mm_loadl_epi64((const __m128i*)src);
		_mm_storeu_si128((__m128i*)dst, _mm_unpacklo_epi32(pixel, pixel));
		src += 2;
		dst += 4;
		--count;
	}
	while (rest) {
		dst[0] = dst[1] = *src++;
		dst += 2;
		--rest;
	}
}

#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static inline void internal_double8_neon(uint8* dst, const uint8* src, unsigned count)
{
	while (count >= 8) {
		uint8x8_t pixel = vld1_u8(src);
		uint8x8x2_t pair;
		pair.val[0] = pixel;
		pair.val[1] = pixel;
		vst2_u8(dst, pair);
		src += 8;
		dst += 16;
		count -= 8;
	}
	while (count) {
		dst[0] = dst[1] = *src++;
		dst += 2;
		--count;
	}
}

static inline void internal_double16_neon(uint16* dst, const uint16* src, unsigned count)
{
	while (count >= 4) {
		uint16x4_t pixel = vld1_u16(src);
		uint16x4x2_t pair;
		pair.val[0] = pixel;
		pair.val[1] = pixel;
		vst2_u16(dst, pair);
		src += 4;
		dst += 8;
		count -= 4;
	}
	while (count) {
		dst[0] = dst[1] = *src++;
		dst += 2;
		--count;
	}
}

static inline void internal_double32_neon(uint32* dst, const uint32* src, unsigned count)
{
	while (count >= 2) {
		uint32x2_t pixel = vld1_u32(src);
		uint32x2x2_t pair;
		pair.val[0] = pixel;
		pair.val[1] = pixel;
		vst2_u32(dst, pair);
		src += 2;
		dst += 4;
		count -= 2;
	}
	while (count) {
		dst[0] = dst[1] = *src++;
		dst += 2;
		--count;
	}
}

#endif

static inline void internal_double8_def(uint8* restrict dst, const uint8* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}

static inline void internal_double8_step_def(uint8* restrict dst, const uint8* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple8_def(uint8* restrict dst, const uint8* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		src += 1;
		--count;
	}
}

static inline void internal_triple8_step_def(uint8* restrict dst, const uint8* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple8_def(uint8* restrict dst, const uint8* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		src += 1;
		--count;
	}
}

static inline void internal_quadruple8_step_def(uint8* restrict dst, const uint8* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		PADD(src, step);
		--count;
	}
}

static inline void internal_double16_def(uint16* restrict dst, const uint16* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}

static inline void internal_double16_step_def(uint16* restrict dst, const uint16* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple16_def(uint16* restrict dst, const uint16* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		src += 1;
		--count;
	}
}

static inline void internal_triple16_step_def(uint16* restrict dst, const uint16* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple16_def(uint16* restrict dst, const uint16* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		src += 1;
		--count;
	}
}

static inline void internal_quadruple16_step_def(uint16* restrict dst, const uint16* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		PADD(src, step);
		--count;
	}
}

static inline void internal_double32_def(uint32* restrict dst, const uint32* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}

static inline void internal_double32_step_def(uint32* restrict dst, const uint32* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple32_def(uint32* restrict dst, const uint32* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		src += 1;
		--count;
	}
}

static inline void internal_triple32_step_def(uint32* restrict dst, const uint32* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple32_def(uint32* restrict dst, const uint32* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		src += 1;
		--count;
	}
}

static inline void internal_quadruple32_step_def(uint32* restrict dst, const uint32* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		PADD(src, step);
		--count;
	}
}

#endif

