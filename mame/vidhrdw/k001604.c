// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

    Konami 001604 2D tilemaps + 2x ROZ

   Character RAM:
   * Foreground tiles 2x or 4x 1Mbit SRAM in a 16-bit bus.
   * Background tiles 2x or 4x 1Mbit SRAM in a 16-bit bus.

   Tile RAM:
   * 3x 256Kbit SRAMs in a 24-bit bus
   * Each tile entry is 24 bits, 32768 total

   Background and foreground layer with ROZ capabilities
   Both tilemaps are 128x128 tiles, with ability to use smaller sub-tilemaps
   Foreground seems to use 8x8 tiles only, background can select 8x8 or 16x16 tiles

   Registers:

   Offset Bits
   00     sxxxxxxxxxxxxxxx ---------------- Foreground X start (13.3 fixed point)
          ---------------- sxxxxxxxxxxxxxxx Foreground Y start (13.3 fixed point)
   04     sxxxxxxxxxxxxxxx ---------------- Foreground ROZ XY (5.11 fixed point)
          ---------------- sxxxxxxxxxxxxxxx Foreground ROZ YY (5.11 fixed point)
   08     sxxxxxxxxxxxxxxx ---------------- Foreground ROZ XX (5.11 fixed point)
          ---------------- sxxxxxxxxxxxxxxx Foreground ROZ YX (5.11 fixed point)
   20     sxxxxxxxxxxxxxxx ---------------- Background X start (13.3 fixed point)
          ---------------- sxxxxxxxxxxxxxxx Background Y start (13.3 fixed point)
   24     sxxxxxxxxxxxxxxx ---------------- Background ROZ XX (5.11 fixed point)
          ---------------- sxxxxxxxxxxxxxxx Background ROZ YX (5.11 fixed point)
   28     sxxxxxxxxxxxxxxx ---------------- Background ROZ YY (5.11 fixed point)
          ---------------- sxxxxxxxxxxxxxxx Background ROZ XY (5.11 fixed point)
   60     x--------------- ---------------- FG tilemap enable?
          -x-------------- ---------------- BG tilemap enable?
          -------x-------- ---------------- Select FG/BG tiles for character RAM read/write access
          ---------x------ ---------------- BG tile size (0 = 16x16, 1 = 8x8)
          ---------------- --------------xx Character bank for FG tiles
          ---------------- ------xx-------- Character bank for BG tiles
          ------------xx-- ---------------- Tilemap layout (00 = "landscape", 256 tiles wide, 11 = "portrait", 128 tiles wide)
    6c    -x-------------- ---------------- Swap FG/BG tilemap location in portrait mode
          ---------------- ----x----------- Enable BG sub tilemap?
          ---------------- -----xx--------- BG sub tilemap width (00 = 128, 10 = 64, 11 = 32 tiles)
          ---------------- ---------xx----- BG sub tilemap height (00 = 128, 10 = 64, 11 = 32 tiles)
          ---------------- -------------xx- BG sub tilemap X (in units of 32 tiles)

***************************************************************************/

/*
    Differences from MAME:

    The characters are decoded to the 0.106 gfx elements when they are
    drawn and the layers are rendered from the tilemap pixmaps with the
    palette entries starting at the palette_base of each chip.
*/

#include "k001604.h"

#define FG_TILES	8192
#define BG8_TILES	8192
#define BG16_TILES	2048

typedef struct
{
	UINT8 *fg_char_ram;
	UINT8 *bg_char_ram;
	UINT32 *tile_ram;
	UINT32 reg[0x100];
	tilemap *fg_tilemap;
	tilemap *bg_tilemap8;
	tilemap *bg_tilemap16;
	int gfx_index[3];
	UINT8 *char_dirty_map[3];
	int char_dirty[3];
	int layer_offset_x;
	int layer_offset_y;
} k001604_state;

static k001604_state k001604_chip[K001604_MAX_CHIPS];

static const gfx_layout char_layout_8x8 =
{
	8, 8,
	8192,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 1*8, 0*8, 3*8, 2*8, 5*8, 4*8, 7*8, 6*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};

static const gfx_layout char_layout_16x16 =
{
	16, 16,
	2048,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 1*8, 0*8, 3*8, 2*8, 5*8, 4*8, 7*8, 6*8, 9*8, 8*8, 11*8, 10*8, 13*8, 12*8, 15*8, 14*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*128
};

