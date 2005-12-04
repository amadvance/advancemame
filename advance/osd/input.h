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

#ifndef __INPUT_H
#define __INPUT_H

/* BitMask Legend */
/*
 * T - Type
 * S - Stick number
 * A - Axe number
 * D - Direction
 * B - Button number
 * K - Key
 * X - Board/Device
 * N - Negate flag
 */

/**************************************************************************/
/* Digital */

/* The DIGITAL values are also saved in the .cfg file. If you change them */
/* the cfg become invalid. */

#define DIGITAL_TYPE_SPECIAL 0 /* Special codes */
#define DIGITAL_TYPE_JOY 1 /* Joy digital move - DAAASSSDDDTTT */
#define DIGITAL_TYPE_JOY_BUTTON 2 /* Joy button - BBBBBBDDDTTT */
#define DIGITAL_TYPE_MOUSE_BUTTON 3 /* Mouse button - BBBBBBDDDTTT */
#define DIGITAL_TYPE_KBD 4 /* Keyboard button - KKKKKKKKKKXXXTTT */
#define DIGITAL_TYPE_GET(i) ((i) & 0x7)

/**************************************************************************/
/* Keyboard */

#define DIGITAL_KBD_BOARD_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_KBD_KEY_GET(i) (((i) >> 6) & 0x3FF)
#define DIGITAL_KBD(board, key) (DIGITAL_TYPE_KBD | (board) << 3 | (key) << 6)

/**************************************************************************/
/* Joystick/Mouse */

/* Warning! Check the INPUT_*_MAX, they must match with the following macros. */

#define DIGITAL_JOY_DEV_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_JOY_STICK_GET(i) (((i) >> 6) & 0x7)
#define DIGITAL_JOY_AXE_GET(i) (((i) >> 9) & 0x7)
#define DIGITAL_JOY_DIR_GET(i) (((i) >> 12) & 0x1)
#define DIGITAL_JOY(joy, stick, axe, dir) (DIGITAL_TYPE_JOY | (joy) << 3 | (stick) << 6 | (axe) << 9 | (dir) << 12)

#define DIGITAL_JOY_BUTTON_DEV_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_JOY_BUTTON_BUTTON_GET(i) (((i) >> 6) & 0x3F)
#define DIGITAL_JOY_BUTTON(joy, button) (DIGITAL_TYPE_JOY_BUTTON | (joy) << 3 | (button) << 6)

#define DIGITAL_MOUSE_BUTTON_DEV_GET(i) (((i) >> 3) & 0x7)
#define DIGITAL_MOUSE_BUTTON_BUTTON_GET(i) (((i) >> 6) & 0x3F)
#define DIGITAL_MOUSE_BUTTON(mouse, button) (DIGITAL_TYPE_MOUSE_BUTTON | (mouse) << 3 | (button) << 6)

/**************************************************************************/
/* Analog */

/* The ANALOG value can be changed without limitation. */
#define ANALOG_TYPE_SPECIAL DIGITAL_TYPE_SPECIAL
#define ANALOG_TYPE_MOUSE 5 /* Mouse - NAAAXXXTTT */
#define ANALOG_TYPE_JOY 6 /* Joy - NAAASSSXXXTTT */
#define ANALOG_TYPE_BALL 7 /* Ball - NAAAXXXTTT */
#define ANALOG_TYPE_GET(i) DIGITAL_TYPE_GET(i)

/* Analog Mouse */
#define ANALOG_MOUSE_DEV_GET(i) (((i) >> 3) & 0x7)
#define ANALOG_MOUSE_AXE_GET(i) (((i) >> 6) & 0x7)
#define ANALOG_MOUSE_NEGATE_GET(i) (((i) >> 9) & 0x1)
#define ANALOG_MOUSE(dev, axe, negate) (ANALOG_TYPE_MOUSE | (dev) << 3 | (axe) << 6 | (negate) << 9)

/* Analog Joy */
#define ANALOG_JOY_DEV_GET(i) (((i) >> 3) & 0x7)
#define ANALOG_JOY_STICK_GET(i) (((i) >> 6) & 0x7)
#define ANALOG_JOY_AXE_GET(i) (((i) >> 9) & 0x7)
#define ANALOG_JOY_NEGATE_GET(i) (((i) >> 12) & 0x1)
#define ANALOG_JOY(joy, stick, axe, negate) (ANALOG_TYPE_JOY | (joy) << 3 | (stick) << 6 | (axe) << 9 | (negate) << 12)

/* Analog Joy/ball */
#define ANALOG_BALL_DEV_GET(i) (((i) >> 3) & 0x7)
#define ANALOG_BALL_AXE_GET(i) (((i) >> 6) & 0x7)
#define ANALOG_BALL_NEGATE_GET(i) (((i) >> 9) & 0x1)
#define ANALOG_BALL(joy, axe, negate) (ANALOG_TYPE_BALL | (joy) << 3 | (axe) << 6 | (negate) << 9)

/**************************************************************************/
/* Special */

#define DIGITAL_SPECIAL(code) (DIGITAL_TYPE_SPECIAL | (code) << 3)

#define DIGITAL_SPECIAL_NONE DIGITAL_SPECIAL(1)
#define DIGITAL_SPECIAL_OR DIGITAL_SPECIAL(2)
#define DIGITAL_SPECIAL_NOT DIGITAL_SPECIAL(3)
#define DIGITAL_SPECIAL_AUTO DIGITAL_SPECIAL(4)

#define ANALOG_SPECIAL_NONE DIGITAL_SPECIAL_NONE
#define ANALOG_SPECIAL_AUTO DIGITAL_SPECIAL_AUTO

#endif

