// license:LGPL-2.1+
// copyright-holders:Angelo Salese, Tomasz Slanina, David Haywood, Luca Elia, Morten Shearman Kirkegaard
/*
Gunpey
Banpresto, 2000

The hardware looks very Raizing/8ing -ish, especially the ROM stickers and PCB silk-screening
which are identical to those on Brave Blade and Battle Bakraid ;-)

PCB Layout
----------

VG-10
|-------------------------------------|
|        M6295  ROM5                  |
|        YMZ280B-F      ROM4   ROM3   |
|  YAC516      16.93MHz               |
|                       61256  ROM1   |
|                       61256  ROM2   |
|J                                    |
|A             PAL       XILINX       |
|M                       XC95108      |
|M             57.2424MHz             |
|A                            V30     |
|                                     |
|              |-------|              |
|              |AXELL  |              |
|       DSW1   |AG-1   |              |
|       DSW2   |AX51101|  T2316162    |
|              |-------|  T2316162    |
|-------------------------------------|

Notes:
						V30 clock: 14.3106MHz (= 57.2424 / 4)
						YMZ280B clock: 16.93MHz
						OKI M6295 clock: 2.11625MHz (= 16.93 / 8), sample rate = clock / 165
						VSync: 60Hz
						HSync: 15.79kHz

						ROMs:
											GP_ROM1.021 \
											GP_ROM2.022 / 27C040, Main program

											GP_ROM3.025 \
											GP_ROM4.525 / SOP44 32M MASK, Graphics

											GP_ROM5.622   SOP44 32M MASK, OKI samples
*/

#include "driver.h"
#include "sound/okim6295.h"
#include "sound/ymz280b.h"
#include <stdbool.h>
typedef UINT8 u8;
typedef UINT16 u16;
typedef UINT32 u32;
typedef INT16 s16;
static u8 *mainram,*m_vram,*m_blit_rom;
static mame_bitmap *render_bitmap;
static u16 m_vram_bank,m_vreg_addr;
static u8 m_blit_ram[16],m_irq_cause,m_irq_mask;
static int m_srcx,m_srcxbase,m_srcxcount,m_srcy,m_srcycount,m_ysize,m_xsize;
static int m_dstx,m_dsty,m_dstxbase,m_dstxcount,m_dstycount;
static mame_timer *m_blitter_end_timer;
static u32 get_video_addr(u16 x,u16 y) { return (x&0x7ff)|((u32)(y&0x7ff)<<11); }
static u16 wram_word(unsigned i) { i=(i&0x7fff)*2;return mainram[i]|(mainram[i+1]<<8); }
typedef struct state_s
{
	unsigned char *buf;
	unsigned char bits;
	int num_bits;
	unsigned char colour[16];
	int basex;
	int ix, iy, iw;
	int dx, dy;
	int ow, oh;
	int ox, oy;
} state_s;

typedef struct huffman_node_s
{
	const char *bits;
	void (*func)(state_s *, unsigned char, int);
	int arg0_bits;
	int arg1_val;
} huffman_node_s;

static void draw_pixel(mame_bitmap *,const rectangle *,int,int,int,int,u16,int,u8);
static int get_next_bit(state_s *s);
static void irq_check(u8 type);
static void draw_pixel_clut(mame_bitmap *bitmap, const rectangle *cliprect, int x, int xi, int y, int yi, u8 pix, int color, int alpha, u8 scene_gradient)
{
	color += ((u32)(pix));

	const u32 col_offs = get_video_addr((color & 0xff) << 1, (color >> 8));
	const u16 color_data = (m_vram[col_offs]) | (m_vram[col_offs + 1] << 8);

	draw_pixel(bitmap, cliprect, x, xi, y, yi, color_data, alpha, scene_gradient);
}

