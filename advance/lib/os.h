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
 * OS services.
 */

/** \addtogroup System */
/*@{*/

#ifndef __OS_H
#define __OS_H

#include "conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Init/Done */

/**
 * Initialize the OS.
 */
int os_init(struct conf_context* context);

/**
 * Deinitialize the OS.
 */
void os_done(void);

/**
 * Initialize the inner OS.
 */
int os_inner_init(const char* title);

/**
 * Deinitialize the inner OS.
 */
void os_inner_done(void);

/**
 * Poll the OS event queue.
 */
void os_poll(void);

/**
 * Main program function.
 * The user must implement this function instead of main().
 */
int os_main(int argc, char* argv[]);

/***************************************************************************/
/* Signal */

/**
 * Check if program termination was requested.
 */
int os_is_quit(void);

/**
 * Manage a signal.
 * The user must implement this function. Eventually call simply os_default_signal().
 */
void os_signal(int signum);

/**
 * Default behaviour for a signal.
 */
void os_default_signal(int signum);

/***************************************************************************/
/* Clocks */

/**
 * Type for the os_clock() function.
 */
typedef long long os_clock_t;

/**
 * Number of clock ticks per second.
 */
extern os_clock_t OS_CLOCKS_PER_SEC;

/**
 * Get the current clock value.
 * The base unit is ::OS_CLOCKS_PER_SEC.
 */
os_clock_t os_clock(void);

/***************************************************************************/
/* Led */

#define OS_LED_NUMLOCK 0x1
#define OS_LED_CAPSLOCK 0x2
#define OS_LED_SCROLOCK 0x4

/**
 * Set the keyboard led status.
 */
void os_led_set(unsigned mask);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/
