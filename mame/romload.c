// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria,Paul Priest,Aaron Giles

/*********************************************************************

    romload.c

    ROM loading functions.

*********************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "hash.h"
#include "png.h"
#include "harddisk.h"
#include "artwork.h"
#include "config.h"
#include <stdarg.h>
#include <ctype.h>


//#define LOG_LOAD



/***************************************************************************

    Global variables

***************************************************************************/

/* disks */
static chd_file *disk_handle[4];
static chd_write_file *disk_diff_handle[4];

/* system BIOS */
int system_bios;

static int total_rom_load_warnings;



/***************************************************************************

    Hard disk handling

***************************************************************************/


static mame_file *open_disk_image_file(const game_driver *gamedrv, const char *filename)
{
	const game_driver *drv;

	for (drv = gamedrv; drv != NULL; drv = driver_get_clone(drv))
	{
		mame_file *file = mame_fopen(drv->name, filename, FILETYPE_IMAGE, 0);
		if (file != NULL)
			return file;
	}
	return NULL;
}

chd_file *get_disk_handle(int diskindex)
{
	return disk_handle[diskindex];
}



/***************************************************************************

    ROM loading code

***************************************************************************/

/*-------------------------------------------------
    rom_first_region - return pointer to first ROM
    region
-------------------------------------------------*/

const rom_entry *rom_first_region(const game_driver *drv)
{
	const rom_entry *romp = drv->rom;
	while (romp && !ROMENTRY_ISREGION(romp) && !ROMENTRY_ISEND(romp))
		romp++;
	return (romp && ROMENTRY_ISREGION(romp)) ? romp : NULL;
}



/*-------------------------------------------------
    rom_next_region - return pointer to next ROM
    region
-------------------------------------------------*/

