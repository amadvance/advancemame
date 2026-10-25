// license:BSD-3-Clause
// copyright-holders:David Haywood, ???
/* IGS037 video, converted from MAME 0.287 to C. */
#include "driver.h"
#include <stdbool.h>
#include "includes/pgm2.h"
static tilemap *m_fg_tilemap, *m_bg_tilemap;
static mame_bitmap *m_sprite_bitmap;
static UINT8 *m_sprites_mask, *m_sprites_colour;
static UINT32 m_sprites_mask_size, m_sprites_colour_size;
UINT32 *pgm2_sp_videoram, *pgm2_fg_videoram, *pgm2_bg_videoram;
UINT32 *pgm2_sp_zoom, *pgm2_lineram, *pgm2_bgscroll, *pgm2_fgscroll, *pgm2_vidmode;
UINT32 pgm2_spritekey;
#define population_count_32(v) __builtin_popcount((UINT32)(v))
static void draw_sprite_pixel(const rectangle *cliprect, UINT32 palette_offset, INT16 realx, INT16 realy, UINT16 pal)
{
	if (realx >= cliprect->min_x && realx <= cliprect->max_x && realy >= cliprect->min_y && realy <= cliprect->max_y)
	{
		const UINT16 pix = m_sprites_colour[palette_offset] & 0x3f; // there are some stray 0xff bytes in some roms, so mask
		const UINT16 pendat = pix + (pal * 0x40);
		UINT16 *const dstptr_bitmap = (UINT16 *)m_sprite_bitmap->line[realy];
		dstptr_bitmap[realx] = pendat;
	}
}

static void draw_sprite_chunk(const rectangle *cliprect, UINT32 *palette_offset, INT16 x, INT16 realy,
		UINT16 sizex, int xdraw, UINT16 pal, UINT32 maskdata, UINT32 zoomx_bits, UINT8 repeats, INT16 *realxdraw, INT8 realdraw_inc, INT8 palette_inc)
{
	for (int xchunk = 0; xchunk < 32; xchunk++)
	{
		UINT8 pix, xzoombit;
		if (palette_inc == -1)
		{
			pix = BIT(maskdata, xchunk);
			xzoombit = BIT(zoomx_bits, xchunk);
		}
		else
		{
			pix = BIT(maskdata, 31 - xchunk);
			xzoombit = BIT(zoomx_bits, 31 - xchunk);
		}

		if (pix)
		{
			if (repeats != 0) // grow
			{
				// draw it the base number of times
				for (int i = 0; i < repeats; i++)
				{
					draw_sprite_pixel(cliprect, (*palette_offset), x + (*realxdraw), realy, pal);
					(*realxdraw) += realdraw_inc;
				}

				// draw it again if zoom bit is set
				if (xzoombit)
				{
					draw_sprite_pixel(cliprect, (*palette_offset), x + (*realxdraw), realy, pal);
					(*realxdraw) += realdraw_inc;
				}

				(*palette_offset) += palette_inc;
				(*palette_offset) &= m_sprites_colour_size - 1;
			}
			else // shrink
			{
				if (xzoombit) draw_sprite_pixel(cliprect, (*palette_offset), x + (*realxdraw), realy, pal);

				(*palette_offset) += palette_inc;
				(*palette_offset) &= m_sprites_colour_size - 1;

				if (xzoombit) (*realxdraw) += realdraw_inc;

			}
		}
		else
		{
			if (repeats != 0) // grow
			{
				for (int i = 0; i < repeats; i++)
				{
					(*realxdraw) += realdraw_inc;
				}

				if (xzoombit)
				{
					(*realxdraw) += realdraw_inc;
				}
			}
			else // shrink
			{
				if (xzoombit) (*realxdraw) += realdraw_inc;
			}
		}
	}

}

static void skip_sprite_chunk(UINT32 *palette_offset, UINT32 maskdata, bool reverse)
{
	INT32 bits = population_count_32(maskdata);

	if (!reverse)
	{
		(*palette_offset) += bits;
	}
	else
	{
		(*palette_offset) -= bits;
	}

	(*palette_offset) &= m_sprites_colour_size - 1;

}

