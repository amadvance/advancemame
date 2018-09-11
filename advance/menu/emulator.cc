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

#include "portable.h"

#include "text.h"
#include "emulator.h"
#include "submenu.h"
#include "common.h"
#include "menu.h"
#include "game.h"
#include "mconfig.h"

#include "advance.h"

#include <fstream>
#include <sstream>

using namespace std;

//---------------------------------------------------------------------------
// utils

static bool spawn_check(int r, bool ignore_error)
{
	if (r == -1) {
		if (!ignore_error)
			target_err("Error process not executed with errno %d.\n", (unsigned)errno);
		return false;
	} else if (WIFSTOPPED(r)) {
		if (!ignore_error)
			target_err("Error process stopped with signal %d.\n", (unsigned)WSTOPSIG(r));
		return false;
	} else if (WIFSIGNALED(r)) {
		if (!ignore_error)
			target_err("Error process terminated with signal %d.\n", (unsigned)WTERMSIG(r));
		return false;
	} else if (WIFEXITED(r)) {
		if (WEXITSTATUS(r) != 0) {
			if (!ignore_error)
				target_err("Error process exited with status %d.\n", (unsigned)WEXITSTATUS(r));
			return false;
		}

		return true;
	} else {
		if (!ignore_error)
			target_err("Unknown process error code %d.\n", (unsigned)r);
		return false;
	}
}

//---------------------------------------------------------------------------
// emulator

#define ATTRIB_CHOICE_DX 20 * int_font_dx_get(text)

emulator::emulator(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: config(0), state(false), name(Aname), has_atleastarom(false)
{
	char path[FILE_MAXPATH];

	config = new config_emulator_state;

	user_exe_path = Aexe_path;
	user_cmd_arg = Acmd_arg;

	if (user_exe_path.length()) {
		string nodash_path = user_exe_path;
		if (nodash_path.length() > 0 && nodash_path[0] == '-')
			nodash_path.erase(0, 1);
		// search the emulator in the system path
		if (target_search(path, FILE_MAXPATH, nodash_path.c_str()) != 0) {
			// if not found assume in the current dir
			config_exe_path = path_abs(path_import(nodash_path), dir_cwd());
		} else {
			// use the file found
			config_exe_path = path_import(path);
		}
	} else {
		// no emulator specified
		config_exe_path = "";
	}

	exclude_missing_orig = exclude;
	exclude_duplicate_orig = include;
}

emulator::~emulator()
{
	delete config;
}

string emulator::attrib_compile(const string& value0, const string& value1)
{
	string name = "\"" + user_name_get() + "\" " + value0 + " " + value1;
	return name;
}

void emulator::attrib_load()
{
	exclude_missing_effective = exclude_missing_orig;
	exclude_duplicate_effective = exclude_duplicate_orig;
}

void emulator::attrib_save()
{
	exclude_missing_orig = exclude_missing_effective;
	exclude_duplicate_orig = exclude_duplicate_effective;
}

bool emulator::attrib_set(const string& value0, const string& value1)
{
	if (value0 == "missing") {
		if (!tristate(exclude_missing_orig, value1))
			return false;
	} else if (value0 == "duplicate") {
		if (!tristate(exclude_duplicate_orig, value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void emulator::attrib_get(adv_conf* config_context, const char* section, const char* tag)
{
	conf_string_set(config_context, section, tag, attrib_compile("missing", tristate(exclude_missing_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("duplicate", tristate(exclude_duplicate_orig)).c_str());
}

bool emulator::filter_working(const game& g) const
{
	return true;
}

bool emulator::filter(const game& g) const
{
	if (exclude_missing_effective == exclude && !g.present_tree_get())
		return false;
	if (exclude_missing_effective == exclude_not && g.present_tree_get())
		return false;
	if (exclude_duplicate_effective == exclude && g.duplicate_get())
		return false;
	if (exclude_duplicate_effective == exclude_not && !g.duplicate_get())
		return false;
	return true;
}

void emulator::cache(const game_set& gar, const game& g) const
{
}

bool emulator::is_present() const
{
	// if empty disable always
	if (user_exe_path.length() == 0)
		return false;

	// check that the emulator file exist and it's executable
	if (access(cpath_export(config_exe_path_get()), X_OK) != 0)
		return false;

	return true;
}

bool emulator::is_empty() const
{
	return !has_atleastarom;
}

bool emulator::is_runnable() const
{
	return true;
}

string emulator::exe_dir_get() const
{
	string dir = file_dir(config_exe_path_get());

	if (dir.length() == 1 && dir[0] == '/')
		return dir; // save the root slash
	else
		return slash_remove(dir);
}

void emulator::scan_game(const game_set& gar, const string& path, const string& name)
{
	game_set::const_iterator i = gar.find(game(name));
	if (i != gar.end()) {
		has_atleastarom = true;
		i->rom_zip_set_insert(path);
	}
}

void emulator::scan_dir(const game_set& gar, const string& dir, bool quiet)
{
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		if (!quiet)
			target_err("Error in '%s' opening roms directory '%s'.\n", user_name_get().c_str(), cpath_export(dir));
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d)) != 0) {
		string file = file_import(dd->d_name);
		if (file_ext(file) == ".zip") {
			scan_game(gar, dir + "/" + file, user_name_get() + "/" + file_basename(file));
		}
	}

	closedir(d);
}

void emulator::scan_dirlist(const game_set& gar, const string& dirlist, bool quiet)
{
	unsigned i = 0;
	while (i < dirlist.length()) {
		int end = dirlist.find(':', i);
		if (end == string::npos) {
			scan_dir(gar, string(dirlist, i), quiet);
			i = dirlist.size();
		} else {
			scan_dir(gar, string(dirlist, i, end - i), quiet);
			i = end + 1;
		}
	}
}

void emulator::load_dir(game_set& gar, const string& dir, const string& filterlist, bool quiet)
{
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		if (!quiet)
			target_err("Error in '%s' opening roms directory '%s'.\n", user_name_get().c_str(), cpath_export(dir));
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d)) != 0) {
		string file = file_import(dd->d_name);
		if (is_globlist(file, filterlist)) {
			struct stat st;
			if (stat(cpath_export(dir + "/" + file), &st) == 0) {
				if (!S_ISDIR(st.st_mode)) {
					string path = dir + "/" + file;
					string basename = file_basename(file);
					string name = user_name_get() + "/" + basename;

					game g;
					g.name_set(name);
					g.auto_description_set(case_auto(file_basename(dd->d_name)));
					g.rom_zip_set_insert(path);
					g.emulator_set(this);
					g.size_set(st.st_size);
					gar.insert(g);
				}
			}
		}
	}

	closedir(d);
}

void emulator::load_dirlist(game_set& gar, const string& dirlist, const string& filterlist, bool quiet)
{
	unsigned i = 0;
	while (i < dirlist.length()) {
		int end = dirlist.find(':', i);
		if (end == string::npos) {
			load_dir(gar, string(dirlist, i), filterlist, quiet);
			i = dirlist.size();
		} else {
			load_dir(gar, string(dirlist, i, end - i), filterlist, quiet);
			i = end + 1;
		}
	}
}

bool emulator::run_process(time_t& duration, const string& dir, int argc, const char** argv, bool ignore_error) const
{
	time_t start, stop;

	bool resume_error = false;

	// resume error if the user ask for it
	if (user_exe_path.length() > 0 && user_exe_path[0] == '-')
		resume_error = true;

	string olddir = dir_cwd();

	log_std(("menu: cwd '%s'\n", cpath_export(olddir)));

#ifdef USE_CHDIR_BEFORE_RUN
	log_std(("menu: chdir '%s'\n", cpath_export(dir)));
	if (chdir(cpath_export(dir)) != 0) {
		log_std(("menu: run error chdir '%s'\n", cpath_export(dir)));
		return false;
	}
#endif

	ostringstream cmdline;
	for (int i = 0; i < argc; ++i) {
		if (i != 0)
			cmdline << " ";
		if (strchr(argv[i], ' ')) {
			cmdline << "\"";
			cmdline << argv[i];
			cmdline << "\"";
		} else {
			cmdline << argv[i];
		}
	}

	log_std(("menu: run '%s'\n", cmdline.str().c_str()));

	start = time(0);

	int r = target_spawn(argv[0], argv);

	stop = time(0);

	if (stop - start > 8)
		duration = stop - start;
	else
		duration = 0;

	bool result = spawn_check(r, ignore_error || resume_error);

	if (resume_error)
		result = true;

	int_idle_time_reset();

	log_std(("menu: chdir '%s'\n", cpath_export(olddir)));
	if (chdir(cpath_export(olddir)) != 0) {
		log_std(("ERROR:menu: chdir '%s' failed\n", cpath_export(olddir)));
	}

	return result;
}

unsigned emulator::compile(const game& g, const char** argv, unsigned argc, const string& list, unsigned orientation) const
{
	int pos = 0;
	token_skip(list, pos, " \t");

	while (pos < list.length()) {
		string arg = token_get(list, pos, " \t");

		int ostart = arg.find("%o[");
		if (ostart != string::npos) {
			int opos = ostart + 3;
			string r0, r90, r180, r270;

			r0 = token_get(arg, opos, ",]");
			if (opos < arg.length() && arg[opos] == ',')
				++opos;
			r90 = token_get(arg, opos, ",]");
			if (opos < arg.length() && arg[opos] == ',')
				++opos;
			r180 = token_get(arg, opos, ",]");
			if (opos < arg.length() && arg[opos] == ',')
				++opos;
			r270 = token_get(arg, opos, "]");

			if (opos < arg.length() && arg[opos] == ']') {
				++opos;
				string o;
				switch (orientation) {
				case ADV_ORIENTATION_ROT0: o = r0; break;
				case ADV_ORIENTATION_ROT90: o = r90; break;
				case ADV_ORIENTATION_ROT180: o = r180; break;
				case ADV_ORIENTATION_ROT270: o = r270; break;
				}
				arg.erase(ostart, opos - ostart);
				arg.insert(ostart, o);
			}
		}

		arg = subs(arg, "%s", g.name_without_emulator_get());
		const path_container& pc = g.rom_zip_set_get();

		if (!pc.empty()) {
			string path = *pc.begin();
			arg = subs(arg, "%p", path_export(path));
			arg = subs(arg, "%f", path_export(file_file(path)));
		}

		if (arg.length())
			argv[argc++] = strdup(arg.c_str());

		token_skip(list, pos, " \t");
	}

	return argc;
}

bool emulator::run(const game& g, const game* bios, unsigned orientation, bool set_difficulty, difficulty_t difficulty, bool set_attenuation, int attenuation, bool ignore_error) const
{
	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(config_exe_path_get()));
	argc = compile(g, argv, argc, user_cmd_arg, orientation);
	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration, exe_dir_get(), argc, argv, ignore_error);

	for (int i = 0; i < argc; ++i)
		free(const_cast<char*>(argv[i]));

	if (ret) {
		g.time_set(g.time_get() + duration);
		g.session_set(g.session_get() + 1);
	}

	return ret;
}

