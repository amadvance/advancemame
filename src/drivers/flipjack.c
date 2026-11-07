// license:BSD-3-Clause
// copyright-holders:Angelo Salese, hap, AJR
/***************************************************************************

Flipper Jack, by Jackson, 1983

xtal: 16mhz, 6mhz
cpu: 2*z80
sound: 2*ay8910
other: 8255 ppi, hd6845 crtc, 1 dipsw

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "vidhrdw/crtc6845.h"

static UINT8 *flipjack_cram, *flipjack_vram, *flipjack_fbram;
static UINT8 flipjack_bank, flipjack_layer, flipjack_coin;

static PALETTE_INIT( flipjack )
{
	int i;
	for (i = 0; i < 64; ++i)
	{
		palette_set_color(2*i+1, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i));
		palette_set_color(2*i, pal1bit(color_prom[i] >> 1), pal1bit(color_prom[i] >> 2), pal1bit(color_prom[i]));
	}
	for (i = 0; i < 8; ++i)
		palette_set_color(128+i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i));
}

static VIDEO_UPDATE( flipjack )
{
	const UINT8 *tiles = memory_region(REGION_GFX1);
	const UINT8 *field = memory_region(REGION_GFX2);
	int y, x, xi;
	int flip = !(flipjack_layer & 1);
	int height = crtc6845_max_ras_addr + 1;
	fillbitmap(bitmap, get_black_pen(), cliprect);
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		int ma = crtc6845_start_addr + (y / height) * crtc6845_horiz_disp;
		int ra = y % height;
		int base = (((ma & 0x3e0) << 3) | ((ra & 7) << 5)) ^ (flip ? 0x1fff : 0);
		for (x = 0; x < crtc6845_horiz_disp && x < 32; ++x)
		{
			int a = (base + (flip ? -x : x)) & 0x1fff;
			int t = a & 0x1f1f;
			int tile = tiles[((flipjack_bank & 3) << 11) | (flipjack_vram[t] << 3) | ((flip ? ~ra : ra) & 7)];
			for (xi = 0; xi < 8; ++xi)
			{
				int bit = flip ? xi : 7-xi;
				int pen = 0;
				int px = x*8+xi;
				if (flipjack_layer & 2)
					pen = 128 + (((field[a] >> bit) & 1) | (((field[a+0x2000] >> bit) & 1) << 1) | (((field[a+0x4000] >> bit) & 1) << 2));
				if ((tile >> bit) & 1)
					pen = (flipjack_cram[t] & 63)*2+1;
				if ((flipjack_layer & 4) && ((flipjack_fbram[a] >> bit) & 1))
					pen = 135;
				if (px >= cliprect->min_x && px <= cliprect->max_x)
					plot_pixel(bitmap, px, y, Machine->pens[pen]);
			}
		}
	}
}

static WRITE8_HANDLER( flipjack_bank_w )
{
	flipjack_bank = data;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000 + ((data & 4) << 11));
}

static WRITE8_HANDLER( flipjack_layer_w ) { flipjack_layer = data; }

static WRITE8_HANDLER( flipjack_sound_w )
{
	soundlatch_w(0, data);
	if (data & 0x80)
		cpunum_set_input_line(1, 0, ASSERT_LINE);
}

static WRITE8_HANDLER( flipjack_sound_ack_w )
{
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}

static INTERRUPT_GEN( flipjack_vblank )
{
	UINT8 coin = readinputport(0) & 1;
	if (!coin && flipjack_coin)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	flipjack_coin = coin;
	cpunum_set_input_line(0, 0, HOLD_LINE);
	/* The audio CPU releases its latched frame interrupt through port zero. */
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
}

static const ppi8255_interface flipjack_ppi =
{
	1,
	{ input_port_1_r }, { input_port_2_r }, { input_port_3_r },
	{ 0 }, { 0 }, { 0 }
};

static MACHINE_RESET( flipjack )
{
	ppi8255_init(&flipjack_ppi);
	flipjack_bank_w(0, 0);
	flipjack_layer = 0;
	flipjack_coin = 1;
	crtc6845_address_w(0, 12);
	crtc6845_register_w(0, 0);
	crtc6845_address_w(0, 13);
	crtc6845_register_w(0, 0);
	cpunum_set_input_line(1, 0, CLEAR_LINE);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}

