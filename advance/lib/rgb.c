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

#include "rgb.h"

#include <stdio.h>

static char rgb_name_buffer[64];

/**
 * Compute a RGB value with a specific format
 * \param r,g,b RGB values 0-255
 * \param def RGB format
 * \return RGB nibble
 */
video_rgb video_rgb_make_from_def(unsigned r, unsigned g, unsigned b, video_rgb_def def)
{
	union video_rgb_def_union rgb;
	rgb.ordinal = def;
	return video_rgb_nibble_insert(r, video_rgb_shift_make_from_def(rgb.nibble.red_len,rgb.nibble.red_pos), video_rgb_mask_make_from_def(rgb.nibble.red_len,rgb.nibble.red_pos))
		| video_rgb_nibble_insert(g, video_rgb_shift_make_from_def(rgb.nibble.green_len,rgb.nibble.green_pos), video_rgb_mask_make_from_def(rgb.nibble.green_len,rgb.nibble.green_pos))
		| video_rgb_nibble_insert(b, video_rgb_shift_make_from_def(rgb.nibble.blue_len,rgb.nibble.blue_pos), video_rgb_mask_make_from_def(rgb.nibble.blue_len,rgb.nibble.blue_pos));
}

/**
 * Get a textual description of a RGB nibble definition.
 * Return a string description in the format red_len/red_pos,green_len/green_pos,blue_len/blue_pos.
 * \return Pointer at a static buffer.
 */
const char* video_rgb_def_name_make(video_rgb_def def)
{
	union video_rgb_def_union rgb;
	rgb.ordinal = def;
	sprintf(rgb_name_buffer,"%d/%d,%d/%d,%d/%d",
		rgb.nibble.red_len,rgb.nibble.red_pos,
		rgb.nibble.green_len,rgb.nibble.green_pos,
		rgb.nibble.blue_len,rgb.nibble.blue_pos
	);
	return rgb_name_buffer;
}

/**
 * Compute the size in bit of the mask.
 */
static unsigned video_rgb_len_get_from_mask(unsigned mask)
{
	unsigned len = 0;
	if (!mask)
		return len;
	while ((mask & 1) == 0)
		mask >>= 1;
	while (mask) {
		++len;
		mask >>= 1;
	}
	return len;
}

/**
 * Make an arbitary RGB format definition.
 * \param red_len Bits of the red channel.
 * \param red_pos Bit position red channel.
 * \param green_len Bits of the green channel.
 * \param green_pos Bit position green channel.
 * \param blue_len Bits of the blue channel.
 * \param blue_pos Bit position blue channel.
 * \return RGB format.
 */
video_rgb_def video_rgb_def_make(unsigned red_len, unsigned red_pos, unsigned green_len, unsigned green_pos, unsigned blue_len, unsigned blue_pos)
{
	union video_rgb_def_union def;
	def.ordinal = 0;

	def.nibble.red_len = red_len;
	def.nibble.red_pos = red_pos;
	def.nibble.green_len = green_len;
	def.nibble.green_pos = green_pos;
	def.nibble.blue_len = blue_len;
	def.nibble.blue_pos = blue_pos;

	return def.ordinal;
}

/**
 * Make an arbitary RGB format definition from a maskshift specification.
 */
video_rgb_def video_rgb_def_make_from_maskshift(unsigned red_mask, unsigned red_shift, unsigned green_mask, unsigned green_shift, unsigned blue_mask, unsigned blue_shift)
{
	unsigned red_len = video_rgb_len_get_from_mask(red_mask);
	unsigned green_len = video_rgb_len_get_from_mask(green_mask);
	unsigned blue_len = video_rgb_len_get_from_mask(blue_mask);
	unsigned red_pos = 8 + red_shift - red_len;
	unsigned green_pos = 8 + green_shift - green_len;
	unsigned blue_pos = 8 + blue_shift - blue_len;

	return video_rgb_def_make(red_len, red_pos, green_len, green_pos, blue_len, blue_pos);
}

/**
 * Compute the distance of two color.
 */
unsigned video_color_dist(const video_color* A, const video_color* B)
{
	int r, g, b;
	r = (int)A->red - B->red;
	g = (int)A->green - B->green;
	b = (int)A->blue - B->blue;
	return r*r + g*g + b*b;
}

