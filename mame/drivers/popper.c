// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

    Popper

    © 1983 Omori Electric Co. (OEC)

    PCB connector pinout

          GND   1   2  GND
          GND   3   4  GND
           5V   5   6  5V
           5V   7   8  5V
      Speaker   9  10  Speaker
          12V  11  12  12V
     1-P Push  13  14  1-P Right
     2-P Push  15  16  2-P Right
    1-P Start  17  18  1-P Left
    2-P Start  19  20  2-P Left
       Coin A  21  22  1-P Down
       Coin B  23  24  2-P Down
      Service  25  26  1-P Up
               27  28  2-P Up
               29  30
               31  32
        Synic  33  34  Red
        Green  35  36  Blue
           5V  37  38  5V
           5V  39  40  5V
          GND  41  42  GND

    TODO:
    - According to the schematics the sub CPU ROM should be 0x2000
    - Verify screen raw parameters
    - Verify graphics with real hardware
    - Watchdog

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"

extern UINT8 *popper_videoram, *popper_attribram, *popper_spriteram;
extern int popper_back_color, popper_vram_page;
PALETTE_INIT( popper );
VIDEO_START( popper );
VIDEO_UPDATE( popper );

static int popper_nmi_enable;
static void *popper_scanline_timer;

static READ8_HANDLER( popper_inputs_r )
{
	static const char *const tags[4] = { "in1", "in0", "dsw2", "dsw1" };
	int i, data = 0;
	for (i = 0; i < 4; i++)
	{
		int port = readinputportbytag(tags[i]);
		data |= ((port >> offset) & 1) << (2 * i);
		data |= ((port >> (offset + 4)) & 1) << (2 * i + 1);
	}
	return data;
}

static WRITE8_HANDLER( popper_nmi_control_w )
{
	popper_nmi_enable = data & 1;
}
static WRITE8_HANDLER( popper_crt_direction_w )
{
	flip_screen_set(data);
}
static WRITE8_HANDLER( popper_back_color_select_w )
{
	popper_back_color = data & 1;
}
static WRITE8_HANDLER( popper_vram_page_select_w )
{
	popper_vram_page = data & 1;
}

static READ8_HANDLER( popper_subcpu_nmi_r )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	return 0;
}

static READ8_HANDLER( popper_subcpu_reset_r )
{
	cpunum_set_input_line(1, INPUT_LINE_RESET, PULSE_LINE);
	return 0;
}

static READ8_HANDLER( popper_watchdog_clear_r )
{
	return 0;
}

static WRITE8_HANDLER( popper_ay1_w )
{
	if (offset == 0)
		AY8910_control_port_0_w(0, data);
	else if (offset == 1)
		AY8910_write_port_0_w(0, data);
}

static WRITE8_HANDLER( popper_ay2_w )
{
	if (offset == 0)
		AY8910_control_port_1_w(0, data);
	else if (offset == 1)
		AY8910_write_port_1_w(0, data);
}


