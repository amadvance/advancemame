/***************************************************************************

    CHD compression frontend

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osd_tool.h"
#include "chdcd.h"
#include "md5.h"
#include "sha1.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>


/***************************************************************************
    CONSTANTS & DEFINES
***************************************************************************/

#define IDE_SECTOR_SIZE			512

#define ENABLE_CUSTOM_CHOMP		0



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static chd_interface_file *chdman_open(const char *filename, const char *mode);
static void chdman_close(chd_interface_file *file);
static UINT32 chdman_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 chdman_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 chdman_length(chd_interface_file *file);



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static chd_interface chdman_interface =
{
	chdman_open,
	chdman_close,
	chdman_read,
	chdman_write,
	chdman_length
};

static chd_file *special_chd;
static UINT64 special_logicalbytes;
static UINT64 special_original_logicalbytes;
static UINT64 special_bytes_checksummed;
static UINT32 special_error_count;
static struct MD5Context special_md5;
static struct sha1_ctx special_sha1;

#define SPECIAL_CHD_NAME "??SPECIALCHD??"

static const char *error_strings[] =
{
	"no error",
	"no drive interface",
	"out of memory",
	"invalid file",
	"invalid parameter",
	"invalid data",
	"file not found",
	"requires parent",
	"file not writeable",
	"read error",
	"write error",
	"codec error",
	"invalid parent",
	"hunk out of range",
	"decompression error",
	"compression error",
	"can't create file",
	"can't verify file"
	"operation not supported",
	"can't find metadata",
	"invalid metadata size",
	"unsupported CHD version"
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    put_bigendian_uint32 - write a UINT32 in big-endian order to memory
-------------------------------------------------*/

INLINE void put_bigendian_uint32(UINT8 *base, UINT32 value)
{
	base[0] = value >> 24;
	base[1] = value >> 16;
	base[2] = value >> 8;
	base[3] = value;
}


/*-------------------------------------------------
    print_big_int - 64-bit int printing with commas
-------------------------------------------------*/

void print_big_int(UINT64 intvalue, char *output)
{
	int chunk;

	chunk = intvalue % 1000;
	intvalue /= 1000;
	if (intvalue)
	{
		print_big_int(intvalue, output);
		strcat(output, ",");
		sprintf(&output[strlen(output)], "%03d", chunk);
	}
	else
		sprintf(&output[strlen(output)], "%d", chunk);
}


/*-------------------------------------------------
    big_int_string - return a string for a big int
-------------------------------------------------*/

char *big_int_string(UINT64 intvalue)
{
	static char buffer[256];
	buffer[0] = 0;
	print_big_int(intvalue, buffer);
	return buffer;
}


/*-------------------------------------------------
    progress - generic progress callback
-------------------------------------------------*/

static void progress(const char *fmt, ...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, fmt);
	vprintf(fmt, arg);
	fflush(stdout);
	va_end(arg);
}


/*-------------------------------------------------
    error_string - return an error sting
-------------------------------------------------*/

static const char *error_string(int err)
{
	static char temp_buffer[100];

	if (err < sizeof(error_strings) / sizeof(error_strings[0]))
		return error_strings[err];

	sprintf(temp_buffer, "unknown error %d", err);
	return temp_buffer;
}


/*-------------------------------------------------
    error - generic usage error display
-------------------------------------------------*/

static void error(void)
{
	printf("usage: chdman -info input.chd\n");
	printf("   or: chdman -createhd inputhd.raw output.chd [inputoffs [cylinders heads sectors [sectorsize [hunksize]]]]\n");
	printf("   or: chdman -createblankhd output.chd cylinders heads sectors [sectorsize [hunksize]]\n");
	printf("   or: chdman -createcd input.toc output.chd\n");
	printf("   or: chdman -copydata input.chd output.chd\n");
	printf("   or: chdman -extract input.chd output.raw\n");
	printf("   or: chdman -extractcd input.chd output.toc output.bin\n");
	printf("   or: chdman -verify input.chd\n");
	printf("   or: chdman -verifyfix input.chd\n");
	printf("   or: chdman -update input.chd output.chd\n");
	printf("   or: chdman -chomp inout.chd output.chd maxhunk\n");
	printf("   or: chdman -merge parent.chd diff.chd output.chd\n");
	printf("   or: chdman -diff parent.chd compare.chd diff.chd\n");
	printf("   or: chdman -setchs inout.chd cylinders heads sectors\n");
	exit(1);
}


/*-------------------------------------------------
    fatalerror - error hook for assertions
-------------------------------------------------*/

void CLIB_DECL fatalerror(const char *text,...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);
	vfprintf(stderr, text, arg);
	va_end(arg);

	exit(-1);
}


/*-------------------------------------------------
    special_chd_init - set up a CHD file as a
    "fake" raw file for input
-------------------------------------------------*/

static void special_chd_init(chd_file *chd, UINT64 logicalbytes)
{
	/* set the input chd and logical bytes */
	special_chd = chd;
	special_original_logicalbytes = chd_get_header(chd)->logicalbytes;
	special_logicalbytes = logicalbytes ? logicalbytes : special_original_logicalbytes;

	/* init the checksums */
	special_error_count = 0;
	special_bytes_checksummed = 0;
	MD5Init(&special_md5);
	sha1_init(&special_sha1);
}


/*-------------------------------------------------
    special_chd_finished - finish using a CHD
    file as a "fake" raw file for input
-------------------------------------------------*/

static void special_chd_finished(void)
{
	static const UINT8 empty_checksum[CHD_SHA1_BYTES] = { 0 };
	const chd_header *header = chd_get_header(special_chd);
	UINT8 final_sha1[CHD_SHA1_BYTES];
	UINT8 final_md5[CHD_MD5_BYTES];

	/* finish the checksums */
	MD5Final(final_md5, &special_md5);
	sha1_final(&special_sha1);
	sha1_digest(&special_sha1, SHA1_DIGEST_SIZE, final_sha1);

	/* we can only compare if we've checksummed all the logical bytes */
	if (special_bytes_checksummed == special_original_logicalbytes)
	{
		/* check the MD5 */
		if (memcmp(header->md5, empty_checksum, CHD_MD5_BYTES))
		{
			if (memcmp(header->md5, final_md5, CHD_MD5_BYTES))
			{
				printf("WARNING: expected input MD5 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
						header->md5[0], header->md5[1], header->md5[2], header->md5[3],
						header->md5[4], header->md5[5], header->md5[6], header->md5[7],
						header->md5[8], header->md5[9], header->md5[10], header->md5[11],
						header->md5[12], header->md5[13], header->md5[14], header->md5[15]);
				printf("                 actual MD5 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
						final_md5[0], final_md5[1], final_md5[2], final_md5[3],
						final_md5[4], final_md5[5], final_md5[6], final_md5[7],
						final_md5[8], final_md5[9], final_md5[10], final_md5[11],
						final_md5[12], final_md5[13], final_md5[14], final_md5[15]);
			}
			else
				printf("Input MD5 verified\n");
		}

		/* check the SHA1 */
		if (memcmp(header->sha1, empty_checksum, CHD_SHA1_BYTES))
		{
			if (memcmp(header->sha1, final_sha1, CHD_SHA1_BYTES))
			{
				printf("WARNING: expected input SHA1 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
						header->sha1[0], header->sha1[1], header->sha1[2], header->sha1[3],
						header->sha1[4], header->sha1[5], header->sha1[6], header->sha1[7],
						header->sha1[8], header->sha1[9], header->sha1[10], header->sha1[11],
						header->sha1[12], header->sha1[13], header->sha1[14], header->sha1[15],
						header->sha1[16], header->sha1[17], header->sha1[18], header->sha1[19]);
				printf("                 actual SHA1 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
						final_sha1[0], final_sha1[1], final_sha1[2], final_sha1[3],
						final_sha1[4], final_sha1[5], final_sha1[6], final_sha1[7],
						final_sha1[8], final_sha1[9], final_sha1[10], final_sha1[11],
						final_sha1[12], final_sha1[13], final_sha1[14], final_sha1[15],
						final_sha1[16], final_sha1[17], final_sha1[18], final_sha1[19]);
			}
			else
				printf("Input SHA1 verified\n");
		}
	}
	else if (special_error_count)
		printf("WARNING: found %d errors in the input file, checksums not verified\n", special_error_count);
	else
		printf("WARNING: entire input file not read, checksums not verified\n");

	/* reset everything */
	special_chd = NULL;
}



