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

#include "inone.h"
#include "log.h"

static adv_device DEVICE[] = {
{ "auto", -1, "No input" },
{ 0, 0, 0 }
};

adv_error inputb_none_init(int inputb_id)
{
	log_std(("inputb:none: inputb_none_init(id:%d)\n", inputb_id));

	return 0;
}

void inputb_none_done(void)
{
	log_std(("inputb:none: inputb_none_done()\n"));
}

adv_error inputb_none_enable(adv_bool graphics)
{
	log_std(("inputb:none: inputb_none_enable(graphics:%d)\n", (int)graphics));

	return 0;
}

void inputb_none_disable(void)
{
	log_std(("inputb:none: inputb_none_disable()\n"));
}

adv_bool inputb_none_hit(void)
{
	log_debug(("inputb:none: inputb_none_pos_get()\n"));

	return 0;
}

unsigned inputb_none_get(void)
{
	log_debug(("inputb:none: inputb_none_button_get()\n"));

	return 0;
}

unsigned inputb_none_flags(void)
{
	return 0;
}

adv_error inputb_none_load(adv_conf* context)
{
	return 0;
}

void inputb_none_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

inputb_driver inputb_none_driver = {
	"none",
	DEVICE,
	inputb_none_load,
	inputb_none_reg,
	inputb_none_init,
	inputb_none_done,
	inputb_none_enable,
	inputb_none_disable,
	inputb_none_flags,
	inputb_none_hit,
	inputb_none_get
};

