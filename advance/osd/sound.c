/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include <math.h>

/**
 * Base for the sample gain adjustment.
 */
#define ADJUST_MULT_BASE 4096

/**
 * Expected volume factor.
 * Value guessed with some tries.
 */
#define ADJUST_EXPECTED_FACTOR 0.25 /* -12 dB */

/***************************************************************************/
/* Advance interface */

static void sound_volume_recompute(struct advance_sound_context* context)
{
	double volume = context->state.volume;

	/* if mute, zero the volume */
	if (context->state.mute_flag)
		volume = 0;

	/* if disabled, zero the volume */
	if (context->state.disabled_flag)
		volume = 0;

	/* if already changing the volume, don't use the mixer */
	if (context->config.adjust_flag) {
		double n;

		if ((soundb_flags() & SOUND_DRIVER_FLAGS_VOLUME_SAMPLE) != 0) {
			/* adjust the volume internally */
			context->state.adjust_volume_factor = volume;
			soundb_volume(1.0);
		} else {
			context->state.adjust_volume_factor = 1.0;
			soundb_volume(volume);
		}

		n = ADJUST_MULT_BASE * context->state.adjust_power_factor * context->state.adjust_volume_factor;

		/* prevent multiplication overflow */
		if (n > 65535)
			n = 65535;
		if (n < 0)
			n = 0;

		context->state.adjust_mult = n;
	} else {
		soundb_volume(volume);
	}
}

/* Adjust the sound volume computing the maximum normalized power */
static void sound_adjust(struct advance_sound_context* context, unsigned channel, const short* input_sample, short* output_sample, unsigned sample_count, adv_bool compute_power)
{
	unsigned i;
	int mult;
	unsigned count;

	count = channel * sample_count;

	if (compute_power) {
		long long power;

		/* use a local variable for faster access */
		power = context->state.adjust_power_accumulator;

		i = 0;
		while (i < count) {
			unsigned run_sample = count - i;
			unsigned run_counter = context->state.adjust_power_counter_limit - context->state.adjust_power_counter;
			unsigned run = run_sample < run_counter ? run_sample : run_counter;

			context->state.adjust_power_counter += run;

			while (run) {
				int v = input_sample[i];

				power += v*v;

				++i;
				--run;
			}

			if (context->state.adjust_power_counter == context->state.adjust_power_counter_limit) {
				/* mean power */
				power /= context->state.adjust_power_counter_limit;

				/* check for the maximum power */
				if (power != 0 && power > context->state.adjust_power) {
					double n;

					context->state.adjust_power = power;

					/* normalized power */
					n = sqrt(power) / 32768;

					n = ADJUST_EXPECTED_FACTOR / n;

					/* limit the maximum gain */
					if (n > 64)
						n = 64;

					log_std(("osd:sound: normalize factor %g, time %g\n", (double)n, advance_timer()));

					context->state.adjust_power_factor = n;

					sound_volume_recompute(context);
				}

				/* restart the computation */
				power = 0;
				context->state.adjust_power_counter = 0;
			}
		}

		context->state.adjust_power_accumulator = power;
	} else {
		/* restart the computation */
		context->state.adjust_power_accumulator = 0;
		context->state.adjust_power_counter = 0;
	}

	mult = context->state.adjust_mult;

	/* now convert */
	for(i=0;i<count;++i) {
		int v = input_sample[i];

		v = v * mult / ADJUST_MULT_BASE;

		if (v > 32767)
			v = 32767;
		if (v < -32768)
			v = -32768;

		output_sample[i] = v;
	}
}

static void sound_scale(struct advance_sound_context* context, unsigned channel, const short* input_sample, short* output_sample, unsigned sample_count, unsigned sample_recount)
{
	adv_slice slice;

	slice_set(&slice, sample_count, sample_recount);

	if (channel == 1) {
		if (sample_count < sample_recount) {
			/* expansion */
			int count = slice.count;
			int error = slice.error;
			while (count) {
				unsigned run = slice.whole;
				if ((error += slice.up) > 0) {
					++run;
					error -= slice.down;
				}

				while (run) {
					output_sample[0] = input_sample[0];
					output_sample += 1;
					--run;
				}
				input_sample += 1;

				--count;
			}
		} else {
			/* reduction */
			int count = slice.count;
			int error = slice.error;
			while (count) {
				unsigned run = slice.whole;
				if ((error += slice.up) > 0) {
					++run;
					error -= slice.down;
				}

				output_sample[0] = input_sample[0];
				output_sample += 1;
				input_sample += run;

				--count;
			}
		}
	} else if (channel == 2) {
		if (sample_count < sample_recount) {
			/* expansion */
			int count = slice.count;
			int error = slice.error;
			while (count) {
				unsigned run = slice.whole;
				if ((error += slice.up) > 0) {
					++run;
					error -= slice.down;
				}

				while (run) {
					output_sample[0] = input_sample[0];
					output_sample[1] = input_sample[1];
					output_sample += 2;
					--run;
				}
				input_sample += 2;

				--count;
			}
		} else {
			/* reduction */
			int count = slice.count;
			int error = slice.error;
			while (count) {
				unsigned run = slice.whole;
				if ((error += slice.up) > 0) {
					++run;
					error -= slice.down;
				}

				output_sample[0] = input_sample[0];
				output_sample[1] = input_sample[1];
				output_sample += 2;
				input_sample += 2*run;

				--count;
			}
		}
	}
}

static void sound_play_recount(struct advance_sound_context* context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount)
{
	unsigned output_channel = context->state.output_mode != SOUND_MODE_MONO ? 2 : 1;

	if (sample_count != sample_recount) {
		short* sample_re = (short*)malloc(sample_recount * context->state.output_bytes_per_sample);

		sound_scale(context, output_channel, sample_buffer, sample_re, sample_count, sample_recount);

		soundb_play(sample_re, sample_recount);

		free(sample_re);
	} else {
		soundb_play(sample_buffer, sample_count);
	}
}

