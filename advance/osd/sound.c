/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

static struct game_adjust_struct {
	const char* name;
	int gain;
} GAME_ADJUST[] = {
#include "adjust.h"
{ 0 }
};

/**
 * Base for the sample gain adjustment.
 */
#define SAMPLE_MULT_BASE 4096

/**
 * Expected volume factor.
 * Value guessed with some tries.
 */
#define ADJUST_EXPECTED_FACTOR 0.25 /* -12 dB */

/***************************************************************************/
/* Advance interface */

/**
 * Maximum volume requested at the hardware mixer.
 */
#define SOUND_MIXER_MAX 0.9

static void sound_volume_update(struct advance_sound_context* context)
{
	double volume;
	double mult;

	if (context->config.attenuation > 0)
		context->config.attenuation = 0;

	if (context->config.attenuation < -40)
		context->config.attenuation = -40;

	/* initial volume */
	if (context->config.attenuation <= -40) {
		volume = 0;
	} else {
		volume = pow(10, (double)context->config.attenuation / 20);
	}

	/* if mute, zero the volume */
	if (context->state.mute_flag)
		volume = 0;

	/* if disabled, zero the volume */
	if (context->state.disabled_flag)
		volume = 0;

	/* add the current power normalization factor */
	if (!context->state.adjust_power_waitfirstsound_flag)
		volume *= context->state.adjust_power_factor;

	if ((soundb_flags() & SOUND_DRIVER_FLAGS_VOLUME_SAMPLE) != 0
		|| volume > SOUND_MIXER_MAX) {
		/* adjust the volume scaling the sample */
		soundb_volume(SOUND_MIXER_MAX);
		volume /= SOUND_MIXER_MAX;
	} else {
		/* adjust the volume with the hardware mixer */
		soundb_volume(volume);
		volume = 1.0;
	}

	mult = SAMPLE_MULT_BASE * volume;

	/* prevent multiplication overflow */
	if (mult > 65535)
		mult = 65535;
	if (mult < 0)
		mult = 0;

	context->state.sample_mult = mult;
}

