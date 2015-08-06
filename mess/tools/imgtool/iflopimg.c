/*********************************************************************

	iflopimg.c

	Bridge code for Imgtool into the standard floppy code

*********************************************************************/

#include "formats/flopimg.h"
#include "imgtoolx.h"
#include "library.h"
#include "iflopimg.h"

struct ImgtoolFloppyExtra
{
	const struct FloppyFormat *format;
	imgtoolerr_t (*create)(imgtool_image *image, imgtool_stream *stream, option_resolution *opts);
	imgtoolerr_t (*open)(imgtool_image *image, imgtool_stream *stream);
};


imgtoolerr_t imgtool_floppy_error(floperr_t err)
{
	switch(err)
	{
		case FLOPPY_ERROR_SUCCESS:
			return IMGTOOLERR_SUCCESS;

		case FLOPPY_ERROR_OUTOFMEMORY:
			return IMGTOOLERR_OUTOFMEMORY;

		case FLOPPY_ERROR_INVALIDIMAGE:
			return IMGTOOLERR_CORRUPTIMAGE;

		case FLOPPY_ERROR_SEEKERROR:
			return IMGTOOLERR_SEEKERROR;

		case FLOPPY_ERROR_UNSUPPORTED:
			return IMGTOOLERR_UNIMPLEMENTED;
			
		default:
			return IMGTOOLERR_UNEXPECTED;
	}
}



/*********************************************************************
	Imgtool ioprocs
*********************************************************************/

static void imgtool_floppy_closeproc(void *file)
{
	stream_close((imgtool_stream *) file);
}

static int imgtool_floppy_seekproc(void *file, INT64 offset, int whence)
{
	stream_seek((imgtool_stream *) file, offset, whence);
	return 0;
}

static size_t imgtool_floppy_readproc(void *file, void *buffer, size_t length)
{
	return stream_read((imgtool_stream *) file, buffer, length);
}

static size_t imgtool_floppy_writeproc(void *file, const void *buffer, size_t length)
{
	stream_write((imgtool_stream *) file, buffer, length);
	return length;
}

static UINT64 imgtool_floppy_filesizeproc(void *file)
{
	return stream_size((imgtool_stream *) file);
}

static struct io_procs imgtool_ioprocs =
{
	imgtool_floppy_closeproc,
	imgtool_floppy_seekproc,
	imgtool_floppy_readproc,
	imgtool_floppy_writeproc,
	imgtool_floppy_filesizeproc
};

static struct io_procs imgtool_noclose_ioprocs =
{
	NULL,
	imgtool_floppy_seekproc,
	imgtool_floppy_readproc,
	imgtool_floppy_writeproc,
	imgtool_floppy_filesizeproc
};



/*********************************************************************
	Imgtool handlers
*********************************************************************/

static const struct ImgtoolFloppyExtra *get_extra(const struct ImageModule *module)
{
	return (const struct ImgtoolFloppyExtra *) module->extra;
}



struct imgtool_floppy_image
{
	floppy_image *floppy;
};

