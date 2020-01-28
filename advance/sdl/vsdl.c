/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2008 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "portable.h"

#include "vsdl.h"
#include "video.h"
#include "log.h"
#include "ossdl.h"
#include "error.h"
#include "target.h"

#ifdef USE_KEYBOARD_SDL
#include "ksdl.h"
#endif

#if defined(USE_VIDEO_SVGALIB) || defined(USE_VIDEO_FB)
#include "oslinux.h"
#endif

#include "SDL.h"

#ifdef USE_SMP
#include <pthread.h>
#endif

/* If defined it ensures to close any screen reference/handle when calling */
/* the sdl_done() function. It's required to allow to start subprocess */
/* which need exclusive access at the screen */
/* #define USE_VIDEO_RESTORE */

/***************************************************************************/
/* Options */

typedef struct sdl_internal_struct {
	adv_bool active; /**< !=0 if present. */
	adv_bool mode_active; /**< !=0 if mode set. */
	adv_bool lock_active; /**< !=0 if lock active. */
	unsigned flags; /**< Precomputed driver flags. */
#if SDL_MAJOR_VERSION == 1
	/* With SDL1 we can use either a surface or a overlay */
	SDL_Surface* surface; /**< Surface. */
	SDL_Overlay* overlay; /**< Overlay. */
#else
	/* With SDL2 we use always an overlay */
	SDL_Window* window; /**< Window. */
	unsigned window_size_x; /**< Window size. */
	unsigned window_size_y;
	SDL_Renderer* renderer; /**< Render. */
	SDL_Texture* texture; /**< Texture */
#ifdef USE_SMP
	pthread_t thread; /**< Main thread for renderer and texture */
#endif
	void* overlay_ptr; /**< Overlay data pointer. */
	void* overlay_alloc; /**< Overlay allocated data. */
	unsigned overlay_pitch; /**< Pitch of the overlay. */
	adv_bool overlay_vsync; /**< Update happens in the vsync. */
#endif
	adv_output output; /**< Output mode. */
	adv_cursor cursor; /**< Cursor mode. */
	unsigned overlay_size_x;
	unsigned overlay_size_y;
	unsigned overlay_bit;
	unsigned index; /**< Screen index. */
} sdl_internal;

static sdl_internal sdl_state;

unsigned char* (*sdl_write_line)(unsigned y);

static adv_device DEVICE[] = {
	{ "auto", -1, "SDL video" },
	{ 0, 0, 0 }
};

/***************************************************************************/
/* Functions */

static adv_bool sdl_is_active(void)
{
	return sdl_state.active != 0;
}

static adv_bool sdl_mode_is_active(void)
{
	return sdl_state.mode_active != 0;
}

static unsigned sdl_flags(void)
{
	return sdl_state.flags;
}

#if SDL_MAJOR_VERSION == 1
static unsigned sdl_mode_flags(void)
{
	unsigned flags;

	switch (sdl_state.output) {
	default:
		assert(0);
	case adv_output_window:
		flags = 0; /* software surface */
		break;
#ifdef USE_VIDEO_RESTORE
	case adv_output_fullscreen:
		flags = SDL_FULLSCREEN;
		break;
	case adv_output_overlay:
		flags = SDL_FULLSCREEN;
		break;
#else
	case adv_output_fullscreen:
		flags = SDL_FULLSCREEN | SDL_HWSURFACE;
		break;
	case adv_output_overlay:
		flags = SDL_FULLSCREEN | SDL_HWSURFACE;
		break;
#endif
	}

	return flags;
}
#endif

#include "icondef.dat"

#if SDL_MAJOR_VERSION == 1
static void sdl_icon(void)
{
	SDL_Surface* surface;
	SDL_Color colors[ICON_PALETTE];
	unsigned i, x, y;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, ICON_SIZE, ICON_SIZE, 8, 0, 0, 0, 0);
	if (!surface) {
		log_std(("video:sdl: Failed sdl_icon() in SDL_CreateRGBSurface()\n"));
		return;
	}

	for (y = 0; y < ICON_SIZE; ++y) {
		unsigned char* p = (unsigned char*)surface->pixels + y * surface->pitch;
		for (x = 0; x < ICON_SIZE; ++x)
			p[x] = icon_pixel[y * ICON_SIZE + x];
	}

	for (i = 0; i < ICON_PALETTE; ++i) {
		colors[i].r = icon_palette[i * 3 + 0];
		colors[i].g = icon_palette[i * 3 + 1];
		colors[i].b = icon_palette[i * 3 + 2];
	}

	if (SDL_SetColors(surface, colors, 0, ICON_PALETTE) != 1) {
		log_std(("video:sdl: Failed sdl_icon() in SDL_SetColors()\n"));
		SDL_FreeSurface(surface);
		return;
	}

	SDL_WM_SetIcon(surface, icon_mask);

	SDL_FreeSurface(surface);
}
#else
static void sdl_icon(SDL_Window* window)
{
	SDL_Surface* surface;
	unsigned i, x, y;
	unsigned rmask, gmask, bmask, amask;
	uint32* data;

	data = malloc(ICON_SIZE * ICON_SIZE * 4);
	if (!data) {
		log_std(("video:sdl: Failed sdl_icon() in malloc()\n"));
		return;
	}

	for (y = 0; y < ICON_SIZE; ++y) {
		for (x = 0; x < ICON_SIZE; ++x) {
			unsigned mask = icon_mask[y * 4 + 3] | (icon_mask[y * 4 + 2] << 8) | (icon_mask[y * 4 + 1] << 16) | (icon_mask[y * 4 + 0] << 24);
			if (mask & (1 << (31 - x))) {
				unsigned pal = icon_pixel[y * ICON_SIZE + x] * 3;
				data[y * ICON_SIZE + x] = icon_palette[pal + 0]
					| (icon_palette[pal + 1] << 8)
					| (icon_palette[pal + 2] << 16)
					| 0xff000000;
			} else {
				data[y * ICON_SIZE + x] = 0;
			}
		}
	}

	surface = SDL_CreateRGBSurfaceFrom(data, ICON_SIZE, ICON_SIZE, 32, ICON_SIZE * 4, 0xff, 0xff00, 0xff0000, 0xff000000);
	if (!surface) {
		log_std(("video:sdl: Failed sdl_icon() in SDL_CreateRGBSurfaceFrom()\n"));
		return;
	}

	SDL_SetWindowIcon(window, surface);

	SDL_FreeSurface(surface);

	free(data);
}
#endif

static int abssum(int x1, int y1, int x2, int y2)
{
	return abs(x1 - x2) + abs(y1 - y2);
}

static adv_error sdl_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	char name[64];
	const adv_device* i;
	unsigned j;
	adv_bool has_window_manager;
#if SDL_MAJOR_VERSION == 1
	SDL_Rect** map; /* it must not be released */
	const SDL_VideoInfo* info;
