/*
 * This file is part of the AdvanceMAME project.
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

#include "video.h"
#include "bitmap.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

struct bitmap* bitmap_alloc(unsigned x, unsigned y, unsigned bit) {
	struct bitmap* bmp = (struct bitmap*)malloc(sizeof(struct bitmap));
	if (!bmp)
		return 0;

	bmp->size_x = x;
	bmp->size_y = y;
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

struct bitmap* bitmap_dup(struct bitmap* src) {
	unsigned i;

	struct bitmap* bmp = (struct bitmap*)malloc(sizeof(struct bitmap));
	if (!bmp)
		return 0;

	bmp->size_x = src->size_x;
	bmp->size_y = src->size_y;
	bmp->bytes_per_pixel = src->bytes_per_pixel;
	bmp->bytes_per_scanline = (bmp->size_x * bmp->bytes_per_pixel + 3) & ~0x3;
	bmp->heap = malloc( bmp->bytes_per_scanline * bmp->size_y );
	if (!bmp->heap) {
		free(bmp);
		return 0;
	}
	bmp->ptr = bmp->heap;

	for(i=0;i<bmp->size_y;++i)
		memcpy(bitmap_line(bmp,i), bitmap_line(src,i), bmp->size_x * bmp->bytes_per_pixel);

	return bmp;
}

struct bitmap* bitmap_import(unsigned x, unsigned y, unsigned bit, unsigned bytes_per_scanline, void* ptr, void* heap) {
	struct bitmap* bmp = (struct bitmap*)malloc(sizeof(struct bitmap));
	assert( bmp );

	bmp->size_x = x;
	bmp->size_y = y;
	bmp->bytes_per_pixel = (bit + 7) / 8;
	bmp->bytes_per_scanline = bytes_per_scanline;
	bmp->ptr = ptr;
	bmp->heap = heap;

	return bmp;
}

void bitmap_free(struct bitmap* bmp) {
	if (bmp)
		free(bmp->heap);
	free(bmp);
}

uint8* bitmap_line(struct bitmap* bmp, unsigned line) {
	assert( bmp );
	switch (bmp->bytes_per_scanline) {
		case 8 :
			return bmp->ptr + line * 8;
		case 16 :
			return bmp->ptr + line * 16;
		default:
			return bmp->ptr + line * bmp->bytes_per_scanline;
	}
}

void bitmap_putpixel(struct bitmap* bmp, unsigned x, unsigned y, unsigned v) {
	uint8* ptr = bmp->ptr + x * bmp->bytes_per_pixel + y * bmp->bytes_per_scanline;
	switch (bmp->bytes_per_pixel) {
		case 1 :
			ptr[0] = v;
		break;
		case 2 :
			ptr[0] = v;
			ptr[1] = v >> 8;
		break;
		case 3 :
			ptr[0] = v;
			ptr[1] = v >> 8;
			ptr[2] = v >> 16;
		break;
		case 4 :
			ptr[0] = v;
			ptr[1] = v >> 8;
			ptr[2] = v >> 16;
			ptr[3] = v >> 24;
		break;
	};
}

void bitmap_orientation(struct bitmap* bmp, unsigned orientation_mask) {
	if (orientation_mask & ORIENTATION_FLIP_XY) {
		struct bitmap* newbmp;

		/* new ptr */
		newbmp = bitmap_alloc(bmp->size_y,bmp->size_x,bmp->bytes_per_pixel * 8);
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
		uint8* y0 = bitmap_line(bmp,0);
		uint8* y1 = bitmap_line(bmp,bmp->size_y - 1);
		void* buf = malloc(bytes_per_scanline);
		for(;y0<y1; y0 += bytes_per_scanline, y1 -= bytes_per_scanline) {
			memcpy(buf,y0,bytes_per_scanline);
			memcpy(y0,y1,bytes_per_scanline);
			memcpy(y1,buf,bytes_per_scanline);
		}
		free(buf);
	}

	if (orientation_mask & ORIENTATION_MIRROR_X) {
		if (bmp->bytes_per_pixel == 1) {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint8* x0 = (uint8*)bitmap_line(bmp,y);
				uint8* x1 = x0 + bmp->size_x - 1;
				for(;x0<x1;++x0,--x1) {
					uint8 t = *x0;
					*x0 = *x1;
					*x1 = t;
				}
			}
		} else {
			unsigned y;
			for(y=0;y<bmp->size_y;++y) {
				uint16* x0 = (uint16*)bitmap_line(bmp,y);
				uint16* x1 = x0 + bmp->size_x - 1;
				for(;x0<x1;++x0,--x1) {
					uint16 t = *x0;
					*x0 = *x1;
					*x1 = t;
				}
			}
		}
	}
}

struct color_node {
	unsigned index;
	unsigned count;
};

