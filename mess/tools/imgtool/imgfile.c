/***************************************************************************

	imgfile.c

	Core code for Imgtool functions on files

***************************************************************************/

#include <ctype.h>
#include "imgtool.h"
#include "opresolv.h"
#include "pool.h"

struct _imgtool_image
{
	const struct ImageModule *module;
	memory_pool pool;
};

struct _imgtool_imageenum
{
	imgtool_image *image;
};



static imgtoolerr_t markerrorsource(imgtoolerr_t err)
{
	assert(imgtool_error != NULL);

	switch(err)
	{
		case IMGTOOLERR_OUTOFMEMORY:
		case IMGTOOLERR_UNEXPECTED:
		case IMGTOOLERR_BUFFERTOOSMALL:
			/* Do nothing */
			break;

		case IMGTOOLERR_FILENOTFOUND:
		case IMGTOOLERR_BADFILENAME:
			err |= IMGTOOLERR_SRC_FILEONIMAGE;
			break;

		default:
			err |= IMGTOOLERR_SRC_IMAGEFILE;
			break;
	}
	return err;
}



static void internal_error(const struct ImageModule *module, const char *message)
{
#ifdef MAME_DEBUG
	logerror("%s: %s\n", module->name, message);
#endif
}



static imgtoolerr_t internal_open(const struct ImageModule *module, const char *fname,
	int read_or_write, option_resolution *createopts, imgtool_image **outimg)
{
	imgtoolerr_t err;
	imgtool_stream *f = NULL;
	imgtool_image *image = NULL;
	size_t size;

	if (outimg)
		*outimg = NULL;

	/* is the requested functionality implemented? */
	if ((read_or_write == OSD_FOPEN_RW_CREATE) ? !module->create : !module->open)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* open the stream */
	f = stream_open(fname, read_or_write);
	if (!f)
	{
		err = IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;
		goto done;
	}

	/* setup the image structure */
	size = sizeof(struct _imgtool_image) + module->image_extra_bytes;
	image = (imgtool_image *) malloc(size);
	if (!image)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	memset(image, '\0', size);
	pool_init(&image->pool);
	image->module = module;
	
	/* actually call create or open */
	if (read_or_write == OSD_FOPEN_RW_CREATE)
		err = module->create(image, f, createopts);
	else
		err = module->open(image, f);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (err)
	{
		if (f)
			stream_close(f);
		if (image)
		{
			free(image);
			image = NULL;
		}
	}

	if (outimg)
		*outimg = image;
	else if (image)
		img_close(image);
	return err;
}



imgtoolerr_t img_open(const struct ImageModule *module, const char *fname, int read_or_write, imgtool_image **outimg)
{
	read_or_write = read_or_write ? OSD_FOPEN_RW : OSD_FOPEN_READ;
	return internal_open(module, fname, read_or_write, NULL, outimg);
}



