// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/*

Cycle Maabou (c) 1984 Taito Corporation / Seta
Sky Destroyer (c) 1985 Taito Corporation

TODO:
- low-level emulation of inputs/soundlatch (controlled by several i8741, like Gladiator / Ougon no Shiro);
- color prom resistor network is guessed, cyclemb yellows are more reddish on pcb video and photos;

Cycle Mahbou
(c)1984 Taito/Seta

----------------------------------------
Top
P1-002A
----------------------------------------
P0_20.3D     [53e3a36e]
P0_21.3E     [a7dab6d8]


----------------------------------------
Bottom
P0-001A
CPU  :Z80A x2
OSC  :18.000MHz
Other:AP-001,AP-004,AP-005,AP-006,P7,P8,P9
----------------------------------------
P0_1.1A      [a1588264]
P0_2.1B      [04141837]
P0_3.1C      [a9dd4b22]
P0_4.1E      [456a30df]
P0_5.1F      [a3b9c297]
P0_6.1H      [ec76a0a6]
P0_7.1K      [6507d23f]
P0_10.1N     [a98415db]
P0_11.1R     [626556fe]
P0_12.1S     [1e08902c]
P0_13.1T     [086639c1]
P0_14.1U     [3f5fe2b6]

P0_15.10C    [9cc52c32]
P0_16.10D    [8d03227e]

AP-002.7B    [Not Dump] 8741
AP-003.7C    [Not Dump] /

P1.2E        [6297104c] 82S123
P2.4E        [70a09cc5] /

P0_3.11T     [be89c1f7] 82S129
P0_4.11U     [4886d832] /

*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"

typedef struct
{
	UINT8 current_value;
	int reverse;
} DIAL_STATUS;

typedef struct
{
	UINT8 rxd;
	UINT8 txd;
	UINT8 rst;
	UINT8 state;
	UINT8 packet_type;
} MCU_STATUS;

static UINT8 *cyclemb_vram, *cyclemb_cram;
static UINT8 *cyclemb_obj1_ram, *cyclemb_obj2_ram, *cyclemb_obj3_ram;

static DIAL_STATUS dial_status[2];
static MCU_STATUS mcu[2];
static UINT16 dsw_pc_hack;
static int use_dial;
static int screen_display;
static int sprite_page;

static PALETTE_INIT( cyclemb )
{
	int i;

	for (i = 0; i < 256; i++)
	{
		int val = color_prom[i | 0x100] | (color_prom[i | 0x000] << 4);
		int bit0, bit1, bit2;
		int r, g, b;

		bit0 = 0;
		bit1 = (val >> 6) & 1;
		bit2 = (val >> 7) & 1;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (val >> 3) & 1;
		bit1 = (val >> 4) & 1;
		bit2 = (val >> 5) & 1;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (val >> 0) & 1;
		bit1 = (val >> 1) & 1;
		bit2 = (val >> 2) & 1;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i, r, g, b);
	}
}

static void cyclemb_draw_tilemap(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const gfx_element *gfx = Machine->gfx[0];
	int x, y, count;

	count = 0;

	for (y = 0; y < 32; y++)
	{
		for (x = 0; x < 64; x++)
		{
			int attr = cyclemb_cram[count];
			int tile = cyclemb_vram[count] | ((attr & 3) << 8);
			int color = ((attr & 0xf8) >> 3) ^ 0x1f;
			int odd_line = y & 1 ? 0x40 : 0x00;
			int scrollx = (cyclemb_vram[(y / 2) + odd_line] + (cyclemb_cram[(y / 2) + odd_line] << 8) + 48) & 0x1ff;

			if (!(attr & 4))
				color += 0x20;

			if (flip_screen)
			{
				drawgfx(bitmap, gfx, tile, color, 1, 1, 504 - (x * 8) - scrollx, 248 - (y * 8), cliprect, TRANSPARENCY_NONE, 0);
				/* wrap-around */
				drawgfx(bitmap, gfx, tile, color, 1, 1, 504 - (x * 8) - scrollx + 512, 248 - (y * 8), cliprect, TRANSPARENCY_NONE, 0);
			}
			else
			{
				drawgfx(bitmap, gfx, tile, color, 0, 0, (x * 8) - scrollx, y * 8, cliprect, TRANSPARENCY_NONE, 0);
				/* wrap-around */
				drawgfx(bitmap, gfx, tile, color, 0, 0, (x * 8) - scrollx + 512, y * 8, cliprect, TRANSPARENCY_NONE, 0);
			}

			++count;
		}
	}
}

