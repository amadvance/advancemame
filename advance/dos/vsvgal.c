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

#include "vsvgal.h"
#include "video.h"
#include "log.h"
#include "error.h"

#include "libdos.h"

#include <sys/mman.h>
#include <sys/nearptr.h>
#include <signal.h>
#include <unistd.h>
#include <dos.h>

/***************************************************************************/
/* State */

struct svgaline_chipset_struct {
	DriverSpecs* drv;
	int chipset;
	const char* name;
	unsigned cap;
};

typedef struct svgaline_internal_struct {
	int active;
	int mode_active;

	struct svgaline_chipset_struct* driver;
	unsigned char driver_regs[MAX_REGS];

	unsigned linear_base;
	unsigned linear_size;
	void* linear_pointer;
} svgaline_internal;

static svgaline_internal svgaline_state;

unsigned char* (*svgaline_write_line)(unsigned y);

/***************************************************************************/
/* Options */

struct svgaline_option_struct {
	adv_bool initialized;
	int divide_clock;
};

static struct svgaline_option_struct svgaline_option;

/***************************************************************************/
/* Internal */

static unsigned char* svgaline_linear_write_line(unsigned y) {
	return (unsigned char*)svgaline_state.linear_pointer + libdos_mode.bytes_per_scanline * y;
}

/* Short names for the most common flags */
#define FLAGS_ALL (VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL)
#define FLAGS_NOTV (FLAGS_ALL & ~(VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVPAL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVNTSC))
#define FLAGS_NOTVINTERLACE (FLAGS_NOTV & ~(VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE))

