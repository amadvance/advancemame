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

#include "video.h"
#include "bitmap.h"
#include "endianrw.h"
#include "rgb.h"
#include "slice.h"
#include "filter.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

/**
 * Allocate a bitmap.
 * \param width Width.
 * \param height Height.
 * \param bit Bits per pixel.
 * \return The allocated bitmap or 0 on error.
 */
adv_bitmap* bitmap_alloc(unsigned width, unsigned height, unsigned bit)
{
	adv_bitmap* bmp = (adv_bitmap*)malloc(sizeof(adv_bitmap));
	if (!bmp)
		return 0;

	bmp->size_x = width;
	bmp->size_y = height;
	bmp->bytes_per_pixel = (bit + 7) / 8;
	bmp->bytes_per_scanline = (bmp->size_x * bmp->bytes_per_pixel + 3) & ~0x3;
	bmp->heap = malloc( bmp->bytes_per_scanline * bmp->size_y );
	if (!bmp->heap) {
		free(bmp);
		return 0;
	}
	bmp->ptr = bmp->heap;

	return bmp;
}

/**
 * Duplicate a bitmap.
 * The duplicated bitmap has a normalized bytes per scanline.
 * \param bmp Bitmap to duplicate.
 * \return The allocated bitmap or 0 on error.
 */
adv_bitmap* bitmap_dup(adv_bitmap* bmp)
{
	unsigned i;

	adv_bitmap* r = (adv_bitmap*)malloc(sizeof(adv_bitmap));
	if (!r)
		return 0;

	r->size_x = bmp->size_x;
	r->size_y = bmp->size_y;
	r->bytes_per_pixel = bmp->bytes_per_pixel;
	r->bytes_per_scanline = (r->size_x * r->bytes_per_pixel + 3) & ~0x3;
	r->heap = malloc( r->bytes_per_scanline * r->size_y );
	if (!r->heap) {
		free(r);
		return 0;
	}
	r->ptr = r->heap;

	for(i=0;i<r->size_y;++i)
		memcpy(bitmap_line(r, i), bitmap_line(bmp, i), r->size_x * r->bytes_per_pixel);

	return r;
}

/**
 * Create a bitmap allocated externally.
 * \param width Width.
 * \param height Height.
 * \param pixel Bytes per pixel.
 * \param dat_ptr Pointer at the allocated data.
 * \param dat_size Size of the allocated data.
 * \param ptr Pointer at the first pixel.
 * \param scanline Bytes per scanline.
 */
adv_bitmap* bitmap_import(unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline)
{
	adv_bitmap* bmp = (adv_bitmap*)malloc(sizeof(adv_bitmap));
	assert( bmp );

	bmp->size_x = width;
	bmp->size_y = height;
	bmp->bytes_per_pixel = pixel;
	bmp->bytes_per_scanline = scanline;
	bmp->ptr = ptr;
	bmp->heap = dat_ptr;

	return bmp;
}

/**
 * Create a palettized bitmap allocated externally.
 */
adv_bitmap* bitmappalette_import(adv_color_rgb* rgb, unsigned* rgb_max, unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline, unsigned char* pal_ptr, unsigned pal_size)
{
	if (pixel == 1) {
		unsigned char* p = pal_ptr;
		unsigned n = pal_size / 3;
		unsigned i;
		for(i=0;i<n;++i) {
			rgb[i].red = *p++;
			rgb[i].green = *p++;
			rgb[i].blue = *p++;
			rgb[i].alpha = 0;
		}
		*rgb_max = n;
	} else {
		*rgb_max = 0;
	}

	return bitmap_import(width, height, pixel, dat_ptr, dat_size, ptr, scanline);
}

/**
 * Deallocated a bitmap.
 */
void bitmap_free(adv_bitmap* bmp)
{
	if (bmp)
		free(bmp->heap);
	free(bmp);
}

/**
 * Change the orientation of a bitmap.
 * \param bmp B itmap.
 * \param orientation_mask Subset of the ORIENTATION flags.
 */
