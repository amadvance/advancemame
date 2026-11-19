// license:BSD-3-Clause
// copyright-holders:Luca Elia
/************************************************************************************************************

                                 -= Subsino (Newer) Tilemaps Hardware =-

                                  driver by   Luca Elia (l.elia@tin.it)


Two 1024x512 tilemaps. 256 color tiles. Tiles are 8x8 or a multiple (dynamic tile size).
There is RAM for 512 scroll values (line scroll). Video RAM is mirrored on multiple ranges.
One peculiarity is that video RAM access is split into high and low byte. The former is mapped
in program space, the latter in I/O space.

----------------------------------------------------------------------------------------------------------------
Year  Game                CPU         Sound            Custom                            Other
----------------------------------------------------------------------------------------------------------------
1996  Magic Train         HD647180*   U6295            SS9601, SS9602                    HM86171 RAMDAC, Battery
1996  Water-Nymph         HD647180*   U6295            SS9601, SS9602                    HM86171 RAMDAC, Battery
----------------------------------------------------------------------------------------------------------------
*   SS9600

Protection seems to work the same way on every game in this driver, using a bitbanged Dallas 1-Wire EEPROM.
First a Read ROM command is issued, and only the first 8 bits returned are examined to determine whether they
match the expected device code (0x14). If this test passes, the EEPROM contents are recalled and the first
64 bits are read out. These 64 bits are then unscrambled using a permutation table common to all games. The
second and final protection check compares byte 6 in the unscrambled data buffer against a game-specific ID
code. This EEPROM comes in a small TO-92 package which is very difficult to spot on PCB photos, though it
appears to be typically positioned near one of the edge connectors.

TODO:
- mtrain: implement hopper.

************************************************************************************************************/

#include "driver.h"
#include "machine/generic.h"
#include "machine/ds2430a.h"
#include "machine/subsino_crypt.h"
#include "sound/okim6295.h"
#include "sound/3812intf.h"

#define TILE_8x8	0
#define TILE_8x32	1

#define VRAM_LO		0
#define VRAM_HI		1

/* Layers */
static UINT16 *layer_videoram[2];
static int layer_scroll_x[2];
static int layer_scroll_y[2];
static int layer_tilesize[2];
static tilemap *layer_tmap[2];

static UINT16 *reelram;
static mame_bitmap *reelbitmap;

static UINT8 ss9601_byte_lo;
static UINT8 ss9601_disable;

static UINT8 dsw_mask;

/* SS9602 I/O */
static UINT8 io_data[10];
static UINT8 io_dir[10];

/* HM86171 RAMDAC */
static UINT8 ramdac_palram[3][256];
static UINT8 ramdac_index;
static UINT8 ramdac_int_index;


/***************************************************************************
                              Tilemaps Access
***************************************************************************/

static void get_tile_info(int layer, int tile_index)
{
	int addr;
	int offs;

	if (layer_tilesize[layer] == TILE_8x32)
	{
		addr = tile_index & (~0x180);
		offs = (tile_index / 0x80) & 3;
	}
	else
	{
		addr = tile_index;
		offs = 0;
	}

	SET_TILE_INFO(0, layer_videoram[layer][addr] + offs, 0, 0)
}

static void get_tile_info_0(int tile_index)
{
	get_tile_info(0, tile_index);
}

static void get_tile_info_1(int tile_index)
{
	get_tile_info(1, tile_index);
}

static void ss9601_videoram_w(int layer, int vram, offs_t offset, UINT8 data)
{
	UINT16 *videoram = layer_videoram[layer];
	int y;

	if (vram == VRAM_HI)
		videoram[offset] = (data << 8) | (videoram[offset] & 0xff);
	else
		videoram[offset] = data | (videoram[offset] & 0xff00);

	if (layer_tilesize[layer] == TILE_8x32)
	{
		offset &= ~0x180;
		for (y = 0; y < 0x80 * 4; y += 0x80)
			tilemap_mark_tile_dirty(layer_tmap[layer], offset + y);
	}
	else
		tilemap_mark_tile_dirty(layer_tmap[layer], offset);
}

static WRITE8_HANDLER( ss9601_byte_lo_w )
{
	ss9601_byte_lo = data;
}

static WRITE8_HANDLER( videoram_mirror_w )
{
	int vram = (ss9601_byte_lo & 0x08) ? VRAM_HI : VRAM_LO;

	switch (ss9601_byte_lo & (~0x08))
	{
	case 0x00:
		ss9601_videoram_w(1, vram, offset, data);
		ss9601_videoram_w(1, vram, offset + 0x1000, data);
		break;

	case 0x04:
		ss9601_videoram_w(0, vram, offset, data);
		ss9601_videoram_w(0, vram, offset + 0x1000, data);
		break;

	case 0x06:
		if (vram == VRAM_HI)
			reelram[offset] = (data << 8) | (reelram[offset] & 0xff);
		else
			reelram[offset] = data | (reelram[offset] & 0xff00);
		break;
	}
}

static WRITE8_HANDLER( tilesize_w )
{
	int sizes[2];
	int i;

	switch (data)
	{
	case 0x01:
		sizes[0] = TILE_8x32;
		sizes[1] = TILE_8x8;
		break;

	default:
		sizes[0] = TILE_8x8;
		sizes[1] = TILE_8x8;
		break;
	}

	for (i = 0; i < 2; i++)
	{
		if (layer_tilesize[i] != sizes[i])
		{
			layer_tilesize[i] = sizes[i];
			tilemap_mark_all_tiles_dirty(layer_tmap[i]);
		}
	}
}

