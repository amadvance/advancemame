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

#ifndef __VSTRETCH_H
#define __VSTRETCH_H

#include "blit.h"

/****************************************************************************/
/* stretchx8 */

#define VIDEO8_LINE_INIT \
	uint8* dst8 = (uint8*)dst;

#define VIDEO8_LINE_WRITE1 \
	dst8[0] = color; \
	dst8 += 1;

#define VIDEO8_LINE_WRITE2 \
	dst8[0] = color; \
	dst8[1] = color; \
	dst8 += 2;

#define VIDEO8_LINE_WRITE3 \
	dst8[0] = color; \
	dst8[1] = color; \
	dst8[2] = color; \
	dst8 += 3;

static inline void video_line_stretchx8_1x_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;

	while (count) {
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		internal_fill8(dst, P8DER0(src), run);
		PADD(dst, run);
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx8_1x(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_1x_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx8_1x_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_1x_step(stage, line, dst, src, 1, count);
}

static inline void video_line_stretchx8_x1_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;


	VIDEO8_LINE_INIT

	while (count) {
		unsigned run = whole;
		unsigned color = P8DER0(src);
		VIDEO8_LINE_WRITE1
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		PADD(src, sdp * run);
		--count;
	}
}

static void video_line_stretchx8_x1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_x1_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx8_x1_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_x1_step(stage, line, dst, src, 1, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx8_11_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy8_mmx(dst, src, count);
}
#endif

static void video_line_stretchx8_11_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy8_def(dst, src, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx8_11_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy8_step2_mmx(dst, src, count);
}
#endif

static void video_line_stretchx8_11_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy8_step2_def(dst, src, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx8_11_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy8_step_mmx(dst, src, count, stage->sdp);
}
#endif

static void video_line_stretchx8_11_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy8_step_def(dst, src, count, stage->sdp);
}

static inline void video_line_stretchx8_12_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO8_LINE_INIT

	while (count) {
		unsigned color = P8DER0(src);
		if ((error += up) > 0) {
			error -= down;
			VIDEO8_LINE_WRITE2
		} else {
			VIDEO8_LINE_WRITE1
		}
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx8_12(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_12_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx8_12_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_12_step(stage, line, dst, src, 1, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx8_22_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double8_mmx(dst, src, count);
}
#endif

static void video_line_stretchx8_22_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double8_def(dst, src, count);
}

static void video_line_stretchx8_22(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double8_step_def(dst, src, count, stage->sdp);
}

static inline void video_line_stretchx8_23_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = whole;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO8_LINE_INIT

	while (count) {
		unsigned color = P8DER0(src);
		if ((error += up) > 0) {
			error -= down;
			VIDEO8_LINE_WRITE3
		} else {
			VIDEO8_LINE_WRITE2
		}
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx8_23(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_23_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx8_23_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx8_23_step(stage, line, dst, src, 1, count);
}

static void video_line_stretchx8_33_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_triple8_def(dst, src, count);
}

static void video_line_stretchx8_33(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_triple8_step_def(dst, src, count, stage->sdp);
}

static void video_line_stretchx8_44_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_quadruple8_def(dst, src, count);
}

static void video_line_stretchx8_44(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_quadruple8_step_def(dst, src, count, stage->sdp);
}

/* Initialize the line rescaling system */
static void video_stage_stretchx8_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_stretch, sdx, sdp, 1, ddx, 1);

	if (sdx > ddx) {
		STAGE_PUT(stage, video_line_stretchx8_x1_step1, video_line_stretchx8_x1);
	} else if (sdx == ddx) {
		STAGE_TYPE(stage, pipe_x_copy);
		STAGE_PUT(stage, BLITTER(video_line_stretchx8_11_step1), BLITTER(video_line_stretchx8_11));
	} else {
		if (stage->slice.whole == 1) {
			STAGE_PUT(stage, video_line_stretchx8_12_step1, video_line_stretchx8_12);
		} else if (stage->slice.whole == 2 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_double);
			STAGE_PUT(stage, BLITTER(video_line_stretchx8_22_step1), video_line_stretchx8_22);
		} else if (stage->slice.whole == 2) {
			STAGE_PUT(stage, video_line_stretchx8_23_step1, video_line_stretchx8_23);
		} else if (stage->slice.whole == 3 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_triple);
			STAGE_PUT(stage, video_line_stretchx8_33_step1, video_line_stretchx8_33);
		} else if (stage->slice.whole == 4 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_quadruple);
			STAGE_PUT(stage, video_line_stretchx8_44_step1, video_line_stretchx8_44);
		} else {
			STAGE_PUT(stage, video_line_stretchx8_1x_step1, video_line_stretchx8_1x);
		}
	}
}

/****************************************************************************/
/* stretchx16 */