void bitmap_orientation(adv_bitmap* bmp, unsigned orientation_mask)
{
	if (orientation_mask & ORIENTATION_FLIP_XY) {
		adv_bitmap* newbmp;

		/* new ptr */
		newbmp = bitmap_alloc(bmp->size_y, bmp->size_x, bmp->bytes_per_pixel * 8);
		assert( newbmp );

		{
			unsigned size_x = newbmp->size_x;
			unsigned size_y = newbmp->size_y;
			unsigned src_bytes_per_scanline = bmp->bytes_per_scanline;
			unsigned dst_bytes_per_scanline = newbmp->bytes_per_scanline;
			uint8* src = (uint8*)bmp->ptr;
			uint8* dst = (uint8*)newbmp->ptr;

			if (bmp->bytes_per_pixel == 1) {
				unsigned y;
				for(y=0;y<size_y;++y) {
					uint8* srcline = src;
					uint8* dstline = dst;
					unsigned x;
					for(x=0;x<size_x;++x) {
						*dstline = *srcline;
						dstline += 1;
						srcline += src_bytes_per_scanline;
					}
					dst += dst_bytes_per_scanline;
					src += 1;
				}
			} else if (bmp->bytes_per_pixel == 2) {
				unsigned y;
				for(y=0;y<size_y;++y) {
					uint8* srcline = src;
					uint8* dstline = dst;
					unsigned x;
					for(x=0;x<size_x;++x) {
						dstline[0] = srcline[0];
						dstline[1] = srcline[1];
						dstline += 2;
						srcline += src_bytes_per_scanline;
					}
					dst += dst_bytes_per_scanline;
					src += 2;
				}
			} else if (bmp->bytes_per_pixel == 3) {
				unsigned y;
				for(y=0;y<size_y;++y) {
					uint8* srcline = src;
					uint8* dstline = dst;
					unsigned x;
					for(x=0;x<size_x;++x) {
						dstline[0] = srcline[0];
						dstline[1] = srcline[1];
						dstline[2] = srcline[2];
						dstline += 3;
						srcline += src_bytes_per_scanline;
					}
					dst += dst_bytes_per_scanline;
					src += 3;
				}
			} else if (bmp->bytes_per_pixel == 4) {
				unsigned y;
				for(y=0;y<size_y;++y) {
					uint8* srcline = src;
					uint8* dstline = dst;
					unsigned x;
					for(x=0;x<size_x;++x) {
						/* dstline and srcline are always aligned at 4 bytes */
						*(uint32*)dstline = *(uint32*)srcline;
						dstline += 4;
						srcline += src_bytes_per_scanline;
					}
					dst += dst_bytes_per_scanline;
					src += 4;
				}
			}
		}

		/* copy */
		free(bmp->heap);
		*bmp = *newbmp;
		newbmp->heap = 0;
		newbmp->ptr = 0;
		bitmap_free(newbmp);
	}

	if (orientation_mask & ORIENTATION_MIRROR_Y) {
		unsigned bytes_per_scanline = bmp->bytes_per_scanline;
		uint8* y0 = bitmap_line(bmp, 0);
		uint8* y1 = bitmap_line(bmp, bmp->size_y - 1);
		void* buf = malloc(bytes_per_scanline);
		for(;y0<y1; y0 += bytes_per_scanline, y1 -= bytes_per_scanline) {
			memcpy(buf, y0, bytes_per_scanline);
			memcpy(y0, y1, bytes_per_scanline);
			memcpy(y1, buf, bytes_per_scanline);
		}
		free(buf);
	}

	if (orientation_mask & ORIENTATION_MIRROR_X) {
		if (bmp->bytes_per_pixel == 1) {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint8* x0 = (uint8*)bitmap_line(bmp, y);
				uint8* x1 = x0 + bmp->size_x - 1;
				for(;x0<x1;++x0, --x1) {
					uint8 t = *x0;
					*x0 = *x1;
					*x1 = t;
				}
			}
		} else if (bmp->bytes_per_pixel == 2) {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint8* x0 = (uint8*)bitmap_line(bmp, y);
				uint8* x1 = x0 + (bmp->size_x - 1) * 2;
				for(;x0<x1;x0 += 2, x1 -= 2) {
					uint8 t[2];
					t[0] = x0[0];
					t[1] = x0[1];
					x0[0] = x1[0];
					x0[1] = x1[1];
					x1[0] = t[0];
					x1[1] = t[1];
				}
			}
		} else if (bmp->bytes_per_pixel == 3) {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint8* x0 = (uint8*)bitmap_line(bmp, y);
				uint8* x1 = x0 + (bmp->size_x - 1) * 3;
				for(;x0<x1;x0 += 3, x1 -= 3) {
					uint8 t[3];
					t[0] = x0[0];
					t[1] = x0[1];
					t[2] = x0[2];
					x0[0] = x1[0];
					x0[1] = x1[1];
					x0[2] = x1[2];
					x1[0] = t[0];
					x1[1] = t[1];
					x1[2] = t[2];
				}
			}
		} else if (bmp->bytes_per_pixel == 4) {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint8* x0 = (uint8*)bitmap_line(bmp, y);
				uint8* x1 = x0 + (bmp->size_x - 1) * 4;
				for(;x0<x1;x0 += 4, x1 -= 4) {
					uint8 t[4];
					t[0] = x0[0];
					t[1] = x0[1];
					t[2] = x0[2];
					t[3] = x0[3];
					x0[0] = x1[0];
					x0[1] = x1[1];
					x0[2] = x1[2];
					x0[3] = x1[3];
					x1[0] = t[0];
					x1[1] = t[1];
					x1[2] = t[2];
					x1[3] = t[3];
				}
			}
		}
	}
}

