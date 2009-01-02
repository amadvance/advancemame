/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2004, 2005 Andrea Mazzoleni
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

#include "portable.h"

#include "vsvgawin.h"
#include "video.h"
#include "log.h"
#include "ossdl.h"
#include "error.h"
#include "conf.h"
#include "snstring.h"

#ifdef USE_KEYBOARD_SDL
#include "ksdl.h"
#endif

#include "SDL.h"

#include <windows.h>

#include "svgalib.h"

/***************************************************************************/
/* State */

struct svgawin_chipset_struct {
	DriverSpecs* drv;
	int chipset;
	const char* name;
	unsigned cap;
};

typedef struct svgawin_internal_struct {
	adv_bool active; /**< !=0 if present. */
	adv_bool mode_active; /**< !=0 if mode set. */
	adv_bool lock_active; /**< !=0 if lock active. */

	unsigned cap;
	unsigned char regs_saved[ADV_SVGALIB_STATE_SIZE];

	SDL_Surface* surface;
} svgawin_internal;

static svgawin_internal svgawin_state;

unsigned char* (*svgawin_write_line)(unsigned y);

/***************************************************************************/
/* Options */

#define STUB_NONE 0 /**< Don't use any Windows graphics output. */
#define STUB_WINDOW 1 /**< Create a stub window using SDL. */
#define STUB_FULLSCREEN 2 /**< Create a stub fullscreen using SDL. */

struct svgawin_option_struct {
	adv_bool initialized;
	int stub;
	int divide_clock;
	int skip;
};

static struct svgawin_option_struct svgawin_option;

/***************************************************************************/
/* Internal */

static adv_bool is_svgalib_framebuffer_valid(void)
{
	return adv_svgalib_linear_pointer_get() != MAP_FAILED;
}

static adv_bool is_sdl_framebuffer_valid(void)
{
	if (svgawin_option.stub == STUB_FULLSCREEN) {
		unsigned flags = SDL_FULLSCREEN | SDL_HWSURFACE;

		return (svgawin_state.surface->flags & flags) == flags;
	} else {
		return 0;
	}
}

static unsigned char* svgawin_svgalib_write_line(unsigned y)
{
	assert(svgawin_state.lock_active);
	return (unsigned char*)adv_svgalib_linear_pointer_get() + adv_svgalib_scanline_get() * y;
}

static unsigned char* svgawin_sdl_write_line(unsigned y)
{
	assert(svgawin_state.lock_active);
	return (unsigned char*)svgawin_state.surface->pixels + adv_svgalib_scanline_get() * y;;
}

/* Keep the same order of svgawin_chipset_struct cards */
static adv_device DEVICE[] = {
	{ "auto", -1, "SVGAWIN video" },
#ifdef INCLUDE_NV3_DRIVER
	{ "nv3", 0, "nVidia Riva/GeForce" },
#endif
#ifdef INCLUDE_NV3_DRIVER
	{ "nv3_leg", 1, "nVidia Riva/GeForce (SVGALIB 1.9.19)" },
#endif
#ifdef INCLUDE_TRIDENT_DRIVER
	{ "trident", 2, "Trident" },
#endif
#ifdef INCLUDE_RENDITION_DRIVER
	{ "rendition", 3, "Rendition" },
#endif
#ifdef INCLUDE_G400_DRIVER
	{ "g400", 4, "Matrox Mystique/G100/G200/G400/G450" },
#endif
#ifdef INCLUDE_PM2_DRIVER
	{ "pm2", 5, "Permedia 2" },
#endif
#ifdef INCLUDE_UNICHROME_DRIVER
	{ "unichrome", 6, "VIA Unichrome" },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ "savage", 7, "S3 Savage" },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ "savage_leg", 8, "S3 Savage (SVGALIB 1.9.18)" },
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER
	{ "millenium", 9, "Matrox Millennium/Millenium II" },
#endif
#ifdef INCLUDE_R128_DRIVER
	{ "r128", 10, "ATI Rage 128/Radeon" },
