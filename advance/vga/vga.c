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

#include "tsr.h"
#include "lib.h"

#include "card.h"

extern void far * int_10_old;

/***************************************************************************/
/* vga */

/* OEM information */
#define OEM_VENDOR_STR "Andrea Mazzoleni"
#define OEM_PRODUCT_STR "AdvanceVGA"
#define OEM_VERSION_NUM 0x000B
#define OEM_VERSION_STR "0.11 " __DATE__
#define OEM_STR OEM_VENDOR_STR " " OEM_PRODUCT_STR " " OEM_VERSION_STR

/* Free mode numbers */
#define VGA_CUSTOM_MODE 0x20

/* VGA frequencies */
#define VGA_DOTCLOCK_LOW 25175000
#define VGA_DOTCLOCK_HIGH 28322000

unsigned long VGA_TEXT_PIXELCLOCK[] = {
	VGA_DOTCLOCK_HIGH,
	VGA_DOTCLOCK_LOW,
	VGA_DOTCLOCK_HIGH/2,
	VGA_DOTCLOCK_LOW/2,
	0
};

unsigned long VGA_GRAPH_PIXELCLOCK[] = {
	VGA_DOTCLOCK_HIGH/2,
	VGA_DOTCLOCK_LOW/2,
	VGA_DOTCLOCK_HIGH/4,
	VGA_DOTCLOCK_LOW/4,
	0
};

/* VGA dotclock pivots */
#define VGA_DOTCLOCK_PIVOT_0 (VGA_DOTCLOCK_LOW/4 + VGA_DOTCLOCK_HIGH/4)
#define VGA_DOTCLOCK_PIVOT_1 (VGA_DOTCLOCK_LOW/2 + VGA_DOTCLOCK_HIGH/4)
#define VGA_DOTCLOCK_PIVOT_2 (VGA_DOTCLOCK_LOW/2 + VGA_DOTCLOCK_HIGH/2)

void vga_mode_set(const card_crtc* param)
{
	STACK_DECL card_crtc cp;
	unsigned pixel_per_clock;
	unsigned font_rows;
	int text_mode;

	memcpy(&cp, param, sizeof(card_crtc));

	if (card_graph_get(0x06) & 0x1)
		text_mode = 0;
	else
		text_mode = 1;

	if (text_mode) {
		/* set the correct font for a 80 columns */
		if (cp.HDisp < 360)
			pixel_per_clock = 8;
		else if (cp.HDisp < 500)
			pixel_per_clock = 9;
		else if (cp.HDisp < 720)
			pixel_per_clock = 8;
		else
			pixel_per_clock = 9;
	} else
		pixel_per_clock = 4;

	cp.HDisp = cp.HDisp * 8 / pixel_per_clock;
	cp.HBStart = cp.HBStart * 8 / pixel_per_clock;
	cp.HSStart = cp.HSStart * 8 / pixel_per_clock;
	cp.HSEnd = cp.HSEnd * 8 / pixel_per_clock;
	cp.HBEnd = cp.HBEnd * 8 / pixel_per_clock;
	cp.HTotal = cp.HTotal * 8 / pixel_per_clock;
	if (!text_mode)
		cp.dotclockHz = cp.dotclockHz * 8 / pixel_per_clock;

	if (text_mode) {
		/* set the correct font for a 25 rows */
		STACK_DECL regs_32 regs;
		font_rows = cp.VDisp / 25;
		if (font_rows < 13) {
			/* 8 rows */
			if (font_rows < 7)
				font_rows = 7;
			if (font_rows > 8)
				font_rows = 8;
			regs.x.ax = 0x1102;
			regs.x.bx = 0;
		} else if (font_rows < 15) {
			/* 14 rows */
			if (font_rows < 13)
				font_rows = 13;
			if (font_rows > 14)
				font_rows = 14;
			regs.x.ax = 0x1101;
			regs.x.bx = 0;
		} else {
			/* 16 rows */
			if (font_rows < 15)
				font_rows = 15;
			if (font_rows > 16)
				font_rows = 16;
			regs.x.ax = 0x1104;
			regs.x.bx = 0;
		}
		int_32_addr_call(int_10_old, &regs);
	}

	card_signal_disable();

	card_crtc_all_set(&cp);

	card_doublescan_set(0);
	if (text_mode) {
		card_char_size_x_set(pixel_per_clock);
		card_char_size_y_set(font_rows);
	} else {
		card_char_size_x_set(8);
		card_char_size_y_set(cp.doublescan ? 2 : 1);
	}
	card_polarity_set(cp.hpolarity, cp.vpolarity);

	if (cp.dotclockHz <= VGA_DOTCLOCK_PIVOT_0) {
		card_divider_set(2);
		card_pll_set(0);
	} else if (cp.dotclockHz <= VGA_DOTCLOCK_PIVOT_1) {
		card_divider_set(2);
		card_pll_set(1);
	} else if (cp.dotclockHz <= VGA_DOTCLOCK_PIVOT_2) {
		card_divider_set(1);
		card_pll_set(0);
	} else {
		card_divider_set(1);
		card_pll_set(1);
	}

	if (text_mode) {
		/* enable and adjust the cursor */
		uint16 start = font_rows - 2;
		uint16 end = font_rows - 1;
		card_crtc_set(0xA, start );
		card_crtc_set(0xB, end );

		/* DOS cursor shape */
		*(uint16 far*)MK_FP(0x40, 0x60) = start + (end << 8);

		/* DOS font height */
		*(uint16 far*)MK_FP(0x40, 0x85) = font_rows;
	}

	card_signal_enable();
}