#else
	int display, display_mac;
	int driver, driver_mac;
	SDL_DisplayMode info_dm;
#endif

	adv_bool initialized_now;

	assert(!sdl_is_active());

	log_std(("video:sdl: sdl_init(id:%d,output:%d,overlay_size:%d)\n", device_id, (unsigned)output, overlay_size));

	memset(&sdl_state, 0, sizeof(sdl_state));
	initialized_now = 0;

	if (sizeof(sdl_video_mode) > MODE_DRIVER_MODE_SIZE_MAX) {
		error_set("Invalid structure size.\n");
		goto err;
	}

	if (!os_internal_sdl_get()) {
		error_set("Unsupported without the SDL library.\n");
		goto err;
	}

#if defined(USE_VIDEO_SVGALIB)
	if (os_internal_svgalib_is_video_active()) {
		error_set("Not compatible with SVGALIB video.\n");
		goto err;
	}
#endif

#if defined(USE_VIDEO_FB)
	if (os_internal_fb_is_video_active()) {
		error_set("Not compatible with FrameBuffer video.\n");
		goto err;
	}
#endif

#ifdef USE_VC
	if (!target_wm()) {
		error_set("With VideoCore you can use SDL only from a Window Manager.\n");
		goto err;
	}
#endif

	sdl_state.cursor = cursor;

	i = DEVICE;
	while (i->name && i->id != device_id)
		++i;
	if (!i->name)
		goto err;

	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		log_std(("video:sdl: call SDL_InitSubSystem(SDL_INIT_VIDEO)\n"));

		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			error_set("Unable to intialize the SDL library, %s.\n", SDL_GetError());
			goto err;
		}

		initialized_now = 1;

		/* set the window information */
#if SDL_MAJOR_VERSION == 1
		SDL_WM_SetCaption(os_internal_sdl_title_get(), os_internal_sdl_title_get());
		sdl_icon();
#endif
	}

#if SDL_MAJOR_VERSION == 1
	/* print info */
	if (SDL_VideoDriverName(name, sizeof(name))) {
		log_std(("video:sdl: SDL driver %s\n", name));
	}

	info = SDL_GetVideoInfo();

	log_std(("video:sdl: video hw_available:%d\n", (unsigned)info->hw_available));
	log_std(("video:sdl: video wm_available:%d\n", (unsigned)info->wm_available));
	log_std(("video:sdl: video blit_hw:%d\n", (unsigned)info->blit_hw));
	log_std(("video:sdl: video blit_hw_CC:%d\n", (unsigned)info->blit_hw_CC));
	log_std(("video:sdl: video blit_hw_A:%d\n", (unsigned)info->blit_hw_A));
	log_std(("video:sdl: video blit_sw:%d\n", (unsigned)info->blit_sw));
	log_std(("video:sdl: video blit_sw_CC:%d\n", (unsigned)info->blit_sw_CC));
	log_std(("video:sdl: video blit_sw_A:%d\n", (unsigned)info->blit_sw_A));
	log_std(("video:sdl: video blit_fill:%d\n", (unsigned)info->blit_fill));
	log_std(("video:sdl: video video_mem:%d kBytes\n", (unsigned)info->video_mem));
	log_std(("video:sdl: video bit:%d byte:%d rgb:%d/%d:%d/%d:%d/%d\n",
		(unsigned)info->vfmt->BitsPerPixel, (unsigned)info->vfmt->BytesPerPixel,
		(unsigned)8 - info->vfmt->Rloss, (unsigned)info->vfmt->Rshift,
		(unsigned)8 - info->vfmt->Gloss, (unsigned)info->vfmt->Gshift,
		(unsigned)8 - info->vfmt->Bloss, (unsigned)info->vfmt->Bshift
		));
	log_std(("video:sdl: video current_w:%d\n", (unsigned)info->current_w));
	log_std(("video:sdl: video current_h:%d\n", (unsigned)info->current_h));

	if (info->wm_available) {
		has_window_manager = 1;
	} else {
		/* in Mac OS X Quartz and SDL 1.2.5 the wm_available flag is not set */
		log_std(("video:sdl: SDL_ListModes(0,0)\n"));
		map = SDL_ListModes(0, 0);
		if (map == 0) {
			log_std(("video:sdl: no resolutions\n"));
			has_window_manager = 0;
		} else if (map == (SDL_Rect**)-1) {
			/* all resolution available */
			log_std(("video:sdl: all resolutions\n"));
			has_window_manager = 1;
		} else {
			log_std(("video:sdl: some resolutions\n"));
			has_window_manager = 0;
		}
	}

	if (output == adv_output_auto) {
		if (has_window_manager) {
#ifdef USE_VIDEO_RESTORE
			sdl_state.output = adv_output_window;
#else
			sdl_state.output = adv_output_overlay;
#endif
		} else {
			sdl_state.output = adv_output_fullscreen;
		}
	} else {
		sdl_state.output = output;
	}

	if (sdl_state.output == adv_output_window) {
		/* reduce a little to allow space for decorations */
		target_size_set(info->current_w - info->current_w / 8, info->current_h - info->current_h / 8);
	} else {
		target_size_set(info->current_w, info->current_h);
	}

	/* get the list of modes */
	log_std(("video:sdl: SDL_ListModes(0, sdl_mode_flags())\n"));
	map = SDL_ListModes(0, sdl_mode_flags());
	if (map == 0) {
		log_std(("video:sdl: no resolutions\n"));
	} else if (map == (SDL_Rect**)-1) {
		log_std(("video:sdl: all resolutions\n"));
	} else {
		for (j = 0; map[j]; ++j) {
			log_std(("video:sdl: mode %dx%d\n", (unsigned)map[j]->w, (unsigned)map[j]->h));
		}
	}

	if (sdl_state.output == adv_output_window && !has_window_manager) {
		error_set("The 'window' output mode is not available without a Window Manager.\n");
		goto err_quit;
	}

	if (sdl_state.output == adv_output_window) {
		log_std(("video:sdl: use window output\n"));
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW;
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32;
		switch (info->vfmt->BitsPerPixel) {
		case 8: sdl_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_PALETTE8; break;
		case 15: sdl_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR15; break;
		case 16: sdl_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR16; break;
		case 24: sdl_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR24; break;
		case 32: sdl_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR32; break;
		}
	} else if (sdl_state.output == adv_output_fullscreen) {
		unsigned x;
		unsigned y;

		log_std(("video:sdl: use fullscreen output\n"));

		sdl_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;

		if (map == 0 || map == (SDL_Rect**)-1) {
			error_set("No fullscreen mode available.\n");
			goto err_quit;
		}

		x = map[0]->w;
		y = map[0]->h;

		if (SDL_VideoModeOK(x, y, 8, sdl_mode_flags()) == 8)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
		if (SDL_VideoModeOK(x, y, 15, sdl_mode_flags()) == 15)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_BGR15;
		if (SDL_VideoModeOK(x, y, 16, sdl_mode_flags()) == 16)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_BGR16;
		if (SDL_VideoModeOK(x, y, 24, sdl_mode_flags()) == 24)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_BGR24;
		if (SDL_VideoModeOK(x, y, 32, sdl_mode_flags()) == 32)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_BGR32;

		if ((sdl_state.flags & VIDEO_DRIVER_FLAGS_MODE_MASK) == 0) {
			error_set("No fullscreen bit depth available.\n");
			goto err_quit;
		}
	} else if (sdl_state.output == adv_output_overlay) {
		unsigned mode_x;
		unsigned mode_y;
		adv_bool mode_flag;
		unsigned size_x;
		unsigned size_y;

		log_std(("video:sdl: use overlay output\n"));

		sdl_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY;
#ifndef USE_VIDEO_RESTORE
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_INTERNAL_STATIC;
#endif
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_YUY2;

		if (map == 0 || map == (SDL_Rect**)-1) {
			error_set("No fullscreen video mode available.\n");
			goto err_quit;
		}

		if (has_window_manager) {
			sdl_state.overlay_bit = info->vfmt->BitsPerPixel;
		} else {
			sdl_state.overlay_bit = 16;
		}

		log_std(("video:sdl: overlay bitsperpixel %d\n", sdl_state.overlay_bit));

		if (overlay_size) {
			size_x = overlay_size;
			size_y = overlay_size * 3 / 4;
		} else {
			/* Sanity check */
			if (info->current_w > 320 && info->current_h > 200) {
				size_x = info->current_w;
				size_y = info->current_h;
			} else {
				size_x = 1280; /* Common resolution of LCD screen */
				size_y = 1024;
			}
		}

		/* select the mode */
		mode_flag = 0;
		mode_x = 0;
		mode_y = 0;
		for (j = 0; map[j]; ++j) {
			if (SDL_VideoModeOK(map[j]->w, map[j]->h, sdl_state.overlay_bit, sdl_mode_flags()) != 0) {
				if (!mode_flag || abssum(mode_x, mode_y, size_x, size_y) > abssum(map[j]->w, map[j]->h, size_x, size_y)) {
					mode_flag = 1;
					mode_x = map[j]->w;
					mode_y = map[j]->h;
				}
			}
		}

		if (!mode_flag) {
			error_set("No fullscreen video mode available.\n");
			goto err_quit;
		}

		sdl_state.overlay_size_x = mode_x;
		sdl_state.overlay_size_y = mode_y;

		log_std(("video:sdl: overlay size %dx%d\n", sdl_state.overlay_size_x, sdl_state.overlay_size_y));
	} else {
		error_set("Invalid output mode.\n");
		goto err_quit;
	}
