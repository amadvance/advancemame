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

#ifndef __VPALETTE_H
#define __VPALETTE_H

#include "blit.h"

/****************************************************************************/
/* palette8to8 */

static void video_line_palette8to8_step1(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count;
	unsigned* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (inner_count) {
		*dst8++ = palette[src8[0]];
		src8 += 1;
		--inner_count;
	}
}

static void video_line_palette8to8(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count;
	int line_index1 = stage->sdp;
	unsigned* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (inner_count) {
		*dst8++ = palette[src8[0]];
		src8 += line_index1;
		--inner_count;
	}
}

static void video_stage_palette8to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, int unsigned* palette) {
	STAGE_PALETTE(stage,pipe_palette8to8,sdx,sdp,1,1,palette,video_line_palette8to8_step1,video_line_palette8to8);
}

/****************************************************************************/
/* palette8to16 */

#if defined(USE_ASM_i586)
static void video_line_palette8to16_step1_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzbl (%0),%%eax\n"
		"movzbl 1(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzbl 2(%0),%%eax\n"
		"movzbl 3(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm2\n"
		"movd (%3,%%edx,4),%%mm3\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $4,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (stage->palette)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette8to16_step1_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2;
	unsigned* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src8[0]] | palette[src8[1]] << 16;
		src8 += 2;
		--inner_count;
	}
}

static void video_line_palette8to16(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2;
	int line_index1 = stage->sdp;
	int line_index2 = line_index1 + line_index1;
	unsigned* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src8[0]] | palette[src8[line_index1]] << 16;
		src8 += line_index2;
		--inner_count;
	}
}

static void video_stage_palette8to16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, int unsigned* palette) {
	STAGE_PALETTE(stage,pipe_palette8to16,sdx,sdp,1,2,palette,BLITTER(video_line_palette8to16_step1),video_line_palette8to16);
}

/****************************************************************************/
/* palette8to32 */

static void video_line_palette8to32_step4(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count;
	unsigned* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src8[0]];
		src8 += 1;
		--inner_count;
	}
}

static void video_line_palette8to32(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count;
	int line_index1 = stage->sdp;
	unsigned* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src8[0]];
		src8 += line_index1;
		--inner_count;
	}
}

static void video_stage_palette8to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, int unsigned* palette) {
	STAGE_PALETTE(stage,pipe_palette8to32,sdx,sdp,1,4,palette,video_line_palette8to32_step4,video_line_palette8to32);
}

/****************************************************************************/
/* palette16to8 */

#if defined(USE_ASM_i586)
static void video_line_palette16to8_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl 2(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"punpcklbw %%mm1, %%mm0\n"
		"movzwl 4(%0),%%eax\n"
		"movzwl 6(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm2\n"
		"movd (%3,%%edx,4),%%mm3\n"
		"punpcklbw %%mm3, %%mm2\n"
		"punpcklwd %%mm2, %%mm0\n"

		"movzwl 8(%0),%%eax\n"
		"movzwl 10(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm4\n"
		"movd (%3,%%edx,4),%%mm5\n"
		"punpcklbw %%mm5, %%mm4\n"
		"movzwl 12(%0),%%eax\n"
		"movzwl 14(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm6\n"
		"movd (%3,%%edx,4),%%mm7\n"
		"punpcklbw %%mm7, %%mm6\n"
		"punpcklwd %%mm6, %%mm4\n"

		"punpckldq %%mm4, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $16,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (stage->palette)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette16to8_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 4;
	unsigned* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src16[0]]
			| palette[src16[1]] << 8
			| palette[src16[2]] << 16
			| palette[src16[3]] << 24;
		src16 += 4;
		--inner_count;
	}
}

