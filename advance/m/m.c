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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

void probe(void) {
	int i,j,k;

	printf("Mouses %d\n",os_mouse_count_get());
	for(i=0;i<os_mouse_count_get();++i) {
		printf("Mouse %d, buttons %d\n",i,os_mouse_button_count_get(i));
	}

	printf("\n");
}

static int done;

void sigint(int signum) {
	done = 1;
}

void run(void) {
	char msg[1024];
	char new_msg[1024];
	int i,j,k;
	os_clock_t last;

	printf("Press Break to exit\n");

	signal(SIGINT,sigint);

	last = os_clock();
	msg[0] = 0;
	while (!done) {

		new_msg[0] = 0;
		for(i=0;i<os_mouse_count_get();++i) {
			int x,y;

			if (i!=0)
				strcat(new_msg,"\n");

			sprintf(new_msg + strlen(new_msg), "mouse %d, [",i);
			for(j=0;j<os_mouse_button_count_get(i);++j) {
				if (os_mouse_button_get(i,j))
					strcat(new_msg,"_");
				else
					strcat(new_msg,"-");
			}
			strcat(new_msg,"], ");

			os_mouse_pos_get(i,&x,&y);

			sprintf(new_msg + strlen(new_msg), " [%6d/%6d]",x,y);
		}

		if (strcmp(msg,new_msg)!=0) {
			os_clock_t current = os_clock();
			double period = (current - last) * 1000.0 / OS_CLOCKS_PER_SEC;
			strcpy(msg,new_msg);
			sprintf(new_msg + strlen(new_msg)," (%4.0f ms)",period);
			last = current;
			printf("%s\n",new_msg);
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
	int mouse_id;
	struct conf_context* context;
	const char* s;
        const char* section_map[1];
	int i;

	context = conf_init();

	if (os_init(context) != 0)
		goto err_conf;

	conf_string_register_default(context, "device_mouse", "auto");

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	if (argc > 1) {
		fprintf(stderr,"Unknow argument '%s'\n",argv[1]);
		goto err_os;
	}

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	s = conf_string_get_default(context, "device_mouse");
	mouse_id = 0;
	for (i=0;OS_MOUSE[i].name;++i) {
		if (strcmp(OS_MOUSE[i].name, s) == 0) {
                	mouse_id = OS_MOUSE[i].id;
			break;
		}
	}
	if (!OS_MOUSE[i].name) {
		printf("Invalid argument '%s' for option 'device_mouse'\n",s);
		printf("Valid values are:\n");
		for (i=0;OS_MOUSE[i].name;++i) {
			printf("%8s %s\n", OS_MOUSE[i].name, OS_MOUSE[i].desc);
		}
		goto err_os;
	}

	if (os_inner_init() != 0)
		goto err_os;

	if (os_mouse_init(mouse_id) != 0)
		goto err_os_inner;

	probe();
	run();

	os_mouse_done();
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