#else
	has_window_manager = target_wm();

	/* print info */
	driver_mac = SDL_GetNumVideoDrivers();
	log_std(("video:sdl: SDL_GetNumVideoDrivers() -> %d\n", driver_mac));
	for (driver = 0; driver < driver_mac; ++driver) {
		log_std(("video:sdl: Driver name: %s\n", SDL_GetVideoDriver(driver)));
	}

	log_std(("video:sdl: You can select the video driver with SDL_VIDEODRIVER=\n"));

	display_mac = SDL_GetNumVideoDisplays();
	log_std(("video:sdl: SDL_GetNumVideoDisplays() -> %d\n", display_mac));
	for (display = 0; display < display_mac; ++display) {
		int mode;
		int mode_mac;
		SDL_DisplayMode dm;

		if (SDL_GetDesktopDisplayMode(display, &dm) != 0) {
			log_std(("video:sdl: Failed SDL_GetDesktopDisplayMode(%d, ...)\n", display));
			continue;
		}

		log_std(("video:sdl: SDL_GetDesktopDisplayMode(%d) -> %dx%d %d Hz\n", display, dm.w, dm.h, dm.refresh_rate));

		mode_mac = SDL_GetNumDisplayModes(display);
		if (mode_mac <= 0) {
			log_std(("video:sdl: Failed SDL_GetNumDisplayModes(%d)\n", display));
			continue;
		}

		log_std(("video:sdl: SDL_GetNumDisplayModes(%d) -> %d\n", display, mode_mac));

		for (mode = 0; mode < mode_mac; ++mode) {
			if (SDL_GetDisplayMode(display, mode, &dm) != 0) {
				log_std(("video:sdl: Failed SDL_GetDisplayMode(%d, %d, ...)\n", display, mode));
				continue;
			}

			log_std(("video:sdl: Mode %d.%d : %dx%d %d Hz\n", display, mode, dm.w, dm.h, dm.refresh_rate));
		}
	}

	driver_mac = SDL_GetNumRenderDrivers();
	log_std(("video:sdl: SDL_GetNumRenderDrivers -> %d\n", driver_mac));
	for (driver = 0; driver < driver_mac; ++driver) {
		SDL_RendererInfo ri;
		unsigned texture;

		if (SDL_GetRenderDriverInfo(driver, &ri) != 0) {
			log_std(("video:sdl: Failed SDL_GetRenderDriverInfo(%d, ...)\n", driver));
			continue;
		}

		log_std(("video:sdl: Render name: %s\n", ri.name));
		log_std(("video:sdl: flags:%s%s%s%s\n",
			ri.flags & SDL_RENDERER_SOFTWARE ? " SDL_RENDERER_SOFTWARE" : "",
			ri.flags & SDL_RENDERER_ACCELERATED ? " SDL_RENDERER_ACCELERATED" : "",
			ri.flags & SDL_RENDERER_PRESENTVSYNC ? " SDL_RENDERER_PRESENTVSYNC" : "",
			ri.flags & SDL_RENDERER_TARGETTEXTURE ? " SDL_RENDERER_TARGETTEXTURE" : ""
			));
		log_std(("video:sdl: max texture size: %ux%u\n", ri.max_texture_width, ri.max_texture_height));
		log_std(("video:sdl: num formats: %u\n", ri.num_texture_formats));
		for (texture = 0; texture < ri.num_texture_formats; ++texture)
			log_std(("video:sdl: pixel format: %s, %u\n", SDL_GetPixelFormatName(ri.texture_formats[texture]), SDL_BYTESPERPIXEL(ri.texture_formats[texture])));
	}

	log_std(("video:sdl: You can select the render with SDL_RENDER_DRIVER=\n"));

	if (SDL_GetDesktopDisplayMode(0, &info_dm) != 0) {
		log_std(("ERROR: video:sdl: Failed SDL_GetDesktopDisplayMode(0, ...)\n"));
		error_set("No current display mode.\n");
		goto err_quit;
	}

	if (output == adv_output_auto) {
		if (has_window_manager) {
#ifdef USE_VIDEO_RESTORE
			sdl_state.output = adv_output_window;
#else
			sdl_state.output = adv_output_overlay;
#endif
		} else {
			sdl_state.output = adv_output_overlay;
		}
	} else {
		sdl_state.output = output;
	}

	if (sdl_state.output == adv_output_window) {
		/* reduce a little to allow space for decorations */
		target_size_set(info_dm.w - info_dm.w / 8, info_dm.h - info_dm.h / 8);
	} else {
		target_size_set(info_dm.w, info_dm.h);
	}

	if (sdl_state.output == adv_output_window) {
		log_std(("video:sdl: use window output\n"));
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW;
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_BGR32 | VIDEO_DRIVER_FLAGS_DEFAULT_BGR32;
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_YUY2;
	} else if (sdl_state.output == adv_output_overlay) {
		log_std(("video:sdl: use overlay output\n"));

		sdl_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY;
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_BGR32 | VIDEO_DRIVER_FLAGS_DEFAULT_BGR32;
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_YUY2;
	} else if (sdl_state.output == adv_output_fullscreen) {
		error_set("The 'fullscreen' output mode is not available with SDL2. Use the better 'overlay' mode.\n");
		goto err_quit;
	} else {
		error_set("Invalid output mode.\n");
		goto err_quit;
	}

