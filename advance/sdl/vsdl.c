/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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
#include "log.h"
#include "ossdl.h"
#include "error.h"

#include "SDL.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/***************************************************************************/
/* Options */

struct sdl_option_struct {
	adv_bool initialized;
	adv_bool fullscreen;
};

static struct sdl_option_struct sdl_option;

typedef struct sdl_internal_struct {
	adv_bool active; /**< !=0 if present. */
	adv_bool mode_active; /**< !=0 if mode set. */
	adv_bool lock_active; /**< !=0 if lock active. */
	unsigned flags; /**< Precomputed driver flags. */
	SDL_Surface* surface; /**< Screen surface. */

	unsigned char* text_map; /**< Text buffer. */
	unsigned text_dx; /**< Text columns. */
	unsigned text_dy; /**< Text rows. */
} sdl_internal;

static sdl_internal sdl_state;

unsigned char* (*sdl_write_line)(unsigned y);

static adv_device DEVICE[] = {
{ "auto", 1, "SDL video" },
{ 0, 0, 0 }
};

/** Bit depth used for the fake text modes. */
#define SDL_TEXT_BIT 16

static unsigned SDL_ModeFlags(void) {
	unsigned flags;

	if ((sdl_flags() & VIDEO_DRIVER_FLAGS_INFO_WINDOWMANAGER) != 0) {
		/* use a window if a window manager is present */
		flags = 0;
	} else {
		/* use fullscreen direct access otherwise */
		flags = SDL_FULLSCREEN | SDL_HWSURFACE;
	}

	return flags;
}

#include "icondef.dat"

static void SDL_WM_DefIcon(void) {
	SDL_Surface* surface;
	SDL_Color colors[ICON_PALETTE];
	unsigned i,x,y;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, ICON_SIZE, ICON_SIZE, 8, 0, 0, 0, 0);
	if (!surface) {
		log_std(("os: SDL_WM_DefIcon() failed in SDL_CreateRGBSurface\n"));
		return;
	}

	for(y=0;y<ICON_SIZE;++y) {
		unsigned char* p = (unsigned char*)surface->pixels + y * surface->pitch;
		for(x=0;x<ICON_SIZE;++x)
			p[x] = icon_pixel[y*ICON_SIZE+x];
	}

	for(i=0;i<ICON_PALETTE;++i) {
		colors[i].r = icon_palette[i*3+0];
		colors[i].g = icon_palette[i*3+1];
		colors[i].b = icon_palette[i*3+2];
	}

	if (SDL_SetColors(surface, colors, 0, ICON_PALETTE) != 1) {
		log_std(("os: SDL_WM_DefIcon() failed in SDL_SetColors\n"));
		SDL_FreeSurface(surface);
		return;
	}

	SDL_WM_SetIcon(surface, icon_mask);

	SDL_FreeSurface(surface);
}

