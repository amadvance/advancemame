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
 * Log.
 */

#ifndef __LOG_H
#define __LOG_H

#include "extra.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Log */

void log_va(const char* text, va_list arg);
void log_f(const char* text, ...) __attribute__((format(printf, 1, 2)));
void log_f_modeline_cb(const char* text, unsigned pixel_clock, unsigned hde, unsigned hbs, unsigned hrs, unsigned hre, unsigned hbe, unsigned ht, unsigned vde, unsigned vbs, unsigned vrs, unsigned vre, unsigned vbe, unsigned vt, adv_bool hsync_pol, adv_bool vsync_pol, adv_bool doublescan, adv_bool interlace);
void log_f_modeline_c(const char* text, unsigned pixel_clock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool hsync_pol, adv_bool vsync_pol, adv_bool doublescan, adv_bool interlace);

/** \addtogroup Log */
/*@{*/

adv_error log_init(const char* file, adv_bool sync_flag);
void log_done(void);
void log_abort(void);
FILE* log_handle(void);

/**
 * Print something in the standard log file.
 * This function must be called using the double (( )) convention.
 * This convention allows the use of the printf format and to disable selectively at
 * compile time the log command.
 */
#define log_std(a) log_f a

/**
 * Print a modeline with blanking information in the standard log file.
 * This function must be called using the double (( )) convention.
 * This convention allows the use of the printf format and to disable selectively at
 * compile time the log command. 
 */
#define log_std_modeline_cb(a) log_f_modeline_cb a

/**
 * Print a modeline in the standard log file.
 * This function must be called using the double (( )) convention.
 * This convention allows the use of the printf format and to disable selectively at
 * compile time the log command. 
 */
#define log_std_modeline_c(a) log_f_modeline_c a

/**
 * Print a log entry in the debug output file.
 * This function must be called using the double (( )) convention.
 * This convention allows the use of the printf format and to disable selectively at
 * compile time the log command.  
 */
#ifndef NDEBUG
#define log_debug(a) log_f a
#else
#define log_debug(a) do { } while (0)
#endif

/**
 * Print something in the pedantic log file.
 * This function must be called using the double (( )) convention.
 * This convention allows the use of the printf format and to disable selectively at
 * compile time the log command.  
 */
#define log_pedantic(a) do { } while (0)

/*@}*/

#ifdef __cplusplus
}
#endif

#endif
