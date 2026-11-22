// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Tatsumi Lock-On hardware

    driver by Phil Bennett

***************************************************************************/

#include "driver.h"
#include "sound/2203intf.h"
#include <math.h>

#define PIXEL_CLOCK            (21000000.0 / 3)
#define FRAMEBUFFER_CLOCK      10000000.0
#define HBSTART                320
#define HBEND                  0
#define HTOTAL                 448
#define VBSTART                240
#define VBEND                  0
#define VTOTAL                 280

#define CURSOR_XPOS            168
#define CURSOR_YPOS            239
#define FRAMEBUFFER_MAX_X      431
#define FRAMEBUFFER_MAX_Y      416
#define FRAMEBUFFER_SIZE       512

#define V30_GND_ADDR           ((ctrl_reg & 0x3) << 16)
#define V30_OBJ_ADDR           ((ctrl_reg & 0x18) << 13)

#define RAM16(p,i)             ((p)[(i) * 2] | ((p)[(i) * 2 + 1] << 8))

/* memory pointers */
static UINT8 *char_ram;
static UINT8 *hud_ram;
static UINT8 *scene_ram;
static UINT8 *ground_ram;
static UINT8 *object_ram;

/* video-related */
static tilemap *lockon_tilemap;
static UINT8 gnd_reg[6];
static UINT8 rot_reg[16];
static UINT16 fb_buffer[2][FRAMEBUFFER_SIZE * FRAMEBUFFER_SIZE];
static UINT16 *front_buffer;
static UINT16 *back_buffer;
static mame_timer *bufend_timer;
static mame_timer *cursor_timer;

/* object palette RAM control */
static UINT32 iden;
static UINT8 obj_pal_ram[2048];
static UINT32 obj_pal_latch;
static UINT32 obj_pal_addr;

/* palette of the colour PROMs */
static UINT8 prom_rgb[1024][3];

/* misc */
static UINT8 ctrl_reg;
static UINT32 main_inten;
static UINT8 adc_data;


/*************************************
 *
 *  Palette decoding
 *
 *************************************/

/* output of the resistor network driven by TTL levels, normalized to 0..255 */
static int res_net(int bits, double rgnd)
{
	static const double resistors[5] = { 4700, 2200, 1000, 470, 220 };
	double vout[2];
	int k;

	for (k = 0; k < 2; ++k)
	{
		int b = k ? 0x1f : bits;
		double num = 5.0 / 560;
		double den = 1.0 / 560;
		int i;

		if (rgnd != 0)
			den += 1.0 / rgnd;

		for (i = 0; i < 5; ++i)
		{
			den += 1.0 / resistors[i];
			if (BIT(b, i))
				num += 3.4 / resistors[i];
		}

		vout[k] = num / den;
	}

	{
		double v0;
		double num = 5.0 / 560;
		double den = 1.0 / 560;
		int i;

		if (rgnd != 0)
			den += 1.0 / rgnd;
		for (i = 0; i < 5; ++i)
			den += 1.0 / resistors[i];
		v0 = num / den;

		return (int)(((vout[0] - v0) / (vout[1] - v0)) * 255.0 + 0.5);
	}
}

static PALETTE_INIT( lockon )
{
	const UINT8 *prom = memory_region(REGION_PROMS);
	int i;

	for (i = 0; i < 1024; ++i)
	{
		UINT8 p1 = prom[i];
		UINT8 p2 = prom[i + 0x400];
		double rgnd = (p2 & 0x80) ? 0 : 580;
		int r = res_net((p2 >> 2) & 0x1f, rgnd);
		int g = res_net(((p1 >> 5) & 0x7) | (p2 & 3) << 3, rgnd);
		int b = res_net(p1 & 0x1f, rgnd);

		prom_rgb[i][0] = r;
		prom_rgb[i][1] = g;
		prom_rgb[i][2] = b;
		palette_set_color(i, r, g, b);
	}
}


/*************************************
 *
 *  Character tilemap handling
 *
 *************************************/

static WRITE8_HANDLER( lockon_char_w )
{
	if (char_ram[offset] != data)
	{
		char_ram[offset] = data;
		tilemap_mark_tile_dirty(lockon_tilemap, offset >> 1);
	}
}

static void get_lockon_tile_info(int tile_index)
{
	UINT32 data = RAM16(char_ram, tile_index);
	UINT32 tileno = data & 0x03ff;
	UINT32 col = (data >> 10) & 0x3f;

	col = (col & 0x1f) + (col & 0x20 ? 64 : 0);
	SET_TILE_INFO(0, tileno, col, 0)
}


/*************************************
 *
 *  Scene tilemap hardware
 *
 *************************************/

static void scene_draw(void)
{
	/* 3bpp characters */
	const UINT8 *gfx1 = memory_region(REGION_USER1);
	const UINT8 *gfx2 = gfx1 + 0x10000;
	const UINT8 *gfx3 = gfx1 + 0x20000;
	const UINT8 *clut = gfx1 + 0x30000;
	UINT32 scroll_h = (gnd_reg[0] | (gnd_reg[1] << 8)) & 0x1ff;
	UINT32 scroll_v = (gnd_reg[2] | (gnd_reg[3] << 8)) & 0x81ff;
	UINT32 x, y;

	for (y = 0; y < FRAMEBUFFER_MAX_Y; ++y)
	{
		UINT32 d0 = 0, d1 = 0, d2 = 0;
		UINT32 colour = 0;
		UINT32 y_offs;
		UINT32 x_offs;
		UINT32 y_gran;
		UINT32 ram_mask = 0x7ff;
		UINT16 *bmpaddr = back_buffer + y * FRAMEBUFFER_SIZE;

		y_offs = (y + scroll_v) & 0x1ff;

		/* clamp: stops tilemap wrapping when screen is rotated */
		if (BIT(scroll_v, 15) && (y_offs & 0x100))
			ram_mask = 0x7;

		x_offs = (scroll_h - 8) & 0x1ff;
		y_gran = y_offs & 7;

		if (x_offs & 7)
		{
			UINT32 tileidx;
			UINT16 addr = ((y_offs & ~7) << 3) + ((x_offs >> 3) & 0x3f);
			UINT16 ram_val = RAM16(scene_ram, addr & ram_mask);

			colour = (clut[ram_val & 0x7fff] & 0x3f) << 3;
			tileidx = ((ram_val & 0x0fff) << 3) + y_gran;

			d0 = gfx1[tileidx];
			d1 = gfx2[tileidx];
			d2 = gfx3[tileidx];
		}

		for (x = 0; x < FRAMEBUFFER_MAX_X; ++x)
		{
			UINT32 x_gran = (x_offs & 7) ^ 7;
			UINT32 col;

			if (!(x_offs & 7))
			{
				UINT32 tileidx;
				UINT16 addr = ((y_offs & ~7) << 3) + ((x_offs >> 3) & 0x3f);
				UINT16 ram_val = RAM16(scene_ram, addr & ram_mask);

				colour = (clut[ram_val & 0x7fff] & 0x3f) << 3;
				tileidx = ((ram_val & 0x0fff) << 3) + y_gran;

				d0 = gfx1[tileidx];
				d1 = gfx2[tileidx];
				d2 = gfx3[tileidx];
			}

			col = colour
					| (((d2 >> x_gran) & 1) << 2)
					| (((d1 >> x_gran) & 1) << 1)
					| ( (d0 >> x_gran) & 1);

			*bmpaddr++ = 0xa00 + col;

			x_offs = (x_offs + 1) & 0x1ff;
		}
	}
}


