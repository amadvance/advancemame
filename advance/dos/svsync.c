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

#include "svsync.h"
#include "sounddrv.h"
#include "log.h"
#include "target.h"

#include <dos.h>

#include "wss.h"
#include "wss.c"

struct soundb_vsync_context {
	unsigned channel;
	unsigned rate;
};

static struct soundb_vsync_context vsync_state;

static adv_device DEVICE[] = {
{ "sb", 1, "Sound Blaster" },
{ "sbwin", 9, "Sound Blaster (Windows)" },
{ "ac97", 3, "AC97" },
{ "ac97win", 2, "AC97 (Windows)" },
{ "gusmax", 4, "Gravis Ultrasound Max" },
{ "gus", 5, "Gravis Ultrasound" },
{ "audio", 8, "Ensoniq AudioDrive" },
{ "wss", 6, "Windows Sound System" },
{ "ess", 7, "Ensoniq Soundscape" },
{ 0, 0, 0 }
};

adv_error soundb_vsync_init(int device_id, unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	log_std(("sound:vsync: soundb_vsync_init(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", device_id, *rate, stereo_flag, buffer_time));

	/* ensure to be able to store the required buffer time */
	buffer_time *= 2;

	if (stereo_flag) {
		vsync_state.channel = 2;
	} else {
		vsync_state.channel = 1;
	}

	log_std(("sound:vsync: w_sound_device_init(%d, %d)\n", device_id, *rate));
	if (w_sound_device_init(device_id, *rate) == 0) {
		log_std(("sound:vsync: w_sound_device_init() failed\n"));
		return -1;
	}

	vsync_state.rate = w_get_nominal_sample_rate();
	*rate = vsync_state.rate;

	log_std(("sound:vsync: rate %d\n", vsync_state.rate));

	if (w_get_device_name()) {
		log_std(("sound:vsync: device %s\n", w_get_device_name()));
	}

	return 0;
}

void soundb_vsync_done(void)
{
	log_std(("sound:vsync: soundb_vsync_done()\n"));

	log_std(("sound:vsync: w_sound_device_exit()\n"));
	w_sound_device_exit();
}

void soundb_vsync_stop(void)
{
	log_std(("sound:vsync: soundb_vsync_stop()\n"));

	w_set_master_volume(0);
	w_clear_buffer();
}

unsigned soundb_vsync_buffered(void)
{
	return w_get_latency();
}

adv_error soundb_vsync_start(double silence_time)
{
	unsigned sample_count;

	log_std(("sound:vsync: soundb_vsync_start(silence_time:%g)\n", silence_time));

	sample_count = silence_time * vsync_state.rate;

	if (sample_count) {
		w_lock_mixing_buffer(sample_count);
		w_mixing_zero();
		w_unlock_mixing_buffer();
	}

	w_set_master_volume(255);

	return 0;
}

void soundb_vsync_volume(double volume)
{
	int v;

	log_std(("sound:vsync: soundb_vsync_volume(volume:%g)\n", (double)volume));

	v = volume * 255;
	if (v < 0)
		v = 0;
	if (v > 255)
		v = 255;

	log_std(("sound:vsync: w_set_master_volume(%d)\n", v));
	w_set_master_volume(v);
}

void soundb_vsync_play(const adv_sample* sample_map, unsigned sample_count)
{
	unsigned count = sample_count;

	log_debug(("sound:vsync: soundb_vsync_play(count:%d)\n", sample_count));

	if (sample_count) {
		w_lock_mixing_buffer(sample_count);
		if (vsync_state.channel == 2)
			w_mixing_stereo((short*)sample_map, sample_count, 256, 256);
		else
			w_mixing((short*)sample_map, sample_count, 256, 256);
		w_unlock_mixing_buffer();
	}
}

unsigned soundb_vsync_flags(void)
{
	return 0;
}

adv_error soundb_vsync_load(adv_conf* context)
{
	return 0;
}

void soundb_vsync_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_vsync_driver = {
	"vsync",
	DEVICE,
	soundb_vsync_load,
	soundb_vsync_reg,
	soundb_vsync_init,
	soundb_vsync_done,
	soundb_vsync_flags,
	soundb_vsync_play,
	soundb_vsync_buffered,
	soundb_vsync_start,
	soundb_vsync_stop,
	soundb_vsync_volume
};