static WRITE8_HANDLER( ss9601_scroll_w )
{
	switch (offset)
	{
	/* Layer 0 */
	case 0:
		layer_scroll_x[0] = (layer_scroll_x[0] & 0xf00) | data;
		break;
	case 1:
		layer_scroll_y[0] = (layer_scroll_y[0] & 0xf00) | data;
		break;
	case 2:
		layer_scroll_x[0] = (layer_scroll_x[0] & 0x0ff) | ((data & 0x0f) << 8);
		layer_scroll_y[0] = (layer_scroll_y[0] & 0x0ff) | ((data & 0xf0) << 4);
		break;

	/* Layer 1 */
	case 3:
		layer_scroll_x[1] = (layer_scroll_x[1] & 0xf00) | data;
		break;
	case 4:
		layer_scroll_y[1] = (layer_scroll_y[1] & 0xf00) | data;
		break;
	case 5:
		layer_scroll_x[1] = (layer_scroll_x[1] & 0x0ff) | ((data & 0x0f) << 8);
		layer_scroll_y[1] = (layer_scroll_y[1] & 0x0ff) | ((data & 0xf0) << 4);
		break;
	}
}

static WRITE8_HANDLER( ss9601_disable_w )
{
	ss9601_disable = data;
}


/***************************************************************************
                                  RAMDAC
***************************************************************************/

static WRITE8_HANDLER( ramdac_index_w )
{
	ramdac_index = data;
	ramdac_int_index = 0;
}

static WRITE8_HANDLER( ramdac_pal_w )
{
	ramdac_palram[ramdac_int_index][ramdac_index] = data & 0x3f;

	palette_set_color(ramdac_index,
			pal6bit(ramdac_palram[0][ramdac_index]),
			pal6bit(ramdac_palram[1][ramdac_index]),
			pal6bit(ramdac_palram[2][ramdac_index]));

	if (++ramdac_int_index == 3)
	{
		ramdac_int_index = 0;
		++ramdac_index;
	}
}

static WRITE8_HANDLER( ramdac_mask_w )
{
	/* the mask is not used */
}


/***************************************************************************
                                Video Update
***************************************************************************/

static VIDEO_START( subsino2 )
{
	int i;

	for (i = 0; i < 2; i++)
	{
		layer_tmap[i] = tilemap_create(i ? get_tile_info_1 : get_tile_info_0, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 0x80, 0x40);
		if (!layer_tmap[i])
			return 1;

		tilemap_set_transparent_pen(layer_tmap[i], 0);

		layer_videoram[i] = auto_malloc(0x80 * 0x40 * sizeof(UINT16));
		memset(layer_videoram[i], 0, 0x80 * 0x40 * sizeof(UINT16));

		layer_scroll_x[i] = 0;
		layer_scroll_y[i] = 0;
		layer_tilesize[i] = TILE_8x8;
	}

	reelram = auto_malloc(0x2000 * sizeof(UINT16));
	memset(reelram, 0, 0x2000 * sizeof(UINT16));

	reelbitmap = auto_bitmap_alloc(0x80 * 8, 0x40 * 8);
	if (!reelbitmap)
		return 1;

	ss9601_disable = 0;

	return 0;
}

static int wrap_overlap(int t0, int tlen, int w0, int wlen, int size)
{
	if (((t0 - w0) & (size - 1)) < wlen)
		return 1;

	return ((w0 - t0) & (size - 1)) < tlen;
}

