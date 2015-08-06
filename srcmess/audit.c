/***************************************************************************

    audit.c

    ROM set auditing functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "hash.h"
#include "audit.h"
#include "harddisk.h"
#include "sound/samples.h"

static audit_record *audit_records = NULL;

static const game_driver *chd_gamedrv;

/*-------------------------------------------------
    audit_chd_open - interface for opening
    a hard disk image
-------------------------------------------------*/

chd_interface_file *audit_chd_open(const char *filename, const char *mode)
{
	const game_driver *drv;

	/* attempt reading up the chain through the parents */
	for (drv = chd_gamedrv; drv != NULL; drv = driver_get_clone(drv))
	{
		void* file = mame_fopen(drv->name, filename, FILETYPE_IMAGE, 0);

		if (file != NULL)
			return file;
	}

	return NULL;
}



/*-------------------------------------------------
    audit_chd_close - interface for closing
    a hard disk image
-------------------------------------------------*/

void audit_chd_close(chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}



/*-------------------------------------------------
    audit_chd_read - interface for reading
    from a hard disk image
-------------------------------------------------*/

UINT32 audit_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



/*-------------------------------------------------
    audit_chd_write - interface for writing
    to a hard disk image
-------------------------------------------------*/

UINT32 audit_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	return 0;
}


/*-------------------------------------------------
    audit_chd_length - interface for getting
    the length a hard disk image
-------------------------------------------------*/

UINT64 audit_chd_length(chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}



static chd_interface audit_chd_interface =
{
	audit_chd_open,
	audit_chd_close,
	audit_chd_read,
	audit_chd_write,
	audit_chd_length
};


/* returns 1 if rom is defined in this set */
int audit_is_rom_used (const game_driver *gamedrv, const char* hash)
{
	const rom_entry *region, *rom;

	for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			/* Compare all the available checksums */
			if (hash_data_is_equal(ROM_GETHASHDATA(rom), hash, 0))
				return 1;

	return 0;
}


/* returns nonzero if romset is missing */
int audit_has_missing_roms (int game)
{
	const game_driver *gamedrv = drivers[game];
	const game_driver *clone_of = driver_get_clone(gamedrv);

	if (clone_of != NULL)
	{
		audit_record	*aud;
		int				count;
		int 			i;

#if 1
        int cloneRomsFound = 0;
        int uniqueRomsFound = 0;

		if ((count = audit_roms (game, &aud)) == 0)
			return 1;

		if (count == -1) return 0;

        /* count number of roms found that are unique to clone */
        for (i = 0; i < count; i++)
			if (!audit_is_rom_used (clone_of, aud[i].exphash))
			{
				uniqueRomsFound++;
				if (aud[i].status != AUD_ROM_NOT_FOUND)
					cloneRomsFound++;
			}
#else
		int cloneRomsFound = 0;

		if ((count = audit_roms (game, &aud)) == 0)
			return 1;

		if (count == -1) return 0;

		/* count number of roms found that are unique to clone */
		for (i = 0; i < count; i++)
			if (aud[i].status != AUD_ROM_NOT_FOUND)
				if (!audit_is_rom_used (clone_of, aud[i].exphash))
					cloneRomsFound++;
#endif

		return !cloneRomsFound;
	}
	else
		return !mame_faccess (gamedrv->name, FILETYPE_ROM);
}

/* Fills in an audit record for each rom in the romset. Sets 'audit' to
   point to the list of audit records. Returns total number of roms
   in the romset (same as number of audit records), 0 if romset missing. */
