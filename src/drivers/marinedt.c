// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/*****************************************************************************************************************

	Marine Date (c) 1981 Taito

	driver by Angelo Salese

	TODO:
	- discrete sound
	- imperfect colors: unused bit 2 of color prom, guesswokred sea gradient, mg16 entirely unused.
	  also unused colors 0x10-0x1f (might be a flashing bank)
	- collision detection isn't perfect, sometimes octopus gets stuck and dies even if moves are still available.
	  HW collision detection isn't perfect even from the reference, presumably needs a trojan run on the real HW.
	- ROM writes (irq mask?)
	- Merge devices with crbaloon/bking/grchamp drivers (PC3259).

*****************************************************************************************************************

Marine Date
Taito 1981
PCB Layout
----------
Top board
MGO70001
MGN00001
 |---------------------------------------------|
 | VOL   VR1  VR2  VR3  VR4  VR5  VR6  VR7     |
 |  LM3900 LM3900 LM3900 LM3900 LM3900 LM3900 |-|
 |MB3712                                      |P|
 |   4006  LM3900 LM3900 LM3900               | |
 |2  4030                                     |-|
 |2                                            |
 |W                                  DSW(8)    |
 |A                                           |-|
 |Y   HD14584     NE555       MG17   DSW(8)   |Q|
 |                                            | |
 |    HD14584                                 |-|
 |          HD14584                            |
 |---------------------------------------------|
Notes: (PCB contains lots of resistors/caps/transistors etc)
      MG17    - 82S123 bipolar PROM (no location on PCB)
      MB3712  - Hitachi MB3712 Audio Power Amplifier
      LM3900  - Texas Instruments LM3900 Quad Operational Amplifier
      HD14584 - Hitachi HD14584 Hex schmitt Trigger
      NE555   - NE555 Timer
      4006    - RCA CD4006 18-Stage Static Register
      4030    - RCA CD4030 Quad Exclusive-Or Gate
      VR*     - Volume pots for each sound
      VOL     - Master Volume pot
Middle board
MGO70002
MGN00002
 |---------------------------------------------|
 |                                    MG15.1A  |
|-|                                   MG14.2A |-|
|S|                                           |Q|
| |                                           | |
|-|               MG16.4E                     |-|
 |                                             |
 |                                             |
|-|    MG13.6H              MG12.6C           |-|
|R|                                           |P|
| |                                   PC3259  | |
|-|                                   PC3259  |-|
 |                                             |
 |---------------------------------------------|
Notes:
      MG12/13    - Hitachi HN462532 4kx8 EPROM
      MG14/15/16 - 82S123 bipolar PROM
      PC3259     - PC3259 8025 H08 unknown DIP24 IC. Package design indicates it was manufactured by Fujitsu
Lower board
AA017779
sticker: MGN00003
sticker: CLN00002
 |---------------------------------------------|
 | 9.987MHz               2114                 |
|-|                       2114                 |
|R|             MG07.10D       2114            |
| |             MG06.9D        2114            |
|-|                            2114           1|
 |              MG05.7D                       8|Edge
 |              MG04.6D                       W|Connector 'T'
|-|             MG03.5D                       A|
|S|             MG02.4D                       Y|
| |             MG01.3D  MG09.4F               |
|-|                      MG10.3F               |
 |              Z80      MG11.1F               |
 |---------------------------------------------|
Notes:
      Z80  - Clock 2.49675MHz [9.987/4]
      2114 - 1kx4 SRAM
      All EPROMs are 2716
      Wire jumpers for ROM configuration - J1 open
                                           J2 1-2, 3-9, 4-8, 5-7
                                           J4 1-2, 4-5, 7-8, 10-11
Top and Middle PCBs are plugged in with the solder-sides together.
Lower PCB is plugged in with components facing up.

*****************************************************************************************************************/

#include "driver.h"

static tilemap *tx_tilemap;
static UINT8 *marinedt_vram;

static mame_bitmap *marinedt_sea_bitmap[2];
static mame_bitmap *marinedt_obj_bitmap[2];

static struct
{
	UINT8 offs;
	UINT8 x;
	UINT8 y;
} marinedt_obj[2];

static UINT8 marinedt_layer_en;
static UINT8 marinedt_in_select;
static int marinedt_screen_flip;
static UINT8 marinedt_sea_bank;

