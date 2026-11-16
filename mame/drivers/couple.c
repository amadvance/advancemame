// license:BSD-3-Clause
// copyright-holders:David Haywood
/*******************************************************************************************

The Couples (c) 1986/1988 The Couples (?)

Preliminary driver by Angelo Salese

This seems the second revision of the Merit.c HW,with the rom banking,the backup ram &
some sort of protection/funky ram banking which involves a program uploaded into
the backup ram itself.

TODO:
-Fix graphics bugs;
-Support the C.R.T.C 6845 features;
-Inputs are sometimes buggy;
-Colors;
-Merge with the Merit.c driver;
-Uncertain Manufacturer & Year of production,is it Merit,it is an hack/bootleg running
on that board or Merit got bankrupt and can't use his trademark?
- update 2006018 by f205v: in-game grafics clearly displays Merit simbol, for this reason
I changed manufacturer to "Merit"

********************************************************************************************

Main CPU : Z80B CPU

Sound : AY3-8910 + 2 x Nec D8255AC-2

Other : MC6845P

Battery backup ram (6264)

Provided to you by Thierry (ShinobiZ) & Gerald (COY)

*******************************************************************************************/

/*
    Differences from MAME:

    In MAME this driver was merged into merit.cpp (Merit trivia games, by
    Pierpaolo Prazzoli), where these sets are bootlegs of Match'em Up, the
    clones of the new parent matchem. Here the original couple.c driver is
    kept, aligned to the MAME sets, names, inputs and hardware description:

    - the 2K ROM at 0xb000 is the CRT-209 module code (a plain 2716 on the
      bootlegs) and not a color PROM. It is mapped as ROM over the backup RAM
      and descrambled at start-up (address and data lines swapped, as the
      init_crt209 of MAME). The backup RAM hack of the old driver is removed.
    - the colors come from the 4 bit palette RAM at 0xf800, addressed by the
      attribute and the tile pixel, like in MAME, with the same 16 pens.
    - the CRTC is not emulated: the screen is a fixed 64x32 tiles of 8x8
      pixels, redrawn every frame (the palette RAM is not updated per scanline).
    - not emulated: the flip screen, the player lamps, the coin counter of the
      AY8910 port A and the display blanking color of the PPI port C bit 3.
*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "vidhrdw/crtc6845.h"

#define NUM_PENS 16

static UINT8 *vram_attr, *vram_video;
static UINT8 palette_ram[0x400];
static UINT16 extra_video_bank_bit;

/*
Attribute RAM (0xe800)
x--- ---- extra tile number bit
-x-- ---- gfx region (0 = 3 bit planes, 1 = single bit plane)
--xx xxxx ---- palette group (with bit 6, 128 groups of 8 pens in the palette RAM)
*/

VIDEO_START( couple )
{
	memset(palette_ram, 0, sizeof(palette_ram));
	return 0;
}

VIDEO_UPDATE( couple )
{
	const UINT8 *gfx1 = memory_region(REGION_GFX1);
	const UINT8 *gfx2 = memory_region(REGION_GFX2);
	int rlen = memory_region_length(REGION_GFX2);
	int tile, ra, i;

	for (tile = 0; tile < 64*32; ++tile)
	{
		int attr = vram_attr[tile];
		int region = (attr >> 6) & 1;
		int colour = (attr & 0x7f) << 3;
		int code = vram_video[tile] | ((attr & 0x80) << 1) | extra_video_bank_bit;
		int x = (tile & 63) * 8;
		int y = (tile >> 6) * 8;

		for (ra = 0; ra < 8; ++ra)
		{
			int addr = ((code << 4) | ra) & (rlen - 1);
			int p0, p1, p2;

			if (region)
			{
				p0 = gfx2[addr];
				p1 = p2 = 0xff;
			}
			else
			{
				p0 = gfx1[addr];
				p1 = gfx1[rlen | addr];
				p2 = gfx1[(rlen << 1) | addr];
			}

			for (i = 7; i >= 0; --i)
			{
				int col = colour | (((p0 >> i) & 1) << 2) | (((p1 >> i) & 1) << 1) | ((p2 >> i) & 1);
				plot_pixel(bitmap, x + 7 - i, y + ra, Machine->pens[palette_ram[col] & (NUM_PENS - 1)]);
			}
		}
	}
}

