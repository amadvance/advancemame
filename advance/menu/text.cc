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
 */

#include "bitmap.h"
#include "text.h"
#include "common.h"
#include "play.h"
#include "font.h"
#include "fontdef.h"
#include "png.h"
#include "mng.h"
#include "pcx.h"
#include "icon.h"
#include "video.h"
#include "generate.h"
#include "crtcbag.h"
#include "clear.h"
#include "blit.h"
#include "target.h"
#include "os.h"
#include "videoall.h"
#include "keyall.h"
#include "mouseall.h"
#include "joyall.h"

#include <list>
#include <iostream>
#include <iomanip>

#include <unistd.h>
#include <math.h>
#include <stdio.h>

using namespace std;

static string text_sound_event_key;

static bool text_alpha_mode;

// Orientation flag
static unsigned text_orientation = 0;

// Font (already orientation corrected)
static unsigned real_font_dx;
static unsigned real_font_dy;
static struct bitmapfont* real_font_map;

// -------------------------------------------------------------------------
// Size

__inline__ void swap(unsigned& a, unsigned& b) {
	unsigned t = a;
	a = b;
	b = t;
}

__inline__ void swap(int& a, int& b) {
	int t = a;
	a = b;
	b = t;
}

int text_dx_get() {
	if (text_orientation & ORIENTATION_FLIP_XY)
		return video_size_y();
	else
		return video_size_x();
}

int text_dy_get() {
	if (text_orientation & ORIENTATION_FLIP_XY)
		return video_size_x();
	else
		return video_size_y();
}

int text_font_dx_get() {
	if (text_orientation & ORIENTATION_FLIP_XY)
		return real_font_dy;
	else
		return real_font_dx;
}

int text_font_dy_get() {
	if (text_orientation & ORIENTATION_FLIP_XY)
		return real_font_dx;
	else
		return real_font_dy;
}

// -------------------------------------------------------------------------
// Backdrop

// Backdrop (already orientation corrected)
class backdrop_data {
	struct bitmap* map;
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
	const struct bitmap* bitmap_get() const { return map; }
	void bitmap_set(struct bitmap* Amap) { assert(!map); map = Amap; }

	unsigned target_dx_get() const { return target_dx; }
	unsigned target_dy_get() const { return target_dy; }
	unsigned aspectx_get() const { return aspectx; }
	unsigned aspecty_get() const { return aspecty; }

	// RGB palette for 8 bits video modes
	video_color rgb[256];
	unsigned rgb_max;
};

backdrop_data::backdrop_data(const resource& Ares, unsigned Atarget_dx, unsigned Atarget_dy, unsigned Aaspectx, unsigned Aaspecty)
	: res(Ares), target_dx(Atarget_dx), target_dy(Atarget_dy), aspectx(Aaspectx), aspecty(Aaspecty) {
	map = 0;
}

backdrop_data::~backdrop_data() {
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
static unsigned backdrop_missing_color;

// Color used for the lighting box backdrop
static unsigned backdrop_box_color;

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

static string text_cfg_file;

static bool text_updating_active; // is updating the video

// -----------------------------------------------------------------------
// Joystick

static void text_joystick_init(struct conf_context* config_context) {
	joystickb_reg(config_context, 0);
	joystickb_reg_driver_all(config_context);
}

static void text_joystick_done() {
}

static bool text_joystick_load(struct conf_context* config_context) {
	if (joystickb_load(config_context) != 0)
		return false;
	return true;
}

static bool text_joystick_init2() {
	if (joystickb_init() != 0)
		return false;
	return true;
}

static void text_joystick_done2() {
	joystickb_done();
}

static int text_joystick_button_raw_poll() {
	for(int i=0;i<joystickb_count_get();++i) {
		for(int j=0;j<joystickb_button_count_get(i);++j) {
			if (joystickb_button_get(i,j)) {
				switch (j) {
					case 0 : return TEXT_KEY_ENTER;
					case 1 : return TEXT_KEY_ESC;
					case 2 : return TEXT_KEY_MENU;
				}
			}
		}
	}

	return TEXT_KEY_NONE;
}

static int text_joystick_move_raw_poll() {

	for(int i=0;i<joystickb_count_get();++i) {
		for(int j=0;j<joystickb_stick_count_get(i);++j) {
			if (joystickb_stick_axe_count_get(i,j) > 0) {
				if (joystickb_stick_axe_digital_get(i,j,0,0))
					return TEXT_KEY_RIGHT;
				if (joystickb_stick_axe_digital_get(i,j,0,1))
					return TEXT_KEY_LEFT;
			}
			if (joystickb_stick_axe_count_get(i,j) > 1) {
				if (joystickb_stick_axe_digital_get(i,j,1,0))
					return TEXT_KEY_DOWN;
				if (joystickb_stick_axe_digital_get(i,j,1,1))
					return TEXT_KEY_UP;
			}
		}
	}

	return TEXT_KEY_NONE;
}

// -----------------------------------------------------------------------
// Key

static void text_key_init(struct conf_context* config_context) {
	keyb_reg(config_context, 1);
	keyb_reg_driver_all(config_context);
}

static void text_key_done() {
}

static bool text_key_load(struct conf_context* config_context) {
	if (keyb_load(config_context) != 0)
		return false;

	return true;
}

static bool text_key_init2() {
	if (keyb_init(0) != 0)
		return false;

	return true;
}

static void text_key_done2() {
	keyb_done();
}

// -----------------------------------------------------------------------
// Mouse

static int text_mouse_delta;
static int text_mouse_pos_x;
static int text_mouse_pos_y;

static void text_mouse_init(struct conf_context* config_context) {
	mouseb_reg(config_context, 0);
	mouseb_reg_driver_all(config_context);
	conf_int_register_limit_default(config_context, "mouse_delta", 1, 1000, 100);
}

static void text_mouse_done() {
}

static bool text_mouse_load(struct conf_context* config_context) {
	text_mouse_pos_x = 0;
	text_mouse_pos_y = 0;
	text_mouse_delta = conf_int_get_default(config_context, "mouse_delta");

	if (mouseb_load(config_context) != 0)
		return false;

	return true;
}

static bool text_mouse_init2() {
	if (mouseb_init() != 0)
		return false;

	return true;
}

static void text_mouse_done2() {
	mouseb_done();
}

static int text_mouse_button_raw_poll() {
	for(int i=0;i<mouseb_count_get();++i) {
		if (mouseb_button_count_get(i) > 0 && mouseb_button_get(i,0))
			return TEXT_KEY_ENTER;

		if (mouseb_button_count_get(i) > 1 && mouseb_button_get(i,1))
			return TEXT_KEY_ESC;

		if (mouseb_button_count_get(i) > 2 && mouseb_button_get(i,2))
			return TEXT_KEY_MENU;
	}

	return TEXT_KEY_NONE;
}

static int text_mouse_move_raw_poll() {
	for(int i=0;i<mouseb_count_get();++i) {
		int x, y;

		mouseb_pos_get(i,&x,&y);

		text_mouse_pos_x += x;
		text_mouse_pos_y += y;
	}

	if (text_mouse_pos_x >= text_mouse_delta) {
		text_mouse_pos_x -= text_mouse_delta;
		return TEXT_KEY_RIGHT;
	}

	if (text_mouse_pos_x <= -text_mouse_delta) {
		text_mouse_pos_x += text_mouse_delta;
		return TEXT_KEY_LEFT;
	}

	if (text_mouse_pos_y >= text_mouse_delta) {
		text_mouse_pos_y -= text_mouse_delta;
		return TEXT_KEY_DOWN;
	}

	if (text_mouse_pos_y <= -text_mouse_delta) {
		text_mouse_pos_y += text_mouse_delta;
		return TEXT_KEY_UP;
	}

	return TEXT_KEY_NONE;
}

// -------------------------------------------------------------------------
// Video mode choice

#define DEFAULT_GRAPH_MODE "default_graph" // default video mode

static unsigned text_mode_size;
static unsigned text_mode_depth;
static video_mode text_current_mode; // selected video mode
static video_monitor text_monitor; // monitor info
static video_generate_interpolate_set text_interpolate;
static video_crtc_container text_modelines;
static bool text_has_clock = false;
static bool text_has_generate = false;

// comparing for graphics mode
bool mode_graphics_less(const video_mode* A, const video_mode* B) {
	int areaA = A->size_x * A->size_y;
	int areaB = B->size_x * B->size_y;

	int difA = abs( areaA - static_cast<int>(text_mode_size*text_mode_size*3/4) );
	int difB = abs( areaB - static_cast<int>(text_mode_size*text_mode_size*3/4) );

	return difA < difB;
}

static bool text_mode_find(bool& mode_found, unsigned depth, video_crtc_container& modelines) {
	video_crtc_container_iterator i;
	video_error err;

	// search the default name
	for(video_crtc_container_iterator_begin(&i,&modelines);!video_crtc_container_iterator_is_end(&i);video_crtc_container_iterator_next(&i)) {
		const video_crtc* crtc = video_crtc_container_iterator_get(&i);
		if (strcmp(crtc->name,DEFAULT_GRAPH_MODE)==0) {

			// check the clocks only if the driver is programmable
			if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0) {
				if (!crtc_clock_check(&text_monitor,crtc)) {
					target_err("The selected mode '%s' is out of your monitor capabilities.\n", DEFAULT_GRAPH_MODE);
					return false;
				}
			}

			if (video_mode_generate(&text_current_mode,crtc,depth,VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)!=0) {
				target_err("The selected mode '%s' is out of your video board capabilities.\n", DEFAULT_GRAPH_MODE);
				return false;
			}

			mode_found = true;
			return true;
		}
	}

	// generate an exact mode with clock
	if (text_has_generate) {
		video_crtc crtc;
		err = generate_find_interpolate(&crtc, text_mode_size, text_mode_size*3/4, 70, &text_monitor, &text_interpolate, video_mode_generate_driver_flags(), GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK);
		if (err == 0) {
			if (crtc_clock_check(&text_monitor,&crtc)) {
				video_mode mode;
				if (video_mode_generate(&mode,&crtc,depth,VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)==0) {
					text_current_mode = mode;
					mode_found = true;
					video_log("text: generating a perfect mode from the format option.\n");
					return true;
				}
			}
		}
	}

	// generate any resolution for a window manager
	if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_WINDOWMANAGER)!=0) {
		video_crtc crtc;
		crtc_fake_set(&crtc, text_mode_size, text_mode_size*3/4);

		video_mode mode;
		if (video_mode_generate(&mode,&crtc,depth,VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)==0) {
			text_current_mode = mode;
			mode_found = true;
			video_log("text: generating a perfect mode for the window manager.\n");
			return true;
		}
	}

	// search the best on the list
	for(video_crtc_container_iterator_begin(&i,&modelines);!video_crtc_container_iterator_is_end(&i);video_crtc_container_iterator_next(&i)) {
		const video_crtc* crtc = video_crtc_container_iterator_get(&i);

		// check the clocks only if the driver is programmable
		if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)!=0) {
			if (!crtc_clock_check(&text_monitor,crtc)) {
				continue;
			}
		}

		video_mode mode;
		if (video_mode_generate(&mode, crtc, depth, VIDEO_FLAGS_TYPE_GRAPHICS | VIDEO_FLAGS_INDEX_RGB)==0) {
			if (!mode_found || mode_graphics_less(&mode, &text_current_mode)) {
				text_current_mode = mode;
				mode_found = true;
			}
		}
	}

	return true;
}

