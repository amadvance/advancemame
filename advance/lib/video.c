/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2008 Andrea Mazzoleni
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

#include "video.h"

#include "log.h"
#include "target.h"
#include "os.h"
#include "error.h"
#include "rgb.h"
#include "snstring.h"
#include "measure.h"

/***************************************************************************/
/* State */

/** State of the video support. */
struct video_state_struct video_state;

struct video_option_struct {
	adv_bool initialized; /**< Initialized flag. */
	adv_bool scan_single; /**< Allow singlescan modes. */
	adv_bool scan_double; /**< Allow doublescan modes. */
	adv_bool scan_interlace; /**< Allow interlace modes. */
	adv_bool fast_change; /**< Allow the fast change. */
	adv_bool mode_palette8; /**< Allow palette8 modes. */
	adv_bool mode_bgr8; /**< Allow bgr8 modes. */
	adv_bool mode_bgr15; /**< Allow bgr15 modes. */
	adv_bool mode_bgr16; /**< Allow bgr16 modes. */
	adv_bool mode_bgr24; /**< Allow bgr24 modes. */
	adv_bool mode_bgr32; /**< Allow bgr32 modes. */
	adv_bool mode_yuy2; /**< Allow yuy2 modes. */
	adv_output output; /**< Output mode. */
	adv_cursor cursor; /**< Cursor mode. */
	unsigned overlay_size; /**< Overlay size. 0 for auto selection. */
	char name[DEVICE_NAME_MAX]; /**< Name of the device. */
};

/** State of the video configuration options. */
static struct video_option_struct video_option;

