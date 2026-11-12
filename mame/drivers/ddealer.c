// license:BSD-3-Clause
// copyright-holders:Angelo Salese, David Haywood
/*********************************************************************************************************

    Double Dealer (c)NMK 1991

    driver by Angelo Salese & David Haywood, based on early work by Tomasz Slanina

    Appears to be a down-grade of the nmk16 HW

    pcb marked GD91071

    68000P10
    YM2203C
    91071-3 (Mask ROM)
    NMK-110 8131
    NMK 901
    NMK 902
    NMK 903 x2
    82S135N ("5")
    82S129N ("6")
    xtals 16.000 MHz and 6.000 MHz
    DSW x2

*********************************************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tlcs90/tlcs90.h"
#include "sound/2203intf.h"

static UINT16 *ddealer_vregs;
static UINT16 *ddealer_back_vram;
static UINT16 *ddealer_fg_vram;
static UINT16 *ddealer_work_ram;

static tilemap *back_tilemap;
static tilemap *fg_tilemap;
static tilemap *fg_tilemap_left;
static tilemap *fg_tilemap_right;

static UINT8 bus_status;

static MACHINE_RESET( ddealer )
{
	bus_status = 0x04;
}

/* MCU */

static READ8_HANDLER( mcu_port5_r )
{
	return cpu_getscanline() >> 2;
}

static READ8_HANDLER( mcu_port6_r )
{
	bus_status ^= 0x04;
	return bus_status;
}

static READ8_HANDLER( mcu_side_shared_lo_r )
{
	return cpunum_read_byte(0, 0x004000 + offset);
}

static WRITE8_HANDLER( mcu_side_shared_lo_w )
{
	cpunum_write_byte(0, 0x004000 + offset, data);
}

static READ8_HANDLER( mcu_side_shared_hi_r )
{
	return cpunum_read_byte(0, 0x010000 + offset);
}

static WRITE8_HANDLER( mcu_side_shared_hi_w )
{
	cpunum_write_byte(0, 0x010000 + offset, data);
}

/* the work RAM is accessed directly, as switching context to the halted 68000 for every access is unreliable */
static READ8_HANDLER( mcu_side_ram_r )
{
	return ((UINT8 *)ddealer_work_ram)[BYTE_XOR_BE(offset)];
}

static WRITE8_HANDLER( mcu_side_ram_w )
{
	((UINT8 *)ddealer_work_ram)[BYTE_XOR_BE(offset)] = data;

	/* the 68000 may be executing from this RAM: drop its prefetched word so the new code is seen */
	cpunum_set_info_int(0, CPUINFO_INT_REGISTER + M68K_PREF_ADDR, 0xffffffff);
}

