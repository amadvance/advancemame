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
 */

#include "advance.h"

#include "text.h"
#include "common.h"
#include "play.h"

#include <list>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <unistd.h>
#include <math.h>
#include <stdio.h>

using namespace std;

static string int_sound_event_key;

static bool int_alpha_mode;

// Orientation flag
static unsigned int_orientation = 0;

// Font (already orientation corrected)
static unsigned real_font_dx;
static unsigned real_font_dy;
static adv_font* real_font_map;

// -------------------------------------------------------------------------
// Size

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
	if (int_orientation & ORIENTATION_MIRROR_X) {
		x = video_size_x() - x - dx;
	}
	if (int_orientation & ORIENTATION_MIRROR_Y) {
		y = video_size_y() - y - dy;
	}
	if (int_orientation & ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}
}

void int_rotate(int& x, int& y, int& dx, int& dy)
{
	if (int_orientation & ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}
	if (int_orientation & ORIENTATION_MIRROR_X) {
		x = video_size_x() - x - dx;
	}
	if (int_orientation & ORIENTATION_MIRROR_Y) {
		y = video_size_y() - y - dy;
	}
}

int int_dx_get()
{
	if (int_orientation & ORIENTATION_FLIP_XY)
		return video_size_y();
	else
		return video_size_x();
}

int int_dy_get()
{
	if (int_orientation & ORIENTATION_FLIP_XY)
		return video_size_x();
	else
		return video_size_y();
}

int int_font_dx_get()
{
	if (int_orientation & ORIENTATION_FLIP_XY)
		return real_font_dy;
	else
		return real_font_dx;
}

int int_font_dy_get()
{
	if (int_orientation & ORIENTATION_FLIP_XY)
		return real_font_dx;
	else
		return real_font_dy;
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
public:
	backdrop_data(const resource& Ares, unsigned Atarget_dx, unsigned Atarget_dy, unsigned Aaspectx, unsigned Aaspecty);
	~backdrop_data();

	bool has_bitmap() const { return map != 0; }
	const resource& res_get() const { return res; }
	const adv_bitmap* bitmap_get() const { return map; }
	void bitmap_set(adv_bitmap* Amap) { assert(!map); map = Amap; }

	unsigned target_dx_get() const { return target_dx; }
	unsigned target_dy_get() const { return target_dy; }
	unsigned aspectx_get() const { return aspectx; }
	unsigned aspecty_get() const { return aspecty; }
};

backdrop_data::backdrop_data(const resource& Ares, unsigned Atarget_dx, unsigned Atarget_dy, unsigned Aaspectx, unsigned Aaspecty)
	: res(Ares), target_dx(Atarget_dx), target_dy(Atarget_dy), aspectx(Aaspectx), aspecty(Aaspecty) {
	map = 0;
}

backdrop_data::~backdrop_data()
{
	if (map)
		bitmap_free(map);
}

struct backdrop_pos_t {
	// Position of the backdrop in the screen
	int x;
	int y;
	int dx;
	int dy;

	// Position of the backdrop in the screen, already rotated
	int real_x;
	int real_y;
	int real_dx;
	int real_dy;

	bool highlight;

	bool redraw;
} pos;

struct backdrop_t {
	// Backdrop (already orientation corrected)
	backdrop_data* data;
	resource last;
	backdrop_pos_t pos;
};

// Color used for missing backdrop
static int_color backdrop_missing_color;

// Color used for the lighting box backdrop
static int_color backdrop_box_color;

#define BACKDROP_MAX 512
static unsigned backdrop_mac;

static struct backdrop_t backdrop_map[BACKDROP_MAX];

#define BACKDROP_CACHE_MAX (BACKDROP_MAX*2+1)
typedef list<backdrop_data*> pbackdrop_list;
static pbackdrop_list* backdrop_cache_list;
static unsigned backdrop_cache_max;

static double backdrop_expand_factor; // stretch factor

static unsigned backdrop_outline; // size of the backdrop outline
static unsigned backdrop_cursor; // size of the backdrop cursor

static string int_cfg_file;

static bool int_updating_active; // is updating the video

// -----------------------------------------------------------------------
// Joystick

static void int_joystick_reg(adv_conf* config_context)
{
	joystickb_reg(config_context, 0);
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

static int int_joystick_button_raw_poll()
{
	for(int i=0;i<joystickb_count_get();++i) {
		for(int j=0;j<joystickb_button_count_get(i);++j) {
			if (joystickb_button_get(i, j)) {
				switch (j) {
					case 0 : return INT_KEY_ENTER;
					case 1 : return INT_KEY_ESC;
					case 2 : return INT_KEY_MENU;
					case 3 : return INT_KEY_SPACE;
					case 4 : return INT_KEY_MODE;
				}
			}
		}
	}

	return INT_KEY_NONE;
}

static int int_joystick_move_raw_poll()
{
	for(int i=0;i<joystickb_count_get();++i) {
		for(int j=0;j<joystickb_stick_count_get(i);++j) {
			if (joystickb_stick_axe_count_get(i, j) > 0) {
				if (joystickb_stick_axe_digital_get(i, j, 0, 0))
					return INT_KEY_RIGHT;
				if (joystickb_stick_axe_digital_get(i, j, 0, 1))
					return INT_KEY_LEFT;
			}
			if (joystickb_stick_axe_count_get(i, j) > 1) {
				if (joystickb_stick_axe_digital_get(i, j, 1, 0))
					return INT_KEY_DOWN;
				if (joystickb_stick_axe_digital_get(i, j, 1, 1))
					return INT_KEY_UP;
			}
		}
	}

	return INT_KEY_NONE;
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

static bool int_key_init()
{
	if (keyb_init(0) != 0)
		return false;

	return true;
}

static void int_key_done()
{
	keyb_done();
}

static bool int_key_enable()
{
	if (keyb_enable(1) != 0)
		return false;

	return true;
}

static void int_key_disable()
{
	keyb_disable();
}

// -----------------------------------------------------------------------
// Mouse

static int int_mouse_delta;
static int int_mouse_pos_x;
static int int_mouse_pos_y;

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

static int int_mouse_button_raw_poll()
{
	for(int i=0;i<mouseb_count_get();++i) {
		if (mouseb_button_count_get(i) > 0 && mouseb_button_get(i, 0))
			return INT_KEY_ENTER;

		if (mouseb_button_count_get(i) > 1 && mouseb_button_get(i, 1))
			return INT_KEY_ESC;

		if (mouseb_button_count_get(i) > 2 && mouseb_button_get(i, 2))
			return INT_KEY_MENU;
	}

	return INT_KEY_NONE;
}

static int int_mouse_move_raw_poll()
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
		return INT_KEY_RIGHT;
	}

	if (int_mouse_pos_x <= -int_mouse_delta) {
		int_mouse_pos_x += int_mouse_delta;
		return INT_KEY_LEFT;
	}

	if (int_mouse_pos_y >= int_mouse_delta) {
		int_mouse_pos_y -= int_mouse_delta;
		return INT_KEY_DOWN;
	}

	if (int_mouse_pos_y <= -int_mouse_delta) {
		int_mouse_pos_y += int_mouse_delta;
		return INT_KEY_UP;
	}

	return INT_KEY_NONE;
}

// -------------------------------------------------------------------------
// Video mode choice

#define DEFAULT_GRAPH_MODE "default_graph" // default video mode

static unsigned int_mode_size;
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

	int difA = abs( areaA - static_cast<int>(int_mode_size*int_mode_size*3/4) );
	int difB = abs( areaB - static_cast<int>(int_mode_size*int_mode_size*3/4) );

	return difA < difB;
}

static bool int_mode_find(bool& mode_found, unsigned index, adv_crtc_container& modelines)
{
	adv_crtc_container_iterator i;
	adv_error err;

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
		err = generate_find_interpolate(&crtc, int_mode_size, int_mode_size*3/4, 70, &int_monitor, &int_interpolate, video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0), GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK);
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

	// generate any resolution for a window manager
	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW))!=0) {
		adv_crtc crtc;
		crtc_fake_set(&crtc, int_mode_size, int_mode_size*3/4);

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
// text

static double int_gamma;
static double int_brightness;

static int int_key_saved = INT_KEY_NONE;
static int int_key_last = INT_KEY_NONE;

static unsigned int_idle_0; // seconds before the first 0 event
static unsigned int_idle_0_rep; // seconds before the second 0 event
static unsigned int_idle_1; // seconds before the first 1 event
static unsigned int_idle_1_rep; // seconds before the second 1 event
static unsigned int_repeat; // milli seconds before the first key repeat event
static unsigned int_repeat_rep; // milli seconds before the second key repeat event
static time_t int_idle_time;
static bool int_idle_0_state;
static bool int_idle_1_state;

static bool int_wait_for_backdrop; // wait for backdrop completion

static unsigned video_buffer_size;
static unsigned video_buffer_line_size;
static unsigned video_buffer_pixel_size;
static unsigned char* video_buffer;

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
		monitor_parse(&int_monitor, "10 - 80", "30.5 - 60", "55 - 90");
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

bool int_init(unsigned size, const string& sound_event_key)
{
	unsigned index;
	bool mode_found = false;

	int_mode_size = size;
	int_sound_event_key = sound_event_key;
	mode_reset(&int_current_mode);

	if (video_init() != 0) {
		target_err("%s\n", error_get());
		goto out;
	}

	if (video_blit_init() != 0) {
		video_done();
		target_err("%s\n", error_get());
		goto int_video;
	}

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_OUTPUT_ZOOM)!=0) {
		target_err("Zoom output mode not supported by this program.\n");
		goto int_blit;
	}

	// disable generate if the clocks are not available
	if (!int_has_clock)
		int_has_generate = false;

	// disable generate if the driver is not programmable
	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)==0)
		int_has_generate = false;

	// add modes if the list is empty and no generation is possibile
	if (!int_has_generate
		&& crtc_container_is_empty(&int_modelines)) {
		if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) != 0) {
			crtc_container_insert_default_modeline_svga(&int_modelines);
			crtc_container_insert_default_modeline_vga(&int_modelines);
		} else {
			crtc_container_insert_default_active(&int_modelines);
		}
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
		target_err("No video modes available for your current configuration.\n");
		goto int_blit;
	}

	return true;

