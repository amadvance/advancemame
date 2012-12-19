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
 * \param bufoff Buffer offset.
 * \param offset Where to set the found offset.
 * \return
 *  - ==0 not found
 *  - !=0 found, *offset valid
 */
static int ecd_find_sig(char* buffer, int buflen, off_t bufoff, int* ecd_offset, int* ecd64_offset)
{
	int i;
	for (i=buflen-22; i>=0; i--) {
		if (buffer[i] == 'P' && buffer[i+1] == 'K' && buffer[i+2] == 0x5 && buffer[i+3] == 6) {
			uint32 offset_to_start_of_cent_dir;
			uint64 offset_to_64_end_of_cent_dir;
			char* p = buffer + i;

			/* get the offset of central directory */
			offset_to_start_of_cent_dir = le_uint32_read(p + ZIP_EO_offset_to_start_of_cent_dir);

			/* check if it's a 32 bit zip file */
			if (offset_to_start_of_cent_dir != 0xFFFFFFFF) {
				*ecd_offset = i;
				*ecd64_offset = i;
				return 1;
			}

			/* check if there is enough space for the zip64 central directory locator */
			if (i < 20) {
				/* we need more bytes */
				return 0;
			}

			p -= 20;
			
			/* check for the zip64 end central directory locator marker */
			if (p[0] != 'P' || p[1] != 'K' || p[2] != 0x6 || p[3] != 0x7) {
				log_std(("zip: Invalid ZIP64 format, missing end central directory locator marker\n"));
				return -1;
			}

			/* get the offset of the zip64 end of central directory */
			offset_to_64_end_of_cent_dir = le_uint64_read(p + 8);

			/* check if the zip64 end of central directory in fully in buffer */
			if (offset_to_64_end_of_cent_dir < bufoff)
				return 0;

			p = buffer + (offset_to_64_end_of_cent_dir - bufoff);

			/* check for zip64 end of central directory marker */
			if (p[0] != 'P' || p[1] != 'K' || p[2] != 0x6 || p[3] != 0x6) {
				log_std(("zip: Invalid ZIP64 format, missing end central directory marker\n"));
				return -1;
			}

			*ecd_offset = i;
			*ecd64_offset = offset_to_64_end_of_cent_dir - bufoff;
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

static int ecd_read(adv_zip* zip)
{
	char* buf;
	int buf_pos = zip->length - ECD_READ_BUFFER_SIZE;
	int buf_pos_aligned = buf_pos & ~(ECD_READ_BUFFER_SIZE-1);
	int buf_length = zip->length - buf_pos_aligned; /* initial buffer length */
	unsigned increment = ECD_READ_BUFFER_SIZE;

	while (1) {
		int offset32;
		int offset64;
		int r;

		if (buf_length > zip->length)
			buf_length = zip->length;

		if (fseeko(zip->fp, zip->length - buf_length, SEEK_SET) != 0) {
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

		r = ecd_find_sig(buf, buf_length, zip->length - buf_length, &offset32, &offset64);
		if (r < 0)
			return -1;
		if (r) {
			zip->ecd_length = buf_length - offset32;

			zip->ecd = malloc(zip->ecd_length);
			if (!zip->ecd) {
				free(buf);
				return -1;
			}

			memcpy(zip->ecd, buf + offset32, zip->ecd_length);

			if (offset64 < offset32) {
				zip->ecd64_length = offset32 - offset64;

				zip->ecd64 = malloc(zip->ecd64_length);
				if (!zip->ecd64) {
					free(buf);
					return -1;
				}

				memcpy(zip->ecd64, buf + offset64, zip->ecd64_length);
			} else {
				zip->ecd64_length = 0;
				zip->ecd64 = 0;
			}
			

			free(buf);
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
	if (fseeko(zip->fp, 0L, SEEK_END) != 0) {
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* get length */
	zip->length = ftello(zip->fp);
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
	if (ecd_read(zip) != 0) {
		log_std(("zip: Invalid ZIP format, missing end central directory in file %s\n", zipfile));
		fclose(zip->fp);
		free(zip);
		return 0;
	}

	/* compile ecd info */
	zip->number_of_this_disk = le_uint16_read(zip->ecd+ZIP_EO_number_of_this_disk);
	if (zip->number_of_this_disk == 0xFFFF) {
		zip->number_of_this_disk = le_uint32_read(zip->ecd64 + 16);
	}
	zip->number_of_disk_start_cent_dir = le_uint16_read(zip->ecd+ZIP_EO_number_of_disk_start_cent_dir);
	if (zip->number_of_disk_start_cent_dir == 0xFFFF) {
		zip->number_of_disk_start_cent_dir = le_uint32_read(zip->ecd64 + 20);
	}
	zip->total_entries_cent_dir_this_disk = le_uint16_read(zip->ecd+ZIP_EO_total_entries_cent_dir_this_disk);
	zip->total_entries_cent_dir = le_uint16_read(zip->ecd+ZIP_EO_total_entries_cent_dir);
	zip->size_of_cent_dir = le_uint32_read(zip->ecd+ZIP_EO_size_of_cent_dir);
	zip->offset_to_start_of_cent_dir = le_uint32_read(zip->ecd+ZIP_EO_offset_to_start_of_cent_dir);
	if (zip->offset_to_start_of_cent_dir == 0xFFFFFFFF) {
		if (!zip->ecd64) {
			log_std(("zip: Invalid ZIP64 format, missing end of central directory in file %s\n", zipfile));
			goto bail;
		}
		zip->offset_to_start_of_cent_dir = le_uint64_read(zip->ecd64 + 48);
	}
	zip->zipfile_comment_length = le_uint16_read(zip->ecd+ZIP_EO_zipfile_comment_length);
	zip->zipfile_comment = zip->ecd+ZIP_EO_zipfile_comment;

	/* verify that we can work with this zipfile */
	if (zip->number_of_this_disk != zip->number_of_disk_start_cent_dir /* no spanning allowed */
		|| zip->total_entries_cent_dir_this_disk != zip->total_entries_cent_dir
		|| zip->total_entries_cent_dir < 1
		|| zip->size_of_cent_dir == 0xFFFFFFFF /* 64 bit not supported in cent dir size */
	) {
		log_std(("zip: Unsupported ZIP format in file %s\n", zipfile));
		goto bail;
	}

	/* allocate space for cent_dir */
	zip->cd = (char*)malloc(zip->size_of_cent_dir);
	if (!zip->cd) {
		goto bail;
	}

	/* seek to start of cent_dir */
	if (fseeko(zip->fp, zip->offset_to_start_of_cent_dir, SEEK_SET)!=0) {
		goto bail;
	}

	/* read from start of central directory */
	if (fread(zip->cd, zip->size_of_cent_dir, 1, zip->fp)!=1) {
		goto bail;
	}

	/* reset ent */
	zip->ent.name = 0;

	/* rewind */
	zip->cd_pos = 0;

	/* file name */
	zip->zip = strdup(zipfile);
	if (!zip->zip) {
		goto bail;
	}

	return zip;
bail:
	free(zip->ecd);
	free(zip->ecd64);
	fclose(zip->fp);
	free(zip);
	return 0;
}

adv_zipent* zip_read(adv_zip* zip)
{
	unsigned i;
	char* cd = zip->cd + zip->cd_pos;

	/* end of directory */
	if (zip->cd_pos >= zip->size_of_cent_dir)
		return 0;

	/* check for marker */
	if (cd[0] != 'P' || cd[1] != 'K' || cd[2] != 0x1 || cd[3] != 0x2) {
		log_std(("zip: Invalid ZIP format, missing file header marker\n"));
		return 0;
	}

	/* compile zipent info */
	zip->ent.version_made_by = *(cd+ZIP_CO_version_made_by);
	zip->ent.host_os = *(cd+ZIP_CO_host_os);
	zip->ent.version_needed_to_extract = *(cd+ZIP_CO_version_needed_to_extract);
	zip->ent.os_needed_to_extract = *(cd+ZIP_CO_os_needed_to_extract);
	zip->ent.general_purpose_bit_flag = le_uint16_read(cd+ZIP_CO_general_purpose_bit_flag);
	zip->ent.compression_method = le_uint16_read(cd+ZIP_CO_compression_method);
	zip->ent.last_mod_file_time = le_uint16_read(cd+ZIP_CO_last_mod_file_time);
	zip->ent.last_mod_file_date = le_uint16_read(cd+ZIP_CO_last_mod_file_date);
	zip->ent.crc32 = le_uint32_read(cd+ZIP_CO_crc32);
	zip->ent.compressed_size = le_uint32_read(cd+ZIP_CO_compressed_size);
	zip->ent.uncompressed_size = le_uint32_read(cd+ZIP_CO_uncompressed_size);
	zip->ent.filename_length = le_uint16_read(cd+ZIP_CO_filename_length);
	zip->ent.extra_field_length = le_uint16_read(cd+ZIP_CO_extra_field_length);
	zip->ent.file_comment_length = le_uint16_read(cd+ZIP_CO_file_comment_length);
	zip->ent.disk_number_start = le_uint16_read(cd+ZIP_CO_disk_number_start);
	zip->ent.internal_file_attrib = le_uint16_read(cd+ZIP_CO_internal_file_attributes);
	zip->ent.external_file_attrib = le_uint32_read(cd+ZIP_CO_external_file_attributes);
	zip->ent.offset_lcl_hdr_frm_frst_disk = le_uint32_read(cd+ZIP_CO_relative_offset_of_local_header);

	/* check to see if filename length is illegally long (past the size of this directory entry) */
	if (zip->cd_pos + ZIP_LO_FIXED + zip->ent.filename_length + zip->ent.extra_field_length + zip->ent.file_comment_length > zip->size_of_cent_dir) {
		log_std(("zip: Invalid ZIP format, central directory overflow\n"));
		return 0;
	}

	cd += ZIP_CO_FIXED;

	/* filename */
	free(zip->ent.name);
	zip->ent.name = (char*)malloc(zip->ent.filename_length + 1);
	memcpy(zip->ent.name, cd, zip->ent.filename_length);
	zip->ent.name[zip->ent.filename_length] = 0;

	/* convert to lower case, the DOS adv_zip are case insensitive */
	for(i=0;zip->ent.name[i];++i)
		zip->ent.name[i] = tolower(zip->ent.name[i]);

	cd += zip->ent.filename_length;

	/* extra fields */
	i = 0;
	while (i < zip->ent.extra_field_length) {
		unsigned id = le_uint16_read(cd);
		unsigned size = le_uint16_read(cd + 2);

		cd += 4;
		i += 4;

		if (id == 0x0001) {
			if (zip->ent.compressed_size == 0xFFFFFFFF) {
				zip->ent.compressed_size = le_uint64_read(cd);
				cd += 8;
				i += 8;
				size -= 8;
			}
			if (zip->ent.uncompressed_size == 0xFFFFFFFF) {
				zip->ent.uncompressed_size = le_uint64_read(cd);
				cd += 8;
				i += 8;
				size -= 8;
			}
			if (zip->ent.offset_lcl_hdr_frm_frst_disk == 0xFFFFFFFF) {
				zip->ent.offset_lcl_hdr_frm_frst_disk = le_uint64_read(cd);
				cd += 8;
				i += 8;
				size -= 8;
			}
			if (zip->ent.disk_number_start == 0xFFFF) {
				zip->ent.disk_number_start = le_uint32_read(cd);
				cd += 4;
				i += 4;
				size -= 4;
			}
			if (size != 0) {
				log_std(("zip: Invalid ZIP64 format, malformed extra field\n"));
				return 0;
			}
		} else {
			cd += size;
			i += size;
		}
	}

	if (zip->ent.compressed_size == 0xFFFFFFFF
		|| zip->ent.uncompressed_size == 0xFFFFFFFF
		|| zip->ent.offset_lcl_hdr_frm_frst_disk == 0xFFFFFFFF
		|| zip->ent.disk_number_start == 0xFFFF
	) {
		log_std(("zip: Invalid ZIP64 format, missing extra field\n"));
		return 0;
	}

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
	free(zip->ecd64);
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
	off_t offset;

	if (fseeko(zip->fp, ent->offset_lcl_hdr_frm_frst_disk, SEEK_SET)!=0) {
		return -1;
	}

	if (fread(buf, ZIP_LO_FIXED, 1, zip->fp)!=1) {
		return -1;
	}

	{
		unsigned filename_length = le_uint16_read(buf+ZIP_LO_filename_length);
		unsigned extra_field_length = le_uint16_read(buf+ZIP_LO_extra_field_length);

		/* calculate offset to data and seek there */
		offset = ent->offset_lcl_hdr_frm_frst_disk + ZIP_LO_FIXED + filename_length + extra_field_length;

		if (fseeko(zip->fp, offset, SEEK_SET) != 0) {
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