#ifdef USE_VC /* only for Raspberry */
	if (has_window_manager) {
		/*
		 * In X the opengl/opengles/opengles2 renderers are slower than the software one.
		 *
		 * This applies to both the "-output overlay" and "-output window" modes.
		 *
		 * Verified with SDL2 2.0.5 and Raspbian "jessie" 6/1/2017 on Raspberry 3B+.
		 * Verified with SDL2 2.0.9 and Raspbian "buster" 26/9/2019 on Raspberry 4.
		 *
		 * You can override with the environment variable SDL_RENDER_DRIVER=...
		 */
		SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_DEFAULT);
	}
#endif

	/* avoid hot keys */
	SDL_SetHintWithPriority(SDL_HINT_GRAB_KEYBOARD, "1", SDL_HINT_DEFAULT);

#ifdef USE_VIDEO_RESTORE /* only for AdvanceMENU */
	/* avoid minimization of the menu when it starts a game */
	SDL_SetHintWithPriority(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0", SDL_HINT_DEFAULT);
#endif
#endif

	log_std(("video:sdl: current %ux%u\n", target_size_x(), target_size_y()));

	sdl_state.active = 1;

	return 0;

err_quit:
	if (initialized_now) {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
err:
	return -1;
}

void sdl_done(void)
{
	assert(sdl_is_active());
	assert(!sdl_mode_is_active());

	log_std(("video:sdl: sdl_done()\n"));

	if (SDL_WasInit(SDL_INIT_VIDEO) != 0) {
		log_std(("video:sdl: call SDL_QuitSubSystem(SDL_INIT_VIDEO)\n"));
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	sdl_state.active = 0;
}

static unsigned char* sdl_linear_write_line(unsigned y)
{
	assert(sdl_state.lock_active);

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay)
		return (unsigned char*)sdl_state.overlay->pixels[0] + (unsigned)sdl_state.overlay->pitches[0] * y;
	else
		return (unsigned char*)sdl_state.surface->pixels + (unsigned)sdl_state.surface->pitch * y;
#else
	return (unsigned char*)sdl_state.overlay_ptr + (unsigned)sdl_state.overlay_pitch * y;
#endif
}

#if SDL_MAJOR_VERSION == 1
static adv_error sdl_overlay_set(const sdl_video_mode* mode)
{
	sdl_state.overlay = SDL_CreateYUVOverlay(mode->size_x * 2, mode->size_y, SDL_YUY2_OVERLAY, sdl_state.surface);
	if (!sdl_state.overlay) {
		log_std(("video:sdl: SDL_CreateYUVOverlay() failed, %s\n", SDL_GetError()));
		error_set("Unable to create the SDL overlay.");
		return -1;
	}

	/* In Windows DirectX 5 and SDL 1.2.5 the pitches value is set only after the first lock */
	if (SDL_LockYUVOverlay(sdl_state.overlay) != 0) {
		SDL_FreeYUVOverlay(sdl_state.overlay);
		sdl_state.overlay = 0;
		log_std(("ERROR:video:sdl: overlay invalid lock\n"));
		error_set("Invalid (not lockable) SDL overlay.");
		return -1;
	}

	SDL_UnlockYUVOverlay(sdl_state.overlay);

	log_std(("video:sdl: overlay size:%dx%d planes:%d scanline:%d\n", (unsigned)sdl_state.overlay->w, (unsigned)sdl_state.overlay->h, (unsigned)sdl_state.overlay->planes, (unsigned)sdl_state.overlay->pitches[0]));
	log_std(("video:sdl: overlay hw_overlay:%d\n", (unsigned)sdl_state.overlay->hw_overlay));

	/* In Linux XFree+XV and SDL 1.2.5 if the overlay is too big incorrect values are returned */
	if (sdl_state.overlay->w * 2 > sdl_state.overlay->pitches[0]) {
		SDL_FreeYUVOverlay(sdl_state.overlay);
		sdl_state.overlay = 0;
		log_std(("ERROR:video:sdl: overlay invalid pitches\n"));
		error_set("Invalid (erroneous pitch) SDL overlay.");
		return -1;
	}

	return 0;
}
#else
static adv_error sdl_overlay_set(void)
{
	SDL_RendererInfo ri;
	unsigned texture;
	const char* renderer;
	unsigned format;
	unsigned width;
	Uint32 query_format;
	int query_width;
	int query_height;
	Uint32 flags;

	if (sdl_state.texture) {
		SDL_DestroyTexture(sdl_state.texture);
		sdl_state.texture = 0;
	}
	if (sdl_state.renderer) {
		SDL_DestroyRenderer(sdl_state.renderer);
		sdl_state.renderer = 0;
	}

	flags = SDL_RENDERER_ACCELERATED;
	if (sdl_state.overlay_vsync)
		flags |= SDL_RENDERER_PRESENTVSYNC;
	sdl_state.renderer = SDL_CreateRenderer(sdl_state.window, -1, flags);
	if (sdl_state.renderer == 0) {
		log_std(("ERROR:video:sdl: Failed SDL_CreateRenderer(), %s\n", SDL_GetError()));
		return -1;
	}

	if (SDL_GetRendererInfo(sdl_state.renderer, &ri) != 0) {
		log_std(("ERROR:video:sdl: Failed SDL_GetRendererInfo(), %s\n", SDL_GetError()));
		return -1;
	}

	log_std(("video:sdl: Renderer name: %s\n", ri.name));
	log_std(("video:sdl: flags:%s%s%s%s\n",
		ri.flags & SDL_RENDERER_SOFTWARE ? " SDL_RENDERER_SOFTWARE" : "",
		ri.flags & SDL_RENDERER_ACCELERATED ? " SDL_RENDERER_ACCELERATED" : "",
		ri.flags & SDL_RENDERER_PRESENTVSYNC ? " SDL_RENDERER_PRESENTVSYNC" : "",
		ri.flags & SDL_RENDERER_TARGETTEXTURE ? " SDL_RENDERER_TARGETTEXTURE" : ""
		));
	log_std(("video:sdl: max texture size: %ux%u\n", ri.max_texture_width, ri.max_texture_height));
	log_std(("video:sdl: num formats: %u\n", ri.num_texture_formats));
	for (texture = 0; texture < ri.num_texture_formats; ++texture)
		log_std(("video:sdl: pixel format: %s, %u\n", SDL_GetPixelFormatName(ri.texture_formats[texture]), SDL_BYTESPERPIXEL(ri.texture_formats[texture])));

	switch (sdl_state.index) {
	case MODE_FLAGS_INDEX_BGR32:
		format = SDL_PIXELFORMAT_ARGB8888;
		width = sdl_state.overlay_size_x;
		break;
	case MODE_FLAGS_INDEX_YUY2:
		format = SDL_PIXELFORMAT_YUY2;
		/*
		 * The SDL YUY2 format uses 16 bits pixel with alternate U and V,
		 * To represent a full color we need two of them.
		 */
		width = sdl_state.overlay_size_x * 2;
		break;
	default:
		log_std(("ERROR:video:sdl: Invalid index\n"));
		return -1;
	}

	sdl_state.texture = SDL_CreateTexture(sdl_state.renderer,
			format,
			SDL_TEXTUREACCESS_STREAMING,
			width, sdl_state.overlay_size_y
		);
	if (sdl_state.texture == 0) {
		log_std(("ERROR:video:sdl: Failed SDL_CreateTexture(), %s\n", SDL_GetError()));
		return -1;
	}

	if (SDL_QueryTexture(sdl_state.texture, &query_format, 0, &query_width, &query_height) != 0) {
		log_std(("ERROR:video:sdl: Failed SDL_QueryTexture, %s\n", SDL_GetError()));
		return -1;
	}

	log_std(("video:sdl: using texture %dx%d %s %u\n", query_width, query_height, SDL_GetPixelFormatName(query_format), SDL_BYTESPERPIXEL(query_format)));

#ifdef USE_SMP
	/*
	 * Keep track of the main thread.
	 * The "renderer" and "texture" are specific of the thread.
	 */
	sdl_state.thread = pthread_self();
#endif
	return 0;
}
#endif

void sdl_write_lock(void)
{
	assert(!sdl_state.lock_active);

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay) {
		while (SDL_LockYUVOverlay(sdl_state.overlay) != 0) {
			target_yield();
		}
	} else if (sdl_state.surface) {
		if (SDL_MUSTLOCK(sdl_state.surface)) {
			while (SDL_LockSurface(sdl_state.surface) != 0) {
				target_yield();
			}
		}
	}
#endif

	sdl_state.lock_active = 1;
}

