/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003 Andrea Mazzoleni
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

#include "portable.h"

#include "target.h"
#include "file.h"
#include "log.h"
#include "os.h"
#include "snstring.h"
#include "oswin.h"

#include "SDL.h"

#include <windows.h>
#include <io.h>

#define BUFFER_SIZE 16384

struct target_context {
	char buffer_out[BUFFER_SIZE];
	char buffer_err[BUFFER_SIZE];

	unsigned usleep_granularity; /**< Minimun sleep time in microseconds. */
};

static struct target_context TARGET;

target_clock_t TARGET_CLOCKS_PER_SEC;

/***************************************************************************/
/* Init */

adv_error target_init(void)
{
	LARGE_INTEGER f;
	LARGE_INTEGER c;

	TARGET.buffer_out[0] = 0;
	TARGET.buffer_err[0] = 0;
	TARGET.usleep_granularity = 0;

	if (!QueryPerformanceFrequency(&f) || !QueryPerformanceCounter(&c)) {
		target_err("Error initializing the high-resolution performance counter.\n");
		return -1;
	}

	TARGET_CLOCKS_PER_SEC = f.QuadPart;

	return 0;
}

void target_done(void)
{
	target_flush();
}

/***************************************************************************/
/* Scheduling */

void target_yield(void)
{
	Sleep(0);
}

void target_idle(void)
{
	Sleep(1);
}

void target_usleep_granularity(unsigned us)
{
	TARGET.usleep_granularity = us;
}

void target_usleep(unsigned us)
{
#if 0 /* OSDEF Test code */
	/* For now disabled. It needs more testing. */
	/* TODO Try to increase the precision using MultimediaTimer and */
	/* timeBeginPeriod/timeEndPeriod */

	target_clock_t start;
	target_clock_t stop;
	unsigned requested;
	unsigned effective;

	/* if too short don't wait */
	if (us <= TARGET.usleep_granularity) {
		return;
	}

	requested = us - TARGET.usleep_granularity;

	start = target_clock();
	Sleep(requested / 1000);
	stop = target_clock();

	effective = (stop - start) * 1000000 / TARGET_CLOCKS_PER_SEC;

	if (effective > us) {
		/* don't adjust the granularity, it should be measured by the OS code */
		/* TARGET.usleep_granularity += effective - us; */
		log_std(("WARNING:linux: target_usleep() too long, granularity %d [us] (requested %d, tryed %d, effective %d)\n", TARGET.usleep_granularity, us, requested, effective));
	}
#endif
}

/***************************************************************************/
/* Clock */

target_clock_t target_clock(void)
{
	LARGE_INTEGER c;
	target_clock_t r;
    
	if (!QueryPerformanceCounter(&c)) {
		log_std(("ERROR:windows: QueryPerformanceCounter() failed\n"));
		return 0;
	}

	r = c.QuadPart;

	return r;
}

/***************************************************************************/
/* Hardware */

void target_port_set(unsigned addr, unsigned value)
{
	log_std(("ERROR:windows: write at port 0x%x not allowed\n", addr));
}

unsigned target_port_get(unsigned addr)
{
	log_std(("ERROR:windows: read at port 0x%x not allowed\n", addr));
	return 0;
}

void target_writeb(unsigned addr, unsigned char c)
{
	log_std(("ERROR:windows: write at address 0x%x not allowed\n", addr));
}

unsigned char target_readb(unsigned addr)
{
	log_std(("ERROR:windows: read at address 0x%x not allowed\n", addr));
	return 0;
}

/***************************************************************************/
/* Mode */

void target_mode_reset(void)
{
	/* nothing */
}

/***************************************************************************/
/* Sound */

void target_sound_error(void)
{
	/* MessageBeep(MB_ICONASTERISK); */
}

void target_sound_warn(void)
{
	/* MessageBeep(MB_ICONQUESTION); */
}

void target_sound_signal(void)
{
	/* MessageBeep(MB_OK); */
}

/***************************************************************************/
/* APM */

#define WIN2K_EWX_FORCEIFHUNG 0x00000010