unsigned char* (*video_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

static void video_color_def_adjust(adv_color_def def_ordinal)
{
	union adv_color_def_union def;

	video_state.color_def = def_ordinal;
	def.ordinal = def_ordinal;

	if (def.nibble.type == adv_color_type_rgb) {
		rgb_shiftmask_get(&video_state.rgb_red_shift, &video_state.rgb_red_mask, def.nibble.red_len, def.nibble.red_pos);
		rgb_shiftmask_get(&video_state.rgb_green_shift, &video_state.rgb_green_mask, def.nibble.green_len, def.nibble.green_pos);
		rgb_shiftmask_get(&video_state.rgb_blue_shift, &video_state.rgb_blue_mask, def.nibble.blue_len, def.nibble.blue_pos);
		video_state.rgb_red_len = def.nibble.red_len;
		video_state.rgb_green_len = def.nibble.green_len;
		video_state.rgb_blue_len = def.nibble.blue_len;
		video_state.rgb_mask_bit = rgb_wholemask_make_from_def(def_ordinal);
		video_state.rgb_high_bit = rgb_highmask_make_from_def(def_ordinal);
		video_state.rgb_low_bit = rgb_lowmask_make_from_def(def_ordinal);
	} else {
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
}

/***************************************************************************/
/* Fake Text */

static void video_fake_text_init(void)
{
	video_state.fake_text_map = 0;
	video_state.fake_text_last_map = 0;
	video_state.fake_text_dx = 0;
	video_state.fake_text_dy = 0;
}

static void video_fake_text_done(void)
{
	free(video_state.fake_text_map);
	free(video_state.fake_text_last_map);
	video_state.fake_text_map = 0;
	video_state.fake_text_last_map = 0;
	video_state.fake_text_dx = 0;
	video_state.fake_text_dy = 0;
}

static unsigned char* video_fake_text_write_line(unsigned y)
{
	assert(y < video_state.fake_text_dy);
	return video_state.fake_text_map + y * video_state.fake_text_dx * 2;
}

static void video_fake_text_adjust(void)
{

	video_write_line = video_fake_text_write_line;
	video_color_def_adjust(color_def_make(adv_color_type_text));

	video_state.fake_text_dx = video_state.mode.size_x / 8;
	video_state.fake_text_dy = video_state.mode.size_y / 16;

	free(video_state.fake_text_map);
	free(video_state.fake_text_last_map);
	video_state.fake_text_map = malloc(video_state.fake_text_dx * video_state.fake_text_dy * 2);
	video_state.fake_text_last_map = malloc(video_state.fake_text_dx * video_state.fake_text_dy * 2);

	memset(video_state.fake_text_map, 0, video_state.fake_text_dx * video_state.fake_text_dy * 2);
	memset(video_state.fake_text_last_map, 0, video_state.fake_text_dx * video_state.fake_text_dy * 2);

	video_state.mode.flags &= ~MODE_FLAGS_INDEX_MASK;
	video_state.mode.flags |= MODE_FLAGS_INDEX_TEXT;
}

static adv_color_rgb fake_text_palette[16] = {
{   0,  0,  0, 0 },
{ 192,  0,  0, 0 },
{   0, 192,  0, 0 },
{ 192, 192,  0, 0 },
{   0,  0, 192, 0 },
{ 192,  0, 192, 0 },
{   0, 192, 192, 0 },
{ 192, 192, 192, 0 },
{ 128, 128, 128, 0 },
{ 255, 128, 128, 0 },
{ 128, 255, 128, 0 },
{ 255, 255, 128, 0 },
{ 128, 128, 255, 0 },
{ 255, 128, 255, 0 },
{ 128, 255, 255, 0 },
{ 255, 255, 255, 0 }
};

#include "fonttxt.dat"

void video_fake_text_wait_vsync(void)
{
	unsigned x, y;
	unsigned i;
	unsigned color[16];
	unsigned size;
	unsigned color_def;
	unsigned char* (*write_line)(unsigned y);

	/* not available in graphics mode */
	if (!video_state.fake_text_map)
		return;

	color_def = video_current_driver()->color_def();
	write_line = *video_current_driver()->write_line;

	/* no change required */
	size = video_state.fake_text_dy * video_state.fake_text_dx * 2;
	if (memcmp(video_state.fake_text_map, video_state.fake_text_last_map, size) == 0)
		return;

	/* update the last buffer */
	memcpy(video_state.fake_text_last_map, video_state.fake_text_map, size);

	for(i=0;i<16;++i)
		color[i] = pixel_make_from_def(fake_text_palette[i].red, fake_text_palette[i].green, fake_text_palette[i].blue, color_def);

	video_write_lock();

	for(y=0;y<video_state.fake_text_dy;++y) {
		for(x=0;x<video_state.fake_text_dx;++x) {
			unsigned cx, cy;
			unsigned sx, sy;
			unsigned ch, at;
			unsigned c0, c1;
			unsigned char* bit;

			sx = x * 8;
			sy = y * 16;
			ch = video_state.fake_text_map[(y*video_state.fake_text_dx+x)*2];
			at = video_state.fake_text_map[(y*video_state.fake_text_dx+x)*2+1];
			c0 = color[(at >> 4) & 0xF];
			c1 = color[at & 0xF];
			bit = FONT + ch * 16;

			for(cy=0;cy<16;++cy) {
				unsigned char* line = write_line(sy + cy) + sx * 2;
				for(cx=0;cx<8;++cx) {
					unsigned c;
					if ((bit[cy] & (0x80 >> cx)) != 0)
						c = c1;
					else
						c = c0;
					line[0] = c;
					line[1] = c >> 8;
					line += 2;
				}
			}
		}
	}

	video_write_unlock(0, 0, 0, 0, 0);
}

static inline unsigned video_fake_text_font_size_x(void)
{
	return 8;
}

static inline unsigned video_fake_text_font_size_y(void)
{
	return 16;
}

static inline unsigned video_fake_text_virtual_x(void)
{
	return video_state.fake_text_dx * 8;
}

static inline unsigned video_fake_text_virtual_y(void)
{
	return video_state.fake_text_dy * 16;
}

static inline adv_error video_fake_text_display_set(unsigned offset, adv_bool waitvsync)
{
	return 0;
}

static inline unsigned video_fake_text_bytes_per_scanline(void)
{
	return video_state.fake_text_dx * 2;
}

static inline unsigned video_fake_text_bytes_per_page(void)
{
	return video_state.fake_text_dx * video_state.fake_text_dy * 2;
}

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
	video_option.mode_palette8 = 1;
	video_option.mode_bgr8 = 1;
	video_option.mode_bgr15 = 1;
	video_option.mode_bgr16 = 1;
	video_option.mode_bgr24 = 1;
	video_option.mode_bgr32 = 1;
	video_option.mode_yuy2 = 1;
	video_option.fast_change = 0;
	video_option.output = adv_output_auto;
	video_option.cursor = adv_cursor_auto;
	video_option.overlay_size = 0;
	sncpy(video_option.name, DEVICE_NAME_MAX, "auto");
}

static adv_conf_enum_int OPTION_OUTPUT[] = {
{ "auto", adv_output_auto },
{ "window", adv_output_window },
{ "fullscreen", adv_output_fullscreen },
{ "overlay", adv_output_overlay }
};

static adv_conf_enum_int OPTION_CURSOR[] = {
{ "auto", adv_cursor_auto },
{ "off", adv_cursor_off },
{ "on", adv_cursor_on },
};

/**
 * Register the configuration options.
 */
void video_reg(adv_conf* context, adv_bool auto_detect)
{
	conf_string_register_default(context, "device_video", auto_detect ? "auto" : "none");
	conf_bool_register_default(context, "device_video_singlescan", 1);
	conf_bool_register_default(context, "device_video_doublescan", 1);
	conf_bool_register_default(context, "device_video_interlace", 1);
	conf_bool_register_default(context, "device_video_fastchange", 0);
	conf_bool_register_default(context, "device_color_palette8", 1);
	conf_bool_register_default(context, "device_color_bgr8", 1);
	conf_bool_register_default(context, "device_color_bgr15", 1);
	conf_bool_register_default(context, "device_color_bgr16", 1);
	conf_bool_register_default(context, "device_color_bgr24", 1);
	conf_bool_register_default(context, "device_color_bgr32", 1);
	conf_bool_register_default(context, "device_color_yuy2", 1);
	conf_int_register_enum_default(context, "device_video_output", conf_enum(OPTION_OUTPUT), adv_output_auto);
	conf_int_register_enum_default(context, "device_video_cursor", conf_enum(OPTION_CURSOR), adv_cursor_auto);
	conf_string_register_default(context, "device_video_overlaysize", "auto");
}

/**
 * Register a video driver.
 * \note Call before adv_video_init().
 */
void video_reg_driver(adv_conf* context, adv_video_driver* driver)
{
	assert(!video_is_active());

	assert(video_state.driver_mac < DEVICE_MAX);

	log_std(("video: register driver %s\n", driver->name));

	video_state.driver_map[video_state.driver_mac] = driver;
	video_state.driver_map[video_state.driver_mac]->reg(context);

	++video_state.driver_mac;
}

/**
 * Count the active video drivers.
 * This fucntion automatically skip disabled drivers.
 */
unsigned video_driver_vector_max(void)
{
	unsigned count = 0;
	unsigned i;
	for(i=0;i<video_state.driver_mac;++i)
		if (video_state.driver_map[i] != 0)
			++count;
	return count;
}

/**
 * Return the 'n video driver.
 * This function automatically skip disabled drivers.
 */
const adv_video_driver* video_driver_vector_pos(unsigned i)
{
	unsigned count = 0;
	unsigned j;
	for(j=0;j<video_state.driver_mac;++j) {
		if (video_state.driver_map[j] != 0) {
			if (i == count)
				return video_state.driver_map[j];
			++count;
		}
	}
	return 0;
}

/**
 * Initialize the video system.
 */
adv_error adv_video_init(void)
{
	unsigned i;
	adv_bool at_least_one;
	adv_bool forward;

	assert(!video_is_active());

	log_std(("video: adv_video_init\n"));

	if (!video_option.initialized) {
		video_default();
	}

	/* store the error prefix */
	error_nolog_set("Unable to initialize the video driver. The errors are:\n");

	/* enable all the video driver */

#ifdef __MSDOS__ /* TODO convert the order to forward removing the dependencies */
	/* backward order */
	/* some MSDOS drivers have dependencies to respect */
	forward = 0;
#else
	/* forward order */
	/* some low priority drivers are disabled if some high priority are enabled */
	forward = 1;
#endif

	at_least_one = 0;
	for(i=0;i<video_state.driver_mac;++i) {
		unsigned j;
		const adv_device* dev;

		if (forward)
			j = i;
		else
			j = video_state.driver_mac - 1 - i;

		assert(video_state.driver_map[j]->mode_size() <= MODE_DRIVER_MODE_SIZE_MAX);

		dev = device_match(video_option.name, (const adv_driver*)video_state.driver_map[j], 0);

		error_cat_set(video_state.driver_map[j]->name, 1);

		if (dev && video_state.driver_map[j]->init(dev->id, video_option.output, video_option.overlay_size, video_option.cursor) == 0) {
			log_std(("video: select driver %s\n", video_state.driver_map[j]->name));
			at_least_one = 1;
		} else {
			video_state.driver_map[j] = 0; /* deactivate the driver */
		}
	}

	error_cat_set(0, 0);

	log_std(("video: adv_video_init report (multiline):\n%s\n", error_get()));

	if (!at_least_one) {
		log_std(("video: no video driver activated\n"));
		return -1;
	}

	error_reset();

	video_fake_text_init();

	/* enable */
	video_state.active = 1;
	video_state.mode_active = 0;

	video_state.old_mode_required = 0;

	return 0;
}

/**
 * Deinitialize the video system.
 */
void adv_video_done(void)
{
	unsigned i;

	assert(video_is_active() && !video_mode_is_active());

	log_std(("video: adv_video_done\n"));

	/* safer */
	if (video_mode_is_active())
		video_mode_done(1);

	/* disable */
	video_state.active = 0;

	/* disable all the video driver */
	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i]) {
			video_state.driver_map[i]->done();
		}
	}

	video_fake_text_done();

	/* reset the driver list */
	video_state.driver_mac = 0;
}

