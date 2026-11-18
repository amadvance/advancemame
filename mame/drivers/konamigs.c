// license:BSD-3-Clause
// copyright-holders:MetalliC, Angelo Salese
/*************************************************************************

    Konami GSAN1 hardware
    (c) 2000

    CPU: Hitachi HD6417709 SH-3
    GPU: Hitachi HD64413AF 'Q2SD' Quick 2D Graphics Renderer with Synchronous DRAM Interface
    SPU: Yamaha YMZ280B-F
    Misc:
         Altera Max EPM3256ATC144-10
         Altera Max EPM3064ATC100-10
         Epson RTC-4553

*************************************************************************/

#include "driver.h"
#include "cpu/sh3/sh3.h"
#include "machine/idectrl.h"
#include "machine/ticket.h"
#include "sound/ymz280b.h"
#include <time.h>

static UINT16 *vram;
static UINT32 *workram;
static UINT8 *ymzram;
static UINT16 gpuregs[0x400];
static int dbmode, fg16bit, bg16bit, rend16bit, width1024, rsae, vbkem;
static INT16 xo, yo, uxmin, uxmax, uymin, uymax, sxmax, symax;
static UINT8 portc_data, porte_data;
static int medal;
enum { RTC_SECOND, RTC_MINUTE, RTC_HOUR, RTC_WEEKDAY, RTC_DAY, RTC_MONTH, RTC_YEAR };
static UINT8 rtc_nvdata[15], rtc_clock[RTC_YEAR + 1];
static UINT8 rtc_mode, rtc_sysr, rtc_cntrl1, rtc_cntrl2;
static UINT8 rtc_bitstream, rtc_stream_pos, rtc_read_latch, rtc_pins;

static inline UINT32 get_rend_offset(void)
{
	UINT16 val = rsae ? gpuregs[0x098 / 2] : gpuregs[0x014 / 2 + (BIT(gpuregs[0x002 / 2], 8) ? 0 : 1)];
	return (val & 0x7f) << 16;
}
static inline UINT16 get_color(UINT8 col)
{
	return ((gpuregs[0x100 + col * 2] & 0x00f8) << 8) | ((gpuregs[0x101 + col * 2] & 0xfc00) >> 5) | ((gpuregs[0x101 + col * 2] & 0x00f8) >> 3);
}
static inline UINT32 pixel_address(UINT32 base, UINT16 x, UINT16 y, int bits16)
{
	/* Both formats use 32-byte rows within a tile, with 16 rows per tile. */
	UINT32 byte_x = (UINT32)x << bits16;
	UINT32 stride = (width1024 ? 1024 : 512) << bits16;
	UINT32 offset = (byte_x & 31) + (y & 15) * 32
		+ (byte_x & (stride - 32)) * 16 + (y & ~15U) * stride;
	return (base + offset) & 0x7fffff;
}

static inline UINT16 get_pixel(UINT32 base, UINT16 x, UINT16 y, int bits16)
{
	UINT32 offset = pixel_address(base, x, y, bits16);
	UINT16 value = vram[offset / 2];
	return bits16 ? value : (value >> ((offset & 1) * 8)) & 0xff;
}

static inline void put_pixel(UINT32 base, UINT16 x, UINT16 y, UINT16 color, int bits16)
{
	UINT32 offset = pixel_address(base, x, y, bits16);
	if (bits16)
		vram[offset / 2] = color;
	else
	{
		unsigned shift = (offset & 1) * 8;
		vram[offset / 2] = (vram[offset / 2] & (0xff00 >> shift)) | (color << shift);
	}
}

static void do_render(int resume);

static void gpu_w(offs_t offset, UINT16 data, UINT16 mem_mask, int from_list)
{
	UINT16 prevval = gpuregs[offset];
	COMBINE_DATA(&gpuregs[offset]);
	data = gpuregs[offset];

	switch (offset)
	{
	case 0x000:
		if (BIT(data, 15))
		{
			gpuregs[0x000 / 2] &= ~(1 << 10);
			gpuregs[0x002 / 2] &= ~0x1680;
		}
		if (BIT(data, 14))
		{
			gpuregs[0x002 / 2] &= ~(1 << 15);
			gpuregs[0x002 / 2] &= ~(1 << 14);
			gpuregs[0x002 / 2] &= ~(1 << 11);
		}
		if (BIT(data, 10))
		{
			if (vbkem)
			{
				gpuregs[0x002 / 2] |= 1 << 7;
				vbkem = 0;
			}
		}
		if (BIT(data, 8))
		{
			gpuregs[0x000 / 2] &= ~(1 << 8);
			/* Register writes in a display list cannot start a nested list. */
			if (!from_list)
				do_render(0);
		}
		dbmode = (data >> 6) & 3;
		break;
	case 0x004 / 2:
		gpuregs[0x002 / 2] &= ~(data & 0xff80);
		break;
	case 0x006 / 2:
		if (data)
			logerror("Q2SD interrupts not implemented! enabled %04x\n", data);
		break;
	case 0x00c / 2:
		fg16bit = BIT(data, 0);
		bg16bit = BIT(data, 1) ^ BIT(data, 0);
		rend16bit = BIT(data, 2) ^ BIT(data, 0);
		width1024 = BIT(data, 6);
		rsae = BIT(data, 15);
		break;

	case 0x002 / 2:
	case 0x03e / 2: case 0x040 / 2:
	case 0x080 / 2: case 0x082 / 2:
	case 0x084 / 2: case 0x086 / 2:
	case 0x088 / 2: case 0x08a / 2: case 0x08c / 2: case 0x08e / 2:
	case 0x090 / 2: case 0x092 / 2:
		gpuregs[offset] = prevval;
		break;
	}
}

