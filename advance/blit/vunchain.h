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

#ifndef __VUNCHAIN_H
#define __VUNCHAIN_H

#include "blit.h"

/****************************************************************************/
/* unchained8 */

static void video_line_unchained8_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 4; /* 4 is the number of plane iterations */
	uint8* src8 = (uint8*)src;
	unsigned p;

	for(p=0;p<4;++p) {
		video_unchained_plane_set(p);
		internal_unchained8(dst, src8 + p, inner_count);
	}
}

static void video_line_unchained8_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 4; /* 4 is the number of plane iterations */
	uint8* src8 = (uint8*)src;
	unsigned p;

	for(p=0;p<4;++p) {
		video_unchained_plane_set(p);
		internal_unchained8_step2(dst, src8 + 2*p, inner_count);
	}
}

static void video_line_unchained8(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 4; /* 4 is the number of plane iterations */
	uint8* src8 = (uint8*)src;
	unsigned p;

	for(p=0;p<4;++p) {
		video_unchained_plane_set(p);
		internal_unchained8_step(dst, src8 + stage->sdp*p, inner_count, stage->sdp);
	}
}

static void video_line_unchained8_step1_plane(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_unchained8(dst, src, stage->slice.count / 4);
}

static void video_line_unchained8_step2_plane(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_unchained8_step2(dst, src, stage->slice.count / 4);
}

static void video_line_unchained8_plane(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_unchained8_step(dst, src, stage->slice.count / 4, stage->sdp);
}

static void video_stage_unchained8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	stage->type = pipe_unchained;
	stage->sdx = sdx;
	stage->sbpp = 1;
	video_slice_init(&stage->slice,sdx,sdx);
	stage->sdp = sdp;
	stage->buffer_size = 0;
	stage->plane_num = 4;

	stage->put_plain = video_line_unchained8_step1;
	stage->plane_put_plain = video_line_unchained8_step1_plane;
	if (sdp == 1) {
		stage->put = video_line_unchained8_step1;
		stage->plane_put = video_line_unchained8_step1_plane;
	} else if (sdp == 2) {
		stage->put = video_line_unchained8_step2;
		stage->plane_put = video_line_unchained8_step2_plane;
	} else {
		stage->put = video_line_unchained8;
		stage->plane_put = video_line_unchained8_plane;
	}
}

/****************************************************************************/
/* unchained8_palette16to8 */

static void video_line_unchained8_palette16to8_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 4; /* 4 is the number of plane iteration */
	uint16* src16 = (uint16*)src;
	unsigned p;

	for(p=0;p<4;++p) {
		video_unchained_plane_set(p);
		internal_unchained8_palette16to8(dst, src16 + p, inner_count, stage->palette);
	}
}

static void video_line_unchained8_palette16to8_step1_plane(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_unchained8_palette16to8(dst, src, stage->slice.count / 4, stage->palette);
}

static void video_stage_unchained8_palette16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, unsigned* palette) {
	stage->type = pipe_unchained_palette16to8;
	stage->sdx = sdx;
	stage->sbpp = 2;
	video_slice_init(&stage->slice,sdx,sdx);
	stage->sdp = 2;
	stage->palette = palette;
	stage->buffer_size = 0;
	stage->plane_num = 4;

	stage->put_plain = video_line_unchained8_palette16to8_step1;
	stage->plane_put_plain = video_line_unchained8_palette16to8_step1_plane;
	stage->put = video_line_unchained8_palette16to8_step1;
	stage->plane_put = video_line_unchained8_palette16to8_step1_plane;
}

/****************************************************************************/
/* unchained8_double */

static void video_line_unchained8_double_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2; /* 2 is the number of plane iterations */
	uint8* src8 = (uint8*)src;

	video_unchained_plane_mask_set(0x3);
	internal_unchained8_double(dst, src8, inner_count);

	video_unchained_plane_mask_set(0xC);
	internal_unchained8_double(dst, src8 + 1, inner_count);
}

static void video_line_unchained8_double_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2; /* 2 is the number of plane iterations */
	uint8* src8 = (uint8*)src;

	video_unchained_plane_mask_set(0x3);
	internal_unchained8_double_step2(dst, src8, inner_count);

	video_unchained_plane_mask_set(0xC);
	internal_unchained8_double_step2(dst, src8 + 2, inner_count);
}

static void video_line_unchained8_double(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2; /* 2 is the number of plane iterations */
	uint8* src8 = (uint8*)src;

	video_unchained_plane_mask_set(0x3);
	internal_unchained8_double_step(dst, src8, inner_count, stage->sdp);

	video_unchained_plane_mask_set(0xC);
	internal_unchained8_double_step(dst, src8 + stage->sdp, inner_count, stage->sdp);
}

static void video_stage_unchained8_double_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	stage->type = pipe_unchained_x_double;
	stage->sdx = sdx;
	stage->sbpp = 1;
	video_slice_init(&stage->slice,sdx,sdx);
	stage->sdp = sdp;
	stage->buffer_size = 0;
	stage->plane_put = 0;
	stage->plane_put_plain = 0;

	stage->put_plain = video_line_unchained8_double_step1;
	if (stage->sdp == 1)
		stage->put = video_line_unchained8_double_step1;
	else if (stage->sdp == 2)
		stage->put = video_line_unchained8_double_step2;
	else
		stage->put = video_line_unchained8_double;
}

/****************************************************************************/
/* unchained8_double_palette16to8 */

static void video_line_unchained8_double_palette16to8_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2; /* 2 is the number of plane iterations */
	uint16* src16 = (uint16*)src;

	video_unchained_plane_mask_set(0x3);
	internal_unchained8_double_palette16to8(dst, src16, inner_count,stage->palette);

	video_unchained_plane_mask_set(0xC);
	internal_unchained8_double_palette16to8(dst, src16 + 1, inner_count,stage->palette);
}

static void video_stage_unchained8_double_palette16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, unsigned* palette) {
	stage->type = pipe_unchained_x_double_palette16to8;
	stage->sdx = sdx;
	stage->sbpp = 2;
	video_slice_init(&stage->slice,sdx,sdx);
	stage->sdp = 2;
	stage->palette = palette;
	stage->buffer_size = 0;
	stage->plane_put = 0;
	stage->plane_put_plain = 0;

	stage->put_plain = video_line_unchained8_double_palette16to8_step1;
	stage->put = video_line_unchained8_double_palette16to8_step1;
}

#endif
