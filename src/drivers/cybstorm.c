// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Phil Bennett
/*
    Atari Cyberstorm hardware
*/

#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"

static UINT32 latch_data;
static UINT8 alpha_tile_bank;
static UINT16 *video_ram;

static void get_alpha_tile_info(int tile_index)
{
	UINT16 data = atarigen_alpha[tile_index];
	int code = ((data & 0x400) ? alpha_tile_bank * 0x400 : 0) + (data & 0x3ff);
	SET_TILE_INFO(2, code, (data >> 11) & 15, (data & 0x8000) ? TILE_IGNORE_TRANSPARENCY : 0)
}

static void get_playfield_tile_info(int tile_index)
{
	UINT16 attr = atarigen_playfield_upper[tile_index] & 0xff;
	SET_TILE_INFO(0, atarigen_playfield[tile_index], 8 + (attr & 7), (attr & 0x80) ? TILE_FLIPX : 0)
	tile_info.priority = (attr >> 4) & 3;
}

static void get_playfield2_tile_info(int tile_index)
{
	UINT16 attr = atarigen_playfield_upper[tile_index] >> 8;
	SET_TILE_INFO(0, atarigen_playfield2[tile_index], attr & 7, (attr & 0x80) ? TILE_FLIPX : 0)
	tile_info.priority = (attr >> 4) & 3;
}

static VIDEO_START( cybstorm )
{
	static const struct atarimo_desc desc = {
		1, 1, 1, 0, 1, 0, 0, 8, 0, 0,
		0x1000, 0x1000, 0,
		{{0x03ff,0,0,0}},
		{{0}},
		{{0,0x7fff,0,0}},
		{{0x3c00,0,0,0}},
		{{0,0,0x000f,0}},
		{{0,0,0xff80,0}},
		{{0,0,0,0xff80}},
		{{0,0,0,0x0070}},
		{{0,0,0,0x0007}},
		{{0,0x8000,0,0}},
		{{0}},
		{{0,0,0x0070,0}},
		{{0}}, {{0}}, {{0}}, 0, 0
	};

	/* Sprite color banks are spaced by 256 despite having six pixel planes. */
	Machine->gfx[1]->color_granularity = 256;
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 8, 8, 64, 64);
	atarigen_playfield2_tilemap = tilemap_create(get_playfield2_tile_info, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	atarigen_alpha_tilemap = tilemap_create(get_alpha_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 30);
	if (!atarigen_playfield_tilemap || !atarigen_playfield2_tilemap || !atarigen_alpha_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_playfield2_tilemap, 0);
	tilemap_set_transparent_pen(atarigen_alpha_tilemap, 0);
	return !atarimo_init(0, &desc);
}

static VIDEO_UPDATE( cybstorm )
{
	struct atarimo_rect_list rects;
	mame_bitmap *mobitmap;
	int i, x, y;
	fillbitmap(priority_bitmap, 0, cliprect);
	for (i = 0; i < 4; ++i)
		tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, i, i);
	for (i = 0; i < 4; ++i)
		tilemap_draw(bitmap, cliprect, atarigen_playfield2_tilemap, i, 0x80 | (i << 2));
	mobitmap = atarimo_render(0, cliprect, &rects);
	for (i = 0; i < rects.numrects; ++i)
		for (y = rects.rect[i].min_y; y <= rects.rect[i].max_y; ++y)
		{
			UINT16 *mo = mobitmap->line[y];
			UINT16 *pf = bitmap->line[y];
			UINT8 *pri = priority_bitmap->line[y];
			for (x = rects.rect[i].min_x; x <= rects.rect[i].max_x; ++x)
				if (mo[x])
				{
					int mopriority = mo[x] >> ATARIMO_PRIORITY_SHIFT;
					if ((mopriority & 4) && (mopriority & 3))
						continue;
					if ((pri[x] & 0x80) ? (mopriority > ((pri[x] >> 2) & 3)) : ((pri[x] & 3) != 3))
						pf[x] = (mo[x] & ATARIMO_DATA_MASK) | 0x1000;
				}
		}
	/* Preserve the sprite markers until the stain pass has consumed them. */
	for (i = 0; i < rects.numrects; ++i)
		for (y = rects.rect[i].min_y; y <= rects.rect[i].max_y; ++y)
		{
			UINT16 *mo = mobitmap->line[y];
			UINT16 *pf = bitmap->line[y];
			int count = 0;
			for (x = rects.rect[i].min_x; x <= rects.rect[i].max_x || (count && x < bitmap->width); ++x)
			{
				int marker = mo[x] & ((4 << ATARIMO_PRIORITY_SHIFT) | 0x3f);
				if (marker == ((4 << ATARIMO_PRIORITY_SHIFT) | 3))
					++count;
				if (count && !(pf[x] & 0x1000))
					pf[x] |= 0x2000;
				if (marker == ((4 << ATARIMO_PRIORITY_SHIFT) | 7))
					--count;
				mo[x] = 0;
			}
		}
	tilemap_draw(bitmap, cliprect, atarigen_alpha_tilemap, 0, 0);
}

