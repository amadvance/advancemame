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
 * CRTC containers.
 */

/** \addtogroup Crtc */
/*@{*/

#ifndef __CRTCBAG_H
#define __CRTCBAG_H

#include "crtc.h"
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Video crtc container */

/**
 * Container of adv_crtc objets.
 */
typedef struct adv_crtc_container_struct {
	adv_crtc* base;
} adv_crtc_container;

int crtc_compare(const adv_crtc* a, const adv_crtc* b);

void crtc_container_init(adv_crtc_container* cc);
void crtc_container_done(adv_crtc_container* cc);

adv_error crtc_container_load(adv_conf* context, adv_crtc_container* cc);
void crtc_container_save(adv_conf* context, adv_crtc_container* cc);
void crtc_container_clear(adv_conf* context);
void crtc_container_register(adv_conf* context);

const adv_crtc* crtc_container_has(adv_crtc_container* cc, const adv_crtc* vm, int (*compare)(const adv_crtc* a, const adv_crtc* b));
void crtc_container_remove(adv_crtc_container* cc, adv_bool (*modeselect)(const adv_crtc*, void*), void*);
const adv_crtc* crtc_container_insert(adv_crtc_container* cc, const adv_crtc* vm);
const adv_crtc* crtc_container_insert_sort(adv_crtc_container* cc, const adv_crtc* vm, int (*compare)(const adv_crtc* a, const adv_crtc* b));
adv_bool crtc_container_is_empty(const adv_crtc_container* cc);

typedef struct adv_crtc_container_iterator_struct {
	adv_crtc* base;
} adv_crtc_container_iterator;

void crtc_container_iterator_begin(adv_crtc_container_iterator* cci, adv_crtc_container* cc);
void crtc_container_iterator_next(adv_crtc_container_iterator* cci);
adv_bool crtc_container_iterator_is_end(adv_crtc_container_iterator* cci);
adv_crtc* crtc_container_iterator_get(adv_crtc_container_iterator* cci);

adv_error crtc_container_insert_default_modeline_vga(adv_crtc_container* cc);
adv_error crtc_container_insert_default_modeline_svga(adv_crtc_container* cc);
adv_error crtc_container_insert_default_bios_vga(adv_crtc_container* cc);
adv_error crtc_container_insert_default_bios_vbe(adv_crtc_container* cc);
void crtc_container_insert_default_active(adv_crtc_container* cc);
void crtc_container_insert_default_all(adv_crtc_container* cc);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/
