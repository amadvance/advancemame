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

#include "advance.h"

#include <zlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/***************************************************************************/
/* fread/fwrite */

static void write_be_u32(uint8* v, unsigned i) {
	v[0] = (i >> 24) & 0xFF;
	v[1] = (i >> 16) & 0xFF;
	v[2] = (i >> 8) & 0xFF;
	v[3] = i & 0xFF;
}

static size_t fwrite_le_u16(unsigned* p, size_t num, FILE* f) {
	unsigned count = 0;
	while (count<num) {
		unsigned char b0,b1;
		b0 = *p & 0xFF;
		b1 = (*p >> 8) & 0xFF;
		if (fwrite(&b0, 1, 1, f) != 1 || fwrite(&b1, 1, 1, f) != 1) 
			return count;
		++count;
		++p;
	}
	return count;
}


static size_t fwrite_le_u32(unsigned* p, size_t num, FILE* f) {
	unsigned count = 0;
	while (count<num) {
		unsigned char b0,b1,b2,b3;
		b0 = *p & 0xFF;
		b1 = (*p >> 8) & 0xFF;
		b2 = (*p >> 16) & 0xFF;
		b3 = (*p >> 24) & 0xFF;
		if (fwrite(&b0, 1, 1, f) != 1 || fwrite(&b1, 1, 1, f) != 1 ||
			fwrite(&b2, 1, 1, f) != 1 || fwrite(&b3, 1, 1, f) != 1)
			return count;
		++count;
		++p;
	}
	return count;
}

/***************************************************************************/
/* Wave */

static size_t fwrite_tag(char* p, FILE* f) {
	return fwrite( p, 1, 4, f ) == 4;
}

static size_t fskip(size_t num, FILE* f) {    
	unsigned count = 0;
	while (count<num) {
		int c = getc(f);
		if (c==EOF) return count;
		++count;
	}
	return count;
}

/* record_id */

typedef struct {
	char id[4];
	unsigned size;
} record_id;

static size_t fwrite_id( record_id* p, FILE* f) {
	return fwrite_tag( p->id, f ) && fwrite_le_u32( &p->size, 1, f );
}

#define WAVE_FORMAT_PCM      0x0001 /* Microsoft Pulse Code Modulation (PCM) format */
#define IBM_FORMAT_MULAW     0x0101 /* IBM mu-law format */
#define IBM_FORMAT_ALAW      0x0102 /* IBM a-law format */
#define IBM_FORMAT_ADPCM     0x0103 /* IBM AVC Adaptive Differential Pulse Code Modulation format */

typedef struct {
	unsigned wFormatTag; /* Format category */
	unsigned wChannels; /* Number of channels */
	unsigned dwSamplesPerSec; /* Sampling rate */
	unsigned dwAvgu8sPerSec; /* For buffer estimation */
	unsigned wBlockAlign; /* Data block size */
} record_fmt;

static size_t fwrite_fmt( record_fmt* p, FILE* f) {
	return fwrite_le_u16( &p->wFormatTag, 1, f ) &&
		fwrite_le_u16( &p->wChannels, 1, f ) &&
		fwrite_le_u32( &p->dwSamplesPerSec, 1, f ) &&
		fwrite_le_u32( &p->dwAvgu8sPerSec, 1, f ) &&
		fwrite_le_u16( &p->wBlockAlign, 1, f );
}

/* record_fmt_PCM */

typedef struct {
	unsigned wBitsPerSample; /* Sample size */
} record_fmt_specific_PCM;

static size_t fwrite_fmt_PCM( record_fmt_specific_PCM* p, FILE* f) {
	return fwrite_le_u16( &p->wBitsPerSample, 1, f);
}