static void cybstorm_scanline_update(int scanline)
{

	if (scanline <= Machine->visible_area.max_y && atarivc_state.rowscroll_enable)
	{
		UINT16 *base = &atarigen_alpha[scanline / 8 * 64 + 48];
		int scan, i;

		for (scan = 0; scan < 8; scan++, scanline++)
			for (i = 0; i < 2; i++)
			{
				int data = *base++;
				switch (data & 15)
				{
					case 9:
						force_partial_update(scanline - 1);
						atarivc_state.mo_xscroll = (data >> 7) & 0x1ff;
						atarimo_set_xscroll(0, atarivc_state.mo_xscroll);
						break;

					case 10:
						force_partial_update(scanline - 1);
						atarivc_state.pf1_xscroll_raw = (data >> 7) & 0x1ff;
						atarivc_update_pf_xscrolls();
						tilemap_set_scrollx(atarigen_playfield_tilemap, 0, atarivc_state.pf0_xscroll);
						tilemap_set_scrollx(atarigen_playfield2_tilemap, 0, atarivc_state.pf1_xscroll);
						break;

					case 11:
						force_partial_update(scanline - 1);
						atarivc_state.pf0_xscroll_raw = (data >> 7) & 0x1ff;
						atarivc_update_pf_xscrolls();
						tilemap_set_scrollx(atarigen_playfield_tilemap, 0, atarivc_state.pf0_xscroll);
						break;

					case 13:
						force_partial_update(scanline - 1);
						atarivc_state.mo_yscroll = (data >> 7) & 0x1ff;
						atarimo_set_yscroll(0, atarivc_state.mo_yscroll);
						break;

					case 14:
						force_partial_update(scanline - 1);
						atarivc_state.pf1_yscroll = (data >> 7) & 0x1ff;
						tilemap_set_scrolly(atarigen_playfield2_tilemap, 0, atarivc_state.pf1_yscroll);
						break;

					case 15:
						force_partial_update(scanline - 1);
						atarivc_state.pf0_yscroll = (data >> 7) & 0x1ff;
						tilemap_set_scrolly(atarigen_playfield_tilemap, 0, atarivc_state.pf0_yscroll);
						break;
				}
			}
	}
}

static void update_interrupts(void)
{
	cpunum_set_input_line(0, 4, atarigen_scanline_int_state ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(0, 6, atarigen_sound_int_state ? ASSERT_LINE : CLEAR_LINE);
}

static MACHINE_RESET( cybstorm )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarivc_reset(atarivc_eof_data, 2);
	atarigen_scanline_timer_reset(cybstorm_scanline_update, 8);
	atarijsa_reset();
}

static WRITE32_HANDLER( latch_w )
{
	UINT32 old = latch_data;
	COMBINE_DATA(&latch_data);
	cpunum_set_input_line(1, INPUT_LINE_RESET, (latch_data & 0x00100000) ? CLEAR_LINE : ASSERT_LINE);
	if ((old ^ latch_data) & 0x00e00000)
	{
		force_partial_update(cpu_getscanline());
		tilemap_mark_all_tiles_dirty(atarigen_alpha_tilemap);
		alpha_tile_bank = (latch_data >> 21) & 7;
	}
}

