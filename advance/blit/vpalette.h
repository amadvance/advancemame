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

#ifndef __VPALETTE_H
#define __VPALETTE_H

#include "blit.h"

/****************************************************************************/
/* palette8to8 */

static void video_line_palette8to8_step1(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint8* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = palette[src8[0]];
		src8 += 1;
		--count;
	}
}

static void video_line_palette8to8(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	const uint8* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = palette[src8[0]];
		src8 += step1;
		--count;
	}
}

static void video_stage_palette8to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint8* palette)
{
	STAGE_SIZE(stage, pipe_palette8to8, sdx, sdp, 1, sdx, 1);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, video_line_palette8to8_step1, video_line_palette8to8);
}

/****************************************************************************/
/* palette8to16 */

#if defined(USE_ASM_INLINE)
static void video_line_palette8to16_step1_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	const uint16* palette = stage->palette;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzbl (%0), %%eax\n"
		"movzbl 1(%0), %%edx\n"
		"movzwl (%3, %%eax, 2), %%eax\n"
		"movzwl (%3, %%edx, 2), %%edx\n"
		"movd %%eax, %%mm0\n"
		"movd %%edx, %%mm1\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzbl 2(%0), %%eax\n"
		"movzbl 3(%0), %%edx\n"
		"movzwl (%3, %%eax, 2), %%eax\n"
		"movzwl (%3, %%edx, 2), %%edx\n"
		"movd %%eax, %%mm2\n"
		"movd %%edx, %%mm3\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (palette)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P16DER0(dst) = palette[P8DER0(src)];
		PADD(dst, 2);
		PADD(src, 1);
		--rest;
	}
}
#endif

static void video_line_palette8to16_step1_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint16* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[src8[0]], palette[src8[1]]);
		src8 += 2;
		--count;
	}
}

static void video_line_palette8to16(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	int step2 = step1 + step1;
	const uint16* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[src8[0]], palette[src8[step1]]);
		src8 += step2;
		--count;
	}
}

static void video_stage_palette8to16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint16* palette)
{
	STAGE_SIZE(stage, pipe_palette8to16, sdx, sdp, 1, sdx, 2);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette8to16_step1), video_line_palette8to16);
}

/****************************************************************************/
/* palette8to32 */

static void video_line_palette8to32_step4(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint32* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[src8[0]];
		src8 += 1;
		--count;
	}
}

static void video_line_palette8to32(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	const uint32* palette = stage->palette;
	uint8* src8 = (uint8*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[src8[0]];
		src8 += step1;
		--count;
	}
}

static void video_stage_palette8to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint32* palette)
{
	STAGE_SIZE(stage, pipe_palette8to32, sdx, sdp, 1, sdx, 4);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, video_line_palette8to32_step4, video_line_palette8to32);
}

/****************************************************************************/
/* palette16to8 */

#if defined(USE_ASM_INLINE)
static void video_line_palette16to8_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;
	const uint8* palette = stage->palette;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl 2(%0), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm0\n"
		"movd %%edx, %%mm1\n"
		"punpcklbw %%mm1, %%mm0\n"
		"movzwl 4(%0), %%eax\n"
		"movzwl 6(%0), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm2\n"
		"movd %%edx, %%mm3\n"
		"punpcklbw %%mm3, %%mm2\n"
		"punpcklwd %%mm2, %%mm0\n"

		"movzwl 8(%0), %%eax\n"
		"movzwl 10(%0), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm4\n"
		"movd %%edx, %%mm5\n"
		"punpcklbw %%mm5, %%mm4\n"
		"movzwl 12(%0), %%eax\n"
		"movzwl 14(%0), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm6\n"
		"movd %%edx, %%mm7\n"
		"punpcklbw %%mm7, %%mm6\n"
		"punpcklwd %%mm6, %%mm4\n"

		"punpckldq %%mm4, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (palette)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P8DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 1);
		PADD(src, 2);
		--rest;
	}
}
#endif

static void video_line_palette16to8_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint8* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 4;

	while (count) {
		*dst32++ = cpu_uint32_make_uint8(palette[src16[0]], palette[src16[1]], palette[src16[2]], palette[src16[3]]);
		src16 += 4;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static void video_line_palette16to8_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;
	const uint8* palette = stage->palette;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm0\n"
		"movd %%edx, %%mm1\n"
		"addl %4, %0\n"
		"addl %4, %0\n"
		"punpcklbw %%mm1, %%mm0\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm2\n"
		"movd %%edx, %%mm3\n"
		"addl %4, %0\n"
		"addl %4, %0\n"
		"punpcklbw %%mm3, %%mm2\n"
		"punpcklwd %%mm2, %%mm0\n"

		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm4\n"
		"movd %%edx, %%mm5\n"
		"addl %4, %0\n"
		"addl %4, %0\n"
		"punpcklbw %%mm5, %%mm4\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movzbl (%3, %%eax, 1), %%eax\n"
		"movzbl (%3, %%edx, 1), %%edx\n"
		"movd %%eax, %%mm6\n"
		"movd %%edx, %%mm7\n"
		"addl %4, %0\n"
		"addl %4, %0\n"
		"punpcklbw %%mm7, %%mm6\n"
		"punpcklwd %%mm6, %%mm4\n"

		"punpckldq %%mm4, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+m" (count)
		: "b" (palette), "c" (stage->sdp)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P8DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 1);
		PADD(src, 2);
		--rest;
	}
}
#endif

