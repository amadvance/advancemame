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

#ifndef PREFIX
#error Macro PREFIX undefined
#endif

struct os_fixed {
	char root_dir[OS_MAXPATH];
	char home_dir[OS_MAXPATH];
	char dir_buffer[OS_MAXPATH];
	char file_root[OS_MAXPATH];
	char file_home[OS_MAXPATH];
};

#define MOUSE_BUTTON_MAX 3 /**< Max number os mouse buttons */
#define KEY_MAX SDLK_LAST /**< Max number of keys */
#define JOYSTICK_MAX 4 /**< Max number of joysticks */

struct os_context {
	FILE* msg;
	int msg_sync_flag;

	int key_id; /**< Keyboard identifier */
	int key_map[KEY_MAX];

	int mouse_id; /**< Mouse identifier */
	int mouse_button_map[3];
	int mouse_x;
	int mouse_y;

	int joystick_id; /**< Joystick identifier */
	unsigned joystick_counter; /**< Number of joysticks active */
	SDL_Joystick* joystick_map[JOYSTICK_MAX];
	char joystick_axe_name[32];
	char joystick_button_name[32];
	char joystick_stick_name[32];

	int is_term; /**< Is termination requested */
};

static struct os_fixed OSF;
static struct os_context OS;

#define KEY_TYPE_NONE 0
#define KEY_TYPE_AUTO 1

#define MOUSE_TYPE_NONE 0
#define MOUSE_TYPE_AUTO 1

#define JOYSTICK_TYPE_NONE 0
#define JOYSTICK_TYPE_AUTO 1

/***************************************************************************/
/* Debug */

void os_msg_va(const char *text, va_list arg) {
	if (OS.msg) {
		vfprintf(OS.msg,text,arg);
		if (OS.msg_sync_flag)
			fflush(OS.msg);
	}
}

void os_msg(const char *text, ...) {
	va_list arg;
	va_start(arg, text);
	os_msg_va(text, arg);
	va_end(arg);
}

int os_msg_init(const char* file, int sync_flag) {
	OS.msg_sync_flag = sync_flag;

	if (file) {
		OS.msg = fopen(file,"w");
		if (!OS.msg)
			return -1;
	}

	return 0;
}

void os_msg_done(void) {
	if (OS.msg) {
		fclose(OS.msg);
		OS.msg = 0;
	}
}

/***************************************************************************/
/* Clock */

os_clock_t OS_CLOCKS_PER_SEC = 1000;

os_clock_t os_clock(void) {
	return SDL_GetTicks();
}

/***************************************************************************/
/* Init */

static void strcatslash(char* str) {
	if (str[0] && str[strlen(str)-1] !='/')
		strcat(str,"/");
}

