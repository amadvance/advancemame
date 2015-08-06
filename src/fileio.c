/***************************************************************************

    fileio.c

    File access functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <zlib.h>

#include "osdepend.h"
#include "driver.h"
#include "chd.h"
#include "hash.h"
#include "unzip.h"

#ifdef MESS
#include "image.h"
#endif


/***************************************************************************
    DEBUGGING
***************************************************************************/

/* Verbose outputs to error.log ? */
#define VERBOSE 				0

/* enable lots of logging */
#if VERBOSE
#define VPRINTF(x)				logerror x
#else
#define VPRINTF(x)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PLAIN_FILE				0
#define RAM_FILE				1
#define ZIPPED_FILE				2
#define UNLOADED_ZIPPED_FILE	3

#define FILEFLAG_OPENREAD		0x0001
#define FILEFLAG_OPENWRITE		0x0002
#define FILEFLAG_HASH			0x0100
#define FILEFLAG_REVERSE_SEARCH	0x0200
#define FILEFLAG_VERIFY_ONLY	0x0400
#define FILEFLAG_NOZIP			0x0800
#define FILEFLAG_MUST_EXIST		0x1000
#define FILEFLAG_CREATE_GAMEDIR	0x8000

#ifdef MESS
#define FILEFLAG_GHOST			0x0004
#define FILEFLAG_ALLOW_ABSOLUTE	0x2000
#define FILEFLAG_ZIP_PATHS		0x4000
#endif

#ifdef MAME_DEBUG
#define DEBUG_COOKIE			0xbaadf00d
#endif



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* typedef struct _mame_file mame_file -- declared in fileio.h */
struct _mame_file
{
#ifdef DEBUG_COOKIE
	UINT32		debug_cookie;
#endif
	osd_file *	file;
	UINT8 *		data;
	UINT64		offset;
	UINT64		length;
	UINT8		eof;
	UINT8		type;
	char		hash[HASH_BUF_SIZE];
	int			back_char; /* Buffered char for unget. EOF for empty. */
};



/***************************************************************************
    GLOBALS
***************************************************************************/