const rom_entry *rom_next_region(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
    rom_first_file - return pointer to first ROM
    file
-------------------------------------------------*/

const rom_entry *rom_first_file(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
    rom_next_file - return pointer to next ROM
    file
-------------------------------------------------*/

const rom_entry *rom_next_file(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISFILE(romp) && !ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISREGIONEND(romp) ? NULL : romp;
}


/*-------------------------------------------------
    rom_first_chunk - return pointer to first ROM
    chunk
-------------------------------------------------*/

const rom_entry *rom_first_chunk(const rom_entry *romp)
{
	return (ROMENTRY_ISFILE(romp)) ? romp : NULL;
}


/*-------------------------------------------------
    rom_next_chunk - return pointer to next ROM
    chunk
-------------------------------------------------*/

const rom_entry *rom_next_chunk(const rom_entry *romp)
{
	romp++;
	return (ROMENTRY_ISCONTINUE(romp) || ROMENTRY_ISIGNORE(romp)) ? romp : NULL;
}


/* Archive markers belong to the declaration, not the machine configuration.
   The lookup is also used by XML so file ownership cannot drift from loading. */
const char *rom_archive_bios(const rom_entry *region, const rom_entry *rom)
{
	return rom > region + 1 && ROMENTRY_ISBIOS_LOAD(rom - 1) ? ROM_GETARCHIVEBIOS(rom - 1) : 0;
}

static const game_driver *rom_archive_resource(const char *name)
{
	int i;
	for (i = 0; drivers[i]; ++i)
		if ((drivers[i]->flags & GAME_IS_DEVICE) && !strcmp(drivers[i]->name, name))
			return drivers[i];
	return 0;
}

static UINT32 rom_archive_file_length(const rom_entry *rom)
{
	UINT32 length = 0;
	const rom_entry *chunk;
	for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
		length += ROM_GETLENGTH(chunk);
	return length;
}

static const rom_entry *rom_archive_inventory_file(const game_driver *resource, const rom_entry *rom)
{
	const rom_entry *region, *file;
	for (region = rom_first_region(resource); region; region = rom_next_region(region))
		for (file = rom_first_file(region); file; file = rom_next_file(file))
			if (rom_archive_file_length(rom) == rom_archive_file_length(file) &&
				((ROM_NOGOODDUMP(rom) && ROM_NOGOODDUMP(file) && !strcmp(ROM_GETNAME(rom), ROM_GETNAME(file))) ||
				(!ROM_NOGOODDUMP(rom) && !ROM_NOGOODDUMP(file) && hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(file), 0))))
				return file;
	return 0;
}

int rom_validate_archive_bios(const game_driver *game, char *error, unsigned error_size)
{
	const rom_entry *entry, *region = 0;
	const game_driver *resource = 0;
	const char *archive = 0;
	for (entry = game->rom; entry && !ROMENTRY_ISEND(entry); ++entry)
	{
		if (ROMENTRY_ISREGION(entry))
		{
			region = entry;
			archive = 0;
			resource = 0;
		}
		else if (ROMENTRY_ISBIOS_LOAD(entry))
		{
			archive = ROM_GETARCHIVEBIOS(entry);
			if (!region || ROMREGION_ISDISKDATA(region))
			{
				snprintf(error, error_size, "%s: BIOS_LOAD requires a ROM region", game->name);
				return 1;
			}
			if (!ROMENTRY_ISFILE(entry + 1))
			{
				snprintf(error, error_size, "%s: BIOS_LOAD archive prefix must precede a file", game->name);
				return 1;
			}
			if (!archive || !*archive || strlen(archive) > DRIVER_NAME_MAX || strspn(archive, "abcdefghijklmnopqrstuvwxyz0123456789_") != strlen(archive))
			{
				snprintf(error, error_size, "%s: invalid firmware archive name", game->name);
				return 1;
			}
			resource = rom_archive_resource(archive);
			if (!resource || (game->flags & GAME_IS_DEVICE))
			{
				snprintf(error, error_size, "%s: unknown or nested firmware archive %s", game->name, archive);
				return 1;
			}
			if (rom_validate_archive_bios(resource, error, error_size))
				return 1;
		}
		else if (resource && ROMENTRY_ISFILE(entry))
		{
			/* XML hides archived files from the game, so every such file must
			   exist in the exported resource inventory, including its full length. */
			if (!rom_archive_inventory_file(resource, entry))
			{
				snprintf(error, error_size, "%s: ROM %s is absent from archive inventory %s", game->name, ROM_GETNAME(entry), archive);
				return 1;
			}
			resource = 0;
			archive = 0;
		}
	}
	return 0;
}

mame_file *rom_open_archive_file(const game_driver *game, const char *archive, const rom_entry *rom)
{
	const game_driver *parent;
	mame_file *file = 0;
	if (archive)
	{
		const game_driver *resource = rom_archive_resource(archive);
		const rom_entry *canonical = resource ? rom_archive_inventory_file(resource, rom) : 0;
		/* A legacy filename may differ from the canonical BIOS filename. Use
		   the inventory name here so a bad dump still produces a checksum
		   warning rather than failing a CRC-only alias lookup as missing. */
		file = mame_fopen_rom(archive, ROM_GETNAME(canonical ? canonical : rom), ROM_GETHASHDATA(rom));
	}
	for (parent = game; !file && parent; parent = driver_get_clone(parent))
		if (!archive || strcmp(parent->name, archive))
			file = mame_fopen_rom(parent->name, ROM_GETNAME(rom), ROM_GETHASHDATA(rom));
	return file;
}


/*-------------------------------------------------
    debugload - log data to a file
-------------------------------------------------*/

void CLIB_DECL debugload(const char *string, ...)
{
#ifdef LOG_LOAD
	static int opened;
	va_list arg;
	FILE *f;

	f = fopen("romload.log", opened++ ? "a" : "w");
	if (f)
	{
		va_start(arg, string);
		vfprintf(f, string, arg);
		va_end(arg);
		fclose(f);
	}
#endif
}


/*-------------------------------------------------
    determine_bios_rom - determine system_bios
    from SystemBios structure, options.bios, and
    optional ROM_BIOS_DEFAULT
-------------------------------------------------*/

int determine_bios_rom(const bios_entry *bios, const rom_entry *romp)
{
	const bios_entry *firstbios = bios;
	const char *bios_name = NULL;

	/* set to default */
	int bios_no = 0;

	if (!bios)
		return 0;

	/* Not system_bios_0 and options.bios is set and not default */
	if (options.bios != NULL && strcmp(options.bios, "default") != 0)
	{
		bios_name = options.bios;
	}
	else if (romp != NULL)
	{
		/* Look for ROM_BIOS_DEFAULT in the game's rom entries */
		for (; !ROMENTRY_ISEND(romp); romp++)
		{
			if (ROMENTRY_ISBIOS_DEFAULT(romp))
			{
				bios_name = ROM_GETDEFAULTBIOS(romp);
				break;
			}
		}
	}

	if (bios_name != NULL)
	{
		/* Allow '-bios n' to still be used */
		while (!BIOSENTRY_ISEND(bios))
		{
			char bios_number[12];
			sprintf(bios_number, "%d", bios->value);

			if (!strcmp(bios_number, bios_name))
			{
				bios_no = bios->value;
				break;
			}

			bios++;
		}

		bios = firstbios;

		/* Test for bios short names */
		while (!BIOSENTRY_ISEND(bios))
		{
			if (!strcmp(bios->_name, bios_name))
			{
				bios_no = bios->value;
				break;
			}

			bios++;
		}
	}

	debugload("Using System BIOS: %d\n", bios_no);

	return bios_no;
}


/*-------------------------------------------------
    count_roms - counts the total number of ROMs
    that will need to be loaded
-------------------------------------------------*/

static int count_roms(const rom_entry *romp)
{
	const rom_entry *region, *rom;
	int count = 0;

	/* determine the correct biosset to load based on options.bios string */
	int this_bios = determine_bios_rom(Machine->gamedrv->bios, Machine->gamedrv->rom);

	/* loop over regions, then over files */
	for (region = rom_first_region(Machine->gamedrv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			if (!ROM_GETBIOSFLAGS(rom) || (ROM_GETBIOSFLAGS(rom) == (this_bios+1))) /* alternate bios sets */
				count++;

	/* return the total count */
	return count;
}


/*-------------------------------------------------
    fill_random - fills an area of memory with
    random data
-------------------------------------------------*/

static void fill_random(UINT8 *base, UINT32 length)
{
	while (length--)
		*base++ = rand();
}


/*-------------------------------------------------
    handle_missing_file - handles error generation
    for missing files
-------------------------------------------------*/

static void handle_missing_file(rom_load_data *romdata, const rom_entry *romp)
{
	if (romdata->archive_bios)
		snprintf(romdata->errorbuf + strlen(romdata->errorbuf), sizeof(romdata->errorbuf) - strlen(romdata->errorbuf), "Archive %s, region %x: ", romdata->archive_bios, romdata->destination);

	/* optional files are okay */
	if (ROM_ISOPTIONAL(romp))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "OPTIONAL %-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* no good dumps are okay */
	else if (ROM_NOGOODDUMP(romp))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND (NO GOOD DUMP KNOWN)\n", ROM_GETNAME(romp));
		romdata->warnings++;
	}

	/* anything else is bad */
	else
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NOT FOUND\n", ROM_GETNAME(romp));
		romdata->errors++;
	}
}


/*-------------------------------------------------
    dump_wrong_and_correct_checksums - dump an
    error message containing the wrong and the
    correct checksums for a given ROM
-------------------------------------------------*/

static void dump_wrong_and_correct_checksums(rom_load_data* romdata, const char* hash, const char* acthash)
{
	unsigned i;
	char chksum[256];
	unsigned found_functions;
	unsigned wrong_functions;

	found_functions = hash_data_used_functions(hash) & hash_data_used_functions(acthash);

	hash_data_print(hash, found_functions, chksum);
	sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "    EXPECTED: %s\n", chksum);

	/* We dump informations only of the functions for which MAME provided
        a correct checksum. Other functions we might have calculated are
        useless here */
	hash_data_print(acthash, found_functions, chksum);
	sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "       FOUND: %s\n", chksum);

	/* For debugging purposes, we check if the checksums available in the
       driver are correctly specified or not. This can be done by checking
       the return value of one of the extract functions. Maybe we want to
       activate this only in debug buils, but many developers only use
       release builds, so I keep it as is for now. */
	wrong_functions = 0;
	for (i=0;i<HASH_NUM_FUNCTIONS;i++)
		if (hash_data_extract_printable_checksum(hash, 1<<i, chksum) == 2)
			wrong_functions |= 1<<i;

	if (wrong_functions)
	{
		for (i=0;i<HASH_NUM_FUNCTIONS;i++)
			if (wrong_functions & (1<<i))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)],
					"\tInvalid %s checksum treated as 0 (check leading zeros)\n",
					hash_function_name(1<<i));

				romdata->warnings++;
			}
	}
}