static INTERRUPT_GEN(gsan_vblank)
{
	switch (dbmode)
	{
	case 0:
		gpuregs[0x002 / 2] ^= 1 << 8;
		vbkem = 0;
		break;
	case 1:
		if (vbkem)
		{
			vbkem = 0;
			do_render(1);
		}
		else
			gpuregs[0x002 / 2] ^= 1 << 8;
		break;
	case 2:
	case 3:
		if (BIT(gpuregs[0x000 / 2], 9))
		{
			gpuregs[0x000 / 2] &= ~(1 << 9);
			gpuregs[0x002 / 2] ^= 1 << 8;
		}
		if (vbkem)
		{
			vbkem = 0;
			do_render(1);
		}
		break;
	}

	gpuregs[0x002 / 2] |= 1 << 11;
}

static void draw_quad_tex(UINT16 cmd, const UINT16 *data)
{
	if (cmd & 0x57f)
		logerror("Q2SD unhandled draw tex mode %04X\n", cmd);
	UINT32 ssx = data[0] & 0x3ff;
	UINT32 ssy = data[1] & 0x3ff;
	INT16 sdx = data[4];
	INT16 sdy = data[5];
	int edx = (INT16)data[8];
	int edy = (INT16)data[9];

	sdx += xo;
	edx += xo;
	sdy += yo;
	edy += yo;

	INT16 sclipx, sclipy, eclipx, eclipy;
	if (BIT(cmd, 7))
	{
		sclipx = uxmin;
		sclipy = uymin;
		eclipx = MIN(uxmax, sxmax);
		eclipy = MIN(uymax, symax);
	}
	else
	{
		sclipx = 0;
		sclipy = 0;
		eclipx = sxmax;
		eclipy = symax;
	}
	UINT32 fg_offs = get_rend_offset();

	INT16 ddx = (edx >= sdx) ? 1 : -1;
	INT16 ddy = (edy >= sdy) ? 1 : -1;
	edx += ddx;
	edy += ddy;

	UINT32 src_offs = ((gpuregs[0x01c / 2] & 0x7f) << 16) | (gpuregs[0x01c / 2] & 0xe000);

	int opaque = !BIT(cmd, 9);

	for (int y = sdy, sy = 0; y != edy; y += ddy, sy++)
		for (int x = sdx, sx = 0; x != edx; x += ddx, sx++)
			if (x >= sclipx && x <= eclipx && y >= sclipy && y <= eclipy)
			{
				UINT16 pix = get_pixel(src_offs, ssx + sx, ssy + sy, rend16bit);
				if (pix || opaque)
					put_pixel(fg_offs, x, y, pix, rend16bit);
			}
}

static void draw_quad_bin(UINT16 cmd, const UINT16 *data)
{
	if (cmd & 0x57f)
		logerror("Q2SD unhandled draw bin mode %04X\n", cmd);

	UINT32 src_offs = ((data[0] & 0x03ff) << 13) | (data[1] & 0x1fff);
	UINT32 tdx = data[2];
	INT16 sdx = data[4];
	INT16 sdy = data[5];
	int edx = (INT16)data[8];
	int edy = (INT16)data[9];
	UINT16 color0 = data[0xc];
	UINT16 color1 = data[0xd];

	sdx += xo;
	edx += xo;
	sdy += yo;
	edy += yo;

	INT16 sclipx, sclipy, eclipx, eclipy;
	if (BIT(cmd, 7))
	{
		sclipx = uxmin;
		sclipy = uymin;
		eclipx = MIN(uxmax, sxmax);
		eclipy = MIN(uymax, symax);
	}
	else
	{
		sclipx = 0;
		sclipy = 0;
		eclipx = sxmax;
		eclipy = symax;
	}
	UINT32 fg_offs = get_rend_offset();

	INT16 ddx = (edx >= sdx) ? 1 : -1;
	INT16 ddy = (edy >= sdy) ? 1 : -1;
	edx += ddx;
	edy += ddy;

	int opaque = !BIT(cmd, 9);
	if (!rend16bit)
	{
		color0 &= 0xff;
		color1 &= 0xff;
	}

	for (int y = sdy, sy = 0; y != edy; y += ddy, sy++)
		for (int x = sdx, sx = 0; x != edx; x += ddx, sx++)
			if (x >= sclipx && x <= eclipx && y >= sclipy && y <= eclipy)
			{
				UINT32 pixidx = sx + sy * tdx + src_offs * 8;
				int pix = (vram[(pixidx / 16) & 0x3fffff] >> (pixidx % 16)) & 1;
				if (opaque || pix)
					put_pixel(fg_offs, x, y, pix ? color1 : color0, rend16bit);
			}
}

