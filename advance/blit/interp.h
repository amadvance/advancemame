/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
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
typedef unsigned interp_uint32;

/***************************************************************************/
/* interpolation */

static unsigned interp_mask[2];
static unsigned interp_red_mask, interp_green_mask, interp_blue_mask;
static int interp_red_shift, interp_green_shift, interp_blue_shift;
static unsigned interp_near_mask;

#define INTERP_16_MASK_1(v) ((v) & interp_mask[0])
#define INTERP_16_MASK_2(v) ((v) & interp_mask[1])
#define INTERP_16_UNMASK_1(v) ((v) & interp_mask[0])
#define INTERP_16_UNMASK_2(v) ((v) & interp_mask[1])

static inline interp_uint16 interp_16_521(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*5 + INTERP_16_MASK_1(p2)*2 + INTERP_16_MASK_1(p3)*1) / 8)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*5 + INTERP_16_MASK_2(p2)*2 + INTERP_16_MASK_2(p3)*1) / 8);
}

static inline interp_uint16 interp_16_332(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*3 + INTERP_16_MASK_1(p2)*3 + INTERP_16_MASK_1(p3)*2) / 8)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*3 + INTERP_16_MASK_2(p2)*3 + INTERP_16_MASK_2(p3)*2) / 8);
}

static inline interp_uint16 interp_16_611(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*6 + INTERP_16_MASK_1(p2) + INTERP_16_MASK_1(p3)) / 8)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*6 + INTERP_16_MASK_2(p2) + INTERP_16_MASK_2(p3)) / 8);
}

static inline interp_uint16 interp_16_71(interp_uint16 p1, interp_uint16 p2)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*7 + INTERP_16_MASK_1(p2)) / 8)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*7 + INTERP_16_MASK_2(p2)) / 8);
}

static inline interp_uint16 interp_16_211(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*2 + INTERP_16_MASK_1(p2) + INTERP_16_MASK_1(p3)) / 4)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*2 + INTERP_16_MASK_2(p2) + INTERP_16_MASK_2(p3)) / 4);
}

static inline interp_uint16 interp_16_772(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1(((INTERP_16_MASK_1(p1) + INTERP_16_MASK_1(p2))*7 + INTERP_16_MASK_1(p3)*2) / 16)
		| INTERP_16_UNMASK_2(((INTERP_16_MASK_2(p1) + INTERP_16_MASK_2(p2))*7 + INTERP_16_MASK_2(p3)*2) / 16);
}

static inline interp_uint16 interp_16_11(interp_uint16 p1, interp_uint16 p2)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1) + INTERP_16_MASK_1(p2)) / 2)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1) + INTERP_16_MASK_2(p2)) / 2);
}

static inline interp_uint16 interp_16_31(interp_uint16 p1, interp_uint16 p2)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*3 + INTERP_16_MASK_1(p2)) / 4)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*3 + INTERP_16_MASK_2(p2)) / 4);
}

static inline interp_uint16 interp_16_1411(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*14 + INTERP_16_MASK_1(p2) + INTERP_16_MASK_1(p3)) / 16)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*14 + INTERP_16_MASK_2(p2) + INTERP_16_MASK_2(p3)) / 16);
}

static inline interp_uint16 interp_16_431(interp_uint16 p1, interp_uint16 p2, interp_uint16 p3)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*4 + INTERP_16_MASK_1(p2)*3 + INTERP_16_MASK_1(p3)) / 8)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*4 + INTERP_16_MASK_2(p2)*3 + INTERP_16_MASK_2(p3)) / 8);
}

static inline interp_uint16 interp_16_53(interp_uint16 p1, interp_uint16 p2)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*5 + INTERP_16_MASK_1(p2)*3) / 8)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*5 + INTERP_16_MASK_2(p2)*3) / 8);
}

static inline interp_uint16 interp_16_151(interp_uint16 p1, interp_uint16 p2)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*15 + INTERP_16_MASK_1(p2)) / 16)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*15 + INTERP_16_MASK_2(p2)) / 16);
}