static void get_tile_info(int tile_index)
{
	int code = marinedt_vram[tile_index];
	SET_TILE_INFO(0, code, 0, 0);
}

// initialize sea bitmap gradient
static void init_seabitmap(void)
{
	rectangle clip;
	int x, y;

	clip.min_x = 32;
	clip.max_x = 256;
	clip.min_y = 32;
	clip.max_y = 256;

	fillbitmap(marinedt_sea_bitmap[0], 64, &clip);
	fillbitmap(marinedt_sea_bitmap[1], 64 + 32, &clip);

	for (y = clip.min_y; y <= clip.max_y; y++)
	{
		for (x = clip.min_x; x <= clip.max_x; x++)
		{
			UINT8 blue_pen = 0x48 + ((x - 32) / 8);
			if (blue_pen > 0x5f)
				blue_pen = 0x5f;

			plot_pixel(marinedt_sea_bitmap[0], x, y, blue_pen);
			plot_pixel(marinedt_sea_bitmap[1], x, y, blue_pen + 0x20);
		}
	}
}

VIDEO_START( marinedt )
{
	tx_tilemap = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	if (!tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap, 0);

	marinedt_sea_bitmap[0] = auto_bitmap_alloc(512, 512);
	marinedt_sea_bitmap[1] = auto_bitmap_alloc(512, 512);
	marinedt_obj_bitmap[0] = auto_bitmap_alloc(512, 512);
	marinedt_obj_bitmap[1] = auto_bitmap_alloc(512, 512);

	if (!marinedt_sea_bitmap[0] || !marinedt_sea_bitmap[1] ||
		!marinedt_obj_bitmap[0] || !marinedt_obj_bitmap[1])
		return 1;

	init_seabitmap();

	state_save_register_global(marinedt_obj[0].x);
	state_save_register_global(marinedt_obj[0].y);
	state_save_register_global(marinedt_obj[0].offs);
	state_save_register_global(marinedt_obj[1].x);
	state_save_register_global(marinedt_obj[1].y);
	state_save_register_global(marinedt_obj[1].offs);
	state_save_register_global_bitmap(marinedt_obj_bitmap[0]);
	state_save_register_global_bitmap(marinedt_obj_bitmap[1]);
	state_save_register_global(marinedt_layer_en);
	state_save_register_global(marinedt_sea_bank);

	return 0;
}

VIDEO_UPDATE( marinedt )
{
	if (marinedt_layer_en & 8)
		copybitmap(bitmap, marinedt_sea_bitmap[marinedt_sea_bank],
				marinedt_screen_flip == 0, marinedt_screen_flip == 0,
				marinedt_screen_flip ? 0 : -256, marinedt_screen_flip ? 0 : -224,
				cliprect, TRANSPARENCY_NONE, 0);
	else
		fillbitmap(bitmap, 0, cliprect);

	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);

	if (marinedt_layer_en & 2)
		copybitmap(bitmap, marinedt_obj_bitmap[1], 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
	if (marinedt_layer_en & 1)
		copybitmap(bitmap, marinedt_obj_bitmap[0], 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
}

static WRITE8_HANDLER( marinedt_vram_w )
{
	marinedt_vram[offset] = data;
	tilemap_mark_tile_dirty(tx_tilemap, offset);
}

static void marinedt_obj_reg_w(UINT8 which, UINT8 reg, UINT8 data)
{
	rectangle visarea;
	int tilenum, color, fx, fy;

	visarea.min_x = 0;
	visarea.max_x = 255;
	visarea.min_y = 32;
	visarea.max_y = 255;

	switch (reg)
	{
		case 0: marinedt_obj[which].offs = data; break;
		// TODO: are offsets good?
		case 1: marinedt_obj[which].x = (data + 4) & 0xff; break;
		case 2: marinedt_obj[which].y = (data + 1) & 0xff; break;
	}

	tilenum = ((marinedt_obj[which].offs & 4) << 1) | ((marinedt_obj[which].offs & 0x38) >> 3);
	color = (marinedt_obj[which].offs & 3);
	fx = (marinedt_obj[which].offs >> 6) & 1;
	fy = (marinedt_obj[which].offs >> 7) & 1;

	fillbitmap(marinedt_obj_bitmap[which], 0, &visarea);
	drawgfx(marinedt_obj_bitmap[which], Machine->gfx[which + 1],
			tilenum, color, fx, !fy,
			marinedt_obj[which].x, marinedt_obj[which].y,
			&visarea, TRANSPARENCY_PEN, 0);
}

static WRITE8_HANDLER( marinedt_obj_0_w )
{
	marinedt_obj_reg_w(0, offset, data);
}

static WRITE8_HANDLER( marinedt_obj_1_w )
{
	marinedt_obj_reg_w(1, offset, data);
}

inline static UINT32 marinedt_obj_to_obj_collision(void)
{
	int x, y;

	// bail out if any obj is disabled
	if ((marinedt_layer_en & 3) != 3)
		return 0;

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32; x++)
		{
			int resx, resy;

			resx = marinedt_obj[0].x + x;
			resy = marinedt_obj[0].y + y;

			if (resx < 0 || resx >= 512 || resy < 0 || resy >= 512)
				continue;

			if ((read_pixel(marinedt_obj_bitmap[0], resx, resy) & 3) == 0)
				continue;

			// return value is never read most likely
			if (read_pixel(marinedt_obj_bitmap[1], resx, resy) != 0)
				return ((resy / 8) * 32) | (((resx / 8) - 1) & 0x1f);
		}
	}

	return 0;
}

