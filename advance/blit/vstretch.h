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

#ifndef __VSTRETCH_H
#define __VSTRETCH_H

#include "blit.h"

/****************************************************************************/
/* stretchx8 */

static uint32 mask8_set_all[256];

/* This code access only the ds segment */
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

static __inline__ void video_line_stretchx8_1x_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		unsigned run = stage->slice.whole;
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		internal_fill8(dst,P8DER0(src),run);
		PADD(dst,run);
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx8_1x(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_1x_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx8_1x_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_1x_step(stage, dst, src, 1);
}

static __inline__ void video_line_stretchx8_x1_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO8_LINE_INIT

	while (inner_count) {
		unsigned run = stage->slice.whole;
		unsigned color = P8DER0(src);
		VIDEO8_LINE_WRITE1
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src,sdp * run);
		--inner_count;

	}
}

static void video_line_stretchx8_x1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx8_x1_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_x1_step(stage, dst, src, 1);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx8_11_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy8_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx8_11_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy8_def(dst,src,stage->slice.count);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx8_11_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy8_step2_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx8_11_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy8_step2_def(dst,src,stage->slice.count);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx8_11_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy8_step_mmx(dst,src,stage->slice.count,stage->sdp);
}
#endif

static void video_line_stretchx8_11_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy8_step_def(dst,src,stage->slice.count,stage->sdp);
}

static __inline__ void video_line_stretchx8_12_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO8_LINE_INIT

	while (inner_count) {
		unsigned color = P8DER0(src);
		if ((error += stage->slice.up) > 0) {
			error -= stage->slice.down;
			VIDEO8_LINE_WRITE2
		} else {
			VIDEO8_LINE_WRITE1
		}
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx8_12(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_12_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx8_12_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_12_step(stage, dst, src, 1);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx8_22_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_double8_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx8_22_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_double8_def(dst,src,stage->slice.count);
}

static void video_line_stretchx8_22(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2;

	while (inner_count) {
		unsigned color0,color1;
		uint32 mask;
		color0 = P8DER0(src);
		PADD(src,stage->sdp);
		color1 = P8DER0(src);
		PADD(src,stage->sdp);
		mask = color0 | color0 << 8 | color1 << 16 | color1 << 24;
		P32DER0(dst) = mask;
		PADD(dst,4);
		--inner_count;
	}
}

static __inline__ void video_line_stretchx8_23_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO8_LINE_INIT

	while (inner_count) {
		unsigned color = P8DER0(src);
		if ((error += stage->slice.up) > 0) {
			error -= stage->slice.down;
			VIDEO8_LINE_WRITE3
		} else {
			VIDEO8_LINE_WRITE2
		}
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx8_23(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_23_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx8_23_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx8_23_step(stage, dst, src, 1);
}

static void video_line_stretchx8_33_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count / 4;

	while (inner_count) {
		unsigned color0 = P8DER(src,0);
		unsigned color1 = P8DER(src,1);
		unsigned color2 = P8DER(src,2);
		unsigned color3 = P8DER(src,3);
		*dst32++ = color0 | color0 << 8 | color0 << 16 | color1 << 24;
		*dst32++ = color1 | color1 << 8 | color2 << 16 | color2 << 24;
		*dst32++ = color2 | color3 << 8 | color3 << 16 | color3 << 24;
		PADD(src,4);
		--inner_count;
	}
}

static void video_line_stretchx8_33(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count / 4;

	while (inner_count) {
		unsigned color0;
		unsigned color1;
		unsigned color2;
		unsigned color3;
		color0 = P8DER0(src);
		PADD(src,stage->sdp);
		color1 = P8DER0(src);
		PADD(src,stage->sdp);
		color2 = P8DER0(src);
		PADD(src,stage->sdp);
		color3 = P8DER0(src);
		PADD(src,stage->sdp);
		*dst32++ = color0 | color0 << 8 | color0 << 16 | color1 << 24;
		*dst32++ = color1 | color1 << 8 | color2 << 16 | color2 << 24;
		*dst32++ = color2 | color3 << 8 | color3 << 16 | color3 << 24;
		--inner_count;
	}
}

static void video_line_stretchx8_44_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		*dst32++ = mask8_set_all[P8DER0(src)];
		PADD(src,1);
		--inner_count;
	}
}