static void draw_sprite_line(const rectangle *cliprect, UINT32 *mask_offset, UINT32 *palette_offset, INT16 x, INT16 realy,
		bool flipx, bool reverse, UINT16 sizex, UINT16 pal, UINT8 zoomybit, UINT32 zoomx_bits, UINT8 xrepeats)
{
	INT16 realxdraw = 0;

	if (flipx ^ reverse)
		realxdraw = (population_count_32(zoomx_bits) * sizex) - 1;

	for (int xdraw = 0; xdraw < sizex; xdraw++)
	{
		UINT32 maskdata = m_sprites_mask[(*mask_offset) + 0] << 24;
		maskdata |= m_sprites_mask[(*mask_offset) + 1] << 16;
		maskdata |= m_sprites_mask[(*mask_offset) + 2] << 8;
		maskdata |= m_sprites_mask[(*mask_offset) + 3] << 0;

		maskdata ^= pgm2_spritekey;

		if (reverse)
		{
			(*mask_offset) -= 4;
		}
		else if (!reverse)
		{
			(*mask_offset) += 4;
		}

		(*mask_offset) &= m_sprites_mask_size - 1;

		if (zoomybit)
		{
			if (!flipx)
			{
				if (!reverse) draw_sprite_chunk(cliprect,  &((*palette_offset)),  x,  realy,  sizex,  xdraw,  pal,  maskdata,  zoomx_bits,  xrepeats,  &realxdraw,  1,  1);
				else draw_sprite_chunk(cliprect,  &((*palette_offset)),  x,  realy,  sizex,  xdraw,  pal,  maskdata,  zoomx_bits,  xrepeats,  &realxdraw,  -1,  -1);
			}
			else
			{
				if (!reverse) draw_sprite_chunk(cliprect,  &((*palette_offset)),  x,  realy,  sizex,  xdraw,  pal,  maskdata,  zoomx_bits,  xrepeats,  &realxdraw,  -1,  1);
				else draw_sprite_chunk(cliprect,  &((*palette_offset)),  x,  realy,  sizex,  xdraw,  pal,  maskdata,  zoomx_bits,  xrepeats,  &realxdraw,  1,  -1);
			}
		}
		else skip_sprite_chunk( &((*palette_offset)),  maskdata,  reverse);
	}
}

