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

#include "vvgal.h"
#include "video.h"
#include "log.h"
#include "error.h"
#include "snstring.h"

#include <sys/nearptr.h>

/***************************************************************************/
/* State */

typedef struct vgaline_internal_struct {
	adv_bool active;
	adv_bool mode_active;
} vgaline_internal;

static vgaline_internal vgaline_state;

unsigned char* (*vgaline_write_line)(unsigned y);

/**
 * Return the offset to access in writing the video memory.
 */
static unsigned char* vgaline_write_line_graph(unsigned y)
{
	return (unsigned char*)(__djgpp_conventional_base + vga_address() + y * vga_bytes_per_scanline());
}

static unsigned char* vgaline_write_line_text(unsigned y)
{
	return (unsigned char*)(__djgpp_conventional_base + vga_address() + y * vga_bytes_per_scanline());
}

/***************************************************************************/
/* CRTC */

/* Functions used for computing register values when a division is required */
static inline unsigned div_low(unsigned value, unsigned value_div)
{
	return value / value_div;
}

static inline unsigned div_high(unsigned value, unsigned value_div)
{
	return (value + value_div - 1) / value_div;
}

static void video_crtc_realize_h(const adv_crtc* crtc, unsigned *de, unsigned* bs, unsigned* rs, unsigned* re, unsigned* be, unsigned* tt, unsigned x_div)
{
	/* output horizontal */
	*tt = div_low(crtc->ht, x_div);
	*de = div_high(crtc->hde, x_div);
	*rs = div_high(crtc->hrs, x_div);
	*re = div_low(crtc->hre, x_div);
	*bs = *de + 1;
	if (*bs > *rs)
		*bs = *rs;
	*be = *re + 1;
/* Not aggressive version
	*bs = *rs;
	*be = *re + 1;
*/
}

static void video_crtc_realize_v(const adv_crtc* crtc, unsigned *de, unsigned* bs, unsigned* rs, unsigned* re, unsigned* be, unsigned* tt, unsigned y_mul)
{
	*tt = crtc->vt * y_mul;
	*de = crtc->vde * y_mul;
	*rs = crtc->vrs * y_mul;
	*re = crtc->vre * y_mul;
	*bs = *de + 1;
	if (*bs > *rs)
		*bs = *rs;
	*be = *re + 1;
/* Not aggressive version
	*bs = *rs;
	*be = *re + 1;
*/
}

/**
 * Compute crtc value from vga tweak information.
 * \param info tweak information of the video mode
 * \param size_x, size_y size of the video mode
 * \param vclock vclock of the video mode
 * \param crtc resulting crtc value
 * \note It supposes a NOT interlaced mode (VGA regs don't support this feature)
 */
adv_error crtc_import(adv_crtc* crtc, struct vga_info* info, unsigned size_x, unsigned size_y, double vclock)
{
	unsigned y_mul = 0;
	unsigned x_div = 0;

	if (size_x % info->hde != 0) {
		error_set("Video mode doesn't support VGA CRTC regs grabbing");
		return -1;
	}

	x_div = size_x / info->hde;

	if (!x_div) {
		error_set("Video mode doesn't support VGA CRTC regs grabbing");
		return -1;
	}

	crtc->hde = info->hde * x_div;
	crtc->hrs = info->hrs * x_div;
	crtc->hre = info->hre * x_div;
	crtc->ht = info->ht * x_div;

	if (info->vde % size_y != 0) {
		error_set("Video mode doesn't support VGA CRTC regs grabbing");
		return -1;
	}
	y_mul = info->vde / size_y;

	if (!y_mul) {
		error_set("Video mode doesn't support VGA CRTC regs grabbing");
		return -1;
	}

	if (y_mul >= 2)
		crtc_doublescan_set(crtc);
	else
		crtc_singlescan_set(crtc);

	crtc->vde = info->vde / y_mul;
	crtc->vrs = info->vrs / y_mul;
	crtc->vre = info->vre / y_mul;
	crtc->vt = info->vt / y_mul;

	/* pixelclock is computed, not imported */
	crtc->pixelclock = crtc->vt * y_mul * crtc->ht * vclock;

	if (info->hsync)
		crtc_nhsync_set(crtc);
	else
		crtc_phsync_set(crtc);
	if (info->vsync)
		crtc_nvsync_set(crtc);
	else
		crtc_pvsync_set(crtc);

	crtc_name_set(crtc, "imported");

	return 0;
}

