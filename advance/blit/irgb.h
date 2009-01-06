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

#ifndef __IRGB_H
#define __IRGB_H

#include "icommon.h"

/***************************************************************************/
/* internal_rgb_raw */

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_raw128_012carry_mmx(void* dst, const void* src, const void* mask, const void* carry, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstqq = (*srcqq & maskqq[0]) + ((*srcqq >> 1) & maskqq[1]) + ((*srcqq >> 2) & maskqq[2]) + (*srcqq & (*srcqq >> 1) & carry) */
	__asm__ __volatile__(
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n" /* shift 0 */
		"movq 8(%0), %%mm4\n" /* shift 0 */
		"movq %%mm0, %%mm1\n" /* shift 1 */
		"movq %%mm4, %%mm5\n" /* shift 1 */
		"movq %%mm0, %%mm2\n" /* shift 2 */
		"movq %%mm4, %%mm6\n" /* shift 2 */
		"movq %%mm0, %%mm3\n" /* carry */
		"movq %%mm4, %%mm7\n" /* carry */
		"psrlq $1, %%mm1\n"
		"psrlq $1, %%mm5\n"
		"pand (%3), %%mm0\n"
		"pand 8(%3), %%mm4\n"
		"pand %%mm1, %%mm3\n"
		"pand %%mm5, %%mm7\n"
		"psrlq $2, %%mm2\n"
		"psrlq $2, %%mm6\n"
		"pand (%4), %%mm3\n"
		"pand 8(%4), %%mm7\n"
		"pand 16(%3), %%mm1\n"
		"pand 24(%3), %%mm5\n"
		"pand 32(%3), %%mm2\n"
		"pand 40(%3), %%mm6\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm5, %%mm4\n"
		"paddd %%mm3, %%mm2\n"
		"paddd %%mm7, %%mm6\n"
		"paddd %%mm2, %%mm0\n"
		"paddd %%mm6, %%mm4\n"
		"movq %%mm0, (%1)\n"
		"movq %%mm4, 8(%1)\n"

		"addl $16, %0\n"
		"addl $16, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask), "r" (carry)
		: "cc"
	);
}

static inline void internal_rgb_raw128_01_mmx(void* dst, const void* src, const void* mask, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstqq = (*srcqq & maskqq[0]) + ((*srcq >> 1) & maskqq[1])*/
	__asm__ __volatile__(
		"movq (%3), %%mm2\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm3\n"
		"movq 24(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq 8(%0), %%mm4\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm4, %%mm5\n"
		"psrlq $1, %%mm1\n"
		"psrlq $1, %%mm5\n"
		"pand %%mm2, %%mm0\n"
		"pand %%mm6, %%mm4\n"
		"pand %%mm3, %%mm1\n"
		"pand %%mm7, %%mm5\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm5, %%mm4\n"
		"movq %%mm0, (%1)\n"
		"movq %%mm4, 8(%1)\n"

		"addl $16, %0\n"
		"addl $16, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask)
		: "cc"
	);
}

#define internal_rgb_raw128_12carry_mmx internal_rgb_raw128_012carry_mmx
#define internal_rgb_raw128_1_mmx internal_rgb_raw128_01_mmx
#define internal_rgb_raw128_2_mmx internal_rgb_raw128_01_mmx

static inline void internal_rgb_raw64_012carry_mmx(void* dst, const void* src, const void* mask, const void* carry, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstq = (*srcq & maskq[0]) + ((*srcq >> 1) & maskq[1]) + ((*srcq >> 2) & maskq[2]) + (*srcq & (*srcq >> 1) & carry) */
	__asm__ __volatile__(
		"movq (%4), %%mm4\n" /* carry mask */
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n" /* shift 0 */
		"movq %%mm0, %%mm1\n" /* shift 1 */
		"movq %%mm0, %%mm2\n" /* shift 2 */
		"movq %%mm0, %%mm3\n" /* carry */
		"psrlq $1, %%mm1\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm1, %%mm3\n"
		"psrlq $2, %%mm2\n"
		"pand %%mm4, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm3, %%mm2\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask), "r" (carry)
		: "cc"
	);
}

static inline void internal_rgb_raw64_01_mmx(void* dst, const void* src, const void* mask, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstq = (*srcq & maskq[0]) + ((*srcq >> 1) & maskq[1])*/
	__asm__ __volatile__(
		"movq (%3), %%mm2\n"
		"movq 8(%3), %%mm3\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"psrlq $1, %%mm1\n"
		"pand %%mm2, %%mm0\n"
		"pand %%mm3, %%mm1\n"
		"paddd %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask)
		: "cc"
	);
}

static inline void internal_rgb_raw64_02_mmx(void* dst, const void* src, const void* mask, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstq = (*srcq & maskq[0]) + ((*srcq >> 2) & maskq[2])*/
	__asm__ __volatile__(
		"movq (%3), %%mm2\n"
		"movq 16(%3), %%mm3\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"psrlq $2, %%mm1\n"
		"pand %%mm2, %%mm0\n"
		"pand %%mm3, %%mm1\n"
		"paddd %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask)
		: "cc"
	);
}

static inline void internal_rgb_raw64_12carry_mmx(void* dst, const void* src, const void* mask, void* carry, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstq = ((*srcq >> 1) & maskq[1]) + ((*srcq >> 2) & maskq[2]) + (*srcq & (*srcq >> 1) & carry) */
	__asm__ __volatile__(
		"movq (%4), %%mm3\n" /* carry mask */
		"movq 8(%3), %%mm4\n"
		"movq 16(%3), %%mm5\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n" /* shift 1 */
		"movq %%mm0, %%mm1\n" /* shift 2 */
		"movq %%mm0, %%mm2\n" /* carry */
		"psrlq $1, %%mm0\n"
		"psrlq $2, %%mm1\n"
		"pand %%mm0, %%mm2\n"
		"pand %%mm4, %%mm0\n"
		"pand %%mm5, %%mm1\n"
		"pand %%mm3, %%mm2\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask), "r" (carry)
		: "cc"
	);
}

static inline void internal_rgb_raw64_1_mmx(void* dst, const void* src, const void* mask, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstq = ((*srcq >> 1) & maskq[1]) */
	__asm__ __volatile__(
		"movq 8(%3), %%mm2\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"psrlq $1, %%mm0\n"
		"pand %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask)
		: "cc"
	);
}

