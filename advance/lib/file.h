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

/** \file
 * Filesystem.
 */

/** \addtogroup File */
/*@{*/

#ifndef __FILE_H
#define __FILE_H

#include "extra.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Max path length. */
#define FILE_MAXPATH 512

/***************************************************************************/
/* Init */

/**
 * Initialize the file system.
 * It's called in the main() function.
 */
adv_error file_init(void);

/**
 * Deinitialize the file system.
 * It's called in the main() function.
 */
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
 * Create a directory.
 * \note The arg is in the OS depended format.
 * \return 0 on success.
 */
adv_error file_dir_make(const char* dir);

/**
 * Check if a directory is an absolute path.
 * \note The arg is in the OS depended format.
 */
adv_bool file_path_is_abs(const char* path);

/**
 * Compute the absolute path.
 * If the file is already an absolute path the directory is ignored.
 * \param dir The base directory.
 * \param file The relative file spec.
 * \return The resulting absolute path.
 * \note The args and the returned value are in the OS depended format.
 */
const char* file_abs(const char* dir, const char* file);

/**
 * Convert a path from the OS format in a standard UNIX format.
 * The returned buffer may be the same argument or a static buffer.
 * If a static buffer is used, you need at least two static buffer to use
 * alternatively.
 */
const char* file_import(const char* path);

/**
 * Convert a path to the OS format from a standard UNIX format.
 * The returned buffer may be the same argument or a static buffer.
 * If a static buffer is used, you need at least two static buffer to use
 * alternatively.
 */
const char* file_export(const char* path);

/***************************************************************************/
/* Config */

/**
 * Complete path of a file in the host directory.
 * If the path is relative, the host directory is added. If the
 * path is absolute the path isn't changed.
 * In *nix systems the host dir is generally $prefix/etc, on dos
 * systems it isn't supported.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path or 0 if the host dir is not supported.
 */
const char* file_config_file_host(const char* file);

/**
 * Complete path of a file in the data directory.
 * If the path is relative, the data directory is added. If the
 * path is absolute the path isn't changed.
 * In *nix systems the host dir is generally share/advance on dos
 * systems it isn't supported.
 * \note The arg and the returned value are in the OS depended format.
 * \return The complete path or 0 if the data dir is not supported.
 */
const char* file_config_file_data(const char* file);

/**
 * Complete path of a file in the home data directory.
 * If the path is relative, the home directory is added. If the
 * path is absolute the path isn't changed.
 * \note The arg and the returned value are in the OS format.
 * \return The complete path. It's always != 0.
 */
const char* file_config_file_home(const char* file);

/**
 * Complete a directory list.
 * \param list Subdirectory list. It may contains both relative
 * and absolute directories. Relative directories are expanded
 * calling expand_dir().
 * \param ref_dir Reference directory. Use 0 for the current
 * program. Or use the directory of the executable. Used in DOS
 * where programs store the files in the executable directory.
 * \param expand_dir Function to expand relative directories. Use
 * file_config_dir_multidir() or file_config_dir_singledir().
 * \note The returned value is in the OS format.
 */
const char* file_config_list(const char* list, const char* (*expand_dir)(const char* tag), const char* ref_dir);

/**
 * Return the directory list where to search a file.
 * \param tag Subdirectory name without any slash.
 * \note The returned value is in the OS format.
 * \return The directory list with the tag added. Generally
 * the $home/$tag and the $data/$tag directories are returned.
 */
const char* file_config_dir_multidir(const char* tag);

/**
 * Return a single directory where to search a file.
 * Use this version instead of file_config_dir_multidir() if only a
 * search path is allowed. For example for saving files.
 * \param tag Subdirectory name without any slash.
 * \note The returned value is in the OS format.
 * \return The directory with the tag added. Generally
 * the $home/$tag directory is returned.
 */
const char* file_config_dir_singledir(const char* tag);

/**
 * Return the directory list where to search a file.
 * \note The returned value is in the OS format.
 * \return The directory list. Generally
 * the $home and the $data directories are returned.
 */
const char* file_config_dir_singlefile(void);

#ifdef __cplusplus
}
#endif

#endif

/*@}*/