/***************************************************************************/
/* Internal */

adv_color_def vgaline_rgb_def(void)
{
	return color_def_make(adv_color_type_palette); /* always palettized */
}

static int vgaline_mode_graph_realize(struct vga_regs* regs, const adv_crtc* crtc)
{
	unsigned x_div = 4; /* 4 pixel for clock in 8 bit mode */
	unsigned y_mul = crtc_is_doublescan(crtc) ? 2 : 1;
	unsigned de, bs, rs, re, be, tt;
	unsigned dotclock;

	*regs = vga_mode_bios_13;

	/* get dot clock */
	dotclock = vga_dotclock_nearest_get(crtc->pixelclock * 8 / x_div);

	/* output horizontal */
	video_crtc_realize_h(crtc, &de, &bs, &rs, &re, &be, &tt, x_div);
	vga_regs_htt_set(regs, tt);
	vga_regs_hde_set(regs, de);
	vga_regs_hrs_set(regs, rs);
	vga_regs_hre_set(regs, re);
	vga_regs_hbs_set(regs, bs);
	vga_regs_hbe_set(regs, be);

	/* output vertical */
	video_crtc_realize_v(crtc, &de, &bs, &rs, &re, &be, &tt, y_mul);
	vga_regs_vtt_set(regs, tt);
	vga_regs_vde_set(regs, de);
	vga_regs_vrs_set(regs, rs);
	vga_regs_vre_set(regs, re);
	vga_regs_vbs_set(regs, bs);
	vga_regs_vbe_set(regs, be);

	/* set char size */
	vga_regs_char_size_x_set(regs, 8);

	/* set char size emulating doublescan */
	vga_regs_char_size_y_set(regs, crtc_is_doublescan(crtc) ? 2 : 1);

	/* reset double scan */
	vga_regs_doublescan_set(regs, 0);

	/* set dot clock */
	switch (dotclock) {
		case VGA_DOTCLOCK_HIGH:
			vga_regs_dotclock_middle_set(regs, 0);
			vga_regs_masterclock_input_set(regs, 1);
		break;
		case VGA_DOTCLOCK_HIGH/2 :
			vga_regs_dotclock_middle_set(regs, 1);
			vga_regs_masterclock_input_set(regs, 1);
		break;
		case VGA_DOTCLOCK_LOW :
			vga_regs_dotclock_middle_set(regs, 0);
			vga_regs_masterclock_input_set(regs, 0);
		break;
		case VGA_DOTCLOCK_LOW/2 :
			vga_regs_dotclock_middle_set(regs, 1);
			vga_regs_masterclock_input_set(regs, 0);
		break;
		default:
			return -1;
	}

	/* set polarity */
	vga_regs_hsync_set(regs, crtc_is_nhsync(crtc));
	vga_regs_vsync_set(regs, crtc_is_nvsync(crtc));

	/* set scanline length */
	vga_regs_offset_set(regs, (crtc->hde + 2 * x_div - 1) / (2 * x_div));

	/* set chained/unchained mode */
	if (crtc->hde * crtc->vde > 0x10000) {
		vga_regs_chained_set(regs, 0);
	} else {
		vga_regs_chained_set(regs, 1);
	}

	return 0;
}

