// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*********************************************************************

    artwork.c

    Second generation artwork implementation.

**********************************************************************

    Still to do:
        - tinting
        - mechanism to disable built-in artwork

    Longer term:
        - struct mame_layer
          {
            mame_bitmap *bitmap;
            int rectcount;
            rectangle rectlist[MAX_RECTS];
          }
        - add 4 mame_layers for backdrop, overlay, bezel, ui to mame_display
        - some mechanism to let the OSD do the blending
        - MMX optimized implementations

**********************************************************************

    This file represents the second attempt at providing external
    artwork support. Some parts of this code are based on the
    original version, by Mike Balfour and Mathis Rosenhauer.

    The goal: to provide artwork support with minimal knowledge of
    the game drivers. The previous implementation required the
    game drivers to allocate extra pens, extend the screen bitmap,
    and handle a lot of the mundane details by hand. This is no
    longer the case.

    The key to all this is the .art file. A .art file is just a
    text file describing all the artwork needed for a particular
    game. It lives either in the $ARTWORK/gamename/ directory
    or in the $ARTWORK/gamename.zip file, and is called
    gamename.art.

**********************************************************************

    THE ART FILE

    The .art file is very simply formatted. It consists of any
    number of entries that look like this:

    [artname]:
        file       = [filename]
        alphafile  = [alphafilename]
        layer      = [backdrop|overlay|bezel|marquee|panel|side|flyer]
        position   = [left],[top],[right],[bottom]
        priority   = [priority]
        visible    = [visible]
        alpha      = [alpha]
        brightness = [brightness]

    Comments in the .art file follow standard C++ comment format,
    starting with a double-slash //. C-style comments are not
    recognized.

    Fields are:

    [artname] - name that is used to reference this piece of
        artwork in the game driver. Game drivers can show/hide
        pieces of artwork. It is permissible to use the same
        name for multiple pieces; in that case, a show/hide
        command from the game will affect all pieces with that
        name. This field is required.

    file - name of the PNG file containing the main artwork.
        This file should live in the same directory as the .art
        file itself. Most PNG formats are supported. If the
        PNG file does not have an alpha channel or transparent
        colors, it will be loaded fully opaque. This field is
        required.

    alphafile - name of a PNG file containing the alpha channel.
        Like the main file, this file should live in the same
        directory as the .art file. The alphafile must have the
        exact same dimensions as the main art file in order to
        be valid. When loaded, the brightness of each pixel in
        the alphafile controls the alpha channel for the
        corresponding pixel in the main art.

    layer - classifies this piece of artwork into one of several
        predefined categories. Command line options can control
        which categories of artwork are actually displayed. The
        layer is also used to group the artwork for rendering
        (see discussion of rendering below.) This field is
        required.

    position - specifies the position of this piece of artwork
        relative to the game bitmap. See the section on
        positioning, below, for the precise details. This field
        is required.

    priority - specifies the front-to-back ordering of this
        piece of art. The various artwork pieces are assembled
        from the bottom up, lowest priority to highest priority.
        If you want a piece of artwork to appear behind another
        piece of artwork, use a lower priority. The default
        priority is 0.

    visible - sets the initial visible state. By default, all
        artwork is visible. The driver code can change this state
        at runtime.

    alpha - specifies a global, additional alpha value for the
        entire piece of artwork. This alpha value is multiplied
        by the per-pixel alpha value for the loaded artwork.
        The default value is 1.0, which has no net effect on the
        loaded alpha. An alpha of 0.0 will make the entire piece
        of artwork fully transparent.

    brightness - specifies a global brightness adjustment factor
        for the entire piece of artwork. The red, green, and blue
        components of every pixel are multiplied by this value
        when the image is loaded. The default value is 1.0, which
        has no net effect on the loaded artwork. A brightness
        value of 0.0 will produce an entirely black image.

    Once the .art file is loaded, the artwork is categories into
    three groups: backdrops, overlays, and everything else. Each
    of these groups is handled in its own way.

**********************************************************************

    BLENDING

    Conceptually, here is how it all fits together:

    1. A combined backdrop bitmap is assembled. This consists of
    taking an opaque black bitmap, and alpha blending all the
    backdrop graphics, in order from lowest priority to highest,
    into it.

    2. A combined overlay bitmap is assembled. This consists of
    taking a translucent white overlay and performing a CMY blend
    of all the overlay graphics, in order from lowest priority to
    highest, into it.

    3. A combined bezel bitmap is assembled. This consists of
    taking a fully transparent bitmap, and alpha blending all the
    bezel, marquee, panel, side, and flyer graphics, in order from
    lowest to highest, into it.

    4. Depending on the user configurable artwork scale setting,
    the game bitmap is potentially expanded 2x.

    5. The combined overlay bitmap is applied to the game bitmap,
    by using the brightness of the game pixel to control the
    brightness of the corresponding overlay bitmap pixel, as
    follows:

        RGB[mix1] = (RGB[overlay] * A[overlay]) +
                (RGB[overlay] - RGB[overlay] * A[overlay]) * Y[game];

    where

        RGB[mix1] -> RGB components of final mixed bitmap
        A[overlay] -> alpha value of combined overlay
        RGB[overlay] -> RGB components of combined overlay
        Y[game] -> brightness of game pixel

    6. The result of the overlay + game blending is then added to
    the backdrop, as follows:

        RGB[mix2] = RGB[mix1] + RGB[backdrop]

    where

        RGB[mix2] -> RGB components of final mixed bitmap
        RGB[mix1] -> RGB components of game + overlay mixing
        RGB[backdrop] -> RGB components of combined backdrop graphics

    7. The combined bezel bitmap is alpha blended against the
    result of the previous operation, as follows:

        RGB[final] = (RGB[mix2] * (1 - A[bezel])) + (RGB[bezel] * A[bezel])

    where

        RGB[final] -> RGB components of final bitmap
        A[bezel] -> alpha value of combined bezel
        RGB[bezel] -> RGB components of combined bezel
        RGB[mix2] -> RGB components of game + overlay + backdrop mixing

