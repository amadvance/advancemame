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

#ifndef __ISCALE_H
#define __ISCALE_H

#include "icommon.h"

/***************************************************************************/
/* internal_scale2x */

/* Scale by a factor of 2
 * in:
 *   src0 previous row, normal length
 *   src1 current row, normal length
 *   src2 next row, normal length
 *   count length in pixel
 * out:
 *   dst0 first destination row, double length
 *   dst1 second destination row, double length
 */

static void internal_scale2x_8_def(uint8* dst0, uint8* dst1, const uint8* src0, const uint8* src1, const uint8* src2, unsigned count) {
	/* first pixel */
	dst0[0] = src1[0];
	dst1[0] = src1[0];
	if (src1[1] == src0[0] && src2[0] != src0[0])
		dst0[1] = src0[0];
	else
		dst0[1] = src1[0];
	if (src1[1] == src2[0] && src0[0] != src2[0])
		dst1[1] = src2[0];
	else
		dst1[1] = src1[0];
	++src0;
	++src1;
	++src2;
	dst0 += 2;
	dst1 += 2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[1] != src0[0])
			dst0[0] = src0[0];
		else
			dst0[0] = src1[0];
		if (src1[1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
			dst0[1] = src0[0];
		else
			dst0[1] = src1[0];

		if (src1[-1] == src2[0] && src0[0] != src2[0] && src1[1] != src2[0])
			dst1[0] = src2[0];
		else
			dst1[0] = src1[0];
		if (src1[1] == src2[0] && src0[0] != src2[0] && src1[-1] != src2[0])
			dst1[1] = src2[0];
		else
			dst1[1] = src1[0];

		++src0;
		++src1;
		++src2;
		dst0 += 2;
		dst1 += 2;
		--count;
	}

	/* last pixel */
	if (src1[-1] == src0[0] && src2[0] != src0[0])
		dst0[0] = src0[0];
	else
		dst0[0] = src1[0];
	if (src1[-1] == src2[0] && src0[0] != src2[0])
		dst1[0] = src2[0];
	else
		dst1[0] = src1[0];
	dst0[1] = src1[0];
	dst1[1] = src1[0];
}

static void internal_scale2x_16_def(uint16* dst0, uint16* dst1, const uint16* src0, const uint16* src1, const uint16* src2, unsigned count) {
	/* first pixel */
	dst0[0] = src1[0];
	dst1[0] = src1[0];
	if (src1[1] == src0[0] && src2[0] != src0[0])
		dst0[1] =src0[0];
	else
		dst0[1] =src1[0];
	if (src1[1] == src2[0] && src0[0] != src2[0])
		dst1[1] =src2[0];
	else
		dst1[1] =src1[0];
	++src0;
	++src1;
	++src2;
	dst0 += 2;
	dst1 += 2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[1] != src0[0])
			dst0[0] = src0[0];
		else
			dst0[0] = src1[0];
		if (src1[1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
			dst0[1] =src0[0];
		else
			dst0[1] =src1[0];

		if (src1[-1] == src2[0] && src0[0] != src2[0] && src1[1] != src2[0])
			dst1[0] =src2[0];
		else
			dst1[0] =src1[0];
		if (src1[1] == src2[0] && src0[0] != src2[0] && src1[-1] != src2[0])
			dst1[1] =src2[0];
		else
			dst1[1] =src1[0];

		++src0;
		++src1;
		++src2;
		dst0 += 2;
		dst1 += 2;
		--count;
	}

	/* last pixel */
	if (src1[-1] == src0[0] && src2[0] != src0[0])
		dst0[0] =src0[0];
	else
		dst0[0] =src1[0];
	if (src1[-1] == src2[0] && src0[0] != src2[0])
		dst1[0] =src2[0];
	else
		dst1[0] =src1[0];
	dst0[1] =src1[0];
	dst1[1] =src1[0];
}

