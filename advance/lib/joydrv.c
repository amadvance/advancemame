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

#include "joydrv.h"
#include "log.h"
#include "error.h"
#include "snstring.h"
#include "jnone.h"

struct joystickb_state_struct joystickb_state;

void joystickb_default(void)
{
	joystickb_state.is_initialized_flag = 1;
	sncpy(joystickb_state.name, DEVICE_NAME_MAX, "none");
}

void joystickb_reg(adv_conf* context, adv_bool auto_detect)
{
	conf_string_register_default(context, "device_joystick", auto_detect ? "auto" : "none");
}

void joystickb_reg_driver(adv_conf* context, joystickb_driver* driver)
{
	assert(joystickb_state.driver_mac < JOYSTICK_DRIVER_MAX);

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
		error_set("No joystick driver was compiled in.");
		return -1;
	}

	joystickb_state.is_initialized_flag = 1;
	sncpy(joystickb_state.name, DEVICE_NAME_MAX, conf_string_get_default(context, "device_joystick"));

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
	error_nolog_set("Unable to initialize the joystick driver. The errors are:\n");

	for(i=0;i<joystickb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(joystickb_state.name, (const adv_driver*)joystickb_state.driver_map[i], 1);

		error_cat_set(joystickb_state.driver_map[i]->name, 1);

		if (dev && joystickb_state.driver_map[i]->init(dev->id) == 0) {
			joystickb_state.driver_current = joystickb_state.driver_map[i];
			break;
		}
	}

	error_cat_set(0, 0);

	if (!joystickb_state.driver_current)
		return -1;

	error_reset();

	log_std(("joystickb: select driver %s\n", joystickb_state.driver_current->name));

	joystickb_state.is_active_flag = 1;
	joystickb_state.is_enabled_flag = 0;

	return 0;
}

void joystickb_done(void)
{
	assert(joystickb_state.driver_current);
	assert(joystickb_state.is_active_flag);

	joystickb_state.driver_current->done();

	joystickb_state.driver_current = 0;
	joystickb_state.is_active_flag = 0;
}

adv_error joystickb_enable(void) 
{
	assert(joystickb_state.is_active_flag && !joystickb_state.is_enabled_flag);

	if (joystickb_state.driver_current->enable
		&& joystickb_state.driver_current->enable() != 0)
		return -1;

	joystickb_state.is_enabled_flag = 1;

	return 0;
}

void joystickb_disable(void)
{
	assert(joystickb_state.is_active_flag && joystickb_state.is_enabled_flag);

	if (joystickb_state.driver_current->disable)
		joystickb_state.driver_current->disable();

	joystickb_state.is_enabled_flag = 0;
}

void joystickb_init_null(void)
{
	assert(joystickb_state.driver_current == 0);
	assert(!joystickb_state.is_active_flag);

	if (!joystickb_state.is_initialized_flag) {
		joystickb_default();
	}

	joystickb_state.driver_current = &joystickb_none_driver;

	joystickb_state.driver_current->init(-1); /* it must never fail */

	joystickb_state.is_active_flag = 1;
}

void joystickb_abort(void)
{
	if (joystickb_state.is_active_flag) {
		joystickb_done();
	}
}

unsigned joystickb_count_get(void)
{
	assert(joystickb_state.is_active_flag);

	return joystickb_state.driver_current->count_get();
}

unsigned joystickb_stick_count_get(unsigned joystick)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());

	return joystickb_state.driver_current->stick_count_get(joystick);
}

unsigned joystickb_stick_axe_count_get(unsigned joystick, unsigned stick)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(stick < joystickb_stick_count_get(joystick));

	return joystickb_state.driver_current->stick_axe_count_get(joystick, stick);
}

unsigned joystickb_button_count_get(unsigned joystick)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());

	return joystickb_state.driver_current->button_count_get(joystick);
}

unsigned joystickb_rel_count_get(unsigned joystick)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());

	if (joystickb_state.driver_current->rel_count_get)
		return joystickb_state.driver_current->rel_count_get(joystick);
	else
		return 0;
}

const char* joystickb_stick_name_get(unsigned joystick, unsigned stick)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(stick < joystickb_stick_count_get(joystick));

	if (joystickb_state.driver_current->stick_name_get)
		return joystickb_state.driver_current->stick_name_get(joystick, stick);

	if (stick == 0)
		snprintf(joystickb_state.stick_name_buffer, sizeof(joystickb_state.stick_name_buffer), "stick");
	else
		snprintf(joystickb_state.stick_name_buffer, sizeof(joystickb_state.stick_name_buffer), "stick%d", stick+1);

	return joystickb_state.stick_name_buffer;
}

