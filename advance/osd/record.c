/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#include "portable.h"

#include "emu.h"

#include "advance.h"

#include <zlib.h>

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
/* PNG/MNG orientation */

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

/*************************************************************************************/
/* Video */

/* Cancel a video recording */
static void video_cancel(struct advance_record_context* context)
{
	if (!context->state.video_active_flag)
		return;

	context->state.video_active_flag = 0;

	fzclose(context->state.video_f);
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
	unsigned pix_width;
	unsigned pix_height;

	if (context->state.video_active_flag)
		return -1;

	context->state.video_frequency = frequency;
	context->state.video_sample_counter = 0;
	context->state.video_stopped_flag = 0;

	sncpy(context->state.video_file_buffer, sizeof(context->state.video_file_buffer), file);

	context->state.video_f = fzopen(context->state.video_file_buffer, "wb");
	if (!context->state.video_f) {
		log_std(("ERROR: opening file %s\n", context->state.video_file_buffer));
		return -1;
	}

	if (adv_mng_write_signature(context->state.video_f, 0) != 0) {
		log_std(("ERROR: writing signature in file %s\n", context->state.video_file_buffer));
		fzclose(context->state.video_f);
		remove(context->state.video_file_buffer);
		return -1;
	}

	video_freq_step(&mng_frequency, &mng_step, frequency / context->config.video_interlace);

	context->state.video_freq_base = mng_frequency;
	context->state.video_freq_step = mng_step;

	pix_width = width;
	pix_height = height;

	png_orientation_size(&pix_width, &pix_height, orientation);

	if (adv_mng_write_mhdr(pix_width, pix_height, context->state.video_freq_base, 1, context->state.video_f, 0) != 0) {
		log_std(("ERROR: writing header in file %s\n", context->state.video_file_buffer));
		fzclose(context->state.video_f);
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
	const uint8* pix_ptr;
	unsigned pix_width;
	unsigned pix_height;
	int pix_pixel_pitch;
	int pix_scanline_pitch;

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

	pix_ptr = video_buffer;
	pix_width = video_width;
	pix_height = video_height;
	pix_pixel_pitch = video_bytes_per_pixel;
	pix_scanline_pitch = video_bytes_per_scanline;

	png_orientation(&pix_ptr, &pix_width, &pix_height, &pix_pixel_pitch, &pix_scanline_pitch, orientation);

	if (adv_mng_write_fram(context->state.video_freq_step, context->state.video_f, 0) != 0) {
		log_std(("ERROR: writing image frame in file %s\n", context->state.video_file_buffer));
		goto err;
	}

	if (adv_png_write_raw_def(pix_width, pix_height, color_def, pix_ptr, pix_pixel_pitch, pix_scanline_pitch, palette_map, palette_max, 1, context->state.video_f, 0)!=0) {
		log_std(("ERROR: writing image data in file %s\n", context->state.video_file_buffer));
		goto err;
	}

	return 0;

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

	if (adv_mng_write_mend(context->state.video_f, 0)!=0) {
		goto err;
	}

	fzclose(context->state.video_f);

	*time = context->state.video_sample_counter / context->state.video_frequency;

	return 0;

err:
	log_std(("ERROR: closing file %s\n", context->state.video_file_buffer));
	fzclose(context->state.video_f);
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

adv_error advance_record_png_write(adv_fz* f, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max, unsigned orientation)
{
	const uint8* pix_ptr;
	unsigned pix_width;
	unsigned pix_height;
	int pix_pixel_pitch;
	int pix_scanline_pitch;

	if (adv_png_write_signature(f, 0) != 0) {
		return -1;
	}

	pix_ptr = video_buffer;
	pix_width = video_width;
	pix_height = video_height;
	pix_pixel_pitch = video_bytes_per_pixel;
	pix_scanline_pitch = video_bytes_per_scanline;

	png_orientation(&pix_ptr, &pix_width, &pix_height, &pix_pixel_pitch, &pix_scanline_pitch, orientation);

	if (adv_png_write_raw_def(pix_width, pix_height, color_def, pix_ptr, pix_pixel_pitch, pix_scanline_pitch, palette_map, palette_max, 1, f, 0) != 0) {
		return -1;
	}

	return 0;
}

static adv_error snapshot_update(struct advance_record_context* context, const void* video_buffer, unsigned video_width, unsigned video_height, unsigned video_bytes_per_pixel, unsigned video_bytes_per_scanline, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max, unsigned orientation)
{
	const char* file = context->state.snapshot_file_buffer;
	adv_fz* f;

	if (!context->state.snapshot_active_flag) {
		return -1;
	}

	context->state.snapshot_active_flag = 0;

	f = fzopen(file, "wb");
	if (!f) {
		log_std(("ERROR: opening file %s\n", file));
		return -1;
	}

	if (advance_record_png_write(f, video_buffer, video_width, video_height, video_bytes_per_pixel, video_bytes_per_scanline, color_def, palette_map, palette_max, orientation) != 0) {
		log_std(("ERROR: writing file %s\n", file));
		fzclose(f);
		remove(file);
		return -1;
	}

	fzclose(f);

	return 0;
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
