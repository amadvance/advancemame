// license:BSD-3-Clause
// copyright-holders:Bryan McPhail
/*******************************************************************************

    Todo:
        There are some kind of resistors hooked up to the background colours,
        the same prom colours can change for the background but not the
        foreground.  It's rarely used (Liberation title screen only?).

    Emulation by Bryan McPhail, mish@tendril.co.uk

*******************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"

static int background_color,background_disable;
static tilemap *background_tilemap, *fix_tilemap;
static UINT8 deco16_io_ram[16];
UINT8 *prosport_bg_vram;
static UINT8 prosport_char_dirty[1024], prosport_sprite_dirty[256];
static int prosport_gfx_dirty;

#if 0
void debug_print(mame_bitmap *bitmap)
{
	int i,j;
	char buf[20*16];
	char *bufptr = buf;
	for (i = 0;i < 16;i+=2)
		bufptr += sprintf(bufptr,"%04X",deco16_io_ram[i+1]|(deco16_io_ram[i]<<8));
	ui_draw_text(buf, 10, 6*6);
}
#endif

static UINT32 back_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return ((row & 0xf)) + ((15-(col &0xf))<<4) + ((row&0x10)<<5) + ((col&0x10)<<4);
}

static UINT32 fix_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x1f) + ((31-(col &0x1f))<<5);
}

static void get_back_tile_info( int tile_index )
{
	const UINT8 *RAM = memory_region(REGION_USER1);
	int tile,bank;

	/* Convert tile index of 512x512 to paged format */
	if (tile_index&0x100) {
		if (tile_index&0x200)
			tile_index=(tile_index&0xff)+(deco16_io_ram[5]<<8); /* Bottom right */
		else
			tile_index=(tile_index&0xff)+(deco16_io_ram[4]<<8); /* Bottom left */
	} else {
			if (tile_index&0x200)
			tile_index=(tile_index&0xff)+(deco16_io_ram[3]<<8); /* Top right */
		else
			tile_index=(tile_index&0xff)+(deco16_io_ram[2]<<8); /* Top left */
	}

	tile=RAM[tile_index];
	if (tile>0x7f) bank=3; else bank=2;
	SET_TILE_INFO(bank,tile&0x7f,background_color,0)
}

static void prosport_get_back_tile_info(int tile_index)
{
	int tile = (prosport_bg_vram[tile_index] >> 4) + ((tile_index & 8) ? 0x10 : 0);
	tile += deco16_io_ram[0] & 0x20;
	SET_TILE_INFO(8,tile,0,0)
}

static void get_fix_tile_info( int tile_index )
{
	int tile,color;

	tile=videoram[tile_index+0x400]+((videoram[tile_index]&0x7)<<8);
	color=(videoram[tile_index]&0x70)>>4;

//if (tile&0x300) tile-=0x000;
//else if(tile&0x200) tile-=0x100;
//else if (tile&0x100) tile-=0x100;
//else tile+=0x200;

	SET_TILE_INFO(0,tile,color,0)
}

/***************************************************************************/

WRITE8_HANDLER( deco16_io_w )
{
	deco16_io_ram[offset]=data;
	if (offset>1 && offset<6)
		tilemap_mark_all_tiles_dirty(background_tilemap);

	switch (offset) {
		case 6: /* Background colour */
			if (((data>>4)&3)!=background_color) {
				background_color=(data>>4)&3;
				tilemap_mark_all_tiles_dirty(background_tilemap);
			}
			background_disable=data&0x4;
			flip_screen_set(data&0x1);
			break;
		case 7: /* Background palette resistors? */
			/* Todo */
			break;
		case 8: /* Irq ack */
			cpunum_set_input_line(0,DECO16_IRQ_LINE,CLEAR_LINE);
			break;
		case 9: /* Sound */
			soundlatch_w(0,data);
			cpunum_set_input_line(1,M6502_IRQ_LINE,HOLD_LINE);
			break;
	}
}

void prosport_video_reset(void)
{
	memset(deco16_io_ram, 0, sizeof(deco16_io_ram));
	flip_screen_set(0);
	tilemap_mark_all_tiles_dirty(background_tilemap);
}

WRITE8_HANDLER( prosport_io_w )
{
	UINT8 previous = deco16_io_ram[offset];
	deco16_io_ram[offset] = data;
	switch (offset)
	{
		case 0:
			flip_screen_set(data & 0x80);
			if ((previous ^ data) & 0x20) tilemap_mark_all_tiles_dirty(background_tilemap);
			break;
		case 2:
			soundlatch_w(0, data);
			cpunum_set_input_line(1, M6502_IRQ_LINE, HOLD_LINE);
			break;
		case 4:
			cpunum_set_input_line(0, DECO16_IRQ_LINE, CLEAR_LINE);
			break;
	}
}

WRITE8_HANDLER( prosport_bg_vram_w )
{
	prosport_bg_vram[offset] = data;
	tilemap_mark_tile_dirty(background_tilemap, offset);
}

