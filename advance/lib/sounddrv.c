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

#include "sounddrv.h"
#include "log.h"
#include "error.h"
#include "snstring.h"

struct soundb_state_struct soundb_state;

void soundb_default(void)
{
	soundb_state.is_initialized_flag = 1;
	sncpy(soundb_state.name, DEVICE_NAME_MAX, "auto");
}

void soundb_reg(adv_conf* context, adv_bool auto_detect)
{
	conf_string_register_default(context, "device_sound", auto_detect ? "auto" : "none");
}

void soundb_reg_driver(adv_conf* context, soundb_driver* driver)
{
	assert(soundb_state.driver_mac < SOUND_DRIVER_MAX);

	soundb_state.driver_map[soundb_state.driver_mac] = driver;
	soundb_state.driver_map[soundb_state.driver_mac]->reg(context);

	log_std(("sound: register driver %s\n", driver->name));

	++soundb_state.driver_mac;
}

adv_error soundb_load(adv_conf* context)
{
	unsigned i;
	int at_least_one;

	if (soundb_state.driver_mac == 0) {
		error_set("No sound driver was compiled in.");
		return -1;
	}

	soundb_state.is_initialized_flag = 1;
	sncpy(soundb_state.name, DEVICE_NAME_MAX, conf_string_get_default(context, "device_sound"));

	/* load specific driver options */
	at_least_one = 0;
	for(i=0;i<soundb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(soundb_state.name, (adv_driver*)soundb_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (soundb_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_sound", soundb_state.name, (const adv_driver**)soundb_state.driver_map, soundb_state.driver_mac);
		return -1;
	}

	return 0;
}

adv_error soundb_init(unsigned* rate, adv_bool stereo_flag, double buffer_time)
{
	unsigned i;

	assert(soundb_state.driver_current == 0);
	assert(!soundb_state.is_active_flag);

	soundb_state.is_playing_flag = 0;

	if (!soundb_state.is_initialized_flag) {
		soundb_default();
	}

	/* store the error prefix */
	error_nolog_set("Unable to initialize the sound driver. The errors are:\n");

	for(i=0;i<soundb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(soundb_state.name, (const adv_driver*)soundb_state.driver_map[i], 1);

		error_cat_set(soundb_state.driver_map[i]->name, 1);

		if (dev && soundb_state.driver_map[i]->init(dev->id, rate, stereo_flag, buffer_time) == 0) {
			soundb_state.driver_current = soundb_state.driver_map[i];
			break;
		}
	}

	error_cat_set(0, 0);

	if (!soundb_state.driver_current)
		return -1;

	error_reset();

	log_std(("sound: select driver %s\n", soundb_state.driver_current->name));

	soundb_state.is_active_flag = 1;

	return 0;
}

void soundb_done(void)
{
	assert(soundb_state.driver_current);
	assert(soundb_state.is_active_flag);

	soundb_state.driver_current->done();

	soundb_state.driver_current = 0;
	soundb_state.is_active_flag = 0;
}

void soundb_abort(void)
{
	if (soundb_state.is_active_flag) {
		if (soundb_state.is_playing_flag)
			soundb_stop();
		soundb_done();
	}
}

void soundb_play(const adv_sample* sample_map, unsigned sample_count)
{
	assert(soundb_state.is_active_flag && soundb_state.is_playing_flag);

	soundb_state.driver_current->play(sample_map, sample_count);
}

unsigned soundb_buffered(void)
{
	assert(soundb_state.is_active_flag && soundb_state.is_playing_flag);

	return soundb_state.driver_current->buffered();
}

void soundb_stop(void)
{
	assert(soundb_state.is_active_flag && soundb_state.is_playing_flag);

	soundb_state.driver_current->stop();

	soundb_state.is_playing_flag = 0;
}

adv_error soundb_start(double silence_time)
{
	assert(soundb_state.is_active_flag && !soundb_state.is_playing_flag);

	if (soundb_state.driver_current->start(silence_time) != 0)
		return -1;

	soundb_state.is_playing_flag = 1;

	return 0;
}

void soundb_volume(double v)
{
	assert(soundb_state.is_active_flag);

	if (soundb_state.driver_current->volume)
		soundb_state.driver_current->volume(v);
}

const char* soundb_name(void)
{
	assert(soundb_state.is_active_flag);

	return soundb_state.driver_current->name;
}

unsigned soundb_flags(void)
{
	assert(soundb_state.is_active_flag);

	if (soundb_state.driver_current->flags)
		return soundb_state.driver_current->flags();
	else
		return 0;
}

adv_bool soundb_is_active(void)
{
	return soundb_state.is_active_flag != 0;
}

adv_bool soundb_is_playing(void)
{
	assert(soundb_is_active());

	return soundb_state.is_playing_flag != 0;
}