/*-------------------------------------------------
    verify_length_and_hash - verify the length
    and hash signatures of a file
-------------------------------------------------*/

static void verify_length_and_hash(rom_load_data *romdata, const char *name, UINT32 explength, const char* hash)
{
	UINT32 actlength;
	const char* acthash;

	/* we've already complained if there is no file */
	if (!romdata->file)
		return;

	/* get the length and CRC from the file */
	actlength = mame_fsize(romdata->file);
	acthash = mame_fhash(romdata->file);

	/* verify length */
	if (explength != actlength)
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG LENGTH (expected: %08x found: %08x)\n", name, explength, actlength);
		romdata->warnings++;
	}

	/* If there is no good dump known, write it */
	if (hash_data_has_info(hash, HASH_INFO_NO_DUMP))
	{
			sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NO GOOD DUMP KNOWN\n", name);
		romdata->warnings++;
	}
	/* verify checksums */
	else if (!hash_data_is_equal(hash, acthash, 0))
	{
		if (romdata->archive_bios)
			snprintf(romdata->errorbuf + strlen(romdata->errorbuf), sizeof(romdata->errorbuf) - strlen(romdata->errorbuf), "Archive %s, region %x: ", romdata->archive_bios, romdata->destination);
		/* otherwise, it's just bad */
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG CHECKSUMS:\n", name);

		dump_wrong_and_correct_checksums(romdata, hash, acthash);

		romdata->warnings++;
	}
	/* If it matches, but it is actually a bad dump, write it */
	else if (hash_data_has_info(hash, HASH_INFO_BAD_DUMP))
	{
		sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s ROM NEEDS REDUMP\n",name);
		romdata->warnings++;
	}
}


/*-------------------------------------------------
    display_rom_load_results - display the final
    results of ROM loading
-------------------------------------------------*/

static int display_rom_load_results(rom_load_data *romdata)
{
	int region;

	/* final status display */
	osd_display_loading_rom_message(NULL, romdata);

	/* if we had errors, they are fatal */
	if (romdata->errors)
	{
		strcat(romdata->errorbuf, "ERROR: required files are missing, the game cannot be run.");
		fatalerror("%s", romdata->errorbuf);
	}

	/* if we had warnings, output them, but continue */
	if (romdata->warnings)
	{
		strcat(romdata->errorbuf, "WARNING: the game might not run correctly.");
		/* AdvanceMAME: Display the message in the ui */
		osd_display_loading_rom_message(romdata->errorbuf, 0);
	}

	/* clean up any regions */
	if (romdata->errors)
		for (region = 0; region < MAX_MEMORY_REGIONS; region++)
			free_memory_region(region);

	/* return true if we had any errors */
	return (romdata->errors != 0);
}


/*-------------------------------------------------
    region_post_process - post-process a region,
    byte swapping and inverting data as necessary
-------------------------------------------------*/