unsigned emulator::preview_set(game_set& gar) const
{
	unsigned preview = 0;

	// search the previews in the software directory
	if (software_path_get().length()) {
		gar.preview_software_list_set(config_alts_path_get(), user_name_get(), &game::preview_snap_set_ifmissing, ".png", ".pcx");
		gar.preview_software_list_set(config_alts_path_get(), user_name_get(), &game::preview_clip_set_ifmissing, ".mng", "");
		gar.preview_software_list_set(config_alts_path_get(), user_name_get(), &game::preview_sound_set_ifmissing, ".mp3", ".wav");
	}

	// search the previews in the root directory
	gar.preview_list_set(config_alts_path_get(), user_name_get(), &game::preview_snap_set_ifmissing, ".png", ".pcx");
	gar.preview_list_set(config_alts_path_get(), user_name_get(), &game::preview_clip_set_ifmissing, ".mng", "");
	gar.preview_list_set(config_alts_path_get(), user_name_get(), &game::preview_sound_set_ifmissing, ".mp3", ".wav");

	if (gar.preview_list_set(config_icon_path_get(), user_name_get(), &game::preview_icon_set_ifmissing, ".ico", ""))
		preview |= preview_icon;
	if (gar.preview_list_set(config_flyer_path_get(), user_name_get(), &game::preview_flyer_set_ifmissing, ".png", ".pcx"))
		preview |= preview_flyer;
	if (gar.preview_list_set(config_cabinet_path_get(), user_name_get(), &game::preview_cabinet_set_ifmissing, ".png", ".pcx"))
		preview |= preview_cabinet;
	if (gar.preview_list_set(config_marquee_path_get(), user_name_get(), &game::preview_marquee_set_ifmissing, ".png", ".pcx"))
		preview |= preview_marquee;
	if (gar.preview_list_set(config_title_path_get(), user_name_get(), &game::preview_title_set_ifmissing, ".png", ".pcx"))
		preview |= preview_title;

	return preview;
}

void emulator::update(const game& g) const
{
	// update always the preview
	if (!g.preview_software_list_set(config_alts_path_get(), &game::preview_snap_set, ".png", ".pcx")
		&& !g.preview_list_set(config_alts_path_get(), &game::preview_snap_set, ".png", ".pcx"))
		g.preview_snap_set(resource());

	if (!g.preview_software_list_set(config_alts_path_get(), &game::preview_clip_set, ".mng", "")
		&& !g.preview_list_set(config_alts_path_get(), &game::preview_clip_set, ".mng", ""))
		g.preview_clip_set(resource());

	if (!g.preview_software_list_set(config_alts_path_get(), &game::preview_sound_set, ".mp3", ".wav")
		&& !g.preview_list_set(config_alts_path_get(), &game::preview_sound_set, ".mp3", ".wav"))
		g.preview_sound_set(resource());
}

bool emulator::validate_config_file(const string& file) const
{
	if (access(cpath_export(file), F_OK) != 0) {
		target_err("Error opening the '%s' configuration file '%s'. It doesn't exist.\n", user_name_get().c_str(), cpath_export(file));
		return false;
	}

	if (access(cpath_export(file), R_OK) != 0) {
		target_err("Error opening the '%s' configuration file '%s'. It isn't readable.\n", user_name_get().c_str(), cpath_export(file));
		return false;
	}

	return true;
}

//---------------------------------------------------------------------------
// mame_like

