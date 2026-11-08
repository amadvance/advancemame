// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    chdwrite.c

    Uncompressed CHD v5 differencing and writable image support

***************************************************************************/

#include "chdwrite.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define CHD_V5_UNCOMPRESSED_MAP_ENTRY_BYTES	4
#define CHD_METADATA_HEADER_SIZE		16
#define CHD_MAX_METADATA_SIZE			(16 * 1024 * 1024 - 1)
#define CHD_MAX_METADATA_ENTRIES		65536
#define CHD_MAX_HUNK_BYTES			(65536 * 256)
#define CHD_V5_MAPOFFSET_OFFSET			40
#define CHD_V5_METAOFFSET_OFFSET		48
#define CHD_V5_RAWSHA1_OFFSET			64
#define CHD_V5_SHA1_OFFSET			84
#define CHD_V5_PARENTSHA1_OFFSET		104
#define CHD_CD_FRAME_SIZE			(2352 + 96)
#define CHD_MAX_REGISTERED_WRITABLES		32


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _chd_write_file
{
	const chd_write_callbacks *	callbacks;
	void *				file;
	chd_file *			parent;
	chd_header			header;
	uint8_t *			rawmap;
	uint64_t			rawmapsize;
};

typedef struct _chd_metadata_node chd_metadata_node;
struct _chd_metadata_node
{
	uint64_t	offset;
	uint64_t	prev;
	uint64_t	next;
	uint32_t	metatag;
	uint32_t	length;
	uint8_t		flags;
};

typedef struct _chd_writable_binding chd_writable_binding;
struct _chd_writable_binding
{
	chd_file *		chd;
	chd_write_file *	writable;
};


/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static const uint8_t nullsha1[CHD_SHA1_BYTES] = { 0 };
static chd_writable_binding writable_bindings[CHD_MAX_REGISTERED_WRITABLES];


/***************************************************************************
    BIG-ENDIAN SERIALIZATION HELPERS
***************************************************************************/

static uint32_t get_be24(const uint8_t *base)
{
	return ((uint32_t)base[0] << 16) | ((uint32_t)base[1] << 8) | (uint32_t)base[2];
}

static void put_be24(uint8_t *base, uint32_t value)
{
	base[0] = (uint8_t)((value >> 16) & 0xff);
	base[1] = (uint8_t)((value >> 8) & 0xff);
	base[2] = (uint8_t)(value & 0xff);
}

static uint32_t get_be32(const uint8_t *base)
{
	return ((uint32_t)base[0] << 24) | ((uint32_t)base[1] << 16) | ((uint32_t)base[2] << 8) | (uint32_t)base[3];
}

static void put_be32(uint8_t *base, uint32_t value)
{
	base[0] = (uint8_t)((value >> 24) & 0xff);
	base[1] = (uint8_t)((value >> 16) & 0xff);
	base[2] = (uint8_t)((value >> 8) & 0xff);
	base[3] = (uint8_t)(value & 0xff);
}

static uint64_t get_be64(const uint8_t *base)
{
	return ((uint64_t)get_be32(base) << 32) | (uint64_t)get_be32(base + 4);
}

static void put_be64(uint8_t *base, uint64_t value)
{
	put_be32(base, (uint32_t)(value >> 32));
	put_be32(base + 4, (uint32_t)(value & 0xffffffffU));
}


/***************************************************************************
    LOW-LEVEL FILE I/O HELPERS
***************************************************************************/

static int file_read_exact(const chd_write_callbacks *callbacks, void *file, uint64_t offset, void *buffer, size_t count)
{
	if (count == 0)
		return 1;
	if (!callbacks || !callbacks->fseek || !callbacks->fread)
		return 0;
	if (offset > (uint64_t)0x7fffffffffffffffULL)
		return 0;
	if (callbacks->fseek(file, (int64_t)offset, SEEK_SET) != 0)
		return 0;
	return (callbacks->fread(buffer, 1, count, file) == count);
}

static int file_write_exact(const chd_write_callbacks *callbacks, void *file, uint64_t offset, const void *buffer, size_t count)
{
	if (count == 0)
		return 1;
	if (!callbacks || !callbacks->fseek || !callbacks->fwrite)
		return 0;
	if (offset > (uint64_t)0x7fffffffffffffffULL)
		return 0;
	if (callbacks->fseek(file, (int64_t)offset, SEEK_SET) != 0)
		return 0;
	return (callbacks->fwrite(buffer, 1, count, file) == count);
}

static int file_write_zeros(const chd_write_callbacks *callbacks, void *file, uint64_t offset, uint64_t count)
{
	uint8_t zeros[4096];

	if (count == 0)
		return 1;
	memset(zeros, 0, sizeof(zeros));
	while (count > 0)
	{
		size_t chunk = (count > sizeof(zeros)) ? sizeof(zeros) : (size_t)count;
		if (!file_write_exact(callbacks, file, offset, zeros, chunk))
			return 0;
		offset += chunk;
		count -= chunk;
	}
	return 1;
}

