/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002 Andrea Mazzoleni
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

#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Log */

int log_init(const char* file, int sync_flag);
void log_done(void);
void log_abort(void);

void log_va(const char *text, va_list arg) __attribute__((format(printf,1,0)));
void log_f(const char *text, ...) __attribute__((format(printf,1,2)));

#ifndef NDEBUG
#define log_std(a) log_f a
#define log_debug(a) log_f a
#define log_pedantic(a) do { } while (0)
#else
#define log_std(a) log_f a
#define log_debug(a) do { } while (0)
#define log_pedantic(a) do { } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif
