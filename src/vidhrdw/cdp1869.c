// license:BSD-3-Clause
// copyright-holders:Curt Coder
/* RCA CDP1869/70/76 Video Interface System */

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "vidhrdw/cdp1869.h"
#include "sound/cdp1869.h"

typedef struct
{
	int ntsc_pal;
	int dispoff;
	int fresvert, freshorz;
	int dblpage, line16, line9, cmem, cfc;
	UINT8 col, bkg;
	UINT16 pma, hma;
	UINT8 cram[CDP1869_CHARRAM_SIZE];
	UINT8 pram[CDP1869_PAGERAM_SIZE];
	int cramsize, pramsize;
	UINT8 (*color_callback)(UINT8 cramdata, UINT16 cramaddr, UINT16 pramaddr);
} CDP1869_VIDEO_CONFIG;

static CDP1869_VIDEO_CONFIG cdp1869;

static UINT16 cdp1802_get_r_x(void)
{
	return activecpu_get_reg(CDP1802_R0 + activecpu_get_reg(CDP1802_X));
}

WRITE8_HANDLER ( cdp1869_out3_w )
{
	cdp1869.bkg = (data & 0x07);
	cdp1869.cfc = (data & 0x08) >> 3;
	cdp1869.dispoff = (data & 0x10) >> 4;
	cdp1869.col = (data & 0x60) >> 5;
	cdp1869.freshorz = (data & 0x80) >> 7;
}

WRITE8_HANDLER ( cdp1869_out4_w )
{
	UINT16 word = cdp1802_get_r_x();

	cdp1869_set_toneamp(0, word & 0x0f);
	cdp1869_set_tonefreq(0, (word & 0x70) >> 4);
	cdp1869_set_toneoff(0, (word & 0x80) >> 7);
	cdp1869_set_tonediv(0, (word & 0x7f00) >> 8);
}

WRITE8_HANDLER ( cdp1869_out5_w )
{
	UINT16 word = cdp1802_get_r_x();

	cdp1869.cmem = (word & 0x01);
	/* OUT5 latches the address bus along with the display control bits. */
	cdp1869.pma = cdp1869.cmem ? word & 0x7ff : 0;
	cdp1869.line9 = (word & 0x08) >> 3;
	cdp1869.line16 = (word & 0x20) >> 5;
	cdp1869.dblpage = (word & 0x40) >> 6;
	cdp1869.fresvert = (word & 0x80) >> 7;

	cdp1869_set_wnamp(0, (word & 0x0f00) >> 8);
	cdp1869_set_wnfreq(0, (word & 0x7000) >> 12);
	cdp1869_set_wnoff(0, (word & 0x8000) >> 15);
}

WRITE8_HANDLER ( cdp1869_out6_w )
{
	UINT16 word = cdp1802_get_r_x();

	cdp1869.pma = word & 0x7ff;
}

WRITE8_HANDLER ( cdp1869_out7_w )
{
	UINT16 word = cdp1802_get_r_x();

	cdp1869.hma = word & 0x7fc;
}

static void cdp1869_set_color(int i, int c, int l)
{
	int luma = 0, r, g, b;

	luma += (l & 4) ? CDP1869_WEIGHT_RED : 0;
	luma += (l & 1) ? CDP1869_WEIGHT_GREEN : 0;
	luma += (l & 2) ? CDP1869_WEIGHT_BLUE : 0;

	luma = (luma * 0xff) / 100;

	r = (c & 4) ? luma : 0;
	g = (c & 1) ? luma : 0;
	b = (c & 2) ? luma : 0;

	palette_set_color(i, r, g, b);
}

PALETTE_INIT( cdp1869 )
{
	int i, c, l;

	for (i = 0; i < 8; i++)
	{
		cdp1869_set_color(i, i, 15);
	}

	for (c = 0; c < 8; c++)
	{
		for (l = 0; l < 8; l++)
		{
			cdp1869_set_color(i, c, l);
			i++;
		}
	}
}

static int cdp1869_get_lines(void)
{
	if (cdp1869.line16 && !cdp1869.dblpage)
	{
		return 16;
	}
	else if (!cdp1869.line9)
	{
		return 9;
	}
	else
	{
		return 8;
	}
}

static UINT16 cdp1869_get_pmemsize(int cols, int rows)
{
	int pmemsize = cols * rows;

	if (cdp1869.dblpage) pmemsize *= 2;
	if (cdp1869.line16) pmemsize *= 2;

	return pmemsize;
}

