// license:BSD-3-Clause
// copyright-holders:David Haywood
/* vidhrdw/stlforce.c - see main driver for other notes */

#include "driver.h"

static tilemap *stlforce_bg_tilemap, *stlforce_mlow_tilemap, *stlforce_mhigh_tilemap, *stlforce_tx_tilemap;

extern UINT16 *stlforce_bg_videoram, *stlforce_mlow_videoram, *stlforce_mhigh_videoram, *stlforce_tx_videoram;
extern UINT16 *stlforce_bg_scrollram, *stlforce_mlow_scrollram, *stlforce_mhigh_scrollram, *stlforce_vidattrram;

extern UINT16 *stlforce_spriteram;

int stlforce_sprxoffs;
static int mortalr_video;
static UINT16 mortalr_sprites[0x400];
static int mortalr_sprite_phase;

/* background, appears to be the bottom layer */

static void get_stlforce_bg_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_bg_videoram[tile_index] & (mortalr_video ? 0x1fff : 0x0fff);
	colour = stlforce_bg_videoram[tile_index] & 0xe000;
	colour = colour >> 13;
	SET_TILE_INFO(0,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_bg_videoram_w )
{
	if (stlforce_bg_videoram[offset] != data)
	{
		stlforce_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_bg_tilemap,offset);
	}
}

/* middle layer, low */

static void get_stlforce_mlow_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_mlow_videoram[tile_index] & (mortalr_video ? 0x1fff : 0x0fff);
	colour = stlforce_mlow_videoram[tile_index] & 0xe000;
	colour = colour >> 13;
	colour += 8;
	tileno += mortalr_video ? 0x2000 : 0x1000;

	SET_TILE_INFO(0,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_mlow_videoram_w )
{
	if (stlforce_mlow_videoram[offset] != data)
	{
		stlforce_mlow_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_mlow_tilemap,offset);
	}
}

/* middle layer, high */

static void get_stlforce_mhigh_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_mhigh_videoram[tile_index] & (mortalr_video ? 0x1fff : 0x0fff);
	colour = stlforce_mhigh_videoram[tile_index] & 0xe000;
	colour = colour >> 13;
	colour += 16;
	tileno += mortalr_video ? 0x4000 : 0x2000;

	SET_TILE_INFO(0,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_mhigh_videoram_w )
{
	if (stlforce_mhigh_videoram[offset] != data)
	{
		stlforce_mhigh_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_mhigh_tilemap,offset);
	}
}

/* text layer, appears to be the top layer */

static void get_stlforce_tx_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_tx_videoram[tile_index] & 0x0fff;
	colour = stlforce_tx_videoram[tile_index] & 0xe000;
	colour = colour >> 13;

	tileno += mortalr_video ? 0x18000 : 0xc000;

	colour += 24;
	SET_TILE_INFO(1,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_tx_videoram_w )
{
	if (stlforce_tx_videoram[offset] != data)
	{
		stlforce_tx_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_tx_tilemap,offset);
	}
}

/* sprites - quite a bit still needs doing .. */

static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{

	const UINT16 *source = stlforce_spriteram+0x0;
	const UINT16 *finish = stlforce_spriteram+0x800;
	const gfx_element *gfx = Machine->gfx[2];
	int ypos, xpos, attr, num;

	while( source<finish )
	{
		if(source[0] & 0x0800)
		{
			ypos = source[0]& 0x01ff;
			attr = source[1]& 0x000f;
			xpos = source[3]& 0x03ff;
			num = (source[2] & 0x1fff);

			ypos = 512-ypos;

			drawgfx( bitmap,
					 gfx,
					 num,
					 64+attr,
					 0,0,
					 xpos+stlforce_sprxoffs,ypos,
					 cliprect,
					 TRANSPARENCY_PEN,0 );
		}

		source += 0x4;
	}
}

VIDEO_UPDATE( stlforce )
{
	int i;
	if(stlforce_vidattrram[6] & 1)
	{
		for(i=0;i<256;i++)
			tilemap_set_scrollx(stlforce_bg_tilemap, i, stlforce_bg_scrollram[i]+9); //+9 for twinbrat
	}
	else
	{
		for(i=0;i<256;i++)
			tilemap_set_scrollx(stlforce_bg_tilemap, i, stlforce_bg_scrollram[0]+9); //+9 for twinbrat
	}

	if(stlforce_vidattrram[6] & 4)
	{
		for(i=0;i<256;i++)
			tilemap_set_scrollx(stlforce_mlow_tilemap, i, stlforce_mlow_scrollram[i]+8);
	}
	else
	{
		for(i=0;i<256;i++)
			tilemap_set_scrollx(stlforce_mlow_tilemap, i, stlforce_mlow_scrollram[0]+8);
	}

	if(stlforce_vidattrram[6] & 0x10)
	{
		for(i=0;i<256;i++)
			tilemap_set_scrollx(stlforce_mhigh_tilemap, i, stlforce_mhigh_scrollram[i]+8);
	}
	else
	{
		for(i=0;i<256;i++)
			tilemap_set_scrollx(stlforce_mhigh_tilemap, i, stlforce_mhigh_scrollram[0]+8);
	}

	tilemap_set_scrolly(stlforce_bg_tilemap, 0, stlforce_vidattrram[1]);
	tilemap_set_scrolly(stlforce_mlow_tilemap, 0, stlforce_vidattrram[2]);
	tilemap_set_scrolly(stlforce_mhigh_tilemap, 0, stlforce_vidattrram[3]);

	tilemap_set_scrollx(stlforce_tx_tilemap, 0, stlforce_vidattrram[0]+8);
	tilemap_set_scrolly(stlforce_tx_tilemap, 0, stlforce_vidattrram[4]);

	tilemap_draw(bitmap,cliprect,stlforce_bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,stlforce_mlow_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,stlforce_mhigh_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,stlforce_tx_tilemap,0,0);
}