#endif
#ifdef INCLUDE_BANSHEE_DRIVER
	{ "banshee", 11, "3dfx Voodoo Banshee/3/4/5" },
#endif
#ifdef INCLUDE_SIS_DRIVER
	{ "sis", 12, "SIS" },
#endif
#ifdef INCLUDE_LAGUNA_DRIVER
	{ "laguna", 13, "Cirrus Logic Laguna 5462/5464/5465" },
#endif
#ifdef INCLUDE_RAGE_DRIVER
	{ "rage", 14, "ATI Rage" },
#endif
#ifdef INCLUDE_MX_DRIVER
	{ "mx", 15, "MX" },
#endif
#ifdef INCLUDE_ET6000_DRIVER
	{ "et6000", 16, "ET6000" },
#endif
#ifdef INCLUDE_S3_DRIVER
	{ "s3", 17, "S3" },
#endif
#ifdef INCLUDE_ARK_DRIVER
	{ "ark", 18, "ARK" },
#endif
#ifdef INCLUDE_APM_DRIVER
	{ "apm", 19, "APM" },
#endif
	{ 0, 0, 0 }
};

/***************************************************************************/
/* Public */

#include "icondef.dat"

static void SDL_WM_DefIcon(void)
{
	SDL_Surface* surface;
	SDL_Color colors[ICON_PALETTE];
	unsigned i, x, y;

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

adv_bool svgawin_is_active(void)
{
	return svgawin_state.active != 0;
}

adv_bool svgawin_mode_is_active(void)
{
	return svgawin_state.mode_active != 0;
}

static adv_error sdl_init(int device_id)
{
	char buf[64];
	unsigned j;

	assert(!svgawin_is_active());

	svgawin_state.surface = 0;

	if (SDL_WasInit(SDL_INIT_VIDEO)==0) {
		log_std(("video:svgawin: call SDL_InitSubSystem(SDL_INIT_VIDEO)\n"));
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			error_set("Unable to initialize the SDL library, %s.\n", SDL_GetError());
			return -1;
		}

		/* set the window information */
		SDL_WM_SetCaption(os_internal_sdl_title_get(), os_internal_sdl_title_get());
		SDL_WM_DefIcon();
	}

	if (SDL_VideoDriverName(buf, sizeof(buf))) {
		log_std(("video:svgawin: SDL driver %s\n", buf));
	}

	return 0;
}

static int probe_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg)
{
	unsigned dw;
	unsigned base_class;

	if (adv_svgalib_pci_read_dword(bus_device_func, 0x8, &dw)!=0)
		return 0;

	base_class = (dw >> 16) & 0xFFFF;
	if (base_class != 0x300 /* DISPLAY | VGA */)
		return 0;

	*(int*)_arg = 1;

	log_std(("video:svgawin: PCI/AGP Board VendorID %04x, DeviceID %04x, Bus %d, Device %d\n", vendor, device, bus_device_func >> 8, bus_device_func & 0xFF));

	return 0;
}

static void probe(void)
{
	int found;
	found = 0;

	adv_svgalib_pci_scan_device(probe_callback, &found);

	if (!found)
		log_std(("video:svgawin: No PCI/AGP boards found\n"));

	log_std(("video:svgawin: Video driver : %s\n", adv_svgalib_driver_get()));

	log_std(("video:svgawin: Bit depth : "));
	if (adv_svgalib_state.has_bit8) log_std(("8 "));
	if (adv_svgalib_state.has_bit15) log_std(("15 "));
	if (adv_svgalib_state.has_bit16) log_std(("16 "));
	if (adv_svgalib_state.has_bit24) log_std(("24 "));
	if (adv_svgalib_state.has_bit32) log_std(("32 "));
	log_std(("\n"));
	if (adv_svgalib_state.has_interlace)
		log_std(("video:svgawin: Interlace : yes\n"));
	else
		log_std(("video:svgawin: Interlace : no\n"));
	log_std(("video:svgawin: Linear memory : %08x, %d Mbyte\n", adv_svgalib_linear_base_get(), adv_svgalib_linear_size_get() / (1024*1024)));
	if (adv_svgalib_mmio_size_get()) {
		log_std(("video:svgawin: MMIO memory : %08x, %d byte\n", adv_svgalib_mmio_base_get(), adv_svgalib_mmio_size_get()));
	}
}