static void video_line_stretchx8_44(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		*dst32++ = mask8_set_all[P8DER0(src)];
		PADD(src,stage->sdp);
		--inner_count;
	}
}

/* Inizialize the line rescaling system */
static void video_stage_stretchx8_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp) {
	STAGE_SIZE(stage, pipe_x_stretch, sdx, sdp, 1, ddx, 1);

	if (sdx > ddx) {
		stage->put_plain = video_line_stretchx8_x1_step1;
		stage->put = video_line_stretchx8_x1;
	} else if (sdx == ddx) {
		if (stage->sdp == 1) {
			stage->type = pipe_x_copy;
			stage->put_plain = BLITTER(video_line_stretchx8_11_step1);
			stage->put = BLITTER(video_line_stretchx8_11_step1);
		} else {
			stage->type = pipe_x_copy;
			stage->put_plain = BLITTER(video_line_stretchx8_11_step1);
			stage->put = BLITTER(video_line_stretchx8_11);
		}
	} else {
		if (stage->slice.whole == 1) {
			stage->put_plain = video_line_stretchx8_12_step1;
			stage->put = video_line_stretchx8_12;
		} else if (stage->slice.whole == 2 && stage->slice.up == 0 && stage->sdp == 1) {
			stage->type = pipe_x_double;
			stage->put_plain = BLITTER(video_line_stretchx8_22_step1);
			stage->put = BLITTER(video_line_stretchx8_22_step1);
		} else if (stage->slice.whole == 2 && stage->slice.up == 0) {
			stage->type = pipe_x_double;
			stage->put_plain = BLITTER(video_line_stretchx8_22_step1);
			stage->put = video_line_stretchx8_22;
		} else if (stage->slice.whole == 2) {
			stage->put_plain = video_line_stretchx8_23_step1;
			stage->put = video_line_stretchx8_23;
		} else if (stage->slice.whole == 3 && stage->slice.up == 0 && stage->sdp == 1) {
			stage->type = pipe_x_triple;
			stage->put_plain = video_line_stretchx8_33_step1;
			stage->put = video_line_stretchx8_33_step1;
		} else if (stage->slice.whole == 3 && stage->slice.up == 0) {
			stage->type = pipe_x_triple;
			stage->put_plain = video_line_stretchx8_33_step1;
			stage->put = video_line_stretchx8_33;
		} else if (stage->slice.whole == 4 && stage->slice.up == 0 && stage->sdp == 1) {
			stage->type = pipe_x_quadruple;
			stage->put_plain = video_line_stretchx8_44_step1;
			stage->put = video_line_stretchx8_44_step1;
		} else if (stage->slice.whole == 4 && stage->slice.up == 0) {
			stage->type = pipe_x_quadruple;
			stage->put_plain = video_line_stretchx8_44_step1;
			stage->put = video_line_stretchx8_44;
		} else {
			stage->put_plain = video_line_stretchx8_1x_step1;
			stage->put = video_line_stretchx8_1x;
		}
	}
}

/****************************************************************************/
/* stretchx16 */

/* This code access only the ds segment */
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

static __inline__ void video_line_stretchx16_1x_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		unsigned run = stage->slice.whole;
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		internal_fill16(dst,P16DER0(src),run);
		PADD(dst,run*2);
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx16_1x(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_1x_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx16_1x_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_1x_step(stage, dst, src, 2);
}

static __inline__ void video_line_stretchx16_x1_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO16_LINE_INIT

	while (inner_count) {
		unsigned run = stage->slice.whole;
		unsigned color = P16DER0(src);
		VIDEO16_LINE_WRITE1
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src,sdp * run);
		--inner_count;
	}
}