#define VIDEO16_LINE_INIT \
	uint16* dst16 = (uint16*)dst;

#define VIDEO16_LINE_WRITE1 \
	dst16[0] = color; \
	dst16 += 1;

#define VIDEO16_LINE_WRITE2 \
	dst16[0] = color; \
	dst16[1] = color; \
	dst16 += 2;

#define VIDEO16_LINE_WRITE3 \
	dst16[0] = color; \
	dst16[1] = color; \
	dst16[2] = color; \
	dst16 += 3;

static inline void video_line_stretchx16_1x_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;

	unsigned t = 0;
	unsigned s = 0;

	while (count) {
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		internal_fill16(dst, P16DER0(src), run);
		t += run;
		s += 1;
		PADD(dst, run*2);
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx16_1x(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_1x_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx16_1x_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_1x_step(stage, line, dst, src, 2, count);
}

static inline void video_line_stretchx16_x1_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO16_LINE_INIT

	while (count) {
		unsigned run = whole;
		unsigned color = P16DER0(src);
		VIDEO16_LINE_WRITE1
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		PADD(src, sdp * run);
		--count;
	}
}

static void video_line_stretchx16_x1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_x1_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx16_x1_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_x1_step(stage, line, dst, src, 2, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx16_11_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy16_mmx(dst, src, count);
}
#endif

static void video_line_stretchx16_11_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy16_def(dst, src, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx16_11_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy16_step_mmx(dst, src, count, stage->sdp);
}
#endif

static void video_line_stretchx16_11_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy16_step_def(dst, src, count, stage->sdp);
}

static inline void video_line_stretchx16_12_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO16_LINE_INIT

	while (count) {
		unsigned color = P16DER0(src);
		if ((error += up) > 0) {
			error -= down;
			VIDEO16_LINE_WRITE2
		} else {
			VIDEO16_LINE_WRITE1
		}
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx16_12(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_12_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx16_12_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_12_step(stage, line, dst, src, 2, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx16_22_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double16_mmx(dst, src, count);
}
#endif

static void video_line_stretchx16_22_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double16_def(dst, src, count);
}

static void video_line_stretchx16_22(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double16_step_def(dst, src, count, stage->sdp);
}

static inline void video_line_stretchx16_23_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO16_LINE_INIT

	while (count) {
		unsigned color = P16DER0(src);
		if ((error += up) > 0) {
			error -= down;
			VIDEO16_LINE_WRITE3
		} else {
			VIDEO16_LINE_WRITE2
		}
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx16_23(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_23_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx16_23_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx16_23_step(stage, line, dst, src, 2, count);
}

static void video_line_stretchx16_33_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_triple16_def(dst, src, count);
}

static void video_line_stretchx16_33(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_triple16_step_def(dst, src, count, stage->sdp);
}

static void video_line_stretchx16_44_step2(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_quadruple16_def(dst, src, count);
}

static void video_line_stretchx16_44(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_quadruple16_step_def(dst, src, count, stage->sdp);
}

static void video_stage_stretchx16_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_stretch, sdx, sdp, 2, ddx, 2);

	if (sdx > ddx) {
		STAGE_PUT(stage, video_line_stretchx16_x1_step2, video_line_stretchx16_x1);
	} else if (sdx == ddx) {
		STAGE_TYPE(stage, pipe_x_copy);
		STAGE_PUT(stage, BLITTER(video_line_stretchx16_11_step2), BLITTER(video_line_stretchx16_11));
	} else {
		if (stage->slice.whole == 1) {
			STAGE_PUT(stage, video_line_stretchx16_12_step2, video_line_stretchx16_12);
		} else if (stage->slice.whole == 2 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_double);
			STAGE_PUT(stage, BLITTER(video_line_stretchx16_22_step2), video_line_stretchx16_22);
		} else if (stage->slice.whole == 2) {
			STAGE_PUT(stage, video_line_stretchx16_23_step2, video_line_stretchx16_23);
		} else if (stage->slice.whole == 3 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_triple);
			STAGE_PUT(stage, video_line_stretchx16_33_step2, video_line_stretchx16_33);
		} else if (stage->slice.whole == 4 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_quadruple);
			STAGE_PUT(stage, video_line_stretchx16_44_step2, video_line_stretchx16_44);
		} else {
			STAGE_PUT(stage, video_line_stretchx16_1x_step2, video_line_stretchx16_1x);
		}
	}
}

/****************************************************************************/
/* stretchx32 */

#define VIDEO32_LINE_INIT \
	uint32* dst32 = (uint32*)dst;

#define VIDEO32_LINE_WRITE1 \
	dst32[0] = color; \
	dst32 += 1;