/***************************************************************************/
/* mode */

#define MODE_MAX 24

typedef struct mode_info_struct {
	card_crtc param;
	unsigned effective_mode;
	unsigned reported_mode;

	unsigned width; /* Size of the mode */
	unsigned height;
} mode_info;

static mode_info mode_map[MODE_MAX];

/* number of video mode defined */
static unsigned mode_max;

/* active video mode */
static mode_info* mode_active;

static unsigned mode_number_generator = VGA_CUSTOM_MODE;

int is_clock(unsigned long* map, unsigned long clock)
{
	while (*map) {
		long err = *map - clock;
		if (err < 0)
			err = -err;
		if (err <= *map / 100) // 1% error acceptable
			return 1;
		++map;
	}
	return 0;
}

int is_graph_mode(unsigned x, unsigned y, unsigned long clock)
{
	if ((unsigned long)x * y > 256*1024UL)
		return 0;
	if (!is_clock(VGA_GRAPH_PIXELCLOCK, clock))
		return 0;

	return 1;
}

int is_text_mode(unsigned x, unsigned y, unsigned long clock)
{
	(void)x;

	switch (y) {
		case 8*25 :
		case 14*25 :
		case 16*25 :
			break;
		default:
			return 0;
	}
	if (!is_clock(VGA_TEXT_PIXELCLOCK, clock))
		return 0;

	return 1;
}

int is_mode_3(unsigned x, unsigned y, unsigned long clock)
{
	if (!is_text_mode(x, y, clock))
		return 0;

	switch (x) {
		case 80*8 :
		case 80*9 :
			return 1;
		default:
			return 0;
	}
}

int is_mode_1(unsigned x, unsigned y, unsigned long clock)
{
	if (!is_text_mode(x, y, clock))
		return 0;

	switch (x) {
		case 40*8 :
		case 40*9 :
			return 1;
		default:
			return 0;
	}
}

#define VGA_DOTCLOCK_LOW 25175000
#define VGA_DOTCLOCK_HIGH 28322000

int is_mode_13(unsigned x, unsigned y, unsigned long clock)
{
	return is_graph_mode(x, y, clock) && x == 320 && y == 200;
}

