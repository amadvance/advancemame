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

#ifndef __CATEGORY_H
#define __CATEGORY_H

#include "game.h"

#include "common.h"
#include "resource.h"

#include <set>
#include <string>

// ------------------------------------------------------------------------
// category

class category {
	std::string name;
	bool state; /// if the category is listed or not
	bool undefined; /// if it's the undefined category

	category();
public:
	category(const category& A);
	~category();

	category(const std::string& Aname);
	category(const std::string& Aname, bool Aundefined);

	const std::string& name_get() const { return name; }

	void state_set(bool Astate) { state = Astate; }
	bool state_get() const { return state; }

	bool undefined_get() const { return undefined; }

	bool operator<(const category& A) const { return case_less(name, A.name); }
};

inline bool pgame_by_group_less(const game* A, const game* B)
{
	return *A->group_derived_get() < *B->group_derived_get();
}

inline bool pgame_by_type_less(const game* A, const game* B)
{
	return *A->type_derived_get() < *B->type_derived_get();
}

// ------------------------------------------------------------------------
// category_container

typedef std::set<std::string> category_container;

struct pcategory_less : std::binary_function<const category*, const category*, bool> {
	bool operator()(const category* A, const category* B) const
	{
		return *A < *B;
	}
};

typedef std::set<category*, pcategory_less> pcategory_container_base;

class pcategory_container : public pcategory_container_base {
	const category* undefined;
public:
	pcategory_container();
	~pcategory_container();

	const category* undefined_get() const
	{
		return undefined;
	}

	const category* insert(const std::string& name);
};

#endif

