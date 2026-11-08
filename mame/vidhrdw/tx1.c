// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Tatsumi TX-1/Buggy Boy hardware

***************************************************************************/
#include "driver.h"
#include "vidhrdw/res_net.h"
#include "cpu/i86/i86.h"
#include "includes/tx1.h"

#define CURSOR_YPOS 239
#define CURSOR_XPOS 168
#define PRINT_CRTC_DATA 0

static mame_timer *interrupt_timer;

static void interrupt_callback(int param)
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xff);
	timer_adjust(interrupt_timer, cpu_getscanlinetime(CURSOR_YPOS), 0, 0);
}

READ16_HANDLER( tx1_crtc_r )
{
	return 0xffff;
}

WRITE16_HANDLER( tx1_crtc_w )
{
if (PRINT_CRTC_DATA)
{
	data &= 0xff;
	if (offset == 0)
	{
		switch (data)
		{
			case 0x00: logerror("Horizontal Total         "); break;
			case 0x01: logerror("Horizontal displayed     "); break;
			case 0x02: logerror("Horizontal sync position "); break;
			case 0x03: logerror("Horizontal sync width    "); break;
			case 0x04: logerror("Vertical total           "); break;
			case 0x05: logerror("Vertical total adjust    "); break;
			case 0x06: logerror("Vertical displayed       "); break;
			case 0x07: logerror("Vertical sync position   "); break;
			case 0x08: logerror("Interlace mode           "); break;
			case 0x09: logerror("Max. scan line address   "); break;
			case 0x0a: logerror("Cursror start            "); break;
			case 0x0b: logerror("Cursor end               "); break;
			case 0x0c: logerror("Start address (h)        "); break;
			case 0x0d: logerror("Start address (l)        "); break;
			case 0x0e: logerror("Cursor (h)               "); break;
			case 0x0f: logerror("Cursor (l)               "); break;
			case 0x10: logerror("Light pen (h))           "); break;
			case 0x11: logerror("Light pen (l)            "); break;
		}
	}
	else if (offset == 1)
	{
		logerror("0x%.2x, (%d)\n",data, data);
	}
}
}

enum
{
	TX1_RDFLAG_RVA8 = 0,
	TX1_RDFLAG_RVA9,
	TX1_RDFLAG_RVA7,
	TX1_RDFLAG_TNLF,
	TX1_RDFLAG_STLF,
	TX1_RDFLAG_SCCHGF
};

static struct
{
	UINT16	scol;
	UINT32  slock;
	UINT8	flags;

	UINT32	ba_val;
	UINT32	ba_inc;
	UINT32	bank_mode;

	UINT16	h_val;
	UINT16	h_inc;

	UINT8	slin_val;
	UINT8	slin_inc;
} tx1_vregs;

UINT16 *tx1_vram;
UINT16 *tx1_objram;
UINT16 *tx1_rcram;
size_t tx1_objram_size;

static UINT8 *tx1_chr_bmp;
static UINT8 *tx1_obj_bmp;
static UINT8 *tx1_rod_bmp;




PALETTE_INIT( tx1 )
{
	int i;

	for (i = 0; i < 256; ++i)
	{
		int r = color_prom[i + 0x300] & 15;
		int g = color_prom[i + 0x400] & 15;
		int b = color_prom[i + 0x500] & 15;
		palette_set_color(i, r * 17, g * 17, b * 17);
	}
}

WRITE16_HANDLER( tx1_bankcs_w )
{
	offset <<= 1;

	if (offset & 0x04)
	{
		tx1_vregs.ba_inc &= ~0x0000ffff;
		tx1_vregs.ba_inc |= data;

		if (!(offset & 2))
			tx1_vregs.ba_val &= ~0x000ffff;
	}
	if (offset & 0x08)
	{
		data &= 0xff;
		tx1_vregs.ba_inc &= ~0xffff0000;
		tx1_vregs.ba_inc |= data << 16;

		tx1_vregs.bank_mode = BIT(data, 1);

		if (!(offset & 2))
			tx1_vregs.ba_val &= ~0xffff0000;
	}
	if ( !(offset & 0x10) )
	{

		if (offset & 2)
			tx1_vregs.ba_val = (tx1_vregs.ba_inc + tx1_vregs.ba_val) & 0x00ffffff;
	}
	if (offset & 0x20)
	{
		tx1_vregs.h_inc = data;

		if ( !(offset & 2) )
			tx1_vregs.h_val = 0;
	}
	if (!(offset & 0x40))
	{

			tx1_vregs.h_val += tx1_vregs.h_inc;
	}
}

WRITE16_HANDLER( tx1_slincs_w )
{
	if (offset == 1)
		tx1_vregs.slin_inc = data;
	else
		tx1_vregs.slin_inc = tx1_vregs.slin_val = 0;
}

WRITE16_HANDLER( tx1_slock_w )
{
	tx1_vregs.slock = data & 1;
}

WRITE16_HANDLER( tx1_scolst_w )
{
	tx1_vregs.scol = data & 0x0707;
}

WRITE16_HANDLER( tx1_flgcs_w )
{
	tx1_vregs.flags = data & 0xff;
}

static void tx1_draw_char(UINT8 *bitmap)
{
	INT32 x, y;
	UINT32 scroll_x;
	UINT8 *chars, *gfx2;

	chars = memory_region(REGION_GFX1);
	gfx2 = chars + 0x4000;

	scroll_x = tx1_vram[0xfff] & 0x3ff;

	for (y = 0; y < 240; ++y)
	{
		UINT32 d0 = 0, d1 = 0;
		UINT32 colour = 0;
		UINT32 y_offs;
		UINT32 x_offs;
		UINT32 y_gran;

		y_offs = y;

		if ((y_offs >= 64) && (y_offs < 128))
			x_offs = tx1_vregs.slock ? scroll_x : 0;
		else
			x_offs = 0;

		y_gran = y_offs & 7;

		if (x_offs & 7)
		{
			UINT32 tilenum;
			UINT16 ram_val = tx1_vram[((y_offs << 4) & 0xf80) + ((x_offs >> 3) & 0x7f)];

			tilenum = (ram_val & 0x03ff) | ((ram_val & 0x8000) >> 5);
			colour = (ram_val & 0xfc00) >> 8;
			d0 = *(gfx2 + (tilenum << 3) + y_gran);
			d1 = *(chars + (tilenum << 3) + y_gran);
		}

		for (x = 0; x < 256 * 3; ++x)
		{
			UINT32 x_gran = x_offs & 7;

			if (!x_gran)
			{
				UINT32 tilenum;
				UINT16 ram_val = tx1_vram[((y_offs << 4) & 0xf80) + ((x_offs >> 3) & 0x7f)];

				tilenum = (ram_val & 0x03ff) | ((ram_val & 0x8000) >> 5);
				colour = (ram_val & 0xfc00) >> 8;
				d0 = *(gfx2 + (tilenum << 3) + y_gran);
				d1 = *(chars + (tilenum << 3) + y_gran);
			}

			*bitmap++ = colour |
						(((d1 >> (7 ^ x_gran)) & 1) << 1) |
						((d0 >> (7 ^ x_gran)) & 1);

			x_offs = (x_offs + 1) & 0x3ff;
		}
	}
}

