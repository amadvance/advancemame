// license:BSD-3-Clause
// copyright-holders:Bryan McPhail
/*****************************************************************************

    Irem M90 system.  There is 1 video chip - NANAO GA-25, it produces
    2 tilemaps and sprites.  16 control bytes:

    0:  Playfield 1 X scroll
    2:  Playfield 1 Y scroll
    4:  Playfield 2 X scroll
    6:  Playfield 2 Y scroll
    8:  Bit 0x01 - unknown (set by hasamu)
    10: Playfield 1 control
        Bits0x03 - Playfield 1 VRAM base
        Bit 0x04 - Playfield 1 width (0 is 64 tiles, 0x4 is 128 tiles)
        Bit 0x10 - Playfield 1 disable
        Bit 0x20 - Playfield 1 rowscroll enable
    12: Playfield 2 control
        Bits0x03 - Playfield 2 VRAM base
        Bit 0x04 - Playfield 2 width (0 is 64 tiles, 0x4 is 128 tiles)
        Bit 0x10 - Playfield 2 disable
        Bit 0x20 - Playfield 2 rowscroll enable
    14: Bit 0x02 - unknown (set by hasamu)

    Emulation by Bryan McPhail, mish@tendril.co.uk, thanks to Chris Hardy!

*****************************************************************************/

#include "driver.h"

static unsigned char *m90_spriteram;
unsigned char *m90_video_data;
static tilemap *pf1_layer,*pf2_layer,*pf1_wide_layer,*pf2_wide_layer;
static INT32 m90_video_control_data[16];

static void get_tile_info(int tile_index,int layer,int page_mask)
{
	int tile,color;
	tile_index = 4*tile_index + ((m90_video_control_data[0xa+2*layer] & page_mask)*0x4000);

	tile=m90_video_data[tile_index]+(m90_video_data[tile_index+1]<<8);
	color=m90_video_data[tile_index+2];
	SET_TILE_INFO(
			0,
			tile,
			color&0xf,
			TILE_FLIPYX((color & 0xc0) >> 6))
	tile_info.priority = (color & 0x30) ? 1 : 0;
}


/* the bootleg boards use fixed VRAM bases: playfield 1 at 0x0000, playfield 2 at 0x8000 */
static void get_bootleg_tile_info(int tile_index,int base)
{
	int tile,color;
	tile_index = 4*tile_index + base;

	tile=m90_video_data[tile_index]+(m90_video_data[tile_index+1]<<8);
	color=m90_video_data[tile_index+2];
	SET_TILE_INFO(
			0,
			tile,
			color&0xf,
			TILE_FLIPYX((color & 0xc0) >> 6))
	tile_info.priority = (color & 0x30) ? 1 : 0;
}

static void get_pf1_tile_info (int tile_index) { get_tile_info(tile_index,0,3); }
static void get_pf1w_tile_info(int tile_index) { get_tile_info(tile_index,0,2); }
static void get_pf2_tile_info (int tile_index) { get_tile_info(tile_index,1,3); }
static void get_pf2w_tile_info(int tile_index) { get_tile_info(tile_index,1,2); }
static void get_pf1_bootleg_tile_info(int tile_index) { get_bootleg_tile_info(tile_index,0x0000); }
static void get_pf2_bootleg_tile_info(int tile_index) { get_bootleg_tile_info(tile_index,0x8000); }


