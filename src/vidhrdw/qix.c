// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Zsolt Vasvari
// thanks-to:John Butler, Ed Mueller
/***************************************************************************

    Taito Qix hardware

***************************************************************************/

#include "driver.h"
#include "qix.h"


/* Globals */
UINT8 *qix_videoaddress;
UINT8 qix_cocktail_flip;


/* Local variables */
static UINT8 vram_mask;
static UINT8 qix_palettebank;
static int leds;


/*************************************
 *
 *  Video startup
 *
 *************************************/

VIDEO_START( qix )
{
	/* allocate memory for the full video RAM */
	videoram = auto_malloc(256 * 256);

	/* initialize the mask for games that don't use it */
	vram_mask = 0xff;

	/* set up save states */
	state_save_register_global_pointer(videoram, 256 * 256);
	state_save_register_global(qix_cocktail_flip);
	state_save_register_global(qix_palettebank);
	state_save_register_global(leds);

	return 0;
}



/*************************************
 *
 *  Beam position tracking & screen update
 *
 *************************************/

static void qix_screen_update_now(void)
{
	int vpos = cpu_getscanline();
	int width = Machine->drv->screen_width;
	int hpos = cpu_gethorzbeampos();
	if (hpos < (width / 2) && vpos > 0)
		vpos--;
	force_partial_update(vpos);
}



/*************************************
 *
 *  Current scanline read
 *
 *************************************/

READ8_HANDLER( qix_scanline_r )
{
	int scanline = cpu_getscanline();
	return (scanline <= 0xff) ? scanline : 0;
}



/*************************************
 *
 *  Cocktail flip
 *
 *************************************/

WRITE8_HANDLER( qix_flip_screen_w )
{
	qix_cocktail_flip = data;
}



/*************************************
 *
 *  Video RAM mask
 *
 *************************************/

WRITE8_HANDLER( slither_vram_mask_w )
{
	/* Slither appears to extend the basic hardware by providing */
	/* a mask register which controls which data bits get written */
	/* to video RAM */
	vram_mask = data;
}



/*************************************
 *
 *  Direct video RAM read/write
 *
 *  The screen is 256x256 with eight
 *  bit pixels (64K).  The screen is
 *  divided into two halves each half
 *  mapped by the video CPU at
 *  $0000-$7FFF.  The high order bit
 *  of the address latch at $9402
 *  specifies which half of the screen
 *  is being accessed.
 *
 *************************************/

READ8_HANDLER( qix_videoram_r )
{
	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;
	return videoram[offset];
}


WRITE8_HANDLER( qix_videoram_w )
{
	/* update the screen in case the game is writing "behind" the beam -
       Zookeeper likes to do this */
	qix_screen_update_now();

	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;

	/* write the data */
	videoram[offset] = data;
}


WRITE8_HANDLER( slither_videoram_w )
{
	/* update the screen in case the game is writing "behind" the beam -
       Zookeeper likes to do this */
	qix_screen_update_now();

	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;

	/* blend the data */
	videoram[offset] = (videoram[offset] & ~vram_mask) | (data & vram_mask);
}



/*************************************
 *
 *  Latched video RAM read/write
 *
 *  The address latch works as follows.
 *  When the video CPU accesses $9400,
 *  the screen address is computed by
 *  using the values at $9402 (high
 *  byte) and $9403 (low byte) to get
 *  a value between $0000-$FFFF.  The
 *  value at that location is either
 *  returned or written.
 *
 *************************************/

READ8_HANDLER( qix_addresslatch_r )
{
	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];
	return videoram[offset];
}


WRITE8_HANDLER( qix_addresslatch_w )
{
	/* update the screen in case the game is writing "behind" the beam */
	qix_screen_update_now();

	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];

	/* write the data */
	videoram[offset] = data;
}


WRITE8_HANDLER( slither_addresslatch_w )
{
	/* update the screen in case the game is writing "behind" the beam */
	qix_screen_update_now();

	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];

	/* blend the data */
	videoram[offset] = (videoram[offset] & ~vram_mask) | (data & vram_mask);
}



/*************************************
 *
 *  Palette RAM
 *
 *************************************/

WRITE8_HANDLER( qix_paletteram_w )
{
	/* this conversion table should be about right. It gives a reasonable */
	/* gray scale in the test screen, and the red, green and blue squares */
	/* in the same screen are barely visible, as the manual requires. */
	static const UINT8 table[16] =
	{
		0x00,	/* value = 0, intensity = 0 */
		0x12,	/* value = 0, intensity = 1 */
		0x24,	/* value = 0, intensity = 2 */
		0x49,	/* value = 0, intensity = 3 */
		0x12,	/* value = 1, intensity = 0 */
		0x24,	/* value = 1, intensity = 1 */
		0x49,	/* value = 1, intensity = 2 */
		0x92,	/* value = 1, intensity = 3 */
		0x5b,	/* value = 2, intensity = 0 */
		0x6d,	/* value = 2, intensity = 1 */
		0x92,	/* value = 2, intensity = 2 */
		0xdb,	/* value = 2, intensity = 3 */
		0x7f,	/* value = 3, intensity = 0 */
		0x91,	/* value = 3, intensity = 1 */
		0xb6,	/* value = 3, intensity = 2 */
		0xff	/* value = 3, intensity = 3 */
	};
	int bits, intensity, red, green, blue;
	UINT8 old_data = paletteram[offset];

	/* set the palette RAM value */
	paletteram[offset] = data;

	/* trigger an update if a currently visible pen has changed */
	if (((offset >> 8) == qix_palettebank) && (old_data != data))
		qix_screen_update_now();

	/* compute R, G, B from the table */
	intensity = (data >> 0) & 0x03;
	bits = (data >> 6) & 0x03;
	red = table[(bits << 2) | intensity];
	bits = (data >> 4) & 0x03;
	green = table[(bits << 2) | intensity];
	bits = (data >> 2) & 0x03;
	blue = table[(bits << 2) | intensity];

	/* update the palette */
	palette_set_color(offset, red, green, blue);
}


WRITE8_HANDLER( qix_palettebank_w )
{
	/* set the bank value */
	if (qix_palettebank != (data & 3))
	{
		qix_screen_update_now();
		qix_palettebank = data & 3;
	}

	/* LEDs are in the upper 6 bits */
	leds = ~data & 0xfc;
}



/*************************************
 *
 *  Core video refresh
 *
 *************************************/

VIDEO_UPDATE( qix )
{
	pen_t *pens = &Machine->pens[qix_palettebank * 256];
	UINT8 *vram;
	UINT8 scanline[256];
	int x,y;

	/* draw the bitmap */
	if (qix_cocktail_flip)
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			vram = &videoram[(y ^ 0xff) * 256];
			for (x = 0;x < 256;x++)
				scanline[x] = vram[x ^ 0xff];
			draw_scanline8(bitmap, 0, y, 256, scanline, pens, -1);
		}
	}
	else
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			vram = &videoram[y * 256];
			for (x = 0;x < 256;x++)
				scanline[x] = vram[x];
			draw_scanline8(bitmap, 0, y, 256, scanline, pens, -1);
		}
	}
}
