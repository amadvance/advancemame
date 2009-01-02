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
 * Video mode.
 */

/** \addtogroup Mode */
/*@{*/

#ifndef __MODE_H
#define __MODE_H

#include "extra.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Video mode */

/** \name Flags sync
 * Flags describing the sync support of a video mode.
 */
/*@{*/
#define MODE_FLAGS_RETRACE_WAIT_SYNC  0x0010 /**< The wait syncronous operation is supported. */
#define MODE_FLAGS_RETRACE_SET_ASYNC  0x0020 /**< The set asyncronous (triple buffer) operation is supported. */
#define MODE_FLAGS_RETRACE_SET_SYNC   0x0040 /**< The set syncronous (double buffer) operation is supported. */
#define MODE_FLAGS_RETRACE_WRITE_SYNC 0x0080 /**< The write syncronous operation is supported. */
#define MODE_FLAGS_RETRACE_MASK       0x00F0 /**< Mask. */
/*@}*/

/** \name Flags index
 * Flags describing the color index of a video mode.
 */
/*@{*/
#define MODE_FLAGS_INDEX_NONE 0x0000 /**< None mode. */
#define MODE_FLAGS_INDEX_PALETTE8 0x0100 /**< Palette mode. */
#define MODE_FLAGS_INDEX_BGR8 0x0200 /**< BGR8 mode. */
#define MODE_FLAGS_INDEX_BGR15 0x0300 /**< BGR15 mode. */
#define MODE_FLAGS_INDEX_BGR16 0x0400 /**< BGR16 mode. */
#define MODE_FLAGS_INDEX_BGR24 0x0500 /**< BGR24 mode. */
#define MODE_FLAGS_INDEX_BGR32 0x0600 /**< BGR32 mode. */
#define MODE_FLAGS_INDEX_YUY2 0x0700 /**< YUY2 mode. */
#define MODE_FLAGS_INDEX_TEXT 0x0800 /**< Text mode. */
#define MODE_FLAGS_INDEX_MASK 0xFF00 /**< Mask. */
/*@}*/

/** \name Flags internal
 * Flags describing internak info.
 */
/*@{*/
#define MODE_FLAGS_INTERNAL_FAKERGB 0x10000 /**< If it's a rgb over a palette. */
#define MODE_FLAGS_INTERNAL_FAKETEXT 0x20000 /**< If it's a text mode over a graphics mode. */
#define MODE_FLAGS_INTERNAL_MASK 0xF0000 /**< Mask. */
/*@}*/

/** \name Flags user
 * Flags for the user.
 */
/*@{*/
#define MODE_FLAGS_USER_BIT0 0x01000000 /**< First user flag. */
#define MODE_FLAGS_USER_MASK 0xFF000000 /**< Mask. */
/*@}*/

/**
 * Max size of a driver video mode.
 */
#define MODE_DRIVER_MODE_SIZE_MAX 256

/**
 * Max length of video mode name.
 */
#define MODE_NAME_MAX 128

struct adv_video_driver_struct;

/**
 * Video mode specification.
 */
typedef struct vmode_struct {
	unsigned flags; /**< Flags. */
	char name[MODE_NAME_MAX]; /**< Name. */
	unsigned size_x; /**< Width. */
	unsigned size_y; /**< Height. */
	double vclock; /**< Vertical freq, ==0 if not computable. */
	double hclock; /**< Horiz freq, ==0 if not computable. */
	int scan; /**< Scan mode. 0=singlescan, 1=doublescan, -1=interlace. */

	unsigned char driver_mode[MODE_DRIVER_MODE_SIZE_MAX]; /**< Driver mode information. */
	struct adv_video_driver_struct* driver; /**< Video driver of the mode. */
} adv_mode;

/**
 * Get the bits_per_pixel of a index.
 * \return
 *   - ==0 on error
 *   - !=0 bits_per_pixel
 */
static inline unsigned index_bits_per_pixel(unsigned index)
{
	switch (index) {
	case MODE_FLAGS_INDEX_PALETTE8 : return 8;
	case MODE_FLAGS_INDEX_BGR8 : return 8;
	case MODE_FLAGS_INDEX_BGR15 : return 15;
	case MODE_FLAGS_INDEX_BGR16 : return 16;
	case MODE_FLAGS_INDEX_BGR24 : return 24;
	case MODE_FLAGS_INDEX_BGR32 : return 32;
	case MODE_FLAGS_INDEX_YUY2 : return 32;
	case MODE_FLAGS_INDEX_TEXT : return 16;
	default: return 0;
	}
}