static void cyclemb_draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int page_start = sprite_page * 0x80;
	int i;

	for (i = page_start; i < 0x80 + page_start; i += 2)
	{
		int y = 0xf1 - cyclemb_obj2_ram[i];
		int x = cyclemb_obj2_ram[i + 1] - 56;
		int spr_offs = cyclemb_obj1_ram[i + 0] + ((cyclemb_obj3_ram[i + 0] & 3) << 8);
		int col = cyclemb_obj1_ram[i + 1] & 0x3f;
		int region = ((cyclemb_obj3_ram[i] & 0x10) >> 4) + 1;
		int fx, fy;

		if (region == 2)
		{
			spr_offs >>= 2;
			y -= 16;
		}

		if (cyclemb_obj3_ram[i + 1] & 1)
			x += 256;

		fx = (cyclemb_obj3_ram[i + 0] & 4) >> 2;
		fy = (cyclemb_obj3_ram[i + 0] & 8) >> 3;

		if (flip_screen)
		{
			fx = !fx;
			fy = !fy;
		}

		drawgfx(bitmap, Machine->gfx[region], spr_offs, col, fx, fy, x, y, cliprect, TRANSPARENCY_PEN, 0);
	}
}

static void skydest_draw_tilemap(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const gfx_element *gfx = Machine->gfx[0];
	int x, y;

	for (y = 0; y < 32; y++)
	{
		for (x = 2; x < 62; x++)
		{
			int attr = cyclemb_cram[x + y * 64];
			int tile = cyclemb_vram[x + y * 64] | ((attr & 3) << 8);
			int color = (attr & 0xfc) >> 2;
			int scrollx = cyclemb_vram[0 * 64 + 0] | ((cyclemb_cram[0 * 64 + 0] & 0x01) << 8);
			int cycle = (cyclemb_cram[0 * 64 + 0] & 0xf0) >> 4;
			int scrolly;

			color ^= 0x3f;
			if (attr & 0x40)
				color ^= cycle;

			scrollx -= 0xc0;

			if (x < 32)
				scrolly = cyclemb_vram[x * 64 + 0];
			else
				scrolly = cyclemb_vram[(x - 32) * 64 + 1];

			if (flip_screen)
			{
				drawgfx(bitmap, gfx, tile, color, 1, 1, 504 - x * 8 + scrollx, 248 - (((y * 8) - scrolly) & 0xff), cliprect, TRANSPARENCY_NONE, 0);
				drawgfx(bitmap, gfx, tile, color, 1, 1, 504 - x * 8 + scrollx - 480, 248 - (((y * 8) - scrolly) & 0xff), cliprect, TRANSPARENCY_NONE, 0);
				drawgfx(bitmap, gfx, tile, color, 1, 1, 504 - x * 8 + scrollx + 480, 248 - (((y * 8) - scrolly) & 0xff), cliprect, TRANSPARENCY_NONE, 0);
			}
			else
			{
				drawgfx(bitmap, gfx, tile, color, 0, 0, x * 8 + scrollx, ((y * 8) - scrolly) & 0xff, cliprect, TRANSPARENCY_NONE, 0);
				drawgfx(bitmap, gfx, tile, color, 0, 0, x * 8 + scrollx - 480, ((y * 8) - scrolly) & 0xff, cliprect, TRANSPARENCY_NONE, 0);
				drawgfx(bitmap, gfx, tile, color, 0, 0, x * 8 + scrollx + 480, ((y * 8) - scrolly) & 0xff, cliprect, TRANSPARENCY_NONE, 0);
			}
		}
	}
}