VIDEO_START( m90 )
{
	pf1_layer =      tilemap_create(get_pf1_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
	pf1_wide_layer = tilemap_create(get_pf1w_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
	pf2_layer =      tilemap_create(get_pf2_tile_info, tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);
	pf2_wide_layer = tilemap_create(get_pf2w_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,128,64);

	if (!pf1_layer || !pf1_wide_layer || !pf2_layer || !pf2_wide_layer)
		return 1;

	tilemap_set_transparent_pen(pf1_layer,0);
	tilemap_set_transparent_pen(pf1_wide_layer,0);

	state_save_register_global_array(m90_video_control_data);

	return 0;
}

static int bootleg_video_start(int pf1_type)
{
	pf1_layer =      tilemap_create(get_pf1_bootleg_tile_info,tilemap_scan_rows,pf1_type,8,8,64,64);
	pf1_wide_layer = tilemap_create(get_pf1_bootleg_tile_info,tilemap_scan_rows,pf1_type,8,8,128,64);
	pf2_layer =      tilemap_create(get_pf2_bootleg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
	pf2_wide_layer = tilemap_create(get_pf2_bootleg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);

	if (!pf1_layer || !pf1_wide_layer || !pf2_layer || !pf2_wide_layer)
		return 1;

	tilemap_set_transparent_pen(pf1_layer,0);
	tilemap_set_transparent_pen(pf1_wide_layer,0);
	tilemap_set_transparent_pen(pf2_layer,0);
	tilemap_set_transparent_pen(pf2_wide_layer,0);

	state_save_register_global_array(m90_video_control_data);

	return 0;
}

VIDEO_START( bomblord )
{
	return bootleg_video_start(TILEMAP_TRANSPARENT);
}

VIDEO_START( dynablsb )
{
	return bootleg_video_start(TILEMAP_OPAQUE);
}

static void m90_drawsprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;

	for (offs = 0x1f2;offs >= 0;offs -= 6)
	{
		int x,y,sprite,colour,fx,fy,y_multi,i;

		sprite = (m90_spriteram[offs+2] | (m90_spriteram[offs+3]<<8));
		colour = (m90_spriteram[offs+1] >> 1) & 0x0f;

		y = m90_spriteram[offs+0] | ((m90_spriteram[offs+1] & 0x01) << 8);
		x = m90_spriteram[offs+4] | ((m90_spriteram[offs+5] & 0x01) << 8);

		x = x - 16;
		y = 512 - y;

		fx = m90_spriteram[offs+5] & 0x02;
		fy = m90_spriteram[offs+1] & 0x80;

		y_multi = 1 << ((m90_spriteram[offs+1] & 0x60) >> 5);
		y -= 16 * y_multi;

		for (i = 0;i < y_multi;i++)
			if (m90_video_control_data[0xe] & 0x01)
			    pdrawgfx(bitmap,Machine->gfx[1],
					sprite + (fy ? y_multi-1 - i : i),
					colour,
					fx,fy,
					x,y+i*16,
					cliprect,TRANSPARENCY_PEN,0,
					(colour & 0x08) ? 0x00 : 0x02);
			else if (m90_video_control_data[0xe] & 0x02)
				pdrawgfx(bitmap,Machine->gfx[1],
					sprite + (fy ? y_multi-1 - i : i),
					colour,
					fx,fy,
					x,y+i*16,
					cliprect,TRANSPARENCY_PEN,0,
					((colour & 0x0c)==0x0c) ? 0x00 : 0x02);
			else
				pdrawgfx(bitmap,Machine->gfx[1],
					sprite + (fy ? y_multi-1 - i : i),
					colour,
					fx,fy,
					x,y+i*16,
					cliprect,TRANSPARENCY_PEN,0,
					0x02);
	}
}

#define BOOTLEG_SPRITE(offs) (spriteram[2*(offs)] | (spriteram[2*(offs)+1] << 8))

/* the sprite list starts at word 7 and is terminated by end_mark */
static void bootleg_drawsprites(mame_bitmap *bitmap,const rectangle *cliprect,int end_mark,int x_offset,int y_offset)
{
	int offs = 7, last_sprite = 0;
	int x,y,sprite,colour,fx,fy;

	while ((offs < spriteram_size/2 - 3) && (BOOTLEG_SPRITE(offs+0) != end_mark))
	{
		last_sprite = offs;
		offs += 4;
	}

	for (offs = last_sprite; offs >= 7; offs -= 4)
	{
		sprite = BOOTLEG_SPRITE(offs+1);
		colour = (BOOTLEG_SPRITE(offs+2) >> 9) & 0x0f;

		y = (BOOTLEG_SPRITE(offs+0) & 0x1ff) + y_offset;
		x = (BOOTLEG_SPRITE(offs+3) & 0x1ff) + x_offset;

		x = x - 16;
		y = 512 - y;

		if (y < 0) y += 512;

		fx = (BOOTLEG_SPRITE(offs+3) >> 8) & 0x02;
		fy = (BOOTLEG_SPRITE(offs+2) >> 8) & 0x80;

		pdrawgfx(bitmap,Machine->gfx[1],
				sprite,
				colour,
				fx,fy,
				x,y,
				cliprect,TRANSPARENCY_PEN,0,
				(colour & 0x08) ? 0x00 : 0x02);
	}
}

WRITE8_HANDLER( m90_video_control_w )
{
	m90_video_control_data[offset]=data;
}


static void markdirty(tilemap *tilemap,int page,offs_t offset)
{
	offset -= page * 0x4000;

	if (offset >= 0 && offset < 0x4000)
		tilemap_mark_tile_dirty(tilemap,offset/4);
}


WRITE8_HANDLER( m90_video_w )
{
	m90_video_data[offset] = data;

	markdirty(pf1_layer,     m90_video_control_data[0xa] & 0x3,offset);
	markdirty(pf1_wide_layer,m90_video_control_data[0xa] & 0x2,offset);
	markdirty(pf2_layer,     m90_video_control_data[0xc] & 0x3,offset);
	markdirty(pf2_wide_layer,m90_video_control_data[0xc] & 0x2,offset);
}

WRITE8_HANDLER( m90_bootleg_video_w )
{
	int layer = offset >> 15;

	m90_video_data[offset] = data;

	/* the 64 tiles wide layer uses only the first half of the layer VRAM */
	if (!(offset & 0x4000))
		tilemap_mark_tile_dirty(layer ? pf2_layer : pf1_layer,(offset & 0x3fff) >> 2);
	tilemap_mark_tile_dirty(layer ? pf2_wide_layer : pf1_wide_layer,(offset & 0x7fff) >> 2);
}

#define BOOTLEG_VIDEO(offs) (m90_video_data[offs] | (m90_video_data[(offs)+1] << 8))

static void bootleg_draw_layer(mame_bitmap *bitmap,const rectangle *cliprect,tilemap *tmap)
{
	tilemap_draw(bitmap,cliprect,tmap,0,0);
	tilemap_draw(bitmap,cliprect,tmap,1,1);
}

VIDEO_UPDATE( bomblord )
{
	int i;

	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,get_black_pen(),cliprect);

	/* Setup scrolling */
	if (m90_video_control_data[0xc]&0x20)
	{
		tilemap_set_scroll_rows(pf1_layer,512);
		tilemap_set_scroll_rows(pf1_wide_layer,512);
		for (i=0; i<512; i++)
		{
			tilemap_set_scrollx(pf1_layer,i, BOOTLEG_VIDEO(0xf400+2*i)-12);
			tilemap_set_scrollx(pf1_wide_layer,i, BOOTLEG_VIDEO(0xf400+2*i)-12+256);
		}
	}
	else
	{
		tilemap_set_scroll_rows(pf1_layer,1);
		tilemap_set_scroll_rows(pf1_wide_layer,1);
		tilemap_set_scrollx(pf1_layer,0, BOOTLEG_VIDEO(0xf004)-12);
		tilemap_set_scrollx(pf1_wide_layer,0, BOOTLEG_VIDEO(0xf004)-12);
	}

	if (m90_video_control_data[0xc] & 0x02)
	{
		tilemap_set_scrollx(pf2_wide_layer,0, BOOTLEG_VIDEO(0xf000)-16);
		tilemap_set_scrolly(pf2_wide_layer,0, BOOTLEG_VIDEO(0xf008)+388);
		bootleg_draw_layer(bitmap,cliprect,pf2_wide_layer);
	}
	else
	{
		tilemap_set_scrollx(pf2_layer,0, BOOTLEG_VIDEO(0xf000)-16);
		tilemap_set_scrolly(pf2_layer,0, BOOTLEG_VIDEO(0xf008)-120);
		bootleg_draw_layer(bitmap,cliprect,pf2_layer);
	}

	if (m90_video_control_data[0xc] & 0x04)
	{
		tilemap_set_scrolly(pf1_wide_layer,0, BOOTLEG_VIDEO(0xf00c)+392);
		bootleg_draw_layer(bitmap,cliprect,pf1_wide_layer);
	}
	else
	{
		tilemap_set_scrolly(pf1_layer,0, BOOTLEG_VIDEO(0xf00c)-116);
		bootleg_draw_layer(bitmap,cliprect,pf1_layer);
	}

	bootleg_drawsprites(bitmap,cliprect,0x8000,16,152);
}

VIDEO_UPDATE( dynablsb )
{
	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,get_black_pen(),cliprect);

	if (!(BOOTLEG_VIDEO(0xf008) & 0x4000))
	{
		tilemap_set_scrollx(pf1_wide_layer,0, BOOTLEG_VIDEO(0xf004)+64);
		tilemap_set_scrolly(pf1_wide_layer,0, BOOTLEG_VIDEO(0xf006)+512);
		bootleg_draw_layer(bitmap,cliprect,pf1_wide_layer);
	}
	else
	{
		tilemap_set_scrollx(pf1_layer,0, BOOTLEG_VIDEO(0xf004)+64);
		tilemap_set_scrolly(pf1_layer,0, BOOTLEG_VIDEO(0xf006)+4);
		bootleg_draw_layer(bitmap,cliprect,pf1_layer);
	}

	if (!(BOOTLEG_VIDEO(0xf008) & 0x8000))
	{
		tilemap_set_scrollx(pf2_wide_layer,0, BOOTLEG_VIDEO(0xf000)+68);
		tilemap_set_scrolly(pf2_wide_layer,0, BOOTLEG_VIDEO(0xf002)+512);
		bootleg_draw_layer(bitmap,cliprect,pf2_wide_layer);
	}
	else
	{
		tilemap_set_scrollx(pf2_layer,0, BOOTLEG_VIDEO(0xf000)+68);
		tilemap_set_scrolly(pf2_layer,0, BOOTLEG_VIDEO(0xf002)+4);
		bootleg_draw_layer(bitmap,cliprect,pf2_layer);
	}

	bootleg_drawsprites(bitmap,cliprect,0xffff,-64,288);
}

