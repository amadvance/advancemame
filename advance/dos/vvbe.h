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
 * Video driver "vbe".
 */

#ifndef __VBE_H
#define __VBE_H

#include "videodrv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vbe_video_mode_struct {
	unsigned mode; /**< VBE mode number (with | vbeLinearBuffer if used) */
} vbe_video_mode;

unsigned vbe_flags(void);

adv_error vbe_mode_generate(vbe_video_mode* mode, const adv_crtc* crtc, unsigned flags);
adv_error vbe_mode_import(adv_mode* mode, const vbe_video_mode* vbe_mode);
adv_error vbe_mode_grab(vbe_video_mode* mode);
adv_error vbe_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);
int vbe_mode_compare(const vbe_video_mode* a, const vbe_video_mode* b);

/**
 * Video driver "vbe".
 * \ingroup Video
 */
extern adv_video_driver video_vbe_driver;

#ifdef __cplusplus
}
#endif

#endif