static size_t fwrite_header(int speed, int bit, int channel, int size, FILE* f) {
	record_id riff_id;
	char wave_id[4];
	record_id fmt_id;
	record_fmt fmt;
	record_fmt_specific_PCM fmt_PCM;
	record_id data_id;
	
	unsigned size_byte;
	if (bit <= 8)
		size_byte = 1;
	else if (bit <= 16)
		size_byte = 2;
	else
		size_byte = 4;
	
	memcpy(riff_id.id,"RIFF",4);
	riff_id.size = 0x24 + size;
	if (!fwrite_id( &riff_id, f )) return 0;
	
	memcpy(wave_id,"WAVE",4);
	if (!fwrite_tag( wave_id, f )) return 0;

	memcpy(fmt_id.id, "fmt ", 4);
	fmt_id.size = 0x10;
	if (!fwrite_id( &fmt_id, f )) return 0;
	
	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.wChannels = channel; 
	fmt.dwSamplesPerSec = speed;
	fmt.dwAvgu8sPerSec = size_byte*channel*speed;
	fmt.wBlockAlign = size_byte*channel;
	if (!fwrite_fmt( &fmt, f )) return 0;

	fmt_PCM.wBitsPerSample = bit;
	if (!fwrite_fmt_PCM( &fmt_PCM, f )) return 0;
	
	memcpy( data_id.id, "data", 4);
	data_id.size = size;
	if (!fwrite_id( &data_id, f )) return 0;

	return 1;
}

static size_t fwrite_header_size(int size, FILE* f) {
	unsigned dsize;

	dsize = 0x24 + size;
	if (fseek(f,4,SEEK_SET)!=0) 
		return 0;
	if (!fwrite_le_u32( &dsize, 1, f)) 
		return 0;

	dsize = size;
	if (fseek(f,0x28,SEEK_SET)!=0) 
		return 0;
	if (!fwrite_le_u32( &dsize, 1, f)) 
		return 0;

	return 1;
}

/***************************************************************************/
/* Sound */

#define SOUND_FREQUENCY_ERROR 0.01

static unsigned SOUND_FREQUENCY_MAP[] = {
	48000, /* MPEG1 */
	44100, /* MPEG1 */
	32000, /* MPEG1 */
	24000, /* MPEG2 */
	22050, /* MPEG2 */
	16000, /* MPEG2 */
	12000, /* MPEG2.5 */
	11025, /* MPEG2.5 */
	8000, /* MPEG2.5 */
	0
};

static void sound_cancel(struct advance_record_context* context) {

	if (!context->state.sound_active_flag)
		return;

	context->state.sound_active_flag = 0;

	fclose(context->state.sound_f);
	remove(context->state.sound_file);
}

static int sound_start(struct advance_record_context* context, const char* file, double frequency, int stereo) {
	unsigned* f;

	if (context->state.sound_active_flag)
		return -1;

	/* adjust the frequency to match the MP3 standard */
	f = SOUND_FREQUENCY_MAP;
	while (*f) {
		double v = *f;
		v /= frequency;
		if (v > 1)
			v = 1 / v;
		v = 1 - v;
		if (v<SOUND_FREQUENCY_ERROR) {
			context->state.sound_frequency = *f;
			break;
		}
                 ++f;
	}
	if (!*f)
		context->state.sound_frequency = frequency;

	context->state.sound_stereo_flag = stereo;
	context->state.sound_sample_counter = 0;
	context->state.sound_sample_size = 2;
	if (stereo)
		context->state.sound_sample_size *= 2;

	strcpy(context->state.sound_file,file);

	context->state.sound_f = fopen(context->state.sound_file, "wb");
	if (!context->state.sound_f) {
		log_std(("ERROR: opening file %s\n", context->state.sound_file));
		return -1;
	}

	if (fwrite_header(context->state.sound_frequency, 16, context->state.sound_stereo_flag ? 2 : 1, 0, context->state.sound_f)!=1) {
		log_std(("ERROR: writing file %s\n", context->state.sound_file));
		fclose(context->state.sound_f);
		remove(context->state.sound_file);
		return -1;
	}

	context->state.sound_active_flag = 1;

	return 0;
}

/**
 * Insert some data in the sound recording. Automatically save if full.
 * \param map samples buffer
 * \param mac number of 16 bit samples mono or stereo in little endian format
 */