static void skydest_draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int page_start = sprite_page * 0x80;
	int i;

	for (i = page_start; i < 0x80 + page_start; i += 2)
	{
		int y = cyclemb_obj2_ram[i] - 1;
		int x = cyclemb_obj2_ram[i + 1];
		int spr_offs, col, region, fx, fy;

		if (cyclemb_obj3_ram[i + 1] & 1)
			x |= 0x100;

		x = 0x138 - x;

		spr_offs = cyclemb_obj1_ram[i + 0] + ((cyclemb_obj3_ram[i + 0] & 3) << 8);
		col = cyclemb_obj1_ram[i + 1] & 0x3f;
		region = ((cyclemb_obj3_ram[i] & 0x10) >> 4) + 1;
		if (region == 2)
		{
			spr_offs >>= 2;
			x -= 16;
		}

		fx = (cyclemb_obj3_ram[i + 0] & 4) >> 2;
		fy = (cyclemb_obj3_ram[i + 0] & 8) >> 3;

		if (flip_screen)
		{
			fx = !fx;
			fy = !fy;
		}

		drawgfx(bitmap, Machine->gfx[region], spr_offs, col, fx, fy, x, y, cliprect, TRANSPARENCY_PEN, 0);
	}
}

static VIDEO_START( cyclemb )
{
	return 0;
}

static VIDEO_UPDATE( cyclemb )
{
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	if (screen_display)
	{
		cyclemb_draw_tilemap(bitmap, cliprect);
		cyclemb_draw_sprites(bitmap, cliprect);
	}
}

static VIDEO_UPDATE( skydest )
{
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	if (screen_display)
	{
		skydest_draw_tilemap(bitmap, cliprect);
		skydest_draw_sprites(bitmap, cliprect);
	}
}

static WRITE8_HANDLER( cyclemb_bankswitch_w )
{
	memory_set_bank(1, data & 0x03);
	sprite_page = (data & 0x04) >> 2;
}

static WRITE8_HANDLER( skydest_bankswitch_w )
{
	memory_set_bank(1, data & 0x03);
	sprite_page = (data & 0x04) >> 2;
	flip_screen_set((data & 0x40) == 0);
}

static WRITE8_HANDLER( cyclemb_screen_display_w )
{
	screen_display = (data & 0x01) != 0;
}

static WRITE8_HANDLER( cyclemb_flip_w )
{
	flip_screen_set(data & 1);
}

static void update_dial(int player, const char *tag)
{
	int input_value;
	int delta;

	if (!use_dial)
		return;

	input_value = (INT8)readinputportbytag(tag);
	delta = input_value;
	if (delta < -0x1f)
		delta = -0x1f;
	if (delta > 0x1f)
		delta = 0x1f;

	if (delta != 0)
	{
		dial_status[player].reverse = delta < 0;
		dial_status[player].current_value = (dial_status[player].current_value + (delta < 0 ? -delta : delta)) & 0x1f;
	}
}

static UINT32 dial_r(void *param)
{
	const DIAL_STATUS *status = param;

	return status->current_value | (status->reverse ? 0x80 : 0x00);
}

static READ8_HANDLER( skydest_i8741_0_r )
{
	if (offset == 1)
		return 1;
	else
	{
		/* the dip switch read at boot is recognized by the caller address */
		if (activecpu_get_pc() == dsw_pc_hack)
			mcu[0].rxd = (readinputportbytag("DSW1") & 0x1f) << 2;
		else if (mcu[0].rst)
		{
			int i, pt;

			switch (mcu[0].state)
			{
				case 1:
					mcu[0].packet_type ^= 0x20;
					if (mcu[0].packet_type & 0x20)
						mcu[0].rxd = (readinputportbytag("DSW3") & 0x9f) | mcu[0].packet_type;
					else
						mcu[0].rxd = (readinputportbytag("SYSTEM") & 0x9f) | mcu[0].packet_type;
					break;
				case 2:
					mcu[0].packet_type ^= 0x20;
					if (mcu[0].packet_type & 0x20)
					{
						update_dial(0, "DIAL_P1");
						mcu[0].rxd = (readinputportbytag("P1_1") & 0x9f) | mcu[0].packet_type;
					}
					else
					{
						mcu[0].rxd = (readinputportbytag("P1_0") & 0x1f) | mcu[0].packet_type;
						mcu[0].rxd |= readinputportbytag("SYSTEM") & 0x80;
					}
					break;
				case 3:
					mcu[0].packet_type ^= 0x20;
					if (mcu[0].packet_type & 0x20)
					{
						update_dial(1, "DIAL_P2");
						mcu[0].rxd = (readinputportbytag("P2_1") & 0x9f) | mcu[0].packet_type;
					}
					else
					{
						mcu[0].rxd = (readinputportbytag("P2_0") & 0x1f) | mcu[0].packet_type;
						mcu[0].rxd |= readinputportbytag("SYSTEM") & 0x80;
					}
					break;
				default:
					mcu[0].rxd = 0x00;
					break;
			}

			/* odd parity in bit 6 */
			for (i = 0, pt = 0; i < 8; ++i)
			{
				if (mcu[0].rxd & (1 << i))
					++pt;
			}

			if (pt & 1)
				mcu[0].rxd |= 0x40;
		}

		return mcu[0].rxd;
	}
}

