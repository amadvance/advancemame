/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Andrea Mazzoleni
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
#include "log.h"
#include "file.h"
#include "snstring.h"

#include "oslinux.h"

#if HAVE_SCHED_H
#include <sched.h>
#endif
#if HAVE_TERMIOS_H
#include <termios.h>
#endif
#if GWINSZ_IN_SYS_IOCTL
#include <sys/ioctl.h>
#endif
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_SYS_IO_H
#include <sys/io.h>
#endif

#if HAVE_IOPL
#if HAVE_INOUT
#define USE_DIRECT_PORT
#endif
#endif

#if HAVE_BACKTRACE
#if HAVE_BACKTRACE_SYMBOLS
#define USE_BACKTRACE
#endif
#endif

struct target_context {
	unsigned usleep_granularity; /**< Minimun sleep time in microseconds. */

	target_clock_t last; /**< Last clock. */
	target_clock_t init; /**< First clock. */

	unsigned col; /**< Number of columns. 0 if not detectable. */
	unsigned row; /**< Number of rows. 0 if not detectable. */

#ifdef USE_DIRECT_PORT
	adv_bool io_perm_flag; /**< IO Permission granted. */
	adv_bool io_perm_iopl_flag; /**< IO iopl called. */
#endif
};

static struct target_context TARGET;

/***************************************************************************/
/* Init */

adv_error target_init(void)
{
	TARGET.last = 0;
	TARGET.init = 0;
	TARGET.usleep_granularity = 0;
	TARGET.col = 0;
	TARGET.row = 0;

	TARGET.init = target_clock();

#ifdef TIOCGWINSZ
	{
		struct winsize wind_struct;
		if (ioctl(1, TIOCGWINSZ, &wind_struct) == 0) {
			TARGET.col = wind_struct.ws_col;
			TARGET.row = wind_struct.ws_row;
		}
	}
#endif

#ifdef USE_DIRECT_PORT
	TARGET.io_perm_iopl_flag = 0;
	TARGET.io_perm_flag = 0;
#endif

	return 0;
}

void target_done(void)
{
}

/***************************************************************************/
/* Scheduling */

void target_yield(void)
{
#ifdef _POSIX_PRIORITY_SCHEDULING /* OSDEF Check for POSIX scheduling */
	sched_yield();
#endif
}

void target_idle(void)
{
	struct timespec req;
	req.tv_sec = 0;
	req.tv_nsec = 1000000; /* 1 ms */
	nanosleep(&req, 0);
}

void target_usleep_granularity(unsigned us)
{
	TARGET.usleep_granularity = us;
}

void target_usleep(unsigned us)
{
	struct timeval start_tv;
	struct timeval stop_tv;
	struct timespec req;
	unsigned requested;
	unsigned effective;

	/* if too short don't wait */
	if (us <= TARGET.usleep_granularity) {
		return;
	}

	requested = us - TARGET.usleep_granularity;

	req.tv_sec = requested / 1000000;
	req.tv_nsec = (requested % 1000000) * 1000;

	gettimeofday(&start_tv, NULL);
	nanosleep(&req, 0);
	gettimeofday(&stop_tv, NULL);

	effective = (stop_tv.tv_sec - start_tv.tv_sec) * 1000000 + (stop_tv.tv_usec - start_tv.tv_usec);

	if (effective > us) {
		/* don't adjust the granularity, it should be measured by the OS code */
		/* TARGET.usleep_granularity += effective - us; */
		log_std(("WARNING:linux: target_usleep() too long, granularity %d [us] (requested %d, tryed %d, effective %d)\n", TARGET.usleep_granularity, us, requested, effective));
	}
}

/***************************************************************************/
/* Clock */

target_clock_t TARGET_CLOCKS_PER_SEC = 1000000LL;

target_clock_t target_clock(void)
{
	struct timeval tv;
	target_clock_t r;

	gettimeofday(&tv, NULL);
	r = tv.tv_sec * 1000000LL + tv.tv_usec;

	/* on some laptops strange things may happen when the CPU change it's speed */
	/* a back step of 20ms is reported in Linux */
	if (r < TARGET.last)
		r = TARGET.last;

	TARGET.last = r - TARGET.init;

	return TARGET.last;
}

/***************************************************************************/
/* Hardware */

static void dev_port_set(unsigned addr, unsigned value)
{
	int f;
	off_t o;
	size_t s;
	unsigned char c;

	f = open("/dev/port", O_WRONLY);
	if (f == -1) {
		log_std(("ERROR:linux: port_set failed, error %d open(/dev/port), %s\n", errno, strerror(errno)));
		goto err;
	}

	o = lseek(f, addr, SEEK_SET);
	if (o == -1) {
		log_std(("ERROR:linux: port_set failed, error %d in lseek(0x%x) /dev/port\n", errno, addr));
		goto err_close;
	}
	if (o != addr) {
		log_std(("ERROR:linux: port_set failed, erroneous return value %d in lseek(0x%x) /dev/port\n", (int)o, addr));
		goto err_close;
	}

	c = value;

	s = write(f, &c, 1);
	if (s == -1) {
		log_std(("ERROR:linux: port_set failed, error %d in write(0x%x) /dev/port\n", errno, value));
		goto err_close;
	}
	if (s != 1) {
		log_std(("ERROR:linux: port_set failed, erroneous return value %d in write(0x%x) /dev/port\n", (int)s, value));
		goto err_close;
	}

	close(f);

	return;

err_close:
	close(f);
err:
	return;
}