/* Adjust the sound volume computing the maximum normalized power */
static void sound_normalize(struct advance_sound_context* context, unsigned channel, const short* sample, unsigned sample_count, adv_bool compute_power)
{
	unsigned i;

	if (compute_power) {
		long long power;
		unsigned count;

		/* use a local variable for faster access */
		power = context->state.adjust_power_accumulator;

		i = 0;
		count = sample_count * channel;

		while (i < count) {
			unsigned run_sample = count - i;
			unsigned run_counter = context->state.adjust_power_counter_limit - context->state.adjust_power_counter;
			unsigned run = run_sample < run_counter ? run_sample : run_counter;

			context->state.adjust_power_counter += run;

			while (run) {
				int y = sample[i];

				power += y*y;

				++i;
				--run;
			}

			if (context->state.adjust_power_counter == context->state.adjust_power_counter_limit) {
				/* mean power */
				power /= context->state.adjust_power_counter_limit;

				/* check for the maximum power */
				if (power != 0 && power > context->state.adjust_power_maximum) {
					double n;

					/* now the process is started */
					context->state.adjust_power_waitfirstsound_flag = 0;

					context->state.adjust_power_maximum = power;

					/* normalized power */
					n = sqrt((double)power) / 32768.0;

					n = ADJUST_EXPECTED_FACTOR / n;

					/* limit the maximum gain */
					if (n > 100.0)
						n = 100.0;

					log_std(("osd:sound: normalize factor %g, time %g\n", (double)n, advance_timer()));

					if (n < context->state.adjust_power_factor) {
						char buffer[64];
						int adjust_db;

						context->state.adjust_power_factor = n;

						adjust_db = 20 * log10(context->state.adjust_power_factor);
						if (adjust_db < 0)
							adjust_db = 0;
						if (adjust_db > 40)
							adjust_db = 40;

						context->config.adjust = adjust_db;

						log_std(("osd:sound: normalize factor save adjust dB %d\n", adjust_db));

						snprintf(buffer, sizeof(buffer), "%d", adjust_db);

						conf_set(CONTEXT.cfg, mame_section_name(CONTEXT.game, CONTEXT.cfg), "sound_adjust", buffer);
					}

					sound_volume_update(context);
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
}

/* Adjust the sound volume */
static void sound_adjust(struct advance_sound_context* context, unsigned channel, const short* input_sample, short* output_sample, unsigned sample_count)
{
	unsigned i;
	int mult;
	unsigned count;

	count = channel * sample_count;

	mult = context->state.sample_mult;

	assert(mult != SAMPLE_MULT_BASE);

	for(i=0;i<count;++i) {
		int v = input_sample[i];

		v = v * mult / SAMPLE_MULT_BASE;

		if (v > 32767)
			v = 32767;
		if (v < -32768)
			v = -32768;

		output_sample[i] = v;
	}
}

static void sound_equalizer(struct advance_sound_context* context, unsigned channel, const short* input_sample, short* output_sample, unsigned sample_count)
{
	unsigned i, j;
	int mult;

	for(j=0;j<channel;++j) {
		unsigned off = j;
		for(i=0;i<sample_count;++i) {
			double v, vl, vm, vh;
			int vi;

			v = input_sample[off];

			adv_filter_insert(&context->state.equalizer_low, &context->state.equalizer_low_state[j], v);
			adv_filter_insert(&context->state.equalizer_mid, &context->state.equalizer_mid_state[j], v);
			adv_filter_insert(&context->state.equalizer_high, &context->state.equalizer_high_state[j], v);

			vl = adv_filter_extract(&context->state.equalizer_low, &context->state.equalizer_low_state[j]);
			vm = adv_filter_extract(&context->state.equalizer_mid, &context->state.equalizer_mid_state[j]);
			vh = adv_filter_extract(&context->state.equalizer_high, &context->state.equalizer_high_state[j]);

			v = vl * context->state.equalizer_low_factor
				+ vm * context->state.equalizer_mid_factor
				+ vh * context->state.equalizer_high_factor;

			vi = v;

			if (vi > 32767)
				vi = 32767;
			if (vi < -32768)
				vi = -32768;

			output_sample[off] = vi;
			off += channel;
		}
	}
}

/* Resample */
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

static void sound_play_effect(struct advance_sound_context* context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount)
{
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
				if (sample_mono[0] == -32768) { /* prevent overflow */
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

		sound_play_recount(context, sample_mix, sample_count, sample_recount);

		free(sample_mix);
	} else {
		sound_play_recount(context, sample_buffer, sample_count, sample_recount);
	}
}

static void sound_play_equalizer(struct advance_sound_context* context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount)
{
	unsigned input_channel = context->state.input_mode != SOUND_MODE_MONO ? 2 : 1;

	if (context->state.equalizer_flag) {
		short* sample_mix = (short*)malloc(sample_count * context->state.input_bytes_per_sample);

		sound_equalizer(context, input_channel, sample_buffer, sample_mix, sample_count);

		sound_play_effect(context, sample_mix, sample_count, sample_recount);

		free(sample_mix);
	} else {
		sound_play_effect(context, sample_buffer, sample_count, sample_recount);
	}
}

static void sound_play_adjust(struct advance_sound_context* context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool compute_power)
{
	unsigned input_channel = context->state.input_mode != SOUND_MODE_MONO ? 2 : 1;

	if (context->config.normalize_flag)
		sound_normalize(context, input_channel, sample_buffer, sample_count, compute_power);

	if (context->state.sample_mult != SAMPLE_MULT_BASE) {
		short* sample_mix = (short*)malloc(sample_count * context->state.input_bytes_per_sample);

		sound_adjust(context, input_channel, sample_buffer, sample_mix, sample_count);

		sound_play_equalizer(context, sample_mix, sample_count, sample_recount);

		free(sample_mix);
	} else {
		sound_play_equalizer(context, sample_buffer, sample_count, sample_recount);
	}
}

/**
 * Update the sound stream.
 * It must NEVER block.
 * \param sample_buffer Buffer of sample in signed 16 bit format.
 * \param sample_count Number of samples (not multiplied by 2 if stereo).
 * \param sample_diff Correction of the number of sample to output.
 */
void advance_sound_frame(struct advance_sound_context* context, struct advance_record_context* record_context, struct advance_video_context* video_context, struct advance_safequit_context* safequit_context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool compute_power)
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
		sound_volume_update(context);
	}

	sound_play_adjust(context, sample_buffer, sample_count, sample_recount, compute_power);

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
	conf_int_register_limit_default(cfg_context, "sound_volume", -40, 0, -3);
	conf_int_register_limit_default(cfg_context, "sound_equalizer_lowvolume", -20, 20, 0);
	conf_int_register_limit_default(cfg_context, "sound_equalizer_midvolume", -20, 20, 0);
	conf_int_register_limit_default(cfg_context, "sound_equalizer_highvolume", -20, 20, 0);
	conf_string_register_default(cfg_context, "sound_adjust", "auto");
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
	const char* s;
	int i;

	context->config.mode = conf_int_get_default(cfg_context, "sound_mode");
	context->config.attenuation = conf_int_get_default(cfg_context, "sound_volume");

	s = conf_string_get_default(cfg_context, "sound_adjust");
	if (strcmp(s, "none") == 0) {
		i = -1;
	} else if (strcmp(s, "auto") == 0) {
		for(i=0;GAME_ADJUST[i].name != 0;++i)
			if (mame_is_game_relative(GAME_ADJUST[i].name, option->game))
				break;
		if (GAME_ADJUST[i].name != 0)
			i = GAME_ADJUST[i].gain;
		else
			i = -1;
	} else {
		char* e;
		i = strtol(s, &e, 10);
		if (*e != 0 || i < 0 || i > 40) {
			target_err("Invalid argument '%s' for option 'sound_adjust'.\n", s);
			return -1;
		}
	}
	context->config.adjust = i;
	log_std(("emu:sound: normalize factor load adjust dB %d\n", context->config.adjust));

	context->config.latency_time = conf_float_get_default(cfg_context, "sound_latency");
	context->config.normalize_flag = conf_bool_get_default(cfg_context, "sound_normalize");
	context->config.mutedemo_flag = conf_bool_get_default(cfg_context, "misc_mutedemo");
	context->config.mutestartup_flag = 1;
	option->samplerate = conf_int_get_default(cfg_context, "sound_samplerate");
	option->filter_flag = conf_bool_get_default(cfg_context, "sound_resamplefilter");

	context->config.equalizer_low = conf_int_get_default(cfg_context, "sound_equalizer_lowvolume");
	context->config.equalizer_mid = conf_int_get_default(cfg_context, "sound_equalizer_midvolume");
	context->config.equalizer_high = conf_int_get_default(cfg_context, "sound_equalizer_highvolume");

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

	context->config.attenuation = attenuation;

	sound_volume_update(context);
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
		sound_volume_update(context);
	}
}

static void sound_equalizer_update(struct advance_sound_context* context)
{
	if (context->config.equalizer_low < -20)
		context->config.equalizer_low = -20;
	if (context->config.equalizer_low > 20)
		context->config.equalizer_low = 20;

	if (context->config.equalizer_mid < -20)
		context->config.equalizer_mid = -20;
	if (context->config.equalizer_mid > 20)
		context->config.equalizer_mid = 20;

	if (context->config.equalizer_high < -20)
		context->config.equalizer_high = -20;
	if (context->config.equalizer_high > 20)
		context->config.equalizer_high = 20;

	if (context->config.equalizer_low == 0
		&& context->config.equalizer_mid == 0
		&& context->config.equalizer_high == 0) {
		context->state.equalizer_flag = 0;
	} else {
		adv_bool reset = !context->state.equalizer_flag;

		context->state.equalizer_flag = 1;

		adv_filter_lp_butterworth_set(&context->state.equalizer_low, 0.018, 5);
		adv_filter_bp_butterworth_set(&context->state.equalizer_mid, 0.018, 0.18, 5);
		adv_filter_hp_butterworth_set(&context->state.equalizer_high, 0.18, 5);

		if (reset) {
			unsigned i;
			for(i=0;i<2;++i) {
				adv_filter_state_reset(&context->state.equalizer_low, &context->state.equalizer_low_state[i]);
				adv_filter_state_reset(&context->state.equalizer_mid, &context->state.equalizer_mid_state[i]);
				adv_filter_state_reset(&context->state.equalizer_high, &context->state.equalizer_high_state[i]);
			}
		}

		context->state.equalizer_low_factor = pow(10, (double)context->config.equalizer_low / 20);
		context->state.equalizer_mid_factor = pow(10, (double)context->config.equalizer_mid / 20);
		context->state.equalizer_high_factor = pow(10, (double)context->config.equalizer_high / 20);
	}
}

static void sound_normalize_update(struct advance_sound_context* context)
{
	unsigned i;

	context->state.adjust_power_maximum = 0;

	if (context->config.adjust > 40)
		context->config.adjust = 40;

	if (context->config.adjust < -1)
		context->config.adjust = -1;

	if (context->config.adjust < 0) {
		/* first run, don't apply the correction until the */
		/* first power computation is not zero */
		context->state.adjust_power_waitfirstsound_flag = 1;
		context->state.adjust_power_factor = 100; /* fake high value, never used */
	} else {
		/* use the stored initial value */
		context->state.adjust_power_waitfirstsound_flag = 0;
		context->state.adjust_power_factor = pow(10, (double)context->config.adjust / 20.0);
	}

	context->state.adjust_power_accumulator = 0;
	context->state.adjust_power_counter = 0;
	context->state.adjust_power_counter_limit = context->state.rate * 1; /* recompute every 1 second */
	if (context->state.input_mode != SOUND_MODE_MONO)
		context->state.adjust_power_counter_limit *= 2; /* stereo */
}

static adv_error sound_mode_update(struct advance_sound_context* context)
{
	int new_mode;
	int new_channel;
	int channel;

	/* new requested mode */
	if (context->config.mode == SOUND_MODE_AUTO)
		new_mode = context->state.input_mode;
	else
		new_mode = context->config.mode;
	if (new_mode != SOUND_MODE_MONO)
		new_channel = 2;
	else
		new_channel = 1;
	if (context->state.output_mode != SOUND_MODE_MONO)
		channel = 2;
	else
		channel = 1;

	/* detect change in the number of channel */
	if (new_channel != channel) {
		unsigned rate = context->state.rate;

		if (soundb_is_active()) {
			if (soundb_is_playing())
				soundb_stop();
			soundb_done();
		}

		if (soundb_init(&rate, new_mode != SOUND_MODE_MONO, context->state.buffer_time) != 0) {
			return -1;
		}

		soundb_start(0);
	}

	/* set the new output mode */
	context->state.output_mode = new_mode;
	context->state.output_bytes_per_sample = context->state.output_mode != SOUND_MODE_MONO ? 4 : 2;

	return 0;
}

static adv_error sound_update(struct advance_sound_context* context)
{
	if (sound_mode_update(context) != 0)
		return -1;

	sound_normalize_update(context);
	sound_equalizer_update(context);
	sound_volume_update(context);

	return 0;
}

/**
 * Set a new audio configuration.
 * If the configuration is invalid the previous one is restored.
 */
void advance_sound_reconfigure(struct advance_sound_context* context, struct advance_sound_config_context* config)
{
	struct advance_sound_config_context old_config;

	/* save the old configuration */
	old_config = context->config;

	/* set the new config */
	context->config = *config;

	/* update all the complete state, the configuration is choosen by the name */
	if (sound_update(context) != 0) {

		/* restore the config */
		context->config = old_config;

		log_std(("emu:sound: retrying old config\n"));

		if (sound_update(context) != 0) {
			/* if something go wrong abort */
			log_std(("emu:sound: sound_reconfigure() failed\n"));
			target_err("Unexpected error changing audio options.\n");
			abort();
		}
	}
}

int osd2_sound_init(unsigned* sample_rate, int stereo_flag)
{
	struct advance_sound_context* context = &CONTEXT.sound;
	double low_buffer_time;
	double d;

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
	context->state.input_bytes_per_sample = context->state.input_mode != SOUND_MODE_MONO ? 4 : 2;
	context->state.output_bytes_per_sample = context->state.output_mode != SOUND_MODE_MONO ? 4 : 2;

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
	context->state.buffer_time = low_buffer_time;
	context->state.rate = *sample_rate;
	context->state.latency_min = context->state.rate * context->config.latency_time;
	context->state.latency_max = context->state.rate * low_buffer_time;
	context->state.mute_flag = 0;
	context->state.disabled_flag = 0;
	context->state.equalizer_flag = 0;

	soundb_start(context->config.latency_time);

	sound_normalize_update(context);
	sound_equalizer_update(context);
	sound_volume_update(context);

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