static inline void internal_rgb_raw64_2_mmx(void* dst, const void* src, const void* mask, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* *dstq = ((*srcq >> 2) & maskq[2]) */
	__asm__ __volatile__(
		"movq 16(%3), %%mm2\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"psrlq $2, %%mm0\n"
		"pand %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask)
		: "cc"
	);
}

static inline void internal_rgb_raw64x3_012_mmx(void* dst, const void* src, const void* mask, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	/* dstq[0] = (srcq[0] & maskq[0]) + ((srcq[0] >> 1) & maskq[3]) + ((srcq[0] >> 2) & maskq[6]) */
	/* dstq[1] = (srcq[1] & maskq[1]) + ((srcq[1] >> 1) & maskq[4]) + ((srcq[1] >> 2) & maskq[7]) */
	/* dstq[2] = (srcq[2] & maskq[2]) + ((srcq[2] >> 1) & maskq[5]) + ((srcq[2] >> 2) & maskq[8]) */
	__asm__ __volatile__(
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrlq $1, %%mm1\n"
		"pand (%3), %%mm0\n"
		"psrlq $2, %%mm2\n"
		"pand 24(%3), %%mm1\n"
		"pand 48(%3), %%mm2\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jz 1f\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrlq $1, %%mm1\n"
		"pand 8(%3), %%mm0\n"
		"psrlq $2, %%mm2\n"
		"pand 32(%3), %%mm1\n"
		"pand 56(%3), %%mm2\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jz 1f\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrlq $1, %%mm1\n"
		"pand 16(%3), %%mm0\n"
		"psrlq $2, %%mm2\n"
		"pand 40(%3), %%mm1\n"
		"pand 64(%3), %%mm2\n"
		"paddd %%mm1, %%mm0\n"
		"paddd %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"

		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (mask)
		: "cc"
	);
}

#endif

/* TODO Carry version not implemented */
#define internal_rgb_raw32_012carry_def internal_rgb_raw32_012_def

static inline void internal_rgb_raw32_012_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_01_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

/* TODO Carry version not implemented */
#define internal_rgb_raw32_12carry_def internal_rgb_raw32_12_def

static inline void internal_rgb_raw32_12_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_1_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 >> 1) & mask[2];
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_2_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 >> 2) & mask[4];
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32_02_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
		dst32 += 1;
		src32 += 1;
		--count;
	}
}

static inline void internal_rgb_raw32x2_012_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[3]) + ((v1 >> 2) & mask[5]);
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x2_01_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[3]);
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[2]);
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x2_12_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		v1 = ((v1 >> 1) & mask[3]) + ((v1 >> 2) & mask[5]);
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = ((v0 >> 1) & mask[2]) + ((v0 >> 2) & mask[4]);
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x2_1_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count >= 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];
		v0 = (v0 >> 1) & mask[2];
		v1 = (v1 >> 1) & mask[3];
		dst32[0] = v0;
		dst32[1] = v1;
		dst32 += 2;
		src32 += 2;
		count -= 2;
	}

	if (count == 1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 >> 1) & mask[2];
		dst32[0] = v0;
	}
}

static inline void internal_rgb_raw32x3_012_def(uint32* dst32, const uint32* src32, const uint32* mask, unsigned count)
{
	assert_align(((unsigned)src32 & 0x3) == 0 && ((unsigned)dst32 & 0x3) == 0);

	while (count>=3) {
		unsigned v0, v1, v2;
		v0 = src32[0];
		v1 = src32[1];
		v2 = src32[2];

		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[6]) + ((v0 >> 2) & mask[12]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[7]) + ((v1 >> 2) & mask[13]);
		v2 = (v2 & mask[2]) + ((v2 >> 1) & mask[8]) + ((v2 >> 2) & mask[14]);

		dst32[0] = v0;
		dst32[1] = v1;
		dst32[2] = v2;

		dst32 += 3;
		src32 += 3;
		count -= 3;
	}

	if (count==1) {
		unsigned v0;
		v0 = src32[0];
		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[6]) + ((v0 >> 2) & mask[12]);
		dst32[0] = v0;
	} else if (count == 2) {
		unsigned v0, v1;
		v0 = src32[0];
		v1 = src32[1];

		v0 = (v0 & mask[0]) + ((v0 >> 1) & mask[6]) + ((v0 >> 2) & mask[12]);
		v1 = (v1 & mask[1]) + ((v1 >> 1) & mask[7]) + ((v1 >> 2) & mask[13]);

		dst32[0] = v0;
		dst32[1] = v1;
	}
}

/***************************************************************************/
/* rgb_compute */

static unsigned rgb_raw_pixel_mask_compute(const struct video_pipeline_target_struct* target, unsigned shift, unsigned sub_mask)
{
	unsigned v;
	unsigned red_mask;
	unsigned green_mask;
	unsigned blue_mask;
	int red_shift;
	int green_shift;
	int blue_shift;
	adv_pixel low_mask;

	union adv_color_def_union def;

	def.ordinal = target->color_def;

	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	v = 0;

	/* red */
	if (sub_mask & 0x4)
		v |= red_mask & (red_mask >> shift);

	/* green */
	if (sub_mask & 0x2)
		v |= green_mask & (green_mask >> shift);

	/* blue */
	if (sub_mask & 0x1)
		v |= blue_mask & (blue_mask >> shift);

	return v;
}

static unsigned byte_raw_pixel_mask_compute(const struct video_pipeline_target_struct* target, unsigned shift, unsigned sub_mask)
{
	unsigned v;

	v = 0;

	if (sub_mask & 0x1)
		v |= 0xFFU & (0xFFU >> shift);

	if (sub_mask & 0x2)
		v |= 0xFF00U & (0xFF00U >> shift);

	if (sub_mask & 0x4)
		v |= 0xFF0000U & (0xFF0000U >> shift);

	if (sub_mask & 0x8)
		v |= 0xFF000000U & (0xFF000000U >> shift);

	return v;
}

