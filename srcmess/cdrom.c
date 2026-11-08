// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

  Generic MAME CD-ROM utilties - build IDE and SCSI CD-ROMs on top of this

  IMPORTANT:
  "physical" block addresses are the actual addresses on the emulated CD.
  "chd" block addresses are the block addresses in the CHD file.
  Because we pad each track to a hunk boundry, these addressing
  schemes will differ after track 1!

***************************************************************************/

#include "cdrom.h"

#define VERBOSE	(0)

/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct _cdrom_file
{
	chd_file *chd;				/* CHD file */
	cdrom_toc 		cdtoc;		/* TOC for the CD */
	UINT32				hunksectors;	/* sectors per hunk */
	UINT32				cachehunk;	/* which hunk is cached */
	UINT8 *				cache;		/* cache of the current hunk */

	INT8				audio_playing, audio_pause, audio_ended_normally;
	UINT32				audio_lba, audio_length;

	UINT8 *				audio_cache;
	UINT32				audio_samples;
	INT16 *				audio_bptr;
};



/*************************************
 *
 *  Utility functions
 *
 *************************************/

/* get the track number for a physical frame number */
UINT32 cdrom_get_track_phys(cdrom_file *file, UINT32 frame)
{
	int i;

	for (i = 1; i < file->cdtoc.numtrks+1; i++)
	{
		if (frame < file->cdtoc.tracks[i].physframeofs)
		{
			return i-1;
		}
	}

	#if VERBOSE
	logerror("CDROM: could not find track for frame %d\n", frame);
	#endif
	return 0;
}

/* get the track number for a CHD frame number */
UINT32 cdrom_get_track_chd(cdrom_file *file, UINT32 frame)
{
	int i;

	for (i = 1; i < file->cdtoc.numtrks+1; i++)
	{
		if (frame < file->cdtoc.tracks[i].chdframeofs)
		{
			return i-1;
		}
	}

	#if VERBOSE
	logerror("CDROM: could not find track for frame %d\n", frame);
	#endif
	return 0;
}

/* get physical frame number that a track starts at */
UINT32 cdrom_get_phys_start_of_track(cdrom_file *file, UINT32 track)
{
	return file->cdtoc.tracks[track].physframeofs;
}

/* get CHD frame number that a track starts at */
UINT32 cdrom_get_chd_start_of_track(cdrom_file *file, UINT32 track)
{
	// handle lead-out specially
	if (track == 0xaa)
	{
		return file->cdtoc.tracks[file->cdtoc.numtrks-1].chdframeofs + file->cdtoc.tracks[file->cdtoc.numtrks-1].frames;
	}

	return file->cdtoc.tracks[track].chdframeofs;
}

/* convert a physical frame number to a CHD one */
UINT32 cdrom_phys_frame_to_chd(cdrom_file *file, UINT32 frame)
{
	UINT32 trk = cdrom_get_track_phys(file, frame);

	frame -= cdrom_get_phys_start_of_track(file, trk);
	frame += cdrom_get_chd_start_of_track(file, trk);

	return frame;
}

/* convert a CHD frame number to a physical one */
UINT32 cdrom_chd_frame_to_phys(cdrom_file *file, UINT32 frame)
{
	UINT32 trk = cdrom_get_track_chd(file, frame);

	frame -= cdrom_get_chd_start_of_track(file, trk);
	frame += cdrom_get_phys_start_of_track(file, trk);

	return frame;
}

/* internal utility functions */

static int endian_mode = 0;

INLINE UINT32 get_bigendian_uint32(const UINT8 *base)
{
	if (!endian_mode)
		return (base[3] << 24) | (base[2] << 16) | (base[1] << 8) | base[0];
	else
		return (base[0] << 24) | (base[1] << 16) | (base[2] << 8) | base[3];
}

