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

/** \file
 * Devices.
 */

/** \addtogroup Device */
/*@{*/

#ifndef __DEVICE_H
#define __DEVICE_H

/***************************************************************************/
/* Driver */

#include "extra.h"

/** Max number of devices */
#define DEVICE_MAX 8

/** Max length of a device name or a list of names */
#define DEVICE_NAME_MAX 256

/**
 * Device minimal information.
 * This structure define the common entries for all the devices.
 * A device is always part of a driver. Generally a driver supports
 * more than one device.
 */
typedef struct adv_device_struct {
	const char *name; /** Name of the device. */
	int id; /** Identifier of the device. This identifier must be passed at the driver init() function. */
	const char* desc; /** Description of the device. */
} adv_device;

/**
 * Driver minimal information.
 * This structure define the common entries for all the drivers.
 */
typedef struct adv_driver_struct {
	const char *name; /** Name of the driver. */
	const adv_device* device_map; /** List of supported device. */
} adv_driver;

const adv_device* device_match(const char* tag, const adv_driver* drv, adv_bool allow_none);
adv_error device_check(const char* option, const char* arg, const adv_driver** driver_map, unsigned driver_mac, const char* driver_ignore);
void device_error(const char* option, const char* arg, const adv_driver** driver_map, unsigned driver_mac);

#endif

/*@}*/
