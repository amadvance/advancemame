/*
 * This file is part of the AdvanceMAME project.
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

#ifndef __VRGB_H
#define __VRGB_H

#include "blit.h"

/****************************************************************************/
/* rgb_triad16pix8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad16pix8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad16pix8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad16pix8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad16pix8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad16pix8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad16pix,sdx,sdp,1,1,BLITTER(video_line_rgb_triad16pix8_step1),0);
}

/****************************************************************************/
/* rgb_triad16pix16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad16pix16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad16pix16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad16pix16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad16pix16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad16pix16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad16pix,sdx,sdp,2,2,BLITTER(video_line_rgb_triad16pix16_step2),0);
}

/****************************************************************************/
/* rgb_triad32pix32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad16pix32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad16pix32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad16pix32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad16pix32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad16pix32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad16pix,sdx,sdp,4,4,BLITTER(video_line_rgb_triad16pix32_step4),0);
}

/****************************************************************************/
/* rgb_triad6pix8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad6pix8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad6pix8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad6pix8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad6pix8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad6pix8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad6pix,sdx,sdp,1,1,BLITTER(video_line_rgb_triad6pix8_step1),0);
}

/****************************************************************************/
/* rgb_triad6pix16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad6pix16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad6pix16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad6pix16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad6pix16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad6pix16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad6pix,sdx,sdp,2,2,BLITTER(video_line_rgb_triad6pix16_step2),0);
}

/****************************************************************************/
/* rgb_triad6pix32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad6pix32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad6pix32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad6pix32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad6pix32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad6pix32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad6pix,sdx,sdp,4,4,BLITTER(video_line_rgb_triad6pix32_step4),0);
}

/****************************************************************************/
/* rgb_triad3pix8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad3pix8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad3pix8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad3pix8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad3pix8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad3pix8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad3pix,sdx,sdp,1,1,BLITTER(video_line_rgb_triad3pix8_step1),0);
}

/****************************************************************************/
/* rgb_triad3pix16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad3pix16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad3pix16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad3pix16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad3pix16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad3pix16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad3pix,sdx,sdp,2,2,BLITTER(video_line_rgb_triad3pix16_step2),0);
}

/****************************************************************************/
/* rgb_triad3pix32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triad3pix32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad3pix32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triad3pix32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triad3pix32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triad3pix32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triad3pix,sdx,sdp,4,4,BLITTER(video_line_rgb_triad3pix32_step4),0);
}

/****************************************************************************/
/* rgb_triadstrong16pix8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong16pix8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong16pix8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong16pix8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong16pix8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong16pix8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong16pix,sdx,sdp,1,1,BLITTER(video_line_rgb_triadstrong16pix8_step1),0);
}

/****************************************************************************/
/* rgb_triadstrong16pix16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong16pix16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong16pix16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong16pix16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong16pix16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong16pix16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong16pix,sdx,sdp,2,2,BLITTER(video_line_rgb_triadstrong16pix16_step2),0);
}

/****************************************************************************/
/* rgb_triadstrong32pix32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong16pix32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong16pix32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong16pix32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong16pix32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong16pix32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong16pix,sdx,sdp,4,4,BLITTER(video_line_rgb_triadstrong16pix32_step4),0);
}

/****************************************************************************/
/* rgb_triadstrong6pix8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong6pix8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong6pix8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong6pix8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong6pix8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong6pix8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong6pix,sdx,sdp,1,1,BLITTER(video_line_rgb_triadstrong6pix8_step1),0);
}

/****************************************************************************/
/* rgb_triadstrong6pix16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong6pix16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong6pix16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong6pix16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong6pix16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong6pix16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong6pix,sdx,sdp,2,2,BLITTER(video_line_rgb_triadstrong6pix16_step2),0);
}

/****************************************************************************/
/* rgb_triadstrong6pix32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong6pix32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong6pix32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong6pix32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong6pix32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong6pix32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong6pix,sdx,sdp,4,4,BLITTER(video_line_rgb_triadstrong6pix32_step4),0);
}

/****************************************************************************/
/* rgb_triadstrong3pix8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong3pix8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong3pix8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong3pix8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong3pix8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong3pix8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong3pix,sdx,sdp,1,1,BLITTER(video_line_rgb_triadstrong3pix8_step1),0);
}

/****************************************************************************/
/* rgb_triadstrong3pix16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong3pix16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong3pix16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong3pix16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong3pix16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong3pix16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong3pix,sdx,sdp,2,2,BLITTER(video_line_rgb_triadstrong3pix16_step2),0);
}

/****************************************************************************/
/* rgb_triadstrong3pix32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_triadstrong3pix32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong3pix32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_triadstrong3pix32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_triadstrong3pix32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_triadstrong3pix32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_triadstrong3pix,sdx,sdp,4,4,BLITTER(video_line_rgb_triadstrong3pix32_step4),0);
}

/****************************************************************************/
/* rgb_scandouble8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scandouble8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scandouble8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scandouble8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scandouble8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_scandouble8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scandoublehorz,sdx,sdp,1,1,BLITTER(video_line_rgb_scandouble8_step1),0);
}

/****************************************************************************/
/* rgb_scandouble16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scandouble16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scandouble16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scandouble16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scandouble16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_scandouble16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scandoublehorz,sdx,sdp,2,2,BLITTER(video_line_rgb_scandouble16_step2),0);
}

/****************************************************************************/
/* rgb_scandouble32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scandouble32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scandouble32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scandouble32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scandouble32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_scandouble32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scandoublehorz,sdx,sdp,4,4,BLITTER(video_line_rgb_scandouble32_step4),0);
}

/****************************************************************************/
/* rgb_scandoublevert8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scandoublevert8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scandoublevert8_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scandoublevert8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scandoublevert8_def(dst,src,stage->slice.count);
}

static void video_stage_rgb_scandoublevert8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scandoublevert,sdx,sdp,1,1,BLITTER(video_line_rgb_scandoublevert8_step1),0);
}

/****************************************************************************/
/* rgb_scandoublevert16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scandoublevert16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scandoublevert16_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scandoublevert16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scandoublevert16_def(dst,src,stage->slice.count);
}

static void video_stage_rgb_scandoublevert16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scandoublevert,sdx,sdp,2,2,BLITTER(video_line_rgb_scandoublevert16_step2),0);
}

/****************************************************************************/
/* rgb_scandoublevert32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scandoublevert32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scandoublevert32_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scandoublevert32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scandoublevert32_def(dst,src,stage->slice.count);
}

static void video_stage_rgb_scandoublevert32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scandoublevert,sdx,sdp,4,4,BLITTER(video_line_rgb_scandoublevert32_step4),0);
}

/****************************************************************************/
/* rgb_scantriple8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scantriple8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scantriple8_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scantriple8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scantriple8_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_scantriple8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scantriplehorz,sdx,sdp,1,1,BLITTER(video_line_rgb_scantriple8_step1),0);
}

/****************************************************************************/
/* rgb_scantriple16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scantriple16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scantriple16_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scantriple16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scantriple16_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_scantriple16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scantriplehorz,sdx,sdp,2,2,BLITTER(video_line_rgb_scantriple16_step2),0);
}

/****************************************************************************/
/* rgb_scantriple32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scantriple32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scantriple32_mmx(stage->state_mutable,dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scantriple32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	((struct video_stage_horz_struct*)stage)->state_mutable = internal_rgb_scantriple32_def(stage->state_mutable,dst,src,stage->slice.count);
}

static void video_stage_rgb_scantriple32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scantriplehorz,sdx,sdp,4,4,BLITTER(video_line_rgb_scantriple32_step4),0);
}

/****************************************************************************/
/* rgb_scantriplevert8 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scantriplevert8_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scantriplevert8_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scantriplevert8_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scantriplevert8_def(dst,src,stage->slice.count);
}

static void video_stage_rgb_scantriplevert8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scantriplevert,sdx,sdp,1,1,BLITTER(video_line_rgb_scantriplevert8_step1),0);
}

/****************************************************************************/
/* rgb_scantriplevert16 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scantriplevert16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scantriplevert16_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scantriplevert16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scantriplevert16_def(dst,src,stage->slice.count);
}

static void video_stage_rgb_scantriplevert16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scantriplevert,sdx,sdp,2,2,BLITTER(video_line_rgb_scantriplevert16_step2),0);
}

/****************************************************************************/
/* rgb_scantriplevert32 */

#if defined(USE_ASM_i586)
static void video_line_rgb_scantriplevert32_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scantriplevert32_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_rgb_scantriplevert32_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_rgb_scantriplevert32_def(dst,src,stage->slice.count);
}

static void video_stage_rgb_scantriplevert32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_x_rgb_scantriplevert,sdx,sdp,4,4,BLITTER(video_line_rgb_scantriplevert32_step4),0);
}

#endif
