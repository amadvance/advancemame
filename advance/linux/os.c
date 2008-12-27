/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

#include "os.h"
#include "oslinux.h"
#include "log.h"
#include "target.h"
#include "file.h"
#include "snstring.h"
#include "measure.h"

#if defined(USE_SDL)
#include "ossdl.h"
#include "SDL.h"
#include "ksdl.h"
#include "isdl.h"
#include "msdl.h"
#endif

#if defined(USE_SVGALIB)
#include <vga.h>
#include <vgamouse.h>
#endif

#if defined(USE_SLANG)
#if HAVE_SLANG_H
#include <slang.h>
#else
#if HAVE_SLANG_SLANG_H
#include <slang/slang.h>
#else
#error slang.h file not found!
#endif
#endif
#endif

#if defined(USE_CURSES)
#include <curses.h>
#endif

#if defined(USE_X)
#include <X11/Xlib.h>
#endif

#if HAVE_TERMIOS_H
#include <termios.h>
#endif
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include <setjmp.h>

#ifdef USE_SMP
#include <pthread.h>
#endif

struct os_context {
#ifdef USE_SVGALIB
	adv_bool svgalib_active; /**< SVGALIB initialized. */
#endif

#ifdef USE_SLANG
	adv_bool slang_active; /**< Slang initialized. */
#endif

#ifdef USE_CURSES
	adv_bool curses_active; /**< Curses initialized. */
#endif

#ifdef USE_X
	adv_bool x_active; /**< X initialized. */
	Display* x_display; /**< X display. */
#endif

#ifdef USE_SDL
	adv_bool sdl_active; /**< SDL initialized. */
#endif

	int is_quit; /**< Is termination requested. */
	char title_buffer[128]; /**< Title of the window. */

	struct termios term; /**< Term state. */
	adv_bool term_active;
};

static struct os_context OS;
static sigjmp_buf OS_HUP; /**< Restart point of the program after a SIGHUP. */
static int OS_SIGNAL; /**< Prevent recursive signal. */

/***************************************************************************/
/* Init */

int os_init(adv_conf* context)
{
	memset(&OS, 0, sizeof(OS));

	return 0;
}

void os_done(void)
{
}

/***************************************************************************/
/* Signal */

static void os_restore(void)
{
#if defined(USE_KEYBOARD_SVGALIB) || defined(USE_KEYBOARD_SDL) || defined(USE_KEYBOARD_RAW) || defined(USE_KEYBOARD_EVENT)
	log_std(("os: keyb_abort\n"));
	{
		extern void keyb_abort(void);
		keyb_abort();
	}
#endif

#if defined(USE_MOUSE_SVGALIB) || defined(USE_MOUSE_SDL) || defined(USE_MOUSE_RAW) || defined(USE_MOUSE_EVENT)
	log_std(("os: mouseb_abort\n"));
	{
		extern void mouseb_abort(void);
		mouseb_abort();
	}
#endif

#if defined(USE_VIDEO_SVGALIB) || defined(USE_VIDEO_FB) || defined(USE_VIDEO_X) || defined(USE_VIDEO_SDL)
	log_std(("os: adv_video_abort\n"));
	{
		extern void adv_video_abort(void);
		adv_video_abort();
	}
#endif

#if defined(USE_SOUND_OSS) || defined(USE_SOUND_SDL) || defined(USE_SOUND_ALSA)
	log_std(("os: sound_abort\n"));
	{
		extern void soundb_abort(void);
		soundb_abort();
	}
#endif

	target_mode_reset();

	if (OS.term_active) {
		if (tcsetattr(fileno(stdin), TCSAFLUSH, &OS.term) != 0) {
			/* ignore error */
			log_std(("os: tcsetattr(TCSAFLUSH) failed\n"));
		}
	}

	log_std(("os: close log\n"));
	log_abort();
}


static void os_quit_signal(int signum)
{
	OS.is_quit = 1;
}

static void os_hup_signal(int signum)
{
	os_restore();

	/* restart the program */
	siglongjmp(OS_HUP, 1);
}

