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

#include "pcx.h"

/**
 * Header of the PCX image.
 */
struct pcx_header_t {
	unsigned manufacturer; /**< Constant Flag, 10 = ZSoft .pcx. */
	unsigned version; /**< Version information. */
	unsigned encoding; /**< 1 = .PCX run length encoding. */
	unsigned bits_per_pixel; /**< Number of bits to represent a pixel (per Plane) - 1, 2, 4, or 8. */
	unsigned x_min; /**< Image Dimensions. */
	unsigned y_min;
	unsigned x_max;
	unsigned y_max;
	unsigned hdpi; /**< Horizontal Resolution of image in DPI. */
	unsigned vdpi; /**< Vertical Resolution of image in DPI. */
	unsigned char colornmap[48]; /**< Color palette setting. */
	unsigned reserved;
	unsigned planes; /**< Number of color planes. */
	unsigned bytes_per_line; /**< Number of bytes to allocate for a scanline plane.  MUST be an EVEN number.  Do NOT calculate from Xmax-Xmin. */
	unsigned palette_info; /**< 1 = Color/BW, 2 = Grayscale (ignored in PB IV/ IV +). */
	unsigned h_screen_size; /**< Horizontal screen size in pixels. */
	unsigned v_screen_size; /**< Vertical screen size in pixels. */
	unsigned char filler[54];
};

static adv_error pcx_header_read(adv_fz* fz, struct pcx_header_t* header)
{
	if (le_uint8_fzread(fz, &header->manufacturer) != 0)
		return -1;
	if (le_uint8_fzread(fz, &header->version) != 0)
		return -1;
	if (le_uint8_fzread(fz, &header->encoding) != 0)
		return -1;
	if (le_uint8_fzread(fz, &header->bits_per_pixel) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->x_min) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->y_min) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->x_max) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->y_max) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->hdpi) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->vdpi) != 0)
		return -1;
	if (fzread(&header->colornmap, 48, 1, fz) != 1)
		return -1;
	if (le_uint8_fzread(fz, &header->reserved) != 0)
		return -1;
	if (le_uint8_fzread(fz, &header->planes) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->bytes_per_line) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->palette_info) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->h_screen_size) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->v_screen_size) != 0)
		return -1;
	if (fzread(&header->filler, 54, 1, fz) != 1)
		return -1;
	return 0;
}

struct pcx_decode_state {
	unsigned count;
	unsigned char value;
};

static void pcx_decode(uint8* buffer, unsigned size, adv_fz* f, struct pcx_decode_state* state, unsigned delta)
{
	while (size) {
		unsigned run;
		if (!state->count) {
			uint8 c = fzgetc(f);
			if ((c & 0xC0) == 0xC0) {
				state->count = (c & 0x3F);
				state->value = fzgetc(f);
			} else {
				state->count = 1;
				state->value = c;
			}
		}
		run = state->count;
		if (run > size)
			run = size;
		state->count -= run;
		size -= run;
		while (run) {
			*buffer = state->value;
			buffer += delta;
			--run;
		}
	}
}

static void pcx_ignore(unsigned size, adv_fz* f, struct pcx_decode_state* state)
{
	while (size) {
		unsigned run;
		if (!state->count) {
			uint8 c = fzgetc(f);
			if ((c & 0xC0) == 0xC0) {
				state->count = (c & 0x3F);
				state->value = fzgetc(f);
			} else {
				state->count = 1;
				state->value = c;
			}
		}
		run = state->count;
		if (run > size)
			run = size;
		state->count -= run;
		size -= run;
	}
}

/**
 * Load a .pcx file in a bitmap.
 * Only the 8 and 24 bits format are supported.
 * \param f File to load.
 * \param rgb Where to put the palette information. it must point to a vector of 256 elements.
 * \param rgb_max Where to put the number of palette entries.
 * \return The loaded bitmap or 0 on error.
 */
adv_bitmap* adv_bitmap_load_pcx(adv_color_rgb* rgb, unsigned* rgb_max, adv_fz* f)
{
	struct pcx_header_t h;
	adv_bitmap* bitmap;
	unsigned width, height, depth;

	if (pcx_header_read(f, &h)!=0) {
		goto out;
	}

	/* limitations */
	if (h.bits_per_pixel != 8) {
		goto out;
	}

	if (h.planes != 1 && h.planes != 3) {
		goto out;
	}

	width = h.x_max - h.x_min + 1;
	height = h.y_max - h.y_min + 1;
	depth = h.planes;

	bitmap = adv_bitmap_alloc(width, height, depth);
	if (!bitmap) {
		goto out;
	}

	if (depth == 1) {
		unsigned i;
		unsigned y;

		for(y=0;y<height;++y) {
			struct pcx_decode_state state;
			uint8* dst_off = adv_bitmap_line(bitmap, y);
			state.count = 0;
			pcx_decode(dst_off, width, f, &state, 1);
			pcx_ignore(h.bytes_per_line - width, f, &state);
			if (state.count!=0)
				goto out_bitmap;
		}

		if (fzgetc(f)!=12)
			goto out_bitmap;

		for (i=0;i<256;++i) {
			rgb[i].red = fzgetc(f);
			rgb[i].green = fzgetc(f);
			rgb[i].blue = fzgetc(f);
		}
		*rgb_max = 256;
	} else {
		unsigned y;
		for(y=0;y<height;++y) {
			struct pcx_decode_state state;
			uint8* dst_off = adv_bitmap_line(bitmap, y);
			state.count = 0;
			pcx_decode(dst_off, width, f, &state, 3);
			pcx_ignore(h.bytes_per_line - width, f, &state);
			pcx_decode(dst_off + 1, width, f, &state, 3);
			pcx_ignore(h.bytes_per_line - width, f, &state);
			pcx_decode(dst_off + 2, width, f, &state, 3);
			pcx_ignore(h.bytes_per_line - width, f, &state);
			if (state.count!=0)
				goto out_bitmap;
		}
	}

	return bitmap;

out_bitmap:
	adv_bitmap_free(bitmap);
out:
	return 0;
}
