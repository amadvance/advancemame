/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2008 Andrea Mazzoleni
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

#include "portable.h"

#include "interp.h"

#include "rgb.h"

unsigned interp_mask[2];
unsigned interp_red_mask, interp_green_mask, interp_blue_mask;
int interp_red_shift, interp_green_shift, interp_blue_shift;
unsigned interp_near_mask;
unsigned interp_highnot_mask;

/***************************************************************************/
/* diff */

#define INTERP_Y_LIMIT 0x30
#define INTERP_U_LIMIT 0x07
#define INTERP_V_LIMIT 0x06

/* Multipled version */
#define INTERP_Y_LIMIT_S2 (INTERP_Y_LIMIT << 2)
#define INTERP_U_LIMIT_S2 (INTERP_U_LIMIT << 2)
#define INTERP_V_LIMIT_S3 (INTERP_V_LIMIT << 3)
#define INTERP_U_LIMIT_S8 (INTERP_U_LIMIT << 8)
#define INTERP_V_LIMIT_S24 (INTERP_V_LIMIT << 24)

int interp_16_diff(interp_uint16 p1, interp_uint16 p2)
{
	int r, g, b;
	int y4, u4, v8;
	int i1, i2;

	/* assume standard rgb formats */
	if (p1 == p2)
		return 0;

	i1 = p1;
	i2 = p2;

	if (interp_green_mask == 0x7E0) {
		b = ((i1 & 0x1F) - (i2 & 0x1F)) << 3;
		g = ((i1 & 0x7E0) - (i2 & 0x7E0)) >> 3;
		r = ((i1 & 0xF800) - (i2 & 0xF800)) >> 8;
	} else {
		b = ((i1 & 0x1F) - (i2 & 0x1F)) << 3;
		g = ((i1 & 0x3E0) - (i2 & 0x3E0)) >> 2;
		r = ((i1 & 0x7C00) - (i2 & 0x7C00)) >> 7;
	}

	/* not exact, but fast */
	y4 = r + g + b;
	u4 = r - b;
	v8 = -r + 2*g - b;

	if (y4 < -INTERP_Y_LIMIT_S2 || y4 > INTERP_Y_LIMIT_S2)
		return 1;

	if (u4 < -INTERP_U_LIMIT_S2 || u4 > INTERP_U_LIMIT_S2)
		return 1;

	if (v8 < -INTERP_V_LIMIT_S3 || v8 > INTERP_V_LIMIT_S3)
		return 1;

	return 0;
}

int interp_32_diff(interp_uint32 p1, interp_uint32 p2)
{
	int r, g, b;
	int y, u, v;
	int i1, i2;

	/* assume standard rgb formats */
	if ((p1 & 0xF8F8F8) == (p2 & 0xF8F8F8))
		return 0;

	i1 = p1;
	i2 = p2;

	b = ((i1 & 0xFF) - (i2 & 0xFF));
	g = ((i1 & 0xFF00) - (i2 & 0xFF00)) >> 8;
	r = ((i1 & 0xFF0000) - (i2 & 0xFF0000)) >> 16;

	/* not exact, but fast */
	y = r + g + b;
	u = r - b;
	v = -r + 2*g - b;

	if (y < -INTERP_Y_LIMIT_S2 || y > INTERP_Y_LIMIT_S2)
		return 1;

	if (u < -INTERP_U_LIMIT_S2 || u > INTERP_U_LIMIT_S2)
		return 1;

	if (v < -INTERP_V_LIMIT_S3 || v > INTERP_V_LIMIT_S3)
		return 1;

	return 0;
}

int interp_yuy2_diff(interp_uint32 p1, interp_uint32 p2)
{
	int y, u, v;
	int i1, i2;

	if (p1 == p2)
		return 0;

	i1 = p1;
	i2 = p2;

	y = (i1 & 0xFF) - (i2 & 0xFF);
	u = (i1 & 0xFF00) - (i2 & 0xFF00);
	v = (i1 & 0xFF000000) - (i2 & 0xFF000000);

	if (y < -INTERP_Y_LIMIT || y > INTERP_Y_LIMIT)
		return 1;

	if (u < -INTERP_U_LIMIT_S8 || u > INTERP_U_LIMIT_S8)
		return 1;

	if (v < -INTERP_V_LIMIT_S24 || v > INTERP_V_LIMIT_S24)
		return 1;

	return 0;
}

