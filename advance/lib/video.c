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

#include "video.h"
#include "log.h"
#include "target.h"
#include "os.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef __MSDOS__
#include "pci.h" /* for pci_* */
#endif

/***************************************************************************/
/* State */

/** State of the video support. */
video_internal video_state;

struct video_option_struct {
	video_bool initialized; /**< Initialized flag. */
	video_bool scan_single; /**< Allow singlescan modes. */
	video_bool scan_double; /**< Allow doublescan modes. */
	video_bool scan_interlace; /**< Allow interlace modes. */
	video_bool fast_change; /**< Allow the fast change. */
	video_bool mode_8bit; /**< Allow 8 bit modes. */
	video_bool mode_15bit; /**< Allow 15 bit modes. */
	video_bool mode_16bit; /**< Allow 16 bit modes. */
	video_bool mode_32bit; /**< Allow 32 bit modes. */
	char name[DEVICE_NAME_MAX]; /**< Name of the device. */
};

/** State of the video configuration options. */
static struct video_option_struct video_option;

unsigned char* (*video_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

#ifdef __MSDOS__
static int pci_scan_device_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* arg)
{
	DWORD dw;
	unsigned base_class;
	unsigned subsys_card;
	unsigned subsys_vendor;

	(void)arg;

	if (pci_read_dword(bus_device_func,0x8,&dw)!=0)
		return 0;

	base_class = (dw >> 24) & 0xFF;
	if (base_class != 0x3 /* PCI_CLASS_DISPLAY */)
		return 0;

	if (pci_read_dword(bus_device_func,0x2c,&dw)!=0)
		return 0;

	subsys_vendor = dw & 0xFFFF;
	subsys_card = (dw >> 16) & 0xFFFF;

	log_std(("video: found pci display vendor:%04x, device:%04x, subsys_vendor:%04x, subsys_card:%04x\n",vendor,device,subsys_vendor,subsys_card));

	return 0;
}
#endif

/***************************************************************************/
/* Public */

/**
 * Set the default configuration options.
 */
void video_default(void)
{
	video_option.initialized = 1;
	video_option.scan_single = 1;
	video_option.scan_double = 1;
	video_option.scan_interlace = 1;
	video_option.mode_8bit = 1;
	video_option.mode_15bit = 1;
	video_option.mode_16bit = 1;
	video_option.mode_32bit = 1;
	video_option.fast_change = 0;
	strcpy(video_option.name,"auto");
}

/**
 * Register the configuration options.
 */
void video_reg(struct conf_context* context, video_bool auto_detect)
{
	conf_string_register_default(context, "device_video", auto_detect ? "auto" : "none");
	conf_bool_register_default(context, "device_video_singlescan", 1);
	conf_bool_register_default(context, "device_video_doublescan", 1);
	conf_bool_register_default(context, "device_video_interlace", 1);
	conf_bool_register_default(context, "device_video_fastchange", 0);
	conf_bool_register_default(context, "device_video_8bit", 1);
	conf_bool_register_default(context, "device_video_15bit", 1);
	conf_bool_register_default(context, "device_video_16bit", 1);
	conf_bool_register_default(context, "device_video_32bit", 1);
}

/**
 * Register a video driver.
 * \note Call before video_init().
 */
void video_reg_driver(struct conf_context* context, video_driver* driver)
{
	assert( !video_is_active() );

	assert( video_state.driver_mac < DEVICE_MAX );

	log_std(("video: register driver %s\n", driver->name));

	video_state.driver_map[video_state.driver_mac] = driver;
	video_state.driver_map[video_state.driver_mac]->reg(context);

	++video_state.driver_mac;
}

/**
 * Inizialize the video system.
 */
