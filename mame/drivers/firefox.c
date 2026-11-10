// license:BSD-3-Clause
// copyright-holders:smf, Aaron Giles, Chris Hardy
// thanks-to:Scott Waye
/* Atari Fire Fox with a Philips 22VP931 laserdisc player. */
#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m6809/m6809.h"
#include "machine/ldvp931.h"
#include "machine/ldv1000.h"
#include "machine/6532riot.h"
#include "sound/5220intf.h"
#include "sound/custom.h"
#include "sound/pokey.h"

#define MASTER_CLOCK 14318180
#define CPU_CLOCK (MASTER_CLOCK / 8)

static UINT8 *tiles, *tile_palette, *sprite_palette;
static UINT8 novram[256], saved_novram[256];
static UINT8 disc_data, disc_read, latch0, adc_channel, sprite_bank;
static UINT8 main_pending, sound_pending, riot_pa, riot_pb, riot_status;
static tilemap *bgtiles;
static mame_bitmap *overlay;
static UINT32 overlay_palette[512];

static READ8_HANDLER( disc_status_r )
{
	return 0xdf ^ (vp931_ready() ? 0 : 0x40) ^ (vp931_pending() ? 0x80 : 0);
}

static READ8_HANDLER( disc_data_r ) { return disc_read; }
static WRITE8_HANDLER( disc_data_w ) { disc_data = data; }
static WRITE8_HANDLER( disc_read_w ) { disc_read = vp931_read(); }
static READ8_HANDLER( inputs_r )
{
	return readinputport(1) | (main_pending << 7) | (sound_pending << 6);
}

static READ8_HANDLER( sound_to_main_r )
{
	sound_pending = 0;
	return soundlatch2_r(0);
}

static WRITE8_HANDLER( main_to_sound_w )
{
	main_pending = 1;
	soundlatch_w(0, data);
	r6532_0_PA7_w(0, 0x80);
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
}

static READ8_HANDLER( main_to_sound_r )
{
	main_pending = 0;
	r6532_0_PA7_w(0, 0);
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
	return soundlatch_r(0);
}

static WRITE8_HANDLER( sound_to_main_w )
{
	sound_pending = 1;
	soundlatch2_w(0, data);
}

static READ8_HANDLER( riot_porta_r )
{
	return (main_pending << 7) | (sound_pending << 6) | 0x10 | (!tms5220_ready_r() << 2);
}

static READ8_HANDLER( riot_portb_r ) { return riot_status; }
static WRITE8_HANDLER( riot_portb_w ) { riot_pb = data; }
static WRITE8_HANDLER( riot_porta_w )
{
	if ((riot_pa & 2) && !(data & 2))
		riot_status = tms5220_status_r(0);
	if ((riot_pa & 1) && !(data & 1))
		tms5220_data_w(0, riot_pb);
	riot_pa = data;
}

