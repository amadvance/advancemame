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
	unsigned rate; /**< Playing rate in Hz. */
	snd_pcm_t* handle; /**< Alsa handle. */
	int volume; /**< Volume adjustement. ALSA_VOLUME_BASE == full volume. */
	snd_pcm_uframes_t buffer_size; /**< ALSA buffer size in frames. */
	snd_pcm_uframes_t period_size; /**< ALSA period size in frames. */
	double latency_time; /**< Last latency requested with soundb_alsa_start(), 0 if unknown. Kept on a restart of the driver. */
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
	int dir;
	int r;

	r = snd_pcm_hw_params_get_period_time(hw_params, &period_time, &dir);
	if (r < 0)
		log_std(("ERROR:sound:alsa: snd_pcm_hw_params_get_period_time: %s\n", snd_strerror(r)));
	else
		log_std(("sound:alsa: hw period_time %u [us], dir %d\n", period_time, dir));

	r = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);
	if (r < 0)
		log_std(("ERROR:sound:alsa: snd_pcm_hw_params_get_period_size: %s\n", snd_strerror(r)));
	else
		log_std(("sound:alsa: hw period_size %u, dir %d\n", (unsigned)period_size, dir));

	r = snd_pcm_hw_params_get_periods(hw_params, &period_count, &dir);
	if (r < 0)
		log_std(("ERROR:sound:alsa: snd_pcm_hw_params_get_periods: %s\n", snd_strerror(r)));
	else
		log_std(("sound:alsa: hw periods %u, dir %d\n", (unsigned)period_count, dir));

	r = snd_pcm_hw_params_get_buffer_time(hw_params, &buffer_time, &dir);
	if (r < 0)
		log_std(("ERROR:sound:alsa: snd_pcm_hw_params_get_buffer_time: %s\n", snd_strerror(r)));
	else
		log_std(("sound:alsa: hw buffer_time %u [us], dir %d\n", buffer_time, dir));

	r = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	if (r < 0)
		log_std(("ERROR:sound:alsa: snd_pcm_hw_params_get_buffer_size: %s\n", snd_strerror(r)));
	else
		log_std(("sound:alsa: hw buffer_size %d\n", (unsigned)buffer_size));
}