#define COUNT_SORT_BIT_MAX 8
#define COUNT_SORT_MAX (1U << COUNT_SORT_BIT_MAX)

static unsigned count_sort[COUNT_SORT_MAX];

#if 0
static void countsort(struct color_node* indexin[], struct color_node* indexout[], unsigned bit, unsigned skipbit) {
	unsigned max = 1 << bit;
	unsigned mask = max - 1;
	unsigned i;

	for(i=0;i<=max;i++)
		count_sort[i] = 0;

	for(i=0;i<BITMAP_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> skipbit) & mask;
		count_sort[j+1]++;
	}

	for(i=1;i<max;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<BITMAP_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> skipbit) & mask;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}
#endif

static void countsort80(struct color_node* indexin[], struct color_node* indexout[]) {
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<BITMAP_INDEX_MAX;i++) {
		unsigned j = indexin[i]->count & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<BITMAP_INDEX_MAX;i++) {
		unsigned j = indexin[i]->count & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

static void countsort88(struct color_node* indexin[], struct color_node* indexout[]) {
	unsigned i;

	for(i=0;i<=256;i++)
		count_sort[i] = 0;

	for(i=0;i<BITMAP_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> 8) & 0xFF;
		count_sort[j+1]++;
	}

	for(i=1;i<256;i++)
		count_sort[i] += count_sort[i-1];

	for(i=0;i<BITMAP_INDEX_MAX;i++) {
		unsigned j = (indexin[i]->count >> 8) & 0xFF;
		indexout[count_sort[j]] = indexin[i];
		count_sort[j]++;
	}
}

