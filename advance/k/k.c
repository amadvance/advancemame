/*
 * This file is part of the AdvanceMAME project.
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
 */

#include "os.h"
#include "conf.h"
#include "key.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static int done;

void sigint(int signum) {
	done = 1;
}

void run(void) {
	char cmd[256];
	int esc_pressed_before;
	int esc_count;
	os_clock_t last;

	printf("Press ESC three times or Break to exit\n");

	signal(SIGINT,sigint);

	last = os_clock();
	esc_pressed_before = 0;
	esc_count = 0;
	strcpy(cmd, "unknown");

	while (esc_count < 3 && !done) {
		int i;
		int esc_pressed = 0;
		unsigned count = 0;
		char newcmd[256];
		*newcmd = 0;

		for(i=0;i<OS_KEY_MAX;++i) {
			if (os_key_get(i)) {
				if (i==OS_KEY_ESC)
					esc_pressed = 1;
				++count;
				sprintf(newcmd + strlen(newcmd), "%s ", key_name(i));
			}
		}

		if (strcmp(cmd,newcmd)!=0) {
			os_clock_t current = os_clock();
			double period = (current - last) * 1000.0 / OS_CLOCKS_PER_SEC;
			last = current;
			strcpy(cmd,newcmd);
			printf("(%6.1f ms) [%3d] %s\n",period,count,cmd);
		}

		if (count == 1 && esc_pressed) {
			esc_pressed_before = 1;
		} else if (count == 0) {
			if (esc_pressed_before) {
				++esc_count;
				esc_pressed_before = 0;
			}
		} else {
			esc_count = 0;
		}

		os_poll();
		os_idle();
	}
}

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...) {
	va_list arg;
	va_start(arg, desc);
	vfprintf(stderr, desc, arg);
	fprintf(stderr, "\n");
	if (valid)
		fprintf(stderr, "%s\n", valid);
	va_end(arg);
}

void os_signal(int signum) {
	os_default_signal(signum);
}

int os_main(int argc, char* argv[]) {
	int keyboard_id = 0;
	struct conf_context* context;
	const char* s;
        const char* section_map[1];
	int i;

	context = conf_init();

	if (os_init(context) != 0)
		goto err_conf;

	conf_string_register_default(context, "device_keyboard", "auto");

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	if (argc > 1) {
		fprintf(stderr,"Unknow argument '%s'\n",argv[1]);
		goto err_os;
	}

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	s = conf_string_get_default(context, "device_keyboard");
	for (i=0;OS_KEY[i].name;++i) {
		if (strcmp(OS_KEY[i].name, s) == 0) {
                	keyboard_id = OS_KEY[i].id;
			break;
		}
	}
	if (!OS_KEY[i].name) {
		printf("Invalid argument '%s' for option 'device_keyboard'\n",s);
		printf("Valid values are:\n");
		for (i=0;OS_KEY[i].name;++i) {
			printf("%8s %s\n", OS_KEY[i].name, OS_KEY[i].desc);
		}
		goto err_os;
	}

	if (os_inner_init() != 0)
		goto err_os;

	if (os_key_init(keyboard_id, 0) != 0)
		goto err_os_inner;

	run();

	os_key_done();
	os_inner_done();
	os_done();
	conf_done(context);

	return EXIT_SUCCESS;

err_os_inner:
	os_inner_done();
err_os:
	os_done();
err_conf:
	conf_done(context);
err:
	return EXIT_FAILURE;

}

#ifdef __MSDOS__

/* Keep Allegro small */
BEGIN_GFX_DRIVER_LIST
END_GFX_DRIVER_LIST

BEGIN_COLOR_DEPTH_LIST
END_COLOR_DEPTH_LIST

BEGIN_DIGI_DRIVER_LIST
END_DIGI_DRIVER_LIST

BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST

#endif