int_blit:
	video_blit_done();
int_video:
	video_done();
out:
	return false;
}

void int_done()
{
	video_blit_done();
	video_done();
}

bool int_set(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep, unsigned idle_1, unsigned idle_1_rep, unsigned repeat, unsigned repeat_rep, bool backdrop_fast, bool alpha_mode)
{

	int_alpha_mode = alpha_mode;

	int_idle_time = time(0);
	int_idle_0 = idle_0;
	int_idle_1 = idle_1;
	int_idle_0_rep = idle_0_rep;
	int_idle_1_rep = idle_1_rep;
	int_repeat = repeat;
	int_repeat_rep = repeat_rep;
	int_idle_0_state = true;
	int_idle_1_state = true;
	int_wait_for_backdrop = !backdrop_fast;
	if (gamma < 0.1) gamma = 0.1;
	if (gamma > 10) gamma = 10;
	int_gamma = 1.0 / gamma;
	int_brightness = brightness;

	if (!int_key_init()) {
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

void int_unset(bool reset_video_mode)
{
	int_key_disable();

	if (reset_video_mode) {
		if ((video_driver_flags() & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)==0) {
			video_write_lock();
			video_clear(0, 0, video_size_x(), video_size_y(), 0);
			video_write_unlock(0, 0, video_size_x(), video_size_y());
		}
		video_mode_restore();
	} else {
		video_mode_done(0);
	}

	int_mouse_done();
	int_joystick_done();
	int_key_done();
}

bool int_enable(const string& font, unsigned orientation)
{
	int_orientation = orientation;

	// load the font
	real_font_map = 0;
	if (font != "none") {
		real_font_map = font_load(cpath_export(font));
	}
	if (!real_font_map)
		real_font_map = font_default(video_size_y() / 25);

	// set the orientation
	font_orientation(real_font_map, int_orientation);

	// compute font size
	real_font_dx = font_size_x(real_font_map);
	real_font_dy = font_size_y(real_font_map);

	video_buffer_pixel_size = video_bytes_per_pixel();
	video_buffer_line_size = video_size_x() * video_bytes_per_pixel();
	video_buffer_size = video_size_y() * video_buffer_line_size;
	video_buffer = (unsigned char*)operator new(video_buffer_size);

	int_clear();

	int_updating_active = false;

	return true;
}

void int_disable()
{
	font_free(real_font_map);
	operator delete(video_buffer);
}

//---------------------------------------------------------------------------
// Generic put

static void gen_clear_raw8rgb(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, adv_pixel pixel)
{
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned char* dst_ptr = buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = pixel;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw32rgb(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, adv_pixel pixel)
{
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned* dst_ptr = (unsigned*)buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = pixel;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw16rgb(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, adv_pixel pixel)
{
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned short* dst_ptr = (unsigned short*)buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = pixel;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, int x, int y, int dx, int dy, const int_rgb& color)
{
	assert( x>=0 && y>=0 && x+dx<=video_size_x() &&  y+dy<=video_size_y() );

	adv_pixel pixel = video_pixel_get(color.r, color.g, color.b);

	switch (video_index()) {
	case MODE_FLAGS_INDEX_BGR8 :
		gen_clear_raw8rgb(ptr, ptr_p, ptr_d, x, y, dx, dy, pixel);
		break;
	case MODE_FLAGS_INDEX_BGR15 :
	case MODE_FLAGS_INDEX_BGR16 :
		gen_clear_raw16rgb(ptr, ptr_p, ptr_d, x, y, dx, dy, pixel);
		break;
	case MODE_FLAGS_INDEX_BGR32 :
		gen_clear_raw32rgb(ptr, ptr_p, ptr_d, x, y, dx, dy, pixel);
		break;
	}
}

static void gen_backdrop_raw8(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, struct backdrop_t* back, const adv_bitmap* map)
{
	unsigned x0 = (back->pos.real_dx - map->size_x) / 2;
	unsigned x1 = back->pos.real_dx -  map->size_x - x0;
	unsigned y0 = (back->pos.real_dy - map->size_y) / 2;
	unsigned y1 = back->pos.real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y, x0, back->pos.real_dy, backdrop_missing_color.background);
	if (x1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x + back->pos.real_dx - x1, back->pos.real_y, x1, back->pos.real_dy, backdrop_missing_color.background);
	if (y0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y, back->pos.real_dx, y0, backdrop_missing_color.background);
	if (y1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y + back->pos.real_dy - y1, back->pos.real_dx, y1, backdrop_missing_color.background);
	unsigned char* buffer = ptr + (back->pos.real_y + y0) * ptr_d + (back->pos.real_x + x0) * ptr_p;
	for(unsigned cy=0;cy<map->size_y;++cy) {
		memcpy(buffer, bitmap_line(const_cast<adv_bitmap*>(map), cy), map->size_x);
		buffer += ptr_d;
	}
}

static void gen_backdrop_raw16(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, struct backdrop_t* back, const adv_bitmap* map)
{
	unsigned x0 = (back->pos.real_dx - map->size_x) / 2;
	unsigned x1 = back->pos.real_dx -  map->size_x - x0;
	unsigned y0 = (back->pos.real_dy - map->size_y) / 2;
	unsigned y1 = back->pos.real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y, x0, back->pos.real_dy, backdrop_missing_color.background);
	if (x1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x + back->pos.real_dx - x1, back->pos.real_y, x1, back->pos.real_dy, backdrop_missing_color.background);
	if (y0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y, back->pos.real_dx, y0, backdrop_missing_color.background);
	if (y1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y + back->pos.real_dy - y1, back->pos.real_dx, y1, backdrop_missing_color.background);
	unsigned char* buffer = ptr + (back->pos.real_y + y0) * ptr_d + (back->pos.real_x + x0) * ptr_p;
	for(unsigned cy=0;cy<map->size_y;++cy) {
		memcpy(buffer, bitmap_line(const_cast<adv_bitmap*>(map), cy), map->size_x * 2);
		buffer += ptr_d;
	}
}

static void gen_backdrop_raw32(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, struct backdrop_t* back, const adv_bitmap* map)
{
	unsigned x0 = (back->pos.real_dx - map->size_x) / 2;
	unsigned x1 = back->pos.real_dx -  map->size_x - x0;
	unsigned y0 = (back->pos.real_dy - map->size_y) / 2;
	unsigned y1 = back->pos.real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y, x0, back->pos.real_dy, backdrop_missing_color.background);
	if (x1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x + back->pos.real_dx - x1, back->pos.real_y, x1, back->pos.real_dy, backdrop_missing_color.background);
	if (y0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y, back->pos.real_dx, y0, backdrop_missing_color.background);
	if (y1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x, back->pos.real_y + back->pos.real_dy - y1, back->pos.real_dx, y1, backdrop_missing_color.background);
	unsigned char* buffer = ptr + (back->pos.real_y + y0) * ptr_d + (back->pos.real_x + x0) * ptr_p;
	for(unsigned cy=0;cy<map->size_y;++cy) {
		memcpy(buffer, bitmap_line(const_cast<adv_bitmap*>(map), cy), map->size_x * 4);
		buffer += ptr_d;
	}
}

//---------------------------------------------------------------------------
// Video put

static void video_box(int x, int y, int dx, int dy, int width, const int_rgb& color)
{
	adv_pixel pixel = video_pixel_get(color.r, color.g, color.b);
	video_clear(x, y, dx, width, pixel);
	video_clear(x, y+dy-width, dx, width, pixel);
	video_clear(x, y+width, width, dy-2*width, pixel);
	video_clear(x+dx-width, y+width, width, dy-2*width, pixel);
}

static void video_backdrop_box()
{
	for(int i=0;i<backdrop_mac;++i) {
		struct backdrop_t* back = backdrop_map + i;
		if (back->pos.highlight && backdrop_cursor) {
			unsigned x = back->pos.real_x - backdrop_outline;
			unsigned y = back->pos.real_y - backdrop_outline;
			unsigned dx = back->pos.real_dx + 2*backdrop_outline;
			unsigned dy = back->pos.real_dy + 2*backdrop_outline;

			video_write_lock();
			video_box(x, y, dx, dy, backdrop_cursor, backdrop_box_color.foreground);
			video_write_unlock(x, y, dx, dy);

			int_rgb c = backdrop_box_color.foreground;
			backdrop_box_color.foreground = backdrop_box_color.background;
			backdrop_box_color.background = c;
		}
	}
}

//---------------------------------------------------------------------------
// Text put

static void int_clear_raw(int x, int y, int dx, int dy, const int_rgb& color)
{
	gen_clear_raw(video_buffer, video_buffer_pixel_size, video_buffer_line_size, x, y, dx, dy, color);
}

static void int_backdrop_raw(struct backdrop_t* back, const adv_bitmap* map)
{
	switch (video_bytes_per_pixel()) {
		case 1 :
			gen_backdrop_raw8(video_buffer, video_buffer_pixel_size, video_buffer_line_size, back, map);
			break;
		case 2 :
			gen_backdrop_raw16(video_buffer, video_buffer_pixel_size, video_buffer_line_size, back, map);
			break;
		case 4 :
			gen_backdrop_raw32(video_buffer, video_buffer_pixel_size, video_buffer_line_size, back, map);
			break;
	}
}

static void int_put8rgb_char_font(unsigned x, unsigned y, unsigned bitmap, adv_pixel pixel_foreground, adv_pixel pixel_background)
{
	adv_bitmap* src = real_font_map->data[bitmap];
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src, cy);
		unsigned char* dst_ptr = buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? pixel_foreground : pixel_background;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

static void int_put16rgb_char_font(unsigned x, unsigned y, unsigned bitmap, adv_pixel pixel_foreground, adv_pixel pixel_background)
{
	adv_bitmap* src = real_font_map->data[bitmap];
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src, cy);
		unsigned short* dst_ptr = (unsigned short*)buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? pixel_foreground : pixel_background;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

static void int_put32rgb_char_font(unsigned x, unsigned y, unsigned bitmap, adv_pixel pixel_foreground, adv_pixel pixel_background)
{
	adv_bitmap* src = real_font_map->data[bitmap];
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src, cy);
		unsigned* dst_ptr = (unsigned*)buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? pixel_foreground : pixel_background;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

unsigned int_put_width(char c)
{
	adv_bitmap* src = real_font_map->data[(unsigned char)c];
	if (int_orientation & ORIENTATION_FLIP_XY)
		return src->size_y;
	else
		return src->size_x;
}

void int_put(int x, int y, char c, const int_color& color)
{
	if (x>=0 && y>=0 && x+int_put_width(c)<=int_dx_get() && y+int_font_dy_get()<=int_dy_get()) {
		adv_bitmap* src = real_font_map->data[(unsigned char)c];
		if (int_orientation & ORIENTATION_FLIP_XY)
			swap(x, y);
		if (int_orientation & ORIENTATION_MIRROR_X)
			x = video_size_x() - src->size_x - x;
		if (int_orientation & ORIENTATION_MIRROR_Y)
			y = video_size_y() - src->size_y - y;

		assert( x>=0 && y>=0 && x+src->size_x<=video_size_x() &&  y+src->size_y<=video_size_y() );

		adv_pixel pixel_foreground = video_pixel_get(color.foreground.r, color.foreground.g, color.foreground.b);
		adv_pixel pixel_background = video_pixel_get(color.background.r, color.background.g, color.background.b);

		switch (video_index()) {
		case MODE_FLAGS_INDEX_BGR8 :
			int_put8rgb_char_font(x, y, (unsigned char)c, pixel_foreground, pixel_background);
			break;
		case MODE_FLAGS_INDEX_BGR15 :
		case MODE_FLAGS_INDEX_BGR16 :
			int_put16rgb_char_font(x, y, (unsigned char)c, pixel_foreground, pixel_background);
			break;
		case MODE_FLAGS_INDEX_BGR32 :
			int_put32rgb_char_font(x, y, (unsigned char)c, pixel_foreground, pixel_background);
			break;
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

void int_put_special(bool& in, int x, int y, int dx, const string& s, const int_color& c0, const int_color& c1, const int_color& c2)
{
	for(unsigned i=0;i<s.length();++i) {
		if (int_put_width(s[i]) <= dx) {
			if (s[i]=='(' || s[i]=='[')
				in = true;
			if (!in && i==0) {
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

static void int_clear_noclip(int x, int y, int dx, int dy, const int_rgb& color)
{
	if (int_orientation & ORIENTATION_FLIP_XY) {
		swap(x, y);
		swap(dx, dy);
	}

	if (int_orientation & ORIENTATION_MIRROR_X)
		x = video_size_x() - dx - x;
	if (int_orientation & ORIENTATION_MIRROR_Y)
		y = video_size_y() - dy - y;

	int_clear_raw(x, y, dx, dy, color);
}

void int_clear() 
{
	// clear the bitmap
	if ((video_driver_flags() & VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)!=0
		&& video_buffer_pixel_size > 1) {
		memset(video_buffer, 0xFF, video_buffer_size);
	} else {
		memset(video_buffer, 0x0, video_buffer_size);
	}
}	

void int_clear(int x, int y, int dx, int dy, const int_rgb& color)
{
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
	if (dx > 0 && dy > 0)
		int_clear_noclip(x, y, dx, dy, color);
}

bool int_image(const char* file)
{
	if (color_def_type_get(video_color_def()) != adv_color_type_rgb)
		return false;

	adv_fz* f = fzopen(file, "r");
	if (!f)
		return false;

	unsigned pix_width;
	unsigned pix_height;
	unsigned pix_pixel;
	unsigned char* dat_ptr;
	unsigned dat_size;
	unsigned char* pix_ptr;
	unsigned pix_scanline;
	unsigned char* pal_ptr;
	unsigned pal_size;

	int r = png_read(&pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, f);
	if (r != 0) {
		fzclose(f);
		return false;
	}

	fzclose(f);

	adv_color_def def = png_color_def(pix_pixel);

	if (color_def_type_get(def) != adv_color_type_rgb) {
		free(dat_ptr);
		free(pal_ptr);
		return false;
	}

	adv_bitmap* bitmap = bitmap_import(pix_width, pix_height, pix_pixel, 0, 0, pix_ptr, pix_scanline);
	if (!bitmap) {
		free(dat_ptr);
		free(pal_ptr);
		return 0;
	}

	adv_bitmap* bitmap_scr = bitmap_import(video_size_x(), video_size_y(), video_bits_per_pixel(), 0, 0, video_buffer, video_buffer_line_size);

	if (def != video_color_def()) {
		adv_bitmap* bitmap_cvt = bitmap_alloc(bitmap->size_x, bitmap->size_y, video_bits_per_pixel());
		bitmap_cvt_rgb(bitmap_cvt, video_color_def(), bitmap, def);
		bitmap_put(bitmap_scr, 0, 0, bitmap_cvt);
		bitmap_free(bitmap_cvt);
	} else {
		bitmap_put(bitmap_scr, 0, 0, bitmap);
	}

	bitmap_free(bitmap_scr);

	free(dat_ptr);
	free(pal_ptr);
	bitmap_free(bitmap);

	return true;
}

void int_box(int x, int y, int dx, int dy, int width, const int_rgb& color)
{
	int_clear(x, y, dx, width, color);
	int_clear(x, y+dy-width, dx, width, color);
	int_clear(x, y+width, width, dy-2*width, color);
	int_clear(x+dx-width, y+width, width, dy-2*width, color);
}

void int_put(int x, int y, const string& s, const int_color& color)
{
	for(unsigned i=0;i<s.length();++i) {
		int_put(x, y, s[i], color);
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

unsigned int_put_width(const string& s)
{
	unsigned size = 0;
	for(unsigned i=0;i<s.length();++i)
		size += int_put_width(s[i]);
	return size;
}

unsigned int_put_right(int x, int y, int dx, const string& s, const int_color& color)
{
	unsigned size = int_put_width(s);
	return int_put(x + dx - size, y, dx, s, color);
}

//---------------------------------------------------------------------------
// Backdrop

// Reduce the size of the cache
static void int_backdrop_cache_reduce()
{
	// limit the cache size
	if (backdrop_cache_list) {
		while (backdrop_cache_list->size() > backdrop_cache_max) {
			list<backdrop_data*>::iterator i = backdrop_cache_list->end();
			--i;
			backdrop_data* data = *i;
			backdrop_cache_list->erase(i);
			delete data;
		}
	}
}

// Delete or insert in the cache the backdrop image
static void int_backdrop_cache(backdrop_data* data)
{
	if (data) {
		if (data->has_bitmap()) {
			// insert the image in the cache
			backdrop_cache_list->insert(backdrop_cache_list->begin(), data);
		} else {
			delete data;
		}
	}
}

static backdrop_data* int_backdrop_cache_find(const resource& res, unsigned dx, unsigned dy, unsigned aspectx, unsigned aspecty)
{
	// search in the cache
	for(list<backdrop_data*>::iterator i=backdrop_cache_list->begin();i!=backdrop_cache_list->end();++i) {
		if ((*i)->res_get() == res
			&& dx == (*i)->target_dx_get()
			&& dy == (*i)->target_dy_get()) {

			// extract from the cache
			backdrop_data* data = *i;

			// remove from the cache
			backdrop_cache_list->erase(i);

			return data;
		}
	}

	return new backdrop_data(res, dx, dy, aspectx, aspecty);
}

static void int_backdrop_cache_all()
{
	for(unsigned i=0;i<backdrop_mac;++i) {
		int_backdrop_cache(backdrop_map[i].data);
		backdrop_map[i].data = 0;
	}
}

// Select the backdrop
void int_backdrop_set(int back_index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty)
{
	assert( back_index < backdrop_mac );

	int_backdrop_cache(backdrop_map[back_index].data);

	backdrop_map[back_index].data = int_backdrop_cache_find(res, backdrop_map[back_index].pos.dx, backdrop_map[back_index].pos.dy, aspectx, aspecty);

	if (backdrop_map[back_index].last == res) {
		if (backdrop_map[back_index].pos.highlight != highlight) {
			backdrop_map[back_index].pos.highlight = highlight;
			backdrop_map[back_index].pos.redraw = true;
		}
	} else {
		backdrop_map[back_index].last = res;
		backdrop_map[back_index].pos.highlight = highlight;
		backdrop_map[back_index].pos.redraw = true;
	}
}

// Clear the backdrop
void int_backdrop_clear(int back_index, bool highlight)
{
	assert( back_index < backdrop_mac );

	int_backdrop_cache(backdrop_map[back_index].data);

	backdrop_map[back_index].data = 0;

	if (!backdrop_map[back_index].last.is_valid()) {
		if (backdrop_map[back_index].pos.highlight != highlight) {
			backdrop_map[back_index].pos.highlight = highlight;
			backdrop_map[back_index].pos.redraw = true;
		}
	} else {
		backdrop_map[back_index].last = resource();
		backdrop_map[back_index].pos.highlight = highlight;
		backdrop_map[back_index].pos.redraw = true;
	}
}

void int_backdrop_done()
{
	assert(backdrop_mac != 0);

	for(int i=0;i<backdrop_mac;++i) {
		if (backdrop_map[i].data)
			delete backdrop_map[i].data;
		backdrop_map[i].data = 0;
	}
	backdrop_mac = 0;

	for(list<backdrop_data*>::iterator i=backdrop_cache_list->begin();i!=backdrop_cache_list->end();++i)
		delete *i;
	delete backdrop_cache_list;
	backdrop_cache_list = 0;
}

void int_backdrop_init(const int_color& Abackdrop_missing_color, const int_color& Abackdrop_box_color, unsigned Amac, unsigned outline, unsigned cursor, double expand_factor)
{
	assert(backdrop_mac == 0);

	backdrop_cache_list = new pbackdrop_list;
	backdrop_missing_color = Abackdrop_missing_color;
	backdrop_box_color = Abackdrop_box_color;
	backdrop_outline = outline;
	backdrop_cursor = cursor;
	backdrop_expand_factor = expand_factor;

	backdrop_mac = Amac;
	backdrop_cache_max = backdrop_mac*2 + 1;

	int_updating_active = false;

	for(int i=0;i<backdrop_mac;++i) {
		backdrop_map[i].data = 0;
		backdrop_map[i].last = resource();
		backdrop_map[i].pos.highlight = false;
		backdrop_map[i].pos.redraw = true;
	}

	if (backdrop_mac == 1 && video_index_rgb_to_packed_is_available())
		video_index_rgb_to_packed();
	if (backdrop_mac > 1 && video_index_packed_to_rgb_is_available())
		video_index_packed_to_rgb(0);
}

// Set the backdrop position and size
void int_backdrop_pos(int back_index, int x, int y, int dx, int dy)
{
	assert( back_index < backdrop_mac );

	int_backdrop_cache(backdrop_map[back_index].data);
	backdrop_map[back_index].data = 0;

	backdrop_map[back_index].last = resource();
	backdrop_map[back_index].pos.highlight = false;
	backdrop_map[back_index].pos.redraw = true;

	backdrop_map[back_index].pos.x = backdrop_map[back_index].pos.real_x = x + backdrop_outline;
	backdrop_map[back_index].pos.y = backdrop_map[back_index].pos.real_y = y + backdrop_outline;
	backdrop_map[back_index].pos.dx = backdrop_map[back_index].pos.real_dx = dx - 2*backdrop_outline;
	backdrop_map[back_index].pos.dy = backdrop_map[back_index].pos.real_dy = dy - 2*backdrop_outline;

	if (int_orientation & ORIENTATION_FLIP_XY) {
		swap(backdrop_map[back_index].pos.real_x, backdrop_map[back_index].pos.real_y);
		swap(backdrop_map[back_index].pos.real_dx, backdrop_map[back_index].pos.real_dy);
	}
	if (int_orientation & ORIENTATION_MIRROR_X) {
		backdrop_map[back_index].pos.real_x = video_size_x() - backdrop_map[back_index].pos.real_x - backdrop_map[back_index].pos.real_dx;
	}
	if (int_orientation & ORIENTATION_MIRROR_Y) {
		backdrop_map[back_index].pos.real_y = video_size_y() - backdrop_map[back_index].pos.real_y - backdrop_map[back_index].pos.real_dy;
	}
}

//---------------------------------------------------------------------------
// Clip

static bool int_clip_active;
static bool int_clip_running;
static resource int_clip_res;
static int int_clip_index;
static adv_fz* int_clip_f;
static unsigned int_clip_aspectx;
static unsigned int_clip_aspecty;
target_clock_t int_clip_wait;
unsigned int_clip_count;
adv_mng* int_mng_context;

void int_clip_clear()
{
	if (int_clip_f) {
		fzclose(int_clip_f);
		mng_done(int_mng_context);
		int_clip_f = 0;
	}
	int_clip_active = false;
	int_clip_running = false;
}

void int_clip_set(int back_index, const resource& res, unsigned aspectx, unsigned aspecty)
{
	int_clip_clear();

	int_clip_index = back_index;
	int_clip_res = res;
	int_clip_active = true;
	int_clip_running = false;
	int_clip_aspectx = aspectx;
	int_clip_aspecty = aspecty;
}

void int_clip_init()
{
	int_clip_f = 0;
	int_clip_active = false;
}

void int_clip_start()
{
	if (!int_clip_active)
		return;

	int_clip_running = true;
	int_clip_wait = target_clock(); // reset the start time
}

void int_clip_done()
{
	int_clip_clear();
}

static bool int_clip_need_load()
{
	if (!int_clip_active)
		return false;

	if (!int_clip_running)
		return false;

	if (!int_clip_f)
		return true;

	if (target_clock() > int_clip_wait)
		return true;

	return false;
}

bool int_clip_is_active()
{
	return int_clip_active && int_clip_running;
}

static adv_bitmap* int_clip_load(adv_color_rgb* rgb_map, unsigned* rgb_max)
{
	if (!int_clip_active)
		return 0;

	if (!int_clip_f) {
		// first load
		int_clip_f = int_clip_res.open();
		if (!int_clip_f) {
			int_clip_active = false;
			int_clip_f = 0;
			return 0;
		}

		int_mng_context = mng_init(int_clip_f);
		if (int_mng_context == 0) {
			fzclose(int_clip_f);
			int_clip_f = 0;
			int_clip_active = false;
			return 0;
		}

		int_clip_wait = target_clock();
		int_clip_count = 0;
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

	int r = mng_read(int_mng_context, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, int_clip_f);
	if (r != 0) {
		mng_done(int_mng_context);
		fzclose(int_clip_f);
		int_clip_f = 0;
		int_clip_running = false;
		return 0;
	}

	double delay = tick / (double)mng_frequency_get(int_mng_context);

	adv_bitmap* bitmap = bitmappalette_import(rgb_map, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
	if (!bitmap) {
		free(dat_ptr);
		free(pal_ptr);
		mng_done(int_mng_context);
		fzclose(int_clip_f);
		int_clip_f = 0;
		int_clip_active = false;
		return 0;
	}

	free(pal_ptr);

	int_clip_wait += (target_clock_t)(delay * TARGET_CLOCKS_PER_SEC);

	++int_clip_count;

	return bitmap;
}

//---------------------------------------------------------------------------
// Update

extern "C" int fast_exit_handler(void)
{
	if (int_wait_for_backdrop)
		return 0;

	int_keypressed();
	return int_key_saved == INT_KEY_PGUP
		|| int_key_saved == INT_KEY_PGDN
		|| int_key_saved == INT_KEY_INS
		|| int_key_saved == INT_KEY_DEL
		|| int_key_saved == INT_KEY_HOME
		|| int_key_saved == INT_KEY_END
		|| int_key_saved == INT_KEY_UP
		|| int_key_saved == INT_KEY_DOWN
		|| int_key_saved == INT_KEY_LEFT
		|| int_key_saved == INT_KEY_RIGHT
		|| int_key_saved == INT_KEY_MODE;
}

static void icon_apply(adv_bitmap* bitmap, adv_bitmap* bitmap_mask, adv_color_rgb* rgb, unsigned* rgb_max, const int_rgb& trasparent)
{
	unsigned index;
	if (*rgb_max == 256) {
		unsigned count[256];
		for(unsigned i=0;i<256;++i)
			count[i] = 0;

		for(unsigned y=0;y<bitmap->size_y;++y) {
			uint8* line = bitmap_line(bitmap, y);
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
				uint8* line = bitmap_line(bitmap, y);
				for(unsigned x=0;x<bitmap->size_x;++x)
					if (line[x] == index)
						line[x] = substitute;
			}
		}
	} else {
		index = *rgb_max;
		++*rgb_max;
	}

	rgb[index].red = trasparent.r;
	rgb[index].green = trasparent.g;
	rgb[index].blue = trasparent.b;
	rgb[index].alpha = 0;

	for(unsigned y=0;y<bitmap->size_y;++y) {
		uint8* line = bitmap_line(bitmap, y);
		uint8* line_mask = bitmap_line(bitmap_mask, y);
		for(unsigned x=0;x<bitmap->size_x;++x) {
			if (!line_mask[x])
				line[x] = index;
		}
	}
}

static adv_bitmap* backdrop_load(const resource& res, adv_color_rgb* rgb, unsigned* rgb_max)
{
	string ext = file_ext(res.path_get());

	if (ext == ".png") {
		adv_fz* f = res.open();
		if (!f)
			return 0;

		unsigned pix_width;
		unsigned pix_height;
		unsigned pix_pixel;
		unsigned char* dat_ptr;
		unsigned dat_size;
		unsigned char* pix_ptr;
		unsigned pix_scanline;
		unsigned char* pal_ptr;
		unsigned pal_size;

		int r = png_read(&pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, f);
		if (r != 0) {
			fzclose(f);
			return 0;
		}

		adv_bitmap* bitmap = bitmappalette_import(rgb, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
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

	if (ext == ".pcx") {
		adv_fz* f = res.open();
		if (!f)
			return 0;
		adv_bitmap* bitmap = pcx_load(f, rgb, rgb_max);
		fzclose(f);
		return bitmap;
	}

	if (ext == ".ico") {
		adv_fz* f = res.open();
		if (!f)
			return 0;
		adv_bitmap* bitmap_mask;
		adv_bitmap* bitmap = icon_load(f, rgb, rgb_max, &bitmap_mask);
		if (!bitmap) {
			fzclose(f);
			return 0;
		}

		// TODO
		icon_apply(bitmap, bitmap_mask, rgb, rgb_max, backdrop_missing_color.background);

		bitmap_free(bitmap_mask);
		fzclose(f);
		return bitmap;
	}

	if (ext == ".mng") {
		adv_mng* mng;

		adv_fz* f = res.open();
		if (!f)
			return 0;

		mng = mng_init(f);
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

		int r = mng_read(mng, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, f);
		if (r != 0) {
			mng_done(mng);
			fzclose(f);
			return 0;
		}

		adv_bitmap* bitmap = bitmappalette_import(rgb, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
		if (!bitmap) {
			free(dat_ptr);
			free(pal_ptr);
			mng_done(mng);
			fzclose(f);
			return 0;
		}

		free(pal_ptr);

		// duplicate the bitmap, it must exists also after destroying the mng context
		adv_bitmap* dup_bitmap = bitmap_dup(bitmap);
		bitmap_free(bitmap);
		bitmap = dup_bitmap;

		mng_done(mng);
		fzclose(f);

		return bitmap;
	}

	return 0;
}

void int_backdrop_compute_real_size(unsigned* rx, unsigned* ry, struct backdrop_t* back, adv_bitmap* bitmap, unsigned aspectx, unsigned aspecty)
{
	if (int_orientation & ORIENTATION_FLIP_XY) {
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

	unsigned dx = back->pos.real_dx;
	unsigned dy = back->pos.real_dy;
	if (aspectx * dy > aspecty * dx) {
		*rx = dx;
		*ry = static_cast<unsigned>(dx * aspecty * backdrop_expand_factor / aspectx);
	} else {
		*rx = static_cast<unsigned>(dy * aspectx * backdrop_expand_factor / aspecty);
		*ry = dy;
	}
	if (*rx > dx)
		*rx = dx;
	if (*ry > dy)
		*ry = dy;
}

void int_backdrop_compute_virtual_size(unsigned* rx, unsigned* ry, struct backdrop_t* back, adv_bitmap* bitmap, unsigned aspectx, unsigned aspecty)
{
	if (!aspectx || !aspecty) {
		aspectx = bitmap->size_x;
		aspecty = bitmap->size_y;
	}

	if (!aspectx || !aspecty) {
		aspectx = 1;
		aspecty = 1;
	}

	if (int_orientation & ORIENTATION_FLIP_XY) {
		aspectx *= 4 * video_size_y();
		aspecty *= 3 * video_size_x();
	} else {
		aspectx *= 3 * video_size_x();
		aspecty *= 4 * video_size_y();
	}

	unsigned dx = back->pos.dx;
	unsigned dy = back->pos.dy;

	if (aspectx * dy > aspecty * dx) {
		*rx = dx;
		*ry = static_cast<unsigned>(dx * aspecty * backdrop_expand_factor / aspectx);
	} else {
		*rx = static_cast<unsigned>(dy * aspectx * backdrop_expand_factor / aspecty);
		*ry = dy;
	}
	if (*rx > dx)
		*rx = dx;
	if (*ry > dy)
		*ry = dy;
}

static adv_bitmap* int_backdrop_compute_bitmap(struct backdrop_t* back, adv_bitmap* bitmap, adv_color_rgb* rgb, unsigned* rgb_max, unsigned aspectx, unsigned aspecty)
{
	if (bitmap->size_x < back->pos.dx) {
		unsigned rx, ry;

		// flip
		bitmap_orientation(bitmap, int_orientation & ORIENTATION_FLIP_XY);

		int_backdrop_compute_real_size(&rx, &ry, back, bitmap, aspectx, aspecty);

		// resize & mirror
		adv_bitmap* shrink_bitmap = bitmap_resize(bitmap, 0, 0, bitmap->size_x, bitmap->size_y, rx, ry, int_orientation);
		if (!shrink_bitmap) {
			bitmap_free(bitmap);
			return 0;
		}
		bitmap_free(bitmap);
		bitmap = shrink_bitmap;
	} else {
		unsigned rx, ry;

		int_backdrop_compute_virtual_size(&rx, &ry, back, bitmap, aspectx, aspecty);

		// resize & mirror
		unsigned flip = (int_orientation & ORIENTATION_FLIP_XY) ? ORIENTATION_MIRROR_X | ORIENTATION_MIRROR_Y : 0;
		adv_bitmap* shrink_bitmap = bitmap_resize(bitmap, 0, 0, bitmap->size_x, bitmap->size_y, rx, ry, int_orientation ^ flip);
		if (!shrink_bitmap) {
			bitmap_free(bitmap);
			return 0;
		}
		bitmap_free(bitmap);
		bitmap = shrink_bitmap;

		// flip
		bitmap_orientation(bitmap, int_orientation & ORIENTATION_FLIP_XY);
	}

	adv_bitmap* pal_bitmap;

	pal_bitmap = bitmap_alloc(bitmap->size_x, bitmap->size_y, video_bits_per_pixel());
	if (bitmap->bytes_per_pixel == 1) {
		/* palette bitmap */
		unsigned color_map[256];
		for(unsigned i=0;i<*rgb_max;++i)
			video_pixel_make(color_map + i, rgb[i].red, rgb[i].green, rgb[i].blue);
		bitmap_cvt_palette(pal_bitmap, bitmap, color_map);
	} else {
		/* rgb bitmap */
		adv_color_def def = png_color_def(bitmap->bytes_per_pixel);
		bitmap_cvt_rgb(pal_bitmap, video_color_def(), bitmap, def);
	}

	bitmap_free(bitmap);
	bitmap = pal_bitmap;

	return bitmap;
}

static void int_backdrop_clear(struct backdrop_t* back)
{
	int_clear(back->pos.x, back->pos.y, back->pos.dx, back->pos.dy, backdrop_missing_color.background);
}

static void int_backdrop_load(struct backdrop_t* back)
{
	if (!back->data->bitmap_get()) {
		adv_color_rgb rgb[256];
		unsigned rgb_max;

		adv_bitmap* bitmap = backdrop_load(back->data->res_get(), rgb, &rgb_max);
		if (!bitmap)
			return;

		bitmap = int_backdrop_compute_bitmap(back, bitmap, rgb, &rgb_max, back->data->aspectx_get(), back->data->aspecty_get());
		if (!bitmap)
			return;

		// store
		back->data->bitmap_set(bitmap);
	}
}

// Update the backdrop image
static void int_backdrop_update(struct backdrop_t* back)
{
	if (back->data) {
		if (!fast_exit_handler())
			int_backdrop_load(back);
	}

	if (back->pos.redraw) {
		if (back->data) {
			if (back->data->bitmap_get()) {
				back->pos.redraw = false;
				int_backdrop_raw(back, back->data->bitmap_get());
			} else {
				// the image need to be update when loaded
				int_backdrop_clear(back);
			}
		} else {
			back->pos.redraw = false;
			int_backdrop_clear(back);
		}

		if (backdrop_outline)
			int_box(back->pos.x-backdrop_outline, back->pos.y-backdrop_outline, back->pos.dx+2*backdrop_outline, back->pos.dy+2*backdrop_outline, backdrop_outline, backdrop_missing_color.foreground);
	}
}

static void int_copy_partial(unsigned y0, unsigned y1)
{
	video_write_lock();

	if (video_is_unchained()) {
		for(unsigned plane=0;plane<4;++plane) {
			video_unchained_plane_set(plane);
			unsigned char* buffer = video_buffer + plane * video_bytes_per_pixel() + y0 * video_buffer_line_size;

			for(unsigned y=y0;y<y1;++y) {
				unsigned char* dst_ptr = video_write_line(y);
				unsigned char* src_ptr = buffer;
				for(unsigned x=0;x<video_size_x()/4;++x) {
					*dst_ptr = *src_ptr;
					++dst_ptr;
					src_ptr += 4;
				}
				buffer += video_buffer_line_size;
			}
		}
	} else {
		unsigned char* buffer = video_buffer + y0 * video_buffer_line_size;
		for(unsigned y=y0;y<y1;++y) {
			memcpy(video_write_line(y), buffer, video_buffer_line_size );
			buffer += video_buffer_line_size;
		}
	}

	video_write_unlock(0, y0, video_size_x(), y1 - y0);
}

unsigned int_update_pre(bool progressive)
{
	int_updating_active = false;

	play_poll();

	int_backdrop_cache_reduce();

	int y = 0;
	for(int i=0;i<backdrop_mac;++i) {
		play_poll();

		// this thick work only if the backdrop are from top to down
		if (int_orientation == 0 && progressive) {
			// update progressively the screen
			int yl = backdrop_map[i].pos.real_y - backdrop_outline;
			if (yl > y) {
				int_copy_partial(y, yl);
				y = yl;
			}
		}

		int_backdrop_update(backdrop_map + i);
	}

	int_backdrop_cache_all();

	return y;
}

void int_update_post(unsigned y)
{
	play_poll();

	int_copy_partial(y, video_size_y());

	video_backdrop_box();

	play_poll();

	int_updating_active = true;
}

void int_update(bool progressive)
{
	int_update_post(int_update_pre(progressive));
}

//---------------------------------------------------------------------------
// Key

#define SEQ_MAX 256

struct key_cvt {
	const char* name;
	unsigned event;
	unsigned seq[SEQ_MAX];
};

// Special operator
#define OP_NONE KEYB_MAX
#define OP_OR (KEYB_MAX + 1)
#define OP_NOT (KEYB_MAX + 2)

static struct key_cvt KEYTAB[] = {
{"up", INT_KEY_UP, { KEYB_UP, OP_OR, KEYB_8_PAD, KEYB_MAX } },
{"down", INT_KEY_DOWN, { KEYB_DOWN, OP_OR, KEYB_2_PAD, KEYB_MAX } },
{"left", INT_KEY_LEFT, { KEYB_LEFT, OP_OR, KEYB_4_PAD, KEYB_MAX } },
{"right", INT_KEY_RIGHT, { KEYB_RIGHT, OP_OR, KEYB_6_PAD, KEYB_MAX } },
{"enter", INT_KEY_ENTER, { KEYB_ENTER, OP_OR, KEYB_ENTER_PAD, KEYB_MAX } },
{"shutdown", INT_KEY_OFF, { KEYB_LCONTROL, KEYB_ESC, KEYB_MAX } },
{"esc", INT_KEY_ESC, { KEYB_ESC, KEYB_MAX } },
{"space", INT_KEY_SPACE, { KEYB_SPACE, KEYB_MAX } },
{"mode", INT_KEY_MODE, { KEYB_TAB, KEYB_MAX } },
{"home", INT_KEY_HOME, { KEYB_HOME, KEYB_MAX } },
{"end", INT_KEY_END, { KEYB_END, KEYB_MAX } },
{"pgup", INT_KEY_PGUP, { KEYB_PGUP, KEYB_MAX } },
{"pgdn", INT_KEY_PGDN, { KEYB_PGDN, KEYB_MAX } },
{"help", INT_KEY_HELP, { KEYB_F1, KEYB_MAX } },
{"group", INT_KEY_GROUP, { KEYB_F2, KEYB_MAX } },
{"type", INT_KEY_TYPE, { KEYB_F3, KEYB_MAX } },
{"exclude", INT_KEY_EXCLUDE, { KEYB_F4, KEYB_MAX } },
{"sort", INT_KEY_SORT, { KEYB_F5, KEYB_MAX } },
{"setgroup", INT_KEY_SETGROUP, { KEYB_F9, KEYB_MAX } },
{"settype", INT_KEY_SETTYPE, { KEYB_F10, KEYB_MAX } },
{"runclone", INT_KEY_RUN_CLONE, { KEYB_F12, KEYB_MAX } },
{"del", INT_KEY_DEL, { KEYB_DEL, KEYB_MAX } },
{"ins", INT_KEY_INS, { KEYB_INSERT, KEYB_MAX } },
{"command", INT_KEY_COMMAND, { KEYB_F8, KEYB_MAX } },
{"menu", INT_KEY_MENU, { KEYB_BACKQUOTE, OP_OR, KEYB_BACKSLASH, KEYB_MAX } },
{"emulator", INT_KEY_EMU, { KEYB_F6, KEYB_MAX } },
{"snapshot", INT_KEY_SNAPSHOT, { KEYB_PERIOD_PAD, KEYB_MAX } },
{"rotate", INT_KEY_ROTATE, { KEYB_0_PAD, KEYB_MAX } },
{"lock", INT_KEY_LOCK, { KEYB_SCRLOCK, KEYB_MAX } },
{ 0, 0, { 0 } }
};

int seq_pressed(const unsigned* code)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for(j=0;j<SEQ_MAX;++j) {
		switch (code[j]) {
			case OP_NONE :
				return res && count;
			case OP_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case OP_NOT :
				invert = !invert;
				break;
			default:
				if (res) {
					adv_bool pressed = 0;
					for(unsigned k=0;k<keyb_count_get();++k)
						pressed = pressed || (keyb_get(k, code[j]) != 0);
					if ((pressed != 0) == invert)
						res = 0;
				}
				invert = 0;
				++count;
		}
	}
	return res && count;
}

static int seq_valid(const unsigned* seq)
{
	int j;
	int positive = 0; // if isn't a completly negative sequence
	int pred_not = 0;
	int operand = 0;
	for(j=0;j<SEQ_MAX;++j)
	{
		switch (seq[j])
		{
			case OP_NONE :
				return positive && operand;
			case OP_OR :
				if (!operand || !positive)
					return 0;
				pred_not = 0;
				positive = 0;
				operand = 0;
				break;
			case OP_NOT :
				if (pred_not)
					return 0;
				pred_not = !pred_not;
				operand = 0;
				break;
			default:
				if (!pred_not)
					positive = 1;
				pred_not = 0;
				operand = 1;
				break;
		}
	}

	return positive && operand;
}

static struct key_conv {
	int code;
	char c;
} KEY_CONV[] = {
{ KEYB_A, 'a' },
{ KEYB_B, 'b' },
{ KEYB_C, 'c' },
{ KEYB_D, 'd' },
{ KEYB_E, 'e' },
{ KEYB_F, 'f' },
{ KEYB_G, 'g' },
{ KEYB_H, 'h' },
{ KEYB_I, 'i' },
{ KEYB_J, 'j' },
{ KEYB_K, 'k' },
{ KEYB_L, 'l' },
{ KEYB_M, 'm' },
{ KEYB_N, 'n' },
{ KEYB_O, 'o' },
{ KEYB_P, 'p' },
{ KEYB_Q, 'q' },
{ KEYB_R, 'r' },
{ KEYB_S, 's' },
{ KEYB_T, 't' },
{ KEYB_U, 'u' },
{ KEYB_V, 'v' },
{ KEYB_W, 'w' },
{ KEYB_X, 'x' },
{ KEYB_Y, 'y' },
{ KEYB_Z, 'z' },
{ KEYB_0, '0' },
{ KEYB_1, '1' },
{ KEYB_2, '2' },
{ KEYB_3, '3' },
{ KEYB_4, '4' },
{ KEYB_5, '5' },
{ KEYB_6, '6' },
{ KEYB_7, '7' },
{ KEYB_8, '8' },
{ KEYB_9, '9' },
{ KEYB_MAX, ' ' },
};

static int keyboard_raw_poll()
{
	for(const struct key_cvt* i=KEYTAB;i->name;++i) {
		if (seq_pressed(i->seq))
			return i->event;
	}

	if (int_alpha_mode) {
		for(unsigned i=0;KEY_CONV[i].code != KEYB_MAX;++i) {
			for(unsigned k=0;k<keyb_count_get();++k)
				if (keyb_get(k, KEY_CONV[i].code))
					return KEY_CONV[i].c;
		}
	}

	return INT_KEY_NONE;
}

static int key_poll()
{
	static int key_repeat_last = INT_KEY_NONE;

	static target_clock_t key_repeat_last_time = 0;
	static bool key_repeat_last_counter = 0;

	int r = INT_KEY_NONE;

	if (r == INT_KEY_NONE) {
		if (os_is_quit())
			r = INT_KEY_ESC;
	}

	if (r == INT_KEY_NONE) {
		r = keyboard_raw_poll();
	}

	if (r == INT_KEY_NONE) {
		r = int_joystick_button_raw_poll();
	}

	if (r == INT_KEY_NONE) {
		r = int_mouse_button_raw_poll();
	}

	if (r == INT_KEY_NONE) {
		r = int_mouse_move_raw_poll();

		// never repeat or wait or play for the mouse movements
		if (r != INT_KEY_NONE) {
			key_repeat_last = r;
			key_repeat_last_counter = 0;
			key_repeat_last_time = target_clock();
			return r;
		}
	}

	if (r == INT_KEY_NONE) {
		r = int_joystick_move_raw_poll();
	}

	if (r == INT_KEY_NONE) {
		key_repeat_last = INT_KEY_NONE;
		key_repeat_last_counter = 0;
		key_repeat_last_time = target_clock();
		return INT_KEY_NONE;
	} else if (r != key_repeat_last) {
		key_repeat_last = r;
		key_repeat_last_counter = 0;
		key_repeat_last_time = target_clock();
		play_foreground_effect_key(int_sound_event_key);
		return r;
	} else {
		if ((key_repeat_last_counter == 0 && (target_clock() - key_repeat_last_time > int_repeat * TARGET_CLOCKS_PER_SEC / 1000)) ||
			(key_repeat_last_counter > 0 && (target_clock() - key_repeat_last_time > int_repeat_rep * TARGET_CLOCKS_PER_SEC / 1000))) {
			key_repeat_last_time = target_clock();
			++key_repeat_last_counter;
			return r;
		} else {
			return INT_KEY_NONE;
		}
	}
}

void int_idle_repeat_reset()
{
	int_key_last = INT_KEY_NONE;
}

void int_idle_time_reset()
{
	int_idle_time = time(0);
}

void int_idle_0_enable(bool state)
{
	int_idle_0_state = state;
}

void int_idle_1_enable(bool state)
{
	int_idle_1_state = state;
}

static void int_clip_idle()
{
	adv_color_rgb rgb_map[256];
	unsigned rgb_max;
	backdrop_t* back = backdrop_map + int_clip_index;

	if (!int_clip_need_load()) {
		return;
	}

	adv_bitmap* bitmap = int_clip_load(rgb_map, &rgb_max);
	if (!bitmap) {
		// update the screen with the previous backdrop
		int_copy_partial(back->pos.real_y, back->pos.real_y + back->pos.real_dy);
		return;
	}

	adv_pixel pixel = video_pixel_get(backdrop_missing_color.background.r, backdrop_missing_color.background.g, backdrop_missing_color.background.b);

	// source range and steps
	unsigned char* ptr = bitmap->ptr;
	int dw = bitmap->bytes_per_scanline;
	int dp = bitmap->bytes_per_pixel;
	int dx = bitmap->size_x;
	int dy = bitmap->size_y;

	// set the correct orientation
	if (int_orientation & ORIENTATION_FLIP_XY) {
		int t;
		t = dp;
		dp = dw;
		dw = t;
		t = dx;
		dx = dy;
		dy = t;
	}
	if (int_orientation & ORIENTATION_MIRROR_X) {
		ptr = ptr + (dx-1) * dp;
		dp = - dp;
	}
	if (int_orientation & ORIENTATION_MIRROR_Y) {
		ptr = ptr + (dy-1) * dw;
		dw = - dw;
	}

	// destination range
	unsigned dst_dx;
	unsigned dst_dy;
	unsigned dst_x;
	unsigned dst_y;
	dst_x = back->pos.real_x;
	dst_y = back->pos.real_y;
	dst_dx = back->pos.real_dx;
	dst_dy = back->pos.real_dy;

	// compute the size of the bitmap
	unsigned rel_dx;
	unsigned rel_dy;
	int_backdrop_compute_real_size(&rel_dx, &rel_dy, back, bitmap, int_clip_aspectx, int_clip_aspecty);

	// start writing
	video_write_lock();

	// adjust the destination range if too big
	if (dst_dx > rel_dx) {
		unsigned pre_dx = (dst_dx - rel_dx) / 2;
		unsigned post_dx = (dst_dx - rel_dx + 1) / 2;

		if (int_clip_count == 1) {
			video_clear(dst_x, dst_y, pre_dx, dst_dy, pixel);
			video_clear(dst_x + pre_dx + rel_dx, dst_y, post_dx, dst_dy, pixel);
		}

		dst_x += pre_dx;
		dst_dx = rel_dx;
	}
	if (dst_dy > rel_dy) {
		unsigned pre_dy = (dst_dy - rel_dy) / 2;
		unsigned post_dy = (dst_dy - rel_dy + 1) / 2;

		if (int_clip_count == 1) {
			video_clear(dst_x, dst_y, dst_dx, pre_dy, pixel);
			video_clear(dst_x, dst_y + pre_dy + rel_dy, dst_dx, post_dy, pixel);
		}

		dst_y += pre_dy;
		dst_dy = rel_dy;
	}

	unsigned combine = VIDEO_COMBINE_Y_NONE;

	// write
	if (bitmap->bytes_per_pixel == 1) {
		uint32 palette32[256];
		uint16 palette16[256];
		uint8 palette8[256];
		for(unsigned i=0;i<rgb_max;++i) {
			adv_pixel p = video_pixel_get(rgb_map[i].red, rgb_map[i].green, rgb_map[i].blue);
			palette32[i] = p;
			palette16[i] = p;
			palette8[i] = p;
		}
		video_stretch_palette_8(dst_x, dst_y, dst_dx, dst_dy, ptr, dx, dy, dw, dp, palette8, palette16, palette32, combine);
	} else if (bitmap->bytes_per_pixel == 3 || bitmap->bytes_per_pixel == 4) {
		adv_color_def def = png_color_def(bitmap->bytes_per_pixel);
		video_stretch(dst_x, dst_y, dst_dx, dst_dy, ptr, dx, dy, dw, dp, def, combine);
	}

	// end write
	video_write_unlock(dst_x, dst_y, dst_dx, dst_dy);

	bitmap_free(bitmap);
}

static void int_box_idle()
{
	static target_clock_t int_backdrop_box_last = 0;
	target_clock_t int_backdrop_box_new = target_clock();
	if (int_backdrop_box_new >= int_backdrop_box_last + TARGET_CLOCKS_PER_SEC/20) {
		int_backdrop_box_last = int_backdrop_box_new;
		video_backdrop_box();
	}
}

static void int_idle()
{
	if (int_idle_0_state && int_key_last == INT_KEY_IDLE_0 && int_idle_0_rep && time(0) - int_idle_time > int_idle_0_rep)
		int_key_saved = INT_KEY_IDLE_0;

	if (int_idle_1_state && int_key_last == INT_KEY_IDLE_1 && int_idle_1_rep && time(0) - int_idle_time > int_idle_1_rep)
		int_key_saved = INT_KEY_IDLE_1;

	if (int_idle_0_state && int_idle_0 && time(0) - int_idle_time > int_idle_0)
		int_key_saved = INT_KEY_IDLE_0;

	if (int_idle_1_state && int_idle_1 && time(0) - int_idle_time > int_idle_1)
		int_key_saved = INT_KEY_IDLE_1;

	if (int_key_saved == INT_KEY_NONE) {
		if (int_updating_active) {
			int_clip_idle();
			int_box_idle();
			target_idle();
		}
	}

	play_poll();
	keyb_poll();
	mouseb_poll();
	joystickb_poll();
}

int int_keypressed()
{
	static target_clock_t key_pressed_last_time = 0;

	if (int_key_saved != INT_KEY_NONE)
		return 1;

	target_clock_t now = target_clock();

	int_idle();

	/* don't check too fast */
	if (now - key_pressed_last_time >= TARGET_CLOCKS_PER_SEC / 25) {
		key_pressed_last_time = now;

		int_key_saved = key_poll();

		if (int_key_saved != INT_KEY_NONE)
			return 1;
	}

	return 0;
}

unsigned int_getkey(bool update_background)
{
	if (update_background)
		int_update();

	while (!int_keypressed()) { }

	int_idle_time = time(0);

	assert( int_key_saved != INT_KEY_NONE);

	int_key_last = int_key_saved;
	int_key_saved = INT_KEY_NONE;

	return int_key_last;
}

static void int_key_insert(unsigned event, unsigned* seq)
{
	unsigned i;
	for(i=0;KEYTAB[i].name && KEYTAB[i].event != event;++i);
	if (KEYTAB[i].name) {
		unsigned j;
		for(j=0;seq[j]!=OP_NONE;++j)
			KEYTAB[i].seq[j] = seq[j];
		KEYTAB[i].seq[j] = OP_NONE;
	}
}

static unsigned string2event(const string& s)
{
	unsigned i;
	for(i=0;KEYTAB[i].name && KEYTAB[i].name != s;++i);
	if (KEYTAB[i].name)
		return KEYTAB[i].event;
	else
		return INT_KEY_NONE;
}

bool int_key_in(const string& s)
{
	string sevent;
	unsigned seq[SEQ_MAX];
	unsigned seq_count;
	int pos = 0;

	sevent = arg_get(s, pos);

	unsigned event = string2event(sevent);
	if (event == INT_KEY_NONE)
		return false;

	seq_count = 0;
	while (pos < s.length()) {
		if (seq_count+1 >= SEQ_MAX)
			return false;

		string skey = arg_get(s, pos);
		if (skey == "or") {
			seq[seq_count++] = OP_OR;
		} else if (skey == "not") {
			seq[seq_count++] = OP_NOT;
		} else if (skey == "and") {
			/* nothing */
		} else {
			unsigned key = key_code(skey.c_str());
			if (key == KEYB_MAX)
				return false;
			seq[seq_count++] = key;
		}
	}

	seq[seq_count] = OP_NONE;

	if (!seq_valid(seq))
		return false;

	int_key_insert(event, seq);

	return true;
}

void int_key_out(adv_conf* config_context, const char* tag)
{
	for(unsigned i=0;KEYTAB[i].name;++i) {
		string s;
		s += KEYTAB[i].name;
		s += " ";
		for(unsigned j=0;KEYTAB[i].seq[j] != KEYB_MAX && j<SEQ_MAX;++j) {
			unsigned k = KEYTAB[i].seq[j];
			if (j != 0)
				s += " ";
			if (k == OP_OR)
				s += "or";
			else if (k == OP_NOT)
				s += "and";
			else {
				s += key_name(k);
			}
		}

		conf_set(config_context, "", tag, s.c_str());
	}
}

//---------------------------------------------------------------------------
// Color

int_color COLOR_HELP_NORMAL = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_HELP_TAG = { { 255, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_TITLE = { { 255, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_NORMAL = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_CHOICE_SELECT = { { 0, 0, 0 }, { 0, 255, 0 } };
int_color COLOR_MENU_NORMAL = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_HIDDEN = { { 128, 128, 128 }, { 255, 255, 255 } };
int_color COLOR_MENU_TAG = { { 255, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_SELECT = { { 0, 0, 0 }, { 0, 255, 0 } };
int_color COLOR_MENU_HIDDEN_SELECT = { { 128, 128, 128 }, { 0, 255, 0 } };
int_color COLOR_MENU_TAG_SELECT = { { 255, 0, 0 }, { 0, 255, 0 } };
int_color COLOR_MENU_BAR = { { 0, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_BAR_TAG = { { 255, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_BAR_HIDDEN = { { 128, 128, 128 }, { 255, 255, 255 } };
int_color COLOR_MENU_GRID = { { 255, 0, 0 }, { 255, 255, 255 } };
int_color COLOR_MENU_BACKDROP = { { 0, 0, 0 }, { 128, 128, 128 } };
int_color COLOR_MENU_ICON = { { 255, 255, 255 }, { 255, 255, 255 } };
int_color COLOR_MENU_CURSOR = { { 128, 128, 128 }, { 255, 255, 255 } };

static struct {
	int_color* var;
	const char* name;
} COLOR_TAB[] = {
{ &COLOR_HELP_NORMAL, "help" },
{ &COLOR_HELP_TAG, "help_tag" },
{ &COLOR_CHOICE_TITLE, "submenu_bar" },
{ &COLOR_CHOICE_NORMAL, "submenu_item" },
{ &COLOR_CHOICE_SELECT, "submenu_item_select" },
{ &COLOR_MENU_NORMAL, "menu_item" },
{ &COLOR_MENU_HIDDEN, "menu_hidden" },
{ &COLOR_MENU_TAG, "menu_tag" },
{ &COLOR_MENU_SELECT, "menu_item_select" },
{ &COLOR_MENU_HIDDEN_SELECT, "menu_hidden_select" },
{ &COLOR_MENU_TAG_SELECT, "menu_tag_select" },
{ &COLOR_MENU_BAR, "bar" },
{ &COLOR_MENU_BAR_TAG, "bar_tag" },
{ &COLOR_MENU_BAR_HIDDEN, "bar_hidden" },
{ &COLOR_MENU_GRID, "grid" },
{ &COLOR_MENU_BACKDROP, "backdrop" },
{ &COLOR_MENU_ICON, "icon" },
{ &COLOR_MENU_CURSOR, "cursor" },
{ 0, 0 }
};

static struct color_name {
	const char* name;
	int_rgb rgb;
}  COLOR_NAME[] = {
{ "black", { 0, 0, 0 } },
{ "blue", { 0, 0, 192 } },
{ "green", { 0, 192, 0 } },
{ "cyan", { 0, 192, 192 } },
{ "red", { 192, 0, 0 } },
{ "magenta", { 192, 0, 192 } },
{ "brown", { 192, 192, 0 } },
{ "lightgray", { 192, 192, 192 } },
{ "gray", { 128, 128, 128 } },
{ "lightblue", { 0, 0, 255 } },
{ "lightgreen", { 0, 255, 0 } },
{ "lightcyan", { 0, 255, 255 } },
{ "lightred", { 255, 0, 0 } },
{ "lightmagenta", { 255, 0, 255 } },
{ "yellow", { 255, 255, 0 } },
{ "white", { 255, 255, 255 } }
};

static unsigned hexdigit2int(char c)
{
	if (c>='A' && c<='F')
		return c - 'A' + 10;
	if (c>='a' && c<='f')
		return c - 'a' + 10;
	if (c>='0' && c<='9')
		return c - '0';
	return 0;
}

static unsigned hexnibble2int(char c0, char c1)
{
	return hexdigit2int(c0) * 16 + hexdigit2int(c1);
}

static int_rgb string2color(const string& s)
{
	for(unsigned i=0;i<16;++i)
		if (s == COLOR_NAME[i].name)
			return COLOR_NAME[i].rgb;

	if (s.length() == 6 && s.find_first_not_of("0123456789abcdefABCDEF") == string::npos) {
		int_rgb c;
		c.r = hexnibble2int(s[0], s[1]);
		c.g = hexnibble2int(s[2], s[3]);
		c.b = hexnibble2int(s[4], s[5]);
		return c;
	}

	return COLOR_NAME[0].rgb;
}

static string color2string(const int_rgb& c)
{
	for(unsigned i=0;i<16;++i)
		if (c.r == COLOR_NAME[i].rgb.r && c.g == COLOR_NAME[i].rgb.g && c.b == COLOR_NAME[i].rgb.b)
			return COLOR_NAME[i].name;

	ostringstream s;

	s << setfill('0') << setw(2) << hex << (unsigned)c.r << (unsigned)c.g << (unsigned)c.b;

	return s.str();
}

bool int_color_in(const string& s)
{
	string sname;
	string sarg0;
	string sarg1;
	unsigned i = 0;

	while (i < s.length() && !isspace(s[i])) {
		sname += s[i];
		++i;
	}
	
	while (i < s.length() && isspace(s[i]))
		++i;

	while (i < s.length() && !isspace(s[i])) {
		sarg0 += s[i];
		++i;
	}
	
	while (i < s.length() && isspace(s[i]))
		++i;

	while (i < s.length() && !isspace(s[i])) {
		sarg1 += s[i];
		++i;
	}

	while (i < s.length() && isspace(s[i]))
		++i;

	if (i != s.length())
		return false;

	for(i=0;COLOR_TAB[i].name;++i) {
		if (COLOR_TAB[i].name == sname)
			break;
	}

	if (!COLOR_TAB[i].name)
		return false;

	COLOR_TAB[i].var->foreground = string2color(sarg0);
	COLOR_TAB[i].var->background = string2color(sarg1);

	return true;
}

void int_color_out(adv_conf* config_context, const char* tag)
{
	for(unsigned i=0;COLOR_TAB[i].name;++i) {
		string s;
		s += COLOR_TAB[i].name;
		s += " ";
		s += color2string(COLOR_TAB[i].var->foreground);
		s += " ";
		s += color2string(COLOR_TAB[i].var->background);
		conf_set(config_context, "", tag, s.c_str());
	}
}