unsigned bitmap_reduction(unsigned* convert, video_color* palette, unsigned size, const struct bitmap* bmp) {
	unsigned i,y;
	unsigned res_size;
	static struct color_node map[BITMAP_INDEX_MAX];
	static struct color_node* index1[BITMAP_INDEX_MAX];
	static struct color_node* index2[BITMAP_INDEX_MAX];

	assert( bmp->bytes_per_pixel == 3 );

	/* clear all */
	for(i=0;i<BITMAP_INDEX_MAX;++i) {
		map[i].count = 0;
		map[i].index = 0;
		index1[i] = map + i;
	}

	/* count */
	for(y=0;y<bmp->size_y;++y) {
		unsigned x;
		uint8* line = (uint8*)bitmap_line((struct bitmap* )bmp,y);
		for(x=0;x<bmp->size_x;++x) {
			unsigned j;
			j = BITMAP_COLOR_TO_INDEX( line[0], line[1], line[2] );
			++map[j].count;
			line += 3;
		}
	}

	/* sort */
	countsort80(index1,index2);
	countsort88(index2,index1);

	/* create palette */
	for(i=0;i<size && index1[BITMAP_INDEX_MAX - i - 1]->count;++i) {
		unsigned subindex = BITMAP_INDEX_MAX - i - 1;
		unsigned j = index1[subindex] - map;
		index1[subindex]->index = i + 1;
		palette[i].red = BITMAP_INDEX_TO_RED( j );
		palette[i].green = BITMAP_INDEX_TO_GREEN( j );
		palette[i].blue = BITMAP_INDEX_TO_BLUE( j );
		palette[i].alpha = 0;
	}
	res_size = i;

	/* make index table */
	for(i=0;i<BITMAP_INDEX_MAX;++i) {
		if (map[i].index) {
			convert[i] = map[i].index - 1;
		} else if (map[i].count) {
			int red = BITMAP_INDEX_TO_RED( i );
			int green = BITMAP_INDEX_TO_GREEN( i );
			int blue = BITMAP_INDEX_TO_BLUE( i );
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

struct bitmap* bitmap_resize(struct bitmap* bmp, unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned sx, unsigned sy, unsigned orientation) {
	struct bitmap* newbmp;
	unsigned i,j;
	unsigned* x_map;

	if (!sx || !sy || !dx || !dy)
		return 0;

	/* new ptr */
	newbmp = bitmap_alloc(sx,sy,bmp->bytes_per_pixel * 8);
	assert( newbmp );

	x_map = malloc(sizeof(unsigned) * sx);
	assert(x_map);

	// set orientation (only mirror) and scaling
	for(i=0;i<sx;++i) {
		unsigned xv = i * (dx-1) / (sx-1);
		if (orientation & ORIENTATION_MIRROR_X)
			xv = dx - xv - 1;
		x_map[i] = xv + x;
	}

	if (bmp->bytes_per_pixel == 1) {
		// 8 bit
		for(j=0;j<sy;++j) {
			uint8* src;
			uint8* dst;
			unsigned yv = j * (dy-1) / (sy-1);
			if (orientation & ORIENTATION_MIRROR_Y)
				yv = dy - yv - 1;
			src = (uint8*)bitmap_line(bmp,yv + y);
			dst = (uint8*)bitmap_line(newbmp,j);
			for(i=0;i<sx;++i) {
				*dst = src[x_map[i]];
				++dst;
			}
		}
	} else if (bmp->bytes_per_pixel == 3) {
		// 24 bit
		for(j=0;j<sy;++j) {
			uint8* src;
			uint8* dst;
			unsigned yv = j * dy / sy;
			if (orientation & ORIENTATION_MIRROR_Y)
				yv = dy - yv - 1;
			src = (uint8*)bitmap_line(bmp,yv + y);
			dst = (uint8*)bitmap_line(newbmp,j);
			for(i=0;i<sx;++i) {
				unsigned off = x_map[i] * 3;
				dst[0] = src[off];
				dst[1] = src[off+1];
				dst[2] = src[off+2];
				dst += 3;
			}
		}
	}

	free(x_map);
	return newbmp;
}

struct bitmap* bitmap_addborder(struct bitmap* bmp, unsigned x0, unsigned x1, unsigned y0, unsigned y1, unsigned color) {
	struct bitmap* newbmp;
	unsigned i,j;
	unsigned sx = bmp->size_x;
	unsigned sy = bmp->size_y;
	unsigned tx = sx+x0+x1;
	unsigned ty = sy+y0+y1;

	/* new ptr */
	newbmp = bitmap_alloc(tx,ty,bmp->bytes_per_pixel * 8);
	assert( newbmp );

	if (bmp->bytes_per_pixel == 1) {
		// 8 bit
		for(j=0;j<y0;++j) {
			uint8* dst;
			dst = (uint8*)bitmap_line(newbmp,j);
			for(i=0;i<tx;++i)
				*dst++ = color;
		}
		for(j=0;j<sy;++j) {
			uint8* src;
			uint8* dst;
			src = (uint8*)bitmap_line(bmp,j);
			dst = (uint8*)bitmap_line(newbmp,j+y0);
			for(i=0;i<x0;++i)
				*dst++ = color;
			for(i=0;i<sx;++i)
				*dst++ = src[i];
			for(i=0;i<x1;++i)
				*dst++ = color;
		}
		for(j=0;j<y1;++j) {
			uint8* dst;
			dst = (uint8*)bitmap_line(newbmp,j+y0+sy);
			for(i=0;i<tx;++i)
				*dst++ = color;
		}
	} else if (bmp->bytes_per_pixel == 3) {
		// 24 bit
		unsigned c0 = color & 0xFF;
		unsigned c1 = (color >> 8) & 0xFF;
		unsigned c2 = (color >> 16) & 0xFF;
		for(j=0;j<y0;++j) {
			uint8* dst;
			dst = (uint8*)bitmap_line(newbmp,j);
			for(i=0;i<tx;++i) {
				*dst++ = c0;
				*dst++ = c1;
				*dst++ = c2;
			}
		}
		for(j=0;j<sy;++j) {
			uint8* src;
			uint8* dst;
			src = (uint8*)bitmap_line(bmp,j);
			dst = (uint8*)bitmap_line(newbmp,j+y0);
			for(i=0;i<x0;++i) {
				*dst++ = c0;
				*dst++ = c1;
				*dst++ = c2;
			}
			for(i=0;i<sx;++i) {
				dst[0] = src[i*3];
				dst[1] = src[i*3+1];
				dst[2] = src[i*3+2];
				dst += 3;
			}
			for(i=0;i<x1;++i) {
				*dst++ = c0;
				*dst++ = c1;
				*dst++ = c2;
			}
		}
		for(j=0;j<y1;++j) {
			uint8* dst;
			dst = (uint8*)bitmap_line(newbmp,j+y0+sy);
			for(i=0;i<tx;++i) {
				*dst++ = c0;
				*dst++ = c1;
				*dst++ = c2;
			}
		}
	}

	return newbmp;
}

void bitmap_cutoff(struct bitmap* bitmap, unsigned* _cx, unsigned* _cy) {
	unsigned yu = 0;
	unsigned yd = bitmap->size_y - 1;

	unsigned cx = bitmap->size_x / 2;
	unsigned cy = bitmap->size_y / 2;

	if (bitmap->bytes_per_pixel == 1) {
		uint8 c = *(uint8*)bitmap_line(bitmap,0);
		while (yu < yd && cx) {
			unsigned i;
			uint8* pu = (uint8*)bitmap_line(bitmap,yu);
			uint8* pd = (uint8*)bitmap_line(bitmap,yd);

			i = 0;
			while (i < cx && pu[i]==c && pu[bitmap->size_x - i -1]==c)
				++i;
			cx = i;

			i = 0;
			while (i < cx && pd[i]==c && pd[bitmap->size_x - i -1]==c)
				++i;
			cx = i;

			if (yu < cy && cx != bitmap->size_x / 2)
				cy = yu;

			++yu;
			--yd;
		}
	} else if (bitmap->bytes_per_pixel == 2) {
		uint16 c = *(uint16*)bitmap_line(bitmap,0);
		while (yu < yd && cx) {
			unsigned i;
			uint16* pu = (uint16*)bitmap_line(bitmap,yu);
			uint16* pd = (uint16*)bitmap_line(bitmap,yd);

			i = 0;
			while (i < cx && pu[i]==c && pu[bitmap->size_x - i -1]==c)
				++i;
			cx = i;

			i = 0;
			while (i < cx && pd[i]==c && pd[bitmap->size_x - i -1]==c)
				++i;
			cx = i;

			if (yu < cy && cx != bitmap->size_x / 2)
				cy = yu;

			++yu;
			--yd;
		}
	} else if (bitmap->bytes_per_pixel == 3) {
		uint8* dot = (uint8*)bitmap_line(bitmap,0);
		uint8 c0 = dot[0];
		uint8 c1 = dot[1];
		uint8 c2 = dot[2];
		while (yu < yd && cx) {
			unsigned i;
			unsigned i3;
			unsigned si3;
			uint8* pu = (uint8*)bitmap_line(bitmap,yu);
			uint8* pd = (uint8*)bitmap_line(bitmap,yd);

			i = 0;
			i3 = 0;
			si3 = bitmap->size_x*3 - 3;
			while (i < cx && pu[i3]==c0 && pu[i3+1]==c1 && pu[i3+2]==c2 && pu[si3]==c0 && pu[si3+1]==c1 && pu[si3+2]==c2) {
				++i;
				i3 += 3;
				si3 -= 3;
			}
			cx = i;

			i = 0;
			i3 = 0;
			si3 = bitmap->size_x*3 - 3;
			while (i < cx && pd[i3]==c0 && pd[i3+1]==c1 && pd[i3+2]==c2 && pd[si3]==c0 && pd[si3+1]==c1 && pd[si3+2]==c2) {
				++i;
				i3 += 3;
				si3 -= 3;
			}
			cx = i;

			if (yu < cy && cx != bitmap->size_x / 2)
				cy = yu;

			++yu;
			--yd;
		}
	} else {
		cx = 0;
		cy = 0;
	}

	*_cx = cx;
	*_cy = cy;
}

void bitmap_cvt_8to8(struct bitmap* dst, struct bitmap* src, unsigned* color_map) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint8* dst_ptr = bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			*dst_ptr = color_map[*src_ptr];
			dst_ptr += 1;
			src_ptr += 1;
		}
	}
}