static VIDEO_UPDATE( subsino2 )
{
	int layers_ctrl = ~ss9601_disable;
	int x, y;
	int sx, sy;

	fillbitmap(bitmap, get_black_pen(), cliprect);

	/* Layer 0 is always made of reels */
	if (layers_ctrl & 1)
	{
		tilemap *tmap = layer_tmap[0];

		tilemap_set_scroll_rows(tmap, 1);
		tilemap_set_scroll_cols(tmap, 1);

		/* the reels are drawn in a larger bitmap, update only what is going to be copied to the screen */
		sx = (cliprect->min_x + layer_scroll_x[0]) & 0x3ff;
		sy = (cliprect->min_y + layer_scroll_y[0] + 1) & 0x1ff;

		for (y = 0; y < 0x20 / 4; y++)
		{
			if (!wrap_overlap(4 * 0x10 * y, 4 * 0x10, sy, cliprect->max_y - cliprect->min_y + 1, 0x200))
				continue;

			for (x = 0; x < 0x80; x++)
			{
				rectangle visible;
				rectangle tmp;
				int reeladdr = y * 0x80 * 4 + x;
				int reelscroll = reelram[reeladdr];
				int reelscroll_y;
				int reelwrap_y;

				if (!wrap_overlap(8 * x, 8, sx, cliprect->max_x - cliprect->min_x + 1, 0x400))
					continue;

				visible.min_x = 8 * x;
				visible.max_x = 8 * (x + 1) - 1;
				visible.min_y = 4 * 0x10 * y;
				visible.max_y = 4 * 0x10 * (y + 1) - 1;

				tilemap_set_scrollx(tmap, 0, (reelscroll >> 9) * 8 - visible.min_x);

				/* wrap around at half tilemap (0x100) */
				reelscroll_y = (reelscroll & 0x100) + ((reelscroll - visible.min_y) & 0xff);
				reelwrap_y = 0x100 - (reelscroll_y & 0xff);

				tmp = visible;

				/* draw above the wrap around y */
				if (reelwrap_y - 1 >= visible.min_y)
				{
					if (reelwrap_y - 1 <= visible.max_y)
						tmp.max_y = reelwrap_y - 1;
					tilemap_set_scrolly(tmap, 0, reelscroll_y);
					tilemap_draw(reelbitmap, &tmp, tmap, TILEMAP_IGNORE_TRANSPARENCY, 0);
					tmp.max_y = visible.max_y;
				}

				/* draw below the wrap around y */
				if (reelwrap_y <= visible.max_y)
				{
					if (reelwrap_y >= visible.min_y)
						tmp.min_y = reelwrap_y;
					tilemap_set_scrolly(tmap, 0, -((reelwrap_y & 0xff) | (reelscroll_y & 0x100)));
					tilemap_draw(reelbitmap, &tmp, tmap, TILEMAP_IGNORE_TRANSPARENCY, 0);
					tmp.min_y = visible.min_y;
				}
			}
		}

		sx = -layer_scroll_x[0];
		sy = -(layer_scroll_y[0] + 1);
		copyscrollbitmap(bitmap, reelbitmap, 1, &sx, 1, &sy, cliprect, TRANSPARENCY_NONE, 0);
	}

	if (layers_ctrl & 2)
	{
		tilemap *tmap = layer_tmap[1];

		tilemap_set_scroll_rows(tmap, 1);
		tilemap_set_scroll_cols(tmap, 1);
		tilemap_set_scrollx(tmap, 0, layer_scroll_x[1]);
		tilemap_set_scrolly(tmap, 0, layer_scroll_y[1] + 1);
		tilemap_draw(bitmap, cliprect, tmap, 0, 0);
	}
}


/***************************************************************************
                                Input / Output
***************************************************************************/

static WRITE8_HANDLER( dsw_mask_w )
{
	dsw_mask = data;
}

static UINT8 dsw_r(void)
{
	return ((readinputportbytag("DSW1") & dsw_mask) ? 0x01 : 0) |
		((readinputportbytag("DSW2") & dsw_mask) ? 0x02 : 0) |
		((readinputportbytag("DSW3") & dsw_mask) ? 0x04 : 0) |
		((readinputportbytag("DSW4") & dsw_mask) ? 0x08 : 0);
}

