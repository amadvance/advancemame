/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005 Andrea Mazzoleni
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
 */

#include "portable.h"

#include "text.h"
#include "common.h"
#include "play.h"

#include "advance.h"

#include <list>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
#include <deque>
#include <cmath>

using namespace std;

// -------------------------------------------------------------------------
// Orientation/Size

static unsigned int_orientation = 0; // orientation flags
static unsigned int_font_dx; // font width
static unsigned int_font_dy; // font height
static adv_font* int_font; // font (already orientation corrected)

static inline void swap(unsigned& a, unsigned& b)
{
	unsigned t = a;
	a = b;
	b = t;
}

static inline void swap(int& a, int& b)
{
	int t = a;
	a = b;
	b = t;
}

void int_invrotate(int& x, int& y, int& dx, int& dy)
{
	if (int_orientation & ADV_ORIENTATION_FLIP_X) {
		x = video_size_x() - x - dx;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_Y) {
		y = video_size_y() - y - dy;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}
}

void int_rotate(int& x, int& y, int& dx, int& dy)
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_X) {
		x = video_size_x() - x - dx;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_Y) {
		y = video_size_y() - y - dy;
	}
}

int int_dx_get()
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY)
		return video_size_y();
	else
		return video_size_x();
}

int int_dy_get()
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY)
		return video_size_x();
	else
		return video_size_y();
}

int int_font_dx_get()
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY)
		return int_font_dy;
	else
		return int_font_dx;
}

int int_font_dx_get(const string& s)
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY)
		return adv_font_sizey_string(int_font, s.c_str(), s.c_str() + s.length());
	else
		return adv_font_sizex_string(int_font, s.c_str(), s.c_str() + s.length());
}

int int_font_dy_get()
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY)
		return int_font_dx;
	else
		return int_font_dy;
}

// -----------------------------------------------------------------------
// Joystick

static void int_joystick_reg(adv_conf* config_context)
{
	joystickb_reg(config_context, 1);
	joystickb_reg_driver_all(config_context);
}

static void int_joystick_unreg()
{
}

static bool int_joystick_load(adv_conf* config_context)
{
	if (joystickb_load(config_context) != 0)
		return false;
	return true;
}

static bool int_joystick_init()
{
	if (joystickb_init() != 0)
		return false;
	return true;
}

static void int_joystick_done()
{
	joystickb_done();
}

static void int_joystick_button_raw_poll()
{
	for(int i=0;i<joystickb_count_get();++i) {
		for(int j=0;j<joystickb_button_count_get(i);++j) {
			if (joystickb_button_get(i, j)) {
				switch (j) {
				case 0 :
					event_push(EVENT_ENTER);
					break;
				case 1 :
					event_push(EVENT_ESC);
					break;
				case 2 :
					event_push(EVENT_MENU);
					break;
				case 3 :
					event_push(EVENT_SPACE);
					break;
				case 4 :
					event_push(EVENT_MODE);
					break;
				case 5 :
					event_push(EVENT_PREVIEW);
					break;
				case 6 :
					event_push(EVENT_PGUP);
					break;
				case 7 :
					event_push(EVENT_PGDN);
					break;
				}
			}
		}
	}
}

static void int_joystick_move_raw_poll()
{
	for(int i=0;i<joystickb_count_get();++i) {
		for(int j=0;j<joystickb_stick_count_get(i);++j) {
			if (joystickb_stick_axe_count_get(i, j) > 0) {
				if (joystickb_stick_axe_digital_get(i, j, 0, 0))
					event_push(EVENT_RIGHT);
				if (joystickb_stick_axe_digital_get(i, j, 0, 1))
					event_push(EVENT_LEFT);
			}
			if (joystickb_stick_axe_count_get(i, j) > 1) {
				if (joystickb_stick_axe_digital_get(i, j, 1, 0))
					event_push(EVENT_DOWN);
				if (joystickb_stick_axe_digital_get(i, j, 1, 1))
					event_push(EVENT_UP);
			}
		}
	}
}

// -----------------------------------------------------------------------
// Key

static void int_key_reg(adv_conf* config_context)
{
	keyb_reg(config_context, 1);
	keyb_reg_driver_all(config_context);
}

static void int_key_unreg()
{
}

static bool int_key_load(adv_conf* config_context)
{
	if (keyb_load(config_context) != 0)
		return false;

	return true;
}

static bool int_key_init(bool disable_special)
{
	if (keyb_init(disable_special) != 0)
		return false;

	return true;
}

static void int_key_done()
{
	keyb_done();
}

static bool int_key_enable()
{
	if (keyb_enable(1) != 0) {
		return false;
	}

	if (mouseb_enable() != 0) {
		keyb_disable();
		return false;
	}

	if (joystickb_enable() != 0) {
		mouseb_disable();
		keyb_disable();
		return false;
	}

	return true;
}

static void int_key_disable()
{
	joystickb_disable();
	mouseb_disable();
	keyb_disable();
}

// -----------------------------------------------------------------------
// Mouse

static int int_mouse_delta; // mouse delta for a movement
static int int_mouse_pos_x; // mouse x position
static int int_mouse_pos_y; // mouse y position

static void int_mouse_reg(adv_conf* config_context)
{
	mouseb_reg(config_context, 0);
	mouseb_reg_driver_all(config_context);
	conf_int_register_limit_default(config_context, "mouse_delta", 1, 1000, 100);
}

static void int_mouse_unreg()
{
}

static bool int_mouse_load(adv_conf* config_context)
{
	int_mouse_pos_x = 0;
	int_mouse_pos_y = 0;
	int_mouse_delta = conf_int_get_default(config_context, "mouse_delta");

	if (mouseb_load(config_context) != 0)
		return false;

	return true;
}

static bool int_mouse_init()
{
	if (mouseb_init() != 0)
		return false;

	return true;
}

static void int_mouse_done()
{
	mouseb_done();
}

static void int_mouse_button_raw_poll()
{
	for(int i=0;i<mouseb_count_get();++i) {
		if (mouseb_button_count_get(i) > 0 && mouseb_button_get(i, 0))
			event_push(EVENT_ENTER);

		if (mouseb_button_count_get(i) > 1 && mouseb_button_get(i, 1))
			event_push(EVENT_ESC);

		if (mouseb_button_count_get(i) > 2 && mouseb_button_get(i, 2))
			event_push(EVENT_MENU);
	}
}

static void int_mouse_move_raw_poll()
{
	for(int i=0;i<mouseb_count_get();++i) {
		int x, y;

		x = 0;
		y = 0;

		if (mouseb_axe_count_get(i) > 0)
			int_mouse_pos_x += mouseb_axe_get(i, 0);
		if (mouseb_axe_count_get(i) > 1)
			int_mouse_pos_y += mouseb_axe_get(i, 1);
	}

	if (int_mouse_pos_x >= int_mouse_delta) {
		int_mouse_pos_x -= int_mouse_delta;
		event_push_repeat(EVENT_RIGHT);
	}

	if (int_mouse_pos_x <= -int_mouse_delta) {
		int_mouse_pos_x += int_mouse_delta;
		event_push_repeat(EVENT_LEFT);
	}

	if (int_mouse_pos_y >= int_mouse_delta) {
		int_mouse_pos_y -= int_mouse_delta;
		event_push_repeat(EVENT_DOWN);
	}

	if (int_mouse_pos_y <= -int_mouse_delta) {
		int_mouse_pos_y += int_mouse_delta;
		event_push_repeat(EVENT_UP);
	}
}

// -------------------------------------------------------------------------
// Video mode choice

#define DEFAULT_GRAPH_MODE "default_graph" // default video mode

static unsigned int_mode_sizex; // requested mode size
static unsigned int_mode_sizey; // requested mode size
static adv_mode int_current_mode; // selected video mode
static adv_monitor int_monitor; // monitor info
static adv_generate_interpolate_set int_interpolate;
static adv_crtc_container int_modelines;
static bool int_has_clock = false;
static bool int_has_generate = false;

// comparing for graphics mode
bool int_mode_graphics_less(const adv_mode* A, const adv_mode* B)
{
	int areaA = A->size_x * A->size_y;
	int areaB = B->size_x * B->size_y;

	int difA = abs(areaA - static_cast<int>(int_mode_sizex*int_mode_sizey));
	int difB = abs(areaB - static_cast<int>(int_mode_sizex*int_mode_sizey));

	return difA < difB;
}

static bool int_mode_find(bool& mode_found, unsigned index, adv_crtc_container& modelines)
{
	adv_crtc_container_iterator i;
	adv_error err;

	log_std(("text: searching for mode %dx%d, index %u.\n", int_mode_sizex, int_mode_sizey, index));

	// search the default name
	for(crtc_container_iterator_begin(&i, &modelines);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		const adv_crtc* crtc = crtc_container_iterator_get(&i);
		if (strcmp(crtc->name, DEFAULT_GRAPH_MODE)==0) {

			// check the clocks only if the driver is programmable
			if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0) {
				if (!crtc_clock_check(&int_monitor, crtc)) {
					target_err("The selected mode '%s' is out of your monitor capabilities.\n", DEFAULT_GRAPH_MODE);
					return false;
				}
			}

			if (video_mode_generate(&int_current_mode, crtc, index)!=0) {
				target_err("The selected mode '%s' is out of your video board capabilities.\n", DEFAULT_GRAPH_MODE);
				return false;
			}

			mode_found = true;
			return true;
		}
	}

	// generate an exact mode with clock
	if (int_has_generate) {
		adv_crtc crtc;
		err = generate_find_interpolate(&crtc, int_mode_sizex, int_mode_sizey, 70, &int_monitor, &int_interpolate, video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0), GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK);
		if (err == 0) {
			if (crtc_clock_check(&int_monitor, &crtc)) {
				adv_mode mode;
				mode_reset(&mode);
				if (video_mode_generate(&mode, &crtc, index)==0) {
					int_current_mode = mode;
					mode_found = true;
					log_std(("text: generating a perfect mode from the format option.\n"));
					return true;
				}
			}
		}
	}

	// generate any resolution for a window manager or with an overlay
	if (video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)!=0
		|| video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY)!=0
	) {
		adv_crtc crtc;
		crtc_fake_set(&crtc, int_mode_sizex, int_mode_sizey);

		adv_mode mode;
		mode_reset(&mode);
		if (video_mode_generate(&mode, &crtc, index)==0) {
			int_current_mode = mode;
			mode_found = true;
			log_std(("text: generating a perfect mode for the window manager.\n"));
			return true;
		}
	}

	// search the best on the list
	for(crtc_container_iterator_begin(&i, &modelines);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		const adv_crtc* crtc = crtc_container_iterator_get(&i);

		// check the clocks only if the driver is programmable
		if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0) {
			if (!crtc_clock_check(&int_monitor, crtc)) {
				continue;
			}
		}

		adv_mode mode;
		mode_reset(&mode);
		if (video_mode_generate(&mode, crtc, index)==0) {
			if (!mode_found || int_mode_graphics_less(&mode, &int_current_mode)) {
				int_current_mode = mode;
				mode_found = true;
			}
		}
	}

	return true;
}

