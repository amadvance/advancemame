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

#include "icon.h"

/*
Icon format

HEADER
n*ENTRY
n*(
	BITMAP_HEADER
	k*RGB
	XORDATA
	ANDDATA
)
*/

/**
 * ICON file header.
 */
struct icon_header_t {
	unsigned reserved;
	unsigned type;
	unsigned count;
};

static adv_error icon_header_read(adv_fz* fz, struct icon_header_t* header)
{
	if (le_uint16_fzread(fz, &header->reserved) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->type) != 0)
		return -1;
	if (le_uint16_fzread(fz, &header->count) != 0)
		return -1;
	return 0;
}

/**
 * ICON image entry.
 */
struct icon_entry_t {
	unsigned width;
	unsigned height;
	unsigned color;
	unsigned reserved;
	unsigned planes;
	unsigned bit;
	unsigned size;
	unsigned offset;
};

static adv_error icon_entry_read(adv_fz* fz, struct icon_entry_t* entry)
{
	if (le_uint8_fzread(fz, &entry->width) != 0)
		return -1;
	if (le_uint8_fzread(fz, &entry->height) != 0)
		return -1;
	if (le_uint8_fzread(fz, &entry->color) != 0)
		return -1;
	if (le_uint8_fzread(fz, &entry->reserved) != 0)
		return -1;
	if (le_uint16_fzread(fz, &entry->planes) != 0)
		return -1;
	if (le_uint16_fzread(fz, &entry->bit) != 0)
		return -1;
	if (le_uint32_fzread(fz, &entry->size) != 0)
		return -1;
	if (le_uint32_fzread(fz, &entry->offset) != 0)
		return -1;
	return 0;
}

/**
 * ICON rgb entry.
 */
struct icon_rgb_t {
	unsigned blue;
	unsigned green;
	unsigned red;
	unsigned reserved;
};

static adv_error icon_rgb_read(adv_fz* fz, struct icon_rgb_t* rgb)
{
	if (le_uint8_fzread(fz, &rgb->blue) != 0)
		return -1;
	if (le_uint8_fzread(fz, &rgb->green) != 0)
		return -1;
	if (le_uint8_fzread(fz, &rgb->red) != 0)
		return -1;
	if (le_uint8_fzread(fz, &rgb->reserved) != 0)
		return -1;
	return 0;
}

/**
 * ICON bitmap header.
 */
struct icon_bitmap_header_t {
	unsigned size; 
	unsigned width; 
	unsigned height; 
	unsigned planes; 
	unsigned bit; 
	unsigned compression; 
	unsigned image_size; 
	unsigned x_pels;
	unsigned y_pels;
	unsigned color_used;
	unsigned color_important;
};

static adv_error icon_bitmap_header_read(adv_fz* fz, struct icon_bitmap_header_t* header)
{
	unsigned size;

	memset(header, 0, sizeof(struct icon_bitmap_header_t));

	if (le_uint32_fzread(fz, &size) != 0)
		return -1;

	header->size = size;
	if (header->size > 40)
		header->size = 40;

	do {
		if (size == 4)
			break;
		if (le_uint32_fzread(fz, &header->width) != 0)
			return -1;
		if (size == 8)
			break;
		if (le_uint32_fzread(fz, &header->height) != 0)
			return -1;
		if (size == 12)
			break;
		if (le_uint16_fzread(fz, &header->planes) != 0)
			return -1;
		if (size == 14)
			break;
		if (le_uint16_fzread(fz, &header->bit) != 0)
			return -1;
		if (size == 16)
			break;
		if (le_uint32_fzread(fz, &header->compression) != 0)
			return -1;
		if (size == 20)
			break;
		if (le_uint32_fzread(fz, &header->image_size) != 0)
			return -1;
		if (size == 24)
			break;
		if (le_uint32_fzread(fz, &header->x_pels) != 0)
			return -1;
		if (size == 28)
			break;
		if (le_uint32_fzread(fz, &header->y_pels) != 0)
			return -1;
		if (size == 32)
			break;
		if (le_uint32_fzread(fz, &header->color_used) != 0)
			return -1;
		if (size == 36)
			break;
		if (le_uint32_fzread(fz, &header->color_important) != 0)
			return -1;
	} while (0);

	if (size > header->size) {
		if (fzseek(fz, size - header->size, SEEK_CUR) != 0)
			return -1;
	}

	return 0;
}

/**
 * Load a .ico file in a bitmap.
 * Only the 16 and 256 color formats are supported.
 * \param f File to load.
 * \param rgb Where to put the palette information. it must point to a vector of 256 elements.
 * \param rgb_max Where to put the number of palette entries.
 * \param bitmap_mask Where to put the mask bitmap. 
 * \return The loaded bitmap or 0 on error.
 */
