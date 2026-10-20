/*

    Laser Battle / Lazarian (c) 1981 Zaccaria
    Cat and Mouse           (c) 1982 Zaccaria

    driver by Pierpaolo Prazzoli

    The 2 games have a similar video hardware, but sound hardware is very different
    and they don't use the collision detection provided by the s2636 chips.

TODO:
- analog filtering/distortion in laserbat (with schematics)
- verify analog filtering in catnmous
- confirm layer alignment against hardware

*/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "machine/6821pia.h"
#include "sound/ay8910.h"
#include "sound/sn76477.h"
#include "sound/custom.h"
#include "streams.h"

static UINT8 *pvi_ram[3];
static UINT8 bg_ram[0x400], eff_ram[0x200], mixing_table[0x10000];
static UINT8 cnt_eff, cnt_nav, wcoh, wcov, csound1, csound2;
static int input_mux;
static int pvi_count[3][4], pvi_display[3][4], pvi_duplicate[3][4];
static int is_catnmous;
static mame_bitmap *video_bitmap;

static WRITE8_HANDLER( laserbat_videoram_w )
{
	if (cnt_nav & 0x10)
		eff_ram[offset & 0x1ff] = data;
	else
		bg_ram[offset] = data;
}

static WRITE8_HANDLER( laserbat_s2636_1_w ) { pvi_ram[0][offset] = data; }
static WRITE8_HANDLER( laserbat_s2636_2_w ) { pvi_ram[1][offset] = data; }
static WRITE8_HANDLER( laserbat_s2636_3_w ) { pvi_ram[2][offset] = data; }
static WRITE8_HANDLER( video_extra_w ) { cnt_nav = data; }
static WRITE8_HANDLER( effects_w ) { cnt_eff = data; }
static WRITE8_HANDLER( sprite_x_y_w )
{
	if (offset == 0) wcoh = data;
	else wcov = data;
}

static WRITE8_HANDLER( laserbat_input_mux_w )
{
	input_mux = (data >> 4) & 3;
	flip_screen_set((data & 8) && !(readinputportbytag("ROW1") & 0x10));
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
	coin_counter_w(2, data & 4);
}

static READ8_HANDLER( laserbat_input_r )
{
	static const char *const rows[] = { "ROW0", "ROW1", "SW1", "SW2" };
	return readinputportbytag((cnt_eff & 0x80) ? "ROW2" : rows[input_mux]);
}

static WRITE8_HANDLER( csound1_w )
{
	csound1 = data;
	if (is_catnmous) pia_0_ca1_w(0, (data >> 5) & 1);
}

static void laserbat_sound_w(int data);

static WRITE8_HANDLER( csound2_w )
{
	if (is_catnmous)
		cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x80) ? ASSERT_LINE : CLEAR_LINE);
	else
		laserbat_sound_w(data);
	csound2 = data;
}

static PALETTE_INIT( laserbat )
{
	static const int weights[] = { 0, 107, 120, 173, 255, 255, 255, 255 };
	static const int blue_weights[] = { 0, 0, 60, 121, 241, 255, 255, 255 };
	static const int cat_weights[] = { 0, 40, 51, 87, 123, 189, 224, 255 };
	static const int cat_blue[] = { 0, 70, 165, 255 };
	int entry;
	int cat = !strcmp(Machine->gamedrv->name, "catnmous") || !strcmp(Machine->gamedrv->name, "catmousa");
	for (entry = 0; entry < 256; entry++)
	{
		int r = ((entry & 1) << 1) | ((entry & 8) >> 1) | ((entry & 0x40) >> 6);
		int g = (entry & 2) | ((entry & 0x10) >> 2) | ((entry & 0x80) >> 7);
		int b = ((entry & 4) >> 1) | ((entry & 0x20) >> 3) | 1;
		if (cat)
			palette_set_color(entry, cat_weights[r], cat_weights[g], cat_blue[((entry & 4) >> 2) | ((entry & 0x20) >> 4)]);
		else
			palette_set_color(entry, weights[r], weights[g], blue_weights[b]);
	}
}

/* Scanline object counters follow MAME's S2636 implementation. */
static void pvi_line(UINT8 *row, const UINT8 *ram, int chip, int y)
{
	static const int offsets[] = { 0, 0x10, 0x20, 0x40 };
	int i;
	for (i = 0; i < 4; i++)
	{
		int offs = offsets[i];
		if (y == 8)
		{
			pvi_count[chip][i] = ram[offs + 12] - 8;
			pvi_display[chip][i] = pvi_duplicate[chip][i] = 0;
		}
		if (!pvi_count[chip][i])
		{
			pvi_count[chip][i] = 80;
			pvi_display[chip][i] = 1;
		}
		if (pvi_display[chip][i])
		{
			int inc = 1 << (3 - ((ram[0xc0] >> (i * 2)) & 3));
			int color = (ram[0xc1 + (i >> 1)] >> ((i & 1) ? 0 : 3)) & 7;
			int x, px, bits;
			pvi_count[chip][i] -= inc;
			bits = ram[offs + 9 - (pvi_count[chip][i] >> 3)];
			px = 171 + 3 * (ram[offs + (pvi_duplicate[chip][i] ? 11 : 10)] - 16);
			for (x = 0; x < 64; x += inc, px += 3)
				if ((bits << (x >> 3)) & 0x80)
				{
					int n;
					for (n = 0; n < 3; n++)
						if (px + n >= 171 && px + n <= 906) row[px + n] |= color;
				}
			if (!pvi_count[chip][i])
			{
				pvi_count[chip][i] = 1 + ram[offs + 13];
				pvi_display[chip][i] = 0;
				pvi_duplicate[chip][i] = 1;
			}
		}
		else pvi_count[chip][i]--;
	}
}