adv_error sdl_init(int device_id) {
	char name[64];
	const adv_device* i;
	unsigned j;
	SDL_Rect** map;
	const SDL_VideoInfo* info;

	assert( !sdl_is_active() );

	log_std(("video:sdl: sdl_init()\n"));

	memset(&sdl_state,0, sizeof(sdl_state));

	sdl_state.mode_active = 0;
	sdl_state.lock_active = 0;
	sdl_state.surface = 0;

	i = DEVICE;
	while (i->name && i->id != device_id)
		++i;
	if (!i->name)
		return -1;

	if (!sdl_option.initialized) {
		sdl_default();
	}

	if (SDL_WasInit(SDL_INIT_VIDEO)==0) {
		log_std(("video:sdl: call SDL_InitSubSystem(SDL_INIT_VIDEO)\n"));
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			log_std(("video:sdl: SDL_InitSubSystem(SDL_INIT_VIDEO) failed, %s\n",  SDL_GetError()));
			return -1;
		}

		/* set the window information */
		SDL_WM_SetCaption(os_internal_title_get(),os_internal_title_get());
		SDL_WM_DefIcon();
	}

	if (SDL_VideoDriverName(name,sizeof(name))) {
		log_std(("video:sdl: SDL driver %s\n", name));
	}

	info = SDL_GetVideoInfo();

	sdl_state.flags = 0;

	if (info->wm_available && !sdl_option.fullscreen) {
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_INFO_WINDOWMANAGER;
		switch (info->vfmt->BitsPerPixel) {
			case 8 : sdl_state.flags |= VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_8BIT; break;
			case 15 : sdl_state.flags |= VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_15BIT; break;
			case 16 : sdl_state.flags |= VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_16BIT; break;
			case 32 : sdl_state.flags |= VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_32BIT; break;
		}
	}

	map = SDL_ListModes(0, SDL_ModeFlags() );
	if (map == 0) {
		log_std(("video:sdl: no default video modes\n"));
	} else if (map == (SDL_Rect **)-1) {
		log_std(("video:sdl: all resolution availables\n"));
	} else {
		for(j=0;map[j];++j) {
			log_std(("video:sdl: mode %dx%d\n", (unsigned)map[j]->w, (unsigned)map[j]->h));
		}
	}

	/* check the available bit depth */
	if (!info->wm_available || sdl_option.fullscreen) {
		unsigned x;
		unsigned y;

		if (map == 0 || map == (SDL_Rect **)-1) {
			log_std(("video:sdl: No fullscreen video mode available\n"));
			return -1;
		}

		x = map[0]->w;
		y = map[0]->h;

		if (SDL_VideoModeOK(x, y, 8, SDL_ModeFlags()) == 8)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT;
		if (SDL_VideoModeOK(x, y, 15, SDL_ModeFlags()) == 15)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT;
		if (SDL_VideoModeOK(x, y, 16, SDL_ModeFlags()) == 16)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT;
		if (SDL_VideoModeOK(x, y, 32, SDL_ModeFlags()) == 32)
			sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT;

		if ((sdl_state.flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL) == 0) {
			log_std(("video:sdl: No fullscreen bit depths available\n"));
			return -1;
		}

	} else {
		sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL;
	}

	sdl_state.flags |= VIDEO_DRIVER_FLAGS_MODE_TEXT;

	sdl_state.active = 1;

	return 0;
}

void sdl_done(void) {
	assert( sdl_is_active() );
	assert( !sdl_mode_is_active() );

	log_std(("video:sdl: sdl_done()\n"));

	if (SDL_WasInit(SDL_INIT_VIDEO)!=0) {
		log_std(("video:sdl: call SDL_QuitSubSystem(SDL_INIT_VIDEO)\n"));
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	sdl_state.active = 0;
}

adv_bool sdl_is_active(void) {
	 return sdl_state.active != 0;
}

adv_bool sdl_mode_is_active(void) {
	return sdl_state.mode_active != 0;
}

unsigned sdl_flags(void) {
	return sdl_state.flags;
}

static unsigned char* sdl_linear_write_line_graphics(unsigned y) {
	assert( sdl_state.lock_active );
	
	return (unsigned char*)sdl_state.surface->pixels + (unsigned)sdl_state.surface->pitch * y;
}

static unsigned char* sdl_linear_write_line_text(unsigned y) {
	assert(y < sdl_state.text_dy);
	return sdl_state.text_map + y * sdl_state.text_dx * 2;
}

void sdl_write_lock(void) {
	assert( !sdl_state.lock_active );
	
	if (SDL_MUSTLOCK(sdl_state.surface)) {
		while (SDL_LockSurface( sdl_state.surface ) != 0) {
			SDL_Delay(1);
		}
	}
	
	sdl_state.lock_active = 1;
}

void sdl_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y) {
	assert( sdl_state.lock_active );
	
	if (SDL_MUSTLOCK(sdl_state.surface))
		SDL_UnlockSurface(sdl_state.surface);
	SDL_UpdateRect(sdl_state.surface, x, y, size_x, size_y);
	
	sdl_state.lock_active = 0;
}

