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

#include "os.h"
#include "oslinux.h"
#include "log.h"
#include "target.h"
#include "file.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <sys/time.h>

/* Check if svgalib is used in some way */
#if defined(USE_VIDEO_SVGALIB) || defined(USE_KEYBOARD_SVGALIB) || defined(USE_MOUSE_SVGALIB) || defined(USE_JOYSTICK_SVGALIB)
#define USE_SVGALIB
#include <vga.h>
#include <vgamouse.h>
#endif

/* Check if sLang is used in some way */
#if defined(USE_VIDEO_SLANG) || defined(USE_INPUT_SLANG)
#define USE_SLANG
#include <slang/slang.h>
#endif

/* Check if dga is used in some way */
#if defined(USE_VIDEO_X) || defined(USE_KEYBOARD_X) || defined(USE_MOUSE_X)
#define USE_X
#include <X11/Xlib.h>
#endif

/* Check if SDL is used in some way */
#if defined(USE_VIDEO_SDL) || defined(USE_KEYBOARD_SDL) || defined(USE_MOUSE_SDL) || defined(USE_JOYSTICK_SDL) || defined(USE_SOUND_SDL) || defined(USE_INPUT_SDL)
#define USE_SDL
#include "ossdl.h"
#include "SDL.h"
#include "ksdl.h"
#include "isdl.h"
#include "msdl.h"
#endif

struct os_context {
#ifdef USE_SVGALIB
	int svgalib_active; /**< SVGALIB initialized. */
#endif

#ifdef USE_SLANG
	int slang_active; /**< Slang initialized. */
#endif

#ifdef USE_X
	int x_active; /**< X initialized. */
	Display* x_display; /**< X display. */
#endif

#ifdef USE_SDL
	int sdl_active; /**< SDL initialized. */
#endif

	int is_quit; /**< Is termination requested. */
	char title[128]; /**< Title of the window. */
};

static struct os_context OS;

/***************************************************************************/
/* Clock */

os_clock_t OS_CLOCKS_PER_SEC = 1000000LL;

os_clock_t os_clock(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000LL + tv.tv_usec;
}

/***************************************************************************/
/* Init */

int os_init(adv_conf* context) {
	memset(&OS,0,sizeof(OS));

	return 0;
}

void os_done(void) {
}

static void os_quit_signal(int signum) {
	OS.is_quit = 1;
}

