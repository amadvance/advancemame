/***************************************************************************

Target Hits (c) 1994 Gaelco (Designed & Developed by Zigurat. Produced by Gaelco)

Driver by Manuel Abadia <manu@teleline.es>

The DS5002FP has 32KB undumped gameplay code making the game unplayable :_(

***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "sound/okim6295.h"

extern UINT16 *targeth_vregs;
extern UINT16 *targeth_videoram;
extern UINT16 *targeth_spriteram;

/* from vidhrdw/targeth.c */
WRITE16_HANDLER( targeth_vram_w );
VIDEO_START( targeth );
VIDEO_UPDATE( targeth );


static const gfx_layout tilelayout16_0x080000 =
{
	16,16,														/* 16x16 tiles */
	RGN_FRAC(1,4),												/* number of tiles */
	4,															/* bitplanes */
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7, 16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static const gfx_decode gfxdecodeinfo_0x080000[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout16_0x080000, 0, 64 },
	{ -1 }
};


static INTERRUPT_GEN(targeth_interrupt )
{
	switch(cpu_getiloops()){
		case 0: /* IRQ 2: drives the game */
			cpunum_set_input_line(0, 2, HOLD_LINE);
			break;
		case 1: /* IRQ 4: Read 1P Gun */
			cpunum_set_input_line(0, 4, HOLD_LINE);
			break;
		case 2:	/* IRQ 6: Read 2P Gun */
			cpunum_set_input_line(0, 6, HOLD_LINE);
			break;
	}
}

/* correct i think */
static READ16_HANDLER(service_mirror_r)
{
   return readinputport(7) << 8;
}

static ADDRESS_MAP_START( targeth_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)			/* ROM */
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)			/* Video RAM */
	AM_RANGE(0x108000, 0x108001) AM_READ(input_port_0_word_r)/* Gun 1P X */
	AM_RANGE(0x108002, 0x108003) AM_READ(input_port_1_word_r)/* Gun 1P Y */
	AM_RANGE(0x108004, 0x108005) AM_READ(input_port_2_word_r)/* Gun 2P X */
	AM_RANGE(0x108006, 0x108007) AM_READ(input_port_3_word_r)/* Gun 2P Y */
	AM_RANGE(0x200000, 0x2007ff) AM_READ(MRA16_RAM)			/* Palette */
	AM_RANGE(0x440000, 0x440fff) AM_READ(MRA16_RAM)			/* Sprite RAM */
	AM_RANGE(0x700000, 0x700001) AM_READ(input_port_4_word_r)/* DIPSW #2 */
	AM_RANGE(0x700002, 0x700003) AM_READ(input_port_5_word_r)/* DIPSW #1 */
	AM_RANGE(0x700006, 0x700007) AM_READ(input_port_6_word_r)/* Coins, Start & Fire buttons */
	AM_RANGE(0x700008, 0x700009) AM_READ(input_port_7_word_r)/* Service & Guns Reload? */
	AM_RANGE(0x70000a, 0x70000b) AM_READ(service_mirror_r)   /* quickshts reads the service switch here instead */
	AM_RANGE(0x70000e, 0x70000f) AM_READ(OKIM6295_status_0_lsb_r)/* OKI6295 status register */
	AM_RANGE(0xfe0000, 0xfeffff) AM_READ(MRA16_RAM)			/* Work RAM (partially shared with DS5002FP) */
ADDRESS_MAP_END

static WRITE16_HANDLER( OKIM6295_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_SOUND1);

	if (ACCESSING_LSB){
		memcpy(&RAM[0x30000], &RAM[0x40000 + (data & 0x0f)*0x10000], 0x10000);
	}
}

static WRITE16_HANDLER( targeth_coin_counter_w )
{
	coin_counter_w( (offset >> 3) & 0x01, data & 0x01);
}

