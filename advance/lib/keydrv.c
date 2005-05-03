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

#include "keydrv.h"
#include "log.h"
#include "error.h"
#include "snstring.h"
#include "knone.h"

struct keyb_state_struct keyb_state;

void keyb_default(void)
{
	keyb_state.is_initialized_flag = 1;
	sncpy(keyb_state.name, DEVICE_NAME_MAX, "auto");
}

void keyb_reg(adv_conf* context, adv_bool auto_detect)
{
	conf_string_register_default(context, "device_keyboard", auto_detect ? "auto" : "none");
}

void keyb_reg_driver(adv_conf* context, keyb_driver* driver)
{
	assert(keyb_state.driver_mac < KEYB_DRIVER_MAX);

	keyb_state.driver_map[keyb_state.driver_mac] = driver;
	keyb_state.driver_map[keyb_state.driver_mac]->reg(context);

	log_std(("keyb: register driver %s\n", driver->name));

	++keyb_state.driver_mac;
}

adv_error keyb_load(adv_conf* context)
{
	unsigned i;
	int at_least_one;

	if (keyb_state.driver_mac == 0) {
		error_set("No keyboard driver was compiled in.");
		return -1;
	}

	keyb_state.is_initialized_flag = 1;
	sncpy(keyb_state.name, DEVICE_NAME_MAX, conf_string_get_default(context, "device_keyboard"));

	/* load specific driver options */
	at_least_one = 0;
	for(i=0;i<keyb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(keyb_state.name, (adv_driver*)keyb_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (keyb_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_keyboard", keyb_state.name, (const adv_driver**)keyb_state.driver_map, keyb_state.driver_mac);
		return -1;
	}

	return 0;
}

adv_error keyb_init(adv_bool disable_special)
{
	unsigned i;

	assert(keyb_state.driver_current == 0);
	assert(!keyb_state.is_active_flag);

	if (!keyb_state.is_initialized_flag) {
		keyb_default();
	}

	/* store the error prefix */
	error_nolog_set("Unable to initialize the keyboard driver. The errors are:\n");

	for(i=0;i<keyb_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(keyb_state.name, (const adv_driver*)keyb_state.driver_map[i], 0);

		error_cat_set(keyb_state.driver_map[i]->name, 1);

		if (dev && keyb_state.driver_map[i]->init(dev->id, disable_special) == 0) {
			keyb_state.driver_current = keyb_state.driver_map[i];
			break;
		}
	}

	error_cat_set(0, 0);

	if (!keyb_state.driver_current)
		return -1;

	error_reset();

	log_std(("keyb: select driver %s\n", keyb_state.driver_current->name));

	keyb_state.is_active_flag = 1;
	keyb_state.is_enabled_flag = 0;

	return 0;
}

void keyb_done(void)
{
	assert(keyb_state.driver_current);
	assert(keyb_state.is_active_flag && !keyb_state.is_enabled_flag);

	keyb_state.driver_current->done();

	keyb_state.driver_current = 0;
	keyb_state.is_active_flag = 0;
}

void keyb_init_null(void)
{
	assert(keyb_state.driver_current == 0);
	assert(!keyb_state.is_active_flag);

	if (!keyb_state.is_initialized_flag) {
		keyb_default();
	}

	keyb_state.driver_current = &keyb_none_driver;

	keyb_state.driver_current->init(-1, 1); /* it must never fail */

	keyb_state.is_active_flag = 1;
}

adv_error keyb_enable(adv_bool graphics)
{
	assert(keyb_state.is_active_flag && !keyb_state.is_enabled_flag);

	if (keyb_state.driver_current->enable
		&& keyb_state.driver_current->enable(graphics) != 0)
		return -1;

	keyb_state.is_enabled_flag = 1;

	return 0;
}

void keyb_disable(void)
{
	assert(keyb_state.is_active_flag && keyb_state.is_enabled_flag);

	if (keyb_state.driver_current->disable)
		keyb_state.driver_current->disable();

	keyb_state.is_enabled_flag = 0;
}

void keyb_abort(void)
{
	if (keyb_state.is_enabled_flag) {
		keyb_disable();
	}

	if (keyb_state.is_active_flag) {
		keyb_done();
	}
}

unsigned keyb_count_get(void)
{
	assert(keyb_state.is_active_flag);

	return keyb_state.driver_current->count_get();
}

unsigned keyb_flags(void)
{
	assert(keyb_state.is_active_flag);

	return keyb_state.driver_current->flags();
}

adv_bool keyb_has(unsigned keyboard, unsigned code)
{
	assert(keyb_state.is_active_flag);
	assert(keyboard < keyb_count_get());
	assert(code < KEYB_MAX);

	return keyb_state.driver_current->has(keyboard, code);
}

unsigned keyb_get(unsigned keyboard, unsigned code)
{
	assert(keyb_state.is_active_flag && keyb_state.is_enabled_flag);
	assert(keyboard < keyb_count_get());
	assert(code < KEYB_MAX);

	return keyb_state.driver_current->get(keyboard, code);
}

void keyb_all_get(unsigned keyboard, unsigned char* code_map)
{
	assert(keyb_state.is_active_flag && keyb_state.is_enabled_flag);
	assert(keyboard < keyb_count_get());

	keyb_state.driver_current->all_get(keyboard, code_map);
}

void keyb_led_set(unsigned keyboard, unsigned led_mask)
{
	assert(keyb_state.is_active_flag && keyb_state.is_enabled_flag);
	assert(keyboard < keyb_count_get());

	if (keyb_state.driver_current->led_set)
		keyb_state.driver_current->led_set(keyboard, led_mask);
}

void keyb_poll(void)
{
	assert(keyb_state.is_active_flag && keyb_state.is_enabled_flag);

	keyb_state.driver_current->poll();
}

const char* keyb_name(void)
{
	assert(keyb_state.is_active_flag);

	return keyb_state.driver_current->name;
}

