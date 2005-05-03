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

#include "mnone.h"
#include "log.h"

static adv_device DEVICE[] = {
{ "auto", -1, "No mouse" },
{ 0, 0, 0 }
};

adv_error mouseb_none_init(int mouseb_id)
{
	log_std(("mouseb:none: mouseb_none_init(id:%d)\n", mouseb_id));

	return 0;
}

void mouseb_none_done(void)
{
	log_std(("mouseb:none: mouseb_none_done()\n"));
}

unsigned mouseb_none_count_get(void)
{
	log_debug(("mouseb:none: mouseb_none_count_get()\n"));

	return 0;
}

unsigned mouseb_none_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:none: mouseb_none_axe_count_get()\n"));

	return 2;
}

unsigned mouseb_none_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:none: mouseb_none_button_count_get()\n"));

	return 0;
}

int mouseb_none_axe_get(unsigned mouse, unsigned axe)
{
	log_debug(("mouseb:none: mouseb_none_pos_get()\n"));

	return 0;
}

unsigned mouseb_none_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:none: mouseb_none_button_get()\n"));

	return 0;
}

void mouseb_none_poll(void)
{
	log_debug(("mouseb:none: mouseb_none_poll()\n"));
}

unsigned mouseb_none_flags(void)
{
	return 0;
}

adv_error mouseb_none_load(adv_conf* context)
{
	return 0;
}

void mouseb_none_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_none_driver = {
	"none",
	DEVICE,
	mouseb_none_load,
	mouseb_none_reg,
	mouseb_none_init,
	mouseb_none_done,
	0,
	0,
	mouseb_none_flags,
	mouseb_none_count_get,
	mouseb_none_axe_count_get,
	0,
	mouseb_none_button_count_get,
	0,
	mouseb_none_axe_get,
	mouseb_none_button_get,
	mouseb_none_poll
};