// -------------------------------------------------------------------------
// Visual Interface

static bool int_updating_active; ///< If updating at the video is possible, or we are in a drawing stage.

static double int_gamma; ///< Video gamma.
static double int_brightness; ///< Video brightness.

static unsigned int_idle_0; ///< Seconds before the first 0 event.
static unsigned int_idle_0_rep; ///< Seconds before the second 0 event.
static unsigned int_idle_1; ///< Seconds before the first 1 event.
static unsigned int_idle_1_rep; ///< Seconds before the second 1 event.
static time_t int_idle_time_current; ///< Last time check in idle.
static bool int_idle_0_state; ///< Idle event 0 enabler.
static bool int_idle_1_state; ///< Idle event 1 enabler.
static int int_last; ///< Last event.

static bool int_wait_for_backdrop; ///< Wait for the backdrop draw completion before accepting events.

static unsigned video_buffer_size; ///< Video buffer size in bytes.
static unsigned video_buffer_line_size; ///< Bideo buffer scanline size in bytes.
static unsigned video_buffer_pixel_size; ///< Video buffer pixel size in bytes.
static void* video_foreground_alloc; ///< Video foreground_buffer in memory.
static void* video_background_alloc; ///< Video background buffer in memory.
static unsigned char* video_foreground_buffer; ///< Video foreground_buffer in memory.
static unsigned char* video_background_buffer; ///< Video background buffer in memory.
static adv_bitmap* video_foreground_bitmap; ///< Video buffer bitmap in memory.
static adv_bitmap* video_background_bitmap; ///< Video buffer bitmap in memory.
adv_bool video_alpha_flag; ///< Color translucency enabled.
adv_color_def video_alpha_color_def; ///< Color definition for the alpha buffers.
unsigned video_alpha_bytes_per_pixel; ///< Pixel size of the alpha buffers.

void int_reg(adv_conf* config_context)
{
	int_mouse_reg(config_context);
	int_joystick_reg(config_context);
	int_key_reg(config_context);
	generate_interpolate_register(config_context);
	monitor_register(config_context);

	video_reg(config_context, 1);
	video_reg_driver_all(config_context);

	crtc_container_register(config_context);

	crtc_container_init(&int_modelines);
}

bool int_load(adv_conf* config_context)
{
	adv_error err;

	if (!int_joystick_load(config_context))
		return false;
	if (!int_mouse_load(config_context))
		return false;
	if (!int_key_load(config_context))
		return false;

	err = generate_interpolate_load(config_context, &int_interpolate);
	if (err<0) {
		target_err("%s\n", error_get());
		return false;
	}
	if (err==0) {
		int_has_generate = true;
	} else {
		int_has_generate = false;
		log_std(("text: format option not found.\n"));
	}

	err = monitor_load(config_context, &int_monitor);
	if (err<0) {
		target_err("%s\n", error_get());
		return false;
	}
	if (err==0) {
		int_has_clock = true;
	} else {
		int_has_clock = false;
		monitor_parse(&int_monitor, "10 - 150 / 30.5 - 60 / 55 - 90");
		log_std(("text: clock options not found. Use default SVGA monitor clocks.\n"));
	}

	err = video_load(config_context, "");
	if (err != 0) {
		target_err("%s\n", error_get());
		return false;
	}

	err = crtc_container_load(config_context, &int_modelines);
	if (err!=0) {
		target_err("%s\n", error_get());
		return false;
	}

	return true;
}

void int_unreg(void)
{
	crtc_container_done(&int_modelines);
	int_mouse_unreg();
	int_joystick_unreg();
	int_key_unreg();
}

bool int_init(unsigned sizex, unsigned sizey)
{
	unsigned index;
	bool mode_found = false;
	bool crtc_default = false;
	unsigned driver_flags;

	int_mode_sizex = sizex;
	int_mode_sizey = sizey;
	mode_reset(&int_current_mode);

	log_std(("text: init request size %ux%u\n", sizex, sizey));

	if (adv_video_init() != 0) {
		target_err("%s\n", error_get());
		goto out;
	}

	if (video_blit_init() != 0) {
		adv_video_done();
		target_err("%s\n", error_get());
		goto int_video;
	}

	/* The SDL overlay doesn't allow the video mode restore when starting the emulators */
	driver_flags = video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0);
	if ((driver_flags & VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY)!=0
		&& (driver_flags & VIDEO_DRIVER_FLAGS_INTERNAL_STATIC)!=0
	) {
		target_err("'overlay' output mode not supported in this configuration. Use 'fullscreen' instead.\n");
		goto int_blit;
	}

	// disable generate if the clocks are not available
	if (!int_has_clock)
		int_has_generate = false;

	// disable generate if the driver is not programmable
	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)==0)
		int_has_generate = false;

	// add modes if the list is empty
	if (crtc_container_is_empty(&int_modelines)) {
		int is_prog = (video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) !=0;
		if (!is_prog) {
			/* if it is NOT programmable */
			crtc_container_insert_default_active(&int_modelines);
			crtc_default = true;
		}
		if (is_prog && int_has_clock && !int_has_generate) {
			/* if it's programmable, with VCLOCK to check modeline, but no generation */
			crtc_container_insert_default_active(&int_modelines);
			crtc_default = true;
		}
	}

	// set auto resolution
	if (int_mode_sizex == 0 || int_mode_sizey == 0) {
		int_mode_sizex = target_size_x();
		int_mode_sizey = target_size_y();
	}
	// otherwise ask fo a generic big one to select the biggest available mode
	if (int_mode_sizex == 0 || int_mode_sizey == 0) {
#ifdef __MSDOS__
		int_mode_sizex = 640;
		int_mode_sizey = 480;
#else
		int_mode_sizex = 1920;
		int_mode_sizey = 1080;
#endif
	}

	// check if the video driver has a default bit depth
	switch (video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_DEFAULT_MASK) {
	case VIDEO_DRIVER_FLAGS_DEFAULT_BGR8 : index = MODE_FLAGS_INDEX_BGR8; break;
	case VIDEO_DRIVER_FLAGS_DEFAULT_BGR15 : index = MODE_FLAGS_INDEX_BGR15; break;
	case VIDEO_DRIVER_FLAGS_DEFAULT_BGR16 : index = MODE_FLAGS_INDEX_BGR16; break;
	case VIDEO_DRIVER_FLAGS_DEFAULT_BGR32 : index = MODE_FLAGS_INDEX_BGR32; break;
	default:
		index = 0;
		break;
	}

	if (index) {
		if (!int_mode_find(mode_found, index, int_modelines))
			goto int_blit;
	}

	// if no mode found retry with a different bit depth
	if (!mode_found) {
		unsigned select[] = { MODE_FLAGS_INDEX_BGR16, MODE_FLAGS_INDEX_BGR15, MODE_FLAGS_INDEX_BGR32, MODE_FLAGS_INDEX_BGR8, 0 };
		unsigned* i;

		i = select;
		while (*i && !mode_found) {
			if (!int_mode_find(mode_found, *i, int_modelines))
				goto int_blit;
			++i;
		}
	}

	if (!mode_found) {
		if (!int_has_generate && !crtc_default)
			target_err("No video mode available for your configuration.\nTry removing any explicit modeline in the configuration file.\n");
		else
			target_err("No video mode available for your configuration.\nTry selecting a specific resolution like with -display_size 1280x1024\n");
		goto int_blit;
	}

	return true;

int_blit:
	video_blit_done();
int_video:
	adv_video_done();
out:
	return false;
}

void int_done()
{
	video_blit_done();
	adv_video_done();
}

bool int_set(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep, unsigned idle_1, unsigned idle_1_rep, bool backdrop_fast, unsigned translucency, bool disable_special)
{
	int_idle_time_current = time(0);
	int_idle_0 = idle_0;
	int_idle_1 = idle_1;
	int_idle_0_rep = idle_0_rep;
	int_idle_1_rep = idle_1_rep;
	int_idle_0_state = true;
	int_idle_1_state = true;
	int_wait_for_backdrop = !backdrop_fast;
	if (gamma < 0.1) gamma = 0.1;
	if (gamma > 10) gamma = 10;
	int_gamma = 1.0 / gamma;
	int_brightness = brightness;

	if (!int_key_init(disable_special)) {
		target_err("%s\n", error_get());
		goto err;
	}

	if (!int_joystick_init()) {
		target_err("%s\n", error_get());
		goto err_key;
	}

	if (!int_mouse_init()) {
		target_err("%s\n", error_get());
		goto err_joy;
	}

	if (video_mode_set(&int_current_mode) != 0) {
		video_mode_restore();
		target_err("%s\n", error_get());
		goto err_mouse;
	}

	video_alpha_flag = translucency != 255;
	video_alpha_color_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0); /* BGRA */
	video_alpha_bytes_per_pixel = color_def_bytes_per_pixel_get(video_alpha_color_def);

	color_setup(video_color_def(), video_alpha_color_def, translucency);

	if (!int_key_enable()) {
		video_mode_restore();
		target_err("%s\n", error_get());
		goto err_mouse;
	}

	return true;
