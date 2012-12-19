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

#ifndef __RESOURCE_H
#define __RESOURCE_H

#include "fz.h"

#include <string>

// ------------------------------------------------------------------------
// resource

class resource {
	std::string path;
	off_t offset;
	unsigned size_compressed;
	unsigned size_uncompressed;
	bool compressed;
	bool collection;
public:
	resource(const resource& A);

	resource();
	resource(const std::string& Apath);
	resource(const std::string& Apath, off_t Aoffset, unsigned Asize, bool is_collection);
	resource(const std::string& Apath, off_t Aoffset, unsigned Asize_compressed, unsigned Asize_uncompressed, bool is_collection);

	resource& operator=(const resource& A);

	bool operator==(const resource& A) const;
	bool operator!=(const resource& A) const { return !operator==(A); }

	bool is_valid() const;
	bool is_present() const;

	bool is_collection() const;
	bool is_deletable() const { return is_valid() && !is_collection(); }

	const std::string& path_get() const;
	std::string archive_get() const;

	adv_fz* open() const;
};

#endif