static int vgaline_mode_text_realize(struct vga_regs* regs, const adv_crtc* crtc, unsigned font_x, unsigned font_y)
{
	unsigned x_div = font_x;
	unsigned y_mul = crtc_is_doublescan(crtc) ? 2 : 1;
	unsigned de, bs, rs, re, be, tt;
	unsigned dotclock;

	*regs = vga_mode_bios_3;

	/* Get dot clock (for text mode is equal at dot clock) */
	dotclock = vga_dotclock_nearest_get(crtc->pixelclock);

	/* output horizontal */
	video_crtc_realize_h(crtc, &de, &bs, &rs, &re, &be, &tt, x_div);
	vga_regs_htt_set(regs, tt);
	vga_regs_hde_set(regs, de);
	vga_regs_hrs_set(regs, rs);
	vga_regs_hre_set(regs, re);
	vga_regs_hbs_set(regs, bs);
	vga_regs_hbe_set(regs, be);

	/* output vertical */
	video_crtc_realize_v(crtc, &de, &bs, &rs, &re, &be, &tt, y_mul);
	vga_regs_vtt_set(regs, tt);
	vga_regs_vde_set(regs, de);
	vga_regs_vrs_set(regs, rs);
	vga_regs_vre_set(regs, re);
	vga_regs_vbs_set(regs, bs);
	vga_regs_vbe_set(regs, be);

	/* set char size */
	vga_regs_char_size_x_set(regs, font_x);
	vga_regs_char_size_y_set(regs, font_y);

	/* set double scan as requested */
	vga_regs_doublescan_set(regs, crtc_is_doublescan(crtc));

	/* set dot clock */
	switch (dotclock) {
		case VGA_DOTCLOCK_HIGH :
			vga_regs_dotclock_middle_set(regs, 0);
			vga_regs_masterclock_input_set(regs, 1);
		break;
		case VGA_DOTCLOCK_HIGH/2 :
			vga_regs_dotclock_middle_set(regs, 1);
			vga_regs_masterclock_input_set(regs, 1);
		break;
		case VGA_DOTCLOCK_LOW :
			vga_regs_dotclock_middle_set(regs, 0);
			vga_regs_masterclock_input_set(regs, 0);
		break;
		case VGA_DOTCLOCK_LOW/2 :
			vga_regs_dotclock_middle_set(regs, 1);
			vga_regs_masterclock_input_set(regs, 0);
		break;
		default:
			return -1;
	}

	/* set polarity */
	vga_regs_hsync_set(regs, crtc_is_nhsync(crtc));
	vga_regs_vsync_set(regs, crtc_is_nvsync(crtc));

	/* set scanline length */
	vga_regs_offset_set(regs, (crtc->hde + 2 * x_div - 1) / (2 * x_div));

	return 0;
}

static int vgaline_mode_realize(struct vga_regs* regs, const vgaline_video_mode* mode)
{
	if (mode->is_text) {
		if (vgaline_mode_text_realize(regs, &mode->crtc, mode->font_x, mode->font_y)!=0) {
			error_set("Error in vgaline mode realize text");
			return -1;
		}
	} else {
		if (vgaline_mode_graph_realize(regs, &mode->crtc)!=0) {
			error_set("Error in vgaline mode realize graphics");
			return -1;
		}
	}
	return 0;
}

/***************************************************************************/
/* Public */

static adv_device DEVICE[] = {
{ "auto", 1, "VGA video" },
{ 0, 0, 0 }
};

adv_error vgaline_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	const adv_device* i;

	(void)cursor;

	assert(!vgaline_is_active());

	if (sizeof(vgaline_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	i = DEVICE;
	while (i->name && i->id != device_id)
		++i;
	if (!i->name)
		return -1;

	vgaline_state.active = 1;
	return 0;
}

void vgaline_done(void)
{
	assert(vgaline_is_active());

	vgaline_state.active = 0;
}

adv_bool vgaline_is_active(void)
{
	return vgaline_state.active;
}

adv_bool vgaline_mode_is_active(void)
{
	return vgaline_state.mode_active;
}

unsigned vgaline_flags(void)
{
	return VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_TEXT
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_CRTC
		| VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;
}

adv_error vgaline_mode_set(const vgaline_video_mode* mode)
{
	struct vga_regs regs;

	log_std_modeline_c(("vgaline: mode_set modeline", mode->crtc.pixelclock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc)));

	if (vgaline_mode_realize(&regs, mode)!=0) {
		return -1;
	}

	vga_mode_set(&regs);

	if (mode->is_text) {
		vgaline_write_line = vgaline_write_line_text;
		switch (mode->font_y) {
			case 8 :
				vga_font_copy(vga_font_bios_8, 8, 0, 1);
				break;
			case 14 :
				vga_font_copy(vga_font_bios_14, 14, 0, 1);
				break;
			case 16 :
				vga_font_copy(vga_font_bios_16, 16, 0, 1);
				break;
		}
		vga_palette_raw_set(vga_palette_bios_text, 0, 256);
	} else {
		vgaline_write_line = vgaline_write_line_graph;
		vga_palette_raw_set(vga_palette_bios_graph, 0, 256);
	}

	vgaline_state.mode_active = 1;

	return 0;
}

