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

#include "category.h"
#include "game.h"

#include <iostream>
#include <fstream>

#include <sys/stat.h>

using namespace std;

// ------------------------------------------------------------------------
// category

category::category(const category& A) : name(A.name), state(A.state), undefined(false) {
}

category::~category() {
}

category::category(const string& Aname) : name(Aname), state(false), undefined(false) {
}

category::category(const string& Aname, bool Aundefined) : name(Aname), state(false), undefined(Aundefined) {
}

// ------------------------------------------------------------------------
// category container

#define CATEGORY_UNDEFINED "<undefined>"

pcategory_container::pcategory_container()  {
	category* c = new category(CATEGORY_UNDEFINED,true);
	undefined = c;
	pcategory_container_base::insert(c);
}

pcategory_container::~pcategory_container() {
}

const category* pcategory_container::insert(const std::string& name) {
	if (name.length() == 0 || name == CATEGORY_UNDEFINED)
		return undefined;

	category* c = new category(name);

	pair<pcategory_container_base::iterator,bool> i = pcategory_container_base::insert(c);

	if (i.second) {
		// inserted
	} else {
		// not inserted
		delete c;
	}

	// return the category in the container
	return *i.first;
}

const category* pcategory_container::insert_double(const string& name, category_container& cat_include) {
	if (name.length() == 0 || name == CATEGORY_UNDEFINED)
		return undefined;

	category* c = new category(name);

	pair<pcategory_container_base::iterator,bool> i = pcategory_container_base::insert(c);

	if (i.second) {
		// inserted
		cat_include.insert(name);
	} else {
		// not inserted
		delete c;
	}

	// return the category in the container
	return *i.first;
}

void pcategory_container::import_ini(game_set& gar, const string& file, const string& section, const string& emulator, void (game::*set)(const category*) const, category_container& include) {
	int j = 0;
	string ss = file_read( file );

	bool in = false;
	while (j < ss.length()) {
		string s = token_get(ss,j,"\r\n");
		token_skip(ss,j,"\r\n");

		int i = 0;
		token_skip(s, i," \t");

		if (i<s.length() && s[i]=='[') {
			token_skip(s, i, "[");
			string cmd = token_get(s, i, "]");
			in = cmd == section;
		} else if (in && i<s.length() && isalnum(s[i])) {
			string tag = token_get(s, i, " =");
			token_skip(s, i, " =");
			string category = token_get(s, i, "");
			if (category.length()) {
				string name = emulator + "/" + tag;
				game_set::const_iterator i = gar.find( game( name ) );
				if (i!=gar.end()) {
					((*i).*set)(insert_double(category,include));
				}
			}
		}
	}
}

void pcategory_container::import_mac(game_set& gar, const string& file, const string& section, const string& emulator, void (game::*set)(const category*) const, category_container& include) {
	(void)section;
	int j = 0;

	string ss = file_read( file );

	string main_category;
	while (j < ss.length()) {

		string s = token_get(ss,j,"\r\n");
		token_skip(ss,j,"\r\n");

		int i = 0;
		token_skip(s, i," \t");

		if (i<s.length() && isalnum(s[i])) {
			string tag = token_get(s, i, " \t");
			token_skip(s, i," \t");
			string category = token_get(s, i, "");
			if (category.length()) {
				main_category = category;
				// remove special chars
				if (main_category.length() && main_category[main_category.length()-1]=='-')
					main_category.erase(main_category.length()-1,1);
				if (main_category.length() && main_category[0] == '¥')
					main_category.erase(0,1);
			}
			if (main_category.length()) {
				string name = emulator + "/" + tag;
				game_set::const_iterator i = gar.find( game( name ) );
				if (i!=gar.end()) {
					((*i).*set)(insert_double(main_category,include));
				}
			}
		}
	}
}