static void draw_sprites(const rectangle *cliprect)
{
	fillbitmap(m_sprite_bitmap, 0x8000, cliprect);

	INT32 endoflist = -1;

	//logerror("frame\n");

	for (int i = 0; i < 0x2000 / 4; i += 4)
	{
		if (pgm2_sp_videoram[i + 2] & 0x80000000)
		{
			endoflist = i;
			break;
		}
	}

	if (endoflist > 0)
	{
		for (int i = 0; i < endoflist - 2; i += 4)
		{
			//logerror("sprite with %08x %08x %08x %08x\n", pgm2_sp_videoram[i + 0], pgm2_sp_videoram[i + 1], pgm2_sp_videoram[i + 2], pgm2_sp_videoram[i + 3]);

			INT16 x = (pgm2_sp_videoram[i + 0] & 0x000007ff) >> 0;
			INT16 y = (pgm2_sp_videoram[i + 0] & 0x003ff800) >> 11;
			UINT16 pal = (pgm2_sp_videoram[i + 0] & 0x0fc00000) >> 22;
			const UINT8 pri = (pgm2_sp_videoram[i + 0] & 0x80000000) >> 31;

			const UINT32 unk0 = (pgm2_sp_videoram[i + 0] & 0x30000000) >> 0;

			// kov3 uses this in places (eg. horsemen special move heads) and shops when it can only possibly mean 'disable'
			// it is also used in places on kov2nl and orleg2, often the 'power up' effects surrounding your character to create an on/off flicker each frame
			bool disable = (pgm2_sp_videoram[i + 0] & 0x40000000) >> 30;
			if (disable) continue;

			const UINT16 sizex = (pgm2_sp_videoram[i + 1] & 0x0000003f) >> 0;
			const UINT16 sizey = (pgm2_sp_videoram[i + 1] & 0x00007fc0) >> 6;
			bool const flipx = (pgm2_sp_videoram[i + 1] & 0x00800000) >> 23;
			bool const reverse = (pgm2_sp_videoram[i + 1] & 0x80000000) >> 31; // more of a 'reverse entire drawing' flag than y-flip, but used for that purpose
			const UINT8 zoomx = (pgm2_sp_videoram[i + 1] & 0x007f0000) >> 16;
			const UINT8 zoomy = (pgm2_sp_videoram[i + 1] & 0x7f000000) >> 24;
			const UINT32 unk1 = (pgm2_sp_videoram[i + 1] & 0x00008000) >> 0;

			if (unk0 || unk1)
			{
				//popmessage("sprite rendering unused bits set unk0 %08x unk1 %08x\n", unk0, unk1);
			}

			UINT32 mask_offset = (pgm2_sp_videoram[i + 2] << 1);
			UINT32 palette_offset = (pgm2_sp_videoram[i + 3]);

			// use all the bits of zoom to lookup, probably why the table is copied 4x in RAM
			const UINT32 zoomy_bits = pgm2_sp_zoom[zoomy];
			const UINT32 zoomx_bits = pgm2_sp_zoom[zoomx];

			// but use these bits as the scale factor
			const UINT8 xrepeats = (zoomx & 0x60)>>5;
			const UINT8 yrepeats = (zoomy & 0x60)>>5;

			if (x & 0x400) x -= 0x800;
			if (y & 0x400) y -= 0x800;

			if (reverse)
				mask_offset -= 2;

			mask_offset &= m_sprites_mask_size - 1;
			palette_offset &= m_sprites_colour_size - 1;

			pal |= (pri << 6); // encode priority with the palette for manual mixing later

			INT16 realy = y;

			INT16 sourceline = 0;
			for (int ydraw = 0; ydraw < sizey; sourceline++)
			{
				UINT8 zoomy_bit = BIT(zoomy_bits, sourceline & 0x1f);

				// store these for when we need to draw a line twice
				const UINT32 pre_palette_offset = palette_offset;
				const UINT32 pre_mask_offset = mask_offset;

				if (yrepeats != 0) // grow
				{
					for (int i = 0; i < yrepeats; i++)
					{
						// draw it the base number of times
						palette_offset = pre_palette_offset;
						mask_offset = pre_mask_offset;
						draw_sprite_line(cliprect,  &(mask_offset),  &(palette_offset),  x,  realy,  flipx,  reverse,  sizex,  pal,  1,  zoomx_bits,  xrepeats);
						realy++;
					}

					if (zoomy_bit) // draw it again if zoom bit is set
					{
						palette_offset = pre_palette_offset;
						mask_offset = pre_mask_offset;
						draw_sprite_line(cliprect,  &(mask_offset),  &(palette_offset),  x,  realy,  flipx,  reverse,  sizex,  pal,  1,  zoomx_bits,  xrepeats);
						realy++;
					}

					ydraw++;
				}
				else // shrink
				{
					draw_sprite_line(cliprect,  &(mask_offset),  &(palette_offset),  x,  realy,  flipx,  reverse,  sizex,  pal,  1,  zoomx_bits,  xrepeats);

					if (zoomy_bit)
					{
						realy++;
					}
					ydraw++;
				}
			}
		}
	}
}

static void copy_sprites_from_bitmap(mame_bitmap *bitmap, const rectangle *cliprect, UINT16 pri)
{
	pri <<= 12;

	pen_t const *const paldata = Machine->pens;

	for (int y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		const UINT16 *const srcptr_bitmap = (UINT16 *)m_sprite_bitmap->line[y];
		UINT32 *const dstptr_bitmap = (UINT32 *)bitmap->line[y];

		for (int x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			const UINT16 pix = srcptr_bitmap[x];

			if (pix != 0x8000)
			{
				if ((pix & 0x1000) == pri)
					dstptr_bitmap[x] = paldata[pix & 0xfff];
			}
		}

	}
}

