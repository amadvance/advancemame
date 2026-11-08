// license:BSD-3-Clause
// copyright-holders:Nathan Woods, Raphael Nabet, Miodrag Milanovic
/*********************************************************************

	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in
	the MESS image code.  Additionally, the need for diff files comes
	from MAME's need for "cannonical" hard drive images.

	Raphael Nabet 2003

	Update: 23-Feb-2004 - Unlike floppy disks, for which we support
	myriad formats on many systems, it is my intention for MESS to
	standardize on the CHD file format for hard drives so I made a few
	changes to support this

*********************************************************************/

#include "harddriv.h"

#define MAX_HARDDISKS	8

static hard_disk_file *drive_handles[MAX_HARDDISKS];



static OPTION_GUIDE_START(mess_hd_option_guide)
	OPTION_INT('C', "cylinders",		"Cylinders")
	OPTION_INT('H', "heads",			"Heads")
	OPTION_INT('S', "sectors",			"Sectors")
	OPTION_INT('L', "sectorlength",		"Sector Bytes")
	OPTION_INT('K', "hunksize",			"Hunk Bytes")
OPTION_GUIDE_END

static const char *mess_hd_option_spec =
	"C1-[512]-1024;H1/2/[4]/8;S1-[16]-64;L128/256/[512]/1024;K512/1024/2048/[4096]";


#define MESSHDTAG "mess_hd"

struct mess_hd
{
	hard_disk_file *hard_disk_handle;
	chd_file *chd;
	chd_write_file *writable;
};

static struct mess_hd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSHDTAG);
}






/*************************************
 *
 *	device_init_mess_hd()
 *
 *	Device init
 *
 *************************************/

int device_init_mess_hd(mess_image *image)
{
	struct mess_hd *hd;

	hd = image_alloctag(image, MESSHDTAG, sizeof(struct mess_hd));
	if (!hd)
		return INIT_FAIL;

	hd->hard_disk_handle = NULL;
	hd->chd = NULL;
	hd->writable = NULL;

	return INIT_PASS;
}



/*************************************
 *
 *	device_load_mess_hd()
 *  device_create_mess_hd()
 *
 *	Device load and create
 *
 *************************************/

static int internal_load_mess_hd(mess_image *image, const char *metadata)
{
	chd_error err = CHDERR_NONE;
	struct mess_hd *hd;
	chd_file *chd = NULL;
	chd_write_file *writable = NULL;
	int id = image_index_in_device(image);

	hd = get_drive(image);
	hd->hard_disk_handle = NULL;
	hd->chd = NULL;
	hd->writable = NULL;

	/* if the image was opened read-write, try opening it as an uncompressed writable CHD v5 */
	if (image_is_writable(image))
	{
		err = chd_open_uncompressed_v5_file(image_fp(image), &writable);
		if (err != CHDERR_NONE)
		{
			/* compressed or older CHD versions cannot be modified in-place; fall back to read-only */
			if (err == CHDERR_FILE_NOT_WRITEABLE || err == CHDERR_UNSUPPORTED_VERSION)
			{
				if (metadata)
					goto error;
				image_make_readonly(image);
				err = CHDERR_NONE;
			}
			else
			{
				goto error;
			}
		}
	}

	/* if we created the image and hence have metadata to set, write the metadata */
	if (metadata)
	{
		if (!writable)
		{
			err = CHDERR_FILE_NOT_WRITEABLE;
			goto error;
		}
		err = chd_diff_write_metadata(writable, HARD_DISK_METADATA_TAG, 0, metadata, (UINT32)strlen(metadata) + 1, CHD_MDFLAGS_CHECKSUM);
		if (err != CHDERR_NONE)
			goto error;
	}

	/* open the read-only libchdr handle as well */
	err = chd_open_core_file_callbacks(&mame_core_file_callbacks_no_close, image_fp(image), CHD_OPEN_READ, NULL, &chd);
	if (err != CHDERR_NONE)
		goto error;

	/* open the hard disk file */
	hd->hard_disk_handle = hard_disk_open_diff(chd, writable);
	if (!hd->hard_disk_handle)
		goto error;

	hd->chd = chd;
	hd->writable = writable;
	drive_handles[id] = hd->hard_disk_handle;

	return INIT_PASS;

error:
	if (writable)
		chd_close_diff(writable);
	if (chd)
		chd_close(chd);

	if (err != CHDERR_NONE)
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_error_string(err));
	return INIT_FAIL;
}



