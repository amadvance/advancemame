// license:BSD-3-Clause
// copyright-holders:Richard Davies
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "phoenix.h"
#include "cpu/i8085/i8085.h"

static UINT8 *videoram_pg1;
static UINT8 *videoram_pg2;
static UINT8 *current_videoram_pg;
static int current_videoram_pg_index;
static int palette_bank;
static int cocktail_mode;
static int pleiads_protection_question;
static UINT8 survival_protection_value;
static UINT8 survival_input_latches[2];
static UINT8 survival_input_readc;
static tilemap *fg_tilemap, *bg_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Phoenix has two 256x4 palette PROMs, one containing the high bits and the
  other the low bits (2x2x2 color space).
  The palette PROMs are connected to the RGB output this way:

  bit 3 --
        -- 270 ohm resistor  -- GREEN
        -- 270 ohm resistor  -- BLUE
  bit 0 -- 270 ohm resistor  -- RED

  bit 3 --
        -- GREEN
        -- BLUE
  bit 0 -- RED

  plus 270 ohm pullup and pulldown resistors on all lines

***************************************************************************/

PALETTE_INIT( phoenix )
{
	int i;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		r = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		g = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		b = 0x55 * bit0 + 0xaa * bit1;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* first bank of characters use colors 0x00-0x1f and 0x40-0x5f */
	/* second bank of characters use colors 0x20-0x3f and 0x60-0x7f */
	for (i = 0;i < 0x40;i++)
	{
		int col;


		col = ((i & 0x1c) >> 2) | ((i & 0x03) << 3) | ((i & 0x20) << 1);

		COLOR(0,i) = col;
		COLOR(1,i) = col | 0x20;
	}
}

PALETTE_INIT( survival )
{
	int i,bit,channel;
	int levels[4];
	int rgb[256][3];
	int ymin = 255000, ymax = 0;
	static const int shifts[3] = { 0, 2, 1 };
	static const double resistors[2] = { 180.0, 1.0 };

	palette_init_phoenix(colortable,color_prom);
	for (i = 0; i < 4; i++)
	{
		double conductance = 1.0 / 100.0 + 1.0 / 270.0;
		double voltage = 5.0 / 100.0;
		for (bit = 0; bit < 2; bit++)
			if (!(i & (1 << bit)))
			{
				conductance += 1.0 / resistors[bit];
				voltage += 0.05 / resistors[bit];
			}
		levels[i] = voltage / conductance * 255.0 / 5.0 + 0.4;
	}
	for (i = 0; i < 256; i++)
	{
		int y;
		for (channel = 0; channel < 3; channel++)
		{
			int value = ((color_prom[i] >> shifts[channel]) & 1) |
				(((color_prom[i + 256] >> shifts[channel]) & 1) << 1);
			rgb[i][channel] = levels[value];
		}
		y = 299 * rgb[i][0] + 587 * rgb[i][1] + 114 * rgb[i][2];
		if (y < ymin) ymin = y;
		if (y > ymax) ymax = y;
	}
	for (i = 0; i < 256; i++)
	{
		int y = 299 * rgb[i][0] + 587 * rgb[i][1] + 114 * rgb[i][2];
		int u = (rgb[i][2] - y / 1000) * 492 / 1000;
		int v = (rgb[i][0] - y / 1000) * 877 / 1000;
		int target = (y - ymin) * 256 / (ymax - ymin);
		int r = target + 1140 * v / 1000;
		int g = target - 395 * u / 1000 - 581 * v / 1000;
		int b = target + 2032 * u / 1000;
		palette_set_color(i,r < 0 ? 0 : r > 255 ? 255 : r,
			g < 0 ? 0 : g > 255 ? 255 : g,b < 0 ? 0 : b > 255 ? 255 : b);
	}
}

PALETTE_INIT( pleiads )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		r = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		g = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		b = 0x55 * bit0 + 0xaa * bit1;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* first bank of characters use colors 0x00-0x1f, 0x40-0x5f, 0x80-0x9f and 0xc0-0xdf */
	/* second bank of characters use colors 0x20-0x3f, 0x60-0x7f, 0xa0-0xbf and 0xe0-0xff */
	for (i = 0;i < 0x80;i++)
	{
		int col;


		col = ((i & 0x1c) >> 2) | ((i & 0x03) << 3) | ((i & 0x60) << 1);

		COLOR(0,i) = col;
		COLOR(1,i) = col | 0x20;
	}
}

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code;

	code = current_videoram_pg[tile_index];
	SET_TILE_INFO(
			1,
			code,
			(code >> 5) | (palette_bank << 3),
			(tile_index & 0x1f) ? 0 : TILE_IGNORE_TRANSPARENCY)	/* first row (column) is opaque */
}