static void tile_info_common(k001604_state *k, int tile_index, int which, int tilebase, int xstart, int tile_mask)
{
	UINT32 x = tile_index & 0x7f;
	UINT32 y = tile_index / 128;
	UINT32 pitch = (k->reg[0x18] & 0x40000) ? 128 : 256;
	UINT32 val = k->tile_ram[((y * pitch) + x + xstart + tilebase) & 0x7fff];
	int flags = 0;

	if (val & 0x400000)
		flags |= TILE_FLIPX;
	if (val & 0x800000)
		flags |= TILE_FLIPY;

	SET_TILE_INFO(k->gfx_index[which], val & tile_mask, (val >> 17) & 0x1f, flags);
}

static void tile_info_fg(int tile_index)
{
	k001604_state *k = tile_info.user_data;
	int tilebase = (k->reg[0x18] & 0x40000) ? ((k->reg[0x1b] & 0x40000000) ? 0x4000 : 0x0000) : 0x0000;

	tile_info_common(k, tile_index, 0, tilebase, 0, 0x1fff);
}

static void tile_info_bg8(int tile_index)
{
	k001604_state *k = tile_info.user_data;
	int tilebase = (k->reg[0x18] & 0x40000) ? ((k->reg[0x1b] & 0x40000000) ? 0x0000 : 0x4000) : 0x0000;
	int xstart = (k->reg[0x18] & 0x40000) ? 0 : 128;

	tile_info_common(k, tile_index, 1, tilebase, xstart, 0x1fff);
}

static void tile_info_bg16(int tile_index)
{
	k001604_state *k = tile_info.user_data;
	int tilebase = (k->reg[0x18] & 0x40000) ? ((k->reg[0x1b] & 0x40000000) ? 0x0000 : 0x4000) : 0x0000;
	int xstart = (k->reg[0x18] & 0x40000) ? 0 : 128;

	tile_info_common(k, tile_index, 2, tilebase, xstart, 0x7ff);
}

static int alloc_gfx(const gfx_layout *layout, UINT8 *src, int palette_base)
{
	int index;
	gfx_element *gfx;

	for (index = 0; index < MAX_GFX_ELEMENTS; ++index)
		if (Machine->gfx[index] == 0)
			break;
	if (index == MAX_GFX_ELEMENTS)
		return -1;

	gfx = allocgfx(layout);
	if (!gfx)
		return -1;

	decodegfx(gfx, src, 0, gfx->total_elements);
	/* the tilemaps store the palette index in the pixmap, as there is no color table */
	gfx->colortable = Machine->remapped_colortable + palette_base;
	gfx->total_colors = 32;
	Machine->gfx[index] = gfx;

	return index;
}

int k001604_start(int chip, int palette_base)
{
	k001604_state *k = &k001604_chip[chip];
	int i;

	memset(k, 0, sizeof(*k));

	k->fg_char_ram = auto_malloc(0x80000);
	k->bg_char_ram = auto_malloc(0x80000);
	k->tile_ram = auto_malloc(0x8000 * sizeof(UINT32));
	memset(k->fg_char_ram, 0, 0x80000);
	memset(k->bg_char_ram, 0, 0x80000);
	memset(k->tile_ram, 0, 0x8000 * sizeof(UINT32));

	k->char_dirty_map[0] = auto_malloc(FG_TILES);
	k->char_dirty_map[1] = auto_malloc(BG8_TILES);
	k->char_dirty_map[2] = auto_malloc(BG16_TILES);
	memset(k->char_dirty_map[0], 0, FG_TILES);
	memset(k->char_dirty_map[1], 0, BG8_TILES);
	memset(k->char_dirty_map[2], 0, BG16_TILES);

	k->gfx_index[0] = alloc_gfx(&char_layout_8x8, k->fg_char_ram, palette_base);
	if (k->gfx_index[0] < 0)
		return 1;
	k->gfx_index[1] = alloc_gfx(&char_layout_8x8, k->bg_char_ram, palette_base);
	if (k->gfx_index[1] < 0)
		return 1;
	k->gfx_index[2] = alloc_gfx(&char_layout_16x16, k->bg_char_ram, palette_base);
	if (k->gfx_index[2] < 0)
		return 1;

	/* the layers are drawn by the ROZ renderer from the pixmaps, so they are opaque */
	k->fg_tilemap = tilemap_create(tile_info_fg, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 128, 128);
	k->bg_tilemap8 = tilemap_create(tile_info_bg8, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 128, 128);
	k->bg_tilemap16 = tilemap_create(tile_info_bg16, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 128, 128);
	if (!k->fg_tilemap || !k->bg_tilemap8 || !k->bg_tilemap16)
		return 1;

	tilemap_set_user_data(k->fg_tilemap, k);
	tilemap_set_user_data(k->bg_tilemap8, k);
	tilemap_set_user_data(k->bg_tilemap16, k);

	for (i = 0; i < 3; ++i)
		k->char_dirty[i] = 0;

	k->layer_offset_x = Machine->visible_area.min_x;
	k->layer_offset_y = Machine->visible_area.min_y;

	return 0;
}

