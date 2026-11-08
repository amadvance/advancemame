// license:BSD-3-Clause
// copyright-holders:Luca Elia, David Haywood
#include "driver.h"
#include "includes/kaneko16.h"



UINT16 *galpanic_bgvideoram,*galpanic_fgvideoram;
size_t galpanic_fgvideoram_size;
int galpanic_clear_sprites;

static mame_bitmap *sprites_bitmap;
static int galpanic_bitmap_palette_base;

VIDEO_START( galpanic )
{
	galpanic_bitmap_palette_base = 1024;
	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((sprites_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	galpanic_clear_sprites = 1;

	return 0;
}

PALETTE_INIT( galpanic )
{
	int i;

	/* first 1024 colors are dynamic */

	/* initialize 555 RGB lookup */
	for (i = 0;i < 32768;i++)
	{
		int r,g,b;

		r = (i >>  5) & 0x1f;
		g = (i >> 10) & 0x1f;
		b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		palette_set_color(i+1024,r,g,b);
	}
}



WRITE16_HANDLER( galpanic_bgvideoram_w )
{
	int sx,sy;


	data = COMBINE_DATA(&galpanic_bgvideoram[offset]);

	sy = offset / 256;
	sx = offset % 256;

	plot_pixel(tmpbitmap, sx, sy, Machine->pens[galpanic_bitmap_palette_base + (data >> 1)]);
}

WRITE16_HANDLER( galpanic_paletteram_w )
{
	int r,g,b;

	data = COMBINE_DATA(&paletteram16[offset]);

	r = (data >>  6) & 0x1f;
	g = (data >> 11) & 0x1f;
	b = (data >>  1) & 0x1f;
	/* bit 0 seems to be a transparency flag for the front bitmap */

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset,r,g,b);
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void galpanic_draw_sprites(mame_bitmap *bitmap)
{
	int offs;
	int sx,sy;

	sx = sy = 0;
	for (offs = 0;offs < spriteram_size/2;offs += 8)
	{
		int x,y,code,color,flipx,flipy,attr1,attr2;

		attr1 = spriteram16[offs + 3];
		x = spriteram16[offs + 4] - ((attr1 & 0x01) << 8);
		y = spriteram16[offs + 5] + ((attr1 & 0x02) << 7);
		if (attr1 & 0x04)	/* multi sprite */
		{
			sx += x;
			sy += y;
		}
		else
		{
			sx = x;
			sy = y;
		}

		color = (attr1 & 0xf0) >> 4;

		/* bit 0 [offs + 0] is used but I don't know what for */

		attr2 = spriteram16[offs + 7];
		code = spriteram16[offs + 6] + ((attr2 & 0x1f) << 8);
		flipx = attr2 & 0x80;
		flipy = attr2 & 0x40;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				sx,sy - 16,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

static void comad_draw_sprites(mame_bitmap *bitmap)
{
	int offs;
	int sx=0, sy=0;

	for (offs = 0;offs < spriteram_size/2;offs += 4)
	{
		int code,color,flipx,flipy;

		code = spriteram16[offs + 1] & 0x1fff;
		color = (spriteram16[offs] & 0x003c) >> 2;
		flipx = spriteram16[offs] & 0x0002;
		flipy = spriteram16[offs] & 0x0001;

		if((spriteram16[offs] & 0x6000) == 0x6000) /* Link bits */
		{
			sx += spriteram16[offs + 2] >> 6;
			sy += spriteram16[offs + 3] >> 6;
		}
		else
		{
			sx = spriteram16[offs + 2] >> 6;
			sy = spriteram16[offs + 3] >> 6;
		}

		sx = (sx&0x1ff) - (sx&0x200);
		sy = (sy&0x1ff) - (sy&0x200);

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

static void draw_fgbitmap(mame_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < galpanic_fgvideoram_size/2;offs++)
	{
		int sx,sy,color;

		sx = offs % 256;
		sy = offs / 256;
		color = galpanic_fgvideoram[offs];
		if (color)
			plot_pixel(bitmap, sx, sy, Machine->pens[color]);
	}
}

VIDEO_UPDATE( galpanic )
{
	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	draw_fgbitmap(bitmap);

	if(galpanic_clear_sprites)
	{
		fillbitmap(sprites_bitmap,0,cliprect);
		galpanic_draw_sprites(bitmap);
	}
	else
	{
		/* keep sprites on the bitmap without clearing them */
		galpanic_draw_sprites(sprites_bitmap);
		copybitmap(bitmap,sprites_bitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( comad )
{
	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	draw_fgbitmap(bitmap);

	if(galpanic_clear_sprites)
	{
		fillbitmap(sprites_bitmap,0,cliprect);
		comad_draw_sprites(bitmap);
	}
	else
	{
		/* keep sprites on the bitmap without clearing them */
		comad_draw_sprites(sprites_bitmap);
		copybitmap(bitmap,sprites_bitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

UINT16 *galsnew_viewram, *galsnew_viewregs;
static tilemap *galsnew_layers[2];
static UINT16 galsnew_banks[2];

static void galsnew_tile_info(int index, int layer)
{
	UINT16 *ram = galsnew_viewram + (layer ? 0 : 0x800);
	UINT16 attr = ram[index * 2];
	SET_TILE_INFO(1, ram[index * 2 + 1] + galsnew_banks[layer], (attr >> 2) & 0x3f, TILE_FLIPXY(attr & 3));
	tile_info.priority = (attr >> 8) & 7;
}

static void galsnew_tile0(int index) { galsnew_tile_info(index, 0); }
static void galsnew_tile1(int index) { galsnew_tile_info(index, 1); }

WRITE16_HANDLER( galsnew_viewram_w )
{
	COMBINE_DATA(&galsnew_viewram[offset]);
	if (offset < 0x1000)
		tilemap_mark_tile_dirty(galsnew_layers[offset < 0x800 ? 1 : 0], (offset & 0x7ff) / 2);
}

static void galsnew_set_bank(int layer, UINT16 data, UINT16 mem_mask)
{
	if (ACCESSING_LSB && galsnew_banks[layer] != (data & 0xff) * 256)
	{
		galsnew_banks[layer] = (data & 0xff) * 256;
		tilemap_mark_all_tiles_dirty(galsnew_layers[layer]);
	}
}

WRITE16_HANDLER( galsnew_tilebank_w ) { galsnew_set_bank(1, data, mem_mask); }
WRITE16_HANDLER( galsnew_tilebank0_w ) { galsnew_set_bank(0, data, mem_mask); }

PALETTE_INIT( galsnew )
{
	int i;
	for (i = 0; i < 32768; ++i)
	{
		int r = (i >> 5) & 31, g = (i >> 10) & 31, b = i & 31;
		palette_set_color(i + 2048, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
	}
}

VIDEO_START( galsnew )
{
	int layer;
	if (video_start_galpanic() || video_start_kaneko16_sprites())
		return 1;
	galpanic_bitmap_palette_base = 2048;
	galsnew_layers[0] = tilemap_create(galsnew_tile0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	galsnew_layers[1] = tilemap_create(galsnew_tile1, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	if (!galsnew_layers[0] || !galsnew_layers[1])
		return 1;
	for (layer = 0; layer < 2; ++layer)
	{
		tilemap_set_transparent_pen(galsnew_layers[layer], 0);
		tilemap_set_scroll_rows(galsnew_layers[layer], 512);
		tilemap_set_scrolldx(galsnew_layers[layer], -0x5b - layer * 2, 256 + 0x5b + layer * 2 - 1);
		tilemap_set_scrolldy(galsnew_layers[layer], -8, 224 + 8 - 1);
		galsnew_banks[layer] = 0;
		kaneko16_priority.sprite[layer] = 8;
		kaneko16_priority.sprite[layer + 2] = 8;
	}
	kaneko16_sprite_type = kaneko16_sprite_fliptype = 0;
	kaneko16_sprite_xoffs = 0;
	kaneko16_sprite_yoffs = -0x40;
	kaneko16_sprite_flipx = kaneko16_sprite_flipy = 0;
	return 0;
}

VIDEO_UPDATE( galsnew )
{
	int x, y, layer, priority;
	UINT16 control = galsnew_viewregs[4];
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			int index = y * 256 + x;
			int color = galpanic_fgvideoram[index] & 0x7ff;
			plot_pixel(bitmap, x, y, Machine->pens[(paletteram16[color] & 1) ? 2048 + (galpanic_bgvideoram[index] >> 1) : color]);
		}
	for (layer = 0; layer < 2; ++layer)
	{
		int shift = layer ? 0 : 8;
		int reg = layer ? 0 : 2;
		UINT16 *rows = galsnew_viewram + (layer ? 0x1000 : 0x1800);
		tilemap_set_enable(galsnew_layers[layer], !(control & (0x10 << shift)));
		tilemap_set_flip(galsnew_layers[layer], ((control & 0x100) ? TILEMAP_FLIPY : 0) | ((control & 0x200) ? TILEMAP_FLIPX : 0));
		tilemap_set_scrolly(galsnew_layers[layer], 0, galsnew_viewregs[reg + 1] >> 6);
		for (y = 0; y < 512; ++y)
			tilemap_set_scrollx(galsnew_layers[layer], y, (galsnew_viewregs[reg] + ((control & (8 << shift)) ? rows[y] : 0)) >> 6);
	}
	fillbitmap(priority_bitmap, 0, cliprect);
	for (priority = 0; priority < 8; ++priority)
		for (layer = 0; layer < 2; ++layer)
			tilemap_draw(bitmap, cliprect, galsnew_layers[layer], priority, 0);
	kaneko16_draw_sprites(bitmap, cliprect, -1);
}
