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

#include "portable.h"

#include "scrvga.h"

#include <pc.h>
#include <go32.h>
#include <sys/farptr.h>

/***************************************************************************/
/* Tables */

#include "scrvga.dat"

/***************************************************************************/
/* VGA REGS */

/* Bit mask a register */
static inline unsigned mask(unsigned value, unsigned rbit, unsigned len)
{
	return (value >> rbit) & ((1 << len)-1);
}

/* Compute an end value */
static inline unsigned end_value(unsigned start, unsigned end_nibble, unsigned end_bits)
{
	unsigned end;
	unsigned end_mask = (1 << end_bits) - 1;
	assert((end_nibble & ~end_mask) == 0);
	if ((start & end_mask) > end_nibble) {
		end = start + end_mask + 1;
	} else {
		end = start;
	}
	return (end & ~end_mask) | end_nibble;
}

/* Set a register with mask */
static unsigned char setregmask(unsigned char value, unsigned rbit, unsigned lbit, unsigned char reg)
{
	assert(lbit <= 8);

	if (lbit != 8) {
		reg &= ~(((1 << lbit) - 1) << rbit);
		reg |= value << rbit;
		return reg;
	} else {
		return value;
	}
}

void vga_regs_htt_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x0] = value - 5;
}

void vga_regs_hde_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x1] = value - 1;
}

void vga_regs_hbs_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x2] = value;
}

void vga_regs_hbe_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x3] = setregmask(mask(value, 0, 5), 0, 5, regs->crtc[0x3]);
	regs->crtc[0x5] = setregmask(mask(value, 5, 1), 7, 1, regs->crtc[0x5]);
}

void vga_regs_hrs_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x4] = value;
}

void vga_regs_hre_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x5] = setregmask(mask(value, 0, 5), 0, 5, regs->crtc[0x5]);
}

void vga_regs_vtt_set(struct vga_regs* regs, unsigned value)
{
	value -= 2;
	regs->crtc[0x6] = mask(value, 0, 8);
	regs->crtc[0x7] = setregmask(mask(value, 8, 1), 0, 1, regs->crtc[0x7]);
	regs->crtc[0x7] = setregmask(mask(value, 9, 1), 5, 1, regs->crtc[0x7]);
}

void vga_regs_vde_set(struct vga_regs* regs, unsigned value)
{
	value -= 1;
	regs->crtc[0x12] = mask(value, 0, 8);
	regs->crtc[0x7] = setregmask(mask(value, 8, 1), 1, 1, regs->crtc[0x7]);
	regs->crtc[0x7] = setregmask(mask(value, 9, 1), 6, 1, regs->crtc[0x7]);
}

void vga_regs_vbs_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x15] = mask(value, 0, 8);
	regs->crtc[0x7] = setregmask(mask(value, 8, 1), 3, 1, regs->crtc[0x7]);
	regs->crtc[0x9] = setregmask(mask(value, 9, 1), 5, 1, regs->crtc[0x9]);
}

/**
 * Set a value compatible for 7 and 8 bit version of blank end.
 * VGA doc report use of 7 bit, but other documentations report that many
 * SVGA use all 8 bit.
 * \note Call after vertical_blank_start_set.
 */
void vga_regs_vbe_set(struct vga_regs* regs, unsigned value)
{
	/* get first 8 bit of start */
	unsigned start8 = regs->crtc[0x15];
	/* compute first 8 bit of end, compatible also for 7 bit regs */
	unsigned end8 = end_value(start8, value & 0x7F, 7);
	regs->crtc[0x16] = mask(end8, 0, 8);
}

void vga_regs_vrs_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x10] = mask(value, 0, 8);
	regs->crtc[0x7] = setregmask(mask(value, 8, 1), 2, 1, regs->crtc[0x7]);
	regs->crtc[0x7] = setregmask(mask(value, 9, 1), 7, 1, regs->crtc[0x7]);
}

