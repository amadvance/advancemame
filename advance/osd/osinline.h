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

#ifndef __OSINLINE_H
#define __OSINLINE_H

#include "osd_cpu.h"

#include <assert.h>

#ifdef USE_ASM_INLINE

extern int the_blit_mmx; /* defined in blit.c, !=0 if MMX is available */

/* Fast multiplication, return a*b/2^32 */
#define vec_mult osd_vec_mult
static inline int osd_vec_mult(int a, int b)
{
	int r;
	__asm__ __volatile__ (
		"movl %1, %0\n"
		"imull %2\n"
		"movl %%edx, %%eax\n"
		: "=&a" (r)
		: "mr" (a), "mr" (b)
		:  "%edx", "cc"
	);
	return r;
}

/* TileMap support functions : MMX implementation */

/* Conversion table from 8 bit to 64 bit. */
__extension__ extern unsigned long long mmx_8to64_map[256];

#define pdo16 osd_pdo16
static void osd_pdo16(UINT16* cpy_dst, const UINT16* cpy_src, int count, UINT8* or_dst, UINT32 mixcode)
{
	unsigned code = mixcode & 0xFF;
	unsigned count8 = count / 8;

	if (the_blit_mmx && count8) {
		count = count % 8;

		__asm__ __volatile__(
			"movq (%4), %%mm3\n"

			".p2align 4\n"
			"0:\n"

			"movq (%0), %%mm0\n"
			"movq (%2), %%mm1\n"
			"movq 8(%2), %%mm2\n"
			"por %%mm3, %%mm0\n"
			"movq %%mm1, (%1)\n"
			"movq %%mm2, 8(%1)\n"
			"movq %%mm0, (%0)\n"

			"addl $8, %0\n"
			"addl $16, %1\n"
			"addl $16, %2\n"

			"decl %3\n"
			"jnz 0b\n"

			: "+r" (or_dst), "+r" (cpy_dst), "+r" (cpy_src)
			: "r" (count8), "r" (mmx_8to64_map + code)
			: "cc"
		);
	}

	while (count) {
		*or_dst |= code;
		*cpy_dst = *cpy_src;

		++or_dst;
		++cpy_dst;
		++cpy_src;
		--count;
	}
}

#define pdo16pal osd_pdo16pal
static void osd_pdo16pal(UINT16* cpy_dst, const UINT16* cpy_src, int count, UINT8* or_dst, UINT32 mixcode)
{
	unsigned pal = (mixcode >> 16) | (mixcode & 0xFFFF0000);
	unsigned code = mixcode & 0xFF;
	unsigned count8 = count / 8;

	if (the_blit_mmx && count8) {
		count = count % 8;

		__asm__ __volatile__(
			"movd %0, %%mm5\n"
			"movq (%1), %%mm7\n"
			"movq %%mm5, %%mm6\n"
			"psllq $32, %%mm5\n"
			"por %%mm5, %%mm6\n"
			:
			: "r" (pal), "r" (mmx_8to64_map + code)
		);

		__asm__ __volatile__(
			".p2align 4\n"
			"0:\n"

			"movq (%0), %%mm0\n"
			"movq (%2), %%mm1\n"
			"movq 8(%2), %%mm2\n"
			"paddw %%mm6, %%mm1\n"
			"paddw %%mm6, %%mm2\n"
			"por %%mm7, %%mm0\n"
			"movq %%mm1, (%1)\n"
			"movq %%mm2, 8(%1)\n"
			"movq %%mm0, (%0)\n"

			"addl $8, %0\n"
			"addl $16, %1\n"
			"addl $16, %2\n"

			"decl %3\n"
			"jnz 0b\n"

			: "+r" (or_dst), "+r" (cpy_dst), "+r" (cpy_src)
			: "r" (count8)
			: "cc"
		);
	}

	while (count) {
		*or_dst |= code;
		*cpy_dst = *cpy_src + pal;

		++or_dst;
		++cpy_dst;
		++cpy_src;
		--count;
	}
}

#define pdo16np osd_pdo16np
static void osd_pdo16np(UINT16* cpy_dst, const UINT16* cpy_src, int count, UINT8* or_dst, UINT32 mixcode)
{
	unsigned code = mixcode & 0xFF;
	unsigned count4 = count / 4;

	if (the_blit_mmx && count4) {
		count = count % 4;

		__asm__ __volatile__(
			".p2align 4\n"
			"0:\n"

			"movq (%1), %%mm1\n"
			"movq %%mm1, (%0)\n"

			"addl $8, %0\n"
			"addl $8, %1\n"

			"decl %2\n"
			"jnz 0b\n"

			: "+r" (cpy_dst), "+r" (cpy_src)
			: "r" (count4)
			: "cc"
		);
	}

	while (count) {
		*cpy_dst = *cpy_src;
		++cpy_dst;
		++cpy_src;
		--count;
	}
}