imgtoolerr_t img_open_byname(imgtool_library *library, const char *modulename, const char *fname, int read_or_write, imgtool_image **outimg)
{
	const struct ImageModule *module;

	module = imgtool_library_findmodule(library, modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_open(module, fname, read_or_write, outimg);
}



void img_close(imgtool_image *img)
{
	if (img->module->close)
		img->module->close(img);
	pool_exit(&img->pool);
	free(img);
}



imgtoolerr_t img_info(imgtool_image *img, char *string, size_t len)
{
	if (len > 0)
	{
		if (img->module->info)
			img->module->info(img, string, len);
		else
			string[0] = '\0';
	}
	return 0;
}



#define PATH_MUSTBEDIR		0x00000001
#define PATH_LEAVENULLALONE	0x00000002
#define PATH_CANBEBOOTBLOCK	0x00000004

static imgtoolerr_t cannonicalize_path(imgtool_image *image, UINT32 flags,
	const char **path, char **alloc_path)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	char *new_path = NULL;
	char path_separator, alt_path_separator;
	const char *s;
	int in_path_separator, i, j;
	
	path_separator = image->module->path_separator;
	alt_path_separator = image->module->alternate_path_separator;

	/* is this path NULL?  if so, is that ignored? */
	if (!*path && (flags & PATH_LEAVENULLALONE))
		goto done;

	/* is this the special filename for bootblocks? */
	if (*path == FILENAME_BOOTBLOCK)
	{
		if (!(flags & PATH_CANBEBOOTBLOCK))
			err = IMGTOOLERR_UNEXPECTED;
		else if (!image->module->supports_bootblock)
			err = IMGTOOLERR_FILENOTFOUND;
		goto done;
	}

	if (path_separator == '\0')
	{
		if (flags & PATH_MUSTBEDIR)
		{
			/* do we specify a path when paths are not supported? */
			if (*path && **path)
			{
				err = IMGTOOLERR_CANNOTUSEPATH | IMGTOOLERR_SRC_FUNCTIONALITY;
				goto done;
			}
			*path = NULL;	/* normalize empty path */
		}
	}
	else
	{
		s = *path ? *path : "";

		/* allocate space for a new cannonical path */
		new_path = malloc(strlen(s) + 4);
		if (!new_path)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}

		/* copy the path */
		in_path_separator = TRUE;
		i = j = 0;
		do
		{
			if ((s[i] != '\0') && (s[i] != path_separator) && (s[i] != alt_path_separator))
			{
				new_path[j++] = s[i];
				in_path_separator = FALSE;
			}
			else if (!in_path_separator)
			{
				new_path[j++] = '\0';
				in_path_separator = TRUE;
			}
		}
		while(s[i++] != '\0');
		new_path[j++] = '\0';
		new_path[j++] = '\0';
		*path = new_path;
	}

done:
	*alloc_path = new_path;
	return err;
}



