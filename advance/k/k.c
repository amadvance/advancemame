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
#include "keyall.h"
#include "target.h"
#include "portable.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static int done;

void sigint(int signum)
{
	done = 1;
}

void run(void)
{
	char msg[1024];
	char new_msg[1024];
	int esc_pressed_before;
	int esc_count;
	target_clock_t last;

	printf("Press ESC three times or Break to exit\n");

	signal(SIGINT, sigint);

	last = target_clock();
	esc_pressed_before = 0;
	esc_count = 0;
	sncpy(msg, sizeof(msg), "unknown");

	while (esc_count < 3 && !done) {
		int i;
		int esc_pressed = 0;
		unsigned count = 0;
		*new_msg = 0;

		for(i=0;i<KEYB_MAX;++i) {
			if (keyb_get(i)) {
				if (i==KEYB_ESC)
					esc_pressed = 1;
				++count;
				snprintf(new_msg + strlen(new_msg), sizeof(new_msg) - strlen(new_msg), "%s ", key_name(i));
			}
		}

		if (strcmp(msg, new_msg)!=0) {
			target_clock_t current = target_clock();
			double period = (current - last) * 1000.0 / TARGET_CLOCKS_PER_SEC;
			last = current;
			sncpy(msg, sizeof(msg), new_msg);
			printf("(%6.1f ms) [%3d] %s\n", period, count, msg);
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
		keyb_poll();
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

	keyb_reg(context, 1);
	keyb_reg_driver_all(context);

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	if (argc > 1) {
		fprintf(stderr, "Unknown argument '%s'\n", argv[1]);
		goto err_os;
	}

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	if (keyb_load(context) != 0)
		goto err_os;

	if (os_inner_init("AdvanceKEY") != 0)
		goto err_os;

	if (keyb_init(0) != 0)
		goto err_os_inner;

	run();

	keyb_done();
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

