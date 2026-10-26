// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

	vidhrdw/pc8801.c

	NEC PC-8801 series video hardware emulation

***************************************************************************/

#include "driver.h"
#include "includes/pc8801.h"

#define screen_width (((pc8801_crtc.param[0][0] & 0x7f) + 2) * 8)
#define blink_speed ((((pc8801_crtc.param[0][1] & 0xc0) >> 6) + 1) << 3)
#define screen_height ((pc8801_crtc.param[0][1] & 0x3f) + 1)
#define lines_per_char ((pc8801_crtc.param[0][2] & 0x1f) + 1)
#define vretrace ((((pc8801_crtc.param[0][3] & 0xe0) >> 5) + 1))
#define hretrace (((pc8801_crtc.param[0][3] & 0x1f) + 2) * 8)
#define text_color_flag ((pc8801_crtc.param[0][4] & 0xe0) == 0x40)

int pc8801_pixel_clock(void)
{
	return pc8801_is_24KHz;
}

static UINT8 calc_cursor_pos(int x, int y, int yi)
{
	if (!pc8801_crtc.cursor_on)
		return 0;

	if (x == pc8801_crtc.param[4][0] && y == pc8801_crtc.param[4][1])
	{
		if (!(pc8801_crtc.param[0][2] & 0x40) && yi != 7)
			return 0;

		if (!(pc8801_crtc.param[0][2] & 0x20))
			return 1;

		if (((cpu_getcurrentframe() / blink_speed) & 1) == 0)
			return 1;

		return 0;
	}

	return 0;
}

static UINT8 extract_text_attribute(UINT32 address, int x, UINT8 width, UINT8 *non_special)
{
	UINT8 *vram = pc8801_wram;
	int i;
	int fifo_size;
	int offset;

	*non_special = 0;
	if (pc8801_crtc.param[0][4] & 0x80)
		return 0;

	fifo_size = (pc8801_crtc.param[0][4] & 0x20) ? 0 : ((pc8801_crtc.param[0][4] & 0x1f) + 1);

	if (fifo_size == 0)
	{
		*non_special = 1;
		return text_color_flag ? 0xe8 : 0;
	}

	offset = (vram[address & 0xffff] == 0) ? 2 : 0;

	for (i = 0; i < fifo_size; i++)
	{
		if (x < vram[(address + offset) & 0xffff])
		{
			return vram[(address + 1) & 0xffff];
		}
		else
		{
			address += 2;
		}
	}

	return vram[(address - 3 + offset) & 0xffff];
}

static void pc8801_draw_char(mame_bitmap *bitmap, int x, int y, int pal, UINT8 gfx_mode, UINT8 reverse, UINT8 secret,
                             UINT8 blink, UINT8 upper, UINT8 lower, int y_size, int width, UINT8 non_special)
{
	int xi, yi;
	UINT8 *vram = pc8801_wram;
	UINT8 *gfx_rom = pc8801_gfx_rom;
	UINT8 is_cursor;
	UINT8 y_height, y_double;
	UINT8 y_step;

	y_height = lines_per_char;
	y_double = pc8801_pixel_clock();
	y_step = non_special ? 80 : 120;
	is_cursor = 0;

	for (yi = 0; yi < y_height; yi++)
	{
		if (pc8801_gfx_ctrl & 1)
			is_cursor = calc_cursor_pos(x, y, yi);

		for (xi = 0; xi < 8; xi++)
		{
			int res_x, res_y;
			int tile;
			int color;

			tile = vram[(x + (y * y_step) + pc8801_dma_address[2]) & 0xffff];
			res_x = x * 8 + xi * (width + 1);
			res_y = y * y_height + yi;

			if (res_x < Machine->visible_area.min_x || res_x > Machine->visible_area.max_x ||
			    res_y < Machine->visible_area.min_y || res_y > Machine->visible_area.max_y)
				continue;

			if (gfx_mode)
			{
				UINT8 mask;

				mask = (xi & 4) ? 0x10 : 0x01;
				mask <<= ((yi & (0x6 << y_double)) >> (1 + y_double));
				color = (tile & mask) ? pal : -1;
			}
			else
			{
				UINT8 char_data;
				UINT8 blink_mask = 0;

				if (blink && ((cpu_getcurrentframe() / blink_speed) & 3) == 1)
					blink_mask = 1;

				if (yi >= (1 << (y_double + 3)) || secret || blink_mask)
					char_data = 0;
				else
					char_data = (gfx_rom[(tile * 8 + (yi >> y_double)) & 0x7ff] >> (7 - xi)) & 1;

				if (yi == 0 && upper)
					char_data = 1;
				if (yi == y_height && lower)
					char_data = 1;

				if (is_cursor)
					char_data ^= 1;
				if (reverse)
					char_data ^= 1;

				color = char_data ? pal : -1;
			}

			if (color != -1)
			{
				plot_pixel(bitmap, res_x, res_y, Machine->pens[color]);
				if (width && (res_x + 1 <= Machine->visible_area.max_x))
				{
					plot_pixel(bitmap, res_x + 1, res_y, Machine->pens[color]);
				}
			}
		}
	}
}

