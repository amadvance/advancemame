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

enum advance_t {
	advance_mame, advance_mess, advance_pac, advance_menu
} the_advance; /* The current operating mode */

unsigned x_from_y(unsigned y)
{
	unsigned x;
	x = y * 2 / 3;
	x = (x + 0xF) & ~0xF;
	if (x < 256)
		x = 256;
	return x;
}

/***************************************************************************/
/* Draw */

int draw_test(int x, int y, const char* s, adv_crtc* crtc, int modify)
{
	char buffer[256];

	draw_test_default();

	draw_string(x, y, s, DRAW_COLOR_WHITE);
	++y;

	/* draw info */
	if (modify) {
		++y;
		draw_string(x, y, "ARROWS      Center", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "i/k         Expand x/y", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "SHIFT + i/k Shrink x/y", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "ENTER       Accept", DRAW_COLOR_WHITE);
		++y;
		draw_string(x, y, "ESC         Abort without saving", DRAW_COLOR_WHITE);
	}
	++y;
	++y;
	snprintf(buffer, sizeof(buffer), "modeline %.2f %d %d %d %d %d %d %d %d", (double)crtc->pixelclock / 1E6, crtc->hde, crtc->hrs, crtc->hre, crtc->ht, crtc->vde, crtc->vrs, crtc->vre, crtc->vt);
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	snprintf(buffer, sizeof(buffer), "pclock  %6.2f [MHz]", crtc_pclock_get(crtc) / 1E6);
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;
	snprintf(buffer, sizeof(buffer), "hclock  %6.2f [kHz]", crtc_hclock_get(crtc) / 1E3);
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;
	snprintf(buffer, sizeof(buffer), "vclock  %6.2f [Hz]", crtc_vclock_get(crtc));
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	snprintf(buffer, sizeof(buffer), "hsync   %6.2f [us]", (double)(crtc->hre - crtc->hrs) * 1E6 / crtc_pclock_get(crtc));
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;
	snprintf(buffer, sizeof(buffer), "vsync   %6.2f [us]", (double)(crtc->vre - crtc->vrs) * 1E6 / crtc_hclock_get(crtc));
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);
	++y;

	if (crtc_is_doublescan(crtc))
		snprintf(buffer, sizeof(buffer), "scan     double");
	else if (crtc_is_interlace(crtc))
		snprintf(buffer, sizeof(buffer), "scan     interlace");
	else
		snprintf(buffer, sizeof(buffer), "scan     single");
	draw_string(x, y, buffer, DRAW_COLOR_WHITE);

	return y;
}

static void draw_text_bar(void)
{
	char buffer[256];
	unsigned i;

	snprintf(buffer, sizeof(buffer), " AdvanceCONFIG - " __DATE__);
	draw_text_left(0, 0, text_size_x(), buffer, COLOR_BAR);

	sncpy(buffer, sizeof(buffer), "");
	for(i=0;i<video_driver_vector_max();++i) {
		if (video_driver_vector_pos(i) != 0) {
			if (*buffer)
				sncat(buffer, sizeof(buffer), "/");
			sncat(buffer, sizeof(buffer), video_driver_vector_pos(i)->name);
		}
	}
	draw_text_left(text_size_x() - strlen(buffer), 0, strlen(buffer), buffer, COLOR_BAR);

	snprintf(buffer, sizeof(buffer), " ENTER Select  ESC Back");
	draw_text_left(0, text_size_y() - 1, text_size_x(), buffer, COLOR_BAR);
}

/***************************************************************************/
/* Command */

enum monitor_enum {
	monitor_pc,
	monitor_arcade_standard,
	monitor_arcade_extended,
	monitor_arcade_medium,
	monitor_arcade_vga,
	monitor_pal,
	monitor_ntsc,
	monitor_custom,
	monitor_previous
};

struct monitor_data_struct {
	enum monitor_enum type;
	const char* name;
	adv_generate generate;
};

int monitor_y;

static void entry_monitor(int x, int y, int dx, void* data, int n, int selected)
{
	struct monitor_data_struct* p = ((struct monitor_data_struct*)data) + n;

	draw_text_left(x, y, dx, p->name, selected ? COLOR_REVERSE : COLOR_NORMAL);

	if (selected) {
		if (p->type == monitor_custom) {
			draw_text_left(0, monitor_y, text_size_x(), "format = ? ? ? ? ? ? ? ?", COLOR_NORMAL);
		} else {
			char buffer[256];
			adv_generate generate;

			generate = p->generate;
			snprintf(buffer, sizeof(buffer), "format = %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f",
				generate.hactive, generate.hfront, generate.hsync, generate.hback,
				generate.vactive, generate.vfront, generate.vsync, generate.vback);
			draw_text_left(0, monitor_y, text_size_x(), buffer, COLOR_NORMAL);
		}
	}
}