#ifdef MESS
int mess_ghost_images;
#endif



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static mame_file *generic_fopen(int pathtype, const char *gamename, const char *filename, const char *hash, UINT32 flags, osd_file_error *error);
static const char *get_extension_for_filetype(int filetype);
static int checksum_file(int pathtype, int pathindex, const char *file, UINT8 **p, UINT64 *size, char* hash);
static chd_interface_file *chd_open_cb(const char *filename, const char *mode);
static void chd_close_cb(chd_interface_file *file);
static UINT32 chd_read_cb(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 chd_write_cb(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 chd_length_cb(chd_interface_file *file);



/***************************************************************************
    HARD DISK INTERFACE
***************************************************************************/

static chd_interface mame_chd_interface =
{
	chd_open_cb,
	chd_close_cb,
	chd_read_cb,
	chd_write_cb,
	chd_length_cb
};


/*-------------------------------------------------
    fileio_init - initialize the internal file
    I/O system; note that the OS layer is free to
    call mame_fopen before fileio_init
-------------------------------------------------*/

void fileio_init(void)
{
	chd_set_interface(&mame_chd_interface);
	add_exit_callback(fileio_exit);
}


/*-------------------------------------------------
    fileio_exit - clean up behind ourselves
-------------------------------------------------*/

void fileio_exit(void)
{
	unzip_cache_clear();
}


/*-------------------------------------------------
    mame_fopen_error - open a file for access and
    return an error code
-------------------------------------------------*/

mame_file *mame_fopen_error(const char *gamename, const char *filename, int filetype, int openforwrite, osd_file_error *error)
{
	/* first verify that we can handle this type of file */
	switch (filetype)
	{
		/* read-only cases */
#ifndef MESS
		case FILETYPE_IMAGE:
		case FILETYPE_INI:
#endif
		case FILETYPE_ROM:
		case FILETYPE_SAMPLE:
		case FILETYPE_HIGHSCORE_DB:
		case FILETYPE_ARTWORK:
		case FILETYPE_HISTORY:
		case FILETYPE_LANGUAGE:
		case FILETYPE_CTRLR:
			if (openforwrite)
			{
				logerror("mame_fopen: type %02x write not supported\n", filetype);
				return NULL;
			}
			break;

		/* write-only cases */
		case FILETYPE_SCREENSHOT:
		case FILETYPE_MOVIE:
		case FILETYPE_DEBUGLOG:
			if (!openforwrite)
			{
				logerror("mame_fopen: type %02x read not supported\n", filetype);
				return NULL;
			}
			break;
	}

	/* now open the file appropriately */
	switch (filetype)
	{
		/* generic files that live in a single directory */
		case FILETYPE_DEBUGLOG:
		case FILETYPE_CTRLR:
		case FILETYPE_LANGUAGE:
		case FILETYPE_HIGHSCORE_DB:
			return generic_fopen(filetype, NULL, filename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD, error);

		/* game-specific files that live in a single directory */
		case FILETYPE_HIGHSCORE:
		case FILETYPE_CONFIG:
		case FILETYPE_INPUTLOG:
		case FILETYPE_COMMENT:
		case FILETYPE_INI:
		case FILETYPE_HASH:		/* MESS-specific */
			return generic_fopen(filetype, NULL, gamename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD, error);

		/* generic multi-directory files */
		case FILETYPE_SAMPLE:
		case FILETYPE_ARTWORK:
			return generic_fopen(filetype, gamename, filename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD, error);

		/* ROM files */
		case FILETYPE_ROM:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD | FILEFLAG_HASH, error);

		/* memory card files */
		case FILETYPE_MEMCARD:
			return generic_fopen(filetype, gamename, filename, 0, openforwrite ? FILEFLAG_OPENWRITE | FILEFLAG_CREATE_GAMEDIR : FILEFLAG_OPENREAD, error);

		/* cheat file */
		case FILETYPE_CHEAT:
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENREAD | (openforwrite ? FILEFLAG_OPENWRITE : 0), error);

		/* disk images */
		case FILETYPE_IMAGE:
#ifndef MESS
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD | FILEFLAG_NOZIP, error);
#else
			{
				int flags = FILEFLAG_ALLOW_ABSOLUTE;
				switch (openforwrite)
				{
					case OSD_FOPEN_READ:
						flags |= FILEFLAG_OPENREAD | FILEFLAG_ZIP_PATHS;
						break;
					case OSD_FOPEN_WRITE:
						flags |= FILEFLAG_OPENWRITE;
						break;
					case OSD_FOPEN_RW:
						flags |= FILEFLAG_OPENREAD | FILEFLAG_OPENWRITE | FILEFLAG_MUST_EXIST;
						break;
					case OSD_FOPEN_RW_CREATE:
						flags |= FILEFLAG_OPENREAD | FILEFLAG_OPENWRITE;
						break;
				}
				if (mess_ghost_images)
					flags |= FILEFLAG_GHOST;

				return generic_fopen(filetype, gamename, filename, 0, flags, error);
			}
#endif

		/* differencing disk images */
		case FILETYPE_IMAGE_DIFF:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD | FILEFLAG_OPENWRITE, error);

		/* NVRAM files */
		case FILETYPE_NVRAM:
#ifdef MESS
			if (filename)
				return generic_fopen(filetype, gamename, filename, 0, openforwrite ? FILEFLAG_OPENWRITE | FILEFLAG_CREATE_GAMEDIR : FILEFLAG_OPENREAD, error);
#endif
			return generic_fopen(filetype, NULL, gamename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD, error);

		/* save state files */
		case FILETYPE_STATE:
#ifndef MESS
			return generic_fopen(filetype, NULL, filename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD, error);
#else
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_ALLOW_ABSOLUTE | (openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD), error);
#endif

		/* screenshot files */
		case FILETYPE_SCREENSHOT:
		case FILETYPE_MOVIE:
#ifndef MESS
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENWRITE, error);
#else
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_ALLOW_ABSOLUTE | FILEFLAG_OPENWRITE, error);
#endif

		/* history files */
		case FILETYPE_HISTORY:
#ifndef MESS
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENREAD, error);
#else
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_ALLOW_ABSOLUTE | FILEFLAG_OPENREAD, error);
#endif

		/* anything else */
		default:
			logerror("mame_fopen(): unknown filetype %02x\n", filetype);
			return NULL;
	}
	return NULL;
}


/*-------------------------------------------------
    mame_fopen - open a file without returning
    any error codes
-------------------------------------------------*/

mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite)
{
	return mame_fopen_error(gamename, filename, filetype, openforwrite, NULL);
}