**********************************************************************

    POSITIONING

    The positioning of the artwork is a little tricky.
    Conceptually, the game bitmap occupies the space from (0,0)
    to (1,1). If you have a piece of artwork that exactly covers
    the game area, then it too should stretch from (0,0) to (1,1).
    However, most of the time, this is not the case.

    For example, if you have, say, the Spy Hunter bezel at the
    bottom of the screen, then you will want to specify the top
    of the artwork at 1.0 and the bottom at something larger, maybe
    1.25. The nice thing about the new artwork system is that it
    will automatically stretch the bitmaps out to accomodate areas
    beyond the game bitmap, and will still keep the proper aspect
    ratio.

    Another common example is a backdrop that extends beyond all
    four corners of the game bitmap. Here is how you would handle
    that, in detail:

    Let's say you have some artwork like this:

     <============ 883 pixels ===============>

    (1)-------------------------------------(2)  ^
     |                  ^                    |   |
     |              26 pixels                |   |
     |                  v                    |   |
     |     (5)-----------------------(6)     |   |
     |      |                         |      |   |
     |      |                         |      |   |
     |      |                         |      |   |
     |<---->|                         |      |   |
     |  97  |      Game screen        |      |  768
     |pixels|       700 x 500         |      | pixels
     |      |                         |<---->|   |
     |      |                         |  86  |   |
     |      |                         |pixels|   |
     |      |                         |      |   |
     |      |                         |      |   |
     |     (7)-----------------------(8)     |   |
     |                  ^                    |   |
     |              42 pixels                |   |
     |                  v                    |   |
    (3)-------------------------------------(4)  v

    If you're looking at the raw coordinates as might seem
    logical, you would imagine that they come out like this:

        (1) is at (0,0)
        (2) is at (883,0)
        (3) is at (0,768)
        (4) is at (883,768)

        (5) is at (97,26)
        (6) is at (797,26)
        (7) is at (97,526)
        (8) is at (797,526)

    The first thing you need to do is adjust the coordinates
    so that the upper left corner of the game screen (point 5)
    is at (0,0). To do that, you need to subtract 97 from
    each X coordinate and 26 from each Y coordinate:

        (1) is at (0-97,0-26)     -> (-97,-26)
        (2) is at (883-97,0-26)   -> (786,-26)
        (3) is at (0-97,768-26)   -> (-97,742)
        (4) is at (883-97,768-26) -> (883,742)

        (5) is at (97-97,26-26)   -> (0,0)
        (6) is at (797-97,26-26)  -> (700,0)
        (7) is at (97-97,526-26)  -> (0,500)
        (8) is at (797-97,526-26) -> (700,500)

    The final thing you need to do is make it so the bottom
    right corner of the image (point 8) is at (1.0,1.0). To do
    that, you need to divide each coordinate by the width
    or height of the image

        (1) is at (-97/700,-26/500)  -> (-0.13857,-0.052)
        (2) is at (786/700,-26/500)  -> (1.122857,-0.052)
        (3) is at (-97/700,742/500)  -> (-0.13857, 1.484)
        (4) is at (883/700,742/500)  -> (1.122857, 1.484)

        (5) is at (0/700,0/500)      -> (0.0,0.0)
        (6) is at (700/700,0/500)    -> (1.0,0.0)
        (7) is at (0/700,500/500)    -> (0.0,1.0)
        (8) is at (700/700,500/500)  -> (1.0,1.0)

    Alternately, you can also provide pixel coordinates, but it will
    still be relative to the game's native resolution. So, if
    the game normally runs at 256x224, you'll need to compute
    the division factor so that the bottom right corner of the
    game (point 8) ends up at (256,224) instead of (1.0,1.0).

    Basically, if you have the original coordinates shown
    right below the image, you can compute the values needed by
    doing this for X coordinates:

        (X coordinate on artwork) - (X coordinate of game's upper-left)
        ---------------------------------------------------------------
                   (width of game in artwork pixels)

    And this for Y coordinates:

        (Y coordinate on artwork) - (Y coordinate of game's upper-left)
        ---------------------------------------------------------------
                   (height of game in artwork pixels)

*********************************************************************/


#include "osdepend.h"
#include "driver.h"
#include "png.h"
#include "artwork.h"
#include "artwork_lay.h"
#include <stdint.h>
#include "profiler.h"
#include "vidhrdw/vector.h"
#include <ctype.h>
#include <math.h>


/***************************************************************************

    Constants & macros

***************************************************************************/

/* maxima */
#define MAX_PIECES				1024
#define MAX_HINTS_PER_SCANLINE	4

/* fixed-point fraction helpers */
#define FRAC_BITS				24
#define FRAC_ONE				(1 << FRAC_BITS)
#define FRAC_MASK				(FRAC_ONE - 1)
#define FRAC_HALF				(FRAC_ONE / 2)

/* layer types */
enum
{
	LAYER_UNKNOWN,
	LAYER_BACKDROP,
	LAYER_OVERLAY,
	LAYER_BEZEL,
	LAYER_MARQUEE,
	LAYER_PANEL,
	LAYER_SIDE,
	LAYER_FLYER
};

/* UI transparency hack */
#define UI_TRANSPARENT_COLOR16	0xfffe
#define UI_TRANSPARENT_COLOR32	0xfffffffe

/* assemble ARGB components in the platform's preferred format */
#define ASSEMBLE_ARGB(a,r,g,b)	(((a) << ashift) | ((r) << rshift) | ((g) << gshift) | ((b) << bshift))



/***************************************************************************

    Type definitions

***************************************************************************/

struct _artwork_piece
{
	/* linkage */
	struct _artwork_piece *	next;

	/* raw data from the .art file */
	UINT8					layer;
	UINT8					has_alpha;
	int						priority;
	float					alpha;
	float					brightness;
	float					top;
	float					left;
	float					bottom;
	float					right;
	char *					tag;
	char *					filename;
	char *					alpha_filename;

	/* bitmaps */
	mame_bitmap *	rawbitmap;
	mame_bitmap *	prebitmap;
	mame_bitmap *	yrgbbitmap;
	UINT32 *				scanlinehint;
	UINT8					blendflags;

	/* derived/dynamic data */
	int						intersects_game;
	int						visible;
	rectangle		bounds;
};
typedef struct _artwork_piece artwork_piece;



/***************************************************************************

    Local variables

***************************************************************************/

static UINT8 rshift, gshift, bshift, ashift;
static UINT32 nonalpha_mask;
static UINT32 transparent_color;

static artwork_piece *artwork_list;
static int num_underlays, num_overlays, num_bezels;
static int num_pieces;

static mame_bitmap *underlay, *overlay, *overlay_yrgb, *bezel, *final;
static rectangle underlay_invalid, overlay_invalid, bezel_invalid;
static rectangle gamerect, screenrect;
static int gamescale;

static mame_bitmap *uioverlay;
static UINT32 *uioverlayhint;
static rectangle uibounds, last_uibounds;

static UINT32 *palette_lookup;

static int original_attributes;
static UINT8 global_artwork_enable;

static const artwork_overlay_piece *overlay_list;
static UINT32 *lay_scaled_game;

extern mame_bitmap *scrbitmap[];



/***************************************************************************

    Prototypes

***************************************************************************/

static int artwork_prep(void);
static void render_lay_transformed_game(mame_bitmap *, const rgb_t *, mame_display *);
static int artwork_load(const game_driver *gamename, int width, int height, const artwork_callbacks *callbacks);
static int compute_rgb_components(int depth, UINT32 rgb_components[3], UINT32 rgb32_components[3]);
static int scale_bitmap(artwork_piece *piece, int newwidth, int newheight);
static void trim_bitmap(artwork_piece *piece);
static void sort_pieces(void);
static void update_palette_lookup(mame_display *display);
static int update_layers(void);
static void render_game_bitmap(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display);
static void render_game_bitmap_underlay(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display);
static void render_game_bitmap_overlay(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display);
static void render_game_bitmap_underlay_overlay(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display);
static void render_ui_overlay(mame_bitmap *bitmap, UINT32 *dirty, const rgb_t *palette, mame_display *display);
static void erase_rect(mame_bitmap *bitmap, const rectangle *bounds, UINT32 color);
static void alpha_blend_intersecting_rect(mame_bitmap *dstbitmap, const rectangle *dstbounds, mame_bitmap *srcbitmap, const rectangle *srcbounds, const UINT32 *hintlist);
static void add_intersecting_rect(mame_bitmap *dstbitmap, const rectangle *dstbounds, mame_bitmap *srcbitmap, const rectangle *srcbounds);
static void cmy_blend_intersecting_rect(mame_bitmap *dstbitmap, mame_bitmap *dstyrgbbitmap, const rectangle *dstbounds, mame_bitmap *srcbitmap, mame_bitmap *srcyrgbbitmap, const rectangle *srcbounds, UINT8 blendflags);
static int generate_overlay(const artwork_overlay_piece *list, int width, int height);
static void add_range_to_hint(UINT32 *hintbase, int scanline, int startx, int endx);



#if 0
#pragma mark INLINES
#endif

/*-------------------------------------------------
    union_rect - compute the union of two rects
-------------------------------------------------*/

INLINE void union_rect(rectangle *dst, const rectangle *src)
{
	if (dst->max_x == 0)
		*dst = *src;
	else if (src->max_x != 0)
	{
		dst->min_x = (src->min_x < dst->min_x) ? src->min_x : dst->min_x;
		dst->max_x = (src->max_x > dst->max_x) ? src->max_x : dst->max_x;
		dst->min_y = (src->min_y < dst->min_y) ? src->min_y : dst->min_y;
		dst->max_y = (src->max_y > dst->max_y) ? src->max_y : dst->max_y;
	}
}



/*-------------------------------------------------
    compute_brightness - compute the effective
    brightness for an RGB pixel
-------------------------------------------------*/

INLINE UINT8 compute_brightness(rgb_t rgb)
{
	return (RGB_RED(rgb) * 222 + RGB_GREEN(rgb) * 707 + RGB_BLUE(rgb) * 71) / 1000;
}



/*-------------------------------------------------
    compute_pre_pixel - compute a premultiplied
    pixel
-------------------------------------------------*/

INLINE UINT32 compute_pre_pixel(UINT8 a, UINT8 r, UINT8 g, UINT8 b)
{
	/* premultiply the RGB components with the pixel's alpha */
	r = (r * a) / 0xff;
	g = (g * a) / 0xff;
	b = (b * a) / 0xff;

	/* compute the inverted alpha */
	a = 0xff - a;
	return ASSEMBLE_ARGB(a,r,g,b);
}



/*-------------------------------------------------
    compute_yrgb_pixel - compute a YRGB pixel
-------------------------------------------------*/

INLINE UINT32 compute_yrgb_pixel(UINT8 a, UINT8 r, UINT8 g, UINT8 b)
{
	/* compute the premultiplied brightness */
	int bright = (r * 222 + g * 707 + b * 71) / 1000;
	bright = (bright * a) >> 8;

	/* now assemble */
	return MAKE_ARGB(bright,r,g,b);
}



/*-------------------------------------------------
    add_and_clamp - add two pixels and clamp
    each component to the max
-------------------------------------------------*/

INLINE UINT32 add_and_clamp(UINT32 game, UINT32 underpix)
{
	UINT32 temp1 = game + underpix;
	UINT32 temp2 = game ^ underpix ^ temp1;

	/* handle overflow (carry out of top component */
	if (temp1 < game)
		temp1 |= 0xff000000;

	/* handle carry out of next component */
	if (temp2 & 0x01000000)
	{
		temp1 -= 0x01000000;
		temp1 |= 0x00ff0000;
	}

	/* handle carry out of next component */
	if (temp2 & 0x00010000)
	{
		temp1 -= 0x00010000;
		temp1 |= 0x0000ff00;
	}

	/* handle carry out of final component */
	if (temp2 & 0x00000100)
	{
		temp1 -= 0x00000100;
		temp1 |= 0x000000ff;
	}
	return temp1;
}



/*-------------------------------------------------
    blend_over - blend two pixels with overlay
-------------------------------------------------*/

INLINE UINT32 blend_over(UINT32 game, UINT32 pre, UINT32 yrgb)
{
	/* case 1: no game pixels; just return the premultiplied pixel */
	if ((game & nonalpha_mask) == 0)
		return pre;

	/* case 2: apply the effect */
	else
	{
		UINT8 bright = RGB_GREEN(game);
		UINT8 r, g, b;

		yrgb -= pre;
		r = (RGB_RED(yrgb) * bright) / 256;
		g = (RGB_GREEN(yrgb) * bright) / 256;
		b = (RGB_BLUE(yrgb) * bright) / 256;
		return pre + ASSEMBLE_ARGB(0,r,g,b);
	}
}



#if 0
#pragma mark -
#pragma mark OSD FRONTENDS
#endif

/*-------------------------------------------------
    artwork_create_display - tweak the display
    parameters based on artwork, and call through
    to osd_create_display
-------------------------------------------------*/

static void artwork_lay_exit(void)
{
	if (lay_scaled_game) {
		free(lay_scaled_game);
		lay_scaled_game = NULL;
	}
}

int artwork_create_display(osd_create_params *params, UINT32 *rgb_components, const artwork_callbacks *callbacks)
{
	int original_width = params->width;
	int original_height = params->height;
	int original_depth = params->depth;
	double min_x, min_y, max_x, max_y;
	UINT32 rgb32_components[3];
	artwork_piece *piece;

	/* reset UI */
	uioverlay = NULL;
	uioverlayhint = NULL;

	/* first load the artwork; if none, quit now */
	artwork_list = NULL;
	if (!artwork_load(Machine->gamedrv, original_width, original_height, callbacks))
		return 1;
	if (!artwork_list && !artwork_lay_active() && (!callbacks->activate_artwork || !callbacks->activate_artwork(params)))
		return osd_create_display(params, rgb_components);

	/* .lay rotation precedes the final OSD monitor orientation. */
	if (artwork_lay_screen_swapped()) {
		int t=original_width;
		original_width=original_height;original_height=t;
		t=params->aspect_x;params->aspect_x=params->aspect_y;params->aspect_y=t;
	}
	/* determine the game bitmap scale factor */
	gamescale = options.artwork_res;
	if (gamescale < 1 || (params->video_attributes & VIDEO_TYPE_VECTOR))
		gamescale = 1;
	else if (gamescale > 4)
		gamescale = 4;

	/* compute the extent of all the artwork */
	min_x = min_y = 0.0;
	max_x = max_y = 1.0;
	if (!options.artwork_crop)
	{
		artwork_lay_range range;
		artwork_lay_extents(&range);
		min_x = range.left; min_y = range.top;
		max_x = range.right; max_y = range.bottom;
		for (piece = artwork_list; piece; piece = piece->next)
		{
			if (piece->left < min_x) min_x = piece->left;
			if (piece->right > max_x) max_x = piece->right;
			if (piece->top < min_y) min_y = piece->top;
			if (piece->bottom > max_y) max_y = piece->bottom;
		}
	}

	/* now compute the altered width/height and the new aspect ratio */
	params->width = (int)((max_x - min_x) * (double)(original_width * gamescale) + 0.5);
	params->height = (int)((max_y - min_y) * (double)(original_height * gamescale) + 0.5);
	params->aspect_x = (int)((double)params->aspect_x * 100. * (max_x - min_x));
	params->aspect_y = (int)((double)params->aspect_y * 100. * (max_y - min_y));

	/* vector games need to fit inside the original bounds, so scale back down */
	if (params->video_attributes & VIDEO_TYPE_VECTOR)
	{
		/* shrink the width/height if over */
		if (params->width > original_width)
		{
			params->width = original_width;
			params->height = original_width * params->aspect_y / params->aspect_x;
		}
		if (params->height > original_height)
		{
			params->height = original_height;
			params->width = original_height * params->aspect_x / params->aspect_y;
		}

		/* compute the new raw width/height and update the vector info */
		original_width = (int)((double)params->width / (max_x - min_x));
		original_height = (int)((double)params->height / (max_y - min_y));
		options.vector_width = original_width;
		options.vector_height = original_height;
	}

	/* adjust the parameters */
	original_attributes = params->video_attributes;
	params->video_attributes |= VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN;
	params->depth = 32;

	/* allocate memory for the bitmaps */
	underlay = auto_bitmap_alloc_depth(params->width, params->height, 32);
	overlay = auto_bitmap_alloc_depth(params->width, params->height, 32);
	overlay_yrgb = auto_bitmap_alloc_depth(params->width, params->height, 32);
	bezel = auto_bitmap_alloc_depth(params->width, params->height, 32);
	final = auto_bitmap_alloc_depth(params->width, params->height, 32);
	if (!final || !overlay || !overlay_yrgb || !underlay || !bezel)
		return 1;

	/* allocate the UI overlay */
	uioverlay = auto_bitmap_alloc_depth(params->width, params->height, Machine->color_depth);
	if (!uioverlay)
		return 1;
	uioverlayhint = auto_malloc(uioverlay->height * MAX_HINTS_PER_SCANLINE * sizeof(uioverlayhint[0]));
	fillbitmap(uioverlay, (Machine->color_depth == 32) ? UI_TRANSPARENT_COLOR32 : UI_TRANSPARENT_COLOR16, NULL);
	memset(uioverlayhint, 0, uioverlay->height * MAX_HINTS_PER_SCANLINE * sizeof(uioverlayhint[0]));

	/* compute the screen rect */
	screenrect.min_x = screenrect.min_y = 0;
	screenrect.max_x = params->width - 1;
	screenrect.max_y = params->height - 1;

	/* compute the game rect */
	gamerect.min_x = (int)(-min_x * (double)(original_width * gamescale) + 0.5);
	gamerect.min_y = (int)(-min_y * (double)(original_height * gamescale) + 0.5);
	gamerect.max_x = gamerect.min_x + original_width * gamescale - 1;
	gamerect.max_y = gamerect.min_y + original_height * gamescale - 1;

	/* now try to create the display */
	if (osd_create_display(params, rgb32_components))
		return 1;

	/* fill in our own RGB components */
	if (compute_rgb_components(original_depth, rgb_components, rgb32_components))
	{
		osd_close_display();
		return 1;
	}

	/* now compute all the artwork pieces' coordinates */
	for (piece = artwork_list; piece; piece = piece->next)
	{
		piece->bounds.min_x = (int)((piece->left - min_x) * (double)(original_width * gamescale) + 0.5);
		piece->bounds.min_y = (int)((piece->top - min_y) * (double)(original_height * gamescale) + 0.5);
		piece->bounds.max_x = (int)((piece->right - min_x) * (double)(original_width * gamescale) + 0.5) - 1;
		piece->bounds.max_y = (int)((piece->bottom - min_y) * (double)(original_height * gamescale) + 0.5) - 1;
	}

	/* now do the final prep on the artwork */
	add_exit_callback(artwork_lay_exit);
	return artwork_prep();
}



/*-------------------------------------------------
    artwork_system_active - checks to see if the
    artwork system is currently active
-------------------------------------------------*/

static int artwork_system_active(void)
{
	return artwork_list || artwork_lay_active() || uioverlay;
}



/*-------------------------------------------------
    artwork_update_visible_area - resize artwork
    when the game changes resolution
-------------------------------------------------*/

void artwork_update_visible_area(mame_display *display)
{
	double min_x, min_y, max_x, max_y;
	int width, height;
	int original_width = ( display->game_visible_area.max_x - display->game_visible_area.min_x ) + 1;
	int original_height = ( display->game_visible_area.max_y - display->game_visible_area.min_y ) + 1;

	artwork_piece *piece;
	if (artwork_lay_screen_swapped()) {
		int t=original_width;
		original_width=original_height;original_height=t;
	}
	/* compute the extent of all the artwork */
	min_x = min_y = 0.0;
	max_x = max_y = 1.0;
	if (!options.artwork_crop)
	{
		artwork_lay_range range;
		artwork_lay_extents(&range);
		min_x = range.left; min_y = range.top;
		max_x = range.right; max_y = range.bottom;
		for (piece = artwork_list; piece; piece = piece->next)
		{
			if (piece->left < min_x) min_x = piece->left;
			if (piece->right > max_x) max_x = piece->right;
			if (piece->top < min_y) min_y = piece->top;
			if (piece->bottom > max_y) max_y = piece->bottom;
		}
	}

	width = (int)((max_x - min_x) * (double)(original_width * gamescale) + 0.5);
	height = (int)((max_y - min_y) * (double)(original_height * gamescale) + 0.5);

	/* compute the screen rect */
	screenrect.min_x = screenrect.min_y = 0;
	screenrect.max_x = width - 1;
	screenrect.max_y = height - 1;

	/* compute the game rect */
	gamerect.min_x = (int)(-min_x * (double)(original_width * gamescale) + 0.5);
	gamerect.min_y = (int)(-min_y * (double)(original_height * gamescale) + 0.5);
	gamerect.max_x = gamerect.min_x + original_width * gamescale - 1;
	gamerect.max_y = gamerect.min_y + original_height * gamescale - 1;

	/* now compute all the artwork pieces' coordinates */
	for (piece = artwork_list; piece; piece = piece->next)
	{
		piece->bounds.min_x = (int)((piece->left - min_x) * (double)(original_width * gamescale) + 0.5);
		piece->bounds.min_y = (int)((piece->top - min_y) * (double)(original_height * gamescale) + 0.5);
		piece->bounds.max_x = (int)((piece->right - min_x) * (double)(original_width * gamescale) + 0.5) - 1;
		piece->bounds.max_y = (int)((piece->bottom - min_y) * (double)(original_height * gamescale) + 0.5) - 1;
	}

	/* set UI visible area */
	ui_set_visible_area(screenrect.min_x, screenrect.min_y, screenrect.max_x, screenrect.max_y);

	artwork_prep();
}



/*-------------------------------------------------
    artwork_update_video_and_audio - update the
    screen, adjusting for artwork
-------------------------------------------------*/

void artwork_update_video_and_audio(mame_display *display)
{
	static rectangle ui_changed_bounds;
	static int ui_changed;
	int artwork_changed = 0, ui_visible = 0;

	/* do nothing if no artwork */
	if (!artwork_system_active())
	{
		osd_update_video_and_audio(display);
		return;
	}

	profiler_mark(PROFILER_ARTWORK);

	/* if the visible area has changed, update it */
	if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
		artwork_update_visible_area(display);

	/* update the palette */
	if (display->changed_flags & GAME_PALETTE_CHANGED)
		update_palette_lookup(display);

	/* process the artwork and UI only if we're not frameskipping */
	if (display->changed_flags & GAME_BITMAP_CHANGED)
	{
		/* see if there's any UI to display this frame */
		ui_visible = (uibounds.max_x != 0);

		/* if the UI bounds changed, refresh everything */
		if (last_uibounds.min_x != uibounds.min_x || last_uibounds.min_y != uibounds.min_y ||
			last_uibounds.max_x != uibounds.max_x || last_uibounds.max_y != uibounds.max_y)
		{
			/* compute the union of the two rects */
			ui_changed_bounds = last_uibounds;
			union_rect(&ui_changed_bounds, &uibounds);
			last_uibounds = uibounds;

			/* track changes for a few frames */
			ui_changed = 3;
		}

		/* if we have changed pending, mark the artwork dirty */
		if (ui_changed)
		{
			union_rect(&underlay_invalid, &ui_changed_bounds);
			union_rect(&overlay_invalid, &ui_changed_bounds);
			union_rect(&bezel_invalid, &ui_changed_bounds);
			ui_changed--;
		}

		/* artwork disabled case */
		if (!global_artwork_enable)
		{
			fillbitmap(final, MAKE_ARGB(0,0,0,0), NULL);
			union_rect(&underlay_invalid, &screenrect);
			union_rect(&overlay_invalid, &screenrect);
			union_rect(&bezel_invalid, &screenrect);
			render_game_bitmap(display->game_bitmap, palette_lookup, display);
		}

		/* artwork enabled */
		else
		{
			/* update the underlay and overlay */
			artwork_changed = update_layers();
			/* Reset pixels outside gamerect before repainting the scene. */
			if (artwork_lay_has_backdrop()) {
				int y;
				erase_rect(underlay, &screenrect, 0);
				artwork_lay_draw(underlay, LAY_BACK, rshift, gshift, bshift, ashift);
				for (y = 0; y < final->height; ++y)
					memcpy(final->line[y], underlay->line[y], (size_t)final->width * sizeof(UINT32));
			} else if (artwork_lay_active()) {
				erase_rect(final, &screenrect, 0);
			}

			/* render to the final bitmap */
			if (artwork_lay_screen_transformed())
				render_lay_transformed_game(display->game_bitmap, palette_lookup, display);
			else if (num_underlays && num_overlays)
				render_game_bitmap_underlay_overlay(display->game_bitmap, palette_lookup, display);
			else if (num_underlays)
				render_game_bitmap_underlay(display->game_bitmap, palette_lookup, display);
			else if (num_overlays)
				render_game_bitmap_overlay(display->game_bitmap, palette_lookup, display);
			else
				render_game_bitmap(display->game_bitmap, palette_lookup, display);

			/* Static .lay foreground items respect their view ordering. */
			if (artwork_lay_active()) {
				artwork_lay_draw(final, LAY_FRONT, rshift, gshift, bshift, ashift);
				artwork_changed = 1;
			}

			/* apply the bezel */
			if (num_bezels)
			{
				artwork_piece *piece;
				for (piece = artwork_list; piece; piece = piece->next)
					if (piece->layer >= LAYER_BEZEL && piece->intersects_game)
						alpha_blend_intersecting_rect(final, &gamerect, piece->prebitmap, &piece->bounds, piece->scanlinehint);
			}
		}

		/* add UI */
		if (ui_visible)
			render_ui_overlay(uioverlay, uioverlayhint, palette_lookup, display);

		/* if artwork changed, or there's UI, we can't use dirty pixels */
		if (artwork_changed || ui_changed || ui_visible)
		{
			display->changed_flags &= ~VECTOR_PIXELS_CHANGED;
			display->vector_dirty_pixels = NULL;
		}
	}
	profiler_mark(PROFILER_END);

	/* blit the union of the game/screen rect and the UI bounds */
	display->game_bitmap_update = (artwork_changed || ui_changed) ? screenrect : gamerect;
	union_rect(&display->game_bitmap_update, &uibounds);

	/* force the visible area constant */
	display->game_visible_area = screenrect;
	display->game_bitmap = final;
	osd_update_video_and_audio(display);

	/* reset the UI bounds (but only if we rendered the UI) */
	if (display->changed_flags & GAME_BITMAP_CHANGED)
		uibounds.max_x = 0;
}



/*-------------------------------------------------
    artwork_override_screenshot_params - override
    certain parameters when saving a screenshot
-------------------------------------------------*/

void artwork_override_screenshot_params(mame_bitmap **bitmap, rectangle *rect, UINT32 *rgb_components)
{
	if ((*bitmap == scrbitmap[0] || *bitmap == uioverlay) && artwork_system_active())
	{
		*rect = screenrect;

		/* snapshots require correct direct_rgb_components */
		rgb_components[0] = 0xff << rshift;
		rgb_components[1] = 0xff << gshift;
		rgb_components[2] = 0xff << bshift;
		*bitmap = final;
	}
}



/*-------------------------------------------------
    artwork_get_ui_bitmap - get the UI bitmap
-------------------------------------------------*/

mame_bitmap *artwork_get_ui_bitmap(void)
{
	return uioverlay ? uioverlay : scrbitmap[0];
}



/*-------------------------------------------------
    artwork_mark_ui_dirty - mark a portion of the
    UI bitmap dirty
-------------------------------------------------*/

void artwork_mark_ui_dirty(int minx, int miny, int maxx, int maxy)
{
	/* add to the UI overlay hint if it exists */
	if (uioverlayhint)
	{
		rectangle rect;
		int y;

		/* clip to visible */
		if (minx < 0)
			minx = 0;
		if (maxx >= uioverlay->width)
			maxx = uioverlay->width - 1;
		if (miny < 0)
			miny = 0;
		if (maxy >= uioverlay->height)
			maxy = uioverlay->height - 1;

		/* update the global rect */
		rect.min_x = minx;
		rect.max_x = maxx;
		rect.min_y = miny;
		rect.max_y = maxy;
		union_rect(&uibounds, &rect);

		/* add hints for each scanline */
		if (minx <= maxx)
			for (y = miny; y <= maxy; y++)
				add_range_to_hint(uioverlayhint, y, minx, maxx);
	}
}



/*-------------------------------------------------
    artwork_get_screensize - get the real screen
    size
-------------------------------------------------*/

void artwork_get_screensize(int *width, int *height)
{
	if (artwork_list || artwork_lay_active())
	{
		*width = screenrect.max_x - screenrect.min_x + 1;
		*height = screenrect.max_y - screenrect.min_y + 1;
	}
	else
	{
		*width = Machine->drv->screen_width;
		*height = Machine->drv->screen_height;
	}
}



/*-------------------------------------------------
    artwork_enable - globally enable/disable
    artwork
-------------------------------------------------*/

void artwork_enable(int enable)
{
	global_artwork_enable = enable;
}



/*-------------------------------------------------
    artwork_set_overlay - set the hard-coded
    overlay for this game
-------------------------------------------------*/

void artwork_set_overlay(const artwork_overlay_piece *overlist)
{
	overlay_list = overlist;
}



/*-------------------------------------------------
    artwork_show - show/hide a tagged piece of art
-------------------------------------------------*/

void artwork_show(const char *tag, int show)
{
	artwork_piece *piece;

	/* find all the pieces that match the tag */
	for (piece = artwork_list; piece; piece = piece->next)
		if (piece->tag && !strcmp(piece->tag, tag))
		{
			/* if the state is changing, invalidate that area */
			if (piece->visible != show)
			{
				piece->visible = show;

				/* backdrop */
				if (piece->layer == LAYER_BACKDROP)
					union_rect(&underlay_invalid, &piece->bounds);

				/* overlay */
				else if (piece->layer == LAYER_OVERLAY)
					union_rect(&overlay_invalid, &piece->bounds);

				/* bezel */
				else if (piece->layer >= LAYER_BEZEL)
					union_rect(&bezel_invalid, &piece->bounds);
			}
		}
}



#if 0
#pragma mark -
#pragma mark LAYER PROCESSING
#endif

/*-------------------------------------------------
    update_layers - update any dirty areas of
    the layers
-------------------------------------------------*/

static int update_layers(void)
{
	artwork_piece *piece = artwork_list;
	rectangle combined;
	int changed = 0;

	/* update the underlays */
	if (underlay_invalid.max_x != 0)
	{
		sect_rect(&underlay_invalid, &screenrect);
		erase_rect(underlay, &underlay_invalid, 0);
		for (piece = artwork_list; piece; piece = piece->next)
			if (piece->layer == LAYER_BACKDROP && piece->visible && piece->prebitmap)
				alpha_blend_intersecting_rect(underlay, &underlay_invalid, piece->prebitmap, &piece->bounds, piece->scanlinehint);
	}

	/* update the overlays */
	if (overlay_invalid.max_x != 0)
	{
		sect_rect(&overlay_invalid, &screenrect);
		erase_rect(overlay, &overlay_invalid, transparent_color);
		erase_rect(overlay_yrgb, &overlay_invalid, ASSEMBLE_ARGB(0,0xff,0xff,0xff));
		for (piece = artwork_list; piece; piece = piece->next)
			if (piece->layer == LAYER_OVERLAY && piece->visible && piece->prebitmap)
				cmy_blend_intersecting_rect(overlay, overlay_yrgb, &overlay_invalid, piece->prebitmap, piece->yrgbbitmap, &piece->bounds, piece->blendflags);
	}

	/* update the bezels */
	if (bezel_invalid.max_x != 0)
	{
		sect_rect(&bezel_invalid, &screenrect);
		erase_rect(bezel, &bezel_invalid, transparent_color);
		for (piece = artwork_list; piece; piece = piece->next)
			if (piece->layer >= LAYER_BEZEL && piece->visible && piece->prebitmap)
				alpha_blend_intersecting_rect(bezel, &bezel_invalid, piece->prebitmap, &piece->bounds, piece->scanlinehint);
	}

	/* combine the invalid rects */
	combined = underlay_invalid;
	union_rect(&combined, &overlay_invalid);
	union_rect(&combined, &bezel_invalid);
	if (combined.max_x != 0)
	{
		/* blend into the final bitmap */
		erase_rect(final, &combined, 0);
		alpha_blend_intersecting_rect(final, &combined, underlay, &screenrect, NULL);
		add_intersecting_rect(final, &combined, overlay, &screenrect);
		alpha_blend_intersecting_rect(final, &combined, bezel, &screenrect, NULL);
		changed = 1;
	}

	/* reset the invalid rects */
	underlay_invalid.max_x = 0;
	overlay_invalid.max_x = 0;
	bezel_invalid.max_x = 0;
	return changed;
}



/*-------------------------------------------------
    erase_rect - erase the given bounds of a 32bpp
    bitmap
-------------------------------------------------*/

static void erase_rect(mame_bitmap *bitmap, const rectangle *bounds, UINT32 color)
{
	int x, y;

	/* loop over rows */
	for (y = bounds->min_y; y <= bounds->max_y; y++)
	{
		UINT32 *dest = (UINT32 *)bitmap->base + y * bitmap->rowpixels;
		for (x = bounds->min_x; x <= bounds->max_x; x++)
			dest[x] = color;
	}
}



/*-------------------------------------------------
    alpha_blend_intersecting_rect - alpha blend an
    artwork piece into a bitmap
-------------------------------------------------*/

static void alpha_blend_intersecting_rect(mame_bitmap *dstbitmap, const rectangle *dstbounds, mame_bitmap *srcbitmap, const rectangle *srcbounds, const UINT32 *hintlist)
{
	rectangle sect = *srcbounds;
	UINT32 dummy_range[2];
	int lclip, rclip;
	int x, y, h;

	/* compute the intersection */
	sect_rect(&sect, dstbounds);

	/* compute the source-relative left/right clip */
	lclip = sect.min_x - srcbounds->min_x;
	rclip = sect.max_x - srcbounds->min_x;

	/* set up a dummy range */
	dummy_range[0] = srcbitmap->width - 1;
	dummy_range[1] = 0;

	/* adjust the hintlist for the starting offset */
	if (hintlist)
		hintlist -= srcbounds->min_y * MAX_HINTS_PER_SCANLINE;

	/* loop over rows */
	for (y = sect.min_y; y <= sect.max_y; y++)
	{
		UINT32 *src = (UINT32 *)srcbitmap->base + (y - srcbounds->min_y) * srcbitmap->rowpixels;
		UINT32 *dest = (UINT32 *)dstbitmap->base + y * dstbitmap->rowpixels + srcbounds->min_x;
		const UINT32 *hint = hintlist ? &hintlist[y * MAX_HINTS_PER_SCANLINE] : &dummy_range[0];

		/* loop over hints */
		for (h = 0; h < MAX_HINTS_PER_SCANLINE && hint[h] != 0; h++)
		{
			int start = hint[h] >> 16;
			int stop = hint[h] & 0xffff;

			/* clip to the sect rect */
			if (start < lclip)
				start = lclip;
			else if (start > rclip)
				continue;
			if (stop > rclip)
				stop = rclip;
			else if (stop < lclip)
				continue;

			/* loop over columns */
			for (x = start; x <= stop; x++)
			{
				/* we don't bother optimizing for transparent here because we hope that the */
				/* hints have removed most of the need */
				UINT32 pix = src[x];
				UINT32 dpix = dest[x];
				int alpha = (pix >> ashift) & 0xff;

				/* alpha is inverted, so alpha 0 means fully opaque */
				if (alpha == 0)
					dest[x] = pix;

				/* otherwise, we do a proper blend */
				else
				{
					int r = ((pix >> rshift) & 0xff) + ((alpha * ((dpix >> rshift) & 0xff)) >> 8);
					int g = ((pix >> gshift) & 0xff) + ((alpha * ((dpix >> gshift) & 0xff)) >> 8);
					int b = ((pix >> bshift) & 0xff) + ((alpha * ((dpix >> bshift) & 0xff)) >> 8);

					/* add the alpha values in inverted space (looks weird but is correct) */
					int a = alpha + ((dpix >> ashift) & 0xff) - 0xff;
					if (a < 0) a = 0;
					dest[x] = ASSEMBLE_ARGB(a,r,g,b);
				}
			}
		}
	}
}



/*-------------------------------------------------
    add_intersecting_rect - add a
    artwork piece into a bitmap
-------------------------------------------------*/

static void add_intersecting_rect(mame_bitmap *dstbitmap, const rectangle *dstbounds, mame_bitmap *srcbitmap, const rectangle *srcbounds)
{
	rectangle sect = *srcbounds;
	int x, y, width;

	/* compute the intersection and resulting width */
	sect_rect(&sect, dstbounds);
	width = sect.max_x - sect.min_x + 1;

	/* loop over rows */
	for (y = sect.min_y; y <= sect.max_y; y++)
	{
		UINT32 *src = (UINT32 *)srcbitmap->base + (y - srcbounds->min_y) * srcbitmap->rowpixels + (sect.min_x - srcbounds->min_x);
		UINT32 *dest = (UINT32 *)dstbitmap->base + y * dstbitmap->rowpixels + sect.min_x;

		/* loop over columns */
		for (x = 0; x < width; x++)
		{
			UINT32 pix = src[x];

			/* just add and clamp */
			if (pix != transparent_color)
				dest[x] = add_and_clamp(pix, dest[x]);
		}
	}
}



/*-------------------------------------------------
    cmy_blend_intersecting_rect - CMY blend an
    artwork piece into a bitmap
-------------------------------------------------*/

static void cmy_blend_intersecting_rect(
	mame_bitmap *dstprebitmap, mame_bitmap *dstyrgbbitmap, const rectangle *dstbounds,
	mame_bitmap *srcprebitmap, mame_bitmap *srcyrgbbitmap, const rectangle *srcbounds,
	UINT8 blendflags)
{
	rectangle sect = *srcbounds;
	int x, y, width;

	/* compute the intersection and resulting width */
	sect_rect(&sect, dstbounds);
	width = sect.max_x - sect.min_x + 1;

	/* loop over rows */
	for (y = sect.min_y; y <= sect.max_y; y++)
	{
		UINT32 *srcpre = (UINT32 *)srcprebitmap->base + (y - srcbounds->min_y) * srcprebitmap->rowpixels + (sect.min_x - srcbounds->min_x);
		UINT32 *srcyrgb = (UINT32 *)srcyrgbbitmap->base + (y - srcbounds->min_y) * srcyrgbbitmap->rowpixels + (sect.min_x - srcbounds->min_x);
		UINT32 *destpre = (UINT32 *)dstprebitmap->base + y * dstprebitmap->rowpixels + sect.min_x;
		UINT32 *destyrgb = (UINT32 *)dstyrgbbitmap->base + y * dstyrgbbitmap->rowpixels + sect.min_x;

		/* loop over columns */
		for (x = 0; x < width; x++)
		{
			UINT32 spre = srcpre[x];
			UINT32 dpre = destpre[x];
			UINT32 syrgb = srcyrgb[x];
			UINT32 dyrgb = destyrgb[x];

			/* handle "non-blending" mode */
			if (blendflags & OVERLAY_FLAG_NOBLEND)
			{
				if ((spre & nonalpha_mask) && spre >= dpre)
				{
					destpre[x] = spre;
					destyrgb[x] = syrgb;
				}
			}

			/* simple copy if nothing at the dest */
			else if (dpre == transparent_color && dyrgb == 0)
			{
				destpre[x] = spre;
				destyrgb[x] = syrgb;
			}
			else
			{
				/* subtract CMY and alpha from each pixel */
				int sc = (~syrgb >> rshift) & 0xff;
				int sm = (~syrgb >> gshift) & 0xff;
				int sy = (~syrgb >> bshift) & 0xff;
				int sa = (~spre >> ashift) & 0xff;
				int dc = (~dyrgb >> rshift) & 0xff;
				int dm = (~dyrgb >> gshift) & 0xff;
				int dy = (~dyrgb >> bshift) & 0xff;
				int da = (~dpre >> ashift) & 0xff;
				int dr, dg, db;
				int max;

				/* add and clamp the alphas */
				da += sa;
				if (da > 0xff) da = 0xff;

				/* add the CMY */
				dc += sc;
				dm += sm;
				dy += sy;

				/* compute the maximum intensity */
				max = (dc > dm) ? dc : dm;
				max = (dy > max) ? dy : max;

				/* if that's out of range, scale by it */
				if (max > 0xff)
				{
					dc = (dc * 0xff) / max;
					dm = (dm * 0xff) / max;
					dy = (dy * 0xff) / max;
				}

				/* convert back to RGB */
				dr = dc ^ 0xff;
				dg = dm ^ 0xff;
				db = dy ^ 0xff;

				/* recompute the two pixels */
				destpre[x] = compute_pre_pixel(da,dr,dg,db);
				destyrgb[x] = compute_yrgb_pixel(da,dr,dg,db);
			}
		}
	}
}



#if 0
#pragma mark -
#pragma mark GAME BITMAP PROCESSING
#endif

/*-------------------------------------------------
    update_palette - update any dirty palette
    entries
-------------------------------------------------*/

static void update_palette_lookup(mame_display *display)
{
	int i, j;

	/* loop over dirty colors in batches of 32 */
	for (i = 0; i < display->game_palette_entries; i += 32)
	{
		UINT32 dirtyflags = display->game_palette_dirty[i / 32];
		if (dirtyflags)
		{
			display->game_palette_dirty[i / 32] = 0;

			/* loop over all 32 bits and update dirty entries */
			for (j = 0; j < 32; j++, dirtyflags >>= 1)
				if (dirtyflags & 1)
				{
					/* extract the RGB values */
					rgb_t rgbvalue = display->game_palette[i + j];
					int r = RGB_RED(rgbvalue);
					int g = RGB_GREEN(rgbvalue);
					int b = RGB_BLUE(rgbvalue);

					/* update the lookup table */
					palette_lookup[i + j] = ASSEMBLE_ARGB(0, r, g, b);
				}
		}
	}
}



/*-------------------------------------------------
    render_game_bitmap - render the game bitmap
    raw
-------------------------------------------------*/

#define PIXEL(x,y,srcdstbase,srcdstrpix,bits)	(*((UINT##bits *)srcdstbase##base + (y) * srcdstrpix##rowpixels + (x)))

static void render_game_bitmap(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display)
{
	int srcrowpixels = bitmap->rowpixels;
	int dstrowpixels = final->rowpixels;
	void *srcbase, *dstbase;
	int width, height;
	int x, y;
	int src_dx, src_dy, src_dp, src_dw;
	UINT8 *src_ptr;
	int scaled_dx, scaled_dy, scaled_dp, scaled_dw;
	UINT8 *scaled_ptr;

	/* compute common parameters */
	width = Machine->absolute_visible_area.max_x - Machine->absolute_visible_area.min_x + 1;
	height = Machine->absolute_visible_area.max_y - Machine->absolute_visible_area.min_y + 1;
	srcbase = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * bitmap->rowbytes;
	dstbase = (UINT8 *)final->base + gamerect.min_y * final->rowbytes + gamerect.min_x * sizeof(UINT32);

	/* vector case */
	if (display->changed_flags & VECTOR_PIXELS_CHANGED)
	{
		vector_pixel_t offset = VECTOR_PIXEL(gamerect.min_x, gamerect.min_y);
		vector_pixel_t *list = display->vector_dirty_pixels;

		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = palette[PIXEL(x,y,src,src,16)];
			}
		}

		/* 32bpp case */
		else
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = PIXEL(x,y,src,src,32);
			}
		}

		return;
	}

	/* 1x scale */
	if (gamescale == 1)
	{
		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			for (y = 0; y < height; y++)
			{
				UINT16 *src = (UINT16 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = palette[*src++];
			}
		}

		/* 32bpp case */
		else
		{
			for (y = 0; y < height; y++)
			{
				UINT32 *src = (UINT32 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = *src++;
			}
		}

		return;
	}

	/* for any other scale use the OSD layer to strech the game bitmap with effects */
	src_dx = width;
	src_dy = height;
	src_dp = bitmap->depth != 32 ? 2 : 4;
	src_dw = bitmap->rowbytes;
	src_ptr = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * src_dw + Machine->absolute_visible_area.min_x * src_dp;

	scaled_dx = width * gamescale;
	scaled_dy = height * gamescale;
	scaled_dp = 4;
	scaled_dw = scaled_dx * scaled_dp;
	scaled_ptr = malloc(scaled_dy * scaled_dw);

	/* we have only two case, palette16 or rgb32 */
	if (src_dp == 2) {
		osd_stretch_palett16to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw, palette);
	} else {
		osd_stretch_32to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw);
	}

	/* draw the scaled bitmap */
	for (y = 0; y < scaled_dy; y++)
	{
		UINT32 *src = (UINT32*)(scaled_ptr + y * scaled_dw);
		UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;

		memcpy(dst, src, scaled_dw);
	}
}