static void region_post_process(rom_load_data *romdata, const rom_entry *regiondata)
{
	int type = ROMREGION_GETTYPE(regiondata);
	int datawidth = ROMREGION_GETWIDTH(regiondata) / 8;
	int littleendian = ROMREGION_ISLITTLEENDIAN(regiondata);
	UINT8 *base;
	int i, j;

	debugload("+ datawidth=%d little=%d\n", datawidth, littleendian);

	/* if this is a CPU region, override with the CPU width and endianness */
	if (type >= REGION_CPU1 && type < REGION_CPU1 + MAX_CPU)
	{
		int cputype = Machine->drv->cpu[type - REGION_CPU1].cpu_type;
		if (cputype != 0)
		{
			datawidth = cputype_databus_width(cputype, ADDRESS_SPACE_PROGRAM) / 8;
			littleendian = (cputype_endianness(cputype) == CPU_IS_LE);
			debugload("+ CPU region #%d: datawidth=%d little=%d\n", type - REGION_CPU1, datawidth, littleendian);
		}
	}

	/* if the region is inverted, do that now */
	if (ROMREGION_ISINVERTED(regiondata))
	{
		debugload("+ Inverting region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i++)
			*base++ ^= 0xff;
	}

	/* swap the endianness if we need to */
#ifdef LSB_FIRST
	if (datawidth > 1 && !littleendian)
#else
	if (datawidth > 1 && littleendian)
#endif
	{
		debugload("+ Byte swapping region\n");
		for (i = 0, base = romdata->regionbase; i < romdata->regionlength; i += datawidth)
		{
			UINT8 temp[8];
			memcpy(temp, base, datawidth);
			for (j = datawidth - 1; j >= 0; j--)
				*base++ = temp[j];
		}
	}
}


/*-------------------------------------------------
    open_rom_file - open a ROM file, searching
    up the parent and loading by checksum
-------------------------------------------------*/

static int open_rom_file(rom_load_data *romdata, const rom_entry *romp)
{
	const game_driver *drv;

	++romdata->romsloaded;

	/* update status display */
	if (osd_display_loading_rom_message(ROM_GETNAME(romp), romdata))
       return 0;

	/* Attempt reading up the chain through the parents. It automatically also
       attempts any kind of load by checksum supported by the archives. */
	romdata->file = rom_open_archive_file(Machine->gamedrv, romdata->archive_bios, romp);

	/* return the result */
	return (romdata->file != NULL);
}


/*-------------------------------------------------
    rom_fread - cheesy fread that fills with
    random data for a NULL file
-------------------------------------------------*/

static int rom_fread(rom_load_data *romdata, UINT8 *buffer, int length)
{
	/* files just pass through */
	if (romdata->file)
		return mame_fread(romdata->file, buffer, length);

	/* otherwise, fill with randomness */
	else
		fill_random(buffer, length);

	return length;
}


/*-------------------------------------------------
    read_rom_data - read ROM data for a single
    entry
-------------------------------------------------*/

static int read_rom_data(rom_load_data *romdata, const rom_entry *romp)
{
	int datashift = ROM_GETBITSHIFT(romp);
	int datamask = ((1 << ROM_GETBITWIDTH(romp)) - 1) << datashift;
	int numbytes = ROM_GETLENGTH(romp);
	int groupsize = ROM_GETGROUPSIZE(romp);
	int skip = ROM_GETSKIPCOUNT(romp);
	int reversed = ROM_ISREVERSED(romp);
	int numgroups = (numbytes + groupsize - 1) / groupsize;
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int i;

	if (ROMENTRY_ISIGNORE(romp))
	{
		if (romdata->file)
			mame_fseek(romdata->file, numbytes, SEEK_CUR);
		return 0;
	}

	debugload("Loading ROM data: offs=%X len=%X mask=%02X group=%d skip=%d reverse=%d\n", ROM_GETOFFSET(romp), numbytes, datamask, groupsize, skip, reversed);

	/* make sure the length was an even multiple of the group size */
	if (numbytes % groupsize != 0)
	{
		printf("Error in RomModule definition: %s length not an even multiple of group size\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure we only fill within the region space */
	if (ROM_GETOFFSET(romp) + numgroups * groupsize + (numgroups - 1) * skip > romdata->regionlength)
	{
		printf("Error in RomModule definition: %s out of memory region space\n", ROM_GETNAME(romp));
		return -1;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: %s has an invalid length\n", ROM_GETNAME(romp));
		return -1;
	}

	/* special case for simple loads */
	if (datamask == 0xff && (groupsize == 1 || !reversed) && skip == 0)
		return rom_fread(romdata, base, numbytes);

	/* chunky reads for complex loads */
	skip += groupsize;
	while (numbytes)
	{
		int evengroupcount = (sizeof(romdata->tempbuf) / groupsize) * groupsize;
		int bytesleft = (numbytes > evengroupcount) ? evengroupcount : numbytes;
		UINT8 *bufptr = romdata->tempbuf;

		/* read as much as we can */
		debugload("  Reading %X bytes into buffer\n", bytesleft);
		if (rom_fread(romdata, romdata->tempbuf, bytesleft) != bytesleft)
			return 0;
		numbytes -= bytesleft;

		debugload("  Copying to %08X\n", (int)base);

		/* unmasked cases */
		if (datamask == 0xff)
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = *bufptr++;

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}
		}

		/* masked cases */
		else
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = (*base & ~datamask) | ((*bufptr++ << datashift) & datamask);

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}
		}
	}
	debugload("  All done\n");
	return ROM_GETLENGTH(romp);
}