static floppy_image *get_floppy(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg = (struct imgtool_floppy_image *) img_extrabytes(img);
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_open_internal(imgtool_image *image, imgtool_stream *f, int noclose)
{
	floperr_t ferr;
	imgtoolerr_t err;
	struct imgtool_floppy_image *fimg;
	const struct ImgtoolFloppyExtra *extra;

	extra = get_extra(img_module(image));
	fimg = (struct imgtool_floppy_image *) img_extrabytes(image);

	/* open up the floppy */
	ferr = floppy_open(f, noclose ? &imgtool_noclose_ioprocs : &imgtool_ioprocs,
		NULL, extra->format, FLOPPY_FLAGS_READWRITE, &fimg->floppy);
	if (ferr)
	{
		err = imgtool_floppy_error(ferr);
		return err;
	}

	if (extra->open)
	{
		err = extra->open(image, NULL);
		if (err)
			return err;
	}

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t imgtool_floppy_open(imgtool_image *image, imgtool_stream *f)
{
	return imgtool_floppy_open_internal(image, f, FALSE);
}



static imgtoolerr_t imgtool_floppy_create(imgtool_image *image, imgtool_stream *f, option_resolution *opts)
{
	floperr_t ferr;
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct FloppyFormat *format;
	const struct ImgtoolFloppyExtra *extra;
	struct imgtool_floppy_image *fimg;

	extra = get_extra(img_module(image));
	format = extra->format;
	fimg = (struct imgtool_floppy_image *) img_extrabytes(image);

	/* open up the floppy */
	ferr = floppy_create(f, &imgtool_ioprocs, format, opts, &fimg->floppy);
	if (ferr)
	{
		err = imgtool_floppy_error(ferr);
		goto done;
	}

	/* do we have to do extra stuff when creating the image? */
	if (extra->create)
	{
		err = extra->create(image, NULL, opts);
		if (err)
			goto done;
	}

	/* do we have to do extra stuff when opening the image? */
	if (extra->open)
	{
		err = extra->open(image, NULL);
		if (err)
			goto done;
	}

done:
	return err;
}



static void imgtool_floppy_close(imgtool_image *img)
{
	floppy_close(get_floppy(img));
}



imgtoolerr_t imgtool_floppy_get_sector_size(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, UINT32 *sector_size)
{
	floperr_t ferr;

	ferr = floppy_get_sector_length(get_floppy(image), head, track, sector, sector_size);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t imgtool_floppy_read_sector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, void *buffer, size_t len)
{
	floperr_t ferr;

	ferr = floppy_read_sector(get_floppy(image), head, track, sector, 0, buffer, len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t imgtool_floppy_write_sector(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len)
{
	floperr_t ferr;

	ferr = floppy_write_sector(get_floppy(image), head, track, sector, 0, buffer, len);
	if (ferr)
		return imgtool_floppy_error(ferr);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t imgtool_floppy_createmodule(imgtool_library *library, const char *format_name,
	const char *description, const struct FloppyFormat *format,
	void (*getinfo)(UINT32 state, union imgtoolinfo *info))
{
	imgtoolerr_t err;
	struct ImageModule *module;
	int format_index;
	char buffer[512];
	struct ImgtoolFloppyExtra *extra;

	for (format_index = 0; format[format_index].construct; format_index++)
	{
		extra = imgtool_library_alloc(library, sizeof(*extra));
		if (!extra)
			return IMGTOOLERR_OUTOFMEMORY;
		memset(extra, 0, sizeof(*extra));
		extra->format = &format[format_index];

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s_%s",
			format[format_index].name, format_name);

		err = imgtool_library_createmodule(library, buffer, &module);
		if (err)
			return err;

		snprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), "%s (%s)",
			format[format_index].description, description);

		module->image_extra_bytes		= sizeof(struct imgtool_floppy_image);
		module->description				= imgtool_library_strdup(library, buffer);
		module->open					= imgtool_floppy_open;
		module->create					= imgtool_floppy_create;
		module->close					= imgtool_floppy_close;
		module->extensions				= format[format_index].extensions;
		module->extra					= extra;
		module->createimage_optguide	= format[format_index].param_guidelines ? floppy_option_guide : NULL;
		module->createimage_optspec		= format[format_index].param_guidelines;
		module->get_sector_size			= imgtool_floppy_get_sector_size;
		module->read_sector				= imgtool_floppy_read_sector;
		module->write_sector			= imgtool_floppy_write_sector;

		if (getinfo)
		{
			extra->create						= (imgtoolerr_t	(*)(imgtool_image *, imgtool_stream *, option_resolution *)) imgtool_get_info_ptr(getinfo, IMGTOOLINFO_PTR_CREATE);
			extra->open							= (imgtoolerr_t	(*)(imgtool_image *, imgtool_stream *)) imgtool_get_info_ptr(getinfo, IMGTOOLINFO_PTR_OPEN);
			module->eoln						= imgtool_library_strdup(library, imgtool_get_info_ptr(getinfo, IMGTOOLINFO_STR_EOLN));
			module->path_separator				= (char) imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_PATH_SEPARATOR);
			module->alternate_path_separator	= (char) imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_ALTERNATE_PATH_SEPARATOR);
			module->prefer_ucase				= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_PREFER_UCASE) ? 1 : 0;
			module->initial_path_separator		= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_INITIAL_PATH_SEPARATOR) ? 1 : 0;
			module->open_is_strict				= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_OPEN_IS_STRICT) ? 1 : 0;
			module->supports_creation_time		= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_SUPPORTS_CREATION_TIME) ? 1 : 0;
			module->supports_lastmodified_time	= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_SUPPORTS_LASTMODIFIED_TIME) ? 1 : 0;
			module->tracks_are_called_cylinders	= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_TRACKS_ARE_CALLED_CYLINDERS) ? 1 : 0;
			module->writing_untested			= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_WRITING_UNTESTED) ? 1 : 0;
			module->creation_untested			= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_CREATION_UNTESTED) ? 1 : 0;
			module->supports_bootblock			= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_SUPPORTS_BOOTBLOCK) ? 1 : 0;
			module->info						= (void (*)(imgtool_image *, char *, size_t)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_INFO);
			module->begin_enum					= (imgtoolerr_t (*)(imgtool_imageenum *, const char *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_BEGIN_ENUM);
			module->next_enum					= (imgtoolerr_t (*)(imgtool_imageenum *, imgtool_dirent *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_NEXT_ENUM);
			module->close_enum					= (void (*)(imgtool_imageenum *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_CLOSE_ENUM);
			module->free_space					= (imgtoolerr_t (*)(imgtool_image *, UINT64 *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_FREE_SPACE);
			module->read_file					= (imgtoolerr_t (*)(imgtool_image *, const char *, const char *, imgtool_stream *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_READ_FILE);
			module->write_file					= (imgtoolerr_t (*)(imgtool_image *, const char *, const char *, imgtool_stream *, option_resolution *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_WRITE_FILE);
			module->delete_file					= (imgtoolerr_t (*)(imgtool_image *, const char *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_DELETE_FILE);
			module->list_forks					= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_forkent *, size_t)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_LIST_FORKS);
			module->create_dir					= (imgtoolerr_t (*)(imgtool_image *, const char *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_CREATE_DIR);
			module->delete_dir					= (imgtoolerr_t (*)(imgtool_image *, const char *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_DELETE_DIR);
			module->list_attrs					= (imgtoolerr_t (*)(imgtool_image *, const char *, UINT32 *, size_t)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_LIST_ATTRS);
			module->get_attrs					= (imgtoolerr_t (*)(imgtool_image *, const char *, const UINT32 *, imgtool_attribute *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_GET_ATTRS);
			module->set_attrs					= (imgtoolerr_t (*)(imgtool_image *, const char *, const UINT32 *, const imgtool_attribute *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_SET_ATTRS);
			module->attr_name					= (imgtoolerr_t (*)(UINT32, const imgtool_attribute *, char *, size_t)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_ATTR_NAME);
			module->get_iconinfo				= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_iconinfo *)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_GET_ICON_INFO);
			module->suggest_transfer			= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_transfer_suggestion *, size_t))  imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_SUGGEST_TRANSFER);
			module->get_chain					= (imgtoolerr_t (*)(imgtool_image *, const char *, imgtool_chainent *, size_t)) imgtool_get_info_fct(getinfo, IMGTOOLINFO_PTR_GET_CHAIN);
			module->writefile_optguide			= (const struct OptionGuide *) imgtool_get_info_ptr(getinfo, IMGTOOLINFO_PTR_WRITEFILE_OPTGUIDE);
			module->writefile_optspec			= imgtool_library_strdup(library, imgtool_get_info_ptr(getinfo, IMGTOOLINFO_STR_WRITEFILE_OPTSPEC));
			module->image_extra_bytes			+= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_IMAGE_EXTRA_BYTES);
			module->imageenum_extra_bytes		+= imgtool_get_info_int(getinfo, IMGTOOLINFO_INT_ENUM_EXTRA_BYTES);
		}
	}
	return IMGTOOLERR_SUCCESS;
}