err_mouse:
	int_mouse_done();
err_joy:
	int_joystick_done();
err_key:
	int_key_done();
err:
	return false;
}

void int_unplug()
{
    joystickb_disable();
    mouseb_disable();
	keyb_disable();
	mouseb_done();
	joystickb_done();
}

void int_plug()
{
	if (joystickb_init() != 0)
		joystickb_init_null();

	if (mouseb_init() != 0)
		mouseb_init_null();

	if (keyb_enable(1) != 0) {
		keyb_done();
		keyb_init_null();
		keyb_enable(1);
	}

	if (mouseb_enable() != 0) {
		mouseb_done();
		mouseb_init_null();
		mouseb_enable();
	}

	if (joystickb_enable() != 0) {
		joystickb_done();
		joystickb_init_null();
		joystickb_enable();
	}
}

void int_unset(bool reset_video_mode)
{
	int_key_disable();

	if (reset_video_mode) {
		if ((video_driver_flags() & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)==0) {
			video_write_lock();
			video_clear(0, 0, video_size_x(), video_size_y(), 0);
			video_write_unlock(0, 0, video_size_x(), video_size_y(), 0);
			video_display_set(0, 0);
		}
		video_mode_restore();
	} else {
		video_mode_done(0);
	}

	int_mouse_done();
	int_joystick_done();
	int_key_done();
}

bool int_enable(int fontx, int fonty, const string& font, unsigned orientation)
{
	int_orientation = orientation;
	unsigned font_size_x;
	unsigned font_size_y;

	if (fonty >= 5 && fonty <= 100)
		font_size_y = video_size_y() / fonty;
	else
		font_size_y = video_size_y() / 45;
	if (fontx >= 5 && fontx <= 200)
		font_size_x = video_size_x() / fontx;
	else
		font_size_x = font_size_y * video_size_x() * 3 / video_size_y() / 4;

	// load the font
	int_font = 0;
	if (font != "none" && font != "auto") {
		adv_fz* f = fzopen(font.c_str(), "rb");
		if (f) {
			int_font = adv_font_load(f, font_size_x, font_size_y);
			fzclose(f);
		}
	}
	if (!int_font)
		int_font = adv_font_default(font_size_x, font_size_y, 0);

	// set the orientation
	adv_font_orientation(int_font, int_orientation);

	// compute font size
	int_font_dx = adv_font_sizex(int_font);
	int_font_dy = adv_font_sizey(int_font);

	video_buffer_pixel_size = video_bytes_per_pixel();
	video_buffer_line_size = ALIGN_UNSIGNED(video_size_x() * video_bytes_per_pixel(), ALIGN);
	video_buffer_size = video_size_y() * video_buffer_line_size;
	video_foreground_alloc = operator new(video_buffer_size + ALIGN);
	video_background_alloc = operator new(video_buffer_size + ALIGN);
	video_foreground_buffer = (unsigned char*)ALIGN_PTR(video_foreground_alloc, ALIGN);
	video_background_buffer = (unsigned char*)ALIGN_PTR(video_background_alloc, ALIGN);
	video_foreground_bitmap = adv_bitmap_import_rgb(video_size_x(), video_size_y(), video_buffer_pixel_size, 0, 0, video_foreground_buffer, video_buffer_line_size);
	video_background_bitmap = adv_bitmap_import_rgb(video_size_x(), video_size_y(), video_buffer_pixel_size, 0, 0, video_background_buffer, video_buffer_line_size);

	memset(video_background_buffer, 0, video_buffer_size);
	memset(video_foreground_buffer, 0, video_buffer_size);

	int_updating_active = false;

	return true;
}

void int_disable()
{
	adv_font_free(int_font);
	operator delete(video_foreground_alloc);
	adv_bitmap_free(video_foreground_bitmap);
	operator delete(video_background_alloc);
	adv_bitmap_free(video_background_bitmap);
}

/**
 * Save the current video buffer and return a pointer at the copy.
 */
void* int_save()
{
	void* buffer = operator new(video_buffer_size);

	memcpy(buffer, video_foreground_buffer, video_buffer_size);

	return buffer;
}

/**
 * Restore a previously saved video buffer.
 */
void int_restore(void* buffer)
{
	memcpy(video_foreground_buffer, buffer, video_buffer_size);

	operator delete(buffer);
}

static int fast_exit_handler(void)
{
	if (int_wait_for_backdrop)
		return 0;

	// update
	int_event_waiting();

	int key = event_peek();

	return key == EVENT_PGUP
		|| key == EVENT_PGDN
		|| key == EVENT_INS
		|| key == EVENT_DEL
		|| key == EVENT_HOME
		|| key == EVENT_END
		|| key == EVENT_UP
		|| key == EVENT_DOWN
		|| key == EVENT_LEFT
		|| key == EVENT_RIGHT
		|| key == EVENT_MODE;
}

// -------------------------------------------------------------------------
// Cell Position

class cell_pos_t {
public:
	// Position of the cell in the screen
	int x;
	int y;
	int dx;
	int dy;

	// Position of the cell in the screen, already rotated
	int real_x;
	int real_y;
	int real_dx;
	int real_dy;

	void compute_size(unsigned* rx, unsigned* ry, const adv_bitmap* bitmap, unsigned aspectx, unsigned aspecty, double aspect_expand);
	void draw_backdrop(const adv_bitmap* map, const adv_color_rgb& background);
	void draw_clip(const adv_bitmap* map, adv_color_rgb* rgb_map, unsigned rgb_max, unsigned aspectx, unsigned aspecty, double aspect_expand, const adv_color_rgb& background, bool clear);
	void clear(const adv_color_rgb& background);
	void redraw();
	void border(int width, const adv_color_rgb& color);
};

void cell_pos_t::redraw()
{
	video_write_lock();

	video_stretch_direct(real_x, real_y, real_dx, real_dy, video_foreground_buffer + real_y * video_buffer_line_size + real_x * video_bytes_per_pixel(), real_dx, real_dy, video_buffer_line_size, video_bytes_per_pixel(), video_color_def(), 0);

	video_write_unlock(real_x, real_y, real_dx, real_dy, 0);
}

void cell_pos_t::compute_size(unsigned* rx, unsigned* ry, const adv_bitmap* bitmap, unsigned aspectx, unsigned aspecty, double aspect_expand)
{
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		unsigned t = aspectx;
		aspectx = aspecty;
		aspecty = t;
	}

	if (!aspectx || !aspecty) {
		aspectx = bitmap->size_x;
		aspecty = bitmap->size_y;
	}

	if (!aspectx || !aspecty) {
		aspectx = 1;
		aspecty = 1;
	}

	aspectx *= 3 * video_size_x();
	aspecty *= 4 * video_size_y();

	if (aspectx * real_dy > aspecty * real_dx) {
		*rx = real_dx;
		*ry = static_cast<unsigned>(real_dx * aspecty * aspect_expand / aspectx);
	} else {
		*rx = static_cast<unsigned>(real_dy * aspectx * aspect_expand / aspecty);
		*ry = real_dy;
	}
	if (*rx > real_dx)
		*rx = real_dx;
	if (*ry > real_dy)
		*ry = real_dy;
}

static void gen_clear_raw(int x, int y, int dx, int dy, const adv_color_rgb& color)
{
	adv_pixel pixel = video_pixel_get(color.red, color.green, color.blue);

	adv_bitmap_clear(video_foreground_bitmap, x, y, dx, dy, pixel);
}

static void gen_clear_alpha(int x, int y, int dx, int dy, const adv_color_rgb& color)
{
	if (video_alpha_flag)
		adv_bitmap_clear_alphaback(video_foreground_bitmap, x, y, video_color_def(), video_background_bitmap, x, y, color, dx, dy);
	else
		gen_clear_raw(x, y, dx, dy, color);
}

void cell_pos_t::draw_backdrop(const adv_bitmap* map, const adv_color_rgb& background)
{
	unsigned x0 = (real_dx - map->size_x) / 2;
	unsigned x1 = real_dx -  map->size_x - x0;
	unsigned y0 = (real_dy - map->size_y) / 2;
	unsigned y1 = real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_alpha(real_x, real_y, x0, real_dy, background);
	if (x1)
		gen_clear_alpha(real_x + real_dx - x1, real_y, x1, real_dy, background);
	if (y0)
		gen_clear_alpha(real_x, real_y, real_dx, y0, background);
	if (y1)
		gen_clear_alpha(real_x, real_y + real_dy - y1, real_dx, y1, background);

	adv_bitmap_put(video_foreground_bitmap, real_x + x0, real_y + y0, map, 0, 0, map->size_x, map->size_y);
}

void cell_pos_t::clear(const adv_color_rgb& background)
{
	gen_clear_alpha(real_x, real_y, real_dx, real_dy, background);
}