video_error video_init(void)
{
	unsigned i;
	int at_least_one;

	assert( !video_is_active() );

	log_std(("video: video_init\n"));

#ifdef __MSDOS__
	if (pci_scan_device(pci_scan_device_callback,0)!=0) {
		log_std(("video: error scanning pci display device, resume and continue\n"));
	}
#endif

	if (!video_option.initialized) {
		video_default();
	}

	/* enable all the video driver */
	/* backward order is used to respect the dependencies */
	at_least_one = 0;
	for(i=0;i<video_state.driver_mac;++i) {
		unsigned j = video_state.driver_mac - 1 - i;
		const device* dev;

		assert(video_state.driver_map[j]->mode_size() <= VIDEO_DRIVER_MODE_SIZE_MAX);

		dev = device_match(video_option.name, (const driver*)video_state.driver_map[j], 0);

		if (dev && video_state.driver_map[j]->init(dev->id) == 0) {
			log_std(("video: select driver %s\n", video_state.driver_map[j]->name));
			at_least_one = 1;
		} else {
			video_state.driver_map[j] = 0; /* deactivate the driver */
		}
	}

	if (!at_least_one) {
		log_std(("video: no video driver activated\n"));
		return -1;
	}

	/* enable */
	video_state.active = 1;
	video_state.mode_active = 0;

	video_state.old_mode_required = 0;

	return 0;
}

/**
 * Deinizialize the video system.
 */
void video_done(void)
{
	unsigned i;

	assert( video_is_active() && !video_mode_is_active() );

	log_std(("video: video_done\n"));

	/* safer */
	if (video_mode_is_active())
		video_mode_done(1);

	/* disable all the video driver */
	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i]) {
			video_state.driver_map[i]->done();
		}
	}

	/* disable */
	video_state.active = 0;

	/* reset the driver list */
	video_state.driver_mac = 0;
}

/**
 * Emergency abort.
 * Callable in any context.
 */
void video_abort(void)
{
	if (video_is_active()) {
		if (video_mode_is_active())
			video_mode_done(1);
		video_done();
	}
}

/***************************************************************************/
/* Options */

/**
 * Load the video configuration.
 * \param ignore list of driver to ignore (separated by space).
 */
