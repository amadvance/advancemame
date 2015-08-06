/***************************************************************************

    video.c

    Core MAME video routines.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "artwork.h"
#include "profiler.h"
#include "png.h"
#include "vidhrdw/vector.h"

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
#include "mamedbg.h"
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define FRAMES_PER_FPS_UPDATE		12

/* VERY IMPORTANT: bitmap_alloc must allocate also a "safety area" 16 pixels wide all
   around the bitmap. This is required because, for performance reasons, some graphic
   routines don't clip at boundaries of the bitmap. */
#define BITMAP_SAFETY				16



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	union
	{
		void		(*full_refresh)(void);
	} func;
};



/***************************************************************************
    GLOBALS
***************************************************************************/

/* handy globals for other parts of the system */
int vector_updates = 0;

/* main bitmap to render to */
mame_bitmap *scrbitmap[8];

/* the active video display */
static mame_display current_display;
static UINT8 visible_area_changed;
static UINT8 refresh_rate_changed;

/* video updating */
static UINT8 full_refresh_pending;
static int last_partial_scanline;

/* speed computation */
static cycles_t last_fps_time;
static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int vfcount;
static performance_info performance;

/* movie file */
static mame_file *movie_file = NULL;
static int movie_frame = 0;

/* misc other statics */
static UINT32 leds_status;
static callback_item *full_refresh_callback_list;

/* artwork callbacks */
#ifndef MESS
static artwork_callbacks mame_artwork_callbacks =
{
	NULL,
	artwork_load_artwork_file
};
#endif

#ifdef MESS
int mess_skip_this_frame;
#endif



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void video_pause(int pause);
static void video_exit(void);
static int allocate_graphics(const gfx_decode *gfxdecodeinfo);
static void decode_graphics(const gfx_decode *gfxdecodeinfo);
static void compute_aspect_ratio(const machine_config *drv, int *aspect_x, int *aspect_y);
static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height);
static int init_buffered_spriteram(void);
static void recompute_fps(int skipped_it);



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    video_init - start up the video system
-------------------------------------------------*/

int video_init(void)
{
	osd_create_params params;
	artwork_callbacks *artcallbacks;
	int bmwidth = Machine->drv->screen_width;
	int bmheight = Machine->drv->screen_height;

	movie_file = NULL;
	full_refresh_callback_list = NULL;
	movie_frame = 0;

	add_pause_callback(video_pause);
	add_exit_callback(video_exit);

	/* first allocate the necessary palette structures */
	if (palette_start())
		return 1;

#ifndef NEW_RENDER
	/* if we're a vector game, override the screen width and height */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* compute the visible area for raster games */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		params.width = Machine->drv->default_visible_area.max_x - Machine->drv->default_visible_area.min_x + 1;
		params.height = Machine->drv->default_visible_area.max_y - Machine->drv->default_visible_area.min_y + 1;
	}
	else
	{
		params.width = bmwidth;
		params.height = bmheight;
	}

	/* fill in the rest of the display parameters */
	compute_aspect_ratio(Machine->drv, &params.aspect_x, &params.aspect_y);
	params.depth = Machine->color_depth;
	params.colors = palette_get_total_colors_with_ui();
	params.fps = Machine->drv->frames_per_second;
	params.video_attributes = Machine->drv->video_attributes;

#ifdef MESS
	artcallbacks = &mess_artwork_callbacks;
#else
	artcallbacks = &mame_artwork_callbacks;
#endif

	/* initialize the display through the artwork (and eventually the OSD) layer */
	if (artwork_create_display(&params, direct_rgb_components, artcallbacks))
		return 1;

	/* the create display process may update the vector width/height, so recompute */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* now allocate the screen bitmap */
	scrbitmap[0] = auto_bitmap_alloc_depth(bmwidth, bmheight, Machine->color_depth);
	if (!scrbitmap)
		return 1;
