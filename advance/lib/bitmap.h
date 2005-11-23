/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

adv_bitmap* adv_bitmap_alloc(unsigned x, unsigned y, unsigned bytes_per_pixel);
adv_bitmap* adv_bitmap_dup(adv_bitmap* src);
void adv_bitmap_move(adv_bitmap* dst, adv_bitmap* src);
adv_bitmap* adv_bitmap_import_rgb(unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline);
adv_bitmap* adv_bitmap_import_palette(adv_color_rgb* rgb, unsigned* rgb_max, unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline, unsigned char* pal_ptr, unsigned pal_size);
void adv_bitmap_free(adv_bitmap* bmp);

/**
 * Get the pointer of a bitmap line.
 * The specified coords are not clipped.
 * \param bmp Bitmap to draw.
 * \param y Y.
 * \return Pointer at the first pixel of the specified line.
 */
static inline uint8* adv_bitmap_line(adv_bitmap* bmp, int y)
{
	return bmp->ptr + y * (int)bmp->bytes_per_scanline;
}

/**
 * Get the pointer of a bitmap pixel.
 * The specified coords are not clipped.
 * \param bmp Bitmap to draw.
 * \param x X.
 * \param y Y.
 * \return Pointer at the first pixel of the specified line.
 */
static inline uint8* adv_bitmap_pixel(const adv_bitmap* bmp, int x, int y)
{
	return bmp->ptr + x * (int)bmp->bytes_per_pixel + y * (int)bmp->bytes_per_scanline;
}

/**
 * Put a pixel in a bitmap.
 * The specified pixel is clipped if required.
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
 * The specified pixel is clipped if required.
 * \param bmp Bitmap to read.
 * \param x X.
 * \param y Y.
 */
static inline unsigned adv_bitmap_pixel_get(adv_bitmap* bmp, int x, int y)
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
#define ADV_ORIENTATION_FLIP_XY 0x01 /**< Swap the X and Y axes. */
#define ADV_ORIENTATION_FLIP_X 0x02 /**< Mirror on the X axe. */
#define ADV_ORIENTATION_FLIP_Y 0x04 /**< Mirror on the Y axe. */
#define ADV_ORIENTATION_ROT0 0
#define ADV_ORIENTATION_ROT90 (ADV_ORIENTATION_FLIP_XY | ADV_ORIENTATION_FLIP_X)
#define ADV_ORIENTATION_ROT180 (ADV_ORIENTATION_FLIP_X | ADV_ORIENTATION_FLIP_Y)
#define ADV_ORIENTATION_ROT270 (ADV_ORIENTATION_FLIP_XY | ADV_ORIENTATION_FLIP_Y)

unsigned adv_orientation_rev(unsigned orientation_mask);
void adv_bitmap_orientation(adv_bitmap* bmp, unsigned orientation_mask);
/*@}*/

adv_bitmap* adv_bitmap_cvt_rgbpalette(adv_color_rgb* rgb_map, unsigned* rgb_max, adv_bitmap* src, adv_color_def src_def);
adv_bitmap* adv_bitmap_cvt_palette(adv_bitmap* src, unsigned* color_map);
adv_bitmap* adv_bitmap_cvt_rgb(adv_color_def dst_def, adv_bitmap* src, adv_color_def src_def);
adv_bitmap* adv_bitmap_cvt_palettergb(adv_color_def dst_def, adv_bitmap* src, adv_color_rgb* rgb_map, unsigned rgb_max);

void adv_bitmap_clear(adv_bitmap* dst, int x, int y, int dx, int dy, unsigned color);

void adv_bitmap_box(adv_bitmap* dst, int x, int y, int dx, int dy, unsigned border, unsigned color);
void adv_bitmap_put(adv_bitmap* dst, int dst_x, int dst_y, const adv_bitmap* src_ptr, int src_x, int src_y, int src_dx, int src_dy);

void adv_bitmap_put_alphaback(adv_bitmap* dst, int dst_x, int dst_y, adv_color_def dst_color_def, const adv_bitmap* back, int back_x, int back_y, const adv_bitmap* src, int src_x, int src_y, int src_dx, int src_dy, adv_color_def src_color_def);

static inline void adv_bitmap_put_alpha(adv_bitmap* dst, int dst_x, int dst_y, adv_color_def dst_color_def, const adv_bitmap* src, int src_x, int src_y, int src_dx, int src_dy, adv_color_def src_color_def)
{
	adv_bitmap_put_alphaback(dst, dst_x, dst_y, dst_color_def, dst, dst_x, dst_y, src, src_x, src_y, src_dx, src_dy, src_color_def);
}

void adv_bitmap_clear_alphaback(adv_bitmap* dst, int dst_x, int dst_y, adv_color_def dst_color_def, adv_bitmap* back, int back_x, int back_y, adv_color_rgb src, int src_dx, int src_dy);

static inline void adv_bitmap_clear_alpha(adv_bitmap* dst, int dst_x, int dst_y, adv_color_def dst_color_def, adv_color_rgb src, int src_dx, int src_dy)
{
	adv_bitmap_clear_alphaback(dst, dst_x, dst_y, dst_color_def, dst, dst_x, dst_y, src, src_dx, src_dy);
}

adv_bitmap* adv_bitmap_load_png(adv_color_rgb* rgb, unsigned* rgb_max, adv_fz* f);
adv_bitmap* adv_bitmap_load_png_rgb(adv_fz* f, adv_color_def def);
adv_bitmap* adv_bitmap_load_png_palette(adv_color_rgb* rgb_map, unsigned* rgb_max, adv_fz* f);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


