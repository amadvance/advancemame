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

#include "text.h"
#include "emulator.h"
#include "common.h"
#include "menu.h"
#include "bitmap.h"
#include "game.h"
#include "readinfo.h"
#include "unzip.h"
#include "os.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include <fstream>

using namespace std;

//---------------------------------------------------------------------------
// emulator

static bool spawn_check(const string& tag, int r, bool ignore_error) {
	if (r == -1) {
		if (!ignore_error)
			cerr << "warning:" << tag << ": process not executed (errno: " << errno << ")" << endl;
		return false;
	} else if (WIFSTOPPED(r)) {
		if (!ignore_error)
			cerr << "warning:" << tag << ": process stopped (signal: " << WSTOPSIG(r) << ")" << endl;
		return false;
	} else if (WIFSIGNALED(r)) {
		if (!ignore_error)
			cerr << "warning:" << tag << ": process terminated (signal: " << WTERMSIG(r) << ")" << endl;
		return false;
	} else if (WIFEXITED(r)) {
		if (WEXITSTATUS(r) != 0) {
			if (!ignore_error)
				cerr << "warning:" << tag << ": process exited (status: " << WEXITSTATUS(r) << ")" << endl;
			return false;
		}

		return true;
	} else {
		if (!ignore_error)
			cerr << "warning:" << tag << ": unknow process error (code: " << r << ")" << endl;
		return false;
	}
}

emulator::emulator(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	name(Aname) {
	user_exe_path = Aexe_path;
	user_cmd_arg = Acmd_arg;
	config_exe_path = path_abs(path_import(user_exe_path),dir_cwd());
}

emulator::~emulator() {
}

bool emulator::is_ready() const {
#if __MSDOS__ // TODO How check for the emulator exetuable in Linux ?
	return access(cpath_export(config_exe_path_get()),X_OK)==0;
#else
	return true;
#endif
}

string emulator::exe_dir_get() const {
	string dir = file_dir(config_exe_path_get());
	if (dir.length() == 1 && dir[0]=='/')
		return dir; // save the root slash
	else
		return slash_remove(dir);
}

void emulator::scan_game(const game_set& gar, const string& path, const string& name) {
	game_set::const_iterator i = gar.find( game(name) );
	if (i!=gar.end())
		i->rom_zip_set_insert(path);
}

void emulator::scan_dir(const game_set& gar, const string& dir) {
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		cerr << "warning:" << user_name_get() << ": error opening roms directory " << path_export(dir) << endl;
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d))!=0) {
		string file = os_import(dd->d_name);
		if (file_ext(file) == ".zip") {
			scan_game(gar,dir + "/" + file,user_name_get() + "/" + file_basename(file));
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
		cerr << "warning:" << user_name_get() << ": error opening roms directory " << path_export(dir) << endl;
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d))!=0) {
		string file = os_import(dd->d_name);
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
		os_log(("menu: run error chdir '%s'\n",cpath_export(dir)));
		return false;
	}

	char cmdline[OS_MAXCMD];

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

	os_log(("menu: run '%s'\n",cmdline));

	start = time(0);

	int r = os_spawn(argv[0],argv);

	stop = time(0);

	if (stop - start > 8)
		duration = stop - start;
	else
		duration = 0;

	bool result = spawn_check(user_name_get(),r,ignore_error);

	text_idle_time_reset();

	// if returned with success
	if (result)
		text_idle_repeat_reset();

	chdir(cpath_export(olddir));

	return result;
}

unsigned emulator::compile(const game& g, const char** argv, unsigned argc, const string& list) const {
	int pos = 0;
	token_skip(list,pos," \t");

	while (pos < list.length()) {
		string arg = token_get(list,pos," \t");

		arg = subs(arg,"%s",g.name_without_emulator_get());
		const path_container& pc = g.rom_zip_set_get();

		if (pc.size()) {
			string path = *pc.begin();
			arg = subs(arg,"%p",path_export(path));
			arg = subs(arg,"%f",path_export(file_file(path)));
		}

		argv[argc++] = strdup(arg.c_str());
		token_skip(list,pos," \t");
	}

	return argc;
}