mame_info::mame_info(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	emulator(Aname, Aexe_path, Acmd_arg)
{

	exclude_clone_orig = exclude;
	exclude_bad_orig = exclude;
	exclude_vector_orig = include;
	exclude_vertical_orig = include;
}

void mame_info::attrib_load()
{
	emulator::attrib_load();
	exclude_clone_effective = exclude_clone_orig;
	exclude_bad_effective = exclude_bad_orig;
	exclude_vector_effective = exclude_vector_orig;
	exclude_vertical_effective = exclude_vertical_orig;
}

void mame_info::attrib_save()
{
	emulator::attrib_save();
	exclude_clone_orig = exclude_clone_effective;
	exclude_bad_orig = exclude_bad_effective;
	exclude_vector_orig = exclude_vector_effective;
	exclude_vertical_orig = exclude_vertical_effective;
}

bool mame_info::attrib_set(const std::string& value0, const std::string& value1)
{
	if (emulator::attrib_set(value0, value1))
		return true;

	if (value0 == "clone") {
		if (!tristate(exclude_clone_orig, value1))
			return false;
	} else if (value0 == "bad") {
		if (!tristate(exclude_bad_orig, value1))
			return false;
	} else if (value0 == "vector") {
		if (!tristate(exclude_vector_orig, value1))
			return false;
	} else if (value0 == "vertical") {
		if (!tristate(exclude_vertical_orig, value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void mame_info::attrib_get(adv_conf* config_context, const char* section, const char* tag)
{
	emulator::attrib_get(config_context, section, tag);
	conf_string_set(config_context, section, tag, attrib_compile("clone", tristate(exclude_clone_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("bad", tristate(exclude_bad_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("vector", tristate(exclude_vector_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("vertical", tristate(exclude_vertical_orig)).c_str());
}

bool mame_info::filter_working(const game& g) const
{
	bool good;

	good = g.play_get() != play_preliminary;

	if (exclude_bad_effective == exclude && !good)
		return false;
	if (exclude_bad_effective == exclude_not && good)
		return false;

	return true;
}

bool mame_info::filter(const game& g) const
{
	if (!emulator::filter(g))
		return false;

	// use always the bios in the check for the mess software
	const game& bios = g.bios_get();

	// softwares of clones are always listed
	if (!g.software_get()) {
		if (exclude_clone_effective == exclude && bios.parent_get() != 0)
			return false;
	}
	if (exclude_clone_effective == exclude_not && bios.parent_get() == 0)
		return false;

	bool good;
	if (exclude_clone_effective == exclude)
		good = bios.play_best_get() != play_preliminary;
	else
		good = bios.play_get() != play_preliminary;
	if (exclude_bad_effective == exclude && !good)
		return false;
	if (exclude_bad_effective == exclude_not && good)
		return false;

	if (exclude_vector_effective == exclude && bios.flag_get(flag_derived_vector))
		return false;
	if (exclude_vector_effective == exclude_not && !bios.flag_get(flag_derived_vector))
		return false;

	if (exclude_vertical_effective == exclude && bios.flag_get(flag_derived_vertical))
		return false;
	if (exclude_vertical_effective == exclude_not && !bios.flag_get(flag_derived_vertical))
		return false;

	// is a resource, not a game
	if (g.flag_get(flag_derived_resource))
		return false;

	return true;
}

void mame_info::cache(const game_set& gar, const game& g) const
{
	emulator::cache(gar, g);
}

bool mame_info::tree_get() const
{
	return exclude_clone_effective == exclude;
}

extern "C" int info_ext_get(void* _arg)
{
	istream* arg = static_cast<istream*>(_arg);
	return arg->get();
}

extern "C" void info_ext_unget(void* _arg, char c)
{
	istream* arg = static_cast<istream*>(_arg);
	arg->putback(c);
}

bool mame_info::update_xml()
{
	struct stat st_xml;
	struct stat st_mame;
	int err_xml;
	int err_exe;

	err_exe = stat(cpath_export(config_exe_path_get()), &st_mame);
	if (err_exe != 0)
		return false;

	string xml_file = path_abs(path_import(file_config_file_home((user_name_get() + ".xml").c_str())), dir_cwd());

	err_xml = stat(cpath_export(xml_file), &st_xml);

	// if it's updated and not empty
	if (err_xml == 0 && st_xml.st_size != 0 && st_xml.st_mtime >= st_mame.st_mtime)
		return true;

	// if it's readonly and not empty
	if (err_xml == 0 && st_xml.st_size != 0 && access(cpath_export(xml_file), W_OK) != 0)
		return true;

	target_out("Updating the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(xml_file));

	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(config_exe_path_get()));
	if (has_bare_xml())
		argv[argc++] = strdup("-listbare");
	else
		argv[argc++] = strdup("-listxml");
	argv[argc] = 0;

	int r = target_spawn_redirect(argv[0], argv, cpath_export(xml_file));

	for (int i = 0; i < argc; ++i)
		free(const_cast<char*>(argv[i]));

	if (!spawn_check(r, false)) {
		remove(cpath_export(xml_file));
		return false;
	}

	// check again the time
	err_xml = stat(cpath_export(xml_file), &st_xml);
	if (err_xml != 0)
		return false;

	if (st_xml.st_mtime < st_mame.st_mtime) {
#if HAVE_UTIMES
		struct timeval utimes_xml[2];
		utimes_xml[0].tv_sec = st_mame.st_mtime + 1;
		utimes_xml[0].tv_usec = 0;
		utimes_xml[1].tv_sec = st_mame.st_mtime + 1;
		utimes_xml[1].tv_usec = 0;
		target_out("System time is wrong, forcing the time of file '%s'.\n", cpath_export(xml_file));
		err_xml = utimes(cpath_export(xml_file), utimes_xml);
		if (err_xml != 0)
			target_out("Error setting the file of file '%s'.\n", cpath_export(xml_file));
#else
		target_out("System time is wrong, the file '%s' will be generated again until you fix it.\n", cpath_export(xml_file));
#endif
	}

	return true;
}

bool mame_info::load_game_xml(game_set& gar)
{
	string xml_file = path_abs(path_import(file_config_file_home((user_name_get() + ".xml").c_str())), dir_cwd());

	ifstream f(cpath_export(xml_file), ios::in | ios::binary);
	if (!f) {
		target_err("Error opening the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(xml_file));
		target_err("Try running manually the command: '%s -listxml > %s'.\n", user_exe_path.c_str(), cpath_export(xml_file));
		return false;
	}
	if (!load_xml(f, gar)) {
		f.close();
		target_err("Error reading the '%s' information from file '%s'.\n", user_name_get().c_str(), cpath_export(xml_file));
		return false;
	}
	f.close();

	return true;
}

bool mame_info::is_present_xml()
{
	string xml_file = path_abs(path_import(file_config_file_home((user_name_get() + ".xml").c_str())), dir_cwd());

	return access(cpath_export(xml_file), R_OK) == 0;
}

bool mame_info::load_game(game_set& gar, bool quiet)
{
	if (file_ext(config_exe_path_get()) == ".bat") {
		if (is_present_xml()) {
			return load_game_xml(gar);
		}

		target_err("Impossible to generate the '%s' information file with a BAT file.\n", user_name_get().c_str());
	} else {
		if (update_xml()) {
			return load_game_xml(gar);
		}

		target_err("Error generating the '%s' information file with -listxml.\n", user_name_get().c_str());
	}

	return false;
}

void mame_info::update(const game& g) const
{
	emulator::update(g);
}

//---------------------------------------------------------------------------
// mame_mame

static bool fread_uint32be(unsigned& v, FILE* f)
{
	v = 0;
	for (unsigned i = 0; i < 4; ++i) {
		unsigned char c;
		if (fread(&c, 1, 1, f) != 1)
			return false;
		v = (v << 8) + c;
	}
	return true;
}

static bool fread_uint16be(unsigned& v, FILE* f)
{
	v = 0;
	for (unsigned i = 0; i < 2; ++i) {
		unsigned char c;
		if (fread(&c, 1, 1, f) != 1)
			return false;
		v = (v << 8) + c;
	}
	return true;
}

static bool fskip(unsigned size, FILE* f)
{
	return fseek(f, size, SEEK_CUR) == 0;
}

mame_mame::mame_mame(const string& Aname, const string& Aexe_path, const string& Acmd_arg, bool Asupport_difficulty, bool Asupport_attenuation)
	: mame_info(Aname, Aexe_path, Acmd_arg)
{
	support_difficulty = Asupport_difficulty;
	support_attenuation = Asupport_attenuation;
	exclude_neogeo_orig = include;
	exclude_deco_orig = exclude;
	exclude_playchoice_orig = exclude;
}

int mame_mame::attrib_run(int x, int y)
{
	choice_bag ch;

	ch.insert(ch.end(), choice("Present or Missing", " Only\tPresent", " Only\tMissing", exclude_missing_effective, 0));
	ch.insert(ch.end(), choice("Unique or Duplicate", " Only\tUnique", " Only\tDuplicate", exclude_duplicate_effective, 0));
	ch.insert(ch.end(), choice("Working or Preliminary", " Only\tWorking", " Only\tPreliminary", exclude_bad_effective, 0));
	ch.insert(ch.end(), choice("Parent or Clone", " Only\tParent", " Only\tClone", exclude_clone_effective, 0));
	ch.insert(ch.end(), choice("Any Screen Type", " Only\tScreen Raster", " Only\tScreen Vector", exclude_vector_effective, 0));
	ch.insert(ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", exclude_vertical_effective, 0));
	ch.insert(ch.end(), choice("Neogeo", exclude_neogeo_effective, 0));
	ch.insert(ch.end(), choice("Cassette", exclude_deco_effective, 0));
	ch.insert(ch.end(), choice("PlayChoice-10", exclude_playchoice_effective, 0));

	choice_bag::iterator i = ch.begin();

	int key = ch.run(" " + user_name_get() + " Filters", x, y, ATTRIB_CHOICE_DX, i);

	if (key == EVENT_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_duplicate_effective = ch[1].tristate_get();
		exclude_bad_effective = ch[2].tristate_get();
		exclude_clone_effective = ch[3].tristate_get();
		exclude_vector_effective = ch[4].tristate_get();
		exclude_vertical_effective = ch[5].tristate_get();
		exclude_neogeo_effective = ch[6].tristate_get();
		exclude_deco_effective = ch[7].tristate_get();
		exclude_playchoice_effective = ch[8].tristate_get();
	}

	return key;
}

void mame_mame::attrib_load()
{
	mame_info::attrib_load();

	exclude_neogeo_effective = exclude_neogeo_orig;
	exclude_deco_effective = exclude_deco_orig;
	exclude_playchoice_effective = exclude_playchoice_orig;
}

void mame_mame::attrib_save()
{
	mame_info::attrib_save();

	exclude_neogeo_orig = exclude_neogeo_effective;
	exclude_deco_orig = exclude_deco_effective;
	exclude_playchoice_orig = exclude_playchoice_effective;
}

bool mame_mame::attrib_set(const std::string& value0, const std::string& value1)
{
	if (mame_info::attrib_set(value0, value1))
		return true;

	if (value0 == "neogeo") {
		if (!tristate(exclude_neogeo_orig, value1))
			return false;
	} else if (value0 == "deco") {
		if (!tristate(exclude_deco_orig, value1))
			return false;
	} else if (value0 == "playchoice") {
		if (!tristate(exclude_playchoice_orig, value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void mame_mame::attrib_get(adv_conf* config_context, const char* section, const char* tag)
{
	mame_info::attrib_get(config_context, section, tag);

	conf_string_set(config_context, section, tag, attrib_compile("neogeo", tristate(exclude_neogeo_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("deco", tristate(exclude_deco_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("playchoice", tristate(exclude_playchoice_orig)).c_str());
}

bool mame_mame::filter(const game& g) const
{
	if (!mame_info::filter(g))
		return false;

	if (exclude_neogeo_effective == exclude && g.flag_get(flag_derived_neogeo))
		return false;
	if (exclude_neogeo_effective == exclude_not && !g.flag_get(flag_derived_neogeo))
		return false;
	if (exclude_deco_effective == exclude && g.flag_get(flag_derived_deco))
		return false;
	if (exclude_deco_effective == exclude_not && !g.flag_get(flag_derived_deco))
		return false;
	if (exclude_playchoice_effective == exclude && g.flag_get(flag_derived_playchoice))
		return false;
	if (exclude_playchoice_effective == exclude_not && !g.flag_get(flag_derived_playchoice))
		return false;

	return true;
}

void mame_mame::cache(const game_set& gar, const game& g) const
{
	mame_info::cache(gar, g);

	const game& bios = g.bios_get();
	g.flag_set(gar.is_game_tag(bios.name_get(), "neogeo"), flag_derived_neogeo);
	g.flag_set(gar.is_game_tag(bios.name_get(), "decocass"), flag_derived_deco);
	g.flag_set(gar.is_game_tag(bios.name_get(), "playch10"), flag_derived_playchoice);
}

bool mame_mame::load_data(const game_set& gar)
{
	return true;
}

bool mame_mame::load_software(game_set&, bool quiet)
{
	return true;
}

bool mame_mame::run(const game& g, const game* bios, unsigned orientation, bool set_difficulty, difficulty_t difficulty, bool set_attenuation, int attenuation, bool ignore_error) const
{
	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(config_exe_path_get()));
	argc = compile(g, argv, argc, "%s", orientation);

	if (support_difficulty && set_difficulty) {
		const char* opt;
		switch (difficulty) {
		case difficulty_easiest: opt = "easiest"; break;
		case difficulty_easy: opt = "easy"; break;
		case difficulty_medium: opt = "normal"; break;
		case difficulty_hard: opt = "hard"; break;
		case difficulty_hardest: opt = "hardest"; break;
		default:
			opt = 0;
			break;
		}
		if (opt) {
			argv[argc++] = strdup("-difficulty");
			argv[argc++] = strdup(opt);
		}
	}

	if (support_attenuation && set_attenuation) {
		if (attenuation < -40)
			attenuation = -40;
		if (attenuation > 0)
			attenuation = 0;
		argv[argc++] = strdup("-sound_volume");

		ostringstream att;
		att << attenuation;
		argv[argc++] = strdup(att.str().c_str());
	}

	argc = compile(g, argv, argc, user_cmd_arg, orientation);
	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration, exe_dir_get(), argc, argv, ignore_error);

	for (int i = 0; i < argc; ++i)
		free(const_cast<char*>(argv[i]));

	if (ret) {
		g.time_set(g.time_get() + duration);
		g.session_set(g.session_get() + 1);
	}

	return ret;
}

//---------------------------------------------------------------------------
// dmame

#if !USE_NIX
dmame::dmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mame(Aname, Aexe_path, Acmd_arg, false, false)
{
}

string dmame::type_get() const
{
	return "dmame";
}

bool dmame::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir = exe_dir_get();

	string config_file = slash_add(ref_dir) + "mame.cfg";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rompath");
	conf_string_register(context, "snap");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 0, 0, 0, 0, 0) != 0) {
		target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "directory", "rompath", &s) == 0) {
		emu_rom_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_rom_path = list_abs(list_import("roms"), ref_dir);
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "directory", "snap", &s) == 0) {
		emu_snap_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_snap_path = list_abs(list_import("snap"), ref_dir);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), ref_dir));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path), ref_dir));
	config_icon_path = list_abs(list_import(user_icon_path), ref_dir);
	config_flyer_path = list_abs(list_import(user_flyer_path), ref_dir);
	config_cabinet_path = list_abs(list_import(user_cabinet_path), ref_dir);
	config_marquee_path = list_abs(list_import(user_marquee_path), ref_dir);
	config_title_path = list_abs(list_import(user_title_path), ref_dir);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	return true;
}
#endif

//---------------------------------------------------------------------------
// wmame

#if !USE_NIX
wmame::wmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mame(Aname, Aexe_path, Acmd_arg, false, false)
{
}

string wmame::type_get() const
{
	return "mame";
}

bool wmame::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir = exe_dir_get();

	string config_file = slash_add(ref_dir) + "mame.ini";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rompath");
	conf_string_register(context, "snapshot_directory");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "", "rompath", &s) == 0) {
		emu_rom_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_rom_path = list_abs(list_import("roms"), ref_dir);
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "snapshot_directory", &s) == 0) {
		emu_snap_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_snap_path = list_abs(list_import("snap"), ref_dir);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), ref_dir));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path), ref_dir));
	config_icon_path = list_abs(list_import(user_icon_path), ref_dir);
	config_flyer_path = list_abs(list_import(user_flyer_path), ref_dir);
	config_cabinet_path = list_abs(list_import(user_cabinet_path), ref_dir);
	config_marquee_path = list_abs(list_import(user_marquee_path), ref_dir);
	config_title_path = list_abs(list_import(user_title_path), ref_dir);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	return true;
}
#endif

