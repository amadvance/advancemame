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

#ifndef __VCONV_H
#define __VCONV_H

#include "blit.h"

/****************************************************************************/
/* rgb 8888 to 332 */

enum RGB_8888TO332_MASK {
	RGB_8888TO332_MASK_R_0,
	RGB_8888TO332_MASK_R_1,
	RGB_8888TO332_MASK_G_0,
	RGB_8888TO332_MASK_G_1,
	RGB_8888TO332_MASK_B_0,
	RGB_8888TO332_MASK_B_1,
	RGB_8888TO332_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 rgb8888to332_mask[RGB_8888TO332_MASK_MAX] = {
	0x000000E0, 0x000000E0, /* r */
	0x0000001C, 0x0000001C, /* g */
	0x00000003, 0x00000003  /* b */
};

static void video_line_rgb8888to332_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	unsigned rest = count % 8;

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm5\n"
		"movq 8(%3),%%mm6\n"
		"movq 16(%3),%%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"movq %%mm0,%%mm2\n"
		"psrlq $16,%%mm0\n"
		"psrlq $11,%%mm1\n"
		"psrlq $6,%%mm2\n"
		"pand %%mm5,%%mm0\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm0\n"

		"movq 8(%0),%%mm3\n"
		"movq %%mm3,%%mm1\n"
		"movq %%mm3,%%mm2\n"
		"psrlq $16,%%mm3\n"
		"psrlq $11,%%mm1\n"
		"psrlq $6,%%mm2\n"
		"pand %%mm5,%%mm3\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm3\n"

		"packuswb %%mm3,%%mm0\n"

		"movq 16(%0),%%mm4\n"
		"movq %%mm4,%%mm1\n"
		"movq %%mm4,%%mm2\n"
		"psrlq $16,%%mm4\n"
		"psrlq $11,%%mm1\n"
		"psrlq $6,%%mm2\n"
		"pand %%mm5,%%mm4\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm4\n"

		"movq 24(%0),%%mm3\n"
		"movq %%mm3,%%mm1\n"
		"movq %%mm3,%%mm2\n"
		"psrlq $16,%%mm3\n"
		"psrlq $11,%%mm1\n"
		"psrlq $6,%%mm2\n"
		"pand %%mm5,%%mm3\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm3\n"

		"packuswb %%mm3,%%mm4\n"

		"packuswb %%mm4,%%mm0\n"

		"movq %%mm0,(%1)\n"
		"addl $32,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (rgb8888to332_mask)
		: "cc"
	);

	if (rest) {
		uint32* src32 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src32[0] >> (8-2)) & 0x03)
				| ((src32[0] >> (16-3-2)) & 0x1C)
				| ((src32[0] >> (24-3-3-2)) & 0xE0);
			++src32;
			++dst8;
			--rest;
		} while (rest);
	}
}
#endif

static void video_line_rgb8888to332_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count / 4;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src32[0] >> (8-2)) & 0x03)
			| ((src32[0] >> (16-3-2)) & 0x1C)
			| ((src32[0] >> (24-3-3-2)) & 0xE0)
			| ((src32[1] << -(8-2-8)) & 0x0300)
			| ((src32[1] >> (16-3-2-8)) & 0x1C00)
			| ((src32[1] >> (24-3-3-2-8)) & 0xE000)
			| ((src32[2] << -(8-2-16)) & 0x030000)
			| ((src32[2] << -(16-3-2-16)) & 0x1C0000)
			| ((src32[2] >> (24-3-3-2-16)) & 0xE00000)
			| ((src32[3] << -(8-2-24)) & 0x03000000)
			| ((src32[3] << -(16-3-2-24)) & 0x1C000000)
			| ((src32[3] << -(24-3-3-2-24)) & 0xE0000000);
		src32 += 4;
		--count;
	}
}

static void video_stage_rgb8888to332_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_rgb8888to332,sdx,sdp,4,1,BLITTER(video_line_rgb8888to332_step4),0);
}

/****************************************************************************/
/* rgb 8888 to 565 */