static adv_error cmd_monitor(adv_conf* config, adv_generate* generate, enum monitor_enum* type, adv_generate_interpolate_set* interpolate)
{
	unsigned mac = 0;
	unsigned i;
	adv_error res;
	int y;

	struct monitor_data_struct data[16];

	if (generate_interpolate_load(config, interpolate)==0) {
		data[mac].type = monitor_previous;
		data[mac].name = "Previous saved values";
		data[mac].generate = interpolate->map[0].gen;
		++mac;
	}

	data[mac].type = monitor_custom;
	data[mac].name = "Custom";
	generate_default_vga(&data[mac].generate);
	++mac;

	data[mac].type = monitor_pc;
	data[mac].name = "PC Monitor (> 30 kHz)";
	generate_default_vga(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_standard;
	data[mac].name = "Arcade Standard CGA Resolution (15 kHz)";
	generate_default_atari_standard(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_extended;
	data[mac].name = "Arcade Extended Resolution (16.5 kHz)";
	generate_default_atari_extended(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_medium;
	data[mac].name = "Arcade Medium EGA Resolution (25 kHz)";
	generate_default_atari_medium(&data[mac].generate);
	++mac;

	data[mac].type = monitor_arcade_vga;
	data[mac].name = "Arcade VGA Resolution (31.5 kHz)";
	generate_default_atari_vga(&data[mac].generate);
	++mac;

	data[mac].type = monitor_pal;
	data[mac].name = "TV PAL (50 Hz)";
	generate_default_pal(&data[mac].generate);
	++mac;

	data[mac].type = monitor_ntsc;
	data[mac].name = "TV NTSC (60 Hz)";
	generate_default_ntsc(&data[mac].generate);
	++mac;

	for(i=0;i<mac;++i)
		generate_normalize(&data[i].generate);

	text_clear();
	draw_text_bar();

	y = 2;

	y = draw_text_para(0, y, text_size_x(), text_size_y()-6,
"Now you must select the format of video modes supported by your monitor. "
"A predefined set based on the most common monitors is available. "
"\n\n"
"If you know the Active, FrontPorch, Sync and BackPorch values "
"required by your monitor you can enter them directly with the Custom selection "
"(only for expert users)."
	, COLOR_LOW);

	monitor_y = ++y;
	++y;

	draw_text_string(0, ++y, "Select the video mode format:", COLOR_BOLD);
	++y;
	res = draw_text_menu(2, y, text_size_x() - 4, text_size_y() - 2 - y, &data, mac, entry_monitor, 0, 0, 0, 0);

	if (res<0)
		return -1;

	*type = data[res].type;
	*generate = data[res].generate;

	return 0;
}

static adv_error cmd_monitor_custom(adv_generate* generate)
{
	int done;
	char buffer[64];
	text_clear();
	draw_text_bar();
	draw_text_left(0, 2, text_size_x(), "format = ", COLOR_NORMAL);
	draw_text_para(0, 4, text_size_x(), text_size_y()-6,
"Enter the 8 format values for your monitor. "
"In order the Horz Active, Horz Front Porch, Horz Sync, Horz Back Porch, Vert Active, Vert Front Porch, Vert Sync, Vert Back Porch.\n"
"The values are normalized to sum 1.\n\n"
"For example:\n\n"
"640 16 96 48 480 10 2 33"
	, COLOR_LOW);

	*buffer = 0;
	done = 0;
	while (!done) {
		if (draw_text_read(9, 2, buffer, 64, COLOR_REVERSE)!=INPUTB_ENTER)
			return -1;
		done = generate_parse(generate, buffer) == 0;
		if (!done)
			sound_error();
	}

	return 0;
}

struct model_data_struct {
	const char* manufacturer;
	const char* model;
	adv_monitor monitor;
};

int model_y;

static void entry_model(int x, int y, int dx, void* data, int n, int selected)
{
	char buffer[256];
	struct model_data_struct* p = ((struct model_data_struct*)data) + n;

	if (p->model) {
		snprintf(buffer, sizeof(buffer), "  %s - %s", p->manufacturer, p->model);
		draw_text_left(x, y, dx, buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);

		if (selected) {
			sncpy(buffer, sizeof(buffer), "clock = ");
			monitor_print(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), &p->monitor);
			draw_text_left(0, model_y, text_size_x(), buffer, COLOR_NORMAL);
		}
	} else {
		snprintf(buffer, sizeof(buffer), "-- %s --", p->manufacturer);
		draw_text_left(x, y, dx, buffer, COLOR_LOW);
	}
}

static int entry_model_separator(void* data, int n)
{
	struct model_data_struct* p = ((struct model_data_struct*)data) + n;
	return p->model == 0;
}

extern const char* MONITOR[];

static adv_error cmd_model(adv_conf* config, adv_monitor* monitor)
{
	unsigned max = 100;
	struct model_data_struct* data = malloc(max*sizeof(struct model_data_struct));
	unsigned mac = 0;
	adv_error res;
	int y;
	const char** i;
	adv_monitor previous_monitor;

	if (monitor_load(config, &previous_monitor)==0) {
		data[mac].model = strdup("Previous");
		data[mac].manufacturer = strdup("Previous saved values - Read the values from your .cfg file");
		data[mac].monitor = previous_monitor;
		++mac;
	}

	data[mac].model = strdup("Custom");
	data[mac].manufacturer = strdup("Custom - SUGGESTED - Read the values in your monitor manual");
	memset(&data[mac].monitor, 0, sizeof(data[mac].monitor));
	++mac;

	i = (const char**)&MONITOR;
	while (*i) {
		char buffer[256];
		char* manufacturer;
		char* model;
		char* clock;

		sncpy(buffer, sizeof(buffer), *i);
		while (*buffer && isspace(buffer[strlen(buffer)-1]))
			buffer[strlen(buffer)-1] = 0;

		if (*buffer == '#') {
			if (mac < max) {
				data[mac].manufacturer = strdup(buffer + 2);
				data[mac].model = 0;
				++mac;
			}
		} else {
			manufacturer = strtok(buffer, ":");
			model = strtok(NULL, ":");
			clock = strtok(NULL, "");
			if (manufacturer && model && clock && mac < max) {
				data[mac].manufacturer = strdup(manufacturer);
				data[mac].model = strdup(model);
				if (monitor_parse(&data[mac].monitor, clock)!=0) {
					video_mode_restore();
					target_err("Invalid monitor specification %s:%s:%s.", manufacturer, model, clock);
					target_flush();
					exit(EXIT_FAILURE);
				}
				++mac;
			}
		}

		++i;
	}

	text_clear();
	draw_text_bar();

	y = 2;
	y = draw_text_para(0, y, text_size_x(), text_size_y()-6,
"Now you must select the clock range supported by your monitor and video card. "
"The strongly suggested choice is to check your monitor manual for the "
"supported vertical and horizontal clock range.\n\n"
"Some video boards are unable to use a low pclock like 5 MHz. "
"Try eventually to use an higher value like 10 or 12."
	, COLOR_LOW);

	model_y = ++y;
	++y;
	++y;
	++y;

	draw_text_string(0, ++y, "Select the model of your monitor:", COLOR_BOLD);

	++y;
	res = draw_text_menu(2, y, text_size_x() - 4, text_size_y() - 2 - y, data, mac, entry_model, entry_model_separator, 0, 0, 0);

	if (res<0)
		return -1;

	*monitor = data[res].monitor;

	return 0;
}

static adv_error cmd_model_custom(adv_monitor* monitor)
{
	int state;
	char buffer[256];

	text_clear();
	draw_text_bar();
	draw_text_left(0, 2, text_size_x(), "clock = ", COLOR_NORMAL);
	draw_text_para(0, 4, text_size_x(), text_size_y()-6,
"Enter the clock specification of your monitor using the format: 'PIXEL_CLOCK / HORZ_CLOCK / VERT_CLOCK'\n\n"
"The pixel_clock specification is in MHz, the horz_clock is in kHz, the vert_clock is in Hz.\n\n"
"Usually you can find these value in the last page of your monitor manual. "
"You can specify a clock range using '-'.\n"
"If your monitor is multistandard you can put multiple definitions separated with ';'.\n\n"
"If you don't know the pixel_clock, try using the range 5 - 150. "
"If your video board is unable to use a low pixel_clock of 5 MHz, try using an higher value like 10 or 12.\n"
"For example:\n\n"
"clock = 5-150 / 30.5-60 / 55-130 (Standard SVGA)\n"
"\nor\n\n"
"clock = 5-100 / 15.63 / 50 ; 5-100 / 15.75 / 60 (Multistandard PAL/NTSC)\n"
	, COLOR_LOW);

	*buffer = 0;
	state = 0;
	while (state >= 0 && state != 4) {
		draw_text_left(9, 2, 64, buffer, COLOR_NORMAL);
		switch (state) {
		case 0 :
			if (draw_text_read(9, 2, buffer, 64, COLOR_REVERSE)!=INPUTB_ENTER)
				state = -1;
			else
				state = 3;
			break;
		case 3 :
			if (monitor_parse(monitor, buffer) == 0) {
				state = 4;
			} else {
				state = 0;
				sound_error();
			}
		}
	}

	if (state < 0)
		return -1;
	else
		return 0;
}

static adv_error adjust(const char* msg, adv_crtc* crtc, unsigned index, const adv_monitor* monitor, adv_bool only_center)
{
	int done = 0;
	int modify = 1;
	int first = 1;
	int userkey = INPUTB_ESC;
	adv_crtc current = *crtc;

	double hclock = crtc->pixelclock / crtc->ht;

	adv_mode mode;

	while (!done) {
		if (modify) {
			mode_reset(&mode);
			if (crtc_adjust_clock(&current, monitor)==0
				&& crtc_is_valid(&current)
				&& crtc_clock_check(monitor, &current)
				&& video_mode_generate(&mode, &current, index)==0) {
				if (text_mode_set(&mode) != 0) {
					text_done();
					target_err("Error setting the calibration mode.\n");
					target_err("%s\n", error_get());
					target_flush();
					exit(EXIT_FAILURE);
				}
				*crtc = current;
				modify = 0;
				
				video_write_lock();
				draw_test(2, 2, msg, &current, 1);
				video_write_unlock(0, 0, 0, 0, 0);
			} else {
				if (first) {
					text_done();
					target_err("Error in the test mode.\n");
					target_err("%s\n", error_get());
					target_flush();
					exit(EXIT_FAILURE);
				}
				sound_error();
			}
		}

		first = 0;

		current = *crtc;

		target_idle();
		os_poll();

		userkey = inputb_get();

		if (only_center) {
			switch (userkey) {
			case INPUTB_LEFT :
			case INPUTB_RIGHT :
			case 'i' :
			case 'I' :
			case INPUTB_ENTER:
			case INPUTB_ESC:
			break;
			default:
				sound_warn();
				userkey = 0;
			}
		}

		switch (userkey) {
		case INPUTB_ENTER:
		case INPUTB_ESC:
			done = 1;
			break;
		case 'i' :
			current.ht -= current.ht % CRTC_HSTEP;
			current.ht -= CRTC_HSTEP;
			current.pixelclock = hclock * current.ht;
			modify = 1;
			break;
		case 'I' :
			current.ht -= current.ht % CRTC_HSTEP;
			current.ht += CRTC_HSTEP;
			current.pixelclock = hclock * current.ht;
			modify = 1;
			break;
		case 'K' :
			current.vde -= 1;
			modify = 1;
			break;
		case 'k' :
			current.vde += 1;
			modify = 1;
			break;
		case INPUTB_HOME :
			current.hre -= CRTC_HSTEP;
			modify = 1;
			break;
		case INPUTB_END :
			current.hre += CRTC_HSTEP;
			modify = 1;
			break;
		case INPUTB_PGUP :
			current.vre -= 1;
			modify = 1;
			break;
		case INPUTB_PGDN :
			current.vre += 1;
			modify = 1;
			break;
		case INPUTB_LEFT :
			current.hrs -= current.hrs % CRTC_HSTEP;
			current.hrs += CRTC_HSTEP;
			current.hre -= current.hre % CRTC_HSTEP;
			current.hre += CRTC_HSTEP;
			modify = 1;
			break;
		case INPUTB_RIGHT :
			current.hrs -= current.hrs % CRTC_HSTEP;
			current.hrs -= CRTC_HSTEP;
			current.hre -= current.hre % CRTC_HSTEP;
			current.hre -= CRTC_HSTEP;
			modify = 1;
			break;
		case INPUTB_DOWN :
			current.vrs -= 1;
			current.vre -= 1;
			modify = 1;
			break;
		case INPUTB_UP :
			current.vrs += 1;
			current.vre += 1;
			modify = 1;
			break;
		}
	}

	return userkey == INPUTB_ENTER ? 0 : -1;
}

static void adjust_fix(const char* msg, adv_crtc* crtc, unsigned index, const adv_monitor* monitor)
{
	adv_crtc current = *crtc;

	adv_mode mode;

	mode_reset(&mode);
	if (crtc_adjust_clock(&current, monitor)==0
		&& crtc_is_valid(&current)
		&& crtc_clock_check(monitor, &current)
		&& video_mode_generate(&mode, &current, index)==0) {
		if (text_mode_set(&mode) != 0) {
			text_done();
			target_err("Error setting the calibration mode.\n");
			target_err("%s\n", error_get());
			target_flush();
			exit(EXIT_FAILURE);
		}
		
		video_write_lock();
		draw_test(2, 2, msg, &current, 0);
		video_write_unlock(0, 0, 0, 0, 0);

		do {
			target_idle();
			os_poll();
		} while (inputb_get()==INPUTB_NONE);
	}
}

static adv_error cmd_adjust(const char* msg, adv_generate_interpolate* entry, const adv_generate* generate, const adv_monitor* monitor, unsigned y, unsigned index, double horz_clock, adv_bool only_center)
{
	adv_crtc crtc;
	unsigned x;
	adv_generate current_gen;

	current_gen = *generate;

	x = x_from_y(y);

	crtc_reset(&crtc);

	generate_crtc_hsize(&crtc, x, generate);
	generate_crtc_vsize(&crtc, y, generate);
	crtc_hclock_set(&crtc, horz_clock);

	if (crtc_adjust_clock(&crtc, monitor)!=0
		|| crtc_adjust_size(&crtc, monitor)!=0) {
		text_done();
		target_err("Calibration mode unsupported.\n");
		target_err("%s\n", error_get());
		target_flush();
		exit(EXIT_FAILURE);
	}

	/* recompute the horizontal size after crtc_adjust_size() */
	generate_crtc_htotal(&crtc, crtc.ht, generate);

	if (!crtc_clock_check(monitor, &crtc)) {
		text_done();
		target_err("Calibration mode unsupported.\n");
		target_err("%s\n", error_get());
		target_flush();
		exit(EXIT_FAILURE);
	}

	if (adjust(msg, &crtc, index, monitor, only_center)!=0)
		return -1;

	entry->hclock = crtc.pixelclock / crtc.ht;
	entry->gen.hactive = crtc.hde;
	entry->gen.hfront = crtc.hrs - crtc.hde;
	entry->gen.hsync = crtc.hre - crtc.hrs;
	entry->gen.hback = crtc.ht - crtc.hre;

	entry->gen.vactive = crtc.vde;
	entry->gen.vfront = crtc.vrs - crtc.vde;
	entry->gen.vsync = crtc.vre - crtc.vrs;
	entry->gen.vback = crtc.vt - crtc.vre;

	generate_normalize(&entry->gen);

	return 0;
}

struct monitor_corner {
	double hclock;
	unsigned y;
	int level;
};

unsigned monitor_corner_create(const adv_monitor* monitor, double vclock, struct monitor_corner* map, unsigned max, unsigned count)
{
	unsigned i;
	unsigned mac;

	mac = 0;
	for(i=0;i<monitor->mode_mac;++i) {
		const adv_monitor_mode* mode = &monitor->mode_map[i];
		double try_vclock;
		double try_y;

		if (vclock < mode->vclock.low)
			try_vclock = mode->vclock.low;
		else if (vclock > mode->vclock.high)
			try_vclock = mode->vclock.high;
		else
			try_vclock = vclock;

		if (count > 0) { /* low */
			double try_hclock = mode->hclock.low;
			int y = ceil(try_hclock / try_vclock);
			if (monitor_mode_hvclock_check(mode, try_hclock, try_hclock / y)
				&& mac < max) {
				map[mac].hclock = try_hclock;
				map[mac].y = y;
				map[mac].level = -1;
				++mac;
			}
		}

		if (mode->hclock.high != mode->hclock.low) {
			if (count > 2) { /* mid */
				double try_hclock = (mode->hclock.low + mode->hclock.high) / 2;
				int y = floor(try_hclock / try_vclock + 0.5);
				if (monitor_mode_hvclock_check(mode, try_hclock, try_hclock / y)
					&& mac < max) {
					map[mac].hclock = try_hclock;
					map[mac].y = y;
					map[mac].level = 0;
					++mac;
				}
			}

			if (count > 1) { /* high */
				double try_hclock = mode->hclock.high;
				int y = floor(try_hclock / try_vclock);
				if (monitor_mode_hvclock_check(mode, try_hclock, try_hclock / y)
					&& mac < max) {
					map[mac].hclock = try_hclock;
					map[mac].y = y;
					map[mac].level = 1;
					++mac;
				}
			}
		}
	}

	return mac;
}

static adv_error cmd_interpolate_set(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index, struct monitor_corner* corner_map, unsigned corner_mac)
{
	unsigned i;
	double visible_fraction;
	adv_generate current_gen;

	current_gen = *generate;

	visible_fraction = current_gen.vactive / (current_gen.vactive + current_gen.vfront + current_gen.vsync + current_gen.vback);

	interpolate->mac = 0;

	for(i=0;i<corner_mac;++i) {
		char buffer[256];
		int y;
		double hclock;
		adv_bool only_center;

		y = floor(corner_map[i].y * visible_fraction);
		hclock = corner_map[i].hclock;
		only_center = corner_map[i].level >= 0;

		if (only_center) {
			snprintf(buffer, sizeof(buffer), "Center the screen [%d/%d]", i+1, corner_mac);
		} else {
			snprintf(buffer, sizeof(buffer), "Center and resize the screen [%d/%d]", i+1, corner_mac);
		}

		if (cmd_adjust(buffer, interpolate->map + interpolate->mac, &current_gen, monitor, y, index, hclock, only_center)!=0) {
			text_reset();
			return -1;
		}

		/* restart always from the last setting */
		current_gen = interpolate->map[interpolate->mac].gen;

		++interpolate->mac;
		if (interpolate->mac == GENERATE_INTERPOLATE_MAX)
			break;
	}

	text_reset();

	return 0;
}

static adv_error cmd_interpolate_low(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index)
{
	struct monitor_corner corner_map[MONITOR_MODE_MAX * 1];
	unsigned corner_mac;

	corner_mac = monitor_corner_create(monitor, 60, corner_map, sizeof(corner_map)/sizeof(corner_map[0]), 1);

	return cmd_interpolate_set(interpolate, generate, monitor, index, corner_map, corner_mac);
}

static adv_error cmd_interpolate_lowhigh(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index)
{
	struct monitor_corner corner_map[MONITOR_MODE_MAX * 2];
	unsigned corner_mac;

	corner_mac = monitor_corner_create(monitor, 60, corner_map, sizeof(corner_map)/sizeof(corner_map[0]), 2);

	return cmd_interpolate_set(interpolate, generate, monitor, index, corner_map, corner_mac);
}

static adv_error cmd_interpolate_lowmidhigh(adv_generate_interpolate_set* interpolate, const adv_generate* generate, const adv_monitor* monitor, unsigned index)
{
	struct monitor_corner corner_map[MONITOR_MODE_MAX * 3];
	unsigned corner_mac;

	corner_mac = monitor_corner_create(monitor, 60, corner_map, sizeof(corner_map)/sizeof(corner_map[0]), 3);

	return cmd_interpolate_set(interpolate, generate, monitor, index, corner_map, corner_mac);
}

/***************************************************************************/
/* interpolate */

enum interpolate_enum {
	interpolate_done,
	interpolate_mode
};

struct interpolate_data_struct {
	adv_bool selected;
	unsigned type;
	adv_crtc crtc;
	unsigned x;
	unsigned y;
	adv_bool valid;
};

static void entry_interpolate(int x, int y, int dx, void* data, int n, adv_bool selected)
{
	char buffer[1024];
	struct interpolate_data_struct* p = ((struct interpolate_data_struct*)data) + n;
	double vclock;
	double hclock;
	const char* pivot;
	const char* range;

	switch (p->type) {
	case interpolate_mode :
		hclock = p->crtc.pixelclock / p->crtc.ht;
		vclock = hclock / p->crtc.vt;
		if (p->selected && p->valid)
			pivot = " - REFERENCE";
		else
			pivot = "";
		if (!p->valid)
			range = " - OUT OF RANGE";
		else
			range = "";
		snprintf(buffer, sizeof(buffer), "Mode %4dx%4d %.1f/%.1f %s%s", p->x, p->y, hclock / 1E3, vclock, pivot, range);
		break;
	case interpolate_done :
		sncpy(buffer, sizeof(buffer), "Done");
		break;
	}

	draw_text_left(x, y, dx, buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);
}

static void interpolate_create(struct interpolate_data_struct* data, unsigned mac, const adv_generate* generate, const adv_monitor* monitor, double vclock)
{
	unsigned i;

	for(i=0;i<mac;++i) {
		if (data[i].type == interpolate_mode) {
			adv_crtc* crtc = &data[i].crtc;
			unsigned x;
			unsigned y;

			crtc_reset(crtc);

			y = data[i].y;
			x = data[i].x;

			generate_crtc_hsize(crtc, x, generate);
			generate_crtc_vsize(crtc, y, generate);
			crtc_vclock_set(crtc, vclock);

			if (crtc_adjust_clock(crtc, monitor)!=0
				|| crtc_adjust_size(crtc, monitor)!=0) {
				data[i].valid = 0;
			} else {
				generate_crtc_htotal(crtc, crtc->ht, generate);
				data[i].valid = crtc_clock_check(monitor, crtc);
			}
		}
	}
}

static void interpolate_update(adv_generate_interpolate_set* interpolate, struct interpolate_data_struct* data, unsigned mac, const adv_generate* generate, const adv_monitor* monitor, double vclock)
{
	unsigned i;
	interpolate->mac = 0;

	for(i=0;i<mac;++i) {
		if (data[i].type == interpolate_mode && data[i].selected && data[i].valid) {
			if (interpolate->mac < GENERATE_INTERPOLATE_MAX) {
				adv_generate_interpolate* entry = &interpolate->map[interpolate->mac];
				const adv_crtc* crtc = &data[i].crtc;

				entry->hclock = crtc->pixelclock / crtc->ht;
				entry->gen.hactive = crtc->hde;
				entry->gen.hfront = crtc->hrs - crtc->hde;
				entry->gen.hsync = crtc->hre - crtc->hrs;
				entry->gen.hback = crtc->ht - crtc->hre;

				entry->gen.vactive = crtc->vde;
				entry->gen.vfront = crtc->vrs - crtc->vde;
				entry->gen.vsync = crtc->vre - crtc->vrs;
				entry->gen.vback = crtc->vt - crtc->vre;

				generate_normalize(&entry->gen);

				++interpolate->mac;
			} else {
				data[i].selected = 0;
			}
		}
	}

	/* update the list of crtc */
	if (interpolate->mac) {
		for(i=0;i<mac;++i) {
			if (data[i].type == interpolate_mode && !data[i].selected) {
				data[i].valid = generate_find_interpolate_multi(&data[i].crtc, data[i].x, data[i].y, data[i].x*2, data[i].y*2, data[i].x*3, data[i].y*3, data[i].x*4, data[i].y*4, vclock, monitor, interpolate, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN, GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK | GENERATE_ADJUST_VTOTAL)==0;
			}
		}
	} else {
		interpolate_create(data, mac, generate, monitor, vclock);
	}
}

static adv_error interpolate_test(const char* msg, adv_crtc* crtc, const adv_monitor* monitor, unsigned index)
{
	adv_error res;

	adv_mode mode;

	mode_reset(&mode);
	if (video_mode_generate(&mode, crtc, index)!=0) {
		return -1;
	}

	if (text_mode_set(&mode) != 0) {
		text_reset();
		return -1;
	}

	res = adjust(msg, crtc, index, monitor, 0);

	text_reset();

	return res;
}

/***************************************************************************/
/* Adjust msg */

enum adjust_enum {
	adjust_previous,
	adjust_low,
	adjust_lowhigh,
	adjust_lowmidhigh
};

struct adjust_data_struct {
	enum adjust_enum type;
};

static void entry_adjust(int x, int y, int dx, void* data, int n, adv_bool selected)
{
	char buffer[1024];
	struct adjust_data_struct* p = ((struct adjust_data_struct*)data) + n;

	switch (p->type) {
	case adjust_previous:
		snprintf(buffer, sizeof(buffer), "Previous centering settings");
		break;
	case adjust_low:
		snprintf(buffer, sizeof(buffer), "Manual Low centering - SUGGESTED - (low frequency settings)");
		break;
	case adjust_lowhigh:
		snprintf(buffer, sizeof(buffer), "Manual Low/High centering (low/high frequency settings)");
		break;
	case adjust_lowmidhigh:
		snprintf(buffer, sizeof(buffer), "Manual Low/Mid/High centering (low/mid/high frequency settings)");
		break;
	}

	draw_text_left(x, y, dx, buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);
}

adv_error cmd_adjust_msg(int type, enum adjust_enum* adjust_type)
{
	int y;
	struct adjust_data_struct data[6];
	unsigned mac;
	adv_error res;

	text_clear();
	draw_text_bar();

	y = draw_text_para(0, 2, text_size_x(), text_size_y()-7,
"The next step requires to center and to resize some test video modes.\n\n"
"You MUST use only the software control to change the size and the position "
"of the video mode. (You can eventually adjust only the FIRST video mode "
"with the monitor controls if these setting will be shared on all video modes. "
"Generally this doesn't happen with modern PC MultiSync monitors)"
"\n\n"
"If you can't correctly adjust the video modes you can't use the automatic "
"configuration of the Advance programs and you should follow the manual configuration."
"\n\n"
"When the image is centered and fit the whole screen press ENTER to go forward."
	, COLOR_LOW);

	mac = 0;

	if (type == monitor_previous) {
		data[mac].type = adjust_previous;
		++mac;
	}

	data[mac].type = adjust_low;
	++mac;

	data[mac].type = adjust_lowhigh;
	++mac;

	data[mac].type = adjust_lowmidhigh;
	++mac;

	draw_text_string(0, ++y, "Select the configuration method:", COLOR_BOLD);
	++y;

	res = draw_text_menu(2, y, text_size_x() - 4, text_size_y() - 2 - y, &data, mac, entry_adjust, 0, 0, 0, 0);

	if (res >= 0) {
		*adjust_type = data[res].type;
		return 0;
	} else {
		return -1;
	}
}

/***************************************************************************/
/* Test */

enum test_enum {
	test_mode,
	test_exit,
	test_save,
	test_custom_single,
	test_custom_double,
	test_custom_interlace,
	test_custom
};

struct test_data_struct {
	unsigned type;
	unsigned x;
	unsigned y;
};

static void entry_test(int x, int y, int dx, void* data, int n, adv_bool selected)
{
	char buffer[1024];
	struct test_data_struct* p = ((struct test_data_struct*)data) + n;

	switch (p->type) {
	case test_mode :
		snprintf(buffer, sizeof(buffer), "Test mode %4dx%4d (singlescan)", p->x, p->y);
		break;
	case test_save :
		snprintf(buffer, sizeof(buffer), "Save & Exit");
		break;
	case test_exit :
		snprintf(buffer, sizeof(buffer), "Exit without saving");
		break;
	case test_custom_single:
		snprintf(buffer, sizeof(buffer), "Test custom (singlescan) - Use only singlescan modes");
		break;
	case test_custom_double:
		snprintf(buffer, sizeof(buffer), "Test custom (doublescan) - Use only doublescan modes");
		break;
	case test_custom_interlace:
		snprintf(buffer, sizeof(buffer), "Test custom (interlace) - Use only interlace modes");
		break;
	case test_custom:
		snprintf(buffer, sizeof(buffer), "Test custom - Use the nearest available mode (like AdvanceMAME)");
		break;
	}

	draw_text_left(x, y, dx, buffer, selected ? COLOR_REVERSE : COLOR_NORMAL);
}

adv_error cmd_test_mode(adv_generate_interpolate_set* interpolate, const adv_monitor* monitor, int x, int y, double vclock, unsigned index, unsigned cap, adv_bool calib)
{
	adv_crtc crtc;

	adv_mode mode;

	if (generate_find_interpolate_multi(&crtc, x, y, x*2, y*2, x*3, y*3, x*4, y*4, vclock, monitor, interpolate, cap, GENERATE_ADJUST_EXACT | GENERATE_ADJUST_VCLOCK | GENERATE_ADJUST_VTOTAL)!=0) {
		return -1;
	}

	mode_reset(&mode);
	if (video_mode_generate(&mode, &crtc, index)!=0) {
		return -1;
	}

	if (text_mode_set(&mode) != 0) {
		text_reset();
		return -1;
	}

	if (calib) {
		draw_graphics_calib(0, 0, video_size_x(), video_size_y());
		
		do {
			target_idle();
			os_poll();
		} while (inputb_get()==INPUTB_NONE);
	} else {
		adjust_fix("Verify the mode", &crtc, index, monitor);
	}

	text_reset();

	return 0;
}

static adv_error cmd_test_custom(int* x, int* y, double* vclock)
{
	int state;
	char xbuffer[64];
	char ybuffer[64];
	char vbuffer[64];

	text_clear();
	draw_text_bar();
	draw_text_left(0, 2, text_size_x(), "Xsize [pixel] = ", COLOR_NORMAL);
	draw_text_left(0, 3, text_size_x(), "Ysize [pixel] = ", COLOR_NORMAL);
	draw_text_left(0, 4, text_size_x(), "Vclock [Hz]   = ", COLOR_NORMAL);
	draw_text_para(0, 6, text_size_x(), text_size_y()-7,
"Enter the resolution and the Vclock desiderated.\n\n"
	, COLOR_LOW);

	*xbuffer = 0;
	*ybuffer = 0;
	*vbuffer = 0;
	state = 0;
	while (state >= 0 && state != 4) {
		draw_text_left(16, 2, 64, xbuffer, COLOR_NORMAL);
		draw_text_left(16, 3, 64, ybuffer, COLOR_NORMAL);
		draw_text_left(16, 4, 64, vbuffer, COLOR_NORMAL);
		switch (state) {
		case 0 :
			if (draw_text_read(16, 2, xbuffer, 64, COLOR_REVERSE)!=INPUTB_ENTER)
				state = -1;
			else
				state = 1;
			break;
		case 1 :
			if (draw_text_read(16, 3, ybuffer, 64, COLOR_REVERSE)!=INPUTB_ENTER)
				state = 0;
			else
				state = 2;
			break;
		case 2 :
			if (draw_text_read(16, 4, vbuffer, 64, COLOR_REVERSE)!=INPUTB_ENTER)
				state = 1;
			else
				state = 3;
			break;
		case 3 :
			*x = atoi(xbuffer);
			*y = atoi(ybuffer);
			*vclock = atof(vbuffer);
			if (100 < *x && *x < 1920 && 100 < *y && *y < 1280 && 30 < *vclock && *vclock < 200) {
				state = 4;
			} else {
				state = 0;
				sound_error();
			}
		}
	}

	if (state < 0)
		return -1;
	else
		return 0;
}

static adv_error cmd_test(adv_generate_interpolate_set* interpolate, const adv_monitor* monitor, unsigned index)
{
	unsigned mac = 0;
	adv_error res;
	int ymin, ymax;
	int y;
	adv_generate generate;
	int base;
	int pos;
	struct test_data_struct data[128];

	ymin = 192;
	ymax = 768;

	mac = 0;

	data[mac].type = test_save;
	++mac;

	data[mac].type = test_exit;
	++mac;

	if ((VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN & video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0)) != 0) {
		data[mac].type = test_custom_single;
		++mac;
	}

	if ((VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN & video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0)) != 0) {
		data[mac].type = test_custom_double;
		++mac;
	}

	if ((VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE & video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0)) != 0) {
		data[mac].type = test_custom_interlace;
		++mac;
	}

	data[mac].type = test_custom;
	++mac;

	while (ymin <= ymax) {
		data[mac].type = test_mode;
		data[mac].y = ymin;
		data[mac].x = x_from_y(ymin);
		ymin += 16;
		++mac;
	}

	base = 0;
	pos = 0;
	do {
		text_clear();

		draw_text_bar();

		y = 2;

		y = draw_text_para(0, y, text_size_x(), text_size_y()-6,
"Now you can test various video modes. If they are correctly centered "
"and sized save the configuration and exit.\n"
"Otherwise you can go back pressing ESC or exit without saving and try "
"the manual configuration."
		, COLOR_LOW);

		draw_text_string(0, ++y, "Select the video mode to test:", COLOR_BOLD);
		++y;

		res = draw_text_menu(2, y, text_size_x() - 4, text_size_y() - 2 - y, &data, mac, entry_test, 0, &base, &pos, 0);
		if (res >= 0 && data[res].type == test_mode) {
			cmd_test_mode(interpolate, monitor, data[res].x, data[res].y, 60, index, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN, 1);
		}
		if (res >= 0 && data[res].type == test_custom_single) {
			int x, y;
			double vclock;
			if (cmd_test_custom(&x, &y, &vclock) == 0)
				cmd_test_mode(interpolate, monitor, x, y, vclock, index, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN, 0);
		}
		if (res >= 0 && data[res].type == test_custom_double) {
			int x, y;
			double vclock;
			if (cmd_test_custom(&x, &y, &vclock) == 0)
				cmd_test_mode(interpolate, monitor, x, y, vclock, index, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN, 0);
		}
		if (res >= 0 && data[res].type == test_custom_interlace) {
			int x, y;
			double vclock;
			if (cmd_test_custom(&x, &y, &vclock) == 0)
				cmd_test_mode(interpolate, monitor, x, y, vclock, index, VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE, 0);
		}
		if (res >= 0 && data[res].type == test_custom) {
			int x, y;
			double vclock;
			if (cmd_test_custom(&x, &y, &vclock) == 0)
				cmd_test_mode(interpolate, monitor, x, y, vclock, index, video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0), 0);
		}
	} while (res >= 0 && (data[res].type == test_mode || data[res].type == test_custom_single || data[res].type == test_custom_double || data[res].type == test_custom_interlace || data[res].type == test_custom));

	if (res<0)
		return -1;
	return 0;
}

