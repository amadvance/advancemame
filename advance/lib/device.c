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

#include "device.h"
#include "log.h"
#include "target.h"
#include "snstring.h"

static const adv_device* device_match_one(const char* tag, const adv_driver* drv, adv_bool allow_none)
{
	char tag_driver[DEVICE_NAME_MAX];
	char* tag_device;
	const adv_device* i;

	sncpy(tag_driver, DEVICE_NAME_MAX, tag);
	tag_device = strchr(tag_driver, '/');
	if (tag_device) {
		*tag_device = 0;
		++tag_device;
	} else {
		tag_device = "auto";
	}

	if (strcmp(drv->name, "none")==0) {
		if (allow_none || strcmp(tag_driver, "none")==0) {
			assert(drv->device_map->name != 0);
			return drv->device_map;
		} else {
			return 0; /* "auto" never choose "none" */
		}
	}

	if (strcmp(tag_driver, "auto")!=0 && strcmp(tag_driver, drv->name)!=0)
		return 0;

	i = drv->device_map;
	while (i->name) {
		if (strcmp(i->name, tag_device)==0)
			break;
		++i;
	}

	if (!i->name)
		return 0;

	return i;
}

/**
 * Check if a adv_device name match the user specification.
 * \param tag user specification.
 * \param drv adv_device to check.
 * \param allow_none if true allows the "none" driver also if it
 *   isn't specified. Otherwise the "none" driver is used only if explictly
 *   specified. It isn't used from "auto".
 */
const adv_device* device_match(const char* tag, const adv_driver* drv, adv_bool allow_none)
{
	char buffer[DEVICE_NAME_MAX];
	const char* tag_one;

	sncpy(buffer, sizeof(buffer), tag);
	tag_one = strtok(buffer, " \t");
	while (tag_one) {
		const adv_device* dev = device_match_one(tag_one, drv, allow_none);
		if (dev)
			return dev;
		tag_one = strtok(NULL, " \t");
	}

	return 0;
}

void device_error(const char* option, const char* arg, const adv_driver** driver_map, unsigned driver_mac)
{
	unsigned i, j;

	log_std(("adv_device: device_error %s %s\n", option, arg));

	target_err("Invalid argument '%s' for option '%s'\n", arg, option);
	target_err("Valid values are:\n");
	target_err("%16s %s\n", "auto", "Automatic detection");

	for(i=0;i<driver_mac;++i) {
		for(j=0;driver_map[i]->device_map[j].name;++j) {
			char buffer[DEVICE_NAME_MAX];
			if (strcmp(driver_map[i]->device_map[j].name, "auto")==0) {
				snprintf(buffer, sizeof(buffer), "%s", driver_map[i]->name);
			} else {
				snprintf(buffer, sizeof(buffer), "%s/%s", driver_map[i]->name, driver_map[i]->device_map[j].name);
			}
			target_err("%16s %s\n", buffer, driver_map[i]->device_map[j].desc);
		}
	}
}

adv_error device_check(const char* option, const char* arg, const adv_driver** driver_map, unsigned driver_mac, const char* driver_ignore)
{
	char buffer[DEVICE_NAME_MAX];
	const char* tag_one;
	unsigned i, j;

	/* check the validity of every item on the argument */
	sncpy(buffer, sizeof(buffer), arg);
	tag_one = strtok(buffer, " \t");
	while (tag_one) {
		if (strcmp("auto", tag_one)!=0 && strstr(driver_ignore, tag_one)==0) {
			for(i=0;i<driver_mac;++i) {
				if (strcmp(driver_map[i]->name, tag_one)==0)
					break;
				for(j=0;driver_map[i]->device_map[j].name;++j) {
					char cat_buffer[DEVICE_NAME_MAX];
					snprintf(cat_buffer, sizeof(cat_buffer), "%s/%s", driver_map[i]->name, driver_map[i]->device_map[j].name);
					if (strcmp(cat_buffer, tag_one)==0)
						break;
				}
				if (driver_map[i]->device_map[j].name)
					break;
			}
			if (i == driver_mac) {
				device_error(option, tag_one, driver_map, driver_mac);
				return -1;
			}
		}
		tag_one = strtok(NULL, " \t");
	}

	return 0;
}