bool emulator::run(const game& g, bool ignore_error) const {
	const char* argv[OS_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(file_file(config_exe_path_get())));
	argc = compile(g,argv,argc,user_cmd_arg);
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

	gar.preview_list_set(config_alts_path_get(),user_name_get(),&game::preview_snap_set_ifmissing,".png",".pcx");
	gar.preview_list_set(config_alts_path_get(),user_name_get(),&game::preview_clip_set_ifmissing,".mng","");
	gar.preview_list_set(config_alts_path_get(),user_name_get(),&game::preview_sound_set_ifmissing,".mp3",".wav");

	// search the previews in the software directory
	if (software_path_get().length()) {
		gar.preview_software_list_set(config_alts_path_get(),user_name_get(),&game::preview_snap_set_ifmissing,".png",".pcx");
		gar.preview_software_list_set(config_alts_path_get(),user_name_get(),&game::preview_clip_set_ifmissing,".mng","");
		gar.preview_software_list_set(config_alts_path_get(),user_name_get(),&game::preview_sound_set_ifmissing,".mp3",".wav");
	}

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
	if (!g.preview_list_set(config_alts_path_get(),&game::preview_snap_set,".png",".pcx"))
		g.preview_snap_set(resource());

	if (!g.preview_list_set(config_alts_path_get(),&game::preview_clip_set,".mng",""))
		g.preview_clip_set(resource());

	if (!g.preview_list_set(config_alts_path_get(),&game::preview_sound_set,".mp3",".wav"))
		g.preview_sound_set(resource());
}

//---------------------------------------------------------------------------
// mame_like