/***************************************************************************/
/* Save */

void cmd_save(adv_conf* config, const adv_generate_interpolate_set* interpolate, const adv_monitor* monitor, int type)
{
	switch (the_advance) {
	case advance_mame :
	case advance_mess :
	case advance_pac :
		/* set the specific AdvanceMAME options to enable the generate mode. */
		conf_string_set(config, "", "display_mode", "auto");
		conf_string_set(config, "", "display_adjust", "generate_yclock");
		break;
	case advance_menu:
		break;
	}
	generate_interpolate_save(config, interpolate);
	monitor_save(config, monitor);
}

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
	adv_generate generate;
	adv_generate_interpolate_set interpolate;
	enum monitor_enum type;
	enum adjust_enum adjust_type;
	adv_monitor monitor;
	adv_monitor* monitor_loaded;
	int state;
	unsigned index;
	const char* opt_rc;
	adv_bool opt_log;
	adv_bool opt_logsync;
	const char* section_map[1];
	unsigned j;
	adv_conf* config = 0;
	unsigned bit_flag;
	adv_crtc_container mode;
	adv_crtc_container mode_unsorted;
	adv_crtc_container_iterator i;
	adv_error res;

	state = 0;
	index = 0;
	type = monitor_pc;
	opt_rc = 0;
	opt_log = 0;
	opt_logsync = 0;
	the_advance = advance_mame;

	config = conf_init();

	if (os_init(config)!=0) {
		target_err("Error initializing the OS support.\n");
		goto err;
	}

	video_reg(config, 1);
	video_reg_driver_all(config);

	inputb_reg(config, 1);
	inputb_reg_driver_all(config);

	monitor_register(config);
	crtc_container_register(config);
	generate_interpolate_register(config);
	conf_string_register(config, "display_mode");
	conf_string_register(config, "display_adjust");

	if (conf_input_args_load(config, 1, "", &argc, argv, error_callback, 0) != 0)
		goto err_os;

	for(j=1;j<argc;++j) {
		if (target_option_compare(argv[j], "rc") && j+1<argc) {
			opt_rc = argv[++j];
		} else if (target_option_compare(argv[j], "log")) {
			opt_log = 1;
		} else if (target_option_compare(argv[j], "logsync")) {
			opt_logsync = 1;
		} else if (target_option_compare(argv[j], "advmamec")) {
			the_advance = advance_mame;
		} else if (target_option_compare(argv[j], "advmessc")) {
			the_advance = advance_mess;
		} else if (target_option_compare(argv[j], "advpacc")) {
			the_advance = advance_pac;
		} else if (target_option_compare(argv[j], "advmenuc")) {
			the_advance = advance_menu;
		} else if (target_option_compare(argv[j], "bit") && j+1<argc) {
			unsigned bits = atoi(argv[++j]);
			switch (bits) {
			case 8 : index = MODE_FLAGS_INDEX_BGR8; break;
			case 15 : index = MODE_FLAGS_INDEX_BGR15; break;
			case 16 : index = MODE_FLAGS_INDEX_BGR16; break;
			case 24 : index = MODE_FLAGS_INDEX_BGR24; break;
			case 32 : index = MODE_FLAGS_INDEX_BGR32; break;
			default:
				target_err("Invalid bit depth %d.\n", bits);
				goto err_os;
			}
		} else {
			target_err("Unknown option %s.\n", argv[j]);
			goto err_os;
		}
	}

	if (!opt_rc) {
		switch (the_advance) {
		case advance_menu : opt_rc = file_config_file_home("advmenu.rc"); break;
		case advance_mame : opt_rc = file_config_file_home("advmame.rc"); break;
		case advance_mess : opt_rc = file_config_file_home("advmess.rc"); break;
		case advance_pac : opt_rc = file_config_file_home("advpac.rc"); break;
		}
	}

	if (conf_input_file_load_adv(config, 0, opt_rc, opt_rc, 1, 1, STANDARD, sizeof(STANDARD)/sizeof(STANDARD[0]), error_callback, 0) != 0)
		goto err_os;

	if (opt_log || opt_logsync) {
		const char* log = "advcfg.log";
		remove(log);
		log_init(log, opt_logsync);
        }

	log_std(("cfg: %s %s\n", __DATE__, __TIME__));

	section_map[0] = "";
	conf_section_set(config, section_map, 1);

	if (video_load(config, "") != 0) {
		target_err("Error loading the video options from the configuration file %s.\n", opt_rc);
		target_err("%s\n", error_get());
		goto err_os;
	}

	if (inputb_load(config) != 0) {
		target_err("%s\n", error_get());
		goto err_os;
	}

	/* NOTE: After this command all the target_err() string must */
	/* have \n\r at the end to ensure correct newline in graphics mode. */

	if (os_inner_init("AdvanceCFG") != 0) {
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

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CLOCK) == 0) {
		target_err("No video driver is able to program your video board.\n\r");
		troubleshotting();
		goto err_blit;
	}

	if (index == 0) {
		unsigned mask = video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0);
		if ((mask & VIDEO_DRIVER_FLAGS_MODE_BGR8) != 0)
			index = MODE_FLAGS_INDEX_BGR8;
		else if ((mask & VIDEO_DRIVER_FLAGS_MODE_BGR16) != 0)
			index = MODE_FLAGS_INDEX_BGR16;
		else if ((mask & VIDEO_DRIVER_FLAGS_MODE_BGR15) != 0)
			index = MODE_FLAGS_INDEX_BGR15;
		else if ((mask & VIDEO_DRIVER_FLAGS_MODE_BGR32) != 0)
			index = MODE_FLAGS_INDEX_BGR32;
		else if ((mask & VIDEO_DRIVER_FLAGS_MODE_BGR24) != 0)
			index = MODE_FLAGS_INDEX_BGR24;
		else {
			target_err("No valid bit depth supported.\n\r");
			goto err_blit;
		}
	}

	switch (index) {
	case MODE_FLAGS_INDEX_BGR8 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR8; break;
	case MODE_FLAGS_INDEX_BGR15 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR15; break;
	case MODE_FLAGS_INDEX_BGR16 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR16; break;
	case MODE_FLAGS_INDEX_BGR24 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR24; break;
	case MODE_FLAGS_INDEX_BGR32 : bit_flag = VIDEO_DRIVER_FLAGS_MODE_BGR32; break;
	default:
		target_err("Invalid bit depth specification.\n\r");
		goto err_blit;
	}

	if ((video_mode_generate_driver_flags(VIDEO_DRIVER_FLAGS_MODE_GRAPH_MASK, 0) & bit_flag) == 0) {
		target_err("The specified bit depth isn't supported.\n\r");
		target_err("Try another value with the -bit option. For example '-bit 16'\n\r");
		goto err_blit;
	}

	if (inputb_init() != 0) {
		target_err("%s\n\r", error_get());
		goto err_blit;
	}

	res = monitor_load(config, &monitor);
	if (res < 0) {
		target_err("Error loading the clock options from the configuration file %s\n\r", opt_rc);
		target_err("%s\n\r", error_get());
		goto err_blit;
	}
	if (res == 0) {
		monitor_loaded = &monitor;
	} else {
		monitor_loaded = 0;
	}

	/* load selected */
	crtc_container_init(&mode_unsorted);

	if (crtc_container_load(config, &mode_unsorted) != 0) {
		target_err("%s\n\r", error_get());
		goto err_input;
	}

	/* insert modes */
	crtc_container_insert_default_all(&mode_unsorted);

	crtc_container_init(&mode);
	for(crtc_container_iterator_begin(&i, &mode_unsorted);!crtc_container_iterator_is_end(&i);crtc_container_iterator_next(&i)) {
		adv_crtc* crtc = crtc_container_iterator_get(&i);
		crtc_container_insert_sort(&mode, crtc, crtc_compare);
	}
	crtc_container_done(&mode_unsorted);

	if (text_init(&mode, monitor_loaded) != 0) {
		goto err_mode;
	}

	if (inputb_enable(0) != 0) {
		goto err_text;
	}

	while (state >= 0 && state != 8) {
		switch (state) {
		case 0 :
			res = cmd_monitor(config, &generate, &type, &interpolate);
			if (res == -1)
				state = -1;
			else if (type == monitor_custom)
				state = 1;
			else
				state = 2;
			break;
		case 1 :
			res = cmd_monitor_custom(&generate);
			if (res == -1)
				state = 0;
			else if (type == monitor_custom)
				state = 3;
			else
				state = 2;
			break;
		case 2 :
			res = cmd_model(config, &monitor);
			if (res == -1)
				state = 0;
			else if (monitor_is_empty(&monitor))
				state = 3;
			else
				state = 4;
			break;
		case 3 :
			res = cmd_model_custom(&monitor);
			if (res == -1)
				state = 2;
			else
				state = 4;
			break;
		case 4 :
			res = cmd_adjust_msg(type, &adjust_type);
			if (res < 0)
				state = 2;
			else {
				if (adjust_type == adjust_previous) {
					state = 5;
				} else if (adjust_type == adjust_low) {
					res = cmd_interpolate_low(&interpolate, &generate, &monitor, index);
				} else if (adjust_type == adjust_lowhigh) {
					res = cmd_interpolate_lowhigh(&interpolate, &generate, &monitor, index);
				} else if (adjust_type == adjust_lowmidhigh) {
					res = cmd_interpolate_lowmidhigh(&interpolate, &generate, &monitor, index);
				}
				if (res >= 0)
					state = 5;
			}
			break;
		case 5 :
			res = cmd_test(&interpolate, &monitor, index);
			if (res == -1)
				state = 4;
			else
				state = 6;
			break;
		case 6 :
			cmd_save(config, &interpolate, &monitor, type);
			state = 8;
			break;
		}
	}

	log_std(("cfg: shutdown\n"));

	inputb_disable();

	text_done();

	crtc_container_done(&mode);

	inputb_done();

	adv_video_done();

	os_inner_done();

	log_std(("cfg: the end\n"));

	if (opt_log || opt_logsync) {
		log_done();
	}

	os_done();

	conf_save(config, 0, 0, error_callback, 0);

	conf_done(config);

	return EXIT_SUCCESS;

err_text:
	text_done();
err_mode:
	crtc_container_done(&mode);
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
	conf_done(config);
err:
	return EXIT_FAILURE;
}


