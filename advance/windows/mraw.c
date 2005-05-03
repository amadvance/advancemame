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
 * Implementation of a mouse driver using the Windows XP "Raw Input" API. 
 * More information in the MSDN documentation (GetRawInputDeviceList) and at the site:
 * http://link.mywwwserver.com/~jstookey/arcade/rawmouse/
 */

#include "portable.h"

#include "mraw.h"
#include "log.h"
#include "oswin.h"
#include "error.h"
#include "snstring.h"

#include <windows.h>

/* Extra Windows definitions */
#if !defined(WM_INPUT)
#define WM_INPUT 0x00FF
#define RIM_INPUT 0x00000000
#define RIM_INPUTSINK 0x00000001
#define RIM_TYPEMOUSE 0x00000000
#define RIM_TYPEKEYBOARD 0x00000001
#define RIM_TYPEHID 0x00000002
#define MOUSE_MOVE_RELATIVE 0x00000000
#define MOUSE_MOVE_ABSOLUTE 0x00000001
#define MOUSE_VIRTUAL_DESKTOP 0x00000002
#define MOUSE_ATTRIBUTES_CHANGED 0x00000004
#define RI_MOUSE_LEFT_BUTTON_DOWN 0x0001
#define RI_MOUSE_LEFT_BUTTON_UP 0x0002
#define RI_MOUSE_RIGHT_BUTTON_DOWN 0x0004
#define RI_MOUSE_RIGHT_BUTTON_UP 0x0008
#define RI_MOUSE_MIDDLE_BUTTON_DOWN 0x0010
#define RI_MOUSE_MIDDLE_BUTTON_UP 0x0020
#define RI_MOUSE_BUTTON_1_DOWN RI_MOUSE_LEFT_BUTTON_DOWN
#define RI_MOUSE_BUTTON_1_UP RI_MOUSE_LEFT_BUTTON_UP
#define RI_MOUSE_BUTTON_2_DOWN RI_MOUSE_RIGHT_BUTTON_DOWN
#define RI_MOUSE_BUTTON_2_UP RI_MOUSE_RIGHT_BUTTON_UP
#define RI_MOUSE_BUTTON_3_DOWN RI_MOUSE_MIDDLE_BUTTON_DOWN
#define RI_MOUSE_BUTTON_3_UP RI_MOUSE_MIDDLE_BUTTON_UP
#define RI_MOUSE_BUTTON_4_DOWN 0x0040
#define RI_MOUSE_BUTTON_4_UP 0x0080
#define RI_MOUSE_BUTTON_5_DOWN 0x0100
#define RI_MOUSE_BUTTON_5_UP 0x0200
#define RI_MOUSE_WHEEL 0x0400
#define KEYBOARD_OVERRUN_MAKE_CODE 0x00ff
#define RI_KEY_MAKE 0x0000
#define RI_KEY_BREAK 0x0001
#define RI_KEY_E0 0x0002
#define RI_KEY_E1 0x0004
#define RI_KEY_TERMSRV_SET_LED 0x0008
#define RI_KEY_TERMSRV_SHADOW 0x0010
#define RID_INPUT 0x10000003
#define RID_HEADER 0x10000005
#define RIDI_PREPARSEDDATA 0x20000005
#define RIDI_DEVICENAME 0x20000007
#define RIDI_DEVICEINFO 0x2000000b
#define RIDEV_REMOVE 0x00000001
#define RIDEV_EXCLUDE 0x00000010
#define RIDEV_PAGEONLY 0x00000020
#define RIDEV_NOLEGACY 0x00000030
#define RIDEV_INPUTSINK 0x00000100
#define RIDEV_CAPTUREMOUSE 0x00000200
#define RIDEV_NOHOTKEYS 0x00000200
#define RIDEV_APPKEYS 0x00000400