/**
 * Emergency abort.
 * Callable in any context.
 */
void adv_video_abort(void)
{
	if (video_is_active()) {
		if (video_mode_is_active())
			video_mode_done(1);
		adv_video_done();
	}
}

/***************************************************************************/
/* Options */

/**
 * Load the video configuration.
 * \param context Configuration context to use.
 * \param driver_ignore List of driver name to ignore (separated by space).
 */
adv_error video_load(adv_conf* context, const char* driver_ignore)
{
	unsigned i;
	int at_least_one;
	const char* s;

	if (video_state.driver_mac == 0) {
		error_set("No video driver was compiled in.");
		return -1;
	}

	/* options must be loaded before initialization */
	assert(!video_is_active());

	video_option.initialized = 1;
	video_option.scan_single = conf_bool_get_default(context, "device_video_singlescan");
	video_option.scan_double = conf_bool_get_default(context, "device_video_doublescan");
	video_option.scan_interlace = conf_bool_get_default(context, "device_video_interlace");
	video_option.fast_change = conf_bool_get_default(context, "device_video_fastchange");
	video_option.mode_palette8 = conf_bool_get_default(context, "device_color_palette8");
	video_option.mode_bgr8 = conf_bool_get_default(context, "device_color_bgr8");
	video_option.mode_bgr15 = conf_bool_get_default(context, "device_color_bgr15");
	video_option.mode_bgr16 = conf_bool_get_default(context, "device_color_bgr16");
	video_option.mode_bgr24 = conf_bool_get_default(context, "device_color_bgr24");
	video_option.mode_bgr32 = conf_bool_get_default(context, "device_color_bgr32");
	video_option.mode_yuy2 = conf_bool_get_default(context, "device_color_yuy2");
	video_option.output = conf_int_get_default(context, "device_video_output");
	video_option.cursor = conf_int_get_default(context, "device_video_cursor");
	s = conf_string_get_default(context, "device_video_overlaysize");
	if (strcmp(s, "auto") == 0) {
		video_option.overlay_size = 0;
	} else {
		video_option.overlay_size = atoi(s);
	}

	sncpy(video_option.name, DEVICE_NAME_MAX, conf_string_get_default(context, "device_video"));

	if (device_check("device_video", video_option.name, (const adv_driver**)video_state.driver_map, video_state.driver_mac, driver_ignore) != 0) {
		return -1;
	}

	/* load the specific driver options */
	at_least_one = 0;
	for(i=0;i<video_state.driver_mac;++i) {
		const adv_device* dev;

		dev = device_match(video_option.name, (const adv_driver*)video_state.driver_map[i], 0);

		if (dev)
			at_least_one = 1;

		if (video_state.driver_map[i]->load(context) != 0)
			return -1;
	}

	if (!at_least_one) {
		device_error("device_video", video_option.name, (const adv_driver**)video_state.driver_map, video_state.driver_mac);
		return -1;
	}

	return 0;
}

