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
 */

#include "portable.h"

#include "advance.h"

void probe(void)
{
	int i, j;

	printf("Driver %s, keyboards %d\n", keyb_name(), keyb_count_get());
	for(i=0;i<keyb_count_get();++i) {
		printf("keyboard %d\n", i);
		printf("\tkeys");
		for(j=0;j<KEYB_MAX;++j) {
			if (keyb_has(i, j)) {
				printf(" %s", key_name(j));
			}
		}
		printf("\n");
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
	int esc_pressed_before;
	int esc_count;
	target_clock_t last;
	unsigned led;

	printf("Press ESC three times or Break to exit\n");

	signal(SIGINT, sigint);

	last = target_clock();
	esc_pressed_before = 0;
	esc_count = 0;
	sncpy(msg, sizeof(msg), "unknown");
	led = 0;

	while (esc_count < 3 && !done) {
		int i;
		int k;
		int esc_pressed = 0;
		unsigned count = 0;
		*new_msg = 0;

		for(k=0;k<keyb_count_get();++k) {
			for(i=0;i<KEYB_MAX;++i) {
				if (keyb_get(k,i)) {
					if (i==KEYB_ESC)
						esc_pressed = 1;
					++count;
					snprintf(new_msg + strlen(new_msg), sizeof(new_msg) - strlen(new_msg), "%s[%d] ", key_name(i), k);
				}
			}
		}

		if (strcmp(msg, new_msg)!=0) {
			target_clock_t current = target_clock();
			double period = (current - last) * 1000.0 / TARGET_CLOCKS_PER_SEC;
			last = current;
			sncpy(msg, sizeof(msg), new_msg);
			printf("(%6.1f ms) [%3d] %s\n", period, count, msg);

			for(k=0;k<keyb_count_get();++k) {
				keyb_led_set(k, led);
			}
			++led;
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
		target_yield();
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

void os_signal(int signum, void* info, void* context)
{
	os_default_signal(signum, info, context);
}

int os_main(int argc, char* argv[])
{
	int i;
	adv_conf* context;
	const char* section_map[1];
	adv_bool opt_log;
	adv_bool opt_logsync;

	opt_log = 0;
	opt_logsync = 0;

	context = conf_init();

	if (os_init(context) != 0)
		goto err_conf;

	keyb_reg(context, 1);
	keyb_reg_driver_all(context);

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	for(i=1;i<argc;++i) {
		if (target_option_compare(argv[i], "log")) {
			opt_log = 1;
		} else if (target_option_compare(argv[i], "logsync")) {
			opt_logsync = 1;
		} else {
			fprintf(stderr, "Unknown argument '%s'\n", argv[1]);
			goto err_os;
		}
	}

	if (opt_log || opt_logsync) {
		const char* log = "advk.log";
		remove(log);
		log_init(log, opt_logsync);
        }

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	if (keyb_load(context) != 0)
		goto err_os;

	if (os_inner_init("AdvanceKEY") != 0)
		goto err_os;

	if (keyb_init(0) != 0) {
		target_err("%s\n", error_get());
		goto err_os_inner;
	}

	probe();

	if (keyb_enable(0) != 0) {
		target_err("%s\n", error_get());
		goto err_done;
	}

	run();

	keyb_disable();
	keyb_done();
	os_inner_done();

	log_std(("k: the end\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_done();
	conf_done(context);

	return EXIT_SUCCESS;

err_done:
	keyb_done();
err_os_inner:
	os_inner_done();
	log_done();
err_os:
	os_done();
err_conf:
	conf_done(context);
	return EXIT_FAILURE;
}

