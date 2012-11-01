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

#include "portable.h"

#include "draw.h"

#include "advance.h"

/***************************************************************************/
/* Common variable */

/* Sound enabled */
int the_sound_flag = 1;

/* Default video mode */
adv_mode the_default_mode;

/* Default video mode set */
int the_default_mode_flag = 0;

/***************************************************************************/
/* Video crtc container extension */

adv_crtc* crtc_container_pos(adv_crtc_container* vmc, unsigned pos)
{
	adv_crtc_container_iterator i;
	crtc_container_iterator_begin(&i, vmc);
	while (pos && !crtc_container_iterator_is_end(&i)) {
		--pos;
		crtc_container_iterator_next(&i);
	}
	if (crtc_container_iterator_is_end(&i))
		return 0;
	else
		return crtc_container_iterator_get(&i);
}

unsigned crtc_container_max(adv_crtc_container* vmc)
{
	unsigned pos = 0;
	adv_crtc_container_iterator i;
	crtc_container_iterator_begin(&i, vmc);
	while (!crtc_container_iterator_is_end(&i)) {
		++pos;
		crtc_container_iterator_next(&i);
	}
	return pos;
}

/***************************************************************************/
/* Sound */

void sound_error(void)
{
	if (the_sound_flag) {
		target_sound_error();
	}
}

void sound_warn(void)
{
	if (the_sound_flag) {
		target_sound_warn();
	}
}

void sound_signal(void)
{
	if (the_sound_flag) {
		target_sound_signal();
	}
}

/***************************************************************************/
/* Text output */

unsigned text_size_x(void)
{
	return video_size_x() / video_font_size_x();
}

unsigned text_size_y(void)
{
	return video_size_y() / video_font_size_y();
}

void text_clear(void)
{
	unsigned x, y;
	for(x=0;x<text_size_x();++x)
		for(y=0;y<text_size_y();++y)
			video_put_char(x, y, ' ', 0);
}

/* Compare function to get the best video mode to display the program screen */
int text_crtc_compare(const adv_crtc* a, const adv_crtc* b)
{
	unsigned as = a->hde * a->vde;
	unsigned bs = b->hde * b->vde;

	int ad = abs(as - 640*480);
	int bd = abs(bs - 640*480);

	if (ad > bd)
		return -1;
	if (ad < bd)
		return 1;

	return 0;
}

static int text_default_set(adv_crtc_container* cc, adv_monitor* monitor)
{
	adv_crtc_container_iterator i;

	log_std(("text: selecting text mode\n"));

	/* search for the default mode */
	if (cc && monitor && !the_default_mode_flag) {
		for(crtc_container_iterator_begin(&i, cc);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
			const adv_crtc* crtc = crtc_container_iterator_get(&i);
			adv_mode mode;
			if (strcmp(crtc->name, DEFAULT_TEXT_MODE)==0) {
				if ((crtc_is_fake(crtc) || crtc_clock_check(monitor, crtc))
					&& video_mode_generate(&mode, crtc, MODE_FLAGS_INDEX_TEXT)==0) {
					log_std(("text: using specified %s mode\n", crtc->name));
					the_default_mode = mode;
					the_default_mode_flag = 1;
				}
			}
		}
	}

	/* search the best mode in the list */
	if (cc && monitor && !the_default_mode_flag) {
		adv_crtc default_crtc = { 0 };
		for(crtc_container_iterator_begin(&i, cc);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
			const adv_crtc* crtc = crtc_container_iterator_get(&i);
			adv_mode mode;
			if ((crtc_is_fake(crtc) || crtc_clock_check(monitor, crtc))
				&& video_mode_generate(&mode, crtc, MODE_FLAGS_INDEX_TEXT)==0) {
				if (!the_default_mode_flag || text_crtc_compare(crtc, &default_crtc)>0) {
					log_std(("text: using mode %s from list\n", crtc->name));
					the_default_mode = mode;
					default_crtc = *crtc;
					the_default_mode_flag = 1;
				}
			}
		}
	}

	/* add a fake text mode for the windowed modes */
	if (!the_default_mode_flag) {
		if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_TEXT, VIDEO_DRIVER_FLAGS_OUTPUT_WINDOW)) != 0) {
			adv_crtc crtc;
			adv_mode mode;
			crtc_reset(&crtc);
			crtc_fake_set(&crtc, 640, 480);
			if (video_mode_generate(&mode, &crtc, MODE_FLAGS_INDEX_TEXT)==0) {
				log_std(("text: using generated mode\n"));
				the_default_mode = mode;
				the_default_mode_flag = 1;
			}
		}
	}

	/* use the current mode */
	if (!the_default_mode_flag) {
		adv_mode mode;
		if (video_mode_grab(&mode) == 0
			&& mode_is_text(&mode)) {
			log_std(("text: using grabbed mode\n"));
			the_default_mode = mode;
			the_default_mode_flag = 1;
		}
	}

	if (!the_default_mode_flag)
		return 0;

	if (video_mode_set(&the_default_mode)!=0)
		return -1;

	return 0;
}