void vga_regs_vre_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x11] = setregmask(mask(value, 0, 4), 0, 4, regs->crtc[0x11]);
}

void vga_regs_doublescan_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x9] = setregmask(mask(value, 0, 1), 7, 1, regs->crtc[0x9]);
}

void vga_regs_dotclock_middle_set(struct vga_regs* regs, unsigned value)
{
	regs->seq[0x1] = setregmask(mask(value, 0, 1), 3, 1, regs->seq[0x1]);
}

void vga_regs_masterclock_input_set(struct vga_regs* regs, unsigned value)
{
	regs->misc = setregmask(mask(value, 0, 2), 2, 2, regs->misc);
}

void vga_regs_char_size_x_set(struct vga_regs* regs, unsigned value)
{
	assert(value==8 || value==9);
	if (value==8)
		value = 1;
	else
		value = 0;
	regs->seq[0x1] = setregmask(mask(value, 0, 1), 0, 1, regs->seq[0x1]);
}

void vga_regs_char_size_y_set(struct vga_regs* regs, unsigned value)
{
	value -= 1;
	regs->crtc[0x9] = setregmask(mask(value, 0, 4), 0, 4, regs->crtc[0x9]);
}

void vga_regs_hsync_set(struct vga_regs* regs, unsigned value)
{
	regs->misc = setregmask(mask(value, 0, 1), 6, 1, regs->misc);
}

void vga_regs_vsync_set(struct vga_regs* regs, unsigned value)
{
	regs->misc = setregmask(mask(value, 0, 1), 7, 1, regs->misc);
}

void vga_regs_offset_set(struct vga_regs* regs, unsigned value)
{
	regs->crtc[0x13] = mask(value, 0, 8);
}

void vga_regs_chained_set(struct vga_regs* regs, adv_bool chained)
{
	if (chained) {
		/* turn on the Chain-4 bit */
		regs->seq[0x4] = setregmask(1, 3, 1, regs->seq[0x4]);
		/* turn on word mode (negate) */
		regs->crtc[0x17] = setregmask(0, 6, 1, regs->crtc[0x17]);
		/* turn on doubleword mode */
		regs->crtc[0x14] = setregmask(1, 6, 1, regs->crtc[0x14]);
	} else {
		/* turn off the Chain-4 bit */
		regs->seq[0x4] = setregmask(0, 3, 1, regs->seq[0x4]);
		/* turn off word mode (negate) */
		regs->crtc[0x17] = setregmask(1, 6, 1, regs->crtc[0x17]);
		/* turn off doubleword mode */
		regs->crtc[0x14] = setregmask(0, 6, 1, regs->crtc[0x14]);
	}
}

