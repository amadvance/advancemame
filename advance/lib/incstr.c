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

#include "portable.h"

#include "incstr.h"

void inc_str_init(adv_string* str)
{
	str->buffer_mac = 0;
	str->current_mac = 0;
	str->current_max = 1 << STR_MIN;
	str->buffer_map[0] = str->buffer_map0;
	str->result_mac = 0;
	str->current = str->buffer_map[0];
}

void inc_str_done(adv_string* str)
{
	unsigned i;
	for(i=1;i<=str->buffer_mac;++i)
		free(str->buffer_map[i]);
}

static adv_error inc_str_step(adv_string* str)
{
	if (str->buffer_mac == STR_MAX) {
		return -1;
	}

	str->current_mac = 0;
	str->current_max = 1 << (str->buffer_mac + 1 + STR_MIN);
	str->current = malloc(str->current_max);
	if (!str->current) {
		return -1;
	}

	++str->buffer_mac;
	str->buffer_map[str->buffer_mac] = str->current;

	return 0;
}

adv_error inc_str_catn(adv_string* str, const char* s, unsigned len)
{
	while (1) {
		unsigned run = str->current_max - str->current_mac;
		if (run > len)
			run = len;

		memcpy(str->current + str->current_mac, s, run);
		str->current_mac += run;
		str->result_mac += run;
		len -= run;

		if (!len)
			break;

		if (inc_str_step(str) != 0)
			return -1;
	}

	return 0;
}

adv_error inc_str_catc(adv_string* str, char c)
{
	if (str->current_max == str->current_mac)
		if (inc_str_step(str) != 0)
			return -1;

	str->current[str->current_mac] = c;
	++str->current_mac;
	++str->result_mac;

	return 0;
}

unsigned inc_str_len(adv_string* str)
{
	return str->result_mac;
}

char* inc_str_alloc(adv_string* str)
{
	char* result;
	unsigned size;
	unsigned i;

	result = malloc(str->result_mac + 1);
	if (!result) {
		return 0;
	}

	/* copy on the buffer */
	size = 0;
	for(i=0;i<str->buffer_mac;++i) {
		unsigned len = 1 << (i + 8);
		memcpy(result + size, str->buffer_map[i], len);
		size += len;
	}

	if (str->current_mac)
		memcpy(result + size, str->current, str->current_mac);
	size += str->current_mac;

	result[size] = 0;

	return result;
}
