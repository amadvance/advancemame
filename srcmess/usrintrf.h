/***************************************************************************

    usrintrf.h

    Functions used to handle MAME's crude user interface.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __USRINTRF_H__
#define __USRINTRF_H__

#include "mamecore.h"


/*************************************
 *
 *  Constants
 *
 *************************************/

/* justification options for ui_draw_text_full */
enum
{
	JUSTIFY_LEFT = 0,
	JUSTIFY_CENTER,
	JUSTIFY_RIGHT
};

/* word wrapping options for ui_draw_text_full */
enum
{
	WRAP_NEVER,
	WRAP_TRUNCATE,
	WRAP_WORD
};

/* drawing options for ui_draw_text_full */
enum
{
	DRAW_NONE,
	DRAW_NORMAL,
	DRAW_OPAQUE
};

/* pause options for ui_set_pause */
enum
{
	PAUSE_OFF = 0,
	PAUSE_ON = 1,
	PAUSE_SINGLE_STEP = 2
};

/* flags for menu items */
#define MENU_FLAG_LEFT_ARROW   (1 << 0)
#define MENU_FLAG_RIGHT_ARROW  (1 << 1)
#define MENU_FLAG_INVERT       (1 << 2)



/*************************************
 *
 *  Type definitions
 *
 *************************************/

typedef UINT32 (*ui_menu_handler)(UINT32 state);

typedef struct _ui_menu_item ui_menu_item;
struct _ui_menu_item
{
   const char *text;
   const char *subtext;
   UINT32 flags;
   void *ref;
};



/*************************************
 *
 *  Global variables
 *
 *************************************/

/* main init/exit routines */
int ui_init(int show_disclaimer, int show_warnings, int show_gameinfo);
void ui_exit(void);

/* once-per-frame update and render */
void ui_update_and_render(mame_bitmap *bitmap);

/* returns non-zero if the UI has been drawn recently */
int ui_is_dirty(void);

/* called by the OSD layer to set the UI area of the screen */
void ui_set_visible_area(int xmin, int ymin, int xmax, int ymax);

/* returns the line height of the font used by the UI system */
int ui_get_line_height(void);

/* returns the width of a character or string in the UI font */
int ui_get_char_width(UINT16 ch);
int ui_get_string_width(const char *s);

/* returns the current width/height of the UI rendering area */
void ui_get_bounds(int *width, int *height);

/* simple text draw at the given coordinates */
void ui_draw_text(const char *buf, int x, int y);

/* full-on text draw with all the options */
void ui_draw_text_full(const char *buf, int x, int y, int wrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, int *totalwidth, int *totalheight);

/* draw a multi-line message centered with a box around it */
void ui_draw_message_window(const char *text);

/* menu rendering system */
void ui_draw_menu(const ui_menu_item *items, int numitems, int selected);

/* menu keyboard handling */
int ui_menu_generic_keys(int *selected, int num_items);

/* menu stack management */
void ui_menu_stack_reset(void);
UINT32 ui_menu_stack_push(ui_menu_handler new_handler, UINT32 new_state);
UINT32 ui_menu_stack_pop(void);

/* display a temporary message at the bottom of the screen */
void CLIB_DECL ui_popup(const char *text, ...) ATTR_PRINTF(1,2);
void CLIB_DECL ui_popup_time(int seconds, const char *text, ...) ATTR_PRINTF(2,3);

/* informational displays used before the game is fully up and running */
int ui_display_decoding(mame_bitmap *bitmap, int percent);
int ui_display_copyright(mame_bitmap *bitmap);
int ui_display_game_warnings(mame_bitmap *bitmap);
int ui_display_game_info(mame_bitmap *bitmap);

/* temporarily show the FPS display for a period of time */
void ui_show_fps_temp(double seconds);

/* get/set whether or not the FPS is displayed */
void ui_set_show_fps(int show);
int ui_get_show_fps(void);

/* get/set whether or not the profiler is displayed */
void ui_set_show_profiler(int show);
int ui_get_show_profiler(void);

/* return true if a menu is displayed */
int ui_is_setup_active(void);

/* return true if the on-screen thermometers are displayed */
int ui_is_onscrd_active(void);

/* renders the fps counter */
void ui_display_fps(void);

#endif	/* __USRINTRF_H__ */
