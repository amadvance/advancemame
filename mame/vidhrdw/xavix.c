// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

    xavix.c

    XaviX TV games, video hardware

***************************************************************************/

#include "driver.h"
#include "includes/xavix.h"

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256

UINT8 xavix_spritereg;
UINT8 xavix_tmap1_regs[8];
UINT8 xavix_tmap2_regs[8];
int xavix_sprite_xhigh_ignore_hack;

static UINT8 spritefragment_dmaparam1[2];
static UINT8 spritefragment_dmaparam2[2];
static UINT8 txarray[3];

static UINT16 zbuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

/* the state used by the tile data reading */
static int tmp_dataaddress;
static int tmp_databit;
static UINT8 cur_bit;

INLINE void set_data_address(int address, int bit)
{
	tmp_dataaddress = address;
	tmp_databit = bit;
}

INLINE UINT8 get_next_bit(void)
{
	UINT8 ret;

	if (tmp_databit == 0)
		cur_bit = xavix_read_full_data_sp_bypass(tmp_dataaddress);

	ret = (cur_bit >> tmp_databit) & 1;

	++tmp_databit;

	if (tmp_databit == 8)
	{
		tmp_databit = 0;
		++tmp_dataaddress;
	}

	return ret;
}

INLINE UINT8 get_next_byte(void)
{
	UINT8 dat = 0;
	int i;

	for (i = 0; i < 8; ++i)
		dat |= get_next_bit() << i;

	return dat;
}

void xavix_video_reset(void)
{
	memset(spritefragment_dmaparam1, 0, sizeof(spritefragment_dmaparam1));
	memset(spritefragment_dmaparam2, 0, sizeof(spritefragment_dmaparam2));
	memset(xavix_tmap1_regs, 0, sizeof(xavix_tmap1_regs));
	memset(xavix_tmap2_regs, 0, sizeof(xavix_tmap2_regs));
	memset(txarray, 0, sizeof(txarray));
	memset(xavix_fragment_sprite, 0, 0x800);

	xavix_spritereg = 0;
	xavix_sprite_xhigh_ignore_hack = 1;
}

VIDEO_START( xavix )
{
	return 0;
}

/***************************************************************************

    Palette

***************************************************************************/

static UINT8 get_pen_lightness_from_dat(UINT16 dat)
{
	UINT8 y_raw = (dat & 0x1f00) >> 7;
	y_raw |= (dat & 0x8000) >> 15;
	return y_raw;
}

static UINT8 get_pen_saturation_from_dat(UINT16 dat)
{
	UINT8 c_raw = (dat & 0x00e0) >> 4;
	c_raw |= (dat & 0x4000) >> 14;
	return c_raw;
}

static UINT8 get_pen_hue_from_dat(UINT16 dat)
{
	UINT8 h_raw = (dat & 0x001f) << 1;
	h_raw |= (dat & 0x2000) >> 13;
	return h_raw;
}

