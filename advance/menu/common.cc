/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005 Andrea Mazzoleni
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

#include "common.h"
#include "crc.h"

#include "advance.h"

#include <iostream>

using namespace std;

string file_basename(const string& s)
{
	int i = s.rfind('.');
	if (i == string::npos)
		return s;
	else
		return string(s, 0, i);
}

string file_ext(const string& s)
{
	int i = s.rfind('.');
	if (i == string::npos)
		return "";
	else
		return string(s, i, s.length() - i);
}

string file_dir(const string& s)
{
	int i = s.find_last_of("/");
	if (i == string::npos)
		return "";
	else
		return string(s, 0, i + 1);
}

string file_file(const string& s)
{
	int i = s.find_last_of("/");
	if (i == string::npos)
		return s;
	else
		return string(s, i + 1, s.length() - (i + 1));
}

string slash_add(const string& s)
{
	string r = s;
	if (r.length() && r[r.length() - 1] != '/')
		r += '/';
	return r;
}

bool file_exists(const string& path)
{
	return access(cpath_export(path), F_OK) == 0;
}

string slash_remove(const string& s)
{
	if (s.length() && s[s.length() - 1] == '/')
		return s.substr(0, s.length() - 1);
	else
		return s;
}

bool file_crc(const string& file, unsigned& crc)
{
	struct stat st;

	if (stat(cpath_export(file), &st) != 0)
		return false;

	FILE* f = fopen(cpath_export(file), "rb");
	if (!f)
		return false;

	unsigned char* data = (unsigned char*)operator new(st.st_size);

	if (fread(data, st.st_size, 1, f) != 1) {
		fclose(f);
		return false;
	}

	fclose(f);

	crc = crc_compute(0, data, st.st_size);

	operator delete(data);
	return true;
}

string token_get(const string& s, int& ptr, const char* sep)
{
	int start = ptr;
	while (ptr < s.length() && strchr(sep, s[ptr]) == 0)
		++ptr;
	return string(s, start, ptr - start);
}

void token_skip(const string& s, int& ptr, const char* sep)
{
	while (ptr < s.length() && strchr(sep, s[ptr]) != 0)
		++ptr;
}

std::string token_get(const std::string& s, int& ptr, char sep)
{
	char sep_string[2];
	sep_string[0] = sep;
	sep_string[1] = 0;
	return token_get(s, ptr, sep_string);
}

void token_skip(const std::string& s, int& ptr, char sep)
{
	char sep_string[2];
	sep_string[0] = sep;
	sep_string[1] = 0;
	token_skip(s, ptr, sep_string);
}

string strip_space(const string& s)
{
	string r = s;
	while (r.length() && isspace(r[0]))
		r.erase(0, 1);
	while (r.length() && isspace(r[r.length() - 1]))
		r.erase(r.length() - 1, 1);
	return r;
}

string strip_comment(const string& s)
{
	bool in = false;
	bool pred_space = true;
	string r;
	for (int i = 0; i < s.length(); ++i) {
		if (s[i] == '(' || s[i] == '[') {
			in = true;
		} else if (s[i] == ')' || s[i] == ']') {
			in = false;
		} else {
			if (!in) {
				if (!isspace(s[i]) || !pred_space) {
					r += s[i];
					pred_space = isspace(s[i]);
				}
			}
		}
	}

	r = strip_space(r);

	return r;
}

bool file_findinzip_byname(const string& zip_file, const string& name, string& file, unsigned& crc)
{
	adv_zip* zip;
	adv_zipent* ent;

	if (access(cpath_export(zip_file), F_OK) != 0)
		return false;

	zip = zip_open(cpath_export(zip_file));
	if (!zip)
		return false;

	while ((ent = zip_read(zip)) != 0) {
		string zfile = ent->name;
		string zname = file_basename(zfile);
		if (zname == name) {
			crc = ent->crc32;
			file = zfile;
			zip_close(zip);
			return true;
		}
	}

	zip_close(zip);
	return false;
}

