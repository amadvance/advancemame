/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2008 Andrea Mazzoleni
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

#ifndef __IMEAN_H
#define __IMEAN_H

#include "icommon.h"

/***************************************************************************/
/* internal_mean */

enum MEAN_MASK {
	MEAN_MASK_H_0,
	MEAN_MASK_H_1,
	MEAN_MASK_MAX
};

static uint32 mean_mask[MEAN_MASK_MAX];

static uint32 expand_nibble(unsigned bytes_per_pixel, unsigned v)
{
	switch (bytes_per_pixel) {
	case 1 :
		return v | v << 8 | v << 16 | v << 24;
	case 2 :
		return v | v << 16;
	case 4 :
		return v;
	default:
		return 0;
	}
}

static void internal_mean_set(const struct video_pipeline_target_struct* target)
{
	adv_pixel rgb_h = rgb_highmask_make_from_def(target->color_def);
	adv_pixel rgb_m = rgb_wholemask_make_from_def(target->color_def);

	unsigned bytes_per_pixel = target->bytes_per_pixel;

	mean_mask[MEAN_MASK_H_0] = expand_nibble(bytes_per_pixel, (~rgb_h) & rgb_m);
	mean_mask[MEAN_MASK_H_1] = mean_mask[MEAN_MASK_H_0];
}

/**
 * Compute the mean of two series of rgb pixels.
 * This function compute (a + b) / 2 for any rgb nibble, using the
 * the formula (a + b) / 2 = ((a ^ b) >> 1) + (a & b).
 * To extend this formula to a serie of packed nibbles the formula is
 * implemented as (((v0 ^ v1) >> 1) & MASK) + (v0 & v1) where MASK
 * is used to clear the high bit of all the packed nibbles.
 */
static inline uint32 internal_mean_value(uint32 v0, uint32 v1)
{
	return (((v0 ^ v1) >> 1) & mean_mask[MEAN_MASK_H_0]) + (v0 & v1);
}

/***************************************************************************/
/* Compute the mean of dst and src and store the result in dst */

#if defined(USE_ASM_INLINE)
static inline void internal_mean64_vert_self_mmx(void* dst, const void* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"movq (%3), %%mm4\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq (%1), %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"

		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mean_mask)
		: "cc"
	);
}

static inline void internal_mean8_vert_self_mmx(uint8* dst, const uint8* src, unsigned count)
{
	internal_mean64_vert_self_mmx(dst, src, count / 8);
}

static inline void internal_mean16_vert_self_mmx(uint16* dst, const uint16* src, unsigned count)
{
	internal_mean64_vert_self_mmx(dst, src, count / 4);
}

static inline void internal_mean32_vert_self_mmx(uint32* dst, const uint32* src, unsigned count)
{
	internal_mean64_vert_self_mmx(dst, src, count / 2);
}
#endif

static inline void internal_mean32_vert_self_def(uint32* dst32, const uint32* src32, unsigned count)
{
	while (count) {
		dst32[0] = internal_mean_value(dst32[0], src32[0]);
		++src32;
		++dst32;
		--count;
	}
}

static inline void internal_mean8_vert_self_def(uint8* dst, const uint8* src, unsigned count)
{
	internal_mean32_vert_self_def((uint32*)dst, (uint32*)src, count / 4);
}

static inline void internal_mean16_vert_self_def(uint16* dst, const uint16* src, unsigned count)
{
	internal_mean32_vert_self_def((uint32*)dst, (uint32*)src, count / 2);
}

static inline void internal_mean8_vert_self_step(uint8* dst8, const uint8* src8, unsigned count, int step1)
{
	while (count) {
		dst8[0] = internal_mean_value(dst8[0], src8[0]);
		src8 += step1;
		++dst8;
		--count;
	}
}

