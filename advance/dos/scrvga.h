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

#ifndef __SCRVGA_H
#define __SCRVGA_H

#include "extra.h"
#include "rgb.h"

/* VGA port address */
#define VGAREG_ATTRCON_ADDR 0x3c0 /* Attribute Controller Address */
#define VGAREG_ATTRCON_WRITE_DATA 0x3c0 /* Attribute Controller Write Data */
#define VGAREG_ATTRCON_READ_DATA 0x3c1 /* Attribute Controller Read Data */
#define VGAREG_ATTRCON_RESET 0x3da /* Attribute Controller Reset */
#define VGAREG_MISC_WRITE_DATA 0x3c2 /* Miscellaneous Register */
#define VGAREG_MISC_READ_DATA 0x3cc /* Miscellaneous Register */
#define VGAREG_ENABLE_ADDR 0x3c3 /* VGA Enable Register */
#define VGAREG_SEQ_ADDR 0x3c4 /* Sequencer Address */
#define VGAREG_SEQ_DATA 0x3c5 /* Sequencer Data */
#define VGAREG_PEL_MASK 0x3c6 /* PEL mask */
#define VGAREG_DAC_STATE 0x3c7 /* DAC Read State */
#define VGAREG_DAC_READ_ADDR 0x3c7 /* DAC Read Address */
#define VGAREG_DAC_WRITE_ADDR 0x3c8 /* DAC Write Address */
#define VGAREG_DAC_DATA 0x3c9 /* DAC Read/Write Data */
#define VGAREG_GRACON_ADDR 0x3ce /* Graphics Controller Address */
#define VGAREG_GRACON_DATA 0x3cf /* Graphics Controller Data */
#define VGAREG_CRTC_ADDR 0x3d4 /* CRTC (Cathode Ray Tube Controller) Address */
#define VGAREG_CRTC_DATA 0x3d5 /* CRTC (Cathode Ray Tube Controller) Data */
#define VGAREG_STATUS_ADDR 0x3da /* Status Register */

/* VGA registers count */
#define VGAREG_ATTRCON_MAX 0x15 /* Attribute Controller */
#define VGAREG_SEQ_MAX 0x05 /* Sequencer */
#define VGAREG_GRACON_MAX 0x09 /* Graphics Controller */
#define VGAREG_CRTC_MAX 0x19 /* CRTC (Cathode Ray Tube Controller) */

/* VGA dotclocks */
#define VGA_DOTCLOCK_HIGH 28322000
#define VGA_DOTCLOCK_LOW 25175000

struct vga_regs {
	unsigned char attrcon[VGAREG_ATTRCON_MAX];
	unsigned char seq[VGAREG_ATTRCON_MAX];
	unsigned char gracon[VGAREG_ATTRCON_MAX];
	unsigned char crtc[VGAREG_CRTC_MAX];
	unsigned char misc;
};

struct vga_info {
	unsigned vt; /**< Vertical total scanlines */
	unsigned vrs; /**< Vertical retrace start scanline */
	unsigned vre; /**< Vertical retrace end scanline */
	unsigned vbs; /**< Vertical blank start scanline */
	unsigned vbe7; /**< Vertical blank end scanline (7 bit regs version) */
	unsigned vbe; /**< Vertical blank end scanline */
	unsigned vde; /**< Vertical display end scanline */

	unsigned ht; /**< Horizontal total clocks */
	unsigned hrs; /**< Horizontal retrace start clocks */
	unsigned hre; /**< Horizontal retrace end clocks */
	unsigned hbs; /**< Horizontal blank start clocks */
	unsigned hbe; /**< Horizontal blank end clocks */
	unsigned hde; /**< Horizontal display end clocks */

	unsigned size_x; /**< X resolution */
	unsigned size_y; /**< Y resolution */
	unsigned virtual_x; /**< X virtual resolution */
	unsigned virtual_y; /**< Y virtual resolution */
	unsigned bytes_per_scanline; /**< Bytes per scanline */
	unsigned bytes_per_clock; /*<* Byte per clock */
	unsigned partial_y; /**< Unused but shown rows */

	adv_bool hsync; /**< Horizontal sync polarity 1==negative, 0==positive */
	adv_bool vsync; /**< Vertical sync polarity 1==negative, 0==positive */

	adv_bool is_doublescan; /**< adv_bool, hardware bit */
	unsigned memory_mode; /**< 1==byte mode, 2==word mode, 4 dword mode */

	adv_bool is_linear; /* adv_bool, ==0 access by 4 plane (unchained), !=0 linear access (chained) */

	unsigned clocks_per_scanline; /**< Number of clocks per scanline */
	unsigned pixels_per_clock; /**< Number of pixels per clock */
	unsigned dots_per_clock; /**< Number of dots per clock */
	unsigned scanlines_per_line; /**< Number of scanlines for one chars row, include char_size_y and doublescan */
	unsigned bits_per_pixel; /**< Number of bits per pixel */

	adv_bool is_graphics_mode; /**< ==0 text, ==1 graphics mode */

	unsigned char_size_x; /**< Size of char in dot */
	unsigned char_size_y; /**< Size of char in scanline */

