/***************************************************************************

  gb.c

  Video file to handle emulation of the Nintendo GameBoy.

  Original code                               Carsten Sorensen   1998
  Mess modifications, bug fixes and speedups  Hans de Goede      1998
  Bug fixes, SGB and GBC code                 Anthony Kruize     2002

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/gb.h"
#include "profiler.h"

static UINT8 bg_zbuf[160];

INLINE void gb_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam, *vram;
	int i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = CURLINE;
	line = CURLINE + 16;

	oam = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, OAM + 39*4);
	vram = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, VRAM );
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, *spal;
			int xindex, adr;

			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				adr = (oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2;
			}
			else
			{
				adr = (oam[2] & tilemask) * 16 + (line - oam[0]) * 2;
			}
			data = (vram[adr + 1] << 8) | vram[adr];

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->pens[spal[colour]]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

struct layer_struct
{
	UINT8  enabled;
	UINT16 *bg_tiles;
	UINT8  *bg_map;
	UINT8  xindex;
	UINT8  xshift;
	UINT8  xstart;
	UINT8  xend;
	/* GBC specific */
	UINT16 *gbc_tiles[2];
	UINT8  *gbc_map;
	INT16  bgline;
};

void gb_refresh_scanline (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = CURLINE;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	profiler_mark(PROFILER_VIDEO);

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		rectangle r = Machine->visible_area;
		r.min_y = r.max_y = yindex;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && CURLINE >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + CURLINE) & 0xFF;

		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].bg_tiles = (UINT16 *) gb_chrgen + (bgline & 7);
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xstart = 0;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		bgline = (CURLINE - WNDPOSY) & 0xFF;
		xpos = WNDPOSX - 7;		/* Window is offset by 7 pixels */
		if (xpos < 0)
			xpos = 0;

		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].bg_tiles = (UINT16 *) gb_chrgen + (bgline & 7);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		layer[1].xstart = xpos;
		layer[1].xend = 160 - xpos;
		layer[0].xend = xpos;
	}

	while (l < 2)
	{
		/*
		 * BG display on
		 */
		UINT8 *map, xidx, bit, i;
		UINT16 *tiles, data;
		int xindex;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		tiles = layer[l].bg_tiles;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;

		data = (UINT16) (tiles[(map[xidx] ^ gb_tile_no_mod) * 8] << bit);
#ifndef LSB_FIRST
		data = (data << 8) | (data >> 8);
#endif
		xindex = layer[l].xstart;
		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
				plot_pixel(bitmap, xindex, yindex, Machine->pens[gb_bpal[colour]]);
				xindex++;
				*zbuf++ = colour;
				data <<= 1;
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			bit = 0;
			data = tiles[(map[xidx] ^ gb_tile_no_mod) * 8];
		}
		l++;
	}

	if (LCDCONT & 0x02)
		gb_update_sprites();

	profiler_mark(PROFILER_END);
}

/* --- Super Gameboy Specific --- */

