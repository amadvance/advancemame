// license:BSD-3-Clause
// copyright-holders:Angelo Salese, David Haywood
/*
    PoPo Bear (c) 2000 BMC
    BMC-A00211 hardware: 68000, AIA90423 graphics, YM2413 and OKIM6295.
*/

#include "driver.h"
#include "sound/2413intf.h"
#include "sound/okim6295.h"

static UINT16 *popo_vram, *popo_spriteram, *popo_vregs;
static UINT8 *tile_pixels;
static UINT8 irq_enable, dsw_select;
static void *scanline_timer;

static WRITE16_HANDLER( popo_vram_w )
{
	unsigned swapped;
	COMBINE_DATA(&popo_vram[offset]);
	/* Keep a byte-ordered tile view without invalidating every tilemap on writes. */
	swapped = (offset & 0x7f000) | ((offset & 0x1fc) << 3) | ((offset & 0xe00) >> 7) | (offset & 3);
	tile_pixels[swapped * 2] = popo_vram[offset] >> 8;
	tile_pixels[swapped * 2 + 1] = popo_vram[offset];
}

static void popo_draw_layer(mame_bitmap *bitmap, const rectangle *clip, int layer)
{
	static const int xregs[4] = { 2, 4, 7, 9 };
	static const int yregs[4] = { 3, 5, 8, 10 };
	int enable = (popo_vregs[12 + layer / 2] >> ((layer & 1) ? 0 : 8)) & 255;
	int ymask = (enable & 8) ? 511 : 255;
	int y;

	if (!enable)
		return;
	for (y = clip->min_y; y <= clip->max_y; ++y)
	{
		int scrollx, sy, x = clip->min_x;
		if (enable & 2)
		{
			unsigned base = (popo_vregs[yregs[layer]] << 8) & 0x7ffff;
			unsigned hi = (popo_vregs[layer < 2 ? 6 : 11] << 8) & 0x7ffff;
			UINT16 value = popo_vram[(base + y) & 0x7ffff];
			int upper = (popo_vram[(hi + y) & 0x7ffff] >> ((layer & 1) ? 8 : 0)) & 255;
			scrollx = (value & 255) | (upper << 8);
			sy = (value >> 8) & ymask;
		}
		else
		{
			scrollx = popo_vregs[xregs[layer]];
			sy = (y + popo_vregs[yregs[layer]]) & ymask;
		}
		while (x <= clip->max_x)
		{
			int sx = (x + scrollx) & 1023;
			UINT16 tile = popo_vram[0x78000 + layer * 0x2000 + (sy >> 3) * 128 + (sx >> 3)];
			int py = (sy & 7) ^ ((tile & 0x8000) ? 7 : 0);
			int px = sx & 7;
			int count = MIN(8 - px, clip->max_x - x + 1);
			const UINT8 *src = tile_pixels + (tile & 0x3fff) * 64 + py * 8;
			int i;
			for (i = 0; i < count; ++i)
			{
				UINT8 pen = src[(px + i) ^ ((tile & 0x4000) ? 7 : 0)];
				if (pen)
					plot_pixel(bitmap, x + i, y, Machine->pens[pen]);
			}
			x += count;
		}
	}
}

