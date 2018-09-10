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
#include "emulator.h"
#include "common.h"

#include "advance.h"

#include <iostream>
#include <sstream>

using namespace std;

// ------------------------------------------------------------------------
// game

game::game()
{
	flag = 0;
	play = play_perfect;
	play_best = play_preliminary;
	sizex = 0;
	sizey = 0;
	aspectx = 0;
	aspecty = 0;
	time = 0;
	size = 0;
	type = 0;
	group = 0;
	parent = 0;
	emu = 0;
	session = 0;
}

game::game(const string& Aname) : name(Aname)
{
	flag = 0;
	play = play_perfect;
	play_best = play_preliminary;
	sizex = 0;
	sizey = 0;
	aspectx = 0;
	aspecty = 0;
	time = 0;
	size = 0;
	type = 0;
	group = 0;
	parent = 0;
	emu = 0;
	session = 0;
}

game::game(const game& A) :
	flag(A.flag),
	play(A.play), play_best(A.play_best),
	name(A.name), romof(A.romof), cloneof(A.cloneof),
	description(A.description), info(A.info), year(A.year),
	manufacturer(A.manufacturer), software_path(A.software_path),
	sizex(A.sizex), sizey(A.sizey),
	aspectx(A.aspectx), aspecty(A.aspecty),
	group(A.group), type(A.type), time(A.time),
	session(A.session),
	size(A.size),
	rzs(A.rzs),
	clone_bag(A.clone_bag),
	parent(A.parent),
	machinedevice_bag(A.machinedevice_bag),
	snap_path(A.snap_path), clip_path(A.clip_path), flyer_path(A.flyer_path), cabinet_path(A.cabinet_path),
	sound_path(A.sound_path), icon_path(A.icon_path), marquee_path(A.marquee_path),
	emu(A.emu)
{
}

game::~game()
{
}

void game::name_set(const std::string& A)
{
	name = A;
}

void game::auto_description_set(const std::string& A) const
{
	if (!is_user_description_set()) {
		description = A;
	}
}

std::string game::description_tree_get() const
{
	// remove () and []
	return strip_comment(description_get());
}

void game::manufacturer_set(const std::string& s)
{
	// clear previous value
	manufacturer.erase();

	// allocate the whole size to speedup
	manufacturer.reserve(s.length());

	// strip unused information
	unsigned i = 0;
	unsigned l = s.length();

	// skip space at begin
	while (i < s.length() && isspace(s[i]))
		++i;

	// skip space at end
	while (i < l && isspace(s[l - 1]))
		--l;

	if (s[i] == '[') {
		++i;
		while (i < l && s[i] != ']') {
			manufacturer += s[i];
			++i;
		}
	} else {
		while (i < l && s[i] != '(' && s[i] != '/' && s[i] != '+' && s[i] != '&') {
			manufacturer += s[i];
			++i;
		}
	}
}

void game::rom_zip_set_insert(const string& Afile) const
{
	rzs.insert(rzs.end(), string(Afile));
}

const game& game::clone_best_get() const
{
	const game* r = this;
	bool is_software = software_get();
	for (pgame_container::const_iterator i = clone_bag_get().begin(); i != clone_bag_get().end(); ++i) {
		// consider only clone of the same kind machine/software
		if (is_software == (*i)->software_get()) {
			const game* rr = &(*i)->clone_best_get();
			if (game_by_play_less()(*r, *rr)) {
				r = rr;
			}
		}
	}
	return *r;
}

unsigned game::session_tree_get() const
{
	unsigned r = session_get();
	for (pgame_container::const_iterator i = clone_bag_get().begin(); i != clone_bag_get().end(); ++i) {
		r += (*i)->session_tree_get();
	}
	return r;
}

unsigned game::time_tree_get() const
{
	unsigned r = time_get();
	for (pgame_container::const_iterator i = clone_bag_get().begin(); i != clone_bag_get().end(); ++i) {
		if (!(*i)->software_get())
			r += (*i)->time_tree_get();
	}
	return r;
}

string game::name_without_emulator_get() const
{
	int i = name_get().rfind('/');
	if (i == string::npos)
		return name_get();
	else
		return name_get().substr(i + 1);
}

