/*
************************************
 Card Line
 preliminary driver by Tomasz Slanina
 analog[at]op[dot]pl


 SIEMENS 80C32

 MC6845P

 GM76C88 x3 (8K x 8)
 K-665 9546

 STARS B2072 9629

 XTAL 12 MHz, 4 MHz

************************************
*/

#include "driver.h"
#include "cpu/i8051/i8051.h"

static tilemap *bgtilemap  = NULL;
static tilemap *fgtilemap  = NULL;

static void get_bg_tile_info(int tile_index)
{
	int code;

	code = videoram[tile_index ] | (colorram[tile_index]<<8);

	SET_TILE_INFO(
			0,
			code,
			0	,
			0)
}

static void get_fg_tile_info(int tile_index)
{
	int code;

	code = videoram[tile_index+0x800 ] | (colorram[tile_index+0x800]<<8);

	SET_TILE_INFO(
			0,
			code,
			0	,
			0)
}


VIDEO_START( cardline )
{
	bgtilemap  = tilemap_create(get_bg_tile_info, tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);
	fgtilemap  = tilemap_create(get_fg_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap_set_transparent_pen(fgtilemap, 1);
	return 0;
}

VIDEO_UPDATE( cardline )
{
	tilemap_mark_all_tiles_dirty(bgtilemap);
	tilemap_mark_all_tiles_dirty(fgtilemap);
	tilemap_draw(bitmap,cliprect, bgtilemap, 0,0);
	tilemap_draw(bitmap,cliprect, fgtilemap, 0,0);
}

static ADDRESS_MAP_START( mem_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END


static READ8_HANDLER(myread)
{
	return mame_rand();
}

static ADDRESS_MAP_START( mem_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2003, 0x2003) AM_READ(input_port_0_r)
	AM_RANGE(0x2005, 0x2005) AM_READ(myread) //AM_READ(input_port_1_r)//coin + test
	AM_RANGE(0x2006, 0x2006) AM_READ(input_port_2_r) //counters (status?) see input ports for more details
	AM_RANGE(0x2007, 0x2008) AM_NOP
	AM_RANGE(0x2080, 0x213f) AM_NOP
	AM_RANGE(0x2400, 0x2400) AM_NOP
	AM_RANGE(0x2800, 0x2801) AM_NOP
	AM_RANGE(0x2840, 0x2840) AM_NOP
	AM_RANGE(0x2880, 0x2880) AM_NOP
	AM_RANGE(0x3003, 0x3003) AM_NOP
	AM_RANGE(0xc000, 0xcfff) AM_RAM  AM_BASE(&videoram)
	AM_RANGE(0xe000, 0xefff) AM_RAM  AM_BASE(&colorram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mem_io, ADDRESS_SPACE_IO, 8 )
  AM_RANGE(0x00, 0x03) AM_READ(myread) AM_WRITENOP
ADDRESS_MAP_END

INPUT_PORTS_START( cardline )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x01, "0-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x01, "1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) //bookeeping
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) //test
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) //payout
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x01, 0x00, "2-0" )						//coin in counter
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) //"wirtaufbuhung" counter
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )//lamps
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )//coin out counter
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )//"wirtauszahlung" counter
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )// auswurfeinheit
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
INPUT_PORTS_END

static gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7  },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*8*8
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 2 },
	{ -1 }	/* end of array */
};

PALETTE_INIT(cardline)
{
	int i,r,g,b,data;
	int bit0,bit1,bit2;
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		data=color_prom[i];

		/* red component */
		bit0 = (data >> 5) & 0x01;
		bit1 = (data >> 6) & 0x01;
		bit2 = (data >> 7) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (data >> 2) & 0x01;
		bit1 = (data >> 3) & 0x01;
		bit2 = (data >> 4) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (data >> 0) & 0x01;
		bit1 = (data >> 1) & 0x01;
		b = 0x55 * bit0 + 0xaa * bit1;
		palette_set_color(i,r,g,b);
	}
}

static MACHINE_DRIVER_START( cardline )

	/* basic machine hardware */
	MDRV_CPU_ADD(I8051,12000000)
	MDRV_CPU_PROGRAM_MAP(mem_prg,0)
	MDRV_CPU_DATA_MAP(mem_data,0)
	MDRV_CPU_IO_MAP(mem_io,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_PALETTE_INIT(cardline)

	MDRV_VIDEO_START(cardline)
	MDRV_VIDEO_UPDATE(cardline)


MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cardline )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dns0401.u23",   0x0000, 0x10000, CRC(5bbaf5c1) SHA1(70972a744c5981b01a46799a7fd1b0a600489264) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "u38cll01.u38",   0x000001, 0x80000, CRC(12f62496) SHA1(b89eaf09e76c5c42588bf9c8c23190347635cc83) )
	ROM_LOAD16_BYTE( "u39cll01.u39",   0x000000, 0x80000, CRC(fcfa703e) SHA1(9230ad9df02140f3a6c38b24558548a888b23412) )

	ROM_REGION( 0x40000,  REGION_SOUND1, 0 ) // OKI samples
	ROM_LOAD( "3a.u3",   0x0000, 0x40000, CRC(9fa543c5) SHA1(a22396cb341ca4a3f0dd23719620a219c91e0e9d) )

	ROM_REGION( 0x0200,  REGION_PROMS, 0 )
	ROM_LOAD( "82s147.u33",   0x0000, 0x0200, CRC(a3b95911) SHA1(46850ea38950cdccbc2ad91d968218ac964c0eb5) )

ROM_END

GAME( 199?, cardline,  0,       cardline,  cardline,  0, ROT0, "Veltmeijer", "Card Line" , GAME_NOT_WORKING | GAME_NO_SOUND)
