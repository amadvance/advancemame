/*
 * This file is part of the AdvanceMAME project.
 *
 * Copyright (C) 2002 Andrea Mazzoleni
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

#include "target.h"
#include "log.h"

#include "os.h"

#include "SDL.h"

#include <stdio.h>
#include <stdlib.h>

/***************************************************************************/
/* Init */

int target_init(void) {
	return 0;
}

void target_done(void) {
	target_flush();
}

/***************************************************************************/
/* Scheduling */

void target_yield(void) {
}

void target_idle(void) {
	SDL_Delay(1);
}

void target_usleep(unsigned us) {
}

/***************************************************************************/
/* Hardware */

void target_port_set(unsigned addr, unsigned value) {
}

unsigned target_port_get(unsigned addr) {
	return 0;
}

void target_writeb(unsigned addr, unsigned char c) {
}

unsigned char target_readb(unsigned addr) {
	return 0;
}

/***************************************************************************/
/* Mode */

void target_mode_reset(void) {
	/* nothing */
}

/***************************************************************************/
/* Sound */

void target_sound_error(void) {
	/* nothing */
}

void target_sound_warn(void) {
	/* nothing */
}

void target_sound_signal(void) {
	/* nothing */
}

/***************************************************************************/
/* APM */

int target_apm_shutdown(void) {
	/* nothing */
	return 0;
}

int target_apm_standby(void) {
	/* nothing */
	return 0;
}

int target_apm_wakeup(void) {
	/* nothing */
	return 0;
}

/***************************************************************************/
/* System */

int target_system(const char* cmd) {
	if (system(cmd) != 0)
		return -1;
		
	return 0;
}

int target_spawn(const char* file, const char** argv) {
	char cmdline[4096];
	unsigned i;

	*cmdline = 0;
	for(i=0;argv[i];++i) {
		if (i)
			strcat(cmdline," ");
		if (strchr(argv[i],' ') != 0) {
			strcat(cmdline, "\"");
			strcat(cmdline, argv[i]);
			strcat(cmdline, "\"");
		} else {
			strcat(cmdline, argv[i]);
		}
	}

	if (system(cmdline) != 0)
		return -1;

	return 0;
}

int target_mkdir(const char* file) {
	return mkdir(file);
}

void target_sync(void) {
	/* nothing */
}

int target_search(char* path, unsigned path_size, const char* file) {
	strcpy(path, file);
	if (access(path, F_OK) != 0) {
		return -1;
	}

	return 0;
}

void target_out_va(const char* text, va_list arg) {
	vfprintf(stdout, text, arg);
}

void target_err_va(const char *text, va_list arg) {
	vfprintf(stderr, text, arg);
}

void target_nfo_va(const char *text, va_list arg) {
	vfprintf(stderr, text, arg);
}

void target_out(const char *text, ...) {
	va_list arg;
	va_start(arg, text);
	target_out_va(text, arg);
	va_end(arg);
}

void target_err(const char *text, ...) {
	va_list arg;
	va_start(arg, text);
	target_err_va(text, arg);
	va_end(arg);
}

void target_nfo(const char *text, ...) {
	va_list arg;
	va_start(arg, text);
	target_nfo_va(text, arg);
	va_end(arg);
}

void target_flush(void) {
	fflush(stdout);
	fflush(stderr);
}
