/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

/** \addtogroup Keyboard */
/*@{*/

#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Key definitions. They are chosen like the Allegro library 4.0.1 */
#define KEYB_A 1
#define KEYB_B 2
#define KEYB_C 3
#define KEYB_D 4
#define KEYB_E 5
#define KEYB_F 6
#define KEYB_G 7
#define KEYB_H 8
#define KEYB_I 9
#define KEYB_J 10
#define KEYB_K 11
#define KEYB_L 12
#define KEYB_M 13
#define KEYB_N 14
#define KEYB_O 15
#define KEYB_P 16
#define KEYB_Q 17
#define KEYB_R 18
#define KEYB_S 19
#define KEYB_T 20
#define KEYB_U 21
#define KEYB_V 22
#define KEYB_W 23
#define KEYB_X 24
#define KEYB_Y 25
#define KEYB_Z 26
#define KEYB_0 27
#define KEYB_1 28
#define KEYB_2 29
#define KEYB_3 30
#define KEYB_4 31
#define KEYB_5 32
#define KEYB_6 33
#define KEYB_7 34
#define KEYB_8 35
#define KEYB_9 36
#define KEYB_0_PAD 37
#define KEYB_1_PAD 38
#define KEYB_2_PAD 39
#define KEYB_3_PAD 40
#define KEYB_4_PAD 41
#define KEYB_5_PAD 42
#define KEYB_6_PAD 43
#define KEYB_7_PAD 44
#define KEYB_8_PAD 45
#define KEYB_9_PAD 46
#define KEYB_F1 47
#define KEYB_F2 48
#define KEYB_F3 49
#define KEYB_F4 50
#define KEYB_F5 51
#define KEYB_F6 52
#define KEYB_F7 53
#define KEYB_F8 54
#define KEYB_F9 55
#define KEYB_F10 56
#define KEYB_F11 57
#define KEYB_F12 58
#define KEYB_ESC 59
#define KEYB_BACKQUOTE 60
#define KEYB_MINUS 61
#define KEYB_EQUALS 62
#define KEYB_BACKSPACE 63
#define KEYB_TAB 64
#define KEYB_OPENBRACE 65
#define KEYB_CLOSEBRACE 66
#define KEYB_ENTER 67
#define KEYB_SEMICOLON 68
#define KEYB_QUOTE 69
#define KEYB_BACKSLASH 70
#define KEYB_LESS 71
#define KEYB_COMMA 72
#define KEYB_PERIOD 73
#define KEYB_SLASH 74
#define KEYB_SPACE 75
#define KEYB_INSERT 76
#define KEYB_DEL 77
#define KEYB_HOME 78
#define KEYB_END 79
#define KEYB_PGUP 80
#define KEYB_PGDN 81
#define KEYB_LEFT 82
#define KEYB_RIGHT 83
#define KEYB_UP 84
#define KEYB_DOWN 85
#define KEYB_SLASH_PAD 86
#define KEYB_ASTERISK 87
#define KEYB_MINUS_PAD 88
#define KEYB_PLUS_PAD 89
#define KEYB_PERIOD_PAD 90
#define KEYB_ENTER_PAD 91
#define KEYB_PRTSCR 92
#define KEYB_PAUSE 93
/* hole */
#define KEYB_LSHIFT 103
#define KEYB_RSHIFT 104
#define KEYB_LCONTROL 105
#define KEYB_RCONTROL 106
#define KEYB_ALT 107
#define KEYB_ALTGR 108
#define KEYB_LWIN 109
#define KEYB_RWIN 110
#define KEYB_MENU 111
#define KEYB_SCRLOCK 112
#define KEYB_NUMLOCK 113
#define KEYB_CAPSLOCK 114
/* hole */
#define KEYB_MAX 128

const char* key_name(unsigned code);
unsigned key_code(const char* name);

#ifdef __cplusplus
};
#endif

#endif

/*@}*/