int is_less_mode(mode_info* a, mode_info* b)
{
	unsigned long sa = (unsigned long)a->param.HDisp * a->param.VDisp;
	unsigned long sb = (unsigned long)b->param.HDisp * b->param.VDisp;
	if (sa < sb)
		return 1;
	if (sa > sb)
		return 0;
	sa = (unsigned long)a->param.HTotal * a->param.VTotal;
	sb = (unsigned long)b->param.HTotal * b->param.VTotal;
	if (sa > sb)
		return 1;
	return 0;
}

const char* cfg_separator = " \t";

int modeline_load(char far* buffer, mode_info* mode_ptr)
{
	char far* s;

	s = strtok(buffer, cfg_separator);
	if (!s) return -1;

	if (strcmp(s, "device_video_modeline")!=0)
		return -1;

	/* skip the name */
	s = strtok(0, cfg_separator);
	if (!s) return -1;

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	if (!(s[0]>='0' && s[0]<='9'))
		return -1;
	mode_ptr->param.dotclockHz = strtold(s, 1000000L);
	if (!mode_ptr->param.dotclockHz)
		return -1;

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.HDisp = mode_ptr->width = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.HSStart = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.HSEnd = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.HTotal = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.VDisp = mode_ptr->height = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.VSStart = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.VSEnd = strtol(s, 10);

	s = strtok(0, cfg_separator);
	if (!s) return -1;
	mode_ptr->param.VTotal = strtol(s, 10);

	while ((s = strtok(0, cfg_separator))!=0) {
		if (s[0]=='#')
			break;
		else if (strcmp(s, "doublescan")==0)
			mode_ptr->param.doublescan = 1;
		else if (strcmp(s, "interlaced")==0 || strcmp(s, "interlace")==0)
			mode_ptr->param.interlace = 1;
		else if (strcmp(s, "-hsync")==0)
			mode_ptr->param.hpolarity = 1;
		else if (strcmp(s, "-vsync")==0)
			mode_ptr->param.vpolarity = 1;
	}

	/* convert the format */
	if (mode_ptr->param.doublescan) {
		mode_ptr->param.VDisp *= 2;
		mode_ptr->param.VBStart *= 2;
		mode_ptr->param.VSStart *= 2;
		mode_ptr->param.VSEnd *= 2;
		mode_ptr->param.VBEnd *= 2;
		mode_ptr->param.VTotal *= 2;
	}

	if (mode_ptr->param.interlace) {
		mode_ptr->param.VDisp /= 2;
		mode_ptr->param.VBStart /= 2;
		mode_ptr->param.VSStart /= 2;
		mode_ptr->param.VSEnd /= 2;
		mode_ptr->param.VBEnd /= 2;
		mode_ptr->param.VTotal /= 2;
	}

	mode_ptr->param.interlaceratio = 50;

	/* blank */
	mode_ptr->param.HBStart = mode_ptr->param.HDisp + 1;
	if (mode_ptr->param.HBStart > mode_ptr->param.HSStart)
		mode_ptr->param.HBStart = mode_ptr->param.HSStart;
	mode_ptr->param.HBEnd = mode_ptr->param.HSEnd + 1;
	mode_ptr->param.VBStart = mode_ptr->param.VDisp + 1;
	if (mode_ptr->param.VBStart > mode_ptr->param.VSStart)
		mode_ptr->param.VBStart = mode_ptr->param.VSStart;
	mode_ptr->param.VBEnd = mode_ptr->param.VSEnd + 1;

	return 0;
}