DECLARE_HANDLE(HRAWINPUT);
typedef struct tagRAWINPUTHEADER {
	DWORD dwType;
	DWORD dwSize;
	HANDLE hDevice;
	WPARAM wParam;
} RAWINPUTHEADER,*PRAWINPUTHEADER;
typedef struct tagRAWMOUSE {
	USHORT usFlags;
	_ANONYMOUS_UNION union {
		ULONG ulButtons;
		_ANONYMOUS_STRUCT struct {
			USHORT usButtonFlags;
			USHORT usButtonData;
		};
	};
	ULONG ulRawButtons;
	LONG lLastX;
	LONG lLastY;
	ULONG ulExtraInformation;
} RAWMOUSE,*PRAWMOUSE,*LPRAWMOUSE;
typedef struct tagRAWKEYBOARD {
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	USHORT VKey;
	UINT Message;
	ULONG ExtraInformation;
} RAWKEYBOARD,*PRAWKEYBOARD,*LPRAWKEYBOARD;
typedef struct tagRAWHID {
	DWORD dwSizeHid;
	DWORD dwCount;
	BYTE bRawData;
} RAWHID,*PRAWHID,*LPRAWHID;
typedef struct tagRAWINPUT {
	RAWINPUTHEADER header;
	union {
		RAWMOUSE mouse;
		RAWKEYBOARD keyboard;
		RAWHID hid;
	} data;
} RAWINPUT,*PRAWINPUT,*LPRAWINPUT;
typedef struct tagRAWINPUTDEVICE {
	USHORT usUsagePage;
	USHORT usUsage;
	DWORD dwFlags;
	HWND hwndTarget;
} RAWINPUTDEVICE,*PRAWINPUTDEVICE,*LPRAWINPUTDEVICE;
typedef const RAWINPUTDEVICE *PCRAWINPUTDEVICE;
typedef struct tagRAWINPUTDEVICELIST {
	HANDLE hDevice;
	DWORD dwType;
} RAWINPUTDEVICELIST,*PRAWINPUTDEVICELIST;

typedef struct tagRID_DEVICE_INFO_MOUSE {
	DWORD dwId;
	DWORD dwNumberOfButtons;
	DWORD dwSampleRate;
} RID_DEVICE_INFO_MOUSE, *PRID_DEVICE_INFO_MOUSE;
typedef struct tagRID_DEVICE_INFO_KEYBOARD {
	DWORD dwType;
	DWORD dwSubType;
	DWORD dwKeyboardMode;
	DWORD dwNumberOfFunctionKeys;
	DWORD dwNumberOfIndicators;
	DWORD dwNumberOfKeysTotal;
} RID_DEVICE_INFO_KEYBOARD, *PRID_DEVICE_INFO_KEYBOARD;
typedef struct tagRID_DEVICE_INFO_HID {
	DWORD dwVendorId;
	DWORD dwProductId;
	DWORD dwVersionNumber;
	USHORT usUsagePage;
	USHORT usUsage;
} RID_DEVICE_INFO_HID, *PRID_DEVICE_INFO_HID;
typedef struct tagRID_DEVICE_INFO {
	DWORD cbSize;
	DWORD dwType;
	union {
		RID_DEVICE_INFO_MOUSE mouse;
		RID_DEVICE_INFO_KEYBOARD keyboard;
		RID_DEVICE_INFO_HID hid;
	};
} RID_DEVICE_INFO, *PRID_DEVICE_INFO, *LPRID_DEVICE_INFO;
#endif

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
	HANDLE h;
	char name[256];
	RID_DEVICE_INFO info;
	int x, y, z;
	int lx, ly;
	unsigned button;
};

struct mouse_item_context {
	struct raw_context context;
	unsigned button_mac;
	struct mouse_button_context button_map[RAW_MOUSE_BUTTON_MAX];
	unsigned axe_mac;
	struct mouse_axe_context axe_map[RAW_MOUSE_AXE_MAX];
};

struct mouseb_rawinput_context {
	unsigned mac;
	struct mouse_item_context map[RAW_MOUSE_MAX];
	WNDPROC proc;
	HWND window;
};

static struct mouseb_rawinput_context raw_state;