static int os_fixed(void) {
	char* home;

	/* root */
	strcpy(OSF.root_dir,PREFIX);
	strcatslash(OSF.root_dir);
	strcat(OSF.root_dir,"share/advance");

	/* home */
	home = getenv("HOME");
	if (!home || !*home) {
		/* use the root dir as home dir */
		strcpy(OSF.home_dir,OSF.root_dir);
	} else {
		strcpy(OSF.home_dir,home);
		strcatslash(OSF.home_dir);
		strcat(OSF.home_dir,".advance");
	}

	if (OSF.home_dir[0]) {
		struct stat st;
		if (stat(OSF.home_dir,&st) == 0) {
			if (!S_ISDIR(st.st_mode)) {
				fprintf(stderr,"Failure: A file named %s exists\n",OSF.home_dir);
				return -1;
			}
		} else {
			char buffer[OS_MAXPATH];
			if (mkdir(OSF.home_dir,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
				fprintf(stderr,"Failure: Error creating the directory %s\n",OSF.home_dir);
				return -1;
			}
			sprintf(buffer,"%s/cfg",OSF.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/snap",OSF.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/hi",OSF.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/nvram",OSF.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/memcard",OSF.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			sprintf(buffer,"%s/sta",OSF.home_dir);
			mkdir(buffer,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		}
	}

	return 0;
}

int os_init(struct conf_context* context) {
	memset(&OS,0,sizeof(OS));

	return 0;
}

void os_done(void) {
}

static void os_term_signal(int signum) {
	OS.is_term = 1;
}

int os_inner_init(void) {
	SDL_version compiled;

	os_log(("os: root dir %s\n", OSF.root_dir));
	os_log(("os: home dir %s\n", OSF.home_dir));

	/* the SDL_INIT_VIDEO flags must be specified also if the video */
	/* output isn't used */
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) != 0) {
		os_log(("sound: SDL_Init() failed, %s\n", SDL_GetError()));
		return -1;
	}

	SDL_VERSION(&compiled);

	os_log(("os: compiled with sdl %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch));
	os_log(("os: linked with sdl %d.%d.%d\n", SDL_Linked_Version()->major, SDL_Linked_Version()->minor, SDL_Linked_Version()->patch));
	if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
		os_log(("os: little endian system\n"));
	else
		os_log(("os: big endian system\n"));

	/* set the titlebar */
	SDL_WM_SetCaption("Advance",0);

	/* set some signal handlers */
	signal(SIGABRT, os_signal);
	signal(SIGFPE, os_signal);
	signal(SIGILL, os_signal);
	signal(SIGINT, os_signal);
	signal(SIGSEGV, os_signal);
	signal(SIGTERM, os_term_signal);
	signal(SIGHUP, os_signal);
	signal(SIGKILL, os_signal);
	signal(SIGPIPE, os_signal);
	signal(SIGQUIT, os_term_signal);
	signal(SIGUSR1, os_signal); /* used for malloc failure */

	return 0;
}

void os_inner_done(void) {
	SDL_Quit();
}

void os_poll(void) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN :
				if (event.key.keysym.sym < KEY_MAX)
					OS.key_map[event.key.keysym.sym] = 1;

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
				if (event.key.keysym.sym < KEY_MAX)
					OS.key_map[event.key.keysym.sym] = 0;
			break;
			case SDL_MOUSEMOTION :
				OS.mouse_x += event.motion.xrel;
				OS.mouse_y += event.motion.yrel;
			break;
			case SDL_MOUSEBUTTONDOWN :
				if (event.button.button > 0
					&& event.button.button-1 < MOUSE_BUTTON_MAX)
					OS.mouse_button_map[event.button.button-1] = 1;
			break;
			case SDL_MOUSEBUTTONUP :
				if (event.button.button > 0
					&& event.button.button-1 < MOUSE_BUTTON_MAX)
					OS.mouse_button_map[event.button.button-1] = 0;
			break;
			case SDL_QUIT :
				OS.is_term = 1;
				break;
		}
	}

	if (OS.joystick_id != JOYSTICK_TYPE_NONE) {
		SDL_JoystickUpdate();
	}
}

void os_idle(void) {
	SDL_Delay(0);
}

void os_usleep(unsigned us) {
}

/***************************************************************************/
/* Keyboard */

struct os_device OS_KEY[] = {
	{ "none", KEY_TYPE_NONE, "No keyboard" },
	{ "auto", KEY_TYPE_AUTO, "Automatic detection" },
	{ 0, 0, 0 }
};

int os_key_init(int key_id, int disable_special)
{
#ifdef USE_KEYBOARD_SDL
	OS.key_id = key_id;

	return 0;
#else
	return -1;
#endif
}

void os_key_done(void)
{
	OS.key_id = KEY_TYPE_NONE;
}

unsigned os_key_get(unsigned code)
{
	if (OS.key_id != KEY_TYPE_NONE) {
		return OS.key_map[code];
	} else {
		return 0;
	}
}