static void riot_irq(int state)
{
	cpunum_set_input_line(1, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct R6532interface riot = {
	riot_porta_r, riot_portb_r, riot_porta_w, riot_portb_w, riot_irq
};

static WRITE8_HANDLER( latch0_w )
{
	UINT8 mask = 1 << offset;
	int state = (data & 0x80) != 0;
	int changed = ((latch0 & mask) != 0) != state;

	latch0 = (latch0 & ~mask) | (state ? mask : 0);
	switch (offset)
	{
	case 0:
		if (changed && state) memcpy(novram, saved_novram, sizeof(novram));
		break;
	case 1:
		cpunum_set_input_line(1, INPUT_LINE_RESET, state ? ASSERT_LINE : CLEAR_LINE);
		if (state)
		{
			main_pending = sound_pending = 0;
			cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
		}
		break;
	case 2:
		if (changed && state) memcpy(saved_novram, novram, sizeof(novram));
		break;
	case 4:
	case 5:
		vp931_audio(((latch0 >> 5) & 1) | ((latch0 >> 3) & 2));
		break;
	case 6:
		cpunum_set_input_line(2, INPUT_LINE_RESET, state ? CLEAR_LINE : ASSERT_LINE);
		if (changed && !state) vp931_reset();
		break;
	case 7:
		if (changed && !state) vp931_write(disc_data);
		break;
	}
}

static READ8_HANDLER( novram_r ) { return novram[offset]; }
static WRITE8_HANDLER( novram_w ) { novram[offset] = data; }

static NVRAM_HANDLER( firefox )
{
	if (read_or_write)
		mame_fwrite(file, novram, sizeof(novram));
	else
	{
		memset(saved_novram, 0xff, sizeof(saved_novram));
		if (file) mame_fread(file, saved_novram, sizeof(saved_novram));
		memcpy(novram, saved_novram, sizeof(novram));
	}
}

static READ8_HANDLER( adc_r ) { return readinputport(adc_channel ? 5 : 4); }
static WRITE8_HANDLER( adc_select_w ) { adc_channel = offset; }
static WRITE8_HANDLER( irq_clear_w ) { cpunum_set_input_line(0, M6809_IRQ_LINE, CLEAR_LINE); }
static WRITE8_HANDLER( firq_clear_w ) { cpunum_set_input_line(0, M6809_FIRQ_LINE, CLEAR_LINE); }
static WRITE8_HANDLER( self_reset_w ) { cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE); }
static WRITE8_HANDLER( coin_w ) { coin_counter_w(offset, data & 0x80); }
static WRITE8_HANDLER( led_w ) { set_led_status(offset, !(data & 0x80)); }
static WRITE8_HANDLER( bank_w ) { memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000 + (data & 31) * 0x1000); }
static WRITE8_HANDLER( sprite_bank_w ) { sprite_bank = data & 3; }

static WRITE8_HANDLER( tile_w )
{
	tiles[offset] = data;
	tilemap_mark_tile_dirty(bgtiles, offset);
}

static void update_color(UINT8 *ram, int index, int base)
{
	int blue = ram[index + 512];
	overlay_palette[base + index] = ((UINT32)(blue & 3) << 24) |
		MAKE_RGB(ram[index], ram[index + 256], blue);
}

static WRITE8_HANDLER( tile_palette_w )
{
	tile_palette[offset] = data;
	update_color(tile_palette, offset & 255, 0);
}

