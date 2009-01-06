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

#ifndef __IMAX_H
#define __IMAX_H

#include "icommon.h"

/***************************************************************************/
/* internal_maxmin_rgb */

static unsigned max_rgb_mask_0;
static unsigned max_rgb_mask_1;
static unsigned max_rgb_mask_2;
static unsigned max_rgb_shift_1;
static unsigned max_rgb_shift_2;

#define SWAP(type, x, y) \
	do { \
		type temp; \
		temp = x; \
		x = y; \
		y = temp; \
	} while (0)

static void internal_maxmin_rgb_set(const struct video_pipeline_target_struct* target)
{
	unsigned mask_0;
	unsigned mask_1;
	unsigned mask_2;
	int shift_0;
	int shift_1;
	int shift_2;

	union adv_color_def_union def;

	def.ordinal = target->color_def;

	rgb_shiftmask_get(&shift_0, &mask_0, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&shift_1, &mask_1, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&shift_2, &mask_2, def.nibble.blue_len, def.nibble.blue_pos);

	/* shift_0 must be the lowest value */
	if (shift_0 > shift_1) {
		SWAP(int, shift_0, shift_1);
		SWAP(unsigned, mask_0, mask_1);
	}

	if (shift_0 > shift_2) {
		SWAP(int, shift_0, shift_2);
		SWAP(unsigned, mask_0, mask_2);
	}

	max_rgb_mask_0 = mask_0;
	max_rgb_mask_1 = mask_1;
	max_rgb_mask_2 = mask_2;

	assert(shift_1 - shift_0 > 0 && shift_2 - shift_0 > 0);

	max_rgb_shift_1 = shift_1 - shift_0;
	max_rgb_shift_2 = shift_2 - shift_0;
}

static inline unsigned internal_lum_rgb_value(unsigned v)
{
	return (v & max_rgb_mask_0)
		+ ((v & max_rgb_mask_1) >> max_rgb_shift_1)
		+ ((v & max_rgb_mask_2) >> max_rgb_shift_2);
}

static inline void internal_max_rgb8_vert_self(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) < internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_max_rgb8_vert_self_step(uint8* dst, const uint8* src, unsigned count, int step1)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) < internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		src += step1;
		++dst;
		--count;
	}
}

static inline void internal_max_rgb16_vert_self(uint16* dst, const uint16* src, unsigned count)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) < internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_max_rgb16_vert_self_step(uint16* dst, const uint16* src, unsigned count, int step1)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) < internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

static inline void internal_max_rgb32_vert_self(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) < internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_max_rgb32_vert_self_step(uint32* dst, const uint32* src, unsigned count, int step1)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) < internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

static inline void internal_min_rgb8_vert_self(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) > internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_min_rgb8_vert_self_step(uint8* dst, const uint8* src, unsigned count, int step1)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) > internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		src += step1;
		++dst;
		--count;
	}
}

static inline void internal_min_rgb16_vert_self(uint16* dst, const uint16* src, unsigned count)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) > internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_min_rgb16_vert_self_step(uint16* dst, const uint16* src, unsigned count, int step1)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) > internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

static inline void internal_min_rgb32_vert_self(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) > internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_min_rgb32_vert_self_step(uint32* dst, const uint32* src, unsigned count, int step1)
{
	while (count) {
		if (internal_lum_rgb_value(dst[0]) > internal_lum_rgb_value(src[0]))
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

/***************************************************************************/
/* internal_maxmin */

static inline void internal_max8_vert_self(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_max8_vert_self_step(uint8* dst, const uint8* src, unsigned count, int step1)
{
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		src += step1;
		++dst;
		--count;
	}
}

static inline void internal_max16_vert_self(uint16* dst, const uint16* src, unsigned count)
{
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_max16_vert_self_step(uint16* dst, const uint16* src, unsigned count, int step1)
{
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

static inline void internal_max32_vert_self(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_max32_vert_self_step(uint32* dst, const uint32* src, unsigned count, int step1)
{
	while (count) {
		if (dst[0] < src[0])
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

static inline void internal_min8_vert_self(uint8* dst, const uint8* src, unsigned count)
{
	while (count) {
		if (dst[0] > src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_min8_vert_self_step(uint8* dst, const uint8* src, unsigned count, int step1)
{
	while (count) {
		if (dst[0] > src[0])
			dst[0] = src[0];
		src += step1;
		++dst;
		--count;
	}
}

static inline void internal_min16_vert_self(uint16* dst, const uint16* src, unsigned count)
{
	while (count) {
		if (dst[0] > src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_min16_vert_self_step(uint16* dst, const uint16* src, unsigned count, int step1)
{
	while (count) {
		if (dst[0] > src[0])
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

static inline void internal_min32_vert_self(uint32* dst, const uint32* src, unsigned count)
{
	while (count) {
		if (dst[0] > src[0])
			dst[0] = src[0];
		++src;
		++dst;
		--count;
	}
}

static inline void internal_min32_vert_self_step(uint32* dst, const uint32* src, unsigned count, int step1)
{
	while (count) {
		if (dst[0] > src[0])
			dst[0] = src[0];
		PADD(src, step1);
		++dst;
		--count;
	}
}

#endif