static unsigned rgb_raw_carry_mask_compute(const struct video_pipeline_target_struct* target, unsigned sub_mask)
{
	unsigned v;
	unsigned red_mask;
	unsigned green_mask;
	unsigned blue_mask;
	int red_shift;
	int green_shift;
	int blue_shift;
	adv_pixel low_mask;

	union adv_color_def_union def;

	def.ordinal = target->color_def;

	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	low_mask = rgb_lowmask_make_from_def(target->color_def);

	v = 0;

	/* red */
	if (sub_mask & 0x4)
		v |= low_mask & red_mask;

	/* green */
	if (sub_mask & 0x2)
		v |= low_mask & green_mask;

	/* blue */
	if (sub_mask & 0x1)
		v |= low_mask & blue_mask;

	return v;
}

static void rgb_raw_mask4_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask >> 12);
	unsigned pixel1 = rgb_raw_pixel_mask_compute(target, shift, mask >> 8);
	unsigned pixel2 = rgb_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel3 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel2, pixel3);
		dst[1] = dst[0];
		dst[2] = dst[0];
		dst[3] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = cpu_uint32_make_uint16(pixel2, pixel3);
		dst[2] = dst[0];
		dst[3] = dst[1];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		dst[2] = pixel2;
		dst[3] = pixel3;
		break;
	default:
		assert(0);
	}
}

static void rgb_raw_carry4_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned mask)
{
	unsigned pixel0 = rgb_raw_carry_mask_compute(target, mask >> 12);
	unsigned pixel1 = rgb_raw_carry_mask_compute(target, mask >> 8);
	unsigned pixel2 = rgb_raw_carry_mask_compute(target, mask >> 4);
	unsigned pixel3 = rgb_raw_carry_mask_compute(target, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel2, pixel3);
		dst[1] = dst[0];
		dst[2] = dst[0];
		dst[3] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = cpu_uint32_make_uint16(pixel2, pixel3);
		dst[2] = dst[0];
		dst[3] = dst[1];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		dst[2] = pixel2;
		dst[3] = pixel3;
		break;
	default:
		assert(0);
	}
}

static void rgb_raw_mask3_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask >> 8);
	unsigned pixel1 = rgb_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel2 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel2, pixel0);
		dst[1] = cpu_uint32_make_uint8(pixel1, pixel2, pixel0, pixel1);
		dst[2] = cpu_uint32_make_uint8(pixel2, pixel0, pixel1, pixel2);
		dst[3] = dst[0];
		dst[4] = dst[1];
		dst[5] = dst[2];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = cpu_uint32_make_uint16(pixel2, pixel0);
		dst[2] = cpu_uint32_make_uint16(pixel1, pixel2);
		dst[3] = dst[0];
		dst[4] = dst[1];
		dst[5] = dst[2];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		dst[2] = pixel2;
		dst[3] = dst[0];
		dst[4] = dst[1];
		dst[5] = dst[2];
		break;
	default:
		assert(0);
	}
}

static void byte_raw_mask3_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = byte_raw_pixel_mask_compute(target, shift, mask >> 8);
	unsigned pixel1 = byte_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel2 = byte_raw_pixel_mask_compute(target, shift, mask);

	dst[0] = pixel0;
	dst[1] = pixel1;
	dst[2] = pixel2;
	dst[3] = dst[0];
	dst[4] = dst[1];
	dst[5] = dst[2];
}

static void rgb_raw_mask2_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel1 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		break;
	default:
		assert(0);
	}
}

static void byte_raw_mask2_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = byte_raw_pixel_mask_compute(target, shift, mask >> 4);
	unsigned pixel1 = byte_raw_pixel_mask_compute(target, shift, mask);

	dst[0] = pixel0;
	dst[1] = pixel1;
}

static void rgb_raw_carry2_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned mask)
{
	unsigned pixel0 = rgb_raw_carry_mask_compute(target, mask >> 4);
	unsigned pixel1 = rgb_raw_carry_mask_compute(target, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel1, pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel1);
		dst[1] = dst[0];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = pixel1;
		break;
	default:
		assert(0);
	}
}

static void rgb_raw_mask1_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = rgb_raw_pixel_mask_compute(target, shift, mask);

	switch (target->bytes_per_pixel) {
	case 1 :
		dst[0] = cpu_uint32_make_uint8(pixel0, pixel0, pixel0, pixel0);
		dst[1] = dst[0];
		break;
	case 2 :
		dst[0] = cpu_uint32_make_uint16(pixel0, pixel0);
		dst[1] = dst[0];
		break;
	case 4 :
		dst[0] = pixel0;
		dst[1] = dst[0];
		break;
	default:
		assert(0);
	}
}

static void byte_raw_mask1_compute(const struct video_pipeline_target_struct* target, uint32* dst, unsigned shift, unsigned mask)
{
	unsigned pixel0 = byte_raw_pixel_mask_compute(target, shift, mask);

	dst[0] = pixel0;
	dst[1] = dst[0];
}

/***************************************************************************/
/* internal_rgb_triad16pix */

/*
This mask is applyed at the image:

  .... (type 0)
  GBR. (type 1)
  GBR.
  GBR.
  .... (type 0)
  .GBR (type 2)
  .GBR
  .GBR

. = red:75%, green:75%, blue:75%
G = red:75%, green:100%, blue:75%
B = red:75%, green:75%, blue:100%
R = red:100%, green:75%, blue:75%
*/

