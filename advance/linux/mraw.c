/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2003 Andrea Mazzoleni
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

#include "mraw.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "portable.h"

#if defined(USE_SVGALIB)
#include <vga.h>
#include <vgamouse.h>
#endif

/* include the mouse driver */
#include "ms.c"

#define RAW_MOUSE_MAX 4

struct mouseb_raw_context {
	struct raw_mouse_context map[RAW_MOUSE_MAX]; /**< Mouse data. */
	int opened[RAW_MOUSE_MAX];
};

static struct mouseb_raw_context raw_state;

static adv_device DEVICE[] = {
{ "auto", -1, "RAW mouse" },
{ 0, 0, 0 }
};

adv_error mouseb_raw_init(int mouseb_id)
{
	unsigned i;
	adv_bool almost_one;

	log_std(("mouseb:raw: mouseb_raw_init(id:%d)\n", mouseb_id));

#if defined(USE_SVGALIB)
	/* close the SVGALIB mouse device, otherwise it doesn't work. */
	if (os_internal_svgalib_get()) {
		mouse_close();
	}
#endif

	almost_one = 0;
	for(i=0;i<RAW_MOUSE_MAX;++i) {
		log_std(("mouseb:raw: opening mouse %s\n", raw_state.map[i].dev));
		if (raw_mouse_init(&raw_state.map[i]) == 0) {
			raw_state.opened[i] = 1;
			almost_one = 1;
		} else {
			log_std(("mouseb:raw: error opening mouse device\n"));
			raw_state.opened[i] = 0;
		}
	}

	if (!almost_one) {
		return -1;
	}

	return 0;
}

void mouseb_raw_done(void)
{
	unsigned i;

	log_std(("mouseb:raw: mouseb_raw_done()\n"));

	for(i=0;i<RAW_MOUSE_MAX;++i) {
		if (raw_state.opened[i]) {
			raw_state.opened[i] = 0;
			raw_mouse_close(&raw_state.map[i]);
		}
	}
}

unsigned mouseb_raw_count_get(void)
{
	unsigned i;
	unsigned mac;

	log_debug(("mouseb:raw: mouseb_raw_count_get()\n"));

	mac = 0;
	for(i=0;i<RAW_MOUSE_MAX;++i) {
		if (raw_state.opened[i]) {
			mac = i + 1;
		}
	}

	return mac;
}

unsigned mouseb_raw_button_count_get(unsigned m)
{
	unsigned i;
	unsigned mb[6] = {
		MOUSE_LEFTBUTTON,
		MOUSE_RIGHTBUTTON,
		MOUSE_MIDDLEBUTTON,
		MOUSE_FOURTHBUTTON,
		MOUSE_FIFTHBUTTON,
		MOUSE_SIXTHBUTTON
	};

	log_debug(("mouseb:raw: mouseb_raw_button_count_get()\n"));

	assert( m < mouseb_raw_count_get());

	if (m >= RAW_MOUSE_MAX || !raw_state.opened[m])
		return 0;

	for(i=0;i<6;++i)
		if ((raw_state.map[m].info_button & mb[i]) == 0)
			break;

	return i;
}

void mouseb_raw_pos_get(unsigned m, int* x, int* y, int* z)
{
	log_debug(("mouseb:raw: mouseb_raw_pos_get()\n"));

	assert( m < mouseb_raw_count_get());

	if (m >= RAW_MOUSE_MAX || !raw_state.opened[m]) {
		*x = 0;
		*y = 0;
		*z = 0;
		return;
	}

	*x = raw_state.map[m].x;
	*y = raw_state.map[m].y;
	*z = raw_state.map[m].z;

	raw_state.map[m].x = 0;
	raw_state.map[m].y = 0;
	raw_state.map[m].z = 0;
}

unsigned mouseb_raw_button_get(unsigned m, unsigned b)
{
	unsigned mb[6] = {
		MOUSE_LEFTBUTTON,
		MOUSE_RIGHTBUTTON,
		MOUSE_MIDDLEBUTTON,
		MOUSE_FOURTHBUTTON,
		MOUSE_FIFTHBUTTON,
		MOUSE_SIXTHBUTTON
	};

	log_debug(("mouseb:raw: mouseb_raw_button_get()\n"));

	assert( m < mouseb_raw_count_get());
	assert( b < mouseb_raw_button_count_get(m) );

	if (m >= RAW_MOUSE_MAX || !raw_state.opened[m])
		return 0;

	if (b > 6)
		return 0;

	return (raw_state.map[m].button & mb[b]) != 0;
}