void k001604_set_layer_offset(int chip, int x, int y)
{
	k001604_chip[chip].layer_offset_x = x;
	k001604_chip[chip].layer_offset_y = y;
}

static void update_chars(k001604_state *k)
{
	static const struct { int ram; int tiles; int size; } desc[3] = {
		{ 0, FG_TILES, 64 },
		{ 1, BG8_TILES, 64 },
		{ 1, BG16_TILES, 256 }
	};
	int i;

	for (i = 0; i < 3; ++i)
	{
		if (k->char_dirty[i])
		{
			gfx_element *gfx = Machine->gfx[k->gfx_index[i]];
			const gfx_layout *layout = (i == 2) ? &char_layout_16x16 : &char_layout_8x8;
			UINT8 *ram = desc[i].ram ? k->bg_char_ram : k->fg_char_ram;
			int t;

			for (t = 0; t < desc[i].tiles; ++t)
			{
				if (k->char_dirty_map[i][t])
				{
					k->char_dirty_map[i][t] = 0;
					decodechar(gfx, t, ram, layout);
				}
			}
			k->char_dirty[i] = 0;

			if (i == 0)
				tilemap_mark_all_tiles_dirty(k->fg_tilemap);
			else if (i == 1)
				tilemap_mark_all_tiles_dirty(k->bg_tilemap8);
			else
				tilemap_mark_all_tiles_dirty(k->bg_tilemap16);
		}
	}
}

