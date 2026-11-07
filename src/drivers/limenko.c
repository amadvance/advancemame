// license:BSD-3-Clause
// copyright-holders:Pierpaolo Prazzoli, Tomasz Slanina
/*
  Limenko Power System 2

  driver by Pierpaolo Prazzoli and Tomasz Slanina

  Cartridge based system with Hyperstone E1-32XN CPU
  and QDSP QS1000 sound hardware.
*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/i8051/i8051.h"
#include "sound/custom.h"
WRITE8_HANDLER( limenko_wave_w );
void limenko_sound_reset(void);
extern struct CustomSound_interface limenko_sound_interface;

static UINT32 *mainram, *fg_videoram, *md_videoram, *bg_videoram;
static UINT32 *limenko_spriteram, *limenko_videoreg;
static tilemap *fg_tilemap, *md_tilemap, *bg_tilemap;
static mame_bitmap *sprites_bitmap, *sprites_priority;
static UINT8 spriteram_bit;
static int prev_sprites_count;

static READ32_HANDLER( port2_r )
{
	return (readinputport(2) & ~0x80800000) | (EEPROM_read_bit() << 23) | (spriteram_bit << 31);
}

static WRITE32_HANDLER( limenko_eeprom_w )
{
	if (!(mem_mask & 0x00ff0000))
	{
		EEPROM_write_bit(data & 0x40000);
		EEPROM_set_cs_line((data & 0x10000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x20000) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static WRITE32_HANDLER( limenko_coin_w )
{
	if (!(mem_mask & 0x00ff0000))
		coin_counter_w(0, data & 0x10000);
}

static WRITE32_HANDLER( limenko_palette_w )
{
	COMBINE_DATA(&paletteram32[offset]);
	if (!(mem_mask & 0xffff0000))
	{
		UINT16 color = paletteram32[offset] >> 16;
		palette_set_color(offset * 2, pal5bit(color), pal5bit(color >> 5), pal5bit(color >> 10));
	}
	if (!(mem_mask & 0x0000ffff))
	{
		UINT16 color = paletteram32[offset];
		palette_set_color(offset * 2 + 1, pal5bit(color), pal5bit(color >> 5), pal5bit(color >> 10));
	}
}

static WRITE32_HANDLER( fg_videoram_w )
{
	COMBINE_DATA(&fg_videoram[offset]);
	tilemap_mark_tile_dirty(fg_tilemap, offset);
}
static WRITE32_HANDLER( md_videoram_w )
{
	COMBINE_DATA(&md_videoram[offset]);
	tilemap_mark_tile_dirty(md_tilemap, offset);
}
static WRITE32_HANDLER( bg_videoram_w )
{
	COMBINE_DATA(&bg_videoram[offset]);
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static void get_fg_tile_info(int tile_index)
{
	UINT32 data = fg_videoram[tile_index];
	SET_TILE_INFO(0, data & 0x7ffff, data >> 28, 0)
}
static void get_md_tile_info(int tile_index)
{
	UINT32 data = md_videoram[tile_index];
	SET_TILE_INFO(0, data & 0x7ffff, data >> 28, 0)
}
static void get_bg_tile_info(int tile_index)
{
	UINT32 data = bg_videoram[tile_index];
	SET_TILE_INFO(0, data & 0x7ffff, data >> 28, 0)
}

static void draw_single_sprite(int sx, int sy, int width, int height,
	UINT32 code, int color, int flipx, int flipy, int priority)
{
	const UINT8 *gfx = memory_region(REGION_GFX1) + code;
	int x, y, x0 = sx < 0 ? 0 : sx, y0 = sy < 0 ? 0 : sy;
	int x1 = sx + width > 512 ? 512 : sx + width;
	int y1 = sy + height > 512 ? 512 : sy + height;
	for (y = y0; y < y1; ++y)
	{
		UINT16 *dest = sprites_bitmap->line[y];
		UINT8 *pri = sprites_priority->line[y];
		int row = flipy ? height - 1 - (y - sy) : y - sy;
		const UINT8 *source = gfx + row * width;
		for (x = x0; x < x1; ++x)
		{
			UINT8 pen = source[flipx ? width - 1 - (x - sx) : x - sx];
			if (pen && pri[x] < priority)
			{
				dest[x] = (color << 8) | pen;
				pri[x] = priority;
			}
		}
	}
}

static WRITE32_HANDLER( limenko_sprite_buffer_w )
{
	UINT32 *sprites;
	int i;
	spriteram_bit ^= 1;
	sprites = limenko_spriteram + 0x400 * spriteram_bit;
	fillbitmap(sprites_bitmap, 0, 0);
	fillbitmap(sprites_priority, 0, 0);
	/* The DMA renders the previously latched count before accepting the next one. */
	for (i = 0; i <= prev_sprites_count * 2; i += 2)
	{
		UINT32 attr = sprites[i], data = sprites[i + 1];
		int x, y, width, height, flipx, flipy, color, priority;
		UINT32 code;
		if (!(attr & 0x80000000))
			continue;
		x = (attr >> 16) & 511;
		y = attr & 511;
		width = (((attr >> 25) & 7) + 1) * 8;
		height = (((attr >> 9) & 7) + 1) * 8;
		flipx = attr & 0x10000000;
		flipy = attr & 0x1000;
		code = (data & 0x7ffff) << 6;
		color = data >> 28;
		priority = (data & 0x04000000) ? 1 : 2;
		if (code + width * height > memory_region_length(REGION_GFX1))
			continue;
		draw_single_sprite(x, y, width, height, code, color, flipx, flipy, priority);
		draw_single_sprite(x - 512, y, width, height, code, color, flipx, flipy, priority);
		draw_single_sprite(x, y - 512, width, height, code, color, flipx, flipy, priority);
		draw_single_sprite(x - 512, y - 512, width, height, code, color, flipx, flipy, priority);
	}
	prev_sprites_count = (limenko_videoreg[0] >> 16) & 0x1ff;
}