static void laserbat_video_line(int y)
{
	/* The legacy timer counts from the first visible line (hardware line 8). */
	int next_y = (y + 1) % 312;
	timer_set(cpu_getscanlinetime((next_y + 312 - 8) % 312), next_y, laserbat_video_line);
	if (y >= 8 && y <= 254)
	{
		UINT8 pvi[908];
		UINT8 *gfx1 = memory_region(REGION_GFX1), *gfx2 = memory_region(REGION_GFX2);
		UINT16 *row = video_bitmap->line[y];
		int eff1_val = eff_ram[(y & 0xff) | 0x100], eff2_val = eff_ram[y & 0xff];
		int sprite_row = y - ((256 - wcov) & 0xff);
		int sprite_x = 147 + 3 * ((256 - wcoh + 5) & 0xff);
		int bank = is_catnmous && (csound2 & 1) ? 0x800 : 0;
		int px;
		memset(pvi, 0, sizeof(pvi));
		pvi_line(pvi, pvi_ram[0], 0, y);
		pvi_line(pvi, pvi_ram[1], 1, y);
		pvi_line(pvi, pvi_ram[2], 2, y);
		for (px = 171; px <= 906; px++)
		{
			int x = (px - 147) / 3;
			int right = x & 0x80, mask = is_catnmous ? 3 : 0;
			int cmp1 = right ? ((x & 0x7f) < (eff1_val & 0x7f)) : ((x & 0x7f) > (~eff1_val & 0x7f));
			int cmp2 = right ? (((x & 0x7f) | mask) < ((eff2_val & 0x7f) | mask)) : (((x & 0x7f) | mask) > ((~eff2_val & 0x7f) | mask));
			int eff1 = !(cnt_eff & 1) && ((cnt_eff & 0x20) ? cmp1 : !cmp1);
			int eff2 = !(cnt_eff & 2) && ((cnt_eff & 0x40) ? cmp2 : !cmp2) && (cnt_eff & 4);
			int shell = !(cnt_eff & 6) && (((x & 0xff) | mask) == (eff2_val | mask));
			int bits = ((cnt_eff & 0x18) << 11) | ((cnt_nav & 0x0e) << 1);
			int pvi_bits = ~pvi[px];
			bits |= ((pvi_bits & 1) << 7) | ((pvi_bits & 2) << 5) | ((pvi_bits & 4) << 3);
			bits |= (eff1 ? 0x1000 : 0) | (eff2 ? 0x2000 : 0) | (shell ? 0x0800 : 0);
			if (x >= 9)
			{
				int bx = x - 9;
				int tile = bg_ram[((y << 2) & 0x3e0) | ((bx >> 3) & 31)] * 8 + (y & 7);
				int bit = 0x80 >> (bx & 7);
				bits |= (gfx1[tile] & bit ? 0x100 : 0) | (gfx1[0x800 + tile] & bit ? 0x200 : 0) | (gfx1[0x1000 + tile] & bit ? 0x400 : 0);
			}
			if (!(cnt_nav & 1) && sprite_row >= 0 && sprite_row < 32 && px >= sprite_x && px < sprite_x + 96)
			{
				int sx = (px - sprite_x) / 3;
				int addr = bank | ((cnt_nav & 0xe0) << 3) | (sprite_row << 3) | (sx >> 2);
				bits |= (gfx2[addr] >> (6 - 2 * (sx & 3))) & 3;
			}
			row[px] = mixing_table[bits];
		}
	}
}

VIDEO_START( laserbat )
{
	const UINT8 *bitstream = memory_region(REGION_PLDS) + 4;
	UINT32 products[48];
	UINT8 sums[48], mask;
	int term, byte, bit;
	UINT32 inp;
	is_catnmous = !strcmp(Machine->gamedrv->name, "catnmous") || !strcmp(Machine->gamedrv->name, "catmousa");
	for (term = 0; term < 48; term++)
	{
		products[term] = 0;
		for (byte = 0; byte < 4; byte++)
		{
			UINT8 bits = *bitstream++;
			for (bit = 0; bit < 4; bit++, bits >>= 2)
			{
				products[term] >>= 1;
				if (bits & 1) products[term] |= 0x80000000;
				if (bits & 2) products[term] |= 0x00008000;
			}
		}
		sums[term] = ~*bitstream++;
	}
	mask = *bitstream;
	for (inp = 0; inp < 0x10000; inp++)
	{
		mixing_table[inp] = 0;
		for (term = 0; term < 48; term++)
			if (!~(inp | (~inp << 16) | products[term])) mixing_table[inp] |= sums[term];
		mixing_table[inp] ^= mask;
	}
	video_bitmap = auto_bitmap_alloc_depth(908, 312, 16);
	fillbitmap(video_bitmap, 0, NULL);
	timer_set(cpu_getscanlinetime(0), 8, laserbat_video_line);
	return 0;
}

VIDEO_UPDATE( laserbat )
{
	int x, y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		const UINT16 *src = video_bitmap->line[flip_screen ? 262 - y : y];
		UINT16 *dst = bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dst[x] = Machine->pens[src[flip_screen ? 1077 - x : x]];
	}
}

