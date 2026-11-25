/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003 Andrea Mazzoleni
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "portable.h"

#include "ssdl.h"
#include "log.h"
#include "error.h"

#include "ossdl.h"

#include "SDL.h"

/**
 * Base for the volume adjustment.
 */
#define SDL_VOLUME_BASE 32768

/**
 * Default size of the SDL fragment, used with the "auto" option.
 */
#ifdef __WIN32__ /* OSDEF Windows requires a special customization of the default number of sound sample with SDL 1.2.4 */
#define SDL_SAMPLES_DEFAULT 2048
#else
#define SDL_SAMPLES_DEFAULT 512
#endif

struct sdl_option_struct {
	adv_bool initialized;
	unsigned samples; /**< Size of the SDL fragment, 0 for auto. */
};

static struct sdl_option_struct sdl_option;

struct soundb_sdl_context {
	adv_bool active_flag;

	SDL_AudioSpec info;

	unsigned fifo_pos; /**< Position of the first sample to play. */
	unsigned fifo_mac; /**< Number of samples (for all channels) stored. */
	unsigned fifo_size; /**< Size of the fifo in samples, a power of 2. */
	adv_sample* fifo_map; /**< Fifo data. */

	adv_bool started_flag; /**< Samples written by the user, after it underflows are reported. */
	adv_bool underflow_flag;

	int volume; /**< Volume adjustement. SDL_VOLUME_BASE == full volume. */

	double latency_time; /**< Last latency requested with soundb_sdl_start(), 0 if unknown. Kept on a restart of the driver. */
};

static struct soundb_sdl_context sdl_state;

/**
 * Size of the SDL fragment for the specified rate.
 * SDL takes a whole fragment from the fifo at every callback, so the fifo
 * must always contain at least one fragment to avoid an underflow. As the
 * fragment passed to SDL is also counted by soundb_sdl_buffered(), the fifo
 * contains the latency minus one fragment, and then the fragment must not
 * be bigger than half the latency. A power of 2 is used for the reduced
 * size because some SDL backends require it.
 * The reduction is applied also to a size explicitly set by the user,
 * because a bigger fragment always results in an underflow.
 */
static unsigned sdl_samples(unsigned rate)
{
	unsigned samples = sdl_option.samples;

	if (samples == 0)
		samples = SDL_SAMPLES_DEFAULT;

	if (sdl_state.latency_time != 0) {
		unsigned limit = sdl_state.latency_time * rate / 2;
		unsigned reduced = 64;

		while (reduced * 2 <= limit)
			reduced *= 2;

		if (samples > reduced)
			samples = reduced;
	}

	return samples;
}

static adv_device DEVICE[] = {
	{ "auto", -1, "SDL sound" },
	{ 0, 0, 0 }
};

/**
 * Audio callback, called by SDL with the audio lock held.
 */
static void soundb_sdl_callback(void* userdata, Uint8* stream, int len)
{
	struct soundb_sdl_context* state = userdata;
	adv_sample* dst = (adv_sample*)stream;
	unsigned count = len / sizeof(adv_sample);
	unsigned run;
	unsigned first;

	assert(state == &sdl_state);

	run = count;
	if (run > state->fifo_mac)
		run = state->fifo_mac;

	/* copy in two parts for the wrap around of the fifo */
	first = state->fifo_size - state->fifo_pos;
	if (first > run)
		first = run;
	memcpy(dst, state->fifo_map + state->fifo_pos, first * sizeof(adv_sample));
	memcpy(dst + first, state->fifo_map, (run - first) * sizeof(adv_sample));

	state->fifo_pos = (state->fifo_pos + run) & (state->fifo_size - 1);
	state->fifo_mac -= run;

	/* complete with silence if not enough samples */
	if (run < count) {
		memset(dst + run, state->info.silence, (count - run) * sizeof(adv_sample));
		if (state->started_flag)
			state->underflow_flag = 1;
	}
}

adv_error soundb_sdl_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
#if SDL_MAJOR_VERSION == 1
	char name[64];
#endif
	unsigned size;

	log_std(("sound:sdl: soundb_sdl_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", (unsigned)sound_id, (unsigned)*rate, (int)stereo_flag, (double)buffer_time));

	if (!os_internal_sdl_get()) {
		error_set("Unsupported without the SDL library.\n");
		return -1;
	}

	if (!sdl_option.initialized)
		soundb_sdl_default();

	if (sdl_option.samples == 0)
		log_std(("sound:sdl device_sdl_samples auto\n"));
	else
		log_std(("sound:sdl device_sdl_samples %u\n", sdl_option.samples));

	sdl_state.started_flag = 0;
	sdl_state.underflow_flag = 0;
	sdl_state.fifo_pos = 0;
	sdl_state.fifo_mac = 0;
	sdl_state.volume = SDL_VOLUME_BASE;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		error_set("Function SDL_InitSubSystem(SDL_INIT_AUDIO) failed, %s.\n", SDL_GetError());
		goto err;
	}

#if SDL_MAJOR_VERSION == 1
	if (SDL_AudioDriverName(name, sizeof(name))) {
		log_std(("sound:sdl: driver %s\n", name));
	}
