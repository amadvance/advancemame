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

#include "os.h"
#include "log.h"
#include "target.h"
#include "file.h"
#include "conf.h"
#include "osdos.h"

#ifdef USE_VIDEO
#include "pci.h"
#endif

#include "allegro2.h"

#include <process.h>
#include <conio.h>
#include <crt0.h>
#include <dos.h>
#include <sys/exceptn.h>
#include <go32.h>

struct os_context {
#ifdef USE_CONFIG_ALLEGRO_WRAPPER
	/**
	 * Enable the Allegro compatibility.
	 * This option enable the redirection of all the Allegro configuration options
	 * in the current API.
	 * You must simply set this variable at the configuration context to use
	 * before calling any Allegro functions.
	 * Link with --wrap get_config_string --wrap get_config_int --wrap set_config_string --wrap set_config_int --wrap get_config_id --wrap set_config_id.
	 */
	adv_conf* allegro_conf;
#endif

#ifdef USE_VIDEO
	unsigned pci_vga_vendor; /**< Video board vendor. */
	unsigned pci_vga_device; /**< Video board device. */
#endif
};

static struct os_context OS;

/***************************************************************************/
/* Allegro configuration compatibility */

#ifdef USE_CONFIG_ALLEGRO_WRAPPER

const char* __wrap_get_config_string(const char *section, const char *name, const char *def)
{
	char allegro_name_buffer[256];
	const char* result;

	log_std(("allegro: get_config_string(%s, %s, %s)\n", section, name, def));

	/* disable the emulation of the third mouse button */
	if (strcmp(name, "emulate_three")==0)
		return "no";

	if (!OS.allegro_conf)
		return def;

	snprintf(allegro_name_buffer, sizeof(allegro_name_buffer), "allegro_%s", name);
	if (conf_is_registered(OS.allegro_conf, allegro_name_buffer)
		&& conf_string_section_get(OS.allegro_conf, "", allegro_name_buffer, &result) == 0)
		return result;
	else
		return def;
}

int __wrap_get_config_int(const char *section, const char *name, int def)
{
	char allegro_name_buffer[256];
	const char* result;

	log_std(("allegro: get_config_int(%s, %s, %d)\n", section, name, def));

	if (!OS.allegro_conf)
		return def;
	snprintf(allegro_name_buffer, sizeof(allegro_name_buffer), "allegro_%s", name);
	if (conf_is_registered(OS.allegro_conf, allegro_name_buffer)
		&& conf_string_section_get(OS.allegro_conf, "", allegro_name_buffer, &result) == 0)
		return atoi(result);
	else
		return def;
}

int __wrap_get_config_id(const char *section, const char *name, int def)
{
	char allegro_name_buffer[256];
	const char* result;

	log_std(("allegro: get_config_id(%s, %s, %d)\n", section, name, def));

	if (!OS.allegro_conf)
		return def;

	snprintf(allegro_name_buffer, sizeof(allegro_name_buffer), "allegro_%s", name);
	if (conf_is_registered(OS.allegro_conf, allegro_name_buffer)
		&& conf_string_section_get(OS.allegro_conf, "", allegro_name_buffer, &result) == 0)
	{
		unsigned v;
		v = ((unsigned)(unsigned char)result[3]);
		v |= ((unsigned)(unsigned char)result[2]) << 8;
		v |= ((unsigned)(unsigned char)result[1]) << 16;
		v |= ((unsigned)(unsigned char)result[0]) << 24;
		return v;
	} else
		return def;
}

void __wrap_set_config_string(const char *section, const char *name, const char *val)
{
	char allegro_name_buffer[256];

	log_std(("allegro: set_config_string(%s, %s, %s)\n", section, name, val));

	if (!OS.allegro_conf)
		return;

	snprintf(allegro_name_buffer, sizeof(allegro_name_buffer), "allegro_%s", name);
	if (val) {
		if (!conf_is_registered(OS.allegro_conf, allegro_name_buffer))
			conf_string_register(OS.allegro_conf, allegro_name_buffer);
		conf_string_set(OS.allegro_conf, "", allegro_name_buffer, val); /* ignore error */
	} else {
		conf_remove(OS.allegro_conf, "", allegro_name_buffer);
	}
}

void __wrap_set_config_int(const char *section, const char *name, int val)
{
	char allegro_name_buffer[256];
	char int_buffer[16];

	log_std(("allegro: set_config_int(%s, %s, %d)\n", section, name, val));

	if (!OS.allegro_conf)
		return;

	snprintf(allegro_name_buffer, sizeof(allegro_name_buffer), "allegro_%s", name);
	if (!conf_is_registered(OS.allegro_conf, allegro_name_buffer))
		conf_string_register(OS.allegro_conf, allegro_name_buffer);
	snprintf(int_buffer, sizeof(int_buffer), "%d", val);
	conf_string_set(OS.allegro_conf, "", allegro_name_buffer, int_buffer); /* ignore error */
}