/***************************************************************************/
/* Mode */

/**
 * Change the drivers flags with the capability flags.
 */
unsigned video_capability_flags(unsigned flags)
{
	if (!video_option.scan_double)
		flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
	if (!video_option.scan_single)
		flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN;
	if (!video_option.scan_interlace)
		flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
	if (!video_option.mode_palette8)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
	if (!video_option.mode_bgr8)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR8;
	if (!video_option.mode_bgr15)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR15;
	if (!video_option.mode_bgr16)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR16;
	if (!video_option.mode_bgr24)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR24;
	if (!video_option.mode_bgr32)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR32;
	if (!video_option.mode_yuy2)
		flags &= ~VIDEO_DRIVER_FLAGS_MODE_YUY2;

	return flags;
}

/**
 * Change the drivers flags with the fake flags.
 */
unsigned video_fake_flags(unsigned flags)
{
	/* add BGR8 derived from PALETTE8 */
	if ((flags & VIDEO_DRIVER_FLAGS_MODE_PALETTE8) != 0)
		flags |= VIDEO_DRIVER_FLAGS_MODE_BGR8;

	/* add TEXT derived from BGR16 */
	if ((flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) != 0)
		flags |= VIDEO_DRIVER_FLAGS_MODE_TEXT;

	return flags;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

/**
 * Compare two video mode.
 * \return Like strcmp.
 */
int video_mode_compare(const adv_mode* a, const adv_mode* b)
{
	/* we need to compare also the fake rgb flags not present in the driver data */
	unsigned flags_a = mode_flags(a) & MODE_FLAGS_INTERNAL_MASK;
	unsigned flags_b = mode_flags(b) & MODE_FLAGS_INTERNAL_MASK;
	COMPARE(mode_driver(a), mode_driver(b));
	COMPARE(flags_a, flags_b);
	return mode_driver(a)->mode_compare(a->driver_mode, b->driver_mode);
}

/**
 * Unset a video mode.
 * \param restore If the previous video mode must be restored.
 */
void video_mode_done(adv_bool restore)
{
	assert(video_mode_is_active());

	video_current_driver()->mode_done(restore);

	/* disable */
	video_write_line = 0;
	video_state.mode_active = 0;
}

/**
 * Reset the variable describing a video mode.
 */
void mode_reset(adv_mode* mode)
{
	memset(mode, 0, sizeof(adv_mode));
	sncpy(mode->name, MODE_NAME_MAX, "unamed");
}

static void log_clock(void)
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

	log_std(("video: measured/expected vert clock: %.2f/%.2f = %g (err %g%%)\n", video_measured_vclock(), video_vclock(), factor, error * 100.0));
}