static void draw_pixel(mame_bitmap *bitmap, const rectangle *cliprect, int x, int xi, int y, int yi, u16 color_data, int alpha, u8 scene_gradient)
{
	if (!(color_data & 0x8000))
	{
		if (scene_gradient & 0x40)
		{
			s16 r = ((color_data>>10)&31);
			s16 g = ((color_data>>5)&31);
			s16 b = (color_data&31);
			r = MAX(0, r - (scene_gradient & 0x1f));
			g = MAX(0, g - (scene_gradient & 0x1f));
			b = MAX(0, b - (scene_gradient & 0x1f));

			color_data = (color_data & 0x8000) | (r << 10) | (g << 5) | (b << 0);
		}

		if ((x+xi>=cliprect->min_x && x+xi<=cliprect->max_x && y+yi>=cliprect->min_y && y+yi<=cliprect->max_y))
		{
			if (alpha == 0x00)
				((UINT16*)bitmap->line[y + yi])[x + xi] = color_data & 0x7fff;
			else
				((UINT16*)bitmap->line[y + yi])[x + xi] = alpha_blend_r16(color_data, ((UINT16*)bitmap->line[y + yi])[x + xi], alpha) & 0x7fff;
		}
	}
}

static bool draw_gfx(mame_bitmap *bitmap, const rectangle *cliprect, int count, u8 scene_gradient)
{
	const int ZOOM_SHIFT = 15;

	if (!(wram_word(count + 0) & 1))
	{
		int x                = (wram_word(count + 3) >> 8) | ((wram_word(count + 4) & 0x03) << 8);
		int y                = (wram_word(count + 4) >> 8) | ((wram_word(count + 4) & 0x30) << 4);
		const u32 zoomheight = (wram_word(count + 5) >> 8);
		const u32 zoomwidth  = (wram_word(count + 5) & 0xff);
		const int bpp_sel    = (wram_word(count + 0) & 0x18);
		int color            = (wram_word(count + 0) >> 8);

		x -= 0x160;
		y -= 0x188;

		const u32 sourcewidth  = (wram_word(count + 6) & 0xff) << ZOOM_SHIFT;
		const u32 sourceheight = (wram_word(count + 7) >> 8) << ZOOM_SHIFT;
		const int xsource = ((wram_word(count + 2) & 0x003f) << 5) | ((wram_word(count + 1) & 0xf800) >> 11);
		const int ysource = ((wram_word(count + 3) & 0x001f) << 6) | ((wram_word(count + 2) & 0xfc00) >> 10);

		int alpha = (wram_word(count + 1) & 0x1f) << 3;

		u32 widthstep = 1 << ZOOM_SHIFT;
		u32 heightstep = 1 << ZOOM_SHIFT;

		if (zoomwidth) widthstep = sourcewidth / zoomwidth;
		if (zoomheight) heightstep = sourceheight / zoomheight;

		if (bpp_sel == 0x00)
		{
			color <<= 4;
			int ysourceoff = 0;
			for (int yi = 0; yi < zoomheight; yi++)
			{
				const int yi2 = ysourceoff >> ZOOM_SHIFT;
				int xsourceoff = 0;

				for (int xi = 0; xi < zoomwidth; xi++)
				{
					const int xi2 = xsourceoff >> ZOOM_SHIFT;

					const u8 data = m_vram[get_video_addr(xsource + (xi2 >> 1), ysource + yi2)];

					draw_pixel_clut(bitmap, cliprect, x, xi, y, yi, ((data>>((xi2&1)<<2))&15), color, alpha, scene_gradient);
					xsourceoff += widthstep;
				}
				ysourceoff += heightstep;
			}
		}
		else if (bpp_sel == 0x08)
		{

			logerror("6bpp\n");
		}
		else if (bpp_sel == 0x10)
		{
			color <<= 8;
			int ysourceoff = 0;
			for (int yi = 0; yi < zoomheight; yi++)
			{
				const int yi2 = ysourceoff >> ZOOM_SHIFT;
				int xsourceoff = 0;

				for (int xi = 0; xi < zoomwidth; xi++)
				{
					const int xi2 = xsourceoff >> ZOOM_SHIFT;

					const u8 data = m_vram[get_video_addr(xsource + xi2, ysource + yi2)];

					draw_pixel_clut(bitmap, cliprect, x, xi, y, yi, data, color, alpha, scene_gradient);
					xsourceoff += widthstep;
				}
				ysourceoff += heightstep;
			}
		}
		else if (bpp_sel == 0x18)
		{

			logerror("32k\n");

		}
	}

	return wram_word(count + 0) & 0x80;
}