#endif

	/* set the default refresh rate */
	set_refresh_rate(Machine->drv->frames_per_second);

	/* set the default visible area */
	set_visible_area(0,1,0,1);	// make sure everything is recalculated on multiple runs
	set_visible_area(
			Machine->drv->default_visible_area.min_x,
			Machine->drv->default_visible_area.max_x,
			Machine->drv->default_visible_area.min_y,
			Machine->drv->default_visible_area.max_y);

	/* create spriteram buffers if necessary */
	if (Machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		if (init_buffered_spriteram())
			return 1;

#ifndef NEW_RENDER
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* if the debugger is enabled, initialize its bitmap and font */
	if (Machine->debug_mode)
	{
		int depth = options.debug_depth ? options.debug_depth : Machine->color_depth;

		/* first allocate the debugger bitmap */
		Machine->debug_bitmap = auto_bitmap_alloc_depth(options.debug_width, options.debug_height, depth);
		if (!Machine->debug_bitmap)
			return 1;

		/* then create the debugger font */
		Machine->debugger_font = build_debugger_font();
		if (Machine->debugger_font == NULL)
			return 1;
	}
#endif
#endif

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the Machine->gfx[] data */
	if (Machine->drv->gfxdecodeinfo)
		if (allocate_graphics(Machine->drv->gfxdecodeinfo))
			return 1;

	/* initialize the palette - must be done after osd_create_display() */
	if (palette_init())
		return 1;

	/* force the first update to be full */
	set_vh_global_attribute(NULL, 0);

	/* actually decode the graphics */
	if (Machine->drv->gfxdecodeinfo)
		decode_graphics(Machine->drv->gfxdecodeinfo);

	/* reset performance data */
	last_fps_time = osd_cycles();
	rendered_frames_since_last_fps = frames_since_last_fps = 0;
	performance.game_speed_percent = 100;
	performance.frames_per_second = Machine->refresh_rate;
	performance.vector_updates_last_second = 0;

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	/* initialize tilemaps */
	if (tilemap_init() != 0)
		fatalerror("tilemap_init failed");
	return 0;
}


/*-------------------------------------------------
    video_pause - pause the video system
-------------------------------------------------*/

static void video_pause(int pause)
{
	palette_set_global_brightness_adjust(pause ? options.pause_bright : 1.00);
	schedule_full_refresh();
}


/*-------------------------------------------------
    video_exit - close down the video system
-------------------------------------------------*/

static void video_exit(void)
{
	int i;

	/* stop recording any movie */
	record_movie_stop();

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* free the font elements */
	if (Machine->debugger_font)
	{
		freegfx(Machine->debugger_font);
		Machine->debugger_font = NULL;
	}
#endif

	/* close down the OSD layer's display */
	osd_close_display();
}


/*-------------------------------------------------
    compute_aspect_ratio - determine the aspect
    ratio encoded in the video attributes
-------------------------------------------------*/

static void compute_aspect_ratio(const machine_config *drv, int *aspect_x, int *aspect_y)
{
	/* if it's explicitly specified, use it */
	if (drv->aspect_x && drv->aspect_y)
	{
		*aspect_x = drv->aspect_x;
		*aspect_y = drv->aspect_y;
	}

	/* otherwise, attempt to deduce the result */
	else
	{
		*aspect_x = 4;
		*aspect_y = 3;
	}
}


/*-------------------------------------------------
    allocate_graphics - allocate memory for the
    graphics
-------------------------------------------------*/