int device_load_mess_hd(mess_image *image, mame_file *file)
{
	(void)file;
	return internal_load_mess_hd(image, NULL);
}



static int device_create_mess_hd(mess_image *image, mame_file *file, int create_format, option_resolution *create_args)
{
	chd_error err;
	chd_write_file *writable = NULL;
	char metadata[256];
	UINT32 sectorsize, hunksize;
	UINT32 cylinders, heads, sectors, totalsectors;

	(void)file;
	(void)create_format;

	cylinders	= option_resolution_lookup_int(create_args, 'C');
	heads		= option_resolution_lookup_int(create_args, 'H');
	sectors		= option_resolution_lookup_int(create_args, 'S');
	sectorsize	= option_resolution_lookup_int(create_args, 'L');
	hunksize	= option_resolution_lookup_int(create_args, 'K');

	totalsectors = cylinders * heads * sectors;

	/* create the uncompressed CHD v5 file */
	err = chd_create_uncompressed_v5_file(image_fp(image), (UINT64)totalsectors * (UINT64)sectorsize, hunksize, sectorsize, &writable);
	if (err != CHDERR_NONE)
	{
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_error_string(err));
		return INIT_FAIL;
	}
	chd_close_diff(writable);

	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	return internal_load_mess_hd(image, metadata);
}



/*************************************
 *
 *	device_unload_mess_hd()
 *
 *	Device unload
 *
 *************************************/

void device_unload_mess_hd(mess_image *image)
{
	struct mess_hd *hd = get_drive(image);
	assert(hd->hard_disk_handle);
	hard_disk_close(hd->hard_disk_handle);
	hd->hard_disk_handle = NULL;
	if (hd->writable)
	{
		chd_close_diff(hd->writable);
		hd->writable = NULL;
	}
	if (hd->chd)
	{
		chd_close(hd->chd);
		hd->chd = NULL;
	}
	drive_handles[image_index_in_device(image)] = NULL;
}



/*************************************
 *
 *	Get the MESS/MAME hard disk handle (from the mame/harddisk.c core)
 *  after an image has been opened with the mess_hd core
 *
 *************************************/

hard_disk_file *mess_hd_get_hard_disk_file(mess_image *image)
{
	struct mess_hd *hd = get_drive(image);
	return hd->hard_disk_handle;
}



/*************************************
 *
 *	Get the MESS/MAME CHD file (from the mame/chd.c core)
 *  after an image has been opened with the mess_hd core
 *
 *************************************/

chd_file *mess_hd_get_chd_file(mess_image *image)
{
	chd_file *result = NULL;
	hard_disk_file *hd_file;

	if (image)
	{
		hd_file = mess_hd_get_hard_disk_file(image);
		if (hd_file)
			result = hard_disk_get_chd(hd_file);
	}
	return result;
}



/*************************************
 *
 *	Device specification function
 *
 *************************************/

void harddisk_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:						info->i = IO_HARDDISK; break;
		case DEVINFO_INT_READABLE:					info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:					info->i = 1; break;
		case DEVINFO_INT_CREATABLE:					info->i = 1; break;
		case DEVINFO_INT_CREATE_OPTCOUNT:			info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:						info->init = device_init_mess_hd; break;
		case DEVINFO_PTR_LOAD:						info->load = device_load_mess_hd; break;
		case DEVINFO_PTR_CREATE:					info->create = device_create_mess_hd; break;
		case DEVINFO_PTR_UNLOAD:					info->unload = device_unload_mess_hd; break;
		case DEVINFO_PTR_CREATE_OPTGUIDE:			info->p = (void *) mess_hd_option_guide; break;
		case DEVINFO_PTR_CREATE_OPTSPEC+0:			info->p = (void *) mess_hd_option_spec; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "chd,hd"); break;
		case DEVINFO_STR_CREATE_OPTNAME+0:			strcpy(info->s = device_temp_str(), "chd"); break;
		case DEVINFO_STR_CREATE_OPTDESC+0:			strcpy(info->s = device_temp_str(), "MAME/MESS CHD Hard drive"); break;
		case DEVINFO_STR_CREATE_OPTEXTS+0:			strcpy(info->s = device_temp_str(), "chd\0hd\0"); break;
	}
}

hard_disk_file *mess_hd_get_hard_disk_file_by_number(int drivenum)
{
	if ((drivenum < 0) || (drivenum >= MAX_HARDDISKS))
	{
		return NULL;
	}

	return drive_handles[drivenum];
}
