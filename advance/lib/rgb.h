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

/** \file
 * RGB.
 */

#ifndef __RGB_H
#define __RGB_H

#include "extra.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup Color */
/*@{*/

/**
 * RGB color.
 */
typedef struct video_color_struct {
	uint8 blue __attribute__ ((packed)); /**< Blue channel. From 0 to 255. */
	uint8 green __attribute__ ((packed)); /**< Green channel. From 0 to 255. */
	uint8 red __attribute__ ((packed)); /**< Red channel. From 0 to 255. */
	uint8 alpha __attribute__ ((packed)); /**< Alpha channel. From 0 to 255. */
} adv_color;

/**
 * RGB as ordinal value.
 * The effective format is not defined. It depends on the current context.
 */
typedef unsigned adv_rgb;

/**
 * RGB definition as bit nibble.
 */
struct adv_rgb_def_bits {
	unsigned red_len : 4; /**< Bits for the red channel. */
	unsigned red_pos : 5; /**< Shift for the red channel. */
	unsigned green_len : 4; /**< Bits for the green channel. */
	unsigned green_pos : 5; /**< Shift for the green channel. */
	unsigned blue_len : 4; /**< Bits for the blue channel. */
	unsigned blue_pos : 5; /**< Shift for the blue channel. */
	/* 5*3+4*3 = 27 bit */
	unsigned dummy : 5; /**< Unused bit. */
};

/**
 * RGB definition as ordinal value.
 */
typedef unsigned adv_rgb_def;

/**
 * RGB definition as union.
 */
union adv_rgb_def_union {
	adv_rgb_def ordinal;
	struct adv_rgb_def_bits nibble;
};

const char* rgb_def_name_make(adv_rgb_def rgb_def);
adv_rgb_def rgb_def_make(unsigned red_len, unsigned red_pos, unsigned green_len, unsigned green_pos, unsigned blue_len, unsigned blue_pos);
adv_rgb_def rgb_def_make_from_maskshift(unsigned red_mask, int red_shift, unsigned green_mask, int green_shift, unsigned blue_mask, int blue_shift);
adv_rgb rgb_make_from_def(unsigned r, unsigned g, unsigned b, adv_rgb_def def);

/**
 * Get a channel shift from the RGB definition.
 * This value is the number of bit to shift left a 8 bit channel value to match
 * the specified RGB definition. It may be a negative number.
 */
static inline int rgb_shift_make_from_def(unsigned len, unsigned pos) {
	return pos + len - 8;
}

/**
 * Get a channel mask from the RGB definition.
 * This value is the mask bit of the specified channel RGB definition. 
 */
static inline unsigned rgb_mask_make_from_def(unsigned len, unsigned pos) {
	return ((1 << len) - 1) << pos;
}

unsigned video_color_dist(const adv_color* A, const adv_color* B);

/**
 * Shift a value.
 * \param value Value to shift.
 * \param shift Number of bit to shift right. If negative the value is shifted left.
 */
static inline unsigned rgb_shift(unsigned value, int shift) {
	if (shift >= 0)
		return value >> shift;
	else
		return value << -shift;
}

/**
 * Convert a 8 bit channel to a specific subformat channel.
 * \param value 8 bit channel.
 * \param shift Shift for the channel. Generally computed with rgb_shift_make_from_def().
 * \param mask Mask for the channel. Generally computed with rgb_mask_make_from_def().
 */
static inline unsigned rgb_nibble_insert(unsigned value, int shift, unsigned mask) {
	return rgb_shift(value,-shift) & mask;
}

/**
 * Convert a specific subformat channel to a 8 bit channel.
 * \param value Subformat channel.
 * \param shift Shift for the channel. Generally computed with rgb_shift_make_from_def().
 * \param mask Mask for the channel. Generally computed with rgb_mask_make_from_def().
 */
static inline unsigned rgb_nibble_extract(unsigned value, int shift, unsigned mask) {
	return rgb_shift(value & mask,shift);
}

unsigned rgb_approx(unsigned value, unsigned len);

/**
 * Compute the shift and mask values.
 */
static inline void rgb_maskshift_get(unsigned* mask, int* shift, unsigned len, unsigned pos)
{
	*mask = ((1 << len) - 1) << pos;
	*shift = pos + len - 8;
}

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