READ8_HANDLER( prosport_charram_r )
{
	return memory_region(REGION_GFX6)[(offset / 0x800) * 0x2000 + 0x800 + (offset & 0x7ff)];
}

WRITE8_HANDLER( prosport_charram_w )
{
	UINT8 *gfx = memory_region(REGION_GFX6);
	int address = (offset / 0x800) * 0x2000 + 0x800 + (offset & 0x7ff);
	if (gfx[address] == data) return;
	gfx[address] = data;
	prosport_gfx_dirty = 1;
	prosport_char_dirty[((offset & 0x7ff) + 0x800) >> 3] = 1;
	prosport_sprite_dirty[((offset & 0x7ff) + 0x800) >> 5] = 1;
}

WRITE8_HANDLER( liberate_videoram_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(fix_tilemap,offset&0x3ff);
}

/***************************************************************************/

VIDEO_START( prosoccr )
{
	background_tilemap = tilemap_create(get_back_tile_info,back_scan,TILEMAP_OPAQUE,16,16,32,32);
	fix_tilemap = tilemap_create(get_fix_tile_info,fix_scan,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!background_tilemap || !fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

VIDEO_START( prosport )
{
	background_tilemap = tilemap_create(prosport_get_back_tile_info,back_scan,TILEMAP_OPAQUE,16,16,32,32);
	fix_tilemap = tilemap_create(get_fix_tile_info,fix_scan,TILEMAP_TRANSPARENT,8,8,32,32);
	if (!background_tilemap || !fix_tilemap) return 1;
	tilemap_set_transparent_pen(fix_tilemap,0);
	memset(prosport_char_dirty,0,sizeof(prosport_char_dirty));
	memset(prosport_sprite_dirty,0,sizeof(prosport_sprite_dirty));
	prosport_gfx_dirty = 0;
	return 0;
}

VIDEO_START( boomrang )
{
	background_tilemap = tilemap_create(get_back_tile_info,back_scan,TILEMAP_SPLIT,16,16,32,32);
	fix_tilemap = tilemap_create(get_fix_tile_info,fix_scan,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!background_tilemap || !fix_tilemap)
		return 1;

	tilemap_set_transmask(background_tilemap,0,0x0001,0x007e); /* Bottom 1 pen/Top 7 pens */
	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

VIDEO_START( liberate )
{
	background_tilemap = tilemap_create(get_back_tile_info,back_scan,TILEMAP_OPAQUE,16,16,32,32);
	fix_tilemap = tilemap_create(get_fix_tile_info,fix_scan,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!background_tilemap || !fix_tilemap)
		return 1;

	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

/***************************************************************************/

WRITE8_HANDLER( prosport_paletteram_w )
{
	/* RGB output is inverted */
	paletteram_BBGGGRRR_w(offset,~data);
	paletteram[offset] = data;
}

PALETTE_INIT( liberate )
{
	int i,bit0,bit1,bit2,g,r,b;

	for (i = 0;i < 32;i++)
	{
		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
		palette_set_color(i,r,g,b);
	}
	palette_set_color(32,0,0,0); /* Allocate black for when no background is displayed */
}

/***************************************************************************/

static void liberate_drawsprites(mame_bitmap *bitmap)
{
	int offs;

	/* Sprites */
	for (offs = 0;offs < 0x800;offs += 4)
	{
		int multi,fx,fy,sx,sy,sy2,code,color;

		code = spriteram[offs+1] + ( ( spriteram[offs+0] & 0x60 ) << 3 );
		sx = (240 - spriteram[offs+3]);
	//if (sx < -7) sx += 256;

		sy = 240-spriteram[offs+2];
		color = 0;//(spriteram[offs+1] & 0x03);// + ((spriteram[offs+1] & 0x08) >> 1);

//      if (pri==0 && color!=0) continue;
//      if (pri==1 && color==0) continue;

		fx = spriteram[offs+0] & 0x04;
		fy = spriteram[offs+0] & 0x08; // or 0x02 ?
		multi = spriteram[offs+0] & 0x10;


		if (multi) sy-=16;

		if (flip_screen) {
			sy=240-sy;
			sx=240-sx;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			sy2=sy-16;
		}
		else sy2=sy+16;

    	drawgfx(bitmap,Machine->gfx[1],
        		code,
				color,
				fx,fy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
        if (multi)
    		drawgfx(bitmap,Machine->gfx[1],
				code+1,
				color,
				fx,fy,
				sx,sy2,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

static void prosport_drawsprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int offs, gfx = (deco16_io_ram[0] & 0x40) ? 7 : 4 + ((deco16_io_ram[0] & 0x30) >> 4);
	for (offs = 0; offs < 0x800; offs += 4)
	{
		int attr = spriteram[offs], code, x, y, fx, fy;
		if (!(attr & 1)) continue;
		code = spriteram[offs + 1] + ((attr & 3) << 8);
		x = 240 - spriteram[offs + 3];
		y = 240 - spriteram[offs + 2] - ((attr & 0x10) ? 16 : 0);
		fx = attr & 4;
		fy = attr & 2;
		if (flip_screen)
		{
			x = 240 - x;
			y = 240 - y;
			fx = !fx;
			fy = !fy;
		}
		drawgfx(bitmap,Machine->gfx[gfx],code,1,fx,fy,x,y,cliprect,TRANSPARENCY_PEN,0);
	}
}

static void boomrang_drawsprites(mame_bitmap *bitmap, int pri)
{
	int offs,multi,fx,fy,sx,sy,sy2,code,code2,color;

	for (offs = 0x000;offs < 0x800;offs += 4)
	{
		if ((spriteram[offs+0]&1)!=1) continue;
		if ((spriteram[offs+0]&0x8)!=pri) continue;

		code = spriteram[offs+1] + ((spriteram[offs+0]&0xe0)<<3);
		code2=code+1;

		multi = spriteram[offs+0] & 0x10;

		sy=spriteram[offs+2];
		if (multi) sy+=16;
		sx = (240 - spriteram[offs+3]);
//      sy = (240-spriteram[offs+2]);//-16;
		sy = 240-sy;

		color = (spriteram[offs+0]&0x4)>>2;

		fx = 0;
		fy = spriteram[offs+0] & 0x02;
		multi = spriteram[offs+0] & 0x10;

//      if (multi) sy-=16;
		if (fy && multi) { code2=code; code++; }

		if (flip_screen) {
			sy=240-sy;
			sx=240-sx;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			sy2=sy-16;
		}
		else {
			sy2=sy+16;
		}

    	drawgfx(bitmap,Machine->gfx[1],
        		code,
				color,
				fx,fy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
        if (multi)
    		drawgfx(bitmap,Machine->gfx[1],
				code2,
				color,
				fx,fy,
				sx,sy2,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************/

VIDEO_UPDATE( prosoccr )
{
	tilemap_set_scrolly(background_tilemap,0,deco16_io_ram[1]);
	tilemap_set_scrollx(background_tilemap,0,-deco16_io_ram[0]);

	if (background_disable)
		fillbitmap(bitmap,Machine->pens[32],&Machine->visible_area);
	else
		tilemap_draw(bitmap,cliprect,background_tilemap,0,0);
	boomrang_drawsprites(bitmap,0);
	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
}

VIDEO_UPDATE( prosport )
{
	int offs, gfx = (deco16_io_ram[0] & 0x40) ? 3 : (deco16_io_ram[0] & 0x30) >> 4;
	UINT8 *dynamic_gfx = memory_region(REGION_GFX6);

	if (prosport_gfx_dirty)
	{
		for (offs = 0; offs < 1024; offs++)
			if (prosport_char_dirty[offs])
			{
				decodegfx(Machine->gfx[3],dynamic_gfx,offs,1);
				prosport_char_dirty[offs] = 0;
			}
		for (offs = 0; offs < 256; offs++)
			if (prosport_sprite_dirty[offs])
			{
				decodegfx(Machine->gfx[7],dynamic_gfx,offs,1);
				prosport_sprite_dirty[offs] = 0;
			}

		prosport_gfx_dirty = 0;
	}

	tilemap_set_scrolly(background_tilemap,0,(deco16_io_ram[0] & 8) << 5);
	tilemap_set_scrollx(background_tilemap,0,-(((deco16_io_ram[0] & 2) << 7) | deco16_io_ram[1]));
	tilemap_draw(bitmap,cliprect,background_tilemap,0,0);
	for (offs = 0; offs < 0x400; offs++)
	{
		int tile = videoram[offs + 0x400] + ((videoram[offs] & 3) << 8);
		drawgfx(bitmap,Machine->gfx[gfx],tile,1,0,0,248 - 8 * (offs / 32),8 * (offs % 32),cliprect,TRANSPARENCY_PEN,0);
	}
	prosport_drawsprites(bitmap,cliprect);
}

VIDEO_UPDATE( boomrang )
{
	tilemap_set_scrolly(background_tilemap,0, deco16_io_ram[1]);
	tilemap_set_scrollx(background_tilemap,0,-deco16_io_ram[0]);

	if (background_disable)
		fillbitmap(bitmap,Machine->pens[32],&Machine->visible_area);
	else
		tilemap_draw(bitmap,cliprect,background_tilemap,TILEMAP_BACK,0);

	boomrang_drawsprites(bitmap,8);
	if (!background_disable)
		tilemap_draw(bitmap,cliprect,background_tilemap,TILEMAP_FRONT,0);
	boomrang_drawsprites(bitmap,0);
	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
}

VIDEO_UPDATE( liberate )
{
	tilemap_set_scrolly(background_tilemap,0, deco16_io_ram[1]);
	tilemap_set_scrollx(background_tilemap,0,-deco16_io_ram[0]);

	if (background_disable)
		fillbitmap(bitmap,Machine->pens[32],&Machine->visible_area);
	else
		tilemap_draw(bitmap,cliprect,background_tilemap,0,0);

	liberate_drawsprites(bitmap);
	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);
}
