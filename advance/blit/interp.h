/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2004, 2008 Andrea Mazzoleni
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

#ifndef __INTERP_H
#define __INTERP_H

/***************************************************************************/
/* Basic types */

typedef unsigned short interp_uint16;
typedef unsigned short interp_uint16_565;
typedef unsigned short interp_uint16_555;
typedef unsigned interp_uint32;
typedef unsigned interp_uintyuy2;

/***************************************************************************/
/* interpolation */

/** Interpolation types */
#define INTERP_16 0 /* RGB 16 bits */
#define INTERP_32 1 /* RGB 32 bits */
#define INTERP_YUY2 2 /* YUY2 32 bits */
#define INTERP_NONE 3 /* None */

extern unsigned interp_mask[2];
extern unsigned interp_red_mask, interp_green_mask, interp_blue_mask;
extern int interp_red_shift, interp_green_shift, interp_blue_shift;
extern unsigned interp_near_mask;
extern unsigned interp_highnot_mask;

#define interp_is_565 (interp_green_mask == 0x7E0)

/**
 * Select which method to use for computing pixels.
 */
#define USE_INTERP_MASK_3

#define INTERP_16_MASK_1(v) ((v) & interp_mask[0])
#define INTERP_16_MASK_2(v) ((v) & interp_mask[1])
#define INTERP_16_UNMASK_1(v) ((v) & interp_mask[0])
#define INTERP_16_UNMASK_2(v) ((v) & interp_mask[1])
#define INTERP_16_HNMASK interp_highnot_mask

#define INTERP_16_GEN2(a, b) \
	static inline interp_uint16 interp_16_ ## a ## b(interp_uint16 p1, interp_uint16 p2) \
	{ \
		return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * a + INTERP_16_MASK_1(p2) * b) / 16) \
		       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * a + INTERP_16_MASK_2(p2) * b) / 16); \
	}

#define INTERP_16_GEN3(a, b, c) \
	static inline interp_uint16 interp_16_ ## a ## b ## c(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3) \
	{ \
		return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * a + INTERP_16_MASK_1(p2) * b + INTERP_16_MASK_1(p3) * c) / 16) \
		       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * a + INTERP_16_MASK_2(p2) * b + INTERP_16_MASK_2(p3) * c) / 16); \
	}

#define INTERP_32_MASK_1(v) ((v) & 0xFF00FFU)
#define INTERP_32_MASK_2(v) ((v) & 0x00FF00U)
#define INTERP_32_UNMASK_1(v) ((v) & 0xFF00FFU)
#define INTERP_32_UNMASK_2(v) ((v) & 0x00FF00U)
#define INTERP_32_HNMASK (~0x808080U)

#define INTERP_32_GEN2(a, b) \
	static inline interp_uint32 interp_32_ ## a ## b(interp_uint32 p1, interp_uint32 p2) \
	{ \
		return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * a + INTERP_32_MASK_1(p2) * b) / 16) \
		       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * a + INTERP_32_MASK_2(p2) * b) / 16); \
	}

#define INTERP_32_GEN3(a, b, c) \
	static inline interp_uint32 interp_32_ ## a ## b ## c(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3) \
	{ \
		return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * a + INTERP_32_MASK_1(p2) * b + INTERP_32_MASK_1(p3) * c) / 16) \
		       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * a + INTERP_32_MASK_2(p2) * b + INTERP_32_MASK_2(p3) * c) / 16); \
	}

#define INTERP_YUY2_MASK_1(v) ((v) & 0xFF00FFU)
#define INTERP_YUY2_MASK_2(v) (((v) & 0xFF00FF00U) >> 8)
#define INTERP_YUY2_UNMASK_1(v) ((v) & 0xFF00FFU)
#define INTERP_YUY2_UNMASK_2(v) (((v) << 8) & 0xFF00FF00U)
#define INTERP_YUY2_HNMASK (~0x80808080U)

#define INTERP_YUY2_GEN2(a, b) \
	static inline interp_uint32 interp_yuy2_ ## a ## b(interp_uint32 p1, interp_uint32 p2) \
	{ \
		return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * a + INTERP_YUY2_MASK_1(p2) * b) / 16) \
		       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * a + INTERP_YUY2_MASK_2(p2) * b) / 16); \
	}

#define INTERP_YUY2_GEN3(a, b, c) \
	static inline interp_uint32 interp_yuy2_ ## a ## b ## c(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3) \
	{ \
		return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * a + INTERP_YUY2_MASK_1(p2) * b + INTERP_YUY2_MASK_1(p3) * c) / 16) \
		       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * a + INTERP_YUY2_MASK_2(p2) * b + INTERP_YUY2_MASK_2(p3) * c) / 16); \
	}