	adv_bool dotclock_middle; /**< Dot clock is Master clock/2 */
	adv_bool dotclock_master; /**< Master clock 0==25 1==28 */

	double pixel_clock; /**< Pixel frequency in Hz */
	double dot_clock; /**< Dot frequency in Hz */
	double vert_clock; /**< Vertical frequency in Hz */
	double horz_clock; /**< Horizontal frequency in Hz */

	unsigned memory_address; /**< Memory address */
	unsigned memory_size; /**< Size of the memory window */
};

extern unsigned char vga_font_bios_8[];
extern unsigned char vga_font_bios_14[];
extern unsigned char vga_font_bios_16[];
extern unsigned char vga_palette_bios_text[];
extern unsigned char vga_palette_bios_graph[];
extern struct vga_regs vga_mode_bios_1;
extern struct vga_regs vga_mode_bios_3;
extern struct vga_regs vga_mode_bios_13;

/***************************************************************************/
/* VGA REGS */

void vga_regs_htt_set(struct vga_regs* regs, unsigned value);
void vga_regs_hde_set(struct vga_regs* regs, unsigned value);
void vga_regs_hbs_set(struct vga_regs* regs, unsigned value);
void vga_regs_hbe_set(struct vga_regs* regs, unsigned value);
void vga_regs_hrs_set(struct vga_regs* regs, unsigned value);
void vga_regs_hre_set(struct vga_regs* regs, unsigned value);
void vga_regs_vtt_set(struct vga_regs* regs, unsigned value);
void vga_regs_vde_set(struct vga_regs* regs, unsigned value);
void vga_regs_vbs_set(struct vga_regs* regs, unsigned value);
void vga_regs_vbe_set(struct vga_regs* regs, unsigned value);
void vga_regs_vrs_set(struct vga_regs* regs, unsigned value);
void vga_regs_vre_set(struct vga_regs* regs, unsigned value);
void vga_regs_doublescan_set(struct vga_regs* regs, unsigned value);
void vga_regs_dotclock_middle_set(struct vga_regs* regs, unsigned value);
void vga_regs_masterclock_input_set(struct vga_regs* regs, unsigned value);
void vga_regs_char_size_x_set(struct vga_regs* regs, unsigned value);
void vga_regs_char_size_y_set(struct vga_regs* regs, unsigned value);
void vga_regs_hsync_set(struct vga_regs* regs, unsigned value);
void vga_regs_vsync_set(struct vga_regs* regs, unsigned value);
void vga_regs_offset_set(struct vga_regs* regs, unsigned value);
void vga_regs_chained_set(struct vga_regs* regs, adv_bool chained);
void vga_regs_info_get(struct vga_regs* regs, struct vga_info* info);

/***************************************************************************/
/* VGA */

/* VGA status */
typedef struct vga_internal_struct {
	struct vga_info info;
	struct vga_regs regs;
} vga_internal;

extern vga_internal vga_state;

static inline unsigned vga_virtual_x(void)
{
	return vga_state.info.virtual_x;
}

static inline unsigned vga_virtual_y(void)
{
	return vga_state.info.virtual_y;
}

static inline unsigned vga_bytes_per_scanline(void)
{
	return vga_state.info.bytes_per_scanline;
}

static inline unsigned vga_address(void)
{
	return vga_state.info.memory_address;
}

static inline unsigned vga_adjust_bytes_per_page(unsigned bytes_per_page)
{
	return (bytes_per_page + 0xFF) & ~0xFF;
}

/**
 * Return the active font x size.
 */
static inline unsigned vga_font_size_x(void)
{
	return vga_state.info.char_size_x;
}

/**
 * Return the active font y size.
 */
static inline unsigned vga_font_size_y(void)
{
	return vga_state.info.char_size_y;
}

/**
 * Check if unchained mode is active.
 */
static inline adv_bool vga_unchained_is_active(void)
{
	return !vga_state.info.is_linear;
}

void vga_mode_set(const struct vga_regs* regs);
void vga_mode_get(struct vga_regs* regs);
void vga_palette_raw_set(const unsigned char* palette, unsigned start, unsigned count);
void vga_palette_raw_get(unsigned char* palette, unsigned start, unsigned count);
adv_error vga_palette6_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);
adv_error vga_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);
void vga_wait_vsync(void);
void vga_scroll(unsigned offset);
void vga_scanline_set(unsigned byte_length);
void vga_unchained_plane_mask_set(unsigned plane_mask);
void vga_unchained_plane_set(unsigned plane);

void vga_font_copy(unsigned char* font, unsigned row, unsigned slot, adv_bool set);
unsigned vga_font_map_get(adv_bool bit);
void vga_font_map_set(unsigned bit_0_slot, unsigned bit_1_slot);
void vga_font_size_set(adv_bool use_512);

unsigned vga_dotclock_nearest_get(unsigned dotclock);
unsigned vga_pixelclock_nearest_get(unsigned pixelclock, adv_bool is_text);
unsigned vga_pixelclock_next_get(unsigned pixelclock, adv_bool is_text);
unsigned vga_pixelclock_pred_get(unsigned pixelclock, adv_bool is_text);

void vga_writeb(unsigned addr, unsigned char c);
unsigned char vga_readb(unsigned addr);

#endif
