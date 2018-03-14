/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#ifndef __GAME_H
#define __GAME_H

#include "common.h"
#include "resource.h"

class emulator;
class category;

#include <set>
#include <list>
#include <vector>
#include <string>

#include <stdio.h>

enum merge_t {
	merge_no, // All required roms in every set
	merge_differential, // Only the unique roms in every set
	merge_parent, // All roms in the parent set
	merge_any, // Any merge allowed
	merge_disable // Check disabled
};

enum difficulty_t {
	difficulty_none,
	difficulty_easiest,
	difficulty_easy,
	difficulty_medium,
	difficulty_hard,
	difficulty_hardest
};

// ------------------------------------------------------------------------
// Device

typedef std::list<std::string> machinedevice_ext_container;

struct machinedevice {
	machinedevice_ext_container ext_bag; ///< List of extension of the device.
	std::string name; ///< Name of the device option.
};

typedef std::list<machinedevice> machinedevice_container;

// ------------------------------------------------------------------------
// Game

std::string game_name_adjust(const std::string& name);

enum play_t {
	play_perfect, ///< Perfect
	play_imperfect, ///< Minor problems
	play_preliminary ///< Not usable.
};

class game;

typedef std::list<const game*> pgame_container;

class game {
	static const unsigned flag_session_set = 0x1;
	static const unsigned flag_time_set = 0x2;
	static const unsigned flag_user_description_set = 0x4;
	static const unsigned flag_user_type_set = 0x8;
	static const unsigned flag_user_group_set = 0x10;
	static const unsigned flag_software = 0x20;
	static const unsigned flag_tree_present = 0x40;
	static const unsigned flag_duplicate = 0x80;
	static const unsigned flag_filled = 0x100;

	friend class game_set;

	mutable unsigned flag;

	play_t play; /// Playlability of the game
	mutable play_t play_best; /// Playlability of the best clone or the game. It's a derived value.

	// game information
	std::string name;
	std::string romof;
	std::string cloneof;
	std::string sampleof;
	mutable std::string description;
	mutable std::string info;
	std::string year;
	std::string manufacturer;
	mutable std::string software_path;
	unsigned sizex;
	unsigned sizey;
	unsigned aspectx;
	unsigned aspecty;

	mutable const category* group;
	mutable const category* type;
	mutable unsigned time;

	mutable int session;

	mutable unsigned size; // cached size in bytes of the roms

	mutable path_container rzs; // set of rom zip for the game

	mutable pgame_container clone_bag; // clones
	mutable const game* parent; // parent

	mutable machinedevice_container machinedevice_bag; //< Set of devices supported (MESS)

	// path of available images, =="" if none
	mutable resource snap_path;
	mutable resource clip_path;
	mutable resource flyer_path;
	mutable resource cabinet_path;
	mutable resource sound_path;
	mutable resource icon_path;
	mutable resource marquee_path;
	mutable resource title_path;

	emulator* emu;

	bool preview_find_down(resource& path, const resource& (game::*preview_get)() const, const std::string& exclude) const;
	bool preview_find_up(resource& path, const resource& (game::*preview_get)() const, const std::string& exclude) const;

public:
	game();
	game(const std::string& Aname);
	game(const game&);
	~game();

	static const unsigned flag_first = 0x10000;

	void flag_set(bool value, unsigned mask) const
	{
		if (value)
			flag |= mask;
		else
			flag &= ~mask;
	}

	bool flag_get(unsigned mask) const
	{
		return (flag & mask) != 0;
	}

	void name_set(const std::string& A);
	const std::string& name_get() const { return name; }
	void cloneof_set(const std::string& A) { cloneof = A; }
	const std::string& cloneof_get() const { return cloneof; }
	void romof_set(const std::string& A) { romof = A; }
	const std::string& romof_get() const { return romof; }

	void auto_description_set(const std::string& A) const;
	bool is_user_description_set() const { return flag_get(flag_user_description_set); }
	void user_description_set(const std::string& A) const { flag |= flag_user_description_set; description = A; }
	const std::string& description_get() const { return description; }
	std::string description_tree_get() const;

