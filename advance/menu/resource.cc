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

#include "resource.h"
#include "common.h"

using namespace std;

// ------------------------------------------------------------------------
// resource

resource::resource(const resource& A) :
	path(A.path), offset(A.offset), size_compressed(A.size_compressed), size_uncompressed(A.size_uncompressed), compressed(A.compressed), collection(A.collection) {
}

resource::resource()
{
	offset = 0;
	size_uncompressed = 0;
	size_compressed = 0;
	compressed = false;
	collection = false;
}

resource::resource(const string& Apath)
{
	path = Apath;
	offset = 0;
	size_uncompressed = 0;
	size_compressed = 0;
	compressed = false;
	collection = false;
}

resource::resource(const string& Apath, off_t Aoffset, unsigned Asize, bool Acollection)
{
	path = Apath;
	offset = Aoffset;
	size_uncompressed = Asize;
	size_compressed = Asize;
	compressed = false;
	collection = Acollection;
}

resource::resource(const string& Apath, off_t Aoffset, unsigned Asize_compressed, unsigned Asize_uncompressed, bool Acollection)
{
	path = Apath;
	offset = Aoffset;
	size_uncompressed = Asize_uncompressed;
	size_compressed = Asize_compressed;
	compressed = true;
	collection = Acollection;
}

resource& resource::operator=(const resource& A)
{
	path = A.path;
	offset = A.offset;
	size_compressed = A.size_compressed;
	size_uncompressed = A.size_uncompressed;
	compressed = A.compressed;
	collection = A.collection;

	return *this;
}

bool resource::is_valid() const
{
	if (path.length() == 0)
		return false;
	return true;
}

const string& resource::path_get() const
{
	return path;
}

string resource::archive_get() const
{
	if (is_collection()) {
		return slash_remove(file_dir(path_get()));
	} else
		return path_get();
}

bool resource::is_present() const
{
	if (!is_valid())
		return false;
	if (access(cpath_export(archive_get()), F_OK | R_OK)!=0)
		return false;
	return true;
}

bool resource::is_collection() const
{
	return collection;
}

adv_fz* resource::open() const
{
	if (!is_valid())
		return 0;
	if (compressed) {
		return fzopenzipcompressed(cpath_export(archive_get()), offset, size_compressed, size_uncompressed);
	} else {
		if (is_collection())
			return fzopenzipuncompressed(cpath_export(archive_get()), offset, size_uncompressed);
		else
			return fzopen(cpath_export(archive_get()), "rb");
	}
}

bool resource::operator==(const resource& A) const
{
	if (path != A.path)
		return false;
	if (compressed != A.compressed)
		return false;
	if (compressed) {
		if (offset != A.offset)
			return false;
	}
	return true;
}