// -------------------------------------------------------------------------
// text

static double text_gamma;
static double text_brightness;

static int text_key_saved = TEXT_KEY_NONE;
static int text_key_last = TEXT_KEY_NONE;

static unsigned text_idle_0; // seconds before the first 0 event
static unsigned text_idle_0_rep; // seconds before the second 0 event
static unsigned text_idle_1; // seconds before the first 1 event
static unsigned text_idle_1_rep; // seconds before the second 1 event
static unsigned text_repeat; // milli seconds before the first key repeat event
static unsigned text_repeat_rep; // milli seconds before the second key repeat event

static bool text_wait_for_backdrop; // wait for backdrop completion

static time_t text_idle_time;

static unsigned video_buffer_size;
static unsigned video_buffer_line_size;
static unsigned video_buffer_pixel_size;
static unsigned char* video_buffer;

static void palette_set(video_color* pal, unsigned pal_max, unsigned* conv);

void text_init(struct conf_context* config_context) {
	text_mouse_init(config_context);
	text_joystick_init(config_context);
	text_key_init(config_context);
	generate_interpolate_register(config_context);
	monitor_register(config_context);

	video_reg(config_context, 1);
	video_reg_driver_all(config_context);

	video_crtc_container_register(config_context);

	video_crtc_container_init(&text_modelines);
}

bool text_load(struct conf_context* config_context) {
	video_error err;

	if (!text_joystick_load(config_context))
		return false;
	if (!text_mouse_load(config_context))
		return false;
	if (!text_key_load(config_context))
		return false;

	err = generate_interpolate_load(config_context, &text_interpolate);
	if (err<0) {
		target_err("%s\n", video_error_description_get());
		return false;
	}
	if (err==0) {
		text_has_generate = true;
	} else {
		text_has_generate = false;
		video_log("text: format option not found.\n");
	}

	err = monitor_load(config_context,&text_monitor);
	if (err<0) {
		target_err("%s\n", video_error_description_get());
		return false;
	}
	if (err==0) {
		text_has_clock = true;
	} else {
		text_has_clock = false;
		monitor_parse(&text_monitor, "10 - 80", "30.5 - 60", "55 - 90");
		video_log("text: clock options not found. Use default SVGA monitor clocks.\n");
	}

	err = video_load(config_context, "");
	if (err != 0) {
		target_err("%s\n", video_error_description_get());
		return false;
	}

	err = video_crtc_container_load(config_context,&text_modelines);
	if (err!=0) {
		target_err("%s\n", video_error_description_get());
		return false;
	}

	return true;
}

void text_done(void) {
	video_crtc_container_done(&text_modelines);
	text_mouse_done();
	text_joystick_done();
	text_key_done();
}

bool text_init2(unsigned size, unsigned depth, const string& sound_event_key) {
	bool mode_found = false;

	text_mode_size = size;
	text_mode_depth = depth;
	text_sound_event_key = sound_event_key;

	if (video_init() != 0) {
		target_err("Error initializing the video driver.\n");
		goto out;
	}

	if (video_blit_init() != 0) {
		target_err("Error initializing the blit driver.\n");
		goto out_video;
	}

	// disable generate if the clocks are not available
	if (!text_has_clock)
		text_has_generate = false;

	// disable generate if the driver is not programmable
	if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK)==0)
		text_has_generate = false;

	// add modes if the list is empty and no generation is possibile
	if (!text_has_generate
		&& video_crtc_container_is_empty(&text_modelines)) {
		if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) != 0) {
			video_crtc_container_insert_default_modeline_svga(&text_modelines);
			video_crtc_container_insert_default_modeline_vga(&text_modelines);
		} else {
			video_crtc_container_insert_default_system(&text_modelines);
		}
	}

	if (!text_mode_find(mode_found,text_mode_depth,text_modelines))
		goto out_blit;

	// if no mode found retry with a different bit depth
	if (!mode_found) {
		unsigned bits_per_pixel;

		// check if the video driver has a default bit depth
		if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_8BIT) != 0) {
			bits_per_pixel = 8;
		} else if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_15BIT) != 0) {
			bits_per_pixel = 15;
		} else if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_16BIT) != 0) {
			bits_per_pixel = 16;
		} else if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_DEFAULTDEPTH_32BIT) != 0) {
			bits_per_pixel = 32;
		} else {
			bits_per_pixel = 8; // as default uses the 8 bit depth
		}

		// retry only if different
		if (bits_per_pixel != text_mode_depth) {
			text_mode_depth = bits_per_pixel;
			if (!text_mode_find(mode_found,text_mode_depth,text_modelines))
				goto out_blit;
		}
	}

	if (!mode_found) {
		target_err("No video modes available for your current configuration.\n");
		goto out_blit;
	}

	return true;

out_blit:
	video_blit_done();
out_video:
	video_done();
out:
	return false;
}

void text_done2() {
	video_blit_done();
	video_done();
}

bool text_init3(double gamma, double brightness, unsigned idle_0, unsigned idle_0_rep,unsigned idle_1, unsigned idle_1_rep, unsigned repeat, unsigned repeat_rep, bool backdrop_fast, bool alpha_mode) {

	text_alpha_mode = alpha_mode;

	text_idle_time = time(0);
	text_idle_0 = idle_0;
	text_idle_1 = idle_1;
	text_idle_0_rep = idle_0_rep;
	text_idle_1_rep = idle_1_rep;
	text_repeat = repeat;
	text_repeat_rep = repeat_rep;
	text_wait_for_backdrop = !backdrop_fast;
	if (gamma < 0.1) gamma = 0.1;
	if (gamma > 10) gamma = 10;
	text_gamma = 1.0 / gamma;
	text_brightness = brightness;

	if (video_mode_set(&text_current_mode) != 0) {
		video_mode_reset();
		target_err("Error initializing the video driver.\n");
		goto out;
	}

	if (!text_key_init2()) {
		video_mode_reset();
		target_err("Error initializing the keyboard driver.\n");
		goto out;
	}

	if (!text_joystick_init2()) {
		video_mode_reset();
		target_err("Error initializing the joystick driver.\n");
		target_err("Try with the option '-device_joystick none'.\n");
		goto out_key;
	}

	if (!text_mouse_init2()) {
		video_mode_reset();
		target_err("Error initializing the mouse driver.\n");
		target_err("Try with the option '-device_mouse none'.\n");
		goto out_joy;
	}

	if (video_index() == VIDEO_FLAGS_INDEX_PACKED)
		palette_set(0,0,0);
		
	return true;

out_joy:
	text_joystick_done();
out_key:
	text_key_done();
out:
	return false;
}

void text_done3(bool reset_video_mode) {

	text_mouse_done2();
	text_joystick_done2();
	text_key_done2();

	if (reset_video_mode) {
		if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_WINDOWMANAGER)==0) {
			video_write_lock();
			video_clear(0,0,video_size_x(),video_size_y(),0);
			video_write_unlock(0,0,video_size_x(),video_size_y());
		}
		video_mode_reset();
	} else {
		video_mode_done(0);
	}

}

