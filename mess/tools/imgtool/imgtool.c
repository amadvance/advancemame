/***************************************************************************

	imgtool.c

	Miscellaneous stuff in the Imgtool core

***************************************************************************/

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "osdepend.h"
#include "imgtoolx.h"
#include "utils.h"
#include "library.h"
#include "modules.h"

/* ----------------------------------------------------------------------- */

struct imgtool_module_features img_get_module_features(const struct ImageModule *module)
{
	struct imgtool_module_features features;
	memset(&features, 0, sizeof(features));

	if (module->create)
		features.supports_create = 1;
	if (module->open)
		features.supports_open = 1;
	if (module->read_file)
		features.supports_reading = 1;
	if (module->write_file)
		features.supports_writing = 1;
	if (module->delete_file)
		features.supports_deletefile = 1;
	if (module->path_separator)
		features.supports_directories = 1;
	if (module->free_space)
		features.supports_freespace = 1;
	if (module->create_dir)
		features.supports_createdir = 1;
	if (module->delete_dir)
		features.supports_deletedir = 1;
	if (module->supports_creation_time)
		features.supports_creation_time = 1;
	if (module->supports_lastmodified_time)
		features.supports_lastmodified_time = 1;
	if (module->read_sector)
		features.supports_readsector = 1;
	if (module->write_sector)
		features.supports_writesector = 1;
	if (module->list_forks)
		features.supports_forks = 1;
	if (module->get_iconinfo)
		features.supports_geticoninfo = 1;
	if (!features.supports_writing && !features.supports_createdir && !features.supports_deletefile && !features.supports_deletedir)
		features.is_read_only = 1;
	return features;
}



static imgtoolerr_t evaluate_module(const char *fname,
	const struct ImageModule *module, float *result)
{
	imgtoolerr_t err;
	imgtool_image *image = NULL;
	imgtool_imageenum *imageenum = NULL;
	imgtool_dirent ent;
	float current_result;

	*result = 0.0;

	err = img_open(module, fname, OSD_FOPEN_READ, &image);
	if (err)
		goto done;

	if (image)
	{
		current_result = module->open_is_strict ? 0.9 : 0.5;

		err = img_beginenum(image, NULL, &imageenum);
		if (err)
			goto done;

		memset(&ent, 0, sizeof(ent));
		do
		{
			err = img_nextenum(imageenum, &ent);
			if (err)
				goto done;

			if (ent.corrupt)
				current_result = (current_result * 99 + 1.00) / 100;
			else
				current_result = (current_result + 1.00) / 2;
		}
		while(!ent.eof);

		*result = current_result;
	}

done:
	if (ERRORCODE(err) == IMGTOOLERR_CORRUPTIMAGE)
		err = IMGTOOLERR_SUCCESS;
	if (imageenum)
		img_closeenum(imageenum);
	if (image)
		img_close(image);
	return err;
}



imgtoolerr_t img_identify(imgtool_library *library, const char *fname,
	ImageModuleConstPtr *modules, size_t count)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const struct ImageModule *module = NULL;
	const struct ImageModule *insert_module;
	const struct ImageModule *temp_module;
	size_t i = 0;
	const char *extension;
	float val, temp_val, *values = NULL;

	if (count <= 0)
	{
		err = IMGTOOLERR_UNEXPECTED;
		goto done;
	}

	for (i = 0; i < count; i++)
		modules[i] = NULL;
	if (count > 1)
		count--;		/* null terminate */

	values = (float *) malloc(count * sizeof(*values));
	if (!values)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}
	for (i = 0; i < count; i++)
		values[i] = 0.0;

	/* figure out the file extension, if any */
	extension = strrchr(fname, '.');
	if (extension)
		extension++;

	/* iterate through all modules */
	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		if (!extension || findextension(module->extensions, extension))
		{
			err = evaluate_module(fname, module, &val);
			if (err)
				goto done;

			insert_module = module;
			for (i = 0; (val > 0.0) && (i < count); i++)
			{
				if (val > values[i])
				{
					temp_val = values[i];
					temp_module = modules[i];
					values[i] = val;
					modules[i] = insert_module;
					val = temp_val;
					insert_module = temp_module;
				}
			}
		}
	}

	if (!modules[0])
		err = IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_IMAGEFILE;

done:
	if (values)
		free(values);
	return err;
}