static WRITE8_HANDLER( skydest_i8741_0_w )
{
	if (offset == 1)
	{
		switch (data)
		{
			case 0:
				mcu[0].rxd = 0x40;
				mcu[0].rst = 0;
				mcu[0].state = 0;
				break;
			case 1:
				mcu[0].rxd = 0x40;
				mcu[0].rst = 0;
				break;
			case 2:
				mcu[0].rxd = (readinputportbytag("DSW2") & 0x1f) << 2;
				mcu[0].rst = 0;
				break;
			case 3:
				mcu[0].rst = 1;
				mcu[0].txd = 0;
				break;
		}
	}
	else
	{
		mcu[0].txd = data;

		mcu[1].rst = 0;

		if (mcu[0].txd == 0x41)
			mcu[0].state = 1;
		else if (mcu[0].txd == 0x42)
			mcu[0].state = 2;
		else if (mcu[0].txd == 0x44)
			mcu[0].state = 3;
		else
			soundlatch_w(0, data & 0xff);
	}
}

static READ8_HANDLER( skydest_i8741_1_r )
{
	UINT8 ret;

	if (offset == 1)
		return 1;

	if (mcu[1].rst == 1)
		return 0x40;

	ret = soundlatch_r(0);
	soundlatch_w(0, 0);

	return ret;
}

static WRITE8_HANDLER( skydest_i8741_1_w )
{
	if (offset == 1)
	{
		if (data == 0xf0)
			mcu[1].rst = 1;
	}
}

static ADDRESS_MAP_START( cyclemb_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x9000, 0x97ff) AM_RAM AM_BASE(&cyclemb_vram)
	AM_RANGE(0x9800, 0x9fff) AM_RAM AM_BASE(&cyclemb_cram)
	AM_RANGE(0xa000, 0xa7ff) AM_RAM AM_BASE(&cyclemb_obj1_ram)
	AM_RANGE(0xa800, 0xafff) AM_RAM AM_BASE(&cyclemb_obj2_ram)
	AM_RANGE(0xb000, 0xb7ff) AM_RAM AM_BASE(&cyclemb_obj3_ram)
	AM_RANGE(0xb800, 0xbfff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cyclemb_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xc000, 0xc000) AM_WRITE(cyclemb_bankswitch_w)
	AM_RANGE(0xc020, 0xc020) AM_WRITE(cyclemb_screen_display_w)
	AM_RANGE(0xc09e, 0xc09f) AM_READWRITE(skydest_i8741_0_r, skydest_i8741_0_w)
	AM_RANGE(0xc0bf, 0xc0bf) AM_WRITE(cyclemb_flip_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( skydest_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xc000, 0xc000) AM_WRITE(skydest_bankswitch_w)
	AM_RANGE(0xc020, 0xc020) AM_WRITE(cyclemb_screen_display_w)
	AM_RANGE(0xc080, 0xc081) AM_READWRITE(skydest_i8741_0_r, skydest_i8741_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cyclemb_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x6000, 0x63ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cyclemb_sound_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x40, 0x41) AM_READWRITE(skydest_i8741_1_r, skydest_i8741_1_w)
ADDRESS_MAP_END

static MACHINE_RESET( cyclemb )
{
	int i;

	for (i = 0; i < 2; ++i)
	{
		mcu[i].rxd = 0;
		mcu[i].txd = 0;
		mcu[i].rst = 0;
		mcu[i].state = 0;
		mcu[i].packet_type = 0;

		dial_status[i].current_value = 0;
		dial_status[i].reverse = 0;
	}

	screen_display = 1;
	sprite_page = 0;
}

INPUT_PORTS_START( cyclemb )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x7c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(2)

	PORT_START_TAG("P1_0")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P1_1")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dial_r, &dial_status[0])

	PORT_START_TAG("DIAL_P1")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(20) PORT_KEYDELTA(16) PORT_PLAYER(1) PORT_RESET PORT_REVERSE

	PORT_START_TAG("P2_0")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P2_1")
	PORT_BIT( 0x9f, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(dial_r, &dial_status[1])
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DIAL_P2")
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(20) PORT_KEYDELTA(16) PORT_PLAYER(2) PORT_RESET PORT_REVERSE PORT_COCKTAIL

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x02, "Difficulty (Stage 1-3)" )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x08, "Difficulty (Stage 4-5)" )
	PORT_DIPSETTING(    0x0c, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "Disallow Game Over (Cheat)" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Stage" )
	PORT_DIPSETTING(    0x00, "Stage 1" )
	PORT_DIPSETTING(    0x04, "Stage 3" )
	PORT_DIPSETTING(    0x08, "Stage 4" )
	PORT_DIPSETTING(    0x0c, "Bonus Game Only" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( skydest )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x7c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(2)

	PORT_START_TAG("P1_0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P1_1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P2_0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("P2_1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x07, "1 (Easiest)" )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x05, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x01, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hardest)" )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Invincibility (Cheat)" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0,1,2,3,64,65,66,67 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	16*8
};