/*-------------------------------------------------
    guess_chs - given a file and an offset,
    compute a best guess CHS value set
-------------------------------------------------*/

static void guess_chs(const char *filename, int offset, int sectorsize, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps)
{
	/* if this is a direct physical drive read, handle it specially */
	if (!osd_get_physical_drive_geometry(filename, cylinders, heads, sectors, bps))
		  	{
		UINT64 filesize = osd_get_file_size(filename) - offset;
		UINT32 totalsecs, hds, secs;

		/* validate the file */
		if (filesize == 0)
		{
			fprintf(stderr, "Invalid file '%s'\n", filename);
			exit(1);
		}

		/* validate the size */
		if (filesize % sectorsize != 0)
		{
			fprintf(stderr, "Can't guess CHS values because data size is not divisible by the sector size\n");
			exit(1);
		}
		totalsecs = filesize / sectorsize;

		/* now find a valid value */
		for (secs = 63; secs > 1; secs--)
			if (totalsecs % secs == 0)
			{
				size_t totalhds = totalsecs / secs;
				for (hds = 16; hds > 1; hds--)
					if (totalhds % hds == 0)
					{
						*cylinders = totalhds / hds;
						*heads = hds;
						*sectors = secs;
						*bps = IDE_SECTOR_SIZE;
						return;
					}
			}

		/* ack, it didn't work! */
		fprintf(stderr, "Can't guess CHS values because no logical combination works!\n");
		exit(1);
	}
}



/*-------------------------------------------------
    do_create - create a new compressed hard disk
    image from a raw file
-------------------------------------------------*/

static void do_createhd(int argc, char *argv[])
{
	const char *inputfile, *outputfile;
	UINT32 sectorsize, hunksize;
	UINT32 cylinders, heads, sectors, totalsectors;
	chd_file *chd;
	char metadata[256];
	int offset, err;

	/* require 4-5, or 8-10 args total */
	if (argc != 4 && argc != 5 && argc != 8 && argc != 9 && argc != 10)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];
	if (argc >= 5)
		offset = atoi(argv[4]);
	else
		offset = osd_get_file_size(inputfile) % 512;
	if (argc >= 8)
	{
		cylinders = atoi(argv[5]);
		heads = atoi(argv[6]);
		sectors = atoi(argv[7]);
		if (argc >= 9)
			sectorsize = atoi(argv[8]);
		else
			sectorsize = IDE_SECTOR_SIZE;
		if (argc >= 10)
			hunksize = atoi(argv[9]);
		else
			hunksize = (sectorsize > 4096) ? sectorsize : ((4096 / sectorsize) * sectorsize);
	}
	else
	{
		sectorsize = IDE_SECTOR_SIZE;
		guess_chs(inputfile, offset, sectorsize, &cylinders, &heads, &sectors, &sectorsize);
		hunksize = (sectorsize > 4096) ? sectorsize : ((4096 / sectorsize) * sectorsize);
	}

	totalsectors = cylinders * heads * sectors;

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);
	printf("Input offset: %d\n", offset);
	printf("Cylinders:    %d\n", cylinders);
	printf("Heads:        %d\n", heads);
	printf("Sectors:      %d\n", sectors);
	printf("Bytes/sector: %d\n", sectorsize);
	printf("Sectors/hunk: %d\n", hunksize / sectorsize);
	printf("Logical size: %s\n", big_int_string((UINT64)totalsectors * (UINT64)sectorsize));

	/* create the new hard drive */
	err = chd_create(outputfile, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		return;
	}

	/* open the new hard drive */
	chd = chd_open(outputfile, 1, NULL);
	if (!chd)
	{
		printf("Error opening new CHD file: %s\n", error_string(chd_get_last_error()));
		remove(outputfile);
		return;
	}

	/* write the metadata */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
	if (err != CHDERR_NONE)
	{
		printf("Error adding hard disk metadata: %s\n", error_string(chd_get_last_error()));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	/* compress the hard drive */
	err = chd_compress(chd, inputfile, offset, progress);
	if (err != CHDERR_NONE)
	{
		printf("Error during compression: %s\n", error_string(err));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	/* success */
	chd_close(chd);
}

/*-------------------------------------------------
    do_createcd - create a new compressed CD
    image from a raw file
-------------------------------------------------*/

static void do_createcd(int argc, char *argv[])
{
	char *inputfile, *outputfile;
	chd_file *chd;
	chd_exfile *chdex;
	int err;
	UINT32 totalsectors = 0;
	UINT32 sectorsize = CD_FRAME_SIZE;
	UINT32 hunksize = ((CD_FRAME_SIZE * CD_FRAMES_PER_HUNK) / sectorsize) * sectorsize;
	static cdrom_toc toc;
	static cdrom_track_input_info track_info;
	int i;
	static UINT32 metadata[CD_METADATA_WORDS], *mwp;

	/* require 4 args total */
	if (argc != 4)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* setup the CDROM module and get the disc info */
	err = cdrom_parse_toc(inputfile, &toc, &track_info);
	if (err != CHDERR_NONE)
	{
		printf("Error reading input file: %s\n", error_string(err));
		return;
	}

	/* count up the total number of frames */
	totalsectors = 0;
	for (i = 0; i < toc.numtrks; i++)
	{
		totalsectors += toc.tracks[i].frames;
	}
	printf("\nCD-ROM %s has %d tracks and %d total frames\n", inputfile, toc.numtrks, totalsectors);

	/* pad each track to a hunk boundry.  cdrom.c will deal with this on the read side */
	for (i = 0; i < toc.numtrks; i++)
	{
		int hunks = toc.tracks[i].frames / CD_FRAMES_PER_HUNK;

		if ((toc.tracks[i].frames % CD_FRAMES_PER_HUNK) != 0)
		{
			hunks++;
			toc.tracks[i].extraframes = (hunks * CD_FRAMES_PER_HUNK) - toc.tracks[i].frames;

			// adjust the total sector count as well
			totalsectors += toc.tracks[i].extraframes;
		}
		else
		{
			toc.tracks[i].extraframes = 0;
		}

		/*
        printf("Track %02d: file %s offset %d type %d subtype %d datasize %d subsize %d frames %d extra %d\n", i,
            track_info.fname[i],
            track_info.offset[i],
            toc.tracks[i].trktype,
            toc.tracks[i].subtype,
            toc.tracks[i].datasize,
            toc.tracks[i].subsize,
            toc.tracks[i].frames,
            toc.tracks[i].extraframes);
        */
	}

	/* create the new CHD file */
	err = chd_create(outputfile, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		return;
	}

	/* open the new CHD file */
	chd = chd_open(outputfile, 1, NULL);
	if (!chd)
	{
		printf("Error opening new CHD file: %s\n", error_string(chd_get_last_error()));
		remove(outputfile);
		return;
	}

	/* convert the metadata to a "portable" format */
	mwp = &metadata[0];
	put_bigendian_uint32((UINT8 *)mwp, toc.numtrks);
	mwp++;
	for (i = 0; i < CD_MAX_TRACKS; i++)
	{
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].trktype);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].subtype);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].datasize);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].subsize);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].frames);
		mwp++;
		put_bigendian_uint32((UINT8 *)mwp, toc.tracks[i].extraframes);
		mwp++;
	}

	/* write the metadata */
	err = chd_set_metadata(chd, CDROM_STANDARD_METADATA, 0, metadata, sizeof(metadata));
	if (err != CHDERR_NONE)
	{
		printf("Error adding CD-ROM metadata: %s\n", error_string(chd_get_last_error()));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	/* begin state for writing */
	chdex = chd_start_compress_ex(chd);

	/* write each track */
	for (i = 0; i < toc.numtrks; i++)
	{
		int trkbytespersec = toc.tracks[i].datasize + toc.tracks[i].subsize;
		int hunks = (toc.tracks[i].frames + toc.tracks[i].extraframes) / CD_FRAMES_PER_HUNK;

		printf("Compressing track %d / %d (file %s:%d, %d frames, %d hunks)\n", i+1, toc.numtrks, track_info.fname[i], track_info.offset[i], toc.tracks[i].frames, hunks);

 		err = chd_compress_ex(chdex, track_info.fname[i], track_info.offset[i],
				trkbytespersec, CD_FRAMES_PER_HUNK, hunks,
				CD_FRAME_SIZE, progress);
		if (err != CHDERR_NONE)
		{
			printf("Error during compression: %s\n", error_string(err));
			chd_close(chd);
			remove(outputfile);
			return;
		}
	}

	/* cleanup */
	err = chd_end_compress_ex(chdex, progress);
	if (err != CHDERR_NONE)
	{
		printf("Error during compression finalization: %s\n", error_string(err));
		chd_close(chd);
		remove(outputfile);
		return;
	}


	/* success */
	chd_close(chd);
}

