/***************************************************************************

    Legionnaire / Heated Barrel video hardware (derived from D-Con)

***************************************************************************/

#include "driver.h"

UINT16 *legionna_back_data,*legionna_fore_data,*legionna_mid_data,*legionna_scrollram16,*legionna_textram;

static tilemap *background_layer,*foreground_layer,*midground_layer,*text_layer;
//static int legionna_enable;

/******************************************************************************/

static UINT16 gfx_bank = 0;

void heatbrl_setgfxbank(UINT16 data)
{
	gfx_bank = (data &0x4000) >> 2;
}


WRITE16_HANDLER( legionna_control_w )
{
#if 0
	if (ACCESSING_LSB)
	{
		legionna_enable=data;
		if ((legionna_enable&4)==4)
			tilemap_set_enable(foreground_layer,0);
		else
			tilemap_set_enable(foreground_layer,1);

		if ((legionna_enable&2)==2)
			tilemap_set_enable(midground_layer,0);
		else
			tilemap_set_enable(midground_layer,1);

		if ((legionna_enable&1)==1)
			tilemap_set_enable(background_layer,0);
		else
			tilemap_set_enable(background_layer,1);
	}
#endif
}

WRITE16_HANDLER( legionna_background_w )
{
	int oldword = legionna_back_data[offset];
	COMBINE_DATA(&legionna_back_data[offset]);
	if (oldword != legionna_back_data[offset])
		tilemap_mark_tile_dirty(background_layer,offset);
}

WRITE16_HANDLER( legionna_midground_w )
{
	int oldword = legionna_mid_data[offset];
	COMBINE_DATA(&legionna_mid_data[offset]);
	if (oldword != legionna_mid_data[offset])
		tilemap_mark_tile_dirty(midground_layer,offset);
}

WRITE16_HANDLER( legionna_foreground_w )
{
	int oldword = legionna_fore_data[offset];
	COMBINE_DATA(&legionna_fore_data[offset]);
	if (oldword != legionna_fore_data[offset])
		tilemap_mark_tile_dirty(foreground_layer,offset);
}

WRITE16_HANDLER( legionna_text_w )
{
	int oldword = legionna_textram[offset];
	COMBINE_DATA(&legionna_textram[offset]);
	if (oldword != legionna_textram[offset])
		tilemap_mark_tile_dirty(text_layer,offset);
}

static void get_back_tile_info(int tile_index)
{
	int tile=legionna_back_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;
	tile |= gfx_bank;		/* Heatbrl uses banking */

	SET_TILE_INFO(1,tile,color,0)
}

static void get_mid_tile_info(int tile_index)
{
	int tile=legionna_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	SET_TILE_INFO(5,tile,color,0)
}

static void get_mid_tile_info_cupsoc(int tile_index)
{
	int tile=legionna_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	tile |= 0x1000;

	SET_TILE_INFO(1,tile,color,0)
}

static void get_fore_tile_info(int tile_index)	/* this is giving bad tiles... */
{
	int tile=legionna_fore_data[tile_index];
	int color=(tile>>12)&0xf;

	// legionnaire tile numbers / gfx set wrong, see screen after coin insertion
	tile &= 0xfff;

	SET_TILE_INFO(4,tile,color,0)
}

