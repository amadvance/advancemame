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
 */

#include "os.h"
#include "conf.h"
#include "mouseall.h"
#include "target.h"
#include "portable.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

void probe(void)
{
	int i;

	printf("Mouses %d\n", mouseb_count_get());
	for(i=0;i<mouseb_count_get();++i) {
		printf("Mouse %d, buttons %d\n", i, mouseb_button_count_get(i));
	}

	printf("\n");
}

static int done;

void sigint(int signum)
{
	done = 1;
}

void run(void)
{
	char msg[1024];
	char new_msg[1024];
	int i, j;
	target_clock_t last;

	printf("Press Break to exit\n");

	signal(SIGINT, sigint);

	last = target_clock();
	msg[0] = 0;
	while (!done) {

		new_msg[0] = 0;
		for(i=0;i<mouseb_count_get();++i) {
			int x, y, z;

			if (i!=0)
				sncat(new_msg, sizeof(new_msg), "\n");

			snprintf(new_msg + strlen(new_msg), sizeof(new_msg) - strlen(new_msg), "mouse %d, [", i);
			for(j=0;j<mouseb_button_count_get(i);++j) {
				if (mouseb_button_get(i, j))
					sncat(new_msg, sizeof(new_msg), "_");
				else
					sncat(new_msg, sizeof(new_msg), "-");
			}
			sncat(new_msg, sizeof(new_msg), "], ");

			mouseb_pos_get(i, &x, &y, &z);

			snprintf(new_msg + strlen(new_msg), sizeof(new_msg) - strlen(new_msg), " [%6d/%6d/%6d]", x, y, z);
		}

		if (strcmp(msg, new_msg)!=0) {
			target_clock_t current = target_clock();
			double period = (current - last) * 1000.0 / TARGET_CLOCKS_PER_SEC;
			sncpy(msg, sizeof(msg), new_msg);
			snprintf(new_msg + strlen(new_msg), sizeof(new_msg) - strlen(new_msg), " (%4.0f ms)", period);
			last = current;
			printf("%s\n", new_msg);
		}

		os_poll();
		mouseb_poll();
		target_idle();
	}
}

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...)
{
	va_list arg;
	va_start(arg, desc);
	vfprintf(stderr, desc, arg);
	fprintf(stderr, "\n");
	if (valid)
		fprintf(stderr, "%s\n", valid);
	va_end(arg);
}

void os_signal(int signum)
{
	os_default_signal(signum);
}

int os_main(int argc, char* argv[])
{
	adv_conf* context;
        const char* section_map[1];

	context = conf_init();

	if (os_init(context) != 0)
		goto err_conf;

	mouseb_reg(context, 1);
	mouseb_reg_driver_all(context);

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	if (argc > 1) {
		fprintf(stderr, "Unknown argument '%s'\n", argv[1]);
		goto err_os;
	}

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	if (mouseb_load(context) != 0)
		goto err_os;

	if (os_inner_init("AdvanceMOUSE") != 0)
		goto err_os;

	if (mouseb_init() != 0)
		goto err_os_inner;

	probe();
	run();

	mouseb_done();
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
	return EXIT_FAILURE;
}

