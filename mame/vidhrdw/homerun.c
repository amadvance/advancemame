// license:LGPL-2.1+
// copyright-holders:Tomasz Slanina
#include "driver.h"

static int homerun_gfx_ctrl;
int homerun_xpa,homerun_xpb,homerun_xpc;

tilemap *homerun_tilemap;
UINT8 *homerun_videoram;

void homerun_video_reset(void)
{
	homerun_gfx_ctrl = 0;
	memory_set_bankptr(1, memory_region(REGION_CPU1));
	tilemap_mark_all_tiles_dirty(homerun_tilemap);
}

WRITE8_HANDLER(homerun_banking_w)
{
	if (homerun_tilemap && ((homerun_gfx_ctrl ^ data) & 3))
		force_partial_update(cpu_getscanline());
	if (homerun_tilemap && ((homerun_gfx_ctrl ^ data) & 1))
		tilemap_mark_all_tiles_dirty(homerun_tilemap);
	homerun_gfx_ctrl = data;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + ((data >> 5) & 7) * 0x4000);
}

WRITE8_HANDLER( homerun_videoram_w )
{

	homerun_videoram[offset]=data;
	tilemap_mark_tile_dirty(homerun_tilemap,offset&0xfff);
}

WRITE8_HANDLER(homerun_color_w)
{
	int r,g,b;
	int bit0,bit1,bit2;
	bit0 = (data >> 0) & 0x01;
	bit1 = (data >> 1) & 0x01;
	bit2 = (data >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (data >> 3) & 0x01;
	bit1 = (data >> 4) & 0x01;
	bit2 = (data >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = 0;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	palette_set_color(offset,r,g,b);
}

static void get_homerun_tile_info(int tile_index)
{
	int tileno,palno;
	tileno = (homerun_videoram[tile_index])+((homerun_videoram[tile_index+0x1000]&0x38)<<5)+ ((homerun_gfx_ctrl&1)<<11);
	palno=(homerun_videoram[tile_index+0x1000]&0x7);
	SET_TILE_INFO(0,tileno,palno,0)
}



VIDEO_START(homerun)
{
	homerun_tilemap = tilemap_create(get_homerun_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,64,64);
	return 0;
}

static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs;
	for (offs = spriteram_size-4; offs >=0; offs -= 4)
	{
		int code,color,sx,sy,flipx,flipy;
		sx = spriteram[offs+3];
		sy = spriteram[offs+0]-15;
		if (!spriteram[offs] || sy < 0)
			continue;
		code = (spriteram[offs+1] ) +((spriteram[offs+2]&0x8)<<5 )+((homerun_gfx_ctrl&3)<<9);
		color = (spriteram[offs+2] & 0x7)+8 ;
		flipx=(spriteram[offs+2] & 0x40) ;
		flipy=(spriteram[offs+2] & 0x80) ;
		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0);
		drawgfx(bitmap,Machine->gfx[1],code,color,flipx,flipy,
			sx-256,sy,cliprect,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE(homerun)
{
	tilemap_set_scrollx(homerun_tilemap, 0, homerun_xpc+((homerun_xpa&2)<<7));
	tilemap_set_scrolly(homerun_tilemap, 0, homerun_xpb+((homerun_xpa&1)<<8));
	tilemap_draw(bitmap,cliprect,homerun_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
}
