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

#ifndef __IUNCHAIN_H
#define __IUNCHAIN_H

#include "icommon.h"

/***************************************************************************/
/* internal_unchained */

#if defined(USE_ASM_i586)
static inline void internal_unchained8(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 4;

	__asm__ __volatile__ (
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movb 12(%0), %%al\n"
		"movb 8(%0), %%ah\n"
		"bswap %%eax\n"
		"movb (%0), %%al\n"
		"movb 4(%0), %%ah\n"
		"movl %%eax, (%1)\n"
		"addl $16, %0\n"
		"addl $4, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax"
	);

	switch (rest) {
		case 3 : dst[2] = src[8];
		case 2 : dst[1] = src[4];
		case 1 : dst[0] = src[0];
	}
}
#else
static inline void internal_unchained8(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 4;

	count /= 4;
	while (count) {
		P32DER0(dst) = src[0] /* ENDIAN */
			| (unsigned)src[4] << 8
			| (unsigned)src[8] << 16
			| (unsigned)src[12] << 24;
		dst += 4;
		src += 16;
		--count;
	}

	switch (rest) {
		case 3 : dst[2] = src[8];
		case 2 : dst[1] = src[4];
		case 1 : dst[0] = src[0];
	}
}
#endif

#if defined(USE_ASM_i586)
static inline void internal_unchained8_step2(uint8* dst, const uint8* src, unsigned count)
{
	__asm__ __volatile__ (
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movb 24(%0), %%al\n"
		"movb 16(%0), %%ah\n"
		"bswap %%eax\n"
		"movb (%0), %%al\n"
		"movb 8(%0), %%ah\n"
		"movl %%eax, (%1)\n"
		"addl $32, %0\n"
		"addl $4, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "%eax"
	);
}
#else
static inline void internal_unchained8_step2(uint8* dst, const uint8* src, unsigned count)
{
	unsigned rest = count % 4;

	count /= 4;
	while (count) {
		P32DER0(dst) = src[0] /* ENDIAN */
			| (unsigned)src[8] << 8
			| (unsigned)src[16] << 16
			| (unsigned)src[24] << 24;
		dst += 4;
		src += 32;
		--count;
	}

	switch (rest) {
		case 3 : dst[2] = src[16];
		case 2 : dst[1] = src[8];
		case 1 : dst[0] = src[0];
	}
}
#endif

#if defined(USE_ASM_i586)
static inline void internal_unchained8_double(uint8* dst, const uint8* src, unsigned count)
{
	__asm__ __volatile__ (
		"shrl $2, %2\n"
		"jz 1f\n"
		ASM_JUMP_ALIGN
		"0:\n"
		"movb 6(%0), %%al\n"
		"movb 4(%0), %%ah\n"
		"bswap %%eax\n"
		"movb (%0), %%al\n"
		"movb 2(%0), %%ah\n"
		"movl %%eax, (%1)\n"
		"addl $8, %0\n"
		"addl $4, %1\n"
		"decl %2\n"
		"jnz 0b\n"
		"1:\n"
		: "+S" (src), "+D" (dst), "+c" (count)
		:
		: "cc", "eax"
	);
}
#else
static inline void internal_unchained8_double(uint8* dst, const uint8* src, unsigned count)
{
	/* TODO */
}
#endif

static inline void internal_unchained8_step(uint8* dst, const uint8* src, unsigned count, int step)
{
	unsigned rest = count % 4;

	int step1 = 4 * step; /* 4 is for unchained mode */
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

	switch (rest) {
		case 3 : dst[2] = src[step2];
		case 2 : dst[1] = src[step1];
		case 1 : dst[0] = src[0];
	}
}

static inline void internal_unchained8_double_step2(uint8* dst, const uint8* src, unsigned count)
{
	internal_unchained8(dst, src, count);
}

static inline void internal_unchained8_double_step(uint8* dst, const uint8* src, unsigned count, int step)
{
	unsigned rest = count % 4;

	int step1 = 2 * step; /* 2 is for unchained mode with double */
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

	switch (rest) {
		case 3 : dst[2] = src[step2];
		case 2 : dst[1] = src[step1];
		case 1 : dst[0] = src[0];
	}
}

static inline void internal_unchained8_palette16to8(uint8* dst, const uint16* src, unsigned count, unsigned* palette)
{
	unsigned rest = count % 4;

	count /= 4;
	while (count) {
		P32DER0(dst) = palette[src[0]] /* ENDIAN */
			| palette[src[4]] << 8
			| palette[src[8]] << 16
			| palette[src[12]] << 24;
		dst += 4;
		src += 16;
		--count;
	}

	switch (rest) {
		case 3 : dst[2] = palette[src[8]];
		case 2 : dst[1] = palette[src[4]];
		case 1 : dst[0] = palette[src[0]];
	}
}

static inline void internal_unchained8_double_palette16to8(uint8* dst, const uint16* src, unsigned count, unsigned* palette)
{
	unsigned rest = count % 4;

	count /= 4;
	while (count) {
		P32DER0(dst) = palette[src[0]] /* ENDIAN */
			| palette[src[2]] << 8
			| palette[src[4]] << 16
			| palette[src[6]] << 24;
		dst += 4;
		src += 8;
		--count;
	}

	switch (rest) {
		case 3 : dst[2] = palette[src[4]];
		case 2 : dst[1] = palette[src[2]];
		case 1 : dst[0] = palette[src[0]];
	}
}

#endif
