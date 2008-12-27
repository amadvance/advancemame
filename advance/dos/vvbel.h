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
 * Video driver "vbeline".
 */

#ifndef __VBELINE_H
#define __VBELINE_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vbeline_video_mode_struct {
	unsigned mode; /**< VBE mode number (with | vbeLinearBuffer if used) */
	adv_crtc crtc; /**< CRTC values */
} vbeline_video_mode;

adv_error vbeline_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor);
void vbeline_done(void);

adv_bool vbeline_is_active(void);
adv_bool vbeline_mode_is_active(void);

unsigned vbeline_flags(void);

adv_error vbeline_mode_set(const vbeline_video_mode* mode);
void vbeline_mode_done(adv_bool restore);

unsigned vbeline_virtual_x(void);
unsigned vbeline_virtual_y(void);

extern unsigned char* (*vbeline_write_line)(unsigned y);

void vbeline_wait_vsync(void);
adv_error vbeline_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);
adv_error vbeline_pixelclock_getnext(unsigned* pixelclock, unsigned mode);
adv_error vbeline_pixelclock_getpred(unsigned* pixelclock, unsigned mode);

unsigned vbeline_mode_size(void);
adv_error vbeline_mode_import(adv_mode* mode, const vbeline_video_mode* vbeline_mode);
adv_error vbeline_mode_generate(vbeline_video_mode* mode, const adv_crtc* crtc, unsigned flags);
int vbeline_mode_compare(const vbeline_video_mode* a, const vbeline_video_mode* b);

void vbeline_default(void);
adv_error vbeline_load(adv_conf* context);
void vbeline_reg(adv_conf* context);

/**
 * Video driver "vbeline".
 * \ingroup Video
 */
extern adv_video_driver video_vbeline_driver;

#ifdef __cplusplus
}
#endif

#endif