static inline interp_uint16 interp_16_97(interp_uint16 p1, interp_uint16 p2)
{
	return INTERP_16_UNMASK_1((INTERP_16_MASK_1(p1)*9 + INTERP_16_MASK_1(p2)*7) / 16)
		| INTERP_16_UNMASK_2((INTERP_16_MASK_2(p1)*9 + INTERP_16_MASK_2(p2)*7) / 16);
}

#define INTERP_32_MASK_1(v) ((v) & 0xFF00FFU)
#define INTERP_32_MASK_2(v) ((v) & 0x00FF00U)
#define INTERP_32_UNMASK_1(v) ((v) & 0xFF00FFU)
#define INTERP_32_UNMASK_2(v) ((v) & 0x00FF00U)

static inline interp_uint32 interp_32_521(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*5 + INTERP_32_MASK_1(p2)*2 + INTERP_32_MASK_1(p3)*1) / 8)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*5 + INTERP_32_MASK_2(p2)*2 + INTERP_32_MASK_2(p3)*1) / 8);
}

static inline interp_uint32 interp_32_332(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*3 + INTERP_32_MASK_1(p2)*3 + INTERP_32_MASK_1(p3)*2) / 8)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*3 + INTERP_32_MASK_2(p2)*3 + INTERP_32_MASK_2(p3)*2) / 8);
}

static inline interp_uint32 interp_32_211(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*2 + INTERP_32_MASK_1(p2) + INTERP_32_MASK_1(p3)) / 4)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*2 + INTERP_32_MASK_2(p2) + INTERP_32_MASK_2(p3)) / 4);
}

static inline interp_uint32 interp_32_611(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*6 + INTERP_32_MASK_1(p2) + INTERP_32_MASK_1(p3)) / 8)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*6 + INTERP_32_MASK_2(p2) + INTERP_32_MASK_2(p3)) / 8);
}

static inline interp_uint32 interp_32_71(interp_uint32 p1, interp_uint32 p2)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*7 + INTERP_32_MASK_1(p2)) / 8)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*7 + INTERP_32_MASK_2(p2)) / 8);
}

static inline interp_uint32 interp_32_772(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1(((INTERP_32_MASK_1(p1) + INTERP_32_MASK_1(p2))*7 + INTERP_32_MASK_1(p3)*2) / 16)
		| INTERP_32_UNMASK_2(((INTERP_32_MASK_2(p1) + INTERP_32_MASK_2(p2))*7 + INTERP_32_MASK_2(p3)*2) / 16);
}

static inline interp_uint32 interp_32_11(interp_uint32 p1, interp_uint32 p2)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1) + INTERP_32_MASK_1(p2)) / 2)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1) + INTERP_32_MASK_2(p2)) / 2);
}

static inline interp_uint32 interp_32_31(interp_uint32 p1, interp_uint32 p2)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*3 + INTERP_32_MASK_1(p2)) / 4)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*3 + INTERP_32_MASK_2(p2)) / 4);
}

static inline interp_uint32 interp_32_1411(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*14 + INTERP_32_MASK_1(p2) + INTERP_32_MASK_1(p3)) / 16)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*14 + INTERP_32_MASK_2(p2) + INTERP_32_MASK_2(p3)) / 16);
}

static inline interp_uint32 interp_32_431(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*4 + INTERP_32_MASK_1(p2)*3 + INTERP_32_MASK_1(p3)) / 8)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*4 + INTERP_32_MASK_2(p2)*3 + INTERP_32_MASK_2(p3)) / 8);
}

static inline interp_uint32 interp_32_53(interp_uint32 p1, interp_uint32 p2)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*5 + INTERP_32_MASK_1(p2)*3) / 8)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*5 + INTERP_32_MASK_2(p2)*3) / 8);
}

static inline interp_uint32 interp_32_151(interp_uint32 p1, interp_uint32 p2)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*15 + INTERP_32_MASK_1(p2)) / 16)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*15 + INTERP_32_MASK_2(p2)) / 16);
}

static inline interp_uint32 interp_32_97(interp_uint32 p1, interp_uint32 p2)
{
	return INTERP_32_UNMASK_1((INTERP_32_MASK_1(p1)*9 + INTERP_32_MASK_1(p2)*7) / 16)
		| INTERP_32_UNMASK_2((INTERP_32_MASK_2(p1)*9 + INTERP_32_MASK_2(p2)*7) / 16);
}