/*-------------------------------------------------
    fill_rom_data - fill a region of ROM space
-------------------------------------------------*/

static int fill_rom_data(rom_load_data *romdata, const rom_entry *romp)
{
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);

	/* make sure we fill within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		printf("Error in RomModule definition: FILL out of memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: FILL has an invalid length\n");
		return 0;
	}

	/* fill the data (filling value is stored in place of the hashdata) */
	memset(base, (UINT32)ROM_GETHASHDATA(romp) & 0xff, numbytes);
	return 1;
}


/*-------------------------------------------------
    copy_rom_data - copy a region of ROM space
-------------------------------------------------*/

static int copy_rom_data(rom_load_data *romdata, const rom_entry *romp)
{
	UINT8 *base = romdata->regionbase + ROM_GETOFFSET(romp);
	int srcregion = ROM_GETFLAGS(romp) >> 24;
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT32 srcoffs = (UINT32)ROM_GETHASHDATA(romp);  /* srcoffset in place of hashdata */
	UINT8 *srcbase;

	/* make sure we copy within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > romdata->regionlength)
	{
		printf("Error in RomModule definition: COPY out of target memory region space\n");
		return 0;
	}

	/* make sure the length was valid */
	if (numbytes == 0)
	{
		printf("Error in RomModule definition: COPY has an invalid length\n");
		return 0;
	}

	/* make sure the source was valid */
	srcbase = memory_region(srcregion);
	if (!srcbase)
	{
		printf("Error in RomModule definition: COPY from an invalid region\n");
		return 0;
	}

	/* make sure we find within the region space */
	if (srcoffs + numbytes > memory_region_length(srcregion))
	{
		printf("Error in RomModule definition: COPY out of source memory region space\n");
		return 0;
	}

	/* fill the data */
	memcpy(base, srcbase + srcoffs, numbytes);
	return 1;
}


/*-------------------------------------------------
    process_rom_entries - process all ROM entries
    for a region
-------------------------------------------------*/