#define TX1_GET_ROADPIX(NUM) \
{ \
	UINT32 addr = (rva6_0 << 5) | (road##NUM##_hcnt & 0x1f); \
	UINT8 promaddr1, promaddr2, promaddr3; \
	promaddr1 = rom_a[addr]; \
	promaddr2 = rom_b[addr]; \
	promaddr3 = rom_c[addr]; \
	pix[NUM][0][0] = prom_a[promaddr1]; pix[NUM][0][1] = prom_b[promaddr1]; pix[NUM][0][2] = prom_c[promaddr1]; \
	pix[NUM][1][0] = prom_a[promaddr2]; pix[NUM][1][1] = prom_b[promaddr2]; pix[NUM][1][2] = prom_c[promaddr2]; \
	pix[NUM][2][0] = prom_a[promaddr3]; pix[NUM][2][1] = prom_b[promaddr3]; pix[NUM][2][2] = prom_c[promaddr3]; \
	pix[NUM][3][0] = prom_a[0];			pix[NUM][3][1] = prom_b[0];			pix[NUM][3][2] = prom_c[0]; \
}

INLINE void tx1_draw_road_pixel(int screen, UINT8 *bmpaddr,
								UINT8 apix[3], UINT8 bpix[3], UINT32 pixnuma, UINT32 pixnumb,
								UINT8 stl, UINT8 sld, UINT8 selb,
								UINT8 bnk, UINT8 rorev, UINT8 eb, UINT8 r, UINT8 delr)
{
	UINT8 a0 = BIT(apix[0], pixnuma);
	UINT8 a1 = BIT(apix[1], pixnuma);
	UINT8 a2 = BIT(apix[2], pixnuma);

	UINT8 b0 = BIT(bpix[0], pixnumb);
	UINT8 b1 = BIT(bpix[1], pixnumb);
	UINT8 b2 = BIT(bpix[2], pixnumb);

	UINT8 d3;
	UINT8 d2;
	UINT8 d1;
	UINT8 d0;
	UINT8 sel;
	UINT8 c6 = BIT(sld, 6);
	UINT8 c5 = BIT(sld, 5);
	UINT8 c4 = BIT(sld, 4);
	UINT8 c3 = BIT(sld, 2);

	UINT32 addr_offset = screen * 256;

	sel = !bnk && ( (a2 && !b0) || (!a0 && b2) || !a1 || !a2 || !b1 || !b2 );

	d3 =
		(a2 && a1 && a0 && b2 && b1 && b0 && !rorev)
		|| (a1 && b1 && !b0 && stl && !bnk && !c6)
		|| (a1 && !a0 && b1 && stl && !bnk && !c6)
		|| (a1 && !b2 && b1 && stl && !bnk && !c6)
		|| (a2 && b2 && !b1 && stl && !bnk && !c4)
		|| (a2 && !a1 && b2 && stl && !bnk && !c4)
		|| (!a2 && a1 && b2 && stl && !bnk && !c6)
		|| (a2 && !b2 && b1 && stl && !bnk && !c6)
		|| (!a2 && !a1 && stl && !bnk && !c5)
		|| (!b2 && !b1 && stl && !bnk && !c5)
		|| (!b0 && !stl && !bnk && !c3)
		|| (!b1 && !stl && !bnk && !c3)
		|| (!a0 && !stl && !bnk && !c3)
		|| (!a1 && !stl && !bnk && !c3)
		|| (!a2 && !stl && !bnk && !c3)
		|| (!b2 && !stl && !bnk && !c3)
		|| (bnk && !rorev);

	if (eb == 0)
	{
		if (sel == 1)
		{
			d2 = stl && (!a2 || !b2);
			d1 = !stl || (!a2 && !a1) || (!b1 && !b2) || (a2 && !b1) || (b2 && !a1);
			d0 = !stl
				|| (!a2 && !a0 && b1)
				|| (a1 && !b2 && !b0)
				|| (!a2 && !a1 && !a0)
				|| (!b2 && !b1 && !b0)
				|| (a2 && a1 && !b0)
				|| (!a0 && b2 && b1)
				|| (a2 && !b1 && !b0)
				|| (!a1 && !a0 && b2)
				|| (!a2 && !a0 && b2)
				|| (a2 && !b2 && !b0);
		}
		else
		{
			d2 = BIT(tx1_vregs.scol, selb ? 2 : 10);
			d1 = BIT(tx1_vregs.scol, selb ? 1 : 9);
			d0 = BIT(tx1_vregs.scol, selb ? 0 : 8);
		}
	}
	else
		d2 = d1 = d0 = 0;

	*(bmpaddr + addr_offset) = (bnk << 6) | (r << 5) | (!(sel && delr) << 4 ) | (d3 << 3) | (d2 << 2) | (d1 << 1) | d0;
}

static void tx1_draw_road(UINT8 *bitmap)
{
	INT32	y;
	UINT32	rva9_8;
	UINT32	rva7;
	UINT32	tnlf;
	UINT32	stlf;
	UINT32	scchgf;

	UINT32	vc = 0;

	UINT16	road0_hcnt;
	UINT8	road0_pcnt;
	UINT16	road1_hcnt;
	UINT8	road1_pcnt;
	UINT8	pix[2][4][3];

	const UINT8 *const gfx3 = memory_region(REGION_GFX4);
	const UINT8 *const rom_a = gfx3;
	const UINT8 *const rom_b = gfx3 + 0x2000;
	const UINT8 *const rom_c = gfx3 + 0x4000;

	const UINT8 *const proms = memory_region(REGION_PROMS);
	const UINT8 *const prom_a = proms + 0x1100;
	const UINT8 *const prom_b = proms + 0x1300;
	const UINT8 *const prom_c = proms + 0x1500;
	const UINT8 *const vprom  = proms + 0x1700;

	rva9_8	= (tx1_vregs.flags & 3) << 8;
	rva7	= !BIT(tx1_vregs.flags, TX1_RDFLAG_RVA7) << 7;
	tnlf	= BIT(tx1_vregs.flags, TX1_RDFLAG_TNLF);
	scchgf	= BIT(tx1_vregs.flags, TX1_RDFLAG_SCCHGF);
	stlf	= BIT(tx1_vregs.flags, TX1_RDFLAG_STLF);

	for (y = 0; y < 240; ++y)
	{
		UINT32	x;
		UINT8	sld;
		UINT8	rva6_0;
		UINT16	rcrdb15_0;
		UINT16	rva_addr;
		UINT32	bank_cnt;

		UINT16	vat;
		UINT16	vp0;
		UINT16	vp1;
		UINT16	vp2;
		UINT16	vp3;
		UINT16	vp4;

		UINT32	va8, va9, va10, va11, va12, va13, va14, va15;
		UINT32	v0, v1, v2;

		UINT8	hc1_u, hc1_l;
		UINT8	hc0_u, hc0_l;

		UINT32	bnkls, bnkcs, bnkrs;
		UINT32	rl, rc, rr;

		UINT32	stl;
		UINT32	selb;
		UINT32	ebls, ebcs, ebrs;

		UINT8 *bmpaddr = bitmap + (y * 768);

		UINT32 rltmp, rctmp, rrtmp;
		UINT32 eltmp, ectmp, ertmp;

		UINT32 tmpm;
		UINT32 tmpn;
		UINT32 tmpc;
		UINT32 tmpd;

		UINT8 scrcnt0;
		UINT8 scrcnt1;

		UINT8 rorevls = 0;
		UINT8 rorevcs = 0;
		UINT8 rorevrs = 0;

		int febl[2] = { 0, 0 };
		int febr[2] = { 0, 0 };
		int febc[2] = { 0, 0 };

		if (BIT(tx1_vregs.h_val, 15))
			rva6_0 = 0x7f;
		else
			rva6_0 = (~tx1_vregs.h_val >> 7) & 0x7f;

		rva_addr = rva9_8 | rva6_0;

		sld = vprom[rva6_0] + tx1_vregs.slin_val;

		rcrdb15_0 = tx1_rcram[(0x80 + rva_addr) ^ 0x7ff];
		road0_pcnt = rcrdb15_0 & 0x7;
		road0_hcnt = (rcrdb15_0 >> 3) & 0x7f;
		road0_hcnt |= rcrdb15_0 & 0xfc00 ? 0x80 : 0;
		road0_hcnt |= (rcrdb15_0 & 0xfc00) == 0xfc00 ? 0x100 : 0x000;

		rcrdb15_0 = tx1_rcram[(rva7 + rva_addr) ^ 0x7ff];
		road1_pcnt = rcrdb15_0 & 0x7;
		road1_hcnt = (rcrdb15_0 >> 3) & 0x7f;
		road1_hcnt |= rcrdb15_0 & 0xfc00 ? 0x80 : 0;
		road1_hcnt |= (rcrdb15_0 & 0xfc00) == 0xfc00 ? 0x100 : 0x000;
		road1_hcnt |= (rcrdb15_0 & 0x800);

		scrcnt0 = (road0_hcnt >> 5) & 0xf;
		scrcnt1 = (road1_hcnt >> 5) & 0xf;

		if (road0_pcnt & 7)
		{
			TX1_GET_ROADPIX(0);

			febl[0] = (scrcnt0 + 0) & 0xc;
			febc[0] = (scrcnt0 + 1) & 0xc;
			febr[0] = (scrcnt0 + 2) & 0xc;
		}

		if (road1_pcnt & 7)
		{
			UINT32 y4 = (road1_hcnt >> 4) & 1;
			UINT32 temp = (road1_hcnt >> 5) & 0xf;
			UINT32 x, u;
			UINT32 fl11 = BIT(road1_hcnt, 15);

			TX1_GET_ROADPIX(1);

			x = (temp & 0xc);
			u = !(((temp & 1) && y4) || (temp & 2));
			rorevls = ((fl11 && !x) || (fl11 && x && u));
			febl[1] = x;

			temp += 1;
			x = (temp & 0xc);
			u = !(((temp & 1) && y4) || (temp & 2));
			rorevcs = ((fl11 && !x) || (fl11 && x && u));
			febc[1] = x;

			temp += 1;
			x = (temp & 0xc);
			u = !(((temp & 1) && y4) || (temp & 2));
			rorevrs = ((fl11 && !x) || (fl11 && x && u));
			febr[1] = x;
		}

		bank_cnt = (tx1_vregs.ba_val >> 5) & 0x3ff;

		vat = tx1_rcram[(rva9_8 + 7) ^ 0x7f8];
		vp0 = tx1_rcram[(rva9_8 + 6) ^ 0x7f8];
		vp1 = tx1_rcram[(rva9_8 + 5) ^ 0x7f8];
		vp2 = tx1_rcram[(rva9_8 + 4) ^ 0x7f8];
		vp3 = tx1_rcram[(rva9_8 + 3) ^ 0x7f8];
		vp4 = tx1_rcram[(rva9_8 + 2) ^ 0x7f8];

		if (y-1 == vp0) vc++;
		if (y-1 == vp1) vc++;
		if (y-1 == vp2) vc++;
		if (y-1 == vp3) vc++;
		if (y-1 == vp4) vc++;

		va8 = BIT(vat, 8);
		va9 = BIT(vat, 9);
		va10 = BIT(vat, 10);
		va11 = BIT(vat, 11);
		va12 = BIT(vat, 12);
		va13 = BIT(vat, 13);
		va14 = BIT(vat, 14);
		va15 = BIT(vat, 15);

		v0 = BIT(vc, 0);
		v1 = BIT(vc, 1);
		v2 = BIT(vc, 2);

		hc1_u = tx1_rcram[(rva9_8 + 1) ^ 0x7f8] >> 8;
		hc1_l = tx1_rcram[(rva9_8 + 1) ^ 0x7f8] & 0xff;
		hc0_u = tx1_rcram[(rva9_8 + 0) ^ 0x7f8] >> 8;
		hc0_l = tx1_rcram[(rva9_8 + 0) ^ 0x7f8] & 0xff;

		stl = !stlf || v0 || (!v2 && !scchgf) || scchgf;
		selb = (!v2 && !scchgf) || v2;

		rltmp =
		(v2 && !v0 && !va10)
		|| (!v2 && v0 && !va10)
		|| (v2 && !v0 && !va11)
		|| (v2 && !v1 && !v0)
		|| (!v2 && v0 && !va11)
		|| (!v2 && v1 && !v0)
		|| (!v2 && !v1 && v0);

		rctmp =
		(v2 && !v0 && va9 && va8)
		|| (!v2 && v0 && va9 && va8)
		|| (v2 && !v0 && !va11)
		|| (v2 && !v1 && !v0)
		|| (!v2 &&  v0 && !va11)
		|| (!v2 &&  v1 && !v0)
		|| (!v2 && !v1 &&  v0);

		rrtmp =
		(v2 && !v0 && !va11 && !va10)
		|| (!v2 && v0 && !va11 && !va10)
		|| (v2 && !v0 && va9)
		|| (!v2 && v0 && va9)
		|| (v2 && !v1 && !v0)
		|| (!v2 && v1 && !v0)
		|| (!v2 && !v1 && v0);

		ectmp =
		(v2 && !v0 && va13 && va12)
		|| (!v2 && v1 && va13 && va12)
		|| (!v2 && !v1 && v0)
		|| (v2 && !v0 && !va15)
		|| (!v2 && v1 && !va15);

		eltmp =
		(v2 && !v0 && !va14)
		|| (!v2 && v1 && !va14)
		|| (!v2 && !v1 && v0)
		|| (v2 && !v0 && !va15)
		|| (!v2 && v1 && !va15);

		ertmp =
		(v2 && !v0 && !va15 && !va14)
		|| (!v2 && v1 && !va15 && !va14)
		|| (v2 && !v0 && va13)
		|| (!v2 && v1 && va13)
		|| (!v2 && !v1 && v0);

		tmpn = (v2 && !v0) || (!v2 && v0);
		tmpm = (v2 && !v0) || (!v2 && v1);
		tmpc = (!v1 && !v0) || (v2);
		tmpd = v1 && v0 && va11;

		for (x = 0; x < 256; x++)
		{
			UINT32 pixnum0 = (road0_pcnt & 7) ^ 7;
			UINT32 pixnum1 = (road1_pcnt & 7) ^ 7;

			UINT32 cyu, cyl;
			UINT32 delrl, delrc, delrr;

			scrcnt0 = (road0_hcnt >> 5) & 0xf;
			scrcnt1 = (road1_hcnt >> 5) & 0xf;

			if (!(road0_pcnt & 7))
			{
				TX1_GET_ROADPIX(0);

				febl[0] = (scrcnt0 + 0) & 0xc;
				febc[0] = (scrcnt0 + 1) & 0xc;
				febr[0] = (scrcnt0 + 2) & 0xc;
			}

			if (!(road1_pcnt & 7))
			{
				UINT32 y4 = (road1_hcnt >> 4) & 1;
				UINT32 temp = (road1_hcnt >> 5) & 0xf;
				UINT32 x, u;
				UINT32 fl11 = BIT(road1_hcnt, 15);

				TX1_GET_ROADPIX(1);

				x = (temp & 0xc);
				u = !(((temp & 1) && y4) || (temp & 2));
				rorevls = ((fl11 && !x) || (fl11 && x && u));
				febl[1] = x;

				temp += 1;
				x = (temp & 0xc);
				u = !(((temp & 1) && y4) || (temp & 2));
				rorevcs = ((fl11 && !x) || (fl11 && x && u));
				febc[1] = x;

				temp += 1;
				x = (temp & 0xc);
				u = !(((temp & 1) && y4) || (temp & 2));
				rorevrs = ((fl11 && !x) || (fl11 && x && u));
				febr[1] = x;
			}

			if (BIT(tx1_vregs.ba_val, 23))
			{
				bnkls = 1;
				bnkcs = 1;
				bnkrs = 1;
			}
			else if (tx1_vregs.ba_val & 0x007f8000)
			{
				bnkls = 0;
				bnkcs = 0;
				bnkrs = 0;
			}
			else
			{
				bnkls = bank_cnt < 0x400;
				bnkcs = bank_cnt < 0x300;
				bnkrs = bank_cnt < 0x200;
			}

			if (tx1_vregs.bank_mode)
			{
				bnkls ^= 1;
				bnkcs ^= 1;
				bnkrs ^= 1;
			}

			cyu = hc1_u == 0xff;
			cyl = hc1_l == 0xff;

			rl = tnlf && ( rltmp || (tmpn && (!cyl || (va8 && va9 && cyu))) );
			rc = tnlf && ( rctmp || (tmpn && ((!va10 && !cyl) || (va9 && cyu))) );
			rr = tnlf && ( rrtmp || (tmpn && ((!va11 && !cyl) || (va8 && cyu))) );

			delrl = !tnlf || tmpc || ((tmpd && va10) && ((!va8 && cyl) || (!va9 && cyl) || (!cyu && !cyl)));

			delrr =
				!tnlf
				|| tmpc
				|| (v1 && v0 && va10 && !va9 && !va8 && cyl)
				|| (v1 && v0 && va10 && !va9 && cyl && !cyu)
				|| (tmpd && !va9 && !cyu)
				|| (tmpd && !va9 && !va8);

			delrc = !tnlf || tmpc || (tmpd && ((!va8 && cyl && !cyu) || (!va8 && va10 && !cyu) || (!va9 && cyl) || (va10 && !va9)));

			cyu = hc0_u == 0xff;
			cyl = hc0_l == 0xff;

			ebls = tnlf && ( eltmp || (tmpm && (!cyl || (va12 && va13 && cyu))));
			ebcs = tnlf && ( ectmp || (tmpm && ((!va14 && !cyl) || (va13 && cyu))));
			ebrs = tnlf && ( ertmp || (tmpm && ((!va15 && !cyl) || (va12 && cyu))));

			cyu = hc0_u == 0xff;
			cyl = hc0_l == 0xff;

			if (!(bnkls && !rl))
			{
				int a, b;

				if (!febl[0])
					a = (scrcnt0) & 3;
				else
					a = 3;

				if (!febl[1])
					b = (scrcnt1) & 3;
				else
					b = 3;

				tx1_draw_road_pixel(0, bmpaddr,
						 &pix[0][a][0], &pix[1][b][0],
						 pixnum0, pixnum1,
						 stl, sld, selb, bnkls, rorevls, ebls, rl, delrl);
			}
			else
				*(bmpaddr) = (bnkls << 6) | (rl << 5);

			if (!(bnkcs && !rc))
			{
				int a, b;

				if (!febc[0])
					a = (scrcnt0 + 1) & 3;
				else
					a = 3;

				if (!febc[1])
					b = (scrcnt1 + 1) & 3;
				else
					b = 3;

				tx1_draw_road_pixel(1, bmpaddr,
						 &pix[0][a][0], &pix[1][b][0],
						 pixnum0, pixnum1,
						 stl, sld, selb, bnkcs, rorevcs, ebcs, rc, delrc);
			}
			else
				*(bmpaddr + 256) = (bnkcs << 6) | (rc << 5);

			if (!(bnkrs && !rr))
			{
				int a, b;

				if (!febr[0])
					a = (scrcnt0 + 2) & 3;
				else
					a = 3;

				if (!febr[1])
					b = (scrcnt1 + 2) & 3;
				else
					b = 3;

				tx1_draw_road_pixel(2, bmpaddr,
						 &pix[0][a][0], &pix[1][b][0],
						 pixnum0, pixnum1,
						 stl, sld, selb, bnkrs, rorevrs, ebrs, rr, delrr);
			}
			else
				*(bmpaddr + 512) = (bnkrs << 6) | (rr << 5);

			++bmpaddr;

			if (!(++road0_pcnt & 7))
				++road0_hcnt;

			if (!(++road1_pcnt & 7))
				++road1_hcnt;

			if (hc0_u != 0xff) ++hc0_u;
			if (hc0_l != 0xff) ++hc0_l;
			if (hc1_u != 0xff) ++hc1_u;
			if (hc1_l != 0xff) ++hc1_l;

			bank_cnt = (bank_cnt + 1) & 0x7ff;
		}

		tx1_vregs.h_val += tx1_vregs.h_inc;

		tx1_vregs.ba_val = (tx1_vregs.ba_val + tx1_vregs.ba_inc) & 0x00ffffff;
	}
}

static void tx1_draw_objects(UINT8 *bitmap)
{
#define FRAC	16

	UINT32 offs;

	const UINT8 *const ic48 = memory_region(REGION_USER3);
	const UINT8 *const ic281 = ic48 + 0x2000;

	const UINT8 *const proms = memory_region(REGION_PROMS);
	const UINT8 *const ic190 = proms + 0xc00;
	const UINT8 *const ic162 = proms + 0xe00;
	const UINT8 *const ic25  = proms + 0x1000;

	const UINT8 *const ic106 = memory_region(REGION_USER2);
	const UINT8 *const ic73  = ic106 + 0x4000;

	const UINT8 *const pixdata_rgn = memory_region(REGION_GFX2);

	for (offs = 0x0; offs <= tx1_objram_size; offs += 8)
	{
		UINT32	x;
		UINT32	y;
		UINT32	gxflip;

		UINT32	x_scale;
		UINT32	x_step;
		UINT16	y_scale;
		UINT16	y_step;

		UINT8	pctmp0_7;
		UINT8	code;

		if ((tx1_objram[offs] & 0xff00) == 0xff00)
			break;

		x_scale = tx1_objram[offs + 2] & 0xff;

		if (x_scale == 0)
			continue;

		y_scale = tx1_objram[offs + 1];
		y_step  = tx1_objram[offs + 3];

		code = tx1_objram[offs] & 0xff;

		pctmp0_7 = tx1_objram[offs + 2] >> 8;

		gxflip = (pctmp0_7 & 0x80) >> 7;

		y = (tx1_objram[offs] >> 8) + 1;

		for (; y < 240; ++y)
		{
			UINT32	rom_addr2	= 0;
			UINT8	ic106_data	= 0;
			UINT8	ic73_data;

			if (y_scale & 0x8000)
				break;

			{
				UINT32	psa0_11;
				UINT32	ic48_addr;
				UINT32	ic48_data;
				UINT32	rom_addr;
				UINT32	x_acc;
				UINT32	newtile = 1;
				UINT32	dataend = 0;
				UINT8	data1 = 0;
				UINT8	data2 = 0;
				UINT32	xflip = 0;
				UINT32	opcd0_7 = 0;
				UINT32	lasttile = 0;

				ic48_addr = code << 4;
				ic48_addr |= ((y_scale >> 11) & 0xf);
				ic48_data = ic48[ic48_addr];

				if (ic48_data == 0xff)
					break;

				psa0_11 = ((ic25[code] << 8) | ic48_data) & 0xfff;

				rom_addr = (psa0_11 & ~0xff) << 2;

				x_step = (128 << FRAC) / x_scale;
				x_acc = (psa0_11 & 0xff) << (FRAC + 5);

#define TX1_MASK	0xfff

				x = tx1_objram[offs + 4] & TX1_MASK;

				for (;;)
				{
					if (newtile)
					{
						UINT32	psbb0_12;
						UINT32	pscb0_14;
						UINT32	pscb11;
						UINT8	*romptr;
						UINT32	ic281_addr;
						UINT32  grom_addr;
						UINT32	lut_data;
						UINT32	low_addr = ((x_acc >> (FRAC + 3)) & TX1_MASK);

						if (gxflip)
						{
							UINT32 xor_mask;

							if (BIT(psa0_11, 11) && BIT(psa0_11, 10))
								xor_mask = 0xf;
							else if (BIT(psa0_11, 11) || BIT(psa0_11, 10) || BIT(psa0_11, 9))
								xor_mask = 0x7;
							else
								xor_mask = 0x3;

							rom_addr2 = rom_addr + (xor_mask ^ low_addr);
						}
						else
							rom_addr2 = rom_addr + low_addr;

						ic106_data = ic106[rom_addr2 & 0x3fff];

						if ((ic106_data & 0x40) && dataend)
							lasttile = 1;

						dataend |= ic106_data & 0x40;

						ic73_data = ic73[rom_addr2];

						lut_data = (ic106_data << 8) | ic73_data;
						psbb0_12 = lut_data & 0x1fff;

						pscb0_14 = (psbb0_12 & 0xc3f);

						if (BIT(psbb0_12, 12))
							pscb0_14 |= psbb0_12 & 0x3c0;
						else
							pscb0_14 |= (pctmp0_7 & 0xf) << 6;

						if (BIT(lut_data, 13))
							pscb0_14 |= BIT(psbb0_12, 10) << 12;
						else
							pscb0_14 |= ((pctmp0_7 & 0x70) << 8);

						pscb0_14 &= ~(1 << 12);
						pscb0_14 |= BIT(psbb0_12, 10) << 12;

						pscb11 = BIT(pscb0_14, 11);

						romptr = (UINT8*)(pixdata_rgn + pscb11 * (0x4000 * 2));

						grom_addr = ((pscb0_14 << 3) | ((y_scale >> 8) & 7)) & 0x3fff;

						data1 = *(grom_addr + romptr);
						data2 = *(grom_addr + romptr + 0x4000);

						xflip = gxflip ^ !BIT(lut_data, 15);

						ic281_addr = pscb0_14 & 0x3ff;
						ic281_addr |= ((pscb0_14 & 0x7000) >> 2);
						ic281_addr |= pscb11 << 13;

						opcd0_7 = ic281[ic281_addr];

						newtile = 0;
					}

					if (x < 768)
					{
						UINT8	pix;
						UINT8	bit;

						bit	= (x_acc >> FRAC) & 7;

						if (xflip)
							bit ^= 7;

						pix = (((data1 >> bit) & 1) << 1) | ((data2 >> bit) & 1);

						if ( !(!(opcd0_7 & 0x80) && !pix) )
						{
							UINT8 color;
							UINT32 prom_addr;

							prom_addr = ((opcd0_7 << 2) | pix) & 0x1ff;

							if (x & 1)
								color = ~ic190[prom_addr] & 0x3f;
							else
								color = ~ic162[prom_addr] & 0x3f;

							*(bitmap + 768*y + x) = 0x40 | color;
						}
					}

					if ((((x_acc + x_step) >> (FRAC + 3)) & TX1_MASK) != ((x_acc >> (FRAC + 3)) & TX1_MASK))
					{
						if (lasttile)
							break;

						newtile = 1;
					}

					x = (x + 1) & TX1_MASK;
					x_acc += x_step;
				}
			}
			y_scale += y_step;
		}
	}
}

VIDEO_START( tx1 )
{
	tx1_chr_bmp = auto_malloc(sizeof(UINT8) * 256 * 3 * 240);
	tx1_obj_bmp = auto_malloc(sizeof(UINT8) * 256 * 3 * 240);
	tx1_rod_bmp = auto_malloc(sizeof(UINT8) * 256 * 3 * 240);

	interrupt_timer = timer_alloc(interrupt_callback);

	timer_adjust(interrupt_timer, cpu_getscanlinetime(CURSOR_YPOS), 0, 0);
	return 0;
}

VIDEO_EOF( tx1 )
{
	tx1_vregs.slin_val += tx1_vregs.slin_inc;
}

static void tx1_combine_layers(mame_bitmap *bitmap, int screen)
{
	int x, y;
	UINT8 *chr_pal = memory_region(REGION_PROMS) + 0x900;

	int x_offset = screen * 256;

	for (y = 0; y < 240; ++y)
	{
		UINT16 *bmp_addr = ((UINT16 *)bitmap->line[y]) + x_offset;

		UINT32 bmp_offset = y * 768 + x_offset;

		UINT8 *chr_addr = tx1_chr_bmp + bmp_offset;
		UINT8 *rod_addr = tx1_rod_bmp + bmp_offset;
		UINT8 *obj_addr = tx1_obj_bmp + bmp_offset;

		for (x = 0; x < 256; ++x)
		{
			UINT8 out_val;
			UINT32 char_val = chr_addr[x];
			UINT32 c7 = BIT(char_val, 7);
			UINT32 c1 = BIT(char_val, 1);
			UINT32 c0 = BIT(char_val, 0);

			UINT32 road_val = rod_addr[x];
			UINT32 r6 = BIT(road_val, 6);
			UINT32 r5 = BIT(road_val, 5);

			UINT32 obj_val = obj_addr[x];
			UINT32 obj6 = BIT(obj_val, 6);

			UINT32 term1 = !(c7 && c1);
			UINT32 term2 = !(c7 && c0);
			UINT32 term3 = r5 || !r6;
			UINT32 p12 = !(term1 && term2 && term3);
			UINT32 p6 = !(obj6 && term1 && term2);
			UINT32 sel =  p12 | (p6 << 1);

			UINT32 psel =  (!(p6 && p12) << 1) | p6;

			if		(sel == 3)	out_val = ((char_val & 0xc0) >> 2) | (chr_pal[char_val] & 0xf);
			else if (sel == 2)	out_val = road_val & 0x3f;
			else				out_val = obj_val & 0x3f;

			*bmp_addr++ = (psel << 6) | out_val;
		}
	}
}

VIDEO_UPDATE( tx1 )
{
	memset(tx1_obj_bmp, 0, 768*240);
	tx1_draw_char(tx1_chr_bmp);
	tx1_draw_road(tx1_rod_bmp);
	tx1_draw_objects(tx1_obj_bmp);
	tx1_combine_layers(bitmap, 0);
	tx1_combine_layers(bitmap, 1);
	tx1_combine_layers(bitmap, 2);
}

#define BB_RDFLAG_WAVE1		7
#define BB_RDFLAG_WAVE0		6
#define BB_RDFLAG_TNLMD1	5
#define BB_RDFLAG_TNLMD0	4
#define BB_RDFLAG_TNLF		3
#define BB_RDFLAG_LINF		2
#define BB_RDFLAG_RVA7		1
#define BB_RDFLAG_WANGL		0

static struct
{
	UINT32	ba_val;
	UINT32	ba_inc;

	UINT32	bank_mode;

	UINT16	h_val;
	UINT16	h_inc;
	UINT16	h_init;

	UINT8	wa8;
	UINT8	wa4;

	UINT8	slin;
	UINT8	slin_inc;

	UINT16	wave_lfsr;
	UINT16	scol;
	UINT8	sky;
	UINT16	gas;
	UINT8	flags;
	UINT8	shift;
} vregs;

UINT16 *buggyboy_objram;
UINT16 *buggyboy_rcram;
UINT16 *buggyboy_vram;
size_t buggyboy_objram_size;
size_t buggyboy_rcram_size;

static UINT8 *bb_chr_bmp;
static UINT8 *bb_obj_bmp;
static UINT8 *bb_rod_bmp;

PALETTE_INIT( buggyboy )
{
	int i;

	for (i = 0; i < 0x100; i++)
	{
		int bit0, bit1, bit2, bit3, bit4;
		int r, g, b;

		bit0 = BIT(color_prom[i + 0x000], 0);
		bit1 = BIT(color_prom[i + 0x000], 1);
		bit2 = BIT(color_prom[i + 0x000], 2);
		bit3 = BIT(color_prom[i + 0x000], 3);
		bit4 = BIT(color_prom[i + 0x300], 2);
		r = 0x06 * bit4 + 0x0d * bit0 + 0x1e * bit1 + 0x41 * bit2 + 0x8a * bit3;

		bit0 = BIT(color_prom[i + 0x100], 0);
		bit1 = BIT(color_prom[i + 0x100], 1);
		bit2 = BIT(color_prom[i + 0x100], 2);
		bit3 = BIT(color_prom[i + 0x100], 3);
		bit4 = BIT(color_prom[i + 0x300], 1);
		g = 0x06 * bit4 + 0x0d * bit0 + 0x1e * bit1 + 0x41 * bit2 + 0x8a * bit3;

		bit0 = BIT(color_prom[i + 0x200], 0);
		bit1 = BIT(color_prom[i + 0x200], 1);
		bit2 = BIT(color_prom[i + 0x200], 2);
		bit3 = BIT(color_prom[i + 0x200], 3);
		bit4 = BIT(color_prom[i + 0x300], 0);
		b = 0x06 * bit4 + 0x0d * bit0 + 0x1e * bit1 + 0x41 * bit2 + 0x8a * bit3;

		palette_set_color(i, r, g, b);
	}
}

static void buggyboy_draw_char(UINT8 *bitmap, int wide)
{
	INT32 x, y;
	UINT32 scroll_x, scroll_y;
	UINT8 *chars, *gfx2;
	UINT32 total_width;
	UINT32 x_mask;

	chars = memory_region(REGION_GFX1);
	gfx2 = memory_region(REGION_GFX1) + 0x4000;

	if (wide)
	{
		scroll_y = (buggyboy_vram[0xfff] >> 10) & 0x3f;
		scroll_x = buggyboy_vram[0xfff] & 0x3ff;
		total_width = 768;
		x_mask = 0x3ff;
	}
	else
	{
		scroll_y = (buggyboy_vram[0x7ff] >> 10) & 0x3f;
		scroll_x = buggyboy_vram[0x7ff] & 0x1ff;
		total_width = 256;
		x_mask = 0x1ff;
	}

	for (y = 0; y < 240; ++y)
	{
		UINT32 d0 = 0, d1 = 0;
		UINT32 colour = 0;
		UINT32 y_offs;
		UINT32 x_offs;
		UINT32 y_gran;

		if (y < 64)
			y_offs = y;
		else
		{
			y_offs = (y + (scroll_y | 0xc0) + 1) & 0xff;

			if (y_offs < 64)
				y_offs |= 0xc0;
		}

		if ((y_offs >= 64) && (y_offs < 128))
			x_offs = scroll_x;
		else
			x_offs = 0;

		y_gran = y_offs & 7;

		if (x_offs & 7)
		{
			UINT32 tilenum;
			UINT16 ram_val;

			if (wide)
				ram_val = buggyboy_vram[((y_offs << 4) & 0xf80) + ((x_offs >> 3) & 0x7f)];
			else
				ram_val = buggyboy_vram[((y_offs << 3) & 0x7c0) + ((x_offs >> 3) & 0x3f)];

			tilenum = (ram_val & 0x03ff) | ((ram_val & 0x8000) >> 5);
			colour = (ram_val & 0xfc00) >> 8;
			d0 = *(gfx2 + (tilenum << 3) + y_gran);
			d1 = *(chars + (tilenum << 3) + y_gran);
		}

		for (x = 0; x < total_width; ++x)
		{
			UINT32 x_gran = x_offs & 7;

			if (!x_gran)
			{
				UINT32 tilenum;
				UINT16 ram_val;

				if (wide)
					ram_val = buggyboy_vram[((y_offs << 4) & 0xf80) + ((x_offs >> 3) & 0x7f)];
				else
					ram_val = buggyboy_vram[((y_offs << 3) & 0x7c0) + ((x_offs >> 3) & 0x3f)];

				tilenum = (ram_val & 0x03ff) | ((ram_val & 0x8000) >> 5);
				colour = (ram_val & 0xfc00) >> 8;
				d0 = *(gfx2 + (tilenum << 3) + y_gran);
				d1 = *(chars + (tilenum << 3) + y_gran);
			}

			*bitmap++ = colour |
						(((d1 >> (7 ^ x_gran)) & 1) << 1) |
						((d0 >> (7 ^ x_gran)) & 1);

			x_offs = (x_offs + 1) & x_mask;
		}
	}
}

void buggyboy_get_roadpix(int screen, int ls161, UINT8 rva0_6, UINT8 sld, UINT32 *_rorev,
						  UINT8 *rc0, UINT8 *rc1, UINT8 *rc2, UINT8 *rc3,
						  const UINT8 *rom, const UINT8 *prom0, const UINT8 *prom1, const UINT8 *prom2)
{
	UINT16 ls283_159 = (ls161 & 0x780) + 128 + (256 * screen);
	UINT32 ls283_159_co = ls283_159 & 0x800;
	UINT32 rom_flip = ls283_159 & 0x200 ? 0 : 1;
	UINT32 rom_en = !(ls283_159 & 0x400) && !(ls283_159_co ^ (ls161 & 0x800));
	UINT8 d0 = 0;
	UINT8 d1 = 0;

	*_rorev = !( (rom_en && rom_flip) || (!rom_en && (ls161 & 0x4000)) );

	if (rom_en)
	{
		UINT8  rom_data;
		UINT16 prom_addr;

		UINT16 rha = (ls283_159 & 0x180) | (ls161 & 0x78);

		if (rom_flip)
			rha ^= 0x1f8;

		rom_data = rom[(1 << 13) | (rha << 4) | rva0_6];
		prom_addr = (rom_flip ? 0x80 : 0) | (rom_data & 0x7f);

		*rc0 = prom0[prom_addr];
		*rc1 = prom1[prom_addr];
		*rc2 = prom2[prom_addr];

		rom_data = rom[(rha << 4) | rva0_6];
		prom_addr = 0x100 | rom_data;

		d0 = prom0[prom_addr];
		d1 = prom1[prom_addr];
	}
	else
	{

		*rc0 = *rc1 = *rc2 = *rc3 = 0;
	}

	if (BIT(sld, 4))
	{
		if (BIT(sld, 5))
			d1 = ~d1;

		*rc3 = d0 & d1;

		if (rom_flip)
			*rc3 = BITSWAP8(*rc3, 0, 1, 2, 3, 4, 5, 6, 7);
	}
	else
		*rc3 = 0;
}

#define LOAD_HPOS_COUNTER(NUM)													\
	ram_val = buggyboy_rcram[(rva_offs + 0x1f8 + (2*NUM)) >> 1];				\
	rcrs10 = ram_val & 0xfc00 ? 0x0400 : 0x0000;								\
	hp = vregs.wa8 + ((BIT(ram_val, 15) << 11) | rcrs10 | (ram_val & 0x03ff));	\
	hp##NUM = hp & 0xff;														\
	hp >>= 8;																	\
	hps##NUM##0 = (BIT(hp, 0) || BIT(hp, 2)) && !BIT(hp, 3);					\
	hps##NUM##1 = (BIT(hp, 1) || BIT(hp, 2)) && !BIT(hp, 3);					\
	hps##NUM##2 = BIT(hp, 2);													\

#define UPDATE_HPOS(NUM)				\
	if (hp##NUM##_en)					\
	{									\
		if ((hp##NUM & 0xff) == 0xff)	\
			hp##NUM##_cy = 1;			\
		else							\
			hp##NUM = hp##NUM + 1;		\
	}									\

static void buggyboy_draw_road(UINT8 *bitmap)
{
	INT32 x;
	UINT32 y;
	UINT16 rva_offs;
	UINT32 tnlmd0;
	UINT32 tnlmd1;
	UINT32 linf;
	UINT32 tnlf;
	UINT32 wangl;
	UINT32 tcmd;
	UINT32 wave0;
	UINT32 wave1;
	UINT32 rva20_6;

	const UINT8 *rcols = (UINT8*)(memory_region(REGION_PROMS) + 0x1500);
	const UINT8 *rom   = memory_region(REGION_GFX3);
	const UINT8 *prom0 = rom + 0x4000;
	const UINT8 *prom1 = rom + 0x4200;
	const UINT8 *prom2 = rom + 0x4400;
	const UINT8 *vprom = rom + 0x4600;

	tcmd	 = ((vregs.scol & 0xc000) >> 12) | ((vregs.scol & 0x00c0) >> 6);
	tnlmd0   = BIT(vregs.flags, BB_RDFLAG_TNLMD0);
	tnlmd1   = BIT(vregs.flags, BB_RDFLAG_TNLMD1);
	linf     = BIT(vregs.flags, BB_RDFLAG_LINF);
	tnlf     = BIT(vregs.flags, BB_RDFLAG_TNLF);
	wangl    = BIT(vregs.flags, BB_RDFLAG_WANGL);
	wave0    = BIT(vregs.flags, BB_RDFLAG_WAVE0);
	wave1    = BIT(vregs.flags, BB_RDFLAG_WAVE1);
	rva_offs = BIT(vregs.flags, BB_RDFLAG_RVA7) ? 0x800 : 0xc00;

	for (y = 0; y < 240; ++y)
	{
		UINT8	rva0_6;
		UINT8	ram_addr;
		UINT16	rcrdb0_15;
		UINT16	rcrs10;
		UINT16	ls161_156_a;
		UINT16	ls161;
		UINT8	sld;
		UINT32	rva8;
		UINT32	rm0, rm1;
		UINT32	rcmd;
		UINT32	bnkls = 1;
		UINT32	bnkcs = 1;
		UINT32	bnkrs = 1;

		UINT32	x_offs;
		UINT8 sf;

		UINT32	ram_val;
		UINT32	hp;
		UINT32	vp1, vp2, vp3, vp4, vp5, vp6, vp7;

		UINT32	hp0, hp1, hp2, hp3;
		UINT8	hps00, hps01, hps02;
		UINT8	hps10, hps11, hps12;
		UINT8	hps20, hps21, hps22;
		UINT8	hps30, hps31, hps32;

		UINT8	rc0[3] = {0, 0, 0};
		UINT8	rc1[3] = {0, 0, 0};
		UINT8	rc2[3] = {0, 0, 0};
		UINT8	rc3[3] = {0, 0, 0};

		UINT8	hp0_cy = 0, hp1_cy = 0, hp2_cy = 0, hp3_cy = 0;

		UINT8	*bmpaddr = bitmap + (y * 256 * 3);

		UINT32	bank_cnt;
		UINT32	_rorevls = 0;
		UINT32	_rorevcs = 0;
		UINT32	_rorevrs = 0;

		UINT32	ic96_o17;
		UINT32	ic96_term1;
		UINT32	ic97_o12;
		UINT32	ic97_o13;
		UINT32	ic79_p19;

		rva8 = (vregs.h_val & 0x8000) || !(vregs.shift & 0x80);

		rva0_6 = (vregs.h_val >> 7) & 0x7f;

		rva20_6 = ((rva0_6 >> 3) & 0xe) | ((rva0_6 & 2) >> 1);

		ram_addr = (~rva0_6 & 0x7f) << 1;

		rcrdb0_15 = buggyboy_rcram[(rva_offs + ram_addr) >> 1];

		rcrs10 = rcrdb0_15 & 0xfc00 ? 0x0400 : 0x0000;

		ls161_156_a = (rcrdb0_15 & 0xfc00) == 0xfc00 ? 0x800 : 0x0000;

		ls161 =  ((rcrdb0_15 & 0x8000) >> 1) | ls161_156_a | rcrs10 | (rcrdb0_15 & 0x03ff);

		sld = (vprom[rva0_6] + vregs.slin) & 0x38;

		x_offs = ls161 & 7;

		vp1 = buggyboy_rcram[(rva_offs + 0x1e2) >> 1] >= y ? 0 : 1;
		vp2 = buggyboy_rcram[(rva_offs + 0x1e4) >> 1] >= y ? 0 : 1;
		vp3 = buggyboy_rcram[(rva_offs + 0x1e6) >> 1] >= y ? 0 : 1;
		vp4 = buggyboy_rcram[(rva_offs + 0x1e8) >> 1] >= y ? 0 : 1;
		vp5 = buggyboy_rcram[(rva_offs + 0x1ea) >> 1] >= y ? 0 : 1;
		vp6 = buggyboy_rcram[(rva_offs + 0x1ec) >> 1] >= y ? 0 : 1;
		vp7 = buggyboy_rcram[(rva_offs + 0x1ee) >> 1] >= y ? 0 : 1;

		rm0 = vp7 ? BIT(vregs.scol, 4) : BIT(vregs.scol, 12);
		rm1 = vp7 ? BIT(vregs.scol, 5) : BIT(vregs.scol, 13);

		rcmd = (vp7 ? vregs.scol : vregs.scol >> 8) & 0xf;

		LOAD_HPOS_COUNTER(0);
		LOAD_HPOS_COUNTER(1);
		LOAD_HPOS_COUNTER(2);
		LOAD_HPOS_COUNTER(3);

		bank_cnt = (vregs.ba_val >> 5) & 0x3ff;

		if (ls161 & 7)
		{
			buggyboy_get_roadpix(0, ls161, rva0_6, sld, &_rorevls, &rc0[0], &rc1[0], &rc2[0], &rc3[0], rom, prom0, prom1, prom2);
			buggyboy_get_roadpix(1, ls161, rva0_6, sld, &_rorevcs, &rc0[1], &rc1[1], &rc2[1], &rc3[1], rom, prom0, prom1, prom2);
			buggyboy_get_roadpix(2, ls161, rva0_6, sld, &_rorevrs, &rc0[2], &rc1[2], &rc2[2], &rc3[2], rom, prom0, prom1, prom2);
		}

		ic96_term1 = !tnlf || (vp5 && vp6) || (!vp3 && vp6) || (!vp4 && vp5) || (!vp3 && !vp4);
		ic96_o17 = (vp3 && vp4) || tnlmd0 || tnlmd1 || !vp1 || !tnlf;

		ic97_o12 = (!vp1 && !vp2 && !vp6) || (!vp1 && !vp2 && vp7) || (vp4 && !vp6) || (vp4 && vp7);
		ic97_o13 = (!vp1 && !vp2 && !vp5) || (!vp1 && !vp2 && vp7) || (vp3 && !vp5) || (vp3 && vp7);

		ic79_p19 = !(!(vp5 || vp6) || vp7) && !(tnlmd0 || tnlmd1);

		for (x = 0; x < 256; ++x)
		{
			UINT32	pix;
			UINT32	hp0_en, hp1_en, hp2_en, hp3_en;

			UINT32	ic97_o17;
			UINT32	ic97_o18;
			UINT32	ic97_o19;

			UINT32	ic96_o14;
			UINT32	ic96_o15;
			UINT32	ic96_o16;

			UINT32	ic79_o15;
			UINT32	ic79_o16;
			UINT32	ic79_o17;

			UINT32	ic82_o17;
			UINT32	ic82_o16;
			UINT32	ic82_o15;
			UINT32	ic82_o14;

			UINT32	ic80_o17;
			UINT32	ic80_o16;
			UINT32	ic80_o15;
			UINT32	ic80_o14;

			UINT32	ic78_o17;
			UINT32	ic78_o16;
			UINT32	ic78_o15;
			UINT32	ic78_o14;

			UINT32	ic48_o12, ic50_o12, ic52_o12;
			UINT32	ic48_o16, ic50_o16, ic52_o16;
			UINT32	ic48_o17, ic50_o17, ic52_o17;
			UINT32	ic48_o18, ic50_o18, ic52_o18;
			UINT32	ic48_o19, ic50_o19, ic52_o19;

			UINT32	tmp;

			UINT8	px0[3];
			UINT8	px1[3];
			UINT8	px2[3];
			UINT8	px3[3];

			UINT32 lfsr = vregs.wave_lfsr;
			UINT32 wave =
						(wave0 ^ BIT(lfsr, 0))
						&& (wave1 ^ BIT(lfsr, 3))
						&& BIT(lfsr, 5)
						&& !BIT(lfsr, 15)
						&& BIT(lfsr, 11)
						&& BIT(lfsr, 13)
						&& (rva20_6 < ((lfsr >> 8) & 0xf));

			pix = (ls161 & 7) ^ 7;

			hp0_en = !(hp0_cy || hps02);
			hp1_en = !(hp1_cy || hps12);
			hp2_en = !(hp2_cy || hps22);
			hp3_en = !(hp3_cy || hps32);

			if (!(ls161 & 7))
			{
				buggyboy_get_roadpix(0, ls161, rva0_6, sld, &_rorevls, &rc0[0], &rc1[0], &rc2[0], &rc3[0], rom, prom0, prom1, prom2);
				buggyboy_get_roadpix(1, ls161, rva0_6, sld, &_rorevcs, &rc0[1], &rc1[1], &rc2[1], &rc3[1], rom, prom0, prom1, prom2);
				buggyboy_get_roadpix(2, ls161, rva0_6, sld, &_rorevrs, &rc0[2], &rc1[2], &rc2[2], &rc3[2], rom, prom0, prom1, prom2);
			}

			if (BIT(vregs.ba_val, 23))
			{
				bnkls = 1; bnkcs = 1; bnkrs = 1;
			}
			else if (vregs.ba_val & 0x007f8000)
			{
				bnkls = 0; bnkcs = 0; bnkrs = 0;
			}
			else
			{
				bnkls = bank_cnt < 0x400;
				bnkcs = bank_cnt < 0x300;
				bnkrs = bank_cnt < 0x200;
			}

			if (vregs.bank_mode)
			{
				bnkls ^= 1; bnkcs ^= 1; bnkrs ^= 1;
			}

			px0[0] = BIT(rc0[0], pix);
			px1[0] = BIT(rc1[0], pix);
			px2[0] = BIT(rc2[0], pix);
			px3[0] = BIT(rc3[0], pix);

			px0[1] = BIT(rc0[1], pix);
			px1[1] = BIT(rc1[1], pix);
			px2[1] = BIT(rc2[1], pix);
			px3[1] = BIT(rc3[1], pix);

			px0[2] = BIT(rc0[2], pix);
			px1[2] = BIT(rc1[2], pix);
			px2[2] = BIT(rc2[2], pix);
			px3[2] = BIT(rc3[2], pix);

			ic96_o14 =
			ic96_term1
			|| (!hp1_en && hps10 && vp4 && !hps20 && !hps21)
			|| (!hp1_en && hps10 && vp4 && hp2_en && !hps21)
			|| (!hp1_en && hps10 && vp4 && vp6)
			|| (hps11 && vp4 && !hps20 && !hps21)
			|| (hps11 && vp4 && hp2_en && !hps21)
			|| (vp5 && !hps20 && !hps21)
			|| (!vp3 && !hps20 && !hps21)
			|| (vp5 && hp2_en && !hps21)
			|| (!vp3 && hp2_en && !hps21)
			|| (!hp1_en && hps10 && !vp4)
			|| (hps11 && vp4 && vp6)
			|| (hps11 && !vp4);

			ic96_o15 =
			ic96_term1
			|| (hps10 && hps11 && vp4 && hp2_en && !hps20)
			||(!hp1_en && hps11 && vp4 && hp2_en && !hps20)
			|| (hps10 && hps11 && vp6)
			|| (!hp1_en && hps11 && vp6)
			|| (hps10 && hps11 && vp4 && !hps21)
			|| (!hp1_en && hps11 && vp4 && !hps21)
			|| (vp5 && hp2_en && !hps20)
			|| (!vp3 && hp2_en && !hps20)
			|| (hps10 && hps11 && !vp4)
			|| (!hp1_en && hps11 && !vp4)
			|| (vp5 && !hps21)
			|| (!vp3 && !hps21);

			ic96_o16 =
			ic96_term1
			|| (!hp1_en && hps10 && hps11 && vp6)
			|| (!hp1_en && hps10 && hps11 && !vp4)
			|| (!hp1_en && hps10 && hps11 && !hps20)
			|| (!hp1_en && hps10 && hps11 && hp2_en)
			|| (!hp1_en && hps10 && hps11 && !hps21)
			|| (vp5 && !hps20)
			|| (!vp3 && !hps20)
			|| (vp5 && hp2_en)
			|| (!vp3 && hp2_en)
			|| (vp5 && !hps21)
			|| (!vp3 && !hps21);

			tmp = (!vp1 && !vp2) || (vp2 && vp7);
			ic97_o17 = tmp || (!hp0_en && hps00 && vp2 && !hps30 && !hps31) || (!hp0_en && hps00 && vp2 && hp3_en && !hps31) || (hps01 && vp2 && !hps30 && !hps31) || (hps01 && vp2 && hp3_en && !hps31);
			ic97_o18 = tmp || (hps00 && hps01 && vp2 && hp3_en && !hps30) || (!hp0_en && hps01 && vp2 && hp3_en && !hps30) || (hps00 && hps01 && vp2 && !hps31) || (!hp0_en && hps01 && vp2 && !hps31);
			ic97_o19 = tmp || (!hp0_en && hps00 && hps01 && vp2 && !hps30) || (!hp0_en && hps00 && hps01 && vp2 && hp3_en) || (!hp0_en && hps00 && hps01 && vp2 && !hps31);

			{
			UINT32 P4, P6, P7, P18, P19;

			P4 = !(ic96_o16 && ic96_o17 && ic97_o19);
			P6 = BIT(sld, 3);
			P7 = (!vp5 && !vp6) || vp7 || !linf;
			P19 = wangl && bnkls;

			ic78_o17 = (_rorevls && !tnlmd1 && tnlmd0) || (!_rorevls && tnlmd1 && !tnlmd0) || (_rorevls && ic97_o12) || (!_rorevls && ic97_o13);
			P18 = ic78_o17;
			ic78_o16 = P7 || (px1[0] && !px0[0] && tnlmd1 && !tnlmd0) || (px2[0] && tnlmd1 && tnlmd0);
			ic78_o15 = !tnlf || (!P4 && px0[0] && P19) || (!P4 && px1[0] && P19) || (!P4 && P18) || (!P4 && px2[0]);
			ic78_o14 = !P6 || tnlmd0 || tnlmd1 || P7;
			}

			{
			UINT32 P4, P6, P7, P18, P19;

			P4 = !(ic96_o15 && ic96_o17 && ic97_o18);
			P6 = BIT(sld, 3);
			P7 = (!vp5 && !vp6) || vp7 || !linf;
			P19 = wangl && bnkcs;

			ic80_o17 = (_rorevcs && !tnlmd1 && tnlmd0) || (!_rorevcs && tnlmd1 && !tnlmd0) || (_rorevcs && ic97_o12) || (!_rorevcs && ic97_o13);
			P18 = ic80_o17;
			ic80_o16 = P7 || (px1[1] && !px0[1] && tnlmd1 && !tnlmd0) || (px2[1] && tnlmd1 && tnlmd0);
			ic80_o15 = !tnlf || (!P4 && px0[1] && P19) || (!P4 && px1[1] && P19) || (!P4 && P18) || (!P4 && px2[1]);
			ic80_o14 = !P6 || tnlmd0 || tnlmd1 || P7;
			}

			{
			UINT32 P4, P6, P7, P18, P19;

			P4 = !(ic96_o14 && ic96_o17 && ic97_o17);
			P6 = BIT(sld, 3);
			P7 = (!vp5 && !vp6) || vp7 || !linf;
			P19 = wangl && bnkrs;

			ic82_o17 = (_rorevrs && !tnlmd1 && tnlmd0) || (!_rorevrs && tnlmd1 && !tnlmd0) || (_rorevrs && ic97_o12) || (!_rorevrs && ic97_o13);
			P18 = ic82_o17;
			ic82_o16 = P7 || (px1[2] && !px0[2] && tnlmd1 && !tnlmd0) || (px2[2] && tnlmd1 && tnlmd0);
			ic82_o15 = !tnlf || (!P4 && px0[2] && P19) || (!P4 && px1[2] && P19) || (!P4 && P18) || (!P4 && px2[2]);
			ic82_o14 = !P6 || tnlmd0 || tnlmd1 || P7;
			}

			ic79_o17 = (!px2[0] && _rorevls && ic78_o15) || (tnlf && !ic97_o19) || (tnlf && ic79_p19 && px2[0] && ic78_o15);
			ic79_o16 = (!px2[1] && _rorevcs && ic80_o15) || (tnlf && !ic97_o18) || (tnlf && ic79_p19 && px2[1] && ic80_o15);
			ic79_o15 = (!px2[2] && _rorevrs && ic82_o15) || (tnlf && !ic97_o17) || (tnlf && ic79_p19 && px2[2] && ic82_o15);

			{
			UINT32 P5,P7, P8;
			UINT32 rcsd0_3;
			UINT32 cprom_addr;

			P5 = BIT(tcmd, 3) ? ((!vp5 && !vp6) || vp7 || !linf) : ic78_o16;
			P7 = BIT(sld, 5);
			P8 = BIT(sld, 4);

			ic48_o19 = (px2[0] && !rva8) || !bnkls || !P5 || !ic78_o15;

			if (ic48_o19)
			{
				ic48_o12 =
				ic78_o14 &&
				(
					(px2[0] && px1[0] && px0[0] && rm1 && !rm0)
					|| (!px2[0] && px1[0] && px0[0] && !P8 && rm0)
					|| (px2[0] && px0[0] && !P7 && !rm1 && !rm0)
					|| (px2[0] && !px1[0] && px0[0] && !P7 && !rm1)
					|| (px2[0] && px1[0] && px0[0] && !P7 && !P8)
					|| (px2[0] && px1[0] && px0[0] && !P8 && rm1)
					|| (!px2[0] && !px3[0] && !rm0)
					|| (!px1[0] && !px3[0] && rm1)
					|| (!px2[0] && !px1[0] && !px3[0])
					|| (rva8)
					|| (!px0[0] && !px3[0])
					|| (!ic78_o15)
					|| (!P5)
				);

				ic48_o16 = (px2[0] && P5 && rm1) || (P5 && rva8 && ic78_o15) || (!px0[0] && P5) || !ic78_o15;
				ic48_o17 = (px0[0] && P5 && !rm0 && ic78_o15) || (!px1[0] && P5 && ic78_o15) || (P5 && rva8 && ic78_o15);
				ic48_o18 = (!px2[0] && P5 && ic78_o15) || (P5 && rva8 && ic78_o15);

				if (vp6 || ic78_o16)
				{
					if (!(ic78_o15 && P5))
						cprom_addr = (tcmd & 0x7) | (ic78_o16 << 3);
					else
						cprom_addr = rcmd;

					cprom_addr = (~cprom_addr & 0xf) << 4;
				}
				else
					cprom_addr = 0xf0;

				cprom_addr |= (ic79_o17 << 3) |
							  (ic48_o18 << 2) |
							  (ic48_o17 << 1) |
							  ic48_o16;

				rcsd0_3 = rcols[cprom_addr] & 0xf;
				*(bmpaddr + 0) = 0x40 | (!wave << 5) | (ic48_o12 << 4) | rcsd0_3;
			}
			else
				*(bmpaddr + 0) = 0;
			}

			{
			UINT32 rcsd0_3;
			UINT32 cprom_addr;
			UINT32 P5, P7, P8;

			P5 = BIT(tcmd, 3) ? ((!vp5 && !vp6) || vp7 || !linf) : ic80_o16;
			P7 = BIT(sld, 5);
			P8 = BIT(sld, 4);

			ic50_o19 = (px2[1] && !rva8) || !bnkcs || !P5 || !ic80_o15;

			if (ic50_o19)
			{
				if (ic80_o14)
					ic50_o12 =
					(px2[1] && px1[1] && px0[1] && rm1 && !rm0)
					|| (!px2[1] && px1[1] && px0[1] && !P8 && rm0)
					|| (px2[1] && px0[1] && !P7 && !rm1 && !rm0)
					|| (px2[1] && !px1[1] && px0[1] && !P7 && !rm1)
					|| (px2[1] && px1[1] && px0[1] && !P7 && !P8)
					|| (px2[1] && px1[1] && px0[1] && !P8 && rm1)
					|| (!px2[1] && !px3[1] && !rm0)
					|| (!px1[1] && !px3[1] && rm1)
					|| (!px2[1] && !px1[1] && !px3[1])
					|| (rva8)
					|| (!px0[1] && !px3[1])
					|| (!ic80_o15)
					|| (!P5);
				else
					ic50_o12 = 0;

				ic50_o16 = (P5 && px2[1] && rm1) || (P5 && rva8 && ic80_o15) || (P5 && !px0[1]) || !ic80_o15;
				ic50_o17 = (P5 && px0[1] && !rm0 && ic80_o15) || (P5 && !px1[1] && ic80_o15) || (P5 && rva8 && ic80_o15);
				ic50_o18 = (P5 && !px2[1] && ic80_o15) || (P5 && rva8 && ic80_o15);

				if (vp6 || ic80_o16)
				{
					if (!(ic80_o15 && P5))
						cprom_addr = (tcmd & 0x7) | (ic80_o16 << 3);
					else
						cprom_addr = rcmd;

					cprom_addr = ((~cprom_addr) & 0xf) << 4;
				}
				else
					cprom_addr = 0xf0;

				cprom_addr |= (ic79_o16 << 3) |
							  (ic50_o18 << 2) |
							  (ic50_o17 << 1) |
							  ic50_o16;

				rcsd0_3 = rcols[cprom_addr] & 0xf;
				*(bmpaddr + 256) = 0x40 | (!wave << 5) | (ic50_o12 << 4) | rcsd0_3;
			}
			else
				*(bmpaddr + 256) = 0;
			}

			{
			UINT32 rcsd0_3;
			UINT32 cprom_addr;
			UINT32 P5, P7, P8;

			P5 = BIT(tcmd, 3) ? ((!vp5 && !vp6) || vp7 || !linf) : ic82_o16;
			P7 = BIT(sld, 5);
			P8 = BIT(sld, 4);

			ic52_o19 = (px2[2] && !rva8) || !bnkrs || !P5 || !ic82_o15;

			if (ic52_o19)
			{
				ic52_o12 =
				ic82_o14 &&
				(
					(px2[2] && px1[2] && px0[2] && rm1 && !rm0)
					|| (!px2[2] && px1[2] && px0[2] && !P8 && rm0)
					|| (px2[2] && px0[2] && !P7 && !rm1 && !rm0)
					|| (px2[2] && !px1[2] && px0[2] && !P7 && !rm1)
					|| (px2[2] && px1[2] && px0[2] && !P7 && !P8)
					|| (px2[2] && px1[2] && px0[2] && !P8 && rm1)
					|| (!px2[2] && !px3[2] && !rm0)
					|| (!px1[2] && !px3[2] && rm1)
					|| (!px2[2] && !px1[2] && !px3[2])
					|| (rva8)
					|| (!px0[2] && !px3[2])
					|| (!ic82_o15)
					|| (!P5)
				);

				ic52_o16 = (px2[2] && P5 && rm1) || (P5 && rva8 && ic82_o15) || (!px0[2] && P5) || !ic82_o15;
				ic52_o17 = (px0[2] && P5 && !rm0 && ic82_o15) || (!px1[2] && P5 && ic82_o15) || (P5 && rva8 && ic82_o15);
				ic52_o18 = (P5 && ic82_o15 && !px2[2]) || (P5 && ic82_o15 && rva8);

				if (vp6 || ic82_o16)
				{
					if (!(ic82_o15 && P5))
						cprom_addr = (tcmd & 0x7) | (ic82_o16 << 3);
					else
						cprom_addr = rcmd;

					cprom_addr = (~cprom_addr & 0xf) << 4;
				}
				else
					cprom_addr = 0xf0;

				cprom_addr |= (ic79_o15 << 3) |
							  (ic52_o18 << 2) |
							  (ic52_o17 << 1) |
							  ic52_o16;

				rcsd0_3 = rcols[cprom_addr] & 0xf;

				*(bmpaddr + 512) = 0x40 | (!wave << 5) | (ic52_o12 ? 0x10 : 0) | rcsd0_3;
			}
			else
				*(bmpaddr + 512) = 0;
			}

			++bmpaddr;

			UPDATE_HPOS(0);
			UPDATE_HPOS(1);
			UPDATE_HPOS(2);
			UPDATE_HPOS(3);

			vregs.wave_lfsr = (vregs.wave_lfsr << 1) | (BIT(vregs.wave_lfsr, 6) ^ !BIT(vregs.wave_lfsr, 15));

			bank_cnt = (bank_cnt + 1) & 0x7ff;

			ls161 = (ls161 + 1) & 0x7fff;
		}

		if (wangl)
		{
			if (BIT(vregs.flags, BB_RDFLAG_TNLMD0))
				--vregs.wa8;
			else
				++vregs.wa8;
		}

		if (vregs.wa4 != 0xf)
			++vregs.wa4;
		else
		{

			if (wangl)
			{
				if (BIT(vregs.flags, BB_RDFLAG_TNLMD0))
					--vregs.wa8;
				else
					++vregs.wa8;
			}
			vregs.wa4 = 1;
		}

		vregs.h_val += vregs.h_inc;

		sf = vregs.shift;

		if ((vregs.shift & 0x80) == 0)
		{
			vregs.shift <<= 1;

			if ((sf & 0x08) == 0)
				vregs.shift |= BIT(vregs.h_val, 15);
		}

		if ((sf & 0x08) && !(vregs.shift & 0x08))
			vregs.h_inc = vregs.gas;

		vregs.ba_val = (vregs.ba_val + vregs.ba_inc) & 0x00ffffff;
	}
}

static void buggyb1_draw_road(UINT8 *bitmap, int wide)
{
	INT32 x;
	UINT32 y;
	UINT16 rva_offs;
	UINT32 tnlmd0;
	UINT32 tnlmd1;
	UINT32 linf;
	UINT32 tnlf;
	UINT32 wangl;
	UINT32 tcmd;
	UINT32 wave0;
	UINT32 wave1;
	UINT32 rva20_6;

	const UINT8 *rcols = (UINT8*)(memory_region(REGION_PROMS) + 0x1500);
	const UINT8 *rom   = memory_region(REGION_GFX3);
	const UINT8 *prom0 = rom + 0x4000;
	const UINT8 *prom1 = rom + 0x4200;
	const UINT8 *prom2 = rom + 0x4400;
	const UINT8 *vprom = rom + 0x4600;

	tcmd	 = ((vregs.scol & 0xc000) >> 12) | ((vregs.scol & 0x00c0) >> 6);
	tnlmd0   = BIT(vregs.flags, BB_RDFLAG_TNLMD0);
	tnlmd1   = BIT(vregs.flags, BB_RDFLAG_TNLMD1);
	linf     = BIT(vregs.flags, BB_RDFLAG_LINF);
	tnlf     = BIT(vregs.flags, BB_RDFLAG_TNLF);
	wangl    = BIT(vregs.flags, BB_RDFLAG_WANGL);
	wave0    = BIT(vregs.flags, BB_RDFLAG_WAVE0);
	wave1    = BIT(vregs.flags, BB_RDFLAG_WAVE1);
	rva_offs = BIT(vregs.flags, BB_RDFLAG_RVA7) ? 0x800 : 0xc00;

	for (y = 0; y < 240; ++y)
	{
		UINT8	rva0_6;
		UINT8	ram_addr;
		UINT16	rcrdb0_15;
		UINT16	rcrs10;
		UINT16	ls161_156_a;
		UINT16	ls161;
		UINT8	sld;
		UINT32	rva8;
		UINT32	rm0, rm1;
		UINT32	rcmd;
		UINT32	bnkcs = 1;

		UINT32	x_offs;
		UINT8	sf;

		UINT32	ram_val;
		UINT32	hp;
		UINT32	vp1, vp2, vp3, vp4, vp5, vp6, vp7;

		UINT32	ic4_o12;
		UINT32	ic4_o13;
		UINT32	ic149_o15;
		UINT32	ic151_o14;

		UINT32	hp0, hp1, hp2, hp3;
		UINT8	hps00, hps01, hps02;
		UINT8	hps10, hps11, hps12;
		UINT8	hps20, hps21, hps22;
		UINT8	hps30, hps31, hps32;

		UINT8	rc0 = 0, rc1 = 0, rc2 = 0, rc3 = 0;

		UINT8	hp0_cy = 0, hp1_cy = 0, hp2_cy = 0, hp3_cy = 0;

		UINT8	*bmpaddr = bitmap + (y * 256);

		UINT32	bank_cnt;
		UINT32	_rorevcs = 0;

		rva8 = (vregs.h_val & 0x8000) || !(vregs.shift & 0x80);

		rva0_6 = (vregs.h_val >> 7) & 0x7f;

		rva20_6 = ((rva0_6 >> 3) & 0xe) | ((rva0_6 & 2) >> 1);

		ram_addr = (~rva0_6 & 0x7f) << 1;

		rcrdb0_15 = buggyboy_rcram[(rva_offs + ram_addr) >> 1];

		rcrs10 = rcrdb0_15 & 0xfc00 ? 0x0400 : 0x0000;

		ls161_156_a = (rcrdb0_15 & 0xfc00) == 0xfc00 ? 0x800 : 0x0000;

		ls161 =  ((rcrdb0_15 & 0x8000) >> 1) | ls161_156_a | rcrs10 | (rcrdb0_15 & 0x03ff);

		sld = (vprom[rva0_6] + vregs.slin) & 0x38;

		x_offs = ls161 & 7;

		vp1 = buggyboy_rcram[(rva_offs + 0x1e2) >> 1] >= y ? 0 : 1;
		vp2 = buggyboy_rcram[(rva_offs + 0x1e4) >> 1] >= y ? 0 : 1;
		vp3 = buggyboy_rcram[(rva_offs + 0x1e6) >> 1] >= y ? 0 : 1;
		vp4 = buggyboy_rcram[(rva_offs + 0x1e8) >> 1] >= y ? 0 : 1;
		vp5 = buggyboy_rcram[(rva_offs + 0x1ea) >> 1] >= y ? 0 : 1;
		vp6 = buggyboy_rcram[(rva_offs + 0x1ec) >> 1] >= y ? 0 : 1;
		vp7 = buggyboy_rcram[(rva_offs + 0x1ee) >> 1] >= y ? 0 : 1;

		rm0 = vp7 ? BIT(vregs.scol, 4) : BIT(vregs.scol, 12);
		rm1 = vp7 ? BIT(vregs.scol, 5) : BIT(vregs.scol, 13);

		rcmd = (vp7 ? vregs.scol : vregs.scol >> 8) & 0xf;

		LOAD_HPOS_COUNTER(0);
		LOAD_HPOS_COUNTER(1);
		LOAD_HPOS_COUNTER(2);
		LOAD_HPOS_COUNTER(3);

		ic4_o12 = (!vp1 && !vp2 && !vp6) || (!vp1 && !vp2 && vp7) || (vp4 && !vp6) || (vp4 && vp7);
		ic4_o13 = (!vp1 && !vp2 && !vp5) || (!vp1 && !vp2 && vp7) || (vp3 && !vp5) || (vp3 && vp7);
		ic149_o15 = (!vp5 && !vp6) || vp7 || !linf;
		ic151_o14 = !BIT(sld, 3) || tnlmd0 || tnlmd1 || ic149_o15;

		bank_cnt = (vregs.ba_val >> 5) & 0x3ff;

		if (ls161 & 7)
			buggyboy_get_roadpix(1, ls161, rva0_6, sld, &_rorevcs, &rc0, &rc1, &rc2, &rc3, rom, prom0, prom1, prom2);

		for (x = 0; x < 256; ++x)
		{
			UINT32	pix;
			UINT32	hp0_en, hp1_en, hp2_en, hp3_en;
			UINT32	ic149_o16;
			UINT32	ic4_o18;
			UINT32	ic3_o15;
			UINT32	ic150_o12 = 0;
			UINT32	ic150_o16;
			UINT32	ic150_o17;
			UINT32	ic150_o18;
			UINT32	ic150_o19;
			UINT32	ic151_o15;
			UINT32	ic151_o16;
			UINT32	ic151_o17;
			UINT32	rcsd0_3 = 0;
			UINT32	sld5 = BIT(sld, 5);
			UINT32	sld4 = BIT(sld, 4);
			UINT32	mux;
			UINT32	cprom_addr;
			UINT8	px0, px1, px2, px3;

			pix = (ls161 & 7) ^ 7;

			hp0_en = !(hp0_cy || hps02);
			hp1_en = !(hp1_cy || hps12);
			hp2_en = !(hp2_cy || hps22);
			hp3_en = !(hp3_cy || hps32);

			if (!(ls161 & 7))
				buggyboy_get_roadpix(1, ls161, rva0_6, sld, &_rorevcs, &rc0, &rc1, &rc2, &rc3, rom, prom0, prom1, prom2);

			if (vregs.bank_mode == 0)
			{
				if (BIT(vregs.ba_val, 23))
					bnkcs = 1;
				else if (vregs.ba_val & 0x007f8000)
					bnkcs = 0;
				else
					bnkcs = bank_cnt < 0x300;
			}
			else
			{
				if (BIT(vregs.ba_val, 23))
					bnkcs = 0;
				else if (vregs.ba_val & 0x007f8000)
					bnkcs = 1;
				else
					bnkcs = bank_cnt >= 0x300;
			}

			px0 = BIT(rc0, pix);
			px1 = BIT(rc1, pix);
			px2 = BIT(rc2, pix);
			px3 = BIT(rc3, pix);

			if (vp2)
				ic4_o18 = (hps00 && hps01 && hp3_en && !hps30)		||
						  (!hp0_en && hps01 && hp3_en && !hps30)	||
						  (hps00 && hps01 && !hps31)				||
						  (!hp0_en && hps01 && !hps31)				||
						  vp7;
			else
				ic4_o18 = !vp1;

			if (tnlf)
				ic3_o15 = (vp4 && !vp6 && !hp2_en && hps21)		||
						  (vp4 && !vp6 && hps20 && hps21)		||
						  (vp1 && !vp4 && !tnlmd1 && !tnlmd0)	||
						  (vp1 && !vp3 && !tnlmd1 && !tnlmd0)	||
						  (hp1_en && !hps10 && vp3 && !vp5)		||
						  (!hps11 && vp3 && !vp5);
			else
				ic3_o15 = !ic4_o18;

			ic151_o17 = (_rorevcs && !tnlmd1 && tnlmd0)		||
						(!_rorevcs && tnlmd1 && !tnlmd0)	||
						(_rorevcs && ic4_o12)				||
						(!_rorevcs && ic4_o13);

			if (!ic3_o15)
				ic151_o15 = (px0 && (bnkcs && wangl))	||
							(px1 && (bnkcs && wangl))	||
							ic151_o17					||
							px2							||
							!tnlf;
			else
				ic151_o15 = !tnlf;

			ic151_o16 = (px1 && !px0 && tnlmd1 && !tnlmd0)	||
						(px2 && tnlmd1 && tnlmd0)			||
						ic149_o15;

			mux = BIT(tcmd, 3) ? ic149_o15 : ic151_o16;

			ic150_o19 = (px2 && !rva8)	||
						!bnkcs			||
						!mux			||
						!ic151_o15;

			if (ic150_o19)
			{
				ic149_o16 = (_rorevcs && !px2 && ic151_o15)									||
							(tnlf && vp5 && !vp7 && px2 && !tnlmd0 && !tnlmd1 && ic151_o15)	||
							(tnlf && vp6 && !vp7 && px2 && !tnlmd0 && !tnlmd1 && ic151_o15)	||
							(tnlf && !ic4_o18);

				ic150_o16 = (px2 && mux && rm1)			||
							(mux && rva8 && ic151_o15)	||
							(!px0 && mux)				||
							!ic151_o15;

				{
					UINT32 a = mux && ic151_o15;

					ic150_o17 = (a && !rm0 && px0)	||
								(a && !px1)			||
								(rva8 && a);

					ic150_o18 = (a && !px2) ||
								(rva8 && a);
				}

				if (ic151_o14)
					ic150_o12 = rva8 || !mux || !ic151_o15				||
								(px2 && px1 && px0 && rm1 && !rm0)		||
								(!px2 && px1 && px0 && !sld4 && rm0)	||
								(px2 && px0 && !sld5 && !rm1 && !rm0)	||
								(px2 && !px1 && px0 && !sld5 && !rm1)	||
								(px2 && px1 && px0 && !sld5 && !sld4)	||
								(px2 && px1 && px0 && !sld4 && rm1)		||
								(!px2 && !px3 && !rm0)					||
								(!px1 && !px3 && rm1)					||
								(!px2 && !px1 && !px3)					||
								(!px0 && !px3);
				else
					ic150_o12 = 0;

				if (vp6 || ic151_o16)
				{
					UINT32 ic150_i5 = BIT(tcmd, 3) ? ic149_o15 : ic151_o16;

					if (!(ic151_o15 && ic150_i5))
						cprom_addr = (tcmd & 0x7) | (ic151_o16 ? 0x08 : 0);
					else
						cprom_addr = rcmd;

					cprom_addr = ((~cprom_addr) & 0xf) << 4;
				}
				else
					cprom_addr = 0xf0;

				cprom_addr |= (ic149_o16 ? 0x8 : 0) |
							  (ic150_o18 ? 0x4 : 0) |
							  (ic150_o17 ? 0x2 : 0) |
							  (ic150_o16 ? 0x1 : 0);

				rcsd0_3 = rcols[cprom_addr] & 0xf;

				{
					UINT32 lfsr = vregs.wave_lfsr;
					UINT32 wave =
								(wave0 ^ BIT(lfsr, 0))	&&
								(wave1 ^ BIT(lfsr, 3))	&&
								BIT(lfsr, 5)			&&
								!BIT(lfsr, 15)			&&
								BIT(lfsr, 11)			&&
								BIT(lfsr, 13)			&&
								(rva20_6 < ((lfsr >> 8) & 0xf));

					*bmpaddr++ = 0x40 | (wave ? 0 : 0x20) | (ic150_o12 ? 0x10 : 0) | rcsd0_3;
				}
			}
			else
				*bmpaddr++ = 0;

			UPDATE_HPOS(0);
			UPDATE_HPOS(1);
			UPDATE_HPOS(2);
			UPDATE_HPOS(3);

			vregs.wave_lfsr = (vregs.wave_lfsr << 1) | (BIT(vregs.wave_lfsr, 6) ^ !BIT(vregs.wave_lfsr, 15));

			bank_cnt = (bank_cnt + 1) & 0x7ff;

			ls161 = (ls161 + 1) & 0x7fff;
		}

		if (wangl)
		{
			if (BIT(vregs.flags, BB_RDFLAG_TNLMD0))
				--vregs.wa8;
			else
				++vregs.wa8;
		}

		if (vregs.wa4 != 0xf)
			++vregs.wa4;
		else
		{

			if (wangl)
			{
				if (BIT(vregs.flags, BB_RDFLAG_TNLMD0))
					--vregs.wa8;
				else
					++vregs.wa8;
			}
			vregs.wa4 = 1;
		}

		vregs.h_val += vregs.h_inc;

		sf = vregs.shift;

		if ((vregs.shift & 0x80) == 0)
		{
			vregs.shift <<= 1;

			if ((sf & 0x08) == 0)
				vregs.shift |= BIT(vregs.h_val, 15);
		}

		if ((sf & 0x08) && !(vregs.shift & 0x08))
			vregs.h_inc = vregs.gas;

		vregs.ba_val = (vregs.ba_val + vregs.ba_inc) & 0x00ffffff;
	}
}

static void buggyboy_draw_objs(UINT8 *bitmap, int wide)
{
#define FRAC	16

	UINT32 offs;

	UINT32 x_mask;
	UINT32 x_stride;

	const UINT8 *const bug13  = (UINT8*)memory_region(REGION_USER3);
	const UINT8 *const bug18s = bug13 + 0x2000;
	const UINT8 *const bb8    = (UINT8*)memory_region(REGION_PROMS) + 0x1600;

	const UINT8 *const bug16s = (UINT8*)memory_region(REGION_USER2);
	const UINT8 *const bug17s = bug16s + 0x8000;

	const UINT8 *const bb9o = (UINT8*)memory_region(REGION_PROMS) + 0x500;
	const UINT8 *const bb9e = bb9o + 0x800;

	const UINT8 *const pixdata_rgn = (UINT8*)memory_region(REGION_GFX2);

	if (wide)
	{
		x_mask = 0x7ff;
		x_stride = 768;
	}
	else
	{
		x_mask = 0x3ff;
		x_stride = 256;
	}

	for (offs = 0; offs <= buggyboy_objram_size; offs += 8)
	{
		UINT32	x;
		UINT32	y;
		UINT32	gxflip;

		UINT32	x_scale;
		UINT32	x_step;
		UINT16	y_scale;
		UINT16	y_step;

		UINT8	pctmp0_7;
		UINT8	code;

		if ((buggyboy_objram[offs] & 0xff00) == 0xff00)
			break;

		x_scale = buggyboy_objram[offs + 2] & 0xff;

		if (x_scale == 0)
			continue;

		y_scale = buggyboy_objram[offs + 1];
		y_step  = buggyboy_objram[offs + 3];

		code = buggyboy_objram[offs] & 0xff;

		pctmp0_7 = buggyboy_objram[offs + 2] >> 8;

		gxflip = (pctmp0_7 & 0x80) >> 7;

		y = (buggyboy_objram[offs] >> 8) + 1;

		for (; y < 240; ++y)
		{
			UINT32	rom_addr2	= 0;
			UINT8	bug17s_data	= 0;
			UINT8	bug16s_data;

			if (y_scale & 0x8000)
				break;

			{
				UINT32	psa0_12;
				UINT32	bug13_addr;
				UINT32	bug13_data;
				UINT32	rom_addr;
				UINT32	x_acc;
				UINT32	newtile = 1;
				UINT32	dataend = 0;
				UINT8	data1 = 0;
				UINT8	data2 = 0;
				UINT32	xflip = 0;
				UINT32	opcd10_11;
				UINT32	opcd8_9;
				UINT32	opcd0_11 = 0;
				UINT32	lasttile = 0;

				bug13_addr = code << 4;
				bug13_addr |= ((y_scale >> 11) & 0xf);
				bug13_data = bug13[bug13_addr];

				if (bug13_data == 0xff)
					break;

				psa0_12  = (((code & 0x80) << 5) | ((code & 0x40) << 6)) & 0x1000;
				psa0_12 |= ((bb8[code] << 8) | bug13_data) & 0x1fff;

				rom_addr = (psa0_12 & ~0xff) << 2;

				x_step = (128 << FRAC) / x_scale;
				x_acc = (psa0_12 & 0xff) << (FRAC + 5);

				x = buggyboy_objram[offs + 4] & x_mask;

				for (;;)
				{

					if (newtile)
					{
						UINT32	pscb0_11;
						UINT32	psbb0_15;
						UINT32	psbb6_7;
						UINT32	rombank;
						UINT8	*romptr;
						UINT32	bug18s_data;
						UINT32	low_addr = ((x_acc >> (FRAC + 3)) & x_mask);

						if (gxflip)
						{
							UINT32	xor_mask;

							if (BIT(psa0_12, 11) || !BIT(psa0_12, 12))
								xor_mask = 0xf;
							else if (!BIT(psa0_12, 9))
								xor_mask = 0x7;
							else
								xor_mask = 0x3;

							rom_addr2 = rom_addr + (low_addr ^ xor_mask);
						}
						else
							rom_addr2 = rom_addr + low_addr;

						bug17s_data = bug17s[rom_addr2 & 0x7fff];

						if ((bug17s_data & 0x40) && dataend)
							lasttile = 1;

						dataend |= (bug17s_data & 0x40);

						bug16s_data = bug16s[rom_addr2 & 0x7fff];
						psbb0_15 = (bug17s_data << 8) | bug16s_data;
						psbb6_7 = (BIT(psbb0_15, 12) ? psbb0_15 : (pctmp0_7 << 6)) & 0xc0;

						pscb0_11 = ((((psbb0_15 & ~0xc0) | psbb6_7) << 3) | ((y_scale >> 8) & 7)) & 0x7fff;

						rombank = ((BIT(pctmp0_7, 4) << 1) | BIT(psbb0_15, 13)) & 3;

						romptr = (UINT8*)(pixdata_rgn + rombank * (0x8000 * 2));

						data1 = *(pscb0_11 + romptr);
						data2 = *(pscb0_11 + romptr + 0x8000);

						xflip = gxflip ^ !BIT(psbb0_15, 15);

						bug18s_data = bug18s[ (BIT(pctmp0_7, 4)  << 13)	|
											  (BIT(psbb0_15, 13) << 12)	|
											  (psbb0_15 & ~0xf0c0)		|
											  psbb6_7 ];

						opcd10_11 = ((pctmp0_7 << 8) & 0xc00) ^ 0xc00;
						opcd8_9 = ((pctmp0_7 & 0x60) << 3);
						opcd0_11 = (opcd10_11 | opcd8_9 | bug18s_data) & 0xfff;

						newtile = 0;
					}

					if (x < x_stride)
					{
						UINT8	pix;
						UINT8	bit;

						bit	= (x_acc >> FRAC) & 7;

						if (xflip)
							bit ^= 7;

						pix = (((data1 >> bit) & 1) << 1) | ((data2 >> bit) & 1);

						if (!(!(opcd0_11 & 0x80) && !pix))
						{
							UINT8 color;
							UINT32 bb9_addr;

							bb9_addr = ((opcd0_11 << 1) & 0x600) | ((opcd0_11 & 0x7f) << 2) | pix;
							color = ((opcd0_11 >> 6) & 0x30);

							if (x & 1)
								color = ~(color | bb9o[bb9_addr]) & 0x3f;
							else
								color = ~(color | bb9e[bb9_addr]) & 0x3f;

							*(bitmap + x_stride*y + x) = 0x40 | color;
						}
					}

					if ((((x_acc + x_step) >> (FRAC + 3)) & x_mask) != ((x_acc >> (FRAC + 3)) & x_mask))
					{
						if (lasttile)
							break;

						newtile = 1;
					}

					x = (x + 1) & x_mask;
					x_acc += x_step;
				}
			}
			y_scale += y_step;
		}
	}
}

WRITE16_HANDLER( buggyboy_gas_w )
{
	offset <<= 1;

	switch (offset & 0xe0)
	{
		case 0x00:
		{
			vregs.ba_inc &= ~0x0000ffff;
			vregs.ba_inc |= data;

			if (!(offset & 2))
				vregs.ba_val &= ~0x0000ffff;

			break;
		}
		case 0x20:
		{
			data &= 0xff;
			vregs.ba_inc &= ~0xffff0000;
			vregs.ba_inc |= data << 16;

			vregs.bank_mode = data & 1;

			if (!(offset & 2))
				vregs.ba_val &= ~0xffff0000;

			break;
		}
		case 0x40:
		{

			if (offset & 2)
				vregs.ba_val = (vregs.ba_inc + vregs.ba_val) & 0x00ffffff;

			break;
		}
		case 0x60:
		{
			vregs.h_inc = data;
			vregs.shift = 0;

			if (!(offset & 2))
				vregs.h_val = 0;

			break;
		}
		case 0x80:
		{

			if (offset & 2)
				vregs.h_val += vregs.h_inc;
			break;
		}
		case 0xa0:
		{
			vregs.wa8 = data >> 8;
			vregs.wa4 = 0;
			break;
		}
		case 0xe0:
		{
			cpunum_set_input_line(1, INPUT_LINE_TEST, CLEAR_LINE);
			vregs.flags = data;
			break;
		}
	}

	vregs.gas = data;
}

WRITE16_HANDLER( buggyboy_sky_w )
{
	vregs.sky = data;
}

WRITE16_HANDLER( buggyboy_slincs_w )
{
	if (offset == 1)
		vregs.slin_inc = data;
	else
		vregs.slin_inc = vregs.slin = 0;
}

WRITE16_HANDLER( buggyboy_scolst_w )
{
	vregs.scol = data;
}

static void bb_combine_layers(mame_bitmap *bitmap, int screen)
{
	UINT8 *chr_pal = memory_region(REGION_PROMS) + 0x400;
	UINT32 bmp_stride;
	UINT32 x_offset;
	UINT32 y;

	if (screen < 0)
	{
		bmp_stride = 256;
		x_offset = 0;
	}
	else
	{
		bmp_stride = 768;
		x_offset = 256 * screen;
	}

	for (y = 0; y < 240; ++y)
	{
		UINT32 x;

		UINT32 bmp_offset = y * bmp_stride + x_offset;

		UINT8 *chr_addr = bb_chr_bmp + bmp_offset;
		UINT8 *rod_addr = bb_rod_bmp + bmp_offset;
		UINT8 *obj_addr = bb_obj_bmp + bmp_offset;

		UINT32 sky_en = BIT(vregs.sky, 7);
		UINT32 sky_val = (((vregs.sky & 0x7f) + y) >> 2) & 0x3f;

		UINT16 *bmp_addr = ((UINT16 *)bitmap->line[y]) + x_offset;

		for (x = 0; x < 256; ++x)
		{
			UINT32 out_val;

			UINT32 char_val = *chr_addr++;
			UINT32 char_6_7 = (char_val & 0xc0) >> 2;

			UINT32 obj_val = *obj_addr++;
			UINT32 obj6	= BIT(obj_val, 6);

			UINT32 rod_val = *rod_addr++;
			UINT32 rod6	= BIT(rod_val, 6);

			UINT32 chr = !(BIT(char_val, 7) && (char_val & 3) );

			UINT32 sel =
			(
				( BIT(obj_val, 6) && chr) ||
				( sky_en && !(char_val & 3) && (!obj6 && !rod6) )
			) ? 0 : 1;

			sel |= (!(obj6 || rod6) || !chr) ? 2 : 0;

			if		(sel == 0)	out_val = obj_val & 0x3f;
			else if (sel == 1)	out_val = rod_val & 0x3f;
			else if (sel == 2)	out_val = sky_val;
			else				out_val = char_6_7 + chr_pal[char_val];

			*bmp_addr++ = (sel << 6) + out_val;
		}
	}
}

VIDEO_START( buggyboy )
{
	bb_chr_bmp = auto_malloc(sizeof(UINT8) * 3 * 256 * 240);
	bb_obj_bmp = auto_malloc(sizeof(UINT8) * 3 * 256 * 240);
	bb_rod_bmp = auto_malloc(sizeof(UINT8) * 3 * 256 * 240);

	interrupt_timer = timer_alloc(interrupt_callback);

	timer_adjust(interrupt_timer, cpu_getscanlinetime(CURSOR_YPOS), 0, 0);
	return 0;
}

VIDEO_START( buggyb1 )
{
	bb_chr_bmp = auto_malloc(sizeof(UINT8) * 256 * 240);
	bb_obj_bmp = auto_malloc(sizeof(UINT8) * 256 * 240);
	bb_rod_bmp = auto_malloc(sizeof(UINT8) * 256 * 240);

	interrupt_timer = timer_alloc(interrupt_callback);

	timer_adjust(interrupt_timer, cpu_getscanlinetime(CURSOR_YPOS), 0, 0);
	return 0;
}

VIDEO_EOF( buggyboy )
{
	vregs.slin += vregs.slin_inc;

	vregs.wave_lfsr = 0;
}

VIDEO_UPDATE( buggyboy )
{
	/* Road rendering advances hardware counters once for the complete three-screen frame. */
	memset(bb_obj_bmp, 0, 768*240);
	memset(bb_rod_bmp, 0, 768*240);
	buggyboy_draw_char(bb_chr_bmp, 1);
	buggyboy_draw_road(bb_rod_bmp);
	buggyboy_draw_objs(bb_obj_bmp, 1);
	bb_combine_layers(bitmap, 0);
	bb_combine_layers(bitmap, 1);
	bb_combine_layers(bitmap, 2);
}

VIDEO_UPDATE( buggyb1 )
{
	memset(bb_obj_bmp, 0, 256*240);

	buggyboy_draw_char(bb_chr_bmp, 0);
	buggyb1_draw_road(bb_rod_bmp, 0);
	buggyboy_draw_objs(bb_obj_bmp, 0);

	bb_combine_layers(bitmap, -1);
	}
