/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003 Andrea Mazzoleni
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
 * Internal interface for the "linux" host.
 */

#ifndef __OSWIN_H
#define __OSWIN_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Internal */

void target_usleep_granularity(unsigned us);

/* Check if SVGAWIN is used in some way */
#if defined(USE_VIDEO_SVGAWIN)
#define USE_SVGAWIN
void* os_internal_svgawin_get(void);
int os_internal_svgawin_is_video_active(void);
int os_internal_svgawin_is_video_mode_active(void);
#endif

/* Check if SDL is used in some way */
#if defined(USE_VIDEO_SDL) || defined(USE_KEYBOARD_SDL) || defined(USE_MOUSE_SDL) || defined(USE_JOYSTICK_SDL) || defined(USE_SOUND_SDL) || defined(USE_INPUT_SDL)
#define USE_SDL
#include "ossdl.h"
#endif

#ifdef __cplusplus
}
#endif

#endif