static READ32_HANDLER( controls_r )
{
	return readinputport(0);
}

static READ32_HANDLER( status_r )
{
	UINT32 result = readinputport(1);
	if (atarigen_get_hblank()) result ^= 0x00010000;
	if (atarigen_sound_to_cpu_ready) result ^= 0x00100000;
	if (atarigen_cpu_to_sound_ready) result ^= 0x00200000;
	return result;
}

static READ32_HANDLER( sound_r )
{
	if ((mem_mask & 0x00ff0000) != 0x00ff0000)
		return ((atarigen_sound_r(0, 0) & 0xff) << 16) | 0xff00ffff;
	return 0xffffffff;
}

static WRITE32_HANDLER( sound_w )
{
	if ((mem_mask & 0x00ff0000) != 0x00ff0000)
		atarigen_sound_w(0, data >> 16, 0xff00);
}

static READ32_HANDLER( vc_r )
{
	return ((UINT32)atarivc_r(offset * 2, mem_mask >> 16) << 16) | atarivc_r(offset * 2 + 1, mem_mask);
}

static WRITE32_HANDLER( vc_w )
{
	if ((mem_mask & 0xffff0000) != 0xffff0000) atarivc_w(offset * 2, data >> 16, mem_mask >> 16);
	if ((mem_mask & 0x0000ffff) != 0x0000ffff) atarivc_w(offset * 2 + 1, data, mem_mask);
}

static READ32_HANDLER( video_r )
{
	return ((UINT32)video_ram[offset * 2] << 16) | video_ram[offset * 2 + 1];
}

static void video_word_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	switch (offset >> 12)
	{
		case 0: atarigen_playfield2_latched_msb_w(offset & 0xfff, data, mem_mask); break;
		case 1: atarigen_playfield_latched_lsb_w(offset & 0xfff, data, mem_mask); break;
		case 2: atarigen_playfield_dual_upper_w(offset & 0xfff, data, mem_mask); break;
		case 3: atarimo_0_spriteram_w(offset & 0xfff, data, mem_mask); break;
		case 4:
			if (offset < 0x4780)
				atarigen_alpha_w(offset & 0xfff, data, mem_mask);
			else if (offset >= 0x47c0)
				atarimo_0_slipram_w(offset & 0x3f, data, mem_mask);
			else
				COMBINE_DATA(&video_ram[offset]);
			break;
		default: COMBINE_DATA(&video_ram[offset]); break;
	}
}

static WRITE32_HANDLER( video_w )
{
	/* Split in bus order and retain both byte masks for the 16-bit latches. */
	if ((mem_mask & 0xffff0000) != 0xffff0000) video_word_w(offset * 2, data >> 16, mem_mask >> 16);
	if ((mem_mask & 0x0000ffff) != 0x0000ffff) video_word_w(offset * 2 + 1, data, mem_mask);
}