static void get_bg_tile_info(int tile_index)
{
	int code;

	code = current_videoram_pg[tile_index + 0x800];
	SET_TILE_INFO(
			0,
			code,
			(code >> 5) | (palette_bank << 3),
			0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( phoenix )
{
	videoram_pg1 = auto_malloc(0x1000);
	videoram_pg2 = auto_malloc(0x1000);

	memory_configure_bank(1, 0, 1, videoram_pg1, 0);
	memory_configure_bank(1, 1, 1, videoram_pg2, 0);
	memory_set_bank(1, 0);

    current_videoram_pg_index = -1;
	current_videoram_pg = videoram_pg1;		/* otherwise, hiscore loading crashes */


	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,32,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_scrolldx(fg_tilemap,0,8);
	tilemap_set_scrolldy(fg_tilemap,0,48);
	tilemap_set_scrolldx(bg_tilemap,0,8);
	tilemap_set_scrolldy(bg_tilemap,0,48);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_HANDLER( phoenix_videoram_w )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	current_videoram_pg[offset] = data;

	if ((offset & 0x7ff) < 0x340)
	{
		if (offset & 0x800)
			tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
		else
			tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
	}

	/* as part of the protecion, Survival executes code from $43a4 */
	rom[offset + 0x4000] = data;
}


WRITE8_HANDLER( phoenix_videoreg_w )
{
    if (current_videoram_pg_index != (data & 1))
	{
		/* set memory bank */
		current_videoram_pg_index = data & 1;
		memory_set_bank(1, current_videoram_pg_index);
		current_videoram_pg = current_videoram_pg_index ? videoram_pg2 : videoram_pg1;

		cocktail_mode = current_videoram_pg_index && (input_port_3_r(0) & 0x01);

		tilemap_set_flip(ALL_TILEMAPS, cocktail_mode ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* Phoenix has only one palette select effecting both layers */
	if (palette_bank != ((data >> 1) & 1))
	{
		palette_bank = (data >> 1) & 1;

		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE8_HANDLER( pleiads_videoreg_w )
{
    if (current_videoram_pg_index != (data & 1))
	{
		/* set memory bank */
		current_videoram_pg_index = data & 1;
		current_videoram_pg = current_videoram_pg_index ? videoram_pg2 : videoram_pg1;

		cocktail_mode = current_videoram_pg_index && (input_port_3_r(0) & 0x01);

		tilemap_set_flip(ALL_TILEMAPS, cocktail_mode ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}


	/* the palette table is at $0420-$042f and is set by $06bc.
       Four palette changes by level.  The palette selection is
       wrong, but the same paletter is used for both layers. */

    if (palette_bank != ((data >> 1) & 3))
	{
		palette_bank = ((data >> 1) & 3);

		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

		logerror("Palette: %02X\n", (data & 0x06) >> 1);
	}

	pleiads_protection_question = data & 0xfc;

	/* send two bits to sound control C (not sure if they are there) */
	pleiads_sound_control_c_w(offset, data);
}


WRITE8_HANDLER( phoenix_scroll_w )
{
	tilemap_set_scrollx(bg_tilemap,0,data);
}


READ8_HANDLER( phoenix_input_port_0_r )
{
	if (cocktail_mode)
		return (input_port_0_r(0) & 0x07) | (input_port_1_r(0) & 0xf8);
	else
		return input_port_0_r(0);
}

READ8_HANDLER( pleiads_input_port_0_r )
{
	int ret = phoenix_input_port_0_r(0) & 0xf7;

	/* handle Pleiads protection */
	switch (pleiads_protection_question)
	{
	case 0x00:
	case 0x20:
		/* Bit 3 is 0 */
		break;
	case 0x0c:
	case 0x30:
		/* Bit 3 is 1 */
		ret	|= 0x08;
		break;
	default:
		logerror("Unknown protection question %02X at %04X\n", pleiads_protection_question, activecpu_get_pc());
	}

	return ret;
}

READ8_HANDLER( survival_input_port_0_r )
{
	UINT8 ret;
	int survival_sid_value = 0;

	if (cocktail_mode)
		ret = ~input_port_1_r(0);
	else
		ret = ~input_port_0_r(0);

	if (survival_input_readc++ == 2)
	{
		survival_input_readc = 0;
		survival_protection_value = 0;
		return ~ret;
	}

	survival_protection_value = 0xff;

	switch ((ret >> 4) & 0x0f)
	{
		case 0:
			ret = (ret & 0x0f) | 0x70;
			break;
		case 1:
			ret = (ret & 0x0f) | 0x80;
			break;
		case 2:
			survival_sid_value = 0x80;
			survival_protection_value = 0xfe;
			ret = (ret & 0x0f) | 0x20;
			break;
		case 3:
			survival_sid_value = 0x80;
			ret = (ret & 0x0f) | 0xa0;
			break;
		case 4:
			survival_sid_value = 0x80;
			survival_protection_value = 0xfe;
			ret = (ret & 0x0f) | 0x40;
			break;
		case 5:
			survival_sid_value = 0x80;
			ret = (ret & 0x0f) | 0xc0;
			break;
		case 8:
			ret = (ret & 0x0f) | 0x10;
			break;
		case 0xa:
			survival_sid_value = 0x80;
			ret = (ret & 0x0f) | 0x30;
			break;
		case 0xc:
			survival_sid_value = 0x80;
			ret = (ret & 0x0f) | 0x50;
			break;
		default:
			break;
	}

	activecpu_set_info_int(CPUINFO_INT_I8085_SID,survival_sid_value);
	survival_input_latches[0] = survival_input_latches[1];
	survival_input_latches[1] = ~ret;

	return survival_input_latches[0];
}

READ8_HANDLER( survival_protection_r )
{
	return survival_protection_value;
}

MACHINE_RESET( survival )
{
	survival_protection_value = 0;
	survival_input_readc = 0;
	survival_input_latches[0] = survival_input_latches[1] = 0;
	cpunum_set_info_int(0,CPUINFO_INT_I8085_SID,0);
}

/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( phoenix )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