static int allocate_graphics(const gfx_decode *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT32 extxoffs[MAX_ABS_GFX_SIZE], extyoffs[MAX_ABS_GFX_SIZE];
		UINT32 *xoffset, *yoffset;
		gfx_layout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;
		if (glcopy.extxoffs)
		{
			memcpy(extxoffs, glcopy.extxoffs, glcopy.width * sizeof(extxoffs[0]));
			glcopy.extxoffs = extxoffs;
		}
		if (glcopy.extyoffs)
		{
			memcpy(extyoffs, glcopy.extyoffs, glcopy.height * sizeof(extyoffs[0]));
			glcopy.extyoffs = extyoffs;
		}

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* loop over all the planes, converting fractions */
		for (j = 0; j < glcopy.planes; j++)
		{
			UINT32 value = glcopy.planeoffset[j];
			if (IS_FRAC(value))
				glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		/* loop over all the X/Y offsets, converting fractions */
		xoffset = glcopy.extxoffs ? extxoffs : glcopy.xoffset;
		for (j = 0; j < glcopy.width; j++)
		{
			UINT32 value = xoffset[j];
			if (IS_FRAC(value))
				xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		yoffset = glcopy.extyoffs ? extyoffs : glcopy.yoffset;
		for (j = 0; j < glcopy.height; j++)
		{
			UINT32 value = yoffset[j];
			if (IS_FRAC(value))
				yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		/* some games increment on partial tile boundaries; to handle this without reading */
		/* past the end of the region, we may need to truncate the count */
		/* an example is the games in metro.c */
		if (glcopy.planeoffset[0] == GFX_RAW)
		{
			int base = gfxdecodeinfo[i].start;
			int end = region_length/8;
			while (glcopy.total > 0)
			{
				int elementbase = base + (glcopy.total - 1) * glcopy.charincrement / 8;
				int lastpixelbase = elementbase + glcopy.height * yoffset[0] / 8 - 1;
				if (lastpixelbase < end)
					break;
				glcopy.total--;
			}
		}

		/* allocate the graphics */
		Machine->gfx[i] = allocgfx(&glcopy);

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
	return 0;
}


/*-------------------------------------------------
    decode_graphics - decode the graphics
-------------------------------------------------*/

static void decode_graphics(const gfx_decode *gfxdecodeinfo)
{
	int totalgfx = 0, curgfx = 0;
	int i;

	/* count total graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
			totalgfx += Machine->gfx[i]->total_elements;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
		{
			/* if we have a valid region, decode it now */
			if (gfxdecodeinfo[i].memory_region > REGION_INVALID)
			{
				UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
				gfx_element *gfx = Machine->gfx[i];
				int j;

				/* now decode the actual graphics */
				for (j = 0; j < gfx->total_elements; j += 1024)
				{
					int num_to_decode = (j + 1024 < gfx->total_elements) ? 1024 : (gfx->total_elements - j);
					decodegfx(gfx, region_base + gfxdecodeinfo[i].start, j, num_to_decode);
					curgfx += num_to_decode;
		/*          ui_display_decoding(artwork_get_ui_bitmap(), curgfx * 100 / totalgfx);*/
				}
			}

			/* otherwise, clear the target region */
			else
				memset(Machine->gfx[i]->gfxdata, 0, Machine->gfx[i]->char_modulo * Machine->gfx[i]->total_elements);
		}
}


/*-------------------------------------------------
    scale_vectorgames - scale the vector games
    to a given resolution
-------------------------------------------------*/

static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height)
{
	double x_scale, y_scale, scale;

	/* compute the scale values */
	x_scale = (double)gfx_width  / *width;
	y_scale = (double)gfx_height / *height;

	/* pick the smaller scale factor */
	scale = (x_scale < y_scale) ? x_scale : y_scale;

	/* compute the new size */
	*width  = *width  * scale + 0.5;
	*height = *height * scale + 0.5;

	/* round to the nearest 4 pixel value */
	*width  &= ~3;
	*height &= ~3;
}


/*-------------------------------------------------
    init_buffered_spriteram - initialize the
    double-buffered spriteram
-------------------------------------------------*/

static int init_buffered_spriteram(void)
{
	/* make sure we have a valid size */
	if (spriteram_size == 0)
	{
		logerror("video_init():  Video buffers spriteram but spriteram_size is 0\n");
		return 0;
	}

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);

	/* register for saving it */
	state_save_register_global_pointer(buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);

		/* register for saving it */
		state_save_register_global_pointer(buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (UINT16 *)buffered_spriteram;
	buffered_spriteram32 = (UINT32 *)buffered_spriteram;
	buffered_spriteram16_2 = (UINT16 *)buffered_spriteram_2;
	buffered_spriteram32_2 = (UINT32 *)buffered_spriteram_2;
	return 0;
}



/***************************************************************************

    Screen rendering and management.

***************************************************************************/

/*-------------------------------------------------
    set_visible_area - adjusts the visible portion
    of the bitmap area dynamically
-------------------------------------------------*/

void set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
	if (       Machine->visible_area.min_x == min_x
			&& Machine->visible_area.max_x == max_x
			&& Machine->visible_area.min_y == min_y
			&& Machine->visible_area.max_y == max_y)
		return;

	/* "dirty" the area for the next display update */
	visible_area_changed = 1;

	/* bounds check */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) && scrbitmap[0])
		if ((min_x < 0) || (min_y < 0) || (max_x >= scrbitmap[0]->width) || (max_y >= scrbitmap[0]->height))
		{
			fatalerror("set_visible_area(%d,%d,%d,%d) out of bounds; bitmap dimensions are (%d,%d)",
				min_x, min_y, max_x, max_y,
				scrbitmap[0]->width, scrbitmap[0]->height);
		}

	/* set the new values in the Machine struct */
	Machine->visible_area.min_x = min_x;
	Machine->visible_area.max_x = max_x;
	Machine->visible_area.min_y = min_y;
	Machine->visible_area.max_y = max_y;

	/* vector games always use the whole bitmap */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		Machine->absolute_visible_area.min_x = 0;
		Machine->absolute_visible_area.max_x = scrbitmap[0]->width - 1;
		Machine->absolute_visible_area.min_y = 0;
		Machine->absolute_visible_area.max_y = scrbitmap[0]->height - 1;
	}

	/* raster games need to use the visible area */
	else
		Machine->absolute_visible_area = Machine->visible_area;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();

	/* set UI visible area */
	ui_set_visible_area(Machine->absolute_visible_area.min_x,
						Machine->absolute_visible_area.min_y,
						Machine->absolute_visible_area.max_x,
						Machine->absolute_visible_area.max_y);
}