void sdl_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y, adv_bool waitvsync)
{
	assert(sdl_state.lock_active);

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay) {
		SDL_UnlockYUVOverlay(sdl_state.overlay);
		SDL_DisplayYUVOverlay(sdl_state.overlay, &sdl_state.surface->clip_rect);
	} else if (sdl_state.surface) {
		if (SDL_MUSTLOCK(sdl_state.surface))
			SDL_UnlockSurface(sdl_state.surface);
		SDL_UpdateRect(sdl_state.surface, x, y, size_x, size_y);
	}
#endif

	sdl_state.lock_active = 0;
}

adv_error sdl_mode_set(const sdl_video_mode* mode)
{
#if SDL_MAJOR_VERSION == 1
	const SDL_VideoInfo* info;
#endif

	assert(sdl_is_active());
	assert(!sdl_mode_is_active());

	log_std(("video:sdl: sdl_mode_set()\n"));

	/* reopen the screen if needed */
	if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
		log_std(("video:sdl: call SDL_InitSubSystem(SDL_INIT_VIDEO)\n"));
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			log_std(("video:sdl: SDL_InitSubSystem(SDL_INIT_VIDEO) failed, %s\n", SDL_GetError()));
			error_set("Unable to initialize the SDL video, %s", SDL_GetError());
			return -1;
		}

		/* set the window information */
#if SDL_MAJOR_VERSION == 1
		SDL_WM_SetCaption(os_internal_sdl_title_get(), os_internal_sdl_title_get());
		sdl_icon();
#endif
	}

#ifdef USE_KEYBOARD_SDL
	/* clear all the keyboard state, it depends on the video and */
	/* you can miss some key releases on a mode change */
	keyb_sdl_event_release_all();
#endif