void os_default_signal(int signum, void* info, void* context)
{
	if (OS_SIGNAL) {
		/* detect recursive signals, for example a SIGSEGV while processig SIGTERM */
		fprintf(stderr, "Double signals, %d and %d\n", OS_SIGNAL, signum);

		target_signal(signum, info, context);
	}
	OS_SIGNAL = signum;

	log_std(("os: signal %d\n", signum));

	os_restore();

	target_signal(signum, info, context);
}

/***************************************************************************/
/* Inner */

int os_is_quit(void)
{
	return OS.is_quit;
}

static void os_wait(void)
{
	struct timespec req;
	req.tv_sec = 0;
	req.tv_nsec = 1 * 1000000; /* 1 ms */
	nanosleep(&req, 0);
}

static void os_delay(void)
{
	double delay_time;
	target_clock_t start, stop;

	target_yield();

	delay_time = adv_measure_step(os_wait, 0.0001, 0.2, 7);

	if (delay_time > 0) {
		log_std(("os: sleep granularity %g\n", delay_time));
		target_usleep_granularity(delay_time * 1000000);
	} else {
		log_std(("ERROR:os: sleep granularity NOT measured\n"));
		target_usleep_granularity(20000 /* 20 ms */);
	}

	target_yield();

	start = target_clock();
	stop = target_clock();
	while (stop == start)
		stop = target_clock();

	log_std(("os: clock granularity %g\n", (stop - start) / (double)TARGET_CLOCKS_PER_SEC));
}

#ifdef USE_SMP

static pid_t os_thread_id;

static void* os_thread_function(void* arg)
{
	os_thread_id = getpid();
	return 0;
}

static int os_thread(void)
{
#ifdef NDEBUG /* disable on debugging */
	pthread_t pid;
	pid_t process_id;
 
	if (pthread_create(&pid, 0, os_thread_function, 0)) {
		log_std(("ERROR:os: error calling pthread_create()\n"));
		return -1;
	}
	if (pthread_join(pid, 0)) {
		log_std(("ERROR:os: error calling pthread_join()\n"));
		return -1;
	}

	process_id = getpid();

	if (process_id == os_thread_id) {
		log_std(("os: thread_id is equal at process_id. Probably NPTL threading.\n"));
	} else {
		log_std(("os: thread_id is different than process_id. Probably LinuxThread threading.\n"));
	}
#endif

	return 0;
}
#endif

int os_inner_init(const char* title)
{
	const char* display;
	struct utsname uts;
	struct sigaction term_action;
	struct sigaction quit_action;
	struct sigaction hup_action;
	struct sigaction pipe_action;
#ifdef USE_SDL
	SDL_version compiled;
#endif
	unsigned char endian[4] = { 0x1, 0x2, 0x3, 0x4 };
	uint32 endian_little = 0x04030201;
	uint32 endian_big = 0x01020304;

	log_std(("os: os_inner_init\n"));

	if (uname(&uts) != 0) {
		log_std(("ERROR:os: uname failed\n"));
	} else {
		log_std(("os: sys %s\n", uts.sysname));
		log_std(("os: release %s\n", uts.release));
		log_std(("os: version %s\n", uts.version));
		log_std(("os: machine %s\n", uts.machine));
	}

#if HAVE_SYSCONF
#ifdef _SC_CLK_TCK
	log_std(("os: sysconf(_SC_CLK_TCK) %ld\n", sysconf(_SC_CLK_TCK)));
#endif
#ifdef _SC_NPROCESSORS_CONF
	log_std(("os: sysconf(_SC_NPROCESSORS_CONF) %ld\n", sysconf(_SC_NPROCESSORS_CONF)));
#endif
#ifdef _SC_NPROCESSORS_ONLN
	log_std(("os: sysconf(_SC_NPROCESSORS_ONLN) %ld\n", sysconf(_SC_NPROCESSORS_ONLN)));
#endif
#ifdef _SC_PHYS_PAGES
	log_std(("os: sysconf(_SC_PHYS_PAGES) %ld\n", sysconf(_SC_PHYS_PAGES)));
#endif
#ifdef _SC_AVPHYS_PAGES
	log_std(("os: sysconf(_SC_AVPHYS_PAGES) %ld\n", sysconf(_SC_AVPHYS_PAGES)));
#endif
#ifdef _SC_CHAR_BIT
	log_std(("os: sysconf(_SC_CHAR_BIT) %ld\n", sysconf(_SC_CHAR_BIT)));
#endif
#ifdef _SC_LONG_BIT
	log_std(("os: sysconf(_SC_LONG_BIT) %ld\n", sysconf(_SC_LONG_BIT)));
#endif
#ifdef _SC_WORD_BIT
	log_std(("os: sysconf(_SC_WORD_BIT) %ld\n", sysconf(_SC_WORD_BIT)));
#endif
#endif

#ifdef _POSIX_PRIORITY_SCHEDULING /* OSDEF Check for POSIX scheduling */
	log_std(("os: scheduling available\n"));
#else
	log_std(("os: scheduling NOT available\n"));
#endif

	/* print the compiler version */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__) /* OSDEF Detect compiler version */
