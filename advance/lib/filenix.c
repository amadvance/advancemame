/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002 Andrea Mazzoleni
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

#include "file.h"
#include "target.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#ifndef PREFIX
#error Macro PREFIX undefined
#endif

struct file_context {
	char root_dir[FILE_MAXPATH]; /**< Root directory. */
	char home_dir[FILE_MAXPATH]; /**< Home directory. */
	char dir_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
	char file_root_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
	char file_home_buffer[FILE_MAXPATH]; /**< Static buffer for the returned strings. */
};

static struct file_context FL;

/***************************************************************************/
/* Init */

static void strcatslash(char* str) {
	if (str[0] && str[strlen(str)-1] !='/')
		strcat(str,"/");
}

int file_init(void) {
	char* home;

	memset(&FL,0,sizeof(FL));

	/* root */
	strcpy(FL.root_dir,PREFIX);
	strcatslash(FL.root_dir);
	strcat(FL.root_dir,"share/advance");

	/* home */
	home = getenv("HOME");
	if (!home || !*home) {
		/* use the root dir as home dir */
		strcpy(FL.home_dir,FL.root_dir);
	} else {
		strcpy(FL.home_dir,home);
		strcatslash(FL.home_dir);
		strcat(FL.home_dir,".advance");
	}

	if (FL.home_dir[0]) {
		struct stat st;

		if (stat(FL.home_dir,&st) == 0) {
			if (!S_ISDIR(st.st_mode)) {
				target_err("Failure: A file named %s exists\n", FL.home_dir);
				return -1;
			}
		} else {
			if (mkdir(FL.home_dir,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
				target_err("Failure: Error creating the directory %s\n", FL.home_dir);
				return -1;
			}
		}

#if defined(MAME) || defined(MESS) || defined(PAC)
		{
			char buffer[FILE_MAXPATH];
			sprintf(buffer,"%s/cfg",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/snap",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/hi",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/nvram",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/memcard",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/sta",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/inp",FL.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		}
#endif
	}

	return 0;
}

void file_done(void) {
}

/***************************************************************************/
/* File System */

char file_dir_separator(void) {
	return ':';
}

char file_dir_slash(void) {
	return '/';
}

const char* file_import(const char* path) {
	return path;
}

const char* file_export(const char* path) {
	return path;
}

/***************************************************************************/
/* Files */

const char* file_config_file_root(const char* file) {
	if (file[0] == '/')
		sprintf(FL.file_root_buffer,"%s",file);
	else
		/* if relative add the root data dir */
		sprintf(FL.file_root_buffer,"%s/%s",FL.root_dir,file);
	return FL.file_root_buffer;
}

const char* file_config_file_home(const char* file) {
	if (file[0] == '/')
		sprintf(FL.file_home_buffer,"%s",file);
	else
		/* if relative add the home data dir */
		sprintf(FL.file_home_buffer,"%s/%s",FL.home_dir,file);
	return FL.file_home_buffer;
}

const char* file_config_file_legacy(const char* file) {
	return 0;
}

const char* file_config_dir_multidir(const char* tag) {
	assert( tag[0] != '/' );
	sprintf(FL.dir_buffer,"%s/%s:%s/%s",FL.home_dir,tag,FL.root_dir,tag);
	return FL.dir_buffer;
}

const char* file_config_dir_singledir(const char* tag) {
	assert( tag[0] != '/' );
	sprintf(FL.dir_buffer,"%s/%s",FL.home_dir,tag);
	return FL.dir_buffer;
}

const char* file_config_dir_singlefile(void) {
	sprintf(FL.dir_buffer,"%s:%s",FL.home_dir,FL.root_dir);
	return FL.dir_buffer;
}

