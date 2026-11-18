// license:BSD-3-Clause
// copyright-holders:Philip Bennett

/* Entertainment Sciences Real-Time Image Processor video hardware */

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "esripsys.h"

static struct
{
	UINT8 *colour_buf;
	UINT8 *intensity_buf;
	UINT8 *priority_buf;
} line_buffer[2];

static mame_bitmap *raster;
static mame_timer *raster_timer;
int esripsys_scanline;

static UINT8 *fig_scale_table;
static UINT8 *scale_table;

static int video_firq;
static UINT8 bg_intensity;

int esripsys_hblank;
int esripsys_video_firq_en;
int esripsys_frame_vbl;
int esripsys__12sel;
UINT8 *esripsys_pal_ram;

static void scanout(void)
{
	int x;
	UINT8 *colour = line_buffer[esripsys__12sel ? 0 : 1].colour_buf;
	UINT8 *intensity = line_buffer[esripsys__12sel ? 0 : 1].intensity_buf;
	UINT32 *dest = NULL;

	if (esripsys_scanline < ESRIPSYS_VBLANK_START)
		dest = raster->line[esripsys_scanline];

	if (dest)
		for (x = 0; x < ESRIPSYS_HBLANK_START; ++x)
		{
			int idx = colour[x];
			int i = intensity[x];
			int r = esripsys_pal_ram[idx] & 0xf;
			int g = esripsys_pal_ram[256 + idx] & 0xf;
			int b = esripsys_pal_ram[512 + idx] & 0xf;

			dest[x] = MAKE_RGB(r * i, g * i, b * i);
		}

	memset(colour, 0xff, ESRIPSYS_HBLANK_START);
	memset(intensity, bg_intensity, ESRIPSYS_HBLANK_START);
	memset(line_buffer[esripsys__12sel ? 0 : 1].priority_buf, 0, ESRIPSYS_HBLANK_START);
}

static void raster_tick(int end_blank)
{
	if (end_blank)
	{
		/* Finish scanning the current buffer before it becomes the RIP's drawing target. */
		scanout();
		esripsys_scanline = (esripsys_scanline + 1) % ESRIPSYS_VTOTAL;
		esripsys__12sel ^= 1;
		esripsys_hblank = 1;

		if (esripsys_scanline == ESRIPSYS_VBLANK_START)
		{
			cpunum_set_input_line(ESRIPSYS_GAME_CPU, M6809_IRQ_LINE, ASSERT_LINE);
			esripsys_frame_vbl = 0;
		}
		timer_adjust(raster_timer, TIME_IN_HZ(ESRIPSYS_PIXEL_CLOCK) * ESRIPSYS_HBLANK_START, 0, 0);
	}
	else
	{
		if (video_firq)
		{
			video_firq = 0;
			cpunum_set_input_line(ESRIPSYS_GAME_CPU, M6809_FIRQ_LINE, CLEAR_LINE);
		}

		if (esripsys_scanline && !(esripsys_scanline % 6) &&
			esripsys_scanline < ESRIPSYS_VBLANK_START && esripsys_video_firq_en)
		{
			video_firq = 1;
			cpunum_set_input_line(ESRIPSYS_GAME_CPU, M6809_FIRQ_LINE, ASSERT_LINE);
		}

		esripsys_hblank = 0;
		timer_adjust(raster_timer, TIME_IN_HZ(ESRIPSYS_PIXEL_CLOCK) * (ESRIPSYS_HTOTAL - ESRIPSYS_HBLANK_START), 1, 0);
	}
}