typedef INT (WINAPI * GetRawInputDeviceList_type)(RAWINPUTDEVICELIST* pRawInputDeviceList, PINT puiNumDevices, UINT cbSize);
typedef INT (WINAPI * GetRawInputData_type)(HRAWINPUT hRawInput, UINT uiCommand, VOID* pData, INT* pcbSize, UINT cbSizeHeader);
typedef INT (WINAPI * GetRawInputDeviceInfoA_type)(HANDLE hDevice, UINT uiCommand, VOID* pData, INT* pcbSize);
typedef BOOL (WINAPI * RegisterRawInputDevices_type)(const RAWINPUTDEVICE* pRawInputDevices, UINT uiNumDevices, UINT cbSize);

GetRawInputDeviceList_type GetRawInputDeviceList_ptr;
GetRawInputData_type GetRawInputData_ptr;
GetRawInputDeviceInfoA_type GetRawInputDeviceInfoA_ptr;
RegisterRawInputDevices_type RegisterRawInputDevices_ptr;

static adv_device DEVICE[] = {
{ "auto", -1, "RAW mouse" },
{ 0, 0, 0 }
};

static void mouseb_setup(struct mouse_item_context* item, unsigned nbutton)
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
		{ 1, "y" },
		{ 2, "z" }
	};

	item->button_mac = 0;
	for(i=0;i<sizeof(button_map)/sizeof(button_map[0]);++i) {
		if (i < nbutton) {
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
		if (item->axe_mac < RAW_MOUSE_AXE_MAX) {
			item->axe_map[item->axe_mac].code = axe_map[i].code;
			switch (axe_map[i].code) {
			case 0 : item->axe_map[item->axe_mac].pvalue = &item->context.x; break;
			case 1 : item->axe_map[item->axe_mac].pvalue = &item->context.y; break;
			case 2 : item->axe_map[item->axe_mac].pvalue = &item->context.z; break;
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

	\??\Root#RDP_KBD#0000#{884b96c3-56ef-11d1-bc8c-00a0c91405dd} -- Global keyboard
	\??\Root#*PNP030b#1_0_22_0_32_0#{884b96c3-56ef-11d1-bc8c-00a0c91405dd} - Keyboard
	\??\Root#RDP_MOU#0000#{378de44c-56ef-11d1-bc8c-00a0c91405dd} - Global mouse
	\??\Root#*PNP0F03#1_0_21_0_31_0#{378de44c-56ef-11d1-bc8c-00a0c91405dd} - PS2 mouse
	\??\HID#Vid_046d&Pid_c001#5&194dac4e&0&0000#{378de44c-56ef-11d1-bc8c-00a0c91405dd} - HID USB mouse
  
	*/

	/* reverse order to put HID device after the not HID */
	return stricmp(b->context.name, a->context.name);
}

adv_error mouseb_rawinput_init(int mouseb_id)
{
	unsigned i;
	HMODULE h;
	UINT n;
	UINT size;
	RAWINPUTDEVICELIST* l;

	log_std(("mouseb:rawinput: mouseb_rawinput_init(id:%d)\n", mouseb_id));

	h = GetModuleHandle("user32.dll");
	if (!h) {
		error_set("Error loading the user32 library.\n");
		return -1;
	}

	RegisterRawInputDevices_ptr = (RegisterRawInputDevices_type)GetProcAddress(h, "RegisterRawInputDevices");
	GetRawInputDeviceList_ptr = (GetRawInputDeviceList_type)GetProcAddress(h, "GetRawInputDeviceList");
	GetRawInputDeviceInfoA_ptr = (GetRawInputDeviceInfoA_type)GetProcAddress(h, "GetRawInputDeviceInfoA");
	GetRawInputData_ptr = (GetRawInputData_type)GetProcAddress(h, "GetRawInputData");

	if (!RegisterRawInputDevices_ptr || !GetRawInputDeviceList_ptr || !GetRawInputDeviceInfoA_ptr || !GetRawInputData_ptr) {
		error_set("Raw input devices not supported on your system.\n");
		return -1;
	}

	if (GetRawInputDeviceList_ptr(NULL, &n, sizeof(RAWINPUTDEVICELIST)) != 0) {
		error_set("Error getting the number of raw devices.\n");
		return -1;
	}

	if (n == 0) {
		error_set("No input device found.\n");
		return -1;
	}

	size = n * sizeof(RAWINPUTDEVICELIST);
	l = malloc(size);

	n = GetRawInputDeviceList_ptr(l, &size, sizeof(RAWINPUTDEVICELIST));
	if (n == -1) {
		free(l);
		error_set("Error getting the list of raw devices.\n");
		return -1;
	}

	log_std(("mouseb:rawinput: GetRawInputDeviceList() -> %d\n", (unsigned)n));

	raw_state.mac = 0;
	for(i=0;i<n;++i) {
		if (raw_state.mac < RAW_MOUSE_MAX) {
			UINT size;
			struct raw_context* context = &raw_state.map[raw_state.mac].context;

			size = sizeof(RID_DEVICE_INFO);
			context->info.cbSize = sizeof(RID_DEVICE_INFO);
			if (GetRawInputDeviceInfoA_ptr(l[i].hDevice, RIDI_DEVICEINFO, &context->info, &size) == -1) {
				continue;
			}

			size = sizeof(context->name);
			if (GetRawInputDeviceInfoA_ptr(l[i].hDevice, RIDI_DEVICENAME, context->name, &size) < 0) {
				continue;
			}

			log_std(("mouseb:rawinput: GetRawInputDeviceInfo(%d) -> type:%d, %s\n", i, (unsigned)context->info.dwType, context->name));

			/* HACK skip the global mouse which is the combination of all the others */
			if (strstr(context->name, "#RDP_MOU#") != 0) {
				continue;
			}

			if (context->info.dwType != RIM_TYPEMOUSE) {
				continue;
			}

			raw_state.map[raw_state.mac].context.h = l[i].hDevice;
			raw_state.map[raw_state.mac].context.x = 0;
			raw_state.map[raw_state.mac].context.y = 0;
			raw_state.map[raw_state.mac].context.lx = 0;
			raw_state.map[raw_state.mac].context.ly = 0;
			raw_state.map[raw_state.mac].context.z = 0;
			raw_state.map[raw_state.mac].context.button = 0;

			log_std(("mouseb:rawinput: mouse id:%d,buttons:%d,samplerate:%d\n", (unsigned)context->info.mouse.dwId, (unsigned)context->info.mouse.dwNumberOfButtons, (unsigned)context->info.mouse.dwSampleRate));

			mouseb_setup(&raw_state.map[raw_state.mac], context->info.mouse.dwNumberOfButtons);

			++raw_state.mac;
		}
	}

	free(l);

	if (raw_state.mac == 0) {
		error_set("No mouse found.\n");
		return -1;
	}

	qsort(raw_state.map, raw_state.mac, sizeof(raw_state.map[0]), mouseb_compare);

	return 0;
}

void mouseb_rawinput_done(void)
{
	log_std(("mouseb:rawinput: mouseb_rawinput_done()\n"));

	raw_state.mac = 0;
}

static LRESULT __stdcall mouseb_rawinput_proc(HWND h, UINT msg, WPARAM w, LPARAM l)
{
	/* catch all the messages */
	mouseb_rawinput_event_msg(msg, w, l);

	return raw_state.proc(h, msg, w, l);
}

adv_error mouseb_rawinput_enable(void)
{
	RAWINPUTDEVICE d[1];

	log_std(("mouseb:rawinput: mouseb_rawinput_enable()\n"));

	raw_state.window = os_internal_window_get();
	if (!raw_state.window) {
	    log_std(("ERROR:mouseb:rawinput: os_internal_window_get() failed with error %d\n", (unsigned)GetLastError()));
		error_set("Error getting the window handle.\n");
		return -1;
	}

	/* grab the window proc */
	raw_state.proc = (WNDPROC)SetWindowLong(raw_state.window, GWL_WNDPROC, (LONG)mouseb_rawinput_proc);
	if (!raw_state.proc) {
	    log_std(("ERROR:mouseb:rawinput: SetWindowLong(0x%x, GWL_WNDPROC) failed with error %d\n", (unsigned)raw_state.window, (unsigned)GetLastError()));
		error_set("Error setting the window proc.\n");
		return -1;
	}

	d[0].usUsagePage = 0x01;
	d[0].usUsage = 0x02;
	d[0].dwFlags = RIDEV_INPUTSINK; /* capture events also in background */
	d[0].hwndTarget = raw_state.window;

	if (!RegisterRawInputDevices_ptr(d, 1, sizeof(d[0]))) {
	    log_std(("ERROR:mouseb:rawinput: RegisterRawInputDevices() failed with error %d\n", (unsigned)GetLastError()));
		error_set("Error registering input.\n");
		return -1;
	}

	return 0;
}

void mouseb_rawinput_disable(void)
{
	RAWINPUTDEVICE d[1];

	log_std(("mouseb:rawinput: mouseb_rawinput_disable()\n"));

	d[0].usUsagePage = 0x01;
	d[0].usUsage = 0x02; 
	d[0].dwFlags = RIDEV_REMOVE;
	d[0].hwndTarget = raw_state.window;

	if (!RegisterRawInputDevices_ptr(d, 1, sizeof(d[0]))) {
		log_std(("WARNING:mouseb:rawinput: RegisterRawInputDevices(RIDEV_REMOVE) failed with error %d\n", (unsigned)GetLastError()));
	}

	/* restore the window proc */
	if (SetWindowLong(raw_state.window, GWL_WNDPROC, (LONG)raw_state.proc) != (LONG)mouseb_rawinput_proc) {
		log_std(("WARNING:mouseb:rawinput: SetWindowLong(oldproc) failed\n"));
	}
}

unsigned mouseb_rawinput_count_get(void)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_count_get()\n"));

	return raw_state.mac;
}

unsigned mouseb_rawinput_axe_count_get(unsigned mouse)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_axe_count_get()\n"));

	return raw_state.map[mouse].axe_mac;
}

const char* mouseb_rawinput_axe_name_get(unsigned mouse, unsigned axe)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_axe_name_get()\n"));

	return raw_state.map[mouse].axe_map[axe].name;
}