static void popo_draw_sprites(mame_bitmap *bitmap, const rectangle *clip)
{
	int priority, i;
	for (priority = 15; priority >= 0; --priority)
		for (i = 0x7fff8 / 2; i >= 0x7f800 / 2; i -= 4)
		{
			UINT16 param = popo_spriteram[i];
			int x, y, size, bank, mask, add, xi, yi;
			unsigned address;
			if (((param >> 8) & 15) != priority)
				continue;
			/* The list terminates in priority zero after the higher groups are drawn. */
			if (!param)
				return;
			y = (INT16)popo_spriteram[i + 1];
			x = (INT16)popo_spriteram[i + 2];
			size = 8 << ((param >> 4) & 3);
			address = popo_spriteram[i + 3] << 3;
			bank = (param >> 2) & 3;
			if (param & 0xf000)
				bank = mame_rand() & 3;
			mask = (param & 3) == 0 ? 255 : (param & 3) == 2 ? 63 : 31;
			add = (param & 3) ? bank * 64 : 0;
			if ((param & 3) == 3)
				add |= 32;
			for (yi = 0; yi < size; ++yi)
			{
				int dy = y + ((param & 0x80) ? size - 1 - yi : yi);
				for (xi = 0; xi < size; ++xi, ++address)
				{
					int dx = x + ((param & 0x40) ? size - 1 - xi : xi);
					UINT8 pen = (popo_spriteram[(address & 0x7ffff) >> 1] >> ((address & 1) ? 0 : 8)) & mask;
					if (pen && dx >= clip->min_x && dx <= clip->max_x && dy >= clip->min_y && dy <= clip->max_y)
						plot_pixel(bitmap, dx, dy, Machine->pens[256 + (pen | add)]);
				}
			}
		}
}

static VIDEO_START( popobear )
{
	tile_pixels = auto_malloc(0x100000);
	memset(tile_pixels, 0, 0x100000);
	return 0;
}

static VIDEO_UPDATE( popobear )
{
	int layer;
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	/* Diagnostic screens select a linear framebuffer instead of tile layers. */
	if (!popo_vregs[12] && !popo_vregs[13] && (popo_vregs[14] & 0x20))
	{
		unsigned base = (popo_vregs[14] & 15) << 16;
		int x, y;
		for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			{
				unsigned address = (base + y * 512 + x) & 0xfffff;
				UINT8 pen = popo_vram[address >> 1] >> ((address & 1) ? 0 : 8);
				plot_pixel(bitmap, x, y, Machine->pens[pen]);
			}
	}
	for (layer = 3; layer >= 0; --layer)
		popo_draw_layer(bitmap, cliprect, layer);
	if (popo_vregs[0] & 0x100)
		popo_draw_sprites(bitmap, cliprect);
}

static WRITE16_HANDLER( popo_irq_w )
{
	int i;
	if (ACCESSING_MSB)
		irq_enable = data >> 8;
	if (ACCESSING_LSB)
		for (i = 1; i < 8; ++i)
			if (data & (1 << i))
				cpunum_set_input_line(0, i, CLEAR_LINE);
}

static void popo_scanline(int scanline)
{
	int next, irq;
	switch (scanline)
	{
		case 0: irq = 5; next = 64; break;
		case 64: irq = 2; next = 192; break;
		case 192: irq = 2; next = 240; break;
		default: irq = 3; next = 0; break;
	}
	if (irq_enable & (1 << irq))
		cpunum_set_input_line(0, irq, ASSERT_LINE);
	timer_adjust(scanline_timer, cpu_getscanlinetime(next), next, 0);
}

static READ16_HANDLER( popo_timer_r ) { return activecpu_gettotalcycles() >> 8; }
/* The power-on check requires bits 1 and 2 clear in the upper status byte. */
static READ16_HANDLER( popo_status_r ) { return 0x09ff; }

static READ16_HANDLER( popo_input_r )
{
	return (readinputport(1) << 8) | readinputport(2 + dsw_select);
}

static WRITE16_HANDLER( popo_control_w )
{
	if (ACCESSING_MSB)
		dsw_select = (data >> 8) & 3;
}

static WRITE16_HANDLER( popo_coin_w )
{
	if (ACCESSING_LSB)
		coin_counter_w(0, data & 2);
}

