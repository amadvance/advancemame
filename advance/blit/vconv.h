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
/* bgra8888 to bgr332 */

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
static uint32 bgra8888tobgr332_mask[RGB_8888TO332_MASK_MAX] = {
	0x000000E0, 0x000000E0, /* r */
	0x0000001C, 0x0000001C, /* g */
	0x00000003, 0x00000003  /* b */
};

static void video_line_bgra8888tobgr332_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	unsigned rest = count % 8;

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrlq $16, %%mm0\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrlq $16, %%mm3\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packuswb %%mm3, %%mm0\n"

		"movq 16(%0), %%mm4\n"
		"movq %%mm4, %%mm1\n"
		"movq %%mm4, %%mm2\n"
		"psrlq $16, %%mm4\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm4\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm4\n"

		"movq 24(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrlq $16, %%mm3\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packuswb %%mm3, %%mm4\n"

		"packuswb %%mm4, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $32, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra8888tobgr332_mask)
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

static void video_line_bgra8888tobgr332_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count / 4;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
#ifdef USE_LSB
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
#else
		*dst32++ = ((src32[3] >> (8-2)) & 0x03)
			| ((src32[3] >> (16-3-2)) & 0x1C)
			| ((src32[3] >> (24-3-3-2)) & 0xE0)
			| ((src32[2] << -(8-2-8)) & 0x0300)
			| ((src32[2] >> (16-3-2-8)) & 0x1C00)
			| ((src32[2] >> (24-3-3-2-8)) & 0xE000)
			| ((src32[1] << -(8-2-16)) & 0x030000)
			| ((src32[1] << -(16-3-2-16)) & 0x1C0000)
			| ((src32[1] >> (24-3-3-2-16)) & 0xE00000)
			| ((src32[0] << -(8-2-24)) & 0x03000000)
			| ((src32[0] << -(16-3-2-24)) & 0x1C000000)
			| ((src32[0] << -(24-3-3-2-24)) & 0xE0000000);
#endif
		src32 += 4;
		--count;
	}
}

static void video_stage_bgra8888tobgr332_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888tobgr332, sdx, sdp, 4, sdx, 1);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888tobgr332_step4), 0);
}

/****************************************************************************/
/* bgra8888 to bgr565 */

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
static uint32 bgra8888tobgr565_mask[RGB_8888TO565_MASK_MAX] = {
	0x00F80000, 0x00F80000, /* r << 8 */
	0x000007E0, 0x000007E0, /* g */
	0x0000001F, 0x0000001F  /* b */
};

static void video_line_bgra8888tobgr565_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrld $5, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm4\n"
		"movq %%mm3, %%mm2\n"
		"psrld $5, %%mm4\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm4\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm4\n"

		"packuswb %%mm3, %%mm0\n"
		"packssdw %%mm4, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra8888tobgr565_mask)
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

static void video_line_bgra8888tobgr565_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count / 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8-5)) & 0x001F)
			| ((src32[0] >> (16-5-6)) & 0x07E0)
			| ((src32[0] >> (24-5-6-5)) & 0xF800)
			| ((src32[1] << -(8-5-16)) & 0x001F0000)
			| ((src32[1] << -(16-5-6-16)) & 0x07E00000)
			| ((src32[1] << -(24-5-6-5-16)) & 0xF8000000);
#else
		*dst32++ = ((src32[1] >> (8-5)) & 0x001F)
			| ((src32[1] >> (16-5-6)) & 0x07E0)
			| ((src32[1] >> (24-5-6-5)) & 0xF800)
			| ((src32[0] << -(8-5-16)) & 0x001F0000)
			| ((src32[0] << -(16-5-6-16)) & 0x07E00000)
			| ((src32[0] << -(24-5-6-5-16)) & 0xF8000000);

#endif
		src32 += 2;
		--count;
	}
}

static void video_stage_bgra8888tobgr565_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888tobgr565, sdx, sdp, 4, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888tobgr565_step4), 0);
}