unsigned mouseb_rawinput_button_count_get(unsigned mouse)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_button_count_get()\n"));

	return raw_state.map[mouse].button_mac;
}

const char* mouseb_rawinput_button_name_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_button_name_get()\n"));

	return raw_state.map[mouse].button_map[button].name;
}

int mouseb_rawinput_axe_get(unsigned mouse, unsigned axe)
{
	int r;

	log_debug(("mouseb:rawinput: mouseb_rawinput_pos_get()\n"));

	r = *raw_state.map[mouse].axe_map[axe].pvalue;
	*raw_state.map[mouse].axe_map[axe].pvalue -= r; /* thread safe */

	return r;
}

unsigned mouseb_rawinput_button_get(unsigned mouse, unsigned button)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_button_get()\n"));

	return (raw_state.map[mouse].button_map[button].code & *raw_state.map[mouse].button_map[button].pvalue) != 0;
}

static void raw_event(RAWINPUT* r)
{
	unsigned i;
	RAWMOUSE* m;
	struct raw_context* c;

	if (r->header.dwType != RIM_TYPEMOUSE) {
		log_std(("WARNING:mouseb:rawinput: not a mouse device\n"));
		return;
	}

	/* search the device */
	for(i=0;i<raw_state.mac;++i)
		if (r->header.hDevice == raw_state.map[i].context.h)
			break;

	if (i == raw_state.mac) {
		log_std(("WARNING:mouseb:rawinput: input device not found\n"));
		return;
	}

	m = &r->data.mouse;
	c = &raw_state.map[i].context;

	log_debug(("mouseb:rawinput: device:%d -> usFlags:%d,usButtonFlags:%d,ulRawButtons:%d,lLastX:%d,lLastY:%d,ulExtraInformation:%d\n", i, (unsigned)m->usFlags, (unsigned)m->usButtonFlags, (unsigned)m->ulRawButtons, (unsigned)m->lLastX, (unsigned)m->lLastY, (unsigned)m->ulExtraInformation));

	if (m->usFlags & MOUSE_MOVE_ABSOLUTE) {
		/* absolute */
		c->x += m->lLastX - c->lx;
		c->y += m->lLastY - c->ly;
		c->lx = m->lLastX;
		c->ly = m->lLastY;
	} else {
		/* relative */
		c->x += m->lLastX;
		c->y += m->lLastY;
	}

	if (m->usButtonFlags & RI_MOUSE_WHEEL) {
		c->z += (SHORT)m->usButtonData;
	}

	if (m->usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) 
		c->button |= 0x1;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_1_UP) 
		c->button &= ~0x1;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) 
		c->button |= 0x2;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_2_UP) 
		c->button &= ~0x2;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) 
		c->button |= 0x4;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_3_UP) 
		c->button &= ~0x4;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) 
		c->button |= 0x8;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_4_UP) 
		c->button &= ~0x8;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) 
		c->button |= 0x10;
	if (m->usButtonFlags & RI_MOUSE_BUTTON_5_UP) 
		c->button &= ~0x10;

	log_debug(("mouseb:rawinput: id:%d,x:%d,y:%d,z:%d,button:%d\n", i, c->x, c->y, c->z, c->button));
}

