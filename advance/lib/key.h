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

/** \addtogroup Keyboard */
/*@{*/

#ifndef __KEY_H
#define __KEY_H

#include "extra.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \name Keys
 * Key definitions.
 * \note They are chosen like the Allegro library 4.0.1.
 */
/*@{*/
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

/* Extra Key definitions. They are chosen like linux/input.h */
#define KEYB_STOP 128
#define KEYB_AGAIN 129
#define KEYB_PROPS 130
#define KEYB_UNDO 131
#define KEYB_FRONT 132
#define KEYB_COPY 133
#define KEYB_OPEN 134
#define KEYB_PASTE 135
#define KEYB_FIND 136
#define KEYB_CUT 137
#define KEYB_HELP 138

/* #define KEYB_MENU 139 */ /* already present in the previous list */
/* hole */

#define KEYB_CALC 140
#define KEYB_SETUP 141
#define KEYB_SLEEP 142
#define KEYB_WAKEUP 143
#define KEYB_FILE 144
#define KEYB_SENDFILE 145
#define KEYB_DELETEFILE 146
#define KEYB_XFER 147
#define KEYB_PROG1 148
#define KEYB_PROG2 149
#define KEYB_WWW 150
#define KEYB_MSDOS 151
#define KEYB_COFFEE 152
#define KEYB_DIRECTION 153
#define KEYB_CYCLEWINDOWS 154
#define KEYB_MAIL 155
#define KEYB_BOOKMARKS 156
#define KEYB_COMPUTER 157
#define KEYB_BACK 158
#define KEYB_FORWARD 159
#define KEYB_CLOSECD 160
#define KEYB_EJECTCD 161
#define KEYB_EJECTCLOSECD 162
#define KEYB_NEXTSONG 163
#define KEYB_PLAYPAUSE 164
#define KEYB_PREVIOUSSONG 165
#define KEYB_STOPCD 166
#define KEYB_RECORD 167
#define KEYB_REWIND 168
#define KEYB_PHONE 169
#define KEYB_ISO 170
#define KEYB_CONFIG 171
#define KEYB_HOMEPAGE 172
#define KEYB_REFRESH 173
#define KEYB_EXIT 174
#define KEYB_MOVE 175
#define KEYB_EDIT 176
#define KEYB_SCROLLUP 177
#define KEYB_SCROLLDOWN 178
#define KEYB_KPLEFTPAREN 179
#define KEYB_KPRIGHTPAREN 180
#define KEYB_INTL1 181
#define KEYB_INTL2 182
#define KEYB_INTL3 183
#define KEYB_INTL4 184
#define KEYB_INTL5 185
#define KEYB_INTL6 186
#define KEYB_INTL7 187
#define KEYB_INTL8 188
#define KEYB_INTL9 189
#define KEYB_LANG1 190
#define KEYB_LANG2 191
#define KEYB_LANG3 192
#define KEYB_LANG4 193
#define KEYB_LANG5 194
#define KEYB_LANG6 195
#define KEYB_LANG7 196
#define KEYB_LANG8 197
#define KEYB_LANG9 198

/* hole */

#define KEYB_PLAYCD 200
#define KEYB_PAUSECD 201
#define KEYB_PROG3 202
#define KEYB_PROG4 203

/* hole */

#define KEYB_SUSPEND 205
#define KEYB_CLOSE 206

/* hole */

#define KEYB_BRIGHTNESSDOWN 224
#define KEYB_BRIGHTNESSUP 225

/* hole */

/* Extra Key definitions. They are chosen like linux/input.h with different number */
#define KEYB_MACRO 230
#define KEYB_MUTE 231
#define KEYB_VOLUMEDOWN 232
#define KEYB_VOLUMEUP 233
#define KEYB_POWER 234
#define KEYB_COMPOSE 235
#define KEYB_F13 236
#define KEYB_F14 237
#define KEYB_F15 238
#define KEYB_F16 239
#define KEYB_F17 240
#define KEYB_F18 241
#define KEYB_F19 242
#define KEYB_F20 243
#define KEYB_F21 244
#define KEYB_F22 245
#define KEYB_F23 246
#define KEYB_F24 247

/* hole */

#define KEYB_MAX 256
/*@}*/

const char* key_name(unsigned code);
unsigned key_code(const char* name);
adv_bool key_is_defined(unsigned code);

#ifdef __cplusplus
};
#endif

#endif

/*@}*/
