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
 * Video driver "slang".
 */

#ifndef __DRVSLANG_H
#define __DRVSLANG_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct slang_video_mode_struct {
	unsigned font_size_x; /** Fake font size */
	unsigned font_size_y;
} slang_video_mode;

error slang_init(int device_id);
void slang_done(void);

boolean slang_is_active(void);
boolean slang_mode_is_active(void);

unsigned slang_flags(void);

error slang_mode_set(const slang_video_mode* mode);
void slang_mode_done(boolean restore);

unsigned slang_virtual_x(void);
unsigned slang_virtual_y(void);
unsigned slang_bytes_per_scanline(void);
unsigned slang_adjust_bytes_per_page(unsigned bytes_per_page);
video_rgb_def slang_rgb_def(void);

extern unsigned char* (*slang_write_line)(unsigned y);

void slang_wait_vsync(void);
error slang_scroll(unsigned offset, boolean waitvsync);
error slang_scanline_set(unsigned byte_length);
error slang_palette8_set(const video_color* palette, unsigned start, unsigned count, boolean waitvsync);

error slang_mode_import(video_mode* mode, const slang_video_mode* slang_mode);
error slang_mode_generate(slang_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags);
int slang_mode_compare(const slang_video_mode* a, const slang_video_mode* b);

void slang_default(void);
void slang_reg(struct conf_context* context);
error slang_load(struct conf_context* context);

/**
 * Video driver "slang".
 * \ingroup Video
 */
extern video_driver video_slang_driver;

#ifdef __cplusplus
}
#endif

#endif
