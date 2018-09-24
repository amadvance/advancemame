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
 */

/** \file
 * Key.
 */

/** \addtogroup Joystick */
/*@{*/

#ifndef __JOY_H
#define __JOY_H

#include "extra.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \name Joystick buttons
 * Joystick button definitions.
 */
/*@{*/
#define JOYB_0 0
#define JOYB_1 1
#define JOYB_2 2
#define JOYB_3 3
#define JOYB_4 4
#define JOYB_5 5
#define JOYB_6 6
#define JOYB_7 7
#define JOYB_8 8
#define JOYB_9 9
#define JOYB_10 10
#define JOYB_11 11
#define JOYB_12 12
#define JOYB_13 13
#define JOYB_14 14
#define JOYB_15 15

#define JOYB_BASE 128 /* start of named code */

#define JOYB_A 128
#define JOYB_B 129
#define JOYB_C 130
#define JOYB_X 131
#define JOYB_Y 132
#define JOYB_Z 133
#define JOYB_TL 134 /* gamepad top left */
#define JOYB_TR 135 /* gamepad top right */
#define JOYB_TL2 136 /* gamepad top left 2 */
#define JOYB_TR2 137 /* gamepad top right 2 */
#define JOYB_SELECT 138 /* gamepad select/back */
#define JOYB_START 139 /* gamepad start */
#define JOYB_MODE 140 /* gamepad mode */
#define JOYB_THUMBL 141 /* gamepad thumb left */
#define JOYB_THUMBR 142 /* gamepad thumb right */
#define JOYB_GEAR_DOWN 143 /* wheel */
#define JOYB_GEAR_UP 144 /* wheel */
#define JOYB_PLAY 145 /* play */

#define JOYB_MAX 256
/*@}*/

const char* joy_button_name(unsigned code);
unsigned joy_button_code(const char* name);
adv_bool joy_button_is_defined(unsigned code);

#ifdef __cplusplus
};
#endif

#endif

/*@}*/

