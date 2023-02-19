/*********************************************************************

    generic.c

    Generic simple video functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "generic.h"



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

UINT8 *videoram;
UINT16 *videoram16;
UINT32 *videoram32;
size_t videoram_size;

UINT8 *colorram;
UINT16 *colorram16;
UINT32 *colorram32;

UINT8 *spriteram;			/* not used in this module... */
UINT16 *spriteram16;		/* ... */
UINT32 *spriteram32;		/* ... */

UINT8 *spriteram_2;
UINT16 *spriteram16_2;
UINT32 *spriteram32_2;

UINT8 *spriteram_3;
UINT16 *spriteram16_3;
UINT32 *spriteram32_3;

UINT8 *buffered_spriteram;
UINT16 *buffered_spriteram16;
UINT32 *buffered_spriteram32;

UINT8 *buffered_spriteram_2;
UINT16 *buffered_spriteram16_2;
UINT32 *buffered_spriteram32_2;

size_t spriteram_size;		/* ... here just for convenience */
size_t spriteram_2_size;
size_t spriteram_3_size;

UINT8 *dirtybuffer;
UINT16 *dirtybuffer16;
UINT32 *dirtybuffer32;

UINT8 *paletteram;
UINT16 *paletteram16;
UINT32 *paletteram32;

UINT8 *paletteram_2;	/* use when palette RAM is split in two parts */
UINT16 *paletteram16_2;

mame_bitmap *tmpbitmap;
int flip_screen_x, flip_screen_y;

static int global_attribute_changed;



/***************************************************************************

    Inline Helpers

***************************************************************************/

/*-------------------------------------------------
    paletteram16_le - return a 16-bit value
    assembled from the two bytes of little-endian
    palette RAM referenced by offset
-------------------------------------------------*/

INLINE UINT16 paletteram16_le(offs_t offset)
{
	return paletteram[offset & ~1] | (paletteram[offset | 1] << 8);
}


/*-------------------------------------------------
    paletteram16_be - return a 16-bit value
    assembled from the two bytes of big-endian
    palette RAM referenced by offset
-------------------------------------------------*/

INLINE UINT16 paletteram16_be(offs_t offset)
{
	return paletteram[offset | 1] | (paletteram[offset & ~1] << 8);
}


/*-------------------------------------------------
    paletteram16_split - return a 16-bit value
    assembled from the two bytes of split palette
    RAM referenced by offset
-------------------------------------------------*/

INLINE UINT16 paletteram16_split(offs_t offset)
{
	return paletteram[offset] | (paletteram_2[offset] << 8);
}


/*-------------------------------------------------
    paletteram32_be - return a 32-bit value
    assembled from the two words of big-endian
    palette RAM referenced by offset
-------------------------------------------------*/

INLINE UINT32 paletteram32_be(offs_t offset)
{
	return paletteram16[offset | 1] | (paletteram16[offset & ~1] << 16);
}


/*-------------------------------------------------
    set_color_444 - set a 4-4-4 RGB color using
    the 16-bit data provided and the specified
    shift values
-------------------------------------------------*/

INLINE void set_color_444(pen_t color, int rshift, int gshift, int bshift, UINT16 data)
{
	palette_set_color(color, pal4bit(data >> rshift), pal4bit(data >> gshift), pal4bit(data >> bshift));
}


/*-------------------------------------------------
    set_color_4444 - set a 4-4-4-4 IRGB color using
    the 16-bit data provided and the specified
    shift values
-------------------------------------------------*/

INLINE void set_color_4444(pen_t color, int ishift, int rshift, int gshift, int bshift, UINT16 data)
{
	static const UINT8 ztable[16] =
		{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11 };
	int i, r, g, b;

	i = ztable[(data >> ishift) & 15];
	r = ((data >> rshift) & 15) * i;
	g = ((data >> gshift) & 15) * i;
	b = ((data >> bshift) & 15) * i;

	palette_set_color(color, r, g, b);
}


/*-------------------------------------------------
    set_color_555 - set a 5-5-5 RGB color using
    the 16-bit data provided and the specified
    shift values
-------------------------------------------------*/

INLINE void set_color_555(pen_t color, int rshift, int gshift, int bshift, UINT16 data)
{
	palette_set_color(color, pal5bit(data >> rshift), pal5bit(data >> gshift), pal5bit(data >> bshift));
}