void os_key_all_get(unsigned char* code_map)
{
	if (OS.key_id != KEY_TYPE_NONE) {
		unsigned i;
		for(i=0;i<OS_KEY_MAX;++i)
			code_map[i] = OS.key_map[i];
	} else {
		unsigned i;
		for(i=0;i<OS_KEY_MAX;++i)
			code_map[i] = 0;
	}
}

void os_led_set(unsigned mask)
{
}

/***************************************************************************/
/* Input */

int os_input_init(void) {
	return -1;
}

void os_input_done(void) {
}

int os_input_hit(void) {
	return 0;
}

unsigned os_input_get(void) {
	return 0;
}

/***************************************************************************/
/* Mouse */

struct os_device OS_MOUSE[] = {
	{ "none", MOUSE_TYPE_NONE, "No mouse" },
	{ "auto", MOUSE_TYPE_AUTO, "Automatic detection" },
	{ 0, 0, 0 }
};

int os_mouse_init(int mouse_id)
{
#ifdef USE_MOUSE_SDL
	OS.mouse_id = mouse_id;
	return 0;
#else
	return -1;
#endif
}

void os_mouse_done(void) {
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
	return MOUSE_BUTTON_MAX;
}

void os_mouse_pos_get(unsigned mouse, int* x, int* y)
{
	assert( mouse < os_mouse_count_get() );
	*x = OS.mouse_x;
	*y = OS.mouse_y;
	OS.mouse_x = 0;
	OS.mouse_y = 0;
}

unsigned os_mouse_button_get(unsigned mouse, unsigned button)
{
	assert( mouse < os_mouse_count_get() );
	assert( button < os_mouse_button_count_get(0) );
	return OS.mouse_button_map[button];
}

/***************************************************************************/
/* Joystick */

struct os_device OS_JOY[] = {
	{ "auto", JOYSTICK_TYPE_AUTO, "Automatic detection" },
	{ "none", JOYSTICK_TYPE_NONE, "No joystick" },
	{ 0, 0, 0 }
};

int os_joy_init(int joystick_id)
{
#ifdef USE_JOYSTICK_SDL
	OS.joystick_id = joystick_id;

	if (OS.joystick_id != JOYSTICK_TYPE_NONE) {
		unsigned i;

		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0)
			return -1;

		SDL_JoystickEventState(SDL_IGNORE);

		OS.joystick_counter = SDL_NumJoysticks();
		if (OS.joystick_counter > JOYSTICK_MAX)
			OS.joystick_counter = JOYSTICK_MAX;

		for(i=0;i<OS.joystick_counter;++i) {
			OS.joystick_map[i] = SDL_JoystickOpen(i);
			if (!OS.joystick_map[i])
				return -1;
		}
	}

	return 0;
#else
	return -1;
#endif
}

void os_joy_done(void)
{
	if (OS.joystick_id != JOYSTICK_TYPE_NONE) {
		unsigned i;
		for(i=0;i<OS.joystick_counter;++i)
			SDL_JoystickClose(OS.joystick_map[i]);

		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}

	OS.joystick_id = JOYSTICK_TYPE_NONE;
}

const char* os_joy_name_get(void) {
	return "Unknow";
}

const char* os_joy_driver_name_get(void) {
	return "SDL";
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
	return SDL_JoystickNumAxes(OS.joystick_map[j]);
}