video_error video_load(struct conf_context* context, const char* driver_ignore)
{
	unsigned i;
	int at_least_one;

	if (video_state.driver_mac == 0) {
		video_error_description_set("No video driver registered\n");
		return -1;
	}

	/* options must be loaded before initialization */
	assert( !video_is_active() );

	video_option.initialized = 1;
	video_option.scan_single = conf_bool_get_default(context,"device_video_singlescan");
	video_option.scan_double = conf_bool_get_default(context,"device_video_doublescan");
	video_option.scan_interlace = conf_bool_get_default(context,"device_video_interlace");
	video_option.fast_change = conf_bool_get_default(context,"device_video_fastchange");
	video_option.mode_8bit = conf_bool_get_default(context,"device_video_8bit");
	video_option.mode_15bit = conf_bool_get_default(context,"device_video_15bit");
	video_option.mode_16bit = conf_bool_get_default(context,"device_video_16bit");
	video_option.mode_32bit = conf_bool_get_default(context,"device_video_32bit");

	strcpy(video_option.name, conf_string_get_default(context, "device_video"));

	if (device_check("device_video", video_option.name, (const driver**)video_state.driver_map, video_state.driver_mac, driver_ignore) != 0) {
		return -1;
	}

	/* load the specific driver options */
	at_least_one = 0;
	for(i=0;i<video_state.driver_mac;++i) {
		const device* dev;

		dev = device_match(video_option.name, (const driver*)video_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (video_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_video", video_option.name, (const driver**)video_state.driver_map, video_state.driver_mac);
		return -1;
	}

	return 0;
}

/***************************************************************************/
/* Mode */

/**
 * The predefinite capabilities flags.
 */
unsigned video_internal_flags(void)
{
	unsigned flags =
		VIDEO_DRIVER_FLAGS_MODE_MASK
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_MASK
		| VIDEO_DRIVER_FLAGS_INFO_MASK;

	if (!video_option.scan_double)
		flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
	if (!video_option.scan_single)
		flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN;
	if (!video_option.scan_interlace)
		flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
	if (!video_option.mode_8bit)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT;
	if (!video_option.mode_15bit)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT;
	if (!video_option.mode_16bit)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT;
	if (!video_option.mode_32bit)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT;

	return flags;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

/**
 * Compare two video mode.
 * \return Like strcmp.
 */
int video_mode_compare(const video_mode* a, const video_mode* b)
{
	COMPARE(video_mode_driver(a),video_mode_driver(b));

	return video_mode_driver(a)->mode_compare(a->driver_mode,b->driver_mode);
}

/**
 * Unset a video mode.
 * \param restore If the previous video mode must be restored.
 */
void video_mode_done(video_bool restore)
{
	assert( video_mode_is_active() );

	video_current_driver()->mode_done(restore);

	/* disable */
	video_write_line = 0;
	video_state.mode_active = 0;
}

static void video_state_rgb_set_from_def(video_rgb_def _def);
static void video_state_rgb_clear(void);

/**
 * Set a video mode.
 */
video_error video_mode_set(video_mode* mode)
{
	assert( video_is_active() );

	log_std(("video: mode_set %s %dx%d %d %.1f kHz/%.1f Hz\n",video_mode_name(mode),video_mode_size_x(mode),video_mode_size_y(mode),video_mode_bits_per_pixel(mode),(double)video_mode_hclock(mode) / 1E3,(double)video_mode_vclock(mode)));

	video_state.old_mode_required = 1; /* the mode will change */

	/* check for a mode change without a driver change */
	if (video_mode_is_active()
		&& video_option.fast_change
		&& video_mode_driver(mode)->mode_change != 0
		&& video_current_driver() == video_mode_driver(mode)
	) {
		/* change the mode */
		if (video_mode_driver(mode)->mode_change( &mode->driver_mode ) != 0) {
			video_state.mode_active = 0;
			video_write_line = 0;
			return -1;
		}
	} else {
		if (video_mode_is_active())
			video_mode_done(1);

		/* set the mode */
		if (video_mode_driver(mode)->mode_set( &mode->driver_mode ) != 0)
			return -1;
	}

	video_state.mode_active = 1;
	video_state.mode = *mode;
	video_state.virtual_x = video_current_driver()->virtual_x();
	video_state.virtual_y = video_current_driver()->virtual_y();
	video_state.rgb_def = video_current_driver()->rgb_def();
	video_write_line = *video_current_driver()->write_line;

	if (video_type() == VIDEO_FLAGS_TYPE_GRAPHICS && video_index() == VIDEO_FLAGS_INDEX_RGB) {
		video_state_rgb_set_from_def(video_state.rgb_def);
	} else {
		video_state_rgb_clear();
	}

	video_state.measured_vclock = video_measure_step(video_wait_vsync, 1 / 300.0, 1 / 10.0);

	{
		double factor;
		double error;

		if (video_vclock()) {
			factor = video_measured_vclock() / video_vclock();
			error = (video_measured_vclock() - video_vclock()) / video_vclock();
		} else {
			factor = 0;
			error = 0;
		}

		log_std(("video: measured/expected vert clock: %.2f/%.2f = %g (err %g%%)\n",video_measured_vclock(),video_vclock(),factor,error * 100.0));
	}

	return 0;
}

/**
 * Grab the current video mode.
 */
video_error video_mode_grab(video_mode* mode)
{
	unsigned char driver_mode[VIDEO_DRIVER_MODE_SIZE_MAX];
	unsigned i;

	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i] && video_state.driver_map[i]->mode_grab) {
			if (video_state.driver_map[i]->mode_grab(&driver_mode)==0 && video_state.driver_map[i]->mode_import(mode,&driver_mode)==0) {
				return 0;
			}
		}
	}

	return -1;
}

/**
 * Generate a video mode.
 * \param mode Destination of the new video mode.
 * \param crtc Required crtc of the new video mode.
 * \param bits Required bits per pixel of the new video mode.
 * \param flags Required flags of the new video mode.
 */
video_error video_mode_generate(video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags)
{
	unsigned char driver_mode[VIDEO_DRIVER_MODE_SIZE_MAX];
	unsigned i;

	/* clear the error */
	video_error_description_nolog_set("None driver is capable to do the specified video mode.\n\nThe following is the detailed list of errors for every driver:\n");

	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i]) {
			if (video_state.driver_map[i]->mode_generate(&driver_mode,crtc,bits,flags)==0 && video_state.driver_map[i]->mode_import(mode,&driver_mode)==0) {
				return 0;
			}
		}
	}

	return -1;
}

