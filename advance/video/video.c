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

#include "lib.h"

static void help(void)
{
	cputs("AdvanceVIDEO by Andrea Mazzoleni v0.11 " __DATE__ "\r\n");
	cputs("Usage:\r\n");
	cputs("    video [/r] [/s MODE] [/l IMAGE] [/d] [/e]\r\n");
	cputs("Options:\r\n");
	cputs("    /r       call the video board reset BIOS function\r\n");
	cputs("    /s MODE  set a video mode (hex number)\r\n");
	cputs("             3=80 cols, 1=40 cols, 13=320x200\r\n");
	cputs("    /l IMAGE load the specified PCX 8 bit image\r\n");
	cputs("    /d       disable the hardware video output\r\n");
	cputs("    /e       enable the hardware video output\r\n");
}

/***************************************************************************/
/* video */

void video_put(uint8* line, unsigned y, unsigned width)
{
	if (y >= 200)
		return;
	if (width > 320)
		width = 320;
	memcpy(MK_FP(0xA000, y*320), line, width);
}

void palette_put(uint8* palette)
{
	unsigned i;
	outportb(0x3C8, 0);
	for(i=0;i<256;++i) {
		outportb(0x3C9, palette[0] >> 2);
		outportb(0x3C9, palette[1] >> 2);
		outportb(0x3C9, palette[2] >> 2);
		palette += 3;
	}
}

void palette_clear(void)
{
	unsigned i;
	outportb(0x3C8, 0);
	for(i=0;i<256*3;++i)
		outportb(0x3C9, 0);
}

void video_set(unsigned mode)
{
	asm mov ah, 0
	asm mov al, [byte ptr mode]
	asm int 10h
}

/***************************************************************************/
/* card */

void card_out(int edx, uint8 al)
{
	outportb(edx, al);
}

uint8 card_in(int edx)
{
	return inportb(edx);
}

int card_crtc_address_get(void)
{
	if((card_in(0x3CC) & 0x01) == 0)
		return 0x3B4;
	else
		return 0x3D4;
}

void card_crtc_set(uint8 __idx, uint8 data)
{
	card_out(card_crtc_address_get(),    __idx);
	card_out(card_crtc_address_get() + 1, data);
}

uint8 card_crtc_get(uint8 __idx)
{
	uint8 d0;
	card_out(card_crtc_address_get(), __idx);
	d0 = card_in(card_crtc_address_get() + 1);
	return d0;
}

void card_signal_disable(void)
{
	uint8 d0;

	d0 = card_crtc_get(0x17);
	d0 = d0 & 0x7F;
	card_crtc_set(0x17, d0);
}

void card_signal_enable(void)
{
	uint8 d0;

	d0 = card_crtc_get(0x17);
	d0 = d0 | 0x80;
	card_crtc_set(0x17, d0);
}

/***************************************************************************/
/* file */

uint8 file_map[256];
int file_mac;
int file_max;

uint8 file_get(int f)
{
	if (file_mac == file_max) {
		file_max = read(f, &file_map, sizeof(file_map));
		if (file_max == -1)
			file_max = 0;
		file_mac = 0;
	}
	if (file_mac == file_max)
		return 0;
	return file_map[file_mac++];
}

/***************************************************************************/
/* pcx */

struct pcx_header {
	uint8 manufacturer; /* Constant Flag, 10 = ZSoft .pcx */
	uint8 version; /* Version informatio */
	uint8 encoding; /* 1 = .PCX run length encoding */
	uint8 bits_per_pixel; /*  Number of bits to represent a pixel (per Plane) - 1, 2, 4, or 8 */
	uint16 x_min; /* Image Dimensions */
	uint16 y_min;
	uint16 x_max;
	uint16 y_max;
	uint16 hdpi; /* Horizontal Resolution of image in DPI */
	uint16 vdpi; /*	Vertical Resolution of image in DPI*  */
	uint8 colornmap[48]; /*	Color palette setting */
	uint8 reserverd;
	uint8 planes; /* Number of color planes  */
	uint16 bytes_per_line; /* Number of bytes to allocate for a scanline plane.  MUST be an EVEN number.  Do NOT calculate from Xmax-Xmin. */
	uint16 palette_info; /* 1 = Color/BW, 2 = Grayscale (ignored in PB IV/ IV +) */
	uint16 h_screen_size; /* Horizontal screen size in pixels */
	uint16 v_screen_size; /* Vertical screen size in pixels.  */
	uint8 filler[54];
};

