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

#include "soundall.h"
#include "snstring.h"

/**
 * Register all the sound drivers.
 * The drivers are registered on the basis of the following defines:
 *  - USE_SOUND_SEAL
 *  - USE_SOUND_ALLEGRO
 *  - USE_SOUND_VSYNC
 *  - USE_SOUND_ALSA
 *  - USE_SOUND_OSS
 *  - USE_SOUND_SDL
 *  - USE_SOUND_NONE
 */
void soundb_reg_driver_all(adv_conf* context)
{
#ifdef USE_SOUND_SEAL
	soundb_reg_driver(context, &soundb_seal_driver);
#endif
#ifdef USE_SOUND_ALLEGRO
	soundb_reg_driver(context, &soundb_allegro_driver);
#endif
#ifdef USE_SOUND_VSYNC
	soundb_reg_driver(context, &soundb_vsync_driver);
#endif
#ifdef USE_SOUND_ALSA
	soundb_reg_driver(context, &soundb_alsa_driver);
#endif
#ifdef USE_SOUND_OSS
	soundb_reg_driver(context, &soundb_oss_driver);
#endif
#ifdef USE_SOUND_SDL
	soundb_reg_driver(context, &soundb_sdl_driver);
#endif
#ifdef USE_SOUND_NONE
	soundb_reg_driver(context, &soundb_none_driver);
#endif
}

/**
 * Report the available drivers.
 * The driver names are copied in the string separated by spaces.
 */
void soundb_report_driver_all(char* s, unsigned size)
{
	*s = 0;
#ifdef USE_SOUND_SEAL
	sncat(s, size, " seal");
#endif
#ifdef USE_SOUND_ALLEGRO
	sncat(s, size, " allegro");
#endif
#ifdef USE_SOUND_VSYNC
	sncat(s, size, " vsync");
#endif
#ifdef USE_SOUND_ALSA
	sncat(s, size, " alsa");
#endif
#ifdef USE_SOUND_OSS
	sncat(s, size, " oss");
#endif
#ifdef USE_SOUND_SDL
	sncat(s, size, " sdl");
#endif
#ifdef USE_SOUND_NONE
	sncat(s, size, " none");
#endif
}
