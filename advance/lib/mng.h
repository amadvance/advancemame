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
 */

#ifndef __MNG_H
#define __MNG_H

#include "fz.h"
#include "bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

void* mng_init(FZ* f);
void mng_done(void* context);
struct bitmap* mng_load(void* context, FZ* f, video_color* rgb, unsigned* rgb_max, double* delay);

#ifdef __cplusplus
}
#endif

#endif