bool text_init4(const string& font, unsigned orientation) {
	text_orientation = orientation;

	// load the font
	real_font_map = 0;
	if (font != "none") {
		real_font_map = bitmapfont_load(cpath_export(font));
	}
	if (!real_font_map)
		real_font_map = font_default(video_size_y() / 25);

	// set the orientation
	bitmapfont_orientation(real_font_map,text_orientation);

	// compute font size
	real_font_dx = bitmapfont_size_x(real_font_map);
	real_font_dy = bitmapfont_size_y(real_font_map);

	video_buffer_pixel_size = video_bytes_per_pixel();
	video_buffer_line_size = video_size_x() * video_bytes_per_pixel();
	video_buffer_size = video_size_y() * video_buffer_line_size;
	video_buffer = (unsigned char*)operator new(video_buffer_size);

	text_clear();

	text_updating_active = false;

	return true;
}

void text_done4() {
	bitmapfont_free(real_font_map);
	operator delete(video_buffer);
}

//---------------------------------------------------------------------------
// Palette

#define PALETTE_RESERVED_MAX 16

static video_color PALETTE_RESERVED[PALETTE_RESERVED_MAX] = {
{   0,  0,  0,0 },
{ 192,  0,  0,0 },
{   0,192,  0,0 },
{ 192,192,  0,0 },
{   0,  0,192,0 },
{ 192,  0,192,0 },
{   0,192,192,0 },
{ 192,192,192,0 },
{ 128,128,128,0 },
{ 255,  0,  0,0 },
{   0,255,  0,0 },
{ 255,255,  0,0 },
{   0,  0,255,0 },
{ 255,  0,255,0 },
{   0,255,255,0 },
{ 255,255,255,0 }
};

static video_color text_palette[256];

int palette_dist(const video_color& A, const video_color& B) {
	int r = (int)A.red - B.red;
	int g = (int)A.green - B.green;
	int b = (int)A.blue - B.blue;
	return r*r + g*g + b*b;
}

static void palette_set(video_color* pal, unsigned pal_max, unsigned* conv) {
	unsigned i;
	for(i=0;i<PALETTE_RESERVED_MAX;++i)
		text_palette[i] = PALETTE_RESERVED[i];

	for(i=0;i<256-PALETTE_RESERVED_MAX && i<pal_max;++i) {
		text_palette[i+PALETTE_RESERVED_MAX] = pal[i];
		conv[i] = i+PALETTE_RESERVED_MAX;
	}

	// search for best colors
	while (i<pal_max) {
		unsigned best = PALETTE_RESERVED_MAX;
		int dist = palette_dist(pal[i],text_palette[best]);
		for(unsigned j=best+1;j<256;++j) {
			int ndist = palette_dist(pal[i],text_palette[j]);
			if (ndist < dist) {
				dist = ndist;
				best = j;
			}
		}
		conv[i] = best;
		++i;
	}

	for(i=0;i<256;++i) {
		double f = 255.0 * text_brightness;
		text_palette[i].red = (uint8)(f * pow(text_palette[i].red / 255.0, text_gamma));
		text_palette[i].green = (uint8)(f * pow(text_palette[i].green / 255.0, text_gamma));
		text_palette[i].blue = (uint8)(f * pow(text_palette[i].blue / 255.0, text_gamma));
	}
}

// Conversion from text color to video color
static inline video_rgb index2color(unsigned color) {
	if (video_index() == VIDEO_FLAGS_INDEX_PACKED) {
		return color & 0xF;
	} else {
		video_rgb r;
		color &= 0xF;
		video_rgb_make(&r, PALETTE_RESERVED[color].red, PALETTE_RESERVED[color].green, PALETTE_RESERVED[color].blue);
		return r;
	}
}

//---------------------------------------------------------------------------
// Generic put

static void gen_clear_raw8rgb(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned attrib) {
	unsigned color = index2color(attrib);
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned char* dst_ptr = buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = color;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw8packed(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned attrib) {
	unsigned color = index2color(attrib);
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned char* dst_ptr = buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = color;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw32rgb(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned attrib) {
	video_rgb color = index2color(attrib);
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned* dst_ptr = (unsigned*)buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = color;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw16rgb(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, unsigned x, unsigned y, unsigned dx, unsigned dy, unsigned attrib) {
	video_rgb color = index2color(attrib);
	unsigned char* buffer = ptr + x * ptr_p + y * ptr_d;
	for(unsigned cy=0;cy<dy;++cy) {
		unsigned short* dst_ptr = (unsigned short*)buffer;
		for(unsigned cx=0;cx<dx;++cx) {
			*dst_ptr = color;
			dst_ptr += 1;
		}
		buffer += ptr_d;
	}
}

static void gen_clear_raw(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, int x, int y, int dx, int dy, int color) {
	assert( x>=0 && y>=0 && x+dx<=video_size_x() &&  y+dy<=video_size_y() );

	switch (video_bytes_per_pixel()) {
		case 1 :
			if (video_index() == VIDEO_FLAGS_INDEX_PACKED)
				gen_clear_raw8packed(ptr, ptr_p, ptr_d, x,y,dx,dy,color);
			else
				gen_clear_raw8rgb(ptr, ptr_p, ptr_d, x,y,dx,dy,color);
			break;
		case 2 :
			gen_clear_raw16rgb(ptr, ptr_p, ptr_d, x,y,dx,dy,color);
			break;
		case 4 :
			gen_clear_raw32rgb(ptr, ptr_p, ptr_d, x,y,dx,dy,color);
			break;
	}
}

static void gen_backdrop_raw8(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, struct backdrop_t* back, const bitmap* map) {
	unsigned x0 = (back->pos.real_dx - map->size_x) / 2;
	unsigned x1 = back->pos.real_dx -  map->size_x - x0;
	unsigned y0 = (back->pos.real_dy - map->size_y) / 2;
	unsigned y1 = back->pos.real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y,x0,back->pos.real_dy,backdrop_missing_color >> 4);
	if (x1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x + back->pos.real_dx - x1,back->pos.real_y,x1,back->pos.real_dy,backdrop_missing_color >> 4);
	if (y0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y,back->pos.real_dx,y0,backdrop_missing_color >> 4);
	if (y1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y + back->pos.real_dy - y1,back->pos.real_dx,y1,backdrop_missing_color >> 4);
	unsigned char* buffer = ptr + (back->pos.real_y + y0) * ptr_d + (back->pos.real_x + x0) * ptr_p;
	for(unsigned cy=0;cy<map->size_y;++cy) {
		memcpy(buffer,bitmap_line(const_cast<bitmap*>(map),cy),map->size_x);
		buffer += ptr_d;
	}
}

static void gen_backdrop_raw16(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, struct backdrop_t* back, const bitmap* map) {
	unsigned x0 = (back->pos.real_dx - map->size_x) / 2;
	unsigned x1 = back->pos.real_dx -  map->size_x - x0;
	unsigned y0 = (back->pos.real_dy - map->size_y) / 2;
	unsigned y1 = back->pos.real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y,x0,back->pos.real_dy,backdrop_missing_color >> 4);
	if (x1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x + back->pos.real_dx - x1,back->pos.real_y,x1,back->pos.real_dy,backdrop_missing_color >> 4);
	if (y0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y,back->pos.real_dx,y0,backdrop_missing_color >> 4);
	if (y1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y + back->pos.real_dy - y1,back->pos.real_dx,y1,backdrop_missing_color >> 4);
	unsigned char* buffer = ptr + (back->pos.real_y + y0) * ptr_d + (back->pos.real_x + x0) * ptr_p;
	for(unsigned cy=0;cy<map->size_y;++cy) {
		memcpy(buffer,bitmap_line(const_cast<bitmap*>(map),cy),map->size_x * 2);
		buffer += ptr_d;
	}
}

static void gen_backdrop_raw32(unsigned char* ptr, unsigned ptr_p, unsigned ptr_d, struct backdrop_t* back, const bitmap* map) {
	unsigned x0 = (back->pos.real_dx - map->size_x) / 2;
	unsigned x1 = back->pos.real_dx -  map->size_x - x0;
	unsigned y0 = (back->pos.real_dy - map->size_y) / 2;
	unsigned y1 = back->pos.real_dy -  map->size_y - y0;
	if (x0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y,x0,back->pos.real_dy,backdrop_missing_color >> 4);
	if (x1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x + back->pos.real_dx - x1,back->pos.real_y,x1,back->pos.real_dy,backdrop_missing_color >> 4);
	if (y0)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y,back->pos.real_dx,y0,backdrop_missing_color >> 4);
	if (y1)
		gen_clear_raw(ptr, ptr_p, ptr_d, back->pos.real_x,back->pos.real_y + back->pos.real_dy - y1,back->pos.real_dx,y1,backdrop_missing_color >> 4);
	unsigned char* buffer = ptr + (back->pos.real_y + y0) * ptr_d + (back->pos.real_x + x0) * ptr_p;
	for(unsigned cy=0;cy<map->size_y;++cy) {
		memcpy(buffer,bitmap_line(const_cast<bitmap*>(map),cy),map->size_x * 4);
		buffer += ptr_d;
	}
}

//---------------------------------------------------------------------------
// Video put