static adv_error svgalib_init(int device_id)
{
	unsigned i;
	const char* name;
	const adv_device* j;

	j = DEVICE;
	while (j->name && j->id != device_id)
		++j;
	if (!j->name)
		return -1;
	name = j->name;

	if (adv_svgalib_init(svgawin_option.divide_clock, svgawin_option.skip) != 0) {
		error_set("Unable to initialize the SVGAWIN library.\n");
		return -1;
	}

	if (adv_svgalib_detect(name) != 0) {
		error_set("Unable to detect the video board.\n");
		return -1;
	}

	probe();

	log_std(("video:svgawin: found driver %s\n", adv_svgalib_driver_get()));

	svgawin_state.cap = VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC
		| VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;

	if (adv_svgalib_state.has_bit8)
		svgawin_state.cap |= VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
	if (adv_svgalib_state.has_bit15)
		svgawin_state.cap |= VIDEO_DRIVER_FLAGS_MODE_BGR15;
	if (adv_svgalib_state.has_bit16)
		svgawin_state.cap |= VIDEO_DRIVER_FLAGS_MODE_BGR16;
	if (adv_svgalib_state.has_bit24)
		svgawin_state.cap |= VIDEO_DRIVER_FLAGS_MODE_BGR24;
	if (adv_svgalib_state.has_bit32)
		svgawin_state.cap |= VIDEO_DRIVER_FLAGS_MODE_BGR32;
	if (adv_svgalib_state.has_interlace)
		svgawin_state.cap |= VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;

	return 0;
}

adv_error svgawin_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	(void)cursor;

	log_std(("video:svgawin: svgawin_init(device_id:%d,output:%x)\n", device_id, (unsigned)output));

	if (sizeof(svgawin_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	if (!svgawin_option.initialized) {
		svgawin_default();
	}

	svgawin_state.mode_active = 0;
	svgawin_state.lock_active = 0;

	switch (svgawin_option.stub) {
	case STUB_NONE : log_std(("video:svgawin: stub none\n")); break;
	case STUB_WINDOW : log_std(("video:svgawin: stub window\n")); break;
	case STUB_FULLSCREEN : log_std(("video:svgawin: stub fullscreen\n")); break;
	default: 
		log_std(("video:svgawin: stub unknow\n")); 
		return -1;
	}

	if (svgawin_option.stub != STUB_NONE) {
		if (sdl_init(device_id) != 0)
			return -1;
	}

	if (svgalib_init(device_id) != 0)
		return -1;

	svgawin_state.active = 1;

	return 0;
}

static void sdl_done(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)!=0) {
		log_std(("video:svgawin: call SDL_QuitSubSystem(SDL_INIT_VIDEO)\n"));
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}

static void svgalib_done(void)
{
	adv_svgalib_done();
}

void svgawin_done(void)
{
	assert(svgawin_is_active());
	assert(!svgawin_mode_is_active());

	log_std(("video:svgawin: svgawin_done()\n"));

	svgalib_done();

	if (svgawin_option.stub != STUB_NONE) {
		sdl_done();
	}

	svgawin_state.active = 0;
}

unsigned svgawin_flags(void)
{
	assert(svgawin_is_active());
	return svgawin_state.cap;
}

void svgawin_write_lock(void)
{
	assert(!svgawin_state.lock_active);

	if (svgawin_option.stub != STUB_NONE) {
		if (SDL_MUSTLOCK(svgawin_state.surface)) {
			while (SDL_LockSurface(svgawin_state.surface) != 0) {
				SDL_Delay(1);
			}
		}
	}

	svgawin_state.lock_active = 1;
}