/*************************************
 *
 *  Ground hardware
 *
 *  Each framebuffer line corresponds to a three word entry in ground RAM,
 *  starting from offset 3:
 *
 *     FEDCBA9876543210
 *  0 |.............xxx|  Tile line (A0-A2 GFX ROMs)
 *    |...........xx...|  Tile index (A6-A5 GFX ROMs)
 *    |........xxx.....|  ROM lut address (A6-A4)
 *    |.xxxxxxx........|  ROM lut address (A13-A7)
 *    |x...............|  /Line enable
 *
 *  1 |........xxxxxxxx|  TZ2213 value
 *    |xxxxxxxx........|  X offset
 *
 *  2 |........xxxxxxxx|  TZ2213 DX
 *    |.......x........|  Carry
 *    |x...............|  End of list marker
 *
 *  An 8-bit ground control register controls the following:
 *
 *     76543210
 *    |......xx|  LUT ROM A15-A14
 *    |....xx..|  LUT ROM select
 *    |..xx....|  CLUT ROM A13-A12
 *    |.x......|  GFX ROM A15, CLUT ROM A14
 *    |x.......|  GFX ROM bank select (always 0 - only 1 bank is present)
 *
 *************************************/

static void bufend_callback(int param)
{
	cpunum_set_input_line_and_vector(1, 0, HOLD_LINE, 0xff);
}

/* get data for each 8x8x3 ground tile */
#define GET_GROUND_DATA()                                                                \
{                                                                                        \
	UINT32 gfx_a4_3  = (ls163 & 0xc) << 1;                                               \
	UINT32 lut_addr  = lut_address + ((ls163 >> 4) & 0xf);                               \
	UINT32 gfx_a14_7 = lut_rom[lut_addr] << 7;                                           \
	clut_addr = (lut_rom[lut_addr] << 4) | clut_a14_12 | clut_a4_3 | (ls163 & 0xc) >> 2; \
	gfx_addr  = gfx_a15 | gfx_a14_7 | gfx_a6_5 | gfx_a4_3 | gfx_a2_0;                    \
	pal = (clut_rom[clut_addr] << 3);                                                    \
	rom_data1 = gfx_rom[gfx_addr];                                                       \
	rom_data2 = gfx_rom[gfx_addr + 0x10000];                                             \
	rom_data3 = gfx_rom[gfx_addr + 0x20000];                                             \
}

static void ground_draw(void)
{
	/* ROM pointers */
	const UINT8 *gfx_rom  = memory_region(REGION_USER3);
	const UINT8 *lut_rom  = gfx_rom + 0x30000 + ((gnd_reg[4] >> 2) & 0x3 ? 0x10000 : 0);
	const UINT8 *clut_rom = gfx_rom + 0x50000;

	UINT32 lut_a15_14  = (gnd_reg[4] & 0x3) << 14;
	UINT32 clut_a14_12 = (gnd_reg[4] & 0x70) << 8;
	UINT32 gfx_a15 = (gnd_reg[4] & 0x40) << 9;
	UINT32 offs = 3;
	UINT32 y;

	for (y = 0; y < FRAMEBUFFER_MAX_Y; ++y)
	{
		UINT16 *bmpaddr = back_buffer + y * FRAMEBUFFER_SIZE;
		UINT32 ram0 = RAM16(ground_ram, offs);
		UINT32 ram1 = RAM16(ground_ram, offs + 1);
		UINT32 ram2 = RAM16(ground_ram, offs + 2);
		UINT8 ls163;
		UINT32 clut_addr;
		UINT32 gfx_addr;
		UINT8 rom_data1 = 0;
		UINT8 rom_data2 = 0;
		UINT8 rom_data3 = 0;
		UINT32 pal = 0;
		UINT32 x;

		/* draw this line? */
		if (!(ram0 & 0x8000))
		{
			UINT32 gfx_a2_0  =  ram0 & 0x0007;
			UINT32 gfx_a6_5  = (ram0 & 0x0018) << 2;
			UINT32 clut_a4_3 = (ram0 & 0x0018) >> 1;
			UINT8 tz2213_x  = ram1 & 0xff;
			UINT8 tz2213_dx = ram2 & 0xff;

			UINT32 lut_address = lut_a15_14 + ((ram0 & 0x7fe0) >> 1);
			UINT32 cy = ram2 & 0x0100;
			UINT32 color;
			UINT32 gpbal2_0_prev;

			ls163 = ram1 >> 8;

			gpbal2_0_prev = ((ls163 & 3) << 1) | BIT(tz2213_x, 7);

			if (gpbal2_0_prev)
				GET_GROUND_DATA();

			for (x = 0; x < FRAMEBUFFER_MAX_X; x++)
			{
				UINT32 tz2213_cy;
				UINT32 gpbal2_0 = ((ls163 & 3) << 1) | BIT(tz2213_x, 7);

				/* stepped into a new tile? */
				if (gpbal2_0 < gpbal2_0_prev)
					GET_GROUND_DATA();

				gpbal2_0_prev = gpbal2_0;

				color = pal;
				color += (rom_data1 >> gpbal2_0) & 0x1;
				color += ((rom_data2 >> gpbal2_0) & 0x1) << 1;
				color += ((rom_data3 >> gpbal2_0) & 0x1) << 2;

				*bmpaddr++ = 0x800 + color;

				/* update the counters */
				tz2213_cy = (UINT8)tz2213_dx > (UINT8)~(tz2213_x);
				tz2213_x = (tz2213_x + tz2213_dx);

				/* carry? */
				if (tz2213_cy || cy)
					++ls163;
			}
		}

		offs += 3;

		/* end of list marker */
		if (RAM16(ground_ram, offs + 2) & 0x8000)
			timer_adjust(bufend_timer, (1.0 / FRAMEBUFFER_CLOCK) * (FRAMEBUFFER_MAX_X * y), 0, 0);
	}
}


/*************************************
 *
 *  Object hardware
 *
 *  Customs (4 each, 1 per bitplane)
 *  TZA118 - Scaling
 *  TZ4203 - Objects with integrated line buffer.
 *
 *     FEDCBA9876543210
 *  0 |......xxxxxxxxxx|  Y position
 *    |....xx..........|  Object Y size
 *    |..xx............|  Object X size
 *    |.x..............|  Y flip
 *    |x...............|  X flip
 *  1 |........xxxxxxxx|  X/Y scale
 *    |.xxxxxxx........|  Colour
 *    |x...............|  End of list marker
 *  2 |xxxxxxxxxxxxxxxx|  Chunk ROM address
 *  3 |.....xxxxxxxxxxx|  X position
 *
 *************************************/

