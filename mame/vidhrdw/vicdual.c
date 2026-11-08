// license:BSD-3-Clause
// copyright-holders:Zsolt Vasvari
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "includes/vicdual.h"



unsigned char *vicdual_characterram;
static unsigned char dirtycharacter[256];

static int palette_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  The VIC dual game board has one 32x8 palette PROM. The color code is taken
  from the three most significant bits of the character code, plus two
  additional palette bank bits.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 22 ohm resistor  -- RED   \
        -- 22 ohm resistor  -- BLUE  |  foreground
        -- 22 ohm resistor  -- GREEN /
        -- Unused
        -- 22 ohm resistor  -- RED   \
        -- 22 ohm resistor  -- BLUE  |  background
        -- 22 ohm resistor  -- GREEN /
  bit 0 -- Unused

***************************************************************************/
PALETTE_INIT( vicdual )
{
	int i;
	/* for b&w games we'll use the Head On PROM */
	static unsigned char bw_color_prom[] =
	{
		/* for b/w games, let's use the Head On PROM */
		0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
		0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	};


	if (color_prom == 0) color_prom = bw_color_prom;

	for (i = 0;i < 32;i++)
	{
		int bit,r,g,b;


		/* background red component */
		bit = (color_prom[i] >> 3) & 0x01;
		r = 0xff * bit;
		/* background green component */
		bit = (color_prom[i] >> 1) & 0x01;
		g = 0xff * bit;
		/* background blue component */
		bit = (color_prom[i] >> 2) & 0x01;
		b = 0xff * bit;

		palette_set_color(2*i,r,g,b);

		/* foreground red component */
		bit = (color_prom[i] >> 7) & 0x01;
		r = 0xff * bit;
		/* foreground green component */
		bit = (color_prom[i] >> 5) & 0x01;
		g = 0xff * bit;
		/* foreground blue component */
		bit = (color_prom[i] >> 6) & 0x01;
		b = 0xff * bit;

		palette_set_color(2*i+1,r,g,b);
	}

	palette_bank = 0;

	{
		/* Heiankyo Alien doesn't write to port 0x40, it expects it to default to 3 */
		if (strcmp(Machine->gamedrv->name, "heiankyo") == 0 ||
			strcmp(Machine->gamedrv->name, "headon2") == 0)
			palette_bank = 3;

		/* and many others expect it to default to 1 */
		if (strcmp(Machine->gamedrv->name, "invinco") == 0 ||
				strcmp(Machine->gamedrv->name, "digger") == 0 ||
				strcmp(Machine->gamedrv->name, "tranqgun") == 0)
			palette_bank = 1;
	}
}



WRITE8_HANDLER( vicdual_characterram_w )
{
	if (vicdual_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		vicdual_characterram[offset] = data;
	}
}

READ8_HANDLER( vicdual_characterram_r )
{
	return vicdual_characterram[offset];
}

WRITE8_HANDLER( vicdual_palette_bank_w )
{
	if (palette_bank != (data & 3))
	{
		palette_bank = data & 3;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( vicdual )
{
	int offs;


	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer,1,videoram_size);
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || dirtycharacter[charcode])
		{
			int sx,sy;


			/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,vicdual_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,
					(charcode >> 5) + 8 * palette_bank,
					0,0,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);

		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter[offs] == 2) dirtycharacter[offs] = 0;
	}
}

static UINT8 nsub_background_bank;

PALETTE_INIT( nsub )
{
	int i;
	palette_init_vicdual(colortable, color_prom);
	nsub_background_bank = 0;
	for (i = 0; i < 256; i++)
	{
		int g = 0, b = 0;
		if (i >= 24 && i < 105)
			b = (i - 24) * 3;
		else if (i >= 105 && i < 233)
		{
			g = 0x80 + i - 105;
			b = 255;
		}
		palette_set_color(64 + i, 0, g, b);
	}
}

WRITE8_HANDLER( nsub_palette_bank_w )
{
	nsub_background_bank = data & 0x0c;
	vicdual_palette_bank_w(0, data);
}

VIDEO_UPDATE( nsub )
{
	int x, y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int code = videoram[(y / 8) * 32 + x / 8];
			int pixels = vicdual_characterram[code * 8 + (y & 7)];
			int color = (code >> 5) + 8 * palette_bank;
			int pen;
			if (pixels & (0x80 >> (x & 7)))
				pen = color * 2 + 1;
			else
			{
				int index = (nsub_background_bank & 8) ? 255 - x : x;
				pen = color * 2;
				if ((nsub_background_bank & 4) && index >= 24 && index < 233)
					pen = 64 + index;
			}
			plot_pixel(bitmap, x, y, Machine->pens[pen]);
		}
	}
}