static ADDRESS_MAP_START( laserbat_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_ROM
	AM_RANGE(0x2000, 0x33ff) AM_ROM
	AM_RANGE(0x3800, 0x3bff) AM_ROM
	AM_RANGE(0x4000, 0x53ff) AM_ROM
	AM_RANGE(0x6000, 0x73ff) AM_ROM
	AM_RANGE(0x7800, 0x7bff) AM_ROM

	AM_RANGE(0x1400, 0x14ff) AM_MIRROR(0x6000) AM_WRITENOP // always 0 (bullet ram in Quasar)
	AM_RANGE(0x1500, 0x15ff) AM_MIRROR(0x6000) AM_RAM AM_WRITE(laserbat_s2636_1_w) AM_BASE(&pvi_ram[0])
	AM_RANGE(0x1600, 0x16ff) AM_MIRROR(0x6000) AM_RAM AM_WRITE(laserbat_s2636_2_w) AM_BASE(&pvi_ram[1])
	AM_RANGE(0x1700, 0x17ff) AM_MIRROR(0x6000) AM_RAM AM_WRITE(laserbat_s2636_3_w) AM_BASE(&pvi_ram[2])
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0x6000) AM_WRITE(laserbat_videoram_w)
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0x6000) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( laserbat_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(MRA8_NOP) AM_WRITE(effects_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(video_extra_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(laserbat_input_r, csound1_w)
	AM_RANGE(0x04, 0x05) AM_WRITE(sprite_x_y_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(laserbat_input_mux_w)
	AM_RANGE(0x07, 0x07) AM_WRITE(csound2_w)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_5_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( catnmous_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_RAM
	AM_RANGE(0x400c, 0x400f) AM_MIRROR(0x1ff0) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( laserbat_base )
	PORT_START_TAG("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START_TAG("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_DIPNAME( 0x10, 0x10, DEF_STR(Cabinet) ) // sense line on wiring harness
	PORT_DIPSETTING(    0x10, DEF_STR(Upright) )
	PORT_DIPSETTING(    0x00, DEF_STR(Cocktail) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START_TAG("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)

	PORT_START_TAG("SW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR(Coin_A) )         PORT_DIPLOCATION("SW-1:1,2")
	PORT_DIPSETTING(    0x00, DEF_STR(1C_1C) )
	PORT_DIPSETTING(    0x01, DEF_STR(1C_2C) )
	PORT_DIPSETTING(    0x02, DEF_STR(1C_3C) )
	PORT_DIPSETTING(    0x03, DEF_STR(1C_5C) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR(Coin_B) )         PORT_DIPLOCATION("SW-1:3,4")
	PORT_DIPSETTING(    0x00, DEF_STR(1C_2C) )
	PORT_DIPSETTING(    0x04, DEF_STR(1C_3C) )
	PORT_DIPSETTING(    0x08, DEF_STR(1C_5C) )
	PORT_DIPSETTING(    0x0c, DEF_STR(1C_7C) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-1:5")
	PORT_DIPSETTING(    0x10, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-1:6")
	PORT_DIPSETTING(    0x20, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x40, 0x00, "Infinite Lives" )        PORT_DIPLOCATION("SW-1:7")
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x40, DEF_STR(On) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-1:8")
	PORT_DIPSETTING(    0x80, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )

	PORT_START_TAG("SW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-2:1")
	PORT_DIPSETTING(    0x01, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-2:2")
	PORT_DIPSETTING(    0x02, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-2:3")
	PORT_DIPSETTING(    0x04, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR(Difficulty) )     PORT_DIPLOCATION("SW-2:4,5")
	PORT_DIPSETTING(    0x00, DEF_STR(Easy) )
	PORT_DIPSETTING(    0x08, DEF_STR(Medium) )
	PORT_DIPSETTING(    0x10, DEF_STR(Hard) )
	PORT_DIPSETTING(    0x18, "Very Difficult" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-2:6")
	PORT_DIPSETTING(    0x20, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR(Unknown) )        PORT_DIPLOCATION("SW-2:7")
	PORT_DIPSETTING(    0x40, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x80, 0x80, "Coin C" )                PORT_DIPLOCATION("SW-2:8")
	PORT_DIPSETTING(    0x00, DEF_STR(2C_1C) )
	PORT_DIPSETTING(    0x80, DEF_STR(1C_1C) )
	PORT_START_TAG("SENSE")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( laserbat )
	PORT_INCLUDE(laserbat_base)

	PORT_MODIFY("SW1")
	PORT_DIPNAME( 0x30, 0x10, DEF_STR(Lives) )          PORT_DIPLOCATION("SW-1:5,6")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x80, 0x80, "Collision Detection" )   PORT_DIPLOCATION("SW-1:8")
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x80, DEF_STR(On) )

	PORT_MODIFY("SW2")
	PORT_DIPNAME( 0x60, 0x40, DEF_STR(Bonus_Life) )     PORT_DIPLOCATION("SW-2:6,7")
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x20, "10,000" )
	PORT_DIPSETTING(    0x40, "14,000" )
	PORT_DIPSETTING(    0x60, "18,000" )
INPUT_PORTS_END

INPUT_PORTS_START( lazarian )
	PORT_INCLUDE(laserbat)

	PORT_MODIFY("SW1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR(Coin_A) )         PORT_DIPLOCATION("SW-1:1,2")
	PORT_DIPSETTING(    0x00, DEF_STR(2C_1C) )
	PORT_DIPSETTING(    0x01, DEF_STR(1C_1C) )
	PORT_DIPSETTING(    0x02, DEF_STR(1C_2C) )
	PORT_DIPSETTING(    0x03, DEF_STR(1C_3C) )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR(Lives) )          PORT_DIPLOCATION("SW-1:5,6")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Calibration Display" )   PORT_DIPLOCATION("SW-1:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_MODIFY("SW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR(Unused) )         PORT_DIPLOCATION("SW-2:1") // manual says not used
	PORT_DIPSETTING(    0x01, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x02, 0x02, "Firing" )                PORT_DIPLOCATION("SW-2:2")
	PORT_DIPSETTING(    0x02, "Rapid" )
	PORT_DIPSETTING(    0x00, DEF_STR(Normal) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )                PORT_DIPLOCATION("SW-2:3")
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x04, DEF_STR(On) )
INPUT_PORTS_END

INPUT_PORTS_START( catnmous )
	PORT_INCLUDE(laserbat_base)

	PORT_MODIFY("ROW0")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_MODIFY("SW1")
	PORT_DIPNAME( 0x30, 0x10, DEF_STR(Lives) )          PORT_DIPLOCATION("SW-1:5,6")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR(Demo_Sounds) )    PORT_DIPLOCATION("SW-1:8")
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x80, DEF_STR(On) )

	PORT_MODIFY("SW2")
	PORT_DIPNAME( 0x01, 0x01, "Free Play" )             PORT_DIPLOCATION("SW-2:1") // taken from manual, assuming poor translation
	PORT_DIPSETTING(    0x01, "Win Play" )
	PORT_DIPSETTING(    0x00, "No Win Play" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR(Unused) )         PORT_DIPLOCATION("SW-2:2") // manual says not used
	PORT_DIPSETTING(    0x02, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR(Unused) )         PORT_DIPLOCATION("SW-2:3") // manual says not used
	PORT_DIPSETTING(    0x04, DEF_STR(Off) )
	PORT_DIPSETTING(    0x00, DEF_STR(On) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR(Bonus_Life) )     PORT_DIPLOCATION("SW-2:6,7")
	PORT_DIPSETTING(    0x00, DEF_STR(Off) )
	PORT_DIPSETTING(    0x20, "20,000" )
	PORT_DIPSETTING(    0x40, "24,000" )
	PORT_DIPSETTING(    0x60, "28,000" )
INPUT_PORTS_END


/* Laser Battle sound.
 * Only the 8-foot tone outputs are connected, as in MAME.  The CSG
 * modulates the analog filter network; its audio output is not connected.
 * Analog distortion/filter switching is still imperfect, as in MAME.
 */
struct laserbat_tms3615
{
	sound_stream *stream;
	int clock;
	int counter[13];
	UINT16 output;
	UINT16 enable;
};

static struct laserbat_tms3615 *laserbat_synth[2];
static UINT32 laserbat_keys;

static void laserbat_tms_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	static const int divisor[] = { 478, 451, 426, 402, 379, 358, 338, 319, 301, 284, 268, 253, 239 };
	struct laserbat_tms3615 *synth = param;
	int sample, tone;
	for (sample = 0; sample < length; sample++)
	{
		int sum = 0;
		for (tone = 0; tone < 13; tone++)
		{
			synth->counter[tone] -= synth->clock / divisor[tone];
			while (synth->counter[tone] <= 0)
			{
				synth->counter[tone] += synth->clock / 8;
				synth->output ^= 1 << tone;
			}
			if (synth->output & synth->enable & (1 << tone))
				sum += 32767 / 13;
		}
		outputs[0][sample] = sum;
	}
}

static void *laserbat_tms_start(int clock, const struct CustomSound_interface *config)
{
	struct laserbat_tms3615 *synth = auto_malloc(sizeof(*synth));
	memset(synth, 0, sizeof(*synth));
	synth->clock = clock;
	synth->stream = stream_create(0, 1, clock / 8, synth, laserbat_tms_update);
	*(struct laserbat_tms3615 **)config->extra_data = synth;
	return synth;
}

static void laserbat_tms_reset(void *param)
{
	struct laserbat_tms3615 *synth = param;
	stream_update(synth->stream, 0);
	memset(synth->counter, 0, sizeof(synth->counter));
	synth->output = synth->enable = 0;
}

static void laserbat_tms_enable(int chip, int enable)
{
	struct laserbat_tms3615 *synth = laserbat_synth[chip];
	if (synth->enable != enable)
	{
		stream_update(synth->stream, 0);
		synth->enable = enable;
	}
}

static const struct CustomSound_interface laserbat_tms_interface[2] =
{
	{ laserbat_tms_start, 0, laserbat_tms_reset, &laserbat_synth[0] },
	{ laserbat_tms_start, 0, laserbat_tms_reset, &laserbat_synth[1] }
};

static const struct SN76477interface sn76477_interface =
{
	RES_K(47), RES_K(270), CAP_P(1000), /* noise oscillator/filter */
	0, 0, 0,                         /* decay/attack disconnected */
	RES_K(47), 0,                    /* amplitude/feedback */
	5.0 * 2200 / (4700 + 2200), 0, RES_K(47), 5.0, /* VCO/pitch */
	0, CAP_U(4.7),                   /* SLF */
	0, 0                             /* one-shot disconnected */
};

static void laserbat_sound_w(int data)
{
	static const double noise_res[] = { 270000, 220000, 150000, 120000, 82000, 68000, 47000, 33000 };
	static const double vco_res[] = { 47000, 27000, 22000, 15000, 12000, 8200, 6800, 4700 };
	int diff = data ^ csound2;

	if (data & diff & 1)
	{
		SN76477_set_filter_res(0, noise_res[csound1 & 7]);
		SN76477_set_vco_res(0, vco_res[csound1 & 7]);
		SN76477_enable_w(0, (csound1 >> 3) & 1);
		SN76477_mixer_b_w(0, (csound1 >> 4) & 1);
	}

	laserbat_keys = (laserbat_keys & 0x00ffffff) | ((data & 2) ? 0x01000000 : 0);
	/* Latch on leaving a selected key bank while bit 13 was high. */
	if ((csound2 & 0x10) && (diff & 0x1c))
	{
		switch (csound2 & 0x0c)
		{
		case 0x04:
			laserbat_keys = (laserbat_keys & 0x01ffff00) | csound1;
			break;
		case 0x08:
			laserbat_keys = (laserbat_keys & 0x01ff00ff) | ((UINT32)csound1 << 8);
			break;
		case 0x0c:
			laserbat_keys = (laserbat_keys & 0x0100ffff) | ((UINT32)csound1 << 16);
			break;
		}
	}
	laserbat_tms_enable(0, laserbat_keys & 0x1fff);
	laserbat_tms_enable(1, (laserbat_keys >> 12) & 0x1ffe);

	switch (data & 0x1c)
	{
	case 0x00: SN76477_set_slf_res(0, RES_K(27)); break;
	case 0x04:
	case 0x08: SN76477_set_slf_res(0, RES_K(22)); break;
	case 0x0c: SN76477_set_slf_res(0, RES_K(12)); break;
	default:   SN76477_set_slf_res(0, 0); break; /* disconnected */
	}
	SN76477_vco_w(0, (data & 0x40) ? 0 : 1);
	/* Filter/distortion controls and the TMS reset line remain unmodelled. */
}

static void zaccaria_irq0a(int state) { cpunum_set_input_line(1, INPUT_LINE_NMI, state ? ASSERT_LINE : CLEAR_LINE); }
static void zaccaria_irq0b(int state) { cpunum_set_input_line(1,0,state ? ASSERT_LINE : CLEAR_LINE); }

static int cb1_toggle;

static READ8_HANDLER( zaccaria_port0a_r )
{
	int control = pia_get_output_b(0), data = 0xff;
	if ((control & 3) == 1) data &= AY8910_read_port_0_r(0);
	if ((control & 12) == 4) data &= AY8910_read_port_1_r(0);
	return data;
}

static WRITE8_HANDLER( zaccaria_port0a_w )
{
	int control = pia_get_output_b(0);
	if (control & 2)
	{
		if (control & 1) AY8910_control_port_0_w(0, data);
		else AY8910_write_port_0_w(0, data);
	}
	if (control & 8)
	{
		if (control & 4) AY8910_control_port_1_w(0, data);
		else AY8910_write_port_1_w(0, data);
	}
}

static WRITE8_HANDLER( zaccaria_port0b_w )
{
	int value = pia_get_output_a(0);
	if (data & 2)
	{
		if (data & 1) AY8910_control_port_0_w(0, value);
		else AY8910_write_port_0_w(0, value);
	}
	if (data & 8)
	{
		if (data & 4) AY8910_control_port_1_w(0, value);
		else AY8910_write_port_1_w(0, value);
	}
}

static READ8_HANDLER( catnmous_sound_command_r )
{
	return csound1 & 0x1f;
}

static const pia6821_interface pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zaccaria_port0a_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zaccaria_port0a_w, zaccaria_port0b_w, 0, 0,
	/*irqs   : A/B             */ zaccaria_irq0a, zaccaria_irq0b
};

static WRITE8_HANDLER( catnmous_volume_w )
{
	static const int resistors[] = { 8200, 820, 3300, 150, 5600, 390, 1500, 47 };
	int resistance = resistors[data & 7];
	AY8910_set_volume(1, 1, 150 * resistance / (4700 + resistance));
}

static struct AY8910interface ay8910_interface =
{
	0,
	catnmous_sound_command_r,
	catnmous_volume_w,
	0
};

static MACHINE_START( catnmous )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia_0_intf);
	return 0;
}

static MACHINE_RESET( laserbat )
{
	cnt_eff = cnt_nav = wcoh = wcov = csound1 = csound2 = input_mux = 0;
	memset(bg_ram, 0, sizeof(bg_ram));
	memset(eff_ram, 0, sizeof(eff_ram));
	memset(pvi_count, 0, sizeof(pvi_count));
	memset(pvi_display, 0, sizeof(pvi_display));
	memset(pvi_duplicate, 0, sizeof(pvi_duplicate));
	flip_screen_set(0);
	laserbat_keys = 0;
	if (!is_catnmous)
	{
		SN76477_mixer_w(0, 0);
		SN76477_envelope_w(0, 2);
		SN76477_vco_w(0, 1);
		SN76477_enable_w(0, 0);
	}
}

static MACHINE_RESET( catnmous )
{
	machine_reset_laserbat();
	cb1_toggle = 0;
	pia_reset();
}


static INTERRUPT_GEN( laserbat_interrupt )
{
	cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0x0a);
}

static INTERRUPT_GEN( zaccaria_cb1_toggle )
{
	cb1_toggle ^= 1;
	pia_0_cb1_w(0, cb1_toggle);
}


static MACHINE_DRIVER_START( laserbat )
	MDRV_MACHINE_RESET(laserbat)

	MDRV_CPU_ADD(S2650, 14318181/4)
	MDRV_CPU_PROGRAM_MAP(laserbat_map,0)
	MDRV_CPU_IO_MAP(laserbat_io_map,0)
	MDRV_CPU_VBLANK_INT(laserbat_interrupt,1)

	MDRV_FRAMES_PER_SECOND(14318181.0 / (908 * 312))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 65 * 908 / 14318181.0))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(908, 312)
	MDRV_ASPECT_RATIO(4, 3)
	MDRV_VISIBLE_AREA(171, 906, 8, 254)

	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(laserbat)

	MDRV_VIDEO_START(laserbat)
	MDRV_VIDEO_UPDATE(laserbat)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_interface)
	/* CSG controls analog distortion; its audio pin is not connected. */

	MDRV_SOUND_ADD(CUSTOM, 4000000/16/2)
	MDRV_SOUND_CONFIG(laserbat_tms_interface[0])
	MDRV_SOUND_ROUTE(0, "mono", 0.5)

	MDRV_SOUND_ADD(CUSTOM, 4000000/16)
	MDRV_SOUND_CONFIG(laserbat_tms_interface[1])
	MDRV_SOUND_ROUTE(0, "mono", 0.5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( catnmous )

	MDRV_CPU_ADD(S2650, 14318181/4)
	MDRV_CPU_PROGRAM_MAP(laserbat_map,0)
	MDRV_CPU_IO_MAP(laserbat_io_map,0)
	MDRV_CPU_VBLANK_INT(laserbat_interrupt,1)

	MDRV_CPU_ADD(M6802,3579545/4) /* core expects the divided E clock */
	MDRV_CPU_PROGRAM_MAP(catnmous_sound_map,0)
	MDRV_CPU_PERIODIC_INT(zaccaria_cb1_toggle,TIME_IN_HZ(3579545.0/4096))

	MDRV_FRAMES_PER_SECOND(14318181.0 / (908 * 312))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 65 * 908 / 14318181.0))

	MDRV_MACHINE_START(catnmous)
	MDRV_MACHINE_RESET(catnmous)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(908, 312)
	MDRV_ASPECT_RATIO(4, 3)
	MDRV_VISIBLE_AREA(171, 906, 8, 254)

	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(laserbat)

	MDRV_VIDEO_START(laserbat)
	MDRV_VIDEO_UPDATE(laserbat)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	/* Each legacy AY channel has full-scale output: mix six channels safely. */
	MDRV_SOUND_ADD(AY8910, 3579545/2)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0/6)

	MDRV_SOUND_ADD(AY8910, 3579545/2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0/6)