video_error video_mode_generate_check(const char* driver, unsigned driver_flags, unsigned hstep, unsigned hvmax, const video_crtc* crtc, unsigned bits, unsigned flags)
{
	if (crtc->hde % hstep != 0 || crtc->hrs % hstep != 0 || crtc->hre % hstep != 0 || crtc->ht % hstep != 0) {
		video_error_description_nolog_cat("%s: Horizontal crtc values are not a %d dot multiple\n",driver,hstep);
		return -1;
	}
	if (crtc->ht >= hvmax || crtc->vt >= hvmax) {
		video_error_description_nolog_cat("%s: Horizontal or vertical crtc total value bigger than %d\n",driver,hvmax);
		return -1;
	}

	switch (flags & VIDEO_FLAGS_TYPE_MASK) {
		case VIDEO_FLAGS_TYPE_GRAPHICS :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL)==0) {
				video_error_description_nolog_cat("%s: Graphics modes not supported\n",driver);
				return -1;
			}
			break;
		case VIDEO_FLAGS_TYPE_TEXT :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_TEXT)==0) {
				video_error_description_nolog_cat("%s: Text modes not supported\n",driver);
				return -1;
			}
			break;
		default:
			assert(0);
	}

	if (crtc_is_interlace(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) == 0) {
			video_error_description_nolog_cat("%s: Interlace not supported\n",driver);
			return -1;
		}
	}

	if (crtc_is_doublescan(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) == 0) {
			video_error_description_nolog_cat("%s: Doublescan not supported\n",driver);
			return -1;
		}
	}

	if (crtc_is_singlescan(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN) == 0) {
			video_error_description_nolog_cat("%s: Singlescan not supported\n");
			return -1;
		}
	}

	if (crtc_is_tvpal(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVPAL) == 0) {
			video_error_description_nolog_cat("%s: TV-PAL not supported\n");
			return -1;
		}
	}

	if (crtc_is_tvntsc(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_TVNTSC) == 0) {
			video_error_description_nolog_cat("%s: TV-NTSC not supported\n");
			return -1;
		}
	}

	switch (flags & VIDEO_FLAGS_INDEX_MASK) {
		case VIDEO_FLAGS_INDEX_RGB :
		case VIDEO_FLAGS_INDEX_PACKED :
			if ((flags & VIDEO_FLAGS_TYPE_MASK) != VIDEO_FLAGS_TYPE_GRAPHICS) {
				video_error_description_nolog_cat("%s: Graph mode index supported only in graphics modes\n",driver);
				return -1;
			}
			break;
		case VIDEO_FLAGS_INDEX_TEXT :
			if ((flags & VIDEO_FLAGS_TYPE_MASK) != VIDEO_FLAGS_TYPE_TEXT) {
				video_error_description_nolog_cat("%s: Text mode index supported only in text modes\n",driver);
				return -1;
			}
			break;
		default:
			return -1;
	}

	switch (bits) {
		case 0 :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_TEXT)==0) {
				video_error_description_nolog_cat("%s: Text mode bit depth not supported\n",driver);
				return -1;
			}
			break;
		case 8 :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_8BIT) == 0) {
				video_error_description_nolog_cat("%s: %d bit depth not supported\n",driver,bits);
				return -1;
			}
			break;
		case 15 :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_15BIT) == 0) {
				video_error_description_nolog_cat("%s: %d bit depth not supported\n",driver,bits);
				return -1;
			}
			break;
		case 16 :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_16BIT) == 0) {
				video_error_description_nolog_cat("%s: %d bit depth not supported\n",driver,bits);
				return -1;
			}
			break;
		case 24 :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_24BIT) == 0) {
				video_error_description_nolog_cat("%s: %d bit depth not supported\n",driver,bits);
				return -1;
			}
			break;
		case 32 :
			if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_GRAPH_32BIT) == 0) {
				video_error_description_nolog_cat("%s: %d bit depth not supported\n",driver,bits);
				return -1;
			}
			break;
		default:
			video_error_description_nolog_cat("%s: %d bit depth not supported\n",driver,bits);
			return -1;
	}

	return 0;
}

unsigned video_mode_generate_driver_flags(void)
{
	unsigned flags;
	unsigned i;

	flags = 0;

	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i]) {
			/* limit the flags with the video internal options */
			unsigned new_flags = video_state.driver_map[i]->flags() & video_internal_flags();

			/* add flags only with the same programmable capabilities */
			if (!flags || (flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_MASK) == (new_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_MASK))
				flags |= new_flags;
		}
	}

	return flags;
}

/**
 * Reset the startup mode.
 * \note After this call none video mode is active. video_mode_active()==0
 */
