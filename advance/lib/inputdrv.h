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

#ifndef __INPUTDRV_H
#define __INPUTDRV_H

#include "device.h"
#include "conf.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Codes */

#define OS_INPUT_TAB 9
#define OS_INPUT_ENTER 13
#define OS_INPUT_ESC 27
#define OS_INPUT_SPACE 32
#define OS_INPUT_UP 256
#define OS_INPUT_DOWN 257
#define OS_INPUT_LEFT 258
#define OS_INPUT_RIGHT 259
#define OS_INPUT_CTRLUP 260
#define OS_INPUT_CTRLDOWN 261
#define OS_INPUT_CTRLRIGHT 262
#define OS_INPUT_CTRLLEFT 263
#define OS_INPUT_HOME 264
#define OS_INPUT_END 265
#define OS_INPUT_PGUP 266
#define OS_INPUT_PGDN 267
#define OS_INPUT_F1 268
#define OS_INPUT_F2 269
#define OS_INPUT_F3 270
#define OS_INPUT_F4 271
#define OS_INPUT_F5 272
#define OS_INPUT_F6 273
#define OS_INPUT_F7 274
#define OS_INPUT_F8 275
#define OS_INPUT_F9 276
#define OS_INPUT_F10 277
#define OS_INPUT_DEL 278
#define OS_INPUT_INS 279
#define OS_INPUT_BACKSPACE 280
#define OS_INPUT_MAX 512

/***************************************************************************/
/* Driver */

#define INPUT_DRIVER_FLAGS_USER_BIT0 0x10000
#define INPUT_DRIVER_FLAGS_USER_MASK 0xFFFF0000

struct inputb_driver_struct {
	const char* name; /**< Name of the driver */
	const device* device_map; /**< List of supported devices */

	/** Load the configuration options. Call before init() */
	video_error (*load)(struct conf_context* context);

	/** Register the load options. Call before load(). */
	void (*reg)(struct conf_context* context);

	video_error (*init)(int device_id); /**< Initialize the driver */
	void (*done)(void); /**< Deinitialize the driver */

	unsigned (*flags)(void); /**< Get the capabilities of the driver */

	video_bool (*hit)(void);
	unsigned (*get)(void);
};

typedef struct inputb_driver_struct inputb_driver;

#define INPUT_DRIVER_MAX 8

struct inputb_state_struct {
	video_bool is_initialized_flag;
	video_bool is_active_flag;
	unsigned driver_mac;
	inputb_driver* driver_map[INPUT_DRIVER_MAX];
	inputb_driver* driver_current;
	char name[DEVICE_NAME_MAX];
};

extern struct inputb_state_struct inputb_state;

void inputb_reg(struct conf_context* config_context, video_bool auto_detect);
void inputb_reg_driver(struct conf_context* config_context, inputb_driver* driver);
video_error inputb_load(struct conf_context* config_context);
video_error inputb_init(void);
void inputb_done(void);
void inputb_abort(void);

static __inline__ video_bool inputb_hit(void) {
	assert( inputb_state.is_active_flag );

	return inputb_state.driver_current->hit();
}

static __inline__ unsigned inputb_get(void) {
	assert( inputb_state.is_active_flag );

	return inputb_state.driver_current->get();
}

static __inline__ const char* inputb_name(void) {
	return inputb_state.name;
}

#ifdef __cplusplus
}
#endif

#endif
