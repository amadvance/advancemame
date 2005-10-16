/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005 Andrea Mazzoleni
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
 */

#include "portable.h"

#include "play.h"
#include "common.h"

#include "advance.h"

#include <iostream>
#include <iomanip>

using namespace std;

// --------------------------------------------------------------------------
// Sound

extern unsigned char B0_DATA[];
#define B0_DATA_SIZE 2262
extern unsigned char COMEON_DATA[];
#define COMEON_DATA_SIZE 9676
extern unsigned char COOL_DATA[];
#define COOL_DATA_SIZE 8666
extern unsigned char GAMEOVER_DATA[];
#define GAMEOVER_DATA_SIZE 13158
extern unsigned char LETSROCK_DATA[];
#define LETSROCK_DATA_SIZE 12982
extern unsigned char FIRE_DATA[];
#define FIRE_DATA_SIZE 38636

// --------------------------------------------------------------------------
// Internal

#define CHANNEL_BACKGROUND 0
#define CHANNEL_FOREGROUND 1
#define CHANNEL_MAX 2

static unsigned play_rate;
static double play_latency_time;
static double play_buffer_time;
static int play_attenuation;
static bool play_mute;
static unsigned play_priority[CHANNEL_MAX];

static bool play_memory(unsigned channel, unsigned char* data_begin, unsigned char* data_end, bool loop)
{
	if (mixer_play_memory_wav(channel, data_begin, data_end, loop) != 0)
		return false;

	return true;
}

static bool play_file(unsigned channel, const resource& res, bool loop)
{
	if (!res.is_present())
		return false;

	string ext = file_ext(res.path_get());

	if (ext == ".mp3") {
		adv_fz* f = res.open();
		if (!f)
			return false;

		if (mixer_play_file_mp3(channel, f, loop) != 0) {
			fzclose(f);
			return false;
		}
	} else if (ext == ".wav") {
		adv_fz* f = res.open();
		if (!f)
			return false;

		if (mixer_play_file_wav(channel, f, loop) != 0) {
			fzclose(f);
			return false;
		}
	} else {
		return false;
	}

	return true;
}

static void play_wait(unsigned channel)
{
	while (mixer_is_playing(channel)) {
		play_poll();
	}
}

// --------------------------------------------------------------------------
// Public

void play_reg(adv_conf* context)
{
	mixer_reg(context);
	conf_int_register_limit_default(context, "sound_volume", -40, 0, -3);
	conf_int_register_limit_default(context, "sound_samplerate", 5000, 96000, 44100);
	conf_float_register_limit_default(context, "sound_latency", 0.01, 2.0, 0.1);
	conf_float_register_limit_default(context, "sound_buffer", 0.05, 2.0, 0.1);
}

bool play_load(adv_conf* context)
{
	int attenuation;

	if (mixer_load(context)!=0) {
		return false;
	}

	play_attenuation = conf_int_get_default(context, "sound_volume");
	play_latency_time = conf_float_get_default(context, "sound_latency");
	play_buffer_time = conf_float_get_default(context, "sound_buffer");
	play_rate = conf_int_get_default(context, "sound_samplerate");
	play_mute = false;

	return true;
}

bool play_init()
{
	unsigned i;

	for(i=0;i<CHANNEL_MAX;++i)
		play_priority[i] = PLAY_PRIORITY_NONE;

	if (mixer_init(play_rate, CHANNEL_MAX, 1, play_buffer_time + play_latency_time, play_latency_time) != 0)
		return false;

	play_attenuation_set(play_attenuation);

	return true;
}

void play_attenuation_set(int attenuation)
{
	double volume;

	if (attenuation <= -40) {
		play_attenuation = -40;
		volume = 0;
	} else {
		play_attenuation = attenuation;
		volume = 1.0;
		while (attenuation++ < 0)
			volume /= 1.122018454; /* = (10 ^ (1/20)) = 1dB */
	}

	if (play_mute)
		mixer_volume(0);
	else
		mixer_volume(volume);
}

int play_attenuation_get()
{
	return play_attenuation;
}

void play_mute_set(bool mute)
{
	play_mute = mute;

	// recompute the volume
	play_attenuation_set(play_attenuation);
}

bool play_mute_get()
{
	return play_mute;
}