#if SDL_MAJOR_VERSION == 1
	info = SDL_GetVideoInfo();

	log_std(("video:sdl: video hw_available:%d\n", (unsigned)info->hw_available));
	log_std(("video:sdl: video wm_available:%d\n", (unsigned)info->wm_available));
	log_std(("video:sdl: video blit_hw:%d\n", (unsigned)info->blit_hw));
	log_std(("video:sdl: video blit_hw_CC:%d\n", (unsigned)info->blit_hw_CC));
	log_std(("video:sdl: video blit_hw_A:%d\n", (unsigned)info->blit_hw_A));
	log_std(("video:sdl: video blit_sw:%d\n", (unsigned)info->blit_sw));
	log_std(("video:sdl: video blit_sw_CC:%d\n", (unsigned)info->blit_sw_CC));
	log_std(("video:sdl: video blit_sw_A:%d\n", (unsigned)info->blit_sw_A));
	log_std(("video:sdl: video blit_fill:%d\n", (unsigned)info->blit_fill));
	log_std(("video:sdl: video video_mem:%d kBytes\n", (unsigned)info->video_mem));
	log_std(("video:sdl: video bit:%d byte:%d %d/%d:%d/%d:%d/%d\n",
		(unsigned)info->vfmt->BitsPerPixel, (unsigned)info->vfmt->BytesPerPixel,
		(unsigned)8 - info->vfmt->Rloss, (unsigned)info->vfmt->Rshift,
		(unsigned)8 - info->vfmt->Gloss, (unsigned)info->vfmt->Gshift,
		(unsigned)8 - info->vfmt->Bloss, (unsigned)info->vfmt->Bshift
		));

	sdl_state.index = mode->index;

	if (sdl_state.output == adv_output_overlay) {
		if (sdl_state.index != MODE_FLAGS_INDEX_YUY2) {
			log_std(("video:sdl: only YUY2 is supported\n"));
			return -1;
		}

		sdl_state.surface = SDL_SetVideoMode(sdl_state.overlay_size_x, sdl_state.overlay_size_y, sdl_state.overlay_bit, sdl_mode_flags());
		if (!sdl_state.surface) {
			log_std(("video:sdl: SDL_SetVideoMode(%d, %d, %d, SDL_FULLSCREEN | SDL_HWSURFACE) failed, %s\n", sdl_state.overlay_size_x, sdl_state.overlay_size_y, sdl_state.overlay_bit, SDL_GetError()));
			error_set("Unable to set the SDL video mode.");
			return -1;
		}

		if (sdl_overlay_set(mode) != 0) {
			sdl_state.surface = 0;
			return -1;
		}
	} else {
		if (sdl_state.index == MODE_FLAGS_INDEX_YUY2) {
			log_std(("video:sdl: YUY2 is not supported\n"));
			return -1;
		}

		sdl_state.surface = SDL_SetVideoMode(mode->size_x, mode->size_y, index_bits_per_pixel(mode->index), sdl_mode_flags());
		if (!sdl_state.surface) {
			log_std(("video:sdl: SDL_SetVideoMode(%d, %d, %d, SDL_FULLSCREEN | SDL_HWSURFACE) failed, %s\n", mode->size_x, mode->size_y, index_bits_per_pixel(mode->index), SDL_GetError()));
			error_set("Unable to set the SDL video mode.");
			return -1;
		}

		sdl_state.overlay = 0;
	}

	log_std(("video:sdl: surface %dx%d\n", (unsigned)sdl_state.surface->w, (unsigned)sdl_state.surface->h));
	log_std(("video:sdl: surface bit:%d byte:%d %d/%d:%d/%d:%d/%d\n",
		(unsigned)sdl_state.surface->format->BitsPerPixel, (unsigned)sdl_state.surface->format->BytesPerPixel,
		(unsigned)8 - sdl_state.surface->format->Rloss, (unsigned)sdl_state.surface->format->Rshift,
		(unsigned)8 - sdl_state.surface->format->Gloss, (unsigned)sdl_state.surface->format->Gshift,
		(unsigned)8 - sdl_state.surface->format->Bloss, (unsigned)sdl_state.surface->format->Bshift
		));
	if ((sdl_state.surface->flags & SDL_SWSURFACE) != 0)
		log_std(("video:sdl: surface flag SDL_SWSURFACE\n"));
	if ((sdl_state.surface->flags & SDL_HWSURFACE) != 0)
		log_std(("video:sdl: surface flag SDL_HWSURFACE\n"));
	if ((sdl_state.surface->flags & SDL_ASYNCBLIT) != 0)
		log_std(("video:sdl: surface flag SDL_ASYNCBLIT\n"));
	if ((sdl_state.surface->flags & SDL_ANYFORMAT) != 0)
		log_std(("video:sdl: surface flag SDL_ANYFORMAT\n"));
	if ((sdl_state.surface->flags & SDL_HWPALETTE) != 0)
		log_std(("video:sdl: surface flag SDL_HWPALETTE\n"));
	if ((sdl_state.surface->flags & SDL_DOUBLEBUF) != 0)
		log_std(("video:sdl: surface flag SDL_DOUBLEBUF\n"));
	if ((sdl_state.surface->flags & SDL_FULLSCREEN) != 0)
		log_std(("video:sdl: surface flag SDL_FULLSCREEN\n"));
	if ((sdl_state.surface->flags & SDL_RESIZABLE) != 0)
		log_std(("video:sdl: surface flag SDL_RESIZABLE\n"));
	if ((sdl_state.surface->flags & SDL_HWACCEL) != 0)
		log_std(("video:sdl: surface flag SDL_HWACCEL\n"));
	if ((sdl_state.surface->flags & SDL_SRCCOLORKEY) != 0)
		log_std(("video:sdl: surface flag SDL_SRCCOLORKEY\n"));
	if ((sdl_state.surface->flags & SDL_SRCALPHA) != 0)
		log_std(("video:sdl: surface flag SDL_SRCALPHA\n"));
	if ((sdl_state.surface->flags & SDL_PREALLOC) != 0)
		log_std(("video:sdl: surface flag SDL_PREALLOC\n"));
#else
	switch (mode->index) {
	case MODE_FLAGS_INDEX_BGR32:
	case MODE_FLAGS_INDEX_YUY2:
		sdl_state.index = mode->index;
		break;
	default:
		error_set("Color mode not supported.");
		return -1;
	}

	sdl_state.overlay_size_x = mode->size_x;
	sdl_state.overlay_size_y = mode->size_y;
	sdl_state.overlay_bit = 32;

	sdl_state.overlay_pitch = ALIGN_UNSIGNED(sdl_state.overlay_size_x * 4, ALIGN);

	sdl_state.overlay_alloc = malloc(sdl_state.overlay_size_y * sdl_state.overlay_pitch + ALIGN);
	if (!sdl_state.overlay_alloc) {
		error_set("Low memory.");
		return -1;
	}

	sdl_state.overlay_ptr = ALIGN_PTR(sdl_state.overlay_alloc, ALIGN);

	/* always initialize with vsync to allow to measure it */
	sdl_state.overlay_vsync = 1;

	/* on fast video mode change, we don't destroy the window */
	if (sdl_state.window != 0
		&& sdl_state.output == adv_output_window
		&& (sdl_state.window_size_x != sdl_state.overlay_size_x || sdl_state.window_size_y != sdl_state.overlay_size_y)
	) {
		/* destroy the window if the size change */
		SDL_DestroyWindow(sdl_state.window);
		sdl_state.window = 0;
	}
	if (sdl_state.window == 0) {
		sdl_state.window_size_x = sdl_state.overlay_size_x;
		sdl_state.window_size_y = sdl_state.overlay_size_y;
		sdl_state.window = SDL_CreateWindow(
			os_internal_sdl_title_get(),
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			sdl_state.overlay_size_x, sdl_state.overlay_size_y,
			sdl_state.output == adv_output_overlay ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
			);
	}
	if (sdl_state.window == 0) {
		log_std(("ERROR:video:sdl: Failed SDL_CreateWindow(), %s\n", SDL_GetError()));
		error_set("Unable to set the SDL video mode.");
		return -1;
	}

	sdl_icon(sdl_state.window);

	/* grab mouse and keyboard, but not in window mode */
	if (sdl_state.output != adv_output_window) {
		SDL_SetWindowGrab(sdl_state.window, SDL_TRUE);

		/* set the mouse movements as relative */
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}

	if (sdl_overlay_set() != 0) {
		error_set("Unable to set the SDL video mode.");
		return -1;
	}
#endif

	/* disable the mouse cursor on fullscreen mode */
	switch (sdl_state.cursor) {
	case adv_cursor_auto:
		if (sdl_state.output != adv_output_window) {
			SDL_ShowCursor(SDL_DISABLE);
		} else {
			SDL_ShowCursor(SDL_ENABLE);
		}
		break;
	case adv_cursor_on:
		SDL_ShowCursor(SDL_ENABLE);
		break;
	case adv_cursor_off:
		SDL_ShowCursor(SDL_DISABLE);
		break;
	}

	/* enable window manager events */
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	/* write handler */
	sdl_write_line = sdl_linear_write_line;

	sdl_state.mode_active = 1;
	return 0;
}