MACHINE_DRIVER_END


/*

Main cpu : 2650 signetics
quartz   : can't read it
Sub      : 2636 signetics (3 pieces)
ram      : 2114 (6 pieces in total)
special  : 82s100
special  : 2621N

Sound board info :

TMS SN76477N
TMS 3615NS-28 (x2)
Xtal : 4.000 Mhz

+ a few usual 74 chips

*/

ROM_START( laserbat )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "lb02.7c",      0x0000, 0x0400, CRC(23a257cd) SHA1(08d9e1ff1a5cd8a5e5af6a12ba6104d3b2ccfddf) )
	ROM_CONTINUE(			  0x4000, 0x0400 )
	ROM_LOAD( "lb02.6c",      0x0400, 0x0400, CRC(d1d6a67a) SHA1(727898c733633daffb0193cf4a556f89fe7e8a5a) )
	ROM_CONTINUE(			  0x4400, 0x0400 )
	ROM_LOAD( "lb02.5c",      0x0800, 0x0400, CRC(8116f1d3) SHA1(f84ace44434c55ca5d0be9f0beb2d4df75694b2f) )
	ROM_CONTINUE(			  0x4800, 0x0400 )
	ROM_LOAD( "lb02.3c",      0x0c00, 0x0400, CRC(443ef61e) SHA1(2849af0551bba7be2b4792739e04f18d6ace254c) )
	ROM_CONTINUE(			  0x4c00, 0x0400 )
	ROM_LOAD( "lb02.2c",      0x1000, 0x0400, CRC(0cb8f5f1) SHA1(4ce22c5ae277033cb9905339d24cad272a878088) )
	ROM_CONTINUE(			  0x5000, 0x0400 )
	ROM_LOAD( "lb02.7b",      0x2000, 0x0400, CRC(bdc769d1) SHA1(1291c159e779187efbdc3eb4a59a57d8d25ce08e) )
	ROM_CONTINUE(			  0x6000, 0x0400 )
	ROM_LOAD( "lb02.6b",      0x2400, 0x0400, CRC(2103646f) SHA1(bbd15a19524aeb8647014914a0b3025a975dfe7c) )
	ROM_CONTINUE(			  0x6400, 0x0400 )
	ROM_LOAD( "lb02.5b",      0x2800, 0x0400, CRC(3f8c4246) SHA1(b0d5e3733327140f54ac5a93f3f14d4afe085514) )
	ROM_CONTINUE(			  0x6800, 0x0400 )
	ROM_LOAD( "lb02.3b",      0x2c00, 0x0400, CRC(3e557d52) SHA1(860046fcc2d952f3e677e576f1ac23deac2e7caf) )
	ROM_CONTINUE(			  0x6c00, 0x0400 )
	ROM_LOAD( "lb02.2b",      0x3000, 0x0400, CRC(39000248) SHA1(58c6d1c588f4d1a3f579fe14faa8d2ccdfdc001e) )
	ROM_CONTINUE(			  0x7000, 0x0400 )

	ROM_REGION( 0x1800, REGION_GFX1, 0 )
	ROM_LOAD( "lb02.8g",      0x0000, 0x0800, CRC(4bb9f452) SHA1(1ff4ef94f0da3b59377548f3341b083af83f83c6) )
	ROM_LOAD( "lb02.10g",     0x0800, 0x0800, CRC(5fec6517) SHA1(868e57e8498cf1ab0fa3635845cdb5800fd96855) )
	ROM_LOAD( "lb02.11g",     0x1000, 0x0800, CRC(ceaf00a4) SHA1(2e789898207caa7619dcbb01f52c3532d1482618) )

	ROM_REGION( 0x0800, REGION_GFX2, 0 )
	ROM_LOAD( "lb02.14l",     0x0000, 0x0800, CRC(d29962d1) SHA1(5b6d0856c3ebbd5833b522f7c0240309cf3c9777) )

	ROM_REGION( 0x0100, REGION_PLDS, 0 )

	ROM_LOAD( "82s100_prom",  0x0000, 0x00f5, BAD_DUMP CRC(c3eb562a) SHA1(65dff81b2e5321d530e5171dab9aa3809ab38b4d) )