int interp_16_dist(interp_uint16 p1, interp_uint16 p2)
{
	int r, g, b;
	int y4, u4, v8;
	int i1, i2;

	/* assume standard rgb formats */
	if (p1 == p2)
		return 0;

	i1 = p1;
	i2 = p2;

	if (interp_green_mask == 0x7E0) {
		b = ((i1 & 0x1F) - (i2 & 0x1F)) << 3;
		g = ((i1 & 0x7E0) - (i2 & 0x7E0)) >> 3;
		r = ((i1 & 0xF800) - (i2 & 0xF800)) >> 8;
	} else {
		b = ((i1 & 0x1F) - (i2 & 0x1F)) << 3;
		g = ((i1 & 0x3E0) - (i2 & 0x3E0)) >> 2;
		r = ((i1 & 0x7C00) - (i2 & 0x7C00)) >> 7;
	}

	if (b<0) b = -b;
	if (g<0) g = -g;
	if (r<0) r = -r;

	return 3*r + 4*g + 2*b;
}

int interp_32_dist(interp_uint32 p1, interp_uint32 p2)
{
	int r, g, b;
	int y, u, v;
	int i1, i2;

	/* assume standard rgb formats */
	if ((p1 & 0xF8F8F8) == (p2 & 0xF8F8F8))
		return 0;

	i1 = p1;
	i2 = p2;

	b = ((i1 & 0xFF) - (i2 & 0xFF));
	g = ((i1 & 0xFF00) - (i2 & 0xFF00)) >> 8;
	r = ((i1 & 0xFF0000) - (i2 & 0xFF0000)) >> 16;

	if (b<0) b = -b;
	if (g<0) g = -g;
	if (r<0) r = -r;

	return 3*r + 4*g + 2*b;
}

int interp_yuy2_dist(interp_uint32 p1, interp_uint32 p2)
{
	int y;
	int i1, i2;

	if (p1 == p2)
		return 0;

	i1 = p1;
	i2 = p2;

	y = ((i1 & 0xFF) - (i2 & 0xFF));

	if (y<0) y = -y;

	return y;
}

int interp_yuy2_dist3(interp_uint32 p1, interp_uint32 p2, interp_uint32 p3)
{
	int i1, i2, i3;

	if (p1 == p2 && p2 == p3)
		return 0;

	i1 = p1 & 0xFF;
	i2 = p2 & 0xFF;
	i3 = p3 & 0xFF;

	i1 -= i2;
	i2 -= i3;

	if (i1 < 0) i1 = -i1;
	if (i2 < 0) i2 = -i2;

	return i1 + i2;
}

void interp_set(unsigned color_def)
{
	if (color_def_type_get(color_def) == adv_color_type_rgb) {
		union adv_color_def_union def;

		def.ordinal = color_def;

		rgb_shiftmask_get(&interp_red_shift, &interp_red_mask, def.nibble.red_len, def.nibble.red_pos);
		rgb_shiftmask_get(&interp_green_shift, &interp_green_mask, def.nibble.green_len, def.nibble.green_pos);
		rgb_shiftmask_get(&interp_blue_shift, &interp_blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

		interp_mask[0] = interp_red_mask | interp_blue_mask;
		interp_mask[1] = interp_green_mask;

		interp_highnot_mask = (~rgb_highmask_make_from_def(color_def)) & rgb_wholemask_make_from_def(color_def);

		interp_near_mask = ~(
				((interp_red_mask >> 5) & interp_red_mask)
				| ((interp_green_mask >> 5) & interp_green_mask)
				| ((interp_blue_mask >> 5) & interp_blue_mask)
			) & (interp_red_mask | interp_green_mask | interp_blue_mask);
	} else {
		interp_mask[0] = 0;
		interp_mask[1] = 0;
		interp_highnot_mask = 0;
		interp_near_mask = 0;
	}
}