static void pc8801_draw_text(mame_bitmap *bitmap, int y_size, UINT8 width)
{
	int x, y;
	UINT8 attr;
	UINT8 reverse, gfx_mode, secret, upper, lower, blink;
	int pal;
	UINT8 non_special;

	for (y = 0; y < y_size; y++)
	{
		for (x = 0; x < 80; x++)
		{
			if ((x & 1) && !width)
				continue;

			attr = extract_text_attribute((((y * 120) + 80 + pc8801_dma_address[2]) & 0xffff), x, width, &non_special);

			if (text_color_flag && (attr & 8))
			{
				pal = (attr & 0xe0) >> 5;
				gfx_mode = (attr & 0x10) >> 4;
				reverse = 0;
				secret = 0;
				upper = 0;
				lower = 0;
				blink = 0;
				pal |= 8;
			}
			else
			{
				pal = 7;
				gfx_mode = (attr & 0x80) >> 7;
				reverse = (attr & 4) >> 2;
				secret = (attr & 1);
				upper = (attr & 0x10) >> 4;
				lower = (attr & 0x20) >> 5;
				blink = (attr & 2) >> 1;
				pal |= 8;
				reverse ^= pc8801_crtc.inverse;
			}

			pc8801_draw_char(bitmap, x, y, pal, gfx_mode, reverse, secret, blink, upper, lower, y_size, !width, non_special);
		}
	}
}

static void draw_bitmap_3bpp(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int x, y, xi;
	UINT32 count = 0;
	UINT8 *gvram = pc8801_gvram;
	UINT16 y_size;
	UINT16 y_double;

	y_double = pc8801_pixel_clock();
	y_size = (y_double + 1) * 200;

	for (y = 0; y < y_size; y += (y_double + 1))
	{
		for (x = 0; x < 640; x += 8)
		{
			for (xi = 0; xi < 8; xi++)
			{
				int pen = 0;

				pen |= ((gvram[count + 0x0000] >> (7 - xi)) & 1) << 0;
				pen |= ((gvram[count + 0x4000] >> (7 - xi)) & 1) << 1;
				pen |= ((gvram[count + 0x8000] >> (7 - xi)) & 1) << 2;

				if (y_double)
				{
					if (x + xi >= cliprect->min_x && x + xi <= cliprect->max_x)
					{
						if (y + 0 >= cliprect->min_y && y + 0 <= cliprect->max_y)
							plot_pixel(bitmap, x + xi, y + 0, Machine->pens[pen & 7]);
						if (y + 1 >= cliprect->min_y && y + 1 <= cliprect->max_y)
							plot_pixel(bitmap, x + xi, y + 1, Machine->pens[pen & 7]);
					}
				}
				else
				{
					if (x + xi >= cliprect->min_x && x + xi <= cliprect->max_x &&
					    y >= cliprect->min_y && y <= cliprect->max_y)
					{
						plot_pixel(bitmap, x + xi, y, Machine->pens[pen & 7]);
					}
				}
			}
			count++;
		}
	}
}

