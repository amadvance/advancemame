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

/*
 * Slighlty optimized for machines without autoincrement/decrement.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mpg123.h"
#include "mpglib.h"

static inline void write_le(unsigned char* ptr, int v)
{
	ptr[0] = v & 0xFF;
	ptr[1] = (v >> 8) & 0xFF;
}

static inline int write_clip(unsigned char* ptr, mp3internal_real sum)
{
	if (sum > 32767.0) {
		write_le(ptr, 0x7fff);
		return 1;
	} else if (sum < -32768.0) {
		write_le(ptr, -0x8000);
		return 1;
	} else {
		write_le(ptr, sum);
		return 0;
	}
}

int mp3internal_synth_1to1_mono(struct mp3_decoder_state* state, mp3internal_real *bandPtr,unsigned char *samples,int *pnt)
{
	unsigned char ptr_samples_tmp[64*2];
	unsigned char* ptr_tmp1 = ptr_samples_tmp;
	unsigned i;
	int ret;
	int pnt1 = 0;

	ret = mp3internal_synth_1to1(state, bandPtr, 0, ptr_samples_tmp, &pnt1);

	samples += *pnt;

	for(i=0;i<32;i++) {
		samples[0] = ptr_tmp1[0];
		samples[1] = ptr_tmp1[1];
		samples += 2;
		ptr_tmp1 += 4;
	}

	*pnt += 64;

	return ret;
}

int mp3internal_synth_1to1(struct mp3_decoder_state* state, mp3internal_real *bandPtr, int channel, unsigned char *out, int *pnt)
{
  int bo;
  unsigned char* ptr_samples = out + *pnt;
  mp3internal_real *b0,(*buf)[0x110];
  int clip = 0; 
  int bo1;

  bo = state->synth_bo;

  if(!channel) {
    bo--;
    bo &= 0xf;
    buf = state->synth_buffs[0];
  }
  else {
    ptr_samples += 2;
    buf = state->synth_buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    mp3internal_dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
    mp3internal_dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
  }

  state->synth_bo = bo;
  
  {
    int j;
    mp3internal_real *window = mp3internal_decwin + 16 - bo1;

    for (j=16;j;j--,b0+=0x10,window+=0x20,ptr_samples+=4)
    {
      mp3internal_real sum;
      sum  = window[0x0] * b0[0x0];
      sum -= window[0x1] * b0[0x1];
      sum += window[0x2] * b0[0x2];
      sum -= window[0x3] * b0[0x3];
      sum += window[0x4] * b0[0x4];
      sum -= window[0x5] * b0[0x5];
      sum += window[0x6] * b0[0x6];
      sum -= window[0x7] * b0[0x7];
      sum += window[0x8] * b0[0x8];
      sum -= window[0x9] * b0[0x9];
      sum += window[0xA] * b0[0xA];
      sum -= window[0xB] * b0[0xB];
      sum += window[0xC] * b0[0xC];
      sum -= window[0xD] * b0[0xD];
      sum += window[0xE] * b0[0xE];
      sum -= window[0xF] * b0[0xF];

	clip += write_clip(ptr_samples, sum);
    }

    {
      mp3internal_real sum;
      sum  = window[0x0] * b0[0x0];
      sum += window[0x2] * b0[0x2];
      sum += window[0x4] * b0[0x4];
      sum += window[0x6] * b0[0x6];
      sum += window[0x8] * b0[0x8];
      sum += window[0xA] * b0[0xA];
      sum += window[0xC] * b0[0xC];
      sum += window[0xE] * b0[0xE];
	clip += write_clip(ptr_samples, sum);
      b0-=0x10,window-=0x20,ptr_samples+=4;
    }
    window += bo1<<1;

    for (j=15;j;j--,b0-=0x10,window-=0x20,ptr_samples+=4)
    {
      mp3internal_real sum;
      sum = -window[-0x1] * b0[0x0];
      sum -= window[-0x2] * b0[0x1];
      sum -= window[-0x3] * b0[0x2];
      sum -= window[-0x4] * b0[0x3];
      sum -= window[-0x5] * b0[0x4];
      sum -= window[-0x6] * b0[0x5];
      sum -= window[-0x7] * b0[0x6];
      sum -= window[-0x8] * b0[0x7];
      sum -= window[-0x9] * b0[0x8];
      sum -= window[-0xA] * b0[0x9];
      sum -= window[-0xB] * b0[0xA];
      sum -= window[-0xC] * b0[0xB];
      sum -= window[-0xD] * b0[0xC];
      sum -= window[-0xE] * b0[0xD];
      sum -= window[-0xF] * b0[0xE];
      sum -= window[-0x0] * b0[0xF];

	clip += write_clip(ptr_samples, sum);
    }
  }
  *pnt += 128;

  return clip;
}
