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
#include "target.h"
#include "snstring.h"

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#ifndef ADV_DATADIR
#error Macro ADV_DATADIR undefined
#endif

#ifndef ADV_SYSCONFDIR
#error Macro ADV_SYSCONFDIR undefined
#endif

struct file_context {
	char file_abs_buffer[FILE_MAXPATH]; /**< Absolute path returned by file_abs_buffer. */
	char data_dir_buffer[FILE_MAXPATH]; /**< Data directory. */
	char home_dir_buffer[FILE_MAXPATH]; /**< Home directory. */
	char current_dir_buffer[FILE_MAXPATH]; /**< Current directory. */
	char dir_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
	char file_host_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
	char file_data_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
	char file_home_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
	char list_buffer[FILE_MAXPATH*4]; /**< Static buffer for the returned strings. */
};

static struct file_context FL;

/***************************************************************************/
/* Init */

adv_error file_init(void)
{
	char* home;

	memset(&FL, 0, sizeof(FL));

	/* current */
	if (getcwd(FL.current_dir_buffer, sizeof(FL.current_dir_buffer)) == 0) {
		target_err("Unable to get the current directory.\n");
		return -1;
	}
	/* clear the leading slash if present */
	if (FL.current_dir_buffer[0] && FL.current_dir_buffer[strlen(FL.current_dir_buffer)-1]=='/')
		FL.current_dir_buffer[strlen(FL.current_dir_buffer)-1] = 0;


	/* root */
	snprintf(FL.data_dir_buffer, sizeof(FL.data_dir_buffer), "%s", ADV_DATADIR);

	/* home */

	/* try $ADVANCE */
	home = getenv("ADVANCE");
	if (home) {
		snprintf(FL.home_dir_buffer, sizeof(FL.home_dir_buffer), "%s", home);
	} else {
		/* try $HOME/.advance */
		home = getenv("HOME");
		if (home) {
			/* add the .advance subdirectory */
			if (!home[0] || home[strlen(home)-1] != '/')
				snprintf(FL.home_dir_buffer, sizeof(FL.home_dir_buffer), "%s/.advance", home);
			else
				snprintf(FL.home_dir_buffer, sizeof(FL.home_dir_buffer), "%s.advance", home);
		} else {
			/* use ROOT */
			snprintf(FL.home_dir_buffer, sizeof(FL.home_dir_buffer), "%s", FL.data_dir_buffer);

			/* clear the root dir */
			FL.data_dir_buffer[0] = 0;
		}
	}

	if (!FL.home_dir_buffer[0]) {
		target_err("Undefined $home directory.\nCheck your HOME and ADVANCE environment variables.\n");
		return -1;
	}

	/* clear the leading slash if present */
	if (FL.home_dir_buffer[0] && FL.home_dir_buffer[strlen(FL.home_dir_buffer)-1]=='/')
		FL.home_dir_buffer[strlen(FL.home_dir_buffer)-1] = 0;

	/* create the dir */
	if (FL.home_dir_buffer[0]) {
		if (mkdir(FL.home_dir_buffer, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
			if (errno != EEXIST) {
				target_err("Error creating the $home directory %s.\n", FL.home_dir_buffer);
				return -1;
			}
		}
	}

	return 0;
}

void file_done(void)
{
}

/***************************************************************************/
/* File System */

char file_dir_separator(void)
{
	return ':';
}

char file_dir_slash(void)
{
	return '/';
}

adv_error file_dir_make(const char* dir)
{
	return mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

adv_bool file_path_is_abs(const char* file)
{
	return file[0] == '/';
}

const char* file_abs(const char* dir, const char* file)
{
	/* TODO implement the complete . and .. management */
	if (file[0] == '/') {
		snprintf(FL.file_abs_buffer, sizeof(FL.file_abs_buffer), "%s", file);
	} else {
		if (!dir[0] || dir[strlen(dir)-1] != '/')
			snprintf(FL.file_abs_buffer, sizeof(FL.file_abs_buffer), "%s/%s", dir, file);
		else
			snprintf(FL.file_abs_buffer, sizeof(FL.file_abs_buffer), "%s%s", dir, file);
	}
	return FL.file_abs_buffer;
}

const char* file_import(const char* path)
{
	return path;
}

const char* file_export(const char* path)
{
	return path;
}

/***************************************************************************/
/* Files */

const char* file_config_file_host(const char* file)
{
	if (strncmp(file, "./", 2) == 0)
		snprintf(FL.file_host_buffer, sizeof(FL.file_host_buffer), "%s/%s", FL.current_dir_buffer, file + 2);
	else if (file[0] == '/')
		snprintf(FL.file_host_buffer, sizeof(FL.file_host_buffer), "%s", file);
	else {
		/* if relative add the root data dir */
		const char* sysconfdir;
		/* never use /usr/etc */
		if (strcmp(ADV_SYSCONFDIR, "/usr/etc") == 0)
			sysconfdir = "/etc";
		else
			sysconfdir = ADV_SYSCONFDIR;
		snprintf(FL.file_host_buffer, sizeof(FL.file_host_buffer), "%s/%s", sysconfdir, file);
	}
	return FL.file_host_buffer;
}

const char* file_config_file_data(const char* file)
{
	if (FL.data_dir_buffer[0]) {
		if (strncmp(file, "./", 2) == 0)
			snprintf(FL.file_data_buffer, sizeof(FL.file_data_buffer), "%s/%s", FL.current_dir_buffer, file + 2);
		else if (file[0] == '/')
			snprintf(FL.file_data_buffer, sizeof(FL.file_data_buffer), "%s", file);
		else
			/* if relative add the root data dir */
			snprintf(FL.file_data_buffer, sizeof(FL.file_data_buffer), "%s/%s", FL.data_dir_buffer, file);
		return FL.file_data_buffer;
	} else {
		return 0;
	}
}

const char* file_config_file_home(const char* file)
{
	if (strncmp(file, "./", 2) == 0)
		snprintf(FL.file_home_buffer, sizeof(FL.file_home_buffer), "%s/%s", FL.current_dir_buffer, file + 2);
	else if (file[0] == '/')
		snprintf(FL.file_home_buffer, sizeof(FL.file_home_buffer), "%s", file);
	else
		/* if relative add the home data dir */
		snprintf(FL.file_home_buffer, sizeof(FL.file_home_buffer), "%s/%s", FL.home_dir_buffer, file);
	return FL.file_home_buffer;
}

const char* file_config_dir_multidir(const char* tag)
{
	assert(tag[0] != '/');
	if (FL.data_dir_buffer[0])
		snprintf(FL.dir_buffer, sizeof(FL.dir_buffer), "%s/%s:%s/%s", FL.home_dir_buffer, tag, FL.data_dir_buffer, tag);
	else
		snprintf(FL.dir_buffer, sizeof(FL.dir_buffer), "%s/%s", FL.home_dir_buffer, tag);
	return FL.dir_buffer;
}

const char* file_config_dir_singledir(const char* tag)
{
	assert(tag[0] != '/');
	snprintf(FL.dir_buffer, sizeof(FL.dir_buffer), "%s/%s", FL.home_dir_buffer, tag);
	return FL.dir_buffer;
}

const char* file_config_dir_singlefile(void)
{
	if (FL.data_dir_buffer[0])
		snprintf(FL.dir_buffer, sizeof(FL.dir_buffer), "%s:%s", FL.home_dir_buffer, FL.data_dir_buffer);
	else
		snprintf(FL.dir_buffer, sizeof(FL.dir_buffer), "%s", FL.home_dir_buffer);
	return FL.dir_buffer;
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

		file = stoken(&c, &i, list, ":", "");

		if (FL.list_buffer[0])
			sncat(FL.list_buffer, sizeof(FL.list_buffer), ":");

		if (strncmp(file, "./", 2) == 0)
			sncatf(FL.list_buffer, sizeof(FL.list_buffer), "%s/%s", FL.current_dir_buffer, file + 2);
		else if (file[0] == '/')
			sncat(FL.list_buffer, sizeof(FL.list_buffer), file);
		else
			sncat(FL.list_buffer, sizeof(FL.list_buffer), expand_dir(file));
	}

	free(list);

	return FL.list_buffer;
}