/****************************************************************************/
/* bgra8888 to bgra5551 */

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
static uint32 bgra8888tobgra5551_mask[RGB_8888TO555_MASK_MAX] = {
	0x00007C00, 0x00007C00, /* r */
	0x000003E0, 0x000003E0, /* g */
	0x0000001F, 0x0000001F  /* b */
};

static void video_line_bgra8888tobgra5551_step4_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrld $9, %%mm0\n"
		"psrld $6, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrld $9, %%mm3\n"
		"psrld $6, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packssdw %%mm3, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra8888tobgra5551_mask)
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

static void video_line_bgra8888tobgra5551_step4_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count / 2;
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8-5)) & 0x001F)
			| ((src32[0] >> (16-5-5)) & 0x03E0)
			| ((src32[0] >> (24-5-5-5)) & 0x7C00)
			| ((src32[1] << -(8-5-16)) & 0x001F0000)
			| ((src32[1] << -(16-5-5-16)) & 0x03E00000)
			| ((src32[1] << -(24-5-5-5-16)) & 0x7C000000);
#else
		*dst32++ = ((src32[1] >> (8-5)) & 0x001F)
			| ((src32[1] >> (16-5-5)) & 0x03E0)
			| ((src32[1] >> (24-5-5-5)) & 0x7C00)
			| ((src32[0] << -(8-5-16)) & 0x001F0000)
			| ((src32[0] << -(16-5-5-16)) & 0x03E00000)
			| ((src32[0] << -(24-5-5-5-16)) & 0x7C000000);
#endif
		src32 += 2;
		--count;
	}
}

static void video_stage_bgra8888tobgra5551_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra8888tobgra5551, sdx, sdp, 4, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_bgra8888tobgra5551_step4), 0);
}

/****************************************************************************/
/* bgra5551 to bgr332 */

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
static uint32 bgra5551tobgr332_mask[RGB_555TO332_MASK_MAX] = {
	0x00E000E0, 0x00E000E0, /* r */
	0x001C001C, 0x001C001C, /* g */
	0x00030003, 0x00030003  /* b */
};

static void video_line_bgra5551tobgr332_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	unsigned rest = count % 8;

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrld $7, %%mm0\n"
		"psrld $5, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrld $7, %%mm3\n"
		"psrld $5, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packuswb %%mm3, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra5551tobgr332_mask)
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

static void video_line_bgra5551tobgr332_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count / 4;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
#ifdef USE_LSB
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
#else
		*dst32++ = ((src16[3] >> (5-2)) & 0x03)
			| ((src16[3] >> (10-3-2)) & 0x1C)
			| ((src16[3] >> (15-3-3-2)) & 0xE0)
			| ((src16[2] << -(5-2-8)) & 0x0300)
			| ((src16[2] << -(10-3-2-8)) & 0x1C00)
			| ((src16[2] << -(15-3-3-2-8)) & 0xE000)
			| ((src16[1] << -(5-2-16)) & 0x030000)
			| ((src16[1] << -(10-3-2-16)) & 0x1C0000)
			| ((src16[1] << -(15-3-3-2-16)) & 0xE00000)
			| ((src16[0] << -(5-2-24)) & 0x03000000)
			| ((src16[0] << -(10-3-2-24)) & 0x1C000000)
			| ((src16[0] << -(15-3-3-2-24)) & 0xE0000000);
#endif
		src16 += 4;
		--count;
	}
}

static void video_stage_bgra5551tobgr332_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551tobgr332, sdx, sdp, 2, sdx, 1);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551tobgr332_step2), 0);
}

/****************************************************************************/
/* bgra5551 to bgr565 */

enum RGB_555TO565_MASK {
	RGB_555TO565_MASK_RG_0,
	RGB_555TO565_MASK_RG_1,
	RGB_555TO565_MASK_B_0,
	RGB_555TO565_MASK_B_1,
	RGB_555TO565_MASK_MAX
};

