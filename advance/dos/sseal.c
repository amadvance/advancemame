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

#include "sseal.h"
#include "sounddrv.h"
#include "log.h"
#include "target.h"

#include "audio.h"

#include <dos.h>

#ifdef USE_SOUND_INT
/* from allgro/aintdos.h */
#include "allegro.h"
AL_FUNC(void, _dos_irq_init, (void));
AL_FUNC(void, _dos_irq_exit, (void));
AL_FUNC(int, _install_irq, (int num, AL_METHOD(int, handler, (void))));
AL_FUNC(void, _remove_irq, (int num));

#define _eoi(irq) do { outportb(0x20, 0x20); if ((irq)>7) outportb(0xA0, 0x20); } while (0)

#define SOUND_TIMERS_PER_SECOND 1193181
#define SOUND_SECS_TO_TIMER(x) ((x) * SOUND_TIMERS_PER_SECOND)
#define SOUND_BPS_TO_TIMER(x) (SOUND_TIMERS_PER_SECOND / (x))
#define SOUND_INT_BPS 100
#define SOUND_INT 0x8

static void set_timer_rate(unsigned tick)
{
	outportb(0x43, 0x34);
	outportb(0x40, tick & 0xff);
	outportb(0x40, tick >> 8);
}

#endif

struct soundb_seal_context {
	int active_flag;

	unsigned channel;
	unsigned rate;
	unsigned length;
	unsigned pos;

	target_clock_t last;

	HAC voice[2];
	LPAUDIOWAVE wave[2];
};

static struct soundb_seal_context seal_state;

static adv_device DEVICE[] = {
{ "auto", AUDIO_DEVICE_MAPPER, "SEAL sound" },
{ "sb", 1, "Sound Blaster" },
{ "pas", 3, "Pro Audio Spectrum" },
{ "gusmax", 4, "Gravis Ultrasound Max" },
{ "gus", 5, "Gravis Ultrasound" },
{ "wss", 6, "Windows Sound System" },
{ "ess", 7, "Ensoniq Soundscape" },
{ 0, 0, 0 }
};

adv_error soundb_seal_init(int device_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	unsigned i;
	AUDIOINFO info;
	AUDIOCAPS caps;

	log_std(("sound:seal: soundb_seal_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", device_id, *rate, stereo_flag, buffer_time));

	/* ensure to be able to store the required buffer time */
	buffer_time *= 2;

	if (stereo_flag) {
		seal_state.channel = 2;
	} else {
		seal_state.channel = 1;
	}

	if (AInitialize() != AUDIO_ERROR_NONE) {
		log_std(("sound:seal: error in AInitialize\n"));
		return -1;
	}

	if (device_id == AUDIO_DEVICE_MAPPER) {
		UINT id = device_id;

		if (APingAudio(&id) != AUDIO_ERROR_NONE) {
			log_std(("sound:seal: error in APingAudio\n"));
			return -1;
		}

		device_id = id;

		log_std(("sound:seal: ping %d\n", device_id));

		/* disable the AWE32 driver */
		if (device_id == 2)
			device_id = 1;
	}

	info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_RAW_SAMPLE;
	info.nDeviceId = device_id;
	info.nSampleRate = *rate;

	if (stereo_flag) {
		seal_state.channel = 2;
		info.wFormat |= AUDIO_FORMAT_STEREO;
	} else {
		seal_state.channel = 1;
		info.wFormat |= AUDIO_FORMAT_MONO;
	}

	if (AOpenAudio(&info) != AUDIO_ERROR_NONE) {
		log_std(("sound:seal: error in AOpenAudio\n"));
		return -1;
	}

	AGetAudioDevCaps(info.nDeviceId, &caps);

	log_std(("sound:seal: soundcard %d:%s at %d-bit %s %u Hz\n",
		(unsigned)info.nDeviceId,
		caps.szProductName,
		info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
		info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
		info.nSampleRate));

	if (AOpenVoices(seal_state.channel) != AUDIO_ERROR_NONE) {
		return -1;
	}

	for (i=0;i<seal_state.channel;++i) {
		seal_state.wave[i] = 0;
	}

	seal_state.rate = info.nSampleRate;
	seal_state.length = buffer_time * seal_state.rate;

	*rate = seal_state.rate;

	seal_state.active_flag = 1;

	return 0;
}