int audit_roms (int game, audit_record **audit)
{
	const rom_entry *region, *rom, *chunk;
	const char *name;
	const game_driver *gamedrv;
	const game_driver *clone_of;

	int count = 0;
	audit_record *aud;
	int	err;

	if (!audit_records)
	{
		audit_records = (audit_record *)malloc (AUD_MAX_ROMS * sizeof (audit_record));

		// Make sure the memory is cleared - it's needed by the hashing
		//  engine
		memset(audit_records, 0, AUD_MAX_ROMS * sizeof(audit_record));
	}

	if (audit_records)
		*audit = aud = audit_records;
	else
		return 0;

	gamedrv = drivers[game];
	clone_of = driver_get_clone(gamedrv);

	if (!gamedrv->rom) return -1;

	/* check for existence of romset */
	if (!mame_faccess (gamedrv->name, FILETYPE_ROM))
	{
		/* if the game is a clone, check for parent */
		if (clone_of == NULL || (clone_of->flags & NOT_A_DRIVER) ||
				!mame_faccess(clone_of->name,FILETYPE_ROM))
			return 0;
	}

	for (region = rom_first_region(gamedrv); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			if (ROMREGION_ISROMDATA(region))
			{
				const game_driver *drv;

				name = ROM_GETNAME(rom);
				strcpy (aud->rom, name);
				aud->explength = 0;
				aud->length = 0;
				aud->exphash = ROM_GETHASHDATA(rom);

				/* Copy into the variable we pass to the functions
                   to support load-by-checksum */
				hash_data_copy(aud->hash, aud->exphash);

				count++;

				/* obtain hash checksums and length of ROM file */
				drv = gamedrv;
				do
				{
					err = mame_fchecksum(drv->name, name, &aud->length, aud->hash);
					drv = driver_get_clone(drv);
				} while (err && drv);

				/* spin through ROM_CONTINUEs, totaling length */
				for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
					aud->explength += ROM_GETLENGTH(chunk);

				if (err)
				{
					if (hash_data_has_info(aud->exphash, HASH_INFO_NO_DUMP))
					{
						/* not found but it's not good anyway */
						aud->status = AUD_NOT_AVAILABLE;
					}
					else if (ROM_ISOPTIONAL(rom))
					{
						/* optional ROM not found */
						aud->status = AUD_OPTIONAL_ROM_NOT_FOUND;
					}
					else
					{
						/* not found */
						aud->status = AUD_ROM_NOT_FOUND;

						drv = clone_of;

						/* If missing ROM is also present in a parent set, indicate that */
						while (drv)
						{
							if (audit_is_rom_used (drv, aud->exphash))
							{
								if (drv->flags & NOT_A_DRIVER)
								{
									aud->status = AUD_ROM_NOT_FOUND_BIOS;
									break;
								}
								else
									aud->status = AUD_ROM_NOT_FOUND_PARENT;
							}

							// Walk up the inheritance list. If this ROM is a clone of a set which
							// contains a BIOS that is missing, we can correctly mark it as
							// such.
							drv = driver_get_clone(drv);
						}
					}
				}
				/* all cases below assume the ROM was at least found */
				else if (aud->explength != aud->length)
					aud->status = AUD_LENGTH_MISMATCH;
				else if (hash_data_has_info(aud->exphash, HASH_INFO_NO_DUMP))
						aud->status = AUD_ROM_NEED_DUMP; /* new case - found but not known to be dumped */
				else if (!hash_data_is_equal(aud->exphash, aud->hash, 0))
				{
					/* non-matching hash */
						aud->status = AUD_BAD_CHECKSUM;
				}
				else
				{
					/* matching hash */
					if (hash_data_has_info(aud->exphash, HASH_INFO_BAD_DUMP))
						aud->status = AUD_ROM_NEED_REDUMP;
					else
					aud->status = AUD_ROM_GOOD;
				}

				aud++;
			}
			else if (ROMREGION_ISDISKDATA(region))
			{
				const UINT8 nullhash[HASH_BUF_SIZE] = { 0 };
				void *source;
				chd_header header;

				name = ROM_GETNAME(rom);
				strcpy (aud->rom, name);
				aud->explength = 0;
				aud->length = 0;
				aud->exphash = ROM_GETHASHDATA(rom);
				hash_data_clear(aud->hash);
				count++;

				chd_gamedrv = gamedrv;
				chd_set_interface(&audit_chd_interface);
				source = chd_open( name, 0, NULL );
				if( source == NULL )
				{
					err = chd_get_last_error();
					if( err == CHDERR_OUT_OF_MEMORY )
					{
						aud->status = AUD_MEM_ERROR;
					}
					else if (hash_data_has_info(aud->exphash, HASH_INFO_NO_DUMP))
					{
						/* not found but it's not good anyway */
						aud->status = AUD_DISK_NOT_AVAILABLE;
					}
					else
					{
						/* not found */
						aud->status = AUD_DISK_NOT_FOUND;
					}
				}
				else
				{
					header = *chd_get_header(source);

					if (memcmp(nullhash, header.md5, sizeof(header.md5)))
						hash_data_insert_binary_checksum(aud->hash, HASH_MD5, header.md5);
					if (memcmp(nullhash, header.sha1, sizeof(header.sha1)))
						hash_data_insert_binary_checksum(aud->hash, HASH_SHA1, header.sha1);

					 if (hash_data_has_info(aud->exphash, HASH_INFO_NO_DUMP))
					{
						aud->status = AUD_DISK_NEED_DUMP;
					}
					else if (!hash_data_is_equal(aud->exphash, aud->hash, 0))
					{
						aud->status = AUD_DISK_BAD_MD5;
					}
					else
					{
						aud->status = AUD_DISK_GOOD;
					}

					chd_close( source );
				}

				aud++;
			}
		}

        #ifdef MESS
        if (!count)
                return -1;
        else
        #endif
	return count;
}