void cell_pos_t::draw_clip(const adv_bitmap* bitmap, adv_color_rgb* rgb_map, unsigned rgb_max, unsigned aspectx, unsigned aspecty, double aspect_expand, const adv_color_rgb& background, bool clear)
{
	adv_pixel pixel = video_pixel_get(background.red, background.green, background.blue);

	// source range and steps
	unsigned char* ptr = bitmap->ptr;
	int dw = bitmap->bytes_per_scanline;
	int dp = bitmap->bytes_per_pixel;
	int dx = bitmap->size_x;
	int dy = bitmap->size_y;

	// set the correct orientation
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		int t;
		t = dp;
		dp = dw;
		dw = t;
		t = dx;
		dx = dy;
		dy = t;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_X) {
		ptr = ptr + (dx-1) * dp;
		dp = - dp;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_Y) {
		ptr = ptr + (dy-1) * dw;
		dw = - dw;
	}

	// destination range
	unsigned dst_dx;
	unsigned dst_dy;
	unsigned dst_x;
	unsigned dst_y;
	dst_x = real_x;
	dst_y = real_y;
	dst_dx = real_dx;
	dst_dy = real_dy;

	// compute the size of the bitmap
	unsigned rel_dx;
	unsigned rel_dy;
	compute_size(&rel_dx, &rel_dy, bitmap, aspectx, aspecty, aspect_expand);

	// adjust the destination range if too big
	if (dst_dx > rel_dx) {
		unsigned pre_dx = (dst_dx - rel_dx) / 2;
		unsigned post_dx = (dst_dx - rel_dx + 1) / 2;

		if (clear) {
			gen_clear_alpha(dst_x, dst_y, pre_dx, dst_dy, background);
			gen_clear_alpha(dst_x + pre_dx + rel_dx, dst_y, post_dx, dst_dy, background);
		}

		dst_x += pre_dx;
		dst_dx = rel_dx;
	}
	if (dst_dy > rel_dy) {
		unsigned pre_dy = (dst_dy - rel_dy) / 2;
		unsigned post_dy = (dst_dy - rel_dy + 1) / 2;

		if (clear) {
			gen_clear_alpha(dst_x, dst_y, dst_dx, pre_dy, background);
			gen_clear_alpha(dst_x, dst_y + pre_dy + rel_dy, dst_dx, post_dy, background);
		}

		dst_y += pre_dy;
		dst_dy = rel_dy;
	}

	unsigned combine = VIDEO_COMBINE_Y_NONE;
	if (dst_dx < dx)
		combine |= VIDEO_COMBINE_X_MEAN;
	if (dst_dy < dy)
		combine |= VIDEO_COMBINE_Y_MEAN;

	struct video_pipeline_struct pipeline;

	video_pipeline_init(&pipeline);

	video_pipeline_target(&pipeline, video_foreground_buffer, video_buffer_line_size, video_color_def());

	uint32 palette32[256];
	uint16 palette16[256];
	uint8 palette8[256];
	if (bitmap->bytes_per_pixel == 1) {
		for(unsigned i=0;i<rgb_max;++i) {
			adv_pixel p = video_pixel_get(rgb_map[i].red, rgb_map[i].green, rgb_map[i].blue);
			palette32[i] = p;
			palette16[i] = p;
			palette8[i] = p;
		}
		video_pipeline_palette8(&pipeline, dst_dx, dst_dy, dx, dy, dw, dp, palette8, palette16, palette32, combine);
	} else {
		adv_color_def def = adv_png_color_def(bitmap->bytes_per_pixel);
		video_pipeline_direct(&pipeline, dst_dx, dst_dy, dx, dy, dw, dp, def, combine);
	}

	video_pipeline_blit(&pipeline, dst_x, dst_y, ptr);

	video_pipeline_done(&pipeline);
}

void cell_pos_t::border(int width, const adv_color_rgb& color)
{
	int x = real_x - width;
	int y = real_y - width;
	int dx = real_dx + width * 2;
	int dy = real_dy + width * 2;

	gen_clear_raw(x, y, dx, width, color);
	gen_clear_raw(x, y+dy-width, dx, width, color);
	gen_clear_raw(x, y+width, width, dy-2*width, color);
	gen_clear_raw(x+dx-width, y+width, width, dy-2*width, color);
}

// -------------------------------------------------------------------------
// Backdrop

// Backdrop (already orientation corrected)
class backdrop_data {
	adv_bitmap* map;
	resource res;
	unsigned target_dx;
	unsigned target_dy;
	unsigned aspectx;
	unsigned aspecty;

	void icon_apply(adv_bitmap* bitmap, adv_bitmap* bitmap_mask, adv_color_rgb* rgb, unsigned* rgb_max, const adv_color_rgb& background);
	adv_bitmap* image_load(const resource& res, adv_color_rgb* rgb, unsigned* rgb_max, const adv_color_rgb& background);
	adv_bitmap* adapt(adv_bitmap* bitmap, adv_color_rgb* rgb, unsigned* rgb_max, unsigned dst_dx, unsigned dst_dy);

public:
	backdrop_data(const resource& Ares, unsigned Atarget_dx, unsigned Atarget_dy, unsigned Aaspectx, unsigned Aaspecty);
	~backdrop_data();

	bool is_active() const { return map != 0; }
	const resource& res_get() const { return res; }
	const adv_bitmap* bitmap_get() const { return map; }

	unsigned target_dx_get() const { return target_dx; }
	unsigned target_dy_get() const { return target_dy; }
	unsigned aspectx_get() const { return aspectx; }
	unsigned aspecty_get() const { return aspecty; }

	void load(struct cell_pos_t* cell, const adv_color_rgb& background, double aspect_expand);
};

backdrop_data::backdrop_data(const resource& Ares, unsigned Atarget_dx, unsigned Atarget_dy, unsigned Aaspectx, unsigned Aaspecty)
	: res(Ares), target_dx(Atarget_dx), target_dy(Atarget_dy), aspectx(Aaspectx), aspecty(Aaspecty) {
	map = 0;
}

backdrop_data::~backdrop_data()
{
	if (map)
		adv_bitmap_free(map);
}

void backdrop_data::icon_apply(adv_bitmap* bitmap, adv_bitmap* bitmap_mask, adv_color_rgb* rgb, unsigned* rgb_max, const adv_color_rgb& background)
{
	unsigned index;
	if (*rgb_max == 256) {
		unsigned count[256];
		for(unsigned i=0;i<256;++i)
			count[i] = 0;

		for(unsigned y=0;y<bitmap->size_y;++y) {
			uint8* line = adv_bitmap_line(bitmap, y);
			for(unsigned x=0;x<bitmap->size_x;++x)
				++count[line[x]];
		}

		index = 0;
		for(unsigned i=0;i<256;++i)
			if (count[i] < count[index])
				index = i;

		if (count[index] != 0) {
			unsigned substitute = 0;
			for(unsigned y=0;y<bitmap->size_y;++y) {
				uint8* line = adv_bitmap_line(bitmap, y);
				for(unsigned x=0;x<bitmap->size_x;++x)
					if (line[x] == index)
						line[x] = substitute;
			}
		}
	} else {
		index = *rgb_max;
		++*rgb_max;
	}

	rgb[index].red = background.red;
	rgb[index].green = background.green;
	rgb[index].blue = background.blue;
	rgb[index].alpha = 0;

	for(unsigned y=0;y<bitmap->size_y;++y) {
		uint8* line = adv_bitmap_line(bitmap, y);
		uint8* line_mask = adv_bitmap_line(bitmap_mask, y);
		for(unsigned x=0;x<bitmap->size_x;++x) {
			if (!line_mask[x])
				line[x] = index;
		}
	}
}

adv_bitmap* backdrop_data::image_load(const resource& res, adv_color_rgb* rgb, unsigned* rgb_max, const adv_color_rgb& background)
{
	string ext = file_ext(res.path_get());

	if (ext == ".png") {
		adv_fz* f = res.open();
		if (!f)
			return 0;

		adv_bitmap* bitmap = adv_bitmap_load_png(rgb, rgb_max, f);
		if (!bitmap) {
			fzclose(f);
			return 0;
		}

		fzclose(f);
		return bitmap;
	}

	if (ext == ".pcx") {
		adv_fz* f = res.open();
		if (!f)
			return 0;

		adv_bitmap* bitmap = adv_bitmap_load_pcx(rgb, rgb_max, f);

		fzclose(f);
		return bitmap;
	}

	if (ext == ".ico") {
		adv_fz* f = res.open();
		if (!f)
			return 0;

		adv_bitmap* bitmap_mask;
		adv_bitmap* bitmap = adv_bitmap_load_icon(rgb, rgb_max, &bitmap_mask, f);
		if (!bitmap) {
			fzclose(f);
			return 0;
		}

		icon_apply(bitmap, bitmap_mask, rgb, rgb_max, background);

		adv_bitmap_free(bitmap_mask);
		fzclose(f);
		return bitmap;
	}

	if (ext == ".mng") {
		adv_mng* mng;

		adv_fz* f = res.open();
		if (!f)
			return 0;

		mng = adv_mng_init(f);
		if (mng == 0) {
			fzclose(f);
			return 0;
		}

		unsigned pix_width;
		unsigned pix_height;
		unsigned pix_pixel;
		unsigned char* dat_ptr;
		unsigned dat_size;
		unsigned char* pix_ptr;
		unsigned pix_scanline;
		unsigned char* pal_ptr;
		unsigned pal_size;
		unsigned tick;

		int r = adv_mng_read_done(mng, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, f);
		if (r != 0) {
			fzclose(f);
			return 0;
		}

		adv_bitmap* bitmap = adv_bitmap_import_palette(rgb, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
		if (!bitmap) {
			free(dat_ptr);
			free(pal_ptr);
			fzclose(f);
			return 0;
		}

		free(pal_ptr);
		fzclose(f);

		return bitmap;
	}

	return 0;
}

adv_bitmap* backdrop_data::adapt(adv_bitmap* bitmap, adv_color_rgb* rgb, unsigned* rgb_max, unsigned dst_dx, unsigned dst_dy)
{
	// source range and steps
	unsigned char* ptr = bitmap->ptr;
	int dw = bitmap->bytes_per_scanline;
	int dp = bitmap->bytes_per_pixel;
	int dx = bitmap->size_x;
	int dy = bitmap->size_y;

	// set the correct orientation
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		int t;
		t = dp;
		dp = dw;
		dw = t;
		t = dx;
		dx = dy;
		dy = t;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_X) {
		ptr = ptr + (dx-1) * dp;
		dp = - dp;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_Y) {
		ptr = ptr + (dy-1) * dw;
		dw = - dw;
	}

	unsigned combine = VIDEO_COMBINE_Y_NONE;
	if (dst_dx < dx)
		combine |= VIDEO_COMBINE_X_MEAN;
	if (dst_dy < dy)
		combine |= VIDEO_COMBINE_Y_MEAN;

	adv_bitmap* raw_bitmap = adv_bitmap_alloc(dst_dx, dst_dy, video_bytes_per_pixel());

	struct video_pipeline_struct pipeline;

	video_pipeline_init(&pipeline);

	video_pipeline_target(&pipeline, raw_bitmap->ptr, raw_bitmap->bytes_per_scanline, video_color_def());

	uint32 palette32[256];
	uint16 palette16[256];
	uint8 palette8[256];
	if (bitmap->bytes_per_pixel == 1) {
		for(unsigned i=0;i<*rgb_max;++i) {
			adv_pixel p = video_pixel_get(rgb[i].red, rgb[i].green, rgb[i].blue);
			palette32[i] = p;
			palette16[i] = p;
			palette8[i] = p;
		}
		video_pipeline_palette8(&pipeline, dst_dx, dst_dy, dx, dy, dw, dp, palette8, palette16, palette32, combine);
	} else {
		adv_color_def def = adv_png_color_def(bitmap->bytes_per_pixel);
		video_pipeline_direct(&pipeline, dst_dx, dst_dy, dx, dy, dw, dp, def, combine);
	}

	video_pipeline_blit(&pipeline, 0, 0, ptr);

	video_pipeline_done(&pipeline);

	return raw_bitmap;
}