static UINT16 cdp1869_get_pma(void)
{
	if (cdp1869.dblpage)
	{
		return cdp1869.pma;
	}
	else
	{
		return cdp1869.pma & 0x3ff;
	}
}

UINT16 cdp1869_get_cma(UINT16 offset)
{
	UINT16 pma = cdp1869.cmem ? cdp1869_get_pma() : offset;
	/* Unpopulated page RAM drives all ones; CMA3 is unconnected on these boards. */
	UINT8 column = pma < cdp1869.pramsize ? cdp1869.pram[pma] : 0xff;
	return (column << 3) | (offset & 7);
}

static UINT8 cdp1869_read_pageram(UINT16 addr)
{
	if (addr >= cdp1869.pramsize)
	{
		return 0xff;
	}
	else
	{
		return cdp1869.pram[addr];
	}
}

#ifdef UNUSED_FUNCTION
UINT8 cdp1869_read_charram(UINT16 addr)
{
	if (addr >= cdp1869.cramsize)
	{
		return 0xff;
	}
	else
	{
		return cdp1869.cram[addr];
	}
}
#endif

WRITE8_HANDLER ( cdp1869_charram_w )
{
	cdp1869.cram[cdp1869_get_cma(offset)] = data;
}

READ8_HANDLER ( cdp1869_charram_r )
{
	return cdp1869.cram[cdp1869_get_cma(offset)];
}

WRITE8_HANDLER ( cdp1869_pageram_w )
{
	UINT16 addr;

	if (cdp1869.cmem)
	{
		addr = cdp1869_get_pma();
	}
	else
	{
		addr = offset;
	}

	if (addr < cdp1869.pramsize)
		cdp1869.pram[addr] = data;
}

READ8_HANDLER ( cdp1869_pageram_r )
{
	UINT16 addr;

	if (cdp1869.cmem)
	{
		addr = cdp1869_get_pma();
	}
	else
	{
		addr = offset;
	}

	return cdp1869_read_pageram(addr);
}

static int cdp1869_get_color(int ccb0, int ccb1, int pcb)
{
	int r = 0, g = 0, b = 0, color;

	switch (cdp1869.col)
	{
	case 0:
		r = ccb0;
		g = pcb;
		b = ccb1;
		break;

	case 1:
		r = ccb0;
		g = ccb1;
		b = pcb;
		break;

	case 2:
	case 3:
		r = pcb;
		g = ccb1;
		b = ccb0;
		break;
	}

	color = (r << 2) + (b << 1) + g;

	if (cdp1869.cfc)
	{
		return color + ((cdp1869.bkg + 1) * 8);
	}
	else
	{
		return color;
	}
}

static void cdp1869_draw_line(mame_bitmap *bitmap, int x, int y, int data, int color)
{
	int i;

	data <<= 2;

	for (i = 0; i < CDP1869_CHAR_WIDTH; i++)
	{
		if (data & 0x80)
		{
			((UINT16 *)bitmap->line[y])[x] = Machine->pens[color];

			if (!cdp1869.fresvert)
			{
				((UINT16 *)bitmap->line[y + 1])[x] = Machine->pens[color];
			}

			if (!cdp1869.freshorz)
			{
				((UINT16 *)bitmap->line[y])[x + 1] = Machine->pens[color];

				if (!cdp1869.fresvert)
				{
					((UINT16 *)bitmap->line[y + 1])[x + 1] = Machine->pens[color];
				}
			}
		}

		if (!cdp1869.freshorz)
		{
			x++;
		}

		x++;

		data <<= 1;
	}
}

static void cdp1869_draw_char(mame_bitmap *bitmap, int x, int y, UINT16 pramaddr, const rectangle *screenrect)
{
	int i;
	UINT8 code = cdp1869_read_pageram(pramaddr);
	UINT16 addr = code * 8;

	for (i = 0; i < cdp1869_get_lines(); i++)
	{
		UINT8 data = cdp1869.cram[addr];

		UINT8 colorbits = cdp1869.color_callback(data, addr, pramaddr);

		int pcb = colorbits & 0x01;
		int ccb1 = (colorbits & 0x02) >> 1;
		int ccb0 = (colorbits & 0x04) >> 2;

		int color = cdp1869_get_color(ccb0, ccb1, pcb);

		cdp1869_draw_line(bitmap, screenrect->min_x + x, screenrect->min_y + y, data, color);

		addr = code * 8 + ((i + 1) & 7);
		y++;

		if (!cdp1869.fresvert)
		{
			y++;
		}

	}
}