#if defined(USE_ASM_i586)
static uint32 bgra5551tobgr565_mask[RGB_555TO565_MASK_MAX] = {
	0xFFC0FFC0, 0xFFC0FFC0, /* rg */
	0x001F001F, 0x001F001F /* b */
};

static void video_line_bgra5551tobgr565_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm2\n"
		"movq 8(%3), %%mm3\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"pslld $1, %%mm0\n"
		"pand %%mm2, %%mm0\n"
		"pand %%mm3, %%mm1\n"
		"por %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra5551tobgr565_mask)
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

static void video_line_bgra5551tobgr565_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
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

static void video_stage_bgra5551tobgr565_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551tobgr565, sdx, sdp, 2, sdx, 2);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551tobgr565_step2), 0);
}

/****************************************************************************/
/* bgra5551 to bgra8888 */

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
static uint32 bgra5551tobgra8888_mask[RGB_555TO8888_MASK_MAX] = {
	0x000000F8, 0x000000F8, /* r */
	0x0000F800, 0x0000F800, /* g */
	0x00F80000, 0x00F80000 /* b */
};

static void video_line_bgra5551tobgra8888_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	unsigned rest = count % 2;

	__asm__ __volatile__(
		"shrl $1, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm3\n"
		"movq 8(%3), %%mm4\n"
		"movq 16(%3), %%mm5\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movd (%0), %%mm0\n"
		"punpcklwd %%mm0, %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"pslld $3, %%mm0\n"
		"pslld $6, %%mm1\n"
		"pslld $9, %%mm2\n"
		"pand %%mm3, %%mm0\n"
		"pand %%mm4, %%mm1\n"
		"pand %%mm5, %%mm2\n"
		"por %%mm1, %%mm0\n"
		"por %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra5551tobgra8888_mask)
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

static void video_line_bgra5551tobgra8888_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
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

static void video_stage_bgra5551tobgra8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_bgra5551tobgra8888, sdx, sdp, 2, sdx, 4);
	STAGE_PUT(stage, BLITTER(video_line_bgra5551tobgra8888_step2), 0);
}

/****************************************************************************/
/* rgb888 to bgra8888 */

static void video_line_rgb888tobgra8888_step3(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
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

static void video_line_rgb888tobgra8888_step4(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
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

static void video_line_rgb888tobgra8888_step(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
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

static void video_stage_rgb888tobgra8888_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_rgb888tobgra8888, sdx, sdp, 3, sdx, 4);

	stage->put_plain = video_line_rgb888tobgra8888_step3;
	if (sdp == 3)
		stage->put = video_line_rgb888tobgra8888_step3;
	else if (sdp == 4)
		stage->put = video_line_rgb888tobgra8888_step4;
	else
		stage->put = video_line_rgb888tobgra8888_step;
}

/****************************************************************************/
/* rgb888 to bgra8888 */

static void video_line_bgr888tobgra8888_step3(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count >= 2) {
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		/* dst8[3] = 0; */
		dst8[4] = src8[3];
		dst8[5] = src8[4];
		dst8[6] = src8[5];
		/* dst8[7] = 0; */
		dst8 += 8;
		src8 += 6;
		count -= 2;
	}

	if (count) {
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		/* dst8[3] = 0; */
	}
}