static int file_append_aligned(const chd_write_callbacks *callbacks, void *file, const void *buffer, size_t count, uint32_t alignment, uint64_t *out_offset)
{
	uint64_t fsize;
	uint64_t padding = 0;
	uint64_t aligned_offs;

	if (!callbacks || !callbacks->fsize)
		return 0;

	fsize = callbacks->fsize(file);
	if ((int64_t)fsize < 0)
		return 0;

	if (alignment > 1)
	{
		uint64_t rem = fsize % (uint64_t)alignment;
		if (rem != 0)
			padding = (uint64_t)alignment - rem;
	}

	if (fsize + padding < fsize)
		return 0;
	aligned_offs = fsize + padding;
	if (aligned_offs + count < aligned_offs)
		return 0;

	if (padding > 0)
	{
		if (!file_write_zeros(callbacks, file, fsize, padding))
			return 0;
	}

	if (!file_write_exact(callbacks, file, aligned_offs, buffer, count))
		return 0;

	if (out_offset)
		*out_offset = aligned_offs;
	return 1;
}


/***************************************************************************
    MAME_FILE CALLBACK WRAPPERS
***************************************************************************/

static uint64_t mame_chd_fsize(void *file)
{
	return mame_fsize((mame_file *)file);
}

static size_t mame_chd_fread(void *ptr, size_t size, size_t nmemb, void *file)
{
	if (size == 0 || nmemb == 0)
		return 0;
	return mame_fread((mame_file *)file, ptr, size * nmemb) / size;
}

static size_t mame_chd_fwrite(const void *ptr, size_t size, size_t nmemb, void *file)
{
	if (size == 0 || nmemb == 0)
		return 0;
	return mame_fwrite((mame_file *)file, ptr, size * nmemb) / size;
}

static int mame_chd_fclose_owner(void *file)
{
	if (file)
		mame_fclose((mame_file *)file);
	return 0;
}

static int mame_chd_fclose_nonowner(void *file)
{
	(void)file;
	return 0;
}

static int mame_chd_fseek(void *file, int64_t offset, int whence)
{
	return mame_fseek((mame_file *)file, offset, whence);
}

static const chd_write_callbacks mame_owner_callbacks =
{
	mame_chd_fsize,
	mame_chd_fread,
	mame_chd_fwrite,
	mame_chd_fclose_owner,
	mame_chd_fseek
};

static const chd_write_callbacks mame_nonowner_callbacks =
{
	mame_chd_fsize,
	mame_chd_fread,
	mame_chd_fwrite,
	mame_chd_fclose_nonowner,
	mame_chd_fseek
};

const core_file_callbacks mame_core_file_callbacks =
{
	mame_chd_fsize,
	mame_chd_fread,
	mame_chd_fclose_owner,
	mame_chd_fseek
};

const core_file_callbacks mame_core_file_callbacks_no_close =
{
	mame_chd_fsize,
	mame_chd_fread,
	mame_chd_fclose_nonowner,
	mame_chd_fseek
};


/***************************************************************************
    GEOMETRY AND PARENT VALIDATION
***************************************************************************/

static uint32_t resolve_parent_unitbytes(chd_file *parent, const chd_header *pheader)
{
	char metadata[512];
	int cyls, heads, secs, bps;

	if (pheader->unitbytes != 0 && (pheader->hunkbytes % pheader->unitbytes) == 0)
		return pheader->unitbytes;

	if (chd_get_metadata(parent, HARD_DISK_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, NULL, NULL, NULL) == CHDERR_NONE)
	{
		metadata[sizeof(metadata) - 1] = 0;
		if (sscanf(metadata, HARD_DISK_METADATA_FORMAT, &cyls, &heads, &secs, &bps) == 4 &&
			bps > 0 && (pheader->hunkbytes % (uint32_t)bps) == 0)
			return (uint32_t)bps;
	}

	if (chd_get_metadata(parent, CDROM_OLD_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, NULL, NULL, NULL) == CHDERR_NONE ||
		chd_get_metadata(parent, CDROM_TRACK_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, NULL, NULL, NULL) == CHDERR_NONE ||
		chd_get_metadata(parent, CDROM_TRACK_METADATA2_TAG, 0, metadata, sizeof(metadata) - 1, NULL, NULL, NULL) == CHDERR_NONE ||
		chd_get_metadata(parent, GDROM_OLD_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, NULL, NULL, NULL) == CHDERR_NONE ||
		chd_get_metadata(parent, GDROM_TRACK_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, NULL, NULL, NULL) == CHDERR_NONE)
	{
		if ((pheader->hunkbytes % CHD_CD_FRAME_SIZE) == 0)
			return CHD_CD_FRAME_SIZE;
	}

	return pheader->hunkbytes;
}

static chd_error validate_geometry(uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, uint32_t *out_hunkcount, uint64_t *out_unitcount, uint64_t *out_rawmapsize)
{
	uint64_t hunkcount64;
	uint64_t unitcount64;
	uint64_t rawmapsize64;

	if (logicalbytes == 0 || hunkbytes == 0 || unitbytes == 0)
		return CHDERR_INVALID_PARAMETER;
	if (hunkbytes >= CHD_MAX_HUNK_BYTES)
		return CHDERR_INVALID_PARAMETER;
	if ((hunkbytes % unitbytes) != 0)
		return CHDERR_INVALID_PARAMETER;

	/* check arithmetic overflow on (logicalbytes + hunkbytes - 1) */
	if (logicalbytes + (uint64_t)(hunkbytes - 1) < logicalbytes)
		return CHDERR_INVALID_PARAMETER;
	hunkcount64 = (logicalbytes + (uint64_t)(hunkbytes - 1)) / (uint64_t)hunkbytes;
	if (hunkcount64 == 0 || hunkcount64 > (0xffffffffULL - 1))
		return CHDERR_INVALID_PARAMETER;

	/* check raw map size fits in memory */
	rawmapsize64 = hunkcount64 * CHD_V5_UNCOMPRESSED_MAP_ENTRY_BYTES;
	if (rawmapsize64 > 0x7fffffffULL)
		return CHDERR_INVALID_PARAMETER;

	if (logicalbytes + (uint64_t)(unitbytes - 1) < logicalbytes)
		return CHDERR_INVALID_PARAMETER;
	unitcount64 = (logicalbytes + (uint64_t)(unitbytes - 1)) / (uint64_t)unitbytes;

	if (out_hunkcount)
		*out_hunkcount = (uint32_t)hunkcount64;
	if (out_unitcount)
		*out_unitcount = unitcount64;
	if (out_rawmapsize)
		*out_rawmapsize = rawmapsize64;

	return CHDERR_NONE;
}

