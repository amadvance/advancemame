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

#include "portable.h"

#include "emu.h"

#include "glueint.h"

#include "advance.h"

#include <zlib.h>

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

static struct fileio_item FILEIO_CONFIG[] = {
	/* FILETYPE_RAW */
	{ FILETYPE_ROM, "dir_rom", "rom", FILEIO_MODE_MULTI, 0, 0 },
	{ FILETYPE_IMAGE, "dir_image", "image", FILEIO_MODE_MULTI, 0, 0 },
	{ FILETYPE_IMAGE_DIFF, "dir_diff", "diff", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_SAMPLE, "dir_sample", "sample", FILEIO_MODE_MULTI, 0, 0 },
	{ FILETYPE_ARTWORK, "dir_artwork", "artwork", FILEIO_MODE_MULTI, 0, 0 },
	{ FILETYPE_NVRAM, "dir_nvram" , "nvram", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_HIGHSCORE, "dir_hi", "hi", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_HIGHSCORE_DB, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for hiscore.dat */
	/* FILETYPE_CONFIG */
	{ FILETYPE_INPUTLOG, "dir_inp", "inp", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_STATE, "dir_sta", "sta", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_MEMCARD, "dir_memcard", "memcard", FILEIO_MODE_SINGLE, 0, 0 },
	{ FILETYPE_SCREENSHOT, "dir_snap", "snap", FILEIO_MODE_SINGLE, 0, 0 },
	/* FILETYPE_MOVIE */
	{ FILETYPE_HISTORY, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for history.dat, mameinfo.dat */
	{ FILETYPE_CHEAT, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for cheat.dat */
	{ FILETYPE_LANGUAGE, 0, 0, FILEIO_MODE_FILE, 0, 0 }, /* used for language file */
	/* FILETYPE_CTRLR */
	/* FILETYPE_INI */
	/* FILETYPE_HASH, */
	{ FILETYPE_end, 0, 0, 0, 0 }
};

struct fileio_item* fileio_find(int type) {
	struct fileio_item* i = FILEIO_CONFIG;
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
		log_std(("WARNING:fileio: file type %d unknown\n", pathtype));
		return 0;
	}

	return i->dir_mac;
}

int osd_get_path_info(int pathtype, int pathindex, const char* filename)
{
	struct fileio_item* i;
	char path_buffer[FILE_MAXPATH];
	struct stat st;

	log_std(("osd: osd_get_path_info(pathtype:%d,pathindex:%d,filename:%s)\n", pathtype, pathindex, filename));

	i = fileio_find(pathtype);
	if (!i) {
		log_std(("WARNING:fileio: file type %d unknown\n", pathtype));
		return PATH_NOT_FOUND;
	}

	sncpy(path_buffer, sizeof(path_buffer), file_abs(i->dir_map[pathindex], filename));

	log_std(("osd: osd_get_path_info() try %s\n", path_buffer));

	if (stat(path_buffer, &st) != 0) {
		log_std(("osd: osd_get_path_info() -> failed\n"));
		return PATH_NOT_FOUND;
	}
	if (S_ISDIR(st.st_mode)) {
		log_std(("osd: osd_get_path_info() -> directory\n"));
		return PATH_IS_DIRECTORY;
	}
	if (S_ISREG(st.st_mode)) {
		log_std(("osd: osd_get_path_info() -> file\n"));
		return PATH_IS_FILE;
	}

	log_std(("osd: osd_get_path_info() -> failed\n"));
	return PATH_NOT_FOUND;
}

static int partialequal(const char* zipfile, const char* file)
{
	const char* s1 = file;
	/* start comparison after last / */
	const char* s2 = strrchr(zipfile, '/');
	if (s2)
		++s2;
	else
		s2 = zipfile;
	while (*s1 && toupper(*s1)==toupper(*s2)) {
		++s1;
		++s2;
	}
	return !*s1 && !*s2;
}

static void osd_errno_to_filerr(osd_file_error *error)
{
	switch (errno) {
	case ENOENT :
		*error = FILEERR_NOT_FOUND;
		break;
	case EACCES :
		*error = FILEERR_ACCESS_DENIED;
		break;
	default:
		*error = FILEERR_FAILURE;
		break;
	}
/*
	The following error are too specific, no need to report them:
	FILEERR_OUT_OF_MEMORY,
	FILEERR_ALREADY_OPEN,
	FILEERR_TOO_MANY_FILES
*/
}

osd_file* osd_fopen(int pathtype, int pathindex, const char* filename, const char* mode, osd_file_error *error)
{
	struct fileio_item* i;
	char path_buffer[FILE_MAXPATH];
	adv_fz* h;
	char* split;
	struct advance_fileio_context* context = &CONTEXT.fileio;

	log_std(("osd: osd_fopen(pathtype:%d,pathindex:%d,filename:%s,mode:%s)\n", pathtype, pathindex, filename, mode));

	/* set a default error */
	*error = FILEERR_FAILURE;

	i = fileio_find(pathtype);
	if (!i) {
		log_std(("WARNING:fileio: file type %d unknown\n", pathtype));
		return 0;
	}

	/* HACK: for .CHD file MAME adds an initial slash */
	/* remove it assuming always relative paths */
	if (filename[0] == file_dir_slash())
		++filename;

	sncpy(path_buffer, sizeof(path_buffer), file_abs(i->dir_map[pathindex], filename));

	split = strchr(path_buffer, '=');
	if (split != 0) {
		char zip_file_buffer[FILE_MAXPATH];
		char file_buffer[FILE_MAXPATH];
		adv_zip* zip;
		adv_zipent* ent;

		*split = 0;
		snprintf(zip_file_buffer, sizeof(zip_file_buffer), "%s.zip", path_buffer);
		sncpy(file_buffer, sizeof(file_buffer), split + 1);

		log_std(("osd: osd_fopen() try %s %s\n", zip_file_buffer, file_buffer));

		if (access(zip_file_buffer, R_OK)!=0) {
			osd_errno_to_filerr(error);
			log_std(("osd: osd_fopen() -> failed, zip %s not readable\n", zip_file_buffer));
			return 0;
		}

		zip = zip_open(zip_file_buffer);
		if (!zip) {
			osd_errno_to_filerr(error);
			log_std(("osd: osd_fopen() -> failed, zip %s not openable\n", zip_file_buffer));
			return 0;
		}

		h = 0;
		while ((ent = zip_read(zip))!=0) {
			if (partialequal(ent->name, file_buffer)) {
				if (ent->compression_method == 0) {
					h = fzopenzipuncompressed(zip_file_buffer, ent->offset_lcl_hdr_frm_frst_disk, ent->uncompressed_size);
					if (h == 0)
						osd_errno_to_filerr(error);
				} else if (ent->compression_method == 8) {
					h = fzopenzipcompressed(zip_file_buffer, ent->offset_lcl_hdr_frm_frst_disk, ent->compressed_size, ent->uncompressed_size);
					if (h == 0)
						osd_errno_to_filerr(error);
				}
				break;
			}
		}

		zip_close(zip);
	} else {
		log_std(("osd: osd_fopen() try file %s\n", path_buffer));

		/* for the diff file support the write in memory mode */
		if (i->type == FILETYPE_IMAGE_DIFF) {
			/* if the file is already open reuse it */
			if (context->state.diff_handle && strcmp(path_buffer, context->state.diff_file_buffer)==0) {
				h = context->state.diff_handle;
			} else {
				/* try a normal open */
				h = fzopen(path_buffer, mode);
				if (h == 0) {
					osd_errno_to_filerr(error);
					log_std(("osd: fzopen() failed, %s\n", strerror(errno)));
					if (errno == EACCES || errno == EROFS) {
						log_std(("osd: retry with readonly\n"));
						/* reopen in memory */
						h = fzopennullwrite(path_buffer, mode);
						if (h == 0) {
							osd_errno_to_filerr(error);
							log_std(("osd: fzopenullwrite() failed, %s\n", strerror(errno)));
						} else {
							if (context->state.diff_handle == 0) {
								/* save the handle if not already saved */
								context->state.diff_handle = h;
								sncpy(context->state.diff_file_buffer, sizeof(context->state.diff_file_buffer), path_buffer);
							}
						}
					}
				}
			}
		} else {
			/* open a regular file */
			h = fzopen(path_buffer, mode);
			if (h == 0) {
				osd_errno_to_filerr(error);
				log_std(("osd: fzopen() failed, %s\n", strerror(errno)));
			}
		}
	}

	log_std(("osd: osd_fopen() -> return %p\n", h));

	/* clear the error if success */
	if (h != 0)
		*error = FILEERR_SUCCESS;

	return (osd_file*)h;
}

void osd_fclose(osd_file* file)
{
	adv_fz* h = (adv_fz*)file;
	struct advance_fileio_context* context = &CONTEXT.fileio;

	log_std(("osd: osd_fclose(%p)\n", file));

	if (h == context->state.diff_handle) {
		/* don't close the diff memory handler */

		/* reset the position */
		if (fzseek(h, 0, SEEK_SET) != 0) {
			fzclose(h);
			context->state.diff_handle = 0;
		}
	} else {
		fzclose(h);
	}
}

int osd_fseek(osd_file* file, INT64 offset, int whence)
{
	adv_fz* h = (adv_fz*)file;
	int r;

	r = fzseek(h, offset, whence);

	log_std(("osd: osd_fseek(%p, offset:%d, whence:%d)-> %d\n", file, (int)offset, (int)whence, (int)r));

	return r;
}

UINT64 osd_ftell(osd_file* file)
{
	adv_fz* h = (adv_fz*)file;
	UINT64 r;

	r = fztell(h);

	log_std(("osd: osd_ftell(%p) -> %d\n", file, (int)r));

	return r;
}

int osd_feof(osd_file* file)
{
	adv_fz* h = (adv_fz*)file;
	int r;

	r = fzeof(h);

	log_debug(("osd: osd_feof(%p) -> %d\n", file, (int)r));

	return r;
}

UINT32 osd_fread(osd_file* file, void* buffer, UINT32 length)
{
	adv_fz* h = (adv_fz*)file;
	UINT32 r;

	r = fzread(buffer, 1, length, h);

	log_debug(("osd: osd_fread(%p, length:%d) -> %d\n", file, (int)length, (int)r));

	return r;
}

UINT32 osd_fwrite(osd_file* file, const void* buffer, UINT32 length)
{
	adv_fz* h = (adv_fz*)file;
	UINT32 r;

	r = fzwrite(buffer, 1, length, h);

	log_debug(("osd: osd_fwrite(%p, length:%d) -> %d\n", file, (int)length, (int)r));

	return r;
}

int osd_create_directory(int pathtype, int pathindex, const char *dirname)
{
	struct fileio_item* i;
	char path_buffer[FILE_MAXPATH];

	log_std(("osd: osd_create_directory(pathtype:%d,pathindex:%d,dirname:%s)\n", pathtype, pathindex, dirname));

	i = fileio_find(pathtype);
	if (!i) {
		log_std(("WARNING:fileio: file type %d unknown\n", pathtype));
		return -1;
	}

	sncpy(path_buffer, sizeof(path_buffer), file_abs(i->dir_map[pathindex], dirname));

	log_std(("osd: osd_create_directory() -> %s\n", path_buffer));

	if (file_dir_make(path_buffer) != 0) {
		log_std(("ERROR:fileio: mkdir(%s) failed\n", path_buffer));
		return -1;
	}

	return 0;
}

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

static int match(const char* str, const char* pattern)
{
	while (*str && *pattern) {
		if (*pattern == '*') {
			if (match(str+1, pattern))
				return 1;
			++pattern;
		} else if (*pattern == '?') {
			++str;
			++pattern;
		} else if (toupper(*str) == toupper(*pattern)) {
			++str;
			++pattern;
		} else
			return 0;
	}

	while (*pattern == '*' || *pattern == '?')
		++pattern;

	return !*str && !*pattern;
}

int osd_dir_get_entry(void* void_h, char* name, int namelength, int* is_dir)
{
	struct dirio_handle* h = (struct dirio_handle*)void_h;
	struct dirent* d;

	d = readdir(h->h);

	while (d) {
		char file_buffer[FILE_MAXPATH];
		struct stat st;

		snprintf(file_buffer, sizeof(file_buffer), "%s/%s", h->dir, d->d_name);

		/* on any error ignore the file */

		if (stat(file_buffer, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				if (namelength >= strlen(d->d_name) + 1) {
					*is_dir = 1;
					sncpy(name, namelength, d->d_name);
					log_std(("osd: osd_dir_get_entry() -> %s\n", name));
					return strlen(name);
				}
			} else if (match(d->d_name, h->pattern)) {
				if (namelength >= strlen(d->d_name) + 1) {
					*is_dir = 0;
					sncpy(name, namelength, d->d_name);
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

static char FILEIO_CWD_BUFFER[FILE_MAXPATH];

const char* osd_get_cwd(void)
{
	char cwd_buffer[FILE_MAXPATH];

	log_std(("osd: osd_get_cdw()\n"));

	getcwd(cwd_buffer, sizeof(cwd_buffer));

	if (!cwd_buffer[0] || cwd_buffer[strlen(cwd_buffer)-1] != file_dir_slash())
		snprintf(FILEIO_CWD_BUFFER, sizeof(FILEIO_CWD_BUFFER), "%s%c", cwd_buffer, file_dir_slash());
	else
		snprintf(FILEIO_CWD_BUFFER, sizeof(FILEIO_CWD_BUFFER), "%s", cwd_buffer);

	log_std(("osd: osd_get_cwd() -> %s\n", FILEIO_CWD_BUFFER));

	return FILEIO_CWD_BUFFER;
}

int osd_select_file(mess_image *img, char *filename)
{
	log_std(("osd: osd_select_file(image:%p, filename:%s)\n", img, filename));

	return 0;
}

void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
	log_std(("osd: osd_image_load_status_changed(image:%p, is_final_unload:%d)\n", img, is_final_unload));
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

	log_std(("osd: osd_strip_extension(file:%s)\n", file));

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

	log_std(("osd: osd_dirname(file:%s)\n", file));

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

static char FILEIO_SEPARATOR[2];

const char *osd_path_separator(void)
{
	FILEIO_SEPARATOR[0] = file_dir_slash();
	FILEIO_SEPARATOR[1] = 0;
	return FILEIO_SEPARATOR;
}

int osd_is_path_separator(char ch)
{
	return ch == file_dir_slash();
}

int osd_is_absolute_path(const char *path)
{
	return file_path_is_abs(path);
}

void osd_begin_final_unloading(void)
{
	log_std(("osd: osd_begin_final_unloading()\n"));
}

void osd_config_save_xml(int type, struct _mame_file *file)
{
	log_std(("osd: osd_config_save_xml(type:%d,file:%p)\n", type, file));
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

adv_error advance_fileio_init(struct advance_fileio_context* context, adv_conf* cfg_context)
{
	struct fileio_item* i;
	for(i=FILEIO_CONFIG;i->type != FILETYPE_end;++i) {
		i->dir_map = 0;
		i->dir_mac = 0;
	}

	for(i=FILEIO_CONFIG;i->type != FILETYPE_end;++i) {
		if (i->config) {
			const char* def = 0;
			switch (i->mode) {
				case FILEIO_MODE_MULTI : def = file_config_dir_multidir(i->def); break;
				case FILEIO_MODE_SINGLE : def = file_config_dir_singledir(i->def); break;
				case FILEIO_MODE_FILE : def = file_config_dir_singlefile(); break;
			}
			if (def)
				conf_string_register_default(cfg_context, i->config, def);
		}
	}

	context->state.diff_handle = 0;

#ifdef MESS
	conf_string_register_default(cfg_context, "dir_crc", file_config_dir_singledir("crc"));
#endif

	return 0;
}

void advance_fileio_done(struct advance_fileio_context* context)
{
	struct fileio_item* i;
	for(i=FILEIO_CONFIG;i->type != FILETYPE_end;++i) {
		path_free(i->dir_map, i->dir_mac);
	}
	if (context->state.diff_handle) {
		fzclose(context->state.diff_handle);
	}
}

static void dir_create(const char* dir)
{
	struct stat st;
	if (stat(dir, &st) != 0) {
		log_std(("advance:fileio: creating dir %s\n", dir));
		if (file_dir_make(dir) != 0) {
			log_std(("advance:fileio: unable to create dir %s\n", dir));
		}
	}
}

/**
 * Create the default directories.
 */
void advance_fileio_default_dir(void)
{
	struct fileio_item* i;
	for(i=FILEIO_CONFIG;i->type != FILETYPE_end;++i) {
		if (i->config && i->def) {
			const char* def = 0;
			switch (i->mode) {
				case FILEIO_MODE_MULTI : def = file_config_dir_multidir(i->def); break;
				case FILEIO_MODE_SINGLE : def = file_config_dir_singledir(i->def); break;
				case FILEIO_MODE_FILE : def = file_config_dir_singlefile(); break;
			}
			if (def) {
				char** dir_map;
				unsigned dir_mac;
				unsigned j;

				path_allocate(&dir_map, &dir_mac, def);
				for(j=0;j<dir_mac;++j)
					dir_create(dir_map[j]);
				path_free(dir_map, dir_mac);
			}
		}
	}
}

adv_error advance_fileio_config_load(struct advance_fileio_context* context, adv_conf* cfg_context, struct mame_option* option)
{
	struct fileio_item* i;
	for(i=FILEIO_CONFIG;i->type != FILETYPE_end;++i) {
		/* free a previously loaded value */
		path_free(i->dir_map, i->dir_mac);
		i->dir_map = 0;
		i->dir_mac = 0;

		if (i->config) {
			unsigned j;
			const char* s = conf_string_get_default(cfg_context, i->config);
			const char* a;

			switch (i->mode) {
			case FILEIO_MODE_MULTI : a = file_config_list(s, file_config_dir_multidir, 0); break;
			case FILEIO_MODE_SINGLE : a = file_config_list(s, file_config_dir_singledir, 0); break;
			default: a = s; break;
			}

			log_std(("advance:fileio: %s %s\n", i->config, a));
			path_allocate(&i->dir_map, &i->dir_mac, a);
			for(j=0;j<i->dir_mac;++j)
				dir_create(i->dir_map[j]);
		} else {
			/* add the standard directories search as default */
			path_allocate(&i->dir_map, &i->dir_mac, file_config_dir_singlefile());
		}
	}

#ifdef MESS
	{
		const char* s = conf_string_get_default(cfg_context, "dir_crc");
		const char* a = file_config_list(s, file_config_dir_singledir, 0);
		log_std(("advance:fileio: %s %s\n", "dir_crc", a));
		sncpy(option->crc_dir_buffer, sizeof(option->crc_dir_buffer), a);
	}
#endif

	return 0;
}

