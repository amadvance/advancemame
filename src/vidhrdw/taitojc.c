// license:BSD-3-Clause
// copyright-holders:Ville Linde, Angelo Salese, hap
/* Taito JC video hardware. */
#include "driver.h"
#include "vidhrdw/poly.h"
#include "taitojc.h"

typedef struct _poly_extra_data poly_extra_data;
struct _poly_extra_data
{
	mame_bitmap *zbuffer;
	UINT8 *texture;

	int tex_base_x;
	int tex_base_y;
	int tex_wrap_x;
	int tex_wrap_y;
};

#define JC_BITMAP16(b,y,x) ((UINT16 *)((mame_bitmap *)(b))->line[y] + (x))
typedef struct { float x, y, p[4]; } jc_vertex;
typedef struct { int startx, stopx; struct { float start, dpdx; } param[4]; } poly_extent;
static poly_extra_data jc_extra;
typedef void (*jc_scan_callback)(void *, INT32, const poly_extent *, const void *, int);

static void jc_render_triangle(mame_bitmap *dest, const rectangle *clip, jc_scan_callback scan, int params,
	const jc_vertex *a, const jc_vertex *b, const jc_vertex *c)
{
	struct poly_vertex vertices[3];
	const jc_vertex *src[3] = { a, b, c };
	const struct poly_scanline_data *lines;
	int i, j, y;
	for (i = 0; i < 3; i++) {
		vertices[i].x = src[i]->x;
		vertices[i].y = src[i]->y;
		for (j = 0; j < params; j++) vertices[i].p[j] = src[i]->p[j];
	}
	lines = params == 4 ? setup_triangle_3(&vertices[0], &vertices[1], &vertices[2], clip)
		: setup_triangle_1(&vertices[0], &vertices[1], &vertices[2], clip);
	if (!lines) return;
	for (y = lines->sy; y <= lines->ey; y++) {
		const struct poly_scanline *line = &lines->scanline[y - lines->sy];
		poly_extent extent;
		extent.startx = line->sx;
		extent.stopx = line->ex + 1;
		for (j = 0; j < params; j++) {
			extent.param[j].start = line->p[j] / 65536.0;
			extent.param[j].dpdx = lines->dp[j] / 65536.0;
		}
		scan(dest, y, &extent, &jc_extra, 0);
	}
}

static void jc_render_quad(mame_bitmap *dest, const rectangle *clip, jc_scan_callback scan, int params,
	const jc_vertex *a, const jc_vertex *b, const jc_vertex *c, const jc_vertex *d)
{
	/* Render synchronously: the texture and depth buffers may change at the next DSP command. */
	jc_render_triangle(dest, clip, scan, params, a, b, c);
	jc_render_triangle(dest, clip, scan, params, a, c, d);
}
#define TAITOJC_NUM_TILES		0x80
static UINT8 jc_char_dirty[TAITOJC_NUM_TILES];