static WRITE8_HANDLER( sprite_palette_w )
{
	sprite_palette[offset] = data;
	update_color(sprite_palette, offset & 255, 256);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x1fff) AM_READWRITE(MRA8_RAM, tile_w) AM_BASE(&tiles)
	AM_RANGE(0x2000, 0x27ff) AM_RAM AM_BASE(&spriteram)
	AM_RANGE(0x2800, 0x2aff) AM_READWRITE(MRA8_RAM, sprite_palette_w) AM_BASE(&sprite_palette)
	AM_RANGE(0x2b00, 0x2b00) AM_MIRROR(0x04ff) AM_WRITE(sprite_bank_w)
	AM_RANGE(0x2c00, 0x2eff) AM_READWRITE(MRA8_RAM, tile_palette_w) AM_BASE(&tile_palette)
	AM_RANGE(0x3000, 0x3fff) AM_ROMBANK(1)
	AM_RANGE(0x4000, 0x40ff) AM_READWRITE(novram_r, novram_w)
	AM_RANGE(0x4100, 0x4100) AM_MIRROR(0x00f8) AM_READ(input_port_0_r)
	AM_RANGE(0x4101, 0x4101) AM_MIRROR(0x00f8) AM_READ(inputs_r)
	AM_RANGE(0x4102, 0x4102) AM_MIRROR(0x00f8) AM_READ(disc_status_r)
	AM_RANGE(0x4103, 0x4103) AM_MIRROR(0x00f8) AM_READ(input_port_2_r)
	AM_RANGE(0x4104, 0x4104) AM_MIRROR(0x00f8) AM_READ(input_port_3_r)
	AM_RANGE(0x4105, 0x4105) AM_MIRROR(0x00f8) AM_READ(disc_data_r)
	AM_RANGE(0x4106, 0x4106) AM_MIRROR(0x00f8) AM_READ(sound_to_main_r)
	AM_RANGE(0x4107, 0x4107) AM_MIRROR(0x00f8) AM_READ(adc_r)
	AM_RANGE(0x4200, 0x4200) AM_MIRROR(0x0047) AM_WRITE(irq_clear_w)
	AM_RANGE(0x4208, 0x4208) AM_MIRROR(0x0047) AM_WRITE(firq_clear_w)
	AM_RANGE(0x4210, 0x4210) AM_MIRROR(0x0047) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x4218, 0x4218) AM_MIRROR(0x0047) AM_WRITE(disc_read_w)
	AM_RANGE(0x4220, 0x4223) AM_MIRROR(0x0044) AM_WRITE(adc_select_w)
	AM_RANGE(0x4230, 0x4230) AM_MIRROR(0x0047) AM_WRITE(self_reset_w)
	AM_RANGE(0x4280, 0x4287) AM_MIRROR(0x0040) AM_WRITE(latch0_w)
	AM_RANGE(0x4288, 0x4289) AM_MIRROR(0x0040) AM_WRITE(coin_w)
	AM_RANGE(0x428c, 0x428f) AM_MIRROR(0x0040) AM_WRITE(led_w)
	AM_RANGE(0x4290, 0x4290) AM_MIRROR(0x0047) AM_WRITE(bank_w)
	AM_RANGE(0x4298, 0x4298) AM_MIRROR(0x0047) AM_WRITE(main_to_sound_w)
	AM_RANGE(0x42a0, 0x42a0) AM_MIRROR(0x0047) AM_WRITE(disc_data_w)
	AM_RANGE(0x4400, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( audio_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x087f) AM_MIRROR(0x0700) AM_RAM
	AM_RANGE(0x0880, 0x089f) AM_MIRROR(0x0760) AM_READWRITE(r6532_0_r, r6532_0_w)
	AM_RANGE(0x1000, 0x1000) AM_READ(main_to_sound_r)
	AM_RANGE(0x1800, 0x1800) AM_WRITE(sound_to_main_w)
	AM_RANGE(0x2000, 0x200f) AM_READWRITE(pokey1_r, pokey1_w)
	AM_RANGE(0x2800, 0x280f) AM_READWRITE(pokey2_r, pokey2_w)
	AM_RANGE(0x3000, 0x300f) AM_READWRITE(pokey3_r, pokey3_w)
	AM_RANGE(0x3800, 0x380f) AM_READWRITE(pokey4_r, pokey4_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( player_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0x7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( player_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0, 0x1ff) AM_READWRITE(vp931_io_r, vp931_io_w)
ADDRESS_MAP_END

static void get_tile_info(int index)
{
	SET_TILE_INFO(0, tiles[index], 0, 0)
}

static PALETTE_INIT( firefox )
{
	int i;
	/* Keep the pen number in the overlay so equal RGB colors can have different alpha. */
	for (i = 0; i < 512; ++i)
		palette_set_color(i, 0, i >> 8, i & 255);
}

static VIDEO_START( firefox )
{
	bgtiles = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	if (!bgtiles)
		return 1;
	tilemap_set_transparent_pen(bgtiles, 0);
	tilemap_set_scrolldy(bgtiles, 44, 0);
	overlay = auto_bitmap_alloc_depth(512, 524, 32);
	return 0;
}

static VIDEO_UPDATE( firefox )
{
	rectangle area = { 56, 423, cliprect->min_y, cliprect->max_y };
	int sprite, x, y;

	ldv1000_video(bitmap, cliprect);
	fillbitmap(overlay, Machine->pens[256], &area);
	for (sprite = 0; sprite < 32; ++sprite)
	{
		const UINT8 *obj = spriteram + sprite_bank * 0x200 + sprite * 16;
		int flags = obj[0];
		int sx = obj[2] + ((flags & 2) << 7);
		int sy = obj[1] + ((flags & 1) << 8);
		int row;

		if (sx)
			for (row = 0; row < 8; ++row)
				drawgfx(overlay, Machine->gfx[1], obj[15 - row] + ((flags & 0xc0) << 2),
					(flags >> 2) & 3, flags & 0x20, flags & 0x10,
					sx + 8, 544 - sy - row * 16, &area, TRANSPARENCY_PEN, 0);
	}
	tilemap_draw(overlay, &area, bgtiles, 0, 0);
	for (y = area.min_y; y <= area.max_y; ++y)
	{
		UINT32 *dst = bitmap->line[y];
		const UINT32 *src = overlay->line[y];

		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			UINT32 color = overlay_palette[src[56 + x * 368 / 704]];
			int alpha = color >> 24;
			if (alpha == 3)
				dst[x] = color & 0xffffff;
			else if (alpha)
			{
				UINT32 bg = dst[x];
				dst[x] = MAKE_RGB((RGB_RED(color) * alpha + RGB_RED(bg) * (3 - alpha)) / 3,
					(RGB_GREEN(color) * alpha + RGB_GREEN(bg) * (3 - alpha)) / 3,
					(RGB_BLUE(color) * alpha + RGB_BLUE(bg) * (3 - alpha)) / 3);
			}
		}
	}
}

static void video_irq(int line)
{
	force_partial_update(line);
	cpunum_set_input_line(0, M6809_IRQ_LINE, ASSERT_LINE);
}

static INTERRUPT_GEN( firefox_field )
{
	int line;
	if (latch0 & 0x40) vp931_field();
	for (line = 140; line <= 524; line += 128)
		timer_set(TIME_IN_USEC(line * 63.555 / 2), line, video_irq);
}

static MACHINE_START( firefox )
{
	r6532_init(0, &riot);
	return vp931_init();
}

static MACHINE_RESET( firefox )
{
	disc_data = disc_read = latch0 = adc_channel = sprite_bank = 0;
	main_pending = sound_pending = riot_status = riot_pb = 0;
	riot_pa = 0xff;
	bank_w(0, 0);
	vp931_reset();
	vp931_audio(0);
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
}

INPUT_PORTS_START( firefox )
	PORT_START_TAG("rdin0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("rdin1")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START_TAG("opt0")
	PORT_DIPNAME(    0x03, 0x00, "Coins Per Credit" )
	PORT_DIPSETTING( 0x00, "1 Coin 1 Credit" )
	PORT_DIPSETTING( 0x01, "2 Coins 1 Credit" )
	PORT_DIPSETTING( 0x02, "3 Coins 1 Credit" )
	PORT_DIPSETTING( 0x03, "4 Coins 1 Credit" )
	PORT_DIPNAME(    0x0c, 0x00, "Right Coin" )
	PORT_DIPSETTING( 0x00, "1 Coin for 1 Coin Unit" )
	PORT_DIPSETTING( 0x04, "1 Coin for 4 Coin Units" )
	PORT_DIPSETTING( 0x08, "1 Coin for 5 Coin Units" )
	PORT_DIPSETTING( 0x0c, "1 Coin for 6 Coin Units" )
	PORT_DIPNAME(    0x10, 0x00, "Left Coin" )
	PORT_DIPSETTING( 0x00, "1 Coin for 1 Coin Unit" )
	PORT_DIPSETTING( 0x10, "1 Coin for 2 Coin Units" )
	PORT_DIPNAME(    0xe0, 0x00, "Bonus Adder" )
	PORT_DIPSETTING( 0x00, DEF_STR( None ) )
	PORT_DIPSETTING( 0x20, "1 Credit for 2 Coin Units" )
	PORT_DIPSETTING( 0xa0, "1 Credit for 3 Coin Units" )
	PORT_DIPSETTING( 0x40, "1 Credit for 4 Coin Units" )
	PORT_DIPSETTING( 0x80, "1 Credit for 5 Coin Units" )
	PORT_DIPSETTING( 0x60, "2 Credits for 4 Coin Units" )
	PORT_DIPSETTING( 0xe0, DEF_STR( Free_Play ) )

	PORT_START_TAG("opt1")
	PORT_DIPNAME( 0x01, 0x00, "Missions" )
	PORT_DIPSETTING(    0x00, "All .50" )
	PORT_DIPSETTING(    0x01, ".50 .75" )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, "Moderate" )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x00, "Gas Usage" )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, "Moderate" )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x60, 0x00, "Bonus Gas" )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, "Moderate" )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x80, 0x00, "Pro Limit" )
	PORT_DIPSETTING(    0x00, "Moderate" )
	PORT_DIPSETTING(    0x80, DEF_STR( Hardest ) )

	PORT_START_TAG("PITCH")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(30)

	PORT_START_TAG("YAW")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(30)
