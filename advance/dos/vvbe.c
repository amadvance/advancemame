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

#include "vvbe.h"
#include "scrvbe.h"
#include "video.h"
#include "log.h"
#include "error.h"
#include "osdos.h"

static adv_device DEVICE[] = {
{ "auto", 1, "VBE video" },
{ 0, 0, 0 }
};

typedef struct vbe_internal_struct2 {
	unsigned flags;
} vbe_internal2;

static vbe_internal2 vbe_state2;

static void vbe_probe(void)
{
	unsigned flags = vbeMdAvailable | vbeMdGraphMode | vbeMdLinear;

	adv_bool has8bit = 0;
	adv_bool has15bit = 0;
	adv_bool has16bit = 0;
	adv_bool has24bit = 0;
	adv_bool has32bit = 0;

	vbe_mode_iterator i;
	vbe_mode_iterator_begin(&i);

	while (!vbe_mode_iterator_end(&i)) {
		unsigned mode;
		vbe_ModeInfoBlock info;

		mode = vbe_mode_iterator_get(&i) | vbeLinearBuffer;

		if (vbe_mode_info_get(&info, mode) == 0
			&& (info.ModeAttributes & flags) == flags
			&& info.NumberOfPlanes == 1
			&& (info.MemoryModel == vbeMemRGB || info.MemoryModel == vbeMemPK)) {

			switch (info.BitsPerPixel) {
				case 8 : has8bit = 1; break;
				case 15 : has15bit = 1; break;
				case 16 : has16bit = 1; break;
				case 24 : has24bit = 1; break;
				case 32 : has32bit = 1; break;
			}
		}

		vbe_mode_iterator_next(&i);
	}

	/* remove unsupported bit depth */
	if (!has8bit)
		vbe_state2.flags &= ~VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
	if (!has15bit)
		vbe_state2.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR15;
	if (!has16bit)
		vbe_state2.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR16;
	if (!has24bit)
		vbe_state2.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR24;
	if (!has32bit)
		vbe_state2.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR32;
}

