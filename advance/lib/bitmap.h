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
 */

#ifndef __BITMAP_H
#define __BITMAP_H

#include "advstd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bitmap {
	unsigned size_x;
	unsigned size_y;
	unsigned bytes_per_pixel;
	unsigned bytes_per_scanline;
	uint8* ptr; /* pointer at the first pixel */
	uint8* heap; /* pointer at the allocated data */
};

struct bitmap* bitmap_alloc(unsigned x, unsigned y, unsigned bit);
struct bitmap* bitmap_dup(struct bitmap* src);
struct bitmap* bitmap_import(unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline);
struct bitmap* bitmappalette_import(video_color* rgb, unsigned* rgb_max, unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline, unsigned char* pal_ptr, unsigned pal_size);
void bitmap_free(struct bitmap* bmp);
uint8* bitmap_line(struct bitmap* bmp, unsigned line);
void bitmap_putpixel(struct bitmap* bmp, unsigned x, unsigned y, unsigned v);
struct bitmap* bitmap_resize(struct bitmap* bmp, unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned sx, unsigned sy, unsigned orientation);
void bitmap_cutoff(struct bitmap* bitmap, unsigned* _cx, unsigned* _cy);
struct bitmap* bitmap_addborder(struct bitmap* bmp, unsigned x0, unsigned x1, unsigned y0, unsigned y1, unsigned color);

// Orientation flags
#define ORIENTATION_FLIP_XY 0x01
#define ORIENTATION_MIRROR_X 0x02
#define ORIENTATION_MIRROR_Y 0x04

void bitmap_orientation(struct bitmap* bmp, unsigned orientation_mask);

#define BITMAP_COLOR_BIT 4U
#define BITMAP_INDEX_MAX (1U << (3*BITMAP_COLOR_BIT))

#define BITMAP_INDEX_TO_RED(i) ((i << 4) & 0xF0)
#define BITMAP_INDEX_TO_GREEN(i) (i & 0xF0)
#define BITMAP_INDEX_TO_BLUE(i) ((i >> 4) & 0xF0)

#define BITMAP_RED_TO_INDEX(i) ((i >> 4) & 0xF)
#define BITMAP_GREEN_TO_INDEX(i) (i & 0xF0)
#define BITMAP_BLUE_TO_INDEX(i) ((((unsigned)i) << 4) & 0xF00)
#define BITMAP_COLOR_TO_INDEX(r,g,b) (BITMAP_RED_TO_INDEX(r) | BITMAP_GREEN_TO_INDEX(g) | BITMAP_BLUE_TO_INDEX(b))

unsigned bitmap_reduction(unsigned* convert, video_color* palette, unsigned size, const struct bitmap* bmp);

void bitmap_cvt_8to8(struct bitmap* dst, struct bitmap* src, unsigned* color_map);
void bitmap_cvt_24to8rgb(struct bitmap* dst, struct bitmap* src);
void bitmap_cvt_24to8idx(struct bitmap* dst, struct bitmap* src, unsigned* convert_map);
void bitmap_cvt_8to16(struct bitmap* dst, struct bitmap* src, unsigned* color_map);
void bitmap_cvt_24to16(struct bitmap* dst, struct bitmap* src);
void bitmap_cvt_32to24(struct bitmap* dst, struct bitmap* src);
void bitmap_cvt_8to32(struct bitmap* dst, struct bitmap* src, unsigned* color_map);
void bitmap_cvt_24to32(struct bitmap* dst, struct bitmap* src);

#ifdef __cplusplus
}
#endif

#endif
