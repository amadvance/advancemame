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

#include "icon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	uint16 reserved __attribute__ ((packed));
	uint16 type __attribute__ ((packed));
	uint16 count __attribute__ ((packed));
} __attribute__ ((packed));

/**
 * ICON image entry.
 */
struct icon_entry_t {
	uint8 width __attribute__ ((packed));
	uint8 height __attribute__ ((packed));
	uint8 color __attribute__ ((packed));
	uint8 reserved __attribute__ ((packed));
	uint16 planes __attribute__ ((packed));
	uint16 bit __attribute__ ((packed));
	uint32 size __attribute__ ((packed));
	uint32 offset __attribute__ ((packed));
} __attribute__ ((packed));

/* ICON rgb entry */
struct icon_rgb_t {
	uint8 blue __attribute__ ((packed));
	uint8 green __attribute__ ((packed));
	uint8 red __attribute__ ((packed));
	uint8 reserved __attribute__ ((packed));
} __attribute__ ((packed));

/* ICON bitmap header */
struct bitmap_header_t {
	uint32 size __attribute__ ((packed)); 
	uint32 width __attribute__ ((packed)); 
	uint32 height __attribute__ ((packed)); 
	uint16 planes __attribute__ ((packed)); 
	uint16 bit __attribute__ ((packed)); 
	uint32 compression __attribute__ ((packed)); 
	uint32 image_size __attribute__ ((packed)); 
	uint32 x_pels __attribute__ ((packed));
	uint32 y_pels __attribute__ ((packed));
	uint32 color_used __attribute__ ((packed));
	uint32 color_important __attribute__ ((packed));
} __attribute__ ((packed));

/**
 * Load a .ico file in a bitmap.
 * Only the 16 and 256 color format are supported.
 * \param f File to load.
 * \param rgb Where to put the palette information. it must point to a vector of 256 elements.
 * \param rgb_max Where to put the number of palette entries.
 * \param bitmap_mask Where to put the mask bitmap. 
 * \return The loaded bitmap or 0 on error.
 */
adv_bitmap* icon_load(adv_fz* f, adv_color_rgb* rgb, unsigned* rgb_max, adv_bitmap** bitmap_mask)
{
	adv_bitmap* bitmap;
	struct icon_header_t header;
	struct icon_entry_t* entry;
	int i;

	if (fzread(&header, sizeof(struct icon_header_t), 1, f)!=1) /* ENDIAN */
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

	if (fzread(entry, sizeof(struct icon_entry_t), header.count, f)!=header.count) /* ENDIAN */
		goto out_entry;

	for(i=0;i<header.count;++i) {
		struct bitmap_header_t bitmap_header;
		uint32 size;
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

		if (fzread(&size, sizeof(uint32), 1, f)!=1) /* ENDIAN */
			goto out_entry;

		memset(&bitmap_header, 0, sizeof(struct bitmap_header_t));
		if (size > sizeof(struct bitmap_header_t))
			bitmap_header.size = sizeof(struct bitmap_header_t);
		else
			bitmap_header.size = size;
		
		if (fzread(&bitmap_header.width, bitmap_header.size - 4, 1, f)!=1) /* ENDIAN */
			goto out_entry;

		if (size > bitmap_header.size)
			fzseek(f, size - bitmap_header.size, SEEK_CUR);

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
			if (fzread(&color, sizeof(struct icon_rgb_t), 1, f)!=1)
				goto out_entry;
			rgb[j].red = color.red;
			rgb[j].green = color.green;
			rgb[j].blue = color.blue;
		}
		*rgb_max = colors;

		bitmap = bitmap_alloc(entry[i].width, entry[i].height, 8);
		if (!bitmap)
			goto out_entry;

		*bitmap_mask = bitmap_alloc(entry[i].width, entry[i].height, 8);
		if (!*bitmap_mask)
			goto out_bitmap;

		if (colors == 256) {
			/* read bitmap xor data (8 bit per pixel) */
			for(y=0;y<bitmap->size_y;++y) {
				uint8* line = bitmap_line(bitmap, bitmap->size_y - y - 1);
				if (fzread(line, bitmap->size_x*bitmap->bytes_per_pixel, 1, f)!=1)
					goto out_bitmap_mask;
			}
		} else if (colors == 16) {
			/* read bitmap xor data (4 bit per pixel) */
			for(y=0;y<bitmap->size_y;++y) {
				int k;
				uint8* line = bitmap_line(bitmap, bitmap->size_y - y - 1);
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
			uint8* line = bitmap_line(*bitmap_mask, bitmap->size_y - y - 1);
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
	bitmap_free(*bitmap_mask);
out_bitmap:
	bitmap_free(bitmap);
out_entry:
	free(entry);
out:
	return 0;
}
