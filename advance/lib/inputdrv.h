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
 * Input drivers.
 */

/** \addtogroup Input */
/*@{*/

#ifndef __INPUTDRV_H
#define __INPUTDRV_H

#include "device.h"
#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Codes */

#define INPUTB_NONE 0
#define INPUTB_TAB 9
#define INPUTB_ENTER 13
#define INPUTB_ESC 27
#define INPUTB_SPACE 32
#define INPUTB_UP 256
#define INPUTB_DOWN 257
#define INPUTB_LEFT 258
#define INPUTB_RIGHT 259
#define INPUTB_HOME 264
#define INPUTB_END 265
#define INPUTB_PGUP 266
#define INPUTB_PGDN 267
#define INPUTB_F1 268
#define INPUTB_F2 269
#define INPUTB_F3 270
#define INPUTB_F4 271
#define INPUTB_F5 272
#define INPUTB_F6 273
#define INPUTB_F7 274
#define INPUTB_F8 275
#define INPUTB_F9 276
#define INPUTB_F10 277
#define INPUTB_DEL 278
#define INPUTB_INS 279
#define INPUTB_BACKSPACE 280
#define INPUTB_MAX 512

/***************************************************************************/
/* Driver */

#define INPUT_DRIVER_FLAGS_USER_BIT0 0x10000
#define INPUT_DRIVER_FLAGS_USER_MASK 0xFFFF0000

/**
 * Input driver.
 * This struct abstract all the driver funtionalities.
 */
typedef struct inputb_driver_struct {
	const char* name; /**< Name of the driver */
	const adv_device* device_map; /**< List of supported devices */

	/** Load the configuration options. Call before init() */
	adv_error (*load)(adv_conf* context);

	/** Register the load options. Call before load(). */
	void (*reg)(adv_conf* context);

	adv_error (*init)(int device_id); /**< Initialize the driver */
	void (*done)(void); /**< Deinitialize the driver */

	adv_error (*enable)(adv_bool graphics);
	void (*disable)(void);

	unsigned (*flags)(void); /**< Get the capabilities of the driver */

	adv_bool (*hit)(void);
	unsigned (*get)(void);
} inputb_driver;

#define INPUT_DRIVER_MAX 8

struct inputb_state_struct {
	adv_bool is_initialized_flag;
	adv_bool is_active_flag;
	adv_bool is_enabled_flag; /**< If the inputbb_enable() function was called. */
	unsigned driver_mac;
	inputb_driver* driver_map[INPUT_DRIVER_MAX];
	inputb_driver* driver_current;
	char name[DEVICE_NAME_MAX];
};

extern struct inputb_state_struct inputb_state;

void inputb_reg(adv_conf* config_context, adv_bool auto_detect);
void inputb_reg_driver(adv_conf* config_context, inputb_driver* driver);
adv_error inputb_load(adv_conf* config_context);
adv_error inputb_init(void);
void inputb_done(void);

/**
 * Enable the input driver.
 * The input driver must be enabled after the video mode set.
 * \param graphics Enable the graphics mode behaviour, disabling echo and cursor.
 */
adv_error inputb_enable(adv_bool graphics);

/**
 * Disable the input driver.
 */
void inputb_disable(void);

void inputb_abort(void);
adv_bool inputb_hit(void);
unsigned inputb_get(void);

/**
 * Get the driver/device name.
 * \return Pointer at a static buffer.
 */
const char* inputb_name(void);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/