void __wrap_set_config_id(const char *section, const char *name, int val)
{
	char allegro_name_buffer[256];
	char id_buffer[16];

	log_std(("allegro: set_config_id(%s, %s, %d)\n", section, name, val));

	if (!OS.allegro_conf)
		return;
	snprintf(allegro_name_buffer, sizeof(allegro_name_buffer), "allegro_%s", name);
	if (!conf_is_registered(OS.allegro_conf, allegro_name_buffer))
		conf_string_register(OS.allegro_conf, allegro_name_buffer);
	id_buffer[3] = ((unsigned)val) & 0xFF;
	id_buffer[2] = (((unsigned)val) >> 8) & 0xFF;
	id_buffer[1] = (((unsigned)val) >> 16) & 0xFF;
	id_buffer[0] = (((unsigned)val) >> 24) & 0xFF;
	id_buffer[4] = 0;
	conf_string_set(OS.allegro_conf, "", allegro_name_buffer, id_buffer); /* ignore error */
}

#endif

/***************************************************************************/
/* Ticker */

static void ticker_measure(target_clock_t* map, unsigned max)
{
	clock_t start;
	clock_t stop;
	target_clock_t tstart;
	target_clock_t tstop;
	unsigned mac;

	mac = 0;
	start = clock();
	do {
		stop = clock();
	} while (stop == start);

	start = stop;
	tstart = target_clock();
	while (mac < max) {
		do {
			stop = clock();
		} while (stop == start);

		tstop = target_clock();

		map[mac] = (tstop - tstart) * CLOCKS_PER_SEC / (stop - start);
		++mac;

		start = stop;
		tstart = tstop;
	}
}

static int ticker_cmp(const void *e1, const void *e2)
{
	const target_clock_t* t1 = (const target_clock_t*)e1;
	const target_clock_t* t2 = (const target_clock_t*)e2;

	if (*t1 < *t2) return -1;
	if (*t1 > *t2) return 1;
	return 0;
}

static void ticker_setup(void)
{
	target_clock_t v[7];
	double error;
	int i;

	ticker_measure(v, 7);

	qsort(v, 7, sizeof(target_clock_t), ticker_cmp);

	for(i=0;i<7;++i)
		log_std(("os: clock estimate %g\n", (double)v[i]));

	TARGET_CLOCKS_PER_SEC = v[3]; /* median value */

	if (v[0])
		error = (v[6] - v[0]) / (double)v[0];
	else
		error = 0;

	log_std(("os: select clock %g (err %g%%)\n", (double)v[3], error * 100.0));
}

/***************************************************************************/
/* Init */

int os_init(adv_conf* context)
{
	memset(&OS, 0, sizeof(OS));

#ifdef USE_CONFIG_ALLEGRO_WRAPPER
	OS.allegro_conf = context;
#endif

	return 0;
}

void os_done(void)
{
#ifdef USE_CONFIG_ALLEGRO_WRAPPER
	OS.allegro_conf = 0;
#endif
}

static void os_align(void)
{
	char* m[32];
	unsigned i;

	/* align test */
	for(i=0;i<32;++i) {
		m[i] = (char*)malloc(i);
		if (((unsigned)m[i]) & 0x7)
			log_std(("ERROR:os: unaligned malloc ptr:%p, size:%d\n", (void*)m[i], i));
	}

	for(i=0;i<32;++i) {
		free(m[i]);
	}
}

#ifdef USE_VIDEO
static int pci_scan_device_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* arg)
{
	DWORD dw;
	unsigned base_class;
	unsigned subsys_card;
	unsigned subsys_vendor;

	(void)arg;

	if (pci_read_dword(bus_device_func, 0x8, &dw)!=0)
		return 0;

	base_class = (dw >> 24) & 0xFF;
	if (base_class != 0x3 /* PCI_CLASS_DISPLAY */)
		return 0;

	if (pci_read_dword(bus_device_func, 0x2c, &dw)!=0)
		return 0;

	subsys_vendor = dw & 0xFFFF;
	subsys_card = (dw >> 16) & 0xFFFF;

	log_std(("os: found pci display vendor:%04x, device:%04x, subsys_vendor:%04x, subsys_card:%04x\n", vendor, device, subsys_vendor, subsys_card));

	OS.pci_vga_vendor = vendor;
	OS.pci_vga_device = device;

	return 0;
}
#endif