static void get_text_tile_info(int tile_index)
{
	int tile = legionna_textram[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( legionna )
{
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	midground_layer =  tilemap_create(get_mid_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	text_layer =       tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,32);

	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;

	legionna_scrollram16 = auto_malloc(0x60);

	tilemap_set_transparent_pen(background_layer,15);
	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}

VIDEO_START( cupsoc )
{
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	midground_layer =  tilemap_create(get_mid_tile_info_cupsoc, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	text_layer =       tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,32);

	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;

	legionna_scrollram16 = auto_malloc(0x60);

	tilemap_set_transparent_pen(background_layer,15);
	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}


/*************************************************************************

    Legionnaire Spriteram (similar to Dcon)
    ---------------------

    It has "big sprites" created by setting width or height >0. Tile
    numbers are read consecutively.

    +0   x....... ........  Sprite enable
    +0   .x...... ........  Flip x
    +0   ..x..... ........  Flip y ???
    +0   ...xxx.. ........  Width: do this many tiles horizontally
    +0   ......xx x.......  Height: do this many tiles vertically
    +0   ........ .?......  unused ?
    +0   ........ ..xxxxxx  Color bank

    +1   .x...... ........  Priority? (1=high?)
    +1   ..xxxxxx xxxxxxxx  Tile number
    +2   xxxxxxxx xxxxxxxx  X coordinate (signed)
    +3   xxxxxxxx xxxxxxxx  Y coordinate (signed)

*************************************************************************/

static void draw_sprites(mame_bitmap *bitmap,const rectangle *cliprect,int pri)
{
	int offs,fx,fy,x,y,color,sprite;
	int dx,dy,ax,ay;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		UINT16 data = spriteram16[offs];
		if (!(data &0x8000)) continue;

		sprite = spriteram16[offs+1];
		if ((sprite>>14)!=pri) continue;

		sprite &= 0x3fff;

		y = spriteram16[offs+3];
		x = spriteram16[offs+2];

		if (x &0x8000)	x = -(0x200-(x &0x1ff));
		else	x &= 0x1ff;
		if (y &0x8000)	y = -(0x200-(y &0x1ff));
		else	y &= 0x1ff;

		color = (data &0x3f) + 0x40;
		fx =  (data &0x4000) >> 14;
		fy =  (data &0x2000) >> 13;	/* ??? */
		dy = ((data &0x0380) >> 7)  + 1;
		dx = ((data &0x1c00) >> 10) + 1;

		if (!fx)
		{
			for (ax=0; ax<dx; ax++)
				for (ay=0; ay<dy; ay++)
				{
					drawgfx(bitmap,Machine->gfx[3],
					sprite++,
					color,fx,fy,x+ax*16,y+ay*16,
					cliprect,TRANSPARENCY_PEN,15);
				}
		}
		else
		{
			for (ax=0; ax<dx; ax++)
				for (ay=0; ay<dy; ay++)
				{
					drawgfx(bitmap,Machine->gfx[3],
					sprite++,
					color,fx,fy,x+(dx-ax-1)*16,y+ay*16,
					cliprect,TRANSPARENCY_PEN,15);
				}
		}
	}
}


VIDEO_UPDATE( legionna )
{
#ifdef MAME_DEBUG
	static int dislayer[5];	/* Layer toggles to help get the layers correct */
#endif

#ifdef MAME_DEBUG
	if (code_pressed_memory (KEYCODE_Z))
	{
		dislayer[0] ^= 1;
		ui_popup("bg0: %01x",dislayer[0]);
	}

	if (code_pressed_memory (KEYCODE_X))
	{
		dislayer[1] ^= 1;
		ui_popup("bg1: %01x",dislayer[1]);
	}

	if (code_pressed_memory (KEYCODE_C))
	{
		dislayer[2] ^= 1;
		ui_popup("bg2: %01x",dislayer[2]);
	}

	if (code_pressed_memory (KEYCODE_V))
	{
		dislayer[3] ^= 1;
		ui_popup("sprites: %01x",dislayer[3]);
	}

	if (code_pressed_memory (KEYCODE_B))
	{
		dislayer[4] ^= 1;
		ui_popup("text: %01x",dislayer[4]);
	}
#endif

	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer, 0, legionna_scrollram16[0] );
	tilemap_set_scrolly( background_layer, 0, legionna_scrollram16[1] );
	tilemap_set_scrollx( midground_layer,  0, legionna_scrollram16[2] );
	tilemap_set_scrolly( midground_layer,  0, legionna_scrollram16[3] );
	tilemap_set_scrollx( foreground_layer, 0, legionna_scrollram16[4] );
	tilemap_set_scrolly( foreground_layer, 0, legionna_scrollram16[5] );

//  if ((legionna_enable&1)!=1)

	fillbitmap(bitmap,get_black_pen(),cliprect);	/* wrong color? */

#ifdef MAME_DEBUG
	if (dislayer[2]==0)
#endif
	tilemap_draw(bitmap,cliprect,foreground_layer,TILEMAP_IGNORE_TRANSPARENCY,0);

#ifdef MAME_DEBUG
	if (dislayer[1]==0)
#endif
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);