static const gfx_layout spritelayout_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static const gfx_layout spritelayout_32x32 =
{
	32,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 72*8+0, 72*8+1, 72*8+2, 72*8+3,
			80*8+0, 80*8+1, 80*8+2, 80*8+3, 88*8+0, 88*8+1, 88*8+2, 88*8+3},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			128*8, 129*8, 130*8, 131*8, 132*8, 133*8, 134*8, 135*8,
			160*8, 161*8, 162*8, 163*8, 164*8, 165*8, 166*8, 167*8 },
	64*8*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 0x40 },
	{ REGION_GFX2, 0, &spritelayout_16x16,  0, 0x40 },
	{ REGION_GFX2, 0, &spritelayout_32x32,  0, 0x40 },
	{ -1 }
};

static MACHINE_DRIVER_START( cyclemb )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 18000000/3)
	MDRV_CPU_PROGRAM_MAP(cyclemb_map, 0)
	MDRV_CPU_IO_MAP(cyclemb_io, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80, 18000000/6)
	MDRV_CPU_PROGRAM_MAP(cyclemb_sound_map, 0)
	MDRV_CPU_IO_MAP(cyclemb_sound_io, 0)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(60))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cyclemb)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(cyclemb)
	MDRV_VIDEO_START(cyclemb)
	MDRV_VIDEO_UPDATE(cyclemb)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 18000000/12)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( skydest )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(cyclemb)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(skydest_io, 0)

	/* video hardware */
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 34*8-1, 2*8, 30*8-1)
	MDRV_VIDEO_UPDATE(skydest)
MACHINE_DRIVER_END