enum RGB_8888TO565_MASK {
	RGB_8888TO565_MASK_R_0,
	RGB_8888TO565_MASK_R_1,
	RGB_8888TO565_MASK_G_0,
	RGB_8888TO565_MASK_G_1,
	RGB_8888TO565_MASK_B_0,
	RGB_8888TO565_MASK_B_1,
	RGB_8888TO565_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 rgb8888to565_mask[RGB_8888TO565_MASK_MAX] = {
	0x00F80000, 0x00F80000, /* r << 8 */
	0x000007E0, 0x000007E0, /* g */
	0x0000001F, 0x0000001F  /* b */
};

static void video_line_rgb8888to565_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm5\n"
		"movq 8(%3),%%mm6\n"
		"movq 16(%3),%%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"movq %%mm0,%%mm2\n"
		"psrld $5,%%mm1\n"
		"psrld $3,%%mm2\n"
		"pand %%mm5,%%mm0\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"

		"movq 8(%0),%%mm3\n"
		"movq %%mm3,%%mm4\n"
		"movq %%mm3,%%mm2\n"
		"psrld $5,%%mm4\n"
		"psrld $3,%%mm2\n"
		"pand %%mm5,%%mm3\n"
		"pand %%mm6,%%mm4\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm4\n"

		"packuswb %%mm3,%%mm0\n"
		"packssdw %%mm4,%%mm1\n"
		"por %%mm1,%%mm0\n"

		"movq %%mm0,(%1)\n"
		"addl $16,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (rgb8888to565_mask)
		: "cc"
	);

	if (rest) {
		uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8-5)) & 0x001F)
				| ((src32[0] >> (16-5-6)) & 0x07E0)
				| ((src32[0] >> (24-5-6-5)) & 0xF800);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

static void video_line_rgb8888to565_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count / 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src32[0] >> (8-5)) & 0x001F)
			| ((src32[0] >> (16-5-6)) & 0x07E0)
			| ((src32[0] >> (24-5-6-5)) & 0xF800)
			| ((src32[1] << -(8-5-16)) & 0x001F0000)
			| ((src32[1] << -(16-5-6-16)) & 0x07E00000)
			| ((src32[1] << -(24-5-6-5-16)) & 0xF8000000);
		src32 += 2;
		--count;
	}
}

static void video_stage_rgb8888to565_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_rgb8888to565,sdx,sdp,4,2,BLITTER(video_line_rgb8888to565_step4),0);
}

/****************************************************************************/
/* rgb 8888 to 555 */

enum RGB_8888TO555_MASK {
	RGB_8888TO555_MASK_R_0,
	RGB_8888TO555_MASK_R_1,
	RGB_8888TO555_MASK_G_0,
	RGB_8888TO555_MASK_G_1,
	RGB_8888TO555_MASK_B_0,
	RGB_8888TO555_MASK_B_1,
	RGB_8888TO555_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 rgb8888to555_mask[RGB_8888TO555_MASK_MAX] = {
	0x00007C00, 0x00007C00, /* r */
	0x000003E0, 0x000003E0, /* g */
	0x0000001F, 0x0000001F  /* b */
};

static void video_line_rgb8888to555_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm5\n"
		"movq 8(%3),%%mm6\n"
		"movq 16(%3),%%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"movq %%mm0,%%mm2\n"
		"psrld $9,%%mm0\n"
		"psrld $6,%%mm1\n"
		"psrld $3,%%mm2\n"
		"pand %%mm5,%%mm0\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm0\n"

		"movq 8(%0),%%mm3\n"
		"movq %%mm3,%%mm1\n"
		"movq %%mm3,%%mm2\n"
		"psrld $9,%%mm3\n"
		"psrld $6,%%mm1\n"
		"psrld $3,%%mm2\n"
		"pand %%mm5,%%mm3\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm3\n"

		"packssdw %%mm3,%%mm0\n"

		"movq %%mm0,(%1)\n"
		"addl $16,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (rgb8888to555_mask)
		: "cc"
	);

	if (rest) {
		uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8-5)) & 0x001F)
				| ((src32[0] >> (16-5-5)) & 0x03E0)
				| ((src32[0] >> (24-5-5-5)) & 0x7C00);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

static void video_line_rgb8888to555_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count / 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src32[0] >> (8-5)) & 0x001F)
			| ((src32[0] >> (16-5-5)) & 0x03E0)
			| ((src32[0] >> (24-5-5-5)) & 0x7C00)
			| ((src32[1] << -(8-5-16)) & 0x001F0000)
			| ((src32[1] << -(16-5-5-16)) & 0x03E00000)
			| ((src32[1] << -(24-5-5-5-16)) & 0x7C000000);
		src32 += 2;
		--count;
	}
}

