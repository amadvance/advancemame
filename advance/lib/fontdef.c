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
 */

#include "portable.h"

#include "font.h"
#include "fz.h"

#include "fontdef.dat"

/**
 * Return the default font.
 * \param height Height in pixel of the required font. The effective height may differs.
 */
adv_font* adv_font_default(unsigned sizex, unsigned sizey, adv_bool disable_alpha) {
	adv_fz* f;
	adv_font* font;
	unsigned fx;
	unsigned fy;

#ifdef USE_FREETYPE
	if (!disable_alpha && sizey>=13) {
		f = fzopenmemory(FONT_TTF, sizeof(FONT_TTF));

		font = adv_font_load(f, sizex, sizey);
		if (font) {
			fzclose(f);
			return font;
		}
	}
#endif

	if (sizey >= 17) {
		f = fzopenmemory(FONT_HELV17, sizeof(FONT_HELV17));
	} else if (sizey >= 15) {
		f = fzopenmemory(FONT_HELV15, sizeof(FONT_HELV15));
	} else if (sizey >= 13) {
		f = fzopenmemory(FONT_HELV13, sizeof(FONT_HELV13));
	} else {
		f = fzopenmemory(FONT_HELV11, sizeof(FONT_HELV11));
	}

	font = adv_font_load(f, sizex, sizey);

	fx = 0;
	if (adv_font_sizex_char(font, 'M'))
		fx = sizex / adv_font_sizex_char(font, 'M');
	if (fx == 0)
		fx = 1;

	fy = 0;
	if (adv_font_sizey_char(font, 'M'))
		fy = sizey / adv_font_sizey_char(font, 'M');
	if (fy == 0)
		fy = 1;

	if (fx!=1 || fy!=1)
		adv_font_scale(font, fx, fy);

	fzclose(f);

	return font;
}