adv_error sdl_mode_set(const sdl_video_mode* mode) {
	const SDL_VideoInfo* info;

	assert( sdl_is_active() );
	assert( !sdl_mode_is_active() );

	log_std(("video:sdl: sdl_mode_set()\n"));

	/* reopen the screen if needed */
	if (SDL_WasInit(SDL_INIT_VIDEO)==0) {
		log_std(("video:sdl: call SDL_InitSubSystem(SDL_INIT_VIDEO)\n"));
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			log_std(("video:sdl: SDL_InitSubSystem(SDL_INIT_VIDEO) failed, %s\n",  SDL_GetError()));
			return -1;
		}

		/* set the window information */
		SDL_WM_SetCaption(os_internal_title_get(),os_internal_title_get());
		SDL_WM_DefIcon();
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
	log_std(("video:sdl: video video_mem:%d\n", (unsigned)info->video_mem));

	if (mode->is_text) {
		sdl_state.surface = SDL_SetVideoMode(mode->size_x, mode->size_y, SDL_TEXT_BIT, 0);
		if (!sdl_state.surface) {
			log_std(("video:sdl: SDL_SetVideoMode(%d,%d,0,0) failed, %s\n", mode->size_x, mode->size_y, SDL_GetError()));
			return -1;
		}
		sdl_state.text_dx = mode->size_x / 8;
		sdl_state.text_dy = mode->size_y / 16;
		sdl_state.text_map = malloc(sdl_state.text_dx * sdl_state.text_dy * 2);
		memset(sdl_state.text_map, 0, sdl_state.text_dx * sdl_state.text_dy * 2);
	} else {
		sdl_state.surface = SDL_SetVideoMode(mode->size_x, mode->size_y, mode->bits_per_pixel, SDL_ModeFlags() );
		if (!sdl_state.surface) {
			log_std(("video:sdl: SDL_SetVideoMode(%d,%d,%d,SDL_HWSURFACE) failed, %s\n", mode->size_x, mode->size_y, mode->bits_per_pixel, SDL_GetError()));
			return -1;
		}
		sdl_state.text_dx = 0;
		sdl_state.text_dy = 0;
		sdl_state.text_map = 0;
	}

	/* disable the mouse cursor on fullscreen mode */
	if ((sdl_state.surface->flags & SDL_FULLSCREEN) != 0) {
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_ShowCursor(SDL_ENABLE);
	}

	log_std(("video:sdl: surface %dx%dx%d\n", (unsigned)sdl_state.surface->w, (unsigned)sdl_state.surface->h, (unsigned)sdl_state.surface->format->BitsPerPixel));
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

	/* write handler */
	if (mode->is_text) {
		sdl_write_line = sdl_linear_write_line_text;
	} else {
		sdl_write_line = sdl_linear_write_line_graphics;
	}

	sdl_state.mode_active = 1;
	return 0;
}

void sdl_mode_done(adv_bool restore) {
	assert( sdl_is_active() );
	assert( sdl_mode_is_active() );

	log_std(("video:sdl: sdl_mode_done()\n"));

#ifdef USE_VIDEO_RESTORE
	/* close the screen if we are fullscreen. */
	/* otherwise we cannot see the started programs by AdvanceMENU. */
	if ((sdl_state.surface->flags & SDL_FULLSCREEN) != 0) {
		if (SDL_WasInit(SDL_INIT_VIDEO)!=0) {
			log_std(("video:sdl: call SDL_QuitSubSystem(SDL_INIT_VIDEO)\n"));
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		}
	}
#endif

	sdl_state.surface = 0;

	free(sdl_state.text_map);
	sdl_state.text_map = 0;
	sdl_state.text_dx = 0;
	sdl_state.text_dy = 0;

	sdl_state.mode_active = 0;
}

unsigned sdl_virtual_x(void) {
	return sdl_state.surface->w;
}

