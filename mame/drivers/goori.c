// license:BSD-3-Clause
// copyright-holders:David Haywood
/*

  Goori Goori (c) Unico 1999

 hardware is loosely derived from kaneko/snowbros.cpp / Kaneko Pandora

 ToDo:
 is more than one button used?

Goori Goori PCB layout:
+-----------------------------------------------+
|               1         29F1610_OL 29F1610_SL |
|LED VR       M6295       29F1610_OH 29F1610_SH |
| YM3012  YM2151 3.579545MHz                    |
|                                               |
|                      GAL                      |
|J                 6164   +---------+  681000   |
|A                 6164   |  Actel  |           |
|M                        |A40MX04-F|  681000   |
|M                        |  PL84   |           |
|A                        +---------+  681000   |
|                       +---------+             |
|                       |  Actel  |    681000   |
|                  2    |A40MX04-F|             |
|            3   62H256 |  PL84   |             |
| 93C46    62H256       +---------+ 76C88       |
|    S1     68000    W2465                 6116 |
|16.000MHz                                      |
+-----------------------------------------------+

   CPU: MC68HC000FN16
 Sound: OKI M6295, YM2151+YM3012 (rebadged as AD-65, BS901+BS902)
   OSC: 16.000MHz, 3.579545MHz
EEPROM: ST 93C46CB1 (set to 16bit with jumper pad)
   RAM: SEC KM681000BLG-7L (x4), HMC62H256AK-15 (x2), UM6164DK-12 (x2)
        Winbond W2465AK-15, LGS GM76C88ALK-15 & HT6116-70
 Other: GAL16V8D, LED power LED, VR volume pot, S1 Service button

ROMS:
   1 - TMS 27C020-15 - Sound samples
   2 - unknown type  - 68000 Program code EVEN bytes (jumper pad made for 2M ROMs)
   3 - unknown type  - 68000 Program code ODD bytes  (jumper pad made for 2M ROMs)
   29F1610_OL - MX 29F1610ML - PCB silkscreened OBJ 16M/L
   29F1610_OH - MX 29F1610ML - PCB silkscreened OBJ 16M/H
   29F1610_SL - MX 29F1610ML - PCB silkscreened SCR 16M/L
   29F1610_SH - MX 29F1610ML - PCB silkscreened SCR 16M/H

*/
/*
    Differences from MAME:

    Written in C, from goori.cpp. The EEPROM is the generic 93C46 with 16 bit
    cells.
*/

#include "driver.h"
#include "machine/eeprom.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

static tilemap *bg_tilemap;
static UINT16 *bg_videoram;
static UINT16 *spriteram_goori;
static int display_enable;

static WRITE16_HANDLER( goori_bg_videoram_w )
{
	COMBINE_DATA(&bg_videoram[offset]);
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
}

static void get_bg_tile_info(int tile_index)
{
	int tilenolow = (bg_videoram[(tile_index * 2) + 1] & 0xff00) >> 8;
	int tilenohigh = (bg_videoram[(tile_index * 2) + 0] & 0xff00) >> 8;

	SET_TILE_INFO(1, tilenolow | (tilenohigh << 8), 0x1f, 0)
}

static VIDEO_START( goori )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 32, 32);
	if (!bg_tilemap)
		return 1;

	return 0;
}

static VIDEO_UPDATE( goori )
{
	int i;

	fillbitmap(bitmap, 0, cliprect);

	if (!display_enable)
		return;

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	for (i = 0; i < 0x2000 / 2; i += 8)
	{
		/* 0 unused
		   1 unused
		   2 unused
		   3 colour and xpos msb
		   4 xpos
		   5 ypos
		   6 tile low
		   7 tile high (and flip) */
		int tile = spriteram_goori[i + 6] | ((spriteram_goori[i + 7] & 0x3f) << 8);
		int x = spriteram_goori[i + 4] | ((spriteram_goori[i + 3] & 1) << 8);
		int y = spriteram_goori[i + 5];
		int colour = (spriteram_goori[i + 3] & 0xf8) >> 3;
		int flipx = (spriteram_goori[i + 7] >> 7) & 1;

		if (x >= 0x3e0)
			x -= 0x400;

		drawgfx(bitmap, Machine->gfx[0], tile, colour, flipx, 0, x, y, cliprect, TRANSPARENCY_PEN, 0xff);
	}
}