/*
    Create a new non-compressed hard disk image, with all hunks filled with 0s.

    Example:
        [program] -createblankhd out.hd 615 4 32 256 32768
*/
static void do_createblankhd(int argc, char *argv[])
{
	const char *outputfile;
	UINT32 sectorsize, hunksize;
	UINT32 cylinders, heads, sectors, totalsectors;
	chd_file *chd;
	char metadata[256];
	int err;
	int hunknum;
	int totalhunks;
	UINT8 *cache;
	clock_t lastupdate;

	/* require 6, 7, or 8 args total */
	if (argc != 6 && argc != 7 && argc != 8)
		error();

	/* extract the data */
	outputfile = argv[2];
	cylinders = atoi(argv[3]);
	heads = atoi(argv[4]);
	sectors = atoi(argv[5]);
	if (argc >= 7)
		sectorsize = atoi(argv[6]);
	else
		sectorsize = IDE_SECTOR_SIZE;
	if (argc >= 8)
		hunksize = atoi(argv[7]);
	else
		hunksize = (sectorsize > 4096) ? sectorsize : ((4096 / sectorsize) * sectorsize);
	totalsectors = cylinders * heads * sectors;

	/* print some info */
	printf("Output file:  %s\n", outputfile);
	printf("Cylinders:    %d\n", cylinders);
	printf("Heads:        %d\n", heads);
	printf("Sectors:      %d\n", sectors);
	printf("Bytes/sector: %d\n", sectorsize);
	printf("Sectors/hunk: %d\n", hunksize / sectorsize);
	printf("Logical size: %s\n", big_int_string((UINT64)totalsectors * (UINT64)sectorsize));

	/* create the new hard drive */
	err = chd_create(outputfile, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_NONE, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		return;
	}

	/* open the new hard drive */
	chd = chd_open(outputfile, 1, NULL);
	if (!chd)
	{
		printf("Error opening new CHD file: %s\n", error_string(chd_get_last_error()));
		remove(outputfile);
		return;
	}

	/* write the metadata */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
	if (err != CHDERR_NONE)
	{
		printf("Error adding hard disk metadata: %s\n", error_string(chd_get_last_error()));
		chd_close(chd);
		remove(outputfile);
		return;
	}

	/* alloc and zero buffer*/
	cache = malloc(hunksize);
	if (! cache)
	{
		printf("Error allocating memory buffer\n");
		chd_close(chd);
		remove(outputfile);
		return;
	}
	memset(cache, 0, hunksize);

	/* Zero every hunk */
	totalhunks = (((UINT64)totalsectors * (UINT64)sectorsize) + hunksize - 1) / hunksize;
	lastupdate = 0;
	for (hunknum = 0; hunknum < totalhunks; hunknum++)
	{
		clock_t curtime = clock();

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			UINT64 sourcepos = (UINT64)hunknum * hunksize;
			if (sourcepos)
				progress("Zeroing hunk %d/%d...  \r", hunknum, totalhunks);
			lastupdate = curtime;
		}

		/* write out the data */
		if (chd_write(chd, hunknum, 1, cache) != 1)
		{
			printf("Error writing CHD file: %s\n", error_string(chd_get_last_error()));
			chd_close(chd);
			remove(outputfile);
			return;
		}
	}
	progress("Creation complete!                    \n");

	/* free buffer */
	free(cache);

	/* success */
	chd_close(chd);
}

/*
    Compute the largest common divisor of two numbers.
*/
INLINE UINT32 lcd_u32(UINT32 a, UINT32 b)
{
	UINT32 c;

	/* We use the traditional Euclid algorithm. */
	while (b)
	{
		c = a % b;
		a = b;
		b = c;
	}

	return a;
}

/*-------------------------------------------------
    Copy all hunks of data from one CHD file to another.  The hunk sizes do not
    need to match.  If the source is shorter than the destination, the source
    data will be padded with 0s.

    Example
        [program] -copydata in.hd out.hd
-------------------------------------------------*/
static void do_copydata(int argc, char *argv[])
{
	const char *inputfile, *outputfile;
	chd_file *in_chd, *out_chd;
	UINT32 in_hunksize, out_hunksize;
	UINT32 in_totalhunks, out_totalhunks;
	UINT32 in_hunknum, out_hunknum;
	UINT8 *cache;
	UINT32 cache_data_len;
	clock_t lastupdate;

	/* require 4 args total */
	if (argc != 4)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);

	/* open the src hard drive */
	in_chd = chd_open(inputfile, 0, NULL);
	if (!in_chd)
	{
		printf("Error opening src CHD file: %s\n", error_string(chd_get_last_error()));
		return;
	}
	in_hunksize = chd_get_header(in_chd)->hunkbytes;
	in_totalhunks = chd_get_header(in_chd)->totalhunks;

	/* open the dest hard drive */
	out_chd = chd_open(outputfile, 1, NULL);
	if (!out_chd)
	{
		printf("Error opening dest CHD file: %s\n", error_string(chd_get_last_error()));
		return;
	}
	out_hunksize = chd_get_header(out_chd)->hunkbytes;
	out_totalhunks = chd_get_header(out_chd)->totalhunks;

	/* alloc buffer */
	cache = malloc(in_hunksize + out_hunksize - lcd_u32(in_hunksize, out_hunksize));
	if (! cache)
	{
		printf("Error allocating memory buffer\n");
		chd_close(out_chd);
		chd_close(in_chd);
		return;
	}

	/* copy data */
	cache_data_len = 0;
	in_hunknum = 0;
	lastupdate = 0;
	for (out_hunknum = 0; out_hunknum < out_totalhunks;)
	{
		clock_t curtime = clock();

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			if (out_hunknum)
				progress("Copying hunk %d/%d...  \r", out_hunknum, out_totalhunks);
			lastupdate = curtime;
		}

		/* read in the data */
		while (cache_data_len < out_hunksize)
		{
			if (in_hunknum < in_totalhunks)
			{	/* read data if available */
				if (chd_read(in_chd, in_hunknum, 1, cache+cache_data_len) != 1)
				{
					printf("Error reading CHD file: %s\n", error_string(chd_get_last_error()));
					chd_close(out_chd);
					chd_close(in_chd);
					return;
				}
				in_hunknum++;
				cache_data_len += in_hunksize;
			}
			else
			{	/* if beyond EOF, just zero the buffer */
				memset(cache+cache_data_len, 0, out_hunksize-cache_data_len);
				cache_data_len = out_hunksize;
			}
		}

		/* write out the data */
		while (cache_data_len >= out_hunksize)
		{
			if (chd_write(out_chd, out_hunknum, 1, cache) != 1)
			{
				printf("Error writing CHD file: %s\n", error_string(chd_get_last_error()));
				chd_close(out_chd);
				chd_close(in_chd);
				return;
			}
			out_hunknum++;
			cache_data_len -= out_hunksize;
			if (cache_data_len)
				memmove(cache, cache+out_hunksize, cache_data_len);
		}
	}
	progress("Copy complete!                    \n");

	/* free buffer */
	free(cache);

	/* success */
	chd_close(out_chd);
	chd_close(in_chd);
}