/* Compile tweak_mode_info strcture with video mode information */
void vga_regs_info_get(struct vga_regs* regs, struct vga_info* info)
{
	int temp;

	info->dotclock_master = mask(regs->misc, 2, 2);

	switch (info->dotclock_master) {
		case 0 :
			info->dot_clock = VGA_DOTCLOCK_LOW;
			break;
		case 1 :
			info->dot_clock = VGA_DOTCLOCK_HIGH;
			break;
		default:
			info->dot_clock = 0;
	}

	switch (mask(regs->seq[0x1], 3, 1)) {
		case 0:
			info->dotclock_middle = 0;
			break;
		case 1:
			info->dot_clock /= 2;
			info->dotclock_middle = 1;
			break;
	}

	switch (mask(regs->seq[0x1], 0, 1)) {
		case 0 :
			info->char_size_x = 9;
			break;
		case 1 :
			info->char_size_x = 8;
			break;
		default:
			info->char_size_x = 0;
	}

	switch (mask(regs->gracon[0x5], 5, 2)) {
		case 0:
			info->bits_per_pixel = 4; /* might also be COLOR2 !!! */
			info->pixels_per_clock = 8;
			info->bytes_per_clock = 1;
			break;
		case 1:
			info->bits_per_pixel = 2;
			info->pixels_per_clock = 16;
			info->bytes_per_clock = 2;
			break;
		case 2:
			info->bits_per_pixel = 8;
			info->pixels_per_clock = 4;
			info->bytes_per_clock = 4;
			break;
		default:
			info->bits_per_pixel = 0;
			info->pixels_per_clock = 0;
			info->bytes_per_clock = 0;
			break;
	}

	switch (mask(regs->gracon[0x6], 2, 2)) {
		case 0 :
			info->memory_address = 0xa0000;
			info->memory_size = 0x20000;
			break;
		case 1 :
			info->memory_address = 0xa0000;
			info->memory_size = 0x10000;
			break;
		case 2 :
			info->memory_address = 0xb0000;
			info->memory_size = 0x8000;
			break;
		case 3 :
			info->memory_address = 0xb8000;
			info->memory_size = 0x8000;
			break;
	}

	temp = ((regs->gracon[0x6] & 1) != 0);
	if (temp == ((regs->attrcon[0x10] & 1) != 0))
		info->is_graphics_mode = temp ? 1 : 0;
	else {
		info->is_graphics_mode = -1;
	}

	if (!info->is_graphics_mode) {
		info->bits_per_pixel = 0;
		info->pixels_per_clock = info->char_size_x;
		info->bytes_per_clock = 2;
		info->dots_per_clock = info->char_size_x;
	} else {
		info->dots_per_clock = 8;
	}

	info->is_linear = mask(regs->seq[0x4], 3, 1);
	if (mask(regs->crtc[0x14], 6, 1))
		info->memory_mode = 4; /* dword mode */
	else if (mask(regs->crtc[0x17], 6, 1)==0)
		info->memory_mode = 2; /* word mode */
	else
		info->memory_mode = 1; /* byte mode */
	info->clocks_per_scanline = regs->crtc[0x13] * 2 * info->memory_mode / info->bytes_per_clock;

	info->hde = regs->crtc[0x1] + 1;
	info->vde = (regs->crtc[0x12] | (mask(regs->crtc[0x7], 1, 1) << 8) | (mask(regs->crtc[0x7], 6, 1) << 9)) + 1;

	info->size_x = info->hde * info->pixels_per_clock;
	info->virtual_x = info->clocks_per_scanline * info->pixels_per_clock;
	if (info->is_graphics_mode && !info->is_linear)
		info->virtual_x *= 4;

	info->bytes_per_scanline = regs->crtc[0x13] * 2 * info->memory_mode;

	info->char_size_y = mask(regs->crtc[0x9], 0, 4) + 1;
	info->scanlines_per_line = info->char_size_y;
	info->is_doublescan = mask(regs->crtc[0x9], 7, 1);
	if (info->is_doublescan)
		info->scanlines_per_line *= 2;

	if (info->scanlines_per_line != 0) {
		info->size_y = info->vde / info->scanlines_per_line;
		info->partial_y = info->vde % info->scanlines_per_line;
	} else {
		info->size_y = 0;
		info->partial_y = 0;
	}
	if (info->bytes_per_scanline != 0) {
		info->virtual_y = (info->is_graphics_mode ? 65536L : 32768L) / info->bytes_per_scanline;
	} else {
		info->virtual_y = 0;
	}

	info->ht = regs->crtc[0x0] + 5;
	info->vt = (regs->crtc[0x6] | (mask(regs->crtc[0x7], 0, 1) << 8) | (mask(regs->crtc[0x7], 5, 1) << 9)) + 2;

	info->vrs = regs->crtc[0x10] | (mask(regs->crtc[0x7], 2, 1) << 8) | (mask(regs->crtc[0x7], 7, 1) << 9);
	info->vre = end_value(info->vrs, mask(regs->crtc[0x11], 0, 4), 4);

	info->vbs = regs->crtc[0x15] | (mask(regs->crtc[0x7], 3, 1) << 8) | (mask(regs->crtc[0x9], 5, 1) << 9);
	info->vbe = end_value(info->vbs, mask(regs->crtc[0x16], 0, 8), 8);
	info->vbe7 = end_value(info->vbs, mask(regs->crtc[0x16], 0, 7), 7);

	info->hbs = regs->crtc[0x2];
	info->hbe = end_value(info->hbs, mask(regs->crtc[0x3], 0, 5) | mask(regs->crtc[0x5], 7, 1) << 5, 6);

	info->hrs = regs->crtc[0x4];
	info->hre = end_value(info->hrs, mask(regs->crtc[0x5], 0, 5), 5);

	info->hsync = mask(regs->misc, 6, 1);
	info->vsync = mask(regs->misc, 7, 1);

	info->horz_clock = (double)info->dot_clock / (info->ht*info->dots_per_clock);
	info->vert_clock = (double)info->dot_clock / (info->ht*info->dots_per_clock*info->vt);
	if (info->dots_per_clock)
		info->pixel_clock = info->dot_clock * info->pixels_per_clock / info->dots_per_clock;
	else
		info->pixel_clock = 0;

	if (!info->is_graphics_mode) {
		info->size_y *= info->scanlines_per_line;
		info->virtual_y *= info->scanlines_per_line;
	}
}

