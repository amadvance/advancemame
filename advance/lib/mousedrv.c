/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005 Andrea Mazzoleni
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

#include "mousedrv.h"
#include "log.h"
#include "error.h"
#include "snstring.h"
#include "mnone.h"

struct mouseb_state_struct mouseb_state;

void mouseb_default(void)
{
	mouseb_state.is_initialized_flag = 1;
	sncpy(mouseb_state.name, DEVICE_NAME_MAX, "none");
}

void mouseb_reg(adv_conf* context, adv_bool auto_detect)
{
	conf_string_register_default(context, "device_mouse", auto_detect ? "auto" : "none");
}

void mouseb_reg_driver(adv_conf* context, mouseb_driver* driver)
{
	assert(mouseb_state.driver_mac < MOUSE_DRIVER_MAX);

	mouseb_state.driver_map[mouseb_state.driver_mac] = driver;
	mouseb_state.driver_map[mouseb_state.driver_mac]->reg(context);

	log_std(("mouseb: register driver %s\n", driver->name));

	++mouseb_state.driver_mac;
}

adv_error mouseb_load(adv_conf* context)
{
	unsigned i;
	int at_least_one;

	if (mouseb_state.driver_mac == 0) {
		error_set("No mouse driver was compiled in.");
		return -1;
	}

	mouseb_state.is_initialized_flag = 1;
	sncpy(mouseb_state.name, DEVICE_NAME_MAX, conf_string_get_default(context, "device_mouse"));

	/* load specific driver options */
	at_least_one = 0;
	for(i=0;i<mouseb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(mouseb_state.name, (adv_driver*)mouseb_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (mouseb_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_mouse", mouseb_state.name, (const adv_driver**)mouseb_state.driver_map, mouseb_state.driver_mac);
		return -1;
	}

	return 0;
}

adv_error mouseb_init(void)
{
	unsigned i;

	assert(mouseb_state.driver_current == 0);
	assert(!mouseb_state.is_active_flag);

	if (!mouseb_state.is_initialized_flag) {
		mouseb_default();
	}

	/* store the error prefix */
	error_nolog_set("Unable to initialize the mouse driver. The errors are:\n");

	for(i=0;i<mouseb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(mouseb_state.name, (const adv_driver*)mouseb_state.driver_map[i], 1);

		error_cat_set(mouseb_state.driver_map[i]->name, 1);

		if (dev && mouseb_state.driver_map[i]->init(dev->id) == 0) {
			mouseb_state.driver_current = mouseb_state.driver_map[i];
			break;
		}
	}

	error_cat_set(0, 0);

	if (!mouseb_state.driver_current)
		return -1;

	error_reset();

	log_std(("mouseb: select driver %s\n", mouseb_state.driver_current->name));

	mouseb_state.is_active_flag = 1;
	mouseb_state.is_enabled_flag = 0;

	return 0;
}

void mouseb_done(void)
{
	assert(mouseb_state.driver_current);
	assert(mouseb_state.is_active_flag);

	mouseb_state.driver_current->done();

	mouseb_state.driver_current = 0;
	mouseb_state.is_active_flag = 0;
}

adv_error mouseb_enable(void) 
{
	assert(mouseb_state.is_active_flag && !mouseb_state.is_enabled_flag);

	if (mouseb_state.driver_current->enable 
		&& mouseb_state.driver_current->enable() != 0)
		return -1;

	mouseb_state.is_enabled_flag = 1;

	return 0;
}

void mouseb_disable(void)
{
	assert(mouseb_state.is_active_flag && mouseb_state.is_enabled_flag);

	if (mouseb_state.driver_current->disable)
		mouseb_state.driver_current->disable();

	mouseb_state.is_enabled_flag = 0;
}

void mouseb_init_null(void)
{
	assert(mouseb_state.driver_current == 0);
	assert(!mouseb_state.is_active_flag);

	if (!mouseb_state.is_initialized_flag) {
		mouseb_default();
	}

	mouseb_state.driver_current = &mouseb_none_driver;

	mouseb_state.driver_current->init(-1); /* it must never fail */

	mouseb_state.is_active_flag = 1;
}

void mouseb_abort(void)
{
	if (mouseb_state.is_active_flag) {
		mouseb_done();
	}
}

unsigned mouseb_count_get(void)
{
	assert(mouseb_state.is_active_flag);

	return mouseb_state.driver_current->count_get();
}

unsigned mouseb_axe_count_get(unsigned mouse)
{
	assert(mouseb_state.is_active_flag);
	assert(mouse < mouseb_count_get());

	return mouseb_state.driver_current->axe_count_get(mouse);
}

unsigned mouseb_button_count_get(unsigned mouse)
{
	assert(mouseb_state.is_active_flag);
	assert(mouse < mouseb_count_get());

	return mouseb_state.driver_current->button_count_get(mouse);
}

int mouseb_axe_get(unsigned mouse, unsigned axe)
{
	assert(mouseb_state.is_active_flag);
	assert(mouse < mouseb_count_get());
	assert(axe < mouseb_axe_count_get(mouse));

	return mouseb_state.driver_current->axe_get(mouse, axe);
}

unsigned mouseb_button_get(unsigned mouse, unsigned button)
{
	assert(mouseb_state.is_active_flag);
	assert(mouse < mouseb_count_get());
	assert(button < mouseb_button_count_get(mouse));

	return mouseb_state.driver_current->button_get(mouse, button);
}

const char* mouseb_button_name_get(unsigned mouse, unsigned button)
{
	assert(mouseb_state.is_active_flag);
	assert(mouse < mouseb_count_get());
	assert(button < mouseb_button_count_get(mouse));

	if (mouseb_state.driver_current->button_name_get)
		return mouseb_state.driver_current->button_name_get(mouse, button);

	switch (button) {
	case 0 : snprintf(mouseb_state.button_name_buffer, sizeof(mouseb_state.button_name_buffer), "left"); break;
	case 1 : snprintf(mouseb_state.button_name_buffer, sizeof(mouseb_state.button_name_buffer), "right"); break;
	case 2 : snprintf(mouseb_state.button_name_buffer, sizeof(mouseb_state.button_name_buffer), "middle"); break;
	default: snprintf(mouseb_state.button_name_buffer, sizeof(mouseb_state.button_name_buffer), "button%d", button+1);
	}

	return mouseb_state.button_name_buffer;
}

const char* mouseb_axe_name_get(unsigned mouse, unsigned axe)
{
	assert(mouseb_state.is_active_flag);
	assert(mouse < mouseb_count_get());
	assert(axe < mouseb_axe_count_get(mouse));

	if (mouseb_state.driver_current->axe_name_get)
		return mouseb_state.driver_current->axe_name_get(mouse, axe);

	switch (axe) {
	case 0 : snprintf(mouseb_state.axe_name_buffer, sizeof(mouseb_state.axe_name_buffer), "x"); break;
	case 1 : snprintf(mouseb_state.axe_name_buffer, sizeof(mouseb_state.axe_name_buffer), "y"); break;
	case 2 : snprintf(mouseb_state.axe_name_buffer, sizeof(mouseb_state.axe_name_buffer), "z"); break;
	default: snprintf(mouseb_state.axe_name_buffer, sizeof(mouseb_state.axe_name_buffer), "axe%d", axe+1);
	}

	return mouseb_state.axe_name_buffer;
}

void mouseb_poll(void)
{
	assert(mouseb_state.is_active_flag);

	mouseb_state.driver_current->poll();
}

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
const char* mouseb_name(void)
{
	assert(mouseb_state.is_active_flag);

	return mouseb_state.driver_current->name;
}

