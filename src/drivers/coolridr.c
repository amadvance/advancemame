// license:LGPL-2.1+
// copyright-holders:Angelo Salese, David Haywood, Tomasz Slanina
/******************************************************************************************************

    System H1 (c) 1994 Sega

    Cool Riders

*******************************************************************************************************/

#include "driver.h"
#include "cpu/sh2/sh2.h"
#include "cpu/m68000/m68000.h"
#include "sound/scsp.h"

#define CLIPMAXX_FULL (496-1)
#define CLIPMAXY_FULL (384-1)
#define CLIPMINX_FULL (0)
#define CLIPMINY_FULL (0)
#define VRAM_SIZE 0x100000

#define DECODECACHE_NUMOBJECTCACHES (128)
#define DECODECACHE_NUMSPRITETILES (16*16)
#define MAX_RENDER_OBJECTS (250000)

struct cool_render_object
{
	UINT32 spriteblit[12];
	mame_bitmap *drawbitmap;
	UINT32 clipvals[3];
	UINT8 screen;
	UINT32 zpri;
	UINT8 blittype;
	UINT8 *indirect_tiles;
	UINT32 *indirect_zoom;
};

struct dectile
{
	UINT16 tempshape_multi[16*16];
	int tempshape_multi_decoded;
	int is_blank;
};

struct objectcache
{
	UINT32 lastromoffset;
	UINT16 lastused_flipx;
	UINT16 lastused_flipy;
	UINT32 lastblit_rotate;
	UINT32 lastb1mode;
	UINT32 lastb1colorNumber;
	UINT32 lastb2colorNumber;
	UINT32 lastb2altpenmask;
	UINT16 lastused_hCellCount;
	UINT16 lastused_vCellCount;
	int repeatcount;

	struct dectile tiles[DECODECACHE_NUMSPRITETILES];
};

struct objcachemanager
{
	int current_object;
	struct objectcache *objcache;
	UINT16 tempshape[16*16];
};

static UINT32 *m_sysh1_workram_h;
static UINT32 *m_framebuffer_vram;
static UINT32 *m_txt_vram;
static UINT32 *m_sysh1_txt_blit;
static UINT32 *m_sound_dma;
static UINT16 *m_soundram;
static UINT16 *m_soundram2;
static UINT8 *m_compressedgfx;
static UINT16 *m_expanded_10bit_gfx;
static UINT16 *m_rearranged_16bit_gfx;
static UINT16 *m_h1_vram;
static UINT8 *m_h1_pcg;
static UINT16 *m_h1_pal;
static UINT8 pcg_dirty[0x1000];
static gfx_element *coolridr_gfx;

static UINT16 m_textBytesToWrite;
static INT16 m_blitterSerialCount;
static UINT8 m_blitterMode;
static UINT8 m_blittype;
static UINT16 m_blitterAddr;
static UINT16 m_textOffset;
static UINT32 m_blitterClearMode;
static INT16 m_blitterClearCount;
static pen_t m_tilepals[0x10000];
static pen_t m_fadedpals[2][0x8000];
static UINT32 m_spriteblit[12];
static UINT32 m_clipvals[2][3];
static UINT8 m_clipblitterMode[2];

static mame_bitmap *m_temp_bitmap_sprites;
static mame_bitmap *m_temp_bitmap_sprites2;
static mame_bitmap *m_screen1_bitmap;
static mame_bitmap *m_screen2_bitmap;

static int m_scsp_last_line;
static UINT8 an_mux_data;
static UINT8 sound_data, sound_fifo;

static int m_listcount1, m_listcount2;
static struct cool_render_object **m_cool_render_object_list1;
static struct cool_render_object **m_cool_render_object_list2;

static UINT32 m_pen_fill[2];
static int m_color_bank;
static struct { UINT32 setting; UINT32 gradient; } m_rgb_ctrl[2];
static struct objcachemanager decode[2];
static int debug_randompal;
static int scanline_count;

static const gfx_layout h1_tile_layout =
{
	16,16,
	0x1000,
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*128
};

#define COOLRIDERS_DRAWGFX_CORE(PIXEL_TYPE, COOL_PIXEL_OP)                               \
do {                                                                                    \
	do {                                                                                \
		const UINT8 *srcdata;                                                           \
		INT32 destendx, destendy;                                                       \
		INT32 srcx, srcy;                                                               \
		INT32 curx, cury;                                                               \
		INT32 dy;                                                                       \
																						\
		if (cliprect->min_x > cliprect->max_x || cliprect->min_y > cliprect->max_y)   \
			break;                                                                      \
																						\
		destendx = destx + gfx->width - 1;                                              \
		if (destx > cliprect->max_x || destendx < cliprect->min_x)                      \
			break;                                                                      \
																						\
		srcx = 0;                                                                       \
		if (destx < cliprect->min_x)                                                     \
		{                                                                               \
			srcx = cliprect->min_x - destx;                                              \
			destx = cliprect->min_x;                                                     \
		}                                                                               \
																						\
		if (destendx > cliprect->max_x)                                                  \
			destendx = cliprect->max_x;                                                  \
																						\
		destendy = desty + gfx->height - 1;                                             \
		if (desty > cliprect->max_y || destendy < cliprect->min_y)                      \
			break;                                                                      \
																						\
		srcy = 0;                                                                       \
		if (desty < cliprect->min_y)                                                     \
		{                                                                               \
			srcy = cliprect->min_y - desty;                                              \
			desty = cliprect->min_y;                                                     \
		}                                                                               \
																						\
		if (destendy > cliprect->max_y)                                                  \
			destendy = cliprect->max_y;                                                  \
																						\
		if (flipx)                                                                      \
			srcx = gfx->width - 1 - srcx;                                               \
																						\
		dy = gfx->line_modulo;                                                          \
		if (flipy)                                                                      \
		{                                                                               \
			srcy = gfx->height - 1 - srcy;                                              \
			dy = -dy;                                                                   \
		}                                                                               \
																						\
		srcdata = gfx->gfxdata + code * gfx->char_modulo;                              \
																						\
		UINT32 numblocks = (destendx + 1 - destx) / 4;                                  \
		UINT32 leftovers = (destendx + 1 - destx) - 4 * numblocks;                      \
																						\
		srcdata += srcy * gfx->line_modulo + srcx;                                      \
																						\
		if (!flipx)                                                                     \
		{                                                                               \
			for (cury = desty; cury <= destendy; cury++)                                \
			{                                                                           \
				PIXEL_TYPE *destptr = ((PIXEL_TYPE *)dest->line[cury]) + destx;        \
				const UINT8 *srcptr = srcdata;                                          \
				srcdata += dy;                                                          \
																						\
				for (curx = 0; curx < numblocks; curx++)                                \
				{                                                                       \
					COOL_PIXEL_OP(destptr[0], srcptr[0]);                               \
					COOL_PIXEL_OP(destptr[1], srcptr[1]);                               \
					COOL_PIXEL_OP(destptr[2], srcptr[2]);                               \
					COOL_PIXEL_OP(destptr[3], srcptr[3]);                               \
					srcptr += 4;                                                        \
					destptr += 4;                                                       \
				}                                                                       \
				for (curx = 0; curx < leftovers; curx++)                                \
				{                                                                       \
					COOL_PIXEL_OP(destptr[0], srcptr[0]);                               \
					srcptr++;                                                           \
					destptr++;                                                          \
				}                                                                       \
			}                                                                           \
		}                                                                               \
		else                                                                            \
		{                                                                               \
			for (cury = desty; cury <= destendy; cury++)                                \
			{                                                                           \
				PIXEL_TYPE *destptr = ((PIXEL_TYPE *)dest->line[cury]) + destx;        \
				const UINT8 *srcptr = srcdata;                                          \
				srcdata += dy;                                                          \
																						\
				for (curx = 0; curx < numblocks; curx++)                                \
				{                                                                       \
					COOL_PIXEL_OP(destptr[0], srcptr[ 0]);                              \
					COOL_PIXEL_OP(destptr[1], srcptr[-1]);                              \
					COOL_PIXEL_OP(destptr[2], srcptr[-2]);                              \
					COOL_PIXEL_OP(destptr[3], srcptr[-3]);                              \
					srcptr -= 4;                                                        \
					destptr += 4;                                                       \
				}                                                                       \
				for (curx = 0; curx < leftovers; curx++)                                \
				{                                                                       \
					COOL_PIXEL_OP(destptr[0], srcptr[0]);                               \
					srcptr--;                                                           \
					destptr++;                                                          \
				}                                                                       \
			}                                                                           \
		}                                                                               \
	} while (0);                                                                        \
} while (0)

#define COOLRIDERS_PIXEL_OP_REMAP_OPAQUE(DEST, SOURCE) (DEST) = paldata[SOURCE]

#define COOLRIDERS_PIXEL_OP_REMAP_TRANSPEN(DEST, SOURCE)                             \
do                                                                                  \
{                                                                                   \
	UINT32 srcdata = (SOURCE);                                                      \
	if (srcdata != transpen)                                                        \
		(DEST) = paldata[srcdata];                                                  \
}                                                                                   \
while (0)

static void coolriders_drawgfx_opaque(mame_bitmap *dest, const rectangle *cliprect, gfx_element *gfx,
		UINT32 code, UINT32 color, int flipx, int flipy, INT32 destx, INT32 desty)
{
	const pen_t *paldata = &m_tilepals[(color * 256) & 0xffff];
	code %= gfx->total_elements;
	COOLRIDERS_DRAWGFX_CORE(UINT16, COOLRIDERS_PIXEL_OP_REMAP_OPAQUE);
}

static void coolriders_drawgfx_transpen(mame_bitmap *dest, const rectangle *cliprect, gfx_element *gfx,
		UINT32 code, UINT32 color, int flipx, int flipy, INT32 destx, INT32 desty,
		INT32 transpen)
{
	const pen_t *paldata;
	if (transpen > 0xff || transpen < 0)
	{
		coolriders_drawgfx_opaque(dest, cliprect, gfx, code, color, flipx, flipy, destx, desty);
		return;
	}

	code %= gfx->total_elements;
	paldata = &m_tilepals[(color * 256) & 0xffff];
	COOLRIDERS_DRAWGFX_CORE(UINT16, COOLRIDERS_PIXEL_OP_REMAP_TRANSPEN);
}