adv_error text_init(adv_crtc_container* cc, adv_monitor* monitor)
{
	if (text_default_set(cc, monitor)!=0) {
		video_mode_restore();
		target_err("Error initialing the default video mode.\n\r\"%s\"\n\r", error_get());
		return -1;
	}

	if (the_default_mode_flag == 0) {
		video_mode_restore();
		target_err("No text modes available for your hardware.\n\rPlease check your `device_video_p/h/vclock' options in the configuration file.\nEventually add a specific modeline named '" DEFAULT_TEXT_MODE "'\n\rand ensure that you have a text mode video driver compiled in.\n\rEnsure also to use the auto option for device_video.\n\r");
		return -1;
	}

	return 0;
}

void text_reset(void)
{
	inputb_disable();

	if (video_mode_is_active())
		video_mode_done(1);

	if (video_mode_set(&the_default_mode)!=0) {
		video_mode_restore();
		target_err("Error initialing the default video mode\n\r");
		exit(EXIT_FAILURE);
	}

	if (inputb_enable(0) != 0) {
		target_err("Error initialing the input mode\n\r");
		exit(EXIT_FAILURE);
	}
}

void text_done(void)
{
	video_mode_restore();
}

void text_put(int x, int y, char c, int color)
{
	if (y>=0 && y<text_size_y() && x>=0 && x<text_size_x()) {
		video_put_char(x, y, c, color);
	}
}

adv_error text_mode_set(adv_mode* mode)
{
	inputb_disable();

	if (video_mode_set(mode)!=0) {
		inputb_enable(0); /* ignore error */
		return -1;
	}

	if (inputb_enable(1) != 0) {
		return -1;
	}

	return 0;
}

/***************************************************************************/
/* Draw interface */

#define DRAW_FONTX 6
#define DRAW_FONTY 8