static unsigned dev_port_get(unsigned addr)
{
	int f;
	off_t o;
	size_t s;
	unsigned char c;

	f = open("/dev/port", O_RDONLY);
	if (f == -1) {
		log_std(("ERROR:linux: port_get failed, error %d open(/dev/port), %s\n", errno, strerror(errno)));
		goto err;
	}

	o = lseek(f, addr, SEEK_SET);
	if (o == -1) {
		log_std(("ERROR:linux: port_get failed, error %d in lseek(0x%x) /dev/port\n", errno, addr));
		goto err_close;
	}
	if (o != addr) {
		log_std(("ERROR:linux: port_get failed, erroneous return value %d in lseek(0x%x) /dev/port\n", (int)o, addr));
		goto err_close;
	}

	s = read(f, &c, 1);
	if (s == -1) {
		log_std(("ERROR:linux: port_get failed, error %d in read() /dev/port\n", errno));
		goto err_close;
	}
	if (s != 1) {
		log_std(("ERROR:linux: port_get failed, erroneous return value %d in read() /dev/port\n", (int)s));
		goto err_close;
	}

	close(f);

	return c;

err_close:
	close(f);
err:
	return 0;
}

#ifdef USE_DIRECT_PORT
static adv_bool io_port(void)
{
	/* call iopl at the first use */
	if (!TARGET.io_perm_iopl_flag) {
		TARGET.io_perm_iopl_flag = 1;

		if (iopl(3) == 0) {
			TARGET.io_perm_flag = 1;
			log_std(("linux: iopl(3) success, using direct io port\n"));
		} else {
			TARGET.io_perm_flag = 0;
			log_std(("WARNING:linux: iopl(3) failed, using /dev/port to io port programming\n"));
		}
	}

	return TARGET.io_perm_flag;
}
#endif

void target_port_set(unsigned addr, unsigned value)
{
#ifdef USE_DIRECT_PORT
	if (io_port()) {
		outb(addr, value);
		return;
	}
#endif
	dev_port_set(addr, value);
}

unsigned target_port_get(unsigned addr)
{
#ifdef USE_DIRECT_PORT
	if (io_port()) {
		return inb(addr);
	}
#endif
	return dev_port_get(addr);
}

void target_writeb(unsigned addr, unsigned char c)
{
	log_std(("ERROR:linux: write at address 0x%x not allowed, you must be root\n", addr));
}

unsigned char target_readb(unsigned addr)
{
	log_std(("ERROR:linux: read at address 0x%x not allowed, you must be root\n", addr));
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
	/* nothing */
}

void target_sound_warn(void)
{
	/* nothing */
}

void target_sound_signal(void)
{
	/* nothing */
}

/***************************************************************************/
/* APM */