void svgawin_write_unlock(unsigned x, unsigned y, unsigned size_x, unsigned size_y, adv_bool waitvsync)
{
	assert(svgawin_state.lock_active);

	if (svgawin_option.stub != STUB_NONE) {
		if (SDL_MUSTLOCK(svgawin_state.surface))
			SDL_UnlockSurface(svgawin_state.surface);
	}

	svgawin_state.lock_active = 0;
}

static void sdl_mode_done(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)!=0) {
		log_std(("video:svgawin: call SDL_QuitSubSystem(SDL_INIT_VIDEO)\n"));
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}

	svgawin_state.surface = 0;
}

static void windows_restore(void)
{
	DEVMODE mode;

	memset(&mode, 0, sizeof(mode));
	mode.dmSize = sizeof(mode);
	mode.dmFields = 0;

	ShowCursor(FALSE);

	if (ChangeDisplaySettings(&mode, CDS_RESET) != DISP_CHANGE_SUCCESSFUL) {
		return;
	}

	ShowCursor(TRUE);
}

static void svgalib_mode_done(void)
{
	adv_svgalib_unset();

	adv_svgalib_linear_unmap();
}

void svgawin_mode_done(adv_bool restore)
{
	assert(svgawin_is_active());
	assert(svgawin_mode_is_active());

	log_std(("video:svgawin: svgawin_mode_done()\n"));

	svgalib_mode_done();

	if (svgawin_option.stub != STUB_FULLSCREEN) {
		/* restore the register only if required */
		if (restore) {
			/* first return to the original Windows mode */
			windows_restore();
			/* set the original registers */
			adv_svgalib_restore(svgawin_state.regs_saved);
		}
	}
	if (svgawin_option.stub != STUB_NONE) {
		sdl_mode_done();
	}
	if (svgawin_option.stub == STUB_FULLSCREEN) {
		/* on fullscreen stub mode the sdl always reset the screen, so we need to always restore the registers */

		/* first return to the original Windows mode */
		windows_restore();

		/* set the original registers */
		adv_svgalib_restore(svgawin_state.regs_saved);
	}

	svgawin_state.mode_active = 0;
}

static adv_error sdl_mode_set(const svgawin_video_mode* mode)
{
	const SDL_VideoInfo* info;
	unsigned x, y, bits;
	unsigned flags;
	const char* sflags;

	/* reopen the screen if needed */
	if (SDL_WasInit(SDL_INIT_VIDEO)==0) {
		log_std(("video:svgawin: call SDL_InitSubSystem(SDL_INIT_VIDEO)\n"));
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
			log_std(("video:svgawin: SDL_InitSubSystem(SDL_INIT_VIDEO) failed, %s\n",  SDL_GetError()));
			return -1;
		}

		/* set the window information */
		SDL_WM_SetCaption(os_internal_sdl_title_get(), os_internal_sdl_title_get());
		SDL_WM_DefIcon();
	}

#ifdef USE_KEYBOARD_SDL
	/* clear all the keyboard state, it depends on the video */
	keyb_sdl_event_release_all();