static unsigned char draw_font[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x30, 0x48, 0x84, 0xb4, 0xb4, 0x84, 0x48, 0x30, 0x30, 0x48, 0x84, 0x84, 0x84, 0x84, 0x48, 0x30,
	0x00, 0xfc, 0x84, 0x8c, 0xd4, 0xa4, 0xfc, 0x00, 0x00, 0xfc, 0x84, 0x84, 0x84, 0x84, 0xfc, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x68, 0x78, 0x78, 0x30, 0x00, 0x00,
	0x80, 0xc0, 0xe0, 0xf0, 0xe0, 0xc0, 0x80, 0x00, 0x04, 0x0c, 0x1c, 0x3c, 0x1c, 0x0c, 0x04, 0x00,
	0x20, 0x70, 0xf8, 0x20, 0x20, 0xf8, 0x70, 0x20, 0x48, 0x48, 0x48, 0x48, 0x48, 0x00, 0x48, 0x00,
	0x00, 0x00, 0x30, 0x68, 0x78, 0x30, 0x00, 0x00, 0x00, 0x30, 0x68, 0x78, 0x78, 0x30, 0x00, 0x00,
	0x70, 0xd8, 0xe8, 0xe8, 0xf8, 0xf8, 0x70, 0x00, 0x1c, 0x7c, 0x74, 0x44, 0x44, 0x4c, 0xcc, 0xc0,
	0x20, 0x70, 0xf8, 0x70, 0x70, 0x70, 0x70, 0x00, 0x70, 0x70, 0x70, 0x70, 0xf8, 0x70, 0x20, 0x00,
	0x00, 0x10, 0xf8, 0xfc, 0xf8, 0x10, 0x00, 0x00, 0x00, 0x20, 0x7c, 0xfc, 0x7c, 0x20, 0x00, 0x00,
	0xb0, 0x54, 0xb8, 0xb8, 0x54, 0xb0, 0x00, 0x00, 0x00, 0x28, 0x6c, 0xfc, 0x6c, 0x28, 0x00, 0x00,
	0x00, 0x30, 0x30, 0x78, 0x78, 0xfc, 0x00, 0x00, 0xfc, 0x78, 0x78, 0x30, 0x30, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x20, 0x00,
	0x50, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xf8, 0x50, 0xf8, 0x50, 0x00, 0x00,
	0x20, 0x70, 0xc0, 0x70, 0x18, 0xf0, 0x20, 0x00, 0x40, 0xa4, 0x48, 0x10, 0x20, 0x48, 0x94, 0x08,
	0x60, 0x90, 0xa0, 0x40, 0xa8, 0x90, 0x68, 0x00, 0x10, 0x20, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x20, 0x40, 0x40, 0x40, 0x40, 0x40, 0x20, 0x00, 0x10, 0x08, 0x08, 0x08, 0x08, 0x08, 0x10, 0x00,
	0x20, 0xa8, 0x70, 0xf8, 0x70, 0xa8, 0x20, 0x00, 0x00, 0x20, 0x20, 0xf8, 0x20, 0x20, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00,
	0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 0x10, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00,
	0x70, 0x88, 0x08, 0x10, 0x20, 0x40, 0xf8, 0x00, 0x70, 0x88, 0x08, 0x30, 0x08, 0x88, 0x70, 0x00,
	0x10, 0x30, 0x50, 0x90, 0xf8, 0x10, 0x10, 0x00, 0xf8, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70, 0x00,
	0x70, 0x80, 0xf0, 0x88, 0x88, 0x88, 0x70, 0x00, 0xf8, 0x08, 0x08, 0x10, 0x20, 0x20, 0x20, 0x00,
	0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70, 0x00, 0x70, 0x88, 0x88, 0x88, 0x78, 0x08, 0x70, 0x00,
	0x00, 0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x60,
	0x10, 0x20, 0x40, 0x80, 0x40, 0x20, 0x10, 0x00, 0x00, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00,
	0x40, 0x20, 0x10, 0x08, 0x10, 0x20, 0x40, 0x00, 0x70, 0x88, 0x08, 0x10, 0x20, 0x00, 0x20, 0x00,
	0x30, 0x48, 0x94, 0xa4, 0xa4, 0x94, 0x48, 0x30, 0x70, 0x88, 0x88, 0xf8, 0x88, 0x88, 0x88, 0x00,
	0xf0, 0x88, 0x88, 0xf0, 0x88, 0x88, 0xf0, 0x00, 0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70, 0x00,
	0xf0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xf0, 0x00, 0xf8, 0x80, 0x80, 0xf0, 0x80, 0x80, 0xf8, 0x00,
	0xf8, 0x80, 0x80, 0xf0, 0x80, 0x80, 0x80, 0x00, 0x70, 0x88, 0x80, 0x98, 0x88, 0x88, 0x70, 0x00,
	0x88, 0x88, 0x88, 0xf8, 0x88, 0x88, 0x88, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x08, 0x08, 0x08, 0x08, 0x88, 0x88, 0x70, 0x00, 0x88, 0x90, 0xa0, 0xc0, 0xa0, 0x90, 0x88, 0x00,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf8, 0x00, 0x88, 0xd8, 0xa8, 0x88, 0x88, 0x88, 0x88, 0x00,
	0x88, 0xc8, 0xa8, 0x98, 0x88, 0x88, 0x88, 0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00,
	0xf0, 0x88, 0x88, 0xf0, 0x80, 0x80, 0x80, 0x00, 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x08,
	0xf0, 0x88, 0x88, 0xf0, 0x88, 0x88, 0x88, 0x00, 0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70, 0x00,
	0xf8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00,
	0x88, 0x88, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00, 0x88, 0x88, 0x88, 0x88, 0xa8, 0xd8, 0x88, 0x00,
	0x88, 0x50, 0x20, 0x20, 0x20, 0x50, 0x88, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x00,
	0xf8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xf8, 0x00, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x00,
	0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x30, 0x00,
	0x20, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
	0x40, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x08, 0x78, 0x88, 0x78, 0x00,
	0x80, 0x80, 0xf0, 0x88, 0x88, 0x88, 0xf0, 0x00, 0x00, 0x00, 0x70, 0x88, 0x80, 0x80, 0x78, 0x00,
	0x08, 0x08, 0x78, 0x88, 0x88, 0x88, 0x78, 0x00, 0x00, 0x00, 0x70, 0x88, 0xf8, 0x80, 0x78, 0x00,
	0x18, 0x20, 0x70, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x78, 0x88, 0x88, 0x78, 0x08, 0x70,
	0x80, 0x80, 0xf0, 0x88, 0x88, 0x88, 0x88, 0x00, 0x20, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x20, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0xc0, 0x80, 0x80, 0x90, 0xa0, 0xe0, 0x90, 0x88, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xf0, 0xa8, 0xa8, 0xa8, 0xa8, 0x00,
	0x00, 0x00, 0xb0, 0xc8, 0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x70, 0x00,
	0x00, 0x00, 0xf0, 0x88, 0x88, 0xf0, 0x80, 0x80, 0x00, 0x00, 0x78, 0x88, 0x88, 0x78, 0x08, 0x08,
	0x00, 0x00, 0xb0, 0xc8, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x78, 0x80, 0x70, 0x08, 0xf0, 0x00,
	0x20, 0x20, 0x70, 0x20, 0x20, 0x20, 0x18, 0x00, 0x00, 0x00, 0x88, 0x88, 0x88, 0x98, 0x68, 0x00,
	0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20, 0x00, 0x00, 0x00, 0xa8, 0xa8, 0xa8, 0xa8, 0x50, 0x00,
	0x00, 0x00, 0x88, 0x50, 0x20, 0x50, 0x88, 0x00, 0x00, 0x00, 0x88, 0x88, 0x88, 0x78, 0x08, 0x70,
	0x00, 0x00, 0xf8, 0x10, 0x20, 0x40, 0xf8, 0x00, 0x08, 0x10, 0x10, 0x20, 0x10, 0x10, 0x08, 0x00,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x10, 0x20, 0x20, 0x40, 0x00,
	0x00, 0x68, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x50, 0x20, 0x50, 0xa8, 0x50, 0x00, 0x00,
};

