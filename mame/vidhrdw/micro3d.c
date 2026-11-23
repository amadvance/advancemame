// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Microprose Games 3D hardware

    TODO:
    * Fix buffering issue
    * Find correct noise-fill LFSR arrangement

****************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "cpu/am29000/am29000.h"
#include "includes/micro3d.h"


UINT16 dpytap, dudate, dumask;


/*************************************
 *
 *  Defines
 *
 *************************************/

#define VTX_SEX(x)              ((x) | ((x) & (1 << 29) ? 0xc0000000 : 0))

enum
{
	STATE_DRAW_CMD,
	STATE_DRAW_CMD_DATA,
	STATE_DRAW_VTX_DATA
};

enum
{
	CLIP_Z_MIN,
	CLIP_Z_MAX,
	CLIP_X_MIN,
	CLIP_X_MAX,
	CLIP_Y_MIN,
	CLIP_Y_MAX
};

typedef struct
{
	INT32 x, y, z;
} micro3d_vtx;


/*************************************
 *
 *  Local variables
 *
 *************************************/

static UINT16 creg;
static UINT16 xfer3dk;

/* 3D pipeline */
static UINT32 pipe_data;
static UINT32 pipeline_state;
static INT32 vtx_fifo[512];
static UINT32 fifo_idx;
static UINT32 draw_cmd;
static UINT32 draw_state;
static INT32 x_min;
static INT32 x_max;
static INT32 y_min;
static INT32 y_max;
static INT32 z_min;
static INT32 z_max;
static INT32 x_mid;
static INT32 y_mid;
static UINT32 dpram_bank;
static UINT32 draw_dpram[1024];
static UINT16 *frame_buffers[2];
static UINT16 *tmp_buffer;
static UINT8 drawing_buffer;
static UINT8 display_buffer;


/*************************************
 *
 *  Palette
 *
 *************************************/

void changecolor_BBBBBRRRRRGGGGGG(pen_t color,int data)
{
	int r,g,b;

	r = (data >>  6) & 0x1f;
	g = (data >>  1) & 0x1f;
	b = (data >>  11) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(color,r,g,b);
}

WRITE16_HANDLER( paletteram16_BBBBBRRRRRGGGGGG_word_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	changecolor_BBBBBRRRRRGGGGGG(offset,paletteram16[offset]);
}


/*************************************
 *
 *  Video initialisation
 *
 *************************************/

VIDEO_START(micro3d)
{
	int i;

	/* two 1024x512 3D frame buffers and the one the 3D is drawn into */
	for (i = 0; i < 2; i++)
	{
		frame_buffers[i] = auto_malloc(1024 * 512 * sizeof(UINT16));
		memset(frame_buffers[i], 0, 1024 * 512 * sizeof(UINT16));
	}

	tmp_buffer = auto_malloc(1024 * 512 * sizeof(UINT16));
	memset(tmp_buffer, 0, 1024 * 512 * sizeof(UINT16));

	return 0;
}

void micro3d_video_reset(void)
{
	pipeline_state = 0;
	creg = 0;
	draw_state = STATE_DRAW_CMD;

	drawing_buffer = 0;
	display_buffer = 1;
}


/*************************************
 *
 *  2D graphics
 *
 *************************************/

VIDEO_UPDATE( micro3d )
{
	int x,y;
	UINT32 dpyctl, tap;
	int sd_11_7 = (creg & 0x1f) << 7;

	/* If the display is blanked, fill with black */
	if (tms34010_io_display_blanked(1))
	{
		fillbitmap(bitmap, Machine->pens[0], cliprect);
		return;
	}

	/* fetch current scanline advance and column offset values */
	cpuintrf_push_context(1);
	dpyctl = tms34010_io_register_r(REG_DPYCTL, 0);
	tap = tms34010_io_register_r(REG_DPYTAP, 0) & 0x3fff;
	cpuintrf_pop_context();
	dudate = dpyctl & 0x03fc;

	/* 7-bits per pixel for the TI 2D, it would seem.
       Bit 7 selects 2D or 3D. */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 scanline[576];
		UINT16 *frame_src = frame_buffers[display_buffer] + (y << 10);
		/* DPYADR counts down by DUDATE every line, the address is its complement */
		UINT32 addr = (dpyadr + y * dudate) & 0xfffc;
		UINT16 *src = &micro3d_sprite_vram[((addr >> 4) << 8) & 0x7fe00];
		int coladdr = ((addr & 0x007c) << 4) | tap;

		for (x = 0; x < 576; x+=2)
		{
			UINT16 adata = src[coladdr++ & 0x1ff];

			/* TODO: The upper four bits of the 3D buffer affect priority
               0xfxx forces 3D priority? */
			if (adata & 0x0080)
				scanline[x] = sd_11_7 | (adata & 0x007f);
			else
				scanline[x] = frame_src[x] & 0xfff;

			if (adata & 0x8000)
				scanline[x+1] = sd_11_7 | ((adata & 0x7f00) >> 8);
			else
				scanline[x+1] = frame_src[x+1] & 0xfff;
		}

		/* Draw the scanline */
		draw_scanline16(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x + 1, &scanline[cliprect->min_x], Machine->pens, -1);
	}
}

