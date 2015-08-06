/***************************************************************************

  Generic MAME CD-ROM utilties - build IDE and SCSI CD-ROMs on top of this

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

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

/*************************************
 *
 *  Open a CD-ROM
 *
 *************************************/

cdrom_file *cdrom_open(chd_file *chd)
{
	int i;
	cdrom_file *file;
	UINT32 metatag;
	UINT32 count, physofs, chdofs;
	static UINT32 metadata[CD_METADATA_WORDS], *mrp;

	/* punt if no CHD */
	if (!chd)
		return NULL;

	/* allocate memory for the CD-ROM file */
	file = malloc(sizeof(cdrom_file));
	if (!file)
		return NULL;

	/* initialize the audio info */
	file->audio_playing = 0;
	file->audio_pause = 0;
	file->audio_length = 0;
	file->audio_samples = 0;

	/* read the CD-ROM metadata */
	metatag = CDROM_STANDARD_METADATA;
	count = chd_get_metadata(chd, &metatag, 0, metadata, sizeof(metadata));
	if (count == 0)
		return NULL;

	/* reconstruct the TOC from it */
	/* TODO: I don't know why sometimes the data is one endian and sometimes another */
	mrp = &metadata[0];
	endian_mode = 0;
	file->cdtoc.numtrks = get_bigendian_uint32((UINT8 *)mrp);

	if ((file->cdtoc.numtrks < 0) || (file->cdtoc.numtrks > CD_MAX_TRACKS))
	{
		endian_mode = 1;
		file->cdtoc.numtrks = get_bigendian_uint32((UINT8 *)mrp);
	}

	mrp++;
	for (i = 0; i < CD_MAX_TRACKS; i++)
	{
		file->cdtoc.tracks[i].trktype = get_bigendian_uint32((UINT8 *)mrp);
                mrp++;
		file->cdtoc.tracks[i].subtype = get_bigendian_uint32((UINT8 *)mrp);
                mrp++;
		file->cdtoc.tracks[i].datasize = get_bigendian_uint32((UINT8 *)mrp);
                mrp++;
		file->cdtoc.tracks[i].subsize = get_bigendian_uint32((UINT8 *)mrp);
                mrp++;
		file->cdtoc.tracks[i].frames = get_bigendian_uint32((UINT8 *)mrp);
                mrp++;
		file->cdtoc.tracks[i].extraframes = get_bigendian_uint32((UINT8 *)mrp);
		mrp++;
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
	file->hunksectors = CD_FRAMES_PER_HUNK;
	file->cachehunk = -1;

	/* allocate a cache */
	file->cache = malloc(chd_get_header(chd)->hunkbytes);
	if (!file->cache)
	{
		free(file);
		return NULL;
	}

	/* allocate an audio cache */
	file->audio_cache = malloc(CD_MAX_SECTOR_DATA*4);
	if (!file->audio_cache)
	{
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
		if (!chd_read(file->chd, hunknum, 1, file->cache))
			return 0;
		file->cachehunk = hunknum;
	}

	/* copy out the requested sector */
	if (datatype == tracktype)
	{
		memcpy(buffer, &file->cache[sectoroffs * CD_FRAME_SIZE], file->cdtoc.tracks[track].datasize);
	}
	else
	{
		/* return 2048 bytes of mode1 data from a 2336 byte mode1 raw sector */
		if ((datatype == CD_TRACK_MODE1) && (tracktype == CD_TRACK_MODE1_RAW))
		{
			memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + 16], 2048);
			return 1;
		}

		/* return 2048 bytes of mode1 data from a 2352 byte mode2 form 1 raw sector */
		if ((datatype == CD_TRACK_MODE1) && (tracktype == CD_TRACK_MODE2_FORM1))
		{
			memcpy(buffer, &file->cache[(sectoroffs * CD_FRAME_SIZE) + 24], 2048);
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
	UINT32 tracktype;

	tracktype = file->cdtoc.tracks[track].trktype;

	/* if we haven't cached this hunk, read it now */
	if (file->cachehunk != hunknum)
	{
		if (!chd_read(file->chd, hunknum, 1, file->cache))
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

	/* CD-DA data on the disc is big-endian, flip if we're not */
	#ifdef LSB_FIRST
	for (i = 0; i < file->audio_samples*4; i += 2)
	{
		UINT8 tmp;

		tmp = file->audio_cache[i+1];
		file->audio_cache[i+1] = file->audio_cache[i];
		file->audio_cache[i] = tmp;
	}
	#endif

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