unsigned draw_color(unsigned r, unsigned g, unsigned b)
{
	if (video_is_graphics()) {
		adv_pixel c;
		video_pixel_make(&c, r, g, b);
		return c;
	} else if (video_is_text()) {
		return (r >> 7) | ((g >> 7) << 1) | ((b >> 7) << 2);
	}

	return 0;
}

void draw_char(int x, int y, char c, unsigned color)
{
	if (video_is_graphics()) {
		int i, j;
		unsigned char* p = draw_font + 8*(unsigned)c;
		x *= DRAW_FONTX;
		y *= DRAW_FONTY;
		for(i=0;i<DRAW_FONTY;++i) {
			for(j=0;j<DRAW_FONTX;++j) {
				if (p[i] & (1 << (7-j)))
					video_put_pixel(x+j, y+i, color);
				else
					video_put_pixel(x+j, y+i, DRAW_COLOR_BLACK);
			}
		}
	} else if (video_is_text()) {
		text_put(x, y, c, color);
	}
}

void draw_string(int x, int y, const char* s, unsigned color)
{
	unsigned len = strlen(s);
	while (len) {
		draw_char(x, y, *s, color);
		++s;
		--len;
		++x;
	}
}

void draw_text_fill(int x, int y, char c, int dx, unsigned color)
{
	while (dx>0) {
		text_put(x, y, c, color);
		--dx;
		++x;
	}
}

void draw_text_fillrect(int x, int y, char c, int dx, int dy, unsigned color)
{
	while (dy-->0) {
		draw_text_fill(x, y++, c, dx, color);
	}
}