#endif

	info = SDL_GetVideoInfo();

	log_std(("video:svgawin: video hw_available:%d\n", (unsigned)info->hw_available));
	log_std(("video:svgawin: video wm_available:%d\n", (unsigned)info->wm_available));
	log_std(("video:svgawin: video blit_hw:%d\n", (unsigned)info->blit_hw));
	log_std(("video:svgawin: video blit_hw_CC:%d\n", (unsigned)info->blit_hw_CC));
	log_std(("video:svgawin: video blit_hw_A:%d\n", (unsigned)info->blit_hw_A));
	log_std(("video:svgawin: video blit_sw:%d\n", (unsigned)info->blit_sw));
	log_std(("video:svgawin: video blit_sw_CC:%d\n", (unsigned)info->blit_sw_CC));
	log_std(("video:svgawin: video blit_sw_A:%d\n", (unsigned)info->blit_sw_A));
	log_std(("video:svgawin: video blit_fill:%d\n", (unsigned)info->blit_fill));
	log_std(("video:svgawin: video video_mem:%d\n", (unsigned)info->video_mem));

	/* the 640x480 mode is always available */
	x = 640;
	y = 480;

	/* use the same bit depth of the requested mode */
	bits = index_bits_per_pixel(mode->index);

	/* HACK the nv3 driver displays incorrectly with 15 bit modes if the same bit depth of the video mode is used */
	if (strcmp(adv_svgalib_driver_get(), "nv3") == 0 && bits == 15) {
		log_std(("video:svgawin: adjusting bit depth from %d to 16 for nv3 driver", bits));
		bits = 16;
	}

	if (svgawin_option.stub == STUB_FULLSCREEN) {
		flags = SDL_FULLSCREEN | SDL_HWSURFACE;
		sflags = "SDL_FULLSCREEN | SDL_HWSURFACE";
	} else {
		flags = 0;
		sflags = "0";
	}

	log_std(("video:svgawin: call SDL_SetVideoMode(%d, %d, %d, %s)\n", x, y, bits, sflags));
	svgawin_state.surface = SDL_SetVideoMode(x, y, bits, flags);
	if (!svgawin_state.surface) {
		log_std(("video:svgawin: SDL_SetVideoMode(%d, %d, %d, %s) failed, %s\n", x, y, bits, sflags, SDL_GetError()));
		error_set("SDL is unable to set the video mode %dx%d, %d bits, %s flags.\n", x, y, bits, sflags);
		return -1;
	}

	log_std(("video:svgawin: surface %dx%dx%d\n", (unsigned)svgawin_state.surface->w, (unsigned)svgawin_state.surface->h, (unsigned)svgawin_state.surface->format->BitsPerPixel));
	if ((svgawin_state.surface->flags & SDL_SWSURFACE) != 0)
		log_std(("video:svgawin: surface flag SDL_SWSURFACE\n"));
	if ((svgawin_state.surface->flags & SDL_HWSURFACE) != 0)
		log_std(("video:svgawin: surface flag SDL_HWSURFACE\n"));
	if ((svgawin_state.surface->flags & SDL_ASYNCBLIT) != 0)
		log_std(("video:svgawin: surface flag SDL_ASYNCBLIT\n"));
	if ((svgawin_state.surface->flags & SDL_ANYFORMAT) != 0)
		log_std(("video:svgawin: surface flag SDL_ANYFORMAT\n"));
	if ((svgawin_state.surface->flags & SDL_HWPALETTE) != 0)
		log_std(("video:svgawin: surface flag SDL_HWPALETTE\n"));
	if ((svgawin_state.surface->flags & SDL_DOUBLEBUF) != 0)
		log_std(("video:svgawin: surface flag SDL_DOUBLEBUF\n"));
	if ((svgawin_state.surface->flags & SDL_FULLSCREEN) != 0)
		log_std(("video:svgawin: surface flag SDL_FULLSCREEN\n"));
	if ((svgawin_state.surface->flags & SDL_RESIZABLE) != 0)
		log_std(("video:svgawin: surface flag SDL_RESIZABLE\n"));
	if ((svgawin_state.surface->flags & SDL_HWACCEL) != 0)
		log_std(("video:svgawin: surface flag SDL_HWACCEL\n"));
	if ((svgawin_state.surface->flags & SDL_SRCCOLORKEY) != 0)
		log_std(("video:svgawin: surface flag SDL_SRCCOLORKEY\n"));
	if ((svgawin_state.surface->flags & SDL_SRCALPHA) != 0)
		log_std(("video:svgawin: surface flag SDL_SRCALPHA\n"));
	if ((svgawin_state.surface->flags & SDL_PREALLOC) != 0)
		log_std(("video:svgawin: surface flag SDL_PREALLOC\n"));

	/* disable the mouse cursor on fullscreen mode */
	if ((svgawin_state.surface->flags & SDL_FULLSCREEN) != 0) {
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_ShowCursor(SDL_ENABLE);
	}

	/* enable window manager events */
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	return 0;
}

