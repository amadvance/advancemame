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

#include "snone.h"
#include "log.h"

struct soundb_none_context {
	unsigned rate;
	unsigned latency;
};

static struct soundb_none_context none_state;

static adv_device DEVICE[] = {
{ "auto", 0, "No sound" },
{ 0, 0, 0 }
};

int soundb_none_init(int device_id, unsigned* rate, int stereo_flag, double buffer_time)
{
	log_std(("sound: soundb_none(id:%d, rate:%d, stereo:%d, buffer_time:%g)\n", device_id, *rate, stereo_flag, buffer_time));

	none_state.rate = *rate;
	none_state.latency = 0;

	return 0;
}

void soundb_none_done(void)
{
	log_std(("sound: soundb_none_done()\n"));
}

void soundb_none_stop(void)
{
	log_std(("sound: soundb_none_stop()\n"));
}

unsigned soundb_none_buffered(void)
{
	return none_state.latency - none_state.latency / 8;
}

int soundb_none_start(double silence_time)
{
	log_std(("sound: soundb_none_start(silence_time:%g)\n", silence_time));
	none_state.latency = silence_time * none_state.rate;
	return 0;
}

void soundb_none_volume(double volume)
{
	log_std(("sound: soundb_none_volume(volume:%g)\n", (double)volume));
}

void soundb_none_play(const short* sample_map, unsigned sample_count)
{
	log_debug(("sound: soundb_none_play(count:%d)\n", sample_count));
}

unsigned soundb_none_flags(void)
{
	return 0;
}

int soundb_none_load(adv_conf* context)
{
	return 0;
}

void soundb_none_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

soundb_driver soundb_none_driver = {
	"none",
	DEVICE,
	soundb_none_load,
	soundb_none_reg,
	soundb_none_init,
	soundb_none_done,
	soundb_none_flags,
	soundb_none_play,
	soundb_none_buffered,
	soundb_none_start,
	soundb_none_stop,
	soundb_none_volume
};