adv_bitmap* adv_bitmap_load_icon(adv_color_rgb* rgb, unsigned* rgb_max, adv_bitmap** bitmap_mask, adv_fz* f)
{
	adv_bitmap* bitmap;
	struct icon_header_t header;
	struct icon_entry_t* entry;
	int i;

	if (icon_header_read(f, &header) != 0)
		goto out;

	if (header.reserved != 0)
		goto out;

	if (header.type != 1)
		goto out;

	if (header.count < 1)
		goto out;

	entry = malloc(header.count * sizeof(struct icon_entry_t));
	if (!entry)
		goto out;

	for(i=0;i<header.count;++i) {
		if (icon_entry_read(f, entry + i)!=0)
			goto out_entry;
	}

	for(i=0;i<header.count;++i) {
		struct icon_bitmap_header_t bitmap_header;
		unsigned j, y;
		unsigned colors;

		if (entry[i].color == 0) {
			colors = 256;
			if (entry[i].bit != 8 && entry[i].bit != 0) /* 0 is out of standard but acceptable */
				continue; /* unsupported */
		} else if (entry[i].color == 16) {
			colors = 16;
			if (entry[i].bit != 4 && entry[i].bit != 0) /* 0 is out of standard but acceptable */
				continue; /* unsupported */
		} else {
			continue; /* unsupported */
		}

		if (entry[i].planes != 1 && entry[i].planes != 0) /* 0 is out of standard but acceptable */
			continue; /* unsupported */

		if (fzseek(f, entry[i].offset, SEEK_SET)!=0)
			goto out_entry;

		if (icon_bitmap_header_read(f, &bitmap_header) != 0)
			goto out_entry;

		if (colors == 256) {
			if (bitmap_header.bit != 8)
				continue; /* unsupported */
		} else if (colors == 16) {
			if (bitmap_header.bit != 4)
				continue; /* unsupported */
		}

		if (bitmap_header.planes != 1)
			continue; /* unsupported */

		if (bitmap_header.compression != 0) 
			continue; /* unsupported */

		for(j=0;j<colors;++j) {
			struct icon_rgb_t color;
			if (icon_rgb_read(f, &color) != 0)
				goto out_entry;
			rgb[j].red = color.red;
			rgb[j].green = color.green;
			rgb[j].blue = color.blue;
		}
		*rgb_max = colors;

		bitmap = adv_bitmap_alloc(entry[i].width, entry[i].height, 1);
		if (!bitmap)
			goto out_entry;

		*bitmap_mask = adv_bitmap_alloc(entry[i].width, entry[i].height, 1);
		if (!*bitmap_mask)
			goto out_bitmap;

		if (colors == 256) {
			/* read bitmap xor data (8 bit per pixel) */
			for(y=0;y<bitmap->size_y;++y) {
				unsigned char* line = adv_bitmap_line(bitmap, bitmap->size_y - y - 1);
				if (fzread(line, bitmap->size_x*bitmap->bytes_per_pixel, 1, f)!=1)
					goto out_bitmap_mask;
			}
		} else if (colors == 16) {
			/* read bitmap xor data (4 bit per pixel) */
			for(y=0;y<bitmap->size_y;++y) {
				int k;
				unsigned char* line = adv_bitmap_line(bitmap, bitmap->size_y - y - 1);
				if (fzread(line, bitmap->size_x / 2, 1, f)!=1)
					goto out_bitmap_mask;
				/* convert */
				for(k=bitmap->size_x-1;k>=0;--k) {
					if (k & 1)
						line[k] = line[k/2] & 0xF;
					else
						line[k] = line[k/2] >> 4;
				}
			}
		}

		/* read bitmap mask data (1 bit per pixel) */
		for(y=0;y<bitmap->size_y;++y) {
			unsigned x ;
			unsigned char* line = adv_bitmap_line(*bitmap_mask, bitmap->size_y - y - 1);
			x = 0;
			while (x < bitmap->size_x) {
				unsigned bc;
				int b = fzgetc(f);
				if (b==EOF)
					goto out_bitmap_mask;
				if (x + 8 > bitmap->size_x)
					bc = bitmap->size_x - x; /* waste unused bit */
				else
					bc = 8;
				x += bc;
				while (bc) {
					if (b & 0x80)
						*line = 0;
					else
						*line = 1;
					b = b << 1;
					++line;
					--bc;
				}
			}
		}

		free(entry);
		return bitmap;
	}

	goto out_entry;

out_bitmap_mask:
	adv_bitmap_free(*bitmap_mask);
out_bitmap:
	adv_bitmap_free(bitmap);
out_entry:
	free(entry);
out:
	return 0;
}