static ADDRESS_MAP_START( targeth_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)								/* ROM */
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(targeth_vram_w) AM_BASE(&targeth_videoram)		/* Video RAM */
	AM_RANGE(0x108000, 0x108007) AM_WRITE(MWA16_RAM) AM_BASE(&targeth_vregs)				/* Video Registers */
	AM_RANGE(0x10800c, 0x10800d) AM_WRITE(MWA16_NOP)								/* CLR Video INT */
	AM_RANGE(0x200000, 0x2007ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)/* Palette */
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(MWA16_RAM) AM_BASE(&targeth_spriteram)			/* Sprite RAM */
	AM_RANGE(0x70000c, 0x70000d) AM_WRITE(OKIM6295_bankswitch_w)					/* OKI6295 bankswitch */
	AM_RANGE(0x70000e, 0x70000f) AM_WRITE(OKIM6295_data_0_lsb_w)					/* OKI6295 data register */
	AM_RANGE(0x70002a, 0x70003b) AM_WRITE(targeth_coin_counter_w)					/* Coin counters */
	AM_RANGE(0x700010, 0x70001b) AM_WRITE(MWA16_NOP)                                /* ??? Guns reload related? */ /* switch address to here fixes sound */							    /* ??? Guns reload related? */ /* switch address fixes sound */
	AM_RANGE(0xfe0000, 0xfeffff) AM_WRITE(MWA16_RAM)								/* Work RAM (partially shared with DS5002FP) */
ADDRESS_MAP_END


INPUT_PORTS_START( targeth )
PORT_START	/* Gun 1 X */
	PORT_BIT( 0x01ff, 200, IPT_LIGHTGUN_X ) PORT_MINMAX( 0, 400 + 4) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_PLAYER(1)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* Gun 1 Y */
	PORT_BIT( 0x01ff, 128, IPT_LIGHTGUN_Y ) PORT_MINMAX(4,255) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_PLAYER(1)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* Gun 2 X */
	PORT_BIT( 0x01ff, 400 + 4, IPT_LIGHTGUN_X ) PORT_MINMAX( 0, 400 + 4) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_REVERSE PORT_PLAYER(2)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* Gun 2 Y */
	PORT_BIT( 0x01ff, 255, IPT_LIGHTGUN_Y ) PORT_MINMAX(4,255) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_REVERSE PORT_PLAYER(2)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Gun alarm" )	/* This doesn't work. What's supposed to do? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "Credit configuration" )
	PORT_DIPSETTING(    0x40, "Start 1C/Continue 1C" )
	PORT_DIPSETTING(    0x00, "Start 2C/Continue 1C" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

PORT_START	/* Button 1, COINSW & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

PORT_START	/* Service & Button 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* this MUST be low or the game doesn't boot */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* Reload 1P? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* Reload 2P? */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static MACHINE_DRIVER_START( targeth )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12 MHz */
	MDRV_CPU_PROGRAM_MAP(targeth_readmem,targeth_writemem)
	MDRV_CPU_VBLANK_INT(targeth_interrupt,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 16*16)
	MDRV_VISIBLE_AREA(3*8, 23*16-8-1, 16, 16*16-8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_0x080000)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(targeth)
	MDRV_VIDEO_UPDATE(targeth)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( targeth )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"targeth.c23",	0x000000, 0x040000, CRC(840887d6) SHA1(9a36b346608d531a62a2e0704ea44f12e07f9d91) )
	ROM_LOAD16_BYTE(	"targeth.c22",	0x000001, 0x040000, CRC(d2435eb8) SHA1(ce75a115dad8019c8e66a1c3b3e15f54781f65ae) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "targeth.i13",	0x000000, 0x080000, CRC(b892be24) SHA1(9cccaaacf20e77c7358f0ceac60b8a1012f1216c) )
	ROM_LOAD( "targeth.i11",	0x080000, 0x080000, CRC(6797faf9) SHA1(112cffe72f91cb46c262e19a47b0cab3237dd60f) )
	ROM_LOAD( "targeth.i9",		0x100000, 0x080000, CRC(0e922c1c) SHA1(6920e345c82e76f7e0af6101f39eb65ac1f112b9) )
	ROM_LOAD( "targeth.i7",		0x180000, 0x080000, CRC(d8b41000) SHA1(cbe91eb91bdc7a60b2333c6bea37d08a57902669) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "targeth.c1",		0x000000, 0x080000, CRC(d6c9dfbc) SHA1(3ec70dea94fc89df933074012a52de6034571e87) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(					0x040000, 0x080000 )
	ROM_LOAD( "targeth.c3",		0x0c0000, 0x080000, CRC(d4c771df) SHA1(7cc0a86ef6aa3d26ab8f19d198f62112bf012870) )
ROM_END

ROM_START( quickshts )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "book_16-06_o_6f77_27c020.bin", 0x000001, 0x040000, CRC(9305509b) SHA1(aca1c468b9af2c56ca0d34dcbaa1b18f9c643250) )
	ROM_LOAD16_BYTE( "book_16-06_e_7fd9_27c020.bin", 0x000000, 0x040000, CRC(74999de4) SHA1(ad735a8a5f0543d7419b3b34133982537b378ea4) )
