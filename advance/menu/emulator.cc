/*
 * This file is part of the Advance project.
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

#include "text.h"
#include "emulator.h"
#include "common.h"
#include "menu.h"
#include "bitmap.h"
#include "game.h"
#include "readinfo.h"
#include "unzip.h"
#include "target.h"
#include "log.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <fstream>

using namespace std;

//---------------------------------------------------------------------------
// utils

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

static bool spawn_check(int r, bool ignore_error) {
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

#define ATTRIB_CHOICE_X text_dx_get()/8 + 2
#define ATTRIB_CHOICE_Y text_dy_get()/5 + 2
#define ATTRIB_CHOICE_DX 20*text_font_dx_get()

emulator::emulator(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	state(false), name(Aname) {
	char path[FILE_MAXPATH];

	user_exe_path = Aexe_path;
	user_cmd_arg = Acmd_arg;

	if (user_exe_path.length()) {
		// search the emulator in the system path
		if (target_search(path, FILE_MAXPATH, user_exe_path.c_str()) != 0) {
			// if not found assume in the current dir
			config_exe_path = path_abs(path_import(user_exe_path),dir_cwd());
		} else {
			// use the file found
			config_exe_path = path_import(path);
		}
	} else {
		// no emulator specified
		config_exe_path = "";
	}

	exclude_missing_orig = exclude;
}

emulator::~emulator() {
}

string emulator::attrib_compile(const string& value0, const string& value1) {
	string name = "\"" + user_name_get() + "\" " + value0 + " " + value1;
	return name;
}

void emulator::attrib_load() {
	exclude_missing_effective = exclude_missing_orig;
}

void emulator::attrib_save() {
	exclude_missing_orig = exclude_missing_effective;
}

bool emulator::attrib_set(const string& value0, const string& value1) {
	if (value0 == "missing") {
		if (!tristate(exclude_missing_orig,value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void emulator::attrib_get(adv_conf* config_context, const char* section, const char* tag) {
	conf_string_set(config_context,section,tag, attrib_compile("missing", tristate(exclude_missing_orig)).c_str() );
}

bool emulator::filter(const game& g) const {
	if (exclude_missing_effective == exclude && !g.present_tree_get())
		return false;
	if (exclude_missing_effective == exclude_not && g.present_tree_get())
		return false;
	return true;
}

void emulator::cache(const game_set& gar, const game& g) const {
}

bool emulator::is_present() const {
	// if empty disable always
	if (user_exe_path.length() == 0)
		return false;

	// check that the emulator file exist and it's executable
	if (access(cpath_export(config_exe_path_get()),X_OK)!=0)
		return false;

	return true;
}

bool emulator::is_runnable() const {
	return true;
}

string emulator::exe_dir_get() const {
	string dir = file_dir(config_exe_path_get());

	if (dir.length() == 1 && dir[0]=='/')
		return dir; // save the root slash
	else
		return slash_remove(dir);
}

void emulator::scan_game(const game_set& gar, const string& path, const string& name) {
	game_set::const_iterator i = gar.find(game(name));
	if (i!=gar.end())
		i->rom_zip_set_insert(path);
}

void emulator::scan_dir(const game_set& gar, const string& dir) {
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		target_err("Error in '%s' opening roms directory '%s'.\n", user_name_get().c_str(), cpath_export(dir));
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		if (file_ext(file) == ".zip") {
			scan_game(gar,dir + "/" + file, user_name_get() + "/" + file_basename(file));
		}
	}

	closedir(d);
}

void emulator::scan_dirlist(const game_set& gar, const string& dirlist) {
	unsigned i = 0;
	while (i<dirlist.length()) {
		unsigned end = dirlist.find(':',i);
		if (end==string::npos) {
			scan_dir(gar, string( dirlist, i ));
			i = dirlist.size();
		} else {
			scan_dir(gar, string( dirlist, i, end-i ));
			i = end + 1;
		}
	}
}

void emulator::load_dir(game_set& gar, const string& dir, const string& filterlist) {
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		target_err("Error in '%s' opening roms directory '%s'.\n", user_name_get().c_str(), cpath_export(dir));
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		if (is_globlist(file,filterlist)) {
			struct stat st;
			if (stat(cpath_export(dir + "/" + file),&st)==0) {
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

void emulator::load_dirlist(game_set& gar, const string& dirlist, const string& filterlist) {
	unsigned i = 0;
	while (i<dirlist.length()) {
		unsigned end = dirlist.find(':',i);
		if (end==string::npos) {
			load_dir(gar, string( dirlist, i ), filterlist);
			i = dirlist.size();
		} else {
			load_dir(gar, string( dirlist, i, end-i ), filterlist);
			i = end + 1;
		}
	}
}

bool emulator::run_process(time_t& duration, const string& dir, int argc, const char** argv, bool ignore_error) const {
	time_t start,stop;

	string olddir = dir_cwd();

	if (chdir(cpath_export(dir))!=0) {
		log_std(("menu: run error chdir '%s'\n",cpath_export(dir)));
		return false;
	}

	char cmdline[TARGET_MAXCMD];

	cmdline[0] = 0;
	for(int i=0;i<argc;++i) {
		if (i!=0)
			strcat(cmdline," ");
		if (strchr(argv[i],' ')) {
			strcat(cmdline,"\"");
			strcat(cmdline,argv[i]);
			strcat(cmdline,"\"");
		} else {
			strcat(cmdline,argv[i]);
		}
	}

	log_std(("menu: run '%s'\n",cmdline));

	start = time(0);

	int r = target_spawn(argv[0],argv);

	stop = time(0);

	if (stop - start > 8)
		duration = stop - start;
	else
		duration = 0;

	bool result = spawn_check(r,ignore_error);

	text_idle_time_reset();

	// if returned with success
	if (result)
		text_idle_repeat_reset();

	chdir(cpath_export(olddir));

	return result;
}

unsigned emulator::compile(const game& g, const char** argv, unsigned argc, const string& list, unsigned orientation) const {
	int pos = 0;
	token_skip(list,pos," \t");

	while (pos < list.length()) {
		string arg = token_get(list,pos," \t");

		int ostart = arg.find("%o[");
		if (ostart != string::npos) {
			int opos = ostart + 3;
			string r0, r90, r180, r270;

			r0 = token_get(arg,opos,",]");
			if (opos < arg.length() && arg[opos]==',')
				++opos;
			r90 = token_get(arg,opos,",]");
			if (opos < arg.length() && arg[opos]==',')
				++opos;
			r180 = token_get(arg,opos,",]");
			if (opos < arg.length() && arg[opos]==',')
				++opos;
			r270 = token_get(arg,opos,"]");

			if (opos < arg.length() && arg[opos]==']') {
				++opos;
				string o;
				switch (orientation) {
				case TEXT_ORIENTATION_ROT0 : o = r0; break;
				case TEXT_ORIENTATION_ROT90 : o = r90; break;
				case TEXT_ORIENTATION_ROT180 : o = r180; break;
				case TEXT_ORIENTATION_ROT270 : o = r270; break;
				}
				arg.erase(ostart,opos - ostart);
				arg.insert(ostart,o);
			}
		}

		arg = subs(arg,"%s",g.name_without_emulator_get());
		const path_container& pc = g.rom_zip_set_get();

		if (pc.size()) {
			string path = *pc.begin();
			arg = subs(arg,"%p",path_export(path));
			arg = subs(arg,"%f",path_export(file_file(path)));
		}

		if (arg.length())
			argv[argc++] = strdup(arg.c_str());

		token_skip(list,pos," \t");
	}

	return argc;
}

bool emulator::run(const game& g, unsigned orientation, bool ignore_error) const {
	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(file_file(config_exe_path_get())));
	argc = compile(g,argv,argc,user_cmd_arg,orientation);
	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret)
		g.time_set( g.time_get() + duration );

	return ret;
}

unsigned emulator::preview_set(game_set& gar) const {
	unsigned preview = 0;

	// search the previews in the software directory
	if (software_path_get().length()) {
		gar.preview_software_list_set(config_alts_path_get(),user_name_get(),&game::preview_snap_set_ifmissing,".png",".pcx");
		gar.preview_software_list_set(config_alts_path_get(),user_name_get(),&game::preview_clip_set_ifmissing,".mng","");
		gar.preview_software_list_set(config_alts_path_get(),user_name_get(),&game::preview_sound_set_ifmissing,".mp3",".wav");
	}

	// search the previews in the root directory
	gar.preview_list_set(config_alts_path_get(),user_name_get(),&game::preview_snap_set_ifmissing,".png",".pcx");
	gar.preview_list_set(config_alts_path_get(),user_name_get(),&game::preview_clip_set_ifmissing,".mng","");
	gar.preview_list_set(config_alts_path_get(),user_name_get(),&game::preview_sound_set_ifmissing,".mp3",".wav");

	if (gar.preview_list_set(config_icon_path_get(),user_name_get(),&game::preview_icon_set_ifmissing,".ico",""))
		preview |= preview_icon;
	if (gar.preview_list_set(config_flyer_path_get(),user_name_get(),&game::preview_flyer_set_ifmissing,".png",".pcx"))
		preview |= preview_flyer;
	if (gar.preview_list_set(config_cabinet_path_get(),user_name_get(),&game::preview_cabinet_set_ifmissing,".png",".pcx"))
		preview |= preview_cabinet;
	if (gar.preview_list_set(config_marquee_path_get(),user_name_get(),&game::preview_marquee_set_ifmissing,".png",".pcx"))
		preview |= preview_marquee;
	if (gar.preview_list_set(config_title_path_get(),user_name_get(),&game::preview_title_set_ifmissing,".png",".pcx"))
		preview |= preview_title;

	return preview;
}

void emulator::update(const game& g) const {
	// update always the preview
	if (!g.preview_software_list_set(config_alts_path_get(),&game::preview_snap_set,".png",".pcx")
		&& !g.preview_list_set(config_alts_path_get(),&game::preview_snap_set,".png",".pcx"))
		g.preview_snap_set(resource());

	if (!g.preview_software_list_set(config_alts_path_get(),&game::preview_clip_set,".mng","")
		&& !g.preview_list_set(config_alts_path_get(),&game::preview_clip_set,".mng",""))
		g.preview_clip_set(resource());

	if (!g.preview_software_list_set(config_alts_path_get(),&game::preview_sound_set,".mp3",".wav")
		&& !g.preview_list_set(config_alts_path_get(),&game::preview_sound_set,".mp3",".wav"))
		g.preview_sound_set(resource());
}

bool emulator::validate_config_file(const string& file) const {
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
	emulator(Aname,Aexe_path,Acmd_arg) {

	exclude_clone_orig = exclude;
	exclude_bad_orig = exclude;
	exclude_vector_orig = include;
	exclude_vertical_orig = include;
}

void mame_info::attrib_load() {
	emulator::attrib_load();
	exclude_clone_effective = exclude_clone_orig;
	exclude_bad_effective = exclude_bad_orig;
	exclude_vector_effective = exclude_vector_orig;
	exclude_vertical_effective = exclude_vertical_orig;
}

void mame_info::attrib_save() {
	emulator::attrib_save();
	exclude_clone_orig = exclude_clone_effective;
	exclude_bad_orig = exclude_bad_effective;
	exclude_vector_orig = exclude_vector_effective;
	exclude_vertical_orig = exclude_vertical_effective;
}

bool mame_info::attrib_set(const std::string& value0, const std::string& value1) {
	if (emulator::attrib_set(value0, value1))
		return true;

	if (value0 == "clone") {
		if (!tristate(exclude_clone_orig,value1))
			return false;
	} else if (value0 == "bad") {
		if (!tristate(exclude_bad_orig,value1))
			return false;
	} else if (value0 == "vector") {
		if (!tristate(exclude_vector_orig,value1))
			return false;
	} else if (value0 == "vertical") {
		if (!tristate(exclude_vertical_orig,value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void mame_info::attrib_get(adv_conf* config_context, const char* section, const char* tag) {
	emulator::attrib_get(config_context, section, tag);
	conf_string_set(config_context,section,tag, attrib_compile("clone", tristate(exclude_clone_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("bad", tristate(exclude_bad_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("vector", tristate(exclude_vector_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("vertical", tristate(exclude_vertical_orig)).c_str() );
}

bool mame_info::filter(const game& g) const {
	if (!emulator::filter(g))
		return false;

	// use always the bios in the check for the mess software
	const game& bios = g.bios_get();

	// softwares of clones are always listed
	if (!g.software_get()) {
		if (exclude_clone_effective == exclude && bios.parent_get()!=0)
			return false;
	}
	if (exclude_clone_effective == exclude_not && bios.parent_get()==0)
		return false;

	bool good;
	if (exclude_clone_effective == exclude)
		good = bios.play_best_get() == play_perfect;
	else
		good = bios.play_get() == play_perfect;
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

void mame_info::cache(const game_set& gar, const game& g) const {
	emulator::cache(gar,g);
}

bool mame_info::tree_get() const {
	return exclude_clone_effective == exclude;
}

bool mame_info::internal_load(FILE* f, game_set& gar) {
	info_t token = info_token_get(f);
	while (token!=info_eof) {
		if (token != info_symbol) return false;
		bool isresource = strcmp(info_text_get(),"resource")==0;
		bool isgame = strcmp(info_text_get(),"game")==0 || strcmp(info_text_get(),"machine")==0;
		if (isgame || isresource) {
			if (info_token_get(f) != info_open) return false;
			game g;
			g.emulator_set(this);
			g.flag_set(isresource, flag_derived_resource);
			token = info_token_get(f);
			while (token != info_close) {
				if (token != info_symbol)
					return false;
				if (strcmp(info_text_get(),"name")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.name_set( user_name_get() + "/" + info_text_get() );
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
					g.cloneof_set( user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"romof")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.romof_set( user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"driver")==0) {
					if (info_token_get(f) != info_open)  return false;
					token = info_token_get(f);
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(),"status")==0) {
							if (info_token_get(f) != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary")==0)
								g.play_set(play_not);
						} else if (strcmp(info_text_get(),"color")==0) {
							if (info_token_get(f) != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary")==0)
								if (g.play_get() < play_major)
									g.play_set(play_major);
						} else if (strcmp(info_text_get(),"sound")==0) {
							if (info_token_get(f) != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary")==0)
								if (g.play_get() < play_minor)
									g.play_set(play_minor);
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
							g.flag_set( strcmp( info_text_get(),"vector") == 0, flag_derived_vector );
						} else if (strcmp(info_text_get(),"orientation")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.flag_set( strcmp( info_text_get(),"vertical") == 0, flag_derived_vertical );
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
			gar.insert( g );
		} else {
			if (info_skip_value(f) == info_error)
				return false;
		}
		token = info_token_get(f);
	}

	return true;
}

bool mame_info::load_game(game_set& gar) {
	struct stat st_info;
	struct stat st_mame;
	int err_info;
	int err_exe;

	string info_file = path_abs(path_import(file_config_file_home( (user_name_get() + ".lst").c_str())),dir_cwd());

	err_info = stat(cpath_export(info_file), &st_info);
	err_exe = stat(cpath_export(config_exe_path_get()), &st_mame);

	if (file_ext(config_exe_path_get()) != ".bat"
		&& err_exe==0
		&& (err_info!=0 || st_info.st_mtime < st_mame.st_mtime || st_info.st_size == 0)) {
		target_out("Updating the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));

		char cmd[TARGET_MAXCMD];
		sprintf(cmd,"%s -listinfo > %s", cpath_export(config_exe_path_get()),cpath_export(info_file));

		int r = target_system(cmd);

		bool result = spawn_check(r,false);
		if (!result)
			return false;
	}

	FILE* f = fopen(cpath_export(info_file),"rt");
	if (!f) {
		target_err("Error opening the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));
		target_err("Try running manually the command: '%s -listinfo > %s'.\n", user_exe_path.c_str(), cpath_export(info_file));
		return false;
	}
	info_init();
	if (!internal_load(f,gar)) {
		info_done();
		target_err("Error reading the '%s' information from file '%s' at row %d column %d.\n", user_name_get().c_str(), cpath_export(info_file), info_row_get()+1, info_col_get()+1);
		return false;
	}
	info_done();

	fclose(f);

	return true;
}

void mame_info::update(const game& g) const {
	emulator::update(g);
}

//---------------------------------------------------------------------------
// mame_mame

static bool fread_uint32be(unsigned& v, FILE* f) {
	v = 0;
	for(unsigned i=0;i<4;++i) {
		unsigned char c;
		if (fread(&c,1,1,f)!=1)
			return false;
		v = (v << 8) + c;
	}
	return true;
}

static bool fread_uint16be(unsigned& v, FILE* f) {
	v = 0;
	for(unsigned i=0;i<2;++i) {
		unsigned char c;
		if (fread(&c,1,1,f)!=1)
			return false;
		v = (v << 8) + c;
	}
	return true;
}

static bool fskip(unsigned size, FILE* f) {
	return fseek(f,size,SEEK_CUR)==0;
}

mame_mame::mame_mame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_info(Aname,Aexe_path,Acmd_arg) {

	exclude_neogeo_orig = include;
	exclude_deco_orig = exclude;
	exclude_playchoice_orig = exclude;
}

void mame_mame::attrib_run() {
	choice_bag ch;

	ch.insert( ch.end(), choice("Present or Missing"," Only\tPresent"," Only\tMissing", exclude_missing_effective, 0) );
	ch.insert( ch.end(), choice("Working or Preliminary"," Only\tWorking"," Only\tPreliminary", exclude_bad_effective, 0) );
	ch.insert( ch.end(), choice("Parent or Clone"," Only\tParent"," Only\tClone", exclude_clone_effective, 0) );
	ch.insert( ch.end(), choice("Any Screen Type", " Only\tScreen Raster", " Only\tScreen Vector", exclude_vector_effective, 0) );
	ch.insert( ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", exclude_vertical_effective, 0) );
	ch.insert( ch.end(), choice("Neogeo", exclude_neogeo_effective, 0) );
	ch.insert( ch.end(), choice("Cassette", exclude_deco_effective, 0) );
	ch.insert( ch.end(), choice("PlayChoice-10", exclude_playchoice_effective, 0) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" " + user_name_get() + " Attrib", ATTRIB_CHOICE_X, ATTRIB_CHOICE_Y, ATTRIB_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_bad_effective = ch[1].tristate_get();
		exclude_clone_effective = ch[2].tristate_get();
		exclude_vector_effective = ch[3].tristate_get();
		exclude_vertical_effective = ch[4].tristate_get();
		exclude_neogeo_effective = ch[5].tristate_get();
		exclude_deco_effective = ch[6].tristate_get();
		exclude_playchoice_effective = ch[7].tristate_get();
	}
}

void mame_mame::attrib_load() {
	mame_info::attrib_load();

	exclude_neogeo_effective = exclude_neogeo_orig;
	exclude_deco_effective = exclude_deco_orig;
	exclude_playchoice_effective = exclude_playchoice_orig;
}

void mame_mame::attrib_save() {
	mame_info::attrib_save();

	exclude_neogeo_orig = exclude_neogeo_effective;
	exclude_deco_orig = exclude_deco_effective;
	exclude_playchoice_orig = exclude_playchoice_effective;
}

bool mame_mame::attrib_set(const std::string& value0, const std::string& value1) {
	if (mame_info::attrib_set(value0, value1))
		return true;

	if (value0 == "neogeo") {
		if (!tristate(exclude_neogeo_orig,value1))
			return false;
	} else if (value0 == "deco") {
		if (!tristate(exclude_deco_orig,value1))
			return false;
	} else if (value0 == "playchoice") {
		if (!tristate(exclude_playchoice_orig,value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void mame_mame::attrib_get(adv_conf* config_context, const char* section, const char* tag) {
	mame_info::attrib_get(config_context, section, tag);

	conf_string_set(config_context,section,tag, attrib_compile("neogeo", tristate(exclude_neogeo_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("deco", tristate(exclude_deco_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("playchoice", tristate(exclude_playchoice_orig)).c_str() );
}

bool mame_mame::filter(const game& g) const {
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

void mame_mame::cache(const game_set& gar, const game& g) const {
	mame_info::cache(gar,g);

	const game& bios = g.bios_get();
	g.flag_set(gar.is_game_tag(bios.name_get(),"neogeo"), flag_derived_neogeo);
	g.flag_set(gar.is_game_tag(bios.name_get(),"decocass"), flag_derived_deco);
	g.flag_set(gar.is_game_tag(bios.name_get(),"playch10"), flag_derived_playchoice);
}

bool mame_mame::load_data(const game_set& gar) {
	load_game_cfg_dir(gar,cfg_path_get());
	return true;
}

bool mame_mame::load_game_coin(const string& file, unsigned& total_coin) const {
	FILE* f = fopen(cpath_export(file),"rb");
	if (!f)
		goto out;

	char header[8];
	if (fread(header,8,1,f)!=1)
		goto out_err;

	if (memcmp(header,"MAMECFG\x8",8)!=0)
		goto out_err;

	unsigned count;
	if (!fread_uint32be(count,f))
		goto out_err;

	for(unsigned j=0;j<2;++j) {
		for(unsigned i=0;i<count;++i) {
			if (!fskip(8,f))
				goto out_err;
			unsigned key_count;
			if (!fread_uint16be(key_count,f))
				goto out_err;
			if (!fskip(key_count*4,f))
				goto out_err;
		}
	}

	unsigned coin;
	coin = 0;
	for(unsigned i=0;i<4;++i) {
		unsigned coin_partial;
		if (!fread_uint32be(coin_partial,f))
			goto out_err;
		coin += coin_partial;
	}

	total_coin = coin;

	// ignore others info

	fclose(f);
	return true;

out_err:
	fclose(f);
out:
	return false;
}

void mame_mame::load_game_cfg_dir(const game_set& gar, const string& dir) const {
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		target_err("Error opening the '%s' .cfg files directory '%s'.\n", user_name_get().c_str(), cpath_export(dir));
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d))!=0) {
		string file = file_import(dd->d_name);
		if (file_ext(file) == ".cfg") {
			string name = user_name_get() + "/" + file_basename(file);
			game_set::const_iterator j = gar.find(game(name));
			if (j != gar.end() && !j->is_coin_set()) {
				unsigned coin;
				if (load_game_coin(slash_add(dir) + file, coin )) {
					j->coin_set(coin);
				}
			}
			// ignore error
		}
	}

	closedir(d);
}

bool mame_mame::load_software(game_set&) {
	return true;
}

bool mame_mame::run(const game& g, unsigned orientation, bool ignore_error) const {
	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	unsigned coin_pre;
	bool coin_pre_set;
	unsigned coin_post;
	bool coin_post_set = false;

	string cfg_file = slash_add(cfg_path_get()) + g.bios_get().name_without_emulator_get() + ".cfg";

	coin_pre_set = load_game_coin(cfg_file, coin_pre);
	if (!coin_pre_set)
		coin_pre = 0;

	argv[argc++] = strdup(cpath_export(file_file(config_exe_path_get())));
	argc = compile(g,argv,argc,"%s",orientation);
	argc = compile(g,argv,argc,user_cmd_arg,orientation);
	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	coin_post_set = load_game_coin(cfg_file, coin_post);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret) {
		g.time_set( g.time_get() + duration );
		if (coin_post_set && coin_post >= coin_pre) {
			g.coin_set( g.coin_get() + coin_post - coin_pre );
		}
	}

	return ret;
}

//---------------------------------------------------------------------------
// dmame

dmame::dmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mame(Aname,Aexe_path,Acmd_arg) {
}

string dmame::type_get() const {
	return "dmame";
}

bool dmame::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mame.cfg";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rompath");
	conf_string_register(context, "snap");
	conf_string_register(context, "cfg");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 0, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "directory", "rompath", &s)==0) {
		emu_rom_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_rom_path = list_abs(list_import("roms"),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "directory", "snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import("snap"),exe_dir_get());
	}

	if (conf_string_section_get(context, "directory", "cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import("cfg"),exe_dir_get());
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// wmame

wmame::wmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mame(Aname,Aexe_path,Acmd_arg) {
}

string wmame::type_get() const {
	return "mame";
}

bool wmame::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mame.ini";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rompath");
	conf_string_register(context, "snapshot_directory");
	conf_string_register(context, "cfg_directory");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "", "rompath", &s)==0) {
		emu_rom_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_rom_path = list_abs(list_import(file_config_dir_singledir("roms")),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "snapshot_directory", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(file_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "cfg_directory", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(file_config_dir_singledir("cfg")),exe_dir_get());
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// wmame

xmame::xmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mame(Aname,Aexe_path,Acmd_arg) {
}

string xmame::type_get() const {
	return "xmame";
}

bool xmame::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	char* home = getenv("HOME");
	if (!home || !*home)
		return true; // ignore if missing

	string home_dir = home;
	if (home_dir.length() && home_dir[home_dir.length()-1] != '/')
		home_dir += '/';
	home_dir += ".xmame";

	string config_file = path_import(home_dir + "/xmamerc");

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rompath");
	conf_string_register(context, "screenshotdir");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	// use the current directory for the relative path

	if (conf_string_section_get(context, "", "rompath", &s)==0) {
		emu_rom_path = list_abs(list_import(s), dir_cwd());
	} else {
		emu_rom_path = list_abs(list_import(home_dir + "/roms"), dir_cwd());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "screenshotdir", &s)==0) {
		emu_snap_path = list_abs(list_import(s), dir_cwd());
	} else {
		emu_snap_path = list_abs(list_import("."), dir_cwd());
	}

	emu_cfg_path = list_abs(list_import(home_dir + "/cfg"), dir_cwd());

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// advmame

advmame::advmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mame(Aname,Aexe_path,Acmd_arg) {
}

string advmame::type_get() const {
	return "advmame";
}

bool advmame::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = path_abs(path_import(file_config_file_home("advmame.rc")),exe_dir_get());

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "dir_rom");
	conf_string_register(context, "dir_snap");
	conf_string_register(context, "dir_cfg");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "", "dir_rom", &s)==0) {
		emu_rom_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_rom_path = list_abs(list_import(file_config_dir_singledir("rom")),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "dir_snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(file_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(file_config_dir_singledir("cfg")),exe_dir_get());
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// advpac

advpac::advpac(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mame(Aname,Aexe_path,Acmd_arg) {
}

string advpac::type_get() const {
	return "advpac";
}

bool advpac::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = path_abs(path_import(file_config_file_home("advpac.rc")),exe_dir_get());

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "dir_rom");
	conf_string_register(context, "dir_snap");
	conf_string_register(context, "dir_cfg");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "", "dir_rom", &s)==0) {
		emu_rom_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_rom_path = list_abs(list_import(file_config_dir_singledir("rom")),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "dir_snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(file_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(file_config_dir_singledir("cfg")),exe_dir_get());
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// mame_mess

mame_mess::mame_mess(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_info(Aname,Aexe_path,Acmd_arg) {
}

void mame_mess::attrib_run() {
	choice_bag ch;

	ch.insert( ch.end(), choice("Present or Missing"," Only\tPresent"," Only\tMissing", exclude_missing_effective, 0) );
	ch.insert( ch.end(), choice("Working or Preliminary"," Only\tWorking"," Only\tPreliminary", exclude_bad_effective, 0) );
	ch.insert( ch.end(), choice("Parent or Clone"," Only\tParent"," Only\tClone", exclude_clone_effective, 0) );
	ch.insert( ch.end(), choice("Any Screen Type", " Only\tScreen Raster", " Only\tScreen Vector", exclude_vector_effective, 0) );
	ch.insert( ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", exclude_vertical_effective, 0) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" " + user_name_get() + " Attrib", ATTRIB_CHOICE_X, ATTRIB_CHOICE_Y, ATTRIB_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_bad_effective = ch[1].tristate_get();
		exclude_clone_effective = ch[2].tristate_get();
		exclude_vector_effective = ch[3].tristate_get();
		exclude_vertical_effective = ch[4].tristate_get();
	}
}

//---------------------------------------------------------------------------
// mess

dmess::dmess(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mess(Aname,Aexe_path,Acmd_arg) {
}

string dmess::type_get() const {
	return "dmess";
}

bool dmess::load_data(const game_set& gar) {
	return true;
}

bool dmess::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mess.cfg";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "snap");
	conf_string_register(context, "cfg");
	conf_string_register(context, "biospath");
	conf_string_register(context, "softwarepath");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 0, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "directory", "biospath", &s)==0) {
		emu_rom_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_rom_path = list_abs("bios",exe_dir_get());
	}

	if (conf_string_section_get(context, "directory", "softwarepath", &s)==0) {
		emu_software_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_software_path = list_abs("software",exe_dir_get());
	}

	if (conf_string_section_get(context, "directory", "snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs("snap",exe_dir_get());
	}

	if (conf_string_section_get(context, "directory", "cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs("cfg",exe_dir_get());
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (i->emulator_get() == this) {
			if (conf_string_section_get(context, const_cast<char*>(i->name_get().c_str()), "softwarepath", &s)==0) {
				i->software_path_set(s);
			} else {
				i->software_path_set(emu_software_path);
			}
		}
	}

	conf_done(context);

	return true;
}

void dmess::scan_software_by_sys(game_container& gac, const string& software, const string& parent)
{
	string software_dir = software + parent;
	DIR* dir = opendir(cpath_export(software_dir));
	if (!dir)
		return;

	struct dirent* ddir;
	while ((ddir = readdir(dir))!=0) {
		string file = file_import(ddir->d_name);
		if (file_ext(file) == ".zip") {
			string name = file_basename(file);

			game g;
			g.emulator_set(this);
			g.name_set( user_name_get() + "/" + parent + "/" + name );
			g.software_set(true);
			g.cloneof_set(user_name_get() + "/" + parent);
			g.romof_set(user_name_get() + "/" + parent);

			string path = software_dir + "/" + file;
			g.rom_zip_set_insert(path);
			g.auto_description_set(case_auto(file_basename(ddir->d_name)));

			gac.insert(gac.end(),g);
		}
	}

	closedir(dir);
}

void dmess::scan_software(game_container& gac, const game_set& gar) {
	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (i->emulator_get() == this) {
			int ptr = 0;
			while (ptr < i->software_path_get().length()) {
				string path = slash_add(token_get(i->software_path_get(),ptr,file_dir_separator()));

				token_skip(i->software_path_get(),ptr,file_dir_separator());

				scan_software_by_sys(gac,path,i->name_without_emulator_get());
			}
		}
	}
}

void dmess::scan_alias(game_set& gar, game_container& gac, const string& cfg) {
	ifstream f(cpath_export(cfg));
	if (!f) {
		target_err("Error opening the '%s' configuration file '%s' for reading the alias.\n", user_name_get().c_str(), cpath_export(cfg));
		return;
	}

	game_set::const_iterator parent_g = gar.end();

	while (!f.eof()) {
		string s;
		getline(f,s,'\n');

		if (s.length() && s[0]!='#') {
			int ptr = 0;

			token_skip(s,ptr," ");
			string first = token_get(s,ptr," =");

			if (first.length()>=2 && first[0]=='[' && first[first.length()-1]==']') {
				// section definition
				string name = user_name_get() + "/" + string(first,1,first.length()-2);
				parent_g = gar.find(game(name));
			} else {
				if (parent_g != gar.end()) {
					// alias definition
					game g;
					g.emulator_set(this);
					g.name_set( parent_g->name_get() + string("/") + first );
					g.software_set(true);
					g.flag_set(true,flag_derived_alias);
					g.cloneof_set(parent_g->name_get());
					g.romof_set(parent_g->name_get());
					token_skip(s,ptr," =");

					// skip until the first #
					token_get(s,ptr,"#");

					if (ptr < s.length() && s[ptr]=='#') {
						++ptr; // skip the #
						g.auto_description_set( strip_space(token_get(s,ptr,"|")));
						if (ptr < s.length() && s[ptr]=='|') ++ptr;
						g.year_set(strip_space(token_get(s,ptr,"|")));
						if (ptr < s.length() && s[ptr]=='|') ++ptr;
						g.manufacturer_set(strip_space(token_get(s,ptr,"|")));
					} else {
						g.auto_description_set( case_auto(first) );
					}

					// insert the game
					gac.insert(gac.end(),g);
				}
			}
		}
	}

	f.close();
}

bool dmess::load_software(game_set& gar) {
	game_container gac;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mess.cfg";

	if (!validate_config_file(config_file))
		return false;

	scan_alias(gar,gac,config_file);
	scan_software(gac,gar);

	// any duplicate alias/software is rejected in the insertion
	// in the game_set. the first is used.
	for(game_container::const_iterator i=gac.begin();i!=gac.end();++i)
		gar.insert(*i);

	return true;
}

string dmess::image_name_get(const string& snap_create, const string& name) {
	char path[FILE_MAXPATH];
	sprintf(path,"%s%.8s.png", slash_add(snap_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.png",slash_add(snap_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

bool dmess::run(const game& g, unsigned orientation, bool ignore_error) const {
	string snapshot_rename_dir;
	string image_create_file;
	string image_rename_file;

	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	if (g.software_get()) {
		const game* bios = &g.bios_get();
		// name without emulator and bios
		string stripped_name = file_file(g.name_get());

		image_create_file = image_name_get(emu_snap_path, bios->name_without_emulator_get());
		snapshot_rename_dir = slash_add(emu_snap_path) + bios->name_without_emulator_get();
		image_rename_file = snapshot_rename_dir + "/" + stripped_name + ".png";

		argv[argc++] = strdup(cpath_export(file_file(config_exe_path)));
		argv[argc++] = strdup(bios->name_without_emulator_get().c_str());

		if (g.flag_get(flag_derived_alias)) {
			argv[argc++] = strdup("-alias");
			argv[argc++] = strdup(stripped_name.c_str());
		} else {
			// default rom (without extension)
			string rom = stripped_name;
			// search the first file in the zip with the same filename of the zip
			for(path_container::const_iterator i=g.rom_zip_set_get().begin();i!=g.rom_zip_set_get().end();++i) {
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
		argv[argc++] = strdup(cpath_export(file_file(config_exe_path)));
		argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	}

	argc = compile(g,argv,argc,user_cmd_arg,orientation);

	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret)
		g.time_set( g.time_get() + duration );

	if (g.software_get()) {
		if (access(cpath_export(image_create_file),F_OK)==0) {
			target_mkdir(cpath_export(snapshot_rename_dir));
			// ignore error
			rename(cpath_export(image_create_file), cpath_export(image_rename_file));
			// ignore error
		}
	}

	return true;
}

//---------------------------------------------------------------------------
// advmess

advmess::advmess(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_mess(Aname,Aexe_path,Acmd_arg) {
}

string advmess::type_get() const {
	return "advmess";
}

bool advmess::load_data(const game_set& gar) {
	return true;
}

bool advmess::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = path_abs(path_import(file_config_file_home("advmess.rc")),exe_dir_get());

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "dir_rom");
	conf_string_register(context, "dir_image");
	conf_string_register(context, "dir_snap");
	conf_string_register(context, "dir_cfg");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 1, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	if (conf_string_section_get(context, "", "dir_rom", &s)==0) {
		emu_rom_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_rom_path = list_abs(list_import(file_config_dir_singledir("rom")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_image", &s)==0) {
		emu_software_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_software_path = list_abs(list_import(file_config_dir_singledir("image")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(file_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(file_config_dir_singledir("cfg")),exe_dir_get());
	}

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (i->emulator_get() == this) {
			if (conf_string_section_get(context, const_cast<char*>(i->name_get().c_str()), "dir_image", &s)==0) {
				i->software_path_set(s);
			} else {
				i->software_path_set(emu_software_path);
			}
		}
	}

	conf_done(context);

	return true;
}

void advmess::scan_software_by_sys(game_container& gac, const string& software, const game& bios)
{
	string parent = bios.name_without_emulator_get();
	string software_dir = software + parent;

	DIR* dir = opendir(cpath_export(software_dir));
	if (!dir)
		return;

	struct dirent* ddir;
	while ((ddir = readdir(dir))!=0) {
		if (ddir->d_name[0] != '.') { // skip some special files
			bool found = false;

			string file = file_import(ddir->d_name);
			string ext = file_ext(file);

			if (ext == ".zip") {
				found = true;
			} else if (ext.length() > 1) {
				string ext_without_dot = string(ext,1,ext.length() - 1);
				for(machinedevice_container::const_iterator i=bios.machinedevice_bag_get().begin();!found && i!=bios.machinedevice_bag_get().end();++i) {
					for(machinedevice_ext_container::const_iterator j=i->ext_bag.begin();!found && j!=i->ext_bag.end();++j) {
						if (ext_without_dot == *j)
							found = true;
					}
				}
			}

			if (found) {
				string name = file_basename(file);

				game g;
				g.emulator_set(this);
				g.name_set( user_name_get() + "/" + parent + "/" + name );
				g.software_set(true);
				g.cloneof_set(user_name_get() + "/" + parent);
				g.romof_set(user_name_get() + "/" + parent);

				string path = software_dir + "/" + file;
				g.rom_zip_set_insert(path);

				g.auto_description_set(case_auto(file_basename(ddir->d_name)));

				gac.insert(gac.end(),g);
			}
		}
	}

	closedir(dir);
}

void advmess::scan_software(game_container& gac, const game_set& gar) {
	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (i->emulator_get() == this) {
			int ptr = 0;
			while (ptr < i->software_path_get().length()) {
				string path = slash_add(token_get(i->software_path_get(),ptr,file_dir_separator()));

				token_skip(i->software_path_get(),ptr,file_dir_separator());

				scan_software_by_sys(gac,path,*i);
			}
		}
	}
}

bool advmess::load_software(game_set& gar) {
	game_container gac;

	scan_software(gac,gar);

	for(game_container::const_iterator i=gac.begin();i!=gac.end();++i)
		gar.insert(*i);

	return true;
}

string advmess::image_name_get(const string& snap_create, const string& name) {
	char path[FILE_MAXPATH];
	sprintf(path,"%s%.8s.png", slash_add(snap_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.png",slash_add(snap_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

string advmess::clip_name_get(const string& clip_create, const string& name) {
	char path[FILE_MAXPATH];
	sprintf(path,"%s%.8s.mng", slash_add(clip_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.mng",slash_add(clip_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

string advmess::sound_name_get(const string& sound_create, const string& name) {
	char path[FILE_MAXPATH];
	sprintf(path,"%s%.8s.wav", slash_add(sound_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.wav",slash_add(sound_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

///
// Check if the specified ext is supported.
// If yes add the required option at the command line
bool advmess::compile_ext(const game& g, unsigned& argc, const char* argv[], const string& ext) const {

	for(machinedevice_container::const_iterator i=g.machinedevice_bag_get().begin();i!=g.machinedevice_bag_get().end();++i) {
		for(machinedevice_ext_container::const_iterator j=i->ext_bag.begin();j!=i->ext_bag.end();++j) {

			log_std(("menu:advmess: compile check ext .%s\n",j->c_str()));

			// case unsensitive compare, it may be in a zip file
			if (case_equal(ext, string(".") + *j)) {

				string option = "-dev_" + i->name;
				argv[argc] = strdup(option.c_str());
				++argc;

				log_std(("menu:advmess: ext match for %s\n",i->name.c_str()));
				return true;
			}
		}
	}

	return false;
}

///
// Scan the zip file for any supported file.
bool advmess::compile_zip(const game& g, unsigned& argc, const char* argv[], const string& zip_file) const {
	adv_zip* zip;
	adv_zipent* ent;
	bool something_found = false;

	if (access(cpath_export(zip_file),F_OK)!=0)
		return false;

	log_std(("menu:advmess: compile zip %s\n",cpath_export(zip_file)));

	zip = openzip(cpath_export(zip_file));
	if (!zip)
		return false;

	// name of the zip without the extension
	string name = file_file(file_basename(zip_file));

	while ((ent = readzip(zip))!=0) {
		log_std(("menu:advmess: compile in zip %s\n",ent->name));

		string zpath = ent->name;
		string zfile = file_file(zpath);
		string zname = file_basename(zfile);
		string zext = file_ext(zfile);

                if (compile_ext(g,argc,argv,zext)) {
			if (zname != name) {
				string spec = name + '=' + zfile;
				argv[argc] = strdup( spec.c_str() );
			} else {
				argv[argc] = strdup( zfile.c_str() );
			}
			++argc;
			something_found = true;
		}
	}

	closezip(zip);

	return something_found;
}

///
// Check if the file is a supported one.
// If yes add the required options at the command line.
bool advmess::compile_single(const game& g, unsigned& argc, const char* argv[], const string& file) const {

	string ext = file_ext(file);

	log_std(("menu:advmess: compile file %s\n",cpath_export(file)));

	if (compile_ext(g, argc,  argv, ext)) {

		argv[argc] = strdup( file_file(file).c_str() );
		++argc;

		return true;
	}

	return false;
}

bool advmess::compile_file(const game& g, unsigned& argc, const char* argv[], const string& file) const {
	if (file_ext(file) == ".zip")
		return compile_zip(g,argc,argv,file);
	else
		return compile_single(g,argc,argv,file);
}

bool advmess::run(const game& g, unsigned orientation, bool ignore_error) const {
	string snapshot_rename_dir;
	string image_create_file;
	string image_rename_file;
	string clip_create_file;
	string clip_rename_file;
	string sound_create_file;
	string sound_rename_file;

	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	if (g.software_get()) {
		const game* bios = &g.bios_get();
		bool found = false;

		// name without emulator and bios
		string stripped_name = file_file(g.name_get());

		image_create_file = image_name_get(emu_snap_path, bios->name_without_emulator_get());
		clip_create_file = clip_name_get(emu_snap_path, bios->name_without_emulator_get());
		sound_create_file = sound_name_get(emu_snap_path, bios->name_without_emulator_get());
		snapshot_rename_dir = slash_add(emu_snap_path) + bios->name_without_emulator_get();
		image_rename_file = snapshot_rename_dir + "/" + stripped_name + ".png";
		clip_rename_file = snapshot_rename_dir + "/" + stripped_name + ".mng";
		sound_rename_file = snapshot_rename_dir + "/" + stripped_name + ".wav";

		argv[argc++] = strdup(cpath_export(file_file(config_exe_path)));
		argv[argc++] = strdup(bios->name_without_emulator_get().c_str());

		// default rom (without extension)
		string rom = stripped_name;

		// search the first file in the zip with the same filename of the zip
		for(path_container::const_iterator i=g.rom_zip_set_get().begin();i!=g.rom_zip_set_get().end();++i) {
			if (compile_file(g.bios_get(), argc, argv, *i)) {
				found = true;
				break;
			}

		if (!found) {
			log_std(("menu:advmess: compile abort\n"));

			for(int i=0;i<argc;++i)
				free(const_cast<char*>(argv[i]));

			return false;
		}

		}
	} else {
		argv[argc++] = strdup(cpath_export(file_file(config_exe_path)));
		argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	}

	argc = compile(g,argv,argc,user_cmd_arg,orientation);

	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret)
		g.time_set( g.time_get() + duration );

	if (g.software_get()) {
		if (access(cpath_export(image_create_file),F_OK)==0) {
			target_mkdir(cpath_export(snapshot_rename_dir));
			// ignore error
			rename(cpath_export(image_create_file), cpath_export(image_rename_file));
			// ignore error
		}
		if (access(cpath_export(clip_create_file),F_OK)==0) {
			target_mkdir(cpath_export(snapshot_rename_dir));
			// ignore error
			rename(cpath_export(clip_create_file), cpath_export(clip_rename_file));
			// ignore error
		}
		if (access(cpath_export(sound_create_file),F_OK)==0) {
			target_mkdir(cpath_export(snapshot_rename_dir));
			// ignore error
			rename(cpath_export(sound_create_file), cpath_export(sound_rename_file));
			// ignore error
		}
	}

	return true;
}

//---------------------------------------------------------------------------
// raine_info

raine_info::raine_info(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	emulator(Aname,Aexe_path,Acmd_arg) {

	exclude_clone_orig = exclude;
	exclude_bad_orig = exclude;
	exclude_vertical_orig = include;
}

void raine_info::attrib_run() {
	choice_bag ch;

	ch.insert( ch.end(), choice("Present or Missing"," Only\tPresent"," Only\tMissing", exclude_missing_effective, 0) );
	ch.insert( ch.end(), choice("Working or Preliminary"," Only\tWorking"," Only\tPreliminary", exclude_bad_effective, 0) );
	ch.insert( ch.end(), choice("Parent or Clone"," Only\tParent"," Only\tClone", exclude_clone_effective, 0) );
	ch.insert( ch.end(), choice("Any Orientation", " Only\tHorizontal", " Only\tVertical", exclude_vertical_effective, 0) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" " + user_name_get() + " Attrib", ATTRIB_CHOICE_X, ATTRIB_CHOICE_Y, ATTRIB_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
		exclude_bad_effective = ch[1].tristate_get();
		exclude_clone_effective = ch[2].tristate_get();
		exclude_vertical_effective = ch[3].tristate_get();
	}
}

void raine_info::attrib_load() {
	emulator::attrib_load();
	exclude_clone_effective = exclude_clone_orig;
	exclude_bad_effective = exclude_bad_orig;
	exclude_vertical_effective = exclude_vertical_orig;
}

void raine_info::attrib_save() {
	emulator::attrib_save();
	exclude_clone_orig = exclude_clone_effective;
	exclude_bad_orig = exclude_bad_effective;
	exclude_vertical_orig = exclude_vertical_effective;
}

bool raine_info::attrib_set(const std::string& value0, const std::string& value1) {
	if (emulator::attrib_set(value0, value1))
		return true;

	if (value0 == "clone") {
		if (!tristate(exclude_clone_orig,value1))
			return false;
	} else if (value0 == "bad") {
		if (!tristate(exclude_bad_orig,value1))
			return false;
	} else if (value0 == "vertical") {
		if (!tristate(exclude_vertical_orig,value1))
			return false;
	} else {
		return false;
	}
	return true;
}

void raine_info::attrib_get(adv_conf* config_context, const char* section, const char* tag) {
	emulator::attrib_get(config_context, section, tag);
	conf_string_set(config_context,section,tag, attrib_compile("clone", tristate(exclude_clone_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("bad", tristate(exclude_bad_orig)).c_str() );
	conf_string_set(config_context,section,tag, attrib_compile("vertical", tristate(exclude_vertical_orig)).c_str() );
}

bool raine_info::filter(const game& g) const {
	if (!emulator::filter(g))
		return false;

	if (exclude_clone_effective == exclude && g.parent_get()!=0)
			return false;
	if (exclude_clone_effective == exclude_not && g.parent_get()==0)
		return false;

	bool good;
	if (exclude_clone_effective == exclude)
		good = g.play_best_get() == play_perfect;
	else
		good = g.play_get() == play_perfect;
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

void raine_info::cache(const game_set& gar, const game& g) const {
	emulator::cache(gar,g);
}

bool raine_info::tree_get() const {
	return exclude_clone_effective == exclude;
}

bool raine_info::load_data(const game_set& gar) {
	return true;
}

bool raine_info::internal_load(FILE* f, game_set& gar) {
	info_t token = info_token_get(f);
	while (token!=info_eof) {
		if (token != info_symbol) return false;
		bool isgame = strcmp(info_text_get(),"game")==0;
		if (isgame) {
			if (info_token_get(f) != info_open) return false;
			game g;
			g.emulator_set(this);
			token = info_token_get(f);
			while (token != info_close) {
				if (token != info_symbol)
					return false;
				if (strcmp(info_text_get(),"name")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.name_set( user_name_get() + "/" + info_text_get() );
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
					g.cloneof_set( user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"romof")==0) {
					if (info_token_get(f) != info_symbol) return false;
					g.romof_set( user_name_get() + "/" + info_text_get() );
				} else if (strcmp(info_text_get(),"driver")==0) {
					if (info_token_get(f) != info_open)  return false;
					token = info_token_get(f);
					while (token != info_close) {
						if (token != info_symbol) return false;
						if (strcmp(info_text_get(),"status")==0) {
							if (info_token_get(f) != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary")==0)
								g.play_set(play_not);
						} else if (strcmp(info_text_get(),"color")==0) {
							if (info_token_get(f) != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary")==0)
								if (g.play_get() < play_major)
									g.play_set(play_major);
						} else if (strcmp(info_text_get(),"sound")==0) {
							if (info_token_get(f) != info_symbol) return false;
							if (strcmp(info_text_get(), "preliminary")==0)
								if (g.play_get() < play_minor)
									g.play_set(play_minor);
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
						if (strcmp(info_text_get(),"orientation")==0) {
							if (info_token_get(f) != info_symbol) return false;
							g.flag_set( strcmp( info_text_get(),"vertical") == 0, flag_derived_vertical );
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
				} else {
					if (info_skip_value(f) == info_error) return false;
				}
				token = info_token_get(f);
			}
			gar.insert( g );
		} else {
			if (info_skip_value(f) == info_error)
				return false;
		}
		token = info_token_get(f);
	}

	return true;
}

bool raine_info::load_game(game_set& gar) {
	struct stat st_info;
	struct stat st_mame;
	int err_info;
	int err_exe;

	string info_file = path_abs(path_import(file_config_file_home((user_name_get() + ".lst").c_str())),dir_cwd());

	err_info = stat(cpath_export(info_file), &st_info);
	err_exe = stat(cpath_export(config_exe_path_get()), &st_mame);

	if (file_ext(config_exe_path_get()) != ".bat"
		&& err_exe==0
		&& (err_info!=0 || st_info.st_mtime < st_mame.st_mtime)) {
		target_out("Updating the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));

		char cmd[TARGET_MAXCMD];
		sprintf(cmd,"%s -gameinfo > %s",cpath_export(config_exe_path_get()),cpath_export(info_file));

		int r = target_system(cmd);

		bool result = spawn_check(r,false);
		if (!result)
			return false;
	}

	FILE* f = fopen(cpath_export(info_file),"rt");
	if (!f) {
		target_err("Error opening the '%s' information file '%s'.\n", user_name_get().c_str(), cpath_export(info_file));
		target_err("Try running manually the command: '%s -gameinfo > %s'.\n", user_exe_path.c_str(), cpath_export(info_file));
		return false;
	}
	info_init();
	if (!internal_load(f,gar)) {
		info_done();
		target_err("Error reading the '%s' information from file '%s' at row %d column %d.\n", user_name_get().c_str(), cpath_export(info_file), info_row_get()+1, info_col_get()+1);
		return false;
	}
	info_done();

	fclose(f);
	return true;
}

bool raine_info::load_software(game_set&) {
	return true;
}

//---------------------------------------------------------------------------
// draine

draine::draine(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	raine_info(Aname,Aexe_path,Acmd_arg) {
}

string draine::type_get() const {
	return "draine";
}

bool draine::load_cfg(const game_set& gar) {
	const char* s;
	adv_conf* context;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "config/raine.cfg";

	if (!validate_config_file(config_file))
		return false;

	context = conf_init();

	conf_string_register(context, "rom_dir_0");
	conf_string_register(context, "rom_dir_1");
	conf_string_register(context, "rom_dir_2");
	conf_string_register(context, "rom_dir_3");
	conf_string_register(context, "screenshots");

	if (conf_input_file_load_adv(context, 0, cpath_export(config_file), 0, 1, 0, 0, 0, 0, 0) != 0) {
		conf_done(context);
		return false;
	}

	emu_rom_path = "";
	if (conf_string_section_get(context, "Directories", "rom_dir_0", &s)==0) {
		emu_rom_path = dir_cat(emu_rom_path,list_abs(list_import(s), exe_dir_get()));
	}
	if (conf_string_section_get(context, "Directories", "rom_dir_1", &s)==0) {
		emu_rom_path = dir_cat(emu_rom_path,list_abs(list_import(s), exe_dir_get()));
	}
	if (conf_string_section_get(context, "Directories", "rom_dir_2", &s)==0) {
		emu_rom_path = dir_cat(emu_rom_path,list_abs(list_import(s), exe_dir_get()));
	}
	if (conf_string_section_get(context, "Directories", "rom_dir_3", &s)==0) {
		emu_rom_path = dir_cat(emu_rom_path,list_abs(list_import(s), exe_dir_get()));
	}
	if (emu_rom_path.length() == 0) {
		emu_rom_path = list_abs(list_import("roms"),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "Directories", "screenshots", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import("screens"),exe_dir_get());
	}

	emu_cfg_path = "";

	log_std(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	log_std(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	log_std(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	log_std(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	log_std(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	log_std(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	log_std(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	log_std(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	log_std(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	log_std(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	log_std(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

bool draine::run(const game& g, unsigned orientation, bool ignore_error) const {
	const char* argv[TARGET_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(file_file(config_exe_path_get())));
	argv[argc++] = strdup("-game");
	argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	argv[argc++] = strdup("-nogui");
	argc = compile(g,argv,argc,user_cmd_arg,orientation);
	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret)
		g.time_set( g.time_get() + duration );

	return ret;
}

//---------------------------------------------------------------------------
// generic

generic::generic(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	emulator(Aname,Aexe_path,Acmd_arg) {
}

void generic::attrib_run() {
	choice_bag ch;

	ch.insert( ch.end(), choice("Present or Missing"," Only\tPresent"," Only\tMissing", exclude_missing_effective, 0) );

	choice_bag::iterator i = ch.begin();
	int key = ch.run(" " + user_name_get() + " Attrib", ATTRIB_CHOICE_X, ATTRIB_CHOICE_Y, ATTRIB_CHOICE_DX, i);

	if (key == TEXT_KEY_ENTER) {
		exclude_missing_effective = ch[0].tristate_get();
	}
}

bool generic::tree_get() const {
	return false;
}

string generic::type_get() const {
	return "generic";
}

bool generic::load_data(const game_set& gar) {
	return true;
}

bool generic::load_cfg(const game_set& gar) {
	config_rom_path = list_abs(list_import(user_rom_path),exe_dir_get());
	config_alts_path = list_abs(list_import(user_alts_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());

	// no scan done, the zip path is already added in the load_game() call

	return true;
}

bool generic::load_game(game_set& gar) {
	load_dirlist(gar,list_abs(list_import(user_rom_path),exe_dir_get()),list_import(user_rom_filter));
	return true;
}

bool generic::load_software(game_set&) {
	return true;
}

bool generic::is_present() const {
	// if empty enable always
	if (user_exe_path.length()==0)
		return true;

	return emulator::is_present();
}

bool generic::is_runnable() const {
	// if empty it isn't runnable
	if (user_exe_path.length()==0)
		return false;

	return emulator::is_runnable();
}

bool generic::run(const game& g, unsigned orientation, bool ignore_error) const {
	// if empty don't run
	if (user_exe_path.length()==0)
		return false;

	return emulator::run(g, orientation, ignore_error);
}