	void auto_info_set(const std::string& A) const { info = A; }
	const std::string& info_get() const { return info; }

	bool is_user_group_set() const { return flag_get(flag_user_group_set); }
	void auto_group_set(const category* A) const;
	void user_group_set(const category* A) const;
	const category* group_get() const { return group; }

	bool is_user_type_set() const { return flag_get(flag_user_type_set); }
	void auto_type_set(const category* A) const;
	void user_type_set(const category* A) const;
	const category* type_get() const { return type; }

	void play_set(play_t A) { play = A; }
	play_t play_get() const { return play; }
	void play_best_set(play_t A) const { play_best = A; }
	play_t play_best_get() const { return play_best; }
	void year_set(const std::string& A) { year = A; }
	const std::string& year_get() const { return year; }
	void manufacturer_set(const std::string& A);
	const std::string& manufacturer_get() const { return manufacturer; }
	void software_path_set(const std::string& A) const { software_path = A; }
	const std::string& software_path_get() const { return software_path; }
	void software_set(bool A) { flag_set(A, flag_software); }
	bool software_get() const { return flag_get(flag_software); }
	void filled_set(bool A) const { flag_set(A, flag_filled); }
	bool filled_get() const { return flag_get(flag_filled); }
	void time_set(unsigned A) const { flag |= flag_time_set; time = A; }
	bool is_time_set() const { return flag_get(flag_time_set); }

	unsigned time_get() const { return time; }
	unsigned time_tree_get() const;
	void session_set(unsigned A) const { flag |= flag_session_set; session = A; }
	bool is_session_set() const { return flag_get(flag_session_set); }
	unsigned session_tree_get() const;
	unsigned session_get() const { return session; }
	void size_set(unsigned Asize) const { size = Asize; }
	unsigned size_get() const { return size; }
	void sizex_set(unsigned A) { sizex = A; }
	unsigned sizex_get() const { return sizex; }
	void sizey_set(unsigned A) { sizey = A; }
	unsigned sizey_get() const { return sizey; }
	void aspectx_set(unsigned A) { aspectx = A; }
	unsigned aspectx_get() const { return aspectx; }
	void aspecty_set(unsigned A) { aspecty = A; }
	unsigned aspecty_get() const { return aspecty; }
	void emulator_set(emulator* A) { emu = A; }
	emulator* emulator_get() const { return emu; }

	void preview_snap_set(const resource& A) const { snap_path = A; }
	void preview_snap_set_ifmissing(const resource& A) const { if (!snap_path.is_valid()) snap_path = A; }
	const resource& preview_snap_get() const { return snap_path; }
	void preview_clip_set(const resource& A) const { clip_path = A; }
	void preview_clip_set_ifmissing(const resource& A) const { if (!clip_path.is_valid()) clip_path = A; }
	const resource& preview_clip_get() const { return clip_path; }
	void preview_flyer_set(const resource& A) const { flyer_path = A; }
	void preview_flyer_set_ifmissing(const resource& A) const { if (!flyer_path.is_valid()) flyer_path = A; }
	const resource& preview_flyer_get() const { return flyer_path; }
	void preview_cabinet_set(const resource& A) const { cabinet_path = A; }
	void preview_cabinet_set_ifmissing(const resource& A) const { if (!cabinet_path.is_valid()) cabinet_path = A; }
	const resource& preview_cabinet_get() const { return cabinet_path; }
	void preview_sound_set(const resource& A) const { sound_path = A; }
	void preview_sound_set_ifmissing(const resource& A) const { if (!sound_path.is_valid()) sound_path = A; }
	const resource& preview_sound_get() const { return sound_path; }
	void preview_icon_set(const resource& A) const { icon_path = A; }
	void preview_icon_set_ifmissing(const resource& A) const { if (!icon_path.is_valid()) icon_path = A; }
	const resource& preview_icon_get() const { return icon_path; }
	void preview_marquee_set(const resource& A) const { marquee_path = A; }
	void preview_marquee_set_ifmissing(const resource& A) const { if (!marquee_path.is_valid()) marquee_path = A; }
	const resource& preview_marquee_get() const { return marquee_path; }
	void preview_title_set(const resource& A) const { title_path = A; }
	void preview_title_set_ifmissing(const resource& A) const { if (!title_path.is_valid()) title_path = A; }
	const resource& preview_title_get() const { return title_path; }