WRITE16_HANDLER( micro3d_creg_w )
{
	if (~data & 0x80)
		cpunum_set_input_line(1, 0, CLEAR_LINE);

	creg = data;
}

WRITE16_HANDLER( micro3d_xfer3dk_w )
{
	xfer3dk = data;
}


/*************************************
 *
 *  3D graphics
 *
 *************************************/

static int inside(micro3d_vtx *v, int plane)
{
	switch (plane)
	{
		case CLIP_Z_MIN: return v->z >= z_min;
		case CLIP_Z_MAX: return v->z <= z_max;
		case CLIP_X_MIN: return v->x >= x_min;
		case CLIP_X_MAX: return v->x <= x_max;
		case CLIP_Y_MIN: return v->y >= y_min;
		case CLIP_Y_MAX: return v->y <= y_max;
	}

	return 0;
}

/* Calculate where two points intersect */
static micro3d_vtx intersect(micro3d_vtx *v1, micro3d_vtx *v2, int plane)
{
	micro3d_vtx vo = { 0, 0, 0 };
	float m = 0.0;

	if (v1->x != v2->x)
		m = (float)(v1->y - v2->y) / (float)(v1->x - v2->x);

	switch (plane)
	{
		case CLIP_Z_MIN:
		{
			float mxz, myz;

			if (v1->z != v2->z)
			{
				mxz = (float)(v1->x - v2->x) / (float)(v1->z - v2->z);
				myz = (float)(v1->y - v2->y) / (float)(v1->z - v2->z);
			}
			else
			{
				mxz = 0.0;
				myz = 0.0;
			}

			vo.x = v2->x + (z_min - v2->z) * mxz;
			vo.y = v2->y + (z_min - v2->z) * myz;
			vo.z = z_min;
			break;
		}
		case CLIP_Z_MAX:
		{
			float mxz, myz;

			if (v1->z != v2->z)
			{
				mxz = (float)(v1->x - v2->x) / (float)(v1->z - v2->z);
				myz = (float)(v1->y - v2->y) / (float)(v1->z - v2->z);
			}
			else
			{
				mxz = 0.0;
				myz = 0.0;
			}

			vo.x = v2->x + (z_max - v2->z) * mxz;
			vo.y = v2->y + (z_max - v2->z) * myz;
			vo.z = z_max;
			break;
		}
		case CLIP_X_MIN:
		{
			vo.x = x_min;
			vo.y = v2->y + (x_min - v2->x) * m;
			vo.z = 0;
			break;
		}
		case CLIP_X_MAX:
		{
			vo.x = x_max;
			vo.y = v2->y + (x_max - v2->x) * m;
			vo.z = 0;
			break;
		}
		case CLIP_Y_MIN:
		{
			if (v1->x != v2->x)
				vo.x = v2->x + (y_min - v2->y) / m;
			else
				vo.x = v2->x;

			vo.y = y_min;
			vo.z = 0;
			break;
		}
		case CLIP_Y_MAX:
		{
			if (v1->x != v2->x)
				vo.x = v2->x + (y_max - v2->y) / m;
			else
				vo.x = v2->x;

			vo.y = y_max;
			vo.z = 0;
			break;
		}
	}
	return vo;
}

