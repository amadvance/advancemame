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

#ifndef __INCSTR_H
#define __INCSTR_H

#include <stdio.h>
#include <string.h>

/**
 * Length of the first substring. 2^length.
 */
#define STR_MIN 8

/**
 * Max length of a substring. 2^length.
 */
#define STR_MAX (31 - STR_MIN)

struct inc_str {
	char buffer_map0[1 << STR_MIN]; /**< First substring */
	char* buffer_map[STR_MAX]; /**< Vector of substring */
	unsigned buffer_mac; /**< Number of substrings */
	unsigned current_mac; /**< Space used in the current substring */
	unsigned current_max; /**< Total space in the current substring */
	char* current; /**< Current substring */
	unsigned result_mac; /**< Current length */
};

/**
 * Initialize the string.
 */
void inc_str_init(struct inc_str* str);

/**
 * Deinitialize the string.
 */
void inc_str_done(struct inc_str* str);

/**
 * Cat a string.
 * \return
 *  - ==0 if ok
 *  - !=0 if error, errno set
 */
/**{*/
int inc_str_catm(struct inc_str* str, const char* s, unsigned len);
int inc_str_catc(struct inc_str* str, char c);
static __inline__ int inc_str_cat(struct inc_str* str, const char* s) {
	return inc_str_catm(str,s,strlen(s));
}
/**}*/

/**
 * Get the current string.
 * \return pointer to the string in the heap. It must be freed when unused.
 */
char* inc_str_alloc(struct inc_str* str);

/**
 * Get the current length of the string.
 * \return the length of the current string.
 */
unsigned inc_str_len(struct inc_str* str);

#endif
