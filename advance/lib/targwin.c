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

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>

#define BUFFER_SIZE 8192

struct target_context {
	char buffer_out[BUFFER_SIZE];
	char buffer_err[BUFFER_SIZE];
};

static struct target_context TARGET;

/***************************************************************************/
/* Init */

int target_init(void) {
	memset(&TARGET, 0, sizeof(TARGET));
	return 0;
}

void target_done(void) {
	target_flush();
}

/***************************************************************************/
/* Scheduling */

void target_yield(void) {
	Sleep(0);
}

void target_idle(void) {
	Sleep(1);
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
	MessageBeep(MB_ICONASTERISK);
}

void target_sound_warn(void) {
	MessageBeep(MB_ICONQUESTION);
}

void target_sound_signal(void) {
	MessageBeep(MB_OK);
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

static int exec(char* cmdline) {
	DWORD exitcode;
	PROCESS_INFORMATION process;
	STARTUPINFO startup;

	log_std(("win: CreateProcess(%s)\n", cmdline));

	memset(&startup,0, sizeof(startup));
	startup.cb = sizeof(startup);
	if (!CreateProcess(0, cmdline, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &startup, &process)) {
		log_std(("win: CreateProcess() failed %d\n", (unsigned)GetLastError()));
		return -1;
	}

	/* wait until the process terminate */
	while (1) {
		 MSG msg;
		if (!GetExitCodeProcess(process.hProcess, &exitcode)) {
			log_std(("win: GetExitCodeProcess() failed %d\n", (unsigned)GetLastError()));
			exitcode = -1;
			break;
		}

		if (exitcode != STILL_ACTIVE) {
			break;
		}

		if (os_is_term()) {
			log_std(("win: GetExitCodeProcess() aborted due TERM signal\n"));
			exitcode = -1;
			break;
		}

		/* flush the message queue, otherwise the window isn't updated */
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Sleep(100);
	}

	log_std(("win: GetExitCodeProcess() return %d\n", (unsigned)exitcode));

	CloseHandle(process.hProcess);
	CloseHandle(process.hThread);

	return exitcode;
}

int target_system(const char* cmd) {
	char cmdline[4096];
	char* comspec;

	comspec = getenv("COMSPEC");
	if (!comspec) {
		log_std(("win: getenv(COMSPEC) failed\n"));
		return -1;
	}

	strcpy(cmdline,comspec);
	strcat(cmdline," /C ");
	strcat(cmdline,cmd);

	return exec(cmdline);
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

	return exec(cmdline);
}

int target_mkdir(const char* file) {
	return mkdir(file);
}

void target_sync(void) {
	/* nothing */
}

int target_search(char* path, unsigned path_size, const char* file) {
	char* part;
	DWORD len;

	log_std(("win: target_search(%s)\n", file));

	len = SearchPath(0, file, ".exe", path_size, path, &part);

	if (len > path_size) {
		log_std(("win: SearchPath() failed due buffer too small\n"));
		return -1;
	}

	if (!len) {
		log_std(("win: SearchPath() failed %d\n", (unsigned)GetLastError()));
		return -1;
	}

	log_std(("win: target_search() return %s\n", path));
	return 0;
}

/***************************************************************************/
/* Stream */

void target_out_va(const char* text, va_list arg) {
	unsigned len = strlen(TARGET.buffer_out);
	/* euristic */
	if (len + strlen(text) < BUFFER_SIZE * 2 / 3)
		vsprintf(TARGET.buffer_out + len, text, arg);
}

void target_err_va(const char *text, va_list arg) {
	unsigned len = strlen(TARGET.buffer_err);
	/* euristic */
	if (len + strlen(text) < BUFFER_SIZE * 2 / 3)
		vsprintf(TARGET.buffer_err + len, text, arg);
}

void target_nfo_va(const char *text, va_list arg) {
	/* nothing */
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
	MSG msg;

	/* flush the message queue, otherwise the MessageBox may be not displayed */
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) 
			break;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	if (*TARGET.buffer_err) {
		MessageBox(NULL, TARGET.buffer_err, "Advance Error", MB_ICONERROR);
		*TARGET.buffer_err = 0;
	}
	
	if (*TARGET.buffer_out) {
		MessageBox(NULL, TARGET.buffer_out, "Advance Message", MB_ICONINFORMATION);
		*TARGET.buffer_out = 0;
	}
}

static void target_backtrace(void) {
}

void target_signal(int signum) {
	if (signum == SIGINT) {
		fprintf(stderr,"Break pressed\n\r");
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr,"Signal %d.\n",signum);
		fprintf(stderr,"%s, %s\n\r", __DATE__, __TIME__);

		if (signum == SIGILL) {
			fprintf(stderr,"Are you using the correct binary ?\n");
		}

		_exit(EXIT_FAILURE);
	}
}

void target_crash(void) {
	unsigned* i = (unsigned*)0;
	++*i;
	abort();
}