static inline interp_uint16 interp_16_11(interp_uint16 p1, interp_uint16 p2)
{
	/*
	 * This function compute (a + b) / 2 for any rgb nibble, using the
	 * the formula (a + b) / 2 = ((a ^ b) >> 1) + (a & b).
	 * To extend this formula to a serie of packed nibbles the formula is
	 * implemented as (((v0 ^ v1) >> 1) & MASK) + (v0 & v1) where MASK
	 * is used to clear the high bit of all the packed nibbles.
	 */
	return (((p1 ^ p2) >> 1) & INTERP_16_HNMASK) + (p1 & p2);
}

static inline interp_uint16 interp_16_211(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return interp_16_11(p1, interp_16_11(p2, p3));
}

static inline interp_uint16 interp_16_31(interp_uint16 p1, interp_uint16 p2)
{
	return interp_16_11(p1, interp_16_11(p1, p2));
}

static inline interp_uint16 interp_16_521(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * 5 + INTERP_16_MASK_1(p2) * 2 + INTERP_16_MASK_1(p3)) / 8)
	       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * 5 + INTERP_16_MASK_2(p2) * 2 + INTERP_16_MASK_2(p3)) / 8);
#else
	return interp_16_11(p1, interp_16_11(p2, interp_16_11(p1, p3)));
#endif
}

static inline interp_uint16 interp_16_431(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * 4 + INTERP_16_MASK_1(p2) * 3 + INTERP_16_MASK_1(p3)) / 8)
	       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * 4 + INTERP_16_MASK_2(p2) * 3 + INTERP_16_MASK_2(p3)) / 8);
#else
	return interp_16_11(p1, interp_16_11(p2, interp_16_11(p2, p3)));
#endif
}

static inline interp_uint16 interp_16_53(interp_uint16 p1, interp_uint16 p2)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * 5 + INTERP_16_MASK_1(p2) * 3) / 8)
	       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * 5 + INTERP_16_MASK_2(p2) * 3) / 8);
#else
	return interp_16_11(p1, interp_16_11(p2, interp_16_11(p1, p2)));
#endif
}

static inline interp_uint16 interp_16_332(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * 3 + INTERP_16_MASK_1(p2) * 3 + INTERP_16_MASK_1(p3) * 2) / 8)
	       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * 3 + INTERP_16_MASK_2(p2) * 3 + INTERP_16_MASK_2(p3) * 2) / 8);
#else
	interp_uint16 t = interp_16_11(p1, p2);
	return interp_16_11(t, interp_16_11(p3, t));
#endif
}

static inline interp_uint16 interp_16_611(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * 6 + INTERP_16_MASK_1(p2) + INTERP_16_MASK_1(p3)) / 8)
	       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * 6 + INTERP_16_MASK_2(p2) + INTERP_16_MASK_2(p3)) / 8);
#else
	return interp_16_11(p1, interp_16_11(p1, interp_16_11(p2, p3)));
#endif
}

static inline interp_uint16 interp_16_71(interp_uint16 p1, interp_uint16 p2)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) * 7 + INTERP_16_MASK_1(p2)) / 8)
	       | INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) * 7 + INTERP_16_MASK_2(p2)) / 8);
#else
	return interp_16_11(p1, interp_16_11(p1, interp_16_11(p1, p2)));
#endif
}

INTERP_16_GEN3(6, 5, 5)
INTERP_16_GEN3(7, 5, 4)
INTERP_16_GEN3(7, 6, 3)
INTERP_16_GEN3(7, 7, 2)
INTERP_16_GEN3(8, 5, 3)
INTERP_16_GEN3(9, 4, 3)
INTERP_16_GEN3(9, 6, 1)
INTERP_16_GEN3(10, 3, 3)
INTERP_16_GEN3(10, 5, 1)
INTERP_16_GEN3(11, 3, 2)
INTERP_16_GEN2(11, 5)
INTERP_16_GEN3(12, 3, 1)
INTERP_16_GEN2(13, 3)
INTERP_16_GEN3(14, 1, 1)
INTERP_16_GEN2(15, 1)
INTERP_16_GEN2(9, 7)

