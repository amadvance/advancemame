/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 2001 Andrea Mazzoleni
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>

struct os_context {
#ifdef USE_KEYBOARD_SVGALIB
	int key_id;
#endif

#ifdef USE_MOUSE_SVGALIB
	int mouse_id;
	int mouse_x;
	int mouse_y;
	int mouse_button_mask;
	unsigned mouse_button_mac;
	unsigned mouse_button_map[16];
#endif

#ifdef USE_JOYSTICK_SVGALIB
	int joystick_id;
	unsigned joystick_counter; /**< Number of joysticks active */
	char joystick_axe_name[32];
	char joystick_button_name[32];
	char joystick_stick_name[32];
#endif

#ifdef USE_INPUT_SVGALIB
	unsigned input_last;
#endif

	int is_term; /**< Is termination requested */
};

static struct os_context OS;

#define KEY_TYPE_NONE 0
#define KEY_TYPE_AUTO 1

#define MOUSE_TYPE_NONE 0
#define MOUSE_TYPE_AUTO 1

#define JOYSTICK_TYPE_NONE 0
#define JOYSTICK_TYPE_AUTO 1

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

#if defined(USE_VIDEO_SVGALIB) || defined(USE_KEYBOARD_SVGALIB) || defined(USE_MOUSE_SVGALIB) || defined(USE_JOYSTICK_SVGALIB) || defined(USE_INPUT_SVGALIB)
	vga_disabledriverreport();
	vga_init();
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

void os_inner_done(void) {
#ifdef USE_MOUSE_SVGALIB
	mouse_close(); /* always called */
#endif
}

void os_poll(void) {
#ifdef USE_KEYBOARD_SVGALIB
	if (OS.key_id != KEY_TYPE_NONE) {
		keyboard_update();
	}
#endif

#ifdef USE_MOUSE_SVGALIB
	if (OS.mouse_id != MOUSE_TYPE_NONE) {
		mouse_setposition(0,0);
		mouse_update();
		OS.mouse_x = mouse_getx();
		OS.mouse_y = mouse_gety();
		OS.mouse_button_mask = mouse_getbutton();
	}
#endif

#ifdef USE_JOYSTICK_SVGALIB
	if (OS.joystick_id != JOYSTICK_TYPE_NONE) {
		joystick_update();
	}
#endif
}

/***************************************************************************/
/* Keyboard */

#ifdef USE_KEYBOARD_SVGALIB

struct os_device OS_KEY[] = {
	{ "none", KEY_TYPE_NONE, "No keyboard" },
	{ "auto", KEY_TYPE_AUTO, "Automatic detection" },
	{ 0, 0, 0 }
};

int os_key_init(int key_id, int disable_special)
{
	OS.key_id = key_id;

	if (OS.key_id != KEY_TYPE_NONE) {
		if (keyboard_init() != 0) {
			log_std(("ERROR: keyboard_init() failed\n"));
			return -1;
		}
	}

	(void)disable_special; /* TODO disable special key sequence */

	return 0;
}

void os_key_done(void)
{
	if (OS.key_id != KEY_TYPE_NONE) {
		keyboard_close();
	}

	OS.key_id = KEY_TYPE_NONE;
}

unsigned os_key_get(unsigned code)
{
	if (OS.key_id != KEY_TYPE_NONE) {
		if (code == SCANCODE_BREAK || code == SCANCODE_BREAK_ALTERNATIVE) /* disable the pause key */
			return 0;
		else
			return keyboard_keypressed(code);
	} else {
		return 0;
	}
}

void os_key_all_get(unsigned char* code_map)
{
	if (OS.key_id != KEY_TYPE_NONE) {
		unsigned i;
		for(i=0;i<OS_KEY_MAX;++i)
			code_map[i] = keyboard_keypressed(i);
		code_map[SCANCODE_BREAK] = 0; /* disable the pause key */
		code_map[SCANCODE_BREAK_ALTERNATIVE] = 0;
	} else {
		unsigned i;
		for(i=0;i<OS_KEY_MAX;++i)
			code_map[i] = 0;
	}
}

void os_led_set(unsigned mask)
{
	/* TODO drive the led */
}

#endif

/***************************************************************************/
/* Input */

#ifdef USE_INPUT_SVGALIB

int os_input_init(void) {
	OS.input_last = 0;
	return 0;
}

void os_input_done(void) {
}

int os_input_hit(void) {
	if (OS.input_last != 0)
		return 1;
	OS.input_last = vga_getkey();
	return OS.input_last != 0;
}