static void internal_scale2x_32_def(uint32* dst0, uint32* dst1, const uint32* src0, const uint32* src1, const uint32* src2, unsigned count) {
	/* first pixel */
	dst0[0] = src1[0];
	dst1[0] = src1[0];
	if (src1[1] == src0[0] && src2[0] != src0[0])
		dst0[1] = src0[0];
	else
		dst0[1] = src1[0];
	if (src1[1] == src2[0] && src0[0] != src2[0])
		dst1[1] = src2[0];
	else
		dst1[1] = src1[0];
	++src0;
	++src1;
	++src2;
	dst0 += 2;
	dst1 += 2;

	/* central pixels */
	count -= 2;
	while (count) {
		if (src1[-1] == src0[0] && src2[0] != src0[0] && src1[1] != src0[0])
			dst0[0] = src0[0];
		else
			dst0[0] = src1[0];
		if (src1[1] == src0[0] && src2[0] != src0[0] && src1[-1] != src0[0])
			dst0[1] = src0[0];
		else
			dst0[1] = src1[0];

		if (src1[-1] == src2[0] && src0[0] != src2[0] && src1[1] != src2[0])
			dst1[0] = src2[0];
		else
			dst1[0] = src1[0];
		if (src1[1] == src2[0] && src0[0] != src2[0] && src1[-1] != src2[0])
			dst1[1] = src2[0];
		else
			dst1[1] = src1[0];

		++src0;
		++src1;
		++src2;
		dst0 += 2;
		dst1 += 2;
		--count;
	}

	/* last pixel */
	if (src1[-1] == src0[0] && src2[0] != src0[0])
		dst0[0] = src0[0];
	else
		dst0[0] = src1[0];
	if (src1[-1] == src2[0] && src0[0] != src2[0])
		dst1[0] = src2[0];
	else
		dst1[0] = src1[0];
	dst0[1] = src1[0];
	dst1[1] = src1[0];
}

#if defined(USE_ASM_i586)

/*
The pixel map is :

	ABC (src0)
	DEF (src1)
	GHI (src2)

This function compute 4 new pixels in substitution of the source pixel E
like this map :

	ab (dst0)
	cd (dst1)

These are the variable used in the mmx implementation :

	&current point at E
	&current_left point at D
	&current_right point at F
	&current_upper point at B
	&current_lower point at H

	%0 point at B (src0)
	%1 point at E (src1)
	%2 point at H (src2)
	%3 point at dst
	%4 is the counter

	%mm0 current_left
	%mm1 current_next
	%mm2 tmp0
	%mm3 tmp1
	%mm4 tmp2
	%mm5 tmp3
	%mm6 current_upper
	%mm7 current
*/

static __inline__ void internal_scale2x_8_mmx_single(uint8* dst, const uint8* src0, const uint8* src1, const uint8* src2, unsigned count) {
	/* always do the first and last run */
	count -= 2*8;

	__asm__ __volatile__(
/* first run */
		/* set the current, current_pre, current_next registers */
		"pxor %%mm0,%%mm0\n" /* use a fake black out of screen */
		"movq 0(%1),%%mm7\n"
		"movq 8(%1),%%mm1\n"
		"psrlq $56,%%mm0\n"
		"psllq $56,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $8,%%mm2\n"
		"psrlq $8,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqb %%mm6,%%mm2\n"
		"pcmpeqb %%mm6,%%mm4\n"
		"pcmpeqb (%2),%%mm3\n"
		"pcmpeqb (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqb %%mm1,%%mm2\n"
		"pcmpeqb %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpcklbw %%mm4,%%mm2\n"
		"punpckhbw %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		/* next */
		"addl $8,%0\n"
		"addl $8,%1\n"
		"addl $8,%2\n"
		"addl $16,%3\n"

/* central runs */
		"shrl $3,%4\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"

		/* set the current, current_pre, current_next registers */
		"movq -8(%1),%%mm0\n"
		"movq (%1),%%mm7\n"
		"movq 8(%1),%%mm1\n"
		"psrlq $56,%%mm0\n"
		"psllq $56,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $8,%%mm2\n"
		"psrlq $8,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqb %%mm6,%%mm2\n"
		"pcmpeqb %%mm6,%%mm4\n"
		"pcmpeqb (%2),%%mm3\n"
		"pcmpeqb (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqb %%mm1,%%mm2\n"
		"pcmpeqb %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpcklbw %%mm4,%%mm2\n"
		"punpckhbw %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		/* next */
		"addl $8,%0\n"
		"addl $8,%1\n"
		"addl $8,%2\n"
		"addl $16,%3\n"

		"decl %4\n"
		"jnz 0b\n"
		"1:\n"

/* final run */
		/* set the current, current_pre, current_next registers */
		"movq -8(%1),%%mm0\n"
		"movq (%1),%%mm7\n"
		"pxor %%mm1,%%mm1\n" /* use a fake black out of screen */
		"psrlq $56,%%mm0\n"
		"psllq $56,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $8,%%mm2\n"
		"psrlq $8,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqb %%mm6,%%mm2\n"
		"pcmpeqb %%mm6,%%mm4\n"
		"pcmpeqb (%2),%%mm3\n"
		"pcmpeqb (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqb %%mm1,%%mm2\n"
		"pcmpeqb %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpcklbw %%mm4,%%mm2\n"
		"punpckhbw %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		: "+r" (src0), "+r" (src1), "+r" (src2), "+r" (dst), "+r" (count)
		:
		: "cc"
	);
}