static void popper_scanline(int scanline)
{
	int next = (scanline + 1) % 264;
	cpunum_set_input_line(0, INPUT_LINE_NMI, (popper_nmi_enable && scanline == 240) ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(1, 0, (scanline & 31) == 0 ? ASSERT_LINE : CLEAR_LINE);
	/* Keep the shared-RAM handshake synchronized at the shortest CPU quantum. */
	if (scanline == 0 || scanline == 16)
		cpu_boost_interleave(0, TIME_IN_HZ(6144000.0 / (384 * 264)));
	/* The core numbers scanlines from the first visible line. */
	timer_adjust(popper_scanline_timer, cpu_getscanlinetime((next + 264 - 16) % 264), next, 0);
}

static MACHINE_START( popper )
{
	popper_scanline_timer = timer_alloc(popper_scanline);
	state_save_register_global(popper_nmi_enable);
	state_save_register_global(popper_back_color);
	state_save_register_global(popper_vram_page);
	return 0;
}

static MACHINE_RESET( popper )
{
	popper_nmi_enable = popper_back_color = popper_vram_page = 0;
	flip_screen_set(0);
	timer_adjust(popper_scanline_timer, TIME_NOW, 16, 0);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0xc000, 0xc0ff) AM_READWRITE(MRA8_RAM, MWA8_RAM)
	AM_RANGE(0xc100, 0xc6ff) AM_READWRITE(MRA8_RAM, MWA8_RAM) AM_BASE(&popper_videoram)
	AM_RANGE(0xc700, 0xc8ff) AM_READWRITE(MRA8_RAM, MWA8_RAM)
	AM_RANGE(0xc900, 0xceff) AM_READWRITE(MRA8_RAM, MWA8_RAM) AM_BASE(&popper_attribram)
	AM_RANGE(0xcf00, 0xcfff) AM_READWRITE(MRA8_RAM, MWA8_RAM)
	AM_RANGE(0xd000, 0xd7ff) AM_READWRITE(MRA8_RAM, MWA8_RAM) AM_BASE(&popper_spriteram)
	AM_RANGE(0xd800, 0xdfff) AM_READWRITE(MRA8_RAM, MWA8_RAM) AM_SHARE(1)
	AM_RANGE(0xe000, 0xe003) AM_MIRROR(0x03fc) AM_READ(popper_inputs_r)
	AM_RANGE(0xe000, 0xe000) AM_MIRROR(0x1ff8) AM_WRITE(popper_nmi_control_w)
	AM_RANGE(0xe001, 0xe001) AM_MIRROR(0x1ff8) AM_WRITE(popper_crt_direction_w)
	AM_RANGE(0xe002, 0xe002) AM_MIRROR(0x1ff8) AM_WRITE(popper_back_color_select_w)
	AM_RANGE(0xe003, 0xe003) AM_MIRROR(0x1ff8) AM_WRITE(popper_vram_page_select_w)
	AM_RANGE(0xe004, 0xe007) AM_MIRROR(0x1ff8) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xe400, 0xe400) AM_MIRROR(0x03ff) AM_READ(popper_subcpu_nmi_r)
	AM_RANGE(0xe800, 0xf7ff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0xf800, 0xf800) AM_MIRROR(0x03ff) AM_READ(popper_subcpu_reset_r)
	AM_RANGE(0xfc00, 0xfc00) AM_MIRROR(0x03ff) AM_READ(popper_watchdog_clear_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(MRA8_ROM, MWA8_ROM)
	AM_RANGE(0x2000, 0x7fff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
	AM_RANGE(0x8000, 0x8003) AM_MIRROR(0x1ffc) AM_WRITE(popper_ay1_w)
	AM_RANGE(0xa000, 0xa003) AM_MIRROR(0x1ffc) AM_WRITE(popper_ay2_w)
	AM_RANGE(0xc000, 0xc7ff) AM_MIRROR(0x1800) AM_READWRITE(MRA8_RAM, MWA8_RAM) AM_SHARE(1)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(MRA8_NOP, MWA8_NOP)
ADDRESS_MAP_END


INPUT_PORTS_START( popper )
	PORT_START_TAG("in0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_COIN1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)                  PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_COIN2)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)   PORT_COCKTAIL  PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_SERVICE1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)                    PORT_8WAY
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)     PORT_COCKTAIL  PORT_8WAY

	PORT_START_TAG("in1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)                 PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1)         PORT_COCKTAIL
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)  PORT_COCKTAIL  PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_START1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)                  PORT_8WAY
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_START2)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)   PORT_COCKTAIL  PORT_8WAY

	PORT_START_TAG("dsw1")
	PORT_DIPNAME(0x03, 0x00, DEF_STR( Coin_A ))      PORT_DIPLOCATION("DSW1:1,2")
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(   0x02, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(   0x01, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(   0x03, DEF_STR( 1C_2C ))
	PORT_DIPNAME(0x0c, 0x04, DEF_STR( Coin_B ))      PORT_DIPLOCATION("DSW1:3,4")
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(   0x08, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(   0x04, DEF_STR( 1C_5C ))
	PORT_DIPSETTING(   0x0c, DEF_STR( 1C_6C ))
	PORT_DIPNAME(0x30, 0x20, DEF_STR( Lives ))       PORT_DIPLOCATION("DSW1:5,6")
	PORT_DIPSETTING(   0x00, "2")
	PORT_DIPSETTING(   0x20, "3")
	PORT_DIPSETTING(   0x10, "4")
	PORT_DIPSETTING(   0x30, "5")
	PORT_DIPNAME(0xc0, 0x00, DEF_STR( Bonus_Life ))  PORT_DIPLOCATION("DSW1:7,8")
	PORT_DIPSETTING(   0x00, "20,000 Points")
	PORT_DIPSETTING(   0x80, "30,000 Points")
	PORT_DIPSETTING(   0x40, "40,000 Points")
	PORT_DIPSETTING(   0xc0, "50,000 Points")

	PORT_START_TAG("dsw2")
	PORT_DIPNAME(0x01, 0x01, DEF_STR( Demo_Sounds )) PORT_DIPLOCATION("DSW2:1")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x01, DEF_STR( On ))
	PORT_DIPNAME(0x02, 0x02, "Unused") PORT_DIPLOCATION("DSW2:2")
	PORT_DIPSETTING(0x02, DEF_STR( Off ))
	PORT_DIPSETTING(0x00, DEF_STR( On ))
	PORT_DIPNAME(0x04, 0x00, DEF_STR( Free_Play ))   PORT_DIPLOCATION("DSW2:3")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x04, DEF_STR( On ))
	PORT_DIPNAME(0x08, 0x00, "Invulnerability")      PORT_DIPLOCATION("DSW2:4")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x08, DEF_STR( On ))
	PORT_DIPNAME(0x10, 0x00, DEF_STR( Flip_Screen )) PORT_DIPLOCATION("DSW2:5")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x10, DEF_STR( On ))
	PORT_DIPNAME(0x20, 0x20, DEF_STR( Cabinet ))     PORT_DIPLOCATION("DSW2:6")
	PORT_DIPSETTING(   0x00, DEF_STR( Cocktail ))
	PORT_DIPSETTING(   0x20, DEF_STR( Upright ))
	PORT_DIPNAME(0x40, 0x00, "Game Repeating")       PORT_DIPLOCATION("DSW2:7")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x40, DEF_STR( On ))
	PORT_DIPNAME(0x80, 0x00, DEF_STR( Pause ))       PORT_DIPLOCATION("DSW2:8")
	PORT_DIPSETTING(   0x00, DEF_STR( Off ))
	PORT_DIPSETTING(   0x80, DEF_STR( On ))