static adv_error svgalib_mode_set(const svgawin_video_mode* mode)
{
	unsigned clock;

	log_std(("video:svgawin: mode_set\n"));
	log_std_modeline_c(("video:svgawin: mode_set modeline", mode->crtc.pixelclock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc)));
	log_std(("video:svgawin: expected vert clock: %.2f Hz\n", crtc_vclock_get(&mode->crtc)));

	clock = mode->crtc.pixelclock;
	if (svgawin_option.divide_clock)
		clock *= 2;

	adv_svgalib_linear_map();

	if (adv_svgalib_set(clock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc), crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), index_bits_per_pixel(mode->index), 0, 0) != 0) {
		adv_svgalib_linear_unmap();
		error_set("Generic error setting the svgalib mode\n.");
		return -1;
	}

	log_std(("video:svgawin: mode_set done\n"));

	return 0;
}

adv_error svgawin_mode_set(const svgawin_video_mode* mode)
{
	assert(svgawin_is_active());
	assert(!svgawin_mode_is_active());

	log_std(("video:svgawin: svgawin_mode_set()\n"));

	if (svgawin_option.stub == STUB_FULLSCREEN) {
		adv_svgalib_save(svgawin_state.regs_saved);
	}
	if (svgawin_option.stub != STUB_NONE) {
		if (sdl_mode_set(mode) != 0)
			goto err;
	} 
	if (svgawin_option.stub != STUB_FULLSCREEN) {
		adv_svgalib_save(svgawin_state.regs_saved);
	}

	if (svgalib_mode_set(mode) != 0) {
		goto err_sdl;
	}

	log_std(("video:svgawin: lock the video memory\n"));
	svgawin_write_lock();

	/* select the framebuffer to use */
	if (is_svgalib_framebuffer_valid()) {
		svgawin_write_line = svgawin_svgalib_write_line;
		log_std(("video:svgawin: using svgalib linear pointer %08x\n", (unsigned)adv_svgalib_linear_pointer_get()));
	} else if (is_sdl_framebuffer_valid()) {
		svgawin_write_line = svgawin_sdl_write_line;
		log_std(("video:svgawin: using sdl linear pointer %08x\n", (unsigned)svgawin_state.surface->pixels));
	} else {
		error_set("Unusable framebuffer pointer.\n");
		goto err_svgalib_lock;
	}

	log_std(("video:svgawin: clear the video memory\n"));
	memset(svgawin_write_line(0), 0, adv_svgalib_scanline_get() * mode->crtc.vde);

	log_std(("video:svgawin: unlock the video memory\n"));
	svgawin_write_unlock(0, 0, 0, 0, 0);

	svgawin_state.mode_active = 1;

	return 0;

err_svgalib_lock:
	svgawin_write_unlock(0, 0, 0, 0, 0);
err_svgalib:
	svgalib_mode_done();
err_sdl:
	if (svgawin_option.stub != STUB_FULLSCREEN) {
		adv_svgalib_restore(svgawin_state.regs_saved);
		windows_restore();
	}
	if (svgawin_option.stub != STUB_NONE) {
		sdl_mode_done();
	}
	if (svgawin_option.stub == STUB_FULLSCREEN) {
		adv_svgalib_restore(svgawin_state.regs_saved);
	}
err:
	return -1;
}

adv_error svgawin_mode_change(const svgawin_video_mode* mode)
{
	assert(svgawin_is_active() && svgawin_mode_is_active());

	log_std(("video:svgawin: svgawin_mode_change()\n"));

	svgalib_mode_done();

	svgawin_state.mode_active = 0;

	if (svgalib_mode_set(mode) != 0) {
		if (svgawin_option.stub != STUB_FULLSCREEN) {
			adv_svgalib_restore(svgawin_state.regs_saved);
			windows_restore();
		}
		if (svgawin_option.stub != STUB_NONE) {
			sdl_mode_done();
		}
		if (svgawin_option.stub == STUB_FULLSCREEN) {
			adv_svgalib_restore(svgawin_state.regs_saved);
		}
		return -1;
	}

	/* write handler */
	if (svgawin_option.stub == STUB_FULLSCREEN) {
		svgawin_write_line = svgawin_sdl_write_line;
	} else {
		svgawin_write_line = svgawin_svgalib_write_line;
	}

	svgawin_state.mode_active = 1;

	return 0;
}