void game::auto_group_set(const category* A) const
{
	if (!is_user_group_set())
		group = A;
}

void game::user_group_set(const category* A) const
{
	if (!A->undefined_get())
		flag |= flag_user_group_set;
	group = A;
}

void game::auto_type_set(const category* A) const
{
	if (!is_user_type_set())
		type = A;
}

void game::user_type_set(const category* A) const
{
	if (!A->undefined_get())
		flag |= flag_user_type_set;
	type = A;
}

const category* game::group_derived_get() const
{
	if (!group_get()->undefined_get())
		return group_get();
	if (parent_get())
		return parent_get()->group_derived_get();
	return group_get();
}

const category* game::type_derived_get() const
{
	if (!type_get()->undefined_get())
		return type_get();
	if (parent_get())
		return parent_get()->type_derived_get();
	return type_get();
}

const game& game::bios_get() const
{
	const game* bios = this;
	while (bios->parent_get() && bios->software_get())
		bios = bios->parent_get();
	return *bios;
}

const game& game::root_get() const
{
	const game* root = this;
	while (root->parent_get())
		root = root->parent_get();
	return *root;
}

bool game::preview_zip_set(const string& zip, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const
{
	adv_zip* d = zip_open(cpath_export(slash_remove(zip)));
	if (!d) {
		log_std(("menu:game: failed opening %s\n", cpath_export(zip)));
		return false;
	}

	string game_name = name_without_emulator_get();

	adv_zipent* dd;
	while ((dd = zip_read(d)) != 0) {
		string file = file_file(dd->name);
		string ext = file_ext(file);
		string name = file_basename(file);
		if (name == game_name && ext.length() && (ext == ext0 || ext == ext1)) {
			string zipfile = slash_add(zip) + file;
			off_t offset = dd->offset_lcl_hdr_frm_frst_disk;
			if (dd->compression_method == 0x0) {
				((*this).*preview_set)(resource(zipfile, offset, dd->uncompressed_size, true));
				zip_close(d);
				return true;
			} else if (dd->compression_method == 0x8) {
				((*this).*preview_set)(resource(zipfile, offset, dd->compressed_size, dd->uncompressed_size, true));
				zip_close(d);
				return true;
			}
		}
	}

	zip_close(d);
	return false;
}

bool game::preview_dir_set(const string& dir, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const
{
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d) {
		log_std(("menu:game: failed opening %s\n", cpath_export(dir)));
		return false;
	}

	string game_name = name_without_emulator_get();

	struct dirent* dd;
	while ((dd = readdir(d)) != 0) {
		string file = file_import(dd->d_name);
		string ext = file_ext(file);
		string name = file_basename(file);
		if (name == game_name && ext.length() && (ext == ext0 || ext == ext1)) {
			((*this).*preview_set)(slash_add(dir) + file);
			closedir(d);
			return true;
		} else if (ext == ".zip") {
			if (preview_zip_set(slash_add(dir) + file, preview_set, ext0, ext1)) {
				closedir(d);
				return true;
			}
		}
	}

	closedir(d);
	return false;
}

bool game::preview_list_set(const string& list, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const
{
	int i = 0;
	while (i < list.length()) {
		string dir = token_get(list, i, ":");
		token_skip(list, i, ":");
		if (preview_dir_set(dir, preview_set, ext0, ext1))
			return true;
	}
	return false;
}

bool game::preview_software_list_set(const string& list, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const
{
	if (!software_get())
		return false;

	string bios = bios_get().name_without_emulator_get();

	int i = 0;
	while (i < list.length()) {
		string dir = token_get(list, i, ":");
		token_skip(list, i, ":");
		if (preview_dir_set(dir + "/" + bios, preview_set, ext0, ext1))
			return true;
	}
	return false;
}


bool game::preview_find_down(resource& path, const resource& (game::*preview_get)() const, const string& exclude) const
{
	if ((this->*preview_get)().is_valid()) {
		path = (this->*preview_get)();
		return true;
	}

	for (pgame_container::const_iterator i = clone_bag_get().begin(); i != clone_bag_get().end(); ++i) {
		if (!(*i)->software_get() && (*i)->name_get() != exclude) {
			if ((*i)->preview_find_down(path, preview_get, string())) {
				return true;
			}
		}
	}

	return false;
}

bool game::preview_find_up(resource& path, const resource& (game::*preview_get)() const, const string& exclude) const
{
	if (preview_find_down(path, preview_get, exclude))
		return true;
	if (parent_get())
		if (parent_get()->preview_find_up(path, preview_get, name_get()))
			return true;
	return false;
}

bool game::preview_find(resource& path, const resource& (game::*preview_get)() const) const
{
	if (software_get()) {
		if ((this->*preview_get)().is_valid()) {
			path = (this->*preview_get)();
			return true;
		}
	} else {
		if (preview_find_up(path, preview_get, string()))
			return true;
	}
	return false;
}

// ------------------------------------------------------------------------
// game_set

void game_set::cache(merge_t merge)
{
	dupe_set dar;

	for (iterator i = begin(); i != end(); ++i) {
		// erase the clone list
		i->clone_bag_erase();

		// test cloneof and compute the parent
		if (i->cloneof_get().length() != 0) {
			iterator j = find(game(i->cloneof_get()));
			if (j == end()) {
				target_err("Missing definition of cloneof '%s' for game '%s'.\n", i->cloneof_get().c_str(), i->name_get().c_str());
				(const_cast<game*>((&*i)))->cloneof_set(string());
				i->parent_set(0);
			} else {
				i->parent_set(&*j);

				while (j != end()) {
					if (&*j == &*i) {
						target_err("Circular cloneof reference for game '%s'.\n", i->name_get().c_str());
						(const_cast<game*>((&*i)))->cloneof_set(string());
						i->parent_set(0);
						break;
					}
					j = find(j->cloneof_get());
				}
			}
		} else {
			i->parent_set(0);
		}

		// test romof
		if (i->romof_get().length() != 0) {
			game_set::const_iterator j = find(i->romof_get());
			if (j == end()) {
				target_err("Missing definition of romof '%s' for game '%s'.\n", i->romof_get().c_str(), i->name_get().c_str());
				(const_cast<game*>((&*i)))->romof_set(string());
			} else {
				while (j != end()) {
					if (&*j == &*i) {
						target_err("Circular romof reference for game '%s'.\n", i->name_get().c_str());
						(const_cast<game*>((&*i)))->romof_set(string());
						break;
					}
					j = find(j->romof_get());
				}
			}
		}
	}

	// compute the clone list for every game
	for (iterator i = begin(); i != end(); ++i) {
		const game* j = i->parent_get();
		while (j != 0) {
			j->clone_bag_get().insert(j->clone_bag_get().end(), &*i);
			j = j->parent_get();
		}
	}

	// compute the derived play_best
	for (iterator i = begin(); i != end(); ++i) {
		i->play_best_set(i->clone_best_get().play_get());
	}

	// compute the derived tree_present
	for (iterator i = begin(); i != end(); ++i) {
		bool present = is_tree_rom_of_present(i->name_get(), merge);
		i->flag_set(present, game::flag_tree_present);
	}

	// specific emulator cache and fill up dup cache
	for (iterator i = begin(); i != end(); ++i) {
		i->emulator_get()->cache(*this, *i);

		// skip prelimiary
		if (i->play_best_get() == play_preliminary)
			continue;

		std::string stripped_name = file_file(i->name_get());

		pair<dupe_set::const_iterator, bool> j = dar.insert(dupe(stripped_name, 0));
		j.first->count_set(j.first->count_get() + 1);
	}

	// set dupe info
	for (iterator i = begin(); i != end(); ++i) {
		std::string stripped_name = file_file(i->name_get());

		dupe_set::const_iterator j = dar.find(dupe(stripped_name, 0));

		i->flag_set(j != dar.end() && (i->play_best_get() == play_preliminary || j->count_get() > 1), game::flag_duplicate);
	}
}

bool game_set::is_tree_rom_of_present(const string& name, merge_t type) const
{
	switch (type) {
	case merge_differential: {
		const_iterator i = find(game(name));
		while (i != end()) {
			if (!i->present_get())
				return false;
			if (i->romof_get().length() == 0)
				return true;
			i = find(game(i->romof_get()));
		}
		return false;
	}
	case merge_any: {
		const_iterator i = find(game(name));
		while (i != end()) {
			if (i->present_get())
				return true;
			if (i->romof_get().length() == 0)
				return false;
			i = find(game(i->romof_get()));
		}
		return false;
	}
	case merge_no: {
		const_iterator i = find(game(name));
		if (i != end())
			return i->present_get();
		return false;
	}
	case merge_parent: {
		const_iterator i = root_clone_of_get(name);
		if (i != end())
			return i->present_get();
		return false;
	}
	case merge_disable:
		return true;
	}
	return false;
}

bool game_set::is_game_tag(const string& name, const string& tag) const
{
	game_set::const_iterator i = find(game(name));
	while (i != end()) {
		if (i->name_get().length() >= 1 + tag.length()
			&& i->name_get()[i->name_get().length() - tag.length() - 1] == '/'
			&& i->name_get().substr(i->name_get().length() - tag.length(), tag.length()).compare(tag) == 0)
			return true;
		i = find(game(i->romof_get()));
	}
	return false;
}

game_set::const_iterator game_set::root_rom_of_get(const string& name) const
{
	game_set::const_iterator i = find(game(name));
	while (i != end()) {
		if (i->romof_get().length() == 0)
			return i;
		i = find(i->romof_get());
	}
	return i;
}

game_set::const_iterator game_set::root_clone_of_get(const string& name) const
{
	game_set::const_iterator i = find(game(name));
	while (i != end()) {
		if (i->cloneof_get().length() == 0)
			return i;
		i = find(i->cloneof_get());
	}
	return i;
}

bool game_set::is_game_clone_of(const string& name_son, const string& name_parent) const
{
	game_set::const_iterator i = find(game(name_son));
	while (i != end()) {
		if (i->name_get() == name_parent)
			return true;
		if (i->cloneof_get().length() == 0)
			return false;
		i = find(game(i->cloneof_get()));
	}
	return false;
}

bool game_set::is_game_rom_of(const string& name_son, const string& name_parent) const
{
	game_set::const_iterator i = find(game(name_son));
	while (i != end()) {
		if (i->name_get() == name_parent)
			return true;
		if (i->romof_get().length() == 0)
			return false;
		i = find(game(i->romof_get()));
	}
	return false;
}

bool game_set::preview_zip_set(const string& zip, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1)
{
	bool almost_one = false;
	adv_zip* d = zip_open(cpath_export(slash_remove(zip)));
	if (!d) {
		log_std(("menu:game: failed opening %s\n", cpath_export(zip)));
		return almost_one;
	}

	adv_zipent* dd;
	while ((dd = zip_read(d)) != 0) {
		string file = file_file(dd->name);
		string ext = file_ext(file);
		if (ext.length() && (ext == ext0 || ext == ext1)) {
			string name = emulator_name + "/" + file_basename(file);
			const_iterator j = find(name);
			if (j != end()) {
				string zipfile = slash_add(zip) + file;
				off_t offset = dd->offset_lcl_hdr_frm_frst_disk;
				if (dd->compression_method == 0x0) {
					((*j).*preview_set)(resource(zipfile, offset, dd->uncompressed_size, true));
					almost_one = true;
				} else if (dd->compression_method == 0x8) {
					((*j).*preview_set)(resource(zipfile, offset, dd->compressed_size, dd->uncompressed_size, true));
					almost_one = true;
				}
			}
		}
	}

	zip_close(d);
	return almost_one;
}

bool game_set::preview_dir_set(const string& dir, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1)
{
	bool almost_one = false;
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d) {
		log_std(("menu:game: failed opening %s\n", cpath_export(dir)));
		return almost_one;
	}

	struct dirent* dd;
	while ((dd = readdir(d)) != 0) {
		string file = file_import(dd->d_name);
		string ext = file_ext(file);
		if (ext.length() && (ext == ext0 || ext == ext1)) {
			string name = emulator_name + "/" + file_basename(file);
			const_iterator j = find(name);
			if (j != end()) {
				((*j).*preview_set)(slash_add(dir) + file);
				almost_one = true;
			}
		} else if (ext == ".zip") {
			if (preview_zip_set(slash_add(dir) + file, emulator_name, preview_set, ext0, ext1))
				almost_one = true;
		}
	}

	closedir(d);
	return almost_one;
}

bool game_set::preview_list_set(const string& list, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1)
{
	bool almost_one = false;
	int i = 0;

	while (i < list.length()) {
		string dir = token_get(list, i, ":");
		if (preview_dir_set(dir, emulator_name, preview_set, ext0, ext1))
			almost_one = true;
		token_skip(list, i, ":");
	}

	return almost_one;
}

bool game_set::preview_software_dir_set(const string& dir, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1)
{
	bool almost_one = false;
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d) {
		log_std(("menu:game: failed opening %s\n", cpath_export(dir)));
		return almost_one;
	}

	struct dirent* dd;
	while ((dd = readdir(d)) != 0) {
		string file = file_import(dd->d_name);
		string path = slash_add(dir) + file;

		// check for directory
		struct stat st;
		if (stat(cpath_export(path), &st) != 0)
			continue;
		if (!S_ISDIR(st.st_mode))
			continue;

		// check if is a bios
		string name = emulator_name + "/" + file;
		const_iterator j = find(name);
		if (j != end() && !j->software_get()) {
			// search in the directory
			if (preview_dir_set(path, emulator_name + "/" + file, preview_set, ext0, ext1))
				almost_one = true;
		}
	}

	closedir(d);
	return almost_one;
}

bool game_set::preview_software_list_set(const string& list, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1)
{
	bool almost_one = false;
	int i = 0;
	while (i < list.length()) {
		string dir = token_get(list, i, ":");
		token_skip(list, i, ":");
		if (preview_software_dir_set(dir, emulator_name, preview_set, ext0, ext1))
			almost_one = true;
	}
	return almost_one;
}

// -------------------------------------------------------------------------
// Sort category

bool pgame_by_emulator_less(const game* A, const game* B)
{
	assert(A->emulator_get() != 0 && B->emulator_get() != 0);
	return A->emulator_get()->user_name_get() > B->emulator_get()->user_name_get();
}

string sort_item_root_name(const game& g)
{
	const game& root = g.root_get();

	string pre = root.description_get().substr(0, 1);

	if (pre.length()) {
		if (!isalpha(pre[0]))
			pre = "0-9";
		else
			pre = toupper(pre[0]);
	}

	return pre;
}

string sort_item_name(const game& g)
{
	string pre = g.description_get().substr(0, 1);

	if (pre.length()) {
		if (isdigit(pre[0]))
			pre = "0-9";
		else
			pre = toupper(pre[0]);
	}

	return pre;
}

string sort_item_manufacturer(const game& g)
{
	if (!g.manufacturer_get().length()
		|| g.manufacturer_get()[0] == '?')
		return "<unknown>";
	else
		return g.manufacturer_get();
}

string sort_item_year(const game& g)
{
	if (!g.year_get().length())
		return "<unknown>";
	else
		return g.year_get();
}

string sort_item_time(const game& g)
{
	return g.emulator_get()->user_name_get();
}

string sort_item_smart_time(const game& g)
{
	return sort_item_root_name(g);
}

string sort_item_session(const game& g)
{
	(void)g;
	return string();
}

string sort_item_group(const game& g)
{
	return g.group_get()->name_get();
}

string sort_item_type(const game& g)
{
	return g.type_get()->name_get();
}

string sort_item_size(const game& g)
{
	(void)g;
	return string();
}

string sort_item_res(const game& g)
{
	if (g.sizex_get() && g.sizey_get()) {
		ostringstream s;
		s << g.sizex_get() << "x" << g.sizey_get();
		return s.str();
	} else {
		return "<unknown>";
	}
}

string sort_item_info(const game& g)
{
	if (g.info_get().length())
		return g.info_get();
	else
		return "<undefined>";
}

string sort_item_timepersession(const game& g)
{
	(void)g;
	return string();
}

string sort_item_emulator(const game& g)
{
	return g.emulator_get()->user_name_get();
}

