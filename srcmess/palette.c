/******************************************************************************

    palette.c

    Palette handling functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

******************************************************************************/

#include "driver.h"
#include <math.h>
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
#include "debug/mamedbg.h"
#endif

#define VERBOSE 0


/*-------------------------------------------------
    CONSTANTS
-------------------------------------------------*/

#define PEN_BRIGHTNESS_BITS		8
#define MAX_PEN_BRIGHTNESS		(4 << PEN_BRIGHTNESS_BITS)

enum
{
	PALETTIZED_16BIT = 0,
	DIRECT_15BIT = 1,
	DIRECT_32BIT = 2,
	DIRECT_RGB = DIRECT_15BIT | DIRECT_32BIT
};

/*-------------------------------------------------
    GLOBAL VARIABLES
-------------------------------------------------*/

UINT32 direct_rgb_components[3];
UINT16 *palette_shadow_table;



/*-------------------------------------------------
    LOCAL VARIABLES
-------------------------------------------------*/

rgb_t *game_palette;				/* RGB palette as set by the driver */
static rgb_t *adjusted_palette;		/* actual RGB palette after brightness/gamma adjustments */
static UINT32 *dirty_palette;
static UINT16 *pen_brightness;

static UINT8 adjusted_palette_dirty;
static UINT8 debug_palette_dirty;

static UINT16 shadow_factor, highlight_factor;
static double global_brightness, global_brightness_adjust, global_gamma;

static UINT8 colormode, highlight_method;
static pen_t total_colors;
static pen_t total_colors_with_ui;

static pen_t black_pen, white_pen;

static UINT8 color_correct_table[(MAX_PEN_BRIGHTNESS * MAX_PEN_BRIGHTNESS) >> PEN_BRIGHTNESS_BITS];



/*-------------------------------------------------
    PROTOTYPES
-------------------------------------------------*/

static int palette_alloc(void);
static void palette_reset(void);
static void recompute_adjusted_palette(int brightness_or_gamma_changed);
static void internal_modify_pen(pen_t pen, rgb_t color, int pen_bright);



/*-------------------------------------------------
    rgb_to_direct15 - convert an RGB triplet to
    a 15-bit OSD-specified RGB value
-------------------------------------------------*/

INLINE UINT16 rgb_to_direct15(rgb_t rgb)
{
	return  (  RGB_RED(rgb) >> 3) * (direct_rgb_components[0] / 0x1f) +
			(RGB_GREEN(rgb) >> 3) * (direct_rgb_components[1] / 0x1f) +
			( RGB_BLUE(rgb) >> 3) * (direct_rgb_components[2] / 0x1f);
}



/*-------------------------------------------------
    rgb_to_direct32 - convert an RGB triplet to
    a 32-bit OSD-specified RGB value
-------------------------------------------------*/

INLINE UINT32 rgb_to_direct32(rgb_t rgb)
{
	return    RGB_RED(rgb) * (direct_rgb_components[0] / 0xff) +
			RGB_GREEN(rgb) * (direct_rgb_components[1] / 0xff) +
			 RGB_BLUE(rgb) * (direct_rgb_components[2] / 0xff);
}



/*-------------------------------------------------
    adjust_palette_entry - adjust a palette
    entry for brightness and gamma
-------------------------------------------------*/

