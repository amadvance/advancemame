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

#include "wave.h"

#include <string.h>

typedef struct {
	char id[4];
	unsigned size;
} record_id;

typedef struct {
	unsigned wFormatTag; // Format category
	unsigned wChannels; // Number of channels
	unsigned dwSamplesPerSec; // Sampling rate
	unsigned dwAvgu8sPerSec; // For buffer estimation
	unsigned wBlockAlign; // Data block size
} record_fmt;

typedef struct {
	unsigned wBitsPerSample; // Sample size
} record_fmt_specific_PCM;

static int read_u16(const unsigned char** data_begin, const unsigned char* data_end, unsigned* p, size_t num) {
	unsigned count = 0;
	while (count<num) {
		if (*data_begin + 2 > data_end)
			return -1;
		*p = (*data_begin)[0] | (unsigned)(*data_begin)[1] << 8;
		*data_begin += 2;
		++count;
		++p;
	}
	return 0;
}

static int read_u32(const unsigned char** data_begin, const unsigned char* data_end, unsigned* p, size_t num) {
	unsigned count = 0;
	while (count<num) {
		if (*data_begin + 4 > data_end)
			return -1;
		*p = (*data_begin)[0] | (unsigned)(*data_begin)[1] << 8 | (unsigned)(*data_begin)[2] << 16 | (unsigned)(*data_begin)[3] << 24;
		*data_begin += 4;
		++count;
		++p;
	}
	return 0;
}

static int read_tag(const unsigned char** data_begin, const unsigned char* data_end, char* p) {
	if (*data_begin + 4 > data_end)
		return -1;
	memcpy(p,*data_begin,4);
	*data_begin += 4;
	return 0;
}

static int skip(const unsigned char** data_begin, const unsigned char* data_end, size_t num) {
	if (*data_begin + num > data_end)
		return -1;
	*data_begin += num;
	return 0;
}

static int read_id(const unsigned char** data_begin, const unsigned char* data_end, record_id* p) {
	return read_tag(data_begin, data_end, p->id )==0
		&& read_u32(data_begin, data_end, &p->size, 1)==0
		? 0 : -1;
}

static int read_fmt(const unsigned char** data_begin, const unsigned char* data_end, record_fmt* p) {
	return read_u16( data_begin, data_end, &p->wFormatTag, 1)==0 &&
		read_u16( data_begin, data_end, &p->wChannels, 1)==0 &&
		read_u32( data_begin, data_end, &p->dwSamplesPerSec, 1)==0 &&
		read_u32( data_begin, data_end, &p->dwAvgu8sPerSec, 1)==0 &&
		read_u16( data_begin, data_end, &p->wBlockAlign, 1)==0
		? 0 : -1;
}

static int read_fmt_PCM(const unsigned char** data_begin, const unsigned char* data_end, record_fmt_specific_PCM* p) {
	return read_u16( data_begin, data_end, &p->wBitsPerSample, 1 ) == 0
		? 0 : -1;
}

static int fread_u16(FZ* f, unsigned* p, size_t num) {
	unsigned count = 0;
	while (count<num) {
		unsigned char b0,b1;
		if (fzread(&b0, 1, 1, f) != 1 || fzread(&b1, 1, 1, f) != 1)
			return -1;
		*p = b0 | (unsigned)b1 << 8;
		++count;
		++p;
	}
	return 0;
}

static int fread_u32(FZ* f, unsigned* p, size_t num) {
	unsigned count = 0;
	while (count<num) {
		unsigned char b0,b1,b2,b3;
		if (fzread( &b0, 1, 1, f ) != 1 || fzread(&b1, 1, 1, f ) != 1 ||
		    fzread( &b2, 1, 1, f ) != 1 || fzread(&b3, 1, 1, f ) != 1)
			return -1;
		*p = b0 | (unsigned)b1 << 8 | (unsigned)b2 << 16 | (unsigned)b3 << 24;
		++count;
		++p;
	}
	return 0;
}

static int fread_tag(FZ* f, char* p) {
	return fzread( p, 4, 1, f ) == 1
		? 0 : -1;
}

static int fskip(FZ* f, size_t num) {
	unsigned count = 0;
	while (count<num) {
		char ch;
		if (fzread(&ch,1,1,f) != 1)
			return -1;
		++count;
	}
	return 0;
}

static int fread_id(FZ* f, record_id* p) {
	return fread_tag( f, p->id )==0
		&& fread_u32(f, &p->size, 1)==0
		? 0 : -1;
}

static int fread_fmt(FZ* f, record_fmt* p) {
	return fread_u16(f, &p->wFormatTag, 1)==0 &&
		fread_u16(f, &p->wChannels, 1)==0 &&
		fread_u32(f, &p->dwSamplesPerSec, 1)==0 &&
		fread_u32(f, &p->dwAvgu8sPerSec, 1)==0 &&
		fread_u16(f, &p->wBlockAlign, 1)==0
		? 0 : -1;
}

static int fread_fmt_PCM(FZ* f, record_fmt_specific_PCM* p) {
	return fread_u16( f, &p->wBitsPerSample, 1 )==0
		? 0 : -1;
}