static void video_box(int x, int y, int dx, int dy, int width, int color) {
	video_clear(x,y,dx,width,color);
	video_clear(x,y+dy-width,dx,width,color);
	video_clear(x,y+width,width,dy-2*width,color);
	video_clear(x+dx-width,y+width,width,dy-2*width,color);
}

static void video_backdrop_box() {
	for(int i=0;i<backdrop_mac;++i) {
		struct backdrop_t* back = backdrop_map + i;
		if (back->pos.highlight && backdrop_cursor) {
			unsigned color = index2color(backdrop_box_color);
			unsigned x = back->pos.real_x - backdrop_outline;
			unsigned y = back->pos.real_y - backdrop_outline;
			unsigned dx = back->pos.real_dx + 2*backdrop_outline;
			unsigned dy = back->pos.real_dy + 2*backdrop_outline;

			video_write_lock();
			video_box(x,y,dx,dy,backdrop_cursor,color);
			video_write_unlock(x, y, dx, dy);

			backdrop_box_color = ((backdrop_box_color << 4) & 0xF0) | ((backdrop_box_color >> 4) & 0xF);
		}
	}
}

//---------------------------------------------------------------------------
// Text put

static void text_clear_raw(int x, int y, int dx, int dy, int color) {
	gen_clear_raw(video_buffer, video_buffer_pixel_size, video_buffer_line_size, x, y, dx, dy, color);
}