/**
 * Counter of the number of color.
 */
struct color_node {
	unsigned index;
	unsigned count;
};

#define COUNT_SORT_BIT_MAX 8
#define COUNT_SORT_MAX (1U << COUNT_SORT_BIT_MAX)

static unsigned count_sort[COUNT_SORT_MAX];

#define REDUCE_INDEX_TO_RED(i) ((i << 4) & 0xF0)
#define REDUCE_INDEX_TO_GREEN(i) (i & 0xF0)
#define REDUCE_INDEX_TO_BLUE(i) ((i >> 4) & 0xF0)
#define REDUCE_RED_TO_INDEX(i) ((i >> 4) & 0xF)
#define REDUCE_GREEN_TO_INDEX(i) (i & 0xF0)
#define REDUCE_BLUE_TO_INDEX(i) ((((unsigned)i) << 4) & 0xF00)
#define REDUCE_COLOR_TO_INDEX(r, g, b) (REDUCE_RED_TO_INDEX(r) | REDUCE_GREEN_TO_INDEX(g) | REDUCE_BLUE_TO_INDEX(b))

#if 0
static void countsort(struct color_node* indexin[], struct color_node* indexout[], unsigned bit, unsigned skipbit)
{
	unsigned max = 1 << bit;
	unsigned mask = max - 1;
	unsigned i;

	for(i=0;i<=max;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> skipbit) & mask;
		count_sort[j+1]++;
	}

	for(i=1;i<max;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> skipbit) & mask;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}
#endif

/**
 * Count sort of the lower 8 bit.
 */
static void countsort80(struct color_node* indexin[], struct color_node* indexout[])
{
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;i++) {
		unsigned j = indexin[i]->count & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;i++) {
		unsigned j = indexin[i]->count & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

/**
 * Count sort of the higher 8 bit.
 */
static void countsort88(struct color_node* indexin[], struct color_node* indexout[])
{
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> 8) & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> 8) & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

/**
 * Reduce the number of colors of a 24 bit bitmap.
 * \param convert Where to put the conversion table. It must have size of REDUCE_INDEX_MAX elements.
 * \param palette Where to put the new palette.
 * \param size Size of the new palette.
 * \param bmp Bitmap at 24 bit.
 */
unsigned bitmap_reduce(unsigned* convert, adv_color_rgb* palette, unsigned size, const adv_bitmap* bmp)
{
	unsigned i, y;
	unsigned res_size;
	static struct color_node map[REDUCE_INDEX_MAX];
	static struct color_node* index1[REDUCE_INDEX_MAX];
	static struct color_node* index2[REDUCE_INDEX_MAX];

	assert( bmp->bytes_per_pixel == 3 );

	/* clear all */
	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		map[i].count = 0;
		map[i].index = 0;
		index1[i] = map + i;
	}

	/* count */
	for(y=0;y<bmp->size_y;++y) {
		unsigned x;
		uint8* line = (uint8*)bitmap_line((adv_bitmap* )bmp, y);
		for(x=0;x<bmp->size_x;++x) {
			unsigned j;
			j = REDUCE_COLOR_TO_INDEX( line[0], line[1], line[2] );
			++map[j].count;
			line += 3;
		}
	}

	/* sort */
	countsort80(index1, index2);
	countsort88(index2, index1);

	/* create palette */
	for(i=0;i<size && index1[REDUCE_INDEX_MAX - i - 1]->count;++i) {
		unsigned subindex = REDUCE_INDEX_MAX - i - 1;
		unsigned j = index1[subindex] - map;
		index1[subindex]->index = i + 1;
		palette[i].red = REDUCE_INDEX_TO_RED( j );
		palette[i].green = REDUCE_INDEX_TO_GREEN( j );
		palette[i].blue = REDUCE_INDEX_TO_BLUE( j );
		palette[i].alpha = 0;
	}
	res_size = i;

	/* make index table */
	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		if (map[i].index) {
			convert[i] = map[i].index - 1;
		} else if (map[i].count) {
			int red = REDUCE_INDEX_TO_RED( i );
			int green = REDUCE_INDEX_TO_GREEN( i );
			int blue = REDUCE_INDEX_TO_BLUE( i );
			long diff = (red-palette[0].red)*(red-palette[0].red) + (green-palette[0].green)*(green-palette[0].green) + (blue-palette[0].blue)*(blue-palette[0].blue);
			unsigned best = 0;
			unsigned j;

			for(j=1;j<res_size;++j) {
				long new_diff = (red-palette[j].red)*(red-palette[j].red) + (green-palette[j].green)*(green-palette[j].green) + (blue-palette[j].blue)*(blue-palette[j].blue);
				if (new_diff < diff) {
					best = j;
					diff = new_diff;
				}
			}
			convert[i] = best;
		} else {
			convert[i] = 0;
		}
	}

	return res_size;
}

