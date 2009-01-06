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

#ifndef __IDOUBLE_H
#define __IDOUBLE_H

#include "icommon.h"

/***************************************************************************/
/* internal double */

#if defined(USE_ASM_INLINE)
static inline void internal_double8_mmx(uint8* dst, const uint8* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"punpcklbw %%mm0, %%mm0\n"
		"punpckhbw %%mm1, %%mm1\n"
		"movq %%mm0, (%1)\n"
		"movq %%mm1, 8(%1)\n"
		"addl $8, %0\n"
		"addl $16, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc"
	);
}
#endif

#if defined(USE_ASM_INLINE)
static inline void internal_double16_mmx(uint16* dst, const uint16* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"punpcklwd %%mm0, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"punpckhwd %%mm1, %%mm1\n"
		"movq %%mm1, 8(%1)\n"
		"addl $8, %0\n"
		"addl $16, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc"
	);
}
#endif

#if defined(USE_ASM_INLINE)
static inline void internal_double32_mmx(uint32* dst, const uint32* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"punpckldq %%mm0, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"punpckhdq %%mm1, %%mm1\n"
		"movq %%mm1, 8(%1)\n"
		"addl $8, %0\n"
		"addl $16, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc"
	);
}
#endif

#if defined(USE_ASM_INLINE)
static inline void internal_double8_def(uint8* dst, const uint8* src, unsigned count)
{
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0), %%eax\n"
		"movl %%eax, %%edx\n"
		"bswap %%eax\n"
		"xchgw %%ax, %%dx\n"
		"roll $8, %%eax\n"
		"movl %%eax, (%1)\n"
		"rorl $8, %%edx\n"
		"movl %%edx, 4(%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax", "%edx"
	);
}
#else
static inline void internal_double8_def(uint8* restrict dst, const uint8* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}
#endif

static inline void internal_double8_step_def(uint8* restrict dst, const uint8* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple8_def(uint8* restrict dst, const uint8* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		src += 1;
		--count;
	}
}

static inline void internal_triple8_step_def(uint8* restrict dst, const uint8* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple8_def(uint8* restrict dst, const uint8* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		src += 1;
		--count;
	}
}

static inline void internal_quadruple8_step_def(uint8* restrict dst, const uint8* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		PADD(src, step);
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static inline void internal_double16_def(uint16* dst, const uint16* src, unsigned count)
{
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"shrl $1, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0), %%eax\n"
		"movl %%eax, %%edx\n"
		"roll $16, %%eax\n"
		"xchgw %%ax, %%dx\n"
		"movl %%eax, (%1)\n"
		"movl %%edx, 4(%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax", "%edx"
	);
}
#else
static inline void internal_double16_def(uint16* restrict dst, const uint16* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}
#endif

static inline void internal_double16_step_def(uint16* restrict dst, const uint16* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple16_def(uint16* restrict dst, const uint16* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		src += 1;
		--count;
	}
}

static inline void internal_triple16_step_def(uint16* restrict dst, const uint16* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple16_def(uint16* restrict dst, const uint16* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		src += 1;
		--count;
	}
}

static inline void internal_quadruple16_step_def(uint16* restrict dst, const uint16* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		PADD(src, step);
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static inline void internal_double32_def(uint32* dst, const uint32* src, unsigned count)
{
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0), %%eax\n"
		"movl %%eax, (%1)\n"
		"movl %%eax, 4(%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax", "%edx"
	);
}
#else
static inline void internal_double32_def(uint32* restrict dst, const uint32* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}
#endif

static inline void internal_double32_step_def(uint32* restrict dst, const uint32* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple32_def(uint32* restrict dst, const uint32* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		src += 1;
		--count;
	}
}

static inline void internal_triple32_step_def(uint32* restrict dst, const uint32* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst += 3;
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple32_def(uint32* restrict dst, const uint32* restrict src, unsigned count)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		src += 1;
		--count;
	}
}

static inline void internal_quadruple32_step_def(uint32* restrict dst, const uint32* restrict src, unsigned count, int step)
{
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst[2] = src[0];
		dst[3] = src[0];
		dst += 4;
		PADD(src, step);
		--count;
	}
}


#endif

