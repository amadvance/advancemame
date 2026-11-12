// license:BSD-3-Clause
// copyright-holders:Luca Elia
/*************************************************************************************************************

                                        -= Andamiro's Midas hardware =-

                                            driver by Luca Elia

    a reengineered Neo-Geo, with a few differences: no Z80, better sound chip, serial eeprom and 256 color tiles.
    Plus a PIC12C508A microcontroller, probably for the protection checks (I've patched them out for now).

    Hardware description:

    http://web.archive.org/web/20041018094226/http://www.andamiro.com/kor/business/hard_05.html

    CPU         MC68000

    VRAM        256kbyte (4Display/Access bank)

    PaletteRAM  96kbyte

    Display     320(x)*224(y)

    Sprite      16(x)*240(y(max))*380(max) (96 sprite/line(max))
                128 level y-axis scaling (or line control effect)
                16 level x-axis scale-down x,y fip
                255color/sprite(of 256 palette set)

    Text        8dot*8dot, 40(x)*28(y),
                255color/text(of 16 palette set)

    Color       32640 of 24bit True Color
                (255 colors/sprite)

    Sound       8 channel 44.1KHz(max) stereo
                4bit ADPCM, 8bit PCM, 16bit PCM

    Controller  4 direction,
                6 button joystick * 2 player (max. 4 playersupport)
                light-gun*2 player
                trackball*2 player

    Maximum ROM 2Gbit

    size  220(x)mm * 210(y)mm


    Notes:

    - hammer: keep test button pressed during boot for hardware tests

*************************************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "sound/ymz280b.h"

static UINT16 *midas_gfxram, *midas_gfxregs;

static tilemap *tmap;
static int midas_has_gun;

static void get_tile_info(int tile_index)
{
	UINT16 code = midas_gfxram[tile_index + 0x7000];
	SET_TILE_INFO(1, code & 0xfff, (code >> 12) & 0xf, 0)
}

static VIDEO_START( midas )
{
	midas_gfxram = auto_malloc(0x20000);

	tmap = tilemap_create(get_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8, 8, 0x80, 0x20);
	if (!tmap)
		return 1;

	tilemap_set_transparent_pen(tmap, 0);

	return 0;
}

static VIDEO_START( hammer )
{
	midas_has_gun = 1;

	return video_start_midas();
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	UINT16 *s = midas_gfxram + 0x8000;
	UINT16 *codes = midas_gfxram;

	int sx_old = 0, sy_old = 0, ynum_old = 0, xzoom_old = 0;
	int xdim, ydim, xscale, yscale;
	int i, y;

	for (i = 0; i < 0x180; ++i, ++s, codes += 0x40)
	{
		int zoom = s[0x000];
		int sy = s[0x200];
		int sx = s[0x400];

		int xzoom = ((zoom >> 8) & 0x0f) + 1;
		int yzoom = ((zoom >> 0) & 0x7f) + 1;

		int ynum;

		if (sy & 0x40)
		{
			ynum = ynum_old;

			sx = sx_old + xzoom_old;
			sy = sy_old;

			if (sx >= 0x1f0)
				sx -= 0x200;
		}
		else
		{
			ynum = sy & 0x3f;

			sx = (sx >> 7);
			sy = 0x200 - (sy >> 7);

			if (sx >= 0x1f0)
				sx -= 0x200;

			if (ynum > 0x20)
				ynum = 0x20;
		}

		ynum_old = ynum;
		sx_old = sx;
		sy_old = sy;
		xzoom_old = xzoom;

		/* fixed point values (16.16), for accuracy */
		sx <<= 16;
		sy <<= 16;

		xdim = (xzoom << 16) * 16 / 16;
		ydim = (yzoom << 16) * 16 / 0x80;

		xscale = xdim / 16;
		yscale = ydim / 16;

		/* round up the scale to avoid holes between tiles */
		if (xscale & 0xffff)
			xscale += (1 << 16) / 16;
		if (yscale & 0xffff)
			yscale += (1 << 16) / 16;

		for (y = 0; y < ynum; ++y)
		{
			UINT16 code = codes[y * 2];
			UINT16 attr = codes[y * 2 + 1];

			drawgfxzoom(bitmap, Machine->gfx[0],
					code,
					attr >> 8,
					attr & 1, attr & 2,
					sx / 0x10000, ((sy + y * ydim) / 0x10000) & 0x1ff,
					cliprect, TRANSPARENCY_PEN, 0,
					xscale, yscale);
		}
	}
}