int os_inner_init(const char* title) {
	const char* display;
	os_clock_t start, stop;
	struct utsname uts;
#ifdef USE_SDL
	SDL_version compiled;
#endif

	if (uname(&uts) != 0) {
		log_std(("ERROR: uname failed\n"));
	} else {
		log_std(("os: sys %s\n",uts.sysname));
		log_std(("os: release %s\n",uts.release));
		log_std(("os: version %s\n",uts.version));
		log_std(("os: machine %s\n",uts.machine));
	}

	/* print the compiler version */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define COMPILER_RESOLVE(a) #a
#define COMPILER(a,b,c) COMPILER_RESOLVE(a) "." COMPILER_RESOLVE(b) "." COMPILER_RESOLVE(c)
	log_std(("os: compiler GNU %s\n",COMPILER(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)));
#else
	log_std(("os: compiler unknown\n"));
#endif

	usleep(10000);
	start = os_clock();
	stop = os_clock();
	while (stop == start)
		stop = os_clock();
	log_std(("os: clock delta %ld\n",(unsigned long)(stop - start)));

	usleep(10000);
	start = os_clock();
	usleep(1000);
	stop = os_clock();
	log_std(("os: 0.001 delay, effective %g\n",(stop - start) / (double)OS_CLOCKS_PER_SEC ));

	log_std(("os: sysconf(_SC_CLK_TCK) %ld\n",sysconf(_SC_CLK_TCK)));
	log_std(("os: sysconf(_SC_NPROCESSORS_CONF) %ld\n",sysconf(_SC_NPROCESSORS_CONF)));
	log_std(("os: sysconf(_SC_NPROCESSORS_ONLN) %ld\n",sysconf(_SC_NPROCESSORS_ONLN)));
	log_std(("os: sysconf(_SC_PHYS_PAGES) %ld\n",sysconf(_SC_PHYS_PAGES)));
	log_std(("os: sysconf(_SC_AVPHYS_PAGES) %ld\n",sysconf(_SC_AVPHYS_PAGES)));
	log_std(("os: sysconf(_SC_CHAR_BIT) %ld\n",sysconf(_SC_CHAR_BIT)));
	log_std(("os: sysconf(_SC_LONG_BIT) %ld\n",sysconf(_SC_LONG_BIT)));
	log_std(("os: sysconf(_SC_WORD_BIT) %ld\n",sysconf(_SC_WORD_BIT)));

	display = getenv("DISPLAY");
	if (display)
		log_std(("os: DISPLAY=%s\n", display));
	else
		log_std(("os: DISPLAY undef\n"));

#if defined(USE_SVGALIB)
	OS.svgalib_active = 0;
	if (display == 0) {
		vga_disabledriverreport();
		log_std(("os: vga_init()\n"));
		if (vga_init() != 0) {
			log_std(("os: vga_init() failed\n"));
			target_err("Error initializing the SVGALIB video support.\n");
			return -1;
		}
		OS.svgalib_active = 1;
	} else {
		log_std(("os: vga_init() skipped\n"));
	}
#endif
#if defined(USE_SDL)
	log_std(("os: SDL_Init(SDL_INIT_NOPARACHUTE)\n"));
	if (SDL_Init(SDL_INIT_NOPARACHUTE) != 0) {
		log_std(("os: SDL_Init() failed, %s\n", SDL_GetError()));
		target_err("Error initializing the SDL video support.\n");
		return -1;
	} 
	OS.sdl_active = 1;
	SDL_VERSION(&compiled);

	log_std(("os: compiled with sdl %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch));
	log_std(("os: linked with sdl %d.%d.%d\n", SDL_Linked_Version()->major, SDL_Linked_Version()->minor, SDL_Linked_Version()->patch));
	if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
		log_std(("os: little endian system\n"));
	else
		log_std(("os: big endian system\n"));
#endif
#if defined(USE_SLANG)
	SLtt_get_terminfo();
	SLang_init_tty(-1, 0, 0);
	SLsmg_init_smg();
	OS.slang_active = 1;
#endif
#if defined(USE_X)
	OS.x_active = 0;
	if (display != 0) {
		int event_base, error_base;
		int major_version, minor_version;

		log_std(("os: XOpenDisplay()\n"));
		OS.dga_display = XOpenDisplay(0);
		if (!OS.dga_display) {
			log_std(("os: XOpenDisplay() failed\n"));
			target_err("Couldn't open X11 display.");
			return -1;
		}
		OS.x_active = 1;
	}
#endif
      
	/* set the titlebar */
	strcpy(OS.title, title);

	/* set some signal handlers */
	signal(SIGABRT, os_signal);
	signal(SIGFPE, os_signal);
	signal(SIGILL, os_signal);
	signal(SIGINT, os_signal);
	signal(SIGSEGV, os_signal);
	signal(SIGTERM, os_signal);
	signal(SIGHUP, os_signal);
	signal(SIGPIPE, os_signal);
	signal(SIGQUIT, os_quit_signal);

	return 0;
}

void* os_internal_svgalib_get(void) {
#if defined(USE_SVGALIB)
	if (OS.svgalib_active)
		return &OS.svgalib_active;
#endif
	return 0;
}

void* os_internal_slang_get(void) {
#if defined(USE_SLANG)
	if (OS.slang_active)
		return &OS.slang_active;
#endif
	return 0;
}

void* os_internal_x_get(void) {
#if defined(USE_X)
	if (OS.x_active)
		return OS.x_display;
#endif
	return 0;
}

void* os_internal_sdl_get(void) {
#if defined(USE_SDL)
	if (OS.sdl_active)
		return &OS.sdl_active;
#endif
	return 0;
}

void os_inner_done(void) {
#ifdef USE_X
	if (OS.x_display) {
		log_std(("os: XCloseDisplay()\n"));
		XCloseDisplay(OS.x_display);
		OS.x_active = 0;
	}
#endif
#ifdef USE_SLANG
	if (OS.slang_active) {
		SLsmg_reset_smg();
		SLang_reset_tty();
		OS.slang_active = 0;
	}
#endif
#ifdef USE_SDL
	if (OS.sdl_active) {
		log_std(("os: SDL_Quit()\n"));
		SDL_Quit();
		OS.sdl_active = 0;
	}
#endif
#ifdef USE_SVGALIB
	if (OS.svgalib_active) {
#ifdef USE_MOUSE_SVGALIB
		mouse_close(); /* always called */
#endif
		OS.svgalib_active = 0;
	}
#endif
}

const char* os_internal_title_get(void) {
	return OS.title;
}

void os_poll(void) {
#ifdef USE_SDL
	SDL_Event event;

	/* The event queue works only with the video initialized */
	if (!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	log_debug(("os: SDL_PollEvent()\n"));
	while (SDL_PollEvent(&event)) {
		log_debug(("os: SDL_PollEvent() -> event.type:%d\n",(int)event.type));
		switch (event.type) {
			case SDL_KEYDOWN :
#ifdef USE_KEYBOARD_SDL
				keyb_sdl_event_press(event.key.keysym.sym);
#endif
#ifdef USE_INPUT_SDL
				inputb_sdl_event_press(event.key.keysym.sym);
#endif

				/* toggle fullscreen check */
				if (event.key.keysym.sym == SDLK_RETURN
					&& (event.key.keysym.mod & KMOD_ALT) != 0) {
					if (SDL_WasInit(SDL_INIT_VIDEO) && SDL_GetVideoSurface()) {
						SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());

						if ((SDL_GetVideoSurface()->flags & SDL_FULLSCREEN) != 0) {
							SDL_ShowCursor(SDL_DISABLE);
						} else {
							SDL_ShowCursor(SDL_ENABLE);
						}
					}
				}
			break;
			case SDL_KEYUP :
#ifdef USE_KEYBOARD_SDL
				keyb_sdl_event_release(event.key.keysym.sym);
#endif
#ifdef USE_INPUT_SDL
				inputb_sdl_event_release(event.key.keysym.sym);
#endif
			break;
			case SDL_MOUSEMOTION :
#ifdef USE_MOUSE_SDL
				mouseb_sdl_event_move(event.motion.xrel, event.motion.yrel);
#endif
			break;
			case SDL_MOUSEBUTTONDOWN :
#ifdef USE_MOUSE_SDL
				if (event.button.button > 0)
					mouseb_sdl_event_press(event.button.button-1);
#endif
			break;
			case SDL_MOUSEBUTTONUP :
#ifdef USE_MOUSE_SDL
				if (event.button.button > 0)
					mouseb_sdl_event_release(event.button.button-1);
#endif
			break;
			case SDL_QUIT :
				OS.is_quit = 1;
				break;
		}
	}
#endif	
}

/***************************************************************************/
/* Led */

void os_led_set(unsigned mask)
{
	/* TODO drive the led */
}

/***************************************************************************/
/* Signal */

int os_is_quit(void) {
	return OS.is_quit;
}

void os_default_signal(int signum)
{
	log_std(("os: signal %d\n",signum));

#if defined(USE_KEYBOARD_SVGALIB) || defined(USE_KEYBOARD_SDL) || defined(USE_KEYBOARD_RAW)
	log_std(("os: keyb_abort\n"));
	{
		extern void keyb_abort(void);
		keyb_abort();
	}
#endif

#if defined(USE_VIDEO_SVGALIB) || defined(USE_VIDEO_FB) || defined(USE_VIDEO_X) || defined(USE_VIDEO_SDL)
	log_std(("os: video_abort\n"));
	{
		extern void video_abort(void);
		video_abort();
	}
#endif

#if defined(USE_SOUND_OSS) || defined(USE_SOUND_SDL)
	log_std(("os: sound_abort\n"));
	{
		extern void sound_abort(void);
		sound_abort();
	}
#endif

	target_mode_reset();

	log_std(("os: close log\n"));
	log_abort();

	target_signal(signum);
}

/***************************************************************************/
/* Main */

int main(int argc, char* argv[])
{
	if (target_init() != 0)
		return EXIT_FAILURE;

	if (file_init() != 0) {
		target_done();
		return EXIT_FAILURE;
	}

	if (os_main(argc,argv) != 0) {
		file_done();
		target_done();
		return EXIT_FAILURE;
	}

	file_done();
	target_done();
	
	return EXIT_SUCCESS;
}

