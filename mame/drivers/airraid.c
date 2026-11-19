// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina, Angelo Salese, hap, David Haywood
/* Air Raid (aka Cross Shooter) (c) 1987 Seibu

  this driver is for the single board version on the S-0087-011A-0 PCB
  for the version using a S-0086-002-B0 base PCB and separate video board see stfight.c

  Video system is very similar to darkmist.c

  Custom Modules note:

  The Air Raid / Cross Shooter PCB contains 3 custom modules.
  These modules have not been dumped.

  2 of these are responsible for background layer generation.
  the 3rd is responsible for sprites.

  Both the tile gfx data, and tilemap layout data are
  contained within the background modules, none of this is
  accessible by the CPU (the CPU simply provides a scroll
  position)

  The sprite modules contain the sprite gfx data.

  The modules also contain the CLUT PROM data for their layer
  which controls colours and transparency.  It is likely
  each module contains the equivalent logic of a SEI0010BU
  in order to handle this.  This means there is a possibility
  that the raw data from the ROMs is not exposed on the edge
  of the modules, only post-lookup data.

  The above information is based off a development board
  which was unfortunately stripped of all ROMs.

-----------------------------------------------------------------------

  TODO:
  - cshooter/airraid gfx, obviously

-----------------------------------------------------------------------

Stephh's notes (based on the game Z80 code and some tests) :

  - Other notes :

      * 0x0006 contains the "region" stuff (value in 'cshooter' = 0xc4) :
          . bits 2 and 3 determine the manufacturer :
              0x00 : "J.K.H. Corp."         (no logo)
              0x04 : "Taito Corporation."   (+ logo)
              0x08 : "International Games"  (+ logo)
              0x0c : "Seibu Kaihatsu,Inc."  (+ logo)
          . bits 6 and 7 determine the title screen :
              0x00 : unknown - scrambled GFX *probably air raid, haze
              0x40 : unknown - scrambled GFX (alternate entry) *probably air raid, haze
              0x80 : "Cross Shooter"
              0xc0 : "Cross Shooter" (same as 0x80)

-----------------------------------------------------------------------

    Custom SIMM module placement

    The Air Raid PCB has 3 custom modules shown
    in the rough diagram of the PCB below.

    The modules do not appear to be 100%
    identical based on external shapes visible.

    The data they contain is as follows
    MODULE1
    Background tilemap graphic data
    Background tilemap layout data

    MODULE2
    Foreground tilemap graphic data
    Foreground tilemap layout data

    MODULE3
    Sprite graphic data

    it's also possible each module contains a
    0x100 colour look up table for that layer.


   |-------------------------------------------------|
   |    A   B   C   D   E   F   G   H   I   J   K    |
 20|            ##MOD1#######                        |
   | #                            Z80                |
   | M                                              -|
   | O                                              |
   | D                                              |=J
   | 2                                              |=A
   | #                                              |=M
 11|                                                |=M
   |                                                |=A
   |                                                |
   |                                                -|
   |                                  Z80            |
   |                                                 |
   |        ##MOD3#######                            |
   |-------------------------------------------------|

*/

#include "driver.h"
#include "sndhrdw/seibu.h"

#define MAIN_CLOCK 12000000
#define SOUND_CLOCK 14318180

#define DISPLAY_SPR 1
#define DISPLAY_FG 2
#define DISPLAY_BG 4
#define DISPLAY_TXT 8

#define BG_MASK_X 0x7fff
#define BG_MASK_Y 0x3ff
#define FG_MASK_X 0xfff
#define FG_MASK_Y 0x7ff

#define GETSCROLL(n) (((vregs[(n)] << 1) & 0xff) + ((vregs[(n)] & 0x80) ? 1 : 0) + (((vregs[(n) - 1] << 4) | (vregs[(n) - 1] << 12)) & 0xff00))

static UINT8 *txram;
static UINT8 *vregs;
static int hw;

static WRITE8_HANDLER( txram_w )
{
	txram[offset] = data;
}

