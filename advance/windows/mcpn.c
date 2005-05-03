/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2005 Andrea Mazzoleni
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

/** 
 * Implementation of a mouse driver using the custom CPN driver.
 * More information at: http://cpnmouse.sourceforge.net/
 */

#include "portable.h"

#include "mcpn.h"
#include "log.h"
#include "oswin.h"
#include "error.h"
#include "snstring.h"

/* include the CPN support */
#include "lapi.c"

#define RAW_MOUSE_MAX 8
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

struct raw_context {
	unsigned number;
	char name[256];
	int x, y;
	unsigned button;
};

struct mouse_item_context {
	struct raw_context context;
	unsigned button_mac;
	struct mouse_button_context button_map[RAW_MOUSE_BUTTON_MAX];
	unsigned axe_mac;
	struct mouse_axe_context axe_map[RAW_MOUSE_AXE_MAX];
};

struct mouseb_cpn_context {
	HANDLE thread;
	DWORD threadid;
	HANDLE forward_event;
	HANDLE back_event;
	unsigned mac;
	struct mouse_item_context map[RAW_MOUSE_MAX];
};

static struct mouseb_cpn_context raw_state;

static adv_device DEVICE[] = {
{ "auto", -1, "CPN mouse" },
{ 0, 0, 0 }
};

static void __cdecl mouseb_callback(int number, signed int dx, signed int dy, unsigned int buttons, int suspended)
{
	unsigned i;
	struct raw_context* c;
	POINT p;
	unsigned flags;

	log_debug(("mouseb:cpn: callback number:%d,dx,%d,dy:%d,buttons:0x%x,suspend:%d\n", number, dx, dy, buttons, suspended));

	/* search the device */
	for(i=0;i<raw_state.mac;++i)
		if (number == raw_state.map[i].context.number)
			break;

	if (i == raw_state.mac) {
		log_std(("WARNING:mouseb:cpn: input device not found\n"));
		return;
	}

	c = &raw_state.map[i].context;

	c->x += dx;
	c->y += dy;

	if (buttons & 0x1)
		c->button |= 0x1;
	if (buttons & 0x2)
		c->button &= ~0x1;
	if (buttons & 0x4)
		c->button |= 0x2;
	if (buttons & 0x8)
		c->button &= ~0x2;
	if (buttons & 0x10)
		c->button |= 0x4;
	if (buttons & 0x20)
		c->button &= ~0x4;
	if (buttons & 0x40)
		c->button |= 0x8;
	if (buttons & 0x80)
		c->button &= ~0x8;
}

static void mouseb_setup(struct mouse_item_context* item)
{
	unsigned i;
	struct button_entry {
		int code;
		const char* name;
	} button_map[] = {
		{ 0x1, "left" },
		{ 0x2, "right" },
		{ 0x4, "middle" },
		{ 0x8, "button4" },
		{ 0x10, "button5" }
	};

	struct axe_entry {
		int code;
		const char* name;
	} axe_map[] = {
		{ 0, "x" },
		{ 1, "y" }
	};

	item->button_mac = 0;
	for(i=0;i<sizeof(button_map)/sizeof(button_map[0]);++i) {
		if (item->button_mac < RAW_MOUSE_BUTTON_MAX) {
			item->button_map[item->button_mac].code = button_map[i].code;
			item->button_map[item->button_mac].pvalue = &item->context.button;
			sncpy(item->button_map[item->button_mac].name, sizeof(item->button_map[item->button_mac].name), button_map[i].name);
			++item->button_mac;
		}
	}

	item->axe_mac = 0;
	for(i=0;i<sizeof(axe_map)/sizeof(axe_map[0]);++i) {
		if (item->axe_mac < RAW_MOUSE_AXE_MAX) {
			item->axe_map[item->axe_mac].code = axe_map[i].code;
			switch (axe_map[i].code) {
			case 0 : item->axe_map[item->axe_mac].pvalue = &item->context.x; break;
			case 1 : item->axe_map[item->axe_mac].pvalue = &item->context.y; break;
			}
			sncpy(item->axe_map[item->axe_mac].name, sizeof(item->axe_map[item->axe_mac].name), axe_map[i].name);
			++item->axe_mac;
		}
	}
}

