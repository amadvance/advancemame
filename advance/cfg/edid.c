/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2016 Andrea Mazzoleni
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

#include "advance.h"

int parse_edid(unsigned char* edid, unsigned size, adv_monitor* monitor, adv_generate* generate)
{
	unsigned i;
	unsigned char checksum;
	char serial[14];
	char descriptor[14];
	char name[14];
	unsigned h_min;
	unsigned h_max;
	unsigned v_min;
	unsigned v_max;
	unsigned p_max;
	int has_clock;
	unsigned p_clock;
	unsigned h_active;
	unsigned h_blank;
	unsigned h_frontporch;
	unsigned h_sync;
	unsigned v_active;
	unsigned v_blank;
	unsigned v_frontporch;
	unsigned v_sync;
	unsigned char flags;
	int has_preferred;

	/* it must be at least 128 bytes */
	if (size < 128)
		return -1;

	/* verify header */
	if (edid[0] != 0x00
		|| edid[1] != 0xff
		|| edid[2] != 0xff
		|| edid[3] != 0xff
		|| edid[4] != 0xff
		|| edid[5] != 0xff
		|| edid[6] != 0xff
		|| edid[7] != 0x00
	)
		return -1;

	/* verify schecksum */
	checksum = 0;
	for (i = 0; i < 128; ++i)
		checksum += edid[i];

	if (checksum != 0)
		return -1;

	log_std(("edid: size %u\n", size));

	for (i = 0; i + 8 <= size; i += 8) {
		log_std(("edid: %04x: %02x%02x%02x%02x%02x%02x%02x%02x\n", i,
			edid[i], edid[i + 1], edid[i + 2], edid[i + 3],
			edid[i + 4], edid[i + 5], edid[i + 6], edid[i + 7]));
	}

	log_std(("edid: vendor '%c%c%c'\n", (edid[8] >> 2 & 0x1f) + 'A' - 1, (((edid[8] & 0x3) << 3) | ((edid[9] & 0xe0) >> 5)) + 'A' - 1, (edid[9] & 0x1f) + 'A' - 1));

	i = edid[0xA] | (edid[0xB] << 8);
	log_std(("edid: ID product code %u, 0x%04x\n", i, i));
	i = edid[0xC] | (edid[0xD] << 8) | (edid[0xE] << 16) | (edid[0xF] << 24);
	log_std(("edid: ID serial %u, 0x%08x\n", i, i));

	if (edid[0x10] == 0)
		log_std(("edid: manufacture date not specified\n"));
	else if (edid[0x10] <= 54)
		log_std(("edid: manufactured week %u of %u\n", edid[0x10], edid[0x11] + 1990));
	else if (edid[0x10] != 0xff)
		log_std(("edid: manufactured in %u\n", edid[0x11] + 1990));
	else if (edid[0x10] == 0xff)
		log_std(("edid: model year %d\n", edid[0x11] + 1990));

	log_std(("edid: version %u.%u\n", edid[0x12], edid[0x13]));

	if (edid[0x14] & 0x80) {
		log_std(("edid: digital signal\n"));
		switch (edid[0x14] & 0xf) {
		case 0: log_std(("edid: digital interface not defined\n")); break;
		case 1: log_std(("edid: DVI\n")); break;
		case 2: log_std(("edid: HDMI-a\n")); break;
		case 3: log_std(("edid: HDMI-b\n")); break;
		case 4: log_std(("edid: MDDI\n")); break;
		case 5: log_std(("edid: DisplayPort\n")); break;
		}
		switch ((edid[0x14] >> 4) % 0x7) {
		case 0: log_std(("edid: color bit depth not defined\n")); break;
		case 1: log_std(("edid: 6 bits per color\n")); break;
		case 2: log_std(("edid: 8 bits per color\n")); break;
		case 3: log_std(("edid: 10 bits per color\n")); break;
		case 4: log_std(("edid: 12 bits per color\n")); break;
		case 5: log_std(("edid: 14 bits per color\n")); break;
		case 6: log_std(("edid: 16 bits per color\n")); break;
		}
	} else {
		log_std(("edid: analog signal\n"));
		log_std(("edid: separate H/V sync are%ssupported\n", edid[0x14] & 0x08 ? " " : " NOT "));
		log_std(("edid: composite sync on horizontal is%ssupported\n", edid[0x14] & 0x04 ? " " : " NOT "));
		log_std(("edid: composite sync on green is%ssupported\n", edid[0x14] & 0x02 ? " " : " NOT "));
		log_std(("edid: serration on vertical is%ssupported\n", edid[0x14] & 0x01 ? " " : " NOT "));
	}

	if (edid[0x15] == 0) {
		if (edid[0x16] == 0) {
			log_std(("edid: screen size and aspect ratio undefined\n"));
		} else {
			log_std(("edid: aspect ratio portrait 100:%u\n", edid[0x16] + 99));
		}
	} else {
		if (edid[0x16] == 0) {
			log_std(("edid: aspect ratio landscape %u:100\n", edid[0x16] + 99));
		} else {
			log_std(("edid: screen width %u cm\n", edid[0x15]));
			log_std(("edid: screen height %u cm\n", edid[0x16]));
		}
	}

	log_std(("edid: standy mode is%ssupported\n", edid[0x18] & 0x80 ? " " : " NOT "));
	log_std(("edid: suspend mode is%ssupported\n", edid[0x18] & 0x40 ? " " : " NOT "));
	log_std(("edid: active off (very low power) is%ssupported\n", edid[0x18] & 0x40 ? " " : " NOT "));
	if (edid[0x14] & 0x80) {
		switch ((edid[0x18] >> 2) & 0x3) {
		case 0: log_std(("edid: color encoding RGB 4:4:4\n")); break;
		case 1: log_std(("edid: color encoding RGB 4:4:4 & YCrCb 4:4:4\n")); break;
		case 2: log_std(("edid: color encoding RGB 4:4:4 & YCrCb 4:2:2\n")); break;
		case 3: log_std(("edid: color encoding RGB 4:4:4 & YCrCb 4:4:4 & YCrCb 4:2:2\n")); break;
		}
	} else {
		switch ((edid[0x18] >> 2) & 0x3) {
		case 0: log_std(("edid: monocrome or grayscale display\n")); break;
		case 1: log_std(("edid: RGB color display\n")); break;
		case 2: log_std(("edid: Non RGB color display\n")); break;
		case 3: log_std(("edid: display color type undefined\n")); break;
		}
	}
	log_std(("edid: sRGB is%sthe default color space\n", edid[0x18] & 0x4 ? " " : " NOT "));
	log_std(("edid: preferred timing does%sinclude the native pixel format and refresh\n", edid[0x18] & 0x2 ? " " : " NOT "));
	log_std(("edid: display is%scontinous frequency\n", edid[0x18] & 0x1 ? " " : " NOT "));

	serial[0] = 0;
	descriptor[0] = 0;
	name[0] = 0;
	h_min = 0;
	h_max = 0;
	v_min = 0;
	v_max = 0;
	p_max = 0;
	has_clock = 0;
	has_preferred = 0;
	for (i = 0x36; i < 0x7E; i += 0x12) {
		if (edid[i] != 0x00) {
			const char* f_interlace;
			const char* f_sync;
			const char* f_3d;

			p_clock = ((edid[i + 1] << 8) | edid[i]) * 10000;

			h_active = edid[i + 2] + ((edid[i + 4] & 0xf0) << 4);
			h_blank = edid[i + 3] + ((edid[i + 4] & 0x0f) << 8);
			v_active = edid[i + 5] + ((edid[i + 7] & 0xf0) << 4);
			v_blank = edid[i + 6] + ((edid[i + 7] & 0x0f) << 8);

			h_frontporch = edid[i + 8] | ((edid[i + 11] & 0xc0) << 2);
			h_sync = edid[i + 9] | ((edid[i + 11] & 0x30) << 4);
			v_frontporch = ((edid[i + 10] & 0xf0) >> 4) | ((edid[i + 11] & 0x0c) << 2);
			v_sync = (edid[i + 10] & 0x0f) | ((edid[i + 11] & 0x03) << 4);

			if (generate && !has_preferred) {
				generate->hactive = h_active;
				generate->hfront = h_frontporch;
				generate->hsync = h_sync;
				generate->hback = h_blank - h_frontporch - h_sync;
				generate->vactive = v_active;
				generate->vfront = v_frontporch;
				generate->vsync = v_sync;
				generate->vback = v_blank - v_frontporch - v_sync;
				generate_normalize(generate);
			}
			has_preferred = 1;

			flags = edid[17];

			if (flags & 0x80) {
				f_interlace = " interlace";
			} else {
				f_interlace = "";
			}

			if (flags & 0x10) {
				if (flags & 0x8) {
					switch ((flags >> 1) & 0x3) {
					case 0: f_sync = " -hsync -vsync"; break;
					case 1: f_sync = " +hsync -vsync"; break;
					case 2: f_sync = " -hsync +vsync"; break;
					case 3: f_sync = " +hsync +vsync"; break;
					default: f_sync = "";
					}
				} else {
					f_sync = " csync";
				}
			} else {
				if (flags & 0x8) {
					f_sync = " analog-bsync";
				} else {
					f_sync = " analog-csync";
				}
			}

			switch (((flags >> 4) & 0x6) | (flags & 0x1)) {
			default: f_3d = ""; break;
			case 2: f_3d = " 3d-seq-right-sync"; break;
			case 3: f_3d = " 3d-seq-left-sync"; break;
			case 4: f_3d = " 3d-2way-right-even"; break;
			case 5: f_3d = " 3d-2way-left-even"; break;
			case 6: f_3d = " 3d-4way"; break;
			case 7: f_3d = " 3d-side"; break;
			}

			log_std(("edid: modeline %g %u %u %u %u %u %u %u %u%s%s%s\n",
				(double)p_clock / 1E6,
				h_active, h_active + h_frontporch, h_active + h_frontporch + h_sync, h_active + h_blank,
				v_active, v_active + v_frontporch, v_active + v_frontporch + v_sync, v_active + v_blank,
				f_interlace,
				f_sync,
				f_3d
				));
		} else {
			unsigned j;
			unsigned v;
			unsigned h;

			switch (edid[i + 3]) {
			case 0xff:
				for (j = 0; j < 13; ++j) {
					char c = edid[i + 5 + j];
					if (c == 0x0a)
						break;
					serial[j] = c;
				}
				serial[j] = 0;
				log_std(("edid: serial '%s'\n", serial));
				break;
			case 0xfe:
				for (j = 0; j < 13; ++j) {
					char c = edid[i + 5 + j];
					if (c == 0x0a)
						break;
					descriptor[j] = c;
				}
				descriptor[j] = 0;
				log_std(("edid: descriptor '%s'\n", descriptor));
				break;
			case 0xfd:
				v_min = edid[i + 5];
				v_max = edid[i + 6];
				h_min = edid[i + 7];
				h_max = edid[i + 8];
				p_max = edid[i + 9] * 10;

				/* extra offset */
				v = edid[i + 4] & 0x3;
				h = (edid[i + 4] >> 2) & 0x3;
				if (v == 0x3)
					v_min += 255;
				if (v == 0x2 || v == 0x3)
					v_max += 255;
				if (h == 0x3)
					h_min += 255;
				if (h == 0x2 || h == 0x3)
					h_max += 255;

				if (monitor && !has_clock) {
					monitor->mode_mac = 1;
					/*
					 * Raspberry Pi when using the DPI interface support only
					 * some lower clocks as 19.2 MHz, 9.6 MHz, 6.4 MHz, and 4.8 MHz.
					 * To allow the 4.8 MHz value we must use a lower 4 MHz limit.
					 */
					monitor->mode_map[0].pclock.low = 4 * 1E6;
					monitor->mode_map[0].pclock.high = p_max * 1E6;
					monitor->mode_map[0].hclock.low = h_min * 1E3;
					monitor->mode_map[0].hclock.high = h_max * 1E3;
					monitor->mode_map[0].vclock.low = v_min;
					monitor->mode_map[0].vclock.high = v_max;
				}
				has_clock = 1;

				log_std(("edid: clock range %u / %u - %u / %u - %u\n", p_max, h_min, h_max, v_min, v_max));
				break;
			case 0xfc:
				for (j = 0; j < 13; ++j) {
					char c = edid[i + 5 + j];
					if (c == 0x0a)
						break;
					name[j] = c;
				}
				name[j] = 0;
				log_std(("edid: name '%s'\n", name));
				break;
			}
		}
	}

	return 0;
}