static WRITE8_HANDLER( bank_w )
{
	/* format of this address is TTBB tbfs
	   TT = bg tilemap upper scroll bits
	   BB = prg ROM bank select
	   t = text layer enable
	   b = bg layer disable
	   f = fg layer disable
	   s = sprite layer enable */
	memory_set_bankptr(1, memory_region(REGION_USER1) + 0x4000 * ((data >> 4) & 3));

	hw = data & 0xcf;
}

static READ8_HANDLER( main_comm_r )
{
	return seibu_main_word_r(offset, 0);
}

static WRITE8_HANDLER( main_comm_w )
{
	/* the word handler expects the mask of the bits not accessed */
	seibu_main_word_w(offset, data, 0xff00);
}

static INTERRUPT_GEN( airraid_interrupt )
{
	if (cpu_getiloops())
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xd7); /* RST 10h, updates the scroll registers */
	else
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xcf); /* RST 08h, vblank-out */
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc000, 0xc000) AM_READ(input_port_0_r)
	AM_RANGE(0xc001, 0xc001) AM_READ(input_port_1_r)
	AM_RANGE(0xc002, 0xc002) AM_READ(input_port_2_r)
	AM_RANGE(0xc003, 0xc003) AM_READ(input_port_3_r)
	AM_RANGE(0xc004, 0xc004) AM_READ(input_port_4_r)
	AM_RANGE(0xc500, 0xc500) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc700, 0xc700) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xd000, 0xd7ff) AM_READWRITE(MRA8_RAM, txram_w) AM_BASE(&txram)
	AM_RANGE(0xd800, 0xd8ff) AM_READWRITE(MRA8_RAM, paletteram_xxxxBBBBGGGGRRRR_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0xda00, 0xdaff) AM_READWRITE(MRA8_RAM, paletteram_xxxxBBBBGGGGRRRR_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0xdc00, 0xdc0f) AM_RAM AM_BASE(&vregs)
	AM_RANGE(0xdc11, 0xdc11) AM_WRITE(bank_w)
	AM_RANGE(0xde00, 0xde0f) AM_READWRITE(main_comm_r, main_comm_w)
	AM_RANGE(0xe000, 0xfdff) AM_RAM
	AM_RANGE(0xfe00, 0xffff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x27ff) AM_RAM
	AM_RANGE(0x4000, 0x4000) AM_WRITE(seibu_pending_w)
	AM_RANGE(0x4001, 0x4001) AM_WRITE(seibu_irq_clear_w)
	AM_RANGE(0x4002, 0x4002) AM_WRITE(seibu_rst10_ack_w)
	AM_RANGE(0x4003, 0x4003) AM_WRITE(seibu_rst18_ack_w)
	AM_RANGE(0x4008, 0x4008) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x4009, 0x4009) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0x4010, 0x4011) AM_READ(seibu_soundlatch_r)
	AM_RANGE(0x4012, 0x4012) AM_READ(seibu_main_data_pending_r)
	AM_RANGE(0x4013, 0x4013) AM_READ(input_port_5_r)
	AM_RANGE(0x4018, 0x4019) AM_WRITE(seibu_main_data_w)
	AM_RANGE(0x401b, 0x401b) AM_WRITE(seibu_coin_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( airraid )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "2k 10k 20k" )
	PORT_DIPSETTING(    0x08, "5k 20k 40k" )
	PORT_DIPSETTING(    0x04, "6k 30k 60k" )
	PORT_DIPSETTING(    0x00, "7k 40k 80k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Coin Slots" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	SEIBU_COIN_INPUTS	/* COIN */
INPUT_PORTS_END

INPUT_PORTS_START( failgate )
	PORT_INCLUDE( airraid )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Energy Consumption" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_MODIFY("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0,4 },
	{ 8,9,10,11,0,1,2,3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128*1
};

static const gfx_layout char16layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,4,8,12 },
	{ 0,1,2,3, 16,17,18,19, 512+0,512+1,512+2,512+3, 512+16,512+17,512+18,512+19 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ REGION_GFX2, 0, &char16layout, 0, 16 },
	{ REGION_GFX3, 0, &char16layout, 0, 16 },
	{ REGION_GFX4, 0, &char16layout, 0, 16 },
	{ -1 }
};