struct pcx_decode_state {
	unsigned count;
	uint8 value;
};

static void pcx_decode(uint8* buffer, unsigned size, int f, struct pcx_decode_state* state, unsigned delta)
{
	while (size) {
		unsigned run;
		if (!state->count) {
			uint8 c = file_get(f);
			if ((c & 0xC0) == 0xC0) {
				state->count = (c & 0x3F);
				state->value = file_get(f);
			} else {
				state->count = 1;
				state->value = c;
			}
		}
		run = state->count;
		if (run > size)
			run = size;
		state->count -= run;
		size -= run;
		if (buffer) {
			while (run) {
				*buffer = state->value;
				buffer += delta;
				--run;
			}
		}
	}
}

uint8 pcx_buffer[1024];

int pcx_load(const char *filename)
{
	int f;
	struct pcx_header h;
	unsigned width, height, y;

	f = open(filename, O_RDONLY);
	if (f == -1) {
		goto out;
	}

	if (read(f, &h, sizeof(h))!=sizeof(h)) {
		goto out_close;
	}

	/* limitations */
	if (h.bits_per_pixel != 8) {
		goto out_close;
	}

	if (h.planes != 1) {
		goto out_close;
	}

	width = h.x_max - h.x_min + 1;
	height = h.y_max - h.y_min + 1;

	for(y=0;y<height;++y) {
		struct pcx_decode_state state;
		state.count = 0;
		pcx_decode(pcx_buffer, width, f, &state, 1);
		video_put(pcx_buffer, y, width);
		pcx_decode(0, h.bytes_per_line - width, f, &state, 0);
		if (state.count!=0)
			goto out_close;
	}

	if (file_get(f)!=12)
		goto out_close;

	/* load the palette */
	for(y=0;y<768;++y)
		pcx_buffer[y] = file_get(f);
	palette_put(pcx_buffer);

	close(f);
	return 0;

out_close:
	close(f);
out:
	return -1;
}

/***************************************************************************/
/* main */

int main(int argl, const char far* args)
{
	int arg_set = 0;
	int arg_load = 0;
	int arg_d = 0;
	int arg_e = 0;
	int arg_r = 0;
	char file[128];
	int mode;
	char far* tok;

	char arg[128];
	memcpy(arg, args, argl);
	arg[argl] = 0;

	tok = strtok(arg, " \t");
	while (tok) {
		if (optionmatch(tok, "s")) {
			arg_set = 1;
			tok = strtok(0, " \t");
			if (!tok) {
				cputs("Missing mode number\r\n");
				return EXIT_FAILURE;
			}
			mode = strtou(tok, 16);
		} else if (optionmatch(tok, "l")) {
			arg_load = 1;
			tok = strtok(0, " \t");
			if (!tok) {
				cputs("Missing image name\r\n");
				return EXIT_FAILURE;
			}
			strcpy(file, tok);
		} else if (optionmatch(tok, "d")) {
			arg_d = 1;
		} else if (optionmatch(tok, "e")) {
			arg_e = 1;
		} else if (optionmatch(tok, "r")) {
			arg_r = 1;
		} else {
			cputs("Unknown option ");
			cputs(tok);
			cputs("\r\n");
			return EXIT_FAILURE;
		}
		tok = strtok(0, " \t");
	}

	if (arg_r) {
		typedef void far (*func)(void);
		func f;
		f = (func)MK_FP(0xC000, 3);
		asm push ds
		asm push si
		asm push di
		asm push bp
		f();
		asm pop bp
		asm pop di
		asm pop si
		asm pop ds
	}

	if (arg_set)
		video_set(mode);

	if (arg_load) {
		video_set(0x13);
		if (pcx_load(file)!=0) {
			cputs("Error loading file ");
			cputs(file);
			cputs("\r\n");
			return EXIT_FAILURE;
		}
	}

	if (arg_d)
		card_signal_disable();

	if (arg_e)
		card_signal_enable();

	if (arg_set + arg_load + arg_d + arg_e + arg_r == 0)
		help();

	return EXIT_SUCCESS;
}