/*-------------------------------------------------
    render_game_bitmap_underlay - render the game
    bitmap on top of an underlay
-------------------------------------------------*/

static void render_game_bitmap_underlay(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display)
{
	int srcrowpixels = bitmap->rowpixels;
	int dstrowpixels = final->rowpixels;
	void *srcbase, *dstbase, *undbase;
	int width, height;
	int x, y;
	int src_dx, src_dy, src_dp, src_dw;
	UINT8 *src_ptr;
	int scaled_dx, scaled_dy, scaled_dp, scaled_dw;
	UINT8 *scaled_ptr;

	/* compute common parameters */
	width = Machine->absolute_visible_area.max_x - Machine->absolute_visible_area.min_x + 1;
	height = Machine->absolute_visible_area.max_y - Machine->absolute_visible_area.min_y + 1;
	srcbase = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * bitmap->rowbytes;
	dstbase = (UINT8 *)final->base + gamerect.min_y * final->rowbytes + gamerect.min_x * sizeof(UINT32);
	undbase = (UINT8 *)underlay->base + gamerect.min_y * underlay->rowbytes + gamerect.min_x * sizeof(UINT32);

	/* vector case */
	if (display->changed_flags & VECTOR_PIXELS_CHANGED)
	{
		vector_pixel_t offset = VECTOR_PIXEL(gamerect.min_x, gamerect.min_y);
		vector_pixel_t *list = display->vector_dirty_pixels;

		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = add_and_clamp(palette[PIXEL(x,y,src,src,16)], PIXEL(x,y,und,dst,32));
			}
		}

		/* 32bpp case */
		else
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = add_and_clamp(PIXEL(x,y,src,src,32), PIXEL(x,y,und,dst,32));
			}
		}

		return;
	}

	/* 1x scale */
	if (gamescale == 1)
	{
		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			for (y = 0; y < height; y++)
			{
				UINT16 *src = (UINT16 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				UINT32 *und = (UINT32 *)undbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = add_and_clamp(palette[*src++], *und++);
			}
		}

		/* 32bpp case */
		else
		{
			for (y = 0; y < height; y++)
			{
				UINT32 *src = (UINT32 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				UINT32 *und = (UINT32 *)undbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = add_and_clamp(*src++, *und++);
			}
		}

		return;
	}

	/* for any other scale use the OSD layer to strech the game bitmap with effects */
	src_dx = width;
	src_dy = height;
	src_dp = bitmap->depth != 32 ? 2 : 4;
	src_dw = bitmap->rowbytes;
	src_ptr = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * src_dw + Machine->absolute_visible_area.min_x * src_dp;

	scaled_dx = width * gamescale;
	scaled_dy = height * gamescale;
	scaled_dp = 4;
	scaled_dw = scaled_dx * scaled_dp;
	scaled_ptr = malloc(scaled_dy * scaled_dw);

	/* we have only two case, palette16 or rgb32 */
	if (src_dp == 2) {
		osd_stretch_palett16to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw, palette);
	} else {
		osd_stretch_32to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw);
	}

	/* draw the scaled bitmap */
	for (y = 0; y < scaled_dy; y++)
	{
		UINT32 *src = (UINT32*)(scaled_ptr + y * scaled_dw);
		UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
		UINT32 *und = (UINT32 *)undbase + y * dstrowpixels;

		if (gamescale == 2) {
			for (x = 0; x < width; x++)
			{
				dst[0] = add_and_clamp(src[0], und[0]);
				dst[1] = add_and_clamp(src[1], und[1]);
				src += 2;
				dst += 2;
				und += 2;
			}
		} else if (gamescale == 3) {
			for (x = 0; x < width; x++)
			{
				dst[0] = add_and_clamp(src[0], und[0]);
				dst[1] = add_and_clamp(src[1], und[1]);
				dst[2] = add_and_clamp(src[2], und[2]);
				src += 3;
				dst += 3;
				und += 3;
			}
		} else if (gamescale == 4) {
			for (x = 0; x < width; x++)
			{
				dst[0] = add_and_clamp(src[0], und[0]);
				dst[1] = add_and_clamp(src[1], und[1]);
				dst[2] = add_and_clamp(src[2], und[2]);
				dst[3] = add_and_clamp(src[3], und[3]);
				src += 4;
				dst += 4;
				und += 4;
			}
		}
	}
}



