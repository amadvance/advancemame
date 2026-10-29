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

#ifndef __VPALETTE_H
#define __VPALETTE_H

#ifdef CONFIG_X86
#include <emmintrin.h>
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
#include <arm_neon.h>
#endif

#include "blit.h"

/****************************************************************************/
/* palette8to8 */

static void video_line_palette8to8_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint8* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = palette[src8[0]];
		src8 += 1;
		--count;
	}
}

static void video_line_palette8to8(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	const uint8* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = palette[src8[0]];
		src8 += step1;
		--count;
	}
}

static void video_stage_palette8to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint8* palette)
{
	STAGE_SIZE(stage, pipe_palette8to8, sdx, sdp, 1, sdx, 1);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, video_line_palette8to8_step1, video_line_palette8to8);
}

/****************************************************************************/
/* palette8to16 */

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette8to16_step1_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;
	const uint16* palette = stage->palette;

	while (count >= 8) {
		__m128i pixel = _mm_set_epi16(palette[P8DER(src, 7 * (1))], palette[P8DER(src, 6 * (1))], palette[P8DER(src, 5 * (1))], palette[P8DER(src, 4 * (1))], palette[P8DER(src, 3 * (1))], palette[P8DER(src, 2 * (1))], palette[P8DER(src, 1 * (1))], palette[P8DER(src, 0 * (1))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 8 * (1));
		PADD(dst, 16);
		count -= 8;
	}
	while (rest) {
		P16DER0(dst) = palette[P8DER0(src)];
		PADD(dst, 2);
		PADD(src, 1);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette8to16_step1_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint16* palette = stage->palette;
	uint16* output = dst;
	const int step = 1;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint16 a = palette[P8DER(src, 0)];
		uint16 b = palette[P8DER(src, step)];
		uint16 c = palette[P8DER(src, 2 * step)];
		uint16 d = palette[P8DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P8DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette8to16_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	const uint16* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[src8[0]], palette[src8[1]]);
		src8 += 2;
		--count;
	}
	if (rest) {
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			*tail_dst++ = palette[P8DER0(src8)];
			PADD(src8, 1);
			--rest;
		}
	}
}

static void video_line_palette8to16(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	int step1 = stage->sdp;
	int step2 = step1 + step1;
	const uint16* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[src8[0]], palette[src8[step1]]);
		src8 += step2;
		--count;
	}
	if (rest) {
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			*tail_dst++ = palette[P8DER0(src8)];
			PADD(src8, step1);
			--rest;
		}
	}
}

static void video_stage_palette8to16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint16* palette)
{
	STAGE_SIZE(stage, pipe_palette8to16, sdx, sdp, 1, sdx, 2);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette8to16_step1), video_line_palette8to16);
}

/****************************************************************************/
/* palette8to32 */

static void video_line_palette8to32_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint32* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[src8[0]];
		src8 += 1;
		--count;
	}
}

static void video_line_palette8to32(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	const uint32* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[src8[0]];
		src8 += step1;
		--count;
	}
}

static void video_stage_palette8to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint32* palette)
{
	STAGE_SIZE(stage, pipe_palette8to32, sdx, sdp, 1, sdx, 4);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, video_line_palette8to32_step4, video_line_palette8to32);
}

