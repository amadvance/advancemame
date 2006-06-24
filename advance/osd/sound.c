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
 * Expected volume factor.
 * Value guessed with some tries.
 */
#define ADJUST_EXPECTED_FACTOR 0.25 /* -12 dB */

/***************************************************************************/
/* Advance interface */

/**
 * Maximum volume requested at the hardware mixer.
 */
#define SOUND_MIXER_MAX 1.0

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
	volume *= pow(10, (double)context->config.adjust / 20.0);

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

static void sound_dft_normalize(struct advance_sound_context* context, double* X)
{
	unsigned i;
	int j;
	double power;
	unsigned count;
	double gain;
	double* loudness = context->state.dft_equal_loudness;
	double m;
	int index;

	/* compute the power of the DFT using the equal loudness curve */
	power = 0;
	for(i=1;i<context->state.dft_padded_size/2;++i) {
		m = X[i] * loudness[i];
		power += m * m;
	}
	/* duplicate the two halve */
	power *= 2;
	/* add the first and central samples */
	m = X[0] * loudness[0];
	power += m * m;
	m = X[i] * loudness[i];
	power += m * m;
	power /= context->state.dft_padded_size;

	/* normalize the power */
	power = sqrt(power);

	log_debug(("emu:sound: normalized power %g\n", power));

	/* ignore silence (and prevent error in the log operation) */
	if (power < 1E-6)
		return;

	/* compute the normalized power in dB */
	m = 20 * log10(power);

	/* convert to integer and shift in the range 0 - SOUND_POWER_DB_MAX-1 */
	j = m + SOUND_POWER_DB_MAX;
	if (j < 0)
		j = 0;
	if (j >= SOUND_POWER_DB_MAX)
		j = SOUND_POWER_DB_MAX - 1;

	/* ignore silence */
	if (j == 0)
		return;

	/* add the new power */
	++context->state.adjust_power_db_map[j];
	++context->state.adjust_power_db_counter;

	/* remove the oldest power */
	if (context->state.adjust_power_db_counter > context->state.adjust_power_history_max) {
		unsigned k = context->state.adjust_power_history_map[context->state.adjust_power_history_mac];
		--context->state.adjust_power_db_map[k];
		--context->state.adjust_power_db_counter;
	}

	/* save the current measure */
	context->state.adjust_power_history_map[context->state.adjust_power_history_mac] = j;
	++context->state.adjust_power_history_mac;
	if (context->state.adjust_power_history_mac == context->state.adjust_power_history_max)
		context->state.adjust_power_history_mac = 0;

	/* wait to have some data */
	if (context->state.adjust_power_db_counter < 20)
		return;

	/* find the first 5% with higher power */
	j = context->state.adjust_power_db_counter / 20;
	i = SOUND_POWER_DB_MAX - 1;
	while (i > 0 && j > 0) {
		j -= context->state.adjust_power_db_map[i];
		--i;
	}

	/* use the median power at the 5% */
	power = pow(10, (i - (double)SOUND_POWER_DB_MAX) / 20);

	/* compute the amplification factor using the normalized power */
	gain = ADJUST_EXPECTED_FACTOR / power;

	/* limit the gain */
	if (gain < 1)
		gain = 1;
	if (gain > 100)
		gain = 100;

	log_debug(("osd:sound: normalize factor %g, time %g (power %g, expected %g)\n", (double)gain, advance_timer(), power, (double)ADJUST_EXPECTED_FACTOR));

	j = 20 * log10(gain);

	if (j < 0)
		j = 0;
	if (j > 40)
		j = 40;

	/* change only if different */
	if (j != context->config.adjust) {
		context->config.adjust = j;

		log_std(("osd:sound: normalize factor dB %d\n", j));

		sound_volume_update(context);
	}
}