/*-------------------------------------------------
    render_game_bitmap_overlay - render the game
    bitmap blended with an overlay
-------------------------------------------------*/

static void render_game_bitmap_overlay(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display)
{
	int srcrowpixels = bitmap->rowpixels;
	int dstrowpixels = final->rowpixels;
	void *srcbase, *dstbase, *overbase, *overyrgbbase;
	int width, height;
	int x, y;
	int src_dx, src_dy, src_dp, src_dw;
	UINT8 *src_ptr;
	int scaled_dx, scaled_dy, scaled_dp, scaled_dw;
	UINT8 *scaled_ptr;

	/* compute common parameters */
	width = Machine->absolute_visible_area.max_x - Machine->absolute_visible_area.min_x + 1;
	height = Machine->absolute_visible_area.max_y - Machine->absolute_visible_area.min_y + 1;
	srcbase = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * bitmap->rowbytes;
	dstbase = (UINT8 *)final->base + gamerect.min_y * final->rowbytes + gamerect.min_x * sizeof(UINT32);
	overbase = (UINT8 *)overlay->base + gamerect.min_y * overlay->rowbytes + gamerect.min_x * sizeof(UINT32);
	overyrgbbase = (UINT8 *)overlay_yrgb->base + gamerect.min_y * overlay_yrgb->rowbytes + gamerect.min_x * sizeof(UINT32);

	/* vector case */
	if (display->changed_flags & VECTOR_PIXELS_CHANGED)
	{
		vector_pixel_t offset = VECTOR_PIXEL(gamerect.min_x, gamerect.min_y);
		vector_pixel_t *list = display->vector_dirty_pixels;

		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = blend_over(palette[PIXEL(x,y,src,src,16)], PIXEL(x,y,over,dst,32), PIXEL(x,y,overyrgb,dst,32));
			}
		}

		/* 32bpp case */
		else
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = blend_over(PIXEL(x,y,src,src,32), PIXEL(x,y,over,dst,32), PIXEL(x,y,overyrgb,dst,32));
			}
		}

		return;
	}

	/* 1x scale */
	if (gamescale == 1)
	{
		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			for (y = 0; y < height; y++)
			{
				UINT16 *src = (UINT16 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				UINT32 *over = (UINT32 *)overbase + y * dstrowpixels;
				UINT32 *overyrgb = (UINT32 *)overyrgbbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = blend_over(palette[*src++], *over++, *overyrgb++);
			}
		}

		/* 32bpp case */
		else
		{
			for (y = 0; y < height; y++)
			{
				UINT32 *src = (UINT32 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				UINT32 *over = (UINT32 *)overbase + y * dstrowpixels;
				UINT32 *overyrgb = (UINT32 *)overyrgbbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = blend_over(*src++, *over++, *overyrgb++);
			}
		}

		return;
	}

	/* for any other scale use the OSD layer to strech the game bitmap with effects */
	src_dx = width;
	src_dy = height;
	src_dp = bitmap->depth != 32 ? 2 : 4;
	src_dw = bitmap->rowbytes;
	src_ptr = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * src_dw + Machine->absolute_visible_area.min_x * src_dp;

	scaled_dx = width * gamescale;
	scaled_dy = height * gamescale;
	scaled_dp = 4;
	scaled_dw = scaled_dx * scaled_dp;
	scaled_ptr = malloc(scaled_dy * scaled_dw);

	/* we have only two case, palette16 or rgb32 */
	if (src_dp == 2) {
		osd_stretch_palett16to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw, palette);
	} else {
		osd_stretch_32to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw);
	}

	/* draw the scaled bitmap */
	for (y = 0; y < scaled_dy; y++)
	{
		UINT32 *src = (UINT32*)(scaled_ptr + y * scaled_dw);
		UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
		UINT32 *over = (UINT32 *)overbase + y * dstrowpixels;
		UINT32 *overyrgb = (UINT32 *)overyrgbbase + y * dstrowpixels;

		if (gamescale == 2) {
			for (x = 0; x < width; x++)
			{
				dst[0] = blend_over(src[0], over[0], overyrgb[0]);
				dst[1] = blend_over(src[1], over[1], overyrgb[1]);
				src += 2;
				dst += 2;
				over += 2;
				overyrgb += 2;
			}
		} else if (gamescale == 3) {
			for (x = 0; x < width; x++)
			{
				dst[0] = blend_over(src[0], over[0], overyrgb[0]);
				dst[1] = blend_over(src[1], over[1], overyrgb[1]);
				dst[2] = blend_over(src[2], over[2], overyrgb[2]);
				src += 3;
				dst += 3;
				over += 3;
				overyrgb += 3;
			}
		} else if (gamescale == 4) {
			for (x = 0; x < width; x++)
			{
				dst[0] = blend_over(src[0], over[0], overyrgb[0]);
				dst[1] = blend_over(src[1], over[1], overyrgb[1]);
				dst[2] = blend_over(src[2], over[2], overyrgb[2]);
				dst[3] = blend_over(src[3], over[3], overyrgb[3]);
				src += 4;
				dst += 4;
				over += 4;
				overyrgb += 4;
			}
		}
	}
}



