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

#ifndef __OSDUTILS_H
#define __OSDUTILS_H

#include <string.h>
#include <ctype.h>

#include <stdio.h> /* for vsnprintf */
#include <stdarg.h> /* for vsnprintf */

#include <sys/stat.h> /* for mkdir */
#include <sys/types.h> /* for mkdir */

#ifdef __WIN32__
#include <io.h> /* for mkdir */
#endif

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

#ifndef __WIN32__
#define strcmpi strcmpi /* For some #ifdef */
static inline int strcmpi(const char* a, const char* b) /* LEGACY: Still used in MESS  */
{
	return strcasecmp(a, b);
}
#endif

#ifndef __WIN32__
#define strncmpi strncmpi /* For some #ifdef */
static inline int strncmpi(const char* a, const char* b, size_t count) /* LEGACY: Still used in MESS  */
{
	return strncasecmp(a, b, count);
}
#endif

#if !defined(__MSDOS__) && !defined(__WIN32__)
static inline void strlwr(char* s)
{
	while (*s) {
		*s = tolower(*s);
		++s;
	}
}
#endif

#if !defined(__MSDOS__) && !defined(__WIN32__)
static inline void strupr(char* s)
{
	while (*s) {
		*s = toupper(*s);
		++s;
	}
}
#endif

static inline void osd_mkdir(const char* dir)
{
#ifdef __WIN32__
	mkdir(dir);
#else
	mkdir(dir, S_IRWXU | S_IRGRP | S_IROTH);
#endif
}

/**
 * Selects the CRLF convention to use for output files.
 * - 1 CR (Mac)
 * - 2 LF (*nix)
 * - 3 CRLF (Dos/Windows)
 */
#define CRLF 2 /* Always use the UNIX convention */

#define EOLN "\n" /* LEGACY: Still used in MESS */

#if defined(__MSDOS__) || defined(__WIN32__)
#define PATH_SEPARATOR '\\' /* LEGACY: Still used in MESS  */
#define PATH_SEPARATOR_STR "\\" /* LEGACY: Still used in MESS  */
#else
#define PATH_SEPARATOR '/' /* LEGACY: Still used in MESS */
#define PATH_SEPARATOR_STR "/" /* LEGACY: Still used in MESS  */
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __MSDOS__
int rpl_snprintf(char* str, size_t count, const char* fmt, ...);
int rpl_vsnprintf(char* str, size_t count, const char* fmt, va_list arg);
#define snprintf rpl_snprintf
#define vsnprintf rpl_vsnprintf
#endif

#ifdef __WIN32__
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#ifdef __WIN32__
#if __GNUC__ == 2
/* math functions for gcc 2.95.3 for Windows */
double rpl_asinh(double x);
double rpl_acosh(double x);
double rpl_alogb(double x);
int rpl_isnan(double x);
int rpl_isunordered(double x, double y);
#define asinh rpl_asinh
#define acosh rpl_acosh
#define logb rpl_logb
#define isnan rpl_isnan
#define isunordered rpl_isunordered
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