static VIDEO_UPDATE( midas )
{
	fillbitmap(bitmap, 4095, cliprect);

	draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, tmap, 0, 0);

	/* the sensor ranges are 0x40 x 0x30 over the 320 x 224 visible area */
	if (midas_has_gun)
		draw_crosshair(bitmap, readinputport(6) * 320 / 0x40, 16 + readinputport(7) * 224 / 0x30, cliprect, 0);
}

static WRITE16_HANDLER( midas_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		/* latch the bit */
		EEPROM_write_bit(data & 0x04);

		/* reset line asserted: reset */
		EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE);

		/* clock line asserted: write latch or select next bit to read */
		EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static NVRAM_HANDLER( midas )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file)
			EEPROM_load(file);
	}
}

static READ16_HANDLER( ret_ffff )
{
	return 0xffff;
}

static READ16_HANDLER( midas_service_r )
{
	return readinputport(1) | (EEPROM_read_bit() << 3);
}

static WRITE16_HANDLER( midas_gfxregs_w )
{
	COMBINE_DATA(midas_gfxregs + offset);

	switch (offset)
	{
		case 1:
		{
			UINT16 addr = midas_gfxregs[0];
			midas_gfxram[addr] = data;
			midas_gfxregs[0] += midas_gfxregs[2];

			if (addr >= 0x7000 && addr <= 0x7fff)
				tilemap_mark_tile_dirty(tmap, addr - 0x7000);

			break;
		}
	}
}

/***************************************************************************************
                                       Live Quiz Show
***************************************************************************************/

static WRITE16_HANDLER( livequiz_coin_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(0, data & 0x0001);
}

static ADDRESS_MAP_START( livequiz_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM

	AM_RANGE(0x900000, 0x900001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x920000, 0x920001) AM_READ(midas_service_r)
	AM_RANGE(0x940000, 0x940001) AM_READ(input_port_2_word_r)
	AM_RANGE(0x980000, 0x980001) AM_READ(input_port_3_word_r)

	AM_RANGE(0x980000, 0x980001) AM_WRITE(livequiz_coin_w)

	AM_RANGE(0x9a0000, 0x9a0001) AM_WRITE(midas_eeprom_w)

	AM_RANGE(0x9c0000, 0x9c0005) AM_WRITE(midas_gfxregs_w) AM_BASE(&midas_gfxregs)

	AM_RANGE(0xa00000, 0xa3ffff) AM_READWRITE(MRA16_RAM, paletteram16_xrgb_word_be_w) AM_BASE(&paletteram16)
	AM_RANGE(0xa40000, 0xa7ffff) AM_RAM

	AM_RANGE(0xb00000, 0xb00001) AM_READ(ret_ffff)
	AM_RANGE(0xb20000, 0xb20001) AM_READ(ret_ffff)
	AM_RANGE(0xb40000, 0xb40001) AM_READ(ret_ffff)
	AM_RANGE(0xb60000, 0xb60001) AM_READ(ret_ffff)

	AM_RANGE(0xb80008, 0xb80009) AM_WRITE(YMZ280B_register_0_lsb_w)
	AM_RANGE(0xb8000a, 0xb8000b) AM_READWRITE(YMZ280B_status_0_lsb_r, YMZ280B_data_0_lsb_w)

	AM_RANGE(0xba0000, 0xba0001) AM_READ(input_port_4_word_r)
	AM_RANGE(0xbc0000, 0xbc0001) AM_READ(input_port_5_word_r)

	AM_RANGE(0xd00000, 0xd1ffff) AM_RAM	/* zoom table? */

	AM_RANGE(0xe00000, 0xe3ffff) AM_RAM
ADDRESS_MAP_END

/***************************************************************************************
                                          Hammer
***************************************************************************************/

/* the three dispensers (two prize ones and a ticket one) are driven by the same motor/status protocol */
#define DISPENSER_PRIZE1 0
#define DISPENSER_PRIZE2 1
#define DISPENSER_TICKET 2

struct dispenser_state
{
	UINT8 status;
	UINT8 power;
	mame_timer *timer;
};

static struct dispenser_state dispenser[3];
static const int dispenser_msec[3] = { 1000 * 5, 1000 * 5, 200 };

static void dispenser_toggle(int which)
{
	/* while powered, the status line toggles at every interval */
	if (dispenser[which].power)
	{
		dispenser[which].status ^= 1;
		timer_adjust(dispenser[which].timer, TIME_IN_MSEC(dispenser_msec[which]), which, 0);
	}
}

static void dispenser_motor(int which, int on)
{
	struct dispenser_state *d = &dispenser[which];

	if (on)
	{
		if (!d->power)
		{
			d->power = 1;
			d->status = 1;
			timer_adjust(d->timer, TIME_IN_MSEC(dispenser_msec[which]), which, 0);
		}
	}
	else
	{
		if (d->power)
		{
			d->power = 0;
			timer_adjust(d->timer, TIME_NEVER, 0, 0);
		}
	}
}

static MACHINE_START( hammer )
{
	int i;

	for (i = 0; i < 3; ++i)
	{
		dispenser[i].status = 1;
		dispenser[i].power = 0;
		dispenser[i].timer = timer_alloc(dispenser_toggle);
	}

	return 0;
}

static READ16_HANDLER( hammer_sensor_r )
{
	if (readinputport(5) & 0x80)
		return 0xffff;

	return (readinputport(7) << 8) | readinputport(6);
}

static READ16_HANDLER( hammer_port_r )
{
	return readinputport(5)
		| (dispenser[DISPENSER_PRIZE1].status << 0)
		| (dispenser[DISPENSER_PRIZE2].status << 1)
		| (dispenser[DISPENSER_TICKET].status << 4);
}

static WRITE16_HANDLER( hammer_coin_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, data & 0x0001);
		coin_counter_w(1, data & 0x0002);
	}
}