void img_attrname(const struct ImageModule *module, UINT32 attribute, const imgtool_attribute *attr_value,
	char *buffer, size_t buffer_len)
{
	imgtoolerr_t err = IMGTOOLERR_UNIMPLEMENTED;

	buffer[0] = '\0';

	if (attr_value)
	{
		if (module->attr_name)
			err = module->attr_name(attribute, attr_value, buffer, buffer_len);

		if (err == IMGTOOLERR_UNIMPLEMENTED)
		{
			switch(attribute & 0xF0000)
			{
				case IMGTOOLATTR_INT_FIRST:
					snprintf(buffer, buffer_len, "%d", (int) attr_value->i);
					break;
			}
		}
	}
	else
	{
		switch(attribute)
		{
			case IMGTOOLATTR_INT_MAC_TYPE:
				snprintf(buffer, buffer_len, "File type");
				break;
			case IMGTOOLATTR_INT_MAC_CREATOR:
				snprintf(buffer, buffer_len, "File creator");
				break;
			case IMGTOOLATTR_INT_MAC_FINDERFLAGS:
				snprintf(buffer, buffer_len, "Finder flags");
				break;
			case IMGTOOLATTR_INT_MAC_COORDX:
				snprintf(buffer, buffer_len, "X coordinate");
				break;
			case IMGTOOLATTR_INT_MAC_COORDY:
				snprintf(buffer, buffer_len, "Y coordinate");
				break;
			case IMGTOOLATTR_INT_MAC_FINDERFOLDER:
				snprintf(buffer, buffer_len, "Finder folder");
				break;
			case IMGTOOLATTR_INT_MAC_ICONID:
				snprintf(buffer, buffer_len, "Icon ID");
				break;
			case IMGTOOLATTR_INT_MAC_SCRIPTCODE:
				snprintf(buffer, buffer_len, "Script code");
				break;
			case IMGTOOLATTR_INT_MAC_EXTENDEDFLAGS:
				snprintf(buffer, buffer_len, "Extended flags");
				break;
			case IMGTOOLATTR_INT_MAC_COMMENTID:
				snprintf(buffer, buffer_len, "Comment ID");
				break;
			case IMGTOOLATTR_INT_MAC_PUTAWAYDIRECTORY:
				snprintf(buffer, buffer_len, "Putaway directory");
				break;
			case IMGTOOLATTR_TIME_CREATED:
				snprintf(buffer, buffer_len, "Creation time");
				break;
			case IMGTOOLATTR_TIME_LASTMODIFIED:
				snprintf(buffer, buffer_len, "Last modified time");
				break;
		}
	}
}



/* ----------------------------------------------------------------------- */

int imgtool_validitychecks(void)
{
	int error = 0;
	int val;
	imgtoolerr_t err;
	imgtool_library *library;
	const struct ImageModule *module = NULL;
	const struct OptionGuide *guide_entry;
	struct imgtool_module_features features;

	err = imgtool_create_cannonical_library(TRUE, &library);
	if (err)
		goto done;

	while((module = imgtool_library_iterate(library, module)) != NULL)
	{
		features = img_get_module_features(module);

		if (!module->name)
		{
			printf("imgtool module %s has null 'name'\n", module->name);
			error = 1;
		}
		if (!module->description)
		{
			printf("imgtool module %s has null 'description'\n", module->name);
			error = 1;
		}
		if (!module->extensions)
		{
			printf("imgtool module %s has null 'extensions'\n", module->extensions);
			error = 1;
		}

		/* sanity checks on modules that do not support directories */
		if (!module->path_separator)
		{
			if (module->alternate_path_separator)
			{
				printf("imgtool module %s specified alternate_path_separator but not path_separator\n", module->name);
				error = 1;
			}
			if (module->initial_path_separator)
			{
				printf("imgtool module %s specified initial_path_separator without directory support\n", module->name);
				error = 1;
			}
			if (module->create_dir)
			{
				printf("imgtool module %s implements create_dir without directory support\n", module->name);
				error = 1;
			}
			if (module->delete_dir)
			{
				printf("imgtool module %s implements delete_dir without directory support\n", module->name);
				error = 1;
			}
		}

		/* sanity checks on sector operations */
		if (module->read_sector && !module->get_sector_size)
		{
			printf("imgtool module %s implements read_sector without supporting get_sector_size\n", module->name);
			error = 1;
		}
		if (module->write_sector && !module->get_sector_size)
		{
			printf("imgtool module %s implements write_sector without supporting get_sector_size\n", module->name);
			error = 1;
		}

		/* sanity checks on creation options */
		if (module->createimage_optguide || module->createimage_optspec)
		{
			if (!module->create)
			{
				printf("imgtool module %s has creation options without supporting create\n", module->name);
				error = 1;
			}
			if ((!module->createimage_optguide && module->createimage_optspec)
				|| (module->createimage_optguide && !module->createimage_optspec))
			{
				printf("imgtool module %s does has partially incomplete creation options\n", module->name);
				error = 1;
			}

			if (module->createimage_optguide && module->createimage_optspec)
			{
				guide_entry = module->createimage_optguide;
				while(guide_entry->option_type != OPTIONTYPE_END)
				{
					if (option_resolution_contains(module->createimage_optspec, guide_entry->parameter))
					{
						switch(guide_entry->option_type)
						{
							case OPTIONTYPE_INT:
							case OPTIONTYPE_ENUM_BEGIN:
								err = option_resolution_getdefault(module->createimage_optspec,
									guide_entry->parameter, &val);
								if (err)
									goto done;
								break;

							default:
								break;
						}
						if (!guide_entry->identifier)
						{
							printf("imgtool module %s creation option %d has null identifier\n",
								module->name, (int) (guide_entry - module->createimage_optguide));
							error = 1;
						}
						if (!guide_entry->display_name)
						{
							printf("imgtool module %s creation option %d has null display_name\n",
								module->name, (int) (guide_entry - module->createimage_optguide));
							error = 1;
						}
					}
					guide_entry++;
				}
			}
		}
	}

done:
	if (err)
	{
		printf("imgtool: %s\n", imgtool_error(err));
		error = 1;
	}
	if (library)
		imgtool_library_close(library);
	return error;
}



char *imgtool_temp_str(void)
{
	static int index;
	static char temp_string_pool[32][256];
	return temp_string_pool[index++ % (sizeof(temp_string_pool) / sizeof(temp_string_pool[0]))];
}

