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

#include "font.h"
#include "fz.h"

#include "fontdef.dat"

/**
 * Return the default font.
 * \param height Height in pixel of the required font. The effective height may differs.
 */
adv_font* adv_font_default(unsigned sizex, unsigned sizey, adv_bool disable_alpha)
{
	adv_fz* f;
	adv_font* font;

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

	fzclose(f);

	return font;
}

