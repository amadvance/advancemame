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

#include "advstd.h"
#include "log.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

/****************************************************************************/
/* Error */

/**
 * Max length of the error description.
 */
#define ERROR_DESC_MAX 2048

static char error[ERROR_DESC_MAX]; /**< Last error description. */

/** 
 * Description of the last error. 
 */
const char* error_description_get(void) {
	/* remove the trailing \n */
	while (error[0] && isspace(error[strlen(error)-1]))
		error[strlen(error)-1] = 0;
	return error;
}

/**
 * Set the description of the last error.
 * \note The description IS logged.
 */
void error_description_set(const char* text, ...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(error,text,arg);
	log_std(("video: set_error_description \""));
	log_va(text,arg);
	log_std(("\"\n"));
	va_end(arg);
}

/**
 * Set the description of the last error.
 * \note The description IS NOT logged.
 */
void error_description_nolog_set(const char* text, ...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(error,text,arg);
	va_end(arg);
}

/**
 * Add some text at the description of the last error.
 * \note The description IS NOT logged.
 */
void error_description_nolog_cat(const char* text, ...)
{
	va_list arg;
	char buffer[ERROR_DESC_MAX];
	va_start(arg,text);
	vsprintf(buffer,text,arg);

	strncat(error,buffer,ERROR_DESC_MAX);
	error[ERROR_DESC_MAX-1] = 0;

	va_end(arg);
}