ROM_END

ROM_START( lazarian )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "laz.7c",      0x0000, 0x0400, CRC(a2454cf2) SHA1(163b9323e77ee0107e13860b3468e002c335df9e) )
	ROM_CONTINUE(			 0x4000, 0x0400 )
	ROM_LOAD( "laz.6c",      0x0400, 0x0400, CRC(23ee6013) SHA1(7ad53d6c321b0161906a512f6575620fd049d2f7) )
	ROM_CONTINUE(			 0x4400, 0x0400 )
	ROM_LOAD( "laz.5c",      0x0800, 0x0400, CRC(4234a2ed) SHA1(dc98b04ae7dd1c35687bd8bdf42e8feb5eed321d) )
	ROM_CONTINUE(			 0x4800, 0x0400 )
	ROM_LOAD( "laz.3c",      0x0c00, 0x0400, CRC(e901a636) SHA1(86320181a4d697fedfe8d8cbf9189854781e3d8c) )
	ROM_CONTINUE(			 0x4c00, 0x0400 )
	ROM_LOAD( "laz.2c",      0x1000, 0x0400, CRC(657ed7c2) SHA1(8611912001d18af8c932efc7700c0d8b60efb2e8) )
	ROM_CONTINUE(			 0x5000, 0x0400 )
	ROM_LOAD( "laz.7b",      0x2000, 0x0400, CRC(43135808) SHA1(2b704ca2f7a0fc46fddd5d7fb7d832a29d0562d0) )
	ROM_CONTINUE(			 0x6000, 0x0400 )
	ROM_LOAD( "laz.6b",      0x2400, 0x0400, CRC(95701e50) SHA1(61d6a268696cefb760bf288bcc4eab7ac5f32ec7) )
	ROM_CONTINUE(			 0x6400, 0x0400 )
	ROM_LOAD( "laz.5b",      0x2800, 0x0400, CRC(685842ba) SHA1(ee842d1d2c0676fddddf6e4e9cfd0b2962ae900d) )
	ROM_CONTINUE(			 0x6800, 0x0400 )
	ROM_LOAD( "laz.3b",      0x2c00, 0x0400, CRC(9ddbe048) SHA1(70d1e8af073c85aba08e5251691842069617e6ac) )
	ROM_CONTINUE(			 0x6c00, 0x0400 )
	ROM_LOAD( "laz10-62.2b", 0x3800, 0x0400, CRC(4ad9f7af) SHA1(71bcb9d148a7372b7be0abccdf71eeedba8b6c0a) )
	ROM_CONTINUE(			 0x7800, 0x0400 )
	ROM_CONTINUE(			 0x3000, 0x0400 )
	ROM_CONTINUE(			 0x7000, 0x0400 )

	ROM_REGION( 0x1800, REGION_GFX1, 0 )
	ROM_LOAD( "laz.8g",      0x0000, 0x0800, CRC(3cf76c01) SHA1(1824bc05e8dd2a522409e95fe81d2ad64182dcac) )
	ROM_LOAD( "laz.10g",     0x0800, 0x0800, CRC(256ae65d) SHA1(7f9e8ea1bbcb9e2175544556795c88c9981db571) )
	ROM_LOAD( "laz.11g",     0x1000, 0x0800, CRC(fec8266a) SHA1(7b90ae8d9eeb148012cca1bc93546dc3bf509258) )

	ROM_REGION( 0x0800, REGION_GFX2, 0 )
	ROM_LOAD( "laz.14l",      0x0000, 0x0800, CRC(d29962d1) SHA1(5b6d0856c3ebbd5833b522f7c0240309cf3c9777) )

	ROM_REGION( 0x0100, REGION_PLDS, 0 )
	ROM_LOAD( "lz82s100.bin", 0x0000, 0x00f5, CRC(c3eb562a) SHA1(65dff81b2e5321d530e5171dab9aa3809ab38b4d) )