unsigned os_joy_button_count_get(unsigned j) {
	assert(j < os_joy_count_get() );
	return SDL_JoystickNumButtons(OS.joystick_map[j]);
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
	return SDL_JoystickGetButton(OS.joystick_map[j],b) != 0;
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
	r = SDL_JoystickGetAxis(OS.joystick_map[j], a);
	if (d)
		return r < -16384;
	else
		return r > 16384;
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
	r = SDL_JoystickGetAxis(OS.joystick_map[j], a);
	r = r >> 8; /* adjust the upper limit from -128 to 128 */
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

/***************************************************************************/
/* Hardware */

void os_port_set(unsigned addr, unsigned value) {
}

unsigned os_port_get(unsigned addr) {
	return 0;
}

void os_writeb(unsigned addr, unsigned char c) {
}

unsigned char os_readb(unsigned addr) {
	return 0;
}

int os_mmx_get(void) {
	/* TODO MMX detect, assume yes */
	return 1;
}

void os_mode_reset(void) {
	/* no mode reset */
}

/***************************************************************************/
/* Sound */

void os_sound_error(void) {
	/* nothing */
}

void os_sound_warn(void) {
	/* nothing */
}

void os_sound_signal(void) {
	/* nothing */
}

/***************************************************************************/
/* APM */

int os_apm_shutdown(void) {
	return 0;
}

int os_apm_standby(void) {
	return 0;
}

int os_apm_wakeup(void) {
	return 0;
}

/***************************************************************************/
/* System */

int os_system(const char* cmd) {
	os_log(("sdl: system %s\n",cmd));

	return system(cmd);
}

int os_spawn(const char* file, const char** argv) {
	int pid, status;
	int i;

	os_log(("sdl: spawn %s\n",file));
	for(i=0;argv[i];++i)
		os_log(("sdl: spawn arg %s\n",argv[i]));

	pid = fork();
	if (pid == -1)
		return -1;

	if (pid == 0) {
		execvp(file, (char**)argv);
		exit(127);
	} else {
		while (1) {
			if (waitpid(pid, &status, 0) == -1) {
				if (errno != EINTR) {
					status = -1;
					break;
				}
			} else
				break;
		}

		return status;
	}
}

/***************************************************************************/
/* FileSystem */

char os_dir_separator(void) {
	return ':';
}

char os_dir_slash(void) {
	return '/';
}

const char* os_import(const char* path) {
	return path;
}

const char* os_export(const char* path) {
	return path;
}

/***************************************************************************/
/* Files */

const char* os_config_file_root(const char* file) {
	if (file[0] == '/')
		sprintf(OSF.file_root,"%s",file);
	else
		/* if relative add the root data dir */
		sprintf(OSF.file_root,"%s/%s",OSF.root_dir,file);
	return OSF.file_root;
}

const char* os_config_file_home(const char* file) {
	if (file[0] == '/')
		sprintf(OSF.file_home,"%s",file);
	else
		/* if relative add the home data dir */
		sprintf(OSF.file_home,"%s/%s",OSF.home_dir,file);
	return OSF.file_home;
}

const char* os_config_file_legacy(const char* file) {
	return 0;
}

const char* os_config_dir_multidir(const char* tag) {
	assert( tag[0] != '/' );
	sprintf(OSF.dir_buffer,"%s/%s:%s/%s",OSF.home_dir,tag,OSF.root_dir,tag);
	return OSF.dir_buffer;
}

const char* os_config_dir_singledir(const char* tag) {
	assert( tag[0] != '/' );
	sprintf(OSF.dir_buffer,"%s/%s",OSF.home_dir,tag);
	return OSF.dir_buffer;
}

const char* os_config_dir_singlefile(void) {
	sprintf(OSF.dir_buffer,"%s:%s",OSF.home_dir,OSF.root_dir);
	return OSF.dir_buffer;
}

/***************************************************************************/
/* Signal */

int os_is_term(void) {
	return OS.is_term;
}

void os_default_signal(int signum)
{
	os_log(("os: signal %d\n", signum));

#if defined(USE_VIDEO_SDL)
	os_log(("os: video_abort\n"));
	{
		extern void video_abort(void);
		video_abort();
	}
#endif

#if defined(USE_SOUND_SDL)
	os_log(("os: sound_abort\n"));
	{
		extern void sound_abort(void);
		sound_abort();
	}
#endif

	SDL_Quit();

	os_mode_reset();

	os_log(("os: close log\n"));

	os_msg_done();

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
	if (os_fixed() != 0)
		return EXIT_FAILURE;
	if (os_main(argc,argv) != 0)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