ROM_START( cyclemb )
	ROM_REGION( 0x18000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "p0_1.1a",   0x00000, 0x2000, CRC(a1588264) SHA1(ff17df61207e39443a8ea62be1fce102c163d8e1) )
	ROM_LOAD( "p0_2.1b",   0x02000, 0x2000, CRC(04141837) SHA1(18d2f17fd5334b306ca13a1c26780f4a868a4ac8) )
	ROM_LOAD( "p0_3.1c",   0x04000, 0x2000, CRC(a9dd4b22) SHA1(8d3535ecd43aa0eccf3856b7cbad8702d17dd576) )
	ROM_LOAD( "p0_4.1e",   0x06000, 0x2000, CRC(456a30df) SHA1(75594178e6299ef5a81c134138ac1f1231a36caa) )
	ROM_LOAD( "p0_5.1f",   0x10000, 0x2000, CRC(a3b9c297) SHA1(edbab8639cb73e1376306ef70ef4ae451a75e4a9) )
	ROM_LOAD( "p0_6.1h",   0x12000, 0x2000, CRC(ec76a0a6) SHA1(9d1d3c050c76df42da53896f38ae53c5f79b0c5c) )

	ROM_REGION( 0x4000, REGION_CPU2, 0 )
	ROM_LOAD( "p0_15.10c",   0x0000, 0x2000, CRC(9cc52c32) SHA1(05d4e7c8ce8fdfc995013c0ed693b4d4778acc25) )
	ROM_LOAD( "p0_16.10d",   0x2000, 0x2000, CRC(8d03227e) SHA1(7e90437cbe5e854025e799348bb2cbca98368bd9) )

	ROM_REGION( 0x0400, REGION_USER1, 0 )
	ROM_LOAD( "ap_002.7b", 0x00000, 0x0400, NO_DUMP )

	ROM_REGION( 0x0400, REGION_USER2, 0 )
	ROM_LOAD( "ap-003.7c", 0x00000, 0x0400, NO_DUMP )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "p0_21.3e",   0x0000, 0x2000, CRC(a7dab6d8) SHA1(c5802e76abd394a2ce1526815bfbfc12e5e57587) )
	ROM_LOAD( "p0_20.3d",   0x2000, 0x2000, CRC(53e3a36e) SHA1(d95c1dfe216bb8b1f3e14c72a480eb2befa9d1dd) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_ERASEFF )
	ROM_LOAD( "p0_7.1k",    0x0000, 0x2000, CRC(6507d23f) SHA1(1640b25a6efa0976f13ed7838f31ef53c37c8d2d) )
	ROM_LOAD( "p0_10.1n",   0x6000, 0x2000, CRC(a98415db) SHA1(218a1d3ad27c30263daf87be87b4d5e06d5ac604) )
	ROM_LOAD( "p0_11.1r",   0x8000, 0x2000, CRC(626556fe) SHA1(ebd08a407fe466af14813bdeeb852d6816da932e) )
	ROM_LOAD( "p0_12.1s",   0xa000, 0x2000, CRC(1e08902c) SHA1(3d5f620580dc1fc43cd5f99b2a1e62a6d749f8b9) )
	ROM_LOAD( "p0_13.1t",   0xc000, 0x2000, CRC(086639c1) SHA1(3afbe76bb466d4c5916ef85d4cfc42e0c3f69883) )
	ROM_LOAD( "p0_14.1u",   0xe000, 0x2000, CRC(3f5fe2b6) SHA1(a7d1d0bc449f557ba827936b0fdbcccf7b1ee629) )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "p0_3.11t",   0x0000, 0x100, CRC(be89c1f7) SHA1(7fb2d9fccf6c74130c3e0db4ea4269aeb45359e3) )
	ROM_LOAD( "p0_4.11u",   0x0100, 0x100, CRC(4886d832) SHA1(49e77923b7e2a0d5e9d990706dac258ecfd7720e) )

	ROM_REGION( 0x40, REGION_USER3, 0 )
	ROM_LOAD( "p1.2e",      0x000, 0x020, CRC(6297104c) SHA1(f2a40811505625a7a7ef4a7e4168c556c263449b) )
	ROM_LOAD( "p2.4e",      0x020, 0x020, CRC(70a09cc5) SHA1(82c0f3122d2c1e8be74b857737380c2e978adeef) )
ROM_END