static int get_info_from_type_string(const char *typestring, UINT32 *trktype, UINT32 *datasize)
{
	if (!strcmp(typestring, "MODE1"))
	{
		*trktype = CD_TRACK_MODE1;
		*datasize = 2048;
	}
	else if (!strcmp(typestring, "MODE1/2048"))
	{
		*trktype = CD_TRACK_MODE1;
		*datasize = 2048;
	}
	else if (!strcmp(typestring, "MODE1_RAW"))
	{
		*trktype = CD_TRACK_MODE1_RAW;
		*datasize = 2352;
	}
	else if (!strcmp(typestring, "MODE1/2352"))
	{
		*trktype = CD_TRACK_MODE1_RAW;
		*datasize = 2352;
	}
	else if (!strcmp(typestring, "MODE2"))
	{
		*trktype = CD_TRACK_MODE2;
		*datasize = 2336;
	}
	else if (!strcmp(typestring, "MODE2/2336"))
	{
		*trktype = CD_TRACK_MODE2;
		*datasize = 2336;
	}
	else if (!strcmp(typestring, "MODE2_FORM1"))
	{
		*trktype = CD_TRACK_MODE2_FORM1;
		*datasize = 2048;
	}
	else if (!strcmp(typestring, "MODE2/2048"))
	{
		*trktype = CD_TRACK_MODE2_FORM1;
		*datasize = 2048;
	}
	else if (!strcmp(typestring, "MODE2_FORM2"))
	{
		*trktype = CD_TRACK_MODE2_FORM2;
		*datasize = 2324;
	}
	else if (!strcmp(typestring, "MODE2/2324"))
	{
		*trktype = CD_TRACK_MODE2_FORM2;
		*datasize = 2324;
	}
	else if (!strcmp(typestring, "MODE2_FORM_MIX"))
	{
		*trktype = CD_TRACK_MODE2_FORM_MIX;
		*datasize = 2336;
	}
	else if (!strcmp(typestring, "MODE2_RAW"))
	{
		*trktype = CD_TRACK_MODE2_RAW;
		*datasize = 2352;
	}
	else if (!strcmp(typestring, "MODE2/2352"))
	{
		*trktype = CD_TRACK_MODE2_RAW;
		*datasize = 2352;
	}
	else if (!strcmp(typestring, "AUDIO"))
	{
		*trktype = CD_TRACK_AUDIO;
		*datasize = 2352;
	}
	else
	{
		return 0;
	}

	return 1;
}

static int get_info_from_subtype_string(const char *subtypestring, UINT32 *subtype, UINT32 *subsize)
{
	if (!strcmp(subtypestring, "NONE"))
	{
		*subtype = CD_SUB_NONE;
		*subsize = 0;
	}
	else if (!strcmp(subtypestring, "RW"))
	{
		*subtype = CD_SUB_NORMAL;
		*subsize = 96;
	}
	else if (!strcmp(subtypestring, "RW_RAW"))
	{
		*subtype = CD_SUB_RAW;
		*subsize = 96;
	}
	else
	{
		return 0;
	}

	return 1;
}

static int set_track_pregap(cdrom_track_info *track, int pregap, int postgap, const char *pgtype, const char *pgsub)
{
	track->pregap = pregap;
	track->postgap = postgap;
	if (pregap > 0)
	{
		const char *pgtype_str = (pgtype[0] == 'V') ? &pgtype[1] : pgtype;
		if (!get_info_from_type_string(pgtype_str, &track->pgtype, &track->pgdatasize) ||
		    !get_info_from_subtype_string(pgsub, &track->pgsub, &track->pgsubsize))
			return 0;
		if (pgtype[0] != 'V')
			track->pgdatasize = 0;
	}
	return 1;
}