static void sound_play_adjust_own(struct advance_sound_context* context, short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool compute_power)
{
	unsigned output_channel = context->state.output_mode != SOUND_MODE_MONO ? 2 : 1;

	if (context->config.adjust_flag) {
		sound_adjust(context, output_channel, sample_buffer, sample_buffer, sample_count, compute_power);

		sound_play_recount(context, sample_buffer, sample_count, sample_recount);
	} else {
		sound_play_recount(context, sample_buffer, sample_count, sample_recount);
	}
}

static void sound_play_adjust_const(struct advance_sound_context* context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool compute_power)
{
	unsigned output_channel = context->state.output_mode != SOUND_MODE_MONO ? 2 : 1;

	if (context->config.adjust_flag) {
		short* sample_mix = (short*)malloc(sample_count * context->state.output_bytes_per_sample);

		sound_adjust(context, output_channel, sample_buffer, sample_mix, sample_count, compute_power);

		sound_play_recount(context, sample_mix, sample_count, sample_recount);

		free(sample_mix);
	} else {
		sound_play_recount(context, sample_buffer, sample_count, sample_recount);
	}
}

/**
 * Update the sound stream.
 * It must NEVER block.
 * \param sample_buffer Buffer of sample in signed 16 bit format.
 * \param sample_count Number of samples (not multiplied by 2 if stereo).
 * \param sample_diff Correction of the number of sample to output.
 */
void advance_sound_update(struct advance_sound_context* context, struct advance_record_context* record_context, struct advance_video_context* video_context, struct advance_safequit_context* safequit_context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool compute_power)
{
	adv_bool mute;

	if (!context->state.active_flag)
		return;

	log_debug(("advance: sound play count:%d, diff:%d\n", sample_count, sample_recount));

	/* compute the new mute state */
	mute = 0;
	if (context->config.mutedemo_flag) {
		unsigned mask = advance_safequit_event_mask(safequit_context);
		if ((mask & (1 << safequit_event_demomode)) != 0
			&& (mask & (1 << safequit_event_zerocoin)) != 0) {
			mute = 1;
		}
	}

	if (context->config.mutestartup_flag) {
		if (video_context->state.fastest_flag) {
			mute = 1;
		}
	}

	/* update the mute state */
	if (context->state.mute_flag != mute) {
		context->state.mute_flag = mute;
		sound_volume_recompute(context);
	}

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
				if (sample_mono[0] == -32768) {
					sample_surround[0] = -32768;
					sample_surround[1] = 32767;
				} else {
					sample_surround[0] = sample_mono[0];
					sample_surround[1] = -sample_mono[0];
				}
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
			sound_play_adjust_own(context, sample_mix, sample_count, sample_recount, compute_power);
		free(sample_mix);
	} else {
		sound_play_adjust_const(context, sample_buffer, sample_count, sample_recount, compute_power);
	}

	if (!video_context->state.pause_flag)
		advance_record_sound_update(record_context, sample_buffer, sample_count);
}

int advance_sound_latency(struct advance_sound_context* context, double extra_latency)
{
	return context->state.latency_min + context->state.rate * extra_latency;
}

/**
 * Return the current sound latency error.
 * If this value is positive the stream needs less samples.
 * If this values is negative the stream needs more samples.
 * \param extra_latency Extra latency time (in seconds) required.
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
	conf_int_register_limit_default(cfg_context, "sound_volume", -40, 0, 0);
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
	context->config.mutedemo_flag = conf_bool_get_default(cfg_context, "misc_mutedemo");
	context->config.mutestartup_flag = 1;
	option->samplerate = conf_int_get_default(cfg_context, "sound_samplerate");
	option->filter_flag = conf_bool_get_default(cfg_context, "sound_resamplefilter");

	if (soundb_load(cfg_context)!=0) {
		return -1;
	}

	return 0;
}

/***************************************************************************/
/* OSD interface */

/**
 * Set the attenuation in dB.
 */
void osd_set_mastervolume(int attenuation)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	double volume;

	if (attenuation > 0) attenuation = 0;
	if (attenuation < -40) attenuation = -40;

	context->config.attenuation = attenuation;

	if (context->state.active_flag) {
		if (attenuation <= -40) {
			volume = 0;
		} else {
			volume = 1;
			while (attenuation++ < 0)
				volume /= 1.122018454; /* = (10 ^ (1/20)) = 1dB */
		}

		context->state.volume = volume;
		sound_volume_recompute(context);
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
		context->state.disabled_flag = !enable_it;
		sound_volume_recompute(context);
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

	/* size the buffer to allow a double latency, the specified latency */
	/* is the target latency, a latency bigger than the target must be allowed */
	low_buffer_time = context->config.latency_time * 2;

	/* allow always a big maximum latency. Note that this is the upper */
	/* limit latency, not the effective latency. It's used in case of "turbo" */
	/* mode, on which there are a lot of generated samples */
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
	context->state.adjust_power_factor = 1;
	context->state.adjust_volume_factor = 1;
	context->state.adjust_power_accumulator = 0;
	context->state.adjust_power_counter = 0;
	context->state.adjust_power_counter_limit = context->state.rate / 2; /* recompute every 1/2 seconds */
	if (context->state.input_mode != SOUND_MODE_MONO)
		context->state.adjust_power_counter_limit *= 2; /* stereo */
	context->state.adjust_power = 0;
	context->state.active_flag = 1;
	context->state.mute_flag = 0;
	context->state.disabled_flag = 0;

	soundb_start(context->config.latency_time);

	/* set the internal volume, it may be faked changing the samples */
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

