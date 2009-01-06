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

#ifndef __VCONV_H
#define __VCONV_H

#include "blit.h"

/****************************************************************************/
/* bgra8888 to bgr332 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra8888tobgr332_step4_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra8888tobgr332_mmx(dst, src, count);
}
#endif

static void video_line_bgra8888tobgr332_step4_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra8888tobgr332_def(dst, src, count);
}

static void video_stage_bgra8888tobgr332_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888tobgr332, sdx, sdp, 4, sdx, 1);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888tobgr332_step4), 0);
}

/****************************************************************************/
/* bgra8888 to bgr565 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra8888tobgr565_step4_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra8888tobgr565_mmx(dst, src, count);
}
#endif

static void video_line_bgra8888tobgr565_step4_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra8888tobgr565_def(dst, src, count);
}

static void video_stage_bgra8888tobgr565_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888tobgr565, sdx, sdp, 4, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888tobgr565_step4), 0);
}

/****************************************************************************/
/* bgra8888 to bgra5551 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra8888tobgra5551_step4_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra8888tobgra5551_mmx(dst, src, count);
}
#endif

static void video_line_bgra8888tobgra5551_step4_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra8888tobgra5551_def(dst, src, count);
}

static void video_stage_bgra8888tobgra5551_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888tobgra5551, sdx, sdp, 4, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888tobgra5551_step4), 0);
}

/****************************************************************************/
/* bgra5551 to bgr332 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra5551tobgr332_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra5551tobgr332_mmx(dst, src, count);
}
#endif

static void video_line_bgra5551tobgr332_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra5551tobgr332_def(dst, src, count);
}

static void video_stage_bgra5551tobgr332_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551tobgr332, sdx, sdp, 2, sdx, 1);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551tobgr332_step2), 0);
}

/****************************************************************************/
/* bgra5551 to bgr565 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra5551tobgr565_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra5551tobgr565_mmx(dst, src, count);
}
#endif

static void video_line_bgra5551tobgr565_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra5551tobgr565_def(dst, src, count);
}

static void video_stage_bgra5551tobgr565_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551tobgr565, sdx, sdp, 2, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551tobgr565_step2), 0);
}

/****************************************************************************/
/* bgra5551 to bgra8888 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra5551tobgra8888_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra5551tobgra8888_mmx(dst, src, count);
}
#endif

static void video_line_bgra5551tobgra8888_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_convbgra5551tobgra8888_def(dst, src, count);
}

static void video_stage_bgra5551tobgra8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551tobgra8888, sdx, sdp, 2, sdx, 4);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551tobgra8888_step2), 0);
}

/****************************************************************************/
/* rgb888 to bgra8888 */

static void video_line_rgb888tobgra8888_step3(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count >= 2) {
#ifdef USE_LSB
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		dst8[3] = 0;
		dst8[4] = src8[5];
		dst8[5] = src8[4];
		dst8[6] = src8[3];
		dst8[7] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[2];
		dst8[2] = src8[1];
		dst8[3] = src8[0];
		dst8[4] = 0;
		dst8[5] = src8[5];
		dst8[6] = src8[4];
		dst8[7] = src8[3];
#endif
		dst8 += 8;
		src8 += 6;
		count -= 2;
	}

	if (count) {
#ifdef USE_LSB
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		dst8[3] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[2];
		dst8[2] = src8[1];
		dst8[3] = src8[0];
#endif
	}
}

static void video_line_rgb888tobgra8888_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		dst8[3] = 0;
		dst8 += 4;
		src8 += step1;
		--count;
	}
}

static void video_stage_rgb888tobgra8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_rgb888tobgra8888, sdx, sdp, 3, sdx, 4);

	stage->put_plain = video_line_rgb888tobgra8888_step3;
	if (sdp == 3)
		stage->put = video_line_rgb888tobgra8888_step3;
	else
		stage->put = video_line_rgb888tobgra8888_step;
}

/****************************************************************************/
/* rgba8888 to bgra8888 */

static void video_line_rgba8888tobgra8888_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count >= 2) {
#ifdef USE_LSB
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		dst8[3] = 0;
		dst8[4] = src8[6];
		dst8[5] = src8[5];
		dst8[6] = src8[4];
		dst8[7] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[3];
		dst8[2] = src8[2];
		dst8[3] = src8[1];
		dst8[4] = 0;
		dst8[5] = src8[7];
		dst8[6] = src8[6];
		dst8[7] = src8[5];
#endif
		dst8 += 8;
		src8 += 8;
		count -= 2;
	}

	if (count) {
#ifdef USE_LSB
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		dst8[3] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[3];
		dst8[2] = src8[2];
		dst8[3] = src8[1];
#endif
	}
}

static void video_line_rgba8888tobgra8888_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
#ifdef USE_LSB
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		dst8[3] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[3];
		dst8[2] = src8[2];
		dst8[3] = src8[1];
#endif
		dst8 += 4;
		src8 += step1;
		--count;
	}
}

static void video_stage_rgba8888tobgra8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_rgba8888tobgra8888, sdx, sdp, 4, sdx, 4);

	stage->put_plain = video_line_rgba8888tobgra8888_step4;
	if (sdp == 4)
		stage->put = video_line_rgba8888tobgra8888_step4;
	else
		stage->put = video_line_rgba8888tobgra8888_step;
}

/****************************************************************************/
/* bgr888 to bgra8888 */

