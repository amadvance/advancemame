// license:BSD-3-Clause
// copyright-holders:Nathan Woods, Raphael Nabet
/*
	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in the MESS
	image code.

	Raphael Nabet 2003
*/

/*#include "imgtool.h"*/
#include "imgtoolx.h"

#include "harddisk.h"
#include "imghd.h"


static imgtoolerr_t imghd_map_chderror(chd_error chderr)
{
	imgtoolerr_t err;

	switch(chderr)
	{
		case CHDERR_NONE:
			err = IMGTOOLERR_SUCCESS;
			break;
		case CHDERR_OUT_OF_MEMORY:
			err = IMGTOOLERR_OUTOFMEMORY;
			break;
		case CHDERR_FILE_NOT_WRITEABLE:
			err = IMGTOOLERR_READONLY;
			break;
		case CHDERR_READ_ERROR:
			err = IMGTOOLERR_READERROR;
			break;
		case CHDERR_WRITE_ERROR:
			err = IMGTOOLERR_WRITEERROR;
			break;
		case CHDERR_INVALID_PARAMETER:
			err = IMGTOOLERR_PARAMCORRUPT;
			break;
		case CHDERR_NOT_SUPPORTED:
		case CHDERR_UNSUPPORTED_VERSION:
		case CHDERR_UNSUPPORTED_FORMAT:
			err = IMGTOOLERR_UNIMPLEMENTED;
			break;
		default:
			err = IMGTOOLERR_UNEXPECTED;
			break;
	}
	return err;
}


static uint64_t imgtool_chd_fsize(void *file)
{
	return stream_size((imgtool_stream *)file);
}

static size_t imgtool_chd_fread(void *ptr, size_t size, size_t nmemb, void *file)
{
	if (size == 0 || nmemb == 0)
		return 0;
	return stream_read((imgtool_stream *)file, ptr, size * nmemb) / size;
}

static size_t imgtool_chd_fwrite(const void *ptr, size_t size, size_t nmemb, void *file)
{
	if (size == 0 || nmemb == 0)
		return 0;
	return stream_write((imgtool_stream *)file, ptr, size * nmemb) / size;
}

static int imgtool_chd_fclose(void *file)
{
	(void)file;
	return 0;
}

static int imgtool_chd_fseek(void *file, int64_t offset, int whence)
{
	return stream_seek((imgtool_stream *)file, offset, whence);
}

static core_file_callbacks imgtool_chd_callbacks =
{
	imgtool_chd_fsize,
	imgtool_chd_fread,
	imgtool_chd_fclose,
	imgtool_chd_fseek
};

static const chd_write_callbacks imgtool_chd_write_callbacks =
{
	imgtool_chd_fsize,
	imgtool_chd_fread,
	imgtool_chd_fwrite,
	imgtool_chd_fclose,
	imgtool_chd_fseek
};


/*
	imghd_create()

	Create a MAME HD image
*/
imgtoolerr_t imghd_create(imgtool_stream *stream, UINT32 hunksize, UINT32 cylinders, UINT32 heads, UINT32 sectors, UINT32 seclen)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	chd_write_file *chd = NULL;
	chd_error rc;
	UINT64 logicalbytes;
	char metadata[256];

	/* sanity check args */
	if (cylinders == 0 || heads == 0 || sectors == 0 || seclen == 0)
		return IMGTOOLERR_PARAMCORRUPT;

	if (hunksize == 0)
		hunksize = 4096;
	if ((hunksize % seclen) != 0)
		return IMGTOOLERR_PARAMCORRUPT;

	/* bail if we are read only */
	if (stream_isreadonly(stream))
		return IMGTOOLERR_READONLY;

	/* calculations */
	logicalbytes = (UINT64)cylinders * heads * sectors * seclen;

	/* create the new uncompressed CHD v5 hard drive */
	rc = chd_create_uncompressed_v5_callbacks(&imgtool_chd_write_callbacks, stream, logicalbytes, hunksize, seclen, &chd);
	if (rc != CHDERR_NONE)
	{
		err = imghd_map_chderror(rc);
		goto done;
	}

	/* write the hard disk metadata */
	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, seclen);
	rc = chd_diff_write_metadata(chd, HARD_DISK_METADATA_TAG, 0, metadata, (UINT32)strlen(metadata) + 1, CHD_MDFLAGS_CHECKSUM);
	if (rc != CHDERR_NONE)
	{
		err = imghd_map_chderror(rc);
		goto done;
	}

done:
	if (chd)
		chd_close_diff(chd);
	return err;
}