/*-------------------------------------------------
    set_refresh_rate - adjusts the refresh rate
    of the video mode dynamically
-------------------------------------------------*/

void set_refresh_rate(float fps)
{
	/* bail if already equal */
	if (Machine->refresh_rate == fps)
		return;

	/* "dirty" the rate for the next display update */
	refresh_rate_changed = 1;

	/* set the new values in the Machine struct */
	Machine->refresh_rate = fps;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}


/*-------------------------------------------------
    schedule_full_refresh - force a full erase
    and refresh the next frame
-------------------------------------------------*/

void schedule_full_refresh(void)
{
	full_refresh_pending = 1;
}


/*-------------------------------------------------
    reset_partial_updates - reset the partial
    updating mechanism for a new frame
-------------------------------------------------*/

void reset_partial_updates(void)
{
	last_partial_scanline = 0;
	performance.partial_updates_this_frame = 0;
}


/*-------------------------------------------------
    force_partial_update - perform a partial
    update from the last scanline up to and
    including the specified scanline
-------------------------------------------------*/

void force_partial_update(int scanline)
{
	rectangle clip = Machine->visible_area;
	callback_item *cb;

	/* if skipping this frame, bail */
	if (osd_skip_this_frame())
		return;

	/* skip if less than the lowest so far */
	if (scanline < last_partial_scanline)
		return;

	/* if there's a dirty bitmap and we didn't do any partial updates yet, handle it now */
	if (full_refresh_pending && last_partial_scanline == 0)
	{
		fillbitmap(scrbitmap[0], get_black_pen(), NULL);
		for (cb = full_refresh_callback_list; cb; cb = cb->next)
			(*cb->func.full_refresh)();
		full_refresh_pending = 0;
	}

	/* set the start/end scanlines */
	if (last_partial_scanline > clip.min_y)
		clip.min_y = last_partial_scanline;
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		profiler_mark(PROFILER_VIDEO);
#ifdef MESS
		{
			int update_says_skip = 0;
			(*Machine->drv->video_update)(0, scrbitmap[0], &clip, &update_says_skip);
			if (!update_says_skip)
				mess_skip_this_frame = 0;
			else if (mess_skip_this_frame == -1)
				mess_skip_this_frame = 1;
		}
#else
		(*Machine->drv->video_update)(0, scrbitmap[0], &clip);
#endif
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);
	}

	/* remember where we left off */
	last_partial_scanline = scanline + 1;
}


/*-------------------------------------------------
    add_full_refresh_callback - request callback on
	full refesh
-------------------------------------------------*/