static void oki_bank_bit0_w(UINT8 data)
{
	/* it writes 0x32 or 0x33 */
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

/* Subsino SS9602 I/O: ports 0-8 are 8 bits wide, port 9 is 4 bits wide */

static UINT8 io_port_in(int port)
{
	switch (port)
	{
	case 3: /* serial out on bit 6, serial in on bit 7 */
		return (readinputportbytag("IN-D") & 0x7f) | (ds2430a_data_r() ? 0x80 : 0);
	case 4:
		return readinputportbytag("IN-A");
	case 5:
		return readinputportbytag("IN-B");
	case 6:
		return readinputportbytag("IN-C");
	case 7:
		return dsw_r();
	case 9:
		return cpu_getvblank() ? 0x04 : 0x00;
	}

	return 0;
}

static void io_port_out(int port, UINT8 data)
{
	switch (port)
	{
	case 0:
		coin_counter_w(0, data & 0x01); /* key in */
		coin_counter_w(1, data & 0x02); /* coin in */
		coin_counter_w(2, data & 0x10); /* pay out */
		break;

	case 3:
		ds2430a_data_w(!(data & 0x40));
		break;

	case 8:
		dsw_mask = data;
		break;

	case 9:
		oki_bank_bit0_w(data);
		break;
	}
}

static UINT8 io_read_port_data(int port)
{
	UINT8 dir = io_dir[port];
	UINT8 data = io_data[port] & dir;

	if (dir != 0xff)
		data |= io_port_in(port) & ~dir;

	return data;
}

static void io_write_port_data(int port, UINT8 data)
{
	UINT8 dir = io_dir[port];

	if ((dir & (data ^ io_data[port])) != 0)
		io_port_out(port, data & dir);

	io_data[port] = data;
}

static void io_write_port_dir(int port, UINT8 dir)
{
	if (io_dir[port] != dir)
	{
		io_dir[port] = dir;
		io_port_out(port, io_data[port] & dir);
	}
}

static void io_write_port_data_and_dir(int port, UINT8 data, UINT8 dir)
{
	if (io_dir[port] != dir || (dir & (data ^ io_data[port])) != 0)
	{
		io_dir[port] = dir;
		io_port_out(port, data & dir);
	}

	io_data[port] = data;
}

static READ8_HANDLER( ss9602_r )
{
	offset &= 0x1f;

	if (offset <= 0x08)
		return io_read_port_data(offset);
	else if (offset == 0x12)
		return io_read_port_data(9) & 0x0f;
	else if (offset >= 0x18 && offset <= 0x1e)
		return "SUBSION"[offset - 0x18];

	return 0;
}

static WRITE8_HANDLER( ss9602_w )
{
	offset &= 0x1f;

	if (offset <= 0x08)
		io_write_port_data(offset, data);
	else if (offset <= 0x11)
		io_write_port_dir(offset - 0x09, data);
	else if (offset == 0x12)
		io_write_port_data_and_dir(9, data & 0x0f, (data & 0xf0) >> 4);
}


/***************************************************************************
                                Memory Maps
***************************************************************************/

static ADDRESS_MAP_START( mtrain_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x06000, 0x0d7ff) AM_ROM AM_REGION(REGION_USER1, 0x8100)
	AM_RANGE(0x0d800, 0x0dfff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)	/* battery */
	AM_RANGE(0x0e000, 0x0efff) AM_WRITE(videoram_mirror_w)
	AM_RANGE(0x0f11f, 0x0f11f) AM_WRITE(ss9601_disable_w)
	AM_RANGE(0x0f120, 0x0f125) AM_WRITE(ss9601_scroll_w)
	AM_RANGE(0x0f12f, 0x0f12f) AM_WRITE(ss9601_byte_lo_w)
	AM_RANGE(0x0f140, 0x0f15f) AM_READWRITE(ss9602_r, ss9602_w)
	AM_RANGE(0x0f160, 0x0f160) AM_WRITE(ramdac_index_w)
	AM_RANGE(0x0f161, 0x0f161) AM_WRITE(ramdac_pal_w)
	AM_RANGE(0x0f162, 0x0f162) AM_WRITE(ramdac_mask_w)
	AM_RANGE(0x0f164, 0x0f164) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x0f168, 0x0f168) AM_WRITE(tilesize_w)
	AM_RANGE(0x10000, 0x180ff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x0fe00, 0x0ffff) AM_MIRROR(0xf0000) AM_RAM	/* internal RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( tbonusal_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x06000, 0x0d7ff) AM_ROM AM_REGION(REGION_USER1, 0x8100)
	AM_RANGE(0x0d800, 0x0dfff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)	/* battery */
	AM_RANGE(0x0e000, 0x0efff) AM_WRITE(videoram_mirror_w)
	AM_RANGE(0x0f11f, 0x0f11f) AM_WRITE(ss9601_disable_w)
	AM_RANGE(0x0f120, 0x0f125) AM_WRITE(ss9601_scroll_w)
	AM_RANGE(0x0f12f, 0x0f12f) AM_WRITE(ss9601_byte_lo_w)
	AM_RANGE(0x0f140, 0x0f15f) AM_READWRITE(ss9602_r, ss9602_w)
	AM_RANGE(0x0f160, 0x0f160) AM_WRITE(ramdac_index_w)
	AM_RANGE(0x0f161, 0x0f161) AM_WRITE(ramdac_pal_w)
	AM_RANGE(0x0f162, 0x0f162) AM_WRITE(ramdac_mask_w)
	AM_RANGE(0x0f166, 0x0f166) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x0f167, 0x0f167) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x0f168, 0x0f168) AM_WRITE(tilesize_w)
	AM_RANGE(0x10000, 0x180ff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x0fe00, 0x0ffff) AM_MIRROR(0xf0000) AM_RAM	/* internal RAM */
ADDRESS_MAP_END


/***************************************************************************
                                Graphics Layout
***************************************************************************/

static const gfx_layout layout_8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 16, 8, 24, 32, 48, 40, 56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8, 0, 1 },
	{ -1 } /* end of array */
};


/***************************************************************************
                                Input Ports
***************************************************************************/

INPUT_PORTS_START( mtrain )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/20 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/50 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/100 Credits" )
	PORT_DIPNAME( 0x38, 0x00, "Key Coinage" )           PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x08, "1 Key/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Key/2 Credits" )
	PORT_DIPSETTING(    0x18, "1 Key/5 Credits" )
	PORT_DIPSETTING(    0x00, "1 Key/10 Credits" )
	PORT_DIPSETTING(    0x20, "1 Key/20 Credits" )
	PORT_DIPSETTING(    0x28, "1 Key/25 Credits" )
	PORT_DIPSETTING(    0x30, "1 Key/50 Credits" )
	PORT_DIPSETTING(    0x38, "1 Key/100 Credits" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x00, "Minimum Bet" )           PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x02, "20" )
	PORT_DIPSETTING(    0x03, "40" )
	PORT_DIPNAME( 0x0c, 0x0c, "Max Bet" )               PORT_DIPLOCATION("SW2:3,4")
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "40" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x0c, "80" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Game Limit" )            PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x20, "10k" )
	PORT_DIPSETTING(    0x00, "20k" )
	PORT_DIPSETTING(    0x40, "30k" )
	PORT_DIPSETTING(    0x60, "60k" )
	PORT_DIPNAME( 0x80, 0x80, "Double Up" )             PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x07, 0x07, "Win Rate" )      PORT_DIPLOCATION("SW3:1,2,3")
	PORT_DIPSETTING(    0x07, "55%" )
	PORT_DIPSETTING(    0x06, "60%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "70%" )
	PORT_DIPSETTING(    0x03, "75%" )
	PORT_DIPSETTING(    0x00, "80%" )
	PORT_DIPSETTING(    0x02, "85%" )
	PORT_DIPSETTING(    0x01, "90%" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x07, 0x07, "Double-Up Rate" )        PORT_DIPLOCATION("SW4:1,2,3")
	PORT_DIPSETTING(    0x00, "82%" )
	PORT_DIPSETTING(    0x01, "84%" )
	PORT_DIPSETTING(    0x02, "86%" )
	PORT_DIPSETTING(    0x03, "88%" )
	PORT_DIPSETTING(    0x04, "90%" )
	PORT_DIPSETTING(    0x05, "92%" )
	PORT_DIPSETTING(    0x06, "94%" )
	PORT_DIPSETTING(    0x07, "96%" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x00, "5k" )
	PORT_DIPSETTING(    0x08, "10k" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN-A")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1      ) PORT_NAME("Start All")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1    ) PORT_NAME("Bet / Stop All")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2    ) PORT_NAME("Info / Double?")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-B")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2        ) PORT_NAME("Key In") PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1         )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2     ) PORT_NAME("Stats")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1     ) PORT_NAME("Service Mode")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3      ) PORT_NAME("Payout")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4      ) PORT_NAME("Key Out")

	PORT_START_TAG("IN-C")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5    ) PORT_CODE(KEYCODE_C) PORT_NAME("Stop 3 / Small")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON6    ) PORT_CODE(KEYCODE_X) PORT_NAME("Stop 2 / Big")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7    ) PORT_CODE(KEYCODE_Z) PORT_NAME("Stop 1 / Take")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-D")
	PORT_BIT( 0x47, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_OTHER   ) PORT_NAME("Reset") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  ) /* serial in */