chd_error chd_validate_parent(chd_file *parent)
{
	const chd_header *pheader;
	uint32_t unitbytes;

	if (!parent)
		return CHDERR_INVALID_PARAMETER;

	pheader = chd_get_header(parent);
	if (!pheader)
		return CHDERR_INVALID_PARAMETER;

	/* CHD v5 differencing requires a parent with version >= 3 and a valid SHA1 */
	if (pheader->version < 3 || pheader->version > CHD_HEADER_VERSION)
		return CHDERR_UNSUPPORTED_VERSION;

	if (memcmp(nullsha1, pheader->sha1, CHD_SHA1_BYTES) == 0)
		return CHDERR_INVALID_PARENT;

	unitbytes = resolve_parent_unitbytes(parent, pheader);
	if (validate_geometry(pheader->logicalbytes, pheader->hunkbytes, unitbytes, NULL, NULL, NULL) != CHDERR_NONE)
		return CHDERR_INVALID_PARENT;

	return CHDERR_NONE;
}


/***************************************************************************
    METADATA CHAIN WALKING AND VALIDATION
***************************************************************************/

static chd_error metadata_find_node(chd_write_file *chd, uint32_t searchtag, uint32_t searchindex, chd_metadata_node *node)
{
	uint32_t steps = 0;

	node->offset = chd->header.metaoffset;
	node->prev = 0;

	while (node->offset != 0)
	{
		uint8_t raw_meta_header[CHD_METADATA_HEADER_SIZE];

		if (++steps > CHD_MAX_METADATA_ENTRIES)
			return CHDERR_INVALID_METADATA;

		if (node->offset < CHD_V5_HEADER_SIZE)
			return CHDERR_INVALID_METADATA;

		if (!file_read_exact(chd->callbacks, chd->file, node->offset, raw_meta_header, sizeof(raw_meta_header)))
			return CHDERR_READ_ERROR;

		node->metatag = get_be32(&raw_meta_header[0]);
		node->flags = raw_meta_header[4];
		node->length = get_be24(&raw_meta_header[5]);
		node->next = get_be64(&raw_meta_header[8]);

		if (node->length == 0)
			return CHDERR_INVALID_METADATA;

		if (searchtag == CHDMETATAG_WILDCARD || node->metatag == searchtag)
		{
			if (searchindex-- == 0)
				return CHDERR_NONE;
		}

		node->prev = node->offset;
		node->offset = node->next;
	}

	return CHDERR_METADATA_NOT_FOUND;
}

static chd_error metadata_set_previous_next(chd_write_file *chd, uint64_t prevoffset, uint64_t nextoffset)
{
	uint64_t write_offs;
	uint8_t rawbuf[8];

	if (prevoffset == 0)
		write_offs = CHD_V5_METAOFFSET_OFFSET;
	else
		write_offs = prevoffset + 8;

	put_be64(rawbuf, nextoffset);
	if (!file_write_exact(chd->callbacks, chd->file, write_offs, rawbuf, sizeof(rawbuf)))
		return CHDERR_WRITE_ERROR;

	if (prevoffset == 0)
		chd->header.metaoffset = nextoffset;

	return CHDERR_NONE;
}

static chd_error validate_metadata_chain(chd_write_file *chd, uint64_t file_size)
{
	uint64_t offset = chd->header.metaoffset;
	uint64_t map_end = chd->header.mapoffset + chd->rawmapsize;
	uint32_t steps = 0;

	while (offset != 0)
	{
		uint8_t raw_meta_header[CHD_METADATA_HEADER_SIZE];
		uint32_t length;
		uint64_t next;
		uint64_t entry_end;

		if (++steps > CHD_MAX_METADATA_ENTRIES)
			return CHDERR_INVALID_FILE;

		/* metadata must not overlap the header or hunk map */
		if (offset < CHD_V5_HEADER_SIZE)
			return CHDERR_INVALID_FILE;
		if (offset < map_end && (offset + CHD_METADATA_HEADER_SIZE) > chd->header.mapoffset)
			return CHDERR_INVALID_FILE;

		/* metadata header must fit within file_size */
		if (offset + CHD_METADATA_HEADER_SIZE < offset || offset + CHD_METADATA_HEADER_SIZE > file_size)
			return CHDERR_INVALID_FILE;

		if (!file_read_exact(chd->callbacks, chd->file, offset, raw_meta_header, sizeof(raw_meta_header)))
			return CHDERR_INVALID_FILE;

		length = get_be24(&raw_meta_header[5]);
		next = get_be64(&raw_meta_header[8]);

		if (length == 0)
			return CHDERR_INVALID_FILE;

		entry_end = offset + CHD_METADATA_HEADER_SIZE + (uint64_t)length;
		if (entry_end < offset || entry_end > file_size)
			return CHDERR_INVALID_FILE;
		if (offset < map_end && entry_end > chd->header.mapoffset)
			return CHDERR_INVALID_FILE;

		offset = next;
	}

	return CHDERR_NONE;
}


