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

#include "idos.h"
#include "log.h"
#include "target.h"

#include <pc.h>
#include <keys.h>

static adv_device DEVICE[] = {
{ "auto", -1, "DOS input" },
{ 0, 0, 0 }
};

static struct keyb_pair {
	int os;
	int code;
} KEYS[] = {
{ INPUTB_TAB, K_Tab },
{ INPUTB_ENTER, K_Return },
{ INPUTB_ESC, K_Escape },
{ INPUTB_SPACE, K_Space },
{ INPUTB_UP, K_Up },
{ INPUTB_DOWN, K_Down },
{ INPUTB_LEFT, K_Left },
{ INPUTB_RIGHT, K_Right },
{ INPUTB_HOME, K_Home },
{ INPUTB_END, K_End },
{ INPUTB_PGUP, K_PageUp },
{ INPUTB_PGDN, K_PageDown },
{ INPUTB_F1, K_F1 },
{ INPUTB_F2, K_F2 },
{ INPUTB_F3, K_F3 },
{ INPUTB_F4, K_F4 },
{ INPUTB_F5, K_F5 },
{ INPUTB_F6, K_F6 },
{ INPUTB_F7, K_F7 },
{ INPUTB_F8, K_F8 },
{ INPUTB_F9, K_F9 },
{ INPUTB_F10, K_F10 },
{ INPUTB_BACKSPACE, K_BackSpace },
{ INPUTB_DEL, K_Delete },
{ INPUTB_INS, K_Insert },
{ INPUTB_MAX, 0 }
};

adv_error inputb_dos_init(int inputb_id)
{
	log_std(("input:dos: inputb_dos_init(id:%d)\n", inputb_id));

	return 0;
}

void inputb_dos_done(void)
{
	log_std(("input:dos: inputb_dos_done()\n"));
}

adv_error inputb_dos_enable(adv_bool graphics)
{
	log_std(("input:dos: inputb_dos_enable(graphics:%d)\n", (int)graphics));

	return 0;
}

void inputb_dos_disable(void)
{
	log_std(("input:dos: inputb_dos_disable()\n"));
}

adv_bool inputb_dos_hit(void)
{
	log_debug(("inputb:dos: inputb_dos_count_get()\n"));

	return kbhit();
}

unsigned inputb_dos_get(void)
{
	int code = getkey();
	struct keyb_pair* i;

	for(i=KEYS;i->os!=INPUTB_MAX;++i)
		if (i->code == code)
			return i->os;

	return code;
}

unsigned inputb_dos_flags(void)
{
	return 0;
}

adv_error inputb_dos_load(adv_conf* context)
{
	return 0;
}

void inputb_dos_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

inputb_driver inputb_dos_driver = {
	"dos",
	DEVICE,
	inputb_dos_load,
	inputb_dos_reg,
	inputb_dos_init,
	inputb_dos_done,
	inputb_dos_enable,
	inputb_dos_disable,
	inputb_dos_flags,
	inputb_dos_hit,
	inputb_dos_get
};