const char* joystickb_stick_axe_name_get(unsigned joystick, unsigned stick, unsigned axe)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(stick < joystickb_stick_count_get(joystick));
	assert(axe < joystickb_stick_axe_count_get(joystick, stick));

	if (joystickb_state.driver_current->stick_axe_name_get)
		return joystickb_state.driver_current->stick_axe_name_get(joystick, stick, axe);

	switch (axe) {
	case 0 : snprintf(joystickb_state.axe_name_buffer, sizeof(joystickb_state.axe_name_buffer), "x"); break;
	case 1 : snprintf(joystickb_state.axe_name_buffer, sizeof(joystickb_state.axe_name_buffer), "y"); break;
	case 2 : snprintf(joystickb_state.axe_name_buffer, sizeof(joystickb_state.axe_name_buffer), "z"); break;
	default: snprintf(joystickb_state.axe_name_buffer, sizeof(joystickb_state.axe_name_buffer), "axe%d", axe+1);
	}

	return joystickb_state.axe_name_buffer;
}

const char* joystickb_button_name_get(unsigned joystick, unsigned button)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(button < joystickb_button_count_get(joystick));

	if (joystickb_state.driver_current->button_name_get)
		return joystickb_state.driver_current->button_name_get(joystick, button);

	snprintf(joystickb_state.button_name_buffer, sizeof(joystickb_state.button_name_buffer), "button%d", button+1);

	return joystickb_state.button_name_buffer;
}

const char* joystickb_rel_name_get(unsigned joystick, unsigned rel)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(rel < joystickb_rel_count_get(joystick));

	if (joystickb_state.driver_current->rel_name_get)
		return joystickb_state.driver_current->rel_name_get(joystick, rel);

	switch (rel) {
	case 0 : snprintf(joystickb_state.rel_name_buffer, sizeof(joystickb_state.rel_name_buffer), "x"); break;
	case 1 : snprintf(joystickb_state.rel_name_buffer, sizeof(joystickb_state.rel_name_buffer), "y"); break;
	case 2 : snprintf(joystickb_state.rel_name_buffer, sizeof(joystickb_state.rel_name_buffer), "z"); break;
	default: snprintf(joystickb_state.rel_name_buffer, sizeof(joystickb_state.rel_name_buffer), "rel%d", rel+1);
	}

	return joystickb_state.rel_name_buffer;
}

unsigned joystickb_button_get(unsigned joystick, unsigned button)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(button < joystickb_button_count_get(joystick));

	return joystickb_state.driver_current->button_get(joystick, button);
}

unsigned joystickb_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(stick < joystickb_stick_count_get(joystick));
	assert(axe < joystickb_stick_axe_count_get(joystick, stick));

	return joystickb_state.driver_current->stick_axe_digital_get(joystick, stick, axe, d);
}

int joystickb_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(stick < joystickb_stick_count_get(joystick));
	assert(axe < joystickb_stick_axe_count_get(joystick, stick));

	return joystickb_state.driver_current->stick_axe_analog_get(joystick, stick, axe);
}

int joystickb_rel_get(unsigned joystick, unsigned rel)
{
	assert(joystickb_state.is_active_flag);
	assert(joystick < joystickb_count_get());
	assert(rel < joystickb_rel_count_get(joystick));

	return joystickb_state.driver_current->rel_get(joystick, rel);
}

void joystickb_calib_start(void)
{
	assert(joystickb_state.is_active_flag);

	if (joystickb_state.driver_current->calib_start)
		joystickb_state.driver_current->calib_start();
}

const char* joystickb_calib_next(void)
{
	assert(joystickb_state.is_active_flag);

	if (joystickb_state.driver_current->calib_next)
		return joystickb_state.driver_current->calib_next();
	else
		return 0;
}

void joystickb_poll(void)
{
	assert(joystickb_state.is_active_flag);

	joystickb_state.driver_current->poll();
}

const char* joystickb_name(void)
{
	assert(joystickb_state.is_active_flag);

	return joystickb_state.driver_current->name;
}

int joystickb_adjust_analog(int value, int low_limit, int high_limit)
{
	int r;
	int a;
	int b;
	int c;

	if (low_limit >= high_limit)
		return 0;
	if (value <= low_limit)
		return -JOYSTICK_DRIVER_BASE;
	if (value >= high_limit)
		return JOYSTICK_DRIVER_BASE;

	/* special case for -128, 127 */
	if (-low_limit == high_limit + 1) {
		++high_limit;
		if (value > high_limit/2)
			++value;
	}

	a = value - low_limit;
	b = JOYSTICK_DRIVER_BASE * 2;
	c = high_limit - low_limit;

	r = a * (long long)b / c;

	r -= JOYSTICK_DRIVER_BASE;

	// ensure limits
	if (r < -JOYSTICK_DRIVER_BASE)
		r = -JOYSTICK_DRIVER_BASE;
	if (r > JOYSTICK_DRIVER_BASE)
		r = JOYSTICK_DRIVER_BASE;

	return r;
}