adv_error target_apm_shutdown(void)
{
	OSVERSIONINFO VersionInformation;
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	DWORD flags = EWX_POWEROFF;

#if 0 /* OSDEF Reference code */
	/* Forces processes to terminate. When this flag is set, the system does not send */
	/* the WM_QUERYENDSESSION and WM_ENDSESSION messages. This can cause the applications to */
	/* lose data. Therefore, you should only use this flag in an emergency. */
	flags |= EWX_FORCE;
#endif

	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx(&VersionInformation)) {
		return -1;
	}

	/* It doesn't work on Windows 95/98/Me because all the Advance applications */
	/* are console applications. */
	/* From the MSDN: ExitWindowEx does not work from a console application. */
	if (VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) {
		return -1;
	}

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return -1;
 
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
 
	if (GetLastError() != ERROR_SUCCESS)
		return -1;

	if ((flags & EWX_FORCE) == 0) {
		if (VersionInformation.dwMajorVersion >= 5)
			/* Forces processes to terminate if they do not respond to the WM_QUERYENDSESSION or */
			/* WM_ENDSESSION message. This flag is ignored if EWX_FORCE is used. */
			flags |= WIN2K_EWX_FORCEIFHUNG;
	}

	if (!ExitWindowsEx(flags, 0))
		return -1;

	return 0;
}

adv_error target_apm_standby(void)
{
	/* nothing */
	return 0;
}

adv_error target_apm_wakeup(void)
{
	/* nothing */
	return 0;
}

/***************************************************************************/
/* System */

#define EXEC_MAX 2048

