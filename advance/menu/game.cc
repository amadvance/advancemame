/*
 * This file is part of the AdvanceMAME project.
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
#include "emulator.h"
#include "readinfo.h"
#include "common.h"
#include "unzip.h"

#include <iostream>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

using namespace std;

// ------------------------------------------------------------------------
// game

string game_name_adjust(const string& name) {
	string r = name;
	for(unsigned i=0;i<r.length();++i)
		r[i] = tolower(r[i]);
	return r;
}

game::game() {
	flag = flag_working | flag_sound | flag_color;
	sizex = 0;
	sizey = 0;
	aspectx = 0;
	aspecty = 0;
	time = 0;
	size = 0;
	type = CATEGORY_UNDEFINED;
	group = CATEGORY_UNDEFINED;
	parent = 0;
	emu = 0;
	coin = 0;
}

game::game(const string& Aname) : name(Aname) {
	flag = flag_working | flag_sound | flag_color;
	sizex = 0;
	sizey = 0;
	aspectx = 0;
	aspecty = 0;
	time = 0;
	size = 0;
	type = CATEGORY_UNDEFINED;
	group = CATEGORY_UNDEFINED;
	parent = 0;
	emu = 0;
	coin = 0;
}

game::game(const game& A) :
	flag(A.flag),
	name(A.name), romof(A.romof),cloneof(A.cloneof),
	description(A.description), year(A.year),
	manufacturer(A.manufacturer), software_path(A.software_path),
	sizex(A.sizex), sizey(A.sizey),
	aspectx(A.aspectx), aspecty(A.aspecty),
	group(A.group), type(A.type), time(A.time),
	coin(A.coin),
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

game::~game() {
}

void game::auto_description_set(const std::string& A) const {
	if (!is_user_description_set()) {
		if (A.length() >= 4 && A[0]=='T' && A[1]=='h' && A[2]=='e' && A[3]==' ') {
			description = A.substr(4,A.length() - 4) + ", The";
		} else {
			description = A;
		}
	}
}

void game::rom_zip_set_insert(const string& Afile) const {
	rzs.insert( rzs.end(), string( Afile ) );
}

const game& game::clone_best_get() const {
	const game* r = this;
	for(pgame_container::const_iterator i = clone_bag_get().begin();i!=clone_bag_get().end();++i) {
		const game* rr = &(*i)->clone_best_get();
		if (game_by_play_less()(*r,*rr)) {
			r = rr;
		}
	}
	return *r;
}

unsigned game::coin_tree_get() const {
	unsigned r = coin_get();
	for(pgame_container::const_iterator i = clone_bag_get().begin();i!=clone_bag_get().end();++i) {
		r += (*i)->coin_tree_get();
	}
	return r;
}

unsigned game::time_tree_get() const {
	unsigned r = time_get();
	for(pgame_container::const_iterator i = clone_bag_get().begin();i!=clone_bag_get().end();++i) {
		r += (*i)->time_tree_get();
	}
	return r;
}

string game::name_without_emulator_get() const {
	int i = name_get().find('/');
	if (i == string::npos)
		return name_get();
	else
		return name_get().substr(i + 1);
}

string game::group_derived_get() const {
	if (group_get() != CATEGORY_UNDEFINED)
		return group_get();
	if (parent_get())
		return parent_get()->group_derived_get();
	return CATEGORY_UNDEFINED;
}

string game::type_derived_get() const {
	if (type_get() != CATEGORY_UNDEFINED)
		return type_get();
	if (parent_get())
		return parent_get()->type_derived_get();
	return CATEGORY_UNDEFINED;
}

const game& game::bios_get() const {
	const game* bios = this;
	while (bios->parent_get() && bios->software_get())
		bios = bios->parent_get();
	return *bios;
}

const game& game::root_get() const {
	const game* root = this;
	while (root->parent_get())
		root = root->parent_get();
	return *root;
}

bool game::preview_zip_set(const string& zip, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const {
	ZIP* d = openzip(cpath_export(slash_remove(zip)));
	if (!d)
		return false;

	string game_name = name_without_emulator_get();

	struct zipent* dd;
	while ((dd = readzip(d))!=0) {
		string file = dd->name;
		string ext = file_ext(file);
		string name = file_basename(file);
		if (name == game_name && ext.length() && (ext == ext0 || ext == ext1)) {
			string zipfile = slash_add(zip) + file;
			unsigned offset = dd->offset_lcl_hdr_frm_frst_disk;
			if (dd->compression_method == 0x0) {
				((*this).*preview_set)( resource(zipfile, offset, dd->uncompressed_size, true ) );
				closezip(d);
				return true;
			} else if (dd->compression_method == 0x8) {
				((*this).*preview_set)( resource(zipfile, offset, dd->compressed_size, dd->uncompressed_size, true ) );
				closezip(d);
				return true;
			}
		}
	}

	closezip(d);
	return false;
}

bool game::preview_dir_set(const string& dir, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const {
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d)
		return false;

	string game_name = name_without_emulator_get();

	struct dirent* dd;
	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		string ext = file_ext(file);
		string name = file_basename(file);
		if (name == game_name && ext.length() && (ext == ext0 || ext == ext1)) {
			((*this).*preview_set)( slash_add(dir) + file );
			closedir(d);
			return true;
		} else if (ext == ".zip") {
			if (preview_zip_set( slash_add(dir) + file, preview_set, ext0, ext1 )) {
				closedir(d);
				return true;
			}
		}
	}

	closedir(d);
	return false;
}

bool game::preview_list_set(const string& list, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) const {
	int i = 0;
	while (i<list.length()) {
		string dir = token_get(list,i,":");
		token_skip(list,i,":");
		if (preview_dir_set(dir,preview_set,ext0,ext1))
			return true;
	}
	return false;
}

bool game::preview_find_down(resource& path, const resource& (game::*preview_get)() const, const string& exclude) const {
	if ((this->*preview_get)().is_valid()) {
		path = (this->*preview_get)();
		return true;
	}

	for(pgame_container::const_iterator i=clone_bag_get().begin();i!=clone_bag_get().end();++i) {
		if ((*i)->name_get() != exclude) {
			if ((*i)->preview_find_down(path,preview_get,string())) {
				return true;
			}
		}
	}

	return false;
}

bool game::preview_find_up(resource& path, const resource& (game::*preview_get)() const, const string& exclude) const {
	if (preview_find_down(path,preview_get,exclude))
		return true;
	if (parent_get())
		if (parent_get()->preview_find_up(path,preview_get,name_get()))
			return true;
	return false;
}

bool game::preview_find(resource& path, const resource& (game::*preview_get)() const) const {
	if (software_get()) {
		if ((this->*preview_get)().is_valid()) {
			path = (this->*preview_get)();
			return true;
		}
	} else {
		if (preview_find_up(path,preview_get,string()))
			return true;
	}
	return false;
}

// ------------------------------------------------------------------------
// game_set

static const char* manufacturer_strip(const char* s) {
	static char buffer[1024];
	char* dst = buffer;
	// remove begin space
	while (*s && isspace(*s))
		++s;
	if (s[0] == '[') {
		++s;
		while (*s && *s!=']') {
			*dst++ = *s++;
		}
	} else {
		while (*s && *s!='(' && *s!='/' && *s!='+' && *s!='&') {
			*dst++ = *s++;
		}
	}
	// remove end space
	while (dst != buffer && isspace(dst[-1]))
		--dst;
	*dst = 0;
	return buffer;
}

bool game_set::load(FILE* f, const emulator* Aemu) {
	info_init();
	bool r = internal_load(f,Aemu);
	info_done();
	return r;
}

bool game_set::internal_load(FILE* f, const emulator* Aemu) {
	info_t token = info_token_get(f);
	while (token!=info_eof) {
		if (token != info_symbol) return false;
		bool isresource = strcmp(info_text_get(),"resource")==0;
		bool isgame = strcmp(info_text_get(),"game")==0 || strcmp(info_text_get(),"machine")==0;
		if (isgame || isresource) {
			if (info_token_get(f) != info_open) return false;
			game g;
			g.emulator_set(Aemu);
			g.resource_set(isresource);
			token = info_token_get(f);
			while (token != info_close) {
				if (token != info_symbol)
					return false;
				if (strcmp(info_text_get(),"name")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.name_set( Aemu->user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"description")==0) {
					if (info_token_get(f) != info_string) return false;
					g.auto_description_set( info_text_get() );
				} else if (strcmp(info_text_get(),"manufacturer")==0) {
					if (info_token_get(f) != info_string) return false;
					g.manufacturer_set( manufacturer_strip( info_text_get() ) );
				} else if (strcmp(info_text_get(),"year")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.year_set( info_text_get() );
				} else if (strcmp(info_text_get(),"cloneof")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.cloneof_set( Aemu->user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"romof")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.romof_set( Aemu->user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"driver")==0) {
					if (info_token_get(f) != info_open)  return false;
					token = info_token_get(f);
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(),"status")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.working_set( strcmp( info_text_get(), "preliminary") != 0 );
						} else if (strcmp(info_text_get(),"color")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.color_set( strcmp( info_text_get(), "preliminary") != 0 );
						} else if (strcmp(info_text_get(),"sound")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.sound_set( strcmp( info_text_get(), "preliminary") != 0 );
						} else {
							if (info_skip_value(f) == info_error) return false;
						}
						token = info_token_get(f);
					}
				} else if (strcmp(info_text_get(),"video")==0)  {
					if (info_token_get(f) != info_open)  return false;
					token = info_token_get(f);
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(),"screen")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.vector_set( strcmp( info_text_get(),"vector") == 0 );
						} else if (strcmp(info_text_get(),"orientation")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.vertical_set(strcmp(info_text_get(),"vertical")==0);
						} else if (strcmp(info_text_get(),"x")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.sizex_set( atoi(info_text_get()) );
						} else if (strcmp(info_text_get(),"y")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.sizey_set( atoi(info_text_get()) );
						} else if (strcmp(info_text_get(),"aspectx")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.aspectx_set( atoi(info_text_get()) );
						} else if (strcmp(info_text_get(),"aspecty")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.aspecty_set( atoi(info_text_get()) );
						} else {
							if (info_skip_value(f) == info_error) return false;
						}
						token = info_token_get(f);
					}
				} else if (strcmp(info_text_get(),"rom")==0) {
					unsigned size = 0;
					bool merge = false;
					if (info_token_get(f) != info_open) return false;
					token = info_token_get(f);
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(),"size")==0) {
							if (info_token_get(f) != info_symbol) return false;
							size = atoi( info_text_get() );
						} else if (strcmp(info_text_get(),"merge")==0) {
							if (info_token_get(f) != info_symbol) return false;
							merge = true;
						} else {
							if (info_skip_value(f) == info_error) return false;
						}
						token = info_token_get(f);
					}
					if (!merge)
						g.size_set( g.size_get() + size );
				} else if (strcmp(info_text_get(),"device")==0) {
					machinedevice dev;
					if (info_token_get(f) != info_open) return false;
					token = info_token_get(f);
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(),"ext")==0) {
							if (info_token_get(f) != info_string) return false;
							dev.ext_bag.insert(dev.ext_bag.end(), string( info_text_get() ) );
						} else if (strcmp(info_text_get(),"name")==0) {
							if (info_token_get(f) != info_string) return false;
							dev.name = string(info_text_get());
						} else {
							if (info_skip_value(f) == info_error) return false;
						}
						token = info_token_get(f);
					}
					g.machinedevice_bag_get().insert(g.machinedevice_bag_get().end(),dev);
				} else {
					if (info_skip_value(f) == info_error) return false;
				}
				token = info_token_get(f);
			}
			insert( g );
		} else {
			if (info_skip_value(f) == info_error)
				return false;
		}
		token = info_token_get(f);
	}

	return true;
}

void game_set::sync_relationships() {
	for(iterator i=begin();i!=end();++i) {
		// erase the clone list
		i->clone_bag_erase();

		// test cloneof and compute the parent
		if (i->cloneof_get().length() != 0) {
			iterator j = find( game(i->cloneof_get()) );
			if (j == end()) {
				target_err("Missing definition of cloneof '%s' for game '%s'.\n", i->cloneof_get().c_str(), i->name_get().c_str());
				(const_cast<game*>((&*i)))->cloneof_set(string());
				i->parent_set( 0 );
			} else {
				i->parent_set( &*j );

				while (j != end()) {
					if (&*j == &*i) {
						target_err("Circular cloneof reference for game '%s'.\n", i->name_get().c_str());
						(const_cast<game*>((&*i)))->cloneof_set(string());
						i->parent_set( 0 );
						break;
					}
					j = find( j->cloneof_get() );
				}
			}
		} else {
			i->parent_set( 0 );
		}

		// test romof
		if (i->romof_get().length() != 0) {
			game_set::const_iterator j = find( i->romof_get() );
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
					j = find( j->romof_get() );
				}
			}
		}
	}

	// compute the clone list for every game
	for(iterator i=begin();i!=end();++i) {
		const game* j = i->parent_get();
		while (j!=0) {
			j->clone_bag_get().insert(j->clone_bag_get().end(),&*i);
			j = j->parent_get();
		}
	}
}

bool game_set::is_tree_rom_of_present(const string& name, merge_t type) const {
	switch (type) {
		case merge_differential : {
			const_iterator i = find( game( name ) );
			while (i!=end()) {
				if (!i->is_present())
					return false;
				if (i->romof_get().length()==0)
					return true;
				i = find( game( i->romof_get() ) );
			}
			return false;
		}
		case merge_any : {
			const_iterator i = find( game( name ) );
			while (i!=end()) {
				if (i->is_present())
					return true;
				if (i->romof_get().length()==0)
					return false;
				i = find( game( i->romof_get() ) );
			}
			return false;
		}
		case merge_no : {
			const_iterator i = find( game( name ) );
			if (i!=end())
				return i->is_present();
			return false;
		}
		case merge_parent : {
			const_iterator i = root_clone_of_get(name);
			if (i!=end())
				return i->is_present();
			return false;
		}
		case merge_disable :
			return true;
	}
	return false;
}

bool game_set::is_game_tag(const string& name, const string& tag) const {
	game_set::const_iterator i = find( game( name ) );
	while (i!=end()) {
		if (i->name_get().length() >= 1+tag.length()
			&& i->name_get()[i->name_get().length() - tag.length() - 1]=='/'
			&& i->name_get().substr(i->name_get().length() - tag.length(),tag.length()).compare(tag) == 0)
			return true;
		i = find( game( i->romof_get() ) );
	}
	return false;
}

game_set::const_iterator game_set::root_rom_of_get(const string& name) const {
	game_set::const_iterator i = find( game( name ) );
	while (i != end()) {
		if (i->romof_get().length() == 0)
			return i;
		i = find( i->romof_get() );
	}
	return i;
}

game_set::const_iterator game_set::root_clone_of_get(const string& name) const {
	game_set::const_iterator i = find( game( name ) );
	while (i != end()) {
		if (i->cloneof_get().length() == 0)
			return i;
		i = find( i->cloneof_get() );
	}
	return i;
}

bool game_set::is_game_clone_of(const string& name_son, const string& name_parent) const {
	game_set::const_iterator i = find( game( name_son ) );
	while (i!=end()) {
		if (i->name_get() == name_parent)
			return true;
		if (i->cloneof_get().length() == 0)
			return false;
		i = find( game( i->cloneof_get() ) );
	}
	return false;
}

bool game_set::is_game_rom_of(const string& name_son, const string& name_parent) const {
	game_set::const_iterator i = find( game( name_son ) );
	while (i!=end()) {
		if (i->name_get() == name_parent)
			return true;
		if (i->romof_get().length() == 0)
			return false;
		i = find( game( i->romof_get() ) );
	}
	return false;
}

bool game_set::preview_zip_set(const string& zip, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) {
	bool almost_one = false;
	ZIP* d = openzip(cpath_export(slash_remove(zip)));
	if (!d)
		return almost_one;

	struct zipent* dd;
	while ((dd = readzip(d))!=0) {
		string file = dd->name;
		string ext = file_ext(file);
		if (ext.length() && (ext == ext0 || ext == ext1)) {
			string name = emulator_name + "/" + file_basename(file);
			const_iterator j = find( name );
			if (j!=end()) {
				string zipfile = slash_add(zip) + file;
				unsigned offset = dd->offset_lcl_hdr_frm_frst_disk;
				if (dd->compression_method == 0x0) {
					((*j).*preview_set)( resource(zipfile, offset, dd->uncompressed_size, true ) );
					almost_one = true;
				} else if (dd->compression_method == 0x8) {
					((*j).*preview_set)( resource(zipfile, offset, dd->compressed_size, dd->uncompressed_size, true ) );
					almost_one = true;
				}
			}
		}
	}

	closezip(d);
	return almost_one;
}

bool game_set::preview_dir_set(const string& dir, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) {
	bool almost_one = false;
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d)
		return almost_one;

	struct dirent* dd;
	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		string ext = file_ext(file);
		if (ext.length() && (ext == ext0 || ext == ext1)) {
			string name = emulator_name + "/" + file_basename(file);
			const_iterator j = find( name );
			if (j!=end()) {
				((*j).*preview_set)( slash_add(dir) + file );
				almost_one = true;
			}
		} else if (ext == ".zip") {
			if (preview_zip_set( slash_add(dir) + file, emulator_name, preview_set, ext0, ext1 ))
				almost_one = true;
		}
	}

	closedir(d);
	return almost_one;
}

bool game_set::preview_list_set(const string& list, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) {
	bool almost_one = false;
	int i = 0;

	while (i<list.length()) {
		string dir = token_get(list,i,":");
		if (preview_dir_set(dir,emulator_name,preview_set,ext0,ext1))
			almost_one = true;
		token_skip(list,i,":");
	}

	return almost_one;
}

bool game_set::preview_software_dir_set(const string& dir, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) {
	bool almost_one = false;
	DIR* d = opendir(cpath_export(slash_remove(dir)));
	if (!d)
		return almost_one;

	struct dirent* dd;
	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		string path = slash_add(dir) + file;

		// check for directory
		struct stat st;
		if (stat(cpath_export(path),&st)!=0)
			continue;
		if (!S_ISDIR(st.st_mode))
			continue;

		// check if is a bios
		string name = emulator_name + "/" + file;
		const_iterator j = find( name );
		if (j!=end() && !j->software_get()) {
			// search in the directory
			if (preview_dir_set(path,emulator_name + "/" + file,preview_set,ext0,ext1))
				almost_one = true;
		}
	}

	closedir(d);
	return almost_one;
}

bool game_set::preview_software_list_set(const string& list, const string& emulator_name, void (game::*preview_set)(const resource& s) const, const string& ext0, const string& ext1) {
	bool almost_one = false;
	int i = 0;
	while (i<list.length()) {
		string dir = token_get(list,i,":");
		token_skip(list,i,":");
		if (preview_software_dir_set(dir,emulator_name,preview_set,ext0,ext1))
			almost_one = true;
	}
	return almost_one;
}

void game_set::import_nms(const string& file, const string& emulator, void (game::*set)(const string& s) const) {
	int j = 0;

	string ss = file_read( file );

	bool in = false;
	while (j < ss.length()) {
		string s = token_get(ss,j,"\r\n");
		token_skip(ss,j,"\r\n");

		int i = 0;
		token_skip(s, i," \t");

		string desc = token_get(s, i, '|');
		token_skip(s, i, "|");
		string tag = token_get(s, i, "");

		if (desc.length() && tag.length()) {
			string name = emulator + "/" + tag;
			game_set::iterator k = find( game( name ) );
			if (k!=end()) {
				((*k).*set)(desc);
			}
		}
	}
}

// -------------------------------------------------------------------------
// Sort category

string sort_item_root_name(const game& g) {
	const game& root = g.root_get();

	string pre = root.description_get().substr(0,1);

	if (pre.length()) {
		if (!isalpha(pre[0]))
			pre = "0-9";
		else
			pre = toupper(pre[0]);
	}

	return pre;
}

string sort_item_name(const game& g) {
	string pre = g.description_get().substr(0,1);

	if (pre.length()) {
		if (isdigit(pre[0]))
			pre = "0-9";
		else
			pre = toupper(pre[0]);
	}

	return pre;
}

string sort_item_manufacturer(const game& g) {
	return g.manufacturer_get();
}

string sort_item_year(const game& g) {
	if (!g.year_get().length())
		return "unknown";
	else
		return g.year_get();
}

string sort_item_time(const game& g) {
	(void)g;
	return string();
}

string sort_item_coin(const game& g) {
	(void)g;
	return string();
}

string sort_item_group(const game& g) {
	return g.group_get();
}

string sort_item_type(const game& g) {
	return g.type_get();
}

string sort_item_size(const game& g) {
	(void)g;
	return string();
}

string sort_item_res(const game& g) {
	char buffer[32];
	sprintf(buffer,"%dx%d", g.sizex_get(), g.sizey_get());
	return buffer;
}

