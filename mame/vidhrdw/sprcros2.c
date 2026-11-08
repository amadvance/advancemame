// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/* Super Cross II video hardware. */

#include "driver.h"

UINT8 *sprcros2_fgvram, *sprcros2_fgattr;
UINT8 *sprcros2_bgvram, *sprcros2_bgattr, *sprcros2_sprram;
UINT8 sprcros2_bg_scrollx, sprcros2_bg_scrolly;
UINT8 sprcros2_screen_enable;

PALETTE_INIT( sprcros2 )
{
	int i;

	/* create a lookup table for the palette */
	for (i = 0; i < 0x20; i++)
	{
		int bit0, bit1, bit2;
		int r, g, b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = 0x47 * bit0 + 0xb8 * bit1;
		palette_set_color(i,r,g,b);

	}

	/* color_prom now points to the beginning of the lookup table */
	color_prom += 0x20;

	/* bg */
	for (i = 0; i < 0x100; i++)
	{
		UINT8 ctabentry = (color_prom[i] & 0x0f) | ((color_prom[i + 0x100] & 0x0f) << 4);
		colortable[i] = ctabentry;
	}

	/* sprites & fg */
	for (i = 0x100; i < 0x300; i++)
	{
		UINT8 ctabentry = color_prom[i + 0x100];
		colortable[i] = ctabentry;
	}
}

VIDEO_START( sprcros2 )
{
	return 0;
}

static void sprcros2_bg_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int count = 0;
	int x, y;

	for (y=0;y<32;y++)
	{
		for (x=0;x<32;x++)
		{
			UINT16 tile = sprcros2_bgvram[count];
			int flipx = sprcros2_bgattr[count] & 0x08;
			UINT8 color = (sprcros2_bgattr[count] & 0xf0) >> 4;
			tile |= (sprcros2_bgattr[count] & 7) << 8;

			drawgfx(bitmap,Machine->gfx[0],tile,color,flipx,0,x*8-sprcros2_bg_scrollx,y*8-sprcros2_bg_scrolly,cliprect,TRANSPARENCY_NONE,0);
			drawgfx(bitmap,Machine->gfx[0],tile,color,flipx,0,x*8+256-sprcros2_bg_scrollx,y*8-sprcros2_bg_scrolly,cliprect,TRANSPARENCY_NONE,0);
			drawgfx(bitmap,Machine->gfx[0],tile,color,flipx,0,x*8-sprcros2_bg_scrollx,y*8+256-sprcros2_bg_scrolly,cliprect,TRANSPARENCY_NONE,0);
			drawgfx(bitmap,Machine->gfx[0],tile,color,flipx,0,x*8+256-sprcros2_bg_scrollx,y*8+256-sprcros2_bg_scrolly,cliprect,TRANSPARENCY_NONE,0);

			count++;
		}
	}

}

static void sprcros2_obj_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{

	int count;

	for(count=0x40-4;count>-1;count-=4)
	{
		UINT8 x,y,tile,color;
		int flipx;

		y = 224-sprcros2_sprram[count+2];
		x = sprcros2_sprram[count+3];
		tile = sprcros2_sprram[count+0];
		flipx = (sprcros2_sprram[count+1] & 2);
		color = (sprcros2_sprram[count+1] & 0x38) >> 3;
		drawgfx(bitmap,Machine->gfx[1],tile,color,flipx,0,x,y & 0xff,cliprect,TRANSPARENCY_PEN,0);
	}
}

static void sprcros2_fg_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int count = 0;
	int x, y;

	for (y=0;y<32;y++)
	{
		for (x=0;x<32;x++)
		{
			UINT16 tile = sprcros2_fgvram[count];
			UINT8 color = (sprcros2_fgattr[count] & 0xfc) >> 2;
			tile |= (sprcros2_fgattr[count] & 3) << 8;

			/* Only the upper foreground color groups use transparent pen zero. */
			if((color & 0x30) != 0x30)
				drawgfx(bitmap,Machine->gfx[2],tile,color,0,0,x*8,y*8,cliprect,TRANSPARENCY_NONE,0);
			else
				drawgfx(bitmap,Machine->gfx[2],tile,color,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);

			count++;
		}
	}
}




VIDEO_UPDATE( sprcros2 )
{
	if(!sprcros2_screen_enable)
	{
		fillbitmap(bitmap,Machine->pens[0],cliprect);
		return;
	}
	sprcros2_bg_draw(bitmap,cliprect);
	sprcros2_obj_draw(bitmap,cliprect);
	sprcros2_fg_draw(bitmap,cliprect);
}
