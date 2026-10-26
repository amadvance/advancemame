// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Miodrag Milanovic, Carl
/**********************************************************************

	Intel 82720 / NEC uPD7220 Graphics Display Controller emulation

**********************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "i82720.h"
#include "i82720cm.h"

enum
{
	COMMAND_INVALID = -1,
	COMMAND_RESET,
	COMMAND_SYNC,
	COMMAND_VSYNC,
	COMMAND_CCHAR,
	COMMAND_START,
	COMMAND_BCTRL,
	COMMAND_ZOOM,
	COMMAND_CURS,
	COMMAND_PRAM,
	COMMAND_PITCH,
	COMMAND_WDAT,
	COMMAND_MASK,
	COMMAND_FIGS,
	COMMAND_FIGD,
	COMMAND_GCHRD,
	COMMAND_RDAT,
	COMMAND_CURD,
	COMMAND_LPRD,
	COMMAND_DMAR,
	COMMAND_DMAW,
	COMMAND_5A,
	COMMAND_05
};

enum
{
	FIFO_READ = 0,
	FIFO_WRITE
};

enum
{
	FIFO_EMPTY = -1,
	FIFO_PARAMETER,
	FIFO_COMMAND
};

static const int x_dir[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
static const int y_dir[8] = { 1, 1, 0,-1,-1,-1, 0, 1};

static struct
{
	UINT16 mask;
	UINT8 pitch;
	UINT32 ead;
	UINT16 dad;
	UINT32 lad;

	UINT8 ra[16];
	int ra_addr;

	UINT8 sr;
	UINT8 cr;
	UINT8 pr[17];
	int param_ptr;

	UINT8 fifo[16];
	int fifo_flag[16];
	int fifo_ptr;
	int fifo_dir;

	UINT8 mode;

	int de;
	int m;
	int aw;
	int al;
	int vs;
	int vfp;
	int vbp;
	int hs;
	int hfp;
	int hbp;

	int dc;
	int sc;
	int br;
	int ctop;
	int cbot;
	int lr;

	int disp;
	int gchr;

	UINT8 bitmap_mod;

	struct {
		UINT8 dir;
		UINT8 figure_type;
		UINT16 dc;
		UINT8 gd;
		UINT16 d;
		UINT16 d1;
		UINT16 d2;
		UINT16 dm;
	} figs;

	UINT8 *vram;
	UINT32 vram_size;
	UINT32 vram_mask;
} gdc;

static UINT8 g_unk_video = 0x04;
static compis_gdc_interface g_intf;

static const unsigned char compis_palette[16*3] =
{
	0, 0, 0,
	33, 200, 66,
	33, 200, 66,
	94, 220, 120,
	84, 85, 237,
	125, 118, 252,
	212, 82, 77,
	66, 235, 245,
	252, 85, 84,
	255, 121, 120,
	212, 193, 84,
	230, 206, 128,
	33, 176, 59,
	201, 91, 186,
	204, 204, 204,
	255, 255, 255
};

PALETTE_INIT( compis_gdc )
{
	palette_set_colors(0, compis_palette, COMPIS_PALETTE_SIZE);
}

INLINE UINT8 vram_read_byte(offs_t address)
{
	if (!gdc.vram) return 0;
	return gdc.vram[address & gdc.vram_mask];
}

INLINE void vram_write_byte(offs_t address, UINT8 data)
{
	if (!gdc.vram) return;
	gdc.vram[address & gdc.vram_mask] = data;
}

INLINE UINT16 vram_read_word(offs_t address)
{
	if (!gdc.vram) return 0;
	return gdc.vram[address & gdc.vram_mask] | (gdc.vram[(address + 1) & gdc.vram_mask] << 8);
}

INLINE void vram_write_word(offs_t address, UINT16 data)
{
	if (!gdc.vram) return;
	gdc.vram[address & gdc.vram_mask] = data & 0xff;
	gdc.vram[(address + 1) & gdc.vram_mask] = (data >> 8) & 0xff;
}

INLINE void fifo_clear(void)
{
	int i;
	for (i = 0; i < 16; i++)
	{
		gdc.fifo[i] = 0;
		gdc.fifo_flag[i] = FIFO_EMPTY;
	}

	gdc.fifo_ptr = -1;

	gdc.sr &= ~UPD7220_SR_DATA_READY;
	gdc.sr |= UPD7220_SR_FIFO_EMPTY;
	gdc.sr &= ~UPD7220_SR_FIFO_FULL;
}

INLINE int fifo_param_count(void)
{
	int i;
	for (i = 0; i < 16; i++)
	{
		if (gdc.fifo_flag[i] != FIFO_PARAMETER) break;
	}
	return i;
}

INLINE void fifo_set_direction(int dir)
{
	if (gdc.fifo_dir != dir)
	{
		fifo_clear();
	}
	gdc.fifo_dir = dir;
}

INLINE void queue(UINT8 data, int flag)
{
	if (gdc.fifo_ptr < 15)
	{
		gdc.fifo_ptr++;
		gdc.fifo[gdc.fifo_ptr] = data;
		gdc.fifo_flag[gdc.fifo_ptr] = flag;

		if (gdc.fifo_ptr == 15)
		{
			gdc.sr |= UPD7220_SR_FIFO_FULL;
		}

		gdc.sr &= ~UPD7220_SR_FIFO_EMPTY;
	}
}

INLINE void dequeue(UINT8 *data, int *flag)
{
	*data = gdc.fifo[0];
	*flag = gdc.fifo_flag[0];

	if (gdc.fifo_ptr > -1)
	{
		int i;
		for (i = 0; i < 15; i++)
		{
			gdc.fifo[i] = gdc.fifo[i + 1];
			gdc.fifo_flag[i] = gdc.fifo_flag[i + 1];
		}

		gdc.fifo[15] = 0;
		gdc.fifo_flag[15] = 0;

		gdc.fifo_ptr--;

		if (gdc.fifo_ptr <= 0)
			gdc.sr |= UPD7220_SR_FIFO_EMPTY;
		if (gdc.fifo_ptr == -1)
			gdc.sr &= ~UPD7220_SR_DATA_READY;
	}
}

INLINE void reset_figs_param(void)
{
	gdc.figs.dc = 0x0000;
	gdc.figs.d = 0x0008;
	gdc.figs.d1 = 0x0008;
	gdc.figs.d2 = 0x0000;
	gdc.figs.dm = 0x0000;
	gdc.figs.gd = 0;
}

INLINE void read_vram(UINT8 type, UINT8 mod)
{
	if (type == 1) return;

	while (gdc.figs.dc && gdc.fifo_ptr < (type ? 15 : 14))
	{
		switch (type)
		{
			case 0:
				queue(vram_read_byte(gdc.ead * 2), 0);
				queue(vram_read_byte(gdc.ead * 2 + 1), 0);
				break;
			case 2:
				queue(vram_read_byte(gdc.ead * 2), 0);
				break;
			case 3:
				queue(vram_read_byte(gdc.ead * 2 + 1), 0);
				break;
		}

		gdc.figs.dc--;
		gdc.ead += x_dir[gdc.figs.dir] + (y_dir[gdc.figs.dir] * gdc.pitch);
		gdc.ead &= 0x3ffff;
	}

	if (gdc.figs.dc == 0)
		reset_figs_param();
}

INLINE void write_vram(UINT8 type, UINT8 mod)
{
	UINT16 result;
	int i;

	if (type == 1) return;

	result = gdc.pr[1] | (gdc.pr[2] << 8);

	switch (type)
	{
		case 0:
			result &= gdc.mask;
			break;
		case 2:
			result &= (gdc.mask & 0xff);
			break;
		case 3:
			result <<= 8;
			result &= (gdc.mask & 0xff00);
			break;
	}

	for (i = 0; i < gdc.figs.dc + 1; i++)
	{
		switch (mod & 3)
		{
			case 0x00: // replace
				if (type == 0)
					vram_write_word(gdc.ead * 2 + 0, result);
				if (type == 2)
					vram_write_byte(gdc.ead * 2 + 0, result & 0xff);
				if (type == 3)
					vram_write_byte(gdc.ead * 2 + 1, result >> 8);
				break;
			case 0x01: // complement
				if (type == 0)
					vram_write_word(gdc.ead * 2 + 0, vram_read_word(gdc.ead * 2 + 0) ^ result);
				if (type == 2)
					vram_write_byte(gdc.ead * 2 + 0, vram_read_byte(gdc.ead * 2 + 0) ^ (result & 0xff));
				if (type == 3)
					vram_write_byte(gdc.ead * 2 + 1, vram_read_byte(gdc.ead * 2 + 1) ^ (result >> 8));
				break;
			case 0x02: // reset to zero
				if (type == 0)
					vram_write_word(gdc.ead * 2 + 0, vram_read_word(gdc.ead * 2 + 0) & ~result);
				if (type == 2)
					vram_write_byte(gdc.ead * 2 + 0, vram_read_byte(gdc.ead * 2 + 0) & ~(result & 0xff));
				if (type == 3)
					vram_write_byte(gdc.ead * 2 + 1, vram_read_byte(gdc.ead * 2 + 1) & ~(result >> 8));
				break;
			case 0x03: // set to one
				if (type == 0)
					vram_write_word(gdc.ead * 2 + 0, vram_read_word(gdc.ead * 2 + 0) | result);
				if (type == 2)
					vram_write_byte(gdc.ead * 2 + 0, vram_read_byte(gdc.ead * 2 + 0) | (result & 0xff));
				if (type == 3)
					vram_write_byte(gdc.ead * 2 + 1, vram_read_byte(gdc.ead * 2 + 1) | (result >> 8));
				break;
		}

		gdc.ead += x_dir[gdc.figs.dir] + (y_dir[gdc.figs.dir] * gdc.pitch);
		gdc.ead &= 0x3ffff;
	}
}

INLINE void get_text_partition(int index, UINT32 *sad, UINT16 *len, int *im, int *wd)
{
	*sad = ((gdc.ra[(index * 4) + 1] & 0x1f) << 8) | gdc.ra[(index * 4) + 0];
	*len = ((gdc.ra[(index * 4) + 3] & 0x3f) << 4) | (gdc.ra[(index * 4) + 2] >> 4);
	*im = (gdc.ra[(index * 4) + 3] >> 6) & 1;
	*wd = (gdc.ra[(index * 4) + 3] >> 7) & 1;
}

INLINE void get_graphics_partition(int index, UINT32 *sad, UINT16 *len, int *im, int *wd)
{
	*sad = ((gdc.ra[(index * 4) + 2] & 0x03) << 16) | (gdc.ra[(index * 4) + 1] << 8) | gdc.ra[(index * 4) + 0];
	*len = ((gdc.ra[(index * 4) + 3] & 0x3f) << 4) | (gdc.ra[(index * 4) + 2] >> 4);
	*im = (gdc.ra[(index * 4) + 3] >> 6) & 1;
	*wd = (gdc.ra[(index * 4) + 3] >> 7) & 1;
}

static void draw_pixel(int x, int y, int xi, UINT16 tile_data)
{
	UINT32 addr = ((y * (gdc.pitch << (gdc.figs.gd ? 0 : 1))) + (x >> 3)) & 0x3ffff;
	UINT16 data = vram_read_word(addr);
	UINT16 new_pixel = (tile_data & (1 << (xi & 0xf))) ? (1 << (x & 0xf)) : 0;

	switch (gdc.bitmap_mod)
	{
		case 0: // replace
			vram_write_word(addr, (data & ~(1 << (x & 0xf))) | new_pixel);
			break;
		case 1: // complement
			vram_write_word(addr, data ^ new_pixel);
			break;
		case 2: // reset
			vram_write_word(addr, data & ~new_pixel);
			break;
		case 3: // set
			vram_write_word(addr, data | new_pixel);
			break;
	}
}

static void draw_line(int x, int y)
{
	int xi, yi;
	int d = (gdc.figs.d & 0x2000) ? (INT16)(gdc.figs.d | 0xe000) : gdc.figs.d;
	int d2 = (gdc.figs.d2 & 0x2000) ? (INT16)(gdc.figs.d2 | 0xe000) : gdc.figs.d2;
	UINT16 pattern = (gdc.ra[8]) | (gdc.ra[9] << 8);
	const int dot_dir[4] = {1, -1, -1, 1};

	for (yi = xi = 0; yi <= gdc.figs.dc; yi++)
	{
		switch (gdc.figs.dir & 3)
		{
			case 1:
			case 2:
				draw_pixel(yi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3] + x, xi * dot_dir[gdc.figs.dir >> 1] + y, yi, pattern);
				break;
			default:
				draw_pixel(xi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3] + x, yi * dot_dir[gdc.figs.dir >> 1] + y, yi, pattern);
				break;
		}
		if (d > 0)
		{
			xi++;
			d += d2;
		}
		else
		{
			d += gdc.figs.d1;
		}
	}

	switch (gdc.figs.dir & 3)
	{
		case 1:
		case 2:
			x += yi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3];
			y += xi * dot_dir[gdc.figs.dir >> 1];
			break;
		default:
			x += xi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3];
			y += yi * dot_dir[gdc.figs.dir >> 1];
			break;
	}

	gdc.ead = (x >> 4) + (y * (gdc.pitch >> gdc.figs.gd));
	gdc.dad = x & 0x0f;
}

static void draw_arc(int x, int y)
{
	int xi = gdc.figs.d + 1, yi = 0, err = -gdc.figs.d;
	int x0, y0, i;
	UINT16 pattern = (gdc.ra[8]) | (gdc.ra[9] << 8);
	const int dot_dir[4] = {1, -1, -1, 1};

	switch (gdc.figs.dir & 3)
	{
		case 1:
		case 2:
			x0 = x;
			y0 = y + xi * dot_dir[gdc.figs.dir >> 1];
			break;
		default:
			x0 = x + xi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3];
			y0 = y;
			break;
	}

	for (i = 0; i <= gdc.figs.dc; i++)
	{
		if (i >= gdc.figs.dm)
		{
			switch (gdc.figs.dir & 3)
			{
				case 1:
				case 2:
					draw_pixel(yi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3] + x0, xi * dot_dir[gdc.figs.dir >> 1] + y0, i, pattern);
					break;
				default:
					draw_pixel(xi * dot_dir[gdc.figs.dir >> 1] + x0, yi * dot_dir[((gdc.figs.dir >> 1) + 3) & 3] + y0, i, pattern);
					break;
			}
		}
		yi++;
		if (err < 0)
		{
			err += (yi + 1) << 1;
		}
		else
		{
			xi--;
			err += (yi - xi + 1) << 1;
		}
	}
	switch (gdc.figs.dir & 3)
	{
		case 1:
		case 2:
			x += (gdc.figs.dc + 1) * dot_dir[((gdc.figs.dir >> 1) + 3) & 3];
			break;
		default:
			y += (gdc.figs.dc + 1) * dot_dir[gdc.figs.dir >> 1];
			break;
	}

	gdc.ead = (x >> 4) + (y * (gdc.pitch >> gdc.figs.gd));
	gdc.dad = x & 0x0f;
}

static void draw_rectangle(int x, int y)
{
	int i;
	const int rect_x_dir[8] = { 0, 1, 0,-1, 1, 1,-1,-1 };
	const int rect_y_dir[8] = { 1, 0,-1, 0, 1,-1,-1, 1 };
	UINT8 rect_type, rect_dir;
	UINT16 pattern = (gdc.ra[8]) | (gdc.ra[9] << 8);

	rect_type = (gdc.figs.dir & 1) << 2;
	rect_dir = rect_type | (((gdc.figs.dir >> 1) + 0) & 3);

	for (i = 0; i < gdc.figs.d; i++)
	{
		draw_pixel(x, y, i, pattern);
		x += rect_x_dir[rect_dir];
		y += rect_y_dir[rect_dir];
	}

	rect_dir = rect_type | (((gdc.figs.dir >> 1) + 1) & 3);
	for (i = 0; i < gdc.figs.d2; i++)
	{
		draw_pixel(x, y, i, pattern);
		x += rect_x_dir[rect_dir];
		y += rect_y_dir[rect_dir];
	}

	rect_dir = rect_type | (((gdc.figs.dir >> 1) + 2) & 3);
	for (i = 0; i < gdc.figs.d; i++)
	{
		draw_pixel(x, y, i, pattern);
		x += rect_x_dir[rect_dir];
		y += rect_y_dir[rect_dir];
	}

	rect_dir = rect_type | (((gdc.figs.dir >> 1) + 3) & 3);
	for (i = 0; i < gdc.figs.d2; i++)
	{
		draw_pixel(x, y, i, pattern);
		x += rect_x_dir[rect_dir];
		y += rect_y_dir[rect_dir];
	}

	gdc.ead = (x >> 4) + (y * (gdc.pitch >> gdc.figs.gd));
	gdc.dad = x & 0x0f;
}

static void draw_char(int x, int y)
{
	int isize, psize, pi, pz, ii, iz, curpixel;
	UINT16 tile_data;

	isize = gdc.figs.d & 0x3ff;
	psize = ((gdc.figs.d & 0x400) + gdc.figs.dc) + 1;

	for (pi = 0; pi < psize; pi++)
	{
		tile_data = (gdc.ra[((psize - 1 - pi) & 7) | 8] << 8) | gdc.ra[((psize - 1 - pi) & 7) | 8];
		for (pz = 0; pz <= gdc.gchr; pz++)
		{
			for (ii = 0, curpixel = 0; ii < isize; ii++)
			{
				for (iz = 0; iz <= gdc.gchr; iz++)
				{
					draw_pixel(x + (curpixel * x_dir[gdc.figs.dir]), y + (curpixel * y_dir[gdc.figs.dir]), ii, tile_data);
					curpixel++;
				}
			}
			if (gdc.figs.figure_type == 2)
			{
				x += x_dir[(gdc.figs.dir + 2) & 7];
				y += y_dir[(gdc.figs.dir + 2) & 7];
			}
			else
			{
				x += x_dir[(gdc.figs.dir + 1) & 7];
				y += y_dir[(gdc.figs.dir + 1) & 7];
			}
		}
	}

	gdc.ead = (x >> 4) + (y * (gdc.pitch >> gdc.figs.gd));
	gdc.dad = (x & 0xf);
}

static int translate_command(UINT8 data)
{
	int command = COMMAND_INVALID;

	switch (data)
	{
		case UPD7220_COMMAND_RESET: command = COMMAND_RESET; break;
		case UPD7220_COMMAND_CCHAR: command = COMMAND_CCHAR; break;
		case UPD7220_COMMAND_START: command = COMMAND_START; break;
		case UPD7220_COMMAND_ZOOM:  command = COMMAND_ZOOM;  break;
		case UPD7220_COMMAND_CURS:  command = COMMAND_CURS;  break;
		case UPD7220_COMMAND_PITCH: command = COMMAND_PITCH; break;
		case UPD7220_COMMAND_MASK:  command = COMMAND_MASK;  break;
		case UPD7220_COMMAND_FIGS:  command = COMMAND_FIGS;  break;
		case UPD7220_COMMAND_FIGD:  command = COMMAND_FIGD;  break;
		case UPD7220_COMMAND_GCHRD: command = COMMAND_GCHRD; break;
		case UPD7220_COMMAND_CURD:  command = COMMAND_CURD;  break;
		case UPD7220_COMMAND_LPRD:  command = COMMAND_LPRD;  break;
		case UPD7220_COMMAND_5A:    command = COMMAND_5A;    break;
		case UPD7220_COMMAND_05:    command = COMMAND_05;    break;
		default:
			switch (data & 0xfe)
			{
				case UPD7220_COMMAND_SYNC:  command = COMMAND_SYNC;  break;
				case UPD7220_COMMAND_VSYNC: command = COMMAND_VSYNC; break;
				case UPD7220_COMMAND_BCTRL: command = COMMAND_BCTRL; break;
				default:
					switch (data & 0xf0)
					{
						case UPD7220_COMMAND_PRAM: command = COMMAND_PRAM; break;
						default:
							switch (data & 0xe4)
							{
								case UPD7220_COMMAND_WDAT: command = COMMAND_WDAT; break;
								case UPD7220_COMMAND_RDAT: command = COMMAND_RDAT; break;
								case UPD7220_COMMAND_DMAR: command = COMMAND_DMAR; break;
								case UPD7220_COMMAND_DMAW: command = COMMAND_DMAW; break;
							}
					}
			}
	}

	return command;
}

static void process_fifo(void)
{
	UINT8 data;
	int flag;
	UINT16 eff_pitch;

	if (gdc.fifo_ptr < 0) return;

	eff_pitch = gdc.pitch >> gdc.figs.gd;
	dequeue(&data, &flag);

	if (flag == FIFO_COMMAND)
	{
		gdc.cr = data;
		gdc.param_ptr = 1;
	}
	else
	{
		gdc.pr[gdc.param_ptr] = data;
		gdc.param_ptr++;
	}

	switch (translate_command(gdc.cr))
	{
		case COMMAND_INVALID:
			break;

		case COMMAND_5A:
			break;

		case COMMAND_RESET:
			switch (gdc.param_ptr)
			{
				case 0:
					gdc.de = 0;
					gdc.ra[0] = gdc.ra[1] = gdc.ra[2] = 0;
					gdc.ra[3] = 0x19;
					gdc.ead = 0;
					gdc.dad = 0;
					gdc.mask = 0;
					break;

				case 9:
					gdc.mode = gdc.pr[1];
					gdc.aw = gdc.pr[2] + 2;
					gdc.hs = (gdc.pr[3] & 0x1f) + 1;
					gdc.vs = ((gdc.pr[4] & 0x03) << 3) | (gdc.pr[3] >> 5);
					gdc.hfp = (gdc.pr[4] >> 2) + 1;
					gdc.hbp = (gdc.pr[5] & 0x3f) + 1;
					gdc.vfp = gdc.pr[6] & 0x3f;
					gdc.al = ((gdc.pr[8] & 0x03) << 8) | gdc.pr[7];
					gdc.vbp = gdc.pr[8] >> 2;
					gdc.pitch = gdc.aw;
					break;
			}
			break;

		case COMMAND_SYNC:
			if (gdc.param_ptr == 9)
			{
				gdc.mode = gdc.pr[1];
				gdc.aw = gdc.pr[2] + 2;
				gdc.hs = (gdc.pr[3] & 0x1f) + 1;
				gdc.vs = ((gdc.pr[4] & 0x03) << 3) | (gdc.pr[3] >> 5);
				gdc.hfp = (gdc.pr[4] >> 2) + 1;
				gdc.hbp = (gdc.pr[5] & 0x3f) + 1;
				gdc.vfp = gdc.pr[6] & 0x3f;
				gdc.al = ((gdc.pr[8] & 0x03) << 8) | gdc.pr[7];
				gdc.vbp = gdc.pr[8] >> 2;
				gdc.pitch = gdc.aw;
			}
			break;

		case COMMAND_VSYNC:
			gdc.m = gdc.cr & 0x01;
			break;

		case COMMAND_CCHAR:
			if (gdc.param_ptr == 2)
			{
				gdc.lr = (gdc.pr[1] & 0x1f) + 1;
				gdc.dc = (gdc.pr[1] >> 7) & 1;
			}
			if (gdc.param_ptr == 3)
			{
				gdc.ctop = gdc.pr[2] & 0x1f;
				gdc.sc = (gdc.pr[2] >> 5) & 1;
				gdc.br = (gdc.pr[2] >> 6);
			}
			if (gdc.param_ptr == 4)
			{
				gdc.br = ((gdc.pr[3] & 0x07) << 2) | (gdc.pr[2] >> 6);
				gdc.cbot = gdc.pr[3] >> 3;
			}
			break;

		case COMMAND_START:
			gdc.de = 1;
			break;

		case COMMAND_05:
			gdc.de = 0;
			break;

		case COMMAND_BCTRL:
			gdc.de = gdc.cr & 0x01;
			break;

		case COMMAND_ZOOM:
			if (flag == FIFO_PARAMETER)
			{
				gdc.gchr = gdc.pr[1] & 0x0f;
				gdc.disp = gdc.pr[1] >> 4;
			}
			break;

		case COMMAND_CURS:
			if (gdc.param_ptr >= 3)
			{
				UINT8 upper_addr = (gdc.param_ptr == 3) ? 0 : (gdc.pr[3] & 0x03);
				gdc.ead = (upper_addr << 16) | (gdc.pr[2] << 8) | gdc.pr[1];

				if (gdc.param_ptr == 4)
				{
					gdc.dad = gdc.pr[3] >> 4;
				}
			}
			break;

		case COMMAND_PRAM:
			if (flag == FIFO_COMMAND)
			{
				gdc.ra_addr = gdc.cr & 0x0f;
			}
			else
			{
				if (gdc.ra_addr < 16)
				{
					gdc.ra[gdc.ra_addr] = data;
					gdc.ra_addr++;
				}
				gdc.param_ptr = 0;
			}
			break;

		case COMMAND_PITCH:
			if (flag == FIFO_PARAMETER)
			{
				gdc.pitch = data;
			}
			break;

		case COMMAND_WDAT:
			gdc.bitmap_mod = gdc.cr & 3;
			if (gdc.param_ptr == 3 || (gdc.param_ptr == 2 && (gdc.cr & 0x10)))
			{
				fifo_set_direction(FIFO_WRITE);
				write_vram((gdc.cr & 0x18) >> 3, gdc.cr & 3);
				reset_figs_param();
				gdc.param_ptr = 1;
			}
			break;

		case COMMAND_MASK:
			if (gdc.param_ptr == 3)
			{
				gdc.mask = (gdc.pr[2] << 8) | gdc.pr[1];
			}
			break;

		case COMMAND_FIGS:
			if (gdc.param_ptr == 2)
			{
				gdc.figs.dir = gdc.pr[1] & 0x7;
				gdc.figs.figure_type = (gdc.pr[1] & 0xf8) >> 3;
			}
			if (gdc.param_ptr == 3)
				gdc.figs.dc = (gdc.pr[2]) | (gdc.figs.dc & 0x3f00);
			if (gdc.param_ptr == 4)
			{
				gdc.figs.dc = (gdc.pr[2]) | ((gdc.pr[3] & 0x3f) << 8);
				gdc.figs.gd = ((gdc.pr[3] & 0x40) != 0) && ((gdc.mode & UPD7220_MODE_DISPLAY_MASK) == UPD7220_MODE_DISPLAY_MIXED);
			}
			if (gdc.param_ptr == 6)
				gdc.figs.d = (gdc.pr[4]) | ((gdc.pr[5] & 0x3f) << 8);
			if (gdc.param_ptr == 8)
				gdc.figs.d2 = (gdc.pr[6]) | ((gdc.pr[7] & 0x3f) << 8);
			if (gdc.param_ptr == 10)
				gdc.figs.d1 = (gdc.pr[8]) | ((gdc.pr[9] & 0x3f) << 8);
			if (gdc.param_ptr == 12)
				gdc.figs.dm = (gdc.pr[10]) | ((gdc.pr[11] & 0x3f) << 8);
			break;

		case COMMAND_FIGD:
			if (gdc.figs.figure_type == 0)
				draw_pixel(((gdc.ead % eff_pitch) << 4) | (gdc.dad & 0xf), (gdc.ead / eff_pitch), gdc.dad, (gdc.ra[8]) | (gdc.ra[9] << 8));
			else if (gdc.figs.figure_type == 1)
				draw_line(((gdc.ead % eff_pitch) << 4) | (gdc.dad & 0xf), (gdc.ead / eff_pitch));
			else if (gdc.figs.figure_type == 4)
				draw_arc(((gdc.ead % eff_pitch) << 4) | (gdc.dad & 0xf), (gdc.ead / eff_pitch));
			else if (gdc.figs.figure_type == 8)
				draw_rectangle(((gdc.ead % eff_pitch) << 4) | (gdc.dad & 0xf), (gdc.ead / eff_pitch));

			reset_figs_param();
			gdc.sr |= UPD7220_SR_DRAWING_IN_PROGRESS;
			break;

		case COMMAND_GCHRD:
			if ((gdc.figs.figure_type & 0xf) == 2)
				draw_char(((gdc.ead % eff_pitch) << 4) | (gdc.dad & 0xf), (gdc.ead / eff_pitch));

			reset_figs_param();
			gdc.sr |= UPD7220_SR_DRAWING_IN_PROGRESS;
			break;

		case COMMAND_RDAT:
			fifo_set_direction(FIFO_READ);
			read_vram((gdc.cr & 0x18) >> 3, gdc.cr & 3);
			gdc.sr |= UPD7220_SR_DATA_READY;
			break;

		case COMMAND_CURD:
		{
			UINT16 dad = 1 << gdc.dad;
			fifo_set_direction(FIFO_READ);
			queue(gdc.ead & 0xff, 0);
			queue((gdc.ead >> 8) & 0xff, 0);
			queue((gdc.ead >> 16) & 0xff, 0);
			queue(dad & 0xff, 0);
			queue(dad >> 8, 0);
			gdc.sr |= UPD7220_SR_DATA_READY;
			break;
		}

		case COMMAND_LPRD:
			fifo_set_direction(FIFO_READ);
			queue(gdc.lad & 0xff, 0);
			queue((gdc.lad >> 8) & 0xff, 0);
			queue((gdc.lad >> 16) & 0xff, 0);
			gdc.sr |= UPD7220_SR_DATA_READY;
			gdc.sr &= ~UPD7220_SR_LIGHT_PEN_DETECT;
			break;

		case COMMAND_DMAR:
		case COMMAND_DMAW:
			break;
	}
}

static void continue_command(void)
{
	if (gdc.figs.dc && translate_command(gdc.cr) == COMMAND_RDAT)
	{
		read_vram((gdc.cr & 0x18) >> 3, gdc.cr & 3);
		gdc.sr |= UPD7220_SR_DATA_READY;
	}
}

UINT8 i82720_r(int offset)
{
	UINT8 data;

	if (offset & 1)
	{
		int flag;
		fifo_set_direction(FIFO_READ);
		dequeue(&data, &flag);
		continue_command();
	}
	else
	{
		data = gdc.sr;
		gdc.sr &= ~UPD7220_SR_DRAWING_IN_PROGRESS;
		gdc.sr &= ~UPD7220_SR_DMA_EXECUTE;
	}

	return data;
}

void i82720_w(int offset, UINT8 data)
{
	if (offset & 1)
	{
		fifo_set_direction(FIFO_WRITE);
		queue(data, 1);
	}
	else
	{
		queue(data, 0);
	}

	process_fifo();
}

READ8_HANDLER( compis_gdc_r )
{
	switch (offset & 0x0e)
	{
		case 0x00:
			return i82720_r(0);

		case 0x02:
			return i82720_r(1);

		case 0x04:
			switch (g_unk_video)
			{
				case 0x04: g_unk_video = 0x44; break;
				case 0x44: g_unk_video = 0x64; break;
				default:   g_unk_video = 0x04; break;
			}
			return g_unk_video;

		default:
			return 0;
	}
}

WRITE8_HANDLER( compis_gdc_w )
{
	switch (offset & 0x0e)
	{
		case 0x00:
			i82720_w(0, data);
			break;

		case 0x02:
			i82720_w(1, data);
			break;

		default:
			break;
	}
}

void compis_gdc_vblank_int(void)
{
	gdc.sr |= UPD7220_SR_VSYNC_ACTIVE;
}

void compis_gdc_reset(void)
{
	memset(&gdc, 0, sizeof(gdc));
	gdc.sr = UPD7220_SR_FIFO_EMPTY;
	gdc.fifo_ptr = -1;
	g_unk_video = 0x04;
	fifo_clear();
}

static void draw_graphics_line(mame_bitmap *bitmap, UINT32 addr, int y, int wd, int pitch)
{
	int sx;

	if (y < 0 || y >= bitmap->height)
		return;

	for (sx = 0; sx < pitch; sx++)
	{
		if ((sx << 4) < gdc.aw * 16)
		{
			int px = sx << 4;
			UINT16 gfx = vram_read_word(addr);
			int i;

			for (i = 0; i < 16; i++)
			{
				int x = px + i;
				if (x >= 0 && x < bitmap->width)
				{
					plot_pixel(bitmap, x, y, (gfx & (1 << i)) ? 1 : 0);
				}
			}
		}

		addr += (wd + 1) * 2;
	}
}

static void update_graphics(mame_bitmap *bitmap, const rectangle *cliprect, int force_bitmap)
{
	UINT32 addr, sad;
	UINT16 len;
	int im, wd, area;
	int y, bsy = 0;
	int mixed = ((gdc.mode & UPD7220_MODE_DISPLAY_MASK) == UPD7220_MODE_DISPLAY_MIXED);
	UINT8 interlace = ((gdc.mode & UPD7220_MODE_INTERLACE_MASK) == UPD7220_MODE_INTERLACE_ON) ? 0 : 1;

	for (area = 0; area < 4; area++)
	{
		get_graphics_partition(area, &sad, &len, &im, &wd);

		if (im || force_bitmap)
		{
			if (area >= 3)
				break;

			if (!interlace)
				len <<= 1;

			for (y = 0; y < len; y++)
			{
				int line_y = y + bsy + gdc.vbp;
				int lr_div = mixed ? 1 : (gdc.lr ? gdc.lr : 1);
				addr = ((sad << 1) & 0x3ffff) + ((y / lr_div) * (gdc.pitch << (im ? 0 : 1)));

				draw_graphics_line(bitmap, addr, line_y, wd, (gdc.pitch << interlace));
			}
		}

		bsy += len;
	}
}

VIDEO_UPDATE( compis_gdc )
{
	fillbitmap(bitmap, 0, cliprect);

	if (gdc.de)
	{
		switch (gdc.mode & UPD7220_MODE_DISPLAY_MASK)
		{
			case UPD7220_MODE_DISPLAY_MIXED:
				update_graphics(bitmap, cliprect, 0);
				break;

			case UPD7220_MODE_DISPLAY_GRAPHICS:
				update_graphics(bitmap, cliprect, 1);
				break;

			default:
				update_graphics(bitmap, cliprect, 1);
				break;
		}
	}
}

VIDEO_START( compis_gdc )
{
	UINT32 vramsize = g_intf.vramsize ? g_intf.vramsize : 0x8000;

	gdc.vram = auto_malloc(vramsize);
	if (!gdc.vram)
		return 1;

	memset(gdc.vram, 0, vramsize);
	gdc.vram_size = vramsize;
	gdc.vram_mask = vramsize - 1;

	compis_gdc_reset();

	return 0;
}

void mdrv_compisgdc(machine_config *machine, const compis_gdc_interface *intf)
{
	int scr_width = (intf->mode == GDC_MODE_UHRG) ? 1280 : 640;
	int scr_height = (intf->mode == GDC_MODE_UHRG) ? 800 : 400;

	g_intf = *intf;

	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(scr_width, scr_height)
	MDRV_VISIBLE_AREA(0, scr_width - 1, 0, scr_height - 1)
	MDRV_PALETTE_LENGTH(COMPIS_PALETTE_SIZE)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT(compis_gdc)

	MDRV_VIDEO_START(compis_gdc)
	MDRV_VIDEO_UPDATE(compis_gdc)
}
