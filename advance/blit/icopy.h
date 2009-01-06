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

#ifndef __ICOPY_H
#define __ICOPY_H

#include "icommon.h"

/***************************************************************************/
/* internal copy */

#if defined(USE_ASM_INLINE)
static inline void internal_copy8_mmx(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 8;

	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc"
	);

	/* Note: the (count) register is marked "+" instead of "" to */
	/* inform the compiler that this register is modified and */
	/* it can't be used to compute the "rest" value */

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		src += 1;
		--rest;
	}
}
#endif

#if defined(USE_ASM_INLINE)
static uint8 copy8_mask[8] = { 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

static inline void internal_copy8_step2_mmx(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 8;

	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm2\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq 8(%0), %%mm1\n"
		"pand %%mm2, %%mm0\n"
		"pand %%mm2, %%mm1\n"
		"packuswb %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (copy8_mask)
		: "cc"
	);

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		src += 2;
		--rest;
	}
}
#endif

#if defined(USE_ASM_INLINE)
static inline void internal_copy8_def(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 4;

	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0), %%eax\n"
		"movl %%eax, (%1)\n"
		"addl $4, %0\n"
		"addl $4, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax"
	);

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		src += 1;
		--rest;
	}
}
#else
static inline void internal_copy8_def(uint8* dst, const uint8* src, unsigned count)
{
	memcpy(dst, src, count);
}
#endif

static inline void internal_copy8_step2_def(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		src += 2;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static inline void internal_copy16_mmx(uint16* dst, const uint16* src, unsigned count)
{
	internal_copy8_mmx((uint8*)dst, (uint8*)src, 2*count);
}
#endif

static inline void internal_copy16_def(uint16* dst, const uint16* src, unsigned count)
{
	internal_copy8_def((uint8*)dst, (uint8*)src, 2*count);
}

#if defined(USE_ASM_INLINE)
static inline void internal_copy32_mmx(uint32* dst, const uint32* src, unsigned count)
{
	internal_copy8_mmx((uint8*)dst, (uint8*)src, 4*count);
}
#endif

static inline void internal_copy32_def(uint32* dst, const uint32* src, unsigned count)
{
	internal_copy8_def((uint8*)dst, (uint8*)src, 4*count);
}

#if defined(USE_ASM_INLINE)
static inline void internal_copy8_step_mmx(uint8* dst, const uint8* src, unsigned count, int step)
{
	unsigned rest = count % 8;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzbl (%0), %%eax\n"
		"movzbl (%0, %3), %%edx\n"
		"movd %%eax, %%mm0\n"
		"movd %%edx, %%mm1\n"
		"addl %3, %0\n"
		"addl %3, %0\n"
		"punpcklbw %%mm1, %%mm0\n"

		"movzbl (%0), %%eax\n"
		"movzbl (%0, %3), %%edx\n"
		"movd %%eax, %%mm2\n"
		"movd %%edx, %%mm3\n"
		"addl %3, %0\n"
		"addl %3, %0\n"
		"punpcklbw %%mm3, %%mm2\n"
		"punpcklwd %%mm2, %%mm0\n"

		"movzbl (%0), %%eax\n"
		"movzbl (%0, %3), %%edx\n"
		"movd %%eax, %%mm4\n"
		"movd %%edx, %%mm5\n"
		"addl %3, %0\n"
		"addl %3, %0\n"
		"punpcklbw %%mm5, %%mm4\n"

		"movzbl (%0), %%eax\n"
		"movzbl (%0, %3), %%edx\n"
		"movd %%eax, %%mm6\n"
		"movd %%edx, %%mm7\n"
		"addl %3, %0\n"
		"addl %3, %0\n"
		"punpcklbw %%mm7, %%mm6\n"
		"punpcklwd %%mm6, %%mm4\n"

		"punpckldq %%mm4, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (step)
		: "cc"
	);

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--rest;
	}
}
#endif

static inline void internal_copy8_step_def(uint8* dst, const uint8* src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static inline void internal_copy16_step_mmx(uint16* dst, const uint16* src, unsigned count, int step)
{
	unsigned rest = count % 4;

	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %3), %%edx\n"
		"movd %%eax, %%mm0\n"
		"addl %3, %0\n"
		"movd %%edx, %%mm1\n"
		"addl %3, %0\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzwl (%0), %%eax\n"
		"movzwl (%0, %3), %%edx\n"
		"movd %%eax, %%mm2\n"
		"addl %3, %0\n"
		"movd %%edx, %%mm3\n"
		"addl %3, %0\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (step)
		: "cc", "%eax", "%edx"
	);

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--rest;
	}
}
#endif

static inline void internal_copy16_step_def(uint16* dst, const uint16* src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static inline void internal_copy32_step_mmx(uint32* dst, const uint32* src, unsigned count, int step)
{
	unsigned rest = count % 2;

	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movd (%0), %%mm0\n"
		"movd (%0, %3), %%mm1\n"
		"addl %3, %0\n"
		"addl %3, %0\n"
		"punpckldq %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (step)
		: "cc"
	);

	if (rest) {
		dst[0] = src[0];
	}
}
#endif

static inline void internal_copy32_step_def(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src, step);
		--count;
	}
}

/***************************************************************************/
/* internal fill */

/* Set optimized for small counts > 0 */
static inline void internal_fill8(uint8* dst, unsigned src, unsigned count)
{
	assert(count > 0);

	do {
		*dst++ = src;
		--count;
	} while (count);
}

/* Set optimized for small counts > 0 */
static inline void internal_fill16(uint16* dst, unsigned src, unsigned count)
{
	assert(count > 0);

	do {
		*dst++ = src;
		--count;
	} while (count);
}

/* Set optimized for small counts > 0 */
static inline void internal_fill32(uint32* dst, unsigned src, unsigned count)
{
	assert(count > 0);

	do {
		*dst++ = src;
		--count;
	} while (count);
}

/***************************************************************************/
/* internal zero */

#if defined(USE_ASM_INLINE)
static inline void internal_zero8_mmx(uint8* dst, unsigned count)
{
	unsigned rest = count % 8;

	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"xorq %%mm0, %%mm0\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq %%mm0, (%0)\n"
		"addl $8, %0\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+D" (dst), "+c" (count)
		:
		: "cc"
	);

	while (rest) {
		dst[0] = 0;
		dst += 1;
		--rest;
	}
}
#endif

static inline void internal_zero8_def(uint8* dst, unsigned count)
{
	memset(dst, 0, count);
}

#if defined(USE_ASM_INLINE)
static inline void internal_zero16_mmx(uint16* dst, unsigned count)
{
	internal_zero8_mmx((uint8*)dst, 2*count);
}
#endif

static inline void internal_zero16_def(uint16* dst, unsigned count)
{
	internal_zero8_def((uint8*)dst, 2*count);
}

#if defined(USE_ASM_INLINE)
static inline void internal_zero32_mmx(uint32* dst, unsigned count)
{
	internal_zero8_mmx((uint8*)dst, 4*count);
}
#endif

static inline void internal_zero32_def(uint32* dst, unsigned count)
{
	internal_zero8_def((uint8*)dst, 4*count);
}

#endif