static void verify_dump_hash_data(const char* hash, const char* head, verify_printf_proc verify_printf)
{
	char buf[512];

	verify_printf("\t%s", head ? head : "");
	hash_data_print(hash, 0, buf);
	verify_printf("%s\n", buf);
}


/* Generic function for evaluating a romset. Some platforms may wish to
   call audit_roms() instead and implement their own reporting (like MacMAME). */
int audit_verify_roms (int game, verify_printf_proc verify_printf)
{
	audit_record			*aud;
	int						count;
	int						archive_status = 0;
	const game_driver *gamedrv = drivers[game];
	const game_driver *clone_of = driver_get_clone(gamedrv);

	if ((count = audit_roms (game, &aud)) == 0)
		return NOTFOUND;

	if (count == -1) return CORRECT;

#if 1
    if (clone_of != NULL)
    {
        int i;
        int cloneRomsFound = 0;
        int uniqueRomsFound = 0;

        /* count number of roms found that are unique to clone */
        for (i = 0; i < count; i++)
			if (!audit_is_rom_used (clone_of, aud[i].exphash))
			{
				uniqueRomsFound++;
				if (aud[i].status != AUD_ROM_NOT_FOUND)
					cloneRomsFound++;
			}
        #ifndef MESS
        /* Different MESS systems can use the same ROMs */
        if (uniqueRomsFound && !cloneRomsFound)
            return CLONE_NOTFOUND;
        #endif
    }
#else
	if (clone_of != NULL)
	{
		int i;
		int cloneRomsFound = 0;

		/* count number of roms found that are unique to clone */
		for (i = 0; i < count; i++)
			if (aud[i].status != AUD_ROM_NOT_FOUND)
				if (!audit_is_rom_used (clone_of, aud[i].exphash))
					cloneRomsFound++;

                #ifndef MESS
                /* Different MESS systems can use the same ROMs */
		if (cloneRomsFound == 0)
			return CLONE_NOTFOUND;
                #endif
	}
#endif

	while (count--)
	{
		archive_status |= aud->status;

		switch (aud->status)
		{
			case AUD_ROM_NOT_FOUND:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND\n",
					drivers[game]->name, aud->rom, aud->explength);
				verify_dump_hash_data(aud->exphash, NULL, verify_printf);
				break;
			case AUD_ROM_NOT_FOUND_PARENT:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND (shared with parent)\n",
					drivers[game]->name, aud->rom, aud->explength);
				verify_dump_hash_data(aud->exphash, NULL, verify_printf);
				break;
			case AUD_ROM_NOT_FOUND_BIOS:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND (BIOS)\n",
					drivers[game]->name, aud->rom, aud->explength);
				verify_dump_hash_data(aud->exphash, NULL, verify_printf);
				break;
			case AUD_OPTIONAL_ROM_NOT_FOUND:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND BUT OPTIONAL\n",
					drivers[game]->name, aud->rom, aud->explength);
				verify_dump_hash_data(aud->exphash, NULL, verify_printf);
				break;
			case AUD_NOT_AVAILABLE:
				verify_printf ("%-8s: %-12s %7d bytes NOT FOUND - NO GOOD DUMP KNOWN\n",
					drivers[game]->name, aud->rom, aud->explength);
				break;
			case AUD_ROM_NEED_DUMP:
				verify_printf ("%-8s: %-12s %7d bytes FOUND BUT NO GOOD DUMP KNOWN\n",
					drivers[game]->name, aud->rom, aud->explength);
				break;
			case AUD_BAD_CHECKSUM:
				verify_printf ("%-8s: %-12s %7d bytes INCORRECT CHECKSUM:\n",
					drivers[game]->name, aud->rom, aud->explength);
				verify_dump_hash_data(aud->exphash, "EXPECTED: ", verify_printf);
				verify_dump_hash_data(aud->hash,    "   FOUND: ", verify_printf);
				break;
			case AUD_ROM_NEED_REDUMP:
				verify_printf ("%-8s: %-12s %7d bytes ROM NEEDS REDUMP\n",
					drivers[game]->name, aud->rom, aud->explength);
				break;
			case AUD_MEM_ERROR:
				verify_printf ("Out of memory reading ROM %s\n", aud->rom);
				break;
			case AUD_LENGTH_MISMATCH:
				verify_printf ("%-8s: %-12s %7d bytes INCORRECT LENGTH: %8d\n",
					drivers[game]->name, aud->rom, aud->explength, aud->length);
				verify_dump_hash_data(aud->exphash, NULL, verify_printf);
				break;
			case AUD_ROM_GOOD:
