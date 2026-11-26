// license:BSD-3-Clause
// copyright-holders:David Haywood, Ville Linde
/***************************************************************************

    Konami K001005 Polygon Renderer (KS10071)
    Konami K001006 Texel Unit (KS10081)

***************************************************************************/

/*
    Differences from MAME:

    The polygon rasterizer is the legacy poly manager (polylgcy.c) and the
    colors are handled as packed ARGB integers. The K001006 palette is kept
    by the chip and the 3D framebuffer is stored in this file.

    The polygons are not rendered at every sync command of the FIFO, but they
    are accumulated until the buffer swap, or until a change of the K001006
    state read by the rasterizer, to render them in fewer and larger batches.
*/

#include "k001005.h"
#include "cpu/sharc/sharc.h"
#include "polylgcy.h"

#define POLY_Z		0
#define POLY_FOG	1
#define POLY_DIFF	2
#define POLY_U		3
#define POLY_V		4
#define POLY_W		5
#define POLY_R		6
#define POLY_G		7
#define POLY_B		8
#define POLY_A		9

#define FIFO_SIZE	0x10000

typedef struct
{
	int texture_x, texture_y;
	int texture_width, texture_height;
	int texture_page;
	int texture_palette;
	int texture_mirror;
	UINT32 poly_color;
	UINT32 diffuse_light;
	UINT32 ambient_light;
	UINT32 fog_color;
	UINT32 cmd;
	int fog_enable;
} k001005_polydata;

/* K001006 */
static UINT16 *k001006_pal_ram[K001006_MAX_CHIPS];
static UINT16 *k001006_unknown_ram[K001006_MAX_CHIPS];
static UINT32 k001006_pens[K001006_MAX_CHIPS][0x800];
static UINT32 k001006_addr[K001006_MAX_CHIPS];
static int k001006_device_sel[K001006_MAX_CHIPS];
static int k001006_enable_bilinear[K001006_MAX_CHIPS];
static UINT8 *k001006_texrom;
static UINT32 k001006_texrom_size;

/* K001005 */
static poly_manager *poly;
static int k001005_width, k001005_height;
static UINT32 *k001005_fb[2];
static float *k001005_zb;
static int k001005_fb_page;
static rectangle k001005_cliprect;
static UINT16 *k001005_ram[2];
static UINT32 *k001005_fifo;
static UINT32 k001005_status;
static int k001005_ram_ptr;
static int k001005_fifo_read_ptr;
static int k001005_fifo_write_ptr;
static UINT32 *k001005_3dfifo;
static int k001005_3dfifo_ptr;
static poly_vertex k001005_vertexb[4];
static int k001005_vertexb_ptr;
static int *k001005_tex_mirror_table[2][8];

static UINT32 k001005_light_r, k001005_light_g, k001005_light_b;
static UINT32 k001005_ambient_r, k001005_ambient_g, k001005_ambient_b;
static UINT32 k001005_fog_r, k001005_fog_g, k001005_fog_b;
static float k001005_fog_start_z, k001005_fog_end_z;
static UINT16 k001005_reg_fog_start;
static INT16 k001005_viewport_min_x, k001005_viewport_max_x;
static INT16 k001005_viewport_min_y, k001005_viewport_max_y;
static INT16 k001005_viewport_center_x, k001005_viewport_center_y;

#define CH_A(c) ((c) >> 24)
#define CH_R(c) (((c) >> 16) & 0xff)
#define CH_G(c) (((c) >> 8) & 0xff)
#define CH_B(c) ((c) & 0xff)

/* (c * factor + o * (256 - factor)) / 256 on every channel */
INLINE UINT32 blend_argb(UINT32 c, UINT32 o, int factor)
{
	int rest = 256 - factor;
	return MAKE_ARGB(
			(CH_A(c) * factor + CH_A(o) * rest) >> 8,
			(CH_R(c) * factor + CH_R(o) * rest) >> 8,
			(CH_G(c) * factor + CH_G(o) * rest) >> 8,
			(CH_B(c) * factor + CH_B(o) * rest) >> 8);
}

/* (c * scale) / 256 on every channel */
INLINE UINT32 scale_argb(UINT32 c, UINT32 scale)
{
	return MAKE_ARGB(
			(CH_A(c) * CH_A(scale)) >> 8,
			(CH_R(c) * CH_R(scale)) >> 8,
			(CH_G(c) * CH_G(scale)) >> 8,
			(CH_B(c) * CH_B(scale)) >> 8);
}

INLINE int clamp255(int v)
{
	return (v < 0) ? 0 : (v > 255) ? 255 : v;
}