int mode_load(const char far* file)
{
	STACK_DECL char buffer[256];
	STACK_DECL mode_info info;
	STACK_DECL mode_info info_1;
	STACK_DECL mode_info info_3;
	STACK_DECL mode_info info_13;

	int handle = open(file, O_RDONLY);
	if (handle == -1) {
		cputs("Configuration file ");
		cputs(file);
		cputs(" not found\r\n");
		return 0;
	}

	mode_max = 0;

	memset(&info_1, 0, sizeof(mode_info));
	memset(&info_3, 0, sizeof(mode_info));
	memset(&info_13, 0, sizeof(mode_info));

	while (gets(handle, buffer, sizeof(buffer))) {
		memset(&info, 0, sizeof(mode_info));

		if (modeline_load(buffer, &info)==0) {

			if (is_graph_mode(info.width, info.height, info.param.dotclockHz) && mode_max + 3 < MODE_MAX) {
				info.effective_mode = 0x13;
				info.reported_mode = mode_number_generator++;
				memcpy(mode_map + mode_max, &info, sizeof(mode_info));
				++mode_max;
			}

			if (is_mode_1(info.width, info.height, info.param.dotclockHz) && is_less_mode(&info_1, &info)) {
				memcpy(&info_1, &info, sizeof(mode_info));
			}

			if (is_mode_3(info.width, info.height, info.param.dotclockHz) && is_less_mode(&info_3, &info)) {
				memcpy(&info_3, &info, sizeof(mode_info));
			}

			if (is_mode_13(info.width, info.height, info.param.dotclockHz) && is_less_mode(&info_13, &info)) {
				memcpy(&info_13, &info, sizeof(mode_info));
			}
		}
	}

	close(handle);

	if (info_1.width) {
		info_1.effective_mode = 0x1;
		info_1.reported_mode = 0x1;
		memcpy(mode_map + mode_max, &info_1, sizeof(mode_info));
		++mode_max;
	}

	if (info_3.width) {
		info_3.effective_mode = 0x3;
		info_3.reported_mode = 0x3;
		memcpy(mode_map + mode_max, &info_3, sizeof(mode_info));
		++mode_max;
	}

	if (info_13.width) {
		info_13.effective_mode = 0x13;
		info_13.reported_mode = 0x13;
		memcpy(mode_map + mode_max, &info_13, sizeof(mode_info));
		++mode_max;
	}

	if (!mode_max) {
		cputs("No modelines found\r\n");
		return 0;
	}

	return 1;
}

void mode_print(void)
{
	unsigned i;

	cputs("\r\nAvailable modes:\r\n");
	for(i=0;i<mode_max;++i) {
		unsigned v;
		cputs("        ");
		cputu(mode_map[i].reported_mode, 4, ' ', 16);
		cputs("h ");
		if (mode_map[i].effective_mode != 0x13) {
			cputs(" text ");
		} else {
			cputs(" graph");
		}
		cputu(mode_map[i].width, 4, ' ', 10);
		cputs("x");
		cputu(mode_map[i].height, 4, ' ', 10);
		cputs(" ");

		v = mode_map[i].param.dotclockHz / (mode_map[i].param.HTotal * 100L);
		cputu(v / 10, 2, ' ', 10);
		cputs(".");
		cputu(v % 10, 1, ' ', 10);
		cputs(" kHz/");

		v = 10L * mode_map[i].param.dotclockHz / (mode_map[i].param.HTotal * (long)mode_map[i].param.VTotal);
		cputu(v / 10, 2, ' ', 10);
		cputs(".");
		cputu(v % 10, 1, ' ', 10);
		cputs(" Hz ");

		cputs("\r\n");
	}
}

/***************************************************************************/
/* init/done */

int vga_init(const char far* file)
{
	mode_active = 0;

	if (!mode_load(file)) {
		return 0;
	}

	return 1;
}

void vga_done(void)
{
}

/***************************************************************************/
/* vga_service */

unsigned vga_service_call_00;
unsigned vga_service_call_auto;