static void update_pen(int pen, UINT8 shval, UINT8 lval)
{
	static const double hues[32][3] =
	{
		{ 1.00, 0.00, 0.00 },
		{ 1.00, 0.25, 0.00 },
		{ 1.00, 0.50, 0.00 },
		{ 1.00, 0.75, 0.00 },
		{ 1.00, 1.00, 0.00 },
		{ 0.75, 1.00, 0.00 },
		{ 0.50, 1.00, 0.00 },
		{ 0.25, 1.00, 0.00 },
		{ 0.00, 1.00, 0.00 },
		{ 0.00, 1.00, 0.25 },
		{ 0.00, 1.00, 0.50 },
		{ 0.00, 1.00, 0.75 },
		{ 0.00, 1.00, 1.00 },
		{ 0.00, 0.75, 1.00 },
		{ 0.00, 0.50, 1.00 },
		{ 0.00, 0.25, 1.00 },
		{ 0.00, 0.00, 1.00 },
		{ 0.25, 0.00, 1.00 },
		{ 0.50, 0.00, 1.00 },
		{ 0.75, 0.00, 1.00 },
		{ 1.00, 0.00, 1.00 },
		{ 1.00, 0.00, 0.75 },
		{ 1.00, 0.00, 0.50 },
		{ 1.00, 0.00, 0.25 },

		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
	};

	const double wr = 0.299;
	const double wg = 0.587;
	const double wb = 0.114;

	UINT16 dat = shval | (lval << 8);

	int y_raw = get_pen_lightness_from_dat(dat) >> 1;
	int c_raw = get_pen_saturation_from_dat(dat) >> 1;
	int h_raw = get_pen_hue_from_dat(dat) >> 1;

	double y = y_raw / 20.0;
	double c = c_raw / 5.0;

	double r0 = hues[h_raw][0];
	double g0 = hues[h_raw][1];
	double b0 = hues[h_raw][2];

	double z = wr * r0 + wg * g0 + wb * b0;

	double r1, g1, b1;

	if (y < z)
		c *= y / z;
	else if (z < 1)
		c *= (1 - y) / (1 - z);

	r1 = (r0 - z) * c + y;
	g1 = (g0 - z) * c + y;
	b1 = (b0 - z) * c + y;

	r1 *= 0.92;
	g1 *= 0.92;
	b1 *= 0.92;

	if (r1 < 0)
		r1 = 0;
	else if (r1 > 1)
		r1 = 1.0;

	if (g1 < 0)
		g1 = 0;
	else if (g1 > 1)
		g1 = 1.0;

	if (b1 < 0)
		b1 = 0;
	else if (b1 > 1)
		b1 = 1.0;

	palette_set_color(pen, (int)(r1 * 255.0), (int)(g1 * 255.0), (int)(b1 * 255.0));

	force_partial_update(cpu_getscanline());
}

WRITE8_HANDLER( xavix_palram_sh_w )
{
	xavix_palram_sh[offset] = data;
	update_pen(offset, xavix_palram_sh[offset], xavix_palram_l[offset]);
}

WRITE8_HANDLER( xavix_palram_l_w )
{
	xavix_palram_l[offset] = data;
	update_pen(offset, xavix_palram_sh[offset], xavix_palram_l[offset]);
}

/***************************************************************************

    Sprite RAM and registers

***************************************************************************/

WRITE8_HANDLER( xavix_spriteram_w )
{
	if (offset < 0x100)
	{
		xavix_fragment_sprite[offset] = data;
		xavix_fragment_sprite[offset + 0x400] = data & 0x01;
	}
	else if (offset < 0x400)
	{
		xavix_fragment_sprite[offset] = data;
	}
	else if (offset < 0x500)
	{
		xavix_fragment_sprite[offset] = data & 1;
		xavix_fragment_sprite[offset - 0x400] = (xavix_fragment_sprite[offset - 0x400] & 0xfe) | (data & 0x01);
		xavix_sprite_xhigh_ignore_hack = 0;
	}
	else
	{
		xavix_fragment_sprite[offset] = data;
	}
}

WRITE8_HANDLER( xavix_spritefragment_dma_params_1_w )
{
	spritefragment_dmaparam1[offset] = data;
}

WRITE8_HANDLER( xavix_spritefragment_dma_params_2_w )
{
	spritefragment_dmaparam2[offset] = data;
}

WRITE8_HANDLER( xavix_spritefragment_dma_trg_w )
{
	UINT16 len = data & 0x07;
	UINT16 src = (spritefragment_dmaparam1[1] << 8) | spritefragment_dmaparam1[0];
	UINT16 dst = (spritefragment_dmaparam2[0] << 8);
	UINT8 src_block_size = spritefragment_dmaparam2[1];

	/* the size 0 is the maximum */
	--src_block_size;

	if (len == 0x00)
		len = 0x08;

	len = len << 8;

	if (data & 0x40)
	{
		int readaddress = 0;
		int i;

		for (i = 0; i < len; ++i)
		{
			/* only the first bytes of each block of 0x100 are copied */
			if ((i & 0xff) <= src_block_size)
			{
				UINT8 dat = xavix_read_full_data_sp_bypass(src + readaddress);
				xavix_spriteram_w((dst + i) & 0x7ff, dat);
				++readaddress;
			}
		}
	}
}

READ8_HANDLER( xavix_spritefragment_dma_status_r )
{
	return 0x00;
}

READ8_HANDLER( xavix_pal_ntsc_r )
{
	/* the PAL or NTSC flag */
	return readinputport(14);
}

