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

#include "salsa.h"
#include "log.h"
#include "target.h"

/* new ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

#include <assert.h>

struct sound_alsa_context {
	unsigned channel;
	unsigned rate;
	unsigned sample_length;
	unsigned size;
	snd_pcm_t* handle;
};

static struct sound_alsa_context alsa_state;

static adv_device DEVICE[] = {
{ "auto", -1, "ALSA automatic detection" },
{ 0, 0, 0 }
};

static void alsa_log(snd_pcm_hw_params_t* hw_params, snd_pcm_sw_params_t* sw_params)
{
	unsigned period_time;
	snd_pcm_uframes_t period_size;
	unsigned period_count;
	unsigned buffer_time;
	snd_pcm_uframes_t buffer_size;
	unsigned tick_time;

	snd_pcm_uframes_t xfer_align;

	snd_pcm_hw_params_get_period_time(hw_params, &period_time, 0);
	snd_pcm_hw_params_get_period_size(hw_params, &period_size, 0);
	snd_pcm_hw_params_get_periods(hw_params, &period_count, 0);
	snd_pcm_hw_params_get_buffer_time(hw_params, &buffer_time, 0);
	snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	snd_pcm_hw_params_get_tick_time(hw_params, &tick_time, 0);

	log_std(("sound:alsa: hw period_time %d, period_size %d, periods %d, buffer_time %d, buffer_size %d, tick_time %d\n",
		(unsigned)period_time, (unsigned)period_size, (unsigned)period_count, (unsigned)buffer_time, (unsigned)buffer_size, (unsigned)tick_time
	));

	xfer_align = snd_pcm_sw_params_get_xfer_align(sw_params);
	log_std(("sound:alsa: sw xfer_align %d\n",
		(unsigned)xfer_align
	));
}

adv_error sound_alsa_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	int r;
	snd_pcm_hw_params_t* hw_params;
	snd_pcm_sw_params_t* sw_params;
	snd_pcm_uframes_t period_size;
	unsigned period_count;

	log_std(("sound:alsa: sound_alsa_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", sound_id, *rate, stereo_flag, buffer_time));

	if (stereo_flag) {
		alsa_state.sample_length = 4;
		alsa_state.channel = 2;
	} else {
		alsa_state.sample_length = 2;
		alsa_state.channel = 1;
	}

	r = snd_pcm_open(&alsa_state.handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't open audio device: %s\n", snd_strerror(r)));
		goto err;
	}

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);

	r = snd_pcm_hw_params_any(alsa_state.handle, hw_params);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't get hardware config: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_access(alsa_state.handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set interleaved access: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_format(alsa_state.handle, hw_params, SND_PCM_FORMAT_S16_LE);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set audio format: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_channels(alsa_state.handle, hw_params, alsa_state.channel);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set audio channels: %s\n", snd_strerror(r)));
		goto err_close;
	}

	alsa_state.rate = *rate;
	r = snd_pcm_hw_params_set_rate_near(alsa_state.handle, hw_params, &alsa_state.rate, 0);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set audio frequency: %s\n", snd_strerror(r)));
		goto err_close;
	}

	period_size = 2048;
	r = snd_pcm_hw_params_set_period_size_near(alsa_state.handle, hw_params, &period_size, 0);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set period size %d: %s\n", (unsigned)period_size, snd_strerror(r)));
		goto err_close;
	}

	log_std(("sound:alsa: set_period_size_near(%d)\n", (unsigned)period_size));

	period_count = alsa_state.rate * buffer_time / period_size + 1;
	if (period_count < 2)
		period_count = 2;
	r = snd_pcm_hw_params_set_periods_min(alsa_state.handle, hw_params, &period_count, 0);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set period number %d: %s\n", (unsigned)period_count, snd_strerror(r)));
		goto err_close;
	}
	alsa_state.size = period_size * period_count;

	log_std(("sound:alsa: set_periods_min(%d)\n", (unsigned)period_count));

	r = snd_pcm_hw_params(alsa_state.handle, hw_params);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set hw audio parameters: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_sw_params_current(alsa_state.handle, sw_params);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't get software audio parameters: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_sw_params_set_xfer_align(alsa_state.handle, sw_params, 1);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set xfer_align: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_sw_params(alsa_state.handle, sw_params);
	if (r < 0) {
		log_std(("sound:alsa: Couldn't set sw audio parameters: %s\n", snd_strerror(r)));
		goto err_close;
	}

	alsa_log(hw_params, sw_params);

	*rate = alsa_state.rate;

	return 0;

err_close:
	snd_pcm_close(alsa_state.handle);
err:
	target_err("Error initializing the ALSA sound.\n");
	return -1;
}

void sound_alsa_done(void)
{
	log_std(("sound:alsa: sound_alsa_done()\n"));

	snd_pcm_drop(alsa_state.handle);
	snd_pcm_close(alsa_state.handle);
}

void sound_alsa_stop(void)
{
	log_std(("sound:alsa: sound_alsa_stop()\n"));
}

unsigned sound_alsa_buffered(void)
{
	snd_pcm_sframes_t buffered;
	int r;

	r = snd_pcm_delay(alsa_state.handle, &buffered);
	if (r < 0) {
		log_std(("ERROR:sound:alsa snd_pcm_delay() failed %d\n", r));
		buffered = 0;
	}

	log_debug(("sound:alsa: snd_pcm_delay() = %d\n", (unsigned)buffered));

	return buffered;
}

void sound_alsa_volume(double volume)
{
	log_std(("sound:alsa: sound_alsa_volume(volume:%g)\n", (double)volume));
}

void sound_alsa_play(const short* sample_map, unsigned sample_count)
{
	int r;

	log_debug(("sound:alsa: sound_alsa_play(count:%d)\n", sample_count));

	/* calling write with a 0 size result in wrong output */
	while (sample_count) {
		r = snd_pcm_writei(alsa_state.handle, sample_map, sample_count);

		if (r < 0) {
			log_std(("ERROR:sound:alsa: snd_pcm_writei() failed %d\n", r));

			if (r == -EAGAIN) {
				continue;
			}

			if (r < 0) {
				r = snd_pcm_prepare(alsa_state.handle);
				if (r < 0)
					log_std(("sound:alsa: snd_pcm_prepare() failed %d\n", r));
			}

			if (r < 0) {
				break;
			}
		} else {
			sample_count -= r;
			sample_map += r * alsa_state.channel;
		}
	}
}

adv_error sound_alsa_start(double silence_time)
{
	short buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:alsa: sound_alsa_start(silence_time:%g)\n", silence_time));

	for(i=0;i<256;++i)
		buf[i] = 0x8000;

	sample = silence_time * alsa_state.rate * alsa_state.channel;

	log_std(("sound:alsa: writing %d bytes, %d sample of silence\n", sample / alsa_state.channel * alsa_state.sample_length, sample / alsa_state.channel));

	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		sound_alsa_play(buf, run / alsa_state.channel);
	}

	return 0;
}

unsigned sound_alsa_flags(void)
{
	return 0;
}

adv_error sound_alsa_load(adv_conf* context)
{
	return 0;
}

void sound_alsa_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

sound_driver sound_alsa_driver = {
	"alsa",
	DEVICE,
	sound_alsa_load,
	sound_alsa_reg,
	sound_alsa_init,
	sound_alsa_done,
	sound_alsa_flags,
	sound_alsa_play,
	sound_alsa_buffered,
	sound_alsa_start,
	sound_alsa_stop,
	sound_alsa_volume
};


