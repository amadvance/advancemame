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

#ifndef __ICOPY_H
#define __ICOPY_H

#include "icommon.h"

/***************************************************************************/
/* internal copy */

#if defined(USE_ASM_i586)
static __inline__ void internal_copy8_mmx(uint8* dst, uint8* src, unsigned count)
{
	unsigned rest = count % 8;

	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0),%%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%0\n"
		"addl $8,%1\n"
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

#if defined(USE_ASM_i586)
static uint8 copy8_mask[8] = { 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 };

static __inline__ void internal_copy8_step2_mmx(uint8* dst, uint8* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		"movq (%3),%%mm2\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0),%%mm0\n"
		"movq 8(%0),%%mm1\n"
		"pand %%mm2,%%mm0\n"
		"pand %%mm2,%%mm1\n"
		"packuswb %%mm1,%%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $16,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (copy8_mask)
		: "cc"
	);
}
#endif

#if defined(USE_ASM_i586)
static __inline__ void internal_copy8_def(uint8* dst, uint8* src, unsigned count)
{
	unsigned rest = count % 4;

	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0),%%eax\n"
		"movl %%eax,(%1)\n"
		"addl $4,%0\n"
		"addl $4,%1\n"
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
static __inline__ void internal_copy8_def(uint8* dst, uint8* src, unsigned count)
{
	memcpy(dst,src,count);
}
#endif

static __inline__ void internal_copy8_step2_def(uint8* dst, uint8* src, unsigned count)
{
	unsigned rest = count % 4;

	count /= 4;
	while (count) {
		P32DER0(dst) = src[0] /* ENDIAN */
			| (unsigned)src[2] << 8
			| (unsigned)src[4] << 16
			| (unsigned)src[6] << 24;
		dst += 4;
		src += 8;
		--count;
	}

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		src += 2;
		--rest;
	}
}

#if defined(USE_ASM_i586)
static __inline__ void internal_copy16_mmx(uint16* dst, uint16* src, unsigned count) {
	internal_copy8_mmx((uint8*)dst, (uint8*)src, 2*count);
}
#endif

static __inline__ void internal_copy16_def(uint16* dst, uint16* src, unsigned count) {
	internal_copy8_def((uint8*)dst, (uint8*)src, 2*count);
}

#if defined(USE_ASM_i586)
static __inline__ void internal_copy32_mmx(uint32* dst, uint32* src, unsigned count) {
	internal_copy8_mmx((uint8*)dst, (uint8*)src, 4*count);
}
#endif

static __inline__ void internal_copy32_def(uint32* dst, uint32* src, unsigned count) {
	internal_copy8_def((uint8*)dst, (uint8*)src, 4*count);
}

