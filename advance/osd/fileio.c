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

#include "advance.h"
#include "unzip.h"
#include "conf.h"
#include "fz.h"
#include "log.h"

#include "mame2.h"

#include <zlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <assert.h>
#include <dirent.h>

/***************************************************************************/
/* Declaration */

/** File handle used by all the osd_* functions. */
struct fileio_handle {
	FZ* f; /**< File opened. */

	unsigned crc; /**< Current crc value of the file. */
	int is_crc_set; /**< If the crc was computed. */
};

/** Directory handle used by all the osd_dir_* functions. */
struct dirio_handle {
	DIR* h; /**< Dir opened. */

	char* dir; /**< Name of the directory opened. */
	char* pattern; /** Pattern applyed at the directory. */
};

#define FILEIO_MODE_DIRGAME 0 /**< Single file named like the game in a single dir. */
#define FILEIO_MODE_DIRNAME 1 /**< Single file in a single dir. */
#define FILEIO_MODE_NAME 2 /**< Single file named specifically in the root dir. */
#define FILEIO_MODE_COLLECTION 3 /**< Collection of files. */

#define FILEIO_OPEN_NONE 0 /**< Open not allowed. */
#define FILEIO_OPEN_READ 1 /**< Open for reading "rb". */
#define FILEIO_OPEN_WRITE 2 /**< Open for writing "wb". */
#define FILEIO_OPEN_READWRITE 3 /**< Open for reading and writing "r+b". */

struct fileio_item {
	int type; /**< OSD_FILETYPE_* */

	const char* config; /**< Configuration directory tag */
	const char* def; /**< Default directory */
	const char* extension; /**< Default extension if any, otherwise 0 */

	unsigned mode; /**< FILEIO_MODE_* */

	unsigned open_0; /**< Open mode for mode 0 (arg of osd_fopen) */
	unsigned open_1; /**< Open mode for mode 1 (arg of osd_fopen) */

	unsigned dir_mac; /**< Number of directories */
	char** dir_map; /**< Vector of directories */
};

static struct fileio_item CONFIG[] = {
	{ OSD_FILETYPE_ROM, "dir_rom", "rom", 0, FILEIO_MODE_COLLECTION, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 },
/* TODO implementare in OSD_FILETYPE_ROM_NOCRC la lettura senza crc */
/*	{ OSD_FILETYPE_ROM_NOCRC, "dir_rom", "rom", 0, FILEIO_MODE_COLLECTION, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 }, */
	{ OSD_FILETYPE_IMAGE_R, "dir_imager", "image", ".chd", FILEIO_MODE_COLLECTION, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 },
	{ OSD_FILETYPE_IMAGE_RW, "dir_imagerw", "image", ".chd", FILEIO_MODE_COLLECTION, FILEIO_OPEN_READWRITE, FILEIO_OPEN_READWRITE, 0, 0 },
	{ OSD_FILETYPE_IMAGE_DIFF, "dir_imagediff", "image", ".dif", FILEIO_MODE_COLLECTION, FILEIO_OPEN_READWRITE, FILEIO_OPEN_READWRITE, 0, 0 },
	{ OSD_FILETYPE_SAMPLE, "dir_sample", "sample", ".wav", FILEIO_MODE_COLLECTION, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 },
	{ OSD_FILETYPE_ARTWORK, "dir_artwork", "artwork", ".png", FILEIO_MODE_COLLECTION, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 },
	{ OSD_FILETYPE_NVRAM, "dir_nvram" , "nvram", ".nv", FILEIO_MODE_DIRGAME, FILEIO_OPEN_READ, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_HIGHSCORE, "dir_hi", "hi", ".hi", FILEIO_MODE_DIRGAME, FILEIO_OPEN_READ, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_HIGHSCORE_DB, 0, 0, 0, FILEIO_MODE_NAME, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 }, /* used for hiscore.dat */
	{ OSD_FILETYPE_CONFIG, "dir_cfg", "cfg", ".cfg", FILEIO_MODE_DIRGAME, FILEIO_OPEN_READ, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_INPUTLOG, "dir_inp", "inp", ".inp", FILEIO_MODE_DIRNAME, FILEIO_OPEN_READ, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_STATE, "dir_sta", "sta", ".sta", FILEIO_MODE_DIRGAME, FILEIO_OPEN_READ, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_MEMCARD, "dir_memcard", "memcard", ".mem", FILEIO_MODE_DIRNAME, FILEIO_OPEN_READ, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_SCREENSHOT, "dir_snap", "snap", ".png", FILEIO_MODE_DIRNAME, FILEIO_OPEN_NONE, FILEIO_OPEN_WRITE, 0, 0 },
	{ OSD_FILETYPE_HISTORY, 0, 0, 0, FILEIO_MODE_NAME, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 }, /* used for history.dat, mameinfo.dat, safequit.dat */
	{ OSD_FILETYPE_CHEAT, 0, 0, 0, FILEIO_MODE_NAME, FILEIO_OPEN_READ, FILEIO_OPEN_READWRITE, 0, 0 }, /* used for cheat.dat */
	{ OSD_FILETYPE_LANGUAGE, 0, 0, 0, FILEIO_MODE_NAME, FILEIO_OPEN_READ, FILEIO_OPEN_NONE, 0, 0 }, /* used for language file */
	{ OSD_FILETYPE_end, 0, 0, 0, 0, 0, 0 }
};

