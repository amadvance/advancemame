/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2005, 2008 Andrea Mazzoleni
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

/** \file
 * Internal interface for the "windows" host.
 */

#ifndef __OSWIN_H
#define __OSWIN_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/* Internal */

void target_usleep_granularity(unsigned us);

/* Check if SVGAWIN is used in some way */
#if defined(USE_VIDEO_SVGAWIN)
#define USE_SVGAWIN
void* os_internal_svgawin_get(void);
int os_internal_svgawin_is_video_active(void);
int os_internal_svgawin_is_video_mode_active(void);
#endif

/* Check if SDL is used in some way */
#if defined(USE_VIDEO_SDL) || defined(USE_KEYBOARD_SDL) || defined(USE_MOUSE_SDL) || defined(USE_JOYSTICK_SDL) || defined(USE_SOUND_SDL) || defined(USE_INPUT_SDL)
#define USE_SDL
#include "ossdl.h"
#endif

#if defined(USE_MOUSE_RAWINPUT)
void mouseb_rawinput_event_msg(unsigned msg, unsigned wparam, unsigned lparam);
#endif

#if defined(USE_JOYSTICK_LGRAWINPUT)
void joystickb_lgrawinput_event_msg(unsigned msg, unsigned wparam, unsigned lparam);
#endif

void* os_internal_window_get(void);

/* Disable hot keys */
void os_internal_ignore_hot_key(void);
void os_internal_restore_hot_key(void);

/* Splash */
void os_internal_splash_stop(void);

/***************************************************************************/
/* Extension */

#include <windows.h>

/* Raw Input definitions */
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

/* HID definitions */
typedef struct _HIDD_ATTRIBUTES {
	ULONG Size;
	USHORT VendorID;
	USHORT ProductID;
	USHORT VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

int GetRawInputDeviceHIDInfo(const char* name, unsigned* vid, unsigned* pid, unsigned* rev);

#ifdef __cplusplus
}
#endif

#endif

