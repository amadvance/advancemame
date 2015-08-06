/*

Dream World
(c)2000 SemiCom

can't make any more progress until the protection is figured out
but I suspect it will be similar to all the protection semicom
used for everything else (0x200 bytes of code/data placed in RAM
at startup by the MCU)

protection data is read via a port, scrambled a bit, and put in
main ram, the interrupt is pointed to the data in ram.


*/

#include "driver.h"

UINT32*dreamwld_bg_videoram;
UINT32*dreamwld_bg2_videoram;
UINT32*dreamwld_spriteram;
UINT32*dreamwld_mainram;

static tilemap *dreamwld_bg_tilemap;
static tilemap *dreamwld_bg2_tilemap;

/* this is the sprite format as it is stored in mainram for dreamwld */
static void dreamwld_drawsprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	const gfx_element *gfx = Machine->gfx[0];
	UINT32 *source = dreamwld_spriteram;
	UINT32 *finish = source + 0x1200/4-3;
	UINT16 *redirect = (UINT16 *)memory_region(REGION_GFX3);

//  source++;

	while( source<finish )
	{
		int xpos, ypos, tileno;//, flipx, flipy, chain, enable, number, count;
		int xsize,ysize;
		int xct,yct;
		int xflip;
		int xinc;

	//  xsize = 8;
	//  ysize = 8;

		ypos =  (source[0]&0x000001ff) >>0;
		ysize = (source[0]&0x00000e00) >>9;

		xsize = (source[1]&0x0e000000) >> 25;
		xpos =  (source[1]&0x01ff0000) >> 16;
		xpos -=16;

		xflip = (source[1]&0x00004000) >> 14;

		xinc = 16;
		if (xflip)
		{
			xinc = -16;
			xpos+=16;//*xsize;
		}

		ysize++;xsize++;

		tileno = (source[2]&0xffff0000) >> 16; // eeeh sprite list is a bit strange?
	//  tileno+=0xf00;

//      tileno=;

		for (yct=0;yct<ysize;yct++)
		{
			for (xct=0;xct<xsize;xct++)
			{
				drawgfx(bitmap,gfx,redirect[tileno],0,xflip,0,xpos+xct*xinc,ypos+yct*16,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,gfx,redirect[tileno],0,xflip,0,(xpos+xct*xinc)-0x200,ypos+yct*16,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,gfx,redirect[tileno],0,xflip,0,(xpos+xct*xinc)-0x200,(ypos+yct*16)-0x200,cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,gfx,redirect[tileno],0,xflip,0,xpos+xct*xinc,(ypos+yct*16)-0x200,cliprect,TRANSPARENCY_PEN,0);

				tileno++;
			}
		}

		source+=2;
	}
}


WRITE32_HANDLER( dreamwld_bg_videoram_w )
{
	COMBINE_DATA(&dreamwld_bg_videoram[offset]);
	tilemap_mark_tile_dirty(dreamwld_bg_tilemap,offset*2);
	tilemap_mark_tile_dirty(dreamwld_bg_tilemap,offset*2+1);

}

static void get_dreamwld_bg_tile_info(int tile_index)
{
	int tileno;
	tileno = (tile_index&1)?(dreamwld_bg_videoram[tile_index>>1]&0xffff):((dreamwld_bg_videoram[tile_index>>1]>>16)&0xffff);

	tileno &=0x1fff;
	SET_TILE_INFO(1,tileno,0,0)
}


WRITE32_HANDLER( dreamwld_bg2_videoram_w )
{
	COMBINE_DATA(&dreamwld_bg2_videoram[offset]);
	tilemap_mark_tile_dirty(dreamwld_bg2_tilemap,offset*2);
	tilemap_mark_tile_dirty(dreamwld_bg2_tilemap,offset*2+1);

}