static void video_wait_vsync_for_measure(void)
{
	if (video_current_driver()->write_lock && video_current_driver()->write_unlock) {
		video_current_driver()->write_lock();
		video_current_driver()->write_unlock(0, 0, 0, 0, 1);
	}
	if (video_current_driver()->wait_vsync)
		video_current_driver()->wait_vsync();
}

/**
 * Set a video mode.
 */
adv_error video_mode_set(adv_mode* mode)
{
	unsigned color_def;
	double vsync_time;

	assert(video_is_active());

	log_std(("video: mode_set %s %dx%d %s %.2f/%.2f\n", mode_name(mode), mode_size_x(mode), mode_size_y(mode), index_name(mode_index(mode)), (double)mode_hclock(mode) / 1E3, (double)mode_vclock(mode)));

	video_state.old_mode_required = 1; /* the mode will change */

	/* check for a mode change without a driver change */
	if (video_mode_is_active()
		&& video_current_driver() == mode_driver(mode)
		&& ((video_driver_flags() & VIDEO_DRIVER_FLAGS_INTERNAL_DANGEROUSCHANGE) == 0 || video_option.fast_change)
		&& mode_driver(mode)->mode_change != 0
	) {
		/* change the mode */
		if (mode_driver(mode)->mode_change(&mode->driver_mode) != 0) {
			video_state.mode_active = 0;
			video_write_line = 0;
			return -1;
		}
	} else {
		if (video_mode_is_active())
			video_mode_done(1);

		/* set the mode */
		if (mode_driver(mode)->mode_set(&mode->driver_mode) != 0)
			return -1;
	}

	video_state.mode_active = 1;
	video_state.mode = *mode;
	video_state.mode_original = *mode;
	video_state.virtual_x = video_current_driver()->virtual_x();
	video_state.virtual_y = video_current_driver()->virtual_y();
	video_write_line = *video_current_driver()->write_line;
	color_def = video_current_driver()->color_def();
	video_color_def_adjust(color_def);

	/* adjust for fake rgb modes */
	if ((mode_flags(mode) & MODE_FLAGS_INTERNAL_FAKERGB) != 0) {
		log_std(("video: convert from palette8 to bgr8\n"));
		video_index_packed_to_rgb(0);
	}

	if ((mode_flags(mode) & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		log_std(("video: convert from bgr16 to text\n"));
		video_fake_text_adjust();
	}

	vsync_time = adv_measure_step(video_wait_vsync_for_measure, 1 / 300.0, 1 / 10.0, 11);

	if (vsync_time > 0) {
		video_state.measured_vclock = 1 / vsync_time;
		log_std(("video: measured vsync %g\n", (double)video_state.measured_vclock));
	} else {
		video_state.measured_vclock = 0;
		log_std(("WARNING:video: vsync time not measured\n"));
	}

	log_clock();

	return 0;
}

/**
 * Grab the current video mode.
 */
adv_error video_mode_grab(adv_mode* mode)
{
	unsigned char driver_mode[MODE_DRIVER_MODE_SIZE_MAX];
	unsigned i;

	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i] && video_state.driver_map[i]->mode_grab) {
			if (video_state.driver_map[i]->mode_grab(&driver_mode)==0 && video_state.driver_map[i]->mode_import(mode, &driver_mode)==0) {
				return 0;
			}
		}
	}

	return -1;
}

/**
 * Generate a video mode.
 * \param mode Destination of the new video mode.
 * \param crtc Required crtc of the new video mode. It can be a fake crtc (with most valuess at 0).
 * \param flags Required flags of the new video mode.
 */
