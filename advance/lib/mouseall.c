/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005 Andrea Mazzoleni
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

#include "mouseall.h"
#include "snstring.h"

/**
 * Register all the mouse drivers.
 * The drivers are registered on the basis of the following defines:
 *  - USE_MOUSE_RAWINPUT
 *  - USE_MOUSE_CPN
 *  - USE_MOUSE_SDL
 *  - USE_MOUSE_EVENT
 *  - USE_MOUSE_SVGALIB
 *  - USE_MOUSE_RAW
 *  - USE_MOUSE_ALLEGRO
 *  - USE_MOUSE_NONE
 */
void mouseb_reg_driver_all(adv_conf* context)
{
	/* the order is also the detection precedence */
#ifdef USE_MOUSE_RAWINPUT
	mouseb_reg_driver(context, &mouseb_rawinput_driver);
#endif
#ifdef USE_MOUSE_CPN
	mouseb_reg_driver(context, &mouseb_cpn_driver);
#endif
#ifdef USE_MOUSE_SDL
	mouseb_reg_driver(context, &mouseb_sdl_driver);
#endif
#ifdef USE_MOUSE_EVENT
	mouseb_reg_driver(context, &mouseb_event_driver);
#endif
#ifdef USE_MOUSE_SVGALIB
	mouseb_reg_driver(context, &mouseb_svgalib_driver);
#endif
#ifdef USE_MOUSE_RAW
	mouseb_reg_driver(context, &mouseb_raw_driver);
#endif
#ifdef USE_MOUSE_ALLEGRO
	mouseb_reg_driver(context, &mouseb_allegro_driver);
#endif
#ifdef USE_MOUSE_NONE
	mouseb_reg_driver(context, &mouseb_none_driver);
#endif
}

/**
 * Report the available drivers.
 * The driver names are copied in the string separated by spaces.
 */
void mouseb_report_driver_all(char* s, unsigned size)
{
	*s = 0;

	/* the order is not relevant */
#ifdef USE_MOUSE_RAWINPUT
	sncat(s, size, " rawinput");
#endif
#ifdef USE_MOUSE_CPN
	sncat(s, size, " cpn");
#endif
#ifdef USE_MOUSE_EVENT
	sncat(s, size, " event");
#endif
#ifdef USE_MOUSE_SVGALIB
	sncat(s, size, " svgalib");
#endif
#ifdef USE_MOUSE_RAW
	sncat(s, size, " raw");
#endif
#ifdef USE_MOUSE_SDL
	sncat(s, size, " sdl");
#endif
#ifdef USE_MOUSE_ALLEGRO
	sncat(s, size, " allegro");
#endif
#ifdef USE_MOUSE_NONE
	sncat(s, size, " none");
#endif
}