SEIBU_SOUND_SYSTEM_YM2151_HARDWARE

/* the clut of each layer selects the final pen and its transparency (0x40),
   so the layers are drawn directly to the screen instead of a temporary bitmap */
static void draw_gfx(mame_bitmap *bitmap, const rectangle *cliprect, const gfx_element *gfx, int code, int color, int sx, int sy, const UINT8 *clut, int base, int transparent0)
{
	const UINT8 *data;
	int xstep, ystep;
	int x, y;

	if (sx > cliprect->max_x || sy > cliprect->max_y || sx + gfx->width <= cliprect->min_x || sy + gfx->height <= cliprect->min_y)
		return;

	data = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;
	color *= gfx->color_granularity;

	/* the gfx data is stored transposed in the rotated games */
	if (gfx->flags & GFX_SWAPXY)
	{
		xstep = gfx->line_modulo;
		ystep = 1;
	}
	else
	{
		xstep = 1;
		ystep = gfx->line_modulo;
	}

	for (y = 0; y < gfx->height; ++y)
	{
		int py = sy + y;

		if (py < cliprect->min_y || py > cliprect->max_y)
			continue;

		for (x = 0; x < gfx->width; ++x)
		{
			int px = sx + x;
			int pix;
			int real;

			if (px < cliprect->min_x || px > cliprect->max_x)
				continue;

			pix = data[y * ystep + x * xstep];

			if (transparent0 && pix == 0)
				continue;

			real = clut[color + pix];

			if (real & 0x40)
				continue;

			plot_pixel(bitmap, px, py, Machine->pens[(real & 0x3f) + base]);
		}
	}
}

static void draw_bg(mame_bitmap *bitmap, const rectangle *cliprect, int scrollx, int scrolly)
{
	const UINT8 *map = memory_region(REGION_USER2);
	const UINT8 *clut = memory_region(REGION_PROMS) + 0x200;
	int col0, row0, tx, ty;

	scrollx &= BG_MASK_X;
	scrolly &= BG_MASK_Y;
	col0 = scrollx >> 4;
	row0 = scrolly >> 4;

	for (ty = 0; ty < 17; ++ty)
	{
		int row = (row0 + ty) & 0x3f;

		for (tx = 0; tx < 17; ++tx)
		{
			int col = (col0 + tx) & 0x7ff;
			int index = ((row & 0xf) * 0x10) + (col & 0xf) + (((col & 0x7f0) >> 4) * 0x100) + ((row & 0x30) >> 4) * 0x8000;
			int attr = map[index * 2 + 0];
			int tile = map[index * 2 + 1] | ((attr & 0x70) << 4);

			draw_gfx(bitmap, cliprect, Machine->gfx[2], tile, attr & 0xf, tx * 16 - (scrollx & 15), ty * 16 - (scrolly & 15), clut, 0x80, 0);
		}
	}
}

static void draw_fg(mame_bitmap *bitmap, const rectangle *cliprect, int scrollx, int scrolly)
{
	const UINT8 *map = memory_region(REGION_USER3);
	const UINT8 *clut = memory_region(REGION_PROMS) + 0x100;
	int col0, row0, tx, ty;

	scrollx &= FG_MASK_X;
	scrolly &= FG_MASK_Y;
	col0 = scrollx >> 4;
	row0 = scrolly >> 4;

	for (ty = 0; ty < 17; ++ty)
	{
		int row = (row0 + ty) & 0x7f;

		for (tx = 0; tx < 17; ++tx)
		{
			int col = (col0 + tx) & 0xff;
			int index = ((row & 0xf) * 0x10) + (col & 0xf) + (((col & 0xf0) >> 4) * 0x100) + ((row & 0x70) >> 4) * 0x1000;
			int attr = map[index * 2 + 0];
			int tile = map[index * 2 + 1] | ((attr & 0x70) << 4);

			draw_gfx(bitmap, cliprect, Machine->gfx[3], tile, attr & 0xf, tx * 16 - (scrollx & 15), ty * 16 - (scrolly & 15), clut, 0x00, 0);
		}
	}
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const UINT8 *clut = memory_region(REGION_PROMS) + 0x300;
	int i;

	for (i = 0x200 - 4; i >= 0; i -= 4)
	{
		int tile;

		if (spriteram[i + 1] & 0x80)
			continue;

		tile = spriteram[i] | ((spriteram[i + 1] & 0x70) << 4);

		draw_gfx(bitmap, cliprect, Machine->gfx[1], tile, spriteram[i + 1] & 0x0f, spriteram[i + 3], spriteram[i + 2], clut, 0x40, 1);
	}
}