static void video_line_bgr888tobgra8888_step(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		dst8[0] = src8[0];
		dst8[1] = src8[1];
		dst8[2] = src8[2];
		/* dst8[3] = 0; */
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

#if defined(USE_ASM_i586)
/*
	Y =  0.299  R + 0.587  G + 0.114  B
	U = -0.1687 R - 0.3313 G + 0.5    B + 128
	V =  0.5    R - 0.4187 G - 0.0813 B + 128
*/

static uint32 bgra8888toyuy2_coeff[] = {
	/*uuuuyyyy    vvvvyyyy */
	0x0080001d, 0xffec001d, /* b */
	0xffac0096, 0xff950096, /* g */
	0xffd5004c, 0x0080004c, /* r */
	0x80000000, 0x80000000  /* add */
};

static inline void bgra8888toyuy2_mmx(void* dst, const void* src0, const void* src1)
{
	__asm__ __volatile__ (

#if 0
/* Basic single pixel implementation */

		/* mm0 = 0 a 0 r 0 g 0 b */

		/* transpose */
		"movq %%mm2, %%mm1\n"
		"punpcklwd %%mm2, %%mm1\n"
		"punpckhwd %%mm2, %%mm2\n"
		"movq %%mm1, %%mm0\n"
		"punpckldq %%mm2, %%mm2\n"
		"punpckldq %%mm1, %%mm0\n"
		"punpckhdq %%mm1, %%mm1\n"

		/* mm0 = 0 b 0 b 0 b 0 b */
		/* mm1 = 0 g 0 g 0 g 0 g */
		/* mm2 = 0 r 0 r 0 r 0 r */

		/* multiply */
		"pmullw 0(%3), %%mm0\n"
		"pmullw 8(%3), %%mm1\n"
		"pmullw 16(%3), %%mm2\n"

		/* add the component without saturation */
		"paddw %%mm1, %%mm0\n"
		"paddw 24(%3), %%mm2\n"
		"paddw %%mm2, %%mm0\n"

		/* reduce the precision */
		"psrlw $8, %%mm0\n"

		/* mm0 = 0 v 0 y 0 u 0 y */
#endif

/* Fast double pixel implementation */
		"movd (%0), %%mm2\n"
		"movd (%1), %%mm5\n"
		"pxor %%mm0, %%mm0\n"
		"punpcklbw %%mm0, %%mm2\n"
		"movq %%mm2, %%mm1\n"
		"punpcklwd %%mm2, %%mm1\n"
		"punpckhwd %%mm2, %%mm2\n"
		"movq %%mm1, %%mm0\n"
		"punpckldq %%mm2, %%mm2\n"
		"punpckldq %%mm1, %%mm0\n"
		"pmullw 0(%3), %%mm0\n"
		"pxor %%mm3, %%mm3\n"
		"punpckhdq %%mm1, %%mm1\n"
		"punpcklbw %%mm3, %%mm5\n"
		"pmullw 8(%3), %%mm1\n"
		"movq %%mm5, %%mm4\n"
		"punpcklwd %%mm5, %%mm4\n"
		"punpckhwd %%mm5, %%mm5\n"
		"pmullw 16(%3), %%mm2\n"
		"movq %%mm4, %%mm3\n"
		"punpckldq %%mm5, %%mm5\n"
		"punpckldq %%mm4, %%mm3\n"
		"punpckhdq %%mm4, %%mm4\n"
		"pmullw 0(%3), %%mm3\n"
		"paddw %%mm1, %%mm0\n"
		"paddw 24(%3), %%mm2\n"
		"pmullw 8(%3), %%mm4\n"
		"paddw %%mm2, %%mm0\n"
		"pmullw 16(%3), %%mm5\n"
		"psrlw $8, %%mm0\n"
		"paddw %%mm4, %%mm3\n"
		"paddw 24(%3), %%mm5\n"
		"paddw %%mm5, %%mm3\n"
		"psrlw $8, %%mm3\n"
		"packuswb %%mm3, %%mm0\n"
		"movq %%mm0, (%2)\n"

		:
		: "r" (src0), "r" (src1), "r" (dst), "r" (bgra8888toyuy2_coeff)
		: "cc", "memory"
	);
}

static void video_line_bgra8888toyuy2_step_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count / 2;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		bgra8888toyuy2_mmx(dst8, src8, src8 + step1);

		dst8 += 8;
		src8 += step1 * 2;
		--count;
	}
}

#endif

