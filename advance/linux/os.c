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
#include "osint.h"
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
#include <execinfo.h>

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
#if defined(USE_VIDEO_DGA) || defined(USE_KEYBOARD_DGA) || defined(USE_MOUSE_DGA)
#define USE_DGA
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#endif

struct os_context {
#ifdef USE_SVGALIB
	int svgalib_active; /**< SVGALIB initialized. */
#endif

#ifdef USE_SLANG
	int slang_active; /**< Slang initialized. */
#endif

#ifdef USE_DGA
	int dga_active; /**< DGA initialized. */
	Display* dga_display; /**< DGA Display. */
#endif

	int is_term; /**< Is termination requested. */
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

int os_init(struct conf_context* context) {
	memset(&OS,0,sizeof(OS));

	return 0;
}

void os_done(void) {
}

static void os_term_signal(int signum) {
	OS.is_term = 1;
}

int os_inner_init(const char* title) {
	const char* display;
	os_clock_t start, stop;
	struct utsname uts;

	if (uname(&uts) != 0) {
		log_std(("ERROR: uname failed\n"));
	} else {
		log_std(("os: sys %s\n",uts.sysname));
		log_std(("os: release %s\n",uts.release));
		log_std(("os: version %s\n",uts.version));
		log_std(("os: machine %s\n",uts.machine));
	}

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
		target_err("The SVGALIB video isn't supported in X Window.\n");
	}
#endif
#if defined(USE_SLANG)
	SLtt_get_terminfo();
	SLang_init_tty(-1, 0, 0);
	SLsmg_init_smg();
	OS.slang_active = 1;
#endif
#if defined(USE_DGA)
	OS.dga_active = 0;
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
		/* check for the DGA extension */
		log_std(("video:dga: XDGAQueryExtension()\n"));
		if (!XDGAQueryExtension(OS.dga_display, &event_base, &error_base)) {
			log_std(("video:dga: XDGAQueryExtension() failed\n"));
			XCloseDisplay(OS.dga_display);
			target_err("DGA extensions not available");
			return -1;
		}
		log_std(("video:dga: XDGAQueryExtension() event_base:%d error_base:%d\n", event_base, error_base));
		log_std(("video:dga: XDGAQueryVersion()\n"));
		if (!XDGAQueryVersion(OS.dga_display, &major_version, &minor_version)) {
			log_std(("video:dga: XDGAQueryVersion() failed\n"));
			XCloseDisplay(OS.dga_display);
			target_err("DGA version not available");
			return -1;
		}
		log_std(("video:dga: XDGAQueryVersion() major_version:%d, minor_version:%d\n", major_version, minor_version));
		if (major_version < 2) {
			XCloseDisplay(OS.dga_display);
			target_err("DGA driver requires DGA 2.0 or newer");
			return -1;
		}
		OS.dga_active = 1;
	}
#endif

	/* set some signal handlers */
	signal(SIGABRT, os_signal);
	signal(SIGFPE, os_signal);
	signal(SIGILL, os_signal);
	signal(SIGINT, os_signal);
	signal(SIGSEGV, os_signal);
	signal(SIGTERM, os_term_signal);
	signal(SIGHUP, os_signal);
	signal(SIGPIPE, os_signal);
	signal(SIGQUIT, os_term_signal);
	signal(SIGUSR1, os_signal); /* used for malloc failure */

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

void* os_internal_dga_get(void) {
#if defined(USE_DGA)
	if (OS.dga_active)
		return OS.dga_display;
#endif
	return 0;
}

void os_inner_done(void) {
#ifdef USE_MOUSE_SVGALIB
	if (OS.svgalib_active) {
		mouse_close(); /* always called */
	}
#endif
#ifdef USE_SVGALIB
	OS.svgalib_active = 0;
#endif
#ifdef USE_SLANG
	OS.slang_active = 0;
	SLsmg_reset_smg();
	SLang_reset_tty();
#endif
#ifdef USE_DGA
	/* close up the display */
	if (OS.dga_display) {
		log_std(("os: XCloseDisplay()\n"));
		XCloseDisplay(OS.dga_display);
		OS.dga_active = 0;
	}
#endif
}

void os_poll(void) {
}

/***************************************************************************/
/* Led */

void os_led_set(unsigned mask)
{
	/* TODO drive the led */
}

/***************************************************************************/
/* Signal */

int os_is_term(void) {
	return OS.is_term;
}

/**
 * Print the stack backtrace.
 * The programm need to be compiled without CFLAGS=-fomit-frame-pointer and with
 * LDFLAGS=-rdynamic
 */
static void os_backtrace(void) {
	void* buffer[256];
	char** symbols;
	int size;
	int i;
	size = backtrace(buffer,256);
	symbols = backtrace_symbols(buffer,size);

	if (size > 1) {
		printf("Stack backtrace:\n");
		for(i=0;i<size;++i)
			printf("%s\n", symbols[i]);
	} else {
		printf("No stack backtrace: compile without CFLAGS=-fomit-frame-pointer and with LDFLAGS=-rdynamic\n");
	}

	free(symbols);
}

void os_default_signal(int signum)
{
	log_std(("os: signal %d\n",signum));

#if defined(USE_KEYBOARD_SVGALIB)
	log_std(("os: keyb_abort\n"));
	{
		extern void keyb_abort(void);
		keyb_abort();
	}
#endif

#if defined(USE_VIDEO_SVGALIB) || defined(USE_VIDEO_FB) || defined(USE_VIDEO_DGA)
	log_std(("os: video_abort\n"));
	{
		extern void video_abort(void);
		video_abort();
	}
#endif

#if defined(USE_SOUND_OSS)
	log_std(("os: sound_abort\n"));
	{
		extern void sound_abort(void);
		sound_abort();
	}
#endif

	target_mode_reset();

	log_std(("os: close log\n"));
	log_abort();

	if (signum == SIGINT) {
		fprintf(stderr,"Break pressed\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGQUIT) {
		fprintf(stderr,"Quit pressed\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGUSR1) {
		fprintf(stderr,"Low memory\n\r");
		_exit(EXIT_FAILURE);
	} else {
		fprintf(stderr,"AdvanceMAME signal %d.\n",signum);
		fprintf(stderr,"%s, %s\n\r", __DATE__, __TIME__);

		os_backtrace();

		if (signum == SIGILL) {
			fprintf(stderr,"Are you using the correct binary ?\n");
		}

		_exit(EXIT_FAILURE);
	}
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