/**
 * Convert a 24 bit bitmap to a palettized 8 bit version.
 * The conversion map must be computed with bitmap_reduce().
 */
void bitmap_cvt_reduce_24to8(adv_bitmap* dst, adv_bitmap* src, unsigned* convert_map)
{
	unsigned cx, cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint8* dst_ptr = bitmap_line(dst, cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned color = convert_map[REDUCE_COLOR_TO_INDEX(src_ptr[0], src_ptr[1], src_ptr[2])];
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 3;
		}
	}
}

/**
 * Resize a bitmap.
 * \param bmp Bitmap to resize.
 * \param x, y Start position of the bitmap range.
 * \param src_dx, src_dy Size of the bitmap range to resize.
 * \param dst_dx, dst_dy Size of the resulting bitmap range.
 * \param orientation_mask Orientation operations.
 * \return Resized bitmap, or 0 on error.
 */
adv_bitmap* bitmap_resize(adv_bitmap* src, unsigned x, unsigned y, unsigned src_dx, unsigned src_dy, unsigned dst_dx, unsigned dst_dy, unsigned orientation_mask)
{
	adv_bitmap* dst;
	unsigned* map_x;
	unsigned* map_y;
	unsigned i, j;

	if (!dst_dx || !dst_dy || !src_dx || !src_dy)
		return 0;

	/* new ptr */
	dst = bitmap_alloc(dst_dx, dst_dy, src->bytes_per_pixel * 8);
	map_x = malloc(sizeof(unsigned) * dst_dx);
	map_y = malloc(sizeof(unsigned) * dst_dy);

	slice_vector(map_x, src_dx, dst_dx);
	if (orientation_mask & ORIENTATION_MIRROR_X) {
		for(i=0;i<dst_dx;++i)
			map_x[i] = x + src_dx - map_x[i] - 1;
	} else {
		for(i=0;i<dst_dx;++i)
			map_x[i] = x + map_x[i];
	}

	slice_vector(map_y, src_dy, dst_dy);
	if (orientation_mask & ORIENTATION_MIRROR_Y) {
		for(i=0;i<dst_dy;++i)
			map_y[i] = y + src_dy - map_y[i] - 1;
	} else {
		for(i=0;i<dst_dy;++i)
			map_y[i] = y + map_y[i];
	}

	if (src->bytes_per_pixel == 1) {
		for(j=0;j<dst_dy;++j) {
			uint8* src_ptr;
			uint8* dst_ptr;
			src_ptr = (uint8*)bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)bitmap_line(dst, j);
			for(i=0;i<dst_dx;++i) {
				*dst_ptr = src_ptr[map_x[i]];
				++dst_ptr;
			}
		}
	} else if (src->bytes_per_pixel == 2) {
		for(j=0;j<dst_dy;++j) {
			uint8* src_ptr;
			uint8* dst_ptr;
			src_ptr = (uint8*)bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)bitmap_line(dst, j);
			for(i=0;i<dst_dx;++i) {
				unsigned off = map_x[i] * 2;
				dst_ptr[0] = src_ptr[off];
				dst_ptr[1] = src_ptr[off+1];
				dst_ptr += 2;
			}
		}
	} else if (src->bytes_per_pixel == 3) {
		for(j=0;j<dst_dy;++j) {
			uint8* src_ptr;
			uint8* dst_ptr;
			src_ptr = (uint8*)bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)bitmap_line(dst, j);
			for(i=0;i<dst_dx;++i) {
				unsigned off = map_x[i] * 3;
				dst_ptr[0] = src_ptr[off];
				dst_ptr[1] = src_ptr[off+1];
				dst_ptr[2] = src_ptr[off+2];
				dst_ptr += 3;
			}
		}
	} else if (src->bytes_per_pixel == 4) {
		for(j=0;j<dst_dy;++j) {
			uint8* src_ptr;
			uint8* dst_ptr;
			src_ptr = (uint8*)bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)bitmap_line(dst, j);
			for(i=0;i<dst_dx;++i) {
				unsigned off = map_x[i] * 4;
				dst_ptr[0] = src_ptr[off];
				dst_ptr[1] = src_ptr[off+1];
				dst_ptr[2] = src_ptr[off+2];
				dst_ptr[3] = src_ptr[off+3];
				dst_ptr += 4;
			}
		}
	}

	free(map_x);
	free(map_y);

	return dst;
}