static struct svgaline_chipset_struct cards[] = {
#ifdef INCLUDE_NV3_DRIVER
	{ &__svgalib_nv3_driverspecs, NV3, "nv3", FLAGS_ALL },
#endif
#ifdef INCLUDE_TRIDENT_DRIVER
	{ &__svgalib_trident_driverspecs, TRIDENT, "trident", FLAGS_NOTV },
#endif
#ifdef INCLUDE_RENDITION_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	{ &__svgalib_rendition_driverspecs, RENDITION, "rendition", FLAGS_NOTVINTERLACE },
#endif
#ifdef INCLUDE_G400_DRIVER
	{ &__svgalib_g400_driverspecs, G400, "g400", FLAGS_NOTV },
#endif
#ifdef INCLUDE_PM2_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	{ &__svgalib_pm2_driverspecs, PM2, "pm2", FLAGS_NOTVINTERLACE },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ &__svgalib_savage_driverspecs, SAVAGE, "savage", FLAGS_NOTV },
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER
	{ &__svgalib_mil_driverspecs, MILLENNIUM, "millenium", FLAGS_NOTV },
#endif
#ifdef INCLUDE_R128_DRIVER
	{ &__svgalib_r128_driverspecs, R128, "r128", FLAGS_NOTV },
#endif
#ifdef INCLUDE_BANSHEE_DRIVER
	{ &__svgalib_banshee_driverspecs, BANSHEE, "banshee", FLAGS_NOTV },
#endif
#ifdef INCLUDE_SIS_DRIVER
	{ &__svgalib_sis_driverspecs, SIS, "sis", FLAGS_NOTV },
#endif
#ifdef INCLUDE_I740_DRIVER
	/* A comment in the driver report that interlaced modes don't work  */
	{ &__svgalib_i740_driverspecs, I740, "i740", FLAGS_NOTVINTERLACE },
#endif
#ifdef INCLUDE_I810_DRIVER
	/* A comment in the driver report that interlaced modes don't work  */
	{ &__svgalib_i810_driverspecs, I810, "i810", FLAGS_NOTVINTERLACE },
#endif
#ifdef INCLUDE_LAGUNA_DRIVER
	{ &__svgalib_laguna_driverspecs, LAGUNA, "laguna", FLAGS_NOTV },
#endif
#ifdef INCLUDE_RAGE_DRIVER
	{ &__svgalib_rage_driverspecs, RAGE, "rage", FLAGS_NOTV },
#endif
#ifdef INCLUDE_MX_DRIVER
	{ &__svgalib_mx_driverspecs, MX, "mx", FLAGS_NOTV },
#endif
#ifdef INCLUDE_NEO_DRIVER
	{ &__svgalib_neo_driverspecs, NEOMAGIC, "neomagic", FLAGS_NOTV },
#endif
#ifdef INCLUDE_CHIPS_DRIVER
	{ &__svgalib_chips_driverspecs, CHIPS, "chips", FLAGS_NOTV },
#endif
#ifdef INCLUDE_MACH64_DRIVER
	{ &__svgalib_mach64_driverspecs, MACH64, "mach64", FLAGS_NOTV },
#endif
#ifdef INCLUDE_MACH32_DRIVER
	{ &__svgalib_mach32_driverspecs, MACH32, "mach32", FLAGS_NOTV },
#endif
#ifdef INCLUDE_EGA_DRIVER
	{ &__svgalib_ega_driverspecs, EGA, "ega", FLAGS_NOTV },
#endif
#ifdef INCLUDE_ET6000_DRIVER
	/* This must be before ET4000 */
	{ &__svgalib_et6000_driverspecs, ET6000, "et6000", FLAGS_NOTV },
#endif
#ifdef INCLUDE_ET4000_DRIVER
	{ &__svgalib_et4000_driverspecs, ET4000, "et4000", FLAGS_NOTV },
#endif
#ifdef INCLUDE_TVGA_DRIVER
	{ &__svgalib_tvga8900_driverspecs, TVGA8900, "tvga8900", FLAGS_NOTV }
#endif
#ifdef INCLUDE_CIRRUS_DRIVER
	{ &__svgalib_cirrus_driverspecs, CIRRUS, "cirrus", FLAGS_NOTV },
#endif
#ifdef INCLUDE_OAK_DRIVER
	{ &__svgalib_oak_driverspecs, OAK, "oak", FLAGS_NOTV },
#endif
#ifdef INCLUDE_PARADISE_DRIVER
	{ &__svgalib_paradise_driverspecs, PARADISE, "paradise", FLAGS_NOTV },
#endif
#ifdef INCLUDE_S3_DRIVER
	{ &__svgalib_s3_driverspecs, S3, "s3", FLAGS_NOTV },
#endif
#ifdef INCLUDE_ET3000_DRIVER
	{ &__svgalib_et3000_driverspecs, ET3000, "et3000", FLAGS_NOTV },
#endif
#ifdef INCLUDE_ARK_DRIVER
	{ &__svgalib_ark_driverspecs, ARK, "ark", FLAGS_NOTV },
#endif
#ifdef INCLUDE_GVGA6400_DRIVER
	{ &__svgalib_gvga6400_driverspecs, GVGA6400, "gvga6400", FLAGS_NOTV },
#endif
#ifdef INCLUDE_ATI_DRIVER
	{ &__svgalib_ati_driverspecs, ATI, "ati", FLAGS_NOTV },
#endif
#ifdef INCLUDE_ALI_DRIVER
	{ &__svgalib_ali_driverspecs, ALI, "ali", FLAGS_NOTV },
#endif
#ifdef INCLUDE_APM_DRIVER
	/* The driver doesn't check the INTERLACED flags */
	/* On certain cards this may toggle the video signal on/off which is ugly. Hence we test this last. */
	{ &__svgalib_apm_driverspecs, APM, "apm", FLAGS_NOTVINTERLACE },
#endif
	{ 0, 0, 0, 0 }
};

