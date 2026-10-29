// license:BSD-3-Clause
// copyright-holders:David Haywood
#include "driver.h"

static int battlex_scroll_lsb;
static int battlex_scroll_msb;

static tilemap *bg_tilemap;

WRITE8_HANDLER( battlex_palette_w )
{
	int palette_num = offset / 8;
	int color_num = offset & 7;
	int r = (data & 1) ? 255 : 0;
	int g = (data & 4) ? 255 : 0;
	int b = (data & 2) ? 255 : 0;

	palette_set_color(offset, r, g, b);
	palette_set_color(64 + palette_num * 16 + color_num, r, g, b);
	palette_set_color(64 + palette_num * 16 + color_num + 8, r / 3, g / 3, b / 3);
}

WRITE8_HANDLER( battlex_scroll_x_lsb_w )
{
	battlex_scroll_lsb = data;
}

WRITE8_HANDLER( battlex_scroll_x_msb_w )
{
	battlex_scroll_msb = data;
}

WRITE8_HANDLER( battlex_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE8_HANDLER( battlex_flipscreen_w )
{
	/* bit 4 is used, but for what? */

	/* bit 7 is flip screen */

	if (flip_screen != (data & 0x80))
	{
		flip_screen_set(data & 0x80);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int tile = videoram[tile_index*2] | (((videoram[tile_index*2+1] & 0x01)) << 8);
	int color = (videoram[tile_index*2+1] & 0x0e) >> 1;

	SET_TILE_INFO(0,tile,color,0)
}

static void dodgeman_get_bg_tile_info(int tile_index)
{
	int attr = videoram[tile_index * 2 + 1];
	int tile = videoram[tile_index * 2] | ((attr & 3) << 8);
	int color = (attr & 0x0c) >> 2;
	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( dodgeman )
{
	bg_tilemap = tilemap_create(dodgeman_get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 64, 32);
	return bg_tilemap ? 0 : 1;
}

VIDEO_START( battlex )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void battlex_drawsprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	const gfx_element *gfx = Machine->gfx[1];
	UINT8 *source = spriteram;
	UINT8 *finish = spriteram + 0x200;

	while( source<finish )
	{
		int sx = (source[0] & 0x7f) * 2 - (source[0] & 0x80) * 2;
		int sy = source[3];
		int tile = source[2];
		int color = source[1] & 0x07;	/* bits 3,4,5 also used during explosions */
		int flipy = source[1] & 0x80;
		int flipx = source[1] & 0x40;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,gfx,tile,color,flipx,flipy,sx,sy,cliprect,TRANSPARENCY_PEN,0);

		source += 4;
	}

}

VIDEO_UPDATE(battlex)
{
	tilemap_set_scrollx(bg_tilemap, 0, battlex_scroll_lsb | (battlex_scroll_msb << (flip_screen ? 3 : 8)));
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	battlex_drawsprites(bitmap, cliprect);
}
