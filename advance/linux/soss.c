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

#include "soss.h"
#include "log.h"
#include "error.h"

#include <sys/soundcard.h>

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

/**
 * Base for the volume adjustment.
 */
#define OSS_VOLUME_BASE 32768

struct soundb_oss_context {
	unsigned channel;
	unsigned rate;
	unsigned sample_length;
	int handle;
	int volume; /**< Volume adjustement. OSS_VOLUME_BASE == full volume. */
};

static struct soundb_oss_context oss_state;

static adv_device DEVICE[] = {
{ "auto", -1, "OSS automatic detection" },
{ 0, 0, 0 }
};

adv_error soundb_oss_init(int sound_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	int i;
	audio_buf_info info;

	log_std(("sound:oss: soundb_oss_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", sound_id, *rate, stereo_flag, buffer_time));

	if (stereo_flag) {
		oss_state.sample_length = 4;
		oss_state.channel = 2;
	} else {
		oss_state.sample_length = 2;
		oss_state.channel = 1;
	}

	oss_state.volume = OSS_VOLUME_BASE;

	oss_state.handle = open("/dev/dsp", O_WRONLY | O_NONBLOCK, 0);
	if (!oss_state.handle) {
		log_std(("sound:oss: open(/dev/dsp, O_WRONLY | O_NONBLOCK, 0) failed\n"));
		goto err;
	}

	i = AFMT_S16_LE;
	if (ioctl(oss_state.handle, SNDCTL_DSP_SETFMT, &i) < 0) {
		log_std(("sound:oss: ioctl(SNDCTL_DSP_SETFMT, AFMT_S16_LE) failed\n"));
		goto err_close;
	}
	if (i != AFMT_S16_LE) {
		log_std(("sound:oss: ioctl(SNDCTL_DSP_SETFMT, AFMT_S16_LE) return a different format\n"));
		goto err_close;
	}

	if (stereo_flag)
		i = 2;
	else
		i = 1;
	if (ioctl(oss_state.handle, SNDCTL_DSP_CHANNELS, &i) < 0) {
		log_std(("sound:oss: ioctl(SNDCTL_DSP_CHANNELS, %d) failed\n", i));
		goto err_close;
	}

	i = *rate;
	if (ioctl(oss_state.handle, SNDCTL_DSP_SPEED, &i) < 0) {
		log_std(("sound:oss: ioctl(SNDCTL_DSP_SPEED, %d) failed\n", i));
		goto err_close;
	}
	oss_state.rate = i;

	i = 8;
	if (stereo_flag)
		++i;
	i |= 0x7fff << 16; /* do not limit the number of framents, ignore the buffer_time arg */
	if (ioctl(oss_state.handle, SNDCTL_DSP_SETFRAGMENT, &i) < 0) {
		log_std(("sound:oss: ioctl(SNDCTL_DSP_SETFRAGMENT, %d:%d) failed\n", i >> 16, i & 0xFFFF));
		goto err_close;
	}

	if (ioctl(oss_state.handle, SNDCTL_DSP_GETOSPACE, &info) < 0) {
		log_std(("sound:oss: ioctl(SNDCTL_DSP_GETOSPACE) failed\n"));
		goto err_close;
	}

	log_std(("sound:oss: sound getospace() = fragsize %d, fragtotal %d, freebytes %d\n", (int)info.fragsize, (int)info.fragstotal, (int)info.bytes));

	*rate = oss_state.rate;

	return 0;

err_close:
	close(oss_state.handle);
err:
	error_set("Error initializing the OSS interface.\n");
	return -1;
}

void soundb_oss_done(void)
{
	log_std(("sound:oss: soundb_oss_done()\n"));

	close(oss_state.handle);
}

void soundb_oss_stop(void)
{
	log_std(("sound:oss: soundb_oss_stop()\n"));
}

unsigned soundb_oss_buffered(void)
{
	unsigned bytes_buffered;
	audio_buf_info info;

	if (ioctl(oss_state.handle, SNDCTL_DSP_GETOSPACE, &info) < 0) {
		log_std(("ERROR:sound:oss: ioctl(SNDCTL_DSP_GETOSPACE) failed\n"));
		return 0;
	}

	log_debug(("sound:oss: getospace() = fragsize %d, fragtotal %d, freebytes %d\n", (int)info.fragsize, (int)info.fragstotal, (int)info.bytes));

	bytes_buffered = info.fragstotal * info.fragsize - info.bytes;

	return bytes_buffered / oss_state.sample_length;
}

void soundb_oss_volume(double volume)
{
	log_std(("sound:oss: soundb_oss_volume(volume:%g)\n", (double)volume));

	oss_state.volume = volume * OSS_VOLUME_BASE;
	if (oss_state.volume < 0)
		oss_state.volume = 0;
	if (oss_state.volume > OSS_VOLUME_BASE)
		oss_state.volume = OSS_VOLUME_BASE;
}

void soundb_oss_play(const adv_sample* sample_map, unsigned sample_count)
{
	int r;

	log_debug(("sound:oss: soundb_oss_play(count:%d)\n", sample_count));

	/* calling write with a 0 size result in wrong output */
	while (sample_count) {
		unsigned channel_length = oss_state.sample_length / oss_state.channel;

		if (oss_state.volume == OSS_VOLUME_BASE) {
			r = write(oss_state.handle, sample_map, sample_count * oss_state.sample_length);
		} else {
			/* adjust the volume and write */
			const unsigned buf_size = 2048;
			adv_sample buf_map[buf_size];
			unsigned run;
			unsigned i;

			run = sample_count * oss_state.channel;
			if (run > buf_size)
				run = buf_size;

			for(i=0;i<run;++i)
				buf_map[i] = (int)sample_map[i] * oss_state.volume / OSS_VOLUME_BASE;

			r = write(oss_state.handle, buf_map, run * channel_length);
		}

		if (r < 0) {
			if (errno == EAGAIN) {
				/* audio buffer full, it should never happen */
				log_std(("WARNING:sound:oss: write() failed: internal buffer full\n"));
				/* retry */
				continue;
			}

			break;
		} else {
			sample_count -= r / oss_state.sample_length;
			sample_map += r / channel_length;
		}
	}
}

adv_error soundb_oss_start(double silence_time)
{
	adv_sample buf[256];
	unsigned sample;
	unsigned i;

	log_std(("sound:oss: soundb_oss_start(silence_time:%g)\n", silence_time));

	for(i=0;i<256;++i)
		buf[i] = 0x0;

	sample = silence_time * oss_state.rate * oss_state.channel;

	log_std(("sound:oss: writing %d bytes, %d sample of silence\n", sample / oss_state.channel * oss_state.sample_length, sample / oss_state.channel));

	while (sample) {
		unsigned run = sample;
		if (run > 256)
			run = 256;
		sample -= run;
		soundb_oss_play(buf, run / oss_state.channel);
	}

	return 0;
}

unsigned soundb_oss_flags(void)
{
	return SOUND_DRIVER_FLAGS_VOLUME_SAMPLE;
}

adv_error soundb_oss_load(adv_conf* context)
{
	return 0;
}

void soundb_oss_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_oss_driver = {
	"oss",
	DEVICE,
	soundb_oss_load,
	soundb_oss_reg,
	soundb_oss_init,
	soundb_oss_done,
	soundb_oss_flags,
	soundb_oss_play,
	soundb_oss_buffered,
	soundb_oss_start,
	soundb_oss_stop,
	soundb_oss_volume
};


