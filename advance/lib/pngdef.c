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

#include "portable.h"

#include "pngdef.h"
#include "rgb.h"
#include "png.h"
#include "mode.h"
#include "endianrw.h"

/**
 * Get the color definition of a PNG image.
 * \param bytes_per_pixel Size of the pixel.
 */
adv_color_def adv_png_color_def(unsigned bytes_per_pixel)
{
	adv_color_def def;
#ifdef USE_LSB
	if (bytes_per_pixel == 3 || bytes_per_pixel == 4)
		def = color_def_make_rgb_from_sizelenpos(bytes_per_pixel, 8, 0, 8, 8, 8, 16);
	else
		def = color_def_make(adv_color_type_palette);
#else
	if (bytes_per_pixel == 3)
		def = color_def_make_rgb_from_sizelenpos(bytes_per_pixel, 8, 16, 8, 8, 8, 0);
	else if (bytes_per_pixel == 4)
		def = color_def_make_rgb_from_sizelenpos(bytes_per_pixel, 8, 24, 8, 16, 8, 8);
	else
		def = color_def_make(adv_color_type_palette);
#endif
	return def;
}

/**
 * Check if a color def is supported by PNG format.
 */
adv_bool adv_png_color_def_is_valid(adv_color_def def)
{
	unsigned pixel = color_def_bytes_per_pixel_get(def);
	return def == adv_png_color_def(pixel);
}

static adv_error png_write_raw_pal(
	unsigned pix_width, unsigned pix_height, unsigned pix_pixel,
	const unsigned char* pix_ptr, int pix_pixel_pitch, int pix_scanline_pitch,
	adv_color_rgb* rgb_ptr, unsigned rgb_max,
	adv_bool fast,
	adv_fz* f, unsigned* count
)
{
	uint8 palette[3*256];
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;

	if (rgb_max > 256)
		goto err;

	for (i=0;i<rgb_max;++i) {
		palette[i*3] = rgb_ptr[i].red;
		palette[i*3+1] = rgb_ptr[i].green;
		palette[i*3+2] = rgb_ptr[i].blue;
	}

	i_size = pix_height * pix_width;
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<pix_height;++i) {
		for(j=0;j<pix_width;++j) {
			adv_pixel pixel;

			pixel = cpu_uint_read(pix_ptr, pix_pixel);

			p[0] = pixel;

			p += 1;
			pix_ptr += pix_pixel_pitch;
		}
		pix_ptr += pix_scanline_pitch - pix_pixel_pitch * pix_width;
	}

	if (adv_png_write_raw(pix_width, pix_height, 1, i_ptr, 1, 1 * pix_width, palette, rgb_max * 3, 0, 0, fast, f, count) != 0) {
		goto err_free;
	}

	free(i_ptr);
	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

static adv_error png_write_raw_paltorgb(
	unsigned pix_width, unsigned pix_height, unsigned pix_pixel,
	const unsigned char* pix_ptr, int pix_pixel_pitch, int pix_scanline_pitch,
	adv_color_rgb* rgb_ptr, unsigned rgb_max,
	adv_bool fast,
	adv_fz* f, unsigned* count)
{
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;

	i_size = pix_height * (pix_width * 3);
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<pix_height;++i) {
		for(j=0;j<pix_width;++j) {
			adv_pixel pixel;

			pixel = cpu_uint_read(pix_ptr, pix_pixel);

			p[0] = rgb_ptr[pixel].red;
			p[1] = rgb_ptr[pixel].green;
			p[2] = rgb_ptr[pixel].blue;

			p += 3;
			pix_ptr += pix_pixel_pitch;
		}
		pix_ptr += pix_scanline_pitch - pix_pixel_pitch * pix_width;
	}

	if (adv_png_write_raw(pix_width, pix_height, 3, i_ptr, 3, 3 * pix_width, 0, 0, 0, 0, fast, f, count) != 0) {
		goto err_free;
	}

	free(i_ptr);
	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

