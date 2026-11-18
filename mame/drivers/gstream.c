// license:BSD-3-Clause
// copyright-holders:David Haywood
/*
    G-Stream (c)2002 Oriental Soft Japan
    X2222 (prototype) (c)2000 Oriental Soft

    CPU:  E1-32XT
    Sound: 2x AD-65 (OKIM6295 clone)
*/

#include "driver.h"
#include "sound/okim6295.h"

#define GSTREAM_PALETTE_ENTRIES 0x1c00
#define GSTREAM_PALETTE_PENS    0x2000

static UINT32 *workram, *video_ram;
static UINT16 scroll_x[3], scroll_y[3];
static UINT16 *rgb_tiles[4];
static UINT8 x2222_toggle;

static NVRAM_HANDLER( gstream )
{
	if (read_or_write)
		mame_fwrite(file, generic_nvram32, generic_nvram_size);
	else if (file)
		mame_fread(file, generic_nvram32, generic_nvram_size);
	else
		memcpy(generic_nvram32, memory_region(REGION_USER3), generic_nvram_size);
}

static WRITE32_HANDLER( gstream_palette_w )
{
	UINT32 colors;

	COMBINE_DATA(&paletteram32[offset]);
	if (offset >= GSTREAM_PALETTE_ENTRIES / 2)
		return;

	colors = paletteram32[offset];
	palette_set_color(2 * offset, pal5bit(colors >> 16), pal6bit(colors >> 21), pal5bit(colors >> 27));
	palette_set_color(2 * offset + 1, pal5bit(colors), pal6bit(colors >> 5), pal5bit(colors >> 11));
}

