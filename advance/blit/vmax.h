/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
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

#ifndef __VMAX_H
#define __VMAX_H

#include "blit.h"

/****************************************************************************/
/* meanx8 */

static inline void video_line_maxx8rgb_x1_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;
	int count = stage->slice.count;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		unsigned run = whole;
		unsigned color = P8DER0(src);
		unsigned color_max = internal_max_rgb_value(color);
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		PADD(src, sdp);
		if (count > 1) {
			--run;
			while (run) {
				unsigned color_new = P8DER0(src);
				unsigned color_new_max = internal_max_rgb_value(color_new);
				if (color_max < color_new_max) {
					color = color_new;
					color_max = color_new_max;
				}
				PADD(src, sdp);
				--run;
			}
		}
		dst8[0] = color;
		dst8 += 1;
		--count;
	}
}

static inline void video_line_maxx8pal_x1_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;
	int count = stage->slice.count;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		unsigned run = whole;
		unsigned color = P8DER0(src);
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		PADD(src, sdp);
		if (count > 1) {
			--run;
			while (run) {
				unsigned color_new = P8DER0(src);
				if (color < color_new) {
					color = color_new;
				}
				PADD(src, sdp);
				--run;
			}
		}
		dst8[0] = color;
		dst8 += 1;
		--count;
	}
}

static void video_line_maxx8rgb_x1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx8rgb_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_maxx8rgb_x1_step1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx8rgb_x1_step(stage, dst, src, 1);
}

static void video_line_maxx8pal_x1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx8pal_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_maxx8pal_x1_step1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx8pal_x1_step(stage, dst, src, 1);
}

static void video_stage_maxx8_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	if (sdx > ddx) {
		STAGE_SIZE(stage, pipe_x_max, sdx, sdp, 1, ddx, 1);
		if (index_is_rgb(video_index())) {
			STAGE_PUT(stage, video_line_maxx8rgb_x1_step1, video_line_maxx8rgb_x1);
		} else {
			STAGE_PUT(stage, video_line_maxx8pal_x1_step1, video_line_maxx8pal_x1);
		}
	} else {
		video_stage_stretchx8_set(stage, ddx, sdx, sdp);
	}
}

/****************************************************************************/
/* maxx16 */

static inline void video_line_maxx16rgb_x1_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int count = stage->slice.count;
	uint16* dst16 = (uint16*)dst;

	while (count) {
		unsigned run = stage->slice.whole;
		unsigned color = P16DER0(src);
		unsigned color_max = internal_max_rgb_value(color);
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src, sdp);
		if (count > 1) {
			--run;
			while (run) {
				unsigned color_new = P8DER0(src);
				unsigned color_new_max = internal_max_rgb_value(color_new);
				if (color_max < color_new_max) {
					color = color_new;
					color_max = color_new_max;
				}
				PADD(src, sdp);
				--run;
			}
		}
		dst16[0] = color;
		dst16 += 1;
		--count;
	}
}

static inline void video_line_maxx16pal_x1_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int count = stage->slice.count;
	uint16* dst16 = (uint16*)dst;

	while (count) {
		unsigned run = stage->slice.whole;
		unsigned color = P16DER0(src);
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src, sdp);
		if (count > 1) {
			--run;
			while (run) {
				unsigned color_new = P8DER0(src);
				if (color < color_new) {
					color = color_new;
				}
				PADD(src, sdp);
				--run;
			}
		}
		dst16[0] = color;
		dst16 += 1;
		--count;
	}
}

static void video_line_maxx16rgb_x1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx16rgb_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_maxx16rgb_x1_step2(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx16rgb_x1_step(stage, dst, src, 2);
}

static void video_line_maxx16pal_x1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx16pal_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_maxx16pal_x1_step2(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx16pal_x1_step(stage, dst, src, 2);
}

static void video_stage_maxx16_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	if (sdx > ddx) {
		STAGE_SIZE(stage, pipe_x_max, sdx, sdp, 2, ddx, 2);
		if (index_is_rgb(video_index())) {
			STAGE_PUT(stage, video_line_maxx16rgb_x1_step2, video_line_maxx16rgb_x1);
		} else {
			STAGE_PUT(stage, video_line_maxx16pal_x1_step2, video_line_maxx16pal_x1);
		}
	} else {
		video_stage_stretchx16_set(stage, ddx, sdx, sdp);
	}
}

/****************************************************************************/
/* maxx32 */

static inline void video_line_maxx32_1x_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int count = stage->slice.count;

	while (count) {
		unsigned run = stage->slice.whole;
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		internal_fill32(dst, P32DER0(src), run);
		PADD(dst, run*4);
		PADD(src, sdp);
		--count;
	}
}

static inline void video_line_maxx32rgb_x1_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int count = stage->slice.count;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		unsigned run = stage->slice.whole;
		unsigned color = P32DER0(src);
		unsigned color_max = internal_max_rgb_value(color);
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src, sdp);
		if (count > 1) {
			--run;
			while (run) {
				unsigned color_new = P8DER0(src);
				unsigned color_new_max = internal_max_rgb_value(color_new);
				if (color_max < color_new_max) {
					color = color_new;
					color_max = color_new_max;
				}
				PADD(src, sdp);
				--run;
			}
		}
		dst32[0] = color;
		dst32 += 1;
		--count;
	}
}

static inline void video_line_maxx32pal_x1_step(const struct video_stage_horz_struct* stage, void* dst, const void* src, int sdp)
{
	int error = stage->slice.error;
	int count = stage->slice.count;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		unsigned run = stage->slice.whole;
		unsigned color = P32DER0(src);
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src, sdp);
		if (count > 1) {
			--run;
			while (run) {
				unsigned color_new = P8DER0(src);
				if (color < color_new) {
					color = color_new;
				}
				PADD(src, sdp);
				--run;
			}
		}
		dst32[0] = color;
		dst32 += 1;
		--count;
	}
}

static void video_line_maxx32rgb_x1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx32rgb_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_maxx32rgb_x1_step4(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx32rgb_x1_step(stage, dst, src, 4);
}

static void video_line_maxx32pal_x1(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx32pal_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_maxx32pal_x1_step4(const struct video_stage_horz_struct* stage, void* dst, const void* src)
{
	video_line_maxx32pal_x1_step(stage, dst, src, 4);
}

static void video_stage_maxx32_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	if (sdx > ddx) {
		STAGE_SIZE(stage, pipe_x_max, sdx, sdp, 4, ddx, 4);
		if (index_is_rgb(video_index())) {
			STAGE_PUT(stage, video_line_maxx32rgb_x1_step4, video_line_maxx32rgb_x1);
		} else {
			STAGE_PUT(stage, video_line_maxx32pal_x1_step4, video_line_maxx32pal_x1);
		}
	} else {
		video_stage_stretchx32_set(stage, ddx, sdx, sdp);
	}
}

#endif
