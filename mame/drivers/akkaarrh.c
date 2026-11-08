// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*
    Atari Akka Arrh hardware
*/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/pokey.h"

static UINT8 *akkaarrh_videoram;
static UINT8 *akkaarrh_spriteram;
static tilemap *akkaarrh_tilemap[4];
static UINT8 video_mirror;
static UINT8 earom[64];
static UINT8 earom_address, earom_data, earom_control;

static void get_tile_info(int tile_index)
{
	int attr = akkaarrh_videoram[tile_index + 0x400];
	SET_TILE_INFO(0, akkaarrh_videoram[tile_index], attr & 15, TILE_FLIPYX(attr >> 6))
}

static VIDEO_START( akkaarrh )
{
	int i;
	for (i = 0; i < 4; ++i)
	{
		akkaarrh_tilemap[i] = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 30);
		if (!akkaarrh_tilemap[i])
			return 1;
		tilemap_set_flip(akkaarrh_tilemap[i], ((i & 1) ? TILEMAP_FLIPX : 0) | ((i & 2) ? TILEMAP_FLIPY : 0));
	}
	return 0;
}

static VIDEO_UPDATE( akkaarrh )
{
	int i;
	if (!(video_mirror & 1))
		tilemap_draw(bitmap, cliprect, akkaarrh_tilemap[0], 0, 0);
	else
	{
		/* Zoom uses reflected copies of the upper-left quadrant. */
		for (i = 0; i < 4; ++i)
		{
			rectangle clip = *cliprect;
			int left = (i & 1) * 128;
			int top = (i >> 1) * 120;
			if (clip.min_x < left) clip.min_x = left;
			if (clip.max_x > left + 127) clip.max_x = left + 127;
			if (clip.min_y < top) clip.min_y = top;
			if (clip.max_y > top + 119) clip.max_y = top + 119;
			if (clip.min_x <= clip.max_x && clip.min_y <= clip.max_y)
				tilemap_draw(bitmap, &clip, akkaarrh_tilemap[i], 0, 0);
		}
	}
	for (i = 0; i < 0x100; i += 4)
		drawgfx(bitmap, Machine->gfx[1], akkaarrh_spriteram[i + 1], akkaarrh_spriteram[i] & 15,
			0, 0, akkaarrh_spriteram[i + 3], 240 - akkaarrh_spriteram[i + 2], cliprect, TRANSPARENCY_PEN, 0);
}

static WRITE8_HANDLER( akkaarrh_videoram_w )
{
	int i;
	akkaarrh_videoram[offset] = data;
	for (i = 0; i < 4; ++i)
		tilemap_mark_tile_dirty(akkaarrh_tilemap[i], offset & 0x3ff);
}

static WRITE8_HANDLER( akkaarrh_palette_w )
{
	paletteram[offset] = data;
	palette_set_color(offset, pal3bit(data >> 5), pal2bit(data >> 3), pal3bit(data));
}

static WRITE8_HANDLER( akkaarrh_irq_ack_w )
{
	cpunum_set_input_line(0, M6502_IRQ_LINE, CLEAR_LINE);
}

static WRITE8_HANDLER( akkaarrh_mirror_w )
{
	video_mirror = data;
}

static WRITE8_HANDLER( akkaarrh_output_w )
{
	int i;
	if (!offset)
	{
		set_led_status(0, !(data & 8));
		set_led_status(1, !(data & 4));
		coin_counter_w(0, data & 1);
		coin_counter_w(1, data & 2);
	}
	else
		for (i = 0; i < 8; ++i)
			set_led_status(2 + (offset - 1) * 8 + i, (data >> i) & 1);
}

static READ8_HANDLER( akkaarrh_earom_r )
{
	return earom_data;
}

static WRITE8_HANDLER( akkaarrh_earom_w )
{
	earom_address = offset & 63;
	earom_data = data;
}

static void earom_update(void)
{
	if (!(earom_control & 2))
	{
		/* Programming only clears bits; an erase cycle must precede it. */
		if (earom_control & 4)
			earom[earom_address] = 0xff;
		else
			earom[earom_address] &= earom_data;
	}
}

static WRITE8_HANDLER( akkaarrh_earom_control_w )
{
	UINT8 old = earom_control;
	earom_control = (old & 1) | ((data & 4) ? 0 : 2) | ((data & 2) ? 4 : 0) | (data & 8);
	if ((earom_control & 8) && earom_control != old)
		earom_update();
	old = earom_control;
	earom_control = (earom_control & ~1) | (data & 1);
	/* Reads latch on the falling clock edge while chip select is active. */
	if ((earom_control & 8) && (old & 1) && !(data & 1))
	{
		if (earom_control & 2)
			earom_data = earom[earom_address];
		earom_update();
	}
}

static NVRAM_HANDLER( akkaarrh )
{
	if (read_or_write)
		mame_fwrite(file, earom, sizeof(earom));
	else if (file)
		mame_fread(file, earom, sizeof(earom));
	else
		memset(earom, 0xff, sizeof(earom));
}

static MACHINE_START( akkaarrh )
{
	state_save_register_global(video_mirror);
	state_save_register_global_array(earom);
	state_save_register_global(earom_address);
	state_save_register_global(earom_data);
	state_save_register_global(earom_control);
	return 0;
}