#if 0    /* if you want a full accounting of roms */
				verify_printf ("%-8s: %-12s %7d bytes ROM GOOD\n",
					drivers[game]->name, aud->rom, aud->explength);
				verify_dump_hash_data(aud->hash, NULL, verify_printf);
#endif
				break;
			case AUD_DISK_GOOD:
#if 0    /* if you want a full accounting of roms */
				verify_printf ("%-8s: %-12s GOOD\n",
					drivers[game]->name, aud->rom);
				verify_dump_hash_data(aud->hash, NULL, verify_printf);
#endif
				break;
			case AUD_DISK_NOT_FOUND:
				verify_printf ("%-8s: %-12s NOT FOUND\n",
					drivers[game]->name, aud->rom);
				verify_dump_hash_data(aud->exphash, NULL, verify_printf);
				break;
			case AUD_DISK_BAD_MD5:
				verify_printf ("%-8s: %-12s INCORRECT CHECKSUM:\n",
					drivers[game]->name, aud->rom);
				verify_dump_hash_data(aud->exphash, "EXPECTED: ", verify_printf);
				verify_dump_hash_data(aud->hash,    "   FOUND: ", verify_printf);
				break;
			case AUD_DISK_NOT_AVAILABLE:
				verify_printf ("%-8s: %-12s NO GOOD DUMP KNOWN\n",
					drivers[game]->name, aud->rom);
				break;
			case AUD_DISK_NEED_DUMP:
				verify_printf ("%-8s: %-12s FOUND BUT NO GOOD DUMP KNOWN\n",
					drivers[game]->name, aud->rom);
				verify_dump_hash_data(aud->hash, NULL, verify_printf);
				break;
		}
		aud++;
	}

	if (archive_status & (AUD_ROM_NOT_FOUND|AUD_ROM_NOT_FOUND_PARENT|AUD_ROM_NOT_FOUND_BIOS|AUD_BAD_CHECKSUM|AUD_MEM_ERROR|AUD_LENGTH_MISMATCH|AUD_DISK_NOT_FOUND|AUD_DISK_BAD_MD5))
		return INCORRECT;
	if (archive_status & (AUD_ROM_NEED_DUMP|AUD_ROM_NEED_REDUMP|AUD_NOT_AVAILABLE|AUD_DISK_NOT_AVAILABLE|AUD_DISK_NEED_DUMP))
		return BEST_AVAILABLE;
	if (archive_status & (AUD_OPTIONAL_ROM_NOT_FOUND))
		return MISSING_OPTIONAL;

	return CORRECT;

}


