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
 * Monitor.
 */

/** \addtogroup Monitor */
/*@{*/

#ifndef __MONITOR_H
#define __MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extra.h"
#include "conf.h"

/***************************************************************************/
/* Monitor */

/**
 * Clock range for the monitor specification.
 */
typedef struct adv_monitor_range_struct {
	double low; /**< Lower clock limit. */
	double high; /**< High clock limit. */
} adv_monitor_range;

typedef struct adv_monitor_mode_struct {
	adv_monitor_range pclock; /**< Supported pixel clocks. */
	adv_monitor_range hclock; /**< Supported horizontal clocks. */
	adv_monitor_range vclock; /**< Supported vertical clocks. */
} adv_monitor_mode;

/**
 * Max number of monitor modes in a monitor specification.
 */
#define MONITOR_MODE_MAX 8

/**
 * Monitor clock specification.
 */
typedef struct adv_monitor_struct {
	unsigned mode_mac; /**< Number of supported monitor modes. */
	adv_monitor_mode mode_map[MONITOR_MODE_MAX]; /**< Supported monitor modes. */
} adv_monitor;

adv_bool monitor_mode_pclock_check(const adv_monitor_mode* mode, double pclock);
adv_bool monitor_mode_hclock_check(const adv_monitor_mode* mode, double hclock);
adv_bool monitor_mode_vclock_check(const adv_monitor_mode* mode, double vclock);
adv_bool monitor_mode_hvclock_check(const adv_monitor_mode* mode, double hclock, double vclock);
adv_bool monitor_mode_clock_check(const adv_monitor_mode* mode, double pclock, double hclock, double vclock);
adv_bool monitor_hvclock_check(const adv_monitor* monitor, double hclock, double vclock);
adv_bool monitor_clock_check(const adv_monitor* monitor, double pclock, double hclock, double vclock);

void monitor_reset(adv_monitor* monitor);
adv_bool monitor_is_empty(const adv_monitor* monitor);
adv_error monitor_parse(adv_monitor* monitor, const char* phv);
void monitor_print(char* buffer, unsigned size, const adv_monitor* monitor);
adv_error monitor_load(adv_conf* context, adv_monitor* monitor);
void monitor_save(adv_conf* context, const adv_monitor* monitor);
void monitor_register(adv_conf* context);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/