#else
	if (SDL_GetCurrentAudioDriver()) {
		log_std(("sound:sdl: driver %s\n", SDL_GetCurrentAudioDriver()));
	}
#endif

	/* in auto mode request the most common rate of the modern hardware */
	sdl_state.info.freq = *rate != 0 ? *rate : 48000;
	/* the native endianess allows to copy the samples directly */
	sdl_state.info.format = AUDIO_S16SYS;
	sdl_state.info.channels = stereo_flag ? 2 : 1;
	sdl_state.info.samples = sdl_samples(sdl_state.info.freq);
	sdl_state.info.callback = soundb_sdl_callback;
	sdl_state.info.userdata = &sdl_state;

	log_std(("sound:sdl: request fragment size %d [samples]\n", sdl_state.info.samples));

#if SDL_MAJOR_VERSION != 1 && defined(__WIN32__) /* OSDEF Only in Windows SDL may select a different rate, like the mix rate of WASAPI. In Linux it always uses the requested rate */
	if (*rate == 0) {
		SDL_AudioSpec obtained;

		/* in auto mode allow SDL to select the rate of the device, */
		/* like the mix rate of WASAPI, to avoid a resampling */
		if (SDL_OpenAudio(&sdl_state.info, &obtained) != 0) {
			error_set("Function SDL_OpenAudio(%d, AUDIO_S16SYS, %d, %d) failed, %s.\n", (unsigned)sdl_state.info.freq, (unsigned)sdl_state.info.channels, (unsigned)sdl_state.info.samples, SDL_GetError());
			goto err_quit;
		}

		log_std(("sound:sdl: obtained rate %d, format 0x%x, channels %d\n", (unsigned)obtained.freq, (unsigned)obtained.format, (unsigned)obtained.channels));

		/* allowing a change of the rate, SDL may also change the format */
		/* and the channels, that are instead required by the play function, */
		/* so reopen with the device rate, letting SDL convert the format */
		SDL_CloseAudio();
		sdl_state.info.freq = obtained.freq;
		sdl_state.info.samples = sdl_samples(sdl_state.info.freq);
	}
#endif

	if (SDL_OpenAudio(&sdl_state.info, 0) != 0) {
		error_set("Function SDL_OpenAudio(%d, AUDIO_S16SYS, %d, %d) failed, %s.\n", (unsigned)sdl_state.info.freq, (unsigned)sdl_state.info.channels, (unsigned)sdl_state.info.samples, SDL_GetError());
		goto err_quit;
	}

	log_std(("sound:sdl: result fragment size %d [samples], buffer size %d [bytes]\n", sdl_state.info.samples, sdl_state.info.size));

	/* allocate a fifo able to store the whole buffer time, */
	/* the callback is not yet called because the audio is paused */
	size = buffer_time * sdl_state.info.freq * sdl_state.info.channels;
	sdl_state.fifo_size = 1;
	while (sdl_state.fifo_size < size)
		sdl_state.fifo_size *= 2;
	sdl_state.fifo_map = malloc(sdl_state.fifo_size * sizeof(adv_sample));

	log_std(("sound:sdl: fifo %u [samples]\n", sdl_state.fifo_size / sdl_state.info.channels));

	*rate = sdl_state.info.freq;

	sdl_state.active_flag = 1;

	return 0;

err_quit:
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
err:
	return -1;
}

void soundb_sdl_done(void)
{
	log_std(("sound:sdl: soundb_sdl_done()\n"));

	if (sdl_state.active_flag) {
		sdl_state.active_flag = 0;
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		free(sdl_state.fifo_map);
	}
}

void soundb_sdl_stop(void)
{
	log_std(("sound:sdl: soundb_sdl_stop()\n"));

	SDL_LockAudio();
	sdl_state.started_flag = 0;
	SDL_UnlockAudio();

	SDL_PauseAudio(1);
}

unsigned soundb_sdl_buffered(void)
{
	unsigned buffered;

	SDL_LockAudio();

	/* the samples in the fragment passed to SDL are not measurable, */
	/* and one fragment is added as an estimation of them */
	buffered = sdl_state.fifo_mac / sdl_state.info.channels + sdl_state.info.samples;

	SDL_UnlockAudio();

	return buffered;
}

void soundb_sdl_volume(double volume)
{
	log_std(("sound:sdl: soundb_sdl_volume(volume:%g)\n", (double)volume));

	sdl_state.volume = volume * SDL_VOLUME_BASE;
	if (sdl_state.volume < 0)
		sdl_state.volume = 0;
	if (sdl_state.volume > SDL_VOLUME_BASE)
		sdl_state.volume = SDL_VOLUME_BASE;
}

/**
 * Write samples in the fifo.
 * It must be called with the audio lock held.
 */