/* the internal ROM, RAM and registers of the MCU hide part of the 68000 address space */
static ADDRESS_MAP_START( prot_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x004000, 0x00fdbf) AM_READWRITE(mcu_side_shared_lo_r, mcu_side_shared_lo_w)
	AM_RANGE(0x010000, 0x0effff) AM_READWRITE(mcu_side_shared_hi_r, mcu_side_shared_hi_w)
	AM_RANGE(0x0f0000, 0x0fffff) AM_READWRITE(mcu_side_ram_r, mcu_side_ram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( prot_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(T90_P5, T90_P5) AM_READ(mcu_port5_r)
	AM_RANGE(T90_P6, T90_P6) AM_READ(mcu_port6_r)
ADDRESS_MAP_END

/* video */

static WRITE16_HANDLER( flipscreen_w )
{
	flip_screen_set(data & 0x01);
}

static void get_back_tile_info(int tile_index)
{
	int code = ddealer_back_vram[tile_index];
	SET_TILE_INFO(0, code & 0xfff, code >> 12, 0)
}

static void get_fg_tile_info(int tile_index)
{
	int code = ddealer_fg_vram[tile_index];
	SET_TILE_INFO(1, code & 0xfff, code >> 12, 0)
}

static void get_fg_left_tile_info(int tile_index)
{
	int code = ddealer_fg_vram[tile_index & 0x17ff];
	SET_TILE_INFO(1, code & 0xfff, code >> 12, 0)
}

static void get_fg_right_tile_info(int tile_index)
{
	int code = ddealer_fg_vram[0x800 + (tile_index & 0x17ff)];
	SET_TILE_INFO(1, code & 0xfff, code >> 12, 0)
}

static UINT32 scan_fg(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	return (row & 0x0f) | ((col & 0xff) << 4) | ((row & 0x10) << 8);
}

static VIDEO_START( ddealer )
{
	back_tilemap = tilemap_create(get_back_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 8, 8, 64, 32);
	fg_tilemap = tilemap_create(get_fg_tile_info, scan_fg, TILEMAP_TRANSPARENT, 16, 16, 256, 32);
	fg_tilemap_left = tilemap_create(get_fg_left_tile_info, scan_fg, TILEMAP_TRANSPARENT, 16, 16, 128, 32);
	fg_tilemap_right = tilemap_create(get_fg_right_tile_info, scan_fg, TILEMAP_TRANSPARENT, 16, 16, 128, 32);

	if (!back_tilemap || !fg_tilemap || !fg_tilemap_left || !fg_tilemap_right)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 15);
	tilemap_set_transparent_pen(fg_tilemap_left, 15);
	tilemap_set_transparent_pen(fg_tilemap_right, 15);

	tilemap_set_scrolldx(back_tilemap, 64, 64);
	tilemap_set_scrolldy(back_tilemap, -16, -16);
	tilemap_set_scrolldx(fg_tilemap, 64, 64);
	tilemap_set_scrolldy(fg_tilemap, -16, -16);
	tilemap_set_scrolldx(fg_tilemap_left, 64, 64);
	tilemap_set_scrolldy(fg_tilemap_left, -16, -16);
	tilemap_set_scrolldx(fg_tilemap_right, 64, 64);
	tilemap_set_scrolldy(fg_tilemap_right, -16, -16);

	return 0;
}

static void draw_video_layer(mame_bitmap *bitmap, const rectangle *cliprect, const UINT16 *vreg_base, tilemap *tmap)
{
	int sx, sy;

	sx = (vreg_base[0x4 / 2] & 0xff) | ((vreg_base[0x2 / 2] & 0xff) << 8);
	sy = (vreg_base[0x8 / 2] & 0xff) | ((vreg_base[0x6 / 2] & 0xff) << 8);

	tilemap_set_scrollx(tmap, 0, sx);
	tilemap_set_scrolly(tmap, 0, sy);
	tilemap_draw(bitmap, cliprect, tmap, 0, 0);
}

static VIDEO_UPDATE( ddealer )
{
	tilemap_draw(bitmap, cliprect, back_tilemap, 0, 0);

	if (!flip_screen)
	{
		if (ddealer_vregs[0xcc / 2] & 0x80)
		{
			draw_video_layer(bitmap, cliprect, &ddealer_vregs[0x1e0 / 2], fg_tilemap_left);
			draw_video_layer(bitmap, cliprect, &ddealer_vregs[0xcc / 2], fg_tilemap_right);
		}
		else
			draw_video_layer(bitmap, cliprect, &ddealer_vregs[0x1e0 / 2], fg_tilemap);
	}
	else
	{
		if (ddealer_vregs[0xcc / 2] & 0x80)
		{
			draw_video_layer(bitmap, cliprect, &ddealer_vregs[0xcc / 2], fg_tilemap_left);
			draw_video_layer(bitmap, cliprect, &ddealer_vregs[0x1e0 / 2], fg_tilemap_right);
		}
		else
			draw_video_layer(bitmap, cliprect, &ddealer_vregs[0x1e0 / 2], fg_tilemap);
	}
}

static WRITE16_HANDLER( back_vram_w )
{
	COMBINE_DATA(&ddealer_back_vram[offset]);
	tilemap_mark_tile_dirty(back_tilemap, offset);
}

static WRITE16_HANDLER( fg_vram_w )
{
	COMBINE_DATA(&ddealer_fg_vram[offset]);
	tilemap_mark_tile_dirty(fg_tilemap, offset);
	if (offset & 0x800)
		tilemap_mark_tile_dirty(fg_tilemap_right, offset & 0x17ff);
	else
		tilemap_mark_tile_dirty(fg_tilemap_left, offset & 0x17ff);
}

static WRITE16_HANDLER( ym2203_w )
{
	if (ACCESSING_LSB)
		YM2203_word_0_w(offset, data & 0xff);
}

static ADDRESS_MAP_START( ddealer_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x080006, 0x080007) AM_READ(input_port_3_word_r)
	AM_RANGE(0x080008, 0x080009) AM_READ(input_port_2_word_r)
	AM_RANGE(0x084000, 0x084003) AM_WRITE(ym2203_w)
	AM_RANGE(0x088000, 0x0883ff) AM_READWRITE(MRA16_RAM, paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x08c000, 0x08c1ff) AM_RAM AM_BASE(&ddealer_vregs)
	AM_RANGE(0x090000, 0x093fff) AM_READWRITE(MRA16_RAM, fg_vram_w) AM_BASE(&ddealer_fg_vram)
	AM_RANGE(0x098000, 0x098001) AM_WRITE(flipscreen_w)
	AM_RANGE(0x09c000, 0x09cfff) AM_READWRITE(MRA16_RAM, back_vram_w) AM_BASE(&ddealer_back_vram)
	AM_RANGE(0x0f0000, 0x0fffff) AM_RAM AM_BASE(&ddealer_work_ram) /* at least 0fe000-0fffff is shared with the MCU */
ADDRESS_MAP_END

static INPUT_PORTS_START( ddealer )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x20, 0x20, "Lady Stripping" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END

static const gfx_layout bglayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout fglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static const gfx_decode ddealer_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bglayout, 0x000, 16 },
	{ REGION_GFX2, 0, &fglayout, 0x100, 16 },
	{ -1 }
};