static WRITE8_HANDLER( couple_vram_attr_w )
{
	vram_attr[offset] = data;
}

static WRITE8_HANDLER( couple_vram_video_w )
{
	vram_video[offset] = data;
}

static READ8_HANDLER( couple_palette_r )
{
	return palette_ram[((vram_attr[offset] & 0x7f) << 3) | (offset & 0x07)];
}

static WRITE8_HANDLER( couple_palette_w )
{
	palette_ram[((vram_attr[offset] & 0x7f) << 3) | (offset & 0x07)] = data & 0x0f;
}

/* bit 7 is a random bit */
static READ8_HANDLER( couple_in2_r )
{
	return (input_port_3_r(offset) & 0x7f) | (mame_rand() & 0x80);
}

static WRITE8_HANDLER( couple_misc_w )
{
	extra_video_bank_bit = (((data & 1) << 1) | ((data >> 1) & 1)) << 9;
}

static WRITE8_HANDLER( couple_coin_w )
{
	coin_counter_w(0, ~data & 0x80);
}

static struct AY8910interface ay8910_interface =
{
	0,
	0,
	couple_coin_w,
	0
};

static ppi8255_interface ppi8255_intf =
{
	2,
	{ input_port_0_r,   input_port_1_r },	/* Port A read */
	{ input_port_2_r,   NULL           },	/* Port B read */
	{ couple_in2_r,     NULL           },	/* Port C read */
	{ NULL,   			NULL           },	/* Port A write */
	{ NULL,   			NULL           },	/* Port B write */
	{ NULL,   			couple_misc_w  },	/* Port C write */
};

static ADDRESS_MAP_START( merit_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0x9fff ) AM_ROMBANK(1)
	/* the first matching range wins: the CRT-209 module code overlays the backup RAM */
	AM_RANGE( 0xb000, 0xb7ff ) AM_ROM
	AM_RANGE( 0xa000, 0xbfff ) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE( 0xc004, 0xc007 ) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE( 0xc008, 0xc00b ) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE( 0xe000, 0xe000 ) AM_WRITE(crtc6845_address_w)
	AM_RANGE( 0xe001, 0xe001 ) AM_WRITE(crtc6845_register_w)
	AM_RANGE( 0xe800, 0xefff ) AM_READWRITE(MRA8_RAM, couple_vram_attr_w) AM_BASE(&vram_attr)
	AM_RANGE( 0xf000, 0xf7ff ) AM_READWRITE(MRA8_RAM, couple_vram_video_w) AM_BASE(&vram_video)
	AM_RANGE( 0xf800, 0xfbff ) AM_READWRITE(couple_palette_r, couple_palette_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( merit_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xc00c, 0xc00c) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc10c, 0xc10c) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END

static MACHINE_RESET( couple )
{
	ppi8255_init(&ppi8255_intf);
	extra_video_bank_bit = 0;
}

static PALETTE_INIT( couple )
{
	int i;

	for (i = 0; i < NUM_PENS; ++i)
	{
		int dim = (i & 8) ? 255 : 127;

		palette_set_color(i, dim * (i & 1), dim * ((i >> 1) & 1), dim * ((i >> 2) & 1));
	}
}

/* both players use the same controls. Start a 1 player game with button 1 or a 2 player game with button 2. */
#define PORT_IN0 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define PORT_DSW_ATTEMPTS \
	PORT_DIPNAME( 0x01, 0x00, "Number of Attempts" ) \
	PORT_DIPSETTING(    0x01, "99" ) \
	PORT_DIPSETTING(    0x00, "9" ) \
	PORT_DIPNAME( 0x02, 0x02, "Tries Per Coin" ) \
	PORT_DIPSETTING(    0x02, "3" ) \
	PORT_DIPSETTING(    0x00, "2" )

#define PORT_DSW_COUPLE_COINAGE \
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) ) \
	/* 2 Coins for 2 Credits? I think this is an invalid setting, it doesn't even work correctly */ \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_2C ) )

#define PORT_DSW_DIFFICULTY \
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )

