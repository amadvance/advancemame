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

#include "device.h"

#include <string.h>
#include <stdio.h>

static const device* device_match_one(const char* tag, const driver* drv) {
	char tag_driver[DEVICE_NAME_MAX];
	char* tag_device;
	const device* i;

	strcpy(tag_driver,tag);
	tag_device = strchr(tag_driver,'/');
	if (tag_device) {
		*tag_device = 0;
		++tag_device;
	} else {
		tag_device = "auto";
	}

	if (strcmp(tag_driver,"auto")!=0 && strcmp(tag_driver,drv->name)!=0)
		return 0;

	i = drv->device_map;
	while (i->name) {
		if (strcmp(i->name,tag_device)==0)
			break;
		++i;
	}

	if (!i->name)
		return 0;

	return i;
}

const device* device_match(const char* tag, const driver* drv) {
	char buffer[DEVICE_NAME_MAX];
	const char* tag_one;
	strcpy(buffer, tag);

	tag_one = strtok(buffer," \t");
	while (tag_one) {
		const device* dev = device_match_one(tag_one,drv);
		if (dev)
			return dev;
		tag_one = strtok(NULL," \t");
	}

	return 0;
}

void device_error(const char* option, const char* arg, const driver** driver_map, unsigned driver_mac) {
	unsigned i,j;
	printf("Invalid argument '%s' for option '%s'\n",arg,option);
	printf("Valid values are:\n");
	printf("%16s %s\n","auto","Automatic detection");

	for(i=0;i<driver_mac;++i) {
		for(j=0;driver_map[i]->device_map[j].name;++j) {
			char buffer[DEVICE_NAME_MAX];
			if (strcmp(driver_map[i]->device_map[j].name,"auto")==0) {
				sprintf(buffer,"%s",driver_map[i]->name);
			} else {
				sprintf(buffer,"%s/%s",driver_map[i]->name,driver_map[i]->device_map[j].name);
			}
			printf("%16s %s\n",buffer,driver_map[i]->device_map[j].desc);
		}
	}
}

video_error device_check(const char* option, const char* arg, const driver** driver_map, unsigned driver_mac, const char* driver_ignore) {
	char buffer[DEVICE_NAME_MAX];
	const char* tag_one;
	unsigned i,j;

	/* check the validity of every item on the argument */
	strcpy(buffer, arg);
	tag_one = strtok(buffer," \t");
	while (tag_one) {
		if (strcmp("auto",tag_one)!=0 && strstr(driver_ignore,tag_one)==0) {
			for(i=0;i<driver_mac;++i) {
				if (strcmp(driver_map[i]->name,tag_one)==0)
					break;
				for(j=0;driver_map[i]->device_map[j].name;++j) {
					char buffer_cat[DEVICE_NAME_MAX];
					sprintf(buffer_cat,"%s/%s",driver_map[i]->name,driver_map[i]->device_map[j].name);
					if (strcmp(buffer_cat,tag_one)==0)
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
		tag_one = strtok(NULL," \t");
	}

	return 0;
}