/*-------------------------------------------------
    mame_fopen_rom - similar to mame_fopen, but
    lets you specify an expected checksum
-------------------------------------------------*/

mame_file *mame_fopen_rom(const char *gamename, const char *filename, const char *exphash)
{
	return generic_fopen(FILETYPE_ROM, gamename, filename, exphash, FILEFLAG_OPENREAD | FILEFLAG_HASH, NULL);
}


/*-------------------------------------------------
    mame_fclose - closes a file
-------------------------------------------------*/

void mame_fclose(mame_file *file)
{
#ifdef DEBUG_COOKIE
	assert(file->debug_cookie == DEBUG_COOKIE);
	file->debug_cookie = 0;
#endif

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			osd_fclose(file->file);
			break;

		case ZIPPED_FILE:
		case RAM_FILE:
			if (file->data)
				free(file->data);
			break;
	}

	/* free the file data */
	free(file);
}


/*-------------------------------------------------
    mame_faccess - determine whether or not the
    given file exists in our search paths
-------------------------------------------------*/

int mame_faccess(const char *filename, int filetype)
{
	const char *extension = get_extension_for_filetype(filetype);
	int pathcount = osd_get_path_count(filetype);
	char modified_filename[256];
	int pathindex;

	/* copy the filename and add an extension */
	strncpy(modified_filename, filename, sizeof(modified_filename) - 1);
	modified_filename[sizeof(modified_filename) - 1] = 0;
	if (extension)
	{
		char *p = strchr(modified_filename, '.');
		if (p)
		{
			strncpy(p, extension, sizeof(modified_filename) - (p - modified_filename) - 1);
			modified_filename[sizeof(modified_filename) - 1] = 0;
		}
		else
		{
			strncat(modified_filename, ".", sizeof(modified_filename) - strlen(modified_filename) - 1);
			strncat(modified_filename, extension, sizeof(modified_filename) - strlen(modified_filename) - 1);
		}
	}

	/* loop over all paths */
	for (pathindex = 0; pathindex < pathcount; pathindex++)
	{
		char name[256];

		/* first check the raw filename, in case we're looking for a directory */
		snprintf(name, sizeof(name), "%s", filename);
		VPRINTF(("mame_faccess: trying %s\n", name));
		if (osd_get_path_info(filetype, pathindex, name) != PATH_NOT_FOUND)
			return 1;

		/* try again with a .zip extension */
		snprintf(name, sizeof(name), "%s.zip", filename);
		VPRINTF(("mame_faccess: trying %s\n", name));
		if (osd_get_path_info(filetype, pathindex, name) != PATH_NOT_FOUND)
			return 1;

		/* does such a directory (or file) exist? */
		snprintf(name, sizeof(name), "%s", modified_filename);
		VPRINTF(("mame_faccess: trying %s\n", name));
		if (osd_get_path_info(filetype, pathindex, name) != PATH_NOT_FOUND)
			return 1;
	}

	/* no match */
	return 0;
}


/*-------------------------------------------------
    mame_fread - read from a file
-------------------------------------------------*/

UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length)
{
	/* flush any buffered char */
	file->back_char = EOF;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_fread(file->file, buffer, length);

		case ZIPPED_FILE:
		case RAM_FILE:
			if (file->data)
			{
				if (file->offset + length > file->length)
				{
					length = file->length - file->offset;
					file->eof = 1;
				}
				memcpy(buffer, file->data + file->offset, length);
				file->offset += length;
				return length;
			}
			break;
	}

	return 0;
}


/*-------------------------------------------------
    mame_fwrite - write to a file
-------------------------------------------------*/

UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length)
{
	/* flush any buffered char */
	file->back_char = EOF;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_fwrite(file->file, buffer, length);
	}

	return 0;
}


/*-------------------------------------------------
    mame_fseek - seek within a file
-------------------------------------------------*/

int mame_fseek(mame_file *file, INT64 offset, int whence)
{
	int err = 0;

	/* flush any buffered char */
	file->back_char = EOF;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_fseek(file->file, offset, whence);

		case ZIPPED_FILE:
		case RAM_FILE:
			switch (whence)
			{
				case SEEK_SET:
					file->offset = offset;
					break;
				case SEEK_CUR:
					file->offset += offset;
					break;
				case SEEK_END:
					file->offset = file->length + offset;
					break;
			}
			file->eof = 0;
			break;
	}

	return err;
}


/*-------------------------------------------------
    mame_fchecksum - verify the existence and
    length of a file given a hash checksum
-------------------------------------------------*/

