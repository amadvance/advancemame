/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 1999-2001 Andrea Mazzoleni
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

#include "vsdl.h"
#include "video.h"
#include "os.h"

#include "SDL.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct sdl_internal_struct {
	video_bool active; /* !=0 if present */
	video_bool mode_active; /* !=0 if mode set */

	SDL_Surface* surface;
} sdl_internal;

static sdl_internal sdl_state;

unsigned char* (*sdl_write_line)(unsigned y);

static device DEVICE[] = {
{ "auto", 1, "SDL automatic detection" },
{ "sdl", 2, "SDL (no hardware programming)" },
{ 0, 0, 0 }
};

video_error sdl_init(int device_id) {
	char name[64];
	assert( !sdl_is_active() );

	os_log(("video:sdl: sdl_init()\n"));

	const device* i = DEVICE;
	while (i->name && i->id != device_id)
		++i;
	if (!i->name)
		return -1;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
		os_log(("video:sdl: SDL_InitSubSystem(SDL_INIT_VIDEO) failed, %s\n",  SDL_GetError()));
		return -1;
	}

	if (SDL_VideoDriverName(name,sizeof(name))) {
		video_log("video:sdl: driver %s\n", name);
	}

	sdl_state.active = 1;

	return 0;
}

void sdl_done(void) {
	assert( sdl_is_active() );
	assert( !sdl_mode_is_active() );

	os_log(("video:sdl: sdl_done()\n"));

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	sdl_state.active = 0;
}

video_bool sdl_is_active(void) {
	 return sdl_state.active != 0;
}

video_bool sdl_mode_is_active(void) {
	return sdl_state.mode_active != 0;
}

unsigned sdl_flags(void) {
	return VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
}

static unsigned char* sdl_linear_write_line(unsigned y) {
	return (unsigned char*)sdl_state.surface->pixels + (unsigned)sdl_state.surface->pitch * y;
}

void sdl_write_lock(void) {
	if (SDL_MUSTLOCK(sdl_state.surface)) {
		while (SDL_LockSurface( sdl_state.surface ) != 0) {
			SDL_Delay(1);
		}
	}
}

void sdl_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y) {
	if (SDL_MUSTLOCK(sdl_state.surface))
		SDL_UnlockSurface(sdl_state.surface);
	SDL_UpdateRect(sdl_state.surface, x, y, size_x, size_y);
}