static int sound_update(struct advance_record_context* context, const short* map, unsigned mac) {
	if (!context->state.sound_active_flag)
		return -1;

	if (context->state.sound_sample_counter / context->state.sound_frequency >= context->config.sound_time)
		return -1;

	if (fwrite(map, mac * context->state.sound_sample_size, 1, context->state.sound_f)!=1)
		goto err;

	context->state.sound_sample_counter += mac;

	return 0;
err:
	log_std(("ERROR: writing file %s\n", context->state.sound_file));
	sound_cancel(context);
	return -1;
}

/* Save and stop the current file recording */
static int sound_stop(struct advance_record_context* context, unsigned* time) {

	if (!context->state.sound_active_flag)
		return -1;

	context->state.sound_active_flag = 0;

	if (fwrite_header_size(context->state.sound_sample_size * context->state.sound_sample_counter, context->state.sound_f) != 1) {
		log_std(("ERROR: writing header file %s\n", context->state.sound_file));
		fclose(context->state.sound_f);
		remove(context->state.sound_file);
		return -1;
	}

	fclose(context->state.sound_f);

	*time = context->state.sound_sample_counter / context->state.sound_frequency;

	return 0;
}

/*************************************************************************************/
/* PNG/MNG */

/** Use the FRAM chunk to get a more precise timing */
#define USE_MNG_LC

#define PNG_CN_IHDR 0x49484452
#define PNG_CN_PLTE 0x504C5445
#define PNG_CN_IDAT 0x49444154
#define PNG_CN_IEND 0x49454E44
#define MNG_CN_MHDR 0x4D484452
#define MNG_CN_MEND 0x4D454E44
#define MNG_CN_TERM 0x5445524D
#define MNG_CN_BACK 0x4241434B
#define MNG_CN_FRAM 0x4652414d

static uint8 MNG_Signature[] = "\x8A\x4D\x4E\x47\x0D\x0A\x1A\x0A";
static uint8 PNG_Signature[] = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";

static void png_orientation_size(unsigned* width, unsigned* height, unsigned orientation) {
	if (orientation & OSD_ORIENTATION_SWAP_XY) {
		SWAP(unsigned, *width, *height);
	}
}

static void png_orientation(const uint8** ptr, unsigned* width, unsigned* height, int* pixel_pitch, int* line_pitch, unsigned orientation)
{
	if ((orientation & OSD_ORIENTATION_SWAP_XY)!=0) {
		SWAP(unsigned, *width, *height);
		SWAP(int, *pixel_pitch, *line_pitch);
	}

	if ((orientation & OSD_ORIENTATION_FLIP_X)!=0) {
		*ptr += *pixel_pitch * (*width - 1);
		*pixel_pitch = - *pixel_pitch;
	}

	if ((orientation & OSD_ORIENTATION_FLIP_Y)!=0) {
		*ptr += *line_pitch * (*height - 1);
		*line_pitch = - *line_pitch;
	}
}

static int png_write_chunk(FILE* f, unsigned chunk_type, uint8* chunk_data, unsigned chunk_length)
{
	uint8 v[4];
	unsigned crc;

	write_be_u32(v, chunk_length);
	if (fwrite(v, 4, 1, f) != 1)
		return -1;

	write_be_u32(v, chunk_type);
	if (fwrite(v, 4, 1, f) != 1)
		return -1;

	crc = crc32(0, v, 4);
	if (chunk_length > 0) {
		if (fwrite(chunk_data, chunk_length, 1, f) != 1)
			return -1;

		crc = crc32(crc, chunk_data, chunk_length);
	}

	write_be_u32(v, crc);
	if (fwrite(v, 4, 1, f) != 1)
		return -1;

	return 0;
}

static int png_write_header(FILE* f) {
	if (fwrite(PNG_Signature, 8, 1, f) != 1)
		return -1;

	return 0;
}

static int png_write_footer(FILE* f) {
	return 0;
}