/*
   There's logic to prevent shadow pixels from being drawn against the scene tilemap,
   so that shadows don't appear against the sky.
*/
#define DRAW_OBJ_PIXEL(COLOR)                            \
do {                                                     \
	if (px < FRAMEBUFFER_MAX_X)                          \
	if (COLOR != 0xf)                                    \
	{                                                    \
		UINT8 clr = obj_pal_ram[(pal << 4) + COLOR];     \
		UINT16 *pix = (line + px);                       \
		if (!(clr == 0xff && ((*pix & 0xe00) == 0xa00))) \
			*pix = 0x400 + clr;                          \
	}                                                    \
	px = (px + 1) & 0x7ff;                               \
} while(0)

static void objects_draw(void)
{
	const UINT8  *romlut = memory_region(REGION_USER5);
	const UINT16 *chklut = (const UINT16 *)memory_region(REGION_USER6);
	const UINT8  *gfxrom = memory_region(REGION_USER4);
	const UINT8  *sproms = memory_region(REGION_PROMS) + 0x800;
	UINT32 offs;

	for (offs = 0; offs < 256; offs += 4)
	{
		/* retrieve the object attributes */
		UINT32 attr0     = RAM16(object_ram, offs);
		UINT32 attr1     = RAM16(object_ram, offs + 1);
		UINT32 ypos      = attr0 & 0x03ff;
		UINT32 xpos      = RAM16(object_ram, offs + 3) & 0x07ff;
		UINT32 ysize     = (attr0 >> 10) & 0x3;
		UINT32 xsize     = (attr0 >> 12) & 0x3;
		UINT32 yflip     = BIT(attr0, 14);
		UINT32 xflip     = BIT(attr0, 15);
		UINT32 scale     = attr1 & 0xff;
		UINT32 pal       = (attr1 >> 8) & 0x7f;
		UINT32 opsta     = RAM16(object_ram, offs + 2);
		UINT32 opsta15_8 = opsta & 0xff00;
		UINT32 lines;
		UINT32 y;

		if (iden)
		{
			obj_pal_ram[(pal << 4) + obj_pal_addr] = obj_pal_latch;
			break;
		}

		/* how many lines will this sprite occupy? */
		lines = scale >> (3 - ysize);

		/* account for line buffering */
		ypos -= 1;

		for (y = 0; y < FRAMEBUFFER_MAX_Y; y++)
		{
			UINT32 cy = (y + ypos) & 0x3ff;
			UINT32 optab;
			UINT32 lutaddr;
			UINT32 tile;
			UINT8 cnt;
			UINT32 yidx;
			UINT16 *line = back_buffer + y * FRAMEBUFFER_SIZE;
			UINT32 px = xpos;

			/* outside the limits? */
			if (cy & 0x300)
				continue;

			if ((cy & 0xff) >= lines)
				break;

			lutaddr = (scale & 0x80 ? 0x8000 : 0) | ((scale & 0x7f) << 8) | (cy & 0xff);
			optab = romlut[lutaddr] & 0x7f;

			if (yflip)
				optab ^= 7;

			yidx = (optab & 7);

			/* now calculate the lower 7-bits of the LUT ROM address */
			cnt = (optab >> 3) * (1 << xsize);

			if (xflip)
				cnt ^= 7 >> (3 - xsize);
			if (yflip)
				cnt ^= (0xf >> (3 - ysize)) * (1 << xsize);

			cnt = (cnt + (opsta & 0xff));

			for (tile = 0; tile < (1U << xsize); ++tile)
			{
				UINT16 sc;
				UINT16 scl;
				UINT32 x;
				UINT32 tileaddr;
				UINT16 td0, td1, td2, td3;
				UINT32 j;
				UINT32 bank;

				scl = scale & 0x7f;
				tileaddr = (chklut[opsta15_8 + cnt] & 0x7fff);
				bank = ((tileaddr >> 12) & 3) * 0x40000;
				tileaddr = bank + ((tileaddr & ~0xf000) << 3);

				if (xflip)
					--cnt;
				else
					++cnt;

				/* draw two 8x8 tiles */
				for (j = 0; j < 2; ++j)
				{
					UINT32 tileadd = tileaddr + (0x20000 * (j ^ xflip));

					/* retrieve scale values from PROMs */
					sc = sproms[(scl << 4) + (tile * 2) + j];

					/* data from ROMs is inverted */
					td3 = gfxrom[tileadd + yidx] ^ 0xff;
					td2 = gfxrom[tileadd + 0x8000 + yidx] ^ 0xff;
					td1 = gfxrom[tileadd + 0x10000 + yidx] ^ 0xff;
					td0 = gfxrom[tileadd + 0x18000 + yidx] ^ 0xff;

					if (scale & 0x80)
					{
						for (x = 0; x < 8; ++x)
						{
							UINT8 col;
							UINT8 pix = x;

							if (!xflip)
								pix ^= 0x7;

							col = BIT(td0, pix)
								| (BIT(td1, pix) << 1)
								| (BIT(td2, pix) << 2)
								| (BIT(td3, pix) << 3);

							DRAW_OBJ_PIXEL(col);

							if (BIT(sc, x))
								DRAW_OBJ_PIXEL(col);
						}
					}
					else
					{
						for (x = 0; x < 8; ++x)
						{
							UINT8 col;
							UINT8 pix = x;

							if (BIT(sc, x))
							{
								if (!xflip)
									pix ^= 0x7;

								col = BIT(td0, pix)
									| (BIT(td1, pix) << 1)
									| (BIT(td2, pix) << 2)
									| (BIT(td3, pix) << 3);

								DRAW_OBJ_PIXEL(col);
							}
						}
					}
				}
			}
		}

		/* check for the end of list marker */
		if (attr1 & 0x8000)
			return;
	}
}

/* the mechanism used by the object CPU to update the object ASICs palette RAM */
static WRITE8_HANDLER( lockon_tza112_w )
{
	if (iden && !(offset & 1))
	{
		obj_pal_latch = data;
		obj_pal_addr = (offset >> 1) & 0xf;
		objects_draw();
	}
}

static READ8_HANDLER( lockon_obj_4000_r )
{
	return 0xff;
}

static WRITE8_HANDLER( lockon_obj_4000_w )
{
	if (!(offset & 1))
		iden = data & 1;
}


/*************************************
 *
 *  Frame buffer rotation hardware
 *
 *    FEDCBA9876543210
 *  0 |........xxxxxxxx|  X start address
 *    |.......x........|  Direction
 *  1 |........xxxxxxxx|  TZ2213 IC65 value
 *    |.......x........|  TZ2213 IC65 /enable
 *  2 |........xxxxxxxx|  TZ2213 IC65 delta
 *  3 |........xxxxxxxx|  TZ2213 IC106 delta
 *    |.......x........|  TZ2213 IC106 enable
 *  4 |.......xxxxxxxxx|  Y start address
 *  5 |........xxxxxxxx|  TZ2213 IC66 value
 *  6 |........xxxxxxxx|  TZ2213 IC66 delta
 *    |.......x........|  TZ2213 IC65 /enable
 *  7 |........xxxxxxxx|  TZ2213 IC107 delta
 *    |.......x........|  TZ2213 /enable
 *    |......x.........|  Direction
 *
 *************************************/

