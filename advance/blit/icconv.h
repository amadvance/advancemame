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

#ifndef __ICONV_H
#define __ICONV_H

#include "icommon.h"

/***************************************************************************/
/* internal conv */

#if defined(USE_ASM_INLINE)
static uint32 bgra8888tobgr332_mask[] = {
	0x000000E0, 0x000000E0, /* r */
	0x0000001C, 0x0000001C, /* g */
	0x00000003, 0x00000003  /* b */
};

static inline void internal_convbgra8888tobgr332_mmx(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrlq $16, %%mm0\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrlq $16, %%mm3\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packuswb %%mm3, %%mm0\n"

		"movq 16(%0), %%mm4\n"
		"movq %%mm4, %%mm1\n"
		"movq %%mm4, %%mm2\n"
		"psrlq $16, %%mm4\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm4\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm4\n"

		"movq 24(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrlq $16, %%mm3\n"
		"psrlq $11, %%mm1\n"
		"psrlq $6, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packuswb %%mm3, %%mm4\n"

		"packuswb %%mm4, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $32, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra8888tobgr332_mask)
		: "cc"
	);

	if (rest) {
		const uint32* src32 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src32[0] >> (8-2)) & 0x03)
				| ((src32[0] >> (16-3-2)) & 0x1C)
				| ((src32[0] >> (24-3-3-2)) & 0xE0);
			++src32;
			++dst8;
			--rest;
		} while (rest);
	}
}
#endif

static inline void internal_convbgra8888tobgr332_def(void* dst, const void* src, unsigned count)
{
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8-2)) & 0x03)
			| ((src32[0] >> (16-3-2)) & 0x1C)
			| ((src32[0] >> (24-3-3-2)) & 0xE0)
			| ((src32[1] << -(8-2-8)) & 0x0300)
			| ((src32[1] >> (16-3-2-8)) & 0x1C00)
			| ((src32[1] >> (24-3-3-2-8)) & 0xE000)
			| ((src32[2] << -(8-2-16)) & 0x030000)
			| ((src32[2] << -(16-3-2-16)) & 0x1C0000)
			| ((src32[2] >> (24-3-3-2-16)) & 0xE00000)
			| ((src32[3] << -(8-2-24)) & 0x03000000)
			| ((src32[3] << -(16-3-2-24)) & 0x1C000000)
			| ((src32[3] << -(24-3-3-2-24)) & 0xE0000000);
#else
		*dst32++ = ((src32[3] >> (8-2)) & 0x03)
			| ((src32[3] >> (16-3-2)) & 0x1C)
			| ((src32[3] >> (24-3-3-2)) & 0xE0)
			| ((src32[2] << -(8-2-8)) & 0x0300)
			| ((src32[2] >> (16-3-2-8)) & 0x1C00)
			| ((src32[2] >> (24-3-3-2-8)) & 0xE000)
			| ((src32[1] << -(8-2-16)) & 0x030000)
			| ((src32[1] << -(16-3-2-16)) & 0x1C0000)
			| ((src32[1] >> (24-3-3-2-16)) & 0xE00000)
			| ((src32[0] << -(8-2-24)) & 0x03000000)
			| ((src32[0] << -(16-3-2-24)) & 0x1C000000)
			| ((src32[0] << -(24-3-3-2-24)) & 0xE0000000);
#endif
		src32 += 4;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static uint32 bgra8888tobgr565_mask[] = {
	0x00F80000, 0x00F80000, /* r << 8 */
	0x000007E0, 0x000007E0, /* g */
	0x0000001F, 0x0000001F  /* b */
};

static inline void internal_convbgra8888tobgr565_mmx(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrld $5, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm4\n"
		"movq %%mm3, %%mm2\n"
		"psrld $5, %%mm4\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm4\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm4\n"

		"packuswb %%mm3, %%mm0\n"
		"packssdw %%mm4, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra8888tobgr565_mask)
		: "cc"
	);

	if (rest) {
		const uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8-5)) & 0x001F)
				| ((src32[0] >> (16-5-6)) & 0x07E0)
				| ((src32[0] >> (24-5-6-5)) & 0xF800);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

static inline void internal_convbgra8888tobgr565_def(void* dst, const void* src, unsigned count)
{
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8-5)) & 0x001F)
			| ((src32[0] >> (16-5-6)) & 0x07E0)
			| ((src32[0] >> (24-5-6-5)) & 0xF800)
			| ((src32[1] << -(8-5-16)) & 0x001F0000)
			| ((src32[1] << -(16-5-6-16)) & 0x07E00000)
			| ((src32[1] << -(24-5-6-5-16)) & 0xF8000000);