void draw_text_left(int x, int y, int dx, const char* s, unsigned color)
{
	int len = strlen(s);
	int i;
	for(i=0;i<dx;++i) {
		if (i<len)
			text_put(x+i, y, s[i], color);
		else
			text_put(x+i, y, ' ', color);
	}
}

void draw_text_center(int x, int y, int dx, const char* s, unsigned color)
{
	int len = strlen(s);
	int pre;
	int post;
	if (len > dx)
		len = dx;

	pre = (dx - len + 1) / 2;
	post = dx - pre - len;

	draw_text_fill(x, y, ' ', pre, color);
	draw_text_left(x+pre, y, len, s, color);
	draw_text_fill(x+pre+len, y, ' ', post, color);
}

unsigned draw_text_string(int x, int y, const char* s, unsigned color)
{
	int len = strlen(s);
	if (x + len > text_size_x())
		len = text_size_x() - x;
	draw_text_left(x, y, len, s, color);
	return len;
}

int draw_text_para(int x, int y, int dx, int dy, const char* s, unsigned color)
{
	int iy = 0;
	int ix = 0;
	int needspace = 0;
	while (*s && iy<=dy) {
		unsigned l = strcspn(s, " \n");
		if (needspace) {
		if (ix<dx) {
				draw_text_fill(x+ix, y+iy, ' ', 1, color);
				++ix;
			} else {
				++iy;
				ix = 0;
			}
			needspace = 0;
		}
		if (ix+l>dx) {
			draw_text_fill(x+ix, y+iy, ' ', dx-ix, color);
			++iy;
			ix = 0;
			needspace = 0;
		}
		if (iy<=dy) {
			draw_text_left(x+ix, y+iy, l, s, color);
			ix += l;
			s += l;
			if (*s==' ') {
				++s;
				needspace = 1;
			} else if (*s=='\n') {
				draw_text_fill(x+ix, y+iy, ' ', dx-ix, color);
				++s;
				++iy;
				ix = 0;
				needspace = 0;
			}
		}
	}
	if (iy<=dy && ix<dx) {
		draw_text_fill(x+ix, y+iy, ' ', dx-ix, color);
		++iy;
	}
	return y+iy;
}

int draw_text_read(int x, int y, char* s, int dx, unsigned color)
{
	int maxlen = dx - 1;
	if (strlen(s) > maxlen) {
		s[maxlen] = 0;
	}

	while (1) {
		int userkey;
		int len = strlen(s);
		draw_text_string(x, y, s, color);
		draw_text_fill(x+len, y, ' ', dx-len, color);

		video_wait_vsync();

		target_idle();
		os_poll();

		userkey = inputb_get();

		switch (userkey) {
			case INPUTB_BACKSPACE :
			case INPUTB_DEL :
				if (len) {
					s[len-1] = 0;
				}
				break;
			case INPUTB_ENTER :
			case INPUTB_ESC :
				return userkey;
			default:
				if (len < maxlen) {
					if (userkey>=32 && userkey<=126) {
						s[len] = userkey;
						s[len+1] = 0;
					}
				}
		}
	}
}

/* Set the default color palette */
void draw_graphics_palette(void)
{
	if (video_index() == MODE_FLAGS_INDEX_PALETTE8) {
		video_index_packed_to_rgb(0);
	}
}

static void draw_graphics_ellipse(int xc, int yc, int a0, int b0, unsigned color)
{
	int x = 0;
	int y = b0;
	int a = a0;
	int b = b0;
	int as = a*a;
	int as2 = 2*as;
	int bs = b*b;
	int bs2 = 2*bs;
	int d = bs - as*b + as/4;
	int dx = 0;
	int dy = as2 * b;

	while (dx<dy) {
		video_put_pixel(xc+x, yc+y, color);
		video_put_pixel(xc-x, yc+y, color);
		video_put_pixel(xc+x, yc-y, color);
		video_put_pixel(xc-x, yc-y, color);
		if (d>0) {
			--y;
			dy -= as2;
			d -= dy;
		}
		++x;
		dx += bs2;
		d += bs + dx;
	}

	d += (3*(as-bs)/2 - (dx+dy)) / 2;

	while (y>=0) {
		video_put_pixel(xc+x, yc+y, color);
		video_put_pixel(xc-x, yc+y, color);
		video_put_pixel(xc+x, yc-y, color);
		video_put_pixel(xc-x, yc-y, color);
		if (d<0) {
			++x;
			dx += bs2;
			d += dx;
		}
		--y;
		dy -= as2;
		d += as - dy;
	}
}

