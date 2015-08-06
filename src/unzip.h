/***************************************************************************

    unzip.h

    Functions to manipulate data within ZIP files.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __UNZIP_H__
#define __UNZIP_H__

#include "osdepend.h"

/***************************************************************************
 * Support for retrieving files from zipfiles
 ***************************************************************************/

struct _zip_entry
{
	UINT32	cent_file_header_sig;
	UINT8	version_made_by;
	UINT8	host_os;
	UINT8	version_needed_to_extract;
	UINT8	os_needed_to_extract;
	UINT16	general_purpose_bit_flag;
	UINT16	compression_method;
	UINT16	last_mod_file_time;
	UINT16	last_mod_file_date;
	UINT32	crc32;
	UINT32	compressed_size;
	UINT32	uncompressed_size;
	UINT16	filename_length;
	UINT16	extra_field_length;
	UINT16	file_comment_length;
	UINT16	disk_number_start;
	UINT16	internal_file_attrib;
	UINT32	external_file_attrib;
	UINT32	offset_lcl_hdr_frm_frst_disk;
	char*   name; /* 0 terminated */
};
typedef struct _zip_entry zip_entry;

struct _zip_file
{
	char* zip; /* zip name */
	osd_file* fp; /* zip handler */
	int pathtype,pathindex;	/* additional path info */
	long length; /* length of zip file */

	char* ecd; /* end_of_cent_dir data */
	unsigned ecd_length; /* end_of_cent_dir length */

	char* cd; /* cent_dir data */

	unsigned cd_pos; /* position in cent_dir */

	zip_entry ent; /* buffer for readzip */

	/* end_of_cent_dir */
	UINT32	end_of_cent_dir_sig;
	UINT16	number_of_this_disk;
	UINT16	number_of_disk_start_cent_dir;
	UINT16	total_entries_cent_dir_this_disk;
	UINT16	total_entries_cent_dir;
	UINT32	size_of_cent_dir;
	UINT32	offset_to_start_of_cent_dir;
	UINT16	zipfile_comment_length;
	char*	zipfile_comment; /* pointer in ecd */
};
typedef struct _zip_file zip_file;

/* Opens a zip stream for reading
   return:
     !=0 success, zip stream
     ==0 error
*/
zip_file* openzip(int pathtype, int pathindex, const char* path);

/* Closes a zip stream */
void closezip(zip_file* zip);

/* Reads the current entry from a zip stream
   in:
     zip opened zip
   return:
     !=0 success
     ==0 error
*/
zip_entry* readzip(zip_file* zip);

/* Suspend access to a zip file (release file handler)
   in:
      zip opened zip
   note:
     A suspended zip is automatically reopened at first call of
     readuncompressd() or readcompressed() functions
*/
void suspendzip(zip_file* zip);

/* Resets a zip stream to the first entry
   in:
     zip opened zip
   note:
     ZIP file must be opened and not suspended
*/
void rewindzip(zip_file* zip);

/* Read compressed data from a zip entry
   in:
     zip opened zip
     ent entry to read
   out:
     data buffer for data, ent.compressed_size UINT8s allocated by the caller
   return:
     ==0 success
     <0 error
*/
int readcompresszip(zip_file* zip, zip_entry* ent, char* data);

/* Read decompressed data from a zip entry
   in:
     zip zip stream open
     ent entry to read
   out:
     data buffer for data, ent.uncompressed_size UINT8s allocated by the caller
   return:
     ==0 success
     <0 error
*/
int readuncompresszip(zip_file* zip, zip_entry* ent, char* data);

/* public functions */
int /* error */ load_zipped_file (int pathtype, int pathindex, const char *zipfile, const char *filename,
	unsigned char **buf, unsigned int *length);
int /* error */ checksum_zipped_file (int pathtype, int pathindex, const char *zipfile, const char *filename, unsigned int *length, unsigned int *sum);

void unzip_cache_clear(void);

/* public globals */
extern int	gUnzipQuiet;	/* flag controls error messages */

#endif	/* __UNZIP_H__ */