void play_done()
{
	mixer_done();
}

void play_poll()
{
	mixer_poll();
	os_poll();
}

void play_fill()
{
	mixer_poll();
	mixer_poll();
	mixer_poll();
	mixer_poll();
	mixer_poll();
	mixer_poll();
	os_poll();
}

void play_foreground_effect_begin(const resource& s)
{
	if (s.path_get()=="default") {
		play_memory(CHANNEL_FOREGROUND, LETSROCK_DATA, LETSROCK_DATA+LETSROCK_DATA_SIZE, false);
	} else if (s.path_get()!="none") {
		play_file(CHANNEL_FOREGROUND, s, false);
	}
}

void play_foreground_effect_stop(const resource& s)
{
	if (s.path_get()=="default") {
		play_memory(CHANNEL_FOREGROUND, COMEON_DATA, COMEON_DATA+COMEON_DATA_SIZE, false);
	} else if (s.path_get()!="none") {
		play_file(CHANNEL_FOREGROUND, s, false);
	}
}

void play_foreground_effect_end(const resource& s)
{
	if (s.path_get()=="default") {
		play_memory(CHANNEL_FOREGROUND, GAMEOVER_DATA, GAMEOVER_DATA+GAMEOVER_DATA_SIZE, false);
	} else if (s.path_get()!="none") {
		play_file(CHANNEL_FOREGROUND, s, false);
	}
}

void play_foreground_effect_key(const resource& s)
{
	if (s.path_get()=="default") {
		play_memory(CHANNEL_FOREGROUND, B0_DATA, B0_DATA+B0_DATA_SIZE, false);
	} else if (s.path_get()!="none") {
		play_file(CHANNEL_FOREGROUND, s, false);
	}
}

void play_foreground_effect_start(const resource& s)
{
	if (s.path_get()=="default") {
		play_memory(CHANNEL_FOREGROUND, COOL_DATA, COOL_DATA+COOL_DATA_SIZE, false);
	} else if (s.path_get()!="none") {
		play_file(CHANNEL_FOREGROUND, s, false);
	}
}

void play_foreground_wait()
{
	play_wait(CHANNEL_FOREGROUND);
}

bool play_foreground_is_active()
{
	return mixer_is_playing(CHANNEL_FOREGROUND);
}

void play_foreground_stop()
{
	play_priority[CHANNEL_FOREGROUND] = PLAY_PRIORITY_NONE;
	mixer_stop(CHANNEL_FOREGROUND);
}

void play_background_effect(const resource& s, unsigned priority, bool loop)
{
	if (!play_background_is_active())
		play_priority[CHANNEL_BACKGROUND] = PLAY_PRIORITY_NONE;

	if (priority < play_priority[CHANNEL_BACKGROUND])
		return;

	play_priority[CHANNEL_BACKGROUND] = priority;

	if (s.path_get()=="default") {
		if (!play_memory(CHANNEL_BACKGROUND, FIRE_DATA, FIRE_DATA+FIRE_DATA_SIZE, loop)) {
			mixer_stop(CHANNEL_BACKGROUND);
			play_priority[CHANNEL_BACKGROUND] = PLAY_PRIORITY_NONE;
			return;
		}
	} else if (s.path_get()!="none") {
		if (!play_file(CHANNEL_BACKGROUND, s, loop)) {
			mixer_stop(CHANNEL_BACKGROUND);
			play_priority[CHANNEL_BACKGROUND] = PLAY_PRIORITY_NONE;
			return;
		}
	} else {
		mixer_stop(CHANNEL_BACKGROUND);
		play_priority[CHANNEL_BACKGROUND] = PLAY_PRIORITY_NONE;
		return;
	}
}

void play_background_stop(unsigned priority)
{
	if (!play_background_is_active())
		play_priority[CHANNEL_BACKGROUND] = PLAY_PRIORITY_NONE;

	if (priority < play_priority[CHANNEL_BACKGROUND])
		return;

	play_priority[CHANNEL_BACKGROUND] = PLAY_PRIORITY_NONE;
	mixer_stop(CHANNEL_BACKGROUND);
}

void play_background_wait()
{
	play_wait(CHANNEL_BACKGROUND);
}

bool play_background_is_active()
{
	return mixer_is_playing(CHANNEL_BACKGROUND);
}