#define COMPILER_RESOLVE(a) #a
#define COMPILER(a, b, c) COMPILER_RESOLVE(a) "." COMPILER_RESOLVE(b) "." COMPILER_RESOLVE(c)
	log_std(("os: compiler GNU %s\n", COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)));
#else
	log_std(("os: compiler unknown\n"));
#endif

	/* check for int size */
	if (sizeof(uint8) != 1) {
		target_err("The program is compiled with invalid uint8 type.\n");
		return -1;
	}
	if (sizeof(uint16) != 2) {
		target_err("The program is compiled with invalid uint16 type.\n");
		return -1;
	}
	if (sizeof(uint32) != 4) {
		target_err("The program is compiled with invalid uint32 type.\n");
		return -1;
	}
	if (sizeof(uint64) != 8) {
		target_err("The program is compiled with invalid uint64 type.\n");
		return -1;
	}

	/* check for the endianess */
#ifdef USE_MSB
	log_std(("os: compiled big endian system\n"));
	if (memcmp(endian, &endian_big, 4) != 0) {
		target_err("The program is compiled as bigendian but system doesn't appear to be bigendian.\n");
		return -1;
	}
#endif
#ifdef USE_LSB
	log_std(("os: compiled little endian system\n"));
	if (memcmp(endian, &endian_little, 4) != 0) {
		target_err("The program is compiled as littleendian but system doesn't appear to be littleendian.\n");
		return -1;
	}
#endif

#ifdef USE_SMP
	/* check the thread support */
	if (os_thread() != 0) {
		target_err("Error on the threading support.\n");
		return -1;
	}
#endif

	/* get DISPLAY environment variable */
	display = getenv("DISPLAY");
	if (display)
		log_std(("os: DISPLAY=%s\n", display));
	else
		log_std(("os: DISPLAY undef\n"));

	/* probe the delay system */
	os_delay();

	/* save term if possible */
	if (tcgetattr(fileno(stdin), &OS.term) != 0) {
		log_std(("ERROR:os: error getting the tty state.\n"));
		OS.term_active = 0;
	} else {
		OS.term_active = 1;
	}

#if defined(USE_X)
	OS.x_active = 0;
	{
		int event_base, error_base;
		int major_version, minor_version;

		log_std(("os: XOpenDisplay()\n"));
		OS.dga_display = XOpenDisplay(0);
		if (OS.dga_display) {
			OS.x_active = 1;
		} else {
			log_std(("WARNING:os: XOpenDisplay() failed. All the X drivers will be disabled.\n"));
		}
	}