//---------------------------------------------------------------------------
// umame

#if USE_NIX
umame::umame(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mame(Aname, Aexe_path, Acmd_arg, false, false)
{
}

string umame::type_get() const
{
	return "mame";
}

string list_import_from_dos_forced(string s)
{
	for (int i = 0; i < s.length(); ++i)
		if (s[i] == ';')
			s[i] = ':';
	return s;
}

bool umame::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;
	string home_dir;

#ifdef __WIN32__
	string ref_dir = exe_dir_get();

	home_dir = ref_dir;

	string config_file = slash_add(ref_dir) + "mame.ini";
#else
	char* home = getenv("HOME");
	if (!home || !*home) {
		target_err("Environment variable HOME not set.\n");
		return false;
	}

	home_dir = home;

	string ref_dir = slash_add(home) + ".mame";

	string config_file = path_import(slash_add(ref_dir) + "mame.ini");
#endif

	if (!validate_config_file(config_file)) {
		return false;
	}

	context = conf_init();

	conf_string_register(context, "rompath");
	conf_string_register(context, "snapshot_directory");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
		conf_done(context);
		return false;
	}

	// Uses list_import_from_dos_forced because also Linux SDL MAME uses ';' as separator dir !

	if (conf_string_section_get(context, "", "rompath", &s) == 0) {
		string replace = subs(s, "$HOME", home_dir);
		emu_rom_path = list_abs(list_import_from_dos_forced(list_import(replace)), ref_dir);
	} else {
		emu_rom_path = list_abs(list_import_from_dos_forced(list_import("roms")), ref_dir);
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "snapshot_directory", &s) == 0) {
		string replace = subs(s, "$HOME", home_dir);
		emu_snap_path = list_abs(list_import_from_dos_forced(list_import(replace)), ref_dir);
	} else {
		emu_snap_path = list_abs(list_import_from_dos_forced(list_import("snap")), ref_dir);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), ref_dir));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path), ref_dir));
	config_icon_path = list_abs(list_import(user_icon_path), ref_dir);
	config_flyer_path = list_abs(list_import(user_flyer_path), ref_dir);
	config_cabinet_path = list_abs(list_import(user_cabinet_path), ref_dir);
	config_marquee_path = list_abs(list_import(user_marquee_path), ref_dir);
	config_title_path = list_abs(list_import(user_title_path), ref_dir);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	return true;
}
#endif

//---------------------------------------------------------------------------
// advmame

advmame::advmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mame(Aname, Aexe_path, Acmd_arg, true, true)
{
}

string advmame::type_get() const
{
	return "advmame";
}

string list_importmultidir(const string& list, const string& ref)
{
	return list_import(file_config_list(list.c_str(), file_config_dir_multidir, ref.c_str()));
}

string list_importsingledir(const string& list, const string& ref)
{
	return list_import(file_config_list(list.c_str(), file_config_dir_singledir, ref.c_str()));
}

bool advmame::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir_os = cpath_export(exe_dir_get());

	string config_file = path_abs(path_import(file_config_file_home("advmame.rc")), exe_dir_get());

	context = conf_init();

	conf_string_register(context, "dir_rom");
	conf_string_register(context, "dir_snap");

	// if configuration is missing, we use default rom path
	if (validate_config_file(config_file)) {
		if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
			target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
			conf_done(context);
			return false;
		}
	}

	if (conf_string_section_get(context, "", "dir_rom", &s) == 0) {
		emu_rom_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_rom_path = list_importmultidir("rom", ref_dir_os);
	}

	emu_software_path = "";

	// the emulator correct path is a single dir, but allows the
	// menu to load in the multi dir version
	if (conf_string_section_get(context, "", "dir_snap", &s) == 0) {
		emu_snap_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_snap_path = list_importmultidir("snap", ref_dir_os);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_importmultidir(user_rom_path, ref_dir_os));
	config_alts_path = dir_cat(emu_snap_path, list_importmultidir(user_alts_path, ref_dir_os));
	config_icon_path = list_importmultidir(user_icon_path, ref_dir_os);
	config_flyer_path = list_importmultidir(user_flyer_path, ref_dir_os);
	config_cabinet_path = list_importmultidir(user_cabinet_path, ref_dir_os);
	config_marquee_path = list_importmultidir(user_marquee_path, ref_dir_os);
	config_title_path = list_importmultidir(user_title_path, ref_dir_os);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	return true;
}

//---------------------------------------------------------------------------
// advpac

advpac::advpac(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mame(Aname, Aexe_path, Acmd_arg, false, false)
{
}

string advpac::type_get() const
{
	return "advpac";
}

bool advpac::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir_os = cpath_export(exe_dir_get());

	string config_file = path_abs(path_import(file_config_file_home("advpac.rc")), exe_dir_get());

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "dir_rom");
	conf_string_register(context, "dir_snap");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "", "dir_rom", &s) == 0) {
		emu_rom_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_rom_path = list_importmultidir("rom", ref_dir_os);
	}

	emu_software_path = "";

	// the emulator correct path is a single dir, but allows the
	// menu to load in the multi dir version
	if (conf_string_section_get(context, "", "dir_snap", &s) == 0) {
		emu_snap_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_snap_path = list_importmultidir("snap", ref_dir_os);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_importmultidir(user_rom_path, ref_dir_os));
	config_alts_path = dir_cat(emu_snap_path, list_importmultidir(user_alts_path, ref_dir_os));
	config_icon_path = list_importmultidir(user_icon_path, ref_dir_os);
	config_flyer_path = list_importmultidir(user_flyer_path, ref_dir_os);
	config_cabinet_path = list_importmultidir(user_cabinet_path, ref_dir_os);
	config_marquee_path = list_importmultidir(user_marquee_path, ref_dir_os);
	config_title_path = list_importmultidir(user_title_path, ref_dir_os);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	return true;
}

//---------------------------------------------------------------------------
// mame_mess

mame_mess::mame_mess(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_info(Aname, Aexe_path, Acmd_arg)
{
	exclude_empty_orig = exclude;
}

void mame_mess::attrib_load()
{
	mame_info::attrib_load();
	exclude_empty_effective = exclude_empty_orig;
}

void mame_mess::attrib_save()
{
	mame_info::attrib_save();
	exclude_empty_orig = exclude_empty_effective;
}

bool mame_mess::attrib_set(const std::string& value0, const std::string& value1)
{
	if (mame_info::attrib_set(value0, value1))
		return true;

	if (value0 == "empty") {
		if (!tristate(exclude_empty_orig, value1))
			return false;
	} else {
		return false;
	}

	return true;
}

void mame_mess::attrib_get(adv_conf* config_context, const char* section, const char* tag)
{
	mame_info::attrib_get(config_context, section, tag);
	conf_string_set(config_context, section, tag, attrib_compile("empty", tristate(exclude_empty_orig)).c_str());
}

bool mame_mess::filter(const game& g) const
{
	if (!mame_info::filter(g))
		return false;

	const game& bios = g.bios_get();

	// include always software and the bios that has software
	if (!g.software_get() && !g.filled_get()) {
		if (exclude_empty_effective == exclude && bios.size_get() == 0)
			return false;
		if (exclude_empty_effective == exclude_not && bios.size_get() != 0)
			return false;
	}

	return true;
}

int mame_mess::attrib_run(int x, int y)
{
	choice_bag ch;

	ch.insert(ch.end(), choice("Present or Missing", " Only\tPresent", " Only\tMissing", exclude_missing_effective, 0));
	ch.insert(ch.end(), choice("Unique or Duplicate", " Only\tUnique", " Only\tDuplicate", exclude_duplicate_effective, 0));
	ch.insert(ch.end(), choice("Working or Preliminary", " Only\tWorking", " Only\tPreliminary", exclude_bad_effective, 0));
	ch.insert(ch.end(), choice("Parent or Clone", " Only\tParent", " Only\tClone", exclude_clone_effective, 0));
	ch.insert(ch.end(), choice("Any Screen Type", " Only\tScreen Raster", " Only\tScreen Vector", exclude_vector_effective, 0));
	ch.insert(ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", exclude_vertical_effective, 0));
	ch.insert(ch.end(), choice("With BIOS or not", " Only\tWith BIOS", " Only\tWithout BIOS", exclude_empty_effective, 0));

	choice_bag::iterator i = ch.begin();

	int key = ch.run(" " + user_name_get() + " Filters", x, y, ATTRIB_CHOICE_DX, i);

	if (key == EVENT_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_duplicate_effective = ch[1].tristate_get();
		exclude_bad_effective = ch[2].tristate_get();
		exclude_clone_effective = ch[3].tristate_get();
		exclude_vector_effective = ch[4].tristate_get();
		exclude_vertical_effective = ch[5].tristate_get();
		exclude_empty_effective = ch[6].tristate_get();
	}

	return key;
}

//---------------------------------------------------------------------------
// mess

#if !USE_NIX
dmess::dmess(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mess(Aname, Aexe_path, Acmd_arg)
{
}

string dmess::type_get() const
{
	return "dmess";
}

bool dmess::load_data(const game_set& gar)
{
	return true;
}

bool dmess::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir = exe_dir_get();

	string config_file = slash_add(ref_dir) + "mess.cfg";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "snap");
	conf_string_register(context, "biospath");
	conf_string_register(context, "softwarepath");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 0, 0, 0, 0, 0) != 0) {
		target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "directory", "biospath", &s) == 0) {
		emu_rom_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_rom_path = list_abs("bios", ref_dir);
	}

	if (conf_string_section_get(context, "directory", "softwarepath", &s) == 0) {
		emu_software_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_software_path = list_abs("software", ref_dir);
	}

	if (conf_string_section_get(context, "directory", "snap", &s) == 0) {
		emu_snap_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_snap_path = list_abs("snap", ref_dir);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), ref_dir));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path), ref_dir));
	config_icon_path = list_abs(list_import(user_icon_path), ref_dir);
	config_flyer_path = list_abs(list_import(user_flyer_path), ref_dir);
	config_cabinet_path = list_abs(list_import(user_cabinet_path), ref_dir);
	config_marquee_path = list_abs(list_import(user_marquee_path), ref_dir);
	config_title_path = list_abs(list_import(user_title_path), ref_dir);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	for (game_set::const_iterator i = gar.begin(); i != gar.end(); ++i) {
		if (i->emulator_get() == this) {
			if (conf_string_section_get(context, const_cast<char*>(i->name_get().c_str()), "softwarepath", &s) == 0) {
				i->software_path_set(s);
			} else {
				i->software_path_set(emu_software_path);
			}
		}
	}

	conf_done(context);

	return true;
}