static WRITE32_HANDLER( palette_w )
{
	int half;
	COMBINE_DATA(&paletteram32[offset]);
	/* Byte stores must refresh the color even when the other byte is masked. */
	for (half = 0; half < 2; ++half)
	{
		int shift = half ? 0 : 16;
		if (((mem_mask >> shift) & 0xffff) != 0xffff)
		{
			UINT16 value = paletteram32[offset] >> shift;
			int intensity = value >> 15;
			int r = ((value >> 9) & 0x3e) | intensity;
			int g = ((value >> 4) & 0x3e) | intensity;
			int b = ((value << 1) & 0x3e) | intensity;
			palette_set_color(offset * 2 + half, pal6bit(r), pal6bit(g), pal6bit(b));
		}
	}
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM AM_WRITE(palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0x3effc0, 0x3effff) AM_READWRITE(vc_r, vc_w)
	AM_RANGE(0x3f0000, 0x3fffff) AM_READWRITE(video_r, video_w)
	AM_RANGE(0x9f0000, 0x9f0003) AM_READ(controls_r)
	AM_RANGE(0x9f0010, 0x9f0013) AM_READ(status_r)
	AM_RANGE(0x9f0030, 0x9f0033) AM_READ(sound_r)
	AM_RANGE(0x9f0040, 0x9f0043) AM_WRITE(sound_w)
	AM_RANGE(0x9f0050, 0x9f0053) AM_WRITE(latch_w)
	AM_RANGE(0xfb0000, 0xfb0003) AM_WRITE(watchdog_reset32_w)
	AM_RANGE(0xfc0000, 0xfc0003) AM_WRITE(atarigen_eeprom_enable32_w)
	AM_RANGE(0xfd0000, 0xfd0fff) AM_READWRITE(atarigen_eeprom_upper32_r, atarigen_eeprom32_w) AM_BASE((UINT32 **)&atarigen_eeprom) AM_SIZE(&atarigen_eeprom_size)
	AM_RANGE(0xfe0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static INPUT_PORTS_START( cybstorm )
	PORT_START

	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Jab Punch")
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Strong Punch")
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Fierce Punch")
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Quick Kick")
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2) PORT_NAME("P2 Strong Kick")
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2) PORT_NAME("P2 Fierce Kick")
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Quick Kick")
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1) PORT_NAME("P1 Strong Kick")
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1) PORT_NAME("P1 Fierce Kick")
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Jab Punch")
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Strong Punch")
	PORT_BIT( 0x08000000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Fierce Punch")
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)

	PORT_START

	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x0000ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_SERVICE( 0x00400000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )
	JSA_III_PORT
INPUT_PORTS_END

static const gfx_layout anlayout = {
	8,8, RGN_FRAC(1,1), 2, {0,4},
	{0,1,2,3,8,9,10,11}, {0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16}, 8*16
};

static const gfx_layout pflayout = {
	8,8, RGN_FRAC(1,8), 8,
	{RGN_FRAC(7,8),RGN_FRAC(6,8),RGN_FRAC(5,8),RGN_FRAC(4,8),RGN_FRAC(3,8),RGN_FRAC(2,8),RGN_FRAC(1,8),RGN_FRAC(0,8)},
	{0,1,2,3,4,5,6,7}, {0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8}, 8*8
};

static const gfx_layout molayout = {
	8,8, RGN_FRAC(1,6), 6,
	{RGN_FRAC(5,6),RGN_FRAC(4,6),RGN_FRAC(3,6),RGN_FRAC(2,6),RGN_FRAC(1,6),RGN_FRAC(0,6)},
	{0,1,2,3,4,5,6,7}, {0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8}, 8*8
};

static const gfx_decode gfxdecodeinfo[] = {
	{REGION_GFX1, 0, &pflayout, 0, 16},
	{REGION_GFX2, 0, &molayout, 4096, 64},
	{REGION_GFX3, 0, &anlayout, 16384, 64},
	{-1}
};