/*-------------------------------------------------
    set_color_8888 - set a 8-8-8 RGB color using
    the 32-bit data provided and the specified
    shift values
-------------------------------------------------*/

INLINE void set_color_888(pen_t color, int rshift, int gshift, int bshift, UINT32 data)
{
	palette_set_color(color, (data >> rshift) & 0xff, (data >> gshift) & 0xff, (data >> bshift) & 0xff);
}



/***************************************************************************

    Initialization

***************************************************************************/

/*-------------------------------------------------
    generic_video_init - initialize globals and
    register for save states
-------------------------------------------------*/

void generic_video_init(void)
{
	videoram = NULL;
	videoram16 = NULL;
	videoram32 = NULL;
	videoram_size = 0;
	colorram = NULL;
	colorram16 = NULL;
	colorram32 = NULL;
	spriteram = NULL;
	spriteram16 = NULL;
	spriteram32 = NULL;
	spriteram_2 = NULL;
	spriteram16_2 = NULL;
	spriteram32_2 = NULL;
	spriteram_3 = NULL;
	spriteram16_3 = NULL;
	spriteram32_3 = NULL;
	buffered_spriteram = NULL;
	buffered_spriteram16 = NULL;
	buffered_spriteram32 = NULL;
	buffered_spriteram_2 = NULL;
	buffered_spriteram16_2 = NULL;
	buffered_spriteram32_2 = NULL;
	spriteram_size = 0;		/* ... here just for convenience */
	spriteram_2_size = 0;
	spriteram_3_size = 0;
	dirtybuffer = NULL;
	dirtybuffer16 = NULL;
	dirtybuffer32 = NULL;
	tmpbitmap = NULL;
	flip_screen_x = flip_screen_y = 0;
}



/***************************************************************************

    Generic video start/update callbacks

***************************************************************************/

/*-------------------------------------------------
    video_generic_postload - post-load callback
    that marks all videoram as dirty
-------------------------------------------------*/

static void video_generic_postload(void)
{
	memset(dirtybuffer, 1, videoram_size);
}


/*-------------------------------------------------
    video_start_generic - general video system
    with dirty buffer support
-------------------------------------------------*/

VIDEO_START( generic )
{
	assert_always(videoram_size != 0, "VIDEO_START(generic) requires non-zero videoram_size");

	/* allocate memory for the dirty buffer */
	dirtybuffer = auto_malloc(videoram_size);
	memset(dirtybuffer, 1, videoram_size);

	/* allocate the temporary bitmap */
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (tmpbitmap == NULL)
		return 1;

	/* on a restore, we automatically zap the dirty buffer */
	state_save_register_func_postload(video_generic_postload);
	return 0;
}


/*-------------------------------------------------
    video_start_generic_bitmapped - general video
    system with a bitmap
-------------------------------------------------*/

VIDEO_START( generic_bitmapped )
{
	/* allocate the temporary bitmap */
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (tmpbitmap == NULL)
		return 1;

	/* ensure the contents of the bitmap are saved */
	state_save_register_bitmap("video", 0, "tmpbitmap", tmpbitmap);
	return 0;
}


/*-------------------------------------------------
    video_update_generic_bitmapped - blast the
    generic bitmap to the screen
-------------------------------------------------*/

VIDEO_UPDATE( generic_bitmapped )
{
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->visible_area, TRANSPARENCY_NONE, 0);
}



/***************************************************************************

    Generic read/write handlers

***************************************************************************/

/*-------------------------------------------------
    videoram_r/w - 8-bit access to videoram with
    dirty buffer marking
-------------------------------------------------*/

READ8_HANDLER( videoram_r )
{
	return videoram[offset];
}

WRITE8_HANDLER( videoram_w )
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		videoram[offset] = data;
	}
}


/*-------------------------------------------------
    colorram_r/w - 8-bit access to colorram with
    dirty buffer marking
-------------------------------------------------*/

READ8_HANDLER( colorram_r )
{
	return colorram[offset];
}

WRITE8_HANDLER( colorram_w )
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		colorram[offset] = data;
	}
}


/*-------------------------------------------------
    spriteram_r/w - 8-bit access to spriteram
-------------------------------------------------*/

READ8_HANDLER( spriteram_r )
{
	return spriteram[offset];
}

WRITE8_HANDLER( spriteram_w )
{
	spriteram[offset] = data;
}