INPUT_PORTS_END

static const gfx_layout layer0_charlayout =
{
	8,8,
	RGN_FRAC(2,2),
	1,
	{ 0, 4 },
	{ STEP4(8,1), STEP4(0,1) },
	{ STEP8(0,16) },
	16*8
};

static const gfx_layout layer1_charlayout =
{
	8,8,
	RGN_FRAC(2,2),
	2,
	{ 0, 4 },
	{ STEP4(8,1), STEP4(0,1) },
	{ STEP8(0,16) },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ STEP8(8,1), STEP8(0,1) },
	{ STEP16(0,16) },
	16*2*8
};

static const gfx_decode popper_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layer0_charlayout, 0, 32 },
	{ REGION_GFX1, 0, &layer1_charlayout, 0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 0, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( popper )
	MDRV_CPU_ADD(Z80, 18432000/3/2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_ADD(Z80, 18432000/3/2)
	MDRV_CPU_PROGRAM_MAP(sub_map, 0)
	MDRV_MACHINE_START(popper)
	MDRV_MACHINE_RESET(popper)
	MDRV_FRAMES_PER_SECOND(6144000.0 / (384 * 264))
	MDRV_VBLANK_DURATION(2500)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 264)
	MDRV_VISIBLE_AREA(48, 327, 16, 239)
	MDRV_GFXDECODE(popper_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_PALETTE_INIT(popper)
	MDRV_VIDEO_START(popper)
	MDRV_VIDEO_UPDATE(popper)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 18432000/3/2/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 18432000/3/2/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( popper )
	ROM_REGION(0x8000, REGION_CPU1, 0)
	ROM_LOAD("p1", 0x0000, 0x2000, CRC(56881b70) SHA1(d3ade7a54a6cb8a0babf0d667a6b27f492a739dc))
	ROM_LOAD("p2", 0x2000, 0x2000, CRC(a054d9d2) SHA1(fcd86e7247b40cf07ea595a64c104b99b0e93ced))
	ROM_LOAD("p3", 0x4000, 0x2000, CRC(6201928a) SHA1(53b571b9f2c0568f10cd974641863c2e00777b46))

	ROM_REGION(0x2000, REGION_CPU2, 0)
	ROM_LOAD("p0", 0x0000, 0x1000, CRC(ef5f7c5b) SHA1(c63a3d9ef2868ad7eaacddec810d62d2e124dc15))

	ROM_REGION(0x2000, REGION_GFX1, ROMREGION_DISPOSE)
	ROM_LOAD("p4", 0x0000, 0x2000, CRC(86203349) SHA1(cce2dd3fa786c2fb3ca80e7b93adf94db3b46b01))

	ROM_REGION(0x4000, REGION_GFX2, ROMREGION_DISPOSE)
	ROM_LOAD("p5", 0x0000, 0x2000, CRC(a21ac194) SHA1(2c0e3df8981a12d383b1c4619a0b95a7c2d176a7))
	ROM_LOAD("p6", 0x2000, 0x2000, CRC(d99fa790) SHA1(201271ee4fb812236a38cb5f9070ac29e8186097))

	ROM_REGION(0x40, REGION_PROMS, 0)
	ROM_LOAD("p.m3", 0x00, 0x20, CRC(713217aa) SHA1(6083c3432bf94c9e983fcc79171529f519c86105))
	ROM_LOAD("p.m4", 0x20, 0x20, CRC(384de5c1) SHA1(892c89a01c11671c5708113b4e4c27b84be37ea6))
ROM_END

GAME( 1983, popper, 0, popper, popper, 0, ROT90, "Omori", "Popper", 0 )
