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

#ifndef __CRTCBAG_H
#define __CRTCBAG_H

#include "crtc.h"
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Video crtc container */

typedef struct video_crtc_container_struct {
	video_crtc* base;
} video_crtc_container;

int video_crtc_compare(const video_crtc* a,const video_crtc* b);

void video_crtc_container_init(video_crtc_container* cc);
void video_crtc_container_done(video_crtc_container* cc);

adv_error video_crtc_container_load(struct conf_context* context, video_crtc_container* cc);
void video_crtc_container_save(struct conf_context* context, video_crtc_container* cc);
void video_crtc_container_clear(struct conf_context* context);
void video_crtc_container_register(struct conf_context* context);

const video_crtc* video_crtc_container_has(video_crtc_container* cc, const video_crtc* vm, int (*compare)(const video_crtc* a,const video_crtc* b));
void video_crtc_container_remove(video_crtc_container* cc, int (*modeselect)(const video_crtc*, void*), void*);
const video_crtc* video_crtc_container_insert(video_crtc_container* cc, const video_crtc* vm);
const video_crtc* video_crtc_container_insert_sort(video_crtc_container* cc, const video_crtc* vm, int (*compare)(const video_crtc* a,const video_crtc* b));
adv_bool video_crtc_container_is_empty(const video_crtc_container* cc);

typedef struct video_crtc_container_iterator_struct {
	video_crtc* base;
} video_crtc_container_iterator;

void video_crtc_container_iterator_begin(video_crtc_container_iterator* cci, video_crtc_container* cc);
void video_crtc_container_iterator_next(video_crtc_container_iterator* cci);
adv_bool video_crtc_container_iterator_is_end(video_crtc_container_iterator* cci);
video_crtc* video_crtc_container_iterator_get(video_crtc_container_iterator* cci);

adv_error video_crtc_container_insert_default_modeline_vga(video_crtc_container* cc);
adv_error video_crtc_container_insert_default_modeline_svga(video_crtc_container* cc);
adv_error video_crtc_container_insert_default_bios_vga(video_crtc_container* cc);
adv_error video_crtc_container_insert_default_bios_vbe(video_crtc_container* cc);
void video_crtc_container_insert_default_system(video_crtc_container* cc);

#ifdef __cplusplus
}
#endif

#endif
