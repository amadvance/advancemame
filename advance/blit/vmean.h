/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2008 Andrea Mazzoleni
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

#ifndef __VMEAN_H
#define __VMEAN_H

#include "blit.h"

/****************************************************************************/
/* meanx8 */

static inline void video_line_meanx8_1x_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;
	uint8* dst8 = (uint8*)dst;
	unsigned previous_color;
	adv_bool previous_set;

	previous_color = 0;
	previous_set = 0;
	while (count) {
		unsigned run = whole;
		unsigned color = P8DER0(src);
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		if (previous_set) {
			dst8[0] = internal_mean_value(color, previous_color);
		} else {
			dst8[0] = color;
		}
		if (run > 1) {
			previous_set = 1;
			internal_fill8(dst8 + 1, color, run - 1);
			previous_color = color;
		} else {
			previous_set = 0;
		}
		dst8 += run;
		PADD(src, sdp);
		--count;
	}
}

static inline void video_line_meanx8_x1_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;
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
				color = internal_mean_value(color, P8DER0(src));
				PADD(src, sdp);
				--run;
			}
		}
		dst8[0] = color;
		dst8 += 1;
		--count;
	}
}

static void video_line_meanx8_x1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx8_x1_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_meanx8_x1_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx8_x1_step(stage, line, dst, src, 1, count);
}

static void video_line_meanx8_1x(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx8_1x_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_meanx8_1x_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx8_1x_step(stage, line, dst, src, 1, count);
}

static void video_stage_meanx8_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	if (sdx > ddx) {
		STAGE_SIZE(stage, pipe_x_mean, sdx, sdp, 1, ddx, 1);
		STAGE_PUT(stage, video_line_meanx8_x1_step1, video_line_meanx8_x1);
	} else if (sdx < ddx) {
		STAGE_SIZE(stage, pipe_x_mean, sdx, sdp, 1, ddx, 1);
		STAGE_PUT(stage, video_line_meanx8_1x_step1, video_line_meanx8_1x);
	} else {
		video_stage_stretchx8_set(stage, ddx, sdx, sdp);
	}
}

/****************************************************************************/
/* meanx16 */

static inline void video_line_meanx16_1x_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;
	uint16* dst16 = (uint16*)dst;
	unsigned previous_color;
	adv_bool previous_set;

	previous_color = 0;
	previous_set = 0;
	while (count) {
		unsigned run = stage->slice.whole;
		unsigned color = P16DER0(src);
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		if (previous_set) {
			dst16[0] = internal_mean_value(color, previous_color);
		} else {
			dst16[0] = color;
		}
		if (run > 1) {
			previous_set = 1;
			internal_fill16(dst16 + 1, color, run - 1);
			previous_color = color;
		} else {
			previous_set = 0;
		}
		dst16 += run;
		PADD(src, sdp);
		--count;
	}
}

static inline void video_line_meanx16_x1_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
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
				color = internal_mean_value(color, P16DER0(src));
				PADD(src, sdp);
				--run;
			}
		}
		dst16[0] = color;
		dst16 += 1;
		--count;
	}
}

static void video_line_meanx16_x1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx16_x1_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_meanx16_x1_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx16_x1_step(stage, line, dst, src, 2, count);
}

static void video_line_meanx16_1x(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx16_1x_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_meanx16_1x_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx16_1x_step(stage, line, dst, src, 2, count);
}

static void video_stage_meanx16_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	if (sdx > ddx) {
		STAGE_SIZE(stage, pipe_x_mean, sdx, sdp, 2, ddx, 2);
		STAGE_PUT(stage, video_line_meanx16_x1_step2, video_line_meanx16_x1);
	} else if (sdx < ddx) {
		STAGE_SIZE(stage, pipe_x_mean, sdx, sdp, 2, ddx, 2);
		STAGE_PUT(stage, video_line_meanx16_1x_step2, video_line_meanx16_1x);
	} else {
		video_stage_stretchx16_set(stage, ddx, sdx, sdp);
	}
}

/****************************************************************************/
/* meanx32 */

static inline void video_line_meanx32_1x_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;
	uint32* dst32 = (uint32*)dst;
	unsigned previous_color;
	adv_bool previous_set;

	previous_color = 0;
	previous_set = 0;
	while (count) {
		unsigned run = stage->slice.whole;
		unsigned color = P32DER0(src);
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		if (previous_set) {
			dst32[0] = internal_mean_value(color, previous_color);
		} else {
			dst32[0] = color;
		}
		if (run > 1) {
			previous_set = 1;
			internal_fill32(dst32 + 1, color, run - 1);
			previous_color = color;
		} else {
			previous_set = 0;
		}
		dst32 += run;
		PADD(src, sdp);
		--count;
	}
}

static inline void video_line_meanx32_x1_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
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
				color = internal_mean_value(color, P32DER0(src));
				PADD(src, sdp);
				--run;
			}
		}
		dst32[0] = color;
		dst32 += 1;
		--count;
	}
}

static void video_line_meanx32_x1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx32_x1_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_meanx32_x1_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx32_x1_step(stage, line, dst, src, 4, count);
}

static void video_line_meanx32_1x(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx32_1x_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_meanx32_1x_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_meanx32_1x_step(stage, line, dst, src, 4, count);
}

static void video_stage_meanx32_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	if (sdx > ddx) {
		STAGE_SIZE(stage, pipe_x_mean, sdx, sdp, 4, ddx, 4);
		STAGE_PUT(stage, video_line_meanx32_x1_step4, video_line_meanx32_x1);
	} else if (sdx < ddx) {
		STAGE_SIZE(stage, pipe_x_mean, sdx, sdp, 4, ddx, 4);
		STAGE_PUT(stage, video_line_meanx32_1x_step4, video_line_meanx32_1x);
	} else {
		video_stage_stretchx32_set(stage, ddx, sdx, sdp);
	}
}

#endif

