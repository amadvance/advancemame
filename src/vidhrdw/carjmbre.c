// license:BSD-3-Clause
// copyright-holders:David Haywood
/* Car Jamboree video hardware. */

#include "driver.h"

UINT8 *carjmbre_videoram;
UINT8 *carjmbre_spriteram;

static tilemap *carjmbre_tilemap;
static UINT8 carjmbre_bgcolor;

/* The PROM drives 1k/470/220 ohm RGB ladders, with no 1k blue input.
 * TTL outputs have a 0.05V low level, a 4V high level and about 50 ohms
 * additional output resistance when high. */
static int carjmbre_resnet(int bits,int blue)
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

PALETTE_INIT( carjmbre )
{
	int rgb[64][3];
	int min_y = 255000, max_y = 0;
	int i;

	for(i = 0; i < 64; i++)
	{
		int y;
		rgb[i][0] = carjmbre_resnet(color_prom[i] & 7,0);
		rgb[i][1] = carjmbre_resnet((color_prom[i] >> 3) & 7,0);
		rgb[i][2] = carjmbre_resnet((color_prom[i] >> 6) & 3,1);
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

WRITE8_HANDLER( carjmbre_bgcolor_w )
{
	UINT8 mask = 1 << offset;
	carjmbre_bgcolor = (carjmbre_bgcolor & ~mask) | ((data & 1) ? 0 : mask);
}

WRITE8_HANDLER( carjmbre_videoram_w )
{
	carjmbre_videoram[offset] = data;
	tilemap_mark_tile_dirty(carjmbre_tilemap,offset & 0x3ff);
}

static void get_tile_info(int tile_index)
{
	int attr = carjmbre_videoram[tile_index | 0x400];
	int code = carjmbre_videoram[tile_index] | (attr << 1 & 0x100);
	SET_TILE_INFO(0,code,attr & 0xf,0)
	tile_info.priority = (code >> 6) & 3;
}

VIDEO_START( carjmbre )
{
	carjmbre_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	if(!carjmbre_tilemap)
		return 1;
	tilemap_set_transparent_pen(carjmbre_tilemap,0);
	carjmbre_bgcolor = 0;
	state_save_register_global(carjmbre_bgcolor);
	return 0;
}

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int i;
	for(i = 0x60 - 4; i >= 0; i -= 4)
	{
		/* The game moves the first sprite to the end when copying its list. */
		int offs = (i - 4 + 0x60) % 0x60;
		int sy = carjmbre_spriteram[offs];
		int code = carjmbre_spriteram[offs + 1];
		int color = carjmbre_spriteram[offs + 2] & 0xf;
		int flipx = (carjmbre_spriteram[offs + 2] >> 6) & 1;
		int flipy = (carjmbre_spriteram[offs + 2] >> 7) & 1;
		int sx = carjmbre_spriteram[offs + 3];

		if(flip_screen)
		{
			sy += 1;
			sx = 233 - sx;
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sy = 241 - sy;
			sx -= 7;
		}

		drawgfx(bitmap,Machine->gfx[1],code,color,flipx,flipy,sx,sy,cliprect,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( carjmbre )
{
	int category;
	fillbitmap(bitmap,Machine->pens[carjmbre_bgcolor],cliprect);
	for(category = 0; category < 4; category++)
		tilemap_draw(bitmap,cliprect,carjmbre_tilemap,category,0);
	draw_sprites(bitmap,cliprect);
	/* Category zero contains the score panel and other high priority tiles. */
	tilemap_draw(bitmap,cliprect,carjmbre_tilemap,0,0);
}
