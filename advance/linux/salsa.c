/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#include "salsa.h"
#include "snstring.h"
#include "log.h"
#include "error.h"

/* Configure the ALSA header to use the new (1.0) ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <alsa/asoundlib.h>

/**
 * Base for the volume adjustment.
 */
#define ALSA_VOLUME_BASE 32768

struct alsa_option_struct {
	adv_bool initialized; /**< Options initialized. */
	char device_buffer[256]; /**< Output card device. */
	char mixer_buffer[256]; /**< Mixer card device. */
};

static struct alsa_option_struct alsa_option;

struct soundb_alsa_context {
	unsigned channel; /**< Number of channels (1 or 2). */
	unsigned rate; /**< Playing ratein Hz. */
	unsigned sample_length; /**< Sample (for all channels) length in bytes. */
	snd_pcm_t* handle; /**< Alsa handle. */
	int volume; /**< Volume adjustement. ALSA_VOLUME_BASE == full volume. */
	snd_pcm_uframes_t buffer_size; /**< ALSA buffe size. */
};

static struct soundb_alsa_context alsa_state;

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

	snd_pcm_hw_params_get_period_time(hw_params, &period_time, 0);
	snd_pcm_hw_params_get_period_size(hw_params, &period_size, 0);
	snd_pcm_hw_params_get_periods(hw_params, &period_count, 0);
	snd_pcm_hw_params_get_buffer_time(hw_params, &buffer_time, 0);
	snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

	log_std(("sound:alsa: hw period_time %g [us], period_size %d, periods %d, buffer_time %g [us], buffer_size %d\n",
		(double)(period_time / 1000000.0), (unsigned)period_size, (unsigned)period_count, (double)(buffer_time / 1000000.0), (unsigned)buffer_size
	));
}

adv_error soundb_alsa_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	int r;
	snd_pcm_hw_params_t* hw_params;
	snd_pcm_sw_params_t* sw_params;
	snd_pcm_uframes_t buffer_size;

	log_std(("sound:alsa: soundb_alsa_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", sound_id, *rate, stereo_flag, buffer_time));

	if (!alsa_option.initialized) {
		soundb_alsa_default();
	}

	alsa_state.volume = ALSA_VOLUME_BASE;

	if (stereo_flag) {
		alsa_state.sample_length = 4;
		alsa_state.channel = 2;
	} else {
		alsa_state.sample_length = 2;
		alsa_state.channel = 1;
	}

	log_std(("sound:alsa: using device %s\n", alsa_option.device_buffer));

	r = snd_pcm_open(&alsa_state.handle, alsa_option.device_buffer, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't open audio device %s: %s\n", alsa_option.device_buffer, snd_strerror(r)));
		goto err;
	}

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);

	r = snd_pcm_hw_params_any(alsa_state.handle, hw_params);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't get hardware config: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_access(alsa_state.handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set interleaved access: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_format(alsa_state.handle, hw_params, SND_PCM_FORMAT_S16_LE);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set audio format: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_channels(alsa_state.handle, hw_params, alsa_state.channel);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set audio channels: %s\n", snd_strerror(r)));
		goto err_close;
	}

	alsa_state.rate = *rate;
	r = snd_pcm_hw_params_set_rate_near(alsa_state.handle, hw_params, &alsa_state.rate, 0);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set audio frequency: %s\n", snd_strerror(r)));
		goto err_close;
	}
	log_std(("sound:alsa: selected rate %d\n", alsa_state.rate));

	buffer_size = alsa_state.rate * buffer_time;

	log_std(("sound:alsa: request buffer_size of %d samples\n", (unsigned)buffer_size));

	r = snd_pcm_hw_params_set_buffer_size_min(alsa_state.handle, hw_params, &buffer_size);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set buffer size min %d: %s\n", (unsigned)buffer_size, snd_strerror(r)));
		r = snd_pcm_hw_params_set_buffer_size_near(alsa_state.handle, hw_params, &buffer_size);
		if (r < 0) {
			log_std(("ERROR:sound:alsa: Couldn't set buffer size near %d: %s\n", (unsigned)buffer_size, snd_strerror(r)));
			goto err_close;
		} else {
			log_std(("sound:alsa: set_buffer_size_near() -> %d\n", (unsigned)buffer_size));
		}
	} else {
		log_std(("sound:alsa: set_buffer_size_min() -> %d\n", (unsigned)buffer_size));
	}

	if (buffer_size < alsa_state.rate * buffer_time) {
		log_std(("ERROR:sound:alsa: audio buffer TOO SMALL\n"));
	}

	/* store the buffer size for later use */
	alsa_state.buffer_size = buffer_size;

	r = snd_pcm_hw_params(alsa_state.handle, hw_params);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set hw audio parameters: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_sw_params_current(alsa_state.handle, sw_params);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't get software audio parameters: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_sw_params(alsa_state.handle, sw_params);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't set sw audio parameters: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_pcm_prepare(alsa_state.handle);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Couldn't prepare audio handle: %s\n", snd_strerror(r)));
		goto err_close;
	}

	alsa_log(hw_params, sw_params);

	*rate = alsa_state.rate;

	return 0;

