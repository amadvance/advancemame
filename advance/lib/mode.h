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

/** \name Flags memory
 * Flags describing the memory of a video mode.
 */
/*@{*/
#define MODE_FLAGS_MEMORY_LINEAR 0x0001 /**< The memory is linear. */
#define MODE_FLAGS_MEMORY_UNCHAINED 0x0002 /**< The memory is unchained (Xmode). */
#define MODE_FLAGS_MEMORY_BANKED 0x0004 /**< The memory is banked. */
#define MODE_FLAGS_MEMORY_MASK 0x000F /**< Mask. */
/*@}*/

/** \name Flags sync
 * Flags describing the sync support of a video mode.
 */
/*@{*/
#define MODE_FLAGS_SCROLL_SYNC  0x0020 /**< The syncronous (triple buffer) setpage command is supported. */
#define MODE_FLAGS_SCROLL_ASYNC 0x0040 /**< The asyncronous (double buffer) setpage command is supported. */
#define MODE_FLAGS_SCROLL_MASK  0x00F0 /**< Mask. */
/*@}*/

/** \name Flags index
 * Flags describing the color index of a video mode.
 */
/*@{*/
#define MODE_FLAGS_INDEX_RGB 0x0100 /**< RGB mode. */
#define MODE_FLAGS_INDEX_PACKED 0x0200 /**< Palette mode. */
#define MODE_FLAGS_INDEX_TEXT 0x0300 /**< Text mode. */
#define MODE_FLAGS_INDEX_MASK 0x0F00 /**< Mask. */
/*@}*/

/** \name Flags type
 * Flags describing the type of a video mode.
 */
/*@{*/
#define MODE_FLAGS_TYPE_TEXT 0x1000 /**< Text mode. */
#define MODE_FLAGS_TYPE_GRAPHICS 0x2000 /**< Graphics mode. */
#define MODE_FLAGS_TYPE_MASK 0xF000 /**< Mask. */
/*@}*/

/** \name Flags user
 * Flags for the user.
 */
/*@{*/
#define MODE_FLAGS_USER_BIT0 0x10000000 /**< First user flag. */
#define MODE_FLAGS_USER_MASK 0xF0000000 /**< Mask. */
/*@}*/

/**
 * Max size of a driver video mode
 */
#define MODE_DRIVER_MODE_SIZE_MAX (sizeof(adv_crtc)+16)

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
	unsigned bits_per_pixel; /**< graph=8,15,16,24,32, text=0 */

	unsigned char driver_mode[MODE_DRIVER_MODE_SIZE_MAX]; /**< Driver mode information. */
	struct adv_video_driver_struct* driver; /**< Video driver of the mode. */
} adv_mode;

/**
 * Get the name of a video mode.
 */
static inline const char* mode_name(const adv_mode* mode) {
	return mode->name;
}

/**
 * Get the driver of a video mode.
 */
static inline const struct adv_video_driver_struct* mode_driver(const adv_mode* mode) {
	return mode->driver;
}

/**
 * Get the vertical clock of a video mode.
 */
static inline double mode_vclock(const adv_mode* mode) {
	return mode->vclock;
}

/**
 * Get the horizontal clock of a video mode.
 */
static inline double mode_hclock(const adv_mode* mode) {
	return mode->hclock;
}

/**
 * Get the width of a video mode.
 */
static inline unsigned mode_size_x(const adv_mode* mode) {
	return mode->size_x;
}

/**
 * Get the height of a video mode.
 */
static inline unsigned mode_size_y(const adv_mode* mode) {
	return mode->size_y;
}

/**
 * Get the bits per pixel of a video mode.
 */
static inline unsigned mode_bits_per_pixel(const adv_mode* mode) {
	return mode->bits_per_pixel;
}

/**
 * Get the bytes per pixel of a video mode.
 */
static inline unsigned mode_bytes_per_pixel(const adv_mode* mode) {
	return (mode_bits_per_pixel(mode) + 7) / 8;
}

/**
 * Get the flags of a video mode.
 */
static inline unsigned mode_flags(const adv_mode* mode) {
	return mode->flags;
}

/**
 * Get the index mode of a video mode.
 * \return One of the MODE_FLAGS_INDEX_* flags.
 */
static inline unsigned mode_index(const adv_mode* mode) {
	return mode_flags(mode) & MODE_FLAGS_INDEX_MASK;
}

/**
 * Get the type of a video mode.
 * \return One of the MODE_FLAGS_TYPE_* flags.
 */
static inline unsigned mode_type(const adv_mode* mode) {
	return mode_flags(mode) & MODE_FLAGS_TYPE_MASK;
}

/**
 * Get the memory mode of a video mode.
 * \return One of the MODE_FLAGS_MEMORY_* flags.
 */
static inline unsigned mode_memory(const adv_mode* mode) {
	return mode_flags(mode) & MODE_FLAGS_MEMORY_MASK;
}

/**
 * Check if a video mode is a text mode.
 */
static inline adv_bool mode_is_text(const adv_mode* mode) {
	return mode_type(mode) == MODE_FLAGS_TYPE_TEXT;
}

/**
 * Check if a video mode is a graphics mode.
 */
static inline adv_bool mode_is_graphics(const adv_mode* mode) {
	return mode_type(mode) == MODE_FLAGS_TYPE_GRAPHICS;
}

/**
 * Check if a video mode is a linear mode.
 */
static inline adv_bool mode_is_linear(const adv_mode* mode) {
	return mode_memory(mode) == MODE_FLAGS_MEMORY_LINEAR;
}

/**
 * Check if a video mode is a unchained mode.
 */
static inline adv_bool mode_is_unchained(const adv_mode* mode) {
	return mode_memory(mode) == MODE_FLAGS_MEMORY_UNCHAINED;
}

/**
 * Check if a video mode is a banked mode.
 */
static inline adv_bool mode_is_banked(const adv_mode* mode) {
	return mode_memory(mode) == MODE_FLAGS_MEMORY_BANKED;
}

/**
 * Get the line scan of a video mode.
 * \return
 *   - -1 interlace
 *   - 0 singlescan
 *   - 1 doublescan
 */
static inline int mode_scan(const adv_mode* mode) {
	return mode->scan;
}

void mode_reset(adv_mode* mode);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/

