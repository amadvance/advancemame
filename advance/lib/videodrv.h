/*
 * This file is part of the AdvanceMAME project.
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

#ifndef __VIDEODRV_H
#define __VIDEODRV_H

#include "videostd.h"
#include "device.h"
#include "conf.h"
#include "crtc.h"
#include "mode.h"

/***************************************************************************/
/* Driver */

/* Flags for driver */
#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT 0x1 /**< Support 8 bit modes */
#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT 0x2 /**< Support 15 bit modes */
#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT 0x4 /**< Support 16 bit modes */
#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_24BIT 0x8 /**< Support 24 bit modes */
#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT 0x10 /**< Support 32 bit modes */
#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL (VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT | VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT | VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT | VIDEO_DRIVER_FLAGS_MODE_GRAPH_24BIT | VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT)
#define VIDEO_DRIVER_FLAGS_MODE_TEXT 0x20 /**< Support text mode */
#define VIDEO_DRIVER_FLAGS_MODE_MASK 0xFF /**< Mask for the MODE flags */

#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN 0x100 /**< Single scan mode */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN 0x200 /**< Double scan mode */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE 0x400 /**< Interlaced modes */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK 0x800 /**< Programmable clock */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC 0x1000 /**< Programmable size */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVPAL 0x2000 /**< Output TV-PAL */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVNTSC 0x4000 /**< Output TV-NTSC */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL (VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVPAL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVNTSC)
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_MASK 0xFF00

#define VIDEO_DRIVER_FLAGS_USER_BIT0 0x10000
#define VIDEO_DRIVER_FLAGS_USER_MASK 0xFFFF0000

struct video_driver_struct {
	const char* name; /**< Name of the main driver */
	const device* device_map; /**< List of supported devices */

	/** Load the configuration options. Call before init() */
	video_error (*load)(struct conf_context* context);

	/** Register the load options. Call before load(). */
	void (*reg)(struct conf_context* context);

	video_error (*init)(int id); /**< Initialize the driver */
	void (*done)(void); /**< Deinitialize the driver */

	unsigned (*flags)(void); /**< Get the capabilities of the driver */

	video_error (*mode_set)(const void* mode); /**< Set a video mode */
	video_error (*mode_change)(const void* mode); /**< Change the video mode */
	void (*mode_done)(video_bool restore); /**< Reset a video mode */

	/* Information of the current video mode */
	unsigned (*virtual_x)(void);
	unsigned (*virtual_y)(void);
	unsigned (*font_size_x)(void);
	unsigned (*font_size_y)(void);
	unsigned (*bytes_per_scanline)(void);
	unsigned (*adjust_bytes_per_page)(unsigned bytes_per_page);
	video_rgb_def (*rgb_def)(void);

	unsigned char* (**write_line)(unsigned y);

	/* Operations on the current video mode */
	void (*wait_vsync)(void);
	video_error (*scroll)(unsigned offset, int waitvsync);
	video_error (*scanline_set)(unsigned byte_length);
	video_error (*palette8_set)(const video_color* palette, unsigned start, unsigned count, int waitvsync);
	void (*unchained_plane_mask_set)(unsigned plane_mask);

	/** Return the size of the driver video mode struct */
	unsigned (*mode_size)(void);
	video_error (*mode_grab)(void* mode);
	video_error (*mode_generate)(void* mode, const video_crtc* crtc, unsigned bits, unsigned flags);
	video_error (*mode_import)(video_mode* mode, const void* driver_mode);
	int (*mode_compare)(const void* a, const void* b);
};

typedef struct video_driver_struct video_driver;

#endif