/***************************************************************************
    CREATE UNCOMPRESSED CHD V5 / DIFFERENCING FILE
***************************************************************************/

static chd_error create_common_v5(const chd_write_callbacks *callbacks, void *file, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_file *parent, chd_write_file **out)
{
	uint8_t rawheader[CHD_V5_HEADER_SIZE];
	uint32_t hunkcount = 0;
	uint64_t unitcount = 0;
	uint64_t rawmapsize = 0;
	chd_write_file *chd = NULL;
	const chd_header *pheader = NULL;
	chd_error err;

	if (!out)
		return CHDERR_INVALID_PARAMETER;
	*out = NULL;

	if (!callbacks || !file || !callbacks->fwrite || !callbacks->fseek || !callbacks->fread || !callbacks->fsize)
		return CHDERR_INVALID_PARAMETER;

	if (parent != NULL)
	{
		err = chd_validate_parent(parent);
		if (err != CHDERR_NONE)
		{
			if (callbacks->fclose)
				callbacks->fclose(file);
			return err;
		}
		pheader = chd_get_header(parent);
		logicalbytes = pheader->logicalbytes;
		hunkbytes = pheader->hunkbytes;
		unitbytes = resolve_parent_unitbytes(parent, pheader);
	}

	err = validate_geometry(logicalbytes, hunkbytes, unitbytes, &hunkcount, &unitcount, &rawmapsize);
	if (err != CHDERR_NONE)
	{
		if (callbacks->fclose)
			callbacks->fclose(file);
		return err;
	}

	chd = (chd_write_file *)malloc(sizeof(*chd));
	if (!chd)
	{
		if (callbacks->fclose)
			callbacks->fclose(file);
		return CHDERR_OUT_OF_MEMORY;
	}
	memset(chd, 0, sizeof(*chd));

	chd->rawmap = (uint8_t *)malloc((size_t)rawmapsize);
	if (!chd->rawmap)
	{
		free(chd);
		if (callbacks->fclose)
			callbacks->fclose(file);
		return CHDERR_OUT_OF_MEMORY;
	}
	memset(chd->rawmap, 0, (size_t)rawmapsize);

	chd->callbacks = callbacks;
	chd->file = file;
	chd->parent = parent;
	chd->rawmapsize = rawmapsize;

	chd->header.length = CHD_V5_HEADER_SIZE;
	chd->header.version = CHD_HEADER_VERSION;
	chd->header.flags = CHDFLAGS_IS_WRITEABLE | ((parent != NULL) ? CHDFLAGS_HAS_PARENT : 0);
	chd->header.compression[0] = CHD_CODEC_NONE;
	chd->header.compression[1] = CHD_CODEC_NONE;
	chd->header.compression[2] = CHD_CODEC_NONE;
	chd->header.compression[3] = CHD_CODEC_NONE;
	chd->header.hunkbytes = hunkbytes;
	chd->header.totalhunks = hunkcount;
	chd->header.hunkcount = hunkcount;
	chd->header.logicalbytes = logicalbytes;
	chd->header.mapoffset = CHD_V5_HEADER_SIZE;
	chd->header.metaoffset = 0;
	chd->header.unitbytes = unitbytes;
	chd->header.unitcount = unitcount;
	chd->header.mapentrybytes = CHD_V5_UNCOMPRESSED_MAP_ENTRY_BYTES;
	chd->header.rawmap = chd->rawmap;
	if (pheader != NULL)
		memcpy(chd->header.parentsha1, pheader->sha1, CHD_SHA1_BYTES);

	/* build the 124-byte CHD v5 header */
	memset(rawheader, 0, sizeof(rawheader));
	memcpy(&rawheader[0], "MComprHD", 8);
	put_be32(&rawheader[8], CHD_V5_HEADER_SIZE);
	put_be32(&rawheader[12], CHD_HEADER_VERSION);
	put_be32(&rawheader[16], CHD_CODEC_NONE);
	put_be32(&rawheader[20], CHD_CODEC_NONE);
	put_be32(&rawheader[24], CHD_CODEC_NONE);
	put_be32(&rawheader[28], CHD_CODEC_NONE);
	put_be64(&rawheader[32], logicalbytes);
	put_be64(&rawheader[CHD_V5_MAPOFFSET_OFFSET], chd->header.mapoffset);
	put_be64(&rawheader[CHD_V5_METAOFFSET_OFFSET], 0);
	put_be32(&rawheader[56], hunkbytes);
	put_be32(&rawheader[60], unitbytes);
	/* rawsha1 [64..83] and sha1 [84..103] remain all-zero for uncompressed writable v5 */
	if (pheader != NULL)
		memcpy(&rawheader[CHD_V5_PARENTSHA1_OFFSET], pheader->sha1, CHD_SHA1_BYTES);

	/* write the header and zeroed hunk map */
	if (!file_write_exact(callbacks, file, 0, rawheader, sizeof(rawheader)) ||
		!file_write_exact(callbacks, file, chd->header.mapoffset, chd->rawmap, (size_t)rawmapsize))
	{
		chd_close_diff(chd);
		return CHDERR_CANT_CREATE_FILE;
	}

	/* clone all metadata from parent when creating a differencing image */
	if (parent != NULL)
	{
		err = chd_diff_clone_all_metadata(chd, parent);
		if (err != CHDERR_NONE)
		{
			chd_close_diff(chd);
			return err;
		}
	}

	*out = chd;
	return CHDERR_NONE;
}