void mouseb_raw_poll(void)
{
	unsigned i;

	log_debug(("mouseb:raw: mouseb_raw_poll()\n"));

	for(i=0;i<RAW_MOUSE_MAX;++i) {
		if (raw_state.opened[i]) {
			raw_mouse_poll(&raw_state.map[i], 0);
		}
	}
}

unsigned mouseb_raw_flags(void)
{
	return 0;
}

adv_error mouseb_raw_load(adv_conf* context)
{
	unsigned i;
	for(i=0;i<RAW_MOUSE_MAX;++i) {
		char buf[64];
		const char* s;

		snprintf(buf, sizeof(buf), "device_raw_mousetype[%d]", i);
		raw_state.map[i].type = conf_int_get_default(context, buf);

		/* auto maps to pnp mouse */
		if (raw_state.map[i].type < 0)
			raw_state.map[i].type = MOUSE_PNP;

		snprintf(buf, sizeof(buf), "device_raw_mousedev[%d]", i);
		s = conf_string_get_default(context, buf);
		if (strcmp(s, "auto") == 0) {
			if (i == 0 && access("/dev/mouse", F_OK) == 0) {
				sncpy(raw_state.map[i].dev, sizeof(raw_state.map[i].dev), "/dev/mouse");
			} else {
				snprintf(raw_state.map[i].dev, sizeof(raw_state.map[i].dev), "/dev/input/mouse%d", i);
			}
		} else {
			sncpy(raw_state.map[i].dev, sizeof(raw_state.map[i].dev), s);
		}
	}

	for(i=0;i<RAW_MOUSE_MAX;++i) {
		log_std(("mouseb:raw: mouse %d on device %s\n", i, raw_state.map[i].dev));
		if (access(raw_state.map[i].dev, F_OK) != 0) {
			log_std(("mouseb:raw: device %s doesn't exist\n", raw_state.map[i].dev));
		} else if (access(raw_state.map[i].dev, W_OK | R_OK) != 0) {
			log_std(("mouseb:raw: access denied on %s\n", raw_state.map[i].dev));
		}
	}

	return 0;
}

static adv_conf_enum_int MOUSE_TYPE[] = {
{ "auto", -1 },
{ "pnp", MOUSE_PNP },
{ "ms", MOUSE_MICROSOFT },
{ "ms3", MOUSE_INTELLIMOUSE },
{ "ps2", MOUSE_PS2 },
{ "imps2", MOUSE_IMPS2 },
{ "exps2", MOUSE_EXPPS2 },
{ "msc", MOUSE_MOUSESYSTEMS },
{ "mscgpm", MOUSE_GPM },
{ "mman", MOUSE_LOGIMAN },
{ "mm", MOUSE_MMSERIES },
{ "logi", MOUSE_LOGITECH },
{ "bm", MOUSE_BUSMOUSE },
{ "spaceball", MOUSE_SPACEBALL },
{ "wacomgraphire", MOUSE_WACOM_GRAPHIRE },
{ "drmousee4ds", MOUSE_DRMOUSE4DS },
};

void mouseb_raw_reg(adv_conf* context)
{
	unsigned i;
	for(i=0;i<RAW_MOUSE_MAX;++i) {
		char buf[64];

		snprintf(buf, sizeof(buf), "device_raw_mousetype[%d]", i);
		conf_int_register_enum_default(context, buf, conf_enum(MOUSE_TYPE), MOUSE_PNP);

		snprintf(buf, sizeof(buf), "device_raw_mousedev[%d]", i);
		conf_string_register_default(context, buf, "auto");
	}
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_raw_driver = {
	"raw",
	DEVICE,
	mouseb_raw_load,
	mouseb_raw_reg,
	mouseb_raw_init,
	mouseb_raw_done,
	mouseb_raw_flags,
	mouseb_raw_count_get,
	mouseb_raw_button_count_get,
	mouseb_raw_pos_get,
	mouseb_raw_button_get,
	mouseb_raw_poll
};