/*-------------------------------------------------
    do_extract - extract a raw file from a
    CHD image
-------------------------------------------------*/

static void do_extract(int argc, char *argv[])
{
	const char *inputfile, *outputfile;
	chd_interface_file *outfile = NULL;
	chd_file *infile = NULL;
	chd_header header;
	void *hunk = NULL;
	clock_t lastupdate;
	int hunknum;

	/* require 4 args total */
	if (argc != 4)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);

	/* get the header */
	infile = chd_open(inputfile, 0, NULL);
	if (!infile)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(chd_get_last_error()));
		goto error;
	}
	header = *chd_get_header(infile);

	/* allocate memory to hold a hunk */
	hunk = malloc(header.hunkbytes);
	if (!hunk)
	{
		printf("Out of memory allocating hunk buffer!\n");
		goto error;
	}

	/* create the output file */
	outfile = (*chdman_interface.open)(outputfile, "wb");
	if (!outfile)
	{
		printf("Error opening output file '%s'\n", outputfile);
		goto error;
	}

	/* loop over hunks, reading and writing */
	lastupdate = 0;
	for (hunknum = 0; hunknum < header.totalhunks; hunknum++)
	{
		clock_t curtime = clock();
		UINT32 byteswritten;

		/* progress */
		if (curtime - lastupdate > CLOCKS_PER_SEC / 2)
		{
			progress("Extracting hunk %d/%d...  \r", hunknum, header.totalhunks);
			lastupdate = curtime;
		}

		/* read the hunk into a buffer */
		if (!chd_read(infile, hunknum, 1, hunk))
		{
			printf("Error reading hunk %d from CHD file: %s\n", hunknum, error_string(chd_get_last_error()));
			goto error;
		}

		if ( (((UINT64)hunknum+1) * (UINT64)header.hunkbytes) < header.logicalbytes )
		{
			/* write the hunk to the file */
			byteswritten = (*chdman_interface.write)(outfile, (UINT64)hunknum * (UINT64)header.hunkbytes, header.hunkbytes, hunk);
			if (byteswritten != header.hunkbytes)
			{
				printf("Error writing hunk %d to output file: %s\n", hunknum, error_string(chd_get_last_error()));
				goto error;
			}
		}
		else
		{
			/* write partial hunk to ensure the correct size raw image is written */
			byteswritten = (*chdman_interface.write)(outfile, (UINT64)hunknum * (UINT64)header.hunkbytes,  header.logicalbytes - ((UINT64)hunknum * (UINT64)header.hunkbytes), hunk);
			if (byteswritten != (header.logicalbytes - ((UINT64)hunknum * (UINT64)header.hunkbytes)) )
			{
				printf("Error writing hunk %d to output file: %s\n", hunknum, error_string(chd_get_last_error()));
				goto error;
			}
		}
	}
	progress("Extraction complete!                    \n");

	/* close everything down */
	(*chdman_interface.close)(outfile);
	free(hunk);
	chd_close(infile);
	return;

error:
	/* clean up our mess */
	if (outfile)
		(*chdman_interface.close)(outfile);
	if (hunk)
		free(hunk);
	if (infile)
		chd_close(infile);
}

/*-------------------------------------------------
    do_extractcd - extract a CDRDAO .toc/.bin
    file from a CHD-CD image
-------------------------------------------------*/

static void do_extractcd(int argc, char *argv[])
{
	const char *inputfile, *outputfile, *outputfile2;
	chd_file *infile = NULL;
	void *hunk = NULL;
	cdrom_file *cdrom = NULL;
	cdrom_toc *toc = NULL;
	FILE *outfile = NULL, *outfile2 = NULL;
	int track, m, s, f, frame;
	long trkoffs, trklen;
	static const char *typenames[8] = { "MODE1", "MODE1_RAW", "MODE2", "MODE2_FORM1", "MODE2_FORM2", "MODE2_FORM_MIX", "MODE2_RAW", "AUDIO" };
	static const char *subnames[2] = { "RW", "RW_RAW" };
	UINT8 sector[CD_MAX_SECTOR_DATA + CD_MAX_SUBCODE_DATA];

	/* require 5 args total */
	if (argc != 5)
		error();

	/* extract the data */
	inputfile = argv[2];
	outputfile = argv[3];
	outputfile2 = argv[4];

	/* print some info */
	printf("Input file:   %s\n", inputfile);
	printf("Output files:  %s (toc) and %s (bin)\n", outputfile, outputfile2);

	/* get the header */
	infile = chd_open(inputfile, 0, NULL);
	if (!infile)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(chd_get_last_error()));
		goto error;
	}

	/* open the CD */
	cdrom = cdrom_open(infile);
	if (!cdrom)
	{
		printf("Error opening CHD-CD '%s'\n", inputfile);
		goto error;
	}

	/* get the TOC data */
	toc = cdrom_get_toc(cdrom);

	/* create the output files */
	outfile = fopen(outputfile, "w");
	if (!outfile)
	{
		printf("Error opening output file '%s'\n", outputfile);
		goto error;
	}

	outfile2 = fopen(outputfile2, "wb");
	if (!outfile2)
	{
		printf("Error opening output file '%s'\n", outputfile2);
		goto error;
	}

	/* process away */
	fprintf(outfile, "CD_ROM\n\n\n");

	trkoffs = 0;
	for (track = 0; track < toc->numtrks; track++)
	{
		printf("Extracting track %d\n", track+1);

		fprintf(outfile, "// Track %d\n", track+1);

		/* write out the track type */
		if (toc->tracks[track].subtype != CD_SUB_NONE)
		{
			fprintf(outfile, "TRACK %s %s\n", typenames[toc->tracks[track].trktype], subnames[toc->tracks[track].subtype]);
		}
		else
		{
			fprintf(outfile, "TRACK %s\n", typenames[toc->tracks[track].trktype]);
		}

		/* write out the attributes */
		fprintf(outfile, "NO COPY\n");
		if (toc->tracks[track].trktype == CD_TRACK_AUDIO)
		{
			fprintf(outfile, "NO PRE_EMPHASIS\n");
			fprintf(outfile, "TWO_CHANNEL_AUDIO\n");

			/* the first audio track on a mixed-track disc always has a 2 second pad */
			if (track == 1)
			{
				if (toc->tracks[track].subtype != CD_SUB_NONE)
				{
					fprintf(outfile, "ZERO AUDIO %s 00:02:00\n", subnames[toc->tracks[track].subtype]);
				}
				else
				{
					fprintf(outfile, "ZERO AUDIO 00:02:00\n");
				}
			}
		}

		/* handle the datafile */
		trklen = toc->tracks[track].frames;

		/* convert to minutes/seconds/frames */
		f = trklen;
		s = f / 75;
		f %= 75;
		m = s / 60;
		s %= 60;

		if (track > 0)
		{
			fprintf(outfile, "DATAFILE \"%s\" #%ld %02d:%02d:%02d // length in bytes: %ld\n", outputfile2, trkoffs, m, s, f, trklen*(toc->tracks[track].datasize+toc->tracks[track].subsize));
		}
		else
		{
			fprintf(outfile, "DATAFILE \"%s\" %02d:%02d:%02d // length in bytes: %ld\n", outputfile2, m, s, f, trklen*(toc->tracks[track].datasize+toc->tracks[track].subsize));
		}

		if ((toc->tracks[track].trktype == CD_TRACK_AUDIO) && (track == 1))
		{
			fprintf(outfile, "START 00:02:00\n");
		}

		// now handle the actual writeout
		for (frame = 0; frame < trklen; frame++)
		{
			cdrom_read_data(cdrom, cdrom_get_chd_start_of_track(cdrom, track)+frame, 1, sector, toc->tracks[track].trktype);
			fwrite(sector, toc->tracks[track].datasize, 1, outfile2);
			trkoffs += toc->tracks[track].datasize;
			cdrom_read_subcode(cdrom, cdrom_get_chd_start_of_track(cdrom, track)+frame, sector);
			fwrite(sector, toc->tracks[track].subsize, 1, outfile2);
			trkoffs += toc->tracks[track].subsize;
		}

		fprintf(outfile, "\n\n");
	}

	printf("Completed!\n");

	/* close everything down */
	fclose(outfile);
	fclose(outfile2);
	free(hunk);
	chd_close(infile);
	return;

