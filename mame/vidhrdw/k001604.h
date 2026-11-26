// license:BSD-3-Clause
// copyright-holders:David Haywood
#ifndef _K001604_H
#define _K001604_H

#include "driver.h"

#define K001604_MAX_CHIPS 2

/* palette_base is the first palette entry used by the chip */
int k001604_start(int chip, int palette_base);
/* position of the visible area in the screen timing, by default the visible area origin */
void k001604_set_layer_offset(int chip, int x, int y);
void k001604_draw_back_layer(int chip, mame_bitmap *bitmap, const rectangle *cliprect);
void k001604_draw_front_layer(int chip, mame_bitmap *bitmap, const rectangle *cliprect);

READ32_HANDLER( k001604_0_tile_r );
WRITE32_HANDLER( k001604_0_tile_w );
READ32_HANDLER( k001604_0_char_r );
WRITE32_HANDLER( k001604_0_char_w );
READ32_HANDLER( k001604_0_reg_r );
WRITE32_HANDLER( k001604_0_reg_w );

READ32_HANDLER( k001604_1_tile_r );
WRITE32_HANDLER( k001604_1_tile_w );
READ32_HANDLER( k001604_1_char_r );
WRITE32_HANDLER( k001604_1_char_w );
READ32_HANDLER( k001604_1_reg_r );
WRITE32_HANDLER( k001604_1_reg_w );

#endif