static WRITE16_HANDLER( hammer_motor_w )
{
	if (ACCESSING_LSB)
	{
		dispenser_motor(DISPENSER_PRIZE1, data & 0x0001);
		dispenser_motor(DISPENSER_PRIZE2, data & 0x0002);
		dispenser_motor(DISPENSER_TICKET, data & 0x0010);
	}
}

static ADDRESS_MAP_START( hammer_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM

	AM_RANGE(0x900000, 0x900001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x920000, 0x920001) AM_READ(midas_service_r)
	AM_RANGE(0x940000, 0x940001) AM_READ(input_port_2_word_r)
	AM_RANGE(0x980000, 0x980001) AM_READ(input_port_3_word_r)

	AM_RANGE(0x980000, 0x980001) AM_WRITE(hammer_coin_w)
	AM_RANGE(0x9c000c, 0x9c000d) AM_WRITENOP	/* IRQ Ack */
	AM_RANGE(0x9c000e, 0x9c000f) AM_WRITENOP	/* LED */

	AM_RANGE(0x9a0000, 0x9a0001) AM_WRITE(midas_eeprom_w)

	AM_RANGE(0x9c0000, 0x9c0005) AM_WRITE(midas_gfxregs_w) AM_BASE(&midas_gfxregs)

	AM_RANGE(0xa00000, 0xa3ffff) AM_READWRITE(MRA16_RAM, paletteram16_xrgb_word_be_w) AM_BASE(&paletteram16)
	AM_RANGE(0xa40000, 0xa7ffff) AM_RAM

	AM_RANGE(0xb00000, 0xb00001) AM_READ(ret_ffff)
	AM_RANGE(0xb20000, 0xb20001) AM_READ(ret_ffff)
	AM_RANGE(0xb40000, 0xb40001) AM_READ(ret_ffff)
	AM_RANGE(0xb60000, 0xb60001) AM_READ(ret_ffff)

	AM_RANGE(0xb80008, 0xb80009) AM_WRITE(YMZ280B_register_0_lsb_w)
	AM_RANGE(0xb8000a, 0xb8000b) AM_READWRITE(YMZ280B_status_0_lsb_r, YMZ280B_data_0_lsb_w)

	AM_RANGE(0xba0000, 0xba0001) AM_READ(input_port_4_word_r)
	AM_RANGE(0xbc0000, 0xbc0001) AM_READ(hammer_port_r)

	AM_RANGE(0xbc0002, 0xbc0003) AM_WRITE(hammer_motor_w)

	AM_RANGE(0xbc0004, 0xbc0005) AM_READ(hammer_sensor_r)

	AM_RANGE(0xd00000, 0xd1ffff) AM_RAM	/* zoom table? */

	AM_RANGE(0xe00000, 0xe3ffff) AM_RAM
ADDRESS_MAP_END

static gfx_layout layout16x16x8 =
{
	16,16,
	RGN_FRAC(1,4),
	8,
	{
		RGN_FRAC(3,4)+8,RGN_FRAC(3,4)+0,
		RGN_FRAC(2,4)+8,RGN_FRAC(2,4)+0,
		RGN_FRAC(1,4)+8,RGN_FRAC(1,4)+0,
		RGN_FRAC(0,4)+8,RGN_FRAC(0,4)+0
	},
	{ STEP8(8*16*2+7,-1), STEP8(7,-1) },
	{ STEP16(0,8*2) },
	16*16*2
};

static gfx_layout layout8x8x8_2 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 8,9,10,11, 0,1,2,3 },
	{ (32*2+1)*4, 32*2*4, (48*2+1)*4, 48*2*4, (0+1)*4, 0*4, (16*2+1)*4, 16*2*4 },
	{ 0*8*2, 1*8*2, 2*8*2, 3*8*2, 4*8*2, 5*8*2, 6*8*2, 7*8*2 },
	32*8*2
};