void backdrop_data::load(struct cell_pos_t* cell, const adv_color_rgb& background, double aspect_expand)
{
	if (map)
		return; // already loaded

	adv_color_rgb rgb[256];
	unsigned rgb_max;

	adv_bitmap* bitmap = image_load(res_get(), rgb, &rgb_max, background);
	if (!bitmap)
		return;

	// compute the size of the bitmap
	unsigned dst_dx;
	unsigned dst_dy;

	cell->compute_size(&dst_dx, &dst_dy, bitmap, aspectx, aspecty, aspect_expand);

	adv_bitmap* scaled_bitmap = adapt(bitmap, rgb, &rgb_max, dst_dx, dst_dy);

	adv_bitmap_free(bitmap);

	if (!scaled_bitmap)
		return;

	map = scaled_bitmap;
}

// -------------------------------------------------------------------------
// Backdrop Cache

class backdrop_cache {
	unsigned max;
	list<backdrop_data*> bag;
public:
	backdrop_cache(unsigned Amax);
	~backdrop_cache();

	void reduce();
	void free(backdrop_data* data);
	backdrop_data* alloc(const resource& res, unsigned dx, unsigned dy, unsigned aspectx, unsigned aspecty);
};

backdrop_cache::backdrop_cache(unsigned Amax)
{
	max = Amax;
}

backdrop_cache::~backdrop_cache()
{
	for(list<backdrop_data*>::iterator i=bag.begin();i!=bag.end();++i)
		delete *i;
}

// Reduce the size of the cache
void backdrop_cache::reduce()
{
	// limit the cache size
	while (bag.size() > max) {
		list<backdrop_data*>::iterator i = bag.end();
		--i;
		backdrop_data* data = *i;
		bag.erase(i);
		delete data;
	}
}

// Delete or insert in the cache the backdrop image
void backdrop_cache::free(backdrop_data* data)
{
	if (data) {
		if (data->is_active()) {
			// insert the image in the cache
			bag.insert(bag.begin(), data);
		} else {
			delete data;
		}
	}
}

backdrop_data* backdrop_cache::alloc(const resource& res, unsigned dx, unsigned dy, unsigned aspectx, unsigned aspecty)
{
	// search in the cache
	for(list<backdrop_data*>::iterator i=bag.begin();i!=bag.end();++i) {
		if ((*i)->res_get() == res
			&& dx == (*i)->target_dx_get()
			&& dy == (*i)->target_dy_get()) {

			// extract from the cache
			backdrop_data* data = *i;

			// remove from the cache
			bag.erase(i);

			return data;
		}
	}

	return new backdrop_data(res, dx, dy, aspectx, aspecty);
}

// -------------------------------------------------------------------------
// Clip

class clip_data {
	bool active;
	bool running;
	bool waiting;
	resource res;
	adv_fz* f;
	target_clock_t wait;
	unsigned count;
	adv_mng* mng_context;

	clip_data();
	clip_data(const clip_data&);

public:
	clip_data(const resource& Ares);
	~clip_data();

	void start();
	void rewind();

	adv_bitmap* load(adv_color_rgb* rgb_map, unsigned* rgb_max);
	bool is_waiting();
	bool is_old();
	bool is_first();
	bool is_active();
	const resource& res_get() const { return res; }
};

clip_data::clip_data(const resource& Ares)
{
	f = 0;
	active = true;
	running = false;
	waiting = true;
	res = Ares;
}

clip_data::~clip_data()
{
	if (f) {
		fzclose(f);
		adv_mng_done(mng_context);
	}
}

void clip_data::rewind()
{
	if (f) {
		fzclose(f);
		adv_mng_done(mng_context);
		f = 0;
	}

	active = true;
	running = false;
	waiting = true;
}

void clip_data::start()
{
	waiting = false;

	if (!active)
		return;
	if (running)
		return;

	running = true;
	wait = target_clock(); // reset the start time
}

bool clip_data::is_first()
{
	return count == 1;
}

bool clip_data::is_waiting()
{
	return waiting;
}

bool clip_data::is_old()
{
	if (!active || !running)
		return false;

	if (!f || target_clock() > wait)
		return true;

	return false;
}

bool clip_data::is_active()
{
	return active && running;
}

adv_bitmap* clip_data::load(adv_color_rgb* rgb_map, unsigned* rgb_max)
{
	if (!active)
		return 0;

	if (!f) {
		// first load
		f = res.open();
		if (!f) {
			active = false;
			f = 0;
			return 0;
		}

		mng_context = adv_mng_init(f);
		if (mng_context == 0) {
			fzclose(f);
			f = 0;
			active = false;
			return 0;
		}

		wait = target_clock();
		count = 0;
	}

	unsigned pix_width;
	unsigned pix_height;
	unsigned pix_pixel;
	unsigned char* dat_ptr;
	unsigned dat_size;
	unsigned char* pix_ptr;
	unsigned pix_scanline;
	unsigned char* pal_ptr;
	unsigned pal_size;
	unsigned tick;

	int r = adv_mng_read(mng_context, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, f);
	if (r != 0) {
		adv_mng_done(mng_context);
		fzclose(f);
		f = 0;
		running = false;
		return 0;
	}

	double delay = tick / (double)adv_mng_frequency_get(mng_context);

	adv_bitmap* bitmap = adv_bitmap_import_palette(rgb_map, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
	if (!bitmap) {
		free(dat_ptr);
		free(pal_ptr);
		adv_mng_done(mng_context);
		fzclose(f);
		f = 0;
		active = false;
		return 0;
	}

	free(pal_ptr);

	wait += (target_clock_t)(delay * TARGET_CLOCKS_PER_SEC);

	// limit the late time to 1/10 second
	if (target_clock() - wait > TARGET_CLOCKS_PER_SEC / 10)
		wait = target_clock() - TARGET_CLOCKS_PER_SEC / 10;

	++count;

	return bitmap;
}

// -------------------------------------------------------------------------
// Clip Cache

class clip_cache {
	unsigned max;
	list<clip_data*> bag;
public:
	clip_cache(unsigned Amax);
	~clip_cache();

	void reduce();
	void free(clip_data* data);
	clip_data* alloc(const resource& res);
};

clip_cache::clip_cache(unsigned Amax)
{
	max = Amax;
}

clip_cache::~clip_cache()
{
	for(list<clip_data*>::iterator i=bag.begin();i!=bag.end();++i) {
		clip_data* data = *i;
		delete data;
	}
}

// Reduce the size of the cache
void clip_cache::reduce()
{
	// limit the cache size
	while (bag.size() > 0) {
		list<clip_data*>::iterator i = bag.end();
		--i;
		clip_data* data = *i;
		bag.erase(i);
		delete data;
	}
}

// Delete or insert in the cache the backdrop image
void clip_cache::free(clip_data* data)
{
	if (data) {
		if (max)
			bag.insert(bag.begin(), data);
		else
			delete data;
	}
}

clip_data* clip_cache::alloc(const resource& res)
{
	// search in the cache
	for(list<clip_data*>::iterator i=bag.begin();i!=bag.end();++i) {
		if ((*i)->res_get() == res) {

			// extract from the cache
			clip_data* data = *i;

			// remove from the cache
			bag.erase(i);

			return data;
		}
	}

	return new clip_data(res);
}

//---------------------------------------------------------------------------
// Cell Manager

struct cell_t {
	cell_pos_t pos; ///< Position on the screen.
	backdrop_data* data; ///< Backdrop (already orientation corrected and resized).
	clip_data* cdata; ///< Clip.
	unsigned caspectx; ///< Clip aspect.
	unsigned caspecty;
	resource last; ///< Previous backdrop resource.
	bool highlight; ///< The backdrop has the highlight.
	bool redraw; ///< The backdrop need to be redrawn.
};

#define CELL_MAX 512

class cell_manager {
	class backdrop_cache* int_backdrop_cache;
	class clip_cache* int_clip_cache;

	unsigned backdrop_mac;

	struct cell_t backdrop_map[CELL_MAX];

	// Color used for missing backdrop
	int_color backdrop_missing_color;

	// Color used for the lighting box backdrop
	int_color backdrop_box_color;

	unsigned backdrop_outline; // size of the backdrop outline
	unsigned backdrop_cursor; // size of the backdrop cursor

	double backdrop_expand_factor; // stretch factor

	bool multiclip;

	target_clock_t backdrop_box_last;

	bool idle_update(int index);

	unsigned idle_iterator;

public:
	cell_manager(const int_color& Abackdrop_missing_color, const int_color& Abackdrop_box_color, unsigned Amac, unsigned Ainc, unsigned outline, unsigned cursor, double expand_factor, bool Amulticlip);
	~cell_manager();

	unsigned size() const { return backdrop_mac; }

	void pos_set(int index, int x, int y, int dx, int dy);

	void backdrop_set(int index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty);
	void backdrop_clear(int index, bool highlight);
	void backdrop_update(int index);
	unsigned backdrop_topline(int index);
	void backdrop_box();
	void backdrop_redraw_all();

	void clip_set(int index, const resource& res, unsigned aspectx, unsigned aspecty, bool restart);
	void clip_clear(int index);
	void clip_start(int index);
	bool clip_is_active(int index);

	void reduce();
	bool idle();
};

unsigned cell_manager::backdrop_topline(int index)
{
	return backdrop_map[index].pos.real_y - backdrop_outline;
}

cell_manager::cell_manager(const int_color& Abackdrop_missing_color, const int_color& Abackdrop_box_color, unsigned Amac, unsigned Ainc, unsigned outline, unsigned cursor, double expand_factor, bool Amulticlip)
{
	backdrop_box_last = 0;
	backdrop_missing_color = Abackdrop_missing_color;
	backdrop_box_color = Abackdrop_box_color;
	backdrop_outline = outline;
	backdrop_cursor = cursor;
	backdrop_expand_factor = expand_factor;
	backdrop_mac = Amac;

	int_backdrop_cache = new backdrop_cache(backdrop_mac*2 + Ainc + 1);

	multiclip = Amulticlip;
	if (multiclip)
		int_clip_cache = new clip_cache(backdrop_mac);
	else
		int_clip_cache = new clip_cache(0);

	for(int i=0;i<backdrop_mac;++i) {
		backdrop_map[i].data = 0;
		backdrop_map[i].cdata = 0;
		backdrop_map[i].last = resource();
		backdrop_map[i].highlight = false;
		backdrop_map[i].redraw = true;
	}

	idle_iterator = 0;
}

cell_manager::~cell_manager()
{
	for(int i=0;i<backdrop_mac;++i) {
		if (backdrop_map[i].data)
			delete backdrop_map[i].data;
		backdrop_map[i].data = 0;
		if (backdrop_map[i].cdata)
			delete backdrop_map[i].cdata;
		backdrop_map[i].cdata = 0;
	}

	delete int_backdrop_cache;
	int_backdrop_cache = 0;

	delete int_clip_cache;
	int_clip_cache = 0;
}

void cell_manager::backdrop_redraw_all()
{
	for(int i=0;i<backdrop_mac;++i) {
		backdrop_map[i].redraw = true;
	}
}

// Set the backdrop position and size
void cell_manager::pos_set(int index, int x, int y, int dx, int dy)
{
	assert(index >= 0 && index < backdrop_mac);

	int_backdrop_cache->free(backdrop_map[index].data);
	backdrop_map[index].data = 0;

	int_clip_cache->free(backdrop_map[index].cdata);
	backdrop_map[index].cdata = 0;

	backdrop_map[index].last = resource();
	backdrop_map[index].highlight = false;
	backdrop_map[index].redraw = true;

	backdrop_map[index].pos.x = backdrop_map[index].pos.real_x = x + backdrop_outline;
	backdrop_map[index].pos.y = backdrop_map[index].pos.real_y = y + backdrop_outline;
	backdrop_map[index].pos.dx = backdrop_map[index].pos.real_dx = dx - 2*backdrop_outline;
	backdrop_map[index].pos.dy = backdrop_map[index].pos.real_dy = dy - 2*backdrop_outline;

	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		swap(backdrop_map[index].pos.real_x, backdrop_map[index].pos.real_y);
		swap(backdrop_map[index].pos.real_dx, backdrop_map[index].pos.real_dy);
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_X) {
		backdrop_map[index].pos.real_x = video_size_x() - backdrop_map[index].pos.real_x - backdrop_map[index].pos.real_dx;
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_Y) {
		backdrop_map[index].pos.real_y = video_size_y() - backdrop_map[index].pos.real_y - backdrop_map[index].pos.real_dy;
	}
}

