// license:BSD-3-Clause
// copyright-holders:Andrea Mazzoleni
#ifndef ADV_ARTWORK_LAY_H
#define ADV_ARTWORK_LAY_H
#include "driver.h"
#include "mamecore.h"
#include "artwork_package.h"

/* Lay textures are held in canonical 0xAARRGGBB representation. */
#define LAY_BACK 0
#define LAY_FRONT 1

typedef struct artwork_lay_range
{
	double left, top, right, bottom;
} artwork_lay_range;

/* The layout is discarded if its package is malformed. A missing package is
 * normal. All diagnostics are printed to the console, never to the UI. */
void artwork_lay_open(const game_driver *driver, const char *requested_view,
	int game_width, int game_height, int use_mask);
void artwork_lay_close(void);
int artwork_lay_active(void);
int artwork_lay_has_backdrop(void);
int artwork_lay_has_overlay(void);
int artwork_lay_screen_only(void);
int artwork_lay_has_art(void);
int artwork_lay_screen_swapped(void);
int artwork_lay_screen_transformed(void);
void artwork_lay_screen_matrix(double matrix[6]);
void artwork_lay_extents(artwork_lay_range *extents);
/* Called after OSD display allocation and on visible-area changes. */
int artwork_lay_prepare(int screen_width, int screen_height,
	const rectangle *game_rect);
/* Pre-rasterised items, blended directly onto the MAME 32-bit bitmap. */
void artwork_lay_draw(mame_bitmap *dest, int pass,
	unsigned rshift, unsigned gshift, unsigned bshift, unsigned ashift);
#endif