void vga_service_00(regs_32 far * regs)
{
	unsigned req_mode;
	mode_info* mode_ptr;

	req_mode = regs->h.al;

	for(mode_ptr = mode_map;mode_ptr!=mode_map + mode_max;++mode_ptr)
		if (mode_ptr->reported_mode == req_mode)
			break;
	if (mode_ptr == mode_map + mode_max) {
		int_32_addr_call(int_10_old, regs);
		return;
	}

	/* active mode */
	mode_active = mode_ptr;

	regs->h.al = mode_active->effective_mode;
	int_32_addr_call(int_10_old, regs);
	regs->h.al = req_mode;

	vga_mode_set(&mode_active->param);

	++vga_service_call_00;
}
/*
--------V-M00400049--------------------------
MEM 0040h:0049h - VIDEO - CURRENT VIDEO MODE
Size:	BYTE
SeeAlso: MEM 0040h:004Ah, INT 10/AH=00h
*/
void vga_update_mode(void)
{
	mode_info* mode_ptr;
	uint8 req_mode;

	/* current video mode */
	req_mode = *(uint8 far*)MK_FP(0x0040, 0x0049);

	if (req_mode == 0x13) {
		unsigned hde;
		unsigned vde;
		uint8 d0;

		hde = card_crtc_get(0x1);
		hde += 1;
		hde *= 4; /* default VGA pixel per clock */

		vde = card_crtc_get(0x12);
		vde |= (((unsigned)card_crtc_get(0x7)) << 7) & 0x100;
		vde |= (((unsigned)card_crtc_get(0x7)) << 3) & 0x200;
		vde += 1;

		d0 = card_crtc_get(0x09);
		/* adjust for doublescan */
		if (d0 & 0x80)
			vde /= 2;
		/* adjust for char y */
		vde /= (d0 & 0x1F) + 1;

		for(mode_ptr = mode_map;mode_ptr!=mode_map + mode_max;++mode_ptr)
			if (mode_ptr->effective_mode == req_mode && mode_ptr->width == hde && mode_ptr->height == vde)
				break;
	} else {
		for(mode_ptr = mode_map;mode_ptr!=mode_map + mode_max;++mode_ptr)
			if (mode_ptr->effective_mode == req_mode)
				break;
	}

	if (mode_ptr == mode_map + mode_max) {
		/* mode not found */
		return;
	}

	if (mode_active == mode_ptr) {
		/* already setup */
		return;
	}

	/* active mode */
	mode_active = mode_ptr;

	vga_mode_set(&mode_active->param);

	++vga_service_call_auto;
}

/***************************************************************************/
/* trap */

/* Disable video output */
extern int vga_disable;

/* Ensure no double call */
static vga_trap_in = 0;

void int_10_trap(regs_32 far * int_10_regs)
{
	++vga_trap_in;
	if (vga_trap_in == 1 && int_10_regs->h.ah == 0x00) {
		vga_service_00(int_10_regs);
	} else {
		int_32_addr_call(int_10_old, int_10_regs);
	}
	--vga_trap_in;
}

void int_21_hook(void)
{
	++vga_trap_in;
	if (vga_trap_in == 1)
		vga_update_mode();
	--vga_trap_in;
}

void int_8_hook(void)
{
	++vga_trap_in;
	if (vga_trap_in == 1)
		vga_update_mode();
	--vga_trap_in;
}

/***************************************************************************/
/* TSR */

uint16 TSR_RUTACK1 = 0xAD17;
uint16 TSR_RUTACK2 = 0x175A;

int far _cdecl tsr_remote(void far * arg)
{
	if (arg) {
		vga_disable = 0;
	} else {
		vga_disable = 1;
	}
	return 0;
}