VIDEO_START( cdp1869 )
{
	state_save_register_item("cdp1869", 0, cdp1869.ntsc_pal);
	state_save_register_item("cdp1869", 0, cdp1869.dispoff);
	state_save_register_item("cdp1869", 0, cdp1869.fresvert);
	state_save_register_item("cdp1869", 0, cdp1869.freshorz);
	state_save_register_item("cdp1869", 0, cdp1869.dblpage);
	state_save_register_item("cdp1869", 0, cdp1869.line16);
	state_save_register_item("cdp1869", 0, cdp1869.line9);
	state_save_register_item("cdp1869", 0, cdp1869.cmem);
	state_save_register_item("cdp1869", 0, cdp1869.cfc);
	state_save_register_item("cdp1869", 0, cdp1869.col);
	state_save_register_item("cdp1869", 0, cdp1869.bkg);
	state_save_register_item("cdp1869", 0, cdp1869.pma);
	state_save_register_item("cdp1869", 0, cdp1869.hma);
	state_save_register_item_array("cdp1869", 0, cdp1869.cram);
	state_save_register_item_array("cdp1869", 0, cdp1869.pram);
	state_save_register_item("cdp1869", 0, cdp1869.cramsize);
	state_save_register_item("cdp1869", 0, cdp1869.pramsize);
	return 0;
}

VIDEO_UPDATE( cdp1869 )
{
	fillbitmap(bitmap, get_black_pen(), cliprect);

	if (!cdp1869.dispoff)
	{
		int sx, sy, rows, cols, width, height;
		UINT16 addr, pmemsize;
		rectangle screen, outer;

		switch (cdp1869.ntsc_pal)
		{
		case CDP1869_NTSC:
			outer.min_x = CDP1869_HBLANK_END;
			outer.max_x = CDP1869_HBLANK_START - 1;
			outer.min_y = CDP1869_SCANLINE_VBLANK_END_NTSC;
			outer.max_y = CDP1869_SCANLINE_VBLANK_START_NTSC - 1;
			screen.min_x = CDP1869_SCREEN_START_NTSC;
			screen.max_x = CDP1869_SCREEN_END - 1;
			screen.min_y = CDP1869_SCANLINE_DISPLAY_START_NTSC;
			screen.max_y = CDP1869_SCANLINE_DISPLAY_END_NTSC - 1;
			break;

		default:
		case CDP1869_PAL:
			outer.min_x = CDP1869_HBLANK_END;
			outer.max_x = CDP1869_HBLANK_START - 1;
			outer.min_y = CDP1869_SCANLINE_VBLANK_END_PAL;
			outer.max_y = CDP1869_SCANLINE_VBLANK_START_PAL - 1;
			screen.min_x = CDP1869_SCREEN_START_PAL;
			screen.max_x = CDP1869_SCREEN_END - 1;
			screen.min_y = CDP1869_SCANLINE_DISPLAY_START_PAL;
			screen.max_y = CDP1869_SCANLINE_DISPLAY_END_PAL - 1;
			break;
		}

		sect_rect(&outer, cliprect);
		fillbitmap(bitmap, Machine->pens[cdp1869.bkg], &outer);

		width = CDP1869_CHAR_WIDTH;
		height = cdp1869_get_lines();

		if (!cdp1869.freshorz)
		{
			width *= 2;
		}

		if (!cdp1869.fresvert)
		{
			height *= 2;
		}

		cols = cdp1869.freshorz ? CDP1869_COLUMNS_FULL : CDP1869_COLUMNS_HALF;
		rows = (screen.max_y - screen.min_y + 1) / height;

		pmemsize = cdp1869_get_pmemsize(cols, rows);

		addr = cdp1869.hma;

		for (sy = 0; sy < rows; sy++)
		{
			for (sx = 0; sx < cols; sx++)
			{
				int x = sx * width;
				int y = sy * height;

				cdp1869_draw_char(bitmap, x, y, addr, &screen);

				addr++;

				if (addr == pmemsize) addr = 0;
			}
		}
	}

}

void cdp1869_configure(const CDP1869_interface *intf)
{
	if (intf->charrom_region != REGION_INVALID)
	{
		UINT8 *memory = memory_region(intf->charrom_region);
		memcpy(cdp1869.cram, memory, intf->charram_size);
	}

	cdp1869.ntsc_pal = intf->ntsc_pal;
	cdp1869.cramsize = intf->charram_size;
	cdp1869.pramsize = intf->pageram_size;
	cdp1869.color_callback = intf->get_color_bits;
}
