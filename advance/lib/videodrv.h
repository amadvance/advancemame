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
 * Video driver definitions.
 */

/** \addtogroup Video */
/*@{*/

#ifndef __VIDEODRV_H
#define __VIDEODRV_H

#include "extra.h"
#include "device.h"
#include "conf.h"
#include "crtc.h"
#include "crtcbag.h"
#include "mode.h"
#include "rgb.h"

/***************************************************************************/
/* Driver */

/** \name Mode Flags
 * Video driver flags for the mode type.
 */
/*@{*/
#define VIDEO_DRIVER_FLAGS_MODE_PALETTE8 0x1 /**< Support palette 8 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_BGR8 0x2 /**< Support bgr 8 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_BGR15 0x4 /**< Support bgr 15 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_BGR16 0x8 /**< Support bgr 16 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_BGR24 0x10 /**< Support bgr 24 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_BGR32 0x20 /**< Support bgr 32 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_YUY2 0x40 /**< Support bgr 32 bit modes. */
#define VIDEO_DRIVER_FLAGS_MODE_TEXT 0x80 /**< Support text modes. */
#define VIDEO_DRIVER_FLAGS_MODE_MASK 0xFF /**< Mask for the VIDEO_DRIVER_FLAGS_MODE_* flags. */

#define VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK (VIDEO_DRIVER_FLAGS_MODE_MASK & ~VIDEO_DRIVER_FLAGS_MODE_TEXT) /**< Mask for the graph flags. */
#define VIDEO_DRIVER_FLAGS_MODE_TEXT_MASK (VIDEO_DRIVER_FLAGS_MODE_TEXT) /**< Mask for the text flags. */
/*@}*/


/** \name Programmable Flags
 * Video driver flags for the mode programmable capabilities.
 */
/*@{*/
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN 0x100 /**< Single scan mode. */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN 0x200 /**< Double scan mode. */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE 0x400 /**< Interlaced modes. */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK 0x800 /**< Programmable clock. */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC 0x1000 /**< Programmable size. */
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL (VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC)
#define VIDEO_DRIVER_FLAGS_PROGRAMMABLE_MASK 0xFF00 /**< Mask for the VIDEO_DRIVER_FLAGS_PROGRAMMABLE_* flags. */
/*@}*/

/** \name Default Flags
 * Video driver flags for the default video mode.
 */
/*@{*/
#define VIDEO_DRIVER_FLAGS_DEFAULT_NONE 0x00000 /**< None is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_PALETTE8 0x10000 /**< palette8 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_BGR8 0x20000 /**< bgr8 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_BGR15 0x30000 /**< bgr15 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_BGR16 0x40000 /**< bgr16 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_BGR24 0x50000 /**< bgr24 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_BGR32 0x60000 /**< bgr32 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_YUY2 0x70000 /**< yuy2 is the preferred choice. */
#define VIDEO_DRIVER_FLAGS_DEFAULT_MASK 0xF0000
/*@}*/

/** \name Output Flags
 * Video driver output mode.
 */
/*@{*/
/**
 * If the program is runned in fullscreen.
 */
#define VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN 0x100000

/**
 * If the program is runned as a window.
 * It imply that the game is shown as a window. So, all the window
 * sizes are possible.
 */
#define VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW 0x200000

/**
 * If the program is runned in fullscreen with an overlay.
 * It imply that the game is always zoomed to fit the whole screen.
 * So, all the screen sizes are possible.
 */
#define VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY 0x400000

#define VIDEO_DRIVER_FLAGS_OUTPUT_MASK 0xF00000 /**< Mask for the VIDEO_DRIVER_FLAGS_OUTPUT_* flags. */
/*@}*/

/** \name Internal Flags
 * Video driver flags used internally by the drivers.
 */
/*@{*/
/**
 * Dangerous mode change.
 * If this flag is reported the video mode change from a graphics
 * video mode to another graphics mode is done directly only if the
 * the device_video_fastchange option is set. Otherwise the video mode is
 * first restored to the initial state before setting the new
 * graphics mode.
 */