adv_bool os_internal_brokenint10_active(void)
{
#ifdef USE_VIDEO
	/* ATI bios is broken. Tested with a Rage 128 (0x50xx) and a Radeon 7000 (0x51xx). */
	/* Also a simple "mode co80" freeze the PC. */
	/* Allow ATI Rage (not 128) and ATI Match cards */

	unsigned dh = (OS.pci_vga_device >> 8) & 0xFF;

	return OS.pci_vga_vendor == 0x1002 /* ATI */
		&& (dh != 0x43 && dh != 0x45 && dh != 0x46 && dh != 0x47 && dh != 0x56);
/*
	0x43 Mach64
	0x45 Mach64
	0x46 Mach64
	0x47 Mach64 / Rage (not 128)
	0x56 Mach64
*/
#else
	return 0;
#endif
}

int os_inner_init(const char* title)
{
	log_std(("os: sys DOS\n"));

	/* print the compiler version */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__) /* OSDEF Detect compiler version */
#define COMPILER_RESOLVE(a) #a
#define COMPILER(a, b, c) COMPILER_RESOLVE(a) "." COMPILER_RESOLVE(b) "." COMPILER_RESOLVE(c)
	log_std(("os: compiler GNU %s\n", COMPILER(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)));
#else
	log_std(("os: compiler unknown\n"));
#endif

#ifdef USE_LSB
	log_std(("os: compiled little endian system\n"));
#else
	#error DOS version cannot be big endian
#endif

	ticker_setup();

	log_std(("os: allegro_init()\n"));
	if (allegro_init() != 0) {
		log_std(("os: allegro_init() failed\n"));
		target_err("Error initializing the ALLEGRO library.\n");
		return -1;
	}

	os_align();

#ifdef USE_VIDEO
	OS.pci_vga_vendor = 0;
	OS.pci_vga_device = 0;
	if (pci_scan_device(pci_scan_device_callback, 0)!=0) {
		log_std(("ERROR:os: error scanning pci display device, resume and continue\n"));
	}
#endif

	/* set some signal handlers */
	signal(SIGABRT, (void (*)(int))os_signal);
	signal(SIGFPE, (void (*)(int))os_signal);
	signal(SIGILL, (void (*)(int))os_signal);
	signal(SIGINT, (void (*)(int))os_signal);
	signal(SIGSEGV, (void (*)(int))os_signal);
	signal(SIGTERM, (void (*)(int))os_signal);
	signal(SIGQUIT, (void (*)(int))os_signal);

	return 0;
}

void os_inner_done(void)
{
	allegro_exit();
}

void os_poll(void)
{
}

void os_fire(void)
{
}

/***************************************************************************/
/* Signal */

int os_is_quit(void)
{
	return 0;
}

void os_default_signal(int signum, void* info, void* context)
{
	log_std(("os: signal %d\n", signum));

#if defined(USE_VIDEO)
	log_std(("os: adv_video_abort\n"));
	{
		extern void adv_video_abort(void);
		adv_video_abort();
	}
#endif

#if defined(USE_SOUND)
	log_std(("os: sound_abort\n"));
	{
		extern void soundb_abort(void);
		soundb_abort();
	}
#endif

	target_mode_reset();

	log_std(("os: close log\n"));
	log_abort();

	target_signal(signum, info, context);
}

/***************************************************************************/
/* Main */

static int os_fixed(void)
{
	__dpmi_regs r;
	void* t;

	/* Fix an alignment problem of the DJGPP Library 2.03 (refresh) */
	t = sbrk(0);
	if (((unsigned)t & 0x7) == 4) {
		sbrk(4);
	}

	/* Don't allow NT */
	r.x.ax = 0x3306;
	__dpmi_int(0x21, &r);
	if (r.x.bx == ((50 << 8) | 5)) {
		cprintf("Windows NT/2000/XP not supported.\n\r");
		return -1;
	}

	return 0;
}

/* Lock all the DATA, BSS and STACK segments. (see the DJGPP faq point 18.9) */
int _crt0_startup_flags = _CRT0_FLAG_NONMOVE_SBRK | _CRT0_FLAG_LOCK_MEMORY | _CRT0_FLAG_NEARPTR;

int main(int argc, char* argv[])
{
	/* allocate the HEAP in the pageable memory */
	_crt0_startup_flags &= ~_CRT0_FLAG_LOCK_MEMORY;

	if (os_fixed() != 0)
		return EXIT_FAILURE;

	if (target_init() != 0)
		return EXIT_FAILURE;

	if (file_init() != 0) {
		target_done();
		return EXIT_FAILURE;
	}

	if (os_main(argc, argv) != 0) {
		file_done();
		target_done();
		return EXIT_FAILURE;
	}

	file_done();
	target_done();

	return EXIT_SUCCESS;
}

