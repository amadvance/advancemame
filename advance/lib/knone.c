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

#include "knone.h"
#include "log.h"

static device DEVICE[] = {
{ "auto", -1, "No keyboard" },
{ 0, 0, 0 }
};

adv_error keyb_none_init(int keyb_id, adv_bool disable_special)
{
	log_std(("keyb:none: keyb_none_init(id:%d,disable_special:%d)\n",keyb_id,(int)disable_special));

	return 0;
}

void keyb_none_done(void)
{
	log_std(("keyb:none: keyb_none_done()\n"));
}

unsigned keyb_none_get(unsigned code)
{
	log_debug(("keyb:none: keyb_none_get()\n"));

	return 0;
}

void keyb_none_all_get(unsigned char* code_map)
{
	unsigned i;

	log_debug(("keyb:none: keyb_none_all_get()\n"));

	for(i=0;i<OS_KEY_MAX;++i) {
		code_map[i] = 0;
	}
}

void keyb_none_poll()
{
	log_debug(("keyb:none: keyb_none_poll()\n"));
}

unsigned keyb_none_flags(void)
{
	return 0;
}

adv_error keyb_none_load(struct conf_context* context)
{
	return 0;
}

void keyb_none_reg(struct conf_context* context)
{
}

/***************************************************************************/
/* Driver */

keyb_driver keyb_none_driver = {
	"none",
	DEVICE,
	keyb_none_load,
	keyb_none_reg,
	keyb_none_init,
	keyb_none_done,
	keyb_none_flags,
	keyb_none_get,
	keyb_none_all_get,
	keyb_none_poll
};


