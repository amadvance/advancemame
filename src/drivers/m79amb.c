// license:GPL-2.0+
// copyright-holders:Juergen Buchmueller,Al Kossow
/* Ramtek M-79 Ambush */

#include "driver.h"
#include "m79amb.h"

static UINT8 gun_lut[2][256];

static READ8_HANDLER( gray5bit_controller0_r )
{
	return (input_port_2_r(0) & 0xe0) | gun_lut[0][input_port_4_r(0)];
}

static READ8_HANDLER( gray5bit_controller1_r )
{
	return (input_port_3_r(0) & 0xe0) | gun_lut[1][input_port_5_r(0)];
}

static WRITE8_HANDLER( m79amb_sound0_w )
{
	discrete_sound_w(M79AMB_SHOT_EN,data & 1);
	discrete_sound_w(M79AMB_BOOM_EN,data & 2);
	discrete_sound_w(M79AMB_THUD_EN,data & 4);
}

static WRITE8_HANDLER( m79amb_sound3_w )
{
	set_led_status(0,data & 1);
	discrete_sound_w(M79AMB_MC_REV_EN,data & 2);
	discrete_sound_w(M79AMB_MC_CONTROL_EN,data & 4);
	discrete_sound_w(M79AMB_TANK_TRUCK_JEEP_EN,data & 8);
	discrete_sound_w(M79AMB_WHISTLE_B_EN,data & 0x10);
	discrete_sound_w(M79AMB_WHISTLE_A_EN,data & 0x20);
}

static WRITE8_HANDLER( m79amb_lamp_w )
{
	set_led_status(1,data != 0);
}

static ADDRESS_MAP_START( m79amb_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x5fff) AM_RAM AM_WRITE(ramtek_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x6000, 0x63ff) AM_RAM
	AM_RANGE(0x8000, 0x8000) AM_READWRITE(input_port_0_r,m79amb_sound0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(ramtek_mask_w)
	AM_RANGE(0x8002, 0x8002) AM_READWRITE(input_port_1_r,m79amb_lamp_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(m79amb_sound3_w)
	AM_RANGE(0x8004, 0x8004) AM_READ(gray5bit_controller0_r)
	AM_RANGE(0x8005, 0x8005) AM_READ(gray5bit_controller1_r)
	AM_RANGE(0xc000, 0xc07f) AM_RAM
	AM_RANGE(0xc200, 0xc27f) AM_RAM
ADDRESS_MAP_END

static INPUT_PORTS_START( m79amb )
	PORT_START_TAG("8000")
	PORT_DIPNAME( 0x03, 0x00, "Game Time")
	PORT_DIPSETTING(    0x00, "60 Seconds" )
	PORT_DIPSETTING(    0x01, "90 Seconds" )
	PORT_DIPSETTING(    0x02, "90 Seconds" )
	PORT_DIPSETTING(    0x03, "120 Seconds" )
	PORT_DIPNAME( 0x1c, 0x04, "Points for Extended Time" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x04, "2500" )
	PORT_DIPSETTING(    0x08, "3500" )
	PORT_DIPSETTING(    0x0c, "5000" )
	PORT_DIPSETTING(    0x1c, "No Extended Time" )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x40, "1 Coin/2 Players")
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ))

	PORT_START_TAG("8002")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("8004")
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("8005")
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_START_TAG("GUN1")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(19,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_PLAYER(1)

	PORT_START_TAG("GUN2")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0,234) PORT_SENSITIVITY(50) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_PLAYER(2)
INPUT_PORTS_END


static INTERRUPT_GEN( m79amb_interrupt )
{
	cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0xcf);
}

static PALETTE_INIT( m79amb )
{
	palette_set_color(0,0,0,0);
	palette_set_color(1,255,255,255);
}

static MACHINE_RESET( m79amb )
{
	ramtek_mask_w(0,0);
	m79amb_sound0_w(0,0);
	m79amb_sound3_w(0,0);
	m79amb_lamp_w(0,0);
}

static DRIVER_INIT( m79amb )
{
	static const UINT8 cross[32] =
	{
		19,20,21,23,25,27,29,37,45,53,66,82,88,95,105,111,
		118,130,142,149,158,165,170,177,191,203,209,218,228,243,249,255
	};
	int i,j = 0,k = 255;
	for (i = 0; i < 32; i++)
	{
		UINT8 gray = i ^ (i >> 1) ^ 0x1f;
		while (j < 256 && j <= cross[i]) gun_lut[0][j++] = gray;
		while (k >= 0 && k >= 253 - cross[i]) gun_lut[1][k--] = gray;
	}
}