static chd_error cdrom_parse_metadata(chd_file *chd, cdrom_toc *toc)
{
	char metadata[256];
	UINT32 metatag;
	UINT32 count;
	UINT8 metaflags;
	int i;
	chd_error err;

	memset(toc, 0, sizeof(*toc));

	/* start with the new-style textual metadata */
	for (i = 0; i < CD_MAX_TRACKS; i++)
	{
		cdrom_track_info *track = &toc->tracks[i];
		int trknum = -1, frames = 0, pregap = 0, postgap = 0, padframes = 0;
		char type[64], subtype[32], pgtype[32], pgsub[32];

		err = chd_get_metadata(chd, CDROM_TRACK_METADATA_TAG, i, metadata, sizeof(metadata) - 1, &count, &metatag, &metaflags);
		if (err == CHDERR_NONE)
		{
			metadata[(count < sizeof(metadata)) ? count : (sizeof(metadata) - 1)] = 0;
			if (sscanf(metadata, "TRACK:%d TYPE:%63s SUBTYPE:%31s FRAMES:%d", &trknum, type, subtype, &frames) != 4 ||
			    trknum == 0 || trknum > CD_MAX_TRACKS || frames < 0)
				return CHDERR_INVALID_DATA;
		}
		else
		{
			err = chd_get_metadata(chd, CDROM_TRACK_METADATA2_TAG, i, metadata, sizeof(metadata) - 1, &count, &metatag, &metaflags);
			if (err == CHDERR_NONE)
			{
				metadata[(count < sizeof(metadata)) ? count : (sizeof(metadata) - 1)] = 0;
				if (sscanf(metadata, "TRACK:%d TYPE:%63s SUBTYPE:%31s FRAMES:%d PREGAP:%d PGTYPE:%31s PGSUB:%31s POSTGAP:%d", &trknum, type, subtype, &frames, &pregap, pgtype, pgsub, &postgap) != 8 ||
				    trknum == 0 || trknum > CD_MAX_TRACKS || frames < 0 || pregap < 0 || postgap < 0)
					return CHDERR_INVALID_DATA;
				if (!set_track_pregap(track, pregap, postgap, pgtype, pgsub))
					return CHDERR_INVALID_DATA;
			}
			else
			{
				err = chd_get_metadata(chd, GDROM_OLD_METADATA_TAG, i, metadata, sizeof(metadata) - 1, &count, &metatag, &metaflags);
				if (err != CHDERR_NONE)
					err = chd_get_metadata(chd, GDROM_TRACK_METADATA_TAG, i, metadata, sizeof(metadata) - 1, &count, &metatag, &metaflags);
				if (err == CHDERR_NONE)
				{
					metadata[(count < sizeof(metadata)) ? count : (sizeof(metadata) - 1)] = 0;
					if (sscanf(metadata, "TRACK:%d TYPE:%63s SUBTYPE:%31s FRAMES:%d PAD:%d PREGAP:%d PGTYPE:%31s PGSUB:%31s POSTGAP:%d", &trknum, type, subtype, &frames, &padframes, &pregap, pgtype, pgsub, &postgap) != 9 ||
					    trknum == 0 || trknum > CD_MAX_TRACKS || frames < 0 || padframes < 0 || pregap < 0 || postgap < 0)
						return CHDERR_INVALID_DATA;
					toc->flags |= CD_FLAG_GDROM;
					if (metatag == GDROM_OLD_METADATA_TAG)
						toc->flags |= CD_FLAG_GDROMLE;
					if (!set_track_pregap(track, pregap, postgap, pgtype, pgsub))
						return CHDERR_INVALID_DATA;
					track->padframes = padframes;
				}
				else
				{
					break;
				}
			}
		}

		track->frames = frames;
		track->extraframes = ((frames + CD_TRACK_PADDING - 1) / CD_TRACK_PADDING) * CD_TRACK_PADDING - frames;
		if (!get_info_from_type_string(type, &track->trktype, &track->datasize) ||
		    !get_info_from_subtype_string(subtype, &track->subtype, &track->subsize))
			return CHDERR_INVALID_DATA;
		toc->numtrks++;
	}

	if (toc->numtrks > 0)
		return CHDERR_NONE;

	/* fallback to legacy binary metadata */
	{
		UINT32 oldmeta[CD_METADATA_WORDS];
		UINT32 *mrp;

		err = chd_get_metadata(chd, CDROM_OLD_METADATA_TAG, 0, oldmeta, sizeof(oldmeta), &count, &metatag, &metaflags);
		if (err != CHDERR_NONE)
			return err;

		mrp = &oldmeta[0];
		endian_mode = 0;
		toc->numtrks = get_bigendian_uint32((UINT8 *)mrp);
		if (toc->numtrks > CD_MAX_TRACKS)
		{
			endian_mode = 1;
			toc->numtrks = get_bigendian_uint32((UINT8 *)mrp);
		}
		if (toc->numtrks == 0 || toc->numtrks > CD_MAX_TRACKS)
			return CHDERR_INVALID_DATA;

		mrp++;
		for (i = 0; i < CD_MAX_TRACKS; i++)
		{
			toc->tracks[i].trktype = get_bigendian_uint32((UINT8 *)mrp);
			mrp++;
			toc->tracks[i].subtype = get_bigendian_uint32((UINT8 *)mrp);
			mrp++;
			toc->tracks[i].datasize = get_bigendian_uint32((UINT8 *)mrp);
			mrp++;
			toc->tracks[i].subsize = get_bigendian_uint32((UINT8 *)mrp);
			mrp++;
			toc->tracks[i].frames = get_bigendian_uint32((UINT8 *)mrp);
			mrp++;
			toc->tracks[i].extraframes = get_bigendian_uint32((UINT8 *)mrp);
			mrp++;
		}
	}

	return CHDERR_NONE;
}