void add_full_refresh_callback(void (*callback)(void))
{
	callback_item *cb, **cur;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call add_full_refresh_callback at init time!");

	cb = auto_malloc(sizeof(*cb));
	cb->func.full_refresh = callback;
	cb->next = NULL;

	for (cur = &full_refresh_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    draw_screen - render the final screen bitmap
    and update any artwork
-------------------------------------------------*/

void draw_screen(void)
{
	/* finish updating the screen */
	force_partial_update(Machine->visible_area.max_y);
}


/*-------------------------------------------------
    update_video_and_audio - actually call the
    OSD layer to perform an update
-------------------------------------------------*/

void update_video_and_audio(void)
{
	int skipped_it = osd_skip_this_frame();

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	debug_trace_delay = 0;
#endif

	/* fill in our portion of the display */
	current_display.changed_flags = 0;

#ifdef MESS
	if (mess_skip_this_frame == 1)
		current_display.changed_flags |= GAME_OPTIONAL_FRAMESKIP;
	mess_skip_this_frame = -1;
#endif /* MESS */

	/* set the main game bitmap */
	current_display.game_bitmap = scrbitmap[0];
	current_display.game_bitmap_update = Machine->absolute_visible_area;
	if (!skipped_it)
		current_display.changed_flags |= GAME_BITMAP_CHANGED;

	/* set the visible area */
	current_display.game_visible_area = Machine->absolute_visible_area;
	if (visible_area_changed)
		current_display.changed_flags |= GAME_VISIBLE_AREA_CHANGED;

	/* set the refresh rate */
	current_display.game_refresh_rate = Machine->refresh_rate;
	if (refresh_rate_changed)
		current_display.changed_flags |= GAME_REFRESH_RATE_CHANGED;

	/* set the vector dirty list */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (!full_refresh_pending && !ui_is_dirty() && !skipped_it)
		{
			current_display.vector_dirty_pixels = vector_dirty_list;
			current_display.changed_flags |= VECTOR_PIXELS_CHANGED;
		}

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* set the debugger bitmap */
	current_display.debug_bitmap = Machine->debug_bitmap;
	if (debugger_bitmap_changed)
		current_display.changed_flags |= DEBUG_BITMAP_CHANGED;
	debugger_bitmap_changed = 0;

	/* adjust the debugger focus */
	if (debugger_focus != current_display.debug_focus)
	{
		current_display.debug_focus = debugger_focus;
		current_display.changed_flags |= DEBUG_FOCUS_CHANGED;
	}
#endif

	/* set the LED status */
	if (leds_status != current_display.led_state)
	{
		current_display.led_state = leds_status;
		current_display.changed_flags |= LED_STATE_CHANGED;
	}

	/* update with data from other parts of the system */
	palette_update_display(&current_display);

	/* render */
	artwork_update_video_and_audio(&current_display);

	/* update FPS */
	recompute_fps(skipped_it);

	/* reset dirty flags */
	visible_area_changed = 0;
	refresh_rate_changed = 0;
}


/*-------------------------------------------------
    recompute_fps - recompute the frame rate
-------------------------------------------------*/

static void recompute_fps(int skipped_it)
{
	/* increment the frame counters */
	frames_since_last_fps++;
	if (!skipped_it)
		rendered_frames_since_last_fps++;

	/* if we didn't skip this frame, we may be able to compute a new FPS */
	if (!skipped_it && frames_since_last_fps >= FRAMES_PER_FPS_UPDATE)
	{
		cycles_t cps = osd_cycles_per_second();
		cycles_t curr = osd_cycles();
		double seconds_elapsed = (double)(curr - last_fps_time) * (1.0 / (double)cps);
		double frames_per_sec = (double)frames_since_last_fps / seconds_elapsed;

		/* compute the performance data */
		performance.game_speed_percent = 100.0 * frames_per_sec / Machine->refresh_rate;
		performance.frames_per_second = (double)rendered_frames_since_last_fps / seconds_elapsed;

		/* reset the info */
		last_fps_time = curr;
		frames_since_last_fps = 0;
		rendered_frames_since_last_fps = 0;
	}

	/* for vector games, compute the vector update count once/second */
	vfcount++;
	if (vfcount >= (int)Machine->refresh_rate)
	{
		performance.vector_updates_last_second = vector_updates;
		vector_updates = 0;

		vfcount -= (int)Machine->refresh_rate;
	}
}


/*-------------------------------------------------
    updatescreen - handle frameskipping and UI,
    plus updating the screen during normal
    operations
-------------------------------------------------*/

void updatescreen(void)
{
	/* update sound */
	sound_frame_update();

	/* if we're not skipping this frame, draw the screen */
	if (!osd_skip_this_frame())
	{
		profiler_mark(PROFILER_VIDEO);
		draw_screen();
		profiler_mark(PROFILER_END);
	}

	/* the user interface must be called between vh_update() and osd_update_video_and_audio(), */
	/* to allow it to overlay things on the game display. We must call it even */
	/* if the frame is skipped, to keep a consistent timing. */
	ui_update_and_render(artwork_get_ui_bitmap());

	/* update our movie recording state */
	if (!mame_is_paused())
		record_movie_frame(scrbitmap[0]);

	/* blit to the screen */
	update_video_and_audio();

	/* call the end-of-frame callback */
	if (Machine->drv->video_eof && !mame_is_paused())
	{
		profiler_mark(PROFILER_VIDEO);
		(*Machine->drv->video_eof)();
		profiler_mark(PROFILER_END);
	}
}


/*-------------------------------------------------
    skip_this_frame -
-------------------------------------------------*/

int skip_this_frame(void)
{
	return osd_skip_this_frame();
}



/*-------------------------------------------------
    mame_get_performance_info - return performance
    info
-------------------------------------------------*/

const performance_info *mame_get_performance_info(void)
{
	return &performance;
}




/***************************************************************************

    Screen snapshot and movie recording code

***************************************************************************/

/*-------------------------------------------------
    save_frame_with - save a frame with a
    given handler for screenshots and movies
-------------------------------------------------*/

static void save_frame_with(mame_file *fp, mame_bitmap *bitmap, int (*write_handler)(mame_file *, mame_bitmap *))
{
	rectangle bounds;
	mame_bitmap *osdcopy;
	UINT32 saved_rgb_components[3];

	/* allow the artwork system to override certain parameters */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		bounds.min_x = 0;
		bounds.max_x = bitmap->width - 1;
		bounds.min_y = 0;
		bounds.max_y = bitmap->height - 1;
	}
	else
	{
		bounds = Machine->visible_area;
	}
	memcpy(saved_rgb_components, direct_rgb_components, sizeof(direct_rgb_components));
	artwork_override_screenshot_params(&bitmap, &bounds, direct_rgb_components);

	/* allow the OSD system to muck with the screenshot */
	osdcopy = osd_override_snapshot(bitmap, &bounds);
	if (osdcopy)
		bitmap = osdcopy;

	/* now do the actual work */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		write_handler(fp, bitmap);
	}
	else
	{
		mame_bitmap *copy;
		int sizex, sizey, scalex, scaley;

		sizex = bounds.max_x - bounds.min_x + 1;
		sizey = bounds.max_y - bounds.min_y + 1;

		scalex = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_2_1) ? 2 : 1;
		scaley = (Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_1_2) ? 2 : 1;

		if(Machine->gamedrv->flags & ORIENTATION_SWAP_XY)
		{
			int temp;

			temp = scalex;
			scalex = scaley;
			scaley = temp;
		}

		copy = bitmap_alloc_depth(sizex * scalex,sizey * scaley,bitmap->depth);
		if (copy)
		{
			int x,y,sx,sy;

			sx = bounds.min_x;
			sy = bounds.min_y;

			switch (bitmap->depth)
			{
			case 8:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT8 *)copy->line[y])[x] = ((UINT8 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 15:
			case 16:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT16 *)copy->line[y])[x] = ((UINT16 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			case 32:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT32 *)copy->line[y])[x] = ((UINT32 *)bitmap->line[sy+(y/scaley)])[sx +(x/scalex)];
					}
				}
				break;
			default:
				logerror("Unknown color depth\n");
				break;
			}
			write_handler(fp, copy);
			bitmap_free(copy);
		}
	}
	memcpy(direct_rgb_components, saved_rgb_components, sizeof(saved_rgb_components));

	/* if the OSD system allocated a bitmap; free it */
	if (osdcopy)
		bitmap_free(osdcopy);
}


 /*-------------------------------------------------
    save_screen_snapshot_as - save a snapshot to
    the given file handle
-------------------------------------------------*/