/***************************************************************************/
/* VGA low level */

unsigned char vga_inb(unsigned port, unsigned index)
{
	switch (port) {
		case VGAREG_ATTRCON_ADDR :
			inportb(VGAREG_ATTRCON_RESET); /* flip-flop reset */
			outportb(VGAREG_ATTRCON_ADDR, index | 0x20);
			return inportb(VGAREG_ATTRCON_READ_DATA);
		case VGAREG_CRTC_ADDR :
		case VGAREG_SEQ_ADDR :
		case VGAREG_GRACON_ADDR :
			outportb(port, index);
			return inportb(port + 1);
		default:
			assert(0);
			return 0;
	}
}

void vga_outb(unsigned port, unsigned index, unsigned char value)
{
	switch (port) {
		case VGAREG_ATTRCON_ADDR :
			inportb(VGAREG_ATTRCON_RESET); /* flip-flop reset */
			outportb(VGAREG_ATTRCON_ADDR, index | 0x20);
			outportb(VGAREG_ATTRCON_WRITE_DATA, value);
			break;
		case VGAREG_CRTC_ADDR :
		case VGAREG_SEQ_ADDR :
		case VGAREG_GRACON_ADDR :
			outportb(port, index);
			outportb(port + 1, value);
			break;
		default:
			assert(0);
	}
}

void vga_writeb(unsigned addr, unsigned char c)
{
	_farpokeb(_dos_ds, addr, c);
}

unsigned char vga_readb(unsigned addr)
{
	return _farpeekb(_dos_ds, addr);
}

/***************************************************************************/
/* VGA */

vga_internal vga_state;

void vga_mode_set(const struct vga_regs* regs)
{
	unsigned i;

	vga_state.regs = *regs;

	vga_regs_info_get(&vga_state.regs, &vga_state.info);

	/* misc */
	outportb(VGAREG_MISC_WRITE_DATA, regs->misc);

	/* pel */
	outportb(VGAREG_PEL_MASK, 0xFF);

	/* sequencer reset on */
	vga_outb(VGAREG_SEQ_ADDR, 0, 0x01);

	/* sequencer */
	for(i=1;i<VGAREG_SEQ_MAX;++i)
		vga_outb(VGAREG_SEQ_ADDR, i, regs->seq[i]);

	/* sequencer reset off */
	vga_outb(VGAREG_SEQ_ADDR, 0, 0x03);

	/* enable crtc */
	vga_outb(VGAREG_CRTC_ADDR, 0x11, vga_inb(VGAREG_CRTC_ADDR, 0x11) & 0x7F);

	/* crtc */
	for(i=0;i<VGAREG_CRTC_MAX;++i)
		vga_outb(VGAREG_CRTC_ADDR, i, regs->crtc[i]);

	/* graphics controller */
	for(i=0;i<VGAREG_GRACON_MAX;++i)
		vga_outb(VGAREG_GRACON_ADDR, i, regs->gracon[i]);

	/* attribute controller */
	for(i=0;i<VGAREG_ATTRCON_MAX;++i)
		vga_outb(VGAREG_ATTRCON_ADDR, i, regs->attrcon[i]);
}