error:
	/* clean up our mess */
	if (outfile)
		fclose(outfile);
	if (outfile2)
		fclose(outfile2);
	if (cdrom)
		cdrom_close(cdrom);
	if (infile)
		chd_close(infile);
}

/*-------------------------------------------------
    do_verify - validate the MD5/SHA1 on a drive
    image
-------------------------------------------------*/

static void do_verify(int argc, char *argv[], int fix)
{
	UINT8 actualmd5[CHD_MD5_BYTES], actualsha1[CHD_SHA1_BYTES];
	chd_header header;
	const char *inputfile;
	chd_file *chd;
	int err, fixed = 0;

	/* require 3 args total */
	if (argc != 3)
		error();

	/* extract the data */
	inputfile = argv[2];

	/* print some info */
	printf("Input file:   %s\n", inputfile);

	/* open the new hard drive */
	chd = chd_open(inputfile, 0, NULL);
	if (!chd)
	{
		printf("Error opening CHD file: %s\n", error_string(chd_get_last_error()));
		return;
	}
	header = *chd_get_header(chd);

	/* verify the CHD data */
	err = chd_verify(chd, progress, actualmd5, actualsha1);
	if (err != CHDERR_NONE)
	{
		if (err == CHDERR_CANT_VERIFY)
			printf("Can't verify this type of image (probably writeable)\n");
		else
			printf("\nError during verify: %s\n", error_string(err));
		chd_close(chd);
		return;
	}

	/* verify the MD5 */
	if (!memcmp(header.md5, actualmd5, sizeof(header.md5)))
		printf("MD5 verification successful!\n");
	else
	{
		printf("Error: MD5 in header = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.md5[0], header.md5[1], header.md5[2], header.md5[3],
				header.md5[4], header.md5[5], header.md5[6], header.md5[7],
				header.md5[8], header.md5[9], header.md5[10], header.md5[11],
				header.md5[12], header.md5[13], header.md5[14], header.md5[15]);
		printf("          actual MD5 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				actualmd5[0], actualmd5[1], actualmd5[2], actualmd5[3],
				actualmd5[4], actualmd5[5], actualmd5[6], actualmd5[7],
				actualmd5[8], actualmd5[9], actualmd5[10], actualmd5[11],
				actualmd5[12], actualmd5[13], actualmd5[14], actualmd5[15]);

		/* fix it */
		if (fix)
		{
			memcpy(header.md5, actualmd5, sizeof(header.md5));
			fixed = 1;
		}
	}

	/* verify the SHA1 */
	if (header.version >= 3)
	{
		if (!memcmp(header.sha1, actualsha1, sizeof(header.sha1)))
			printf("SHA1 verification successful!\n");
		else
		{
			printf("Error: SHA1 in header = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
					header.sha1[0], header.sha1[1], header.sha1[2], header.sha1[3],
					header.sha1[4], header.sha1[5], header.sha1[6], header.sha1[7],
					header.sha1[8], header.sha1[9], header.sha1[10], header.sha1[11],
					header.sha1[12], header.sha1[13], header.sha1[14], header.sha1[15],
					header.sha1[16], header.sha1[17], header.sha1[18], header.sha1[19]);
			printf("          actual SHA1 = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
					actualsha1[0], actualsha1[1], actualsha1[2], actualsha1[3],
					actualsha1[4], actualsha1[5], actualsha1[6], actualsha1[7],
					actualsha1[8], actualsha1[9], actualsha1[10], actualsha1[11],
					actualsha1[12], actualsha1[13], actualsha1[14], actualsha1[15],
					actualsha1[16], actualsha1[17], actualsha1[18], actualsha1[19]);

			/* fix it */
			if (fix)
			{
				memcpy(header.sha1, actualsha1, sizeof(header.sha1));
				fixed = 1;
			}
		}
	}

	/* close the drive */
	chd_close(chd);

	/* update the header */
	if (fixed)
	{
		err = chd_set_header(inputfile, &header);
		if (err != CHDERR_NONE)
			printf("Error writing new header: %s\n", error_string(err));
		else
			printf("Updated header successfully\n");
	}
}



/*-------------------------------------------------
    do_info - dump the header information from
    a drive image
-------------------------------------------------*/