unsigned svgawin_virtual_x(void)
{
	unsigned size = adv_svgalib_scanline_get() / adv_svgalib_pixel_get();
	size = size & ~0x7;
	return size;
}

unsigned svgawin_virtual_y(void)
{
	return adv_svgalib_linear_size_get() / adv_svgalib_scanline_get();
}

unsigned svgawin_adjust_bytes_per_page(unsigned bytes_per_page)
{
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned svgawin_bytes_per_scanline(void)
{
	return adv_svgalib_scanline_get();
}

adv_color_def svgawin_color_def(void)
{
	if (adv_svgalib_pixel_get() == 1)
		return color_def_make(adv_color_type_palette);
	else
		return color_def_make_rgb_from_sizelenpos(adv_svgalib_state.mode.bytes_per_pixel, adv_svgalib_state.mode.red_len, adv_svgalib_state.mode.red_pos, adv_svgalib_state.mode.green_len, adv_svgalib_state.mode.green_pos, adv_svgalib_state.mode.blue_len, adv_svgalib_state.mode.blue_pos);
}

void svgawin_wait_vsync(void)
{
	assert(svgawin_is_active() && svgawin_mode_is_active());

	adv_svgalib_wait_vsync();
}

adv_error svgawin_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(svgawin_is_active() && svgawin_mode_is_active());

	if (waitvsync)
		adv_svgalib_wait_vsync();
		
	adv_svgalib_scroll_set(offset);

	return 0;
}

adv_error svgawin_scanline_set(unsigned byte_length)
{
	assert(svgawin_is_active() && svgawin_mode_is_active());

	adv_svgalib_scanline_set(byte_length);

	return 0;
}

adv_error svgawin_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
	if (waitvsync)
		adv_svgalib_wait_vsync();
		
	while (count) {
		adv_svgalib_palette_set(start, palette->red, palette->green, palette->blue);
		++palette;
		++start;
		--count;
	}

	return 0;
}

#define DRIVER(mode) ((svgawin_video_mode*)(&mode->driver_mode))

/**
 * Import information for one video mode.
 * \return 0 if successful
 */