void video_mode_reset(void)
{
	assert(video_is_active());

	if (video_mode_is_active())
		video_mode_done(1);

	/* if no video mode change return */
	if (!video_state.old_mode_required)
		return;

	/* os mode reset */
	target_mode_reset();
}

/**
 * Wait for a vertical retrace.
 * \note if no video mode is active the VGA version is used
 */
void video_wait_vsync(void)
{
	assert( video_mode_is_active() );

	video_current_driver()->wait_vsync();
}

static int video_double_cmp(const void* _a, const void* _b)
{
	const double* a = (const double*)_a;
	const double* b = (const double*)_b;
	if (*a < *b)
		return -1;
	if (*a > *b)
		return 1;
	return 0;
}

#define VIDEO_MEASURE_COUNT 7

/**
 * Measure the time beetween two event.
 * \param low low limit time in second.
 * \param high high limit time in second.
 * \return
 *   - ==0 error in the measure
 *   - !=0 the frequency of the event
 */
double video_measure_step(void (*wait)(void), double low, double high)
{
	double map[VIDEO_MEASURE_COUNT];
	os_clock_t start, stop;
	unsigned map_start,map_end;
	unsigned median;
	unsigned i;
	double error;

	low *= OS_CLOCKS_PER_SEC;
	high *= OS_CLOCKS_PER_SEC;

	i = 0;
	wait();
	start = os_clock();
	while (i < VIDEO_MEASURE_COUNT) {
		wait();
		stop = os_clock();
		map[i] = stop - start;
		start = stop;
		++i;
	}

	qsort(map,VIDEO_MEASURE_COUNT,sizeof(double),video_double_cmp);

	map_start = 0;
	map_end = VIDEO_MEASURE_COUNT;

	/* reject low values */
	while (map_start < map_end && map[map_start] <= low)
		++map_start;

	/* reject high values */
	while (map_start < map_end && map[map_end-1] >= high)
		--map_end;

	if (map_start == map_end) {
		log_std(("video: measure vclock failed, return 0\n"));
		return 0;
	}

	median = map_start + (map_end - map_start) / 2; /* the median */

	for(i=map_start;i<map_end;++i) {
		log_std(("video: measured vclock %g\n", OS_CLOCKS_PER_SEC / map[i]));
	}

	if (map[map_start])
		error = (map[map_end - 1] - map[map_start]) / map[map_start];
	else
		error = 0;

	log_std(("video: used vclock %g (err %g%%)\n", OS_CLOCKS_PER_SEC / map[median], error * 100.0));

	return OS_CLOCKS_PER_SEC / map[median];
}

/** X size of the font for text mode. */
unsigned video_font_size_x(void)
{
	assert( video_mode_is_active() && video_is_text() );

	return video_current_driver()->font_size_x();
}

/** Y size of the font for text mode. */
unsigned video_font_size_y(void)
{
	assert( video_mode_is_active() && video_is_text() );

	return video_current_driver()->font_size_y();
}

/***************************************************************************/
/* Put */

static __inline__ void video_chained_put_pixel(unsigned x, unsigned y, unsigned color)
{
	switch (video_bytes_per_pixel()) {
		case 1 :
			*(uint8*)(video_write_line(y) + x) = color;
		break;
		case 2 :
			*(uint16*)(video_write_line(y) + 2*x) = color;
		break;
		case 3 : {
			uint8* p = (uint8*)(video_write_line(y) + 3*x);
			p[0] = color & 0xFF;
			p[1] = (color >> 8) & 0xFF;
			p[2] = (color >> 16) & 0xFF;
		}
		break;
		case 4 :
			*(uint32*)(video_write_line(y) + 4*x) = color;
		break;
	}
}

static __inline__ void video_unchained_put_pixel(unsigned x, unsigned y, unsigned color)
{
	video_unchained_plane_set(x % 4);
	*(uint8*)(video_write_line(y) + x / 4) = color;
}

void video_put_char(unsigned x, unsigned y, char c, unsigned color)
{
	assert( video_mode_is_active() && video_is_text() );
	*(uint16*)(video_write_line(y) + 2*x) = (color << 8) | (unsigned char)c;
}

static __inline__ void video_put_pixel_noclip(unsigned x, unsigned y, unsigned color)
{
	if (video_is_unchained())
		video_unchained_put_pixel(x,y,color);
	else
		video_chained_put_pixel(x,y,color);
}