READ8_HANDLER( xavix_tmap1_regs_r )
{
	return xavix_tmap1_regs[offset];
}

WRITE8_HANDLER( xavix_tmap1_regs_w )
{
	xavix_tmap1_regs[offset] = data;
}

READ8_HANDLER( xavix_tmap2_regs_r )
{
	return xavix_tmap2_regs[offset];
}

WRITE8_HANDLER( xavix_tmap2_regs_w )
{
	xavix_tmap2_regs[offset] = data;
}

READ8_HANDLER( xavix_spriteregs_r )
{
	return xavix_spritereg;
}

WRITE8_HANDLER( xavix_spriteregs_w )
{
	xavix_spritereg = data;
}

WRITE8_HANDLER( xavix_memoryemu_txarray_w )
{
	if (offset < 0x400)
	{
		/* unused */
	}
	else if (offset < 0x800)
	{
		txarray[0] = data;
	}
	else if (offset < 0xc00)
	{
		txarray[1] = data;
	}
	else if (offset < 0x1000)
	{
		txarray[2] = data;
	}
}

UINT8 xavix_txarray_r(UINT16 offset)
{
	if (offset < 0x100)
	{
		offset &= 0xff;
		return ((offset >> 4) | (offset << 4));
	}
	else if (offset < 0x200)
	{
		offset &= 0xff;
		return ((offset >> 4) | (~offset << 4));
	}
	else if (offset < 0x300)
	{
		offset &= 0xff;
		return (((~offset >> 4) & 0x0f) | (offset << 4));
	}
	else if (offset < 0x400)
	{
		offset &= 0xff;
		return (((~offset >> 4) & 0x0f) | (~offset << 4));
	}
	else if (offset < 0x800)
	{
		return txarray[0];
	}
	else if (offset < 0xc00)
	{
		return txarray[1];
	}
	else if (offset < 0x1000)
	{
		return txarray[2];
	}
	return 0xff;
}

READ8_HANDLER( xavix_memoryemu_txarray_r )
{
	return xavix_txarray_r(offset);
}

/***************************************************************************

    Drawing

***************************************************************************/

static void draw_tile_line(mame_bitmap *bitmap, const rectangle *cliprect, int tile, int bpp, int xpos, int ypos, int drawheight, int drawwidth, int flipx, int flipy, int pal, int zval, int line)
{
	UINT16 *dest;
	UINT16 *zdest;
	int bits_per_tileline;
	int x;

	if (ypos > cliprect->max_y || ypos < cliprect->min_y)
		return;

	if (xpos > cliprect->max_x || xpos + drawwidth < cliprect->min_x)
		return;

	if (bpp > 4)
		pal &= (0xf << (bpp - 4));

	bits_per_tileline = drawwidth * bpp;

	set_data_address(tile, 0);

	if (flipy)
		line = (drawheight - 1) - line;

	tmp_dataaddress = tmp_dataaddress + ((line * bits_per_tileline) / 8);
	tmp_databit = (line * bits_per_tileline) % 8;

	dest = (UINT16 *)bitmap->line[ypos];
	zdest = zbuffer[ypos];

	for (x = 0; x < drawwidth; ++x)
	{
		int col;
		UINT8 dat = 0;
		int i;

		if (flipx)
			col = xpos + (drawwidth - 1) - x;
		else
			col = xpos + x;

		for (i = 0; i < bpp; ++i)
			dat |= get_next_bit() << i;

		if (col >= cliprect->min_x && col <= cliprect->max_x)
		{
			if (zval >= zdest[col])
			{
				int pen = (dat + (pal << 4)) & 0xff;

				if ((xavix_palram_sh[pen] & 0x1f) < 24)
				{
					dest[col] = Machine->pens[pen];
					zdest[col] = zval;
				}
			}
		}
	}
}