static int exec(char* cmdline)
{
	DWORD exitcode;
	PROCESS_INFORMATION process;
	STARTUPINFO startup;

	log_std(("windows: CreateProcess(%s)\n", cmdline));

	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	if (!CreateProcess(0, cmdline, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &startup, &process)) {
		log_std(("ERROR:windows: CreateProcess() failed %d\n", (unsigned)GetLastError()));
		return -1;
	}

	/* wait until the process terminate */
	while (1) {
		MSG msg;
		if (!GetExitCodeProcess(process.hProcess, &exitcode)) {
			log_std(("ERROR:windows: GetExitCodeProcess() failed %d\n", (unsigned)GetLastError()));
			exitcode = -1;
			break;
		}

		if (exitcode != STILL_ACTIVE) {
			break;
		}

		if (os_is_quit()) {
			log_std(("ERROR:windows: GetExitCodeProcess() aborted due TERM signal\n"));
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

	log_std(("windows: GetExitCodeProcess() return %d\n", (unsigned)exitcode));

	CloseHandle(process.hProcess);
	CloseHandle(process.hThread);

	return exitcode;
}

static void sncatarg(char* cmd, unsigned size, const char* arg)
{
	if (arg[0] != '\"' && strchr(arg, ' ') != 0) {
		sncat(cmd, size, "\"");
		sncat(cmd, size, arg);
		sncat(cmd, size, "\"");
	} else {
		sncat(cmd, size, arg);
	}
}

adv_error target_script(const char* script)
{
	char* tmp;
	char file[FILE_MAXPATH];
	const char* argv[2];
	FILE* f;
	int r;

	os_fire();

	log_std(("windows: script\n%s\n", script));

	tmp = getenv("TMP");
	if (!tmp)
		tmp = getenv("TEMP");
	if (!tmp) {
		log_std(("ERROR:windows: getenv(TMP,TEMP) failed\n"));
		return -1;
	}

	sncpy(file, FILE_MAXPATH, tmp);
	if (file[0] && file[strlen(file)-1] != '\\')
		sncat(file, FILE_MAXPATH, "\\");
	sncat(file, FILE_MAXPATH, "advs0000.bat");

	log_std(("windows: file %s\n", file));

	f = fopen(file, "w");
	if (!f) {
		log_std(("ERROR:windows: fopen(%s) failed\n", file));
		goto err;
	}

	if (fprintf(f, "%s", script) < 0) {
		log_std(("ERROR:windows: fprintf() failed\n"));
		goto err_close;
	}

	if (fclose(f) != 0) {
		log_std(("ERROR:windows: fclose() failed\n"));
		goto err;
	}

	argv[0] = file;
	argv[1] = 0;

	r = target_spawn_redirect(file, argv, "NUL");

	log_std(("windows: return %d\n", r));

	remove(file); /* ignore error */

	return r;

err_close:
	fclose(f);
err:
	return -1;
}

adv_error target_spawn_redirect(const char* file, const char** argv, const char* output)
{
	char cmdline[EXEC_MAX];
	char* comspec;
	unsigned i;
	int r;

	os_fire();

	comspec = getenv("COMSPEC");
	if (!comspec) {
		log_std(("ERROR:windows: getenv(COMSPEC) failed\n"));
		return -1;
	}

	*cmdline = 0;

	/* The /S option and the extra " is required for the */
	/* CMD processing. But it don't work for COMMAND.COM */

	sncatarg(cmdline, EXEC_MAX, comspec);
	sncat(cmdline, EXEC_MAX, " /S /C \"");

	for(i=0;argv[i];++i) {
		if (i)
			sncat(cmdline, EXEC_MAX, " ");
		sncatarg(cmdline, EXEC_MAX, argv[i]);
	}

	sncat(cmdline, EXEC_MAX, " > ");
	sncatarg(cmdline, EXEC_MAX, output);
	sncat(cmdline, EXEC_MAX, "\"");

	r = exec(cmdline);

	log_std(("windows: system return %d\n", r));

	return r;
}

adv_error target_spawn(const char* file, const char** argv)
{
	char cmdline[EXEC_MAX];
	unsigned i;
	int r;

	os_fire();

	log_std(("windows: spawn %s\n", file));
	for(i=0;argv[i];++i)
		log_std(("windows: spawn arg%d %s\n", i, argv[i]));

	*cmdline = 0;

	for(i=0;argv[i];++i) {
		if (i)
			sncat(cmdline, EXEC_MAX, " ");
		sncatarg(cmdline, EXEC_MAX, argv[i]);
	}

	r = exec(cmdline);

	log_std(("windows: spawn return %d\n", r));

	return r;
}

adv_error target_mkdir(const char* file)
{
	return mkdir(file);
}

void target_sync(void)
{
	/* nothing */
}

adv_error target_search(char* path, unsigned path_size, const char* file)
{
	char* part;
	DWORD len;

	log_std(("windows: target_search(%s)\n", file));

	len = SearchPath(0, file, ".exe", path_size, path, &part);

	if (len > path_size) {
		log_std(("ERROR:windows: SearchPath() failed due buffer too small\n"));
		return -1;
	}

	if (!len) {
		log_std(("ERROR:windows: SearchPath() failed %d\n", (unsigned)GetLastError()));
		return -1;
	}

	log_std(("windows: target_search() return %s\n", path));
	return 0;
}

/***************************************************************************/
/* Stream */

void target_out_va(const char* text, va_list arg)
{
	unsigned len = strlen(TARGET.buffer_out);
	if (len < BUFFER_SIZE)
		vsnprintf(TARGET.buffer_out + len, BUFFER_SIZE - len, text, arg);
}

void target_err_va(const char *text, va_list arg)
{
	unsigned len = strlen(TARGET.buffer_err);
	if (len < BUFFER_SIZE)
		vsnprintf(TARGET.buffer_err + len, BUFFER_SIZE - len, text, arg);
}

void target_nfo_va(const char *text, va_list arg)
{
	/* nothing */
}

void target_out(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_out_va(text, arg);
	va_end(arg);
}

void target_err(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_err_va(text, arg);
	va_end(arg);
}

void target_nfo(const char *text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_nfo_va(text, arg);
	va_end(arg);
}

void target_flush(void)
{
	MSG msg;

	os_fire();

	/* flush the message queue, otherwise the MessageBox may be not displayed */
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) 
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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

void target_signal(int signum, void* info, void* context)
{
	if (signum == SIGINT) {
		fprintf(stderr, "Break\n\r");
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Signal %d\n\r", signum);
		fprintf(stderr, "Compiled %s, %s\n\r", __DATE__, __TIME__);

		_exit(EXIT_FAILURE);
	}
}

void target_crash(void)
{
	abort();
}

const char* target_option_extract(const char* arg)
{
	if (arg[0] != '-' && arg[0] != '/')
		return 0;
	return arg + 1;
}

adv_bool target_option_compare(const char* arg, const char* opt)
{
	const char* name = target_option_extract(arg);
	return name!=0 && strcasecmp(name, opt) == 0;
}