ROM_START( skydest )
	ROM_REGION( 0x18000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "pd1-1.1a",     0x000000, 0x002000, CRC(78951c75) SHA1(f6d36a1b9b35a346a1e7389956e90332ada07454) )
	ROM_LOAD( "pd0-2.1b",     0x002000, 0x002000, CRC(da2d48cd) SHA1(5f4871f66bca8515505e4ef887cadf41a4e88f4d) )
	ROM_LOAD( "pd0-3.1c",     0x004000, 0x002000, CRC(28ef8eda) SHA1(c2ca346b1170e8ca7239bee4040225c50923e527) )
	ROM_LOAD( "pd1-4.1e",     0x006000, 0x002000, CRC(b8ec9938) SHA1(79f9be7ba74af9542488247c83a4aa731ebb7917) )

	ROM_REGION( 0x4000, REGION_CPU2, 0 )
	ROM_LOAD( "pd0-15.10c",   0x000000, 0x002000, CRC(f8b3d3f7) SHA1(447907f982362f5995d7eb646628cf0e07ba8f64) )
	ROM_LOAD( "pd0-16.10d",   0x002000, 0x002000, CRC(19ce8106) SHA1(31186d3b1c0d124da82310930a002a481941ebb1) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_INVERT )
	ROM_LOAD( "pd0-20.1h",    0x000000, 0x004000, CRC(8b2137f2) SHA1(1f83e081cab116c69a8349fd33ba1916b1c91826) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_ERASEFF )
	ROM_LOAD( "pd0-7.1k",     0x000000, 0x002000, CRC(83137d42) SHA1(7e35f28577d6bfeee184a0ac3095b478999d6477) )
	ROM_LOAD( "pd1-8.1l",     0x002000, 0x002000, CRC(b810858b) SHA1(385e625fc989a1dfa18559a62c99363b62c66a67) )
	ROM_LOAD( "pd0-9.1m",     0x004000, 0x002000, CRC(6f558bee) SHA1(0539feaa848d6cfb9f90a46a851f73fb74e82676) )
	ROM_LOAD( "pd1-10.1n",    0x006000, 0x002000, CRC(5840b5b5) SHA1(1b5b188023c4d3198402c946b8c5a51d7f512a07) )
	ROM_LOAD( "pd0-11.1r",    0x008000, 0x002000, CRC(29e5fce4) SHA1(59748e3a192a45dce7920e8d5a7a11d5145915b0) )
	ROM_LOAD( "pd0-12.1s",    0x00a000, 0x002000, CRC(06234942) SHA1(1cc40a8c8e24ab6db1dc7dc88979be23b7a9cab6) )
	ROM_LOAD( "pd1-13.1t",    0x00c000, 0x002000, CRC(3cca5b95) SHA1(74baec7c128254c394dd3162df7abacf5ed5a99b) )
	ROM_LOAD( "pd0-14.1u",    0x00e000, 0x002000, CRC(7ef05b01) SHA1(f36ad1c0dac201729def78dc18feacda8fcf1a3f) )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "green.11t",    0x000, 0x100, CRC(f803beb7) SHA1(9c979a296de04728d43c94e9e06f8d8600dc9cfb) )
	ROM_LOAD( "red.11u",      0x100, 0x100, CRC(24b7b6f3) SHA1(c2f6477baa5be038c41f5f2ecd16522a6b8d84db) )

	ROM_REGION( 0x40, REGION_USER1, 0 )
	ROM_LOAD( "p1.2e",        0x000, 0x020, NO_DUMP )
	ROM_LOAD( "p0.4e",        0x020, 0x020, NO_DUMP )

	ROM_REGION( 0x100, REGION_USER2, 0 )
	ROM_LOAD( "blue.4j",      0x000, 0x100, CRC(34579681) SHA1(10e5e137837bdd71959f0c4bf52e0f333630a22f) )
ROM_END

static DRIVER_INIT( cyclemb )
{
	UINT8 *rom = memory_region(REGION_CPU2);

	memory_configure_bank(1, 0, 4, memory_region(REGION_CPU1) + 0x10000, 0x1000);
	memory_set_bank(1, 0);
	dsw_pc_hack = 0x760;

	/* patch audio CPU crash + ROM checksum */
	rom[0x282] = 0x00;
	rom[0x283] = 0x00;
	rom[0x284] = 0x00;
	rom[0xa36] = 0x00;
	rom[0xa37] = 0x00;
	rom[0xa38] = 0x00;

	use_dial = 1;
}

static DRIVER_INIT( skydest )
{
	UINT8 *rom = memory_region(REGION_CPU2);

	memory_configure_bank(1, 0, 4, memory_region(REGION_CPU1) + 0x10000, 0x1000);
	memory_set_bank(1, 0);
	dsw_pc_hack = 0x554;

	/* patch audio CPU crash + ROM checksum */
	rom[0x286] = 0x00;
	rom[0x287] = 0x00;
	rom[0x288] = 0x00;
	rom[0xa36] = 0x00;
	rom[0xa37] = 0x00;
	rom[0xa38] = 0x00;

	use_dial = 0;
}

GAME( 1984, cyclemb, 0, cyclemb, cyclemb, cyclemb, ROT0, "Taito Corporation", "Cycle Maabou (Japan)", 0 )
GAME( 1985, skydest, 0, skydest, skydest, skydest, ROT0, "Taito Corporation", "Sky Destroyer (Japan)", 0 )