static void decode_inline_header(int *flipx, int *flipy, int *pal)
{
	UINT8 byte1;
	int done = 0;
	int first = 1;

	*flipx = 0;
	*flipy = 0;

	do
	{
		byte1 = get_next_byte();

		/* the palette and the flips are in the first byte */
		if (first == 1)
		{
			int cmd = (byte1 & 0x0f);

			*pal = (byte1 & 0xf0) >> 4;

			switch (cmd)
			{
			case 0x5:
			case 0x7:
				*flipx = 1; *flipy = 0;
				break;

			case 0x9:
			case 0xb:
				*flipx = 0; *flipy = 1;
				break;

			case 0xd:
			case 0xf:
				*flipx = 1; *flipy = 1;
				break;
			}

			first = 0;
		}

		if ((byte1 & 0x0f) == 0x06)
		{
			done = 1;
		}
	} while (done == 0);
}

static void draw_tilemap_line(mame_bitmap *bitmap, const rectangle *cliprect, int which, int line)
{
	UINT8 *tileregs = (which == 0) ? xavix_tmap1_regs : xavix_tmap2_regs;
	int use_inline_header;
	int alt_tileaddressing2 = 0;
	int ydimension = 0;
	int xdimension = 0;
	int ytilesize = 0;
	int xtilesize = 0;
	int yshift = 0;
	int drawline;
	int scrolly;
	int y;
	int yyline;
	int x;

	if (!(tileregs[0x7] & 0x80))
		return;

	switch (tileregs[0x3] & 0x30)
	{
	case 0x00:
		ydimension = 32;
		xdimension = 32;
		ytilesize = 8;
		xtilesize = 8;
		yshift = 3;
		break;

	case 0x10:
		ydimension = 32;
		xdimension = 16;
		ytilesize = 8;
		xtilesize = 16;
		yshift = 3;
		break;

	case 0x20:
		ydimension = 16;
		xdimension = 32;
		ytilesize = 16;
		xtilesize = 8;
		yshift = 4;
		break;

	case 0x30:
		ydimension = 16;
		xdimension = 16;
		ytilesize = 16;
		xtilesize = 16;
		yshift = 4;
		break;
	}

	use_inline_header = (tileregs[0x7] & 0x10) ? 1 : 0;

	if (tileregs[0x7] & 0x02)
		alt_tileaddressing2 = 1;

	if ((tileregs[0x7] & 0x7f) == 0x04)
		alt_tileaddressing2 = 2;

	if ((tileregs[0x7] & 0x7f) == 0x0d)
		alt_tileaddressing2 = 3;

	scrolly = tileregs[0x5];

	drawline = line + scrolly;
	drawline &= ((ydimension * ytilesize) - 1);

	y = drawline >> yshift;
	yyline = drawline & (ytilesize - 1);

	for (x = 0; x < xdimension; ++x)
	{
		int count = (y * xdimension) + x;
		int tile = 0;
		int bpp;
		int pal;
		int zval;
		int scrollx;
		int basereg;
		int flipx;
		int flipy;
		int gfxbase;

		if (tileregs[0x0] != 0x00)
		{
			UINT32 realaddress = (tileregs[0x0] << 8) + count;
			tile |= xavix_read_full_data_sp_bypass(realaddress);
		}

		/* the high byte is present only in the modes with 16 bits or more */
		if (((tileregs[0x7] & 0x7f) != 0x00) && ((tileregs[0x7] & 0x7f) != 0x08))
		{
			UINT32 realaddress = (tileregs[0x1] << 8) + count;
			tile |= xavix_read_full_data_sp_bypass(realaddress) << 8;
		}

		/* the third byte of the tile number */
		if (alt_tileaddressing2 == 2)
		{
			UINT32 realaddress = (tileregs[0x2] << 8) + count;
			tile |= xavix_read_full_data_sp_bypass(realaddress) << 16;
		}

		bpp = (tileregs[0x3] & 0x0e) >> 1;
		++bpp;
		pal = (tileregs[0x6] & 0xf0) >> 4;
		zval = (tileregs[0x6] & 0x0f) >> 0;
		scrollx = tileregs[0x4];

		flipx = (tileregs[0x03] & 0x40) >> 6;
		flipy = (tileregs[0x03] & 0x80) >> 7;

		if (tile == 0)
			continue;

		if (!use_inline_header)
		{
			if (alt_tileaddressing2 == 0)
			{
				int offset_multiplier = (ytilesize * xtilesize) / 8;

				basereg = 0;
				gfxbase = (xavix_segment_regs[(basereg * 2) + 1] << 16) | (xavix_segment_regs[(basereg * 2)] << 8);

				tile = tile * (offset_multiplier * bpp);
				tile += gfxbase;
			}
			else if (alt_tileaddressing2 == 1)
			{
				if (tileregs[0x7] & 0x01)
				{
					basereg = (tile & 0xf000) >> 12;
					tile = tile & 0x0fff;
					gfxbase = (xavix_segment_regs[(basereg * 2) + 1] << 16) | (xavix_segment_regs[(basereg * 2)] << 8);
					tile += gfxbase;
				}
				else
				{
					basereg = (tile & 0xe000) >> 13;
					tile &= 0x1fff;
					tile = tile * 8;
					gfxbase = (xavix_segment_regs[(basereg * 2) + 1] << 16) | (xavix_segment_regs[(basereg * 2)] << 8);
					tile += gfxbase;
				}
			}
			else if (alt_tileaddressing2 == 3)
			{
				tile = 0;
			}

			/* the attribute of the tile */
			if (tileregs[0x7] & 0x08)
			{
				UINT32 realaddress = (tileregs[0x2] << 8) + count;
				UINT8 extraattr = xavix_read_full_data_sp_bypass(realaddress);

				pal = (extraattr & 0xf0) >> 4;
				zval = (extraattr & 0x0f) >> 0;
			}
		}
		else
		{
			basereg = (tile & 0xf000) >> 12;
			tile &= 0x0fff;
			gfxbase = (xavix_segment_regs[(basereg * 2) + 1] << 16) | (xavix_segment_regs[(basereg * 2)] << 8);

			tile += gfxbase;
			set_data_address(tile, 0);

			decode_inline_header(&flipx, &flipy, &pal);

			tile = tmp_dataaddress;
		}

		draw_tile_line(bitmap, cliprect, tile, bpp, (x * xtilesize) + scrollx, line, ytilesize, xtilesize, flipx, flipy, pal, zval, yyline);
		draw_tile_line(bitmap, cliprect, tile, bpp, ((x * xtilesize) + scrollx) - 256, line, ytilesize, xtilesize, flipx, flipy, pal, zval, yyline);
	}
}