/***************************************************************************/
/* diff */

#define INTERP_Y_LIMIT (0x30 * 4)
#define INTERP_U_LIMIT (0x07 * 4)
#define INTERP_V_LIMIT (0x06 * 8)

static int interp_16_diff(interp_uint16 p1, interp_uint16 p2)
{
	int r, g, b;
	int y, u, v;

#if 0 /* assume standard rgb formats */
	if ((p1 & interp_near_mask) == (p2 & interp_near_mask))
		return 0;

	b = rgb_shift_sign((p1 & interp_blue_mask) - (p2 & interp_blue_mask), interp_blue_shift);
	g = rgb_shift_sign((p1 & interp_green_mask) - (p2 & interp_green_mask), interp_green_shift);
	r = rgb_shift_sign((p1 & interp_red_mask) - (p2 & interp_red_mask), interp_red_shift);
#else
	if (p1 == p2)
		return 0;

	if (interp_green_mask == 0x7E0) {
		b = (int)((p1 & 0x1F) - (p2 & 0x1F)) << 3;
		g = (int)((p1 & 0x7E0) - (p2 & 0x7E0)) >> 3;
		r = (int)((p1 & 0xF800) - (p2 & 0xF800)) >> 8;
	} else {
		b = (int)((p1 & 0x1F) - (p2 & 0x1F)) << 3;
		g = (int)((p1 & 0x3E0) - (p2 & 0x3E0)) >> 2;
		r = (int)((p1 & 0x7C00) - (p2 & 0x7C00)) >> 7;
	}
#endif

	y = r + g + b;

	if (y < -INTERP_Y_LIMIT || y > INTERP_Y_LIMIT)
		return 1;

	u = r - b;

	if (u < -INTERP_U_LIMIT || u > INTERP_U_LIMIT)
		return 1;

	v = -r + 2*g - b;

	if (v < -INTERP_V_LIMIT || v > INTERP_V_LIMIT)
		return 1;

	return 0;
}

static int interp_32_diff(interp_uint32 p1, interp_uint32 p2)
{
	int r, g, b;
	int y, u, v;

#if 0 /* assume standard rgb formats */
	if ((p1 & interp_near_mask) == (p2 & interp_near_mask))
		return 0;

	b = rgb_shift_sign((p1 & interp_blue_mask) - (p2 & interp_blue_mask), interp_blue_shift);
	g = rgb_shift_sign((p1 & interp_green_mask) - (p2 & interp_green_mask), interp_green_shift);
	r = rgb_shift_sign((p1 & interp_red_mask) - (p2 & interp_red_mask), interp_red_shift);
#else
	if ((p1 & 0xF8F8F8) == (p2 & 0xF8F8F8))
		return 0;

	b = (int)((p1 & 0xFF) - (p2 & 0xFF));
	g = (int)((p1 & 0xFF00) - (p2 & 0xFF00)) >> 8;
	r = (int)((p1 & 0xFF0000) - (p2 & 0xFF0000)) >> 16;
#endif

	y = r + g + b;

	if (y < -INTERP_Y_LIMIT || y > INTERP_Y_LIMIT)
		return 1;

	u = r - b;

	if (u < -INTERP_U_LIMIT || u > INTERP_U_LIMIT)
		return 1;

	v = -r + 2*g - b;

	if (v < -INTERP_V_LIMIT || v > INTERP_V_LIMIT)
		return 1;

	return 0;
}

static void interp_set(const struct video_pipeline_target_struct* target)
{
	union adv_color_def_union def;

	def.ordinal = target->color_def;

	rgb_shiftmask_get(&interp_red_shift, &interp_red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&interp_green_shift, &interp_green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&interp_blue_shift, &interp_blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	interp_mask[0] = interp_red_mask | interp_blue_mask;
	interp_mask[1] = interp_green_mask;

	interp_near_mask = ~(
			((interp_red_mask >> 5) & interp_red_mask)
			| ((interp_green_mask >> 5) & interp_green_mask)
			| ((interp_blue_mask >> 5) & interp_blue_mask)
		) & (interp_red_mask | interp_green_mask | interp_blue_mask);
}

#endif
