// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/******************************************************************************

    Flower Video Hardware

******************************************************************************/

#include "driver.h"

static tilemap *bg_tilemap;
static tilemap *fg_tilemap;
static tilemap *tx_tilemap;
static mame_bitmap *temp_bitmap;

UINT8 *flower_workram;
UINT8 *flower_txvram;
UINT8 *flower_bgvram;
UINT8 *flower_fgvram;
UINT8 *flower_bgscroll;
UINT8 *flower_fgscroll;
int flower_flipscreen;

static void get_bg_tile_info(int tile_index)
{
	UINT32 code = flower_bgvram[tile_index];
	UINT32 color = (flower_bgvram[tile_index + 0x100] & 0xf0) >> 4;

	SET_TILE_INFO(1, code, color, 0);
}

static void get_fg_tile_info(int tile_index)
{
	UINT32 code = flower_fgvram[tile_index];
	UINT32 color = (flower_fgvram[tile_index + 0x100] & 0xf0) >> 4;

	SET_TILE_INFO(1, code, color, 0);
}

static void get_tx_tile_info(int tile_index)
{
	UINT32 code = flower_txvram[tile_index];
	UINT32 color = (flower_txvram[tile_index + 0x400] & 0xfc) >> 2;

	SET_TILE_INFO(0, code, color, 0);
}

// convert from 32x32 to 36x28, similar as Namco hardware
static UINT32 flower_tilemap_scan(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	row += 2;
	col -= 2;
	if (col & 0x20)
		return ((col & 0x1f) << 5) + row;
	else
		return (row << 5) + col;
}

VIDEO_START( flower )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 16, 16);
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 16, 16);
	tx_tilemap = tilemap_create(get_tx_tile_info, flower_tilemap_scan, TILEMAP_TRANSPARENT, 8, 8, 36, 28);

	if (!bg_tilemap || !fg_tilemap || !tx_tilemap)
		return 1;

	temp_bitmap = auto_bitmap_alloc(384, 264);
	if (!temp_bitmap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(tx_tilemap, 3);

	tilemap_set_scrolldx(bg_tilemap, 16, 0);
	tilemap_set_scrolldx(fg_tilemap, 16, 0);
	tilemap_set_scrolldy(tx_tilemap, 16, 0);

	return 0;
}

PALETTE_INIT( flower )
{
	int i;

	for (i = 0; i < 256; i++)
	{
		int bit0, bit1, bit2, bit3;
		int r, g, b;

		// red component
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		// green component
		bit0 = (color_prom[i + 0x100] >> 0) & 0x01;
		bit1 = (color_prom[i + 0x100] >> 1) & 0x01;
		bit2 = (color_prom[i + 0x100] >> 2) & 0x01;
		bit3 = (color_prom[i + 0x100] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		// blue component
		bit0 = (color_prom[i + 0x200] >> 0) & 0x01;
		bit1 = (color_prom[i + 0x200] >> 1) & 0x01;
		bit2 = (color_prom[i + 0x200] >> 2) & 0x01;
		bit3 = (color_prom[i + 0x200] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i, r, g, b);
		colortable[i] = i;
	}
}

/*
 [0] YYYY YYYY Y offset
 [1] YXoo oooo Flip Y/X, tile number
 [2] ---- b--b tile bank select
 [3] Yyyy Xxxx Y size, Y zoom, X size, X zoom
 [4] xxxx xxxx X offset LSB
 [5] XXXX XXXX X offset MSB
 [6] cccc ---- color base
 */
static void flower_draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const UINT8 *spr_ptr = &flower_workram[0x1e08];
	const gfx_element *gfx_2 = Machine->gfx[2];
	int i;

	// traverse from top to bottom
	for (i = 0x1f0; i >= 0; i -= 8)
	{
		UINT32 tile = (spr_ptr[i + 1] & 0x3f);
		const UINT32 color = spr_ptr[i + 6] >> 4;
		int x = (spr_ptr[i + 4] | (spr_ptr[i + 5] << 8)) - 39;
		int y = 241 - spr_ptr[i + 0];
		const UINT8 attr = spr_ptr[i + 2];
		const int fy = spr_ptr[i + 1] & 0x80;
		const int fx = spr_ptr[i + 1] & 0x40;
		const UINT8 ysize = ((spr_ptr[i + 3] & 0x80) >> 7) + 1;
		const UINT8 xsize = ((spr_ptr[i + 3] & 0x08) >> 3) + 1;
		const UINT8 ydiv = (ysize == 2) ? 1 : 2;
		const UINT8 xdiv = (xsize == 2) ? 1 : 2;
		UINT32 yshrink_zoom = ((spr_ptr[i + 3] & 0x70) >> 4) + 1;
		UINT32 xshrink_zoom = ((spr_ptr[i + 3] & 0x07) >> 0) + 1;
		int ypixels, xpixels;
		int yi, xi;

		yshrink_zoom <<= 13;
		xshrink_zoom <<= 13;
		ypixels = (yshrink_zoom * 16) >> 16;
		xpixels = (xshrink_zoom * 16) >> 16;

		tile |= (attr & 1) << 6;
		tile |= (attr & 8) << 4;

		if (flower_flipscreen)
		{
			x += xsize * 16;
			x = 288 - x;
			y -= 2;
		}

		if (ysize == 2)
			y -= 16;

		for (yi = 0; yi < ysize; yi++)
		{
			int yoffs = (16 - ypixels) / ydiv;

			for (xi = 0; xi < xsize; xi++)
			{
				int tile_offs;
				int xoffs = (16 - xpixels) / xdiv;

				tile_offs = fx ? (xsize - xi - 1) * 8 : xi * 8;
				tile_offs += fy ? (ysize - yi - 1) : yi;

				drawgfxzoom(bitmap, gfx_2,
					tile + tile_offs, color,
					fx, fy,
					x + xi * xpixels + xoffs, y + yi * ypixels + yoffs,
					cliprect, TRANSPARENCY_PEN, 15,
					xshrink_zoom, yshrink_zoom);
			}
		}
	}
}

VIDEO_UPDATE( flower )
{
	tilemap_set_scrolly(bg_tilemap, 0, flower_bgscroll[0]);
	tilemap_set_scrolly(fg_tilemap, 0, flower_fgscroll[0]);

	fillbitmap(temp_bitmap, 0, cliprect);
	tilemap_draw(temp_bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(temp_bitmap, cliprect, fg_tilemap, 0, 0);
	flower_draw_sprites(temp_bitmap, cliprect);
	tilemap_draw(temp_bitmap, cliprect, tx_tilemap, 0, 0);

	copybitmap(bitmap, temp_bitmap, flower_flipscreen, flower_flipscreen,
		flower_flipscreen ? -96 : 0, flower_flipscreen ? -8 : 0,
		cliprect, TRANSPARENCY_NONE, 0);
}

WRITE8_HANDLER( flower_txvram_w )
{
	flower_txvram[offset] = data;
	tilemap_mark_tile_dirty(tx_tilemap, offset & 0x3ff);
}

WRITE8_HANDLER( flower_bgvram_w )
{
	flower_bgvram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset & 0xff);
}

WRITE8_HANDLER( flower_fgvram_w )
{
	flower_fgvram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset & 0xff);
}

WRITE8_HANDLER( flower_flipscreen_w )
{
	flower_flipscreen = data & 1;
}
