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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for EIP in ucontext.h */
#endif

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
#if HAVE_UCONTEXT_H
#include <ucontext.h>
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

#ifdef __i386__
#if HAVE_UCONTEXT_H
#define USE_BACKTRACE_REG
#endif
#endif

struct target_context {
	unsigned usleep_granularity; /**< Minimun sleep time in microseconds. */

	unsigned col; /**< Number of columns. 0 if not detectable. */
	unsigned row; /**< Number of rows. 0 if not detectable. */

#ifdef USE_DIRECT_PORT
	adv_bool io_perm_flag; /**< IO Permission granted. */
	adv_bool io_perm_iopl_flag; /**< IO iopl called. */
#endif

	adv_bool io_dev_port_flag; /**< /dev/port granted. */
};

static struct target_context TARGET;

/***************************************************************************/
/* Init */

adv_error target_init(void)
{
	TARGET.usleep_granularity = 0;
	TARGET.col = 0;
	TARGET.row = 0;

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

	TARGET.io_dev_port_flag = 1;

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

	return r;
}

/***************************************************************************/
/* Hardware */

static void dev_port_set(unsigned addr, unsigned value)
{
	int f;
	off_t o;
	size_t s;
	unsigned char c;

	if (!TARGET.io_dev_port_flag)
		goto err;

	f = open("/dev/port", O_WRONLY);
	if (f == -1) {
		TARGET.io_dev_port_flag = 0;
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

	if (!TARGET.io_dev_port_flag)
		goto err;

	f = open("/dev/port", O_RDONLY);
	if (f == -1) {
		TARGET.io_dev_port_flag = 0;
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
	log_debug(("linux: port_set(0x%x,0x%02x)\n", addr, value));

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
	unsigned v;

#ifdef USE_DIRECT_PORT
	if (io_port()) {
		v = inb(addr);
		log_std(("linux: port_get(0x%x) = 0x%02x\n", addr, v));
		return v;
	}
#endif

	v = dev_port_get(addr);

	log_debug(("linux: port_get(0x%x) = 0x%02x\n", addr, v));

	return v;
}

void target_writeb(unsigned addr, unsigned char c)
{
	log_std(("ERROR:linux: write at address 0x%x not allowed in this architecture\n", addr));
}

unsigned char target_readb(unsigned addr)
{
	log_std(("ERROR:linux: read at address 0x%x not allowed in this architecture\n", addr));
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
	int r;

	r = execl("/sbin/poweroff", "/sbin/poweroff", NULL);

	if (!WIFEXITED(r) || WEXITSTATUS(r) != 0)
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

adv_error target_script(const char* script)
{
	char file[FILE_MAXPATH];
	int f;
	int r;
	uid_t euid;
	gid_t egid;

	log_std(("linux: script\n%s\n", script));

	/* get the effective user/group id */
	euid = geteuid();
	egid = getegid();

	/* set the real user id, prevent chroot programs to propagate permissions */
	if (seteuid(getuid()) != 0) {
		log_std(("ERROR:linux: script seteuid(getuid()) failed\n"));
		goto err;
	}

	/* set the real group id, prevent chroot programs to propagate permissions */
	if (setegid(getgid()) != 0) {
		log_std(("ERROR:linux: script setegid(getgid()) failed\n"));
		goto err;
	}

	strcpy(file, "/tmp/advscriptXXXXXX");
	f = mkstemp(file);
	if (f == -1) {
		log_std(("ERROR:linux: mkstemp() failed\n"));
		goto err_priv;
	}

	/* set it executable */
	if (fchmod(f, S_IRWXU) != 0) {
		log_std(("ERROR:linux: script fchmod() failed\n"));
		goto err_close;
	}

	if (write(f, script, strlen(script)) != strlen(script)) {
		log_std(("ERROR:linux: script write() failed\n"));
		goto err_close;
	}

	if (close(f) != 0) {
		log_std(("ERROR:linux: script close()e failed\n"));
		goto err_priv;
	}

	r = system(file);

	log_std(("linux: system(script) return %d\n", r));

	remove(file); /* ignore error */

	/* restore privileges */
	if (seteuid(euid) != 0) {
		log_std(("ERROR:linux: script script seteuid(%d) failed\n", (int)euid));
	}
	if (setegid(egid) != 0) {
		log_std(("ERROR:linux: script script setegid(%d) failed\n", (int)egid));
	}

	return r;

err_close:
	close(f);
err_priv:
	/* restore privileges */
	if (seteuid(euid) != 0) {
		log_std(("ERROR:linux: script script seteuid(%d) failed\n", (int)euid));
	}
	if (setegid(egid) != 0) {
		log_std(("ERROR:linux: script script setegid(%d) failed\n", (int)egid));
	}
err:
	return -1;
}

adv_error target_spawn_redirect(const char* file, const char** argv, const char* output)
{
	int r;
	int i;
	int p;

	log_std(("linux: spawn_redirect %s\n", file));
	for(i=0;argv[i];++i)
		log_std(("linux: spawn_redirect arg%d %s\n", i, argv[i]));
	log_std(("linux: spawn_redirect input %s\n", output));

	p = fork();
	if (p == -1) {
		log_std(("ERROR:linux: spawn_redirect fork() failed\n"));
		return -1;
	}

	if (p == 0) {
		int f;

		/* open the output file */
		f = open(output, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (f == -1) {
			log_std(("ERROR:linux: spawn_redirect open failed\n"));
			exit(127);
		}

		/* remap the output stream */
		if (dup2(f, STDOUT_FILENO) == -1) {
			log_std(("ERROR:linux: spawn_redirect dup2 failed\n"));
			exit(127);
		}

		close(f);

		/* set the real user id, prevent chroot programs to propagate permissions */
		if (seteuid(getuid()) != 0) {
			log_std(("ERROR:linux: spawn seteuid(getuid()) failed\n"));
			exit(127);
		}

		/* set the real group id, prevent chroot programs to propagate permissions */
		if (setegid(getgid()) != 0) {
			log_std(("ERROR:linux: spawn setegid(getgid()) failed\n"));
			exit(127);
		}

		/* exec the program */
		execvp(file, (char**)argv);

		exit(127);
	} else {
		while (1) {
			if (waitpid(p, &r, 0) == -1) {
				if (errno != EINTR) {
					r = -1;
					break;
				}
			} else
				break;
		}

		log_std(("linux: spawn_redirect return %d\n", r));

		return r;
	}
}

adv_error target_spawn(const char* file, const char** argv)
{
	int r;
	int i;
	int p;

	log_std(("linux: spawn %s\n", file));
	for(i=0;argv[i];++i)
		log_std(("linux: spawn arg%d %s\n", i, argv[i]));

	p = fork();
	if (p == -1) {
		log_std(("ERROR:linux: spawn fork() failed\n"));
		return -1;
	}

	if (p == 0) {
		/* set the real user id, prevent chroot programs to propagate permissions */
		if (seteuid(getuid()) != 0) {
			log_std(("ERROR:linux: spawn seteuid(getuid()) failed\n"));
			exit(127);
		}

		/* set the real group id, prevent chroot programs to propagate permissions */
		if (setegid(getgid()) != 0) {
			log_std(("ERROR:linux: spawn setegid(getgid()) failed\n"));
			exit(127);
		}

		/* exec the program */
		execvp(file, (char**)argv);

		exit(127);
	} else {
		while (1) {
			if (waitpid(p, &r, 0) == -1) {
				if (errno != EINTR) {
					r = -1;
					break;
				}
			} else
				break;
		}

		log_std(("linux: spawn return %d\n", r));

		return r;
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
	int p;
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

void target_signal(int signum, void* void_info, void* void_context)
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
#ifdef USE_BACKTRACE
		void* buffer[32];
		char** symbols;
		int size;
		int i;
		void* fault;
		void* caller;
		siginfo_t* info = void_info;

		if (info) {
			fault = info->si_addr;
		} else {
			fault = 0;
		}

#ifdef USE_BACKTRACE_REG
		if (void_context) {
			ucontext_t* context = void_context;
			caller = (void*)context->uc_mcontext.gregs[REG_EIP];
		} else {
			caller = 0;
		}
#else
		caller = 0;
#endif

		switch (signum) {
		case SIGILL : fprintf(stderr, "Signal SIGILL"); break;
		case SIGFPE : fprintf(stderr, "Signal SIGFPE"); break;
		case SIGSEGV : fprintf(stderr, "Signal SIGSEGV"); break;
		case SIGBUS : fprintf(stderr, "Signal SIGBUS"); break;
		case SIGABRT : fprintf(stderr, "Signal SIGABRT"); break;
		default : fprintf(stderr, "Signal %d", signum); break;
		}

		if (signum == SIGSEGV) {
			if (info) {
				switch (info->si_code) {
#ifdef SEGV_MAPERR
				case SEGV_MAPERR : fprintf(stderr, "[MAPERR]"); break;
#endif
#ifdef SEGV_ACCERR
				case SEGV_ACCERR : fprintf(stderr, "[ACCERR]"); break;
#endif
				default : fprintf(stderr, "[%xh]", (unsigned)info->si_code); break;
				}
			}
			fprintf(stderr, ", fault at %p, from code at %p\n", fault, caller);
		} else if (signum == SIGILL) {
			if (info) {
				switch (info->si_code) {
#ifdef ILL_ILLOPC
				case ILL_ILLOPC : fprintf(stderr, "[ILLOPC]"); break;
#endif
#ifdef ILL_ILLOPN
				case ILL_ILLOPN : fprintf(stderr, "[ILLOPN]"); break;
#endif
#ifdef ILL_ILLADR
				case ILL_ILLADR : fprintf(stderr, "[ILLADR]"); break;
#endif
#ifdef ILL_ILLTRP
				case ILL_ILLTRP : fprintf(stderr, "[ILLTRP]"); break;
#endif
#ifdef ILL_PRVOPC
				case ILL_PRVOPC : fprintf(stderr, "[PRVOPC]"); break;
#endif
#ifdef ILL_PRVREG
				case ILL_PRVREG : fprintf(stderr, "[PRVREG]"); break;
#endif
#ifdef ILL_COPROC
				case ILL_COPROC : fprintf(stderr, "[COPROC]"); break;
#endif
#ifdef ILL_BADSTK
				case ILL_BADSTK : fprintf(stderr, "[BADSTK]"); break;
#endif
				default : fprintf(stderr, "[%xh]", (unsigned)info->si_code); break;
				}
			}
			fprintf(stderr, ", fault at %p, from code at %p\n", fault, caller);
		} else if (signum == SIGBUS) {
			if (info) {
				switch (info->si_code) {
#ifdef BUS_ADRALN
				case BUS_ADRALN : fprintf(stderr, "[ADRALN]"); break;
#endif
#ifdef BUS_ADRERR
				case BUS_ADRERR : fprintf(stderr, "[ADRERR]"); break;
#endif
#ifdef BUS_OBJERR
				case BUS_OBJERR : fprintf(stderr, "[OBJERR]"); break;
#endif
				default : fprintf(stderr, "[%xh]", (unsigned)info->si_code); break;
				}
			}
			fprintf(stderr, ", fault at %p, from code at %p\n", fault, caller);
		} else if (signum == SIGFPE) {
			if (info) {
				switch (info->si_code) {
#ifdef FPE_INTDIV
				case FPE_INTDIV : fprintf(stderr, "[INTDIV]"); break;
#endif
#ifdef FPE_INTOVF
				case FPE_INTOVF : fprintf(stderr, "[INTOVF]"); break;
#endif
#ifdef FPE_FLTDIV
				case FPE_FLTDIV : fprintf(stderr, "[FLTDIV]"); break;
#endif
#ifdef FPE_FLTOVF
				case FPE_FLTOVF : fprintf(stderr, "[FLTOVF]"); break;
#endif
#ifdef FPE_FLTUND
				case FPE_FLTUND : fprintf(stderr, "[FLTUND]"); break;
#endif
#ifdef FPE_FLTRES
				case FPE_FLTRES : fprintf(stderr, "[FLTRES]"); break;
#endif
#ifdef FPE_FLTINV
				case FPE_FLTINV : fprintf(stderr, "[FLTINV]"); break;
#endif
#ifdef FPE_FLTSUB
				case FPE_FLTSUB : fprintf(stderr, "[FLTSUB]"); break;
#endif
				default : fprintf(stderr, "[%xh]", (unsigned)info->si_code); break;
				}
			}
			fprintf(stderr, ", fault at %p, from code at %p\n", fault, caller);
		} else {
			if (info) {
				fprintf(stderr, "[%xh]", (unsigned)info->si_code);
			}
			fprintf(stderr, ", from code at %p\n", caller);
		}

#ifdef USE_BACKTRACE_REG
		{
			ucontext_t* context = void_context;
			fprintf(stderr, "eax %08x ebx %08x ecx %08x edx %08x esi %08x edi %08x\n", context->uc_mcontext.gregs[REG_EAX], context->uc_mcontext.gregs[REG_EBX], context->uc_mcontext.gregs[REG_ECX], context->uc_mcontext.gregs[REG_EDX], context->uc_mcontext.gregs[REG_ESI], context->uc_mcontext.gregs[REG_EDI]);
			fprintf(stderr, "ebp %08x esp %08x eip %08x efl %08x err %08x trp %08x\n", context->uc_mcontext.gregs[REG_EBP], context->uc_mcontext.gregs[REG_ESP], context->uc_mcontext.gregs[REG_EIP], context->uc_mcontext.gregs[REG_EFL], context->uc_mcontext.gregs[REG_ERR], context->uc_mcontext.gregs[REG_TRAPNO]);
		}
#endif

		fprintf(stderr, "Compiled %s, %s\n", __DATE__, __TIME__);

		size = backtrace(buffer + 1, 32 - 1) + 1;

		buffer[0] = caller;

		symbols = backtrace_symbols(buffer, size);

		if (size > 1) {
			printf("Stack backtrace:\n");
			for(i=0;i<size;++i) {
				printf("%s\n", symbols[i]);
			}
		} else {
			printf("No stack backtrace: compile without CFLAGS=-fomit-frame-pointer and with LDFLAGS=-rdynamic\n");
		}

		free(symbols);
#else
		fprintf(stderr, "Signal %d.\n", signum);
		fprintf(stderr, "Compiled %s, %s\n\r", __DATE__, __TIME__);
#endif
		if (signum == SIGILL) {
			fprintf(stderr, "Are you using the correct binary ?\n");
		}

		_exit(EXIT_FAILURE);
	}
}

void target_crash(void)
{
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