static WRITE8_HANDLER( lockon_fb_clut_w )
{
	if (!(offset & 1))
		palette_set_color(0x400 + (offset >> 1), prom_rgb[0x300 + data][0], prom_rgb[0x300 + data][1], prom_rgb[0x300 + data][2]);
}

static WRITE8_HANDLER( lockon_rotate_w )
{
	rot_reg[offset & 0xf] = data;
}

#define INCREMENT(ACC, CNT)                  \
do {                                         \
	carry = (UINT8)d##ACC > (UINT8)~ACC;     \
	ACC += d##ACC;                           \
	if (carry) ++CNT;                        \
} while(0)

#define DECREMENT(ACC, CNT)                  \
do {                                         \
	carry = (UINT8)d##ACC > (UINT8)ACC;      \
	ACC -= d##ACC;                           \
	if (carry) --CNT;                        \
} while(0)

static void rotate_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	UINT32 xsal  = RAM16(rot_reg, 0) & 0x1ff;
	UINT32 x0ll  = RAM16(rot_reg, 1) & 0xff;
	UINT32 dx0ll = RAM16(rot_reg, 2) & 0x1ff;
	UINT32 dxll  = RAM16(rot_reg, 3) & 0x1ff;
	UINT32 ysal  = RAM16(rot_reg, 4) & 0x1ff;
	UINT32 y0ll  = RAM16(rot_reg, 5) & 0xff;
	UINT32 dy0ll = RAM16(rot_reg, 6) & 0x1ff;
	UINT32 dyll  = RAM16(rot_reg, 7) & 0x3ff;

	/* counters */
	UINT32 cxy = xsal & 0xff;
	UINT32 cyy = ysal & 0x1ff;

	/* accumulator values and deltas */
	UINT8 axy  = x0ll & 0xff;
	UINT8 daxy = dx0ll & 0xff;
	UINT8 ayy  = y0ll & 0xff;
	UINT8 dayy = dy0ll & 0xff;
	UINT8 dayx = dyll & 0xff;
	UINT8 daxx = dxll & 0xff;

	UINT32 xy_up = BIT(xsal, 8);
	UINT32 yx_up = BIT(dyll, 9);
	UINT32 axx_en  = !BIT(dxll, 8);
	UINT32 ayx_en  = !BIT(dyll, 8);
	UINT32 axy_en  = !BIT(dx0ll, 8);
	UINT32 ayy_en  = !BIT(dy0ll, 8);
	UINT32 x, y;

	for (y = 0; y <= cliprect->max_y; ++y)
	{
		UINT32 carry;
		UINT16 *dst = (UINT16 *)bitmap->line[y];

		UINT32 cx = cxy;
		UINT32 cy = cyy;

		UINT8 axx = axy;
		UINT8 ayx = ayy;

		for (x = 0; x <= cliprect->max_x; ++x)
		{
			cx &= 0x1ff;
			cy &= 0x1ff;

			*dst++ = front_buffer[cy * FRAMEBUFFER_SIZE + cx];

			if (axx_en)
				INCREMENT(axx, cx);
			else
				++cx;

			if (ayx_en)
			{
				if (yx_up)
					INCREMENT(ayx, cy);
				else
					DECREMENT(ayx, cy);
			}
			else
			{
				if (yx_up)
					++cy;
				else
					--cy;
			}
		}

		if (axy_en)
		{
			if (xy_up)
				INCREMENT(axy, cxy);
			else
				DECREMENT(axy, cxy);
		}
		else
		{
			if (xy_up)
				++cxy;
			else
				--cxy;
		}

		if (ayy_en)
			INCREMENT(ayy, cyy);
		else
			++cyy;

		cxy &= 0xff;
		cyy &= 0x1ff;
	}
}


/*************************************
 *
 *  HUD drawing hardware
 *
 *  A sprite layer that uses 8x8x1bpp tiles to form bigger sprites
 *
 *  0  |.......xxxxxxxxx|  Y Position
 *     |xxxxxxx.........|  Code
 *  1  |.......xxxxxxxxx|  X Position
 *     |....xxx.........|  Colour
 *     |.xxx............|  Sprite width (0=8, 1=16, 2=24, 3=32 pixels, etc.)
 *     |x...............|  End of list marker
 *
 *************************************/

static void hud_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const UINT8 *tile_rom = memory_region(REGION_USER2);
	UINT32 offs;

	for (offs = 0; offs < 256; offs += 2)
	{
		UINT32 attr0 = RAM16(hud_ram, offs);
		UINT32 attr1 = RAM16(hud_ram, offs + 1);
		UINT32 y_pos;
		UINT32 x_pos;
		UINT32 y_size;
		UINT32 x_size;
		UINT32 layout;
		UINT16 colour;
		UINT32 code;
		UINT32 rom_a12_7;
		UINT32 y;

		/* end of sprite list marker */
		if (attr1 & 0x8000)
			break;

		y_pos  = attr0 & 0x1ff;
		x_pos  = attr1 & 0x1ff;
		x_size = (attr1 >> 12) & 7;
		code   = (attr0 >> 9) & 0x7f;
		colour = 0x200 + ((attr1 >> 9) & 7);
		layout = (code >> 5) & 3;

		rom_a12_7 = (code & 0xfe) << 6;

		/* account for line buffering */
		y_pos -= 1;

		if (layout == 3)
			y_size = 32;
		else if (layout == 2)
			y_size = 16;
		else
			y_size = 8;

		for (y = cliprect->min_y; y <= (UINT32)cliprect->max_y; ++y)
		{
			UINT32 xt;
			UINT32 cy;

			cy = y_pos + y;

			if (cy < 0x200)
				continue;

			if ((cy & 0xff) == y_size)
				break;

			for (xt = 0; xt <= x_size; ++xt)
			{
				UINT32 rom_a6_3;
				UINT32 px;
				UINT8 gfx_strip;

				if (layout == 3)
					rom_a6_3 = (BIT(cy, 4) << 3) | (BIT(cy, 3) << 2) | (BIT(xt, 1) << 1) | BIT(xt, 0);
				else if (layout == 2)
					rom_a6_3 = ((BIT(code, 0) << 3) | (BIT(xt, 1) << 2) | (BIT(cy, 3) << 1) | (BIT(xt, 0)));
				else
					rom_a6_3 = (BIT(code, 0) << 3) | (xt & 7);

				rom_a6_3 <<= 3;

				/* get tile data */
				gfx_strip = tile_rom[rom_a12_7 | rom_a6_3 | (cy & 7)];

				if (gfx_strip == 0)
					continue;

				for (px = 0; px < 8; ++px)
				{
					UINT32 x = x_pos + (xt << 3) + px;

					if (x <= (UINT32)cliprect->max_x)
					{
						UINT16 *dst = ((UINT16 *)bitmap->line[y] + x);

						if (BIT(gfx_strip, px ^ 7) && *dst > 255)
							*dst = colour;
					}
				}
			}
		}
	}
}