#define SCROLL_HANDLER(axis, layer) \
static WRITE32_HANDLER( scroll_##axis##layer##_w ) \
{ \
	UINT16 mask = mem_mask >> 16; \
	scroll_##axis[layer] = (scroll_##axis[layer] & mask) | ((data >> 16) & ~mask); \
}

SCROLL_HANDLER(x, 0)
SCROLL_HANDLER(x, 1)
SCROLL_HANDLER(x, 2)
SCROLL_HANDLER(y, 0)
SCROLL_HANDLER(y, 1)
SCROLL_HANDLER(y, 2)

#undef SCROLL_HANDLER

static READ32_HANDLER( gstream_in1_r )
{
	return (readinputport(1) & 0x7fff) | (readinputport(0) & 0x8000);
}

static READ32_HANDLER( gstream_in2_r )
{
	UINT32 p1 = readinputport(0);
	UINT32 p2 = readinputport(1);

	return 0xffb0 | ((p1 >> 9) & 1) | ((p2 >> 8) & 2)
		| ((p1 >> 8) & 4) | ((p2 >> 7) & 8) | ((p1 >> 9) & 0x40);
}

static READ32_HANDLER( gstream_oki0_r ) { return OKIM6295_status_0_r(0); }
static READ32_HANDLER( gstream_oki1_r ) { return OKIM6295_status_1_r(0); }
static WRITE32_HANDLER( gstream_oki0_w ) { OKIM6295_data_0_w(0, data); }
static WRITE32_HANDLER( gstream_oki1_w ) { OKIM6295_data_1_w(0, data); }

static WRITE32_HANDLER( gstream_bank_w )
{
	int bank0 = ((data & 0xc0) == 0x40 ? 2 : 0) | ((data & 0x0c) == 0x0c);
	int bank1 = ((data & 0x30) == 0x10 ? 2 : 0) | ((data & 0x03) == 0x03);

	OKIM6295_set_bank_base(0, bank0 * 0x40000);
	OKIM6295_set_bank_base(1, bank1 * 0x40000);
}

static READ32_HANDLER( x2222_in4_r )
{
	/* The program polls this bit while waiting for the sound hardware. */
	x2222_toggle ^= 0x80;
	return readinputport(4) | x2222_toggle;
}

static READ32_HANDLER( gstream_speedup_r )
{
	if (activecpu_get_pc() == 0xc0001592)
		activecpu_adjust_icount(-50);
	return workram[0xd1ee0 / 4];
}

static READ32_HANDLER( x2222_speedup_r )
{
	if (activecpu_get_pc() == 0x23f44)
		activecpu_adjust_icount(-50);
	return workram[0x84e3c / 4];
}

static READ32_HANDLER( x2222o_speedup_r )
{
	if (activecpu_get_pc() == 0x22064)
		activecpu_adjust_icount(-50);
	return workram[0x7ffac / 4];
}

static ADDRESS_MAP_START( gstream_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&workram)
	AM_RANGE(0x4e000000, 0x4e1fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0x4f000000, 0x4f000003) AM_WRITE(scroll_x2_w)
	AM_RANGE(0x4f200000, 0x4f200003) AM_WRITE(scroll_y2_w)
	AM_RANGE(0x4f400000, 0x4f406fff) AM_RAM AM_WRITE(gstream_palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0x4f800000, 0x4f800003) AM_WRITE(scroll_x0_w)
	AM_RANGE(0x4fa00000, 0x4fa00003) AM_WRITE(scroll_y0_w)
	AM_RANGE(0x4fc00000, 0x4fc00003) AM_WRITE(scroll_x1_w)
	AM_RANGE(0x4fe00000, 0x4fe00003) AM_WRITE(scroll_y1_w)
	AM_RANGE(0x80000000, 0x80003fff) AM_RAM AM_BASE(&video_ram)
	AM_RANGE(0xffc00000, 0xffc01fff) AM_RAM AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gstream_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x4000, 0x4003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x4010, 0x4013) AM_READ(gstream_in1_r)
	AM_RANGE(0x4020, 0x4023) AM_READ(gstream_in2_r)
	AM_RANGE(0x4030, 0x4033) AM_WRITE(gstream_bank_w)
	AM_RANGE(0x4040, 0x4043) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x4050, 0x4053) AM_READWRITE(gstream_oki0_r, gstream_oki0_w)
	AM_RANGE(0x4060, 0x4063) AM_READWRITE(gstream_oki1_r, gstream_oki1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( x2222_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&workram)
	AM_RANGE(0x40000000, 0x403fffff) AM_RAM
	AM_RANGE(0x4fa00000, 0x4fa00003) AM_WRITE(scroll_y2_w)
	AM_RANGE(0x4fb00000, 0x4fb00003) AM_WRITE(scroll_x2_w)
	AM_RANGE(0x4fc00000, 0x4fc00003) AM_WRITE(scroll_y1_w)
	AM_RANGE(0x4fd00000, 0x4fd00003) AM_WRITE(scroll_x1_w)
	AM_RANGE(0x4fe00000, 0x4fe00003) AM_WRITE(scroll_y0_w)
	AM_RANGE(0x4ff00000, 0x4ff00003) AM_WRITE(scroll_x0_w)
	AM_RANGE(0x80000000, 0x80003fff) AM_RAM AM_BASE(&video_ram)
	AM_RANGE(0xffc00000, 0xffc01fff) AM_RAM
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( x2222_io, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x4000, 0x4003) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x4004, 0x4007) AM_READ(input_port_1_dword_r)
	AM_RANGE(0x4008, 0x400b) AM_READ(input_port_2_dword_r)
	AM_RANGE(0x4010, 0x4013) AM_READ(input_port_3_dword_r)
	AM_RANGE(0x4028, 0x402b) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x4034, 0x4037) AM_READ(x2222_in4_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( gstream )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x7000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE_NO_TOGGLE( 0x8000, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x7000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( x2222 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x0002, 0x0002, "IN1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
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

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0006, 0x0004, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x0018, 0x0008, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0010, "3" )
	PORT_DIPSETTING(      0x0018, "4" )
	PORT_DIPNAME( 0x00e0, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0060, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x00a0, "5" )
	PORT_DIPSETTING(      0x00c0, "6" )
	PORT_DIPSETTING(      0x00e0, "7" )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "IN4" )
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
INPUT_PORTS_END

static const gfx_layout layout16x16 =
{
	16, 16, RGN_FRAC(1,1), 8,
	{ STEP8(0,1) },
	{ STEP16(0,8) },
	{ STEP16(0,16*8) },
	16*16*8
};

static const gfx_layout layout32x32 =
{
	32, 32, RGN_FRAC(1,1), 8,
	{ STEP8(0,1) },
	{ STEP16(0,8), STEP16(16*8,8) },
	{ STEP16(0,32*8), STEP16(16*32*8,32*8) },
	32*32*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &layout32x32, 0x1000, 4 },
	{ REGION_GFX3, 0, &layout32x32, 0x1400, 4 },
	{ REGION_GFX4, 0, &layout32x32, 0x1800, 4 },
	{ REGION_GFX1, 0, &layout16x16, 0, 32 },
	{ -1 }
};