chd_error chd_create_diff_callbacks(const chd_write_callbacks *callbacks, void *file, chd_file *parent, chd_write_file **out)
{
	if (!parent)
	{
		if (callbacks && callbacks->fclose && file)
			callbacks->fclose(file);
		if (out)
			*out = NULL;
		return CHDERR_INVALID_PARAMETER;
	}
	return create_common_v5(callbacks, file, 0, 0, 0, parent, out);
}

chd_error chd_create_uncompressed_v5_callbacks(const chd_write_callbacks *callbacks, void *file, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_write_file **out)
{
	return create_common_v5(callbacks, file, logicalbytes, hunkbytes, unitbytes, NULL, out);
}


/***************************************************************************
    OPEN EXISTING UNCOMPRESSED CHD V5 / DIFFERENCING FILE
***************************************************************************/

static chd_error open_common_v5(const chd_write_callbacks *callbacks, void *file, chd_file *parent, int require_parent, chd_write_file **out)
{
	uint8_t rawheader[CHD_V5_HEADER_SIZE];
	uint64_t file_size;
	uint32_t hdr_length;
	uint32_t hdr_version;
	uint32_t comp0, comp1, comp2, comp3;
	uint64_t logicalbytes;
	uint64_t mapoffset;
	uint64_t metaoffset;
	uint32_t hunkbytes;
	uint32_t unitbytes;
	uint32_t hunkcount = 0;
	uint64_t unitcount = 0;
	uint64_t rawmapsize = 0;
	uint32_t h;
	chd_write_file *chd = NULL;
	const chd_header *pheader = NULL;
	chd_error err;

	if (!out)
		return CHDERR_INVALID_PARAMETER;
	*out = NULL;

	if (!callbacks || !file || !callbacks->fread || !callbacks->fwrite || !callbacks->fseek || !callbacks->fsize)
		return CHDERR_INVALID_PARAMETER;

	if (require_parent)
	{
		if (!parent)
		{
			err = CHDERR_REQUIRES_PARENT;
			goto cleanup;
		}
		err = chd_validate_parent(parent);
		if (err != CHDERR_NONE)
			goto cleanup;
		pheader = chd_get_header(parent);
	}

	file_size = callbacks->fsize(file);
	if ((int64_t)file_size < 16)
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}

	/* read the first 16 bytes to inspect signature, header length, and version */
	if (!file_read_exact(callbacks, file, 0, rawheader, 16))
	{
		err = CHDERR_READ_ERROR;
		goto cleanup;
	}

	if (memcmp(rawheader, "MComprHD", 8) != 0)
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}

	hdr_length = get_be32(&rawheader[8]);
	hdr_version = get_be32(&rawheader[12]);

	/* writable differencing images require CHD v5 */
	if (hdr_version != CHD_HEADER_VERSION)
	{
		err = CHDERR_UNSUPPORTED_VERSION;
		goto cleanup;
	}
	if (hdr_length != CHD_V5_HEADER_SIZE)
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}

	/* version is 5 and length is 124; read the rest of the header */
	if (file_size < CHD_V5_HEADER_SIZE)
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}
	if (!file_read_exact(callbacks, file, 16, &rawheader[16], CHD_V5_HEADER_SIZE - 16))
	{
		err = CHDERR_READ_ERROR;
		goto cleanup;
	}

	comp0 = get_be32(&rawheader[16]);
	comp1 = get_be32(&rawheader[20]);
	comp2 = get_be32(&rawheader[24]);
	comp3 = get_be32(&rawheader[28]);
	if (comp0 != CHD_CODEC_NONE || comp1 != CHD_CODEC_NONE || comp2 != CHD_CODEC_NONE || comp3 != CHD_CODEC_NONE)
	{
		err = CHDERR_FILE_NOT_WRITEABLE;
		goto cleanup;
	}

	logicalbytes = get_be64(&rawheader[32]);
	mapoffset = get_be64(&rawheader[CHD_V5_MAPOFFSET_OFFSET]);
	metaoffset = get_be64(&rawheader[CHD_V5_METAOFFSET_OFFSET]);
	hunkbytes = get_be32(&rawheader[56]);
	unitbytes = get_be32(&rawheader[60]);

	if (validate_geometry(logicalbytes, hunkbytes, unitbytes, &hunkcount, &unitcount, &rawmapsize) != CHDERR_NONE)
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}

	/* validate parent relationship and geometry compatibility */
	if (require_parent)
	{
		uint32_t expected_unitbytes = resolve_parent_unitbytes(parent, pheader);

		if (memcmp(nullsha1, &rawheader[CHD_V5_PARENTSHA1_OFFSET], CHD_SHA1_BYTES) == 0 ||
		    memcmp(&rawheader[CHD_V5_PARENTSHA1_OFFSET], pheader->sha1, CHD_SHA1_BYTES) != 0 ||
		    logicalbytes != pheader->logicalbytes || hunkbytes != pheader->hunkbytes || unitbytes != expected_unitbytes)
		{
			err = CHDERR_INVALID_PARENT;
			goto cleanup;
		}
	}
	else
	{
		if (memcmp(nullsha1, &rawheader[CHD_V5_PARENTSHA1_OFFSET], CHD_SHA1_BYTES) != 0)
		{
			err = CHDERR_REQUIRES_PARENT;
			goto cleanup;
		}
	}

	/* validate map offset and bounds */
	if (mapoffset < CHD_V5_HEADER_SIZE || mapoffset + rawmapsize < mapoffset || mapoffset + rawmapsize > file_size)
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}

	chd = (chd_write_file *)malloc(sizeof(*chd));
	if (!chd)
	{
		err = CHDERR_OUT_OF_MEMORY;
		goto cleanup;
	}
	memset(chd, 0, sizeof(*chd));
	chd->callbacks = callbacks;
	chd->file = file;
	chd->parent = parent;
	chd->rawmapsize = rawmapsize;

	chd->rawmap = (uint8_t *)malloc((size_t)rawmapsize);
	if (!chd->rawmap)
	{
		err = CHDERR_OUT_OF_MEMORY;
		goto cleanup;
	}

	chd->header.length = CHD_V5_HEADER_SIZE;
	chd->header.version = CHD_HEADER_VERSION;
	chd->header.flags = CHDFLAGS_IS_WRITEABLE | (require_parent ? CHDFLAGS_HAS_PARENT : 0);
	chd->header.compression[0] = CHD_CODEC_NONE;
	chd->header.compression[1] = CHD_CODEC_NONE;
	chd->header.compression[2] = CHD_CODEC_NONE;
	chd->header.compression[3] = CHD_CODEC_NONE;
	chd->header.hunkbytes = hunkbytes;
	chd->header.totalhunks = hunkcount;
	chd->header.hunkcount = hunkcount;
	chd->header.logicalbytes = logicalbytes;
	chd->header.mapoffset = mapoffset;
	chd->header.metaoffset = metaoffset;
	chd->header.unitbytes = unitbytes;
	chd->header.unitcount = unitcount;
	chd->header.mapentrybytes = CHD_V5_UNCOMPRESSED_MAP_ENTRY_BYTES;
	chd->header.rawmap = chd->rawmap;
	memcpy(chd->header.rawsha1, &rawheader[CHD_V5_RAWSHA1_OFFSET], CHD_SHA1_BYTES);
	memcpy(chd->header.sha1, &rawheader[CHD_V5_SHA1_OFFSET], CHD_SHA1_BYTES);
	memcpy(chd->header.parentsha1, &rawheader[CHD_V5_PARENTSHA1_OFFSET], CHD_SHA1_BYTES);

	/* read the uncompressed hunk map */
	if (!file_read_exact(callbacks, file, mapoffset, chd->rawmap, (size_t)rawmapsize))
	{
		err = CHDERR_READ_ERROR;
		goto cleanup;
	}

	/* validate every allocated hunk entry in the map */
	for (h = 0; h < hunkcount; h++)
	{
		uint32_t blockindex = get_be32(&chd->rawmap[(size_t)h * 4]);
		if (blockindex != 0)
		{
			uint64_t blockoffs = (uint64_t)blockindex * (uint64_t)hunkbytes;
			uint64_t blockend = blockoffs + (uint64_t)hunkbytes;
			uint64_t mapend = mapoffset + rawmapsize;

			if (blockoffs / (uint64_t)hunkbytes != (uint64_t)blockindex ||
			    blockoffs < CHD_V5_HEADER_SIZE ||
			    (blockoffs < mapend && blockend > mapoffset) ||
			    blockend < blockoffs || blockend > file_size)
			{
				err = CHDERR_INVALID_FILE;
				goto cleanup;
			}
		}
	}

	/* validate metadata linked list */
	err = validate_metadata_chain(chd, file_size);
	if (err != CHDERR_NONE)
		goto cleanup;

	/* verify that the underlying file is actually writable by rewriting the magic tag in-place */
	if (!file_write_exact(callbacks, file, 0, rawheader, 8))
	{
		err = CHDERR_FILE_NOT_WRITEABLE;
		goto cleanup;
	}

	*out = chd;
	return CHDERR_NONE;