/*
	imghd_open()

	Open stream as a MAME HD image
*/
imgtoolerr_t imghd_open(imgtool_stream *stream, struct mess_hard_disk_file *hard_disk)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	chd_error rc;

	hard_disk->stream = NULL;
	hard_disk->hard_disk = NULL;
	hard_disk->chd = NULL;
	hard_disk->writable = NULL;

	if (!stream_isreadonly(stream))
	{
		rc = chd_open_uncompressed_v5_callbacks(&imgtool_chd_write_callbacks, stream, &hard_disk->writable);
		if (rc != CHDERR_NONE)
		{
			err = imghd_map_chderror(rc);
			goto done;
		}
	}

	rc = chd_open_core_file_callbacks(&imgtool_chd_callbacks, stream, CHD_OPEN_READ, NULL, &hard_disk->chd);
	if (rc != CHDERR_NONE)
	{
		err = imghd_map_chderror(rc);
		goto done;
	}

	hard_disk->hard_disk = hard_disk_open_diff(hard_disk->chd, hard_disk->writable);
	if (!hard_disk->hard_disk)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}
	hard_disk->stream = stream;

done:
	if (err)
		imghd_close(hard_disk);
	return err;
}



/*
	imghd_close()

	Close MAME HD image
*/
void imghd_close(struct mess_hard_disk_file *disk)
{
	if (disk->hard_disk)
	{
		hard_disk_close(disk->hard_disk);
		disk->hard_disk = NULL;
	}
	if (disk->writable)
	{
		chd_close_diff(disk->writable);
		disk->writable = NULL;
	}
	if (disk->chd)
	{
		chd_close(disk->chd);
		disk->chd = NULL;
	}
	if (disk->stream)
	{
		stream_close(disk->stream);
		disk->stream = NULL;
	}
}



/*
	imghd_read()

	Read sector(s) from MAME HD image
*/
imgtoolerr_t imghd_read(struct mess_hard_disk_file *disk, UINT32 lbasector, UINT32 numsectors, void *buffer)
{
	UINT32 reply;

	reply = hard_disk_read(disk->hard_disk, lbasector, numsectors, buffer);

	return reply ? IMGTOOLERR_SUCCESS : IMGTOOLERR_READERROR;
}



/*
	imghd_write()

	Write sector(s) from MAME HD image
*/
imgtoolerr_t imghd_write(struct mess_hard_disk_file *disk, UINT32 lbasector, UINT32 numsectors, const void *buffer)
{
	UINT32 reply;

	if (!disk || !disk->writable)
		return IMGTOOLERR_READONLY;

	reply = hard_disk_write(disk->hard_disk, lbasector, numsectors, buffer);

	return reply ? IMGTOOLERR_SUCCESS : IMGTOOLERR_WRITEERROR;
}



/*
	imghd_get_header()

	Return pointer to the header of MAME HD image
*/
const hard_disk_info *imghd_get_header(struct mess_hard_disk_file *disk)
{
	return hard_disk_get_info(disk->hard_disk);
}


static imgtoolerr_t mess_hd_image_create(imgtool_image *image, imgtool_stream *f, option_resolution *createoptions);

enum
{
	mess_hd_createopts_blocksize = 'B',
	mess_hd_createopts_cylinders = 'C',
	mess_hd_createopts_heads     = 'D',
	mess_hd_createopts_sectors   = 'E',
	mess_hd_createopts_seclen    = 'F'
};

OPTION_GUIDE_START( mess_hd_create_optionguide )
	OPTION_INT(mess_hd_createopts_blocksize, "blocksize", "Sectors Per Block" )
	OPTION_INT(mess_hd_createopts_cylinders, "cylinders", "Cylinders" )
	OPTION_INT(mess_hd_createopts_heads, "heads",	"Heads" )
	OPTION_INT(mess_hd_createopts_sectors, "sectors", "Total Sectors" )
	OPTION_INT(mess_hd_createopts_seclen, "seclen", "Sector Bytes" )
OPTION_GUIDE_END

#define mess_hd_create_optionspecs "B[1]-2048;C1-[32]-65536;D1-[8]-64;E1-[128]-4096;F128/256/[512]/1024/2048/4096/8192/16384/32768/65536"


imgtoolerr_t mess_hd_createmodule(imgtool_library *library)
{
	imgtoolerr_t err;
	struct ImageModule *module;

	err = imgtool_library_createmodule(library, "mess_hd", &module);
	if (err)
		return err;

	module->description				= "MESS hard disk image";
	module->extensions				= "hd\0";

	module->create					= mess_hd_image_create;

	module->createimage_optguide	= mess_hd_create_optionguide;
	module->createimage_optspec		= mess_hd_create_optionspecs;

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t mess_hd_image_create(imgtool_image *image, imgtool_stream *f, option_resolution *createoptions)
{
	UINT32  blocksize, cylinders, heads, sectors, seclen;

	/* read options */
	blocksize = option_resolution_lookup_int(createoptions, mess_hd_createopts_blocksize);
	cylinders = option_resolution_lookup_int(createoptions, mess_hd_createopts_cylinders);
	heads = option_resolution_lookup_int(createoptions, mess_hd_createopts_heads);
	sectors = option_resolution_lookup_int(createoptions, mess_hd_createopts_sectors);
	seclen = option_resolution_lookup_int(createoptions, mess_hd_createopts_seclen);

	return imghd_create(f, blocksize * seclen, cylinders, heads, sectors, seclen);
}