static void do_info(int argc, char *argv[])
{
	static const char *compression_type[] =
	{
		"none",
		"zlib",
		"zlib+"
	};
	chd_header header;
	const char *inputfile;
	chd_file *chd;

	/* require 3 args total */
	if (argc != 3)
		error();

	/* extract the data */
	inputfile = argv[2];

	/* print some info */
	printf("Input file:   %s\n", inputfile);

	/* get the header */
	chd = chd_open(inputfile, 0, NULL);
	if (!chd)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(chd_get_last_error()));
		return;
	}
	header = *chd_get_header(chd);

	/* print the info */
	printf("Header Size:  %d bytes\n", header.length);
	printf("File Version: %d\n", header.version);
	printf("Flags:        %s, %s\n",
			(header.flags & CHDFLAGS_HAS_PARENT) ? "HAS_PARENT" : "NO_PARENT",
			(header.flags & CHDFLAGS_IS_WRITEABLE) ? "WRITEABLE" : "READ_ONLY");
	if (header.compression < 3)
		printf("Compression:  %s\n", compression_type[header.compression]);
	else
		printf("Compression:  Unknown type %d\n", header.compression);
	printf("Hunk Size:    %d bytes\n", header.hunkbytes);
	printf("Total Hunks:  %d\n", header.totalhunks);
	printf("Logical size: %s bytes\n", big_int_string(header.logicalbytes));
	if (!(header.flags & CHDFLAGS_IS_WRITEABLE))
		printf("MD5:          %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.md5[0], header.md5[1], header.md5[2], header.md5[3],
				header.md5[4], header.md5[5], header.md5[6], header.md5[7],
				header.md5[8], header.md5[9], header.md5[10], header.md5[11],
				header.md5[12], header.md5[13], header.md5[14], header.md5[15]);
	if (!(header.flags & CHDFLAGS_IS_WRITEABLE) && header.version >= 3)
		printf("SHA1:         %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.sha1[0], header.sha1[1], header.sha1[2], header.sha1[3],
				header.sha1[4], header.sha1[5], header.sha1[6], header.sha1[7],
				header.sha1[8], header.sha1[9], header.sha1[10], header.sha1[11],
				header.sha1[12], header.sha1[13], header.sha1[14], header.sha1[15],
				header.sha1[16], header.sha1[17], header.sha1[18], header.sha1[19]);
	if (header.flags & CHDFLAGS_HAS_PARENT)
	{
		printf("Parent MD5:   %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
				header.parentmd5[0], header.parentmd5[1], header.parentmd5[2], header.parentmd5[3],
				header.parentmd5[4], header.parentmd5[5], header.parentmd5[6], header.parentmd5[7],
				header.parentmd5[8], header.parentmd5[9], header.parentmd5[10], header.parentmd5[11],
				header.parentmd5[12], header.parentmd5[13], header.parentmd5[14], header.parentmd5[15]);
		if (header.version >= 3)
			printf("Parent SHA1:  %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
					header.parentsha1[0], header.parentsha1[1], header.parentsha1[2], header.parentsha1[3],
					header.parentsha1[4], header.parentsha1[5], header.parentsha1[6], header.parentsha1[7],
					header.parentsha1[8], header.parentsha1[9], header.parentsha1[10], header.parentsha1[11],
					header.parentsha1[12], header.parentsha1[13], header.parentsha1[14], header.parentsha1[15],
					header.parentsha1[16], header.parentsha1[17], header.parentsha1[18], header.parentsha1[19]);
	}

	/* print out metadata */
	{
		UINT8 metadata[256];
		int i, j;

		for (i = 0; i < 1000; i++)
		{
			UINT32 metatag = CHDMETATAG_WILDCARD;
			UINT32 count = chd_get_metadata(chd, &metatag, i, metadata, sizeof(metadata));
			if (count == 0 || chd_get_last_error() == CHDERR_METADATA_NOT_FOUND)
				break;

			printf("Metadata:     Tag=%08x  Length=%d\n", metatag, count);
			printf("              ");

			if (count > 60)
				count = 60;
			for (j = 0; j < count; j++)
				printf("%c", isprint(metadata[j]) ? metadata[j] : '.');
			printf("\n");
		}
	}

	chd_close(chd);
}



/*-------------------------------------------------
    handle_custom_chomp - custom chomp a file
-------------------------------------------------*/

#if ENABLE_CUSTOM_CHOMP
static int handle_custom_chomp(const char *name, chd_file *chd, UINT32 *maxhunk)
{
	const chd_header *header = chd_get_header(chd);
	int sectors_per_hunk = (header->hunkbytes / IDE_SECTOR_SIZE);
	UINT8 *temp = malloc(header->hunkbytes);
	if (!temp)
		return CHDERR_OUT_OF_MEMORY;

	/* check for midway */
	if (!strcmp(name, "midway"))
	{
		UINT32 maxsector = 0;
		UINT32 numparts;
		int i;

		if (!chd_read(chd, 0, 1, temp))
			goto error;
		if (temp[0] != 0x54 || temp[1] != 0x52 || temp[2] != 0x41 || temp[3] != 0x50)
			goto error;
		numparts = temp[4] | (temp[5] << 8) | (temp[6] << 16) | (temp[7] << 24);
		printf("%d partitions\n", numparts);
		for (i = 0; i < numparts; i++)
		{
			UINT32 pstart = temp[i*12 + 8] | (temp[i*12 + 9] << 8) | (temp[i*12 + 10] << 16) | (temp[i*12 + 11] << 24);
			UINT32 psize  = temp[i*12 + 12] | (temp[i*12 + 13] << 8) | (temp[i*12 + 14] << 16) | (temp[i*12 + 15] << 24);
			UINT32 pflags = temp[i*12 + 16] | (temp[i*12 + 17] << 8) | (temp[i*12 + 18] << 16) | (temp[i*12 + 19] << 24);
			printf("  %2d. %7d - %7d (%X)\n", i, pstart, pstart + psize - 1, pflags);
			if (i != 0 && pstart + psize > maxsector)
				maxsector = pstart + psize;
		}
		*maxhunk = (maxsector + sectors_per_hunk - 1) / sectors_per_hunk;
		printf("Maximum hunk: %d\n", *maxhunk);
		if (*maxhunk >= header->totalhunks)
		{
			printf("Warning: chomp will have no effect\n");
			*maxhunk = header->totalhunks;
		}
	}

	/* check for atari */
	if (!strcmp(name, "atari"))
	{
		UINT32 sectors[4];
		UINT8 *data;
		int i, maxdiff;

		if (!chd_read(chd, 0x200 / header->hunkbytes, 1, temp))
			goto error;
		data = &temp[0x200 % header->hunkbytes];

		if (data[0] != 0x0d || data[1] != 0xf0 || data[2] != 0xed || data[3] != 0xfe)
			goto error;
		for (i = 0; i < 4; i++)
			sectors[i] = data[i*4+0x40] | (data[i*4+0x41] << 8) | (data[i*4+0x42] << 16) | (data[i*4+0x43] << 24);
		maxdiff = sectors[2] - sectors[1];
		if (sectors[3] - sectors[2] > maxdiff)
			maxdiff = sectors[3] - sectors[2];
		if (sectors[0] != 8)
			goto error;
		*maxhunk = (sectors[3] + maxdiff + sectors_per_hunk - 1) / sectors_per_hunk;
		printf("Maximum hunk: %d\n", *maxhunk);
		if (*maxhunk >= header->totalhunks)
		{
			printf("Warning: chomp will have no effect\n");
			*maxhunk = header->totalhunks;
		}
	}

	free(temp);
	return CHDERR_NONE;

error:
	printf("Error: unable to identify file or compute chomping size.\n");
	free(temp);
	return CHDERR_INVALID_DATA;
}
#endif



/*-------------------------------------------------
    do_merge_update_chomp - merge a parent and its
    child together (also works for update & chomp)
-------------------------------------------------*/

#define OPERATION_UPDATE		0
#define OPERATION_MERGE			1
#define OPERATION_CHOMP			2

