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

#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpg123.h"

/* struct parameter param = { 1 , 1 , 0 , 0 }; */

static const int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

const long mp3_freqs[9] = { 44100, 48000, 32000,
                  22050, 24000, 16000 ,
                  11025 , 12000 , 8000 };


#define HDRCMPMASK 0xfffffd00

/*
 * the code a header and write the information
 * into the frame structure
 */
int mp3internal_decode_header(struct mp3_frame *fr,unsigned long newhead)
{

	if( newhead & (1<<20) ) {
		fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
		fr->mpeg25 = 0;
	} else {
		fr->lsf = 1;
		fr->mpeg25 = 1;
	}

	fr->lay = 4-((newhead>>17)&3);

	if (((newhead>>10)&0x3) == 0x3) {
		/* fprintf(stderr,"mpglib: Stream error\n"); */
		return 0;
	}

	if (fr->mpeg25) {
		fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
	} else
		 fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);

	if (fr->sampling_frequency >= 9) {
		/* fprintf(stderr,"mpglib: Error\n"); */
		return 0;
	}

	fr->error_protection = ((newhead>>16)&0x1)^0x1;
	fr->bitrate_index = ((newhead>>12)&0xf);
	fr->padding = ((newhead>>9)&0x1);
	fr->extension = ((newhead>>8)&0x1);
	fr->mode = ((newhead>>6)&0x3);
	fr->mode_ext = ((newhead>>4)&0x3);
	fr->copyright = ((newhead>>3)&0x1);
	fr->original = ((newhead>>2)&0x1);
	fr->emphasis = newhead & 0x3;

	fr->stereo = (fr->mode == MPG_MD_MONO) ? 1 : 2;

	if (!fr->bitrate_index){
		/* fprintf(stderr,"mpglib: Free format not supported.\n"); */
		return 0;
	}

	switch (fr->lay) {
		case 1:
			/* fprintf(stderr,"mpglib: layer=1 Not supported!\n"); */
			return 0;
		break;
		case 2:
			/* fprintf(stderr,"mpglib: layer=2 Not supported!\n"); */
			return 0;
        	break;
		case 3:
			fr->framesize  = (long)tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
			fr->framesize /= mp3_freqs[fr->sampling_frequency]<<(fr->lsf);
			fr->framesize = fr->framesize + fr->padding - 4;
		break;
		default:
			/* fprintf(stderr,"mpglib: Sorry, unknown layer type.\n"); */
			return 0;
	}

	return 1;
}

unsigned int mp3internal_get1bit(struct mp3_decoder_state* state)
{
  unsigned char rval;
  rval = *state->wordpointer << state->bitindex;

  state->bitindex++;
  state->wordpointer += (state->bitindex>>3);
  state->bitindex &= 7;

  return rval>>7;
}


unsigned int mp3internal_getbits(struct mp3_decoder_state* state, int number_of_bits)
{
  unsigned long rval;

  if(!number_of_bits)
    return 0;

  {
    rval = state->wordpointer[0];
    rval <<= 8;
    rval |= state->wordpointer[1];
    rval <<= 8;
    rval |= state->wordpointer[2];
    rval <<= state->bitindex;
    rval &= 0xffffff;

    state->bitindex += number_of_bits;

    rval >>= (24-number_of_bits);

    state->wordpointer += (state->bitindex>>3);
    state->bitindex &= 7;
  }
  return rval;
}

unsigned int mp3internal_getbits_fast(struct mp3_decoder_state* state, int number_of_bits)
{
  unsigned long rval;

  {
    rval = state->wordpointer[0];
    rval <<= 8;	
    rval |= state->wordpointer[1];
    rval <<= state->bitindex;
    rval &= 0xffff;
    state->bitindex += number_of_bits;

    rval >>= (16-number_of_bits);

    state->wordpointer += (state->bitindex>>3);
    state->bitindex &= 7;
  }
  return rval;
}

void mp3internal_skipbits(struct mp3_decoder_state* state, int number_of_bits)
{
  state->bitindex += number_of_bits;
  state->wordpointer += (state->bitindex>>3);
  state->bitindex &= 7;
}
