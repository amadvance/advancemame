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

#include "portable.h"

#include "unzip.h"
#include "endianrw.h"
#include "log.h"

#include <zlib.h>

/**
 * Locate end-of-central-dir sig in buffer and return offset.
 * \param buffer Buffer in which search.
 * \param buflen Buffer length.
 * \param offset Where to set the found offset.
 * \return
 *  - ==0 not found
 *  - !=0 found, *offset valid
 */
static int ecd_find_sig(char* buffer, int buflen, int *offset)
{
	static char ecdsig[] = { 'P', 'K', 0x05, 0x06 };
	int i;
	for (i=buflen-22; i>=0; i--) {
		if (memcmp(buffer+i, ecdsig, 4) == 0) {
			*offset = i;
			return 1;
		}
	}
	return 0;
}

/*
   Read ecd data in zip structure.
   in:
     zip->fp, zip->length zip file
   out:
     zip->ecd, zip->ecd_length ecd data
     data, size end of zip file read, MUST be deallocated
*/
#define ECD_READ_BUFFER_SIZE 1024

static int ecd_read(adv_zip* zip, char** data, unsigned* size)
{
	char* buf;
	int buf_pos = zip->length - ECD_READ_BUFFER_SIZE;
	int buf_pos_aligned = buf_pos & ~(ECD_READ_BUFFER_SIZE-1);
	int buf_length = zip->length - buf_pos_aligned; /* initial buffer length */
	unsigned increment = ECD_READ_BUFFER_SIZE;

	while (1) {
		int offset;

		if (buf_length > zip->length)
			buf_length = zip->length;

		if (fseek(zip->fp, zip->length - buf_length, SEEK_SET) != 0) {
			return -1;
		}

		/* allocate buffer */
		buf = (char*)malloc(buf_length);
		if (!buf) {
			return -1;
		}

		if (fread(buf, buf_length, 1, zip->fp) != 1) {
			free(buf);
			return -1;
		}

		if (ecd_find_sig(buf, buf_length, &offset)) {
			zip->ecd_length = buf_length - offset;

			zip->ecd = (char*)malloc(zip->ecd_length);
			if (!zip->ecd) {
				free(buf);
				return -1;
			}

			memcpy(zip->ecd, buf + offset, zip->ecd_length);

			/* save buffer */
			*data = buf;
			*size = buf_length;

			return 0;
		}

		free(buf);

		if (buf_length+increment < 4*1024*1024) { /* no more than 4M */
			/* grow buffer */
			buf_length += increment;
			increment *= 2; /* power increment */
		} else {
			return -1;
		}
	}
}