#define PORT_DSW_SOUND \
	PORT_DIPNAME( 0x20, 0x00, "Sound" ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define PORT_DSW_UNUSED_40 \
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define PORT_DSW_UNUSED_80 \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define PORT_DSW_CLEAR_RAM \
	/* Service Mode shows this as "NOT USED" */ \
	PORT_DIPNAME( 0x80, 0x80, "Clear RAM" ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define PORT_DSW_MATCHEM \
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_5C ) ) /* shows "5 PLAYS FOR 4 COINS" */ \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) /* shows "4 PLAYS FOR 4 COINS" */ \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define PORT_IN1 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME( "Bookkeeping" ) /* IPT_GAMBLE_BOOK in MAME */ \
	PORT_SERVICE_NO_TOGGLE( 0x08, IP_ACTIVE_LOW ) \
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/* bit 7 is a random bit, see couple_in2_r() */
#define PORT_IN2 \
	PORT_START \
	PORT_DIPNAME( 0x01, 0x01, "3" ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_START( couple )
	PORT_IN0

	PORT_START
	PORT_DSW_ATTEMPTS
	PORT_DSW_COUPLE_COINAGE
	PORT_DSW_DIFFICULTY
	PORT_DSW_SOUND
	PORT_DSW_UNUSED_40
	PORT_DSW_CLEAR_RAM

	PORT_IN1
	PORT_IN2
INPUT_PORTS_END

/* Different DSWs */
INPUT_PORTS_START( couplep )
	PORT_IN0

	PORT_START
	PORT_DSW_ATTEMPTS
	PORT_DSW_COUPLE_COINAGE
	PORT_DSW_DIFFICULTY
	PORT_DSW_SOUND
	PORT_DIPNAME( 0x40, 0x40, "Bonus Play" )
	PORT_DIPSETTING(    0x40, "at 150.000" )
	PORT_DIPSETTING(    0x00, "at 200.000" )
	PORT_DSW_UNUSED_80

	PORT_IN1
	PORT_IN2
INPUT_PORTS_END

INPUT_PORTS_START( matchem )
	PORT_IN0

	PORT_START
	PORT_DSW_ATTEMPTS
	PORT_DSW_MATCHEM
	PORT_DSW_DIFFICULTY
	PORT_DSW_UNUSED_40
	PORT_DSW_UNUSED_80

	PORT_IN1
	PORT_IN2
INPUT_PORTS_END

static MACHINE_DRIVER_START( couple )
	MDRV_CPU_ADD(Z80,10000000/4)
	MDRV_CPU_PROGRAM_MAP(merit_mem,0)
	MDRV_CPU_IO_MAP(merit_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(couple)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(NUM_PENS)
	MDRV_PALETTE_INIT(couple)

	MDRV_VIDEO_START(couple)
	MDRV_VIDEO_UPDATE(couple)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 10000000/8)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( matchem )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "6221-51_u5-1.u5", 0x00000, 0x8000, CRC(e26a6562) SHA1(f10c5bbfb175c807f16beab8eb453cbf5fbaf2d2) ) /* 6221-51 U5-1 U6-1 01/15/86 */
	ROM_LOAD( "6221-51_u6-1.u6", 0x14000, 0x2000, CRC(35c4229a) SHA1(091c12e4bd4a6cde8323e208a9b24bbc2d1bc8c2) ) /* MBM27C64 */
	ROM_LOAD( "crt-209_6221-51.cpu", 0x0b000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) ) /* contains Z80 program code to read inputs */

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "sex1_u39.u39", 0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "sex1_u38.u38", 0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "sex1_u37.u37", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "sex1_u40.u40", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )

	ROM_REGION( 0x320, REGION_USER2, 0 )
	ROM_LOAD( "dec003.u13",          0x000, 0x117, CRC(5b9a2fec) SHA1(c56c7bbe13028903cfc82440ee8b24df855134c2) ) /* PAL16L8ANC - brute forced */
	ROM_LOAD( "crt-209_pal16l8.bin", 0x200, 0x117, CRC(e916c56f) SHA1(1517091ff1791d923e5bd62d18d1428b6a3a8c72) ) /* SC3339 20-pin 16L8 type PAL (inside CRT-209 module) */
