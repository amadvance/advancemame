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

#include "inputdrv.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

struct inputb_state_struct inputb_state;

void inputb_default(void) {
	inputb_state.is_initialized_flag = 1;
	strcpy(inputb_state.name, "none");
}

void inputb_reg(struct conf_context* context, video_bool auto_detect) {
	conf_string_register_default(context, "device_input", auto_detect ? "auto" : "none");
}

void inputb_reg_driver(struct conf_context* context, inputb_driver* driver) {
	assert( inputb_state.driver_mac < INPUT_DRIVER_MAX );

	inputb_state.driver_map[inputb_state.driver_mac] = driver;
	inputb_state.driver_map[inputb_state.driver_mac]->reg(context);

	log_std(("inputb: register driver %s\n", driver->name));

	++inputb_state.driver_mac;
}

video_error inputb_load(struct conf_context* context) {
	unsigned i;
	int at_least_one;

	if (inputb_state.driver_mac == 0) {
		return -1;
	}

	inputb_state.is_initialized_flag = 1;
	strcpy(inputb_state.name, conf_string_get_default(context, "device_input"));

	/* load specific driver options */
	at_least_one = 0;
	for(i=0;i<inputb_state.driver_mac;++i) {
		const device* dev;

		dev = device_match(inputb_state.name, (driver*)inputb_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (inputb_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_input",inputb_state.name,(const driver**)inputb_state.driver_map,inputb_state.driver_mac);
		return -1;
	}

	return 0;
}

video_error inputb_init(void) {
	unsigned i;

	assert(inputb_state.driver_current == 0);

	assert(!inputb_state.is_active_flag);

	if (!inputb_state.is_initialized_flag) {
		inputb_default();
	}

	for(i=0;i<inputb_state.driver_mac;++i) {
		const device* dev;

		dev = device_match(inputb_state.name, (const driver*)inputb_state.driver_map[i], 0);

		if (dev && inputb_state.driver_map[i]->init(dev->id) == 0) {
			inputb_state.driver_current = inputb_state.driver_map[i];
			break;
		}
	}

	if (!inputb_state.driver_current)
		return -1;

	log_std(("inputb: select driver %s\n", inputb_state.driver_current->name));

	inputb_state.is_active_flag = 1;

	return 0;
}

void inputb_done(void) {
	assert( inputb_state.driver_current );
	assert( inputb_state.is_active_flag );

	inputb_state.driver_current->done();

	inputb_state.driver_current = 0;
	inputb_state.is_active_flag = 0;
}

void inputb_abort(void) {
	if (inputb_state.is_active_flag) {
		inputb_done();
	}
}

