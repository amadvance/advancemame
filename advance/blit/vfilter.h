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

#ifndef __VFILTER_H
#define __VFILTER_H

#include "blit.h"

/****************************************************************************/
/* filter8 */

#if defined(USE_ASM_INLINE)
static void video_line_filter8_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean8_horz_next_step1_mmx(dst, src, count);
}
#endif

static void video_line_filter8_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean8_horz_next_step1_def(dst, src, count);
}

static void video_line_filter8_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean8_horz_next_step(dst, src, count, stage->sdp);
}

static void video_stage_filter8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_filter, sdx, sdp, 1, sdx, 1);
	STAGE_PUT(stage, BLITTER(video_line_filter8_step1), video_line_filter8_step);
}

/****************************************************************************/
/* filter16 */

#if defined(USE_ASM_INLINE)
static void video_line_filter16_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean16_horz_next_step2_mmx(dst, src, count);
}
#endif

static void video_line_filter16_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean16_horz_next_step2_def(dst, src, count);
}

static void video_line_filter16_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean16_horz_next_step(dst, src, count, stage->sdp);
}

static void video_stage_filter16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_filter, sdx, sdp, 2, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_filter16_step2), video_line_filter16_step);
}

/****************************************************************************/
/* filter32 */

#if defined(USE_ASM_INLINE)
static void video_line_filter32_step4_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean32_horz_next_step4_mmx(dst, src, count);
}
#endif

static void video_line_filter32_step4_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean32_horz_next_step4_def(dst, src, count);
}

static void video_line_filter32_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_mean32_horz_next_step(dst, src, count, stage->sdp);
}

static void video_stage_filter32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_filter, sdx, sdp, 4, sdx, 4);
	STAGE_PUT(stage, BLITTER(video_line_filter32_step4), video_line_filter32_step);
}

/****************************************************************************/
/* interlacefilter */

#if defined(USE_ASM_INLINE)
static inline void internal_interlacefilter8_step1_mmx(unsigned line, uint8* buffer, uint8* dst, const uint8* src, unsigned count)
{
	if (line == 0) {
		internal_copy8_mmx(buffer, src, count);
		internal_copy8_mmx(dst, buffer, count);
	} else {
		internal_mean8_vert_self_mmx(buffer, src, count);
		internal_copy8_mmx(dst, buffer, count);
		internal_copy8_mmx(buffer, src, count);
	}
}

static inline void internal_interlacefilter16_step1_mmx(unsigned line, uint16* buffer, uint16* dst, const uint16* src, unsigned count)
{
	if (line == 0) {
		internal_copy16_mmx(buffer, src, count);
		internal_copy16_mmx(dst, buffer, count);
	} else {
		internal_mean16_vert_self_mmx(buffer, src, count);
		internal_copy16_mmx(dst, buffer, count);
		internal_copy16_mmx(buffer, src, count);
	}
}

static inline void internal_interlacefilter32_step1_mmx(unsigned line, uint32* buffer, uint32* dst, const uint32* src, unsigned count)
{
	if (line == 0) {
		internal_copy32_mmx(buffer, src, count);
		internal_copy32_mmx(dst, buffer, count);
	} else {
		internal_mean32_vert_self_mmx(buffer, src, count);
		internal_copy32_mmx(dst, buffer, count);
		internal_copy32_mmx(buffer, src, count);
	}
}
#endif

static inline void internal_interlacefilter8_step1_def(unsigned line, uint8* buffer, uint8* dst, const uint8* src, unsigned count)
{
	if (line == 0) {
		internal_copy8_def(buffer, src, count);
		internal_copy8_def(dst, buffer, count);
	} else {
		internal_mean8_vert_self_def(buffer, src, count);
		internal_copy8_def(dst, buffer, count);
		internal_copy8_def(buffer, src, count);
	}
}

static inline void internal_interlacefilter16_step1_def(unsigned line, uint16* buffer, uint16* dst, const uint16* src, unsigned count)
{
	if (line == 0) {
		internal_copy16_def(buffer, src, count);
		internal_copy16_def(dst, buffer, count);
	} else {
		internal_mean16_vert_self_def(buffer, src, count);
		internal_copy16_def(dst, buffer, count);
		internal_copy16_def(buffer, src, count);
	}
}

static inline void internal_interlacefilter32_step1_def(unsigned line, uint32* buffer, uint32* dst, const uint32* src, unsigned count)
{
	if (line == 0) {
		internal_copy32_def(buffer, src, count);
		internal_copy32_def(dst, buffer, count);
	} else {
		internal_mean32_vert_self_def(buffer, src, count);
		internal_copy32_def(dst, buffer, count);
		internal_copy32_def(buffer, src, count);
	}
}

#if defined(USE_ASM_INLINE)
static void video_line_interlacefilter8_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_interlacefilter8_step1_mmx(line, (uint8*)stage->buffer_extra, (uint8*)dst, (const uint8*)src, count);
}
#endif

/****************************************************************************/
/* interlacefilter8 */

static void video_line_interlacefilter8_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_interlacefilter8_step1_def(line, (uint8*)stage->buffer_extra, (uint8*)dst, (const uint8*)src, count);
}

static void video_stage_interlacefilter8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_interlace_filter, sdx, sdp, 1, sdx, 1);
	STAGE_EXTRA(stage);
	STAGE_PUT(stage, BLITTER(video_line_interlacefilter8_step1), 0);
}

/****************************************************************************/
/* interlacefilter16 */

#if defined(USE_ASM_INLINE)
static void video_line_interlacefilter16_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_interlacefilter8_step1_mmx(line, (uint8*)stage->buffer_extra, (uint8*)dst, (const uint8*)src, count * 2);
}
#endif

static void video_line_interlacefilter16_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_interlacefilter8_step1_def(line, (uint8*)stage->buffer_extra, (uint8*)dst, (const uint8*)src, count * 2);
}

static void video_stage_interlacefilter16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_interlace_filter, sdx, sdp, 2, sdx, 2);
	STAGE_EXTRA(stage);
	STAGE_PUT(stage, BLITTER(video_line_interlacefilter16_step1), 0);
}

/****************************************************************************/
/* interlacefilter32 */

#if defined(USE_ASM_INLINE)
static void video_line_interlacefilter32_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_interlacefilter8_step1_mmx(line, (uint8*)stage->buffer_extra, (uint8*)dst, (const uint8*)src, count * 4);
}
#endif

static void video_line_interlacefilter32_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_interlacefilter8_step1_def(line, (uint8*)stage->buffer_extra, (uint8*)dst, (const uint8*)src, count * 4);
}

static void video_stage_interlacefilter32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_interlace_filter, sdx, sdp, 4, sdx, 4);
	STAGE_EXTRA(stage);
	STAGE_PUT(stage, BLITTER(video_line_interlacefilter32_step1), 0);
}

#endif