INPUT_PORTS_END

INPUT_PORTS_START( strain )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )                  PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "DSW2" )                  PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )      PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "DSW3" )                  PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "DSW4" )                  PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Show Demo" )      PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN-A")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1      ) PORT_NAME("Start All")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1    ) PORT_NAME("Bet / Stop All")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2    ) PORT_NAME("Info / Double?")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-B")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2        ) PORT_NAME("Key In") PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1         )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2     ) PORT_NAME("Stats")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1     ) PORT_NAME("Service Mode")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3      ) PORT_NAME("Payout")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4      ) PORT_NAME("Key Out")

	PORT_START_TAG("IN-C")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5    ) PORT_CODE(KEYCODE_C) PORT_NAME("Stop 3")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON6    ) PORT_CODE(KEYCODE_X) PORT_NAME("Stop 2")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7    ) PORT_CODE(KEYCODE_Z) PORT_NAME("Stop 1 / Take")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-D")
	PORT_BIT( 0x47, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_OTHER   ) PORT_NAME("Reset") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  ) /* serial in */
INPUT_PORTS_END

INPUT_PORTS_START( tbonusal )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )                  PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "DSW2" )                  PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "DSW3" )                  PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "DSW4" )                  PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN-A")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1      ) PORT_NAME("Start All")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1    ) PORT_NAME("Bet / Stop All")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2    ) PORT_NAME("Info / Double?")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-B")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2        ) PORT_NAME("Key In") PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1         )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2     ) PORT_NAME("Stats")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1     ) PORT_NAME("Service Mode")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3      ) PORT_NAME("Payout")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4      ) PORT_NAME("Key Out")

	PORT_START_TAG("IN-C")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5    ) PORT_CODE(KEYCODE_C) PORT_NAME("Stop 3")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON6    ) PORT_CODE(KEYCODE_X) PORT_NAME("Stop 2")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7    ) PORT_CODE(KEYCODE_Z) PORT_NAME("Stop 1 / Take")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-D")
	PORT_BIT( 0x47, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_OTHER   ) PORT_NAME("Reset") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  ) /* serial in */
INPUT_PORTS_END

