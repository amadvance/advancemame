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
 * Target OS operations.
 */

#ifndef __TARGET_H
#define __TARGET_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Signal */

#ifdef __WIN32__
#define WIFSTOPPED(r) 0
#define WIFSIGNALED(r) 0
#define WIFEXITED(r) 1
#define WEXITSTATUS(r) (r)
#define WTERMSIG(r) 0
#define WSTOPSIG(r) 0
#else
#include <sys/wait.h>
#endif

/***************************************************************************/
/* snprintf */

#ifdef __WIN32__
#include <stdio.h>
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#ifdef __MSDOS__
#include <sys/types.h>
int snprintf(char *str, size_t count, const char *fmt, ...);
int vsnprintf(char *str, size_t count, const char *fmt, va_list arg);
#endif

/***************************************************************************/
/* common */

#include "extra.h"

/** \addtogroup Target */
/*@{*/

/** Max command line length. */
#define TARGET_MAXCMD 1024

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
 * If no more process need execution the current process continues immeditiatly.
 * Calling this function generally doesn't reduce the CPU occupation.
 */
void target_yield(void);

/**
 * Put the process in idle state.
 * If no more process need execution the current process waits anyway some time.
 * Generally this call waits at least 10 ms.
 * Calling this function generally reduces the CPU occupation.
 */
void target_idle(void);

/**
 * Wait no more than then specified time.
 * Calling this function generally reduces the CPU occupation.
 */
void target_usleep(unsigned us);

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
 * Execute an external program with pipe support.
 * \return Like system().
 */
adv_error target_system(const char* cmd);

/**
 * Execute an external program.
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
 * \note The display of the messages may be delayed.
 */
void target_out(const char *text, ...) __attribute__((format(printf, 1, 2)));

/**
 * Output an error message like target_err().
 */
void target_err_va(const char *text, va_list arg) __attribute__((format(printf, 1, 0)));

/**
 * Output an error message.
 * \note The display of the messages may be delayed.
 */
void target_err(const char *text, ...) __attribute__((format(printf, 1, 2)));

/**
 * Output a notification message like target_nfo().
 */
void target_nfo_va(const char *text, va_list arg) __attribute__((format(printf, 1, 0)));

/**
 * Output a notification message.
 * \note These messages may be not printed.
 */
void target_nfo(const char *text, ...) __attribute__((format(printf, 1, 2)));

/**
 * Ensure that any delayed message is printed.
 */
void target_flush(void);

/**
 * Process the specified signal.
 */
void target_signal(int signum);

/**
 * Crash the process.
 */
void target_crash(void);

/**
 * Compare an arg with an option.
 */
adv_bool target_option(const char* arg, const char* opt);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