/* Keep the same order of svgaline_chipset_struct cards */
static adv_device DEVICE[] = {
	{ "auto", -1, "SVGALINE video" },
#ifdef INCLUDE_NV3_DRIVER
	{ "nv3", NV3, "nVidia Riva/GeForce" },
#endif
#ifdef INCLUDE_TRIDENT_DRIVER
	{ "trident", TRIDENT, "Trident" },
#endif
#ifdef INCLUDE_RENDITION_DRIVER
	{ "rendition", RENDITION, "Rendition" },
#endif
#ifdef INCLUDE_G400_DRIVER
	{ "g400", G400, "Matrox Mystique/G100/G200/G400/G450" },
#endif
#ifdef INCLUDE_PM2_DRIVER
	{ "pm2", PM2, "Permedia 2" },
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
	{ "savage", SAVAGE, "S3 Savage" },
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER
	{ "millenium", MILLENNIUM, "Matrox Millennium/Millenium II" },
#endif
#ifdef INCLUDE_R128_DRIVER
	{ "r128", R128, "ATI Rage 128/Radeon" },
#endif
#ifdef INCLUDE_BANSHEE_DRIVER
	{ "banshee", BANSHEE, "3dfx Voodoo Banshee/3/4/5" },
#endif
#ifdef INCLUDE_SIS_DRIVER
	{ "sis", SIS, "SIS" },
#endif
#ifdef INCLUDE_I740_DRIVER
	{ "i740", I740, "Intel i740" },
#endif
#ifdef INCLUDE_I810_DRIVER
	{ "i810", I810, "Intel i810" },
#endif
#ifdef INCLUDE_LAGUNA_DRIVER
	{ "laguna", LAGUNA, "Cirrus Logic Laguna 5462/5464/5465" },
#endif
#ifdef INCLUDE_RAGE_DRIVER
	{ "rage", RAGE, "ATI Rage" },
#endif
#ifdef INCLUDE_MX_DRIVER
	{ "mx", MX, "MX" },
#endif
#ifdef INCLUDE_NEO_DRIVER
	{ "neomagic", NEOMAGIC, "NeoMagic" },
#endif
#ifdef INCLUDE_CHIPS_DRIVER
	{ "chips", CHIPS, "Chips & Technologies" },
#endif
#ifdef INCLUDE_MACH64_DRIVER
	{ "mach64", MACH64, "ATI Mach 64" },
#endif
#ifdef INCLUDE_MACH32_DRIVER
	{ "mach32", MACH32, "ATI Mach 32" },
#endif
#ifdef INCLUDE_EGA_DRIVER
	{ "ega", EGA, "EGA" },
#endif
#ifdef INCLUDE_ET6000_DRIVER
	{ "et6000", ET6000, "ET6000" },
#endif
#ifdef INCLUDE_ET4000_DRIVER
	{ "et4000", ET4000, "ET4000" },
#endif
#ifdef INCLUDE_TVGA_DRIVER
	{ "tvga8900", TVGA8900, "TVGA8900" }
#endif
#ifdef INCLUDE_CIRRUS_DRIVER
	{ "cirrus", CIRRUS, "Cirrus Logic" },
#endif
#ifdef INCLUDE_OAK_DRIVER
	{ "oak", OAK, "OAK" },
#endif
#ifdef INCLUDE_PARADISE_DRIVER
	{ "paradise", PARADISE, "Paradise" },
#endif
#ifdef INCLUDE_S3_DRIVER
	{ "s3", S3, "S3" },
#endif
#ifdef INCLUDE_ET3000_DRIVER
	{ "et3000", ET3000, "ET3000" },
#endif
#ifdef INCLUDE_ARK_DRIVER
	{ "ark", ARK, "ARK" },
#endif
#ifdef INCLUDE_GVGA6400_DRIVER
	{ "gvga6400", GVGA6400, "GVGA6400" },
#endif
#ifdef INCLUDE_ATI_DRIVER
	{ "ati", ATI, "ATI" },
#endif
#ifdef INCLUDE_ALI_DRIVER
	{ "ali", ALI, "ALI" },
#endif
#ifdef INCLUDE_APM_DRIVER
	{ "apm", APM, "APM" },
#endif
	{ 0, 0, 0 }
};

#define CARD_MAX (sizeof(cards)/sizeof(cards[0]) - 1)