/* Draw the animation screen */
void draw_graphics_animate(int s_x, int s_y, int s_dx, int s_dy, unsigned counter, int do_clear)
{
	if (video_is_graphics()) {
		adv_pixel c;

		int dx = s_dx / (4*8);
		int dy = s_dy / (3*8);

		int x = counter % ((s_dx - dx) * 2);
		int y = counter % ((s_dy - dy) * 2);

		if (x >= (s_dx - dx))
			x = (s_dx - dx) * 2 - x;
		if (y >= (s_dy - dy))
			y = (s_dy - dy) * 2 - y;

		assert(x + dx <= s_dx);
		assert(y + dy <= s_dy);

		/* draw bar */
		if (do_clear)
			video_pixel_make(&c, 0, 0, 0);
		else
			video_pixel_make(&c, 255, 255, 255);

		video_clear(s_x + x, s_y, dx, s_dy, c);
		video_clear(s_x, s_y+y, s_dx, dy, c);
	}
}

void draw_graphics_clear(void)
{
	if (video_is_graphics()) {
		video_clear(0, 0, video_size_x(), video_size_y(), 0);
	}
}

/* Draw a test calibration screen */
void draw_graphics_calib(int s_x, int s_y, int s_dx, int s_dy)
{
	int i, j, k, l;

	draw_graphics_palette();

	if (video_is_graphics()) {
		adv_pixel c;

		unsigned dx = s_dx*3/5;
		unsigned dy = s_dy*4/5;
		unsigned x = (s_dx-dx)/2;
		unsigned y = (s_dy-dy)/2;
		unsigned dyB = dy/3;
		unsigned dyM = dyB/2;
		unsigned dyS = dyB/4;

		/* draw background */
		video_pixel_make(&c, 128, 128, 128);
		video_clear(s_x, s_y, s_dx, s_dy, c);

		/* draw rows */
		video_pixel_make(&c, 255, 255, 255);
		for(i=0;i<15;++i)
			video_clear(s_x, s_y + s_dy*i/15+s_dy/30, s_dx, 1, c);

		/* draw columns */
		for(i=0;i<20;++i) {
			unsigned rx = s_dx*i/20+s_dx/40;
			for(j=0;j<video_size_y();++j)
				video_put_pixel(s_x+rx, s_y+j, c);
		}

		/* draw ramp */
		for(i=0;i<dx;++i) {
			switch (i*8/dx) {
				case 0 : video_pixel_make(&c, 255, 255, 255); break;
				case 1 : video_pixel_make(&c, 255, 255, 0); break;
				case 2 : video_pixel_make(&c, 0, 255, 255); break;
				case 3 : video_pixel_make(&c, 0, 255, 0); break;
				case 4 : video_pixel_make(&c, 255, 0, 255); break;
				case 5 : video_pixel_make(&c, 255, 0, 0); break;
				case 6 : video_pixel_make(&c, 0, 0, 255); break;
				case 7 : video_pixel_make(&c, 0, 0, 0); break;
			}
			for(j=0;j<dyB;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyB;

		for(i=0;i<dx;++i) {
			video_pixel_make(&c, i*255/dx, i*255/dx, i*255/dx);
			for(j=0;j<dyS;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyS;

		k = 0;
		l = 0;
		for(i=0;i<dx;++i) {
			if (k > i / (dx / 5)) {
				l = !l;
				k = 0;
			}
			++k;
			if  (l)
				video_pixel_make(&c, 0, 0, 0);
			else
				video_pixel_make(&c, 255, 255, 255);
			for(j=0;j<dyM;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyM;

		for(i=0;i<dx;++i) {
			if (i<dx/2)
				video_pixel_make(&c, i*255/(dx/2), 0, 0);
			else
				video_pixel_make(&c, 255, (i-dx/2)*255/(dx/2), (i-dx/2)*255/(dx/2));
			for(j=0;j<dyS;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyS;

		for(i=0;i<dx;++i) {
			if (i<dx/2)
				video_pixel_make(&c, 0, i*255/(dx/2), 0);
			else
				video_pixel_make(&c, (i-dx/2)*255/(dx/2), 255, (i-dx/2)*255/(dx/2));
			for(j=0;j<dyS;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyS;

		for(i=0;i<dx;++i) {
			if (i<dx/2)
				video_pixel_make(&c, 0, 0, i*255/(dx/2));
			else
				video_pixel_make(&c, (i-dx/2)*255/(dx/2), (i-dx/2)*255/(dx/2), 255);
			for(j=0;j<dyS;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyS;

		for(i=0;i<dx;++i) {
			switch (i*5/dx) {
				case 0 : video_pixel_make(&c, 0, 0, 0); break;
				case 1 : video_pixel_make(&c, 64, 64, 64); break;
				case 2 : video_pixel_make(&c, 128, 128, 128); break;
				case 3 : video_pixel_make(&c, 196, 196, 196); break;
				case 4 : video_pixel_make(&c, 255, 255, 255); break;
			}
			for(j=0;j<dyM;++j)
				video_put_pixel(s_x+x+i, s_y+y+j, c);
		}
		y += dyM;


		video_pixel_make(&c, 255, 255, 255);
		draw_graphics_ellipse(s_dx / 2,  s_dy / 2, s_dx * 3 / 9, s_dy * 4 / 9, c);
	}
}

/* Do a speed test */
unsigned draw_graphics_speed(int s_x, int s_y, int s_dx, int s_dy)
{
	draw_graphics_palette();

	if (video_is_graphics()) {
		unsigned i;
		target_clock_t start, stop, end;
		unsigned size, count;
		struct video_pipeline_struct pipeline;
		uint8* data;

		size = s_dx * s_dy * video_bytes_per_pixel();

		data = malloc(size);
		for(i=0;i<size;++i)
			data[i] = i;

		video_pipeline_init(&pipeline);

		video_pipeline_direct(&pipeline, s_dx, s_dy, s_dx, s_dy, s_dx*video_bytes_per_pixel(), video_bytes_per_pixel(), video_color_def(), 0);

		/* fill the cache */
		video_pipeline_blit(&pipeline, s_x, s_y, data);

		count = 0;
		start = target_clock();
		end = start + TARGET_CLOCKS_PER_SEC * 2;
		stop = start;
		while (stop < end) {
			video_pipeline_blit(&pipeline, s_x, s_y, data);
			++count;
			stop = target_clock();
		}

		video_pipeline_done(&pipeline);
		free(data);

		size = size * count * (double)TARGET_CLOCKS_PER_SEC / (stop-start);

		return size;
	}

	return 0;
}

/* Draw a test screen */
void draw_test_default(void)
{
	int i, j;

	int TEST_DELTA1 = 10;
	int TEST_DELTA2 = 2;

	draw_graphics_palette();

	if (video_is_graphics()) {
		adv_pixel c;

		/* draw out of screen data */
		video_pixel_make(&c, 255, 0, 0);
		for(i=0;i<video_size_x();++i)
			video_put_pixel(i, video_size_y(), c);
		for(j=0;j<video_size_y();++j)
			video_put_pixel(video_size_x(), j, c);

		/* draw background */
		video_pixel_make(&c, 0, 0, 0);
		video_clear(0, 0, video_size_x(), video_size_y(), c);

		/* draw border */
		video_pixel_make(&c, 255, 255, 255);
		for(i=0;i<video_size_x();++i) {
			video_put_pixel(i, 0, c);
			video_put_pixel(i, video_size_y()-1, c);
		}
		for(j=0;j<video_size_y();++j) {
			video_put_pixel(0, j, c);
			video_put_pixel(video_size_x()-1, j, c);
		}

		/* draw grid */
		video_pixel_make(&c, 128, 128, 128);
		for(i=TEST_DELTA1;i<video_size_x()-1;i += TEST_DELTA1)
			for(j=TEST_DELTA2;j<video_size_y()-1;j += TEST_DELTA2)
				video_put_pixel(i, j, c);
		for(j=TEST_DELTA1;j<video_size_y()-1;j += TEST_DELTA1)
			for(i=TEST_DELTA2;i<video_size_x()-1;i += TEST_DELTA2)
				video_put_pixel(i, j, c);

		/* draw line */
		video_pixel_make(&c, 196, 196, 196);
		for(i=1;i<video_size_x()/2 && i<video_size_y()/2;++i) {
			video_put_pixel(i, i, c);
			video_put_pixel(video_size_x()-i-1, i, c);
			video_put_pixel(i, video_size_y()-i-1, c);
			video_put_pixel(video_size_x()-i-1, video_size_y()-i-1, c);
		}

	} else if (video_is_text()) {
		/* draw out of screen data */
		for(i=0;i<text_size_x();++i) {
			video_put_char(i, text_size_y(), 'Û', DRAW_COLOR_RED);
		}
		for(j=0;j<text_size_y();++j) {
			video_put_char(text_size_x(), j, 'Û', DRAW_COLOR_RED);
		}

		/* draw border */
		for(i=0;i<text_size_x();++i) {
			video_put_char(i, 0, 'Û', DRAW_COLOR_GRAY);
			video_put_char(i, text_size_y()-1, 'Û', DRAW_COLOR_GRAY);
		}
		for(j=0;j<text_size_y();++j) {
			video_put_char(0, j, 'Û', DRAW_COLOR_GRAY);
			video_put_char(text_size_x()-1, j, 'Û', DRAW_COLOR_GRAY);
		}

		/* draw grid */
		for(i=1;i+1<text_size_x();++i) {
			for(j=1;j+1<text_size_y();++j) {
				video_put_char(i, j, 'ú', DRAW_COLOR_GRAY);
			}
		}
	}
}

/***************************************************************************/
/* Menu */

int draw_text_menu(int x, int y, int dx, int dy, void* data, int mac, entry_print print, entry_separator separator, int* abase, int* apos, int* akey)
{
	int done = 0;
	int base = 0;
	int pos = 0;
	int key = 0;

	if (abase && apos) {
		base = *abase;
		pos = *apos;
	}

	while (!done) {
		int i;
		int dir = 0;

		for(i=0;i<dy;++i) {
			if (base+i >= mac)
				draw_text_fill(x, y+i, ' ', dx, COLOR_NORMAL);
			else
				print(x, y+i, dx, data, base+i, pos == i);
		}

		video_wait_vsync();

		target_idle();
		os_poll();

		key = inputb_get();

		switch (key) {
			case INPUTB_SPACE:
				done = 1;
				break;
			case INPUTB_ENTER:
				done = 1;
				break;
			case INPUTB_ESC:
				done = 1;
				break;
			case INPUTB_DOWN :
				if (base + pos + 1 < mac) {
					if (pos + 1 < dy)
						++pos;
					else
						++base;
				}
				dir = 1;
				break;
			case INPUTB_PGDN :
				for(i=0;i<dy;++i)
				if (base + pos + 1 < mac) {
					if (pos + 1 < dy)
						++pos;
					else
						++base;
				}
				dir = 1;
				break;
			case INPUTB_UP :
				if (base + pos > 0) {
					if (pos > 0)
						--pos;
					else
						--base;
				}
				dir = 0;
				break;
			case INPUTB_PGUP :
				for(i=0;i<dy;++i)
				if (base + pos > 0) {
					if (pos > 0)
						--pos;
					else
						--base;
				}
				dir = 0;
				break;
		}

		while (separator && separator(data, base+pos)) {
			if (dir) {
				if (base + pos + 1 < mac) {
					if (pos + 1 < dy)
						++pos;
					else
						++base;
				} else
					dir = !dir;
			} else {
				if (base + pos > 0) {
					if (pos > 0)
						--pos;
					else
						--base;
				} else
					dir = !dir;
			}
		}
	}

	if (abase && apos) {
		*abase = base;
		*apos = pos;
	}

	if (akey) {
		*akey = key;
	}

	return key != INPUTB_ESC ? base + pos : -1;
}