static MACHINE_DRIVER_START( m79amb )
	MDRV_CPU_ADD(8080,19660800/10)
	MDRV_CPU_PROGRAM_MAP(m79amb_map,0)
	MDRV_CPU_VBLANK_INT(m79amb_interrupt,1)
	MDRV_FRAMES_PER_SECOND(4915200.0 / (320 * 262))
	MDRV_VBLANK_DURATION(1000000.0 * 320 * 38 / 4915200.0)
	MDRV_MACHINE_RESET(m79amb)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256,262)
	MDRV_VISIBLE_AREA(0,255,32,255)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(m79amb)
	MDRV_VIDEO_UPDATE(m79amb)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DISCRETE,0)
	MDRV_SOUND_CONFIG(m79amb_discrete_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.125)
MACHINE_DRIVER_END

ROM_START( m79amb )
	ROM_REGION( 0x10000, REGION_CPU1, ROMREGION_INVERT )     /* 64k for code */
	ROM_LOAD( "m79.10t",      0x0000, 0x0200, CRC(ccf30b1e) SHA1(c1a77f8dc81c491928f81121ca5c9b7f8753794f) )
	ROM_LOAD( "m79.9t",       0x0200, 0x0200, CRC(daf807dd) SHA1(16cd9d553bfb111c8380966cbde39dbddd5fe58c) )
	ROM_LOAD( "m79.8t",       0x0400, 0x0200, CRC(79fafa02) SHA1(440620f5be44febdd7c64014739dc71fb570cc92) )
	ROM_LOAD( "m79.7t",       0x0600, 0x0200, CRC(06f511f8) SHA1(a8dcaec0723b8ac9ad9474e3e931a23d7aa4ec23) )
	ROM_LOAD( "m79.6t",       0x0800, 0x0200, CRC(24634390) SHA1(bc5f2ae5a49904dde1bd5e6b134571bf1d912735) )
	ROM_LOAD( "m79.5t",       0x0a00, 0x0200, CRC(95252aa6) SHA1(e7ea598f864510557b511682a5d2223a512ff029) )
	ROM_LOAD( "m79.4t",       0x0c00, 0x0200, CRC(54cffb0f) SHA1(4a4ad921ef6324c927a2e4a9da624d8096b6d87b) )
	ROM_LOAD( "m79.3ta",      0x0e00, 0x0200, CRC(27db5ede) SHA1(890587181497ed6e1d45ed501790a6d4d3315f00) )
	ROM_LOAD( "m79.10u",      0x1000, 0x0200, CRC(e41d13d2) SHA1(cc2911f46a0465305e4c7bc08f55acd065f93534) )
	ROM_LOAD( "m79.9u",       0x1200, 0x0200, CRC(e35f5616) SHA1(394ad92ad7dd233ece17335cf20aef8861b41508) )
	ROM_LOAD( "m79.8u",       0x1400, 0x0200, CRC(14eafd7c) SHA1(ca2d17f6ae1c3ff461a1b2bc6f37622e70cdaae8) )
	ROM_LOAD( "m79.7u",       0x1600, 0x0200, CRC(b9864f25) SHA1(9330cf96b7bce13e0ee3ad746b00e82ef10c3989) )
	ROM_LOAD( "m79.6u",       0x1800, 0x0200, CRC(dd25197f) SHA1(13eaf40251de82e817f488a9de738aadd8f6715e) )
	ROM_LOAD( "m79.5u",       0x1a00, 0x0200, CRC(251545e2) SHA1(05a0d5e8f143ea376fb3c517cf5e9d0d3534b933) )
	ROM_LOAD( "m79.4u",       0x1c00, 0x0200, CRC(b5f55c75) SHA1(f478cde73ae961be7b58c769f035eef58fd45555) )
	ROM_LOAD( "m79.3u",       0x1e00, 0x0200, CRC(e968691a) SHA1(7024d10f2af195fc4050861706b1f3d22cb22a9c) )
ROM_END

GAME( 1977, m79amb, 0, m79amb, m79amb, m79amb, ROT0, "Ramtek", "M-79 Ambush", GAME_IMPERFECT_SOUND )
