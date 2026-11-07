// license:LGPL-2.1+
// copyright-holders:Angelo Salese, Olivier Galibert
/*
  Gemcrush (c) 1996 Star Fish
  TC0091LVC, a variant of the controller used on Taito L hardware.
*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/2203intf.h"

static UINT8 *video_ram, *bitmap_ram;
static UINT8 video_regs[256], sprite_buffer[0x400], palette_ram[0x200];
static UINT8 ram_bank[4], rom_bank, irq_vector[3], irq_enable;
static UINT8 scroll_regs[2][4], dirty_char[2048];
static tilemap *bg_tilemap[2], *tx_tilemap;
static mame_timer *scan_timer;

static const gfx_layout tile_layout =
{
	8, 8, RGN_FRAC(1,1), 4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16 },
	{ 0, 32, 64, 96, 128, 160, 192, 224 },
	256
};
static const gfx_layout sprite_layout =
{
	16, 16, RGN_FRAC(1,1), 4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16, 259, 258, 257, 256, 275, 274, 273, 272 },
	{ 0, 32, 64, 96, 128, 160, 192, 224, 512, 544, 576, 608, 640, 672, 704, 736 },
	1024
};
static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 16 },
	{ REGION_GFX1, 0, &sprite_layout, 0, 16 },
	{ REGION_GFX2, 0, &tile_layout, 0, 16 },
	{ -1 }
};

static void bg_tile_info(int index, int base)
{
	int attr = video_ram[base + index * 2 + 1];
	int code = video_ram[base + index * 2] | ((attr & 3) << 8)
		| (video_regs[(attr >> 2) & 3] << 10);
	SET_TILE_INFO(0, code, attr >> 4, 0)
}
static void get_bg0_tile_info(int tile_index) { bg_tile_info(tile_index, 0x8000); }
static void get_bg1_tile_info(int tile_index) { bg_tile_info(tile_index, 0x9000); }
static void get_tx_tile_info(int tile_index)
{
	int attr = video_ram[0xa000 + tile_index * 2 + 1];
	int code = video_ram[0xa000 + tile_index * 2] | ((attr & 7) << 8);
	SET_TILE_INFO(2, code, attr >> 4, 0)
}

static void restore_video(void)
{
	memory_set_bankptr(1, memory_region(REGION_CPU1) + ((rom_bank & 31) << 13));
	memset(dirty_char, 1, sizeof(dirty_char));
	tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
}

static VIDEO_START( dfruit )
{
	video_ram = memory_region(REGION_GFX2);
	bitmap_ram = auto_malloc(0x20000);
	memset(bitmap_ram, 0, 0x20000);
	bg_tilemap[0] = tilemap_create(get_bg0_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	bg_tilemap[1] = tilemap_create(get_bg1_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 64, 32);
	tx_tilemap = tilemap_create(get_tx_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	if (!bg_tilemap[0] || !bg_tilemap[1] || !tx_tilemap) return 1;
	tilemap_set_transparent_pen(bg_tilemap[0], 0);
	tilemap_set_transparent_pen(tx_tilemap, 0);
	tilemap_set_scrolldx(bg_tilemap[0], 28, -11);
	tilemap_set_scrolldx(bg_tilemap[1], 38, -21);
	tilemap_set_scrolldx(tx_tilemap, -8, -8);
	state_save_register_global_pointer(video_ram, 0x10000);
	state_save_register_global_pointer(bitmap_ram, 0x20000);
	state_save_register_global_array(sprite_buffer);
	state_save_register_global_2d_array(scroll_regs);
	state_save_register_func_postload(restore_video);
	memset(dirty_char, 1, sizeof(dirty_char));
	return 0;
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int pos, flip = video_regs[4] & 0x10;
	for (pos = 0; pos < 0x3e7; pos += 8)
	{
		int code = sprite_buffer[pos] | (sprite_buffer[pos + 1] << 8);
		int color = sprite_buffer[pos + 2] & 15;
		int fx = sprite_buffer[pos + 3] & 1, fy = sprite_buffer[pos + 3] & 2;
		int x = sprite_buffer[pos + 4] | ((sprite_buffer[pos + 5] & 1) << 8);
		int y = sprite_buffer[pos + 6];
		if (x >= cliprect->max_x) x -= 512;
		if (y >= cliprect->max_y) y -= 256;
		if (flip)
		{
			x = 304 - x;
			y = 240 - y;
			fx = !fx;
			fy = !fy;
		}
		pdrawgfx(bitmap, Machine->gfx[1], code, color, fx, fy, x, y,
			cliprect, TRANSPARENCY_PEN, 0, color & 8 ? 0xaa : 0);
	}
}

static VIDEO_UPDATE( dfruit )
{
	int i, changed = 0, flip = video_regs[4] & 0x10;
	if (!(video_regs[4] & 0x20))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	if ((video_regs[4] & 7) == 7)
	{
		int x, y;
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *dest = bitmap->line[y];
			int row = ((flip ? 256 - y : y) & 255) * 512;
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				dest[x] = Machine->pens[bitmap_ram[row + ((flip ? 320 - x : x) & 511)]];
		}
		return;
	}
	for (i = 0; i < 2048; i++)
		if (dirty_char[i])
		{
			decodechar(Machine->gfx[2], i, video_ram, Machine->drv->gfxdecodeinfo[2].gfxlayout);
			dirty_char[i] = 0;
			changed = 1;
		}
	if (changed) tilemap_mark_all_tiles_dirty(tx_tilemap);
	tilemap_set_flip(ALL_TILEMAPS, flip ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0);
	for (i = 0; i < 2; i++)
	{
		int dx = scroll_regs[i][0] | (scroll_regs[i][1] << 8);
		if (flip) dx = ((dx & 0xfffc) | ((dx - 3) & 3)) ^ 15;
		tilemap_set_scrollx(bg_tilemap[i], 0, -dx);
		tilemap_set_scrolly(bg_tilemap[i], 0, -scroll_regs[i][2]);
	}
	fillbitmap(priority_bitmap, 0, cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap[1], 0, 0);
	tilemap_draw(bitmap, cliprect, bg_tilemap[0], 0, video_regs[4] & 8 ? 0 : 1);
	draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, tx_tilemap, 0, 0);
}

static VIDEO_EOF( dfruit )
{
	/* Sprites and scrolling are latched together at the frame boundary. */
	memcpy(sprite_buffer, video_ram + 0xb000, 0x400);
	memcpy(scroll_regs[0], sprite_buffer + 0x3f4, 4);
	memcpy(scroll_regs[1], sprite_buffer + 0x3fc, 4);
}