INLINE void write_span(UINT32 y, UINT32 x)
{
	int addr = y << 1;

	if (draw_dpram[addr] == 0x3ff000)
	{
		draw_dpram[addr] = (x << 12) | x;
	}
	else
	{
		/* Check start */
		if (x < (draw_dpram[addr] & 0x3ff))
		{
			draw_dpram[addr] &= ~0x3ff;
			draw_dpram[addr] |= x;
		}
		/* Check end */
		if (x > (draw_dpram[addr] >> 12))
		{
			draw_dpram[addr] &= ~0x3ff000;
			draw_dpram[addr] |= (x << 12);
		}
	}
}

/* This is the same algorithm used in the 3D tests */
static void draw_line(UINT32 x1, UINT32 y1, UINT32 x2, UINT32 y2)
{
	UINT32 tmp2;
	UINT32 acc;
	UINT32 y_inc;

	UINT32 dx;
	UINT32 dy;

	if (x2 < x1)
	{
		UINT32 tmp;

		tmp = x1; x1 = x2; x2 = tmp;
		tmp = y1; y1 = y2; y2 = tmp;
	}

	dx = x2 - x1;

	if (y2 >= y1)
		dy = y2 - y1;
	else
		dy = y1 - y2;

	write_span(y1, x1);

	if (dx == 0 && dy == 0)
		return;

	if (y1 <= y2)
		y_inc = 1;
	else
		y_inc = -1;

	if (dx > dy)
	{
		if (y2 != y1)
		{
			tmp2 = dy << 1;
			acc = tmp2 - dx;
			dy = (dy - dx) << 1;

			do
			{
				if (~acc & 0x80000000)
				{
					write_span(y1, x1);
					y1 += y_inc;
					x1++;
					acc += dy;
					write_span(y1, x1);
				}
				else
				{
					acc += tmp2;
					x1++;
				}
			} while (y2 != y1);
		}

		if (x2 != x1)
			write_span(y1, x2);

	}
	else
	{
		tmp2 = dx << 1;
		acc = tmp2 - dy;
		dy = (dx - dy) << 1;

		if (y1 != y2)
		{
			do
			{
				if (acc & 0x80000000)
				{
					acc += tmp2;
					write_span(y1, x1);
					y1 += y_inc;
					write_span(y1, x1);
				}
				else
				{
					write_span(y1, x1);
					x1++;
					y1 += y_inc;
					write_span(y1, x1);

					acc += dy;
				}
			} while (y1 != y2);
		}

		if (x2 != x1)
			write_span(y1, x2);
	}
}

static void rasterise_spans(UINT32 min_y, UINT32 max_y, UINT32 attr)
{
	int y;
	int color = attr & 0xfff;

	if ((attr >> 24) == 0x85)
	{
		for (y = min_y; y <= max_y; ++y)
		{
			int addr = y << 1;
			UINT16 *dest = &tmp_buffer[y * 1024];

			if (draw_dpram[addr] != 0x3ff000)
			{
				int x;
				int start = draw_dpram[addr] & 0x3ff;
				int end = (draw_dpram[addr] >> 12) & 0x3ff;

				for (x = start; x <= end; ++x)
					dest[x] = color;
			}
		}
	}
	else
	{
		/*
            I don't know the LFSR arrangement inside the DRAW2 ASIC
            but here are some possible tap arrangements
        */
		static const UINT8 taps[8][4] =
		{
			{9, 8, 7, 2},
			{9, 8, 6, 5},
			{9, 8, 5, 4},
			{9, 8, 5, 1},
			{9, 8, 4, 2},
			{9, 7, 6, 4},
			{9, 7, 5, 2},
			{9, 6, 5, 3},
		};

		int noise_val = (attr >> 12) & 0x3ff;
		int noise_taps = 0;

		for (y = min_y; y <= max_y; ++y)
		{
			int addr = y << 1;
			UINT16 *dest = &tmp_buffer[y * 1024];

			if (draw_dpram[addr] != 0x3ff000)
			{
				int x;
				int start = draw_dpram[addr] & 0x3ff;
				int end = (draw_dpram[addr] >> 12) & 0x3ff;

				for (x = start; x <= end; ++x)
				{
					int fb;

					if (noise_val & 0x1)
						dest[x] = color;

					fb = ((noise_val >> (taps[noise_taps][0] - 1)) ^ (noise_val >> (taps[noise_taps][1] - 1)) ^ (noise_val >> (taps[noise_taps][2] - 1)) ^ (noise_val >> (taps[noise_taps][3] - 1))) & 1;
					noise_val = ((noise_val << 1) | fb) & 0x1ff;
				}
			}
		}
	}
}

