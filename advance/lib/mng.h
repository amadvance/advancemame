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
 */

#ifndef __MNG_H
#define __MNG_H

#include "png.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MNG_CN_DHDR 0x44484452
#define MNG_CN_MHDR 0x4D484452
#define MNG_CN_MEND 0x4D454E44
#define MNG_CN_DEFI 0x44454649
#define MNG_CN_PPLT 0x50504c54
#define MNG_CN_MOVE 0x4d4f5645
#define MNG_CN_TERM 0x5445524d
#define MNG_CN_SAVE 0x53415645
#define MNG_CN_SEEK 0x5345454b
#define MNG_CN_LOOP 0x4c4f4f50
#define MNG_CN_ENDL 0x454e444c
#define MNG_CN_BACK 0x4241434b
#define MNG_CN_FRAM 0x4652414d

int mng_read_signature(FZ* f);
int mng_write_signature(FZ* f, unsigned* count);

void* mng_init(FZ* f);
void mng_done(void* void_mng);
int mng_read(
	void* void_mng,
	unsigned* pix_width, unsigned* pix_height, unsigned* pix_pixel,
	unsigned char** dat_ptr, unsigned* dat_size,
	unsigned char** pix_ptr, unsigned* pix_scanline,
	unsigned char** pal_ptr, unsigned* pal_size,
	unsigned* tick,
	FZ* f
);
unsigned mng_frequency_get(void* void_mng);
unsigned mng_width_get(void* void_mng);
unsigned mng_height_get(void* void_mng);

#ifdef __cplusplus
}
#endif

#endif
