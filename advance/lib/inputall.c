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

#include "inputall.h"
#include "snstring.h"

/**
 * Register all the input drivers.
 * The drivers are registered on the basis of the following defines:
 *  - USE_INPUT_DOS
 *  - USE_INPUT_SDL
 *  - USE_INPUT_TTY
 *  - USE_INPUT_NONE
 */
void inputb_reg_driver_all(adv_conf* context)
{
#ifdef USE_INPUT_DOS
	inputb_reg_driver(context, &inputb_dos_driver);
#endif
#ifdef USE_INPUT_TTY
	inputb_reg_driver(context, &inputb_tty_driver);
#endif
#ifdef USE_INPUT_SDL
	inputb_reg_driver(context, &inputb_sdl_driver);
#endif
#ifdef USE_INPUT_NONE
	inputb_reg_driver(context, &inputb_none_driver);
#endif
}

/**
 * Report the available drivers.
 * The driver names are copied in the string separated by spaces.
 */
void inputb_report_driver_all(char* s, unsigned size)
{
	*s = 0;
#ifdef USE_INPUT_DOS
	sncat(s, size, " dos");
#endif
#ifdef USE_INPUT_TTY
	sncat(s, size, " tty");
#endif
#ifdef USE_INPUT_SDL
	sncat(s, size, " sdl");
#endif
#ifdef USE_INPUT_NONE
	sncat(s, size, " none");
#endif
}