#define INTERP_16_SPECIALIZE(fmt, mask1, mask2, hnmask) \
	static inline interp_uint16 interp_16_ ## fmt ## _11(interp_uint16 p1, interp_uint16 p2) \
	{ \
		return (((p1 ^ p2) >> 1) & (hnmask)) + (p1 & p2); \
	} \
	static inline interp_uint16 interp_16_ ## fmt ## _31(interp_uint16 p1, interp_uint16 p2) \
	{ \
		return interp_16_ ## fmt ## _11(p1, interp_16_ ## fmt ## _11(p1, p2)); \
	} \
	static inline interp_uint16 interp_16_ ## fmt ## _71(interp_uint16 p1, interp_uint16 p2) \
	{ \
		return (((((p1 & (mask1)) * 7 + (p2 & (mask1))) >> 3) & (mask1))) \
		       | (((((p1 & (mask2)) * 7 + (p2 & (mask2))) >> 3) & (mask2))); \
	}

INTERP_16_SPECIALIZE(565, 0xF81F, 0x07E0, 0x7BEF)
INTERP_16_SPECIALIZE(555, 0x7C1F, 0x03E0, 0x3DEF)

static inline interp_uint32 interp_32_11(interp_uint32 p1, interp_uint32 p2)
{
	/*
	 * This function compute (a + b) / 2 for any rgb nibble, using the
	 * the formula (a + b) / 2 = ((a ^ b) >> 1) + (a & b).
	 * To extend this formula to a serie of packed nibbles the formula is
	 * implemented as (((v0 ^ v1) >> 1) & MASK) + (v0 & v1) where MASK
	 * is used to clear the high bit of all the packed nibbles.
	 */
	return (((p1 ^ p2) >> 1) & INTERP_32_HNMASK) + (p1 & p2);
}

static inline interp_uint32 interp_32_211(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return interp_32_11(p1, interp_32_11(p2, p3));
}

static inline interp_uint32 interp_32_31(interp_uint32 p1, interp_uint32 p2)
{
	return interp_32_11(p1, interp_32_11(p1, p2));
}

static inline interp_uint32 interp_32_521(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * 5 + INTERP_32_MASK_1(p2) * 2 + INTERP_32_MASK_1(p3)) / 8)
	       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * 5 + INTERP_32_MASK_2(p2) * 2 + INTERP_32_MASK_2(p3)) / 8);
#else
	return interp_32_11(p1, interp_32_11(p2, interp_32_11(p1, p3)));
#endif
}

static inline interp_uint32 interp_32_431(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * 4 + INTERP_32_MASK_1(p2) * 3 + INTERP_32_MASK_1(p3)) / 8)
	       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * 4 + INTERP_32_MASK_2(p2) * 3 + INTERP_32_MASK_2(p3)) / 8);
#else
	return interp_32_11(p1, interp_32_11(p2, interp_32_11(p2, p3)));
#endif
}

static inline interp_uint32 interp_32_53(interp_uint32 p1, interp_uint32 p2)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * 5 + INTERP_32_MASK_1(p2) * 3) / 8)
	       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * 5 + INTERP_32_MASK_2(p2) * 3) / 8);
#else
	return interp_32_11(p1, interp_32_11(p2, interp_32_11(p1, p2)));
#endif
}

static inline interp_uint32 interp_32_332(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * 3 + INTERP_32_MASK_1(p2) * 3 + INTERP_32_MASK_1(p3) * 2) / 8)
	       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * 3 + INTERP_32_MASK_2(p2) * 3 + INTERP_32_MASK_2(p3) * 2) / 8);
#else
	interp_uint32 t = interp_32_11(p1, p2);
	return interp_32_11(t, interp_32_11(p3, t));
#endif
}

static inline interp_uint32 interp_32_611(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * 6 + INTERP_32_MASK_1(p2) + INTERP_32_MASK_1(p3)) / 8)
	       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * 6 + INTERP_32_MASK_2(p2) + INTERP_32_MASK_2(p3)) / 8);
#else
	return interp_32_11(p1, interp_32_11(p1, interp_32_11(p2, p3)));
#endif
}

static inline interp_uint32 interp_32_71(interp_uint32 p1, interp_uint32 p2)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) * 7 + INTERP_32_MASK_1(p2)) / 8)
	       | INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) * 7 + INTERP_32_MASK_2(p2)) / 8);
#else
	return interp_32_11(p1, interp_32_11(p1, interp_32_11(p1, p2)));
#endif
}

INTERP_32_GEN3(6, 5, 5)
INTERP_32_GEN3(7, 5, 4)
INTERP_32_GEN3(7, 6, 3)
INTERP_32_GEN3(7, 7, 2)
INTERP_32_GEN3(8, 5, 3)
INTERP_32_GEN3(9, 4, 3)
INTERP_32_GEN3(9, 6, 1)
INTERP_32_GEN3(10, 3, 3)
INTERP_32_GEN3(10, 5, 1)
INTERP_32_GEN3(11, 3, 2)
INTERP_32_GEN2(11, 5)
INTERP_32_GEN3(12, 3, 1)
INTERP_32_GEN2(13, 3)
INTERP_32_GEN3(14, 1, 1)
INTERP_32_GEN2(15, 1)
INTERP_32_GEN2(9, 7)