static gfx_decode midas_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout16x16x8, 0, 0x100 },
	{ REGION_GFX2, 0, &layout8x8x8_2, 0, 0x80 },
	{ -1 }
};

INPUT_PORTS_START( livequiz )

	PORT_START	// 900000
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 920000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL )	// EEPROM
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x0040,   IP_ACTIVE_LOW )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 940000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 980000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// ba0000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3  )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// bc0000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

INPUT_PORTS_START( hammer )

	PORT_START	// 900000
	PORT_DIPNAME( 0x01, 0x01, "Debug Mode" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, "Game Mode" )
	PORT_DIPSETTING(	0x06, "Prize Game" )
	PORT_DIPSETTING(	0x00, "Ticket Game 1" )	// not in manual, does not work (it requires a toggleable ticket dispenser)
	PORT_DIPSETTING(	0x04, "Ticket Game 2" )
	PORT_DIPSETTING(	0x02, "Generic Game" )
	PORT_DIPNAME( 0x08, 0x08, "Warning Sound" )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 920000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_COIN1     )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_COIN2     )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_SERVICE1  )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_SPECIAL   )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_UNKNOWN   )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN   )
	PORT_SERVICE_NO_TOGGLE( 0x0040,   IP_ACTIVE_LOW )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN   )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 940000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// 980000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_TILT    )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// ba0000
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// bc0000
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_SPECIAL )	// prize 1 sensor ("tejisw 1")
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_SPECIAL )	// prize 2 sensor ("tejisw 2")
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_IMPULSE(5)	PORT_NAME( "Hammer" )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, 0x20, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00, 0x3f+1) PORT_SENSITIVITY(35) PORT_KEYDELTA(8)

	PORT_START
	PORT_BIT( 0xff, 0x18, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00, 0x2f+1) PORT_SENSITIVITY(35) PORT_KEYDELTA(8)

INPUT_PORTS_END

static void ymz_irq(int state)
{
}

static struct YMZ280Binterface ymz280b_interface =
{
	REGION_SOUND1,
	ymz_irq
};

static MACHINE_DRIVER_START( livequiz )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 24000000 / 2)
	MDRV_CPU_PROGRAM_MAP(livequiz_map, 0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_NVRAM_HANDLER(midas)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 16, 256-16-1)
	MDRV_GFXDECODE(midas_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(midas)
	MDRV_VIDEO_UPDATE(midas)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hammer )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 28000000 / 2)
	MDRV_CPU_PROGRAM_MAP(hammer_map, 0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_START(hammer)
	MDRV_NVRAM_HANDLER(midas)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 16, 256-16-1)
	MDRV_GFXDECODE(midas_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(hammer)
	MDRV_VIDEO_UPDATE(midas)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)
MACHINE_DRIVER_END

