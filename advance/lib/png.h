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

#ifndef __PNG_H
#define __PNG_H

#include "bitmap.h"
#include "video.h"
#include "fz.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bitmap* png_load(FZ* f, video_color* rgb, unsigned* rgb_max);

/* exported for the MNG use */
int png_read_chunk(FZ* f, unsigned char** data, unsigned* size, unsigned* type);
void png_expand_4(unsigned width, unsigned height, unsigned char* ptr);
void png_expand_2(unsigned width, unsigned height, unsigned char* ptr);
void png_expand_1(unsigned width, unsigned height, unsigned char* ptr);
void png_unfilter_8(unsigned width, unsigned height, unsigned char* ptr, unsigned line);
void png_unfilter_24(unsigned width, unsigned height, unsigned char* ptr, unsigned line);
void png_unfilter_32(unsigned width, unsigned height, unsigned char* ptr, unsigned line);

#ifdef __cplusplus
};
#endif

#endif