static MACHINE_DRIVER_START( cybstorm )
	MDRV_CPU_ADD(M68EC020, 14318181)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_MACHINE_RESET(cybstorm)
	MDRV_NVRAM_HANDLER(atarigen)
	MDRV_FRAMES_PER_SECOND(14318181.0 / (2 * 456 * 262))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 2 * 456 * 22 / 14318181))
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(336, 262)
	MDRV_VISIBLE_AREA(0, 335, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_VIDEO_START(cybstorm)
	MDRV_VIDEO_UPDATE(cybstorm)
	MDRV_IMPORT_FROM(jsa_iiis_stereo)
MACHINE_DRIVER_END

ROM_START( cybstorm )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "st_11.22.prog.6a",   0x000000, 0x080000, CRC(8b112ee9) SHA1(cd8367c47c653b8a1ba236c354f009f4297d521d) )
	ROM_LOAD32_BYTE( "st_11.22.prog.8a",   0x000001, 0x080000, CRC(36b7cec9) SHA1(c9c2ba6df1fc849200e0c66a7cbc292e8b0b22f3) )
	ROM_LOAD32_BYTE( "st_11.22.prog2.13a", 0x000002, 0x080000, CRC(1318f2c5) SHA1(929fbe96621852a10b7072490e1e554cdb2f20d8) )
	ROM_LOAD32_BYTE( "st_11.22.prog.16a",  0x000003, 0x080000, CRC(4ae586a8) SHA1(daa803ed38f6582677b397e744dd8f5f60cfb508) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )
	ROM_LOAD( "st_11.22.6502", 0x010000, 0x004000, CRC(947421b2) SHA1(72b2b66122e779135f1f5af794e4d8513ccbbef6) )
	ROM_CONTINUE(              0x004000, 0x00c000 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "st_11.22.csalph.6c", 0x000000, 0x020000, CRC(bafa4bbe) SHA1(c033a952fab6eb3a06c44ba7f48e58b20fe144f0) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_INVERT | ROMREGION_DISPOSE )
	ROM_LOAD( "st_11.22.pf0", 0x000000, 0x080000, CRC(0cf5874c) SHA1(1d739a3fc42baa5556aa22e051c873db9357396f) )
	ROM_LOAD( "st_11.22.pf1", 0x080000, 0x080000, CRC(ee0a6a81) SHA1(7c36ccbcd51497ea8a872ddf7dabe2ceb0895408) )
	ROM_LOAD( "st_11.22.pf2", 0x100000, 0x080000, CRC(03791514) SHA1(0688b55015f8d86ee92497cb7fcdfbdbfbc492a2) )
	ROM_LOAD( "st_11.22.pf3", 0x180000, 0x080000, CRC(8daf8d8f) SHA1(bfe90c789df5952f2e55c6cceebbf1285ea8d18e) )
	ROM_LOAD( "st_11.22.pf4", 0x200000, 0x080000, CRC(c0f759ab) SHA1(6acbc7c12669c89efeec9218eba03523748e4bf2) )
	ROM_LOAD( "st_11.22.pf5", 0x280000, 0x080000, CRC(921a080e) SHA1(7d3ef110569bacacfa269fab63777bf7ffb4c68e) )
	ROM_LOAD( "st_11.22.pf6", 0x300000, 0x080000, CRC(58b3c0d9) SHA1(226ff2e948c5bb0ca150700a2f3426492fce79f7) )
	ROM_LOAD( "st_11.22.pf7", 0x380000, 0x080000, CRC(f84b27ca) SHA1(a7812e18e15fad9992a59b0ebd177cb848a743bb) )

	ROM_REGION( 0xc00000, REGION_GFX2, ROMREGION_INVERT | ROMREGION_DISPOSE )
	ROM_LOAD( "st_11.22.mo00", 0x000000, 0x080000, CRC(216ffdb9) SHA1(7e6418da1419d82e67bef9ae314781708ed62a76) )
	ROM_LOAD( "st_11.22.mo01", 0x200000, 0x080000, CRC(af15908b) SHA1(9dc8dbf0288a084891bdd646cfb7b8c97b89cf2e) )
	ROM_LOAD( "st_11.22.mo02", 0x400000, 0x080000, CRC(fc066982) SHA1(bbf258ff23619234cb31b4afab4eac1681cdeae0) )
	ROM_LOAD( "st_11.22.mo03", 0x600000, 0x080000, CRC(95c85715) SHA1(585cdb3cadfcd205e7dfcf846230b404093cd018) )
	ROM_LOAD( "st_11.22.mo04", 0x800000, 0x080000, CRC(f53cebc8) SHA1(91280f8bf9f2fbb977f3971324134ffd8eec11b9) )
	ROM_LOAD( "st_11.22.mo05", 0xa00000, 0x080000, CRC(6c696989) SHA1(1a688252faa85a380fd639950068b28de0b50cdf) )
	ROM_LOAD( "st_11.22.mo10", 0x080000, 0x080000, CRC(a65b00da) SHA1(12a482e58207ac6fc2f7a47da1162a38ea902a96) )
	ROM_LOAD( "st_11.22.mo11", 0x280000, 0x080000, CRC(11da3f44) SHA1(18f228524e2a00655f4e965208f99d892741d7cb) )
	ROM_LOAD( "st_11.22.mo12", 0x480000, 0x080000, CRC(44257e7d) SHA1(307f350908b5f8d53495368e19a02e1042d9cb03) )
	ROM_LOAD( "st_11.22.mo13", 0x680000, 0x080000, CRC(8ec4cc3e) SHA1(20e74ce2aa60cd2eb67a39b1bd8cf37454db4776) )
	ROM_LOAD( "st_11.22.mo14", 0x880000, 0x080000, CRC(8f144f42) SHA1(6a35cf399775aaae50eeb35812fcf1343d9f9e1a) )
	ROM_LOAD( "st_11.22.mo15", 0xa80000, 0x080000, CRC(3de4035a) SHA1(ecba9b464eb8b37a85a1c81ee4d7935a11646ed9) )
	ROM_LOAD( "st_11.22.mo20", 0x100000, 0x080000, CRC(6f79ef90) SHA1(e323a7c35dac021c6b32870938dd54e865014078) )
	ROM_LOAD( "st_11.22.mo21", 0x300000, 0x080000, CRC(69726b74) SHA1(31ba5ac1584ba6b63ec4a2189d531319db716690) )
	ROM_LOAD( "st_11.22.mo22", 0x500000, 0x080000, CRC(5323d1f4) SHA1(65e81316467a3e5413ef480ba278a0a70e5c2f51) )
	ROM_LOAD( "st_11.22.mo23", 0x700000, 0x080000, CRC(2387c947) SHA1(26d0f1ee83c5e84df4d143d6d3fd422f34d0d9af) )
	ROM_LOAD( "st_11.22.mo24", 0x900000, 0x080000, CRC(2f133ccf) SHA1(a280879de200eb10e495e8ca804515c0e7d02eb9) )
	ROM_LOAD( "st_11.22.mo25", 0xb00000, 0x080000, CRC(0746b2c5) SHA1(5414c3e600352cfb88d521ed3252941893afe438) )
	ROM_LOAD( "st_11.22.mo30", 0x180000, 0x080000, CRC(7c0f2a9b) SHA1(82a405203356643529017201797b37efb7d1b227) )
	ROM_LOAD( "st_11.22.mo31", 0x380000, 0x080000, CRC(8c21a84c) SHA1(7e84afb2b72af32af731df25fa1dd4d5f4dde7ba) )
	ROM_LOAD( "st_11.22.mo32", 0x580000, 0x080000, CRC(a7382479) SHA1(c694e4ca4111252c3b23e274695ed235dd8a92e2) )
	ROM_LOAD( "st_11.22.mo33", 0x780000, 0x080000, CRC(eac63276) SHA1(d72e85c46ce609b6275280c8cd73ed93aa6eddc3) )
	ROM_LOAD( "st_11.22.mo34", 0x980000, 0x080000, CRC(b8b0c8b6) SHA1(f47218a4d94aa151964687a6e4c02f2b3065fdd3) )
	ROM_LOAD( "st_11.22.mo35", 0xb80000, 0x080000, CRC(f0b9cf9d) SHA1(7ce30b05c1ee02346e8f568f36274b46d1ed99c4) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "st_11.22.5a", 0x080000, 0x080000, CRC(d469692c) SHA1(b7d94c042cf9f28ea65d44f5305d56459562d209) )

ROM_END

static DRIVER_INIT( cybstorm )
{
	video_ram = auto_malloc(0x10000);
	memset(video_ram, 0, 0x10000);
	atarigen_playfield2 = video_ram;
	atarigen_playfield = video_ram + 0x1000;
	atarigen_playfield_upper = video_ram + 0x2000;
	atarimo_0_spriteram = video_ram + 0x3000;
	atarigen_alpha = video_ram + 0x4000;
	atarivc_eof_data = video_ram + 0x4780;
	atarimo_0_slipram = video_ram + 0x47c0;
	atarivc_data = auto_malloc(0x40);
	atarigen_eeprom_default = 0;
	atarijsa_init(1, 2, 1, 0x00400000);
	atarijsa3_init_adpcm(REGION_SOUND1);
}

GAME( 1993, cybstorm, 0, cybstorm, cybstorm, cybstorm, ROT0, "Atari Games", "Cyberstorm (prototype)", GAME_IMPERFECT_GRAPHICS )