static ADDRESS_MAP_START( flipjack_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x4000, 0x67ff) AM_RAM
	AM_RANGE(0x6800, 0x6803) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(flipjack_sound_w)
	AM_RANGE(0x7010, 0x7010) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x7011, 0x7011) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x7020, 0x7020) AM_READ(input_port_4_r)
	AM_RANGE(0x7800, 0x7800) AM_WRITE(flipjack_layer_w)
	AM_RANGE(0x8000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(&flipjack_cram)
	AM_RANGE(0xc000, 0xdfff) AM_RAM AM_BASE(&flipjack_vram)
	AM_RANGE(0xe000, 0xffff) AM_RAM AM_BASE(&flipjack_fbram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( flipjack_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xff, 0xff) AM_WRITE(flipjack_bank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( flipjack_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x27ff) AM_RAM
	AM_RANGE(0x4000, 0x4000) AM_READWRITE(AY8910_read_port_1_r, AY8910_write_port_1_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8000, 0x8000) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( flipjack_sound_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(flipjack_sound_ack_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( flipjack )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Shoot")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Left Flipper")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("P1 Tilt")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Right Flipper")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL PORT_NAME("P2 Shoot")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL PORT_NAME("P2 Left Flipper")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_COCKTAIL PORT_NAME("P2 Tilt")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL PORT_NAME("P2 Right Flipper")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "Drop Target" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x70, "150K & Every 70K" )
	PORT_DIPSETTING(    0x60, "150K & Every 100K" )
	PORT_DIPSETTING(    0x50, "200K & Every 70K" )
	PORT_DIPSETTING(    0x40, "200K & Every 100K" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )
INPUT_PORTS_END

static const struct AY8910interface flipjack_ay = { soundlatch_r, 0, 0, 0 };

static MACHINE_DRIVER_START( flipjack )
	MDRV_CPU_ADD(Z80, 16000000/4)
	MDRV_CPU_PROGRAM_MAP(flipjack_map, 0)
	MDRV_CPU_IO_MAP(flipjack_io, 0)
	MDRV_CPU_VBLANK_INT(flipjack_vblank, 1)
	MDRV_CPU_ADD(Z80, 16000000/4)
	MDRV_CPU_PROGRAM_MAP(flipjack_sound_map, 0)
	MDRV_CPU_IO_MAP(flipjack_sound_io, 0)
	MDRV_FRAMES_PER_SECOND(6000000.0 / (392 * 256))
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(flipjack)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 191)
	MDRV_PALETTE_LENGTH(136)
	MDRV_PALETTE_INIT(flipjack)
	MDRV_VIDEO_UPDATE(flipjack)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 16000000/8)
	MDRV_SOUND_CONFIG(flipjack_ay)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD(AY8910, 16000000/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( flipjack )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "3.d5", 0x00000, 0x2000, CRC(123bd992) SHA1(d845e2b9af5b81d950e5edf35201f1dd1c4af651) )
	ROM_LOAD( "4.f5", 0x08000, 0x2000, CRC(d27e0184) SHA1(f108993fc3fce9173a4961a76fc60655fdd1cd25) )
	ROM_LOAD( "1.l5", 0x10000, 0x2000, CRC(4632263b) SHA1(b1fbb851ffd8aff36aff6f36672122fef3dd0af1) )
	ROM_LOAD( "2.m5", 0x12000, 0x2000, CRC(e2bdce13) SHA1(50d990095a35837570b3117763e990440d8656ae) )

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD( "s.s5",  0x0000, 0x2000, CRC(34515a7b) SHA1(affe34198b77bddd314fae2851fd6a29d80f734e) )

	ROM_REGION( 0x2000, REGION_GFX1, 0 )
	ROM_LOAD( "cg.l6", 0x0000, 0x2000, CRC(8d87f6b9) SHA1(55ca726f190eac9ee7e26b8f4e519f1634bec0dd) )

	ROM_REGION( 0x6000, REGION_GFX2, 0 )
	ROM_LOAD( "b.h6",  0x0000, 0x2000, CRC(bbc8fdcc) SHA1(93758ca13cc49b87508f01c86c652155945dd484) )
	ROM_LOAD( "r.f6",  0x2000, 0x2000, CRC(8c02fe71) SHA1(148e7382dc9b7678c447ada5ad19e03a3a051a7f) )
	ROM_LOAD( "g.d6",  0x4000, 0x2000, CRC(8624d07f) SHA1(fb51c9c785d56854a6530b71868e95ad6be7cbee) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "m3-7611-5.f8", 0x0000, 0x0100, CRC(f0248102) SHA1(22d87935c941e2e8bba5427599f6fd5fa1262ebc) )
ROM_END

GAME( 1983?, flipjack, 0, flipjack, flipjack, 0, ROT270, "Jackson Co., Ltd.", "Flipper Jack", GAME_IMPERFECT_COLORS )