#else
		*dst32++ = ((src32[1] >> (8-5)) & 0x001F)
			| ((src32[1] >> (16-5-6)) & 0x07E0)
			| ((src32[1] >> (24-5-6-5)) & 0xF800)
			| ((src32[0] << -(8-5-16)) & 0x001F0000)
			| ((src32[0] << -(16-5-6-16)) & 0x07E00000)
			| ((src32[0] << -(24-5-6-5-16)) & 0xF8000000);
#endif
		src32 += 2;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static uint32 bgra8888tobgra5551_mask[] = {
	0x00007C00, 0x00007C00, /* r */
	0x000003E0, 0x000003E0, /* g */
	0x0000001F, 0x0000001F  /* b */
};

static inline void internal_convbgra8888tobgra5551_mmx(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrld $9, %%mm0\n"
		"psrld $6, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrld $9, %%mm3\n"
		"psrld $6, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packssdw %%mm3, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra8888tobgra5551_mask)
		: "cc"
	);

	if (rest) {
		const uint32* src32 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = ((src32[0] >> (8-5)) & 0x001F)
				| ((src32[0] >> (16-5-5)) & 0x03E0)
				| ((src32[0] >> (24-5-5-5)) & 0x7C00);
			++src32;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

static inline void internal_convbgra8888tobgra5551_def(void* dst, const void* src, unsigned count)
{
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src32[0] >> (8-5)) & 0x001F)
			| ((src32[0] >> (16-5-5)) & 0x03E0)
			| ((src32[0] >> (24-5-5-5)) & 0x7C00)
			| ((src32[1] << -(8-5-16)) & 0x001F0000)
			| ((src32[1] << -(16-5-5-16)) & 0x03E00000)
			| ((src32[1] << -(24-5-5-5-16)) & 0x7C000000);
#else
		*dst32++ = ((src32[1] >> (8-5)) & 0x001F)
			| ((src32[1] >> (16-5-5)) & 0x03E0)
			| ((src32[1] >> (24-5-5-5)) & 0x7C00)
			| ((src32[0] << -(8-5-16)) & 0x001F0000)
			| ((src32[0] << -(16-5-5-16)) & 0x03E00000)
			| ((src32[0] << -(24-5-5-5-16)) & 0x7C000000);
#endif
		src32 += 2;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static uint32 bgra5551tobgr332_mask[] = {
	0x00E000E0, 0x00E000E0, /* r */
	0x001C001C, 0x001C001C, /* g */
	0x00030003, 0x00030003  /* b */
};

static inline void internal_convbgra5551tobgr332_mmx(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 8;

	__asm__ __volatile__(
		"shrl $3, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm5\n"
		"movq 8(%3), %%mm6\n"
		"movq 16(%3), %%mm7\n"
		ASM_JUMP_ALIGN
		"0:\n"

		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"psrld $7, %%mm0\n"
		"psrld $5, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm0\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm0\n"

		"movq 8(%0), %%mm3\n"
		"movq %%mm3, %%mm1\n"
		"movq %%mm3, %%mm2\n"
		"psrld $7, %%mm3\n"
		"psrld $5, %%mm1\n"
		"psrld $3, %%mm2\n"
		"pand %%mm5, %%mm3\n"
		"pand %%mm6, %%mm1\n"
		"pand %%mm7, %%mm2\n"
		"por %%mm2, %%mm1\n"
		"por %%mm1, %%mm3\n"

		"packuswb %%mm3, %%mm0\n"

		"movq %%mm0, (%1)\n"
		"addl $16, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"

		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra5551tobgr332_mask)
		: "cc"
	);

	if (rest) {
		const uint16* src16 = src;
		uint8* dst8 = dst;
		do {
			*dst8 = ((src16[0] >> (5-2)) & 0x03)
				| ((src16[0] >> (10-3-2)) & 0x1C)
				| ((src16[0] >> (15-3-3-2)) & 0xE0);
			++src16;
			++dst8;
			--rest;
		} while (rest);
	}
}
#endif

