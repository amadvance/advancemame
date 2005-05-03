/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2004, 2005 Andrea Mazzoleni
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

#include "mraw.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "snstring.h"

#if defined(USE_SVGALIB)
#include <vga.h>
#include <vgamouse.h>
#endif

/* include the mouse driver */
#include "ms.c"

#define RAW_MOUSE_MAX 4
#define RAW_MOUSE_NAME_MAX 128
#define RAW_MOUSE_BUTTON_MAX 8
#define RAW_MOUSE_AXE_MAX 8

struct mouse_button_context {
	unsigned code;
	int* pvalue;
	char name[RAW_MOUSE_NAME_MAX];
};

struct mouse_axe_context {
	unsigned code;
	int* pvalue;
	char name[RAW_MOUSE_NAME_MAX];
};

struct mouse_item_context {
	adv_bool active_flag;
	struct raw_mouse_context context;
	unsigned button_mac;
	struct mouse_button_context button_map[RAW_MOUSE_BUTTON_MAX];
	unsigned axe_mac;
	struct mouse_axe_context axe_map[RAW_MOUSE_AXE_MAX];
};

struct mouseb_raw_context {
	unsigned mac;
	struct mouse_item_context map[RAW_MOUSE_MAX];
};

static struct mouseb_raw_context raw_state;

static adv_device DEVICE[] = {
{ "auto", -1, "RAW mouse" },
{ 0, 0, 0 }
};

static void mouseb_setup(struct mouse_item_context* item)
{
	unsigned i;
	struct button_entry {
		int code;
		const char* name;
	} button_map[] = {
		{ MOUSE_LEFTBUTTON, "left" },
		{ MOUSE_RIGHTBUTTON, "right" },
		{ MOUSE_MIDDLEBUTTON, "middle" },
		{ MOUSE_FOURTHBUTTON, "button4" },
		{ MOUSE_FIFTHBUTTON, "button5" },
		{ MOUSE_SIXTHBUTTON, "button6" }
	};

	struct axe_entry {
		int code;
		const char* name;
	} axe_map[] = {
		{ MOUSE_XDIM, "x" },
		{ MOUSE_YDIM, "y" },
		{ MOUSE_ZDIM, "z" },
		{ MOUSE_RXDIM, "rx" },
		{ MOUSE_RYDIM, "ry" },
		{ MOUSE_RZDIM, "rz" }
	};

	item->button_mac = 0;
	for(i=0;i<sizeof(button_map)/sizeof(button_map[0]);++i) {
		if ((item->context.info_button & button_map[i].code) != 0) {
			if (item->button_mac < RAW_MOUSE_BUTTON_MAX) {
				item->button_map[item->button_mac].code = button_map[i].code;
				item->button_map[item->button_mac].pvalue = &item->context.button;
				sncpy(item->button_map[item->button_mac].name, sizeof(item->button_map[item->button_mac].name), button_map[i].name);
				++item->button_mac;
			}
		}
	}

	item->axe_mac = 0;
	for(i=0;i<sizeof(axe_map)/sizeof(axe_map[0]);++i) {
		if ((item->context.info_button & axe_map[i].code) != 0) {
			if (item->axe_mac < RAW_MOUSE_AXE_MAX) {
				item->axe_map[item->axe_mac].code = axe_map[i].code;
				switch (axe_map[i].code) {
				case MOUSE_XDIM : item->axe_map[item->axe_mac].pvalue = &item->context.x; break;
				case MOUSE_YDIM : item->axe_map[item->axe_mac].pvalue = &item->context.y; break;
				case MOUSE_ZDIM : item->axe_map[item->axe_mac].pvalue = &item->context.z; break;
				case MOUSE_RXDIM : item->axe_map[item->axe_mac].pvalue = &item->context.rx; break;
				case MOUSE_RYDIM : item->axe_map[item->axe_mac].pvalue = &item->context.ry; break;
				case MOUSE_RZDIM : item->axe_map[item->axe_mac].pvalue = &item->context.rz; break;
				}
				sncpy(item->axe_map[item->axe_mac].name, sizeof(item->axe_map[item->axe_mac].name), axe_map[i].name);
				++item->axe_mac;
			}
		}
	}
}