static VIDEO_START( limenko )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 128, 64);
	md_tilemap = tilemap_create(get_md_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 128, 64);
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 128, 64);
	if (!bg_tilemap || !md_tilemap || !fg_tilemap)
		return 1;
	tilemap_set_transparent_pen(md_tilemap, 0);
	tilemap_set_transparent_pen(fg_tilemap, 0);
	sprites_bitmap = auto_bitmap_alloc_depth(512, 512, 16);
	sprites_priority = auto_bitmap_alloc_depth(512, 512, 8);
	return 0;
}

static VIDEO_UPDATE( limenko )
{
	int x, y;
	fillbitmap(bitmap, get_black_pen(), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_set_enable(bg_tilemap, limenko_videoreg[0] & 4);
	tilemap_set_enable(md_tilemap, limenko_videoreg[0] & 2);
	tilemap_set_enable(fg_tilemap, limenko_videoreg[0] & 1);
	tilemap_set_scrolly(bg_tilemap, 0, limenko_videoreg[3] & 0xffff);
	tilemap_set_scrolly(md_tilemap, 0, limenko_videoreg[2] & 0xffff);
	tilemap_set_scrolly(fg_tilemap, 0, limenko_videoreg[1] & 0xffff);
	tilemap_set_scrollx(bg_tilemap, 0, limenko_videoreg[3] >> 16);
	tilemap_set_scrollx(md_tilemap, 0, limenko_videoreg[2] >> 16);
	tilemap_set_scrollx(fg_tilemap, 0, limenko_videoreg[1] >> 16);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, md_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 1);
	if (limenko_videoreg[0] & 8)
		for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		{
			UINT16 *source = sprites_bitmap->line[y], *dest = bitmap->line[y];
			UINT8 *spr_pri = sprites_priority->line[y], *tile_pri = priority_bitmap->line[y];
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
				if (source[x] && tile_pri[x] < spr_pri[x])
					dest[x] = Machine->pens[source[x]];
		}
}