/** Test the capability of the driver */
static adv_error svgaline_test_capability(struct svgaline_chipset_struct* driver) {
	unsigned bit_map[5] = { 8,15,16,24,32 };
	unsigned i;

	/* linear frame buffer */
	if (__svgalib_linear_mem_size == 0) {
		log_std(("svgaline: linear_size 0, skip\n"));
		return -1;
	}

	if (__svgalib_linear_mem_base == 0) {
		log_std(("svgaline: linear_base 0, skip\n"));
		return -1;
	}

	if (!driver->drv->saveregs
		|| !driver->drv->setregs
		|| !driver->drv->unlock
		|| !driver->drv->test
		|| !driver->drv->init
		|| !driver->drv->setmode
		|| !driver->drv->modeavailable
		|| !driver->drv->linear) {
		log_std(("svgaline: missing function, skip\n"));
	}

	/* bit depth */
	for(i=0;i<5;++i) {
		unsigned bit = bit_map[i];

		libdos_mode_init(25200000/2,640/2,656/2,752/2,800/2,480,490,492,525,0,0,1,1,bit,0,0);

		if (driver->drv->modeavailable(libdos_mode_number) == 0) {
			unsigned cap = 0;
			switch (bit) {
				case 8 : cap = VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT; break;
				case 15 : cap = VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT; break;
				case 16 : cap = VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT; break;
				case 24 : cap = VIDEO_DRIVER_FLAGS_MODE_GRAPH_24BIT; break;
				case 32 : cap = VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT; break;
			}
			driver->cap &= ~cap;
			log_std(("svgaline: mode bit %d not supported, removed\n", bit));
		}

		libdos_mode_done();
	}

	if ((driver->cap & VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL) == 0) {
		log_std(("svgaline: no modes supported, skip\n"));
		return -1;
	}

	/* interlace */
	if ((driver->cap & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) != 0) {
		libdos_mode_init(40280300,1024,1048,1200,1280,768,784,787,840,0,1,1,1,8,0,0);

		if (driver->drv->modeavailable(libdos_mode_number) == 0) {
			driver->cap &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
			log_std(("svgaline: interlace not supported, removed\n"));
		}

		libdos_mode_done();
	}

	return 0;
}

static void svgaline_mode_print(void) {

#ifndef NDEBUG
	unsigned char driver_regs[MAX_REGS];
	unsigned i,j,k;

	memset(driver_regs,0x00,sizeof(driver_regs));

	k = __svgalib_saveregs(driver_regs);

	for(i=0;i<k;i+=32) {
		log_std(("svgaline: regs %04x:",i));
		for(j=0;j<32 && i+j<k;++j)
			log_std(("%02x",(unsigned)driver_regs[i+j]));
		log_std(("\n"));
	}
#endif

	log_std(("svgaline: svgalib mmap(linear) address %x, size %d\n", svgaline_state.linear_base, svgaline_state.linear_size));
}

/***************************************************************************/
/* Public */

adv_error svgaline_init(int device_id) {
	unsigned i;
	const char* name;
	const adv_device* j;

	assert( !svgaline_is_active() );

	j = DEVICE;
	while (j->name && j->id != device_id)
		++j;
	if (!j->name)
		return -1;
	name = j->name;

	if (!svgaline_option.initialized) {
		svgaline_default();
	}

	libdos_init(svgaline_option.divide_clock);

	for(i=0;cards[i].name;++i) {
		if (strcmp(name,"auto")==0 || strcmp(name,cards[i].name)==0) {
			if (cards[i].drv->test()) {
				log_std(("svgaline: found driver %s\n",cards[i].name));
				if (svgaline_test_capability(&cards[i]) != 0) {
					continue;
				}
				svgaline_state.driver = &cards[i];
				svgaline_state.active = 1;
				__svgalib_chipset = svgaline_state.driver->chipset;
				break;
			}
		}
	}

	if (!svgaline_state.active)
		return -1;

	return 0;
}

void svgaline_done(void) {
	assert(svgaline_is_active() && !svgaline_mode_is_active() );

	svgaline_state.active = 0;

	libdos_done();
}

adv_bool svgaline_is_active(void) {
	return svgaline_state.active != 0;
}

adv_bool svgaline_mode_is_active(void) {
	return svgaline_state.mode_active != 0;
}

unsigned svgaline_flags(void) {
	assert( svgaline_is_active() );
	return svgaline_state.driver->cap;
}