void video_put_pixel(unsigned x, unsigned y, unsigned color)
{
	assert( video_mode_is_active() && video_is_graphics() );
	if (x < video_virtual_x() && y < video_virtual_y())
		video_put_pixel_noclip(x,y,color);
}

/****************************************************************************/
/* Color */

/** Compute the size in bit of the mask. */
static unsigned video_rgb_len_get_from_mask(unsigned mask)
{
	unsigned len = 0;
	if (!mask)
		return len;
	while ((mask & 1) == 0)
		mask >>= 1;
	while (mask) {
		++len;
		mask >>= 1;
	}
	return len;
}

/** Compute shift and mask value. */
static __inline__ void video_rgb_maskshift_get(unsigned* mask, int* shift, unsigned len, unsigned pos)
{
	*mask = ((1 << len) - 1) << pos;
	*shift = pos + len - 8;
}

/**
 * Make an arbitary RGB format definition.
 * \param red_len bits of the red channel
 * \param red_pos bit position red channel
 * \param green_len bits of the green channel
 * \param green_pos bit position green channel
 * \param blue_len bits of the blue channel
 * \param blue_pos bit position blue channel
 * \return RGB format
 */
video_rgb_def video_rgb_def_make(unsigned red_len, unsigned red_pos, unsigned green_len, unsigned green_pos, unsigned blue_len, unsigned blue_pos)
{
	union video_rgb_def_union def;
	def.ordinal = 0;

	def.nibble.red_len = red_len;
	def.nibble.red_pos = red_pos;
	def.nibble.green_len = green_len;
	def.nibble.green_pos = green_pos;
	def.nibble.blue_len = blue_len;
	def.nibble.blue_pos = blue_pos;

	return def.ordinal;
}

/**
 * Make an arbitary RGB format definition from a maskshift specification.
 */
video_rgb_def video_rgb_def_make_from_maskshift(unsigned red_mask, unsigned red_shift, unsigned green_mask, unsigned green_shift, unsigned blue_mask, unsigned blue_shift)
{
	unsigned red_len = video_rgb_len_get_from_mask(red_mask);
	unsigned green_len = video_rgb_len_get_from_mask(green_mask);
	unsigned blue_len = video_rgb_len_get_from_mask(blue_mask);
	unsigned red_pos = 8 + red_shift - red_len;
	unsigned green_pos = 8 + green_shift - green_len;
	unsigned blue_pos = 8 + blue_shift - blue_len;

	return video_rgb_def_make(red_len, red_pos, green_len, green_pos, blue_len, blue_pos);
}

/** Compute the distance of two color. */
unsigned video_color_dist(const video_color* A, const video_color* B)
{
	int r,g,b;
	r = (int)A->red - B->red;
	g = (int)A->green - B->green;
	b = (int)A->blue - B->blue;
	return r*r + g*g + b*b;
}

static void video_state_rgb_set_from_def(video_rgb_def _def)
{
	union video_rgb_def_union def;
	def.ordinal = _def;

	video_rgb_maskshift_get(&video_state.rgb_red_mask,&video_state.rgb_red_shift,def.nibble.red_len,def.nibble.red_pos);
	video_rgb_maskshift_get(&video_state.rgb_green_mask,&video_state.rgb_green_shift,def.nibble.green_len,def.nibble.green_pos);
	video_rgb_maskshift_get(&video_state.rgb_blue_mask,&video_state.rgb_blue_shift,def.nibble.blue_len,def.nibble.blue_pos);

	video_state.rgb_def = def.ordinal;
	video_state.rgb_red_len = def.nibble.red_len;
	video_state.rgb_green_len = def.nibble.green_len;
	video_state.rgb_blue_len = def.nibble.blue_len;
	video_state.rgb_mask_bit = video_rgb_get(0xFF,0xFF,0xFF);
	video_state.rgb_high_bit = video_rgb_get(0x80,0x80,0x80);
	video_state.rgb_low_bit = video_rgb_get(
		1 << (8-video_state.rgb_red_len),
		1 << (8-video_state.rgb_green_len),
		1 << (8-video_state.rgb_blue_len)
	);
}