static u32 screen_update(mame_bitmap *bitmap, const rectangle *cliprect)
{

	const u16 vram_bank = m_vram_bank & 0x7fff;
	const u16 vreg_addr = m_vreg_addr & 0x7fff;

	fillbitmap(bitmap,0,cliprect);

	if ((!(m_vreg_addr & 0x8000)) || (!(m_vram_bank & 0x8000)))
		return 0;

	for (int scene_index = vreg_addr / 2; scene_index < (vreg_addr + 0x400) / 2; scene_index += 0x10 / 2)
	{
		const u16 start_offs = (vram_bank + (wram_word(scene_index + 5) << 8)) / 2;
		const u16 end_offs = (vram_bank + (wram_word(scene_index + 5) << 8) + 0x1000) / 2;
		const bool scene_end_mark = wram_word(scene_index + 0) & 0x80;
		const bool scene_enabled = wram_word(scene_index + 0) & 0x01;
		const u8 scene_gradient = wram_word(scene_index + 1) & 0xff;

		if (scene_enabled)
		{
			for (int count = start_offs; count < end_offs; count += 0x10 / 2)
			{
				if (draw_gfx(bitmap, cliprect, count, scene_gradient))
					break;
			}
		}

		if (scene_end_mark)
			break;
	}

	return 0;
}

static void irq_check(u8 irq_type)
{
	m_irq_cause |= irq_type;

	if (m_irq_cause & m_irq_mask)
		cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0x200/4);
	else
		cpunum_set_input_line_and_vector(0,0,CLEAR_LINE,0x200/4);
}

static WRITE8_HANDLER( status_w )
{
	if (offset == 1)
	{
		m_irq_cause &= ~data;
		irq_check(0);
	}

	if (offset == 0)
	{
		m_irq_mask = data;
		irq_check(0);
	}
}

static READ8_HANDLER( status_r )
{
	if (offset == 1)
		return m_irq_cause;

	return m_irq_mask;
}

static READ8_HANDLER( inputs_r )
{
	switch (offset + 0x7f40)
	{
		case 0x7f40: return readinputportbytag("DSW1");
		case 0x7f41: return readinputportbytag("DSW2");
		case 0x7f42: return readinputportbytag("P1");
		case 0x7f43: return readinputportbytag("P2");
		case 0x7f44: return readinputportbytag("SYSTEM");
	}

	return 0xff;
}

static void blitter_end(int param)
{
	irq_check(4);
}

static bool write_dest_byte(u8 usedata)
{

	m_vram[get_video_addr(m_dstx, m_dsty)] = usedata;

	m_dstx++; m_dstxcount++;
	if (m_dstxcount == m_xsize)
	{
		m_dstxcount = 0;
		m_dstx = m_dstxbase;
		m_dsty++; m_dstycount++;
		if (m_dstycount == m_ysize)
			return true;
	}

	return false;
}

static u8 get_vrom_byte(int x, int y)
{
	return m_blit_rom[get_video_addr(x, y) & (0x400000 - 1)];
}

static int get_next_bit(state_s *s)
{
	if (s->num_bits == 0)
	{
		if (s->ix >= s->basex + s->iw)
		{
			s->ix = s->basex;
			s->iy++;
		}
		s->bits = get_vrom_byte(s->ix, s->iy);
		s->ix++;
		s->num_bits = 8;
	}

	int res = 1 & s->bits;

	s->bits >>= 1;
	s->num_bits--;

	return res;
}

static void set_o(state_s *s,unsigned char v)
{
	assert(s->ox >= 0);
	assert(s->ox < s->ow);
	assert(s->ox < 256);
	assert(s->oy >= 0);
	assert(s->oy < s->oh);
	assert(s->oy < 256);

	unsigned char a = v;
	for (int i = 0; i < 16; i++)
	{
		unsigned char b = s->colour[i];
		s->colour[i] = a;
		a = b;
		if (a == v)
			break;
	}

	s->buf[get_video_addr(s->dx + (s->ox++), s->dy + s->oy)] = v;
}

static unsigned char get_o(state_s *s,int x,int y)
{
	assert(x >= 0);
	assert(x < s->ow);
	assert(x < 256);
	assert(y >= 0);
	assert(y < s->oh);
	assert(y < 256);

	return s->buf[get_video_addr(s->dx + x, s->dy + y)];
}

static void hn_bytes_new_colour(state_s *s, unsigned char v, int n)
{
	for (int i = 0; i < n; i++)
		set_o(s,v);
}

static void hn_bytes_prev_colour(state_s *s, unsigned char v, int n)
{
	int c = s->colour[v];

	for (int i = 0; i < n; i++)
		set_o(s,c);
}

