/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

/** \file
 * Bitmap.
 */

#ifndef __BITMAP_H
#define __BITMAP_H

#include "endianrw.h"
#include "extra.h"
#include "rgb.h"
#include "fz.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bitmap.
 */
typedef struct adv_bitmap_struct {
	unsigned size_x; /**< Width of the bitmap. */
	unsigned size_y; /**< height of the bitmap. */
	unsigned bytes_per_pixel; /**< Bytes per pixel of the bitmap. */
	unsigned bytes_per_scanline; /**< Bytes per scanline of the bitmap. */
	uint8* ptr; /**< Pointer at the first pixel. */
	uint8* heap; /**< Pointer at the allocated data. */
} adv_bitmap;

/** \addtogroup BitMap */
/*@{*/

adv_bitmap* adv_bitmap_alloc(unsigned x, unsigned y, unsigned bit);
adv_bitmap* adv_bitmap_dup(adv_bitmap* src);
adv_bitmap* adv_bitmap_import(unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline);
adv_bitmap* adv_bitmappalette_import(adv_color_rgb* rgb, unsigned* rgb_max, unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline, unsigned char* pal_ptr, unsigned pal_size);
void adv_bitmap_free(adv_bitmap* bmp);

/**
 * Get the pointer of a bitmap line.
 * \param bmp Bitmap to draw.
 * \param y Y.
 * \return Pointer at the first pixel of the specified line.
 */
static inline uint8* adv_bitmap_line(adv_bitmap* bmp, unsigned y)
{
	return bmp->ptr + y * bmp->bytes_per_scanline;
}

/**
 * Get the pointer of a bitmap pixel.
 * \param bmp Bitmap to draw.
 * \param x X.
 * \param y Y.
 * \return Pointer at the first pixel of the specified line.
 */
static inline uint8* adv_bitmap_pixel(adv_bitmap* bmp, unsigned x, unsigned y)
{
	return bmp->ptr + x * bmp->bytes_per_pixel + y * bmp->bytes_per_scanline;
}

/**
 * Put a pixel in a bitmap.
 * \param bmp Bitmap to write.
 * \param x X.
 * \param y Y.
 * \param v Pixel value.
 */
static inline void adv_bitmap_pixel_put(adv_bitmap* bmp, int x, int y, unsigned v)
{
	if (x >= 0 && x < bmp->size_x && y >= 0 && y < bmp->size_y)
		cpu_uint_write(adv_bitmap_pixel(bmp, x, y), bmp->bytes_per_pixel, v);
}

/**
 * Get a pixel in a bitmap.
 * \param bmp Bitmap to read.
 * \param x X.
 * \param y Y.
 * \param v Pixel value.
 */
static inline unsigned adv_bitmap_pixel_get(adv_bitmap* bmp, unsigned x, unsigned y)
{
	if (x >= 0 && x < bmp->size_x && y >= 0 && y < bmp->size_y)
		return cpu_uint_read(adv_bitmap_pixel(bmp, x, y), bmp->bytes_per_pixel);
	else
		return 0;
}

adv_bitmap* adv_bitmap_resize(adv_bitmap* bmp, unsigned x, unsigned y, unsigned src_dx, unsigned src_dy, unsigned dst_dx, unsigned dst_dy, unsigned orientation);
adv_bitmap* adv_bitmap_resample(adv_bitmap* src, unsigned x, unsigned y, unsigned src_dx, unsigned src_dy, unsigned dst_dx, unsigned dst_dy, unsigned orientation_mask, adv_color_def def);
void adv_bitmap_cutoff(adv_bitmap* bitmap, unsigned* cx, unsigned* cy);

/** \name Orientation
 * Orientation operation on a bitmap.
 */
/*@{*/
#define ORIENTATION_FLIP_XY 0x01 /**< Swap the X and Y axes. */
#define ORIENTATION_MIRROR_X 0x02 /**< Mirror on the X axe. */
#define ORIENTATION_MIRROR_Y 0x04 /**< Mirror on the Y axe. */

void adv_bitmap_orientation(adv_bitmap* bmp, unsigned orientation_mask);
/*@}*/

/**
 * Number of bit for channel in the color reduction.
 */
#define REDUCE_COLOR_BIT 4U

/**
 * Size of the conversion table for the color reduction.
 */
#define REDUCE_INDEX_MAX (1U << (3*REDUCE_COLOR_BIT))

unsigned adv_bitmap_reduce(unsigned* convert, adv_color_rgb* palette, unsigned size, const adv_bitmap* bmp);

void adv_bitmap_cvt_reduce_24to8idx(adv_bitmap* dst, adv_bitmap* src, unsigned* convert_map);

void adv_bitmap_cvt_rgb(adv_bitmap* dst, adv_color_def dst_def, adv_bitmap* src, adv_color_def src_def);
void adv_bitmap_cvt_palette(adv_bitmap* dst, adv_bitmap* src, unsigned* color_map);

void adv_bitmap_put(adv_bitmap* dst, int x, int y, adv_bitmap* src);
void adv_bitmap_clear(adv_bitmap* dst, int x, int y, int dx, int dy, unsigned color);
adv_bitmap* adv_bitmap_load(adv_color_rgb* rgb, unsigned* rgb_max, adv_fz* f);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