enum RGB_TRIAD16PIX_MASK {
	RGB_TRIAD16PIX_MASK_0_0_0, /* type 0 shift 0 addr 0 */
	RGB_TRIAD16PIX_MASK_0_0_1, /* type 0 shift 0 addr 1 */
	RGB_TRIAD16PIX_MASK_0_0_2, /* type 0 shift 0 addr 2 */
	RGB_TRIAD16PIX_MASK_0_0_3, /* type 0 shift 0 addr 3 */
	RGB_TRIAD16PIX_MASK_0_1_0, /* type 0 shift 1 addr 0 */
	RGB_TRIAD16PIX_MASK_0_1_1, /* type 0 shift 1 addr 1 */
	RGB_TRIAD16PIX_MASK_0_1_2, /* type 0 shift 1 addr 2 */
	RGB_TRIAD16PIX_MASK_0_1_3, /* type 0 shift 1 addr 3 */
	RGB_TRIAD16PIX_MASK_0_2_0, /* ... */
	RGB_TRIAD16PIX_MASK_0_2_1,
	RGB_TRIAD16PIX_MASK_0_2_2, /* ... */
	RGB_TRIAD16PIX_MASK_0_2_3,
	RGB_TRIAD16PIX_MASK_0_c_0, /* type 0 carry addr 0 */
	RGB_TRIAD16PIX_MASK_0_c_1, /* type 0 carry addr 1 */
	RGB_TRIAD16PIX_MASK_0_c_2, /* type 0 carry addr 2 */
	RGB_TRIAD16PIX_MASK_0_c_3, /* type 0 carry addr 3 */
	RGB_TRIAD16PIX_MASK_1_0_0,
	RGB_TRIAD16PIX_MASK_1_0_1,
	RGB_TRIAD16PIX_MASK_1_0_2,
	RGB_TRIAD16PIX_MASK_1_0_3,
	RGB_TRIAD16PIX_MASK_1_1_0,
	RGB_TRIAD16PIX_MASK_1_1_1,
	RGB_TRIAD16PIX_MASK_1_1_2,
	RGB_TRIAD16PIX_MASK_1_1_3,
	RGB_TRIAD16PIX_MASK_1_2_0,
	RGB_TRIAD16PIX_MASK_1_2_1,
	RGB_TRIAD16PIX_MASK_1_2_2,
	RGB_TRIAD16PIX_MASK_1_2_3,
	RGB_TRIAD16PIX_MASK_1_c_0,
	RGB_TRIAD16PIX_MASK_1_c_1,
	RGB_TRIAD16PIX_MASK_1_c_2,
	RGB_TRIAD16PIX_MASK_1_c_3,
	RGB_TRIAD16PIX_MASK_2_0_0,
	RGB_TRIAD16PIX_MASK_2_0_1,
	RGB_TRIAD16PIX_MASK_2_0_2,
	RGB_TRIAD16PIX_MASK_2_0_3,
	RGB_TRIAD16PIX_MASK_2_1_0,
	RGB_TRIAD16PIX_MASK_2_1_1,
	RGB_TRIAD16PIX_MASK_2_1_2,
	RGB_TRIAD16PIX_MASK_2_1_3,
	RGB_TRIAD16PIX_MASK_2_2_0,
	RGB_TRIAD16PIX_MASK_2_2_1,
	RGB_TRIAD16PIX_MASK_2_2_2,
	RGB_TRIAD16PIX_MASK_2_2_3,
	RGB_TRIAD16PIX_MASK_2_c_0,
	RGB_TRIAD16PIX_MASK_2_c_1,
	RGB_TRIAD16PIX_MASK_2_c_2,
	RGB_TRIAD16PIX_MASK_2_c_3,
	RGB_TRIAD16PIX_MASK_MAX
};

static void internal_rgb_triad16pix_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	/* type 0 */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_0_0_0, 0, 0x0000); /* factor 2^0 = 100% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_0_1_0, 1, 0x7777); /* factor 2^-1 = 50% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_0_2_0, 2, 0x7777); /* factor 2^-2 = 25% */
	rgb_raw_carry4_compute(target, data + RGB_TRIAD16PIX_MASK_0_c_0, 0x7777); /* carry */

	/* type 1 */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_1_0_0, 0, 0x2140); /* factor 2^0 = 100% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_1_1_0, 1, 0x5637); /* factor 2^-1 = 50% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_1_2_0, 2, 0x5637); /* factor 2^-2 = 25% */
	rgb_raw_carry4_compute(target, data + RGB_TRIAD16PIX_MASK_1_c_0, 0x5637); /* carry */

	/* type 2 */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_2_0_0, 0, 0x0214); /* factor 2^0 = 100% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_2_1_0, 1, 0x7563); /* factor 2^-1 = 50% */
	rgb_raw_mask4_compute(target, data + RGB_TRIAD16PIX_MASK_2_2_0, 2, 0x7563); /* factor 2^-2 = 25% */
	rgb_raw_carry4_compute(target, data + RGB_TRIAD16PIX_MASK_2_c_0, 0x7563); /* carry */
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_triad16pix8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 16);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 16);
		break;
	default :
		internal_rgb_raw128_012carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 16);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 16);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 16);
		break;
	default :
		internal_rgb_raw128_01_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 16);
		break;
	}
}

static inline void internal_rgb_triad16pix16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 8);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 8);
		break;
	default :
		internal_rgb_raw128_012carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 8);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 8);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 8);
		break;
	default :
		internal_rgb_raw128_01_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 8);
		break;
	}
}

static inline void internal_rgb_triad16pix32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_12carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, data + RGB_TRIAD16PIX_MASK_0_c_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_012carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, data + RGB_TRIAD16PIX_MASK_1_c_0, count / 4);
		break;
	default :
		internal_rgb_raw128_012carry_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, data + RGB_TRIAD16PIX_MASK_2_c_0, count / 4);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw128_1_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw128_01_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw128_01_mmx(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}
}

#endif

static inline void internal_rgb_triad16pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32_12carry_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 4);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 4);
		break;
	default :
		internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 4);
		break;
	}
}

static inline void internal_rgb_triad16pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_12_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 2);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_012_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 2);
		break;
	default :
		internal_rgb_raw32x2_012_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 2);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_1_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_0_0_0, count / 2);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_1_0_0, count / 2);
		break;
	default :
		internal_rgb_raw32x2_01_def((uint32*)dst, (uint32*)src, data + RGB_TRIAD16PIX_MASK_2_0_0, count / 2);
		break;
	}
}

static inline void internal_rgb_triad16pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
	case 0 :
	case 4 :
		internal_rgb_raw32x2_12_def(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count);
		break;
	case 1 :
	case 2 :
	case 3 :
		internal_rgb_raw32x2_012_def(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count);
		break;
	default :
		internal_rgb_raw32x2_012_def(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count);
		break;
	}
}

static inline void internal_rgb_triadstrong16pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 8) {
		case 0 :
		case 4 :
			internal_rgb_raw32x2_1_def(dst, src, data + RGB_TRIAD16PIX_MASK_0_0_0, count);
			break;
		case 1 :
		case 2 :
		case 3 :
			internal_rgb_raw32x2_01_def(dst, src, data + RGB_TRIAD16PIX_MASK_1_0_0, count);
			break;
		default :
			internal_rgb_raw32x2_01_def(dst, src, data + RGB_TRIAD16PIX_MASK_2_0_0, count);
			break;
	}
}