INLINE rgb_t adjust_palette_entry(rgb_t entry, int pen_bright)
{
	int r = color_correct_table[(RGB_RED(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS];
	int g = color_correct_table[(RGB_GREEN(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS];
	int b = color_correct_table[(RGB_BLUE(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS];
	return MAKE_RGB(r,g,b);
}



/*-------------------------------------------------
    mark_pen_dirty - mark a given pen index dirty
-------------------------------------------------*/

INLINE void mark_pen_dirty(int pen)
{
	dirty_palette[pen / 32] |= 1 << (pen % 32);
}



/*-------------------------------------------------
    palette_start - palette initialization that
    takes place before the display is created
-------------------------------------------------*/

int palette_start(void)
{
	/* init statics */
	adjusted_palette_dirty = 1;
	debug_palette_dirty = 1;

	shadow_factor = (int)(PALETTE_DEFAULT_SHADOW_FACTOR * (double)(1 << PEN_BRIGHTNESS_BITS));
	highlight_factor = (int)(PALETTE_DEFAULT_HIGHLIGHT_FACTOR * (double)(1 << PEN_BRIGHTNESS_BITS));
	global_brightness = (options.brightness > .001) ? options.brightness : 1.0;
	global_brightness_adjust = 1.0;
	global_gamma = (options.gamma > .001) ? options.gamma : 1.0;

	/* determine the color mode */
	if (Machine->color_depth == 15)
		colormode = DIRECT_15BIT;
	else if (Machine->color_depth == 32)
		colormode = DIRECT_32BIT;
	else
		colormode = PALETTIZED_16BIT;

	highlight_method = 0;

	/* ensure that RGB direct video modes don't have a colortable */
	if ((Machine->drv->video_attributes & VIDEO_RGB_DIRECT) &&
			Machine->drv->color_table_len)
	{
		logerror("Error: VIDEO_RGB_DIRECT requires color_table_len to be 0.\n");
		return 1;
	}

	/* compute the total colors, including shadows and highlights */
	total_colors = Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS && !(colormode & DIRECT_RGB))
		total_colors += Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS && !(colormode & DIRECT_RGB))
		total_colors += Machine->drv->total_colors;
	total_colors_with_ui = total_colors;

	/* make sure we still fit in 16 bits */
	if (total_colors > 65536)
	{
		logerror("Error: palette has more than 65536 colors.\n");
		return 1;
	}

	/* allocate all the data structures */
	if (palette_alloc())
		return 1;

	/* set up save/restore of the palette */
	state_save_register_global_pointer(game_palette, total_colors);
	state_save_register_global_pointer(pen_brightness, Machine->drv->total_colors);
	state_save_register_func_postload(palette_reset);

	return 0;
}


//* 072703AT (last update)
/*-------------------------------------------------

    palette_set_shadow_mode(mode)

        mode: 0 = use preset 0 (default shadow)
              1 = use preset 1 (default highlight)
              2 = use preset 2 *
              3 = use preset 3 *

    * Preset 2 & 3 work independently under 32bpp,
      supporting up to four different types of
      shadows at one time. They mirror preset 1 & 2
      in lower depth settings to maintain
      compatibility.


    palette_set_shadow_factor32(factor)

        factor: 1.0(normal) to 0.0(pitch black)


    palette_set_highlight_factor32(factor)

        factor: 1.0(normal) and up(brighter)


    palette_set_shadow_dRGB32(mode, dr, dg, db, noclip)

        mode:    0 to   3 (which preset to configure)

          dr: -255 to 255 ( red displacement )
          dg: -255 to 255 ( green displacement )
          db: -255 to 255 ( blue displacement )

        noclip: 0 = resultant RGB clipped at 0x00/0xff
                1 = resultant RGB wraparound 0x00/0xff


    * Color shadows only work under 32bpp.
      This function has no effect in lower color
      depths where

        palette_set_shadow_factor32() or
        palette_set_highlight_factor32()

      should be used instead.

    * 32-bit shadows are lossy. Even with zero RGB
      displacements the affected area will still look
      slightly darkened.

      Drivers should ensure all shadow pens in
      gfx_drawmode_table[] are set to DRAWMODE_NONE
      when RGB displacements are zero to avoid the
      darkening effect.

-------------------------------------------------*/
#define MAX_SHADOW_PRESETS 4

static UINT32 *shadow_table_base[MAX_SHADOW_PRESETS];


static void internal_set_shadow_preset(int mode, double factor, int dr, int dg, int db, int noclip, int style, int init)
{
#define FP 16
#define FMAX (0x1f<<FP)

	static double oldfactor[MAX_SHADOW_PRESETS] = {-1,-1,-1,-1};
	static int oldRGB[MAX_SHADOW_PRESETS][3] = {{-1,-1,-1},{-1,-1,-1},{-1,-1,-1},{-1,-1,-1}};
	static int oldclip;

	UINT32 *table_ptr32;
	int i, fl, ov, r, g, b, d32;

	if (mode < 0 || mode >= MAX_SHADOW_PRESETS) return;

	if ((table_ptr32 = shadow_table_base[mode]) == NULL) return;

	if (style) // monotone shadows(style 1) or highlights(style 2)
	{
		if (factor < 0) factor = 0;

		if (!init && oldfactor[mode] == factor) return;

		oldfactor[mode] = factor;
		oldRGB[mode][2] = oldRGB[mode][1] = oldRGB[mode][0] = -1;

		if (!(colormode & DIRECT_RGB))
		{
			switch (style)
			{
				// modify shadows(first upper palette)
				case 1:
					palette_set_shadow_factor(factor);
				break;

				// modify highlights(second upper palette)
				case 2:
					palette_set_highlight_factor(factor);
				break;

				default: return;
			}
		}
		else
		{
			d32 = (colormode == DIRECT_32BIT);

			if (factor <= 1.0)
			{
				fl = (int)(factor * (1<<FP));

				for (i=0; i<32768; i++)
				{
					r = (i & 0x7c00) * fl;
					g = (i & 0x03e0) * fl;
					b = (i & 0x001f) * fl;

					r = r>>FP & 0x7c00;
					g = g>>FP & 0x03e0;
					b = b>>FP & 0x001f;

					if (d32)
						table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
					else
						((UINT16*)table_ptr32)[i] = (UINT16)(r | g | b);
				}
			}
			else
			{
				if (highlight_method == 0)
				{
					fl = (int)(factor * (1<<FP));

					for (i=0; i<32768; i++)
					{
						r = (i>>10 & 0x1f) * fl;
						g = (i>>5  & 0x1f) * fl;
						b = (i     & 0x1f) * fl;

						if (r >= FMAX) r = 0x7c00; else r = r>>(FP-10) & 0x7c00;
						if (g >= FMAX) g = 0x03e0; else g = g>>(FP-5)  & 0x03e0;
						if (b >= FMAX) b = 0x001f; else b = b>>(FP);

						if (d32)
							table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
						else
							((UINT16*)table_ptr32)[i] = (UINT16)(r | g | b);
					}
				}
				else if (highlight_method == 1)
				{
					fl = (int)(factor * (1<<FP));

					for (i=0; i<32768; i++)
					{
						r = (i>>10 & 0x1f) * fl;
						g = (i>>5  & 0x1f) * fl;
						b = (i     & 0x1f) * fl;
						ov = 0;

						if (r > FMAX) ov += r - FMAX;
						if (g > FMAX) ov += g - FMAX;
						if (b > FMAX) ov += b - FMAX;

						if (ov) { ov >>= 2;  r += ov;  g += ov;  b += ov; }

						if (r >= FMAX) r = 0x7c00; else r = r>>(FP-10) & 0x7c00;
						if (g >= FMAX) g = 0x03e0; else g = g>>(FP-5)  & 0x03e0;
						if (b >= FMAX) b = 0x001f; else b = b>>(FP);

						if (d32)
							table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
						else
							((UINT16*)table_ptr32)[i] = (UINT16)(r | g | b);
					}
				}
				else
				{
					fl = (int)(factor * 31 - 31);
					dr = fl<<10;
					dg = fl<<5;
					db = fl;

					for (i=0; i<32768; i++)
					{
						r = (i & 0x7c00) + dr;
						g = (i & 0x03e0) + dg;
						b = (i & 0x001f) + db;

						if (r > 0x7c00) r = 0x7c00;
						if (g > 0x03e0) g = 0x03e0;
						if (b > 0x001f) b = 0x001f;

						if (d32)
							table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
						else
							((UINT16*)table_ptr32)[i] = (UINT16)(r | g | b);
					}
				} // end of highlight_methods
			} // end of factor
		} // end of colormode

		#if VERBOSE
			ui_popup("shadow %d recalc factor:%1.2f style:%d", mode, factor, style);
		#endif
	}
	else // color shadows or highlights(style 0)
	{
		if (!(colormode & DIRECT_RGB)) return;

		if (dr < -0xff) dr = -0xff; else if (dr > 0xff) dr = 0xff;
		if (dg < -0xff) dg = -0xff; else if (dg > 0xff) dg = 0xff;
		if (db < -0xff) db = -0xff; else if (db > 0xff) db = 0xff;
		dr >>= 3; dg >>= 3; db >>= 3;

		if (!init && oldclip==noclip && oldRGB[mode][0]==dr && oldRGB[mode][1]==dg && oldRGB[mode][2]==db) return;

		oldclip = noclip;
		oldRGB[mode][0] = dr; oldRGB[mode][1] = dg; oldRGB[mode][2] = db;
		oldfactor[mode] = -1;

		#if VERBOSE
			ui_popup("shadow %d recalc %d %d %d %02x", mode, dr, dg, db, noclip);
		#endif

		dr <<= 10; dg <<= 5;
		d32 = (colormode == DIRECT_32BIT);

		if (noclip)
		{
			for (i=0; i<32768; i++)
			{
				r = (i & 0x7c00) + dr;
				g = (i & 0x03e0) + dg;
				b = (i & 0x001f) + db;

				r &= 0x7c00;
				g &= 0x03e0;
				b &= 0x001f;

				if (d32)
					table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
				else
					((UINT16*)table_ptr32)[i] = (UINT16)(r | g | b);
			}
		}
		else
		{
			for (i=0; i<32768; i++)
			{
				r = (i & 0x7c00) + dr;
				g = (i & 0x03e0) + dg;
				b = (i & 0x001f) + db;

				if (r < 0) r = 0; else if (r > 0x7c00) r = 0x7c00;
				if (g < 0) g = 0; else if (g > 0x03e0) g = 0x03e0;
				if (b < 0) b = 0; else if (b > 0x001f) b = 0x001f;

				if (d32)
					table_ptr32[i] = (UINT32)(r<<9 | g<<6 | b<<3);
				else
					((UINT16*)table_ptr32)[i] = (UINT16)(r | g | b);
			}
		}
	}
#undef FP
#undef FMAX
}


void palette_set_shadow_mode(int mode)
{
	if (mode >= 0 && mode < MAX_SHADOW_PRESETS) palette_shadow_table = (UINT16*)shadow_table_base[mode];
}


void palette_set_shadow_factor32(double factor)
{
	internal_set_shadow_preset(0, factor, 0, 0, 0, 0, 1, 0);
}


void palette_set_highlight_factor32(double factor)
{
	internal_set_shadow_preset(1, factor, 0, 0, 0, 0, 2, 0);
}


void palette_set_shadow_dRGB32(int mode, int dr, int dg, int db, int noclip)
{
	internal_set_shadow_preset(mode, 0, dr, dg, db, noclip, 0, 0);
}


void palette_set_highlight_method(int method)
{
	highlight_method = method;
}



/*-------------------------------------------------
    palette_alloc - allocate memory for palette
    structures
-------------------------------------------------*/

static int palette_alloc(void)
{
	int max_total_colors = total_colors + 2;
	int i;

	/* allocate memory for the raw game palette */
	game_palette = auto_malloc(max_total_colors * sizeof(game_palette[0]));
	for (i = 0; i < max_total_colors; i++)
		game_palette[i] = MAKE_RGB((i & 1) * 0xff, ((i >> 1) & 1) * 0xff, ((i >> 2) & 1) * 0xff);

	/* allocate memory for the adjusted game palette */
	adjusted_palette = auto_malloc(max_total_colors * sizeof(adjusted_palette[0]));
	for (i = 0; i < max_total_colors; i++)
		adjusted_palette[i] = game_palette[i];

	/* allocate memory for the dirty palette array */
	dirty_palette = auto_malloc((max_total_colors + 31) / 32 * sizeof(dirty_palette[0]));
	dirty_palette[(max_total_colors - 1) / 32] = 0; /* initialize all the bits of the last dirty entry */
	for (i = 0; i < max_total_colors; i++)
		mark_pen_dirty(i);

	/* allocate memory for the pen table */
	if (total_colors > 0)
	{
		Machine->pens = auto_malloc(total_colors * sizeof(Machine->pens[0]));
		for (i = 0; i < total_colors; i++)
			Machine->pens[i] = i;

		/* allocate memory for the per-entry brightness table */
		pen_brightness = auto_malloc(Machine->drv->total_colors * sizeof(pen_brightness[0]));
		for (i = 0; i < Machine->drv->total_colors; i++)
			pen_brightness[i] = 1 << PEN_BRIGHTNESS_BITS;
	}
	else
	{
		/* this driver does not use a palette */
		Machine->pens = NULL;
		pen_brightness = NULL;
	}

	/* allocate memory for the colortables, if needed */
	if (Machine->drv->color_table_len)
	{
		/* first for the raw colortable */
		Machine->game_colortable = auto_malloc(Machine->drv->color_table_len * sizeof(Machine->game_colortable[0]));
		for (i = 0; i < Machine->drv->color_table_len; i++)
			Machine->game_colortable[i] = i % total_colors;

		/* then for the remapped colortable */
		Machine->remapped_colortable = auto_malloc(Machine->drv->color_table_len * sizeof(Machine->remapped_colortable[0]));
	}

	/* otherwise, keep the game_colortable NULL and point the remapped_colortable to the pens */
	else
	{
		Machine->game_colortable = NULL;
		Machine->remapped_colortable = Machine->pens;	/* straight 1:1 mapping from palette to colortable */
	}

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* allocate memory for the debugger pens */
	Machine->debug_pens = auto_malloc(DEBUGGER_TOTAL_COLORS * sizeof(Machine->debug_pens[0]));
	for (i = 0; i < DEBUGGER_TOTAL_COLORS; i++)
		Machine->debug_pens[i] = i;

	/* allocate memory for the debugger colortable */
	Machine->debug_remapped_colortable = auto_malloc(2 * DEBUGGER_TOTAL_COLORS * DEBUGGER_TOTAL_COLORS * sizeof(Machine->debug_remapped_colortable[0]));
	for (i = 0; i < DEBUGGER_TOTAL_COLORS * DEBUGGER_TOTAL_COLORS; i++)
	{
		Machine->debug_remapped_colortable[2*i+0] = i / DEBUGGER_TOTAL_COLORS;
		Machine->debug_remapped_colortable[2*i+1] = i % DEBUGGER_TOTAL_COLORS;
	}
#endif

#if 0 //* for reference, do not remove
	/* allocate the shadow lookup table for 16bpp modes */
	palette_shadow_table = NULL;
	if (colormode == PALETTIZED_16BIT)
	{
		/* we allocate a full 65536 entries table, to prevent memory corruption
         * bugs should the tilemap contains pens >= total_colors
         */
		palette_shadow_table = auto_malloc(65536 * sizeof(palette_shadow_table[0]));

		/* map entries up to the total_colors so they point to the next block of colors */
		for (i = 0; i < 65536; i++)
		{
			palette_shadow_table[i] = i;
			if ((Machine->drv->video_attributes & VIDEO_HAS_SHADOWS) && i < Machine->drv->total_colors)
				palette_shadow_table[i] += Machine->drv->total_colors;
		}
	}
#else
	{
		UINT16 *table_ptr16;
		UINT32 *table_ptr32;
		int c = Machine->drv->total_colors;
		int cx2 = c << 1;

		for (i=0; i<MAX_SHADOW_PRESETS; i++) shadow_table_base[i] = NULL;

		if (!(colormode & DIRECT_RGB))
		{
			if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
			{
				table_ptr16 = auto_malloc(65536 * sizeof(UINT16));

				shadow_table_base[0] = shadow_table_base[2] = (UINT32*)table_ptr16;

				for (i=0; i<c; i++) table_ptr16[i] = c + i;
				for (i=c; i<65536; i++) table_ptr16[i] = i;

				internal_set_shadow_preset(0, PALETTE_DEFAULT_SHADOW_FACTOR32, 0, 0, 0, 0, 1, 1);
			}

			if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
			{
				table_ptr16 = auto_malloc(65536 * sizeof(UINT16));

				shadow_table_base[1] = shadow_table_base[3] = (UINT32*)table_ptr16;

				for (i=0; i<c; i++) table_ptr16[i] = cx2 + i;
				for (i=c; i<65536; i++) table_ptr16[i] = i;

				internal_set_shadow_preset(1, PALETTE_DEFAULT_HIGHLIGHT_FACTOR32, 0, 0, 0, 0, 2, 1);
			}
		}
		else
		{
			if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
			{
				table_ptr32 = auto_malloc(65536 * sizeof(UINT32));

				shadow_table_base[0] = table_ptr32;
				shadow_table_base[2] = table_ptr32 + 32768;

				internal_set_shadow_preset(0, PALETTE_DEFAULT_SHADOW_FACTOR32, 0, 0, 0, 0, 1, 1);
			}

			if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
			{
				table_ptr32 = auto_malloc(65536 * sizeof(UINT32));

				shadow_table_base[1] = table_ptr32;
				shadow_table_base[3] = table_ptr32 + 32768;

				internal_set_shadow_preset(1, PALETTE_DEFAULT_HIGHLIGHT_FACTOR32, 0, 0, 0, 0, 2, 1);
			}
		}
		palette_shadow_table = (UINT16*)shadow_table_base[0];
	}
#endif

	return 0;
}



/*-------------------------------------------------
    palette_init - palette initialization that
    takes place after the display is created
-------------------------------------------------*/

int palette_init(void)
{
	int i;

	/* recompute the default palette and initalize the color correction table */
	recompute_adjusted_palette(1);

	/* now let the driver modify the initial palette and colortable */
	if (Machine->drv->init_palette)
		(*Machine->drv->init_palette)(Machine->game_colortable, memory_region(REGION_PROMS));

	/* switch off the color mode */
	switch (colormode)
	{
		/* 16-bit paletteized case */
		case PALETTIZED_16BIT:
		{
			/* refresh the palette to support shadows in static palette games */
			for (i = 0; i < Machine->drv->total_colors; i++)
				palette_set_color(i, RGB_RED(game_palette[i]), RGB_GREEN(game_palette[i]), RGB_BLUE(game_palette[i]));

			/* map the UI pens */
			if (total_colors_with_ui <= 65534)
			{
				game_palette[total_colors + 0] = adjusted_palette[total_colors + 0] = MAKE_RGB(0x00,0x00,0x00);
				game_palette[total_colors + 1] = adjusted_palette[total_colors + 1] = MAKE_RGB(0xff,0xff,0xff);
				black_pen = total_colors_with_ui++;
				white_pen = total_colors_with_ui++;
			}
			else
			{
				game_palette[0] = adjusted_palette[0] = MAKE_RGB(0x00,0x00,0x00);
				game_palette[65535] = adjusted_palette[65535] = MAKE_RGB(0xff,0xff,0xff);
				black_pen = 0;
				white_pen = 65535;
			}
			break;
		}

		/* 15-bit direct case */
		case DIRECT_15BIT:
		{
			/* remap the game palette into direct RGB pens */
			for (i = 0; i < total_colors; i++)
				Machine->pens[i] = rgb_to_direct15(game_palette[i]);

			/* map the UI pens */
			black_pen = rgb_to_direct15(MAKE_RGB(0x00,0x00,0x00));
			white_pen = rgb_to_direct15(MAKE_RGB(0xff,0xff,0xff));
			break;
		}

		case DIRECT_32BIT:
		{
			/* remap the game palette into direct RGB pens */
			for (i = 0; i < total_colors; i++)
				Machine->pens[i] = rgb_to_direct32(game_palette[i]);

			/* map the UI pens */
			black_pen = rgb_to_direct32(MAKE_RGB(0x00,0x00,0x00));
			white_pen = rgb_to_direct32(MAKE_RGB(0xff,0xff,0xff));
			break;
		}
	}

	/* now compute the remapped_colortable */
	for (i = 0; i < Machine->drv->color_table_len; i++)
	{
		pen_t color = Machine->game_colortable[i];

		/* check for invalid colors set by Machine->drv->init_palette */
		assert(color < total_colors);
		Machine->remapped_colortable[i] = Machine->pens[color];
	}

	/* all done */
	return 0;
}



/*-------------------------------------------------
    palette_get_total_colors_with_ui - returns
    the total number of palette entries including
    UI
-------------------------------------------------*/

int palette_get_total_colors_with_ui(void)
{
	int result = Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS && !(colormode & DIRECT_RGB))
		result += Machine->drv->total_colors;
	if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS && !(colormode & DIRECT_RGB))
		result += Machine->drv->total_colors;
	if (result <= 65534)
		result += 2;
	return result;
}



/*-------------------------------------------------
    palette_update_display - update the display
    state with our latest info
-------------------------------------------------*/

void palette_update_display(mame_display *display)
{
	/* palettized case: point to the palette info */
	if (colormode == PALETTIZED_16BIT)
	{
		display->game_palette = adjusted_palette;
		display->game_palette_entries = total_colors_with_ui;
		display->game_palette_dirty = dirty_palette;

		if (adjusted_palette_dirty)
			display->changed_flags |= GAME_PALETTE_CHANGED;
	}

	/* direct case: no palette mucking */
	else
	{
		display->game_palette = NULL;
		display->game_palette_entries = 0;
		display->game_palette_dirty = NULL;
	}

	/* debugger always has a palette */
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	display->debug_palette = debugger_palette;
	display->debug_palette_entries = DEBUGGER_TOTAL_COLORS;
#endif

	/* update the dirty state */
	if (debug_palette_dirty)
		display->changed_flags |= DEBUG_PALETTE_CHANGED;

	/* clear the dirty flags */
	adjusted_palette_dirty = 0;
	debug_palette_dirty = 0;
}



/*-------------------------------------------------
    internal_modify_single_pen - change a single
    pen and recompute its adjusted RGB value
-------------------------------------------------*/

static void internal_modify_single_pen(pen_t pen, rgb_t color, int pen_bright)
{
	rgb_t adjusted_color;

	/* skip if out of bounds or not ready */
	if (pen >= total_colors)
		return;

	/* update the raw palette */
	game_palette[pen] = color;

	/* now update the adjusted color if it's different */
	adjusted_color = adjust_palette_entry(color, pen_bright);
	if (adjusted_color != adjusted_palette[pen])
	{
		/* change the adjusted palette entry */
		adjusted_palette[pen] = adjusted_color;
		adjusted_palette_dirty = 1;

		/* update the pen value or mark the palette dirty */
		switch (colormode)
		{
			/* 16-bit palettized: just mark it dirty for later */
			case PALETTIZED_16BIT:
				mark_pen_dirty(pen);
				break;

			/* 15/32-bit direct: update the Machine->pens array */
			case DIRECT_15BIT:
				Machine->pens[pen] = rgb_to_direct15(adjusted_color);
				break;

			case DIRECT_32BIT:
				Machine->pens[pen] = rgb_to_direct32(adjusted_color);
				break;
		}
	}
}



/*-------------------------------------------------
    internal_modify_pen - change a pen along with
    its corresponding shadow/highlight
-------------------------------------------------*/

static void internal_modify_pen(pen_t pen, rgb_t color, int pen_bright) //* new highlight operation
{
#define FMAX (0xff<<PEN_BRIGHTNESS_BITS)

	int r, g, b, fl, ov;

	/* first modify the base pen */
	internal_modify_single_pen(pen, color, pen_bright);

	/* see if we need to handle shadow/highlight */
	if (pen < Machine->drv->total_colors)
	{
		/* check for shadows */
		if (Machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
		{
			pen += Machine->drv->total_colors;

			if (shadow_factor > (1 << PEN_BRIGHTNESS_BITS) && highlight_method) // luminance > 1.0
			{
				r = color>>16 & 0xff;
				g = color>>8  & 0xff;
				b = color     & 0xff;

				if (highlight_method == 1)
				{
					fl = shadow_factor;

					r *= fl;  g *= fl;  b *= fl;
					ov = 0;

					if (r > FMAX) ov += r - FMAX;
					if (g > FMAX) ov += g - FMAX;
					if (b > FMAX) ov += b - FMAX;

					if (ov) { ov >>= 2;  r += ov;  g += ov;  b += ov; }

					if (r >= FMAX) r = 0xff0000; else r = (r >> PEN_BRIGHTNESS_BITS) << 16;
					if (g >= FMAX) g = 0x00ff00; else g = (g >> PEN_BRIGHTNESS_BITS) << 8;
					if (b >= FMAX) b = 0x0000ff; else b = (b >> PEN_BRIGHTNESS_BITS);
				}
				else
				{
					fl = ((shadow_factor - (1 << PEN_BRIGHTNESS_BITS)) * 255) >> PEN_BRIGHTNESS_BITS;

					r += fl;  g += fl;  b += fl;

					if (r >= 0xff) r = 0xff0000; else r <<= 16;
					if (g >= 0xff) g = 0x00ff00; else g <<= 8;
					if (b >= 0xff) b = 0x0000ff;
				}

				internal_modify_single_pen(pen, r|g|b, pen_bright);
			}
			else // luminance <= 1.0
				internal_modify_single_pen(pen, color, (pen_bright * shadow_factor) >> PEN_BRIGHTNESS_BITS);
		}

		/* check for highlights */
		if (Machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
		{
			pen += Machine->drv->total_colors;

			if (highlight_factor > (1 << PEN_BRIGHTNESS_BITS) && highlight_method) // luminance > 1.0
			{
				r = color>>16 & 0xff;
				g = color>>8  & 0xff;
				b = color     & 0xff;

				if (highlight_method == 1)
				{
					fl = highlight_factor;

					r *= fl;  g *= fl;  b *= fl;
					ov = 0;

					if (r > FMAX) ov += r - FMAX;
					if (g > FMAX) ov += g - FMAX;
					if (b > FMAX) ov += b - FMAX;

					if (ov) { ov >>= 2;  r += ov;  g += ov;  b += ov; }

					if (r >= FMAX) r = 0xff0000; else r = (r >> PEN_BRIGHTNESS_BITS) << 16;
					if (g >= FMAX) g = 0x00ff00; else g = (g >> PEN_BRIGHTNESS_BITS) << 8;
					if (b >= FMAX) b = 0x0000ff; else b = (b >> PEN_BRIGHTNESS_BITS);
				}
				else
				{
					fl = ((highlight_factor - (1 << PEN_BRIGHTNESS_BITS)) * 255) >> PEN_BRIGHTNESS_BITS;

					r += fl;  g += fl;  b += fl;

					if (r >= 0xff) r = 0xff0000; else r <<= 16;
					if (g >= 0xff) g = 0x00ff00; else g <<= 8;
					if (b >= 0xff) b = 0x0000ff;
				}

				internal_modify_single_pen(pen, r|g|b, pen_bright);
			}
			else // luminance <= 1.0
				internal_modify_single_pen(pen, color, (pen_bright * highlight_factor) >> PEN_BRIGHTNESS_BITS);
		}
	}
#undef FMAX
}



/*-------------------------------------------------
    recompute_adjusted_palette - recompute the
    entire palette after some major event
-------------------------------------------------*/

static void recompute_adjusted_palette(int brightness_or_gamma_changed)
{
	int i;

	/* regenerate the color correction table if needed */
	if (brightness_or_gamma_changed)
		for (i = 0; i < sizeof(color_correct_table); i++)
		{
			int value = (int)(255.0 * (global_brightness * global_brightness_adjust) * pow((double)i * (1.0 / 255.0), 1.0 / global_gamma) + 0.5);
			color_correct_table[i] = (value < 0) ? 0 : (value > 255) ? 255 : value;
		}

	/* now update all the palette entries */
	for (i = 0; i < Machine->drv->total_colors; i++)
		internal_modify_pen(i, game_palette[i], pen_brightness[i]);
}



/*-------------------------------------------------
    palette_reset - called after restore to
    actually update the palette
-------------------------------------------------*/

static void palette_reset(void)
{
	/* recompute everything */
	recompute_adjusted_palette(0);
}



/*-------------------------------------------------
    palette_set_color - set a single palette
    entry
-------------------------------------------------*/

void palette_set_color(pen_t pen, UINT8 r, UINT8 g, UINT8 b)
{
	/* make sure we're in range */
	if (pen >= total_colors)
	{
		logerror("error: palette_set_color() called with color %d, but only %d allocated.\n", pen, total_colors);
		return;
	}

	/* set the pen value */
	internal_modify_pen(pen, MAKE_RGB(r, g, b), pen_brightness[pen]);
}

/* handy wrapper for palette_set_color */
void palette_set_colors(pen_t color_base, const UINT8 *colors, int color_count)
{
        while(color_count--)
        {
                palette_set_color(color_base++, colors[0], colors[1], colors[2]);
                colors += 3;
        }
}

/*-------------------------------------------------
    palette_get_color - return a single palette
    entry
-------------------------------------------------*/
void palette_get_color(pen_t pen, UINT8 *r, UINT8 *g, UINT8 *b)
{
	/* record the result from the game palette */
	if (pen < total_colors)
	{
		*r = RGB_RED(game_palette[pen]);
		*g = RGB_GREEN(game_palette[pen]);
		*b = RGB_BLUE(game_palette[pen]);
	}
	/* special case the black pen */
	else if (pen == black_pen)
		*r = *g = *b = 0;
	/* special case the white pen (crosshairs) */
	else if (pen == white_pen)
		*r = *g = *b = 255;
	else
	ui_popup("palette_get_color() out of range");
}

/*-------------------------------------------------
    palette_set_brightness - set the per-pen
    brightness factor
-------------------------------------------------*/

void palette_set_brightness(pen_t pen, double bright)
{
	/* compute the integral brightness value */
	int brightval = (int)(bright * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (brightval > MAX_PEN_BRIGHTNESS)
		brightval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the array and the adjusted palette */
	if (pen_brightness[pen] != brightval)
	{
		pen_brightness[pen] = brightval;
		internal_modify_pen(pen, game_palette[pen], brightval);
	}
}



/*-------------------------------------------------
    palette_set_shadow_factor - set the global
    shadow brightness factor
-------------------------------------------------*/

void palette_set_shadow_factor(double factor)
{
	/* compute the integral shadow factor value */
	int factorval = (int)(factor * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (factorval > MAX_PEN_BRIGHTNESS)
		factorval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the entire palette */
	if (shadow_factor != factorval)
	{
		shadow_factor = factorval;
		recompute_adjusted_palette(0);
	}
}



/*-------------------------------------------------
    palette_set_highlight_factor - set the global
    highlight brightness factor
-------------------------------------------------*/

void palette_set_highlight_factor(double factor)
{
	/* compute the integral highlight factor value */
	int factorval = (int)(factor * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (factorval > MAX_PEN_BRIGHTNESS)
		factorval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the entire palette */
	if (highlight_factor != factorval)
	{
		highlight_factor = factorval;
		recompute_adjusted_palette(0);
	}
}



/*-------------------------------------------------
    palette_set_global_gamma - set the global
    gamma factor
-------------------------------------------------*/

void palette_set_global_gamma(double _gamma)
{
	/* if the gamma changed, recompute */
	if (global_gamma != _gamma)
	{
		global_gamma = _gamma;
		recompute_adjusted_palette(1);
	}
}



/*-------------------------------------------------
    palette_get_global_gamma - return the global
    gamma factor
-------------------------------------------------*/

double palette_get_global_gamma(void)
{
	return global_gamma;
}



/*-------------------------------------------------
    palette_set_global_brightness - set the global
    brightness factor
-------------------------------------------------*/

void palette_set_global_brightness(double brightness)
{
	/* if the gamma changed, recompute */
	if (global_brightness != brightness)
	{
		global_brightness = brightness;
		recompute_adjusted_palette(1);
	}
}



/*-------------------------------------------------
    palette_set_global_brightness_adjust - set
    the global brightness adjustment factor
-------------------------------------------------*/

void palette_set_global_brightness_adjust(double adjustment)
{
	/* if the gamma changed, recompute */
	if (global_brightness_adjust != adjustment)
	{
		global_brightness_adjust = adjustment;
		recompute_adjusted_palette(1);
	}
}



/*-------------------------------------------------
    palette_get_global_brightness - return the global
    brightness factor
-------------------------------------------------*/

double palette_get_global_brightness(void)
{
	return global_brightness;
}



/*-------------------------------------------------
    get_black_pen - use this if you need to
    fillbitmap() the background with black
-------------------------------------------------*/

pen_t get_black_pen(void)
{
	return black_pen;
}

pen_t get_white_pen(void)
{
	return white_pen;
}