static void fill_quad(UINT16 cmd, const UINT16 *data)
{
	if (cmd & 0x77f)
		logerror("Q2SD unhandled draw mode %04X\n", cmd);
	INT16 sdx = data[0];
	INT16 sdy = data[1];
	int edx = (INT16)data[4];
	int edy = (INT16)data[5];
	UINT16 color = data[8];

	sdx += xo;
	edx += xo;
	sdy += yo;
	edy += yo;

	INT16 sclipx, sclipy, eclipx, eclipy;
	if (BIT(cmd, 7))
	{
		sclipx = uxmin;
		sclipy = uymin;
		eclipx = MIN(uxmax, sxmax);
		eclipy = MIN(uymax, symax);
	}
	else
	{
		sclipx = 0;
		sclipy = 0;
		eclipx = sxmax;
		eclipy = symax;
	}
	UINT32 fg_offs = get_rend_offset();
	if (!rend16bit)
		color &= 0xff;

	INT16 ddx = (edx >= sdx) ? 1 : -1;
	INT16 ddy = (edy >= sdy) ? 1 : -1;
	edx += ddx;
	edy += ddy;

	for (int y = sdy; y != edy; y += ddy)
		for (int x = sdx; x != edx; x += ddx)
		{
			if (x >= sclipx && x <= eclipx && y >= sclipy && y <= eclipy)
				put_pixel(fg_offs, x, y, color, rend16bit);
		}
}

static void draw_line(UINT16 cmd, const UINT16 *data)
{
	if (cmd & 0x77f)
		logerror("Q2SD unhandled line mode %04X\n", cmd);

	INT16 sclipx, sclipy, eclipx, eclipy;
	if (BIT(cmd, 7))
	{
		sclipx = uxmin;
		sclipy = uymin;
		eclipx = MIN(uxmax, sxmax);
		eclipy = MIN(uymax, symax);
	}
	else
	{
		sclipx = 0;
		sclipy = 0;
		eclipx = sxmax;
		eclipy = symax;
	}
	UINT32 fg_offs = get_rend_offset();

	UINT16 color = *data++;
	if (!rend16bit)
		color &= 0xff;

	UINT16 count = *data++;
	while (count > 1)
	{
		INT16 x0 = *data++ + xo;
		INT16 y0 = *data++ + yo;
		INT16 x1 = data[0] + xo;
		INT16 y1 = data[1] + yo;
		--count;

		int dx = abs(x1 - x0);
		int dy = -abs(y1 - y0);
		int sx = x0 < x1 ? 1 : -1;
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;

		while (1)
		{
			if (x0 >= sclipx && x0 <= eclipx && y0 >= sclipy && y0 <= eclipy)
				put_pixel(fg_offs, x0, y0, color, rend16bit);

			if (x0 == x1 && y0 == y1)
				break;

			int e2 = 2 * err;
			if (e2 >= dy)
			{
				err += dy;
				x0 += sx;
			}
			if (e2 <= dx)
			{
				err += dx;
				y0 += sy;
			}
		}
	}
}

static void do_render(int resume)
{
	static const UINT8 parameter_words[32] =
	{
		[0x00] = 12, [0x01] = 14, [0x02] = 9, [0x0c] = 2,
		[0x12] = 2, [0x15] = 4, [0x16] = 2, [0x17] = 2,
		[0x1a] = 2, [0x1e] = 2
	};
	unsigned addr_reg = resume ? 0x03e / 2 : 0x018 / 2;
	UINT32 listoffs = (((UINT32)gpuregs[addr_reg] << 16) | gpuregs[addr_reg + 1]) / 2;
	const UINT32 vram_words = 0x400000;

	for (listoffs &= vram_words - 1; listoffs < vram_words; )
	{
		UINT16 cmd = vram[listoffs++];
		UINT16 *params = &vram[listoffs];
		unsigned opcode = cmd >> 11;
		unsigned count = parameter_words[opcode];

		/* Commands are fetched from VRAM, including the variable-length line vertices. */
		if (count > vram_words - listoffs)
			return;
		if (opcode == 0x0c)
		{
			count += (unsigned)params[1] * 2;
			if (count > vram_words - listoffs)
				return;
		}
		listoffs += count;

		switch (opcode)
		{
		case 0x00:
			draw_quad_tex(cmd, params);
			break;
		case 0x01:
			draw_quad_bin(cmd, params);
			break;
		case 0x02:
			fill_quad(cmd, params);
			break;
		case 0x0c:
			draw_line(cmd, params);
			break;
		case 0x12:
			xo = gpuregs[0x84 / 2] = params[0];
			yo = gpuregs[0x86 / 2] = params[1];
			break;
		case 0x15:
			uxmin = gpuregs[0x88 / 2] = params[0];
			uymin = gpuregs[0x8a / 2] = params[1];
			uxmax = gpuregs[0x8c / 2] = params[2];
			uymax = gpuregs[0x8e / 2] = params[3];
			break;
		case 0x16:
			gpu_w(params[0] & 0x3ff, params[1], 0, 1);
			break;
		case 0x17:
			sxmax = gpuregs[0x90 / 2] = params[0];
			symax = gpuregs[0x92 / 2] = params[1];
			break;
		case 0x1a:
			/* Resume at the following command when vertical blank starts. */
			gpuregs[0x03e / 2] = (listoffs * 2) >> 16;
			gpuregs[0x040 / 2] = (listoffs * 2) & 0xffff;
			vbkem = 1;
			return;
		case 0x1e:
			break;
		case 0x1f:
			gpuregs[0x000 / 2] &= ~(1 << 10);
			gpuregs[0x002 / 2] |= 1 << 10;
			return;
		default:
			logerror("Q2SD unknown command %04X at %08X\n", cmd, (listoffs - 1) * 2);
			return;
		}
	}
}