static void draw_tx(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const UINT8 *clut = memory_region(REGION_PROMS);
	int row, col;

	for (row = 0; row < 32; ++row)
	{
		for (col = 0; col < 32; ++col)
		{
			int index = row * 32 + col;
			int code = txram[index * 2];
			int attr = txram[index * 2 + 1];

			draw_gfx(bitmap, cliprect, Machine->gfx[0], (code << 1) | ((attr & 0x20) >> 5), attr & 0xf, col * 8, row * 8, clut, 0xc0, 0);
		}
	}
}

static VIDEO_START( airraid )
{
	hw = 0x09;
	return 0;
}

static VIDEO_UPDATE( airraid )
{
	fillbitmap(bitmap, Machine->pens[0x80], cliprect);

	if ((hw & DISPLAY_BG) == 0)
	{
		/* the upper scroll bits are in the hardware register */
		draw_bg(bitmap, cliprect, GETSCROLL(0x2), GETSCROLL(0x6) + ((hw & 0xc0) >> 6) * 256);
	}

	if ((hw & DISPLAY_FG) == 0)
		draw_fg(bitmap, cliprect, GETSCROLL(0xa), GETSCROLL(0xe));

	if (hw & DISPLAY_SPR)
		draw_sprites(bitmap, cliprect);

	if (hw & DISPLAY_TXT)
		draw_tx(bitmap, cliprect);
}

