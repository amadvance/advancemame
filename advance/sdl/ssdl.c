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
#include "endianrw.h"
#include "error.h"

#include "ossdl.h"

#include "SDL.h"

/**
 * Base for the volume adjustment.
 */
#define SDL_VOLUME_BASE 32768

#define FIFO_MAX 32768

struct sdl_option_struct {
	adv_bool initialized;
	unsigned samples;
};

static struct sdl_option_struct sdl_option;

struct soundb_sdl_context {
	adv_bool active_flag;

	SDL_AudioSpec info;

	unsigned fifo_pos;
	unsigned fifo_mac;
	adv_sample fifo_map[FIFO_MAX];

	adv_bool underflow_flag;

	int volume; /**< Volume adjustement. SDL_VOLUME_BASE == full volume. */
};

static struct soundb_sdl_context sdl_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SDL sound" },
{ 0, 0, 0 }
};

static void soundb_sdl_callback(void *userdata, Uint8 *stream, int len)
{
	struct soundb_sdl_context* state = (struct soundb_sdl_context*)userdata;
	unsigned samples_count;
	Uint8* samples_buffer;

	assert(state == &sdl_state);

	samples_count = len / 2;
	samples_buffer = stream;

	while (samples_count) {
		if (state->fifo_mac) {
			le_uint16_write(samples_buffer, state->fifo_map[state->fifo_pos]);
			state->fifo_pos = (state->fifo_pos + 1) % FIFO_MAX;
			--state->fifo_mac;
		} else {
			le_uint16_write(samples_buffer, state->info.silence);
			state->underflow_flag = 1; /* signal the underflow */
		}

		samples_buffer += 2;
		--samples_count;
	}
}

adv_error soundb_sdl_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	char name[64];

	log_std(("sound:sdl: soundb_sdl_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", (unsigned)sound_id, (unsigned)*rate, (int)stereo_flag, (double)buffer_time));

	if (!os_internal_sdl_get()) {
		error_set("Unsupported without the SDL library.\n");
		return -1;
	}

	if (!sdl_option.initialized) {
		soundb_sdl_default();
	}

	sdl_state.underflow_flag = 0;
	sdl_state.fifo_pos = 0;
	sdl_state.fifo_mac = 0;
	sdl_state.volume = SDL_VOLUME_BASE;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		error_set("Function SDL_InitSubSystem(SDL_INIT_AUDIO) failed, %s.\n", SDL_GetError());
		goto err;
	}

	if (SDL_AudioDriverName(name, sizeof(name))) {
		log_std(("sound:sdl: driver %s\n", name));
	}

	sdl_state.info.freq = *rate;
	sdl_state.info.format = AUDIO_S16LSB;
	sdl_state.info.channels = stereo_flag ? 2 : 1;
	sdl_state.info.samples = sdl_option.samples;
	sdl_state.info.callback = soundb_sdl_callback;
	sdl_state.info.userdata = &sdl_state;

	log_std(("sound:sdl: request fragment size %d [samples]\n", sdl_state.info.samples));

	if (SDL_OpenAudio(&sdl_state.info, 0) != 0) {
		error_set("Function SDL_OpenAudio(%d, AUDIO_S16LSB, %d, %d) failed, %s.\n", (unsigned)sdl_state.info.freq, (unsigned)sdl_state.info.channels, (unsigned)sdl_state.info.samples, SDL_GetError());
		goto err_quit;
	}

	log_std(("sound:sdl: result fragment size %d [samples], buffer size %d [bytes]\n", sdl_state.info.samples, sdl_state.info.size));

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
	}
}

void soundb_sdl_stop(void)
{
	log_std(("sound:sdl: soundb_sdl_stop()\n"));

	SDL_PauseAudio(1);
}

unsigned soundb_sdl_buffered(void)
{
	return sdl_state.fifo_mac / sdl_state.info.channels;
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

void soundb_sdl_play(const adv_sample* sample_map, unsigned sample_count)
{
	unsigned i;
	unsigned count = sample_count * sdl_state.info.channels;

	SDL_LockAudio();

	log_debug(("sound:sdl: soundb_sdl_play(count:%d), stored %d\n", sample_count, sdl_state.fifo_mac / sdl_state.info.channels));

	if (sdl_state.underflow_flag) {
		sdl_state.underflow_flag = 0;
		log_std(("ERROR: sound buffer fifo underflow\n"));
	}

	if (sdl_state.fifo_mac + count > FIFO_MAX) {
		sdl_state.fifo_mac = 0;
		sdl_state.fifo_pos = 0;
		log_std(("ERROR: sound buffer fifo overflow, reset the buffer\n"));
	}

	i = (sdl_state.fifo_pos + sdl_state.fifo_mac) % FIFO_MAX;
	sdl_state.fifo_mac += count;

	if (sdl_state.volume == SDL_VOLUME_BASE) {
		while (count) {
			sdl_state.fifo_map[i] = *sample_map;
			i = (i+1) % FIFO_MAX;
			--count;
			++sample_map;
		}
	} else {
		while (count) {
			sdl_state.fifo_map[i] = (int)*sample_map * sdl_state.volume / SDL_VOLUME_BASE;
			i = (i+1) % FIFO_MAX;
			--count;
			++sample_map;
		}
	}

	log_debug(("sound:sdl: soundb_sdl_play() return stored %d\n", sdl_state.fifo_mac / sdl_state.info.channels));

	SDL_UnlockAudio();
}

adv_error soundb_sdl_start(double silence_time)
{
	adv_sample buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:sdl: soundb_sdl_start(silence_time:%g)\n", (double)silence_time));

	for(i=0;i<256;++i)
		buf[i] = sdl_state.info.silence;

	sample = silence_time * sdl_state.info.freq * sdl_state.info.channels;

	log_std(("sound:sdl: writing %d bytes, %d sample of silence\n", sample / sdl_state.info.channels * 2, sample / sdl_state.info.channels));

	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		soundb_sdl_play(buf, run / sdl_state.info.channels);
	}

	SDL_PauseAudio(0);

	return 0;
}

unsigned soundb_sdl_flags(void)
{
	return SOUND_DRIVER_FLAGS_VOLUME_SAMPLE;
}

static adv_conf_enum_int OPTION[] = {
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
	unsigned def_samples;

#ifdef __WIN32__ /* OSDEF Windows requires a special customization of the default number of sound sample with SDL 1.2.4 */
	def_samples = 2048;
#else
	def_samples = 512;
#endif

	conf_int_register_enum_default(context, "device_sdl_samples", conf_enum(OPTION), def_samples);
}

void soundb_sdl_default(void)
{
#ifdef __WIN32__ /* OSDEF Windows requires a special customization of the default number of sound sample with SDL 1.2.4 */
	sdl_option.samples = 2048;
#else
	sdl_option.samples = 512;
#endif
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