static VIDEO_UPDATE(gsan)
{
	if (BIT(gpuregs[0x000 / 2], 13))
	{
		UINT32 fg_offs = (gpuregs[0x014 / 2 + BIT(gpuregs[0x002 / 2], 8)] & 0x7f) << 16;
		int fg_en = !BIT(gpuregs[0x056 / 2], 3);
		int bg_en = BIT(gpuregs[0x00a / 2], 10);

		int bgsx = gpuregs[0x04c / 2];
		int bgsy = gpuregs[0x04e / 2];

		for (int y = cliprect->min_y; y <= cliprect->max_y; y++)
			for (int x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				UINT16 col = 0;
				if (fg_en)
				{
					col = get_pixel(fg_offs, x, y, fg16bit);
					if (!fg16bit)
						col = get_color(col);
				}
				if (bg_en && col == 0)
				{
					col = get_pixel(0, x + bgsx, y + bgsy, bg16bit);
					if (!bg16bit)
						col = get_color(col);
				}
				((UINT32 *)bitmap->line[y])[x] = MAKE_RGB(pal5bit(col >> 11), pal6bit(col >> 5), pal5bit(col));
			}
	}
	else
		fillbitmap(bitmap, 0, cliprect);
}

static UINT8 bcd(int value)
{
	return ((value / 10) << 4) | (value % 10);
}

static void rtc_normalize(void)
{
	static const UINT8 limits[] = { 60, 60, 24 };
	static const UINT8 month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	unsigned month, days;
	int i;

	for (i = RTC_SECOND; i <= RTC_HOUR; i++)
		if (rtc_clock[i] >= limits[i])
		{
			rtc_clock[i] = 0;
			if (i == RTC_HOUR)
			{
				rtc_clock[RTC_WEEKDAY]++;
				rtc_clock[RTC_DAY]++;
			}
			else
				rtc_clock[i + 1]++;
		}

	rtc_clock[RTC_WEEKDAY] %= 7;
	if (rtc_clock[RTC_MONTH] > 12)
	{
		rtc_clock[RTC_MONTH] = 1;
		rtc_clock[RTC_YEAR]++;
	}
	month = rtc_clock[RTC_MONTH];
	days = month_days[month - 1] + (month == 2 && rtc_clock[RTC_YEAR] % 4 == 0);
	if (rtc_clock[RTC_DAY] > days)
	{
		rtc_clock[RTC_DAY] = 1;
		if (++rtc_clock[RTC_MONTH] > 12)
		{
			rtc_clock[RTC_MONTH] = 1;
			rtc_clock[RTC_YEAR]++;
		}
	}
	rtc_clock[RTC_YEAR] %= 100;
}

static void rtc_tick(int param)
{
	rtc_clock[RTC_SECOND]++;
	rtc_normalize();
}

static UINT8 rtc_access(UINT8 addr, UINT8 data, int write)
{
	static const UINT8 clock_index[] =
	{
		RTC_SECOND, RTC_SECOND, RTC_MINUTE, RTC_MINUTE, RTC_HOUR, RTC_HOUR,
		RTC_WEEKDAY, RTC_DAY, RTC_DAY, RTC_MONTH, RTC_MONTH, RTC_YEAR, RTC_YEAR
	};
	static const UINT8 clock_shift[] = { 0, 4, 0, 4, 0, 4, 0, 0, 4, 0, 4, 0, 4 };

	if (addr == 15)
	{
		if (write)
		{
			rtc_mode = data & 3;
			rtc_sysr = data & 8;
			if (rtc_sysr)
			{
				memset(rtc_clock, 0, sizeof(rtc_clock));
				rtc_clock[RTC_DAY] = rtc_clock[RTC_MONTH] = rtc_clock[RTC_YEAR] = 1;
			}
		}
		return rtc_mode | rtc_sysr;
	}

	if (rtc_mode >= 2)
	{
		unsigned index = addr + (rtc_mode - 2) * 15;
		unsigned shift = (index & 1) * 4;
		UINT8 *value = &rtc_nvdata[index / 2];
		if (write)
			*value = (*value & ~(15 << shift)) | (data << shift);
		return (*value >> shift) & 15;
	}

	if (addr >= 13)
	{
		UINT8 *control = addr == 13 ? &rtc_cntrl1 : &rtc_cntrl2;
		if (write)
			*control = data;
		return *control;
	}

	if (write)
	{
		UINT8 *value = &rtc_clock[clock_index[addr]];
		if (rtc_cntrl1 & 2)
		{
			if (clock_index[addr] == RTC_YEAR)
				*value = clock_shift[addr] ? *value % 10 : *value / 10 * 10;
			else
				*value = clock_index[addr] == RTC_DAY || clock_index[addr] == RTC_MONTH;
		}
		else if (addr != 5)
		{
			*value += clock_shift[addr] ? 10 : 1;
			rtc_normalize();
		}
	}
	return (bcd(rtc_clock[clock_index[addr]]) >> clock_shift[addr]) & 15;
}

