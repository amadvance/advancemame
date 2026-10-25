// license:BSD-3-Clause
// copyright-holders:Dirk Best, Couriersud, Aaron Giles
/* Popper video hardware. */

#include "driver.h"

UINT8 *popper_videoram, *popper_attribram, *popper_spriteram;
int popper_back_color, popper_vram_page;
static tilemap *popper_layer0, *popper_layer1;

/* The PROM drives 1k/470/220 ohm RGB ladders, with no 1k blue input.
 * TTL outputs have a 0.05V low level, a 4V high level and about 50 ohms
 * additional output resistance when high. */
static int popper_resnet(int bits,int blue)
{
	static const double resistance[3] = { 1000.0, 470.0, 220.0 };
	double current = 0.0, conductance = 0.0;
	int i;

	for(i = blue ? 1 : 0; i < 3; i++)
	{
		int high = (bits >> (i - (blue ? 1 : 0))) & 1;
		double r = resistance[i] + (high ? 50.0 : 0.0);
		conductance += 1.0 / r;
		current += (high ? 4.0 : 0.05) / r;
	}
	return (int)(current / conductance * 255.0 / 5.0 + 0.4);
}

PALETTE_INIT( popper )
{
	int rgb[64][3];
	int min_y = 255000, max_y = 0;
	int i;

	for(i = 0; i < 64; i++)
	{
		int y;
		rgb[i][0] = popper_resnet(color_prom[i] & 7,0);
		rgb[i][1] = popper_resnet((color_prom[i] >> 3) & 7,0);
		rgb[i][2] = popper_resnet((color_prom[i] >> 6) & 3,1);
		y = 299 * rgb[i][0] + 587 * rgb[i][1] + 114 * rgb[i][2];
		min_y = MIN(min_y,y);
		max_y = MAX(max_y,y);
	}

	/* Stretch luminance across the palette while retaining its chroma. */
	for(i = 0; i < 64; i++)
	{
		int y = 299 * rgb[i][0] + 587 * rgb[i][1] + 114 * rgb[i][2];
		int u = (rgb[i][2] - y / 1000) * 492 / 1000;
		int v = (rgb[i][0] - y / 1000) * 877 / 1000;
		int level = max_y > min_y ? (y - min_y) * 256 / (max_y - min_y) : y / 1000;
		int r = level + 1140 * v / 1000;
		int g = level - 395 * u / 1000 - 581 * v / 1000;
		int b = level + 2032 * u / 1000;
		palette_set_color(i,MAX(0,MIN(255,r)),MAX(0,MIN(255,g)),MAX(0,MIN(255,b)));
	}
}

static void layer0_tile_info(int tile_index)
{
	int code = (popper_vram_page << 8) | popper_videoram[tile_index];
	int attr = popper_attribram[tile_index];
	int color = (((~popper_back_color & 1) << 3) | ((attr >> 4) & 7)) << 1;
	SET_TILE_INFO(0, code, color, 0)
	tile_info.priority = ((attr & 0x80) != 0) && (attr & 0x70);
}

static void layer1_tile_info(int tile_index)
{
	int code = (popper_vram_page << 8) | popper_videoram[tile_index];
	int attr = popper_attribram[tile_index];
	SET_TILE_INFO(1, code, attr & 0x0f, 0)
	tile_info.priority = (attr >> 7) & 1;
}

VIDEO_START( popper )
{
	popper_layer0 = tilemap_create(layer0_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8, 8, 48, 32);
	popper_layer1 = tilemap_create(layer1_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8, 8, 48, 32);
	if (!popper_layer0 || !popper_layer1)
		return 1;
	tilemap_set_transparent_pen(popper_layer0, 1);
	tilemap_set_transparent_pen(popper_layer1, 0);
	return 0;
}

VIDEO_UPDATE( popper )
{
	int offs;
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	tilemap_mark_all_tiles_dirty(popper_layer0);
	tilemap_mark_all_tiles_dirty(popper_layer1);
	tilemap_draw(bitmap, cliprect, popper_layer0, 0, 0);
	tilemap_draw(bitmap, cliprect, popper_layer1, 0, 0);

	for (offs = 0; offs < 0x800; offs += 4)
	{
		int sx = popper_spriteram[offs + 3];
		int sy = popper_spriteram[offs];
		int flipx = (popper_spriteram[offs + 2] >> 6) & 1;
		int flipy = (popper_spriteram[offs + 2] >> 7) & 1;

		/* Each 16-line strip has its own sprite slots. */
		if (((sy + (flip_screen ? 2 : 0)) >> 4) != ((~(offs >> 7)) & 0x0f))
			continue;
		sx += 64;
		sy = 240 - sy;
		if (flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
			sx = 232 - sx + 128;
			sy = 242 - sy;
		}
		drawgfx(bitmap, Machine->gfx[2], popper_spriteram[offs + 1],
			popper_spriteram[offs + 2] & 0x0f, flipx, flipy, sx, sy,
			cliprect, TRANSPARENCY_PEN, 0);
	}

	tilemap_draw(bitmap, cliprect, popper_layer0, 1, 0);
	tilemap_draw(bitmap, cliprect, popper_layer1, 1, 0);
}
