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
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "emu.h"
#include "unzip.h"
#include "conf.h"
#include "fz.h"
#include "log.h"
#include "target.h"

#include "mame2.h"

#include <zlib.h>

#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>

/***************************************************************************/
/* Declaration */

/** File handle used by all the osd_* functions. */
struct fileio_handle {
	adv_fz* f; /**< File opened. */

	unsigned crc; /**< Current crc value of the file. */
	int is_crc_set; /**< If the crc was computed. */
};

/** Directory handle used by all the osd_dir_* functions. */
struct dirio_handle {
	DIR* h; /**< Dir opened. */
	char* dir; /**< Name of the directory opened. */
	char* pattern; /** Pattern applyed at the directory. */
};

#define FILEIO_MODE_FILE 0 /**< Single file. */
#define FILEIO_MODE_SINGLE 1 /**< Single dir. */
#define FILEIO_MODE_MULTI 2 /**< Multi dir. */

struct fileio_item {
	int type; /**< FILETYPE_* */
	const char* config; /**< Configuration directory tag. */
	const char* def; /**< Default directory. */
	unsigned mode; /**< Mode type. */
	unsigned dir_mac; /**< Number of directories. */
	char** dir_map; /**< Vector of directories. */
};

static struct fileio_item CONFIG[] = {
	/* FILETYPE_RAW */
	{ FILETYPE_ROM, "dir_rom", "rom", FILEIO_MODE_MULTI, 0, 0 },
	/* FILETYPE_ROM_NOCRC */
	{ FILETYPE_IMAGE, "dir_image", "image", FILEIO_MODE_MULTI,0, 0 },
	{ FILETYPE_IMAGE_DIFF, "dir_diff", "diff", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_SAMPLE, "dir_sample", "sample", FILEIO_MODE_MULTI, 0, 0 },
	{ FILETYPE_ARTWORK, "dir_artwork", "artwork", FILEIO_MODE_MULTI, 0, 0 },
	{ FILETYPE_NVRAM, "dir_nvram" , "nvram", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_HIGHSCORE, "dir_hi", "hi", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_HIGHSCORE_DB, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for hiscore.dat */
	{ FILETYPE_CONFIG, "dir_cfg", "cfg", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_INPUTLOG, "dir_inp", "inp", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_STATE, "dir_sta", "sta", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_MEMCARD, "dir_memcard", "memcard", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_SCREENSHOT, "dir_snap", "snap", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_HISTORY, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for history.dat, mameinfo.dat, safequit.dat */
	{ FILETYPE_CHEAT, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for cheat.dat */
	{ FILETYPE_LANGUAGE, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for language file */
	/* FILETYPE_CTRLR */
	/* FILETYPE_INI */
	{ FILETYPE_end, 0, 0, 0, 0 }
};

struct fileio_item* fileio_find(int type) {
	struct fileio_item* i = CONFIG;
	while (i->type != type) {
		if (i->type == FILETYPE_end) {
			return 0;
		}
		++i;
	}
	return i;
}

/***************************************************************************/
/* OSD interface */

int osd_get_path_count(int pathtype)
{
	struct fileio_item* i;

	log_std(("osd: osd_get_path_count(pathtype:%d)\n", pathtype));

	i = fileio_find(pathtype);
	if (!i) {
		log_std(("ERROR:fileio: file type %d unknown\n", pathtype));
		return 0;
	}

	return i->dir_mac;
}

int osd_get_path_info(int pathtype, int pathindex, const char* filename)
{
	struct fileio_item* i;
	char path[FILE_MAXPATH];
	struct stat st;

	log_std(("osd: osd_get_path_info(pathtype:%d,pathindex:%d,filename:%s)\n", pathtype, pathindex, filename));

	i = fileio_find(pathtype);
	if (!i) {
		log_std(("ERROR:fileio: file type %d unknown\n", pathtype));
		return PATH_NOT_FOUND;
	}

	strcpy(path, file_abs(i->dir_map[pathindex], filename));

	if (stat(path, &st) != 0)
		return PATH_NOT_FOUND;
	if (S_ISDIR(st.st_mode))
		return PATH_IS_DIRECTORY;
	if (S_ISREG(st.st_mode));
		return PATH_IS_FILE;
	return PATH_NOT_FOUND;
}

osd_file* osd_fopen(int pathtype, int pathindex, const char* filename, const char* mode)
{
	struct fileio_item* i;
	char path[FILE_MAXPATH];
	FILE* h;

	log_std(("osd: osd_fopen(pathtype:%d,pathindex:%d,filename:%s,mode:%s)\n", pathtype, pathindex, filename, mode));

	i = fileio_find(pathtype);
	if (!i) {
		log_std(("ERROR:fileio: file type %d unknown\n", pathtype));
		return 0;
	}

	strcpy(path, file_abs(i->dir_map[pathindex], filename));

	h = fopen(path, mode);

	return (osd_file*)h;
}

int osd_fseek(osd_file* file, INT64 offset, int whence)
{
	FILE* h = (FILE*)file;

	return fseek(h, offset, whence);
}

UINT64 osd_ftell(osd_file* file)
{
	FILE* h = (FILE*)file;

	return ftell(h);
}

int osd_feof(osd_file* file)
{
	FILE* h = (FILE*)file;

	return feof(h);
}

UINT32 osd_fread(osd_file* file, void* buffer, UINT32 length)
{
	FILE* h = (FILE*)file;

	return fread(buffer, 1, length, h);
}

UINT32 osd_fwrite(osd_file* file, const void* buffer, UINT32 length)
{
	FILE* h = (FILE*)file;

	return fwrite(buffer, 1, length, h);
}

void osd_fclose(osd_file* file)
{
	FILE* h = (FILE*)file;

	fclose(h);
}

/***************************************************************************/
/* Directory interface */

#ifdef MESS

int osd_num_devices(void)
{
	log_std(("osd: osd_num_device()\n"));
	return 0;
}

void osd_change_device(const char* device)
{
	log_std(("osd: osd_change_devicee(device:%s)\n", device));
}

const char *osd_get_device_name(int i)
{
	log_std(("osd: osd_get_device_name(i:%d)\n", i));
	return "";
}

void* osd_dir_open(const char* dir, const char* mask)
{
	struct dirio_handle* h;

	log_std(("osd: osd_dir_open(dir:%s, mask:%s)\n", dir, mask));

	h = malloc(sizeof(struct dirio_handle));

	h->dir = strdup(dir);
	h->pattern = strdup(mask);

	h->h = opendir(dir);
	if (!h->h) {
		free(h->dir);
		h->dir = strdup(mask);
		free(h);
		return 0;
	}

	return h;
}
  
void osd_dir_close(void* void_h)
{
	struct dirio_handle* h = (struct dirio_handle*)void_h;

	log_std(("osd: osd_dir_close(void_h)\n"));

	assert(h && h->h);
  
	if (h->h)
		closedir(h->h);

	free(h->dir);
	free(h->pattern);
	free(h);
}

int osd_dir_get_entry(void* void_h, char* name, int namelength, int* is_dir)
{
	struct dirio_handle* h = (struct dirio_handle*)void_h;
	struct dirent* d;

	d = readdir(h->h);

	while (d) {
		char file[FILE_MAXPATH];
		struct stat st;

		sprintf(file, "%s/%s", h->dir, d->d_name);

		/* on any error ignore the file */

		if (stat(file, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				if (namelength >= strlen(d->d_name) + 1) {
					*is_dir = 1;
					strcpy(name, d->d_name);

					log_std(("osd: osd_dir_get_entry() -> %s\n", name));

					return strlen(name);
				}
			} else if (item_is_match(d->d_name, h->pattern)) {
				if (namelength >= strlen(d->d_name) + 1) {
					*is_dir = 0;
					strcpy(name, d->d_name);

					log_std(("osd: osd_dir_get_entry() -> %s\n", name));

					return strlen(name);
				}
			}
		}

		d = readdir(h->h);
	}

	return 0;
}

void osd_change_directory(const char* dir)
{
	log_std(("osd: osd_change_directory(dir:%s)\n", dir));

	chdir(dir);
}

const char* osd_get_cwd(void)
{
	static char cwd[FILE_MAXPATH];

	log_std(("osd: osd_get_cdw()\n"));

	getcwd(cwd, sizeof(cwd));
	strcat(cwd, "/");

	log_std(("osd: osd_get_cwd() -> %s\n", cwd));

	return cwd;
}

int osd_select_file(int sel, char* filename)
{
	log_std(("osd: osd_select_file(sel:%d, filename:%s)\n", sel, filename));

	return 0;
}

char* osd_basename(char* filename)
{
	char* base;

	log_std(("osd: osd_basename(filename:%s)\n", filename));

	base = strrchr(filename, file_dir_slash());
	if (base)
		++base;
	else
		base = filename;

	log_std(("osd: osd_basename() -> %s\n", base));

	return base;
}

char* osd_strip_extension(const char* file)
{
	char* r;
	char* slash;
	char* dot;

	if (!file)
		return 0;

	r = strdup(file);

	slash = strrchr(r, file_dir_slash());
	dot = strrchr(r, '.');

	if (dot && (!slash || dot > slash))
		*dot = 0;

	return r;
}

char* osd_dirname(const char* file)
{
	char* r;
	char* slash;

	if (!file)
		return 0;

	r = strdup(file);

	slash = strrchr(r, file_dir_slash());
	if (slash) {
		slash[1] = 0;
		return r;
	} else {
		r[0] = 0;
		return r;
	}
}

void osd_device_eject(int type, int id)
{
	image_unload(type, id);
}

#endif

/***************************************************************************/
/* Advance interface */

static adv_error path_allocate(char*** dir_map, unsigned* dir_mac, const char* path)
{
	char* temp_path = strdup(path);
	char* token;
	char separator[2];

	*dir_mac = 0;
	*dir_map = 0;

	separator[0] = file_dir_separator();
	separator[1] = 0;

	token = strtok(temp_path, separator);
	while (token) {
		*dir_map = realloc(*dir_map, ((*dir_mac)+1) * sizeof(char *));
		if (!*dir_map)
			return -1;
		(*dir_map)[*dir_mac] = strdup(token);
		++*dir_mac;
		token = strtok(NULL, separator);
	}

	free(temp_path);

	return 0;
}

static void path_free(char** dir_map, unsigned dir_mac)
{
	int i;
	for(i=0;i<dir_mac;++i)
		free(dir_map[i]);
	free(dir_map);
}

adv_error advance_fileio_init(adv_conf* context)
{

	struct fileio_item* i;
	for(i=CONFIG;i->type != FILETYPE_end;++i) {
		i->dir_map = 0;
		i->dir_mac = 0;
	}

	for(i=CONFIG;i->type != FILETYPE_end;++i) {
		if (i->config) {
			const char* def = 0;
			switch (i->mode) {
				case FILEIO_MODE_MULTI : def = file_config_dir_multidir(i->def); break;
				case FILEIO_MODE_SINGLE : def = file_config_dir_singledir(i->def); break;
				case FILEIO_MODE_FILE : def = file_config_dir_singlefile(); break;
			}
			if (def)
				conf_string_register_default(context, i->config, def);
		}
	}

#ifdef MESS
	conf_string_register_default(context, "dir_crc", file_config_dir_singledir("crc"));
#endif

	return 0;
}

void advance_fileio_done(void)
{
	struct fileio_item* i;
	for(i=CONFIG;i->type != FILETYPE_end;++i) {
		path_free(i->dir_map, i->dir_mac);
	}
}

static void dir_create(char** dir_map, unsigned dir_mac)
{
	unsigned i;
	for(i=0;i<dir_mac;++i) {
		struct stat st;
		if (stat(dir_map[i], &st) != 0) {
			log_std(("advance:fileio: creating dir %s\n", dir_map[i]));
			if (file_mkdir(dir_map[i]) != 0) {
				log_std(("advance:fileio: unable to create dir %s\n", dir_map[i]));
			}
		}
	}
}

adv_error advance_fileio_config_load(adv_conf* context, struct mame_option* option)
{
	struct fileio_item* i;
	for(i=CONFIG;i->type != FILETYPE_end;++i) {
		/* free a previously loaded value */
		path_free(i->dir_map, i->dir_mac);
		i->dir_map = 0;
		i->dir_mac = 0;

		if (i->config) {
			const char* s = conf_string_get_default(context, i->config);
			log_std(("advance:fileio: %s %s\n", i->config, s));
			path_allocate(&i->dir_map, &i->dir_mac, s);
			dir_create(i->dir_map, i->dir_mac);
		} else {
			/* add the standard directories search as default */
			path_allocate(&i->dir_map, &i->dir_mac, file_config_dir_singlefile());
		}
	}

#ifdef MESS
	{
		const char* s = conf_string_get_default(context, "dir_crc");
		log_std(("advance:fileio: %s %s\n", "dir_crc", s));
		strcpy(option->crc_dir, s);
	}
#endif

	return 0;
}