	bool preview_zip_set(const std::string & zip, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1) const;
	bool preview_dir_set(const std::string & dir, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1) const;
	bool preview_list_set(const std::string & list, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1) const;
	bool preview_software_list_set(const std::string & list, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1) const;

	bool preview_find(resource& path, const resource& (game::*preview_get)() const) const;

	void parent_set(const game* A) const { parent = A; }
	unsigned clone_get() const { return clone_bag.size(); }

	void rom_zip_set_insert(const std::string& Afile) const;
	const path_container& rom_zip_set_get() const { return rzs; }

	const category* group_derived_get() const;
	const category* type_derived_get() const;
	std::string name_without_emulator_get() const;

	pgame_container& clone_bag_get() const { return clone_bag; }
	void clone_bag_erase() const { clone_bag.clear(); }
	const game* parent_get() const { return parent; }
	const game& bios_get() const;
	const game& root_get() const;
	const game& clone_best_get() const;

	machinedevice_container& machinedevice_bag_get() const { return machinedevice_bag; }

	bool present_get() const
	{
		return size_get() == 0 || rom_zip_set_get().size() > 0;
	}
	bool present_tree_get() const
	{
		return flag_get(flag_tree_present);
	}
	bool duplicate_get() const
	{
		return flag_get(flag_duplicate);
	}
};

typedef std::list<game> game_container;

struct game_by_name_less : std::binary_function<game, game, bool> {
	bool operator()(const game& A, const game& B) const
	{
		return A.name_get().compare(B.name_get()) < 0;
	}
};

struct game_by_play_less : std::binary_function<game, game, bool> {
	bool operator()(const game& A, const game& B) const
	{
		return A.play_get() > B.play_get();
	}
};

typedef std::set<game, game_by_name_less> game_by_name_set;

class game_set : public game_by_name_set {
public:
	typedef game_by_name_set::const_iterator const_iterator;
	typedef game_by_name_set::iterator iterator;

	void cache(merge_t merge);

	bool is_tree_rom_of_present(const std::string& name, merge_t type) const;
	bool is_game_tag(const std::string& name, const std::string& tag) const;
	game_set::const_iterator root_rom_of_get(const std::string& name) const;
	game_set::const_iterator root_clone_of_get(const std::string& name) const;
	bool is_game_rom_of(const std::string& name_son, const std::string& name_parent) const;
	bool is_game_clone_of(const std::string& name_son, const std::string& name_parent) const;

	bool preview_zip_set(const std::string & zip, const std::string & emulator_name, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1);
	bool preview_dir_set(const std::string & dir, const std::string & emulator_name, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1);
	bool preview_list_set(const std::string & list, const std::string & emulator_name, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1);

	bool preview_software_dir_set(const std::string & dir, const std::string & emulator_name, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1);
	bool preview_software_list_set(const std::string & list, const std::string & emulator_name, void (game::*preview_set)(const resource& s) const, const std::string & ext0, const std::string & ext1);
};

inline bool pgame_combine_less(const game* A, const game* B, bool (*FA)(const game*, const game*), bool (*FB)(const game*, const game*))
{
	if (FA(A, B)) return true;
	if (FA(B, A)) return false;
	return FB(A, B);
}

inline bool pgame_combine_less(const game* A, const game* B, bool (*FA)(const game*, const game*), bool (*FB)(const game*, const game*), bool (*FC)(const game*, const game*))
{
	if (FA(A, B)) return true;
	if (FA(B, A)) return false;
	if (FB(A, B)) return true;
	if (FB(B, A)) return false;
	return FC(A, B);
}

inline bool pgame_combine_less(const game* A, const game* B, bool (*FA)(const game*, const game*), bool (*FB)(const game*, const game*), bool (*FC)(const game*, const game*), bool (*FD)(const game*, const game*))
{
	if (FA(A, B)) return true;
	if (FA(B, A)) return false;
	if (FB(A, B)) return true;
	if (FB(B, A)) return false;
	if (FC(A, B)) return true;
	if (FC(B, A)) return false;
	return FD(A, B);
}