adv_error video_mode_generate(adv_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	unsigned char driver_mode[MODE_DRIVER_MODE_SIZE_MAX];
	unsigned driver_flags;
	unsigned i;

	/* store the error prefix */
	error_nolog_set("No driver is capable to do the specified video mode. The errors are:\n");

	/* try any enabled driver */
	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i]) {
			driver_flags = video_capability_flags(video_state.driver_map[i]->flags());

			error_cat_set(video_state.driver_map[i]->name, 1);

			/* allow also faked crtc (with all values at 0), the driver */
			/* must reject them if they are not allowed  */
			if (video_state.driver_map[i]->mode_generate(&driver_mode, crtc, flags)==0
				&& video_state.driver_map[i]->mode_import(mode, &driver_mode)==0) {
				error_cat_set(0, 0);
				error_reset();
				log_pedantic(("video: using driver %s for mode %s\n", video_state.driver_map[i]->name, mode->name));
				return 0;
			}

			/* convert a BGR8 mode in a PALETTE8 mode if the driver doesn't support BGR8 */
			if ((flags & MODE_FLAGS_INDEX_MASK) == MODE_FLAGS_INDEX_BGR8
				&& (driver_flags & VIDEO_DRIVER_FLAGS_MODE_PALETTE8) != 0
				&& (driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR8) == 0
			) {
				unsigned fake_flags = (flags & ~MODE_FLAGS_INDEX_MASK) | MODE_FLAGS_INDEX_PALETTE8;
				if (video_state.driver_map[i]->mode_generate(&driver_mode, crtc, fake_flags)==0 && video_state.driver_map[i]->mode_import(mode, &driver_mode)==0) {
					error_cat_set(0, 0);
					error_reset();
					/* adjust the flags */
					mode->flags = (mode->flags & ~MODE_FLAGS_INDEX_MASK) | MODE_FLAGS_INDEX_BGR8 | MODE_FLAGS_INTERNAL_FAKERGB;
					log_pedantic(("video: using driver %s for mode %s (fake rgb)\n", video_state.driver_map[i]->name, mode->name));
					return 0;
				}
			}

			/* convert a TEXT mode in a BGR16 mode if the driver doesn't support */
			/* text modes. Only on a Window Manager. */
			if ((flags & MODE_FLAGS_INDEX_MASK) == MODE_FLAGS_INDEX_TEXT
				&& (driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) != 0
				&& (driver_flags & VIDEO_DRIVER_FLAGS_MODE_TEXT) == 0
				&& (driver_flags & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW) != 0
			) {
				unsigned fake_flags = (flags & ~MODE_FLAGS_INDEX_MASK) | MODE_FLAGS_INDEX_BGR16;
				if (video_state.driver_map[i]->mode_generate(&driver_mode, crtc, fake_flags)==0 && video_state.driver_map[i]->mode_import(mode, &driver_mode)==0) {
					error_cat_set(0, 0);
					error_reset();
					/* adjust the flags */
					mode->flags = (mode->flags & ~MODE_FLAGS_INDEX_MASK) | MODE_FLAGS_INDEX_BGR16 | MODE_FLAGS_INTERNAL_FAKETEXT;
					log_pedantic(("video: using driver %s for mode %s (fake text)\n", video_state.driver_map[i]->name, mode->name));
					return 0;
				}
			}
		}
	}

	error_cat_set(0, 0);

	return -1;
}