static adv_error svgaline_mode_set_noint(const svgaline_video_mode* mode)
{
	unsigned clock;

	assert(svgaline_is_active() && !svgaline_mode_is_active());

	log_std(("svgaline: mode_set bits_per_pixel %d\n", mode->bits_per_pixel ));
	log_std_modeline_c(("svgaline: mode_set modeline", mode->crtc.pixelclock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc) ));
	log_std(("svgaline: expected vert clock: %.2f Hz\n", crtc_vclock_get(&mode->crtc) ));

	clock = mode->crtc.pixelclock;
	if (svgaline_option.divide_clock)
		clock *= 2;

	libdos_mode_init(clock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc), crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), mode->bits_per_pixel, crtc_is_tvpal(&mode->crtc), crtc_is_tvntsc(&mode->crtc));

	if (!__svgalib_linear_mem_size) {
		error_set("The current driver does't support the linear frame buffer");
		return -1;
	}

	svgaline_state.linear_base = __svgalib_linear_mem_base;
	svgaline_state.linear_size = __svgalib_linear_mem_size;

	log_std(("svgaline: svgalib mmap(linear) address %x, size %d\n", svgaline_state.linear_base, svgaline_state.linear_size));
	svgaline_state.linear_pointer = mmap(0, svgaline_state.linear_size, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd, svgaline_state.linear_base);

	log_std(("svgaline: svgalib linear pointer %p\n", svgaline_state.linear_pointer));
	__svgalib_linear_pointer = svgaline_state.linear_pointer;

	if (__svgalib_mmio_size) {
		log_std(("svgaline: svgalib mmap(mmio) address %x, size %d\n", (unsigned)__svgalib_mmio_base, (unsigned)__svgalib_mmio_size));
		__svgalib_mmio_pointer = mmap(0, __svgalib_mmio_size, PROT_READ | PROT_WRITE, MAP_SHARED, __svgalib_mem_fd, __svgalib_mmio_base);
		log_std(("svgaline: svgalib mmio pointer %x\n",(unsigned)__svgalib_mmio_pointer));
	} else
		__svgalib_mmio_pointer = 0;

	log_std(("svgaline: svgalib unlock()\n"));

	if (svgaline_state.driver->drv->unlock)
		svgaline_state.driver->drv->unlock();

	log_std(("svgaline: svgalib saveregs()\n"));

	__svgalib_saveregs(svgaline_state.driver_regs);

#ifdef NDEBUG
	log_std(("svgaline: svgalib screenoff()\n"));
	vga_screenoff();
#endif

	log_std(("svgaline: svgalib setmode()\n"));

	if (svgaline_state.driver->drv->setmode(libdos_mode_number, TEXT)) {
		error_set("Generic adv_error setting the svgaline mode");
		return -1;
	}

	log_std(("svgaline: svgalib print()\n"));

	svgaline_mode_print();

	log_std(("svgaline: svgalib delay()\n"));

	usleep(10000); /* wait for signal to stabilize */

	log_std(("svgaline: svgalib screenon()\n"));

	vga_screenon();

	log_std(("svgaline: svgalib linear(LINEAR_ENABLE)\n"));

	if (svgaline_state.driver->drv->linear(LINEAR_ENABLE,svgaline_state.linear_base)!=0) {
		error_set("Generic adv_error setting the linear mode");
		return -1;
	}

	svgaline_write_line = svgaline_linear_write_line;

	svgaline_state.mode_active = 1;

	log_std(("svgaline: mode_set done\n"));

	return 0;
}

adv_error svgaline_mode_set(const svgaline_video_mode* mode) {
	adv_error r;

#ifdef NDEBUG
	/* disable the interrupts */
	disable();
#endif
	r = svgaline_mode_set_noint(mode);
#ifdef NDEBUG
	enable();
#endif

	return r;
}

adv_error svgaline_mode_change(const svgaline_video_mode* mode) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	/* fast unset */
	log_std(("svgaline: svgalib unlock()\n"));

	if (svgaline_state.driver->drv->unlock)
		svgaline_state.driver->drv->unlock();

	log_std(("svgaline: svgalib linear(LINEAR_DISABLE)\n"));

	svgaline_state.driver->drv->linear(LINEAR_DISABLE,svgaline_state.linear_base);

	log_std(("svgaline: svgalib unmap(linear)\n"));

	munmap(svgaline_state.linear_pointer, svgaline_state.linear_size);
	svgaline_state.linear_size = 0;
	svgaline_state.linear_base = 0;
	svgaline_state.linear_pointer = 0;
	__svgalib_linear_pointer = 0;

	if (__svgalib_mmio_size) {
		log_std(("svgaline: svgalib unmap(mmio)\n"));
		munmap(__svgalib_mmio_pointer, __svgalib_mmio_size);
		__svgalib_mmio_pointer = 0;
	}

	libdos_mode_done();

	svgaline_state.mode_active = 0;

	return svgaline_mode_set(mode);
}

