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

/** \name RGB
 * Basic RGB types.
 */
/*@{*/

/**
 * RGB color.
 */
typedef struct video_color_struct {
	uint8 blue __attribute__ ((packed));
	uint8 green __attribute__ ((packed));
	uint8 red __attribute__ ((packed));
	uint8 alpha __attribute__ ((packed));
} video_color;

/**
 * RGB as ordinal value.
 */
typedef unsigned video_rgb;

/**
 * RGB nibble.
 */
struct video_rgb_def_bits {
	unsigned red_len : 4; /* bit used for the red channel */
	unsigned red_pos : 5; /* shift for the red channel */
	unsigned green_len : 4;
	unsigned green_pos : 5;
	unsigned blue_len : 4;
	unsigned blue_pos : 5;
	/* 5*3+4*3 = 27 bit */
	unsigned dummy : 5;
};

/**
 * RGB nibble as ordinal value.
 */
typedef unsigned video_rgb_def;

/**
 * RGB nibble multiformat.
 */
union video_rgb_def_union {
	video_rgb_def ordinal;
	struct video_rgb_def_bits nibble;
};

const char* video_rgb_def_name_make(video_rgb_def rgb_def);
video_rgb_def video_rgb_def_make(unsigned red_len, unsigned red_pos, unsigned green_len, unsigned green_pos, unsigned blue_len, unsigned blue_pos);
video_rgb_def video_rgb_def_make_from_maskshift(unsigned red_mask, unsigned red_shift, unsigned green_mask, unsigned green_shift, unsigned blue_mask, unsigned blue_shift);
video_rgb video_rgb_make_from_def(unsigned r, unsigned g, unsigned b, video_rgb_def def);

static __inline__ unsigned video_rgb_shift_make_from_def(unsigned len, unsigned pos) {
	return pos + len - 8;
}

static __inline__ unsigned video_rgb_mask_make_from_def(unsigned len, unsigned pos) {
	return ((1 << len) - 1) << pos;
}

unsigned video_color_dist(const video_color* A, const video_color* B);

static __inline__ unsigned video_rgb_shift(unsigned value, int shift) {
	if (shift >= 0)
		return value >> shift;
	else
		return value << -shift;
}

static __inline__ unsigned video_rgb_nibble_insert(unsigned value, int shift, unsigned mask) {
	return video_rgb_shift(value,-shift) & mask;
}

static __inline__ unsigned video_rgb_nibble_extract(unsigned value, int shift, unsigned mask) {
	return video_rgb_shift(value & mask,shift);
}

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