/*-------------------------------------------------
    render_game_bitmap_underlay_overlay - render
    the game bitmap blended with an overlay and
    added to an underlay
-------------------------------------------------*/

static void render_game_bitmap_underlay_overlay(mame_bitmap *bitmap, const rgb_t *palette, mame_display *display)
{
	int srcrowpixels = bitmap->rowpixels;
	int dstrowpixels = final->rowpixels;
	void *srcbase, *dstbase, *undbase, *overbase, *overyrgbbase;
	int width, height;
	int x, y;
	int src_dx, src_dy, src_dp, src_dw;
	UINT8 *src_ptr;
	int scaled_dx, scaled_dy, scaled_dp, scaled_dw;
	UINT8 *scaled_ptr;

	/* compute common parameters */
	width = Machine->absolute_visible_area.max_x - Machine->absolute_visible_area.min_x + 1;
	height = Machine->absolute_visible_area.max_y - Machine->absolute_visible_area.min_y + 1;
	srcbase = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * bitmap->rowbytes;
	dstbase = (UINT8 *)final->base + gamerect.min_y * final->rowbytes + gamerect.min_x * sizeof(UINT32);
	undbase = (UINT8 *)underlay->base + gamerect.min_y * underlay->rowbytes + gamerect.min_x * sizeof(UINT32);
	overbase = (UINT8 *)overlay->base + gamerect.min_y * overlay->rowbytes + gamerect.min_x * sizeof(UINT32);
	overyrgbbase = (UINT8 *)overlay_yrgb->base + gamerect.min_y * overlay_yrgb->rowbytes + gamerect.min_x * sizeof(UINT32);

	/* vector case */
	if (display->changed_flags & VECTOR_PIXELS_CHANGED)
	{
		vector_pixel_t offset = VECTOR_PIXEL(gamerect.min_x, gamerect.min_y);
		vector_pixel_t *list = display->vector_dirty_pixels;

		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = add_and_clamp(blend_over(palette[PIXEL(x,y,src,src,16)], PIXEL(x,y,over,dst,32), PIXEL(x,y,overyrgb,dst,32)), PIXEL(x,y,und,dst,32));
			}
		}

		/* 32bpp case */
		else
		{
			while (*list != VECTOR_PIXEL_END)
			{
				vector_pixel_t coords = *list;
				x = VECTOR_PIXEL_X(coords);
				y = VECTOR_PIXEL_Y(coords);
				*list++ = coords + offset;
				PIXEL(x,y,dst,dst,32) = add_and_clamp(blend_over(PIXEL(x,y,src,src,32), PIXEL(x,y,over,dst,32), PIXEL(x,y,overyrgb,dst,32)), PIXEL(x,y,und,dst,32));
			}
		}

		return;
	}

	/* 1x scale */
	if (gamescale == 1)
	{
		/* 16/15bpp case */
		if (bitmap->depth != 32)
		{
			for (y = 0; y < height; y++)
			{
				UINT16 *src = (UINT16 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				UINT32 *und = (UINT32 *)undbase + y * dstrowpixels;
				UINT32 *over = (UINT32 *)overbase + y * dstrowpixels;
				UINT32 *overyrgb = (UINT32 *)overyrgbbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = add_and_clamp(blend_over(palette[*src++], *over++, *overyrgb++), *und++);
			}
		}

		/* 32bpp case */
		else
		{
			for (y = 0; y < height; y++)
			{
				UINT32 *src = (UINT32 *)srcbase + y * srcrowpixels + Machine->absolute_visible_area.min_x;
				UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
				UINT32 *und = (UINT32 *)undbase + y * dstrowpixels;
				UINT32 *over = (UINT32 *)overbase + y * dstrowpixels;
				UINT32 *overyrgb = (UINT32 *)overyrgbbase + y * dstrowpixels;
				for (x = 0; x < width; x++)
					*dst++ = add_and_clamp(blend_over(*src++, *over++, *overyrgb++), *und++);
			}
		}

		return;
	}

	/* for any other scale use the OSD layer to strech the game bitmap with effects */
	src_dx = width;
	src_dy = height;
	src_dp = bitmap->depth != 32 ? 2 : 4;
	src_dw = bitmap->rowbytes;
	src_ptr = (UINT8 *)bitmap->base + Machine->absolute_visible_area.min_y * src_dw + Machine->absolute_visible_area.min_x * src_dp;

	scaled_dx = width * gamescale;
	scaled_dy = height * gamescale;
	scaled_dp = 4;
	scaled_dw = scaled_dx * scaled_dp;
	scaled_ptr = malloc(scaled_dy * scaled_dw);

	/* we have only two case, palette16 or rgb32 */
	if (src_dp == 2) {
		osd_stretch_palett16to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw, palette);
	} else {
		osd_stretch_32to32(scaled_ptr, scaled_dx, scaled_dy, scaled_dw, src_ptr, src_dx, src_dy, src_dw);
	}

	/* draw the scaled bitmap */
	for (y = 0; y < scaled_dy; y++)
	{
		UINT32 *src = (UINT32*)(scaled_ptr + y * scaled_dw);
		UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
		UINT32 *und = (UINT32 *)undbase + y * dstrowpixels;
		UINT32 *over = (UINT32 *)overbase + y * dstrowpixels;
		UINT32 *overyrgb = (UINT32 *)overyrgbbase + y * dstrowpixels;

		if (gamescale == 2) {
			for (x = 0; x < width; x++)
			{
				dst[0] = add_and_clamp(blend_over(src[0], over[0], overyrgb[0]), und[0]);
				dst[1] = add_and_clamp(blend_over(src[1], over[1], overyrgb[1]), und[1]);
				src += 2;
				dst += 2;
				und += 2;
				over += 2;
				overyrgb += 2;
			}
		} else if (gamescale == 3) {
			for (x = 0; x < width; x++)
			{
				dst[0] = add_and_clamp(blend_over(src[0], over[0], overyrgb[0]), und[0]);
				dst[1] = add_and_clamp(blend_over(src[1], over[1], overyrgb[1]), und[1]);
				dst[2] = add_and_clamp(blend_over(src[2], over[2], overyrgb[2]), und[2]);
				src += 3;
				dst += 3;
				und += 3;
				over += 3;
				overyrgb += 3;
			}
		} else if (gamescale == 4) {
			for (x = 0; x < width; x++)
			{
				dst[0] = add_and_clamp(blend_over(src[0], over[0], overyrgb[0]), und[0]);
				dst[1] = add_and_clamp(blend_over(src[1], over[1], overyrgb[1]), und[1]);
				dst[2] = add_and_clamp(blend_over(src[2], over[2], overyrgb[2]), und[2]);
				dst[3] = add_and_clamp(blend_over(src[3], over[3], overyrgb[3]), und[3]);
				src += 4;
				dst += 4;
				und += 4;
				over += 4;
				overyrgb += 4;
			}
		}
	}

	free(scaled_ptr);
}