static void draw_bitmap_1bpp(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int x, y, xi;
	UINT32 count = 0;
	UINT8 *gvram = pc8801_gvram;
	UINT8 color = (pc8801_gfx_ctrl & 1) ? (7 & ((pc8801_layer_mask ^ 0xe) >> 1)) : 7;
	UINT8 is_cursor = 0;

	for (y = 0; y < 200; y++)
	{
		for (x = 0; x < 640; x += 8)
		{
			if (!(pc8801_gfx_ctrl & 1))
				is_cursor = calc_cursor_pos(x / 8, y / lines_per_char, y & (lines_per_char - 1));

			for (xi = 0; xi < 8; xi++)
			{
				int pen = (gvram[count + 0x0000] >> (7 - xi)) & 1;
				if (is_cursor)
					pen ^= 1;

				if (pc8801_gfx_ctrl & 1)
				{
					if (x + xi >= cliprect->min_x && x + xi <= cliprect->max_x)
					{
						if (y * 2 + 0 >= cliprect->min_y && y * 2 + 0 <= cliprect->max_y)
							plot_pixel(bitmap, x + xi, y * 2 + 0, Machine->pens[pen ? color : 0]);
						if (y * 2 + 1 >= cliprect->min_y && y * 2 + 1 <= cliprect->max_y)
							plot_pixel(bitmap, x + xi, y * 2 + 1, Machine->pens[pen ? color : 0]);
					}
				}
				else
				{
					if (x + xi >= cliprect->min_x && x + xi <= cliprect->max_x &&
					    y >= cliprect->min_y && y <= cliprect->max_y)
					{
						plot_pixel(bitmap, x + xi, y, Machine->pens[pen ? color : 0]);
					}
				}
			}
			count++;
		}
	}

	if (!(pc8801_gfx_ctrl & 1))
	{
		count = 0;
		for (y = 200; y < 400; y++)
		{
			for (x = 0; x < 640; x += 8)
			{
				is_cursor = calc_cursor_pos(x / 8, y / lines_per_char, y & (lines_per_char - 1));

				for (xi = 0; xi < 8; xi++)
				{
					int pen = (gvram[count + 0x4000] >> (7 - xi)) & 1;
					if (is_cursor)
						pen ^= 1;

					if (x + xi >= cliprect->min_x && x + xi <= cliprect->max_x &&
					    y >= cliprect->min_y && y <= cliprect->max_y)
					{
						plot_pixel(bitmap, x + xi, y, Machine->pens[pen ? 7 : 0]);
					}
				}
				count++;
			}
		}
	}
}

PALETTE_INIT( pc8801 )
{
	int i;
	for (i = 0; i < 0x10; i++)
		palette_set_color(i, (i & 2) ? 0xff : 0x00, (i & 4) ? 0xff : 0x00, (i & 1) ? 0xff : 0x00);
}

VIDEO_START( pc8801 )
{
	return 0;
}

VIDEO_UPDATE( pc8801 )
{
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	if (pc8801_gfx_ctrl & 8)
	{
		if (pc8801_gfx_ctrl & 0x10)
			draw_bitmap_3bpp(bitmap, cliprect);
		else
			draw_bitmap_1bpp(bitmap, cliprect);
	}


	if (!(pc8801_layer_mask & 1) && (pc8801_dmac_mode & 4) && (pc8801_crtc.status & 0x10) && (pc8801_crtc.irq_mask == 3))
	{
		pc8801_draw_text(bitmap, screen_height, pc8801_txt_width);
	}
}

READ8_HANDLER( pc8801_alu_r )
{
	int i;
	UINT8 *gvram = pc8801_gvram;
	UINT8 b, r, g;

	for (i = 0; i < 3; i++)
		pc8801_alu_reg[i] = gvram[i * 0x4000 + offset];

	b = gvram[offset + 0x0000];
	r = gvram[offset + 0x4000];
	g = gvram[offset + 0x8000];
	if (!(pc8801_alu_ctrl2 & 1)) { b ^= 0xff; }
	if (!(pc8801_alu_ctrl2 & 2)) { r ^= 0xff; }
	if (!(pc8801_alu_ctrl2 & 4)) { g ^= 0xff; }

	return b & r & g;
}

WRITE8_HANDLER( pc8801_alu_w )
{
	int i;
	UINT8 *gvram = pc8801_gvram;

	switch (pc8801_alu_ctrl2 & 0x30)
	{
		case 0x00:
		{
			UINT8 logic_op;
			for (i = 0; i < 3; i++)
			{
				logic_op = (pc8801_alu_ctrl1 & (0x11 << i)) >> i;
				switch (logic_op)
				{
					case 0x00: gvram[i * 0x4000 + offset] &= ~data; break;
					case 0x01: gvram[i * 0x4000 + offset] |= data; break;
					case 0x10: gvram[i * 0x4000 + offset] ^= data; break;
					case 0x11: break;
				}
			}
			break;
		}

		case 0x10:
			for (i = 0; i < 3; i++)
				gvram[i * 0x4000 + offset] = pc8801_alu_reg[i];
			break;

		case 0x20:
			gvram[0x0000 + offset] = pc8801_alu_reg[1];
			break;

		case 0x30:
			gvram[0x4000 + offset] = pc8801_alu_reg[0];
			break;
	}
}

