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
#include "fz.h"
#include "mixer.h"
#include "sounddrv.h"

#include <string.h>

#include "option.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static int done;

void sigint(int signum) {
	done = 1;
}

void run(unsigned channel, const char* file) {
	FZ* f;
	const char* ext;

	ext = strrchr(file,'.');
	if (!ext) {
		fprintf(stderr,"Missing file extension\n");
		done = 1;
		return;
	}

	f = fzopen(file);
	if (!f) {
		fprintf(stderr,"Error opening the file %s\n",file);
		done = 1;
		return;
	}

	if (strcmp(ext,".wav")==0) {
		mixer_play_file_wav(channel,f,0);
	} else if (strcmp(ext,".mp3")==0) {
		mixer_play_file_mp3(channel,f,0);
	} else {
		fprintf(stderr,"Unknow file extension %s\n",ext);
		fzclose(f);
		done = 1;
		return;
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
	int keyboard_id;
	struct conf_context* context;
	const char* s;
        const char* section_map[1];
	int i;
	unsigned n;
	double latency_time;
	double buffer_time;
	double volume;
	unsigned rate;
	int attenuation;

	context = conf_init();

	if (os_init(context) != 0)
		goto err_conf;

	mixer_reg(context);

	conf_int_register_limit_default(context, "sound_volume", -32, 0, 0);
	conf_int_register_limit_default(context, "sound_samplerate", 5000, 96000, 44100);
	conf_float_register_limit_default(context, "sound_latency", 0.05, 1.5, 0.1);
	conf_float_register_limit_default(context, "sound_buffer", 0.05, 1.5, 0.1);

	if (conf_input_args_load(context, 0, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	if (argc == 1) {
		fprintf(stderr,"Missing file\n");
		goto err_os;
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

	if (os_inner_init() != 0)
		goto err_os;

	n = argc - 1;

	if (n > MIXER_CHANNEL_MAX) {
		fprintf(stderr,"Too many files\n");
		goto err_os_inner;
	}

	if (mixer_init(rate,n,1,buffer_time + latency_time, latency_time) != 0) {
		fprintf(stderr,"Error initializing the mixer\n");
		goto err_os_inner;
	}

	mixer_volume(volume);

	for(i=1;i<argc;++i)
		run(i-1,argv[i]);

	signal(SIGINT,sigint);

	while (!done) {
		for(i=0;i<n;++i)
			if (mixer_is_playing(i))
				break;
		if (i==n)
			break;
		mixer_poll();
		os_idle();
	}

	mixer_done();

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

BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST

#endif
