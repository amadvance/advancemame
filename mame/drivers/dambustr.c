// license:GPL-2.0+
// copyright-holders:Norbert Kehrer
/* Dambusters */

#include "driver.h"
#include "galaxian.h"
#include "sound/samples.h"

PALETTE_INIT( dambustr );
VIDEO_START( dambustr );
VIDEO_UPDATE( dambustr );
WRITE8_HANDLER( dambustr_bg_color_w );
WRITE8_HANDLER( dambustr_bg_split_w );

static UINT8 dambustr_noise_data;

static MACHINE_RESET( dambustr )
{
	dambustr_noise_data = 0;
	machine_reset_galaxian();
}

static WRITE8_HANDLER( dambustr_noise_w )
{
	if (dambustr_noise_data != data)
	{
		dambustr_noise_data = data;
		galaxian_noise_enable_w(offset, data);
	}
}

static WRITE8_HANDLER( dambustr_coin_w )
{
	coin_counter_w(offset, data & 1);
}

static ADDRESS_MAP_START( dambustr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8000) AM_WRITE(dambustr_bg_color_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(dambustr_bg_split_w)
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xd000, 0xd3ff) AM_READWRITE(galaxian_videoram_r, galaxian_videoram_w) AM_BASE(&galaxian_videoram)
	AM_RANGE(0xd400, 0xd7ff) AM_READ(galaxian_videoram_r)
	AM_RANGE(0xd800, 0xd83f) AM_RAM AM_WRITE(galaxian_attributesram_w) AM_BASE(&galaxian_attributesram)
	AM_RANGE(0xd840, 0xd85f) AM_RAM AM_BASE(&galaxian_spriteram) AM_SIZE(&galaxian_spriteram_size)
	AM_RANGE(0xd860, 0xd87f) AM_RAM AM_BASE(&galaxian_bulletsram) AM_SIZE(&galaxian_bulletsram_size)
	AM_RANGE(0xd880, 0xd8ff) AM_RAM
	AM_RANGE(0xe000, 0xe000) AM_READ(input_port_0_r)
	AM_RANGE(0xe002, 0xe003) AM_WRITE(dambustr_coin_w)
	AM_RANGE(0xe004, 0xe007) AM_WRITE(galaxian_lfo_freq_w)
	AM_RANGE(0xe800, 0xefff) AM_READ(input_port_1_r)
	AM_RANGE(0xe800, 0xe802) AM_WRITE(galaxian_background_enable_w)
	AM_RANGE(0xe803, 0xe803) AM_WRITE(dambustr_noise_w)
	AM_RANGE(0xe804, 0xe805) AM_WRITE(galaxian_shoot_enable_w)
	AM_RANGE(0xe806, 0xe807) AM_WRITE(galaxian_vol_w)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(input_port_2_r)
	AM_RANGE(0xf001, 0xf001) AM_WRITE(galaxian_nmi_enable_w)
	AM_RANGE(0xf004, 0xf004) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0xf006, 0xf006) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xf007, 0xf007) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xf800, 0xffff) AM_READ(watchdog_reset_r)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(galaxian_pitch_w)
ADDRESS_MAP_END

INPUT_PORTS_START( dambustr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_OTHER ) PORT_NAME("Initials Reset") PORT_CODE(KEYCODE_0)
	PORT_SERVICE_NO_TOGGLE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_DIPNAME( 0x40, 0x40, "Coin Counters" )
	PORT_DIPSETTING( 0x00, "1" )
	PORT_DIPSETTING( 0x40, "2" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0x80, "A 1/1, B 1/2" )
	PORT_DIPSETTING( 0x00, "A 1/2, B 1/6" )
	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING( 0x00, "3" )
	PORT_DIPSETTING( 0x02, "4" )
	PORT_DIPSETTING( 0x01, "5" )
	PORT_DIPSETTING( 0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Disable Background Collision" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Union Jack Flag" )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static const gfx_layout dambustr_charlayout =
{
	8,8, RGN_FRAC(1,2), 2,
	{ 0, RGN_FRAC(1,2) },
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24,32,40,48,56 },
	8*8
};