ROM_END

ROM_START( catnmous )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "02-1.7c",      0x0000, 0x0400, CRC(d26ec566) SHA1(ceb16f64a3c1ff25a9eab6549f1ae24085bb9e27) )
	ROM_CONTINUE(			  0x4000, 0x0400 )
	ROM_LOAD( "02-2.6c",      0x0400, 0x0400, CRC(02a7e36c) SHA1(8495b2906ecb0791a47e9b6f1959ed6cbc14cce8) )
	ROM_CONTINUE(			  0x4400, 0x0400 )
	ROM_LOAD( "02-3.5c",      0x0800, 0x0400, CRC(ee9f90ee) SHA1(dc280dae3a18a9044497bdee41827d2510a04d06) )
	ROM_CONTINUE(			  0x4800, 0x0400 )
	ROM_LOAD( "02-4.3c",      0x0c00, 0x0400, CRC(71b97af9) SHA1(6735184dc16c8db3050be3b7b5dfdb7d46a671fe) )
	ROM_CONTINUE(			  0x4c00, 0x0400 )
	ROM_LOAD( "02-5.2c",      0x1000, 0x0400, CRC(887a1da2) SHA1(9e2548d1792c2d2b76811a1e0daae4d378f1f354) )
	ROM_CONTINUE(			  0x5000, 0x0400 )
	ROM_LOAD( "02-6.7b",      0x2000, 0x0400, CRC(22e045e9) SHA1(dd332e918500d8024d1329bc12c6f939fd41e4a7) )
	ROM_CONTINUE(			  0x6000, 0x0400 )
	ROM_LOAD( "02-7.6b",      0x2400, 0x0400, CRC(af330ad2) SHA1(cac70341687edd1daee323c0e332297c80057e1e) )
	ROM_CONTINUE(			  0x6400, 0x0400 )
	ROM_LOAD( "02-8.5b",      0x2800, 0x0400, CRC(c7d38401) SHA1(33a3bb393451cd3fefa23b5c8013068b5b0de7a5) )
	ROM_CONTINUE(			  0x6800, 0x0400 )
	ROM_LOAD( "02-9.3b",      0x2c00, 0x0400, CRC(c4a33f20) SHA1(355c4345daa681fa2bcfa1e345d2db34f9d94113) )
	ROM_CONTINUE(			  0x6c00, 0x0400 )
	ROM_LOAD( "02-10-11.2b",  0x3800, 0x0400, CRC(3f7d4b89) SHA1(c8e9be0149a2f728526a416ec5663e69cc2e6758) )
	ROM_CONTINUE(			  0x7800, 0x0400 )
	ROM_CONTINUE(			  0x3000, 0x0400 )
	ROM_CONTINUE(			  0x7000, 0x0400 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound01.1d",   0xe000, 0x1000, CRC(f65cb9d0) SHA1(a2fe7563c6da055bf6aa20797b2d9fa184f0133c) )
	ROM_LOAD( "sound01.1f",   0xc000, 0x1000, CRC(473c44de) SHA1(ff08b02d45a2c23cabb5db716aa203225a931424) )
	ROM_LOAD( "sound01.1e",   0xf000, 0x1000, CRC(1bd90c93) SHA1(20fd2b765a42e25cf7f716e6631b8c567785a866) )

	ROM_REGION( 0x1800, REGION_GFX1, 0 )
	ROM_LOAD( "type01.8g",    0x0000, 0x0800, CRC(2b180d4a) SHA1(b6f48ffdbad64b4d9f1fe838000187800c51228c) )
	ROM_LOAD( "type01.10g",   0x0800, 0x0800, CRC(e5259f9b) SHA1(396753291ab36c3ed72208d619665fc0f33d1e17) )
	ROM_LOAD( "type01.11g",   0x1000, 0x0800, CRC(2999f378) SHA1(929082383b2b0006de171587adb932ce57316963) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_14l.14l", 0x0000, 0x1000, CRC(83502383) SHA1(9561f87e1a6425bb9544e71340336db8d43c1fd9) )

	ROM_REGION( 0x0100, REGION_PLDS, 0 )
	ROM_LOAD( "82s100.13m",   0x0000, 0x00f5, CRC(6b724cdb) SHA1(8a0ca3b171b103661a3b2fffbca3d7162089e243) )
ROM_END

ROM_START( catmousa )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_1.7c",  0x0000, 0x0400, CRC(0bf9fc06) SHA1(7d5857121fe51f43e4ae7db34df720198994afdd) )
	ROM_CONTINUE(                                0x4000, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_2.6c",  0x0400, 0x0400, CRC(b0e140a0) SHA1(68d8ca25642e872f2177d09b78d553c033411dd5) )
	ROM_CONTINUE(                                0x4400, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_3.5c",  0x0800, 0x0400, CRC(7bbc0fe5) SHA1(d20e89d89a0958d45ac31b6d2c540fcf3d326068) )
	ROM_CONTINUE(                                0x4800, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_4.3c",  0x0c00, 0x0400, CRC(0350531d) SHA1(6115f907544ab317e0090a10cce3adce26f4afd9) )
	ROM_CONTINUE(                                0x4c00, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_5.2c",  0x1000, 0x0400, CRC(4a26e963) SHA1(be8dd98d3810319a228ce4c07b097eb75f2d1e5c) )
	ROM_CONTINUE(                                0x5000, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_6.7b",  0x2000, 0x0400, CRC(d8d6a029) SHA1(7e5688fd3af97620ed07d9375335fe1deb6e483f) )
	ROM_CONTINUE(                                0x6000, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_7.6b",  0x2400, 0x0400, CRC(ccc871d9) SHA1(355eff250ab3d1a75ed690369add1639e7061ee8) )
	ROM_CONTINUE(                                0x6400, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_8.5b",  0x2800, 0x0400, CRC(23783b84) SHA1(97a3ef7c64e1ded5cc1999d3aa58652ca541166c) )
	ROM_CONTINUE(                                0x6800, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_9.3b",  0x2c00, 0x0400, CRC(e99fce4b) SHA1(2c8efdea55bae5526b547fec53e8f3642fe2bd2e) )
	ROM_CONTINUE(                                0x6c00, 0x0400 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_10.2b", 0x3800, 0x0400, CRC(807b7109) SHA1(6c29197b437ab0132d8361f921e6d0d9b10f917e) )
	ROM_CONTINUE(                                0x7800, 0x0400 )
	ROM_CONTINUE(                                0x3000, 0x0400 )
	ROM_CONTINUE(                                0x7000, 0x0400 )

	ROM_REGION( 0x1800, REGION_GFX1, 0 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_8g.8g",   0x0000, 0x0800, CRC(2b180d4a) SHA1(b6f48ffdbad64b4d9f1fe838000187800c51228c) )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_10g.10g", 0x0800, 0x0800, CRC(e5259f9b) SHA1(396753291ab36c3ed72208d619665fc0f33d1e17) )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_11g.11g", 0x1000, 0x0800, CRC(2999f378) SHA1(929082383b2b0006de171587adb932ce57316963) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	ROM_LOAD( "cat_n_mouse_type_01_mem_n_14l.14l", 0x0000, 0x1000, CRC(83502383) SHA1(9561f87e1a6425bb9544e71340336db8d43c1fd9) )

	ROM_REGION( 0x0100, REGION_PLDS, 0 ) // copied from parent set to give working graphics, need dump to confirm
	ROM_LOAD( "cnm_82s100.10m", 0x0000, 0x00f5, CRC(6b724cdb) SHA1(8a0ca3b171b103661a3b2fffbca3d7162089e243) BAD_DUMP ) // labeled C.N.M - 82S100

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "cat_n_mouse_sound_01_mem_n_1f.1f", 0xc000, 0x1000, CRC(473c44de) SHA1(ff08b02d45a2c23cabb5db716aa203225a931424) )
	ROM_LOAD( "cat_n_mouse_sound_01_mem_n_1d.1d", 0xe000, 0x1000, CRC(f65cb9d0) SHA1(a2fe7563c6da055bf6aa20797b2d9fa184f0133c) )
	ROM_LOAD( "cat_n_mouse_sound_01_mem_n_1e.1e", 0xf000, 0x1000, CRC(1bd90c93) SHA1(20fd2b765a42e25cf7f716e6631b8c567785a866) )
ROM_END


GAME( 1981, laserbat, 0,        laserbat, laserbat, 0, ROT0,  "Zaccaria", "Laser Battle",                    GAME_IMPERFECT_SOUND )
GAME( 1981, lazarian, laserbat, laserbat, lazarian, 0, ROT0,  "Bally Midway (Zaccaria License)", "Lazarian", GAME_IMPERFECT_SOUND )
GAME( 1982, catnmous, 0,        catnmous, catnmous, 0, ROT90, "Zaccaria", "Cat and Mouse (set 1)",           GAME_IMPERFECT_SOUND )
GAME( 1982, catmousa, catnmous, catnmous, catnmous, 0, ROT90, "Zaccaria", "Cat and Mouse (set 2)",           GAME_IMPERFECT_SOUND )
