/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#if defined(USE_ASM_i586)
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

#if defined(USE_ASM_i586)
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

#if defined(USE_ASM_i586)
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

#if defined(USE_ASM_i586)
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
static inline void internal_double8_def(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 2;

	count /= 2;
	while (count) {
		unsigned p0 = P8DER(src, 0);
		unsigned p1 = P8DER(src, 1);
		P32DER0(dst) = cpu_uint32_make_uint8(p0, p0, p1, p1);
		PADD(dst, 4);
		PADD(src, 2);
		--count;
	}

	if (rest) {
		dst[0] = src[0];
		dst[1] = src[0];
	}
}
#endif

static inline void internal_double8_step_def(uint8* dst, const uint8* src, unsigned count, int step)
{
	unsigned rest = count % 2;

	count /= 2;
	while (count) {
		unsigned p0, p1;
		p0 = P8DER0(src);
		PADD(src, step);
		p1 = P8DER0(src);
		PADD(src, step);
		P32DER0(dst) = cpu_uint32_make_uint8(p0, p0, p1, p1);
		PADD(dst, 4);
		PADD(src, 2);
		--count;
	}

	if (rest) {
		dst[0] = src[0];
		dst[1] = src[0];
	}
}

static inline void internal_triple8_def(uint8* dst, const uint8* src, unsigned count)
{
	count /= 4;
	while (count) {
		unsigned p0 = P8DER(src, 0);
		unsigned p1 = P8DER(src, 1);
		unsigned p2 = P8DER(src, 2);
		unsigned p3 = P8DER(src, 3);
		P32DER(dst, 0) = cpu_uint32_make_uint8(p0, p0, p0, p1);
		P32DER(dst, 4) = cpu_uint32_make_uint8(p1, p1, p2, p2);
		P32DER(dst, 8) = cpu_uint32_make_uint8(p2, p3, p3, p3);
		PADD(dst, 12);
		PADD(src, 4);
		--count;
	}

	/* TODO REST */
}

static inline void internal_triple8_step_def(uint8* dst, const uint8* src, unsigned count, int step)
{
	count /= 4;
	while (count) {
		unsigned p0, p1, p2, p3;
		p0 = P8DER0(src);
		PADD(src, step);
		p1 = P8DER0(src);
		PADD(src, step);
		p2 = P8DER0(src);
		PADD(src, step);
		p3 = P8DER0(src);
		PADD(src, step);
		P32DER(dst, 0) = cpu_uint32_make_uint8(p0, p0, p0, p1);
		P32DER(dst, 4) = cpu_uint32_make_uint8(p1, p1, p2, p2);
		P32DER(dst, 8) = cpu_uint32_make_uint8(p2, p3, p3, p3);
		PADD(dst, 12);
		--count;
	}

	/* TODO REST */
}

static inline void internal_quadruple8_def(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		unsigned p0 = P8DER0(src);
		P32DER0(dst) = cpu_uint32_make_uint8(p0, p0, p0, p0);
		PADD(dst, 4);
		PADD(src, 1);
		--count;
	}
}

static inline void internal_quadruple8_step_def(uint8* dst, const uint8* src, unsigned count, int step)
{
	while (count) {
		unsigned p0 = P8DER0(src);
		P32DER0(dst) = cpu_uint32_make_uint8(p0, p0, p0, p0);
		PADD(dst, 4);
		PADD(src, step);
		--count;
	}
}

#if defined(USE_ASM_i586)
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
static inline void internal_double16_def(uint16* dst, const uint16* src, unsigned count)
{
	while (count) {
		unsigned p0 = P16DER0(src);
		P32DER0(dst) = cpu_uint32_make_uint16(p0, p0);
		PADD(dst, 4);
		PADD(src, 2);
		--count;
	}
}
#endif