/*************************************
 *
 *  Open a CD-ROM
 *
 *************************************/

cdrom_file *cdrom_open(chd_file *chd)
{
	int i;
	cdrom_file *file;
	const chd_header *header;
	UINT32 physofs, chdofs;
	chd_error err;

	/* punt if no CHD */
	if (!chd)
		return NULL;

	/* validate the CHDunk size */
	header = chd_get_header(chd);
	if (!header || header->hunkbytes == 0 || (header->hunkbytes % CD_FRAME_SIZE) != 0)
		return NULL;

	/* allocate memory for the CD-ROM file */
	file = malloc(sizeof(cdrom_file));
	if (!file)
		return NULL;

	/* initialize the audio info */
	file->audio_playing = 0;
	file->audio_pause = 0;
	file->audio_ended_normally = 0;
	file->audio_lba = 0;
	file->audio_length = 0;
	file->audio_samples = 0;

	/* read the CD-ROM metadata */
	err = cdrom_parse_metadata(chd, &file->cdtoc);
	if (err != CHDERR_NONE)
	{
		free(file);
		return NULL;
	}

	#if VERBOSE
	logerror("CD has %d tracks\n", file->cdtoc.numtrks);
	#endif

	/* calculate the starting frame for each track, keeping in mind that CHDMAN
       pads tracks out with extra frames to fit hunk size boundries
    */
	physofs = chdofs = 0;
	for (i = 0; i < file->cdtoc.numtrks; i++)
	{
		file->cdtoc.tracks[i].physframeofs = physofs;
		file->cdtoc.tracks[i].chdframeofs = chdofs;

		physofs += file->cdtoc.tracks[i].frames;

		chdofs  += file->cdtoc.tracks[i].frames;
		chdofs  += file->cdtoc.tracks[i].extraframes;

		#if VERBOSE
		logerror("Track %02d is format %d subtype %d datasize %d subsize %d frames %d extraframes %d physofs %d chdofs %d\n", i+1,
			file->cdtoc.tracks[i].trktype,
			file->cdtoc.tracks[i].subtype,
			file->cdtoc.tracks[i].datasize,
			file->cdtoc.tracks[i].subsize,
			file->cdtoc.tracks[i].frames,
			file->cdtoc.tracks[i].extraframes,
			file->cdtoc.tracks[i].physframeofs,
			file->cdtoc.tracks[i].chdframeofs);
		#endif
	}

	/* fill out dummy entries for the last track to help our search */
	file->cdtoc.tracks[i].physframeofs = physofs;
	file->cdtoc.tracks[i].chdframeofs = chdofs;

	/* fill in the data */
	file->chd = chd;
	file->hunksectors = header->hunkbytes / CD_FRAME_SIZE;
	file->cachehunk = -1;

	/* allocate a cache */
	file->cache = malloc(header->hunkbytes);
	if (!file->cache)
	{
		free(file);
		return NULL;
	}

	/* allocate an audio cache */
	file->audio_cache = malloc(CD_MAX_SECTOR_DATA*4);
	if (!file->audio_cache)
	{
		free(file->cache);
		free(file);
		return NULL;
	}

	return file;
}