bool dmess::scan_software_by_sys(game_container& gac, const string& software, const string& parent)
{
	bool atleastone = false;
	string software_dir = software + parent;
	DIR* dir = opendir(cpath_export(software_dir));
	if (!dir)
		return false;

	struct dirent* ddir;
	while ((ddir = readdir(dir)) != 0) {
		string file = file_import(ddir->d_name);
		if (file_ext(file) == ".zip") {
			string name = file_basename(file);

			game g;
			g.emulator_set(this);
			g.name_set(user_name_get() + "/" + parent + "/" + name);
			g.software_set(true);
			g.cloneof_set(user_name_get() + "/" + parent);
			g.romof_set(user_name_get() + "/" + parent);

			string path = software_dir + "/" + file;
			g.rom_zip_set_insert(path);
			g.auto_description_set(case_auto(file_basename(ddir->d_name)));

			gac.insert(gac.end(), g);
			atleastone = true;
		}
	}

	closedir(dir);

	return atleastone;
}

void dmess::scan_software(game_container& gac, const game_set& gar)
{
	for (game_set::const_iterator i = gar.begin(); i != gar.end(); ++i) {
		if (i->emulator_get() == this) {
			int ptr = 0;
			while (ptr < i->software_path_get().length()) {
				string path = slash_add(token_get(i->software_path_get(), ptr, file_dir_separator()));

				token_skip(i->software_path_get(), ptr, file_dir_separator());

				if (scan_software_by_sys(gac, path, i->name_without_emulator_get()))
					i->filled_set(true);
			}
		}
	}
}

void dmess::scan_alias(game_set& gar, game_container& gac, const string& cfg)
{
	ifstream f(cpath_export(cfg));
	if (!f) {
		target_err("Error opening the '%s' configuration file '%s' for reading the alias.\n", user_name_get().c_str(), cpath_export(cfg));
		return;
	}

	game_set::const_iterator parent_g = gar.end();

	while (!f.eof()) {
		string s;
		getline(f, s, '\n');

		if (s.length() && s[0] != '#') {
			int ptr = 0;

			token_skip(s, ptr, " ");
			string first = token_get(s, ptr, " =");

			if (first.length() >= 2 && first[0] == '[' && first[first.length() - 1] == ']') {
				// section definition
				string name = user_name_get() + "/" + string(first, 1, first.length() - 2);
				parent_g = gar.find(game(name));
			} else {
				if (parent_g != gar.end()) {
					// alias definition
					game g;
					g.emulator_set(this);
					g.name_set(parent_g->name_get() + string("/") + first);
					g.software_set(true);
					g.flag_set(true, flag_derived_alias);
					g.cloneof_set(parent_g->name_get());
					g.romof_set(parent_g->name_get());
					token_skip(s, ptr, " =");

					// skip until the first #
					token_get(s, ptr, "#");

					if (ptr < s.length() && s[ptr] == '#') {
						++ptr; // skip the #
						g.auto_description_set(strip_space(token_get(s, ptr, "|")));
						if (ptr < s.length() && s[ptr] == '|') ++ptr;
						g.year_set(strip_space(token_get(s, ptr, "|")));
						if (ptr < s.length() && s[ptr] == '|') ++ptr;
						g.manufacturer_set(strip_space(token_get(s, ptr, "|")));
					} else {
						g.auto_description_set(case_auto(first));
					}

					// insert the game
					gac.insert(gac.end(), g);
				}
			}
		}
	}

	f.close();
}

bool dmess::load_software(game_set& gar, bool quiet)
{
	game_container gac;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mess.cfg";

	if (!validate_config_file(config_file))
		return false;

	scan_alias(gar, gac, config_file);
	scan_software(gac, gar);

	// any duplicate alias/software is rejected in the insertion
	// in the game_set. the first is used.
	for (game_container::const_iterator i = gac.begin(); i != gac.end(); ++i)
		gar.insert(*i);

	return true;
}

string dmess::image_name_get(const string& snap_create, const string& name)
{
	char path_buffer[FILE_MAXPATH];
	snprintf(path_buffer, sizeof(path_buffer), "%s%.8s.png", slash_add(snap_create).c_str(), name.c_str());

	int snapno = 0;
	while (access(cpath_export(path_buffer), F_OK) == 0) {
		snprintf(path_buffer, sizeof(path_buffer), "%s%.4s%04d.png", slash_add(snap_create).c_str(), name.c_str(), snapno);
		++snapno;
	}

	return path_buffer;
}

bool dmess::run(const game& g, const game* bios, unsigned orientation, bool set_difficulty, difficulty_t difficulty, bool set_attenuation, int attenuation, bool ignore_error) const
{
	string snapshot_rename_dir;
	string image_create_file;
	string image_rename_file;

	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	if (g.software_get()) {
		const game* bios = &g.bios_get();

		int i = 0;
		string dir = token_get(emu_snap_path, i, ":"); // use the first dir specification

		// name without emulator and bios
		string stripped_name = file_file(g.name_get());

		image_create_file = image_name_get(dir, bios->name_without_emulator_get());
		snapshot_rename_dir = slash_add(dir) + bios->name_without_emulator_get();
		image_rename_file = snapshot_rename_dir + "/" + stripped_name + ".png";

		argv[argc++] = strdup(cpath_export(config_exe_path));
		argv[argc++] = strdup(bios->name_without_emulator_get().c_str());

		if (g.flag_get(flag_derived_alias)) {
			argv[argc++] = strdup("-alias");
			argv[argc++] = strdup(stripped_name.c_str());
		} else {
			// default rom (without extension)
			string rom = stripped_name;
			// search the first file in the zip with the same filename of the zip
			for (path_container::const_iterator i = g.rom_zip_set_get().begin(); i != g.rom_zip_set_get().end(); ++i) {
				string zfile;
				unsigned zcrc;
				if (file_findinzip_byname(*i, stripped_name, zfile, zcrc)) {
					rom = zfile;
					break;
				}
			}
			argv[argc++] = strdup("-cart");
			argv[argc++] = strdup(rom.c_str());
		}
	} else {
		argv[argc++] = strdup(cpath_export(config_exe_path));
		argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	}

	argc = compile(g, argv, argc, user_cmd_arg, orientation);

	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration, exe_dir_get(), argc, argv, ignore_error);

	for (int i = 0; i < argc; ++i)
		free(const_cast<char*>(argv[i]));

	if (ret) {
		g.time_set(g.time_get() + duration);
		g.session_set(g.session_get() + 1);
	}

	if (g.software_get()) {
		if (access(cpath_export(image_create_file), F_OK) == 0) {
			if (target_mkdir(cpath_export(snapshot_rename_dir)) != 0) {
				log_std(("WARNING:%s: error creating dir %s\n", user_name_get().c_str(), cpath_export(snapshot_rename_dir)));
			}
			if (rename(cpath_export(image_create_file), cpath_export(image_rename_file)) != 0) {
				log_std(("WARNING:%s: error moving file %s to %s\n", user_name_get().c_str(), cpath_export(image_create_file), cpath_export(image_rename_file)));
			}
		}
	}

	return true;
}
#endif

//---------------------------------------------------------------------------
// advmess

advmess::advmess(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: mame_mess(Aname, Aexe_path, Acmd_arg)
{
}

string advmess::type_get() const
{
	return "advmess";
}

bool advmess::load_data(const game_set& gar)
{
	return true;
}

bool advmess::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir_os = path_export(exe_dir_get());

	string config_file = path_abs(path_import(file_config_file_home("advmess.rc")), exe_dir_get());

	context = conf_init();

	conf_string_register(context, "dir_rom");
	conf_string_register(context, "dir_image");
	conf_string_register(context, "dir_snap");

	// if configuration is missing, we use default rom path
	if (validate_config_file(config_file)) {
		if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
			target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
			conf_done(context);
			return false;
		}
	}

	if (conf_string_section_get(context, "", "dir_rom", &s) == 0) {
		emu_rom_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_rom_path = list_importmultidir("rom", ref_dir_os);
	}

	if (conf_string_section_get(context, "", "dir_image", &s) == 0) {
		emu_software_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_software_path = list_importmultidir("image", ref_dir_os);
	}

	// the emulator correct path is a single dir, but allows the
	// menu to load in the multi dir version
	if (conf_string_section_get(context, "", "dir_snap", &s) == 0) {
		emu_snap_path = list_importmultidir(s, ref_dir_os);
	} else {
		emu_snap_path = list_importmultidir("snap", ref_dir_os);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	config_rom_path = dir_cat(emu_rom_path, list_importmultidir(user_rom_path, ref_dir_os));
	config_alts_path = dir_cat(emu_snap_path, list_importmultidir(user_alts_path, ref_dir_os));
	config_icon_path = list_importmultidir(user_icon_path, ref_dir_os);
	config_flyer_path = list_importmultidir(user_flyer_path, ref_dir_os);
	config_cabinet_path = list_importmultidir(user_cabinet_path, ref_dir_os);
	config_marquee_path = list_importmultidir(user_marquee_path, ref_dir_os);
	config_title_path = list_importmultidir(user_title_path, ref_dir_os);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	for (game_set::const_iterator i = gar.begin(); i != gar.end(); ++i) {
		if (i->emulator_get() == this) {
			if (conf_string_section_get(context, const_cast<char*>(i->name_get().c_str()), "dir_image", &s) == 0) {
				i->software_path_set(s);
			} else {
				i->software_path_set(emu_software_path);
			}
		}
	}

	conf_done(context);

	return true;
}

