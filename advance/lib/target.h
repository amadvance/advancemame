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
 * Target OS operations.
 */

#ifndef __TARGET_H
#define __TARGET_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* common */

#include "extra.h"

/** \addtogroup Target */
/*@{*/

/** Max number of arguments. */
#define TARGET_MAXARG 64

/***************************************************************************/
/* Init */

/**
 * Initialize the target system.
 * It's called in the main() function.
 */
adv_error target_init(void);

/** 
 * Deinitialize the target system.
 * It's called in the main() function.
 */
void target_done(void);

/***************************************************************************/
/* Scheduling */

/**
 * Schedule another process if available.
 * If no process is waiting the current process continues immeditiatly.
 * Calling this function generally doesn't reduce the CPU occupation.
 */
void target_yield(void);

/**
 * Put the process in idle state.
 * If no process is waiting the current process waits anyway some time.
 * Generally this call waits at least 10 ms.
 * Calling this function generally reduces the CPU occupation.
 */
void target_idle(void);

/**
 * Wait no more than then specified time.
 * Calling this function generally reduces the CPU occupation.
 * Note that if the requested sleep time is too small the function may return
 * immediatly. In Linux 2.4 this limit is 20 ms, for Linux 2.6 is 3 ms.
 * In these cases the CPU occupation is not reduced.
 * \param us Microsencond to wait.
 */
void target_usleep(unsigned us);

/***************************************************************************/
/* Clocks */

/**
 * Type for the target_clock() function.
 */
typedef long long target_clock_t;

/**
 * Number of clock ticks per second.
 */
extern target_clock_t TARGET_CLOCKS_PER_SEC;

/**
 * Get the current clock value.
 * The base unit is ::TARGET_CLOCKS_PER_SEC.
 */
target_clock_t target_clock(void);

/***************************************************************************/
/* Hardware */

/**
 * Write a byte in a port.
 */
void target_port_set(unsigned addr, unsigned value);

/**
 * Read a byte from a port.
 */
unsigned target_port_get(unsigned addr);

/**
 * Write a byte an a absolute memory address.
 */
void target_writeb(unsigned addr, unsigned char c);

/**
 * Read a byte an a absolute memory address.
 */
unsigned char target_readb(unsigned addr);

/***************************************************************************/
/* Mode */

/**
 * Reset the current video mode.
 * Generally called on emergency.
 */
void target_mode_reset(void);

/***************************************************************************/
/* Sound */

/**
 * Play a short error sound.
 */
void target_sound_error(void);

/**
 * Play a short warning sound.
 */
void target_sound_warn(void);

/**
 * Play a long notify sound.
 */
void target_sound_signal(void);

/***************************************************************************/
/* APM */

/**
 * Shutdown the system.
 * \return ==0 (or never return) if success
 */
adv_error target_apm_shutdown(void);

/**
 * Put the system in standby mode.
 * \return ==0 if success
 */
adv_error target_apm_standby(void);

/**
 * Restore the system after a standby.
 * \return ==0 if success
 */
adv_error target_apm_wakeup(void);

/***************************************************************************/
/* System */

/**
 * Execute a script.
 * \param script Text of the script to run.
 * \return Like running a script containing the specified text.
 */
adv_error target_script(const char* script);

/**
 * Execute an external program with output redirection.
 * \param file Program to run.
 * \param argv Arguments. Use 0 to terminate.
 * \param output Output stream.
 * \return Like system().
 */
adv_error target_spawn_redirect(const char* file, const char** argv, const char* output);

/**
 * Execute an external program.
 * \param file Program to run.
 * \param argv Arguments. Use 0 to terminate.
 * \return Like spawn().
 */
adv_error target_spawn(const char* file, const char** argv);

/**
 * Create a directory.
 * \return like mkdir
 */
adv_error target_mkdir(const char* file);

/**
 * Flush the filesystem cache.
 */
void target_sync(void);

/**
 * Search an executable in the path.
 * \return
 *   == 0 ok
 *   != 0 not found or error
 */
adv_error target_search(char* path, unsigned path_size, const char* file);

/***************************************************************************/
/* Stream */

/**
 * Output an information message like target_out().
 */
void target_out_va(const char *text, va_list arg) __attribute__((format(printf, 1, 0)));

/**
 * Output an information message.
 * The display of the messages may be delayed until a target_flush() call.
 */
void target_out(const char *text, ...) __attribute__((format(printf, 1, 2)));

/**
 * Output an error message like target_err().
 */
void target_err_va(const char *text, va_list arg) __attribute__((format(printf, 1, 0)));

/**
 * Output an error message.
 * The display of the messages may be delayed until a target_flush() call.
 */
void target_err(const char *text, ...) __attribute__((format(printf, 1, 2)));

/**
 * Output a notification message like target_nfo().
 */
void target_nfo_va(const char *text, va_list arg) __attribute__((format(printf, 1, 0)));

/**
 * Output a notification message.
 * These messages may be not printed at all.
 */
void target_nfo(const char *text, ...) __attribute__((format(printf, 1, 2)));

/**
 * Ensure that any delayed message is printed.
 */
void target_flush(void);

/**
 * Process the specified signal.
 */
void target_signal(int signum, void* info, void* context);

/**
 * Crash the process.
 */
void target_crash(void);

/**
 * Compare an arg with an option.
 */
adv_bool target_option_compare(const char* arg, const char* opt);

/**
 * Extract the option name from an argument.
 * \return The option name or 0 if it isn't an option.
 */
const char* target_option_extract(const char* arg);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