static void draw_bg_coolridr(mame_bitmap *bitmap, const rectangle *cliprect, int which)
{
	int bg_r, bg_g, bg_b;

	if (m_pen_fill[which])
	{
		bg_r = (((m_pen_fill[which] >> 16) & 0x78) >> 2) | (((m_pen_fill[which] >> 16) & 0x80) >> 7);
		bg_g = (((m_pen_fill[which] >> 8) & 0x78) >> 2) | (((m_pen_fill[which] >> 8) & 0x80) >> 7);
		bg_b = (((m_pen_fill[which] >> 0) & 0x78) >> 2) | (((m_pen_fill[which] >> 0) & 0x80) >> 7);
		fillbitmap(bitmap, (bg_r << 10) | (bg_g << 5) | bg_b, cliprect);
	}
	else
	{
		UINT32 base_offset;
		int tile, vram_data, color;
		int scrollx;
		int scrolly;
		UINT8 transpen_setting;
		int x, y;

		#define VREG(_offs) \
			m_framebuffer_vram[(0x9b80 + _offs + which * 0x40) / 4]

		scrollx = (VREG(0x2c) >> 16) & 0x7ff;
		scrolly = VREG(0x2c) & 0x3ff;

		base_offset = (VREG(0x1c) * 0x8000) / 2;
		m_color_bank = which * 2;

		transpen_setting = (VREG(0x3c) & 0x80000000) >> 31;

		fillbitmap(bitmap, VREG(0x3c) & 0x7fff, cliprect);

		UINT16 basey = scrolly >> 4;
		for (y = 0; y < 25; y++)
		{
			UINT16 basex = scrollx >> 4;
			for (x = 0; x < 32; x++)
			{
				vram_data = (m_h1_vram[(basex & 0x7f) + ((basey & 0x3f) * 0x80) + base_offset] & 0xffff);
				color = m_color_bank + ((vram_data & 0x800) >> 11) * 4;
				tile = (vram_data & 0x7ff) | ((vram_data & 0x8000) >> 4);

				if (pcg_dirty[tile & 0xfff])
				{
					decodechar(coolridr_gfx, tile & 0xfff, m_h1_pcg, &h1_tile_layout);
					pcg_dirty[tile & 0xfff] = 0;
				}

				coolriders_drawgfx_transpen(bitmap, cliprect, coolridr_gfx, tile, color, 0, 0,
					(x * 16) - (scrollx & 0xf), (y * 16) - (scrolly & 0xf),
					transpen_setting ? -1 : 0);

				basex++;
			}
			basey++;
		}
		#undef VREG
	}
}

static void screen_update_coolridr(int which)
{
	int i;

	for (i = 0; i < 0x8000; i++)
	{
		int r = (i >> 10) & 0x1f;
		int g = (i >> 5) & 0x1f;
		int b = (i >> 0) & 0x1f;

		if (m_rgb_ctrl[which].gradient)
		{
			if (m_rgb_ctrl[which].setting == 0x1240)
			{
				r -= m_rgb_ctrl[which].gradient;
				g -= m_rgb_ctrl[which].gradient;
				b -= m_rgb_ctrl[which].gradient;
				if (r < 0) r = 0;
				if (g < 0) g = 0;
				if (b < 0) b = 0;
			}
			else if (m_rgb_ctrl[which].setting == 0x920)
			{
				r += m_rgb_ctrl[which].gradient;
				g += m_rgb_ctrl[which].gradient;
				b += m_rgb_ctrl[which].gradient;
				if (r > 0x1f) r = 0x1f;
				if (g > 0x1f) g = 0x1f;
				if (b > 0x1f) b = 0x1f;
			}
			else if (m_rgb_ctrl[which].setting == 0x800)
			{
				r += m_rgb_ctrl[which].gradient;
				g -= m_rgb_ctrl[which].gradient;
				b -= m_rgb_ctrl[which].gradient;
				if (r > 0x1f) r = 0x1f;
				if (g < 0) g = 0;
				if (b < 0) b = 0;
			}
		}
		m_fadedpals[which][i] = (r << 10 | g << 5 | b);
	}
}

#define RLE_BLOCK(writeaddrxor) \
	if (!current_decoded) \
	{ \
		lastSpriteNumber = 0xffffffff; \
		if (spriteNumber != lastSpriteNumber) \
		{ \
			blankcount = 256;\
			lastSpriteNumber = spriteNumber; \
			int i = 1; \
			int data_written = 0; \
			while (data_written < 256) \
			{ \
				const UINT16 compdata = expanded_10bit_gfx[(b3romoffset) + spriteNumber + i]; \
				if (((compdata & 0x300) == 0x000) || ((compdata & 0x300) == 0x100)) \
				{ \
					int encodelength = (compdata & 0x03e) >> 1; \
					const UINT16 rledata = rearranged_16bit_gfx[color_offs + ((compdata & 0x1c0) >> 6)]; \
					if (compdata & 1) encodelength = 255; \
					while (data_written < 256 && encodelength >= 0) \
					{ \
						tempshape[data_written ^ writeaddrxor] = rledata; \
						if (tempshape[data_written ^ writeaddrxor] == 0x8000) blankcount--; \
						encodelength--; \
						data_written++; \
					} \
				} \
				else if ((compdata & 0x300) == 0x200) \
				{ \
					int encodelength = (compdata & 0x003); \
					const UINT16 rledata = rearranged_16bit_gfx[color_offs + ((compdata & 0x0fc) >> 2) + 8]; \
					while (data_written < 256 && encodelength >= 0) \
					{ \
						tempshape[data_written ^ writeaddrxor] = rledata; \
						if (tempshape[data_written ^ writeaddrxor] == 0x8000) blankcount--; \
						encodelength--; \
						data_written++; \
					} \
				} \
				else \
				{ \
					UINT16 rawdat = (compdata & 0x0ff); \
					if (b1mode && (rawdat > (b2altpenmask + 0x48))) \
						tempshape[data_written ^ writeaddrxor] = rearranged_16bit_gfx[color_offs2 + (rawdat) + 0x48]; \
					else \
						tempshape[data_written ^ writeaddrxor] = rearranged_16bit_gfx[color_offs + (rawdat) + 0x48]; \
					if (tempshape[data_written ^ writeaddrxor] == 0x8000) blankcount--; \
					data_written++; \
				} \
				i++; \
			} \
			if (!indirect_tile_enable && size < DECODECACHE_NUMSPRITETILES) \
			{ \
				decode[screen].objcache[use_object].tiles[v * used_hCellCount + h].tempshape_multi_decoded = 1; \
				if (blankcount == 0) \
					decode[screen].objcache[use_object].tiles[v * used_hCellCount + h].is_blank = 1; \
				else \
					decode[screen].objcache[use_object].tiles[v * used_hCellCount + h].is_blank = 0; \
			} \
		} \
	}

#define CHECK_DECODE \
	if (used_flipy) \
	{ \
		if (used_flipx) \
		{ \
			RLE_BLOCK(0xff) \
		} \
		else \
		{ \
			RLE_BLOCK(0xf0) \
		} \
	} \
	else \
	{	if (used_flipx) \
		{ \
			RLE_BLOCK(0x0f) \
		} \
		else \
		{ \
			RLE_BLOCK(0x00) \
		} \
	} \
	if (!indirect_tile_enable && size < DECODECACHE_NUMSPRITETILES) \
	{ \
		if (decode[screen].objcache[use_object].tiles[v * used_hCellCount + h].is_blank == 1) \
			continue; \
	} \
	else \
		if (blankcount == 0) continue;

#define GET_SPRITE_NUMBER \
	int lookupnum; \
	if (indirect_tile_enable) \
	{ \
		lookupnum = object->indirect_tiles[h + (v * used_hCellCount)]; \
	} \
	else \
	{ \
		if (!blit_rotate) \
		{ \
			if (!used_flipy) \
			{ \
				if (!used_flipx) \
					lookupnum = h + (v * used_hCellCount); \
				else \
					lookupnum = (used_hCellCount - h - 1) + (v * used_hCellCount); \
			} \
			else \
			{ \
				if (!used_flipx) \
					lookupnum = h + ((used_vCellCount - v - 1) * used_hCellCount); \
				else \
					lookupnum = (used_hCellCount - h - 1) + ((used_vCellCount - v - 1) * used_hCellCount); \
			} \
		} \
		else \
		{ \
			if (!used_flipy) \
			{ \
				if (!used_flipx) \
					lookupnum = v + (h * used_vCellCount); \
				else \
					lookupnum = (used_vCellCount - v - 1) + (h * used_vCellCount); \
			} \
			else \
			{ \
				if (!used_flipx) \
					lookupnum = v + ((used_hCellCount - h - 1) * used_vCellCount); \
				else \
					lookupnum = (used_vCellCount - v - 1) + ((used_hCellCount - h - 1) * used_vCellCount); \
			} \
		} \
	} \
	UINT32 spriteNumber = (expanded_10bit_gfx[(b3romoffset) + (lookupnum << 1) + 0] << 10) | (expanded_10bit_gfx[(b3romoffset) + (lookupnum << 1) + 1]);

#define DO_XCLIP_REAL \
	if (drawx > clipmaxX) { break; } \
	if (drawx < clipminX) { drawx++; continue; }

#define DO_XCLIP_NONE

#define GET_CURRENT_LINESCROLLZOOM \
	UINT32 dword = object->indirect_zoom[v * 16 + realy]; \
	UINT16 hZoomTable = hZoom + (dword >> 16); \
	int linescroll = dword & 0x7fff; \
	if (linescroll & 0x4000) linescroll -= 0x8000; \
	int hPositionTable = linescroll + hPositionx; \
	int sizex = used_hCellCount * 16 * hZoom; \
	hPositionTable *= 0x40; \
	switch (hOrigin & 3) \
	{ \
	case 0: \
		break; \
	case 1: \
		hPositionTable -= sizex / 2; \
		break; \
	case 2: \
		hPositionTable -= sizex - 1; \
		break; \
	case 3: \
		break; \
	}

#define DRAW_PIX \
	if (pix < 0x8000) \
	{ \
		line[drawx] = pix & 0x7fff; \
	} \
	else if (pix > 0x8000) \
	{ \
		if (blit4blendlevelinv == 0x0) \
		{ \
			line[drawx] = pix & 0x7fff; \
		} \
		else \
		{ \
			UINT16 source = line[drawx]; \
			int src_r = ((source >> 10) & 0x1f) * blit4blendlevelinv; \
			int src_g = ((source >> 5) & 0x1f) * blit4blendlevelinv; \
			int src_b = ((source >> 0) & 0x1f) * blit4blendlevelinv; \
			int dest_r = ((pix >> 10) & 0x1f) * blit4blendlevel; \
			int dest_g = ((pix >> 5) & 0x1f) * blit4blendlevel; \
			int dest_b = ((pix >> 0) & 0x1f) * blit4blendlevel; \
			line[drawx] = (((src_r + dest_r) >> 3) << 10) | (((src_g + dest_g) >> 3) << 5) | (((src_b + dest_b) >> 3) << 0); \
		} \
	} \
	drawx++;

#define GET_PIX_ROTATED \
	UINT16 pix = tempshape[realx * 16 + realy];

#define GET_PIX_NORMAL \
	UINT16 pix = tempshape[realy * 16 + realx];

#define YXLOOP \
	int drawy = pixelOffsetY; \
	for (int y = 0; y < blockhigh; y++) \
	{ \
		int realy = ((y * incy) >> 21); \
		GET_CURRENT_LINESCROLLZOOM \
		UINT16 hZoomHere = hZoomTable; \
		if (!hZoomHere) { drawy++; continue; } \
		const int pixelOffsetX = ((hPositionTable) + (h * 16 * hZoomHere)) / 0x40; \
		const int pixelOffsetnextX = ((hPositionTable) + ((h + 1) * 16 * hZoomHere)) / 0x40; \
		if (drawy > clipmaxY) { break; } \
		if (drawy < clipminY) { drawy++; continue; } \
		line = (UINT16 *)drawbitmap->line[drawy]; \
		int blockwide = pixelOffsetnextX - pixelOffsetX; \
		if (pixelOffsetX + blockwide < clipminX) { drawy++; continue; } \
		if (pixelOffsetX > clipmaxX) { drawy++; continue; } \
		if (pixelOffsetX >= clipminX && pixelOffsetX + blockwide < clipmaxX) \
		{ \
			UINT32 incx = 0x8000000 / hZoomHere; \
			int drawx = pixelOffsetX; \
			for (int x = 0; x < blockwide; x++) \
			{ \
				DO_XCLIP_NONE \
				int realx = ((x * incx) >> 21); \
				GET_PIX; \
				DRAW_PIX; \
			} \
		} \
		else \
		{ \
			UINT32 incx = 0x8000000 / hZoomHere; \
			int drawx = pixelOffsetX; \
			for (int x = 0; x < blockwide; x++) \
			{ \
				DO_XCLIP_REAL \
				int realx = ((x * incx) >> 21); \
				GET_PIX; \
				DRAW_PIX; \
			} \
		} \
		drawy++; \
	}

