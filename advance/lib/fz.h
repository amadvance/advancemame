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

#ifndef __FZ_H
#define __FZ_H

#include <stdio.h>
#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum fz_type {
	fz_file, /* real file, eventually only a part */
	fz_memory, /* memory */
	fz_file_compressed /* compressed file, eventually only a part */
};

typedef struct fz {
	unsigned type; /* type of file, fz_* */
	unsigned virtual_pos; /* current position on the virtual file */
	unsigned virtual_size; /* size */

	unsigned real_offset; /* starting position on the real file */
	unsigned real_size; /* real size of the intersting file */

	/* memory */
	const unsigned char* data;

	/* file */
	FILE* f;

	/* compression */
	z_stream z;
	unsigned char* zbuffer;
	unsigned remaining;
} FZ;

FZ* fzopen(const char* file, const char* mode);
FZ* fzopenzipuncompressed(const char* file, unsigned offset, unsigned size);
FZ* fzopenzipcompressed(const char* file, unsigned offset, unsigned size_compressed, unsigned size_uncompressed);
FZ* fzopenmemory(const unsigned char* data, unsigned size);

unsigned fzread(void *buffer, unsigned size, unsigned number, FZ* f);
int fzclose(FZ* f);
long fztell(FZ* f);
long fzsize(FZ* f);
int fzseek(FZ* f, long offset, int mode);

/* only for plain file */
int fzgetc(FZ* f);
int fzungetc(int c, FZ* f);
char* fzgets(char *s, int n, FZ* f);
int fzeof(FZ* f);
unsigned fzwrite(const void *buffer, unsigned size, unsigned number, FZ* f);

#ifdef __cplusplus
}
#endif

#endif