static PALETTE_INIT( gstream )
{
	int i;

	/* Cover all sprite color codes, including the ones beyond palette RAM. */
	for (i = 0; i < GSTREAM_PALETTE_PENS; ++i)
		palette_set_color(i, 0, 0, 0);
}

static PALETTE_INIT( x2222 )
{
	int i;

	for (i = 0; i < 0x10000; ++i)
		palette_set_color(i, pal5bit(i), pal6bit(i >> 5), pal5bit(i >> 11));
}

static void draw_tile(mame_bitmap *bitmap, const rectangle *cliprect, int layer,
	int code, int color, int x, int y)
{
	int size = layer == 3 ? 16 : 32;

	if (x > cliprect->max_x || y > cliprect->max_y
		|| x + size <= cliprect->min_x || y + size <= cliprect->min_y)
		return;

	if (rgb_tiles[layer])
	{
		int left = MAX(x, cliprect->min_x);
		int right = MIN(x + size - 1, cliprect->max_x);
		int top = MAX(y, cliprect->min_y);
		int bottom = MIN(y + size - 1, cliprect->max_y);
		const UINT16 *tile;
		int row, col;

		code &= layer == 3 ? 0x7fff : 0x7ff;
		tile = rgb_tiles[layer] + code * size * size;
		for (row = top; row <= bottom; ++row)
		{
			UINT32 *dest = bitmap->line[row];
			const UINT16 *source = tile + (row - y) * size;
			for (col = left; col <= right; ++col)
			{
				UINT16 pixel = source[col - x];
				if (pixel)
					dest[col] = Machine->pens[pixel];
			}
		}
	}
	else
		drawgfx(bitmap, Machine->gfx[layer], code, color, 0, 0, x, y,
			cliprect, TRANSPARENCY_PEN, 0);
}

static VIDEO_UPDATE( gstream )
{
	int layer, i;
	int x_offset = rgb_tiles[0] ? 0 : 2;
	int clocks = 0;

	fillbitmap(bitmap, get_black_pen(), cliprect);
	for (layer = 2; layer >= 0; --layer)
	{
		const UINT32 *tiles = video_ram + layer * 0x100;
		int sx = scroll_x[layer] & 0x1ff;
		int sy = scroll_y[layer] & 0x1ff;
		int y, x;

		for (y = -(sy & 31); y <= cliprect->max_y; y += 32)
		{
			int row = ((y + sy) >> 5) & 15;
			for (x = -(sx & 31) - x_offset; x <= cliprect->max_x; x += 32)
			{
				int col = ((x + sx + x_offset) >> 5) & 15;
				UINT32 tile = tiles[row * 16 + col] >> 16;
				draw_tile(bitmap, cliprect, layer, tile & 0xfff, tile >> 14, x, y);
			}
		}
	}

	/* The high halfwords hold tilemap entries and the low halfwords hold sprites. */
	for (i = 0; i < 0x1000; i += 4)
	{
		int code = video_ram[i] & 0xffff;
		int x = (video_ram[i + 1] & 0x1ff) - x_offset;
		int y = video_ram[i + 2] & 0xff;
		int color = video_ram[i + 3] & 0x1f;

		clocks += 8 + 128;
		if (clocks >= 432 * 262)
			break;

		draw_tile(bitmap, cliprect, 3, code, color, x, y);
		draw_tile(bitmap, cliprect, 3, code, color, x, y - 0x100);
		draw_tile(bitmap, cliprect, 3, code, color, x - 0x200, y);
		draw_tile(bitmap, cliprect, 3, code, color, x - 0x200, y - 0x100);
	}
}