static missing_sample *missing_samples = NULL;

/* Builds a list of every missing sample. Returns total number of missing
   samples, or -1 if no samples were found. Sets audit to point to the
   list of missing samples. */
int audit_samples (int game, missing_sample **audit)
{
	machine_config drv;
	int skipfirst;
	mame_file *f;
	const char **samplenames, *sharedname;
	int exist;
	static const game_driver *gamedrv;
	int j;
	int count = 0;
	missing_sample *aud;

	gamedrv = drivers[game];
	expand_machine_driver(gamedrv->drv, &drv);

	samplenames = NULL;
#if (HAS_SAMPLES || HAS_VLM5030)
	for( j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
	{
#if (HAS_SAMPLES)
		if( drv.sound[j].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[j].config)->samplenames;
#endif
	}
#endif
    /* does the game use samples at all? */
	if (samplenames == 0 || samplenames[0] == 0)
		return 0;

	/* take care of shared samples */
	if (samplenames[0][0] == '*')
	{
		sharedname=samplenames[0]+1;
		skipfirst = 1;
	}
	else
	{
		sharedname = NULL;
		skipfirst = 0;
	}

	/* do we have samples for this game? */
	exist = mame_faccess (gamedrv->name, FILETYPE_SAMPLE);

	/* try shared samples */
	if (!exist && skipfirst)
		exist = mame_faccess (sharedname, FILETYPE_SAMPLE);

	/* if still not found, we're done */
	if (!exist)
		return -1;

	/* allocate missing samples list (if necessary) */
	if (!missing_samples)
		missing_samples = (missing_sample *)malloc (AUD_MAX_SAMPLES * sizeof (missing_sample));

	if (missing_samples)
		*audit = aud = missing_samples;
	else
		return 0;

	for (j = skipfirst; samplenames[j] != 0; j++)
	{
		/* skip empty definitions */
		if (strlen (samplenames[j]) == 0)
			continue;
		f = mame_fopen (gamedrv->name, samplenames[j], FILETYPE_SAMPLE, 0);
		if (f == NULL && skipfirst)
			f = mame_fopen (sharedname, samplenames[j], FILETYPE_SAMPLE, 0);

		if (f)
			mame_fclose(f);
		else
		{
			strcpy (aud->name, samplenames[j]);
			count++;
			aud++;
		}
	}
	return count;
}


/* Generic function for evaluating a sampleset. Some platforms may wish to
   call audit_samples() instead and implement their own reporting (like MacMAME). */
int audit_verify_samples (int game, verify_printf_proc verify_printf)
{
	missing_sample	*aud;
	int				count;

	count = audit_samples (game, &aud);
	if (count==-1)
		return NOTFOUND;
	else if (count==0)
		return CORRECT;

	/* list missing samples */
	while (count--)
	{
		verify_printf ("%-8s: %s NOT FOUND\n", drivers[game]->name, aud->name);
		aud++;
	}

	return INCORRECT;
}
