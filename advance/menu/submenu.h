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

#ifndef __SUBMENU_H
#define __SUBMENU_H

#include "mconfig.h"

void run_help(config_state& rs);
int run_volume(config_state& rs);
bool run_exit(config_state& rs, int key);
int run_sort(config_state& rs);
void run_exclude(config_state& rs);
int run_type(config_state& rs);
void run_type_next(config_state& rs);
int run_type_move(config_state& rs);
int run_group(config_state& rs);
void run_group_next(config_state& rs);
int run_group_move(config_state& rs);
void run_clone(config_state& rs);
int run_command(config_state& rs);
int run_emu(config_state& rs);
void run_emu_next(config_state& rs);
emulator* run_emu_select(config_state& rs);
int run_submenu(config_state& rs);
void run_runinfo(config_state& rs);
void run_calib(config_state& rs);
int run_difficulty(config_state& rs);
void run_stat(config_state& rs);

#define FIRST_CHOICE_X (int_dx_get() / 20)
#define FIRST_CHOICE_Y (int_dy_get() / 15)
#define SECOND_CHOICE_X (FIRST_CHOICE_X + int_font_dx_get(text) * 2)
#define SECOND_CHOICE_Y (FIRST_CHOICE_Y + int_font_dx_get(text) * 2)
#define THIRD_CHOICE_X (SECOND_CHOICE_X + int_font_dx_get(text) * 2)
#define THIRD_CHOICE_Y (SECOND_CHOICE_Y + int_font_dx_get(text) * 2)
#define FOURTH_CHOICE_X (THIRD_CHOICE_X + int_font_dx_get(text) * 2)
#define FOURTH_CHOICE_Y (THIRD_CHOICE_Y + int_font_dx_get(text) * 2)

#endif