static int process_rom_entries(rom_load_data *romdata, const rom_entry *romp)
{
	UINT32 lastflags = 0;

	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* if this is a continue entry, it's invalid */
		if (ROMENTRY_ISCONTINUE(romp) || ROMENTRY_ISIGNORE(romp))
		{
			printf("Error in RomModule definition: ROM_CONTINUE or ROM_IGNORE not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		/* if this is a reload entry, it's invalid */
		if (ROMENTRY_ISRELOAD(romp))
		{
			printf("Error in RomModule definition: ROM_RELOAD not preceded by ROM_LOAD\n");
			goto fatalerror;
		}

		if (ROMENTRY_ISBIOS_LOAD(romp))
		{
			romdata->archive_bios = ROM_GETARCHIVEBIOS(romp++);
			continue;
		}

		/* handle fills */
		if (ROMENTRY_ISFILL(romp))
		{
			if (!fill_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle copies */
		else if (ROMENTRY_ISCOPY(romp))
		{
			if (!copy_rom_data(romdata, romp++))
				goto fatalerror;
		}

		/* handle files */
		else if (ROMENTRY_ISFILE(romp))
		{
			if (!ROM_GETBIOSFLAGS(romp) || (ROM_GETBIOSFLAGS(romp) == (system_bios+1))) /* alternate bios sets */
			{
				const rom_entry *baserom = romp;
				int explength = 0;

				/* open the file */
				debugload("Opening ROM file: %s\n", ROM_GETNAME(romp));
				if (!open_rom_file(romdata, romp))
					handle_missing_file(romdata, romp);

				/* loop until we run out of reloads */
				do
				{
					/* loop until we run out of continues */
					do
					{
						rom_entry modified_romp = *romp++;
						int readresult;

						/* handle flag inheritance */
						if (!ROM_INHERITSFLAGS(&modified_romp))
							lastflags = modified_romp._flags;
						else
							modified_romp._flags = (modified_romp._flags & ~ROM_INHERITEDFLAGS) | lastflags;

						explength += ROM_GETLENGTH(&modified_romp);

						/* attempt to read using the modified entry */
						readresult = read_rom_data(romdata, &modified_romp);
						if (readresult == -1)
							goto fatalerror;
					}
					while (ROMENTRY_ISCONTINUE(romp) || ROMENTRY_ISIGNORE(romp));

					/* if this was the first use of this file, verify the length and CRC */
					if (baserom)
					{
						debugload("Verifying length (%X) and checksums\n", explength);
						verify_length_and_hash(romdata, ROM_GETNAME(baserom), explength, ROM_GETHASHDATA(baserom));
						debugload("Verify finished\n");
					}

					/* reseek to the start and clear the baserom so we don't reverify */
					if (romdata->file)
						mame_fseek(romdata->file, 0, SEEK_SET);
					baserom = NULL;
					explength = 0;
				}
				while (ROMENTRY_ISRELOAD(romp));

				/* close the file */
				if (romdata->file)
				{
					debugload("Closing ROM file\n");
					mame_fclose(romdata->file);
					romdata->file = NULL;
				}
			}
			else
			{
				romp++; /* skip over file */
			}
			/* The prefix belongs to this file, including all of its chunks. */
			romdata->archive_bios = 0;
		}
		else
		{
			romp++;	/* something else; skip */
		}
	}
	return 1;

	/* error case */
fatalerror:
	if (romdata->file)
		mame_fclose(romdata->file);
	romdata->file = NULL;
	return 0;
}


#define MAX_CHD_PARENT_DEPTH 16

static int chd_has_parent(const chd_header *header)
{
	static const UINT8 nullsha1[CHD_SHA1_BYTES] = { 0 };

	if (header->version < 5)
		return (header->flags & CHDFLAGS_HAS_PARENT) != 0;
	return memcmp(nullsha1, header->parentsha1, sizeof(header->parentsha1)) != 0;
}

static chd_file *open_parent_disk_image_recursive(const game_driver *gamedrv, const chd_header *child_header, const chd_header *visited_headers, int depth)
{
	static const UINT8 nullmd5[CHD_MD5_BYTES] = { 0 };
	static const UINT8 nullsha1[CHD_SHA1_BYTES] = { 0 };
	const game_driver *drv;
	int check_sha1;
	int check_md5;
	int i;

	if (depth >= MAX_CHD_PARENT_DEPTH)
		return NULL;

	check_sha1 = (memcmp(nullsha1, child_header->parentsha1, CHD_SHA1_BYTES) != 0);
	check_md5 = (memcmp(nullmd5, child_header->parentmd5, CHD_MD5_BYTES) != 0);

	/* detect cycles in the parent chain */
	for (i = 0; i < depth; i++)
	{
		if (check_sha1 && (memcmp(visited_headers[i].sha1, child_header->parentsha1, CHD_SHA1_BYTES) == 0 ||
		                   (visited_headers[i].version >= 4 && memcmp(visited_headers[i].rawsha1, child_header->parentsha1, CHD_SHA1_BYTES) == 0)))
			return NULL;
		if (!check_sha1 && check_md5 && memcmp(visited_headers[i].md5, child_header->parentmd5, CHD_MD5_BYTES) == 0)
			return NULL;
	}

	for (drv = gamedrv; drv != NULL; drv = driver_get_clone(drv))
	{
		const rom_entry *region, *rom;

		for (region = rom_first_region(drv); region; region = rom_next_region(region))
		{
			if (!ROMREGION_ISDISKDATA(region))
				continue;

			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				char parentname[1024];
				mame_file *pfile;
				chd_header pheader;

				sprintf(parentname, "%s.chd", ROM_GETNAME(rom));
				pfile = open_disk_image_file(drv, parentname);
				if (!pfile)
					continue;

				if (chd_read_header_core_file_callbacks(&mame_core_file_callbacks, pfile, &pheader) == CHDERR_NONE)
				{
					int match = 0;
					if (check_sha1 && (memcmp(pheader.sha1, child_header->parentsha1, CHD_SHA1_BYTES) == 0 ||
					                   (pheader.version >= 4 && memcmp(pheader.rawsha1, child_header->parentsha1, CHD_SHA1_BYTES) == 0)))
						match = 1;
					else if (!check_sha1 && check_md5 && memcmp(pheader.md5, child_header->parentmd5, CHD_MD5_BYTES) == 0)
						match = 1;

					if (match)
					{
						chd_file *parent = 0;
						chd_file *grandparent = 0;
						chd_error err;

						if (chd_has_parent(&pheader))
						{
							chd_header next_visited[MAX_CHD_PARENT_DEPTH];
							if (depth > 0)
								memcpy(next_visited, visited_headers, depth * sizeof(chd_header));
							next_visited[depth] = *child_header;
							grandparent = open_parent_disk_image_recursive(drv, &pheader, next_visited, depth + 1);
							if (!grandparent)
							{
								mame_fclose(pfile);
								continue;
							}
						}

						/* the file is still owned by us, so we must close it on failure */
						if (mame_fseek(pfile, 0, SEEK_SET) != 0)
						{
							if (grandparent)
								chd_close(grandparent);
							mame_fclose(pfile);
							continue;
						}

						err = chd_open_core_file_callbacks(&mame_core_file_callbacks, pfile, CHD_OPEN_READ, grandparent, &parent);

						/* libchdr owns pfile from now on, even on failure, and it must never be closed or reused here */
						pfile = 0;

						if (err == CHDERR_NONE)
							return parent;

						continue;
					}
				}
				mame_fclose(pfile);
			}
		}
	}
	return NULL;
}

static chd_file *open_parent_disk_image(const game_driver *gamedrv, const chd_header *child_header)
{
	return open_parent_disk_image_recursive(gamedrv, child_header, NULL, 0);
}


/*-------------------------------------------------
    process_disk_entries - process all disk entries
    for a region
-------------------------------------------------*/

static int process_disk_entries(rom_load_data *romdata, const rom_entry *romp)
{
	static const UINT8 nullmd5[CHD_MD5_BYTES] = { 0 };
	static const UINT8 nullsha1[CHD_SHA1_BYTES] = { 0 };

	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* handle files */
		if (ROMENTRY_ISFILE(romp))
		{
			chd_file *source = NULL;
			chd_header child_header;
			chd_file *parent = NULL;
			const chd_header *header;
			mame_file *file;
			char filename[1024];
			char acthash[HASH_BUF_SIZE];
			chd_error err;

			/* make the filename of the source */
			sprintf(filename, "%s.chd", ROM_GETNAME(romp));

			/* first open the source drive */
			debugload("Opening disk image: %s\n", filename);
			file = open_disk_image_file(Machine->gamedrv, filename);
			if (!file)
			{
				handle_missing_file(romdata, romp);
				romp++;
				continue;
			}

			err = chd_read_header_core_file_callbacks(&mame_core_file_callbacks, file, &child_header);
			if (err == CHDERR_NONE)
			{
				if (chd_has_parent(&child_header))
					parent = open_parent_disk_image(Machine->gamedrv, &child_header);

				mame_fseek(file, 0, SEEK_SET);
				err = chd_open_core_file_callbacks(&mame_core_file_callbacks, file, CHD_OPEN_READ, parent, &source);
			}
			else
			{
				mame_fclose(file);
			}

			if (err != CHDERR_NONE || !source)
			{
				if (err == CHDERR_UNSUPPORTED_VERSION)
					sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION\n", filename);
				else
					sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s CHD ERROR: %s\n", filename, chd_error_string(err));
				romdata->errors++;
				romp++;
				continue;
			}

			/* get the header and extract the MD5/SHA1 */
			header = chd_get_header(source);
			hash_data_clear(acthash);
			if (header->version <= 3 && memcmp(nullmd5, header->md5, CHD_MD5_BYTES) != 0)
				hash_data_insert_binary_checksum(acthash, HASH_MD5, header->md5);
			if (header->version >= 3 && memcmp(nullsha1, header->sha1, CHD_SHA1_BYTES) != 0)
				hash_data_insert_binary_checksum(acthash, HASH_SHA1, header->sha1);

			/* verify the checksum; for CHD v4-v5 also check rawsha1 against legacy driver definitions */
			if (!hash_data_is_equal(ROM_GETHASHDATA(romp), acthash, 0) &&
				header->version >= 4 && memcmp(nullsha1, header->rawsha1, CHD_SHA1_BYTES) != 0)
			{
				char rawhash[HASH_BUF_SIZE];
				hash_data_clear(rawhash);
				hash_data_insert_binary_checksum(rawhash, HASH_SHA1, header->rawsha1);
				if (hash_data_is_equal(ROM_GETHASHDATA(romp), rawhash, 0))
					hash_data_copy(acthash, rawhash);
			}

			if (hash_data_has_info(ROM_GETHASHDATA(romp), HASH_INFO_NO_DUMP))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s NO GOOD DUMP KNOWN\n", filename);
				romdata->warnings++;
			}
			else if (!hash_data_is_equal(ROM_GETHASHDATA(romp), acthash, 0))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s WRONG CHECKSUMS:\n", filename);
				dump_wrong_and_correct_checksums(romdata, ROM_GETHASHDATA(romp), acthash);
				romdata->warnings++;
			}
			else if (hash_data_has_info(ROM_GETHASHDATA(romp), HASH_INFO_BAD_DUMP))
			{
				sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s CHD NEEDS REDUMP\n", filename);
				romdata->warnings++;
			}

			/* if not read-only, open or create the CHD v5 diff file */
			if (!DISK_ISREADONLY(romp))
			{
				chd_write_file *diff = NULL;
				mame_file *dfile;
				osd_file_error ferr = FILEERR_SUCCESS;

				/* make the filename of the diff */
				sprintf(filename, "%s.dif", ROM_GETNAME(romp));

				/* try to open an existing diff */
				debugload("Opening differencing image: %s\n", filename);
				dfile = mame_fopen_error(NULL, filename, FILETYPE_IMAGE_DIFF, 0, &ferr);
				if (dfile)
				{
					err = chd_open_diff_file(dfile, source, &diff);
					if (err != CHDERR_NONE || !diff)
					{
						if (err == CHDERR_UNSUPPORTED_VERSION)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION (old diff file, delete it to recreate)\n", filename);
						else if (err == CHDERR_INVALID_PARENT || err == CHDERR_REQUIRES_PARENT)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: DIFF FILE DOES NOT MATCH THE CHD (created for a different CHD, delete it to recreate)\n", filename);
						else if (err == CHDERR_INVALID_FILE || err == CHDERR_READ_ERROR)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: DIFF FILE CORRUPTED (delete it to recreate)\n", filename);
						else
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T OPEN DIFF FILE\n", filename);
						goto diff_error;
					}
				}
				else if (ferr != FILEERR_NOT_FOUND)
				{
					sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T OPEN DIFF FILE\n", filename);
					goto diff_error;
				}
				else
				{
					/* validate parent before creating the diff file on disk */
					err = chd_validate_parent(source);
					if (err != CHDERR_NONE)
					{
						if (err == CHDERR_UNSUPPORTED_VERSION)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION\n", filename);
						else
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T CREATE DIFF FILE\n", filename);
						goto diff_error;
					}

					/* create a new CHD v5 diff file */
					debugload("Creating differencing image: %s\n", filename);
					dfile = mame_fopen_error(NULL, filename, FILETYPE_IMAGE_DIFF, 1, &ferr);
					if (!dfile)
					{
						sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T CREATE DIFF FILE\n", filename);
						goto diff_error;
					}

					err = chd_create_diff_file(dfile, source, &diff);
					if (err != CHDERR_NONE || !diff)
					{
						if (err == CHDERR_UNSUPPORTED_VERSION)
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s UNSUPPORTED CHD VERSION\n", filename);
						else
							sprintf(&romdata->errorbuf[strlen(romdata->errorbuf)], "%-12s: CAN'T CREATE DIFF FILE\n", filename);
						goto diff_error;
					}
				}

				chd_register_writable(source, diff);
				disk_diff_handle[DISK_GETINDEX(romp)] = diff;
			}

			/* we're okay, set the handle */
			debugload("Assigning to handle %d\n", DISK_GETINDEX(romp));
			disk_handle[DISK_GETINDEX(romp)] = source;
			romp++;
			continue;

		diff_error:
			chd_close(source);
			romdata->errors++;
			romp++;
			continue;
		}
	}
	return 1;
}