static int clip_triangle(micro3d_vtx *v, micro3d_vtx *vout, int num_vertices, int plane)
{
	micro3d_vtx clip_out[10];
	int i;

	int prev_i = num_vertices - 1;
	int clip_verts = 0;

	for (i = 0; i < num_vertices; ++i)
	{
		int v1_in = inside(&v[i], plane);
		int v2_in = inside(&v[prev_i], plane);

		/* Edge is inside */
		if (v1_in && v2_in)
		{
			clip_out[clip_verts++] = v[i];
		}
		/* Edge is leaving */
		else if (v1_in && !v2_in)
		{
			clip_out[clip_verts++] = intersect(&v[i], &v[prev_i], plane);
			clip_out[clip_verts++] = v[i];
		}
		/* Edge is entering */
		else if (!v1_in && v2_in)
		{
			clip_out[clip_verts++] = intersect(&v[i], &v[prev_i], plane);
		}

		prev_i = i;
	}

	memcpy(&vout[0], &clip_out[0], sizeof(vout[0]) * clip_verts);
	return clip_verts;
}

static void draw_triangles(UINT32 attr)
{
	int triangles = 0;
	int vertices = fifo_idx / 3;
	int min_y = 0x3ff;
	int max_y = 0;
	int i;

	/* This satisifes the burst write test */
	if (vertices == 0)
	{
		int y;
		int val = ((x_mid + 16) << 12) | x_mid;

		for (y = y_mid; y <= y_mid + 16; ++y)
			draw_dpram[y << 1] = val;

		return;
	}

	/* Draw triangles as fans */
	for (i = 2; i < vertices; ++i)
	{
		int k;
		int clip_vertices = 3;

		micro3d_vtx vo, vm, vn;
		micro3d_vtx vclip_list[10];

		vo.x = vtx_fifo[0];
		vo.y = vtx_fifo[1];
		vo.z = vtx_fifo[2];

		vm.x = vtx_fifo[(i - 1) * 3 + 0];
		vm.y = vtx_fifo[(i - 1) * 3 + 1];
		vm.z = vtx_fifo[(i - 1) * 3 + 2];

		vn.x = vtx_fifo[i * 3 + 0];
		vn.y = vtx_fifo[i * 3 + 1];
		vn.z = vtx_fifo[i * 3 + 2];

		vclip_list[0] = vo;
		vclip_list[1] = vm;
		vclip_list[2] = vn;

		/* Clip against near Z and far Z planes */
		clip_vertices = clip_triangle(vclip_list, vclip_list, clip_vertices, CLIP_Z_MIN);
		clip_vertices = clip_triangle(vclip_list, vclip_list, clip_vertices, CLIP_Z_MAX);

		/* Perform perspective divide */
		for (k = 0; k < clip_vertices; ++k)
		{
			vclip_list[k].x = vclip_list[k].x * z_min / vclip_list[k].z;
			vclip_list[k].y = vclip_list[k].y * z_min / vclip_list[k].z;
			vclip_list[k].z = 0;
		}

		/* Perform screen-space clipping */
		clip_vertices = clip_triangle(vclip_list, vclip_list, clip_vertices, CLIP_Y_MAX);
		clip_vertices = clip_triangle(vclip_list, vclip_list, clip_vertices, CLIP_X_MIN);
		clip_vertices = clip_triangle(vclip_list, vclip_list, clip_vertices, CLIP_X_MAX);
		clip_vertices = clip_triangle(vclip_list, vclip_list, clip_vertices, CLIP_Y_MIN);

		/* Rasterise */
		if (clip_vertices >= 3)
		{
			micro3d_vtx a = vclip_list[0];
			micro3d_vtx b = vclip_list[1];

			triangles = 1;

			a.x += x_mid;
			a.y += y_mid;

			b.x += x_mid;
			b.y += y_mid;

			/* Keep track of the y-extents so we don't have to scan every line later */
			if (a.y < min_y)
				min_y = a.y;
			if (a.y > max_y)
				max_y = a.y;

			if (b.y < min_y)
				min_y = b.y;
			if (b.y > max_y)
				max_y = b.y;

			/* Draw the first line of the triangle/fan */
			draw_line(a.x, a.y, b.x, b.y);

			for (k = 2; k < clip_vertices; ++k)
			{
				micro3d_vtx c = vclip_list[k];

				c.x += x_mid;
				c.y += y_mid;

				if (c.y < min_y)
					min_y = c.y;
				if (c.y > max_y)
					max_y = c.y;

				draw_line(b.x, b.y, c.x, c.y);
				draw_line(a.x, a.y, c.x, c.y);
				b = c;
			}
		}
	}

	if (triangles)
		rasterise_spans(min_y, max_y, attr);
}


