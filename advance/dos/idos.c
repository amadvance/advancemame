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

#include "idos.h"
#include "log.h"
#include "target.h"

#include <string.h>

#include <pc.h>
#include <keys.h>

static device DEVICE[] = {
{ "auto", -1, "DOS input" },
{ 0, 0, 0 }
};

static struct keyb_pair {
	int os;
	int code;
} KEYS[] = {
{ OS_INPUT_TAB, K_Tab },
{ OS_INPUT_ENTER, K_Return },
{ OS_INPUT_ESC, K_Escape },
{ OS_INPUT_SPACE, K_Space },
{ OS_INPUT_UP, K_Up },
{ OS_INPUT_DOWN, K_Down },
{ OS_INPUT_LEFT, K_Left },
{ OS_INPUT_RIGHT, K_Right },
{ OS_INPUT_HOME, K_Home },
{ OS_INPUT_END, K_End },
{ OS_INPUT_PGUP, K_PageUp },
{ OS_INPUT_PGDN, K_PageDown },
{ OS_INPUT_F1, K_F1 },
{ OS_INPUT_F2, K_F2 },
{ OS_INPUT_F3, K_F3 },
{ OS_INPUT_F4, K_F4 },
{ OS_INPUT_F5, K_F5 },
{ OS_INPUT_F6, K_F6 },
{ OS_INPUT_F7, K_F7 },
{ OS_INPUT_F8, K_F8 },
{ OS_INPUT_F9, K_F9 },
{ OS_INPUT_F10, K_F10 },
{ OS_INPUT_BACKSPACE, K_BackSpace },
{ OS_INPUT_DEL, K_Delete },
{ OS_INPUT_INS, K_Insert },
{ OS_INPUT_MAX, 0 }
};

adv_error inputb_dos_init(int inputb_id)
{
	log_std(("input:dos: inputb_dos_init(id:%d)\n",inputb_id));

	return 0;
}

void inputb_dos_done(void)
{
	log_std(("input:dos: inputb_dos_done()\n"));
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

	for(i=KEYS;i->os!=OS_INPUT_MAX;++i)
		if (i->code == code)
			return i->os;

	return code;
}

unsigned inputb_dos_flags(void)
{
	return 0;
}

adv_error inputb_dos_load(struct conf_context* context)
{
	return 0;
}

void inputb_dos_reg(struct conf_context* context)
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
	inputb_dos_flags,
	inputb_dos_hit,
	inputb_dos_get
};