#define pdt16 osd_pdt16
static void osd_pdt16(UINT16* cpy_dst, const UINT16* cpy_src, const UINT8* mask_src, int mask, int value, int count, UINT8* or_dst, UINT32 mixcode)
{
	unsigned code = mixcode & 0xFF;
	unsigned count8 = count / 8;

	if (the_blit_mmx && count8) {
		count = count % 8;

		__asm__ __volatile__(
			"movq (%0), %%mm5\n"
			"movq (%1), %%mm6\n"
			"movq (%2), %%mm7\n"
			:
			: "r" (mmx_8to64_map + code), "r" (mmx_8to64_map + mask), "r" (mmx_8to64_map + value)
		);

		__asm__ __volatile__(
			".p2align 4\n"
			"0:\n"

			/* Original version without instruction shuffling */
			/*
			"movq (%1), %%mm0\n"
			"pand %%mm6, %%mm0\n"
			"pcmpeqb %%mm7, %%mm0\n"
			"movq %%mm0, %%mm1\n"

			"movq (%2), %%mm2\n"
			"movq (%3), %%mm3\n"
			"punpcklbw %%mm1, %%mm1\n"
			"pand %%mm1, %%mm3\n"
			"pandn %%mm2, %%mm1\n"
			"por %%mm3, %%mm1\n"
			"movq %%mm1, (%2)\n"

			"movq %%mm0, %%mm4\n"
			"movq 8(%2), %%mm2\n"
			"movq 8(%3), %%mm3\n"
			"punpckhbw %%mm4, %%mm4\n"
			"pand %%mm4, %%mm3\n"
			"pandn %%mm2, %%mm4\n"
			"por %%mm3, %%mm4\n"
			"movq %%mm4, 8(%2)\n"

			"pand %%mm5, %%mm0\n"
			"por (%0), %%mm0\n"
			"movq %%mm0, (%0)\n"
			*/

			"movq (%1), %%mm0\n"
			"pand %%mm6, %%mm0\n"
			"pcmpeqb %%mm7, %%mm0\n"
			"movq %%mm0, %%mm1\n"
			"movq (%3), %%mm3\n"
			"punpcklbw %%mm1, %%mm1\n"
			"movq (%2), %%mm2\n"
			"pand %%mm1, %%mm3\n"
			"movq %%mm0, %%mm4\n"
			"pandn %%mm2, %%mm1\n"
			"por %%mm3, %%mm1\n"
			"movq %%mm1, (%2)\n"
			"movq 8(%3), %%mm3\n"
			"punpckhbw %%mm4, %%mm4\n"
			"movq 8(%2), %%mm2\n"
			"pand %%mm4, %%mm3\n"
			"pandn %%mm2, %%mm4\n"
			"pand %%mm5, %%mm0\n"
			"por %%mm3, %%mm4\n"
			"por (%0), %%mm0\n"
			"movq %%mm4, 8(%2)\n"
			"movq %%mm0, (%0)\n"

			"addl $8, %0\n"
			"addl $8, %1\n"
			"addl $16, %2\n"
			"addl $16, %3\n"

			"decl %4\n"
			"jnz 0b\n"

			: "+r" (or_dst), "+r" (mask_src), "+r" (cpy_dst), "+r" (cpy_src)
			: "r" (count8)
			: "cc"
		);
	}

	while (count) {
		if ((*mask_src & mask) == value) {
			*or_dst |= code;
			*cpy_dst = *cpy_src;
		}

		++or_dst;
		++mask_src;
		++cpy_dst;
		++cpy_src;
		--count;
	}
}