#endif
#if defined(USE_SVGALIB)
	OS.svgalib_active = 0;
	if (!os_internal_wm_active()) {
		int h;
		log_std(("os: open /dev/svga\n"));

		/* try opening the device, otherwise vga_init() will abort the program. */
		h = open("/dev/svga", O_RDWR);
		if (h >= 0) {
			int res;
			close(h);

			vga_disabledriverreport();

			/* check the version of the SVGALIB */
			res = vga_setmode(-1);
			if (res < 0 || res < 0x1911) { /* 1.9.11 */
				log_std(("WARNING:os: invalid SVGALIB version %x. All the SVGALIB drivers will be disabled.\n", (int)res));
				/* don't print the message. It may be a normal condition. */
				/* target_nfo("Invalid SVGALIB version, you need SVGALIB version 1.9.x or 2.0.x.\nPlease upgrade or recompile without SVGALIB support.\n"); */
			} else {
				log_std(("os: vga_init()\n"));
				if (vga_init() != 0) {
					log_std(("os: vga_init() failed\n"));
					target_err("Error initializing the SVGALIB video support.\n");
					return -1;
				}
				OS.svgalib_active = 1;
			}
		} else {
			log_std(("WARNING:os: open /dev/svga failed. All the SVGALIB drivers will be disabled.\n"));
			/* don't print the message. It may be a normal condition. */
			/* target_nfo("Error opening the SVGALIB device /dev/svga.\n"); */
		}
	} else {
		log_std(("WARNING:os: vga_init() skipped because X is active. All the SVGALIB drivers will be disabled.\n"));
		/* don't print the message. It may be a normal condition. */
		/* target_nfo("SVGALIB not initialized because it's unusable in X.\n"); */
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
#ifdef USE_MSB
	if (SDL_BYTEORDER != SDL_BIG_ENDIAN) {
		target_err("Invalid SDL endianess.\n");
		return -1;
	}
#endif
#ifdef USE_LSB
	if (SDL_BYTEORDER != SDL_LIL_ENDIAN) {
		target_err("Invalid SDL endianess.\n");
		return -1;
	}
#endif
#endif
#if defined(USE_SLANG)
	OS.slang_active = 0;
	if (!os_internal_wm_active()) {
		log_std(("os: SLtt_get_terminfo()\n"));
		SLtt_get_terminfo();
		log_std(("os: SLsmg_init_smg()\n"));
		SLsmg_init_smg();
		OS.slang_active = 1;
	} else {
		log_std(("WARNING:os: SLang_init_tty() skipped because X is active. All the SLang drivers will be disabled.\n"));
	}
#endif
#if defined(USE_CURSES)
	OS.curses_active = 0;
	if (!os_internal_wm_active()) {
		log_std(("os: initscr()\n"));
		initscr();
		start_color();
		cbreak();
		noecho();
		nonl();
		OS.curses_active = 1;
	} else {
		log_std(("WARNING:os: curses initscr() skipped because X is active. All the curses drivers will be disabled.\n"));
	}
#endif

	/* set the titlebar */
	sncpy(OS.title_buffer, sizeof(OS.title_buffer), title);

	/* set some signal handlers */

	/* STANDARD signals */
	term_action.sa_handler = (void (*)(int))os_signal;
	/* block external generated signals in the signal handler */
	sigemptyset(&term_action.sa_mask);
	sigaddset(&term_action.sa_mask, SIGALRM);
	sigaddset(&term_action.sa_mask, SIGINT);
	sigaddset(&term_action.sa_mask, SIGTERM);
	sigaddset(&term_action.sa_mask, SIGHUP);
	sigaddset(&term_action.sa_mask, SIGQUIT);
	term_action.sa_flags = SA_RESTART | SA_SIGINFO;
	/* external generated */
	sigaction(SIGALRM, &term_action, 0);
	sigaction(SIGINT, &term_action, 0);
	sigaction(SIGTERM, &term_action, 0);
	/* internal generated */
	sigaction(SIGABRT, &term_action, 0);
	sigaction(SIGFPE, &term_action, 0);
	sigaction(SIGILL, &term_action, 0);
	sigaction(SIGSEGV, &term_action, 0);
	sigaction(SIGBUS, &term_action, 0);

	/* HUP signal */
	hup_action.sa_handler = os_hup_signal;
	sigemptyset(&hup_action.sa_mask);
	hup_action.sa_flags = SA_RESTART;
	sigaction(SIGHUP, &hup_action, 0);

	/* QUIT signal */
	quit_action.sa_handler = os_quit_signal;
	sigemptyset(&quit_action.sa_mask);
	quit_action.sa_flags = SA_RESTART;
	sigaction(SIGQUIT, &quit_action, 0);

	/* PIPE signal, ignoring it force some functions to */
	/* return with error. It happen for example on the LCD sockets. */
	pipe_action.sa_handler = SIG_IGN;
	sigemptyset(&pipe_action.sa_mask);
	pipe_action.sa_flags = SA_RESTART;
	sigaction(SIGPIPE, &pipe_action, 0);

	return 0;
}