static void get_dreamwld_bg2_tile_info(int tile_index)
{
	int tileno;
	tileno = (tile_index&1)?(dreamwld_bg2_videoram[tile_index>>1]&0xffff):((dreamwld_bg2_videoram[tile_index>>1]>>16)&0xffff);
	SET_TILE_INFO(1,tileno+0x2000,0,0)
}




VIDEO_START(dreamwld)
{
	dreamwld_bg_tilemap = tilemap_create(get_dreamwld_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,      16, 16, 64,32);
	dreamwld_bg2_tilemap = tilemap_create(get_dreamwld_bg2_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,      16, 16, 64,32);

	return 0;
}

VIDEO_UPDATE(dreamwld)
{
	tilemap_draw(bitmap,cliprect,dreamwld_bg_tilemap,0,0);
//  fillbitmap(bitmap, get_black_pen(), cliprect);

	dreamwld_drawsprites(bitmap,cliprect);

}


static READ32_HANDLER( dreamwld_random_read)
{
	return mame_rand();
}

static READ32_HANDLER( inputs_r_1 )
{
	int x;

	x= readinputport(0);

//  return x|(x<<8)|(x<<16)|(x<<24);

	logerror("Protection Read Offset %08x, Mem_mask %08x\n",offset*4, mem_mask);

	return mame_rand()^ (mame_rand()<<16);
}


static READ32_HANDLER( inputs_r_2 )
{
	int x;

	x= readinputport(1);

	return x|(x<<16);

}

static READ32_HANDLER( inputs_r_3 )
{
	int x;

	x= readinputport(2);

	return x|(x<<16);

}

static ADDRESS_MAP_START( dreamwld_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM  AM_WRITE(MWA32_NOP)

	AM_RANGE(0x400000, 0x40100f) AM_RAM  // real sprites?
	AM_RANGE(0x600000, 0x601fff) AM_RAM  // real palette?
	AM_RANGE(0x800000, 0x801fff) AM_READWRITE(MRA32_RAM, dreamwld_bg_videoram_w ) AM_BASE( &dreamwld_bg_videoram )
	AM_RANGE(0x802000, 0x803fff) AM_READWRITE(MRA32_RAM, dreamwld_bg2_videoram_w ) AM_BASE( &dreamwld_bg2_videoram )
	AM_RANGE(0x804000, 0x805fff) AM_RAM
//  AM_RANGE(0xc00000, 0xc0ffff) AM_READ(dreamwld_random_read)

	AM_RANGE(0xc00030, 0xc00033) AM_READ(inputs_r_1) // it reads protection data (irq code) from here and puts it at ffd000
	AM_RANGE(0xc00000, 0xc00003) AM_READ(inputs_r_2)
	AM_RANGE(0xc00004, 0xc00007) AM_READ(inputs_r_3)

	AM_RANGE(0xfe0000, 0xfeffff) AM_RAM AM_BASE( &dreamwld_spriteram ) // real sprite ram or work ram?  -- looks to be work ram...
//  AM_RANGE(0xff0000, 0xffffff) AM_READ(mainram_r) AM_WRITE(mainram_w)  AM_BASE( &dreamwld_mainram )
	AM_RANGE(0xff0000, 0xffcfff) AM_RAM // work ram
	/* ffd000 - ffdfff is ram too, the protection data gets copied here and the interrupt (irq6) is pointed at it */
	AM_RANGE(0xffe000, 0xffffff) AM_RAM // real sprite ram or work ram?
ADDRESS_MAP_END



INPUT_PORTS_START(dreamwld)
	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, "0" )
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

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0001, 0x0001, "1" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* 16bit */
	PORT_DIPNAME( 0x0001, 0x0001, "2" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/*
static const gfx_layout tiles8x8_layout =
{
    8,8,
    RGN_FRAC(1,1),
    4,
    { 0, 1, 2, 3 },
    { 0, 4, 8, 12, 16, 20, 24, 28 },
    { 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
    32*8
};
*/
static const gfx_layout tiles16x16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX3, 0, &tiles16x16_layout, 0, 16 },
	{ REGION_GFX4, 0, &tiles16x16_layout, 0, 16 },
	{ -1 }
};