static inline void internal_mean16_vert_self_step(uint16* dst16, const uint16* src16, unsigned count, int step1)
{
	while (count) {
		dst16[0] = internal_mean_value(dst16[0], src16[0]);
		PADD(src16, step1);
		++dst16;
		--count;
	}
}

static inline void internal_mean32_vert_self_step(uint32* dst32, const uint32* src32, unsigned count, int step1)
{
	while (count) {
		dst32[0] = internal_mean_value(dst32[0], src32[0]);
		PADD(src32, step1);
		++dst32;
		--count;
	}
}

/***************************************************************************/
/* Compute the mean of src and src+1 and store the result in dst */

#if defined(USE_ASM_INLINE)
static uint32 mean8_horz_step1_mask[2] = { 0x00000000, 0xFF000000 };

static inline void internal_mean8_horz_next_step1_mmx(uint8* dst, const uint8* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	count /= 8;
	if (!count)
		return;

	__asm__ __volatile__(
		"decl %2\n"
		"jz 1f\n"
		"movq (%3), %%mm4\n"
		"movq (%0), %%mm7\n" /* previous value */

		ASM_JUMP_ALIGN
		"0:\n"
		"movq %%mm7, %%mm0\n" /* current value */
		"movq 8(%0), %%mm1\n"
		"movq %%mm0, %%mm6\n"
		"movq %%mm1, %%mm7\n" /* next value */
		"psrlq $8, %%mm6\n"
		"psllq $56, %%mm1\n"
		"por %%mm6, %%mm1\n" /* masked value */

		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"

		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"

		"1:"

		"movq %%mm7, %%mm0\n" /* current value */
		"movq %%mm7, %%mm1\n"
		"psrlq $8, %%mm7\n"
		"pand (%4), %%mm1\n"
		"por %%mm7, %%mm1\n" /* masked value */

		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mean_mask), "r" (mean8_horz_step1_mask)
		: "cc"
	);
}

static uint32 mean16_horz_step2_mask[2] = { 0x00000000, 0xFFFF0000 };

static inline void internal_mean16_horz_next_step2_mmx(uint16* dst, const uint16* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	count /= 4;
	if (!count)
		return;

	__asm__ __volatile__(
		"decl %2\n"
		"jz 1f\n"
		"movq (%3), %%mm4\n"
		"movq (%0), %%mm7\n" /* previous value */

		ASM_JUMP_ALIGN
		"0:\n"
		"movq %%mm7, %%mm0\n" /* current value */
		"movq 8(%0), %%mm1\n"
		"movq %%mm0, %%mm6\n"
		"movq %%mm1, %%mm7\n" /* next value */
		"psrlq $16, %%mm6\n"
		"psllq $48, %%mm1\n"
		"por %%mm6, %%mm1\n" /* masked value */

		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"

		"1:"

		"movq %%mm7, %%mm0\n" /* current value */
		"movq %%mm7, %%mm1\n"
		"psrlq $16, %%mm7\n"
		"pand (%4), %%mm1\n"
		"por %%mm7, %%mm1\n" /* masked value */

		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mean_mask), "r" (mean16_horz_step2_mask)
		: "cc"
	);
}

static uint32 mean32_horz_step4_mask[2] = { 0x00000000, 0xFFFFFFFF };

static inline void internal_mean32_horz_next_step4_mmx(uint32* dst, const uint32* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	count /= 2;
	if (!count)
		return;

	__asm__ __volatile__(
		"decl %2\n"
		"jz 1f\n"
		"movq (%3), %%mm4\n"
		"movq (%0), %%mm7\n" /* previous value */

		ASM_JUMP_ALIGN
		"0:\n"
		"movq %%mm7, %%mm0\n" /* current value */
		"movq 8(%0), %%mm1\n"
		"movq %%mm0, %%mm6\n"
		"movq %%mm1, %%mm7\n" /* next value */
		"psrlq $32, %%mm6\n"
		"psllq $32, %%mm1\n"
		"por %%mm6, %%mm1\n" /* masked value */

		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"

		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"

		"1:"

		"movq %%mm7, %%mm0\n" /* current value */
		"movq %%mm7, %%mm1\n"
		"psrlq $32, %%mm7\n"
		"pand (%4), %%mm1\n"
		"por %%mm7, %%mm1\n" /* masked value */

		"movq %%mm0, %%mm2\n"
		"movq %%mm1, %%mm3\n"
		"pxor %%mm1, %%mm0\n"
		"pand %%mm3, %%mm2\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm4, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mean_mask), "r" (mean32_horz_step4_mask)
		: "cc"
	);
}
#endif