/*************************************
 *
 *  Close a CD-ROM
 *
 *************************************/

void cdrom_close(cdrom_file *file)
{
	/* free the cache */
	if (file->cache)
		free(file->cache);
	if (file->audio_cache)
		free(file->audio_cache);
	free(file);
}



/*************************************
 *
 *  Return the handle to the CHD
 *
 *************************************/

chd_file *cdrom_get_chd(cdrom_file *file)
{
	return file->chd;
}


/*************************************
 *
 *  Read a data sector from a CD-ROM
 *
 *************************************/

UINT32 cdrom_read_data(cdrom_file *file, UINT32 lbasector, UINT32 numsectors, void *buffer, UINT32 datatype)
{
	UINT32 hunknum = lbasector / file->hunksectors;
	UINT32 sectoroffs = lbasector % file->hunksectors;
	UINT32 track = cdrom_get_track_chd(file, lbasector);
	UINT32 tracktype;

	tracktype = file->cdtoc.tracks[track].trktype;

	/* for now, just break down multisector reads into single sectors */
	if (numsectors > 1)
	{
		UINT32 total = 0;
		while (numsectors--)
		{
			if (cdrom_read_data(file, lbasector++, 1, (UINT8 *)buffer + (total * file->cdtoc.tracks[track].datasize), datatype))
				total++;
			else
				break;
		}
		return total;
	}

	/* if we haven't cached this hunk, read it now */
	if (file->cachehunk != hunknum)
	{
		if (chd_read(file->chd, hunknum, file->cache) != CHDERR_NONE)
			return 0;
		file->cachehunk = hunknum;
	}

	/* copy out the requested sector */
	if ((datatype == tracktype) || (datatype == CD_TRACK_RAW_DONTCARE))
	{
		memcpy(buffer, &file->cache[sectoroffs * CD_FRAME_SIZE], file->cdtoc.tracks[track].datasize);
	}
	else
	{
		/* return 2048 bytes of mode1 data from a 2352 byte mode1 raw sector */
		if ((datatype == CD_TRACK_MODE1) && (tracktype == CD_TRACK_MODE1_RAW))
		{
			memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + 16], 2048);
			return 1;
		}

		/* return 2048 bytes of mode1 data from a 2048 byte mode2 form1 sector */
		if ((datatype == CD_TRACK_MODE1) && (tracktype == CD_TRACK_MODE2_FORM1))
		{
			memcpy(buffer, &file->cache[sectoroffs * CD_FRAME_SIZE], 2048);
			return 1;
		}

		/* return 2048 bytes of mode1 data from a 2352 byte mode2 raw sector */
		if ((datatype == CD_TRACK_MODE1) && (tracktype == CD_TRACK_MODE2_RAW))
		{
			memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + 24], 2048);
			return 1;
		}

		/* return 2048 bytes of mode1 data from a 2336 byte mode2 form mix sector */
		if ((datatype == CD_TRACK_MODE1) && (tracktype == CD_TRACK_MODE2_FORM_MIX))
		{
			memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + 8], 2048);
			return 1;
		}

		/* return mode 2 2336 byte data from a 2352 byte mode 1 or 2 raw sector (skip the header) */
		if ((datatype == CD_TRACK_MODE2) && ((tracktype == CD_TRACK_MODE1_RAW) || (tracktype == CD_TRACK_MODE2_RAW)))
		{
			memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + 16], 2336);
			return 1;
		}

		#if VERBOSE
		logerror("CDROM: Conversion from type %d to type %d not supported!\n", tracktype, datatype);
		#endif
		return 0;
	}
	return 1;
}

