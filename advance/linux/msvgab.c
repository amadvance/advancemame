/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#include "msvgab.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"

#include <vgamouse.h>

#define BUTTON_MAX 16

struct mouseb_svgalib_context {
	int x;
	int y;
	int button_mask;
	unsigned button_mac;
	unsigned button_map[BUTTON_MAX];
};

static struct mouseb_svgalib_context svgalib_state;

static adv_device DEVICE[] = {
{ "auto", -1, "SVGALIB mouse" },
{ 0, 0, 0 }
};

adv_error mouseb_svgalib_init(int mouseb_id)
{
	struct MouseCaps mouse_caps;
	unsigned i;
	unsigned buttons[] = {
		MOUSE_LEFTBUTTON,
		MOUSE_RIGHTBUTTON,
		MOUSE_MIDDLEBUTTON,
		MOUSE_FOURTHBUTTON,
		MOUSE_FIFTHBUTTON,
		MOUSE_SIXTHBUTTON,
		MOUSE_RESETBUTTON,
		0
	};

	log_std(("mouseb:svgalib: mouseb_svgalib_init(id:%d)\n", mouseb_id));

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	if (!os_internal_svgalib_get()) {
		error_set("Not supported without the svgalib library.\n");
		return -1;
	}

	/* already opened internally by svgalib */

	if (mouse_getcaps(&mouse_caps)!=0) {
		error_set("No mouse found.\n");
		return -1;
	}

	mouse_setxrange(-8191, 8191);
	mouse_setyrange(-8191, 8191);
	mouse_setscale(1);
	mouse_setwrap(MOUSE_NOWRAP);

	svgalib_state.button_mac = 0;
	for(i=0;buttons[i] && i<BUTTON_MAX;++i) {
		if ((mouse_caps.buttons & buttons[i]) != 0) {
			svgalib_state.button_map[svgalib_state.button_mac] = buttons[i];
			++svgalib_state.button_mac;
		}
	}

	svgalib_state.x = 0;
	svgalib_state.y = 0;
	svgalib_state.button_mask = 0;

	return 0;
}

void mouseb_svgalib_done(void)
{
	log_std(("mouseb:svgalib: mouseb_svgalib_done()\n"));
}

unsigned mouseb_svgalib_count_get(void)
{
	log_debug(("mouseb:svgalib: mouseb_svgalib_count_get()\n"));

	return 1;
}
unsigned mouseb_svgalib_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:svgalib: mouseb_svgalib_axe_count_get()\n"));

	return 2;
}

unsigned mouseb_svgalib_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:svgalib: mouseb_svgalib_button_count_get()\n"));

	return svgalib_state.button_mac;
}

int mouseb_svgalib_axe_get(unsigned mouse, unsigned axe)
{
	int r;

	log_debug(("mouseb:svgalib: mouseb_svgalib_pos_get()\n"));

	switch (axe) {
	case 0 :
		r = svgalib_state.x;
		svgalib_state.x = 0;
		break;
	case 1 :
		r = svgalib_state.y;
		svgalib_state.y = 0;
		break;
	default:
		r = 0;
		break;
	}

	return r;
}

unsigned mouseb_svgalib_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:svgalib: mouseb_svgalib_button_get()\n"));

	return (svgalib_state.button_mask & svgalib_state.button_map[button]) != 0;
}

void mouseb_svgalib_poll(void)
{
	log_debug(("mouseb:svgalib: mouseb_svgalib_poll()\n"));

	/* update the position */
	mouse_update();

	/* get the new position */
	svgalib_state.x += mouse_getx();
	svgalib_state.y += mouse_gety();
	svgalib_state.button_mask = mouse_getbutton();

	/* clear the current position */
	mouse_setposition(0, 0);

	/* the range must be reset on a video mode change */
	mouse_setxrange(-8191, 8191);
	mouse_setyrange(-8191, 8191);

	log_debug(("mouseb:svgalib: mouseb_svgalib_poll() -> %d,%d,%d\n", svgalib_state.x, svgalib_state.y, svgalib_state.button_mask));
}

unsigned mouseb_svgalib_flags(void)
{
	return 0;
}

adv_error mouseb_svgalib_load(adv_conf* context)
{
	return 0;
}

void mouseb_svgalib_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_svgalib_driver = {
	"svgalib",
	DEVICE,
	mouseb_svgalib_load,
	mouseb_svgalib_reg,
	mouseb_svgalib_init,
	mouseb_svgalib_done,
	0,
	0,
	mouseb_svgalib_flags,
	mouseb_svgalib_count_get,
	mouseb_svgalib_axe_count_get,
	0,
	mouseb_svgalib_button_count_get,
	0,
	mouseb_svgalib_axe_get,
	mouseb_svgalib_button_get,
	mouseb_svgalib_poll
};

