// license:BSD-3-Clause
// copyright-holders:Nathan Woods, R. Belmont, Miodrag Milanovic
/*********************************************************************

	Code to interface the MESS image code with MAME's CHD-CD core.

	Based on harddriv.c by Raphael Nabet 2003

*********************************************************************/

#include "chd_cd.h"
#include "chdwrite.h"

#define MAX_CDROMS	(4)	// up to this many drives

static cdrom_file *drive_handles[MAX_CDROMS];



static OPTION_GUIDE_START(mess_cd_option_guide)
	OPTION_INT('K', "hunksize",			"Hunk Bytes")
OPTION_GUIDE_END

static const char *mess_cd_option_spec =
	"K512/1024/2048/[4096]";


#define MESSCDTAG "mess_cd"

struct mess_cd
{
	cdrom_file *cdrom_handle;
	chd_file *chd;
};

static struct mess_cd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSCDTAG);
}






/*************************************
 *
 *	device_init_mess_cd()
 *
 *	Device init
 *
 *************************************/

int device_init_mess_cd(mess_image *image)
{
	struct mess_cd *cd;

	cd = image_alloctag(image, MESSCDTAG, sizeof(struct mess_cd));
	if (!cd)
		return INIT_FAIL;

	cd->cdrom_handle = NULL;
	cd->chd = NULL;

	return INIT_PASS;
}



/*************************************
 *
 *	device_load_mess_cd()
 *  	device_create_mess_cd()
 *
 *	Device load and create
 *
 *************************************/

static int internal_load_mess_cd(mess_image *image, const char *metadata)
{
	chd_error err = CHDERR_NONE;
	struct mess_cd *cd;
	chd_file *chd = NULL;
	int id = image_index_in_device(image);

	(void)metadata;
	cd = get_drive(image);
	cd->chd = NULL;

	/* open the CHD file */
	err = chd_open_core_file_callbacks(&mame_core_file_callbacks_no_close, image_fp(image), CHD_OPEN_READ, NULL, &chd);
	if (err != CHDERR_NONE)
		goto error;

	/* open the CD-ROM file */
	cd->cdrom_handle = cdrom_open(chd);
	if (!cd->cdrom_handle)
		goto error;

	cd->chd = chd;
	drive_handles[id] = cd->cdrom_handle;
	return INIT_PASS;

error:
	if (chd)
		chd_close(chd);

	if (err != CHDERR_NONE)
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_error_string(err));
	return INIT_FAIL;
}



int device_load_mess_cd(mess_image *image, mame_file *file)
{
	return internal_load_mess_cd(image, NULL);
}



static int device_create_mess_cd(mess_image *image, mame_file *file, int create_format, option_resolution *create_args)
{
	return INIT_FAIL;   	// cd-roms are not writable
}



/*************************************
 *
 *	device_unload_mess_cd()
 *
 *	Device unload
 *
 *************************************/

void device_unload_mess_cd(mess_image *image)
{
	struct mess_cd *cd = get_drive(image);
	assert(cd->cdrom_handle);
	cdrom_close(cd->cdrom_handle);
	cd->cdrom_handle = NULL;
	if (cd->chd)
	{
		chd_close(cd->chd);
		cd->chd = NULL;
	}
	drive_handles[image_index_in_device(image)] = NULL;
}



/*************************************
 *
 *  Get the MESS/MAME cdrom handle (from the mame/cdrom.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

cdrom_file *mess_cd_get_cdrom_file(mess_image *image)
{
	struct mess_cd *cd = get_drive(image);
	return cd->cdrom_handle;
}



/*************************************
 *
 *	Get the MESS/MAME CHD file (from the mame/chd.c core)
 *  after an image has been opened with the mess_cd core
 *
 *************************************/

chd_file *mess_cd_get_chd_file(mess_image *image)
{
	chd_file *result = NULL;
	cdrom_file *cd_file;

	if (image)
	{
		cd_file = mess_cd_get_cdrom_file(image);
		if (cd_file)
			result = cdrom_get_chd(cd_file);
	}
	return result;
}



/*************************************
 *
 *	Device specification function
 *
 *************************************/

void cdrom_device_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TYPE:						info->i = IO_CDROM; break;
		case DEVINFO_INT_READABLE:					info->i = 1; break;
		case DEVINFO_INT_WRITEABLE:					info->i = 0; break;
		case DEVINFO_INT_CREATABLE:					info->i = 0; break;
		case DEVINFO_INT_CREATE_OPTCOUNT:			info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_INIT:						info->init = device_init_mess_cd; break;
		case DEVINFO_PTR_LOAD:						info->load = device_load_mess_cd; break;
		case DEVINFO_PTR_CREATE:					info->create = device_create_mess_cd; break;
		case DEVINFO_PTR_UNLOAD:					info->unload = device_unload_mess_cd; break;
		case DEVINFO_PTR_CREATE_OPTGUIDE:			info->p = (void *) mess_cd_option_guide; break;
		case DEVINFO_PTR_CREATE_OPTSPEC+0:			info->p = (void *) mess_cd_option_spec; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "chd"); break;
		case DEVINFO_STR_CREATE_OPTNAME+0:			strcpy(info->s = device_temp_str(), "chdcd"); break;
		case DEVINFO_STR_CREATE_OPTDESC+0:			strcpy(info->s = device_temp_str(), "MAME/MESS CHD CD-ROM drive"); break;
		case DEVINFO_STR_CREATE_OPTEXTS+0:			strcpy(info->s = device_temp_str(), "chd\0"); break;
	}
}

cdrom_file *mess_cd_get_cdrom_file_by_number(int drivenum)
{
	if ((drivenum < 0) || (drivenum > MAX_CDROMS))
	{
		return NULL;
	}

	return drive_handles[drivenum];
}