static imgtoolerr_t cannonicalize_fork(imgtool_image *image, const char **fork)
{
	/* does this module support forks? */
	if (image->module->list_forks)
	{
		/* this module supports forks; make sure that fork is non-NULL */
		if (!*fork)
			*fork = "";
	}
	else
	{
		/* this module does not support forks; make sure that fork is NULL */
		if (*fork)
			return IMGTOOLERR_NOFORKS;
	}
	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_beginenum(imgtool_image *img, const char *path, imgtool_imageenum **outenum)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_imageenum *enumeration = NULL;
	char *alloc_path = NULL;
	size_t size;

	assert(img);
	assert(outenum);

	*outenum = NULL;

	if (!img->module->next_enum)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	err = cannonicalize_path(img, PATH_MUSTBEDIR, &path, &alloc_path);
	if (err)
		goto done;

	size = sizeof(struct _imgtool_imageenum) + img_module(img)->imageenum_extra_bytes;
	enumeration = (imgtool_imageenum *) malloc(size);
	if (!enumeration)
		goto done;
	memset(enumeration, '\0', size);
	enumeration->image = img;

	if (img->module->begin_enum)
	{
		err = img->module->begin_enum(enumeration, path);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (alloc_path)
		free(alloc_path);
	if (err && enumeration)
	{
		free(enumeration);
		enumeration = NULL;
	}
	*outenum = enumeration;
	return err;
}



imgtoolerr_t img_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	const struct ImageModule *module;

	module = img_enum_module(enumeration);

	/* This makes it so that drivers don't have to take care of clearing
	 * the attributes if they don't apply
	 */
	memset(ent, 0, sizeof(*ent));

	err = module->next_enum(enumeration, ent);
	if (err)
		return markerrorsource(err);

	/* don't trust the module! */
	if (!module->supports_creation_time && (ent->creation_time != 0))
	{
		internal_error(module, "next_enum() specified creation_time, which is marked as unsupported by this module");
		return IMGTOOLERR_UNEXPECTED;
	}
	if (!module->supports_lastmodified_time && (ent->lastmodified_time != 0))
	{
		internal_error(module, "next_enum() specified lastmodified_time, which is marked as unsupported by this module");
		return IMGTOOLERR_UNEXPECTED;
	}
	if (!module->path_separator && ent->directory)
	{
		internal_error(module, "next_enum() returned a directory, which is marked as unsupported by this module");
		return IMGTOOLERR_UNEXPECTED;
	}
	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_getdirent(imgtool_image *img, const char *path, int index, imgtool_dirent *ent)
{
	imgtoolerr_t err;
	imgtool_imageenum *imgenum = NULL;

	if (index < 0)
	{
		err = IMGTOOLERR_PARAMTOOSMALL;
		goto done;
	}

	err = img_beginenum(img, path, &imgenum);
	if (err)
		goto done;

	do
	{
		err = img_nextenum(imgenum, ent);
		if (err)
			goto done;

		if (ent->eof)
		{
			err = IMGTOOLERR_FILENOTFOUND;
			goto done;
		}
	}
	while(index--);

done:
	if (err)
		memset(ent->filename, 0, sizeof(ent->filename));
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}



void img_closeenum(imgtool_imageenum *enumeration)
{
	const struct ImageModule *module;
	module = img_enum_module(enumeration);
	if (module->close_enum)
		module->close_enum(enumeration);
	free(enumeration);
}



imgtoolerr_t img_countfiles(imgtool_image *img, int *totalfiles)
{
	int err;
	imgtool_imageenum *imgenum;
	imgtool_dirent ent;

	*totalfiles = 0;
	memset(&ent, 0, sizeof(ent));

	err = img_beginenum(img, NULL, &imgenum);
	if (err)
		goto done;

	do {
		err = img_nextenum(imgenum, &ent);
		if (err)
			goto done;

		if (ent.filename[0])
			(*totalfiles)++;
	}
	while(ent.filename[0]);

done:
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}



imgtoolerr_t img_filesize(imgtool_image *img, const char *fname, UINT64 *filesize)
{
	int err;
	imgtool_imageenum *imgenum;
	imgtool_dirent ent;
	const char *path;

	path = NULL;	/* TODO: Need to parse off the path */

	*filesize = -1;
	memset(&ent, 0, sizeof(ent));

	err = img_beginenum(img, path, &imgenum);
	if (err)
		goto done;

	do
	{
		err = img_nextenum(imgenum, &ent);
		if (err)
			goto done;

		if (!mame_stricmp(fname, ent.filename))
		{
			*filesize = ent.filesize;
			goto done;
		}
	}
	while(ent.filename[0]);

	err = IMGTOOLERR_FILENOTFOUND;

done:
	if (imgenum)
		img_closeenum(imgenum);
	return err;
}



imgtoolerr_t img_listattrs(imgtool_image *image, const char *path, UINT32 *attrs, size_t len)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	memset(attrs, 0, sizeof(*attrs) * len);

	if (!image->module->list_attrs)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->list_attrs(image, path, attrs, len);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_getattrs(imgtool_image *image, const char *path, const UINT32 *attrs, imgtool_attribute *values)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	if (!image->module->get_attrs)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->get_attrs(image, path, attrs, values);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_setattrs(imgtool_image *image, const char *path, const UINT32 *attrs, const imgtool_attribute *values)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	if (!image->module->set_attrs)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->set_attrs(image, path, attrs, values);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_getattr(imgtool_image *image, const char *path, UINT32 attr, imgtool_attribute *value)
{
	UINT32 attrs[2];
	attrs[0] = attr;
	attrs[1] = 0;
	return img_getattrs(image, path, attrs, value);
}



imgtoolerr_t img_setattr(imgtool_image *image, const char *path, UINT32 attr, imgtool_attribute value)
{
	UINT32 attrs[2];
	attrs[0] = attr;
	attrs[1] = 0;
	return img_setattrs(image, path, attrs, &value);
}



imgtoolerr_t img_geticoninfo(imgtool_image *image, const char *path, imgtool_iconinfo *iconinfo)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;


	if (!image->module->get_iconinfo)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, 0, &path, &alloc_path);
	if (err)
		goto done;

	memset(iconinfo, 0, sizeof(*iconinfo));
	err = image->module->get_iconinfo(image, path, iconinfo);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_suggesttransfer(imgtool_image *image, const char *path,
	imgtool_stream *stream, imgtool_transfer_suggestion *suggestions, size_t suggestions_length)
{
	imgtoolerr_t err;
	int i, j;
	char *alloc_path = NULL;
	imgtoolerr_t (*check_stream)(imgtool_stream *stream, imgtool_suggestion_viability_t *viability);
	size_t position;

	/* clear out buffer */
	memset(suggestions, 0, sizeof(*suggestions) * suggestions_length);

	if (!image->module->suggest_transfer)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_LEAVENULLALONE, &path, &alloc_path);
	if (err)
		goto done;

	/* invoke the module's suggest call */
	err = image->module->suggest_transfer(image, path, suggestions, suggestions_length);
	if (err)
		goto done;

	/* Loop on resulting suggestions, and do the following:
	 * 1.  Call check_stream if present, and remove disqualified streams
	 * 2.  Fill in missing descriptions
	 */
	i = j = 0;
	while(suggestions[i].viability)
	{
		if (stream && suggestions[i].filter)
		{
			check_stream = (imgtoolerr_t (*)(imgtool_stream *, imgtool_suggestion_viability_t *)) filter_get_info_fct(suggestions[i].filter, FILTINFO_PTR_CHECKSTREAM);
			if (check_stream)
			{
				position = stream_tell(stream);
				err = check_stream(stream, &suggestions[i].viability);
				stream_seek(stream, position, SEEK_SET);
				if (err)
					goto done;
			}
		}

		/* the check_stream proc can remove the option by clearing out the viability */
		if (suggestions[i].viability)
		{
			/* we may have to move this suggestion, if one was removed */
			if (i != j)
				memcpy(&suggestions[j], &suggestions[i], sizeof(*suggestions));

			/* if the description is missing, fill it in */
			if (!suggestions[j].description)
			{
				if (suggestions[j].filter)
					suggestions[j].description = filter_get_info_string(suggestions[i].filter, FILTINFO_STR_HUMANNAME);
				else
					suggestions[j].description = "Raw";
			}

			j++;
		}
		i++;
	}
	suggestions[j].viability = 0;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_getchain(imgtool_image *img, const char *path, imgtool_chainent *chain, size_t chain_size)
{
	size_t i;

	assert(chain_size > 0);

	if (!img->module->get_chain)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	/* initialize the chain array, so the module's get_chain function can be lazy */
	for (i = 0; i < chain_size; i++)
	{
		chain[i].level = 0;
		chain[i].block = ~0;
	}

	return img->module->get_chain(img, path, chain, chain_size - 1);
}