static void draw_sprites_line(mame_bitmap *bitmap, const rectangle *cliprect, int line)
{
	int alt_addressing = 0;
	int i;

	xavix_spritereg &= 0x7f;

	if (xavix_spritereg == 0x00 || xavix_spritereg == 0x01 || xavix_spritereg == 0x05)
	{
		alt_addressing = 1;
	}
	else if (xavix_spritereg == 0x02)
	{
		alt_addressing = 2;
	}
	else if (xavix_spritereg == 0x04)
	{
		alt_addressing = 0;
	}
	else if (xavix_spritereg == 0x07)
	{
		alt_addressing = 3;
	}

	for (i = 0xff; i >= 0; --i)
	{
		UINT8 *spr_attr0 = xavix_fragment_sprite + 0x000;
		UINT8 *spr_attr1 = xavix_fragment_sprite + 0x100;
		UINT8 *spr_ypos = xavix_fragment_sprite + 0x200;
		UINT8 *spr_xpos = xavix_fragment_sprite + 0x300;
		UINT8 *spr_xposh = xavix_fragment_sprite + 0x400;
		UINT8 *spr_addr_lo = xavix_fragment_sprite + 0x500;
		UINT8 *spr_addr_md = xavix_fragment_sprite + 0x600;
		UINT8 *spr_addr_hi = xavix_fragment_sprite + 0x700;

		int ypos = spr_ypos[i];
		int xpos = spr_xpos[i];
		int tile = 0;
		int attr0;
		int attr1;
		int pal;
		int zval;
		int flipx;
		int flipy;
		int drawheight = 8;
		int drawwidth = 8;
		int xpos_adjust;
		int ypos_adjust;
		int spritelowy;
		int spritehighy;

		if (xavix_spritereg == 0x04 || xavix_spritereg == 0x07 || xavix_spritereg == 0x15)
			tile |= (spr_addr_hi[i] << 16);

		if (xavix_spritereg != 0x00)
			tile |= (spr_addr_md[i] << 8);

		tile |= spr_addr_lo[i];

		attr0 = spr_attr0[i];
		attr1 = spr_attr1[i];

		pal = (attr0 & 0xf0) >> 4;

		zval = (attr1 & 0xf0) >> 4;
		flipx = (attr1 & 0x01);
		flipy = (attr1 & 0x02);

		if (attr1 & 0x04) drawwidth = 16;
		if (attr1 & 0x08) drawheight = 16;

		xpos_adjust = -(drawwidth / 2);
		ypos_adjust = -(drawheight / 2);

		ypos ^= 0xff;

		if (ypos & 0x80)
			ypos = -0x80 + (ypos & 0x7f);
		else
			ypos &= 0x7f;

		ypos += 128 + 1;

		ypos += ypos_adjust;

		spritelowy = ypos;
		spritehighy = ypos + drawheight;

		if (line >= spritelowy && line < spritehighy)
		{
			int drawline = line - spritelowy;
			int xposh = spr_xposh[i] & 1;
			int bpp;

			if (xpos & 0x80)
			{
				xpos &= 0x7f;
				xpos = -0x80 + xpos;

				if (!xavix_sprite_xhigh_ignore_hack)
					if (!xposh)
						xpos -= 0x80;
			}
			else
			{
				xpos &= 0x7f;

				if (!xavix_sprite_xhigh_ignore_hack)
					if (xposh)
						xpos += 0x80;
			}

			xpos += 128;

			xpos += xpos_adjust;

			if (xpos < -0x80)
				xpos += 256 + 128;

			bpp = (attr0 & 0x0e) >> 1;
			bpp += 1;

			if (alt_addressing != 0)
			{
				int basereg;

				if (alt_addressing == 1)
				{
					int gfxbase;

					tile = (tile * drawheight * drawwidth * bpp) / 8;
					gfxbase = (xavix_segment_regs[1] << 16) | (xavix_segment_regs[0] << 8);
					tile += gfxbase;
				}
				else if (alt_addressing == 2)
				{
					int gfxbase;

					tile = tile * 8;
					basereg = (tile & 0xf0000) >> 16;
					tile &= 0xffff;
					gfxbase = (xavix_segment_regs[(basereg * 2) + 1] << 16) | (xavix_segment_regs[(basereg * 2)] << 8);
					tile += gfxbase;
				}
				else if (alt_addressing == 3)
				{
					tile = tile * 8;
				}
			}

			draw_tile_line(bitmap, cliprect, tile, bpp, xpos, line, drawheight, drawwidth, flipx, flipy, pal, zval, drawline);
		}
	}
}