inline static UINT32 marinedt_obj_to_layer_collision(void)
{
	int x, y;
	mame_bitmap *pixmap;

	// bail out if obj target is disabled
	if ((marinedt_layer_en & 1) == 0)
		return 0;

	pixmap = tilemap_get_pixmap(tx_tilemap);

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 32; x++)
		{
			int resx, resy;

			resx = marinedt_obj[0].x + x;
			resy = marinedt_obj[0].y + y;

			if (resx < 0 || resx >= 512 || resy < 0 || resy >= 512)
				continue;

			if ((read_pixel(marinedt_obj_bitmap[0], resx, resy) & 3) == 0)
				continue;

			if (!marinedt_screen_flip)
				resy -= 32;

			if (resx < 0 || resx >= pixmap->width || resy < 0 || resy >= pixmap->height)
				continue;

			// TODO: non screen flip path doesn't work properly
			if (read_pixel(pixmap, resx, resy) != 0)
			{
				if (marinedt_screen_flip)
					return ((resy / 8) * 32) | (((resx / 8) - 1) & 0x1f);
				else
					return (((resy / 8) * 32) | (((resx / 8) - 1) & 0x1f)) ^ 0x3ff;
			}
		}
	}

	return 0;
}

static UINT8 marinedt_pc3259_r(offs_t offset)
{
	UINT32 rest, reso;
	UINT8 reg = offset >> 2;
	UINT8 xt, xo;
	rest = marinedt_obj_to_layer_collision();
	reso = marinedt_obj_to_obj_collision();

	switch (reg)
	{
		case 0:
			xt = rest & 0xf;
			xo = reso & 0xf;
			return xt | (xo << 4);
		case 1:
			xt = (rest & 0xf0) >> 4;
			xo = (reso & 0xf0) >> 4;
			return xt | (xo << 4);
		case 2:
			xt = (rest & 0x300) >> 8;
			xo = (reso & 0x300) >> 8;
			return xt | (xo << 4);
		case 3:
		{
			UINT8 res = 0;
			res |= ((reso != 0) << 7);
			res |= ((rest != 0) << 3);
			return res;
		}
	}

	return 0;
}

static READ8_HANDLER( marinedt_pc3259_0_r ) { return marinedt_pc3259_r(0); }
static READ8_HANDLER( marinedt_pc3259_1_r ) { return marinedt_pc3259_r(4); }
static READ8_HANDLER( marinedt_pc3259_2_r ) { return marinedt_pc3259_r(8); }
static READ8_HANDLER( marinedt_pc3259_3_r ) { return marinedt_pc3259_r(12); }

static READ8_HANDLER( marinedt_trackball_r )
{
	return readinputport(3 + (marinedt_in_select & 3));
}

// discrete sound
static WRITE8_HANDLER( marinedt_bgm_w )
{
}

static WRITE8_HANDLER( marinedt_sfx_w )
{
	/*
	 x--- ---- unknown, probably ties to PC3259 pin 16 like crbaloon
	 --x- ---- jet sound SFX
	 ---x ---- foam SFX
	 ---- x--- ink SFX
	 ---- -x-- collision SFX
	 ---- --x- dots hit SFX
	 ---- ---x irq mask in crbaloon, doesn't seem to apply here?
	 */
}