static void irq1_callback(int param)
{
	cpunum_set_input_line(0, 1, HOLD_LINE);
}

static INTERRUPT_GEN( ddealer_interrupt )
{
	cpunum_set_input_line(0, 4, HOLD_LINE);
	timer_set(cpu_getscanlinetime(102), 0, irq1_callback);
}

static struct YM2203interface ym2203_interface =
{
	0,0,0,0,0
};

static MACHINE_DRIVER_START( ddealer )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(ddealer_map,0)
	MDRV_CPU_VBLANK_INT(ddealer_interrupt,1)

	MDRV_CPU_ADD(TMP91640, 4000000)
	MDRV_CPU_PROGRAM_MAP(prot_map,0)
	MDRV_CPU_IO_MAP(prot_io_map,0)

	MDRV_FRAMES_PER_SECOND(56.2)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_RESET(ddealer)

	MDRV_GFXDECODE(ddealer_gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 278)
	MDRV_VISIBLE_AREA(0, 383, 0, 223)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(ddealer)
	MDRV_VIDEO_UPDATE(ddealer)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 6000000 / 8)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)
MACHINE_DRIVER_END

ROM_START( ddealer )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "1.ic6",  0x00001, 0x20000, CRC(ce0dff50) SHA1(2d7a03f6b9609aea7511a4dc49560a901b0b9f19) )
	ROM_LOAD16_BYTE( "2.ic28", 0x00000, 0x20000, CRC(f00c346f) SHA1(bd73efb19d5f9efc88210d92a82a3f4595b41097) )

	ROM_REGION( 0x04000, REGION_CPU2, 0 )
	ROM_LOAD( "nmk-110_ddealer.bin", 0x0000, 0x4000, CRC(088db9b4) SHA1(71946399e37ffa9293eceac637b76c9169ac16e6) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG */
	ROM_LOAD( "4.ic65", 0x00000, 0x20000, CRC(4939ff1b) SHA1(af2f2feeef5520d775731a58cbfc8fcc913b7348) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* FG */
	ROM_LOAD( "3.ic64", 0x00000, 0x80000, CRC(660e367c) SHA1(54827a8998c58c578c594126d5efc18a92363eaa) )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "6.ic86", 0x000, 0x100, CRC(435653a2) SHA1(575b4a46ea65179de3042614da438d2f6d8b572e) )
	ROM_LOAD( "5.ic67", 0x100, 0x100, CRC(1d3d7e17) SHA1(b5aa0d024f0c0b5f72a2d0a23d1576775a7b3826) )
ROM_END

GAME( 1991, ddealer, 0, ddealer, ddealer, 0, ROT0, "NMK", "Double Dealer", 0 )