/*************************************
 *
 *  Driver video handlers
 *
 *************************************/

static void cursor_callback(int param)
{
	if (main_inten)
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xff);
}

static VIDEO_START( lockon )
{
	lockon_tilemap = tilemap_create(get_lockon_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	if (!lockon_tilemap)
		return 1;
	tilemap_set_transparent_pen(lockon_tilemap, 0);

	front_buffer = fb_buffer[0];
	back_buffer = fb_buffer[1];
	memset(fb_buffer, 0, sizeof(fb_buffer));
	memset(obj_pal_ram, 0, sizeof(obj_pal_ram));

	/* timer for ground display list callback */
	bufend_timer = timer_alloc(bufend_callback);

	/* timer for the CRTC cursor pulse */
	cursor_timer = timer_alloc(cursor_callback);
	timer_adjust(cursor_timer, (CURSOR_YPOS * HTOTAL + CURSOR_XPOS) / PIXEL_CLOCK, 0, HTOTAL * VTOTAL / PIXEL_CLOCK);

	return 0;
}

static VIDEO_UPDATE( lockon )
{
	/* if screen output is disabled, fill with black */
	if (!BIT(ctrl_reg, 7))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* scan out the frame buffer in rotated order */
	rotate_draw(bitmap, cliprect);

	/* draw the character tilemap */
	tilemap_draw(bitmap, cliprect, lockon_tilemap, 0, 0);

	/* draw the HUD */
	hud_draw(bitmap, cliprect);
}

static VIDEO_EOF( lockon )
{
	UINT16 *tmp;

	/* swap the frame buffers */
	tmp = front_buffer;
	front_buffer = back_buffer;
	back_buffer = tmp;

	/* draw the frame buffer layers */
	scene_draw();
	ground_draw();
	objects_draw();
}


/*************************************
 *
 *  Machine functions
 *
 *************************************/

/*
    Main control register

     76543210
    |......xx|  Ground CPU A17-A16
    |.....x..|  /Ground CPU bus req.
    |...xx...|  Object CPU A17-A16
    |..x.....|  /Object CPU bus req.
    |.x......|  /Z80 bus req.
    |x.......|  Display enable
*/

static WRITE8_HANDLER( adrst_w )
{
	if (offset & 1)
		return;

	ctrl_reg = data;

	/* bus mastering for shared access */
	cpunum_set_input_line(1, INPUT_LINE_HALT, data & 0x04 ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(2, INPUT_LINE_HALT, data & 0x20 ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(3, INPUT_LINE_HALT, data & 0x40 ? CLEAR_LINE : ASSERT_LINE);
}

static UINT8 cpu_space_r(int cpunum, offs_t address)
{
	UINT8 result;

	cpuintrf_push_context(cpunum);
	result = program_read_byte(address);
	cpuintrf_pop_context();
	return result;
}

static void cpu_space_w(int cpunum, offs_t address, UINT8 data)
{
	cpuintrf_push_context(cpunum);
	program_write_byte(address, data);
	cpuintrf_pop_context();
}

static READ8_HANDLER( main_gnd_r )
{
	return cpu_space_r(1, V30_GND_ADDR | offset);
}

static WRITE8_HANDLER( main_gnd_w )
{
	cpu_space_w(1, V30_GND_ADDR | offset, data);
}

static READ8_HANDLER( main_obj_r )
{
	return cpu_space_r(2, V30_OBJ_ADDR | offset);
}

static WRITE8_HANDLER( main_obj_w )
{
	cpu_space_w(2, V30_OBJ_ADDR | offset, data);
}

static WRITE8_HANDLER( tst_w )
{
	if (offset < 0x1000)
	{
		cpu_space_w(1, V30_GND_ADDR | offset, data);
		cpu_space_w(2, V30_OBJ_ADDR | offset, data);
	}
}

/* only the even bytes are connected to the Z80 bus */
static READ8_HANDLER( main_z80_r )
{
	if (offset & 1)
		return 0xff;
	return cpu_space_r(3, offset >> 1);
}

static WRITE8_HANDLER( main_z80_w )
{
	if (!(offset & 1))
		cpu_space_w(3, offset >> 1, data);
}

static WRITE8_HANDLER( inten_w )
{
	main_inten = 1;
}

static WRITE8_HANDLER( emres_w )
{
	watchdog_reset_w(0, 0);
	main_inten = 0;
}

static READ8_HANDLER( dsw_r )
{
	return readinputportbytag("DSW") >> (offset * 8);
}

static WRITE8_HANDLER( gnd_reg_w )
{
	gnd_reg[offset] = data;
}


/*************************************
 *
 *  CPU memory maps
 *
 *************************************/

static ADDRESS_MAP_START( main_v30, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00000, 0x03fff) AM_RAM
	AM_RANGE(0x04000, 0x04003) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x06000, 0x06001) AM_READ(dsw_r)
	AM_RANGE(0x08000, 0x081ff) AM_RAM AM_BASE(&hud_ram)
	AM_RANGE(0x09000, 0x09fff) AM_READWRITE(MRA8_RAM, lockon_char_w) AM_BASE(&char_ram)
	AM_RANGE(0x0a000, 0x0a001) AM_WRITE(adrst_w)
	AM_RANGE(0x0b000, 0x0bfff) AM_WRITE(lockon_rotate_w)
	AM_RANGE(0x0c000, 0x0cfff) AM_WRITE(lockon_fb_clut_w)
	AM_RANGE(0x0e000, 0x0e001) AM_WRITE(inten_w)
	AM_RANGE(0x0f000, 0x0f001) AM_WRITE(emres_w)
	AM_RANGE(0x10000, 0x1ffff) AM_WRITE(tst_w)
	AM_RANGE(0x20000, 0x2ffff) AM_READWRITE(main_z80_r, main_z80_w)
	AM_RANGE(0x30000, 0x3ffff) AM_READWRITE(main_gnd_r, main_gnd_w)
	AM_RANGE(0x40000, 0x4ffff) AM_READWRITE(main_obj_r, main_obj_w)
	AM_RANGE(0x50000, 0x5ffff) AM_MIRROR(0x80000) AM_ROM
	AM_RANGE(0x60000, 0x6ffff) AM_MIRROR(0x80000) AM_ROM
	AM_RANGE(0x70000, 0x7ffff) AM_MIRROR(0x80000) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ground_v30, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00000, 0x03fff) AM_RAM
	AM_RANGE(0x04000, 0x04fff) AM_RAM AM_BASE(&scene_ram)
	AM_RANGE(0x08000, 0x08fff) AM_RAM AM_BASE(&ground_ram)
	AM_RANGE(0x0c000, 0x0c005) AM_WRITE(gnd_reg_w)
	AM_RANGE(0x20000, 0x2ffff) AM_MIRROR(0xc0000) AM_ROM
	AM_RANGE(0x30000, 0x3ffff) AM_MIRROR(0xc0000) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( object_v30, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00000, 0x03fff) AM_RAM
	AM_RANGE(0x04000, 0x04001) AM_READWRITE(lockon_obj_4000_r, lockon_obj_4000_w)
	AM_RANGE(0x08000, 0x08fff) AM_WRITE(lockon_tza112_w)
	AM_RANGE(0x0c000, 0x0c1ff) AM_RAM AM_BASE(&object_ram)
	AM_RANGE(0x30000, 0x3ffff) AM_MIRROR(0xc0000) AM_ROM
ADDRESS_MAP_END


/*************************************
 *
 *  Sound related
 *
 *************************************/

static WRITE8_HANDLER( sound_vol )
{
#define LO_SHUNT    250.0
#define LO_R0       5600.0
#define LO_R1       10000.0
#define LO_R2       22000.0
#define LO_R3       47000.0
#define LO_R0S      (1/(1/LO_SHUNT + 1/LO_R0))
#define LO_R1S      (1/(1/LO_SHUNT + 1/LO_R1))
#define LO_R2S      (1/(1/LO_SHUNT + 1/LO_R2))
#define LO_R3S      (1/(1/LO_SHUNT + 1/LO_R3))
#define LO_RI       100000.0
#define LO_RP       100000.0

	static const double gains[16] =
	{
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2   + LO_R1   + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2   + LO_R1   + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2   + LO_R1S  + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2   + LO_R1S  + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2S  + LO_R1   + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2S  + LO_R1   + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2S  + LO_R1S  + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3   + LO_R2S  + LO_R1S  + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2   + LO_R1   + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2   + LO_R1   + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2   + LO_R1S  + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2   + LO_R1S  + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2S  + LO_R1   + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2S  + LO_R1   + LO_R0S)) ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2S  + LO_R1S  + LO_R0))  ) / LO_RI,
		( 1 / (1/LO_RP + 1/(LO_R3S  + LO_R2S  + LO_R1S  + LO_R0S)) ) / LO_RI,
	};

	/* the mixer has a single volume per channel, so left and right are averaged */
	float gain = (gains[data & 0xf] + gains[data >> 4]) / 2;
	int ch;

	for (ch = 1; ch < 4; ++ch)
		sndti_set_output_gain(SOUND_YM2203, 0, ch, gain);
}