static void bitmap_filter_stage(
		uint8* dst_ptr, adv_color_def dst_def, int ddp, unsigned dbp,
		uint8* src_ptr, adv_color_def src_def, int sdp, unsigned sbp,
		unsigned count,
		adv_filter* f)
{
	unsigned count_0, count_1, count_2, count_3;
	unsigned order, delay, size;
	unsigned r, g, b;
	adv_filter_real or, og, ob;
	adv_filter_state sr;
	adv_filter_state sg;
	adv_filter_state sb;
	adv_pixel p;
	unsigned i;
	int src_red_shift, src_green_shift, src_blue_shift;
	adv_pixel src_red_mask, src_green_mask, src_blue_mask;
	int dst_red_shift, dst_green_shift, dst_blue_shift;
	adv_pixel dst_red_mask, dst_green_mask, dst_blue_mask;

	union adv_color_def_union sdef;
	union adv_color_def_union ddef;

	sdef.ordinal = src_def;
	ddef.ordinal = dst_def;

	rgb_shiftmask_get(&src_red_shift, &src_red_mask, sdef.nibble.red_len, sdef.nibble.red_pos);
	rgb_shiftmask_get(&src_green_shift, &src_green_mask, sdef.nibble.green_len, sdef.nibble.green_pos);
	rgb_shiftmask_get(&src_blue_shift, &src_blue_mask, sdef.nibble.blue_len, sdef.nibble.blue_pos);
	rgb_shiftmask_get(&dst_red_shift, &dst_red_mask, ddef.nibble.red_len, ddef.nibble.red_pos);
	rgb_shiftmask_get(&dst_green_shift, &dst_green_mask, ddef.nibble.green_len, ddef.nibble.green_pos);
	rgb_shiftmask_get(&dst_blue_shift, &dst_blue_mask, ddef.nibble.blue_len, ddef.nibble.blue_pos);

	order = filter_order_get(f);
	delay = filter_delay_get(f);
	size = count;

	count_0 = order - 1 - delay;
	count_1 = delay;
	count_2 = size - delay;
	count_3 = delay;

	filter_state_reset(f, &sr);
	filter_state_reset(f, &sg);
	filter_state_reset(f, &sb);

	p = cpu_uint_read(src_ptr, sbp);

	r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
	g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
	b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

	for(i=0;i<count_0;++i) {
		filter_insert(f, &sr, r);
		filter_insert(f, &sg, g);
		filter_insert(f, &sb, b);
	}

	for(i=0;i<count_1;++i) {
		p = cpu_uint_read(src_ptr, sbp);

		r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
		g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
		b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

		src_ptr += sdp;

		filter_insert(f, &sr, r);
		filter_insert(f, &sg, g);
		filter_insert(f, &sb, b);
	}

	for(i=0;i<count_2;++i) {
		p = cpu_uint_read(src_ptr, sbp);

		r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
		g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
		b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

		src_ptr += sdp;

		filter_insert(f, &sr, r);
		filter_insert(f, &sg, g);
		filter_insert(f, &sb, b);

		or = filter_extract(f, &sr);
		og = filter_extract(f, &sg);
		ob = filter_extract(f, &sb);
		if (or < 0) or = 0;
		if (or > 255) or = 255;
		if (og < 0) og = 0;
		if (og > 255) og = 255;
		if (ob < 0) ob = 0;
		if (ob > 255) ob = 255;

		p = rgb_nibble_insert(or, dst_red_shift, dst_red_mask)
			| rgb_nibble_insert(og, dst_green_shift, dst_green_mask)
			| rgb_nibble_insert(ob, dst_blue_shift, dst_blue_mask);

		cpu_uint_write(dst_ptr, dbp, p);

		dst_ptr += ddp;
	}

	for(i=0;i<count_3;++i) {
		filter_insert(f, &sr, r);
		filter_insert(f, &sg, g);
		filter_insert(f, &sb, b);

		or = filter_extract(f, &sr);
		og = filter_extract(f, &sg);
		ob = filter_extract(f, &sb);
		if (or < 0) or = 0;
		if (or > 255) or = 255;
		if (og < 0) og = 0;
		if (og > 255) og = 255;
		if (ob < 0) ob = 0;
		if (ob > 255) ob = 255;

		p = rgb_nibble_insert(or, dst_red_shift, dst_red_mask)
			| rgb_nibble_insert(og, dst_green_shift, dst_green_mask)
			| rgb_nibble_insert(ob, dst_blue_shift, dst_blue_mask);

		cpu_uint_write(dst_ptr, dbp, p);

		dst_ptr += ddp;
	}
}