adv_zip* zip_open(const char* zipfile)
{
	char* ezdata = 0;
	unsigned ezsize = 0;

	/* allocate */
	adv_zip* zip = (adv_zip*)malloc(sizeof(adv_zip));
	if (!zip) {
		return 0;
	}

	/* open */
	zip->fp = fopen(zipfile, "rb");
	if (!zip->fp) {
		free(zip);
		return 0;
	}

	/* go to end */
	if (fseek(zip->fp, 0L, SEEK_END) != 0) {
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* get length */
	zip->length = ftell(zip->fp);
	if (zip->length < 0) {
		fclose(zip->fp);
		free(zip);
		return 0;
	}
	if (zip->length == 0) {
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* read ecd data */
	if (ecd_read(zip, &ezdata, &ezsize)!=0) {
		log_std(("zip: ECD not found in file %s\n", zipfile));
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* compile ecd info */
	zip->end_of_cent_dir_sig = le_uint32_read(zip->ecd+ZIP_EO_end_of_central_dir_signature);
	zip->number_of_this_disk = le_uint16_read(zip->ecd+ZIP_EO_number_of_this_disk);
	zip->number_of_disk_start_cent_dir = le_uint16_read(zip->ecd+ZIP_EO_number_of_disk_start_cent_dir);
	zip->total_entries_cent_dir_this_disk = le_uint16_read(zip->ecd+ZIP_EO_total_entries_cent_dir_this_disk);
	zip->total_entries_cent_dir = le_uint16_read(zip->ecd+ZIP_EO_total_entries_cent_dir);
	zip->size_of_cent_dir = le_uint32_read(zip->ecd+ZIP_EO_size_of_cent_dir);
	zip->offset_to_start_of_cent_dir = le_uint32_read(zip->ecd+ZIP_EO_offset_to_start_of_cent_dir);
	zip->zipfile_comment_length = le_uint16_read(zip->ecd+ZIP_EO_zipfile_comment_length);
	zip->zipfile_comment = zip->ecd+ZIP_EO_zipfile_comment;

	/* verify that we can work with this zipfile (no disk spanning allowed) */
	if ((zip->number_of_this_disk != zip->number_of_disk_start_cent_dir) ||
		(zip->total_entries_cent_dir_this_disk != zip->total_entries_cent_dir) ||
		(zip->total_entries_cent_dir < 1)) {
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		free(ezdata);
		return 0;
	}

	/* allocate space for cent_dir */
	zip->cd = (char*)malloc(zip->size_of_cent_dir);
	if (!zip->cd) {
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		free(ezdata);
		return 0;
	}

	/* if buffer read is enougth for cent_dir */
	if (zip->offset_to_start_of_cent_dir >= zip->length - ezsize) {
		/* copy cent_dir from buffer */
		memcpy(zip->cd, ezdata + zip->offset_to_start_of_cent_dir + ezsize - zip->length, zip->size_of_cent_dir);

		free(ezdata);
	} else {
		/* buffer not used */
		free(ezdata);

		/* seek to start of cent_dir */
		if (fseek(zip->fp, zip->offset_to_start_of_cent_dir, SEEK_SET)!=0) {
			free(zip->ecd);
			fclose(zip->fp);
			free(zip);
			return 0;
		}

		/* read from start of central directory */
		if (fread(zip->cd, zip->size_of_cent_dir, 1, zip->fp)!=1) {
			free(zip->cd);
			free(zip->ecd);
			fclose(zip->fp);
			free(zip);
			return 0;
		}
	}

	/* reset ent */
	zip->ent.name = 0;

	/* rewind */
	zip->cd_pos = 0;

	/* file name */
	zip->zip = strdup(zipfile);
	if (!zip->zip) {
		free(zip->cd);
		free(zip->ecd);
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	return zip;
}

adv_zipent* zip_read(adv_zip* zip)
{
	unsigned i;

	/* end of directory */
	if (zip->cd_pos >= zip->size_of_cent_dir)
		return 0;

	/* compile zipent info */
	zip->ent.cent_file_header_sig = le_uint32_read(zip->cd+zip->cd_pos+ZIP_CO_central_file_header_signature);
	zip->ent.version_made_by = *(zip->cd+zip->cd_pos+ZIP_CO_version_made_by);
	zip->ent.host_os = *(zip->cd+zip->cd_pos+ZIP_CO_host_os);
	zip->ent.version_needed_to_extract = *(zip->cd+zip->cd_pos+ZIP_CO_version_needed_to_extract);
	zip->ent.os_needed_to_extract = *(zip->cd+zip->cd_pos+ZIP_CO_os_needed_to_extract);
	zip->ent.general_purpose_bit_flag = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_general_purpose_bit_flag);
	zip->ent.compression_method = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_compression_method);
	zip->ent.last_mod_file_time = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_last_mod_file_time);
	zip->ent.last_mod_file_date = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_last_mod_file_date);
	zip->ent.crc32 = le_uint32_read(zip->cd+zip->cd_pos+ZIP_CO_crc32);
	zip->ent.compressed_size = le_uint32_read(zip->cd+zip->cd_pos+ZIP_CO_compressed_size);
	zip->ent.uncompressed_size = le_uint32_read(zip->cd+zip->cd_pos+ZIP_CO_uncompressed_size);
	zip->ent.filename_length = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_filename_length);
	zip->ent.extra_field_length = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_extra_field_length);
	zip->ent.file_comment_length = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_file_comment_length);
	zip->ent.disk_number_start = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_disk_number_start);
	zip->ent.internal_file_attrib = le_uint16_read(zip->cd+zip->cd_pos+ZIP_CO_internal_file_attributes);
	zip->ent.external_file_attrib = le_uint32_read(zip->cd+zip->cd_pos+ZIP_CO_external_file_attributes);
	zip->ent.offset_lcl_hdr_frm_frst_disk = le_uint32_read(zip->cd+zip->cd_pos+ZIP_CO_relative_offset_of_local_header);

	/* check to see if filename length is illegally long (past the size of this directory entry) */
	if (zip->cd_pos + ZIP_LO_FIXED + zip->ent.filename_length > zip->size_of_cent_dir)
		return 0;

	/* copy filename */
	free(zip->ent.name);
	zip->ent.name = (char*)malloc(zip->ent.filename_length + 1);
	memcpy(zip->ent.name, zip->cd+zip->cd_pos+ZIP_CO_filename, zip->ent.filename_length);
	zip->ent.name[zip->ent.filename_length] = 0;

	/* convert to lower case, the DOS adv_zip are case insensitive */
	for(i=0;zip->ent.name[i];++i)
		zip->ent.name[i] = tolower(zip->ent.name[i]);

	/* skip to next entry in central dir */
	zip->cd_pos += ZIP_CO_FIXED + zip->ent.filename_length + zip->ent.extra_field_length + zip->ent.file_comment_length;

	return &zip->ent;
}

void zip_close(adv_zip* zip)
{
	/* release all */
	free(zip->ent.name);
	free(zip->cd);
	free(zip->ecd);
	fclose(zip->fp);
	free(zip->zip);
	free(zip);
}

void zip_rewind(adv_zip* zip)
{
	zip->cd_pos = 0;
}

/* Seek zip->fp to compressed data
   return:
	==0 success
	<0 error
*/
static int zip_seekcompress(adv_zip* zip, adv_zipent* ent)
{
	char buf[ZIP_LO_FIXED];
	long offset;

	if (fseek(zip->fp, ent->offset_lcl_hdr_frm_frst_disk, SEEK_SET)!=0) {
		return -1;
	}

	if (fread(buf, ZIP_LO_FIXED, 1, zip->fp)!=1) {
		return -1;
	}

	{
		uint16 filename_length = le_uint16_read(buf+ZIP_LO_filename_length);
		uint16 extra_field_length = le_uint16_read(buf+ZIP_LO_extra_field_length);

		/* calculate offset to data and seek there */
		offset = ent->offset_lcl_hdr_frm_frst_disk + ZIP_LO_FIXED + filename_length + extra_field_length;

		if (fseek(zip->fp, offset, SEEK_SET) != 0) {
			return -1;
		}
	}

	return 0;
}

int zip_readcompress(adv_zip* zip, adv_zipent* ent, char* data)
{
	int err = zip_seekcompress(zip, ent);
	if (err!=0)
		return err;

	if (fread(data, ent->compressed_size, 1, zip->fp)!=1) {
		return -1;
	}

	return 0;
}