void vga_mode_get(struct vga_regs* regs)
{
	unsigned i;

	regs->misc = inportb(VGAREG_MISC_READ_DATA);

	/* sequencer */
	for(i=0;i<VGAREG_SEQ_MAX;++i)
		regs->seq[i] = vga_inb(VGAREG_SEQ_ADDR, i);

	/* crtc */
	for(i=0;i<VGAREG_CRTC_MAX;++i)
		regs->crtc[i] = vga_inb(VGAREG_CRTC_ADDR, i);

	/* graphics controller */
	for(i=0;i<VGAREG_GRACON_MAX;++i)
		regs->gracon[i] = vga_inb(VGAREG_GRACON_ADDR, i);

	/* attribute controller */
	for(i=0;i<VGAREG_ATTRCON_MAX;++i)
		regs->attrcon[i] = vga_inb(VGAREG_ATTRCON_ADDR, i);
}

void vga_palette_raw_set(const unsigned char* palette, unsigned start, unsigned count)
{
	unsigned i;

	/* dac */
	outportb(VGAREG_DAC_WRITE_ADDR, start);
	for(i=0;i<count;++i) {
		outportb(VGAREG_DAC_DATA, palette[0]);
		outportb(VGAREG_DAC_DATA, palette[1]);
		outportb(VGAREG_DAC_DATA, palette[2]);
		palette += 3;
	}
}

void vga_palette_raw_get(unsigned char* palette, unsigned start, unsigned count)
{
	unsigned i;

	/* dac */
	outportb(VGAREG_DAC_WRITE_ADDR, start);
	for(i=0;i<count;++i) {
		palette[0] = inportb(VGAREG_DAC_DATA);
		palette[1] = inportb(VGAREG_DAC_DATA);
		palette[2] = inportb(VGAREG_DAC_DATA);
		palette += 3;
	}
}

adv_error vga_palette6_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
	unsigned i;

	if (waitvsync)
		vga_wait_vsync();

	/* dac */
	outportb(VGAREG_DAC_WRITE_ADDR, start);
	for(i=0;i<count;++i) {
		outportb(VGAREG_DAC_DATA, palette->red);
		outportb(VGAREG_DAC_DATA, palette->green);
		outportb(VGAREG_DAC_DATA, palette->blue);
		++palette;
	}

	return 0;
}

adv_error vga_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
	unsigned i;

	if (waitvsync)
		vga_wait_vsync();

	/* dac */
	outportb(VGAREG_DAC_WRITE_ADDR, start);
	for(i=0;i<count;++i) {
		outportb(VGAREG_DAC_DATA, palette->red >> 2);
		outportb(VGAREG_DAC_DATA, palette->green >> 2);
		outportb(VGAREG_DAC_DATA, palette->blue >> 2);
		++palette;
	}

	return 0;
}

/**
 * Upper loop limit for the vsync wait.
 * Any port read take approximatively 0.5 - 1.5us.
 */
#define VGA_VSYNC_LIMIT 100000

void vga_wait_vsync(void)
{
	/* prevent infinite loop, don't disable int */
	unsigned counter = 0;
	while ((inportb(VGAREG_STATUS_ADDR) & 0x08) != 0 && counter < VGA_VSYNC_LIMIT)
		++counter;
	while ((inportb(VGAREG_STATUS_ADDR) & 0x08) == 0 && counter < VGA_VSYNC_LIMIT)
		++counter;
}

