// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    Generic MAME hard disk implementation, with differencing files

***************************************************************************/

#include "driver.h"
#include "harddisk.h"



/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct _hard_disk_file
{
	chd_file *			chd;				/* CHD file */
	chd_write_file *	diff;				/* writable CHD v5 overlay */
	hard_disk_info 		info;				/* hard disk info */
	UINT32				hunksectors;		/* sectors per hunk */
	UINT32				cachehunk;			/* which hunk is cached */
	UINT8 *				cache;				/* cache of the current hunk */
};



/*************************************
 *
 *  Open a hard disk
 *
 *************************************/

hard_disk_file *hard_disk_open_diff(chd_file *chd, chd_write_file *diff)
{
	int cylinders, heads, sectors, sectorbytes;
	const chd_header *header;
	hard_disk_file *file;
	char metadata[256];
	UINT32 metatag;
	UINT32 count;
	UINT8 metaflags;
	chd_error err;

	/* punt if neither CHD nor writable handle is provided */
	if (!chd && !diff)
		return NULL;

	/* read the hard disk metadata */
	err = CHDERR_METADATA_NOT_FOUND;
	if (diff)
		err = chd_diff_get_metadata(diff, HARD_DISK_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, &count, &metatag, &metaflags);
	if (err != CHDERR_NONE && chd)
		err = chd_get_metadata(chd, HARD_DISK_METADATA_TAG, 0, metadata, sizeof(metadata) - 1, &count, &metatag, &metaflags);
	if (err != CHDERR_NONE)
		return NULL;
	metadata[(count < sizeof(metadata)) ? count : (sizeof(metadata) - 1)] = 0;

	/* parse the metadata */
	if (sscanf(metadata, HARD_DISK_METADATA_FORMAT, &cylinders, &heads, &sectors, &sectorbytes) != 4)
		return NULL;
	if (sectorbytes <= 0)
		return NULL;

	header = diff ? chd_diff_get_header(diff) : chd_get_header(chd);
	if (!header || header->hunkbytes == 0 || (header->hunkbytes % (UINT32)sectorbytes) != 0)
		return NULL;

	/* allocate memory for the hard disk file */
	file = malloc(sizeof(hard_disk_file));
	if (!file)
		return NULL;

	/* fill in the data */
	file->chd = chd ? chd : chd_diff_get_parent(diff);
	file->diff = diff;
	file->info.cylinders = cylinders;
	file->info.heads = heads;
	file->info.sectors = sectors;
	file->info.sectorbytes = sectorbytes;
	file->hunksectors = header->hunkbytes / file->info.sectorbytes;
	file->cachehunk = -1;

	/* allocate a cache */
	file->cache = malloc(header->hunkbytes);
	if (!file->cache)
	{
		free(file);
		return NULL;
	}

	return file;
}

hard_disk_file *hard_disk_open(chd_file *chd)
{
	if (!chd)
		return NULL;
	return hard_disk_open_diff(chd, chd_lookup_writable(chd));
}



/*************************************
 *
 *  Close a hard disk
 *
 *************************************/

void hard_disk_close(hard_disk_file *file)
{
	if (!file)
		return;

	/* free the cache */
	if (file->cache)
		free(file->cache);
	free(file);
}



/*************************************
 *
 *  Return the handle to the CHD
 *
 *************************************/

chd_file *hard_disk_get_chd(hard_disk_file *file)
{
	return file->chd;
}



/*************************************
 *
 *  Return hard disk specific info
 *
 *************************************/

hard_disk_info *hard_disk_get_info(hard_disk_file *file)
{
	return &file->info;
}



/*************************************
 *
 *  Read from a hard disk
 *
 *************************************/

UINT32 hard_disk_read(hard_disk_file *file, UINT32 lbasector, UINT32 numsectors, void *buffer)
{
	UINT32 hunknum;
	UINT32 sectoroffs;

	if (!file)
		return 0;

	hunknum = lbasector / file->hunksectors;
	sectoroffs = lbasector % file->hunksectors;

	/* for now, just break down multisector reads into single sectors */
	if (numsectors > 1)
	{
		UINT32 total = 0;
		while (numsectors--)
		{
			if (hard_disk_read(file, lbasector++, 1, (UINT8 *)buffer + total * file->info.sectorbytes))
				total++;
			else
				break;
		}
		return total;
	}

	/* if we haven't cached this hunk, read it now */
	if (file->cachehunk != hunknum)
	{
		chd_error err;
		if (file->diff)
			err = chd_diff_read_hunk(file->diff, hunknum, file->cache);
		else
			err = chd_read(file->chd, hunknum, file->cache);
		if (err != CHDERR_NONE)
			return 0;
		file->cachehunk = hunknum;
	}

	/* copy out the requested sector */
	memcpy(buffer, &file->cache[sectoroffs * file->info.sectorbytes], file->info.sectorbytes);
	return 1;
}



/*************************************
 *
 *  Write to a hard disk
 *
 *************************************/

UINT32 hard_disk_write(hard_disk_file *file, UINT32 lbasector, UINT32 numsectors, const void *buffer)
{
	UINT32 hunknum;
	UINT32 sectoroffs;

	if (!file || !file->diff)
		return 0;

	hunknum = lbasector / file->hunksectors;
	sectoroffs = lbasector % file->hunksectors;

	/* for now, just break down multisector writes into single sectors */
	if (numsectors > 1)
	{
		UINT32 total = 0;
		while (numsectors--)
		{
			if (hard_disk_write(file, lbasector++, 1, (const UINT8 *)buffer + total * file->info.sectorbytes))
				total++;
			else
				break;
		}
		return total;
	}

	/* if we haven't cached this hunk, read it now */
	if (file->cachehunk != hunknum)
	{
		if (chd_diff_read_hunk(file->diff, hunknum, file->cache) != CHDERR_NONE)
			return 0;
		file->cachehunk = hunknum;
	}

	/* copy in the requested sector */
	memcpy(&file->cache[sectoroffs * file->info.sectorbytes], buffer, file->info.sectorbytes);

	/* write it back out */
	if (chd_diff_write_hunk(file->diff, hunknum, file->cache) != CHDERR_NONE)
	{
		file->cachehunk = -1;
		return 0;
	}
	return 1;
}