void mouseb_rawinput_event_msg(unsigned msg, unsigned wparam, unsigned lparam)
{
	RAWINPUT* r;
	HRAWINPUT h;
	INT size;

	/* process a Windows WM_INPUT message */
	/* Note that the message MUST be processed before passing it to the DefWindowProc */
	/* otherwise the input HANDLE will be invalidated. */
	/* This prevent the use of the SDL event queuee. */

	log_debug(("mouseb:rawinput: mouseb_rawinput_event_input(msg:%d,wparam:%x,lparam:%x)\n", msg, wparam, lparam));

	if (msg != WM_INPUT) {
		log_debug(("WARNING:mouseb:rawinput: not an input message\n"));
		return;
	}

	h = (HRAWINPUT)lparam;

	size = 0;
	if (GetRawInputData_ptr(h, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER)) == -1) {
		log_std(("ERROR:mouseb:rawinput: GetRawInputData(NULL) failed with error %d\n", (unsigned)GetLastError()));
		return;
	}

	r = malloc(size);

	if (GetRawInputData_ptr(h, RID_INPUT, r, &size, sizeof(RAWINPUTHEADER)) == -1) {
		log_std(("ERROR:mouseb:rawinput: GetRawInputData(RAWINPUT) failed with error %d\n", (unsigned)GetLastError()));
		return;
	}

	raw_event(r);

	free(r);
}

void mouseb_rawinput_poll(void)
{
	log_debug(("mouseb:rawinput: mouseb_rawinput_poll()\n"));
}

unsigned mouseb_rawinput_flags(void)
{
	return 0;
}

adv_error mouseb_rawinput_load(adv_conf* context)
{
	return 0;
}

void mouseb_rawinput_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

mouseb_driver mouseb_rawinput_driver = {
	"rawinput",
	DEVICE,
	mouseb_rawinput_load,
	mouseb_rawinput_reg,
	mouseb_rawinput_init,
	mouseb_rawinput_done,
	mouseb_rawinput_enable,
	mouseb_rawinput_disable,
	mouseb_rawinput_flags,
	mouseb_rawinput_count_get,
	mouseb_rawinput_axe_count_get,
	mouseb_rawinput_axe_name_get,
	mouseb_rawinput_button_count_get,
	mouseb_rawinput_button_name_get,
	mouseb_rawinput_axe_get,
	mouseb_rawinput_button_get,
	mouseb_rawinput_poll
};