static void internal_scale2x_8_mmx(uint8* dst0, uint8* dst1, const uint8* src0, const uint8* src1, const uint8* src2, unsigned count) {
	assert( count >= 2*8 );
	internal_scale2x_8_mmx_single(dst0, src0, src1, src2, count);
	internal_scale2x_8_mmx_single(dst1, src2, src1, src0, count);
}

static __inline__ void internal_scale2x_16_mmx_single(uint16* dst, const uint16* src0, const uint16* src1, const uint16* src2, unsigned count) {
	/* always do the first and last run */
	count -= 2*4;

	__asm__ __volatile__(
/* first run */
		/* set the current, current_pre, current_next registers */
		"pxor %%mm0,%%mm0\n" /* use a fake black out of screen */
		"movq 0(%1),%%mm7\n"
		"movq 8(%1),%%mm1\n"
		"psrlq $48,%%mm0\n"
		"psllq $48,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $16,%%mm2\n"
		"psrlq $16,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqw %%mm6,%%mm2\n"
		"pcmpeqw %%mm6,%%mm4\n"
		"pcmpeqw (%2),%%mm3\n"
		"pcmpeqw (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqw %%mm1,%%mm2\n"
		"pcmpeqw %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpcklwd %%mm4,%%mm2\n"
		"punpckhwd %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		/* next */
		"addl $8,%0\n"
		"addl $8,%1\n"
		"addl $8,%2\n"
		"addl $16,%3\n"

/* central runs */
		"shrl $2,%4\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"

		/* set the current, current_pre, current_next registers */
		"movq -8(%1),%%mm0\n"
		"movq (%1),%%mm7\n"
		"movq 8(%1),%%mm1\n"
		"psrlq $48,%%mm0\n"
		"psllq $48,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $16,%%mm2\n"
		"psrlq $16,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqw %%mm6,%%mm2\n"
		"pcmpeqw %%mm6,%%mm4\n"
		"pcmpeqw (%2),%%mm3\n"
		"pcmpeqw (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqw %%mm1,%%mm2\n"
		"pcmpeqw %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpcklwd %%mm4,%%mm2\n"
		"punpckhwd %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		/* next */
		"addl $8,%0\n"
		"addl $8,%1\n"
		"addl $8,%2\n"
		"addl $16,%3\n"

		"decl %4\n"
		"jnz 0b\n"
		"1:\n"

/* final run */
		/* set the current, current_pre, current_next registers */
		"movq -8(%1),%%mm0\n"
		"movq (%1),%%mm7\n"
		"pxor %%mm1,%%mm1\n" /* use a fake black out of screen */
		"psrlq $48,%%mm0\n"
		"psllq $48,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $16,%%mm2\n"
		"psrlq $16,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqw %%mm6,%%mm2\n"
		"pcmpeqw %%mm6,%%mm4\n"
		"pcmpeqw (%2),%%mm3\n"
		"pcmpeqw (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqw %%mm1,%%mm2\n"
		"pcmpeqw %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpcklwd %%mm4,%%mm2\n"
		"punpckhwd %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"
		: "+r" (src0), "+r" (src1), "+r" (src2), "+r" (dst), "+r" (count)
		:
		: "cc"
	);
}

static void internal_scale2x_16_mmx(uint16* dst0, uint16* dst1, const uint16* src0, const uint16* src1, const uint16* src2, unsigned count) {
	assert( count >= 2*4 );
	internal_scale2x_16_mmx_single(dst0, src0, src1, src2, count);
	internal_scale2x_16_mmx_single(dst1, src2, src1, src0, count);
}