inline bool pgame_by_desc_less(const game* A, const game* B)
{
	return case_less(A->description_get(), B->description_get());
}

/**
 * Compare all the description from the parent to the specified games.
 */
inline bool pgame_by_leveldesc_less(const game* A, const game* B)
{
	const unsigned stack_max = 16;
	const game* Astack_begin[stack_max];
	const game** Astack_end = Astack_begin;
	const game** Astack_limit = Astack_begin + stack_max;
	const game* Bstack_begin[stack_max];
	const game** Bstack_end = Bstack_begin;
	const game** Bstack_limit = Bstack_begin + stack_max;
	while (A != 0 && Astack_end != Astack_limit) {
		*Astack_end++ = A;
		A = A->parent_get();
	}
	while (B != 0 && Bstack_end != Bstack_limit) {
		*Bstack_end++ = B;
		B = B->parent_get();
	}
	while (Astack_begin != Astack_end && Bstack_begin != Bstack_end) {
		--Astack_end;
		--Bstack_end;
		if ((*Astack_end)->software_get() && !(*Bstack_end)->software_get())
			return true;
		if (!(*Astack_end)->software_get() && (*Bstack_end)->software_get())
			return false;
		if ((*Astack_end)->description_get() != (*Bstack_end)->description_get())
			return pgame_by_desc_less(*Astack_end, *Bstack_end);
	}
	return Astack_begin == Astack_end && Bstack_begin != Bstack_end;
}

inline bool pgame_by_manufacturer_less(const game* A, const game* B)
{
	return case_less(A->manufacturer_get(), B->manufacturer_get());
}

inline bool pgame_by_year_less(const game* A, const game* B)
{
	return A->year_get() > B->year_get();
}

inline bool pgame_by_size_less(const game* A, const game* B)
{
	return A->size_get() > B->size_get();
}

bool pgame_by_emulator_less(const game* A, const game* B);

inline bool pgame_by_res_less(const game* A, const game* B)
{
#if 0
	/* sort by area */
	int aA = A->sizex_get() * A->sizey_get();
	int aB = B->sizex_get() * B->sizey_get();
	if (aA < aB)
		return true;
	if (aA > aB)
		return false;
	return A->sizex_get() < B->sizex_get();
#else
	/* sort by x res */
	if (A->sizex_get() < B->sizex_get())
		return true;
	if (A->sizex_get() > B->sizex_get())
		return false;
	return A->sizey_get() < B->sizey_get();
#endif
}

inline bool pgame_by_info_less(const game* A, const game* B)
{
	return A->info_get() < B->info_get();
}

inline bool pgame_by_clone_less(const game* A, const game* B)
{
	return (A->parent_get() != 0) < (B->parent_get() != 0);
}

inline bool pgame_by_name_less(const game* A, const game* B)
{
	return A->name_get() < B->name_get();
}

// -------------------------------------------------------------------------
// dup

class dupe {
	std::string name;
	mutable int count;
public:
	dupe(const std::string& Aname, int Acount) : name(Aname), count(Acount) { }

	const std::string& name_get() const { return name; }
	int count_get() const { return count; }
	void count_set(int Acount) const { count = Acount; }

	bool operator<(const dupe& A) const
	{
		return name_get() < A.name_get();
	}
};

typedef std::set<dupe> dupe_set;

// -------------------------------------------------------------------------
// Sort category

// Return the element sorted for fast moving
typedef std::string sort_item_func (const game& g);

std::string sort_item_root_name(const game& g);
std::string sort_item_name(const game& g);
std::string sort_item_manufacturer(const game& g);
std::string sort_item_year(const game& g);
std::string sort_item_time(const game& g);
std::string sort_item_smart_time(const game& g);
std::string sort_item_session(const game& g);
std::string sort_item_group(const game& g);
std::string sort_item_type(const game& g);
std::string sort_item_size(const game& g);
std::string sort_item_res(const game& g);
std::string sort_item_info(const game& g);
std::string sort_item_timepersession(const game& g);
std::string sort_item_emulator(const game& g);

#endif

