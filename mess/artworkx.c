/*********************************************************************

	artworkx.c

	MESS specific artwork code

*********************************************************************/

#include <ctype.h>

#include "artworkx.h"
#include "image.h"
#include "png.h"


/***************************************************************************

	Local variables

***************************************************************************/

static char *override_artfile;



/**************************************************************************/

void artwork_use_device_art(mess_image *img, const char *defaultartfile)
{
	const char *strs[3];
	int len, pos, i;

	/* This function builds the override_artfile string.  This string is a
	 * list of NUL terminated strings.  These strings become the basename for
	 * the .art file that we use.
	 *
	 * We use the following strings:
	 * 1.  The basename
	 * 2.  The goodname
	 * 3.  The file specified by defaultartfile
	 */

	strs[0] = image_basename_noext(img);
	strs[1] = strs[0] ? image_longname(img) : NULL;
	strs[2] = defaultartfile;

	/* concatenate the strings; first calculate length of the string */
	len = 1;
	for (i = 0; i < sizeof(strs) / sizeof(strs[0]); i++)
	{
		if (strs[i])
			len += strlen(strs[i]) + 1;
	}

	override_artfile = malloc(len);
	if (!override_artfile)
		return;
	override_artfile[len - 1] = '\0';

	/* now actually concatenate the strings */
	pos = 0;
	for (i = 0; i < sizeof(strs) / sizeof(strs[0]); i++)
	{
		if (strs[i])
		{
			strcpy(&override_artfile[pos], strs[i]);
			pos += strlen(strs[i]) + 1;
		}
	}

	/* small transformations */
	for (i = 0; i < len; i++)
	{
		if (override_artfile[i] == ':')
			override_artfile[i] = '-';
	}
}



static int mess_activate_artwork(osd_create_params *params)
{
	if ((params->width < options.min_width) && (params->height < options.min_height))
	{
		options.artwork_res = 2;
		return TRUE;
	}
	return FALSE;
}



static mame_file *mess_load_artwork_file(const game_driver **driver)
{
	char filename[2048];
	mame_file *artfile = NULL;
	const char *s;

	while (*driver)
	{
		if ((*driver)->name)
		{
			s = override_artfile ? override_artfile : "";
			do
			{
				sprintf(filename, "%s.art", *s ? s : (*driver)->name);
				if (*s)
					s += strlen(s) + 1;
				artfile = mame_fopen((*driver)->name, filename, FILETYPE_ARTWORK, 0);
			}
			while(!artfile && *s);
			if (artfile)
				break;
		}
		*driver = driver_get_clone(*driver);
	}
	return artfile;
}



artwork_callbacks mess_artwork_callbacks =
{
	mess_activate_artwork,
	mess_load_artwork_file
};

/********************************************************************/

int artwork_get_inputscreen_customizations(png_info *png, int cust_type,
	const char *section,
	struct inputform_customization *customizations,
	int customizations_length)
{
	mame_file *file;
	char buffer[1000];
	char current_section[64];
	char ipt_name[64];
	char *p;
	int x1, y1, x2, y2;
	const char *png_filename;
	const char *ini_filename;
	int enabled = TRUE;
	int item_count = 0;

	static const char *cust_files[] =
	{
		"ctrlr.png",		"ctrlr.ini",
		"keyboard.png",		"keyboard.ini"
		"misc.png",			"misc.ini"
	};

	if ((cust_type >= 0) && (cust_type < ((sizeof(cust_files) / sizeof(cust_files[0])) / 2)))
	{
		png_filename = cust_files[cust_type * 2 + 0];
		ini_filename = cust_files[cust_type * 2 + 1];
	}
	else
	{
		png_filename = NULL;
		ini_filename = NULL;
	}

	/* subtract one from the customizations length; so we can place IPT_END */
	customizations_length--;

	/* open the INI file, if available */
	if (ini_filename)
	{
		file = mame_fopen(Machine->gamedrv->name, ini_filename, FILETYPE_ARTWORK, 0);
		if (file)
		{
			/* loop until we run out of lines */
			while (customizations_length && mame_fgets(buffer, sizeof(buffer), file))
			{
				/* strip off any comments */
				p = strstr(buffer, "//");
				if (p)
					*p = 0;

				/* section header? */
				if (buffer[0] == '[')
				{
					strncpyz(current_section, &buffer[1],
						sizeof(current_section) / sizeof(current_section[0]));
					p = strchr(current_section, ']');
					if (!p)
						continue;
					*p = '\0';
					if (section)
						enabled = !mame_stricmp(current_section, section);
					continue;
				}

				if (!enabled || sscanf(buffer, "%64s (%d,%d)-(%d,%d)", ipt_name, &x1, &y1, &x2, &y2) != 5)
					continue;

#if 0
				/* temporarily disabled */
				for (pik = input_keywords; pik->name[0]; pik++)
				{
					pik_name = pik->name;
					if ((pik_name[0] == 'P') && (pik_name[1] == '1') && (pik_name[2] == '_'))
						pik_name += 3;

					if (!strcmp(ipt_name, pik_name))
					{
						if ((x1 > 0) && (y1 > 0) && (x2 > x1) && (y2 > y1))
						{
							customizations->ipt = pik->val;
							customizations->x = x1;
							customizations->y = y1;
							customizations->width = x2 - x1;
							customizations->height = y2 - y1;
							customizations++;				
							customizations_length--;
							item_count++;
						}
						break;
					}
				}
#endif
			}
			mame_fclose(file);
		}
	}

	/* terminate list */
	customizations->ipt = IPT_END;
	customizations->x = -1;
	customizations->y = -1;
	customizations->width = -1;
	customizations->height = -1;

	/* open the PNG, if available */
	memset(png, 0, sizeof(*png));
	if (png_filename && item_count > 0)
	{
		file = mame_fopen(Machine->gamedrv->name, png_filename, FILETYPE_ARTWORK, 0);
		if (file)
		{
			png_read_file(file, png);
			mame_fclose(file);
		}
	}
	return item_count;
}