static inline interp_uint32 interp_yuy2_11(interp_uint32 p1, interp_uint32 p2)
{
	/*
	 * This function compute (a + b) / 2 for any rgb nibble, using the
	 * the formula (a + b) / 2 = ((a ^ b) >> 1) + (a & b).
	 * To extend this formula to a serie of packed nibbles the formula is
	 * implemented as (((v0 ^ v1) >> 1) & MASK) + (v0 & v1) where MASK
	 * is used to clear the high bit of all the packed nibbles.
	 */
	return (((p1 ^ p2) >> 1) & INTERP_YUY2_HNMASK) + (p1 & p2);
}

static inline interp_uint32 interp_yuy2_211(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return interp_yuy2_11(p1, interp_yuy2_11(p2, p3));
}

static inline interp_uint32 interp_yuy2_31(interp_uint32 p1, interp_uint32 p2)
{
	return interp_yuy2_11(p1, interp_yuy2_11(p1, p2));
}

static inline interp_uint32 interp_yuy2_521(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * 5 + INTERP_YUY2_MASK_1(p2) * 2 + INTERP_YUY2_MASK_1(p3)) / 8)
	       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * 5 + INTERP_YUY2_MASK_2(p2) * 2 + INTERP_YUY2_MASK_2(p3)) / 8);
#else
	return interp_yuy2_11(p1, interp_yuy2_11(p2, interp_yuy2_11(p1, p3)));
#endif
}

static inline interp_uint32 interp_yuy2_431(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * 4 + INTERP_YUY2_MASK_1(p2) * 3 + INTERP_YUY2_MASK_1(p3)) / 8)
	       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * 4 + INTERP_YUY2_MASK_2(p2) * 3 + INTERP_YUY2_MASK_2(p3)) / 8);
#else
	return interp_yuy2_11(p1, interp_yuy2_11(p2, interp_yuy2_11(p2, p3)));
#endif
}

static inline interp_uint32 interp_yuy2_53(interp_uint32 p1, interp_uint32 p2)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * 5 + INTERP_YUY2_MASK_1(p2) * 3) / 8)
	       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * 5 + INTERP_YUY2_MASK_2(p2) * 3) / 8);
#else
	return interp_yuy2_11(p1, interp_yuy2_11(p2, interp_yuy2_11(p1, p2)));
#endif
}

static inline interp_uint32 interp_yuy2_332(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * 3 + INTERP_YUY2_MASK_1(p2) * 3 + INTERP_YUY2_MASK_1(p3) * 2) / 8)
	       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * 3 + INTERP_YUY2_MASK_2(p2) * 3 + INTERP_YUY2_MASK_2(p3) * 2) / 8);
#else
	interp_uint32 t = interp_yuy2_11(p1, p2);
	return interp_yuy2_11(t, interp_yuy2_11(p3, t));
#endif
}

static inline interp_uint32 interp_yuy2_611(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * 6 + INTERP_YUY2_MASK_1(p2) + INTERP_YUY2_MASK_1(p3)) / 8)
	       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * 6 + INTERP_YUY2_MASK_2(p2) + INTERP_YUY2_MASK_2(p3)) / 8);
#else
	return interp_yuy2_11(p1, interp_yuy2_11(p1, interp_yuy2_11(p2, p3)));
#endif
}

static inline interp_uint32 interp_yuy2_71(interp_uint32 p1, interp_uint32 p2)
{
#ifdef USE_INTERP_MASK_3
	return INTERP_YUY2_UNMASK_1((INTERP_YUY2_MASK_1(p1) * 7 + INTERP_YUY2_MASK_1(p2)) / 8)
	       | INTERP_YUY2_UNMASK_2((INTERP_YUY2_MASK_2(p1) * 7 + INTERP_YUY2_MASK_2(p2)) / 8);
#else
	return interp_yuy2_11(p1, interp_yuy2_11(p1, interp_yuy2_11(p1, p2)));
#endif
}