/*************************************
 *
 *  Read subcode data from a CD-ROM
 *
 *************************************/

UINT32 cdrom_read_subcode(cdrom_file *file, UINT32 lbasector, void *buffer)
{
	UINT32 hunknum = lbasector / file->hunksectors;
	UINT32 sectoroffs = lbasector % file->hunksectors;
	UINT32 track = cdrom_get_track_chd(file, lbasector);

	/* if we haven't cached this hunk, read it now */
	if (file->cachehunk != hunknum)
	{
		if (chd_read(file->chd, hunknum, file->cache) != CHDERR_NONE)
			return 0;
		file->cachehunk = hunknum;
	}

	/* copy out the requested data */
	memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + file->cdtoc.tracks[track].datasize], file->cdtoc.tracks[track].subsize);
	return 1;
}

/*************************************
 *
 *  Red Book audio track utilities
 *
 *************************************/

/*
 * cdrom_start_audio: begin playback of a Red Book audio track
 */

void cdrom_start_audio(cdrom_file *file, UINT32 start_chd_lba, UINT32 blocks)
{
	file->audio_playing = 1;
	file->audio_pause = 0;
	file->audio_ended_normally = 0;
	file->audio_lba = start_chd_lba;
	file->audio_length = blocks;
}

/*
 * cdrom_stop_audio: stop playback of a Red Book audio track
 */

void cdrom_stop_audio(cdrom_file *file)
{
	file->audio_playing = 0;
}

/*
 * cdrom_pause_audio: pause/unpause playback of a Red Book audio track
 */

void cdrom_pause_audio(cdrom_file *file, int pause)
{
	file->audio_pause = pause;
}

/*
 * cdrom_audio_active: returns Red Book audio playback status
 */

int cdrom_audio_active(cdrom_file *file)
{
	return (file->audio_playing);
}

/*
 * cdrom_get_audio_lba: returns the current LBA (physical sector) during Red Book playback
 */

UINT32 cdrom_get_audio_lba(cdrom_file *file)
{
	return file->audio_lba;
}

/*
 * cdrom_audio_paused: returns if Red Book playback is paused
 */

int cdrom_audio_paused(cdrom_file *file)
{
	return (file->audio_pause);
}

/*
 * cdrom_audio_ended: returns if a Red Book track reached it's natural end
 */

int cdrom_audio_ended(cdrom_file *file)
{
	return (file->audio_ended_normally);
}

/*
 * cdrom_get_audio_data: reads Red Book data off the disc if playback is in progress and
 *                       converts it to 2 16-bit 44.1 kHz streams.
 */