static ADDRESS_MAP_START( popo_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS(AMEF_UNMAP(1))
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x210000, 0x21ffff) AM_RAM
	AM_RANGE(0x280000, 0x2fffff) AM_RAM AM_BASE(&popo_spriteram)
	AM_RANGE(0x300000, 0x3fffff) AM_READWRITE(MRA16_RAM, popo_vram_w) AM_BASE(&popo_vram)
	AM_RANGE(0x480000, 0x48001f) AM_RAM AM_BASE(&popo_vregs)
	AM_RANGE(0x480020, 0x48002f) AM_RAM
	AM_RANGE(0x480032, 0x480033) AM_RAM
	AM_RANGE(0x480036, 0x480039) AM_RAM
	AM_RANGE(0x48003c, 0x48003f) AM_RAM
	AM_RANGE(0x480030, 0x480031) AM_WRITE(popo_irq_w)
	AM_RANGE(0x480034, 0x480035) AM_READWRITE(MRA16_NOP, popo_coin_w)
	AM_RANGE(0x48003a, 0x48003b) AM_READ(popo_timer_r)
	AM_RANGE(0x480400, 0x4807ff) AM_READWRITE(MRA16_RAM, paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x520000, 0x520001) AM_READ(popo_input_r)
	AM_RANGE(0x540000, 0x540001) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0x550000, 0x550001) AM_WRITE(YM2413_register_port_0_lsb_w)
	AM_RANGE(0x550002, 0x550003) AM_WRITE(YM2413_data_port_0_lsb_w)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(popo_control_w)
	AM_RANGE(0x620000, 0x620001) AM_READWRITE(popo_status_r, MWA16_NOP)
	AM_RANGE(0x800000, 0xbfffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static MACHINE_RESET( popobear )
{
	int i;
	irq_enable = dsw_select = 0;
	for (i = 1; i < 8; ++i)
		cpunum_set_input_line(0, i, CLEAR_LINE);
	timer_adjust(scanline_timer, cpu_getscanlinetime(0), 0, 0);
}

static DRIVER_INIT( popobear ) { scanline_timer = timer_alloc(popo_scanline); }

INPUT_PORTS_START( popobear )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Coin_A" ) PORT_DIPLOCATION("SW1:2,3,4")
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW1:5,6")
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW1:7,8")
	PORT_DIPSETTING(    0x40, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "Pilot Lamp" ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( popobear )
	MDRV_CPU_ADD(M68000, 42000000 / 4)
	MDRV_CPU_PROGRAM_MAP(popo_map, 0)
	MDRV_MACHINE_RESET(popobear)
	MDRV_FRAMES_PER_SECOND(59.64)
	MDRV_VBLANK_DURATION(2500)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024, 256)
	MDRV_VISIBLE_AREA(0, 479, 0, 239)
	MDRV_PALETTE_LENGTH(512)
	MDRV_VIDEO_START(popobear)
	MDRV_VIDEO_UPDATE(popobear)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2413, 42000000 / 12)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( popobear )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "popobear_en-a-401_1.6.u4", 0x000000, 0x020000, CRC(0568af9c) SHA1(920531dbc4bbde2d1db062bd5c48b97dd50b7185) )
	ROM_LOAD16_BYTE( "popobear_en-a-301_1.6.u3", 0x000001, 0x020000, CRC(b934adf6) SHA1(93431c7a19af812b549aad35cc1176a81805ffab) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "popobear_en-a-601.u6",     0x000000, 0x100000, CRC(84fa9f3f) SHA1(34dd7873f88b0dae5fb81fe84e82d2b6b49f7332) )
	ROM_LOAD16_BYTE( "popobear_en-a-501.u5",     0x000001, 0x100000, CRC(185901a9) SHA1(7ff82b5751645df53435eaa66edce589684cc5c7) )
	ROM_LOAD16_BYTE( "popobear_en-a-801.u8",     0x200000, 0x100000, CRC(2760f2e6) SHA1(58af59f486c9df930f7c124f89154f8f389a5bd7) )
	ROM_LOAD16_BYTE( "popobear_en-a-701.u7",     0x200001, 0x100000, CRC(45eba6d0) SHA1(0278602ed57ac45040619d590e6cc85e2cfeed31) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )
	ROM_LOAD( "popobear_ta-a-901.u9", 0x00000, 0x40000,  CRC(f1e94926) SHA1(f4d6f5b5811d90d0069f6efbb44d725ff0d07e1c) )
ROM_END

GAME( 2000, popobear, 0, popobear, popobear, popobear, ROT0, "BMC", "PoPo Bear", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
