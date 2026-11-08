// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    chdwrite.h

    Uncompressed CHD v5 differencing and writable image support

***************************************************************************/

#ifndef __CHDWRITE_H__
#define __CHDWRITE_H__

#include "mamecore.h"
#include "fileio.h"
#include "libchdr/chd.h"

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _chd_write_file chd_write_file;

typedef struct _chd_write_callbacks chd_write_callbacks;
struct _chd_write_callbacks
{
	uint64_t (*fsize)(void *file);
	size_t (*fread)(void *ptr, size_t size, size_t nmemb, void *file);
	size_t (*fwrite)(const void *ptr, size_t size, size_t nmemb, void *file);
	int (*fclose)(void *file);
	int (*fseek)(void *file, int64_t offset, int whence);
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* validate whether a parent CHD can be used for a CHD v5 differencing image */
chd_error chd_validate_parent(chd_file *parent);

/* create or open a CHD v5 differencing image using custom I/O callbacks */
chd_error chd_create_diff_callbacks(const chd_write_callbacks *callbacks, void *file, chd_file *parent, chd_write_file **out);
chd_error chd_open_diff_callbacks(const chd_write_callbacks *callbacks, void *file, chd_file *parent, chd_write_file **out);

/* create or open a standalone uncompressed CHD v5 image using custom I/O callbacks */
chd_error chd_create_uncompressed_v5_callbacks(const chd_write_callbacks *callbacks, void *file, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_write_file **out);
chd_error chd_open_uncompressed_v5_callbacks(const chd_write_callbacks *callbacks, void *file, chd_write_file **out);

/* create or open a CHD v5 differencing image backed by a mame_file */
chd_error chd_create_diff_file(mame_file *file, chd_file *parent, chd_write_file **out);
chd_error chd_open_diff_file(mame_file *file, chd_file *parent, chd_write_file **out);

/* create or open a standalone uncompressed CHD v5 image backed by a mame_file (non-owning close) */
chd_error chd_create_uncompressed_v5_file(mame_file *file, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_write_file **out);
chd_error chd_open_uncompressed_v5_file(mame_file *file, chd_write_file **out);

/* close a writable CHD v5 handle */
void chd_close_diff(chd_write_file *chd);

/* hunk read/write on a writable CHD v5 handle (reads parent on unallocated hunks if diff) */
chd_error chd_diff_read_hunk(chd_write_file *chd, uint32_t hunknum, void *buffer);
chd_error chd_diff_write_hunk(chd_write_file *chd, uint32_t hunknum, const void *buffer);

/* metadata access and modification on a writable CHD v5 handle */
chd_error chd_diff_get_metadata(chd_write_file *chd, uint32_t searchtag, uint32_t searchindex, void *output, uint32_t outputlen, uint32_t *resultlen, uint32_t *resulttag, uint8_t *resultflags);
chd_error chd_diff_write_metadata(chd_write_file *chd, uint32_t metatag, uint32_t metaindex, const void *inputbuf, uint32_t inputlen, uint8_t flags);
chd_error chd_diff_clone_all_metadata(chd_write_file *chd, chd_file *parent);

/* header and parent queries */
const chd_header *chd_diff_get_header(chd_write_file *chd);
chd_file *chd_diff_get_parent(chd_write_file *chd);

/* associate a writable CHD v5 handle with a libchdr chd_file handle */
void chd_register_writable(chd_file *chd, chd_write_file *writable);
chd_write_file *chd_lookup_writable(chd_file *chd);
/* standard core_file_callbacks backed by mame_file */
extern const core_file_callbacks mame_core_file_callbacks;
extern const core_file_callbacks mame_core_file_callbacks_no_close;

#ifdef __cplusplus
}
#endif

#endif /* __CHDWRITE_H__ */
