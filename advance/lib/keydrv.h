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

/** \file
 * Keyboard drivers.
 */

#ifndef __KEYDRV_H
#define __KEYDRV_H

#include "device.h"
#include "conf.h"
#include "key.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Driver */

/**
 * Keyboard driver.
 * This struct abstract all the driver funtionalities.
 */
typedef struct keyb_driver_struct {
	const char* name; /**< Name of the driver. */
	const adv_device* device_map; /**< Vector of supported devices. 0 terminated. */
	
	adv_error (*load)(adv_conf* context);
	void (*reg)(adv_conf* context);
	adv_error (*init)(int device_id, adv_bool disable_special);
	void (*done)(void);
	unsigned (*flags)(void);
	unsigned (*get)(unsigned code);
	void (*all_get)(unsigned char* code_map);
	void (*poll)(void);
} keyb_driver;

/**
 * Max number of drivers registrable.
 */
#define KEYB_DRIVER_MAX 8

/**
 * State of the driver system.
 */
struct keyb_state_struct {
	adv_bool is_initialized_flag; /**< If the keyb_load() or keyb_default() function was called. */
	adv_bool is_active_flag; /**< If the keyb_init() function was called. */
	unsigned driver_mac; /**< Number of registered drivers. */
	keyb_driver* driver_map[KEYB_DRIVER_MAX]; /**< Registered drivers. */
	keyb_driver* driver_current; /**< Current driver active. 0 if none. */
	char name[DEVICE_NAME_MAX]; /**< Name of the driver to use. */
};

/**
 * Global state of the driver system.
 */
extern struct keyb_state_struct keyb_state;

/** \addtogroup Keyboard */
/*@{*/

/** \name Flags user
 * Flags for the user.
 */
/*@{*/
#define KEYB_DRIVER_FLAGS_USER_BIT0 0x10000 /**< First user flag. */
#define KEYB_DRIVER_FLAGS_USER_MASK 0xFFFF0000 /**< Mask. */
/*@}*/

/**
 * Register the load option of the keyboard driver.
 * \param config_context Configuration context to use.
 * \param auto_detect Enable the autodetection if no keyboard driver is specified.
 */
void keyb_reg(adv_conf* config_context, adv_bool auto_detect);

/**
 * Register the load options of the specified driver.
 * Call before load().
 * \param config_context Configuration context to use.
 * \param driver Driver to register.
 */
void keyb_reg_driver(adv_conf* config_context, keyb_driver* driver);

/**
 * Load the configuration options.
 * Call before init().
 * \param config_context Configuration context to use.
 */
adv_error keyb_load(adv_conf* config_context);

/**
 * Initialize the keyboard driver.
 * \param disable_special Disable the special OS keys combinations.
 */
adv_error keyb_init(adv_bool disable_special);

/**
 * Deinitialize the keyboard driver.
 * Call it only after a succesful keyb_init().
 */
void keyb_done(void);

/**
 * Abort the keyboard driver.
 * This function can be called from a signal handler, also if the keyboard
 * driver isn't initialized.
 */
void keyb_abort(void);

/**
 * Return the capabilities flag of the keyboard driver.
 */
static inline unsigned keyb_flags(void)
{
	assert( keyb_state.is_active_flag );

	return keyb_state.driver_current->flags();
}

/**
 * Get the status of the specified key.
 * \param code One of the KEYB_* codes.
 * \return
 *  - == 0 not pressed
 *  - != 0 pressed
 */
static inline unsigned keyb_get(unsigned code)
{
	assert( keyb_state.is_active_flag );

	return keyb_state.driver_current->get(code);
}

/**
 * Get the status of all the keys.
 * \param code_map The destination vector of KEYB_MAX elements.
 */
static inline void keyb_all_get(unsigned char* code_map)
{
	assert( keyb_state.is_active_flag );

	keyb_state.driver_current->all_get(code_map);
}

/**
 * Poll the keyboard status.
 * This function must be called periodically to ensure that
 * the keyboard events are processed.
 */
static inline void keyb_poll(void)
{
	assert( keyb_state.is_active_flag );

	keyb_state.driver_current->poll();
}

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
static inline const char* keyb_name(void)
{
	return keyb_state.name;
}

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


