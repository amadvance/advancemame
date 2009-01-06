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
#include "log.h"
#include "file.h"
#include "snstring.h"
#include "osdos.h"

#ifdef USE_VIDEO
#include "scrvga.h"
#endif

#include "allegro2.h"

#include <process.h>
#include <sys/exceptn.h>
#include <sys/farptr.h>
#include <go32.h>
#include <dpmi.h>
#include <dos.h>
#include <conio.h>

struct target_context {
#ifdef USE_VIDEO
	struct vga_regs vga_original;
#endif
};

static struct target_context TARGET;

/***************************************************************************/
/* Init */

adv_error target_init(void)
{
#ifdef USE_VIDEO
	vga_mode_get(&TARGET.vga_original);
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
	/* clear the keyboard BIOS buffer */
	while (kbhit())
		getkey();
}

void target_idle(void)
{
	target_yield();
}

void target_usleep(unsigned us)
{
}

/***************************************************************************/
/* Clock */

target_clock_t TARGET_CLOCKS_PER_SEC;

target_clock_t target_clock(void)
{
	target_clock_t r;

	__asm__ __volatile__ (
		"rdtsc"
		: "=A" (r)
	);

	return r;
}

/***************************************************************************/
/* Hardware */

void target_port_set(unsigned addr, unsigned value)
{
	outportb(addr, value);
}

unsigned target_port_get(unsigned addr)
{
	return inportb(addr);
}

void target_writeb(unsigned addr, unsigned char c)
{
	_farpokeb(_dos_ds, addr, c);
}

unsigned char target_readb(unsigned addr)
{
	return _farpeekb(_dos_ds, addr);
}

/***************************************************************************/
/* Mode */

void target_mode_reset(void)
{
#ifdef USE_VIDEO
	/* restore the original video mode */
	struct vga_info info;

	/* normal behaviour for correct BIOS */
	if (!os_internal_brokenint10_active()) {
		__dpmi_regs r;
		r.x.ax = 0x3;
		__dpmi_int(0x10, &r);
		return;
	}

#ifdef USE_VIDEO_SVGALINE
	/* restore the SVGA */
	os_internal_svgaline_mode_reset();
#endif

	/* restore the VGA */
	vga_mode_set(&TARGET.vga_original);
	vga_regs_info_get(&TARGET.vga_original, &info);

	if (!info.is_graphics_mode) {
		unsigned i;

		if (info.char_size_y>=16) {
			vga_font_copy(vga_font_bios_16, 16, 0, 1);
		} else if (info.char_size_y>=14) {
			vga_font_copy(vga_font_bios_14, 14, 0, 1);
		} else {
			vga_font_copy(vga_font_bios_8, 8, 0, 1);
		}

		vga_palette_raw_set(vga_palette_bios_text, 0, 256);

		for(i=0;i<info.memory_size;i+=2) {
			vga_writeb(info.memory_address + i, 0);
			vga_writeb(info.memory_address + i + 1, 0x7);
		}
	} else {
		vga_palette_raw_set(vga_palette_bios_graph, 0, 256);
	}
#endif
}

/***************************************************************************/
/* Sound */

void target_sound_error(void)
{
	sound(100);
	delay(120);
	nosound();
}

void target_sound_warn(void)
{
	sound(900);
	delay(10);
	nosound();
}

void target_sound_signal(void)
{
	unsigned i;
	for(i=0;i<10;++i) {
		sound(800);
		delay(10);
		nosound();
		delay(10);
	}
}

/***************************************************************************/
/* APM */