VIDEO_START( esripsys )
{
	int i;

	line_buffer[0].colour_buf = auto_malloc(512);
	line_buffer[0].intensity_buf = auto_malloc(512);
	line_buffer[0].priority_buf = auto_malloc(512);

	line_buffer[1].colour_buf = auto_malloc(512);
	line_buffer[1].intensity_buf = auto_malloc(512);
	line_buffer[1].priority_buf = auto_malloc(512);

	raster = auto_bitmap_alloc_depth(ESRIPSYS_HBLANK_START, ESRIPSYS_VBLANK_START, 32);
	if (!raster)
		return 1;

	raster_timer = timer_alloc(raster_tick);

	scale_table = auto_malloc(64 * 64);

	for (i = 0; i < 64; ++i)
	{
		int j;

		for (j = 1; j < 65; ++j)
		{
			int p0 = 0;
			int p1 = 0;
			int p2 = 0;
			int p3 = 0;
			int p4 = 0;
			int p5 = 0;

			if (i & 0x1)
				p0 = BIT(j, 5) && !BIT(j, 4) && !BIT(j,3) && !BIT(j, 2) && !BIT(j, 1) && !BIT(j, 0);
			if (i & 0x2)
				p1 = BIT(j, 4) && !BIT(j, 3) && !BIT(j, 2) && !BIT(j, 1) && !BIT(j, 0);
			if (i & 0x4)
				p2 = BIT(j,3) && !BIT(j, 2) && !BIT(j, 1) && !BIT(j, 0);
			if (i & 0x8)
				p3 = BIT(j, 2) && !BIT(j,1) && !BIT(j,0);
			if (i & 0x10)
				p4 = BIT(j, 1) && !BIT(j, 0);
			if (i & 0x20)
				p5 = BIT(j, 0);

			scale_table[i * 64 + j - 1] = p0 | p1 | p2 | p3 | p4 | p5;
		}
	}

	fig_scale_table = auto_malloc(1024 * 64);

	for (i = 0; i < 1024; ++i)
	{
		int scale;

		for (scale = 0; scale < 64; ++scale)
		{
			int input_pixels = i + 1;
			int scaled_pixels = 0;

			while (input_pixels)
			{
				if (scale_table[scale * 64 + (scaled_pixels & 0x3f)] == 0)
					input_pixels--;

				scaled_pixels++;
			}

			fig_scale_table[i * 64 + scale] = scaled_pixels - 1;
		}
	}

	state_save_register_global_pointer(line_buffer[0].colour_buf, 512);
	state_save_register_global_pointer(line_buffer[0].intensity_buf, 512);
	state_save_register_global_pointer(line_buffer[0].priority_buf, 512);

	state_save_register_global_pointer(line_buffer[1].colour_buf, 512);
	state_save_register_global_pointer(line_buffer[1].intensity_buf, 512);
	state_save_register_global_pointer(line_buffer[1].priority_buf, 512);

	state_save_register_global(video_firq);
	state_save_register_global(bg_intensity);
	state_save_register_global(esripsys_hblank);
	state_save_register_global(esripsys_video_firq_en);
	state_save_register_global(esripsys_frame_vbl);
	state_save_register_global(esripsys__12sel);
	return 0;
}

void esripsys_video_reset(void)
{
	int i;

	video_firq = 0;
	bg_intensity = 0;
	esripsys__12sel = 0;
	esripsys_frame_vbl = 0;
	esripsys_video_firq_en = 0;
	fillbitmap(raster, 0, NULL);

	for (i = 0; i < 2; ++i)
	{
		memset(line_buffer[i].colour_buf, 0xff, ESRIPSYS_HBLANK_START);
		memset(line_buffer[i].intensity_buf, 0, ESRIPSYS_HBLANK_START);
		memset(line_buffer[i].priority_buf, 0, ESRIPSYS_HBLANK_START);
	}

	esripsys_scanline = 0;
	esripsys_hblank = 1;
	timer_adjust(raster_timer, TIME_IN_HZ(ESRIPSYS_PIXEL_CLOCK) * ESRIPSYS_HBLANK_START, 0, 0);
}

