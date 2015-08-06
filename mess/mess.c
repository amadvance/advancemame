/***************************************************************************

	mess.c

	This file is a set of function calls and defs required for MESS

***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <assert.h>

#include "driver.h"
#include "devices/flopdrv.h"
#include "utils.h"
#include "ui_text.h"
#include "state.h"
#include "image.h"
#include "inputx.h"
#include "artwork.h"
#include "hash.h"

/* Globals */
const char *mess_path;

UINT32 mess_ram_size;
UINT8 *mess_ram;
UINT8 mess_ram_default_value = 0xCD;

static int devices_initialload(const game_driver *gamedrv);
static void devices_exit(void);


static int ram_init(const game_driver *gamedrv)
{
	int i;

	/* validate RAM option */
	if (options.ram != 0)
	{
		if (!ram_is_valid_option(gamedrv, options.ram))
		{
			char buffer[RAM_STRING_BUFLEN];
			int opt_count;

			opt_count = ram_option_count(gamedrv);
			if (opt_count == 0)
			{
				/* this driver doesn't support RAM configurations */
				printf("Driver '%s' does not support RAM configurations\n", gamedrv->name);
			}
			else
			{
				printf("%s is not a valid RAM option for driver '%s' (valid choices are ",
					ram_string(buffer, options.ram), gamedrv->name);
				for (i = 0; i < opt_count; i++)
					printf("%s%s",  i ? " or " : "", ram_string(buffer, ram_option(gamedrv, i)));
				printf(")\n");
			}
			return 1;
		}
		mess_ram_size = options.ram;
	}
	else
	{
		/* none specified; chose default */
		mess_ram_size = ram_default(gamedrv);
	}
	/* if we have RAM, allocate it */
	if (mess_ram_size > 0)
	{
		mess_ram = (UINT8 *) auto_malloc(mess_ram_size);
		memset(mess_ram, mess_ram_default_value, mess_ram_size);

		state_save_register_item("mess", 0, mess_ram_size);
		state_save_register_item_pointer("mess", 0, mess_ram, mess_ram_size);
	}
	else
	{
		mess_ram = NULL;
	}
	return 0;
}



/*****************************************************************************
 *  --Initialise Devices--
 *  Call the init() functions for all devices of a driver
 *  ith all user specified image names.
 ****************************************************************************/

int devices_init(const game_driver *gamedrv)
{
	int i;
	const struct IODevice *dev;

	/* convienient place to call this */
	{
		const char *cwd;
		char *s;

		cwd = osd_get_cwd();
		s = auto_malloc(strlen(cwd) + 1);
		strcpy(s, cwd);
		mess_path = s;
	}

	/* initialize natural keyboard support */
	inputx_init();

	/* allocate the IODevice struct */
	Machine->devices = devices_allocate(Machine->gamedrv);
	if (!Machine->devices)
		return 1;

	/* Check that the driver supports all devices requested (options struct)*/
	for( i = 0; i < options.image_count; i++ )
	{
		if (options.image_files[i].device_tag)
			dev = device_find_tag(Machine->devices, options.image_files[i].device_tag);
		else
			dev = device_find(Machine->devices, options.image_files[i].device_type);

		if (!dev)
		{
			printf(" ERROR: Device [%s] is not supported by this system\n", device_typename(options.image_files[i].device_type));
			return 1;
		}
	}

	/* initialize RAM code */
	if (ram_init(gamedrv))
		return 1;

	/* init all devices */
	image_init();
	add_exit_callback(devices_exit);
	return devices_initialload(gamedrv);
}