static inline void bgra8888toyuy2_def(void* dst, const void* src)
{
	const uint8* src8 = (const uint8*)src;
	uint8* dst8 = (uint8*)dst;
	unsigned r, g, b;

#ifdef USE_LSB
	b = src8[0];
	g = src8[1];
	r = src8[2];
#else
	b = src8[3];
	g = src8[2];
	r = src8[1];
#endif

/*
	Y = 0.299R + 0.587G + 0.114B
	U = 0.492 (B - Y)
	V = 0.877 (R - Y)
*/
	unsigned y = ((19595*r + 38469*g + 7471*b) >> 16) & 0xFF;
	unsigned u = ((32243 * (int)(b-y) + 8388608) >> 16) & 0xFF;
	unsigned v = ((57475 * (int)(r-y) + 8388608) >> 16) & 0xFF;

	dst8[0] = y;
	dst8[1] = u;
	dst8[2] = y;
	dst8[3] = v;
}

static void video_line_bgra8888toyuy2_step_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

	while (count) {
		bgra8888toyuy2_def(dst8, src8);

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

#if defined(USE_ASM_i586)

static void video_line_bgra5551toyuy2_step_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count / 2;
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;
	int step1 = stage->sdp;

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

		bgra8888toyuy2_mmx(dst8, &p0, &p1);

		dst8 += 8;
		--count;
	}
}

#endif

static void video_line_bgra5551toyuy2_step_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
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

		bgra8888toyuy2_def(dst8, p4);

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

static void video_line_rgbtoyuy2_step_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		adv_pixel p;
		unsigned char p4[4];

		switch (stage->ssp) {
		default :
			assert(0);
		case 1 :
			p = cpu_uint8_read(src8);
			break;
		case 2 :
			p = cpu_uint16_read(src8);
			break;
		case 3 :
			p = cpu_uint24_read(src8);
			break;
		case 4 :
			p = cpu_uint32_read(src8);
			break;
		}

		cpu_uint32_write(p4, (rgb_shift(p, stage->red_shift) & stage->red_mask)
			| (rgb_shift(p, stage->green_shift) & stage->green_mask)
			| (rgb_shift(p, stage->blue_shift) & stage->blue_mask) );

		bgra8888toyuy2_def(dst8, &p4);

		PADD(src8, stage->sdp);
		dst8 += 4;
		--count;
	}
}

static void video_stage_rgbtoyuy2_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, adv_color_def sdef)
{
	adv_color_def ddef = color_def_make_from_rgb_sizelenpos(4, 8, 16, 8, 8, 8, 0);
	STAGE_SIZE(stage, pipe_rgbtoyuy2, sdx, sdp, color_def_bytes_per_pixel_get(sdef), sdx, 4);
	STAGE_PUT(stage, video_line_rgbtoyuy2_step_def, video_line_rgbtoyuy2_step_def);
	STAGE_CONVERSION(stage, sdef, ddef);
}

/****************************************************************************/
/* rgb to rgb */

static void video_line_rgbtorgb_step_def(const struct video_stage_horz_struct* stage, void* dst, void* src)
{
	unsigned count = stage->slice.count;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		adv_pixel p;

		switch (stage->ssp) {
		default :
			assert(0);
		case 1 :
			p = cpu_uint8_read(src8);
			break;
		case 2 :
			p = cpu_uint16_read(src8);
			break;
		case 3 :
			p = cpu_uint24_read(src8);
			break;
		case 4 :
			p = cpu_uint32_read(src8);
			break;
		}

		p = (rgb_shift(p, stage->red_shift) & stage->red_mask)
			| (rgb_shift(p, stage->green_shift) & stage->green_mask)
			| (rgb_shift(p, stage->blue_shift) & stage->blue_mask);

		switch (stage->dsp) {
		default :
			assert(0);
		case 1 :
			cpu_uint8_write(dst8, p);
			break;
		case 2 :
			cpu_uint16_write(dst8, p);
			break;
		case 3 :
			cpu_uint24_write(dst8, p);
			break;
		case 4 :
			cpu_uint32_write(dst8, p);
			break;
		}

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