static adv_error vbe_init2(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	const adv_device* i;

	(void)cursor;

	if (sizeof(vbe_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (os_internal_brokenint10_active()) {
		error_set("Detected broken video BIOS.\n");
		return -1;
	}

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	i = DEVICE;
	while (i->name && i->id != device_id)
		++i;
	if (!i->name)
		return -1;

	if (vbe_init() != 0)
		return -1;

	vbe_state2.flags = VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32
		| VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;

	vbe_probe();

	return 0;
}

unsigned vbe_flags(void)
{
	return vbe_state2.flags;
}

/**
 * Grab the current video mode.
 * \return 0 if successful
 */
adv_error vbe_mode_grab(vbe_video_mode* mode)
{
	if (!vbe_is_active())
		return -1;

	if (vbe_mode_get(&mode->mode)!=0)
		return -1;

	if (mode->mode < 0x100)
		return -1;

	return 0;
}

#define DRIVER(mode) ((vbe_video_mode*)(&mode->driver_mode))

/**
 * Import information for one video mode.
 * \param mode Mode to write.
 * \param vbe_mode Mode to import.
 * \return 0 if successful
 */
adv_error vbe_mode_import(adv_mode* mode, const vbe_video_mode* vbe_mode)
{
	vbe_ModeInfoBlock info;

	snprintf(mode->name, MODE_NAME_MAX, "vbe_bios_%x", vbe_mode->mode);
	*DRIVER(mode) = *vbe_mode;

	if (vbe_mode_info_get(&info, DRIVER(mode)->mode)!=0) {
		error_set("VBE report that mode %d is unsupported", DRIVER(mode)->mode);
		return -1;
	}
	if ((info.ModeAttributes & vbeMdAvailable) == 0) {
		error_set("VBE report that mode %d is not avaliable", DRIVER(mode)->mode);
		return -1;
	}

	if ((info.ModeAttributes & vbeMdGraphMode) == 0) {
		error_nolog_set("Text modes are unsupported");
		return -1;
	} else {
		if ((DRIVER(mode)->mode & vbeLinearBuffer) != 0) {
			if (vbe_state.info.VESAVersion < 0x200) {
				error_nolog_set("Linear frame buffer not available on VBE version prior 2.0");
				return -1;
			}
			if ((info.ModeAttributes & vbeMdLinear) == 0) {
				error_nolog_set("Linear frame buffer not available in this mode");
				return -1;
			}
		} else {
			if ((info.ModeAttributes & vbeMdNonBanked) != 0) {
				error_nolog_set("Banked frame buffer not available in this mode");
				return -1;
			} else {
				error_nolog_set("Banked frame buffer isn't supported");
				return -1;
			}
		}
		/* Packed or RGB memory model */
		if (info.MemoryModel!=vbeMemPK && info.MemoryModel!=vbeMemRGB) {
			error_nolog_set("Unsupported memory model");
			return -1;
		}
		/* Non planar mode */
		if (info.NumberOfPlanes!=1) {
			error_nolog_set("Unsupported number of planes");
			return -1;
		}
	}

	mode->driver = &video_vbe_driver;
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC | MODE_FLAGS_RETRACE_SET_SYNC
		| (mode->flags & MODE_FLAGS_USER_MASK);
	if ((info.ModeAttributes & vbeMdTripleBuffer) != 0)
		mode->flags |= MODE_FLAGS_RETRACE_SET_ASYNC;
	switch (info.MemoryModel) {
		case vbeMemTXT :
			mode->flags |= MODE_FLAGS_INDEX_TEXT;
			break;
		case vbeMemPK :
			mode->flags |= MODE_FLAGS_INDEX_PALETTE8;
			break;
		case vbeMemRGB :
			switch (info.BitsPerPixel) {
			case 15 : mode->flags |= MODE_FLAGS_INDEX_BGR15; break;
			case 16 : mode->flags |= MODE_FLAGS_INDEX_BGR16; break;
			case 24 : mode->flags |= MODE_FLAGS_INDEX_BGR24; break;
			case 32 : mode->flags |= MODE_FLAGS_INDEX_BGR32; break;
			default:
				return -1;
			}
			break;
		default :
			return -1;
	}

	mode->size_x = info.XResolution;
	mode->size_y = info.YResolution;
	if (info.MemoryModel==vbeMemTXT) {
		mode->size_x *= info.XCharSize;
		mode->size_y *= info.YCharSize;
	}
	mode->vclock = 0;
	mode->hclock = 0;
	if (info.YResolution <= 300)
		mode->scan = 1; /* assume doublescan */
	else
		mode->scan = 0; /* assume singlescan */

	return 0;
}

adv_error vbe_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
	adv_color_rgb vbe_pal[256];
	unsigned shift = 8 - vbe_state.palette_width;

	if (shift) {
		unsigned i;
		for(i=0;i<count;++i) {
			vbe_pal[i].red = palette[i].red >> shift;
			vbe_pal[i].green = palette[i].green >> shift;
			vbe_pal[i].blue = palette[i].blue >> shift;
			vbe_pal[i].alpha = 0;
		}
	} else {
		unsigned i;
		for(i=0;i<count;++i) {
			vbe_pal[i].red = palette[i].red;
			vbe_pal[i].green = palette[i].green;
			vbe_pal[i].blue = palette[i].blue;
			vbe_pal[i].alpha = 0;
		}
	}

	return vbe_palette_set(vbe_pal, start, count, waitvsync);
}

static adv_error vbe_search_target_mode(unsigned req_x, unsigned req_y, unsigned bits, unsigned model, unsigned flags)
{
	vbe_mode_iterator i;

	vbe_mode_iterator_begin(&i);
	while (!vbe_mode_iterator_end(&i)) {
		unsigned mode;
		vbe_ModeInfoBlock info;

		mode = vbe_mode_iterator_get(&i) | vbeLinearBuffer;

		if (vbe_mode_info_get(&info, mode) == 0
			&& (info.ModeAttributes & flags) == flags
			&& info.BitsPerPixel == bits
			&& info.NumberOfPlanes == 1
			&& info.MemoryModel == model
			&& info.XResolution == req_x
			&& info.YResolution == req_y) {
			return mode;
		}

		vbe_mode_iterator_next(&i);
	}

	return -1;
}

adv_error vbe_mode_generate(vbe_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	int number;
	unsigned model;
	unsigned vbeflags = vbeMdAvailable | vbeMdGraphMode | vbeMdLinear;
	unsigned bits;

	assert(vbe_is_active());

	if (!crtc_is_fake(crtc)) {
		error_nolog_set("Programmable modes not supported.\n");
		return -1;
	}

	switch (flags & MODE_FLAGS_INDEX_MASK) {
		case MODE_FLAGS_INDEX_PALETTE8 :
			bits = 8;
			model = vbeMemPK;
			break;
		case MODE_FLAGS_INDEX_BGR15 :
			bits = 15;
			model = vbeMemRGB;
			break;
		case MODE_FLAGS_INDEX_BGR16 :
			bits = 16;
			model = vbeMemRGB;
			break;
		case MODE_FLAGS_INDEX_BGR24 :
			bits = 24;
			model = vbeMemRGB;
			break;
		case MODE_FLAGS_INDEX_BGR32 :
			bits = 32;
			model = vbeMemRGB;
			break;
		default:
			assert(0);
			return -1;
	}

	number = vbe_search_target_mode(crtc->hde, crtc->vde, bits, model, vbeflags);
	if (number < 0 && model == vbeMemRGB) {
		model = vbeMemPK; /* the packed mode is better than RGB */
		number = vbe_search_target_mode(crtc->hde, crtc->vde, bits, model, vbeflags);
	}
	if (number < 0) {
		error_nolog_set("No compatible VBE mode found.\n");
		return -1;
	}

	mode->mode = number;
	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1;

int vbe_mode_compare(const vbe_video_mode* a, const vbe_video_mode* b)
{
	COMPARE(a->mode, b->mode)
	return 0;
}

void vbe_crtc_container_insert_default(adv_crtc_container* cc)
{
	vbe_mode_iterator i;

	log_std(("video:vbe: vbe_crtc_container_insert_default()\n"));

	vbe_mode_iterator_begin(&i);
	while (!vbe_mode_iterator_end(&i)) {
		unsigned mode;
		vbe_ModeInfoBlock info;
		unsigned flags = vbeMdAvailable | vbeMdGraphMode | vbeMdLinear;

		mode = vbe_mode_iterator_get(&i) | vbeLinearBuffer;

		if (vbe_mode_info_get(&info, mode) == 0
			&& (info.ModeAttributes & flags) == flags
			&& info.NumberOfPlanes == 1
			&& (info.MemoryModel == vbeMemRGB || info.MemoryModel == vbeMemPK)) {
			adv_crtc_container_iterator j;

			adv_crtc crtc;
			crtc_fake_set(&crtc, info.XResolution, info.YResolution);

			/* insert only if not already present, generally a mode is listed for many bit depths */
			crtc_container_iterator_begin(&j, cc);
			while (!crtc_container_iterator_is_end(&j)) {
				adv_crtc* crtc_in = crtc_container_iterator_get(&j);
				if (crtc_in->hde == crtc.hde && crtc_in->vde == crtc.vde && crtc_is_fake(crtc_in))
					break;
				crtc_container_iterator_next(&j);
			}

			if (crtc_container_iterator_is_end(&j)) {
				log_std(("video:vbe: mode %dx%d\n", (unsigned)info.XResolution, (unsigned)info.YResolution));
				crtc_container_insert(cc, &crtc);
			}
		}

		vbe_mode_iterator_next(&i);
	}
}

/***************************************************************************/
/* Driver */

static adv_error vbe_mode_set_void(const void* mode)
{
	return vbe_mode_set(((const vbe_video_mode*)mode)->mode, 0);
}

static adv_error vbe_mode_import_void(adv_mode* mode, const void* vbe_mode)
{
	return vbe_mode_import(mode, (const vbe_video_mode*)vbe_mode);
}

static adv_error vbe_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return vbe_mode_generate((vbe_video_mode*)mode, crtc, flags);
}

static int vbe_mode_compare_void(const void* a, const void* b)
{
	return vbe_mode_compare((const vbe_video_mode*)a, (const vbe_video_mode*)b);
}

static void vbe_reg_dummy(adv_conf* context)
{
}

static adv_error vbe_load_dummy(adv_conf* context)
{
	return 0;
}

static unsigned vbe_mode_size(void)
{
	return sizeof(vbe_video_mode);
}

static adv_error vbe_mode_grab_void(void* mode)
{
	return vbe_mode_grab((vbe_video_mode*)mode);
}

adv_video_driver video_vbe_driver = {
	"vbe",
	DEVICE,
	vbe_load_dummy,
	vbe_reg_dummy,
	vbe_init2,
	vbe_done,
	vbe_flags,
	vbe_mode_set_void,
	0,
	vbe_mode_done,
	vbe_virtual_x,
	vbe_virtual_y,
	vbe_font_size_x,
	vbe_font_size_y,
	vbe_bytes_per_scanline,
	vbe_adjust_bytes_per_page,
	vbe_color_def,
	0,
	0,
	&vbe_write_line,
	vbe_wait_vsync,
	vbe_scroll,
	vbe_scanline_set,
	vbe_palette8_set,
	vbe_mode_size,
	vbe_mode_grab_void,
	vbe_mode_generate_void,
	vbe_mode_import_void,
	vbe_mode_compare_void,
	vbe_crtc_container_insert_default
};