static void rtc_write(UINT8 data)
{
	/* Chip select can change while the clock remains high. */
	UINT8 rising = data & ~rtc_pins;
	rtc_pins = data;
	if (BIT(data, 2))
		rtc_stream_pos = 0;
	else if (BIT(rising, 0))
	{
		/* Each byte returns the response to the preceding byte. */
		rtc_read_latch = rtc_bitstream & 1;
		rtc_bitstream = (rtc_bitstream >> 1) | ((data & 2) << 6);
		rtc_stream_pos = (rtc_stream_pos + 1) & 7;
		if (!rtc_stream_pos)
		{
			UINT8 addr = rtc_bitstream & 15;
			rtc_bitstream = addr | (rtc_access(addr, rtc_bitstream >> 4, !BIT(data, 3)) << 4);
		}
	}
}

static NVRAM_HANDLER(gsan)
{
	if (read_or_write)
		mame_fwrite(file, rtc_nvdata, sizeof(rtc_nvdata));
	else if (file)
		mame_fread(file, rtc_nvdata, sizeof(rtc_nvdata));
	else
		memcpy(rtc_nvdata, memory_region(REGION_USER1), sizeof(rtc_nvdata));
}

static READ32_HANDLER(vram_read)
{
	return ((UINT32)vram[offset * 2] << 16) | vram[offset * 2 + 1];
}

static WRITE32_HANDLER(vram_write)
{
	vram[offset * 2] = (vram[offset * 2] & (mem_mask >> 16)) | ((data & ~mem_mask) >> 16);
	COMBINE_DATA(&vram[offset * 2 + 1]);
}

static READ32_HANDLER(gpu_read)
{
	return ((UINT32)gpuregs[offset * 2] << 16) | gpuregs[offset * 2 + 1];
}

static WRITE32_HANDLER(gpu_write)
{
	if ((mem_mask >> 16) != 0xffff)
		gpu_w(offset * 2, data >> 16, mem_mask >> 16, 0);
	if ((mem_mask & 0xffff) != 0xffff)
		gpu_w(offset * 2 + 1, data, mem_mask, 0);
}

#define BYTE_SHIFT(lane) (24 - 8 * (lane))

static READ32_HANDLER(cf_regs_read)
{
	UINT32 result = 0;
	int reg;
	for (reg = 0; reg < 4; reg++)
	{
		int shift = BYTE_SHIFT(reg ^ 1);
		if (!(mem_mask & (0xffU << shift)))
		{
			int task_reg = offset * 4 + reg;
			UINT8 value = ide_controller_0_r(0x1f0 + task_reg);
			/* CompactFlash has no rotational index pulse. The BIOS checks the entire status byte. */
			if (task_reg == 7)
				value &= ~0x02;
			result |= (UINT32)value << shift;
		}
	}
	return result;
}

static WRITE32_HANDLER(cf_regs_write)
{
	int reg;
	/* A device/head and command word must select the device before issuing the command. */
	for (reg = 0; reg < 4; reg++)
	{
		int shift = BYTE_SHIFT(reg ^ 1);
		if (!(mem_mask & (0xffU << shift)))
			ide_controller_0_w(0x1f0 + offset * 4 + reg, (data >> shift) & 0xff);
	}
}

static READ32_HANDLER(cf_data_read)
{
	UINT32 value = 0;
	if (ACCESSING_MSW32)
		value = (UINT32)ide_bus_0_r(0, 0) << 16;
	if (ACCESSING_LSW32)
		value |= ide_bus_0_r(0, 0);
	return value;
}

static WRITE32_HANDLER(cf_data_write)
{
	if (ACCESSING_MSW32)
		ide_bus_0_w(0, 0, data >> 16);
	if (ACCESSING_LSW32)
		ide_bus_0_w(0, 0, data & 0xffff);
}

static READ32_HANDLER(dsw_read)
{
	return readinputportbytag("DSW") << 16;
}

static READ32_HANDLER(rtc_read_port)
{
	return rtc_read_latch << 16;
}

static WRITE32_HANDLER(rtc_write_port)
{
	if (!(mem_mask & 0x00ff0000))
		rtc_write(data >> 16);
}

static READ8_HANDLER(ymz_sample_read)
{
	return ymzram[offset & (medal ? 0x3fffff : 0x7fffff)];
}

static WRITE8_HANDLER(ymz_sample_write)
{
	ymzram[offset & (medal ? 0x3fffff : 0x7fffff)] = data;
}