static MACHINE_START( gstream )
{
	state_save_register_global_array(scroll_x);
	state_save_register_global_array(scroll_y);
	state_save_register_global(x2222_toggle);
	return 0;
}

static MACHINE_RESET( x2222 )
{
	memset(scroll_x, 0, sizeof(scroll_x));
	memset(scroll_y, 0, sizeof(scroll_y));
	x2222_toggle = 0;
}

static MACHINE_RESET( gstream )
{
	machine_reset_x2222();
	OKIM6295_set_bank_base(0, 0);
	OKIM6295_set_bank_base(1, 0);
}

static MACHINE_DRIVER_START( x2222 )
	MDRV_CPU_ADD(E132XT, 16000000 * 4)
	MDRV_CPU_PROGRAM_MAP(x2222_map, 0)
	MDRV_CPU_IO_MAP(x2222_io, 0)
	MDRV_CPU_VBLANK_INT(irq5_line_hold, 1) /* INT1 */
	MDRV_MACHINE_START(gstream)
	MDRV_MACHINE_RESET(x2222)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(0x10000)
	MDRV_PALETTE_INIT(x2222)
	MDRV_VIDEO_UPDATE(gstream)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gstream )
	MDRV_CPU_ADD(E132XT, 16000000 * 4)
	MDRV_CPU_PROGRAM_MAP(gstream_map, 0)
	MDRV_CPU_IO_MAP(gstream_io, 0)
	MDRV_CPU_VBLANK_INT(irq5_line_hold, 1) /* INT1 */
	MDRV_MACHINE_START(gstream)
	MDRV_MACHINE_RESET(gstream)
	MDRV_NVRAM_HANDLER(gstream)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(GSTREAM_PALETTE_PENS)
	MDRV_PALETTE_INIT(gstream)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_VIDEO_UPDATE(gstream)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MDRV_SOUND_ADD(OKIM6295, 1000000 / 132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static DRIVER_INIT( gstream )
{
	memset(rgb_tiles, 0, sizeof(rgb_tiles));
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0xd1ee0, 0xd1ee3, 0, 0, gstream_speedup_r);
}

static DRIVER_INIT( x2222 )
{
	const UINT8 *rom = memory_region(REGION_GFX1);
	UINT16 *sprites = auto_malloc(0x800000 * sizeof(*sprites));
	int layer, i;

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x7ffac, 0x7ffaf, 0, 0, x2222o_speedup_r);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x84e3c, 0x84e3f, 0, 0, x2222_speedup_r);

	/* Each 4 MiB sprite bank supplies every fourth RGB565 pixel. */
	for (layer = 0; layer < 4; ++layer)
	{
		const UINT8 *pair = rom + layer * 0x400000;
		for (i = 0; i < 0x200000; ++i)
			sprites[i * 4 + layer] = pair[2 * i] | (pair[2 * i + 1] << 8);
	}
	rgb_tiles[3] = sprites;

	/* Each background ROM pair interleaves two low bytes with two high bytes. */
	for (layer = 0; layer < 3; ++layer)
	{
		rom = memory_region(REGION_GFX2 + layer);
		rgb_tiles[layer] = auto_malloc(0x200000 * sizeof(*rgb_tiles[layer]));
		for (i = 0; i < 0x200000; ++i)
		{
			int address = (i & ~1) * 2 + (i & 1);
			rgb_tiles[layer][i] = rom[address] | (rom[address + 2] << 8);
		}
	}
}

