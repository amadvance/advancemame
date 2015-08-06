/***************************************************************************

    The Game Room Lethal Justice hardware

    driver by Aaron Giles

    Games supported:
        * Lethal Justice
        * Egg Venture

    Known bugs:
        * some DIP switches not understood

    Note:
    Even though Egg Venture looks like it has a few extra scanlines at
    the bottom, the video hardware is explicitly programmed to only
    display 236 lines. Altering this will lead to bad flickering.

****************************************************************************

         EU21     EU18     EU20   32.000MHz
         M6295    M6295    M6295       Xilinx
   Dip-8 2.000Mhz 2.000Mhz 2.000Mhz
J        VC9                 GUNCN
A  Dip-4 VC8                 Xilinx     EGR4
M               Mach210                 EGR6.3
M   M5M442256x4 Mach210 Mach210 Mach210 EGR3
A          11.2896MHz                   EGR2
     TMS34010           W241024x4       EGR1
 2803A                                  EGR5.3
    40.000MHz   Bt121 Mach210 Mach210 Mach210


Chips:
 TMS34010FNL-40     Main CPU
 Xilinx XC3042-70   Field Programmable Gate Array
 Bt121KPJ80         Triple 8-bit 80MHz VideoDAC
 AMD Mach210A-10JC  Programmable Logic Device (CPLD)
 ST ULN2803A        8 Darlington Transistor Array with common emitter

Note 1: Lethal Justice uses a 11.0592MHz OSC instead of the 11.2896MHz
Note 2: Lethal Justice uses a TMS34010FNL-50 instead of the TMS34010FNL-40

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "lethalj.h"
#include "sound/okim6295.h"



/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( lethalj_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM
	AM_RANGE(0x04000000, 0x0400000f) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x04000010, 0x0400001f) AM_READWRITE(OKIM6295_status_1_lsb_r, OKIM6295_data_1_lsb_w)
	AM_RANGE(0x04100000, 0x0410000f) AM_READWRITE(OKIM6295_status_2_lsb_r, OKIM6295_data_2_lsb_w)
	AM_RANGE(0x04100010, 0x0410001f) AM_READNOP		/* read but never examined */
	AM_RANGE(0x04200000, 0x0420001f) AM_WRITENOP	/* clocks bits through here */
	AM_RANGE(0x04300000, 0x0430007f) AM_READ(lethalj_gun_r)
	AM_RANGE(0x04400000, 0x0440000f) AM_WRITENOP	/* clocks bits through here */
	AM_RANGE(0x04500010, 0x0450001f) AM_READ(input_port_0_word_r)
	AM_RANGE(0x04600000, 0x0460000f) AM_READ(input_port_1_word_r)
	AM_RANGE(0x04700000, 0x047000ff) AM_WRITE(lethalj_blitter_w)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xc0000240, 0xc000025f) AM_WRITENOP	/* seems to be a bug in their code, one of many. */
	AM_RANGE(0xff800000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( lethalj )
	PORT_START
	PORT_BIT( 0x0003, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* ??? Seems to be rigged up to the auto scroll, and acts as a fast forward*/
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xffe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x00c0, 0x0000, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x00c0, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0300, 0x0100, DEF_STR( Lives ))
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPSETTING(      0x0300, "5" )
	PORT_DIPNAME( 0x0c10, 0x0010, "Right Gun Offset" )
	PORT_DIPSETTING(      0x0000, "-4" )
	PORT_DIPSETTING(      0x0400, "-3" )
	PORT_DIPSETTING(      0x0800, "-2" )
	PORT_DIPSETTING(      0x0c00, "-1" )
	PORT_DIPSETTING(      0x0010, "0" )
	PORT_DIPSETTING(      0x0410, "+1" )
	PORT_DIPSETTING(      0x0810, "+2" )
	PORT_DIPSETTING(      0x0c10, "+3" )
	PORT_DIPNAME( 0x3020, 0x0020, "Left Gun Offset" )
	PORT_DIPSETTING(      0x0000, "-4" )
	PORT_DIPSETTING(      0x1000, "-3" )
	PORT_DIPSETTING(      0x2000, "-2" )
	PORT_DIPSETTING(      0x3000, "-1" )
	PORT_DIPSETTING(      0x0020, "0" )
	PORT_DIPSETTING(      0x1020, "+1" )
	PORT_DIPSETTING(      0x2020, "+2" )
	PORT_DIPSETTING(      0x3020, "+3" )
	PORT_DIPNAME( 0x4000, 0x0000, "DIP E" )
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x4000, "1" )
	PORT_DIPNAME( 0x8000, 0x8000, "Global Gun Offset" )
	PORT_DIPSETTING(      0x0000, "-2.5" )
	PORT_DIPSETTING(      0x8000, "+0" )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( eggventr )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x0070, 0x0000, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x0040, DEF_STR( 8C_1C ))
	PORT_DIPSETTING(      0x0030, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0050, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x0070, DEF_STR( Free_Play ))
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) ) // Verified Correct
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPSETTING(      0x0300, "6" )
	PORT_DIPNAME( 0x0c00, 0x0800, DEF_STR( Difficulty ) ) // According to info from The Gameroom
	PORT_DIPSETTING(      0x0c00, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Slot Machine" ) // Verified Correct - Unused for the Deluxe version?? Yes, the slot machine
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) ) // is present in the code as a 'bonus stage' (when the egg reaches Vegas?),
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) ) // but not actually called (EC).
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x7f00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( eggvntdx )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x0070, 0x0000, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x0040, DEF_STR( 8C_1C ))
	PORT_DIPSETTING(      0x0030, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(      0x0020, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(      0x0050, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_4C ))
	PORT_DIPSETTING(      0x0070, DEF_STR( Free_Play ))
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) ) // Verified Correct
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x0100, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPSETTING(      0x0300, "6" )
	PORT_DIPNAME( 0x0c00, 0x0800, DEF_STR( Difficulty ) ) // According to info from The Gameroom
	PORT_DIPSETTING(      0x0c00, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x7f00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END



/*************************************
 *
 *  34010 configuration
 *
 *************************************/

static struct tms34010_config tms_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	NULL,							/* write to shiftreg function */
	NULL,							/* read from shiftreg function */
	NULL,							/* display address changed */
	NULL							/* display interrupt callback */
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( lethalj )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(lethalj_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((1000000 * (258 - 236)) / (60 * 258))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(512, 236)
	MDRV_VISIBLE_AREA(0, 511, 0, 235)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(lethalj)
	MDRV_VIDEO_UPDATE(lethalj)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 2000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	MDRV_SOUND_ADD(OKIM6295, 2000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

	MDRV_SOUND_ADD(OKIM6295, 2000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( lethalj )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* 34010 code */
	ROM_LOAD16_BYTE( "vc8",  0x000000, 0x080000, CRC(8d568e1d) SHA1(e4dd3794789f9ccd7be8374978a3336f2b79136f) )
	ROM_LOAD16_BYTE( "vc9",  0x000001, 0x080000, CRC(8f22add4) SHA1(e773d3ae9cf512810fc266e784d21ed115c8830c) )

	ROM_REGION16_LE( 0x600000, REGION_GFX1, 0 )			/* graphics data */
	ROM_LOAD16_BYTE( "gr1",  0x000000, 0x100000, CRC(27f7b244) SHA1(628b29c066e217e1fe54553ea3ed98f86735e262) )
	ROM_LOAD16_BYTE( "gr2",  0x000001, 0x100000, CRC(1f25d3ab) SHA1(bdb8a3c546cdee9a5630c47b9c5079a956e8a093) )
	ROM_LOAD16_BYTE( "gr4",  0x200000, 0x100000, CRC(c5838b4c) SHA1(9ad03d0f316eb31fdf0ca6f65c02a27d3406d072) )
	ROM_LOAD16_BYTE( "gr3",  0x200001, 0x100000, CRC(ba9fa057) SHA1(db6f11a8964870f04f94fef6f1b1a58168a942ad) )
	ROM_LOAD16_BYTE( "gr6",  0x400000, 0x100000, CRC(51c99b85) SHA1(9a23bf21a73d2884b49c64a8f42c288534c79dc5) )
	ROM_LOAD16_BYTE( "gr5",  0x400001, 0x100000, CRC(80dda9b5) SHA1(d8a79cad112bc7d9e4ba31a950e4807581f3bf46) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0)				/* sound data */
	ROM_LOAD( "sound1", 0x00000, 0x40000, CRC(7d93ca66) SHA1(9e1dc0efa5d0f770c7e1f10de56fbf5620dea437) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0)				/* sound data */
	ROM_LOAD( "u21",    0x00000, 0x40000, CRC(7d3beae0) SHA1(5ec753c5fd5ca0f9492c9e274703a1aa758062a7) )

	ROM_REGION( 0x40000, REGION_SOUND3, 0)				/* sound data */
	ROM_LOAD( "sound1", 0x00000, 0x40000, CRC(7d93ca66) SHA1(9e1dc0efa5d0f770c7e1f10de56fbf5620dea437) )
ROM_END


ROM_START( eggventr )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* 34010 code */
	ROM_LOAD16_BYTE( "eggvc8.10", 0x000000, 0x020000, CRC(225d1164) SHA1(b0dc55f2e8ded1fe7874de05987fcf879772289e) )
	ROM_LOAD16_BYTE( "eggvc9.10", 0x000001, 0x020000, CRC(42f6e904) SHA1(11be8e7383a218aac0e1a63236bbdb7cca0993bf) )
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x040000 )
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x080000 )

	ROM_REGION16_LE( 0x600000, REGION_GFX1, 0 )			/* graphics data */
	ROM_LOAD16_BYTE( "egr1.bin",  0x000000, 0x100000, CRC(f73f80d9) SHA1(6278b45579a256b9576ba6d4f5a15fab26797c3d) )
	ROM_LOAD16_BYTE( "egr2.bin",  0x000001, 0x100000, CRC(3a9ba910) SHA1(465aa3119af103aa65b25042b3572fdcb9c1887a) )
	ROM_LOAD16_BYTE( "egr4.bin",  0x200000, 0x100000, CRC(4ea5900e) SHA1(20341337ee3c6c22580c52312156b818f4187693) )
	ROM_LOAD16_BYTE( "egr3.bin",  0x200001, 0x100000, CRC(3f8dfc73) SHA1(83a168069f896ea7e67a97c6d591d09b19d5f486) )
	ROM_LOAD16_BYTE( "egr6.3",    0x400000, 0x100000, CRC(f299d818) SHA1(abbb333c43675d34c59201b5d297779cfea8b092) )
	ROM_LOAD16_BYTE( "egr5.3",    0x400001, 0x100000, CRC(ebfca07b) SHA1(20465d14b41d99651166f221057737d7b3cc770c) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )

	ROM_REGION( 0x40000, REGION_SOUND3, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )
ROM_END


ROM_START( eggvent7 )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* 34010 code */
	ROM_LOAD16_BYTE( "eggvc8.7", 0x000000, 0x020000, CRC(99999899) SHA1(e3908600fa711baa7f7562f86498ec7e988a5bea) )
	ROM_LOAD16_BYTE( "eggvc9.7", 0x000001, 0x020000, CRC(1b608155) SHA1(256dd981515d57f806a3770bdc6ff46b9000f7f3) )
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x040000 )
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x080000 )

	ROM_REGION16_LE( 0x600000, REGION_GFX1, 0 )			/* graphics data */
	ROM_LOAD16_BYTE( "egr1.bin",  0x000000, 0x100000, CRC(f73f80d9) SHA1(6278b45579a256b9576ba6d4f5a15fab26797c3d) )
	ROM_LOAD16_BYTE( "egr2.bin",  0x000001, 0x100000, CRC(3a9ba910) SHA1(465aa3119af103aa65b25042b3572fdcb9c1887a) )
	ROM_LOAD16_BYTE( "egr4.bin",  0x200000, 0x100000, CRC(4ea5900e) SHA1(20341337ee3c6c22580c52312156b818f4187693) )
	ROM_LOAD16_BYTE( "egr3.bin",  0x200001, 0x100000, CRC(3f8dfc73) SHA1(83a168069f896ea7e67a97c6d591d09b19d5f486) )
	ROM_LOAD16_BYTE( "egr6.3",    0x400000, 0x100000, CRC(f299d818) SHA1(abbb333c43675d34c59201b5d297779cfea8b092) )
	ROM_LOAD16_BYTE( "egr5.3",    0x400001, 0x100000, CRC(ebfca07b) SHA1(20465d14b41d99651166f221057737d7b3cc770c) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )

	ROM_REGION( 0x40000, REGION_SOUND3, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )
ROM_END


ROM_START( eggvntdx )
	ROM_REGION16_LE( 0x100000, REGION_USER1, 0 )		/* 34010 code */
	ROM_LOAD16_BYTE( "eggdlx.vc8", 0x000000, 0x080000, CRC(d7f56141) SHA1(3c16b509fd1c763e452c27084fb0e90cde3947f7) )
	ROM_LOAD16_BYTE( "eggdlx.vc9", 0x000001, 0x080000, CRC(cc5f122e) SHA1(e719a3937378df605cdb86c59a534808473c8f90) )

	ROM_REGION16_LE( 0x600000, REGION_GFX1, 0 )			/* graphics data */
	ROM_LOAD16_BYTE( "egr1.bin",     0x000000, 0x100000, CRC(f73f80d9) SHA1(6278b45579a256b9576ba6d4f5a15fab26797c3d) )
	ROM_LOAD16_BYTE( "egr2.bin",     0x000001, 0x100000, CRC(3a9ba910) SHA1(465aa3119af103aa65b25042b3572fdcb9c1887a) )
	ROM_LOAD16_BYTE( "eggdlx.gr4",   0x200000, 0x100000, CRC(cfb1e28b) SHA1(8d535a27158acee893233cf2012b4ab0ffc8dc03) )
	ROM_LOAD16_BYTE( "eggdlx.gr3",   0x200001, 0x100000, CRC(a7da3891) SHA1(9139c846006bbed4bdb183659a5b40aaa0000708) )
	ROM_LOAD16_BYTE( "eggdlx.gr6",   0x400000, 0x100000, CRC(97d02e8a) SHA1(6f9532fb031953c1187782b4fce5a0cfaf9461b3) )
	ROM_LOAD16_BYTE( "eggdlx.gr5",   0x400001, 0x100000, CRC(387d9176) SHA1(9f26f97cab8baeea1d5e4860a8a35a55bdc601e8) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )

	ROM_REGION( 0x40000, REGION_SOUND3, 0)				/* sound data */
	ROM_LOAD( "eu18.bin", 0x00000, 0x40000, CRC(3760b1db) SHA1(70e258a6036f9ce26b354c4df57e0e4d2c871bcb) )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1996, lethalj,  0,        lethalj, lethalj,  0, ROT0, "The Game Room", "Lethal Justice", 0 )
GAME( 1997, eggventr, 0,        lethalj, eggventr, 0, ROT0, "The Game Room", "Egg Venture (Release 10)", 0 )
GAME( 1997, eggvent7, eggventr, lethalj, eggventr, 0, ROT0, "The Game Room", "Egg Venture (Release 7)", 0 )
GAME( 1997, eggvntdx, eggventr, lethalj, eggvntdx, 0, ROT0, "The Game Room", "Egg Venture Deluxe", 0 )