// Select the backdrop
void cell_manager::backdrop_set(int index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty)
{
	assert(index >= 0 && index < backdrop_mac);

	int_backdrop_cache->free(backdrop_map[index].data);

	backdrop_map[index].data = int_backdrop_cache->alloc(res, backdrop_map[index].pos.dx, backdrop_map[index].pos.dy, aspectx, aspecty);

	if (backdrop_map[index].last == res) {
		if (backdrop_map[index].highlight != highlight) {
			backdrop_map[index].highlight = highlight;
			backdrop_map[index].redraw = true;
		}
	} else {
		backdrop_map[index].last = res;
		backdrop_map[index].highlight = highlight;
		backdrop_map[index].redraw = true;
	}
}

// Clear the backdrop
void cell_manager::backdrop_clear(int index, bool highlight)
{
	assert(index >= 0 && index < backdrop_mac);

	int_backdrop_cache->free(backdrop_map[index].data);

	backdrop_map[index].data = 0;

	if (!backdrop_map[index].last.is_valid()) {
		if (backdrop_map[index].highlight != highlight) {
			backdrop_map[index].highlight = highlight;
			backdrop_map[index].redraw = true;
		}
	} else {
		backdrop_map[index].last = resource();
		backdrop_map[index].highlight = highlight;
		backdrop_map[index].redraw = true;
	}
}

static void box(int x, int y, int dx, int dy, int width, const adv_color_rgb& color)
{
	adv_pixel pixel = video_pixel_get(color.red, color.green, color.blue);
	video_clear(x, y, dx, width, pixel);
	video_clear(x, y+dy-width, dx, width, pixel);
	video_clear(x, y+width, width, dy-2*width, pixel);
	video_clear(x+dx-width, y+width, width, dy-2*width, pixel);
}

void cell_manager::backdrop_box()
{
	if (!backdrop_cursor)
		return;

	for(int i=0;i<backdrop_mac;++i) {
		struct cell_t* back = backdrop_map + i;
		if (back->highlight) {
			unsigned x = back->pos.real_x - backdrop_outline;
			unsigned y = back->pos.real_y - backdrop_outline;
			unsigned dx = back->pos.real_dx + 2*backdrop_outline;
			unsigned dy = back->pos.real_dy + 2*backdrop_outline;

			video_write_lock();
			box(x, y, dx, dy, backdrop_cursor, backdrop_box_color.foreground);
			video_write_unlock(x, y, dx, dy, 0);

			adv_color_rgb c = backdrop_box_color.foreground;
			backdrop_box_color.foreground = backdrop_box_color.background;
			backdrop_box_color.background = c;
		}
	}
}

// Update the backdrop image
void cell_manager::backdrop_update(int index)
{
	struct cell_t* back = backdrop_map + index;

	assert(index >= 0 && index < backdrop_mac);

	if (back->data) {
		if (!fast_exit_handler())
			back->data->load(&back->pos, backdrop_missing_color.background, backdrop_expand_factor);
	}

	if (back->redraw) {
		if (back->data) {
			if (back->data->bitmap_get()) {
				back->redraw = false;
				back->pos.draw_backdrop(back->data->bitmap_get(), backdrop_missing_color.background);
			} else {
				// the image need to be update when loaded
				back->pos.clear(backdrop_missing_color.background);
			}
		} else {
			back->redraw = false;
			back->pos.clear(backdrop_missing_color.background);
		}

		if (backdrop_outline)
			back->pos.border(backdrop_outline, backdrop_missing_color.foreground);
	}
}

void cell_manager::reduce()
{
	if (int_backdrop_cache)
		int_backdrop_cache->reduce();
	if (int_clip_cache)
		int_clip_cache->reduce();
}

bool cell_manager::idle_update(int index)
{
	adv_color_rgb rgb_map[256];
	unsigned rgb_max;

	cell_t* cell = backdrop_map + index;

	clip_data* clip = cell->cdata;
	if (!clip)
		return false;

	if (!clip->is_old())
		return false;

	adv_bitmap* bitmap = clip->load(rgb_map, &rgb_max);

	if (!bitmap) {
		cell->redraw = true; // force the redraw

		backdrop_update(index);

		cell->pos.redraw();

		return false;
	}

	cell->pos.draw_clip(bitmap, rgb_map, rgb_max, cell->caspectx, cell->caspecty, backdrop_expand_factor, backdrop_missing_color.background, clip->is_first());

	cell->pos.redraw();

	adv_bitmap_free(bitmap);

	// ensure to fill the audio buffer
	play_poll();

	return true;
}

bool cell_manager::idle()
{
	if (multiclip) {
		int highlight_index = -1;

		target_clock_t start = target_clock();
		target_clock_t stop = start + TARGET_CLOCKS_PER_SEC / 100; // limit update for 10 ms

		// search the highlight clip
		for(unsigned i=0;i<backdrop_mac;++i) {
			if (backdrop_map[i].highlight) {
				highlight_index = i;
			}
		}

		// always display the highlight clip
		if (highlight_index >= 0) {
			idle_update(highlight_index);
		}

		// update all the clip
		for(unsigned i=0;i<backdrop_mac;++i) {
			target_clock_t backdrop_box_new = target_clock();
			if (backdrop_box_new >= backdrop_box_last + TARGET_CLOCKS_PER_SEC/20) {
				backdrop_box_last = backdrop_box_new;
				backdrop_box();
			}

			// increment the iterator
			++idle_iterator;
			if (idle_iterator >= backdrop_mac)
				idle_iterator = 0;

			if (idle_iterator != highlight_index) {
				idle_update(idle_iterator);

				// don't wait too much
				if (target_clock() > stop)
					break;
			}
		}
	} else {
		for(unsigned i=0;i<backdrop_mac;++i) {
			idle_update(i);
		}

		target_clock_t backdrop_box_new = target_clock();
		if (backdrop_box_new >= backdrop_box_last + TARGET_CLOCKS_PER_SEC/20) {
			backdrop_box_last = backdrop_box_new;
			backdrop_box();
		}
	}

	// render the screen
	video_display_set(0, 0);

	// recheck if some clip is already old
	for(unsigned i=0;i<backdrop_mac;++i) {
		cell_t* cell = backdrop_map + i;
		clip_data* clip = cell->cdata;
		if (clip && clip->is_old())
			return false;
	}

	return true;
}

void cell_manager::clip_set(int index, const resource& res, unsigned aspectx, unsigned aspecty, bool restart)
{
	assert(index >= 0 && index < backdrop_mac);

	int_clip_cache->free(backdrop_map[index].cdata);

	backdrop_map[index].cdata = int_clip_cache->alloc(res);
	backdrop_map[index].caspectx = aspectx;
	backdrop_map[index].caspecty = aspecty;

	if (backdrop_map[index].cdata) {
		if (restart)
			backdrop_map[index].cdata->rewind();
	}
}