static inline void internal_double16_step_def(uint16* dst, const uint16* src, unsigned count, int step)
{
	while (count) {
		unsigned p0 = P16DER0(src);
		P32DER0(dst) = cpu_uint32_make_uint16(p0, p0);
		PADD(dst, 4);
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple16_def(uint16* dst, const uint16* src, unsigned count)
{
	count /= 2;
	while (count) {
		unsigned p0 = P16DER(src, 0);
		unsigned p1 = P16DER(src, 2);
		P32DER(dst, 0) = cpu_uint32_make_uint16(p0, p0);
		P32DER(dst, 4) = cpu_uint32_make_uint16(p0, p1);
		P32DER(dst, 8) = cpu_uint32_make_uint16(p1, p1);
		PADD(dst, 12);
		PADD(src, 4);
		--count;
	}
	/* TODO REST */
}

static inline void internal_triple16_step_def(uint16* dst, const uint16* src, unsigned count, int step)
{
	count /= 2;
	while (count) {
		unsigned p0, p1;
		p0 = P16DER0(src);
		PADD(src, step);
		p1 = P16DER0(src);
		PADD(src, step);
		P32DER(dst, 0) = cpu_uint32_make_uint16(p0, p0);
		P32DER(dst, 4) = cpu_uint32_make_uint16(p0, p1);
		P32DER(dst, 8) = cpu_uint32_make_uint16(p1, p1);
		PADD(dst, 12);
		--count;
	}
	/* TODO REST */
}

static inline void internal_quadruple16_def(uint16* dst, const uint16* src, unsigned count)
{
	while (count) {
		unsigned p0 = P16DER0(src);
		P32DER(dst, 0) = cpu_uint32_make_uint16(p0, p0);
		P32DER(dst, 4) = cpu_uint32_make_uint16(p0, p0);
		PADD(dst, 8);
		PADD(src, 2);
		--count;
	}
}

static inline void internal_quadruple16_step_def(uint16* dst, const uint16* src, unsigned count, int step)
{
	while (count) {
		unsigned p0 = P16DER0(src);
		P32DER(dst, 0) = cpu_uint32_make_uint16(p0, p0);
		P32DER(dst, 4) = cpu_uint32_make_uint16(p0, p0);
		PADD(dst, 8);
		PADD(src, step);
		--count;
	}
}

#if defined(USE_ASM_i586)
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
static inline void internal_double32_def(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		unsigned p0 = P32DER0(src);
		P32DER(dst, 0) = p0;
		P32DER(dst, 4) = p0;
		PADD(dst, 8);
		PADD(src, 4);
		--count;
	}
}
#endif

static inline void internal_double32_step_def(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count) {
		unsigned p0 = P32DER0(src);
		P32DER(dst, 0) = p0;
		P32DER(dst, 4) = p0;
		PADD(dst, 8);
		PADD(src, step);
		--count;
	}
}

static inline void internal_triple32_def(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		unsigned p0 = P32DER0(src);
		P32DER(dst, 0) = p0;
		P32DER(dst, 4) = p0;
		P32DER(dst, 8) = p0;
		PADD(dst, 12);
		PADD(src, 4);
		--count;
	}
}

static inline void internal_triple32_step_def(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count) {
		unsigned p0 = P32DER0(src);
		P32DER(dst, 0) = p0;
		P32DER(dst, 4) = p0;
		P32DER(dst, 8) = p0;
		PADD(dst, 12);
		PADD(src, step);
		--count;
	}
}

static inline void internal_quadruple32_def(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		unsigned p0 = P32DER0(src);
		P32DER(dst, 0) = p0;
		P32DER(dst, 4) = p0;
		P32DER(dst, 8) = p0;
		P32DER(dst, 12) = p0;
		PADD(dst, 16);
		PADD(src, 4);
		--count;
	}
}

static inline void internal_quadruple32_step_def(uint32* dst, const uint32* src, unsigned count, int step)
{
	while (count) {
		unsigned p0 = P32DER0(src);
		P32DER(dst, 0) = p0;
		P32DER(dst, 4) = p0;
		P32DER(dst, 8) = p0;
		P32DER(dst, 12) = p0;
		PADD(dst, 16);
		PADD(src, step);
		--count;
	}
}


#endif