INLINE void sgb_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam, *vram, pal;
	INT16 i, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	/* Offset to center of screen */
	yindex = CURLINE + SGB_YOFFSET;
	line = CURLINE + 16;

	oam = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, OAM + 39*4);
	vram = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, VRAM );
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, *spal;
			INT16 xindex;
			int adr;

			spal = (oam[3] & 0x10) ? gb_spal1 : gb_spal0;
			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				adr = (oam[2] & tilemask) * 16 + (height -1 - line + oam[0]) * 2;
			}
			else
			{
				adr = (oam[2] & tilemask) * 16 + (line - oam[0]) * 2;
			}
			data = (vram[adr + 1] << 8) | vram[adr];

			/* Find the palette to use */
			pal = sgb_pal_map[(xindex >> 3)][((yindex - SGB_YOFFSET) >> 3)] << 2;

			/* Offset to center of screen */
			xindex += SGB_XOFFSET;

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour && !bg_zbuf[xindex - SGB_XOFFSET])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour && !bg_zbuf[xindex - SGB_XOFFSET])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if ((xindex >= SGB_XOFFSET && xindex <= SGB_XOFFSET + 160) && colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + spal[colour]]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void sgb_refresh_scanline (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = CURLINE;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	profiler_mark(PROFILER_VIDEO);

	/* Handle SGB mask */
	switch( sgb_window_mask )
	{
		case 1:	/* Freeze screen */
			return;
		case 2:	/* Blank screen (black) */
			{
				rectangle r = Machine->visible_area;
				r.min_x = SGB_XOFFSET;
				r.max_x -= SGB_XOFFSET;
				r.min_y = SGB_YOFFSET;
				r.max_y -= SGB_YOFFSET;
				fillbitmap( bitmap, Machine->pens[0], &r );
			} return;
		case 3:	/* Blank screen (white - or should it be color 0?) */
			{
				rectangle r = Machine->visible_area;
				r.min_x = SGB_XOFFSET;
				r.max_x -= SGB_XOFFSET;
				r.min_y = SGB_YOFFSET;
				r.max_y -= SGB_YOFFSET;
				fillbitmap( bitmap, Machine->pens[32767], &r );
			} return;
	}

	/* Draw the "border" if we're on the first line */
	if( CURLINE == 0 )
	{
		sgb_refresh_border();
	}

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		rectangle r = Machine->visible_area;
		r.min_x = SGB_XOFFSET;
		r.max_x -= SGB_XOFFSET;
		r.min_y = r.max_y = yindex + SGB_YOFFSET;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && CURLINE >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + CURLINE) & 0xFF;

		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].bg_tiles = (UINT16 *) gb_chrgen + (bgline & 7);
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xstart = 0;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		bgline = (CURLINE - WNDPOSY) & 0xFF;
		/* Window X position is offset by 7 so we'll need to adust */
		xpos = WNDPOSX - 7;
		if (xpos < 0)
			xpos = 0;

		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].bg_tiles = (UINT16 *) gb_chrgen + (bgline & 7);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		layer[1].xstart = xpos;
		layer[1].xend = 160 - xpos;
		layer[0].xend = xpos;
	}

	while (l < 2)
	{
		/*
		 * BG display on
		 */
		UINT8 *map, xidx, bit, i, pal;
		UINT16 *tiles, data;
		int xindex;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		tiles = layer[l].bg_tiles;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;

		data = (UINT16) (tiles[(map[xidx] ^ gb_tile_no_mod) * 8] << bit);
#ifndef LSB_FIRST
		data = (data << 8) | (data >> 8);
#endif
		xindex = layer[l].xstart;

		/* Figure out which palette we're using */
		pal = sgb_pal_map[(xindex >> 3)][(yindex >> 3)] << 2;

		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
				plot_pixel(bitmap, xindex + SGB_XOFFSET, yindex + SGB_YOFFSET, Machine->remapped_colortable[pal + gb_bpal[colour]]);
				xindex++;
				*zbuf++ = colour;
				data <<= 1;
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			pal = sgb_pal_map[(xindex >> 3)][(yindex >> 3)] << 2;
			bit = 0;
			data = tiles[(map[xidx] ^ gb_tile_no_mod) * 8];
		}
		l++;
	}

	if (LCDCONT & 0x02)
		sgb_update_sprites();

	profiler_mark(PROFILER_END);
}

void sgb_refresh_border(void)
{
	UINT16 *tiles, *tiles2, data, data2;
	UINT16 *map;
	UINT16 yidx, xidx, xindex;
	UINT8 pal, i;
	mame_bitmap *bitmap = tmpbitmap;

	map = (UINT16 *)sgb_tile_map - 32;

	for( yidx = 0; yidx < 224; yidx++ )
	{
		xindex = 0;
		map += (yidx % 8) ? 0 : 32;
		for( xidx = 0; xidx < 32; xidx++ )
		{
			if( map[xidx] & 0x8000 ) /* Vertical flip */
				tiles = (UINT16 *)sgb_tile_data + (7 - (yidx % 8));
			else /* No vertical flip */
				tiles = (UINT16 *)sgb_tile_data + (yidx % 8);
			tiles2 = tiles + 8;

			pal = (map[xidx] & 0x1C00) >> 10;
			if( pal == 0 )
				pal = 1;
			pal <<= 4;

			if( sgb_hack ) /* A few games do weird stuff */
			{
				UINT16 tileno = map[xidx] & 0xFF;
				if( tileno >= 128 ) tileno = ((64 + tileno) % 128) + 128;
				else tileno = (64 + tileno) % 128;
				data = tiles[tileno * 16];
				data2 = tiles2[tileno * 16];
			}
			else
			{
				data = tiles[(map[xidx] & 0xFF) * 16];
				data2 = tiles2[(map[xidx] & 0xFF) * 16];
			}

			for( i = 0; i < 8; i++ )
			{
				register UINT8 colour;
				if( (map[xidx] & 0x4000) ) /* Horizontal flip */
				{
					colour = ((data  & 0x0001) ? 1 : 0) |
							 ((data  & 0x0100) ? 2 : 0) |
							 ((data2 & 0x0001) ? 4 : 0) |
							 ((data2 & 0x0100) ? 8 : 0);
					data >>= 1;
					data2 >>= 1;
				}
				else /* No horizontal flip */
				{
					colour = ((data  & 0x0080) ? 1 : 0) |
							 ((data  & 0x8000) ? 2 : 0) |
							 ((data2 & 0x0080) ? 4 : 0) |
							 ((data2 & 0x8000) ? 8 : 0);
					data <<= 1;
					data2 <<= 1;
				}
				/* A slight hack below so we don't draw over the GB screen.
				 * Drawing there is allowed, but due to the way we draw the
				 * scanline, it can obscure the screen even when it shouldn't.
				 */
				if( !((yidx >= SGB_YOFFSET && yidx < SGB_YOFFSET + 144) &&
					(xindex >= SGB_XOFFSET && xindex < SGB_XOFFSET + 160)) )
				{
					plot_pixel(bitmap, xindex, yidx, Machine->remapped_colortable[pal + colour]);
				}
				xindex++;
			}
		}
	}
}

