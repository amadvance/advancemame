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

#ifndef __CATEGORY_H
#define __CATEGORY_H

#include "game.h"

#include "common.h"
#include "resource.h"

#include <set>
#include <string>

// ------------------------------------------------------------------------
// Category

#define CATEGORY_UNDEFINED "<undefined>"

typedef std::set<std::string> category_base_container;

class category_container : public category_base_container {
public:
	void insert_double(const std::string& name, category_container& cat_include);

	void import_ini(game_set& gar, const std::string& file, const std::string& section, const std::string& emulator, void (game::*set)(const std::string& s) const, category_container& include);
	void import_mac(game_set& gar, const std::string& file, const std::string& section, const std::string& emulator, void (game::*set)(const std::string& s) const, category_container& include);
};

#endif