static void hn_copy_directly(state_s *s, unsigned char v, int n)
{
	for (int i = 0; i < n; i++)
	{
		if ((s->ox / 12) & 1)
			set_o(s,get_o(s,s->ox, s->oy + 1));
		else
			set_o(s,get_o(s,s->ox, s->oy - 1));
	}
}

static void hn_copy_plus_one(state_s *s, unsigned char v, int n)
{
	for (int i = 0; i < n; i++)
	{
		if ((s->ox / 12) & 1)
			set_o(s,get_o(s,s->ox + 1, s->oy + 1));
		else
			set_o(s,get_o(s,s->ox + 1, s->oy - 1));
	}
}

static void hn_copy_minus_one(state_s *s, unsigned char v, int n)
{
	for (int i = 0; i < n; i++)
	{
		if ((s->ox / 12) & 1)
			set_o(s,get_o(s,s->ox - 1, s->oy + 1));
		else
			set_o(s,get_o(s,s->ox - 1, s->oy - 1));
	}
}

static const huffman_node_s hn[] = {
	{ "11",                 hn_bytes_new_colour,   8,  1 },
	{ "10111",              hn_bytes_new_colour,   8,  2 },
	{ "10110011",           hn_bytes_new_colour,   8,  3 },
	{ "1010011000",         hn_bytes_new_colour,   8,  4 },
	{ "1010101010000",      hn_bytes_new_colour,   8,  5 },
	{ "101010001011110",    hn_bytes_new_colour,   8,  6 },
	{ "101010010101110",    hn_bytes_new_colour,   8,  7 },
	{ "101010010101111",    hn_bytes_new_colour,   8,  8 },
	{ "101010010101100",    hn_bytes_new_colour,   8,  9 },
	{ "10101000101100",     hn_bytes_new_colour,   8, 10 },
	{ "101010010101101",    hn_bytes_new_colour,   8, 11 },
	{ "10101000101101",     hn_bytes_new_colour,   8, 12 },
	{ "0",                  hn_bytes_prev_colour,  4,  1 },
	{ "100",                hn_bytes_prev_colour,  4,  2 },
	{ "101011",             hn_bytes_prev_colour,  4,  3 },
	{ "1010010",            hn_bytes_prev_colour,  4,  4 },
	{ "101010100",          hn_bytes_prev_colour,  4,  5 },
	{ "1010100100",         hn_bytes_prev_colour,  4,  6 },
	{ "10101010110",        hn_bytes_prev_colour,  4,  7 },
	{ "10100111000",        hn_bytes_prev_colour,  4,  8 },
	{ "101010101001",       hn_bytes_prev_colour,  4,  9 },
	{ "101001111000",       hn_bytes_prev_colour,  4, 10 },
	{ "101010010100",       hn_bytes_prev_colour,  4, 11 },
	{ "1010011001",         hn_bytes_prev_colour,  4, 12 },
	{ "101101",             hn_copy_directly,      0,  2 },
	{ "10101011",           hn_copy_directly,      0,  3 },
	{ "101010011",          hn_copy_directly,      0,  4 },
	{ "101001101",          hn_copy_directly,      0,  5 },
	{ "1010011111",         hn_copy_directly,      0,  6 },
	{ "1010100011",         hn_copy_directly,      0,  7 },
	{ "10101000100",        hn_copy_directly,      0,  8 },
	{ "101010101111",       hn_copy_directly,      0,  9 },
	{ "101001110010",       hn_copy_directly,      0, 10 },
	{ "1010011100111",      hn_copy_directly,      0, 11 },
	{ "101100101",          hn_copy_directly,      0, 12 },
	{ "1011000",            hn_copy_plus_one,      0,  2 },
	{ "101010000",          hn_copy_plus_one,      0,  3 },
	{ "10101001011",        hn_copy_plus_one,      0,  4 },
	{ "101010001010",       hn_copy_plus_one,      0,  5 },
	{ "1010101011101",      hn_copy_plus_one,      0,  6 },
	{ "1010101011100",      hn_copy_plus_one,      0,  7 },
	{ "1010011110010",      hn_copy_plus_one,      0,  8 },
	{ "10101000101110",     hn_copy_plus_one,      0,  9 },
	{ "101010001011111",    hn_copy_plus_one,      0, 10 },
	{ "1010011110011",      hn_copy_plus_one,      0, 11 },
	{ "101000",             hn_copy_minus_one,     0,  2 },
	{ "101100100",          hn_copy_minus_one,     0,  3 },
	{ "1010011101",         hn_copy_minus_one,     0,  4 },
	{ "10101010101",        hn_copy_minus_one,     0,  5 },
	{ "101001111011",       hn_copy_minus_one,     0,  6 },
	{ "101001111010",       hn_copy_minus_one,     0,  7 },
	{ "1010101010001",      hn_copy_minus_one,     0,  8 },
	{ "1010011100110",      hn_copy_minus_one,     0,  9 },
	{ "10101001010101",     hn_copy_minus_one,     0, 10 },
	{ "10101001010100",     hn_copy_minus_one,     0, 11 },
	{ NULL,              NULL,                        0,  0 },
};