static void video_state_rgb_clear(void)
{
	video_state.rgb_def = 0;
	video_state.rgb_red_shift = 0;
	video_state.rgb_green_shift = 0;
	video_state.rgb_blue_shift = 0;
	video_state.rgb_red_mask = 0;
	video_state.rgb_green_mask = 0;
	video_state.rgb_blue_mask = 0;
	video_state.rgb_red_len = 0;
	video_state.rgb_green_len = 0;
	video_state.rgb_blue_len = 0;
	video_state.rgb_mask_bit = 0;
	video_state.rgb_high_bit = 0;
	video_state.rgb_low_bit =  0;
}

static unsigned video_rgb_approx(unsigned value, unsigned len)
{
	unsigned fill = len;
	while (fill < 8) {
		value |= value >> fill;
		fill *= 2;
	}
	return value;
}

unsigned video_red_get_approx(unsigned rgb)
{
	return video_rgb_approx(video_red_get(rgb), video_state.rgb_red_len);
}

unsigned video_green_get_approx(unsigned rgb)
{
	return video_rgb_approx(video_green_get(rgb), video_state.rgb_green_len);
}

unsigned video_blue_get_approx(unsigned rgb)
{
	return video_rgb_approx(video_blue_get(rgb), video_state.rgb_blue_len);
}

/** Convert one video mode from PACKED to RGB. */
void video_index_packed_to_rgb(int waitvsync)
{
	unsigned bit_r;
	unsigned bit_g;
	unsigned bit_b;

	video_color* palette;
	unsigned i;
	unsigned colors = 1 << video_bits_per_pixel();

	bit_b = video_bits_per_pixel() / 3;
	bit_r = (video_bits_per_pixel() - bit_b) / 2;
	bit_g = video_bits_per_pixel() - bit_b - bit_r;

	video_state_rgb_set_from_def(video_rgb_def_make(bit_r,bit_b+bit_g,bit_g,bit_b,bit_b,0));

	palette = malloc(colors * sizeof(video_color));
	assert( palette );

	for(i=0;i<colors;++i) {
		palette[i].red = video_red_get_approx(i);
		palette[i].green = video_green_get_approx(i);
		palette[i].blue = video_blue_get_approx(i);
	}

	video_palette_set(palette,0,colors,waitvsync);

	free(palette);

	/* set RGB */
	video_state.mode.flags &= ~VIDEO_FLAGS_INDEX_MASK;
	video_state.mode.flags |= VIDEO_FLAGS_INDEX_RGB;
}

int video_index_rgb_to_packed_is_available(void)
{
	return video_index() == VIDEO_FLAGS_INDEX_RGB && video_bytes_per_pixel() == 1;
}

int video_index_packed_to_rgb_is_available(void)
{
	return video_index() == VIDEO_FLAGS_INDEX_PACKED;
}

/**
 * Convert a RGB video mode to PACKED
 * \note This function can be called only if video_index_rgb_to_packed_is_available() != 0
 */
void video_index_rgb_to_packed(void)
{
	assert( video_index_rgb_to_packed_is_available() );

	video_state_rgb_clear();

	/* set PACKED */
	video_state.mode.flags &= ~VIDEO_FLAGS_INDEX_MASK;
	video_state.mode.flags |= VIDEO_FLAGS_INDEX_PACKED;
}

/** Last active palette. */
static video_color video_palette[256];

/**
 * Get the current palette.
 */
video_color* video_palette_get(void)
{
	return video_palette;
}

/**
 * Set the palette.
 * \param palette palette data
 * \param start start palette index
 * \param count number of color to set
 * \param waitvsync if !=0 wait a vertical retrace
 */
video_error video_palette_set(video_color* palette, unsigned start, unsigned count, int waitvsync)
{
	assert( video_mode_is_active() );

	assert( video_index() == VIDEO_FLAGS_INDEX_PACKED );
	assert( count <= 256 );

	/* update the static palette */
	memcpy(video_palette + start, palette, count * sizeof(video_color));

	return video_current_driver()->palette8_set(palette,start,count,waitvsync);
}

/**
 * Compute a RGB value with a specific format
 * \param r,g,b RGB values 0-255
 * \param def RGB format
 * \return RGB nibble
 */