adv_error target_apm_shutdown(void)
{
	__dpmi_regs regs;

	sync();

	/* APM detect */

	regs.x.ax = 0x5300;
	regs.x.bx = 0x0000;
	__dpmi_int(0x15, &regs);
	if ((regs.x.flags & 1)!=0 || regs.x.bx != 0x504D) {
		/* APM BIOS not found */
		return -1;
	}

	if (regs.x.ax < 0x102) {
		/* APM BIOS too old */
		return -1;
	}

	/* APM connection */
	regs.x.ax = 0x5301;
	regs.x.bx = 0x0000;
	__dpmi_int(0x15, &regs);
	if ((regs.x.flags & 1) != 0) {
		/* APM real mode connection failed */
		return -1;
	}

	/* APM notify version */
	regs.x.ax = 0x530E;
	regs.x.bx = 0x0000;
	regs.x.cx = 0x0102;
	__dpmi_int(0x15, &regs);
	if ((regs.x.flags & 1) != 0) {
		/* APM notify version failed */
	}

	/* APM off */
	regs.x.ax = 0x5307;
	regs.x.bx = 0x0001;
	regs.x.cx = 0x0003;
	__dpmi_int(0x15, &regs);

	return -1;
}

adv_error target_apm_standby(void)
{
	unsigned mode;
	__dpmi_regs r;

	if (os_internal_brokenint10_active())
		return -1;

	r.x.ax = 0x4F10;
	r.h.bl = 0x00;
	r.x.dx = 0;
	r.x.es = 0;

	__dpmi_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		return -1;
	}

	if (r.h.bh & 0x4) {
		mode = 0x4; /* off */
	} else if (r.h.bh & 0x2) {
		mode = 0x2; /* suspend */
	} else if (r.h.bh & 0x1) {
		mode = 0x1; /* standby */
	} else {
		return -1;
	}

	r.x.ax = 0x4F10;
	r.h.bl = 0x01;
	r.h.bh = mode;

	__dpmi_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		return -1;
	}

	return 0;
}

adv_error target_apm_wakeup(void)
{
	__dpmi_regs r;

	if (os_internal_brokenint10_active())
		return -1;

	r.x.ax = 0x4F10;
	r.h.bl = 0x00;
	r.x.dx = 0;
	r.x.es = 0;

	__dpmi_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		return -1;
	}

	r.x.ax = 0x4F10;
	r.h.bl = 0x01;
	r.h.bh = 0x00;

	__dpmi_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		return -1;
	}

	return 0;
}

/***************************************************************************/
/* System */

adv_error target_script(const char* script)
{
	char* tmp;
	char file[FILE_MAXPATH];
	FILE* f;
	int r;

	log_std(("dos: script\n%s\n", script));

	tmp = getenv("TMP");
	if (!tmp)
		tmp = getenv("TEMP");
	if (!tmp) {
		log_std(("ERROR:dos: getenv(TMP,TEMP) failed\n"));
		return -1;
	}

	sncpy(file, FILE_MAXPATH, tmp);
	if (file[0] && file[strlen(file)-1] != '\\')
		sncat(file, FILE_MAXPATH, "\\");
	sncat(file, FILE_MAXPATH, "advs0000.bat");

	log_std(("dos: file %s\n", file));

	f = fopen(file, "w");
	if (!f) {
		log_std(("dos: fopen(%s) failed\n", file));
		goto err;
	}

	if (fprintf(f, "%s", script) < 0) {
		log_std(("dos: fprintf() failed\n"));
		goto err_close;
	}

	if (fclose(f) != 0) {
		log_std(("dos: fclose() failed\n"));
		goto err;
	}

	__djgpp_exception_toggle();
	r = system(file);
	__djgpp_exception_toggle();

	log_std(("dos: return %d\n", r));

	remove(file); /* ignore error */

	return r;

err_close:
	fclose(f);
err:
	return -1;
}

#define EXEC_MAX 1024

adv_error target_spawn_redirect(const char* file, const char** argv, const char* output)
{
	int r;
	char cmdline[EXEC_MAX];
	unsigned i;

	*cmdline = 0;

	for(i=0;argv[i];++i) {
		if (i)
			sncat(cmdline, EXEC_MAX, " ");
		sncat(cmdline, EXEC_MAX, argv[i]);
	}

	sncat(cmdline, EXEC_MAX, " > ");
	sncat(cmdline, EXEC_MAX, output);

	__djgpp_exception_toggle();
	r = system(cmdline);
	__djgpp_exception_toggle();

	return r;
}