/***************************************************************************/
/* Internal interface */

static int item_has_ext(const char* file) {
	const char* slash = strrchr(file,file_dir_slash());
	const char* dot = strrchr(file,'.');

	return dot!=0 && (slash==0 || slash < dot);
}

static int item_is_match(const char* str, const char* pattern) {
	while (*str && *pattern) {
		if (*pattern == '*') {
			if (item_is_match(str+1,pattern))
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

static int item_is_partialequal(const char* zipfile, const char* file) {
	const char* s1 = file;
	/* start comparison after last / */
	const char* s2 = strrchr(zipfile,'/');
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

static int item_open_raw(const char* file, const char* ext, const char* mode, struct fileio_handle* h) {
	char file_complete[FILE_MAXPATH];

	if (ext && !item_has_ext(file))
		sprintf(file_complete,"%s%s",file,ext);
	else
		strcpy(file_complete,file);

	log_std(("osd: osd_fopen() try %s\n",file_complete));

	h->crc = 0;
	h->is_crc_set = 0;
	h->f = fzopen(file_complete,mode);
	if (!h->f)
		return -1;

	log_std(("osd: osd_fopen() -> %s\n",file_complete));

	return 0;
}

static int item_open_zip(const char* zip_file, const char* file, const char* ext, struct fileio_handle* h) {
	ZIP* zip;
	struct zipent* ent;
	int found;
	char file_complete[FILE_MAXPATH];

	if (ext && !item_has_ext(file))
		sprintf(file_complete,"%s%s",file,ext);
	else
		strcpy(file_complete,file);

	log_std(("osd: osd_fopen() try %s/%s\n",zip_file,file_complete));

	if (access(zip_file,R_OK)!=0)
		return -1;

	zip = openzip(zip_file);
	if (!zip)
		return -1;

	found = 0;
	while (!found && (ent = readzip(zip))!=0) {
		char crc[9];
		sprintf(crc,"%08x",ent->crc32);
		if (item_is_partialequal(ent->name, file_complete) || (ent->crc32 && strcmp(crc, file)==0)) {
			if (ent->compression_method == 0) {
				h->crc = ent->crc32;
				h->is_crc_set = 1;
				h->f = fzopenzipuncompressed(zip_file,ent->offset_lcl_hdr_frm_frst_disk,ent->uncompressed_size);
				found = 1;
			} else if (ent->compression_method == 8) {
				h->crc = ent->crc32;
				h->is_crc_set = 1;
				h->f = fzopenzipcompressed(zip_file,ent->offset_lcl_hdr_frm_frst_disk,ent->compressed_size,ent->uncompressed_size);
				found = 1;
			}
		}
	}

	closezip(zip);

	if (!found)
		return -1;

	log_std(("osd: osd_fopen() -> %s/%s\n",zip_file,file_complete));

	return 0;
}

static struct fileio_handle* item_open(int type, const char* game, const char* name, unsigned mode) {
	int i;
	struct fileio_handle* h = malloc(sizeof(struct fileio_handle));
	const char* open_mode;

	/* find the item */
	const struct fileio_item* item = CONFIG;
	while (item->type != OSD_FILETYPE_end && item->type != type)
		++item;
	if (!item->type == OSD_FILETYPE_end) {
		log_std(("ERROR: osd_fopen() invalid open with unknow type\n"));
		return 0;
	}

	switch (mode) {
	case 0 :
		mode = item->open_0;
		break;
	case 1 :
		mode = item->open_1;
		break;
	default:
		log_std(("ERROR: osd_fopen() invalid open in unknow mode\n"));
		return 0;
	}

	switch (mode) {
	case FILEIO_OPEN_NONE :
		log_std(("ERROR: osd_fopen() invalid open in unsupported mode\n"));
		return 0;
	case FILEIO_OPEN_READ :
		open_mode = "rb";
		break;
	case FILEIO_OPEN_WRITE :
		open_mode = "wb";
		break;
	case FILEIO_OPEN_READWRITE :
		open_mode = "r+b";
		break;
	default:
		log_std(("ERROR: osd_fopen() invalid open in not specified mode\n"));
		return 0;
	}

	h->crc = 0;
	h->is_crc_set = 0;
	h->f = 0;

	for(i=0;!h->f && i<item->dir_mac;++i) {

		/* file not compressed */
		if (!h->f
			&& item->mode == FILEIO_MODE_DIRGAME
			&& game) {
			char file[FILE_MAXPATH];
			strcpy(file, file_abs(item->dir_map[i],game));
			item_open_raw(file, item->extension, open_mode, h);
		}

		/* file not compressed */
		if (!h->f
			&& (item->mode == FILEIO_MODE_DIRNAME || item->mode == FILEIO_MODE_NAME)
			&& name) {
			char file[FILE_MAXPATH];
			strcpy(file, file_abs(item->dir_map[i],name));
			item_open_raw(file, item->extension, open_mode, h);
		}

		/* file not compressed */
		if (!h->f
			&& item->mode == FILEIO_MODE_COLLECTION
			&& game
			&& name) {
			char file[FILE_MAXPATH];
			char sub[FILE_MAXPATH];
			sprintf(sub,"%s%c%s",game,file_dir_slash(),name);
			strcpy(file,file_abs(item->dir_map[i],sub));
			item_open_raw(file, item->extension, open_mode, h);
		}

		/* file compressed in a zip named as the game */
		if (!h->f
			&& item->mode == FILEIO_MODE_COLLECTION
			&& mode == FILEIO_OPEN_READ
			&& game
			&& name) {
			char file[FILE_MAXPATH];
			char sub[FILE_MAXPATH];
			sprintf(sub,"%s.zip",game);
			strcpy(file, file_abs(item->dir_map[i], sub));
			item_open_zip(file, name, item->extension, h);
		}

#ifdef MESS
		/* file compressed in a zip named as the file */
		if (!h->f
			&& item->mode == FILEIO_MODE_COLLECTION
			&& mode == FILEIO_OPEN_READ
			&& game
			&& name
			&& strchr(name,'=')==0) {

			char zip_file[FILE_MAXPATH];
			char file[FILE_MAXPATH];
			char sub[FILE_MAXPATH];
			const char* end;

			end = strchr(name,'.');
			if (!end)
				end = name + strlen(name);

			strncpy(file, name, end - name);
			file[end - name] = 0;

			sprintf(sub, "%s%c%s.zip", game, file_dir_slash(), file);
			strcpy(zip_file, file_abs(item->dir_map[i], sub));
			item_open_zip(zip_file, name, item->extension, h);
		}

		/* file compressed in a zip with the specified name */
		/* for example alpiner=alpinerc.bin search alpinerc.bin in alpiner.zip */
		if (!h->f
			&& item->mode == FILEIO_MODE_COLLECTION
			&& mode == FILEIO_OPEN_READ
			&& game
			&& name
			&& strchr(name,'=')!=0) {

			char zip_file[FILE_MAXPATH];
			char file[FILE_MAXPATH];
			char sub[FILE_MAXPATH];
			const char* end;
			const char* real_name;

			end = strchr(name,'=');
			real_name = end + 1;

			strncpy(file, name, end - name);
			file[end - name] = 0;

			sprintf(sub, "%s%c%s.zip", game, file_dir_slash(), file);
			strcpy(zip_file, file_abs(item->dir_map[i], sub));
			item_open_zip(zip_file, real_name, item->extension, h);
		}
#endif
	}

	if (!h->f) {
		log_std(("osd: osd_fopen() leave with failure\n"));
		free(h);
		return 0;
	}

	return h;
}

static void item_close(struct fileio_handle* f) {
	if (f)
		fzclose(f->f);
	free(f);
}

static int item_stat(int type, const char* name) {
	int i;
	int found;

	/* find the item */
	const struct fileio_item* item = CONFIG;
	while (item->type != OSD_FILETYPE_end && item->type != type)
		++item;
	if (!item->type == OSD_FILETYPE_end) {
		return -1;
	}

	found = 0;
	for(i=0;!found && i<item->dir_mac;++i) {
		char file[FILE_MAXPATH];
		char path[FILE_MAXPATH];
		struct stat st;

		switch (item->mode) {
			case FILEIO_MODE_DIRGAME :
			case FILEIO_MODE_DIRNAME :
			case FILEIO_MODE_NAME :
				sprintf(file,"%s",name);
				if (item->extension && !item_has_ext(file))
					strcat(file,item->extension);
				sprintf(path,"%s/%s",item->dir_map[i],file);

				log_std(("osd: osd_faccess() try %s\n",path));

				found = stat(path,&st) == 0 && !S_ISDIR(st.st_mode);
			break;
                        case FILEIO_MODE_COLLECTION :
				sprintf(file,"%s",name);
				sprintf(path,"%s/%s",item->dir_map[i],file);

				log_std(("osd: osd_faccess() try %s\n",path));

				found = stat(path,&st) == 0 && S_ISDIR(st.st_mode);
				if (!found) {
					sprintf(path,"%s/%s.zip",item->dir_map[i],name);

					log_std(("osd: osd_faccess() try %s\n",path));

					found = stat(path,&st) == 0 && !S_ISDIR(st.st_mode);
				}
			break;
		}
	}

	if (!found)
		return -1;

	return 0;
}

/***************************************************************************/
/* OSD interface */

/**
 * Check if roms/samples for a game exist at all.
 * \return
 *   !=0 if exist
 *   ==0 if not exist
 */
int osd_faccess(const char *newfilename, int filetype)
{
	log_std(("osd: osd_faccess(newfilename:%s,filetype:%d)\n",newfilename,filetype));

	/* used for audit and also for screenshot saving */
	return item_stat(filetype,newfilename) == 0 ? 1 : 0;
}

void* osd_fopen(const char *game, const char *filename, int filetype, int mode)
{
	log_std(("osd: osd_fopen(game:%s, filename:%s, filetype:%d, mode:%d)\n",game,filename,filetype,mode));

	return item_open(filetype, game, filename, mode);
}

int osd_fread(void* void_h, void* buffer, int length)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_pedantic(("osd: osd_fread(void_h, buffer, length:%d)\n",length));

	return fzread(buffer,1,length,h->f);
}

int osd_fread_swap(void* void_h, void* buffer, int length)
{
	int i;
	unsigned char* buf = (unsigned char*)buffer;
	int res;

	log_debug(("osd: osd_fread_swap(void_h, buffer, length:%d)\n",length));

	res = osd_fread(void_h, buf, length);

	for(i = 0; i < length; i += 2) {
		unsigned char t = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = t;
	}

	return res;
}

int osd_fwrite(void* void_h, const void* buffer, int length)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_pedantic(("osd: osd_fwrite(void_h, buffer, length:%d)\n",length));

	return fzwrite(buffer, 1, length, h->f);
}

int osd_fwrite_swap(void* void_h, const void *buffer, int length)
{
	int i;
	unsigned char *buf = (unsigned char *)buffer;
	int res;

	log_debug(("osd: osd_fwrite_swap(void_h, buffer, length:%d)\n",length));

	for(i=0;i<length;i+=2) {
		unsigned char temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	res = osd_fwrite(void_h, buf, length);

	for(i=0;i<length;i+=2) {
		unsigned char temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}

int osd_fread_scatter(void* void_h, void* buffer, int length, int increment)
{
	unsigned char* buf = (unsigned char*)buffer;
	unsigned done = 0;

	log_debug(("osd: osd_fread_scatter(void_h, buffer, length:%d, increment:%d)\n",length,increment));

	while (done < length) {
		unsigned i;
		unsigned char data[4096];
		unsigned run = length - done;
		if (run > sizeof(data))
			run = sizeof(data);
		run = osd_fread(void_h, data, run);
		if (run == 0)
			return done;
		for(i = 0; i < run; i++) {
			*buf = data[i];
			buf += increment;
		}
		done += run;
	}

	return done;
}

int osd_fseek(void* void_h, int offset, int whence)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;
	int r;

	log_debug(("osd: osd_fseek(void_h, offset:%d, whence:%d)\n",offset,whence));

	r = fzseek(h->f, offset, whence);;

	log_debug(("osd: osd_fseek() -> %d\n",r));

	return r;
}

void osd_fclose(void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_debug(("osd: osd_fclose(void_h)\n"));

	item_close(h);
}

int osd_fchecksum(const char* game, const char* filename, unsigned int* length, unsigned int* sum)
{
	(void)game;
	(void)filename;
	(void)length;
	(void)sum;

	log_debug(("osd: osd_fchecksm(game:%s,  filename:%s, length, sum)\n",game,filename));

	assert( 0 );

	/* used only for audit pourpuse */
	return -1;
}

int osd_fsize(void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_debug(("osd: osd_fsize(void_h)\n"));

	return fzsize(h->f);
}

unsigned int osd_fcrc(void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_debug(("osd: osd_fcrc(void_h)\n"));

	if (!h->is_crc_set) {
		unsigned char data[4096];
		unsigned size = fzsize(h->f);

		h->crc = 0;

		/* restart */
		fzseek(h->f, 0, SEEK_SET);

		while (size) {
			unsigned run = sizeof(data);
			if (run > size)
				run = size;
			if (fzread(data,run,1,h->f)!=1) {
				break;
			}
			size -= run;
			h->crc = crc32(h->crc,data,run);
		}

		if (size == 0) {
			h->is_crc_set = 1;
		} else {
			h->crc = 0;
		}

		/* restart */
		fzseek(h->f, 0, SEEK_SET);
	}

	log_std(("osd: osd_fcrc() -> %08x\n",h->crc));

	return h->crc;
}

int osd_fgetc(void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_pedantic(("osd: osd_fgetc(void_h)\n"));

	return fzgetc(h->f);
}

int osd_ungetc(int c, void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_pedantic(("osd: osd_ungetc(c:%d,void_h)\n",c));

	return fzungetc(c, h->f);
}

char *osd_fgets(char* s, int n, void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_pedantic(("osd: osd_fgets(s,n,void_h)\n"));

	return fzgets(s, n, h->f);
}

int osd_feof(void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;

	log_pedantic(("osd: osd_feof(void_h)\n"));

	return fzeof(h->f);
}

int osd_ftell(void* void_h)
{
	struct fileio_handle* h = (struct fileio_handle*)void_h;
	int r;

	log_debug(("osd: osd_ftell(void_h)\n"));

	r = fztell(h->f);

	log_debug(("osd: osd_ftell() -> %d\n", r));

	return r;
}

/**
 * Called while loading ROMs.
 * It is called a last time with name == 0 to signal that the ROM loading
 * process is finished.
 * \return
 *  - !=0 to abort loading
 *  - ==0 on success
 */
int osd_display_loading_rom_message(const char *name, int current, int total)
{
	(void)name;
	(void)current;
	(void)total;

	log_debug(("osd: osd_display_loading_rom_message(name:%s,current:%d,total:%d)\n",name,current,total));

	/* nothing */

	return 0;
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
	log_std(("osd: osd_change_devicee(device:%s)\n",device));
}

const char *osd_get_device_name(int i)
{
	log_std(("osd: osd_get_device_name(i:%d)\n",i));
	return "";
}

void* osd_dir_open(const char* dir, const char* mask)
{
	struct dirio_handle* h;

	log_std(("osd: osd_dir_open(dir:%s,mask:%s)\n",dir,mask));

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

		sprintf(file,"%s/%s",h->dir,d->d_name);

		/* on any error ignore the file */

		if (stat(file,&st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				if (namelength >= strlen(d->d_name) + 1) {
					*is_dir = 1;
					strcpy(name,d->d_name);

					log_std(("osd: osd_dir_get_entry() -> %s\n",name));

					return strlen(name);
				}
			} else if (item_is_match(d->d_name, h->pattern)) {
				if (namelength >= strlen(d->d_name) + 1) {
					*is_dir = 0;
					strcpy(name,d->d_name);

					log_std(("osd: osd_dir_get_entry() -> %s\n",name));

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
	log_std(("osd: osd_change_directory(dir:%s)\n",dir));

	chdir(dir);
}

const char* osd_get_cwd(void)
{
	static char cwd[FILE_MAXPATH];

	log_std(("osd: osd_get_cdw()\n"));

	getcwd(cwd, sizeof(cwd));
	strcat(cwd, "/");

	log_std(("osd: osd_get_cwd() -> %s\n",cwd));

	return cwd;
}

int osd_select_file(int sel, char* filename)
{
	log_std(("osd: osd_select_file(sel:%d,filename:%s)\n",sel,filename));

	return 0;
}

char* osd_basename(char* filename)
{
	char* base;

	log_std(("osd: osd_basename(filename:%s)\n",filename));

	base = strrchr(filename, os_dir_slash());
	if (base)
		++base;
	else
		base = filename;

	log_std(("osd: osd_basename() -> %s\n",base));

	return base;
}

#endif

/***************************************************************************/
/* Advance interface */

static int path_allocate(char*** dir_map, unsigned* dir_mac, const char* path)
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

static void path_free(char** dir_map, unsigned dir_mac) {
	int i;
	for(i=0;i<dir_mac;++i)
		free(dir_map[i]);
	free(dir_map);
}

int advance_fileio_init(struct conf_context* context) {

	struct fileio_item* i;
	for(i=CONFIG;i->type != OSD_FILETYPE_end;++i) {
		i->dir_map = 0;
		i->dir_mac = 0;
	}

	for(i=CONFIG;i->type != OSD_FILETYPE_end;++i) {
		if (i->config) {
			const char* def = 0;
			switch (i->mode) {
				case FILEIO_MODE_COLLECTION : def = file_config_dir_multidir(i->def); break;
				case FILEIO_MODE_DIRGAME : def = file_config_dir_singledir(i->def); break;
				case FILEIO_MODE_DIRNAME : def = file_config_dir_singledir(i->def); break;
				case FILEIO_MODE_NAME : def = file_config_dir_singlefile(); break;
			}
			if (def)
				conf_string_register_default(context, i->config, def);
		}
	}

#ifdef MESS
	conf_string_register_default(context, "dir_crc", os_config_dir_singledir("crc"));
#endif

	return 0;
}

void advance_fileio_done(void) {
	struct fileio_item* i;
	for(i=CONFIG;i->type != OSD_FILETYPE_end;++i) {
		path_free(i->dir_map,i->dir_mac);
	}
}

int advance_fileio_config_load(struct conf_context* context, struct mame_option* option) {
	struct fileio_item* i;
	for(i=CONFIG;i->type != OSD_FILETYPE_end;++i) {
		/* free a previously loaded value */
		path_free(i->dir_map,i->dir_mac);
		i->dir_map = 0;
		i->dir_mac = 0;

		if (i->config) {
			const char* s = conf_string_get_default(context, i->config);
			log_std(("advance:fileio: %s %s\n", i->config, s));
			path_allocate(&i->dir_map,&i->dir_mac,s);
		} else {
			/* add the standard directories search as default */
			path_allocate(&i->dir_map,&i->dir_mac,file_config_dir_singlefile());
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

