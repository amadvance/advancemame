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

#include "islang.h"
#include "log.h"
#include "target.h"
#include "osint.h"

#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

struct inputb_slang_context {
	unsigned last;
};

static struct inputb_slang_context slang_state;

static device DEVICE[] = {
{ "auto", -1, "sLang input" },
{ 0, 0, 0 }
};

video_error inputb_slang_init(int inputb_id)
{
	log_std(("inputb:slang: inputb_slang_init(id:%d)\n",inputb_id));

	if (!os_internal_slang_get()) {
		log_std(("inputb:slang: slang not initialized\n"));
		return -1;
	}

	slang_state.last = 0;

	return 0;
}

void inputb_slang_done(void)
{
	log_std(("inputb:slang: inputb_slang_done()\n"));
}

static int slang_getkey(void)
{
	struct timeval tv;
	fd_set fds;
	int fd = fileno(stdin);
	char c;

	tv.tv_sec = tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (select(fd + 1, &fds, 0, 0, &tv) > 0) {
		if (read(fileno(stdin), &c, 1) != 1) {
			return 0;
		}
		return c;
	}

	return 0;
}

video_bool inputb_slang_hit(void)
{
	log_debug(("inputb:slang: inputb_slang_count_get()\n"));

	if (slang_state.last != 0)
		return 1;

	slang_state.last = slang_getkey();
	return slang_state.last != 0;
}

unsigned inputb_slang_get(void)
{
	const unsigned max = 32;
	char map[max+1];
	unsigned mac;

	log_debug(("inputb:slang: inputb_slang_button_count_get()\n"));

	mac = 0;
	while (mac<max && (mac==0 || slang_state.last)) {

		if (slang_state.last) {
			map[mac] = slang_state.last;
			if (mac > 0 && map[mac] == 27) {
				break;
			}
			++mac;
			slang_state.last = 0;
		} else {
			target_idle();
		}

		slang_state.last = slang_getkey();
	}
	map[mac] = 0;

	if (strcmp(map,"\033[A")==0)
		return OS_INPUT_UP;
	if (strcmp(map,"\033[B")==0)
		return OS_INPUT_DOWN;
	if (strcmp(map,"\033[D")==0)
		return OS_INPUT_LEFT;
	if (strcmp(map,"\033[C")==0)
		return OS_INPUT_RIGHT;
	if (strcmp(map,"\033[1~")==0)
		return OS_INPUT_HOME;
	if (strcmp(map,"\033[4~")==0)
		return OS_INPUT_END;
	if (strcmp(map,"\033[5~")==0)
		return OS_INPUT_PGUP;
	if (strcmp(map,"\033[6~")==0)
		return OS_INPUT_PGDN;
	if (strcmp(map,"\033[[A")==0)
		return OS_INPUT_F1;
	if (strcmp(map,"\033[[B")==0)
		return OS_INPUT_F2;
	if (strcmp(map,"\033[[C")==0)
		return OS_INPUT_F3;
	if (strcmp(map,"\033[[D")==0)
		return OS_INPUT_F4;
	if (strcmp(map,"\033[[E")==0)
		return OS_INPUT_F5;
	if (strcmp(map,"\033[17~")==0)
		return OS_INPUT_F6;
	if (strcmp(map,"\033[18~")==0)
		return OS_INPUT_F7;
	if (strcmp(map,"\033[19~")==0)
		return OS_INPUT_F8;
	if (strcmp(map,"\033[20~")==0)
		return OS_INPUT_F9;
	if (strcmp(map,"\033[21~")==0)
		return OS_INPUT_F10;
	if (strcmp(map,"\r")==0 || strcmp(map,"\n")==0)
		return OS_INPUT_ENTER;
	if (strcmp(map,"\x7F")==0)
		return OS_INPUT_BACKSPACE;

	if (mac != 1)
		return 0;
	else
		return (unsigned char)map[0];

	return 0;
}

unsigned inputb_slang_flags(void)
{
	return 0;
}

video_error inputb_slang_load(struct conf_context* context)
{
	return 0;
}

void inputb_slang_reg(struct conf_context* context)
{
}

/***************************************************************************/
/* Driver */

inputb_driver inputb_slang_driver = {
	"slang",
	DEVICE,
	inputb_slang_load,
	inputb_slang_reg,
	inputb_slang_init,
	inputb_slang_done,
	inputb_slang_flags,
	inputb_slang_hit,
	inputb_slang_get
};