static READ8_HANDLER( banked_ram_r )
{
	int addr = (ram_bank[(offset >> 12) & 3] << 12) | (offset & 0xfff);
	if (addr >= 0x10000 && addr <= 0x1ffff) return video_ram[addr & 0xffff];
	if (addr >= 0x40000 && addr <= 0x5ffff) return bitmap_ram[addr & 0x1ffff];
	if (addr >= 0x80000 && addr <= 0x80fff) return palette_ram[addr & 0x1ff];
	return 0xff;
}
static WRITE8_HANDLER( banked_ram_w )
{
	int addr = (ram_bank[(offset >> 12) & 3] << 12) | (offset & 0xfff);
	if (addr >= 0x14000 && addr <= 0x1ffff)
	{
		addr &= 0xffff;
		if (video_ram[addr] != data)
		{
			video_ram[addr] = data;
			dirty_char[addr / 32] = 1;
			if (addr >= 0x8000 && addr < 0xa000) tilemap_mark_tile_dirty(bg_tilemap[(addr - 0x8000) >> 12], (addr & 0xfff) / 2);
			else if (addr >= 0xa000 && addr < 0xb000) tilemap_mark_tile_dirty(tx_tilemap, (addr & 0xfff) / 2);
		}
	}
	else if (addr >= 0x40000 && addr <= 0x5ffff) bitmap_ram[addr & 0x1ffff] = data;
	else if (addr >= 0x80000 && addr <= 0x80fff)
	{
		int color;
		addr &= 0x1ff;
		palette_ram[addr] = data;
		color = palette_ram[addr & ~1] | (palette_ram[addr | 1] << 8);
		palette_set_color(addr / 2, pal5bit(((color << 1) & 30) | ((color >> 12) & 1)),
			pal5bit(((color >> 3) & 30) | ((color >> 13) & 1)), pal5bit(((color >> 7) & 30) | ((color >> 14) & 1)));
	}
}
static READ8_HANDLER( video_regs_r ) { return video_regs[offset]; }
static WRITE8_HANDLER( video_regs_w )
{
	if (offset < 4 && video_regs[offset] != data)
	{
		tilemap_mark_all_tiles_dirty(bg_tilemap[0]);
		tilemap_mark_all_tiles_dirty(bg_tilemap[1]);
	}
	video_regs[offset] = data;
}
static READ8_HANDLER( irq_vector_r ) { return irq_vector[offset]; }
static WRITE8_HANDLER( irq_vector_w ) { irq_vector[offset] = data; }
static READ8_HANDLER( irq_enable_r ) { return irq_enable; }
static WRITE8_HANDLER( irq_enable_w ) { irq_enable = data; }
static READ8_HANDLER( ram_bank_r ) { return ram_bank[offset]; }
static WRITE8_HANDLER( ram_bank_w ) { ram_bank[offset] = data; }
static READ8_HANDLER( rom_bank_r ) { return rom_bank; }
static WRITE8_HANDLER( rom_bank_w )
{
	rom_bank = data;
	memory_set_bankptr(1, memory_region(REGION_CPU1) + ((rom_bank & 31) << 13));
}
static void scanline_callback(int scanline)
{
	int next;
	if (scanline == 240 && (irq_enable & 4)) cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, irq_vector[2]);
	if (scanline == 0 && (irq_enable & 2)) cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, irq_vector[1]);
	next = scanline == 0 ? 240 : 0;
	timer_adjust(scan_timer, cpu_getscanlinetime(next), next, 0);
}
static WRITE8_HANDLER( output_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
	coin_lockout_w(0, data & 4);
	coin_lockout_w(1, data & 8);
}
static struct YM2203interface ym_interface = { 0, input_port_3_r, output_w };
static ppi8255_interface ppi_interface = { 1, { input_port_0_r }, { input_port_1_r }, { input_port_2_r } };