adv_error video_mode_generate_check(const char* driver, unsigned driver_flags, unsigned hstep, unsigned hvmax, const adv_crtc* crtc, unsigned flags)
{
	/* filter the drivers falgs */
	driver_flags = video_capability_flags(driver_flags);

	if (crtc->hde % hstep != 0 || crtc->hrs % hstep != 0 || crtc->hre % hstep != 0 || crtc->ht % hstep != 0) {
		error_nolog_set("Horizontal crtc values are not a %d dot multiple.\n", hstep);
		return -1;
	}
	if (crtc->ht >= hvmax || crtc->vt >= hvmax) {
		error_nolog_set("Horizontal or vertical crtc total value bigger than %d.\n", hvmax);
		return -1;
	}

	if (crtc_is_interlace(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) == 0) {
			error_nolog_set("Mode interlace not supported.\n");
			return -1;
		}
	}

	if (crtc_is_doublescan(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) == 0) {
			error_nolog_set("Mode doublescan not supported.\n");
			return -1;
		}
	}

	if (crtc_is_singlescan(crtc)) {
		if ((driver_flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN) == 0) {
			error_nolog_set("Mode singlescan not supported.\n");
			return -1;
		}
	}

	switch (flags & MODE_FLAGS_INDEX_MASK) {
	case MODE_FLAGS_INDEX_PALETTE8 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_PALETTE8) == 0) {
			error_nolog_set("Mode palette8 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR8 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR8) == 0) {
			error_nolog_set("Mode bgr8 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR15 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR15) == 0) {
			error_nolog_set("Mode bgr15 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR16 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) == 0) {
			error_nolog_set("Mode bgr16 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR24 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR24) == 0) {
			error_nolog_set("Mode bgr24 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR32 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_BGR32) == 0) {
			error_nolog_set("Mode bgr32 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_YUY2 :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_YUY2) == 0) {
			error_nolog_set("Mode yuy2 not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_TEXT :
		if ((driver_flags & VIDEO_DRIVER_FLAGS_MODE_TEXT) == 0) {
			error_nolog_set("Mode text not supported.\n");
			return -1;
		}
		break;
	default:
		error_nolog_set("Unknown index mode.\n");
		return -1;
	}

	return 0;
}

/**
 * Return the flags of the drivers used to generate video modes.
 * This function add the flags of all the drivers usable.
 * \param flags_or Required flags with OR.
 * \param flags_and Required flags with AND.
 */
unsigned video_mode_generate_driver_flags(unsigned flags_or, unsigned flags_and)
{
	unsigned flags;
	unsigned equality_mask;
	unsigned i;

	flags = 0;

	log_debug(("video: video_mode_generate_driver_flags(flags_or:%x,flags_and:%x)\n", flags_or, flags_and));

	/* add flags only with the same capabilities */
	equality_mask = VIDEO_DRIVER_FLAGS_PROGRAMMABLE_MASK | VIDEO_DRIVER_FLAGS_OUTPUT_MASK;

	for(i=0;i<video_state.driver_mac;++i) {
		if (video_state.driver_map[i]) {
			/* limit the flags with the video internal options */
			unsigned driver_flags = video_fake_flags(video_capability_flags(video_state.driver_map[i]->flags()));
			if ((driver_flags & flags_or) != 0
				&& (driver_flags & flags_and) == flags_and) {
				if (!flags || (flags & equality_mask) == (driver_flags & equality_mask)) {
					log_debug(("video: video_mode_generate_driver_flags() add driver %s\n", video_state.driver_map[i]->name));
					flags |= driver_flags;
				}
			}
		}
	}

	log_debug(("video: video_mode_generate_driver_flags() = %x\n", flags));

	return flags;
}

/**
 * Reset the startup mode.
 * \note After this call none video mode is active. video_mode_active()==0
 */
void video_mode_restore(void)
{
	log_std(("video: video_mode_restore\n"));

	assert(video_is_active());

	if (video_mode_is_active())
		video_mode_done(1);

	/* if no video mode change return */
	if (!video_state.old_mode_required)
		return;

	log_std(("video: mode_reset\n"));

	/* os mode reset */
	target_mode_reset();
}

/**
 * Wait for a vertical retrace.
 * \note if no video mode is active the VGA version is used
 */
void video_wait_vsync(void)
{
	assert(video_mode_is_active());

	video_current_driver()->wait_vsync();

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		video_fake_text_wait_vsync();
	}
}

/**
 * X size of the font for text mode.
 */
unsigned video_font_size_x(void)
{
	assert(video_mode_is_active() && video_is_text());

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		return video_fake_text_font_size_x();
	} else {
		assert(video_current_driver()->font_size_x != 0);
		return video_current_driver()->font_size_x();
	}
}

/**
 * Y size of the font for text mode.
 */
unsigned video_font_size_y(void)
{
	assert(video_mode_is_active() && video_is_text());

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		return video_fake_text_font_size_y();
	} else {
		assert(video_current_driver()->font_size_y != 0);
		return video_current_driver()->font_size_y();
	}
}

/**
 * Horizontal virtual size of the current video mode.
 */
unsigned video_virtual_x(void)
{
	assert(video_mode_is_active());

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0)
		return video_fake_text_virtual_x();
	else
		return video_current_driver()->virtual_x();
}

/**
 * Vertical virtual size of the current video mode.
 */
unsigned video_virtual_y(void)
{
	assert(video_mode_is_active());
	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0)
		return video_fake_text_virtual_y();
	else
		return video_current_driver()->virtual_y();
}

/**
 * Set the screen display start.
 * \param waitvsync If a wait vsync is required before the display start change.
 * The display start change may be immediate (asyncronous) or delayed
 * to the next wait retrace (syncronous) depending of the video
 * driver used.
 */
adv_error video_display_set(unsigned offset, adv_bool waitvsync)
{
	assert(video_mode_is_active());

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		return video_fake_text_display_set(offset, waitvsync);
	} else {
		return video_current_driver()->scroll(offset, waitvsync);
	}
}

unsigned video_bytes_per_scanline(void)
{
	assert(video_mode_is_active());

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		return video_fake_text_bytes_per_scanline();
	} else {
		return video_current_driver()->bytes_per_scanline();
	}
}

unsigned video_bytes_per_page(void)
{
	unsigned bytes_per_page = video_size_y() * video_bytes_per_scanline();

	if ((video_flags() & MODE_FLAGS_INTERNAL_FAKETEXT) != 0) {
		return video_fake_text_bytes_per_page();
	} else {
		return video_current_driver()->adjust_bytes_per_page(bytes_per_page);
	}
}