/*-------------------------------------------------
    spriteram16_r/w - 16-bit access to spriteram16
-------------------------------------------------*/

READ16_HANDLER( spriteram16_r )
{
	return spriteram16[offset];
}

WRITE16_HANDLER( spriteram16_w )
{
	COMBINE_DATA(spriteram16+offset);
}


/*-------------------------------------------------
    spriteram_r/w - 8-bit access to spriteram2
-------------------------------------------------*/

READ8_HANDLER( spriteram_2_r )
{
	return spriteram_2[offset];
}

WRITE8_HANDLER( spriteram_2_w )
{
	spriteram_2[offset] = data;
}



/***************************************************************************

    Generic sprite buffering

***************************************************************************/

/* Mish:  171099

    'Buffered spriteram' is where the graphics hardware draws the sprites
from private ram that the main CPU cannot access.  The main CPU typically
prepares sprites for the next frame in it's own sprite ram as the graphics
hardware renders sprites for the current frame from private ram.  Main CPU
sprite ram is usually copied across to private ram by setting some flag
in the VBL interrupt routine.

    The reason for this is to avoid sprite flicker or lag - if a game
is unable to prepare sprite ram within a frame (for example, lots of sprites
on screen) then it doesn't trigger the buffering hardware - instead the
graphics hardware will use the sprites from the last frame. An example is
Dark Seal - the buffer flag is only written to if the CPU is idle at the time
of the VBL interrupt.  If the buffering is not emulated the sprites flicker
at busy scenes.

    Some games seem to use buffering because of hardware constraints -
Capcom games (Cps1, Last Duel, etc) render spriteram _1 frame ahead_ and
buffer this spriteram at the end of a frame, so the _next_ frame must be drawn
from the buffer.  Presumably the graphics hardware and the main cpu cannot
share the same spriteram for whatever reason.

    Sprite buffering & Mame:

    To use sprite buffering in a driver use VIDEO_BUFFERS_SPRITERAM in the
machine driver.  This will automatically create an area for buffered spriteram
equal to the size of normal spriteram.

    Spriteram size _must_ be declared in the memory map:

    { 0x120000, 0x1207ff, MWA8_BANK2, &spriteram, &spriteram_size },

    Then the video driver must draw the sprites from the buffered_spriteram
pointer.  The function buffer_spriteram_w() is used to simulate hardware
which buffers the spriteram from a memory location write.  The function
buffer_spriteram(UINT8 *ptr, int length) can be used where
more control is needed over what is buffered.

    For examples see darkseal.c, contra.c, lastduel.c, bionicc.c etc.

*/


/*-------------------------------------------------
    buffer_spriteram_w - triggered writes to
    buffer spriteram
-------------------------------------------------*/

WRITE8_HANDLER( buffer_spriteram_w )
{
	memcpy(buffered_spriteram, spriteram, spriteram_size);
}

WRITE16_HANDLER( buffer_spriteram16_w )
{
	memcpy(buffered_spriteram16, spriteram16, spriteram_size);
}

WRITE32_HANDLER( buffer_spriteram32_w )
{
	memcpy(buffered_spriteram32, spriteram32, spriteram_size);
}


/*-------------------------------------------------
    buffer_spriteram_2_w - triggered writes to
    buffer spriteram_2
-------------------------------------------------*/

WRITE8_HANDLER( buffer_spriteram_2_w )
{
	memcpy(buffered_spriteram_2, spriteram_2, spriteram_2_size);
}

WRITE16_HANDLER( buffer_spriteram16_2_w )
{
	memcpy(buffered_spriteram16_2, spriteram16_2, spriteram_2_size);
}

WRITE32_HANDLER( buffer_spriteram32_2_w )
{
	memcpy(buffered_spriteram32_2, spriteram32_2, spriteram_2_size);
}


/*-------------------------------------------------
    buffer_spriteram - for manually buffering
    spriteram
-------------------------------------------------*/

void buffer_spriteram(UINT8 *ptr, int length)
{
	memcpy(buffered_spriteram, ptr, length);
}

void buffer_spriteram_2(UINT8 *ptr, int length)
{
	memcpy(buffered_spriteram_2, ptr, length);
}



/***************************************************************************

    Global video attribute handling code

***************************************************************************/

/*-------------------------------------------------
    updateflip - handle global flipping
-------------------------------------------------*/