static int devices_initialload(const game_driver *gamedrv)
{
	int i;
	int id;
	int result = INIT_FAIL;
	int devcount;
	int *allocated_slots;
	const struct IODevice *dev;
	const char *image_name;
	mess_image *image;
	iodevice_t devtype;
	const char *devtag;
	int devindex;

	/* count number of devices, and record a list of allocated slots */
	devcount = 0;
	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		devcount++;
	if (devcount > 0)
	{
		allocated_slots = malloc(devcount * sizeof(*allocated_slots));
		if (!allocated_slots)
			goto error;
		memset(allocated_slots, 0, devcount * sizeof(*allocated_slots));
	}
	else
	{
		allocated_slots = NULL;
	}

	/* distribute images to appropriate devices */
	for (i = 0; i < options.image_count; i++)
	{
		/* get the image type and filename */
		image_name = options.image_files[i].name;
		image_name = (image_name && image_name[0]) ? image_name : NULL;
		devtype = options.image_files[i].device_type;
		devtag = options.image_files[i].device_tag;
		devindex = options.image_files[i].device_index;

		assert(devtype >= 0);
		assert(devtype < IO_COUNT);

		image = NULL;

		for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		{
			if ((dev->type == devtype) && (!devtag || !strcmp(dev->tag, devtag)))
			{
				if (devindex >= 0)
					id = devindex;
				else
					id = allocated_slots[dev - Machine->devices]++;

				/* try to load this image */
				image = image_from_device_and_index(dev, id);
				result = image_load(image, image_name);
				break;
			}
		}
		if (dev->type >= IO_COUNT)
		{
			if (image)
			{
				printf("Device %s load (%s) failed: %s\n",
					device_typename(devtype),
					osd_basename((char *) image_name),
					image_error(image));
			}
			else
			{
				printf("Too many devices of type %d\n", devtype);
			}
			goto error;
		}
	}

	/* make sure that any required devices have been allocated */
	for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
	{
		if (dev->must_be_loaded)
		{
			for (id = 0; id < dev->count; id++)
			{
				image = image_from_device_and_index(dev, id);
				if (!image_exists(image))
				{
					printf("Driver requires that device %s must have an image to load\n", device_typename(dev->type));
					goto error;
				}
			}
		}
	}
	free(allocated_slots);
	return 0;

error:
	if (allocated_slots)
		free(allocated_slots);
	return 1;
}



/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
static void devices_exit(void)
{
	/* unload all devices */
	image_unload_all(FALSE);
	image_unload_all(TRUE);

	/* exit all devices */
	image_exit();
	Machine->devices = NULL;
}



void showmessdisclaimer(void)
{
	printf(
		"MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		"several computer and console systems. But hardware is useless without software\n"
		"so a file dump of the BIOS, cartridges, discs, and cassettes which run on that\n"
		"hardware is required. Such files, like any other commercial software, are\n"
		"copyrighted material and it is therefore illegal to use them if you don't own\n"
		"the original media from which the files are derived. Needless to say, these\n"
		"files are not distributed together with MESS. Distribution of MESS together\n"
		"with these files is a violation of copyright law and should be promptly\n"
		"reported to the authors so that appropriate legal action can be taken.\n\n");
}



void showmessinfo(void)
{
	printf(
		"M.E.S.S. v%s\n"
		"Multiple Emulation Super System - Copyright (C) 1997-2004 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2004 by Nicola Salmoria and the MAME Team\n\n",
		build_version);

	showmessdisclaimer();

	printf(
		"Usage:  MESS <system> <device> <software> <options>\n\n"
		"        MESS -list        for a brief list of supported systems\n"
		"        MESS -listdevices for a full list of supported devices\n"
		"        MESS -showusage   to see usage instructions\n"
		"See mess.txt for help, readme.txt for options.\n");
}



void ram_dump(const char *filename)
{
	mame_file *file;

	if (!filename)
		filename = "ram.bin";

	file = mame_fopen(Machine->gamedrv->name, filename, FILETYPE_NVRAM, OSD_FOPEN_WRITE);
	if (file)
	{
		mame_fwrite(file, mess_ram, mess_ram_size);

		/* close file */
		mame_fclose(file);
	}
}



void mess_config_init(void)
{
#ifdef WIN32
	extern void win_mess_config_init(void);
	win_mess_config_init();
#endif
}



const game_driver *mess_next_compatible_driver(const game_driver *drv)
{
	if (driver_get_clone(drv))
		drv = driver_get_clone(drv);
	else if (drv->compatible_with && !(drv->compatible_with->flags & NOT_A_DRIVER))
		drv = drv->compatible_with;
	else
		drv = NULL;
	return drv;
}



int mess_count_compatible_drivers(const game_driver *drv)
{
	int count = 0;
	while(drv)
	{
		count++;
		drv = mess_next_compatible_driver(drv);
	}
	return count;
}



UINT32 hash_data_extract_crc32(const char *d)
{
	UINT32 crc = 0;
	UINT8 crc_bytes[4];

	if (hash_data_extract_binary_checksum(d, HASH_CRC, crc_bytes) == 1)
	{
		crc = (((UINT32) crc_bytes[0]) << 24)
			| (((UINT32) crc_bytes[1]) << 16)
			| (((UINT32) crc_bytes[2]) << 8)
			| (((UINT32) crc_bytes[3]) << 0);
	}
	return crc;
}


/***************************************************************************

	Dummy read handlers

***************************************************************************/

READ8_HANDLER( return8_00 )	{ return 0x00; }
READ8_HANDLER( return8_FE )	{ return 0xFE; }
READ8_HANDLER( return8_FF )	{ return 0xFF; }