cleanup:
	if (chd)
		chd_close_diff(chd);
	else if (callbacks && callbacks->fclose && file)
		callbacks->fclose(file);
	return err;
}

chd_error chd_open_diff_callbacks(const chd_write_callbacks *callbacks, void *file, chd_file *parent, chd_write_file **out)
{
	return open_common_v5(callbacks, file, parent, 1, out);
}

chd_error chd_open_uncompressed_v5_callbacks(const chd_write_callbacks *callbacks, void *file, chd_write_file **out)
{
	return open_common_v5(callbacks, file, NULL, 0, out);
}


/***************************************************************************
    MAME_FILE CONVENIENCE WRAPPERS
***************************************************************************/

chd_error chd_create_diff_file(mame_file *file, chd_file *parent, chd_write_file **out)
{
	return chd_create_diff_callbacks(&mame_owner_callbacks, file, parent, out);
}

chd_error chd_open_diff_file(mame_file *file, chd_file *parent, chd_write_file **out)
{
	return chd_open_diff_callbacks(&mame_owner_callbacks, file, parent, out);
}

chd_error chd_create_uncompressed_v5_file(mame_file *file, uint64_t logicalbytes, uint32_t hunkbytes, uint32_t unitbytes, chd_write_file **out)
{
	return chd_create_uncompressed_v5_callbacks(&mame_nonowner_callbacks, file, logicalbytes, hunkbytes, unitbytes, out);
}

chd_error chd_open_uncompressed_v5_file(mame_file *file, chd_write_file **out)
{
	return chd_open_uncompressed_v5_callbacks(&mame_nonowner_callbacks, file, out);
}


