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

#ifndef __COMMON_H
#define __COMMON_H

#include "file.h"

#include <string>
#include <list>

typedef std::list<std::string> path_container;

inline std::string path_import(const std::string& s)
{
	return file_import(s.c_str());
}

inline std::string path_export(const std::string& s)
{
	return file_export(s.c_str());
}

inline std::string list_import(const std::string& s)
{
	return file_import(s.c_str());
}

inline std::string list_export(const std::string& s)
{
	return file_export(s.c_str());
}

inline const char* cpath_import(const std::string& s)
{
	return file_import(s.c_str());
}

inline const char* cpath_export(const std::string& s)
{
	return file_export(s.c_str());
}

inline const char* clist_import(const std::string& s)
{
	return file_import(s.c_str());
}

inline const char* clist_export(const std::string& s)
{
	return file_export(s.c_str());
}

std::string slash_add(const std::string& s);
std::string slash_remove(const std::string& s);

std::string strip_space(const std::string& s);
std::string strip_comment(const std::string& s);

std::string file_basename(const std::string& s);
std::string file_ext(const std::string& s);
std::string file_dir(const std::string& s);
std::string file_file(const std::string& s);
bool file_crc(const std::string& file, unsigned& crc);
std::string file_select_random(const path_container& c);

bool file_exists(const std::string& path);

std::string file_read(const std::string& file);

std::string dir_cat(const std::string& A, const std::string& B);
std::string dir_cwd();

std::string path_abs(const std::string& rel, const std::string& cwd);
std::string path_rel(const std::string& abs, const std::string& cwd);
std::string path_short(const std::string& abs, const std::string& cwd);
std::string list_abs(const std::string& rel, const std::string& cwd);

bool file_findinzip_byfile(const std::string& zip_file, const std::string& name, std::string& file, unsigned& crc);
bool file_findinzip_byname(const std::string& zip_file, const std::string& name, std::string& file, unsigned& crc);

std::string token_get(const std::string& s, int& ptr, const char* sep);
void token_skip(const std::string& s, int& ptr, const char* sep);
std::string token_get(const std::string& s, int& ptr, char sep);
void token_skip(const std::string& s, int& ptr, char sep);

std::string arg_get(const std::string& s, int& ptr);
bool arg_split(const std::string& s, std::string& a0);
bool arg_split(const std::string& s, std::string& a0, std::string& a1);
bool arg_split(const std::string& s, std::string& a0, std::string& a1, std::string& a2);
bool arg_split(const std::string& s, std::string& a0, std::string& a1, std::string& a2, std::string& a3);

bool case_less(const std::string& A, const std::string& B);
bool case_equal(const std::string& A, const std::string& B);
std::string case_auto(const std::string& A);
std::string case_upper(const std::string& A);

std::string subs(const std::string& s, const std::string& from, const std::string& to);

bool is_glob(const char* str_begin, const char* str_end, const char* glob_begin, const char* glob_end);
bool is_glob(const std::string& str, const std::string& glob);
bool is_globlist(const std::string& file, const std::string& globlist);


#endif