static const gfx_layout dambustr_spritelayout =
{
	16,16, RGN_FRAC(1,2), 2,
	{ 0, RGN_FRAC(1,2) },
	{ 0,1,2,3,4,5,6,7, 64,65,66,67,68,69,70,71 },
	{ 0,8,16,24,32,40,48,56, 128,136,144,152,160,168,176,184 },
	32*8
};

static const gfx_decode dambustr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &dambustr_charlayout, 0, 8 },
	{ REGION_GFX1, 0, &dambustr_spritelayout, 0, 8 },
	{ -1 }
};

static MACHINE_DRIVER_START( dambustr )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(dambustr_map, 0)
	MDRV_MACHINE_RESET(dambustr)
	MDRV_GFXDECODE(dambustr_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32+64+2+8)
	MDRV_PALETTE_INIT(dambustr)
	MDRV_VIDEO_START(dambustr)
	MDRV_VIDEO_UPDATE(dambustr)
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(galaxian_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( dambustr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "db8a.pr8",   0x4000, 0x1000, CRC(fd041ff4) SHA1(8d27da7bf0c655633711b960cbc23950c8a371ae) )
	ROM_LOAD( "db6a.pr6",   0x5000, 0x1000, CRC(448db54b) SHA1(c9afbf02bf4d4ac2972ab7ac6adfa4e951ae79c2) )
	ROM_LOAD( "db7a.pr7",   0x6000, 0x1000, CRC(675b1f5e) SHA1(6a386212a640fb467b6956a4dc5a68476af1cf97) )
	ROM_LOAD( "db5a.pr5",   0x7000, 0x1000, CRC(75659ecc) SHA1(b61254fb12f3999607abd88d1cc649dcfbf0384c) )

	ROM_REGION( 0x10000,REGION_USER1,0)
	ROM_LOAD( "db11a.pr11",  0x0000, 0x1000, CRC(427bd3fb) SHA1(cdbaef4040fa2e0598a086e320d51ecb26a591dd) )
	ROM_LOAD( "db9a.pr9",    0x1000, 0x1000, CRC(57164563) SHA1(8471d0660f39511d0afa3cdd63a1e84b0ea80fd0) )
	ROM_LOAD( "db10a.pr10",  0x2000, 0x1000, CRC(075b9c5e) SHA1(ff6ce873897004c0e796813725e260df85a520f9) )
	ROM_LOAD( "db12a.pr12",  0x3000, 0x1000, CRC(ed01a68b) SHA1(9dd37c2a25865717a7acdd7e2a3bef26a4cef3d9) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db3a.pr3",   0x0000, 0x1000, CRC(9e9a9710) SHA1(a9f67a05a2882b9f6f3378cc73e90539de4b8ca4) )
	ROM_LOAD( "db1a.pr1",   0x1000, 0x1000, CRC(4cb964cd) SHA1(1c90b14deb201a64b8ed4378b022e9e4574aed94) )
	ROM_LOAD( "db4a.pr3",   0x2000, 0x1000, CRC(d9d2df33) SHA1(97057fe33c146898755b556558ff707b9f4551ec) )
	ROM_LOAD( "db2a.pr2",   0x3000, 0x1000, CRC(0a0a6af5) SHA1(ecd2a6696ce9154f030c830ccb45690787881a73) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "mi-7603-5.ic4", 0x0000, 0x0020, CRC(f131f92f) SHA1(8b0f623d2ea09b5612dde0f330c5e473a7d72e06) )

	ROM_REGION( 0x0020, REGION_USER2, 0 )
	ROM_LOAD( "mi-7603-5.ic3", 0x0000, 0x0020, CRC(e2a54c47) SHA1(1e08f8e3d0ae0efb2d178ab11ec2bddaeb6d7478) )
ROM_END

ROM_START( dambust )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "db05.bin",   0x7000, 0x1000, CRC(75659ecc) SHA1(b61254fb12f3999607abd88d1cc649dcfbf0384c) )
	ROM_LOAD( "db06p.bin",   0x5000, 0x1000,  CRC(35dcee01) SHA1(2c23c727d9b38322a6d0548dfe6a2a254f3530af) )
	ROM_LOAD( "db07.bin",   0x6000, 0x1000,  CRC(675b1f5e) SHA1(6a386212a640fb467b6956a4dc5a68476af1cf97) )
	ROM_LOAD( "db08.bin",   0x4000, 0x1000,  CRC(fd041ff4) SHA1(8d27da7bf0c655633711b960cbc23950c8a371ae) )

	ROM_REGION( 0x10000,REGION_USER1,0)
 	ROM_LOAD( "db11.bin",   0x0000, 0x1000, CRC(9e6b34fe) SHA1(5cf47f5a5280ac53490240df220edf6178e87f4f) )
	ROM_LOAD( "db09.bin",    0x1000, 0x1000, CRC(57164563) SHA1(8471d0660f39511d0afa3cdd63a1e84b0ea80fd0) )
	ROM_LOAD( "db10p.bin",   0x2000, 0x1000, CRC(c129c57b) SHA1(c25abd7ee97b71941d9fa6acd0d92c116f1ff408) )
	ROM_LOAD( "db12.bin",   0x3000, 0x1000, CRC(ea4c65f5) SHA1(cb761e0543cacd6b437c6e88615f97df83245a34) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db1ap.bin",   0x1000, 0x1000, CRC(4cb964cd) SHA1(1c90b14deb201a64b8ed4378b022e9e4574aed94) )
	ROM_LOAD( "db02.bin",   0x3000, 0x1000, CRC(0a0a6af5) SHA1(ecd2a6696ce9154f030c830ccb45690787881a73) )
	ROM_LOAD( "db03.bin",   0x0000, 0x1000, CRC(9e9a9710) SHA1(a9f67a05a2882b9f6f3378cc73e90539de4b8ca4) )
	ROM_LOAD( "db04.bin",   0x2000, 0x1000, CRC(d9d2df33) SHA1(97057fe33c146898755b556558ff707b9f4551ec) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "mi-7603-5.ic4", 0x0000, 0x0020, CRC(f131f92f) SHA1(8b0f623d2ea09b5612dde0f330c5e473a7d72e06) )
ROM_END


static DRIVER_INIT(dambustr)
{
	int i,j;
	int tmpram[16];
	for(i=0;i<4096*4;i++)
	{
		memory_region(REGION_CPU1)[i]=memory_region(REGION_USER1)[BITSWAP16(i,15,14,13,12, 4,10,9,8,7,6,5,3,11,2,1,0)];
	}

	for (i = 0; i < 0x1000; i++)
	{
		UINT8 *rom = memory_region(REGION_CPU1);
		UINT8 tmp = rom[0x5000 + i];
		rom[0x5000 + i] = rom[0x6000 + i];
		rom[0x6000 + i] = rom[0x1000 + i];
		rom[0x1000 + i] = tmp;
	}
	for (i = 0; i < 0x1000; i++)
	{
		UINT8 *rom = memory_region(REGION_CPU1);
		rom[0x1000 + i] = BITSWAP8(rom[0x1000 + i],7,6,5,1,3,2,4,0);
		rom[0x4000 + i] = BITSWAP8(rom[0x4000 + i],7,6,5,1,3,2,4,0);
		rom[0x5000 + i] = BITSWAP8(rom[0x5000 + i],7,6,5,1,3,2,4,0);
	}

	//swap gfx
	for(i=0;i<0x4000;i+=16)
	{
		for(j=0;j<16;j++) tmpram[j]=memory_region(REGION_GFX1)[i+j];
		for(j=0;j<8;j++)
		{
			memory_region(REGION_GFX1)[i+j]=tmpram[j*2];
			memory_region(REGION_GFX1)[i+j+8]=tmpram[j*2+1];
		}
	}
}

GAME( 1981, dambustr,  0,       dambustr,  dambustr,  dambustr, ROT90, "South West Research", "Dambusters (US, set 1)", 0 )
GAME( 19??, dambust ,  dambustr,       dambustr,  dambustr,  dambustr, ROT90, "GAT", "Dambusters (set 2)",GAME_NOT_WORKING )