/***************************************************************************
    CLOSE WRITABLE HANDLE
***************************************************************************/

void chd_close_diff(chd_write_file *chd)
{
	int i;

	if (!chd)
		return;

	for (i = 0; i < CHD_MAX_REGISTERED_WRITABLES; i++)
	{
		if (writable_bindings[i].writable == chd)
		{
			writable_bindings[i].chd = NULL;
			writable_bindings[i].writable = NULL;
		}
	}

	if (chd->callbacks && chd->callbacks->fclose && chd->file)
		chd->callbacks->fclose(chd->file);

	if (chd->rawmap)
		free(chd->rawmap);

	free(chd);
}


/***************************************************************************
    HUNK READ AND WRITE
***************************************************************************/

chd_error chd_diff_read_hunk(chd_write_file *chd, uint32_t hunknum, void *buffer)
{
	uint32_t blockindex;

	if (!chd || !buffer)
		return CHDERR_INVALID_PARAMETER;
	if (hunknum >= chd->header.totalhunks)
		return CHDERR_HUNK_OUT_OF_RANGE;

	blockindex = get_be32(&chd->rawmap[(size_t)hunknum * 4]);
	if (blockindex != 0)
	{
		uint64_t blockoffs = (uint64_t)blockindex * (uint64_t)chd->header.hunkbytes;
		if (!file_read_exact(chd->callbacks, chd->file, blockoffs, buffer, chd->header.hunkbytes))
			return CHDERR_READ_ERROR;
		return CHDERR_NONE;
	}

	if (chd->parent != NULL)
		return chd_read(chd->parent, hunknum, buffer);

	memset(buffer, 0, chd->header.hunkbytes);
	return CHDERR_NONE;
}

chd_error chd_diff_write_hunk(chd_write_file *chd, uint32_t hunknum, const void *buffer)
{
	uint32_t blockindex;
	uint64_t blockoffs;

	if (!chd || !buffer)
		return CHDERR_INVALID_PARAMETER;
	if (hunknum >= chd->header.totalhunks)
		return CHDERR_HUNK_OUT_OF_RANGE;

	blockindex = get_be32(&chd->rawmap[(size_t)hunknum * 4]);
	if (blockindex != 0)
	{
		blockoffs = (uint64_t)blockindex * (uint64_t)chd->header.hunkbytes;
		if (!file_write_exact(chd->callbacks, chd->file, blockoffs, buffer, chd->header.hunkbytes))
			return CHDERR_WRITE_ERROR;
		return CHDERR_NONE;
	}

	/* For a differencing image (chd->parent != NULL), a zero map entry means
	 * "inherit from parent", NOT "all zeros". Therefore we must ALWAYS allocate
	 * and write the hunk in a .dif file, even when the written data is all zeros. */
	{
		uint64_t aligned_offs = 0;
		uint64_t new_index64;
		uint32_t new_index;
		uint8_t rawentry[4];
		uint64_t map_entry_offs;

		/* write the hunk data to a hunk-aligned offset at EOF */
		if (!file_append_aligned(chd->callbacks, chd->file, buffer, chd->header.hunkbytes, chd->header.hunkbytes, &aligned_offs))
			return CHDERR_WRITE_ERROR;

		new_index64 = aligned_offs / (uint64_t)chd->header.hunkbytes;
		if (new_index64 == 0 || new_index64 > 0xffffffffULL)
			return CHDERR_WRITE_ERROR;
		new_index = (uint32_t)new_index64;

		/* publish the map entry on disk first, then update the in-memory map */
		put_be32(rawentry, new_index);
		map_entry_offs = chd->header.mapoffset + (uint64_t)hunknum * CHD_V5_UNCOMPRESSED_MAP_ENTRY_BYTES;
		if (!file_write_exact(chd->callbacks, chd->file, map_entry_offs, rawentry, sizeof(rawentry)))
			return CHDERR_WRITE_ERROR;

		memcpy(&chd->rawmap[(size_t)hunknum * 4], rawentry, sizeof(rawentry));
	}

	return CHDERR_NONE;
}


/***************************************************************************
    METADATA OPERATIONS
***************************************************************************/

chd_error chd_diff_get_metadata(chd_write_file *chd, uint32_t searchtag, uint32_t searchindex, void *output, uint32_t outputlen, uint32_t *resultlen, uint32_t *resulttag, uint8_t *resultflags)
{
	chd_metadata_node node;
	chd_error err;
	uint32_t copy_len;

	if (!chd)
		return CHDERR_INVALID_PARAMETER;

	memset(&node, 0, sizeof(node));
	err = metadata_find_node(chd, searchtag, searchindex, &node);
	if (err != CHDERR_NONE)
		return err;

	copy_len = (outputlen < node.length) ? outputlen : node.length;
	if (output != NULL && copy_len > 0)
	{
		if (!file_read_exact(chd->callbacks, chd->file, node.offset + CHD_METADATA_HEADER_SIZE, output, copy_len))
			return CHDERR_READ_ERROR;
	}

	if (resultlen)
		*resultlen = node.length;
	if (resulttag)
		*resulttag = node.metatag;
	if (resultflags)
		*resultflags = node.flags;

	return CHDERR_NONE;
}

