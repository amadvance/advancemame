/*
 * This file is part of the AdvanceMAME project.
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

#ifndef __FILE_H
#define __FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Max path length. */
#define FILE_MAXPATH 512

/***************************************************************************/
/* Init */

int file_init(void);
void file_done(void);

/***************************************************************************/
/* FileSystem */

/**
 * Return the char used as a dir separator.
 * Example ':' in UNIX and ';' in MSDOS.
 */
char file_dir_separator(void);

/**
 * Return the char used as a dir slashr.
 * Example '/' in UNIX and '\' in MSDOS.
 */
char file_dir_slash(void);

/**
 * Convert a path from the OS format in a standard UNIX format.
 * The returned buffer may be the same argument or a static buffer.
 * If a static buffer is used, you need at least two static buffer to use
 * alternatively.
 */
const char* file_import(const char* path);

/*
 * Convert a path to the OS format from a standard UNIX format.
 * The returned buffer may be the same argument or a static buffer.
 * If a static buffer is used, you need at least two static buffer to use
 * alternatively.
 */
const char* file_export(const char* path);

/***************************************************************************/
/* Files */

/**
 * Complete path of a file in the root data directory.
 * If the path is relative, the root directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path or 0 if the root dir is not supported.
 */
const char* file_config_file_root(const char* file);

/**
 * Complete path of a file in the home data directory.
 * If the path is relative, the home directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path.
 */
const char* file_config_file_home(const char* file);

/**
 * Complete path of a file in the legacy data directory.
 * If the path is relative, the legacye directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path or 0 if the legacy dir is not supported.
 */
const char* file_config_file_legacy(const char* file);

/**
 * Directory list where to search a subdirectory or a file.
 * \note The returned value is in the OS depended format.
 * \return The directory list with the tag added. Generally first the HOME DATA directory and as second choice the ROOT DATA directory.
 */
const char* file_config_dir_multidir(const char* tag);

/**
 * Single directory where to search a subdirectory or a file.
 * \note The returned value is in the OS depended format.
 * \return The directory with the tag added. Generally the HOME DATA directory.
 */
const char* file_config_dir_singledir(const char* tag);

/**
 * Directory list where search a single support file.
 * \note The returned value is in the OS depended format.
 * \return The directory list. Generally first the HOME DATA directory and as second choice the ROOT DATA directory.
 */
const char* file_config_dir_singlefile(void);

#ifdef __cplusplus
}
#endif

#endif
