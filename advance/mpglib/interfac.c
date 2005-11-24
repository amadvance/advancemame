/*
 * This file is part of MPGLIB.
 *
 * Copyright (C) 1995, 1996, 1997 Michael Hipp
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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "mpg123.h"
#include "mpglib.h"

static int mp3_lib_initialized = 0;

void mp3_lib_init(void) 
{
	assert(mp3_lib_initialized == 0);
	mp3_lib_initialized = 1;
	mp3internal_make_decode_tables(32767);
	mp3internal_init_layer3(SBLIMIT);
}

void mp3_lib_done(void) 
{
	assert(mp3_lib_initialized != 0);
	mp3_lib_initialized = 0;
}

/* Initialize the decoding stream */
void mp3_init(struct mp3_mpstr *mp) 
{
	assert(mp3_lib_initialized != 0);

	memset(mp,0,sizeof(struct mp3_mpstr));
	mp->framesize = 0;
	mp->fsizeold = -1;
	mp->bsize = 0;
	mp->fr.single = -1;
	mp->bsnum = 0;
	mp->state.synth_bo = 1;
	mp->dirty = 0;
}

/* Deinitialize the decoding stream */
void mp3_done(struct mp3_mpstr *mp)
{
	struct mp3_buf *b,*bn;

	assert(mp3_lib_initialized != 0);

	b = mp->tail;
	while(b) {
		free(b->pnt);
		bn = b->next;
		free(b);
		b = bn;
	}
}

#define reset(a) memset(&a,0,sizeof(a))

/* Clear the internal state of the decoding stream */
static void mp3_reset(struct mp3_mpstr *mp) 
{
	/* head */
	/* tail */
	/* bsize */
	/* framesize */
	/* fsizeold */
	reset(mp->fr);
	reset(mp->bsspace);
	reset(mp->state.hybrid_block);
	reset(mp->state.hybrid_blc);
	reset(mp->header);
	/* bsnum */
	reset(mp->state.synth_buffs);
	mp->state.synth_bo = 1;
	mp->fr.single = -1;
	mp->dirty = 0;
}

static struct mp3_buf *addbuf(struct mp3_mpstr *mp,char *buf,int size)
{
	struct mp3_buf *nbuf;

	nbuf = (struct mp3_buf*) malloc( sizeof(struct mp3_buf) );
	if (!nbuf) {
		return NULL;
	}
	nbuf->pnt = (unsigned char*) malloc(size);
	if (!nbuf->pnt) {
		free(nbuf);
		return NULL;
	}
	nbuf->size = size;
	memcpy(nbuf->pnt,buf,size);
	nbuf->next = NULL;
	nbuf->prev = mp->head;
	nbuf->pos = 0;

	if (!mp->tail) {
		mp->tail = nbuf;
	} else {
	  mp->head->next = nbuf;
	}

	mp->head = nbuf;
	mp->bsize += size;

	return nbuf;
}

static void remove_buf(struct mp3_mpstr *mp)
{
	struct mp3_buf *buf = mp->tail;
  
	mp->tail = buf->next;
	if (mp->tail) {
		mp->tail->prev = NULL;
	} else {
		mp->tail = mp->head = NULL;
	}
  
	free(buf->pnt);
	free(buf);
}

static int read_buf_byte(struct mp3_mpstr *mp)
{
	unsigned b;
	int pos;

	pos = mp->tail->pos;
	while (pos >= mp->tail->size) {
		remove_buf(mp);
		pos = mp->tail->pos;
	}

	b = mp->tail->pnt[pos];
	mp->bsize--;
	mp->tail->pos++;

	return b;
}

static void read_head(struct mp3_mpstr *mp)
{
	unsigned long head;

	head = read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);

	mp->header = head;
}

int mp3_decode(struct mp3_mpstr* mp, unsigned char* uin, int isize, unsigned char* uout, int osize, int* done)
{
	int len;
	int ret;
	unsigned estimated_done;
	char* in = (char*)uin;
	char* out = (char*)uout;

	if (in) {
		if (addbuf(mp,in,isize) == NULL) {
			return MP3_ERR;
		}
	}

	/* First decode header */
	if (mp->framesize == 0) {
		if (mp->bsize < 4)
			return MP3_NEED_MORE;
		read_head(mp);
		if (!mp3internal_decode_header(&mp->fr,mp->header))
			return MP3_ERR;
		mp->framesize = mp->fr.framesize;
	}

	if (mp->fr.framesize > mp->bsize)
		return MP3_NEED_MORE;

	if (osize < 4608)
		return MP3_NEED_SPACE;

	mp->state.wordpointer = mp->bsspace[mp->bsnum] + 512;
	mp->bsnum = (mp->bsnum + 1) & 0x1;
	mp->state.bitindex = 0;

	len = 0;
	while (len < mp->framesize) {
		int nlen;
		int blen = mp->tail->size - mp->tail->pos;
		if ((mp->framesize - len) <= blen) {
			nlen = mp->framesize-len;
		} else {
			nlen = blen;
		}
		memcpy(mp->state.wordpointer+len,mp->tail->pnt+mp->tail->pos,nlen);
		len += nlen;
		mp->tail->pos += nlen;
		mp->bsize -= nlen;
		if (mp->tail->pos == mp->tail->size) {
			remove_buf(mp);
		}
	}

	*done = 0;
	if (mp->fr.error_protection)
		mp3internal_skipbits(&mp->state,16);

	/* compute the output number of bytes, also if the decoding process fail */
	estimated_done = SSLIMIT * 64;
	if (!mp->fr.lsf)
		estimated_done *= 2;
	if (mp->fr.single < 0)
		estimated_done *= 2;
	
	if (out) {
		if (mp->dirty) 
			mp3_reset(mp);
		ret = mp3internal_do_layer3(mp,&mp->state,&mp->fr,(unsigned char *)out,done);
		if (ret > 0)
			ret = 0;
		if (ret != MP3_OK)
			*done = estimated_done;
	} else {
		ret = MP3_OK;
		*done = estimated_done;
		mp3internal_skipbits(&mp->state,8*mp->framesize);
		mp->dirty = 1; /* invalid state */
	}

	mp->fsizeold = mp->framesize;
	mp->framesize = 0;

	return ret;
}

int mp3_set_pointer(void* Amp, long backstep)
{
	unsigned char *bsbufold;
	struct mp3_mpstr *mp = Amp;
	if (mp->fsizeold < 0 && backstep > 0) {
		 /* fprintf(stderr,"mpglib: Can't step back %ld!\n",backstep); */
		return MP3_ERR;
	}
	bsbufold = mp->bsspace[mp->bsnum] + 512;
	mp->state.wordpointer -= backstep;
	if (backstep)
		memcpy(mp->state.wordpointer,bsbufold+mp->fsizeold-backstep,backstep);
	mp->state.bitindex = 0;
	return MP3_OK;
}

int mp3_is_valid(unsigned char* head)
{
	unsigned newhead;
	struct mp3_frame fr;

	newhead = head[0];
	newhead <<= 8;
	newhead |= head[1];
	newhead <<= 8;
	newhead |= head[2];
	newhead <<= 8;
	newhead |= head[3];

	return mp3internal_decode_header(&fr,newhead);
}