static WRITE16_HANDLER( goori_ctrl_w )
{
	if (ACCESSING_MSB)
		display_enable = (data >> 12) & 1;
}

static WRITE16_HANDLER( goori_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
		EEPROM_write_bit(data & 0x04);
	}
}

static READ16_HANDLER( goori_system_r )
{
	return readinputportbytag("SYSTEM") | (EEPROM_read_bit() << 15);
}

static READ16_HANDLER( goori_ym2151_status_r )
{
	return YM2151_status_port_0_r(0) << 8;
}

static WRITE16_HANDLER( goori_ym2151_w )
{
	if (ACCESSING_MSB)
		YM2151_word_0_w(offset, data >> 8);
}

static ADDRESS_MAP_START( goori_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x300002, 0x300003) AM_READ(goori_ym2151_status_r)
	AM_RANGE(0x300000, 0x300003) AM_WRITE(goori_ym2151_w)
	AM_RANGE(0x300004, 0x300005) AM_READWRITE(OKIM6295_status_0_msb_r, OKIM6295_data_0_msb_w)
	AM_RANGE(0x300008, 0x300009) AM_WRITE(goori_ctrl_w)
	AM_RANGE(0x30000e, 0x30000f) AM_WRITE(goori_eeprom_w)
	AM_RANGE(0x400000, 0x400fff) AM_RAM AM_WRITE(goori_bg_videoram_w) AM_BASE(&bg_videoram)
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x500004, 0x500005) AM_READ(goori_system_r)
	AM_RANGE(0x600000, 0x603fff) AM_RAM AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x700000, 0x701fff) AM_RAM AM_BASE(&spriteram_goori)
	AM_RANGE(0x800000, 0x800001) AM_READWRITE(MRA16_NOP, MWA16_NOP)
ADDRESS_MAP_END

INPUT_PORTS_START( goori )
	PORT_START_TAG("DSW1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("DSW2")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x2000, IP_ACTIVE_LOW )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_SPECIAL )
INPUT_PORTS_END

static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 8,9,10,11, 0,1,2,3 },
	{ 0,4,16,20,32,36,48,52, 512+0, 512+4, 512+16, 512+20, 512+32, 512+36, 512+48, 512+52 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 1024+0*64, 1024+1*64, 1024+2*64, 1024+3*64, 1024+4*64, 1024+5*64, 1024+6*64, 1024+7*64 },
	8*8*8*4
};

static const gfx_layout layout_16x16x8_raw =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,     0x0, 0x20 },
	{ REGION_GFX2, 0, &layout_16x16x8_raw, 0x0, 0x20 },
	{ -1 }
};

static MACHINE_DRIVER_START( goori )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(goori_map, 0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(48*8, 262)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(goori)
	MDRV_VIDEO_UPDATE(goori)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.80)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.80)

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 0.40)
	MDRV_SOUND_ROUTE(1, "right", 0.40)
MACHINE_DRIVER_END

ROM_START( goori )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2", 0x000000, 0x040000, CRC(82eae7bf) SHA1(a76743f8134f1614ec7fed76f33c5ee8dfe8ab2c) )
	ROM_LOAD16_BYTE( "3", 0x000001, 0x040000, CRC(39093929) SHA1(2e13690ee4994cc5225a96ee47cfcb84cf63041f) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mx29f1610ml_obj_16m_l", 0x000000, 0x200000, CRC(f26451b9) SHA1(c6818a44115d3efed2566442295dc0b253057602) )
	ROM_LOAD( "mx29f1610ml_obj_16m_h", 0x200000, 0x200000, CRC(058ceaec) SHA1(8639d41685a6f3fb2d81b9aaf3c160666de8155d) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mx29f1610ml_scr_16m_l", 0x000000, 0x200000, CRC(8603a662) SHA1(fbe5ccb3fded60b431ffee27471158c95a8328f8) )
	ROM_LOAD( "mx29f1610ml_scr_16m_h", 0x200000, 0x200000, CRC(4223383e) SHA1(aa17eab343dad3f6eab05a844081370e3eebcd2e) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "1", 0x000000, 0x040000, CRC(c74351b9) SHA1(397f4b6aea23e6619e099c5cc99f38bae74bc3e8) )
ROM_END

GAME( 1999, goori, 0, goori, goori, 0, ROT0, "Unico", "Goori Goori", 0 )
