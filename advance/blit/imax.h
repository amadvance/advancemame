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

#ifndef __IMAX_H
#define __IMAX_H

#include "icommon.h"

/***************************************************************************/
/* internal_max_rgb */

static unsigned max_rgb_mask_0;
static unsigned max_rgb_mask_1;
static unsigned max_rgb_mask_2;
static int max_rgb_shift_1;
static int max_rgb_shift_2;

static void internal_max_rgb_set(void) {
	unsigned mask_0;
	unsigned mask_1;
	unsigned mask_2;
	unsigned shift_0;
	unsigned shift_1;
	unsigned shift_2;

	assert( video_index() == MODE_FLAGS_INDEX_RGB );

	if (video_state.rgb_red_shift >= video_state.rgb_green_shift &&
		video_state.rgb_red_shift >= video_state.rgb_blue_shift) {
		mask_0 = video_state.rgb_red_mask;
		mask_1 = video_state.rgb_green_mask;
		mask_2 = video_state.rgb_blue_mask;
		shift_0 = video_state.rgb_red_shift;
		shift_1 = video_state.rgb_green_shift;
		shift_2 = video_state.rgb_blue_shift;
	} else if (video_state.rgb_green_shift >= video_state.rgb_red_shift &&
		video_state.rgb_green_shift >= video_state.rgb_blue_shift) {
		mask_1 = video_state.rgb_red_mask;
		mask_0 = video_state.rgb_green_mask;
		mask_2 = video_state.rgb_blue_mask;
		shift_1 = video_state.rgb_red_shift;
		shift_0 = video_state.rgb_green_shift;
		shift_2 = video_state.rgb_blue_shift;
	} else {
		mask_2 = video_state.rgb_red_mask;
		mask_1 = video_state.rgb_green_mask;
		mask_0 = video_state.rgb_blue_mask;
		shift_2 = video_state.rgb_red_shift;
		shift_1 = video_state.rgb_green_shift;
		shift_0 = video_state.rgb_blue_shift;
	}

	max_rgb_mask_0 = mask_0;
	max_rgb_mask_1 = mask_1;
	max_rgb_mask_2 = mask_2;
	max_rgb_shift_1 = shift_0 - shift_1;
	max_rgb_shift_2 = shift_0 - shift_2;

	assert( max_rgb_shift_1 >= 0 && max_rgb_shift_2 >= 0);
}

static __inline__ unsigned internal_max_rgb_value(unsigned v) {
	return (v & max_rgb_mask_0)
		+ ((v << max_rgb_shift_1) & max_rgb_mask_1)
		+ ((v << max_rgb_shift_2) & max_rgb_mask_2);
}

static __inline__ void internal_max_rgb8_vert_self(uint8* dst, const uint8* src, unsigned count) {
	while (count) {
		if (internal_max_rgb_value(dst[0]) < internal_max_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static __inline__ void internal_max_rgb8_vert_self_step(uint8* dst, const uint8* src, unsigned count, int step1) {
	while (count) {
		if (internal_max_rgb_value(dst[0]) < internal_max_rgb_value(src[0]))
			dst[0] = src[0];
		src += step1;
		++dst;
		--count;
	}
}

static __inline__ void internal_max_rgb16_vert_self(uint16* dst, const uint16* src, unsigned count) {
	while (count) {
		if (internal_max_rgb_value(dst[0]) < internal_max_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static __inline__ void internal_max_rgb16_vert_self_step(uint16* dst, const uint16* src, unsigned count, int step1) {
	while (count) {
		if (internal_max_rgb_value(dst[0]) < internal_max_rgb_value(src[0]))
			dst[0] = src[0];
		PADD(src,step1);
		++dst;
		--count;
	}
}

static __inline__ void internal_max_rgb32_vert_self(uint32* dst, const uint32* src, unsigned count) {
	while (count) {
		if (internal_max_rgb_value(dst[0]) < internal_max_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static __inline__ void internal_max_rgb32_vert_self_step(uint32* dst, const uint32* src, unsigned count, int step1) {
	while (count) {
		if (internal_max_rgb_value(dst[0]) < internal_max_rgb_value(src[0]))
			dst[0] = src[0];
		PADD(src,step1);
		++dst;
		--count;
	}
}

/***************************************************************************/
/* internal_max */

static __inline__ void internal_max8_vert_self(uint8* dst, const uint8* src, unsigned count) {
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static __inline__ void internal_max8_vert_self_step(uint8* dst, const uint8* src, unsigned count, int step1) {
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		src += step1;
		++dst;
		--count;
	}
}

static __inline__ void internal_max16_vert_self(uint16* dst, const uint16* src, unsigned count) {
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static __inline__ void internal_max16_vert_self_step(uint16* dst, const uint16* src, unsigned count, int step1) {
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		PADD(src,step1);
		++dst;
		--count;
	}
}

static __inline__ void internal_max32_vert_self(uint32* dst, const uint32* src, unsigned count) {
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static __inline__ void internal_max32_vert_self_step(uint32* dst, const uint32* src, unsigned count, int step1) {
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		PADD(src,step1);
		++dst;
		--count;
	}
}

#endif