unsigned sdl_virtual_y(void) {
	return sdl_state.surface->h;
}

unsigned sdl_font_size_x(void) {
	if (sdl_state.text_map)
		return 8;
	else
		return 0;
}

unsigned sdl_font_size_y(void) {
	if (sdl_state.text_map)
		return 16;
	else
		return 0;
}


unsigned sdl_adjust_bytes_per_page(unsigned bytes_per_page) {
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned sdl_bytes_per_scanline(void) {
	if (sdl_state.text_map)
		return sdl_state.text_dx * 2;
	else
		return sdl_state.surface->pitch;
}

adv_rgb_def sdl_rgb_def(void) {
	return rgb_def_make(
		8 - sdl_state.surface->format->Rloss, sdl_state.surface->format->Rshift,
		8 - sdl_state.surface->format->Gloss, sdl_state.surface->format->Gshift,
		8 - sdl_state.surface->format->Bloss, sdl_state.surface->format->Bshift
	);
}

static adv_color sdl_palette[16] = {
{   0,  0,  0,0 },
{ 192,  0,  0,0 },
{   0,192,  0,0 },
{ 192,192,  0,0 },
{   0,  0,192,0 },
{ 192,  0,192,0 },
{   0,192,192,0 },
{ 192,192,192,0 },
{ 128,128,128,0 },
{ 255,  0,  0,0 },
{   0,255,  0,0 },
{ 255,255,  0,0 },
{   0,  0,255,0 },
{ 255,  0,255,0 },
{   0,255,255,0 },
{ 255,255,255,0 }
};

#include "sdlfont.dat"

static void SDL_PutPixel(unsigned x, unsigned y, unsigned color) {
	unsigned bytes_per_scanline = sdl_state.surface->pitch;
	unsigned bytes_per_pixel = sdl_state.surface->format->BytesPerPixel;
	unsigned char* p = (unsigned char*)sdl_state.surface->pixels + bytes_per_scanline * y + bytes_per_pixel * x;
	switch (bytes_per_pixel) {
		case 1 :
			p[0] = color;
			break;
		case 2 :
			p[0] = color;
			p[1] = color >> 8;
			break;
		case 3 :
			p[0] = color;
			p[1] = color >> 8;
			p[2] = color >> 16;
			break;
		case 4 :
			p[0] = color;
			p[1] = color >> 8;
			p[2] = color >> 16;
			p[3] = color >> 24;
			break;
	}
}

void sdl_wait_vsync(void) {
	unsigned x, y;
	unsigned i;
	unsigned color[16];

	/* not available in graphics mode */
	if (!sdl_state.text_map)
		return;

	for(i=0;i<16;++i)
		color[i] = SDL_MapRGB(sdl_state.surface->format, sdl_palette[i].red, sdl_palette[i].green, sdl_palette[i].blue);

	sdl_write_lock();

	for(y=0;y<sdl_state.text_dy;++y) {
		for(x=0;x<sdl_state.text_dx;++x) {
			unsigned cx, cy;
			unsigned sx, sy;
			unsigned ch, at;
			unsigned c0, c1;
			unsigned char* bit;

			sx = x * 8;
			sy = y * 16;
			ch = sdl_state.text_map[(y*sdl_state.text_dx+x)*2];
			at = sdl_state.text_map[(y*sdl_state.text_dx+x)*2+1];
			c0 = color[(at >> 4) & 0xF];
			c1 = color[at & 0xF];
			bit = FONT + ch * 16;

			for(cy=0;cy<16;++cy) {
				for(cx=0;cx<8;++cx) {
					unsigned c;
					if ((bit[cy] & (0x80 >> cx)) != 0)
						c = c1;
					else
						c = c0;
					SDL_PutPixel(sx + cx, sy + cy, c);
				}
			}
		}
	}

	sdl_write_unlock(0,0,0,0);
}

adv_error sdl_scroll(unsigned offset, adv_bool waitvsync) {
	assert(sdl_is_active() && sdl_mode_is_active());

	if (offset != 0)
		return -1;

	return 0;
}

adv_error sdl_scanline_set(unsigned byte_length) {
	assert(sdl_is_active() && sdl_mode_is_active());

	if (byte_length != sdl_bytes_per_scanline())
		return -1;

	return 0;
}

adv_error sdl_palette8_set(const adv_color* palette, unsigned start, unsigned count, adv_bool waitvsync) {
	SDL_Color sdl_pal[256];
	unsigned i;

	for(i=0;i<count;++i) {
		sdl_pal[i].r = palette[i].red;
		sdl_pal[i].g = palette[i].green;
		sdl_pal[i].b = palette[i].blue;
	}

	if (SDL_SetPalette(sdl_state.surface, SDL_PHYSPAL, sdl_pal, start, count) != 1) {
		log_std(("video:sdl: SDL_SetPalette(SDL_PHYSPAL) failed, %s\n",  SDL_GetError()));
		return -1;
	}

	return 0;
}

#define DRIVER(mode) ((sdl_video_mode*)(&mode->driver_mode))

/**
 * Import information for one video mode.
 * \return 0 if successful
 */
adv_error sdl_mode_import(adv_mode* mode, const sdl_video_mode* sdl_mode) {
	log_std(("video:sdl: sdl_mode_import()\n"));

	if (sdl_mode->is_text)
		sprintf(mode->name,"sdl_text_%dx%dx%d",sdl_mode->size_x, sdl_mode->size_y, sdl_mode->bits_per_pixel);
	else
		sprintf(mode->name,"sdl_%dx%dx%d",sdl_mode->size_x, sdl_mode->size_y, sdl_mode->bits_per_pixel);

	*DRIVER(mode) = *sdl_mode;

	mode->driver = &video_sdl_driver;
	mode->flags = (mode->flags & MODE_FLAGS_USER_MASK);
	if (sdl_mode->is_text) {
		mode->flags |= MODE_FLAGS_TYPE_TEXT | MODE_FLAGS_INDEX_TEXT | MODE_FLAGS_MEMORY_LINEAR;
		mode->bits_per_pixel = 0;
	} else {
		switch (sdl_mode->bits_per_pixel) {
				case 8 :
				mode->flags |= MODE_FLAGS_INDEX_PACKED | MODE_FLAGS_TYPE_GRAPHICS;
				break;
			case 15 :
			case 16 :
			case 24 :
			case 32 :
				mode->flags |= MODE_FLAGS_INDEX_RGB | MODE_FLAGS_TYPE_GRAPHICS;
				break;
			default :
				return -1;
		}
		mode->flags |= MODE_FLAGS_MEMORY_LINEAR;
		mode->bits_per_pixel = sdl_mode->bits_per_pixel;
	}

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

static adv_error sdl_mode_generate_graphics(sdl_video_mode* mode, const adv_crtc* crtc, unsigned bits, unsigned flags) {
	unsigned suggested_bits;

	suggested_bits = SDL_VideoModeOK(crtc->hde, crtc->vde, bits, SDL_ModeFlags() );

	if (!suggested_bits) {
		error_nolog_cat("sdl: No compatible SDL mode found\n");
		return -1;
	}

	if (suggested_bits != bits) {
		/* if a window manager is active accepts any bit depths */
		if ((sdl_flags() & VIDEO_DRIVER_FLAGS_INFO_WINDOWMANAGER)==0) {
			error_nolog_cat("sdl: No compatible SDL bit depth found\n");
			return -1;
		}
	}

	mode->size_x = crtc->hde;
	mode->size_y = crtc->vde;
	mode->bits_per_pixel = bits;
	mode->is_text = 0;

	return 0;
}

static adv_error sdl_mode_generate_text(sdl_video_mode* mode, const adv_crtc* crtc, unsigned flags) {
	unsigned suggested_bits;

	suggested_bits = SDL_VideoModeOK(crtc->hde, crtc->vde, SDL_TEXT_BIT, 0);

	if (!suggested_bits) {
		error_nolog_cat("sdl: No compatible SDL mode found\n");
		return -1;
	}

	mode->size_x = crtc->hde;
	mode->size_y = crtc->vde;
	mode->bits_per_pixel = 0;
	mode->is_text = 1;

	return 0;
}

adv_error sdl_mode_generate(sdl_video_mode* mode, const adv_crtc* crtc, unsigned bits, unsigned flags)
{
	log_std(("video:sdl: sdl_mode_generate(x:%d,y:%d,bits:%d)\n", crtc->hde, crtc->vde, bits));

	if (!crtc_is_fake(crtc)) {
		error_nolog_cat("sdl: Programmable modes not supported\n");
		return -1;
	}

	switch (flags & MODE_FLAGS_TYPE_MASK) {
		case MODE_FLAGS_TYPE_GRAPHICS :
			if (sdl_mode_generate_graphics(mode, crtc, bits, flags) != 0)
				return -1;
			break;
		case MODE_FLAGS_TYPE_TEXT :
			if (sdl_mode_generate_text(mode, crtc, flags) != 0)
				return -1;
			break;
		default :
			return -1;
	}

	return 0;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1;

int sdl_mode_compare(const sdl_video_mode* a, const sdl_video_mode* b) {
	COMPARE(a->is_text,b->is_text)
	COMPARE(a->size_y,b->size_y)
	COMPARE(a->size_x,b->size_x)
	if (!a->is_text) {
		COMPARE(a->bits_per_pixel,b->bits_per_pixel)
	}
	return 0;
}

void sdl_crtc_container_insert_default(adv_crtc_container* cc) {
	unsigned i;
	SDL_Rect** map;

	log_std(("video:sdl: sdl_crtc_container_insert_default()\n"));

	map = SDL_ListModes(0, SDL_ModeFlags() );
	if (!map) {
		/* no resolutions */
	} else if (map == (SDL_Rect **)-1) {
		/* all resolutions available */
	} else {
		for(i=0;map[i];++i) {
			adv_crtc crtc;
			crtc_fake_set(&crtc, map[i]->w, map[i]->h);
			sprintf(crtc.name, "sdl_%dx%d", map[i]->w, map[i]->h);
			crtc_container_insert(cc, &crtc);
		}
	}
}

void sdl_default(void) {
	sdl_option.fullscreen = 0;

	sdl_option.initialized = 1;
}

void sdl_reg(adv_conf* context) {
	assert( !sdl_is_active() );

	conf_bool_register_default(context, "device_sdl_fullscreen", 0);
}

adv_error sdl_load(adv_conf* context) {
	/* Options must be loaded before initialization */
	assert( !sdl_is_active() );

	sdl_option.fullscreen = conf_bool_get_default(context, "device_sdl_fullscreen");

	sdl_option.initialized = 1;

	return 0;
}

/***************************************************************************/
/* Driver */

static adv_error sdl_mode_set_void(const void* mode) {
	return sdl_mode_set((const sdl_video_mode*)mode);
}

static adv_error sdl_mode_import_void(adv_mode* mode, const void* sdl_mode) {
	return sdl_mode_import(mode, (const sdl_video_mode*)sdl_mode);
}

static adv_error sdl_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned bits, unsigned flags) {
	return sdl_mode_generate((sdl_video_mode*)mode, crtc, bits, flags);
}

static int sdl_mode_compare_void(const void* a, const void* b) {
	return sdl_mode_compare((const sdl_video_mode*)a, (const sdl_video_mode*)b);
}

static unsigned sdl_mode_size(void) {
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
	0,
	sdl_mode_done,
	sdl_virtual_x,
	sdl_virtual_y,
	sdl_font_size_x,
	sdl_font_size_y,
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
	sdl_mode_compare_void,
	sdl_crtc_container_insert_default
};