INPUT_PORTS_END

static const gfx_layout tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(0,4) },
	{ STEP8(0,32) },
	32*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	6,
	{ RGN_FRAC(0,6), RGN_FRAC(1,6), RGN_FRAC(2,6), RGN_FRAC(3,6), RGN_FRAC(4,6), RGN_FRAC(5,6) },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	32*8
};

static const gfx_decode gfxdecode[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0, 1 },
	{ REGION_GFX2, 0, &spritelayout, 256, 4 },
	{ -1 }
};

static MACHINE_DRIVER_START( firefox )
	MDRV_CPU_ADD(M6809, CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(firefox_field, 1)
	MDRV_CPU_ADD(M6502, CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(audio_map, 0)
	MDRV_CPU_ADD(I8039, 11000000 / I8039_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(player_map, 0)
	MDRV_CPU_IO_MAP(player_io, 0)
	MDRV_INTERLEAVE(512)
	MDRV_MACHINE_START(firefox)
	MDRV_MACHINE_RESET(firefox)
	MDRV_NVRAM_HANDLER(firefox)
	MDRV_WATCHDOG_TIME_INIT(TIME_IN_HZ((double)CPU_CLOCK / 65536))
	MDRV_FRAMES_PER_SECOND(60000.0 / 1001)
	MDRV_VBLANK_DURATION(1398)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(720, 524)
	MDRV_VISIBLE_AREA(0, 703, 44, 523)
	MDRV_GFXDECODE(gfxdecode)
	MDRV_PALETTE_LENGTH(512)
	MDRV_PALETTE_INIT(firefox)
	MDRV_VIDEO_START(firefox)
	MDRV_VIDEO_UPDATE(firefox)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(POKEY, CPU_CLOCK)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.3)
	MDRV_SOUND_ADD(POKEY, CPU_CLOCK)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.3)
	MDRV_SOUND_ADD(POKEY, CPU_CLOCK)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.3)
	MDRV_SOUND_ADD(POKEY, CPU_CLOCK)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.3)
	MDRV_SOUND_ADD(TMS5220, MASTER_CLOCK / 22)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.75)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.75)
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(ldv1000_sound)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