adv_error target_apm_shutdown(void)
{
	system("/sbin/poweroff");
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

adv_error target_script(const char* script)
{
	char file[FILE_MAXPATH];
	int f;
	int r;

	log_std(("linux: script\n%s\n", script));

	strcpy(file, "/tmp/advscriptXXXXXX");
	f = mkstemp(file);
	if (f == -1) {
		return -1;
	}

	/* set it executable */
	if (fchmod(f, S_IRWXU) != 0) {
		close(f);
		return -1;
	}

	if (write(f, script, strlen(script)) != strlen(script)) {
		close(f);
		return -1;
	}

	close(f);

	r = system(file);

	log_std(("linux: return %d\n", r));

	remove(file); /* ignore error */

	return r;
}

adv_error target_system(const char* cmd)
{
	log_std(("linux: system %s\n", cmd));

	return system(cmd);
}

adv_error target_spawn(const char* file, const char** argv)
{
	int pid, status;
	int i;

	log_std(("linux: spawn %s\n", file));
	for(i=0;argv[i];++i)
		log_std(("linux: spawn arg%d %s\n", i, argv[i]));

	pid = fork();
	if (pid == -1)
		return -1;

	if (pid == 0) {
		execvp(file, (char**)argv);
		exit(127);
	} else {
		while (1) {
			if (waitpid(pid, &status, 0) == -1) {
				if (errno != EINTR) {
					status = -1;
					break;
				}
			} else
				break;
		}

		return status;
	}
}

adv_error target_mkdir(const char* file)
{
	return mkdir(file, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

void target_sync(void)
{
	sync();
}

adv_error target_search(char* path, unsigned path_size, const char* file)
{
	const char* path_env;
	char* path_list;
	char* dir;

	log_std(("linux: target_search(%s)\n", file));

	/* if it's an absolute path */
	if (file[0] == file_dir_slash()) {
		sncpy(path, path_size, file);

		if (access(path, F_OK) == 0) {
			log_std(("linux: target_search() return %s\n", path));
			return 0;
		}

		log_std(("linux: target_search failed\n"));
		return -1;
	}

	/* get the path list */
	path_env = getenv("PATH");
	if (!path_env) {
		log_std(("linux: genenv(PATH) failed\n"));
	} else {
		char separator[2];
		separator[0] = file_dir_separator();
		separator[1] = 0;

		/* duplicate for the strtok use */
		path_list = strdup(path_env);

		dir = strtok(path_list, separator);
		while (dir) {
			sncpy(path, path_size, dir);

			if (!path[0] || path[strlen(path)-1] != file_dir_slash()) {
				char slash[2];
				slash[0] = file_dir_slash();
				slash[1] = 0;
				sncat(path, path_size, slash);
			}

			sncat(path, path_size, file);

			if (access(path, F_OK) == 0) {
				free(path_list);
				log_std(("linux: target_search() return %s\n", path));
				return 0;
			}

			dir = strtok(0, separator);
		}

		free(path_list);
	}

	log_std(("linux: target_search failed\n"));

	return -1;
}

static void target_wrap(FILE* f, unsigned col, char* s)
{
	unsigned i;
	unsigned p;
	adv_bool has_space;

	p = 0;
	i = 0;
	has_space = 1;
	while (s[p]) {
		const char* t;
		char c;

		t = stoken(&c, &p, s, " \r\n", "");

		if (*t) {
			if (i>0 && i + !has_space + strlen(t) >= col) {
				fprintf(f, "\n");
				i = 0;
				has_space = 1;
			} else {
				if (!has_space) {
					fprintf(f, " ");
					++i;
				}
			}

			fprintf(f, "%s", t);
			i += strlen(t);
			has_space = 0;
		}

		switch (c) {
		case '\n' :
			fprintf(f, "\n");
			i = 0;
			has_space = 1;
			break;
		case '\r' :
			fprintf(f, "\r");
			break;
		}
	}
}

void target_out_va(const char* text, va_list arg)
{
	if (TARGET.col) {
		char buffer[4096];
		vsnprintf(buffer, sizeof(buffer), text, arg);
		target_wrap(stdout, TARGET.col, buffer);
	} else {
		vfprintf(stdout, text, arg);
	}
}

void target_err_va(const char* text, va_list arg)
{
	if (TARGET.col) {
		char buffer[4096];
		vsnprintf(buffer, sizeof(buffer), text, arg);
		target_wrap(stderr, TARGET.col, buffer);
	} else {
		vfprintf(stderr, text, arg);
	}
}

void target_nfo_va(const char* text, va_list arg)
{
	target_err_va(text, arg);
}

void target_out(const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_out_va(text, arg);
	va_end(arg);
}

void target_err(const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_err_va(text, arg);
	va_end(arg);
}

void target_nfo(const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	target_nfo_va(text, arg);
	va_end(arg);
}

void target_flush(void)
{
	fflush(stdout);
	fflush(stderr);
}

static void target_backtrace(void)
{
#ifdef USE_BACKTRACE
	void* buffer[256];
	char** symbols;
	int size;
	int i;

	/* The programm need to be compiled without CFLAGS=-fomit-frame-pointer */
	/* and with LDFLAGS=-rdynamic */
	size = backtrace(buffer, 256);

	symbols = backtrace_symbols(buffer, size);

	if (size > 1) {
		printf("Stack backtrace:\n");
		for(i=0;i<size;++i)
			printf("%s\n", symbols[i]);
	} else {
		printf("No stack backtrace: compile without CFLAGS=-fomit-frame-pointer and with LDFLAGS=-rdynamic\n");
	}

	free(symbols);
#endif
}

void target_signal(int signum)
{
	if (signum == SIGINT) {
		fprintf(stderr, "Break\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGQUIT) {
		fprintf(stderr, "Quit\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGTERM) {
		fprintf(stderr, "Terminated\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGALRM) {
		fprintf(stderr, "Alarm\n\r");
		exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Signal %d.\n", signum);
		fprintf(stderr, "%s, %s\n\r", __DATE__, __TIME__);

		target_backtrace();

		if (signum == SIGILL) {
			fprintf(stderr, "Are you using the correct binary ?\n");
		}

		_exit(EXIT_FAILURE);
	}
}

void target_crash(void)
{
	unsigned* i = (unsigned*)0;
	*i = *i;
	abort();
}

const char* target_option_extract(const char* arg)
{
	if (arg[0] != '-')
		return 0;
	if (arg[1] == '-')
		return arg + 2;
	else
		return arg + 1;
}

adv_bool target_option_compare(const char* arg, const char* opt)
{
	const char* name = target_option_extract(arg);
	return name!=0 && strcasecmp(name, opt) == 0;
}