void vgaline_mode_done(adv_bool restore)
{
	assert(vgaline_mode_is_active());

	vgaline_state.mode_active = 0;
}

adv_error vgaline_mode_grab(vgaline_video_mode* mode)
{
	struct vga_regs regs;
	struct vga_info info;

	assert(vgaline_is_active());

	/* get the video mode */
	vga_mode_get(&regs);

	/* import information */
	vga_regs_info_get(&regs, &info);

	/* generate crtc values */
	if (crtc_import(&mode->crtc, &info, info.size_x, info.size_y, info.vert_clock)!=0)
		return -1;

	mode->font_x = info.char_size_x;
	mode->font_y = info.char_size_y;

	crtc_name_set(&mode->crtc, "grabbed");

	return 0;
}

#define DRIVER(mode) ((vgaline_video_mode*)(&mode->driver_mode))

adv_error vgaline_mode_import(adv_mode* mode, const vgaline_video_mode* vgaline_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, vgaline_mode->crtc.name);
	*DRIVER(mode) = *vgaline_mode;

	/* adjust the pixel clock to an acceptable value */
	DRIVER(mode)->crtc.pixelclock = vga_pixelclock_nearest_get(DRIVER(mode)->crtc.pixelclock, DRIVER(mode)->is_text);

	mode->driver = &video_vgaline_driver;
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC | MODE_FLAGS_RETRACE_SET_ASYNC
		| (mode->flags & MODE_FLAGS_USER_MASK);

	if (DRIVER(mode)->is_text) {
		mode->flags |= MODE_FLAGS_INDEX_TEXT;
	} else {
		mode->flags |= MODE_FLAGS_INDEX_PALETTE8;
		if (DRIVER(mode)->crtc.vde * DRIVER(mode)->crtc.hde > 0x10000)
			return -1;
	}

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

static int vgaline_acceptable_pixelclock(unsigned requested, unsigned effective)
{
	int err = (int)requested - (int)effective;
	if (err < 0)
		err = - err;

	/* 1% error acceptable */
	if (err > (int)requested / 100) {
		return -1;
	}

	return 0;
}