void soundb_seal_done(void)
{
	log_std(("sound:seal: soundb_seal_done()\n"));

	if (seal_state.active_flag) {
		seal_state.active_flag = 0;
		ACloseVoices();
		ACloseAudio();
	}
}

void soundb_seal_stop(void)
{
	unsigned i;

#ifdef USE_SOUND_INT
	log_std(("sound:seal: remove_irq()\n"));
	set_timer_rate(0);
	_remove_irq(SOUND_INT);
#endif

	log_std(("sound:seal: soundb_seal_stop()\n"));

	for (i=0;i<seal_state.channel;++i) {
		assert(seal_state.wave[i]);

		AStopVoice(seal_state.voice[i]);

		ADestroyAudioData(seal_state.wave[i]);
		free(seal_state.wave[i]);
		seal_state.wave[i] = 0;

		ADestroyAudioVoice(seal_state.voice[i]);
	}
}

static unsigned soundb_seal_current(void)
{
	LONG play_pos;
	AGetVoicePosition(seal_state.voice[0], &play_pos);
	return play_pos;
}

unsigned soundb_seal_buffered(void)
{
	unsigned play_pos = soundb_seal_current();
	unsigned missing;

	if (play_pos <= seal_state.pos)
		missing = seal_state.pos - play_pos;
	else
		missing = seal_state.pos + (seal_state.length - play_pos);

	return missing;
}

static adv_bool soundb_seal_overflow(unsigned pos, unsigned length)
{
	unsigned play_pos;

	play_pos = soundb_seal_current();

	return pos <= play_pos && play_pos < pos + length;
}

#ifdef USE_SOUND_INT
static volatile unsigned seal_int_counter;
static volatile unsigned seal_int_increment;

static void soundb_seal_update(void)
{
}

static int soundb_seal_int_handler(void)
{
	AUpdateAudio();

	seal_int_counter += seal_int_increment;
	if (seal_int_counter >= 0x10000) {
		seal_int_counter -= 0x10000;
		return 1; /* chain */
	} else {
		_eoi(SOUND_INT);
		return 0; /* exit */
	}
	return 1; /* chain */
}
#else
static void soundb_seal_update(void)
{
	AUpdateAudioEx(soundb_seal_buffered());
}
#endif

adv_error soundb_seal_start(double silence_time)
{
	unsigned i;

	log_std(("sound:seal: soundb_seal_start(silecen_time:%g)\n", silence_time));

	for(i=0;i<seal_state.channel;++i)
	{
		if (ACreateAudioVoice(&seal_state.voice[i]) != AUDIO_ERROR_NONE)
			return -1;

		if ((seal_state.wave[i] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0) {
			ADestroyAudioVoice(seal_state.voice[i]);
			return -1;
		}

		seal_state.wave[i]->wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		seal_state.wave[i]->nSampleRate = seal_state.rate;
		seal_state.wave[i]->dwLength = 2*seal_state.length; /* 2* for 16 bit */
		seal_state.wave[i]->dwLoopStart = 0;
		seal_state.wave[i]->dwLoopEnd = seal_state.wave[i]->dwLength;

		if (ACreateAudioData(seal_state.wave[i]) != AUDIO_ERROR_NONE) {
			free(seal_state.wave[i]);
			seal_state.wave[i] = 0;
			return -1;
		}

		memset(seal_state.wave[i]->lpData, 0, seal_state.wave[i]->dwLength);

		APrimeVoice(seal_state.voice[i], seal_state.wave[i]);

		ASetVoiceFrequency(seal_state.voice[i], seal_state.rate);
	}

	if (seal_state.channel > 1) {
		ASetVoiceVolume(seal_state.voice[0], 32);
		ASetVoiceVolume(seal_state.voice[1], 32);
		ASetVoicePanning(seal_state.voice[0], 0);
		ASetVoicePanning(seal_state.voice[1], 255);
		AStartVoice(seal_state.voice[0]);
		AStartVoice(seal_state.voice[1]);
	} else {
		ASetVoiceVolume(seal_state.voice[0], 64);
		ASetVoicePanning(seal_state.voice[0], 128);
		AStartVoice(seal_state.voice[0]);
	}

	seal_state.pos = (unsigned)(silence_time * seal_state.rate) % seal_state.length;

	soundb_seal_update();

	seal_state.last = target_clock();

	log_std(("sound:seal: soundb_seal_start current %d, buffered %d\n", soundb_seal_current(), soundb_seal_buffered()));

#ifdef USE_SOUND_INT
	log_std(("sound:seal: install_irq()\n"));
	_install_irq(SOUND_INT, soundb_seal_int_handler);
	seal_int_counter = 0;
	seal_int_increment = SOUND_BPS_TO_TIMER(SOUND_INT_BPS);;
	set_timer_rate(seal_int_increment);
#endif

	return 0;
}

void soundb_seal_volume(double volume)
{
	int v;

	log_std(("sound:seal: soundb_seal_volume(volume:%g)\n", (double)volume));

	v = volume * 256;
	if (v < 0)
		v = 0;
	if (v > 256)
		v = 256;
	ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME, v);
}