static WRITE32_HANDLER( limenko_sound_w )
{
	if (!(mem_mask & 0x00ff0000))
	{
		soundlatch_w(0, data >> 16);
		cpunum_set_input_line(1, I8051_INT1_LINE, ASSERT_LINE);
		cpu_boost_interleave(0, TIME_IN_USEC(250));
	}
}

static WRITE8_HANDLER( limenko_qs_p3_w )
{
	/* Keep the low external RAM page separate from the banked program data. */
	memory_set_bankptr(1, memory_region(REGION_CPU2) + (data & 7) * 0x10000 + 0x100);
	if (!(data & 0x20))
		cpunum_set_input_line(1, I8051_INT1_LINE, CLEAR_LINE);
}

static ADDRESS_MAP_START( limenko_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&mainram)
	AM_RANGE(0x40000000, 0x403fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0x80000000, 0x80007fff) AM_RAM AM_WRITE(fg_videoram_w) AM_BASE(&fg_videoram)
	AM_RANGE(0x80008000, 0x8000ffff) AM_RAM AM_WRITE(md_videoram_w) AM_BASE(&md_videoram)
	AM_RANGE(0x80010000, 0x80017fff) AM_RAM AM_WRITE(bg_videoram_w) AM_BASE(&bg_videoram)
	AM_RANGE(0x80018000, 0x80019fff) AM_RAM AM_BASE(&limenko_spriteram)
	AM_RANGE(0x8001c000, 0x8001dfff) AM_RAM AM_WRITE(limenko_palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0x8001e000, 0x8001ebff) AM_RAM
	AM_RANGE(0x8001ffec, 0x8001ffff) AM_RAM AM_BASE(&limenko_videoreg)
	AM_RANGE(0x8003e000, 0x8003e003) AM_WRITE(limenko_sprite_buffer_w)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( limenko_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x0000, 0x0003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x0800, 0x0803) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x1000, 0x1003) AM_READ(port2_r)
	AM_RANGE(0x4000, 0x4003) AM_WRITE(limenko_coin_w)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(limenko_eeprom_w)
	AM_RANGE(0x5000, 0x5003) AM_WRITE(limenko_sound_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( limenko_qs_program, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( limenko_qs_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM
	AM_RANGE(0x0100, 0xffff) AM_ROMBANK(1)
	AM_RANGE(0x0200, 0x0211) AM_WRITE(limenko_wave_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( limenko_qs_ports, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0001, 0x0001) AM_READ(soundlatch_r) AM_WRITENOP
	AM_RANGE(0x0002, 0x0002) AM_WRITENOP
	AM_RANGE(0x0003, 0x0003) AM_WRITE(limenko_qs_p3_w)
ADDRESS_MAP_END

static MACHINE_RESET( limenko )
{
	spriteram_bit = 1;
	prev_sprites_count = 0;
	fillbitmap(sprites_bitmap, 0, 0);
	fillbitmap(sprites_priority, 0, 0);
	memory_set_bankptr(1, memory_region(REGION_CPU2) + 0x100);
	cpunum_set_input_line(1, I8051_INT1_LINE, CLEAR_LINE);
	limenko_sound_reset();
}

static INPUT_PORTS_START(legendoh)
	PORT_START
	PORT_BIT(0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1)
	PORT_BIT(0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1)
	PORT_BIT(0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1)
	PORT_BIT(0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT(0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT(0x00800000, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(1)
	PORT_BIT(0x01000000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(3)
	PORT_BIT(0x02000000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(3)
	PORT_BIT(0x04000000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(3)
	PORT_BIT(0x08000000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(3)
	PORT_BIT(0x10000000, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(3)
	PORT_BIT(0x20000000, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(3)
	PORT_BIT(0x40000000, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(3)
	PORT_BIT(0x80000000, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(3)
	PORT_BIT(0x0000ffff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START
	PORT_BIT(0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2)
	PORT_BIT(0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2)
	PORT_BIT(0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2)
	PORT_BIT(0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(0x00100000, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT(0x00200000, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT(0x00800000, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2)
	PORT_BIT(0x01000000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(4)
	PORT_BIT(0x02000000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(4)
	PORT_BIT(0x04000000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(4)
	PORT_BIT(0x08000000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(4)
	PORT_BIT(0x10000000, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(4)
	PORT_BIT(0x20000000, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(4)
	PORT_BIT(0x40000000, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(4)
	PORT_BIT(0x80000000, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(4)
	PORT_BIT(0x0000ffff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START
	PORT_BIT(0x00010000, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT(0x00020000, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT(0x00040000, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x00080000, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x00100000, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_SERVICE_NO_TOGGLE(0x00200000, IP_ACTIVE_LOW)
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x01000000, IP_ACTIVE_LOW, IPT_START3)
	PORT_BIT(0x02000000, IP_ACTIVE_LOW, IPT_START4)
	PORT_BIT(0x04000000, IP_ACTIVE_LOW, IPT_COIN3)
	PORT_BIT(0x08000000, IP_ACTIVE_LOW, IPT_COIN4)
	PORT_BIT(0x10000000, IP_ACTIVE_LOW, IPT_SERVICE2)
	PORT_DIPNAME(0x20000000, 0x00000000, "Sound Enable")
	PORT_DIPSETTING(         0x20000000, DEF_STR(Off))
	PORT_DIPSETTING(         0x00000000, DEF_STR(On))
	PORT_BIT(0x80000000, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x4000ffff, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static INPUT_PORTS_START(sb2003)
	PORT_START
	PORT_BIT(0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1)
	PORT_BIT(0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1)
	PORT_BIT(0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1)
	PORT_BIT(0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x00100000, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT(0x00200000, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT(0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT(0x00800000, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(1)
	PORT_BIT(0xff00ffff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START
	PORT_BIT(0x00010000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2)
	PORT_BIT(0x00020000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2)
	PORT_BIT(0x00040000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2)
	PORT_BIT(0x00080000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(0x00100000, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT(0x00200000, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT(0x00400000, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT(0x00800000, IP_ACTIVE_LOW, IPT_BUTTON4) PORT_PLAYER(2)
	PORT_BIT(0xff00ffff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START
	PORT_BIT(0x00010000, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT(0x00020000, IP_ACTIVE_LOW, IPT_START2)
	PORT_BIT(0x00040000, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x00080000, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_SERVICE_NO_TOGGLE(0x00200000, IP_ACTIVE_LOW)
	PORT_BIT(0x00400000, IP_ACTIVE_LOW, IPT_SPECIAL)
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_DIPNAME(0x20000000, 0x00000000, "Sound Enable")
	PORT_DIPSETTING(         0x20000000, DEF_STR(Off))
	PORT_DIPSETTING(         0x00000000, DEF_STR(On))
	PORT_BIT(0x80000000, IP_ACTIVE_HIGH, IPT_SPECIAL)
	PORT_BIT(0x00100000, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_BIT(0x5f00ffff, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static const gfx_layout tile_layout =
{
	8, 8, RGN_FRAC(1,1), 8,
	{ GFX_RAW }, { 0 }, { 8*8 }, 8*8*8
};
static const gfx_decode limenko_gfxdecode[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( limenko )
	MDRV_CPU_ADD(E132XN, 80000000)
	MDRV_CPU_PROGRAM_MAP(limenko_map, 0)
	MDRV_CPU_IO_MAP(limenko_io, 0)
	MDRV_CPU_VBLANK_INT(irq5_line_hold, 1)
	MDRV_CPU_ADD(I8052, 24000000)
	MDRV_CPU_PROGRAM_MAP(limenko_qs_program, 0)
	MDRV_CPU_DATA_MAP(limenko_qs_data, 0)
	MDRV_CPU_IO_MAP(limenko_qs_ports, 0)
	MDRV_MACHINE_RESET(limenko)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_NVRAM_HANDLER(93C46)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 240)
	MDRV_VISIBLE_AREA(0, 383, 0, 239)
	MDRV_GFXDECODE(limenko_gfxdecode)
	MDRV_PALETTE_LENGTH(0x1000)
	MDRV_VIDEO_START(limenko)
	MDRV_VIDEO_UPDATE(limenko)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 24000000)
	MDRV_SOUND_CONFIG(limenko_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START(dynabomb)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)
	ROM_LOAD16_WORD_SWAP("rom.u6", 0x000000, 0x200000, CRC(457e015d) SHA1(3afb56cdf903c9084c1f283dc50ec504ce3e199f))

	ROM_REGION32_BE(0x400000, REGION_USER2, ROMREGION_ERASEFF)
	ROM_LOAD16_WORD_SWAP("rom.u5", 0x000000, 0x200000, CRC(7e837adf) SHA1(8613fa187b8d4574b3935aa439aec2515033d64c))

	ROM_REGION(0x80000, REGION_CPU2, 0)
	ROM_LOAD("rom.u16", 0x00000, 0x20000, CRC(f66d7e4d) SHA1(44f1851405ba525f1ed53521f4de12545ea9c46a))
	ROM_FILL(           0x20000, 0x60000, 0xff)

	ROM_REGION(0x800000, REGION_GFX1, 0)
	ROM_LOAD32_BYTE("rom.u1", 0x000000, 0x200000, CRC(bf33eff6) SHA1(089b6d88d6d744bcfa036c6869f0444d6ceb26c9))
	ROM_LOAD32_BYTE("rom.u2", 0x000001, 0x200000, CRC(790bbcd5) SHA1(fc52c15fffc77dc3b3bc89a9606223c4fbaa578c))
	ROM_LOAD32_BYTE("rom.u3", 0x000002, 0x200000, CRC(ec094b12) SHA1(13c105df066ff308cc7e1842907644790946e5b5))
	ROM_LOAD32_BYTE("rom.u4", 0x000003, 0x200000, CRC(88b24e3c) SHA1(5f267f08144b413b55ef5e15c52e9cda096b80e7))

	ROM_REGION(0x1000000, REGION_SOUND2, 0)
	ROM_LOAD("rom.u18",  0x000000, 0x080000, CRC(50d76732) SHA1(6179c7365b62df620a10a1253d524807408821de))
	ROM_LOAD("rom.u17",  0x080000, 0x080000, CRC(20f2417c) SHA1(1bdc0b03215f5002eed4c25d670bbb5411189907))
	ROM_LOAD("qs1003.u4",0x200000, 0x200000, CRC(19e4b469) SHA1(9460e5b6a0fbf3fdd6a9fa0dcbf5062a2e07fe02))


ROM_END

ROM_START(legendoh)
	ROM_REGION32_BE(0x200000, REGION_USER1, ROMREGION_ERASEFF)



	ROM_LOAD16_WORD_SWAP("01.sys_rom4", 0x180000, 0x80000, CRC(49b4a91f) SHA1(21619e8cd0b2fba8c2e08158497575a1760f52c5))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)
	ROM_LOAD16_WORD_SWAP("sys_rom6", 0x000000, 0x200000, CRC(5c13d467) SHA1(ed07b7e1b22293e256787ab079d00c2fb070bf4f))
	ROM_LOAD16_WORD_SWAP("sys_rom5", 0x200000, 0x200000, CRC(19dc8d23) SHA1(433687c6aa24b9456436eecb1dcb57814af3009d))



	ROM_REGION(0x1200000, REGION_GFX1, 0)
	ROM_LOAD32_BYTE("cg_rom10",     0x0000000, 0x200000, CRC(93a48489) SHA1(a14157d31b4e9c8eb7ebe1b2f1b707ec8c8561a0))
	ROM_LOAD32_BYTE("cg_rom20",     0x0000001, 0x200000, CRC(1a6c0258) SHA1(ac7c3b8c2fdfb542103032144a30293d44759fd1))
	ROM_LOAD32_BYTE("cg_rom30",     0x0000002, 0x200000, CRC(a0559ef4) SHA1(6622f7107b374c9da816b9814fe93347e7422190))
	ROM_LOAD32_BYTE("cg_rom40",     0x0000003, 0x200000, CRC(a607b2b5) SHA1(9a6b867d6a777cbc910b98d505367819e0c20077))
	ROM_LOAD32_BYTE("cg_rom11",     0x0800000, 0x200000, CRC(a9fd5a50) SHA1(d15fc4d1697c1505aa98979af09bcfbbc2521145))
	ROM_LOAD32_BYTE("cg_rom21",     0x0800001, 0x200000, CRC(b05cdeb2) SHA1(43115146496ee3a820278ffc0b5f0325d6af6335))
	ROM_LOAD32_BYTE("cg_rom31",     0x0800002, 0x200000, CRC(a9a0d386) SHA1(501af14ea1af70be4862172701af4850750d3f36))
	ROM_LOAD32_BYTE("cg_rom41",     0x0800003, 0x200000, CRC(1c014f45) SHA1(a76246e90b41cc892575f3a3dc26d8d674e3fc3a))
	ROM_LOAD32_BYTE("02.cg_rom12",  0x1000000, 0x080000, CRC(8b2e8cbc) SHA1(6ed6db843e27d715e473752dd3853a28bb81a368))
	ROM_LOAD32_BYTE("03.cg_rom22",  0x1000001, 0x080000, CRC(a35960c8) SHA1(86914701930512cae81d1ad892d482264f80f695))
	ROM_LOAD32_BYTE("04.cg_rom32",  0x1000002, 0x080000, CRC(3f486cab) SHA1(6507d4bb9b4aa7d43f1026e932c82629d4fa44dd))
	ROM_LOAD32_BYTE("05.cg_rom42",  0x1000003, 0x080000, CRC(5d807bec) SHA1(c72c77ed0478f705018519cf68a54d22524d05fd))

	ROM_REGION(0x80000, REGION_CPU2, 0)
	ROM_LOAD("sou_prg.06", 0x000000, 0x80000, CRC(bfafe7aa) SHA1(3e65869fe0970bafb59a0225642834042fdedfa6))

	ROM_REGION(0x1000000, REGION_SOUND2, 0)
	ROM_LOAD("sou_rom.07", 0x000000, 0x080000, CRC(4c6eb6d2) SHA1(58bced7bd944e03b0e3dfe1107c01819a33b2b31))
	ROM_LOAD("sou_rom.08", 0x080000, 0x080000, CRC(42c32dd5) SHA1(4702771288ba40119de63feb67eed85667235d81))
	ROM_LOAD("qs1003.u4",  0x200000, 0x200000, CRC(19e4b469) SHA1(9460e5b6a0fbf3fdd6a9fa0dcbf5062a2e07fe02))
ROM_END

static READ32_HANDLER( dynabomb_speedup_r )
{
	if (activecpu_get_pc() == 0xc25b8)
		activecpu_adjust_icount(-50);
	return mainram[0xe2784 / 4];
}
static READ32_HANDLER( legendoh_speedup_r )
{
	if (activecpu_get_pc() == 0x23e32)
		activecpu_adjust_icount(-50);
	return mainram[0x32ab0 / 4];
}
static DRIVER_INIT( dynabomb )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0xe2784, 0xe2787, 0, 0, dynabomb_speedup_r);
}
static DRIVER_INIT( legendoh )
{
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x32ab0, 0x32ab3, 0, 0, legendoh_speedup_r);
}

GAME( 2000, dynabomb, 0, limenko, sb2003, dynabomb, ROT0, "Limenko", "Dynamite Bomber (Korea, Rev 1.5)", 0 )
GAME( 2000, legendoh, 0, limenko, legendoh, legendoh, ROT0, "Limenko", "Legend of Heroes", 0 )