static void video_stage_rgb8888to555_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_rgb8888to555,sdx,sdp,4,2,BLITTER(video_line_rgb8888to555_step4),0);
}

/****************************************************************************/
/* rgb 555 to 332 */

enum RGB_555TO332_MASK {
	RGB_555TO332_MASK_R_0,
	RGB_555TO332_MASK_R_1,
	RGB_555TO332_MASK_G_0,
	RGB_555TO332_MASK_G_1,
	RGB_555TO332_MASK_B_0,
	RGB_555TO332_MASK_B_1,
	RGB_555TO332_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 rgb555to332_mask[RGB_555TO332_MASK_MAX] = {
	0x00E000E0, 0x00E000E0, /* r */
	0x001C001C, 0x001C001C, /* g */
	0x00030003, 0x00030003  /* b */
};

static void video_line_rgb555to332_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	unsigned rest = count % 8;

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm5\n"
		"movq 8(%3),%%mm6\n"
		"movq 16(%3),%%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"movq %%mm0,%%mm2\n"
		"psrld $7,%%mm0\n"
		"psrld $5,%%mm1\n"
		"psrld $3,%%mm2\n"
		"pand %%mm5,%%mm0\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm0\n"

		"movq 8(%0),%%mm3\n"
		"movq %%mm3,%%mm1\n"
		"movq %%mm3,%%mm2\n"
		"psrld $7,%%mm3\n"
		"psrld $5,%%mm1\n"
		"psrld $3,%%mm2\n"
		"pand %%mm5,%%mm3\n"
		"pand %%mm6,%%mm1\n"
		"pand %%mm7,%%mm2\n"
		"por %%mm2,%%mm1\n"
		"por %%mm1,%%mm3\n"

		"packuswb %%mm3,%%mm0\n"

		"movq %%mm0,(%1)\n"
		"addl $16,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (rgb555to332_mask)
		: "cc"
	);

	if (rest) {
		uint16* src16 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src16[0] >> (5-2)) & 0x03)
				| ((src16[0] >> (10-3-2)) & 0x1C)
				| ((src16[0] >> (15-3-3-2)) & 0xE0);
			++src16;
			++dst8;
			--rest;
		} while (rest);
	}
}
#endif

static void video_line_rgb555to332_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count / 4;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src16[0] >> (5-2)) & 0x03)
			| ((src16[0] >> (10-3-2)) & 0x1C)
			| ((src16[0] >> (15-3-3-2)) & 0xE0)
			| ((src16[1] << -(5-2-8)) & 0x0300)
			| ((src16[1] << -(10-3-2-8)) & 0x1C00)
			| ((src16[1] << -(15-3-3-2-8)) & 0xE000)
			| ((src16[2] << -(5-2-16)) & 0x030000)
			| ((src16[2] << -(10-3-2-16)) & 0x1C0000)
			| ((src16[2] << -(15-3-3-2-16)) & 0xE00000)
			| ((src16[3] << -(5-2-24)) & 0x03000000)
			| ((src16[3] << -(10-3-2-24)) & 0x1C000000)
			| ((src16[3] << -(15-3-3-2-24)) & 0xE0000000);
		src16 += 4;
		--count;
	}
}

static void video_stage_rgb555to332_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_rgb555to332,sdx,sdp,2,1,BLITTER(video_line_rgb555to332_step2),0);
}

/****************************************************************************/
/* rgb 555 to 565 */

enum RGB_555TO565_MASK {
	RGB_555TO565_MASK_RG_0,
	RGB_555TO565_MASK_RG_1,
	RGB_555TO565_MASK_B_0,
	RGB_555TO565_MASK_B_1,
	RGB_555TO565_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 rgb555to565_mask[RGB_555TO565_MASK_MAX] = {
	0xFFC0FFC0, 0xFFC0FFC0, /* rg */
	0x001F001F, 0x001F001F /* b */
};

static void video_line_rgb555to565_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm2\n"
		"movq 8(%3),%%mm3\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"pslld $1,%%mm0\n"
		"pand %%mm2,%%mm0\n"
		"pand %%mm3,%%mm1\n"
		"por %%mm1,%%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (rgb555to565_mask)
		: "cc"
	);

	if (rest) {
		uint16* src16 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = (src16[0] & 0x001F)
				| ((src16[0] << 1) & 0xFFC0);
			++src16;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

static void video_line_rgb555to565_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count / 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = (src32[0] & 0x001F001F)
			| ((src32[0] << 1) & 0xFFC0FFC0);
		src32 += 1;
		--count;
	}
}