/* the arena clips the layers to the visible area */
static void do_arena(mame_bitmap *bitmap, const rectangle *cliprect, rectangle *clip)
{
	int y;

	fillbitmap(bitmap, get_black_pen(), cliprect);

	*clip = *cliprect;

	if (xavix_arena_control & 0x01)
	{
		/* the clipping is ignored with these values of the registers */
		if (((xavix_arena_start != 0x00) && (xavix_arena_end != 0x00)) && ((xavix_arena_start != 0xff) && (xavix_arena_end != 0xff)))
		{
			clip->max_x = xavix_arena_start - 3;
			clip->min_x = xavix_arena_end - 2;

			if (clip->min_x < cliprect->min_x)
				clip->min_x = cliprect->min_x;

			if (clip->max_x > cliprect->max_x)
				clip->max_x = cliprect->max_x;
		}
	}

	if (clip->min_x <= clip->max_x)
	{
		fillbitmap(bitmap, Machine->pens[0], clip);

		for (y = clip->min_y; y <= clip->max_y; ++y)
			memset(&zbuffer[y][clip->min_x], 0, (clip->max_x - clip->min_x + 1) * sizeof(UINT16));
	}
}

VIDEO_UPDATE( xavix )
{
	rectangle clip;
	int y;

	do_arena(bitmap, cliprect, &clip);

	if (clip.min_x > clip.max_x)
		return;

	for (y = clip.min_y; y <= clip.max_y; ++y)
		draw_tilemap_line(bitmap, &clip, 0, y);

		for (y = clip.min_y; y <= clip.max_y; ++y)
		draw_tilemap_line(bitmap, &clip, 1, y);

	for (y = clip.min_y; y <= clip.max_y; ++y)
		draw_sprites_line(bitmap, &clip, y);
}
