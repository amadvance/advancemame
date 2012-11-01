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
	int i, j, k;

	printf("Driver %s, joysticks %d\n", joystickb_name(), joystickb_count_get());
	for(i=0;i<joystickb_count_get();++i) {
		printf("joy %d, controls %d, buttons %d, ball axes %d\n", i, joystickb_stick_count_get(i), joystickb_button_count_get(i), joystickb_rel_count_get(i));
		for(j=0;j<joystickb_stick_count_get(i);++j) {
			printf("\tcontrol %d [%s], axes %d\n", j, joystickb_stick_name_get(i, j), joystickb_stick_axe_count_get(i, j));
			for(k=0;k<joystickb_stick_axe_count_get(i,j);++k) {
				printf("\t\taxe %d [%s]\n", k, joystickb_stick_axe_name_get(i, j, k));
			}
		}
		for(j=0;j<joystickb_button_count_get(i);++j) {
			printf("\tbutton %d [%s]\n", j, joystickb_button_name_get(i, j));
		}
		for(j=0;j<joystickb_rel_count_get(i);++j) {
			printf("\tball axe %d [%s]\n", j, joystickb_rel_name_get(i, j));
		}
	}

	printf("\n");
}

int button_pressed(void)
{
	int i, j;

	os_poll();
	joystickb_poll();
	target_idle();

	for(i=0;i<joystickb_count_get();++i) {
		for(j=0;j<joystickb_button_count_get(i);++j) {
			if (joystickb_button_get(i, j))
				return 1;
		}
	}

	return 0;
}

void wait_button_press(void)
{
	while (!button_pressed());
}

void wait_button_release(void)
{
	target_clock_t start = target_clock();
	while (target_clock() - start < TARGET_CLOCKS_PER_SEC / 10) {
		if (button_pressed())
			start = target_clock();
	}
}

void calibrate(void)
{
	const char* msg;
	int step;

	joystickb_calib_start();

	msg = joystickb_calib_next();
	if (msg) {
		step = 1;
		printf("Calibration:\n");
		while (msg) {
			printf("%d) %s and press a joystick button\n", step, msg);
			++step;

			wait_button_press();

			msg = joystickb_calib_next();

			wait_button_release();
		}

		printf("\n");
	}
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
	int i, j, k;
	target_clock_t last;

	printf("Press Break to exit\n");

	signal(SIGINT, sigint);

	last = target_clock();
	msg[0] = 0;
	while (!done) {

		new_msg[0] = 0;
		for(i=0;i<joystickb_count_get();++i) {

			if (i!=0)
				sncat(new_msg, sizeof(new_msg), "\n");

			snprintf(new_msg + strlen(new_msg), sizeof(new_msg) - strlen(new_msg), "joy %d, [", i);
			for(j=0;j<joystickb_button_count_get(i);++j) {
				if (joystickb_button_get(i, j))
					sncat(new_msg, sizeof(new_msg), "_");
				else
					sncat(new_msg, sizeof(new_msg), "-");
			}
			sncat(new_msg, sizeof(new_msg), "], ");
			for(j=0;j<joystickb_stick_count_get(i);++j) {
				for(k=0;k<joystickb_stick_axe_count_get(i, j);++k) {
					char digital;
					if (joystickb_stick_axe_digital_get(i, j, k, 0))
						digital = '\\';
					else if (joystickb_stick_axe_digital_get(i, j, k, 1))
						digital = '/';
					else
						digital = '-';
					sncatf(new_msg, sizeof(new_msg), " %d/%d [%6d %c]", j, k, joystickb_stick_axe_analog_get(i, j, k), digital);
				}
			}

			sncat(new_msg, sizeof(new_msg), " [");

			for(j=0;j<joystickb_rel_count_get(i);++j) {
				if (j != 0)
					sncat(new_msg, sizeof(new_msg), "/");
				sncatf(new_msg, sizeof(new_msg), "%d", joystickb_rel_get(i, j));
			}

			sncat(new_msg, sizeof(new_msg), "]");
		}

		if (strcmp(msg, new_msg)!=0) {
			target_clock_t current = target_clock();
			double period = (current - last) * 1000.0 / TARGET_CLOCKS_PER_SEC;
			last = current;
			sncpy(msg, sizeof(msg), new_msg);
			printf("%s (%4.0f ms)\n", msg, period);
		}

		os_poll();
		joystickb_poll();
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

	joystickb_reg(context, 1);
	joystickb_reg_driver_all(context);

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
		const char* log = "advj.log";
		remove(log);
		log_init(log, opt_logsync);
        }

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	if (joystickb_load(context) != 0)
		goto err_os;

	if (os_inner_init("AdvanceJOYSTICK") != 0)
		goto err_os;

	if (joystickb_init() != 0) {
		target_err("%s\n", error_get());
		goto err_os_inner;
	}

	probe();
	calibrate();
	run();

	joystickb_done();
	os_inner_done();

	log_std(("j: the end\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_done();
	conf_done(context);

	return EXIT_SUCCESS;

err_os_inner:
	os_inner_done();
	log_done();
err_os:
	os_done();
err_conf:
	conf_done(context);
	return EXIT_FAILURE;
}