INTERP_YUY2_GEN3(6, 5, 5)
INTERP_YUY2_GEN3(7, 5, 4)
INTERP_YUY2_GEN3(7, 6, 3)
INTERP_YUY2_GEN3(7, 7, 2)
INTERP_YUY2_GEN3(8, 5, 3)
INTERP_YUY2_GEN3(9, 4, 3)
INTERP_YUY2_GEN3(9, 6, 1)
INTERP_YUY2_GEN3(10, 3, 3)
INTERP_YUY2_GEN3(10, 5, 1)
INTERP_YUY2_GEN3(11, 3, 2)
INTERP_YUY2_GEN2(11, 5)
INTERP_YUY2_GEN3(12, 3, 1)
INTERP_YUY2_GEN2(13, 3)
INTERP_YUY2_GEN3(14, 1, 1)
INTERP_YUY2_GEN2(15, 1)
INTERP_YUY2_GEN2(9, 7)

/**
 * Compares two pixels and return if different.
 * Used by HQ/LQ algorithm.
 */
int interp_16_diff(interp_uint16 p1, interp_uint16 p2);
int interp_32_diff(interp_uint32 p1, interp_uint32 p2);
int interp_yuy2_diff(interp_uint32 p1, interp_uint32 p2);

/**
 * Computes the distance between two pixels.
 * Used by XBR algorithm.
 * This is a very rought approximation taking into account only the luminosity.
 * See: http://www.compuphase.com/cmetric.htm
 */
static inline int interp_16_dist(interp_uint16 p1, interp_uint16 p2)
{
	int r, g, b, gw;

	b = (p1 & 31) - (p2 & 31);

	if (interp_is_565) {
		/* RGB565 */
		r = (p1 >> 11) - (p2 >> 11);
		g = ((p1 >> 5) & 63) - ((p2 >> 5) & 63);
		gw = 3;
	} else {
		/* RGB555 */
		r = ((p1 >> 10) & 31) - ((p2 >> 10) & 31);
		g = ((p1 >> 5) & 31) - ((p2 >> 5) & 31);
		gw = 6;
	}

	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (b < 0) b = -b;

	return (3 * r + gw * g + b) * 8;
}

static inline int interp_16_565_dist(interp_uint16 p1, interp_uint16 p2)
{
	int r = (p1 >> 11) - (p2 >> 11);
	int g = ((p1 >> 5) & 63) - ((p2 >> 5) & 63);
	int b = (p1 & 31) - (p2 & 31);

	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (b < 0) b = -b;

	return (3 * (r + g) + b) * 8;
}

static inline int interp_16_555_dist(interp_uint16 p1, interp_uint16 p2)
{
	int r = ((p1 >> 10) & 31) - ((p2 >> 10) & 31);
	int g = ((p1 >> 5) & 31) - ((p2 >> 5) & 31);
	int b = (p1 & 31) - (p2 & 31);

	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (b < 0) b = -b;

	return (3 * r + 6 * g + b) * 8;
}

static inline int interp_32_dist(interp_uint32 a, interp_uint32 b)
{
	int r = (int)((a >> 16) & 255) - (int)((b >> 16) & 255);
	int g = (int)((a >> 8) & 255) - (int)((b >> 8) & 255);
	int bl = (int)(a & 255) - (int)(b & 255);

	if (r < 0) r = -r;
	if (g < 0) g = -g;
	if (bl < 0) bl = -bl;

	return 3 * r + 6 * g + bl;
}

static inline int interp_yuy2_dist(interp_uint32 p1, interp_uint32 p2)
{
	int y = (int)(p1 & 0xFF) - (int)(p2 & 0xFF);

	if (y < 0) y = -y;

	return y;
}

/**
 * Computes the distance between two three.
 * Equivalent at "dist(A,B) + dist(B,C)".
 * Used by XBR algorithm.
 * This is a very rought approximation taking into account only the luminosity.
 * See: http://www.compuphase.com/cmetric.htm
 */
static int interp_16_dist3(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return interp_16_dist(p1, p2) + interp_16_dist(p2, p3);
}

static inline int interp_16_565_dist3(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return interp_16_565_dist(p1, p2) + interp_16_565_dist(p2, p3);
}

static inline int interp_16_555_dist3(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return interp_16_555_dist(p1, p2) + interp_16_555_dist(p2, p3);
}

static inline int interp_32_dist3(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return interp_32_dist(p1, p2) + interp_32_dist(p2, p3);
}

static inline int interp_yuy2_dist3(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	int y1 = (p1 & 0xFF) - (p2 & 0xFF);
	int y2 = (p2 & 0xFF) - (p3 & 0xFF);

	if (y1 < 0) y1 = -y1;
	if (y2 < 0) y2 = -y2;

	return y1 + y2;
}

void interp_set(unsigned color_def);

#endif