static ADDRESS_MAP_START( akkaarrh_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x1000, 0x10ff) AM_RAM AM_BASE(&akkaarrh_spriteram)
	AM_RANGE(0x2000, 0x27ff) AM_RAM AM_WRITE(akkaarrh_videoram_w) AM_BASE(&akkaarrh_videoram)
	AM_RANGE(0x3000, 0x30ff) AM_RAM AM_WRITE(akkaarrh_palette_w) AM_BASE(&paletteram)
	AM_RANGE(0x4000, 0x4000) AM_WRITE(akkaarrh_irq_ack_w)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(akkaarrh_mirror_w)
	AM_RANGE(0x7010, 0x701f) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x7020, 0x702f) AM_READWRITE(pokey2_r, pokey2_w)
	AM_RANGE(0x7040, 0x707f) AM_READNOP AM_WRITE(akkaarrh_earom_w)
	AM_RANGE(0x7080, 0x7080) AM_READ(input_port_0_r)
	AM_RANGE(0x7081, 0x7081) AM_READ(input_port_1_r)
	AM_RANGE(0x7082, 0x7082) AM_READ(input_port_2_r)
	AM_RANGE(0x7083, 0x7083) AM_READ(input_port_3_r)
	AM_RANGE(0x7087, 0x7087) AM_READ(akkaarrh_earom_r)
	AM_RANGE(0x70c0, 0x70c3) AM_WRITE(akkaarrh_output_w)
	AM_RANGE(0x70c7, 0x70c7) AM_WRITE(akkaarrh_earom_control_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static INPUT_PORTS_START( akkaarrh )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Zoom")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Fire")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Power blaster")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(30) PORT_KEYDELTA(20) PORT_REVERSE
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(30) PORT_KEYDELTA(20)
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static const gfx_layout tile_layout =
{
	8, 8, RGN_FRAC(1,4), 4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static const gfx_layout sprite_layout =
{
	16, 16, RGN_FRAC(1,4), 4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
	{ 0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	16*16
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 16 },
	{ REGION_GFX2, 0, &sprite_layout, 0, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( akkaarrh )
	MDRV_CPU_ADD(M6502, 12096000/8)
	MDRV_CPU_PROGRAM_MAP(akkaarrh_map, 0)
	/* The interrupt handler must sample VBLANK once per group of four IRQs. */
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 4)
	MDRV_MACHINE_START(akkaarrh)
	MDRV_NVRAM_HANDLER(akkaarrh)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(1000000.0 * 22 / (60 * 262)))
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 262)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(akkaarrh)
	MDRV_VIDEO_UPDATE(akkaarrh)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(POKEY, 1250000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(POKEY, 1250000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END

ROM_START( akkaarrh )
	 ROM_REGION( 0x10000, REGION_CPU1, 0 )
	 ROM_LOAD( "akka_8000.p1",  0x8000, 0x1000, CRC(578bb162) SHA1(f003d9a63e397c377e4738ca31a637da1da2cbdb) )
	 ROM_LOAD( "akka_9000.m1",  0x9000, 0x1000, CRC(837fa612) SHA1(2a2ccbf879fd9cfc0342e6c5bbcd23962bdbbcde) )
	 ROM_LOAD( "akka_a000.l1",  0xa000, 0x1000, CRC(13c769e9) SHA1(8e6c7b21fa555850ba8d00abbb4516e483964b5d) )
	 ROM_LOAD( "akka_b000.j1",  0xb000, 0x1000, CRC(35c04f28) SHA1(71ad7eaf5bf96c1a0a321f1b04706afd40d9757f) )
	 ROM_LOAD( "akka_c000.h1",  0xc000, 0x1000, CRC(17e85ac4) SHA1(bd010060eaf8fbf27176d4fdec241ee3b42aece8) )
	 ROM_LOAD( "akka_d000.e1",  0xd000, 0x1000, CRC(03fb4143) SHA1(3a2106d7322139b9924566133d084bb7c5d769bc) )
	 ROM_LOAD( "akka_e000.f1",  0xe000, 0x1000, CRC(8d3e671c) SHA1(c99d92c4afaa1ba043520b98d925f83490e49f0a) )
	 ROM_RELOAD(                0xf000, 0x1000 )

	 ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	 ROM_LOAD( "akka_pf0.l6",  0x0000, 0x0800, CRC(5c10b63e) SHA1(27ac6cebb3f6733cb830210f4723938ec2ddaafc) )
	 ROM_LOAD( "akka_pf1.j6",  0x0800, 0x0800, CRC(636fd64c) SHA1(c2985332b8f32a8dd096fc081df12ae1a132e6a3) )
	 ROM_LOAD( "akka_pf2.p6",  0x1000, 0x0800, CRC(a5f25d69) SHA1(8f5af774db3ed3969c8c0a0b530ea62198830e3d) )
	 ROM_LOAD( "akka_pf3.m6",  0x1800, 0x0800, CRC(a3449469) SHA1(42f2ccf6ea02987bfb91ff1ca42684e9700f2f8f) )

	 ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	 ROM_LOAD( "akka_mo0.f11", 0x0000, 0x1000, CRC(71bd1bc6) SHA1(9c3d2c039c648834a7a0b98cd2dcbbbb88f74dd8) )
	 ROM_LOAD( "akka_mo1.d11", 0x1000, 0x1000, CRC(a5ee8ecc) SHA1(d2a8c9791dca9157d9d20b8a4f6e760e6db35f05) )
	 ROM_LOAD( "akka_mo2.a11", 0x2000, 0x1000, CRC(11cec4d9) SHA1(32ea614c25473aeb9a012a3980ad616592f6c3bb) )
	 ROM_LOAD( "akka_mo3.b11", 0x3000, 0x1000, CRC(adcf6a36) SHA1(0555a252ba39400d2c533add6ac492926674d6ad) )
ROM_END

GAME( 1982, akkaarrh, 0, akkaarrh, akkaarrh, 0, ROT0, "Atari", "Akka Arrh (prototype)", 0 )
