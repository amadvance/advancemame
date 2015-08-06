/***************************************************************************

    video.h

    Core MAME video routines.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include "mamecore.h"


/***************************************************************************

    Display state passed to the OSD layer for rendering

***************************************************************************/

/* these flags are set in the mame_display struct to indicate that */
/* a particular piece of state has changed since the last call to */
/* osd_update_video_and_audio() */
#define GAME_BITMAP_CHANGED			0x00000001
#define GAME_PALETTE_CHANGED		0x00000002
#define GAME_VISIBLE_AREA_CHANGED	0x00000004
#define VECTOR_PIXELS_CHANGED		0x00000008
#define DEBUG_BITMAP_CHANGED		0x00000010
#define DEBUG_PALETTE_CHANGED		0x00000020
#define DEBUG_FOCUS_CHANGED			0x00000040
#define LED_STATE_CHANGED			0x00000080
#define GAME_REFRESH_RATE_CHANGED	0x00000100
#ifdef MESS
#define GAME_OPTIONAL_FRAMESKIP     0x00000200
#endif /* MESS */


/* the main mame_display structure, containing the current state of the */
/* video display */
struct _mame_display
{
	/* bitfield indicating which states have changed */
	UINT32			changed_flags;

	/* game bitmap and display information */
	mame_bitmap *	game_bitmap;				/* points to game's bitmap */
	rectangle		game_bitmap_update;			/* bounds that need to be updated */
	const rgb_t *	game_palette;				/* points to game's adjusted palette */
	UINT32			game_palette_entries;		/* number of palette entries in game's palette */
	UINT32 *		game_palette_dirty;			/* points to game's dirty palette bitfield */
	rectangle 		game_visible_area;			/* the game's visible area */
	float			game_refresh_rate;			/* refresh rate */
	void *			vector_dirty_pixels;		/* points to X,Y pairs of dirty vector pixels */

	/* debugger bitmap and display information */
	mame_bitmap *	debug_bitmap;				/* points to debugger's bitmap */
	const rgb_t *	debug_palette;				/* points to debugger's palette */
	UINT32			debug_palette_entries;		/* number of palette entries in debugger's palette */
	UINT8			debug_focus;				/* set to 1 if debugger has focus */

	/* other misc information */
	UINT8			led_state;					/* bitfield of current LED states */
};
/* in mamecore.h: typedef struct _mame_display mame_display; */



/***************************************************************************

    Performance data

***************************************************************************/

struct _performance_info
{
	double			game_speed_percent;			/* % of full speed */
	double			frames_per_second;			/* actual rendered fps */
	int				vector_updates_last_second; /* # of vector updates last second */
	int				partial_updates_this_frame; /* # of partial updates last frame */
};
/* In mamecore.h: typedef struct _performance_info performance_info; */



/***************************************************************************

    Function prototypes

***************************************************************************/

/* ----- screen rendering and management ----- */

int video_init(void);

/* set the current visible area of the screen bitmap */
void set_visible_area(int min_x, int max_x, int min_y, int max_y);

/* set the current refresh rate of the video mode */
void set_refresh_rate(float fps);

/* force an erase and a complete redraw of the video next frame */
void schedule_full_refresh(void);

/* called by cpuexec.c to reset updates at the end of VBLANK */
void reset_partial_updates(void);

/* force a partial update of the screen up to and including the requested scanline */
void force_partial_update(int scanline);

/* finish updating the screen for this frame */
void draw_screen(void);

/* update the video by calling down to the OSD layer */
void update_video_and_audio(void);

/* update the screen, handling frame skipping and rendering */
/* (this calls draw_screen and update_video_and_audio) */
void updatescreen(void);

/* can we skip this frame? */
int skip_this_frame(void);

/* return current performance data */
const performance_info *mame_get_performance_info(void);

/* request callback on full refresh */
void add_full_refresh_callback(void (*callback)(void));

/*
  Save a screen shot of the game display. It is suggested to use the core
  function save_screen_snapshot() or save_screen_snapshot_as(), so the format
  of the screen shots will be consistent across ports. This hook is provided
  only to allow the display of a file requester to let the user choose the
  file name. This isn't scrictly necessary, so you can just call
  save_screen_snapshot() to let the core automatically pick a default name.
*/
void save_screen_snapshot_as(mame_file *fp, mame_bitmap *bitmap);
void save_screen_snapshot(mame_bitmap *bitmap);

/* Movie recording */
void record_movie_start(const char *name);
void record_movie_stop(void);
void record_movie_toggle(void);
void record_movie_frame(mame_bitmap *bitmap);

/* bitmap allocation */
#define bitmap_alloc(w,h) bitmap_alloc_depth(w, h, Machine->color_depth)
#define auto_bitmap_alloc(w,h) auto_bitmap_alloc_depth(w, h, Machine->color_depth)
mame_bitmap *bitmap_alloc_depth(int width, int height, int depth);
mame_bitmap *auto_bitmap_alloc_depth(int width, int height, int depth);
void bitmap_free(mame_bitmap *bitmap);


#endif	/* __VIDEO_H__ */