/* Read a WAVE header from memory */
int wave_memory(const unsigned char** data_begin, const unsigned char* data_end, unsigned* data_nchannel, unsigned* data_bit, unsigned* data_size, unsigned* data_freq) {
	record_id riff_id;
	char wave_id[4];
	record_id fmt_id;
	record_fmt fmt;
	record_fmt_specific_PCM fmt_PCM;
	record_id data_id;

	if (read_id(data_begin, data_end, &riff_id)!=0) return -1;
	if (memcmp( riff_id.id, "RIFF", 4)!=0) return -1;

	if (read_tag(data_begin, data_end, wave_id)!=0) return -1;
	if (memcmp(wave_id, "WAVE", 4)!=0) return -1;

	// read until get fmt tag
	if (read_id(data_begin, data_end, &fmt_id)!=0) return -1;
	while (memcmp(fmt_id.id, "fmt ", 4)!=0) {
		// salta al prossimo chunk
		if (skip(data_begin, data_end, fmt_id.size)!=0) return -1;
		// legge il chunk
		if (read_id(data_begin, data_end, &fmt_id)!=0) return -1;
	}
	if (memcmp(fmt_id.id, "fmt ", 4)!=0) return -1;
	if (fmt_id.size < 16) return -1;

	if (read_fmt(data_begin, data_end, &fmt)!=0) return -1;
	if (fmt.wFormatTag != 0x0001) return -1; //  Microsoft Pulse Code Modulation (PCM) format
	if (fmt.wChannels < 1 || fmt.wChannels > 2) return -1;

	if (read_fmt_PCM(data_begin, data_end, &fmt_PCM)!=0) return -1;
	if (fmt_PCM.wBitsPerSample < 4 || fmt_PCM.wBitsPerSample>16) return -1;

	// skip fmt unknown extension
	if (fmt_id.size > 16) {
		if (skip(data_begin, data_end, fmt_id.size - 16)!=0) return -1;
	}

	// read until get data tag
	if (read_id(data_begin, data_end, &data_id)!=0) return -1;
	while (memcmp(data_id.id, "data", 4)!=0) {
		// salta al prossimo chunk
		if (skip(data_begin, data_end, data_id.size)!=0) return -1;
		// legge il chunk
		if (read_id(data_begin, data_end, &data_id)!=0) return -1;
	}
	if (memcmp(data_id.id, "data", 4)!=0) return -1;

	if (data_end - *data_begin < data_id.size)
		return -1;

	// limitations
	if (fmt.wChannels!=1 && fmt.wChannels!=2)
		return -1;
	if (fmt_PCM.wBitsPerSample!=8 && fmt_PCM.wBitsPerSample!=16)
		return -1;

	*data_nchannel = fmt.wChannels;
	*data_bit = fmt_PCM.wBitsPerSample;
	*data_size = data_id.size;
	*data_freq = fmt.dwSamplesPerSec;

	return 0;
}

/* Read a WAVE header from a file */
int wave_file(FZ* f, unsigned* data_nchannel, unsigned* data_bit, unsigned* data_size, unsigned* data_freq) {
	record_id riff_id;
	char wave_id[4];
	record_id fmt_id;
	record_fmt fmt;
	record_fmt_specific_PCM fmt_PCM;
	record_id data_id;

	if (fread_id(f, &riff_id)!=0) return -1;
	if (memcmp( riff_id.id, "RIFF", 4)!=0) return -1;

	if (fread_tag(f, wave_id)!=0) return -1;
	if (memcmp( wave_id, "WAVE", 4)!=0) return -1;

	// read until get fmt tag
	if (fread_id(f, &fmt_id)!=0) return -1;
	while (memcmp(fmt_id.id, "fmt ", 4)!=0) {
		// salta al prossimo chunk
		if (fskip(f, fmt_id.size)!=0) return -1;
		// legge il chunk
		if (fread_id(f, &fmt_id)!=0) return -1;
	}
	if (memcmp(fmt_id.id, "fmt ", 4)!=0) return -1;
	if (fmt_id.size < 16) return -1;

	if (fread_fmt(f, &fmt)!=0) return -1;
	if (fmt.wFormatTag != 0x0001) return -1; //  Microsoft Pulse Code Modulation (PCM) format
	if (fmt.wChannels < 1 || fmt.wChannels > 2) return -1;

	if (fread_fmt_PCM(f, &fmt_PCM)!=0) return -1;
	if (fmt_PCM.wBitsPerSample < 4 || fmt_PCM.wBitsPerSample>16) return -1;

	// skip fmt unknown extension
	if (fmt_id.size > 16) {
		if (fskip(f, fmt_id.size - 16)!=0) return -1;
	}

	// read until get data tag
	if (fread_id(f, &data_id)!=0) return -1;
	while (memcmp( data_id.id, "data", 4)!=0) {
		// salta al prossimo chunk
		if (fskip(f, data_id.size)!=0) return -1;
		// legge il chunk
		if (fread_id(f, &data_id)!=0) return -1;
	}
	if (memcmp(data_id.id, "data", 4)!=0) return -1;

	// limitations
	if (fmt.wChannels!=1 && fmt.wChannels!=2)
		return -1;
	if (fmt_PCM.wBitsPerSample!=8 && fmt_PCM.wBitsPerSample!=16)
		return -1;

	*data_nchannel = fmt.wChannels;
	*data_bit = fmt_PCM.wBitsPerSample;
	*data_size = data_id.size;
	*data_freq = fmt.dwSamplesPerSec;

	return 0;
}