bool file_findinzip_byfile(const string& zip_file, const string& name, string& file, unsigned& crc)
{
	adv_zip* zip;
	adv_zipent* ent;

	if (access(cpath_export(zip_file), F_OK) != 0)
		return false;

	zip = zip_open(cpath_export(zip_file));
	if (!zip)
		return false;

	while ((ent = zip_read(zip)) != 0) {
		string zfile = ent->name;
		if (zfile == name) {
			crc = ent->crc32;
			file = zfile;
			zip_close(zip);
			return true;
		}
	}

	zip_close(zip);
	return false;
}

string file_select_random(const path_container& c)
{
	int n = rand() % c.size();
	path_container::const_iterator i = c.begin();
	while (n) {
		++i;
		--n;
	}
	return *i;
}

string arg_get(const string& s, int& ptr)
{
	// skip spaces
	while (ptr < s.length() && isspace(s[ptr]))
		++ptr;

	string r;
	if (ptr < s.length() && s[ptr] == '"') {
		++ptr;
		while (ptr < s.length() && s[ptr] != '"') {
			r += s[ptr];
			++ptr;
		}
		if (ptr < s.length() && s[ptr] == '"')
			++ptr;
	} else {
		while (ptr < s.length() && !isspace(s[ptr])) {
			r += s[ptr];
			++ptr;
		}
	}

	// skip spaces
	while (ptr < s.length() && isspace(s[ptr]))
		++ptr;

	return r;
}

bool arg_split(const string& s, string& a0)
{
	int i = 0;
	a0 = arg_get(s, i);
	return i == s.length();
}

bool arg_split(const string& s, string& a0, string& a1)
{
	int i = 0;
	a0 = arg_get(s, i);
	a1 = arg_get(s, i);
	return i == s.length();
}

bool arg_split(const string& s, string& a0, string& a1, string& a2)
{
	int i = 0;
	a0 = arg_get(s, i);
	a1 = arg_get(s, i);
	a2 = arg_get(s, i);
	return i == s.length();
}

bool arg_split(const string& s, string& a0, string& a1, string& a2, string& a3)
{
	int i = 0;
	a0 = arg_get(s, i);
	a1 = arg_get(s, i);
	a2 = arg_get(s, i);
	a3 = arg_get(s, i);
	return i == s.length();
}

string dir_cat(const string& A, const string& B)
{
	if (A.length()) {
		if (B.length())
			return A + ':' + B;
		else
			return A;
	} else {
		if (B.length())
			return B;
		else
			return "";
	}
}

string path_abs(const string& rel, const string& cwd)
{
	string r = slash_remove(cwd);
	int pos = 0;
	while (pos < rel.length()) {
		if (rel.length() >= 3 && rel.substr(pos, 3) == "../") {
			int slash = r.rfind('/');
			if (slash != string::npos) {
				r.erase(slash, r.length() - slash);
			}
			pos += 3;
		} else if (rel.length() >= 2 && rel.substr(pos, 2) == "./") {
			pos += 2;
		} else if (rel.length() >= 1 && rel.substr(pos, 1) == "/") {
			r = "/";
			pos += 1;
		} else {
			int slash = rel.find('/', pos);
			r = slash_add(r);
			if (slash == string::npos) {
				r += rel.substr(pos, rel.length() - pos);
				pos = rel.length();
			} else {
				r += rel.substr(pos, slash - pos);
				pos = slash + 1;
			}
		}
	}
	if (r.length() == 0)
		r = '/';
	return r;
}

string path_rel(const string& abs, const string& cwd)
{
	string c = slash_add(cwd);

	unsigned pos = 0;
	int slash = c.find('/', pos);
	while (slash != string::npos && abs.substr(pos, slash - pos) == cwd.substr(pos, slash - pos)) {
		pos = slash + 1;
		slash = c.find('/', pos);
	}

	unsigned last_pos = pos;
	string r;
	while (slash != string::npos) {
		r += "../";
		pos = slash + 1;
		slash = c.find('/', pos);
	}

	r += abs.substr(last_pos, abs.length() - last_pos);

	return r;
}

