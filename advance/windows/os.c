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
#include "log.h"
#include "ksdl.h"
#include "isdl.h"
#include "msdl.h"
#include "target.h"
#include "file.h"
#include "ossdl.h"

#include "SDL.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

struct os_context {
	int is_quit; /**< Is termination requested. */
	char title[128]; /**< Title of the window. */
};

static struct os_context OS;

/***************************************************************************/
/* Clock */

os_clock_t OS_CLOCKS_PER_SEC = 1000;

os_clock_t os_clock(void) {
	return SDL_GetTicks();
}

/***************************************************************************/
/* Init */

int os_init(adv_conf* context) {
	memset(&OS,0,sizeof(OS));

	return 0;
}

void os_done(void) {
}

int os_inner_init(const char* title) {
	SDL_version compiled;
	os_clock_t start, stop;

	log_std(("os: sys SDL\n"));

	/* print the compiler version */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define COMPILER_RESOLVE(a) #a
#define COMPILER(a,b,c) COMPILER_RESOLVE(a) "." COMPILER_RESOLVE(b) "." COMPILER_RESOLVE(c)
	log_std(("os: compiler GNU %s\n",COMPILER(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)));
#else
	log_std(("os: compiler unknown\n"));
#endif
      
	log_std(("os: SDL_Init(SDL_INIT_NOPARACHUTE)\n"));
	if (SDL_Init(SDL_INIT_NOPARACHUTE) != 0) {
		log_std(("os: SDL_Init() failed, %s\n", SDL_GetError()));
		target_err("Error initializing the SDL video support.\n");
		return -1;
	}

	SDL_VERSION(&compiled);

	log_std(("os: compiled with sdl %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch));
	log_std(("os: linked with sdl %d.%d.%d\n", SDL_Linked_Version()->major, SDL_Linked_Version()->minor, SDL_Linked_Version()->patch));
	if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
		log_std(("os: little endian system\n"));
	else
		log_std(("os: big endian system\n"));


	start = os_clock();
	stop = os_clock();
	while (stop == start)
		stop = os_clock();
	log_std(("os: clock delta %ld\n",(unsigned long)(stop - start)));

	/* set the titlebar */
	strcpy(OS.title, title);

	/* set some signal handlers */
	signal(SIGABRT, os_signal);
	signal(SIGFPE, os_signal);
	signal(SIGILL, os_signal);
	signal(SIGINT, os_signal);
	signal(SIGSEGV, os_signal);
	signal(SIGTERM, os_signal);

	return 0;
}

void os_inner_done(void) {
	log_std(("os: SDL_Quit()\n"));
	SDL_Quit();
}

void os_poll(void) {
	SDL_Event event;

	/* The event queue works only with the video initialized */
	if (!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	while (SDL_PollEvent(&event)) {
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
}

const char* os_internal_title_get(void) {
	return OS.title;
}

/***************************************************************************/
/* Led */

void os_led_set(unsigned mask)
{
}

/***************************************************************************/
/* Signal */

int os_is_quit(void) {
	return OS.is_quit;
}

void os_default_signal(int signum)
{
	log_std(("os: signal %d\n", signum));

#if defined(USE_VIDEO_SDL)
	log_std(("os: video_abort\n"));
	{
		extern void video_abort(void);
		video_abort();
	}
#endif

#if defined(USE_SOUND_SDL)
	log_std(("os: sound_abort\n"));
	{
		extern void sound_abort(void);
		sound_abort();
	}
#endif

	SDL_Quit();

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