video_rgb video_rgb_make_from_def(unsigned r, unsigned g, unsigned b, video_rgb_def def)
{
	union video_rgb_def_union rgb;
	rgb.ordinal = def;
	return video_rgb_nibble_insert(r, video_rgb_shift_make_from_def(rgb.nibble.red_len,rgb.nibble.red_pos), video_rgb_mask_make_from_def(rgb.nibble.red_len,rgb.nibble.red_pos))
		| video_rgb_nibble_insert(g, video_rgb_shift_make_from_def(rgb.nibble.green_len,rgb.nibble.green_pos), video_rgb_mask_make_from_def(rgb.nibble.green_len,rgb.nibble.green_pos))
		| video_rgb_nibble_insert(b, video_rgb_shift_make_from_def(rgb.nibble.blue_len,rgb.nibble.blue_pos), video_rgb_mask_make_from_def(rgb.nibble.blue_len,rgb.nibble.blue_pos));
}

/**
 * Get a textual description of a RGB nibble definition.
 * Return a string description in the format red_len/red_pos,green_len/green_pos,blue_len/blue_pos.
 * \return pointer aat a static buffer
 */
const char* video_rgb_def_name_make(video_rgb_def def)
{
	static char buffer[63];
	union video_rgb_def_union rgb;
	rgb.ordinal = def;
	sprintf(buffer,"%d/%d,%d/%d,%d/%d",
		rgb.nibble.red_len,rgb.nibble.red_pos,
		rgb.nibble.green_len,rgb.nibble.green_pos,
		rgb.nibble.blue_len,rgb.nibble.blue_pos
	);
	return buffer;
}

/****************************************************************************/
/* Error */

/**
 * Set the description of the last error.
 * \note The description IS logged.
 */
void video_error_description_set(const char* text, ...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(video_state.error,text,arg);
	log_std(("video: set_error_description \""));
	video_log_va(text,arg);
	log_std(("\"\n"));
	va_end(arg);
}

/**
 * Set the description of the last error.
 * \note The description IS NOT logged.
 */
void video_error_description_nolog_set(const char* text, ...)
{
	va_list arg;
	va_start(arg,text);
	vsprintf(video_state.error,text,arg);
	va_end(arg);
}

/**
 * Add some text at the description of the last error.
 * \note The description IS NOT logged.
 */
void video_error_description_nolog_cat(const char* text, ...)
{
	va_list arg;
	char buffer[VIDEO_ERROR_DESCRIPTION_MAX];
	va_start(arg,text);
	vsprintf(buffer,text,arg);

	strncat(video_state.error,buffer,VIDEO_ERROR_DESCRIPTION_MAX);
	video_state.error[VIDEO_ERROR_DESCRIPTION_MAX-1] = 0;

	va_end(arg);
}

/****************************************************************************/
/* Log */

void video_log_modeline_cb(const char *text, unsigned pixel_clock, unsigned hde, unsigned hbs, unsigned hrs, unsigned hre, unsigned hbe, unsigned ht, unsigned vde, unsigned vbs, unsigned vrs, unsigned vre, unsigned vbe, unsigned vt, int hsync_pol, int vsync_pol, int doublescan, int interlace)
{
	const char* flag1 = hsync_pol ? " -hsync" : " +hsync";
	const char* flag2 = vsync_pol ? " -vsync" : " +vsync";
	const char* flag3 = doublescan ? " doublescan" : "";
	const char* flag4 = interlace ? " interlace" : "";
	video_log("%s %g %d %d %d %d %d %d %d %d %d %d %d %d%s%s%s%s\n",
		text, (double)pixel_clock / 1E6,
		hde, hbs, hrs, hre, hbe, ht,
		vde, vbs, vrs, vre, vbe, vt,
		flag1, flag2, flag3, flag4
	);
}

void video_log_modeline_c(const char *text, unsigned pixel_clock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int hsync_pol, int vsync_pol, int doublescan, int interlace)
{
	const char* flag1 = hsync_pol ? " -hsync" : " +hsync";
	const char* flag2 = vsync_pol ? " -vsync" : " +vsync";
	const char* flag3 = doublescan ? " doublescan" : "";
	const char* flag4 = interlace ? " interlace" : "";
	video_log("%s %g %d %d %d %d %d %d %d %d%s%s%s%s\n",
		text, (double)pixel_clock / 1E6,
		hde, hrs, hre, ht,
		vde, vrs, vre, vt,
		flag1, flag2, flag3, flag4
	);
}

void video_log(const char* text, ...)
{
	va_list arg;
	va_start(arg,text);
	video_log_va(text,arg);
	va_end(arg);
}