static void updateflip(void)
{
	int min_x,max_x,min_y,max_y;

	tilemap_set_flip(ALL_TILEMAPS,(TILEMAP_FLIPX & flip_screen_x) | (TILEMAP_FLIPY & flip_screen_y));

	min_x = Machine->drv->default_visible_area.min_x;
	max_x = Machine->drv->default_visible_area.max_x;
	min_y = Machine->drv->default_visible_area.min_y;
	max_y = Machine->drv->default_visible_area.max_y;

	if (flip_screen_x)
	{
		int temp;

		temp = Machine->drv->screen_width - min_x - 1;
		min_x = Machine->drv->screen_width - max_x - 1;
		max_x = temp;
	}
	if (flip_screen_y)
	{
		int temp;

		temp = Machine->drv->screen_height - min_y - 1;
		min_y = Machine->drv->screen_height - max_y - 1;
		max_y = temp;
	}

	set_visible_area(min_x,max_x,min_y,max_y);
}


/*-------------------------------------------------
    flip_screen_set - set global flip
-------------------------------------------------*/

void flip_screen_set(int on)
{
	flip_screen_x_set(on);
	flip_screen_y_set(on);
}


/*-------------------------------------------------
    flip_screen_x_set - set global horizontal flip
-------------------------------------------------*/

void flip_screen_x_set(int on)
{
	if (on) on = ~0;
	if (flip_screen_x != on)
	{
		set_vh_global_attribute(&flip_screen_x,on);
		updateflip();
	}
}


/*-------------------------------------------------
    flip_screen_y_set - set global vertical flip
-------------------------------------------------*/

void flip_screen_y_set(int on)
{
	if (on) on = ~0;
	if (flip_screen_y != on)
	{
		set_vh_global_attribute(&flip_screen_y,on);
		updateflip();
	}
}


/*-------------------------------------------------
    set_vh_global_attribute - set an arbitrary
    global video attribute
-------------------------------------------------*/

void set_vh_global_attribute( int *addr, int data )
{
	if (!addr || *addr != data)
	{
		global_attribute_changed = 1;
		if (addr)
			*addr = data;
	}
}


/*-------------------------------------------------
    get_vh_global_attribute - set an arbitrary
    global video attribute
-------------------------------------------------*/

int get_vh_global_attribute_changed(void)
{
	int result = global_attribute_changed;
	global_attribute_changed = 0;
	return result;
}



/***************************************************************************

    Common palette initialization functions

***************************************************************************/

/*-------------------------------------------------
    black_and_white - basic 2-color black & white
-------------------------------------------------*/

PALETTE_INIT( black_and_white )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff); /* white */
}


/*-------------------------------------------------
    RRRR_GGGG_BBBB - standard 4-4-4 palette,
    assuming the commonly used resistor values:

    bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
          -- 470 ohm resistor  -- RED/GREEN/BLUE
          -- 1  kohm resistor  -- RED/GREEN/BLUE
    bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE
-------------------------------------------------*/

PALETTE_INIT( RRRR_GGGG_BBBB )
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* green component */
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* blue component */
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}
}



/***************************************************************************

    Generic palette read handlers

***************************************************************************/

/*-------------------------------------------------
    8-bit read handlers
-------------------------------------------------*/

READ8_HANDLER( paletteram_r )
{
	return paletteram[offset];
}

READ8_HANDLER( paletteram_2_r )
{
	return paletteram_2[offset];
}


/*-------------------------------------------------
    16-bit read handlers
-------------------------------------------------*/

READ16_HANDLER( paletteram16_word_r )
{
	return paletteram16[offset];
}

READ16_HANDLER( paletteram16_2_word_r )
{
	return paletteram16_2[offset];
}


/*-------------------------------------------------
    32-bit read handlers
-------------------------------------------------*/

READ32_HANDLER( paletteram32_r )
{
	return paletteram32[offset];
}



/***************************************************************************

    3-3-2 RGB palette write handlers

***************************************************************************/

