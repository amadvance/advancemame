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
 * Keyboard drivers.
 */

#ifndef __KEYDRV_H
#define __KEYDRV_H

#include "device.h"
#include "conf.h"
#include "key.h"

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
	adv_error (*enable)(adv_bool graphics);
	void (*disable)(void);
	unsigned (*flags)(void);
	unsigned (*count_get)(void);
	adv_bool (*has)(unsigned keyboard, unsigned code);
	unsigned (*get)(unsigned keyboard, unsigned code);
	void (*all_get)(unsigned keyboard, unsigned char* code_map);
	void (*led_set)(unsigned keyboard, unsigned mask);
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
	adv_bool is_enabled_flag; /**< If the keyb_enable() function was called. */
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
 * Initialize the null driver.
 * This command setup the null keyboard driver, also if it isn't configured in the load option.
 */
void keyb_init_null(void);

/**
 * Deinitialize the keyboard driver.
 * Call it only after a succesful keyb_init().
 */
void keyb_done(void);

/**
 * Enable the keyboard driver.
 * The keyboard driver must be enabled after the video mode set.
 * \param graphics Enable the graphics mode behaviour, disabling echo and cursor.
 */
adv_error keyb_enable(adv_bool graphics);

/**
 * Disable the keyboard driver.
 */
void keyb_disable(void);

/**
 * Abort the keyboard driver.
 * This function can be called from a signal handler, also if the keyboard
 * driver isn't initialized.
 */
void keyb_abort(void);

/**
 * Get the number of keyboards.
 */
unsigned keyb_count_get(void);

/**
 * Return the capabilities flag of the keyboard driver.
 */
unsigned keyb_flags(void);

/**
 * Check if a keyboard has the specified key.
 * \param keyboard Keyboard number.
 * \param code One of the KEYB_* codes.
 */
adv_bool keyb_has(unsigned keyboard, unsigned code);

/**
 * Get the status of the specified key.
 * \param keyboard Keyboard number.
 * \param code One of the KEYB_* codes.
 * \return
 *  - == 0 not pressed
 *  - != 0 pressed
 */
unsigned keyb_get(unsigned keyboard, unsigned code);

/**
 * Get the status of all the keys.
 * \param keyboard Keyboard number.
 * \param code_map Destination vector of KEYB_MAX elements.
 */
void keyb_all_get(unsigned keyboard, unsigned char* code_map);


#define KEYB_LED_NUML 0x0001
#define KEYB_LED_CAPSL 0x0002
#define KEYB_LED_SCROLLL 0x0004
#define KEYB_LED_COMPOSE 0x0008
#define KEYB_LED_KANA 0x0010
#define KEYB_LED_SLEEP 0x0020
#define KEYB_LED_SUSPEND 0x0040
#define KEYB_LED_MUTE 0x0080
#define KEYB_LED_MISC 0x0100

/**
 * Set the led state.
 * \param keyboard Keyboard number.
 * \param led_mask Mask of the led to enable.
 */
void keyb_led_set(unsigned keyboard, unsigned led_mask);

/**
 * Poll the keyboard status.
 * This function must be called periodically to ensure that
 * the keyboard events are processed.
 */
void keyb_poll(void);

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
const char* keyb_name(void);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


