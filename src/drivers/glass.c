/***************************************************************************

Glass (c) 1993 Gaelco (Developed by OMK. Produced by Gaelco)

Driver by Manuel Abadia <manu@teleline.es>

The DS5002FP has up to 128KB undumped gameplay code making the game unplayable :_(

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "sound/okim6295.h"

extern UINT16 *glass_vregs;
extern UINT16 *glass_videoram;
extern UINT16 *glass_spriteram;
extern int glass_current_bit;

/* from vidhrdw/glass.c */
WRITE16_HANDLER( glass_vram_w );
WRITE16_HANDLER( glass_blitter_w );
VIDEO_START( glass );
VIDEO_UPDATE( glass );

static int cause_interrupt;

static MACHINE_RESET( glass )
{
	cause_interrupt = 1;
	glass_current_bit = 0;
}

static WRITE16_HANDLER( clr_int_w )
{
	cause_interrupt = 1;
}

static INTERRUPT_GEN( glass_interrupt )
{
	if (cause_interrupt){
		cpunum_set_input_line(0, 6, HOLD_LINE);
		cause_interrupt = 0;
	}
}


static const gfx_layout glass_tilelayout16 =
{
	16,16,									/* 16x16 tiles */
	0x100000/32,							/* number of tiles */
	4,										/* 4 bpp */
	{ 3*0x100000*8, 2*0x100000*8, 1*0x100000*8, 0*0x100000*8 },
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7
	},
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	32*8
};

static const gfx_decode glass_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &glass_tilelayout16, 0, 64 },
	{ -1 }
};


static ADDRESS_MAP_START( glass_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)				/* ROM */
	AM_RANGE(0x100000, 0x101fff) AM_READ(MRA16_RAM)				/* Video RAM */
	AM_RANGE(0x102000, 0x102fff) AM_READ(MRA16_RAM)				/* Extra Video RAM */
	AM_RANGE(0x200000, 0x2007ff) AM_READ(MRA16_RAM)				/* Palette */
	AM_RANGE(0x440000, 0x440fff) AM_READ(MRA16_RAM)				/* Sprite RAM */
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_0_word_r)	/* DIPSW #2 */
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_1_word_r)	/* DIPSW #1 */
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_2_word_r)	/* 1P Inputs */
	AM_RANGE(0x700006, 0x700007) AM_READ(input_port_3_word_r)	/* 2P Inputs + Button 3 */
	AM_RANGE(0x70000e, 0x70000f) AM_READ(OKIM6295_status_0_lsb_r)/* OKI6295 status register */
	AM_RANGE(0xfec000, 0xfeffff) AM_READ(MRA16_RAM)				/* Work RAM (partially shared with DS5002FP) */
ADDRESS_MAP_END


static WRITE16_HANDLER( OKIM6295_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_SOUND1);

	if (ACCESSING_LSB){
		memcpy(&RAM[0x30000], &RAM[0x40000 + (data & 0x0f)*0x10000], 0x10000);
	}
}

static WRITE16_HANDLER( glass_coin_w )
{
	switch (offset >> 3){
		case 0x00:	/* Coin Lockouts */
		case 0x01:
			coin_lockout_w((offset >> 3) & 0x01, ~data & 0x01);
			break;
		case 0x02:	/* Coin Counters */
		case 0x03:
			coin_counter_w((offset >> 3) & 0x01, data & 0x01);
			break;
		case 0x04:	/* Sound Muting (if bit 0 == 1, sound output stream = 0) */
			break;
	}
}

static ADDRESS_MAP_START( glass_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)								/* ROM */
	AM_RANGE(0x100000, 0x101fff) AM_WRITE(glass_vram_w) AM_BASE(&glass_videoram)			/* Video RAM */
	AM_RANGE(0x102000, 0x102fff) AM_WRITE(MWA16_RAM)								/* Extra Video RAM */
	AM_RANGE(0x108000, 0x108007) AM_WRITE(MWA16_RAM) AM_BASE(&glass_vregs)				/* Video Registers */
	AM_RANGE(0x108008, 0x108009) AM_WRITE(clr_int_w)								/* CLR INT Video */
	AM_RANGE(0x200000, 0x2007ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)/*  Palette */
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(MWA16_RAM) AM_BASE(&glass_spriteram)			/* Sprite RAM */
	AM_RANGE(0x700008, 0x700009) AM_WRITE(glass_blitter_w)						/* serial blitter */
	AM_RANGE(0x70000c, 0x70000d) AM_WRITE(OKIM6295_bankswitch_w)					/* OKI6295 bankswitch */
	AM_RANGE(0x70000e, 0x70000f) AM_WRITE(OKIM6295_data_0_lsb_w)					/* OKI6295 data register */
	AM_RANGE(0x70000a, 0x70004b) AM_WRITE(glass_coin_w)							/* Coin Counters/Lockout */
	AM_RANGE(0xfec000, 0xfeffff) AM_WRITE(MWA16_RAM)								/* Work RAM (partially shared with DS5002FP) */