static void video_line_palette16to8_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	int step2 = step1 + step1;
	int step3 = step2 + step1;
	int step4 = step3 + step1;
	const uint8* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	count /= 4;

	while (count) {
		*dst32++ = cpu_uint32_make_uint8(palette[P16DER0(src)], palette[P16DER(src, step1)], palette[P16DER(src, step2)], palette[P16DER(src, step3)]);
		PADD(src, step4);
		--count;
	}
}

static void video_stage_palette16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint8* palette)
{
	STAGE_SIZE(stage, pipe_palette16to8, sdx, sdp, 2, sdx, 1);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette16to8_step2), BLITTER(video_line_palette16to8));
}

/****************************************************************************/
/* palette16to16 */

#if defined(USE_ASM_INLINE)
static void video_line_palette16to16_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	const uint16* palette = stage->palette;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl 2(%0), %%edx\n"
		"movzwl (%3, %%eax, 2), %%eax\n"
		"movzwl (%3, %%edx, 2), %%edx\n"
		"movd %%eax, %%mm0\n"
		"movd %%edx, %%mm1\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzwl 4(%0), %%eax\n"
		"movzwl 6(%0), %%edx\n"
		"movzwl (%3, %%eax, 2), %%eax\n"
		"movzwl (%3, %%edx, 2), %%edx\n"
		"movd %%eax, %%mm2\n"
		"movd %%edx, %%mm3\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (palette)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P16DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 2);
		PADD(src, 2);
		--rest;
	}
}
#endif

static void video_line_palette16to16_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint16* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[src16[0]], palette[src16[1]]);
		src16 += 2;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static void video_line_palette16to16_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;
	const uint16* palette = stage->palette;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movzwl (%3, %%eax, 2), %%eax\n"
		"movzwl (%3, %%edx, 2), %%edx\n"
		"movd %%eax, %%mm0\n"
		"movd %%edx, %%mm1\n"
		"addl %4, %0\n"
		"addl %4, %0\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movzwl (%3, %%eax, 2), %%eax\n"
		"movzwl (%3, %%edx, 2), %%edx\n"
		"movd %%eax, %%mm2\n"
		"movd %%edx, %%mm3\n"
		"addl %4, %0\n"
		"addl %4, %0\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+m" (count)
		: "b" (palette), "c" (stage->sdp)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P16DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 2);
		PADD(src, 2);
		--rest;
	}
}
#endif

static void video_line_palette16to16_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	int step2 = step1 + step1;
	const uint16* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	count /= 2;

	while (count) {
		*dst32++ = cpu_uint32_make_uint16(palette[P16DER0(src)], palette[P16DER(src, step1)]);
		PADD(src, step2);
		--count;
	}
}

static void video_stage_palette16to16_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint16* palette)
{
	STAGE_SIZE(stage, pipe_palette16to16, sdx, sdp, 2, sdx, 2);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette16to16_step2), BLITTER(video_line_palette16to16));
}

/****************************************************************************/
/* palette16to32 */

#if defined(USE_ASM_INLINE)
static void video_line_palette16to32_step2_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	const uint32* palette = stage->palette;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl 2(%0), %%edx\n"
		"movd (%3, %%eax, 4), %%mm0\n"
		"movd (%3, %%edx, 4), %%mm1\n"
		"punpckldq %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (palette)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P32DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 4);
		PADD(src, 2);
		--rest;
	}
}
#endif

static void video_line_palette16to32_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	const uint32* palette = stage->palette;
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[src16[0]];
		src16 += 1;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static void video_line_palette16to32_mmx(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;
	const uint32* palette = stage->palette;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %4), %%edx\n"
		"movd (%3, %%eax, 4), %%mm0\n"
		"addl %4, %0\n"
		"movd (%3, %%edx, 4), %%mm1\n"
		"addl %4, %0\n"
		"punpckldq %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+m" (count)
		: "b" (palette), "c" (stage->sdp)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		P32DER0(dst) = palette[P16DER0(src)];
		PADD(dst, 4);
		PADD(src, 2);
		--rest;
	}
}
#endif

static void video_line_palette16to32_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	int step1 = stage->sdp;
	const uint32* palette = stage->palette;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = palette[P16DER0(src)];
		PADD(src, step1);
		--count;
	}
}

static void video_stage_palette16to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp, const uint32* palette)
{
	STAGE_SIZE(stage, pipe_palette16to32, sdx, sdp, 2, sdx, 4);
	STAGE_PALETTE(stage, palette);
	STAGE_PUT(stage, BLITTER(video_line_palette16to32_step2), BLITTER(video_line_palette16to32));
}

/****************************************************************************/
/* imm16to8 */

static void video_line_imm16to8_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = src16[0] & 0xFF;
		src16 += 1;
		--count;
	}
}

static void video_line_imm16to8_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint8* dst8 = (uint8*)dst;

	while (count) {
		*dst8++ = src16[0] & 0xFF;
		PADD(src16, stage->sdp);
		--count;
	}
}

static void video_stage_imm16to8_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_imm16to8, sdx, sdp, 2, sdx, 1);
	STAGE_PUT(stage, video_line_imm16to8_step2_def, video_line_imm16to8_def);
}

/****************************************************************************/
/* imm16to32 */

static void video_line_imm16to32_step2_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = src16[0];
		src16 += 1;
		--count;
	}
}

static void video_line_imm16to32_def(const struct video_stage_horz_struct* stage, unsigned line, void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = src16[0];
		PADD(src16, stage->sdp);
		--count;
	}
}

static void video_stage_imm16to32_set(struct video_stage_horz_struct* stage, unsigned sdx, int sdp)
{
	STAGE_SIZE(stage, pipe_imm16to32, sdx, sdp, 2, sdx, 4);
	STAGE_PUT(stage, video_line_imm16to32_step2_def, video_line_imm16to32_def);
}

#endif