static void draw_tilemap(k001604_state *k, mame_bitmap *bitmap, const rectangle *cliprect, int front, tilemap *tmap)
{
	static const int subtilemap_dimension[4] = { 128, 128, 64, 32 };
	INT32 startx, starty, incxx, incyx, incxy, incyy;
	UINT32 sub_x, sub_y, sub_xmask, sub_ymask;
	mame_bitmap *pixmap;
	const pen_t *clut = Machine->pens;
	int min_x = k->layer_offset_x;
	int min_y = k->layer_offset_y;
	int sy;

	update_chars(k);
	pixmap = tilemap_get_pixmap(tmap);

	if (front)
	{
		startx = (INT32)((INT16)(k->reg[0x00] >> 16)) << 13;
		starty = (INT32)((INT16)(k->reg[0x00])) << 13;
		incyy = (INT32)((INT16)(k->reg[0x01])) << 5;
		incxy = (INT32)((INT16)(k->reg[0x01] >> 16)) << 5;
		incyx = (INT32)((INT16)(k->reg[0x02])) << 5;
		incxx = (INT32)((INT16)(k->reg[0x02] >> 16)) << 5;

		sub_xmask = (128 * 8) - 1;
		sub_ymask = (128 * 8) - 1;
		sub_x = 0;
		sub_y = 0;
	}
	else
	{
		int tile_size = (k->reg[0x18] & 0x400000) ? 8 : 16;

		startx = (INT32)((INT16)(k->reg[0x08] >> 16)) << 13;
		starty = (INT32)((INT16)(k->reg[0x08])) << 13;
		incxx = (INT32)((INT16)(k->reg[0x09])) << 5;
		incyx = (INT32)((INT16)(k->reg[0x09] >> 16)) << 5;
		incxy = (INT32)((INT16)(k->reg[0x0a])) << 5;
		incyy = (INT32)((INT16)(k->reg[0x0a] >> 16)) << 5;

		if (k->reg[0x1b] & 0x800)
		{
			sub_xmask = (subtilemap_dimension[(k->reg[0x1b] >> 9) & 0x3] * tile_size) - 1;
			sub_ymask = (subtilemap_dimension[(k->reg[0x1b] >> 5) & 0x3] * tile_size) - 1;
			sub_x = (((k->reg[0x1b] >> 1) & 0x3) * 32) * tile_size;
			sub_y = 0;
		}
		else
		{
			sub_xmask = (128 * tile_size) - 1;
			sub_ymask = (128 * tile_size) - 1;
			sub_x = 0;
			sub_y = 0;
		}
	}

	for (sy = cliprect->min_y; sy <= cliprect->max_y; ++sy)
	{
		UINT32 cx = startx;
		UINT32 cy = starty;
		UINT32 *dest = (UINT32 *)bitmap->line[sy] + cliprect->min_x;
		int x;

		if (incxy == 0)
		{
			/* without rotation the source row is the same for the whole line */
			UINT32 py = ((((cy >> 16) + min_y) & sub_ymask) + sub_y) & (pixmap->height - 1);
			const UINT16 *src = (const UINT16 *)pixmap->line[py];

			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			{
				UINT32 px = ((((cx >> 16) + min_x) & sub_xmask) + sub_x) & (pixmap->width - 1);
				UINT16 pen = src[px];

				if ((pen & 0xff) != 0 || !front)
					*dest = clut[pen];

				cx += incxx;
				++dest;
			}
		}
		else
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			{
				/* the sub tilemap X offset can move the window past the pixmap edge, so wrap it */
				UINT32 py = ((((cy >> 16) + min_y) & sub_ymask) + sub_y) & (pixmap->height - 1);
				UINT32 px = ((((cx >> 16) + min_x) & sub_xmask) + sub_x) & (pixmap->width - 1);
				UINT16 pen = ((UINT16 *)pixmap->line[py])[px];

				if ((pen & 0xff) != 0 || !front)
					*dest = clut[pen];

				cx += incxx;
				cy += incxy;
				++dest;
			}
		}

		startx += incyx;
		starty += incyy;
	}
}

void k001604_draw_back_layer(int chip, mame_bitmap *bitmap, const rectangle *cliprect)
{
	k001604_state *k = &k001604_chip[chip];

	fillbitmap(bitmap, 0, cliprect);

	if ((k->reg[0x60 / 4] & 0x40000000) == 0)
		return;

	draw_tilemap(k, bitmap, cliprect, 0, (k->reg[0x18] & 0x400000) ? k->bg_tilemap8 : k->bg_tilemap16);
}

void k001604_draw_front_layer(int chip, mame_bitmap *bitmap, const rectangle *cliprect)
{
	k001604_state *k = &k001604_chip[chip];

	if ((k->reg[0x60 / 4] & 0x80000000) == 0)
		return;

	draw_tilemap(k, bitmap, cliprect, 1, k->fg_tilemap);
}

static UINT32 k001604_char_r(k001604_state *k, offs_t offset)
{
	int bg = (k->reg[0x60 / 4] & 0x1000000) ? 1 : 0;
	int chip = bg ? ((k->reg[0x60 / 4] >> 8) & 0x3) : (k->reg[0x60 / 4] & 0x3);
	UINT32 addr = ((offset + (chip * 0x10000)) * 2) & 0x7ffff;
	UINT8 *ram = bg ? k->bg_char_ram : k->fg_char_ram;

	return ((UINT32)ram[addr + 1] << 24) | ((UINT32)ram[addr] << 8);
}

static void k001604_char_w(k001604_state *k, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	int bg = (k->reg[0x60 / 4] & 0x1000000) ? 1 : 0;
	int chip = bg ? ((k->reg[0x60 / 4] >> 8) & 0x3) : (k->reg[0x60 / 4] & 0x3);
	UINT32 addr = ((offset + (chip * 0x10000)) * 2) & 0x7ffff;
	UINT8 *ram = bg ? k->bg_char_ram : k->fg_char_ram;

	if (!(mem_mask & 0xff000000))
		ram[addr + 1] = data >> 24;
	if (!(mem_mask & 0x0000ff00))
		ram[addr] = data >> 8;

	if (bg)
	{
		k->char_dirty_map[1][addr / 64] = 1;
		k->char_dirty_map[2][addr / 256] = 1;
		k->char_dirty[1] = 1;
		k->char_dirty[2] = 1;
	}
	else
	{
		k->char_dirty_map[0][addr / 64] = 1;
		k->char_dirty[0] = 1;
	}
}

