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

#include "emu.h"
#include "conf.h"
#include "sounddrv.h"
#include "soundall.h"
#include "log.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ADJUST_MULT_BASE 4096

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
		if (attenuation <= -32) {
			volume = 0;
		} else {
			volume = 1;
			while (attenuation++ < 0)
				volume /= 1.122018454; /* = (10 ^ (1/20)) = 1dB */
		}

		context->state.volume = volume;

		soundb_volume(volume);
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
			soundb_volume(context->state.volume);
		else
			soundb_volume(0);
	}
}

int osd2_sound_init(unsigned* sample_rate, int stereo_flag)
{
	struct advance_sound_context* context = &CONTEXT.sound;
	double low_buffer_time;

	log_std(("osd: osd2_sound_init(sample_rate:%d, stereo_flag:%d)\n", *sample_rate, stereo_flag));

	assert(context->state.active_flag == 0);

	/* note that if this function returns !=0, MAME disables the sound */
	/* and doesn't call sd2_sound_done function */

	if (stereo_flag)
		context->state.input_mode = SOUND_MODE_STEREO;
	else
		context->state.input_mode = SOUND_MODE_MONO;
	if (context->config.mode == SOUND_MODE_AUTO)
		context->state.output_mode = context->state.input_mode;
	else
		context->state.output_mode = context->config.mode;

	low_buffer_time = context->config.latency_time;
	/* allow always a big maximum latency. note that this is the upper */
	/* limit latency, not the effective latency. */
	if (low_buffer_time < 0.3)
		low_buffer_time = 0.3;

	if (soundb_init(sample_rate, context->state.output_mode != SOUND_MODE_MONO, low_buffer_time) != 0) {
		return -1;
	}

	context->state.active_flag = 1;
	context->state.rate = *sample_rate;
	context->state.input_bytes_per_sample = context->state.input_mode != SOUND_MODE_MONO ? 4 : 2;
	context->state.output_bytes_per_sample = context->state.output_mode != SOUND_MODE_MONO ? 4 : 2;
	context->state.latency_min = context->state.rate * context->config.latency_time;
	context->state.latency_max = context->state.rate * low_buffer_time;
	context->state.adjust_mult = ADJUST_MULT_BASE;
	context->state.adjust_limit = 0;
	context->state.active_flag = 1;

	soundb_start(context->config.latency_time);

	osd_set_mastervolume(context->config.attenuation);

	return 0;
}

void osd2_sound_done(void)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	log_std(("osd: osd2_sound_done()\n"));

	assert(context->state.active_flag);

	soundb_stop();
	soundb_done();
}

/***************************************************************************/
/* Advance interface */

static void sound_adjust(struct advance_sound_context* context, unsigned channel, const short* input_sample, short* output_sample, unsigned sample_count)
{
	unsigned i;
	int limit;
	int mult;
	int new_mult;
	unsigned count;

	limit = context->state.adjust_limit;
	mult = context->state.adjust_mult;
	count = channel * sample_count;

	for(i=0;i<count;++i) {
		int v = input_sample[i];

		if (v > limit)
			limit = v;
		if (v < -limit)
			limit = -v;

		v = v * mult / ADJUST_MULT_BASE;

		if (v > 32767)
			v = 32767;
		if (v < -32768)
			v = -32768;

		output_sample[i] = v;
	}

	context->state.adjust_limit = limit;

	if (limit > 32767 / 50)
		new_mult = ADJUST_MULT_BASE * 32767 / limit;
	else
		new_mult = ADJUST_MULT_BASE; /* if the increase is too big reject it */

	/* prevent overflow */
	if (new_mult > 65536)
		new_mult = 65536;

	/* only increase the volume */
	if (new_mult < ADJUST_MULT_BASE)
		new_mult = ADJUST_MULT_BASE;

	if (context->state.adjust_mult != new_mult)
		log_std(("osd:sound: adjust factor %g, limit %d\n", (double)new_mult / ADJUST_MULT_BASE, limit));

	context->state.adjust_mult = new_mult;
}

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
		if (context->state.input_mode != context->state.output_mode) {
			short* sample_mix = (short*)malloc(sample_count * context->state.output_bytes_per_sample);

			if (context->state.input_mode == SOUND_MODE_MONO && context->state.output_mode == SOUND_MODE_STEREO) {
				unsigned i;
				const short* sample_mono = sample_buffer;
				short* sample_stereo = sample_mix;
				for(i=0;i<sample_count;++i) {
					sample_stereo[0] = sample_mono[0];
					sample_stereo[1] = sample_mono[0];
					sample_mono += 1;
					sample_stereo += 2;
				}
			} else if (context->state.input_mode == SOUND_MODE_STEREO && context->state.output_mode == SOUND_MODE_MONO) {
				unsigned i;
				const short* sample_stereo = sample_buffer;
				short* sample_mono = sample_mix;
				for(i=0;i<sample_count;++i) {
					sample_mono[0] = (sample_stereo[0] + sample_stereo[1]) / 2;
					sample_mono += 1;
					sample_stereo += 2;
				}
			} else if (context->state.input_mode == SOUND_MODE_MONO && context->state.output_mode == SOUND_MODE_SURROUND) {
				unsigned i;
				const short* sample_mono = sample_buffer;
				short* sample_surround = sample_mix;
				for(i=0;i<sample_count;++i) {
					sample_surround[0] = sample_mono[0];
					sample_surround[1] = -sample_mono[0];
					sample_mono += 1;
					sample_surround += 2;
				}
			} else if (context->state.input_mode == SOUND_MODE_STEREO && context->state.output_mode == SOUND_MODE_SURROUND) {
				unsigned i;
				const short* sample_stereo = sample_buffer;
				short* sample_surround = sample_mix;
				for(i=0;i<sample_count;++i) {
					sample_surround[0] = (3*sample_stereo[0] - sample_stereo[1]) / 4;
					sample_surround[1] = (3*sample_stereo[1] - sample_stereo[0]) / 4;
					sample_surround += 2;
					sample_stereo += 2;
				}
			} else {
				memset(sample_mix, 0, sample_count * context->state.output_bytes_per_sample);
			}

			if (context->config.adjust_flag) {
				unsigned output_channel = context->state.output_mode != SOUND_MODE_MONO ? 2 : 1;
				sound_adjust(context, output_channel, sample_mix, sample_mix, sample_count);
			}

			soundb_play(sample_mix, sample_count);

			free(sample_mix);

		} else {
			if (context->config.adjust_flag) {
				unsigned output_channel = context->state.output_mode != SOUND_MODE_MONO ? 2 : 1;
				short* sample_mix = (short*)malloc(sample_count * context->state.output_bytes_per_sample);

				sound_adjust(context, output_channel, sample_buffer, sample_mix, sample_count);

				soundb_play(sample_mix, sample_count);

				free(sample_mix);
			} else {
				soundb_play(sample_buffer, sample_count);
			}
		}

		if (!video_context->state.pause_flag)
			advance_record_sound_update(record_context, sample_buffer, sample_count);
	}
}

