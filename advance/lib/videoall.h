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

/** \file
 * All the video drivers.
 */

#ifndef __VIDEOALL_H
#define __VIDEOALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "conf.h"
#include "videodrv.h"

#ifdef USE_VIDEO_VGA
#include "vvga.h"
#endif
#ifdef USE_VIDEO_VGALINE
#include "vvgal.h"
#endif
#ifdef USE_VIDEO_SVGALINE
#include "vsvgal.h"
#endif
#ifdef USE_VIDEO_VBE
#include "vvbe.h"
#endif
#ifdef USE_VIDEO_VBELINE
#include "scrvbe.h"
#include "vvbel.h"
#endif
#ifdef USE_VIDEO_SVGALIB
#include "vsvgab.h"
#endif
#ifdef USE_VIDEO_FB
#include "vfb.h"
#endif
#ifdef USE_VIDEO_SLANG
#include "vslang.h"
#endif
#ifdef USE_VIDEO_CURSES
#include "vcurses.h"
#endif
#ifdef USE_VIDEO_SDL
#include "vsdl.h"
#endif
#ifdef USE_VIDEO_SVGAWIN
#include "vsvgawin.h"
#endif
#ifdef USE_VIDEO_NONE
#include "vnone.h"
#endif

/** \addtogroup Video */
/*@{*/

void video_reg_driver_all(adv_conf* context);
void video_report_driver_all(char* s, unsigned size);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
