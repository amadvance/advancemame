/*
 * This file is part of the AdvanceMAME project.
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

#include "snone.h"
#include "os.h"

#include <assert.h>

struct sound_none_context {
	unsigned rate;
	unsigned latency;
};

static struct sound_none_context none_state;

static device DEVICE[] = {
{ "auto", 0, "No sound" },
{ 0, 0, 0 }
};

int sound_none_init(int device_id, unsigned* rate, int stereo_flag, double buffer_time) {
	log_std(("sound: sound_none(id:%d,rate:%d,stereo:%d,buffer_time:%g)\n",device_id,*rate,stereo_flag,buffer_time));

	none_state.rate = *rate;
	none_state.latency = 0;

	return 0;
}

void sound_none_done(void) {
	log_std(("sound: sound_none_done()\n"));
}

void sound_none_stop(void) {
	log_std(("sound: sound_none_stop()\n"));
}

unsigned sound_none_buffered(void) {
	return none_state.latency - none_state.latency / 8;
}

int sound_none_start(double silence_time) {
	log_std(("sound: sound_none_start(silence_time:%g)\n",silence_time));
	none_state.latency = silence_time * none_state.rate;
	return 0;
}

void sound_none_volume(double volume) {
	log_std(("sound: sound_none_volume(volume:%g)\n",(double)volume));
}

void sound_none_play(const short* sample_map, unsigned sample_count) {
	log_std(("sound: sound_none_play(count:%d)\n",sample_count));
}

unsigned sound_none_flags(void) {
	return 0;
}

int sound_none_load(struct conf_context* context) {
	return 0;
}

void sound_none_reg(struct conf_context* context) {
}

/***************************************************************************/
/* Driver */

sound_driver sound_none_driver = {
	"none",
	DEVICE,
	sound_none_load,
	sound_none_reg,
	sound_none_init,
	sound_none_done,
	sound_none_flags,
	sound_none_play,
	sound_none_buffered,
	sound_none_start,
	sound_none_stop,
	sound_none_volume
};