/* --- Gameboy Color Specific --- */

INLINE void gbc_update_sprites (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 height, tilemask, line, *oam;
	int i, xindex, yindex;

	if (LCDCONT & 0x04)
	{
		height = 16;
		tilemask = 0xFE;
	}
	else
	{
		height = 8;
		tilemask = 0xFF;
	}

	yindex = CURLINE;
	line = CURLINE + 16;

	oam = memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM, OAM + 39*4);
	for (i = 39; i >= 0; i--)
	{
		/* if sprite is on current line && x-coordinate && x-coordinate is < 168 */
		if (line >= oam[0] && line < (oam[0] + height) && oam[1] && oam[1] < 168)
		{
			UINT16 data;
			UINT8 bit, pal;

			/* Handle mono mode for GB games */
			if( gbc_mode == GBC_MODE_MONO )
				pal = (oam[3] & 0x10) ? 8 : 4;
			else
				pal = GBC_PAL_OBJ_OFFSET + ((oam[3] & 0x7) * 4);

			xindex = oam[1] - 8;
			if (oam[3] & 0x40)		   /* flip y ? */
			{
				data = *((UINT16 *) &GBC_VRAMMap[(oam[3] & 0x8)>>3][(oam[2] & tilemask) * 16 + (height - 1 - line + oam[0]) * 2]);
			}
			else
			{
				data = *((UINT16 *) &GBC_VRAMMap[(oam[3] & 0x8)>>3][(oam[2] & tilemask) * 16 + (line - oam[0]) * 2]);
			}
#ifndef LSB_FIRST
			data = (data << 8) | (data >> 8);
#endif

			switch (oam[3] & 0xA0)
			{
			case 0xA0:				   /* priority is set (behind bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data >>= 1;
				}
				break;
			case 0x20:				   /* priority is not set (overlaps bgnd & wnd, flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x0100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					if((bg_zbuf[xindex] & 0x80) && (bg_zbuf[xindex] & 0x7f) && (LCDCONT & 0x1))
						colour = 0;
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data >>= 1;
				}
				break;
			case 0x80:				   /* priority is set (behind bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if (colour && !bg_zbuf[xindex])
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data <<= 1;
				}
				break;
			case 0x00:				   /* priority is not set (overlaps bgnd & wnd, don't flip x) */
				for (bit = 0; bit < 8; bit++, xindex++)
				{
					register int colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					if((bg_zbuf[xindex] & 0x80) && (bg_zbuf[xindex] & 0x7f) && (LCDCONT & 0x1))
						colour = 0;
					if (colour)
						plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[pal + colour]);
					data <<= 1;
				}
				break;
			}
		}
		oam -= 4;
	}
}