void save_screen_snapshot_as(mame_file *fp, mame_bitmap *bitmap)
{
	save_frame_with(fp, bitmap, png_write_bitmap);
}


/*-------------------------------------------------
    open the next non-existing file of type
    filetype according to our numbering scheme
-------------------------------------------------*/

static mame_file *mame_fopen_next(int filetype)
{
	char name[FILENAME_MAX];
	int seq;

	/* avoid overwriting existing files */
	/* first of all try with "gamename.xxx" */
	sprintf(name,"%.8s", Machine->gamedrv->name);
	if (mame_faccess(name, filetype))
	{
		seq = 0;
		do
		{
			/* otherwise use "nameNNNN.xxx" */
			sprintf(name,"%.4s%04d",Machine->gamedrv->name, seq++);
		} while (mame_faccess(name, filetype));
	}

    return (mame_fopen(Machine->gamedrv->name, name, filetype, 1));
}


/*-------------------------------------------------
    save_screen_snapshot - save a snapshot.
-------------------------------------------------*/

void save_screen_snapshot(mame_bitmap *bitmap)
{
	mame_file *fp;

	if ((fp = mame_fopen_next(FILETYPE_SCREENSHOT)) != NULL)
	{
		save_screen_snapshot_as(fp, bitmap);
		mame_fclose(fp);
	}
}


