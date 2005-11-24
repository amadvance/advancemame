/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001 Andrea Mazzoleni
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

#ifndef __MPGLIB_H
#define __MPGLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define mp3internal_real double

#define SBLIMIT 32
#define SSLIMIT 18

#define MAXFRAMESIZE 1792

struct mp3_frame {
	int stereo;
	int jsbound;
	int single;
	int lsf;
	int mpeg25;
	int header_change;
	int lay;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int framesize; /* computed framesize */
};

struct mp3_decoder_state {
	unsigned char *wordpointer;
	int bitindex;
	mp3internal_real hybrid_block[2][2][SBLIMIT*SSLIMIT];
	int hybrid_blc[2];
	mp3internal_real synth_buffs[2][2][0x110];
	int  synth_bo;
};

struct mp3_buf {
	unsigned char *pnt;
	long size;
	long pos;
	struct mp3_buf *next;
	struct mp3_buf *prev;
};

struct mp3_framebuf {
	struct mp3_buf *buf;
	long pos;
	struct mp3_frame *next;
	struct mp3_frame *prev;
};

#define MAXFRAMESIZE 1792

extern const long mp3_freqs[9];

struct mp3_mpstr {
	struct mp3_buf *head, *tail;
	int bsize;
	int framesize;
	int fsizeold;
	struct mp3_frame fr;
	unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE+512 */
	unsigned long header;
	int bsnum;
	struct mp3_decoder_state state;
	int dirty; /* != 0 if the decoder state is invalid and need a reset */
};

#define MP3_ERR -1
#define MP3_OK 0
#define MP3_NEED_MORE 1
#define MP3_NEED_SPACE 2

void mp3_lib_init(void);
void mp3_lib_done(void);
void mp3_init(struct mp3_mpstr *mp);
int mp3_decode(struct mp3_mpstr *mp, unsigned char *inmemory, int inmemsize, unsigned char *outmemory, int outmemsize, int *done);
void mp3_done(struct mp3_mpstr *mp);
int mp3_is_valid(unsigned char* newhead);

#ifdef __cplusplus
}
#endif

#endif