/****************************************************************************/
/* palette16to8 */

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette16to8_step2_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 16;
	const uint8* palette = stage->palette;

	while (count >= 16) {
		__m128i pixel = _mm_set_epi8(palette[P16DER(src, 15 * (2))], palette[P16DER(src, 14 * (2))], palette[P16DER(src, 13 * (2))], palette[P16DER(src, 12 * (2))], palette[P16DER(src, 11 * (2))], palette[P16DER(src, 10 * (2))], palette[P16DER(src, 9 * (2))], palette[P16DER(src, 8 * (2))], palette[P16DER(src, 7 * (2))], palette[P16DER(src, 6 * (2))], palette[P16DER(src, 5 * (2))], palette[P16DER(src, 4 * (2))], palette[P16DER(src, 3 * (2))], palette[P16DER(src, 2 * (2))], palette[P16DER(src, 1 * (2))], palette[P16DER(src, 0 * (2))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 16 * (2));
		PADD(dst, 16);
		count -= 16;
	}
	while (rest) {
		P8DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 1);
		PADD(src, 2);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette16to8_step2_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint8* palette = stage->palette;
	uint8* output = dst;
	const int step = 2;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint8 a = palette[P16DER(src, 0)];
		uint8 b = palette[P16DER(src, step)];
		uint8 c = palette[P16DER(src, 2 * step)];
		uint8 d = palette[P16DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P16DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette16to8_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	const uint8* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 4;

	while (count) {
		*dst32++ = cpu_uint32_make_uint8(palette[src16[0]], palette[src16[1]], palette[src16[2]], palette[src16[3]]);
		src16 += 4;
		--count;
	}
	if (rest) {
		uint8* tail_dst = (uint8*)dst32;
		while (rest) {
			*tail_dst++ = palette[P16DER0(src16)];
			PADD(src16, 2);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette16to8_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 16;
	const uint8* palette = stage->palette;

	while (count >= 16) {
		__m128i pixel = _mm_set_epi8(palette[P16DER(src, 15 * (stage->sdp))], palette[P16DER(src, 14 * (stage->sdp))], palette[P16DER(src, 13 * (stage->sdp))], palette[P16DER(src, 12 * (stage->sdp))], palette[P16DER(src, 11 * (stage->sdp))], palette[P16DER(src, 10 * (stage->sdp))], palette[P16DER(src, 9 * (stage->sdp))], palette[P16DER(src, 8 * (stage->sdp))], palette[P16DER(src, 7 * (stage->sdp))], palette[P16DER(src, 6 * (stage->sdp))], palette[P16DER(src, 5 * (stage->sdp))], palette[P16DER(src, 4 * (stage->sdp))], palette[P16DER(src, 3 * (stage->sdp))], palette[P16DER(src, 2 * (stage->sdp))], palette[P16DER(src, 1 * (stage->sdp))], palette[P16DER(src, 0 * (stage->sdp))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 16 * (stage->sdp));
		PADD(dst, 16);
		count -= 16;
	}
	while (rest) {
		P8DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 1);
		PADD(src, stage->sdp);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette16to8_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint8* palette = stage->palette;
	uint8* output = dst;
	const int step = stage->sdp;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint8 a = palette[P16DER(src, 0)];
		uint8 b = palette[P16DER(src, step)];
		uint8 c = palette[P16DER(src, 2 * step)];
		uint8 d = palette[P16DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P16DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette16to8_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	int step1 = stage->sdp;
	int step2 = step1 + step1;
	int step3 = step2 + step1;
	int step4 = step3 + step1;
	const uint8* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	count /= 4;

	while (count) {
		*dst32++ = cpu_uint32_make_uint8(palette[P16DER0(src)], palette[P16DER(src, step1)], palette[P16DER(src, step2)], palette[P16DER(src, step3)]);
		PADD(src, step4);
		--count;
	}
	if (rest) {
		uint8* tail_dst = (uint8*)dst32;
		while (rest) {
			*tail_dst++ = palette[P16DER0(src)];
			PADD(src, step1);
			--rest;
		}
	}
}

static void video_stage_palette16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint8* palette)
{
	STAGE_SIZE(stage, pipe_palette16to8, sdx, sdp, 2, sdx, 1);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette16to8_step2), BLITTER(video_line_palette16to8));
}

/****************************************************************************/
/* palette16to16 */

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette16to16_step2_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;
	const uint16* palette = stage->palette;

	assert_align(((unsigned)src & 0x1) == 0);

	while (count >= 8) {
		__m128i pixel = _mm_set_epi16(palette[P16DER(src, 7 * (2))], palette[P16DER(src, 6 * (2))], palette[P16DER(src, 5 * (2))], palette[P16DER(src, 4 * (2))], palette[P16DER(src, 3 * (2))], palette[P16DER(src, 2 * (2))], palette[P16DER(src, 1 * (2))], palette[P16DER(src, 0 * (2))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 8 * (2));
		PADD(dst, 16);
		count -= 8;
	}
	while (rest) {
		P16DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 2);
		PADD(src, 2);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette16to16_step2_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint16* palette = stage->palette;
	uint16* output = dst;
	const int step = 2;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint16 a = palette[P16DER(src, 0)];
		uint16 b = palette[P16DER(src, step)];
		uint16 c = palette[P16DER(src, 2 * step)];
		uint16 d = palette[P16DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P16DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette16to16_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	const uint16* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[src16[0]], palette[src16[1]]);
		src16 += 2;
		--count;
	}
	if (rest) {
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			*tail_dst++ = palette[P16DER0(src16)];
			PADD(src16, 2);
			--rest;
		}
	}
}

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette16to16_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;
	const uint16* palette = stage->palette;

	assert_align(((unsigned)src & 0x1) == 0);

	while (count >= 8) {
		__m128i pixel = _mm_set_epi16(palette[P16DER(src, 7 * (stage->sdp))], palette[P16DER(src, 6 * (stage->sdp))], palette[P16DER(src, 5 * (stage->sdp))], palette[P16DER(src, 4 * (stage->sdp))], palette[P16DER(src, 3 * (stage->sdp))], palette[P16DER(src, 2 * (stage->sdp))], palette[P16DER(src, 1 * (stage->sdp))], palette[P16DER(src, 0 * (stage->sdp))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 8 * (stage->sdp));
		PADD(dst, 16);
		count -= 8;
	}
	while (rest) {
		P16DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 2);
		PADD(src, stage->sdp);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette16to16_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint16* palette = stage->palette;
	uint16* output = dst;
	const int step = stage->sdp;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint16 a = palette[P16DER(src, 0)];
		uint16 b = palette[P16DER(src, step)];
		uint16 c = palette[P16DER(src, 2 * step)];
		uint16 d = palette[P16DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P16DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette16to16_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	int step1 = stage->sdp;
	int step2 = step1 + step1;
	const uint16* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[P16DER0(src)], palette[P16DER(src, step1)]);
		PADD(src, step2);
		--count;
	}
	if (rest) {
		uint16* tail_dst = (uint16*)dst32;
		while (rest) {
			*tail_dst++ = palette[P16DER0(src)];
			PADD(src, step1);
			--rest;
		}
	}
}

