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

#include "video.h"
#include "bitmap.h"
#include "endianrw.h"
#include "rgb.h"
#include "pngdef.h"
#include "slice.h"
#include "filter.h"
#include "png.h"
#include "blit.h"

/**
 * Allocate a bitmap.
 * \param width Width.
 * \param height Height.
 * \param bytes_per_pixel Bytes per pixel.
 * \return The allocated bitmap or 0 on error.
 */
adv_bitmap* adv_bitmap_alloc(unsigned width, unsigned height, unsigned bytes_per_pixel)
{
	adv_bitmap* bmp = (adv_bitmap*)malloc(sizeof(adv_bitmap));
	if (!bmp)
		return 0;

	bmp->size_x = width;
	bmp->size_y = height;
	bmp->bytes_per_pixel = bytes_per_pixel;
	bmp->bytes_per_scanline = (bmp->size_x * bmp->bytes_per_pixel + 3) & ~0x3;
	bmp->heap = malloc(bmp->bytes_per_scanline * bmp->size_y);
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
adv_bitmap* adv_bitmap_dup(adv_bitmap* bmp)
{
	unsigned i;

	adv_bitmap* r = (adv_bitmap*)malloc(sizeof(adv_bitmap));
	if (!r)
		return 0;

	r->size_x = bmp->size_x;
	r->size_y = bmp->size_y;
	r->bytes_per_pixel = bmp->bytes_per_pixel;
	r->bytes_per_scanline = (r->size_x * r->bytes_per_pixel + 3) & ~0x3;
	r->heap = malloc(r->bytes_per_scanline * r->size_y);
	if (!r->heap) {
		free(r);
		return 0;
	}
	r->ptr = r->heap;

	for(i=0;i<r->size_y;++i)
		memcpy(adv_bitmap_line(r, i), adv_bitmap_line(bmp, i), r->size_x * r->bytes_per_pixel);

	return r;
}

/**
 * Move a bitmap.
 * The destination bitmap will own the src bitmap.
 * The src bitmap is freed.
 * \param dst Where to move the bitmap. It mut be a valid allocated bitmap.
 * \param src Source bitmap. Unusable after this call.
 */
void adv_bitmap_move(adv_bitmap* dst, adv_bitmap* src)
{
	free(dst->heap);

	*dst = *src;

	free(src);
}

/**
 * Create a RGB bitmap allocated externally.
 * \param width Width.
 * \param height Height.
 * \param pixel Bytes per pixel.
 * \param dat_ptr Pointer at the allocated data. If this pointer is
 * different than 0 the data is freed when calling ::bitmap_free().
 * \param dat_size Size of the allocated data.
 * \param ptr Pointer at the first pixel.
 * \param scanline Bytes per scanline.
 */
adv_bitmap* adv_bitmap_import_rgb(unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline)
{
	adv_bitmap* bmp = (adv_bitmap*)malloc(sizeof(adv_bitmap));
	assert(bmp);

	bmp->size_x = width;
	bmp->size_y = height;
	bmp->bytes_per_pixel = pixel;
	bmp->bytes_per_scanline = scanline;
	bmp->ptr = ptr;
	bmp->heap = dat_ptr;

	return bmp;
}

/**
 * Create a palette bitmap allocated externally.
 * \param rgb Where put the bitmap palette. At least space for pal_size/3 entries is required.
 * \param rgb_max Where put the bitmap palette size.
 * \param width Width.
 * \param height Height.
 * \param pixel Bytes per pixel.
 * \param dat_ptr Pointer at the allocated data. If this pointer is
 * different than 0 the data is freed when calling ::bitmap_free().
 * \param dat_size Size of the allocated data.
 * \param ptr Pointer at the first pixel.
 * \param scanline Bytes per scanline.
 * \param pal_ptr Raw palette data in RGB byte format.
 * \param pal_size Size in byte of the raw palette data.
 */
adv_bitmap* adv_bitmap_import_palette(adv_color_rgb* rgb, unsigned* rgb_max, unsigned width, unsigned height, unsigned pixel, unsigned char* dat_ptr, unsigned dat_size, unsigned char* ptr, unsigned scanline, unsigned char* pal_ptr, unsigned pal_size)
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

	return adv_bitmap_import_rgb(width, height, pixel, dat_ptr, dat_size, ptr, scanline);
}

/**
 * Deallocated a bitmap.
 */
void adv_bitmap_free(adv_bitmap* bmp)
{
	if (bmp)
		free(bmp->heap);
	free(bmp);
}

/**
 * Reverse the orientation mirror x/y flags if the x/y axis are swapped.
 **/
unsigned adv_orientation_rev(unsigned orientation_mask)
{
	if ((orientation_mask & ADV_ORIENTATION_FLIP_XY) != 0) {
		adv_bool flipx = (orientation_mask & ADV_ORIENTATION_FLIP_X) != 0;
		adv_bool flipy = (orientation_mask & ADV_ORIENTATION_FLIP_Y) != 0;

		orientation_mask = ADV_ORIENTATION_FLIP_XY;

		if (flipx)
			orientation_mask |= ADV_ORIENTATION_FLIP_Y;
		if (flipy)
			orientation_mask |= ADV_ORIENTATION_FLIP_X;
	}

	return orientation_mask;
}

/**
 * Change the orientation of a bitmap.
 * \param bmp Bitmap to orient.
 * \param orientation_mask Subset of the ADV_ORIENTATION_* flags.
 */
