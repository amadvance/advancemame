/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#ifndef __OSLINUX_H
#define __OSLINUX_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Internal */

void target_usleep_granularity(unsigned us);

/* Check if svgalib is used in some way */
#if defined(USE_VIDEO_SVGALIB) || defined(USE_KEYBOARD_SVGALIB) || defined(USE_MOUSE_SVGALIB) || defined(USE_JOYSTICK_SVGALIB)
#define USE_SVGALIB
void* os_internal_svgalib_get(void);
int os_internal_svgalib_is_video_active(void);
int os_internal_svgalib_is_video_mode_active(void);
#endif

/* Check if FB is used in some way */
#if defined(USE_VIDEO_FB)
#define USE_FB
void* os_internal_fb_get(void);
int os_internal_fb_is_video_active(void);
int os_internal_fb_is_video_mode_active(void);
#endif

/* Check if sLang is used in some way */
#if defined(USE_VIDEO_SLANG)
#define USE_SLANG
void* os_internal_slang_get(void);
#endif

#if defined(USE_VIDEO_CURSES)
#define USE_CURSES
void* os_internal_curses_get(void);
#endif

/* Check if X is used in some way */
#if defined(USE_VIDEO_X) || defined(USE_KEYBOARD_X) || defined(USE_MOUSE_X)
#define USE_X
void* os_internal_x_get(void);
#endif

/* Check if SDL is used in some way */
#if defined(USE_VIDEO_SDL) || defined(USE_KEYBOARD_SDL) || defined(USE_MOUSE_SDL) || defined(USE_JOYSTICK_SDL) || defined(USE_SOUND_SDL) || defined(USE_INPUT_SDL)
#define USE_SDL
#include "ossdl.h"
#endif

/* Check if input-event is used in some way */
#if defined(USE_KEYBOARD_EVENT) || defined(USE_MOUSE_EVENT) || defined(USE_JOYSTICK_EVENT)
#define USE_EVENT
#endif

/**
 * Check if a window manager is active. Otherwise we are in a console system.
 */
adv_bool os_internal_wm_active(void);

#ifdef __cplusplus
}
#endif

#endif