void soundb_seal_play(const adv_sample* sample_map, unsigned sample_count)
{
	unsigned count = sample_count;
	target_clock_t current = target_clock();

	log_debug(("sound:seal: soundb_seal_play(count:%d)\n", sample_count));

	log_debug(("sound:seal: delay from last update %g, samples %g\n",
		(current - seal_state.last) / (double)TARGET_CLOCKS_PER_SEC,
		(current - seal_state.last) / (double)TARGET_CLOCKS_PER_SEC * seal_state.rate
		));

	seal_state.last = current;

	if (soundb_seal_overflow(seal_state.pos, sample_count)) {
		log_std(("ERROR:sound:seal: sound buffer overflow\n"));
	}

	if (seal_state.channel > 1) {
		while (count) {
			short* buf0;
			short* buf1;
			unsigned run = count;
			unsigned i;
			if (run + seal_state.pos > seal_state.length)
				run = seal_state.length - seal_state.pos;
			buf0 = seal_state.pos + (short*)seal_state.wave[0]->lpData;
			buf1 = seal_state.pos + (short*)seal_state.wave[1]->lpData;
			i = run;
			while (i) {
				*buf0++ = *sample_map++;
				*buf1++ = *sample_map++;
				--i;
			}
			AWriteAudioData(seal_state.wave[0], 2*seal_state.pos, 2*run);
			AWriteAudioData(seal_state.wave[1], 2*seal_state.pos, 2*run);
			seal_state.pos += run;
			if (seal_state.pos == seal_state.length)
				seal_state.pos = 0;
			count -= run;
		}
	} else {
		while (count) {
			short* buf0;
			unsigned run = count;
			unsigned i;
			if (run + seal_state.pos > seal_state.length)
				run = seal_state.length - seal_state.pos;
			buf0 = seal_state.pos + (short*)seal_state.wave[0]->lpData;
			i = run;
			while (i) {
				*buf0++ = *sample_map++;
				--i;
			}
			AWriteAudioData(seal_state.wave[0], 2*seal_state.pos, 2*run);
			seal_state.pos += run;
			if (seal_state.pos == seal_state.length)
				seal_state.pos = 0;
			count -= run;
		}
	}

	soundb_seal_update();
}

unsigned soundb_seal_flags(void)
{
	return 0;
}

adv_error soundb_seal_load(adv_conf* context)
{
	return 0;
}

void soundb_seal_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_seal_driver = {
	"seal",
	DEVICE,
	soundb_seal_load,
	soundb_seal_reg,
	soundb_seal_init,
	soundb_seal_done,
	soundb_seal_flags,
	soundb_seal_play,
	soundb_seal_buffered,
	soundb_seal_start,
	soundb_seal_stop,
	soundb_seal_volume
};