VIDEO_UPDATE( esripsys )
{
	copybitmap(bitmap, raster, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
}

WRITE8_HANDLER( esripsys_bg_intensity_w )
{
	bg_intensity = data & 0xf;
}

int esripsys_draw(int l, int r, int fig, int attr, int addr, int col, int x_scale, int line_latch)
{
	UINT8 *colour_buf = line_buffer[esripsys__12sel ? 1 : 0].colour_buf;
	UINT8 *intensity_buf = line_buffer[esripsys__12sel ? 1 : 0].intensity_buf;
	UINT8 *priority_buf = line_buffer[esripsys__12sel ? 1 : 0].priority_buf;

	UINT8 pri = attr & 0xff;
	UINT8 iny = (attr >> 8) & 0xf;
	UINT8 pal = col << 4;
	int x_flip = x_scale & 0x80;
	int xs_typ = x_scale & 0x40;
	int xs_val = x_scale & 0x3f;

	if (xs_typ)
		fig = fig_scale_table[fig * 64 + xs_val];

	if (attr & 0x8000)
	{
		int ptr = 0;
		int cnt;
		UINT8 *rom_l;
		UINT8 *rom_r;
		UINT32 lpos = l;
		UINT32 rpos = r;

		if (x_flip)
		{
			rom_l = memory_region(REGION_GFX3);
			rom_r = memory_region(REGION_GFX2);
		}
		else
		{
			rom_l = memory_region(REGION_GFX2);
			rom_r = memory_region(REGION_GFX3);
		}

		for (cnt = 0; cnt <= fig; cnt++)
		{
			UINT32 rom_addr = (ptr * 0x10000) + (addr & 0xffff);
			UINT8 pix1 = rom_l[rom_addr];
			UINT8 pix2 = rom_r[rom_addr];

			if (lpos < 512)
			{
				if ((pri > priority_buf[lpos]) && pix1 != 0xff)
				{
					colour_buf[lpos] = pix1;
					priority_buf[lpos] = pri;
					intensity_buf[lpos] = iny;
				}
			}

			if (rpos < 512)
			{
				if ((pri > priority_buf[rpos]) && pix2 != 0xff)
				{
					colour_buf[rpos] = pix2;
					priority_buf[rpos] = pri;
					intensity_buf[rpos] = iny;
				}
			}

			if (!xs_typ)
			{
				if (scale_table[xs_val * 64 + (cnt & 0x3f)])
				{
					--lpos;
					++rpos;
				}

				if (++ptr == 4)
				{
					++addr;
					ptr = 0;
				}
			}
			else
			{
				if (!scale_table[xs_val * 64 + (cnt & 0x3f)])
				{
					if (++ptr == 4)
					{
						++addr;
						ptr = 0;
					}
				}

				lpos--;
				rpos++;
			}
		}
	}

	else
	{
		const UINT8* const rom = memory_region(REGION_GFX1);
		int ptr = 0;
		int cnt;
		UINT32 lpos = l;
		UINT32 rpos = r;

		for (cnt = 0; cnt <= fig; cnt++)
		{
			UINT8 px8 = rom[(ptr * 0x10000) + (addr & 0xffff)];
			UINT8 px1;
			UINT8 px2;

			if (x_flip)
			{
				px1 = px8 & 0xf;
				px2 = (px8 >> 4) & 0xf;
			}
			else
			{
				px2 = px8 & 0xf;
				px1 = (px8 >> 4) & 0xf;
			}

			if (lpos < 512)
			{
				if (pri > priority_buf[lpos] && px1 != 0xf)
				{
					colour_buf[lpos] = pal | px1;
					priority_buf[lpos] = pri;
					intensity_buf[lpos] = iny;
				}
			}

			if (rpos < 512)
			{
				if (pri > priority_buf[rpos] && px2 != 0xf)
				{
					colour_buf[rpos] = pal | px2;
					priority_buf[rpos] = pri;
					intensity_buf[rpos] = iny;
				}
			}

			if (!xs_typ)
			{
				if (scale_table[xs_val * 64 + (cnt & 0x3f)])
				{
					lpos--;
					rpos++;
				}

				if (++ptr == 4)
				{
					addr++;
					ptr = 0;
				}
			}
			else
			{
				if (!scale_table[xs_val * 64 + (cnt & 0x3f)])
				{
					if (++ptr == 4)
					{
						addr++;
						ptr = 0;
					}
				}
				lpos--;
				rpos++;
			}
		}
	}

	return fig + 1;
}