/**
 * Resample a bitmap.
 * \param src Bitmap to resample.
 * \param x, y Start position of the bitmap range.
 * \param src_dx, src_dy Size the bitmap range.
 * \param dst_dx, dst_dy Size of the resized bitmap.
 * \param orientation_mask Orientation operations.
 * \param def RGB definition.
 * \return Resampled bitmap, or 0 on error.
 */
adv_bitmap* bitmap_resample(adv_bitmap* src, unsigned x, unsigned y, unsigned src_dx, unsigned src_dy, unsigned dst_dx, unsigned dst_dy, unsigned orientation_mask, adv_color_def def)
{
	adv_bitmap* dst;
	unsigned order = 7;

	if (!dst_dx || !dst_dy || !src_dx || !src_dy)
		return 0;

	if (color_def_type_get(def) != adv_color_type_rgb)
		return bitmap_resize(src, x, y, src_dx, src_dy, dst_dx, dst_dy, orientation_mask);

	if (src_dx > dst_dx) {
		adv_filter f;
		unsigned cy;

		filter_lpfir_set(&f, (double)dst_dx / src_dx / 2, order);

		for(cy=0;cy<src_dy;++cy) {
			uint8* ptr = bitmap_pixel(src, x, y + cy);
			bitmap_filter_stage(
				ptr, def, src->bytes_per_pixel, src->bytes_per_pixel,
				ptr, def, src->bytes_per_pixel, src->bytes_per_pixel,
				src_dx,
				&f
			);
		}
	}

	if (src_dy > dst_dy) {
		adv_filter f;
		unsigned cx;

		filter_lpfir_set(&f, (double)dst_dy / src_dy / 2, order);

		for(cx=0;cx<src_dx;++cx) {
			uint8* ptr = bitmap_pixel(src, x + cx, y);
			bitmap_filter_stage(
				ptr, def, src->bytes_per_scanline, src->bytes_per_pixel,
				ptr, def, src->bytes_per_scanline, src->bytes_per_pixel,
				src_dy,
				&f
			);
		}
	}

	dst = bitmap_resize(src, x, y, src_dx, src_dy, dst_dx, dst_dy, orientation_mask);
	if (!dst)
		return 0;

	if (src_dx < dst_dx) {
		adv_filter f;
		unsigned cy;

		filter_lpfir_set(&f, (double)src_dx / dst_dx / 2, order);

		for(cy=0;cy<dst_dy;++cy) {
			uint8* ptr = bitmap_pixel(dst, 0, cy);
			bitmap_filter_stage(
				ptr, def, dst->bytes_per_pixel, dst->bytes_per_pixel,
				ptr, def, dst->bytes_per_pixel, dst->bytes_per_pixel,
				dst_dx,
				&f
			);
		}
	}

	if (src_dy < dst_dy) {
		adv_filter f;
		unsigned cx;

		filter_lpfir_set(&f, (double)src_dy / dst_dy / 2, order);

		for(cx=0;cx<dst_dx;++cx) {
			uint8* ptr = bitmap_pixel(dst, cx, 0);
			bitmap_filter_stage(
				ptr, def, dst->bytes_per_scanline, dst->bytes_per_pixel,
				ptr, def, dst->bytes_per_scanline, dst->bytes_per_pixel,
				dst_dy,
				&f
			);
		}
	}

	return dst;
}

/**
 * Scan the bitmap to find a fixed color border.
 * \param bmp Bitmap to scan.
 * \param rcx Where to put the width of the border found.
 * \param rcy Where to put the height of the border found.
 */
void bitmap_cutoff(adv_bitmap* bmp, unsigned* rcx, unsigned* rcy)
{
	unsigned yu = 0;
	unsigned yd = bmp->size_y - 1;

	unsigned cx = bmp->size_x / 2;
	unsigned cy = bmp->size_y / 2;

	if (bmp->bytes_per_pixel == 1) {
		uint8 c = *(uint8*)bitmap_line(bmp, 0);
		while (yu < yd && cx) {
			unsigned i;
			uint8* pu = (uint8*)bitmap_line(bmp, yu);
			uint8* pd = (uint8*)bitmap_line(bmp, yd);

			i = 0;
			while (i < cx && pu[i]==c && pu[bmp->size_x - i -1]==c)
				++i;
			cx = i;

			i = 0;
			while (i < cx && pd[i]==c && pd[bmp->size_x - i -1]==c)
				++i;
			cx = i;

			if (yu < cy && cx != bmp->size_x / 2)
				cy = yu;

			++yu;
			--yd;
		}
	} else if (bmp->bytes_per_pixel == 2) {
		uint16 c = *(uint16*)bitmap_line(bmp, 0);
		while (yu < yd && cx) {
			unsigned i;
			uint16* pu = (uint16*)bitmap_line(bmp, yu);
			uint16* pd = (uint16*)bitmap_line(bmp, yd);

			i = 0;
			while (i < cx && pu[i]==c && pu[bmp->size_x - i -1]==c)
				++i;
			cx = i;

			i = 0;
			while (i < cx && pd[i]==c && pd[bmp->size_x - i -1]==c)
				++i;
			cx = i;

			if (yu < cy && cx != bmp->size_x / 2)
				cy = yu;

			++yu;
			--yd;
		}
	} else if (bmp->bytes_per_pixel == 3) {
		uint8* dot = (uint8*)bitmap_line(bmp, 0);
		uint8 c0 = dot[0];
		uint8 c1 = dot[1];
		uint8 c2 = dot[2];
		while (yu < yd && cx) {
			unsigned i;
			unsigned i3;
			unsigned si3;
			uint8* pu = (uint8*)bitmap_line(bmp, yu);
			uint8* pd = (uint8*)bitmap_line(bmp, yd);

			i = 0;
			i3 = 0;
			si3 = bmp->size_x*3 - 3;
			while (i < cx && pu[i3]==c0 && pu[i3+1]==c1 && pu[i3+2]==c2 && pu[si3]==c0 && pu[si3+1]==c1 && pu[si3+2]==c2) {
				++i;
				i3 += 3;
				si3 -= 3;
			}
			cx = i;

			i = 0;
			i3 = 0;
			si3 = bmp->size_x*3 - 3;
			while (i < cx && pd[i3]==c0 && pd[i3+1]==c1 && pd[i3+2]==c2 && pd[si3]==c0 && pd[si3+1]==c1 && pd[si3+2]==c2) {
				++i;
				i3 += 3;
				si3 -= 3;
			}
			cx = i;

			if (yu < cy && cx != bmp->size_x / 2)
				cy = yu;

			++yu;
			--yd;
		}
	} else {
		cx = 0;
		cy = 0;
	}

	*rcx = cx;
	*rcy = cy;
}

/**
 * Convert a 8 bit bitmap.
 * \param dst Destination bitmap. The bitmap must have at least the same size of the source bitmap.
 * \param src Source bitmap.
 * \param color_map Conversion table.
 */
static void bitmap_cvt_palette_8to8(adv_bitmap* dst, adv_bitmap* src, unsigned* color_map)
{
	unsigned cx, cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint8* dst_ptr = bitmap_line(dst, cy);
		for(cx=0;cx<src->size_x;++cx) {
			*dst_ptr = color_map[*src_ptr];
			dst_ptr += 1;
			src_ptr += 1;
		}
	}
}

/**
 * Convert a 8 bit bitmap to a 16 bit bitmap.
 * \param dst Destination bitmap. The bitmap must have at least the same size of the source bitmap.
 * \param src Source bitmap.
 * \param color_map Conversion table. 
 */
static void bitmap_cvt_palette_8to16(adv_bitmap* dst, adv_bitmap* src, unsigned* color_map)
{
	unsigned cx, cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint16* dst_ptr = (uint16*)bitmap_line(dst, cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned color = color_map[*src_ptr];
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
	}
}

/**
 * Convert a 8 bit bitmap to a 32 bit bitmap.
 * \param dst Destination bitmap. The bitmap must have at least the same size of the source bitmap.
 * \param src Source bitmap.
 * \param color_map Conversion table. 
 */