/***************************************************************************/
/* internal_rgb_triad6pix */

/*
This mask is applyed at the image:

  BR (type 0)
  BG (type 1)
  RG (type 2)
  RB (type 3)
  GB (type 4)
  GR (type 5)

G = red:75%, green:100%, blue:75%
B = red:75%, green:75%, blue:100%
R = red:100%, green:75%, blue:75%
*/

enum RGB_TRIAD6PIX_MASK {
	RGB_TRIAD6PIX_MASK_0_0_0, /* type 0 shift 0 addr 0 */
	RGB_TRIAD6PIX_MASK_0_0_1, /* type 0 shift 0 addr 1 */
	RGB_TRIAD6PIX_MASK_0_1_0, /* type 0 shift 1 addr 0 */
	RGB_TRIAD6PIX_MASK_0_1_1, /* type 0 shift 1 addr 1 */
	RGB_TRIAD6PIX_MASK_0_2_0, /* ... */
	RGB_TRIAD6PIX_MASK_0_2_1,
	RGB_TRIAD6PIX_MASK_0_c_0,
	RGB_TRIAD6PIX_MASK_0_c_1,
	RGB_TRIAD6PIX_MASK_1_0_0,
	RGB_TRIAD6PIX_MASK_1_0_1,
	RGB_TRIAD6PIX_MASK_1_1_0,
	RGB_TRIAD6PIX_MASK_1_1_1,
	RGB_TRIAD6PIX_MASK_1_2_0,
	RGB_TRIAD6PIX_MASK_1_2_1,
	RGB_TRIAD6PIX_MASK_1_c_0,
	RGB_TRIAD6PIX_MASK_1_c_1,
	RGB_TRIAD6PIX_MASK_2_0_0,
	RGB_TRIAD6PIX_MASK_2_0_1,
	RGB_TRIAD6PIX_MASK_2_1_0,
	RGB_TRIAD6PIX_MASK_2_1_1,
	RGB_TRIAD6PIX_MASK_2_2_0,
	RGB_TRIAD6PIX_MASK_2_2_1,
	RGB_TRIAD6PIX_MASK_2_c_0,
	RGB_TRIAD6PIX_MASK_2_c_1,
	RGB_TRIAD6PIX_MASK_3_0_0,
	RGB_TRIAD6PIX_MASK_3_0_1,
	RGB_TRIAD6PIX_MASK_3_1_0,
	RGB_TRIAD6PIX_MASK_3_1_1,
	RGB_TRIAD6PIX_MASK_3_2_0,
	RGB_TRIAD6PIX_MASK_3_2_1,
	RGB_TRIAD6PIX_MASK_3_c_0,
	RGB_TRIAD6PIX_MASK_3_c_1,
	RGB_TRIAD6PIX_MASK_4_0_0,
	RGB_TRIAD6PIX_MASK_4_0_1,
	RGB_TRIAD6PIX_MASK_4_1_0,
	RGB_TRIAD6PIX_MASK_4_1_1,
	RGB_TRIAD6PIX_MASK_4_2_0,
	RGB_TRIAD6PIX_MASK_4_2_1,
	RGB_TRIAD6PIX_MASK_4_c_0,
	RGB_TRIAD6PIX_MASK_4_c_1,
	RGB_TRIAD6PIX_MASK_5_0_0,
	RGB_TRIAD6PIX_MASK_5_0_1,
	RGB_TRIAD6PIX_MASK_5_1_0,
	RGB_TRIAD6PIX_MASK_5_1_1,
	RGB_TRIAD6PIX_MASK_5_2_0,
	RGB_TRIAD6PIX_MASK_5_2_1,
	RGB_TRIAD6PIX_MASK_5_c_0,
	RGB_TRIAD6PIX_MASK_5_c_1,
	RGB_TRIAD6PIX_MASK_MAX
};

static void internal_rgb_triad6pix_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	/* type 0 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_0_0_0, 0, 0x14); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_0_1_0, 1, 0x63); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_0_2_0, 2, 0x63); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_0_c_0, 0x63); /* carry */

	/* type 1 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_1_0_0, 0, 0x12); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_1_1_0, 1, 0x65); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_1_2_0, 2, 0x65); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_1_c_0, 0x65); /* carry */

	/* type 2 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_2_0_0, 0, 0x42); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_2_1_0, 1, 0x35); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_2_2_0, 2, 0x35); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_2_c_0, 0x35); /* carry */

	/* type 3 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_3_0_0, 0, 0x41); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_3_1_0, 1, 0x36); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_3_2_0, 2, 0x36); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_3_c_0, 0x36); /* carry */

	/* type 4 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_4_0_0, 0, 0x21); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_4_1_0, 1, 0x56); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_4_2_0, 2, 0x56); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_4_c_0, 0x56); /* carry */

	/* type 5 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_5_0_0, 0, 0x24); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_5_1_0, 1, 0x53); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD6PIX_MASK_5_2_0, 2, 0x53); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD6PIX_MASK_5_c_0, 0x53); /* carry */
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_triad6pix8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_mmx(dst, src, mask, carry, count / 8);
}

static inline void internal_rgb_triadstrong6pix8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_mmx(dst, src, mask, count / 8);
}

static inline void internal_rgb_triad6pix16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_mmx(dst, src, mask, carry, count / 4);
}

static inline void internal_rgb_triadstrong6pix16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_mmx(dst, src, mask, count / 4);
}

static inline void internal_rgb_triad6pix32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);
	const uint32* carry = mask + (RGB_TRIAD6PIX_MASK_0_c_0 - RGB_TRIAD6PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_mmx(dst, src, mask, carry, count / 2);
}

static inline void internal_rgb_triadstrong6pix32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw64_01_mmx(dst, src, mask, count / 2);
}

#endif

static inline void internal_rgb_triad6pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 4);
}

static inline void internal_rgb_triadstrong6pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 4);
}

static inline void internal_rgb_triad6pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 2);
}

static inline void internal_rgb_triadstrong6pix16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 2);
}