static int mouseb_compare(const void* void_a, const void* void_b)
{
	const struct mouse_item_context* a = (const struct mouse_item_context*)void_a;
	const struct mouse_item_context* b = (const struct mouse_item_context*)void_b;

	/* Typical names are:

	\\?\acpi#pnp0f13#4&17a17c97&0#{db4bbc1e-cac8-47e9-8414-7365167feca3} -- PS2 mouse
	\\?\hid#vid_046d&pid_c001#6&2fdeba9e&0&0000#{db4bbc1e-cac8-47e9-8414-7365167feca3} -- HID USB mouse

	*/

	/* reverse order to put HID device after the not HID */
	return stricmp(a->context.name, b->context.name);
}

static DWORD WINAPI mouseb_thread(void* arg)
{
	unsigned n;
	unsigned i;
	adv_bool done;

	/* allocate */
	lRegisterCallback(mouseb_callback);
	
	log_std(("mouseb:cpn: lRegisterCallback() completed\n"));

	n = lGetMice(RAW_MOUSE_MAX);

	log_std(("mouseb:cpn: lGetMice(%d) -> %d\n", RAW_MOUSE_MAX, n));

	/* configure */
	for(i=0;i<n;++i) {
		if (raw_state.mac < RAW_MOUSE_MAX) {
			UINT size;
			const char* name;
			struct raw_context* context = &raw_state.map[raw_state.mac].context;

			context->number = i + 1;

			if (!lHasMouse(context->number)) {
			    log_std(("WARNING:mouseb:cpn: lHasMouse(%d) failed\n", context->number));
				continue;
			}

			name = lGetDevicePath(context->number);
			if (!name || !name[0]) {
			    log_std(("WARNING:mouseb:cpn: lGetDevicePath(%d) failed\n", context->number));
				continue;
			}

            /* suspend the mouse to allow Windows to get events */
            lSuspendMouse(context->number);

			memset(context->name, 0, sizeof(context->name));
			strncpy(context->name, name, sizeof(context->name) - 1);

			log_std(("mouseb:cpn: lGetDevicePath(%d) -> %s\n", context->number, context->name));

			raw_state.map[raw_state.mac].context.x = 0;
			raw_state.map[raw_state.mac].context.y = 0;
			raw_state.map[raw_state.mac].context.button = 0;

			log_std(("mouseb:cpn: mouse id:%d\n", context->number));

			mouseb_setup(&raw_state.map[raw_state.mac]);

			++raw_state.mac;
		}
	}

	qsort(raw_state.map, raw_state.mac, sizeof(raw_state.map[0]), mouseb_compare);	

	/* signal that the thread is ready */
	SetEvent(raw_state.back_event);

	/* user mode APCs are only delivered to a thread in an alertable state. */
	/* This is the reason of thread and of the Wait call.*/
	/* It's the standard behaviour of the undocumented KeInitializeApc() */
	/* and KeInsertQueueApc() functions used by the CPN driver */

	done = 0;
	while (!done) {
		DWORD r;

		r = WaitForSingleObjectEx(raw_state.forward_event, INFINITE, TRUE);
		switch (r) {
		case WAIT_OBJECT_0 :
			/* event signaled */
			done = 1;
			break;
		case WAIT_IO_COMPLETION :
			/* APC callback */
			break;
		default :
			log_std(("WARNING:mouseb:cpn: thread WaitForSingleObjectEx() -> %d\n", (unsigned)r));
			break;
		}
	}

	/* deallocate */
	lUnGetAllMice();
	
	log_std(("mouseb:cpn: lUnGetAllMice() completed\n"));

	lUnRegisterCallback();
	
	log_std(("mouseb:cpn: lUnRegisterCallback() completed\n"));

	return 0;
}
 
static adv_error mouseb_thread_start(void)
{
	raw_state.forward_event = CreateEvent(0, FALSE, FALSE, 0);
	if (!raw_state.forward_event) {
		log_std(("ERROR:mouseb:cpn: CreateEvent() failed with error %d\n", (unsigned)GetLastError()));
		return -1;
	}

	raw_state.back_event = CreateEvent(0, FALSE, FALSE, 0);
	if (!raw_state.back_event) {
		log_std(("ERROR:mouseb:cpn: CreateEvent() failed with error %d\n", (unsigned)GetLastError()));
		return -1;
	}

	log_std(("mouseb:cpn: thread start\n"));
	raw_state.thread = CreateThread(0, 0, mouseb_thread, 0, 0, &raw_state.threadid);
	if (!raw_state.thread) {
		log_std(("ERROR:mouseb:cpn: CreateThread() failed with error %d\n", (unsigned)GetLastError()));
		return -1;
	}
 
	/* increase thread priority */
	if (!SetThreadPriority(raw_state.thread, THREAD_PRIORITY_TIME_CRITICAL)) {
		log_std(("WARNING:mouseb:cpn: SetThreadPriority(THREAD_PRIORITY_TIME_CRITICAL) failed with error %d\n", (unsigned)GetLastError()));
	}
 
	/* wait for completion */
	while (WaitForSingleObjectEx(raw_state.back_event, INFINITE, TRUE) != WAIT_OBJECT_0) {
	}
	log_std(("mouseb:cpn: thread started\n"));

	return 0;
}
 