static WRITE8_HANDLER( adc_start_w )
{
	switch (offset & 7)
	{
		case 0: adc_data = readinputportbytag("ADC_BANK"); break;
		case 1: adc_data = readinputportbytag("ADC_PITCH"); break;
		case 2: adc_data = readinputportbytag("ADC_MISSILE"); break;
		case 3: adc_data = readinputportbytag("ADC_HOVER"); break;
		default: adc_data = 0; break;
	}
}

static READ8_HANDLER( adc_data_r )
{
	return adc_data;
}

static ADDRESS_MAP_START( sound_prg, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x6fff) AM_ROM
	AM_RANGE(0x7000, 0x7000) AM_WRITE(sound_vol)
	AM_RANGE(0x7400, 0x7407) AM_READWRITE(adc_data_r, adc_start_w)
	AM_RANGE(0x7800, 0x7fff) AM_MIRROR(0x8000) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
ADDRESS_MAP_END

static void ym2203_irq(int irq)
{
	cpunum_set_input_line(3, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_HANDLER( ym2203_in_a )
{
	return readinputportbytag("YM2203");
}

static WRITE8_HANDLER( ym2203_out_b )
{
	coin_counter_w(0, ~data & 0x80);
	coin_counter_w(1, ~data & 0x40);
	coin_counter_w(2, ~data & 0x20);

	/* 'Lock-On' lamp */
	set_led_status(1, ~data & 0x10);
}

static struct YM2203interface ym2203_interface =
{
	ym2203_in_a,
	0,
	0,
	ym2203_out_b,
	ym2203_irq
};


/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( lockon )
	PORT_START_TAG("DSW")
	/* DSW 1 - verified on PCB as a 8 position dipswitch block */
	PORT_DIPNAME( 0x0003, 0x0000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0003, "5" )
	PORT_DIPNAME( 0x000c, 0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Hardest ) )

	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0000, "150K & every 200K" )
	PORT_DIPSETTING(      0x0010, "200K & every 200K" )

	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )

	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )

	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	/* DSW 2 - verified on PCB as a 6 position dipswitch block */
	PORT_DIPNAME( 0x0700, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( Free_Play ) )

	PORT_DIPNAME( 0x3800, 0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_6C ) )

	/*
	    Wire jumper beside the dipswitches on PCB TF011.
	    To access the menu, press the service coin during
	    test mode.
	*/
	PORT_DIPNAME( 0x4000, 0x4000, "Enable H/W Tests Menu" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("YM2203")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) PORT_TOGGLE
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1  )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2  )
	PORT_DIPNAME( 0x40, 0x00, "Jumper 1" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Jumper 0" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("ADC_BANK")
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_X ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15)

	PORT_START_TAG("ADC_PITCH")
	PORT_BIT( 0xff, 0x7f, IPT_AD_STICK_Y ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(15)

	PORT_START_TAG("ADC_MISSILE")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON3 )

	PORT_START_TAG("ADC_HOVER")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_BUTTON4 )
INPUT_PORTS_END


/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout char_layout =
{
	8,8,
	1024,
	2,
	{ 0, 8*8*1024 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout, 0, 128 },
	{ -1 }
};


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_RESET( lockon )
{
	memset(gnd_reg, 0, sizeof(gnd_reg));
	memset(rot_reg, 0, sizeof(rot_reg));
	iden = 0;
	obj_pal_latch = 0;
	obj_pal_addr = 0;
	ctrl_reg = 0;
	main_inten = 0;
	adc_data = 0;
}