ROM_START( firefox )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 64k for code + data & 128k for banked roms */
	ROM_LOAD( "136026.209",     0x04000, 0x4000, CRC(9f559f1b) SHA1(142d14cb5158ea77f6fc6d9bf0ce723842f345e2) ) /* 8b/c */
	ROM_LOAD( "136026.210",     0x08000, 0x4000, CRC(d769b40d) SHA1(2d354649a381f3399cb0161267bd1c36a8f2bb4b) ) /* 7b/c */
	ROM_LOAD( "136026.211",     0x0c000, 0x4000, CRC(7293ab03) SHA1(73d0d173da295ad59e431bab0a9814a71146cbc2) ) /* 6b/c */
	ROM_LOAD( "136026.201",     0x10000, 0x4000, CRC(c118547a) SHA1(4d3502cbde3116588ed944bf1750bab50e4c813c) ) /* 8a */
	/* empty 7a */
	/* empty 6a */
	/* empty 5a */
	ROM_LOAD( "136026.205",     0x20000, 0x4000, CRC(dc21677f) SHA1(576a96c1e07e1362a0a367e76dc369ee8a950144) ) /* 4a */
	ROM_LOAD( "136026.127",     0x24000, 0x2000, CRC(c0c765ab) SHA1(79f6c8c1d00684d7143b2d33a5669bdf5cd01e96) ) /* 3a */
	ROM_RELOAD( 0x26000, 0x2000 )
	/* empty 2a */
	/* empty 1a */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for code */
	/* empty 4k/l */
	ROM_LOAD( "136026.128",     0x08000, 0x2000, CRC(5358d870) SHA1(e8f2983a7e612e1a050a3c0b9f19b1077de4c146) ) /* 4m */
	ROM_RELOAD( 0x0a000, 0x2000 )
	ROM_LOAD( "136026.214",     0x0c000, 0x4000, CRC(92378b78) SHA1(62c7a1fee675fa3f9125f8e208b8207f0ce28bbe) ) /* 4n */

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "136026.125",     0x0000,  0x2000, CRC(8a32f9f1) SHA1(f899174f55cd4a24a3be4a0f4bb44d3e8e938586) ) /* 6p */

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	/* empty 6c */
	/* empty 6a */
	ROM_LOAD( "136026.124",     0x08000,  0x4000, CRC(5efe0f6c) SHA1(df35fd9267d966ab379c2f78ed418f4606741b28)) /* 5c */
	ROM_LOAD( "136026.123",     0x0c000,  0x4000, CRC(dffe48b3) SHA1(559907651bb425e26a834b467959b15092d23d27)) /* 5a */
	ROM_LOAD( "136026.118",     0x10000,  0x4000, CRC(0ed4df15) SHA1(7aa599f428112fff4bfedf63fafc22f19fa66546)) /* 4c */
	ROM_LOAD( "136026.122",     0x14000,  0x4000, CRC(8e2c6616) SHA1(59cbd585028bb634034a9dfd552275bd41f01989)) /* 4a */
	ROM_LOAD( "136026.117",     0x18000,  0x4000, CRC(79129084) SHA1(4219ff7cd444ad11e4cb9f1c30ac15fe0cfc5a17)) /* 3c */
	ROM_LOAD( "136026.121",     0x1c000,  0x4000, CRC(494972d4) SHA1(fa0e24e911b233e9644d7794ba03f76bfd39aa8c)) /* 3a */
	ROM_LOAD( "136026.116",     0x20000,  0x4000, CRC(d5282d4e) SHA1(de5fdf82a615625aa77b39e035b4206216faaf9c)) /* 2c */
	ROM_LOAD( "136026.120",     0x24000,  0x4000, CRC(e1b95923) SHA1(b6d0c0af0a8f55e728cd0f4c3222745eefd57f50)) /* 2a */
	ROM_LOAD( "136026.115",     0x28000,  0x4000, CRC(861abc82) SHA1(1845888d07162ae915364a2a91294731f1c5b3bd)) /* 1c */
	ROM_LOAD( "136026.119",     0x2c000,  0x4000, CRC(959471b1) SHA1(a032209a209f51d34360d5c7ad32ec62150158d2)) /* 1a */

	ROM_REGION( 0x800, REGION_CPU3, ROMREGION_ERASE00 )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "firefox", 0, SHA1(3c4be40f55b44d0352b64c0861b6d1b650451ce7) )
ROM_END

GAME( 1984, firefox, 0, firefox, firefox, 0, ROT0, "Atari", "Fire Fox (set 1)", 0 )
