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

#include "font.h"

#include "fontdef.dat"

/**
 * Return the default font.
 * \param height Height in pixel of the required font. The effective height may differs.
 */
adv_font* font_default(unsigned height) {
	if (height >= 17)
		return font_import_grx(FONT_HELV17);
	if (height >= 15)
		return font_import_grx(FONT_HELV15);
	if (height >= 13)
		return font_import_grx(FONT_HELV13);
	return font_import_grx(FONT_HELV11);
}