void sdl_mode_done(adv_bool restore)
{
	assert(sdl_is_active() && sdl_mode_is_active());

	log_std(("video:sdl: sdl_mode_done()\n"));

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay) {
		SDL_FreeYUVOverlay(sdl_state.overlay);
		sdl_state.overlay = 0;
	}

#ifdef USE_VIDEO_RESTORE
	/* close the screen if we are fullscreen to allow started sub process to have exclusive access at the screen. */
	if ((sdl_state.surface->flags & SDL_FULLSCREEN) != 0) {
		if (SDL_WasInit(SDL_INIT_VIDEO) != 0) {
			log_std(("video:sdl: call SDL_QuitSubSystem(SDL_INIT_VIDEO)\n"));
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		}
	}
#endif

	sdl_state.surface = 0;
#else
	free(sdl_state.overlay_alloc);
	sdl_state.overlay_alloc = 0;
	sdl_state.overlay_ptr = 0;
	if (sdl_state.texture) {
		SDL_DestroyTexture(sdl_state.texture);
		sdl_state.texture = 0;
	}
	if (sdl_state.renderer) {
		SDL_DestroyRenderer(sdl_state.renderer);
		sdl_state.renderer = 0;
	}

	/* destroy the window only if "restore" is selected */
	if (restore) {
		if (sdl_state.window) {
			SDL_DestroyWindow(sdl_state.window);
			sdl_state.window = 0;
		}
	}
#endif

	sdl_state.mode_active = 0;
}

adv_error sdl_mode_change(const sdl_video_mode* mode)
{
	assert(sdl_is_active() && sdl_mode_is_active());

	log_std(("video:sdl: sdlb_mode_change()\n"));

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.output == adv_output_overlay) {
		SDL_FreeYUVOverlay(sdl_state.overlay);

		sdl_state.mode_active = 0;

		if (sdl_overlay_set(mode) != 0) {
			sdl_state.surface = 0;
			return -1;
		}

		sdl_state.mode_active = 1;

		return 0;
	} else {
		sdl_mode_done(0);
		return sdl_mode_set(mode);
	}
#else
	sdl_mode_done(0);
	return sdl_mode_set(mode);
#endif
}

unsigned sdl_virtual_x(void)
{
	assert(sdl_is_active() && sdl_mode_is_active());

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay)
		return sdl_state.overlay->w / 2;
	else
		return sdl_state.surface->w;
#else
	return sdl_state.overlay_size_x;
#endif
}

unsigned sdl_virtual_y(void)
{
	assert(sdl_is_active() && sdl_mode_is_active());

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay)
		return sdl_state.overlay->h;
	else
		return sdl_state.surface->h;
#else
	return sdl_state.overlay_size_y;
#endif
}

unsigned sdl_adjust_bytes_per_page(unsigned bytes_per_page)
{
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned sdl_bytes_per_scanline(void)
{
	assert(sdl_is_active() && sdl_mode_is_active());

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.overlay)
		return sdl_state.overlay->pitches[0];
	else
		return sdl_state.surface->pitch;
#else
	return sdl_state.overlay_pitch;
#endif
}

adv_color_def sdl_color_def(void)
{
	assert(sdl_is_active() && sdl_mode_is_active());

#if SDL_MAJOR_VERSION == 1
	switch (sdl_state.index) {
	case MODE_FLAGS_INDEX_BGR15:
	case MODE_FLAGS_INDEX_BGR16:
	case MODE_FLAGS_INDEX_BGR24:
	case MODE_FLAGS_INDEX_BGR32:
		return color_def_make_rgb_from_sizelenpos(
			sdl_state.surface->format->BytesPerPixel,
			8 - sdl_state.surface->format->Rloss, sdl_state.surface->format->Rshift,
			8 - sdl_state.surface->format->Gloss, sdl_state.surface->format->Gshift,
			8 - sdl_state.surface->format->Bloss, sdl_state.surface->format->Bshift
		);
	default:
		return color_def_make_from_index(sdl_state.index);
	}
#else
	return color_def_make_from_index(sdl_state.index);
#endif
}

void sdl_wait_vsync(void)
{
	assert(sdl_is_active() && sdl_mode_is_active());
}

adv_error sdl_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(sdl_is_active() && sdl_mode_is_active());

	if (offset != 0)
		return -1;

	/* normalize to 0 1 */
	waitvsync = waitvsync != 0;

#if SDL_MAJOR_VERSION != 1
	/* reset the renderer if the thread or the vsync request change */
	if (
#ifdef USE_SMP
		sdl_state.thread != pthread_self() ||
#endif
		waitvsync != sdl_state.overlay_vsync
	) {
		log_std(("video:sdl: recompute renderer for thread/vsync change. New vsync=%d\n", waitvsync));
		sdl_state.overlay_vsync = waitvsync;
		sdl_overlay_set();
	}

	if (SDL_UpdateTexture(sdl_state.texture, 0, sdl_state.overlay_ptr, sdl_state.overlay_pitch) != 0) {
		log_std(("ERROR:video:sdl: Failed SDL_UpdateTexture(), %s\n", SDL_GetError()));
	}

	if (SDL_RenderCopy(sdl_state.renderer, sdl_state.texture, 0, 0) != 0) {
		log_std(("ERROR:video:sdl: Failed SDL_RenderCopy(), %s\n", SDL_GetError()));
	}

	SDL_RenderPresent(sdl_state.renderer);
#endif

	return 0;
}

adv_error sdl_scanline_set(unsigned byte_length)
{
	assert(sdl_is_active() && sdl_mode_is_active());

	if (byte_length != sdl_bytes_per_scanline())
		return -1;

	return 0;
}

adv_error sdl_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
#if SDL_MAJOR_VERSION == 1
	SDL_Color sdl_pal[256];
	unsigned i;

	for (i = 0; i < count; ++i) {
		sdl_pal[i].r = palette[i].red;
		sdl_pal[i].g = palette[i].green;
		sdl_pal[i].b = palette[i].blue;
	}

	if (SDL_SetPalette(sdl_state.surface, SDL_PHYSPAL, sdl_pal, start, count) != 1) {
		log_std(("video:sdl: SDL_SetPalette(SDL_PHYSPAL) failed, %s\n", SDL_GetError()));
		return -1;
	}

	return 0;
#else
	/* SDL2 doesn't support palette modes */
	return -1;
#endif
}

#define DRIVER(mode) ((sdl_video_mode*)(&mode->driver_mode))

/**
 * Import information for one video mode.
 * \return 0 if successful
 */
