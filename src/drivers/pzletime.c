// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Pierpaolo Prazzoli
/*
    Puzzle Time (Prototype)
    Elettronica Video-Games S.R.L, 199?

    dump and info provided by Yoshi
*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "sound/okim6295.h"
#include "machine/eeprom.h"
#include "machine/ticket.h"
#include "vidhrdw/generic.h"

static UINT16 *bg_videoram, *mid_videoram, *txt_videoram, *tilemap_regs, *video_regs;
static tilemap *mid_tilemap, *txt_tilemap;

static void get_mid_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = mid_videoram[tile_index] & 0x0fff;
	colour = mid_videoram[tile_index] & 0xf000;
	colour = colour >> 12;
	SET_TILE_INFO(2,tileno,colour,0);
}

static void get_txt_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = txt_videoram[tile_index] & 0x0fff;
	colour = txt_videoram[tile_index] & 0xf000;
	colour = colour >> 12;

	SET_TILE_INFO(0,tileno,colour,0);

	tile_info.priority = (colour & 8) ? 1 : 0;
}

static VIDEO_START( pzletime )
{
	mid_tilemap = tilemap_create(get_mid_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT, 16,16,64,16);
	txt_tilemap = tilemap_create(get_txt_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8, 8,64,32);

	tilemap_set_transparent_pen(mid_tilemap,0);
	tilemap_set_transparent_pen(txt_tilemap,0);
	return 0;
}

static VIDEO_UPDATE( pzletime )
{
	int count;
	int y,x;

	fillbitmap(bitmap, Machine->pens[0], cliprect);

	tilemap_set_scrolly(txt_tilemap, 0, tilemap_regs[0]-3);
	tilemap_set_scrollx(txt_tilemap, 0, tilemap_regs[1]);

	tilemap_set_scrolly(mid_tilemap, 0, tilemap_regs[2]-3);
	tilemap_set_scrollx(mid_tilemap, 0, tilemap_regs[3]-7);

	if(video_regs[2] & 1)
	{
		count = 0;

		for(y=255;y>=0;y--)
		{
			for(x=0;x<512;x++)
			{
				if(bg_videoram[count] & 0x8000)
				{
					int sy = (y - 18) & 0xff;
					int sx = (x - 32) & 0x1ff;
					if (sx >= cliprect->min_x && sx <= cliprect->max_x && sy >= cliprect->min_y && sy <= cliprect->max_y)
						plot_pixel(bitmap, sx, sy, Machine->pens[0x300 + (bg_videoram[count] & 0x7fff)]);
				}

				count++;
			}
		}
	}

	tilemap_draw(bitmap,cliprect,mid_tilemap, 0,0);

	{
		int offs,spr_offs,colour,sx,sy;

		for(offs = 0; offs < 0x2000/2; offs += 4)
		{
			if(spriteram16[offs+0] == 8)
				break;

			spr_offs = spriteram16[offs+3] & 0x0fff;
			sy = 0x200-(spriteram16[offs+0] & 0x1ff)-35;
			sx = (spriteram16[offs+1] & 0x1ff)-30;
			colour = (spriteram16[offs+0] & 0xf000)>>12;

			drawgfx(bitmap,Machine->gfx[1],spr_offs,colour,0,1,sx,sy,cliprect,TRANSPARENCY_PEN,0);
		}
	}

	tilemap_draw(bitmap,cliprect,txt_tilemap,0,0);
	if((cpu_getcurrentframe() & 8) != 0)
		tilemap_draw(bitmap,cliprect,txt_tilemap,1,0);

}

static WRITE16_HANDLER( mid_videoram_w )
{
	COMBINE_DATA(&mid_videoram[offset]);
	tilemap_mark_tile_dirty(mid_tilemap,offset);
}

static WRITE16_HANDLER( txt_videoram_w )
{
	COMBINE_DATA(&txt_videoram[offset]);
	tilemap_mark_tile_dirty(txt_tilemap,offset);
}

static WRITE16_HANDLER( eeprom_w )
{
	if( ACCESSING_LSB )
	{
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE );
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE );
	}
}

static WRITE16_HANDLER( ticket_w )
{
	if( ACCESSING_LSB )
	{
		ticket_dispenser_w(0, (data & 1) << 7);
	}
}

static WRITE16_HANDLER( video_regs_w )
{
	int i;

	COMBINE_DATA(&video_regs[offset]);

	if(offset == 0)
	{
		if(video_regs[0] > 0)
		{
			for (i=0;i<0x300;i++)
			{
				palette_set_brightness( i, (double)0x8000/(double)video_regs[0]);
			}
		}
	}
	else if(offset == 1)
	{
		if(video_regs[1] > 0)
		{
			for (i=0x300;i<32768 + 0x300;i++)
			{
				palette_set_brightness( i, (double)0x8000/(double)video_regs[1]);
			}
		}
	}
}

static WRITE16_HANDLER( oki_bank_w )
{
	OKIM6295_set_bank_base(0, 0x40000 * (data & 0x3));
}

static READ16_HANDLER( system_r )
{
	return (readinputport(0) & ~0xc0) | (EEPROM_read_bit() << 6) | (ticket_dispenser_r(0) & 0x80);
}

static NVRAM_HANDLER( pzletime )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);
		if (file)
			EEPROM_load(file);
		else
			/* a blank EEPROM cannot pass the factory calibration checks */
			EEPROM_set_data(memory_region(REGION_USER2), 0x80);
	}
}

static MACHINE_START( pzletime )
{
	ticket_dispenser_init(2000, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);
	return 0;
}

static MACHINE_RESET( pzletime )
{
	ticket_dispenser_w(0, 0);
	OKIM6295_set_bank_base(0, 0);
}