VIDEO_UPDATE(pgm2)
{
	const UINT32 mode = pgm2_vidmode[0] & 0x00030000; // other bits not used?

	switch (mode >> 16)
	{
		default:
		case 0x00: set_visible_area(0, 320 - 1, 0, 240 - 1); break;
		case 0x01: set_visible_area(0, 448 - 1, 0, 224 - 1); break;
		case 0x02: set_visible_area(0, 512 - 1, 0, 240 - 1); break;
	}

	tilemap_set_scrollx(m_fg_tilemap, 0, pgm2_fgscroll[0] & 0xffff);
	tilemap_set_scrolly(m_fg_tilemap, 0, pgm2_fgscroll[0] >> 16);
	tilemap_set_scrolly(m_bg_tilemap, 0, (pgm2_bgscroll[0x0/4] & 0xffff0000)>>16 );

	for (INT16 y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		const UINT16 linescroll = (y & 1) ? ((pgm2_lineram[(y >> 1)] & 0xffff0000) >> 16) : (pgm2_lineram[(y >> 1)] & 0x0000ffff);
		tilemap_set_scrollx(m_bg_tilemap, (y + ((pgm2_bgscroll[0x0 / 4] & 0xffff0000) >> 16)) & 0x3ff, ((pgm2_bgscroll[0x0 / 4] & 0x0000ffff) >> 0) + linescroll);
	}

	const pen_t *paldata = (Machine->pens + 0x1200);

	fillbitmap(bitmap, paldata[0], cliprect); // are there any places bg pen is showing so we know what it should be?

	copy_sprites_from_bitmap(bitmap, cliprect, 1);

	tilemap_draw(bitmap, cliprect, m_bg_tilemap, 0, 0);

	copy_sprites_from_bitmap(bitmap, cliprect, 0);

	tilemap_draw(bitmap, cliprect, m_fg_tilemap, 0, 0);

}

WRITE32_HANDLER(pgm2_fg_videoram_w)
{
	COMBINE_DATA(&pgm2_fg_videoram[offset]);
	tilemap_mark_tile_dirty(m_fg_tilemap, offset);
}

static void get_fg_tile_info(int tile_index)
{
	const UINT32 tileno = (pgm2_fg_videoram[tile_index] & 0x0003ffff) >> 0;
	const UINT8 colour  = (pgm2_fg_videoram[tile_index] & 0x007c0000) >> 18; // 5 bits
	const UINT8 flipxy  = (pgm2_fg_videoram[tile_index] & 0x01800000) >> 23;

	SET_TILE_INFO(0, tileno, colour, TILE_FLIPXY(flipxy));
}

WRITE32_HANDLER(pgm2_bg_videoram_w)
{
	COMBINE_DATA(&pgm2_bg_videoram[offset]);
	tilemap_mark_tile_dirty(m_bg_tilemap, offset);
}

static void get_bg_tile_info(int tile_index)
{
	const UINT32 tileno = (pgm2_bg_videoram[tile_index] & 0x0003ffff) >> 0;
	const UINT8 colour  = (pgm2_bg_videoram[tile_index] & 0x003c0000) >> 18; // 4 bits
	const UINT8 flipxy  = (pgm2_bg_videoram[tile_index] & 0x01800000) >> 23;

	SET_TILE_INFO(1, tileno, colour, TILE_FLIPXY(flipxy));
}


VIDEO_START(pgm2)
{
 m_sprites_mask = memory_region(REGION_GFX3);
 m_sprites_colour = memory_region(REGION_GFX4);
 m_sprites_mask_size = memory_region_length(REGION_GFX3);
 m_sprites_colour_size = memory_region_length(REGION_GFX4);
 m_fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 96, 64);
 m_bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 32, 32, 64, 32);
 if (!m_fg_tilemap || !m_bg_tilemap) return 1;
 tilemap_set_transparent_pen(m_fg_tilemap, 0);
 tilemap_set_transparent_pen(m_bg_tilemap, 0);
 tilemap_set_scroll_rows(m_bg_tilemap, 1024);
 m_sprite_bitmap = auto_bitmap_alloc_depth(512, 256, 16);
 fillbitmap(m_sprite_bitmap, 0x8000, NULL);
 return m_sprite_bitmap == NULL;
}
void pgm2_draw_sprites(void) { draw_sprites(&Machine->visible_area); }
