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

#ifndef __FONT_H
#define __FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bitmap.h"

#define BITMAP_FONT_MAX 256

struct bitmapfont {
	struct bitmap* data[BITMAP_FONT_MAX];
};

struct bitmapfont* bitmapfont_inport_raw(unsigned char* data, unsigned data_size);
struct bitmapfont* bitmapfont_inport_grx(unsigned char* data);

struct bitmapfont* bitmapfont_load(const char* file);
void bitmapfont_free(struct bitmapfont* font);

unsigned bitmapfont_size_x(struct bitmapfont* font);
unsigned bitmapfont_size_y(struct bitmapfont* font);

void bitmapfont_orientation(struct bitmapfont* font, unsigned orientation_mask);

#ifdef __cplusplus
};
#endif

#endif