static inline void internal_rgb_triad6pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_012carry_def(dst, src, mask, count);
}

static inline void internal_rgb_triadstrong6pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD6PIX_MASK_1_0_0 - RGB_TRIAD6PIX_MASK_0_0_0) * (line % 6);

	internal_rgb_raw32_01_def(dst, src, mask, count);
}

/***************************************************************************/
/* internal_rgb_triad3pix */

/*
This mask is applyed at the image:

  RB (type 0)
  GR (type 1)
  BG (type 2)

R = red:100%, green:75%, blue:75%
G = red:75%, green:100%, blue:75%
B = red:75%, green:75%, blue:100%
*/

enum RGB_TRIAD3PIX_MASK {
	RGB_TRIAD3PIX_MASK_0_0_0, /* type 0 shift 0 addr 0 */
	RGB_TRIAD3PIX_MASK_0_0_1, /* type 0 shift 0 addr 1 */
	RGB_TRIAD3PIX_MASK_0_1_0, /* type 0 shift 1 addr 0 */
	RGB_TRIAD3PIX_MASK_0_1_1, /* type 0 shift 1 addr 1 */
	RGB_TRIAD3PIX_MASK_0_2_0, /* ... */
	RGB_TRIAD3PIX_MASK_0_2_1,
	RGB_TRIAD3PIX_MASK_0_c_0,
	RGB_TRIAD3PIX_MASK_0_c_1,
	RGB_TRIAD3PIX_MASK_1_0_0,
	RGB_TRIAD3PIX_MASK_1_0_1,
	RGB_TRIAD3PIX_MASK_1_1_0,
	RGB_TRIAD3PIX_MASK_1_1_1,
	RGB_TRIAD3PIX_MASK_1_2_0,
	RGB_TRIAD3PIX_MASK_1_2_1,
	RGB_TRIAD3PIX_MASK_1_c_0,
	RGB_TRIAD3PIX_MASK_1_c_1,
	RGB_TRIAD3PIX_MASK_2_0_0,
	RGB_TRIAD3PIX_MASK_2_0_1,
	RGB_TRIAD3PIX_MASK_2_1_0,
	RGB_TRIAD3PIX_MASK_2_1_1,
	RGB_TRIAD3PIX_MASK_2_2_0,
	RGB_TRIAD3PIX_MASK_2_2_1,
	RGB_TRIAD3PIX_MASK_2_c_0,
	RGB_TRIAD3PIX_MASK_2_c_1,
	RGB_TRIAD3PIX_MASK_MAX
};

static void internal_rgb_triad3pix_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	/* type 0 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_0_0_0, 0, 0x41); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_0_1_0, 1, 0x36); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_0_2_0, 2, 0x36); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD3PIX_MASK_0_c_0, 0x36); /* carry */

	/* type 1 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_1_0_0, 0, 0x24); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_1_1_0, 1, 0x53); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_1_2_0, 2, 0x53); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD3PIX_MASK_1_c_0, 0x53); /* carry */

	/* type 2 */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_2_0_0, 0, 0x12); /* factor 2^0 = 100% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_2_1_0, 1, 0x65); /* factor 2^-1 = 50% */
	rgb_raw_mask2_compute(target, data + RGB_TRIAD3PIX_MASK_2_2_0, 2, 0x65); /* factor 2^-2 = 25% */
	rgb_raw_carry2_compute(target, data + RGB_TRIAD3PIX_MASK_2_c_0, 0x65); /* carry */
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_triad3pix8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_mmx(dst, src, mask, carry, count / 8);
}

static inline void internal_rgb_triadstrong3pix8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw64_01_mmx(dst, src, mask, count / 8);
}

static inline void internal_rgb_triad3pix16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_mmx(dst, src, mask, carry, count / 4);
}

static inline void internal_rgb_triadstrong3pix16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw64_01_mmx(dst, src, mask, count / 4);
}

static inline void internal_rgb_triad3pix32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);
	const uint32* carry = mask + (RGB_TRIAD3PIX_MASK_0_c_0 - RGB_TRIAD3PIX_MASK_0_0_0);

	internal_rgb_raw64_012carry_mmx(dst, src, mask, carry, count / 2);
}

static inline void internal_rgb_triadstrong3pix32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw64_01_mmx(dst, src, mask, count / 2);
}

#endif

static inline void internal_rgb_triad3pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 4);
}

static inline void internal_rgb_triadstrong3pix8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_0_0 - RGB_TRIAD3PIX_MASK_0_0_0) * (line % 3);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 4);
}

static inline void internal_rgb_triad3pix16_def(unsigned line, uint8* dst, const uint16* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_012carry_def((uint32*)dst, (uint32*)src, mask, count / 2);
}

static inline void internal_rgb_triadstrong3pix16_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, mask, count / 2);
}

static inline void internal_rgb_triad3pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_012carry_def(dst, src, mask, count);
}

static inline void internal_rgb_triadstrong3pix32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	const uint32* mask = data + (RGB_TRIAD3PIX_MASK_1_1_0 - RGB_TRIAD3PIX_MASK_0_1_0) * (line % 3);

	internal_rgb_raw32_01_def(dst, src, mask, count);
}

/***************************************************************************/
/* internal_rgb_scandouble */

enum RGB_SCANDOUBLE_MASK {
	RGB_SCANDOUBLE_MASK_0_0_0,
	RGB_SCANDOUBLE_MASK_0_0_1,
	RGB_SCANDOUBLE_MASK_0_1_0,
	RGB_SCANDOUBLE_MASK_0_1_1,
	RGB_SCANDOUBLE_MASK_1_0_0,
	RGB_SCANDOUBLE_MASK_1_0_1,
	RGB_SCANDOUBLE_MASK_1_1_0,
	RGB_SCANDOUBLE_MASK_1_1_1,
	RGB_SCANDOUBLE_MASK_MAX
};

static void internal_rgb_scandouble_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_0_0, 0, 0x7); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */

		/* type 1 */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_0_0, 0, 0x0); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_1_0, 1, 0x7); /* factor 2^-1 = 50% */
	}  else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_0_0, 0, 0xF); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */

		/* type 1 */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_0_0, 0, 0xA); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANDOUBLE_MASK_1_1_0, 1, 0x5); /* factor 2^-1 = 50% */
	}
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_scandouble8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		internal_rgb_raw64_1_mmx(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 8);
	} else {
		internal_copy8_mmx(dst, src, count);
	}
}

