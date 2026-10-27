// license:LGPL-2.1+
// copyright-holders:Tomasz Slanina
/***************************************************************************

    Jaleco fcombat

***************************************************************************/

#include "driver.h"

int fcombat_sh;
int fcombat_sv;


#define BACKGROUND_X_START		32
#define BACKGROUND_X_START_FLIP	72

#define VISIBLE_X_MIN			(12*8)
#define VISIBLE_X_MAX			(52*8)
#define VISIBLE_Y_MIN			(2*8)
#define VISIBLE_Y_MAX			(30*8)


static tilemap *bgmap;

static void get_bg_tile_info(int tile_index)
{
	int tileno,palno;//32*16 x 32

	//palno=(tile_index-(tile_index/32*16)*32*16)/32;

	tileno = memory_region(REGION_USER1)[tile_index];
	palno=((tileno>>2)&3)|((tileno>>4)&0x0c);
	SET_TILE_INFO(2,tileno,palno,0)
}

UINT8 fcombat_cocktail_flip;

static UINT8 char_palette, sprite_bank;
static UINT8 char_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( fcombat )
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0, bit1, bit2, r, g, b;

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

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* color_prom now points to the beginning of the char lookup table */

	for (i = 0; i < 0x200; i++)
		colortable[i] = 16 + (color_prom[(i & 0x1c0) | ((i & 3) << 4) | ((i >> 2) & 15)] & 15);

	for (i = 0x200; i < 0x300; i++)
		colortable[i] = color_prom[(i & 0x3c0) | ((i & 3) << 4) | ((i >> 2) & 15)] & 15;
}



/*************************************
 *
 *  Video system startup
 *
 *************************************/

VIDEO_START( fcombat )
{
	bgmap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 16, 16,32*8*2,32);
	return video_start_generic();
}



/*************************************
 *
 *  Video register I/O
 *
 *************************************/

WRITE8_HANDLER( fcombat_videoreg_w )
{
	/* bit 0 = flip screen and joystick input multiplexor */
	fcombat_cocktail_flip = data & 1;

	/* bits 1-2 char lookup table bank */
	char_palette = (data & 0x06) >> 1;

	/* bits 3 char bank */
	char_bank = (data & 0x08) >> 3;

	/* bits 4-5 unused */

	/* bits 6-7 sprite lookup table bank */
	sprite_bank = (data >> 6) & 3;
	//ui_popup("%08x",data);
}



VIDEO_UPDATE( fcombat )
{
	int sx, sy, offs, i, j;

	/* draw background */
	tilemap_set_scrolly( bgmap,0, fcombat_sh);
	tilemap_set_scrollx( bgmap,0, fcombat_cocktail_flip ? fcombat_sv+6 : fcombat_sv-8);

tilemap_mark_all_tiles_dirty(bgmap);
	tilemap_draw(bitmap,cliprect,bgmap,0,0);
	//draw_background(bitmap, cliprect);

	/* draw sprites */
	for (i = 0; i < spriteram_size; i += 4)
	{
		int flags = spriteram[i + 0];
		int y = 256 - spriteram[i + 1];
		int code = spriteram[i + 2]+((flags&0x20)<<3);
		int x = spriteram[i + 3] * 2 + (flags&1) + 52;

		int xflip = flags & 0x80;
		int yflip = flags & 0x40;

		int color = ((flags >> 1) & 0x03) | ((code >> 5) & 0x04) | (code & 0x08) | ((code >> 4) & 0x10);
		const gfx_element *gfx = Machine->gfx[1];
		if ((code & 0x108) == 0x108)
		{
			static const int mask[4] = { 0x308, 0x300, 0x08, 0x00 };
			code ^= mask[sprite_bank];
		}

		if (fcombat_cocktail_flip)
		{
			x = 64 * 8 - gfx->width - x + 2;
			y = 32 * 8 - gfx->height - y + 2;
			xflip = !xflip;
			yflip = !yflip;
		}

		switch ((flags >> 3) & 3)
		{
			// 16x16
			case 0:
				drawgfx(bitmap, gfx, code, color, xflip, yflip, x, y,
				        cliprect, TRANSPARENCY_PEN, 0);
				break;

			// 16x32
			case 1:
				if (fcombat_cocktail_flip)
					y -= 16;
				code &= ~0x10;

				drawgfx(bitmap, gfx, code | (yflip ? 16 : 0), color, xflip, yflip, x, y,
				        cliprect, TRANSPARENCY_PEN, 0);
				drawgfx(bitmap, gfx, code | (yflip ? 0 : 16), color, xflip, yflip, x, y + 16,
				        cliprect, TRANSPARENCY_PEN, 0);
				break;

			// 16x64
			case 2:
				if (fcombat_cocktail_flip)
					y -= 48;
				code &= ~0x30;

				for (j = 0; j < 4; j++)
				{
					int co = 16 * (j ^ (yflip ? 3 : 0));
					drawgfx(bitmap, gfx, code | co, color, xflip, yflip, x, y + j * 16,
					        cliprect, TRANSPARENCY_PEN, 0);
				}
				break;

			// unused
			case 3:
				break;
		}
	}

	/* draw the visible text layer */
	for (sy = VISIBLE_Y_MIN/8; sy < VISIBLE_Y_MAX/8; sy++)
		for (sx = VISIBLE_X_MIN/8; sx < VISIBLE_X_MAX/8; sx++)
		{
			int x = fcombat_cocktail_flip ? (63*8 - 8*sx) : 8*sx;
			int y = fcombat_cocktail_flip ? (31*8 - 8*sy) : 8*sy;

			offs = sx + sy * 64;
			drawgfx(bitmap, Machine->gfx[0],
				videoram[offs] + 256 * char_bank,
				((videoram[offs] & 0xf0) >> 4) + char_palette * 16,
				fcombat_cocktail_flip, fcombat_cocktail_flip, x, y,
				cliprect, TRANSPARENCY_PEN, 0);
		}
}