#if defined(USE_ASM_i586)
static __inline__ void internal_copy8_step_mmx(uint8* dst, uint8* src, unsigned count, int step1) {
	assert_align(((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzbl (%0),%%eax\n"
		"movzbl (%0,%3),%%edx\n"
		"movd %%eax,%%mm0\n"
		"movd %%edx,%%mm1\n"
		"addl %3,%0\n"
		"addl %3,%0\n"
		"punpcklbw %%mm1, %%mm0\n"

		"movzbl (%0),%%eax\n"
		"movzbl (%0,%3),%%edx\n"
		"movd %%eax,%%mm2\n"
		"movd %%edx,%%mm3\n"
		"addl %3,%0\n"
		"addl %3,%0\n"
		"punpcklbw %%mm3, %%mm2\n"
		"punpcklwd %%mm2, %%mm0\n"

		"movzbl (%0),%%eax\n"
		"movzbl (%0,%3),%%edx\n"
		"movd %%eax,%%mm4\n"
		"movd %%edx,%%mm5\n"
		"addl %3,%0\n"
		"addl %3,%0\n"
		"punpcklbw %%mm5, %%mm4\n"

		"movzbl (%0),%%eax\n"
		"movzbl (%0,%3),%%edx\n"
		"movd %%eax,%%mm6\n"
		"movd %%edx,%%mm7\n"
		"addl %3,%0\n"
		"addl %3,%0\n"
		"punpcklbw %%mm7, %%mm6\n"
		"punpcklwd %%mm6, %%mm4\n"

		"punpckldq %%mm4, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (step1)
		: "cc"
	);
}
#endif

static __inline__ void internal_copy8_step_def(uint8* dst, uint8* src, unsigned count, int step1)
{
	unsigned rest = count % 4;

	int step2 = step1 + step1;
	int step3 = step2 + step1;
	int step4 = step3 + step1;

	count /= 4;
	while (count) {
		P32DER0(dst) = src[0] /* ENDIAN */
			| (unsigned)src[step1] << 8
			| (unsigned)src[step2] << 16
			| (unsigned)src[step3] << 24;
		dst += 4;
		src += step4;
		--count;
	}

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		src += step1;
		--rest;
	}
}

#if defined(USE_ASM_i586)
static __inline__ void internal_copy16_step_mmx(uint16* dst, uint16* src, unsigned count, int step1) {
	assert_align(((unsigned)src & 0x1)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%3),%%edx\n"
		"movd %%eax,%%mm0\n"
		"addl %3,%0\n"
		"movd %%edx,%%mm1\n"
		"addl %3,%0\n"
		"punpcklwd %%mm1, %%mm0\n"
		"movzwl (%0),%%eax\n"
		"movzwl (%0,%3),%%edx\n"
		"movd %%eax,%%mm2\n"
		"addl %3,%0\n"
		"movd %%edx,%%mm3\n"
		"addl %3,%0\n"
		"punpcklwd %%mm3, %%mm2\n"
		"punpckldq %%mm2, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (step1)
		: "cc", "%eax", "%edx"
	);
}
#endif

static __inline__ void internal_copy16_step_def(uint16* dst, uint16* src, unsigned count, int step1)
{
	unsigned rest = count % 2;

	int step2 = step1 + step1;

	count /= 2;
	while (count) {
		P32DER0(dst) = P16DER0(src) /* ENDIAN */
			| (unsigned)P16DER(src,step1) << 16;
		dst += 2;
		PADD(src,step2);
		--count;
	}

	while (rest) {
		dst[0] = src[0];
		dst += 1;
		PADD(src,step1);
		--rest;
	}
}

static __inline__ void internal_copy32_step3(uint32* dst, uint32* src, unsigned count)
{
	uint8* dst8 = (uint8*)dst;
	uint8* src8 = (uint8*)src;

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

#if defined(USE_ASM_i586)
static __inline__ void internal_copy32_step_mmx(uint32* dst, uint32* src, unsigned count, int step1) {
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movd (%0),%%mm0\n"
		"movd (%0,%3),%%mm1\n"
		"addl %3,%0\n"
		"addl %3,%0\n"
		"punpckldq %%mm1, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "b" (step1)
		: "cc"
	);
}
#endif

static __inline__ void internal_copy32_step_def(uint32* dst, uint32* src, unsigned count, int step1)
{
	while (count) {
		dst[0] = src[0];
		dst += 1;
		PADD(src,step1);
		--count;
	}
}

/***************************************************************************/
/* internal fill */

/* Set optimized for small counts > 0 */
static __inline__ void internal_fill8(uint8* dst, unsigned src, unsigned count) {
	do {
		*dst++ = src;
		--count;
	} while (count);
}

/* Set optimized for small counts > 0 */
static __inline__ void internal_fill16(uint16* dst, unsigned src, unsigned count) {
	do {
		*dst++ = src;
		--count;
	} while (count);
}

/* Set optimized for small counts > 0 */
static __inline__ void internal_fill32(uint32* dst, unsigned src, unsigned count) {
	do {
		*dst++ = src;
		--count;
	} while (count);
}

#endif