static void sound_dft_process(struct advance_sound_context* context, double* x, double* X)
{
	unsigned i;
	double* xr = adv_dft_re_get(&context->state.dft_plan);
	double* xi = adv_dft_im_get(&context->state.dft_plan);
	double* w = context->state.dft_window;
	unsigned size = context->state.dft_size;
	unsigned padded_size = context->state.dft_padded_size;
	double normalize;

	/* copy the input samples and use a window function */
	for(i=0;i<size;++i) {
		xr[i] = x[i] * w[i];
	}

	/* pad with 0 if required */
	for(;i<padded_size;++i) {
		xr[i] = 0;
	}

	/* note that isn't required to fill xi with 0 because the */
	/* real version of the DFT is used */
	adv_dft_execute(&context->state.dft_plan);

	/* power normalization factor, required to have the same power */
	/* after the DFT computation */
	normalize = sqrt(padded_size);

	/* compute the modulus (only the first halve+1) */
	for(i=0;i<padded_size/2+1;++i) {
		X[i] = hypot(xr[i], xi[i]) / normalize;
	}
}

/* Compute the DFT */
static void sound_dft(struct advance_sound_context* context, unsigned channel, const short* sample, unsigned sample_count, double* x, double* X, unsigned* pcounter, void (*callback)(struct advance_sound_context* context, double* X))
{
	unsigned i;
	unsigned counter = *pcounter;

	i = 0;

	while (i < sample_count) {
		unsigned run = sample_count - i;

		if (counter + run > context->state.dft_size)
			run = context->state.dft_size - counter;

		if (channel == 1) {
			while (run) {
				x[counter] = (double)sample[i] / 32768.0;
				++counter;

				++i;
				--run;
			}
		} else if (channel == 2) {
			while (run) {
				x[counter] = ((double)sample[i*2] + (double)sample[i*2+1]) / 65536.0;
				++counter;

				++i;
				--run;
			}
		} else {
			i += run;
		}

		if (counter == context->state.dft_size) {
			sound_dft_process(context, x, X);
			counter = 0;
			if (callback)
				callback(context, X);
		}
	}

	*pcounter = counter;
}

static void sound_dft_pre(struct advance_sound_context* context, unsigned channel, const short* sample, unsigned sample_count)
{
	sound_dft(context, channel, sample, sample_count, context->state.dft_pre_x, context->state.dft_pre_X, &context->state.dft_pre_counter, sound_dft_normalize);
}