static void video_line_stretchx16_x1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx16_x1_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_x1_step(stage, dst, src, 2);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx16_11_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy16_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx16_11_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy16_def(dst,src,stage->slice.count);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx16_11_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy16_step_mmx(dst,src,stage->slice.count,stage->sdp);
}
#endif

static void video_line_stretchx16_11_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy16_step_def(dst,src,stage->slice.count,stage->sdp);
}

static __inline__ void video_line_stretchx16_12_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO16_LINE_INIT

	while (inner_count) {
		unsigned color = P16DER0(src);
		if ((error += stage->slice.up) > 0) {
			error -= stage->slice.down;
			VIDEO16_LINE_WRITE2
		} else {
			VIDEO16_LINE_WRITE1
		}
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx16_12(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_12_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx16_12_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_12_step(stage, dst, src, 2);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx16_22_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_double16_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx16_22_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_double16_def(dst,src,stage->slice.count);
}

static void video_line_stretchx16_22(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P16DER0(src);
		mask |= mask << 16;
		*dst32++ = mask;
		PADD(src,stage->sdp);
		--inner_count;
	}
}

static __inline__ void video_line_stretchx16_23_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO16_LINE_INIT

	while (inner_count) {
		unsigned color = P16DER0(src);
		if ((error += stage->slice.up) > 0) {
			error -= stage->slice.down;
			VIDEO16_LINE_WRITE3
		} else {
			VIDEO16_LINE_WRITE2
		}
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx16_23(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_23_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx16_23_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx16_23_step(stage, dst, src, 2);
}

static void video_line_stretchx16_33_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count / 2;

	while (inner_count) {
		uint32 mask0 = P16DER(src,0);
		uint32 mask2 = P16DER(src,2);
		uint32 mask1 = mask0 | mask2 << 16;
		mask0 |= mask0 << 16;
		mask2 |= mask2 << 16;
		*dst32++ = mask0;
		*dst32++ = mask1;
		*dst32++ = mask2;
		PADD(src,4);
		--inner_count;
	}
}

static void video_line_stretchx16_33(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count / 2;

	while (inner_count) {
		uint32 mask0,mask1,mask2;
		mask0 = P16DER0(src);
		PADD(src,stage->sdp);
		mask2 = P16DER0(src);
		PADD(src,stage->sdp);
		mask1 = mask0 | mask2 << 16;
		mask0 |= mask0 << 16;
		mask2 |= mask2 << 16;
		*dst32++ = mask0;
		*dst32++ = mask1;
		*dst32++ = mask2;
		--inner_count;
	}
}

static void video_line_stretchx16_44_step2(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P16DER0(src);
		mask |= mask << 16;
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,2);
		--inner_count;
	}
}

static void video_line_stretchx16_44(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P16DER0(src);
		mask |= mask << 16;
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,stage->sdp);
		--inner_count;
	}
}

