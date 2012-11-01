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

#include <math.h>

/***************************************************************************/
/* Common variable */

/* The current operating mode */
enum advance_t {
	advance_mame, advance_mess, advance_pac, advance_menu, advance_vbe, advance_vga, advance_videow
} the_advance;

/* The configuration */
adv_conf* the_config;

int the_mode_index = MODE_FLAGS_INDEX_PALETTE8;

/* Video mode container */
adv_crtc_container the_modes;

/* Container need to be saved */
adv_bool the_modes_modified;

/* Video monitor specifications */
adv_monitor the_monitor;

/* Video mode generator specifications */
adv_generate_interpolate_set the_interpolate;

/* GTF data */
adv_gtf the_gtf;

/***************************************************************************/
/* crtc */

static adv_bool crtc_select_by_addr(const adv_crtc* a, void* b)
{
	return a==b;
}

static adv_bool crtc_select_by_compare(const adv_crtc* a, void* _b)
{
	const adv_crtc* b = (const adv_crtc*)_b;
	return crtc_compare(a, b)==0;
}

/***************************************************************************/
/* Common information screens */

static void draw_text_help(void)
{
	int x = 0;
	int y = 0;
	int dx = text_size_x();
	int dy = text_size_y();

	text_clear();

	y = draw_text_para(x, y, dx, dy,
" HELP"
	, COLOR_REVERSE);

	y = draw_text_para(x, y, dx, dy-y,
"F2    Save the selected modes\n"
"F5    Create a new modeline (favourite modes with the specified size)\n"
"F6    Create a new modeline (favourite modes with the specified clock)\n"
"F7    Duplicate the current mode\n"
"F8    Set an arbitrary clock value\n"
"F9    Show a static test screen for the current video mode\n"
"F10   Show a dynamic test screen for the current video mode\n"
"SPACE Select/Unselect the current video mode\n"
"ENTER Test the current video mode\n"
"TAB   Rename the current video mode\n"
"ESC   Exit\n"
"\n"
"q/a   Increase the x/y Display End register (SHIFT to decrease)\n"
"w/s   Increase the x/y Blank Start register (SHIFT to decrease)\n"
"e/d   Increase the x/y Retrace Start register (SHIFT to decrease)\n"
"r/f   Increase the x/y Retrace End register (SHIFT to decrease)\n"
"t/g   Increase the x/y Blank End register (SHIFT to decrease)\n"
"y/h   Increase the x/y Total End register (SHIFT to decrease)\n"
"u/j   Change the polarization\n"
"i/k   Expand the x/y size (SHIFT to shrink)\n"
"v     Increase the pixel clock (SHIFT to decrease)\n"
"x/c   Change the scan line mode\n"
"n/m   Change the tv mode\n"
"\n"
"Press ESC"
	, COLOR_NORMAL);

	video_wait_vsync();

	do {
		target_idle();
		os_poll();
	} while (inputb_get()==INPUTB_NONE);
}

static void draw_text_error(void)
{
	int x = 0;
	int y = 0;
	int dx = text_size_x();
	int dy = text_size_y();
	
	text_clear();

	draw_text_center(x, y, dx,
"ERROR! An error occurred in your last action!"
	, COLOR_REVERSE);
	++y;

	++y;
	y = draw_text_para(x, y, dx, dy,
"Your action has generated an error. Probably you have requested an "
"unsupported feature by your hardware or software."
	, COLOR_NORMAL);

	if (*error_get()) {
		y = draw_text_para(x, y, dx, dy-y, "\nThe video software reports this error:", COLOR_NORMAL);
		log_std(("v: error \"%s\"\n", error_get()));
		y = draw_text_para(x, y, dx, dy-y, error_get(), COLOR_ERROR);
	}

	y = draw_text_para(x, y, dx, dy-y, "\nPress ESC", COLOR_NORMAL);

	video_wait_vsync();

	do {
		target_idle();
		os_poll();
	} while (inputb_get()==INPUTB_NONE);
}

/***************************************************************************/
/* Menu */

int menu_base;
int menu_rel;
int menu_rel_max;
int menu_base_max;
int menu_max;

static adv_crtc* menu_pos(int pos)
{
	if (pos < 0 || pos >= menu_max)
		return 0;

	return crtc_container_pos(&the_modes, pos);
}

static adv_crtc* menu_current(void)
{
	return menu_pos(menu_base + menu_rel);
}

static void menu_modify(void)
{
	the_modes_modified = 1;
}

static void menu_insert(adv_crtc* crtc)
{
	crtc->user_flags = MODE_FLAGS_USER_BIT0;

	the_modes_modified = 1;

	crtc_container_insert_sort(&the_modes, crtc, crtc_compare);

	menu_max = crtc_container_max(&the_modes);
}

static void menu_remove(adv_crtc* crtc)
{

	the_modes_modified = 1;

	crtc_container_remove(&the_modes, crtc_select_by_addr, crtc);

	menu_max = crtc_container_max(&the_modes);
}

static void menu_item_draw(int x, int y, int dx, int pos, adv_bool selected)
{
	char buffer[256];

	adv_crtc* crtc = menu_pos(pos);

	if (crtc) {
		char tag;
		unsigned pos;
		unsigned color;
		unsigned color_n;
		unsigned color_clock;
		char pfreq_buffer[8];
		char vfreq_buffer[8];
		char hfreq_buffer[8];

		if (selected) {
			if (crtc->user_flags & MODE_FLAGS_USER_BIT0) {
				tag = 'þ';
				color = COLOR_SELECTED_MARK;
				color_n = color;
			} else {
				tag = ' ';
				color = COLOR_SELECTED;
				color_n = color;
			}
		} else {
			if (crtc->user_flags & MODE_FLAGS_USER_BIT0) {
				tag = 'þ';
				if (crtc_is_fake(crtc) || crtc_clock_check(&the_monitor, crtc)) {
					color = COLOR_MARK;
					color_n = color;
				} else {
					color = COLOR_MARK;
					color_n = COLOR_MARK_BAD;
				}
			} else {
				tag = ' ';
				if (crtc_is_fake(crtc) || crtc_clock_check(&the_monitor, crtc)) {
					color = COLOR_NORMAL;
					color_n = color;
				} else {
					color = COLOR_NORMAL;
					color_n = COLOR_BAD;
				}
			}
		}

		snprintf(vfreq_buffer, sizeof(vfreq_buffer), "%7.2f", (double)crtc_vclock_get(crtc));

		snprintf(hfreq_buffer, sizeof(hfreq_buffer), "%7.2f", (double)crtc_hclock_get(crtc) / 1E3);

		snprintf(pfreq_buffer, sizeof(pfreq_buffer), "%7.2f", (double)crtc_pclock_get(crtc) / 1E6);

		snprintf(buffer, sizeof(buffer), " %c %4d %4d", tag, crtc->hde, crtc->vde);

		pos = 0;

		pos += draw_text_string(x + pos, y, buffer, color_n);

		pos += draw_text_string(x + pos, y, pfreq_buffer, color_n);
		pos += draw_text_string(x + pos, y, hfreq_buffer, color_n);
		pos += draw_text_string(x + pos, y, vfreq_buffer, color_n);

		snprintf(buffer, sizeof(buffer), " %s", crtc->name);

		draw_text_left(x + pos, y, dx - pos, buffer, color_n);
	} else {
		draw_text_fill(x, y, ' ', dx, COLOR_NORMAL);
	}
}