/*-------------------------------------------------
    record_movie - start, stop and update the
    recording of a MNG movie
-------------------------------------------------*/

void record_movie_start(const char *name)
{
	if (movie_file != NULL)
		mame_fclose(movie_file);

	if (name)
		movie_file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MOVIE, 1);
	else
		movie_file = mame_fopen_next(FILETYPE_MOVIE);

	movie_frame = 0;
}


void record_movie_stop(void)
{
	if (movie_file)
	{
		mng_capture_stop(movie_file);
		mame_fclose(movie_file);
		movie_file = NULL;
	}
}


void record_movie_toggle(void)
{
	if (movie_file == NULL)
	{
		record_movie_start(NULL);
		if (movie_file)
			ui_popup("REC START");
	}
	else
	{
		record_movie_stop();
		ui_popup("REC STOP (%d frames)", movie_frame);
	}
}


void record_movie_frame(mame_bitmap *bitmap)
{
	if (movie_file != NULL && bitmap != NULL)
	{
		profiler_mark(PROFILER_MOVIE_REC);

		if (movie_frame++ == 0)
			save_frame_with(movie_file, bitmap, mng_capture_start);
		save_frame_with(movie_file, bitmap, mng_capture_frame);

		profiler_mark(PROFILER_END);
	}
}



/***************************************************************************

    Bitmap allocation/freeing code

***************************************************************************/

/*-------------------------------------------------
    pp_* -- pixel plotting callbacks
-------------------------------------------------*/

static void pp_8 (mame_bitmap *b, int x, int y, pen_t p)  { ((UINT8 *)b->line[y])[x] = p; }
static void pp_16(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT16 *)b->line[y])[x] = p; }
static void pp_32(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT32 *)b->line[y])[x] = p; }


/*-------------------------------------------------
    rp_* -- pixel reading callbacks
-------------------------------------------------*/

static pen_t rp_8 (mame_bitmap *b, int x, int y)  { return ((UINT8 *)b->line[y])[x]; }
static pen_t rp_16(mame_bitmap *b, int x, int y)  { return ((UINT16 *)b->line[y])[x]; }
static pen_t rp_32(mame_bitmap *b, int x, int y)  { return ((UINT32 *)b->line[y])[x]; }


/*-------------------------------------------------
    pb_* -- box plotting callbacks
-------------------------------------------------*/

static void pb_8 (mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT8 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_16(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT16 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_32(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT32 *)b->line[y])[x] = p; x++; } y++; } }


/*-------------------------------------------------
    bitmap_alloc_core
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	mame_bitmap *bitmap;

	/* obsolete kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
		depth = -depth;

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(mame_bitmap)) : malloc(sizeof(mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		UINT8 *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(UINT8 *);

		/* align to 16 bytes */
		linearraysize = (linearraysize + 15) & ~15;

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (UINT8 *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		if (pixelsize == 1)
		{
			bitmap->read = rp_8;
			bitmap->plot = pp_8;
			bitmap->plot_box = pb_8;
		}
		else if (pixelsize == 2)
		{
			bitmap->read = rp_16;
			bitmap->plot = pp_16;
			bitmap->plot_box = pb_16;
		}
		else
		{
			bitmap->read = rp_32;
			bitmap->plot = pp_32;
			bitmap->plot_box = pb_32;
		}
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
    bitmap_alloc_depth - allocate a bitmap for a
    specific depth
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, FALSE);
}


/*-------------------------------------------------
    auto_bitmap_alloc_depth - allocate a bitmap
    for a specific depth
-------------------------------------------------*/

mame_bitmap *auto_bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, TRUE);
}


/*-------------------------------------------------
    bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}