static UINT32 bilinear_filter(UINT32 rgb00, UINT32 rgb01, UINT32 rgb10, UINT32 rgb11, int u, int v)
{
	UINT32 ag00 = ((rgb00 >> 8) & 0x00ff00ff) * (256 - u);
	UINT32 rb00 = ((rgb00 >> 0) & 0x00ff00ff) * (256 - u);
	UINT32 ag10 = ((rgb10 >> 8) & 0x00ff00ff) * (256 - u);
	UINT32 rb10 = ((rgb10 >> 0) & 0x00ff00ff) * (256 - u);
	UINT32 ag01 = ((rgb01 >> 8) & 0x00ff00ff) * u;
	UINT32 rb01 = ((rgb01 >> 0) & 0x00ff00ff) * u;
	UINT32 ag11 = ((rgb11 >> 8) & 0x00ff00ff) * u;
	UINT32 rb11 = ((rgb11 >> 0) & 0x00ff00ff) * u;
	UINT32 ag0x = ((ag00 >> 1) & 0x7fff7fff) + ((ag01 >> 1) & 0x7fff7fff);
	UINT32 rb0x = ((rb00 >> 1) & 0x7fff7fff) + ((rb01 >> 1) & 0x7fff7fff);
	UINT32 ag1x = ((ag10 >> 1) & 0x7fff7fff) + ((ag11 >> 1) & 0x7fff7fff);
	UINT32 rb1x = ((rb10 >> 1) & 0x7fff7fff) + ((rb11 >> 1) & 0x7fff7fff);
	UINT32 a = (((ag0x >> 16) * (256 - v)) + ((ag1x >> 16) * v)) >> 15;
	UINT32 r = (((rb0x >> 16) * (256 - v)) + ((rb1x >> 16) * v)) >> 15;
	UINT32 g = (((ag0x & 0xffff) * (256 - v)) + ((ag1x & 0xffff) * v)) >> 15;
	UINT32 b = (((rb0x & 0xffff) * (256 - v)) + ((rb1x & 0xffff) * v)) >> 15;

	return (a << 24) | (r << 16) | (g << 8) | b;
}

/***************************************************************************
    K001006
***************************************************************************/

static void k001006_preprocess_texture_data(UINT8 *dst, const UINT8 *src, UINT32 length)
{
	static const int decode_x[8] = {  0, 16, 2, 18, 4, 20, 6, 22 };
	static const int decode_y[16] = {  0, 8, 32, 40, 1, 9, 33, 41, 64, 72, 96, 104, 65, 73, 97, 105 };
	UINT8 temp[0x40000];
	UINT32 index;

	for (index = 0; index < length; index += 0x40000)
	{
		UINT32 offset = index;
		int i;

		memset(temp, 0, 0x40000);

		for (i = 0; i < 0x800; i++)
		{
			int tx = ((i & 0x400) >> 5) | ((i & 0x100) >> 4) | ((i & 0x40) >> 3) | ((i & 0x10) >> 2) | ((i & 0x4) >> 1) | (i & 0x1);
			int ty = ((i & 0x200) >> 5) | ((i & 0x80) >> 4) | ((i & 0x20) >> 3) | ((i & 0x8) >> 2) | ((i & 0x2) >> 1);
			int x, y;

			tx <<= 3;
			ty <<= 4;

			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 8; x++)
				{
					temp[((ty + y) << 9) + (tx + x)] = src[offset + decode_y[y] + decode_x[x]];
				}
			}
			offset += 128;
		}
		memcpy(&dst[index], temp, 0x40000);
	}
}

int k001006_start(void)
{
	int chip;

	k001006_texrom_size = memory_region_length(REGION_GFX1);
	if (k001006_texrom_size < 0x40000 || (k001006_texrom_size & 0x3ffff))
		return 1;

	for (chip = 0; chip < K001006_MAX_CHIPS; ++chip)
	{
		k001006_pal_ram[chip] = auto_malloc(0x800 * sizeof(UINT16));
		k001006_unknown_ram[chip] = auto_malloc(0x1000 * sizeof(UINT16));
		memset(k001006_pal_ram[chip], 0, 0x800 * sizeof(UINT16));
		memset(k001006_unknown_ram[chip], 0, 0x1000 * sizeof(UINT16));
		memset(k001006_pens[chip], 0, sizeof(k001006_pens[chip]));
		k001006_addr[chip] = 0;
		k001006_device_sel[chip] = 0;
		k001006_enable_bilinear[chip] = 0;
	}

	k001006_texrom = auto_malloc(k001006_texrom_size);
	k001006_preprocess_texture_data(k001006_texrom, memory_region(REGION_GFX1), k001006_texrom_size);

	return 0;
}

static UINT32 k001006_r(int chip, int offset)
{
	if (offset == 1)
	{
		switch (k001006_device_sel[chip])
		{
			case 0x0b:		/* CG Board ROM read */
			{
				const UINT16 *rom = (const UINT16 *)memory_region(REGION_GFX1);
				return rom[k001006_addr[chip] / 2] << 16;
			}
			case 0x0d:		/* Palette RAM read */
			{
				UINT32 addr = k001006_addr[chip];
				k001006_addr[chip] += 2;
				return k001006_pal_ram[chip][(addr >> 1) & 0x7ff];
			}
			case 0x0f:		/* Unknown RAM read */
			{
				UINT32 addr = k001006_addr[chip]++;
				return k001006_unknown_ram[chip][addr & 0xfff];
			}
			default:
			{
				fatalerror("K001006_r chip %d, unknown device %02X", chip, k001006_device_sel[chip]);
			}
		}
	}
	return 0;
}

