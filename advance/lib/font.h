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
 * Font.
 */

#ifndef __FONT_H
#define __FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bitmap.h"
#include "fz.h"

/**
 * Max number of chars in a font.
 */
#define ADV_FONT_MAX 256

/**
 * Fixed width space.
 */
#define ADV_FONT_FIXSPACE '\x0ff'

/**
 * Font.
 */
typedef struct adv_font_struct {
	adv_bitmap* data[ADV_FONT_MAX]; /**< A bitmap for every ASCII char. */
} adv_font;

/** \addtogroup Font */
/*@{*/

adv_font* adv_font_load(adv_fz* f, unsigned sizex, unsigned sizey);
void adv_font_set_char(adv_font* font, char c, adv_bitmap* bitmap);
void adv_font_free(adv_font* font);

unsigned adv_font_sizex(adv_font* font);
unsigned adv_font_sizex_char(adv_font* font, char c);
unsigned adv_font_sizex_string(adv_font* font, const char* begin, const char* end);
const char* adv_font_sizex_limit(adv_font* font, const char* begin, const char* end, unsigned limit);
unsigned adv_font_sizey(adv_font* font);
unsigned adv_font_sizey_char(adv_font* font, char c);
unsigned adv_font_sizey_string(adv_font* font, const char* begin, const char* end);
const char* adv_font_sizey_limit(adv_font* font, const char* begin, const char* end, unsigned limit);

void adv_font_orientation(adv_font* font, unsigned orientation_mask);
void adv_font_scale(adv_font* font, unsigned fx, unsigned fy);

void adv_font_put_char(adv_font* font, adv_bitmap* dst, int x, int y, char c, unsigned color_front, unsigned color_back);
void adv_font_put_string(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front, unsigned color_back);
void adv_font_put_string_oriented(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front, unsigned color_back, unsigned orientation);
void adv_font_put_char_map(adv_font* font, adv_bitmap* dst, int x, int y, char c, const adv_pixel* map);
void adv_font_put_string_map(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, const adv_pixel* map);
void adv_font_put_char_trasp(adv_font* font, adv_bitmap* dst, int x, int y, char c, unsigned color_front);
void adv_font_put_string_trasp(adv_font* font, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned color_front);

/*@}*/

#ifdef __cplusplus
};
#endif

#endif


