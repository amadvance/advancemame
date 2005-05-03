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

#include "malleg.h"
#include "log.h"
#include "target.h"

#include "allegro2.h"

struct mouse_context {
	unsigned buttons_counter; /**< Number of buttons. */
	int x; /**< Current x pos. */
	int y; /**< Current y pos. */
};

struct mouseb_allegro_context {
	struct mouse_context mouse[2]; /**< Mouse context. */
	unsigned secondary_at_index; /**< Relative position of the secondary mouse. */
	unsigned counter; /**< Number of active mouses. */

	int mouse2_button; /**< State of secondary mouse buttons. */
};

static struct mouseb_allegro_context allegro_state;

static adv_device DEVICE[] = {
{ "auto", -1, "Allegro mouse" },
{ 0, 0, 0 }
};

/***************************************************************************/
/* Second mouse */

static int mouse2_init(void)
{
	__dpmi_regs r;

	r.x.ax = 100;
        __dpmi_int(0x33, &r);
	if (r.x.ax == 100 || r.x.ax == 0)
		return -1;

	return 0;
}

static void mouse2_get(int* x, int* y)
{
	__dpmi_regs r;

	r.x.ax = 103;
	__dpmi_int(0x33, &r);

	allegro_state.mouse2_button = r.x.bx;

	r.x.ax = 111;
	__dpmi_int(0x33, &r);

	*x = (short)r.x.cx;
	*y = (short)r.x.dx;
}

/***************************************************************************/
/* Allegro Mouse */

adv_error mouseb_allegro_init(int mouseb_id)
{
	int err;

	log_std(("mouseb:allegro: mouseb_allegro_init(id:%d)\n", mouseb_id));

	allegro_state.counter = 0;

	err = install_mouse();
	if (err != -1) {
		allegro_state.mouse[allegro_state.counter].buttons_counter = err;
		allegro_state.mouse[allegro_state.counter].x = 0;
		allegro_state.mouse[allegro_state.counter].y = 0;
		++allegro_state.counter;
		log_std(("mouseb:allegro: allegro mouse found\n"));
		allegro_state.secondary_at_index = 1;
	} else {
		log_std(("mouseb:allegro: allegro mouse NOT found\n"));
		allegro_state.secondary_at_index = 0;
	}
	if (mouse2_init() == 0) {
		allegro_state.mouse[allegro_state.counter].buttons_counter = 2;
		allegro_state.mouse[allegro_state.counter].x = 0;
		allegro_state.mouse[allegro_state.counter].y = 0;
		++allegro_state.counter;
		log_std(("mouseb:allegro: secondary mouse found\n"));
	} else {
		log_std(("mouseb:allegro: secondary mouse NOT found\n"));
	}

	return 0;
}

void mouseb_allegro_done(void)
{
	log_std(("mouse:allegro: mouseb_allegro_done()\n"));

	remove_mouse();
}

unsigned mouseb_allegro_count_get(void)
{
	log_debug(("mouseb:allegro: mouseb_allegro_count_get()\n"));

	return allegro_state.counter;
}

unsigned mouseb_allegro_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:allegro: mouseb_allegro_axe_count_get()\n"));

	return 2;
}

unsigned mouseb_allegro_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:allegro: mouseb_allegro_button_count_get()\n"));

	return allegro_state.mouse[mouse].buttons_counter;
}

int mouseb_allegro_axe_get(unsigned mouse, unsigned axe)
{
	int r;

	log_debug(("mouseb:allegro: mouseb_allegro_pos_get()\n"));

	switch (axe) {
	case 0 : r = allegro_state.mouse[mouse].x; allegro_state.mouse[mouse].x = 0; break;
	case 1 : r = allegro_state.mouse[mouse].y; allegro_state.mouse[mouse].y = 0; break;
	default : r = 0;
	}

	return r;
}

unsigned mouseb_allegro_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:allegro: mouseb_allegro_button_get()\n"));

	if (mouse == allegro_state.secondary_at_index) {
		return (allegro_state.mouse2_button & (1 << button)) != 0;
	} else {
		return (mouse_b & (1 << button)) != 0;
	}

	return 0;
}

void mouseb_allegro_poll(void)
{
	unsigned i;

	log_debug(("mouseb:allegro: mouseb_allegro_poll()\n"));

	for(i=0;i<allegro_state.counter;++i) {
		int ax, ay;
		if (i == allegro_state.secondary_at_index) {
			mouse2_get(&ax, &ay);
		} else {
			get_mouse_mickeys(&ax, &ay);
		}

		allegro_state.mouse[i].x += ax;
		allegro_state.mouse[i].y += ay;
	}
}

unsigned mouseb_allegro_flags(void)
{
	return 0;
}

adv_error mouseb_allegro_load(adv_conf* context)
{
	return 0;
}

void mouseb_allegro_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_allegro_driver = {
	"allegro",
	DEVICE,
	mouseb_allegro_load,
	mouseb_allegro_reg,
	mouseb_allegro_init,
	mouseb_allegro_done,
	0,
	0,
	mouseb_allegro_flags,
	mouseb_allegro_count_get,
	mouseb_allegro_axe_count_get,
	0,
	mouseb_allegro_button_count_get,
	0,
	mouseb_allegro_axe_get,
	mouseb_allegro_button_get,
	mouseb_allegro_poll
};