int mame_fchecksum(const char *gamename, const char *filename, unsigned int *length, char *hash)
{
	mame_file *file;

	/* first open the file; we pass the source hash because it contains
       the expected checksum for the file (used to load by checksum) */
	file = generic_fopen(FILETYPE_ROM, gamename, filename, hash, FILEFLAG_OPENREAD | FILEFLAG_HASH | FILEFLAG_VERIFY_ONLY, NULL);

	/* if we didn't succeed return -1 */
	if (!file)
		return -1;

	/* close the file and save the length & checksum */
	hash_data_copy(hash, file->hash);
	*length = file->length;
	mame_fclose(file);
	return 0;
}


/*-------------------------------------------------
    mame_fsize - returns the size of a file
-------------------------------------------------*/

UINT64 mame_fsize(mame_file *file)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
		{
			int size, offs;
			offs = osd_ftell(file->file);
			osd_fseek(file->file, 0, SEEK_END);
			size = osd_ftell(file->file);
			osd_fseek(file->file, offs, SEEK_SET);
			return size;
		}

		case RAM_FILE:
		case ZIPPED_FILE:
			return file->length;
	}

	return 0;
}


/*-------------------------------------------------
    mame_fhash - returns the hash for a file
-------------------------------------------------*/

const char *mame_fhash(mame_file *file)
{
	return file->hash;
}


/*-------------------------------------------------
    mame_fgetc - read a character from a file
-------------------------------------------------*/

int mame_fgetc(mame_file *file)
{
	unsigned char buffer;

	/* handle ungetc */
	if (file->back_char != EOF)
	{
		buffer = file->back_char;
		file->back_char = EOF;
		return buffer;
	}

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			if (osd_fread(file->file, &buffer, 1) == 1)
				return buffer;
			return EOF;

		case RAM_FILE:
		case ZIPPED_FILE:
			if (file->offset < file->length)
				return file->data[file->offset++];
			else
				file->eof = 1;
			return EOF;
	}
	return EOF;
}


/*-------------------------------------------------
    mame_ungetc - put back a character read from
    a file
-------------------------------------------------*/

int mame_ungetc(int c, mame_file *file)
{
	file->back_char = c;

	return c;
}


/*-------------------------------------------------
    mame_fgets - read a line from a text file
-------------------------------------------------*/

char *mame_fgets(char *s, int n, mame_file *file)
{
	char *cur = s;

	/* loop while we have characters */
	while (n > 0)
	{
		int c = mame_fgetc(file);
		if (c == EOF)
			break;

		/* if there's a CR, look for an LF afterwards */
		if (c == 0x0d)
		{
			int c2 = mame_fgetc(file);
			if (c2 != 0x0a)
				mame_ungetc(c2, file);
			*cur++ = 0x0d;
			n--;
			break;
		}

		/* if there's an LF, reinterp as a CR for consistency */
		else if (c == 0x0a)
		{
			*cur++ = 0x0d;
			n--;
			break;
		}

		/* otherwise, pop the character in and continue */
		*cur++ = c;
		n--;
	}

	/* if we put nothing in, return NULL */
	if (cur == s)
		return NULL;

	/* otherwise, terminate */
	if (n > 0)
		*cur++ = 0;
	return s;
}


/*-------------------------------------------------
    mame_feof - return 1 if we're at the end
    of file
-------------------------------------------------*/

int mame_feof(mame_file *file)
{
	/* check for buffered chars */
	if (file->back_char != EOF)
		return 0;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_feof(file->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			return (file->eof);
	}

	return 1;
}


/*-------------------------------------------------
    mame_ftell - return the current file position
-------------------------------------------------*/

UINT64 mame_ftell(mame_file *file)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_ftell(file->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			return file->offset;
	}

	return -1L;
}


/*-------------------------------------------------
    mame_fread_swap - read from a data file,
    swapping every other byte
-------------------------------------------------*/