ROM_START( gstream )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "gs_prg_01.u56", 0x000000, 0x080000, CRC(0d0c6a38) SHA1(a810bfc1c9158cccc37710d0ea7268e26e520cc2) )

	ROM_REGION32_BE( 0x200000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "gs_prg_02.u197", 0x000000, 0x200000, CRC(2f8a6bea) SHA1(c0a32838f4bd8599f09002139f87562db625c1c5) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD32_WORD( "gs_gr_07.u107", 0x000000, 0x200000, CRC(84e66fe1) SHA1(73d828714f9ed9baffdc06998f5bf3298396fe9c) )
	ROM_LOAD32_WORD( "gs_gr_11.u108", 0x000002, 0x200000, CRC(946d71d1) SHA1(516bd3f4d7f5bce59f0593ed6565114dbd5a4ef0) )
	ROM_LOAD32_WORD( "gs_gr_08.u109", 0x400000, 0x200000, CRC(abd0d6aa) SHA1(dd294bbdda05697df84247257f735ab51bc26ca3) )
	ROM_LOAD32_WORD( "gs_gr_12.u110", 0x400002, 0x200000, CRC(94b56e4e) SHA1(7c3877f993e575326dbd4c2e5d7570747277b20d) )
	ROM_LOAD32_WORD( "gs_gr_09.u180", 0x800000, 0x200000, CRC(f2c4fd77) SHA1(284c850688e3c0fd292a91a53e24fe3436dc4076) )
	ROM_LOAD32_WORD( "gs_gr_13.u181", 0x800002, 0x200000, CRC(7daaeff0) SHA1(5766d9a3a8c0931305424e0089108ce8df7dfe41) )
	ROM_LOAD32_WORD( "gs_gr_10.u182", 0xc00000, 0x200000, CRC(d696d15d) SHA1(85aaa5cdb35f3a8d3266bb8debec0558c860cb53) )
	ROM_LOAD32_WORD( "gs_gr_14.u183", 0xc00002, 0x200000, CRC(6bd2a1e1) SHA1(aedca91643f14ececc101a7708255ce9b1d70f68) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gs_gr_01.u120", 0x000000, 0x200000, CRC(b82cfab8) SHA1(08f0eaef5c927fb056c6cc9342e39f445aae9062) )
	ROM_LOAD( "gs_gr_02.u121", 0x200000, 0x200000, CRC(37e19cbd) SHA1(490ebb037fce09100ec4bba3f73ecdf101526641) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gs_gr_03.u125", 0x000000, 0x200000, CRC(1a3b2b11) SHA1(a4b1dc1a9709f8f8f2ab2190d7badc246caa540f) )
	ROM_LOAD( "gs_gr_04.u126", 0x200000, 0x200000, CRC(a4e8906c) SHA1(b285d7697cdaa62014cf65d09a19fcbd6a95bb98) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "gs_gr_05.u174", 0x000000, 0x200000, CRC(ef283a73) SHA1(8b598facb344eac33138611abc141a2acb375983) )
	ROM_LOAD( "gs_gr_06.u175", 0x200000, 0x200000, CRC(d4e3a2b2) SHA1(4577c007172c718bf7ca55a8ccee5455c281026c) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "gs_snd_01.u192", 0x000000, 0x080000, CRC(79b64d3f) SHA1(b2166210d3a3b85b9ace90749a444c881f69d551) )
	ROM_LOAD( "gs_snd_02.u194", 0x080000, 0x080000, CRC(e49ed92c) SHA1(a3d7b3fe93a786a246acf2657d9056398c793078) )

	ROM_REGION( 0x100000, REGION_SOUND2, 0 )
	ROM_LOAD( "gs_snd_03.u191", 0x000000, 0x080000, CRC(2bfff4ac) SHA1(cce1bb3c78b86722c926854c737f9589806012ba) )
	ROM_LOAD( "gs_snd_04.u193", 0x080000, 0x080000, CRC(b259de3b) SHA1(1a64f41d4344fefad5832332f1a7655e23f6b017) )

	ROM_REGION32_LE( 0x2000, REGION_USER3, 0 )
	ROM_LOAD( "gstream.nv", 0x000000, 0x2000, CRC(895d724b) SHA1(97941102f94923220d9beb270939f0ad9a40fe0e) )
ROM_END

ROM_START( x2222 )
	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "test.bin", 0x000000, 0x100000, CRC(6260421e) SHA1(095e955d029e98e024d4ec5c7f93a6d4845a92a0) )

	ROM_REGION32_BE( 0x0200000, REGION_USER2, 0 )
	ROM_LOAD( "test.hye", 0x000000, 0x0112dda, CRC(c1142b2f) SHA1(5807930820a53604013a6ac66e4d4ebe3628e1fc) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD( "spr11.bin", 0x000000, 0x200000, CRC(1d15b444) SHA1(27ace509a7e4ec2e62453636acf444a861bb85ce) )
	ROM_LOAD( "spr21.bin", 0x200000, 0x1b8b00, CRC(1c392be2) SHA1(775882f588a8bef33a79fa4f25754a47dc82cb30) )
	ROM_LOAD( "spr12.bin", 0x400000, 0x200000, CRC(73225936) SHA1(50507c52b932198659e08d22d3c0a92e7c69e5ba) )
	ROM_LOAD( "spr22.bin", 0x600000, 0x1b8b00, CRC(cf7ebfa1) SHA1(c968dcf768e5598240f5a131414a5607899b4bef) )
	ROM_LOAD( "spr13.bin", 0x800000, 0x200000, CRC(52595c51) SHA1(a161a5f433aa7aa2f7824ea6b9b70d73ca63b62d) )
	ROM_LOAD( "spr23.bin", 0xa00000, 0x1b8b00, CRC(d894461e) SHA1(14dccfa8c762d928eaea0ac4cfff7d1272b69fdd) )
	ROM_LOAD( "spr14.bin", 0xc00000, 0x200000, CRC(f6cd6599) SHA1(170ea7a9a26fd8038df53fb333357766dabbe7c2) )
	ROM_LOAD( "spr24.bin", 0xe00000, 0x1b8b00, CRC(9542cb08) SHA1(d40c1f0b7d3e9deb12284c2f2c2df0ac43cb6cd2) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bg31.bin", 0x000000, 0x11ac00, CRC(12e67bc2) SHA1(18618a8931af3b3aeab34fd50424a7ffb3da6458) )
	ROM_LOAD16_BYTE( "bg32.bin", 0x000001, 0x11ac00, CRC(95afa0da) SHA1(e534bc0874329475ce7efa836000fe29fc76c44c) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bg21.bin", 0x000000, 0x1c8400, CRC(a10220f8) SHA1(9aa43a8e23cdf55d8623d2694b04971eaced9ba9) )
	ROM_LOAD16_BYTE( "bg22.bin", 0x000001, 0x1c8400, CRC(966f7c1d) SHA1(4699a3014c7e66d0dabd8d7982f43114b71181b7) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bg11.bin", 0x000000, 0x1bc800, CRC(68975462) SHA1(7a2458a3d2465b727f4f5bf45685f35eb4885975) )
	ROM_LOAD16_BYTE( "bg12.bin", 0x000001, 0x1bc800, CRC(feef1240) SHA1(9eb123a19ade74d8b3ce4df0b04ca97c03fb9fdc) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "x2222_sound", 0x000000, 0x080000, NO_DUMP )
ROM_END

ROM_START( x2222o )
	ROM_REGION32_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "older.bin", 0x080000, 0x080000, CRC(d12817bc) SHA1(2458f9d9020598a1646dfc848fddd323eebc5120) )

	ROM_REGION32_BE( 0x0200000, REGION_USER2, 0 )
	ROM_LOAD( "older.hye", 0x000000, 0x010892f, CRC(cf3a004e) SHA1(1cba64cfa235b9540f33a5ee0cc02dfd267e00fc) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD( "spr11.bin", 0x000000, 0x200000, BAD_DUMP CRC(1d15b444) SHA1(27ace509a7e4ec2e62453636acf444a861bb85ce) )
	ROM_LOAD( "spr21.bin", 0x200000, 0x1b8b00, BAD_DUMP CRC(1c392be2) SHA1(775882f588a8bef33a79fa4f25754a47dc82cb30) )
	ROM_LOAD( "spr12.bin", 0x400000, 0x200000, BAD_DUMP CRC(73225936) SHA1(50507c52b932198659e08d22d3c0a92e7c69e5ba) )
	ROM_LOAD( "spr22.bin", 0x600000, 0x1b8b00, BAD_DUMP CRC(cf7ebfa1) SHA1(c968dcf768e5598240f5a131414a5607899b4bef) )
	ROM_LOAD( "spr13.bin", 0x800000, 0x200000, BAD_DUMP CRC(52595c51) SHA1(a161a5f433aa7aa2f7824ea6b9b70d73ca63b62d) )
	ROM_LOAD( "spr23.bin", 0xa00000, 0x1b8b00, BAD_DUMP CRC(d894461e) SHA1(14dccfa8c762d928eaea0ac4cfff7d1272b69fdd) )
	ROM_LOAD( "spr14.bin", 0xc00000, 0x200000, BAD_DUMP CRC(f6cd6599) SHA1(170ea7a9a26fd8038df53fb333357766dabbe7c2) )
	ROM_LOAD( "spr24.bin", 0xe00000, 0x1b8b00, BAD_DUMP CRC(9542cb08) SHA1(d40c1f0b7d3e9deb12284c2f2c2df0ac43cb6cd2) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bg31.bin", 0x000000, 0x11ac00, CRC(12e67bc2) SHA1(18618a8931af3b3aeab34fd50424a7ffb3da6458) )
	ROM_LOAD16_BYTE( "bg32.bin", 0x000001, 0x11ac00, CRC(95afa0da) SHA1(e534bc0874329475ce7efa836000fe29fc76c44c) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bg21.bin", 0x000000, 0x1c8400, CRC(a10220f8) SHA1(9aa43a8e23cdf55d8623d2694b04971eaced9ba9) )
	ROM_LOAD16_BYTE( "bg22.bin", 0x000001, 0x1c8400, CRC(966f7c1d) SHA1(4699a3014c7e66d0dabd8d7982f43114b71181b7) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_ERASE00 )
	ROM_LOAD16_BYTE( "bg11.bin", 0x000000, 0x1bc800, CRC(68975462) SHA1(7a2458a3d2465b727f4f5bf45685f35eb4885975) )
	ROM_LOAD16_BYTE( "bg12.bin", 0x000001, 0x1bc800, CRC(feef1240) SHA1(9eb123a19ade74d8b3ce4df0b04ca97c03fb9fdc) )

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_LOAD( "x2222_sound", 0x000000, 0x080000, NO_DUMP )
ROM_END

GAME( 2002, gstream, 0,     gstream, gstream, gstream, ROT270, "Oriental Soft Japan",    "G-Stream G2020",           GAME_SUPPORTS_SAVE )
GAME( 2000, x2222,   0,     x2222,   x2222,   x2222,   ROT270, "Oriental Soft / Promat", "X2222 (final debug?)",     GAME_SUPPORTS_SAVE | GAME_NO_SOUND )
GAME( 2000, x2222o,  x2222, x2222,   x2222,   x2222,   ROT270, "Oriental Soft / Promat", "X2222 (5-level prototype)", GAME_SUPPORTS_SAVE | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