VIDEO_START( stlforce )
{
	mortalr_video = (Machine->gamedrv->name && !strcmp(Machine->gamedrv->name, "mortalr"));
	mortalr_sprite_phase = 0;
	memset(mortalr_sprites, 0, sizeof(mortalr_sprites));
	stlforce_bg_tilemap    = tilemap_create(get_stlforce_bg_tile_info,   tilemap_scan_cols,TILEMAP_OPAQUE,      16,16,64,16);
	stlforce_mlow_tilemap  = tilemap_create(get_stlforce_mlow_tile_info, tilemap_scan_cols,TILEMAP_TRANSPARENT, 16,16,64,16);
	stlforce_mhigh_tilemap = tilemap_create(get_stlforce_mhigh_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT, 16,16,64,16);
	stlforce_tx_tilemap    = tilemap_create(get_stlforce_tx_tile_info,   tilemap_scan_rows,TILEMAP_TRANSPARENT,  8, 8,64,32);

	tilemap_set_transparent_pen(stlforce_mlow_tilemap,0);
	tilemap_set_transparent_pen(stlforce_mhigh_tilemap,0);
	tilemap_set_transparent_pen(stlforce_tx_tilemap,0);

	tilemap_set_scroll_rows(stlforce_bg_tilemap, 256);
	tilemap_set_scroll_rows(stlforce_mlow_tilemap, 256);
	tilemap_set_scroll_rows(stlforce_mhigh_tilemap, 256);

	return 0;
}

WRITE16_HANDLER( mortalr_sprite_command_w )
{
	if (mortalr_sprite_phase)
	{
		if ((data & 0x0f) == 0)
			memset(mortalr_sprites, 0, sizeof(mortalr_sprites));
		else if ((data & 0x0f) != 0x0d)
			memcpy(mortalr_sprites, stlforce_spriteram, sizeof(mortalr_sprites));
	}
	mortalr_sprite_phase ^= 1;
}

VIDEO_UPDATE( mortalr )
{
	int i, j, offset;
	const UINT16 *source;
	static const int priority[] = { 2, 4, 12, 14 };

	for (i = 0; i < 256; i++)
	{
		tilemap_set_scrollx(stlforce_bg_tilemap, i, stlforce_bg_scrollram[(stlforce_vidattrram[6] & 1) ? i : 0] + 18);
		tilemap_set_scrollx(stlforce_mlow_tilemap, i, stlforce_mlow_scrollram[(stlforce_vidattrram[6] & 4) ? i : 0] + 17);
		tilemap_set_scrollx(stlforce_mhigh_tilemap, i, stlforce_mhigh_scrollram[(stlforce_vidattrram[6] & 0x10) ? i : 0] + 16);
	}
	tilemap_set_scrolly(stlforce_bg_tilemap, 0, stlforce_vidattrram[1] + 1);
	tilemap_set_scrolly(stlforce_mlow_tilemap, 0, stlforce_vidattrram[2] + 1);
	tilemap_set_scrolly(stlforce_mhigh_tilemap, 0, stlforce_vidattrram[3] + 1);
	tilemap_set_scrollx(stlforce_tx_tilemap, 0, stlforce_vidattrram[0] + 15);
	tilemap_set_scrolly(stlforce_tx_tilemap, 0, stlforce_vidattrram[4] + 1);
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);
	if (stlforce_vidattrram[5] & 1) tilemap_draw(bitmap, cliprect, stlforce_bg_tilemap, 0, 1);
	if (stlforce_vidattrram[5] & 2) tilemap_draw(bitmap, cliprect, stlforce_mlow_tilemap, 0, 2);
	if (stlforce_vidattrram[5] & 4) tilemap_draw(bitmap, cliprect, stlforce_mhigh_tilemap, 0, 4);
	if (stlforce_vidattrram[5] & 8) tilemap_draw(bitmap, cliprect, stlforce_tx_tilemap, 0, 16);
	if (!(stlforce_vidattrram[5] & 16)) return;
	for (offset = 0x3fc; offset >= 0; offset -= 4)
	{
		source = mortalr_sprites + offset;
		if (source[0] & 0x0800)
		{
			int x = (source[3] & 0x3ff) - 7;
			int y = 0x1ff - (source[0] & 0x1ff);
			int mask = ~((1 << (priority[(source[1] >> 4) & 3] + 1)) - 1);
			for (i = 0; i <= (source[0] >> 12); i++)
				for (j = 0; j < 4; j++)
					pdrawgfx(bitmap, Machine->gfx[2], source[2] + i, 64 + (source[1] & 15), source[1] & 0x200, 0,
						x - ((j & 1) ? 1024 : 0), y + i * 16 - ((j & 2) ? 512 : 0), cliprect, TRANSPARENCY_PEN, 0, mask);
		}
	}
}