/******************************************************************************

MPGDRAW commands

80000000     Start triangle list [...]
85000xxx     End of vertex list - draw (xxx = colour) [0]
8Axxxxxx     End of vertex list - random fill shading (xxx = colour) [0]

90000000     Set clipping Z min [1]
94000000     Set clipping Z max [1]
98000000     Set clipping Y max [1]
9c000000     Set clipping X min [1]
a0000000     Set clipping X max [1]
a4000001     Set clipping Y min [1] (what does 1 mean?)

d8000000     End of frame (will swap render buffer)    [0]
f8000000     Toggle health LED [0]

b8000000-1fe Draw ASIC DPRAM address
b80003ff     Data

bc000000-1fc DPRAM address for read access

******************************************************************************/

WRITE32_HANDLER( micro3d_fifo_w )
{
	UINT32 opcode = data >> 24;

	switch (draw_state)
	{
		case STATE_DRAW_CMD:
		{
			draw_cmd = data;

			switch (opcode)
			{
				case 0xb4:
				{
					x_mid = data & 0x3ff;
					y_mid = (data >> 10) & 0x3ff;
					break;
				}
				case 0xc8:
				{
					dpram_bank ^= 1;
					break;
				}
				case 0xbc:
				{
					UINT32 dpram_r_addr = (((data & 0x01ff) << 1) | dpram_bank);
					pipe_data = draw_dpram[dpram_r_addr];
					cpunum_set_input_line(3, AM29000_INTR1, ASSERT_LINE);
					break;
				}
				case 0x80:
				{
					int addr;

					fifo_idx = 0;
					draw_state = STATE_DRAW_VTX_DATA;

					/* Invalidate the draw RAM
                     * TODO: Not sure this is the right place for it -
                     * causes monitor mode draw tests to fail
                     */
					for (addr = 0; addr < 512; ++addr)
						draw_dpram[addr << 1] = 0x3ff000;

					break;
				}
				case 0xf8:
				{
					/* 3D pipeline health LEDs toggle */
					break;
				}
				case 0xd8:
				{
					/* TODO: We shouldn't need this extra buffer - is there some sort of sync missing? */
					memcpy(frame_buffers[drawing_buffer], tmp_buffer, 512*1024*2);
					drawing_buffer ^= 1;
					cpunum_set_input_line(1, 0, ASSERT_LINE);
					break;
				}
				default:
					draw_state = STATE_DRAW_CMD_DATA;
			}
			break;
		}
		case STATE_DRAW_CMD_DATA:
		{
			switch (draw_cmd >> 24)
			{
				case 0x90: z_min = VTX_SEX(data); break;
				case 0x94: z_max = VTX_SEX(data); break;
				case 0x98: y_max = VTX_SEX(data); break;
				case 0x9c: x_min = VTX_SEX(data); break;
				case 0xa0: x_max = VTX_SEX(data); break;
				case 0xa4: y_min = VTX_SEX(data); break;
				case 0xb8:
				{
					draw_dpram[((draw_cmd & 0x1ff) << 1) | dpram_bank] = data & 0x00ffffff;
					break;
				}
				default:
					ui_popup("Unknown 3D command: %x %x\n", draw_cmd, data);
			}
			draw_state = STATE_DRAW_CMD;
			break;
		}
		case STATE_DRAW_VTX_DATA:
		{
			if ((opcode == 0x85) || (opcode == 0x8a))
			{
				draw_triangles(data);
				draw_state = STATE_DRAW_CMD;
			}
			else
			{
				vtx_fifo[fifo_idx++] = VTX_SEX(data);
			}
			break;
		}
	}
}

WRITE32_HANDLER( micro3d_alt_fifo_w )
{
	vtx_fifo[fifo_idx++] = VTX_SEX(data);
}

READ32_HANDLER( micro3d_pipe_r )
{
	cpunum_set_input_line(3, AM29000_INTR1, CLEAR_LINE);
	return pipe_data;
}

void micro3d_vblank_buffers(void)
{
	display_buffer = drawing_buffer ^ 1;
}