static __inline__ void internal_scale2x_32_mmx_single(uint32* dst, const uint32* src0, const uint32* src1, const uint32* src2, unsigned count) {
	/* always do the first and last run */
	count -= 2*2;

	__asm__ __volatile__(
/* first run */
		/* set the current, current_pre, current_next registers */
		"pxor %%mm0,%%mm0\n" /* use a fake black out of screen */
		"movq 0(%1),%%mm7\n"
		"movq 8(%1),%%mm1\n"
		"psrlq $32,%%mm0\n"
		"psllq $32,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $32,%%mm2\n"
		"psrlq $32,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqd %%mm6,%%mm2\n"
		"pcmpeqd %%mm6,%%mm4\n"
		"pcmpeqd (%2),%%mm3\n"
		"pcmpeqd (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqd %%mm1,%%mm2\n"
		"pcmpeqd %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpckldq %%mm4,%%mm2\n"
		"punpckhdq %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		/* next */
		"addl $8,%0\n"
		"addl $8,%1\n"
		"addl $8,%2\n"
		"addl $16,%3\n"

/* central runs */
		"shrl $1,%4\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"

		/* set the current, current_pre, current_next registers */
		"movq -8(%1),%%mm0\n"
		"movq (%1),%%mm7\n"
		"movq 8(%1),%%mm1\n"
		"psrlq $32,%%mm0\n"
		"psllq $32,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $32,%%mm2\n"
		"psrlq $32,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqd %%mm6,%%mm2\n"
		"pcmpeqd %%mm6,%%mm4\n"
		"pcmpeqd (%2),%%mm3\n"
		"pcmpeqd (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqd %%mm1,%%mm2\n"
		"pcmpeqd %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpckldq %%mm4,%%mm2\n"
		"punpckhdq %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		/* next */
		"addl $8,%0\n"
		"addl $8,%1\n"
		"addl $8,%2\n"
		"addl $16,%3\n"

		"decl %4\n"
		"jnz 0b\n"
		"1:\n"

/* final run */
		/* set the current, current_pre, current_next registers */
		"movq -8(%1),%%mm0\n"
		"movq (%1),%%mm7\n"
		"pxor %%mm1,%%mm1\n" /* use a fake black out of screen */
		"psrlq $32,%%mm0\n"
		"psllq $32,%%mm1\n"
		"movq %%mm7,%%mm2\n"
		"movq %%mm7,%%mm3\n"
		"psllq $32,%%mm2\n"
		"psrlq $32,%%mm3\n"
		"por %%mm2,%%mm0\n"
		"por %%mm3,%%mm1\n"

		/* current_upper */
		"movq (%0),%%mm6\n"

		/* compute the upper-left pixel for dst0 on %%mm2 */
		/* compute the upper-right pixel for dst0 on %%mm4 */
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"movq %%mm0,%%mm3\n"
		"movq %%mm1,%%mm5\n"
		"pcmpeqd %%mm6,%%mm2\n"
		"pcmpeqd %%mm6,%%mm4\n"
		"pcmpeqd (%2),%%mm3\n"
		"pcmpeqd (%2),%%mm5\n"
		"pandn %%mm2,%%mm3\n"
		"pandn %%mm4,%%mm5\n"
		"movq %%mm0,%%mm2\n"
		"movq %%mm1,%%mm4\n"
		"pcmpeqd %%mm1,%%mm2\n"
		"pcmpeqd %%mm0,%%mm4\n"
		"pandn %%mm3,%%mm2\n"
		"pandn %%mm5,%%mm4\n"
		"movq %%mm2,%%mm3\n"
		"movq %%mm4,%%mm5\n"
		"pand %%mm6,%%mm2\n"
		"pand %%mm6,%%mm4\n"
		"pandn %%mm7,%%mm3\n"
		"pandn %%mm7,%%mm5\n"
		"por %%mm3,%%mm2\n"
		"por %%mm5,%%mm4\n"

		/* set *dst0 */
		"movq %%mm2,%%mm3\n"
		"punpckldq %%mm4,%%mm2\n"
		"punpckhdq %%mm4,%%mm3\n"
		"movq %%mm2,(%3)\n"
		"movq %%mm3,8(%3)\n"

		: "+r" (src0), "+r" (src1), "+r" (src2), "+r" (dst), "+r" (count)
		:
		: "cc"
	);
}

static void internal_scale2x_32_mmx(uint32* dst0, uint32* dst1, const uint32* src0, const uint32* src1, const uint32* src2, unsigned count) {
	assert( count >= 2*2 );
	internal_scale2x_32_mmx_single(dst0, src0, src1, src2, count);
	internal_scale2x_32_mmx_single(dst1, src2, src1, src0, count);
}



#endif

#endif