void vga_scroll(unsigned offset)
{
	vga_outb(VGAREG_CRTC_ADDR, 0xc, (offset >> 8) & 0xff);
	vga_outb(VGAREG_CRTC_ADDR, 0xd, offset & 0xff);
}

void vga_scanline_set(unsigned byte_length)
{
	unsigned offset;

	offset = byte_length / (vga_state.info.memory_mode * 2);
	if (!vga_state.info.is_linear)
		offset /= 4;

	vga_outb(VGAREG_CRTC_ADDR, 0x13, offset);
}

/**
 * Select an unchained plane mask
 * \param plane_mask mask to activate, from 0 to 0xF
 */
void vga_unchained_plane_mask_set(unsigned plane_mask)
{
	assert((plane_mask | 0xf) == 0xf);
	vga_outb(VGAREG_SEQ_ADDR, 0x2, plane_mask & 0xf);
}

/**
 * Select an unchained plane.
 * \param plane plane to activate, from 0 to 3
 */
void vga_unchained_plane_set(unsigned plane)
{
	assert(plane < 4);
	vga_unchained_plane_mask_set(1 << plane);
}

void vga_font_copy(unsigned char* font, unsigned row, unsigned slot, adv_bool set)
{
	unsigned addr = 0xa0000 + slot * 8192;

	unsigned char seq_2 = vga_inb(VGAREG_SEQ_ADDR, 2);
	unsigned char seq_4 = vga_inb(VGAREG_SEQ_ADDR, 4);
	unsigned char gracon_4 = vga_inb(VGAREG_GRACON_ADDR, 4);
	unsigned char gracon_5 = vga_inb(VGAREG_GRACON_ADDR, 5);
	unsigned char gracon_6 = vga_inb(VGAREG_GRACON_ADDR, 6);

	vga_outb(VGAREG_SEQ_ADDR, 0x0, 0x1); /* reset on */
	vga_outb(VGAREG_SEQ_ADDR, 0x2, 0x4); /* CPU writes only to map 2 */
	vga_outb(VGAREG_SEQ_ADDR, 0x4, 0x7); /* sequential addressing */
	vga_outb(VGAREG_SEQ_ADDR, 0x0, 0x3); /* reset off */

	vga_outb(VGAREG_GRACON_ADDR, 0x4, 0x2); /* select map 2 */
	vga_outb(VGAREG_GRACON_ADDR, 0x5, 0x0); /* disable odd-even addressing */
	vga_outb(VGAREG_GRACON_ADDR, 0x6, 0x0); /* map start at A000:0000 */

	if (set) {
		unsigned i;
		for(i=0;i<256;++i) {
			unsigned j;
			for(j=0;j<row;++j)
				vga_writeb(addr++, *font++);
			for(;j<32;++j)
				vga_writeb(addr++, 0);
		}
	} else {
		unsigned i;
		for(i=0;i<256;++i) {
			unsigned j;
			for(j=0;j<row;++j)
				*font++ = vga_readb(addr++);
			for(;j<32;++j)
				vga_readb(addr++);
		}
	}

	vga_outb(VGAREG_SEQ_ADDR, 0x0, 0x1); /* reset on */
	vga_outb(VGAREG_SEQ_ADDR, 0x2, seq_2); /* CPU writes to maps 0 and 1 */
	vga_outb(VGAREG_SEQ_ADDR, 0x4, seq_4); /* odd-even addressing */
	vga_outb(VGAREG_SEQ_ADDR, 0x0, 0x3); /* reset off */

	vga_outb(VGAREG_GRACON_ADDR, 0x4, gracon_4); /* select map 0 for CPU */
	vga_outb(VGAREG_GRACON_ADDR, 0x5, gracon_5); /* enable even-odd addressing */
	vga_outb(VGAREG_GRACON_ADDR, 0x6, gracon_6); /* map start */
}