static void svgaline_mode_done_noint(adv_bool restore) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	/* complete unset */
	log_std(("svgaline: svgalib unlock()\n"));

	if (svgaline_state.driver->drv->unlock)
		svgaline_state.driver->drv->unlock();

	log_std(("svgaline: svgalib linear(LINEAR_DISABLE)\n"));

	svgaline_state.driver->drv->linear(LINEAR_DISABLE,svgaline_state.linear_base);

	if (restore) {
#ifdef NDEBUG
		log_std(("svgaline: svgalib screenoff()\n"));
		vga_screenoff();
#endif
		log_std(("svgaline: svgalib vgasetregs()\n"));

		__svgalib_setregs(svgaline_state.driver_regs);

		log_std(("svgaline: svgalib setregs()\n"));

		svgaline_state.driver->drv->setregs(svgaline_state.driver_regs, TEXT);

		log_std(("svgaline: svgalib delay()\n"));

		usleep(10000); /* wait for signal to stabilize */

		log_std(("svgaline: svgalib screenon()\n"));

		vga_screenon();
	}

	log_std(("svgaline: svgalib unmap(linear)\n"));

	munmap(svgaline_state.linear_pointer, svgaline_state.linear_size);
	svgaline_state.linear_size = 0;
	svgaline_state.linear_base = 0;
	svgaline_state.linear_pointer = 0;
	__svgalib_linear_pointer = 0;

	if (__svgalib_mmio_size) {
		log_std(("svgaline: svgalib unmap(mmio)\n"));
		munmap(__svgalib_mmio_pointer, __svgalib_mmio_size);
		__svgalib_mmio_pointer = 0;
	}

	libdos_mode_done();

	svgaline_state.mode_active = 0;
}

void svgaline_mode_done(adv_bool restore) {
	/* disable the interrupts */
	disable();
	svgaline_mode_done_noint(restore);
	enable();
}

unsigned svgaline_virtual_x(void) {
	unsigned size = libdos_mode.bytes_per_scanline / libdos_mode.bytes_per_pixel;
	size = size & ~0x7;
	return size;
}

unsigned svgaline_virtual_y(void) {
	return __svgalib_linear_mem_size / libdos_mode.bytes_per_scanline;
}

unsigned svgaline_adjust_bytes_per_page(unsigned bytes_per_page) {
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned svgaline_bytes_per_scanline(void) {
	return libdos_mode.bytes_per_scanline;
}

adv_rgb_def svgaline_rgb_def(void) {
	return rgb_def_make(libdos_mode.red_len,libdos_mode.red_pos,libdos_mode.green_len,libdos_mode.green_pos,libdos_mode.blue_len,libdos_mode.blue_pos);
}

void svgaline_wait_vsync(void) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	vga_waitretrace();
}

adv_error svgaline_scroll(unsigned offset, adv_bool waitvsync) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	if (!svgaline_state.driver->drv->setdisplaystart)
		return -1;

	if (waitvsync)
		svgaline_wait_vsync();

	svgaline_state.driver->drv->setdisplaystart(offset);
	return 0;
}

adv_error svgaline_scanline_set(unsigned byte_length) {
	assert(svgaline_is_active() && svgaline_mode_is_active());
	if (!svgaline_state.driver->drv->setlogicalwidth)
		return -1;
	svgaline_state.driver->drv->setlogicalwidth(byte_length);
	return 0;
}

adv_error svgaline_palette8_set(const adv_color* palette, unsigned start, unsigned count, adv_bool waitvsync) {
	if (waitvsync)
		svgaline_wait_vsync();

	while (count) {
		vga_setpalette(start, palette->red >> 2, palette->green >> 2, palette->blue >> 2);
		++palette;
		++start;
		--count;
	}

	return 0;
}

#define DRIVER(mode) ((svgaline_video_mode*)(&mode->driver_mode))

