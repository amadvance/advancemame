/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Andrea Mazzoleni
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

#ifndef __CARD_H
#define __CARD_H

#include "compil.h"

typedef struct {
	/* Horizontal CRTC */
	int HTotal;
	int HDisp;
	int HBStart;
	int HSStart;
	int HSEnd;
	int HBEnd;

	/* Vertical CRTC, doubled in doublescan, halved in interlaced */
	int VTotal;
	int VDisp;
	int VBStart;
	int VSStart;
	int VSEnd;
	int VBEnd;

	int hpolarity; /* HSync polarity, !=0 negative */
	int vpolarity; /* VSync polarity, !=0 negative */
	int doublescan; /* Doublescan flag */
	int interlace; /* Interlace flag */
	int interlaceratio; /* Interlace ratio, 0-100 */

	/* Clocks */
	long dotclockHz;
} card_crtc;

typedef struct {
	unsigned width; /* Size of the mode */
	unsigned height;

	unsigned bits_per_pixel; /* Bits per pixel */
	unsigned bytes_per_pixel; /* Bytes per pixel */
	unsigned bytes_per_scanline; /* Bytes per scanline */

	/* Rgb format */
	unsigned red_len;
	unsigned red_pos;
	unsigned green_len;
	unsigned green_pos;
	unsigned blue_len;
	unsigned blue_pos;
} card_mode;

void card_crtc_all_set(card_crtc STACK_PTR *cp);
void card_generic_all_set(card_crtc STACK_PTR *cp);
DWORD card_bitmov(DWORD dstdata, int dstbitpos, DWORD srcdata, int srcbitpos);

void card_out(int edx, BYTE al);
BYTE card_in(int edx);
void card_outw(int edx, WORD ax);
WORD card_inw(int edx);
void card_outl(int edx, DWORD eax);
DWORD card_inl(int edx);

int card_crtc_address_get(void);
BYTE card_seq_get(BYTE);
void card_seq_set(BYTE, BYTE);
BYTE card_crtc_get(BYTE);
void card_crtc_set(BYTE, BYTE);
BYTE card_attr_get(BYTE);
void card_attr_set(BYTE, BYTE);
BYTE card_graph_get(BYTE);
void card_graph_set(BYTE, BYTE);
int card_divider_get(void);
void card_divider_set(BYTE);
void card_polarity_set(int HPolar, int VPolar);
void card_doublescan_set(int flag);
void card_signal_disable(void);
void card_signal_enable(void);
void card_unlock(void);
void card_lock(void);
void card_char_size_x_set(unsigned);
void card_char_size_y_set(unsigned);
void card_pll_set(unsigned);
unsigned card_pll_get(void);
long card_clock_compute(long dotclock, int mul_min, int mul_max, int div_min, int div_max, int p_min, int p_max, long ref, long vco_min, long vco_max, int STACK_PTR* s_mul, int STACK_PTR* s_div, int STACK_PTR* s_p, int (*validate)(int, int, int));
int card_compatible_mode(const card_mode STACK_PTR* mode0, const card_mode STACK_PTR* mode1);

/***************************************************************************/
/* Log */

#ifdef __PCIREAL__

#define CARD_LOG(a) do { } while (0)

#else

void card_log(const char *text, ...) __attribute__((format(printf, 1, 2)));

#define CARD_LOG(a) card_log a

#endif

#endif
