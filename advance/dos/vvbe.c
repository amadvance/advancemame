/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2001 Andrea Mazzoleni
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

#include "vvbe.h"
#include "scrvbe.h"
#include "video.h"

#include <stdlib.h>
#include <stdio.h>

static device DEVICE[] = {
{ "auto", 1, "VBE video" },
{ 0, 0, 0 }
};

static video_error vbe_init2(int device_id) {
	const device* i = DEVICE;
	while (i->name && i->id != device_id)
		++i;
	if (!i->name)
		return -1;

	return vbe_init();
}

unsigned vbe_flags(void) {
	return VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_SINGLESCAN
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
}

/**
 * Grab the current video mode.
 * \return 0 if successful
 */
video_error vbe_mode_grab(vbe_video_mode* mode) {
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
 * \param mode->driver.vbe.mode video mode VBE number
 * \return 0 if successful
 */
video_error vbe_mode_import(video_mode* mode, const vbe_video_mode* vbe_mode) {
	vbe_ModeInfoBlock info;

	sprintf(mode->name,"vbe_bios_%x",vbe_mode->mode);
	*DRIVER(mode) = *vbe_mode;

	if (vbe_mode_info_get(&info, DRIVER(mode)->mode)!=0) {
		video_error_description_set("VBE report that mode %d is unsupported", DRIVER(mode)->mode);
		return -1;
	}
	if ((info.ModeAttributes & vbeMdAvailable) == 0) {
		video_error_description_set("VBE report that mode %d is not avaliable", DRIVER(mode)->mode);
		return -1;
	}

	if ((info.ModeAttributes & vbeMdGraphMode) == 0) {
		video_error_description_nolog_set("Text modes are unsupported");
		return -1;
	} else {
		if ((DRIVER(mode)->mode & vbeLinearBuffer) != 0) {
			if (vbe_state.info.VESAVersion < 0x200) {
				video_error_description_nolog_set("Linear frame buffer not available on VBE version prior 2.0");
				return -1;
			}
			if ((info.ModeAttributes & vbeMdLinear) == 0) {
				video_error_description_nolog_set("Linear frame buffer not available in this mode");
				return -1;
			}
		} else {
			if ((info.ModeAttributes & vbeMdNonBanked) != 0) {
				video_error_description_nolog_set("Banked frame buffer not available in this mode");
				return -1;
			}
		}
		/* Packed or RGB memory model */
		if (info.MemoryModel!=vbeMemPK && info.MemoryModel!=vbeMemRGB) {
			video_error_description_nolog_set("Unsupported memory model");
			return -1;
		}
		/* Non planar mode */
		if (info.NumberOfPlanes!=1) {
			video_error_description_nolog_set("Unsupported number of planes");
			return -1;
		}
	}

	mode->driver = &video_vbe_driver;
	mode->flags = VIDEO_FLAGS_ASYNC_SETPAGE |
		(mode->flags & VIDEO_FLAGS_USER_MASK);
	if ((info.ModeAttributes & vbeMdTripleBuffer) != 0)
		mode->flags |= VIDEO_FLAGS_SYNC_SETPAGE;
	switch (info.MemoryModel) {
		case vbeMemTXT :
			mode->flags |= VIDEO_FLAGS_INDEX_TEXT | VIDEO_FLAGS_TYPE_TEXT;
			break;
		case vbeMemPK :
			mode->flags |= VIDEO_FLAGS_INDEX_PACKED | VIDEO_FLAGS_TYPE_GRAPHICS;
			break;
		case vbeMemRGB :
			mode->flags |= VIDEO_FLAGS_INDEX_RGB | VIDEO_FLAGS_TYPE_GRAPHICS;
			break;
		default :
			return -1;
	}
	if (DRIVER(mode)->mode & vbeLinearBuffer)
		mode->flags |= VIDEO_FLAGS_MEMORY_LINEAR;
	else
		mode->flags |= VIDEO_FLAGS_MEMORY_BANKED;

	mode->size_x = info.XResolution;
	mode->size_y = info.YResolution;
	if (info.MemoryModel==vbeMemTXT) {
		mode->size_x *= info.XCharSize;
		mode->size_y *= info.YCharSize;
	}
	mode->vclock = 0;
	mode->hclock = 0;
	mode->bits_per_pixel = info.BitsPerPixel;
	if (info.YResolution <= 300)
		mode->scan = 1; /* assume doublescan */
	else
		mode->scan = 0; /* assume singlescan */

	return 0;
}

video_error vbe_palette8_set(const video_color* palette, unsigned start, unsigned count, video_bool waitvsync) {
	video_color vbe_pal[256];
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

	return vbe_palette_set(vbe_pal,start,count,waitvsync);
}

static video_error vbe_search_target_mode(unsigned req_x, unsigned req_y, unsigned bits, unsigned model, unsigned flags) {
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

video_error vbe_mode_generate(vbe_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	int number;
	unsigned model;
	unsigned vbeflags = vbeMdAvailable | vbeMdGraphMode | vbeMdLinear;

	assert( vbe_is_active() );

	if (video_mode_generate_check("vbe",vbe_flags(),1,2048,crtc,bits,flags)!=0)
		return -1;

	switch (flags & VIDEO_FLAGS_INDEX_MASK) {
		case VIDEO_FLAGS_INDEX_RGB :
			model = vbeMemRGB;
			break;
		case VIDEO_FLAGS_INDEX_PACKED :
			model = vbeMemPK;
			break;
		default:
			assert(0);
			return -1;
	}

	number = vbe_search_target_mode(crtc->hde,crtc->vde,bits,model,vbeflags);
	if (number < 0 && model == vbeMemRGB) {
		model = vbeMemPK; /* the packed mode is better than RGB */
		number = vbe_search_target_mode(crtc->hde,crtc->vde,bits,model,vbeflags);
	}
	if (number < 0) {
		video_error_description_nolog_cat("vbe: No compatible VBE mode found\n");
		return -1;
	}

	mode->mode = number;
	return 0;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1;

int vbe_mode_compare(const vbe_video_mode* a, const vbe_video_mode* b) {
	COMPARE(a->mode,b->mode)
	return 0;
}

void vbe_crtc_container_insert_default(video_crtc_container* cc) {
	vbe_mode_iterator i;

	video_log("video:vbe: vbe_crtc_container_insert_default()\n");

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

			video_crtc crtc;
			crtc_fake_set(&crtc, info.XResolution, info.YResolution);

			video_log("video:vbe: mode %dx%d\n", (unsigned)info.XResolution, (unsigned)info.YResolution);

			video_crtc_container_insert(cc, &crtc);
		}

		vbe_mode_iterator_next(&i);
	}
}

/***************************************************************************/
/* Driver */

static video_error vbe_mode_set_void(const void* mode) {
	return vbe_mode_set(((const vbe_video_mode*)mode)->mode,0);
}

static video_error vbe_mode_import_void(video_mode* mode, const void* vbe_mode) {
	return vbe_mode_import(mode, (const vbe_video_mode*)vbe_mode);
}

static video_error vbe_mode_generate_void(void* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	return vbe_mode_generate((vbe_video_mode*)mode, crtc, bits, flags);
}

static int vbe_mode_compare_void(const void* a, const void* b) {
	return vbe_mode_compare((const vbe_video_mode*)a, (const vbe_video_mode*)b);
}

static void vbe_reg_dummy(struct conf_context* context) {
}

static video_error vbe_load_dummy(struct conf_context* context) {
	return 0;
}

static unsigned vbe_mode_size(void) {
	return sizeof(vbe_video_mode);
}

static video_error vbe_mode_grab_void(void* mode) {
	return vbe_mode_grab((vbe_video_mode*)mode);
}

video_driver video_vbe_driver = {
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
	vbe_rgb_def,
	0,
	0,
	&vbe_write_line,
	vbe_wait_vsync,
	vbe_scroll,
	vbe_scanline_set,
	vbe_palette8_set,
	0,
	vbe_mode_size,
	vbe_mode_grab_void,
	vbe_mode_generate_void,
	vbe_mode_import_void,
	vbe_mode_compare_void,
	vbe_crtc_container_insert_default
};