VIDEO_UPDATE( m90 )
{
	static int last_pf1,last_pf2;
	int pf1_base = m90_video_control_data[0xa] & 0x3;
	int pf2_base = m90_video_control_data[0xc] & 0x3;
	int i,pf1_enable,pf2_enable, video_enable;

    if (m90_video_control_data[0xe]&0x04) video_enable=0; else video_enable=1;
	if (m90_video_control_data[0xa]&0x10) pf1_enable=0; else pf1_enable=1;
	if (m90_video_control_data[0xc]&0x10) pf2_enable=0; else pf2_enable=1;
//  tilemap_set_enable(pf1_layer,pf1_enable);
//  tilemap_set_enable(pf2_layer,pf2_enable);
//  tilemap_set_enable(pf1_wide_layer,pf1_enable);
//  tilemap_set_enable(pf2_wide_layer,pf2_enable);

	/* Dirty tilemaps if VRAM base changes */
	if (pf1_base!=last_pf1)
	{
		tilemap_mark_all_tiles_dirty(pf1_layer);
		tilemap_mark_all_tiles_dirty(pf1_wide_layer);
	}
	if (pf2_base!=last_pf2)
	{
		tilemap_mark_all_tiles_dirty(pf2_layer);
		tilemap_mark_all_tiles_dirty(pf2_wide_layer);
	}
	last_pf1=pf1_base;
	last_pf2=pf2_base;

	m90_spriteram=m90_video_data+0xee00;

	/* Setup scrolling */
	if (m90_video_control_data[0xa]&0x20)
	{
		tilemap_set_scroll_rows(pf1_layer,512);
		tilemap_set_scroll_rows(pf1_wide_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_layer,i/2, (m90_video_data[0xf000+i]+(m90_video_data[0xf001+i]<<8))+2);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf1_wide_layer,i/2, (m90_video_data[0xf000+i]+(m90_video_data[0xf001+i]<<8))+256+2);
	}
	else
	{
		tilemap_set_scroll_rows(pf1_layer,1);
		tilemap_set_scroll_rows(pf1_wide_layer,1);
		tilemap_set_scrollx( pf1_layer,0, (m90_video_control_data[3]<<8)+m90_video_control_data[2]+2);
		tilemap_set_scrollx( pf1_wide_layer,0, (m90_video_control_data[3]<<8)+m90_video_control_data[2]+256+2);
	}

	/* Setup scrolling */
	if (m90_video_control_data[0xc]&0x20) {
		tilemap_set_scroll_rows(pf2_layer,512);
		tilemap_set_scroll_rows(pf2_wide_layer,512);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf2_layer,i/2, (m90_video_data[0xf400+i]+(m90_video_data[0xf401+i]<<8))-2);
		for (i=0; i<1024; i+=2)
			tilemap_set_scrollx( pf2_wide_layer,i/2, (m90_video_data[0xf400+i]+(m90_video_data[0xf401+i]<<8))+256-2);
	} else {
		tilemap_set_scroll_rows(pf2_layer,1);
		tilemap_set_scroll_rows(pf2_wide_layer,1);
		tilemap_set_scrollx( pf2_layer,0, (m90_video_control_data[7]<<8)+m90_video_control_data[6]-2);
		tilemap_set_scrollx( pf2_wide_layer,0, (m90_video_control_data[7]<<8)+m90_video_control_data[6]+256-2 );
	}

	fillbitmap(priority_bitmap,0,cliprect);

	if (video_enable) {
		if (!pf2_enable)
			fillbitmap(bitmap,Machine->pens[0],cliprect);


		if (pf2_enable)
		{
			// use the playfield 2 y-offset table for each scanline
			if (m90_video_control_data[0xc] & 0x40) {

				int line;
				rectangle clip;
				clip.min_x = cliprect->min_x;
				clip.max_x = cliprect->max_x;

				for(line = 0; line < 1024; line+=2)
				{
					clip.min_y = clip.max_y = line / 2;

					if (m90_video_control_data[0xc] & 0x4) {
						tilemap_set_scrolly(pf2_wide_layer, 0,(m90_video_control_data[5]<<8)+m90_video_control_data[4] + (m90_video_data[0xfc00+line]+(m90_video_data[0xfc01+line]<<8))+128);
                        tilemap_draw(bitmap,&clip,pf2_wide_layer,0,0);
						tilemap_draw(bitmap,&clip,pf2_wide_layer,1,1);
					} else {
						tilemap_set_scrolly(pf2_layer, 0,(m90_video_control_data[5]<<8)+m90_video_control_data[4] + (m90_video_data[0xfc00+line]+(m90_video_data[0xfc01+line]<<8))+128);
						tilemap_draw(bitmap,&clip,pf2_layer,0,0);
						tilemap_draw(bitmap,&clip,pf2_layer,1,1);
					}
				}
			}
			else
			{
				if (m90_video_control_data[0xc] & 0x4) {
					tilemap_set_scrolly( pf2_wide_layer,0,(m90_video_control_data[5]<<8)+m90_video_control_data[4] );
					tilemap_draw(bitmap,cliprect,pf2_wide_layer,0,0);
					tilemap_draw(bitmap,cliprect,pf2_wide_layer,1,1);
				} else {
					tilemap_set_scrolly( pf2_layer,0,(m90_video_control_data[5]<<8)+m90_video_control_data[4] );
					tilemap_draw(bitmap,cliprect,pf2_layer,0,0);
					tilemap_draw(bitmap,cliprect,pf2_layer,1,1);
				}
			}
		}

		if (pf1_enable)
		{
			// use the playfield 1 y-offset table for each scanline
			if (m90_video_control_data[0xa] & 0x40) {

				int line;
				rectangle clip;
				clip.min_x = cliprect->min_x;
				clip.max_x = cliprect->max_x;

				for(line = 0; line < 1024; line+=2)
				{
					clip.min_y = clip.max_y = line / 2;

					if (m90_video_control_data[0xa] & 0x4) {
                        tilemap_set_scrolly(pf1_wide_layer, 0,(m90_video_control_data[1]<<8)+m90_video_control_data[0] + (m90_video_data[0xf800+line]+(m90_video_data[0xf801+line]<<8))+128);
						tilemap_draw(bitmap,&clip,pf1_wide_layer,0,0);
						tilemap_draw(bitmap,&clip,pf1_wide_layer,1,1);
					} else {
                        tilemap_set_scrolly(pf1_layer, 0,(m90_video_control_data[1]<<8)+m90_video_control_data[0] + (m90_video_data[0xf800+line]+(m90_video_data[0xf801+line]<<8))+128);
						tilemap_draw(bitmap,&clip,pf1_layer,0,0);
						tilemap_draw(bitmap,&clip,pf1_layer,1,1);
					}
				}
			}
			else
			{
				if (m90_video_control_data[0xa] & 0x4) {
					tilemap_set_scrolly( pf1_wide_layer,0,(m90_video_control_data[1]<<8)+m90_video_control_data[0] );
					tilemap_draw(bitmap,cliprect,pf1_wide_layer,0,0);
					tilemap_draw(bitmap,cliprect,pf1_wide_layer,1,1);
				} else {
					tilemap_set_scrolly( pf1_layer,0,(m90_video_control_data[1]<<8)+m90_video_control_data[0] ); 
					tilemap_draw(bitmap,cliprect,pf1_layer,0,0);
					tilemap_draw(bitmap,cliprect,pf1_layer,1,1);
				}
			}
		}

	    m90_drawsprites(bitmap,cliprect);

	} else {
		fillbitmap(bitmap,get_black_pen(),cliprect);
	}

}