adv_error svgaline_mode_import(adv_mode* mode, const svgaline_video_mode* svgaline_mode)
{
	strcpy(mode->name, svgaline_mode->crtc.name);

	*DRIVER(mode) = *svgaline_mode;

	mode->driver = &video_svgaline_driver;
	mode->flags = MODE_FLAGS_SCROLL_ASYNC
		| MODE_FLAGS_MEMORY_LINEAR
		| (mode->flags & MODE_FLAGS_USER_MASK);
	switch (svgaline_mode->bits_per_pixel) {
		case 8 : mode->flags |= MODE_FLAGS_INDEX_PACKED | MODE_FLAGS_TYPE_GRAPHICS; break;
		default: mode->flags |= MODE_FLAGS_INDEX_RGB | MODE_FLAGS_TYPE_GRAPHICS; break;
	}

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->bits_per_pixel = svgaline_mode->bits_per_pixel;
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

adv_error svgaline_mode_generate(svgaline_video_mode* mode, const adv_crtc* crtc, unsigned bits, unsigned flags)
{
	assert( svgaline_is_active() );

	if (video_mode_generate_check("svgaline",svgaline_flags(),8,2048,crtc,bits,flags)!=0)
		return -1;

	/* try the internal driver */
	libdos_mode_init(crtc->pixelclock, crtc->hde, crtc->hrs, crtc->hre, crtc->ht, crtc->vde, crtc->vrs, crtc->vre, crtc->vt, crtc_is_doublescan(crtc), crtc_is_interlace(crtc), crtc_is_nhsync(crtc), crtc_is_nvsync(crtc), bits, 0, 0);

	if (svgaline_state.driver->drv->modeavailable(libdos_mode_number) == 0) {
		error_nolog_cat("svgaline: Generic adv_error checking the availability of the video mode\n");
		libdos_mode_done();
		return -1;
	}

	libdos_mode_done();

	mode->crtc = *crtc;
	mode->bits_per_pixel = bits;

	return 0;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int svgaline_mode_compare(const svgaline_video_mode* a, const svgaline_video_mode* b) {
	COMPARE(a->bits_per_pixel,b->bits_per_pixel);
	return crtc_compare(&a->crtc,&b->crtc);
}

void svgaline_default(void) {
	svgaline_option.initialized = 1;
	svgaline_option.divide_clock = 0;
}

void svgaline_reg(adv_conf* context) {
	assert( !svgaline_is_active() );

	conf_bool_register_default(context, "device_svgaline_divide_clock", 0);

	svgaline_option.initialized = 1;
}

adv_error svgaline_load(adv_conf* context) {
	assert( !svgaline_is_active() );

	svgaline_option.divide_clock = conf_bool_get_default(context, "device_svgaline_divide_clock");

	svgaline_option.initialized = 1;

	return 0;
}

/***************************************************************************/
/* Driver */

static adv_error svgaline_mode_set_void(const void* mode) {
	return svgaline_mode_set((const svgaline_video_mode*)mode);
}

static adv_error svgaline_mode_change_void(const void* mode) {
	return svgaline_mode_change((const svgaline_video_mode*)mode);
}

static adv_error svgaline_mode_import_void(adv_mode* mode, const void* svgaline_mode) {
	return svgaline_mode_import(mode, (const svgaline_video_mode*)svgaline_mode);
}

static adv_error svgaline_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned bits, unsigned flags) {
	return svgaline_mode_generate((svgaline_video_mode*)mode,crtc,bits,flags);
}

static int svgaline_mode_compare_void(const void* a, const void* b) {
	return svgaline_mode_compare((const svgaline_video_mode*)a, (const svgaline_video_mode*)b);
}

static unsigned svgaline_mode_size(void) {
	return sizeof(svgaline_video_mode);
}

adv_video_driver video_svgaline_driver = {
	"svgaline",
	DEVICE,
	svgaline_load,
	svgaline_reg,
	svgaline_init,
	svgaline_done,
	svgaline_flags,
	svgaline_mode_set_void,
	svgaline_mode_change_void,
	svgaline_mode_done,
	svgaline_virtual_x,
	svgaline_virtual_y,
	0,
	0,
	svgaline_bytes_per_scanline,
	svgaline_adjust_bytes_per_page,
	svgaline_rgb_def,
	0,
	0,
	&svgaline_write_line,
	svgaline_wait_vsync,
	svgaline_scroll,
	svgaline_scanline_set,
	svgaline_palette8_set,
	0,
	svgaline_mode_size,
	0,
	svgaline_mode_generate_void,
	svgaline_mode_import_void,
	svgaline_mode_compare_void,
	0
};