imgtoolerr_t img_getchain_string(imgtool_image *img, const char *path, char *buffer, size_t buffer_len)
{
	imgtoolerr_t err;
	imgtool_chainent chain[512];
	UINT64 last_block;
	UINT8 cur_level = 0;
	int len, i;
	int comma_needed = FALSE;

	/* determine the last block identifier */
	chain[0].block = ~0;
	last_block = chain[0].block;

	err = img_getchain(img, path, chain, sizeof(chain) / sizeof(chain[0]));
	if (err)
		return err;

	len = snprintf(buffer, buffer_len, "[");
	buffer += len;
	buffer_len -= len;

	for (i = 0; chain[i].block != last_block; i++)
	{
		while(cur_level < chain[i].level)
		{
			len = snprintf(buffer, buffer_len, " [");
			buffer += len;
			buffer_len -= len;
			cur_level++;
			comma_needed = FALSE;
		}
		while(cur_level > chain[i].level)
		{
			len = snprintf(buffer, buffer_len, "]");
			buffer += len;
			buffer_len -= len;
			cur_level--;
		}

		if (comma_needed)
		{
			len = snprintf(buffer, buffer_len, ", ");
			buffer += len;
			buffer_len -= len;
		}

		len = snprintf(buffer, buffer_len, "%u", (unsigned) chain[i].block);
		buffer += len;
		buffer_len -= len;
		comma_needed = TRUE;
	}

	do
	{
		len = snprintf(buffer, buffer_len, "]");
		buffer += len;
		buffer_len -= len;
	}
	while(cur_level-- > 0);

	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_freespace(imgtool_image *img, UINT64 *sz)
{
	imgtoolerr_t err;
	UINT64 size;

	if (!img->module->free_space)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	err = img->module->free_space(img, &size);
	if (err)
		return err | IMGTOOLERR_SRC_IMAGEFILE;

	if (sz)
		*sz = size;
	return IMGTOOLERR_SUCCESS;
}



imgtoolerr_t img_readfile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *destf, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_stream *newstream = NULL;
	char *alloc_path = NULL;
	union filterinfo u;

	if (!image->module->read_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_CANBEBOOTBLOCK, &filename, &alloc_path);
	if (err)
		goto done;

	err = cannonicalize_fork(image, &fork);
	if (err)
		goto done;

	if (filter)
	{
		/* use a filter */
		filter(FILTINFO_PTR_READFILE, &u);
		if (!u.read_file)
		{
			err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
			goto done;
		}

		err = u.read_file(image, filename, fork, destf);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}
	else
	{
		/* invoke the actual module */
		err = image->module->read_file(image, filename, fork, destf);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (newstream)
		stream_close(newstream);
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_writefile(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *opts, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	char *buf = NULL;
	char *s;
	imgtool_stream *newstream = NULL;
	option_resolution *alloc_resolution = NULL;
	char *alloc_path = NULL;
	UINT64 free_space;
	UINT64 file_size;
	union filterinfo u;

	if (!image->module->write_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* Does this image module prefer upper case file names? */
	if (image->module->prefer_ucase)
	{
		buf = malloc(strlen(filename) + 1);
		if (!buf)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		strcpy(buf, filename);
		for (s = buf; *s; s++)
			*s = toupper(*s);
		filename = buf;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, PATH_CANBEBOOTBLOCK, &filename, &alloc_path);
	if (err)
		goto done;

	err = cannonicalize_fork(image, &fork);
	if (err)
		goto done;

	/* allocate dummy options if necessary */
	if (!opts && image->module->writefile_optguide)
	{
		alloc_resolution = option_resolution_create(image->module->writefile_optguide, image->module->writefile_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	if (opts)
		option_resolution_finish(opts);

	/* if free_space is implemented; do a quick check to see if space is available */
	if (image->module->free_space)
	{
		err = image->module->free_space(image, &free_space);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}

		file_size = stream_size(sourcef);

		if (file_size > free_space)
		{
			err = markerrorsource(IMGTOOLERR_NOSPACE);
			goto done;
		}
	}

	if (filter)
	{
		/* use a filter */
		filter(FILTINFO_PTR_WRITEFILE, &u);
		if (!u.write_file)
		{
			err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
			goto done;
		}

		err = u.write_file(image, filename, fork, sourcef, opts);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}
	else
	{
		/* actually invoke the write file handler */
		err = image->module->write_file(image, filename, fork, sourcef, opts);
		if (err)
		{
			err = markerrorsource(err);
			goto done;
		}
	}

done:
	if (buf)
		free(buf);
	if (alloc_path)
		free(alloc_path);
	if (newstream)
		stream_close(newstream);
	if (alloc_resolution)
		option_resolution_close(alloc_resolution);
	return err;
}



imgtoolerr_t img_getfile(imgtool_image *image, const char *filename, const char *fork,
	const char *dest, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_stream *f;
	char *alloc_dest = NULL;
	const char *filter_extension = NULL;

	if (!dest)
	{
		if (filter)
			filter_extension = filter_get_info_string(filter, FILTINFO_STR_EXTENSION);

		if (filter_extension)
		{
			alloc_dest = malloc(strlen(filename) + 1 + strlen(filter_extension) + 1);
			if (!alloc_dest)
				return IMGTOOLERR_OUTOFMEMORY;

			sprintf(alloc_dest, "%s.%s", filename, filter_extension);
			dest = alloc_dest;
		}
		else
		{
			if (filename == FILENAME_BOOTBLOCK)
				dest = "boot.bin";
			else
				dest = filename;
		}
	}

	f = stream_open(dest, OSD_FOPEN_WRITE);
	if (!f)
	{
		err = IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;
		goto done;
	}

	err = img_readfile(image, filename, fork, f, filter);
	if (err)
		goto done;

done:
	if (f)
		stream_close(f);
	if (alloc_dest)
		free(alloc_dest);
	return err;
}



imgtoolerr_t img_putfile(imgtool_image *image, const char *newfname, const char *fork,
	const char *source, option_resolution *opts, filter_getinfoproc filter)
{
	imgtoolerr_t err;
	imgtool_stream *f;

	if (!newfname)
		newfname = (const char *) osd_basename((char *) source);

	f = stream_open(source, OSD_FOPEN_READ);
	if (!f)
		return IMGTOOLERR_FILENOTFOUND | IMGTOOLERR_SRC_NATIVEFILE;

	err = img_writefile(image, newfname, fork, f, opts, filter);
	stream_close(f);
	return err;
}



imgtoolerr_t img_deletefile(imgtool_image *img, const char *fname)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	if (!img->module->delete_file)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(img, 0, &fname, &alloc_path);
	if (err)
		goto done;

	err = img->module->delete_file(img, fname);
	if (err)
	{
		err = markerrorsource(err);
		goto done;
	}

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_listforks(imgtool_image *image, const char *path, imgtool_forkent *ents, size_t len)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	if (!image->module->list_forks)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(image, 0, &path, &alloc_path);
	if (err)
		goto done;

	err = image->module->list_forks(image, path, ents, len);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_createdir(imgtool_image *img, const char *path)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	/* implemented? */
	if (!img->module->create_dir)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(img, PATH_MUSTBEDIR, &path, &alloc_path);
	if (err)
		goto done;

	err = img->module->create_dir(img, path);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_deletedir(imgtool_image *img, const char *path)
{
	imgtoolerr_t err;
	char *alloc_path = NULL;

	/* implemented? */
	if (!img->module->delete_dir)
	{
		err = IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;
		goto done;
	}

	/* cannonicalize path */
	err = cannonicalize_path(img, PATH_MUSTBEDIR, &path, &alloc_path);
	if (err)
		goto done;

	err = img->module->delete_dir(img, path);
	if (err)
		goto done;

done:
	if (alloc_path)
		free(alloc_path);
	return err;
}



imgtoolerr_t img_create(const struct ImageModule *module, const char *fname,
	option_resolution *opts, imgtool_image **image)
{
	imgtoolerr_t err;
	option_resolution *alloc_resolution = NULL;

	/* allocate dummy options if necessary */
	if (!opts && module->createimage_optguide)
	{
		alloc_resolution = option_resolution_create(module->createimage_optguide, module->createimage_optspec);
		if (!alloc_resolution)
		{
			err = IMGTOOLERR_OUTOFMEMORY;
			goto done;
		}
		opts = alloc_resolution;
	}
	if (opts)
		option_resolution_finish(opts);

	err = internal_open(module, fname, OSD_FOPEN_RW_CREATE, opts, image);
	if (err)
		goto done;

done:
	if (alloc_resolution)
		option_resolution_close(alloc_resolution);
	return err;
}



imgtoolerr_t img_create_byname(imgtool_library *library, const char *modulename, const char *fname,
	option_resolution *opts, imgtool_image **image)
{
	const struct ImageModule *module;

	module = imgtool_library_findmodule(library, modulename);
	if (!module)
		return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;

	return img_create(module, fname, opts, image);
}



imgtoolerr_t img_getsectorsize(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, UINT32 *length)
{
	/* implemented? */
	if (!image->module->get_sector_size)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->get_sector_size(image, track, head, sector, length);
}



imgtoolerr_t img_readsector(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, void *buffer, size_t len)
{
	/* implemented? */
	if (!image->module->read_sector)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->read_sector(image, track, head, sector, buffer, len);
}



imgtoolerr_t img_writesector(imgtool_image *image, UINT32 track, UINT32 head,
	UINT32 sector, const void *buffer, size_t len)
{
	/* implemented? */
	if (!image->module->write_sector)
		return IMGTOOLERR_UNIMPLEMENTED | IMGTOOLERR_SRC_FUNCTIONALITY;

	return image->module->write_sector(image, track, head, sector, buffer, len);
}



void *img_malloc(imgtool_image *image, size_t size)
{
	return pool_malloc(&image->pool, size);
}



const struct ImageModule *img_module(imgtool_image *img)
{
	return img->module;
}



void *img_extrabytes(imgtool_image *img)
{
	assert(img->module->image_extra_bytes > 0);
	return ((UINT8 *) img) + sizeof(*img);
}



const struct ImageModule *img_enum_module(imgtool_imageenum *enumeration)
{
	return enumeration->image->module;
}



void *img_enum_extrabytes(imgtool_imageenum *enumeration)
{
	assert(enumeration->image->module->imageenum_extra_bytes > 0);
	return ((UINT8 *) enumeration) + sizeof(*enumeration);
}



imgtool_image *img_enum_image(imgtool_imageenum *enumeration)
{
	return enumeration->image;
}