static void k001006_w(int chip, int offset, UINT32 data, UINT32 mem_mask)
{
	if (offset == 0)
	{
		COMBINE_DATA(&k001006_addr[chip]);
	}
	else if (offset == 1)
	{
		switch (k001006_device_sel[chip])
		{
			case 0xd:	/* Palette RAM write */
			{
				/* the queued polygons are rendered with the current palette */
				if (poly)
					poly_wait(poly, "palette");

				UINT32 index = (k001006_addr[chip] >> 1) & 0x7ff;
				int a = (data & 0x8000) ? 0x00 : 0xff;
				int b = ((data >> 10) & 0x1f) << 3;
				int g = ((data >> 5) & 0x1f) << 3;
				int r = ((data >> 0) & 0x1f) << 3;

				k001006_pal_ram[chip][index] = data & 0xffff;

				b |= (b >> 5);
				g |= (g >> 5);
				r |= (r >> 5);
				k001006_pens[chip][index] = MAKE_ARGB(a, r, g, b);

				k001006_addr[chip] += 2;
				break;
			}
			case 0xf:	/* Unknown RAM write */
			{
				k001006_unknown_ram[chip][k001006_addr[chip]++ & 0xfff] = data & 0xffff;
				break;
			}
			default:
			{
				k001006_addr[chip]++;
				break;
			}
		}
	}
	else if (offset == 2)
	{
		if (!(mem_mask & 0xffff0000))
		{
			k001006_device_sel[chip] = (data >> 16) & 0xf;

			/* the queued polygons are rendered with the current filter */
			if (poly && k001006_enable_bilinear[chip] != ((data >> 20) & 1))
				poly_wait(poly, "bilinear");
			k001006_enable_bilinear[chip] = (data >> 20) & 1;
		}
	}
}

READ32_HANDLER( k001006_0_r )
{
	return k001006_r(0, offset);
}

WRITE32_HANDLER( k001006_0_w )
{
	k001006_w(0, offset, data, mem_mask);
}

READ32_HANDLER( k001006_1_r )
{
	return k001006_r(1, offset);
}

WRITE32_HANDLER( k001006_1_w )
{
	k001006_w(1, offset, data, mem_mask);
}

INLINE UINT32 k001006_fetch_texel(int page, int pal_index, int u, int v)
{
	UINT8 texel = k001006_texrom[page + (((v & 0x1ff) << 9) | (u & 0x1ff))];
	return (texel == 0) ? 0 : k001006_pens[0][pal_index + texel];
}

/***************************************************************************
    K001005 renderer
***************************************************************************/

static void k001005_swap_buffers(void)
{
	int i;
	float zvalue = 10000000000.0F;

	k001005_fb_page ^= 1;

	memset(k001005_fb[k001005_fb_page], 0, k001005_width * k001005_height * sizeof(UINT32));
	for (i = 0; i < k001005_width * k001005_height; ++i)
		k001005_zb[i] = zvalue;
}

