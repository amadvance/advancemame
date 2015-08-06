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
#define USE_CHD_OPEN	0

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

static hard_disk_file *drive_handles[MAX_HARDDISKS];

static const char *chd_get_error_string(int chderr)
{
	if ((chderr < 0 ) || (chderr >= (sizeof(error_strings) / sizeof(error_strings[0]))))
		return NULL;
	return error_strings[chderr];
}



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
};

static struct mess_hd *get_drive(mess_image *img)
{
	return image_lookuptag(img, MESSHDTAG);
}



/*************************************
 *
 *  chd_create_ref()/chd_open_ref()
 *
 *  These are a set of wrappers that wrap the chd_open()
 *  and chd_create() functions to provide a way to open
 *  the images with a filename.  This is just a stopgap
 *  measure until I get the core CHD code changed.  For
 *  now, this is an ugly hack but it works very well
 *
 *  When these functions get moved into the core, it will
 *  remove the need to specify an 'open' function in the
 *  CHD interface
 *
 *************************************/

#define ENCODED_IMAGE_REF_PREFIX	"/:/M/E/S/S//i/m/a/g/e//#"
#define ENCODED_IMAGE_REF_FORMAT	(ENCODED_IMAGE_REF_PREFIX "%016x")
#define ENCODED_IMAGE_REF_LEN		(sizeof(ENCODED_IMAGE_REF_PREFIX)+16)


static void encode_ptr(void *ptr, char filename[ENCODED_IMAGE_REF_LEN])
{
	snprintf(filename, ENCODED_IMAGE_REF_LEN, ENCODED_IMAGE_REF_FORMAT,
		(unsigned int) ptr);
}



int chd_create_ref(void *ref, UINT64 logicalbytes, UINT32 hunkbytes, UINT32 compression, chd_file *parent)
{
	char filename[ENCODED_IMAGE_REF_LEN];
	encode_ptr(ref, filename);
	return chd_create(filename, logicalbytes, hunkbytes, compression, parent);
}



chd_file *chd_open_ref(void *ref, int writeable, chd_file *parent)
{
	char filename[ENCODED_IMAGE_REF_LEN];
	encode_ptr(ref, filename);
	return chd_open(filename, writeable, parent);
}



/*************************************
 *
 *	decode_image_ref()
 *
 *	This function will decode an image pointer,
 *	provided one has been encoded in the ASCII
 *	string.
 *
 *************************************/

static mess_image *decode_image_ref(const char encoded_image_ref[ENCODED_IMAGE_REF_LEN])
{
	unsigned int ptr;

	if (sscanf(encoded_image_ref, ENCODED_IMAGE_REF_FORMAT, &ptr) == 1)
		return (mess_image *) ptr;

	return NULL;
}



/*************************************
 *
 *	Interface between MAME's CHD system and MESS's image system
 *
 *************************************/

static chd_interface_file *mess_chd_open(const char *filename, const char *mode);
static void mess_chd_close(chd_interface_file *file);
static UINT32 mess_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mess_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 mess_chd_length(chd_interface_file *file);

static chd_interface mess_hard_disk_interface =
{
	mess_chd_open,
	mess_chd_close,
	mess_chd_read,
	mess_chd_write,
	mess_chd_length
};


static chd_interface_file *mess_chd_open(const char *filename, const char *mode)
{
	mess_image *img = decode_image_ref(filename);

	/* used when experimenting with CHDs */
	if (USE_CHD_OPEN && !img)
		return (chd_interface_file *) mame_fopen(NULL, filename, FILETYPE_IMAGE, 0);

	/* invalid "file name"? */
	assert(img);

	/* read-only fp? */
	if (!image_is_writable(img) && !(mode[0] == 'r' && !strchr(mode, '+')))
		return NULL;

	/* otherwise return file pointer */
	return (chd_interface_file *) image_fp(img);
}



static void mess_chd_close(chd_interface_file *file)
{
}



static UINT32 mess_chd_read(chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



static UINT32 mess_chd_write(chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}



static UINT64 mess_chd_length(chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
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

	chd_set_interface(&mess_hard_disk_interface);

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
	int err = 0;
	struct mess_hd *hd;
	chd_file *chd;
	int is_writeable;
	int id = image_index_in_device(image);

	hd = get_drive(image);

	/* open the CHD file */
	do
	{
		is_writeable = image_is_writable(image);
		chd = chd_open_ref(image, is_writeable, NULL);

		if (!chd)
		{
			err = chd_get_last_error();

			/* special case; if we get CHDERR_FILE_NOT_WRITEABLE, make the
			 * image read only and repeat */
			if (err == CHDERR_FILE_NOT_WRITEABLE)
				image_make_readonly(image);
		}
	}
	while(!chd && is_writeable && (err == CHDERR_FILE_NOT_WRITEABLE));
	if (!chd)
		goto error;

	/* if we created the image and hence, have metadata to set, set the metadata */
	if (metadata)
	{
		err = chd_set_metadata(chd, HARD_DISK_STANDARD_METADATA, 0, metadata, strlen(metadata) + 1);
		if (err != CHDERR_NONE)
			goto error;
	}

	/* open the hard disk file */
	hd->hard_disk_handle = hard_disk_open(chd);
	if (!hd->hard_disk_handle)
		goto error;

	drive_handles[id] = hd->hard_disk_handle;

	return INIT_PASS;

error:
	if (chd)
		chd_close(chd);

	err = chd_get_last_error();
	if (err)
		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_get_error_string(err));
	return INIT_FAIL;
}



int device_load_mess_hd(mess_image *image, mame_file *file)
{
	return internal_load_mess_hd(image, NULL);
}



static int device_create_mess_hd(mess_image *image, mame_file *file, int create_format, option_resolution *create_args)
{
	int err;
	char metadata[256];
	UINT32 sectorsize, hunksize;
	UINT32 cylinders, heads, sectors, totalsectors;

	cylinders	= option_resolution_lookup_int(create_args, 'C');
	heads		= option_resolution_lookup_int(create_args, 'H');
	sectors		= option_resolution_lookup_int(create_args, 'S');
	sectorsize	= option_resolution_lookup_int(create_args, 'L');
	hunksize	= option_resolution_lookup_int(create_args, 'K');

	totalsectors = cylinders * heads * sectors;

	/* create the CHD file */
	err = chd_create_ref(image, (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_NONE, NULL);
	if (err != CHDERR_NONE)
		goto error;

	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	return internal_load_mess_hd(image, metadata);

error:
	return INIT_FAIL;
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
	drive_handles[image_index_in_device(image)] = NULL;
}



/*************************************
 *
 *	Get the MESS/MAME hard disk handle (from the src/harddisk.c core)
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
 *	Get the MESS/MAME CHD file (from the src/chd.c core)
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
		case DEVINFO_PTR_UNLOAD:					info->unload = device_unload_mess_hd; break;
		case DEVINFO_PTR_CREATE_OPTGUIDE:			info->p = (void *) mess_hd_option_guide; break;
		case DEVINFO_PTR_CREATE_OPTSPEC+0:			info->p = (void *) mess_hd_option_spec;

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
	if ((drivenum < 0) || (drivenum > MAX_HARDDISKS))
	{
		return NULL;
	}

	return drive_handles[drivenum];
}

