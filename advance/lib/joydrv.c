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

#include "joydrv.h"
#include "log.h"
#include "error.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct joystickb_state_struct joystickb_state;

void joystickb_default(void)
{
	joystickb_state.is_initialized_flag = 1;
	strcpy(joystickb_state.name, "none");
}

void joystickb_reg(adv_conf* context, adv_bool auto_detect)
{
	conf_string_register_default(context, "device_joystick", auto_detect ? "auto" : "none");
}

void joystickb_reg_driver(adv_conf* context, joystickb_driver* driver)
{
	assert( joystickb_state.driver_mac < JOYSTICK_DRIVER_MAX );

	joystickb_state.driver_map[joystickb_state.driver_mac] = driver;
	joystickb_state.driver_map[joystickb_state.driver_mac]->reg(context);

	log_std(("joystickb: register driver %s\n", driver->name));

	++joystickb_state.driver_mac;
}

adv_error joystickb_load(adv_conf* context)
{
	unsigned i;
	int at_least_one;

	if (joystickb_state.driver_mac == 0) {
		return -1;
	}

	joystickb_state.is_initialized_flag = 1;
	strcpy(joystickb_state.name, conf_string_get_default(context, "device_joystick"));

	/* load specific driver options */
	at_least_one = 0;
	for(i=0;i<joystickb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(joystickb_state.name, (adv_driver*)joystickb_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (joystickb_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_joystick", joystickb_state.name, (const adv_driver**)joystickb_state.driver_map, joystickb_state.driver_mac);
		return -1;
	}

	return 0;
}

adv_error joystickb_init(void)
{
	unsigned i;

	assert(joystickb_state.driver_current == 0);

	assert(!joystickb_state.is_active_flag);

	if (!joystickb_state.is_initialized_flag) {
		joystickb_default();
	}

	/* store the error prefix */
	error_nolog_set("Unable to inizialize a joystick driver. The following are the errors:\n");

	for(i=0;i<joystickb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(joystickb_state.name, (const adv_driver*)joystickb_state.driver_map[i], 1);

		if (dev && joystickb_state.driver_map[i]->init(dev->id) == 0) {
			joystickb_state.driver_current = joystickb_state.driver_map[i];
			break;
		}
	}

	if (!joystickb_state.driver_current)
		return -1;

	log_std(("joystickb: select driver %s\n", joystickb_state.driver_current->name));

	joystickb_state.is_active_flag = 1;

	return 0;
}

void joystickb_done(void)
{
	assert( joystickb_state.driver_current );
	assert( joystickb_state.is_active_flag );

	joystickb_state.driver_current->done();

	joystickb_state.driver_current = 0;
	joystickb_state.is_active_flag = 0;
}

void joystickb_abort(void)
{
	if (joystickb_state.is_active_flag) {
		joystickb_done();
	}
}