#define YXLOOP_NO_LINEZOOM \
	for (int y = 0; y < blockhigh; y++) \
	{ \
		int realy = ((y * incy) >> 21); \
		const int drawy = pixelOffsetY + y; \
		if ((drawy > clipmaxY) || (drawy < clipminY)) continue; \
		line = (UINT16 *)drawbitmap->line[drawy]; \
		int drawx = pixelOffsetX; \
		for (int x = 0; x < blockwide; x++) \
		{ \
			DO_XCLIP \
			int realx = ((x * incx) >> 21); \
			GET_PIX; \
			DRAW_PIX \
		} \
	}

#define YXLOOP_NO_ZOOM \
	for (int realy = 0; realy < 16; realy++) \
	{ \
		const int drawy = pixelOffsetY + realy; \
		if ((drawy > clipmaxY) || (drawy < clipminY)) continue; \
		line = (UINT16 *)drawbitmap->line[drawy]; \
		int drawx = pixelOffsetX; \
		for (int realx = 0; realx < 16; realx++) \
		{ \
			DO_XCLIP \
			GET_PIX; \
			DRAW_PIX \
		} \
	}
static void coolridr_draw_object(struct cool_render_object *object)
{
	
	mame_bitmap *drawbitmap = object->drawbitmap;

	/************* object->spriteblit[3] *************/

	UINT32 blit3_unused = object->spriteblit[3] & 0xffe00000;
	UINT32 b3romoffset = (object->spriteblit[3] & 0x001fffff)*16;

	if (blit3_unused) printf("unknown bits in blit word %d -  %08x\n", 3, blit3_unused);

	/************* object->spriteblit[5] *************/

	UINT32 blit5_unused = object->spriteblit[5]&0xfffefffe;
	// this might enable the text indirection thing?
	int indirect_tile_enable = (object->spriteblit[5] & 0x00010000)>>16;
	int indirect_zoom_enable = (object->spriteblit[5] & 0x00000001);


	if (blit5_unused) printf("unknown bits in blit word %d -  %08x\n", 5, blit5_unused);
	// 00010000 (text)
	// 00000001 (other)




	UINT16* rearranged_16bit_gfx = m_rearranged_16bit_gfx;
	UINT16* expanded_10bit_gfx = m_expanded_10bit_gfx;

	INT16 clipminX = CLIPMINX_FULL;
	INT16 clipmaxX = CLIPMAXX_FULL;
	INT16 clipminY = CLIPMINY_FULL;
	INT16 clipmaxY = CLIPMAXY_FULL;


	/************* object->spriteblit[1] *************/

	// 000u0ccc  - c = colour? u = 0/1
	UINT32 blit1_unused = object->spriteblit[1] & 0xfffef800;
	UINT32 b1mode = (object->spriteblit[1] & 0x00010000)>>16;
	UINT32 b1colorNumber = (object->spriteblit[1] & 0x000007ff);    // Probably more bits

	if (blit1_unused!=0) printf("blit1 unknown bits set %08x\n", object->spriteblit[1]);


	if (b1mode)
	{
	//	b1colorNumber = rand()&0xfff;
	}

	/************* object->spriteblit[3] *************/

	// seems to be more complex than just transparency
	UINT32 blit2_unused = object->spriteblit[2]&0xff80f800;
	UINT32 b2altpenmask = (object->spriteblit[2] & 0x007f0000)>>16;
	UINT32 b2colorNumber = (object->spriteblit[2] & 0x000007ff);

	if (b2colorNumber != b1colorNumber)
	{
	//	b1colorNumber = space.machine().rand()&0xfff;
	}

//	if(b1colorNumber > 0x60 || b2colorNumber)
//		printf("%08x %08x\n",b1colorNumber,b2colorNumber);


	if (blit2_unused!=0) printf("blit1 unknown bits set %08x\n", object->spriteblit[2]);
	if (b1mode)
	{
		if (b2altpenmask != 0x7f) printf("b1mode 1, b2altpenmask!=0x7f\n");
	}
	else
	{
		// 0x01/0x02 trips in rare cases (start of one of the attract levels) maybe this is some kind of alpha instead?
		if ((b2altpenmask != 0x00) && (b2altpenmask != 0x01) && (b2altpenmask != 0x02)) printf("b1mode 0, b2altpenmask!=0x00,0x01 or 0x02 (is %02x)\n", b2altpenmask);
	}
	 // 00??0uuu
	 // ?? seems to be 00 or 7f, set depending on b1mode
	 // uuu, at least 11 bits used, maybe 12 usually the same as blit1_unused? leftover?



	/************* object->spriteblit[4] *************/

	UINT32 blit4_unused = object->spriteblit[4] & 0xf8fefeee;
	UINT16 blit4blendlevel = (object->spriteblit[4] & 0x07000000)>>24;
	UINT16 blit4blendlevelinv = blit4blendlevel^0x7;
	blit4blendlevel+=1; // make our maths easier later (not sure about accuracy)
	//object->zpri = 7-blit4;
	// unknown bits in blit word 4 -  00000010 - australia (and various other times)

	UINT32 blit_flipx = object->spriteblit[4] & 0x00000001;
	UINT32 blit_flipy = (object->spriteblit[4] & 0x00000100)>>8;
	UINT32 blit_rotate = (object->spriteblit[4] & 0x00010000)>>16;
	//UINT32 b4_unk = object->spriteblit[4] & 0x00000010;

	if (blit4_unused) printf("unknown bits in blit word %d -  %08x\n", 4, blit4_unused);

	// ---- -111 ---- ---r ---- ---y ---z ---x
	// 1 = used bits? (unknown purpose.. might be object colour mode)
	// x = x-flip
	// y = y-flip
	// r = rotate 90 degrees
	// z = ??? set very occasionally, often at the start of stages, might relate to the clipping done at the same time?

	// this might affect blending logic / amount when 0x8000 palette bit is set, and maybe even z-behavior
	// 7 = road, player bike etc.? possibly a 'do not blend' (solid) flag of sorts because the road has 0x8000 palette bit set...
	// 6 = smoke frome player bike, waves on west indies, waterfalls on niagra, occasional bits of road?
	// 5 = clouds
	// 4 = shadows and some HUD elements, occassional bit of road, some firework effects, all bg elements in the coolriders of coolriders (including reaper)
	// 3 = front coloured jets
	// 2 = middle coloured jets
	// 1 = last few coloured trails from the jets at the start, occassional bit of road
	// 0 = some HUD elements, title screen, tunnels? road during coolriders of.. (this doesn't have 0x8000 set)


	// note the road always has 0x8000 bit set in the palette.  I *think* this is because they do a gradual blend of some kind between the road types
	//  see the number of transitional road bits which have various values above set

	if (blit4blendlevel==debug_randompal)
	{
		b1colorNumber = rand()&0xfff;
	}


	/************* object->spriteblit[6] *************/

	UINT16 vCellCount = (object->spriteblit[6] & 0x03ff0000) >> 16;
	UINT16 hCellCount = (object->spriteblit[6] & 0x000003ff);

	/************* object->spriteblit[7] *************/

	UINT16 vOrigin = (object->spriteblit[7] & 0x00030000) >> 16;
	UINT16 hOrigin = (object->spriteblit[7] & 0x00000003);
	UINT16 OriginUnused = (object->spriteblit[7] & 0xfffcfffc);

	if (blit5_unused) printf("unknown bits in blit word %d -  %08x\n", 7, OriginUnused);

	//printf("%04x %04x\n", vOrigin, hOrigin);

	/************* object->spriteblit[8] *************/

	UINT16 vZoom = (object->spriteblit[8] & 0xffff0000) >> 16;
	UINT16 hZoom = (object->spriteblit[8] & 0x0000ffff);

	// if we have no vertical zoom value there's no point in going any further
	// because there are no known vertical indirect modes
	if (!vZoom)
	{
		// abort, but make sure we clean up
		if (object->indirect_tiles)
			free(object->indirect_tiles);

		if (object->indirect_zoom)
			free(object->indirect_zoom);

		free (object);

		return;
	}

	/************* object->spriteblit[9] *************/

	int vPosition = (object->spriteblit[9] & 0xffff0000) >> 16;
	int hPositionx = (object->spriteblit[9] & 0x0000ffff);

	if (hPositionx & 0x8000) hPositionx -= 0x10000;
	if (vPosition & 0x8000) vPosition -= 0x10000;

	/************* object->spriteblit[10] *************/

	// pointer to per-line zoom and scroll data for sprites
	//UINT32 blit10 = 0; // we've cached the data here already

	/************* object->spriteblit[11] *************/

	//UINT32 textlookup = 0; // we've cached the data here already

	/*
		sample data from attract mode 'filmstrip'

		you can see it's screen regions at least, gets enabled in certain game situations too
		interestingly there is a bit to determine the screen number this applies to, even if that should already be implied from m_blitterMode

		screen 1 clipping(?)

		unknown sprite list type 1 - 00000001 003f00f0 027801f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 03e001f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 000700e3 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 010c01f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

		unknown sprite list type 1 - 00000001 003f00f0 027401f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 03dc01f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 000700df 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 010801f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

		unknown sprite list type 1 - 00000001 003f00f0 027001f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 03d801f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 000700db 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 010401f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

		unknown sprite list type 1 - 00000001 003f00f0 019c01f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 030401f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 00070007 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 0030016f 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

		screen 2 clipping

		unknown sprite list type 1 - 00000001 003f00f0 039803f7 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 050003f7 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 02070203 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 022c036b 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

		unknown sprite list type 1 - 00000001 003f00f0 039403f7 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 04fc03f7 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 020701ff 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 02280367 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

		unknown sprite list type 1 - 00000001 003f00f0 039003f7 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 04f803f7 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 020701fb 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 003f00f0 02240363 00000207 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	  NOTE, we only copy across [1] [2] and [3]   as [0] [1] and [2]
	    the 3rd dword seems to be some kind of offset, it's 0x207 on the 2nd screen, and the actual x-clip rects are also higher on that screen?

		note this is practically the same format as the sysh1_fb_data_w commands..



		between stages (screen 1)
		unknown sprite list type 1 - 00000001 000000a4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a5 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a8 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000a9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ab 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ac 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ae 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000af 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b2 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b5 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b8 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000b9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ba 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000bc 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000bd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000be 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c0 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c2 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c5 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c8 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000c9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000cb 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000cc 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ce 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000cf 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d2 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d5 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d5 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d7 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d7 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d8 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000db 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000db 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000dc 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000dd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000dd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000de 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e0 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e2 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e7 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e8 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ea 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000eb 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ed 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ee 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f0 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f7 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fa 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fc 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fe 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fe 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fc 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fc 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fa 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000fa 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f8 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f6 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f3 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000f1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000ee 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000eb 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e9 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e7 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e4 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000e1 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000dd 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000da 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d7 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 000000d5 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	ending

		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
		unknown sprite list type 1 - 00000001 0000017f 000701f7 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

	romania
		unknown sprite list type 1 - 00000001 00000001 00000001 00000007 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	*/

	// how does this really work? there's more to it
	// used in film strip attract mode, and between stages
	if (object->clipvals[2] & 0x0000007) // clearly this isn't the enable flag, probably just enabled until next frame / it gets disabled
	{
		// these go negative when things scroll off the left in attract mode
		INT16 minx = ((object->clipvals[1]&0xffff0000)>>16) - (((object->clipvals[2]&0x0000ffff)>>0));
		INT16 maxx = ((object->clipvals[1]&0x0000ffff)>>0)  - (((object->clipvals[2]&0x0000ffff)>>0));
		INT16 miny = ((object->clipvals[0]&0xffff0000)>>16); // maybe subtract the top 16 bits of clipvals[2], but not used?
		INT16 maxy = ((object->clipvals[0]&0x0000ffff)>>0);


		clipminX = minx -1;
		if (clipminX<CLIPMINX_FULL) clipminX = CLIPMINX_FULL;
		if (clipminX>CLIPMAXX_FULL) clipminX = CLIPMAXX_FULL;
		clipmaxX = maxx;
		if (clipmaxX<CLIPMINX_FULL) clipmaxX = CLIPMINX_FULL;
		if (clipmaxX>CLIPMAXX_FULL) clipmaxX = CLIPMAXX_FULL;
		if (clipminX>clipmaxX) clipminX = clipmaxX;

		clipminY = miny -1;
		if (clipminY<CLIPMINY_FULL) clipminY = CLIPMINY_FULL;
		if (clipminY>CLIPMAXY_FULL) clipminY = CLIPMAXY_FULL;
		clipmaxY = maxy;
		if (clipmaxY<CLIPMINY_FULL) clipmaxY = CLIPMINY_FULL;
		if (clipmaxY>CLIPMAXY_FULL) clipmaxY = CLIPMAXY_FULL;
		if (clipminY>clipmaxY) clipminY = clipmaxY;


		//b1colorNumber = rand()&0xfff;
	}

	/* DRAW */
	UINT16 used_hCellCount = hCellCount;
	UINT16 used_vCellCount = vCellCount;
	UINT16 used_flipx = blit_flipx;
	UINT16 used_flipy = blit_flipy;

	if (blit_rotate)
	{
		used_hCellCount = vCellCount;
		used_vCellCount = hCellCount;
		used_flipx = blit_flipy;
		used_flipy = blit_flipx;
		// do the zoom params rotate?
	}

	// SPRITES / BLITS

	// for text objects this is an address containing the 8-bit tile numbers to use for ASCII text
	// I guess the tiles are decoded by a DMA operation earlier, from the compressed ROM?

	// we also use this to trigger the actual draw operation



	int size = used_hCellCount * used_vCellCount;

	UINT16* tempshape;
	int screen = object->screen;
	int use_object = 0;

	if (!indirect_tile_enable && size < DECODECACHE_NUMSPRITETILES)
	{
		int found = -1;

		for (int k=0;k<DECODECACHE_NUMOBJECTCACHES;k++)
		{
			if(((decode[screen].objcache[k].lastromoffset == b3romoffset)) &&
			   ((decode[screen].objcache[k].lastused_flipx == used_flipx)) &&
			   ((decode[screen].objcache[k].lastused_flipy == used_flipy)) &&
			   ((decode[screen].objcache[k].lastblit_rotate == blit_rotate)) &&
			   ((decode[screen].objcache[k].lastb1mode == b1mode)) &&
			   ((decode[screen].objcache[k].lastb1colorNumber == b1colorNumber)) &&
			   ((decode[screen].objcache[k].lastb2colorNumber == b2colorNumber)) &&
   			   ((decode[screen].objcache[k].lastused_hCellCount == used_hCellCount)) &&
   			   ((decode[screen].objcache[k].lastused_vCellCount == used_vCellCount)) &&
			   ((decode[screen].objcache[k].lastb2altpenmask == b2altpenmask)))
			{
				found = k;
				break;
			}
		}

		if (found != -1)
		{
			decode[screen].objcache[found].repeatcount++;
			use_object = found;
		}
		else
		{
			use_object = decode[screen].current_object;

			// dirty the cache
			for (int i=0;i<DECODECACHE_NUMSPRITETILES;i++)
				decode[screen].objcache[use_object].tiles[i].tempshape_multi_decoded = 0;

			decode[screen].objcache[use_object].lastromoffset = b3romoffset;
			decode[screen].objcache[use_object].lastused_flipx = used_flipx;
			decode[screen].objcache[use_object].lastused_flipy = used_flipy;
			decode[screen].objcache[use_object].lastblit_rotate = blit_rotate;
			decode[screen].objcache[use_object].lastb1mode = b1mode;
			decode[screen].objcache[use_object].lastb1colorNumber = b1colorNumber;
			decode[screen].objcache[use_object].lastb2colorNumber = b2colorNumber;
			decode[screen].objcache[use_object].lastused_hCellCount = used_hCellCount;
			decode[screen].objcache[use_object].lastused_vCellCount = used_vCellCount;
			decode[screen].objcache[use_object].lastb2altpenmask = b2altpenmask;
			decode[screen].objcache[use_object].repeatcount = 0;

			decode[screen].current_object++;
			if (decode[screen].current_object >= DECODECACHE_NUMOBJECTCACHES)
				decode[screen].current_object = 0;
		}
	}




	int sizey = used_vCellCount * 16 * vZoom;

	vPosition *= 0x40;

	switch (vOrigin & 3)
	{
	case 0:
		// top
		break;
	case 1:
		vPosition -= sizey / 2 ;
		// middle?
		break;
	case 2:
		vPosition -= sizey;
		// bottom?
		break;
	case 3:
		// invalid?
		break;
	}



	// Splat some sprites
	for (int v = 0; v < used_vCellCount; v++)
	{


		const int pixelOffsetY = ((vPosition) + (v* 16 * vZoom)) / 0x40;
		const int pixelOffsetnextY = ((vPosition) + ((v+1)* 16 * vZoom)) / 0x40;

		int blockhigh = pixelOffsetnextY - pixelOffsetY;

		if (pixelOffsetY+blockhigh<0)
			continue;



		if (pixelOffsetY>clipmaxY)
		{
			v = used_vCellCount;
			continue;
		}

		// I don't know, the Rainbow is a non-zoomed sprite and won't link unless you move one half
		//  (fixed in the hOrigin handling instead)
		//if (used_flipx)
		//	hPositionx -= 1;

		int hPosition = 0;



		if (!indirect_zoom_enable)
		{

			int sizex = used_hCellCount * 16 * hZoom;

			hPosition = hPositionx * 0x40;

			switch (hOrigin & 3)
			{
			case 0:
				// left
				break;
			case 1:
				hPosition -= sizex / 2;
				// middle?
				break;
			case 2:
				hPosition -= sizex-1;
				// right?
				break;
			case 3:
				// invalid?
				break;
			}
		}

		UINT32 lastSpriteNumber = 0xffffffff;
		UINT16 blankcount = 0;
		int color_offs = (0x7b20 + (b1colorNumber & 0x7ff))*0x40 * 5; /* yes, * 5 */ \
		int color_offs2 = (0x7b20 + (b2colorNumber & 0x7ff))*0x40 * 5; \

		for (int h = 0; h < used_hCellCount; h++)
		{

			int current_decoded = 0;

			if (!indirect_tile_enable && size < DECODECACHE_NUMSPRITETILES)
			{
				tempshape = decode[screen].objcache[use_object].tiles[v*used_hCellCount + h].tempshape_multi;
				current_decoded = decode[screen].objcache[use_object].tiles[v*used_hCellCount + h].tempshape_multi_decoded;
				/*
				if (object->screen==0)
				{
					if (current_decoded) printf("setting temp shape to %04x tile is marked as decoded %08x \n", v*used_hCellCount + h, ((UINT64)(void*)tempshape)&0xffffffff);
					else printf("setting temp shape to %04x tile is marked as NOT decoded %08x \n", v*used_hCellCount + h, ((UINT64)(void*)tempshape)&0xffffffff);
				}
				*/
			}
			else
			{
				//if (object->screen==0) printf("using base tempshape\n");
				tempshape = decode[screen].tempshape;
			}




			UINT32 incy = 0x8000000 / vZoom;

			// DEBUG: Draw 16x16 block
			UINT16* line;
			//UINT16* zline;


			if (indirect_zoom_enable)
			{
				GET_SPRITE_NUMBER
				CHECK_DECODE


				if (blit_rotate)
				{
					#define GET_PIX GET_PIX_ROTATED
					YXLOOP
                    #undef GET_PIX
				}
				else // no rotate
				{
					#define GET_PIX GET_PIX_NORMAL
					YXLOOP
                    #undef GET_PIX

				}
			}
			else  // no indirect zoom
			{
				if (!hZoom)
				{
					// abort, but make sure we clean up
					goto end;
				}

				if ((hZoom==0x40) && (vZoom==0x40)) // non-zoomed
				{
					const int pixelOffsetX = ((hPosition/0x40) + (h* 16));

					if (pixelOffsetX+16 < clipminX)
						continue;

					if (pixelOffsetX>clipmaxX)
						continue;

					GET_SPRITE_NUMBER
					CHECK_DECODE

					if (pixelOffsetX>=clipminX && pixelOffsetX+16<clipmaxX)
					{
						if (blit_rotate)
						{
							#define DO_XCLIP DO_XCLIP_NONE
							#define GET_PIX GET_PIX_ROTATED
							YXLOOP_NO_ZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
						else // no rotate
						{
							#define DO_XCLIP DO_XCLIP_NONE
							#define GET_PIX GET_PIX_NORMAL
							YXLOOP_NO_ZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
					}
					else
					{
						if (blit_rotate)
						{
							#define DO_XCLIP DO_XCLIP_REAL
							#define GET_PIX GET_PIX_ROTATED
							YXLOOP_NO_ZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
						else // no rotate
						{
							#define DO_XCLIP DO_XCLIP_REAL
							#define GET_PIX GET_PIX_NORMAL
							YXLOOP_NO_ZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
					}
				}
				else // zoomed
				{
					const int pixelOffsetX = ((hPosition) + (h* 16 * hZoom)) / 0x40;
					const int pixelOffsetnextX = ((hPosition) + ((h+1)* 16 * hZoom)) / 0x40;

					int blockwide = pixelOffsetnextX-pixelOffsetX;
					UINT32 incx = 0x8000000 / hZoom;

					if (pixelOffsetX+blockwide < clipminX)
						continue;

					if (pixelOffsetX>clipmaxX)
						continue;

					GET_SPRITE_NUMBER
					CHECK_DECODE

					if (pixelOffsetX>=clipminX && pixelOffsetX+blockwide<clipmaxX)
					{
						if (blit_rotate)
						{
							#define DO_XCLIP DO_XCLIP_NONE
							#define GET_PIX GET_PIX_ROTATED
							YXLOOP_NO_LINEZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
						else // no rotate
						{
							#define DO_XCLIP DO_XCLIP_NONE
							#define GET_PIX GET_PIX_NORMAL
							YXLOOP_NO_LINEZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
					}
					else
					{
						if (blit_rotate)
						{
							#define DO_XCLIP DO_XCLIP_REAL
							#define GET_PIX GET_PIX_ROTATED
							YXLOOP_NO_LINEZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
						else // no rotate
						{
							#define DO_XCLIP DO_XCLIP_REAL
							#define GET_PIX GET_PIX_NORMAL
							YXLOOP_NO_LINEZOOM
							#undef GET_PIX
							#undef DO_XCLIP
						}
					}
				} // end zoomed
			} // end no indirect zoom
		}
	}



	end:

	if (object->indirect_tiles)
		free(object->indirect_tiles);

	if (object->indirect_zoom)
		free(object->indirect_zoom);

	free (object);

	return;

}

static void blit_current_sprite(void)
{
	UINT32 blit0 = m_spriteblit[0];

	if (blit0 == 0)
	{
		/* normal */
	}
	else if (blit0 == 1)
	{
		if (m_blitterMode & 0x80)
		{
			if (m_clipblitterMode[1] >= m_blitterMode)
			{
				m_clipvals[1][0] = m_spriteblit[1];
				m_clipvals[1][1] = m_spriteblit[2];
				m_clipvals[1][2] = m_spriteblit[3];
				m_clipblitterMode[1] = m_blitterMode;
			}
		}
		else
		{
			if (m_clipblitterMode[0] >= m_blitterMode)
			{
				m_clipvals[0][0] = m_spriteblit[1];
				m_clipvals[0][1] = m_spriteblit[2];
				m_clipvals[0][2] = m_spriteblit[3];
				m_clipblitterMode[0] = m_blitterMode;
			}
		}
		return;
	}
	else
	{
		return;
	}

	struct cool_render_object *testobject = (struct cool_render_object *)malloc(sizeof(struct cool_render_object));
	int i;

	for (i = 0; i < 12; i++)
		testobject->spriteblit[i] = m_spriteblit[i];

	int test_indirect_tile_enable = (m_spriteblit[5] & 0x00010000) >> 16;
	if (test_indirect_tile_enable)
	{
		UINT32 test_textlookup = m_spriteblit[11];
		UINT16 test_hCellCount = (m_spriteblit[6] & 0x00003ff);
		UINT16 test_vCellCount = (m_spriteblit[6] & 0x03ff0000) >> 16;
		int bytes = test_vCellCount * test_hCellCount;
		testobject->indirect_tiles = (UINT8 *)malloc(bytes);
		for (i = 0; i < bytes; i++)
		{
			testobject->indirect_tiles[i] = program_read_byte_32be(test_textlookup + i);
		}
	}
	else
	{
		testobject->indirect_tiles = NULL;
	}

	int test_indirect_zoom_enable = (m_spriteblit[5] & 0x00000001);
	if (test_indirect_zoom_enable)
	{
		UINT32 test_blit10 = m_spriteblit[10];
		UINT16 test_vCellCount = (m_spriteblit[6] & 0x03ff0000) >> 16;
		int bytes = test_vCellCount * 4 * 16;
		testobject->indirect_zoom = (UINT32 *)malloc(bytes);
		for (i = 0; i < bytes / 4; i++)
		{
			testobject->indirect_zoom[i] = program_read_dword_32be(test_blit10 + i * 4);
		}
	}
	else
	{
		testobject->indirect_zoom = NULL;
	}

	testobject->zpri = m_blitterAddr | (m_blittype << 12);
	testobject->blittype = m_blittype;

	if (m_blitterMode == 0x30 || m_blitterMode == 0x40 || m_blitterMode == 0x4f || m_blitterMode == 0x50 || m_blitterMode == 0x60)
	{
		testobject->drawbitmap = m_temp_bitmap_sprites;
		testobject->clipvals[0] = m_clipvals[0][0];
		testobject->clipvals[1] = m_clipvals[0][1];
		testobject->clipvals[2] = m_clipvals[0][2];
		testobject->screen = 0;

		if (m_listcount1 < MAX_RENDER_OBJECTS)
		{
			m_cool_render_object_list1[m_listcount1] = testobject;
			m_listcount1++;
		}
		else
		{
			if (testobject->indirect_tiles) free(testobject->indirect_tiles);
			if (testobject->indirect_zoom) free(testobject->indirect_zoom);
			free(testobject);
		}
	}
	else
	{
		testobject->drawbitmap = m_temp_bitmap_sprites2;
		testobject->clipvals[0] = m_clipvals[1][0];
		testobject->clipvals[1] = m_clipvals[1][1];
		testobject->clipvals[2] = m_clipvals[1][2];
		testobject->screen = 1;

		if (m_listcount2 < MAX_RENDER_OBJECTS)
		{
			m_cool_render_object_list2[m_listcount2] = testobject;
			m_listcount2++;
		}
		else
		{
			if (testobject->indirect_tiles) free(testobject->indirect_tiles);
			if (testobject->indirect_zoom) free(testobject->indirect_zoom);
			free(testobject);
		}
	}
}

static WRITE32_HANDLER( sysh1_blit_mode_w )
{
	m_blitterMode = (data & 0x00ff0000) >> 16;

	if (m_blitterMode == 0xf4)
	{
		m_textBytesToWrite = (data & 0xff000000) >> 24;
		m_textOffset = (data & 0x0000ffff);
		m_blitterSerialCount = 0;
	}
	else if (m_blitterMode == 0x30 || m_blitterMode == 0x40 || m_blitterMode == 0x4f || m_blitterMode == 0x50 || m_blitterMode == 0x60
		  || m_blitterMode == 0x90 || m_blitterMode == 0xa0 || m_blitterMode == 0xaf || m_blitterMode == 0xb0 || m_blitterMode == 0xc0)
	{
		m_blitterSerialCount = 0;
		if (m_blitterMode >= 0x80)
			m_blittype = m_blitterMode - 0x90;
		else
			m_blittype = m_blitterMode - 0x30;
		m_blittype >>= 4;
		m_blitterAddr = data & 0x00000fff;
	}
	else if (m_blitterMode == 0x10)
	{
		UINT32 vramAddr;
		for (vramAddr = 0x3f40000; vramAddr < 0x3f4ffff; vramAddr += 4)
			program_write_dword_32be(vramAddr, 0x00000000);
		m_blitterSerialCount = 0;
	}
	else if (m_blitterMode == 0xe0)
	{
		m_blitterSerialCount = 0;
		m_textOffset = (data & 0x0000ffff);
	}
}

static WRITE32_HANDLER( sysh1_blit_data_w )
{
	if (m_blitterMode == 0xf4)
	{
		const UINT32 memOffset = 0x03f40000 + m_textOffset + m_blitterSerialCount;
		program_write_dword_32be(memOffset, data);
		m_blitterSerialCount += 0x04;
	}
	else if (m_blitterMode == 0x30 || m_blitterMode == 0x40 || m_blitterMode == 0x4f || m_blitterMode == 0x50 || m_blitterMode == 0x60
		  || m_blitterMode == 0x90 || m_blitterMode == 0xa0 || m_blitterMode == 0xaf || m_blitterMode == 0xb0 || m_blitterMode == 0xc0)
	{
		if (m_blitterSerialCount < 12)
		{
			m_spriteblit[m_blitterSerialCount] = data;
			m_blitterSerialCount++;
		}
		if (m_blitterSerialCount == 12)
		{
			blit_current_sprite();
		}
	}
	else if (m_blitterMode == 0x10)
	{
		m_blitterSerialCount++;
	}
	else if (m_blitterMode == 0xe0)
	{
		const UINT32 memOffset = 0x3c00000 + m_textOffset + m_blitterSerialCount;
		program_write_dword_32be(memOffset, data);
		m_blitterSerialCount += 0x04;
	}
}

static WRITE32_HANDLER( sysh1_fb_mode_w )
{
	COMBINE_DATA(&m_blitterClearMode);
	m_blitterClearCount = 0;
}

static int zpri_compare(const void *a, const void *b)
{
	const struct cool_render_object *oa = *(const struct cool_render_object **)a;
	const struct cool_render_object *ob = *(const struct cool_render_object **)b;
	if (oa->zpri < ob->zpri) return -1;
	if (oa->zpri > ob->zpri) return 1;
	return 0;
}

static WRITE32_HANDLER( sysh1_fb_data_w )
{
	if (m_blitterClearCount == 3)
	{
		rectangle visarea;
		visarea.min_x = 0;
		visarea.max_x = 495;
		visarea.min_y = 0;
		visarea.max_y = 383;
		int i;

		if (m_blitterClearMode == 0x8c200000)
		{
			copybitmap(m_screen1_bitmap, m_temp_bitmap_sprites, 0, 0, 0, 0, &visarea, TRANSPARENCY_NONE, 0);
			draw_bg_coolridr(m_temp_bitmap_sprites, &visarea, 0);

			m_clipvals[0][0] = 0;
			m_clipvals[0][1] = 0;
			m_clipvals[0][2] = 0;
			m_clipblitterMode[0] = 0xff;

			qsort(m_cool_render_object_list1, m_listcount1, sizeof(struct cool_render_object *), zpri_compare);

			for (i = m_listcount1 - 1; i >= 0; i--)
			{
				coolridr_draw_object(m_cool_render_object_list1[i]);
			}
			m_listcount1 = 0;
		}
		else if (m_blitterClearMode == 0x8c800000)
		{
			copybitmap(m_screen2_bitmap, m_temp_bitmap_sprites2, 0, 0, 0, 0, &visarea, TRANSPARENCY_NONE, 0);
			draw_bg_coolridr(m_temp_bitmap_sprites2, &visarea, 1);

			m_clipvals[1][0] = 0;
			m_clipvals[1][1] = 0;
			m_clipvals[1][2] = 0;
			m_clipblitterMode[1] = 0xff;

			qsort(m_cool_render_object_list2, m_listcount2, sizeof(struct cool_render_object *), zpri_compare);

			for (i = m_listcount2 - 1; i >= 0; i--)
			{
				coolridr_draw_object(m_cool_render_object_list2[i]);
			}
			m_listcount2 = 0;
		}
	}

	m_blitterClearCount++;
}

static READ32_HANDLER( sysh1_unk_blit_r )
{
	return m_sysh1_txt_blit[offset];
}

static WRITE32_HANDLER( sysh1_unk_blit_w )
{
	COMBINE_DATA(&m_sysh1_txt_blit[offset]);
}

static void sysh1_dma_transfer(UINT16 dma_index)
{
	UINT32 src = 0, dst = 0, size = 0;
	UINT8 end_dma_mark = 0;
	UINT8 cmd;

	do
	{
		cmd = (m_framebuffer_vram[(0 + dma_index) / 4] & 0xff000000) >> 24;

		if (m_framebuffer_vram[(0 + dma_index) / 4] & 0x800000)
			end_dma_mark = 1;

		switch (cmd)
		{
			case 0x00:
			case 0x01:
				dma_index += 4;
				break;
			case 0xc0:
				src = (m_framebuffer_vram[(0 + dma_index) / 4] & 0x03ffffff);
				dst = (m_framebuffer_vram[(4 + dma_index) / 4]);
				size = m_framebuffer_vram[(8 + dma_index) / 4];
				dst &= 0x000ffffe;
				dst >>= 1;

				for (UINT32 i = 0; i < size; i += 2)
				{
					m_h1_vram[dst] = program_read_word_32be(src);
					dst++;
					src += 2;
				}
				dma_index += 0xc;
				break;

			case 0xd0:
				src = (m_framebuffer_vram[(0 + dma_index) / 4] & 0x03ffffff);
				dst = (m_framebuffer_vram[(4 + dma_index) / 4]);
				size = m_framebuffer_vram[(8 + dma_index) / 4];
				dst &= 0x000fffff;

				for (UINT32 i = 0; i < size; i++)
				{
					m_h1_pcg[dst] = program_read_byte_32be(src);
					pcg_dirty[(dst / 256) & 0xfff] = 1;
					dst++;
					src++;
				}
				dma_index += 0xc;
				break;

			case 0xe0:
				src = (m_framebuffer_vram[(0 + dma_index) / 4] & 0x03ffffff);
				dst = (m_framebuffer_vram[(4 + dma_index) / 4]);
				size = m_framebuffer_vram[(8 + dma_index) / 4];
				dst &= 0x000ffffe;
				dst >>= 1;

				for (UINT32 i = 0; i < size; i += 2)
				{
					m_h1_pal[dst] = program_read_word_32be(src);
					m_tilepals[dst & 0xffff] = m_h1_pal[dst];
					dst++;
					src += 2;
				}
				dma_index += 0xc;
				break;

			case 0x04:
			case 0x10:
			case 0x20:
			case 0x24:
			case 0x50:
			case 0x54:
				dma_index += 4;
				break;
			case 0x30:
			case 0x34:
				m_pen_fill[(cmd & 4) >> 2] = m_framebuffer_vram[(0 + dma_index) / 4] & 0xffffff;
				dma_index += 4;
				break;
			case 0x40:
			case 0x44:
				m_rgb_ctrl[(cmd & 4) >> 2].setting = m_framebuffer_vram[(0 + dma_index) / 4] & 0xffffe0;
				m_rgb_ctrl[(cmd & 4) >> 2].gradient = m_framebuffer_vram[(0 + dma_index) / 4] & 0x1f;
				dma_index += 4;
				break;
			default:
				dma_index += 4;
				break;
		}
	} while (!end_dma_mark);
}

static WRITE32_HANDLER( sysh1_dma_w )
{
	COMBINE_DATA(&m_framebuffer_vram[offset]);

	if (offset * 4 == 0x000)
	{
		if ((m_framebuffer_vram[offset] & 0xff000000) == 0x0f000000)
			sysh1_dma_transfer(m_framebuffer_vram[offset] & 0xffff);
	}
}

static READ32_HANDLER( h1_soundram_r32 )
{
	UINT32 res = 0;
	if (ACCESSING_MSW32)
		res |= (UINT32)m_soundram[offset * 2] << 16;
	if (ACCESSING_LSW32)
		res |= (UINT32)m_soundram[offset * 2 + 1];
	return res;
}

static WRITE32_HANDLER( h1_soundram_w32 )
{
	/* Cleared mem_mask bits select the bytes written by the sub CPU. */
	if (ACCESSING_MSW32)
	{
		UINT32 d = data >> 16;
		UINT32 m = (~mem_mask >> 16) & 0xffff;
		m_soundram[offset * 2] = (m_soundram[offset * 2] & ~m) | (d & m);
	}
	if (ACCESSING_LSW32)
	{
		UINT32 d = data & 0xffff;
		UINT32 m = ~mem_mask & 0xffff;
		m_soundram[offset * 2 + 1] = (m_soundram[offset * 2 + 1] & ~m) | (d & m);
	}
}

static READ32_HANDLER( h1_soundram2_r32 )
{
	UINT32 res = 0;
	if (ACCESSING_MSW32)
		res |= (UINT32)m_soundram2[offset * 2] << 16;
	if (ACCESSING_LSW32)
		res |= (UINT32)m_soundram2[offset * 2 + 1];
	return res;
}

static WRITE32_HANDLER( h1_soundram2_w32 )
{
	/* Keep the 68000 RAM in sync with sub CPU command and sample writes. */
	if (ACCESSING_MSW32)
	{
		UINT32 d = data >> 16;
		UINT32 m = (~mem_mask >> 16) & 0xffff;
		m_soundram2[offset * 2] = (m_soundram2[offset * 2] & ~m) | (d & m);
	}
	if (ACCESSING_LSW32)
	{
		UINT32 d = data & 0xffff;
		UINT32 m = ~mem_mask & 0xffff;
		m_soundram2[offset * 2 + 1] = (m_soundram2[offset * 2 + 1] & ~m) | (d & m);
	}
}

static READ32_HANDLER( scsp0_r32 )
{
	UINT32 res = 0;
	if (ACCESSING_MSW32)
		res |= (UINT32)SCSP_0_r(offset * 2, mem_mask >> 16) << 16;
	if (ACCESSING_LSW32)
		res |= (UINT32)SCSP_0_r(offset * 2 + 1, mem_mask & 0xffff);
	return res;
}

static WRITE32_HANDLER( scsp0_w32 )
{
	if (ACCESSING_MSW32)
		SCSP_0_w(offset * 2, data >> 16, mem_mask >> 16);
	if (ACCESSING_LSW32)
		SCSP_0_w(offset * 2 + 1, data & 0xffff, mem_mask & 0xffff);
}

static READ32_HANDLER( scsp1_r32 )
{
	UINT32 res = 0;
	if (ACCESSING_MSW32)
		res |= (UINT32)SCSP_1_r(offset * 2, mem_mask >> 16) << 16;
	if (ACCESSING_LSW32)
		res |= (UINT32)SCSP_1_r(offset * 2 + 1, mem_mask & 0xffff);
	return res;
}

static WRITE32_HANDLER( scsp1_w32 )
{
	if (ACCESSING_MSW32)
		SCSP_1_w(offset * 2, data >> 16, mem_mask >> 16);
	if (ACCESSING_LSW32)
		SCSP_1_w(offset * 2 + 1, data & 0xffff, mem_mask & 0xffff);
}

static READ32_HANDLER( analog_mux_r )
{
	UINT8 adc_data = 0;
	switch (an_mux_data)
	{
		case 0x0: adc_data = readinputportbytag_safe("AN0", 0x80); break;
		case 0x1: adc_data = readinputportbytag_safe("AN1", 0x00); break;
		case 0x2: adc_data = readinputportbytag_safe("AN2", 0x00); break;
		case 0x3: adc_data = readinputportbytag_safe("AN3", 0xff); break;
		case 0x4: adc_data = readinputportbytag_safe("AN4", 0x80); break;
		case 0x5: adc_data = readinputportbytag_safe("AN5", 0x00); break;
		case 0x6: adc_data = readinputportbytag_safe("AN6", 0x00); break;
		case 0x7: adc_data = readinputportbytag_safe("AN7", 0xff); break;
	}
	an_mux_data++;
	an_mux_data &= 0x7;
	return adc_data;
}

static WRITE32_HANDLER( analog_mux_w )
{
	an_mux_data = data & 0xff;
}

static WRITE32_HANDLER( lamps_w )
{
}

static READ32_HANDLER( sysh1_sound_dma_r )
{
	if (offset == 8)
	{
		return sound_data;
	}

	if (offset == 2 || offset == 6)
		return 0;

	return m_sound_dma[offset];
}

static WRITE32_HANDLER( sysh1_sound_dma_w )
{
	if (offset == 8)
	{
		cpunum_set_input_line(1, INPUT_LINE_RESET, (data) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}

	if (offset == 2)
	{
		if ((data & 1) && (!(m_sound_dma[2] & 1)))
		{
			UINT32 src = m_sound_dma[0];
			UINT32 dst = m_sound_dma[1];
			UINT32 size = (m_sound_dma[2] >> 16) * 0x40;
			UINT32 i;

			for (i = 0; i < size; i += 2)
			{
				UINT16 val = (cpunum_read_byte(0, src) << 8) | cpunum_read_byte(0, src + 1);
				cpunum_write_byte(1, dst, val >> 8);
				cpunum_write_byte(1, dst + 1, val & 0xff);
				src += 2;
				dst += 2;
			}
		}
	}

	if (offset == 6)
	{
		if ((data & 1) && (!(m_sound_dma[6] & 1)))
		{
			UINT32 src = m_sound_dma[4];
			UINT32 dst = m_sound_dma[5];
			UINT32 size = (m_sound_dma[6] >> 16) * 0x40;
			UINT32 i;

			for (i = 0; i < size; i += 2)
			{
				UINT16 val = (cpunum_read_byte(0, src) << 8) | cpunum_read_byte(0, src + 1);
				cpunum_write_byte(1, dst, val >> 8);
				cpunum_write_byte(1, dst + 1, val & 0xff);
				src += 2;
				dst += 2;
			}
		}
	}

	COMBINE_DATA(&m_sound_dma[offset]);
}

static WRITE16_HANDLER( sound_to_sh1_w )
{
	if (ACCESSING_LSB)
		sound_fifo = data & 0xff;
}

static READ32_HANDLER( in0_r ) { return readinputportbytag_safe("IN0", 0xffffffff); }
static READ32_HANDLER( in1_r ) { return readinputportbytag_safe("IN1", 0xffffffff); }
static READ32_HANDLER( in2_r ) { return readinputportbytag_safe("IN2", 0xffffffff); }
static READ32_HANDLER( in3_r ) { return readinputportbytag_safe("IN3", 0xffffffff); }
static READ32_HANDLER( in5_r ) { return readinputportbytag_safe("IN5", 0xffffffff); }
static READ32_HANDLER( in6_r ) { return readinputportbytag_safe("IN6", 0xffffffff); }

static ADDRESS_MAP_START( system_h1_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM AM_WRITENOP
	AM_RANGE(0x01000000, 0x01ffffff) AM_ROM AM_REGION(REGION_USER1, 0)

	AM_RANGE(0x03c00000, 0x03c1ffff) AM_MIRROR(0x00200000) AM_READ(MRA32_RAM) AM_WRITE(sysh1_dma_w) AM_BASE(&m_framebuffer_vram)

	AM_RANGE(0x03f00000, 0x03f0ffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x03f40000, 0x03f4ffff) AM_RAM AM_BASE(&m_txt_vram)
	AM_RANGE(0x04000000, 0x0400000f) AM_READWRITE(sysh1_unk_blit_r, sysh1_unk_blit_w) AM_BASE(&m_sysh1_txt_blit)
	AM_RANGE(0x04000010, 0x04000013) AM_WRITE(sysh1_blit_mode_w)
	AM_RANGE(0x04000014, 0x04000017) AM_WRITE(sysh1_blit_data_w)
	AM_RANGE(0x04000018, 0x0400001b) AM_WRITE(sysh1_fb_mode_w)
	AM_RANGE(0x0400001c, 0x0400001f) AM_WRITE(sysh1_fb_data_w)

	AM_RANGE(0x06000000, 0x060fffff) AM_RAM AM_BASE(&m_sysh1_workram_h)
	AM_RANGE(0x20000000, 0x201fffff) AM_ROM

	AM_RANGE(0x60000000, 0x600003ff) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( coolridr_submap, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0001ffff) AM_ROM

	AM_RANGE(0x01000000, 0x0100ffff) AM_RAM

	AM_RANGE(0x03000000, 0x0307ffff) AM_READWRITE(h1_soundram_r32, h1_soundram_w32)
	AM_RANGE(0x03100000, 0x03100fff) AM_READWRITE(scsp0_r32, scsp0_w32)
	AM_RANGE(0x03200000, 0x0327ffff) AM_READWRITE(h1_soundram2_r32, h1_soundram2_w32)
	AM_RANGE(0x03300000, 0x03300fff) AM_READWRITE(scsp1_r32, scsp1_w32)

	AM_RANGE(0x04000000, 0x0400003f) AM_READWRITE(sysh1_sound_dma_r, sysh1_sound_dma_w) AM_BASE(&m_sound_dma)

	AM_RANGE(0x05000000, 0x05000fff) AM_RAM
	AM_RANGE(0x05200000, 0x052001ff) AM_RAM
	AM_RANGE(0x05300000, 0x0530ffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x05ff0000, 0x05ffffff) AM_RAM
	AM_RANGE(0x06000000, 0x060001ff) AM_RAM AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x06100000, 0x06100003) AM_READ(in0_r) AM_WRITE(lamps_w)
	AM_RANGE(0x06100004, 0x06100007) AM_READ(in1_r)
	AM_RANGE(0x06100008, 0x0610000b) AM_READ(in5_r)
	AM_RANGE(0x0610000c, 0x0610000f) AM_READ(in6_r)
	AM_RANGE(0x06100010, 0x06100013) AM_READ(in2_r) AM_WRITENOP
	AM_RANGE(0x06100014, 0x06100017) AM_READ(in3_r)
	AM_RANGE(0x0610001c, 0x0610001f) AM_READWRITE(analog_mux_r, analog_mux_w)
	AM_RANGE(0x06200000, 0x06200fff) AM_RAM
	AM_RANGE(0x07fff000, 0x07ffffff) AM_RAM
	AM_RANGE(0x20000000, 0x2001ffff) AM_ROM

	AM_RANGE(0x60000000, 0x600003ff) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( system_h1_sound_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_RAM AM_REGION(REGION_SOUND1, 0)
	AM_RANGE(0x100000, 0x100fff) AM_READWRITE(SCSP_0_r, SCSP_0_w)
	AM_RANGE(0x200000, 0x27ffff) AM_RAM AM_REGION(REGION_SOUND2, 0)
	AM_RANGE(0x300000, 0x300fff) AM_READWRITE(SCSP_1_r, SCSP_1_w)
	AM_RANGE(0x800000, 0x80ffff) AM_MIRROR(0x200000) AM_RAM
	AM_RANGE(0x900000, 0x900001) AM_WRITE(sound_to_sh1_w)
ADDRESS_MAP_END
static INPUT_PORTS_START( coolridr )
	PORT_START_TAG("IN0")
	PORT_DIPNAME( 0x00000001, 0x00000001, "IN0-0" )
	PORT_DIPSETTING(    0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00010000, 0x00010000, "IN0-1" )
	PORT_DIPSETTING(    0x00010000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00020000, 0x00020000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00020000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00040000, 0x00040000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00040000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00080000, 0x00080000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00080000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00100000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00200000, 0x00200000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00200000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00400000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00800000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_BIT( 0xff00ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x00000003, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Music <<") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Music >>") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Shift Up")
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_NAME("P1 Shift Down")
	PORT_BIT( 0x000000c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0000ff00, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_NAME("P1 Coin")
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_NAME("P2 Coin")
	PORT_SERVICE_NO_TOGGLE( 0x00040000, IP_ACTIVE_LOW )
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("P1 Service Switch")
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("P1 Start")
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_START2 ) PORT_NAME("P2 Start")
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("P2 Service Switch")
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff000000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN2")
	PORT_DIPNAME( 0x00000001, 0x00000001, "IN2-0" )
	PORT_DIPSETTING(    0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00010000, 0x00010000, "IN2-1" )
	PORT_DIPSETTING(    0x00010000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00020000, 0x00020000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00020000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00040000, 0x00040000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00040000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00080000, 0x00080000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00080000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00100000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00200000, 0x00200000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00200000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00400000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00800000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_BIT( 0xff00ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN3")
	PORT_DIPNAME( 0x00000001, 0x00000001, "IN3-0" )
	PORT_DIPSETTING(    0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00010000, 0x00010000, "IN3-1" )
	PORT_DIPSETTING(    0x00010000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00020000, 0x00020000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00020000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00040000, 0x00040000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00040000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00080000, 0x00080000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00080000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00100000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00200000, 0x00200000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00200000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00400000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00800000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_BIT( 0xff00ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN4")
	PORT_DIPNAME( 0x00000001, 0x00000001, "IN4-0" )
	PORT_DIPSETTING(    0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00010000, 0x00010000, "IN4-1" )
	PORT_DIPSETTING(    0x00010000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00020000, 0x00020000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00020000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00040000, 0x00040000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00040000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00080000, 0x00080000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00080000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00100000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00200000, 0x00200000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00200000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00400000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00800000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_BIT( 0xff00ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN5")
	PORT_DIPNAME( 0x00000001, 0x00000001, "IN5-0" )
	PORT_DIPSETTING(    0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_BIT( 0x00030000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Music <<") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Music >>") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Shift Up")
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 Shift Down")
	PORT_BIT( 0x00c00000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN6")
	PORT_DIPNAME( 0x00000001, 0x00000001, "IN6-0" )
	PORT_DIPSETTING(    0x00000001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000002, 0x00000002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000004, 0x00000004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000008, 0x00000008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000010, 0x00000010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000020, 0x00000020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000040, 0x00000040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00000080, 0x00000080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00000080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00010000, 0x00010000, "IN6-1" )
	PORT_DIPSETTING(    0x00010000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00020000, 0x00020000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00020000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00040000, 0x00040000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00040000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00080000, 0x00080000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00080000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00100000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00200000, 0x00200000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00200000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00400000, 0x00400000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00400000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00800000, 0x00800000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00800000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00000000, DEF_STR( On ) )
	PORT_BIT( 0xff00ff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("AN0")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(60) PORT_PLAYER(1) PORT_NAME("P1 Handle Bar")

	PORT_START_TAG("AN1")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(60) PORT_PLAYER(1) PORT_REVERSE PORT_NAME("P1 Throttle")

	PORT_START_TAG("AN2")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(60) PORT_PLAYER(1) PORT_REVERSE PORT_NAME("P1 Brake")

	PORT_START_TAG("AN3")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("AN4")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(60) PORT_PLAYER(2) PORT_NAME("P2 Handle Bar")

	PORT_START_TAG("AN5")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(60) PORT_PLAYER(2) PORT_REVERSE PORT_NAME("P2 Throttle")

	PORT_START_TAG("AN6")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(60) PORT_PLAYER(2) PORT_REVERSE PORT_NAME("P2 Brake")

	PORT_START_TAG("AN7")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )


	// driver debug
	INPUT_PORTS_END
#define READ_COMPRESSED_ROM(chip) \
	(m_compressedgfx[(chip)*0x400000 + romoffset] << 8 | m_compressedgfx[(chip)*0x0400000 + romoffset +1])

static UINT32 get_20bit_data(UINT32 romoffset, int _20bitwordnum)
{
	UINT16 testvalue, testvalue2;

	int temp = _20bitwordnum & 3;
	int inc = 0;
	if (_20bitwordnum & 4) inc = 5;

	romoffset += (_20bitwordnum >> 3) * 2;

	if (temp == 0)
	{
		testvalue  = READ_COMPRESSED_ROM(0 + inc);
		testvalue2 = READ_COMPRESSED_ROM(1 + inc);
		return (testvalue << 4) | (testvalue2 & 0xf000) >> 12;
	}
	else if (temp == 1)
	{
		testvalue  = READ_COMPRESSED_ROM(1 + inc);
		testvalue2 = READ_COMPRESSED_ROM(2 + inc);
		return ((testvalue & 0x0fff) << 8) | (testvalue2 & 0xff00) >> 8;
	}
	else if (temp == 2)
	{
		testvalue  = READ_COMPRESSED_ROM(2 + inc);
		testvalue2 = READ_COMPRESSED_ROM(3 + inc);
		return ((testvalue & 0x00ff) << 12) | (testvalue2 & 0xfff0) >> 4;
	}
	else
	{
		testvalue  = READ_COMPRESSED_ROM(3 + inc);
		testvalue2 = READ_COMPRESSED_ROM(4 + inc);
		return ((testvalue & 0x000f) << 16) | (testvalue2);
	}
}

static UINT16 get_10bit_data(UINT32 romoffset, int _10bitwordnum)
{
	UINT32 data = get_20bit_data(romoffset, _10bitwordnum >> 1);
	if (_10bitwordnum & 1) return data & 0x3ff;
	else return (data >> 10) & 0x3ff;
}

static void scsp_irq(int irq)
{
	/* The SCSP reports zero on acknowledgement, not the previous IRQ level. */
	if (m_scsp_last_line && m_scsp_last_line != irq)
		cpunum_set_input_line(1, m_scsp_last_line, CLEAR_LINE);
	m_scsp_last_line = irq;
	if (irq > 0)
		cpunum_set_input_line(1, irq, ASSERT_LINE);
}

static void scsp1_to_sh1_irq(int state)
{
	sound_data = (sound_data & ~0x10) | (state ? 0x10 : 0);
	/* Both chips share the sub CPU interrupt line. */
	cpunum_set_input_line(2, 0xe, (sound_data & 0x30) ? ASSERT_LINE : CLEAR_LINE);
}

static void scsp2_to_sh1_irq(int state)
{
	sound_data = (sound_data & ~0x20) | (state ? 0x20 : 0);
	cpunum_set_input_line(2, 0xe, (sound_data & 0x30) ? ASSERT_LINE : CLEAR_LINE);
}

static struct SCSPinterface scsp_config =
{
	REGION_SOUND1,
	0,
	scsp_irq,
	scsp1_to_sh1_irq
};

static void scsp2_irq(int irq)
{
}

static struct SCSPinterface scsp2_config =
{
	REGION_SOUND2,
	0,
	scsp2_irq,
	scsp2_to_sh1_irq
};

PALETTE_INIT( coolridr )
{
	int i;
	for (i = 0; i < 0x8000; i++)
	{
		palette_set_color(i, pal5bit((i >> 10) & 0x1f), pal5bit((i >> 5) & 0x1f), pal5bit(i & 0x1f));
	}
}

VIDEO_START( coolridr )
{
	m_temp_bitmap_sprites = auto_bitmap_alloc(496, 384);
	m_temp_bitmap_sprites2 = auto_bitmap_alloc(496, 384);
	m_screen1_bitmap = auto_bitmap_alloc(496, 384);
	m_screen2_bitmap = auto_bitmap_alloc(496, 384);

	if (!m_temp_bitmap_sprites || !m_temp_bitmap_sprites2 || !m_screen1_bitmap || !m_screen2_bitmap)
		return 1;

	coolridr_gfx = allocgfx(&h1_tile_layout);
	if (!coolridr_gfx)
		return 1;

	coolridr_gfx->total_colors = 256;
	memset(pcg_dirty, 1, sizeof(pcg_dirty));

	return 0;
}

VIDEO_UPDATE( coolridr )
{
	int y, x;
	screen_update_coolridr(0);
	screen_update_coolridr(1);

	for (y = 0; y < 384; y++)
	{
		UINT16 *linesrc1 = (UINT16 *)m_screen1_bitmap->line[y];
		UINT16 *linesrc2 = (UINT16 *)m_screen2_bitmap->line[y];
		UINT16 *linedest = (UINT16 *)bitmap->line[y];

		for (x = 0; x < 496; x++)
		{
			linedest[x] = m_fadedpals[0][linesrc1[x] & 0x7fff];
			linedest[496 + x] = m_fadedpals[1][linesrc2[x] & 0x7fff];
		}
	}

	}

static void scanline_timer_callback(int param)
{
	if (scanline_count == 0)
	{
		cpunum_set_input_line(0, 6, HOLD_LINE);
	}
	else if (scanline_count == 384)
	{
		cpunum_set_input_line(0, 4, HOLD_LINE);
		cpunum_set_input_line(2, 0xc, HOLD_LINE);
	}

	scanline_count++;
	if (scanline_count >= 512)
		scanline_count = 0;
}

MACHINE_RESET( coolridr )
{
	scanline_count = 0;
	timer_pulse(cpu_getscanlineperiod(), 0, scanline_timer_callback);

	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);

	SCSP_set_ram_base(0, memory_region(REGION_SOUND1));
	SCSP_set_ram_base(1, memory_region(REGION_SOUND2));
	m_soundram = (UINT16 *)memory_region(REGION_SOUND1);
	m_soundram2 = (UINT16 *)memory_region(REGION_SOUND2);

	m_blitterClearCount = 0;
	m_blitterSerialCount = 0;
	m_blitterMode = 0;
	an_mux_data = 0;
	sound_data = 0;
	m_scsp_last_line = 0;
	sound_fifo = 0;
	m_listcount1 = 0;
	m_listcount2 = 0;
}

static READ32_HANDLER( coolridr_hack2_r )
{
	/* The watchdog acknowledgement must not depend on the executing SH-2 PC. */
	return m_sysh1_workram_h[0xd8894 / 4] & ~0x0000ff00;
}

DRIVER_INIT( coolridr )
{
	size_t size;
	UINT16 *compressed;
	int count, i;

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x60d8894, 0x60d8897, 0, 0, coolridr_hack2_r);

	m_compressedgfx = memory_region(REGION_USER2);
	size = memory_region_length(REGION_USER2);

	m_expanded_10bit_gfx = auto_malloc(((size / 10) * 16));
	for (i = 0; i < (0x800000 * 8) / 2; i++)
	{
		m_expanded_10bit_gfx[i] = get_10bit_data(0, i);
	}

	m_rearranged_16bit_gfx = auto_malloc(size);
	compressed = (UINT16 *)m_compressedgfx;
	count = 0;
	for (i = 0; i < size / 2 / 10; i++)
	{
		int k;
		for (k = 0; k < 10; k++)
		{
			UINT16 val = compressed[i + ((0x0400000 / 2) * k)];
			m_rearranged_16bit_gfx[count + k] = ((val & 0x00ff) << 8) | ((val & 0xff00) >> 8);
		}
		count += 10;
	}

	m_h1_vram = auto_malloc(VRAM_SIZE * sizeof(UINT16));
	m_h1_pcg = auto_malloc(VRAM_SIZE * sizeof(UINT8));
	m_h1_pal = auto_malloc(VRAM_SIZE * sizeof(UINT16));

	memset(m_h1_vram, 0, VRAM_SIZE * sizeof(UINT16));
	memset(m_h1_pcg, 0, VRAM_SIZE * sizeof(UINT8));
	memset(m_h1_pal, 0, VRAM_SIZE * sizeof(UINT16));

	m_cool_render_object_list1 = auto_malloc(MAX_RENDER_OBJECTS * sizeof(struct cool_render_object *));
	m_cool_render_object_list2 = auto_malloc(MAX_RENDER_OBJECTS * sizeof(struct cool_render_object *));
	m_listcount1 = 0;
	m_listcount2 = 0;

	decode[0].objcache = auto_malloc(DECODECACHE_NUMOBJECTCACHES * sizeof(struct objectcache));
	decode[1].objcache = auto_malloc(DECODECACHE_NUMOBJECTCACHES * sizeof(struct objectcache));
	memset(decode[0].objcache, 0, DECODECACHE_NUMOBJECTCACHES * sizeof(struct objectcache));
	memset(decode[1].objcache, 0, DECODECACHE_NUMOBJECTCACHES * sizeof(struct objectcache));
	decode[0].current_object = 0;
	decode[1].current_object = 0;

	debug_randompal = 9;
}

static MACHINE_DRIVER_START( coolridr )
	MDRV_CPU_ADD_TAG("main", SH2, 28636360)
	MDRV_CPU_PROGRAM_MAP(system_h1_map, 0)

	MDRV_CPU_ADD_TAG("sound", M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(system_h1_sound_map, 0)

	MDRV_CPU_ADD_TAG("sub", SH2, 16000000)
	MDRV_CPU_PROGRAM_MAP(coolridr_submap, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_MACHINE_RESET(coolridr)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(992, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_PALETTE_INIT(coolridr)

	MDRV_VIDEO_START(coolridr)
	MDRV_VIDEO_UPDATE(coolridr)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp_config)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(SCSP, 0)
	MDRV_SOUND_CONFIG(scsp2_config)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( coolridr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* SH2 code */
	ROM_LOAD32_WORD_SWAP( "ep17659.30", 0x0000000, 0x080000, CRC(473027b0) SHA1(acaa212869dd79550235171b9f054e82750f74c3) )
	ROM_LOAD32_WORD_SWAP( "ep17658.29", 0x0000002, 0x080000, CRC(7ecfdfcc) SHA1(97cb3e6cf9764c8db06de12e4e958148818ef737) )
	ROM_LOAD32_WORD_SWAP( "ep17661.32", 0x0100000, 0x080000, CRC(81a7d90b) SHA1(99f8c3e75b94dd1b60455c26dc38ce08db82fe32) )
	ROM_LOAD32_WORD_SWAP( "ep17660.31", 0x0100002, 0x080000, CRC(27b7a507) SHA1(4c28b1d18d75630a73194b5d4fd166f3b647c595) )

	ROM_REGION32_BE( 0x1000000, REGION_USER1, 0 ) /* gfx data read by SH2 */
	ROM_LOAD32_WORD_SWAP( "mp17650.11", 0x0000002, 0x0200000, CRC(0ccc84a1) SHA1(65951685b0c8073f6bd1cf9959e1b4d0fc6031d8) )
	ROM_LOAD32_WORD_SWAP( "mp17651.12", 0x0000000, 0x0200000, CRC(25fd7dde) SHA1(a1c3f3d947ce20fbf61ea7ab235259be9b7d35a8) )
	ROM_LOAD32_WORD_SWAP( "mp17652.13", 0x0400002, 0x0200000, CRC(be9b4d05) SHA1(0252ba647434f69d6eacb4efc6f55e6af534c7c5) )
	ROM_LOAD32_WORD_SWAP( "mp17653.14", 0x0400000, 0x0200000, CRC(64d1406d) SHA1(779dbbf42a14a6be1de9afbae5bbb18f8f36ceb3) )
	ROM_LOAD32_WORD_SWAP( "mp17654.15", 0x0800002, 0x0200000, CRC(5dee5cba) SHA1(6e6ec8574bdd35cc27903fc45f0d4a36ce9df103) )
	ROM_LOAD32_WORD_SWAP( "mp17655.16", 0x0800000, 0x0200000, CRC(02903cf2) SHA1(16d555fda144e0f1b62b428e9158a0e8ebf7084e) )
	ROM_LOAD32_WORD_SWAP( "mp17656.17", 0x0c00002, 0x0200000, CRC(945c89e3) SHA1(8776d74f73898d948aae3c446d7c710ad0407603) )
	ROM_LOAD32_WORD_SWAP( "mp17657.18", 0x0c00000, 0x0200000, CRC(74676b1f) SHA1(b4a9003a052bde93bebfa4bef9e8dff65003c3b2) )

	ROM_REGION( 0x100000, REGION_CPU2, ROMREGION_ERASE00 )   /* 68000 */

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* SH1/SH2 */
	ROM_LOAD16_WORD_SWAP( "ep17662.12", 0x000000, 0x020000,  CRC(50d66b1f) SHA1(f7b7f2f5b403a13b162f941c338a3e1207762a0b) )

	/* these are compressed sprite data */
	ROM_REGION( 0x2800000, REGION_USER2, ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "mpr-17644.ic5", 0x0000000, 0x0400000, CRC(80199c79) SHA1(e525d8ee9f9176101629853e50cca73b02b16a38) )
	ROM_LOAD16_WORD_SWAP( "mpr-17643.ic4", 0x0400000, 0x0400000, CRC(5100f23b) SHA1(659c2300399ff1cbd24fb1eb18cfd6c26e06fd96) )
	ROM_LOAD16_WORD_SWAP( "mpr-17642.ic3", 0x0800000, 0x0400000, CRC(1a5bcc73) SHA1(a7df04c0a326323ea185db5f55b3e0449d76c535) )
	ROM_LOAD16_WORD_SWAP( "mpr-17641.ic2", 0x0c00000, 0x0400000, CRC(fccc3dae) SHA1(0df7fd8b1110ba9063dc4dc40301267229cb9a35) )
	ROM_LOAD16_WORD_SWAP( "mpr-17640.ic1", 0x1000000, 0x0400000, CRC(981e3e69) SHA1(d242055e0359ec4b5fac4676b2f974fbc974cc68) )
	ROM_LOAD16_WORD_SWAP( "mpr-17649.ic10",0x1400000, 0x0400000, CRC(618c47ae) SHA1(5b69ad36fcf8e70d34c3b2fc71412ce953c5ceb3) )
	ROM_LOAD16_WORD_SWAP( "mpr-17648.ic9", 0x1800000, 0x0400000, CRC(bf184cce) SHA1(62c004ea279f9a649d21426369336c2e1f9d24da) )
	ROM_LOAD16_WORD_SWAP( "mpr-17647.ic8", 0x1c00000, 0x0400000, CRC(9dd9330c) SHA1(c91a7f497c1f4bd283bd683b06dff88893724d51) )
	ROM_LOAD16_WORD_SWAP( "mpr-17646.ic7", 0x2000000, 0x0400000, CRC(b77eb2ad) SHA1(b832c0f1798aca39adba840d56ae96a75346670a) )
	ROM_LOAD16_WORD_SWAP( "mpr-17645.ic6", 0x2400000, 0x0400000, CRC(56968d07) SHA1(e88c3d66ea05affb4681a25d155f097bd1b5a84b) )

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_ERASE00 )   /* first SCSP's RAM */
	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_ERASE00 )   /* second SCSP's RAM */
ROM_END

GAME( 1995, coolridr, 0, coolridr, coolridr, coolridr, ROT0, "Sega", "Cool Riders", GAME_IMPERFECT_SOUND )