static READ32_HANDLER(ymz_ram_read)
{
	UINT8 *ptr = ymzram + offset * 4;
	return ((UINT32)ptr[0] << 24) | ((UINT32)ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

static WRITE32_HANDLER(ymz_ram_write)
{
	int lane;
	for (lane = 0; lane < 4; lane++)
		if (!(mem_mask & (0xffU << BYTE_SHIFT(lane))))
			ymzram[offset * 4 + lane] = data >> BYTE_SHIFT(lane);
}

static READ32_HANDLER(ymz_read)
{
	UINT32 result = 0;
	if (!(mem_mask & 0xff000000))
		result |= (UINT32)YMZ280B_data_0_r(0) << 24;
	if (!(mem_mask & 0x00ff0000))
		result |= YMZ280B_status_0_r(0) << 16;
	return result;
}

static WRITE32_HANDLER(ymz_write)
{
	if (!(mem_mask & 0xff000000))
		YMZ280B_register_0_w(0, data >> 24);
	if (!(mem_mask & 0x00ff0000))
		YMZ280B_data_0_w(0, data >> 16);
}

static UINT8 gsan_port_read(int port)
{
	switch (port)
	{
		case SH3_PORT_C: return portc_data;
		case SH3_PORT_E: return porte_data;
		case SH3_PORT_F:
			return readinputportbytag("PORT_F") | (medal ? ticket_dispenser_r(0) : 0);
		case SH3_PORT_L: return readinputportbytag("PORT_L");
	}
	return 0;
}

static void gsan_port_write(int port, UINT8 data)
{
	if (port == SH3_PORT_C)
	{
		portc_data = data;
		if (medal)
		{
			ticket_dispenser_w(0, ~data);
			coin_counter_w(0, BIT(data, 2));
			coin_counter_w(1, BIT(data, 1));
			coin_counter_w(2, BIT(data, 0));
		}
		else
			coin_counter_w(0, BIT(~data, 3));
	}
	else if (port == SH3_PORT_E)
	{
		porte_data = data;
		if (medal)
		{
			coin_lockout_w(0, BIT(data, 2));
			coin_lockout_w(1, BIT(data, 1));
			coin_lockout_w(2, BIT(data, 0));
		}
	}
}

#define GSAN_MAP_COMMON \
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM AM_REGION(REGION_CPU1, 0) \
	AM_RANGE(0x0c000000, 0x0c3fffff) AM_RAM AM_BASE(&workram) \
	AM_RANGE(0x10000000, 0x100007ff) AM_READWRITE(gpu_read, gpu_write) \
	AM_RANGE(0x14000800, 0x14000807) AM_READWRITE(cf_regs_read, cf_regs_write) \
	AM_RANGE(0x14000c00, 0x14000c03) AM_READWRITE(cf_data_read, cf_data_write) \
	AM_RANGE(0x14001000, 0x14001003) AM_READ(dsw_read) \
	AM_RANGE(0x14001018, 0x1400101b) AM_WRITE(rtc_write_port) \
	AM_RANGE(0x14001038, 0x1400103b) AM_READ(rtc_read_port) \
	AM_RANGE(0x18000000, 0x18000003) AM_READWRITE(ymz_read, ymz_write) \
	AM_RANGE(0x1f000000, 0x1f000fff) AM_RAM

static ADDRESS_MAP_START(gsan_map, ADDRESS_SPACE_PROGRAM, 32)
	GSAN_MAP_COMMON
	AM_RANGE(0x08000000, 0x087fffff) AM_READWRITE(vram_read, vram_write)
	AM_RANGE(0x18800000, 0x18ffffff) AM_READWRITE(ymz_ram_read, ymz_ram_write)
ADDRESS_MAP_END

static ADDRESS_MAP_START(gs_medal_map, ADDRESS_SPACE_PROGRAM, 32)
	GSAN_MAP_COMMON
	AM_RANGE(0x08000000, 0x083fffff) AM_READWRITE(vram_read, vram_write)
	AM_RANGE(0x18800000, 0x18bfffff) AM_READWRITE(ymz_ram_read, ymz_ram_write)
ADDRESS_MAP_END

static INPUT_PORTS_START( ddrkids )
	PORT_START_TAG("PORT_F")
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Select R")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Select L")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("PORT_L")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("S2")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("S1")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easiest ) )
	PORT_DIPSETTING(      0x0001, "Very Easy" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0004, "Medium Hard" )
	PORT_DIPSETTING(      0x0005, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0078, 0x0000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0048, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0058, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0068, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0078, DEF_STR( 4C_5C ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Demo Volume" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Low ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( High ) )
	PORT_DIPNAME( 0x1c00, 0x0800, "Max Stage" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "2" )
	PORT_DIPSETTING(      0x0800, "3" )
	PORT_DIPSETTING(      0x0c00, "4" )
	PORT_DIPSETTING(      0x1000, "5" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x8000, DEF_STR( On ) )

INPUT_PORTS_END

static INPUT_PORTS_START( muscl )
	PORT_START_TAG("PORT_L")
	PORT_SERVICE_NO_TOGGLE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("PORT_F")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("Medal")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x78, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0007, "Coin Slot 1" )
	PORT_DIPSETTING(      0x0003, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0000, "5 Coins/2 Credits" )
	PORT_DIPNAME( 0x0078, 0x0078, "Coin Slot 2" )
	PORT_DIPSETTING(      0x0078, "2 Medals" )

	PORT_DIPSETTING(      0x0068, "3 Medals" )
	PORT_DIPSETTING(      0x0060, "4 Medals" )
	PORT_DIPSETTING(      0x0058, "5 Medals" )
	PORT_DIPSETTING(      0x0050, "6 Medals" )
	PORT_DIPSETTING(      0x0048, "7 Medals" )
	PORT_DIPSETTING(      0x0040, "8 Medals" )
	PORT_DIPSETTING(      0x0038, "9 Medals" )
	PORT_DIPSETTING(      0x0030, "10 Medals" )
	PORT_DIPSETTING(      0x0028, "11 Medals" )
	PORT_DIPSETTING(      0x0020, "12 Medals" )
	PORT_DIPSETTING(      0x0018, "13 Medals" )
	PORT_DIPSETTING(      0x0010, "14 Medals" )
	PORT_DIPSETTING(      0x0008, "15 Medals" )
	PORT_DIPSETTING(      0x0000, "16 Medals" )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0f00, 0x0000, "Standard of Payout" )
	PORT_DIPSETTING(      0x0f00, "15%" )
	PORT_DIPSETTING(      0x0e00, "20%" )
	PORT_DIPSETTING(      0x0d00, "25%" )
	PORT_DIPSETTING(      0x0c00, "30%" )
	PORT_DIPSETTING(      0x0b00, "35%" )
	PORT_DIPSETTING(      0x0a00, "40%" )
	PORT_DIPSETTING(      0x0900, "45%" )
	PORT_DIPSETTING(      0x0800, "50%" )
	PORT_DIPSETTING(      0x0700, "55%" )
	PORT_DIPSETTING(      0x0600, "60%" )
	PORT_DIPSETTING(      0x0500, "65%" )
	PORT_DIPSETTING(      0x0400, "70%" )
	PORT_DIPSETTING(      0x0300, "75%" )
	PORT_DIPSETTING(      0x0200, "80%" )
	PORT_DIPSETTING(      0x0100, "85%" )
	PORT_DIPSETTING(      0x0000, "90%" )
	PORT_DIPNAME( 0x3000, 0x0000, "Play Timer" )
	PORT_DIPSETTING(      0x3000, "1" )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x1000, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x4000, 0x4000, "Backup Memory" )
	PORT_DIPSETTING(      0x4000, "Keep" )
	PORT_DIPSETTING(      0x0000, "Clear" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END

static INPUT_PORTS_START( runpuppy )
	PORT_INCLUDE( muscl )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x3000, 0x0000, "Play Timer" )
	PORT_DIPSETTING(      0x3000, "8" )
	PORT_DIPSETTING(      0x2000, "12" )
	PORT_DIPSETTING(      0x1000, "16" )
	PORT_DIPSETTING(      0x0000, "20" )
