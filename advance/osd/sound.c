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

#include "advance.h"
#include "conf.h"
#include "sounddrv.h"
#include "soundall.h"
#include "log.h"

#include <unistd.h>
#include <string.h>

/***************************************************************************/
/* OSD interface */

/* Attenuation in dB */
void osd_set_mastervolume(int attenuation)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	double volume;

	if (attenuation > 0) attenuation = 0;
	if (attenuation < -32) attenuation = -32;

	context->config.attenuation = attenuation;

	if (context->state.active_flag) {
		volume = 1;
		while (attenuation++ < 0)
			volume /= 1.122018454; /* = (10 ^ (1/20)) = 1dB */

		context->state.volume = volume;

		sound_volume(volume);
	}
}

int osd_get_mastervolume(void)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	return context->config.attenuation;
}

void osd_sound_enable(int enable_it)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	if (context->state.active_flag) {
		if (enable_it)
			sound_volume(context->state.volume);
		else
			sound_volume(0);
	}
}

int osd2_sound_init(unsigned* sample_rate, int stereo_flag)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	log_std(("osd: osd2_sound_init(sample_rate:%d,stereo_flag:%d)\n", *sample_rate, stereo_flag));

	assert(context->state.active_flag == 0);

#ifdef NDEBUG
	/* disable the sound with the none driver in the release build, */
	/* in the debug build use the none driver */
	if (strcmp(sound_name(),"none")==0) {
		/* returning !=0 force MAME to disable the sound */
		/* the osd2_sound_done isn't called */
		return -1;
	}
#endif

	/* 2.0 is to increase a the lower driver latency */
	/* the value is guessed with some tries, don't change it */
	/* without testing on all the drivers */
	if (sound_init(sample_rate, stereo_flag, 2.0 * context->config.latency_time) != 0) {
		return -1;
	}

	context->state.active_flag = 1;
	context->state.rate = *sample_rate;
	context->state.stereo_flag = stereo_flag;
	context->state.bytes_per_sample = stereo_flag ? 4 : 2;
	context->state.latency = context->state.rate * context->config.latency_time;
	context->state.active_flag = 1;

	sound_start(context->config.latency_time);

	osd_set_mastervolume(context->config.attenuation);

	return 0;
}

void osd2_sound_done(void)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	log_std(("osd: osd2_sound_done()\n"));

	assert(context->state.active_flag);

	sound_stop();
	sound_done();
}

/***************************************************************************/
/* Advance interface */

/**
 * Update the sound stream.
 * It must NEVER block.
 * \param sample_buffer buffer of sample in signed 16 bit format
 * \param sample_count number of samples (not multiplied by 2 if stereo)
 */
void advance_sound_update(struct advance_sound_context* context, struct advance_record_context* record_context, struct advance_video_context* video_context, const short* sample_buffer, unsigned sample_count)
{
	log_debug(("advance: sound play %d\n", sample_count));

	if (context->state.active_flag) {
		sound_play(sample_buffer,sample_count);

		if (!video_context->state.pause_flag)
			advance_record_sound_update(record_context, sample_buffer, sample_count);
	}
}

/**
 * Return the current sound latency error.
 * If this value is positive the stream needs less samples.
 * If this values is negative the stream needs more samples.
 * \return Error in samples
 */
int advance_sound_latency_diff(struct advance_sound_context* context)
{
	if (context->state.active_flag) {
		int latency = sound_buffered();

		log_debug(("advance: sound latency %d, dif %d\n", latency, latency - context->state.latency));

		return latency - context->state.latency;
	} else {
		return 0;
	}
}

int advance_sound_init(struct advance_sound_context* context, struct conf_context* cfg_context) {
	conf_bool_register_default(cfg_context, "sound_stereo", 1);
	conf_int_register_limit_default(cfg_context, "sound_volume", -32, 0, 0);
	conf_int_register_limit_default(cfg_context, "sound_samplerate", 5000, 96000, 44100);
	conf_bool_register_default(cfg_context, "sound_resamplefilter", 1);
	conf_float_register_limit_default(cfg_context, "sound_latency", 0.01, 2.0, 0.1);

	sound_reg(cfg_context, 1);
	sound_reg_driver_all(cfg_context);

	return 0;
}

void advance_sound_done(struct advance_sound_context* context) {
}

struct sound_device {
	const char* name;
	int id;
	const char* desc;
};

int advance_sound_config_load(struct advance_sound_context* context, struct conf_context* cfg_context, struct mame_option* option)
{
	context->config.stereo_flag = conf_bool_get_default(cfg_context, "sound_stereo");
	context->config.attenuation = conf_int_get_default(cfg_context, "sound_volume");
	context->config.latency_time = conf_float_get_default(cfg_context, "sound_latency");
	option->samplerate = conf_int_get_default(cfg_context, "sound_samplerate");
	option->filter_flag = conf_bool_get_default(cfg_context, "sound_resamplefilter");

	if (sound_load(cfg_context)!=0) {
		return -1;
	}

	return 0;
}