static void mouseb_thread_end(void)
{
	/* abort the thread */
	log_std(("mouseb:cpn: thread stop\n"));

	/* signal the thread end */
	SetEvent(raw_state.forward_event);

	/* wait for completion */
	while (WaitForSingleObjectEx(raw_state.thread, INFINITE, TRUE) != WAIT_OBJECT_0) {
	}
	log_std(("mouseb:cpn: thread stopped\n"));

	CloseHandle(raw_state.thread);
	CloseHandle(raw_state.forward_event);
	CloseHandle(raw_state.back_event);
} 
 
adv_error mouseb_cpn_init(int mouseb_id)
{
	log_std(("mouseb:cpn: mouseb_cpn_init(id:%d)\n", mouseb_id));

	raw_state.mac = 0;

	if (mouseb_thread_start() != 0) {
		error_set("Error initializing the system.\n");

		return -1;
	}

	if (raw_state.mac == 0) {
		error_set("No mouse found, or CPN driver not installed\n");

		mouseb_thread_end();

		return -1;
	}

	return 0;
}

void mouseb_cpn_done(void)
{
	log_std(("mouseb:cpn: mouseb_cpn_done()\n"));

	mouseb_thread_end();
}

unsigned mouseb_cpn_count_get(void)
{
	log_debug(("mouseb:cpn: mouseb_cpn_count_get()\n"));

	return raw_state.mac;
}

unsigned mouseb_cpn_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:cpn: mouseb_cpn_axe_count_get()\n"));

	return raw_state.map[mouse].axe_mac;
}

const char* mouseb_cpn_axe_name_get(unsigned mouse, unsigned axe)
{
	log_debug(("mouseb:cpn: mouseb_cpn_axe_name_get()\n"));

	return raw_state.map[mouse].axe_map[axe].name;
}

unsigned mouseb_cpn_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:cpn: mouseb_cpn_button_count_get()\n"));

	return raw_state.map[mouse].button_mac;
}

const char* mouseb_cpn_button_name_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:cpn: mouseb_cpn_button_name_get()\n"));

	return raw_state.map[mouse].button_map[button].name;
}

int mouseb_cpn_axe_get(unsigned mouse, unsigned axe)
{
	int r;

	log_debug(("mouseb:cpn: mouseb_cpn_pos_get()\n"));

	r = *raw_state.map[mouse].axe_map[axe].pvalue;
	*raw_state.map[mouse].axe_map[axe].pvalue -= r; /* thread safe */

	return r;
}

unsigned mouseb_cpn_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:cpn: mouseb_cpn_button_get()\n"));

	return (raw_state.map[mouse].button_map[button].code & *raw_state.map[mouse].button_map[button].pvalue) != 0;
}

void mouseb_cpn_poll(void)
{
	log_debug(("mouseb:cpn: mouseb_cpn_poll()\n"));
}

unsigned mouseb_cpn_flags(void)
{
	return 0;
}

adv_error mouseb_cpn_load(adv_conf* context)
{
	return 0;
}

void mouseb_cpn_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_cpn_driver = {
	"cpn",
	DEVICE,
	mouseb_cpn_load,
	mouseb_cpn_reg,
	mouseb_cpn_init,
	mouseb_cpn_done,
	0,
	0,
	mouseb_cpn_flags,
	mouseb_cpn_count_get,
	mouseb_cpn_axe_count_get,
	mouseb_cpn_axe_name_get,
	mouseb_cpn_button_count_get,
	mouseb_cpn_button_name_get,
	mouseb_cpn_axe_get,
	mouseb_cpn_button_get,
	mouseb_cpn_poll
};