floppy_image *imgtool_floppy(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg;
	fimg = (struct imgtool_floppy_image *) img_extrabytes(img);
	return fimg->floppy;
}



static imgtoolerr_t imgtool_floppy_transfer_sector_tofrom_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f, int direction)
{
	floperr_t err;
	floppy_image *floppy;
	void *buffer = NULL;

	floppy = imgtool_floppy(img);

	buffer = malloc(length);
	if (!buffer)
	{
		err = FLOPPY_ERROR_OUTOFMEMORY;
		goto done;
	}

	if (direction)
	{
		err = floppy_read_sector(floppy, head, track, sector, offset, buffer, length);
		if (err)
			goto done;
		stream_write(f, buffer, length);
	}
	else
	{
		stream_read(f, buffer, length);
		err = floppy_write_sector(floppy, head, track, sector, offset, buffer, length);
		if (err)
			goto done;
	}
	
	err = FLOPPY_ERROR_SUCCESS;

done:
	if (buffer)
		free(buffer);
	return imgtool_floppy_error(err);
}



imgtoolerr_t imgtool_floppy_read_sector_to_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f)
{
	return imgtool_floppy_transfer_sector_tofrom_stream(img, head, track, sector, offset, length, f, 1);
}



imgtoolerr_t imgtool_floppy_write_sector_from_stream(imgtool_image *img, int head, int track, int sector, int offset, size_t length, imgtool_stream *f)
{
	return imgtool_floppy_transfer_sector_tofrom_stream(img, head, track, sector, offset, length, f, 0);
}



void *imgtool_floppy_extrabytes(imgtool_image *img)
{
	struct imgtool_floppy_image *fimg;
	fimg = (struct imgtool_floppy_image *) img_extrabytes(img);
	return fimg + 1;
}



