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

#ifndef __MODE_H
#define __MODE_H

#include "videostd.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Video mode */

/* Flags for memory */
#define VIDEO_FLAGS_MEMORY_LINEAR 0x0001
#define VIDEO_FLAGS_MEMORY_UNCHAINED 0x0002
#define VIDEO_FLAGS_MEMORY_BANKED 0x0004
#define VIDEO_FLAGS_MEMORY_MASK 0x000F

/* Flags for triple buffer support */
#define VIDEO_FLAGS_SYNC_SETPAGE 0x0020

/* Flags for double buffer support */
#define VIDEO_FLAGS_ASYNC_SETPAGE 0x0040

/* Flags for color mode */
#define VIDEO_FLAGS_INDEX_RGB 0x0100 /* RGB mode */
#define VIDEO_FLAGS_INDEX_PACKED 0x0200 /* Palette mode */
#define VIDEO_FLAGS_INDEX_TEXT 0x0300 /* Text mode */
#define VIDEO_FLAGS_INDEX_MASK 0x0F00

/* Flags for text or graphics mode */
#define VIDEO_FLAGS_TYPE_TEXT 0x1000 /* Text mode */
#define VIDEO_FLAGS_TYPE_GRAPHICS 0x2000 /* Graphics mode */
#define VIDEO_FLAGS_TYPE_MASK 0xF000

/* Users flags */
#define VIDEO_FLAGS_USER_BIT0 0x10000000
#define VIDEO_FLAGS_USER_BIT1 0x20000000
#define VIDEO_FLAGS_USER_BIT2 0x40000000
#define VIDEO_FLAGS_USER_BIT3 0x80000000
#define VIDEO_FLAGS_USER_MASK 0xF0000000

/** Max size of a driver video mode */
#define VIDEO_DRIVER_MODE_SIZE_MAX (sizeof(video_crtc)+16)

struct video_driver_struct; /**< Forward declaration */

typedef struct video_mode_struct {
	unsigned flags;
	char name[VIDEO_NAME_MAX];
	unsigned size_x;
	unsigned size_y;
	double vclock; /**< Vertical freq, ==0 if not computable */
	double hclock; /**< Horiz freq, ==0 if not computable */
	int scan; /**< Scan mode. 0=singlescan, 1=doublescan, -1=interlace. */
	unsigned bits_per_pixel; /**< graph=8,15,16,24,32, text=0 */

	unsigned char driver_mode[VIDEO_DRIVER_MODE_SIZE_MAX]; /**< Driver mode information */
	struct video_driver_struct* driver; /**< Video driver of the mode */
} video_mode;

/** Name of the video mode. */
static __inline__  const char* video_mode_name(const video_mode* mode) {
	return mode->name;
}

/** Driver of the video mode. */
static __inline__  const struct video_driver_struct* video_mode_driver(const video_mode* mode) {
	return mode->driver;
}

/** Vertical clock of the video mode. */
static __inline__  double video_mode_vclock(const video_mode* mode) {
	return mode->vclock;
}

/** Horizontal clock of the video mode. */
static __inline__  double video_mode_hclock(const video_mode* mode) {
	return mode->hclock;
}

/** Horizontal size of the video mode. */
static __inline__  unsigned video_mode_size_x(const video_mode* mode) {
	return mode->size_x;
}

/** Vertical size of the video mode. */
static __inline__  unsigned video_mode_size_y(const video_mode* mode) {
	return mode->size_y;
}

static __inline__  unsigned video_mode_bits_per_pixel(const video_mode* mode) {
	return mode->bits_per_pixel;
}

static __inline__  unsigned video_mode_bytes_per_pixel(const video_mode* mode) {
	return (video_mode_bits_per_pixel(mode) + 7) / 8;
}

static __inline__  unsigned video_mode_flags(const video_mode* mode) {
	return mode->flags;
}

static __inline__  unsigned video_mode_index(const video_mode* mode) {
	return video_mode_flags(mode) & VIDEO_FLAGS_INDEX_MASK;
}

static __inline__  unsigned video_mode_type(const video_mode* mode) {
	return video_mode_flags(mode) & VIDEO_FLAGS_TYPE_MASK;
}

static __inline__  unsigned video_mode_memory(const video_mode* mode) {
	return video_mode_flags(mode) & VIDEO_FLAGS_MEMORY_MASK;
}

static __inline__  video_bool video_mode_is_text(const video_mode* mode) {
	return video_mode_type(mode) == VIDEO_FLAGS_TYPE_TEXT;
}

static __inline__  video_bool video_mode_is_graphics(const video_mode* mode) {
	return video_mode_type(mode) == VIDEO_FLAGS_TYPE_GRAPHICS;
}

static __inline__  video_bool video_mode_is_linear(const video_mode* mode) {
	return video_mode_memory(mode) == VIDEO_FLAGS_MEMORY_LINEAR;
}

static __inline__  video_bool video_mode_is_unchained(const video_mode* mode) {
	return video_mode_memory(mode) == VIDEO_FLAGS_MEMORY_UNCHAINED;
}

static __inline__  video_bool video_mode_is_banked(const video_mode* mode) {
	return video_mode_memory(mode) == VIDEO_FLAGS_MEMORY_BANKED;
}

/**
 * Return the line scan.
 * \return
 *   - -1 interlace
 *   - 0 singlescan
 *   - 1 doublescan
 */
static __inline__  int video_mode_scan(const video_mode* mode) {
	return mode->scan;
}

#ifdef __cplusplus
}
#endif

#endif