/*-------------------------------------------------
    render_ui_overlay - render the UI overlay
-------------------------------------------------*/

static void render_ui_overlay(mame_bitmap *bitmap, UINT32 *dirty, const rgb_t *palette, mame_display *display)
{
	int srcrowpixels = bitmap->rowpixels;
	int dstrowpixels = final->rowpixels;
	void *srcbase, *dstbase;
	int width, height;
	int x, y, h;

	/* compute common parameters */
	width = bitmap->width;
	height = bitmap->height;
	srcbase = bitmap->base;
	dstbase = final->base;

	/* 16/15bpp case */
	if (bitmap->depth != 32)
	{
		for (y = 0; y < height; y++)
		{
			UINT16 *src = (UINT16 *)srcbase + y * srcrowpixels;
			UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
			UINT32 *hint = &dirty[y * MAX_HINTS_PER_SCANLINE];
			for (h = 0; h < MAX_HINTS_PER_SCANLINE && hint[h] != 0; h++)
			{
				int start = hint[h] >> 16;
				int stop = hint[h] & 0xffff;
				hint[h] = 0;

				for (x = start; x <= stop; x++)
				{
					int pix = src[x];
					if (pix != UI_TRANSPARENT_COLOR16)
					{
						dst[x] = palette[pix];
						src[x] = UI_TRANSPARENT_COLOR16;
					}
				}
			}
		}
	}

	/* 32bpp case */
	else
	{
		for (y = 0; y < height; y++)
		{
			UINT32 *src = (UINT32 *)srcbase + y * srcrowpixels;
			UINT32 *dst = (UINT32 *)dstbase + y * dstrowpixels;
			UINT32 *hint = &dirty[y * MAX_HINTS_PER_SCANLINE];
			for (h = 0; h < MAX_HINTS_PER_SCANLINE && hint[h] != 0; h++)
			{
				int start = hint[h] >> 16;
				int stop = hint[h] & 0xffff;
				hint[h] = 0;

				for (x = start; x <= stop; x++)
				{
					int pix = src[x];
					if (pix != UI_TRANSPARENT_COLOR32)
					{
						dst[x] = pix;
						src[x] = UI_TRANSPARENT_COLOR32;
					}
				}
			}
		}
	}
}



/*-------------------------------------------------
    artwork_load_artwork_file - default MAME way
    to locate an artwork file
-------------------------------------------------*/

mame_file *artwork_load_artwork_file(const game_driver **driver)
{
    /* Retained only for ABI compatibility with the video callback table. */
    (void)driver;
    return NULL;
}



#if 0
#pragma mark -
#pragma mark BITMAP LOADING/MANIPULATING
#endif

/*-------------------------------------------------
    artwork_load - locate the .art file and
    read all the bitmaps
-------------------------------------------------*/

static int artwork_load(const game_driver *driver, int width, int height, const artwork_callbacks *callbacks)
{
    const artwork_overlay_piece *built_in = overlay_list;
    artwork_piece *piece;
    const char *requested = options.artwork_view ? options.artwork_view : "auto";
    (void)callbacks;
    num_pieces = num_underlays = num_overlays = num_bezels = 0;
    overlay_list = NULL;
    artwork_list = NULL;
    artwork_lay_close();
    if (lay_scaled_game) {
        free(lay_scaled_game);
        lay_scaled_game = NULL;
    }

    if (!strcmp(requested, "none")) return 1;
    if (!options.use_artwork) return 1;

    artwork_lay_open(driver, requested, width, height, options.use_artwork);
    if (artwork_lay_has_backdrop()) num_underlays = 1;

    /* A selected external overlay replaces, never compounds, the built-in one. */
    if (built_in && (options.use_artwork & ARTWORK_USE_OVERLAYS) &&
        !artwork_lay_has_overlay() && !artwork_lay_screen_only()) {
        if (!generate_overlay(built_in, width, height))
            printf("artwork: unable to generate built-in overlay\n");
    }
    /* Built-in overlays are created in reverse linked-list order. Preserve
     * the original priority sort and category counts. */
    sort_pieces();
    for (piece = artwork_list; piece; piece = piece->next) {
        if (piece->layer == LAYER_OVERLAY) num_overlays++;
        else if (piece->layer == LAYER_BACKDROP) num_underlays++;
        else if (piece->layer >= LAYER_BEZEL) num_bezels++;
    }
    return 1;
}



/*-------------------------------------------------
    artwork_prep - prepare the artwork
-------------------------------------------------*/

static int artwork_prep(void)
{
	artwork_piece *piece;

	/* mark everything dirty */
	underlay_invalid = screenrect;
	overlay_invalid = screenrect;
	bezel_invalid = screenrect;

	if (lay_scaled_game) {
		free(lay_scaled_game);
		lay_scaled_game = NULL;
	}
	if (artwork_lay_screen_transformed()) {
		const int in_w = Machine->absolute_visible_area.max_x - Machine->absolute_visible_area.min_x + 1;
		const int in_h = Machine->absolute_visible_area.max_y - Machine->absolute_visible_area.min_y + 1;
		const int sw = in_w * gamescale, sh = in_h * gamescale;
		if (sw > 0 && sh > 0 && (size_t)sw <= SIZE_MAX / (size_t)sh / sizeof(UINT32)) {
			size_t bytes = (size_t)sw * sh * sizeof(UINT32);
			lay_scaled_game = (UINT32 *)malloc(bytes);
			if (!lay_scaled_game)
				printf("artwork: cannot allocate transformed game bitmap\n");
		}
	}

	if (artwork_lay_active() && !artwork_lay_prepare(screenrect.max_x + 1,
			screenrect.max_y + 1, &gamerect)) {
		printf("artwork: unable to prepare layout images\n");
		artwork_lay_close();
	}

	/* loop through all the pieces, generating the scaled bitmaps */
	for (piece = artwork_list; piece; piece = piece->next)
	{
		/* scale to the artwork's intended dimensions */
		if (!scale_bitmap(piece, piece->bounds.max_x - piece->bounds.min_x + 1, piece->bounds.max_y - piece->bounds.min_y + 1))
			return 1;

		/* trim the bitmap down if transparent */
		trim_bitmap(piece);

		/* do we intersect the game rect? */
		piece->intersects_game = 0;
		if (piece->bounds.max_x > gamerect.min_x && piece->bounds.min_x < gamerect.max_x &&
			piece->bounds.max_y > gamerect.min_y && piece->bounds.min_y < gamerect.max_y)
			piece->intersects_game = 1;
	}
	return 0;
}



/*-------------------------------------------------
    scale_bitmap - scale the bitmap for a
    given piece of artwork
-------------------------------------------------*/

