/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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
 * Advance Library main include.
 */

/** \defgroup Functionality Functionalities */
/*@{*/
/** \defgroup Error Error */
/** \defgroup LCD LCD */
/** \defgroup Log Log */
/** \defgroup Color Color */
/** \defgroup Complex Complex */
/** \defgroup BitMap BitMap */
/** \defgroup Configuration Configuration */
/** \defgroup Crtc Crtc */
/** \defgroup DFT DFT */
/** \defgroup Generate Crtc Generation */
/** \defgroup Filter Filter */
/** \defgroup Font Font */
/** \defgroup Blit Blit */
/** \defgroup Monitor Monitor */
/** \defgroup Update Update */
/** \defgroup Mode Mode */
/** \defgroup String Dynamic String */
/** \defgroup SafeString Safe String */
/** \defgroup Info Info */
/** \defgroup Mixer Mixer */
/*@}*/
/** \defgroup Stream Streams */
/*@{*/
/** \defgroup AudioFile Audio */
/** \defgroup VideoFile Video */
/** \defgroup CompressedFile Compressed */
/** \defgroup ZIPFile ZIP */
/*@}*/
/** \defgroup Driver Drivers */
/*@{*/
/** \defgroup Device Device */
/** \defgroup Video Video */
/** \defgroup Input Input */
/** \defgroup Joystick Joystick */
/** \defgroup Keyboard Keyboard */
/** \defgroup Mouse Mouse */
/** \defgroup Sound Sound */
/*@}*/
/** \defgroup Portable Portable */
/*@{*/
/** \defgroup Type Type */
/** \defgroup System System */
/** \defgroup Target Target */
/** \defgroup File File */
/** \defgroup Endian Endian */
/*@}*/

#ifndef __ADVANCE_H
#define __ADVANCE_H

#include "bitmap.h"
#include "conf.h"
#include "crtc.h"
#include "crtcbag.h"
#include "device.h"
#include "endianrw.h"
#include "error.h"
#include "extra.h"
#include "file.h"
#include "font.h"
#include "fontdef.h"
#include "fz.h"
#include "generate.h"
#include "gtf.h"
#include "icon.h"
#include "incstr.h"
#include "inone.h"
#include "inputall.h"
#include "inputdrv.h"
#include "jnone.h"
#include "joyall.h"
#include "joydrv.h"
#include "key.h"
#include "keyall.h"
#include "keydrv.h"
#include "knone.h"
#include "log.h"
#include "measure.h"
#include "mixer.h"
#include "mng.h"
#include "mnone.h"
#include "mode.h"
#include "monitor.h"
#include "mouseall.h"
#include "mousedrv.h"
#include "os.h"
#include "pcx.h"
#include "png.h"
#include "pngdef.h"
#include "readinfo.h"
#include "rgb.h"
#include "snone.h"
#include "snstring.h"
#include "soundall.h"
#include "sounddrv.h"
#include "target.h"
#include "unzip.h"
#include "update.h"
#include "video.h"
#include "videoall.h"
#include "videodrv.h"
#include "vnone.h"
#include "wave.h"
#include "filter.h"
#include "dft.h"

#include "clear.h"
#include "blit.h"

#endif

