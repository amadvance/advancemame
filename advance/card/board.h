/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Andrea Mazzoleni
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

#ifndef __BOARD_H
#define __BOARD_H

#include "card.h"

int ati_detect(void);
int ati_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);
void ati_reset(void);

const char* cirrus_driver(void);
int cirrus_detect(void);
void cirrus_reset(void);
int cirrus_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

const char* laguna_driver(void);
int laguna_detect(void);
void laguna_reset(void);
int laguna_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co);

const char* matrox_driver(void);
int matrox_detect(void);
void matrox_reset(void);
int matrox_set(const card_crtc*, const card_mode*, const card_mode*);

const char* neomagic_driver(void);
int neomagic_detect(void);
void neomagic_reset(void);
int neomagic_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

int r128_detect(void);
void r128_reset(void);
int r128_set(const card_crtc*, const card_mode*, const card_mode*);

const char* s3_driver(void);
int s3_detect(void);
void s3_reset(void);
int s3_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

const char* savage_driver(void);
int savage_detect(void);
void savage_reset(void);
int savage_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

const char* sis_driver(void);
int sis_detect(void);
void sis_reset(void);
int sis_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

const char* tdfx_driver(void);
int tdfx_detect(void);
void tdfx_reset(void);
int tdfx_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

const char* trident_driver(void);
int trident_detect(void);
void trident_reset(void);
int trident_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);

const char* vbe3_driver(void);
int vbe3_detect(void);
int vbe3_set(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);
void vbe3_reset(void);

#endif