INLINE void draw_scanline_generic(INT32 scanline, const poly_extent *extent, const k001005_polydata *extra, int use_texture, int use_vertex_color)
{
	UINT32 *fb = k001005_fb[k001005_fb_page] + scanline * k001005_width;
	float *zb = k001005_zb + scanline * k001005_width;
	float z = extent->param[POLY_Z].start;
	float dz = extent->param[POLY_Z].dpdx;
	float diff = extent->param[POLY_DIFF].start;
	float ddiff = extent->param[POLY_DIFF].dpdx;
	float fog = extent->param[POLY_FOG].start;
	float dfog = extent->param[POLY_FOG].dpdx;
	float u = 0, v = 0, w = 0, du = 0, dv = 0, dw = 0;
	float r = 0, g = 0, b = 0, a = 0, dr = 0, dg = 0, db = 0, da = 0;
	UINT32 ambient_color = extra->ambient_light;
	UINT32 fog_color = extra->fog_color;
	int poly_color_a = CH_A(extra->poly_color);
	int texture_x = extra->texture_x * 8;
	int texture_y = extra->texture_y * 8;
	int tex_page = extra->texture_page * 0x40000;
	int palette_index = extra->texture_palette * 256;
	const int *x_mirror_table = k001005_tex_mirror_table[extra->texture_mirror ? 1 : 0][extra->texture_width];
	const int *y_mirror_table = k001005_tex_mirror_table[extra->texture_mirror ? 1 : 0][extra->texture_height];
	int use_z_compare = (extra->cmd >> 2) & 1;
	int use_fb_blend = !((extra->cmd >> 1) & 1);
	int use_fog = extra->fog_enable;
	int use_bilinear = k001006_enable_bilinear[0];
	UINT32 texel = 0;
	UINT32 texel_alpha = 0;
	int x;

	if (use_texture)
	{
		u = extent->param[POLY_U].start;
		v = extent->param[POLY_V].start;
		w = extent->param[POLY_W].start;
		du = extent->param[POLY_U].dpdx;
		dv = extent->param[POLY_V].dpdx;
		dw = extent->param[POLY_W].dpdx;
	}

	if (use_vertex_color)
	{
		r = extent->param[POLY_R].start;
		dr = extent->param[POLY_R].dpdx;
		g = extent->param[POLY_G].start;
		dg = extent->param[POLY_G].dpdx;
		b = extent->param[POLY_B].start;
		db = extent->param[POLY_B].dpdx;
		a = extent->param[POLY_A].start;
		da = extent->param[POLY_A].dpdx;
	}

	for (x = extent->startx; x < extent->stopx; x++)
	{
		if (z <= zb[x] || !use_z_compare)
		{
			int idiff, ifog;
			UINT32 light_color;

			if (use_texture)
			{
				float oow = 1.0F / w;
				int iu = (int)(u * oow);
				int iv = (int)(v * oow);

				if (!use_bilinear)
				{
					int texel_u = texture_x + x_mirror_table[(iu >> 4) & 0x7f];
					int texel_v = texture_y + y_mirror_table[(iv >> 4) & 0x7f];

					texel = k001006_fetch_texel(tex_page, palette_index, texel_u, texel_v);
					texel_alpha = texel >> 24;
				}
				else
				{
					int ufrac, vfrac, texel_u0, texel_u1, texel_v0, texel_v1;
					UINT32 tex00, tex01, tex10, tex11;

					/* sub-texel bias to avoid seams */
					iu -= 7;
					iv -= 7;

					ufrac = iu & 0xf;
					vfrac = iv & 0xf;
					texel_u0 = texture_x + x_mirror_table[(iu >> 4) & 0x7f];
					texel_u1 = texture_x + x_mirror_table[((iu >> 4) + 1) & 0x7f];
					texel_v0 = texture_y + y_mirror_table[(iv >> 4) & 0x7f];
					texel_v1 = texture_y + y_mirror_table[((iv >> 4) + 1) & 0x7f];

					tex00 = k001006_fetch_texel(tex_page, palette_index, texel_u0, texel_v0);
					tex01 = k001006_fetch_texel(tex_page, palette_index, texel_u1, texel_v0);
					tex10 = k001006_fetch_texel(tex_page, palette_index, texel_u0, texel_v1);
					tex11 = k001006_fetch_texel(tex_page, palette_index, texel_u1, texel_v1);

					texel = bilinear_filter(tex00, tex01, tex10, tex11, ufrac * 16, vfrac * 16);
					texel_alpha = tex00 >> 24;
				}
			}

			idiff = clamp255((int)diff);
			ifog = clamp255((int)fog);

			{
				UINT32 d = extra->diffuse_light;
				int lr = clamp255(((CH_R(d) * idiff) >> 8) + CH_R(ambient_color));
				int lg = clamp255(((CH_G(d) * idiff) >> 8) + CH_G(ambient_color));
				int lb = clamp255(((CH_B(d) * idiff) >> 8) + CH_B(ambient_color));
				int la = clamp255(((CH_A(d) * idiff) >> 8) + CH_A(ambient_color));
				light_color = MAKE_ARGB(la, lr, lg, lb);
			}

			if (use_vertex_color)
			{
				int ir = clamp255((int)r);
				int ig = clamp255((int)g);
				int ib = clamp255((int)b);
				int ia = clamp255((int)a);

				if (ia != 0)
				{
					UINT32 color = MAKE_ARGB(ia, ir, ig, ib);

					if (use_texture && texel_alpha != 0)
						color = texel;

					color = scale_argb(color, light_color);

					if (use_fog)
						color = blend_argb(color, fog_color, ifog);

					/* framebuffer blend */
					if (use_fb_blend)
						color = blend_argb(color, fb[x], ia);

					fb[x] = color;
					zb[x] = z;
				}
			}
			else
			{
				if (use_texture)
				{
					if (texel_alpha != 0)
					{
						UINT32 color = scale_argb(texel, light_color);

						if (use_bilinear && texel_alpha < 0xff)
							color = blend_argb(color, fb[x], texel_alpha);

						if (use_fog)
							color = blend_argb(color, fog_color, ifog);

						if (use_fb_blend)
							color = blend_argb(color, fb[x], poly_color_a);

						fb[x] = color;
						zb[x] = z;
					}
				}
				else
				{
					UINT32 color = scale_argb(extra->poly_color, light_color);

					if (use_fog)
						color = blend_argb(color, fog_color, ifog);

					/* framebuffer blend */
					if (use_fb_blend)
						color = blend_argb(color, fb[x], poly_color_a);

					fb[x] = color;
					zb[x] = z;
				}
			}
		}

		z += dz;
		diff += ddiff;
		fog += dfog;

		if (use_vertex_color)
		{
			r += dr;
			g += dg;
			b += db;
			a += da;
		}

		if (use_texture)
		{
			u += du;
			v += dv;
			w += dw;
		}
	}
}

static void draw_scanline_tex(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	draw_scanline_generic(scanline, extent, (const k001005_polydata *)extradata, 1, 0);
}

static void draw_scanline_vertex_color(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	draw_scanline_generic(scanline, extent, (const k001005_polydata *)extradata, 0, 1);
}

static void draw_scanline_vertex_color_tex(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	draw_scanline_generic(scanline, extent, (const k001005_polydata *)extradata, 1, 1);
}

static void draw_scanline_color(void *dest, INT32 scanline, const poly_extent *extent, const void *extradata, int threadid)
{
	draw_scanline_generic(scanline, extent, (const k001005_polydata *)extradata, 0, 0);
}