void cell_manager::clip_clear(int index)
{
	assert(index >= 0 && index < backdrop_mac);

	int_clip_cache->free(backdrop_map[index].cdata);

	backdrop_map[index].cdata = 0;
}

void cell_manager::clip_start(int index)
{
	assert(index >= 0);

	for(unsigned i=0;i<backdrop_mac;++i) {
		if (backdrop_map[i].cdata
			&& (i == index || backdrop_map[i].cdata->is_waiting())) {
			backdrop_map[i].cdata->start();
		}
	}
}

bool cell_manager::clip_is_active(int index)
{
	assert(index >= 0);

	if (index >= backdrop_mac) {
		return false;
	}

	if (backdrop_map[index].cdata) {
		return backdrop_map[index].cdata->is_active();
	} else {
		return false;
	}
}

static class cell_manager* int_cell; // global cell manager

//---------------------------------------------------------------------------
// Backgrop/Clip Interface

void int_backdrop_init(const int_color& back_color, const int_color& back_box_color, unsigned Amac, unsigned Ainc, unsigned Aoutline, unsigned Acursor, double expand_factor, bool multiclip)
{
	int_cell = new cell_manager(back_color, back_box_color, Amac, Ainc, Aoutline, Acursor, expand_factor, multiclip);
}

void int_backdrop_done()
{
	delete int_cell;
	int_cell = 0;
}

void int_backdrop_pos(int index, int x, int y, int dx, int dy)
{
	int_cell->pos_set(index, x, y, dx, dy);
}

void int_backdrop_set(int index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty)
{
	int_cell->backdrop_set(index, res, highlight, aspectx, aspecty);
}

void int_backdrop_redraw_all()
{
	if (int_cell)
		int_cell->backdrop_redraw_all();
}

void int_backdrop_clear(int index, bool highlight)
{
	int_cell->backdrop_clear(index, highlight);
}

void int_clip_set(int index, const resource& res, unsigned aspectx, unsigned aspecty, bool restart)
{
	int_cell->clip_set(index, res, aspectx, aspecty, restart);
}

void int_clip_clear(int index)
{
	int_cell->clip_clear(index);
}

void int_clip_start(int index)
{
	if (int_cell)
		int_cell->clip_start(index);
}

bool int_clip_is_active(int index)
{
	if (int_cell)
		return int_cell->clip_is_active(index);
	else
		return true;
}

//---------------------------------------------------------------------------
// Put Interface

unsigned int_put_width(char c)
{
	adv_bitmap* src = int_font->data[(unsigned char)c];
	if (int_orientation & ADV_ORIENTATION_FLIP_XY)
		return src->size_y;
	else
		return src->size_x;
}

unsigned int_put_width(const string& s)
{
	unsigned size = 0;
	for(unsigned i=0;i<s.length();++i)
		size += int_put_width(s[i]);
	return size;
}

void int_put(int x, int y, char c, const int_color& color)
{
	if (x>=0 && y>=0 && x+int_put_width(c)<=int_dx_get() && y+int_font_dy_get()<=int_dy_get()) {
		adv_bitmap* src = int_font->data[(unsigned char)c];

		if (int_orientation & ADV_ORIENTATION_FLIP_XY)
			swap(x, y);
		if (int_orientation & ADV_ORIENTATION_FLIP_X)
			x = video_size_x() - src->size_x - x;
		if (int_orientation & ADV_ORIENTATION_FLIP_Y)
			y = video_size_y() - src->size_y - y;

		assert(x>=0 && y>=0 && x+src->size_x<=video_size_x() && y+src->size_y<=video_size_y());

		adv_font_put_char_map(int_font, video_foreground_bitmap, x, y, c, color.opaque);
	}
}

void int_put_alpha(int x, int y, char c, const int_color& color)
{
	if (x>=0 && y>=0 && x+int_put_width(c)<=int_dx_get() && y+int_font_dy_get()<=int_dy_get()) {
		adv_bitmap* src = int_font->data[(unsigned char)c];

		if (int_orientation & ADV_ORIENTATION_FLIP_XY)
			swap(x, y);
		if (int_orientation & ADV_ORIENTATION_FLIP_X)
			x = video_size_x() - src->size_x - x;
		if (int_orientation & ADV_ORIENTATION_FLIP_Y)
			y = video_size_y() - src->size_y - y;

		assert(x>=0 && y>=0 && x+src->size_x<=video_size_x() && y+src->size_y<=video_size_y());

		if (video_alpha_flag) {
			adv_bitmap* flat = adv_bitmap_alloc(src->size_x, src->size_y, video_alpha_bytes_per_pixel);

			adv_font_put_char_map(int_font, flat, 0, 0, c, color.alpha);

			adv_bitmap_put_alphaback(video_foreground_bitmap, x, y, video_color_def(), video_background_bitmap, x, y, flat, 0, 0, flat->size_x, flat->size_y, video_alpha_color_def);

			adv_bitmap_free(flat);
		} else {
			adv_font_put_char_map(int_font, video_foreground_bitmap, x, y, c, color.opaque);
		}
	}
}

void int_put_filled(int x, int y, int dx, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		if (int_put_width(s[i]) <= dx) {
			int_put(x, y, s[i], color);
			x += int_put_width(s[i]);
			dx -= int_put_width(s[i]);
		} else
			break;
	}
	if (dx)
		int_clear(x, y, dx, int_font_dy_get(), color.background);
}

void int_put_filled_alpha(int x, int y, int dx, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		if (int_put_width(s[i]) <= dx) {
			int_put_alpha(x, y, s[i], color);
			x += int_put_width(s[i]);
			dx -= int_put_width(s[i]);
		} else
			break;
	}
	if (dx)
		int_clear_alpha(x, y, dx, int_font_dy_get(), color.background);
}

void int_put_special(bool& in, int x, int y, int dx, const string& s, const int_color& c0, const int_color& c1, const int_color& c2)
{
	for(unsigned i=0;i<s.length();++i) {
		if (int_put_width(s[i]) <= dx) {
			if (s[i]=='(' || s[i]=='[')
				in = true;
			if (!in && isupper(s[i])) {
				int_put(x, y, s[i], c0);
			} else {
				int_put(x, y, s[i], in ? c1 : c2);
			}
			x += int_put_width(s[i]);
			dx -= int_put_width(s[i]);
			if (s[i]==')' || s[i]==']')
				in = false;
		} else
			break;
	}
	if (dx)
		int_clear(x, y, dx, int_font_dy_get(), c0.background);
}

void int_put_special_alpha(bool& in, int x, int y, int dx, const string& s, const int_color& c0, const int_color& c1, const int_color& c2)
{
	for(unsigned i=0;i<s.length();++i) {
		if (int_put_width(s[i]) <= dx) {
			if (s[i]=='(' || s[i]=='[')
				in = true;
			if (!in && isupper(s[i])) {
				int_put_alpha(x, y, s[i], c0);
			} else {
				int_put_alpha(x, y, s[i], in ? c1 : c2);
			}
			x += int_put_width(s[i]);
			dx -= int_put_width(s[i]);
			if (s[i]==')' || s[i]==']')
				in = false;
		} else
			break;
	}

	if (dx)
		int_clear_alpha(x, y, dx, int_font_dy_get(), c0.background);
}

void int_put(int x, int y, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		int_put(x, y, s[i], color);
		x += int_put_width(s[i]);
	}
}

void int_put_alpha(int x, int y, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		int_put_alpha(x, y, s[i], color);
		x += int_put_width(s[i]);
	}
}

unsigned int_put(int x, int y, int dx, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		int width = int_put_width(s[i]);
		if (width > dx)
			return i;
		int_put(x, y, s[i], color);
		x += width;
		dx -= width;
	}
	return s.length();
}

unsigned int_put_alpha(int x, int y, int dx, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		int width = int_put_width(s[i]);
		if (width > dx)
			return i;
		int_put(x, y, s[i], color);
		x += width;
		dx -= width;
	}
	return s.length();
}

unsigned int_put_right(int x, int y, int dx, const string& s, const int_color& color)
{
	unsigned size = int_put_width(s);
	return int_put(x + dx - size, y, dx, s, color);
}

unsigned int_put_right_alpha(int x, int y, int dx, const string& s, const int_color& color)
{
	unsigned size = int_put_width(s);
	return int_put_alpha(x + dx - size, y, dx, s, color);
}

void int_clear(const adv_color_rgb& color) 
{
	adv_pixel background = video_pixel_get(color.red, color.green, color.blue);
	adv_pixel overscan;

	// clear the whole bitmap using the overscan color
	if ((video_driver_flags() & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)!=0
		&& video_buffer_pixel_size > 1) {
		// fill with white in a window manager environment
		overscan = video_pixel_get(0xff, 0xff, 0xff);
	} else {
		// fill with black in a full screen environment
		overscan = video_pixel_get(0, 0, 0);
	}

	adv_bitmap_clear(video_foreground_bitmap, 0, 0, video_size_x(), video_size_y(), overscan);
	adv_bitmap_clear(video_background_bitmap, 0, 0, video_size_x(), video_size_y(), background);
}

void int_box(int x, int y, int dx, int dy, int width, const adv_color_rgb& color)
{
	int_clear(x, y, dx, width, color);
	int_clear(x, y+dy-width, dx, width, color);
	int_clear(x, y+width, width, dy-2*width, color);
	int_clear(x+dx-width, y+width, width, dy-2*width, color);
}

void int_clear(int x, int y, int dx, int dy, const adv_color_rgb& color)
{
	// clip
	if (x < 0) {
		dx += x;
		x = 0;
	}
	if (y < 0) {
		dy += y;
		y = 0;
	}
	if (x + dx > int_dx_get()) {
		dx = int_dx_get() - x;
	}
	if (y + dy > int_dy_get()) {
		dy = int_dy_get() - y;
	}
	if (dx <= 0 || dy <= 0)
		return;

	// rotate
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_X)
		x = video_size_x() - dx - x;
	if (int_orientation & ADV_ORIENTATION_FLIP_Y)
		y = video_size_y() - dy - y;

	gen_clear_raw(x, y, dx, dy, color);
}