static void do_merge_update_chomp(int argc, char *argv[], int operation)
{
	const char *parentfile, *inputfile, *outputfile;
	chd_file *parentchd = NULL;
	chd_file *inputchd = NULL;
	chd_file *outputchd = NULL;
	const chd_header *inputheader;
	UINT8 metadata[CHD_MAX_METADATA_SIZE];
	UINT32 metatag, metasize, metaindex;
	UINT32 maxhunk = ~0;
	int err;

	/* require 4-5 args total */
	if (operation == OPERATION_UPDATE && argc != 4)
		error();
	if ((operation == OPERATION_MERGE || operation == OPERATION_CHOMP) && argc != 5)
		error();

	/* extract the data */
	if (operation == OPERATION_MERGE)
	{
		parentfile = argv[2];
		inputfile = argv[3];
		outputfile = argv[4];
	}
	else
	{
		parentfile = NULL;
		inputfile = argv[2];
		outputfile = argv[3];
		if (operation == OPERATION_CHOMP)
			maxhunk = atoi(argv[4]);
	}

	/* print some info */
	if (parentfile)
	{
		printf("Parent file:  %s\n", parentfile);
		printf("Diff file:    %s\n", inputfile);
	}
	else
		printf("Input file:   %s\n", inputfile);
	printf("Output file:  %s\n", outputfile);
	if (operation == OPERATION_CHOMP)
		printf("Maximum hunk: %d\n", maxhunk);

	/* open the parent CHD */
	if (parentfile)
	{
		parentchd = chd_open(parentfile, 0, NULL);
		if (!parentchd)
		{
			printf("Error opening CHD file '%s': %s\n", parentfile, error_string(err = chd_get_last_error()));
			goto error;
		}
	}

	/* open the diff CHD */
	inputchd = chd_open(inputfile, 0, parentchd);
	if (!inputchd)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err = chd_get_last_error()));
		goto error;
	}
	inputheader = chd_get_header(inputchd);

#if ENABLE_CUSTOM_CHOMP
	/* if we're chomping with a auto parameter, now is the time to figure it out */
	if (operation == OPERATION_CHOMP && maxhunk == 0)
		if (handle_custom_chomp(argv[4], inputchd, &maxhunk) != CHDERR_NONE)
			return;
#endif

	/* create the new merged CHD */
	err = chd_create(outputfile, inputheader->logicalbytes, inputheader->hunkbytes, CHDCOMPRESSION_ZLIB_PLUS, NULL);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		goto error;
	}

	/* open the new CHD */
	outputchd = chd_open(outputfile, 1, NULL);
	if (!outputchd)
	{
		printf("Error opening new CHD file: %s\n", error_string(chd_get_last_error()));
		goto error;
	}

	/* clone the metadata from the input file (which should have inherited from the parent) */
	for (metaindex = 0; ; metaindex++)
	{
		metatag = CHDMETATAG_WILDCARD;
		metasize = chd_get_metadata(inputchd, &metatag, metaindex, metadata, sizeof(metadata));
		if (metasize == 0 || chd_get_last_error() == CHDERR_METADATA_NOT_FOUND)
			break;

		err = chd_set_metadata(outputchd, metatag, CHD_METAINDEX_APPEND, metadata, metasize);
		if (err != CHDERR_NONE)
		{
			printf("Error cloning metadata: %s\n", error_string(err));
			goto error;
		}
	}

	/* do the compression; our interface will route reads for us */
	special_chd_init(inputchd, (operation == OPERATION_CHOMP) ? ((UINT64)(maxhunk + 1) * (UINT64)inputheader->hunkbytes) : inputheader->logicalbytes);
	err = chd_compress(outputchd, SPECIAL_CHD_NAME, 0, progress);
	if (err != CHDERR_NONE)
		printf("Error during compression: %s\n", error_string(err));
	special_chd_finished();

error:
	/* close everything down */
	if (outputchd)
		chd_close(outputchd);
	if (inputchd)
		chd_close(inputchd);
	if (parentchd)
		chd_close(parentchd);
	if (err != CHDERR_NONE)
		remove(outputfile);
}



/*-------------------------------------------------
    do_diff - generate a difference between two
    CHD files
-------------------------------------------------*/

static void do_diff(int argc, char *argv[])
{
	const char *parentfile = NULL, *inputfile = NULL, *outputfile = NULL;
	chd_file *parentchd = NULL;
	chd_file *inputchd = NULL;
	chd_file *outputchd = NULL;
	int err;

	/* require 5 args total */
	if (argc != 5)
		error();

	/* extract the data */
	if (argc == 5)
	{
		parentfile = argv[2];
		inputfile = argv[3];
		outputfile = argv[4];
	}

	/* print some info */
	printf("Parent file:  %s\n", parentfile);
	printf("Input file:   %s\n", inputfile);
	printf("Diff file:    %s\n", outputfile);

	/* open the soon-to-be-parent CHD */
	parentchd = chd_open(parentfile, 0, NULL);
	if (!parentchd)
	{
		printf("Error opening CHD file '%s': %s\n", parentfile, error_string(err = chd_get_last_error()));
		goto error;
	}

	/* open the input CHD */
	inputchd = chd_open(inputfile, 0, NULL);
	if (!inputchd)
	{
		printf("Error opening CHD file '%s': %s\n", inputfile, error_string(err = chd_get_last_error()));
		goto error;
	}

	/* create the new CHD as a diff against the parent */
	err = chd_create(outputfile, 0, 0, CHDCOMPRESSION_ZLIB_PLUS, parentchd);
	if (err != CHDERR_NONE)
	{
		printf("Error creating CHD file: %s\n", error_string(err));
		goto error;
	}

	/* open the new CHD */
	outputchd = chd_open(outputfile, 1, parentchd);
	if (!outputchd)
	{
		printf("Error opening new CHD file: %s\n", error_string(chd_get_last_error()));
		goto error;
	}

	/* do the compression; our interface will route reads for us */
	special_chd_init(inputchd, 0);
	err = chd_compress(outputchd, SPECIAL_CHD_NAME, 0, progress);
	if (err != CHDERR_NONE)
		printf("Error during compression: %s\n", error_string(err));
	special_chd_finished();

error:
	/* close everything down */
	if (outputchd)
		chd_close(outputchd);
	if (inputchd)
		chd_close(inputchd);
	if (parentchd)
		chd_close(parentchd);
	if (err != CHDERR_NONE)
		remove(outputfile);
}



/*-------------------------------------------------
    do_setchs - change the CHS values on a hard
    disk image
-------------------------------------------------*/