unsigned os_input_get(void) {
	const unsigned max = 32;
	char map[max+1];
	unsigned mac;
	unsigned i;

	mac = 0;
	while (mac<max && (mac==0 || OS.input_last)) {

		if (OS.input_last) {
			map[mac] = OS.input_last;
			if (mac > 0 && map[mac] == 27) {
				break;
			}
			++mac;
			OS.input_last = 0;
		} else {
			target_idle();
		}

		OS.input_last = vga_getkey();
	}
	map[mac] = 0;

	if (strcmp(map,"\033[A")==0)
		return OS_INPUT_UP;
	if (strcmp(map,"\033[B")==0)
		return OS_INPUT_DOWN;
	if (strcmp(map,"\033[D")==0)
		return OS_INPUT_LEFT;
	if (strcmp(map,"\033[C")==0)
		return OS_INPUT_RIGHT;
	if (strcmp(map,"\033[1~")==0)
		return OS_INPUT_HOME;
	if (strcmp(map,"\033[4~")==0)
		return OS_INPUT_END;
	if (strcmp(map,"\033[5~")==0)
		return OS_INPUT_PGUP;
	if (strcmp(map,"\033[6~")==0)
		return OS_INPUT_PGDN;
	if (strcmp(map,"\033[[A")==0)
		return OS_INPUT_F1;
	if (strcmp(map,"\033[[B")==0)
		return OS_INPUT_F2;
	if (strcmp(map,"\033[[C")==0)
		return OS_INPUT_F3;
	if (strcmp(map,"\033[[D")==0)
		return OS_INPUT_F4;
	if (strcmp(map,"\033[[E")==0)
		return OS_INPUT_F5;
	if (strcmp(map,"\033[17~")==0)
		return OS_INPUT_F6;
	if (strcmp(map,"\033[18~")==0)
		return OS_INPUT_F7;
	if (strcmp(map,"\033[19~")==0)
		return OS_INPUT_F8;
	if (strcmp(map,"\033[20~")==0)
		return OS_INPUT_F9;
	if (strcmp(map,"\033[21~")==0)
		return OS_INPUT_F10;
	if (strcmp(map,"\r")==0 || strcmp(map,"\n")==0)
		return OS_INPUT_ENTER;
	if (strcmp(map,"\x7F")==0)
		return OS_INPUT_BACKSPACE;

	if (mac != 1)
		return 0;
	else
		return map[0];
}

#endif

/***************************************************************************/
/* Mouse */

#ifdef USE_MOUSE_SVGALIB

struct os_device OS_MOUSE[] = {
	{ "none", MOUSE_TYPE_NONE, "No mouse" },
	{ "auto", MOUSE_TYPE_AUTO, "Automatic detection" },
	{ 0, 0, 0 }
};

int os_mouse_init(int mouse_id)
{
	OS.mouse_id = mouse_id;

	if (OS.mouse_id != MOUSE_TYPE_NONE) {
		struct MouseCaps mouse_caps;
		unsigned i;
		unsigned buttons[] = {
			MOUSE_LEFTBUTTON,
			MOUSE_RIGHTBUTTON,
			MOUSE_MIDDLEBUTTON,
			MOUSE_FOURTHBUTTON,
			MOUSE_FIFTHBUTTON,
			MOUSE_SIXTHBUTTON,
			MOUSE_RESETBUTTON,
			0
		};

		/* opened internally at the svgalib */

		if (mouse_getcaps(&mouse_caps)!=0) {
			fprintf(stderr,"Failure: Error getting the mouse capabilities.\n");
			return -1;
		}

		mouse_setxrange(-32728,32727);
		mouse_setyrange(-32728,32727);
		mouse_setscale(1);
		mouse_setwrap(MOUSE_NOWRAP);

		OS.mouse_button_mac = 0;
		for(i=0;buttons[i];++i) {
			if ((mouse_caps.buttons & buttons[i]) != 0) {
				OS.mouse_button_map[OS.mouse_button_mac] = buttons[i];
				++OS.mouse_button_mac;
			}
		}
	}

	return 0;
}

void os_mouse_done(void) {
	/* closed internally at the svgalib */

	OS.mouse_id = MOUSE_TYPE_NONE;
}

unsigned os_mouse_count_get(void)
{
	if (OS.mouse_id != MOUSE_TYPE_NONE)
		return 1;
	else
		return 0;
}

unsigned os_mouse_button_count_get(unsigned mouse)
{
	assert( mouse < os_mouse_count_get() );
	return OS.mouse_button_mac;
}

void os_mouse_pos_get(unsigned mouse, int* x, int* y)
{
	assert( mouse < os_mouse_count_get() );
	*x = OS.mouse_x;
	*y = OS.mouse_y;
}