static INTERRUPT_GEN( dreamwld_interrupt )
{
//  dreamwld_mainram
	dreamwld_spriteram[0x3606/4]=0; // the irq should do this ...
}

static MACHINE_DRIVER_START( dreamwld )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 16000000)
	MDRV_CPU_PROGRAM_MAP(dreamwld_map, 0)
	MDRV_CPU_VBLANK_INT(dreamwld_interrupt,1)
//  MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024,1024)
//  MDRV_VISIBLE_AREA(0, 512-1, 0, 512-1)
	MDRV_VISIBLE_AREA(0, 304-1, 0, 224-1)

	MDRV_PALETTE_LENGTH(256)
	MDRV_GFXDECODE(gfxdecodeinfo)


	MDRV_VIDEO_START(dreamwld)
	MDRV_VIDEO_UPDATE(dreamwld)
MACHINE_DRIVER_END

DRIVER_INIT( dreamwld )
{
	/* nothing for now ..*/
}

ROM_START( dreamwld )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "1.bin", 0x000002, 0x040000, CRC(35c94ee5) SHA1(3440a65a807622b619c97bc2a88fd7d875c26f66) )
	ROM_LOAD32_BYTE( "2.bin", 0x000003, 0x040000, CRC(5409e7fc) SHA1(2f94a6a8e4c94b36b43f0b94d58525f594339a9d) )
	ROM_LOAD32_BYTE( "3.bin", 0x000000, 0x040000, CRC(e8f7ae78) SHA1(cfd393cec6dec967c82e1131547b7e7fdc5d814f) )
	ROM_LOAD32_BYTE( "4.bin", 0x000001, 0x040000, CRC(3ef5d51b) SHA1(82a00b4ff7155f6d5553870dfd510fed9469d9b5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 87C52 MCU Code */
	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP ) /* can't be dumped. */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "6.bin", 0x000000, 0x80000, CRC(c8b91f30) SHA1(706004ca56d0a74bc7a3dfd73a21cdc09eb90f05) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 ) /* OKI Samples */
	ROM_LOAD( "5.bin", 0x000000, 0x80000, CRC(9689570a) SHA1(4414233da8f46214ca7e9022df70953922a63aa4) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* Sprite Tiles? - decoded */
	ROM_LOAD( "9.bin", 0x000000, 0x200000, CRC(fa84e3af) SHA1(5978737d348fd382f4ec004d29870656c864d137) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* Sprite Tiles? - decoded */
	ROM_LOAD( "10.bin",0x000000, 0x200000, CRC(3553e4f5) SHA1(c335494f4a12a01a88e7cd578cae922954303cfd) )

	ROM_REGION( 0x040000, REGION_GFX3, 0 ) /* Sprite Redirect ... */
	ROM_LOAD16_BYTE( "8.bin", 0x000000, 0x020000, CRC(8d570df6) SHA1(e53e4b099c64eca11d027e0083caa101fcd99959) )
	ROM_LOAD16_BYTE( "7.bin", 0x000001, 0x020000, CRC(a68bf35f) SHA1(f48540a5415a7d9723ca6e7e03cab039751dce17) )

	ROM_REGION( 0x10000, REGION_GFX4, 0 ) /* ???? - not decoded seems to be in blocks of 0x41 bytes.. */
	ROM_LOAD( "11.bin", 0x000000, 0x10000, CRC(0da8db45) SHA1(7d5bd71c5b0b28ff74c732edd7c662f46f2ab25b) )
ROM_END


GAME( 2000, dreamwld, 0,        dreamwld, dreamwld, dreamwld, ROT0,  "SemiCom", "Dream World", GAME_NOT_WORKING|GAME_NO_SOUND )