static MACHINE_DRIVER_START( lockon )
	MDRV_CPU_ADD(V30, 16000000/2)
	MDRV_CPU_PROGRAM_MAP(main_v30, 0)

	MDRV_CPU_ADD(V30, 16000000/2)
	MDRV_CPU_PROGRAM_MAP(ground_v30, 0)

	MDRV_CPU_ADD(V30, 16000000/2)
	MDRV_CPU_PROGRAM_MAP(object_v30, 0)

	MDRV_CPU_ADD(Z80, 16000000/4)
	MDRV_CPU_PROGRAM_MAP(sound_prg, 0)
	MDRV_CPU_IO_MAP(sound_io, 0)

	MDRV_FRAMES_PER_SECOND(PIXEL_CLOCK / (HTOTAL * VTOTAL))
	MDRV_VBLANK_DURATION(ceil(1000000.0 * (VTOTAL - VBSTART) * HTOTAL / PIXEL_CLOCK))
	MDRV_INTERLEAVE(12)
	MDRV_WATCHDOG_TIME_INIT(TIME_IN_MSEC(550))

	MDRV_MACHINE_RESET(lockon)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(HTOTAL, VTOTAL)
	MDRV_VISIBLE_AREA(HBEND, HBSTART - 1, VBEND, VBSTART - 1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024 + 2048)

	MDRV_PALETTE_INIT(lockon)
	MDRV_VIDEO_START(lockon)
	MDRV_VIDEO_EOF(lockon)
	MDRV_VIDEO_UPDATE(lockon)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 16000000/4)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.40)
	MDRV_SOUND_ROUTE(0, "right", 0.40)
	MDRV_SOUND_ROUTE(1, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
	MDRV_SOUND_ROUTE(2, "left", 1.0)
	MDRV_SOUND_ROUTE(2, "right", 1.0)
	MDRV_SOUND_ROUTE(3, "left", 1.0)
	MDRV_SOUND_ROUTE(3, "right", 1.0)
MACHINE_DRIVER_END


/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( lockon )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "lo1_02c.89", 0x60000, 0x8000, CRC(bbf17263) SHA1(96821a0ecd6efe6764380fef094f87c1d6e1d299) )
	ROM_LOAD16_BYTE( "lo1_03c.88", 0x60001, 0x8000, CRC(fa58fd36) SHA1(16af24027610bf6d3fdc4c3df3bf6d94c6776420) )
	ROM_LOAD16_BYTE( "lo1_04c.77", 0x50000, 0x8000, CRC(4a88576e) SHA1(80a8bd89cedebf080b2c08a6e81d3c2754024d8a) )
	ROM_LOAD16_BYTE( "lo1_05c.76", 0x50001, 0x8000, CRC(5a171b02) SHA1(f41f641136574e6af67c2245eb5a84799984474a) )
	ROM_LOAD16_BYTE( "lo1_00e.96", 0x70000, 0x8000, CRC(25eec97f) SHA1(884d4e75111d532afed9755bd678cb7678385a86) )
	ROM_LOAD16_BYTE( "lo1_01e.95", 0x70001, 0x8000, CRC(03f0391d) SHA1(53795eab8f1b33c86a72d2f0f1cbcf7faab0011b) )
	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "lo3_01a.30", 0x20000, 0x8000, CRC(3eacdb6b) SHA1(7934c36dac9253dec4d8910954f6f2ae85951fe9) )
	ROM_LOAD16_BYTE( "lo3_03a.33", 0x20001, 0x8000, CRC(4ce96d71) SHA1(cedbc33e86a93d11d5e11c2ef18bcf6390790a88) )
	ROM_LOAD16_BYTE( "lo3_00b.29", 0x30000, 0x8000, CRC(1835dccb) SHA1(8dfb0fea61a3e61f4da3b7f0da02cd19df2e68be) )
	ROM_LOAD16_BYTE( "lo3_02b.32", 0x30001, 0x8000, CRC(2b8931d3) SHA1(f6f40b7857f3d47da8626450b1c1d3c46a1072ab) )
	ROM_REGION( 0x100000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "lo4_00b", 0x30000, 0x8000, CRC(5f6b5a50) SHA1(daf82cafcae86d05587c191b0ff194ca7950e130) )
	ROM_LOAD16_BYTE( "lo4_01b", 0x30001, 0x8000, CRC(7e88bcf2) SHA1(d541458ba6178ec3bce0e9b872b9fa1d8edb107c) )
	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "lo1_08b.24", 0x00000, 0x8000, CRC(73860ec9) SHA1(a94afa274321b9f9ac2184e133132f9829fb9485) )
	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "lo1_07a", 0x00000, 0x2000, CRC(73673b79) SHA1(246b80f0c465cefb7ce1c87dc90a58f0f0ea3e0d) )
	ROM_LOAD( "lo1_06a", 0x02000, 0x2000, CRC(c8205913) SHA1(c791ff14418873ce68b502440c3d7ccc1f9cc00e) )
	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "lo3_12a.120", 0x00000, 0x10000, CRC(a34262a7) SHA1(08204a4474ab1b07b9114da8af03442737922d3b) )
	ROM_LOAD( "lo3_11a.119", 0x10000, 0x10000, CRC(018efa36) SHA1(99eec3f06146627c7f7177b854424e7162ab7c8e) )
	ROM_LOAD( "lo3_10a.118", 0x20000, 0x10000, CRC(d5f4a8f3) SHA1(fcfaef46ef89c4b97970418a75d110271e94d55f) )
	ROM_LOAD( "lo3_13a.121", 0x30000, 0x10000, CRC(e44774a7) SHA1(010d95ea497690ddd2406b8fef1b0aee375a165e) )
	ROM_REGION( 0x2000, REGION_USER2, 0 )
	ROM_LOAD( "lo2_00.53", 0x00000, 0x2000, CRC(8affea15) SHA1(b7bcf0abde9c933e3f2c75c1f5e2ca3417d50ca1) )
	ROM_REGION( 0x60000, REGION_USER3, 0 )
	ROM_LOAD( "lo3_07a.94", 0x00000, 0x10000, CRC(cebc50e1) SHA1(f8b06ce576c3d41b0a8e2cc3ac60d3515d434812) )
	ROM_LOAD( "lo3_06a.92", 0x10000, 0x10000, CRC(f6b6ebdd) SHA1(30e92da3bf83c4bb30faf00cbf01664b993f137c) )
	ROM_LOAD( "lo3_05a.90", 0x20000, 0x10000, CRC(5b6f4c8e) SHA1(fc8b2c929c60fb0177ed3e407e3f0aacc5df8401) )
	ROM_LOAD( "lo3_08.104", 0x30000, 0x10000, CRC(f418cecd) SHA1(6cf2d13c9df86bad9c24609cb8387e817b5d4281) )
	ROM_LOAD( "lo3_09.105", 0x40000, 0x10000, CRC(3c245568) SHA1(9ff6a23d83627f55c9d4f68e0bd89927bfe10664) )
	ROM_LOAD( "lo3_04a.88", 0x50000, 0x10000, CRC(80b67ba9) SHA1(fdbef463b26cd13c43596310f585432c6e0896d0) )
	ROM_REGION( 0x100000, REGION_USER4, 0 )
	ROM_LOAD( "lo5_28a.76", 0x00000, 0x8000, CRC(1186f9b4) SHA1(55598552dafa8cccfb423fc3b65a7fa15831d75b) )
	ROM_LOAD( "lo5_20a.48", 0x08000, 0x8000, CRC(3c1a67b5) SHA1(399935830b32a457ad0de243dd3eb4d368d5c6a6) )
	ROM_LOAD( "lo5_08a.58", 0x10000, 0x8000, CRC(9dedeff5) SHA1(53b0917a4fde4053182d38ea7f99f66e52543c10) )
	ROM_LOAD( "lo5_00a.14", 0x18000, 0x8000, CRC(e9f23ce6) SHA1(4030384a0e8f47e8eea9483482ed1be264aec992) )
	ROM_LOAD( "lo5_24a.62", 0x20000, 0x8000, CRC(1892d083) SHA1(8ee92be93ac222ecc2d9f4fcda3099b1db67516c) )
	ROM_LOAD( "lo5_16a.32", 0x28000, 0x8000, CRC(c4500159) SHA1(e695e31e363cc954aab449f9d3dbc027e27fe7bf) )
	ROM_LOAD( "lo5_04a.28", 0x30000, 0x8000, CRC(099323bc) SHA1(001d30f4c3c27277fadac89dcf616ff89eb0ea1c) )
	ROM_LOAD( "lo5_12a.8",  0x38000, 0x8000, CRC(2f5164ab) SHA1(df775b9e1c3c605a85d44404e4db42e33e80e664) )
	ROM_LOAD( "lo5_29a.77", 0x40000, 0x8000, CRC(45353d8d) SHA1(45cacd36700d24ae9f6eeaebed2fc860ef2d2978) )
	ROM_LOAD( "lo5_21a.49", 0x48000, 0x8000, CRC(39ce2000) SHA1(05f8e6f364ad714232fcea5b535ed5e181febd1e) )
	ROM_LOAD( "lo5_10a.72", 0x50000, 0x8000, CRC(23eeec5a) SHA1(08edd997d773684d329ef554776bc7acff1ac4ce) )
	ROM_LOAD( "lo5_01a.15", 0x58000, 0x8000, CRC(528d1395) SHA1(0221f81900757c10f288807f5c9549b9fdf5390f) )
	ROM_LOAD( "lo5_25a.93", 0x60000, 0x8000, CRC(7f3418bd) SHA1(5e595500f996b71aa73c637a6fddced30d78e222) )
	ROM_LOAD( "lo5_17a.33", 0x68000, 0x8000, CRC(ccf138d3) SHA1(971e7abe5b4d1a9dc8fc71b1ded5d2b81bcffaf2) )
	ROM_LOAD( "lo5_06a.44", 0x70000, 0x8000, CRC(be539b01) SHA1(1eebcbc592c51a676409b5be6c5d6609cd7118c9) )
	ROM_LOAD( "lo5_14a.22", 0x78000, 0x8000, CRC(e63cd59e) SHA1(0518461acdc6c65dca8f21ca29bf528197e7cabe) )
	ROM_LOAD( "lo5_30a.78", 0x80000, 0x8000, CRC(7d3993c5) SHA1(fb18daffcfc46bc1e1cfdee928eea861494af221) )
	ROM_LOAD( "lo5_22a.50", 0x88000, 0x8000, CRC(b1ed0361) SHA1(4bdf439026a858fdd929d5a7baac7d76f51550c5) )
	ROM_LOAD( "lo5_09a.59", 0x90000, 0x8000, CRC(953289bc) SHA1(197066af45c1193c36cd59b4b72b14f1c3bdd33e) )
	ROM_LOAD( "lo5_02a.16", 0x98000, 0x8000, CRC(07aa32a1) SHA1(712b1983747acdd754d3abe934642cbc02ee13f2) )
	ROM_LOAD( "lo5_26a.64", 0xa0000, 0x8000, CRC(a0b5c040) SHA1(ef63f89a368bc73eb77fc02d83b499a0231c1989) )
	ROM_LOAD( "lo5_18a.34", 0xa8000, 0x8000, CRC(89884b24) SHA1(23c1fcc97f3a1abcaad413f4448db26f7c55fd5e) )
	ROM_LOAD( "lo5_05a.29", 0xb0000, 0x8000, CRC(f6b775a2) SHA1(e0146866f2e89675181c5d9d5aba23116daac420) )
	ROM_LOAD( "lo5_13a.9",  0xb8000, 0x8000, CRC(67fbb061) SHA1(78b071cd54642ee7b6d7b9f6b759a1412bb9eef5) )
	ROM_LOAD( "lo5_31a.79", 0xc0000, 0x8000, CRC(d3595292) SHA1(9c45be919296626796b07f70b871fba5d444dbb3) )
	ROM_LOAD( "lo5_23a.51", 0xc8000, 0x8000, CRC(1487895b) SHA1(9d617f37932ca17d902307a97d16cf3b4bb5bc4e) )
	ROM_LOAD( "lo5_11a.73", 0xd0000, 0x8000, CRC(9df0b287) SHA1(6ea3b32a7826186c854cc079711ddea4ebf2ab7c) )
	ROM_LOAD( "lo5_03a.17", 0xd8000, 0x8000, CRC(7aca5d83) SHA1(95456b6c5adc5b776fbd33fd95cc62d4a83c34b6) )
	ROM_LOAD( "lo5_27a.65", 0xe0000, 0x8000, CRC(119ff70a) SHA1(e64d41bc7822c9e99fd025b771551a6c511d13f2) )
	ROM_LOAD( "lo5_19a.35", 0xe8000, 0x8000, CRC(5aaa6a53) SHA1(f8ff547979883ac9a969e76d90d028ec4286ec4c) )
	ROM_LOAD( "lo5_07a.45", 0xf0000, 0x8000, CRC(313f127f) SHA1(0782b8dd5f3a3384c3e7bc9cacaadf6804e06a38) )
	ROM_LOAD( "lo5_15a.23", 0xf8000, 0x8000, CRC(66f9c5db) SHA1(cc68da9312ee0a3441b62d14107e1b7de9b04de3) )
	ROM_REGION( 0x10000, REGION_USER5, 0 )
	ROM_LOAD( "lo4_02.109", 0x0000, 0x8000, CRC(0832edde) SHA1(77f9efbe029773417dbc3836a36687e37b5bee4b) )
	ROM_LOAD( "lo4_03.108", 0x8000, 0x8000, CRC(1efac891) SHA1(faf305a30cab1c6bf8a9d6e2682b2c3745aec956) )
	ROM_REGION16_LE( 0x20000, REGION_USER6, 0 )
	ROM_LOAD16_BYTE( "lo4_04a.119", 0x00000, 0x10000, CRC(098f4151) SHA1(cf38e3c5f3442fbfa97870d25f7c89c465f847a9) )
	ROM_LOAD16_BYTE( "lo4_05a.118", 0x00001, 0x10000, CRC(3b21667c) SHA1(b8337f733ede35145602ee3f0de25c2d4db1b2a5) )
	ROM_REGION( 0x1800, REGION_PROMS, 0 )
	ROM_LOAD( "lo1a.5", 0x000, 0x400, CRC(82391f30) SHA1(d7153c1f3a3e54de4d4d6f432fbcd66449b96b6e) )
	ROM_LOAD( "lo2a.2", 0x400, 0x400, CRC(2bfc6288) SHA1(03d293ddc0c614b606be823826a4375b3d35901f) )
	ROM_LOAD_NIB_LOW ( "lo_3.69", 0x800, 0x800, CRC(9d9c41a9) SHA1(aabeefe95274f10400b4b7810ea50afcc4f19fde) )
	ROM_LOAD_NIB_HIGH( "lo_4.68", 0x800, 0x800, CRC(ca4874ef) SHA1(c742f79729b0dc4d227379e9109c7ed21b4c38bb) )
ROM_END

/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1986, lockon, 0, lockon, lockon, 0, ROT0, "Tatsumi", "Lock-On (rev. E)", 0 )