static const gfx_layout taitojc_char_layout =
{
	16,16,
	TAITOJC_NUM_TILES,
	4,
	{ 0,1,2,3 },
	{ 24,28,16,20,8,12,0,4, 56,60,48,52,40,44,32,36 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};

static void taitojc_tile_info(int tile_index)
{

	UINT32 val = jc.m_tile_ram[tile_index];
	int color = (val >> 22) & 0xff;
	int tile = (val >> 2) & 0x7f;
	SET_TILE_INFO(jc.m_gfx_index, tile, color, 0);
}

READ32_HANDLER(taitojc_tile_r)
{

	return jc.m_tile_ram[offset];
}

READ32_HANDLER(taitojc_char_r)
{

	return jc.m_char_ram[offset];
}

WRITE32_HANDLER(taitojc_tile_w)
{

	COMBINE_DATA(jc.m_tile_ram + offset);
	tilemap_mark_tile_dirty(jc.m_tilemap, offset);
}

WRITE32_HANDLER(taitojc_char_w)
{

	COMBINE_DATA(jc.m_char_ram + offset);
	jc_char_dirty[offset/32] = 1;
}

static void draw_object(mame_bitmap *bitmap, const rectangle *cliprect, UINT32 w1, UINT32 w2)
{

	int x, y, width, height, palette;
	int i, j;
	int x1, x2, y1, y2;
	int ix, iy;
	UINT32 address;
	UINT8 *v;
	UINT8 color_depth;

	color_depth = (w2 & 0x10000) >> 16;

	address		= (w2 & 0x7fff) * 0x20;
	if (w2 & 0x4000)
		address |= 0x40000;

	x			= ((w1 >>  0) & 0x3ff);
	if (x & 0x200)
		x |= ~0x1ff;

	y			= ((w1 >> 16) & 0x3ff);
	if (y & 0x200)
		y |= ~0x1ff;

	width		= ((w1 >> 10) & 0x3f) * 16;
	height		= ((w1 >> 26) & 0x3f) * 16;
	palette		= ((w2 >> 22) & 0x7f) << 8;

	v = (UINT8*)&jc.m_vram[address/4];

	if (address >= 0xf8000 || width == 0 || height == 0)
		return;

	x1 = x;
	x2 = x + width;
	y1 = y;
	y2 = y + height;

	if (x1 > cliprect->max_x || x2 < cliprect->min_x || y1 > cliprect->max_y || y2 < cliprect->min_y)
	{
		return;
	}

	ix = 0;
	iy = 0;

	if (x1 < cliprect->min_x)
	{
		ix = abs(cliprect->min_x - x1);
		x1 = cliprect->min_x;
	}
	if (x2 > cliprect->max_x)
	{
		x2 = cliprect->max_x;
	}
	if (y1 < cliprect->min_y)
	{
		iy = abs(cliprect->min_y - y1);
		y1 = cliprect->min_y;
	}
	if (y2 > cliprect->max_y)
	{
		y2 = cliprect->max_y;
	}

	if(!color_depth)
	{
		for (j=y1; j < y2; j++)
		{
			UINT16 *d = JC_BITMAP16(bitmap, j,  0);
			int index = (iy * (width / 2)) + ix;

			for (i=x1; i < x2; i+=2)
			{
				UINT8 pen = (v[BYTE4_XOR_BE(index)] & 0xf0) >> 4;
				if (pen != 0)
				{
					d[i] = palette + pen;
				}
				pen = (v[BYTE4_XOR_BE(index)] & 0x0f);
				if (pen != 0)
				{
					d[i+1] = palette + pen;
				}
				index++;
			}

			iy++;
		}
	}
	else
	{
		for (j=y1; j < y2; j++)
		{
			UINT16 *d = JC_BITMAP16(bitmap, j,  0);
			int index = (iy * width) + ix;

			for (i=x1; i < x2; i++)
			{
				UINT8 pen = v[BYTE4_XOR_BE(index)];
				if (pen != 0)
				{
					d[i] = palette + pen;
				}
				index++;
			}

			iy++;
		}
	}
}

VIDEO_START( taitojc )
{
	for (jc.m_gfx_index = 0; jc.m_gfx_index < MAX_GFX_ELEMENTS; jc.m_gfx_index++)
		if (!Machine->gfx[jc.m_gfx_index]) break;
	jc.m_tilemap = tilemap_create(taitojc_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16,64,64);
	tilemap_set_transparent_pen(jc.m_tilemap, 0);
	jc.m_char_ram = auto_malloc(0x4000);
	jc.m_tile_ram = auto_malloc(0x4000);
	memset(jc.m_char_ram, 0, 0x4000);
	memset(jc.m_tile_ram, 0, 0x4000);
	memset(jc_char_dirty, 1, sizeof(jc_char_dirty));
	Machine->gfx[jc.m_gfx_index] = allocgfx(&taitojc_char_layout);
	Machine->gfx[jc.m_gfx_index]->colortable = Machine->pens;
	Machine->gfx[jc.m_gfx_index]->total_colors = 32768 / 16;
	jc.m_texture = auto_malloc(0x400000);
	memset(jc.m_texture, 0, 0x400000);
	jc.m_framebuffer = auto_bitmap_alloc_depth(512,400,16);
	jc.m_zbuffer = auto_bitmap_alloc_depth(512,400,16);
	taitojc_clear_frame();
	return 0;
}

VIDEO_UPDATE( taitojc )
{

	int i;
	UINT16 start_offs;

	fillbitmap(bitmap, 0, cliprect);

	start_offs = jc.m_objlist[0xfc4/4] & 0x2000 ? (0x800/4) : (0x400/4);

	if((jc.m_objlist[0xfc4/4] & 0x0000ffff) != 0x0000 && (jc.m_objlist[0xfc4/4] & 0x0000ffff) != 0x2000  && (jc.m_objlist[0xfc4/4] & 0x0000ffff) != 0xf000 )
		logerror("%08x, unknown object flags",jc.m_objlist[0xfc4/4]);

	for (i=start_offs-2; i >= (start_offs-0x400/4); i-=2)
	{
		UINT32 w1 = jc.m_objlist[i + 0];
		UINT32 w2 = jc.m_objlist[i + 1];

		if ((w2 & 0x200000) == 0)
		{
			draw_object(bitmap, cliprect, w1, w2);
		}
	}

	copybitmap(bitmap, jc.m_framebuffer, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);

	for (i=start_offs-2; i >= (start_offs-0x400/4); i-=2)
	{
		UINT32 w1 = jc.m_objlist[i + 0];
		UINT32 w2 = jc.m_objlist[i + 1];

		if ((w2 & 0x200000) != 0)
		{
			draw_object(bitmap, cliprect, w1, w2);
		}
	}

	{
		int i, dirty = 0;
		for (i = 0; i < TAITOJC_NUM_TILES; i++)
			if (jc_char_dirty[i]) {
				decodechar(Machine->gfx[jc.m_gfx_index], i, (UINT8 *)jc.m_char_ram, &taitojc_char_layout);
				jc_char_dirty[i] = 0;
				dirty = 1;
			}
		if (dirty) tilemap_mark_all_tiles_dirty(jc.m_tilemap);
	}
	tilemap_draw(bitmap, cliprect, jc.m_tilemap, 0,0);

}

static void render_solid_scan(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	const poly_extra_data *extra = (const poly_extra_data *)extradata;
	mame_bitmap *destmap = (mame_bitmap *)dest;
	float z = extent->param[0].start;
	int color = extent->param[1].start;
	float dz = extent->param[0].dpdx;
	UINT16 *fb = JC_BITMAP16(destmap, scanline, 0);
	UINT16 *zb;
	int x;

	zb = JC_BITMAP16(extra->zbuffer, scanline, 0);

	for (x = extent->startx; x < extent->stopx; x++)
	{
		int iz = (int)z & 0xffff;

		if (iz <= zb[x])
		{
			fb[x] = color;
			zb[x] = iz;
		}

		z += dz;
	}
}

static void render_shade_scan(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	const poly_extra_data *extra = (const poly_extra_data *)extradata;
	mame_bitmap *destmap = (mame_bitmap *)dest;
	float z = extent->param[0].start;
	float color = extent->param[1].start;
	float dz = extent->param[0].dpdx;
	float dcolor = extent->param[1].dpdx;
	UINT16 *fb = JC_BITMAP16(destmap, scanline, 0);
	UINT16 *zb;
	int x;

	zb = JC_BITMAP16(extra->zbuffer, scanline, 0);

	for (x = extent->startx; x < extent->stopx; x++)
	{
		int ic = (int)color & 0xffff;
		int iz = (int)z & 0xffff;

		if (iz <= zb[x])
		{
			fb[x] = ic;
			zb[x] = iz;
		}

		color += dcolor;
		z += dz;
	}
}

static void render_texture_scan(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	const poly_extra_data *extra = (const poly_extra_data *)extradata;
	mame_bitmap *destmap = (mame_bitmap *)dest;
	float z = extent->param[0].start;
	float u = extent->param[1].start;
	float v = extent->param[2].start;
	float color = extent->param[3].start;
	float dz = extent->param[0].dpdx;
	float du = extent->param[1].dpdx;
	float dv = extent->param[2].dpdx;
	float dcolor = extent->param[3].dpdx;
	UINT16 *fb = JC_BITMAP16(destmap, scanline, 0);
	UINT16 *zb = JC_BITMAP16(extra->zbuffer, scanline, 0);
	int tex_wrap_x = extra->tex_wrap_x;
	int tex_wrap_y = extra->tex_wrap_y;
	int tex_base_x = extra->tex_base_x;
	int tex_base_y = extra->tex_base_y;
	int x;

	for (x = extent->startx; x < extent->stopx; x++)
	{
		int iu, iv;
		UINT8 texel;
		int palette = ((int)color & 0x7f) << 8;
		int iz = (int)z & 0xffff;

		if (!tex_wrap_x)
		{
			iu = ((int)u >> 4) & 0x7ff;
		}
		else
		{
			iu = (tex_base_x + (((int)u >> 4) & 0x3f)) & 0x7ff;
		}

		if (!tex_wrap_y)
		{
			iv = ((int)v >> 4) & 0x7ff;
		}
		else
		{
			iv = (tex_base_y + (((int)v >> 4) & 0x3f)) & 0x7ff;
		}

		texel = extra->texture[(iv * 2048) + iu];

		if (iz <= zb[x] && texel != 0)
		{
			fb[x] = palette | texel;
			zb[x] = iz;
		}

		u += du;
		v += dv;
		color += dcolor;
		z += dz;
	}
}

void taitojc_render_polygons(UINT16 *polygon_fifo, int length)
{
	/* Solid polygons also need a valid depth buffer before any texture command. */
	jc_extra.zbuffer = jc.m_zbuffer;
	jc_extra.texture = jc.m_texture;

	jc_vertex vert[4];
	int i;
	int ptr;

	ptr = 0;
	while (ptr < length)
	{
		UINT16 cmd = polygon_fifo[ptr++];

		switch (cmd & 0x7)
		{
			case 0x03:
			{

				poly_extra_data *extra = (poly_extra_data *)&jc_extra;
				UINT16 texbase;

				texbase = polygon_fifo[ptr++];

				extra->zbuffer = jc.m_zbuffer;
				extra->texture = jc.m_texture;
				extra->tex_base_x = ((texbase >> 0) & 0xff) << 4;
				extra->tex_base_y = ((texbase >> 8) & 0xff) << 4;

				extra->tex_wrap_x = (cmd & 0xc0) ? 1 : 0;
				extra->tex_wrap_y = (cmd & 0x30) ? 1 : 0;

				for (i=0; i < 3; i++)
				{
					vert[i].p[3] = polygon_fifo[ptr++];
					vert[i].p[2] = (UINT16)(polygon_fifo[ptr++]);
					vert[i].p[1] = (UINT16)(polygon_fifo[ptr++]);
					vert[i].y =  (INT16)(polygon_fifo[ptr++]);
					vert[i].x =  (INT16)(polygon_fifo[ptr++]);
					vert[i].p[0] = (UINT16)(polygon_fifo[ptr++]);
				}

				if (vert[0].p[0] < 0x8000 && vert[1].p[0] < 0x8000 && vert[2].p[0] < 0x8000)
				{
					jc_render_triangle( jc.m_framebuffer, &Machine->visible_area, render_texture_scan, 4, &vert[0], &vert[1], &vert[2]);
				}
				break;
			}
			case 0x04:
			{

				for (i=0; i < 4; i++)
				{
					vert[i].p[1] = polygon_fifo[ptr++];
					vert[i].y =  (INT16)(polygon_fifo[ptr++]);
					vert[i].x =  (INT16)(polygon_fifo[ptr++]);
					vert[i].p[0] = (UINT16)(polygon_fifo[ptr++]);
				}

				if (vert[0].p[0] < 0x8000 && vert[1].p[0] < 0x8000 && vert[2].p[0] < 0x8000 && vert[3].p[0] < 0x8000)
				{
					if (vert[0].p[1] == vert[1].p[1] &&
						vert[1].p[1] == vert[2].p[1] &&
						vert[2].p[1] == vert[3].p[1])
					{

						jc_render_quad( jc.m_framebuffer, &Machine->visible_area, render_solid_scan, 2, &vert[0], &vert[1], &vert[2], &vert[3]);
					}
					else
					{
						jc_render_quad( jc.m_framebuffer, &Machine->visible_area, render_shade_scan, 2, &vert[0], &vert[1], &vert[2], &vert[3]);
					}
				}
				break;
			}
			case 0x06:
			{

				poly_extra_data *extra = (poly_extra_data *)&jc_extra;
				UINT16 texbase;

				texbase = polygon_fifo[ptr++];

				extra->zbuffer = jc.m_zbuffer;
				extra->texture = jc.m_texture;
				extra->tex_base_x = ((texbase >> 0) & 0xff) << 4;
				extra->tex_base_y = ((texbase >> 8) & 0xff) << 4;

				extra->tex_wrap_x = (cmd & 0xc0) ? 1 : 0;
				extra->tex_wrap_y = (cmd & 0x30) ? 1 : 0;

				for (i=0; i < 4; i++)
				{
					vert[i].p[3] = polygon_fifo[ptr++];
					vert[i].p[2] = (UINT16)(polygon_fifo[ptr++]);
					vert[i].p[1] = (UINT16)(polygon_fifo[ptr++]);
					vert[i].y =  (INT16)(polygon_fifo[ptr++]);
					vert[i].x =  (INT16)(polygon_fifo[ptr++]);
					vert[i].p[0] = (UINT16)(polygon_fifo[ptr++]);
				}

				if (vert[0].p[0] < 0x8000 && vert[1].p[0] < 0x8000 && vert[2].p[0] < 0x8000 && vert[3].p[0] < 0x8000)
				{
					jc_render_quad( jc.m_framebuffer, &Machine->visible_area, render_texture_scan, 4, &vert[0], &vert[1], &vert[2], &vert[3]);
				}
				break;
			}
			case 0x00:
			{
				ptr += 6;
				break;
			}
			default:
			{

			}
		}
	}

}

void taitojc_clear_frame(void)
{

	rectangle cliprect;

	cliprect.min_x = 0;
	cliprect.min_y = 0;
	cliprect.max_x = 512 - 1;
	cliprect.max_y = 400 - 1;

	fillbitmap(jc.m_framebuffer, 0, &cliprect);
	fillbitmap(jc.m_zbuffer, 0xffff, &cliprect);
}