adv_error target_spawn(const char* file, const char** argv)
{
	int r;

	__djgpp_exception_toggle();
	r = spawnvp(P_WAIT, file, (char**)argv);
	__djgpp_exception_toggle();

	return r;
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

	log_std(("dos: target_search(%s)\n", file));

	/* if it's an absolute path */
	if (file[0] == '/' || file[0] == file_dir_slash() || (file[0] != 0 && file[1] == ':')) {
		sncpy(path, path_size, file);

		if (access(path, F_OK) == 0) {
			log_std(("dos: target_search() return %s\n", path));
			return 0;
		}

		log_std(("dos: target_search failed\n"));
		return -1;
	}

	/* search in the current directory */
	if (getcwd(path, path_size) == 0) {
		log_std(("dos: getcwd() failed\n"));
	} else {
		unsigned i;

		/* convert to the DOS slash */
		for(i=0;path[i];++i)
			if (path[i] == '/')
				path[i] = file_dir_slash();

		/* add the leading slash */
		if (!path[0] || path[strlen(path)-1] != file_dir_slash()) {
			char slash[2];
			slash[0] = file_dir_slash();
			slash[1] = 0;
			sncat(path, path_size, slash);
		}

		sncat(path, path_size, file);

		if (access(path, F_OK) == 0) {
			log_std(("dos: target_search() return %s\n", path));
			return 0;
		}
	}

	/* get the path list */
	path_env = getenv("PATH");
	if (!path_env) {
		log_std(("dos: genenv(PATH) failed\n"));
	} else {
		char separator[2];
		separator[0] = file_dir_separator();
		separator[1] = 0;

		/* duplicate for the strtok use */
		path_list = strdup(path_env);

		dir = strtok(path_list, separator);
		while (dir) {
			unsigned i;
			sncpy(path, path_size, dir);

			/* convert to the DOS slash */
			for(i=0;path[i];++i)
				if (path[i] == '/')
					path[i] = file_dir_slash();

			if (!path[0] || path[strlen(path)-1] != file_dir_slash()) {
				char slash[2];
				slash[0] = file_dir_slash();
				slash[1] = 0;
				sncat(path, path_size, slash);
			}

			sncat(path, path_size, file);

			if (access(path, F_OK) == 0) {
				free(path_list);
				log_std(("dos: target_search() return %s\n", path));
				return 0;
			}

			dir = strtok(0, separator);
		}

		free(path_list);
	}

	log_std(("dos: target_search failed\n"));

	return -1;
}

void target_out_va(const char* text, va_list arg)
{
	vfprintf(stdout, text, arg);
}

void target_err_va(const char *text, va_list arg)
{
	vfprintf(stdout, text, arg);
}

void target_nfo_va(const char *text, va_list arg)
{
	vfprintf(stdout, text, arg);
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
}

void target_signal(int signum, void* info, void* context)
{
	if (signum == SIGINT) {
		cprintf("Break\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGTERM) {
		cprintf("Terminated\n\r");
		exit(EXIT_FAILURE);
	} else if (signum == SIGQUIT) {
		cprintf("Quit\n\r");
		exit(EXIT_FAILURE);
	} else {

		switch (signum) {
		case SIGILL : cprintf("Signal SIGILL\n\r"); break;
		case SIGFPE : cprintf("Signal SIGFPE\n\r"); break;
		case SIGSEGV : cprintf("Signal SIGSEGV\n\r"); break;
		case SIGABRT : cprintf("Signal SIGABRT\n\r"); break;
		default : cprintf("Signal %d\n\r", signum); break;
		}
		cprintf("Compiled %s, %s\n\r", __DATE__, __TIME__);

		__djgpp_traceback_exit(signum);

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

