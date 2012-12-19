/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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
 * ZIP file support.
 */

#ifndef __UNZIP_H
#define __UNZIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extra.h"

#include <stdio.h>

/**************************************************************************/
/* Generic flags */

/* If set, indicates that the file is encrypted. */
#define ZIP_GEN_FLAGS_ENCRYPTED		0x01
/* Compression options for deflate */
#define ZIP_GEN_FLAGS_DEFLATE_NORMAL	0x00
#define ZIP_GEN_FLAGS_DEFLATE_MAXIMUM	0x02
#define ZIP_GEN_FLAGS_DEFLATE_FAST	0x04
#define ZIP_GEN_FLAGS_DEFLATE_SUPERFAST	0x06
#define ZIP_GEN_FLAGS_DEFLATE_MASK	0x06
/* If this bit is set, the fields crc-32, compressed size
and uncompressed size are set to zero in the local
header.  The correct values are put in the data descriptor
immediately following the compressed data and in the central directory. */
#define ZIP_GEN_FLAGS_DEFLATE_ZERO	0x08

/* Compression method */
#define ZIP_METHOD_STORED		0x00
#define ZIP_METHOD_DEFLATE		0x08

/* Internal file attributes */
/* If set, that the file is apparently an ASCII or text file. */
#define ZIP_INT_ATTR_TEXT		0x01

/* External attribute */
/* The mapping of the external attributes is host-system dependent */

/**************************************************************************/
/* Offsets in end of central directory structure */

#define ZIP_EO_end_of_central_dir_signature	0x00
#define ZIP_EO_number_of_this_disk		0x04
#define ZIP_EO_number_of_disk_start_cent_dir	0x06
#define ZIP_EO_total_entries_cent_dir_this_disk	0x08
#define ZIP_EO_total_entries_cent_dir		0x0A
#define ZIP_EO_size_of_cent_dir			0x0C
#define ZIP_EO_offset_to_start_of_cent_dir	0x10
#define ZIP_EO_zipfile_comment_length		0x14
#define ZIP_EO_FIXED				0x16 /* size of fixed data structure */
#define ZIP_EO_zipfile_comment			0x16

/**************************************************************************/
/* Offsets in central directory entry structure */

#define ZIP_CO_central_file_header_signature	0x00
#define ZIP_CO_version_made_by			0x04
#define ZIP_CO_host_os				0x05
#define ZIP_CO_version_needed_to_extract	0x06
#define ZIP_CO_os_needed_to_extract		0x07
#define ZIP_CO_general_purpose_bit_flag		0x08
#define ZIP_CO_compression_method		0x0A
#define ZIP_CO_last_mod_file_time		0x0C
#define ZIP_CO_last_mod_file_date		0x0E
#define ZIP_CO_crc32				0x10
#define ZIP_CO_compressed_size			0x14
#define ZIP_CO_uncompressed_size		0x18
#define ZIP_CO_filename_length			0x1C
#define ZIP_CO_extra_field_length		0x1E
#define ZIP_CO_file_comment_length		0x20
#define ZIP_CO_disk_number_start		0x22
#define ZIP_CO_internal_file_attributes		0x24
#define ZIP_CO_external_file_attributes		0x26
#define ZIP_CO_relative_offset_of_local_header	0x2A
#define ZIP_CO_FIXED				0x2E /* size of fixed data structure */

/**************************************************************************/
/* Offsets in data descriptor structure */

#define ZIP_DO_crc32				0x00
#define ZIP_DO_compressed_size			0x04
#define ZIP_DO_uncompressed_size		0x08
#define ZIP_DO_FIXED				0x0C /* size of fixed data structure */

/* Offsets in local file header structure */
#define ZIP_LO_local_file_header_signature	0x00
#define ZIP_LO_version_needed_to_extract	0x04
#define ZIP_LO_general_purpose_bit_flag		0x06
#define ZIP_LO_compression_method		0x08
#define ZIP_LO_last_mod_file_time		0x0A
#define ZIP_LO_last_mod_file_date		0x0C
#define ZIP_LO_crc				0x0E
#define ZIP_LO_compressed_size			0x12
#define ZIP_LO_uncompressed_size		0x16
#define ZIP_LO_filename_length			0x1A
#define ZIP_LO_extra_field_length		0x1C
#define ZIP_LO_FIXED				0x1E /* size of fixed data structure */

/**
 * ZIP entry context.
 */
typedef struct adv_zipent_struct {
	unsigned version_made_by;
	unsigned host_os;
	unsigned version_needed_to_extract;
	unsigned os_needed_to_extract;
	unsigned general_purpose_bit_flag;
	unsigned compression_method;
	unsigned last_mod_file_time;
	unsigned last_mod_file_date;
	unsigned crc32;
	unsigned compressed_size;
	unsigned uncompressed_size;
	unsigned filename_length;
	unsigned extra_field_length;
	unsigned file_comment_length;
	unsigned disk_number_start;
	unsigned internal_file_attrib;
	unsigned external_file_attrib;
	off_t offset_lcl_hdr_frm_frst_disk;
	char* name; /**< Name of the entry. 0 terminated. */
} adv_zipent;

/**
 * ZIP file context.
 */
typedef struct adv_zip_struct {
	char* zip; /**< Zip name. */
	FILE* fp; /**< Zip handler. */
	off_t length; /**< Length of zip file. */

	char* ecd; /**< end_of_cent_dir data. */
	unsigned ecd_length; /**< end_of_cent_dir length. */

	char* ecd64; /**< zip64_end_of_cent_dir data. */
	unsigned ecd64_length; /**< zip64_end_of_cent_dir length. */

	char* cd; /**< cent_dir data. */
	unsigned cd_pos; /** Current position in cent_dir. */

	adv_zipent ent; /**< Buffer for readzip. */

	/* end_of_cent_dir */
	unsigned number_of_this_disk;
	unsigned number_of_disk_start_cent_dir;
	unsigned total_entries_cent_dir_this_disk;
	unsigned total_entries_cent_dir;
	unsigned size_of_cent_dir;
	off_t offset_to_start_of_cent_dir;
	unsigned zipfile_comment_length;

	char* zipfile_comment; /**< Comment as pointer in the ecd. */
} adv_zip;

/** \addtogroup ZIPFile */
/*@{*/

/**
 * Opens a zip stream for reading.
 * \return
 *  - !=0 success, zip stream
 *  - ==0 error
 */
adv_zip* zip_open(const char* path);

/**
 * Closes a zip stream.
 */
void zip_close(adv_zip* zip);

/**
 * Reads the current entry from a zip stream.
 * \param zip Opened zip
 * \return
 *  - !=0 success
 *  - ==0 error
 */
adv_zipent* zip_read(adv_zip* zip);

/**
 * Resets a zip stream to the first entry.
 * \note The adv_zip file must be opened and not suspended.
 * \param zip opened zip
 */
void zip_rewind(adv_zip* zip);

/**
 * Read compressed data from a zip entry.
 * \param zip Opened zip.
 * \param ent Entry to read.
 * \param data Buffer for data, ent.compressed_size bytes allocated by the caller.
 * \return
 *  - ==0 success
 *  - <0 error
 */
int zip_read_compress(adv_zip* zip, adv_zipent* ent, char* data);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