void cdrom_get_audio_data(cdrom_file *file, stream_sample_t *bufL, stream_sample_t *bufR, UINT32 samples_wanted)
{
	int i, sectoread, remaining;

	/* if no file, audio not playing, audio paused, or out of disc data,
       just zero fill */
	if (!file || !file->audio_playing || file->audio_pause || (!file->audio_length && !file->audio_samples))
	{
		if (file && file->audio_playing && !file->audio_pause && !file->audio_length)
		{
			file->audio_playing = 0;
			file->audio_ended_normally = 1;
		}
		memset(bufL, 0, sizeof(stream_sample_t)*samples_wanted);
		memset(bufR, 0, sizeof(stream_sample_t)*samples_wanted);
		return;
	}

	/* if we've got enough samples, just feed 'em out */
	if (samples_wanted <= file->audio_samples)
	{
		for (i = 0; i < samples_wanted; i++)
		{
			*bufL++ = *file->audio_bptr++;
			*bufR++ = *file->audio_bptr++;
		}

		file->audio_samples -= samples_wanted;
		return;
	}

	/* we don't have enough, so first feed what we've got */
	for (i = 0; i < file->audio_samples; i++)
	{
		*bufL++ = *file->audio_bptr++;
		*bufR++ = *file->audio_bptr++;
	}

	/* remember how much left for later */
	remaining = samples_wanted - file->audio_samples;

	/* reset the buffer and get what we can from the disc */
	file->audio_samples = 0;
	if (file->audio_length >= 4)
	{
		sectoread = 4;
	}
	else
	{
		sectoread = file->audio_length;
	}

	for (i = 0; i < sectoread; i++)
	{
		cdrom_read_data(file, file->audio_lba, 1, &file->audio_cache[CD_MAX_SECTOR_DATA*i], CD_TRACK_AUDIO);

		file->audio_lba++;
	}

	file->audio_samples = (CD_MAX_SECTOR_DATA*sectoread)/4;
	file->audio_length -= sectoread;

	/* CD-DA data on the disc is big-endian unless GDROMLE flag is set, flip if needed */
	#ifdef LSB_FIRST
	if (!(file->cdtoc.flags & CD_FLAG_GDROMLE))
	#else
	if (file->cdtoc.flags & CD_FLAG_GDROMLE)
	#endif
	{
		for (i = 0; i < file->audio_samples*4; i += 2)
		{
			UINT8 tmp;

			tmp = file->audio_cache[i+1];
			file->audio_cache[i+1] = file->audio_cache[i];
			file->audio_cache[i] = tmp;
		}
	}

	/* reset feedout ptr */
	file->audio_bptr = (INT16 *)file->audio_cache;

	/* we've got data, feed it out by calling ourselves recursively */
	cdrom_get_audio_data(file, bufL, bufR, remaining);
}

// returns the last track number
int cdrom_get_last_track(cdrom_file *file)
{
	return file->cdtoc.numtrks;
}

// get the ADR | CONTROL for a track
int cdrom_get_adr_control(cdrom_file *file, int track)
{
	if (track == 0xaa)
	{
		return 0x10;	// audio track, subchannel is position
	}

	if (file->cdtoc.tracks[track].trktype == CD_TRACK_AUDIO)
	{
	 	return 0x10;	// audio track, subchannel is position
	}

	return 0x14;	// data track, subchannel is position
}

// is a track audio?
int cdrom_get_track_type(cdrom_file *file, int track)
{
	if (file->cdtoc.tracks[track].trktype == CD_TRACK_AUDIO)
	{
		return 1;
	}

	return 0;
}

UINT32 cdrom_get_track_length(cdrom_file *file, int track)
{
	return (file->cdtoc.tracks[track].frames * file->cdtoc.tracks[track].datasize);
}

INLINE UINT8 make_bcd(UINT8 data)
{
	return ((data / 10) << 4) | (data % 10);
}

// get the start of a track
// *file = cdrom
// track = track #
// msf = 0 for LBA, 1 for BCD M:S:F
UINT32 cdrom_get_track_start(cdrom_file *file, int track, int msf)
{
	int tstart = cdrom_get_chd_start_of_track(file, track);

	if (msf)
	{
		UINT8 m, s, f;

		m = tstart / (60*75);
		tstart -= (m * 60 * 75);
		s = tstart / 75;
		f = tstart % 75;
		#if VERBOSE
		logerror("CDROM: %d blocks => %d M %d S %d F\n",  cdrom_get_chd_start_of_track(file, track), m, s, f);
		#endif

		tstart = make_bcd(m)<<16 | make_bcd(s)<<8 | make_bcd(f);

		#if VERBOSE
		logerror("CDROM: %08x in BCD\n", tstart);
		#endif

		return tstart;
	}
	else
	{
		return tstart;
	}
}

cdrom_toc *cdrom_get_toc(cdrom_file *file)
{
	return &file->cdtoc;
}