static void video_stage_rgb555to565_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_rgb555to565,sdx,sdp,2,2,BLITTER(video_line_rgb555to565_step2),0);
}

/****************************************************************************/
/* rgb 555 to 8888 */

enum RGB_555TO8888_MASK {
	RGB_555TO8888_MASK_R_0,
	RGB_555TO8888_MASK_R_1,
	RGB_555TO8888_MASK_G_0,
	RGB_555TO8888_MASK_G_1,
	RGB_555TO8888_MASK_B_0,
	RGB_555TO8888_MASK_B_1,
	RGB_555TO8888_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 rgb555to8888_mask[RGB_555TO8888_MASK_MAX] = {
	0x000000F8, 0x000000F8, /* r */
	0x0000F800, 0x0000F800, /* g */
	0x00F80000, 0x00F80000 /* b */
};

static void video_line_rgb555to8888_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	unsigned rest = count % 2;

	__asm__ __volatile__(
		"shrl $1,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm3\n"
		"movq 8(%3),%%mm4\n"
		"movq 16(%3),%%mm5\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movd (%0),%%mm0\n"
		"punpcklwd %%mm0,%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"movq %%mm0,%%mm2\n"
		"pslld $3,%%mm0\n"
		"pslld $6,%%mm1\n"
		"pslld $9,%%mm2\n"
		"pand %%mm3,%%mm0\n"
		"pand %%mm4,%%mm1\n"
		"pand %%mm5,%%mm2\n"
		"por %%mm1,%%mm0\n"
		"por %%mm2,%%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $4,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (rgb555to8888_mask)
		: "cc"
	);

	if (rest) {
		uint16* src16 = src;
		uint32* dst32 = dst;
		do {
			*dst32 = ((src16[0] << 3) & 0x000000F8)
				| ((src16[0] << 6) & 0x0000F800)
				| ((src16[0] << 9) & 0x00F80000);
			++src16;
			++dst32;
			--rest;
		} while (rest);
	}
}
#endif

static void video_line_rgb555to8888_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src16[0] << 3) & 0x000000F8)
			| ((src16[0] << 6) & 0x0000F800)
			| ((src16[0] << 9) & 0x00F80000);
		src16 += 1;
		--count;
	}
}

static void video_stage_rgb555to8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE(stage,pipe_rgb555to8888,sdx,sdp,2,4,BLITTER(video_line_rgb555to8888_step2),0);
}

/****************************************************************************/
/* rgb RGB888 to 8888 */

static void video_line_rgbRGB888to8888_step3(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count >= 2) {
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		/* dst8[3] = 0; */
		dst8[4] = src8[5];
		dst8[5] = src8[4];
		dst8[6] = src8[3];
		/* dst8[7] = 0; */
		dst8 += 8;
		src8 += 6;
		count -= 2;
	}

	if (count) {
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		/* dst8[3] = 0; */
	}
}

static void video_line_rgbRGB888to8888_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count >= 2) {
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		/* dst8[3] = 0; */
		dst8[4] = src8[6];
		dst8[5] = src8[5];
		dst8[6] = src8[4];
		/* dst8[7] = 0; */
		dst8 += 8;
		src8 += 8;
		count -= 2;
	}

	if (count) {
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		/* dst8[3] = 0; */
	}
}

static void video_line_rgbRGB888to8888_step(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		dst8[0] = src8[2];
		dst8[1] = src8[1];
		dst8[2] = src8[0];
		/* dst8[3] = 0; */
		dst8 += 4;
		src8 += step1;
		--count;
	}
}

static void video_stage_rgbRGB888to8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp) {
	STAGE_SIZE(stage,pipe_rgbRGB888to8888,sdx,sdp,3,4);

	stage->plane_put = 0;
	stage->plane_put_plain = 0;
	stage->put_plain = video_line_rgbRGB888to8888_step3;
	if (sdp == 3)
		stage->put = video_line_rgbRGB888to8888_step3;
	else if (sdp == 4)
		stage->put = video_line_rgbRGB888to8888_step4;
	else
		stage->put = video_line_rgbRGB888to8888_step;
}

#endif