err_close:
	snd_pcm_close(alsa_state.handle);
err:
	error_set("Error initializing the ALSA library.\n");
	return -1;
}

void soundb_alsa_done(void)
{
	log_std(("sound:alsa: soundb_alsa_done()\n"));

	snd_pcm_drop(alsa_state.handle);
	snd_pcm_close(alsa_state.handle);
}

void soundb_alsa_stop(void)
{
	log_std(("sound:alsa: soundb_alsa_stop()\n"));
}

unsigned soundb_alsa_buffered(void)
{
	int r;
	snd_pcm_sframes_t avail;

	r = snd_pcm_avail(alsa_state.handle);
	if (r < 0) {
		if (r == -EPIPE) {
			log_std(("ERROR:sound:alsa: snd_pcm_avail() failed: %s. Increase the latency with -sound_latency.\n", snd_strerror(r)));
		} else {
			log_std(("ERROR:sound:alsa: snd_pcm_avail() failed: %s\n", snd_strerror(r)));
		}
		return 0;
	}

	avail = r;

	log_debug(("sound:alsa: buffer_size = %d, snd_pcm_avail() = %d\n", (unsigned)alsa_state.buffer_size, (unsigned)avail));

	if (avail > alsa_state.buffer_size)
		return 0;
	return alsa_state.buffer_size - avail;
}

static void alsa_volume_channel(double volume)
{
	alsa_state.volume = volume * ALSA_VOLUME_BASE;
	if (alsa_state.volume < 0)
		alsa_state.volume = 0;
	if (alsa_state.volume > ALSA_VOLUME_BASE)
		alsa_state.volume = ALSA_VOLUME_BASE;
}