static void video_stage_stretchx16_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp) {
	STAGE_SIZE(stage, pipe_x_stretch, sdx, sdp, 2, ddx, 2);

	if (sdx > ddx) {
		stage->put_plain = video_line_stretchx16_x1_step2;
		stage->put = video_line_stretchx16_x1;
	} else if (sdx == ddx) {
		if (stage->sdp == 2) {
			stage->type = pipe_x_copy;
			stage->put_plain = BLITTER(video_line_stretchx16_11_step2);
			stage->put = BLITTER(video_line_stretchx16_11_step2);
		} else {
			stage->type = pipe_x_copy;
			stage->put_plain = BLITTER(video_line_stretchx16_11_step2);
			stage->put = BLITTER(video_line_stretchx16_11);
		}
	} else {
		if (stage->slice.whole == 1) {
			stage->put_plain = video_line_stretchx16_12_step2;
			stage->put = video_line_stretchx16_12;
		} else if (stage->slice.whole == 2 && stage->slice.up == 0 && stage->sdp == 2) {
			stage->type = pipe_x_double;
			stage->put_plain = BLITTER(video_line_stretchx16_22_step2);
			stage->put = BLITTER(video_line_stretchx16_22_step2);
		} else if (stage->slice.whole == 2 && stage->slice.up == 0) {
			stage->type = pipe_x_double;
			stage->put_plain = BLITTER(video_line_stretchx16_22_step2);
			stage->put = video_line_stretchx16_22;
		} else if (stage->slice.whole == 2) {
			stage->put_plain = video_line_stretchx16_23_step2;
			stage->put = video_line_stretchx16_23;
		} else if (stage->slice.whole == 3 && stage->slice.up == 0 && stage->sdp == 2) {
			stage->type = pipe_x_triple;
			stage->put_plain = video_line_stretchx16_33_step2;
			stage->put = video_line_stretchx16_33_step2;
		} else if (stage->slice.whole == 3 && stage->slice.up == 0) {
			stage->type = pipe_x_triple;
			stage->put_plain = video_line_stretchx16_33_step2;
			stage->put = video_line_stretchx16_33;
		} else if (stage->slice.whole == 4 && stage->slice.up == 0 && stage->sdp == 2) {
			stage->type = pipe_x_quadruple;
			stage->put_plain = video_line_stretchx16_44_step2;
			stage->put = video_line_stretchx16_44_step2;
		} else if (stage->slice.whole == 4 && stage->slice.up == 0) {
			stage->type = pipe_x_quadruple;
			stage->put_plain = video_line_stretchx16_44_step2;
			stage->put = video_line_stretchx16_44;
		} else {
			stage->put_plain = (video_stage_hook*)video_line_stretchx16_1x_step2;
			stage->put = video_line_stretchx16_1x;
		}
	}
}

/****************************************************************************/
/* stretchx32 */

/* This code access only the ds segment */
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

static __inline__ void video_line_stretchx32_1x_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		unsigned run = stage->slice.whole;
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		internal_fill32(dst,P32DER0(src),run);
		PADD(dst,run*4);
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx32_1x(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_1x_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx32_1x_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_1x_step(stage, dst, src, 4);
}

static __inline__ void video_line_stretchx32_x1_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO32_LINE_INIT

	while (inner_count) {
		unsigned run = stage->slice.whole;
		unsigned color = P32DER0(src);
		VIDEO32_LINE_WRITE1
		if ((error += stage->slice.up) > 0) {
			++run;
			error -= stage->slice.down;
		}
		PADD(src,sdp * run);
		--inner_count;
	}
}

static void video_line_stretchx32_x1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_x1_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx32_x1_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_x1_step(stage, dst, src, 4);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx32_11_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy32_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx32_11_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy32_def(dst,src,stage->slice.count);
}

static void video_line_stretchx32_11_step3(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy32_step3(dst,src,stage->slice.count);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx32_11_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy32_step_mmx(dst,src,stage->slice.count,stage->sdp);
}
#endif

static void video_line_stretchx32_11_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_copy32_step_def(dst,src,stage->slice.count,stage->sdp);
}

static __inline__ void video_line_stretchx32_12_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO32_LINE_INIT

	while (inner_count) {
		unsigned color = P32DER0(src);
		if ((error += stage->slice.up) > 0) {
			error -= stage->slice.down;
			VIDEO32_LINE_WRITE2
		} else {
			VIDEO32_LINE_WRITE1
		}
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx32_12(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_12_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx32_12_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_12_step(stage, dst, src, 4);
}

#if defined(USE_ASM_i586)
static void video_line_stretchx32_22_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_double32_mmx(dst,src,stage->slice.count);
}
#endif

static void video_line_stretchx32_22_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	internal_double32_def(dst,src,stage->slice.count);
}

static void video_line_stretchx32_22(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P32DER0(src);
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,stage->sdp);
		--inner_count;
	}
}

static __inline__ void video_line_stretchx32_23_step(const struct video_stage_horz_struct* stage, void* dst, void* src, unsigned sdp) {
	int error = stage->slice.error;
	unsigned inner_count = stage->slice.count;

	VIDEO32_LINE_INIT

	while (inner_count) {
		unsigned color = P32DER0(src);
		if ((error += stage->slice.up) > 0) {
			error -= stage->slice.down;
			VIDEO32_LINE_WRITE3
		} else {
			VIDEO32_LINE_WRITE2
		}
		PADD(src,sdp);
		--inner_count;
	}
}