static inline void internal_rgb_scandouble16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		internal_rgb_raw64_1_mmx(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 4);
	} else {
		internal_copy16_mmx(dst, src, count);
	}
}

static inline void internal_rgb_scandouble32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		if (data[RGB_SCANDOUBLE_MASK_1_0_0] == 0)
			internal_rgb_raw64_1_mmx(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
		else
			internal_rgb_raw64_01_mmx(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
	} else {
		internal_copy32_mmx(dst, src, count);
	}
}
#endif

static inline void internal_rgb_scandouble8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 4);
	} else {
		internal_copy8_def(dst, src, count);
	}
}

static inline void internal_rgb_scandouble16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLE_MASK_1_0_0, count / 2);
	} else {
		internal_copy16_def(dst, src, count);
	}
}

static inline void internal_rgb_scandouble32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	if (line % 2) {
		if (data[RGB_SCANDOUBLE_MASK_1_0_0] == 0)
			internal_rgb_raw32_1_def(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count);
		else
			internal_rgb_raw32_01_def(dst, src, data + RGB_SCANDOUBLE_MASK_1_0_0, count);
	} else {
		internal_copy32_def(dst, src, count);
	}
}

/***************************************************************************/
/* internal_rgb_scandoublevert */

enum RGB_SCANDOUBLEVERT_MASK {
	RGB_SCANDOUBLEVERT_MASK_0_0_0,
	RGB_SCANDOUBLEVERT_MASK_0_0_1,
	RGB_SCANDOUBLEVERT_MASK_0_1_0,
	RGB_SCANDOUBLEVERT_MASK_0_1_1,
	RGB_SCANDOUBLEVERT_MASK_MAX
};

static void internal_rgb_scandoublevert_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, 0, 0x70); /* factor 2^0 = 100% */
		rgb_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_1_0, 1, 0x07); /* factor 2^-1 = 50% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, 0, 0xFA); /* factor 2^0 = 100% */
		byte_raw_mask2_compute(target, data + RGB_SCANDOUBLEVERT_MASK_0_1_0, 1, 0x05); /* factor 2^-1 = 50% */
	}
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_scandoublevert8_mmx(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	internal_rgb_raw64_01_mmx(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 8);
}

static inline void internal_rgb_scandoublevert16_mmx(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	internal_rgb_raw64_01_mmx(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 4);
}

static inline void internal_rgb_scandoublevert32_mmx(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	internal_rgb_raw64_01_mmx(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 2);
}

#endif

static inline void internal_rgb_scandoublevert8_def(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 4);
}

static inline void internal_rgb_scandoublevert16_def(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32_01_def((uint32*)dst, (uint32*)src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count / 2);
}

static inline void internal_rgb_scandoublevert32_def(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32_01_def(dst, src, data + RGB_SCANDOUBLEVERT_MASK_0_0_0, count);
}

/***************************************************************************/
/* internal_rgb_scantriple */

enum RGB_SCANTRIPLE_MASK {
	RGB_SCANTRIPLE_MASK_0_0_0,
	RGB_SCANTRIPLE_MASK_0_0_1,
	RGB_SCANTRIPLE_MASK_0_1_0,
	RGB_SCANTRIPLE_MASK_0_1_1,
	RGB_SCANTRIPLE_MASK_0_2_0,
	RGB_SCANTRIPLE_MASK_0_2_1,
	RGB_SCANTRIPLE_MASK_1_0_0,
	RGB_SCANTRIPLE_MASK_1_0_1,
	RGB_SCANTRIPLE_MASK_1_1_0,
	RGB_SCANTRIPLE_MASK_1_1_1,
	RGB_SCANTRIPLE_MASK_1_2_0,
	RGB_SCANTRIPLE_MASK_1_2_1,
	RGB_SCANTRIPLE_MASK_2_0_0,
	RGB_SCANTRIPLE_MASK_2_0_1,
	RGB_SCANTRIPLE_MASK_2_1_0,
	RGB_SCANTRIPLE_MASK_2_1_1,
	RGB_SCANTRIPLE_MASK_2_2_0,
	RGB_SCANTRIPLE_MASK_2_2_1,
	RGB_SCANTRIPLE_MASK_MAX
};

static void internal_rgb_scantriple_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_0_0, 0, 0x7); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 1 */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_0_0, 0, 0x0); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_1_0, 1, 0x7); /* factor 2^-1 = 50% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 2 */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_0_0, 0, 0x0); /* factor 2^0 = 100% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		rgb_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_2_0, 2, 0x7); /* factor 2^-2 = 25% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_0_0, 0, 0xF); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_0_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 1 */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_0_0, 0, 0xA); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_1_0, 1, 0x5); /* factor 2^-1 = 50% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_1_2_0, 2, 0x0); /* factor 2^-2 = 25% */

		/* type 2 */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_0_0, 0, 0xA); /* factor 2^0 = 100% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_1_0, 1, 0x0); /* factor 2^-1 = 50% */
		byte_raw_mask1_compute(target, data + RGB_SCANTRIPLE_MASK_2_2_0, 2, 0x5); /* factor 2^-2 = 25% */
	}
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_scantriple8_mmx(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy8_mmx(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw64_1_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 8);
		break;
	case 2 :
		internal_rgb_raw64_2_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 8);
		break;
	}
}

static inline void internal_rgb_scantriple16_mmx(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy16_mmx(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw64_1_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 4);
		break;
	case 2 :
		internal_rgb_raw64_2_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 4);
		break;
	}
}

static inline void internal_rgb_scantriple32_mmx(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy32_mmx(dst, src, count);
		break;
	case 1 :
		if (data[RGB_SCANTRIPLE_MASK_1_0_0] == 0)
			internal_rgb_raw64_1_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		else
			internal_rgb_raw64_01_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		break;
	case 2 :
		if (data[RGB_SCANTRIPLE_MASK_2_0_0] == 0)
			internal_rgb_raw64_2_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		else
			internal_rgb_raw64_02_mmx(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		break;
	}
}

#endif