static adv_error png_write_raw_rgb(
	unsigned pix_width, unsigned pix_height, adv_color_def pix_def,
	const unsigned char* pix_ptr, int pix_pixel_pitch, int pix_scanline_pitch,
	adv_bool fast,
	adv_fz* f, unsigned* count)
{
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;
	unsigned pix_pixel = color_def_bytes_per_pixel_get(pix_def);
	union adv_color_def_union def;
	int red_shift, green_shift, blue_shift;
	unsigned red_mask, green_mask, blue_mask;

	i_size = pix_height * (pix_width * 3);
	i_ptr = (uint8*)malloc(i_size);
	if (!i_ptr)
		goto err;

	def.ordinal = pix_def;
	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	p = i_ptr;
	for(i=0;i<pix_height;++i) {
		for(j=0;j<pix_width;++j) {
			adv_pixel pixel;

			pixel = cpu_uint_read(pix_ptr, pix_pixel);

			p[0] = rgb_nibble_extract(pixel, red_shift, red_mask);
			p[1] = rgb_nibble_extract(pixel, green_shift, green_mask);
			p[2] = rgb_nibble_extract(pixel, blue_shift, blue_mask);

			p += 3;
			pix_ptr += pix_pixel_pitch;
		}

		pix_ptr += pix_scanline_pitch - pix_pixel_pitch * pix_width;
	}

	if (adv_png_write_raw(pix_width, pix_height, 3, i_ptr, 3, 3 * pix_width, 0, 0, 0, 0, fast, f, count) != 0) {
		goto err_free;
	}

	free(i_ptr);
	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

adv_error adv_png_write_raw_def(
	unsigned pix_width, unsigned pix_height, adv_color_def pix_def,
	const unsigned char* pix_ptr, int pix_pixel_pitch, int pix_scanline_pitch,
	adv_color_rgb* rgb_ptr, unsigned rgb_max,
	adv_bool fast,
	adv_fz* f, unsigned* count)
{
	adv_color_type type;
	unsigned pixel;

	type = color_def_type_get(pix_def);
	pixel = color_def_bytes_per_pixel_get(pix_def);

	if (type == adv_color_type_palette) {
		if (rgb_max <= 256) {
			/* write palette image */
			return png_write_raw_pal(pix_width, pix_height, pixel, pix_ptr, pix_pixel_pitch, pix_scanline_pitch, rgb_ptr, rgb_max, fast, f, count);
		} else {
			/* convert from palette to 24 bit rgb */
			return png_write_raw_paltorgb(pix_width, pix_height, pixel, pix_ptr, pix_pixel_pitch, pix_scanline_pitch, rgb_ptr, rgb_max, fast, f, count);
		}
	} else if (type == adv_color_type_rgb) {
		if (adv_png_color_def_is_valid(pix_def)) {
			/* write rgb image */
			return adv_png_write_raw(pix_width, pix_height, pixel, pix_ptr, pix_pixel_pitch, pix_scanline_pitch, 0, 0, 0, 0, fast, f, count);
		} else {
			/* convert from generic rgb to 24 bit rgb */
			return png_write_raw_rgb(pix_width, pix_height, pix_def, pix_ptr, pix_pixel_pitch, pix_scanline_pitch, fast, f, count);
		}
	} else {
		return -1;
	}
}

/**
 * Save a complete PNG image, eventually converting it.
 * \param pix_width Image width.
 * \param pix_height Image height.
 * \param pix_def Image color definition.
 * \param pix_ptr Pointer at the start of the image data.
 * \param pix_pixel_pitch Pitch for the next pixel.
 * \param pix_scanline_pitch Pitch for the next scanline.
 * \param rgn_ptr Plette data pointer. Use 0 for RGB image.
 * \param rgb_size Palette size in number of colors. Use 0 for RGB image.
 * \param f File to write.
 * \param count Pointer at the incremental counter of bytes written. Use 0 for disabling it.
 */
adv_error adv_png_write_def(
	unsigned pix_width, unsigned pix_height, adv_color_def pix_def,
	const unsigned char* pix_ptr, int pix_pixel_pitch, int pix_scanline_pitch,
	adv_color_rgb* rgb_ptr, unsigned rgb_max,
	adv_bool fast,
	adv_fz* f, unsigned* count)
{
	if (adv_png_write_signature(f, count) != 0) {
		return -1;
	}

	return adv_png_write_raw_def(
		pix_width, pix_height, pix_def,
		pix_ptr, pix_pixel_pitch, pix_scanline_pitch,
		rgb_ptr, rgb_max,
		fast,
		f, count
	);
}