bool advmess::scan_software_by_sys(game_container& gac, const string& software, const game& bios)
{
	bool atleastone = false;
	string parent = bios.name_without_emulator_get();
	string software_dir = software + parent;

	DIR* dir = opendir(cpath_export(software_dir));
	if (!dir)
		return false;

	struct dirent* ddir;
	while ((ddir = readdir(dir)) != 0) {
		if (ddir->d_name[0] != '.') { // skip some special files
			bool found = false;

			string file = file_import(ddir->d_name);
			string ext = file_ext(file);

			if (ext == ".zip") {
				found = true;
			} else if (ext.length() > 1) {
				string ext_without_dot = string(ext, 1, ext.length() - 1);
				for (machinedevice_container::const_iterator i = bios.machinedevice_bag_get().begin(); !found && i != bios.machinedevice_bag_get().end(); ++i) {
					for (machinedevice_ext_container::const_iterator j = i->ext_bag.begin(); !found && j != i->ext_bag.end(); ++j) {
						if (ext_without_dot == *j)
							found = true;
					}
				}
			}

			if (found) {
				string name = file_basename(file);

				game g;
				g.emulator_set(this);
				g.name_set(user_name_get() + "/" + parent + "/" + name);
				g.software_set(true);
				g.cloneof_set(user_name_get() + "/" + parent);
				g.romof_set(user_name_get() + "/" + parent);

				string path = software_dir + "/" + file;
				g.rom_zip_set_insert(path);

				g.auto_description_set(case_auto(file_basename(ddir->d_name)));

				gac.insert(gac.end(), g);
				atleastone = true;
			}
		}
	}

	closedir(dir);

	return atleastone;
}

void advmess::scan_software(game_container& gac, const game_set& gar)
{
	for (game_set::const_iterator i = gar.begin(); i != gar.end(); ++i) {
		if (i->emulator_get() == this) {
			int ptr = 0;
			while (ptr < i->software_path_get().length()) {
				string path = slash_add(token_get(i->software_path_get(), ptr, file_dir_separator()));

				token_skip(i->software_path_get(), ptr, file_dir_separator());

				if (scan_software_by_sys(gac, path, *i))
					i->filled_set(true);
			}
		}
	}
}

bool advmess::load_software(game_set& gar, bool quiet)
{
	game_container gac;

	scan_software(gac, gar);

	for (game_container::const_iterator i = gac.begin(); i != gac.end(); ++i)
		gar.insert(*i);

	return true;
}

string advmess::image_name_get(const string& snap_create, const string& name)
{
	char path_buffer[FILE_MAXPATH];
	snprintf(path_buffer, sizeof(path_buffer), "%s%.8s.png", slash_add(snap_create).c_str(), name.c_str());

	int snapno = 0;
	while (access(cpath_export(path_buffer), F_OK) == 0) {
		snprintf(path_buffer, sizeof(path_buffer), "%s%.4s%04d.png", slash_add(snap_create).c_str(), name.c_str(), snapno);
		++snapno;
	}

	return path_buffer;
}

string advmess::clip_name_get(const string& clip_create, const string& name)
{
	char path_buffer[FILE_MAXPATH];
	snprintf(path_buffer, sizeof(path_buffer), "%s%.8s.mng", slash_add(clip_create).c_str(), name.c_str());

	int snapno = 0;
	while (access(cpath_export(path_buffer), F_OK) == 0) {
		snprintf(path_buffer, sizeof(path_buffer), "%s%.4s%04d.mng", slash_add(clip_create).c_str(), name.c_str(), snapno);
		++snapno;
	}

	return path_buffer;
}

string advmess::sound_name_get(const string& sound_create, const string& name)
{
	char path_buffer[FILE_MAXPATH];
	snprintf(path_buffer, sizeof(path_buffer), "%s%.8s.wav", slash_add(sound_create).c_str(), name.c_str());

	int snapno = 0;
	while (access(cpath_export(path_buffer), F_OK) == 0) {
		snprintf(path_buffer, sizeof(path_buffer), "%s%.4s%04d.wav", slash_add(sound_create).c_str(), name.c_str(), snapno);
		++snapno;
	}

	return path_buffer;
}

///
// Check if the specified ext is supported.
// If yes add the required option at the command line
bool advmess::compile_ext(const game& g, unsigned& argc, const char* argv[], const string& ext) const
{
	for (machinedevice_container::const_iterator i = g.machinedevice_bag_get().begin(); i != g.machinedevice_bag_get().end(); ++i) {
		for (machinedevice_ext_container::const_iterator j = i->ext_bag.begin(); j != i->ext_bag.end(); ++j) {

			log_std(("menu:advmess: compile check ext .%s\n", j->c_str()));

			// case unsensitive compare, it may be in a zip file
			if (case_equal(ext, string(".") + *j)) {

				string option = "-dev_" + i->name;
				argv[argc] = strdup(option.c_str());
				++argc;

				log_std(("menu:advmess: ext match for %s\n", i->name.c_str()));
				return true;
			}
		}
	}

	return false;
}

///
// Scan the zip file for any supported file.
bool advmess::compile_zip(const game& g, unsigned& argc, const char* argv[], const string& zip_file) const
{
	adv_zip* zip;
	adv_zipent* ent;
	bool something_found = false;

	if (access(cpath_export(zip_file), F_OK) != 0)
		return false;

	log_std(("menu:advmess: compile zip %s\n", cpath_export(zip_file)));

	zip = zip_open(cpath_export(zip_file));
	if (!zip)
		return false;

	// name of the zip without the extension
	string name = file_file(file_basename(zip_file));

	while ((ent = zip_read(zip)) != 0) {
		log_std(("menu:advmess: compile in zip %s\n", ent->name));

		string zpath = ent->name;
		string zfile = file_file(zpath);
		string zname = file_basename(zfile);
		string zext = file_ext(zfile);

		if (compile_ext(g, argc, argv, zext)) {
			string spec = name + '=' + zfile;
			argv[argc] = strdup(spec.c_str());
			++argc;
			something_found = true;
		}
	}

	zip_close(zip);

	return something_found;
}

///
// Check if the file is a supported one.
// If yes add the required options at the command line.
bool advmess::compile_single(const game& g, unsigned& argc, const char* argv[], const string& file) const
{
	string ext = file_ext(file);

	log_std(("menu:advmess: compile file %s\n", cpath_export(file)));

	if (compile_ext(g, argc, argv, ext)) {

		argv[argc] = strdup(file_file(file).c_str());
		++argc;

		return true;
	}

	return false;
}

bool advmess::compile_file(const game& g, unsigned& argc, const char* argv[], const string& file) const
{
	if (file_ext(file) == ".zip")
		return compile_zip(g, argc, argv, file);
	else
		return compile_single(g, argc, argv, file);
}

bool advmess::run(const game& g, const game* bios, unsigned orientation, bool set_difficulty, difficulty_t difficulty, bool set_attenuation, int attenuation, bool ignore_error) const
{
	string snapshot_rename_dir;
	string image_create_file;
	string image_rename_file;
	string clip_create_file;
	string clip_rename_file;
	string sound_create_file;
	string sound_rename_file;

	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	if (!bios)
		bios = &g.bios_get();

	if (g.software_get()) {
		bool found = false;

		int i = 0;
		string dir = token_get(emu_snap_path, i, ":"); // use the first dir specification

		// name without emulator and bios
		string stripped_name = file_file(g.name_get());

		image_create_file = image_name_get(dir, bios->name_without_emulator_get());
		clip_create_file = clip_name_get(dir, bios->name_without_emulator_get());
		sound_create_file = sound_name_get(dir, bios->name_without_emulator_get());
		snapshot_rename_dir = slash_add(dir) + bios->name_without_emulator_get();
		image_rename_file = snapshot_rename_dir + "/" + stripped_name + ".png";
		clip_rename_file = snapshot_rename_dir + "/" + stripped_name + ".mng";
		sound_rename_file = snapshot_rename_dir + "/" + stripped_name + ".wav";

		log_std(("%s: image path %s\n", user_name_get().c_str(), cpath_export(image_create_file)));
		log_std(("%s: clip path %s\n", user_name_get().c_str(), cpath_export(clip_create_file)));
		log_std(("%s: sound path %s\n", user_name_get().c_str(), cpath_export(sound_create_file)));

		argv[argc++] = strdup(cpath_export(config_exe_path));
		argv[argc++] = strdup(bios->name_without_emulator_get().c_str());

		for (path_container::const_iterator i = g.rom_zip_set_get().begin(); i != g.rom_zip_set_get().end(); ++i) {
			if (compile_file(g.bios_get(), argc, argv, *i)) {
				found = true;
				break;
			}

			if (!found) {
				log_std(("menu:advmess: compile abort\n"));

				if (!ignore_error) {
					string error = "Format not supported. ";

					error += "Supported extensions for system '" + file_file(g.bios_get().name_get()) + "' are:";
					for (machinedevice_container::const_iterator i = g.bios_get().machinedevice_bag_get().begin(); i != g.bios_get().machinedevice_bag_get().end(); ++i) {
						for (machinedevice_ext_container::const_iterator j = i->ext_bag.begin(); j != i->ext_bag.end(); ++j) {
							error += " ." + *j;
						}
					}
					error += "\n";
					target_err(error.c_str());
				}

				for (int i = 0; i < argc; ++i)
					free(const_cast<char*>(argv[i]));

				return false;
			}

		}
	} else {
		argv[argc++] = strdup(cpath_export(config_exe_path));
		argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	}

	if (attenuation < -32)
		attenuation = -32;
	if (attenuation > 0)
		attenuation = 0;
	argv[argc++] = strdup("-sound_volume");

	ostringstream att;
	att << attenuation;
	argv[argc++] = strdup(att.str().c_str());

	argc = compile(g, argv, argc, user_cmd_arg, orientation);

	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration, exe_dir_get(), argc, argv, ignore_error);

	for (int i = 0; i < argc; ++i)
		free(const_cast<char*>(argv[i]));

	if (ret) {
		g.time_set(g.time_get() + duration);
		g.session_set(g.session_get() + 1);
	}

	if (g.software_get()) {
		if (access(cpath_export(image_create_file), F_OK) == 0) {
			if (target_mkdir(cpath_export(snapshot_rename_dir)) != 0) {
				log_std(("WARNING:%s: error creating dir %s\n", user_name_get().c_str(), cpath_export(snapshot_rename_dir)));
			}
			if (rename(cpath_export(image_create_file), cpath_export(image_rename_file)) != 0) {
				log_std(("WARNING:%s: error moving file %s to %s\n", user_name_get().c_str(), cpath_export(image_create_file), cpath_export(image_rename_file)));
			}
		}
		if (access(cpath_export(clip_create_file), F_OK) == 0) {
			if (target_mkdir(cpath_export(snapshot_rename_dir)) != 0) {
				log_std(("WARNING:%s: error creating dir %s\n", user_name_get().c_str(), cpath_export(snapshot_rename_dir)));
			}
			if (rename(cpath_export(clip_create_file), cpath_export(clip_rename_file)) != 0) {
				log_std(("WARNING:%s: error moving file %s to %s\n", user_name_get().c_str(), cpath_export(clip_create_file), cpath_export(clip_rename_file)));
			}
		}
		if (access(cpath_export(sound_create_file), F_OK) == 0) {
			if (target_mkdir(cpath_export(snapshot_rename_dir)) != 0) {
				log_std(("WARNING:%s: error creating dir %s\n", user_name_get().c_str(), cpath_export(snapshot_rename_dir)));
			}
			if (rename(cpath_export(sound_create_file), cpath_export(sound_rename_file)) != 0) {
				log_std(("WARNING:%s: error moving file %s to %s\n", user_name_get().c_str(), cpath_export(sound_create_file), cpath_export(sound_rename_file)));
			}
		}
	}

	return true;
}