static int scale_bitmap(artwork_piece *piece, int newwidth, int newheight)
{
	UINT32 global_brightness, global_alpha;
	UINT64 sumscale;
	UINT32 dx, dy;
	int x, y;

	/* skip if no bitmap */
	if (!piece->rawbitmap)
		return 1;

	/* allocate two new bitmaps */
	piece->prebitmap = auto_bitmap_alloc_depth(newwidth, newheight, -32);
	piece->yrgbbitmap = auto_bitmap_alloc_depth(newwidth, newheight, -32);
	if (!piece->prebitmap || !piece->yrgbbitmap)
		return 0;

	/* also allocate memory for the scanline hints */
	piece->scanlinehint = auto_malloc(newheight * MAX_HINTS_PER_SCANLINE * sizeof(piece->scanlinehint[0]));
	memset(piece->scanlinehint, 0, newheight * MAX_HINTS_PER_SCANLINE * sizeof(piece->scanlinehint[0]));

	/* convert global brightness and alpha to fixed point */
	global_brightness = (int)(piece->brightness * 65536.0);
	global_alpha = (int)(piece->alpha * 65536.0);

	/* compute parameters for scaling */
	dx = (piece->rawbitmap->width << 12) / newwidth;
	dy = (piece->rawbitmap->height << 12) / newheight;
	sumscale = (UINT64)dx * (UINT64)dy;

	/* loop over the target vertically */
	for (y = 0; y < newheight; y++)
	{
		int prevstate = 0, statex = 0;
		int newstate;

		UINT32 starty = y * dy;

		/* loop over the target horizontally */
		for (x = 0; x < newwidth; x++)
		{
			UINT32 startx = x * dx;
			UINT32 a, r, g, b;

			/* if the source is higher res than the target, use full averaging */
			if (dx > 0x1000 || dy > 0x1000)
			{
				UINT64 sumr = 0, sumg = 0, sumb = 0, suma = 0;
				UINT32 xchunk, ychunk;
				UINT32 curx, cury;

				UINT32 yremaining = dy;

				/* accumulate all source pixels that contribute to this pixel */
				for (cury = starty; yremaining; cury += ychunk)
				{
					UINT32 xremaining = dx;

					/* determine the Y contribution, clamping to the amount remaining */
					ychunk = 0x1000 - (cury & 0xfff);
					if (ychunk > yremaining)
						ychunk = yremaining;
					yremaining -= ychunk;

					/* loop over all source pixels in the X direction */
					for (curx = startx; xremaining; curx += xchunk)
					{
						UINT32 factor;
						UINT32 pix;

						/* determine the X contribution, clamping to the amount remaining */
						xchunk = 0x1000 - (curx & 0xfff);
						if (xchunk > xremaining)
							xchunk = xremaining;
						xremaining -= xchunk;

						/* total contribution = x * y */
						factor = xchunk * ychunk;

						/* fetch the source pixel */
						pix = ((UINT32 *)piece->rawbitmap->line[cury >> 12])[curx >> 12];

						/* accumulate the RGBA values */
						sumr += factor * RGB_RED(pix);
						sumg += factor * RGB_GREEN(pix);
						sumb += factor * RGB_BLUE(pix);
						suma += factor * RGB_ALPHA(pix);
					}
				}

				/* apply final scale */
				a = suma / sumscale;
				r = sumr / sumscale;
				g = sumg / sumscale;
				b = sumb / sumscale;
			}

			/* otherwise, use bilinear filtering to scale up */
			else
			{
				UINT32 pix0, pix1, pix2, pix3;
				UINT32 sumr, sumg, sumb, suma;
				UINT32 nextx, nexty;
				UINT32 curx, cury;
				UINT32 factor;

				/* adjust start to the center */
				curx = startx + dx / 2 - 0x800;
				cury = starty + dy / 2 - 0x800;

				/* compute the neighboring pixel */
				nextx = curx + 0x1000;
				nexty = cury + 0x1000;

				/* clamp start */
				if ((INT32)curx < 0) curx += 0x1000;
				if ((INT32)cury < 0) cury += 0x1000;

				/* fetch the four relevant pixels */
				pix0 = ((UINT32 *)piece->rawbitmap->line[cury >> 12])[curx >> 12];
				pix1 = ((UINT32 *)piece->rawbitmap->line[cury >> 12])[nextx >> 12];
				pix2 = ((UINT32 *)piece->rawbitmap->line[nexty >> 12])[curx >> 12];
				pix3 = ((UINT32 *)piece->rawbitmap->line[nexty >> 12])[nextx >> 12];

				/* compute the x/y scaling factors */
				curx &= 0xfff;
				cury &= 0xfff;

				/* contributions from pixel 0 (top,left) */
				factor = (0x1000 - curx) * (0x1000 - cury);
				sumr = factor * RGB_RED(pix0);
				sumg = factor * RGB_GREEN(pix0);
				sumb = factor * RGB_BLUE(pix0);
				suma = factor * RGB_ALPHA(pix0);

				/* contributions from pixel 1 (top,right) */
				factor = curx * (0x1000 - cury);
				sumr += factor * RGB_RED(pix1);
				sumg += factor * RGB_GREEN(pix1);
				sumb += factor * RGB_BLUE(pix1);
				suma += factor * RGB_ALPHA(pix1);

				/* contributions from pixel 2 (bottom,left) */
				factor = (0x1000 - curx) * cury;
				sumr += factor * RGB_RED(pix2);
				sumg += factor * RGB_GREEN(pix2);
				sumb += factor * RGB_BLUE(pix2);
				suma += factor * RGB_ALPHA(pix2);

				/* contributions from pixel 3 (bottom,right) */
				factor = curx * cury;
				sumr += factor * RGB_RED(pix3);
				sumg += factor * RGB_GREEN(pix3);
				sumb += factor * RGB_BLUE(pix3);
				suma += factor * RGB_ALPHA(pix3);

				/* apply final scale */
				r = sumr >> 24;
				g = sumg >> 24;
				b = sumb >> 24;
				a = suma >> 24;
			}

			/* apply alpha and brightness */
			a = (a * global_alpha) >> 16;
			r = (r * global_brightness) >> 16;
			g = (g * global_brightness) >> 16;
			b = (b * global_brightness) >> 16;

			/* store to both bitmaps */
			*((UINT32 *)piece->prebitmap->base + y * piece->prebitmap->rowpixels + x) = compute_pre_pixel(a,r,g,b);
			*((UINT32 *)piece->yrgbbitmap->base + y * piece->yrgbbitmap->rowpixels + x) = compute_yrgb_pixel(a,r,g,b);

			/* look for state changes */
			newstate = (a != 0);
			if (newstate != prevstate)
			{
				prevstate = newstate;

				/* if starting a new run of non-transparent pixels, remember the start point */
				if (newstate)
					statex = x;

				/* otherwise, add the current run */
				else
					add_range_to_hint(piece->scanlinehint, y, statex, x - 1);
			}
		}

		/* add the final range */
		if (prevstate)
			add_range_to_hint(piece->scanlinehint, y, statex, x - 1);
	}

	/* guess it worked! */
	return 1;
}



/*-------------------------------------------------
    trim_bitmap - remove any transparent borders
    from a scaled image
-------------------------------------------------*/

static void trim_bitmap(artwork_piece *piece)
{
	UINT32 *hintbase = piece->scanlinehint;
	int top, bottom, left, right;
	int x, y, height, width;

	/* skip if no bitmap */
	if (!piece->rawbitmap)
		return;

	/* don't trim overlay bitmaps */
	if (piece->layer == LAYER_OVERLAY)
		return;

	/* scan from the top down, looking for empty rows */
	height = piece->prebitmap->height;
	width = piece->prebitmap->width;
	for (top = 0; top < height; top++)
		if (hintbase[top * MAX_HINTS_PER_SCANLINE] != 0)
			break;

	/* scan from the bottom up, looking for empty rows */
	for (bottom = height - 1; bottom >= top; bottom--)
		if (hintbase[bottom * MAX_HINTS_PER_SCANLINE] != 0)
			break;

	/* now find the min/max */
	left = width - 1;
	right = 0;
	for (y = top; y <= bottom; y++)
	{
		const UINT32 *hintdata = &hintbase[y * MAX_HINTS_PER_SCANLINE];

		/* check the minimum against the left */
		if (hintdata[0] && (hintdata[0] >> 16) < left)
			left = hintdata[0] >> 16;

		/* find the maximum */
		for (x = 0; x < MAX_HINTS_PER_SCANLINE; x++)
			if (hintdata[x] && (hintdata[x] & 0xffff) > right)
				right = hintdata[x] & 0xffff;
	}

	logerror("Trimming bitmap from (%d,%d)-(%d,%d) to (%d,%d)-(%d,%d)\n",
			piece->bounds.min_x, piece->bounds.min_y, piece->bounds.max_x, piece->bounds.max_y,
			piece->bounds.min_x + left, piece->bounds.min_y + top, piece->bounds.min_x + right, piece->bounds.min_y + bottom);

	/* skip if all is normal */
	if (left == 0 && top == 0 && right == width - 1 && bottom == height - 1)
		return;

	/* now shift the bitmap data */
	for (y = top; y <= bottom; y++)
	{
		UINT32 *hintsrc = &hintbase[y * MAX_HINTS_PER_SCANLINE];
		UINT32 *hintdst = &hintbase[(y - top) * MAX_HINTS_PER_SCANLINE];
		UINT32 *dst1 = (UINT32 *)piece->prebitmap->base + (y - top) * piece->prebitmap->rowpixels;
		UINT32 *dst2 = (UINT32 *)piece->yrgbbitmap->base + (y - top) * piece->yrgbbitmap->rowpixels;
		UINT32 *src1 = (UINT32 *)piece->prebitmap->base + y * piece->prebitmap->rowpixels + left;
		UINT32 *src2 = (UINT32 *)piece->yrgbbitmap->base + y * piece->yrgbbitmap->rowpixels + left;

		memmove(dst1, src1, (right - left + 1) * sizeof(UINT32));
		memmove(dst2, src2, (right - left + 1) * sizeof(UINT32));

		/* adjust the hints */
		for (x = 0; x < MAX_HINTS_PER_SCANLINE; x++)
		{
			UINT32 data = hintsrc[x];
			if (data)
				data -= (left << 16) | left;
			hintdst[x] = data;
		}
	}

	/* and adjust the info */
	piece->bounds.max_x = piece->bounds.min_x + right;
	piece->bounds.min_x += left;
	piece->bounds.max_y = piece->bounds.min_y + bottom;
	piece->bounds.min_y += top;
}




#if 0
#pragma mark -
#pragma mark PIECE LIST MANAGEMENT
#endif

/*-------------------------------------------------
    create_new_piece - allocate a new piece
    entry
-------------------------------------------------*/

static artwork_piece *create_new_piece(const char *tag)
{
	/* allocate a new piece */
	artwork_piece *newpiece = auto_malloc(sizeof(artwork_piece));
	num_pieces++;

	/* initialize to default values */
	memset(newpiece, 0, sizeof(*newpiece));
	newpiece->layer = LAYER_UNKNOWN;
	newpiece->has_alpha = 0;
	newpiece->priority = 0;
	newpiece->alpha = 1.0;
	newpiece->brightness = 1.0;
	newpiece->filename = NULL;
	newpiece->alpha_filename = NULL;
	newpiece->intersects_game = 0;
	newpiece->visible = 1;

	/* allocate space for the filename */
	newpiece->tag = auto_malloc(strlen(tag) + 1);
	strcpy(newpiece->tag, tag);

	/* link into the list */
	newpiece->next = artwork_list;
	artwork_list = newpiece;
	return newpiece;
}



/*-------------------------------------------------
    artwork_sort_compare - qsort compare function
    to sort pieces by priority
-------------------------------------------------*/

static int CLIB_DECL artwork_sort_compare(const void *item1, const void *item2)
{
	const artwork_piece *piece1 = *((const artwork_piece **)item1);
	const artwork_piece *piece2 = *((const artwork_piece **)item2);
	if (piece1->layer < piece2->layer)
		return -1;
	else if (piece1->layer > piece2->layer)
		return 1;
	else if (piece1->priority < piece2->priority)
		return -1;
	else if (piece1->priority > piece2->priority)
		return 1;
	else
		return 0;
}



/*-------------------------------------------------
    sort_pieces - sort the pieces by priority
-------------------------------------------------*/

static void sort_pieces(void)
{
	artwork_piece *array[MAX_PIECES];
	artwork_piece *piece;
	int i = 0;

	/* copy the list into the array, filtering as we go */
	for (piece = artwork_list; piece; piece = piece->next)
	{
		switch (piece->layer)
		{
			case LAYER_BACKDROP:
				if (options.use_artwork & ARTWORK_USE_BACKDROPS)
					array[i++] = piece;
				break;

			case LAYER_OVERLAY:
				if (options.use_artwork & ARTWORK_USE_OVERLAYS)
					array[i++] = piece;
				break;

			default:
				if (options.use_artwork & ARTWORK_USE_BEZELS)
					array[i++] = piece;
				break;
		}
	}
	num_pieces = i;
	if (num_pieces == 0)
	{
		artwork_list = NULL;
		return;
	}

	/* now sort it */
	if (num_pieces > 1)
		qsort(array, num_pieces, sizeof(array[0]), artwork_sort_compare);

	/* now reassemble the list */
	artwork_list = piece = array[0];
	for (i = 1; i < num_pieces; i++)
	{
		piece->next = array[i];
		piece = piece->next;
	}
	piece->next = NULL;
}



/*-------------------------------------------------
    validate_pieces - make sure we got valid data
-------------------------------------------------*/





#if 0
#pragma mark -
#pragma mark OVERLAY GENERATION
#endif

/*-------------------------------------------------
    generate_rect_piece - generate a rectangular
    overlay piece
-------------------------------------------------*/

