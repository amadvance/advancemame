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

#ifndef __IDOUBLE_H
#define __IDOUBLE_H

#include "icommon.h"

/***************************************************************************/
/* internal double */

#if defined(USE_ASM_i586)
static __inline__ void internal_double8_mmx(uint8* dst, uint8* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $3,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"punpcklbw %%mm0, %%mm0\n"
		"punpckhbw %%mm1, %%mm1\n"
		"movq %%mm0,(%1)\n"
		"movq %%mm1,8(%1)\n"
		"addl $8,%0\n"
		"addl $16,%1\n"
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
static __inline__ void internal_double16_mmx(uint16* dst, uint16* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"punpcklwd %%mm0, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"punpckhwd %%mm1, %%mm1\n"
		"movq %%mm1,8(%1)\n"
		"addl $8,%0\n"
		"addl $16,%1\n"
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
static __inline__ void internal_double32_mmx(uint32* dst, uint32* src, unsigned count)
{
	assert_align(((unsigned)src & 0x7)==0 && ((unsigned)dst & 0x7)==0);

	__asm__ __volatile__(
		"shrl $1,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0),%%mm0\n"
		"movq %%mm0,%%mm1\n"
		"punpckldq %%mm0, %%mm0\n"
		"movq %%mm0,(%1)\n"
		"punpckhdq %%mm1, %%mm1\n"
		"movq %%mm1,8(%1)\n"
		"addl $8,%0\n"
		"addl $16,%1\n"
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
static __inline__ void internal_double8_def(uint8* dst, uint8* src, unsigned count)
{
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"shrl $2,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0),%%eax\n"
		"movl %%eax, %%edx\n"
		"bswap %%eax\n"
		"xchgw %%ax,%%dx\n"
		"roll $8, %%eax\n"
		"movl %%eax,(%1)\n"
		"rorl $8, %%edx\n"
		"movl %%edx,4(%1)\n"
		"addl $4,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax", "%edx"
	);
}
#else
static __inline__ void internal_double8_def(uint8* dst, uint8* src, unsigned count) {
	unsigned rest = count % 2;

	count /= 2;
	while (count) {
		P32DER0(dst) = src[0] /* ENDIAN */
			| (unsigned)src[0] << 8
			| (unsigned)src[1] << 16
			| (unsigned)src[1] << 24;
		dst += 4;
		src += 2;
		--count;
	}

	while (rest) {
		dst[0] = src[0];
                dst += 1;
		src += 1;
		--rest;
	}
}
#endif

#if defined(USE_ASM_i586)
static __inline__ void internal_double16_def(uint16* dst, uint16* src, unsigned count)
{
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"shrl $1,%2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0),%%eax\n"
		"movl %%eax, %%edx\n"
		"roll $16, %%eax\n"
		"xchgw %%ax,%%dx\n"
		"movl %%eax,(%1)\n"
		"movl %%edx,4(%1)\n"
		"addl $4,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax", "%edx"
	);
}
#else
static __inline__ void internal_double16_def(uint16* dst, uint16* src, unsigned count) {
	while (count) {
		P32DER0(dst) = src[0] /* ENDIAN */
			| (unsigned)src[0] << 16;
		dst += 2;
		src += 1;
		--count;
	}
}
#endif

#if defined(USE_ASM_i586)
static __inline__ void internal_double32_def(uint32* dst, uint32* src, unsigned count)
{
	assert_align(((unsigned)src & 0x3)==0 && ((unsigned)dst & 0x3)==0);

	__asm__ __volatile__ (
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movl (%0),%%eax\n"
		"movl %%eax,(%1)\n"
		"movl %%eax,4(%1)\n"
		"addl $4,%0\n"
		"addl $8,%1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax", "%edx"
	);
}
#else
static __inline__ void internal_double32_def(uint32* dst, uint32* src, unsigned count) {
	while (count) {
		dst[0] = src[0];
		dst[1] = src[0];
		dst += 2;
		src += 1;
		--count;
	}
}
#endif

#endif
