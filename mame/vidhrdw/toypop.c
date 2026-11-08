// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/* Namco Universal System 16 video hardware. */

#include "driver.h"

UINT8 *toypop_master_workram;
UINT8 *toypop_fgvram, *toypop_fgattr;
UINT16 *toypop_bgvram;
UINT8 toypop_pal_bank;

PALETTE_INIT( toypop )
{
	int i;

	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		// red component
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		// green component
		bit0 = (color_prom[i+0x100] >> 0) & 0x01;
		bit1 = (color_prom[i+0x100] >> 1) & 0x01;
		bit2 = (color_prom[i+0x100] >> 2) & 0x01;
		bit3 = (color_prom[i+0x100] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		// blue component
		bit0 = (color_prom[i+0x200] >> 0) & 0x01;
		bit1 = (color_prom[i+0x200] >> 1) & 0x01;
		bit2 = (color_prom[i+0x200] >> 2) & 0x01;
		bit3 = (color_prom[i+0x200] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < 256;i++)
	{
		UINT8 entry;

		// characters
		colortable[i + 0*256] = (color_prom[i + 0x300] & 0x0f) | 0x70;
		colortable[i + 1*256] = (color_prom[i + 0x300] & 0x0f) | 0xf0;
		// sprites
		entry = color_prom[i + 0x500];
		colortable[i + 2*256] = entry;
	}
	for (i = 0;i < 16;i++)
	{
		// background
		colortable[i + 3*256 + 0*16] = 0x60 + i;
		colortable[i + 3*256 + 1*16] = 0xe0 + i;
	}
}

VIDEO_START( toypop )
{
	return 0;
}

static void toypop_bg_draw(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int x, y;
	UINT8 scanline[288];
	pen_t *pens = &Machine->pens[0x60 + (toypop_pal_bank << 7)];

	/* Two pixels occupy the low nibbles of each byte of a background word.
	 * Address the word and nibble separately so clipped odd endpoints work. */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			UINT16 data = toypop_bgvram[0x100 + y * 144 + x / 2];
			scanline[x - cliprect->min_x] = (data >> ((x & 1) ? 0 : 8)) & 0x0f;
		}
		draw_scanline8(bitmap,cliprect->min_x,y,cliprect->max_x - cliprect->min_x + 1,scanline,pens,-1);
	}
}

static void toypop_fg_draw(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int count;

	for (count=0;count<32*32;count++)
	{
		int x, y;
		UINT16 tile = toypop_fgvram[count];
		UINT8 color = (toypop_fgattr[count] & 0x3f) + (toypop_pal_bank<<6);

		if(count < 64)
		{
			x = 34 + (count / 32);
			y = (count % 32) - 2;
		}
		else if(count >= 32*30)
		{
			x = (count / 32) - 30;
			y = (count % 32) - 2;
		}
		else
		{
			x = 2 + (count % 32);
			y = (count / 32) - 2;
		}

		drawgfx(bitmap,Machine->gfx[0],tile,color,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
	}
}

// TODO: this is likely to be a lot more complex, and maybe is per scanline too
static void toypop_obj_draw(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int count;
	UINT8 *base_spriteram = toypop_master_workram;
	const UINT16 bank1 = 0x0800;
	const UINT16 bank2 = 0x1000;

	for (count=0x780;count<0x800;count+=2)
	{
		int enabled = (base_spriteram[count+bank2+1] & 2) == 0;

		UINT8 tile = base_spriteram[count];
		UINT8 color = base_spriteram[count+1];
		int x = base_spriteram[count+bank1+1] + (base_spriteram[count+bank2+1] << 8) - 71;
		int y = 224 - (base_spriteram[count+bank1] + 7);

		int fx = (base_spriteram[count+bank2] & 1) == 1;
		int fy = (base_spriteram[count+bank2] & 2) == 2;
		UINT8 width = ((base_spriteram[count+bank2] & 4) >> 2) + 1;
		UINT8 height = ((base_spriteram[count+bank2] & 8) >> 3) + 1;

		int xi, yi;

		if(!enabled)
			continue;

		if(height == 2)
			y -=16;

		for(yi=0;yi<height;yi++)
		{
			for(xi=0;xi<width;xi++)
			{
				UINT16 sprite_offs = tile + (xi ^ ((width - 1) & fx)) + yi * 2;
				drawgfx(bitmap,Machine->gfx[1],sprite_offs,color,fx,fy,x + xi*16,y + yi*16,cliprect,TRANSPARENCY_COLOR,0xff);
			}
		}
	}
}

VIDEO_UPDATE( toypop )
{
	toypop_bg_draw(bitmap,cliprect);
	toypop_fg_draw(bitmap,cliprect);
	toypop_obj_draw(bitmap,cliprect);
}