adv_error svgawin_mode_import(adv_mode* mode, const svgawin_video_mode* svgawin_mode)
{
	log_std(("video:svgawin: svgawin_mode_import()\n"));

	sncpy(mode->name, MODE_NAME_MAX, svgawin_mode->crtc.name);

	*DRIVER(mode) = *svgawin_mode;

	mode->driver = &video_svgawin_driver;
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC | MODE_FLAGS_RETRACE_SET_ASYNC
		| (mode->flags & MODE_FLAGS_USER_MASK)
		| svgawin_mode->index;
	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

adv_error svgawin_mode_generate(svgawin_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	assert(svgawin_is_active());

	log_std(("video:svgawin: svgawin_mode_generate(x:%d, y:%d)\n", crtc->hde, crtc->vde));

	if (crtc_is_fake(crtc)) {
		error_nolog_set("Not programmable modes not supported.\n");
		return -1;
	}

	if (video_mode_generate_check("svgawin", svgawin_flags(), 8, 2048, crtc, flags)!=0)
		return -1;

	if (adv_svgalib_check(crtc->pixelclock, crtc->hde, crtc->hrs, crtc->hre, crtc->ht, crtc->vde, crtc->vrs, crtc->vre, crtc->vt, crtc_is_doublescan(crtc), crtc_is_interlace(crtc), crtc_is_nhsync(crtc), crtc_is_nvsync(crtc), index_bits_per_pixel(flags & MODE_FLAGS_INDEX_MASK), 0, 0) != 0) {
		error_nolog_set("Generic error checking the availability of the video mode.\n");
		return -1;
	}

	mode->crtc = *crtc;
	mode->index = flags & MODE_FLAGS_INDEX_MASK;

	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int svgawin_mode_compare(const svgawin_video_mode* a, const svgawin_video_mode* b)
{
	COMPARE(a->index, b->index);
	return crtc_compare(&a->crtc, &b->crtc);
}

void svgawin_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:svgawin: svgawin_crtc_container_insert_default()\n"));

	crtc_container_insert_default_modeline_svga(cc);
}

void svgawin_default(void)
{
	svgawin_option.initialized = 1;
	svgawin_option.divide_clock = 0;
	svgawin_option.skip = 0;
	svgawin_option.stub = 0;
}

static adv_conf_enum_int OPTION_STUB[] = {
{ "none", STUB_NONE },
{ "window", STUB_WINDOW },
{ "fullscreen", STUB_FULLSCREEN }
};

void svgawin_reg(adv_conf* context)
{
	assert(!svgawin_is_active());

	conf_bool_register_default(context, "device_svgawin_divideclock", 0);
	conf_int_register_limit_default(context, "device_svgawin_skipboard", 0, 8, 0);
	conf_int_register_enum_default(context, "device_svgawin_stub", conf_enum(OPTION_STUB), STUB_FULLSCREEN);

	svgawin_option.initialized = 1;
}

adv_error svgawin_load(adv_conf* context)
{
	assert(!svgawin_is_active());

	svgawin_option.divide_clock = conf_bool_get_default(context, "device_svgawin_divideclock");
	svgawin_option.skip = conf_int_get_default(context, "device_svgawin_skipboard");
	svgawin_option.stub = conf_int_get_default(context, "device_svgawin_stub");

	svgawin_option.initialized = 1;

	return 0;
}

/***************************************************************************/
/* Driver */

static adv_error svgawin_mode_set_void(const void* mode)
{
	return svgawin_mode_set((const svgawin_video_mode*)mode);
}

static adv_error svgawin_mode_change_void(const void* mode)
{
	return svgawin_mode_change((const svgawin_video_mode*)mode);
}

static adv_error svgawin_mode_import_void(adv_mode* mode, const void* svgawin_mode)
{
	return svgawin_mode_import(mode, (const svgawin_video_mode*)svgawin_mode);
}

static adv_error svgawin_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return svgawin_mode_generate((svgawin_video_mode*)mode, crtc, flags);
}

static int svgawin_mode_compare_void(const void* a, const void* b)
{
	return svgawin_mode_compare((const svgawin_video_mode*)a, (const svgawin_video_mode*)b);
}

static unsigned svgawin_mode_size(void)
{
	return sizeof(svgawin_video_mode);
}

adv_video_driver video_svgawin_driver = {
	"svgawin",
	DEVICE,
	svgawin_load,
	svgawin_reg,
	svgawin_init,
	svgawin_done,
	svgawin_flags,
	svgawin_mode_set_void,
	svgawin_mode_change_void,
	svgawin_mode_done,
	svgawin_virtual_x,
	svgawin_virtual_y,
	0,
	0,
	svgawin_bytes_per_scanline,
	svgawin_adjust_bytes_per_page,
	svgawin_color_def,
	svgawin_write_lock,
	svgawin_write_unlock,
	&svgawin_write_line,
	svgawin_wait_vsync,
	svgawin_scroll,
	svgawin_scanline_set,
	svgawin_palette8_set,
	svgawin_mode_size,
	0,
	svgawin_mode_generate_void,
	svgawin_mode_import_void,
	svgawin_mode_compare_void,
	svgawin_crtc_container_insert_default
};

/***************************************************************************/
/* Internal interface */

int os_internal_svgawin_is_video_active(void)
{
	return svgawin_is_active();
}

int os_internal_svgawin_is_video_mode_active(void)
{
	return svgawin_is_active() && svgawin_mode_is_active();
}

