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

#include "portable.h"

#include "category.h"
#include "game.h"

#include <iostream>
#include <fstream>

using namespace std;

// ------------------------------------------------------------------------
// category

category::category(const category& A) : name(A.name), state(A.state), undefined(false)
{
}

category::~category()
{
}

category::category(const string& Aname) : name(Aname), state(false), undefined(false)
{
}

category::category(const string& Aname, bool Aundefined) : name(Aname), state(false), undefined(Aundefined)
{
}

// ------------------------------------------------------------------------
// category container

#define CATEGORY_UNDEFINED "<undefined>"

pcategory_container::pcategory_container()
{
	category* c = new category(CATEGORY_UNDEFINED, true);
	undefined = c;
	pcategory_container_base::insert(c);
}

pcategory_container::~pcategory_container()
{
}

const category* pcategory_container::insert(const std::string& name)
{
	if (name.length() == 0 || name == CATEGORY_UNDEFINED)
		return undefined;

	category* c = new category(name);

	pair<pcategory_container_base::iterator, bool> i = pcategory_container_base::insert(c);

	if (i.second) {
		// inserted
	} else {
		// not inserted
		delete c;
	}

	// return the category in the container
	return *i.first;
}