UINT32 mame_fread_swap(mame_file *file, void *buffer, UINT32 length)
{
	UINT8 *buf;
	UINT8 temp;
	int res, i;

	/* standard read first */
	res = mame_fread(file, buffer, length);

	/* swap the result */
	buf = buffer;
	for (i = 0; i < res; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}


/*-------------------------------------------------
    mame_fwrite_swap - write to a data file,
    swapping every other byte
-------------------------------------------------*/

UINT32 mame_fwrite_swap(mame_file *file, const void *buffer, UINT32 length)
{
	UINT8 *buf;
	UINT8 temp;
	int res, i;

	/* swap the data first */
	buf = (UINT8 *)buffer;
	for (i = 0; i < length; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	/* do the write */
	res = mame_fwrite(file, buffer, length);

	/* swap the data back */
	for (i = 0; i < length; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}


/*-------------------------------------------------
    mame_fputs - write a line to a text file
-------------------------------------------------*/

#if !defined(CRLF) || (CRLF < 1) || (CRLF > 3)
#error CRLF undefined: must be 1 (CR), 2 (LF) or 3 (CR/LF)
#endif

int mame_fputs(mame_file *f, const char *s)
{
	char convbuf[1024];
	char *pconvbuf;

	for (pconvbuf = convbuf; *s; s++)
	{
		if (*s == '\n')
		{
			if (CRLF == 1)		/* CR only */
				*pconvbuf++ = 13;
			else if (CRLF == 2)	/* LF only */
				*pconvbuf++ = 10;
			else if (CRLF == 3)	/* CR+LF */
			{
				*pconvbuf++ = 13;
				*pconvbuf++ = 10;
			}
		}
		else
			*pconvbuf++ = *s;
	}
	*pconvbuf++ = 0;

	return mame_fwrite(f, convbuf, strlen(convbuf));
}


/*-------------------------------------------------
    mame_vfprintf - vfprintf to a text file
-------------------------------------------------*/

static int mame_vfprintf(mame_file *f, const char *fmt, va_list va)
{
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, va);
	return mame_fputs(f, buf);
}


/*-------------------------------------------------
    mame_fprintf - vfprintf to a text file
-------------------------------------------------*/

int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...)
{
	int rc;
	va_list va;
	va_start(va, fmt);
	rc = mame_vfprintf(f, fmt, va);
	va_end(va);
	return rc;
}


/*-------------------------------------------------
    compose_path - form a pathname from standard
    elements
-------------------------------------------------*/

INLINE void compose_path(char *output, size_t outputlen, const char *gamename, const char *filename, const char *extension)
{
	char *filename_base = output;
	*output = 0;

#ifdef MESS
	if (filename && osd_is_absolute_path(filename))
	{
		strncpy(output, filename, outputlen - 1);
		output[outputlen - 1] = 0;
		return;
	}
#endif

	/* if there's a gamename, add that; only add a '/' if there is a filename as well */
	if (gamename)
	{
		strncat(output, gamename, outputlen - strlen(output) - 1);
		if (filename)
		{
			strncat(output, "/", outputlen - strlen(output) - 1);
			filename_base = &output[strlen(output)];
		}
	}

	/* if there's a filename, add that */
	if (filename)
		strncat(output, filename, outputlen - strlen(output) - 1);

	/* if there's no extension in the filename, add the extension */
	if (extension && !strchr(filename_base, '.'))
	{
		strncat(output, ".", outputlen - strlen(output) - 1);
		strncat(output, extension, outputlen - strlen(output) - 1);
	}
}


/*-------------------------------------------------
    get_extension_for_filetype - return extension
    for a given file type
-------------------------------------------------*/

static const char *get_extension_for_filetype(int filetype)
{
	const char *extension;

	/* now open the file appropriately */
	switch (filetype)
	{
		case FILETYPE_RAW:			/* raw data files */
		case FILETYPE_ROM:			/* ROM files */
		case FILETYPE_HIGHSCORE_DB:	/* highscore database/history files */
		case FILETYPE_HISTORY:		/* game history files */
		case FILETYPE_CHEAT:		/* cheat file */
		default:					/* anything else */
			extension = NULL;
			break;

#ifndef MESS
		case FILETYPE_IMAGE:		/* disk image files */
			extension = "chd";
			break;
#endif

		case FILETYPE_IMAGE_DIFF:	/* differencing drive images */
			extension = "dif";
			break;

		case FILETYPE_SAMPLE:		/* samples */
			extension = "wav";
			break;

		case FILETYPE_ARTWORK:		/* artwork files */
		case FILETYPE_SCREENSHOT:	/* screenshot files */
			extension = "png";
			break;

		case FILETYPE_MOVIE:	    /* recorded movie files */
			extension = "mng";
			break;

		case FILETYPE_NVRAM:		/* NVRAM files */
			extension = "nv";
			break;

		case FILETYPE_HIGHSCORE:	/* high score files */
			extension = "hi";
			break;

		case FILETYPE_LANGUAGE:		/* language files */
			extension = "lng";
			break;

		case FILETYPE_CTRLR:		/* controller files */
		case FILETYPE_CONFIG:		/* config files */
			extension = "cfg";
			break;

		case FILETYPE_INPUTLOG:		/* input logs */
			extension = "inp";
			break;

		case FILETYPE_STATE:		/* save state files */
			extension = "sta";
			break;

		case FILETYPE_MEMCARD:		/* memory card files */
			extension = "mem";
			break;

		case FILETYPE_INI:			/* game specific ini files */
			extension = "ini";
			break;

		case FILETYPE_COMMENT:
			extension = "cmt";
			break;

#ifdef MESS
		case FILETYPE_HASH:
			extension = "hsi";
			break;
#endif
	}
	return extension;
}


/*-------------------------------------------------
    generic_fopen - master logic for finding and
    opening a file
-------------------------------------------------*/

static mame_file *generic_fopen(int pathtype, const char *gamename, const char *filename, const char* hash, UINT32 flags, osd_file_error *error)
{
	static const char *access_modes[] = { "rb", "rb", "wb", "r+b", "rb", "rb", "wbg", "r+bg" };
	const char *extension = get_extension_for_filetype(pathtype);
	int pathcount = osd_get_path_count(pathtype);
	int pathindex, pathstart, pathstop, pathinc;
	mame_file file, *newfile;
	char tempname[256];
	osd_file_error dummy;

#ifdef MESS
	int is_absolute_path = FALSE;
	if (filename)
	{
		is_absolute_path = osd_is_absolute_path(filename);
		if (is_absolute_path)
		{
			if ((flags & FILEFLAG_ALLOW_ABSOLUTE) == 0)
				return NULL;
			pathcount = 1;
		}
	}
#endif /* MESS */

	if (!error)
		error = &dummy;
	*error = FILEERR_SUCCESS;

	VPRINTF(("generic_fopen(%d, %s, %s, %s, %X)\n", pathcount, gamename, filename, extension, flags));

	/* reset the file handle */
	memset(&file, 0, sizeof(file));

	file.back_char = EOF;

	/* check for incompatible flags */
	if ((flags & FILEFLAG_OPENWRITE) && (flags & FILEFLAG_HASH))
		fprintf(stderr, "Can't use HASH option with WRITE option in generic_fopen!\n");

	/* determine start/stop based on reverse search flag */
	if (!(flags & FILEFLAG_REVERSE_SEARCH))
	{
		pathstart = 0;
		pathstop = pathcount;
		pathinc = 1;
	}
	else
	{
		pathstart = pathcount - 1;
		pathstop = -1;
		pathinc = -1;
	}

	/* loop over paths */
	for (pathindex = pathstart; pathindex != pathstop; pathindex += pathinc)
	{
		char name[1024];

		/* ----------------- STEP 1: OPEN THE FILE RAW -------------------- */

		/* first look for path/gamename as a directory */
		compose_path(name, sizeof(name), gamename, NULL, NULL);
		VPRINTF(("Trying %s\n", name));

#ifdef MESS
		if (is_absolute_path)
		{
			*name = 0;
		}
#endif
		if (flags & FILEFLAG_CREATE_GAMEDIR)
		{
			if (osd_get_path_info(pathtype, pathindex, name) == PATH_NOT_FOUND)
				osd_create_directory(pathtype, pathindex, name);
		}

		/* if the directory exists, proceed */
		if (*name == 0 || osd_get_path_info(pathtype, pathindex, name) == PATH_IS_DIRECTORY)
		{
			/* now look for path/gamename/filename.ext */
			compose_path(name, sizeof(name), gamename, filename, extension);

			/* if we need checksums, load it into RAM and compute it along the way */
			if (flags & FILEFLAG_HASH)
			{
				if (checksum_file(pathtype, pathindex, name, &file.data, &file.length, file.hash) == 0)
				{
					file.type = RAM_FILE;
					break;
				}
			}

			/* otherwise, just open it straight */
			else
			{
				file.type = PLAIN_FILE;
				file.file = osd_fopen(pathtype, pathindex, name, access_modes[flags & 7], error);
				if (file.file == NULL && (flags & (3 | FILEFLAG_MUST_EXIST)) == 3)
					file.file = osd_fopen(pathtype, pathindex, name, "w+b", error);
				if (file.file != NULL)
					break;
				if (*error != FILEERR_NOT_FOUND)
				{
					pathindex = pathstop;	/* acknowledges the error */
					break;
				}
			}

#ifdef MESS
			if (flags & FILEFLAG_ZIP_PATHS)
			{
				int path_info = PATH_NOT_FOUND;
				const char *oldname = name;
				const char *zipentryname;
				char *newname = NULL;
				char *oldnewname = NULL;
				char *s;
				UINT32 ziplength;

				while ((oldname[0]) && ((path_info = osd_get_path_info(pathtype, pathindex, oldname)) == PATH_NOT_FOUND))
				{
					/* get name of parent directory into newname & oldname */
					newname = osd_dirname(oldname);

					/* if we are at a "blocking point", break out now */
					if (newname && !strcmp(oldname, newname))
					{
						free(newname);
						newname = NULL;
					}

					if (oldnewname)
						free(oldnewname);
					oldname = oldnewname = newname;
					if (!newname)
						break;

					/* remove any trailing path separator if needed */
					for (s = newname + strlen(newname) - 1; s >= newname && osd_is_path_separator(*s); s--)
						*s = '\0';
				}

				if (newname)
				{
					if ((oldname[0]) &&(path_info == PATH_IS_FILE))
					{
						zipentryname = name + strlen(newname);
						while(osd_is_path_separator(*zipentryname))
							zipentryname++;

						if (load_zipped_file(pathtype, pathindex, newname, zipentryname, &file.data, &ziplength) == 0)
						{
							unsigned functions;
							functions = hash_data_used_functions(hash);
							VPRINTF(("Using (mame_fopen) zip file for %s\n", filename));
							file.length = ziplength;
							file.type = ZIPPED_FILE;
							hash_compute(file.hash, file.data, file.length, functions);
							free(newname);
							break;
						}
					}
					free(newname);
				}
			}
			if (is_absolute_path)
				continue;
#endif
		}

		/* ----------------- STEP 2: OPEN THE FILE IN A ZIP -------------------- */

		/* now look for it within a ZIP file */
		if (!(flags & (FILEFLAG_OPENWRITE | FILEFLAG_NOZIP)))
		{
			/* first look for path/gamename.zip */
			compose_path(name, sizeof(name), gamename, NULL, "zip");
			VPRINTF(("Trying %s file\n", name));

			/* if the ZIP file exists, proceed */
			if (osd_get_path_info(pathtype, pathindex, name) == PATH_IS_FILE)
			{
				UINT32 ziplength;

				/* if the file was able to be extracted from the ZIP, continue */
				compose_path(tempname, sizeof(tempname), NULL, filename, extension);

				/* verify-only case */
				if (flags & FILEFLAG_VERIFY_ONLY)
				{
					UINT8 crcs[4];
					UINT32 crc = 0;

					/* Since this is a .ZIP file, we extract the CRC from the expected hash
                       (if any), so that we can load by CRC if needed. We must check that
                       the hash really contains a CRC, because it could be a NO_DUMP rom
                       for which we do not know the CRC yet. */
					if (hash && hash_data_extract_binary_checksum(hash, HASH_CRC, crcs) != 0)
					{
						/* Store the CRC in a single DWORD */
						crc = ((unsigned long)crcs[0] << 24) |
							  ((unsigned long)crcs[1] << 16) |
							  ((unsigned long)crcs[2] <<  8) |
							  ((unsigned long)crcs[3] <<  0);
					}

					hash_data_clear(file.hash);

					if (checksum_zipped_file(pathtype, pathindex, name, tempname, &ziplength, &crc) == 0)
					{
						file.length = ziplength;
						file.type = UNLOADED_ZIPPED_FILE;

						crcs[0] = (UINT8)(crc >> 24);
						crcs[1] = (UINT8)(crc >> 16);
						crcs[2] = (UINT8)(crc >> 8);
						crcs[3] = (UINT8)(crc >> 0);
						hash_data_insert_binary_checksum(file.hash, HASH_CRC, crcs);
						break;
					}
				}

				/* full load case */
				else
				{
					int err;

					/* Try loading the file */
					err = load_zipped_file(pathtype, pathindex, name, tempname, &file.data, &ziplength);

					/* If it failed, since this is a ZIP file, we can try to load by CRC
                       if an expected hash has been provided. unzip.c uses this ugly hack
                       of specifying the CRC as filename. */
					if (err && hash)
					{
						char crcn[9];

						if (hash_data_extract_printable_checksum(hash, HASH_CRC, crcn) != 0)
							err = load_zipped_file(pathtype, pathindex, name, crcn, &file.data, &ziplength);
					}

					if (err == 0)
					{
						unsigned functions;

						VPRINTF(("Using (mame_fopen) zip file for %s\n", filename));
						file.length = ziplength;
						file.type = ZIPPED_FILE;

						/* Since we already loaded the file, we can easily calculate the
                           checksum of all the functions. In practice, we use only the
                           functions for which we have an expected checksum to compare with. */
						functions = hash_data_used_functions(hash);

						hash_compute(file.hash, file.data, file.length, functions);
						break;
					}
				}
			}
		}
	}

	/* if we didn't succeed, just return NULL */
	if (pathindex == pathstop)
	{
		if (*error == FILEERR_SUCCESS)
			*error = FILEERR_NOT_FOUND;
		return NULL;
	}

	/* otherwise, duplicate the file */
	newfile = malloc(sizeof(file));
	if (newfile)
	{
		*newfile = file;
#ifdef DEBUG_COOKIE
		newfile->debug_cookie = DEBUG_COOKIE;
#endif
	}

	return newfile;
}


/*-------------------------------------------------
    checksum_file - load and checksum a file
-------------------------------------------------*/

static int checksum_file(int pathtype, int pathindex, const char *file, UINT8 **p, UINT64 *size, char *hash)
{
	UINT64 length;
	UINT8 *data;
	osd_file *f;
	unsigned int functions;
	osd_file_error dummy;

	/* open the file */
	f = osd_fopen(pathtype, pathindex, file, "rb", &dummy);
	if (!f)
		return -1;

	/* determine length of file */
	if (osd_fseek(f, 0L, SEEK_END) != 0)
	{
		osd_fclose(f);
		return -1;
	}

	length = osd_ftell(f);
	if (length == -1L)
	{
		osd_fclose(f);
		return -1;
	}

	/* allocate space for entire file */
	data = malloc(length);
	if (!data)
	{
		osd_fclose(f);
		return -1;
	}

	/* read entire file into memory */
	if (osd_fseek(f, 0L, SEEK_SET) != 0)
	{
		free(data);
		osd_fclose(f);
		return -1;
	}

	if (osd_fread(f, data, length) != length)
	{
		free(data);
		osd_fclose(f);
		return -1;
	}

	*size = length;

	/* compute the checksums (only the functions for which we have an expected
       checksum). Take also care of crconly: if the user asked, we will calculate
       only the CRC, but only if there is an expected CRC for this file. */
	functions = hash_data_used_functions(hash);
	hash_compute(hash, data, length, functions);

	/* if the caller wants the data, give it away, otherwise free it */
	if (p)
		*p = data;
	else
		free(data);

	/* close the file */
	osd_fclose(f);
	return 0;
}


/*-------------------------------------------------
    chd_open_cb - interface for opening
    a hard disk image
-------------------------------------------------*/

chd_interface_file *chd_open_cb(const char *filename, const char *mode)
{
	/* look for read-only drives first in the ROM path */
	if (mode[0] == 'r' && !strchr(mode, '+'))
	{
		const game_driver *drv;

		/* attempt reading up the chain through the parents */
		for (drv = Machine->gamedrv; drv != NULL; drv = driver_get_clone(drv))
		{
			void *file = mame_fopen(drv->name, filename, FILETYPE_IMAGE, 0);
			if (file != NULL)
				return file;
		}
		return NULL;
	}

	/* look for read/write drives in the diff area */
	return (chd_interface_file *)mame_fopen(NULL, filename, FILETYPE_IMAGE_DIFF, 1);
}


/*-------------------------------------------------
    chd_close_cb - interface for closing
    a hard disk image
-------------------------------------------------*/

void chd_close_cb(chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}


/*-------------------------------------------------
    chd_read_cb - interface for reading
    from a hard disk image
-------------------------------------------------*/

UINT32 chd_read_cb(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    chd_write_cb - interface for writing
    to a hard disk image
-------------------------------------------------*/

UINT32 chd_write_cb(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    chd_length_cb - interface for getting
    the length a hard disk image
-------------------------------------------------*/

UINT64 chd_length_cb(chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}
