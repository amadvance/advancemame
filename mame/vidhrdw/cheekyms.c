// license:BSD-3-Clause
// copyright-holders:Lee Taylor, Chris Moore
/* Universal Cheeky Mouse video hardware */

#include "driver.h"
#include "sound/dac.h"

static UINT8 sprites[0x20];
UINT8 cheekyms_port80;
static tilemap *playfield;
static mame_bitmap *bitmap_buffer;

PALETTE_INIT( cheekyms )
{
	int i,j,bit,r,g,b;

	for (i = 0; i < 3; i++)
	{
		const unsigned char* color_prom_save = color_prom;

		/* lower nibble */
		for (j = 0;j < Machine->drv->total_colors/6;j++)
		{
			/* red component */
			bit = (color_prom[0] >> 0) & 0x01;
			r = 0xff * bit;
			/* green component */
			bit = (color_prom[0] >> 1) & 0x01;
			g = 0xff * bit;
			/* blue component */
			bit = (color_prom[0] >> 2) & 0x01;
			b = 0xff * bit;

			palette_set_color(((i*2)*Machine->drv->total_colors/6)+j,r,g,b);
			color_prom++;
		}

		color_prom = color_prom_save;

		/* upper nibble */
		for (j = 0;j < Machine->drv->total_colors/6;j++)
		{
			/* red component */
			bit = (color_prom[0] >> 4) & 0x01;
			r = 0xff * bit;
			/* green component */
			bit = (color_prom[0] >> 5) & 0x01;
			g = 0xff * bit;
			/* blue component */
			bit = (color_prom[0] >> 6) & 0x01;
			b = 0xff * bit;

			palette_set_color(((i*2+1)*Machine->drv->total_colors/6)+j,r,g,b);
			color_prom++;
		}
	}
}



static void get_tile_info(int index)
{
	int x = index & 0x1f;
	int y = index >> 5;
	int color;
	int palette = (cheekyms_port80 >> 2) & 0x10;

	if (x >= 0x1e)
		color = y < 0x0c ? 0x15 : (y < 0x14 ? 0x16 : 0x14);
	else
		color = palette | ((y == 4 || y == 0x1b) ? 0x0c : x >> 1);
	SET_TILE_INFO(0, videoram[index], color, 0)
}

WRITE8_HANDLER( cheekyms_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(playfield, offset);
	}
}

WRITE8_HANDLER( cheekyms_sprite_w )
{
	sprites[offset] = data;
}

WRITE8_HANDLER( cheekyms_port_40_w )
{
	DAC_data_w(0, (data & 0x80) ? 0x80 : 0);
}

WRITE8_HANDLER( cheekyms_port_80_w )
{
	if ((cheekyms_port80 ^ data) & 0x40)
		tilemap_mark_all_tiles_dirty(playfield);
	cheekyms_port80 = data;
	if (!(data & 4))
		cpunum_set_input_line(0, 0, CLEAR_LINE);
	flip_screen_set(data & 0x80);
}

VIDEO_START( cheekyms )
{
	memset(sprites, 0, sizeof(sprites));
	cheekyms_port80 = 0;
	bitmap_buffer = auto_bitmap_alloc(256, 256);
	playfield = tilemap_create(get_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	if (!playfield)
		return 1;
	tilemap_set_transparent_pen(playfield, 0);
	return 0;
}

VIDEO_UPDATE( cheekyms )
{
	int offs, x, y;
	int scrolly = (cheekyms_port80 >> 3) & 7;

	fillbitmap(bitmap, Machine->pens[0], cliprect);
	fillbitmap(bitmap_buffer, 0, &Machine->visible_area);

	for (offs = 0; offs < sizeof(sprites)/sizeof(sprites[0]); offs += 4)
	{
		int v1, sx, sy, col, code;

		v1  = sprites[offs + 0];
		sy  = sprites[offs + 1];
		sx  = 256 - sprites[offs + 2];
		col = (~sprites[offs + 3] & 0x07);

		if (!(sprites[offs + 3] & 0x08)) continue;

		code = (~v1 << 1) & 0x1f;

		if (v1 & 0x80)
		{
			if (!flip_screen)
			{
				code++;
			}

			drawgfx(bitmap,Machine->gfx[1],
					code,col,
					0,0,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,0);
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[1],
					code + 0x20,
					col,
					0,0,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,0);

			drawgfx(bitmap,Machine->gfx[1],
					code + 0x21,
					col,
					0,0,
					sx + 8*(v1 & 2),sy + 8*(~v1 & 2),
					cliprect,TRANSPARENCY_PEN,0);
		}
	}


	tilemap_draw(bitmap_buffer, &Machine->visible_area, playfield, 0, 0);

	/* Scroll only the character window; source pixels must not wrap into the border. */
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels;
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			int min_x = flip_screen ? (32 - 12 - 1) * 8 : 8 * 8;
			int max_x = flip_screen ? (32 - 8) * 8 : 12 * 8;
			int source_y = y;
			UINT16 pixel;
			if (x >= min_x && x < max_x && y > 5 * 8 && y < 27 * 8)
			{
				source_y += scrolly;
				if (source_y >= 27 * 8)
					continue;
			}
			pixel = *((UINT16 *)bitmap_buffer->base + source_y * bitmap_buffer->rowpixels + x);
			if (pixel)
				dest[x] = pixel;
		}
	}
}