INPUT_PORTS_END


static struct sh3_config gsan_sh3 = { 0, 64000000 / 4, gsan_port_read, gsan_port_write };
static struct ide_interface gsan_ide = { NULL };
static struct YMZ280Binterface gsan_ymz = { REGION_SOUND1, NULL, ymz_sample_read, ymz_sample_write };

static MACHINE_START(gsan)
{
	ymz280b_enable_sample_ram(0, memory_region_length(REGION_SOUND1));
	return 0;
}

static MACHINE_RESET(gsan)
{
	memset(gpuregs, 0, sizeof(gpuregs));
	gpuregs[0x000 / 2] = 0xc000;
	gpuregs[0x002 / 2] = 0x0044;
	gpuregs[0x00a / 2] = 0x0088;
	gpuregs[0x072 / 2] = 0xc000;
	dbmode = fg16bit = bg16bit = rend16bit = width1024 = rsae = vbkem = 0;
	xo = yo = uxmin = uxmax = uymin = uymax = sxmax = symax = 0;
	portc_data = porte_data = 0xff;
	rtc_mode = rtc_stream_pos = rtc_pins = 0;
	if (medal)
		ticket_dispenser_w(0, 0);
	ide_controller_reset(0);
}

static MACHINE_DRIVER_START(gsan)
	MDRV_CPU_ADD_TAG("main", SH3BE, 64000000)
	MDRV_CPU_CONFIG(gsan_sh3)
	MDRV_CPU_PROGRAM_MAP(gsan_map, 0)
	MDRV_CPU_VBLANK_INT(gsan_vblank, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(4167)
	MDRV_MACHINE_START(gsan)
	MDRV_MACHINE_RESET(gsan)
	MDRV_NVRAM_HANDLER(gsan)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 399, 0, 299)
	MDRV_VIDEO_UPDATE(gsan)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(gsan_ymz)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START(gs_medal)
	MDRV_IMPORT_FROM(gsan)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(gs_medal_map, 0)
	MDRV_FRAMES_PER_SECOND(7200000.0 / (457 * 262))
	MDRV_VBLANK_DURATION(1396)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
MACHINE_DRIVER_END