static adv_error vgaline_mode_generate_text(vgaline_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{

	mode->is_text = 1;
	mode->crtc = *crtc;

	if (crtc->hde % 9 == 0 && crtc->hrs % 9 == 0 && crtc->hre % 9 == 0 && crtc->ht % 9 == 0) {
		mode->font_x = 9;
	} else if (crtc->hde % 8 == 0 && crtc->hrs % 2 == 0 && crtc->hre % 2 == 0 && crtc->ht % 2 == 0) {
		mode->font_x = 8;
	} else {
		error_nolog_set("Unsupported horizontal crtc values. Only multiple of 8 or 9 pixels are supported.\n");
		return -1;
	}

	if (video_mode_generate_check("vgaline", vgaline_flags(), 1, 1024, crtc, flags)!=0)
		return -1;

	mode->font_y = crtc->vde / 25;
	if (mode->font_y >= 16)
		mode->font_y = 16;
	else if (mode->font_y >= 14)
		mode->font_y = 14;
	else
		mode->font_y = 8;

	return 0;
}

static adv_error vgaline_mode_generate_graphics(vgaline_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	if (video_mode_generate_check("vgaline", vgaline_flags(), 2, 1024, crtc, flags)!=0)
		return -1;

	if (crtc->hde * crtc->vde > 64 * 1024) {
		error_nolog_set("Mode to big for the VGA memory.\n");
		return -1;
	}

	mode->is_text = 0;
	mode->crtc = *crtc;
	mode->font_x = 0;
	mode->font_y = 0;

	return 0;
}

adv_error vgaline_mode_generate(vgaline_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	unsigned pixelclock;

	assert(vgaline_is_active());

	if (crtc_is_fake(crtc)) {
		error_nolog_set("Not programmable modes are not supported.\n");
		return -1;
	}

	switch (flags & MODE_FLAGS_INDEX_MASK) {
	case MODE_FLAGS_INDEX_PALETTE8 :
		if (vgaline_mode_generate_graphics(mode, crtc, flags) != 0)
			return -1;
		break;
	case MODE_FLAGS_INDEX_TEXT :
		if (vgaline_mode_generate_text(mode, crtc, flags) != 0)
			return -1;
		break;
	default :
		return -1;
	}

	pixelclock = crtc->pixelclock;

	/* get the real pixelclock */
	pixelclock = vga_pixelclock_nearest_get(pixelclock, mode->is_text);

	if (vgaline_acceptable_pixelclock(crtc->pixelclock, pixelclock)!=0) {
		error_nolog_set("Pixel clock not supported. Nearest supported value is %d Hz.\n", pixelclock);
		return -1;
	}

	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1;

int vgaline_mode_compare(const vgaline_video_mode* a, const vgaline_video_mode* b)
{
	COMPARE(a->is_text, b->is_text)

	if (a->is_text) {
		COMPARE(a->font_x, b->font_x)
		COMPARE(a->font_y, b->font_y)
	}

	return crtc_compare(&a->crtc, &b->crtc);
}

void vgaline_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:vgaline: vgaline_crtc_container_insert_default()\n"));

	crtc_container_insert_default_bios_vga(cc);
	crtc_container_insert_default_modeline_vga(cc);
}

/***************************************************************************/
/* Driver */

static adv_error vgaline_mode_set_void(const void* mode)
{
	return vgaline_mode_set((const vgaline_video_mode*)mode);
}

static adv_error vgaline_mode_import_void(adv_mode* mode, const void* vgaline_mode)
{
	return vgaline_mode_import(mode, (const vgaline_video_mode*)vgaline_mode);
}

static adv_error vgaline_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return vgaline_mode_generate((vgaline_video_mode*)mode, crtc, flags);
}

static int vgaline_mode_compare_void(const void* a, const void* b)
{
	return vgaline_mode_compare((const vgaline_video_mode*)a, (const vgaline_video_mode*)b);
}

static unsigned vgaline_mode_size(void)
{
	return sizeof(vgaline_video_mode);
}

static adv_error vgaline_mode_grab_void(void* mode)
{
	return vgaline_mode_grab((vgaline_video_mode*)mode);
}

static void vgaline_reg_dummy(adv_conf* context)
{
}

static adv_error vgaline_load_dummy(adv_conf* context)
{
	return 0;
}

static adv_error vgaline_scroll(unsigned offset, adv_bool waitvsync)
{
	if (waitvsync)
		vga_wait_vsync();
	vga_scroll(offset);
	return 0;
}

static adv_error vgaline_scanline_set(unsigned byte_length)
{
	vga_scanline_set(byte_length);
	return 0;
}

adv_video_driver video_vgaline_driver = {
	"vgaline",
	DEVICE,
	vgaline_load_dummy,
	vgaline_reg_dummy,
	vgaline_init,
	vgaline_done,
	vgaline_flags,
	vgaline_mode_set_void,
	0,
	vgaline_mode_done,
	vga_virtual_x,
	vga_virtual_y,
	vga_font_size_x,
	vga_font_size_y,
	vga_bytes_per_scanline,
	vga_adjust_bytes_per_page,
	vgaline_rgb_def,
	0,
	0,
	&vgaline_write_line,
	vga_wait_vsync,
	vgaline_scroll,
	vgaline_scanline_set,
	vga_palette8_set,
	vgaline_mode_size,
	vgaline_mode_grab_void,
	vgaline_mode_generate_void,
	vgaline_mode_import_void,
	vgaline_mode_compare_void,
	vgaline_crtc_container_insert_default
};