unsigned os_mouse_button_get(unsigned mouse, unsigned button)
{
	assert( mouse < os_mouse_count_get() );
	assert( button < os_mouse_button_count_get(0) );
	return (OS.mouse_button_mask & OS.mouse_button_map[button]) != 0;
}

#endif

/***************************************************************************/
/* Joystick */

#ifdef USE_JOYSTICK_SVGALIB

struct os_device OS_JOY[] = {
	{ "auto", JOYSTICK_TYPE_AUTO, "Automatic detection" },
	{ "none", JOYSTICK_TYPE_NONE, "No joystick" },
	{ 0, 0, 0 }
};

int os_joy_init(int joystick_id)
{
	OS.joystick_id = joystick_id;

	if (OS.joystick_id != JOYSTICK_TYPE_NONE) {
		unsigned i;
		for(i=0;i<4;++i) {
			if (joystick_init(i, NULL)<=0) {
				break;
			}
		}
		OS.joystick_counter = i;
	}

	return 0;
}

void os_joy_done(void)
{
	if (OS.joystick_id != JOYSTICK_TYPE_NONE) {
		unsigned i;
		for(i=0;i<OS.joystick_counter;++i)
			joystick_close(i);
	}

	OS.joystick_id = JOYSTICK_TYPE_NONE;
}

const char* os_joy_name_get(void) {
	return "Unknown";
}

const char* os_joy_driver_name_get(void) {
	return "Kernel";
}

unsigned os_joy_count_get(void) {
	return OS.joystick_counter;
}

unsigned os_joy_stick_count_get(unsigned j) {
	assert(j < os_joy_count_get() );
	return 1;
}

unsigned os_joy_stick_axe_count_get(unsigned j, unsigned s) {
	assert(j < os_joy_count_get() );
	assert(s < os_joy_stick_count_get(j) );
	(void)s;
	return joystick_getnumaxes(j);
}

unsigned os_joy_button_count_get(unsigned j) {
	assert(j < os_joy_count_get() );
	return joystick_getnumbuttons(j);
}

const char* os_joy_stick_name_get(unsigned j, unsigned s) {
	(void)j;
	sprintf(OS.joystick_stick_name,"S%d",s+1);
	return OS.joystick_stick_name;
}

const char* os_joy_stick_axe_name_get(unsigned j, unsigned s, unsigned a) {
	(void)j;
	(void)s;
	sprintf(OS.joystick_axe_name,"A%d",a+1);
	return OS.joystick_axe_name;
}

const char* os_joy_button_name_get(unsigned j, unsigned b) {
	(void)j;
	sprintf(OS.joystick_button_name,"B%d",b+1);
	return OS.joystick_button_name;
}

int os_joy_button_get(unsigned j, unsigned b) {
	assert(j < os_joy_count_get() );
	assert(b < os_joy_button_count_get(j) );
	return joystick_getbutton(j, b) != 0;
}

/**
 * Return the digital position of the axe.
 * \return 0 or 1
 */
int os_joy_stick_axe_digital_get(unsigned j, unsigned s, unsigned a, unsigned d) {
	int r;
	assert(j < os_joy_count_get() );
	assert(s < os_joy_stick_count_get(j) );
	assert(a < os_joy_stick_axe_count_get(j,s) );
	r = joystick_getaxis(j,a);
	if (d)
		return r < -64;
	else
		return r > 64;
}

/**
 * Return the analog position of the axe.
 * \return From -128 to 128
 */
int os_joy_stick_axe_analog_get(unsigned j, unsigned s, unsigned a) {
	int r;
	assert(j < os_joy_count_get() );
	assert(s < os_joy_stick_count_get(j) );
	assert(a < os_joy_stick_axe_count_get(j,s) );
	r = joystick_getaxis(j,a);
	if (r > 64) /* adjust the upper limit from 127 to 128 */
		++r;
	return r;
}

void os_joy_calib_start(void)
{
	/* no calibration */
}

const char* os_joy_calib_next(void)
{
	/* no calibration */
	return 0;
}

#endif

/***************************************************************************/
/* Signal */

int os_is_term(void) {
	return OS.is_term;
}

static void os_backtrace(void) {
	void* buffer[256];
	char** symbols;
	int size;
	int i;
	size = backtrace(buffer,256);
	symbols = backtrace_symbols(buffer,size);
	printf("Stack backtrace:\n");
	for(i=0;i<size;++i)
		printf("%s\n", symbols[i]);
	free(symbols);
}

void os_default_signal(int signum)
{
	log_std(("os: signal %d\n",signum));

#if defined(USE_VIDEO_SVGALIB) || defined(USE_VIDEO_FB)
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

