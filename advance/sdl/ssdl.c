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

#include "ssdl.h"
#include "log.h"

#include <assert.h>

#include "SDL.h"

#define FIFO_MAX 32768

struct sdl_option_struct {
	video_bool initialized;
	unsigned samples;
};

static struct sdl_option_struct sdl_option;

struct sound_sdl_context {
	video_bool active_flag;

	SDL_AudioSpec info;

	unsigned fifo_pos;
	unsigned fifo_mac;
	sound_sample_t fifo_map[FIFO_MAX];
	
	int underflow_flag;
};

static struct sound_sdl_context sdl_state;

static device DEVICE[] = {
{ "auto", -1, "SDL sound" },
{ 0, 0, 0 }
};

static void sound_sdl_callback(void *userdata, Uint8 *stream, int len) {
	struct sound_sdl_context* state = (struct sound_sdl_context*)userdata;
	unsigned samples_count;
	sound_sample_t* samples_buffer;

	assert( state == &sdl_state );

	samples_count = len / 2;
	samples_buffer = (sound_sample_t*)stream;

	while (samples_count) {
		if (state->fifo_mac) {
			*samples_buffer = state->fifo_map[state->fifo_pos];
			state->fifo_pos = (state->fifo_pos + 1) % FIFO_MAX;
			--state->fifo_mac;
		} else {
			*samples_buffer = state->info.silence;
			state->underflow_flag = 1; /* signal the underflow */
		}

		++samples_buffer;
		--samples_count;
	}
}

video_error sound_sdl_init(int sound_id, unsigned* rate, video_bool stereo_flag, double buffer_time)
{
	char name[64];

	log_std(("sound:sdl: sound_sdl_init(id:%d,rate:%d,stereo:%d,buffer_time:%g)\n",(unsigned)sound_id, (unsigned)*rate, (int)stereo_flag, (double)buffer_time));

	if (!sdl_option.initialized) {
		sound_sdl_default();
	}

	sdl_state.underflow_flag = 0;
	sdl_state.fifo_pos = 0;
	sdl_state.fifo_mac = 0;

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		log_std(("sound:sdl: SDL_InitSubSystem(SDL_INIT_AUDIO) failed %s\n", SDL_GetError()));
		return -1;
	}

	if (SDL_AudioDriverName(name,sizeof(name))) {
		log_std(("sound:sdl: driver %s\n", name));
	}

	sdl_state.info.freq = *rate;
	sdl_state.info.format = AUDIO_S16LSB;
	sdl_state.info.channels = stereo_flag ? 2 : 1;
	sdl_state.info.samples = sdl_option.samples;
	sdl_state.info.callback = sound_sdl_callback;
	sdl_state.info.userdata = &sdl_state;

	log_std(("sound:sdl: request fragment size %d [samples]\n", sdl_state.info.samples));

	if (SDL_OpenAudio(&sdl_state.info, 0) != 0) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		log_std(("sound:sdl: SDL_OpenAudio(%d,AUDIO_S16LSB,%d,%d) failed, %s\n", (unsigned)sdl_state.info.freq, (unsigned)sdl_state.info.channels, (unsigned)sdl_state.info.samples, SDL_GetError()));
		return -1;
	}

	log_std(("sound:sdl: result fragment size %d [samples], buffer size %d [bytes]\n", sdl_state.info.samples, sdl_state.info.size));

	*rate = sdl_state.info.freq;

	sdl_state.active_flag = 1;

	return 0;
}

void sound_sdl_done(void) {
	log_std(("sound:sdl: sound_sdl_done()\n"));

	if (sdl_state.active_flag) {
		sdl_state.active_flag = 0;
		SDL_CloseAudio();
		SDL_InitSubSystem(SDL_INIT_AUDIO);
	}
}

void sound_sdl_stop(void) {
	log_std(("sound:sdl: sound_sdl_stop()\n"));

	SDL_PauseAudio(1);
}

unsigned sound_sdl_buffered(void) {
	return sdl_state.fifo_mac / sdl_state.info.channels;
}

void sound_sdl_volume(double volume) {
	log_std(("sound:sdl: sound_sdl_volume(volume:%g)\n",(double)volume));
	/* no hardware volume control with SDL */
}

void sound_sdl_play(const sound_sample_t* sample_map, unsigned sample_count) {
	unsigned i;
	unsigned count = sample_count * sdl_state.info.channels;

	SDL_LockAudio();

	log_debug(("sound:sdl: sound_sdl_play(count:%d), stored %d\n", sample_count, sdl_state.fifo_mac / sdl_state.info.channels));

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
	while (count) {
		sdl_state.fifo_map[i] = *sample_map;
		i = (i+1) % FIFO_MAX;
		--count;
		++sample_map;
	}

	log_debug(("sound:sdl: sound_sdl_play() return stored %d\n", sdl_state.fifo_mac / sdl_state.info.channels));

	SDL_UnlockAudio();
}

video_error sound_sdl_start(double silence_time) {
	sound_sample_t buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:sdl: sound_sdl_start(silence_time:%g)\n", (double)silence_time));

	for(i=0;i<256;++i)
		buf[i] = sdl_state.info.silence;

	sample = silence_time * sdl_state.info.freq * sdl_state.info.channels;

	log_std(("sound:sdl: writing %d bytes, %d sample of silence\n", sample / sdl_state.info.channels * 2, sample / sdl_state.info.channels));

	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		sound_sdl_play(buf,run / sdl_state.info.channels);
	}

	SDL_PauseAudio(0);

	return 0;
}

unsigned sound_sdl_flags(void) {
	return 0;
}

static struct conf_enum_int OPTION[] = {
{ "512", 512 },
{ "1024", 1024 },
{ "2048", 2048 },
{ "4096", 4096 },
{ "8192", 8192 }
};

video_error sound_sdl_load(struct conf_context* context) {
	sdl_option.samples = conf_int_get_default(context, "device_sdl_samples");

	sdl_option.initialized = 1;

	return 0;
}

void sound_sdl_reg(struct conf_context* context) {
	conf_int_register_enum_default(context, "device_sdl_samples", conf_enum(OPTION), 512);
}

void sound_sdl_default(void) {
	sdl_option.samples = 512;
	sdl_option.initialized = 1;
}

/***************************************************************************/
/* Driver */

sound_driver sound_sdl_driver = {
	"sdl",
	DEVICE,
	sound_sdl_load,
	sound_sdl_reg,
	sound_sdl_init,
	sound_sdl_done,
	sound_sdl_flags,
	sound_sdl_play,
	sound_sdl_buffered,
	sound_sdl_start,
	sound_sdl_stop,
	sound_sdl_volume
};