static void k001604_tile_w(k001604_state *k, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	COMBINE_DATA(k->tile_ram + offset);

	if (k->reg[0x18] & 0x40000)
	{
		/* portrait */
		UINT32 fg_tilebase = (k->reg[0x1b] & 0x40000000) ? 0x4000 : 0x0000;
		UINT32 bg_tilebase = (k->reg[0x1b] & 0x40000000) ? 0x0000 : 0x4000;

		if (offset >= fg_tilebase && offset < fg_tilebase + 0x4000)
			tilemap_mark_tile_dirty(k->fg_tilemap, offset - fg_tilebase);
		if (offset >= bg_tilebase && offset < bg_tilebase + 0x4000)
		{
			if (k->reg[0x18] & 0x400000)
				tilemap_mark_tile_dirty(k->bg_tilemap8, offset - bg_tilebase);
			else
				tilemap_mark_tile_dirty(k->bg_tilemap16, offset - bg_tilebase);
		}
	}
	else
	{
		/* landscape */
		UINT32 x = offset & 0xff;
		UINT32 y = offset / 256;

		if (x < 128)
			tilemap_mark_tile_dirty(k->fg_tilemap, (y * 128) + x);
		else if (k->reg[0x18] & 0x400000)
			tilemap_mark_tile_dirty(k->bg_tilemap8, (y * 128) + (x - 128));
		else
			tilemap_mark_tile_dirty(k->bg_tilemap16, (y * 128) + (x - 128));
	}
}

static UINT32 k001604_reg_r(k001604_state *k, offs_t offset)
{
	switch (offset)
	{
		case 0x54/4:	return mame_rand() << 16;
		case 0x5c/4:	return (mame_rand() << 16) | (mame_rand() & 0xffff);
	}

	return k->reg[offset];
}

static void k001604_reg_w(k001604_state *k, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	UINT32 old = k->reg[offset];

	COMBINE_DATA(k->reg + offset);

	/* the tile dirty marking depends on the layout and tile size, so a change invalidates the cached tiles */
	if ((offset == 0x18 && ((old ^ k->reg[offset]) & 0x440000)) || (offset == 0x1b && ((old ^ k->reg[offset]) & 0x40000000)))
	{
		tilemap_mark_all_tiles_dirty(k->fg_tilemap);
		tilemap_mark_all_tiles_dirty(k->bg_tilemap8);
		tilemap_mark_all_tiles_dirty(k->bg_tilemap16);
	}
}

READ32_HANDLER( k001604_0_tile_r ) { return k001604_chip[0].tile_ram[offset]; }
WRITE32_HANDLER( k001604_0_tile_w ) { k001604_tile_w(&k001604_chip[0], offset, data, mem_mask); }
READ32_HANDLER( k001604_0_char_r ) { return k001604_char_r(&k001604_chip[0], offset); }
WRITE32_HANDLER( k001604_0_char_w ) { k001604_char_w(&k001604_chip[0], offset, data, mem_mask); }
READ32_HANDLER( k001604_0_reg_r ) { return k001604_reg_r(&k001604_chip[0], offset); }
WRITE32_HANDLER( k001604_0_reg_w ) { k001604_reg_w(&k001604_chip[0], offset, data, mem_mask); }

READ32_HANDLER( k001604_1_tile_r ) { return k001604_chip[1].tile_ram[offset]; }
WRITE32_HANDLER( k001604_1_tile_w ) { k001604_tile_w(&k001604_chip[1], offset, data, mem_mask); }
READ32_HANDLER( k001604_1_char_r ) { return k001604_char_r(&k001604_chip[1], offset); }
WRITE32_HANDLER( k001604_1_char_w ) { k001604_char_w(&k001604_chip[1], offset, data, mem_mask); }
READ32_HANDLER( k001604_1_reg_r ) { return k001604_reg_r(&k001604_chip[1], offset); }
WRITE32_HANDLER( k001604_1_reg_w ) { k001604_reg_w(&k001604_chip[1], offset, data, mem_mask); }