#ifdef MAME_DEBUG
	if (dislayer[0]==0)
#endif
	tilemap_draw(bitmap,cliprect,background_layer,0,0);

	draw_sprites(bitmap,cliprect,2);
	draw_sprites(bitmap,cliprect,1);
	draw_sprites(bitmap,cliprect,0);
	draw_sprites(bitmap,cliprect,3);

#ifdef MAME_DEBUG
	if (dislayer[4]==0)
#endif
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}

VIDEO_UPDATE( godzilla )
{
	tilemap_set_scrollx( text_layer, 0, 0 );
	tilemap_set_scrolly( text_layer, 0, 112 );

	fillbitmap(bitmap,get_black_pen(),cliprect);

	tilemap_draw(bitmap,cliprect,background_layer,0,0);
	draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);
	draw_sprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,foreground_layer,0,0);
	draw_sprites(bitmap,cliprect,0);
	draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}

VIDEO_UPDATE( sdgndmrb )
{
	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer, 0, legionna_scrollram16[0] );
	tilemap_set_scrolly( background_layer, 0, legionna_scrollram16[1] );
	tilemap_set_scrollx( midground_layer,  0, legionna_scrollram16[2] );
	tilemap_set_scrolly( midground_layer,  0, legionna_scrollram16[3] );
	tilemap_set_scrollx( foreground_layer, 0, legionna_scrollram16[4] );
	tilemap_set_scrolly( foreground_layer, 0, legionna_scrollram16[5] );
//  tilemap_set_scrollx( text_layer, 0, 128 /* legionna_scrollram16[6] */);
//  tilemap_set_scrolly( text_layer, 0, 0 /* legionna_scrollram16[7] */ );

	fillbitmap(bitmap,get_black_pen(),cliprect);

	tilemap_draw(bitmap,cliprect,background_layer,0,0);
	draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);
	draw_sprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,foreground_layer,0,0);
	draw_sprites(bitmap,cliprect,0);
	draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}


UINT16 legionna_layer_disable;
static UINT16 *legionna_dma_palette;

WRITE16_HANDLER( legionna_dma_video_w )
{
	if (offset < 0x400)
		legionna_background_w(offset, data, mem_mask);
	else if (offset < 0x800)
		legionna_foreground_w(offset - 0x400, data, mem_mask);
	else if (offset < 0xc00)
		legionna_midground_w(offset - 0x800, data, mem_mask);
	else if (offset < 0x1400)
		legionna_text_w(offset - 0xc00, data, mem_mask);
}

