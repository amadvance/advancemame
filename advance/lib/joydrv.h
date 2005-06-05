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
 * Joystick drivers.
 */

/** \addtogroup Joystick */
/*@{*/

#ifndef __JOYDRV_H
#define __JOYDRV_H

#include "device.h"
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Driver */

/**
 * Base unit of the analog axes.
 */
#define JOYSTICK_DRIVER_BASE 65536

#define JOYSTICK_DRIVER_FLAGS_USER_BIT0 0x10000
#define JOYSTICK_DRIVER_FLAGS_USER_MASK 0xFFFF0000

/**
 * Joystick driver.
 * This struct abstract all the driver funtionalities.
 */
typedef struct joystickb_driver_struct {
	const char* name; /**< Name of the driver */
	const adv_device* device_map; /**< List of supported devices */

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
	unsigned (*stick_count_get)(unsigned joystick);
	unsigned (*stick_axe_count_get)(unsigned joystick, unsigned stick);
	const char* (*stick_name_get)(unsigned joystick, unsigned stick);
	const char* (*stick_axe_name_get)(unsigned joystick, unsigned stick, unsigned axe);
	unsigned (*stick_axe_digital_get)(unsigned joystick, unsigned stick, unsigned axe, unsigned d);
	int (*stick_axe_analog_get)(unsigned joystick, unsigned stick, unsigned axe);
	unsigned (*button_count_get)(unsigned joystick);
	const char* (*button_name_get)(unsigned joystick, unsigned button);
	unsigned (*button_get)(unsigned joystick, unsigned button);
	unsigned (*rel_count_get)(unsigned joystick);
	const char* (*rel_name_get)(unsigned joystick, unsigned rel);
	int (*rel_get)(unsigned joystick, unsigned rel);
	void (*calib_start)(void);
	const char* (*calib_next)(void);
	void (*poll)(void);
} joystickb_driver;

#define JOYSTICK_DRIVER_MAX 8

struct joystickb_state_struct {
	adv_bool is_initialized_flag;
	adv_bool is_active_flag;
	adv_bool is_enabled_flag;
	unsigned driver_mac;
	joystickb_driver* driver_map[JOYSTICK_DRIVER_MAX];
	joystickb_driver* driver_current;
	char name[DEVICE_NAME_MAX];
	char axe_name_buffer[DEVICE_NAME_MAX];
	char button_name_buffer[DEVICE_NAME_MAX];
	char stick_name_buffer[DEVICE_NAME_MAX];
	char rel_name_buffer[DEVICE_NAME_MAX];
};

extern struct joystickb_state_struct joystickb_state;

/**
 * Adjust a generic value to the library limit -JOYSTICK_DRIVER_BASE, JOYSTICK_DRIVER_BASE.
 * \param value Value to adjust.
 * \param min Low limit.
 * \param max High limit.
 * \return Adjusted value.
 */
int joystickb_adjust_analog(int value, int min, int max);

void joystickb_reg(adv_conf* config_context, adv_bool auto_detect);
void joystickb_reg_driver(adv_conf* config_context, joystickb_driver* driver);
adv_error joystickb_load(adv_conf* config_context);
adv_error joystickb_init(void);
void joystickb_init_null(void);
void joystickb_done(void);
adv_error joystickb_enable(void);
void joystickb_disable(void);
void joystickb_abort(void);
unsigned joystickb_count_get(void);
unsigned joystickb_stick_count_get(unsigned joystick);
unsigned joystickb_stick_axe_count_get(unsigned joystick, unsigned stick);
unsigned joystickb_button_count_get(unsigned joystick);
unsigned joystickb_rel_count_get(unsigned joystick);
const char* joystickb_stick_name_get(unsigned joystick, unsigned stick);
const char* joystickb_stick_axe_name_get(unsigned joystick, unsigned stick, unsigned axe);
const char* joystickb_button_name_get(unsigned joystick, unsigned button);
const char* joystickb_rel_name_get(unsigned joystick, unsigned rel);
unsigned joystickb_button_get(unsigned joystick, unsigned button);
unsigned joystickb_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d);
int joystickb_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe);
int joystickb_rel_get(unsigned joystick, unsigned rel);
void joystickb_calib_start(void);
const char* joystickb_calib_next(void);
void joystickb_poll(void);

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
const char* joystickb_name(void);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/