static void video_stage_palette16to16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint16* palette)
{
	STAGE_SIZE(stage, pipe_palette16to16, sdx, sdp, 2, sdx, 2);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette16to16_step2), BLITTER(video_line_palette16to16));
}

/****************************************************************************/
/* palette16to32 */

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette16to32_step2_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	const uint32* palette = stage->palette;

	assert_align(((unsigned)src & 0x1) == 0);

	while (count >= 4) {
		__m128i pixel = _mm_set_epi32(palette[P16DER(src, 3 * (2))], palette[P16DER(src, 2 * (2))], palette[P16DER(src, 1 * (2))], palette[P16DER(src, 0 * (2))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 4 * (2));
		PADD(dst, 16);
		count -= 4;
	}
	while (rest) {
		P32DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 4);
		PADD(src, 2);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette16to32_step2_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint32* palette = stage->palette;
	uint32* output = dst;
	const int step = 2;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint32 a = palette[P16DER(src, 0)];
		uint32 b = palette[P16DER(src, step)];
		uint32 c = palette[P16DER(src, 2 * step)];
		uint32 d = palette[P16DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P16DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette16to32_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint32* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[src16[0]];
		src16 += 1;
		--count;
	}
}

#if defined(CONFIG_X86)
static __attribute__((target("sse2"))) void video_line_palette16to32_sse2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	const uint32* palette = stage->palette;

	assert_align(((unsigned)src & 0x1) == 0);

	while (count >= 4) {
		__m128i pixel = _mm_set_epi32(palette[P16DER(src, 3 * (stage->sdp))], palette[P16DER(src, 2 * (stage->sdp))], palette[P16DER(src, 1 * (stage->sdp))], palette[P16DER(src, 0 * (stage->sdp))]);
		_mm_storeu_si128((__m128i*)dst, pixel);
		PADD(src, 4 * (stage->sdp));
		PADD(dst, 16);
		count -= 4;
	}
	while (rest) {
		P32DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 4);
		PADD(src, stage->sdp);
		--rest;
	}
}
#endif

#if defined(CONFIG_NEON) || defined(CONFIG_NEON32)
static void video_line_palette16to32_neon(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint32* palette = stage->palette;
	uint32* output = dst;
	const int step = stage->sdp;
	/* Neither ARMv7 nor AArch64 NEON has a general palette gather. */
	while (count >= 4) {
		uint32 a = palette[P16DER(src, 0)];
		uint32 b = palette[P16DER(src, step)];
		uint32 c = palette[P16DER(src, 2 * step)];
		uint32 d = palette[P16DER(src, 3 * step)];
		output[0] = a; output[1] = b; output[2] = c; output[3] = d;
		output += 4;
		PADD(src, 4 * step);
		count -= 4;
	}
	while (count) {
		*output++ = palette[P16DER0(src)];
		PADD(src, step);
		--count;
	}
}

#endif

static void video_line_palette16to32_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	const uint32* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[P16DER0(src)];
		PADD(src, step1);
		--count;
	}
}

static void video_stage_palette16to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint32* palette)
{
	STAGE_SIZE(stage, pipe_palette16to32, sdx, sdp, 2, sdx, 4);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette16to32_step2), BLITTER(video_line_palette16to32));
}

/****************************************************************************/
/* imm16to8 */

static void video_line_imm16to8_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = src16[0] & 0xFF;
		src16 += 1;
		--count;
	}
}

static void video_line_imm16to8_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = src16[0] & 0xFF;
		PADD(src16, stage->sdp);
		--count;
	}
}

static void video_stage_imm16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_imm16to8, sdx, sdp, 2, sdx, 1);
	STAGE_PUT(stage, video_line_imm16to8_step2_def, video_line_imm16to8_def);
}

/****************************************************************************/
/* imm16to32 */

static void video_line_imm16to32_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = src16[0];
		src16 += 1;
		--count;
	}
}

static void video_line_imm16to32_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = src16[0];
		PADD(src16, stage->sdp);
		--count;
	}
}

static void video_stage_imm16to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_imm16to32, sdx, sdp, 2, sdx, 4);
	STAGE_PUT(stage, video_line_imm16to32_step2_def, video_line_imm16to32_def);
}

#endif