uint16 _pascal tsr(int (*remote)(void far* arg), char far* args)
{
	int arg_adjust = 0;
	int arg_unload = 0;
	int arg_load = 0;
	int arg_enable = 0;
	int arg_disable = 0;
	char far* tok;
	STACK_DECL char buffer[128];

	strcpy(buffer, "vga.rc");

	tok = strtok(args, " \t");
	while (tok) {
		if (optionmatch(tok, "l")) {
			arg_load = 1;
		} else if (optionmatch(tok, "u")) {
			arg_unload = 1;
		} else if (optionmatch(tok, "a")) {
			arg_adjust = 1;
		} else if (optionmatch(tok, "e")) {
			arg_enable = 1;
		} else if (optionmatch(tok, "d")) {
			arg_disable = 1;
		} else if (optionmatch(tok, "c")) {
			tok = strtok(0, " \t");
			if (!tok) {
				cputs("Missing config file path\r\n");
				return TSR_FAILURE;
			}
			strcpy(buffer, tok);
		} else {
			cputs("Unknown option ");
			cputs(tok);
			cputs("\r\n");
			return TSR_FAILURE;
		}
		tok = strtok(0, " \t");
	}

	if (arg_unload + arg_load + arg_adjust + arg_enable > 1
		|| arg_unload + arg_adjust + arg_enable + arg_disable > 1) {
		cputs("Incompatible commands\n\r");
		return TSR_FAILURE;
	}

	if (arg_adjust) {
		if (remote != 0) {
			cputs("You can't adjust the video mode if " OEM_PRODUCT_STR " is already loaded\n\r");
			return TSR_FAILURE;
		}

		if (!vga_init(buffer))
			return TSR_FAILURE;

		vga_update_mode();

		vga_done();

		return TSR_SUCCESS;
	}

	if (arg_load) {
		if (remote != 0) {
			cputs(OEM_PRODUCT_STR " already loaded\n\r");
			return TSR_FAILURE;
		}

		if (!vga_init(buffer))
			return TSR_FAILURE;

		vga_update_mode();

		if (arg_disable) {
			vga_disable = 1;
		}

		return TSR_LOAD;
	}

	if (arg_unload) {
		if (remote == 0) {
			cputs(OEM_PRODUCT_STR " already unloaded\n\r");
			return TSR_FAILURE;
		}
		return TSR_UNLOAD;
	}

	if (arg_enable || arg_disable) {
		if (remote == 0) {
			cputs("You can enable/disable video output if " OEM_PRODUCT_STR " isn't loaded\r\n");
			return TSR_FAILURE;
		}
		remote( arg_enable ? MK_FP(0, 1) : 0);
		return TSR_SUCCESS;
	}

	cputs(OEM_PRODUCT_STR " by " OEM_VENDOR_STR " v" OEM_VERSION_STR "\r\n");
	cputs(
"Usage:\n\r"
"    vga [/l] [/u] [/a] [/e] [/d] [/c CONFIG]\n\r"
"Commands:\n\r"
"    /l         Load the TSR\n\r"
"    /u         Unload the TSR\n\r"
"    /a         Adjust the current video mode\n\r"
"    /e         Enable text video output\n\r"
"    /d         Disable text video output\n\r"
"Options:\n\r"
"    /c CONFIG  Use this config file instead of vga.rc\n\r"
);

	return TSR_SUCCESS;
}

uint16 _pascal tsr_init_0(void)
{
	if (!vga_disable) {
		cputs(OEM_PRODUCT_STR " by " OEM_VENDOR_STR " v" OEM_VERSION_STR "\r\n");
		mode_print();
	}

	return TSR_SUCCESS;
}

uint16 _pascal tsr_init_1(void)
{
	return TSR_SUCCESS;
}

void _pascal tsr_done_0(void)
{
	vga_done();
}

void _pascal tsr_done_1(void)
{
	cputs(OEM_PRODUCT_STR " unloaded\r\n");

	if (vga_service_call_00 || vga_service_call_auto) {
		cputs("\r\nVGA calls:\r\n");
		cputs("        ");
		cputu(vga_service_call_00, 6, ' ', 10);
		cputs(" Mode Set\r\n");
		cputs("        ");
		cputu(vga_service_call_auto, 6, ' ', 10);
		cputs(" Mode Adjust\r\n");
	}
}