static MACHINE_START( dfruit )
{
	scan_timer = timer_alloc(scanline_callback);
	state_save_register_global_array(video_regs);
	state_save_register_global_array(palette_ram);
	state_save_register_global_array(ram_bank);
	state_save_register_global_array(irq_vector);
	state_save_register_global(irq_enable);
	state_save_register_global(rom_bank);
	return 0;
}
static MACHINE_RESET( dfruit )
{
	ppi8255_init(&ppi_interface);
	memset(irq_vector, 0, sizeof(irq_vector));
	memset(ram_bank, 0x80, sizeof(ram_bank));
	memset(video_regs, 0, sizeof(video_regs));
	irq_enable = rom_bank = 0;
	memory_set_bankptr(1, memory_region(REGION_CPU1));
	timer_adjust(scan_timer, cpu_getscanlinetime(0), 0, 0);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x7fff) AM_ROMBANK(1)
	AM_RANGE(0x8000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xa003) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0xa004, 0xa004) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0xa005, 0xa005) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0xa008, 0xa008) AM_READNOP
	AM_RANGE(0xc000, 0xfdff) AM_READWRITE(banked_ram_r, banked_ram_w)
	AM_RANGE(0xfe00, 0xfeff) AM_READWRITE(video_regs_r, video_regs_w)
	AM_RANGE(0xff00, 0xff02) AM_READWRITE(irq_vector_r, irq_vector_w)
	AM_RANGE(0xff03, 0xff03) AM_READWRITE(irq_enable_r, irq_enable_w)
	AM_RANGE(0xff04, 0xff07) AM_READWRITE(ram_bank_r, ram_bank_w)
	AM_RANGE(0xff08, 0xff08) AM_READWRITE(rom_bank_r, rom_bank_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( gemcrush )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_DIPNAME( 0x02, 0x02, "IN0" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "IN1" )
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "IN2" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x20, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( dfruit )
	MDRV_CPU_ADD(Z80, 14000000 / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_START(dfruit)
	MDRV_MACHINE_RESET(dfruit)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 319, 16, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(dfruit)
	MDRV_VIDEO_UPDATE(dfruit)
	MDRV_VIDEO_EOF(dfruit)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2203, 14000000 / 4)
	MDRV_SOUND_CONFIG(ym_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

ROM_START( gemcrush )
	ROM_REGION( 0x40000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "gcj_00.ic2",   0x000000, 0x040000, CRC(e1431390) SHA1(f1f63e23d4b73cc099adddeadcf1ea3e27688bcd) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD( "gcj_01.ic10",  0x000000, 0x080000, CRC(5b9e7a6e) SHA1(345357feed8e80e6a06093fcb69f2b38063d057a) )
	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_ERASE00 )
ROM_END

GAME( 1996, gemcrush, 0, dfruit, gemcrush, 0, ROT270, "Star Fish", "Gemcrush (Japan, prototype)", GAME_NO_COCKTAIL | GAME_SUPPORTS_SAVE )