static void menu_draw(int x, int y, int dx, int dy)
{
	unsigned i;
	for(i=0;i<dy;++i) {
		if (menu_base + i < menu_max) {
			menu_item_draw(x, y+i, dx, menu_base + i, i == menu_rel);
		} else
			draw_text_fill(x, y+i, ' ', dx, COLOR_NORMAL);
	}
}

/***************************************************************************/
/* Draw information bars */

static void format_info(char* buffer0, char* buffer1, char* buffer2, unsigned size, adv_crtc* crtc)
{
	double HD, HF, HS, HB;
	double VD, VF, VS, VB;
	double f;

	HD = crtc->hde;
	HF = crtc->hrs - crtc->hde;
	HS = crtc->hre - crtc->hrs;
	HB = crtc->ht - crtc->hre;
	f = 1 / (HD+HF+HS+HB);
	HD *= f;
	HF *= f;
	HS *= f;
	HB *= f;

	VD = crtc->vde;
	VF = crtc->vrs - crtc->vde;
	VS = crtc->vre - crtc->vrs;
	VB = crtc->vt - crtc->vre;
	f = 1 / (VD+VF+VS+VB);
	VD *= f;
	VF *= f;
	VS *= f;
	VB *= f;

	snprintf(buffer0, size, "plz clock  dsen rtst rten totl  disp  front sync  back  pclock");
	snprintf(buffer1, size, "h%c %7.3f%5d%5d%5d%5d %6.3f%6.3f%6.3f%6.3f %8.4f", crtc_is_nhsync(crtc) ? '-' : '+', crtc_hclock_get(crtc) / 1E3, crtc->hde, crtc->hrs, crtc->hre, crtc->ht, HD, HF, HS, HB, crtc_pclock_get(crtc) / 1E6);
	snprintf(buffer2, size, "v%c %7.3f%5d%5d%5d%5d %6.3f%6.3f%6.3f%6.3f%s%s", crtc_is_nvsync(crtc) ? '-' : '+', crtc_vclock_get(crtc), crtc->vde, crtc->vrs, crtc->vre, crtc->vt, VD, VF, VS, VB, crtc_is_doublescan(crtc) ? " doublescan" : "", crtc_is_interlace(crtc) ? " interlace" : "");
}

static void draw_text_info(int x, int y, int dx, int dy, int pos)
{
	char buffer[3][256];

	adv_crtc* crtc = menu_pos(pos);

	if (crtc) {
		format_info(buffer[0], buffer[1], buffer[2], 256, crtc);
	} else {
		sncpy(buffer[0], sizeof(buffer[0]), "");
		sncpy(buffer[1], sizeof(buffer[1]), "");
		sncpy(buffer[2], sizeof(buffer[2]), "");
	}

	draw_text_left(x, y+0, dx, buffer[0], COLOR_INFO_TITLE);
	draw_text_left(x, y+1, dx, buffer[1], COLOR_INFO_NORMAL);
	draw_text_left(x, y+2, dx, buffer[2], COLOR_INFO_NORMAL);

	if (crtc) {
		if (crtc_is_fake(crtc)) {
			draw_text_left(x+dx-8, y, 8, " SYSTEM ", COLOR_INFO_NORMAL);
		} else if (!crtc_clock_check(&the_monitor, crtc)) {
			draw_text_left(x+dx-14, y, 14, " OUT OF RANGE ", COLOR_ERROR);
		} else {
			draw_text_left(x+dx-14, y, 14, " PROGRAMMABLE ", COLOR_INFO_NORMAL);
		}
	} else {
		draw_text_fillrect(x, y, ' ', dx, dy, COLOR_INFO_NORMAL);
	}
}

static void draw_text_index(int x, int y, int dx)
{
	int i;
	int pos = x;

	pos += draw_text_string(pos, y, "Color ", COLOR_TITLE);

	for(i=0;i<8;++i) {
		const char* text;
		unsigned index;
		unsigned color;
		switch (i) {
			default:
			case 0 : text = "text"; index = MODE_FLAGS_INDEX_TEXT; break;
			case 1 : text = "palette8"; index = MODE_FLAGS_INDEX_PALETTE8; break;
			case 2 : text = "bgr8"; index = MODE_FLAGS_INDEX_BGR8; break;
			case 3 : text = "bgr15"; index = MODE_FLAGS_INDEX_BGR15; break;
			case 4 : text = "bgr16"; index = MODE_FLAGS_INDEX_BGR16; break;
			case 5 : text = "bgr24"; index = MODE_FLAGS_INDEX_BGR24; break;
			case 6 : text = "bgr32"; index = MODE_FLAGS_INDEX_BGR32; break;
			case 7 : text = "yuy2"; index = MODE_FLAGS_INDEX_YUY2; break;
		}
		if (the_mode_index == index)
			color = COLOR_SELECTED;
		else
			color = COLOR_NORMAL;
		pos += draw_text_string(pos, y, " ", color);
		pos += draw_text_string(pos, y, text, color);
		pos += draw_text_string(pos, y, " ", color);
	}

	draw_text_fillrect(pos, y, ' ', dx - (pos - x), 1, COLOR_NORMAL);
}

static void draw_text_bar(int x, int by1, int by2, int dx)
{
	char buffer[256];
	unsigned i;

	snprintf(buffer, sizeof(buffer), " AdvanceVIDEO Config - " __DATE__);

	draw_text_left(x, by1, dx, buffer, COLOR_BAR);

	sncpy(buffer, sizeof(buffer), "");
	for(i=0;i<video_driver_vector_max();++i) {
		if (video_driver_vector_pos(i) != 0) {
			if (*buffer)
				sncat(buffer, sizeof(buffer), "/");
			sncat(buffer, sizeof(buffer), video_driver_vector_pos(i)->name);
		}
	}

	draw_text_left(x + dx - strlen(buffer), by1, strlen(buffer), buffer, COLOR_BAR);

	snprintf(buffer, sizeof(buffer), " #    x    y pclock hclock vclock name");
	draw_text_left(x, by1+2, dx, buffer, COLOR_TITLE);

	snprintf(buffer, sizeof(buffer), " F1 Help  F2 Save  SPACE Select  TAB Rename  ENTER Test  ESC Exit");
	draw_text_left(x, by2, dx, buffer, COLOR_BAR);
}

/***************************************************************************/
/* Test screen */

static int test_default_command(int x, int y)
{
	draw_string(x, y, "ENTER  Save & Exit", DRAW_COLOR_WHITE);
	++y;
	draw_string(x, y, "ESC    Exit", DRAW_COLOR_WHITE);
	++y;

	return y;
}