ADDRESS_MAP_END


INPUT_PORTS_START( glass )
PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Version ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Light" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, "Credit configuration" )
	PORT_DIPSETTING(    0x40, "Start 1C" )
	PORT_DIPSETTING(    0x00, "Start 2C" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

PORT_START	/* 1P INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

PORT_START	/* 2P INPUTS + Button 3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static MACHINE_DRIVER_START( glass )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)		/* 12 MHz (M680000 P12) */
	MDRV_CPU_PROGRAM_MAP(glass_readmem,glass_writemem)
	MDRV_CPU_VBLANK_INT(glass_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(glass)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 368-1, 16, 256-1)
	MDRV_GFXDECODE(glass_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(glass)
	MDRV_VIDEO_UPDATE(glass)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( glass )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "1.c23",	0x000000, 0x040000, CRC(aeebd4ed) SHA1(04759dc146dff0fc74b78d70e79dfaebe68328f9) )
	ROM_LOAD16_BYTE( "2.c22",	0x000001, 0x040000, CRC(165e2e01) SHA1(180a2e2b5151f2321d85ac23eff7fbc9f52023a5) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	/* 0x000000-0x3fffff filled in later in the DRIVER_INIT */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "h13.bin", 0x000000, 0x200000, CRC(13ab7f31) SHA1(468424f74d6cccd1b445a9f20e2d24bc46d61ed6) )
	ROM_LOAD( "h11.bin", 0x200000, 0x200000, CRC(c6ac41c8) SHA1(22408ef1e35c66d0fba0c72972c46fad891d1193) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* 16 bitmaps (320x200, indexed colors) */
	ROM_LOAD( "h9.bin", 0x000000, 0x100000, CRC(b9492557) SHA1(3f5c0d696d65e1cd492763dfa749c813dd56a9bf) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "c1.bin",	 0x000000, 0x100000, CRC(d9f075a2) SHA1(31a7a677861f39d512e9d1f51925c689e481159a) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(				0x040000, 0x100000 )
ROM_END

ROM_START( glass10 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "c23.bin",	0x000000, 0x040000, CRC(688cdf33) SHA1(b59dcc3fc15f72037692b745927b110e97d8282e) )
	ROM_LOAD16_BYTE( "c22.bin",	0x000001, 0x040000, CRC(ab17c992) SHA1(1509b5b4bbfb4e022e0ab6fbbc0ffc070adfa531) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	/* 0x000000-0x3fffff filled in later in the DRIVER_INIT */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "h13.bin", 0x000000, 0x200000, CRC(13ab7f31) SHA1(468424f74d6cccd1b445a9f20e2d24bc46d61ed6) )
	ROM_LOAD( "h11.bin", 0x200000, 0x200000, CRC(c6ac41c8) SHA1(22408ef1e35c66d0fba0c72972c46fad891d1193) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* 16 bitmaps (320x200, indexed colors) */
	ROM_LOAD( "h9.bin", 0x000000, 0x100000, CRC(b9492557) SHA1(3f5c0d696d65e1cd492763dfa749c813dd56a9bf) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "c1.bin",	 0x000000, 0x100000, CRC(d9f075a2) SHA1(31a7a677861f39d512e9d1f51925c689e481159a) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(				0x040000, 0x100000 )
ROM_END

ROM_START( glass10a )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "gl.c23",	0x000000, 0x040000, CRC(c1393bea) SHA1(a5f877ba38305a7b49fa3c96b9344cbf71e8c9ef
) )
	ROM_LOAD16_BYTE( "gl.c22",	0x000001, 0x040000, CRC(0d6fa33e) SHA1(37e9258ef7e108d034c80abc8e5e5ab6dacf0a61) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	/* 0x000000-0x3fffff filled in later in the DRIVER_INIT */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "h13.bin", 0x000000, 0x200000, CRC(13ab7f31) SHA1(468424f74d6cccd1b445a9f20e2d24bc46d61ed6) )
	ROM_LOAD( "h11.bin", 0x200000, 0x200000, CRC(c6ac41c8) SHA1(22408ef1e35c66d0fba0c72972c46fad891d1193) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* 16 bitmaps (320x200, indexed colors) */
	ROM_LOAD( "h9.bin", 0x000000, 0x100000, CRC(b9492557) SHA1(3f5c0d696d65e1cd492763dfa749c813dd56a9bf) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "c1.bin",	 0x000000, 0x100000, CRC(d9f075a2) SHA1(31a7a677861f39d512e9d1f51925c689e481159a) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(				0x040000, 0x100000 )
ROM_END

ROM_START( glasskr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )    /* 68000 code */
	ROM_LOAD16_BYTE( "glassk.c23", 0x000000, 0x080000, CRC(6ee19376) SHA1(8a8fdeebe094bd3e29c35cf59584e3cab708732d) )
	ROM_LOAD16_BYTE( "glassk.c22", 0x000001, 0x080000, CRC(bd546568) SHA1(bcd5e7591f4e68c9470999b8a0ef1ee4392c907c) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	/* 0x000000-0x3fffff filled in later in the DRIVER_INIT */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "h13.bin", 0x000000, 0x200000, CRC(13ab7f31) SHA1(468424f74d6cccd1b445a9f20e2d24bc46d61ed6) )
	ROM_LOAD( "h11.bin", 0x200000, 0x200000, CRC(c6ac41c8) SHA1(22408ef1e35c66d0fba0c72972c46fad891d1193) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )   /* 16 bitmaps (320x200, indexed colors) */
	ROM_LOAD( "glassk.h9", 0x000000, 0x100000, CRC(d499be4c) SHA1(204f754813be687e8dc00bfe7b5dbc4857ac8738) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )    /* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "c1.bin", 0x000000, 0x100000, CRC(d9f075a2) SHA1(31a7a677861f39d512e9d1f51925c689e481159a) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(         0x040000, 0x100000 )
ROM_END

/***************************************************************************

    Split even/odd bytes from ROMs in 16 bit mode to different memory areas

***************************************************************************/

void glass_ROM16_split(int src_reg, int dst_reg, int start, int length, int dest1, int dest2)
{
	int i;

	/* get a pointer to the source data */
	UINT8 *src = (UINT8 *)memory_region(src_reg);

	/* get a pointer to the destination data */
	UINT8 *dst = (UINT8 *)memory_region(dst_reg);

	/* fill destination areas with the proper data */
	for (i = 0; i < length/2; i++){
		dst[dest1 + i] = src[start + i*2 + 0];
		dst[dest2 + i] = src[start + i*2 + 1];
	}
}

static DRIVER_INIT( glass )
{
	/*
    For REGION_GFX2 we have this memory map:
        0x000000-0x1fffff ROM H13
        0x200000-0x3fffff ROM H11

    and we are going to construct this one for REGION_GFX1:
        0x000000-0x0fffff ROM H13 even bytes
        0x100000-0x1fffff ROM H13 odd bytes
        0x200000-0x2fffff ROM H11 even bytes
        0x300000-0x3fffff ROM H11 odd bytes
    */

	/* split ROM H13 */
	glass_ROM16_split(REGION_GFX2, REGION_GFX1, 0x0000000, 0x0200000, 0x0000000, 0x0100000);

	/* split ROM H11 */
	glass_ROM16_split(REGION_GFX2, REGION_GFX1, 0x0200000, 0x0200000, 0x0200000, 0x0300000);
}

GAME( 1993, glass,    0,     glass, glass, glass, ROT0, "Gaelco", "Glass (Ver 1.1)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1993, glass10 , glass, glass, glass, glass, ROT0, "Gaelco", "Glass (Ver 1.0)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1993, glass10a, glass, glass, glass, glass, ROT0, "Gaelco", "Glass (Ver 1.0 set 2)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1993, glasskr,  glass, glass, glass, glass, ROT0, "Gaelco (Promat license)", "Glass (Ver 1.1, Break Edition) (censored, unprotected)", 0 )