ROM_END

ROM_START( couple ) /* PCB is marked: "230188", bootleg of Match'em Up (6221-51 U5-0) */
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "1.1d",  0x00000, 0x8000, CRC(bc70337a) SHA1(ffc484bc3965f0780d3fa5d8801af27a7164a417) )
	ROM_LOAD( "2.1e",   0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )
	ROM_LOAD( "7.7a",  0x0b000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) ) /* same use as the CRT-209 modules, just a standard 2716 EPROM */

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "3.9c",  0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "4.9d",  0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "5.10c", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "6.10d", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )
ROM_END

/*f205v's dump,same except for the first z80 rom,first noticeable differences are that
it doesn't jump to the backup ram area and it gives an extra play if you reach a certain
amount of points (there is a dip switch to select the trigger: 150.000 or 200.000*/
ROM_START( couplep ) /* PCB is marked: "230188", bootleg of Match'em Up (6221-51 U5-0) */
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "p_1.1d", 0x00000, 0x8000, CRC(4601ace6) SHA1(a824ceebf8b9ce77ef2c8e92636e4261f2ae0420) ) /* doesn't jump to the backup RAM area */
	ROM_LOAD( "2.1e",   0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )
	ROM_LOAD( "7.7a",  0x0b000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) ) /* same use as the CRT-209 modules, just a standard 2716 EPROM */

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "3.9c",  0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "4.9d",  0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "5.10c", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "6.10d", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )
ROM_END

ROM_START( couplei ) /* PCB is marked: "230188", bootleg of Match'em Up (6221-51 U5-0) */
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "i_1.1d", 0x00000, 0x8000, CRC(760fa29e) SHA1(a37a1562028d9615adff3d2ef88e0156354c720a) ) /* looks like an intermediate release between set1 and set2 */
	ROM_LOAD( "2.1e",   0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )
	ROM_LOAD( "7.7a",  0x0b000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) ) /* same use as the CRT-209 modules, just a standard 2716 EPROM */

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "3.9c",  0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "4.9d",  0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "5.10c", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "6.10d", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )
ROM_END

static DRIVER_INIT( couple )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	/* the banked rom isn't a *real* banking, it's just a strange rom hook-up: it is always
	   the same 8K, at 0x14000 (2.1e of the bootlegs, u6 of Match'em Up) */
	memory_set_bankptr(1, ROM + 0x14000);

	/* the CRT-209 code is stored with scrambled address and data lines, also in the
	   standard 2716 of the bootlegs */
	{
		UINT8 buffer[0x800];
		UINT8 *crt = ROM + 0xb000;
		int i;

		memcpy(buffer, crt, 0x800);
		for (i = 0; i < 0x800; ++i)
		{
#define B(v, n) (((v) >> (n)) & 1)
			int a = (i & ~0xff) | (B(i,1) << 7) | (B(i,0) << 6) | (B(i,2) << 5) | (B(i,3) << 4)
				| (B(i,4) << 3) | (B(i,5) << 2) | (B(i,6) << 1) | B(i,7);
			UINT8 d = buffer[a];

			crt[i] = (B(d,2) << 7) | (B(d,6) << 6) | (B(d,5) << 5) | (B(d,3) << 4)
				| (B(d,4) << 3) | (B(d,7) << 2) | (B(d,0) << 1) | B(d,1);
#undef B
		}
	}
}

/* Year is uncertain:
title screen says "1988"
PCB marking (set 2) and (set 3) says "230188"
service mode says "6221-51 U5-0 12/02/86" */

GAME( 1986, matchem, 0,       couple, matchem, couple, ROT0, "Merit",   "Match'em Up (6221-51, U5-1)", 0 )
GAME( 1988, couple,  matchem, couple, couple,  couple, ROT0, "bootleg", "The Couples (set 1)", 0 )
GAME( 1988, couplep, matchem, couple, couplep, couple, ROT0, "bootleg", "The Couples (set 2)", 0 )
GAME( 1988, couplei, matchem, couple, couple,  couple, ROT0, "bootleg", "The Couples (set 3)", 0 )