static DRIVER_INIT(gsan)
{
	time_t now = time(NULL);
	const struct tm *date = localtime(&now);

	ymzram = memory_region(REGION_SOUND1);
	medal = memory_region_length(REGION_SOUND1) == 0x400000;
	vram = auto_malloc(0x800000);
	memset(vram, 0, 0x800000);
	sh3_set_fastram(0, 0xffff, 1, (UINT32 *)memory_region(REGION_CPU1));
	sh3_set_fastram(0x0c000000, 0x0c3fffff, 0, workram);

	ide_controller_init(0, &gsan_ide);
	if (medal)
		ticket_dispenser_init(100, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);

	rtc_clock[RTC_SECOND] = date->tm_sec;
	rtc_clock[RTC_MINUTE] = date->tm_min;
	rtc_clock[RTC_HOUR] = date->tm_hour;
	rtc_clock[RTC_WEEKDAY] = date->tm_wday;
	rtc_clock[RTC_DAY] = date->tm_mday;
	rtc_clock[RTC_MONTH] = date->tm_mon + 1;
	rtc_clock[RTC_YEAR] = date->tm_year % 100;
	timer_pulse(TIME_IN_SEC(1), 0, rtc_tick);

	state_save_register_global_pointer(vram, 0x400000);
	state_save_register_global_pointer(ymzram, memory_region_length(REGION_SOUND1));
	state_save_register_global_array(gpuregs);
	state_save_register_global(dbmode);
	state_save_register_global(fg16bit);
	state_save_register_global(bg16bit);
	state_save_register_global(rend16bit);
	state_save_register_global(width1024);
	state_save_register_global(rsae);
	state_save_register_global(vbkem);
	state_save_register_global(xo);
	state_save_register_global(yo);
	state_save_register_global(uxmin);
	state_save_register_global(uxmax);
	state_save_register_global(uymin);
	state_save_register_global(uymax);
	state_save_register_global(sxmax);
	state_save_register_global(symax);
	state_save_register_global(portc_data);
	state_save_register_global(porte_data);
	state_save_register_global_array(rtc_nvdata);
	state_save_register_global_array(rtc_clock);
	state_save_register_global(rtc_mode);
	state_save_register_global(rtc_sysr);
	state_save_register_global(rtc_cntrl1);
	state_save_register_global(rtc_cntrl2);
	state_save_register_global(rtc_bitstream);
	state_save_register_global(rtc_stream_pos);
	state_save_register_global(rtc_read_latch);
	state_save_register_global(rtc_pins);
}

ROM_START( ddrkids )
	ROM_REGION( 0x800000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_REGION32_BE( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gqan4-a.u17", 0x00000, 0x10000, CRC(f1346f33) SHA1(8e5d3fb64fb6e320bbde8e4cbde0689ad176a94e) )

	ROM_REGION( 0x0f, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "nvram.u9", 0x00, 0x0f, CRC(96a2e20b) SHA1(e857d915b1ddcb34f4dfb63b1cd743a439776009) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "gqan4_b-005", 0, SHA1(6f9b190e06607766dea348f22f536aa1eb1336b5) )
ROM_END

ROM_START( musclhit )
	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_REGION32_BE( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gsan5-a.u17", 0x00000, 0x10000, CRC(6ae1d1e8) SHA1(3224e4b8198aa38c094088456281cbd62c085407) )

	ROM_REGION( 0x0f, REGION_USER1, 0 )
	ROM_LOAD( "nvram.u9", 0x00, 0x0f, CRC(17614a6a) SHA1(f4714659937e7dd3eedc18bbedc4b3000134df16) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "gsan6_a-213", 0, SHA1(d9e7a350428d1621fc70e81561390c01837a94c0) )
ROM_END

ROM_START( runpuppy )
	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_ERASE00 )
	ROM_REGION32_BE( 0x10000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "gsan1-a.u17", 0x00000, 0x08000, CRC(515da4bf) SHA1(72062296077db26d6bd1bc47556c2af00d5952e6) )

	ROM_REGION( 0x0f, REGION_USER1, 0 )
	ROM_LOAD( "nvram.u9", 0x00, 0x0f, CRC(907eb7d3) SHA1(bdbe3618a2c6dd3fb66f8e4c0226c5d827e38d67) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "an10311003", 0, SHA1(5f972e29c201cdd6697f25140b37a11f02b605f5) )
ROM_END

GAME( 2000, ddrkids,       0, gsan,     ddrkids, gsan, ROT0, "Konami",       "Dance Dance Revolution Kids (GQAN4 JAA)", GAME_IMPERFECT_GRAPHICS|GAME_SUPPORTS_SAVE )
GAME( 2000, musclhit,      0, gs_medal, muscl,   gsan, ROT0, "Konami / TBS", "Muscle Ranking Kinniku Banzuke Spray Hitter", GAME_IMPERFECT_GRAPHICS|GAME_SUPPORTS_SAVE )
GAME( 2000, runpuppy,      0, gs_medal, runpuppy,gsan, ROT0, "Konami",       "Run Run Puppy", GAME_IMPERFECT_GRAPHICS|GAME_SUPPORTS_SAVE )