static void do_setchs(int argc, char *argv[])
{
	int oldcyls, oldhds, oldsecs, oldsecsize;
	int cyls, hds, secs, err;
	const char *inoutfile;
	chd_header header;
	chd_file *chd = NULL;
	char metadata[256];
	UINT64 old_logicalbytes;
	UINT32 metasize;
	UINT32 metatag;
	UINT8 was_readonly = 0;

	/* require 6 args total */
	if (argc != 6)
		error();

	/* extract the data */
	inoutfile = argv[2];
	cyls = atoi(argv[3]);
	hds = atoi(argv[4]);
	secs = atoi(argv[5]);

	/* print some info */
	printf("Input file:   %s\n", inoutfile);
	printf("Cylinders:    %d\n", cyls);
	printf("Heads:        %d\n", hds);
	printf("Sectors:      %d\n", secs);

	/* open the file read-only and get the header */
	chd = chd_open(inoutfile, 0, NULL);
	if (!chd)
	{
		printf("Error opening CHD file '%s' read-only: %s\n", inoutfile, error_string(chd_get_last_error()));
		return;
	}
	header = *chd_get_header(chd);
	chd_close(chd);

	/* if the drive is not writeable, note that, and make it so */
	if (!(header.flags & CHDFLAGS_IS_WRITEABLE))
	{
		was_readonly = 1;
		header.flags |= CHDFLAGS_IS_WRITEABLE;

		/* write the new header */
		err = chd_set_header(inoutfile, &header);
		if (err != CHDERR_NONE)
		{
			printf("Error making CHD file writeable: %s\n", error_string(err));
			return;
		}
	}

	/* open the file read/write */
	chd = chd_open(inoutfile, 1, NULL);
	if (!chd)
	{
		printf("Error opening CHD file '%s' read/write: %s\n", inoutfile, error_string(chd_get_last_error()));
		return;
	}

	/* get the hard disk metadata */
	metatag = HARD_DISK_STANDARD_METADATA;
	metasize = chd_get_metadata(chd, &metatag, 0, metadata, sizeof(metadata));
	if (metasize == 0 || sscanf(metadata, HARD_DISK_METADATA_FORMAT, &oldcyls, &oldhds, &oldsecs, &oldsecsize) != 4)
	{
		printf("CHD file '%s' is not a hard disk!\n", inoutfile);
		goto cleanup;
	}

	/* write our own */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cyls, hds, secs, oldsecsize);
	err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
	if (err != CHDERR_NONE)
	{
		printf("Error writing new metadata to CHD file: %s\n", error_string(err));
		goto cleanup;
	}

	/* get the header and compute the new logical size */
	header = *chd_get_header(chd);
	old_logicalbytes = header.logicalbytes;
	header.logicalbytes = (UINT64)cyls * (UINT64)hds * (UINT64)secs * (UINT64)oldsecsize;

	/* close the file */
	chd_close(chd);

	/* restore the read-only state */
	if (was_readonly)
		header.flags &= ~CHDFLAGS_IS_WRITEABLE;

	/* set the new logical size */
	if (header.logicalbytes != old_logicalbytes || was_readonly)
	{
		err = chd_set_header(inoutfile, &header);
		if (err != CHDERR_NONE)
			printf("Error writing new header to CHD file: %s\n", error_string(err));
	}

	/* print a warning if the size is different */
	if (header.logicalbytes < old_logicalbytes)
		printf("WARNING: new size is smaller; run chdman -update to reclaim empty space\n");
	else if (header.logicalbytes > old_logicalbytes)
		printf("WARNING: new size is larger; run chdman -update to account for new empty space\n");
	return;

cleanup:
	if (chd)
		chd_close(chd);
	if (was_readonly)
	{
		header.flags &= ~CHDFLAGS_IS_WRITEABLE;
		chd_set_header(inoutfile, &header);
	}
}



/*-------------------------------------------------
    chdman_open - open file
-------------------------------------------------*/

static chd_interface_file *chdman_open(const char *filename, const char *mode)
{
	/* if it's the special CHD filename, just hand back our handle */
	if (!strcmp(filename, SPECIAL_CHD_NAME))
		return (chd_interface_file *)special_chd;

	/* otherwise, open normally */
	else
		return (chd_interface_file *) osd_tool_fopen(filename, mode);
}



/*-------------------------------------------------
    chdman_close - close file
-------------------------------------------------*/

static void chdman_close(chd_interface_file *file)
{
	/* if it's the special chd handle, do nothing */
	if (file == (chd_interface_file *)special_chd)
		return;

	osd_tool_fclose((osd_tool_file *) file);
}



/*-------------------------------------------------
    chdman_read - read from an offset
-------------------------------------------------*/

static UINT32 chdman_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	/* if it's the special chd handle, read from it */
	if (file == (chd_interface_file *)special_chd)
	{
		const chd_header *header = chd_get_header(special_chd);
		UINT32 result;

		/* validate the read: we can only handle block-aligned reads here */
		if (offset % header->hunkbytes != 0)
		{
			printf("Error: chdman read from non-aligned offset %08X%08X\n", (UINT32)(offset >> 32), (UINT32)offset);
			return 0;
		}
		if (count % header->hunkbytes != 0)
		{
			printf("Error: chdman read non-aligned amount %08X\n", count);
			return 0;
		}

		/* if we're reading past the logical end, indicate we didn't read a thing */
		if (offset >= special_logicalbytes)
			return 0;

		/* read the block(s) */
		result = header->hunkbytes * chd_read(special_chd, offset / header->hunkbytes, count / header->hunkbytes, buffer);

		/* count errors */
		if (result != count && chd_get_last_error() != CHDERR_NONE)
			special_error_count++;

		/* update the checksums */
		if (offset == special_bytes_checksummed && offset < special_original_logicalbytes)
		{
			UINT32 bytestochecksum = result;

			/* clamp to the original number of logical bytes */
			if (special_bytes_checksummed + bytestochecksum > special_original_logicalbytes)
				bytestochecksum = special_original_logicalbytes - special_bytes_checksummed;

			/* update the two checksums */
			if (bytestochecksum)
			{
				MD5Update(&special_md5, buffer, bytestochecksum);
				sha1_update(&special_sha1, bytestochecksum, buffer);
				special_bytes_checksummed += bytestochecksum;
			}
		}

		/* if we were supposed to read past the end, indicate the poper number of bytes */
		if (offset + result > special_logicalbytes)
			result = special_logicalbytes - offset;
		return result;
	}

	/* otherwise, do it normally */
	else
	{
		return osd_tool_fread((osd_tool_file *) file, offset, count, buffer);
	}
}



/*-------------------------------------------------
    chdman_write - write to an offset
-------------------------------------------------*/

static UINT32 chdman_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	/* if it's the special chd handle, this is bad */
	if (file == (chd_interface_file *)special_chd)
	{
		printf("Error: chdman write to CHD image = bad!\n");
		return 0;
	}

	/* otherwise, do it normally */
	else
	{
		return osd_tool_fwrite((osd_tool_file *) file, offset, count, buffer);
	}
}



/*-------------------------------------------------
    chdman_length - return the current EOF
-------------------------------------------------*/

static UINT64 chdman_length(chd_interface_file *file)
{
	/* if it's the special chd handle, this is bad */
	if (file == (chd_interface_file *)special_chd)
		return special_logicalbytes;

	/* otherwise, do it normally */
	else
	{
		return osd_tool_flength((osd_tool_file *) file);
	}
}



/*-------------------------------------------------
    main - entry point
-------------------------------------------------*/

int main(int argc, char **argv)
{
	extern char build_version[];
	printf("chdman - MAME Compressed Hunks of Data (CHD) manager %s\n", build_version);

	/* require at least 1 argument */
	if (argc < 2)
		error();

	/* set the interface for everyone */
	chd_set_interface(&chdman_interface);

	/* handle the appropriate command */
	if (!strcmp(argv[1], "-createhd"))
		do_createhd(argc, argv);
	else if (!strcmp(argv[1], "-createblankhd"))
		do_createblankhd(argc, argv);
	else if (!strcmp(argv[1], "-copydata"))
		do_copydata(argc, argv);
	else if (!strcmp(argv[1], "-createcd"))
		do_createcd(argc, argv);
	else if (!strcmp(argv[1], "-extract"))
		do_extract(argc, argv);
	else if (!strcmp(argv[1], "-extractcd"))
		do_extractcd(argc, argv);
	else if (!strcmp(argv[1], "-verify"))
		do_verify(argc, argv, 0);
	else if (!strcmp(argv[1], "-verifyfix"))
		do_verify(argc, argv, 1);
	else if (!strcmp(argv[1], "-update"))
		do_merge_update_chomp(argc, argv, OPERATION_UPDATE);
	else if (!strcmp(argv[1], "-chomp"))
		do_merge_update_chomp(argc, argv, OPERATION_CHOMP);
	else if (!strcmp(argv[1], "-info"))
		do_info(argc, argv);
	else if (!strcmp(argv[1], "-merge"))
		do_merge_update_chomp(argc, argv, OPERATION_MERGE);
	else if (!strcmp(argv[1], "-diff"))
		do_diff(argc, argv);
	else if (!strcmp(argv[1], "-setchs"))
		do_setchs(argc, argv);
	else
		error();

	return 0;
}