#if defined(USE_SVGALIB)
void* os_internal_svgalib_get(void)
{
	if (OS.svgalib_active)
		return &OS.svgalib_active;
	return 0;
}
#endif

#if defined(USE_SLANG)
void* os_internal_slang_get(void)
{
	if (OS.slang_active)
		return &OS.slang_active;
	return 0;
}
#endif

#if defined(USE_CURSES)
void* os_internal_curses_get(void)
{
	if (OS.curses_active)
		return &OS.curses_active;
	return 0;
}
#endif

#if defined(USE_X)
void* os_internal_x_get(void)
{
	if (OS.x_active)
		return OS.x_display;
	return 0;
}
#endif

#if defined(USE_SDL)
const char* os_internal_sdl_title_get(void)
{
	return OS.title_buffer;
}

void* os_internal_sdl_get(void)
{
	if (OS.sdl_active)
		return &OS.sdl_active;
	return 0;
}
#endif

adv_bool os_internal_wm_active(void)
{
#ifdef USE_X
	if (OS.x_active) {
		return 1;
	}
#else
	if (getenv("DISPLAY") != 0) {
		return 1;
	}
#endif
	return 0;
}

void os_inner_done(void)
{
	log_std(("os: os_inner_done\n"));

#ifdef USE_X
	if (OS.x_display) {
		log_std(("os: XCloseDisplay()\n"));
		XCloseDisplay(OS.x_display);
		OS.x_active = 0;
	}
#endif
#ifdef USE_CURSES
	if (OS.curses_active) {
		log_std(("os: endwin()\n"));
		endwin();
		OS.curses_active = 0;
	}
#endif
#ifdef USE_SLANG
	if (OS.slang_active) {
		log_std(("os: SLsmg_reset_smg()\n"));
		SLsmg_reset_smg();
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
		mouse_close(); /* always called */
		OS.svgalib_active = 0;
	}
#endif

	/* restore term */
	if (OS.term_active) {
		log_std(("os: tcsetattr(%sICANON %sECHO)\n", (OS.term.c_lflag & ICANON) ? "" : "~", (OS.term.c_lflag & ECHO) ? "" : "~"));
		if (tcsetattr(fileno(stdin), TCSAFLUSH, &OS.term) != 0) {
			/* ignore error */
			log_std(("os: tcsetattr(TCSAFLUSH) failed\n"));
		}
	}
}

void os_poll(void)
{
#ifdef USE_SDL
	SDL_Event event;

	/* The event queue works only with the video initialized */
	if (!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	log_debug(("os: SDL_PollEvent()\n"));
	while (SDL_PollEvent(&event)) {
		log_debug(("os: SDL_PollEvent() -> event.type:%d\n", (int)event.type));
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

void os_fire(void)
{
}

/***************************************************************************/
/* Main */

int main(int argc, char* argv[])
{
	char** copyv;
	int i;

	/* duplicate the arguments */
	copyv = alloca((argc+1) * sizeof(char*));
	for(i=0;i<argc;++i) {
		copyv[i] = alloca(strlen(argv[i]) + 1);
		strcpy(copyv[i], argv[i]);
	}
	copyv[i] = 0;

	/* set the entry point to restart the program */
	if (sigsetjmp(OS_HUP, 1) != 0) {

		/* restart the program */
		execv(copyv[0], copyv);

		/* abort if fail */
		abort();
	}

	if (target_init() != 0)
		return EXIT_FAILURE;

	if (file_init() != 0) {
		target_done();
		return EXIT_FAILURE;
	}

	if (os_main(argc, argv) != 0) {
		file_done();
		target_done();
		return EXIT_FAILURE;
	}

	file_done();
	target_done();

	return EXIT_SUCCESS;
}