string path_short(const string& abs, const string& cwd)
{
	string rel = path_rel(abs, cwd);
	if (rel.length() < abs.length())
		return rel;
	else
		return abs;
}

string list_abs(const string& list, const string& cwd)
{
	string r;
	int i = 0;
	while (i < list.length()) {
		if (r.length())
			r += ':';
		r += path_abs(token_get(list, i, ":"), cwd);
		token_skip(list, i, ":");
	}
	return r;
}

string dir_cwd()
{
	char cwd[FILE_MAXPATH];
	if (getcwd(cwd, FILE_MAXPATH) == 0) {
		return "/";
	} else {
		return file_import(cwd);
	}
}

bool case_less(const string& A, const string& B)
{
	int i = 0;
	int l = min(A.length(), B.length());
	while (i < l) {
		char ca = toupper(A[i]);
		char cb = toupper(B[i]);
		if (ca < cb)
			return true;
		if (ca > cb)
			return false;
		++i;
	}
	return A.length() < B.length();
}

bool case_equal(const string& A, const string& B)
{
	if (A.length() != B.length())
		return false;

	for (int i = 0; i < A.length(); ++i)
		if (toupper(A[i]) != toupper(B[i]))
			return false;

	return true;
}

string case_auto(const string& A)
{
	for (unsigned i = 0; i < A.length(); ++i)
		if (isupper(A[i]))
			return A;

	string r = A;
	for (unsigned i = 0; i < r.length(); ++i) {
		if (i == 0 || isspace(r[i - 1]))
			r[i] = toupper(r[i]);
	}

	return r;
}

string case_upper(const string& A)
{
	string r = A;

	for (unsigned i = 0; i < r.length(); ++i) {
		r[i] = toupper(r[i]);
	}

	return r;
}

string subs(const string& s, const string& from, const string& to)
{
	string r = s;
	int sub = r.find(from);
	while (sub != string::npos) {
		r.erase(sub, from.length());
		r.insert(sub, to);
		sub = r.find(from);
	}
	return r;
}

bool is_glob(const char* str_begin, const char* str_end, const char* glob_begin, const char* glob_end)
{
	if (str_begin == str_end && glob_begin == glob_end)
		return true;
	if (str_begin != str_end && glob_begin != glob_end) {
		if (*glob_begin == '*')
			return is_glob(str_begin + 1, str_end, glob_begin, glob_end)
			       || is_glob(str_begin, str_end, glob_begin + 1, glob_end);
		if (*glob_begin == *str_begin)
			return is_glob(str_begin + 1, str_end, glob_begin + 1, glob_end);
	}
	return false;
}

bool is_glob(const string& str, const string& glob)
{
	return is_glob(str.c_str(), str.c_str() + str.length(), glob.c_str(), glob.c_str() + glob.length());
}

bool is_globlist(const string& file, const string& globlist)
{
	unsigned i = 0;
	if (globlist.length() == 0)
		return true;
	while (i < globlist.length()) {
		int end = globlist.find(':', i);
		if (end == string::npos) {
			string filter(globlist, i);
			i = globlist.size();
			if (is_glob(file, filter))
				return true;
		} else {
			string filter(globlist, i, end - i);
			if (is_glob(file, filter))
				return true;
			i = end + 1;
		}
	}
	return false;
}

string file_read(const string& file)
{
	struct stat st;

	const char* path = file_config_file_home(file.c_str());

	if (stat(path, &st) != 0) {
		target_err("Error opening the file '%s'.\n", path);
		return string("");
	}

	FILE* f = fopen(path, "rb");
	if (!f) {
		target_err("Error opening the file '%s'.\n", path);
		return string("");
	}

	char* ssc = (char*)operator new(st.st_size + 1);

	if (fread(ssc, st.st_size, 1, f) != 1) {
		operator delete(ssc);
		fclose(f);
		target_err("Error reading the file '%s'.\n", path);
		return string("");
	}

	fclose(f);
	ssc[st.st_size] = 0;

	string ss = ssc;
	operator delete(ssc);

	return ss;
}