INPUT_PORTS_START( wtrnymph )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/20 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/50 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/100 Credits" )
	PORT_DIPNAME( 0x38, 0x00, "Key Coinage" )           PORT_DIPLOCATION("SW1:4,5,6")
	PORT_DIPSETTING(    0x08, "1 Key/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Key/2 Credits" )
	PORT_DIPSETTING(    0x18, "1 Key/5 Credits" )
	PORT_DIPSETTING(    0x00, "1 Key/10 Credits" )
	PORT_DIPSETTING(    0x20, "1 Key/20 Credits" )
	PORT_DIPSETTING(    0x28, "1 Key/25 Credits" )
	PORT_DIPSETTING(    0x30, "1 Key/50 Credits" )
	PORT_DIPSETTING(    0x38, "1 Key/100 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Pay Out" )               PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, "Coin" )
	PORT_DIPSETTING(    0x00, "Key" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x00, "Minimum Bet" )           PORT_DIPLOCATION("SW2:1,2")
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x02, "16" )
	PORT_DIPSETTING(    0x03, "20" )
	PORT_DIPNAME( 0x0c, 0x0c, "Max Bet" )               PORT_DIPLOCATION("SW2:3,4")
	PORT_DIPSETTING(    0x08, "10" )
	PORT_DIPSETTING(    0x04, "20" )
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPSETTING(    0x0c, "60" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Game Limit" )            PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x20, "10k" )
	PORT_DIPSETTING(    0x00, "20k" )
	PORT_DIPSETTING(    0x40, "30k" )
	PORT_DIPSETTING(    0x60, "40k" )
	PORT_DIPNAME( 0x80, 0x80, "Double Up" )             PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x07, 0x07, "Win Rate" )      PORT_DIPLOCATION("SW3:1,2,3")
	PORT_DIPSETTING(    0x07, "55%" )
	PORT_DIPSETTING(    0x06, "60%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "70%" )
	PORT_DIPSETTING(    0x03, "75%" )
	PORT_DIPSETTING(    0x00, "80%" )
	PORT_DIPSETTING(    0x02, "85%" )
	PORT_DIPSETTING(    0x01, "90%" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW3:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW4")
	PORT_DIPNAME( 0x07, 0x07, "Double-Up Rate" )        PORT_DIPLOCATION("SW4:1,2,3")
	PORT_DIPSETTING(    0x00, "82%" )
	PORT_DIPSETTING(    0x01, "84%" )
	PORT_DIPSETTING(    0x02, "88%" )
	PORT_DIPSETTING(    0x03, "90%" )
	PORT_DIPSETTING(    0x04, "92%" )
	PORT_DIPSETTING(    0x05, "94%" )
	PORT_DIPSETTING(    0x06, "96%" )
	PORT_DIPSETTING(    0x07, "98%" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x00, "5k" )
	PORT_DIPSETTING(    0x08, "10k" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN-A")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1      ) PORT_NAME("Start All")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1    ) PORT_NAME("Bet / Stop All")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2    ) PORT_NAME("Info / Double?")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN    )

	PORT_START_TAG("IN-B")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2        ) PORT_NAME("Key In") PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1         )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN       )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2     ) PORT_NAME("Stats")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1     ) PORT_NAME("Service Mode")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3      ) PORT_NAME("Payout")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4      ) PORT_NAME("Key Out")

	PORT_START_TAG("IN-C")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5    ) PORT_CODE(KEYCODE_C) PORT_NAME("Stop 3 / Right")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON6    ) PORT_CODE(KEYCODE_X) PORT_NAME("Stop 2 / Left / Play Gambling 1")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER      ) PORT_NAME("Play Gambling 2") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN    )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7    ) PORT_CODE(KEYCODE_Z) PORT_NAME("Stop 1 / Take / Rotate")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER      ) PORT_NAME("Play Tetris")     PORT_CODE(KEYCODE_T)

	PORT_START_TAG("IN-D")
	PORT_BIT( 0x47, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_OTHER   ) PORT_NAME("Reset") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  ) /* serial in */
INPUT_PORTS_END


/***************************************************************************
                                Machine Drivers
***************************************************************************/

static MACHINE_RESET( subsino2 )
{
	memset(io_data, 0, sizeof(io_data));
	memset(io_dir, 0, sizeof(io_dir));

	ramdac_index = 0;
	ramdac_int_index = 0;
}

static struct YM3812interface ym3812_interface =
{
	0
};

MACHINE_DRIVER_START( mtrain )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z180, 12000000/2)	/* unknown clock */
	MDRV_CPU_PROGRAM_MAP(mtrain_map,0)

	MDRV_FRAMES_PER_SECOND(58.7270)
	MDRV_VBLANK_DURATION(2500)	/* not accurate, the game reads the vblank state */

	MDRV_MACHINE_RESET(subsino2)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 0, 256-32-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(subsino2)
	MDRV_VIDEO_UPDATE(subsino2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 8467200/8/132)	/* probably */
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( tbonusal )
	MDRV_IMPORT_FROM(mtrain)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tbonusal_map,0)

	MDRV_SOUND_ADD(YM3812, 3000000)	/* ? chip and clock unknown */
	MDRV_SOUND_CONFIG(ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)
MACHINE_DRIVER_END


/***************************************************************************
                                ROMs
***************************************************************************/

#define HD647180X_FAKE_INTERNAL_ROM \
	ROM_FILL( 0x0000, 1, 0x3e ) \
	ROM_FILL( 0x0001, 1, 0x09 ) \
	ROM_FILL( 0x0002, 1, 0xed ) \
	ROM_FILL( 0x0003, 1, 0x39 ) \
	ROM_FILL( 0x0004, 1, 0x38 ) \
	ROM_FILL( 0x0005, 1, 0xc3 ) \
	ROM_FILL( 0x0006, 1, 0x16 ) \
	ROM_FILL( 0x0007, 1, 0xf0 )

ROM_START( mtrain )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	HD647180X_FAKE_INTERNAL_ROM

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "v1.4_27c512.u17", 0x00000, 0x10000, CRC(38d5340d) SHA1(6c32b7cd42e2ad0ad56ac308d008d649ebb10684) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "v1.4_27c2001.u2", 0x00000, 0x40000, CRC(b7e65d04) SHA1(5eea1b8c1129963b3b83a59410cd0e1de70621e4) )
	ROM_LOAD32_BYTE( "v1.4_27c2001.u3", 0x00002, 0x40000, CRC(cef2c079) SHA1(9ee54a08ef8db90a80a4b3568bb82ce09ee41e65) )
	ROM_LOAD32_BYTE( "v1.4_27c2001.u4", 0x00001, 0x40000, CRC(a794f287) SHA1(7b9c0d57224a700f49e55ba5aeb7ed9d35a71e02) )
	ROM_LOAD32_BYTE( "v1.4_27c2001.u5", 0x00003, 0x40000, CRC(96067e95) SHA1(bec7dffaf6920ff2bd85a43fb001a997583e25ee) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "v1.4_27c2001.u27", 0x00000, 0x40000, CRC(51cae476) SHA1(d1da4e5c3d53d18d8b69dfb57796d0ae311d99bf) )
	ROM_RELOAD(                   0x40000, 0x40000 )

	ROM_REGION( 0x28, REGION_USER2, 0 )
	ROM_LOAD( "ds2430a.bin", 0x00, 0x28, BAD_DUMP CRC(a73211f7) SHA1(ebe175b9b8ea3fffcc9dd03ea51ccef36b016eb8) )
