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

#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Key definitions */
#define OS_KEY_A 1
#define OS_KEY_B 2
#define OS_KEY_C 3
#define OS_KEY_D 4
#define OS_KEY_E 5
#define OS_KEY_F 6
#define OS_KEY_G 7
#define OS_KEY_H 8
#define OS_KEY_I 9
#define OS_KEY_J 10
#define OS_KEY_K 11
#define OS_KEY_L 12
#define OS_KEY_M 13
#define OS_KEY_N 14
#define OS_KEY_O 15
#define OS_KEY_P 16
#define OS_KEY_Q 17
#define OS_KEY_R 18
#define OS_KEY_S 19
#define OS_KEY_T 20
#define OS_KEY_U 21
#define OS_KEY_V 22
#define OS_KEY_W 23
#define OS_KEY_X 24
#define OS_KEY_Y 25
#define OS_KEY_Z 26
#define OS_KEY_0 27
#define OS_KEY_1 28
#define OS_KEY_2 29
#define OS_KEY_3 30
#define OS_KEY_4 31
#define OS_KEY_5 32
#define OS_KEY_6 33
#define OS_KEY_7 34
#define OS_KEY_8 35
#define OS_KEY_9 36
#define OS_KEY_0_PAD 37
#define OS_KEY_1_PAD 38
#define OS_KEY_2_PAD 39
#define OS_KEY_3_PAD 40
#define OS_KEY_4_PAD 41
#define OS_KEY_5_PAD 42
#define OS_KEY_6_PAD 43
#define OS_KEY_7_PAD 44
#define OS_KEY_8_PAD 45
#define OS_KEY_9_PAD 46
#define OS_KEY_F1 47
#define OS_KEY_F2 48
#define OS_KEY_F3 49
#define OS_KEY_F4 50
#define OS_KEY_F5 51
#define OS_KEY_F6 52
#define OS_KEY_F7 53
#define OS_KEY_F8 54
#define OS_KEY_F9 55
#define OS_KEY_F10 56
#define OS_KEY_F11 57
#define OS_KEY_F12 58
#define OS_KEY_ESC 59
#define OS_KEY_BACKQUOTE 60
#define OS_KEY_MINUS 61
#define OS_KEY_EQUALS 62
#define OS_KEY_BACKSPACE 63
#define OS_KEY_TAB 64
#define OS_KEY_OPENBRACE 65
#define OS_KEY_CLOSEBRACE 66
#define OS_KEY_ENTER 67
#define OS_KEY_SEMICOLON 68
#define OS_KEY_QUOTE 69
#define OS_KEY_BACKSLASH 70
#define OS_KEY_LESS 71
#define OS_KEY_COMMA 72
#define OS_KEY_PERIOD 73
#define OS_KEY_SLASH 74
#define OS_KEY_SPACE 75
#define OS_KEY_INSERT 76
#define OS_KEY_DEL 77
#define OS_KEY_HOME 78
#define OS_KEY_END 79
#define OS_KEY_PGUP 80
#define OS_KEY_PGDN 81
#define OS_KEY_LEFT 82
#define OS_KEY_RIGHT 83
#define OS_KEY_UP 84
#define OS_KEY_DOWN 85
#define OS_KEY_SLASH_PAD 86
#define OS_KEY_ASTERISK 87
#define OS_KEY_MINUS_PAD 88
#define OS_KEY_PLUS_PAD 89
#define OS_KEY_PERIOD_PAD 90
#define OS_KEY_ENTER_PAD 91
#define OS_KEY_PRTSCR 92
#define OS_KEY_PAUSE 93
#define OS_KEY_LSHIFT 94
#define OS_KEY_RSHIFT 95
#define OS_KEY_LCONTROL 96
#define OS_KEY_RCONTROL 97
#define OS_KEY_ALT 98
#define OS_KEY_ALTGR 99
#define OS_KEY_LWIN 100
#define OS_KEY_RWIN 101
#define OS_KEY_MENU 102
#define OS_KEY_SCRLOCK 103
#define OS_KEY_NUMLOCK 104
#define OS_KEY_CAPSLOCK 105
#define OS_KEY_MAX 128

const char* key_name(unsigned code);
unsigned key_code(const char* name);

#ifdef __cplusplus
};
#endif

#endif
