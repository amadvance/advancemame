/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#if defined(USE_ASM_i586)
static void video_line_filter8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean8_horz_next_step1_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_filter8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean8_horz_next_step1_def(dst,src,stage->slice.count);
}

static void video_line_filter8_step(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean8_horz_next_step(dst,src,stage->slice.count,stage->sdp);
}

static void video_stage_filter8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_filter,sdx,sdp,1,1,BLITTER(video_line_filter8_step1),video_line_filter8_step);
}

/****************************************************************************/
/* filter16 */

#if defined(USE_ASM_i586)
static void video_line_filter16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean16_horz_next_step2_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_filter16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean16_horz_next_step2_def(dst,src,stage->slice.count);
}

static void video_line_filter16_step(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean16_horz_next_step(dst,src,stage->slice.count,stage->sdp);
}

static void video_stage_filter16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_filter,sdx,sdp,2,2,BLITTER(video_line_filter16_step2),video_line_filter16_step);
}

/****************************************************************************/
/* filter32 */

#if defined(USE_ASM_i586)
static void video_line_filter32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean32_horz_next_step4_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_filter32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean32_horz_next_step4_def(dst,src,stage->slice.count);
}

static void video_line_filter32_step(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_mean32_horz_next_step(dst,src,stage->slice.count,stage->sdp);
}

static void video_stage_filter32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_filter,sdx,sdp,4,4,BLITTER(video_line_filter32_step4),video_line_filter32_step);
}

#endif
