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

#include "file.h"
#include "snstring.h"

#include <dir.h>
#include <direct.h>

/** Number of universal buffers. */
#define UNIVERSAL_MAX 4

/** Size of a universal buffer. */
#define UNIVERSAL_SIZE 4096

struct file_context {
	char file_abs_buffer[FILE_MAXPATH]; /**< Absolute path returned by file_abs_buffer. */
	char file_home_buffer[FILE_MAXPATH];
	char list_buffer[FILE_MAXPATH*4];
	char universal_map[UNIVERSAL_MAX][UNIVERSAL_SIZE];
	unsigned universal_mac;
};

static struct file_context FL;

/***************************************************************************/
/* Init */

adv_error file_init(void)
{
	memset(&FL, 0, sizeof(FL));

	FL.universal_mac = 0;

	return 0;
}

void file_done(void)
{
}

/***************************************************************************/
/* File System */

char file_dir_separator(void)
{
	return ';';
}

char file_dir_slash(void)
{
	return '\\';
}

adv_error file_dir_make(const char* dir)
{
#ifdef __WIN32__
	return mkdir(dir);
#else
	return mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}

adv_bool file_path_is_abs(const char* path)
{
	return path[0] == '\\' || (path[0] != 0 && path[1]==':');
}

const char* file_abs(const char* dir, const char* file)
{
	if (file_path_is_abs(file)) {
		snprintf(FL.file_abs_buffer, sizeof(FL.file_abs_buffer), "%s", file);
	} else {
		while (file[0] == '.' && file[1] == '\\')
			file += 2;

		if (!dir[0] || dir[strlen(dir)-1] != '\\')
			snprintf(FL.file_abs_buffer, sizeof(FL.file_abs_buffer), "%s\\%s", dir, file);
		else
			snprintf(FL.file_abs_buffer, sizeof(FL.file_abs_buffer), "%s%s", dir, file);
	}

	return FL.file_abs_buffer;
}

static char* import_conv(char* dst_begin, char* dst_end, const char* begin, const char* end)
{
	adv_bool need_slash;

	if (end - begin == 2 && begin[1]==':') {
		/* only drive spec */
		memcpy(dst_begin, "/dos/", 5);
		dst_begin += 5;
		*dst_begin++ = tolower(begin[0]);
		begin += 2;
		need_slash = 1;
	} else if (end - begin >= 3 && begin[1]==':' && (begin[2]=='\\' || begin[2]=='/')) {
		/* absolute path */
		memcpy(dst_begin, "/dos/", 5);
		dst_begin += 5;
		*dst_begin++ = tolower(begin[0]);
		begin += 3;
		need_slash = 1;
	} else if (end - begin >= 3 && begin[1]==':') {
		/* drive + relative path, assume it's an absolute path */
		memcpy(dst_begin, "/dos/", 5);
		dst_begin += 5;
		*dst_begin++ = tolower(begin[0]);
		begin += 2;
		need_slash = 1;
	} else if (end - begin >= 1 && (begin[0]=='\\' || begin[0]=='/')) {
		/* absolute path on the current drive */
		memcpy(dst_begin, "/dos/", 5);
		dst_begin += 5;
#ifdef __MSDOS__
		*dst_begin++ = getdisk() + 'a';
#else
		*dst_begin++ = _getdrive() + 'a';
#endif
		begin += 1;
		need_slash = 1;
	} else {
		/* relative path, keep it relative */
		need_slash = 0;
	}

	/* add a slash if something other is specified */
	if (need_slash && begin != end) {
		*dst_begin++ = '/';
	}

	/* copy the remaining path */
	while (begin != end) {
		if (*begin=='\\')
			*dst_begin++ = '/';
		else
			*dst_begin++ = tolower(*begin);
		++begin;
	}

	return dst_begin;
}

static char* export_conv(char* dst_begin, char* dst_end, const char* begin, const char* end)
{

	if (end - begin == 6 && memcmp(begin, "/dos/", 5)==0) {
		/* root dir */
		*dst_begin++ = begin[5];
		*dst_begin++ = ':';
		*dst_begin++ = '\\';
		begin += 6;
	} else if (end - begin >= 7 && memcmp(begin, "/dos/", 5)==0) {
		/* absolute path */
		*dst_begin++ = begin[5];
		*dst_begin++ = ':';
		*dst_begin++ = '\\';
		begin += 7;
	} else {
		/* relative path */
	}

	/* copy the remaining path */
	while (begin != end) {
		if (*begin=='/')
			*dst_begin++ = '\\';
		else
			*dst_begin++ = *begin;
		++begin;
	}

	return dst_begin;
}

const char* file_import(const char* path)
{
	char* buffer = FL.universal_map[FL.universal_mac];
	const char* begin = path;
	char* dst_begin = buffer;
	char* dst_end = buffer + UNIVERSAL_SIZE;

	FL.universal_mac = (FL.universal_mac + 1) % UNIVERSAL_MAX;

	while (*begin) {
		const char* end = strchr(begin, ';');
		if (!end)
			end = begin + strlen(begin);
		if (dst_begin != buffer)
			*dst_begin++ = ':';
		dst_begin = import_conv(dst_begin, dst_end, begin, end);
		begin = end;
		if (*begin)
			++begin;
	}

	*dst_begin = 0;

	return buffer;
}

const char* file_export(const char* path)
{
	char* buffer = FL.universal_map[FL.universal_mac];
	const char* begin = path;
	char* dst_begin = buffer;
	char* dst_end = buffer + UNIVERSAL_SIZE;

	FL.universal_mac = (FL.universal_mac + 1) % UNIVERSAL_MAX;

	while (*begin) {
		const char* end = strchr(begin, ':');
		if (!end)
			end = begin + strlen(begin);
		if (dst_begin != buffer)
			*dst_begin++ = ';';
		dst_begin = export_conv(dst_begin, dst_end, begin, end);
		begin = end;
		if (*begin)
			++begin;
	}

	*dst_begin = 0;

	return buffer;
}

/***************************************************************************/
/* Files */

const char* file_config_file_host(const char* file)
{
	return 0;
}

const char* file_config_file_data(const char* file)
{
	return 0;
}

const char* file_config_file_home(const char* file)
{
	snprintf(FL.file_home_buffer, sizeof(FL.file_home_buffer), "%s", file);
	return FL.file_home_buffer;
}

const char* file_config_dir_multidir(const char* tag)
{
	return tag;
}

const char* file_config_dir_singledir(const char* tag)
{
	return tag;
}

const char* file_config_dir_singlefile(void)
{
	return ".";
}

const char* file_config_list(const char* const_list, const char* (*expand_dir)(const char* tag), const char* ref_dir)
{
	char* list = strdup(const_list);
	int i;

	FL.list_buffer[0] = 0;

	i = 0;
	while (list[i]) {
		char c;
		const char* file;

		file = stoken(&c, &i, list, ";", "");

		if (FL.list_buffer[0])
			sncat(FL.list_buffer, sizeof(FL.list_buffer), ";");

		if (ref_dir)
			sncat(FL.list_buffer, sizeof(FL.list_buffer), file_abs(ref_dir, file));
		else
			sncat(FL.list_buffer, sizeof(FL.list_buffer), file);
	}

	free(list);

	return FL.list_buffer;
}