void gbc_refresh_scanline (void)
{
	mame_bitmap *bitmap = tmpbitmap;
	UINT8 *zbuf = bg_zbuf;
	int l = 0, yindex = CURLINE;

	/* layer info layer[0]=background, layer[1]=window */
	struct layer_struct layer[2];

	profiler_mark(PROFILER_VIDEO);

	/* if background or screen disabled clear line */
	if ((LCDCONT & 0x81) != 0x81)
	{
		rectangle r = Machine->visible_area;
		r.min_y = r.max_y = yindex;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}

	/* if lcd disabled return */
	if (!(LCDCONT & 0x80))
		return;

	/* Window is enabled if the hardware says so AND the current scanline is
	 * within the window AND the window X coordinate is <=166 */
	layer[1].enabled = ((LCDCONT & 0x20) && CURLINE >= WNDPOSY && WNDPOSX <= 166) ? 1 : 0;

	/* BG is enabled if the hardware says so AND (window_off OR (window_on
	 * AND window's X position is >=7 ) ) */
	layer[0].enabled = ((LCDCONT & 0x01) && ((!layer[1].enabled) || (layer[1].enabled && WNDPOSX >= 7))) ? 1 : 0;

	if (layer[0].enabled)
	{
		int bgline;

		bgline = (SCROLLY + CURLINE) & 0xFF;

		layer[0].bgline = bgline;
		layer[0].bg_map = gb_bgdtab;
		layer[0].bg_map += (bgline << 2) & 0x3E0;
		layer[0].gbc_map = gbc_bgdtab;
		layer[0].gbc_map += (bgline << 2) & 0x3E0;
		layer[0].gbc_tiles[0] = (UINT16 *)gb_chrgen + (bgline & 7);
		layer[0].gbc_tiles[1] = (UINT16 *)gbc_chrgen + (bgline & 7);
		layer[0].xindex = SCROLLX >> 3;
		layer[0].xshift = SCROLLX & 7;
		layer[0].xstart = 0;
		layer[0].xend = 160;
	}

	if (layer[1].enabled)
	{
		int bgline, xpos;

		bgline = (CURLINE - WNDPOSY) & 0xFF;
		/* Window X position is offset by 7 so we'll need to adust */
		xpos = WNDPOSX - 7;
		if (xpos < 0)
			xpos = 0;

		layer[1].bgline = bgline;
		layer[1].bg_map = gb_wndtab;
		layer[1].bg_map += (bgline << 2) & 0x3E0;
		layer[1].gbc_map = gbc_wndtab;
		layer[1].gbc_map += (bgline << 2) & 0x3E0;
		layer[1].gbc_tiles[0] = (UINT16 *)gb_chrgen + (bgline & 7);
		layer[1].gbc_tiles[1] = (UINT16 *)gbc_chrgen + (bgline & 7);
		layer[1].xindex = 0;
		layer[1].xshift = 0;
		layer[1].xstart = xpos;
		layer[1].xend = 160 - xpos;
		layer[0].xend = xpos;
	}

	while (l < 2)
	{
		/*
		 * BG display on
		 */
		UINT8 *map, *gbcmap, xidx, bit, i;
		UINT16 *tiles, data;
		int xindex;

		if (!layer[l].enabled)
		{
			l++;
			continue;
		}

		map = layer[l].bg_map;
		gbcmap = layer[l].gbc_map;
		xidx = layer[l].xindex;
		bit = layer[l].xshift;
		i = layer[l].xend;

		tiles = layer[l].gbc_tiles[(gbcmap[xidx] & 0x8) >> 3];
		if( (gbcmap[xidx] & 0x40) >> 6 ) /* vertical flip */
			tiles -= ((layer[l].bgline & 7) << 1) - 7;
		data = (UINT16)(tiles[(map[xidx] ^ gb_tile_no_mod) * 8] << bit);
#ifndef LSB_FIRST
		data = (data << 8) | (data >> 8);
#endif

		xindex = layer[l].xstart;
		while (i)
		{
			while ((bit < 8) && i)
			{
				register int colour;
				if( ((gbcmap[xidx] & 0x20) >> 5) ) /* horizontal flip */
				{
					colour = ((data & 0x100) ? 2 : 0) | ((data & 0x0001) ? 1 : 0);
					data >>= 1;
				}
				else /* no horizontal flip */
				{
					colour = ((data & 0x8000) ? 2 : 0) | ((data & 0x0080) ? 1 : 0);
					data <<= 1;
				}
				plot_pixel(bitmap, xindex, yindex, Machine->remapped_colortable[(((gbcmap[xidx] & 0x7) * 4) + colour)]);
				xindex++;
				/* If the priority bit is set then bump up the value, we'll
				 * check this when drawing sprites */
				*zbuf++ = colour + (gbcmap[xidx] & 0x80);
				bit++;
				i--;
			}
			xidx = (xidx + 1) & 31;
			bit = 0;
			tiles = layer[l].gbc_tiles[(gbcmap[xidx] & 0x8) >> 3];
			if( (gbcmap[xidx] & 0x40) >> 6 ) /* vertical flip */
				tiles -= ((layer[l].bgline & 7) << 1) - 7;
			data = (UINT16)(tiles[(map[xidx] ^ gb_tile_no_mod) * 8]);
		}
		l++;
	}

	if (LCDCONT & 0x02)
		gbc_update_sprites();

	profiler_mark(PROFILER_END);
}