void bitmap_cvt_24to8rgb(struct bitmap* dst, struct bitmap* src) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint8* dst_ptr = bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			video_rgb color;
			video_rgb_make(&color,src_ptr[0],src_ptr[1],src_ptr[2]);
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 3;
		}
	}
}

void bitmap_cvt_24to8idx(struct bitmap* dst, struct bitmap* src, unsigned* convert_map) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint8* dst_ptr = bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned color = convert_map[BITMAP_COLOR_TO_INDEX(src_ptr[0],src_ptr[1],src_ptr[2])];
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 3;
		}
	}
}

void bitmap_cvt_8to16(struct bitmap* dst, struct bitmap* src, unsigned* color_map) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint16* dst_ptr = (uint16*)bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned color = color_map[*src_ptr];
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
	}
}

void bitmap_cvt_24to16(struct bitmap* dst, struct bitmap* src) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint16* dst_ptr = (uint16*)bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			video_rgb color;
			video_rgb_make(&color,src_ptr[0],src_ptr[1],src_ptr[2]);
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 3;
		}
	}
}

void bitmap_cvt_8to32(struct bitmap* dst, struct bitmap* src, unsigned* color_map) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint32* dst_ptr = (uint32*)bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			unsigned color = color_map[*src_ptr];
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
	}
}

void bitmap_cvt_24to32(struct bitmap* dst, struct bitmap* src) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint32* dst_ptr = (uint32*)bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			video_rgb color;
			video_rgb_make(&color,src_ptr[0],src_ptr[1],src_ptr[2]);
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 3;
		}
	}
}

void bitmap_cvt_32to24(struct bitmap* dst, struct bitmap* src) {
	unsigned cx,cy;
	for(cy=0;cy<src->size_y;++cy) {
		uint8* src_ptr = bitmap_line(src,cy);
		uint8* dst_ptr = bitmap_line(dst,cy);
		for(cx=0;cx<src->size_x;++cx) {
			dst_ptr[0] = src_ptr[0];
			dst_ptr[1] = src_ptr[1];
			dst_ptr[2] = src_ptr[2];
			dst_ptr += 3;
			src_ptr += 4;
		}
	}
}