static void video_line_stretchx32_23(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_23_step(stage, dst, src, stage->sdp);
}

static void video_line_stretchx32_23_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	video_line_stretchx32_23_step(stage, dst, src, 4);
}

static void video_line_stretchx32_33_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P32DER0(src);
		*dst32++ = mask;
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,4);
		--inner_count;
	}
}

static void video_line_stretchx32_33(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P32DER0(src);
		*dst32++ = mask;
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,stage->sdp);
		--inner_count;
	}
}

static void video_line_stretchx32_44_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P32DER0(src);
		*dst32++ = mask;
		*dst32++ = mask;
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,4);
		--inner_count;
	}
}

static void video_line_stretchx32_44(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	uint32* dst32 = (uint32*)dst;
	unsigned inner_count = stage->slice.count;

	while (inner_count) {
		uint32 mask = P32DER0(src);
		*dst32++ = mask;
		*dst32++ = mask;
		*dst32++ = mask;
		*dst32++ = mask;
		PADD(src,stage->sdp);
		--inner_count;
	}
}

static void video_stage_stretchx32_set(struct video_stage_horz_struct* stage, unsigned ddx, unsigned sdx, int sdp) {
	STAGE_SIZE(stage, pipe_x_stretch, sdx, sdp, 4, ddx, 4);

	if (sdx > ddx) {
		stage->put_plain = video_line_stretchx32_x1_step4;
		stage->put = video_line_stretchx32_x1;
	} else if (sdx == ddx) {
		if (stage->sdp == 4) {
			stage->type = pipe_x_copy;
			stage->put_plain = BLITTER(video_line_stretchx32_11_step4);
			stage->put = BLITTER(video_line_stretchx32_11_step4);
		} else {
			stage->type = pipe_x_copy;
			stage->put_plain = BLITTER(video_line_stretchx32_11_step4);
			stage->put = BLITTER(video_line_stretchx32_11);
		}
	} else {
		if (stage->slice.whole == 1) {
			stage->put_plain = video_line_stretchx32_12_step4;
			stage->put = video_line_stretchx32_12;
		} else if (stage->slice.whole == 2 && stage->slice.up == 0 && stage->sdp == 4) {
			stage->type = pipe_x_double;
			stage->put_plain = BLITTER(video_line_stretchx32_22_step4);
			stage->put = BLITTER(video_line_stretchx32_22_step4);
		} else if (stage->slice.whole == 2 && stage->slice.up == 0) {
			stage->type = pipe_x_double;
			stage->put_plain = BLITTER(video_line_stretchx32_22_step4);
			stage->put = video_line_stretchx32_22;
		} else if (stage->slice.whole == 2) {
			stage->put_plain = video_line_stretchx32_23_step4;
			stage->put = video_line_stretchx32_23;
		} else if (stage->slice.whole == 3 && stage->slice.up == 0 && stage->sdp == 4) {
			stage->type = pipe_x_triple;
			stage->put_plain = video_line_stretchx32_33_step4;
			stage->put = video_line_stretchx32_33_step4;
		} else if (stage->slice.whole == 3 && stage->slice.up == 0) {
			stage->type = pipe_x_triple;
			stage->put_plain = video_line_stretchx32_33_step4;
			stage->put = video_line_stretchx32_33;
		} else if (stage->slice.whole == 4 && stage->slice.up == 0 && stage->sdp == 4) {
			stage->type = pipe_x_quadruple;
			stage->put_plain = video_line_stretchx32_44_step4;
			stage->put = video_line_stretchx32_44_step4;
		} else if (stage->slice.whole == 4 && stage->slice.up == 0) {
			stage->type = pipe_x_quadruple;
			stage->put_plain = video_line_stretchx32_44_step4;
			stage->put = video_line_stretchx32_44;
		} else {
			stage->put_plain = video_line_stretchx32_1x_step4;
			stage->put = video_line_stretchx32_1x;
		}
	}
}

#endif
