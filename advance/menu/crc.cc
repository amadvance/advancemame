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

#include "crc.h"
#include "common.h"

#include <zlib.h>

#include <fstream>

using namespace std;

unsigned crc_compute(unsigned crc, const unsigned char* buf, unsigned len)
{
	return crc32(crc, buf, len);
}

static void crc_scan(const crc_list* cl, istream& f)
{
	while (!f.eof()) {
		string s;
		getline(f, s, '\n');

		if (s.length() && s[0]!='#') {
			int ptr = 0;

			crc_info ci;

			token_skip(s, ptr, " ");
			string first = token_get(s, ptr, " =");
			if (first.length() && first[0]!='[') {
				ci.crc = strtoul(first.c_str(), 0, 16);
				if (ci.crc) {
					token_skip(s, ptr, " =");
					ci.description = strip_space(token_get(s, ptr, "|"));
					token_skip(s, ptr, " ");
					if (ptr < s.length() && s[ptr]=='|') ++ptr;
					token_skip(s, ptr, " ");
					ci.year = strip_space(token_get(s, ptr, "|"));
					token_skip(s, ptr, " ");
					if (ptr < s.length() && s[ptr]=='|') ++ptr;
					token_skip(s, ptr, " ");
					ci.manufacturer = strip_space(token_get(s, ptr, "|"));
					cl->bag.insert(ci);
				}
			}
		}
	}
}

void crc_scan(crc_list_set& cls, const game_set& gs, const string& crc_dir)
{
	for(game_set::const_iterator i=gs.begin();i!=gs.end();++i) {
		string path = crc_dir + i->name_get() + ".crc";
		ifstream f(cpath_export(path));
		if (f.good()) {
			crc_list cl;
			cl.name = i->name_get();
			pair<crc_list_set::const_iterator, bool> j = cls.insert(cl);
			crc_scan(&*j.first, f);
			f.close();
		}
	}
}
