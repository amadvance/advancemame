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

#ifndef __DEVICE_H
#define __DEVICE_H

/***************************************************************************/
/* Driver */

#include "videostd.h"

/** Max number of devices */
#define DEVICE_MAX 8

/** Max length of a device name or a list of names */
#define DEVICE_NAME_MAX 256

struct device_struct {
	const char *name;
	int id;
	const char* desc;
};

typedef struct device_struct device;

struct driver_struct {
	const char *name; /** Name of the driver */
	const device* device_map; /** List of supported device */
};

typedef struct driver_struct driver;

const device* device_match(const char* tag, const driver* drv);
video_error device_check(const char* option, const char* arg, const driver** driver_map, unsigned driver_mac, const char* driver_ignore);
void device_error(const char* option, const char* arg, const driver** driver_map, unsigned driver_mac);

#endif
