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

#include "sounddrv.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void sound_default(void) {
	sound_state.is_initialized_flag = 1;
	strcpy(sound_state.name, "auto");
}

void sound_reg(struct conf_context* context, adv_bool auto_detect) {
	conf_string_register_default(context, "device_sound", auto_detect ? "auto" : "none");
}

void sound_reg_driver(struct conf_context* context, sound_driver* driver) {
	assert( sound_state.driver_mac < SOUND_DRIVER_MAX );

	sound_state.driver_map[sound_state.driver_mac] = driver;
	sound_state.driver_map[sound_state.driver_mac]->reg(context);

	log_std(("sound: register driver %s\n", driver->name));

	++sound_state.driver_mac;
}

adv_error sound_load(struct conf_context* context) {
	unsigned i;
	int at_least_one;

	if (sound_state.driver_mac == 0) {
		return -1;
	}

	sound_state.is_initialized_flag = 1;
	strcpy(sound_state.name, conf_string_get_default(context, "device_sound"));

	/* load specific driver options */
	at_least_one = 0;
	for(i=0;i<sound_state.driver_mac;++i) {
		const device* dev;

		dev = device_match(sound_state.name, (driver*)sound_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (sound_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_sound",sound_state.name,(const driver**)sound_state.driver_map,sound_state.driver_mac);
		return -1;
	}

	return 0;
}

adv_error sound_init(unsigned* rate, adv_bool stereo_flag, double buffer_time) {
	unsigned i;

	assert(sound_state.driver_current == 0);

	assert( !sound_state.is_active_flag );

	sound_state.is_playing_flag = 0;

	if (!sound_state.is_initialized_flag) {
		sound_default();
	}

	/* store the error prefix */
	error_description_nolog_set("Unable to inizialize a sound driver. The following are the errors:\n");

	for(i=0;i<sound_state.driver_mac;++i) {
		const device* dev;

		dev = device_match(sound_state.name,(const driver*)sound_state.driver_map[i], 1);

		if (dev && sound_state.driver_map[i]->init(dev->id,rate,stereo_flag,buffer_time) == 0) {
			sound_state.driver_current = sound_state.driver_map[i];
			break;
		}
	}

	if (!sound_state.driver_current)
		return -1;

	log_std(("sound: select driver %s\n", sound_state.driver_current->name));

	sound_state.is_active_flag = 1;

	return 0;
}

void sound_done(void) {
	assert( sound_state.driver_current );
	assert( sound_state.is_active_flag );

	sound_state.driver_current->done();

	sound_state.driver_current = 0;
	sound_state.is_active_flag = 0;
}

void sound_abort(void) {
	if (sound_state.is_active_flag) {
		if (sound_state.is_playing_flag)
			sound_stop();
		sound_done();
	}
}