static bool next_node(const huffman_node_s **res, state_s *s)
{
	char bits[128];

	memset(bits, 0, sizeof(bits));

	for (int i = 0; i < sizeof(bits) - 1; i++)
	{
		bits[i] = '0' + get_next_bit(s);

		for (int j = 0;; j++)
		{
			if (!hn[j].bits)
				return true;

			if (strncmp(bits, hn[j].bits, i + 1) == 0)
			{
				if (!hn[j].bits[i + 1])
				{
					*res = &hn[j];
					return false;
				}
				break;
			}
		}
	}

	return true;
}

static bool decompress_sprite(unsigned char *buf, int ix, int iy, int ow, int oh, int dx, int dy)
{
	const huffman_node_s *n;
	state_s s;
	unsigned char v;
	int eol;

	s.buf = buf;
	s.num_bits = 0;

	memset(s.colour, 0, sizeof(s.colour));

	s.basex = ix;

	s.ix = ix;
	s.iy = iy;
	s.iw = get_vrom_byte(s.ix++, s.iy) + 1;
	s.dx = dx;
	s.dy = dy;

	s.ow = ow;
	s.oh = oh;
	s.ox = 0;
	s.oy = 0;

	int temp;
	temp = get_next_bit(&s);
	temp |= get_next_bit(&s) << 1;
	temp |= get_next_bit(&s) << 2;
	assert(temp == 0);

	while (s.ox < s.ow)
	{
		if (next_node(&n, &s))
			return true;

		v = 0;
		for (int i = 0; i < n->arg0_bits; i++)
			v |= get_next_bit(&s) << i;

		n->func(&s, v, n->arg1_val);

		if ((s.ox % 12) == 0)
		{
			s.ox -= 12;
			if ((s.ox / 12) & 1)
				s.oy -= 1;
			else
				s.oy += 1;

			eol = 1;
		}
		else if (s.ox == s.ow)
		{
			s.ox -= s.ow % 12;
			if ((s.ox / 12) & 1)
				s.oy -= 1;
			else
				s.oy += 1;

			eol = 1;
		}
		else
			eol = 0;

		if (eol)
		{
			if ((s.oy == s.oh) || (s.oy == -1))
			{
				s.ox += 12;
				if ((s.ox / 12) & 1)
					s.oy -= 1;
				else
					s.oy += 1;
			}
		}
	}

	return false;
}

static WRITE8_HANDLER( blitter_w )
{

	m_blit_ram[offset] = data;

	if ((offset == 0) && (data == 2))
	{
		m_srcx = m_blit_ram[0x04] | (m_blit_ram[0x05] << 8);
		m_srcy = m_blit_ram[0x06] | (m_blit_ram[0x07] << 8);
		m_dstx = m_blit_ram[0x08] | (m_blit_ram[0x09] << 8);
		m_dsty = m_blit_ram[0x0a] | (m_blit_ram[0x0b] << 8);
		m_xsize = m_blit_ram[0x0c] + 1;
		m_ysize = m_blit_ram[0x0e] + 1;
		int compression = m_blit_ram[0x01];

		m_dstx <<= 1;
		m_xsize <<= 1;

		if (compression)
		{
			if (compression == 8)
			{
				if (decompress_sprite(m_vram, m_srcx, m_srcy, m_xsize, m_ysize, m_dstx, m_dsty))
					logerror("[-] Failed to decompress sprite at %04x %04x\n", m_srcx, m_srcy);
			}
			else
				logerror("unknown compression mode %02x\n",compression);
		}
		else
		{
			m_dstxbase = m_dstx;
			m_dstxcount = 0;
			m_dstycount = 0;
			m_srcxbase = m_srcx;
			m_srcxcount = 0;
			m_srcycount = 0;

			for (;;)
			{
				u8 usedata = m_blit_rom[get_video_addr(m_srcx, m_srcy)];
				m_srcx++; m_srcxcount++;
				if (m_srcxcount == m_xsize)
				{
					m_srcxcount = 0;
					m_srcx = m_srcxbase;
					m_srcy++; m_srcycount++;
				}

				if (write_dest_byte(usedata))
					break;
			}
		}

		timer_adjust(m_blitter_end_timer,TIME_IN_SEC((double)(m_xsize*m_ysize)/(57242400/4)),0,0);
	}
}