void int_clear_alpha(int x, int y, int dx, int dy, const adv_color_rgb& color)
{
	// clip
	if (x < 0) {
		dx += x;
		x = 0;
	}
	if (y < 0) {
		dy += y;
		y = 0;
	}
	if (x + dx > int_dx_get()) {
		dx = int_dx_get() - x;
	}
	if (y + dy > int_dy_get()) {
		dy = int_dy_get() - y;
	}
	if (dx <= 0 || dy <= 0)
		return;

	// rotate
	if (int_orientation & ADV_ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}
	if (int_orientation & ADV_ORIENTATION_FLIP_X)
		x = video_size_x() - dx - x;
	if (int_orientation & ADV_ORIENTATION_FLIP_Y)
		y = video_size_y() - dy - y;

	gen_clear_alpha(x, y, dx, dy, color);
}

bool int_clip(const string& file, bool loop)
{
	unsigned aspectx = int_dx_get();
	unsigned aspecty = int_dy_get();
	resource res = path_import(file);

	bool wait = true;

	int_backdrop_init(COLOR_MENU_BACKDROP, COLOR_MENU_BACKDROP, 1, 0, 0, 0, 1.0, false);

	int_backdrop_pos(0, 0, 0, int_dx_get(), int_dy_get());

	if (file.find(".mng") != string::npos) {
		int_clip_set(0, res, aspectx, aspecty, false);

		int_clip_start(0);

		int_update();

		while (!int_event_waiting() && int_clip_is_active(0)) {
		}

		int_clip_clear(0);

		if (loop)
			wait = false;
	} else {
		int_backdrop_set(0, res, false, aspectx, aspecty);

		int_update();
	}

	while (int_event_waiting()) {
		wait = false;
		int_event_get(false);
	}

	int_backdrop_done();

	return wait;
}

bool int_image(const string& file, unsigned& scale_x, unsigned& scale_y)
{
	adv_fz* f;
	f = fzopen(file.c_str(), "rb");
	if (!f) {
		log_std(("ERROR:text: error opening file %s\n", file.c_str()));
		return false;
	}

	adv_color_rgb rgb_map[256];
	unsigned rgb_max;
	adv_bitmap* bitmap;

	if (file.find(".mng") != string::npos) {
		adv_mng* mng;

		mng = adv_mng_init(f);
		if (mng == 0) {
			fzclose(f);
			return false;
		}

		unsigned pix_width;
		unsigned pix_height;
		unsigned pix_pixel;
		unsigned char* dat_ptr;
		unsigned dat_size;
		unsigned char* pix_ptr;
		unsigned pix_scanline;
		unsigned char* pal_ptr;
		unsigned pal_size;
		unsigned tick;

		int r = adv_mng_read_done(mng, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, f);
		if (r != 0) {
			fzclose(f);
			return false;
		}

		bitmap = adv_bitmap_import_palette(rgb_map, &rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
		if (!bitmap) {
			free(dat_ptr);
			free(pal_ptr);
			fzclose(f);
			return false;
		}

		free(pal_ptr);
	} else {
		bitmap = adv_bitmap_load_png(rgb_map, &rgb_max, f);
	}
	if (!bitmap) {
		log_std(("ERROR:text: error reading file %s\n", file.c_str()));
		fzclose(f);
		return false;
	}

	fzclose(f);

	scale_x = bitmap->size_x;
	scale_y = bitmap->size_y;

	struct video_pipeline_struct pipeline;
	unsigned combine = VIDEO_COMBINE_X_MEAN | VIDEO_COMBINE_Y_MEAN;

	video_pipeline_init(&pipeline);

	video_pipeline_target(&pipeline, video_background_buffer, video_buffer_line_size, video_color_def());

	uint32 palette32[256];
	uint16 palette16[256];
	uint8 palette8[256];
	if (bitmap->bytes_per_pixel == 1) {
		for(unsigned i=0;i<rgb_max;++i) {
			adv_pixel p = video_pixel_get(rgb_map[i].red, rgb_map[i].green, rgb_map[i].blue);
			palette32[i] = p;
			palette16[i] = p;
			palette8[i] = p;
		}
		video_pipeline_palette8(&pipeline, video_size_x(), video_size_y(), bitmap->size_x, bitmap->size_y, bitmap->bytes_per_scanline, bitmap->bytes_per_pixel, palette8, palette16, palette32, combine);
	} else {
		adv_color_def def = adv_png_color_def(bitmap->bytes_per_pixel);
		video_pipeline_direct(&pipeline, video_size_x(), video_size_y(), bitmap->size_x, bitmap->size_y, bitmap->bytes_per_scanline, bitmap->bytes_per_pixel, def, combine);
	}

	video_pipeline_blit(&pipeline, 0, 0, bitmap->ptr);

	video_pipeline_done(&pipeline);

	adv_bitmap_free(bitmap);

	// copy also into the foreground
	memcpy(video_foreground_buffer, video_background_buffer, video_buffer_size);

	// invalidate all the backdrop if any
	int_backdrop_redraw_all();

	return true;
}

//---------------------------------------------------------------------------
// Update Interface

static void int_copy_partial(unsigned y0, unsigned y1)
{
	video_write_lock();

	unsigned char* buffer = video_foreground_buffer + y0 * video_buffer_line_size;
	for(unsigned y=y0;y<y1;++y) {
		memcpy(video_write_line(y), buffer, video_buffer_line_size);
		buffer += video_buffer_line_size;
	}

	video_write_unlock(0, y0, video_size_x(), y1 - y0, 0);
}

unsigned int_update_pre(bool progressive)
{
	int_updating_active = false;

	play_poll();

	int y = 0;

	if (int_cell) {
		for(int i=0;i<int_cell->size();++i) {
			play_poll();

			// this trick works only if the backdrops positioned are from top to down
			if (int_orientation == 0 && progressive) {
				// update progressively the screen
				int yl = int_cell->backdrop_topline(i);
				if (yl > y) {
					int_copy_partial(y, yl);
					y = yl;
				}
			}

			int_cell->backdrop_update(i);
		}

		int_cell->reduce();
	}

	return y;
}

void int_update_post(unsigned y)
{
	play_poll();

	int_copy_partial(y, video_size_y());

	if (int_cell) {
		int_cell->backdrop_box();
	}

	// render the screen
	video_display_set(0, 0);

	play_poll();

	int_updating_active = true;
}

void int_update(bool progressive)
{
	int_update_post(int_update_pre(progressive));
}

static void key_poll()
{
	if (os_is_quit()) {
		event_push(EVENT_ESC);
	}

	int_joystick_button_raw_poll();
	int_joystick_move_raw_poll();
	int_mouse_button_raw_poll();
	int_mouse_move_raw_poll();
	event_poll();
}

void int_idle_time_reset()
{
	int_idle_time_current = time(0);
	int_last = EVENT_NONE;
}

void int_idle_0_enable(bool state)
{
	int_idle_0_state = state;
}

void int_idle_1_enable(bool state)
{
	int_idle_1_state = state;
}

static void int_idle()
{
	time_t now = time(0);
	time_t elapsed = now - int_idle_time_current;

	if (int_idle_0_state) {
		if (int_idle_0_rep != 0
			&& int_last == EVENT_IDLE_0
			&& elapsed > int_idle_0_rep
		) {
			log_std(("text: push IDLE_0 repeat\n"));
			event_push_repeat(EVENT_IDLE_0);
		} else if (int_idle_0 != 0
			&& elapsed > int_idle_0
		) {
			log_std(("text: push IDLE_0\n"));
			event_push_repeat(EVENT_IDLE_0);
		}
	}

	if (int_idle_1_state) {
		if (int_idle_1_rep != 0
			&& int_last == EVENT_IDLE_1
			&& elapsed > int_idle_1_rep
		) {
			log_std(("text: push IDLE_1 repeat\n"));
			event_push_repeat(EVENT_IDLE_1);
		} else if (int_idle_1 != 0
			&& elapsed > int_idle_1
		) {
			log_std(("text: push IDLE_1\n"));
			event_push_repeat(EVENT_IDLE_1);
		}
	}

	if (event_peek() == EVENT_NONE) {
		if (int_updating_active) {
			if (int_cell) {
				// idle only if there is time
				if (int_cell->idle()) {
					target_idle();
				} else {
					// we are late, allow to allocate 100% CPU
					target_yield();
				}
			} else {
				target_idle();
			}
		}
	}

	play_poll();
	keyb_poll();
	mouseb_poll();
	joystickb_poll();
}

bool int_event_waiting()
{
	static target_clock_t key_pressed_last_time = 0;

	// low level mute/unmute management
	while (event_peek() == EVENT_MUTE) {
		event_pop();
		play_mute_set(!play_mute_get());
	}

	if (event_peek() != EVENT_NONE)
		return 1;

	target_clock_t now = target_clock();

	// if you ask for keypress, assume a not CPU intensive state
	int_idle();

	// don't check too frequently
	if (now - key_pressed_last_time >= TARGET_CLOCKS_PER_SEC / 25) {
		key_pressed_last_time = now;

		key_poll();
	}

	// low level mute/unmute management
	while (event_peek() == EVENT_MUTE) {
		event_pop();
		play_mute_set(!play_mute_get());
	}

	if (event_peek() != EVENT_NONE)
		return 1;

	return 0;
}

unsigned int_event_get(bool update_background)
{
	if (update_background)
		int_update();

	// wait for a keypress, internally a idle call is already done
	while (!int_event_waiting()) {
	}

	int_idle_time_current = time(0);

#if 0 /* OSDEF: Save interface image, only for debugging. */
	if (event_peek() == EVENT_INS) {
		char name[256];
		static int ssn = 0;
		++ssn;

		snprintf(name, sizeof(name), "im%d.png", ssn);
		adv_fz* f = fzopen(name, "wb");
		if (f) {
			adv_png_write_def(video_size_x(), video_size_y(), video_color_def(), video_foreground_buffer, video_buffer_pixel_size, video_buffer_line_size, 0, 0, 0, f, 0);
			fzclose(f);
		}
	}
#endif

	return int_last = event_pop();
}