adv_error soundb_alsa_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	int r;
	snd_pcm_hw_params_t* hw_params;
	snd_pcm_sw_params_t* sw_params;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t min_buffer_size;
	snd_pcm_uframes_t period_size;
	unsigned period_count;

	log_std(("sound:alsa: soundb_alsa_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", sound_id, *rate, stereo_flag, buffer_time));

	if (!alsa_option.initialized)
		soundb_alsa_default();

	log_std(("sound:alsa: device_alsa_device %s\n", alsa_option.device_buffer));
	log_std(("sound:alsa: device_alsa_mixer %s\n", alsa_option.mixer_buffer));

	alsa_state.volume = ALSA_VOLUME_BASE;

	if (stereo_flag) {
		alsa_state.channel = 2;
	} else {
		alsa_state.channel = 1;
	}

	log_std(("sound:alsa: using device %s\n", alsa_option.device_buffer));

	r = snd_pcm_open(&alsa_state.handle, alsa_option.device_buffer, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (r < 0) {
		error_set("Couldn't open the ALSA device '%s', %s.\n", alsa_option.device_buffer, snd_strerror(r));
		goto err;
	}

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);

	r = snd_pcm_hw_params_any(alsa_state.handle, hw_params);
	if (r < 0) {
		error_set("Couldn't get the ALSA hardware configuration, %s.\n", snd_strerror(r));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_access(alsa_state.handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (r < 0) {
		error_set("Couldn't set the ALSA interleaved access, %s.\n", snd_strerror(r));
		goto err_close;
	}

	/* the samples are in the native endianess */
	r = snd_pcm_hw_params_set_format(alsa_state.handle, hw_params, SND_PCM_FORMAT_S16);
	if (r < 0) {
		error_set("Couldn't set the ALSA 16 bit format, %s.\n", snd_strerror(r));
		goto err_close;
	}

	r = snd_pcm_hw_params_set_channels(alsa_state.handle, hw_params, alsa_state.channel);
	if (r < 0) {
		error_set("Couldn't set %u ALSA channels, %s.\n", alsa_state.channel, snd_strerror(r));
		goto err_close;
	}

	/* in auto mode request the most common rate of the modern hardware, */
	/* a device without it selects the nearest supported rate. */
	/* Note that the pulse and pipewire plugins accept any rate, */
	/* and don't report the rate of the sound server */
	alsa_state.rate = *rate != 0 ? *rate : 48000;
	r = snd_pcm_hw_params_set_rate_near(alsa_state.handle, hw_params, &alsa_state.rate, 0);
	if (r < 0) {
		error_set("Couldn't set the ALSA rate %u, %s.\n", alsa_state.rate, snd_strerror(r));
		goto err_close;
	}
	log_std(("sound:alsa: selected rate %d\n", alsa_state.rate));

	/* compute the buffer and period size */
	/* we arbirarly request 32 periods to give enough granularity */
	/* with less than 16, some jitter may be present */
	period_count = 32;
	buffer_size = alsa_state.rate * buffer_time;
	period_size = buffer_size / period_count;

	log_std(("sound:alsa: request period_size of %d samples\n", (unsigned)period_size));

	r = snd_pcm_hw_params_set_period_size_near(alsa_state.handle, hw_params, &period_size, 0);
	if (r < 0) {
		error_set("Couldn't set the ALSA period size %u, %s.\n", (unsigned)period_size, snd_strerror(r));
		goto err_close;
	} else {
		log_std(("sound:alsa: set_period_size_near() -> %d\n", (unsigned)period_size));
	}

	/* set a minimum buffer size */
	/* this ensures that the next call to "near" won't get a smaller value */
	min_buffer_size = buffer_size;

	/* if the period is bigger than requested, increase also the buffer */
	if (min_buffer_size < period_size * period_count)
		min_buffer_size = period_size * period_count;

	log_std(("sound:alsa: request min_buffer_size of %d samples\n", (unsigned)min_buffer_size));

	r = snd_pcm_hw_params_set_buffer_size_min(alsa_state.handle, hw_params, &min_buffer_size);
	if (r < 0) {
		error_set("Couldn't set the ALSA minimum buffer size %u, %s.\n", (unsigned)min_buffer_size, snd_strerror(r));
		goto err_close;
	} else {
		log_std(("sound:alsa: set_buffer_size_min() -> %d\n", (unsigned)min_buffer_size));
	}

	/* now request the real buffer size */
	log_std(("sound:alsa: request buffer_size of %d samples\n", (unsigned)buffer_size));

	r = snd_pcm_hw_params_set_buffer_size_near(alsa_state.handle, hw_params, &buffer_size);
	if (r < 0) {
		error_set("Couldn't set the ALSA buffer size %u, %s.\n", (unsigned)buffer_size, snd_strerror(r));
		goto err_close;
	} else {
		log_std(("sound:alsa: set_buffer_size_near() -> %d\n", (unsigned)buffer_size));
	}

	if (buffer_size < alsa_state.rate * buffer_time) {
		log_std(("ERROR:sound:alsa: audio buffer TOO SMALL\n"));
	}

	/* store the size for later use */
	alsa_state.buffer_size = buffer_size;
	alsa_state.period_size = period_size;

	r = snd_pcm_hw_params(alsa_state.handle, hw_params);
	if (r < 0) {
		error_set("Couldn't set the ALSA hardware parameters, %s.\n", snd_strerror(r));
		goto err_close;
	}

	r = snd_pcm_sw_params_current(alsa_state.handle, sw_params);
	if (r < 0) {
		error_set("Couldn't get the ALSA software parameters, %s.\n", snd_strerror(r));
		goto err_close;
	}

	r = snd_pcm_sw_params(alsa_state.handle, sw_params);
	if (r < 0) {
		error_set("Couldn't set the ALSA software parameters, %s.\n", snd_strerror(r));
		goto err_close;
	}

	r = snd_pcm_prepare(alsa_state.handle);
	if (r < 0) {
		error_set("Couldn't prepare the ALSA device, %s.\n", snd_strerror(r));
		goto err_close;
	}

	alsa_log(hw_params, sw_params);

	*rate = alsa_state.rate;

	return 0;

err_close:
	snd_pcm_close(alsa_state.handle);
err:
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

/**
 * Write silence directly in the ALSA buffer.
 * Errors are only logged, without any recovery, because this function is
 * also used by the recovery itself.
 */
static void alsa_silence(unsigned count)
{
	static adv_sample silence[2 * 256];

	log_std(("sound:alsa: writing %u samples of silence\n", count));

	while (count) {
		snd_pcm_sframes_t r;
		unsigned run = count;
		if (run > 256)
			run = 256;

		r = snd_pcm_writei(alsa_state.handle, silence, run);
		if (r < 0) {
			log_std(("ERROR:sound:alsa: snd_pcm_writei() of silence failed: %s\n", snd_strerror(r)));
			return;
		}

		count -= r;
	}
}

/**
 * Recover from an error of the ALSA device.
 * \param func Name of the failed function, used for logging.
 * \param err Error returned by the function.
 * \return 0 if the device is recovered.
 */
static adv_error alsa_recover(const char* func, int err)
{
	int r;

	if (err == -EPIPE)
		log_std(("ERROR:sound:alsa: %s() failed: underrun. Increase the latency with -sound_latency.\n", func));
	else
		log_std(("ERROR:sound:alsa: %s() failed: %s\n", func, snd_strerror(err)));

	/* handle both an underrun (-EPIPE) and a system suspend (-ESTRPIPE) */
	r = snd_pcm_recover(alsa_state.handle, err, 1);
	if (r < 0) {
		log_std(("ERROR:sound:alsa: snd_pcm_recover() failed: %s\n", snd_strerror(r)));
		return -1;
	}

	/* after an underrun the buffer is empty, and the playback would restart */
	/* with only the samples of the current frame. This results in more */
	/* underruns until the sync control slowly recovers the latency, so the */
	/* buffer is immediately refilled with silence up to the latency. */
	/* After a suspend the buffer is empty if the device was prepared, but */
	/* it still contains the old samples if the device was resumed, so the */
	/* silence written is only the one missing to reach the latency */
	if (err == -EPIPE || err == -ESTRPIPE) {
		snd_pcm_sframes_t avail;
		unsigned latency;
		unsigned buffered;

		avail = snd_pcm_avail(alsa_state.handle);
		if (avail < 0) {
			log_std(("ERROR:sound:alsa: snd_pcm_avail() after the recover failed: %s\n", snd_strerror(avail)));
			return 0;
		}

		if (avail > alsa_state.buffer_size)
			buffered = 0;
		else
			buffered = alsa_state.buffer_size - avail;

		latency = alsa_state.latency_time * alsa_state.rate;
		if (buffered < latency)
			alsa_silence(latency - buffered);
	}

	return 0;
}

unsigned soundb_alsa_buffered(void)
{
	snd_pcm_sframes_t avail;

	avail = snd_pcm_avail(alsa_state.handle);
	if (avail < 0) {
		/* recover here, and not only at the next write, to report */
		/* to the sync control the real level of the buffer instead of 0 */
		if (alsa_recover("snd_pcm_avail", avail) != 0)
			return 0;

		avail = snd_pcm_avail(alsa_state.handle);
		if (avail < 0) {
			log_std(("ERROR:sound:alsa: snd_pcm_avail() failed: %s\n", snd_strerror(avail)));
			return 0;
		}
	}

	log_debug(("sound:alsa: buffer_size = %d, snd_pcm_avail() = %d, buffered = %d\n", (int)alsa_state.buffer_size, (int)avail, (int)(alsa_state.buffer_size - avail)));

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
			for (c = 0; c <= SND_MIXER_SCHN_LAST; ++c) {
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

			for (c = 0; c <= SND_MIXER_SCHN_LAST; ++c) {
				snd_mixer_selem_set_playback_volume(elem, c, v);
			}
		} else {
			log_std(("sound:alsa: skip set playback volume\n"));
		}
	} else {
		if (snd_mixer_selem_has_playback_switch(elem)) {
			log_std(("sound:alsa: disable playback\n"));
			for (c = 0; c <= SND_MIXER_SCHN_LAST; ++c) {
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
	snd_pcm_sframes_t r;

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

			for (i = 0; i < run; ++i)
				buf_map[i] = (int)sample_map[i] * alsa_state.volume / ALSA_VOLUME_BASE;

			r = snd_pcm_writei(alsa_state.handle, buf_map, run / alsa_state.channel);
		}

		log_debug(("sound:alsa: snd_pcm_writei() -> %d\n", (int)r));

		if (r == -EAGAIN) {
			/* the buffer is full, drop only the samples in excess, keeping */
			/* the ones already stored. Waiting for free space with a retry */
			/* would block the emulation with a busy loop */
			log_std(("ERROR:sound:alsa: buffer overflow, %u samples dropped\n", sample_count));
			break;
		}

		if (r < 0) {
			if (alsa_recover("snd_pcm_writei", r) != 0)
				break;
			/* retry the write */
			continue;
		}

		sample_count -= r;
		sample_map += r * alsa_state.channel;
	}
}

adv_error soundb_alsa_start(double silence_time)
{
	log_std(("sound:alsa: soundb_alsa_start(silence_time:%g)\n", silence_time));

	/* store the latency for the recovery of an underrun. It's not changed */
	/* by a restart of the driver called with 0 */
	if (silence_time != 0)
		alsa_state.latency_time = silence_time;

	alsa_silence(silence_time * alsa_state.rate);

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

