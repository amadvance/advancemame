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

#include "videoall.h"
#include "video.h"
#include "snstring.h"

/**
 * Register all the video drivers.
 * The drivers are registered on the basis of the following defines:
 *  - USE_VIDEO_SVGALIB
 *  - USE_VIDEO_SVGALINE
 *  - USE_VIDEO_SVGAWIN
 *  - USE_VIDEO_FB
 *  - USE_VIDEO_VBELINE
 *  - USE_VIDEO_VGALINE
 *  - USE_VIDEO_SDL
 *  - USE_VIDEO_VBE
 *  - USE_VIDEO_SLANG
 *  - USE_VIDEO_CURSES
 *  - USE_VIDEO_NONE
 */
void video_reg_driver_all(adv_conf* context)
{
#ifdef USE_VIDEO_SVGALIB
	video_reg_driver(context, &video_svgalib_driver);
#endif
#ifdef USE_VIDEO_SVGALINE
	video_reg_driver(context, &video_svgaline_driver);
#endif
#ifdef USE_VIDEO_SVGAWIN
	video_reg_driver(context, &video_svgawin_driver);
#endif
#ifdef USE_VIDEO_FB
	video_reg_driver(context, &video_fb_driver);
#endif
#ifdef USE_VIDEO_VBELINE
	video_reg_driver(context, &video_vbeline_driver);
#endif
#ifdef USE_VIDEO_VGALINE
	video_reg_driver(context, &video_vgaline_driver);
#endif
#ifdef USE_VIDEO_SDL
	video_reg_driver(context, &video_sdl_driver);
#endif
#ifdef USE_VIDEO_VBE
	video_reg_driver(context, &video_vbe_driver);
#endif
#ifdef USE_VIDEO_SLANG
	video_reg_driver(context, &video_slang_driver);
#endif
#ifdef USE_VIDEO_CURSES
	video_reg_driver(context, &video_curses_driver);
#endif
#ifdef USE_VIDEO_NONE
	video_reg_driver(context, &video_none_driver);
#endif
}

/**
 * Report the available drivers.
 * The driver names are copied in the string separated by spaces.
 */
void video_report_driver_all(char* s, unsigned size)
{
	*s = 0;
#ifdef USE_VIDEO_SVGALIB
	sncat(s, size, " svgalib");
#endif
#ifdef USE_VIDEO_SVGALINE
	sncat(s, size, " svgaline");
#endif
#ifdef USE_VIDEO_SVGAWIN
	sncat(s, size, " svgawin");
#endif
#ifdef USE_VIDEO_FB
	sncat(s, size, " fb");
#endif
#ifdef USE_VIDEO_VBELINE
	sncat(s, size, " vbeline");
#endif
#ifdef USE_VIDEO_VGALINE
	sncat(s, size, " vgaline");
#endif
#ifdef USE_VIDEO_SDL
	sncat(s, size, " sdl");
#endif
#ifdef USE_VIDEO_VBE
	sncat(s, size, " vbe");
#endif
#ifdef USE_VIDEO_SLANG
	sncat(s, size, " slang");
#endif
#ifdef USE_VIDEO_NONE
	sncat(s, size, " none");
#endif
}

