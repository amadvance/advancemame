/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#include "emu.h"
#include "log.h"
#include "endianrw.h"
#include "snstring.h"
#include "wave.h"
#include "portable.h"

#include <zlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

static void sound_cancel(struct advance_record_context* context)
{
	if (!context->state.sound_active_flag)
		return;

	context->state.sound_active_flag = 0;

	fclose(context->state.sound_f);
	remove(context->state.sound_file_buffer);
}

static adv_error sound_start(struct advance_record_context* context, const char* file, double frequency, int stereo)
{
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
	context->state.sound_stopped_flag = 0;

	sncpy(context->state.sound_file_buffer, sizeof(context->state.sound_file_buffer), file);

	context->state.sound_f = fopen(context->state.sound_file_buffer, "wb");
	if (!context->state.sound_f) {
		log_std(("ERROR: opening file %s\n", context->state.sound_file_buffer));
		return -1;
	}

	if (wave_write(context->state.sound_f, context->state.sound_stereo_flag ? 2 : 1, 16, 0, context->state.sound_frequency) != 0) {
		log_std(("ERROR: writing file %s\n", context->state.sound_file_buffer));
		fclose(context->state.sound_f);
		remove(context->state.sound_file_buffer);
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
static adv_error sound_update(struct advance_record_context* context, const short* map, unsigned mac)
{
	unsigned i;

	if (!context->state.sound_active_flag)
		return -1;

	if (context->state.sound_stopped_flag) {
		return 0;
	}

	if (context->state.sound_sample_counter / context->state.sound_frequency >= context->config.sound_time) {
		if (!context->state.video_active_flag)
			advance_global_message(&CONTEXT.global, "Sound recording full");
		context->state.sound_stopped_flag = 1;
		return 0;
	}

	for(i=0;i<mac * context->state.sound_sample_size / 2;++i) {
		unsigned char p[2];
		le_uint16_write(p, map[i]);
		if (fwrite(p, 2, 1, context->state.sound_f) != 1)
			goto err;
	}

	context->state.sound_sample_counter += mac;

	return 0;
err:
	log_std(("ERROR: writing file %s\n", context->state.sound_file_buffer));
	sound_cancel(context);
	return -1;
}

/* Save and stop the current file recording */
static adv_error sound_stop(struct advance_record_context* context, unsigned* time)
{
	if (!context->state.sound_active_flag)
		return -1;

	context->state.sound_active_flag = 0;

	if (wave_write_size(context->state.sound_f, context->state.sound_sample_size * context->state.sound_sample_counter) != 0) {
		log_std(("ERROR: writing header file %s\n", context->state.sound_file_buffer));
		fclose(context->state.sound_f);
		remove(context->state.sound_file_buffer);
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

static void png_orientation_size(unsigned* width, unsigned* height, unsigned orientation)
{
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

static adv_error png_write_chunk(FILE* f, unsigned chunk_type, uint8* chunk_data, unsigned chunk_length)
{
	uint8 v[4];
	unsigned crc;

	be_uint32_write(v, chunk_length);
	if (fwrite(v, 4, 1, f) != 1)
		return -1;

	be_uint32_write(v, chunk_type);
	if (fwrite(v, 4, 1, f) != 1)
		return -1;

	crc = crc32(0, v, 4);
	if (chunk_length > 0) {
		if (fwrite(chunk_data, chunk_length, 1, f) != 1)
			return -1;

		crc = crc32(crc, chunk_data, chunk_length);
	}

	be_uint32_write(v, crc);
	if (fwrite(v, 4, 1, f) != 1)
		return -1;

	return 0;
}

static adv_error png_write_header(FILE* f)
{
	if (fwrite(PNG_Signature, 8, 1, f) != 1)
		return -1;

	return 0;
}

static adv_error png_write_footer(FILE* f)
{
	return 0;
}

static adv_error mng_write_header(FILE* f, unsigned width, unsigned height, unsigned frequency, unsigned orientation)
{
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
	be_uint32_write(mhdr, width);
	be_uint32_write(mhdr + 4, height);
	be_uint32_write(mhdr + 8, frequency);
	be_uint32_write(mhdr + 24, simplicity);

	if (png_write_chunk(f, MNG_CN_MHDR, mhdr, 28)!=0)
		return -1;

	return 0;
}

static adv_error mng_write_footer(FILE* f)
{

	if (png_write_chunk(f, MNG_CN_MEND, 0, 0)!=0)
		return -1;

	return 0;
}

#ifdef USE_MNG_LC
static adv_error mng_write_image_frame(FILE* f, unsigned tick)
{
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
	be_uint32_write(fram+6, fi); /* Delay in tick */

	if (png_write_chunk(f, MNG_CN_FRAM, fram, 10)!=0)
		return -1;

	return 0;
}
#endif

static adv_error png_write_image_header(FILE* f, unsigned width, unsigned height, unsigned bit_depth, unsigned color_type, unsigned orientation)
{
	uint8 ihdr[13];

	png_orientation_size(&width, &height, orientation);

	be_uint32_write(ihdr, width);
	be_uint32_write(ihdr+4, height);

	ihdr[8] = bit_depth;
	ihdr[9] = color_type;
	ihdr[10] = 0; /* compression */
	ihdr[11] = 0; /* filter */
	ihdr[12] = 0; /* interlace */

	if (png_write_chunk(f, PNG_CN_IHDR, ihdr, 13)!=0)
		return -1;

	return 0;
}

static adv_error png_write_image_footer(FILE* f)
{
	if (png_write_chunk(f, PNG_CN_IEND, 0, 0)!=0)
		return -1;

	return 0;
}

static adv_error png_write_data(FILE* f, unsigned width, unsigned height, const uint8* ptr, unsigned bytes_per_pixel, unsigned bytes_per_scanline, int fast)
{
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

static adv_error png_write_image_4rgb(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, adv_color_def color_def, adv_bool fast, unsigned orientation)
{
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;
	int pixel_pitch = 4;
	int line_pitch = bytes_per_scanline;
	union adv_color_def_union def;
	int red_shift, green_shift, blue_shift;
	unsigned red_mask, green_mask, blue_mask;

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	i_size = height * (width * 3);
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	def.ordinal = color_def;
	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			adv_pixel pixel;

			pixel = cpu_uint32_read(ptr);

			p[0] = rgb_nibble_extract(pixel, red_shift, red_mask);
			p[1] = rgb_nibble_extract(pixel, green_shift, green_mask);
			p[2] = rgb_nibble_extract(pixel, blue_shift, blue_mask);

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

static adv_error png_write_image_2rgb(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, adv_color_def color_def, adv_bool fast, unsigned orientation)
{
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;
	int pixel_pitch = 2;
	int line_pitch = bytes_per_scanline;
	union adv_color_def_union def;
	int red_shift, green_shift, blue_shift;
	unsigned red_mask, green_mask, blue_mask;

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	i_size = height * (width * 3);
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	def.ordinal = color_def;
	rgb_shiftmask_get(&red_shift, &red_mask, def.nibble.red_len, def.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, def.nibble.green_len, def.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, def.nibble.blue_len, def.nibble.blue_pos);

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			adv_pixel pixel;

			pixel = cpu_uint16_read(ptr);

			p[0] = rgb_nibble_extract(pixel, red_shift, red_mask);
			p[1] = rgb_nibble_extract(pixel, green_shift, green_mask);
			p[2] = rgb_nibble_extract(pixel, blue_shift, blue_mask);

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

static adv_error png_write_image_2pal(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, adv_color_rgb* rgb, unsigned rgb_max, adv_bool fast, unsigned orientation)
{
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;
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
			adv_pixel pixel;

			pixel = cpu_uint16_read(ptr);

			p[0] = rgb[pixel].red;
			p[1] = rgb[pixel].green;
			p[2] = rgb[pixel].blue;

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

static adv_error png_write_image_2palsmall(FILE* f, const uint8* ptr, unsigned bytes_per_scanline, unsigned width, unsigned height, adv_color_rgb* rgb, unsigned rgb_max, adv_bool fast, unsigned orientation)
{
	uint8 palette[3*256];
	uint8* i_ptr;
	unsigned i_size;
	uint8* p;
	unsigned i, j;
	int pixel_pitch = 2;
	int line_pitch = bytes_per_scanline;

	if (rgb_max > 256)
		goto err;

	for (i=0;i<rgb_max;++i) {
		palette[i*3] = rgb[i].red;
		palette[i*3+1] = rgb[i].green;
		palette[i*3+2] = rgb[i].blue;
	}

	png_orientation(&ptr, &width, &height, &pixel_pitch, &line_pitch, orientation);

	if (png_write_chunk(f, PNG_CN_PLTE, palette, rgb_max*3)!=0)
		goto err;

	i_size = height * width;
	i_ptr = malloc(i_size);
	if (!i_ptr)
		goto err;

	p = i_ptr;
	for(i=0;i<height;++i) {
		for(j=0;j<width;++j) {
			adv_pixel pixel;

			pixel = cpu_uint16_read(ptr);

			p[0] = pixel;

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
static void video_cancel(struct advance_record_context* context)
{
	if (!context->state.video_active_flag)
		return;

	context->state.video_active_flag = 0;

	fclose(context->state.video_f);
	remove(context->state.video_file_buffer);
}

static void video_freq_step(unsigned* base, unsigned* step, double freq)
{
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

static adv_error video_start(struct advance_record_context* context, const char* file, double frequency, unsigned width, unsigned height, unsigned orientation)
{
	unsigned mng_frequency;
	unsigned mng_step;

	if (context->state.video_active_flag)
		return -1;

	context->state.video_frequency = frequency;
	context->state.video_sample_counter = 0;
	context->state.video_stopped_flag = 0;

	sncpy(context->state.video_file_buffer, sizeof(context->state.video_file_buffer), file);

	context->state.video_f = fopen(context->state.video_file_buffer, "wb");
	if (!context->state.video_f) {
		log_std(("ERROR: opening file %s\n", context->state.video_file_buffer));
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
		log_std(("ERROR: writing header in file %s\n", context->state.video_file_buffer));
		fclose(context->state.video_f);
		remove(context->state.video_file_buffer);
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
static adv_error video_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max, unsigned orientation)
{
	unsigned color_type;

	if (!context->state.video_active_flag)
		return -1;

	if (context->state.video_stopped_flag)
		return 0;

	if (context->state.video_sample_counter / context->state.video_frequency >= context->config.video_time) {
		advance_global_message(&CONTEXT.global, "Video recording full");
		context->state.video_stopped_flag = 1;
		return 0;
	}

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
		log_std(("ERROR: writing image frame in file %s\n", context->state.video_file_buffer));
		goto err;
	}
#endif

	if (png_write_image_header(context->state.video_f, video_width, video_height, 8, color_type, orientation) != 0) {
		log_std(("ERROR: writing image header in file %s\n", context->state.video_file_buffer));
		goto err;
	}

	if (video_bytes_per_pixel == 2 && palette_map && palette_max <= 256) {
		if (png_write_image_2palsmall(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 1, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 4 && !palette_map) {
		if (png_write_image_4rgb(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, color_def, 1, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && !palette_map) {
		if (png_write_image_2rgb(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, color_def, 1, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && palette_map) {
		if (png_write_image_2pal(context->state.video_f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 1, orientation)!=0)
			goto err_data;
	} else {
		log_std(("ERROR: unknown image format for file %s\n", context->state.video_file_buffer));
		goto err;
	}

	if (png_write_image_footer(context->state.video_f) != 0) {
		log_std(("ERROR: writing image footer in file %s\n", context->state.video_file_buffer));
		goto err;
	}

	return 0;
err_data:
	log_std(("ERROR: writing image data in file %s\n", context->state.video_file_buffer));
err:
	video_cancel(context);
	return -1;
}

/* Save and stop the current file recording */
static adv_error video_stop(struct advance_record_context* context, unsigned* time)
{
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
	log_std(("ERROR: closing file %s\n", context->state.video_file_buffer));
	fclose(context->state.video_f);
	remove(context->state.video_file_buffer);
	return -1;
}

/*************************************************************************************/
/* Snapshot */

static adv_error snapshot_start(struct advance_record_context* context, const char* file)
{
	context->state.snapshot_active_flag = 1;

	sncpy(context->state.snapshot_file_buffer, sizeof(context->state.snapshot_file_buffer), file);

	return 0;
}

static adv_error snapshot_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max, unsigned orientation)
{
	const char* file = context->state.snapshot_file_buffer;
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
		if (png_write_image_2palsmall(f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 0, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 4 && !palette_map) {
		if (png_write_image_4rgb(f, video_buffer, video_bytes_per_scanline, video_width, video_height, color_def, 0, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && !palette_map) {
		if (png_write_image_2rgb(f, video_buffer, video_bytes_per_scanline, video_width, video_height, color_def, 0, orientation)!=0)
			goto err_data;
	} else if (video_bytes_per_pixel == 2 && palette_map) {
		if (png_write_image_2pal(f, video_buffer, video_bytes_per_scanline, video_width, video_height, palette_map, palette_max, 0, orientation)!=0)
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

static void advance_record_next(struct advance_record_context* context, const mame_game* game, char* path_wav, unsigned size_wav, char* path_mng, unsigned size_mng)
{
	unsigned counter = 0;

	snprintf(path_wav, size_wav, "%s/%.8s.wav", context->config.dir_buffer, mame_game_name(game));
	snprintf(path_mng, size_mng, "%s/%.8s.mng", context->config.dir_buffer, mame_game_name(game));

	if (access(path_wav, F_OK)==0 || access(path_mng, F_OK)==0) {
		do {
			snprintf(path_wav, size_wav, "%s/%.4s%04d.wav", context->config.dir_buffer, mame_game_name(game), counter);
			snprintf(path_mng, size_mng, "%s/%.4s%04d.mng", context->config.dir_buffer, mame_game_name(game), counter);
			++counter;
		} while (access(path_wav, F_OK)==0 || access(path_mng, F_OK)==0);
	}
}

static void advance_record_terminate(struct advance_record_context* context, unsigned* sound_time, unsigned* video_time)
{
	if (context->config.sound_flag) {
		/* save only if at least 1 second of data is available */
		if (context->state.sound_sample_counter / context->state.sound_frequency > 1.0) {
			if (sound_stop(context, sound_time) != 0) {
				*sound_time = 0;
			}
		} else {
			sound_cancel(context);
			*sound_time = 0;
		}
	} else {
		*sound_time = 0;
	}

	if (context->config.video_flag) {
		/* save only if at least 1 second of data is available */
		if (context->state.video_sample_counter / context->state.video_frequency > 1.0) {
			if (video_stop(context, video_time) != 0) {
				*video_time = 0;
			}
		} else {
			video_cancel(context);
			*video_time = 0;
		}
	} else {
		*video_time = 0;
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

	advance_record_next(context, game, path_wav, FILE_MAXPATH, path_mng, FILE_MAXPATH);

	advance_global_message(&CONTEXT.global, "Start recording");

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	log_std(("osd: osd_record_start()\n"));

	if (context->config.sound_flag && sound_context->state.rate) {
		sound_start(context, path_wav, sound_context->state.rate, sound_context->state.input_mode != SOUND_MODE_MONO);
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

	unsigned sound_time;
	unsigned video_time;

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	log_std(("osd: osd_record_stop()\n"));

	advance_record_terminate(context, &sound_time, &video_time);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif

	if (sound_time != 0 || video_time != 0)
		advance_global_message(&CONTEXT.global, "Stop recording %d/%d [s]", sound_time, video_time);
}

static void advance_snapshot_next(struct advance_record_context* context, const mame_game* game, char* path_png, unsigned size)
{
	unsigned counter = 0;

	snprintf(path_png, size, "%s/%.8s.png", context->config.dir_buffer, mame_game_name(game));

	if (access(path_png, F_OK)==0) {
		do {
		snprintf(path_png, size, "%s/%.4s%04d.png", context->config.dir_buffer, mame_game_name(game), counter);
			++counter;
		} while (access(path_png, F_OK)==0);
	}
}

void osd_save_snapshot(void)
{
	struct advance_record_context* context = &CONTEXT.record;
	const mame_game* game = CONTEXT.game;
	char path_png_buffer[FILE_MAXPATH];

#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	log_std(("osd: osd_save_snapshot()\n"));

	advance_snapshot_next(context, game, path_png_buffer, sizeof(path_png_buffer));

	snapshot_start(context, path_png_buffer);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

/*************************************************************************************/
/* Advance */

adv_bool advance_record_sound_is_active(struct advance_record_context* context)
{
	return context->state.sound_active_flag
		&& !context->state.sound_stopped_flag;
}

adv_bool advance_record_video_is_active(struct advance_record_context* context)
{
	return context->state.video_active_flag
		&& !context->state.video_stopped_flag;
}

adv_bool advance_record_snapshot_is_active(struct advance_record_context* context)
{
	return context->state.snapshot_active_flag;
}

void advance_record_sound_update(struct advance_record_context* context, const short* sample_buffer, unsigned sample_count)
{
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	sound_update(context, sample_buffer, sample_count);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

void advance_record_video_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max, unsigned orientation)
{
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	video_update(context, video_buffer, video_width, video_height, video_bytes_per_pixel, video_bytes_per_scanline, color_def, palette_map, palette_max, orientation);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

void advance_record_snapshot_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max, unsigned orientation)
{
#ifdef USE_SMP
	pthread_mutex_lock(&context->state.access_mutex);
#endif

	snapshot_update(context, video_buffer, video_width, video_height, video_bytes_per_pixel, video_bytes_per_scanline, color_def, palette_map, palette_max, orientation);

#ifdef USE_SMP
	pthread_mutex_unlock(&context->state.access_mutex);
#endif
}

adv_error advance_record_config_load(struct advance_record_context* context, adv_conf* cfg_context)
{
	sncpy(context->config.dir_buffer, sizeof(context->config.dir_buffer), conf_string_get_default(cfg_context, "dir_snap"));
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

adv_error advance_record_init(struct advance_record_context* context, adv_conf* cfg_context)
{
	conf_int_register_limit_default(cfg_context, "record_sound_time", 1, 1000000, 15);
	conf_int_register_limit_default(cfg_context, "record_video_time", 1, 1000000, 15);
	conf_bool_register_default(cfg_context, "record_sound", 1);
	conf_bool_register_default(cfg_context, "record_video", 1);
	conf_int_register_limit_default(cfg_context, "record_video_interleave", 1, 30, 2);

#ifdef USE_SMP
	if (pthread_mutex_init(&context->state.access_mutex, NULL) != 0)
		return -1;
#endif

	return 0;
}

void advance_record_done(struct advance_record_context* context)
{
	sound_cancel(context);
	video_cancel(context);

#ifdef USE_SMP
	pthread_mutex_destroy(&context->state.access_mutex);
#endif
}
