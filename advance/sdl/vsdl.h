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

#ifndef __VSDL_H
#define __VSDL_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sdl_video_mode_struct {
	unsigned size_x; /**< SDL mode size x */
	unsigned size_y; /**< SDL mode size y */
	unsigned bits_per_pixel; /**< SDL mode bits per pixel */
} sdl_video_mode;

video_error sdl_init(int device_id);
void sdl_done(void);
video_bool sdl_is_active(void);
video_bool sdl_mode_is_active(void);
unsigned sdl_flags(void);
void sdl_write_lock(void);
void sdl_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y);
video_error sdl_mode_set(const sdl_video_mode* mode);
void sdl_mode_done(video_bool restore);
unsigned sdl_virtual_x(void);
unsigned sdl_virtual_y(void);
unsigned sdl_adjust_bytes_per_page(unsigned bytes_per_page);
unsigned sdl_bytes_per_scanline(void);
video_rgb_def sdl_rgb_def(void);
void sdl_wait_vsync(void);
video_error sdl_scroll(unsigned offset, video_bool waitvsync);
video_error sdl_scanline_set(unsigned byte_length);
video_error sdl_palette8_set(const video_color* palette, unsigned start, unsigned count, video_bool waitvsync);
video_error sdl_mode_import(video_mode* mode, const sdl_video_mode* sdl_mode);
video_error sdl_mode_generate(sdl_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags);
int sdl_mode_compare(const sdl_video_mode* a, const sdl_video_mode* b);
void sdl_default(void);
void sdl_reg(struct conf_context* context);
video_error sdl_load(struct conf_context* context);

extern video_driver video_sdl_driver;

#ifdef __cplusplus
}
#endif

#endif