static inline void internal_mean8_horz_next_step1_def(uint8* dst8, const uint8* src8, unsigned count)
{
	count /= 4;
	if (count) {
		const uint32* src32 = (const uint32*)src8;
		uint32* dst32 = (uint32*)dst8;
		--count;
		while (count) {
#ifdef USE_LSB
			*dst32 = internal_mean_value(src32[0], (src32[0] >> 8) | (src32[1] << 24));
#else
			*dst32 = internal_mean_value(src32[0], (src32[0] << 8) | (src32[1] >> 24));
#endif
			++dst32;
			++src32;
			--count;
		}
#ifdef USE_LSB
		*dst32 = internal_mean_value(src32[0], (src32[0] >> 8) | (src32[0] & 0xFF000000));
#else
		*dst32 = internal_mean_value(src32[0], (src32[0] << 8) | (src32[0] & 0x000000FF));
#endif
	}
}

static inline void internal_mean16_horz_next_step2_def(uint16* dst16, const uint16* src16, unsigned count)
{
	count /= 2;
	if (count) {
		const uint32* src32 = (uint32*)src16;
		uint32* dst32 = (uint32*)dst16;
		--count;
		while (count) {
#ifdef USE_LSB
			*dst32 = internal_mean_value(src32[0], (src32[0] >> 16) | (src32[1] << 16));
#else
			*dst32 = internal_mean_value(src32[0], (src32[0] << 16) | (src32[1] >> 16));
#endif
			++dst32;
			++src32;
			--count;
		}
#ifdef USE_LSB
		*dst32 = internal_mean_value(src32[0], (src32[0] >> 16) | (src32[0] & 0xFFFF0000));
#else
		*dst32 = internal_mean_value(src32[0], (src32[0] << 16) | (src32[0] & 0x0000FFFF));
#endif
	}
}

static inline void internal_mean32_horz_next_step4_def(uint32* dst32, const uint32* src32, unsigned count)
{
	if (count) {
		--count;
		while (count) {
			*dst32 = internal_mean_value(src32[0], src32[1]);
			++dst32;
			++src32;
			--count;
		}
		*dst32 = *src32;
	}
}

static inline void internal_mean8_horz_next_step(uint8* dst8, const uint8* src8, unsigned count, int step)
{
	if (count) {
		--count;
		while (count) {
			*dst8 = internal_mean_value(P8DER0(src8), P8DER(src8, step));
			dst8 += 1;
			PADD(src8, step);
			--count;
		}
		*dst8 = *src8;
	}
}

static inline void internal_mean16_horz_next_step(uint16* dst16, const uint16* src16, unsigned count, int step)
{
	if (count) {
		--count;
		while (count) {
			*dst16 = internal_mean_value(P16DER0(src16), P16DER(src16, step));
			++dst16;
			PADD(src16, step);
			--count;
		}
		*dst16 = *src16;
	}
}

static inline void internal_mean32_horz_next_step(uint32* dst32, const uint32* src32, unsigned count, int step)
{
	if (count) {
		--count;
		while (count) {
			*dst32 = internal_mean_value(P32DER0(src32), P32DER(src32, step));
			++dst32;
			PADD(src32, step);
			--count;
		}
		*dst32 = *src32;
	}
}

#endif

