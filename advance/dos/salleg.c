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

#include "salleg.h"
#include "sounddrv.h"
#include "log.h"
#include "target.h"

#include <dos.h>

#include "allegro2.h"

struct soundb_allegro_context {
	adv_bool active_flag;

	unsigned channel;
	unsigned rate;
	unsigned length;
	unsigned pos;

	target_clock_t last;

	int voice;
	SAMPLE* wave;
};

static struct soundb_allegro_context allegro_state;

static adv_device DEVICE[] = {
{ "auto", DIGI_AUTODETECT, "Allegro sound" },
{ "sb10", DIGI_SB10, "Sound Blaster 1.0" },
{ "sb15", DIGI_SB15, "Sound Blaster 1.5" },
{ "sb20", DIGI_SB20, "Sound Blaster 2.0" },
{ "sbpro", DIGI_SBPRO, "Sound Blaster Pro" },
{ "sb16", DIGI_SB16, "Sound Blaster 16" },
{ "audio", DIGI_AUDIODRIVE, "Ensoniq AudioDrive" },
{ "wss", DIGI_WINSOUNDSYS, "Windows Sound System" },
{ "ess", DIGI_SOUNDSCAPE, "Ensoniq Soundscape" },
{ 0, 0, 0 }
};

static int allegro_freq;

int __real__mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices);

int __wrap__mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices)
{
	log_std(("sound:allegro: sound bufsize %d, freq %d, stereo %d, is16bit %d\n", bufsize, freq, stereo, is16bit));
	allegro_freq = freq;
	return __real__mixer_init(bufsize, freq, stereo, is16bit, voices);
}

adv_error soundb_allegro_init(int device_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	log_std(("sound:allegro: soundb_allegro_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", device_id, *rate, stereo_flag, buffer_time));

	/* ensure to be able to store the required buffer time */
	buffer_time *= 2;

	if (stereo_flag) {
		allegro_state.channel = 2;
	} else {
		allegro_state.channel = 1;
	}

	reserve_voices(1, 0);
	set_volume_per_voice(0);

	if (install_sound(device_id, MIDI_NONE, 0) != 0) {
		return -1;
	}

	set_volume(255, 0);

	allegro_state.wave = 0;
	allegro_state.rate = allegro_freq;
	allegro_state.length = buffer_time * allegro_state.rate;

	*rate = allegro_state.rate;

	allegro_state.active_flag = 1;

	return 0;
}

void soundb_allegro_done(void)
{
	log_std(("sound:allegro: soundb_allegro_done()\n"));

	if (allegro_state.active_flag) {
		allegro_state.active_flag = 0;
		remove_sound();
	}
}

void soundb_allegro_stop(void)
{
	log_std(("sound:allegro: soundb_allegro_stop()\n"));

	voice_stop(allegro_state.voice);
	deallocate_voice(allegro_state.voice);
	destroy_sample(allegro_state.wave);
	allegro_state.wave = 0;
}

static unsigned soundb_allegro_current(void)
{
	int play_pos;
	play_pos = voice_get_position(allegro_state.voice);
	return play_pos;
}

unsigned soundb_allegro_buffered(void)
{
	unsigned play_pos = soundb_allegro_current();
	unsigned missing;

	if (play_pos <= allegro_state.pos)
		missing = allegro_state.pos - play_pos;
	else
		missing = allegro_state.pos + (allegro_state.length - play_pos);

	return missing;
}

static adv_bool soundb_allegro_overflow(unsigned pos, unsigned length)
{
	unsigned play_pos;

	play_pos = soundb_allegro_current();

	return pos <= play_pos && play_pos < pos + length;
}

adv_error soundb_allegro_start(double silence_time)
{
	unsigned i;

	log_std(("sound:allegro: soundb_allegro_start(silence_time:%g)\n", silence_time));

	allegro_state.wave = create_sample(16, allegro_state.channel > 1, allegro_state.rate, allegro_state.length);
	if (!allegro_state.wave)
		return -1;

	for(i=0;i<allegro_state.length * allegro_state.channel;++i)
		((unsigned short*)allegro_state.wave->data)[i] = 0x8000;

	allegro_state.voice = allocate_voice(allegro_state.wave);

	voice_set_playmode(allegro_state.voice, PLAYMODE_LOOP);

	voice_start(allegro_state.voice);

	allegro_state.pos = (unsigned)(silence_time * allegro_state.rate) % allegro_state.length;

	allegro_state.last = target_clock();

	log_std(("sound:allegro: soundb_allegro_start current %d, buffered %d\n", soundb_allegro_current(), soundb_allegro_buffered()));

	return 0;
}

void soundb_allegro_volume(double volume)
{
	int v;

	log_std(("sound:allegro: soundb_allegro_volume(volume:%g)\n", (double)volume));

	v = volume * 255;
	if (v < 0)
		v = 0;
	if (v > 255)
		v = 255;
	set_volume(v, 0);
}

void soundb_allegro_play(const adv_sample* sample_map, unsigned sample_count)
{
	unsigned count = sample_count;
	target_clock_t current = target_clock();

	log_debug(("sound:allegro: soundb_allegro_play(count:%d)\n", sample_count));

	log_debug(("sound:allegro: delay from last update %g, samples %g\n",
		(current - allegro_state.last) / (double)TARGET_CLOCKS_PER_SEC,
		(current - allegro_state.last) / (double)TARGET_CLOCKS_PER_SEC * allegro_state.rate
		));

	allegro_state.last = current;

	if (soundb_allegro_overflow(allegro_state.pos, sample_count)) {
		log_std(("ERROR:sound:allegro: sound buffer overflow\n"));
	}

	if (allegro_state.channel > 1) {
		while (count) {
			unsigned short* buf;
			unsigned run = count;
			unsigned i;
			if (run + allegro_state.pos > allegro_state.length)
				run = allegro_state.length - allegro_state.pos;
			buf = 2 * allegro_state.pos + (unsigned short*)allegro_state.wave->data;
			i = run;
			while (i) {
				*buf++ = *sample_map++ ^ 0x8000;
				*buf++ = *sample_map++ ^ 0x8000;
				--i;
			}
			allegro_state.pos += run;
			if (allegro_state.pos == allegro_state.length)
				allegro_state.pos = 0;
			count -= run;
		}
	} else {
		while (count) {
			unsigned short* buf;
			unsigned run = count;
			unsigned i;
			if (run + allegro_state.pos > allegro_state.length)
				run = allegro_state.length - allegro_state.pos;
			buf = allegro_state.pos + (unsigned short*)allegro_state.wave->data;
			i = run;
			while (i) {
				*buf++ = *sample_map++ ^ 0x8000;
				--i;
			}
			allegro_state.pos += run;
			if (allegro_state.pos == allegro_state.length)
				allegro_state.pos = 0;
			count -= run;
		}
	}
}

unsigned soundb_allegro_flags(void)
{
	return 0;
}

adv_error soundb_allegro_load(adv_conf* context)
{
	return 0;
}

void soundb_allegro_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_allegro_driver = {
	"allegro",
	DEVICE,
	soundb_allegro_load,
	soundb_allegro_reg,
	soundb_allegro_init,
	soundb_allegro_done,
	soundb_allegro_flags,
	soundb_allegro_play,
	soundb_allegro_buffered,
	soundb_allegro_start,
	soundb_allegro_stop,
	soundb_allegro_volume
};