#define pdt16pal osd_pdt16pal
static void osd_pdt16pal(UINT16* cpy_dst, const UINT16* cpy_src, const UINT8* mask_src, int mask, int value, int count, UINT8* or_dst, UINT32 mixcode)
{
	unsigned pal = (mixcode >> 16) | (mixcode & 0xFFFF0000);
	unsigned code = mixcode & 0xFF;

	unsigned count8 = count / 8;

	if (the_blit_mmx && count8) {
		count = count % 8;

		__asm__ __volatile__(
			"movd %0, %%mm4\n"
			"movq (%1), %%mm6\n"
			"movq %%mm4, %%mm5\n"
			"movq (%2), %%mm7\n"
			"psllq $32, %%mm4\n"
			"por %%mm4, %%mm5\n"
			:
			: "r" (pal), "r" (mmx_8to64_map + mask), "r" (mmx_8to64_map + value)
		);

		__asm__ __volatile__(
			".p2align 4\n"
			"0:\n"

			"movq (%1), %%mm0\n"
			"pand %%mm6, %%mm0\n"
			"pcmpeqb %%mm7, %%mm0\n"
			"movq %%mm0, %%mm1\n"
			"movq (%3), %%mm3\n"
			"punpcklbw %%mm1, %%mm1\n"
			"paddw %%mm5, %%mm3\n"
			"movq (%2), %%mm2\n"
			"pand %%mm1, %%mm3\n"
			"movq %%mm0, %%mm4\n"
			"pandn %%mm2, %%mm1\n"
			"por %%mm3, %%mm1\n"
			"movq %%mm1, (%2)\n"
			"movq 8(%3), %%mm3\n"
			"punpckhbw %%mm4, %%mm4\n"
			"paddw %%mm5, %%mm3\n"
			"movq 8(%2), %%mm2\n"
			"pand %%mm4, %%mm3\n"
			"pandn %%mm2, %%mm4\n"
			"pand (%5), %%mm0\n"
			"por %%mm3, %%mm4\n"
			"por (%0), %%mm0\n"
			"movq %%mm4, 8(%2)\n"
			"movq %%mm0, (%0)\n"

			"addl $8, %0\n"
			"addl $8, %1\n"
			"addl $16, %2\n"
			"addl $16, %3\n"

			"decl %4\n"
			"jnz 0b\n"

			: "+r" (or_dst), "+r" (mask_src), "+r" (cpy_dst), "+r" (cpy_src)
			: "r" (count8), "r" (mmx_8to64_map + code)
			: "cc"
		);
	}

	while (count) {
		if ((*mask_src & mask) == value) {
			*or_dst |= code;
			*cpy_dst = *cpy_src + pal;
		}

		++or_dst;
		++mask_src;
		++cpy_dst;
		++cpy_src;
		--count;
	}
}

#define pdt16np osd_pdt16np
static void osd_pdt16np(UINT16* cpy_dst, const UINT16* cpy_src, const UINT8* mask_src, int mask, int value, int count, UINT8* or_dst, UINT32 mixcode)
{
	unsigned count8 = count / 8;

	if (the_blit_mmx && count8) {
		__asm__ __volatile__(
			"movq (%0), %%mm6\n"
			"movq (%1), %%mm7\n"
			:
			: "r" (mmx_8to64_map + mask), "r" (mmx_8to64_map + value)
		);

		__asm__ __volatile__(
			".p2align 4\n"
			"0:\n"

			"movq (%0), %%mm0\n"
			"pand %%mm6, %%mm0\n"
			"pcmpeqb %%mm7, %%mm0\n"
			"movq %%mm0, %%mm1\n"
			"movq (%2), %%mm3\n"
			"punpcklbw %%mm1, %%mm1\n"
			"movq (%1), %%mm2\n"
			"pand %%mm1, %%mm3\n"
			"movq %%mm0, %%mm4\n"
			"pandn %%mm2, %%mm1\n"
			"por %%mm3, %%mm1\n"
			"movq %%mm1, (%1)\n"
			"movq 8(%2), %%mm3\n"
			"punpckhbw %%mm4, %%mm4\n"
			"movq 8(%1), %%mm2\n"
			"pand %%mm4, %%mm3\n"
			"pandn %%mm2, %%mm4\n"
			"por %%mm3, %%mm4\n"
			"movq %%mm4, 8(%1)\n"

			"addl $8, %0\n"
			"addl $16, %1\n"
			"addl $16, %2\n"

			"decl %3\n"
			"jnz 0b\n"

			: "+r" (mask_src), "+r" (cpy_dst), "+r" (cpy_src), "+r" (count8)
			:
			: "cc"
		);

		count = count % 8;
	}

	while (count) {
		if ((*mask_src & mask) == value) {
			*cpy_dst = *cpy_src;
		}

		++mask_src;
		++cpy_dst;
		++cpy_src;
		--count;
	}
}

#define osd_pend osd_pend
static inline void osd_pend(void)
{
	if (the_blit_mmx) {
		__asm__ __volatile__ (
			"emms"
		);
	}
}

#endif

#endif