static int mng_write_header(FILE* f, unsigned width, unsigned height, unsigned frequency, unsigned orientation) {
	uint8 mhdr[28];
	unsigned simplicity;

	png_orientation_size(&width, &height, orientation);

	if (fwrite(MNG_Signature, 8, 1, f) != 1)
		return -1;

	simplicity = (1 << 0)
#ifdef USE_MNG_LC
		| (1 << 1)
#endif
		| (1 << 6);

	memset(mhdr, 0, 28);
	write_be_u32(mhdr, width);
	write_be_u32(mhdr + 4, height);
	write_be_u32(mhdr + 8, frequency);
	write_be_u32(mhdr + 24, simplicity);

	if (png_write_chunk(f, MNG_CN_MHDR, mhdr, 28)!=0)
		return -1;

	return 0;
}

static int mng_write_footer(FILE* f) {

	if (png_write_chunk(f, MNG_CN_MEND, 0, 0)!=0)
		return -1;

	return 0;
}

#ifdef USE_MNG_LC
static int mng_write_image_frame(FILE* f, unsigned tick) {
	uint8 fram[10];
	unsigned fi;

	fi = tick;
	if (fi < 1)
		fi = 1;

	fram[0] = 1; /* Framing_mode: 1 */
	fram[1] = 0; /* Null byte */
	fram[2] = 2; /* Reset delay */
	fram[3] = 0; /* No timeout change */
	fram[4] = 0; /* No clip change */
	fram[5] = 0; /* No sync id change */
	write_be_u32(fram+6, fi); /* Delay in tick */

	if (png_write_chunk(f, MNG_CN_FRAM, fram, 10)!=0)
		return -1;

	return 0;
}
#endif

static int png_write_image_header(FILE* f, unsigned width, unsigned height, unsigned bit_depth, unsigned color_type, unsigned orientation) {
	uint8 ihdr[13];

	png_orientation_size(&width, &height, orientation);

	write_be_u32(ihdr, width);
	write_be_u32(ihdr+4, height);

	ihdr[8] = bit_depth;
	ihdr[9] = color_type;
	ihdr[10] = 0; /* compression */
	ihdr[11] = 0; /* filter */
	ihdr[12] = 0; /* interlace */

	if (png_write_chunk(f, PNG_CN_IHDR, ihdr, 13)!=0)
		return -1;

	return 0;
}

static int png_write_image_footer(FILE* f) {
	if (png_write_chunk(f, PNG_CN_IEND, 0, 0)!=0)
		return -1;

	return 0;
}

static int png_write_data(FILE* f, unsigned width, unsigned height, const uint8* ptr, unsigned bytes_per_pixel, unsigned bytes_per_scanline, int fast) {
	uint8* z_ptr;
	uint8* f_ptr;
	unsigned long z_size;
	unsigned long f_size;
	uint8* p;
	unsigned i;
	unsigned method;

	f_size = height * (width * bytes_per_pixel+1);
	z_size = f_size * 102 / 100 + 12;

	z_ptr = malloc(z_size);
	f_ptr = malloc(f_size);
	if (!z_ptr || !f_ptr)
		goto err_free;

	p = f_ptr;
	for(i=0;i<height;++i) {
		*p++ = 0; /* no filter */
		memcpy(p, ptr, width * bytes_per_pixel);
		ptr += bytes_per_scanline;
		p += width * bytes_per_pixel;
	}

	if (fast)
		method = 1;
	else
		method = 6;

	if (compress2(z_ptr, &z_size, f_ptr, f_size, method) != Z_OK)
		goto err_free;

	if (png_write_chunk(f, PNG_CN_IDAT, z_ptr, z_size)!=0)
		goto err_free;

	free(z_ptr);
	free(f_ptr);

	return 0;

err_free:
	free(z_ptr);
	free(f_ptr);

	return -1;
}

static int png_write_image_32rgb(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, int fast, unsigned orientation) {
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i,j;
	int pixel_pitch = 4;
	int line_pitch = bytes_per_scanline;

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	i_size = height * (width * 3);
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			p[0] = ptr[0];
			p[1] = ptr[1];
			p[2] = ptr[2];

			p += 3;
			ptr += pixel_pitch;
		}

		ptr += line_pitch - pixel_pitch * width;
	}

	if (png_write_data(f, width, height, i_ptr, 3, 3 * width, fast) != 0)
		goto err_free;

	free(i_ptr);

	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

