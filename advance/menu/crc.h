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

#ifndef __CRC_H
#define __CRC_H

#include "game.h"

#include <string>
#include <set>

struct crc_info {
	unsigned crc;
	mutable std::string description;
	mutable std::string year;
	mutable std::string manufacturer;

	bool operator<(const crc_info& A) const {
		return crc < A.crc;
	}
};

typedef std::set<crc_info> crc_info_set;

struct crc_list {
	std::string name;
	mutable crc_info_set bag;

	bool operator<(const crc_list& A) const {
		return name < A.name;
	}
};

typedef std::set<crc_list> crc_list_set;

void crc_scan(crc_list_set& cls, const game_set& gs, const std::string& crc_dir);

unsigned crc_compute(unsigned crc, const unsigned char* buf, unsigned len);

#endif