WRITE8_HANDLER( pc8801_alu_ctrl1_w )
{
	pc8801_alu_ctrl1 = data;
}

WRITE8_HANDLER( pc8801_alu_ctrl2_w )
{
	pc8801_alu_ctrl2 = data;
}

READ8_HANDLER( pc8801_vram_select_r )
{
	return 0xf8 | ((pc8801_vram_sel == 3) ? 0 : (1 << pc8801_vram_sel));
}

WRITE8_HANDLER( pc8801_vram_select_w )
{
	pc8801_vram_sel = offset & 3;
}

WRITE8_HANDLER( pc8801_bgpal_w )
{
}

WRITE8_HANDLER( pc8801_palram_w )
{
	offset &= 7;
	if (pc8801_misc_ctrl & 0x20)
	{
		if ((data & 0x40) == 0)
		{
			pc8801_palram[offset].b = data & 0x7;
			pc8801_palram[offset].r = (data & 0x38) >> 3;
		}
		else
		{
			pc8801_palram[offset].g = data & 0x7;
		}
	}
	else
	{
		pc8801_palram[offset].b = (data & 1) ? 7 : 0;
		pc8801_palram[offset].r = (data & 2) ? 7 : 0;
		pc8801_palram[offset].g = (data & 4) ? 7 : 0;
	}

	palette_set_color(offset,
		(pc8801_palram[offset].r << 5) | (pc8801_palram[offset].r << 2) | (pc8801_palram[offset].r >> 1),
		(pc8801_palram[offset].g << 5) | (pc8801_palram[offset].g << 2) | (pc8801_palram[offset].g >> 1),
		(pc8801_palram[offset].b << 5) | (pc8801_palram[offset].b << 2) | (pc8801_palram[offset].b >> 1)
	);
}

WRITE8_HANDLER( pc8801_layer_masking_w )
{
	pc8801_layer_mask = data;
}

READ8_HANDLER( pc8801_crtc_param_r )
{
	return 0xff;
}

WRITE8_HANDLER( pc88_crtc_param_w )
{
	if (pc8801_crtc.param_count < 5)
	{
		pc8801_crtc.param[pc8801_crtc.cmd][pc8801_crtc.param_count] = data;
		pc8801_crtc.param_count++;
	}
}

READ8_HANDLER( pc8801_crtc_status_r )
{
	return pc8801_crtc.status;
}

WRITE8_HANDLER( pc88_crtc_cmd_w )
{
	pc8801_crtc.cmd = (data & 0xe0) >> 5;
	pc8801_crtc.param_count = 0;

	switch (pc8801_crtc.cmd)
	{
		case 0:
			pc8801_crtc.status &= (~0x16);
			break;
		case 1:
			pc8801_crtc.status |= 0x10;
			pc8801_crtc.status &= (~0x08);
			pc8801_crtc.inverse = data & 1;
			break;
		case 2:
			pc8801_crtc.irq_mask = data & 3;
			break;
		case 3:
			pc8801_crtc.status &= (~0x01);
			break;
		case 4:
			pc8801_crtc.cursor_on = data & 1;
			break;
		case 5:
		case 6:
			pc8801_crtc.status &= (~0x06);
			break;
	}
}

READ8_HANDLER( pc8801_dmac_r )
{
	return 0xff;
}

WRITE8_HANDLER( pc8801_dmac_w )
{
	if (offset & 1)
		pc8801_dma_counter[offset >> 1] = pc8801_dmac_ff ? ((pc8801_dma_counter[offset >> 1] & 0xff) | (data << 8)) : ((pc8801_dma_counter[offset >> 1] & 0xff00) | (data & 0xff));
	else
		pc8801_dma_address[offset >> 1] = pc8801_dmac_ff ? ((pc8801_dma_address[offset >> 1] & 0xff) | (data << 8)) : ((pc8801_dma_address[offset >> 1] & 0xff00) | (data & 0xff));

	pc8801_dmac_ff ^= 1;
}

READ8_HANDLER( pc8801_dmac_status_r )
{
	return 0xff;
}

WRITE8_HANDLER( pc8801_dmac_mode_w )
{
	pc8801_dmac_mode = data;
	pc8801_dmac_ff = 0;
}
