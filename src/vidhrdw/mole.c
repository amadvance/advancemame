/***************************************************************************
  vidhrdw/mole.c
  Functions to emulate the video hardware of Mole Attack!.
  Mole Attack's Video hardware is essentially two banks of 512 characters.
  The program uses a single byte to indicate which character goes in each location,
  and uses a control location (0x8400) to select the character sets
***************************************************************************/

#include "driver.h"

static int tile_bank;
static UINT16 *tileram;
static tilemap *bg_tilemap;

PALETTE_INIT( mole )
{
	int i;
	int r, g, b;

	for (i = 0; i < 8; i++) {
		r = (i & 1) ? 0xff : 0x00;
		g = (i & 4) ? 0xff : 0x00;
		b = (i & 2) ? 0xff : 0x00;
		palette_set_color(i, r, g, b);
	}
}

static void get_bg_tile_info(int tile_index)
{
	UINT16 code = tileram[tile_index];
	SET_TILE_INFO((code & 0x200) ? 1 : 0, code & 0x1ff, 0, 0)
}

VIDEO_START( mole )
{
	tileram = (UINT16 *)auto_malloc(0x400 * sizeof(UINT16));

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 40, 25);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

WRITE8_HANDLER( mole_videoram_w )
{
	if (tileram[offset] != data)
	{
		tileram[offset] = data | (tile_bank << 8);
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( mole_tilebank_w )
{
	tile_bank = data;
	tilemap_mark_all_tiles_dirty(bg_tilemap);
}

WRITE8_HANDLER( mole_flipscreen_w )
{
	flip_screen_set(data & 0x01);
}

VIDEO_UPDATE( mole )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
}