static inline void internal_rgb_scantriple8_def(unsigned line, uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy8_def(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 4);
		break;
	case 2 :
		internal_rgb_raw32_2_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 4);
		break;
	}
}

static inline void internal_rgb_scantriple16_def(unsigned line, uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy16_def(dst, src, count);
		break;
	case 1 :
		internal_rgb_raw32_1_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_1_0_0, count / 2);
		break;
	case 2 :
		internal_rgb_raw32_2_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLE_MASK_2_0_0, count / 2);
		break;
	}
}

static inline void internal_rgb_scantriple32_def(unsigned line, uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	switch (line % 3) {
	case 0 :
		internal_copy32_def(dst, src, count);
		break;
	case 1 :
		if (data[RGB_SCANTRIPLE_MASK_1_0_0] == 0)
			internal_rgb_raw32_1_def(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count);
		else
			internal_rgb_raw32_01_def(dst, src, data + RGB_SCANTRIPLE_MASK_1_0_0, count);
		break;
	case 2 :
		if (data[RGB_SCANTRIPLE_MASK_2_0_0] == 0)
			internal_rgb_raw32_2_def(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count);
		else
			internal_rgb_raw32_02_def(dst, src, data + RGB_SCANTRIPLE_MASK_2_0_0, count);
		break;
	}
}

/***************************************************************************/
/* internal_rgb_scantriplevert */

enum RGB_SCANTRIPLEVERT_MASK {
	RGB_SCANTRIPLEVERT_MASK_0_0_0,
	RGB_SCANTRIPLEVERT_MASK_0_0_1,
	RGB_SCANTRIPLEVERT_MASK_0_0_2,
	RGB_SCANTRIPLEVERT_MASK_0_0_3,
	RGB_SCANTRIPLEVERT_MASK_0_0_4,
	RGB_SCANTRIPLEVERT_MASK_0_0_5,
	RGB_SCANTRIPLEVERT_MASK_0_1_0,
	RGB_SCANTRIPLEVERT_MASK_0_1_1,
	RGB_SCANTRIPLEVERT_MASK_0_1_2,
	RGB_SCANTRIPLEVERT_MASK_0_1_3,
	RGB_SCANTRIPLEVERT_MASK_0_1_4,
	RGB_SCANTRIPLEVERT_MASK_0_1_5,
	RGB_SCANTRIPLEVERT_MASK_0_2_0,
	RGB_SCANTRIPLEVERT_MASK_0_2_1,
	RGB_SCANTRIPLEVERT_MASK_0_2_2,
	RGB_SCANTRIPLEVERT_MASK_0_2_3,
	RGB_SCANTRIPLEVERT_MASK_0_2_4,
	RGB_SCANTRIPLEVERT_MASK_0_2_5,
	RGB_SCANTRIPLEVERT_MASK_MAX
};

static void internal_rgb_scantriplevert_set(const struct video_pipeline_target_struct* target, uint32* data)
{
	if (color_def_type_get(target->color_def) == adv_color_type_rgb) {
		/* type 0 */
		rgb_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, 0, 0x700); /* factor 2^0 = 100% */
		rgb_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_1_0, 1, 0x070); /* factor 2^-1 = 50% */
		rgb_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_2_0, 2, 0x007); /* factor 2^-2 = 25% */
	} else if (color_def_type_get(target->color_def) == adv_color_type_yuy2) {
		/* type 0 */
		byte_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, 0, 0xFAA); /* factor 2^0 = 100% */
		byte_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_1_0, 1, 0x050); /* factor 2^-1 = 50% */
		byte_raw_mask3_compute(target, data + RGB_SCANTRIPLEVERT_MASK_0_2_0, 2, 0x005); /* factor 2^-2 = 25% */
	}
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_scantriplevert8_mmx(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	internal_rgb_raw64x3_012_mmx(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 8);
}

static inline void internal_rgb_scantriplevert16_mmx(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	internal_rgb_raw64x3_012_mmx(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 4);
}

static inline void internal_rgb_scantriplevert32_mmx(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	internal_rgb_raw64x3_012_mmx(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 2);
}

#endif

static inline void internal_rgb_scantriplevert8_def(uint8* dst, const uint8* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32x3_012_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 4);
}

static inline void internal_rgb_scantriplevert16_def(uint16* dst, const uint16* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32x3_012_def((uint32*)dst, (uint32*)src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count / 2);
}

static inline void internal_rgb_scantriplevert32_def(uint32* dst, const uint32* src, const uint32* data, unsigned count)
{
	internal_rgb_raw32x3_012_def(dst, src, data + RGB_SCANTRIPLEVERT_MASK_0_0_0, count);
}

/***************************************************************************/
/* internal_rgb_skipdouble */

static void internal_rgb_skipdouble_set(const struct video_pipeline_target_struct* target)
{
}

#if defined(USE_ASM_INLINE)

static inline void internal_rgb_skipdouble8_mmx(unsigned line, uint8* dst, const uint8* src, unsigned count)
{
	if (line % 2) {
		internal_zero8_mmx(dst, count);
	} else {
		internal_copy8_mmx(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble16_mmx(unsigned line, uint16* dst, const uint16* src, unsigned count)
{
	if (line % 2) {
		internal_zero16_mmx(dst, count);
	} else {
		internal_copy16_mmx(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble32_mmx(unsigned line, uint32* dst, const uint32* src, unsigned count)
{
	if (line % 2) {
		internal_zero32_mmx(dst, count);
	} else {
		internal_copy32_mmx(dst, src, count);
	}
}

#endif

static inline void internal_rgb_skipdouble8_def(unsigned line, uint8* dst, const uint8* src, unsigned count)
{
	if (line % 2) {
		internal_zero8_def(dst, count);
	} else {
		internal_copy8_def(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble16_def(unsigned line, uint16* dst, const uint16* src, unsigned count)
{
	if (line % 2) {
		internal_zero16_def(dst, count);
	} else {
		internal_copy16_def(dst, src, count);
	}
}

static inline void internal_rgb_skipdouble32_def(unsigned line, uint32* dst, const uint32* src, unsigned count)
{
	if (line % 2) {
		internal_zero32_def(dst, count);
	} else {
		internal_copy32_def(dst, src, count);
	}
}

#endif

