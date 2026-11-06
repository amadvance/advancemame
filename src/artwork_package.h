// license:BSD-3-Clause
// copyright-holders:Andrea Mazzoleni
#ifndef ADV_ARTWORK_PACKAGE_H
#define ADV_ARTWORK_PACKAGE_H
#include "driver.h"
#include "fileio.h"

typedef struct artwork_package
{
	int path_index;
	int zipped;
	char game[128];
} artwork_package;

/* 1=found, 0=not found, -1=selected package unreadable */
int artwork_package_find(const game_driver *driver, artwork_package *out);
mame_file *artwork_package_open(const artwork_package *pkg, const char *relative_name);

#endif