static WRITE8_HANDLER( marinedt_layer_enable_w )
{
	/*
	    ---x ---- enabled when shark appears (enables red gradient on sea bitmap apparently)
	    ---- x--- sea layer draw enable (disabled in test mode)
	    ---- --x- obj 2 draw enable
	    ---- ---x obj 1 draw enable
	*/
	marinedt_layer_en = data & 0x0f;
	marinedt_sea_bank = (data & 0x10) >> 4;
}

static WRITE8_HANDLER( marinedt_output_w )
{
	/*
	    ---- x--- trackball input select (x/y)
	    ---- -x-- trackball player select
	    ---- --x- flipscreen
	    ---- ---x global coin lockout (disabled in service mode)
	*/
	marinedt_in_select = (data & 0x0c) >> 2;
	marinedt_screen_flip = (data >> 1) & 1;
	flip_screen_set(!marinedt_screen_flip);
	coin_lockout_global_w(!(data & 1));
}

static ADDRESS_MAP_START( marinedt_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0400) AM_READ(MRA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_MIRROR(0x0400) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( marinedt_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0400) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x4800, 0x4bff) AM_MIRROR(0x0400) AM_WRITE(marinedt_vram_w) AM_BASE(&marinedt_vram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( marinedt_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(4) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(marinedt_trackball_r)
	AM_RANGE(0x02, 0x02) AM_READ(marinedt_pc3259_0_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_1_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x06, 0x06) AM_READ(marinedt_pc3259_1_r)
	AM_RANGE(0x0a, 0x0a) AM_READ(marinedt_pc3259_2_r)
	AM_RANGE(0x0e, 0x0e) AM_READ(marinedt_pc3259_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( marinedt_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(4) )
	AM_RANGE(0x02, 0x04) AM_WRITE(marinedt_obj_0_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(marinedt_bgm_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(marinedt_sfx_w)
	AM_RANGE(0x08, 0x0b) AM_WRITE(marinedt_obj_1_w)
	AM_RANGE(0x0d, 0x0d) AM_WRITE(marinedt_layer_enable_w)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x0f, 0x0f) AM_WRITE(marinedt_output_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( marinedt )
	PORT_START	/* IN0 - DSW1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SWA:4,3,2,1")
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SWA:8,7,6,5")
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START	/* IN1 - SYSTEM */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Ink Button")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 Ink Button") PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* IN2 - DSW2 */
	PORT_DIPNAME( 0x01, 0x00, "DSWB" ) PORT_DIPLOCATION("SWB:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Disable sprite-tile collision (Cheat)" ) PORT_DIPLOCATION("SWB:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_BIT( 0x04, 0x00, IPT_DIPSWITCH_NAME ) PORT_NAME( DEF_STR( Service_Mode ) ) PORT_DIPLOCATION("SWB:3") PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SWB:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, "Number of Coin Chutes" ) PORT_DIPLOCATION("SWB:5")
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x20, 0x00, "Year Display" ) PORT_DIPLOCATION("SWB:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("SWB:8,7")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	PORT_START	/* IN3 - P1_TRACKX */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_START	/* IN4 - P2_TRACKX */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE PORT_COCKTAIL

	PORT_START	/* IN5 - P1_TRACKY */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10)

	PORT_START	/* IN6 - P2_TRACKY */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_COCKTAIL
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ STEP8(7,-1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout objlayout =
{
	32,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ STEP4(32*8*7,1), STEP4(32*8*6,1), STEP4(32*8*5,1), STEP4(32*8*4,1), STEP4(32*8*3,1), STEP4(32*8*2,1), STEP4(32*8*1,1), STEP4(32*8*0,1) },
	{ STEP16(0,8), STEP16(16*8,8) },
	32*32*2
};

static const gfx_decode marinedt_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 4 },
	{ REGION_GFX2, 0, &objlayout,  48, 4 },
	{ REGION_GFX3, 0, &objlayout,  32, 4 },
	{ -1 }
};

MACHINE_RESET( marinedt )
{
	marinedt_layer_en = 0;
}

PALETTE_INIT( marinedt )
{
	int i;
	int bit0, bit1, bit2;
	int r, g, b;

	for (i = 0; i < 64; i++)
	{
		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
//		bit2 = (color_prom[i] >> 2) & 0x01;
		r = (0x55 * bit0 + 0xaa * bit1);

		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		g = (0x55 * bit0 + 0xaa * bit1);

		/* blue component */
		bit0 = (color_prom[i] >> 5) & 0x01;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = (0x55 * bit0 + 0xaa * bit1);
		// matches yellow haired siren
		if (bit2 == 0)
			b /= 2;

		palette_set_color(i, r, g, b);
	}

	for (i = 0; i < 32; i++)
	{
		b = color_prom[i + 0x60];
		palette_set_color(64 + 31 - i, 0, 0, b);
		palette_set_color(64 + 63 - i, 0xff, 0, b);
	}
}

static MACHINE_DRIVER_START( marinedt )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 9987000/4)
	MDRV_CPU_PROGRAM_MAP(marinedt_readmem, marinedt_writemem)
	MDRV_CPU_IO_MAP(marinedt_readport, marinedt_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(marinedt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 32*8-1)
	MDRV_GFXDECODE(marinedt_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128)

	MDRV_PALETTE_INIT(marinedt)
	MDRV_VIDEO_START(marinedt)
	MDRV_VIDEO_UPDATE(marinedt)

	/* sound hardware */
	// discrete sound
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( marinedt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mg01",     0x0000, 0x0800, CRC(ad09f04d) SHA1(932fc973b4a2fbbebd7e6437ed30c8444e3d4afb) )
	ROM_LOAD( "mg02",     0x0800, 0x0800, CRC(555a2b0f) SHA1(143a8953ce5070c31dc4c1f623833b2a5a2cf657) )
	ROM_LOAD( "mg03",     0x1000, 0x0800, CRC(2abc79b3) SHA1(1afb331a2c0e320b6d026bc5cb47a53ac3356c2a) )
	ROM_LOAD( "mg04",     0x1800, 0x0800, CRC(be928364) SHA1(8d9ae71e2751c009187e41d84fbad9519ab551e1) )
	ROM_LOAD( "mg05",     0x2000, 0x0800, CRC(44cd114a) SHA1(833165c5c00c6e505acf29fef4a3ae3f9647b443) )
	ROM_LOAD( "mg06",     0x2800, 0x0800, CRC(a7e2c69b) SHA1(614fc479d13c1726382fe7b4b0379c1dd4915af0) )
	ROM_LOAD( "mg07",     0x3000, 0x0800, CRC(b85d1f9a) SHA1(4fd3e76b1816912df84477dba4655d395f5e7072) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg11",     0x0000, 0x0800, CRC(50d66dd7) SHA1(858d1d2a75e091b0e382d964c5e4ddcd8e6f07dd) )
	ROM_LOAD( "mg10",     0x0800, 0x0800, CRC(b41251e3) SHA1(e125a971b401c78efeb4b03d0fab43e392d3fc14) )
	ROM_LOAD( "mg09",     0x1000, 0x0800, CRC(f4c349ca) SHA1(077f65eeac616a778d6c42bb95677fa2892ab697) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mg12",     0x0000, 0x1000, CRC(7c6486d5) SHA1(a7f17a803937937f05fc90621883a0fd44b297a0) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mg13",     0x0000, 0x1000, CRC(17817044) SHA1(8c9b96620e3c414952e6d85c6e81b0df85c88e7a) )

	ROM_REGION( 0x0080, REGION_PROMS, ROMREGION_INVERT )
	ROM_LOAD( "mg14.bpr", 0x0000, 0x0020, CRC(f75f4e3a) SHA1(36e665987f475c57435fa8c224a2a3ce0c5e672b) )
	ROM_LOAD( "mg15.bpr", 0x0020, 0x0020, CRC(cd3ab489) SHA1(a77478fb94d0cf8f4317f89cc9579def7c294b4f) )
	ROM_LOAD( "mg16.bpr", 0x0040, 0x0020, CRC(92c868bc) SHA1(483ae6f47845ddacb701528e82bd388d7d66a0fb) )
	ROM_LOAD( "mg17.bpr", 0x0060, 0x0020, CRC(13261a02) SHA1(050edd18e4f79d19d5206f55f329340432fd4099) )
ROM_END

GAME( 1981, marinedt, 0, marinedt, marinedt, 0, ROT270, "Taito", "Marine Date", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_COLORS | GAME_NO_SOUND )
