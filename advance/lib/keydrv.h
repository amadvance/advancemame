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

#define KEYB_DRIVER_FLAGS_USER_BIT0 0x10000
#define KEYB_DRIVER_FLAGS_USER_MASK 0xFFFF0000

struct keyb_driver_struct {
	const char* name; /**< Name of the driver */
	const device* device_map; /**< List of supported devices */

	/** Load the configuration options. Call before init() */
	video_error (*load)(struct conf_context* context);

	/** Register the load options. Call before load(). */
	void (*reg)(struct conf_context* context);

	video_error (*init)(int device_id, video_bool disable_special); /**< Initialize the driver */
	void (*done)(void); /**< Deinitialize the driver */

	unsigned (*flags)(void); /**< Get the capabilities of the driver */

	unsigned (*get)(unsigned code);
	void (*all_get)(unsigned char* code_map);
	void (*poll)(void);
};

typedef struct keyb_driver_struct keyb_driver;

#define KEYB_DRIVER_MAX 8

struct keyb_state_struct {
	video_bool is_initialized_flag;
	video_bool is_active_flag;
	unsigned driver_mac;
	keyb_driver* driver_map[KEYB_DRIVER_MAX];
	keyb_driver* driver_current;
	char name[DEVICE_NAME_MAX];
};

extern struct keyb_state_struct keyb_state;

void keyb_reg(struct conf_context* config_context, video_bool auto_detect);
void keyb_reg_driver(struct conf_context* config_context, keyb_driver* driver);
video_error keyb_load(struct conf_context* config_context);
video_error keyb_init(int disable_special);
void keyb_done(void);
void keyb_abort(void);

static __inline__ unsigned keyb_get(unsigned code) {
	assert( keyb_state.is_active_flag );

	return keyb_state.driver_current->get(code);
}

static __inline__ void keyb_all_get(unsigned char* code_map) {
	assert( keyb_state.is_active_flag );

	keyb_state.driver_current->all_get(code_map);
}

static __inline__ void keyb_poll(void) {
	assert( keyb_state.is_active_flag );

	keyb_state.driver_current->poll();
}

static __inline__ const char* keyb_name(void) {
	return keyb_state.name;
}

#ifdef __cplusplus
}
#endif

#endif