static int test_crtc(int x, int y, adv_crtc* crtc, int print_clock, int print_measured_clock, int print_key)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "Horz  Vert");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	if (print_clock) {
		snprintf(buffer, sizeof(buffer), "%4.1f %5.1f %sClock Requested [kHz Hz]", crtc_hclock_get(crtc) / 1E3, crtc_vclock_get(crtc), print_key ? "     " : "");
		draw_string(x, y, buffer, DRAW_COLOR_WHITE);
		++y;
	}

	if (print_measured_clock) {
		snprintf(buffer, sizeof(buffer), "     %5.1f %sClock Measured [Hz]", video_measured_vclock(), print_key ? "     " : "");
		draw_string(x, y, buffer, DRAW_COLOR_WHITE);
		++y;
	}

	snprintf(buffer, sizeof(buffer), "%4d  %4d %sDisplay End", crtc->hde, crtc->vde, print_key ? "[qa] " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	snprintf(buffer, sizeof(buffer), "%4d  %4d %sRetrace Start", crtc->hrs, crtc->vrs, print_key ? "[ed] " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;
	if (!(crtc->hde<=crtc->hrs)) {
		snprintf(buffer, sizeof(buffer), "HDE<=HRS");
		draw_string(x, y, buffer, DRAW_COLOR_RED);
		++y;
	}
	if (!(crtc->vde<=crtc->vrs)) {
		snprintf(buffer, sizeof(buffer), "VDE<=VRS");
		draw_string(x, y, buffer, DRAW_COLOR_RED);
		++y;
	}

	snprintf(buffer, sizeof(buffer), "%4d  %4d %sRetrace End", crtc->hre, crtc->vre, print_key ? "[rf] " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;
	if (!(crtc->hrs<crtc->hre)) {
		snprintf(buffer, sizeof(buffer), "HRS<HRE");
		draw_string(x, y, buffer, DRAW_COLOR_RED);
		++y;
	}
	if (!(crtc->vrs<crtc->vre)) {
		snprintf(buffer, sizeof(buffer), "VRE<VRE");
		draw_string(x, y, buffer, DRAW_COLOR_RED);
		++y;
	}

	snprintf(buffer, sizeof(buffer), "%4d  %4d %sTotal", crtc->ht, crtc->vt, print_key ? "[yh] " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;
	if (!(crtc->hre<=crtc->ht)) {
		snprintf(buffer, sizeof(buffer), "HRE<=HT");
		draw_string(x, y, buffer, DRAW_COLOR_RED);
		++y;
	}
	if (!(crtc->vre<=crtc->vt)) {
		snprintf(buffer, sizeof(buffer), "VRE<=VT");
		draw_string(x, y, buffer, DRAW_COLOR_RED);
		++y;
	}

	snprintf(buffer, sizeof(buffer), "   %c     %c %sPolarization", crtc_is_nhsync(crtc) ? '-' : '+', crtc_is_nvsync(crtc) ? '-' : '+', print_key ? "[uj] " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	snprintf(buffer, sizeof(buffer), "      %4s %sDoublescan", crtc_is_doublescan(crtc) ? "on" : "off", print_key ? "[x]  " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	snprintf(buffer, sizeof(buffer), "      %4s %sInterlaced", crtc_is_interlace(crtc) ? "on" : "off", print_key ? "[c]  " : "");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	if (print_clock) {
		snprintf(buffer, sizeof(buffer), "%4.2f      %sPixelclock [MHz]", (double)crtc->pixelclock / 1E6, print_key ? "[v]  " : "");
		draw_string(x, y, buffer, DRAW_COLOR_WHITE);
		++y;
	}

	if (print_key) {
		++y;
		draw_string(x, y, "Q...U  Inc horz (SHIFT dec)", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "A...J  Inc vert (SHIFT dec)", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "I,K    Inc horz/vert size (SHIFT dec)", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "XCV    Flip flag", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "ARROWS Center", DRAW_COLOR_WHITE);
		++y;
	}

	return y;
}

#ifdef USE_VIDEO_VGALINE
static int test_vgaline(int x, int y, vgaline_video_mode* mode)
{
	char buffer[256];
	draw_test_default();

	if (video_is_text()) {
		snprintf(buffer, sizeof(buffer), "vgaline %s %dx%d %dx%d", index_name(video_index()), video_size_x(), video_size_y(), video_size_x() / video_font_size_x(), video_size_y() / video_font_size_y());
		draw_string(x, y, buffer, DRAW_COLOR_WHITE);
		++y;
	} else {
		snprintf(buffer, sizeof(buffer), "vgaline %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
		draw_string(x, y, buffer, DRAW_COLOR_WHITE);
		++y;
	}

	++y;

	y = test_crtc(x, y, &mode->crtc, 1, 1, 1);
	y = test_default_command(x, y);

	return y;
}
#endif

#ifdef USE_VIDEO_VBELINE
static int test_vbeline(int x, int y, vbeline_video_mode* mode)
{
	char buffer[256];
	vbe_ModeInfoBlock info;

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "vbeline %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	vbe_mode_info_get(&info, mode->mode);

	snprintf(buffer, sizeof(buffer), "based on vbe mode 0x%x %dx%dx%d", mode->mode, info.XResolution, info.YResolution, info.BitsPerPixel);
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	y = test_crtc(x, y, &mode->crtc, 1, 1, 1);
	y = test_default_command(x, y);
	return y;
}
#endif

#ifdef USE_VIDEO_SVGALINE
static int test_svgaline(int x, int y, svgaline_video_mode* mode)
{
	char buffer[256];

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "svgaline %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	y = test_crtc(x, y, &mode->crtc, 1, 1, 1);
	y = test_default_command(x, y);
	return y;
}
#endif

#ifdef USE_VIDEO_SVGAWIN
static int test_svgawin(int x, int y, svgawin_video_mode* mode)
{
	char buffer[256];

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "svgawin %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	y = test_crtc(x, y, &mode->crtc, 1, 1, 1);
	y = test_default_command(x, y);
	return y;
}
#endif


#ifdef USE_VIDEO_SVGALIB
static int test_svgalib(int x, int y, svgalib_video_mode* mode)
{
	char buffer[256];

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "svgalib %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	y = test_crtc(x, y, &mode->crtc, 1, 1, 1);
	y = test_default_command(x, y);
	return y;
}
#endif

#ifdef USE_VIDEO_FB
static int test_fb(int x, int y, fb_video_mode* mode)
{
	char buffer[256];

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "fb %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	y = test_crtc(x, y, &mode->crtc, 1, 1, 1);
	y = test_default_command(x, y);
	return y;
}
#endif

#ifdef USE_VIDEO_SDL
static int test_sdl(int x, int y, sdl_video_mode* mode)
{
	char buffer[256];

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "sdl %s %dx%dx%d [%dx%d]", index_name(video_index()), video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	y = test_default_command(x, y);
	return y;
}
#endif

#ifdef USE_VIDEO_VGA
static int test_vga(int x, int y, vga_video_mode* mode)
{
	char buffer[256];
	struct tweak_crtc info;
	adv_crtc crtc;

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "vga %s 0x%x %dx%dx%d [%dx%d]", index_name(video_index()), mode->mode, video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	tweak_crtc_get(&info, tweak_reg_read, 0);
	if (crtc_import(&crtc, &info, video_size_x(), video_size_y(), video_measured_vclock())==0) {
		++y;
		y = test_crtc(x, y, &crtc, 0, 1, 0);
	}

	++y;
	y = test_default_command(x, y);

	return y;
}
#endif

#ifdef USE_VIDEO_VBE
static int test_vbe(int x, int y, vbe_video_mode* mode)
{
	char buffer[256];
	struct vga_info info;
	struct vga_regs regs;
	adv_crtc crtc;

	draw_test_default();

	snprintf(buffer, sizeof(buffer), "vbe %s 0x%x %dx%dx%d [%dx%d]", index_name(video_index()), mode->mode, video_size_x(), video_size_y(), video_bits_per_pixel(), video_virtual_x(), video_virtual_y());
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	++y;

	vga_mode_get(&regs);
	vga_regs_info_get(&regs, &info);
	if (crtc_import(&crtc, &info, video_size_x(), video_size_y(), video_measured_vclock())==0) {
		++y;
		y = test_crtc(x, y, &crtc, 0, 1, 0);
	}

	++y;
	y = test_default_command(x, y);

	return y;
}
#endif

static int test_draw(int x, int y, adv_mode* mode)
{
	if (0) ;
#ifdef USE_VIDEO_VGA
	else if (video_current_driver() == &video_vga_driver)
		y = test_vga(x, y, (vga_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_VGALINE
	else if (video_current_driver() == &video_vgaline_driver)
		y = test_vgaline(x, y, (vgaline_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_SVGALINE
	else if (video_current_driver() == &video_svgaline_driver)
		y = test_svgaline(x, y, (svgaline_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_SVGAWIN
	else if (video_current_driver() == &video_svgawin_driver)
		y = test_svgawin(x, y, (svgawin_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_VBE
	else if (video_current_driver() == &video_vbe_driver)
		y = test_vbe(x, y, (vbe_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_VBELINE
	else if (video_current_driver() == &video_vbeline_driver)
		y = test_vbeline(x, y, (vbeline_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_SVGALIB
	else if (video_current_driver() == &video_svgalib_driver)
		y = test_svgalib(x, y, (svgalib_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_FB
	else if (video_current_driver() == &video_fb_driver)
		y = test_fb(x, y, (fb_video_mode*)mode->driver_mode);
#endif
#ifdef USE_VIDEO_SDL
	else if (video_current_driver() == &video_sdl_driver)
		y = test_sdl(x, y, (sdl_video_mode*)mode->driver_mode);
#endif
	return y;
}

static adv_bool test_exe_crtc(int userkey, adv_crtc* crtc)
{
	adv_bool modify = 0;
	unsigned pred_t;
	int xdelta;

	if (crtc->hde % 9 == 0 && crtc->hrs % 9 == 0 && crtc->hre % 9 == 0 && crtc->ht % 9 == 0)
		xdelta = 9;
	else
		xdelta = 8;

	switch (userkey) {
		case 'i' :
			pred_t = crtc->ht;
			crtc->ht -= crtc->ht % xdelta;
			crtc->ht -= xdelta;
			crtc->pixelclock = (double)crtc->pixelclock * crtc->ht / pred_t;
			modify = 1;
			break;
		case 'I' :
			pred_t = crtc->ht;
			crtc->ht -= crtc->ht % xdelta;
			crtc->ht += xdelta;
			crtc->pixelclock = (double)crtc->pixelclock * crtc->ht / pred_t;
			modify = 1;
			break;
		case 'k' :
			crtc->vt -= 1;
			modify = 1;
			break;
		case 'K' :
			crtc->vt += 1;
			modify = 1;
			break;
		case INPUTB_LEFT :
			crtc->hrs -= crtc->hrs % xdelta;
			crtc->hrs += xdelta;
			crtc->hre -= crtc->hre % xdelta;
			crtc->hre += xdelta;
			modify = 1;
			break;
		case INPUTB_RIGHT :
			crtc->hrs -= crtc->hrs % xdelta;
			crtc->hrs -= xdelta;
			crtc->hre -= crtc->hre % xdelta;
			crtc->hre -= xdelta;
			modify = 1;
			break;
		case INPUTB_DOWN :
			crtc->vrs -= 1;
			crtc->vre -= 1;
			modify = 1;
			break;
		case INPUTB_UP :
			crtc->vrs += 1;
			crtc->vre += 1;
			modify = 1;
			break;
		case 'u' :
		case 'U' :
			modify = 1;
			if (crtc_is_nhsync(crtc))
				crtc_phsync_set(crtc);
			else
				crtc_nhsync_set(crtc);
			break;
		case 'j' :
		case 'J' :
			modify = 1;
			if (crtc_is_nvsync(crtc))
				crtc_pvsync_set(crtc);
			else
				crtc_nvsync_set(crtc);
			break;
		case 'e' :
			crtc->hrs -= crtc->hrs % xdelta;
			crtc->hrs += xdelta;
			modify = 1;
			break;
		case 'E' :
			crtc->hrs -= crtc->hrs % xdelta;
			crtc->hrs -= xdelta;
			modify = 1;
			break;
		case 'r' :
			crtc->hre -= crtc->hre % xdelta;
			crtc->hre += xdelta;
			modify = 1;
			break;
		case 'R' :
			crtc->hre -= crtc->hre % xdelta;
			crtc->hre -= xdelta;
			modify = 1;
			break;
		case 'y' :
			crtc->ht -= crtc->ht % xdelta;
			crtc->ht += xdelta;
			modify = 1;
			break;
		case 'Y' :
			crtc->ht -= crtc->ht % xdelta;
			crtc->ht -= xdelta;
			modify = 1;
			break;
		case 'q' :
			crtc->hde -= crtc->hde % xdelta;
			crtc->hde += xdelta;
			modify = 1;
			break;
		case 'Q' :
			crtc->hde -= crtc->hde % xdelta;
			crtc->hde -= xdelta;
			modify = 1;
			break;
		case 'a' :
			++crtc->vde;
			modify = 1;
			break;
		case 'A' :
			--crtc->vde;
			modify = 1;
			break;
		case 'd' :
			++crtc->vrs;
			modify = 1;
			break;
		case 'D' :
			--crtc->vrs;
			modify = 1;
			break;
		case 'f' :
			++crtc->vre;
			modify = 1;
			break;
		case 'F' :
			--crtc->vre;
			modify = 1;
			break;
		case 'h' :
			++crtc->vt;
			modify = 1;
			break;
		case 'H' :
			--crtc->vt;
			modify = 1;
			break;
		case 'x' :
		case 'X' :
			modify = 1;
			if (crtc_is_doublescan(crtc))
				crtc_singlescan_set(crtc);
			else
				crtc_doublescan_set(crtc);
			break;
		case 'c' :
		case 'C' :
			modify = 1;
			if (crtc_is_interlace(crtc))
				crtc_singlescan_set(crtc);
			else
				crtc_interlace_set(crtc);
			break;
		case 'v' :
			modify = 1;
			crtc->pixelclock += 100000;
			break;
		case 'V' :
			modify = 1;
			crtc->pixelclock -= 100000;
			break;
	}
	return modify;
}

/***************************************************************************/
/* Menu command */

static void cmd_type(int key)
{
	if (key == INPUTB_RIGHT) {
		switch (the_mode_index) {
			case MODE_FLAGS_INDEX_TEXT : the_mode_index = MODE_FLAGS_INDEX_PALETTE8; break;
			case MODE_FLAGS_INDEX_PALETTE8 : the_mode_index = MODE_FLAGS_INDEX_BGR8; break;
			case MODE_FLAGS_INDEX_BGR8 : the_mode_index = MODE_FLAGS_INDEX_BGR15; break;
			case MODE_FLAGS_INDEX_BGR15 : the_mode_index = MODE_FLAGS_INDEX_BGR16; break;
			case MODE_FLAGS_INDEX_BGR16 : the_mode_index = MODE_FLAGS_INDEX_BGR24; break;
			case MODE_FLAGS_INDEX_BGR24 : the_mode_index = MODE_FLAGS_INDEX_BGR32; break;
			case MODE_FLAGS_INDEX_BGR32 : the_mode_index = MODE_FLAGS_INDEX_YUY2; break;
			case MODE_FLAGS_INDEX_YUY2 : the_mode_index = MODE_FLAGS_INDEX_TEXT; break;
		}
	} else {
		switch (the_mode_index) {
			case MODE_FLAGS_INDEX_TEXT : the_mode_index = MODE_FLAGS_INDEX_YUY2; break;
			case MODE_FLAGS_INDEX_PALETTE8 : the_mode_index = MODE_FLAGS_INDEX_TEXT; break;
			case MODE_FLAGS_INDEX_BGR8 : the_mode_index = MODE_FLAGS_INDEX_PALETTE8; break;
			case MODE_FLAGS_INDEX_BGR15 : the_mode_index = MODE_FLAGS_INDEX_BGR8; break;
			case MODE_FLAGS_INDEX_BGR16 : the_mode_index = MODE_FLAGS_INDEX_BGR15; break;
			case MODE_FLAGS_INDEX_BGR24 : the_mode_index = MODE_FLAGS_INDEX_BGR16; break;
			case MODE_FLAGS_INDEX_BGR32 : the_mode_index = MODE_FLAGS_INDEX_BGR24; break;
			case MODE_FLAGS_INDEX_YUY2 : the_mode_index = MODE_FLAGS_INDEX_BGR32; break;
		}
	}
}

static void cmd_select(void)
{
	adv_crtc* crtc;

	crtc = menu_current();
	if (!crtc)
		return;

	crtc->user_flags = crtc->user_flags ^ MODE_FLAGS_USER_BIT0;

	menu_modify();
}

static adv_error cmd_offvideo_test(int userkey)
{
	adv_crtc* crtc;
	adv_crtc crtc_save;
	adv_bool modify = 0;

	crtc = menu_current();
	if (!crtc)
		return -1;

	crtc_save = *crtc;

	modify = test_exe_crtc(userkey, crtc);

	if (!modify) {
		sound_warn();
		return 0;
	}

	menu_modify();

	if (strcmp(crtc->name, DEFAULT_TEXT_MODE)==0) {
		if (!crtc_clock_check(&the_monitor, crtc) && crtc_clock_check(&the_monitor, &crtc_save)) {
			*crtc = crtc_save;
			sound_error();
			return 0;
		}
		text_reset();
	}

	return 0;
}

static adv_error cmd_onvideo_test(void)
{
	adv_crtc* crtc;
	adv_mode mode;
	adv_bool done;
	adv_crtc crtc_save;
	adv_bool dirty = 1;
	adv_bool crtc_save_modified;

	mode_reset(&mode);

	crtc = menu_current();
	if (!crtc)
		return -1;

	if (!crtc_is_fake(crtc) && !crtc_clock_check(&the_monitor, crtc))
		return -1;

	if (video_mode_generate(&mode, crtc, the_mode_index)!=0) {
		return -1;
	}

	if (text_mode_set(&mode) != 0) {
		text_reset();
		return -1;
	}

	crtc_save = *crtc;
	crtc_save_modified = the_modes_modified;

	done = 0;
	while (!done) {
		int userkey;
		adv_bool modify = 0;

		adv_crtc last_crtc = *crtc;
		adv_mode last_mode = mode;
		adv_bool vm_last_modified = the_modes_modified;

		if (dirty) {
			video_write_lock();
			test_draw(1, 1, &mode);
			video_write_unlock(0, 0, 0, 0, 0);
			dirty = 0;
		}

		video_wait_vsync();

		target_idle();
		os_poll();

		userkey = inputb_get();

		switch (userkey) {
			case INPUTB_ESC :
				done = 1;
				/* restore */
				*crtc = crtc_save;
				the_modes_modified = crtc_save_modified;
				break;
			case INPUTB_ENTER :
				done = 1;
				break;
		}

		if (!done) {
			modify = test_exe_crtc(userkey, crtc);

			if (modify) {
				the_modes_modified = 1;
				dirty = 1;

				if ((crtc_is_fake(crtc) || crtc_clock_check(&the_monitor, crtc))
					&& video_mode_generate(&mode, crtc, the_mode_index)==0) {
					if (text_mode_set(&mode) != 0) {
						text_reset();
						/* abort */
						*crtc = crtc_save;
						the_modes_modified = crtc_save_modified;
						return -1;
					}
				} else {
					/* restore */
					mode = last_mode;
					*crtc = last_crtc;
					the_modes_modified = vm_last_modified;

					dirty = 1;
					sound_error();
				}
			} else {
				sound_warn();
			}
		}
	}

	return 0;
}

static adv_error cmd_onvideo_calib(void)
{
	adv_mode mode;
	adv_crtc* crtc;
	unsigned speed;
	char buffer[128];

	mode_reset(&mode);

	if (the_mode_index == MODE_FLAGS_INDEX_TEXT) {
		error_set("Command supported only in graphics mode");
		return -1;
	}

	crtc = menu_current();
	if (!crtc)
		return -1;

	if (!crtc_is_fake(crtc) && !crtc_clock_check(&the_monitor, crtc))
		return -1;

	if (video_mode_generate(&mode, crtc, the_mode_index)!=0) {
		return -1;
	}

	if (text_mode_set(&mode) != 0) {
		text_reset();
		return -1;
	}

	video_write_lock();

	draw_graphics_palette();
	/* draw_graphics_out_of_screen(0); */
	draw_graphics_clear();

	speed = draw_graphics_speed(0, 0, video_size_x(), video_size_y());
	draw_graphics_calib(0, 0, video_size_x(), video_size_y());

	snprintf(buffer, sizeof(buffer), " %.2f MB/s", speed / (double)(1024*1024));
	draw_string(0, 0, buffer, DRAW_COLOR_WHITE);

	video_write_unlock(0, 0, 0, 0, 0);

	video_wait_vsync();

	do {
		target_idle();
		os_poll();
	} while (inputb_get()==INPUTB_NONE);

	return 0;
}

static adv_error cmd_onvideo_animate(void)
{
	adv_mode mode;
	adv_crtc* crtc;
	unsigned i;
	int counter;

	mode_reset(&mode);

	if (the_mode_index == MODE_FLAGS_INDEX_TEXT) {
		error_set("Command supported only in graphics mode");
		return -1;
	}

	crtc = menu_current();
	if (!crtc)
		return -1;

	if (!crtc_is_fake(crtc) && !crtc_clock_check(&the_monitor, crtc))
		return -1;

	if (video_mode_generate(&mode, crtc, the_mode_index)!=0) {
		return -1;
	}

	if (text_mode_set(&mode) != 0) {
		text_reset();
		return -1;
	}

	update_init(2);

	draw_graphics_palette();

	for(i=0;i<3;++i) {
		update_start();
		video_clear(update_x_get(), update_y_get(), video_size_x(), video_size_y(), 0);
		update_stop(update_x_get(), update_y_get(), video_size_x(), video_size_y(), 1);
	}

	counter = update_page_max_get();
	
	while (!inputb_hit()) {
		os_poll();
		
		update_start();
		draw_graphics_animate(update_x_get(), update_y_get(), video_size_x(), video_size_y(), counter - update_page_max_get() + 1, 1);
		++counter;
		draw_graphics_animate(update_x_get(), update_y_get(), video_size_x(), video_size_y(), counter, 0);
		update_stop(update_x_get(), update_y_get(), video_size_x(), video_size_y(), 1);
	}

	update_done();

	do {
		target_idle();
		os_poll();
	} while (inputb_get()==INPUTB_NONE);

	return 0;
}

static void cmd_gotopos(int i)
{
	if (i >= menu_max)
		i = menu_max - 1;
	if (i<0)
		i = 0;
	if (menu_base <= i && i < menu_base + menu_rel_max) {
		menu_rel = i - menu_base;
	} else if (i<menu_base) {
		menu_base = i;
		menu_rel = 0;
	} else {
		menu_base = i - menu_rel_max + 1;
		if (menu_base<0)
			menu_base = 0;
		menu_rel = i - menu_base;
	}
}

static adv_error cmd_input_key(const char* tag, const char* keys)
{
	draw_text_fill(0, text_size_y()-1, ' ', text_size_x(), COLOR_REVERSE);
	draw_text_string(2, text_size_y()-1, tag, COLOR_REVERSE);
	draw_text_string(2+strlen(tag), text_size_y()-1, keys, COLOR_INPUT);

	while (1) {
		int i;
		unsigned k;

		video_wait_vsync();

		target_idle();
		os_poll();

		k = inputb_get();
		if (k == INPUTB_ESC)
			return -1;

		for(i=0;keys[i];++i)
			if (toupper(k)==toupper(keys[i]))
				return i;
	}
}

static adv_error cmd_input_string(const char* tag, char* buffer, unsigned length)
{
	draw_text_fill(0, text_size_y()-1, ' ', text_size_x(), COLOR_REVERSE);
	draw_text_string(2, text_size_y()-1, tag, COLOR_REVERSE);

	if (draw_text_read(2 + strlen(tag), text_size_y()-1, buffer, length, COLOR_INPUT) == INPUTB_ENTER) {
		return 0;
	}

	return -1;
}

static void cmd_rename(void)
{
	adv_crtc* crtc;
	char buffer[128];

	crtc = menu_current();
	if (!crtc)
		return;

	sncpy(buffer, sizeof(buffer), "");
	if (cmd_input_string(" Name : ", buffer, MODE_NAME_MAX)!=0)
		return;

	crtc_name_set(crtc, buffer);

	menu_modify();
}

static void cmd_copy(void)
{
	adv_crtc* crtc;
	adv_crtc copy;

	crtc = menu_current();
	if (!crtc)
		return;

	copy = *crtc;
	crtc_name_set(&copy, "duplicated");

	menu_insert(&copy);
}

static adv_error cmd_modeline_create(int favourite_vtotal)
{
	adv_crtc crtc;
	char buffer[80];
	double freq = 0;
	unsigned x;
	unsigned y;
	adv_error res;

	sncpy(buffer, sizeof(buffer), "");
	if (cmd_input_string(" Vertical clock [Hz] (example 60.0) : ", buffer, 10)!=0)
		return 0;
	freq = strtod(buffer, 0);
	if (freq < 10 || freq > 200) {
		error_set("Invalid vertical clock value, usually in the range 10 - 200.0 [Hz]");
		return -1;
	}

	sncpy(buffer, sizeof(buffer), "");
	if (cmd_input_string(" X resolution [pixel] : ", buffer, 10)!=0)
		return 0;
	x = atoi(buffer);
	if (x < 64 || x > 2048) {
		error_set("Invalid x resolution value");
		return -1;
	}

	sncpy(buffer, sizeof(buffer), "");
	if (cmd_input_string(" Y resolution [pixel] : ", buffer, 10)!=0)
		return 0;
	y = atoi(buffer);
	if (y < 64 || y > 2048) {
		error_set("Invalid y resolution value");
		return -1;
	}

	res = generate_find_interpolate_multi(&crtc, x, y, x*2, y*2, x*3, y*3, x*4, y*4, freq, &the_monitor, &the_interpolate, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC, GENERATE_ADJUST_EXACT);
	if (favourite_vtotal) {
		if (res != 0)
			res = generate_find_interpolate_multi(&crtc, x, y, x*2, y*2, x*3, y*3, x*4, y*4, freq, &the_monitor, &the_interpolate, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC, GENERATE_ADJUST_VCLOCK);
	} else {
		if (res != 0)
			res = generate_find_interpolate_multi(&crtc, x, y, x*2, y*2, x*3, y*3, x*4, y*4, freq, &the_monitor, &the_interpolate, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC, GENERATE_ADJUST_VTOTAL);
	}
	if (res != 0)
		res = generate_find_interpolate_multi(&crtc, x, y, x*2, y*2, x*3, y*3, x*4, y*4, freq, &the_monitor, &the_interpolate, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC, GENERATE_ADJUST_VCLOCK | GENERATE_ADJUST_VTOTAL);
	if (res != 0) {
		error_set("Request out of your monitor range");
		return -1;
	}

	menu_insert(&crtc);

	return 0;
}

static adv_error cmd_mode_clock(void)
{
	adv_crtc* crtc;
	char buffer[80];
	double freq = 0;

	int i = menu_base + menu_rel;
	if (i >= menu_max)
		return 0;

	crtc = crtc_container_pos(&the_modes, i);

	sncpy(buffer, sizeof(buffer), "");
	switch (cmd_input_key(" Set Vertical/Horizontal/Pixel clock ? ", "vhp")) {
		case 0 :
			if (cmd_input_string(" Vertical clock [Hz] (example 60.0) : ", buffer, 10)!=0)
				return 0;
			freq = strtod(buffer, 0);
			if (freq < 10 || freq > 200) {
				error_set("Invalid vertical clock value, usually in the range 10 - 200.0 [Hz]");
				return -1;
			}
			crtc->pixelclock = freq * crtc->ht * crtc->vt;
			if (crtc_is_interlace(crtc))
				crtc->pixelclock /= 2;
			if (crtc_is_doublescan(crtc))
				crtc->pixelclock *= 2;
			break;
		case 1 :
			if (cmd_input_string(" Horizontal clock [kHz] (example 31.5) : ", buffer, 10)!=0)
				return 0;
			freq = strtod(buffer, 0) * 1E3;
			if (freq < 10*1E3 || freq > 100*1E3) {
				error_set("Invalid horizontal clock value, usually in the range 15.0 - 80.0 [kHz]");
				return -1;
			}
			crtc->pixelclock = freq * crtc->ht;
			break;
		case 2 :
			if (cmd_input_string(" Pixel clock [MHz] (example 14.0) : ", buffer, 10)!=0)
				return 0;
			freq = strtod(buffer, 0) * 1E6;
			if (freq < 1*1E6 || freq > 300*1E6) {
				error_set("Invalid pixel clock value, usually in the range 1.0 - 300.0 [MHz]");
				return -1;
			}
			crtc->pixelclock = freq;
			break;
		default: return 0;
	}

	menu_modify();

	return 0;
}

static void cmd_del(void)
{
	adv_crtc* crtc;

	crtc = menu_current();
	if (!crtc)
		return;

	menu_remove(crtc);
}

static void cmd_save(void)
{
	adv_crtc_container selected;
	adv_crtc_container_iterator i;
	crtc_container_init(&selected);

	for(crtc_container_iterator_begin(&i, &the_modes);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		const adv_crtc* crtc = crtc_container_iterator_get(&i);
		if (crtc->user_flags & MODE_FLAGS_USER_BIT0)
			crtc_container_insert(&selected, crtc);
	}

	crtc_container_save(the_config, &selected);
	crtc_container_done(&selected);

	the_modes_modified = 0;
}

static adv_error cmd_exit(void)
{
	if (the_modes_modified) {
		adv_error res;

		sound_warn();

		res = cmd_input_key(" Save before exiting : ", "yn");
		if (res < 0)
			return 0;

		if (res == 0)
			cmd_save();
	}

	return 1;
}

/***************************************************************************/
/* Menu main */

/* Window coordinate */
#define MENU_X 0
#define MENU_Y 3
#define MENU_DX (text_size_x())
#define MENU_DY (text_size_y()-7)

#define INFO_X 0
#define INFO_Y (text_size_y()-4)
#define INFO_DX (text_size_x())
#define INFO_DY 3

#define BAR_X 0
#define BAR_Y1 0
#define BAR_Y2 (text_size_y()-1)
#define BAR_DX (text_size_x())

/* Menu */
static int menu_run(void)
{
	adv_bool done;
	int userkey;

	menu_base = 0;
	menu_rel = 0;
	menu_rel_max = MENU_DY;
	menu_max = crtc_container_max(&the_modes);
	menu_base_max = menu_max -  menu_rel_max;
	if (menu_base_max < 0)
		menu_base_max = 0;

	done = 0;
	while (!done) {

		draw_text_index(BAR_X, BAR_Y1+1, BAR_DX);
		draw_text_bar(BAR_X, BAR_Y1, BAR_Y2, BAR_DX);
		draw_text_info(INFO_X, INFO_Y, INFO_DX, INFO_DY, menu_base + menu_rel);
		menu_draw(MENU_X, MENU_Y, MENU_DX, MENU_DY);

		video_wait_vsync();

		target_idle();
		os_poll();

		userkey = inputb_get();

		switch (userkey) {
			case INPUTB_UP:
				cmd_gotopos(menu_base + menu_rel - 1);
				break;
			case INPUTB_DOWN:
				cmd_gotopos(menu_base + menu_rel + 1);
				break;
			case INPUTB_HOME: {
				int i = menu_base + menu_rel - 1;
				if (i<0)
					i = 0;
				while (i>0 && !(crtc_container_pos(&the_modes, i)->user_flags & MODE_FLAGS_USER_BIT0))
					--i;
				cmd_gotopos(i);
				break;
			}
			case INPUTB_END: {
				int i = menu_base + menu_rel + 1;
				if (i >= menu_max)
					i = menu_max - 1;
				while (i < menu_max - 1 && !(crtc_container_pos(&the_modes, i)->user_flags & MODE_FLAGS_USER_BIT0))
					++i;
				cmd_gotopos(i);
				break;
			}
			case INPUTB_PGDN:
				cmd_gotopos(menu_base + menu_rel + menu_rel_max);
				break;
			case INPUTB_PGUP:
				cmd_gotopos(menu_base + menu_rel - menu_rel_max);
				break;
			case INPUTB_F2:
				cmd_save();
				break;
			case INPUTB_LEFT :
			case INPUTB_RIGHT :
				cmd_type(userkey);
				break;
			case INPUTB_ESC:
				done = cmd_exit();
				break;
			case INPUTB_SPACE:
				cmd_select();
				cmd_gotopos(menu_base + menu_rel + 1);
				break;
			case INPUTB_ENTER:
				if (cmd_onvideo_test() != 0) {
					text_reset();
					draw_text_error();
				} else {
					text_reset();
				}
				break;
			case INPUTB_F9:
				if (cmd_onvideo_calib() != 0) {
					text_reset();
					draw_text_error();
				} else {
					text_reset();
				}
				break;
			case INPUTB_F10:
				if (cmd_onvideo_animate() != 0) {
					text_reset();
					draw_text_error();
				} else {
					text_reset();
				}
				break;
			case INPUTB_TAB :
				cmd_rename();
				break;
			case INPUTB_F5 :
				if (cmd_modeline_create(1) !=0) {
					text_reset();
					draw_text_error();
				} else {
					text_reset();
				}
				break;
			case INPUTB_F6 :
				if (cmd_modeline_create(0) !=0) {
					text_reset();
					draw_text_error();
				} else {
					text_reset();
				}
				break;
			case INPUTB_F7 :
				cmd_copy();
				break;
			case INPUTB_F8 :
				if (cmd_mode_clock() !=0) {
					text_reset();
					draw_text_error();
				} else {
					text_reset();
				}
				break;
			case INPUTB_DEL :
				cmd_del();
				cmd_gotopos(menu_base + menu_rel);
				break;
			case INPUTB_F1:
				draw_text_help();
				break;
			default:
				if (cmd_offvideo_test(userkey) != 0) {
					draw_text_error();
				}
				break;
		}
	}
	return userkey;
}

/***************************************************************************/
/* External utilities */

#ifdef __MSDOS__

#include <dpmi.h>

adv_bool the_advance_vbe_active; /* if AdvanceVBE is active */
adv_bool the_advance_vga_active; /* if AdvanceVGA is active */

/* RUThere code for int2f */
#define VBE_RUT_ACK1 0xAD17
#define VBE_RUT_ACK2 0x17BE
#define VGA_RUT_ACK1 0xAD17
#define VGA_RUT_ACK2 0x175A

static void msdos_rut(void)
{
	__dpmi_regs r;
	unsigned i;

	for(i=0xC0;i<=0xFF;++i) {
		r.h.ah = i;
		r.h.al = 0;
		r.x.bx = 0;
		r.x.cx = 0;
		r.x.dx = 0;
		__dpmi_int(0x2f, &r);
		if (r.h.al == 0xFF && r.x.cx == VBE_RUT_ACK1 && r.x.dx == VBE_RUT_ACK2) {
			the_advance_vbe_active = 1;
			break;
		}
	}

	for(i=0xC0;i<=0xFF;++i) {
		r.h.ah = i;
		r.h.al = 0;
		r.x.bx = 0;
		r.x.cx = 0;
		r.x.dx = 0;
		__dpmi_int(0x2f, &r);
		if (r.h.al == 0xFF && r.x.cx == VGA_RUT_ACK1 && r.x.dx == VGA_RUT_ACK2) {
			the_advance_vga_active = 1;
			break;
		}
	}
}

#endif

/***************************************************************************/
/* Main */

void adv_svgalib_log_va(const char *text, va_list arg)
{
	log_va(text, arg);
}

static void error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...)
{
	va_list arg;
	va_start(arg, desc);
	target_err_va(desc, arg);
	target_err("\n");
	if (valid)
		target_err("%s\n", valid);
	va_end(arg);
}

static adv_conf_conv STANDARD[] = {
{ "*", "*", "*", "%s", "%s", "%s", 1 }
};

void os_signal(int signum, void* info, void* context)
{
	os_default_signal(signum, info, context);
}

void troubleshotting(void)
{
	target_err("\n\r");
	target_err("Ensure to use the 'device_video_output auto' option.\n\r");
#ifdef USE_VIDEO_SVGAWIN
	target_err("Ensure to have installed the svgawin.sys driver with the svgawin.exe utility.\n\r");
#endif
#if defined(USE_VIDEO_FB) || defined(USE_VIDEO_SVGALIB)
	{
		char* term;
		if (getenv("DISPLAY") != 0) {
			target_err("Try to run this program in console mode and not in X.\n\r");
		} else {
#if defined(USE_VIDEO_FB)
			target_err("Ensure to have a Frame Buffer driver (other than VESA) in your Linux kernel.\n\r");
#endif
#if defined(USE_VIDEO_SVGALIB)
			target_err("Ensure to have a correctly installed and recent SVGALIB library.\n\r");
#endif
		}
		term = getenv("TERM");
		if (!term || strcmp(term, "linux")!=0)
			target_err("Try to run this program in a TERM=linux terminal.\n\r");
	}
#endif
}

int os_main(int argc, char* argv[])
{
	adv_crtc_container selected;
	adv_crtc_container_iterator i;
	const char* opt_rc;
	adv_bool opt_log;
	adv_bool opt_logsync;
	int j;
	adv_error res;
	const char* section_map[1];
	char buffer[1024];

	opt_rc = 0;
	opt_log = 0;
	opt_logsync = 0;
	the_advance = advance_mame;
	the_sound_flag = 1;

	the_config = conf_init();

	if (os_init(the_config)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err_conf;
	}

	video_reg(the_config, 1);
	monitor_register(the_config);
	crtc_container_register(the_config);
	generate_interpolate_register(the_config);
	gtf_register(the_config);
	inputb_reg(the_config, 1);
	inputb_reg_driver_all(the_config);
	
	/* MSDOS requires a special driver sub set */
#ifndef __MSDOS__
	video_reg_driver_all(the_config);
#endif

	if (conf_input_args_load(the_config, 1, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	for(j=1;j<argc;++j) {
		if (target_option_compare(argv[j], "rc") && j+1<argc) {
			opt_rc = argv[++j];
		} else if (target_option_compare(argv[j], "log")) {
			opt_log = 1;
		} else if (target_option_compare(argv[j], "logsync")) {
			opt_logsync = 1;
		} else if (target_option_compare(argv[j], "nosound")) {
			the_sound_flag = 0;
		} else if (target_option_compare(argv[j], "advmamev")) {
			the_advance = advance_mame;
		} else if (target_option_compare(argv[j], "advmessv")) {
			the_advance = advance_mess;
		} else if (target_option_compare(argv[j], "advpacv")) {
			the_advance = advance_pac;
		} else if (target_option_compare(argv[j], "advmenuv")) {
			the_advance = advance_menu;
#ifdef __MSDOS__
		} else if (target_option_compare(argv[j], "vgav")) {
			the_advance = advance_vga;
		} else if (target_option_compare(argv[j], "vbev")) {
			the_advance = advance_vbe;
#endif
#ifdef __WIN32__
		} else if (target_option_compare(argv[j], "videowv")) {
			the_advance = advance_videow;
#endif
		} else {
			target_err("Unknown option %s\n", argv[j]);
			goto err;
		}
	}

#ifdef __MSDOS__
	/* WARNING the MSDOS drivers are registered after the command line management. */
	/* It implyes that you cannot specify any driver options on the command line */
	msdos_rut();

	if (the_advance == advance_vga) {
		if (the_advance_vga_active) {
			target_err("The AdvanceVGA utility is active. Disable it before running vgav.\n");
			goto err;
		}
		video_reg_driver(the_config, &video_vgaline_driver);
	} else if (the_advance == advance_vbe) {
		if (the_advance_vbe_active) {
			target_err("The AdvanceVBE utility is active. Disable it before running vbev.\n");
			goto err;
		}
		video_reg_driver(the_config, &video_vbeline_driver);
		video_reg_driver(the_config, &video_vgaline_driver); /* for the text modes */
	} else {
		video_reg_driver_all(the_config);
	}
#endif

	if (!opt_rc) {
		switch (the_advance) {
			case advance_vbe : opt_rc = "vbe.rc"; break;
			case advance_vga : opt_rc = "vga.rc"; break;
			case advance_menu : opt_rc = file_config_file_home("advmenu.rc"); break;
			case advance_mame : opt_rc = file_config_file_home("advmame.rc"); break;
			case advance_mess : opt_rc = file_config_file_home("advmess.rc"); break;
			case advance_pac : opt_rc = file_config_file_home("advpac.rc"); break;
			case advance_videow : opt_rc = file_config_file_home("videow.rc"); break;
			default : opt_rc = "advv.rc"; break;
		}
	}

	if (access(opt_rc, R_OK)!=0) {
		target_err("Configuration file %s not found.\n", opt_rc);
		goto err_os;
	}

	if (conf_input_file_load_adv(the_config, 0, opt_rc, opt_rc, 1, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_os;

	if (opt_log || opt_logsync) {
		const char* log = "advv.log";
		remove(log);
		log_init(log, opt_logsync);
        }

	log_std(("v: %s %s\n", __DATE__, __TIME__));

	section_map[0] = "";
	conf_section_set(the_config, section_map, 1);

	if (video_load(the_config, "") != 0) {
		target_err("Error loading the video options from the configuration file %s.\n", opt_rc);
		target_err("%s\n", error_get());
		goto err_os;
	}

	if (inputb_load(the_config) != 0) {
		target_err("%s\n", error_get());
		goto err_os;
	}

	/* NOTE: After this command all the target_err() string must */
	/* have \n\r at the end to ensure correct newline in graphics mode. */

	if (os_inner_init("AdvanceVIDEO") != 0) {
		goto err_os;
	}

	if (adv_video_init() != 0) {
		target_err("%s\n\r", error_get());
		troubleshotting();
		goto err_os_inner;
	}

	if (video_blit_init() != 0) {
		target_err("%s\n\r", error_get());
		goto err_video;
	}

	if (the_advance != advance_vbe && the_advance != advance_vga) {
		if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) == 0) {
			target_err("No active video driver is able to program your video board.\n\r");
			troubleshotting();
			goto err_blit;
		}
	}

	if (inputb_init() != 0) {
		target_err("%s\n\r", error_get());
		goto err_blit;
	}

	if (monitor_load(the_config, &the_monitor) != 0) {
		target_err("Error loading the clock options from the configuration file %s.\n\r", opt_rc);
		target_err("%s\n\r", error_get());
		goto err_input;
	}

	monitor_print(buffer, sizeof(buffer), &the_monitor);
	log_std(("v: clock %s\n", buffer));

	/* load generate_linear config */
	res = generate_interpolate_load(the_config, &the_interpolate);
	if (res<0) {
		target_err("Error loading the format options from the configuration file %s.\n\r", opt_rc);
		target_err("%s\n\r", error_get());
		goto err_input;
	}
	if (res>0) {
		generate_default_vga(&the_interpolate.map[0].gen);
		the_interpolate.map[0].hclock = 31500;
		the_interpolate.mac = 1;
	}

	/* load generate_linear config */
	res = gtf_load(the_config, &the_gtf);
	if (res<0) {
		target_err("Error loading the gtf options from the configuration file %s.\n\r", opt_rc);
		target_err("%s\n\r", error_get());
		goto err_input;
	}
	if (res>0) {
		gtf_default_vga(&the_gtf);
	}

	/* all mode */
	crtc_container_init(&selected);

	/* insert modes */
	crtc_container_insert_default_all(&selected);

	/* sort */
	crtc_container_init(&the_modes);
	for(crtc_container_iterator_begin(&i, &selected);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		adv_crtc* crtc = crtc_container_iterator_get(&i);
		crtc_container_insert_sort(&the_modes, crtc, crtc_compare);
	}
	crtc_container_done(&selected);

	/* load selected */
	crtc_container_init(&selected);

	if (crtc_container_load(the_config, &selected) != 0) {
		target_err("%s\n\r", error_get());
		goto err_input;
	}

	/* union set */
	for(crtc_container_iterator_begin(&i, &selected);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		adv_crtc* crtc = crtc_container_iterator_get(&i);
		adv_bool has = crtc_container_has(&the_modes, crtc, crtc_compare) != 0;
		if (has)
			crtc_container_remove(&the_modes, crtc_select_by_compare, crtc);
		crtc->user_flags |= MODE_FLAGS_USER_BIT0;
		crtc_container_insert_sort(&the_modes, crtc, crtc_compare);
	}
	crtc_container_done(&selected);

	the_modes_modified = 0;

	if (text_init(&the_modes, &the_monitor) != 0) {
		goto err_input;
	}

	if (inputb_enable(0) != 0) {
		goto err_text;
	}

	sound_signal();

	menu_run();

	log_std(("v: shutdown\n"));

	inputb_disable();

	text_done();

	crtc_container_done(&the_modes);

	inputb_done();

	video_blit_done();

	adv_video_done();

	os_inner_done();

	log_std(("v: the end\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_done();

	conf_save(the_config, 0, 0, error_callback, 0);

	conf_done(the_config);

	return EXIT_SUCCESS;

err_text:
	text_done();
err_input:
	inputb_done();
err_blit:
	video_blit_done();
err_video:
	adv_video_done();
err_os_inner:
	os_inner_done();
err_os:
	if (opt_log || opt_logsync) {
		log_done();
	}
	os_done();
err_conf:
	conf_done(the_config);
err:
	return EXIT_FAILURE;
}

