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

#include "soundall.h"

/**
 * Register all the sound drivers.
 * The drivers are registered on the basis of the following defines:
 *  - USE_SOUND_SEAL
 *  - USE_SOUND_ALLEGRO
 *  - USE_SOUND_VSYNC
 *  - USE_SOUND_OSS
 *  - USE_SOUND_SDL
 *  - USE_SOUND_NONE
 */
void sound_reg_driver_all(adv_conf* context) {
#ifdef USE_SOUND_SEAL
	sound_reg_driver(context, &sound_seal_driver);
#endif
#ifdef USE_SOUND_ALLEGRO
	sound_reg_driver(context, &sound_allegro_driver);
#endif
#ifdef USE_SOUND_VSYNC
	sound_reg_driver(context, &sound_vsync_driver);
#endif
#ifdef USE_SOUND_OSS
	sound_reg_driver(context, &sound_oss_driver);
#endif
#ifdef USE_SOUND_SDL
	sound_reg_driver(context, &sound_sdl_driver);
#endif
#ifdef USE_SOUND_NONE
	sound_reg_driver(context, &sound_none_driver);
#endif
}