static WRITE8_HANDLER( blitter_upper_w )
{

}

static WRITE8_HANDLER( blitter_upper2_w )
{

}

static WRITE8_HANDLER( output_w )
{

	OKIM6295_set_bank_base(0,((data & 0x70) >> 4)*0x40000);
}

static WRITE8_HANDLER( vram_bank_w )
{
	m_vram_bank = (offset&1) ? ((m_vram_bank&0xff)|(data<<8)) : ((m_vram_bank&0xff00)|data);
}

static WRITE8_HANDLER( vregs_addr_w )
{
	m_vreg_addr = (offset&1) ? ((m_vreg_addr&0xff)|(data<<8)) : ((m_vreg_addr&0xff00)|data);
}

static u8 ymz_register, ymz_read_latch, ymz_read_enable;
static u32 ymz_address_hi, ymz_address_mid, ymz_address;

static WRITE8_HANDLER( gunpey_ymz_w )
{
	if (!(offset&1))
	{
		ymz_register=data;
		YMZ280B_register_0_w(0,data);
		return;
	}
	switch (ymz_register)
	{
		case 0x84: ymz_address_hi=data<<16; break;
		case 0x85: ymz_address_mid=data<<8; break;
		case 0x86:
			ymz_address=ymz_address_hi|ymz_address_mid|data;
			if (ymz_read_enable) ymz_read_latch=memory_region(REGION_SOUND2)[ymz_address&0x3fffff];
			break;
		case 0xff: ymz_read_enable=data&0x40; break;
	}
	YMZ280B_data_0_w(0,data);
}

static READ8_HANDLER( gunpey_ymz_r )
{
	if (!(offset&1))
	{
		u8 data;
		if (!ymz_read_enable) return 0xff;
		data=ymz_read_latch;
		ymz_read_latch=memory_region(REGION_SOUND2)[ymz_address&0x3fffff];
		ymz_address=(ymz_address+1)&0xffffff;
		return data;
	}
	else
	{
		u8 data;
		YMZ280B_register_0_w(0,0);
		data=YMZ280B_status_0_r(0);
		YMZ280B_register_0_w(0,ymz_register);
		return data;
	}
}