//---------------------------------------------------------------------------
// raine_info

raine_info::raine_info(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: emulator(Aname, Aexe_path, Acmd_arg)
{
	exclude_clone_orig = exclude;
	exclude_bad_orig = exclude;
	exclude_vertical_orig = include;
}

int raine_info::attrib_run(int x, int y)
{
	choice_bag ch;

	ch.insert(ch.end(), choice("Present or Missing", " Only\tPresent", " Only\tMissing", exclude_missing_effective, 0));
	ch.insert(ch.end(), choice("Unique or Duplicate", " Only\tUnique", " Only\tDuplicate", exclude_duplicate_effective, 0));
	ch.insert(ch.end(), choice("Working or Preliminary", " Only\tWorking", " Only\tPreliminary", exclude_bad_effective, 0));
	ch.insert(ch.end(), choice("Parent or Clone", " Only\tParent", " Only\tClone", exclude_clone_effective, 0));
	ch.insert(ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", exclude_vertical_effective, 0));

	choice_bag::iterator i = ch.begin();

	int key = ch.run(" " + user_name_get() + " Filters", x, y, ATTRIB_CHOICE_DX, i);

	if (key == EVENT_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_duplicate_effective = ch[1].tristate_get();
		exclude_bad_effective = ch[2].tristate_get();
		exclude_clone_effective = ch[3].tristate_get();
		exclude_vertical_effective = ch[4].tristate_get();
	}

	return key;
}

void raine_info::attrib_load()
{
	emulator::attrib_load();
	exclude_clone_effective = exclude_clone_orig;
	exclude_bad_effective = exclude_bad_orig;
	exclude_vertical_effective = exclude_vertical_orig;
}

void raine_info::attrib_save()
{
	emulator::attrib_save();
	exclude_clone_orig = exclude_clone_effective;
	exclude_bad_orig = exclude_bad_effective;
	exclude_vertical_orig = exclude_vertical_effective;
}

bool raine_info::attrib_set(const std::string& value0, const std::string& value1)
{
	if (emulator::attrib_set(value0, value1))
		return true;

	if (value0 == "clone") {
		if (!tristate(exclude_clone_orig, value1))
			return false;
	} else if (value0 == "bad") {
		if (!tristate(exclude_bad_orig, value1))
			return false;
	} else if (value0 == "vertical") {
		if (!tristate(exclude_vertical_orig, value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void raine_info::attrib_get(adv_conf* config_context, const char* section, const char* tag)
{
	emulator::attrib_get(config_context, section, tag);
	conf_string_set(config_context, section, tag, attrib_compile("clone", tristate(exclude_clone_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("bad", tristate(exclude_bad_orig)).c_str());
	conf_string_set(config_context, section, tag, attrib_compile("vertical", tristate(exclude_vertical_orig)).c_str());
}

bool raine_info::filter(const game& g) const
{
	if (!emulator::filter(g))
		return false;

	if (exclude_clone_effective == exclude && g.parent_get() != 0)
		return false;
	if (exclude_clone_effective == exclude_not && g.parent_get() == 0)
		return false;

	bool good;
	if (exclude_clone_effective == exclude)
		good = g.play_best_get() != play_preliminary;
	else
		good = g.play_get() != play_preliminary;
	if (exclude_bad_effective == exclude && !good)
		return false;
	if (exclude_bad_effective == exclude_not && good)
		return false;

	if (exclude_vertical_effective == exclude && g.flag_get(flag_derived_vertical))
		return false;
	if (exclude_vertical_effective == exclude_not && !g.flag_get(flag_derived_vertical))
		return false;

	return true;
}

void raine_info::cache(const game_set& gar, const game& g) const
{
	emulator::cache(gar, g);
}

bool raine_info::tree_get() const
{
	return exclude_clone_effective == exclude;
}

bool raine_info::load_data(const game_set& gar)
{
	return true;
}

bool raine_info::load_info(game_set& gar)
{
	info_t token = info_token_get();
	while (token != info_eof) {
		if (token != info_symbol) return false;
		bool isgame = strcmp(info_text_get(), "game") == 0;
		if (isgame) {
			if (info_token_get() != info_open) return false;
			game g;
			g.emulator_set(this);
			token = info_token_get();
			while (token != info_close) {
				if (token != info_symbol)
					return false;
				if (strcmp(info_text_get(), "name") == 0) {
					if (info_token_get() != info_symbol) return false;
					g.name_set(user_name_get() + "/" + info_text_get());
				} else if (strcmp(info_text_get(), "description") == 0) {
					if (info_token_get() != info_string) return false;
					g.auto_description_set(info_text_get());
				} else if (strcmp(info_text_get(), "manufacturer") == 0) {
					if (info_token_get() != info_string) return false;
					g.manufacturer_set(info_text_get());
				} else if (strcmp(info_text_get(), "year") == 0) {
					if (info_token_get() != info_symbol) return false;
					g.year_set(info_text_get());
				} else if (strcmp(info_text_get(), "cloneof") == 0) {
					if (info_token_get() != info_symbol) return false;
					g.cloneof_set(user_name_get() + "/" + info_text_get());
				} else if (strcmp(info_text_get(), "romof") == 0) {
					if (info_token_get() != info_symbol) return false;
					g.romof_set(user_name_get() + "/" + info_text_get());
				} else if (strcmp(info_text_get(), "driver") == 0) {
					if (info_token_get() != info_open) return false;
					token = info_token_get();
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(), "status") == 0) {
							if (info_token_get() != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary") == 0)
								g.play_set(play_preliminary);
						} else if (strcmp(info_text_get(), "color") == 0) {
							if (info_token_get() != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary") == 0)
								g.play_set(play_preliminary);
						} else if (strcmp(info_text_get(), "sound") == 0) {
							if (info_token_get() != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary") == 0)
								g.play_set(play_preliminary);
						} else {
							if (info_skip_value() == info_error) return false;
						}
						token = info_token_get();
					}
				} else if (strcmp(info_text_get(), "video") == 0) {
					if (info_token_get() != info_open) return false;
					token = info_token_get();
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(), "orientation") == 0) {
							if (info_token_get() != info_symbol) return false;
							g.flag_set(strcmp(info_text_get(), "vertical") == 0, flag_derived_vertical);
						} else if (strcmp(info_text_get(), "x") == 0) {
							if (info_token_get() != info_symbol) return false;
							g.sizex_set(atoi(info_text_get()));
						} else if (strcmp(info_text_get(), "y") == 0) {
							if (info_token_get() != info_symbol) return false;
							g.sizey_set(atoi(info_text_get()));
						} else if (strcmp(info_text_get(), "aspectx") == 0) {
							if (info_token_get() != info_symbol) return false;
							g.aspectx_set(atoi(info_text_get()));
						} else if (strcmp(info_text_get(), "aspecty") == 0) {
							if (info_token_get() != info_symbol) return false;
							g.aspecty_set(atoi(info_text_get()));
						} else {
							if (info_skip_value() == info_error) return false;
						}
						token = info_token_get();
					}
				} else if (strcmp(info_text_get(), "rom") == 0) {
					unsigned size = 0;
					bool merge = false;
					if (info_token_get() != info_open) return false;
					token = info_token_get();
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(), "size") == 0) {
							if (info_token_get() != info_symbol) return false;
							size = atoi(info_text_get());
						} else if (strcmp(info_text_get(), "merge") == 0) {
							if (info_token_get() != info_symbol) return false;
							merge = true;
						} else {
							if (info_skip_value() == info_error) return false;
						}
						token = info_token_get();
					}
					if (!merge)
						g.size_set(g.size_get() + size);
				} else {
					if (info_skip_value() == info_error) return false;
				}
				token = info_token_get();
			}
			gar.insert(g);
		} else {
			if (info_skip_value() == info_error)
				return false;
		}
		token = info_token_get();
	}

	return true;
}

bool raine_info::load_game(game_set& gar, bool quiet)
{
	struct stat st_info;
	struct stat st_mame;
	int err_info;
	int err_exe;

	string info_file = path_abs(path_import(file_config_file_home((user_name_get() + ".lst").c_str())), dir_cwd());

	err_info = stat(cpath_export(info_file), &st_info);
	err_exe = stat(cpath_export(config_exe_path_get()), &st_mame);

	if (file_ext(config_exe_path_get()) != ".bat"
		&& err_exe == 0
		&& (err_info != 0 || st_info.st_mtime < st_mame.st_mtime)
		&& (err_info != 0 || access(cpath_export(info_file), W_OK) == 0)
	) {
		target_out("Updating the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));

		const char* argv[TARGET_MAXARG];
		unsigned argc = 0;

		argv[argc++] = strdup(cpath_export(config_exe_path_get()));
		argv[argc++] = strdup("-gamenfo");
		argv[argc] = 0;

		int r = target_spawn_redirect(argv[0], argv, cpath_export(info_file));

		for (int i = 0; i < argc; ++i)
			free(const_cast<char*>(argv[i]));

		bool result = spawn_check(r, false);
		if (!result)
			return false;
	}

	ifstream f(cpath_export(info_file), ios::in | ios::binary);
	if (!f) {
		target_err("Error opening the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));
		target_err("Try running manually the command: '%s -gameinfo > %s'.\n", user_exe_path.c_str(), cpath_export(info_file));
		return false;
	}
	info_init(info_ext_get, info_ext_unget, &f);
	if (!load_info(gar)) {
		info_done();
		f.close();
		target_err("Error reading the '%s' information from file '%s' at row %d column %d.\n", user_name_get().c_str(), cpath_export(info_file), info_row_get() + 1, info_col_get() + 1);
		return false;
	}
	info_done();
	f.close();

	return true;
}

bool raine_info::load_software(game_set&, bool quiet)
{
	return true;
}

//---------------------------------------------------------------------------
// draine

#if !USE_NIX
draine::draine(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: raine_info(Aname, Aexe_path, Acmd_arg)
{
}

string draine::type_get() const
{
	return "draine";
}

bool draine::load_cfg(const game_set& gar, bool quiet)
{
	const char* s;
	adv_conf* context;

	string ref_dir = exe_dir_get();

	string config_file = slash_add(ref_dir) + "config/raine.cfg";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rom_dir_0");
	conf_string_register(context, "rom_dir_1");
	conf_string_register(context, "rom_dir_2");
	conf_string_register(context, "rom_dir_3");
	conf_string_register(context, "screenshots");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 0, 0, 0, 0, 0) != 0) {
		target_err("Error loading the configuration file %s.\n", cpath_export(config_file));
		conf_done(context);
		return false;
	}

	emu_rom_path = "";
	if (conf_string_section_get(context, "Directories", "rom_dir_0", &s) == 0) {
		emu_rom_path = dir_cat(emu_rom_path, list_abs(list_import(s), ref_dir));
	}
	if (conf_string_section_get(context, "Directories", "rom_dir_1", &s) == 0) {
		emu_rom_path = dir_cat(emu_rom_path, list_abs(list_import(s), ref_dir));
	}
	if (conf_string_section_get(context, "Directories", "rom_dir_2", &s) == 0) {
		emu_rom_path = dir_cat(emu_rom_path, list_abs(list_import(s), ref_dir));
	}
	if (conf_string_section_get(context, "Directories", "rom_dir_3", &s) == 0) {
		emu_rom_path = dir_cat(emu_rom_path, list_abs(list_import(s), ref_dir));
	}
	if (emu_rom_path.length() == 0) {
		emu_rom_path = list_abs(list_import("roms"), ref_dir);
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "Directories", "screenshots", &s) == 0) {
		emu_snap_path = list_abs(list_import(s), ref_dir);
	} else {
		emu_snap_path = list_abs(list_import("screens"), ref_dir);
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path)));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path)));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path)));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), ref_dir));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path), ref_dir));
	config_icon_path = list_abs(list_import(user_icon_path), ref_dir);
	config_flyer_path = list_abs(list_import(user_flyer_path), ref_dir);
	config_cabinet_path = list_abs(list_import(user_cabinet_path), ref_dir);
	config_marquee_path = list_abs(list_import(user_marquee_path), ref_dir);
	config_title_path = list_abs(list_import(user_title_path), ref_dir);

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path)));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path)));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path)));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path)));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path)));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path)));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path)));

	scan_dirlist(gar, config_rom_path_get(), quiet);

	return true;
}