static int generate_rect_piece(artwork_piece *piece, const artwork_overlay_piece *data, int width, int height)
{
	int gfxwidth, gfxheight;

	/* extract coordinates */
	piece->top = data->top;
	piece->left = data->left;
	piece->bottom = data->bottom;
	piece->right = data->right;

	/* convert from pixel coordinates if necessary */
	if (fabs(piece->left) > 4.0 || fabs(piece->right) > 4.0 ||
		fabs(piece->top) > 4.0 || fabs(piece->bottom) > 4.0)
	{
		piece->left /= (double)width;
		piece->right /= (double)width;
		piece->top /= (double)height;
		piece->bottom /= (double)height;
	}

	/* compute the effective width/height */
	gfxwidth = (int)((piece->right - piece->left) * (double)width * 2.0 + 0.5);
	gfxheight = (int)((piece->bottom - piece->top) * (double)height * 2.0 + 0.5);

	/* allocate a source bitmap 2x the game bitmap's size */
	piece->rawbitmap = auto_bitmap_alloc_depth(gfxwidth, gfxheight, 32);
	if (!piece->rawbitmap)
		return 0;

	/* fill the bitmap */
	fillbitmap(piece->rawbitmap, data->color, NULL);
	return 1;
}



/*-------------------------------------------------
    generate_disk_piece - generate a disk-shaped
    overlay piece
-------------------------------------------------*/

static void render_disk(mame_bitmap *bitmap, int r, UINT32 color)
{
	int xc = bitmap->width / 2, yc = bitmap->height / 2;
	int x = 0, twox = 0;
	int y = r;
	int twoy = r+r;
	int p = 1 - r;
	int i;

	while (x < y)
	{
		x++;
		twox +=2;
		if (p < 0)
			p += twox + 1;
		else
		{
			y--;
			twoy -= 2;
			p += twox - twoy + 1;
		}

		for (i = 0; i < twox; i++)
		{
			plot_pixel(bitmap, xc-x+i, yc-y, color);
			plot_pixel(bitmap, xc-x+i, yc+y-1, color);
		}

		for (i = 0; i < twoy; i++)
		{
			plot_pixel(bitmap, xc-y+i, yc-x, color);
			plot_pixel(bitmap, xc-y+i, yc+x-1, color);
		}
	}
}


static int generate_disk_piece(artwork_piece *piece, const artwork_overlay_piece *data, int width, int height)
{
	double x = data->left, y = data->top, r = data->right;
	rectangle temprect;
	int gfxwidth, gfxradius;

	/* convert from pixel coordinates if necessary */
	if (fabs(x) > 4.0 || fabs(y) > 4.0 || fabs(r) > 4.0)
	{
		x /= (double)width;
		y /= (double)height;
		r /= (double)width;
	}

	/* generate coordinates */
	piece->top = y - r * (double)width / (double)height;
	piece->left = x - r;
	piece->bottom = y + r * (double)width / (double)height;
	piece->right = x + r;

	/* compute the effective width/height */
	gfxwidth = (int)((piece->right - piece->left) * (double)width * 2.0 + 0.5);
	gfxradius = (int)(r * (double)width * 2.0 + 0.5);

	/* allocate a source bitmap 2x the game bitmap's size */
	piece->rawbitmap = auto_bitmap_alloc_depth(gfxwidth, gfxwidth, 32);
	if (!piece->rawbitmap)
		return 0;

	/* fill the bitmap with white */
	temprect.min_x = temprect.min_y = 0;
	temprect.max_x = piece->rawbitmap->width - 1;
	temprect.max_y = piece->rawbitmap->height - 1;
	erase_rect(piece->rawbitmap, &temprect, MAKE_ARGB(0,0xff,0xff,0xff));

	/* now render the disk */
	render_disk(piece->rawbitmap, gfxradius, data->color);
	return 1;
}



/*-------------------------------------------------
    generate_overlay - generate an overlay with
    the given pieces
-------------------------------------------------*/

static int generate_overlay(const artwork_overlay_piece *list, int width, int height)
{
	artwork_piece *piece;
	int priority = 0;

	/* loop until done */
	while (list->type != OVERLAY_TYPE_END)
	{
		/* first create a new piece to use */
		piece = create_new_piece(OVERLAY_TAG);
		if (!piece)
			return 0;

		/* fill in the basics */
		piece->has_alpha = 1;
		piece->layer = LAYER_OVERLAY;
		piece->priority = priority++;
		piece->blendflags = list->type & OVERLAY_FLAG_MASK;

		/* switch off the type */
		switch (list->type & ~OVERLAY_FLAG_MASK)
		{
			case OVERLAY_TYPE_RECTANGLE:
				if (!generate_rect_piece(piece, list, width, height))
					return 0;
				break;

			case OVERLAY_TYPE_DISK:
				if (!generate_disk_piece(piece, list, width, height))
					return 0;
				break;
		}

		/* next */
		list++;
	}

	return 1;
}



#if 0
#pragma mark -
#pragma mark ART FILE PARSING
#endif

/*-------------------------------------------------
    compute_rgb_components - compute the RGB
    components
-------------------------------------------------*/

static int compute_rgb_components(int depth, UINT32 rgb_components[3], UINT32 rgb32_components[3])
{
	UINT32 temp;
	int r, g, b;

	/* first convert the RGB components we got back into shifts */
	for (temp = rgb32_components[0], rshift = 0; !(temp & 1); temp >>= 1)
		rshift++;
	for (temp = rgb32_components[1], gshift = 0; !(temp & 1); temp >>= 1)
		gshift++;
	for (temp = rgb32_components[2], bshift = 0; !(temp & 1); temp >>= 1)
		bshift++;

	/* compute the alpha shift for the leftover byte */
	nonalpha_mask = rgb32_components[0] | rgb32_components[1] | rgb32_components[2];
	temp = ~nonalpha_mask;
	for (ashift = 0; !(temp & 1); temp >>= 1)
		ashift++;

	/* compute a transparent color; this is in the premultiplied space, so alpha is inverted */
	transparent_color = ASSEMBLE_ARGB(0xff,0x00,0x00,0x00);

	/* allocate a palette lookup */
	palette_lookup = auto_malloc(65536 * sizeof(palette_lookup[0]));

	/* switch off the depth */
	switch (depth)
	{
		case 16:
			/* do nothing */
			break;

		case 32:
			/* copy original components */
			memcpy(rgb_components, rgb32_components, sizeof(rgb_components[0])*3);
			break;

		case 15:
			/* make up components */
			rgb_components[0] = 0x7c00;
			rgb_components[1] = 0x03e0;
			rgb_components[2] = 0x001f;

			/* now build up the palette */
			for (r = 0; r < 32; r++)
				for (g = 0; g < 32; g++)
					for (b = 0; b < 32; b++)
					{
						int rr = (r << 3) | (r >> 2);
						int gg = (g << 3) | (g >> 2);
						int bb = (b << 3) | (b >> 2);
						palette_lookup[(r << 10) | (g << 5) | b] = ASSEMBLE_ARGB(0, rr, gg, bb);
					}
			break;
	}

	return 0;
}



/*-------------------------------------------------
    add_range_to_hint - add a given range to the
    hint record for the specified scanline
-------------------------------------------------*/

static void add_range_to_hint(UINT32 *hintbase, int scanline, int startx, int endx)
{
	int closestdiff = 100000, closestindex = -1;
	int i;

	/* first address the correct hint */
	hintbase += scanline * MAX_HINTS_PER_SCANLINE;

	/* first, look for an intersection */
	for (i = 0; i < MAX_HINTS_PER_SCANLINE; i++)
	{
		UINT32 hint = hintbase[i];
		int hintstart = hint >> 16;
		int hintend = hint & 0xffff;
		int diff;

		/* stop if we hit a 0 */
		if (hint == 0)
			break;

		/* do we intersect? */
		if (startx <= hintend && endx >= hintstart)
		{
			closestindex = i;
			goto intersect;
		}

		/* see how close we are to this entry */
		if (hintend < startx)
			diff = startx - hintend;
		else
			diff = hintstart - endx;

		/* if this is the closest, remember it */
		if (diff < closestdiff)
		{
			closestdiff = diff;
			closestindex = i;
		}
	}

	/* okay, we didn't find an intersection; do we have room to add? */
	if (i < MAX_HINTS_PER_SCANLINE)
	{
		UINT32 newhint = (startx << 16) | endx;

		/* if there's nothing there yet, just assign to the first entry */
		if (i == 0)
			hintbase[0] = newhint;

		/* otherwise, shuffle the existing entries to make room for us */
		else
		{
			/* determine our new index */
			i = closestindex;
			if (hintbase[i] < newhint)
				i++;

			/* shift things over */
			if (i < MAX_HINTS_PER_SCANLINE - 1)
				memmove(&hintbase[i+1], &hintbase[i], (MAX_HINTS_PER_SCANLINE - (i+1)) * sizeof(hintbase[0]));
			hintbase[i] = newhint;
		}
		return;
	}

intersect:
	/* intersect with the closest entry */
	{
		UINT32 hint = hintbase[closestindex];
		int hintstart = hint >> 16;
		int hintend = hint & 0xffff;

		/* compute the intersection */
		if (startx < hintstart)
			hintstart = startx;
		if (endx > hintend)
			hintend = endx;
		hintbase[closestindex] = (hintstart << 16) | hintend;
	}
}

/*
 * Export some variables needed by OSD vector code in xmame.
 */
const rectangle *artwork_get_game_rect(void)
{
	if (artwork_list || artwork_lay_active())
		return &gamerect;
	else
		return NULL;
}

int artwork_overlay_active(void)
{
	return num_overlays || artwork_lay_has_overlay();
}

/* Draw the emulated screen into a transformed .lay screen rectangle.
 * Uses the existing OSD stretcher for game video effects before applying the
 * layout's orthogonal transform, keeping artwork images out of video filters. */
static void render_lay_transformed_game(mame_bitmap *bitmap,
                                        const rgb_t *palette,
                                        mame_display *display)
{
    const int in_w=Machine->absolute_visible_area.max_x-
                   Machine->absolute_visible_area.min_x+1;
    const int in_h=Machine->absolute_visible_area.max_y-
                   Machine->absolute_visible_area.min_y+1;
    const int sw=in_w*gamescale, sh=in_h*gamescale;
    const int dw=gamerect.max_x-gamerect.min_x+1;
    const int dh=gamerect.max_y-gamerect.min_y+1;
    const int depth=(bitmap->depth==32)?4:2;
    const UINT8 *src=(const UINT8*)bitmap->base+
        Machine->absolute_visible_area.min_y*bitmap->rowbytes+
        Machine->absolute_visible_area.min_x*depth;
    UINT32 *scaled=lay_scaled_game;
    double m[6],det;
    int x,y;
    if(!scaled||in_w<=0||in_h<=0||sw<=0||sh<=0||dw<=0||dh<=0) return;
    if(depth==2)
        osd_stretch_palett16to32((UINT8*)scaled,sw,sh,sw*4,
                                 src,in_w,in_h,bitmap->rowbytes,palette);
    else
        osd_stretch_32to32((UINT8*)scaled,sw,sh,sw*4,
                            src,in_w,in_h,bitmap->rowbytes);
    artwork_lay_screen_matrix(m);
    det=m[0]*m[4]-m[1]*m[3];
    if(fabs(det)<1e-12) return;
    for(y=0;y<dh;y++){
        UINT32 *dst=(UINT32*)final->line[gamerect.min_y+y];
        UINT32 *und=num_underlays?(UINT32*)underlay->line[gamerect.min_y+y]:NULL;
        for(x=0;x<dw;x++){
            double nx=(x+0.5)/(double)dw-m[2];
            double ny=(y+0.5)/(double)dh-m[5];
            double ux=(m[4]*nx-m[1]*ny)/det;
            double uy=(-m[3]*nx+m[0]*ny)/det;
            int sx=(int)(ux*sw),sy=(int)(uy*sh);
            UINT32 pix;
            if(sx<0||sx>=sw||sy<0||sy>=sh)continue;
            pix=scaled[(size_t)sy*sw+sx];
            if(num_overlays){
                int ox=(int)(ux*dw),oy=(int)(uy*dh);
                if(ox>=0&&ox<dw&&oy>=0&&oy<dh){
                    UINT32 ov=((UINT32*)overlay->line[gamerect.min_y+oy])[gamerect.min_x+ox];
                    UINT32 oyv=((UINT32*)overlay_yrgb->line[gamerect.min_y+oy])[gamerect.min_x+ox];
                    pix=blend_over(pix,ov,oyv);
                }
            }
            if(und)pix=add_and_clamp(pix,und[gamerect.min_x+x]);
            dst[gamerect.min_x+x]=pix;
        }
    }
    /* A transformed vector screen is fully composed, not incrementally
     * addressed using the untransformed vector dirty-pixel list. */
    if(display->changed_flags & VECTOR_PIXELS_CHANGED){
        display->changed_flags &= ~VECTOR_PIXELS_CHANGED;
        display->vector_dirty_pixels=NULL;
    }
}