static VIDEO_START( gunpey )
{
	m_vram=auto_malloc(0x400000);
	memset(m_vram,0xff,0x400000);
	m_blit_rom=memory_region(REGION_GFX1);
	render_bitmap=auto_bitmap_alloc_depth(320,240,16);
	m_blitter_end_timer=timer_alloc(blitter_end);
	return render_bitmap ? 0 : 1;
}
static MACHINE_RESET( gunpey )
{
	m_irq_cause=m_irq_mask=0;m_vram_bank=m_vreg_addr=0;
	ymz_register=ymz_read_latch=ymz_read_enable=0;
	ymz_address_hi=ymz_address_mid=ymz_address=0;
	memset(m_blit_ram,0,sizeof(m_blit_ram));
	memset(m_vram,0xff,0x400000);
	timer_adjust(m_blitter_end_timer,TIME_NEVER,0,0);
	OKIM6295_set_bank_base(0,0);
}
static PALETTE_INIT( gunpey )
{
	int i;
	for(i=0;i<32768;i++) palette_set_color(i,pal5bit(i>>10),pal5bit(i>>5),pal5bit(i));
}
static VIDEO_UPDATE( gunpey )
{
	int x,y;
	screen_update(render_bitmap,cliprect);
	for(y=cliprect->min_y;y<=cliprect->max_y;y++)
		for(x=cliprect->min_x;x<=cliprect->max_x;x++)
			plot_pixel(bitmap,x,y,Machine->pens[((UINT16*)render_bitmap->line[y])[x]]);
}
static INTERRUPT_GEN( gunpey_interrupt ) { irq_check(0x50); }
static ADDRESS_MAP_START(mem_map,ADDRESS_SPACE_PROGRAM,8)
	AM_RANGE(0x00000,0x0ffff) AM_RAM AM_BASE(&mainram)
	AM_RANGE(0x80000,0xfffff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START(io_map,ADDRESS_SPACE_IO,8)
	AM_RANGE(0x7f40,0x7f45) AM_READ(inputs_r)
	AM_RANGE(0x7f48,0x7f48) AM_WRITE(output_w)
	AM_RANGE(0x7f80,0x7f81) AM_READWRITE(gunpey_ymz_r,gunpey_ymz_w)
	AM_RANGE(0x7f88,0x7f88) AM_READWRITE(OKIM6295_status_0_r,OKIM6295_data_0_w)
	AM_RANGE(0x7fc8,0x7fc9) AM_READWRITE(status_r,status_w)
	AM_RANGE(0x7fd0,0x7fdf) AM_WRITE(blitter_w)
	AM_RANGE(0x7fe0,0x7fe5) AM_WRITE(blitter_upper_w)
	AM_RANGE(0x7ff0,0x7ff5) AM_WRITE(blitter_upper2_w)
	AM_RANGE(0x7fec,0x7fed) AM_WRITE(vregs_addr_w)
	AM_RANGE(0x7fee,0x7fef) AM_WRITE(vram_bank_w)
ADDRESS_MAP_END
static INPUT_PORTS_START( gunpey )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty (vs. mode)" )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x30, 0x00, "Matches (vs. mode)?" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1        ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2        ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3        ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1        ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2        ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3        ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static const struct YMZ280Binterface ymz_interface={REGION_SOUND2,0,0,0};
static MACHINE_DRIVER_START( gunpey )
	MDRV_CPU_ADD(V30,57242400/4)
	MDRV_CPU_PROGRAM_MAP(mem_map,0)
	MDRV_CPU_IO_MAP(io_map,0)
	MDRV_CPU_VBLANK_INT(gunpey_interrupt,1)
	MDRV_FRAMES_PER_SECOND((57242400.0/8)/(442*264))
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(gunpey)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(320,240)
	MDRV_VISIBLE_AREA(0,319,0,239)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_PALETTE_INIT(gunpey)
	MDRV_VIDEO_START(gunpey)
	MDRV_VIDEO_UPDATE(gunpey)
	MDRV_SPEAKER_STANDARD_STEREO("left","right")
	MDRV_SOUND_ADD(OKIM6295,16934400/8/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"left",0.125)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"right",0.125)
	MDRV_SOUND_ADD(YMZ280B,16934400)
	MDRV_SOUND_CONFIG(ymz_interface)
	MDRV_SOUND_ROUTE(0,"left",0.25)
	MDRV_SOUND_ROUTE(1,"right",0.25)
MACHINE_DRIVER_END
ROM_START( gunpey )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "gp_rom1.021",  0x00000, 0x80000, CRC(07a589a7) SHA1(06c4140ffd5f74b3d3ddfc424f43fcd08d903490) )
	ROM_LOAD16_BYTE( "gp_rom2.022",  0x00001, 0x80000, CRC(f66bc4cf) SHA1(54931d878d228c535b9e2bf22a0a3e41756f0fe5) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD( "gp_rom3.025",  0x00000, 0x400000,  CRC(f2d1f9f0) SHA1(0d20301fd33892074508b9d127456eae80cc3a1c) )

	ROM_REGION( 0x400000, REGION_SOUND2, 0 )
	ROM_LOAD( "gp_rom4.525",  0x000000, 0x400000, CRC(78dd1521) SHA1(91d2046c60e3db348f29f776def02e3ef889f2c1) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "gp_rom5.622",  0x000000, 0x400000,  CRC(f79903e0) SHA1(4fd50b4138e64a48ec1504eb8cd172a229e0e965))
ROM_END

GAME(2000,gunpey,0,gunpey,gunpey,0,ROT0,"Bandai / Banpresto","Gunpey (Japan)",0)