video_error sdl_mode_set(const sdl_video_mode* mode) {
	assert( sdl_is_active() );
	assert( !sdl_mode_is_active() );

	unsigned flags = SDL_HWSURFACE;

	os_log(("video:sdl: sdl_mode_set()\n"));

	sdl_state.surface = SDL_SetVideoMode(mode->size_x, mode->size_y, mode->bits_per_pixel, flags);
	if (!sdl_state.surface) {
		os_log(("video:sdl: SDL_SetVideoMode(%d,%d,%d,SDL_HWSURFACE) failed, %s\n", mode->size_x, mode->size_y, mode->bits_per_pixel, SDL_GetError()));
		return -1;
	}

	/* disable the mouse cursor on fullscreen mode */
	if ((sdl_state.surface->flags & SDL_FULLSCREEN) != 0) {
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_ShowCursor(SDL_ENABLE);
	}

	video_log("video:sdl: surface %dx%dx%d\n", (unsigned)sdl_state.surface->w, (unsigned)sdl_state.surface->h, (unsigned)sdl_state.surface->format->BitsPerPixel);
	if ((sdl_state.surface->flags & SDL_SWSURFACE) != 0)
		video_log("video:sdl: surface SDL_SWSURFACE\n");
	if ((sdl_state.surface->flags & SDL_HWSURFACE) != 0)
		video_log("video:sdl: surface SDL_HWSURFACE\n");
	if ((sdl_state.surface->flags & SDL_ASYNCBLIT) != 0)
		video_log("video:sdl: surface SDL_ASYNCBLIT\n");
	if ((sdl_state.surface->flags & SDL_ANYFORMAT) != 0)
		video_log("video:sdl: surface SDL_ANYFORMAT\n");
	if ((sdl_state.surface->flags & SDL_HWPALETTE) != 0)
		video_log("video:sdl: surface SDL_HWPALETTE\n");
	if ((sdl_state.surface->flags & SDL_DOUBLEBUF) != 0)
		video_log("video:sdl: surface SDL_DOUBLEBUF\n");
	if ((sdl_state.surface->flags & SDL_FULLSCREEN) != 0)
		video_log("video:sdl: surface SDL_FULLSCREEN\n");
	if ((sdl_state.surface->flags & SDL_RESIZABLE) != 0)
		video_log("video:sdl: surface SDL_RESIZABLE\n");
	if ((sdl_state.surface->flags & SDL_HWACCEL) != 0)
		video_log("video:sdl: surface SDL_HWACCEL\n");
	if ((sdl_state.surface->flags & SDL_SRCCOLORKEY) != 0)
		video_log("video:sdl: surface SDL_SRCCOLORKEY\n");
	if ((sdl_state.surface->flags & SDL_SRCALPHA) != 0)
		video_log("video:sdl: surface SDL_SRCALPHA\n");
	if ((sdl_state.surface->flags & SDL_PREALLOC) != 0)
		video_log("video:sdl: surface SDL_PREALLOC\n");

	/* write handler */
	sdl_write_line = sdl_linear_write_line;

	sdl_state.mode_active = 1;
	return 0;
}

void sdl_mode_done(video_bool restore) {
	assert( sdl_is_active() );
	assert( sdl_mode_is_active() );

	os_log(("video:sdl: sdl_mode_done()\n"));

	sdl_state.mode_active = 0;
}

unsigned sdl_virtual_x(void) {
	return sdl_state.surface->w;
}

unsigned sdl_virtual_y(void) {
	return sdl_state.surface->h;
}

