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

/** \file
 * Font.
 */

#ifndef __FONT_H
#define __FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bitmap.h"

/**
 * Max number of chars in a font.
 */
#define BITMAP_FONT_MAX 256

/**
 * Font.
 */
typedef struct adv_font_struct {
	adv_bitmap* data[BITMAP_FONT_MAX]; /**< A bitmap for every ASCII char. */
} adv_font;

/** \addtogroup Font */
/*@{*/

adv_font* font_import_raw(unsigned char* data, unsigned data_size);
adv_font* font_import_grx(unsigned char* data);

adv_font* font_load(const char* file);
void font_free(adv_font* font);

unsigned font_size_x(adv_font* font);
unsigned font_size_y(adv_font* font);

void font_orientation(adv_font* font, unsigned orientation_mask);

/*@}*/

#ifdef __cplusplus
};
#endif

#endif