WRITE16_HANDLER( legionna_dma_palette_w )
{
	int val, r, g, b;
	COMBINE_DATA(&legionna_dma_palette[offset]);
	val = legionna_dma_palette[offset];
	r = val & 31;
	g = (val >> 5) & 31;
	b = (val >> 10) & 31;
	palette_set_color(offset, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
}

static void cop_get_mid_tile_info(int tile_index)
{
	int val = legionna_mid_data[tile_index];
	SET_TILE_INFO(1, (val & 0xfff) | 0x1000, ((val >> 12) & 15) | 16, 0)
}

static void cop_get_fore_tile_info(int tile_index)
{
	int val = legionna_fore_data[tile_index];
	SET_TILE_INFO(2, val & 0xfff, (val >> 12) & 15, 0)
}

VIDEO_START( legioncop )
{
	legionna_back_data = auto_malloc(0x800);
	legionna_fore_data = auto_malloc(0x800);
	legionna_mid_data = auto_malloc(0x800);
	legionna_textram = auto_malloc(0x1000);
	legionna_dma_palette = auto_malloc(0x1000);
	legionna_scrollram16 = auto_malloc(0x60);
	memset(legionna_back_data, 0, 0x800);
	memset(legionna_fore_data, 0, 0x800);
	memset(legionna_mid_data, 0, 0x800);
	memset(legionna_textram, 0, 0x1000);
	memset(legionna_dma_palette, 0, 0x1000);
	gfx_bank = 0;
	background_layer = tilemap_create(get_back_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16,32,32);
	foreground_layer = tilemap_create(cop_get_fore_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16,32,32);
	midground_layer = tilemap_create(cop_get_mid_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16,32,32);
	text_layer = tilemap_create(get_text_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8,64,32);
	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;
	tilemap_set_transparent_pen(background_layer, 15);
	tilemap_set_transparent_pen(foreground_layer, 15);
	tilemap_set_transparent_pen(midground_layer, 15);
	tilemap_set_transparent_pen(text_layer, 15);
	return 0;
}

static void cop_draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int gundam)
{
	static const UINT32 legionna_mask[4] = { 0, 0xfff0, 0xfffc, 0xfffe };
	static const UINT32 grainbow_mask[4] = { 0xfff0, 0xfffc, 0xfffe, 0 };
	int offs;
	for (offs = 0; offs < spriteram_size/2; offs += 4)
	{
		int attr = spriteram16[offs];
		int code = spriteram16[offs+1] & 0x3fff;
		int pri = spriteram16[offs+1] >> 14;
		int x = spriteram16[offs+2] & 0x1ff;
		int y = spriteram16[offs+3] & 0x1ff;
		int fx = (attr >> 14) & 1, fy = (attr >> 13) & 1;
		int dx = ((attr >> 10) & 7) + 1, dy = ((attr >> 7) & 7) + 1;
		int ax, ay;
		UINT32 mask;
		if (!(attr & 0x8000)) continue;
		if (x >= 0x180) x -= 0x200;
		if (y >= 0x180) y -= 0x200;
		if (gundam) {
			x += 16;
			y += 16;
			pri = (pri & 2) | ((attr >> 6) & 1);
		}
		mask = gundam ? grainbow_mask[pri] : legionna_mask[pri];
		for (ax = 0; ax < dx; ax++)
			for (ay = 0; ay < dy; ay++)
				pdrawgfx(bitmap, Machine->gfx[3], code++, attr & 63, fx, fy,
					x + 16 * (fx ? dx-ax-1 : ax), y + 16 * (fy ? dy-ay-1 : ay),
					cliprect, TRANSPARENCY_PEN, 15, mask);
	}
}

static void cop_video_update(mame_bitmap *bitmap, const rectangle *cliprect, int gundam)
{
	tilemap_set_scrollx(background_layer, 0, legionna_scrollram16[0]);
	tilemap_set_scrolly(background_layer, 0, legionna_scrollram16[1]);
	tilemap_set_scrollx(midground_layer, 0, legionna_scrollram16[2]);
	tilemap_set_scrolly(midground_layer, 0, legionna_scrollram16[3]);
	tilemap_set_scrollx(foreground_layer, 0, legionna_scrollram16[4]);
	tilemap_set_scrolly(foreground_layer, 0, legionna_scrollram16[5]);
	fillbitmap(bitmap, get_black_pen(), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);
	if (gundam) {
		if (!(legionna_layer_disable & 1)) tilemap_draw(bitmap, cliprect, background_layer, 0, 0);
		if (!(legionna_layer_disable & 2)) tilemap_draw(bitmap, cliprect, midground_layer, 0, 1);
	} else {
		if (!(legionna_layer_disable & 1)) tilemap_draw(bitmap, cliprect, midground_layer, 0, 0);
		if (!(legionna_layer_disable & 2)) tilemap_draw(bitmap, cliprect, background_layer, 0, 1);
	}
	if (!(legionna_layer_disable & 4)) tilemap_draw(bitmap, cliprect, foreground_layer, 0, 2);
	if (!(legionna_layer_disable & 8)) tilemap_draw(bitmap, cliprect, text_layer, 0, 4);
	if (!(legionna_layer_disable & 16)) cop_draw_sprites(bitmap, cliprect, gundam);
}

VIDEO_UPDATE( legioncop )
{
	cop_video_update(bitmap, cliprect, 0);
}

VIDEO_UPDATE( grainbow )
{
	cop_video_update(bitmap, cliprect, 1);
}