unsigned sdl_adjust_bytes_per_page(unsigned bytes_per_page) {
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned sdl_bytes_per_scanline(void) {
	return sdl_state.surface->pitch;
}

video_rgb_def sdl_rgb_def(void) {
	return video_rgb_def_make(
		8 - sdl_state.surface->format->Rloss, sdl_state.surface->format->Rshift,
		8 - sdl_state.surface->format->Gloss, sdl_state.surface->format->Gshift,
		8 - sdl_state.surface->format->Bloss, sdl_state.surface->format->Bshift
	);
}

void sdl_wait_vsync(void) {
	/* not available */
}

video_error sdl_scroll(unsigned offset, video_bool waitvsync) {
	assert(sdl_is_active() && sdl_mode_is_active());

	if (offset != 0)
		return -1;

	return 0;
}

video_error sdl_scanline_set(unsigned byte_length) {
	assert(sdl_is_active() && sdl_mode_is_active());

	if (byte_length != sdl_state.surface->pitch)
		return -1;

	return 0;
}

video_error sdl_palette8_set(const video_color* palette, unsigned start, unsigned count, video_bool waitvsync) {
	SDL_Color sdl_pal[256];
	unsigned i;

	for(i=0;i<count;++i) {
		sdl_pal[i].r = palette[i].red;
		sdl_pal[i].g = palette[i].green;
		sdl_pal[i].b = palette[i].blue;
	}

	if (SDL_SetPalette(sdl_state.surface, SDL_PHYSPAL, sdl_pal, start, count) != 1) {
		os_log(("video:sdl: SDL_SetPalette(SDL_PHYSPAL) failed, %s\n",  SDL_GetError()));
		return -1;
	}

	return 0;
}

#define DRIVER(mode) ((sdl_video_mode*)(&mode->driver_mode))

/**
 * Import information for one video mode.
 * \return 0 if successful
 */
video_error sdl_mode_import(video_mode* mode, const sdl_video_mode* sdl_mode) {
	sprintf(mode->name,"sdl_%dx%dx%d",sdl_mode->size_x, sdl_mode->size_y, sdl_mode->bits_per_pixel);
	*DRIVER(mode) = *sdl_mode;

	mode->driver = &video_sdl_driver;
	mode->flags = (mode->flags & VIDEO_FLAGS_USER_MASK);
	switch (sdl_mode->bits_per_pixel) {
		case 8 :
			mode->flags |= VIDEO_FLAGS_INDEX_PACKED | VIDEO_FLAGS_TYPE_GRAPHICS;
			break;
		case 15 :
		case 16 :
		case 24 :
		case 32 :
			mode->flags |= VIDEO_FLAGS_INDEX_RGB | VIDEO_FLAGS_TYPE_GRAPHICS;
			break;
		default :
			return -1;
	}
	mode->flags |= VIDEO_FLAGS_MEMORY_LINEAR;

	mode->size_x = sdl_mode->size_x;
	mode->size_y = sdl_mode->size_y;
	mode->vclock = 0;
	mode->hclock = 0;
	mode->bits_per_pixel = sdl_mode->bits_per_pixel;
	if (sdl_mode->size_y <= 300)
		mode->scan = 1; /* assume doublescan */
	else
		mode->scan = 0; /* assume singlescan */

	return 0;
}

video_error sdl_mode_generate(sdl_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	int number;
	unsigned model;

	if (video_mode_generate_check("sdl",sdl_flags(),1,2048,crtc,bits,flags)!=0)
		return -1;

	if (SDL_VideoModeOK(crtc->hde, crtc->vde, bits, 0) != bits) {
		video_error_description_nolog_cat("No compatible SDL mode found\n");
		return -1;
	}

	mode->size_x = crtc->hde;
	mode->size_y = crtc->vde;
	mode->bits_per_pixel = bits;

	return 0;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1;

int sdl_mode_compare(const sdl_video_mode* a, const sdl_video_mode* b) {
	COMPARE(a->size_y,b->size_y)
	COMPARE(a->size_x,b->size_x)
	COMPARE(a->bits_per_pixel,b->bits_per_pixel)
	return 0;
}

/***************************************************************************/
/* Driver */

static video_error sdl_mode_set_void(const void* mode) {
	return sdl_mode_set((const sdl_video_mode*)mode);
}

static video_error sdl_mode_import_void(video_mode* mode, const void* sdl_mode) {
	return sdl_mode_import(mode, (const sdl_video_mode*)sdl_mode);
}

static video_error sdl_mode_generate_void(void* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	return sdl_mode_generate((sdl_video_mode*)mode, crtc, bits, flags);
}

static int sdl_mode_compare_void(const void* a, const void* b) {
	return sdl_mode_compare((const sdl_video_mode*)a, (const sdl_video_mode*)b);
}

static void sdl_reg_dummy(struct conf_context* context) {
}

static video_error sdl_load_dummy(struct conf_context* context) {
	return 0;
}

static unsigned sdl_mode_size(void) {
	return sizeof(sdl_video_mode);
}

video_driver video_sdl_driver = {
	"sdl",
	DEVICE,
	sdl_load_dummy,
	sdl_reg_dummy,
	sdl_init,
	sdl_done,
	sdl_flags,
	sdl_mode_set_void,
	0,
	sdl_mode_done,
	sdl_virtual_x,
	sdl_virtual_y,
	0,
	0,
	sdl_bytes_per_scanline,
	sdl_adjust_bytes_per_page,
	sdl_rgb_def,
	sdl_write_lock,
	sdl_write_unlock,
	&sdl_write_line,
	sdl_wait_vsync,
	sdl_scroll,
	sdl_scanline_set,
	sdl_palette8_set,
	0,
	sdl_mode_size,
	0,
	sdl_mode_generate_void,
	sdl_mode_import_void,
	sdl_mode_compare_void
};

