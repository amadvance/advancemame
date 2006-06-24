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

#include "joyall.h"
#include "snstring.h"

/**
 * Register all the joystick drivers.
 * The drivers are registered on the basis of the following defines:
 *  - USE_JOYSTICK_EVENT
 *  - USE_JOYSTICK_SVGALIB
 *  - USE_JOYSTICK_RAW
 *  - USE_JOYSTICK_LGRAWINPUT
 *  - USE_JOYSTICK_SDL
 *  - USE_JOYSTICK_ALLEGRO
 *  - USE_JOYSTICK_LGALLEGRO
 *  - USE_JOYSTICK_NONE
 */
void joystickb_reg_driver_all(adv_conf* context)
{
	/* the order is also the detection precedence */
#ifdef USE_JOYSTICK_EVENT
	joystickb_reg_driver(context, &joystickb_event_driver);
#endif
#ifdef USE_JOYSTICK_SVGALIB
	joystickb_reg_driver(context, &joystickb_svgalib_driver);
#endif
#ifdef USE_JOYSTICK_RAW
	joystickb_reg_driver(context, &joystickb_raw_driver);
#endif
#ifdef USE_JOYSTICK_LGRAWINPUT
	joystickb_reg_driver(context, &joystickb_lgrawinput_driver);
#endif
#ifdef USE_JOYSTICK_SDL
	joystickb_reg_driver(context, &joystickb_sdl_driver);
#endif
#ifdef USE_JOYSTICK_ALLEGRO
	joystickb_reg_driver(context, &joystickb_allegro_driver);
#endif
#ifdef USE_JOYSTICK_LGALLEGRO
	joystickb_reg_driver(context, &joystickb_lgallegro_driver);
#endif
#ifdef USE_JOYSTICK_NONE
	joystickb_reg_driver(context, &joystickb_none_driver);
#endif
}

/**
 * Report the available drivers.
 * The driver names are copied in the string separated by spaces.
 */
void joystickb_report_driver_all(char* s, unsigned size)
{
	*s = 0;

	/* the order is not relevant */
#ifdef USE_JOYSTICK_EVENT
	sncat(s, size, " event");
#endif
#ifdef USE_JOYSTICK_SVGALIB
	sncat(s, size, " svgalib");
#endif
#ifdef USE_JOYSTICK_RAW
	sncat(s, size, " raw");
#endif
#ifdef USE_JOYSTICK_LGRAWINPUT
	sncat(s, size, " lgrawinput");
#endif
#ifdef USE_JOYSTICK_SDL
	sncat(s, size, " sdl");
#endif
#ifdef USE_JOYSTICK_ALLEGRO
	sncat(s, size, " allegro");
#endif
#ifdef USE_JOYSTICK_LGALLEGRO
	sncat(s, size, " lgallegro");
#endif
#ifdef USE_JOYSTICK_NONE
	sncat(s, size, " none");
#endif
}