/**
 * Get the bytes_per_pixel of a index.
 * \return
 *   - ==0 on error
 *   - !=0 bits_per_pixel
 */
static inline unsigned index_bytes_per_pixel(unsigned index)
{
	switch (index) {
	case MODE_FLAGS_INDEX_PALETTE8 : return 1;
	case MODE_FLAGS_INDEX_BGR8 : return 1;
	case MODE_FLAGS_INDEX_BGR15 : return 2;
	case MODE_FLAGS_INDEX_BGR16 : return 2;
	case MODE_FLAGS_INDEX_BGR24 : return 3;
	case MODE_FLAGS_INDEX_BGR32 : return 4;
	case MODE_FLAGS_INDEX_YUY2 : return 4;
	case MODE_FLAGS_INDEX_TEXT : return 2;
	default: return 0;
	}
}

/**
 * Get the index name.
 */
static inline const char* index_name(unsigned index)
{
	switch (index) {
	case MODE_FLAGS_INDEX_PALETTE8 : return "palette8";
	case MODE_FLAGS_INDEX_BGR8 : return "bgr8";
	case MODE_FLAGS_INDEX_BGR15 : return "bgr15";
	case MODE_FLAGS_INDEX_BGR16 : return "bgr16";
	case MODE_FLAGS_INDEX_BGR24 : return "bgr24";
	case MODE_FLAGS_INDEX_BGR32 : return "bgr32";
	case MODE_FLAGS_INDEX_YUY2 : return "yuy2";
	case MODE_FLAGS_INDEX_TEXT : return "text";
	default: return "unknown";
	}
}

/**
 * Check if a index is a RGB mode.
 * \return
 *   - ==0 on error
 *   - !=0 bits_per_pixel
 */
static inline adv_bool index_is_rgb(unsigned index)
{
	switch (index & MODE_FLAGS_INDEX_MASK) {
	case MODE_FLAGS_INDEX_BGR8 :
	case MODE_FLAGS_INDEX_BGR15 :
	case MODE_FLAGS_INDEX_BGR16 :
	case MODE_FLAGS_INDEX_BGR24 :
	case MODE_FLAGS_INDEX_BGR32 :
		return 1;
	default:
		return 0;
	}
}

/**
 * Get the name of a video mode.
 */
static inline const char* mode_name(const adv_mode* mode)
{
	return mode->name;
}

/**
 * Get the driver of a video mode.
 */
static inline const struct adv_video_driver_struct* mode_driver(const adv_mode* mode)
{
	return mode->driver;
}

/**
 * Get the vertical clock of a video mode.
 */
static inline double mode_vclock(const adv_mode* mode)
{
	return mode->vclock;
}

/**
 * Get the horizontal clock of a video mode.
 */
static inline double mode_hclock(const adv_mode* mode)
{
	return mode->hclock;
}

/**
 * Get the width of a video mode.
 */
static inline unsigned mode_size_x(const adv_mode* mode)
{
	return mode->size_x;
}

/**
 * Get the height of a video mode.
 */
static inline unsigned mode_size_y(const adv_mode* mode)
{
	return mode->size_y;
}

/**
 * Get the bits per pixel of a video mode.
 */
static inline unsigned mode_bits_per_pixel(const adv_mode* mode)
{
	return index_bits_per_pixel(mode->flags & MODE_FLAGS_INDEX_MASK);
}

/**
 * Get the bytes per pixel of a video mode.
 */
static inline unsigned mode_bytes_per_pixel(const adv_mode* mode)
{
	return index_bytes_per_pixel(mode->flags & MODE_FLAGS_INDEX_MASK);
}

/**
 * Get the flags of a video mode.
 */
static inline unsigned mode_flags(const adv_mode* mode)
{
	return mode->flags;
}

/**
 * Get the index mode of a video mode.
 * \return One of the MODE_FLAGS_INDEX_* flags.
 */
static inline unsigned mode_index(const adv_mode* mode)
{
	return mode_flags(mode) & MODE_FLAGS_INDEX_MASK;
}

/**
 * Check if a video mode is a text mode.
 */
static inline adv_bool mode_is_text(const adv_mode* mode)
{
	return mode_index(mode) == MODE_FLAGS_INDEX_TEXT;
}

/**
 * Check if a video mode is a graphics mode.
 */
static inline adv_bool mode_is_graphics(const adv_mode* mode)
{
	return !mode_is_text(mode);
}

/**
 * Get the line scan of a video mode.
 * \return
 *   - -1 interlace
 *   - 0 singlescan
 *   - 1 doublescan
 */
static inline int mode_scan(const adv_mode* mode)
{
	return mode->scan;
}

void mode_reset(adv_mode* mode);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