static void text_backdrop_raw(struct backdrop_t* back, const bitmap* map) {
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

static void text_put8rgb_char_font(unsigned x, unsigned y, unsigned bitmap, unsigned attrib) {
	struct bitmap* src = real_font_map->data[bitmap];
	unsigned color_fore = index2color(attrib);
	unsigned color_back = index2color(attrib >> 4);
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src,cy);
		unsigned char* dst_ptr = buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? color_fore : color_back;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

static void text_put8packed_char_font(unsigned x, unsigned y, unsigned bitmap, unsigned attrib) {
	struct bitmap* src = real_font_map->data[bitmap];
	unsigned color_fore = index2color(attrib);
	unsigned color_back = index2color(attrib >> 4);
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src,cy);
		unsigned char* dst_ptr = buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? color_fore : color_back;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

static void text_put16rgb_char_font(unsigned x, unsigned y, unsigned bitmap, unsigned attrib) {
	struct bitmap* src = real_font_map->data[bitmap];
	video_rgb color_fore = index2color(attrib);
	video_rgb color_back = index2color(attrib >> 4);
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src,cy);
		unsigned short* dst_ptr = (unsigned short*)buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? color_fore : color_back;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

static void text_put32rgb_char_font(unsigned x, unsigned y, unsigned bitmap, unsigned attrib) {
	struct bitmap* src = real_font_map->data[bitmap];
	video_rgb color_fore = index2color(attrib);
	video_rgb color_back = index2color(attrib >> 4);
	unsigned char* buffer = video_buffer + x * video_buffer_pixel_size + y * video_buffer_line_size;
	for(unsigned cy=0;cy<src->size_y;++cy) {
		unsigned char* src_ptr = bitmap_line(src,cy);
		unsigned* dst_ptr = (unsigned*)buffer;
		for(unsigned cx=0;cx<src->size_x;++cx) {
			unsigned color = *src_ptr ? color_fore : color_back;
			*dst_ptr = color;
			dst_ptr += 1;
			src_ptr += 1;
		}
		buffer += video_buffer_line_size;
	}
}

unsigned text_put_width(char c) {
	struct bitmap* src = real_font_map->data[(unsigned char)c];
	if (text_orientation & ORIENTATION_FLIP_XY)
		return src->size_y;
	else
		return src->size_x;
}

void text_put(int x, int y, char c, int color) {
	if (x>=0 && y>=0 && x+text_put_width(c)<=text_dx_get() && y+text_font_dy_get()<=text_dy_get()) {
		struct bitmap* src = real_font_map->data[(unsigned char)c];
		if (text_orientation & ORIENTATION_FLIP_XY)
			swap(x,y);
		if (text_orientation & ORIENTATION_MIRROR_X)
			x = video_size_x() - src->size_x - x;
		if (text_orientation & ORIENTATION_MIRROR_Y)
			y = video_size_y() - src->size_y - y;

		assert( x>=0 && y>=0 && x+src->size_x<=video_size_x() &&  y+src->size_y<=video_size_y() );

		switch (video_bytes_per_pixel()) {
			case 1 :
				if (video_index() == VIDEO_FLAGS_INDEX_PACKED)
					text_put8packed_char_font(x,y,(unsigned char)c,color);
				else
					text_put8rgb_char_font(x,y,(unsigned char)c,color);
				break;
			case 2 :
				text_put16rgb_char_font(x,y,(unsigned char)c,color);
				break;
			case 4 :
				text_put32rgb_char_font(x,y,(unsigned char)c,color);
				break;
		}
	}
}

void text_put_filled(int x, int y, int dx, const string& s, int color) {
	for(unsigned i=0;i<s.length();++i) {
		if (text_put_width(s[i]) <= dx) {
			text_put(x,y,s[i],color);
			x += text_put_width(s[i]);
			dx -= text_put_width(s[i]);
		} else
			break;
	}
	if (dx)
		text_clear(x,y,dx,text_font_dy_get(),color >> 4);
}

void text_put_special(bool& in, int x, int y, int dx, const string& s, int c0, int c1, int c2) {
	for(unsigned i=0;i<s.length();++i) {
		if (text_put_width(s[i]) <= dx) {
			if (s[i]=='(' || s[i]=='[')
				in = true;
			if (!in && i==0) {
				text_put(x,y,s[i],c0);
			} else {
				text_put(x,y,s[i], in ? c1 : c2);
			}
			x += text_put_width(s[i]);
			dx -= text_put_width(s[i]);
			if (s[i]==')' || s[i]==']')
				in = false;
		} else
			break;
	}
	if (dx)
		text_clear(x,y,dx,text_font_dy_get(),c0 >> 4);
}

static void text_clear_noclip(int x, int y, int dx, int dy, int color) {
	if (text_orientation & ORIENTATION_FLIP_XY) {
		swap(x,y);
		swap(dx,dy);
	}
	if (text_orientation & ORIENTATION_MIRROR_X)
		x = video_size_x() - dx - x;
	if (text_orientation & ORIENTATION_MIRROR_Y)
		y = video_size_y() - dy - y;

	text_clear_raw(x, y, dx, dy, color);
}

void text_clear(int x, int y, int dx, int dy, int color) {
	if (x < 0) {
		dx += x;
		x = 0;
	}
	if (y < 0) {
		dy += y;
		y = 0;
	}
	if (x + dx > text_dx_get()) {
		dx = text_dx_get() - x;
	}
	if (y + dy > text_dy_get()) {
		dy = text_dy_get() - y;
	}
	if (dx > 0 && dy > 0)
		text_clear_noclip(x,y,dx,dy,color);
}

void text_clear() {
	// clear the bitmap
	if ((video_mode_generate_driver_flags() & VIDEO_DRIVER_FLAGS_INFO_WINDOWMANAGER)!=0
		&& video_buffer_pixel_size > 1) {
		memset(video_buffer,0xFF,video_buffer_size);
	} else {
		memset(video_buffer,0x0,video_buffer_size);
	}
}

void text_box(int x, int y, int dx, int dy, int width, int color) {
	text_clear(x,y,dx,width,color);
	text_clear(x,y+dy-width,dx,width,color);
	text_clear(x,y+width,width,dy-2*width,color);
	text_clear(x+dx-width,y+width,width,dy-2*width,color);
}

void text_put(int x, int y, const string& s, int color) {
	for(unsigned i=0;i<s.length();++i) {
		text_put(x,y,s[i],color);
		x += text_put_width(s[i]);
	}
}

unsigned text_put(int x, int y, int dx, const string& s, int color) {
	for(unsigned i=0;i<s.length();++i) {
		int width = text_put_width(s[i]);
		if (width > dx)
			return i;
		text_put(x,y,s[i],color);
		x += width;
		dx -= width;
	}
	return s.length();
}

unsigned text_put_width(const string& s) {
	unsigned size = 0;
	for(unsigned i=0;i<s.length();++i)
		size += text_put_width(s[i]);
	return size;
}

//---------------------------------------------------------------------------
// Backdrop

// Reduce the size of the cache
static void text_backdrop_cache_reduce() {
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
static void text_backdrop_cache(backdrop_data* data) {
	if (data) {
		if (data->has_bitmap()) {
			// insert the image in the cache
			backdrop_cache_list->insert(backdrop_cache_list->begin(),data);
		} else {
			delete data;
		}
	}
}

static backdrop_data* text_backdrop_cache_find(const resource& res, unsigned dx, unsigned dy, unsigned aspectx, unsigned aspecty) {
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

	return new backdrop_data(res,dx,dy,aspectx,aspecty);
}

static void text_backdrop_cache_all() {
	for(unsigned i=0;i<backdrop_mac;++i) {
		text_backdrop_cache(backdrop_map[i].data);
		backdrop_map[i].data = 0;
	}
}

// Select the backdrop
void text_backdrop_set(int back_index, const resource& res, bool highlight, unsigned aspectx, unsigned aspecty) {
	assert( back_index < backdrop_mac );

	text_backdrop_cache(backdrop_map[back_index].data);

	backdrop_map[back_index].data = text_backdrop_cache_find(res,backdrop_map[back_index].pos.dx,backdrop_map[back_index].pos.dy,aspectx,aspecty);

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
void text_backdrop_clear(int back_index, bool highlight) {
	assert( back_index < backdrop_mac );

	text_backdrop_cache(backdrop_map[back_index].data);

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

void text_backdrop_done() {
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

void text_backdrop_init(unsigned Abackdrop_missing_color, unsigned Abackdrop_box_color, unsigned Amac, unsigned outline, unsigned cursor, double expand_factor) {
	assert(backdrop_mac == 0);

	backdrop_cache_list = new pbackdrop_list;
	backdrop_missing_color = Abackdrop_missing_color;
	backdrop_box_color = Abackdrop_box_color;
	backdrop_outline = outline;
	backdrop_cursor = cursor;
	backdrop_expand_factor = expand_factor;

	backdrop_mac = Amac;
	backdrop_cache_max = backdrop_mac*2 + 1;

	text_updating_active = false;

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
void text_backdrop_pos(int back_index, int x, int y, int dx, int dy) {
	assert( back_index < backdrop_mac );

	text_backdrop_cache(backdrop_map[back_index].data);
	backdrop_map[back_index].data = 0;

	backdrop_map[back_index].last = resource();
	backdrop_map[back_index].pos.highlight = false;
	backdrop_map[back_index].pos.redraw = true;

	backdrop_map[back_index].pos.x = backdrop_map[back_index].pos.real_x = x + backdrop_outline;
	backdrop_map[back_index].pos.y = backdrop_map[back_index].pos.real_y = y + backdrop_outline;
	backdrop_map[back_index].pos.dx = backdrop_map[back_index].pos.real_dx = dx - 2*backdrop_outline;
	backdrop_map[back_index].pos.dy = backdrop_map[back_index].pos.real_dy = dy - 2*backdrop_outline;

	if (text_orientation & ORIENTATION_FLIP_XY) {
		swap(backdrop_map[back_index].pos.real_x,backdrop_map[back_index].pos.real_y);
		swap(backdrop_map[back_index].pos.real_dx,backdrop_map[back_index].pos.real_dy);
	}
	if (text_orientation & ORIENTATION_MIRROR_X) {
		backdrop_map[back_index].pos.real_x = video_size_x() - backdrop_map[back_index].pos.real_x - backdrop_map[back_index].pos.real_dx;
	}
	if (text_orientation & ORIENTATION_MIRROR_Y) {
		backdrop_map[back_index].pos.real_y = video_size_y() - backdrop_map[back_index].pos.real_y - backdrop_map[back_index].pos.real_dy;
	}
}

//---------------------------------------------------------------------------
// Clip

static bool text_clip_active;
static bool text_clip_running;
static resource text_clip_res;
static int text_clip_index;
static FZ* text_clip_f;
static unsigned text_clip_aspectx;
static unsigned text_clip_aspecty;
os_clock_t text_clip_wait;
unsigned text_clip_count;
void* text_mng_context;

void text_clip_clear() {
	if (text_clip_f) {
		fzclose(text_clip_f);
		mng_done(text_mng_context);
		text_clip_f = 0;
	}
	text_clip_active = false;
	text_clip_running = false;
}

void text_clip_set(int back_index, const resource& res, unsigned aspectx, unsigned aspecty) {
	text_clip_clear();

	text_clip_index = back_index;
	text_clip_res = res;
	text_clip_active = true;
	text_clip_running = false;
	text_clip_aspectx = aspectx;
	text_clip_aspecty = aspecty;
}

void text_clip_init() {
	text_clip_f = 0;
	text_clip_active = false;
}

void text_clip_start() {
	if (!text_clip_active)
		return;

	text_clip_running = true;
	text_clip_wait = os_clock(); // reset the start time
}

void text_clip_done() {
	text_clip_clear();
}

static bool text_clip_need_load() {
	if (!text_clip_active)
		return false;

	if (!text_clip_running)
		return false;

	if (!text_clip_f)
		return true;

	if (os_clock() > text_clip_wait)
		return true;

	return false;
}

static struct bitmap* text_clip_load(video_color* rgb_map, unsigned* rgb_max) {
	if (!text_clip_active)
		return 0;

	if (!text_clip_f) {
		// first load
		text_clip_f = text_clip_res.open();
		if (!text_clip_f) {
			text_clip_active = false;
			return 0;
		}

		text_mng_context = mng_init(text_clip_f);
		if (text_mng_context == 0) {
			fzclose(text_clip_f);
			text_clip_f = 0;
			text_clip_active = false;
			return 0;
		}

		text_clip_wait = os_clock();
		text_clip_count = 0;
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

	int r = mng_read(text_mng_context, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, text_clip_f);
	if (r != 0) {
		mng_done(text_mng_context);
		fzclose(text_clip_f);
		text_clip_f = 0;
		text_clip_active = false;
		return 0;
	}

	double delay = tick / (double)mng_frequency_get(text_mng_context);

	struct bitmap* bitmap = bitmappalette_import(rgb_map, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
	if (!bitmap) {
		free(dat_ptr);
		free(pal_ptr);
		mng_done(text_mng_context);
		fzclose(text_clip_f);
		text_clip_f = 0;
		text_clip_active = false;
		return 0;
	}

	free(pal_ptr);

	text_clip_wait += (os_clock_t)(delay * OS_CLOCKS_PER_SEC);

	++text_clip_count;

	return bitmap;
}

//---------------------------------------------------------------------------
// Update

extern "C" int fast_exit_handler(void) {
	if (text_wait_for_backdrop)
		return 0;

	text_keypressed();
	return text_key_saved == TEXT_KEY_PGUP
		|| text_key_saved == TEXT_KEY_PGDN
		|| text_key_saved == TEXT_KEY_INS
		|| text_key_saved == TEXT_KEY_DEL
		|| text_key_saved == TEXT_KEY_HOME
		|| text_key_saved == TEXT_KEY_END
		|| text_key_saved == TEXT_KEY_UP
		|| text_key_saved == TEXT_KEY_DOWN
		|| text_key_saved == TEXT_KEY_LEFT
		|| text_key_saved == TEXT_KEY_RIGHT
		|| text_key_saved == TEXT_KEY_MODE;
}

static void icon_apply(struct bitmap* bitmap, struct bitmap* bitmap_mask, video_color* rgb, unsigned* rgb_max, video_color trasparent) {
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

	rgb[index] = trasparent;

	for(unsigned y=0;y<bitmap->size_y;++y) {
		uint8* line = bitmap_line(bitmap, y);
		uint8* line_mask = bitmap_line(bitmap_mask, y);
		for(unsigned x=0;x<bitmap->size_x;++x) {
			if (!line_mask[x])
				line[x] = index;
		}
	}
}

static struct bitmap* backdrop_load(const resource& res, video_color* rgb, unsigned* rgb_max) {
	string ext = file_ext(res.path_get());

	if (ext == ".png") {
		FZ* f = res.open();
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

		struct bitmap* bitmap = bitmappalette_import(rgb, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
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
		FZ* f = res.open();
		if (!f)
			return 0;
		struct bitmap* bitmap =  pcx_load(f,rgb,rgb_max);
		fzclose(f);
		return bitmap;
	}

	if (ext == ".ico") {
		FZ* f = res.open();
		if (!f)
			return 0;
		struct bitmap* bitmap_mask;
		struct bitmap* bitmap = icon_load(f,rgb,rgb_max,&bitmap_mask);
		if (!bitmap) {
			fzclose(f);
			return 0;
		}
		icon_apply(bitmap,bitmap_mask,rgb,rgb_max,PALETTE_RESERVED[backdrop_missing_color >> 4]);
		bitmap_free(bitmap_mask);
		fzclose(f);
		return bitmap;
	}

	if (ext == ".mng") {
		void* mng_context;

		FZ* f = res.open();
		if (!f)
			return 0;

		mng_context = mng_init(f);
		if (mng_context == 0) {
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

		int r = mng_read(mng_context, &pix_width, &pix_height, &pix_pixel, &dat_ptr, &dat_size, &pix_ptr, &pix_scanline, &pal_ptr, &pal_size, &tick, f);
		if (r != 0) {
			mng_done(mng_context);
			fzclose(f);
			return 0;
		}

		struct bitmap* bitmap = bitmappalette_import(rgb, rgb_max, pix_width, pix_height, pix_pixel, dat_ptr, dat_size, pix_ptr, pix_scanline, pal_ptr, pal_size);
		if (!bitmap) {
			free(dat_ptr);
			free(pal_ptr);
			mng_done(mng_context);
			fzclose(f);
			return 0;
		}

		free(pal_ptr);

		// duplicate the bitmap, it must exists also after destroying the mng context
		struct bitmap* dup_bitmap;
		if (bitmap->bytes_per_pixel == 4) {
			dup_bitmap = bitmap_alloc(bitmap->size_x, bitmap->size_y, 24);
			bitmap_cvt_32to24(dup_bitmap, bitmap);
		} else {
			dup_bitmap = bitmap_dup(bitmap);
		}

		bitmap_free(bitmap);
		mng_done(mng_context);
		fzclose(f);

		return dup_bitmap;
	}

	return 0;
}

void text_backdrop_compute_real_size(unsigned* rx, unsigned* ry, struct backdrop_t* back, struct bitmap* bitmap, unsigned aspectx, unsigned aspecty) {
	if (text_orientation & ORIENTATION_FLIP_XY) {
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

void text_backdrop_compute_virtual_size(unsigned* rx, unsigned* ry, struct backdrop_t* back, struct bitmap* bitmap, unsigned aspectx, unsigned aspecty) {
	if (!aspectx || !aspecty) {
		aspectx = bitmap->size_x;
		aspecty = bitmap->size_y;
	}

	if (!aspectx || !aspecty) {
		aspectx = 1;
		aspecty = 1;
	}

	if (text_orientation & ORIENTATION_FLIP_XY) {
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

static struct bitmap* text_backdrop_compute_bitmap(struct backdrop_t* back, struct bitmap* bitmap, video_color* rgb, unsigned* rgb_max, unsigned aspectx, unsigned aspecty) {
	if (bitmap->size_x < back->pos.dx) {
		unsigned rx,ry;

		// flip
		bitmap_orientation(bitmap,text_orientation & ORIENTATION_FLIP_XY);

		text_backdrop_compute_real_size(&rx, &ry, back, bitmap, aspectx, aspecty);

		// resize & mirror
		struct bitmap* shrink_bitmap = bitmap_resize(bitmap,0,0,bitmap->size_x,bitmap->size_y,rx,ry,text_orientation);
		if (!shrink_bitmap) {
			bitmap_free(bitmap);
			return 0;
		}
		bitmap_free(bitmap);
		bitmap = shrink_bitmap;
	} else {
		unsigned rx,ry;

		text_backdrop_compute_virtual_size(&rx, &ry, back, bitmap, aspectx, aspecty);

		// resize & mirror
		unsigned flip = (text_orientation & ORIENTATION_FLIP_XY) ? ORIENTATION_MIRROR_X | ORIENTATION_MIRROR_Y : 0;
		struct bitmap* shrink_bitmap = bitmap_resize(bitmap,0,0,bitmap->size_x,bitmap->size_y,rx,ry,text_orientation ^ flip);
		if (!shrink_bitmap) {
			bitmap_free(bitmap);
			return 0;
		}
		bitmap_free(bitmap);
		bitmap = shrink_bitmap;

		// flip
		bitmap_orientation(bitmap,text_orientation & ORIENTATION_FLIP_XY);
	}

	struct bitmap* pal_bitmap;
	if (video_bytes_per_pixel() == 1) {
		// video 8 bit
		pal_bitmap = bitmap_alloc(bitmap->size_x,bitmap->size_y,8);
		if (video_index() == VIDEO_FLAGS_INDEX_PACKED) {
			// video index mode
			if (bitmap->bytes_per_pixel == 1) {
				// bitmap 8 bit
				unsigned index_map[256];
				palette_set(rgb,*rgb_max,index_map);
				bitmap_cvt_8to8(pal_bitmap,bitmap,index_map);
			} else if (bitmap->bytes_per_pixel==3) {
				// bitmap 24 bit
				unsigned index_map[256];
				unsigned convert_map[BITMAP_INDEX_MAX];
				*rgb_max = 256 - PALETTE_RESERVED_MAX;
				*rgb_max = bitmap_reduction(convert_map, rgb, *rgb_max, bitmap);
				palette_set(rgb,*rgb_max,index_map);
				for(unsigned i=0;i<BITMAP_INDEX_MAX;++i)
					convert_map[i] = index_map[convert_map[i]];
				bitmap_cvt_24to8idx(pal_bitmap,bitmap,convert_map);
			} else {
				bitmap_free(bitmap);
				bitmap_free(pal_bitmap);
				return 0;
			}
		} else {
			// video rgb mode
			unsigned color_map[256];
			for(unsigned i=0;i<*rgb_max;++i)
				video_rgb_make(color_map + i,rgb[i].red,rgb[i].green,rgb[i].blue);
			if (bitmap->bytes_per_pixel==1) {
				// bitmap 8 bit
				bitmap_cvt_8to8(pal_bitmap,bitmap,color_map);
			} else if (bitmap->bytes_per_pixel==3) {
				// bitmap 24 bit
				bitmap_cvt_24to8rgb(pal_bitmap,bitmap);
			} else {
				bitmap_free(bitmap);
				bitmap_free(pal_bitmap);
				return 0;
			}
		}
	} else if (video_bytes_per_pixel() == 2) {
		// video 16 bit
		pal_bitmap = bitmap_alloc(bitmap->size_x,bitmap->size_y,16);
		if (bitmap->bytes_per_pixel == 1) {
			// bitmap 8 bit
			unsigned color_map[256];
			for(unsigned i=0;i<*rgb_max;++i)
				video_rgb_make(color_map + i,rgb[i].red,rgb[i].green,rgb[i].blue);
			bitmap_cvt_8to16(pal_bitmap,bitmap,color_map);
		} else if (bitmap->bytes_per_pixel == 3) {
			// bitmap 24 bit
			bitmap_cvt_24to16(pal_bitmap,bitmap);
		} else {
			bitmap_free(bitmap);
			bitmap_free(pal_bitmap);
			return 0;
		}
	} else if (video_bytes_per_pixel() == 4) {
		// video 32 bit
		pal_bitmap = bitmap_alloc(bitmap->size_x,bitmap->size_y,32);
		if (bitmap->bytes_per_pixel == 1) {
			// bitmap 8 bit
			unsigned color_map[256];
			for(unsigned i=0;i<*rgb_max;++i)
				video_rgb_make(color_map + i,rgb[i].red,rgb[i].green,rgb[i].blue);
			bitmap_cvt_8to32(pal_bitmap,bitmap,color_map);
		} else if (bitmap->bytes_per_pixel == 3) {
			// bitmap 24 bit
			bitmap_cvt_24to32(pal_bitmap,bitmap);
		} else {
			bitmap_free(bitmap);
			bitmap_free(pal_bitmap);
			return 0;
		}
	} else {
		bitmap_free(bitmap);
		return 0;
	}

	bitmap_free(bitmap);
	bitmap = pal_bitmap;

	return bitmap;
}

static void text_backdrop_clear(struct backdrop_t* back) {
	text_clear(back->pos.x,back->pos.y,back->pos.dx,back->pos.dy,backdrop_missing_color >> 4);
}

static void text_backdrop_load(struct backdrop_t* back) {
	if (!back->data->bitmap_get()) {

		struct bitmap* bitmap = backdrop_load(back->data->res_get(), back->data->rgb, &back->data->rgb_max);
		if (!bitmap)
			return;

		bitmap = text_backdrop_compute_bitmap(back, bitmap, back->data->rgb, &back->data->rgb_max, back->data->aspectx_get(), back->data->aspecty_get());
		if (!bitmap)
			return;

		// store
		back->data->bitmap_set(bitmap);
	} else {
		if (video_index() == VIDEO_FLAGS_INDEX_PACKED) {
			unsigned index_map[256];
			palette_set(back->data->rgb,back->data->rgb_max,index_map);
		}
	}
}

// Update the backdrop image
static void text_backdrop_update(struct backdrop_t* back) {
	if (back->data) {
		if (!fast_exit_handler())
			text_backdrop_load(back);
	}

	if (back->pos.redraw) {
		if (back->data) {
			if (back->data->bitmap_get()) {
				back->pos.redraw = false;
				text_backdrop_raw(back, back->data->bitmap_get());
			} else {
				// the image need to be update when loaded
				text_backdrop_clear(back);
			}
		} else {
			back->pos.redraw = false;
			text_backdrop_clear(back);
		}

		if (backdrop_outline)
			text_box(back->pos.x-backdrop_outline,back->pos.y-backdrop_outline,back->pos.dx+2*backdrop_outline,back->pos.dy+2*backdrop_outline,backdrop_outline,backdrop_missing_color);
	}
}

static void text_copy_partial(unsigned y0, unsigned y1) {
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

unsigned text_update_pre(bool progressive) {
	text_updating_active = false;

	play_poll();

	text_backdrop_cache_reduce();

	/* this palette is used if no one image is present */
	if (video_index() == VIDEO_FLAGS_INDEX_PACKED)
		palette_set(0,0,0);

	int y = 0;
	for(int i=0;i<backdrop_mac;++i) {
		play_poll();

		// this thick work only if the backdrop are from top to down
		if (text_orientation == 0 && progressive) {
			// update progressively the screen
			int yl = backdrop_map[i].pos.real_y - backdrop_outline;
			if (yl > y) {
				text_copy_partial(y,yl);
				y = yl;
			}
		}

		text_backdrop_update(backdrop_map + i);
	}

	text_backdrop_cache_all();

	return y;
}

void text_update_post(unsigned y) {
	play_poll();

	text_copy_partial(y,video_size_y());

	video_backdrop_box();

	if (video_index() == VIDEO_FLAGS_INDEX_PACKED)
		video_palette_set(text_palette,0,256,false);

	play_poll();

	text_updating_active = true;
}

void text_update(bool progressive) {
	text_update_post(text_update_pre(progressive));
}

#ifdef __SNAPSHOT__
void text_snapshot() {
	static unsigned snapshot_num = 0;
	char snapshot_name[32];
	sprintf(snapshot_name,"snap%d.bmp",snapshot_num);
	snapshot_num++;

	video_snapshot_save(snapshot_name,0,0);
}
#endif

//---------------------------------------------------------------------------
// Key

#define SEQ_MAX 256

struct key_cvt {
	const char* name;
	unsigned event;
	unsigned seq[SEQ_MAX];
};

// Special operator
#define OP_NONE OS_KEY_MAX
#define OP_OR (OS_KEY_MAX + 1)
#define OP_NOT (OS_KEY_MAX + 2)

static struct key_cvt KEYTAB[] = {
{"up", TEXT_KEY_UP, { OS_KEY_UP, OS_KEY_MAX } },
{"down", TEXT_KEY_DOWN, { OS_KEY_DOWN, OS_KEY_MAX } },
{"left", TEXT_KEY_LEFT, { OS_KEY_LEFT, OS_KEY_MAX } },
{"right", TEXT_KEY_RIGHT, { OS_KEY_RIGHT, OS_KEY_MAX } },
{"enter", TEXT_KEY_ENTER, { OS_KEY_ENTER, OS_KEY_MAX } },
{"shutdown", TEXT_KEY_OFF, { OS_KEY_LCONTROL, OS_KEY_ESC, OS_KEY_MAX } },
{"esc", TEXT_KEY_ESC, { OS_KEY_ESC, OS_KEY_MAX } },
{"space", TEXT_KEY_SPACE, { OS_KEY_SPACE, OS_KEY_MAX } },
{"mode", TEXT_KEY_MODE, { OS_KEY_TAB, OS_KEY_MAX } },
{"home", TEXT_KEY_HOME, { OS_KEY_HOME, OS_KEY_MAX } },
{"end", TEXT_KEY_END, { OS_KEY_END, OS_KEY_MAX } },
{"pgup", TEXT_KEY_PGUP, { OS_KEY_PGUP, OS_KEY_MAX } },
{"pgdn", TEXT_KEY_PGDN, { OS_KEY_PGDN, OS_KEY_MAX } },
{"help", TEXT_KEY_HELP, { OS_KEY_F1, OS_KEY_MAX } },
{"group", TEXT_KEY_GROUP, { OS_KEY_F2, OS_KEY_MAX } },
{"type", TEXT_KEY_TYPE, { OS_KEY_F3, OS_KEY_MAX } },
{"exclude", TEXT_KEY_EXCLUDE, { OS_KEY_F4, OS_KEY_MAX } },
{"sort", TEXT_KEY_SORT, { OS_KEY_F5, OS_KEY_MAX } },
{"setgroup", TEXT_KEY_SETGROUP, { OS_KEY_F9, OS_KEY_MAX } },
{"settype", TEXT_KEY_SETTYPE, { OS_KEY_F10, OS_KEY_MAX } },
{"runclone", TEXT_KEY_RUN_CLONE, { OS_KEY_F12, OS_KEY_MAX } },
{"del", TEXT_KEY_DEL, { OS_KEY_DEL, OS_KEY_MAX } },
{"ins", TEXT_KEY_INS, { OS_KEY_INSERT, OS_KEY_MAX } },
{"command", TEXT_KEY_COMMAND, { OS_KEY_F8, OS_KEY_MAX } },
{"menu", TEXT_KEY_MENU, { OS_KEY_BACKQUOTE, OP_OR, OS_KEY_BACKSLASH, OS_KEY_MAX } },
{"emulator", TEXT_KEY_EMU, { OS_KEY_F6, OS_KEY_MAX } },
{"snapshot", TEXT_KEY_SNAPSHOT, { OS_KEY_PERIOD_PAD, OS_KEY_MAX } },
{"rotate", TEXT_KEY_ROTATE, { OS_KEY_0_PAD, OS_KEY_MAX } },
{"lock", TEXT_KEY_LOCK, { OS_KEY_SCRLOCK, OS_KEY_MAX } },
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
					int pressed = keyb_get(code[j]);
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
{ OS_KEY_A, 'a' },
{ OS_KEY_B, 'b' },
{ OS_KEY_C, 'c' },
{ OS_KEY_D, 'd' },
{ OS_KEY_E, 'e' },
{ OS_KEY_F, 'f' },
{ OS_KEY_G, 'g' },
{ OS_KEY_H, 'h' },
{ OS_KEY_I, 'i' },
{ OS_KEY_J, 'j' },
{ OS_KEY_K, 'k' },
{ OS_KEY_L, 'l' },
{ OS_KEY_M, 'm' },
{ OS_KEY_N, 'n' },
{ OS_KEY_O, 'o' },
{ OS_KEY_P, 'p' },
{ OS_KEY_Q, 'q' },
{ OS_KEY_R, 'r' },
{ OS_KEY_S, 's' },
{ OS_KEY_T, 't' },
{ OS_KEY_U, 'u' },
{ OS_KEY_V, 'v' },
{ OS_KEY_W, 'w' },
{ OS_KEY_X, 'x' },
{ OS_KEY_Y, 'y' },
{ OS_KEY_Z, 'z' },
{ OS_KEY_0, '0' },
{ OS_KEY_1, '1' },
{ OS_KEY_2, '2' },
{ OS_KEY_3, '3' },
{ OS_KEY_4, '4' },
{ OS_KEY_5, '5' },
{ OS_KEY_6, '6' },
{ OS_KEY_7, '7' },
{ OS_KEY_8, '8' },
{ OS_KEY_9, '9' },
{ OS_KEY_MAX, ' ' },
};

static int keyboard_raw_poll() {
	for(const struct key_cvt* i=KEYTAB;i->name;++i) {
		if (seq_pressed(i->seq))
			return i->event;
	}

	if (text_alpha_mode) {
		for(unsigned i=0;KEY_CONV[i].code != OS_KEY_MAX;++i)
			if (keyb_get(KEY_CONV[i].code))
				return KEY_CONV[i].c;
	}

	return TEXT_KEY_NONE;
}

static int key_poll() {
	static int key_repeat_last = TEXT_KEY_NONE;

	static os_clock_t key_repeat_last_time = 0;
	static bool key_repeat_last_counter = 0;

	int r = TEXT_KEY_NONE;

	if (r == TEXT_KEY_NONE) {
		if (os_is_term())
			r = TEXT_KEY_ESC;
	}

	if (r == TEXT_KEY_NONE) {
		r = keyboard_raw_poll();
	}

	if (r == TEXT_KEY_NONE) {
		r = text_joystick_button_raw_poll();
	}

	if (r == TEXT_KEY_NONE) {
		r = text_mouse_button_raw_poll();
	}

	if (r == TEXT_KEY_NONE) {
		r = text_mouse_move_raw_poll();

		// never repeat or wait or play for the mouse movements
		if (r != TEXT_KEY_NONE) {
			key_repeat_last = r;
			key_repeat_last_counter = 0;
			key_repeat_last_time = os_clock();
			return r;
		}
	}

	if (r == TEXT_KEY_NONE) {
		r = text_joystick_move_raw_poll();
	}

	if (r == TEXT_KEY_NONE) {
		key_repeat_last = TEXT_KEY_NONE;
		key_repeat_last_counter = 0;
		key_repeat_last_time = os_clock();
		return TEXT_KEY_NONE;
	} else if (r != key_repeat_last) {
		key_repeat_last = r;
		key_repeat_last_counter = 0;
		key_repeat_last_time = os_clock();
		play_foreground_effect_key(text_sound_event_key);
		return r;
	} else {
		if ((key_repeat_last_counter == 0 && (os_clock() - key_repeat_last_time > text_repeat * OS_CLOCKS_PER_SEC / 1000)) ||
			(key_repeat_last_counter > 0 && (os_clock() - key_repeat_last_time > text_repeat_rep * OS_CLOCKS_PER_SEC / 1000))) {
			key_repeat_last_time = os_clock();
			++key_repeat_last_counter;
			return r;
		} else {
			return TEXT_KEY_NONE;
		}
	}
}

void text_idle_repeat_reset() {
	text_key_last = TEXT_KEY_NONE;
}

void text_idle_time_reset() {
	text_idle_time = time(0);
}

static void text_clip_idle() {
	video_color rgb_map[256];
	unsigned rgb_max;
	backdrop_t* back = backdrop_map + text_clip_index;

	if (video_index() != VIDEO_FLAGS_INDEX_RGB)
		return;

	if (!text_clip_need_load()) {
		return;
	}

	struct bitmap* bitmap = text_clip_load(rgb_map, &rgb_max);
	if (!bitmap) {
		// update the screen with the previous backdrop
		text_copy_partial(back->pos.real_y, back->pos.real_y + back->pos.real_dy);
		return;
	}

	unsigned color = index2color(backdrop_missing_color >> 4);

	// source range and steps
	unsigned char* ptr = bitmap->ptr;
	int dw = bitmap->bytes_per_scanline;
	int dp = bitmap->bytes_per_pixel;
	int dx = bitmap->size_x;
	int dy = bitmap->size_y;

	// set the correct orientation
	if (text_orientation & ORIENTATION_FLIP_XY) {
		int t;
		t = dp;
		dp = dw;
		dw = t;
		t = dx;
		dx = dy;
		dy = t;
	}
	if (text_orientation & ORIENTATION_MIRROR_X) {
		ptr = ptr + (dx-1) * dp;
		dp = - dp;
	}
	if (text_orientation & ORIENTATION_MIRROR_Y) {
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
	text_backdrop_compute_real_size(&rel_dx, &rel_dy, back, bitmap, text_clip_aspectx, text_clip_aspecty);

	// start writing
	video_write_lock();

	// adjust the destination range if too big
	if (dst_dx > rel_dx) {
		unsigned pre_dx = (dst_dx - rel_dx) / 2;
		unsigned post_dx = (dst_dx - rel_dx + 1) / 2;

		if (text_clip_count == 1) {
			video_clear(dst_x, dst_y, pre_dx, dst_dy, color);
			video_clear(dst_x + pre_dx + rel_dx, dst_y, post_dx, dst_dy, color);
		}

		dst_x += pre_dx;
		dst_dx = rel_dx;
	}
	if (dst_dy > rel_dy) {
		unsigned pre_dy = (dst_dy - rel_dy) / 2;
		unsigned post_dy = (dst_dy - rel_dy + 1) / 2;

		if (text_clip_count == 1) {
			video_clear(dst_x, dst_y, dst_dx, pre_dy, color);
			video_clear(dst_x, dst_y + pre_dy + rel_dy, dst_dx, post_dy, color);
		}

		dst_y += pre_dy;
		dst_dy = rel_dy;
	}

#if 1
	unsigned combine = VIDEO_COMBINE_Y_NONE;
#else
	unsigned combine =  VIDEO_COMBINE_Y_FILTER | VIDEO_COMBINE_X_FILTER;
#endif

	// write
	if (bitmap->bytes_per_pixel == 1) {
		video_rgb palette[256];
		unsigned i;

		for(i=0;i<rgb_max;++i)
			video_rgb_make(palette + i, rgb_map[i].red, rgb_map[i].green, rgb_map[i].blue);

		video_stretch_palette_8(dst_x, dst_y, dst_dx, dst_dy, ptr, dx, dy, dw, dp, palette, combine);
	} else if (bitmap->bytes_per_pixel == 3 || bitmap->bytes_per_pixel == 4) {
		video_rgb_def rgb_def = video_rgb_def_make(8,0,8,8,8,16);

		video_stretch(dst_x, dst_y, dst_dx, dst_dy, ptr, dx, dy, dw, dp, rgb_def, combine);
	}

	// end write
	video_write_unlock(dst_x, dst_y, dst_dx, dst_dy);

	bitmap_free(bitmap);
}

static void text_box_idle() {
	static os_clock_t text_backdrop_box_last = 0;
	os_clock_t text_backdrop_box_new = os_clock();
	if (text_backdrop_box_new >= text_backdrop_box_last + OS_CLOCKS_PER_SEC/20) {
		text_backdrop_box_last = text_backdrop_box_new;
		video_backdrop_box();
	}
}

static void text_idle() {
	if (text_key_last == TEXT_KEY_IDLE_0 && text_idle_0_rep && time(0) - text_idle_time > text_idle_0_rep)
		text_key_saved = TEXT_KEY_IDLE_0;

	if (text_key_last == TEXT_KEY_IDLE_1 && text_idle_1_rep && time(0) - text_idle_time > text_idle_1_rep)
		text_key_saved = TEXT_KEY_IDLE_1;

	if (text_idle_0 && time(0) - text_idle_time > text_idle_0)
		text_key_saved = TEXT_KEY_IDLE_0;

	if (text_idle_1 && time(0) - text_idle_time > text_idle_1)
		text_key_saved = TEXT_KEY_IDLE_1;

	if (text_key_saved == TEXT_KEY_NONE) {
		if (text_updating_active) {
			text_clip_idle();
			text_box_idle();
			target_idle();
		}
	}

	play_poll();
	keyb_poll();
	mouseb_poll();
	joystickb_poll();
}

int text_keypressed() {
	static os_clock_t key_pressed_last_time = 0;

	if (text_key_saved != TEXT_KEY_NONE)
		return 1;

	os_clock_t now = os_clock();

	text_idle();

	/* don't check too fast */
	if (now - key_pressed_last_time >= OS_CLOCKS_PER_SEC / 25) {
		key_pressed_last_time = now;

		text_key_saved = key_poll();

		if (text_key_saved != TEXT_KEY_NONE)
			return 1;
	}

	return 0;
}

unsigned text_getkey(bool update_background) {
	if (update_background)
		text_update();

	while (!text_keypressed()) { }

	text_idle_time = time(0);

	assert( text_key_saved != TEXT_KEY_NONE);

	text_key_last = text_key_saved;
	text_key_saved = TEXT_KEY_NONE;

#ifdef __SNAPSHOT__
	if (text_key_last == TEXT_KEY_SNAPSHOT)
		text_snapshot();
#endif

	return text_key_last;
}

static void text_key_insert(unsigned event, unsigned* seq) {
	unsigned i;
	for(i=0;KEYTAB[i].name && KEYTAB[i].event != event;++i);
	if (KEYTAB[i].name) {
		unsigned j;
		for(j=0;seq[j]!=OP_NONE;++j)
			KEYTAB[i].seq[j] = seq[j];
		KEYTAB[i].seq[j] = OP_NONE;
	}
}

static unsigned string2event(const string& s) {
	unsigned i;
	for(i=0;KEYTAB[i].name && KEYTAB[i].name != s;++i);
	if (KEYTAB[i].name)
		return KEYTAB[i].event;
	else
		return TEXT_KEY_NONE;
}

bool text_key_in(const string& s) {
	string sevent;
	unsigned seq[SEQ_MAX];
	unsigned seq_count;
	int pos = 0;

	sevent = arg_get(s,pos);

	unsigned event = string2event(sevent);
	if (event == TEXT_KEY_NONE)
		return false;

	seq_count = 0;
	while (pos < s.length()) {
		if (seq_count+1 >= SEQ_MAX)
			return false;

		string skey = arg_get(s,pos);
		if (skey == "or") {
			seq[seq_count++] = OP_OR;
		} else if (skey == "not") {
			seq[seq_count++] = OP_NOT;
		} else if (skey == "and") {
			/* nothing */
		} else {
			unsigned key = key_code(skey.c_str());
			if (key == OS_KEY_MAX)
				return false;
			seq[seq_count++] = key;
		}
	}

	seq[seq_count] = OP_NONE;

	if (!seq_valid(seq))
		return false;

	text_key_insert(event,seq);

	return true;
}

void text_key_out(struct conf_context* config_context, const char* tag) {
	for(unsigned i=0;KEYTAB[i].name;++i) {
		string s;
		s += KEYTAB[i].name;
		s += " ";
		for(unsigned j=0;KEYTAB[i].seq[j] != OS_KEY_MAX && j<SEQ_MAX;++j) {
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

// 9 blue
// A green
// B cyan
// C red
// D magenta
// E yellow
// F white

unsigned COLOR_HELP_NORMAL = 0xF0;
unsigned COLOR_HELP_TAG = 0xFC;
unsigned COLOR_CHOICE_TITLE = 0xFC;
unsigned COLOR_CHOICE_NORMAL = 0xF0;
unsigned COLOR_CHOICE_SELECT = 0xA0;
unsigned COLOR_MENU_NORMAL = 0xF0;
unsigned COLOR_MENU_HIDDEN = 0xF8;
unsigned COLOR_MENU_TAG = 0xFC;
unsigned COLOR_MENU_SELECT = 0xA0;
unsigned COLOR_MENU_HIDDEN_SELECT = 0xA8;
unsigned COLOR_MENU_TAG_SELECT = 0xAC;
unsigned COLOR_MENU_BAR = 0xF0;
unsigned COLOR_MENU_BAR_TAG = 0xFC;
unsigned COLOR_MENU_BAR_HIDDEN = 0xF8;
unsigned COLOR_MENU_GRID = 0xFC;
unsigned COLOR_MENU_BACKDROP = 0x80;
unsigned COLOR_MENU_ICON = 0xFF;
unsigned COLOR_MENU_CURSOR = 0xF8;

static struct {
	unsigned* var;
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
{ 0,0 }
};

static const char* COLOR[] = {
"black",
"blue",
"green",
"cyan",
"red",
"magenta",
"brown",
"lightgray",
"gray",
"lightblue",
"lightgreen",
"lightcyan",
"lightred",
"lightmagenta",
"yellow",
"white"
};

static unsigned string2color(const string& s) {
	for(unsigned i=0;i<16;++i)
		if (COLOR[i] == s)
			return i;
	return 0;
}

static const char* color2string(unsigned c) {
	if (c<16)
		return COLOR[c];
	else
		return COLOR[0];
}

bool text_color_in(const string& s) {
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

	*COLOR_TAB[i].var = string2color(sarg0) | string2color(sarg1) << 4;

	return true;
}

void text_color_out(struct conf_context* config_context, const char* tag) {
	for(unsigned i=0;COLOR_TAB[i].name;++i) {
		string s;
		s += COLOR_TAB[i].name;
		s += " ";
		s += color2string(*COLOR_TAB[i].var & 0xF);
		s += " ";
		s += color2string(*COLOR_TAB[i].var >> 4);
		conf_set(config_context, "", tag, s.c_str());
	}
}