#define VIDEO32_LINE_WRITE2 \
	dst32[0] = color; \
	dst32[1] = color; \
	dst32 += 2;

#define VIDEO32_LINE_WRITE3 \
	dst32[0] = color; \
	dst32[1] = color; \
	dst32[2] = color; \
	dst32 += 3;

static inline void video_line_stretchx32_1x_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;

	while (count) {
		unsigned run = whole;
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		internal_fill32(dst, P32DER0(src), run);
		PADD(dst, run*4);
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx32_1x(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_1x_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx32_1x_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_1x_step(stage, line, dst, src, 4, count);
}

static inline void video_line_stretchx32_x1_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	unsigned whole = stage->slice.whole;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO32_LINE_INIT

	while (count) {
		unsigned run = whole;
		unsigned color = P32DER0(src);
		VIDEO32_LINE_WRITE1
		if ((error += up) > 0) {
			++run;
			error -= down;
		}
		PADD(src, sdp * run);
		--count;
	}
}

static void video_line_stretchx32_x1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_x1_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx32_x1_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_x1_step(stage, line, dst, src, 4, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx32_11_step4_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy32_mmx(dst, src, count);
}
#endif

static void video_line_stretchx32_11_step4_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy32_def(dst, src, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx32_11_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy32_step_mmx(dst, src, count, stage->sdp);
}
#endif

static void video_line_stretchx32_11_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_copy32_step_def(dst, src, count, stage->sdp);
}

static inline void video_line_stretchx32_12_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO32_LINE_INIT

	while (count) {
		unsigned color = P32DER0(src);
		if ((error += up) > 0) {
			error -= down;
			VIDEO32_LINE_WRITE2
		} else {
			VIDEO32_LINE_WRITE1
		}
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx32_12(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_12_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx32_12_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_12_step(stage, line, dst, src, 4, count);
}

#if defined(USE_ASM_INLINE)
static void video_line_stretchx32_22_step4_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double32_mmx(dst, src, count);
}
#endif

static void video_line_stretchx32_22_step4_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double32_def(dst, src, count);
}

static void video_line_stretchx32_22(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_double32_step_def(dst, src, count, stage->sdp);
}

static inline void video_line_stretchx32_23_step(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, int sdp, unsigned count)
{
	int error = stage->slice.error;
	int up = stage->slice.up;
	int down = stage->slice.down;

	VIDEO32_LINE_INIT

	while (count) {
		unsigned color = P32DER0(src);
		if ((error += up) > 0) {
			error -= down;
			VIDEO32_LINE_WRITE3
		} else {
			VIDEO32_LINE_WRITE2
		}
		PADD(src, sdp);
		--count;
	}
}

static void video_line_stretchx32_23(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_23_step(stage, line, dst, src, stage->sdp, count);
}

static void video_line_stretchx32_23_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	video_line_stretchx32_23_step(stage, line, dst, src, 4, count);
}

static void video_line_stretchx32_33_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_triple32_def(dst, src, count);
}

static void video_line_stretchx32_33(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_triple32_step_def(dst, src, count, stage->sdp);
}

static void video_line_stretchx32_44_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_quadruple32_def(dst, src, count);
}

static void video_line_stretchx32_44(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	internal_quadruple32_step_def(dst, src, count, stage->sdp);
}

static void video_stage_stretchx32_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_x_stretch, sdx, sdp, 4, ddx, 4);

	if (sdx > ddx) {
		STAGE_PUT(stage, video_line_stretchx32_x1_step4, video_line_stretchx32_x1);
	} else if (sdx == ddx) {
		STAGE_TYPE(stage, pipe_x_copy);
		STAGE_PUT(stage, BLITTER(video_line_stretchx32_11_step4), BLITTER(video_line_stretchx32_11));
	} else {
		if (stage->slice.whole == 1) {
			STAGE_PUT(stage, video_line_stretchx32_12_step4, video_line_stretchx32_12);
		} else if (stage->slice.whole == 2 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_double);
			STAGE_PUT(stage, BLITTER(video_line_stretchx32_22_step4), video_line_stretchx32_22);
		} else if (stage->slice.whole == 2) {
			STAGE_PUT(stage, video_line_stretchx32_23_step4, video_line_stretchx32_23);
		} else if (stage->slice.whole == 3 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_triple);
			STAGE_PUT(stage, video_line_stretchx32_33_step4, video_line_stretchx32_33);
		} else if (stage->slice.whole == 4 && stage->slice.up == 0) {
			STAGE_TYPE(stage, pipe_x_quadruple);
			STAGE_PUT(stage, video_line_stretchx32_44_step4, video_line_stretchx32_44);
		} else {
			STAGE_PUT(stage, video_line_stretchx32_1x_step4, video_line_stretchx32_1x);
		}
	}
}

#endif