static MACHINE_DRIVER_START( airraid )
	MDRV_CPU_ADD(Z80, MAIN_CLOCK / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(airraid_interrupt, 2)

	MDRV_CPU_ADD(Z80, SOUND_CLOCK / 4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(20)

	MDRV_MACHINE_RESET(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-1-16)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(airraid)
	MDRV_VIDEO_UPDATE(airraid)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, SOUND_CLOCK / 4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( airraid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.16j",   0x00000, 0x08000, CRC(7ac2cedf) SHA1(272831f51a2731e067b5aec6dba6bddd3c5350c9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "5.6f",    0x00000, 0x02000, CRC(30be398c) SHA1(6c61200ee8888d6270c8cec50423b3b5602c2027) )
	ROM_LOAD( "4.7f",    0x08000, 0x08000, CRC(3cd715b4) SHA1(da735fb5d262908ddf7ed7dacdea68899f1723ff) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "2.19j",   0x00000, 0x10000, CRC(842ae6c2) SHA1(0468445e4ab6f42bac786f9a258df3972fd1fde9) )

	ROM_REGION( 0x0200, REGION_USER4, 0 )
	ROM_LOAD( "pr.c19",   0x0000, 0x0100, NO_DUMP )
	ROM_LOAD( "6308.a13", 0x0100, 0x0100, NO_DUMP )

	ROM_REGION( 0x02000, REGION_GFX1, 0 )	/* TX Layer */
	ROM_LOAD( "3.13e",   0x00000, 0x02000, CRC(672ec0e8) SHA1(a11cd90d6494251ceee3bc7c72f4e7b1580b77e2) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )	/* clut: tx, fg, bg, sprites */
	ROM_LOAD( "63s281.d16",  0x0000, 0x0100, CRC(0b8b914b) SHA1(8cf4910b846de79661cc187887171ed8ebfd6719) )
	ROM_LOAD( "fg_clut",     0x0100, 0x0100, NO_DUMP )
	ROM_LOAD( "bg_clut",     0x0200, 0x0100, NO_DUMP )
	ROM_LOAD( "spr_clut",    0x0300, 0x0100, NO_DUMP )

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "bg_layouts_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "bg_layouts_odd",    0x00001, 0x20000, NO_DUMP )
	ROM_REGION( 0x40000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "bg_tiles_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "bg_tiles_odd",    0x00001, 0x20000, NO_DUMP )

	ROM_REGION( 0x40000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "fg_layouts_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "fg_layouts_odd",    0x00001, 0x20000, NO_DUMP )
	ROM_REGION( 0x40000, REGION_GFX4, 0 )
	ROM_LOAD16_BYTE( "fg_tiles_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "fg_tiles_odd",    0x00001, 0x20000, NO_DUMP )

	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sprite_tiles_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "sprite_tiles_odd",    0x00001, 0x20000, NO_DUMP )
ROM_END

ROM_START( cshooter )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.k19",   0x00000, 0x08000, CRC(71418952) SHA1(9745ca006576381c9e9595d8e42ab276bab80a41) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "5.6f",    0x00000, 0x02000, CRC(30be398c) SHA1(6c61200ee8888d6270c8cec50423b3b5602c2027) )
	ROM_LOAD( "4.7f",    0x08000, 0x08000, CRC(3cd715b4) SHA1(da735fb5d262908ddf7ed7dacdea68899f1723ff) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "2.k20",   0x00000, 0x10000, CRC(5812fe72) SHA1(3b28bff6b62a411d2195bb228952db62ad32ef3d) )

	ROM_REGION( 0x0200, REGION_USER4, 0 )
	ROM_LOAD( "82s129.c19",  0x0000, 0x0100, CRC(dcd4e66d) SHA1(17f53c7905c20f0c6c0a74ab519b8e4a442835c3) )
	ROM_LOAD( "63s281.a13",  0x0100, 0x0100, CRC(208d17ca) SHA1(a77d56337bcac8d9a7bc3411239dfb3045e069ec) )

	ROM_REGION( 0x02000, REGION_GFX1, 0 )	/* TX Layer */
	ROM_LOAD( "3.13e",   0x00000, 0x02000, CRC(704c26d7) SHA1(e5964f409cbc2c4752e3969f3e84ace08d5ad9cb) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )	/* clut: tx, fg, bg, sprites */
	ROM_LOAD( "63s281.d16",  0x0000, 0x0100, CRC(0b8b914b) SHA1(8cf4910b846de79661cc187887171ed8ebfd6719) )
	ROM_LOAD( "fg_clut",     0x0100, 0x0100, NO_DUMP )
	ROM_LOAD( "bg_clut",     0x0200, 0x0100, NO_DUMP )
	ROM_LOAD( "spr_clut",    0x0300, 0x0100, NO_DUMP )

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "bg_layouts_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "bg_layouts_odd",    0x00001, 0x20000, NO_DUMP )
	ROM_REGION( 0x40000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "bg_tiles_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "bg_tiles_odd",    0x00001, 0x20000, NO_DUMP )

	ROM_REGION( 0x40000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "fg_layouts_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "fg_layouts_odd",    0x00001, 0x20000, NO_DUMP )
	ROM_REGION( 0x40000, REGION_GFX4, 0 )
	ROM_LOAD16_BYTE( "fg_tiles_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "fg_tiles_odd",    0x00001, 0x20000, NO_DUMP )

	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "sprite_tiles_even",   0x00000, 0x20000, NO_DUMP )
	ROM_LOAD16_BYTE( "sprite_tiles_odd",    0x00001, 0x20000, NO_DUMP )
ROM_END

ROM_START( failgate )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "fail.k19",   0x00000, 0x08000, CRC(b8ed4b0d) SHA1(9e204b7213625f0e17f87061fdcbe79a056b0986) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "fail.f6",    0x00000, 0x02000, CRC(9cb6e402) SHA1(089c6c186dc4816cc33207e5762c2721012041fa) )
	ROM_LOAD( "fail.f7",    0x08000, 0x08000, CRC(9d74085e) SHA1(8309ec3258590beaec342b4689a0d36c26137ed1) )

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "fail.k20",   0x00000, 0x10000, CRC(893132a0) SHA1(2893d235130903b65be6a17e0f32b70e8c8fea73) )

	ROM_REGION( 0x02000, REGION_GFX1, 0 )
	ROM_LOAD( "fail.f11",        0x00000, 0x02000, CRC(845047d0) SHA1(b39b402724ab2b486a4e32ca20e6cd289e354433) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )	/* clut: tx, fg, bg, sprites */
	ROM_LOAD( "fail.d16",        0x0000, 0x0100, CRC(c784cc55) SHA1(acb053d5f353b68139456eac472dc9f7542df042) )
	ROM_LOAD( "back0.fail.2b",   0x0100, 0x0100, CRC(dbb1709f) SHA1(4b82776bc0d852070a3187ee263513a73c03fac8) )
	ROM_LOAD( "back1.fail.2b",   0x0200, 0x0100, CRC(36eead99) SHA1(cc934605c0cfc39abbab65bdc18fbb1d48ffdace) )
	ROM_LOAD( "obj.fail.2a",     0x0300, 0x0100, CRC(5d11f0de) SHA1(c3d16395fd1b3f5501c73fd658a67a7ba91492ae) )

	ROM_REGION( 0x40000, REGION_USER2, 0 )
	ROM_LOAD( "back1.fail.1c",   0x00000, 0x40000, CRC(de6bb59b) SHA1(ca991c7d0d4f30b6f499fb2133fc3054fa9d8953) )
	ROM_REGION( 0x40000, REGION_GFX3, 0 )
	ROM_LOAD( "back1.fail.1b",   0x00000, 0x40000, CRC(0421b0f7) SHA1(aeb78f05bb956cbec0ad9d45f24914177a48e15a) )

	ROM_REGION( 0x10000, REGION_USER3, 0 )
	ROM_LOAD( "back0.fail.1c",   0x00000, 0x10000, CRC(71aae804) SHA1(d8c217364fede3db132849c76a09f28b701c045c) )
	ROM_REGION( 0x40000, REGION_GFX4, 0 )
	ROM_LOAD( "back0.fail.1b",   0x00000, 0x40000, CRC(a80066d6) SHA1(77a97b14b1f27f9b043e93f25dca427d6412bfdd) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 )
	ROM_LOAD( "obj.fail.1a",   0x00000, 0x20000, CRC(b32f1357) SHA1(92514eeb45ceef3463de741bc19da85737da8e1e) )
ROM_END

static DRIVER_INIT( airraid )
{
	memory_set_bankptr(1, memory_region(REGION_USER1));
	seibu_sound_decrypt(REGION_CPU2, 0x2000);
}

static DRIVER_INIT( airraide )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *decrypt = auto_malloc(0x8000);
	int A;

	memory_set_decrypted_region(0, 0x0000, 0x7fff, decrypt);

	for (A = 0x0000; A < 0x8000; A++)
	{
		/* decode the opcodes */
		decrypt[A] = rom[A];

		if (BIT(A,5) && !BIT(A,3))
			decrypt[A] ^= 0x40;

		if (BIT(A,10) && !BIT(A,9) && BIT(A,3))
			decrypt[A] ^= 0x20;

		if ((BIT(A,10) ^ BIT(A,9)) && BIT(A,1))
			decrypt[A] ^= 0x02;

		if (BIT(A,9) || !BIT(A,5) || BIT(A,3))
			decrypt[A] = BITSWAP8(decrypt[A],7,6,1,4,3,2,5,0);

		/* decode the data */
		if (BIT(A,5))
			rom[A] ^= 0x40;

		if (BIT(A,9) || !BIT(A,5))
			rom[A] = BITSWAP8(rom[A],7,6,1,4,3,2,5,0);
	}

	init_airraid();
}

GAME( 1987, cshooter, airraid,  airraid, airraid,  airraide, ROT270, "Seibu Kaihatsu (J.K.H. license)",  "Cross Shooter (Single PCB)", GAME_NOT_WORKING )
GAME( 1987, airraid,  0,        airraid, airraid,  airraide, ROT270, "Seibu Kaihatsu",                   "Air Raid (Single PCB)",      GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )

GAME( 1991, failgate, 0,        airraid, failgate, airraid,  ROT270, "Success (Seibu Kaihatsu license)", "Fail Gate",                  0 )
