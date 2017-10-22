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

#ifndef __MENU_H
#define __MENU_H

#include "mconfig.h"

#include <vector>

// ------------------------------------------------------------------------
// Menu array

class menu_entry {
	const game* g;
	std::string desc;
	unsigned ident;
public:
	menu_entry(const game* Ag, unsigned Aident);
	menu_entry(const std::string& desc);

	bool has_game() const { return g != 0; }
	const game& game_get() const { return *g; }
	bool is_selectable() const { return has_game(); }
	const std::string& desc_get() const { return has_game() ? game_get().description_get() : desc; }
	unsigned ident_get() const { return ident; }

	std::string category(sort_item_func* category_extract);
};

typedef std::vector<menu_entry*> menu_array;

int run_menu(config_state& rs, bool flipxy, bool silent);

#endif

