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

static int done;

void sigint(int signum)
{
	done = 1;
}

void run(unsigned channel, const char* file)
{
	adv_fz* f;
	const char* ext;

	ext = strrchr(file, '.');
	if (!ext) {
		target_err("Missing file extension\n");
		done = 1;
		return;
	}

	f = fzopen(file, "rb");
	if (!f) {
		target_err("Error opening the file %s\n", file);
		done = 1;
		return;
	}

	if (strcmp(ext, ".wav")==0) {
		mixer_play_file_wav(channel, f, 0);
	} else if (strcmp(ext, ".mp3")==0) {
		mixer_play_file_mp3(channel, f, 0);
	} else {
		target_err("Unknown file extension %s\n", ext);
		fzclose(f);
		done = 1;
		return;
	}
}

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...)
{
	va_list arg;
	va_start(arg, desc);
	target_err_va(desc, arg);
	target_err("\n");
	if (valid)
		target_err("%s\n", valid);
	va_end(arg);
}

void os_signal(int signum, void* info, void* context)
{
	os_default_signal(signum, info, context);
}

int os_main(int argc, char* argv[])
{
	int keyboard_id;
	adv_conf* context;
	const char* s;
	const char* section_map[1];
	const char** file_map;
	unsigned file_mac;
	int i;
	double latency_time;
	double buffer_time;
	double volume;
	unsigned rate;
	int attenuation;
	adv_bool opt_log;
	adv_bool opt_logsync;

	opt_log = 0;
	opt_logsync = 0;
	file_map = 0;
	file_mac = 0;

	context = conf_init();

	if (os_init(context) != 0)
		goto err_conf;

	mixer_reg(context);

	conf_int_register_limit_default(context, "sound_volume", -32, 0, 0);
	conf_int_register_limit_default(context, "sound_samplerate", 5000, 96000, 44100);
	conf_float_register_limit_default(context, "sound_latency", 0.01, 2.0, 0.1);
	conf_float_register_limit_default(context, "sound_buffer", 0.05, 2.0, 0.1);

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	file_map = malloc(argc * sizeof(const char*));

	for(i=1;i<argc;++i) {
		if (target_option_compare(argv[i], "log")) {
			opt_log = 1;
		} else if (target_option_compare(argv[i], "logsync")) {
			opt_logsync = 1;
		} else if (target_option_extract(argv[i]) == 0) {
			file_map[file_mac++] = argv[i];
		} else {
			target_err("Unknown command line option '%s'.\n", argv[i]);
			goto err_os;
		}
	}

	if (argc <= 1 || file_mac == 0) {
		target_err("Syntax: advs FILES...\n");
		goto err_os;
	}

	if (opt_log || opt_logsync) {
		const char* log = "advs.log";
		remove(log);
		log_init(log, opt_logsync);
        }

	section_map[0] = "";
	conf_section_set(context, section_map, 1);

	if (mixer_load(context) != 0) {
		goto err_os;
	}

	attenuation = conf_int_get_default(context, "sound_volume");
	latency_time = conf_float_get_default(context, "sound_latency");
	buffer_time = conf_float_get_default(context, "sound_buffer");
	rate = conf_int_get_default(context, "sound_samplerate");
	volume = 1.0;
	while (attenuation++ < 0)
		volume /= 1.122018454; /* = (10 ^ (1/20)) = 1dB */

	if (os_inner_init("AdvanceSOUND") != 0)
		goto err_os;

	if (file_mac > MIXER_CHANNEL_MAX) {
		target_err("Too many files\n");
		goto err_os_inner;
	}

	if (mixer_init(rate, file_mac, 1, buffer_time + latency_time, latency_time) != 0) {
		target_err("%s\n", error_get());
		goto err_os_inner;
	}

	mixer_volume(volume);

	for(i=0;i<file_mac;++i)
		run(i, file_map[i]);

	free(file_map);

	signal(SIGINT, sigint);

	while (!done) {
		for(i=0;i<file_mac;++i)
			if (mixer_is_playing(i))
				break;
		if (i==file_mac)
			break;

		mixer_poll();
		target_idle();
	}

	log_std(("s: shutdown\n"));

	mixer_done();

	os_inner_done();

	log_std(("s: the end\n"));

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
err:
	return EXIT_FAILURE;

}