/***************************************************************************/
/* Put */

void video_put_char(unsigned x, unsigned y, char c, unsigned color)
{
	assert(video_mode_is_active() && video_is_text());

	if (x < video_virtual_x() / video_font_size_x()
		&& y < video_virtual_y() / video_font_size_y()) {
		unsigned char* line = video_write_line(y) + 2 * x;
		line[0] = c;
		line[1] = color;
	}
}

static inline void video_put_pixel_noclip(unsigned x, unsigned y, unsigned color)
{
	unsigned char* line = video_write_line(y);
	switch (video_bytes_per_pixel()) {
	case 1 :
		line[x] = color & 0xFF;
		break;
	case 2 :
		line += 2*x;
		line[0] = color & 0xFF;
		line[1] = (color >> 8) & 0xFF;
		break;
	case 3 :
		line += 3*x;
		line[0] = color & 0xFF;
		line[1] = (color >> 8) & 0xFF;
		line[2] = (color >> 16) & 0xFF;
		break;
	case 4 :
		line += 4*x;
		line[0] = color & 0xFF;
		line[1] = (color >> 8) & 0xFF;
		line[2] = (color >> 16) & 0xFF;
		line[3] = (color >> 24) & 0xFF;
		break;
	}
}

void video_put_pixel(unsigned x, unsigned y, unsigned color)
{
	assert(video_mode_is_active() && video_is_graphics());
	if (x < video_virtual_x() && y < video_virtual_y())
		video_put_pixel_noclip(x, y, color);
}

/****************************************************************************/
/* Color */

/** Convert one video mode from PACKED to RGB. */
void video_index_packed_to_rgb(int waitvsync)
{
	unsigned bit_r;
	unsigned bit_g;
	unsigned bit_b;

	adv_color_rgb* palette;
	unsigned i;
	unsigned colors = 1 << video_bits_per_pixel();

	bit_b = video_bits_per_pixel() / 3;
	bit_r = (video_bits_per_pixel() - bit_b) / 2;
	bit_g = video_bits_per_pixel() - bit_b - bit_r;

	/* set the palette mode */
	video_state.mode.flags &= ~MODE_FLAGS_INDEX_MASK;
	video_state.mode.flags |= MODE_FLAGS_INDEX_PALETTE8;

	/* set the new color definition */
	video_color_def_adjust(color_def_make_rgb_from_sizelenpos(1, bit_r, bit_b+bit_g, bit_g, bit_b, bit_b, 0));

	palette = malloc(colors * sizeof(adv_color_rgb));
	assert(palette);

	for(i=0;i<colors;++i) {
		palette[i].red = video_red_get_approx(i);
		palette[i].green = video_green_get_approx(i);
		palette[i].blue = video_blue_get_approx(i);
	}

	video_palette_set(palette, 0, colors, waitvsync);

	free(palette);

	/* set the RGB mode */
	video_state.mode.flags &= ~MODE_FLAGS_INDEX_MASK;
	video_state.mode.flags |= MODE_FLAGS_INDEX_BGR8 | MODE_FLAGS_INTERNAL_FAKERGB;
}

adv_bool video_index_rgb_to_packed_is_available(void)
{
	return (video_state.mode.flags & MODE_FLAGS_INTERNAL_FAKERGB) != 0;
}

adv_bool video_index_packed_to_rgb_is_available(void)
{
	return video_index() == MODE_FLAGS_INDEX_PALETTE8;
}

/**
 * Convert a RGB video mode to PACKED
 * \note This function can be called only if video_index_rgb_to_packed_is_available() != 0
 */
void video_index_rgb_to_packed(void)
{
	assert(video_index_rgb_to_packed_is_available());

	video_color_def_adjust(color_def_make(adv_color_type_palette));

	/* restore the palette mode */
	video_state.mode.flags &= ~(MODE_FLAGS_INDEX_MASK | MODE_FLAGS_INTERNAL_FAKERGB);
	video_state.mode.flags |= MODE_FLAGS_INDEX_PALETTE8;
}

/** Last active palette. */
static adv_color_rgb video_palette[256];

/**
 * Get the current palette.
 */
adv_color_rgb* video_palette_get(void)
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
adv_error video_palette_set(adv_color_rgb* palette, unsigned start, unsigned count, int waitvsync)
{
	assert(video_mode_is_active());

	assert(video_index() == MODE_FLAGS_INDEX_PALETTE8);
	assert(start + count <= 256);

	/* update the static palette */
	memcpy(video_palette + start, palette, count * sizeof(adv_color_rgb));

	return video_current_driver()->palette8_set(palette, start, count, waitvsync);
}