unsigned vga_font_map_get(adv_bool bit)
{
	unsigned c = vga_inb(VGAREG_SEQ_ADDR, 0x3);

	if (bit)
		return mask(c, 2, 2) | mask(c, 5, 1) << 2;
	else
		return mask(c, 0, 2) | mask(c, 4, 1) << 2;
}

void vga_font_map_set(unsigned bit_0_slot, unsigned bit_1_slot)
{
	unsigned c;

	assert(bit_0_slot < 8 && bit_1_slot < 8);

	c = mask(bit_0_slot, 0, 2) | mask(bit_0_slot, 2, 1) << 4;
	c |= mask(bit_1_slot, 0, 2) << 2 | mask(bit_1_slot, 2, 1) << 5;

	vga_outb(VGAREG_SEQ_ADDR, 0x0, 0x1); /* reset on */
	vga_outb(VGAREG_SEQ_ADDR, 0x3, c);
	vga_outb(VGAREG_SEQ_ADDR, 0x0, 0x3); /* reset off */
}

void vga_font_size_set(adv_bool use_512)
{

	if (use_512)
		/* disable intensity bit */
		vga_outb(VGAREG_ATTRCON_ADDR, 0x12, 0x07);
	else
		/* enable intensity bit */
		vga_outb(VGAREG_ATTRCON_ADDR, 0x12, 0x0f);

	/* Wilton (1987) mentions the following; I don't know what */
	/* it means, but it works, and it appears necessary */
	inportb(VGAREG_ATTRCON_RESET);
	outportb(VGAREG_ATTRCON_ADDR, 0x20);
}

/***************************************************************************/
/* STATE */

static unsigned vga_dotclock[] = {
	VGA_DOTCLOCK_LOW/2,
	VGA_DOTCLOCK_HIGH/2,
	VGA_DOTCLOCK_LOW,
	VGA_DOTCLOCK_HIGH,
	0
};

unsigned vga_dotclock_nearest_get(unsigned dotclock)
{
	unsigned best;
	int best_diff;
	unsigned i;

	best = 0;
	best_diff = abs(vga_dotclock[0] - dotclock);
	for(i=1;vga_dotclock[i];++i) {
		int diff = abs(vga_dotclock[i] - dotclock);
		if (diff < best_diff) {
			best = i;
			best_diff = diff;
		}
	}

	return vga_dotclock[best];
}

static unsigned vga_regs_clock_multiplier_get(adv_bool is_text)
{
	if (is_text)
		return 1;
	else
		return 2;
}

unsigned vga_pixelclock_nearest_get(unsigned pixelclock, adv_bool is_text)
{
	unsigned multiplier;

	multiplier = vga_regs_clock_multiplier_get(is_text);

	pixelclock *= multiplier;

	pixelclock = vga_dotclock_nearest_get(pixelclock);

	pixelclock /= multiplier;

	return pixelclock;
}

/* Return the next avaliable pixel clock */
unsigned vga_pixelclock_next_get(unsigned pixelclock, adv_bool is_text)
{
	unsigned i;
	unsigned multiplier;

	multiplier = vga_regs_clock_multiplier_get(is_text);

	pixelclock *= multiplier;

	i = 0;
	while (vga_dotclock[i]!=0 && vga_dotclock[i]<=pixelclock)
		++i;
	if (!vga_dotclock[i])
		--i;

	pixelclock = vga_dotclock[i];
	pixelclock /= multiplier;

	return pixelclock;
}

/* Return the pred avaliable pixel clock */
unsigned vga_pixelclock_pred_get(unsigned pixelclock, adv_bool is_text)
{
	unsigned i;
	unsigned multiplier;

	multiplier = vga_regs_clock_multiplier_get(is_text);

	pixelclock *= multiplier;

	i = 0;
	while (vga_dotclock[i]!=0 && vga_dotclock[i]<pixelclock)
		++i;
	if (i)
		--i;

	pixelclock = vga_dotclock[i];
	pixelclock /= multiplier;

	return pixelclock;
}