bool draine::run(const game& g, const game* bios, unsigned orientation, bool set_difficulty, difficulty_t difficulty, bool set_attenuation, int attenuation, bool ignore_error) const
{
	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(config_exe_path_get()));
	argv[argc++] = strdup("-game");
	argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	argv[argc++] = strdup("-nogui");
	argc = compile(g, argv, argc, user_cmd_arg, orientation);
	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration, exe_dir_get(), argc, argv, ignore_error);

	for (int i = 0; i < argc; ++i)
		free(const_cast<char*>(argv[i]));

	if (ret) {
		g.time_set(g.time_get() + duration);
		g.session_set(g.session_get() + 1);
	}

	return ret;
}
#endif

//---------------------------------------------------------------------------
// generic

generic::generic(const string& Aname, const string& Aexe_path, const string& Acmd_arg)
	: emulator(Aname, Aexe_path, Acmd_arg)
{
}

int generic::attrib_run(int x, int y)
{
	choice_bag ch;

	ch.insert(ch.end(), choice("Present or Missing", " Only\tPresent", " Only\tMissing", exclude_missing_effective, 0));
	ch.insert(ch.end(), choice("Unique or Duplicate", " Only\tUnique", " Only\tDuplicate", exclude_duplicate_effective, 0));

	choice_bag::iterator i = ch.begin();

	int key = ch.run(" " + user_name_get() + " Filters", x, y, ATTRIB_CHOICE_DX, i);

	if (key == EVENT_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_duplicate_effective = ch[1].tristate_get();
	}

	return key;
}

bool generic::tree_get() const
{
	return false;
}

string generic::type_get() const
{
	return "generic";
}

bool generic::load_data(const game_set& gar)
{
	return true;
}

bool generic::load_cfg(const game_set& gar, bool quiet)
{
	string ref_dir = exe_dir_get();

	config_rom_path = list_abs(list_import(user_rom_path), ref_dir);
	config_alts_path = list_abs(list_import(user_alts_path), ref_dir);
	config_flyer_path = list_abs(list_import(user_flyer_path), ref_dir);
	config_cabinet_path = list_abs(list_import(user_cabinet_path), ref_dir);
	config_marquee_path = list_abs(list_import(user_marquee_path), ref_dir);
	config_title_path = list_abs(list_import(user_title_path), ref_dir);
	config_icon_path = list_abs(list_import(user_icon_path), ref_dir);

	// no scan done, the zip path is already added in the load_game() call

	return true;
}

bool generic::load_info(game_set& gar)
{
	info_t token = info_token_get();
	while (token != info_eof) {
		if (token != info_symbol) return false;
		bool isgame = strcmp(info_text_get(), "game") == 0;
		if (isgame) {
			if (info_token_get() != info_open) return false;

			string name;
			string description;
			string manufacturer;
			string year;
			string cloneof;

			token = info_token_get();
			while (token != info_close) {
				if (token != info_symbol)
					return false;
				if (strcmp(info_text_get(), "name") == 0) {
					if (info_token_get() != info_symbol) return false;
					name = user_name_get() + "/" + info_text_get();
				} else if (strcmp(info_text_get(), "description") == 0) {
					if (info_token_get() != info_string) return false;
					description = info_text_get();
				} else if (strcmp(info_text_get(), "manufacturer") == 0) {
					if (info_token_get() != info_string) return false;
					manufacturer = info_text_get();
				} else if (strcmp(info_text_get(), "year") == 0) {
					if (info_token_get() != info_symbol) return false;
					year = info_text_get();
				} else if (strcmp(info_text_get(), "cloneof") == 0) {
					if (info_token_get() != info_symbol) return false;
					cloneof = user_name_get() + "/" + info_text_get();
				} else {
					if (info_skip_value() == info_error) return false;
				}
				token = info_token_get();
			}
			if (name.length()) {
				game g;
				g.emulator_set(this);
				g.name_set(name);
				game_set::const_iterator i = gar.find(g);
				if (i != gar.end()) {
					if (description.length())
						i->auto_description_set(description);
					if (manufacturer.length())
						(const_cast<game&>(*i)).manufacturer_set(manufacturer);
					if (year.length())
						(const_cast<game&>(*i)).year_set(year);
					if (cloneof.length())
						(const_cast<game&>(*i)).cloneof_set(cloneof);
				} else {
					log_std(("%s: ignoring game info %s\n", user_name_get().c_str(), name.c_str()));
				}
			}
		} else {
			if (info_skip_value() == info_error)
				return false;
		}
		token = info_token_get();
	}

	return true;
}

bool generic::load_game(game_set& gar, bool quiet)
{
	load_dirlist(gar, list_abs(list_import(user_rom_path), exe_dir_get()), list_import(user_rom_filter), quiet);

	string info_file = path_abs(path_import(file_config_file_home((user_name_get() + ".lst").c_str())), dir_cwd());

	if (access(cpath_export(info_file), R_OK) == 0) {
		log_std(("%s: loading info %s\n", user_name_get().c_str(), cpath_export(info_file)));

		ifstream f(cpath_export(info_file), ios::in | ios::binary);
		if (!f) {
			target_err("Error opening the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));
			return false;
		}
		info_init(info_ext_get, info_ext_unget, &f);
		if (!load_info(gar)) {
			info_done();
			f.close();
			target_err("Error reading the '%s' information from file '%s' at row %d column %d.\n", user_name_get().c_str(), cpath_export(info_file), info_row_get() + 1, info_col_get() + 1);
			return false;
		}
		info_done();
		f.close();
	} else {
		log_std(("%s: missing info %s\n", user_name_get().c_str(), cpath_export(info_file)));
	}

	return true;
}

bool generic::load_software(game_set&, bool quiet)
{
	return true;
}

bool generic::is_empty() const
{
	// allow emulator without roms info definition
	return false;
}

bool generic::is_present() const
{
	// if empty enable always, this allow to create fake listing of mp3 and mng without a real emulator
	if (user_exe_path.length() == 0)
		return true;

	return emulator::is_present();
}

bool generic::is_runnable() const
{
	// if empty it isn't runnable
	if (user_exe_path.length() == 0)
		return false;

	return emulator::is_runnable();
}

bool generic::run(const game& g, const game* bios, unsigned orientation, bool set_difficulty, difficulty_t difficulty, bool set_attenuation, int attenuation, bool ignore_error) const
{
	// if empty don't run
	if (user_exe_path.length() == 0)
		return false;

	return emulator::run(g, bios, orientation, set_difficulty, difficulty, set_attenuation, attenuation, ignore_error);
}

