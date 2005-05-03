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

/** \file
 * Mouse drivers.
 */

#ifndef __MOUSEDRV_H
#define __MOUSEDRV_H

#include "device.h"
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Driver */

/**
 * Mouse driver.
 * This struct abstract all the driver funtionalities.
 */
typedef struct mouseb_driver_struct {
	const char* name; /**< Name of the driver */
	const adv_device* device_map;  /**< Vector of supported devices. 0 terminated. */

	/** Load the configuration options. Call before init() */
	adv_error (*load)(adv_conf* context);

	/** Register the load options. Call before load(). */
	void (*reg)(adv_conf* context);

	adv_error (*init)(int device_id); /**< Initialize the driver */
	void (*done)(void); /**< Deinitialize the driver */
	adv_error (*enable)(void); /**< Enable the driver */
	void (*disable)(void); /**< Disable the driver */

	unsigned (*flags)(void); /**< Get the capabilities of the driver */

	unsigned (*count_get)(void);
	unsigned (*axe_count_get)(unsigned mouse);
	const char* (*axe_name_get)(unsigned mouse, unsigned axe);
	unsigned (*button_count_get)(unsigned mouse);
	const char* (*button_name_get)(unsigned mouse, unsigned button);
	int (*axe_get)(unsigned mouse, unsigned axe);
	unsigned (*button_get)(unsigned mouse, unsigned button);
	void (*poll)(void);
} mouseb_driver;

/**
 * Max number of drivers registrable.
 */
#define MOUSE_DRIVER_MAX 8

/**
 * State of the driver system.
 */
struct mouseb_state_struct {
	adv_bool is_initialized_flag; /**< If the mouseb_load() or mouseb_default() function was called. */
	adv_bool is_active_flag; /**< If the mouseb_init() function was called. */
	adv_bool is_enabled_flag; /**< If the mouseb_enable() function was called. */	
	unsigned driver_mac; /**< Number of registered drivers. */
	mouseb_driver* driver_map[MOUSE_DRIVER_MAX]; /**< Registered drivers. */
	mouseb_driver* driver_current; /**< Current driver active. 0 if none. */
	char name[DEVICE_NAME_MAX]; /**< Name of the driver to use. */
	char axe_name_buffer[DEVICE_NAME_MAX];
	char button_name_buffer[DEVICE_NAME_MAX];
};

/**
 * Global state of the driver system.
 */
extern struct mouseb_state_struct mouseb_state;

/** \addtogroup Mouse */
/*@{*/

#define MOUSE_DRIVER_FLAGS_USER_BIT0 0x10000
#define MOUSE_DRIVER_FLAGS_USER_MASK 0xFFFF0000

void mouseb_reg(adv_conf* config_context, adv_bool auto_detect);
void mouseb_reg_driver(adv_conf* config_context, mouseb_driver* driver);
adv_error mouseb_load(adv_conf* config_context);
adv_error mouseb_init(void);
void mouseb_init_null(void);
void mouseb_done(void);
adv_error mouseb_enable(void);
void mouseb_disable(void);
void mouseb_abort(void);
unsigned mouseb_count_get(void);
unsigned mouseb_axe_count_get(unsigned mouse);
unsigned mouseb_button_count_get(unsigned mouse);
int mouseb_axe_get(unsigned mouse, unsigned axe);
unsigned mouseb_button_get(unsigned mouse, unsigned button);
const char* mouseb_button_name_get(unsigned mouse, unsigned button);
const char* mouseb_axe_name_get(unsigned mouse, unsigned axe);
void mouseb_poll(void);

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
const char* mouseb_name(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