mame_like::mame_like(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	emulator(Aname,Aexe_path,Acmd_arg) {
}

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

bool mame_like::load_game(game_set& gar) {
	struct stat st_info;
	struct stat st_mame;
	int err_info;
	int err_exe;

	string info_file = path_abs(path_import(os_config_file_home( (user_name_get() + ".lst").c_str())),dir_cwd());

	err_info = stat(cpath_export(info_file), &st_info);
	err_exe = stat(cpath_export(config_exe_path_get()), &st_mame);

	if (file_ext(config_exe_path_get()) != ".bat"
		&& err_exe==0
		&& (err_info!=0 || st_info.st_mtime < st_mame.st_mtime || st_info.st_size == 0)) {
		cerr << "warning:" << user_name_get() << ": updating the file " << path_export(info_file) << endl;

		char cmd[OS_MAXCMD];
		sprintf(cmd,"%s -listinfo > %s", cpath_export(config_exe_path_get()),cpath_export(info_file));

		int r = os_system(cmd);

		bool result = spawn_check(user_name_get(),r,false);

		if (!result)
			return false;
	}

	FILE* f = fopen(cpath_export(info_file),"rt");
	if (!f) {
		cerr << "error:" << user_name_get() << ": file " << path_export(info_file) << " not found!" << endl;
		cerr << "error:" << user_name_get() << ": run manually the command: " << user_exe_path << " -listinfo > " << path_export(info_file) << endl;
		return false;
	}
	if (!gar.load(f,this)) {
		cerr << "error:" << user_name_get() << ": loading game information from file " << path_export(info_file) << " at row " << info_row_get()+1 << " column " << info_col_get()+1 << endl;
		return false;
	}

	fclose(f);

	return true;
}

void mame_like::update(const game& g) const {
	emulator::update(g);
}

//---------------------------------------------------------------------------
// mame

mame::mame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_like(Aname,Aexe_path,Acmd_arg) {
}

string mame::type_get() const {
	return "mame";
}

bool mame::load_data(const game_set& gar) {
	load_game_cfg_dir(gar,cfg_path_get());
	return true;
}

bool mame::load_cfg(const game_set& gar) {
	const char* s;
	struct conf_context* context;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mame.cfg";

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

	os_log(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	os_log(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	os_log(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	os_log(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	os_log(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	os_log(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	os_log(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	os_log(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	os_log(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	os_log(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	os_log(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

bool mame::load_game_coin(const string& file, unsigned& total_coin) const {
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

void mame::load_game_cfg_dir(const game_set& gar, const string& dir) const {
	DIR* d = opendir(cpath_export(dir));
	if (!d) {
		cerr << "warning:" << user_name_get() << ": error opening cfg directory " << path_export(dir) << endl;
		return;
	}

	struct dirent* dd;

	while ((dd = readdir(d))!=0) {
		string file = os_import(dd->d_name);
		if (file_ext(file) == ".cfg") {
			string name = user_name_get() + "/" + file_basename(file);
			game_set::const_iterator j = gar.find( game(name) );
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

bool mame::load_software(game_set&) {
	return true;
}

bool mame::run(const game& g, bool ignore_error) const {
	const char* argv[OS_MAXARG];
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
	argc = compile(g,argv,argc,"%s");
	argc = compile(g,argv,argc,user_cmd_arg);
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
// advmame

advmame::advmame(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame(Aname,Aexe_path,Acmd_arg) {
}

string advmame::type_get() const {
	return "advmame";
}

bool advmame::load_cfg(const game_set& gar) {
	const char* s;
	struct conf_context* context;

	string config_file = path_abs(path_import(os_config_file_home("advmame.rc")),exe_dir_get());

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
		emu_rom_path = list_abs(list_import(os_config_dir_singledir("rom")),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "dir_snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(os_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(os_config_dir_singledir("cfg")),exe_dir_get());
	}

	os_log(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	os_log(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	os_log(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	os_log(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	os_log(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	os_log(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	os_log(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	os_log(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	os_log(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	os_log(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	os_log(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// advpac

advpac::advpac(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame(Aname,Aexe_path,Acmd_arg) {
}

string advpac::type_get() const {
	return "advpac";
}

bool advpac::load_cfg(const game_set& gar) {
	const char* s;
	struct conf_context* context;

	string config_file = path_abs(path_import(os_config_file_home("advpac.rc")),exe_dir_get());

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
		emu_rom_path = list_abs(list_import(os_config_dir_singledir("rom")),exe_dir_get());
	}

	emu_software_path = "";

	if (conf_string_section_get(context, "", "dir_snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(os_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(os_config_dir_singledir("cfg")),exe_dir_get());
	}

	os_log(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	os_log(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	os_log(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	os_log(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	os_log(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	os_log(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	os_log(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	os_log(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	os_log(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	os_log(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	os_log(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

//---------------------------------------------------------------------------
// mess

mess::mess(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	mame_like(Aname,Aexe_path,Acmd_arg) {
}

string mess::type_get() const {
	return "mess";
}

bool mess::load_data(const game_set& gar) {
	return true;
}

bool mess::load_cfg(const game_set& gar) {
	const char* s;
	struct conf_context* context;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mess.cfg";

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

	os_log(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	os_log(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	os_log(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	os_log(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	os_log(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	os_log(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	os_log(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	os_log(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	os_log(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	os_log(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	os_log(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (conf_string_section_get(context, const_cast<char*>(i->name_get().c_str()), "softwarepath", &s)==0) {
			i->software_path_set(s);
		} else {
			i->software_path_set(emu_software_path);
		}
	}

	conf_done(context);

	return true;
}

void mess::scan_software_by_sys(game_container& gac, const string& software, const string& parent)
{
	string software_dir = software + parent;
	DIR* dir = opendir(cpath_export(software_dir));
	if (!dir)
		return;

	struct dirent* ddir;
	while ((ddir = readdir(dir))!=0) {
		string file = os_import(ddir->d_name);
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

void mess::scan_software(game_container& gac, const game_set& gar) {
	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		int ptr = 0;
		while (ptr < i->software_path_get().length()) {
			string path = slash_add(token_get(i->software_path_get(),ptr,os_dir_separator()));

			token_skip(i->software_path_get(),ptr,os_dir_separator());

			scan_software_by_sys(gac,path,i->name_without_emulator_get());
		}
	}
}

void mess::scan_alias(game_set& gar, game_container& gac, const string& cfg) {
	ifstream f(cpath_export(cfg));
	if (!f) {
		cerr << "warning:" << user_name_get() << ": error opening " << path_export(cfg) << " for alias" << endl;
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
				parent_g = gar.find( game(name) );
			} else {
				if (parent_g != gar.end()) {
					// alias definition
					game g;
					g.emulator_set(this);
					g.name_set( parent_g->name_get() + string("/") + first );
					g.software_set(true);
					g.alias_set(true);
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

bool mess::load_software(game_set& gar) {
	game_container gac;

	string config_file = slash_add(file_dir(config_exe_path_get())) + "mess.cfg";

	scan_alias(gar,gac,config_file);
	scan_software(gac,gar);

	// any duplicate alias/software is rejected in the insertion
	// in the game_set. the first is used.
	for(game_container::const_iterator i=gac.begin();i!=gac.end();++i)
		gar.insert(*i);

	return true;
}

string mess::image_name_get(const string& snap_create, const string& name) {
	char path[OS_MAXPATH];
	sprintf(path,"%s%.8s.png", slash_add(snap_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.png",slash_add(snap_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

bool mess::run(const game& g, bool ignore_error) const {
	string snapshot_rename_dir;
	string image_create_file;
	string image_rename_file;

	const char* argv[OS_MAXARG];
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

		if (g.alias_get()) {
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

	argc = compile(g,argv,argc,user_cmd_arg);

	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret)
		g.time_set( g.time_get() + duration );

	if (g.software_get()) {
		if (access(cpath_export(image_create_file),F_OK)==0) {
			mkdir(cpath_export(snapshot_rename_dir), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
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
	mame_like(Aname,Aexe_path,Acmd_arg) {
}

string advmess::type_get() const {
	return "advmess";
}

bool advmess::load_data(const game_set& gar) {
	return true;
}

bool advmess::load_cfg(const game_set& gar) {
	const char* s;
	struct conf_context* context;

	string config_file = path_abs(path_import(os_config_file_home("advmess.rc")),exe_dir_get());

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
		emu_rom_path = list_abs(list_import(os_config_dir_singledir("rom")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_image", &s)==0) {
		emu_software_path = list_abs(list_import(s), exe_dir_get());
	} else {
		emu_software_path = list_abs(list_import(os_config_dir_singledir("image")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_snap", &s)==0) {
		emu_snap_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_snap_path = list_abs(list_import(os_config_dir_singledir("snap")),exe_dir_get());
	}

	if (conf_string_section_get(context, "", "dir_cfg", &s)==0) {
		emu_cfg_path = list_abs(list_import(s),exe_dir_get());
	} else {
		emu_cfg_path = list_abs(list_import(os_config_dir_singledir("cfg")),exe_dir_get());
	}

	os_log(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	os_log(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	os_log(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	os_log(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	os_log(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	os_log(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	os_log(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	os_log(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	os_log(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	os_log(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	os_log(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	for(game_set::const_iterator i=gar.begin();i!=gar.end();++i) {
		if (conf_string_section_get(context, const_cast<char*>(i->name_get().c_str()), "dir_image", &s)==0) {
			i->software_path_set(s);
		} else {
			i->software_path_set(emu_software_path);
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

			string file = os_import(ddir->d_name);
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
		int ptr = 0;
		while (ptr < i->software_path_get().length()) {
			string path = slash_add(token_get(i->software_path_get(),ptr,os_dir_separator()));

			token_skip(i->software_path_get(),ptr,os_dir_separator());

			scan_software_by_sys(gac,path,*i);
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
	char path[OS_MAXPATH];
	sprintf(path,"%s%.8s.png", slash_add(snap_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.png",slash_add(snap_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

string advmess::clip_name_get(const string& clip_create, const string& name) {
	char path[OS_MAXPATH];
	sprintf(path,"%s%.8s.mng", slash_add(clip_create).c_str(),name.c_str());

	int snapno = 0;
	while (access(cpath_export(path),F_OK)==0) {
		sprintf(path,"%s%.4s%04d.mng",slash_add(clip_create).c_str(),name.c_str(),snapno);
		++snapno;
	}

	return path;
}

string advmess::sound_name_get(const string& sound_create, const string& name) {
	char path[OS_MAXPATH];
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

			os_log(("menu:advmess: compile check ext .%s\n",j->c_str()));

			// case unsensitve compare, it may be in a zip file
			if (case_equal(ext, string(".") + *j)) {

				string option = "-dev_" + i->name;
				argv[argc] = strdup(option.c_str());
				++argc;

				os_log(("menu:advmess: ext match for %s\n",i->name.c_str()));
				return true;
			}
		}
	}

	return false;
}

///
// Scan the zip file for any supported file.
bool advmess::compile_zip(const game& g, unsigned& argc, const char* argv[], const string& zip_file) const {
	ZIP* zip;
	struct zipent* ent;
	bool something_found = false;

	if (access(cpath_export(zip_file),F_OK)!=0)
		return false;

	os_log(("menu:advmess: compile zip %s\n",cpath_export(zip_file)));

	zip = openzip(cpath_export(zip_file));
	if (!zip)
		return false;

	// name of the zip without the extension
	string name = file_file(file_basename(zip_file));

	while ((ent = readzip(zip))!=0) {
		os_log(("menu:advmess: compile in zip %s\n",ent->name));

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

	os_log(("menu:advmess: compile file %s\n",cpath_export(file)));

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

bool advmess::run(const game& g, bool ignore_error) const {
	string snapshot_rename_dir;
	string image_create_file;
	string image_rename_file;
	string clip_create_file;
	string clip_rename_file;
	string sound_create_file;
	string sound_rename_file;

	const char* argv[OS_MAXARG];
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
			os_log(("menu:advmess: compile abort\n"));

			for(int i=0;i<argc;++i)
				free(const_cast<char*>(argv[i]));

			return false;
		}

		}
	} else {
		argv[argc++] = strdup(cpath_export(file_file(config_exe_path)));
		argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	}

	argc = compile(g,argv,argc,user_cmd_arg);

	argv[argc] = 0;

	time_t duration;
	bool ret = run_process(duration,exe_dir_get(),argc,argv,ignore_error);

	for(int i=0;i<argc;++i)
		free(const_cast<char*>(argv[i]));

	if (ret)
		g.time_set( g.time_get() + duration );

	if (g.software_get()) {
		if (access(cpath_export(image_create_file),F_OK)==0) {
			mkdir(cpath_export(snapshot_rename_dir), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			// ignore error
			rename(cpath_export(image_create_file), cpath_export(image_rename_file));
			// ignore error
		}
		if (access(cpath_export(clip_create_file),F_OK)==0) {
			mkdir(cpath_export(snapshot_rename_dir), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			// ignore error
			rename(cpath_export(clip_create_file), cpath_export(clip_rename_file));
			// ignore error
		}
		if (access(cpath_export(sound_create_file),F_OK)==0) {
			mkdir(cpath_export(snapshot_rename_dir), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			// ignore error
			rename(cpath_export(sound_create_file), cpath_export(sound_rename_file));
			// ignore error
		}
	}

	return true;
}

//---------------------------------------------------------------------------
// raine

raine::raine(const string& Aname, const string& Aexe_path, const string& Acmd_arg) :
	emulator(Aname,Aexe_path,Acmd_arg) {
}

string raine::type_get() const {
	return "raine";
}

bool raine::load_data(const game_set& gar) {
	return true;
}

bool raine::load_cfg(const game_set& gar) {
	const char* s;
	struct conf_context* context;

	string config_file = slash_add(exe_dir_get()) + "config/raine.cfg";

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

	os_log(("%s: emu_rom_path %s\n", user_name_get().c_str(), cpath_export(emu_rom_path) ));
	os_log(("%s: emu_software_path %s\n", user_name_get().c_str(), cpath_export(emu_software_path) ));
	os_log(("%s: emu_snap_path %s\n", user_name_get().c_str(), cpath_export(emu_snap_path) ));
	os_log(("%s: emu_cfg_path %s\n", user_name_get().c_str(), cpath_export(emu_cfg_path) ));

	conf_done(context);

	config_rom_path = dir_cat(emu_rom_path, list_abs(list_import(user_rom_path), exe_dir_get()));
	config_alts_path = dir_cat(emu_snap_path, list_abs(list_import(user_alts_path),exe_dir_get()));
	config_icon_path = list_abs(list_import(user_icon_path),exe_dir_get());
	config_flyer_path = list_abs(list_import(user_flyer_path),exe_dir_get());
	config_cabinet_path = list_abs(list_import(user_cabinet_path),exe_dir_get());
	config_marquee_path = list_abs(list_import(user_marquee_path),exe_dir_get());
	config_title_path = list_abs(list_import(user_title_path),exe_dir_get());

	os_log(("%s: rom_path %s\n", user_name_get().c_str(), cpath_export(config_rom_path) ));
	os_log(("%s: alts_path %s\n", user_name_get().c_str(), cpath_export(config_alts_path) ));
	os_log(("%s: icon_path %s\n", user_name_get().c_str(), cpath_export(config_icon_path) ));
	os_log(("%s: flyer_path %s\n", user_name_get().c_str(), cpath_export(config_flyer_path) ));
	os_log(("%s: cabinet_path %s\n", user_name_get().c_str(), cpath_export(config_cabinet_path) ));
	os_log(("%s: marquee_path %s\n", user_name_get().c_str(), cpath_export(config_marquee_path) ));
	os_log(("%s: title_path %s\n", user_name_get().c_str(), cpath_export(config_title_path) ));

	scan_dirlist(gar, config_rom_path_get());

	return true;
}

bool raine::load_game(game_set& gar) {
	struct stat st_info;
	struct stat st_mame;
	int err_info;
	int err_exe;

	string info_file = path_abs(path_import(os_config_file_home((user_name_get() + ".lst").c_str())),dir_cwd());

	err_info = stat(cpath_export(info_file), &st_info);
	err_exe = stat(cpath_export(config_exe_path_get()), &st_mame);

	if (file_ext(config_exe_path_get()) != ".bat"
		&& err_exe==0
		&& (err_info!=0 || st_info.st_mtime < st_mame.st_mtime)) {
		cerr << "warning:" << user_name_get() << ": updating the file " << path_export(info_file) << endl;

		char cmd[OS_MAXCMD];
		sprintf(cmd,"%s -gameinfo > %s",cpath_export(config_exe_path_get()),cpath_export(info_file));

		int r = os_system(cmd);

		bool result = spawn_check(user_name_get(),r,false);
		if (!result)
			return false;
	}

	FILE* f = fopen(cpath_export(info_file),"rt");
	if (!f) {
		cerr << "error:" << user_name_get() << ": file " << path_export(info_file) << " not found!" << endl;
		cerr << "error:" << user_name_get() << ": run manually the command: " << user_exe_path << " -gameinfo > " << path_export(info_file) << endl;
		return false;
	}
	if (!gar.load(f,this)) {
		cerr << "error:" << user_name_get() << ": loading game information from file " << path_export(info_file) << " at row " << info_row_get()+1 << " column " << info_col_get()+1 << endl;
		return false;
	}

	fclose(f);
	return true;
}

bool raine::load_software(game_set&) {
	return true;
}

bool raine::run(const game& g, bool ignore_error) const {
	const char* argv[OS_MAXARG];
	unsigned argc = 0;

	argv[argc++] = strdup(cpath_export(file_file(config_exe_path_get())));
	argv[argc++] = strdup("-game");
	argv[argc++] = strdup(g.name_without_emulator_get().c_str());
	argv[argc++] = strdup("-nogui");
	argc = compile(g,argv,argc,user_cmd_arg);
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

bool generic::is_ready() const {
	if (user_exe_path.length()!=0)
		return emulator::is_ready();
	else
		return true;
}

bool generic::run(const game& g, bool ignore_error) const {
	if (user_exe_path.length()!=0)
		return emulator::run(g,ignore_error);
	else
		return false;
}