adv_error mouseb_raw_init(int mouseb_id)
{
	unsigned i;
	adv_bool eacces = 0;

	log_std(("mouseb:raw: mouseb_raw_init(id:%d)\n", mouseb_id));

#if defined(USE_SVGALIB)
	/* close the SVGALIB mouse device. SVGALIB always call mouse_init(), also */
	/* if mouse input is not requested */
	if (os_internal_svgalib_get()) {
		mouse_close();
	}
#endif

	raw_state.mac = 0;
	for(i=0;i<RAW_MOUSE_MAX;++i) {
		if (raw_mouse_init(&raw_state.map[i].context) == 0) {
			log_std(("mouseb:raw: open device %s\n", raw_state.map[i].context.dev));

			mouseb_setup(&raw_state.map[i]);

			raw_state.map[i].active_flag = 1;
			raw_state.mac = i + 1;
		} else {
			if (errno != ENODEV) {
				log_std(("ERROR:mouseb:raw: error opening device %s, errno %d (%s)\n", raw_state.map[i].context.dev, errno, strerror(errno)));
			}
			if (errno == EACCES) {
				eacces = 1;
			}
			raw_state.map[i].active_flag = 0;
		}
	}

	if (raw_state.mac == 0) {
		if (eacces)
			error_set("No mouse found. Check the /dev/mouse and /dev/input/mouse* permissions.\n");
		else
			error_set("No mouse found.\n");
		return -1;
	}

	return 0;
}

void mouseb_raw_done(void)
{
	unsigned i;

	log_std(("mouseb:raw: mouseb_raw_done()\n"));

	for(i=0;i<raw_state.mac;++i) {
		if (raw_state.map[i].active_flag) {
			raw_mouse_close(&raw_state.map[i].context);
		}
	}

	raw_state.mac = 0;
}

unsigned mouseb_raw_count_get(void)
{
	log_debug(("mouseb:raw: mouseb_raw_count_get()\n"));

	return raw_state.mac;
}

unsigned mouseb_raw_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:raw: mouseb_raw_axe_count_get()\n"));

	return raw_state.map[mouse].axe_mac;
}

const char* mouseb_raw_axe_name_get(unsigned mouse, unsigned axe)
{
	log_debug(("mouseb:raw: mouseb_raw_axe_name_get()\n"));

	return raw_state.map[mouse].axe_map[axe].name;
}

unsigned mouseb_raw_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:raw: mouseb_raw_button_count_get()\n"));

	return raw_state.map[mouse].button_mac;
}

const char* mouseb_raw_button_name_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:raw: mouseb_raw_button_name_get()\n"));

	return raw_state.map[mouse].button_map[button].name;
}

int mouseb_raw_axe_get(unsigned mouse, unsigned axe)
{
	int r;

	log_debug(("mouseb:raw: mouseb_raw_pos_get()\n"));

	r = *raw_state.map[mouse].axe_map[axe].pvalue;
	*raw_state.map[mouse].axe_map[axe].pvalue = 0;

	return r;
}

unsigned mouseb_raw_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:raw: mouseb_raw_button_get()\n"));

	return (raw_state.map[mouse].button_map[button].code & *raw_state.map[mouse].button_map[button].pvalue) != 0;
}

void mouseb_raw_poll(void)
{
	unsigned i;

	log_debug(("mouseb:raw: mouseb_raw_poll()\n"));

	for(i=0;i<RAW_MOUSE_MAX;++i) {
		if (raw_state.map[i].active_flag) {
			raw_mouse_poll(&raw_state.map[i].context, 0);
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
		raw_state.map[i].context.type = conf_int_get_default(context, buf);

		snprintf(buf, sizeof(buf), "device_raw_mousedev[%d]", i);
		s = conf_string_get_default(context, buf);
		if (strcmp(s, "auto") == 0) {
			if (i == 0 && access("/dev/mouse", F_OK) == 0) {
				sncpy(raw_state.map[i].context.dev, sizeof(raw_state.map[i].context.dev), "/dev/mouse");
				if (raw_state.map[i].context.type < 0)
					raw_state.map[i].context.type = MOUSE_PNP;
			} else {
				snprintf(raw_state.map[i].context.dev, sizeof(raw_state.map[i].context.dev), "/dev/input/mouse%d", i);
				switch (raw_state.map[i].context.type) {
				case MOUSE_PS2 :
				case MOUSE_IMPS2 :
				case MOUSE_EXPPS2 :
					/* the /dev/input/mouse interfaces is compatible only with these three protocols */
					break;
				default:
					raw_state.map[i].context.type = MOUSE_IMPS2;
					break;
				}
			}
		} else {
			sncpy(raw_state.map[i].context.dev, sizeof(raw_state.map[i].context.dev), s);
			if (raw_state.map[i].context.type < 0)
				raw_state.map[i].context.type = MOUSE_PNP;
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
	0,
	0,
	mouseb_raw_flags,
	mouseb_raw_count_get,
	mouseb_raw_axe_count_get,
	mouseb_raw_axe_name_get,
	mouseb_raw_button_count_get,
	mouseb_raw_button_name_get,
	mouseb_raw_axe_get,
	mouseb_raw_button_get,
	mouseb_raw_poll
};