/*-------------------------------------------------
    RRR-GGG-BB writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_RRRGGGBB_w )
{
	paletteram[offset] = data;
	palette_set_color(offset, pal3bit(data >> 5), pal3bit(data >> 2), pal2bit(data >> 0));
}


/*-------------------------------------------------
    BB-GGG-RR writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_BBGGGRRR_w )
{
	paletteram[offset] = data;
	palette_set_color(offset, pal3bit(data >> 0), pal3bit(data >> 3), pal2bit(data >> 6));
}


/*-------------------------------------------------
    BB-GG-RR-II writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_BBGGRRII_w )
{
	int i = (data >> 0) & 3;

	paletteram[offset] = data;
	palette_set_color(offset, pal4bit((data >> 0) & 0x0c) | i,
	                          pal4bit((data >> 2) & 0x0c) | i,
	                          pal4bit((data >> 4) & 0x0c) | i);
}



/***************************************************************************

    4-4-4 RGB palette write handlers

***************************************************************************/

/*-------------------------------------------------
    xxxx-BBBB-GGGG-RRRR writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xxxxBBBBGGGGRRRR_le_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 0, 4, 8, paletteram16_le(offset));
}

WRITE8_HANDLER( paletteram_xxxxBBBBGGGGRRRR_be_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 0, 4, 8, paletteram16_be(offset));
}

WRITE8_HANDLER( paletteram_xxxxBBBBGGGGRRRR_split1_w )
{
	paletteram[offset] = data;
	set_color_444(offset, 0, 4, 8, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_xxxxBBBBGGGGRRRR_split2_w )
{
	paletteram_2[offset] = data;
	set_color_444(offset, 0, 4, 8, paletteram16_split(offset));
}

WRITE16_HANDLER( paletteram16_xxxxBBBBGGGGRRRR_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_444(offset, 0, 4, 8, paletteram16[offset]);
}


/*-------------------------------------------------
    xxxx-BBBB-RRRR-GGGG writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xxxxBBBBRRRRGGGG_le_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 4, 0, 8, paletteram16_le(offset));
}

WRITE8_HANDLER( paletteram_xxxxBBBBRRRRGGGG_be_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 4, 0, 8, paletteram16_be(offset));
}

WRITE8_HANDLER( paletteram_xxxxBBBBRRRRGGGG_split1_w )
{
	paletteram[offset] = data;
	set_color_444(offset, 4, 0, 8, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_xxxxBBBBRRRRGGGG_split2_w )
{
	paletteram_2[offset] = data;
	set_color_444(offset, 4, 0, 8, paletteram16_split(offset));
}

WRITE16_HANDLER( paletteram16_xxxxBBBBRRRRGGGG_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_444(offset, 4, 0, 8, paletteram16[offset]);
}


/*-------------------------------------------------
    xxxx-RRRR-BBBB-GGGG writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xxxxRRRRBBBBGGGG_split1_w )
{
	paletteram[offset] = data;
	set_color_444(offset, 8, 0, 4, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_xxxxRRRRBBBBGGGG_split2_w )
{
	paletteram_2[offset] = data;
	set_color_444(offset, 8, 0, 4, paletteram16_split(offset));
}


/*-------------------------------------------------
    xxxx-RRRR-GGGG-BBBB writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xxxxRRRRGGGGBBBB_le_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 8, 4, 0, paletteram16_le(offset));
}

WRITE8_HANDLER( paletteram_xxxxRRRRGGGGBBBB_be_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 8, 4, 0, paletteram16_be(offset));
}

WRITE8_HANDLER( paletteram_xxxxRRRRGGGGBBBB_split1_w )
{
	paletteram[offset] = data;
	set_color_444(offset, 8, 4, 0, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_xxxxRRRRGGGGBBBB_split2_w )
{
	paletteram_2[offset] = data;
	set_color_444(offset, 8, 4, 0, paletteram16_split(offset));
}

WRITE16_HANDLER( paletteram16_xxxxRRRRGGGGBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_444(offset, 8, 4, 0, paletteram16[offset]);
}


/*-------------------------------------------------
    RRRR-GGGG-BBBB-xxxx writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_RRRRGGGGBBBBxxxx_be_w )
{
	paletteram[offset] = data;
	set_color_444(offset / 2, 12, 8, 4, paletteram16_be(offset));
}

WRITE8_HANDLER( paletteram_RRRRGGGGBBBBxxxx_split1_w )
{
	paletteram[offset] = data;
	set_color_444(offset, 12, 8, 4, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_RRRRGGGGBBBBxxxx_split2_w )
{
	paletteram_2[offset] = data;
	set_color_444(offset, 12, 8, 4, paletteram16_split(offset));
}

WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBxxxx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_444(offset, 12, 8, 4, paletteram16[offset]);
}



/***************************************************************************

    5-5-5 RGB palette write handlers

***************************************************************************/