static void sound_dft_post(struct advance_sound_context* context, unsigned channel, const short* sample, unsigned sample_count)
{
	sound_dft(context, channel, sample, sample_count, context->state.dft_post_x, context->state.dft_post_X, &context->state.dft_post_counter, 0);
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

		if (v > 32767) {
			++context->state.overflow;
			v = 32767;
		}
		if (v < -32768) {
			++context->state.overflow;
			v = -32768;
		}

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
			double v, vl, vm, vh, vr;
			int vi;

			v = input_sample[off];

			vr = 0;

			if (context->config.equalizer_low > -40) {
				adv_filter_insert(&context->state.equalizer_low, &context->state.equalizer_low_state[j], v);
				vr += context->state.equalizer_low_factor * adv_filter_extract(&context->state.equalizer_low, &context->state.equalizer_low_state[j]);
			}

			if (context->config.equalizer_mid > -40) {
				adv_filter_insert(&context->state.equalizer_mid, &context->state.equalizer_mid_state[j], v);
				vr += context->state.equalizer_mid_factor * adv_filter_extract(&context->state.equalizer_mid, &context->state.equalizer_mid_state[j]);
			}

			if (context->config.equalizer_high > -40) {
				adv_filter_insert(&context->state.equalizer_high, &context->state.equalizer_high_state[j], v);
				vr += context->state.equalizer_high_factor * adv_filter_extract(&context->state.equalizer_high, &context->state.equalizer_high_state[j]);
			}

			/* lrint is potentially faster than a cast to int */
			vi = lrint(vr);

			if (vi > 32767) {
				++context->state.overflow;
				vi = 32767;
			}
			if (vi < -32768) {
				++context->state.overflow;
				vi = -32768;
			}

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
	unsigned input_channel = context->state.input_mode != SOUND_MODE_MONO ? 2 : 1;

	/* compute the DFT only if the menu is active */
	if (context->state.menu_sub_flag)
		sound_dft_post(context, input_channel, sample_buffer, sample_count);

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
		} else if (context->state.input_mode == SOUND_MODE_STEREO && context->state.output_mode == SOUND_MODE_MONO) {
			unsigned i;
			const short* sample_stereo = sample_buffer;
			short* sample_mono = sample_mix;
			for(i=0;i<sample_count;++i) {
				sample_mono[0] = (sample_stereo[0] + sample_stereo[1]) / 2;
				sample_mono += 1;
				sample_stereo += 2;
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

static void sound_play_adjust(struct advance_sound_context* context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool normal_speed)
{
	unsigned input_channel = context->state.input_mode != SOUND_MODE_MONO ? 2 : 1;

	if (context->config.normalize_flag) {
		sound_dft_pre(context, input_channel, sample_buffer, sample_count);
	}

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
void advance_sound_frame(struct advance_sound_context* context, struct advance_record_context* record_context, struct advance_video_context* video_context, struct advance_safequit_context* safequit_context, const short* sample_buffer, unsigned sample_count, unsigned sample_recount, adv_bool normal_speed)
{
	adv_bool mute;
	double start, stop;

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

	start = advance_timer();

	sound_play_adjust(context, sample_buffer, sample_count, sample_recount, normal_speed);

	stop = advance_timer();

	/* update the time only if the menu is not active */
	if (!context->state.menu_sub_flag)
		context->state.time = (stop - start) * 0.05 + context->state.time * 0.95;

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
	conf_int_register_limit_default(cfg_context, "sound_equalizer_lowvolume", -40, 20, 0);
	conf_int_register_limit_default(cfg_context, "sound_equalizer_midvolume", -40, 20, 0);
	conf_int_register_limit_default(cfg_context, "sound_equalizer_highvolume", -40, 20, 0);
	conf_string_register_default(cfg_context, "sound_adjust", "auto");
	conf_int_register_limit_default(cfg_context, "sound_samplerate", 5000, 96000, 44100);
	conf_bool_register_default(cfg_context, "sound_normalize", 1);
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
		i = 0;
	} else if (strcmp(s, "auto") == 0) {
		for(i=0;GAME_ADJUST[i].name != 0;++i)
			if (mame_is_game_relative(GAME_ADJUST[i].name, option->game))
				break;
		if (GAME_ADJUST[i].name != 0)
			i = GAME_ADJUST[i].gain;
		else
			i = 0;
	} else {
		char* e;
		i = strtol(s, &e, 10);
		if (*e != 0 || i < 0 || i > 40) {
			target_err("Invalid argument '%s' for option 'sound_adjust'.\n", s);
			return -1;
		}
	}
	context->config.adjust = i;

	context->config.latency_time = conf_float_get_default(cfg_context, "sound_latency");
	context->config.normalize_flag = conf_bool_get_default(cfg_context, "sound_normalize");
	context->config.mutedemo_flag = conf_bool_get_default(cfg_context, "misc_mutedemo");
	context->config.mutestartup_flag = 1;
	option->samplerate = conf_int_get_default(cfg_context, "sound_samplerate");

	context->config.equalizer_low = conf_int_get_default(cfg_context, "sound_equalizer_lowvolume");
	context->config.equalizer_mid = conf_int_get_default(cfg_context, "sound_equalizer_midvolume");
	context->config.equalizer_high = conf_int_get_default(cfg_context, "sound_equalizer_highvolume");

	if (soundb_load(cfg_context)!=0) {
		return -1;
	}

	return 0;
}

void advance_sound_config_save(struct advance_sound_context* context, const char* section)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	char buffer[16];

	conf_int_set_if_different(cfg_context, section, "sound_mode", context->config.mode);
	snprintf(buffer, sizeof(buffer), "%d", context->config.adjust);
	conf_string_set_if_different(cfg_context, section, "sound_adjust", buffer);
	conf_bool_set_if_different(cfg_context, section, "sound_normalize", context->config.normalize_flag);
	conf_int_set_if_different(cfg_context, section, "sound_equalizer_lowvolume", context->config.equalizer_low);
	conf_int_set_if_different(cfg_context, section, "sound_equalizer_midvolume", context->config.equalizer_mid);
	conf_int_set_if_different(cfg_context, section, "sound_equalizer_highvolume", context->config.equalizer_high);

	advance_global_message(&CONTEXT.global, "Audio options saved in %s/", section);
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

void osd_sound_enable(int enable)
{
	struct advance_sound_context* context = &CONTEXT.sound;

	if (context->state.active_flag) {
		context->state.disabled_flag = !enable;
		sound_volume_update(context);
	}
}

void osd2_sound_pause(int pause)
{
	/* osd_sound_enable is already called by MAME */
}

static void sound_equalizer_update(struct advance_sound_context* context)
{
	if (context->config.equalizer_low < -40)
		context->config.equalizer_low = -40;
	if (context->config.equalizer_low > 20)
		context->config.equalizer_low = 20;

	if (context->config.equalizer_mid < -40)
		context->config.equalizer_mid = -40;
	if (context->config.equalizer_mid > 20)
		context->config.equalizer_mid = 20;

	if (context->config.equalizer_high < -40)
		context->config.equalizer_high = -40;
	if (context->config.equalizer_high > 20)
		context->config.equalizer_high = 20;

	if (context->config.equalizer_low == 0
		&& context->config.equalizer_mid == 0
		&& context->config.equalizer_high == 0) {
		context->state.equalizer_flag = 0;
	} else {
		adv_bool reset = !context->state.equalizer_flag;

		context->state.equalizer_flag = 1;

		adv_filter_lp_chebyshev_set(&context->state.equalizer_low, context->config.eql_cut1, 5, -1);
		adv_filter_bp_chebyshev_set(&context->state.equalizer_mid, context->config.eql_cut1, context->config.eql_cut2, 5, -1);
		adv_filter_hp_chebyshev_set(&context->state.equalizer_high, context->config.eql_cut2, 5, -1);

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
	if (context->config.adjust > 40)
		context->config.adjust = 40;
	if (context->config.adjust < 0)
		context->config.adjust = 0;
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

struct loudness {
	int f; /**< Frequency in Hz. */
	double m; /**< Attenuation in dB. */
} LOUDNESS[] = {
{ 0, 120 },
{ 20, 113 },
{ 30, 103 },
{ 40, 97 },
{ 50, 93 },
{ 60, 91 },
{ 70, 89 },
{ 80, 87 },
{ 90, 86 },
{ 100, 85 },
{ 200, 78 },
{ 300, 76 },
{ 400, 76 },
{ 500, 76 },
{ 600, 76 },
{ 700, 77 },
{ 800, 78 },
{ 900, 79.5 },
{ 1000, 80 },
{ 1500, 79 },
{ 2000, 77 },
{ 2500, 74 },
{ 3000, 71.5 },
{ 3700, 70 },
{ 4000, 70.5 },
{ 5000, 74 },
{ 6000, 79 },
{ 7000, 84 },
{ 8000, 86 },
{ 9000, 86 },
{ 10000, 85 },
{ 12000, 95 },
{ 15000, 110 },
{ 20000, 125 },
{ 22050, 140 }
};

#define LOUDNESS_MAX (sizeof(LOUDNESS)/sizeof(LOUDNESS[0]))

int osd2_sound_init(unsigned* sample_rate, int stereo_flag)
{
	struct advance_sound_context* context = &CONTEXT.sound;
	double low_buffer_time;
	double d;
	unsigned i;

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

	context->state.time = 0;

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

	context->state.menu_sub_flag = 0;
	context->state.menu_sub_selected = 0;

	context->state.active_flag = 1;
	context->state.buffer_time = low_buffer_time;
	context->state.rate = *sample_rate;
	context->state.latency_min = context->state.rate * context->config.latency_time;
	context->state.latency_max = context->state.rate * low_buffer_time;
	context->state.mute_flag = 0;
	context->state.disabled_flag = 0;
	context->state.equalizer_flag = 0;

	context->state.dft_size = 20 * *sample_rate / 1000; /* 20 ms */
	if (context->state.dft_size < 64)
		context->state.dft_size = 64;
	context->state.dft_padded_size = 1;
	while (context->state.dft_padded_size < context->state.dft_size)
		context->state.dft_padded_size *= 2;
	context->state.dft_size = context->state.dft_padded_size;
	log_std(("emu:sound: dft sample %d, size %d\n", context->state.dft_size, context->state.dft_padded_size));

	context->state.dft_pre_counter = 0;
	context->state.dft_pre_x = (double*)malloc(context->state.dft_size * sizeof(double));
	context->state.dft_pre_X = (double*)malloc(context->state.dft_padded_size * sizeof(double));
	for(i=0;i<context->state.dft_padded_size;++i)
		context->state.dft_pre_X[i] = 0;

	context->state.dft_post_counter = 0;
	context->state.dft_post_x = (double*)malloc(context->state.dft_size * sizeof(double));
	context->state.dft_post_X = (double*)malloc(context->state.dft_padded_size * sizeof(double));
	for(i=0;i<context->state.dft_padded_size;++i)
		context->state.dft_post_X[i] = 0;

	/* set the DFT plan */
	adv_dftr_init(&context->state.dft_plan, context->state.dft_padded_size);

	/* set the DFT window */
	context->state.dft_window = (double*)malloc(context->state.dft_size * sizeof(double));
	d = 0;
	for(i=0;i<context->state.dft_size;++i) {
		double w;
		w = 0.54 - 0.46 * cos(2*M_PI*i/(context->state.dft_size-1)); /* Hamming */
		/* w = 0.42 - 0.5 * cos(2*M_PI*i/(context->state.dft_size-1)) + 0.08 * cos(4*M_PI*i/(size-1)); */ /* Blackman */
		/* w = 1.0; */
		context->state.dft_window[i] = w;
		d += context->state.dft_window[i];
	}
	/* adjust the window gain */
	d /= context->state.dft_size;
	for(i=0;i<context->state.dft_size;++i) {
		context->state.dft_window[i] /= d;
	}

	/* set the equal loudness coefficients */
	context->state.dft_equal_loudness = (double*)malloc(context->state.dft_padded_size * sizeof(double));
	for(i=0;i<context->state.dft_padded_size/2+1;++i) {
		unsigned j;
		double f = *sample_rate * (double)i / context->state.dft_padded_size;
		double a;

		j = 0;
		while (j<LOUDNESS_MAX) {
			if (LOUDNESS[j].f > f)
				break;
			++j;
		}
		if (j == 0) {
			a = LOUDNESS[0].m;
		} else if (j == LOUDNESS_MAX) {
			a = LOUDNESS[LOUDNESS_MAX-1].m;
		} else {
			double ml = LOUDNESS[j-1].m;
			double mr = LOUDNESS[j].m;
			double fl = LOUDNESS[j-1].f;
			double fr = LOUDNESS[j].f;

			/* linear interpolation */
			a = ml + (mr - ml) * (f - fl) / (fr - fl);
		}

		/* convert from decibel */
		a = pow(10,(80-a) / 20);

		log_debug(("emu:sound: normalize freq:%g magnitute:%g\n", f, a));

		context->state.dft_equal_loudness[i] = a;
	}
	/* second halve of the spectrum to 0 */
	for(;i<context->state.dft_padded_size;++i) {
		context->state.dft_equal_loudness[i] = 0;
	}

	for(i=0;i<SOUND_POWER_DB_MAX;++i)
		context->state.adjust_power_db_map[i] = 0;
	context->state.adjust_power_db_counter = 0;

	context->state.adjust_power_history_max = 3 * 60 * *sample_rate / context->state.dft_size; /* 3 minutes */
	context->state.adjust_power_history_map = (unsigned char*)malloc(context->state.adjust_power_history_max * sizeof(unsigned char));
	for(i=0;i<context->state.adjust_power_history_max;++i)
		context->state.adjust_power_history_map[i] = 0;
	context->state.adjust_power_history_mac = 0;

	context->config.eql_cut1 = 800. / *sample_rate;
	context->config.eql_cut2 = 8000. / *sample_rate;
	if (context->config.eql_cut1 > 0.1)
		context->config.eql_cut1 = 0.1;
	if (context->config.eql_cut2 > 0.5)
		context->config.eql_cut2 = 0.5;

	context->state.overflow = 0;

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

	adv_dft_free(&context->state.dft_plan);

	free(context->state.dft_pre_x);
	free(context->state.dft_pre_X);
	free(context->state.dft_post_x);
	free(context->state.dft_post_X);
	free(context->state.dft_window);
	free(context->state.dft_equal_loudness);
	free(context->state.adjust_power_history_map);
}