#if defined(USE_ASM_i586)
static void video_line_palette16to8_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"addl %4,%0\n"
		"punpcklbw %%mm1, %%mm0\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm2\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm3\n"
		"addl %4,%0\n"
		"punpcklbw %%mm3, %%mm2\n"
		"punpcklwd %%mm2, %%mm0\n"

		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm4\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm5\n"
		"addl %4,%0\n"
		"punpcklbw %%mm5, %%mm4\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm6\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm7\n"
		"addl %4,%0\n"
		"punpcklbw %%mm7, %%mm6\n"
		"punpcklwd %%mm6, %%mm4\n"

		"punpckldq %%mm4, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+m" (count)
		: "b" (stage->palette), "c" (stage->sdp)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette16to8_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 4;
	int line_index1 = stage->sdp;
	int line_index2 = line_index1 + line_index1;
	int line_index3 = line_index2 + line_index1;
	int line_index4 = line_index3 + line_index1;
	unsigned* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[P16DER0(src)]
			| palette[P16DER(src,line_index1)] << 8
			| palette[P16DER(src,line_index2)] << 16
			| palette[P16DER(src,line_index3)] << 24;
		PADD(src,line_index4);
		--inner_count;
	}
}

static void video_stage_palette16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, unsigned* palette) {
	STAGE_PALETTE(stage,pipe_palette16to8,sdx,sdp,2,1,palette,BLITTER(video_line_palette16to8_step2),BLITTER(video_line_palette16to8));
}

/****************************************************************************/
/* palette16to16 */

#if defined(USE_ASM_i586)
static void video_line_palette16to16_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl 2(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzwl 4(%0),%%eax\n"
		"movzwl 6(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm2\n"
		"movd (%3,%%edx,4),%%mm3\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (stage->palette)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette16to16_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2;
	unsigned* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src16[0]] | palette[src16[1]] << 16;
		src16 += 2;
		--inner_count;
	}
}

#if defined(USE_ASM_i586)
static void video_line_palette16to16_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"addl %4,%0\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm2\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm3\n"
		"addl %4,%0\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+m" (count)
		: "b" (stage->palette), "c" (stage->sdp)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette16to16_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count / 2;
	int line_index1 = stage->sdp;
	int line_index2 = line_index1 + line_index1;
	unsigned* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[P16DER0(src)] | palette[P16DER(src,line_index1)] << 16;
		PADD(src,line_index2);
		--inner_count;
	}
}

static void video_stage_palette16to16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, unsigned* palette) {
	STAGE_PALETTE(stage,pipe_palette16to16,sdx,sdp,2,2,palette,BLITTER(video_line_palette16to16_step2),BLITTER(video_line_palette16to16));
}

/****************************************************************************/
/* palette16to32 */

#if defined(USE_ASM_i586)
static void video_line_palette16to32_step2_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl 2(%0),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"punpckldq %%mm1, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $4,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (stage->palette)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette16to32_step2_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count;
	unsigned* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[src16[0]];
		src16 += 1;
		--inner_count;
	}
}

#if defined(USE_ASM_i586)
static void video_line_palette16to32_mmx(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned count = stage->slice.count;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%4),%%edx\n"
		"movd (%3,%%eax,4),%%mm0\n"
		"addl %4,%0\n"
		"movd (%3,%%edx,4),%%mm1\n"
		"addl %4,%0\n"
		"punpckldq %%mm1, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+m" (count)
		: "b" (stage->palette), "c" (stage->sdp)
		: "cc", "%eax", "%edx"
	);
}
#endif

static void video_line_palette16to32_def(const struct video_stage_horz_struct* stage, void* dst, void* src) {
	unsigned inner_count = stage->slice.count;
	int line_index1 = stage->sdp;
	unsigned* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	while (inner_count) {
		*dst32++ = palette[P16DER0(src)];
		PADD(src,line_index1);
		--inner_count;
	}
}

static void video_stage_palette16to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, unsigned* palette) {
	STAGE_PALETTE(stage,pipe_palette16to32,sdx,sdp,2,4,palette,BLITTER(video_line_palette16to32_step2),BLITTER(video_line_palette16to32));
}

#endif