chd_error chd_diff_write_metadata(chd_write_file *chd, uint32_t metatag, uint32_t metaindex, const void *inputbuf, uint32_t inputlen, uint8_t flags)
{
	uint8_t raw_meta_header[CHD_METADATA_HEADER_SIZE];
	uint64_t replacement_next = 0;
	uint64_t offset = 0;
	chd_metadata_node node;
	chd_error err;

	if (!chd || !inputbuf)
		return CHDERR_INVALID_PARAMETER;
	if (inputlen < 1 || inputlen > CHD_MAX_METADATA_SIZE)
		return CHDERR_INVALID_METADATA_SIZE;

	memset(&node, 0, sizeof(node));
	err = metadata_find_node(chd, metatag, metaindex, &node);
	if (err == CHDERR_NONE)
	{
		if (inputlen <= node.length)
		{
			if (!file_write_exact(chd->callbacks, chd->file, node.offset + CHD_METADATA_HEADER_SIZE, inputbuf, inputlen))
				return CHDERR_WRITE_ERROR;

			if (inputlen != node.length)
			{
				uint8_t rawlen[3];
				put_be24(rawlen, inputlen);
				if (!file_write_exact(chd->callbacks, chd->file, node.offset + 5, rawlen, sizeof(rawlen)))
					return CHDERR_WRITE_ERROR;
			}
			return CHDERR_NONE;
		}

		/* the replacement inherits the successor, the old record is unlinked only after the replacement is written */
		replacement_next = node.next;
	}
	else if (err != CHDERR_METADATA_NOT_FOUND)
	{
		return err;
	}

	put_be32(&raw_meta_header[0], metatag);
	raw_meta_header[4] = flags;
	put_be24(&raw_meta_header[5], inputlen & 0x00ffffffU);
	put_be64(&raw_meta_header[8], replacement_next);

	if (!file_append_aligned(chd->callbacks, chd->file, raw_meta_header, sizeof(raw_meta_header), 1, &offset))
		return CHDERR_WRITE_ERROR;
	if (!file_write_exact(chd->callbacks, chd->file, offset + sizeof(raw_meta_header), inputbuf, inputlen))
		return CHDERR_WRITE_ERROR;

	err = metadata_set_previous_next(chd, node.prev, offset);
	if (err != CHDERR_NONE)
		return err;

	return CHDERR_NONE;
}

chd_error chd_diff_clone_all_metadata(chd_write_file *chd, chd_file *parent)
{
	uint32_t index;
	uint8_t *buf = NULL;
	uint32_t buf_cap = 0;

	if (!chd || !parent)
		return CHDERR_INVALID_PARAMETER;

	for (index = 0; index < CHD_MAX_METADATA_ENTRIES; index++)
	{
		uint8_t dummy = 0;
		uint32_t resultlen = 0;
		uint32_t resulttag = 0;
		uint8_t resultflags = CHD_MDFLAGS_CHECKSUM;
		chd_error err;

		err = chd_get_metadata(parent, CHDMETATAG_WILDCARD, index, &dummy, 0, &resultlen, &resulttag, &resultflags);
		if (err == CHDERR_METADATA_NOT_FOUND)
			break;
		if (err != CHDERR_NONE)
		{
			if (buf)
				free(buf);
			return err;
		}

		if (resultlen == 0 || resultlen > CHD_MAX_METADATA_SIZE)
		{
			if (buf)
				free(buf);
			return CHDERR_INVALID_METADATA_SIZE;
		}

		if (resultlen > buf_cap)
		{
			uint8_t *newbuf = (uint8_t *)realloc(buf, resultlen);
			if (!newbuf)
			{
				if (buf)
					free(buf);
				return CHDERR_OUT_OF_MEMORY;
			}
			buf = newbuf;
			buf_cap = resultlen;
		}

		err = chd_get_metadata(parent, CHDMETATAG_WILDCARD, index, buf, resultlen, &resultlen, &resulttag, &resultflags);
		if (err != CHDERR_NONE)
		{
			free(buf);
			return err;
		}

		err = chd_diff_write_metadata(chd, resulttag, CHD_METAINDEX_APPEND, buf, resultlen, resultflags);
		if (err != CHDERR_NONE)
		{
			free(buf);
			return err;
		}
	}

	if (buf)
		free(buf);
	return CHDERR_NONE;
}


/***************************************************************************
    HEADER AND PARENT QUERIES
***************************************************************************/

const chd_header *chd_diff_get_header(chd_write_file *chd)
{
	return chd ? &chd->header : NULL;
}

chd_file *chd_diff_get_parent(chd_write_file *chd)
{
	return chd ? chd->parent : NULL;
}


/***************************************************************************
    WRITABLE HANDLE REGISTRATION
***************************************************************************/

void chd_register_writable(chd_file *chd, chd_write_file *writable)
{
	int i;

	if (!chd || !writable)
		return;

	for (i = 0; i < CHD_MAX_REGISTERED_WRITABLES; i++)
	{
		if (writable_bindings[i].chd == chd)
		{
			writable_bindings[i].writable = writable;
			return;
		}
	}

	for (i = 0; i < CHD_MAX_REGISTERED_WRITABLES; i++)
	{
		if (writable_bindings[i].chd == NULL)
		{
			writable_bindings[i].chd = chd;
			writable_bindings[i].writable = writable;
			return;
		}
	}
}

chd_write_file *chd_lookup_writable(chd_file *chd)
{
	int i;

	if (!chd)
		return NULL;

	for (i = 0; i < CHD_MAX_REGISTERED_WRITABLES; i++)
	{
		if (writable_bindings[i].chd == chd)
			return writable_bindings[i].writable;
	}
	return NULL;
}