/*-------------------------------------------------
    rom_init - new, more flexible ROM
    loading system
-------------------------------------------------*/

int rom_init(const rom_entry *romp)
{
	const rom_entry *regionlist[REGION_MAX];
	const rom_entry *region;
	static rom_load_data romdata;
	int regnum;
	char error[256];

	if (rom_validate_archive_bios(Machine->gamedrv, error, sizeof(error)))
		fatalerror("%s", error);

	/* if no roms, bail */
	if (romp == NULL)
		return 0;

	/* reset the region list */
	memset((void *)regionlist, 0, sizeof(regionlist));

	/* reset the romdata struct */
	memset(&romdata, 0, sizeof(romdata));
	romdata.romstotal = count_roms(romp);

	/* reset the disk list */
	memset(disk_handle, 0, sizeof(disk_handle));
	memset(disk_diff_handle, 0, sizeof(disk_diff_handle));

	/* determine the correct biosset to load based on options.bios string */
	system_bios = determine_bios_rom(Machine->gamedrv->bios, Machine->gamedrv->rom);
	/* Partial allocation failures must also release regions and disk handles. */
	add_exit_callback(rom_exit);

	/* loop until we hit the end */
	for (region = rom_first_region(Machine->gamedrv), regnum = 0; region; region = rom_next_region(region), regnum++)
	{
		int regiontype = ROMREGION_GETTYPE(region);
		romdata.archive_bios = 0;
		romdata.destination = regiontype;

		debugload("Processing region %02X (length=%X)\n", regiontype, ROMREGION_GETLENGTH(region));

		/* the first entry must be a region */
		if (!ROMENTRY_ISREGION(region))
		{
			printf("Error: missing ROM_REGION header\n");
			return 1;
		}

		/* allocate memory for the region */
		if (new_memory_region(regiontype, ROMREGION_GETLENGTH(region), ROMREGION_GETFLAGS(region)) != 0)
		{
			printf("Error: unable to allocate memory for region %d\n", regiontype);
			return 1;
		}

		/* remember the base and length */
		romdata.regionlength = memory_region_length(regiontype);
		romdata.regionbase = memory_region(regiontype);
		debugload("Allocated %X bytes @ %08X\n", romdata.regionlength, (int)romdata.regionbase);

		/* clear the region if it's requested */
		if (ROMREGION_ISERASE(region))
			memset(romdata.regionbase, ROMREGION_GETERASEVAL(region), romdata.regionlength);

		/* or if it's sufficiently small (<= 4MB) */
		else if (romdata.regionlength <= 0x400000)
			memset(romdata.regionbase, 0, romdata.regionlength);

#ifdef MAME_DEBUG
		/* if we're debugging, fill region with random data to catch errors */
		else
			fill_random(romdata.regionbase, romdata.regionlength);
#endif

		/* now process the entries in the region */
		if (ROMREGION_ISROMDATA(region))
		{
			if (!process_rom_entries(&romdata, region + 1))
				return 1;
		}
		else if (ROMREGION_ISDISKDATA(region))
		{
			if (!process_disk_entries(&romdata, region + 1))
				return 1;
		}

		/* add this region to the list */
		if (regiontype < REGION_MAX)
			regionlist[regiontype] = region;
	}

	/* post-process the regions */
	for (regnum = 0; regnum < REGION_MAX; regnum++)
		if (regionlist[regnum])
		{
			debugload("Post-processing region %02X\n", regnum);
			romdata.regionlength = memory_region_length(regnum);
			romdata.regionbase = memory_region(regnum);
			region_post_process(&romdata, regionlist[regnum]);
		}

	/* display the results and exit */
	total_rom_load_warnings = romdata.warnings;

	return display_rom_load_results(&romdata);
}


/*-------------------------------------------------
    rom_exit - clean up after ourselves
-------------------------------------------------*/

void rom_exit(void)
{
	int i;

	/* free the memory allocated for various regions */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		free_memory_region(i);

	/* close all hard drives */
	for (i = 0; i < sizeof(disk_handle) / sizeof(disk_handle[0]); i++)
	{
		if (disk_diff_handle[i])
		{
			chd_close_diff(disk_diff_handle[i]);
			disk_diff_handle[i] = NULL;
		}
		if (disk_handle[i])
		{
			chd_close(disk_handle[i]);
			disk_handle[i] = NULL;
		}
	}
}


int rom_load_warnings(void)
{
	return total_rom_load_warnings;
}