/*-------------------------------------------------
    x-BBBBB-GGGGG-RRRRR writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xBBBBBGGGGGRRRRR_le_w )
{
	paletteram[offset] = data;
	set_color_555(offset / 2, 0, 5, 10, paletteram16_le(offset));
}

WRITE8_HANDLER( paletteram_xBBBBBGGGGGRRRRR_be_w )
{
	paletteram[offset] = data;
	set_color_555(offset / 2, 0, 5, 10, paletteram16_be(offset));
}

WRITE8_HANDLER( paletteram_xBBBBBGGGGGRRRRR_split1_w )
{
	paletteram[offset] = data;
	set_color_555(offset, 0, 5, 10, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_xBBBBBGGGGGRRRRR_split2_w )
{
	paletteram_2[offset] = data;
	set_color_555(offset, 0, 5, 10, paletteram16_split(offset));
}

WRITE16_HANDLER( paletteram16_xBBBBBGGGGGRRRRR_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_555(offset, 0, 5, 10, paletteram16[offset]);
}


/*-------------------------------------------------
    x-BBBBB-RRRRR-GGGGG writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xBBBBBRRRRRGGGGG_split1_w )
{
	paletteram[offset] = data;
	set_color_555(offset, 5, 0, 10, paletteram16_split(offset));
}

WRITE8_HANDLER( paletteram_xBBBBBRRRRRGGGGG_split2_w )
{
	paletteram_2[offset] = data;
	set_color_555(offset, 5, 0, 10, paletteram16_split(offset));
}


/*-------------------------------------------------
    x-RRRRR-GGGGG-BBBBB writes
-------------------------------------------------*/

WRITE8_HANDLER( paletteram_xRRRRRGGGGGBBBBB_le_w )
{
	paletteram[offset] = data;
	set_color_555(offset / 2, 10, 5, 0, paletteram16_le(offset));
}

WRITE16_HANDLER( paletteram16_xRRRRRGGGGGBBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_555(offset, 10, 5, 0, paletteram16[offset]);
}


/*-------------------------------------------------
    x-GGGGG-RRRRR-BBBBB writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_xGGGGGRRRRRBBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_555(offset, 5, 10, 0, paletteram16[offset]);
}


/*-------------------------------------------------
    x-GGGGG-BBBBB-RRRRR writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_xGGGGGBBBBBRRRRR_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_555(offset, 0, 10, 5, paletteram16[offset]);
}


/*-------------------------------------------------
    RRRRR-GGGGG-BBBBB-x writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_RRRRRGGGGGBBBBBx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_555(offset, 11, 6, 1, paletteram16[offset]);
}


/*-------------------------------------------------
    RRRR-GGGG-BBBB-RGBx writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBRGBx_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	data = paletteram16[offset];
	palette_set_color(offset, pal5bit(((data >> 11) & 0x1e) | ((data >> 3) & 0x01)),
	                          pal5bit(((data >>  7) & 0x1e) | ((data >> 2) & 0x01)),
	                          pal5bit(((data >>  3) & 0x1e) | ((data >> 1) & 0x01)));
}



/***************************************************************************

    4-4-4-4 RGBI palette write handlers

***************************************************************************/

/*-------------------------------------------------
    IIII-RRRR-GGGG-BBBB writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_IIIIRRRRGGGGBBBB_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_4444(offset, 12, 8, 4, 0, paletteram16[offset]);
}


/*-------------------------------------------------
    RRRR-GGGG-BBBB-IIII writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_RRRRGGGGBBBBIIII_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_4444(offset, 0, 12, 8, 4, paletteram16[offset]);
}



/***************************************************************************

    8-8-8 RGB palette write handlers

***************************************************************************/

/*-------------------------------------------------
    xxxxxxxx-RRRRRRRR-GGGGGGGG-BBBBBBBB writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_xrgb_word_be_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_888(offset / 2, 16, 8, 0, paletteram32_be(offset));
}


/*-------------------------------------------------
    xxxxxxxx-BBBBBBBB-GGGGGGGG-RRRRRRRR writes
-------------------------------------------------*/

WRITE16_HANDLER( paletteram16_xbgr_word_be_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	set_color_888(offset / 2, 0, 8, 16, paletteram32_be(offset));
}