static void sdl_write(const adv_sample* sample_map, unsigned sample_count)
{
	unsigned mask = sdl_state.fifo_size - 1;
	unsigned count = sample_count * sdl_state.info.channels;
	unsigned space = sdl_state.fifo_size - sdl_state.fifo_mac;
	unsigned i;

	/* drop only the samples in excess, keeping the ones already stored */
	if (count > space) {
		log_std(("ERROR:sound:sdl: buffer overflow, %u samples dropped\n", (count - space) / sdl_state.info.channels));
		count = space - space % sdl_state.info.channels;
	}

	i = (sdl_state.fifo_pos + sdl_state.fifo_mac) & mask;
	sdl_state.fifo_mac += count;

	if (sdl_state.volume == SDL_VOLUME_BASE) {
		/* copy in two parts for the wrap around of the fifo */
		unsigned first = sdl_state.fifo_size - i;
		if (first > count)
			first = count;
		memcpy(sdl_state.fifo_map + i, sample_map, first * sizeof(adv_sample));
		memcpy(sdl_state.fifo_map, sample_map + first, (count - first) * sizeof(adv_sample));
	} else {
		while (count) {
			sdl_state.fifo_map[i] = (int)*sample_map * sdl_state.volume / SDL_VOLUME_BASE;
			i = (i + 1) & mask;
			--count;
			++sample_map;
		}
	}
}

void soundb_sdl_play(const adv_sample* sample_map, unsigned sample_count)
{
	SDL_LockAudio();

	log_debug(("sound:sdl: soundb_sdl_play(count:%d), stored %d\n", sample_count, sdl_state.fifo_mac / sdl_state.info.channels));

	if (sdl_state.underflow_flag) {
		sdl_state.underflow_flag = 0;
		log_std(("ERROR:sound:sdl: buffer underflow. Increase the latency with -sound_latency.\n"));
	}

	sdl_write(sample_map, sample_count);

	/* report the underflows only after the first samples written by the user, */
	/* because the silence written at the start may be consumed before them */
	/* by the remaining initialization of the program */
	sdl_state.started_flag = 1;

	SDL_UnlockAudio();
}

adv_error soundb_sdl_start(double silence_time)
{
	adv_sample buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:sdl: soundb_sdl_start(silence_time:%g)\n", (double)silence_time));

	/* the silence time is the requested latency, a restart of the driver */
	/* uses 0 and keeps the latency of the previous start */
	if (silence_time != 0) {
		unsigned samples;

		sdl_state.latency_time = silence_time;

		/* if required, reopen with a smaller fragment for the latency. */
		/* The fifo is kept because the callback is not running */
		samples = sdl_samples(sdl_state.info.freq);
		if (samples != sdl_state.info.samples) {
			log_std(("sound:sdl: reopen with fragment size %u [samples] for the latency\n", samples));

			SDL_CloseAudio();
			sdl_state.info.samples = samples;
			if (SDL_OpenAudio(&sdl_state.info, 0) != 0) {
				error_set("Function SDL_OpenAudio(%d, AUDIO_S16SYS, %d, %d) failed, %s.\n", (unsigned)sdl_state.info.freq, (unsigned)sdl_state.info.channels, (unsigned)sdl_state.info.samples, SDL_GetError());
				return -1;
			}

			log_std(("sound:sdl: result fragment size %d [samples], buffer size %d [bytes]\n", sdl_state.info.samples, sdl_state.info.size));
		}
	}

	for (i = 0; i < 256; ++i)
		buf[i] = sdl_state.info.silence;

	sample = silence_time * sdl_state.info.freq * sdl_state.info.channels;

	log_std(("sound:sdl: writing %d bytes, %d sample of silence\n", sample / sdl_state.info.channels * 2, sample / sdl_state.info.channels));

	SDL_LockAudio();
	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		sdl_write(buf, run / sdl_state.info.channels);
	}
	SDL_UnlockAudio();

	SDL_PauseAudio(0);

	return 0;
}

unsigned soundb_sdl_flags(void)
{
	return SOUND_DRIVER_FLAGS_VOLUME_SAMPLE;
}

static adv_conf_enum_int OPTION[] = {
	{ "auto", 0 },
	{ "128", 128 },
	{ "256", 256 },
	{ "512", 512 },
	{ "1024", 1024 },
	{ "1536", 1536 },
	{ "2048", 2048 },
	{ "2560", 2560 },
	{ "3072", 3072 },
	{ "3584", 3584 },
	{ "4096", 4096 },
	{ "6144", 6144 },
	{ "8192", 8192 },
	{ "12288", 12288 },
	{ "16384", 16384 }
};

adv_error soundb_sdl_load(adv_conf* context)
{
	sdl_option.samples = conf_int_get_default(context, "device_sdl_samples");

	sdl_option.initialized = 1;

	return 0;
}

void soundb_sdl_reg(adv_conf* context)
{
	conf_int_register_enum_default(context, "device_sdl_samples", conf_enum(OPTION), 0);
}

void soundb_sdl_default(void)
{
	sdl_option.samples = 0;

	sdl_option.initialized = 1;
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_sdl_driver = {
	"sdl",
	DEVICE,
	soundb_sdl_load,
	soundb_sdl_reg,
	soundb_sdl_init,
	soundb_sdl_done,
	soundb_sdl_flags,
	soundb_sdl_play,
	soundb_sdl_buffered,
	soundb_sdl_start,
	soundb_sdl_stop,
	soundb_sdl_volume
};

