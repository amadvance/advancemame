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
 * Functions and defines required for portability.
 */

#ifndef __PORTABLE_H
#define __PORTABLE_H

#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

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
/* wchar */

#ifdef __MSDOS__

#include <wchar.h>

static inline wchar_t towlower(wchar_t c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';
	else
		return c;
}

static inline wchar_t towupper(wchar_t c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	else
		return c;
}
#endif

/***************************************************************************/
/* strcmpi */

#ifndef __WIN32__
#define strcmpi strcmpi /* For some #ifdef */
static inline int strcmpi(const char* a, const char* b)
{
	return strcasecmp(a, b);
}
#endif

#ifndef __WIN32__
#define strncmpi strncmpi /* For some #ifdef */
static inline int strncmpi(const char* a, const char* b, size_t count)
{
	return strncasecmp(a, b, count);
}
#endif

/***************************************************************************/
/* strlwr */

#if !defined(__MSDOS__) && !defined(__WIN32__)
static inline void strlwr(char* s) {
	while (*s) {
		*s = tolower(*s);
		++s;
	}
}
#endif

/***************************************************************************/
/* strupr */

#if !defined(__MSDOS__) && !defined(__WIN32__)
static inline void strupr(char* s) {
	while (*s) {
		*s = toupper(*s);
		++s;
	}
}
#endif

/***************************************************************************/
/* strzcpy */

void sncpy(char* dst, size_t len, const char* src);
void sncat(char* dst, size_t len, const char* src);

#ifdef __cplusplus
}
#endif

#endif