void adv_bitmap_orientation(adv_bitmap* bmp, unsigned orientation_mask)
{
	if (orientation_mask & ADV_ORIENTATION_FLIP_XY) {
		adv_bitmap* rotated;
		unsigned size_x;
		unsigned size_y;
		unsigned src_bytes_per_scanline;
		unsigned dst_bytes_per_scanline;
		uint8* src;
		uint8* dst;

		/* new ptr */
		rotated = adv_bitmap_alloc(bmp->size_y, bmp->size_x, bmp->bytes_per_pixel);

		size_x = rotated->size_x;
		size_y = rotated->size_y;
		src_bytes_per_scanline = bmp->bytes_per_scanline;
		dst_bytes_per_scanline = rotated->bytes_per_scanline;
		src = (uint8*)bmp->ptr;
		dst = (uint8*)rotated->ptr;

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

		adv_bitmap_move(bmp, rotated);
	}

	if (orientation_mask & ADV_ORIENTATION_FLIP_Y) {
		unsigned bytes_per_scanline = bmp->bytes_per_scanline;
		uint8* y0 = adv_bitmap_line(bmp, 0);
		uint8* y1 = adv_bitmap_line(bmp, bmp->size_y - 1);
		void* buf = malloc(bytes_per_scanline);
		for(;y0<y1; y0 += bytes_per_scanline, y1 -= bytes_per_scanline) {
			memcpy(buf, y0, bytes_per_scanline);
			memcpy(y0, y1, bytes_per_scanline);
			memcpy(y1, buf, bytes_per_scanline);
		}
		free(buf);
	}

	if (orientation_mask & ADV_ORIENTATION_FLIP_X) {
		if (bmp->bytes_per_pixel == 1) {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint8* x0 = (uint8*)adv_bitmap_line(bmp, y);
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
				uint8* x0 = (uint8*)adv_bitmap_line(bmp, y);
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
				uint8* x0 = (uint8*)adv_bitmap_line(bmp, y);
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
				uint8* x0 = (uint8*)adv_bitmap_line(bmp, y);
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

/**
 * Number of bit for channel in the color reduction.
 */
#define REDUCE_COLOR_BIT 4U

/**
 * Size of the conversion table for the color reduction.
 */
#define REDUCE_INDEX_MAX (1U << (3*REDUCE_COLOR_BIT))

#define REDUCE_INDEX_TO_RED(i) ((i << 4) & 0xF0)
#define REDUCE_INDEX_TO_GREEN(i) (i & 0xF0)
#define REDUCE_INDEX_TO_BLUE(i) ((i >> 4) & 0xF0)
#define REDUCE_RED_TO_INDEX(i) ((i >> 4) & 0xF)
#define REDUCE_GREEN_TO_INDEX(i) (i & 0xF0)
#define REDUCE_BLUE_TO_INDEX(i) ((((unsigned)i) << 4) & 0xF00)
#define REDUCE_COLOR_TO_INDEX(r, g, b) (REDUCE_RED_TO_INDEX(r) | REDUCE_GREEN_TO_INDEX(g) | REDUCE_BLUE_TO_INDEX(b))

#if 0 /* OSDEF Reference code */
static void countsort(struct color_node* indexin[], struct color_node* indexout[], unsigned bit, unsigned skipbit)
{
	unsigned max = 1 << bit;
	unsigned mask = max - 1;
	unsigned count_sort[max + 1];
	unsigned i;

	for(i=0;i<=max;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = (indexin[i]->count >> skipbit) & mask;
		count_sort[j+1]++;
	}

	for(i=1;i<max;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = (indexin[i]->count >> skipbit) & mask;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}
#endif

/**
 * Count sort. Bits 0-8.
 */
static void countsort80(struct color_node* indexin[], struct color_node* indexout[])
{
	unsigned count_sort[256 + 1];
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = indexin[i]->count & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = indexin[i]->count & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

/**
 * Count sort. Bits 8-16.
 */
static void countsort88(struct color_node* indexin[], struct color_node* indexout[])
{
	unsigned count_sort[256 + 1];
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = (indexin[i]->count >> 8) & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = (indexin[i]->count >> 8) & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

/**
 * Count sort. Bits 24-16.
 */
static void countsort816(struct color_node* indexin[], struct color_node* indexout[])
{
	unsigned count_sort[256 + 1];
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = (indexin[i]->count >> 16) & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		unsigned j = (indexin[i]->count >> 16) & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

static unsigned adv_bitmap_cvt_reduce_step1(unsigned* convert_map, adv_color_rgb* rgb_map, unsigned rgb_max, adv_bitmap* src, adv_color_def src_def)
{
	unsigned i, cy;
	unsigned sdp;

	struct color_node map[REDUCE_INDEX_MAX];
	struct color_node* index1[REDUCE_INDEX_MAX];
	struct color_node* index2[REDUCE_INDEX_MAX];

	int src_red_shift, src_green_shift, src_blue_shift;
	adv_pixel src_red_mask, src_green_mask, src_blue_mask;

	union adv_color_def_union sdef;

	sdef.ordinal = src_def;

	rgb_shiftmask_get(&src_red_shift, &src_red_mask, sdef.nibble.red_len, sdef.nibble.red_pos);
	rgb_shiftmask_get(&src_green_shift, &src_green_mask, sdef.nibble.green_len, sdef.nibble.green_pos);
	rgb_shiftmask_get(&src_blue_shift, &src_blue_mask, sdef.nibble.blue_len, sdef.nibble.blue_pos);

	sdp = src->bytes_per_pixel;

	/* clear all */
	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		map[i].count = 0;
		map[i].index = 0;
		index1[i] = map + i;
	}

	/* count */
	for(cy=0;cy<src->size_y;++cy) {
		unsigned cx;
		uint8* src_ptr = adv_bitmap_line(src, cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned r, g, b;
			adv_pixel p;

			p = cpu_uint_read(src_ptr, sdp);

			r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
			g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
			b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

			p = REDUCE_COLOR_TO_INDEX(r, g, b);

			++map[p].count;

			src_ptr += sdp;
		}
	}

	/* sort */
	countsort80(index1, index2);
	countsort88(index2, index1);
	countsort816(index1, index2);

	/* create palette */
	for(i=0;i<rgb_max && index2[REDUCE_INDEX_MAX - i - 1]->count;++i) {
		unsigned subindex = REDUCE_INDEX_MAX - i - 1;
		unsigned j = index2[subindex] - map;
		index2[subindex]->index = i + 1;
		rgb_map[i].red = REDUCE_INDEX_TO_RED(j);
		rgb_map[i].green = REDUCE_INDEX_TO_GREEN(j);
		rgb_map[i].blue = REDUCE_INDEX_TO_BLUE(j);
		rgb_map[i].alpha = 0;
	}

	/* new limit */
	rgb_max = i;

	/* make index table */
	for(i=0;i<REDUCE_INDEX_MAX;++i) {
		if (map[i].index) {
			convert_map[i] = map[i].index - 1;
		} else if (map[i].count) {
			convert_map[i] = video_color_find(REDUCE_INDEX_TO_RED(i), REDUCE_INDEX_TO_GREEN(i), REDUCE_INDEX_TO_BLUE(i), rgb_map, rgb_max);
		} else {
			convert_map[i] = 0;
		}
	}

	return rgb_max;
}

static void adv_bitmap_cvt_reduce_step2(adv_bitmap* dst, adv_bitmap* src, adv_color_def src_def, unsigned* convert_map)
{
	unsigned cx, cy;
	unsigned sdp, ddp;
	int src_red_shift, src_green_shift, src_blue_shift;
	adv_pixel src_red_mask, src_green_mask, src_blue_mask;

	union adv_color_def_union sdef;

	sdef.ordinal = src_def;

	rgb_shiftmask_get(&src_red_shift, &src_red_mask, sdef.nibble.red_len, sdef.nibble.red_pos);
	rgb_shiftmask_get(&src_green_shift, &src_green_mask, sdef.nibble.green_len, sdef.nibble.green_pos);
	rgb_shiftmask_get(&src_blue_shift, &src_blue_mask, sdef.nibble.blue_len, sdef.nibble.blue_pos);

	sdp = src->bytes_per_pixel;
	ddp = dst->bytes_per_pixel;

	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = adv_bitmap_line(src, cy);
		uint8* dst_ptr = adv_bitmap_line(dst, cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned r, g, b;
			adv_pixel p;

			p = cpu_uint_read(src_ptr, sdp);

			r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
			g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
			b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

			p = convert_map[REDUCE_COLOR_TO_INDEX(r, g, b)];

			cpu_uint_write(dst_ptr, ddp, p);

			dst_ptr += ddp;
			src_ptr += sdp;
		}
	}
}

/**
 * Convert a RGB bitmap to a palettized 8 bit version.
 */
adv_bitmap* adv_bitmap_cvt_rgbpalette(adv_color_rgb* rgb_map, unsigned* rgb_max, adv_bitmap* src, adv_color_def src_def)
{
	unsigned convert_map[REDUCE_INDEX_MAX];
	adv_bitmap* dst;

	dst = adv_bitmap_alloc(src->size_x, src->size_y, 1);

	*rgb_max = adv_bitmap_cvt_reduce_step1(convert_map, rgb_map, 256, src, src_def);

	adv_bitmap_cvt_reduce_step2(dst, src, src_def, convert_map);

	return dst;
}

/**
 * Resize a bitmap.
 * \param src Bitmap to resize.
 * \param x, y Start position of the bitmap range.
 * \param src_dx, src_dy Size of the bitmap range to resize.
 * \param dst_dx, dst_dy Size of the resulting bitmap range.
 * \param orientation_mask Orientation operations.
 * \return Resized bitmap, or 0 on error.
 */
adv_bitmap* adv_bitmap_resize(adv_bitmap* src, unsigned x, unsigned y, unsigned src_dx, unsigned src_dy, unsigned dst_dx, unsigned dst_dy, unsigned orientation_mask)
{
	adv_bitmap* dst;
	unsigned* map_x;
	unsigned* map_y;
	unsigned i, j;

	if (!dst_dx || !dst_dy || !src_dx || !src_dy)
		return 0;

	/* new ptr */
	dst = adv_bitmap_alloc(dst_dx, dst_dy, src->bytes_per_pixel);
	map_x = malloc(sizeof(unsigned) * dst_dx);
	map_y = malloc(sizeof(unsigned) * dst_dy);

	slice_vector(map_x, src_dx, dst_dx);
	if (orientation_mask & ADV_ORIENTATION_FLIP_X) {
		for(i=0;i<dst_dx;++i)
			map_x[i] = x + src_dx - map_x[i] - 1;
	} else {
		for(i=0;i<dst_dx;++i)
			map_x[i] = x + map_x[i];
	}

	slice_vector(map_y, src_dy, dst_dy);
	if (orientation_mask & ADV_ORIENTATION_FLIP_Y) {
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
			src_ptr = (uint8*)adv_bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)adv_bitmap_line(dst, j);
			for(i=0;i<dst_dx;++i) {
				*dst_ptr = src_ptr[map_x[i]];
				++dst_ptr;
			}
		}
	} else if (src->bytes_per_pixel == 2) {
		for(j=0;j<dst_dy;++j) {
			uint8* src_ptr;
			uint8* dst_ptr;
			src_ptr = (uint8*)adv_bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)adv_bitmap_line(dst, j);
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
			src_ptr = (uint8*)adv_bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)adv_bitmap_line(dst, j);
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
			src_ptr = (uint8*)adv_bitmap_line(src, map_y[j]);
			dst_ptr = (uint8*)adv_bitmap_line(dst, j);
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

static void adv_bitmap_filter_stage(
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

	order = adv_filter_order_get(f);
	delay = adv_filter_delay_get(f);
	size = count;

	count_0 = order - delay;
	count_1 = delay;
	count_2 = size - delay;
	count_3 = delay;

	adv_filter_state_reset(f, &sr);
	adv_filter_state_reset(f, &sg);
	adv_filter_state_reset(f, &sb);

	p = cpu_uint_read(src_ptr, sbp);

	r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
	g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
	b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

	for(i=0;i<count_0;++i) {
		adv_filter_insert(f, &sr, r);
		adv_filter_insert(f, &sg, g);
		adv_filter_insert(f, &sb, b);
	}

	for(i=0;i<count_1;++i) {
		p = cpu_uint_read(src_ptr, sbp);

		r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
		g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
		b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

		src_ptr += sdp;

		adv_filter_insert(f, &sr, r);
		adv_filter_insert(f, &sg, g);
		adv_filter_insert(f, &sb, b);
	}

	for(i=0;i<count_2;++i) {
		p = cpu_uint_read(src_ptr, sbp);

		r = rgb_nibble_extract(p, src_red_shift, src_red_mask);
		g = rgb_nibble_extract(p, src_green_shift, src_green_mask);
		b = rgb_nibble_extract(p, src_blue_shift, src_blue_mask);

		src_ptr += sdp;

		adv_filter_insert(f, &sr, r);
		adv_filter_insert(f, &sg, g);
		adv_filter_insert(f, &sb, b);

		or = adv_filter_extract(f, &sr);
		og = adv_filter_extract(f, &sg);
		ob = adv_filter_extract(f, &sb);
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
		adv_filter_insert(f, &sr, r);
		adv_filter_insert(f, &sg, g);
		adv_filter_insert(f, &sb, b);

		or = adv_filter_extract(f, &sr);
		og = adv_filter_extract(f, &sg);
		ob = adv_filter_extract(f, &sb);
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
adv_bitmap* adv_bitmap_resample(adv_bitmap* src, unsigned x, unsigned y, unsigned src_dx, unsigned src_dy, unsigned dst_dx, unsigned dst_dy, unsigned orientation_mask, adv_color_def def)
{
	adv_bitmap* dst;
	unsigned order = 19;

	if (!dst_dx || !dst_dy || !src_dx || !src_dy)
		return 0;

	if (color_def_type_get(def) != adv_color_type_rgb)
		return adv_bitmap_resize(src, x, y, src_dx, src_dy, dst_dx, dst_dy, orientation_mask);

	if (src_dx > dst_dx) {
		adv_filter f;
		unsigned cy;

		adv_filter_lp_windowedsinc_set(&f, (double)dst_dx / src_dx / 2, order);

		for(cy=0;cy<src_dy;++cy) {
			uint8* ptr = adv_bitmap_pixel(src, x, y + cy);
			adv_bitmap_filter_stage(
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

		adv_filter_lp_windowedsinc_set(&f, (double)dst_dy / src_dy / 2, order);

		for(cx=0;cx<src_dx;++cx) {
			uint8* ptr = adv_bitmap_pixel(src, x + cx, y);
			adv_bitmap_filter_stage(
				ptr, def, src->bytes_per_scanline, src->bytes_per_pixel,
				ptr, def, src->bytes_per_scanline, src->bytes_per_pixel,
				src_dy,
				&f
			);
		}
	}

	dst = adv_bitmap_resize(src, x, y, src_dx, src_dy, dst_dx, dst_dy, orientation_mask);
	if (!dst)
		return 0;

	if (src_dx < dst_dx) {
		adv_filter f;
		unsigned cy;

		adv_filter_lp_windowedsinc_set(&f, (double)src_dx / dst_dx / 2, order);

		for(cy=0;cy<dst_dy;++cy) {
			uint8* ptr = adv_bitmap_pixel(dst, 0, cy);
			adv_bitmap_filter_stage(
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

		adv_filter_lp_windowedsinc_set(&f, (double)src_dy / dst_dy / 2, order);

		for(cx=0;cx<dst_dx;++cx) {
			uint8* ptr = adv_bitmap_pixel(dst, cx, 0);
			adv_bitmap_filter_stage(
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
void adv_bitmap_cutoff(adv_bitmap* bmp, unsigned* rcx, unsigned* rcy)
{
	unsigned yu = 0;
	unsigned yd = bmp->size_y - 1;

	unsigned cx = bmp->size_x / 2;
	unsigned cy = bmp->size_y / 2;

	unsigned dp = bmp->bytes_per_pixel;

	unsigned c = cpu_uint_read(adv_bitmap_line(bmp, 0), dp);

	while (yu < yd && cx) {
		unsigned i;
		uint8* pu = adv_bitmap_line(bmp, yu);
		uint8* pd = adv_bitmap_line(bmp, yd);

		i = 0;
		while (i < cx && cpu_uint_read(pu + i*dp, dp)==c && cpu_uint_read(pu + (bmp->size_x - i -1)*dp, dp)==c)
			++i;
		cx = i;

		i = 0;
		while (i < cx && cpu_uint_read(pd + i*dp, dp)==c && cpu_uint_read(pd + (bmp->size_x - i -1)*dp, dp)==c)
			++i;
		cx = i;

		if (yu < cy && cx != bmp->size_x / 2)
			cy = yu;

		++yu;
		--yd;
	}

	*rcx = cx;
	*rcy = cy;
}

/**
 * Convert a palette bitmap.
 * \param src Source bitmap.
 * \param color_map Conversion table.
 * \return The converted bitmap.
 */
adv_bitmap* adv_bitmap_cvt_palette(adv_bitmap* src, unsigned* color_map)
{
	unsigned cx, cy;
	unsigned sdp, ddp;
	adv_bitmap* dst;

	dst = adv_bitmap_alloc(src->size_x, src->size_y, src->bytes_per_pixel);

	sdp = src->bytes_per_pixel;
	ddp = dst->bytes_per_pixel;

	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = adv_bitmap_line(src, cy);
		uint8* dst_ptr = adv_bitmap_line(dst, cy);

		for(cx=0;cx<src->size_x;++cx) {
			adv_pixel p;

			p = cpu_uint_read(src_ptr, sdp);

			p = color_map[p];

			cpu_uint_write(dst_ptr, ddp, p);

			src_ptr += sdp;
			dst_ptr += ddp;
		}
	}

	return dst;
}

/**
 * Convert a RGB bitmap.
 * \param dst_def Destination RGB definition.
 * \param src Source bitmap.
 * \param src_def Source RGB definition.
 * \return The converted bitmap.
 */
adv_bitmap* adv_bitmap_cvt_rgb(adv_color_def dst_def, adv_bitmap* src, adv_color_def src_def)
{
	unsigned cx, cy;
	union adv_color_def_union sdef;
	union adv_color_def_union ddef;
	int red_shift, green_shift, blue_shift;
	adv_pixel red_mask, green_mask, blue_mask;
	unsigned sdp, ddp;
	adv_bitmap* dst;

	dst = adv_bitmap_alloc(src->size_x, src->size_y, color_def_bytes_per_pixel_get(dst_def));

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
		uint8* src_ptr = adv_bitmap_line(src, cy);
		uint8* dst_ptr = adv_bitmap_line(dst, cy);

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

	return dst;
}

/**
 * Convert a palette bitmap to a RGB bitmap.
 * \param dst_def Destination RGB definition.
 * \param src Source bitmap.
 * \param rgb_map Source palette definition.
 * \param rgb_max Source palette size.
 * \return The converted bitmap.
 */
adv_bitmap* adv_bitmap_cvt_palettergb(adv_color_def dst_def, adv_bitmap* src, adv_color_rgb* rgb_map, unsigned rgb_max)
{
	unsigned cx, cy;
	unsigned sdp, ddp;
	adv_bitmap* dst;

	dst = adv_bitmap_alloc(src->size_x, src->size_y, color_def_bytes_per_pixel_get(dst_def));

	sdp = src->bytes_per_pixel;
	ddp = dst->bytes_per_pixel;

	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = adv_bitmap_line(src, cy);
		uint8* dst_ptr = adv_bitmap_line(dst, cy);

		for(cx=0;cx<src->size_x;++cx) {
			adv_pixel p;
			unsigned i;

			i = cpu_uint_read(src_ptr, sdp);

			p = pixel_make_from_def(rgb_map[i].red, rgb_map[i].green, rgb_map[i].blue, dst_def);

			cpu_uint_write(dst_ptr, ddp, p);

			src_ptr += sdp;
			dst_ptr += ddp;
		}
	}

	return dst;
}

static void bitmap_vclear(adv_bitmap* dst, int x, int y, int dy, unsigned color)
{
	unsigned dp;
	unsigned ds;
	uint8* dst_ptr;

	dp = dst->bytes_per_pixel;
	ds = dst->bytes_per_scanline;

	dst_ptr = adv_bitmap_pixel(dst, x, y);

	if (dp == 1) {
		unsigned count = dy;
		while (count) {
			*dst_ptr = color;
			dst_ptr += ds;
			--count;
		}
	} else if (dp == 2) {
		unsigned count = dy;
		while (count) {
			uint16* dst16 = (uint16*)dst_ptr;
			*dst16 = color;
			dst_ptr += ds;
			--count;
		}
	} else if (dp == 4) {
		unsigned count = dy;
		while (count) {
			uint32* dst32 = (uint32*)dst_ptr;
			*dst32 = color;
			dst_ptr += ds;
			--count;
		}
	} else {
		unsigned count = dy;
		while (count) {
			cpu_uint_write(dst_ptr, dp, color);
			dst_ptr += ds;
			--count;
		}
	}
}

/**
 * Clear part of the bitmap.
 * The specified range is clipped if required.
 */
void adv_bitmap_clear(adv_bitmap* dst, int x, int y, int dx, int dy, unsigned color)
{
	unsigned cy;
	unsigned dp;
	unsigned ds;
	uint8* dst_ptr;

	if (x < 0) {
		dx += x;
		x = 0;
	}
	if (y < 0) {
		dy += y;
		y = 0;
	}
	if (x + dx > dst->size_x)
		dx = dst->size_x - x;
	if (y + dy > dst->size_y)
		dy = dst->size_y - y;
	if (dx<=0 || dy<=0)
		return;

	if (dx == 1) {
		bitmap_vclear(dst, x, y, dy, color);
		return;
	}

	dp = dst->bytes_per_pixel;
	ds = dst->bytes_per_scanline;

	dst_ptr = adv_bitmap_pixel(dst, x, y);

	for(cy=0;cy<dy;++cy) {
		if (dp == 1) {
			uint8* dst8 = (uint8*)dst_ptr;
			unsigned count = dx;
			while (count) {
				*dst8++ = color;
				--count;
			}
		} else if (dp == 2) {
			uint16* dst16 = (uint16*)dst_ptr;
			unsigned count = dx;
			while (count) {
				*dst16++ = color;
				--count;
			}
		} else if (dp == 4) {
			uint32* dst32 = (uint32*)dst_ptr;
			unsigned count = dx;
			while (count) {
				*dst32++ = color;
				--count;
			}
		} else {
			uint8* dst8 = (uint8*)dst_ptr;
			unsigned count = dx;
			while (count) {
				cpu_uint_write(dst8, dp, color);
				dst8 += dp;
				--count;
			}
		}
		++y;
		dst_ptr += ds;
	}
}

/**
 * Draw a box in the bitmap.
 * The specified range is clipped if required.
 */
void adv_bitmap_box(adv_bitmap* dst, int x, int y, int dx, int dy, unsigned border, unsigned color)
{
	if (!border)
		return;

	adv_bitmap_clear(dst, x, y, dx, border, color);
	adv_bitmap_clear(dst, x, y + dy - border, dx, border, color);
	adv_bitmap_clear(dst, x, y + 1, border, dy - 2, color);
	adv_bitmap_clear(dst, x + dx - border, y + 1, border, dy - 2, color);
}

/**
 * Draw a bitmap into another bitmap.
 * The image is clipped only on the destination boundaries.
 */
void adv_bitmap_put(adv_bitmap* dst, int dst_x, int dst_y, const adv_bitmap* src, int src_x, int src_y, int src_dx, int src_dy)
{
	unsigned count;
	unsigned bytes_per_row;
	unsigned src_ds, dst_ds;
	uint8* src8;
	uint8* dst8;

	assert(src->bytes_per_pixel == dst->bytes_per_pixel);

	if (dst_x < 0) {
		src_dx += dst_x;
		dst_x = 0;
	}
	if (dst_y < 0) {
		src_dy += dst_y;
		dst_y = 0;
	}
	if (dst_x + src_dx > dst->size_x)
		src_dx = dst->size_x - dst_x;
	if (dst_y + src_dy > dst->size_y)
		src_dy = dst->size_y - dst_y;
	if (src_dx<=0 || src_dy<=0)
		return;

	bytes_per_row = src_dx * src->bytes_per_pixel;

	src_ds = src->bytes_per_scanline;
	dst_ds = dst->bytes_per_scanline;

	src8 = adv_bitmap_pixel(src, src_x, src_y);
	dst8 = adv_bitmap_pixel(dst, dst_x, dst_y);

	count = src_dy;
	while (count) {
		memcpy(dst8, src8, bytes_per_row);

		dst8 += dst_ds;
		src8 += src_ds;
		--count;
	}
}

static void bitmap_put_alpha_bgra8888(uint8* dst8, unsigned dst_ds, const uint8* back8, unsigned back_ds, const uint8* src8, unsigned src_ds, unsigned dx, unsigned dy)
{
	unsigned count_x, count_y;
	adv_pixel pred_p;
	adv_pixel pred_op;
	adv_pixel pred_r;

	src_ds -= dx * 4;
	dst_ds -= dx * 4;
	back_ds -= dx * 4;

	pred_p = 0;
	pred_op = 0;
	pred_r = 0;

	count_y = dy;
	while (count_y) {
		count_x = dx;
		while (count_x) {
			adv_pixel p, op;

			p = cpu_uint32_read(src8);
			op = cpu_uint32_read(back8);

			/* check if equal at the previous one */
			if (op != pred_op || p != pred_p) {
#ifdef USE_LSB
				if (src8[3] == 0) {
					pred_r = op;
					cpu_uint32_write(dst8, pred_r);
				} else if (src8[3] == 255) {
					pred_r = p;
					cpu_uint32_write(dst8, pred_r);
				} else {
					unsigned char pixel[4];
					int a = src8[3];
					pixel[0] = back8[0] + (src8[0] - back8[0]) * a / 256;
					pixel[1] = back8[1] + (src8[1] - back8[1]) * a / 256;
					pixel[2] = back8[2] + (src8[2] - back8[2]) * a / 256;
					pixel[3] = 0;
					pred_r = cpu_uint32_read(pixel);
					cpu_uint32_write(dst8, pred_r);
				}
#else
				if (src8[0] == 0) {
					pred_r = op;
					cpu_uint32_write(dst8, pred_r);
				} else if (src8[0] == 255) {
					pred_r = p;
					cpu_uint32_write(dst8, pred_r);
				} else {
					unsigned char pixel[4];
					int a = src8[0];
					pixel[0] = 0;
					pixel[1] = back8[1] + (src8[1] - back8[1]) * a / 256;
					pixel[2] = back8[2] + (src8[2] - back8[2]) * a / 256;
					pixel[3] = back8[3] + (src8[3] - back8[3]) * a / 256;
					pred_r = cpu_uint32_read(pixel);
					cpu_uint32_write(dst8, pred_r);
				}
#endif
				pred_p = p;
				pred_op = op;
			} else {
				cpu_uint32_write(dst8, pred_r);
			}

			src8 += 4;
			dst8 += 4;
			back8 += 4;
			--count_x;
		}

		src8 += src_ds;
		dst8 += dst_ds;
		back8 += back_ds;
		--count_y;
	}
}

static void bitmap_put_alpha(uint8* dst8, unsigned dst_dp, unsigned dst_ds, adv_color_def dst_color_def, const uint8* back8, unsigned back_dp, unsigned back_ds, const uint8* src8, unsigned src_dp, unsigned src_ds, adv_color_def src_color_def, unsigned dx, unsigned dy)
{
	unsigned count_x, count_y;
	int src_alpha_shift;
	adv_pixel src_alpha_mask;
	int conv_red_shift, conv_green_shift, conv_blue_shift;
	adv_pixel conv_red_mask, conv_green_mask, conv_blue_mask;
	union adv_color_def_union sdef;
	union adv_color_def_union ddef;
	adv_pixel pred_p;
	adv_pixel pred_op;
	adv_pixel pred_r;

	pred_p = 0;
	pred_op = 0;
	pred_r = 0;

	sdef.ordinal = src_color_def;
	ddef.ordinal = dst_color_def;

	alpha_shiftmask_get(&src_alpha_shift, &src_alpha_mask, src_color_def);

	conv_red_shift = rgb_conv_shift_get(sdef.nibble.red_len, sdef.nibble.red_pos, ddef.nibble.red_len, ddef.nibble.red_pos);
	conv_red_mask = rgb_conv_mask_get(sdef.nibble.red_len, sdef.nibble.red_pos, ddef.nibble.red_len, ddef.nibble.red_pos);
	conv_green_shift = rgb_conv_shift_get(sdef.nibble.green_len, sdef.nibble.green_pos, ddef.nibble.green_len, ddef.nibble.green_pos);
	conv_green_mask = rgb_conv_mask_get(sdef.nibble.green_len, sdef.nibble.green_pos, ddef.nibble.green_len, ddef.nibble.green_pos);
	conv_blue_shift = rgb_conv_shift_get(sdef.nibble.blue_len, sdef.nibble.blue_pos, ddef.nibble.blue_len, ddef.nibble.blue_pos);
	conv_blue_mask = rgb_conv_mask_get(sdef.nibble.blue_len, sdef.nibble.blue_pos, ddef.nibble.blue_len, ddef.nibble.blue_pos);

	src_ds -= dx * src_dp;
	dst_ds -= dx * dst_dp;
	back_ds -= dx * dst_dp;

	count_y = dy;
	while (count_y) {
		count_x = dx;
		while (count_x) {
			adv_pixel p;
			adv_pixel op;

			p = cpu_uint_read(src8, src_dp);
			op = cpu_uint_read(back8, back_dp);

			/* check if equal at the previous one */
			if (op != pred_op || p != pred_p) {
				int a;

				a = rgb_nibble_extract(p, src_alpha_shift, src_alpha_mask);

				if (a == 0) {
					pred_r = op;
					cpu_uint_write(dst8, dst_dp, pred_r);
				} else if (a == 255) {
					pred_r = (rgb_shift(p, conv_red_shift) & conv_red_mask)
						| (rgb_shift(p, conv_green_shift) & conv_green_mask)
						| (rgb_shift(p, conv_blue_shift) & conv_blue_mask);
					cpu_uint_write(dst8, dst_dp, pred_r);
				} else {
					int r, g, b;
					int or, og, ob;

					r = rgb_shift(p, conv_red_shift) & conv_red_mask;
					g = rgb_shift(p, conv_green_shift) & conv_green_mask;
					b = rgb_shift(p, conv_blue_shift) & conv_blue_mask;

					or = op & conv_red_mask;
					og = op & conv_green_mask;
					ob = op & conv_blue_mask;

					or += (r - or) * a / 256;
					og += (g - og) * a / 256;
					ob += (b - ob) * a / 256;

					pred_r = (or & conv_red_mask)
						| (og & conv_green_mask)
						| (ob & conv_blue_mask);

					cpu_uint_write(dst8, dst_dp, pred_r);
				}

				pred_p = p;
				pred_op = op;
			} else {
				cpu_uint_write(dst8, dst_dp, pred_r);
			}

			src8 += src_dp;
			dst8 += dst_dp;
			back8 += back_dp;
			--count_x;
		}

		src8 += src_ds;
		dst8 += dst_ds;
		back8 += back_ds;
		--count_y;
	}
}

/**
 * Draw an alpha bitmap into another bitmap.
 * The image is clipped only on the destination boundaries.
 */
void adv_bitmap_put_alphaback(adv_bitmap* dst, int dst_x, int dst_y, adv_color_def dst_color_def, const adv_bitmap* back, int back_x, int back_y, const adv_bitmap* src, int src_x, int src_y, int src_dx, int src_dy, adv_color_def src_color_def)
{
	uint8* src8;
	uint8* dst8;
	uint8* back8;

	assert(color_def_type_get(src_color_def) == adv_color_type_rgb);
	assert(color_def_type_get(dst_color_def) == adv_color_type_rgb);

	if (dst_x < 0) {
		src_dx += dst_x;
		back_x += dst_x;
		dst_x = 0;
	}
	if (dst_y < 0) {
		src_dy += dst_y;
		back_y += dst_y;
		dst_y = 0;
	}
	if (dst_x + src_dx > dst->size_x)
		src_dx = dst->size_x - dst_x;
	if (dst_y + src_dy > dst->size_y)
		src_dy = dst->size_y - dst_y;
	if (src_dx<=0 || src_dy<=0)
		return;

	src8 = adv_bitmap_pixel(src, src_x, src_y);
	dst8 = adv_bitmap_pixel(dst, dst_x, dst_y);
	back8 = adv_bitmap_pixel(back, back_x, back_y);

	if (dst_color_def == src_color_def
		&& dst_color_def == color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0)) {
		bitmap_put_alpha_bgra8888(dst8, dst->bytes_per_scanline,
			back8, back->bytes_per_scanline,
			src8, src->bytes_per_scanline,
			src_dx, src_dy);
	} else {
		bitmap_put_alpha(dst8, dst->bytes_per_pixel, dst->bytes_per_scanline, dst_color_def,
			back8, back->bytes_per_pixel, back->bytes_per_scanline,
			src8, src->bytes_per_pixel, src->bytes_per_scanline, src_color_def,
			src_dx, src_dy);
	}
}

static void bitmap_clear_alpha(uint8* dst8, unsigned dst_dp, unsigned dst_ds, adv_color_def dst_color_def, const uint8* back8, unsigned back_dp, unsigned back_ds, adv_color_rgb src, unsigned dx, unsigned dy)
{
	unsigned count_x, count_y;
	int conv_red_shift, conv_green_shift, conv_blue_shift;
	adv_pixel conv_red_mask, conv_green_mask, conv_blue_mask;
	union adv_color_def_union ddef;
	adv_pixel pred_op;
	adv_pixel pred_r;
	adv_pixel s;
	int r, g, b, a;

	ddef.ordinal = dst_color_def;

	rgb_shiftmask_get(&conv_red_shift, &conv_red_mask, ddef.nibble.red_len, ddef.nibble.red_pos);
	rgb_shiftmask_get(&conv_green_shift, &conv_green_mask, ddef.nibble.green_len, ddef.nibble.green_pos);
	rgb_shiftmask_get(&conv_blue_shift, &conv_blue_mask, ddef.nibble.blue_len, ddef.nibble.blue_pos);

	r = rgb_nibble_insert(src.red, conv_red_shift, conv_red_mask);
	g = rgb_nibble_insert(src.green, conv_green_shift, conv_green_mask);
	b = rgb_nibble_insert(src.blue, conv_blue_shift, conv_blue_mask);
	a = src.alpha;

	pred_op = 0;
	pred_r = ((r * a / 256) & conv_red_mask)
		| ((g * a / 256) & conv_green_mask)
		| ((b * a / 256) & conv_blue_mask);

	dst_ds -= dx * dst_dp;
	back_ds -= dx * dst_dp;

	count_y = dy;
	while (count_y) {
		count_x = dx;
		while (count_x) {
			adv_pixel op;

			op = cpu_uint_read(back8, back_dp);

			/* check if equal at the previous one */
			if (op != pred_op) {
				int or, og, ob;

				or = op & conv_red_mask;
				og = op & conv_green_mask;
				ob = op & conv_blue_mask;

				or += (r - or) * a / 256;
				og += (g - og) * a / 256;
				ob += (b - ob) * a / 256;

				pred_r = (or & conv_red_mask)
					| (og & conv_green_mask)
					| (ob & conv_blue_mask);

				cpu_uint_write(dst8, dst_dp, pred_r);

				pred_op = op;
			} else {
				cpu_uint_write(dst8, dst_dp, pred_r);
			}

			dst8 += dst_dp;
			back8 += back_dp;
			--count_x;
		}

		dst8 += dst_ds;
		back8 += back_ds;
		--count_y;
	}
}

/**
 * Clear part of the bitmap.
 * The specified range is clipped if required.
 */
void adv_bitmap_clear_alphaback(adv_bitmap* dst, int dst_x, int dst_y, adv_color_def dst_color_def, adv_bitmap* back, int back_x, int back_y, adv_color_rgb src, int src_dx, int src_dy)
{
	uint8* dst8;
	uint8* back8;

	if (src.alpha == 255) {
		adv_bitmap_clear(dst, dst_x, dst_y, src_dx, src_dy, pixel_make_from_def(src.red, src.green, src.blue, dst_color_def));
		return;
	}

	if (src.alpha == 0) {
		if (dst != back)
			adv_bitmap_put(dst, dst_x, dst_y, back, back_x, back_y, src_dx, src_dy);
		return;
	}

	if (dst_x < 0) {
		src_dx += dst_x;
		back_x += dst_x;
		dst_x = 0;
	}
	if (dst_y < 0) {
		src_dy += dst_y;
		back_y += dst_y;
		dst_y = 0;
	}
	if (dst_x + src_dx > dst->size_x)
		src_dx = dst->size_x - dst_x;
	if (dst_y + src_dy > dst->size_y)
		src_dy = dst->size_y - dst_y;
	if (src_dx<=0 || src_dy<=0)
		return;

	dst8 = adv_bitmap_pixel(dst, dst_x, dst_y);
	back8 = adv_bitmap_pixel(back, back_x, back_y);

	bitmap_clear_alpha(dst8, dst->bytes_per_pixel, dst->bytes_per_scanline, dst_color_def,
		back8, back->bytes_per_pixel, back->bytes_per_scanline,
		src, src_dx, src_dy);
}

/**
 * Load a bitmap in PNG format.
 * \param rgb_map Where put the palette entries. It must have space for up of 256 colors.
 * \param rgb_max Where put the number of palette entry. It's 0 for RGB images.
 * \param f Stream to read.
 * \return Bitmap read. It may be in RGB or palette format. The exact rgb format
 * can be obtained with the png_color_def() function.
 */
adv_bitmap* adv_bitmap_load_png(adv_color_rgb* rgb_map, unsigned* rgb_max, adv_fz* f)
{
	unsigned pix_width;
	unsigned pix_height;
	unsigned pix_pixel;
	unsigned char* dat_ptr;
	unsigned dat_size;
	unsigned char* pix_ptr;
	unsigned pix_scanline;
	unsigned char* pal_ptr;
	unsigned pal_size;
	adv_bitmap* bmp;

	if (adv_png_read(&pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, f) != 0) {
		return 0;
	}

	if (pal_ptr) {
		bmp = adv_bitmap_import_palette(rgb_map, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);

		free(pal_ptr);
	} else {
		*rgb_max = 0;
		bmp = adv_bitmap_import_rgb(pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline);
	}

	return bmp;
}

/**
 * Load a bitmap in PNG format as RGB image.
 * If required the bitmap is converted to the specified format.
 * \param f Stream to read.
 * \param def Color RGB definition required.
 * \return Bitmap read.
 */
adv_bitmap* adv_bitmap_load_png_rgb(adv_fz* f, adv_color_def def)
{
	adv_bitmap* dst;
	adv_color_rgb rgb_map[256];
	unsigned rgb_max;

	dst = adv_bitmap_load_png(rgb_map, &rgb_max, f);
	if (!dst)
		return 0;

	if (rgb_max != 0) {
		/* convert from palette */
		adv_bitmap* cvt;

		cvt = adv_bitmap_cvt_palettergb(def, dst, rgb_map, rgb_max);

		adv_bitmap_free(dst);

		return cvt;
	} else if (def != adv_png_color_def(dst->bytes_per_pixel)) {
		/* convert from rgb */
		adv_bitmap* cvt;

		cvt = adv_bitmap_cvt_rgb(def, dst, adv_png_color_def(dst->bytes_per_pixel));

		adv_bitmap_free(dst);

		return cvt;
	} else {
		/* already in the correct format */
		return dst;
	}
}

/**
 * Load a bitmap in PNG format as palette image.
 * If required the bitmap is converted to the palette format.
 * \param rgb_map Where put the palette entries. It must have space for up of 256 colors.
 * \param rgb_max Where put the number of palette entry.
 * \param f Stream to read.
 * \return Bitmap read.
 */
adv_bitmap* adv_bitmap_load_png_palette(adv_color_rgb* rgb_map, unsigned* rgb_max, adv_fz* f)
{
	adv_bitmap* dst;

	dst = adv_bitmap_load_png(rgb_map, rgb_max, f);
	if (!dst)
		return 0;

	if (*rgb_max != 0) {
		/* already in the correct format */
		return dst;
	} else {
		/* convert from rgb */
		adv_bitmap* cvt;

		cvt = adv_bitmap_cvt_rgbpalette(rgb_map, rgb_max, dst, adv_png_color_def(dst->bytes_per_pixel));

		adv_bitmap_free(dst);

		return cvt;
	}
}