INLINE int clamp_int(int v, int lo, int hi)
{
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

/*
    Command
    0x00: xxxxxxxx xxxxxxxx xxxxxxx- --------    0x80000000 (exact number of bits unknown)
    0x00: -------- -------- -------x --------    0 = per-poly color, 1 = per-vertex color
    0x00: -------- -------- -------- x-------    ? Texture related
    0x00: -------- -------- -------- -x------    Unused?
    0x00: -------- -------- -------- --x-----    1 = enable smooth shading?
    0x00: -------- -------- -------- ---x----    1 = texture mirroring
    0x00: -------- -------- -------- ----x---    ? Texture related
    0x00: -------- -------- -------- -----x--    1 = enable Z-buffer read
    0x00: -------- -------- -------- ------x-    0 = blend enabled, 1 = disabled
    0x00: -------- -------- -------- -------x    0 = per-vertex Z, 1 = per-poly Z (0x80000121 seems like an exception)

    Texture header
    0x01: -xxx---- -------- -------- --------    Texture palette
    0x01: ----xx-- -------- -------- --------    Unknown flags, set by commands 0x7b...0x7e. Used mostly on polygons further away from camera. Some kind of depth-based effect?
    0x01: ------xx x------- -------- --------    Texture width / 8 - 1
    0x01: -------- -xxx---- -------- --------    Texture height / 8 - 1
    0x01: -------- -------x xxxx---- --------    Texture page
    0x01: -------- -------- ----x-x- x-x-x-x-    Texture X / 8
    0x01: -------- -------- -----x-x -x-x-x-x    Texture Y / 8
*/

static int k001005_parse_polygon(int index, UINT32 cmd)
{
	int viewport_min_x = clamp_int(256 + k001005_viewport_min_x + k001005_viewport_center_x, k001005_cliprect.min_x, k001005_cliprect.max_x);
	int viewport_max_x = clamp_int(256 + k001005_viewport_max_x + k001005_viewport_center_x + 1, k001005_cliprect.min_x, k001005_cliprect.max_x);
	int viewport_min_y = clamp_int(200 + k001005_viewport_min_y - k001005_viewport_center_y, k001005_cliprect.min_y, k001005_cliprect.max_y);
	int viewport_max_y = clamp_int(200 + k001005_viewport_max_y - k001005_viewport_center_y + 1, k001005_cliprect.min_y, k001005_cliprect.max_y);
	rectangle cliprect;
	int start_index = index;
	const UINT32 *fifo = k001005_3dfifo;
	int has_texture = (cmd & 0x18) != 0;
	int has_vertex_color = (cmd >> 8) & 1;
	/* command 0x121 breaks the logic here, maybe vertex color enforces vertex z too? */
	int has_vertex_z = !(cmd & 1) || has_vertex_color;
	UINT32 texture_x = 0;
	UINT32 texture_y = 0;
	UINT32 texture_width = 0;
	UINT32 texture_height = 0;
	UINT32 texture_page = 0;
	UINT32 texture_palette = 0;
	UINT32 tex_header;
	poly_draw_scanline_func scan_tex_vc;

	cliprect.min_x = viewport_min_x;
	cliprect.max_x = viewport_max_x;
	cliprect.min_y = viewport_min_y;
	cliprect.max_y = viewport_max_y;

	scan_tex_vc = has_texture ? draw_scanline_vertex_color_tex : draw_scanline_vertex_color;

	/* texture header - only for textured polys */
	if (has_texture)
	{
		tex_header = fifo[index++];

		texture_x = (((tex_header >> 6) & 0x20) | ((tex_header >> 5) & 0x10) | ((tex_header >> 4) & 0x8) |
					((tex_header >> 3) & 0x4) | ((tex_header >> 2) & 0x2) | ((tex_header >> 1) & 0x1));

		texture_y = (((tex_header >> 5) & 0x20) | ((tex_header >> 4) & 0x10) | ((tex_header >> 3) & 0x8) |
					((tex_header >> 2) & 0x4) | ((tex_header >> 1) & 0x2) | (tex_header & 0x1));

		texture_width = (tex_header >> 23) & 0x7;
		texture_height = (tex_header >> 20) & 0x7;
		texture_page = (tex_header >> 12) & 0x1f;
		texture_palette = (tex_header >> 28) & 0x7;
	}

	while ((fifo[index] & 0xffff0000) != 0x80000000 && index < k001005_3dfifo_ptr)
	{
		k001005_polydata *extra = (k001005_polydata *)poly_get_extra_data(poly);
		int last_vertex = 0;
		int is_quad = 0;
		UINT32 polygon_color = 0;
		float polygon_z = 0.0f;
		UINT32 polygon_diffuse = 0;
		int num_new_verts = 0;
		int v0, v1, v2, v3;

		do
		{
			poly_vertex *vertex = &k001005_vertexb[k001005_vertexb_ptr];

			/* X/Y coords, flags - all polys have this */
			int x = fifo[index] & 0x3fff;
			int y = (fifo[index] >> 16) & 0x1fff;

			if (x & 0x2000)
				x |= 0xffffc000;
			if (y & 0x1000)
				y |= 0xffffe000;

			vertex->x = ((float)x / 16.0f) + 256.0f;
			vertex->y = ((float)(-y) / 16.0f) + 200.0f;

			is_quad = (fifo[index] >> 14) & 1;
			last_vertex = (fifo[index] >> 15) & 1;
			index++;

			/* Z + diffuse intensity - if Z enabled */
			if (has_vertex_z)
			{
				UINT32 z = fifo[index] & 0xffffff00;	/* 32-bit float with low 8-bits of mantissa masked out */
				int diffuse = fifo[index] & 0xff;
				index++;

				vertex->p[POLY_Z] = u2f(z);
				vertex->p[POLY_DIFF] = diffuse;
				vertex->p[POLY_W] = 1.0f / vertex->p[POLY_Z];
			}
			else
			{
				vertex->p[POLY_W] = 1.0f;
			}

			/* textured polygons have a polygon color field after last vertex data, but before the last UV coords */
			if (last_vertex && has_texture)
			{
				/* polygon Z comes before the last UV coords for textured polygons */
				if (!has_vertex_z)
				{
					UINT32 z = (fifo[index] & 0x07ffff00) | 0x48000000;	/* like fog values, these seem to be missing the 4 upper bits of exponent */
					polygon_diffuse = fifo[index] & 0xff;
					index++;
					polygon_z = u2f(z);
				}

				if (!has_vertex_color)
				{
					polygon_color = fifo[index];
					index++;
				}
			}

			/* vertex color */
			if (has_vertex_color)
			{
				UINT32 vertex_color = fifo[index];
				index++;

				vertex->p[POLY_A] = (vertex_color >> 24) & 0xff;
				vertex->p[POLY_B] = (vertex_color >> 16) & 0xff;
				vertex->p[POLY_G] = (vertex_color >> 8) & 0xff;
				vertex->p[POLY_R] = vertex_color & 0xff;
			}

			/* UV coords - only for texture polys */
			if (has_texture)
			{
				INT32 tu = (INT16)(fifo[index] >> 16);
				INT32 tv = (INT16)(fifo[index] & 0xffff);
				index++;

				vertex->p[POLY_U] = (float)tu * vertex->p[POLY_W];
				vertex->p[POLY_V] = (float)tv * vertex->p[POLY_W];
			}

			/* fog */
			if (k001005_reg_fog_start == 0xffff)
			{
				/* max fog start value means fog is off */
				vertex->p[POLY_FOG] = 0.0f;
			}
			else
			{
				float fog_factor = (k001005_fog_end_z - vertex->p[POLY_Z]) / (k001005_fog_end_z - k001005_fog_start_z);
				vertex->p[POLY_FOG] = fog_factor * 255.0f;
			}

			num_new_verts++;
			k001005_vertexb_ptr = (k001005_vertexb_ptr + 1) & 3;
		}
		while (!last_vertex && num_new_verts < 4);

		/* for non-textured polygons, polygon color comes after vertex data */
		if (!has_texture)
		{
			/* polygon Z */
			if (!has_vertex_z)
			{
				UINT32 z = (fifo[index] & 0x07ffff00) | 0x48000000;	/* like fog values, these seem to be missing the 4 upper bits of exponent */
				polygon_diffuse = fifo[index] & 0xff;
				index++;

				polygon_z = u2f(z);
			}

			/* polygon color */
			if (!has_vertex_color)
			{
				polygon_color = fifo[index];
				index++;
			}
		}

		/* apply constant Z to all verts if needed */
		if (!has_vertex_z)
		{
			int j;
			for (j = 0; j < 4; j++)
			{
				k001005_vertexb[j].p[POLY_Z] = polygon_z;
				k001005_vertexb[j].p[POLY_DIFF] = polygon_diffuse;
			}
		}

		extra->texture_x = texture_x;
		extra->texture_y = texture_y;
		extra->texture_width = texture_width;
		extra->texture_height = texture_height;
		extra->texture_page = texture_page;
		extra->texture_palette = texture_palette;
		extra->texture_mirror = (cmd & 0x10) != 0;
		extra->diffuse_light = MAKE_ARGB(0xff, k001005_light_r, k001005_light_g, k001005_light_b);
		extra->ambient_light = MAKE_ARGB(0xff, k001005_ambient_r, k001005_ambient_g, k001005_ambient_b);
		extra->fog_color = MAKE_ARGB(0xff, k001005_fog_r, k001005_fog_g, k001005_fog_b);
		extra->fog_enable = (k001005_reg_fog_start != 0xffff) && !(cmd & 1);
		extra->cmd = cmd;
		extra->poly_color = MAKE_ARGB((polygon_color >> 24) & 0xff, polygon_color & 0xff, (polygon_color >> 8) & 0xff, (polygon_color >> 16) & 0xff);

		/* If 4 new vertices were found, but no last vertex tag - we're reading garbage.
           The data comes directly from the display list, so it seems intentional. */
		if (num_new_verts >= 4 && !last_vertex)
			break;

		/* The vertex buffer is a 4-entry circular buffer.
           Each polygon has at least one new vertex. 0-3 vertices are reused based on how many new vertices were inserted. */
		v0 = (k001005_vertexb_ptr - 4) & 3;
		v1 = (k001005_vertexb_ptr - 3) & 3;
		v2 = (k001005_vertexb_ptr - 2) & 3;
		v3 = (k001005_vertexb_ptr - 1) & 3;

		/* Some triangle strips have different shading values compared to reused vertices, causing unintended smooth shading.
           Bit 0x20 could be a select between flat shading and gouraud shading. */
		if (!(cmd & 0x20) && num_new_verts < 3)
		{
			int last_diffuse = (int)k001005_vertexb[v3].p[POLY_DIFF];
			k001005_vertexb[v0].p[POLY_DIFF] = last_diffuse;
			k001005_vertexb[v1].p[POLY_DIFF] = last_diffuse;
			k001005_vertexb[v2].p[POLY_DIFF] = last_diffuse;
		}

		/* No texture, constant color:   Z, Fog, Diffuse
           Texture, constant color:      Z, Fog, Diffuse, U, V, W
           Per-vertex color:             Z, Fog, Diffuse, U, V, W, R, G, B, A */
		if (is_quad)
		{
			if (has_vertex_color)
			{
				poly_render_triangle(poly, 0, &cliprect, scan_tex_vc, 10, &k001005_vertexb[v0], &k001005_vertexb[v1], &k001005_vertexb[v2]);
				poly_render_triangle(poly, 0, &cliprect, scan_tex_vc, 10, &k001005_vertexb[v2], &k001005_vertexb[v3], &k001005_vertexb[v0]);
			}
			else if (has_texture)
			{
				poly_render_triangle(poly, 0, &cliprect, draw_scanline_tex, 6, &k001005_vertexb[v0], &k001005_vertexb[v1], &k001005_vertexb[v2]);
				poly_render_triangle(poly, 0, &cliprect, draw_scanline_tex, 6, &k001005_vertexb[v2], &k001005_vertexb[v3], &k001005_vertexb[v0]);
			}
			else
			{
				poly_render_triangle(poly, 0, &cliprect, draw_scanline_color, 3, &k001005_vertexb[v0], &k001005_vertexb[v1], &k001005_vertexb[v2]);
				poly_render_triangle(poly, 0, &cliprect, draw_scanline_color, 3, &k001005_vertexb[v2], &k001005_vertexb[v3], &k001005_vertexb[v0]);
			}
		}
		else
		{
			if (has_vertex_color)
				poly_render_triangle(poly, 0, &cliprect, scan_tex_vc, 10, &k001005_vertexb[v1], &k001005_vertexb[v2], &k001005_vertexb[v3]);
			else if (has_texture)
				poly_render_triangle(poly, 0, &cliprect, draw_scanline_tex, 6, &k001005_vertexb[v1], &k001005_vertexb[v2], &k001005_vertexb[v3]);
			else
				poly_render_triangle(poly, 0, &cliprect, draw_scanline_color, 3, &k001005_vertexb[v1], &k001005_vertexb[v2], &k001005_vertexb[v3]);
		}
	}

	return index - start_index;
}

static void k001005_render_polygons(void)
{
	int index = 0;

	do
	{
		UINT32 cmd = k001005_3dfifo[index++];

		if (cmd == 0x80000000 || cmd == 0x80000018)
		{
		}
		else if ((cmd & 0xffff0000) == 0x80000000)
		{
			index += k001005_parse_polygon(index, cmd);
		}
	}
	while (index < k001005_3dfifo_ptr);

	k001005_3dfifo_ptr = 0;
}

static void k001005_push_data(UINT32 data)
{
	/* process the current vertex data if a sync command is being sent (usually means the global registers are being changed) */
	if (data == 0x80000000)
		k001005_render_polygons();

	k001005_3dfifo[k001005_3dfifo_ptr++] = data;
}

int k001005_start(int dsp_cpunum, int width, int height)
{
	int k, i;

	k001005_width = width;
	k001005_height = height;

	k001005_ram[0] = auto_malloc(0x140000 * sizeof(UINT16));
	k001005_ram[1] = auto_malloc(0x140000 * sizeof(UINT16));
	memset(k001005_ram[0], 0, 0x140000 * sizeof(UINT16));
	memset(k001005_ram[1], 0, 0x140000 * sizeof(UINT16));

	k001005_fifo = auto_malloc(0x800 * sizeof(UINT32));
	memset(k001005_fifo, 0, 0x800 * sizeof(UINT32));

	k001005_3dfifo = auto_malloc(FIFO_SIZE * sizeof(UINT32));
	memset(k001005_3dfifo, 0, FIFO_SIZE * sizeof(UINT32));

	for (i = 0; i < 2; ++i)
	{
		k001005_fb[i] = auto_malloc(width * height * sizeof(UINT32));
		memset(k001005_fb[i], 0, width * height * sizeof(UINT32));
	}
	k001005_zb = auto_malloc(width * height * sizeof(float));

	for (k = 0; k < 8; k++)
	{
		int size = (k + 1) * 8;

		k001005_tex_mirror_table[0][k] = auto_malloc(128 * sizeof(int));
		k001005_tex_mirror_table[1][k] = auto_malloc(128 * sizeof(int));

		for (i = 0; i < 128; i++)
		{
			k001005_tex_mirror_table[0][k][i] = i % size;
			k001005_tex_mirror_table[1][k][i] = (i % (size * 2)) >= size ? ((size - 1) - (i % size)) : (i % size);
		}
	}

	poly = poly_alloc(4000, sizeof(k001005_polydata), 0);

	k001005_cliprect.min_x = 0;
	k001005_cliprect.max_x = width - 1;
	k001005_cliprect.min_y = 0;
	k001005_cliprect.max_y = height - 1;

	k001005_fb_page = 0;
	k001005_3dfifo_ptr = 0;
	k001005_vertexb_ptr = 0;
	k001005_status = 0;
	k001005_ram_ptr = 0;
	k001005_fifo_read_ptr = 0;
	k001005_fifo_write_ptr = 0;
	k001005_light_r = k001005_light_g = k001005_light_b = 0;
	k001005_ambient_r = k001005_ambient_g = k001005_ambient_b = 0;
	k001005_fog_r = k001005_fog_g = k001005_fog_b = 0;
	k001005_fog_start_z = k001005_fog_end_z = 0;
	k001005_reg_fog_start = 0;
	k001005_viewport_min_x = k001005_viewport_max_x = 0;
	k001005_viewport_min_y = k001005_viewport_max_y = 0;
	k001005_viewport_center_x = k001005_viewport_center_y = 0;

	k001005_swap_buffers();

	return 0;
}

/*
    The K001005 is accessed only by its SHARC, so the FIFO flag is changed
    directly in the running core. Changing it as an input line queues an
    event that ends the timeslice of all the CPUs at every FIFO access.
*/

READ32_HANDLER( k001005_r )
{
	switch (offset)
	{
		case 0x000:			/* FIFO read, high 16 bits */
			return k001005_fifo[k001005_fifo_read_ptr] >> 16;

		case 0x001:			/* FIFO read, low 16 bits */
		{
			UINT16 value = k001005_fifo[k001005_fifo_read_ptr] & 0xffff;

			if (k001005_status != 1 && k001005_status != 2)
			{
				if (k001005_fifo_read_ptr < 0x3ff)
					sharc_set_flag_input(1, 0);
				else
					sharc_set_flag_input(1, 1);
			}
			else
			{
				sharc_set_flag_input(1, 1);
			}

			k001005_fifo_read_ptr++;
			k001005_fifo_read_ptr &= 0x7ff;
			return value;
		}

		case 0x11b:			/* status ? */
			return 0x8002;

		case 0x11c:			/* slave status ? */
			return 0x8000;

		case 0x11f:
		{
			UINT32 ret;

			if (k001005_ram_ptr >= 0x400000)
				ret = k001005_ram[1][k001005_ram_ptr & 0x3fffff];
			else
				ret = k001005_ram[0][k001005_ram_ptr & 0x3fffff];
			k001005_ram_ptr++;
			return ret;
		}
	}
	return 0;
}

WRITE32_HANDLER( k001005_w )
{
	switch (offset)
	{
		case 0x000:			/* FIFO write */
			if (k001005_status != 1 && k001005_status != 2)
			{
				if (k001005_fifo_write_ptr < 0x400)
					sharc_set_flag_input(1, 1);
				else
					sharc_set_flag_input(1, 0);
			}
			else
			{
				sharc_set_flag_input(1, 1);
			}

			k001005_fifo[k001005_fifo_write_ptr] = data;
			k001005_fifo_write_ptr++;
			k001005_fifo_write_ptr &= 0x7ff;

			k001005_push_data(data);

			/* HACK to get past the FIFO B test (GTI Club, Thunder Hurricane, Winding Heat and Midnight Run) */
			if (activecpu_get_pc() == 0x201ee || activecpu_get_pc() == 0x201e6)
			{
				/* this is used to make the SHARC timeout */
				cpu_spinuntil_trigger(10000);
			}
			break;

		case 0x103: k001005_viewport_min_x = data & 0xffff; break;
		case 0x104: k001005_viewport_max_x = data & 0xffff; break;
		case 0x105: k001005_viewport_max_y = data & 0xffff; break;
		case 0x106: k001005_viewport_min_y = data & 0xffff; break;

		case 0x107: k001005_viewport_center_x = data & 0xffff; break;
		case 0x108: k001005_viewport_center_y = data & 0xffff; break;

		case 0x10a: k001005_light_r = data & 0xff; break;
		case 0x10b: k001005_light_g = data & 0xff; break;
		case 0x10c: k001005_light_b = data & 0xff; break;

		case 0x10d: k001005_ambient_r = data & 0xff; break;
		case 0x10e: k001005_ambient_g = data & 0xff; break;
		case 0x10f: k001005_ambient_b = data & 0xff; break;

		case 0x110: k001005_fog_r = data & 0xff; break;
		case 0x111: k001005_fog_g = data & 0xff; break;
		case 0x112: k001005_fog_b = data & 0xff; break;

		case 0x117:			/* linear fog start Z */
			/* 4 bits exponent + 12 bits mantissa, value of 0xffff is used to effectively turn off fog,
               implicit exponent 1001xxxx, sign bit 0 (z-values are all positive) */
			k001005_reg_fog_start = data & 0xffff;
			k001005_fog_start_z = u2f((0x90000 | (data & 0xffff)) << 11);
			break;

		case 0x118:			/* linear fog end Z */
			k001005_fog_end_z = u2f((0x90000 | (data & 0xffff)) << 11);
			break;

		case 0x11a:
			k001005_status = data;
			k001005_fifo_write_ptr = 0;
			k001005_fifo_read_ptr = 0;

			if (data == 2)
			{
				if (k001005_3dfifo_ptr > 0)
					k001005_render_polygons();

				/* the queued polygons are rendered in the back buffer, before the swap */
				poly_wait(poly, "swap");
				k001005_swap_buffers();
			}
			break;

		case 0x11d:
			k001005_fifo_write_ptr = 0;
			k001005_fifo_read_ptr = 0;
			break;

		case 0x11e:
			k001005_ram_ptr = data;
			break;

		case 0x11f:
			if (k001005_ram_ptr >= 0x400000)
				k001005_ram[1][(k001005_ram_ptr++) & 0x3fffff] = data & 0xffff;
			else
				k001005_ram[0][(k001005_ram_ptr++) & 0x3fffff] = data & 0xffff;
			break;
	}
}

void k001005_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int j;

	for (j = cliprect->min_y; j <= cliprect->max_y; j++)
	{
		UINT32 *bmp = (UINT32 *)bitmap->line[j];
		const UINT32 *src = k001005_fb[k001005_fb_page ^ 1] + (j - cliprect->min_y) * k001005_width;
		int i;

		for (i = cliprect->min_x; i <= cliprect->max_x; i++)
		{
			UINT32 pixel = src[i - cliprect->min_x];
			if (pixel & 0xff000000)
				bmp[i] = pixel & 0x00ffffff;
		}
	}
}