ROM_START( livequiz )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "flash.u1", 0x000000, 0x200000, CRC(8ec44493) SHA1(a987886cb87ac0a744f01f2e4a7cc6d12efeaa04) )

	ROM_REGION( 0x400, REGION_USER1, 0 )	/* PIC12C508A */
	ROM_LOAD( "main_pic12c508a.u27", 0x000000, 0x000400, CRC(a84f0a7e) SHA1(fb27c05fb27b98ca603697e1be214dc6c8d5f884) )
	ROM_LOAD( "sub_pic12c508a.u4",   0x000000, 0x000400, CRC(e52ebdc4) SHA1(0f3af66b5ea184e49188e74a873699324a3930f1) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD( "flash.u15", 0x000000, 0x200000, CRC(d6eb56f1) SHA1(52d67bb25dd968c79eccb05159a578516b27e557) )
	ROM_LOAD( "flash.u19", 0x200000, 0x200000, CRC(daa81532) SHA1(9e66bb4639b92c3d76b7918535f55883f22f24b2) )
	ROM_LOAD( "flash.u16", 0x400000, 0x200000, CRC(4c9fd873) SHA1(6e185304ce29771265d3c48b0ef0e840d8bed02d) )
	ROM_LOAD( "flash.u20", 0x600000, 0x200000, CRC(b540a8c7) SHA1(25b9b30c7d5ff1e410ea30580017e45590542561) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 )
	ROM_LOAD( "27c4096.u23", 0x000000, 0x080000, CRC(25121de8) SHA1(edf24d87551639b871baf3243b452a4e2ba84107) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "flash.u5", 0x000000, 0x200000, CRC(dc062792) SHA1(ec415c918c47ce9d181f014cde317af5717600e4) )
ROM_END

static DRIVER_INIT( livequiz )
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);

	/* protection checks */
	rom[0x13345a / 2] = 0x4e75;
}

ROM_START( hammer )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "p.u22", 0x000000, 0x200000, CRC(687f1596) SHA1(3dc5fb0af1e8c4f3a42ce4aad39635b1111831d8) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD( "a0l.u44", 0x000000, 0x200000, CRC(b9cafd81) SHA1(24698970d1aea0907e2963c872ce61077f44c3af) )
	ROM_LOAD( "a0h.u46", 0x200000, 0x200000, CRC(f60f188b) SHA1(486f26c473b46efb402662b2f374e361cd7b4284) )

	ROM_LOAD( "a1l.u48", 0x400000, 0x200000, CRC(82129cf9) SHA1(6d68e943854bc9e8ea555bf03107dc9e836ca4d9) )
	ROM_LOAD( "a1h.u50", 0x600000, 0x200000, CRC(76897c90) SHA1(aded60d3db834598cd54ad9140eee7be4129cb27) )

	ROM_LOAD( "a2l.u45", 0x800000, 0x200000, CRC(d8086ee5) SHA1(9d5f2b3a0f903a69cfd1108ddf5ea61b571c3fe3) )
	ROM_LOAD( "a2h.u47", 0xa00000, 0x200000, CRC(a64aa2df) SHA1(7e4eb049cd6a5971a455488a484f225763921614) )

	ROM_LOAD( "a3l.u49", 0xc00000, 0x200000, CRC(4e83cf00) SHA1(e66a0b4eae0f46bf36126be3795cfac3ad3d4282) )
	ROM_LOAD( "a3h.u51", 0xe00000, 0x200000, CRC(834de39f) SHA1(6e9867180ca20e64f60bad5cad82674ce8f45b7b) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_ERASE00 )
	/* the tiles rom of livequiz is not present in this set */

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "s0.u25", 0x000000, 0x200000, CRC(c049a3e0) SHA1(0c7016c3128c170a84ad3f92fad1165775210e3d) )
	ROM_LOAD( "s1.u26", 0x200000, 0x200000, CRC(9cc4b3ec) SHA1(b91a8747074a1032eb7f70a015d394fe8e896d7e) )
ROM_END

GAME( 1999, livequiz, 0, livequiz, livequiz, livequiz, ROT0, "Andamiro", "Live Quiz Show", GAME_IMPERFECT_GRAPHICS )
GAME( 2000, hammer,   0, hammer,   hammer,   0,        ROT0, "Andamiro", "Hammer",         0 )