static int png_write_image_15rgb(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, int fast, unsigned orientation) {
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i,j;
	int pixel_pitch = 2;
	int line_pitch = bytes_per_scanline;

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	i_size = height * (width * 3);
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			unsigned v = *(uint16*)ptr;
			unsigned r = (v >> 10) & 0x1F;
			unsigned g = (v >> 5) & 0x1F;
			unsigned b = (v & 0x1F);
			p[0] = (r << 3) | (r >> 2);
			p[1] = (g << 3) | (g >> 2);
			p[2] = (b << 3) | (b >> 2);
			p += 3;
			ptr += pixel_pitch;
		}
		ptr += line_pitch - pixel_pitch * width;
	}

	if (png_write_data(f, width, height, i_ptr, 3, 3 * width, fast) != 0)
		goto err_free;

	free(i_ptr);

	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

static int png_write_image_16pal(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, osd_rgb_t* rgb, unsigned rgb_max, int fast, unsigned orientation) {
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i,j;
	int pixel_pitch = 2;
	int line_pitch = bytes_per_scanline;

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	i_size = height * (width * 3);
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			unsigned v = *(uint16*)ptr;
			p[0] = osd_rgb_red(rgb[v]);
			p[1] = osd_rgb_green(rgb[v]);
			p[2] = osd_rgb_blue(rgb[v]);
			p += 3;
			ptr += pixel_pitch;
		}
		ptr += line_pitch - pixel_pitch * width;
	}

	if (png_write_data(f, width, height, i_ptr, 3, 3 * width, fast) != 0)
		goto err_free;

	free(i_ptr);

	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

static int png_write_image_16palsmall(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, osd_rgb_t* rgb, unsigned rgb_max, int fast, unsigned orientation) {
	uint8 palette[3*256];
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i,j;
	int pixel_pitch = 2;
	int line_pitch = bytes_per_scanline;

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	for (i=0;i<rgb_max;++i) {
		palette[i*3] = osd_rgb_red(rgb[i]);
		palette[i*3+1] = osd_rgb_green(rgb[i]);
		palette[i*3+2] = osd_rgb_blue(rgb[i]);
	}

	if (png_write_chunk(f, PNG_CN_PLTE, palette, rgb_max*3)!=0)
		goto err;

	i_size = height * width;
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			p[0] = *ptr;
			p += 1;
			ptr += pixel_pitch;
		}
		ptr += line_pitch - pixel_pitch * width;
	}

	if (png_write_data(f, width, height, i_ptr, 1, 1 * width, fast) != 0)
		goto err_free;

	free(i_ptr);

	return 0;

err_free:
	free(i_ptr);
err:
	return -1;
}

/*************************************************************************************/
/* Video */

/* Cancel a video recording */
static void video_cancel(struct advance_record_context* context) {
	if (!context->state.video_active_flag)
		return;

	context->state.video_active_flag = 0;

	fclose(context->state.video_f);
	remove(context->state.video_file);
}

static void video_freq_step(unsigned* base, unsigned* step, double freq) {
	unsigned b = 0;
	unsigned s;
	double err;

	do {
		double r;

		++b;

		s = b / freq;
		if (s < 1)
			s = 1;

		r = b / (double)s;

		err = (r - freq) / freq;
		if (err < 0)
			err = - err;

		/* no more than 1:1000000 error */
	} while (err > 1E-6);

	*base = b;
	*step = s;
}

static int video_start(struct advance_record_context* context, const char* file, double frequency, unsigned width, unsigned height, unsigned orientation) {
	unsigned mng_frequency;
	unsigned mng_step;

	if (context->state.video_active_flag)
		return -1;

	context->state.video_frequency = frequency;
	context->state.video_sample_counter = 0;

	strcpy(context->state.video_file,file);

	context->state.video_f = fopen(context->state.video_file, "wb");
	if (!context->state.video_f) {
		log_std(("ERROR: opening file %s\n", context->state.video_file));
		return -1;
	}

#ifdef USE_MNG_LC
	video_freq_step(&mng_frequency, &mng_step, frequency / context->config.video_interlace);
#else
	mng_frequency = frequency / context->config.video_interlace;
	if (mng_frequency < 1)
		mng_frequency = 1;
	mng_step = 1;
#endif

	context->state.video_freq_base = mng_frequency;
	context->state.video_freq_step = mng_step;

	if (mng_write_header(context->state.video_f, width, height, context->state.video_freq_base, orientation) != 0) {
		log_std(("ERROR: writing header in file %s\n", context->state.video_file));
		fclose(context->state.video_f);
		remove(context->state.video_file);
		return -1;
	}

	context->state.video_active_flag = 1;

	return 0;
}