ROM_END

ROM_START( mtraina )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	HD647180X_FAKE_INTERNAL_ROM

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "out_1v131.u17", 0x00000, 0x10000, CRC(6761be7f) SHA1(a492f8179d461a454516dde33ff04473d4cfbb27) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "rom_4.u02", 0x00000, 0x40000, CRC(b7e65d04) SHA1(5eea1b8c1129963b3b83a59410cd0e1de70621e4) )
	ROM_LOAD32_BYTE( "rom_3.u03", 0x00002, 0x40000, CRC(cef2c079) SHA1(9ee54a08ef8db90a80a4b3568bb82ce09ee41e65) )
	ROM_LOAD32_BYTE( "rom_2.u04", 0x00001, 0x40000, CRC(a794f287) SHA1(7b9c0d57224a700f49e55ba5aeb7ed9d35a71e02) )
	ROM_LOAD32_BYTE( "rom_1.u05", 0x00003, 0x40000, CRC(96067e95) SHA1(bec7dffaf6920ff2bd85a43fb001a997583e25ee) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom_5.u27", 0x00000, 0x40000, CRC(51cae476) SHA1(d1da4e5c3d53d18d8b69dfb57796d0ae311d99bf) )
	ROM_RELOAD(            0x40000, 0x40000 )

	ROM_REGION( 0x28, REGION_USER2, 0 )
	ROM_LOAD( "ds2430a.bin", 0x00, 0x28, BAD_DUMP CRC(a73211f7) SHA1(ebe175b9b8ea3fffcc9dd03ea51ccef36b016eb8) )
ROM_END

ROM_START( strain )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	HD647180X_FAKE_INTERNAL_ROM

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "v1.9_27c512_u17.bin", 0x00000, 0x10000, CRC(36379ab2) SHA1(b48374f80ffa107a7ea3e08eb432259e443dc4a6) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "v1.0_mx27c4000_u2.bin", 0x00000, 0x80000, CRC(0b77b3be) SHA1(daf1180cabce3e1bbb9a8f91c02e0fe4f0fd811e) )
	ROM_LOAD32_BYTE( "v1.0_mx27c4000_u3.bin", 0x00002, 0x80000, CRC(c003661d) SHA1(49d76a9273928c35dcd6a6ab114d798f5553d79a) )
	ROM_LOAD32_BYTE( "v1.0_mx27c4000_u4.bin", 0x00001, 0x80000, CRC(6392f562) SHA1(83881ec85a3dff82f32214b2654ee79e5e9a2d2a) )
	ROM_LOAD32_BYTE( "v1.0_mx27c4000_u5.bin", 0x00003, 0x80000, CRC(85abe66c) SHA1(32698faf75bd0c42ab99b0c53b3ffa0891eedaca) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "v1.0_mx27c4000_u27.bin", 0x00000, 0x80000, CRC(d5bbebc7) SHA1(59fa804caa991dc2ad7f735b7c171defd836140a) )

	ROM_REGION( 0x28, REGION_USER2, 0 )
	ROM_LOAD( "ds2430a.bin", 0x00, 0x28, BAD_DUMP CRC(133705eb) SHA1(974b7fd5f7eaa84c4ba2a5ba9e014ac459fa7d23) )
ROM_END

ROM_START( tbonusal )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	HD647180X_FAKE_INTERNAL_ROM

	ROM_REGION( 0x1a000, REGION_USER1, 0 )
	ROM_LOAD( "n-alpha 1.6-u17.bin", 0x00000, 0x10000, CRC(1bdc1c92) SHA1(2cd7ec5a89865b76df2cfe9d18b2ab42923f8def) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "n-alpha 1.6-u2.bin", 0x000000, 0x80000, CRC(392943be) SHA1(776d36a94b8a70ec2eaf88dfd2804517656b53a0) )
	ROM_LOAD32_BYTE( "n-alpha 1.6-u3.bin", 0x000002, 0x80000, CRC(24c8f62e) SHA1(692a96f233d8576a6921bfe23c97502ff26c62db) )
	ROM_LOAD32_BYTE( "n-alpha 1.6-u4.bin", 0x000001, 0x80000, CRC(bed035a9) SHA1(6b141bb8fb7969338faa702bd03970331bbbe6e1) )
	ROM_LOAD32_BYTE( "n-alpha 1.6-u5.bin", 0x000003, 0x80000, CRC(d00d48c6) SHA1(28b505a3f07c5d5bb8e8609c6d6e883260594588) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_ERASE00 )

	ROM_REGION( 0x28, REGION_USER2, 0 )
	ROM_LOAD( "ds2430a.bin", 0x00, 0x28, BAD_DUMP CRC(7c832409) SHA1(fe16074490fe4edab2be2de5fa83941dac9969b0) )