static void bitmap_cvt_palette_8to32(adv_bitmap* dst, adv_bitmap* src, unsigned* color_map)
{
	unsigned cx, cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint32* dst_ptr = (uint32*)bitmap_line(dst, cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned color = color_map[*src_ptr];
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
	}
}

/**
 * Convert a palette bitmap.
 * \param dst Destination bitmap. The bitmap must have at least the same size of the source bitmap.
 * \param src Source bitmap.
 * \param color_map Conversion table. 
 */
void bitmap_cvt_palette(adv_bitmap* dst, adv_bitmap* src, unsigned* color_map)
{
	unsigned cx, cy;
	unsigned sdp, ddp;

	sdp = src->bytes_per_pixel;
	ddp = dst->bytes_per_pixel;

	/* specialized versions */
	if (sdp == 1) {
		switch (dst->bytes_per_pixel) {
		case 1 :
			bitmap_cvt_palette_8to8(dst, src, color_map);
			return;
		case 2 :
			bitmap_cvt_palette_8to16(dst, src, color_map);
			return;
		case 4 :
			bitmap_cvt_palette_8to32(dst, src, color_map);
			return;
		}
	}

	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint8* dst_ptr = bitmap_line(dst, cy);

		for(cx=0;cx<src->size_x;++cx) {
			adv_pixel p;

			p = cpu_uint_read(src_ptr, sdp);

			p = color_map[p];

			cpu_uint_write(dst_ptr, ddp, p);

			src_ptr += sdp;
			dst_ptr += ddp;
		}
	}
}

/**
 * Convert a RGB bitmap.
 * \param dst Destination bitmap.
 * \param dst_def Destination RGB definition.
 * \param src Source bitmap.
 * \param src_def Source RGB definition.
 */
void bitmap_cvt_rgb(adv_bitmap* dst, adv_color_def dst_def, adv_bitmap* src, adv_color_def src_def)
{
	unsigned cx, cy;
	union adv_color_def_union sdef;
	union adv_color_def_union ddef;
	int red_shift, green_shift, blue_shift;
	adv_pixel red_mask, green_mask, blue_mask;
	unsigned sdp, ddp;

	sdef.ordinal = src_def;
	ddef.ordinal = dst_def;

	red_shift = rgb_conv_shift_get(sdef.nibble.red_len, sdef.nibble.red_pos, ddef.nibble.red_len, ddef.nibble.red_pos);
	red_mask = rgb_conv_mask_get(sdef.nibble.red_len, sdef.nibble.red_pos, ddef.nibble.red_len, ddef.nibble.red_pos);
	green_shift = rgb_conv_shift_get(sdef.nibble.green_len, sdef.nibble.green_pos, ddef.nibble.green_len, ddef.nibble.green_pos);
	green_mask = rgb_conv_mask_get(sdef.nibble.green_len, sdef.nibble.green_pos, ddef.nibble.green_len, ddef.nibble.green_pos);
	blue_shift = rgb_conv_shift_get(sdef.nibble.blue_len, sdef.nibble.blue_pos, ddef.nibble.blue_len, ddef.nibble.blue_pos);
	blue_mask = rgb_conv_mask_get(sdef.nibble.blue_len, sdef.nibble.blue_pos, ddef.nibble.blue_len, ddef.nibble.blue_pos);

	sdp = src->bytes_per_pixel;
	ddp = dst->bytes_per_pixel;

	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint8* dst_ptr = bitmap_line(dst, cy);

		for(cx=0;cx<src->size_x;++cx) {
			adv_pixel p;

			p = cpu_uint_read(src_ptr, sdp);

			p = (rgb_shift(p, red_shift) & red_mask)
				| (rgb_shift(p, green_shift) & green_mask)
				| (rgb_shift(p, blue_shift) & blue_mask);

			cpu_uint_write(dst_ptr, ddp, p);

			src_ptr += sdp;
			dst_ptr += ddp;
		}
	}
}

void bitmap_put(adv_bitmap* dst, unsigned x, unsigned y, adv_bitmap* src)
{
	unsigned cy;
	int w;
	int h;

	h = src->size_y;
	if (y + h > dst->size_y)
		h = dst->size_y - y;
	w = src->size_x;
	if (x + w > dst->size_x)
		w = dst->size_x - x;

	if (h<=0 || w<=0)
		return;

	for(cy=0;cy<h;++cy) {
		uint8* src_ptr = bitmap_line(src, cy);
		uint8* dst_ptr = bitmap_line(dst, cy);
		memcpy(dst_ptr, src_ptr, w * src->bytes_per_pixel);
	}
}

