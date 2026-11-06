// license:BSD-3-Clause
// copyright-holders:Andrea Mazzoleni
/*
 * Package selection is deliberately separate from the image search: images
 * may only be fetched from the directory or ZIP containing default.lay.
 */
#include "artwork_package.h"
#include "osdepend.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Implemented in the core I/O layer so PNG/XML readers can use mame_file. */
extern mame_file *mame_fopen_artwork_member(int path_index, const char *game, int zipped, const char *member);

static int safe_relative(const char *s)
{
	const char *p, *begin;

	if (!s || !*s || *s == '/' || *s == '\\')
		return 0;
	for (p = s; *p; ++p)
		if (*p == '\\' || *p == ':' || (unsigned char)*p < 32)
			return 0;
	begin = s;
	for (p = s; ; ++p)
	{
		if (*p == '/' || !*p)
		{
			size_t n = (size_t)(p - begin);
			if (!n || (n == 1 && begin[0] == '.') ||
				(n == 2 && begin[0] == '.' && begin[1] == '.'))
				return 0;
			if (!*p)
				break;
			begin = p + 1;
		}
	}
	return 1;
}

int artwork_package_find(const game_driver *driver, artwork_package *out)
{
	int path_count = osd_get_path_count(FILETYPE_ARTWORK);
	const game_driver *d;

	memset(out, 0, sizeof(*out));
	for (d = driver; d; d = driver_get_clone(d))
	{
		int idx;
		if (!d->name)
			continue;
		if (strlen(d->name) >= sizeof(out->game))
			return -1;
		for (idx = 0; idx < path_count; ++idx)
		{
			char archive[256];
			int kind;
			kind = osd_get_path_info(FILETYPE_ARTWORK, idx, d->name);
			if (kind == PATH_IS_DIRECTORY)
			{
				out->path_index = idx;
				out->zipped = 0;
				strcpy(out->game, d->name);
				return 1;
			}
			if (snprintf(archive, sizeof(archive), "%s.zip", d->name) >= (int)sizeof(archive))
				continue;
			kind = osd_get_path_info(FILETYPE_ARTWORK, idx, archive);
			if (kind == PATH_IS_FILE)
			{
				out->path_index = idx;
				out->zipped = 1;
				strcpy(out->game, d->name);
				return 1;
			}
		}
	}
	return 0;
}

mame_file *artwork_package_open(const artwork_package *pkg, const char *relative_name)
{
	if (!pkg || !safe_relative(relative_name))
		return NULL;
	return mame_fopen_artwork_member(pkg->path_index, pkg->game, pkg->zipped, relative_name);
}