ROM_END

ROM_START( wtrnymph )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	HD647180X_FAKE_INTERNAL_ROM

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "ocean-n tetris_1 v1.4.u17", 0x00000, 0x10000, CRC(c7499123) SHA1(39a9ea6d927ee839cfb127747e5e3df3535af098) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "ocean-n tetris_2 v1.21.u2", 0x00000, 0x40000, CRC(813aac90) SHA1(4555adf8dc363359b10f1d5cfae2dcebed411679) )
	ROM_LOAD32_BYTE( "ocean-n tetris_3 v1.21.u3", 0x00002, 0x40000, CRC(83c39379) SHA1(e7f9315d19370c18b664b759e433052a88f8c146) )
	ROM_LOAD32_BYTE( "ocean-n tetris_4 v1.21.u4", 0x00001, 0x40000, CRC(6fc64b42) SHA1(80110d7dae28cca5e39c8a7c2ceebf589116ae23) )
	ROM_LOAD32_BYTE( "ocean-n tetris_5 v1.21.u5", 0x00003, 0x40000, CRC(8c7515ee) SHA1(a67b21c1e8ca8a098fe558c73561bca13962893e) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "ocean-n tetris_6 v1.21.u27", 0x00000, 0x40000, CRC(1c8a886d) SHA1(faa983801b368a6d04ef80e359c6fb67b240c60d) )
	ROM_RELOAD(                             0x40000, 0x40000 )

	ROM_REGION( 0x28, REGION_USER2, 0 )
	ROM_LOAD( "ds2430a.bin", 0x00, 0x28, BAD_DUMP CRC(1b585f27) SHA1(ee89dfc731d867507c15009910e9f743c652a399) )
ROM_END

ROM_START( blushark )
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	HD647180X_FAKE_INTERNAL_ROM

	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "1 v1.0.u17", 0x00000, 0x10000, CRC(e659142c) SHA1(f1608f7164a624ea48d4579715959af15ae75c28) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "2_v1.u2", 0x00000, 0x40000, CRC(e720ac2b) SHA1(4b2b89075a848e24fb16f98f0da2f18606ae8e10) )
	ROM_LOAD32_BYTE( "3_v1.u3", 0x00002, 0x40000, CRC(4c5c4ef8) SHA1(62076c27129af3649e244d8095056fddcfeaddfc) )
	ROM_LOAD32_BYTE( "4_v1.u4", 0x00001, 0x40000, CRC(d3367795) SHA1(7ca4f3c2472c70b17a5929d492c8bc54fae27950) )
	ROM_LOAD32_BYTE( "5_v1.u5", 0x00003, 0x40000, CRC(d0806709) SHA1(feb98c5892050e57eac35126b6e573883c23a335) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "6_v1.u27", 0x00000, 0x40000, CRC(943ac197) SHA1(4880aca75a135d1932eb8c4de05013b0b60069e9) )
	ROM_RELOAD(           0x40000, 0x40000 )

	ROM_REGION( 0x28, REGION_USER2, 0 )
	ROM_LOAD( "ds1971.q3", 0x00, 0x28, CRC(c59a888e) SHA1(780b31bb06be6d4ce6ec3e99d871077c3e081315) )
ROM_END


/***************************************************************************
                                Init
***************************************************************************/

static void init_subsino2(void (*bitswaps)(UINT8 *decrypt, int i), const UINT8 *xors)
{
	subsino_decrypt(memory_region(REGION_USER1) + 0x8100, bitswaps, xors, 0x7f00);
	/* the game bitbangs the 1-Wire bus with timing loops, the scale makes the pulses
	   measured with the Z180 core timings fit the 1-Wire protocol */
	ds2430a_init(0.38, memory_region(REGION_USER2));
}

static DRIVER_INIT( mtrain )
{
	init_subsino2(crsbingo_bitswaps, crsbingo_xors);
}

static DRIVER_INIT( tbonusal )
{
	init_subsino2(sharkpy_bitswaps, sharkpy_xors);
}

static DRIVER_INIT( wtrnymph )
{
	init_subsino2(victor5_bitswaps, victor5_xors);
}

GAME( 1997, mtrain,   0,        mtrain,   mtrain,   mtrain,   ROT0, "Subsino",                          "Magic Train (Ver. 1.4)", 0 )
GAME( 1996, mtraina,  mtrain,   mtrain,   mtrain,   mtrain,   ROT0, "Subsino",                          "Magic Train (Ver. 1.31)", 0 )
GAME( 1996, strain,   0,        mtrain,   strain,   mtrain,   ROT0, "Subsino",                          "Super Train (Ver. 1.9)", GAME_NOT_WORKING )
GAME( 1995, tbonusal, 0,        tbonusal, tbonusal, tbonusal, ROT0, "Subsino (American Alpha license)", "Treasure Bonus (American Alpha, Ver. 1.6)", GAME_NOT_WORKING )
GAME( 1996, wtrnymph, 0,        mtrain,   wtrnymph, wtrnymph, ROT0, "Subsino",                          "Water-Nymph (Ver. 1.4)", 0 )
GAME( 199?, blushark, wtrnymph, mtrain,   wtrnymph, wtrnymph, ROT0, "hack?",                            "Blue Shark (Subsino, Ver. 1.0)", GAME_NOT_WORKING )