/**
 * Return the current sound latency error.
 * If this value is positive the stream needs less samples.
 * If this values is negative the stream needs more samples.
 * \param extra_latency Extra latency time for the skipped samples.
 * \return Error in samples
 */
int advance_sound_latency_diff(struct advance_sound_context* context, double extra_latency)
{
	if (context->state.active_flag) {
		int buffered = soundb_buffered();
		int expected;

		expected = context->state.latency_min + context->state.rate * extra_latency;
		if (expected < context->state.latency_min)
			expected = context->state.latency_min;
		if (expected > context->state.latency_max)
			expected = context->state.latency_max;

		log_debug(("advance: sound buffered %d, expected %d, diff %d, min %d, max %d\n", buffered, expected, buffered - expected, context->state.latency_min, context->state.latency_max));

		return buffered - expected;
	} else {
		return 0;
	}
}

static adv_conf_enum_int OPTION_CHANNELS[] = {
{ "auto", SOUND_MODE_AUTO },
{ "mono", SOUND_MODE_MONO },
{ "stereo", SOUND_MODE_STEREO },
{ "surround", SOUND_MODE_SURROUND }
};

adv_error advance_sound_init(struct advance_sound_context* context, adv_conf* cfg_context)
{
	conf_int_register_enum_default(cfg_context, "sound_mode", conf_enum(OPTION_CHANNELS), SOUND_MODE_AUTO);
	conf_int_register_limit_default(cfg_context, "sound_volume", -32, 0, 0);
	conf_int_register_limit_default(cfg_context, "sound_samplerate", 5000, 96000, 44100);
	conf_bool_register_default(cfg_context, "sound_normalize", 1);
	conf_bool_register_default(cfg_context, "sound_resamplefilter", 1);
	conf_float_register_limit_default(cfg_context, "sound_latency", 0.0, 2.0, 0.05);

	soundb_reg(cfg_context, 1);
	soundb_reg_driver_all(cfg_context);

	return 0;
}

void advance_sound_done(struct advance_sound_context* context)
{
}

struct sound_device {
	const char* name;
	int id;
	const char* desc;
};

adv_error advance_sound_config_load(struct advance_sound_context* context, adv_conf* cfg_context, struct mame_option* option)
{
	context->config.mode = conf_int_get_default(cfg_context, "sound_mode");
	context->config.attenuation = conf_int_get_default(cfg_context, "sound_volume");
	context->config.latency_time = conf_float_get_default(cfg_context, "sound_latency");
	context->config.adjust_flag = conf_bool_get_default(cfg_context, "sound_normalize");
	option->samplerate = conf_int_get_default(cfg_context, "sound_samplerate");
	option->filter_flag = conf_bool_get_default(cfg_context, "sound_resamplefilter");

	if (soundb_load(cfg_context)!=0) {
		return -1;
	}

	return 0;
}