static ADDRESS_MAP_START( pzletime_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_ROM
	AM_RANGE(0x700000, 0x700005) AM_RAM AM_WRITE(video_regs_w) AM_BASE(&video_regs)
	AM_RANGE(0x800000, 0x800001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x900000, 0x9005ff) AM_RAM AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xa00000, 0xa00007) AM_RAM AM_BASE(&tilemap_regs)
	AM_RANGE(0xb00000, 0xb3ffff) AM_RAM AM_BASE(&bg_videoram)
	AM_RANGE(0xc00000, 0xc00fff) AM_RAM AM_WRITE(mid_videoram_w) AM_BASE(&mid_videoram)
	AM_RANGE(0xc01000, 0xc01fff) AM_RAM AM_WRITE(txt_videoram_w) AM_BASE(&txt_videoram)
	AM_RANGE(0xd00000, 0xd01fff) AM_RAM AM_BASE(&spriteram16)
	AM_RANGE(0xe00000, 0xe00001) AM_READ(input_port_1_word_r) AM_WRITE(eeprom_w)
	AM_RANGE(0xe00002, 0xe00003) AM_READ(system_r) AM_WRITE(ticket_w)
	AM_RANGE(0xe00004, 0xe00005) AM_WRITE(oki_bank_w)
	AM_RANGE(0xf00000, 0xf0ffff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( pzletime )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x0004, IP_ACTIVE_LOW )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("INPUT")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

static const gfx_layout layout8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout layout16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 16*32+4, 16*32+0, 16*32+12, 16*32+8, 16*32+20, 16*32+16, 16*32+28, 16*32+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout8x8, 0x100, 0x10 },
	{ REGION_GFX2, 0, &layout16x16, 0x200, 0x10 },
	{ REGION_GFX3, 0, &layout16x16, 0x000, 0x10 },
	{ -1 }
};

static PALETTE_INIT( pzletime )
{
	int i;

	for (i = 0;i < 32768;i++)
		palette_set_color(i+0x300,pal5bit(i >> 10),pal5bit(i >> 5),pal5bit(i >> 0));
}

static MACHINE_DRIVER_START( pzletime )

	MDRV_CPU_ADD(M68000,10000000)
	MDRV_CPU_PROGRAM_MAP(pzletime_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x300 + 32768)
	MDRV_NVRAM_HANDLER(pzletime)
	MDRV_MACHINE_START(pzletime)
	MDRV_MACHINE_RESET(pzletime)

	MDRV_PALETTE_INIT(pzletime)
	MDRV_VIDEO_START(pzletime)
	MDRV_VIDEO_UPDATE(pzletime)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 937500 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( pzletime )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "5.bin", 0x000000, 0x80000, CRC(78b027dc) SHA1(6719908a075ecf0666bb817ac8a31056a7f315c6) )
	ROM_LOAD16_BYTE( "1.bin", 0x000001, 0x80000, CRC(0a69cbc7) SHA1(bae8b5746209c6773da27acaec7bd535a69019d2) )
	ROM_LOAD16_BYTE( "6.bin", 0x100000, 0x80000, CRC(526733ef) SHA1(21a921416d1ae7b9d49789d70ae99f240b012489) )
	ROM_LOAD16_BYTE( "2.bin", 0x100001, 0x80000, CRC(2a877266) SHA1(b8e909b3bd21af71782c501fa6eef590045b81e0) )
	ROM_LOAD16_BYTE( "7.bin", 0x200000, 0x80000, CRC(2efdd6d3) SHA1(de35d7a1bcd3ad608b8dfc184e06d6719253a1c7) )
	ROM_LOAD16_BYTE( "3.bin", 0x200001, 0x80000, CRC(1ddacade) SHA1(78f09fdb541e369765abfdf39607ca8f4c771d16) )
	ROM_LOAD16_BYTE( "8.bin", 0x300000, 0x80000, CRC(be7cf043) SHA1(5dadafb6f89f2fc373b77b18746b461117228f08) )
	ROM_LOAD16_BYTE( "4.bin", 0x300001, 0x80000, CRC(374ab900) SHA1(bd7f649bdf2927c1f5cb53492a08cc66c4658a72) )

	ROM_REGION( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "12.bin",  0x00000, 0x80000,  CRC(203897c1) SHA1(c2495871c796bc7f2dabca1630317313b5aa740a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_COPY( REGION_USER1, 0x000000, 0x000000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x000000, 0x020000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x000000, 0x040000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x020000, 0x060000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x000000, 0x080000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x040000, 0x0a0000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x000000, 0x0c0000, 0x020000 )
	ROM_COPY( REGION_USER1, 0x060000, 0x0e0000, 0x020000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "10.bin",  0x00000, 0x80000, CRC(d6ed11a5) SHA1(585aad4e962e7c9ba33e96d4d53e2feddd1a6cd9) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11.bin",  0x000000, 0x80000, CRC(566e09a3) SHA1(b04d23bd82c609f35e6b651006b5c029f36f54dc) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "9.bin",   0x000000, 0x80000, CRC(a8144a7e) SHA1(9dfdd6c17a91cad6b56c622671042ac2ee2c9ec8) )
	ROM_REGION( 0x80, REGION_USER2, 0 )
	ROM_LOAD( "pzletime.nv", 0, 0x80, CRC(e5ed3d40) SHA1(8c163a6e5839e5c82d52f046d3268202fdf9f4d1) )
ROM_END

GAME( 199?, pzletime, 0, pzletime,  pzletime,  0, ROT0, "Elettronica Video-Games", "Puzzle Time (prototype)", 0 )