static void video_line_bgr888tobgra8888_step3(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count >= 2) {
#ifdef USE_LSB
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		dst8[3] = 0;
		dst8[4] = src8[3];
		dst8[5] = src8[4];
		dst8[6] = src8[5];
		dst8[7] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[0];
		dst8[2] = src8[1];
		dst8[3] = src8[2];
		dst8[4] = 0;
		dst8[5] = src8[3];
		dst8[6] = src8[4];
		dst8[7] = src8[5];
#endif
		dst8 += 8;
		src8 += 6;
		count -= 2;
	}

	if (count) {
#ifdef USE_LSB
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		dst8[3] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[0];
		dst8[2] = src8[1];
		dst8[3] = src8[2];
#endif
	}
}

static void video_line_bgr888tobgra8888_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
#ifdef USE_LSB
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		dst8[3] = 0;
#else
		dst8[0] = 0;
		dst8[1] = src8[0];
		dst8[2] = src8[1];
		dst8[3] = src8[2];
#endif
		dst8 += 4;
		src8 += step1;
		--count;
	}
}

static void video_stage_bgr888tobgra8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgr888tobgra8888, sdx, sdp, 3, sdx, 4);

	stage->put_plain = video_line_bgr888tobgra8888_step3;
	if (sdp == 3)
		stage->put = video_line_bgr888tobgra8888_step3;
	else
		stage->put = video_line_bgr888tobgra8888_step;
}

/****************************************************************************/
/* bgra8888 to yuy2 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra8888toyuy2_step_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	count /= 2;

	while (count) {
		pixel_convbgra8888toyuy2_mmx(dst8, src8, src8 + step1);

		dst8 += 8;
		src8 += step1 * 2;
		--count;
	}
}
#endif

static void video_line_bgra8888toyuy2_step_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		pixel_convbgra8888toyuy2_def(dst8, src8);

		dst8 += 4;
		src8 += step1;
		--count;
	}
}

static void video_stage_bgra8888toyuy2_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888toyuy2, sdx, sdp, 4, sdx, 4);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888toyuy2_step), BLITTER(video_line_bgra8888toyuy2_step));
}

/****************************************************************************/
/* bgra5551 to yuy2 */

#if defined(USE_ASM_INLINE)
static void video_line_bgra5551toyuy2_step_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	count /= 2;

	while (count) {
		uint32 p0;
		uint32 p1;

		p0 = ((src16[0] << 3) & 0x000000F8)
			| ((src16[0] << 6) & 0x0000F800)
			| ((src16[0] << 9) & 0x00F80000);

		PADD(src16, step1);

		p1 = ((src16[0] << 3) & 0x000000F8)
			| ((src16[0] << 6) & 0x0000F800)
			| ((src16[0] << 9) & 0x00F80000);

		PADD(src16, step1);

		pixel_convbgra8888toyuy2_mmx(dst8, &p0, &p1);

		dst8 += 8;
		--count;
	}
}
#endif

static void video_line_bgra5551toyuy2_step_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		unsigned char p4[4];

		cpu_uint32_write(p4,
			((src16[0] << 3) & 0x000000F8)
			| ((src16[0] << 6) & 0x0000F800)
			| ((src16[0] << 9) & 0x00F80000)
		);

		pixel_convbgra8888toyuy2_def(dst8, p4);

		dst8 += 4;
		PADD(src16, step1);
		--count;
	}
}

static void video_stage_bgra5551toyuy2_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551toyuy2, sdx, sdp, 2, sdx, 4);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551toyuy2_step), BLITTER(video_line_bgra5551toyuy2_step));
}

/****************************************************************************/
/* rgb to yuy2 */

static void video_line_rgbtoyuy2_step_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		adv_pixel p;
		unsigned char p4[4];

		p = cpu_uint_read(src8, stage->ssp);

		cpu_uint32_write(p4, (rgb_shift(p, stage->red_shift) & stage->red_mask)
			| (rgb_shift(p, stage->green_shift) & stage->green_mask)
			| (rgb_shift(p, stage->blue_shift) & stage->blue_mask));

		pixel_convbgra8888toyuy2_def(dst8, &p4);

		PADD(src8, stage->sdp);
		dst8 += 4;
		--count;
	}
}

static void video_stage_rgbtoyuy2_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, adv_color_def sdef)
{
	adv_color_def ddef = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0);
	STAGE_SIZE(stage, pipe_rgbtoyuy2, sdx, sdp, color_def_bytes_per_pixel_get(sdef), sdx, 4);
	STAGE_PUT(stage, video_line_rgbtoyuy2_step_def, video_line_rgbtoyuy2_step_def);
	STAGE_CONVERSION(stage, sdef, ddef);
}

/****************************************************************************/
/* rgb to rgb */

static void video_line_rgbtorgb_step_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		adv_pixel p;

		p = cpu_uint_read(src8, stage->ssp);

		p = (rgb_shift(p, stage->red_shift) & stage->red_mask)
			| (rgb_shift(p, stage->green_shift) & stage->green_mask)
			| (rgb_shift(p, stage->blue_shift) & stage->blue_mask);

		cpu_uint_write(dst8, stage->dsp, p);

		PADD(src8, stage->sdp);
		PADD(dst8, stage->dsp);
		--count;
	}
}

static void video_stage_rgbtorgb_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, adv_color_def sdef, adv_color_def ddef)
{
	STAGE_SIZE(stage, pipe_rgbtorgb, sdx, sdp, color_def_bytes_per_pixel_get(sdef), sdx, color_def_bytes_per_pixel_get(ddef));
	STAGE_PUT(stage, video_line_rgbtorgb_step_def, video_line_rgbtorgb_step_def);
	STAGE_CONVERSION(stage, sdef, ddef);
}

#endif