static inline void internal_convbgra5551tobgr332_def(void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 4;
	while (count) {
#ifdef USE_LSB
		*dst32++ = ((src16[0] >> (5-2)) & 0x03)
			| ((src16[0] >> (10-3-2)) & 0x1C)
			| ((src16[0] >> (15-3-3-2)) & 0xE0)
			| ((src16[1] << -(5-2-8)) & 0x0300)
			| ((src16[1] << -(10-3-2-8)) & 0x1C00)
			| ((src16[1] << -(15-3-3-2-8)) & 0xE000)
			| ((src16[2] << -(5-2-16)) & 0x030000)
			| ((src16[2] << -(10-3-2-16)) & 0x1C0000)
			| ((src16[2] << -(15-3-3-2-16)) & 0xE00000)
			| ((src16[3] << -(5-2-24)) & 0x03000000)
			| ((src16[3] << -(10-3-2-24)) & 0x1C000000)
			| ((src16[3] << -(15-3-3-2-24)) & 0xE0000000);
#else
		*dst32++ = ((src16[3] >> (5-2)) & 0x03)
			| ((src16[3] >> (10-3-2)) & 0x1C)
			| ((src16[3] >> (15-3-3-2)) & 0xE0)
			| ((src16[2] << -(5-2-8)) & 0x0300)
			| ((src16[2] << -(10-3-2-8)) & 0x1C00)
			| ((src16[2] << -(15-3-3-2-8)) & 0xE000)
			| ((src16[1] << -(5-2-16)) & 0x030000)
			| ((src16[1] << -(10-3-2-16)) & 0x1C0000)
			| ((src16[1] << -(15-3-3-2-16)) & 0xE00000)
			| ((src16[0] << -(5-2-24)) & 0x03000000)
			| ((src16[0] << -(10-3-2-24)) & 0x1C000000)
			| ((src16[0] << -(15-3-3-2-24)) & 0xE0000000);
#endif
		src16 += 4;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static uint32 bgra5551tobgr565_mask[] = {
	0xFFC0FFC0, 0xFFC0FFC0, /* rg */
	0x001F001F, 0x001F001F /* b */
};

static inline void internal_convbgra5551tobgr565_mmx(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 4;

	__asm__ __volatile__(
		"shrl $2, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm2\n"
		"movq 8(%3), %%mm3\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movq (%0), %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"pslld $1, %%mm0\n"
		"pand %%mm2, %%mm0\n"
		"pand %%mm3, %%mm1\n"
		"por %%mm1, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $8, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra5551tobgr565_mask)
		: "cc"
	);

	if (rest) {
		const uint16* src16 = src;
		uint16* dst16 = dst;
		do {
			*dst16 = (src16[0] & 0x001F)
				| ((src16[0] << 1) & 0xFFC0);
			++src16;
			++dst16;
			--rest;
		} while (rest);
	}
}
#endif

static inline void internal_convbgra5551tobgr565_def(void* dst, const void* src, unsigned count)
{
	uint32* src32 = (uint32*)src;
	uint32* dst32 = (uint32*)dst;

	count /= 2;
	while (count) {
		*dst32++ = (src32[0] & 0x001F001F)
			| ((src32[0] << 1) & 0xFFC0FFC0);
		src32 += 1;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
static uint32 bgra5551tobgra8888_mask[] = {
	0x000000F8, 0x000000F8, /* r */
	0x0000F800, 0x0000F800, /* g */
	0x00F80000, 0x00F80000 /* b */
};

static inline void internal_convbgra5551tobgra8888_mmx(void* dst, const void* src, unsigned count)
{
	unsigned rest = count % 2;

	__asm__ __volatile__(
		"shrl $1, %2\n"
		"jz 1f\n"
		"movq (%3), %%mm3\n"
		"movq 8(%3), %%mm4\n"
		"movq 16(%3), %%mm5\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movd (%0), %%mm0\n"
		"punpcklwd %%mm0, %%mm0\n"
		"movq %%mm0, %%mm1\n"
		"movq %%mm0, %%mm2\n"
		"pslld $3, %%mm0\n"
		"pslld $6, %%mm1\n"
		"pslld $9, %%mm2\n"
		"pand %%mm3, %%mm0\n"
		"pand %%mm4, %%mm1\n"
		"pand %%mm5, %%mm2\n"
		"por %%mm1, %%mm0\n"
		"por %%mm2, %%mm0\n"
		"movq %%mm0, (%1)\n"
		"addl $4, %0\n"
		"addl $8, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		: "r" (bgra5551tobgra8888_mask)
		: "cc"
	);

	if (rest) {
		const uint16* src16 = src;
		uint32* dst32 = dst;
		do {
			*dst32 = ((src16[0] << 3) & 0x000000F8)
				| ((src16[0] << 6) & 0x0000F800)
				| ((src16[0] << 9) & 0x00F80000);
			++src16;
			++dst32;
			--rest;
		} while (rest);
	}
}
#endif

static inline void internal_convbgra5551tobgra8888_def(void* dst, const void* src, unsigned count)
{
	uint16* src16 = (uint16*)src;
	uint32* dst32 = (uint32*)dst;

	while (count) {
		*dst32++ = ((src16[0] << 3) & 0x000000F8)
			| ((src16[0] << 6) & 0x0000F800)
			| ((src16[0] << 9) & 0x00F80000);
		src16 += 1;
		--count;
	}
}

#if defined(USE_ASM_INLINE)
/*
	Y =  0.299  R + 0.587  G + 0.114  B
	U = -0.1687 R - 0.3313 G + 0.5    B + 128
	V =  0.5    R - 0.4187 G - 0.0813 B + 128

	Y = (76*R + 150*G + 29*B) >> 8
	U = (-43*R - 84*G + 128*B) >> 8 + 128
	V = (128*R - 107*G - 20*B) >> 8 + 128
*/

static uint32 bgra8888toyuy2_coeff[] = {
	/*uuuuyyyy    vvvvyyyy */
	0x0080001d, 0xffec001d, /* b */
	0xffac0096, 0xff950096, /* g */
	0xffd5004c, 0x0080004c, /* r */
	0x80000000, 0x80000000  /* add */
};

static inline void pixel_convbgra8888toyuy2_mmx(void* dst, const void* src0, const void* src1)
{
	__asm__ __volatile__ (

#if 0 /* OSDEF Reference code */
/* Basic single pixel implementation */

		/* mm0 = 0 a 0 r 0 g 0 b */

		/* transpose */
		"movq %%mm2, %%mm1\n"
		"punpcklwd %%mm2, %%mm1\n"
		"punpckhwd %%mm2, %%mm2\n"
		"movq %%mm1, %%mm0\n"
		"punpckldq %%mm2, %%mm2\n"
		"punpckldq %%mm1, %%mm0\n"
		"punpckhdq %%mm1, %%mm1\n"

		/* mm0 = 0 b 0 b 0 b 0 b */
		/* mm1 = 0 g 0 g 0 g 0 g */
		/* mm2 = 0 r 0 r 0 r 0 r */

		/* multiply */
		"pmullw 0(%3), %%mm0\n"
		"pmullw 8(%3), %%mm1\n"
		"pmullw 16(%3), %%mm2\n"

		/* add the component without saturation */
		"paddw %%mm1, %%mm0\n"
		"paddw 24(%3), %%mm2\n"
		"paddw %%mm2, %%mm0\n"

		/* reduce the precision */
		"psrlw $8, %%mm0\n"

		/* mm0 = 0 v 0 y 0 u 0 y */
#endif

/* Fast double pixel implementation */
		"movd (%0), %%mm2\n"
		"movd (%1), %%mm5\n"
		"pxor %%mm0, %%mm0\n"
		"punpcklbw %%mm0, %%mm2\n"
		"movq %%mm2, %%mm1\n"
		"punpcklwd %%mm2, %%mm1\n"
		"punpckhwd %%mm2, %%mm2\n"
		"movq %%mm1, %%mm0\n"
		"punpckldq %%mm2, %%mm2\n"
		"punpckldq %%mm1, %%mm0\n"
		"pmullw 0(%3), %%mm0\n"
		"pxor %%mm3, %%mm3\n"
		"punpckhdq %%mm1, %%mm1\n"
		"punpcklbw %%mm3, %%mm5\n"
		"pmullw 8(%3), %%mm1\n"
		"movq %%mm5, %%mm4\n"
		"punpcklwd %%mm5, %%mm4\n"
		"punpckhwd %%mm5, %%mm5\n"
		"pmullw 16(%3), %%mm2\n"
		"movq %%mm4, %%mm3\n"
		"punpckldq %%mm5, %%mm5\n"
		"punpckldq %%mm4, %%mm3\n"
		"punpckhdq %%mm4, %%mm4\n"
		"pmullw 0(%3), %%mm3\n"
		"paddw %%mm1, %%mm0\n"
		"paddw 24(%3), %%mm2\n"
		"pmullw 8(%3), %%mm4\n"
		"paddw %%mm2, %%mm0\n"
		"pmullw 16(%3), %%mm5\n"
		"psrlw $8, %%mm0\n"
		"paddw %%mm4, %%mm3\n"
		"paddw 24(%3), %%mm5\n"
		"paddw %%mm5, %%mm3\n"
		"psrlw $8, %%mm3\n"
		"packuswb %%mm3, %%mm0\n"
		"movq %%mm0, (%2)\n"

		:
		: "r" (src0), "r" (src1), "r" (dst), "r" (bgra8888toyuy2_coeff)
		: "cc", "memory"
	);
}
#endif

static inline void pixel_convbgra8888toyuy2_def(void* dst, const void* src)
{
	const uint8* src8 = (const uint8*)src;
	uint8* dst8 = (uint8*)dst;
	int r, g, b;
	int y, u, v;

#ifdef USE_LSB
	b = src8[0];
	g = src8[1];
	r = src8[2];
#else
	b = src8[3];
	g = src8[2];
	r = src8[1];
#endif

/*
      Y =  0.299  R + 0.587  G + 0.114  B
      U = -0.1687 R - 0.3313 G + 0.5    B + 128
      V =  0.5    R - 0.4187 G - 0.0813 B + 128
*/
	y = ((19595*r + 38469*g + 7471*b) >> 16);
	u = ((-11055*r - 21712*g + 32768*b) >> 16) + 128;
	v = ((32768*r - 27439*g - 5328*b) >> 16) + 128;

	dst8[0] = y;
	dst8[1] = u;
	dst8[2] = y;
	dst8[3] = v;
}

static inline void pixel_alphabgra8888_def(void* dst, const void* src)
{
	int a;
	uint8* src8 = (uint8*)src;
	uint8* dst8 = (uint8*)dst;

#ifdef USE_LSB
	a = src8[3];
	if (a == 0) {
		/* nothing */
	} else if (a == 255) {
		uint32* src32 = (uint32*)src;
		uint32* dst32 = (uint32*)dst;
		*dst32 = *src32;
	} else {
		dst8[0] += (src8[0] - dst8[0]) * a / 256;
		dst8[1] += (src8[1] - dst8[1]) * a / 256;
		dst8[2] += (src8[2] - dst8[2]) * a / 256;
	}
#else
	a = src8[0];
	if (a == 0) {
		/* nothing */
	} else if (a == 255) {
		uint32* src32 = (uint32*)src;
		uint32* dst32 = (uint32*)dst;
		*dst32 = *src32;
	} else {
		dst8[1] += (src8[1] - dst8[1]) * a / 256;
		dst8[2] += (src8[2] - dst8[2]) * a / 256;
		dst8[3] += (src8[3] - dst8[3]) * a / 256;
	}
#endif
}

static uint32 alphabgra8888_coeff[] = {
	0x00FF00FF, 0x00FF00FF
};

static inline void pixel_alphabgra8888_mmx(void* dst, const void* src)
{
	uint8* src8 = (uint8*)src;

	if (src8[3] == 0) {
		/* nothing */
	} else if (src8[3] == 255) {
		uint32* src32 = (uint32*)src;
		uint32* dst32 = (uint32*)dst;
		*dst32 = *src32;
	} else {
		__asm__ __volatile__ (
			"movq (%2), %%mm3\n" /* mm3 = 0F0F0F0F */

			"movd (%0), %%mm1\n" /* mm1 = 0000ARGB (src) */
			"punpcklbw %%mm1, %%mm1\n" /* mm1 = AARRGGBB */
			"pand %%mm3, %%mm1\n" /* mm1 = 0A0R0G0B */
			"movd (%1), %%mm2\n" /* mm2 = 0000ARGB (dst) */
			"punpcklbw %%mm2, %%mm2\n" /* mm2 = AARRGGBB */
			"pand %%mm3, %%mm2\n" /* mm2 = 0A0R0G0B */

			"movq %%mm1, %%mm4\n" /* mm4 = 0A0R0G0B */
			"psrlq $48, %%mm4\n" /* mm4 = 0000000A */
			"punpcklwd %%mm4, %%mm4\n" /* mm4 = 00000A0A */
			"punpcklwd %%mm4, %%mm4\n" /* mm4 = 0A0A0A0A */

			"psubw %%mm2, %%mm1\n" /* mm1 = src - dst */
			"pmullw %%mm4, %%mm1\n" /* mm1 = (alpha) * (src - dst) */
			"psrlw $8, %%mm1\n" /* mm1 = (alpha) * (src - dst) / 256 */
			"paddw %%mm1, %%mm2\n" /* mm2 = dst + (alpha) * (src - dst) / 256 */
			"pand %%mm3, %%mm2\n" /* mm2 = 0A0R0G0B */

			"packuswb %%mm2, %%mm2\n" /* mm2 = ARGBARGB */
			"movd %%mm2, (%1)\n"

			:
			: "r" (src), "r" (dst), "r" (alphabgra8888_coeff)
			: "cc", "memory"
		);
	}
}

#endif

