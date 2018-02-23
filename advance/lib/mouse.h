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

/** \addtogroup Mouse */
/*@{*/

#ifndef __MOUSE_H
#define __MOUSE_H

#include "extra.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \name Mouse buttons
 * Mouse button definitions.
 */
/*@{*/
#define MOUSEB_0 0
#define MOUSEB_1 1
#define MOUSEB_2 2
#define MOUSEB_3 3
#define MOUSEB_4 4
#define MOUSEB_5 5
#define MOUSEB_6 6
#define MOUSEB_7 7
#define MOUSEB_8 8
#define MOUSEB_9 9
#define MOUSEB_10 10
#define MOUSEB_11 11
#define MOUSEB_12 12
#define MOUSEB_13 13
#define MOUSEB_14 14
#define MOUSEB_15 15

#define MOUSEB_BASE 128 /* start of named code */

#define MOUSEB_LEFT 128
#define MOUSEB_RIGHT 129
#define MOUSEB_MIDDLE 130
#define MOUSEB_SIDE 131
#define MOUSEB_EXTRA 132
#define MOUSEB_FORWARD 133
#define MOUSEB_BACK 134

#define MOUSEB_MAX 256
/*@}*/

const char* mouse_button_name(unsigned code);
unsigned mouse_button_code(const char* name);
adv_bool mouse_button_is_defined(unsigned code);

#ifdef __cplusplus
};
#endif

#endif

/*@}*/