/*
	// Either this isn't using a DS5002FP (but something else) or it expects a different ROM.
	ROM_REGION( 0x8000, "gaelco_ds5002fp:sram", 0 ) // DS5002FP code
	ROM_LOAD( "targeth_ds5002fp.bin", 0x0000, 0x8000, BAD_DUMP CRC(abcdfee4) SHA1(c5955d5dbbcecbe1c2ae77d59671ae40eb814d30) )

	ROM_REGION( 0x100, "gaelco_ds5002fp:mcu:internal", ROMREGION_ERASE00 )
	ROM_LOAD( "targeth_ds5002fp_scratch", 0x00, 0x80, BAD_DUMP CRC(c927bcb1) SHA1(86b5c7ee6a4a5f0aa538a6742253da1afadb4345) ) // default state so you don't have to manually initialize game
	DS5002FP_SET_MON( 0x49 )
	DS5002FP_SET_RPCTL( 0x00 )
	DS5002FP_SET_CRCR( 0x80 )
*/
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "targeth.i13",	0x000000, 0x080000, CRC(b892be24) SHA1(9cccaaacf20e77c7358f0ceac60b8a1012f1216c) )
	ROM_LOAD( "targeth.i11",	0x080000, 0x080000, CRC(6797faf9) SHA1(112cffe72f91cb46c262e19a47b0cab3237dd60f) )
	ROM_LOAD( "targeth.i9",		0x100000, 0x080000, CRC(0e922c1c) SHA1(6920e345c82e76f7e0af6101f39eb65ac1f112b9) )
	ROM_LOAD( "targeth.i7",		0x180000, 0x080000, CRC(d8b41000) SHA1(cbe91eb91bdc7a60b2333c6bea37d08a57902669) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "targeth.c1",		0x000000, 0x080000, CRC(d6c9dfbc) SHA1(3ec70dea94fc89df933074012a52de6034571e87) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(					0x040000, 0x080000 )
	ROM_LOAD( "targeth.c3",		0x0c0000, 0x080000, CRC(d4c771df) SHA1(7cc0a86ef6aa3d26ab8f19d198f62112bf012870) )
ROM_END

GAME( 1994, targeth,   0,       targeth, targeth, 0, ROT0, "Gaelco", "Target Hits", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING ) // not getting the game?
/* This is a strange set, it reports the DS5002 / Coprocessor as "Not Ready" but isn't using it for the usual protection (maybe only NVRAM storage, check)
   It also reads the service port on a different address, could be it has a different co-processor entirely? 
*/
GAME( 1994, quickshts, targeth, targeth, targeth, 0, ROT0, "Gaelco", "Quick Shots (ver 1.0)", 0 ) /* 16/Jun/1994. Marked "Book" internally at Gaelco */