#define VIDEO_DRIVER_FLAGS_INTERNAL_DANGEROUSCHANGE 0x10000000
#define VIDEO_DRIVER_FLAGS_INTERNAL_BIT0 0x20000000 /**< First internal flag for the user. */
#define VIDEO_DRIVER_FLAGS_INTERNAL_MASK 0xF0000000 /**< Available internal flags. */
/*@}*/

/**
 * Output mode of a video driver.
 */
typedef enum adv_output_enum {
	adv_output_auto = -1, /**< Automatically detected. */
	adv_output_fullscreen = 0, /**< Fullscreen mode of operation. */
	adv_output_window = 1, /**< Window mode of operation. */
	adv_output_overlay = 2 /**< Fullscreen overlay mode of operation. */
} adv_output;

/**
 * Cursort mode of a video driver.
 */
typedef enum adv_cursor_enum {
	adv_cursor_auto = -1, /**< Automatically choice. On on window modes, off on fullscreen modes. */
	adv_cursor_off = 0, /**< Always off. */
	adv_cursor_on = 1, /**< Always on. */
} adv_cursor;

/**
 * Video driver.
 * This struct abstract all the driver funtionalities.
 */
typedef struct adv_video_driver_struct {
	const char* name; /**< Name of the main driver */
	const adv_device* device_map; /**< List of supported devices */

	/**
	 * Load the configuration options of the video driver.
	 * Call before init().
	 */
	adv_error (*load)(adv_conf* context);

	/**
	 * Register the load options of the video driver.
	 * Call before load().
	 */
	void (*reg)(adv_conf* context);

	/**
	 * Initialize the driver.
	 * \param id Choosen device from ::device_map.
	 * \param output Output mode.
	 */
	adv_error (*init)(int id, adv_output output, unsigned overlay_size, adv_cursor cursor);

	/**
	 * Deinitialize the driver.
	 */
	void (*done)(void);

	unsigned (*flags)(void); /**< Get the capabilities of the driver. */

	adv_error (*mode_set)(const void* mode); /**< Set a video mode. */
	adv_error (*mode_change)(const void* mode); /**< Change the video mode. */
	void (*mode_done)(adv_bool restore); /**< Reset a video mode. */

	/* Information of the current video mode */
	unsigned (*virtual_x)(void);
	unsigned (*virtual_y)(void);
	unsigned (*font_size_x)(void);
	unsigned (*font_size_y)(void);
	unsigned (*bytes_per_scanline)(void);
	unsigned (*adjust_bytes_per_page)(unsigned bytes_per_page);
	adv_color_def (*color_def)(void);

	/**
	 * Grant access in writing.
	 */
	void (*write_lock)(void);

	/**
	 * Remove the lock for writing.
	 */
	void (*write_unlock)(unsigned x, unsigned y, unsigned size_x, unsigned size_y, adv_bool waitvsync);

	/**
	 * Access the memory for writing.
	 * You must call write_lock() before this function.
	 */
	unsigned char* (**write_line)(unsigned y);

	/* Operations on the current video mode */
	void (*wait_vsync)(void);
	adv_error (*scroll)(unsigned offset, adv_bool waitvsync);
	adv_error (*scanline_set)(unsigned byte_length);
	adv_error (*palette8_set)(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);

	/** Return the size of the driver video mode struct */
	unsigned (*mode_size)(void);
	adv_error (*mode_grab)(void* mode);
	adv_error (*mode_generate)(void* mode, const adv_crtc* crtc, unsigned flags);
	adv_error (*mode_import)(adv_mode* mode, const void* driver_mode);
	int (*mode_compare)(const void* a, const void* b);

	/**
	 * Insert a set of default modelines in the container.
	 * All the available video modes are added in the container.
	 * If the video driver is programmable, none video mode is added.
	 */
	void (*crtc_container_insert_default)(adv_crtc_container* cc);
} adv_video_driver;

#endif

/*@}*/
