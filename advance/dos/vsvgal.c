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

#include "svgalib.h"

#include <sys/mman.h>
#include <sys/nearptr.h>
#include <signal.h>
#include <unistd.h>
#include <dos.h>

/***************************************************************************/
/* State */

typedef struct svgaline_internal_struct {
	int active;
	int mode_active;

	unsigned cap;
	unsigned char saved[ADV_SVGALIB_STATE_SIZE];
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
	return (unsigned char*)adv_svgalib_linear_pointer_get() + adv_svgalib_scanline_get() * y;
}

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

	if (adv_svgalib_init(svgaline_option.divide_clock) != 0) {
		log_std(("svgaline: error calling adv_svgalib_init()\n"));
		return -1;
	}

	if (adv_svgalib_detect(name) != 0) {
		log_std(("svgaline: error calling adv_svgalib_detect(%s)\n", name));
		return -1;
	}

	log_std(("svgaline: found driver %s\n", adv_svgalib_driver_get()));

	svgaline_state.cap = VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC;

	if (adv_svgalib_state.has_bit8)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT;
	if (adv_svgalib_state.has_bit15)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT;
	if (adv_svgalib_state.has_bit16)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT;
	if (adv_svgalib_state.has_bit24)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_24BIT;
	if (adv_svgalib_state.has_bit32)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT;
	if (adv_svgalib_state.has_interlace)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
	if (adv_svgalib_state.has_tvpal)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVPAL;
	if (adv_svgalib_state.has_tvntsc)
		svgaline_state.cap |= VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVNTSC;

	svgaline_state.active = 1;

	return 0;
}

void svgaline_done(void) {
	assert(svgaline_is_active() && !svgaline_mode_is_active() );

	svgaline_state.active = 0;

	adv_svgalib_done();
}

adv_bool svgaline_is_active(void) {
	return svgaline_state.active != 0;
}

adv_bool svgaline_mode_is_active(void) {
	return svgaline_state.mode_active != 0;
}

unsigned svgaline_flags(void) {
	assert( svgaline_is_active() );
	return svgaline_state.cap;
}

adv_error svgaline_mode_set(const svgaline_video_mode* mode)
{
	unsigned clock;

	assert(svgaline_is_active() && !svgaline_mode_is_active());

	log_std(("svgaline: mode_set bits_per_pixel %d\n", mode->bits_per_pixel ));
	log_std_modeline_c(("svgaline: mode_set modeline", mode->crtc.pixelclock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc) ));
	log_std(("svgaline: expected vert clock: %.2f Hz\n", crtc_vclock_get(&mode->crtc) ));

	clock = mode->crtc.pixelclock;
	if (svgaline_option.divide_clock)
		clock *= 2;

	adv_svgalib_linear_map();

	adv_svgalib_save(svgaline_state.saved);

	if (adv_svgalib_set(clock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc), crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), mode->bits_per_pixel, crtc_is_tvpal(&mode->crtc), crtc_is_tvntsc(&mode->crtc)) != 0) {
		adv_svgalib_linear_unmap();
		error_set("Generic error setting the svgaline mode");
		return -1;
	}

	svgaline_write_line = svgaline_linear_write_line;

	svgaline_state.mode_active = 1;

	log_std(("svgaline: mode_set done\n"));

	return 0;
}

adv_error svgaline_mode_change(const svgaline_video_mode* mode) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	adv_svgalib_unset();

	adv_svgalib_linear_unmap();

	svgaline_state.mode_active = 0;

	return svgaline_mode_set(mode);
}

void svgaline_mode_done(adv_bool restore) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	adv_svgalib_unset();

	if (restore)
		adv_svgalib_restore(svgaline_state.saved);

	adv_svgalib_linear_unmap();

	svgaline_state.mode_active = 0;
}

unsigned svgaline_virtual_x(void) {
	unsigned size = adv_svgalib_scanline_get() / adv_svgalib_pixel_get();
	size = size & ~0x7;
	return size;
}

unsigned svgaline_virtual_y(void) {
	return adv_svgalib_linear_size_get() / adv_svgalib_scanline_get();
}

unsigned svgaline_adjust_bytes_per_page(unsigned bytes_per_page) {
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned svgaline_bytes_per_scanline(void) {
	return adv_svgalib_scanline_get();
}

adv_rgb_def svgaline_rgb_def(void) {
	return rgb_def_make(adv_svgalib_state.mode.red_len,adv_svgalib_state.mode.red_pos,adv_svgalib_state.mode.green_len,adv_svgalib_state.mode.green_pos,adv_svgalib_state.mode.blue_len,adv_svgalib_state.mode.blue_pos);
}

void svgaline_wait_vsync(void) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	adv_svgalib_wait_vsync();
}

adv_error svgaline_scroll(unsigned offset, adv_bool waitvsync) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	if (waitvsync)
		adv_svgalib_wait_vsync();

	adv_svgalib_scroll_set(offset);

	return 0;
}

adv_error svgaline_scanline_set(unsigned byte_length) {
	assert(svgaline_is_active() && svgaline_mode_is_active());

	adv_svgalib_scanline_set(byte_length);
	return 0;
}

adv_error svgaline_palette8_set(const adv_color* palette, unsigned start, unsigned count, adv_bool waitvsync) {
	if (waitvsync)
		adv_svgalib_wait_vsync();

	while (count) {
		adv_svgalib_palette_set(start, palette->red, palette->green, palette->blue);
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
	adv_svgalib_mode_init(crtc->pixelclock, crtc->hde, crtc->hrs, crtc->hre, crtc->ht, crtc->vde, crtc->vrs, crtc->vre, crtc->vt, crtc_is_doublescan(crtc), crtc_is_interlace(crtc), crtc_is_nhsync(crtc), crtc_is_nvsync(crtc), bits, 0, 0);

	if (adv_svgalib_state.driver->drv->modeavailable(adv_svgalib_state.mode_number) == 0) {
		error_nolog_cat("svgaline: Generic error checking the availability of the video mode\n");
		adv_svgalib_mode_done();
		return -1;
	}

	adv_svgalib_mode_done();

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

	conf_bool_register_default(context, "device_svgaline_divideclock", 0);

	svgaline_option.initialized = 1;
}

adv_error svgaline_load(adv_conf* context) {
	assert( !svgaline_is_active() );

	svgaline_option.divide_clock = conf_bool_get_default(context, "device_svgaline_divideclock");

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