static void alsa_volume_mixer(double volume)
{
	snd_mixer_t* handle;
	snd_mixer_elem_t* elem;
	snd_mixer_selem_id_t* sid;
	unsigned c;
	long pmin, pmax;
	long v;
	int r;

	snd_mixer_selem_id_alloca(&sid);

	log_std(("sound:alsa: soundb_alsa_volume(volume:%g)\n", (double)volume));

	snd_mixer_selem_id_set_name(sid, "Master");

	r = snd_mixer_open(&handle, 0);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Mixer open error: %s\n", snd_strerror(r)));
		goto err;
	}

	r = snd_mixer_attach(handle, alsa_option.mixer_buffer);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Mixer attach error: %s\n", snd_strerror(r)));
		goto err_close;
	}

	if ((r = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
		log_std(("ERROR:sound:alsa: Mixer register error: %s\n", snd_strerror(r)));
		goto err_close;
	}

	r = snd_mixer_load(handle);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: Mixer load error: %s\n", snd_strerror(r)));
		goto err_close;
	}

	elem = snd_mixer_find_selem(handle, sid);
	if (!elem) {
		log_std(("ERROR:sound:alsa: Unable to find simple control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid)));
		goto err_close;
	}

	if (volume > 0) {
		if (snd_mixer_selem_has_playback_switch(elem)) {
			log_std(("sound:alsa: enable playback\n"));
			for(c=0;c<=SND_MIXER_SCHN_LAST;++c) {
				snd_mixer_selem_set_playback_switch(elem, c, 1);
			}
		} else {
			log_std(("sound:alsa: skip enable playback\n"));
		}

		if (snd_mixer_selem_has_playback_volume(elem)) {
			log_std(("sound:alsa: set playback volume\n"));

			snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

			v = pmin + (pmax - pmin) * volume + 0.5;
			if (v < pmin)
				v = pmin;
			if (v > pmax)
				v = pmax;

			log_std(("sound:alsa: min:%d, max:%d, set:%d\n", (int)pmin, (int)pmax, (int)v));

			for(c=0;c<=SND_MIXER_SCHN_LAST;++c) {
				snd_mixer_selem_set_playback_volume(elem, c, v);
			}
		} else {
			log_std(("sound:alsa: skip set playback volume\n"));
		}
	} else {
		if (snd_mixer_selem_has_playback_switch(elem)) {
			log_std(("sound:alsa: disable playback\n"));
			for(c=0;c<=SND_MIXER_SCHN_LAST;++c) {
				snd_mixer_selem_set_playback_switch(elem, c, 0);
			}
		} else {
			log_std(("sound:alsa: skip disable playback\n"));
		}
	}

	snd_mixer_close(handle);

	return;

err_close:
	snd_mixer_close(handle);
err:
	return;
}

void soundb_alsa_volume(double volume)
{
	if (strcmp(alsa_option.mixer_buffer, "channel") == 0)
		alsa_volume_channel(volume);
	else
		alsa_volume_mixer(volume);
}

void soundb_alsa_play(const adv_sample* sample_map, unsigned sample_count)
{
	int r;

	log_debug(("sound:alsa: soundb_alsa_play(count:%d)\n", sample_count));

	/* calling write with a 0 size result in wrong output */
	while (sample_count) {
		if (alsa_state.volume == ALSA_VOLUME_BASE) {
			/* write directly */
			r = snd_pcm_writei(alsa_state.handle, sample_map, sample_count);
		} else {
			/* adjust the volume and write */
			const unsigned buf_size = 2048;
			adv_sample buf_map[buf_size];
			unsigned run;
			unsigned i;

			run = sample_count * alsa_state.channel;
			if (run > buf_size)
				run = buf_size;

			for(i=0;i<run;++i)
				buf_map[i] = (int)sample_map[i] * alsa_state.volume / ALSA_VOLUME_BASE;

			r = snd_pcm_writei(alsa_state.handle, buf_map, run / alsa_state.channel);
		}

		log_debug(("sound:alsa: snd_pcm_writei() -> %d\n", r));

		if (r < 0) {
			if (r == -EAGAIN) {
				/* audio buffer full, it should never happen */
				log_std(("WARNING:sound:alsa: snd_pcm_writei() failed: internal buffer full\n"));
				/* retry */
				continue;
			}

			if (r == -EPIPE)
				log_std(("ERROR:sound:alsa: snd_pcm_writei() failed: %s. Increase the latency with -sound_latency.\n", snd_strerror(r)));
			else
				log_std(("ERROR:sound:alsa: snd_pcm_writei() failed: %s (%d)\n", snd_strerror(r), r));

			if (r < 0) {
				r = snd_pcm_prepare(alsa_state.handle);
				if (r < 0)
					log_std(("ERROR:sound:alsa: snd_pcm_prepare() failed: %s\n", snd_strerror(r)));
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

adv_error soundb_alsa_start(double silence_time)
{
	adv_sample buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:alsa: soundb_alsa_start(silence_time:%g)\n", silence_time));

	for(i=0;i<256;++i)
		buf[i] = 0x0;

	sample = silence_time * alsa_state.rate * alsa_state.channel;

	log_std(("sound:alsa: writing %d bytes, %d sample of silence\n", sample / alsa_state.channel * alsa_state.sample_length, sample / alsa_state.channel));

	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		soundb_alsa_play(buf, run / alsa_state.channel);
	}

	return 0;
}

unsigned soundb_alsa_flags(void)
{
	unsigned flags = 0;
	if (alsa_option.initialized
		&& strcmp(alsa_option.mixer_buffer, "channel") == 0)
		flags |= SOUND_DRIVER_FLAGS_VOLUME_SAMPLE;
	return flags;
}

adv_error soundb_alsa_load(adv_conf* context)
{
	sncpy(alsa_option.device_buffer, sizeof(alsa_option.device_buffer), conf_string_get_default(context, "device_alsa_device"));
	sncpy(alsa_option.mixer_buffer, sizeof(alsa_option.mixer_buffer), conf_string_get_default(context, "device_alsa_mixer"));

	alsa_option.initialized = 1;

	return 0;
}

void soundb_alsa_reg(adv_conf* context)
{
	conf_string_register_default(context, "device_alsa_device", "default");
	conf_string_register_default(context, "device_alsa_mixer", "channel");
}

void soundb_alsa_default(void)
{
	sncpy(alsa_option.device_buffer, sizeof(alsa_option.device_buffer), "default");
	sncpy(alsa_option.mixer_buffer, sizeof(alsa_option.mixer_buffer), "channel");

	alsa_option.initialized = 1;
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_alsa_driver = {
	"alsa",
	DEVICE,
	soundb_alsa_load,
	soundb_alsa_reg,
	soundb_alsa_init,
	soundb_alsa_done,
	soundb_alsa_flags,
	soundb_alsa_play,
	soundb_alsa_buffered,
	soundb_alsa_start,
	soundb_alsa_stop,
	soundb_alsa_volume
};