adv_error sdl_mode_import(adv_mode* mode, const sdl_video_mode* sdl_mode)
{
	log_debug(("video:sdl: sdl_mode_import()\n"));

	snprintf(mode->name, MODE_NAME_MAX, "sdl_%dx%dx%d", sdl_mode->size_x, sdl_mode->size_y, index_bits_per_pixel(sdl_mode->index));

	*DRIVER(mode) = *sdl_mode;

	mode->driver = &video_sdl_driver;
	mode->flags = (mode->flags & MODE_FLAGS_USER_MASK) | sdl_mode->index;
#if SDL_MAJOR_VERSION != 1
	/*
	 * Always assume that vsync is supported.
	 * If it's not the measure will be wrong and the vsync rejected at higher level.
	 */
	mode->flags |= MODE_FLAGS_RETRACE_SCROLL_SYNC;
#endif
	mode->size_x = sdl_mode->size_x;
	mode->size_y = sdl_mode->size_y;
	mode->vclock = 0;
	mode->hclock = 0;
	if (sdl_mode->size_y <= 300)
		mode->scan = 1; /* assume doublescan */
	else
		mode->scan = 0; /* assume singlescan */

	return 0;
}

adv_error sdl_mode_generate(sdl_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	unsigned suggested_bits;
	unsigned request_x;
	unsigned request_y;
	unsigned request_bits;

	log_debug(("video:sdl: sdl_mode_generate(x:%d, y:%d)\n", crtc->hde, crtc->vde));

	if (!crtc_is_fake(crtc)) {
		error_nolog_set("Programmable modes not supported.\n");
		return -1;
	}

#if SDL_MAJOR_VERSION == 1
	switch (flags & MODE_FLAGS_INDEX_MASK) {
	case MODE_FLAGS_INDEX_PALETTE8:
	case MODE_FLAGS_INDEX_BGR15:
	case MODE_FLAGS_INDEX_BGR16:
	case MODE_FLAGS_INDEX_BGR24:
	case MODE_FLAGS_INDEX_BGR32:
		if (sdl_state.output == adv_output_overlay) {
			error_nolog_set("Only yuy2 is supported in overlay mode.\n");
			return -1;
		}

		request_x = crtc->hde;
		request_y = crtc->vde;
		request_bits = index_bits_per_pixel(flags & MODE_FLAGS_INDEX_MASK);

		suggested_bits = SDL_VideoModeOK(request_x, request_y, request_bits, sdl_mode_flags());

		if (!suggested_bits) {
			error_nolog_set("No compatible SDL mode found.\n");
			return -1;
		}

		if (suggested_bits != request_bits) {
			/* if it's a window accepts any bit depths */
			if (sdl_state.output != adv_output_window) {
				error_nolog_set("No compatible SDL bit depth found.\n");
				return -1;
			}
		}
		break;
	case MODE_FLAGS_INDEX_YUY2:
		if (sdl_state.output != adv_output_overlay) {
			error_nolog_set("yuy2 supported only in overlay mode.\n");
			return -1;
		}
		break;
	default:
		error_nolog_set("Index mode not supported.\n");
		return -1;
	}
#else
	switch (flags & MODE_FLAGS_INDEX_MASK) {
	case MODE_FLAGS_INDEX_BGR32:
	case MODE_FLAGS_INDEX_YUY2:
		break;
	default:
		error_nolog_set("Index mode not supported.\n");
		return -1;
	}
#endif

	mode->size_x = crtc->hde;
	mode->size_y = crtc->vde;
	mode->index = flags & MODE_FLAGS_INDEX_MASK;

	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1;

int sdl_mode_compare(const sdl_video_mode* a, const sdl_video_mode* b)
{
	COMPARE(a->index, b->index)
	COMPARE(a->size_y, b->size_y)
	COMPARE(a->size_x, b->size_x)
	return 0;
}

void sdl_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:sdl: sdl_crtc_container_insert_default()\n"));

#if SDL_MAJOR_VERSION == 1
	if (sdl_state.output == adv_output_fullscreen) {
		SDL_Rect** map; /* it must not be released */
		map = SDL_ListModes(0, sdl_mode_flags());
		if (!map) {
			/* no resolutions */
		} else if (map == (SDL_Rect**)-1) {
			/* all resolutions available */
		} else {
			unsigned i;
			for (i = 0; map[i]; ++i) {
				adv_crtc crtc;
				crtc_fake_set(&crtc, map[i]->w, map[i]->h);
				snprintf(crtc.name, CRTC_NAME_MAX, "sdl_%dx%d", map[i]->w, map[i]->h);
				crtc_container_insert(cc, &crtc);
			}
		}
	}
#endif
}

void sdl_reg(adv_conf* context)
{
}

adv_error sdl_load(adv_conf* context)
{
	return 0;
}

/***************************************************************************/
/* Driver */

static adv_error sdl_mode_set_void(const void* mode)
{
	return sdl_mode_set((const sdl_video_mode*)mode);
}

static adv_error sdl_mode_change_void(const void* mode)
{
	return sdl_mode_change((const sdl_video_mode*)mode);
}

static adv_error sdl_mode_import_void(adv_mode* mode, const void* sdl_mode)
{
	return sdl_mode_import(mode, (const sdl_video_mode*)sdl_mode);
}

static adv_error sdl_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return sdl_mode_generate((sdl_video_mode*)mode, crtc, flags);
}

static int sdl_mode_compare_void(const void* a, const void* b)
{
	return sdl_mode_compare((const sdl_video_mode*)a, (const sdl_video_mode*)b);
}

static unsigned sdl_mode_size(void)
{
	return sizeof(sdl_video_mode);
}

adv_video_driver video_sdl_driver = {
	"sdl",
	DEVICE,
	sdl_load,
	sdl_reg,
	sdl_init,
	sdl_done,
	sdl_flags,
	sdl_mode_set_void,
	sdl_mode_change_void,
	sdl_mode_done,
	sdl_virtual_x,
	sdl_virtual_y,
	0,
	0,
	sdl_bytes_per_scanline,
	sdl_adjust_bytes_per_page,
	sdl_color_def,
	sdl_write_lock,
	sdl_write_unlock,
	&sdl_write_line,
	sdl_wait_vsync,
	sdl_scroll,
	sdl_scanline_set,
	sdl_palette8_set,
	sdl_mode_size,
	0,
	sdl_mode_generate_void,
	sdl_mode_import_void,
	sdl_mode_compare_void,
	sdl_crtc_container_insert_default
};

/***************************************************************************/
/* Internal interface */

int os_internal_sdl_is_video_active(void)
{
	return sdl_is_active();
}

int os_internal_sdl_is_video_mode_active(void)
{
	return sdl_is_active() && sdl_mode_is_active();
}

void* os_internal_sdl_window_get(void)
{
#if SDL_MAJOR_VERSION == 1
	return 0;
#else
	return sdl_state.window;
#endif
}