/**
 * Insert some data in the video recording. Automatically save if full.
 * \param map samples buffer
 * \param mac number of 16 bit samples mono or stereo in little endian format
 */
static int video_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, video_rgb_def rgb_def, osd_rgb_t* palette_map, unsigned palette_max, unsigned orientation) {
	unsigned color_type;

	if (!context->state.video_active_flag)
		return -1;

	if (context->state.video_sample_counter / context->state.video_frequency >= context->config.video_time)
		return -1;

	context->state.video_sample_counter += 1;

	/* skip frames */
	if (context->state.video_sample_counter % context->config.video_interlace != 0) {
		return 0;
	}

	if (palette_map && palette_max <= 256)
		color_type = 3;
	else
		color_type = 2;

#ifdef USE_MNG_LC
	if (mng_write_image_frame(context->state.video_f, context->state.video_freq_step) != 0) {
		log_std(("ERROR: writing image frame in file %s\n", context->state.video_file));
		goto err;
	}
#endif

	if (png_write_image_header(context->state.video_f, video_width, video_height, 8, color_type, orientation) != 0) {
		log_std(("ERROR: writing image header in file %s\n", context->state.video_file));
		goto err;
	}

	if (video_bytes_per_pixel == 2 && palette_map && palette_max <= 256) {
		if (png_write_image_16palsmall(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 1, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 4 && !palette_map) {
		if (png_write_image_32rgb(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, 1, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && !palette_map) {
		if (png_write_image_15rgb(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, 1, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && palette_map) {
		if (png_write_image_16pal(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 1, orientation)!=0)
			goto err_data;
	} else {
		log_std(("ERROR: unknown image format for file %s\n", context->state.video_file));
		goto err;
	}

	if (png_write_image_footer(context->state.video_f) != 0) {
		log_std(("ERROR: writing image footer in file %s\n", context->state.video_file));
		goto err;
	}

	return 0;
err_data:
	log_std(("ERROR: writing image data in file %s\n", context->state.video_file));
err:
	video_cancel(context);
	return -1;
}

/* Save and stop the current file recording */
static int video_stop(struct advance_record_context* context, unsigned* time) {
	if (!context->state.video_active_flag)
		return -1;

	context->state.video_active_flag = 0;

	if (mng_write_footer(context->state.video_f)!=0) {
		goto err;
	}

	fclose(context->state.video_f);

	*time = context->state.video_sample_counter / context->state.video_frequency;

	return 0;

err:
	log_std(("ERROR: closing file %s\n", context->state.video_file));
	fclose(context->state.video_f);
	remove(context->state.video_file);
	return -1;
}

/*************************************************************************************/
/* Snapshot */

static int snapshot_start(struct advance_record_context* context, const char* file) {
	context->state.snapshot_active_flag = 1;
	strcpy(context->state.snapshot_file, file);

	return 0;
}

static int snapshot_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, video_rgb_def rgb_def, osd_rgb_t* palette_map, unsigned palette_max, unsigned orientation) {
	const char* file = context->state.snapshot_file;
	FILE* f;
	unsigned color_type;

	if (!context->state.snapshot_active_flag)
		return -1;

	context->state.snapshot_active_flag = 0;

	f = fopen(file, "wb");
	if (!f) {
		log_std(("ERROR: opening file %s\n", file));
		goto err;
	}

	if (png_write_header(f) != 0) {
		log_std(("ERROR: writing header in file %s\n", file));
		goto err_file;
	}

	if (palette_map && palette_max <= 256)
		color_type = 3;
	else
		color_type = 2;

	if (png_write_image_header(f, video_width, video_height, 8, color_type, orientation) != 0) {
		log_std(("ERROR: writing image header in file %s\n", file));
		goto err_file;
	}

	if (video_bytes_per_pixel == 2 && palette_map && palette_max <= 256) {
		if (png_write_image_16palsmall(f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 0, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 4 && !palette_map) {
		if (png_write_image_32rgb(f, video_buffer, video_bytes_per_scanline, video_width, video_height, 0, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && !palette_map) {
		if (png_write_image_15rgb(f, video_buffer, video_bytes_per_scanline, video_width, video_height, 0, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && palette_map) {
		if (png_write_image_16pal(f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 0, orientation)!=0)
			goto err_data;
	} else {
		log_std(("ERROR: unknown image format for file %s\n", file));
		goto err_file;
	}

	if (png_write_image_footer(f) != 0) {
		log_std(("ERROR: writing image footer in file %s\n", file));
		goto err_file;
	}

	if (png_write_footer(f)!=0) {
		log_std(("ERROR: writing file footer in file %s\n", file));
		goto err_file;
	}

	fclose(f);

	return 0;
err_data:
	log_std(("ERROR: writing image data in file %s\n", file));
err_file:
	fclose(f);
	remove(file);
err:
	return -1;
}

/*************************************************************************************/
/* OSD */

static void advance_record_next(struct advance_record_context* context, const mame_game* game, char* path_wav, char* path_mng) {
	unsigned counter = 0;

	sprintf(path_wav,"%s/%.8s.wav", context->config.dir, mame_game_name(game));
	sprintf(path_mng,"%s/%.8s.mng", context->config.dir, mame_game_name(game));

	if (access(path_wav,F_OK)==0 || access(path_mng,F_OK)==0) {
		do {
			sprintf(path_wav,"%s/%.4s%04d.wav", context->config.dir, mame_game_name(game), counter);
			sprintf(path_mng,"%s/%.4s%04d.mng", context->config.dir, mame_game_name(game), counter);
			++counter;
		} while (access(path_wav,F_OK)==0 || access(path_mng,F_OK)==0);
	}
}

void osd_record_start(void)
{
	struct advance_record_context* context = &CONTEXT.record;
	struct advance_sound_context* sound_context = &CONTEXT.sound;
	struct advance_video_context* video_context = &CONTEXT.video;
	const mame_game* game = CONTEXT.game;
	char path_wav[FILE_MAXPATH];
	char path_mng[FILE_MAXPATH];

	if (context->config.sound_flag && sound_context->state.rate) {
		sound_cancel(context); /* ignore error */
	}

	if (context->config.video_flag) {
		video_cancel(context); /* ignore error */
	}

	advance_record_next(context, game, path_wav, path_mng);

	if (context->config.sound_flag && !context->config.video_flag)
		mame_ui_message("Start recording");

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	log_std(("osd: osd_record_start()\n"));

	if (context->config.sound_flag && sound_context->state.rate) {
		sound_start(context, path_wav, sound_context->state.rate, sound_context->state.stereo_flag);
	}

	if (context->config.video_flag) {
		unsigned size_x = video_context->state.game_used_size_x;
		unsigned size_y = video_context->state.game_used_size_y;

		/* restore the original orientation */
		if ((video_context->config.blit_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
			SWAP(unsigned, size_x, size_y);
		}

		video_start(context, path_mng, video_context->state.game_fps, size_x, size_y, video_context->config.game_orientation);
	}

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

void osd_record_stop(void)
{
	struct advance_record_context* context = &CONTEXT.record;
	struct advance_sound_context* sound_context = &CONTEXT.sound;

	unsigned sound_time = 0;
	unsigned video_time = 0;

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	log_std(("osd: osd_record_stop()\n"));

	if (context->config.sound_flag) {
		if (sound_stop(context, &sound_time) != 0) {
			sound_time = 0;
		}
	}

	if (context->config.video_flag) {
		if (video_stop(context, &video_time) != 0) {
			video_time = 0;
		}
	}

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif

	if (sound_time != 0 || video_time != 0) {
		mame_ui_message("Stop recording %d/%d [s]",sound_time,video_time);
	}
}

static void advance_snapshot_next(struct advance_record_context* context, const mame_game* game, char* path_png) {
	unsigned counter = 0;

	sprintf(path_png,"%s/%.8s.png", context->config.dir, mame_game_name(game));

	if (access(path_png,F_OK)==0) {
		do {
			sprintf(path_png,"%s/%.4s%04d.png", context->config.dir, mame_game_name(game), counter);
			++counter;
		} while (access(path_png,F_OK)==0);
	}
}

void osd2_save_snapshot(unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
	struct advance_record_context* context = &CONTEXT.record;
	struct advance_sound_context* sound_context = &CONTEXT.sound;
	struct advance_video_context* video_context = &CONTEXT.video;
	const mame_game* game = CONTEXT.game;
	char path_png[FILE_MAXPATH];

	log_std(("osd: osd_save_snapshot(x1:%d,y1:%d,x2:%d,y2:%d)\n", x1, y1, x2, y2));

	advance_snapshot_next(context, game, path_png);

	snapshot_start(context, path_png);
}

/*************************************************************************************/
/* Advance */

int advance_record_sound_is_active(struct advance_record_context* context) {
	return context->state.sound_active_flag
		&& context->state.sound_sample_counter / context->state.sound_frequency < context->config.sound_time;
}

int advance_record_video_is_active(struct advance_record_context* context) {
	return context->state.video_active_flag
		&& context->state.video_sample_counter / context->state.video_frequency < context->config.video_time;
}

int advance_record_snapshot_is_active(struct advance_record_context* context) {
	return context->state.snapshot_active_flag;
}

void advance_record_sound_update(struct advance_record_context* context, const short* sample_buffer, unsigned sample_count) {
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	sound_update(context,sample_buffer,sample_count);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

void advance_record_video_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, video_rgb_def rgb_def, osd_rgb_t* palette_map, unsigned palette_max, unsigned orientation) {
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	video_update(context, video_buffer, video_width, video_height, video_bytes_per_pixel, video_bytes_per_scanline, rgb_def, palette_map, palette_max, orientation);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

void advance_record_snapshot_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, video_rgb_def rgb_def, osd_rgb_t* palette_map, unsigned palette_max, unsigned orientation) {
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	snapshot_update(context, video_buffer, video_width, video_height, video_bytes_per_pixel, video_bytes_per_scanline, rgb_def, palette_map, palette_max, orientation);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

int advance_record_config_load(struct advance_record_context* context, struct conf_context* cfg_context) {
	strcpy(context->config.dir, conf_string_get_default(cfg_context, "dir_snap"));
	context->config.sound_time = conf_int_get_default(cfg_context, "record_sound_time");
	context->config.video_time = conf_int_get_default(cfg_context, "record_video_time");
	context->config.video_flag = conf_bool_get_default(cfg_context, "record_video");
	context->config.sound_flag = conf_bool_get_default(cfg_context, "record_sound");
	context->config.video_interlace = conf_int_get_default(cfg_context, "record_video_interleave");

	/* override */
	if (context->config.sound_time == 0) {
		context->config.sound_flag = 0;
	}
	if (context->config.video_time == 0) {
		context->config.video_flag = 0;
	}

	return 0;
}

int advance_record_init(struct advance_record_context* context, struct conf_context* cfg_context) {
	conf_int_register_limit_default(cfg_context, "record_sound_time", 1, 1000000, 15);
	conf_int_register_limit_default(cfg_context, "record_video_time", 1, 1000000, 15);
	conf_bool_register_default(cfg_context, "record_sound", 1);
	conf_bool_register_default(cfg_context, "record_video", 1);
	conf_int_register_limit_default(cfg_context, "record_video_interleave", 1, 30, 2);

#ifdef USE_SMP
	if (pthread_mutex_init(&context->state.access_mutex,NULL) != 0)
		return -1;
#endif

	return 0;
}

void advance_record_done(struct advance_record_context* context) {
	sound_cancel(context);
	video_cancel(context);

#ifdef USE_SMP
	pthread_mutex_destroy(&context->state.access_mutex);
#endif
}
