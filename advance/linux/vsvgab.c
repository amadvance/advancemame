/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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

#include "vsvgab.h"
#include "video.h"
#include "log.h"
#include "osint.h"

#include <vga.h>
#include <string.h>

/***************************************************************************/
/* State */

typedef struct svgalib_internal_struct {
	video_bool active;
	video_bool mode_active;
	unsigned mode_number;
	unsigned memory_size;
	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;
	unsigned char* ptr;
	unsigned red_len;
	unsigned red_pos;
	unsigned green_len;
	unsigned green_pos;
	unsigned blue_len;
	unsigned blue_pos;
} svgalib_internal;

static svgalib_internal svgalib_state;

unsigned char* (*svgalib_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

static unsigned char* svgalib_linear_write_line(unsigned y) {
	return svgalib_state.ptr + svgalib_state.bytes_per_scanline * y;
}

/***************************************************************************/
/* Public */

static device DEVICE[] = {
{ "auto", -1, "SVGALIB video" },
{ 0, 0, 0 }
};

video_error svgalib_init(int device_id) {
	int res;

	/* assume that vga_init() is already called */
	assert( !svgalib_is_active() );

	log_std(("video:svgalib: svgalib_init()\n"));

	if (!os_internal_svgalib_get()) {
		log_std(("video:svgalib: svgalib not initialized\n"));
		return -1;
	}

	/* check the version of the SVGALIB */
	res = vga_setmode(-1);
	if (res < 0 || res < 0x1911) { /* 1.9.11 */
		video_error_description_set("You need SVGALIB 1.9.11 or newer");
		return -1;
	}

	svgalib_state.active = 1;
	return 0;
}

void svgalib_done(void) {
	assert(svgalib_is_active() && !svgalib_mode_is_active() );

	log_std(("video:svgalib: svgalib_done()\n"));

	svgalib_state.active = 0;
}

video_bool svgalib_is_active(void) {
	return svgalib_state.active != 0;
}

video_bool svgalib_mode_is_active(void) {
	return svgalib_state.mode_active != 0;
}

unsigned svgalib_flags(void) {
	assert( svgalib_is_active() );
	return VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL;
}

/* Flags copied from svgalib/src/timings.h */
#define SVGALIB_PHSYNC          0x1     /* Positive hsync polarity. */
#define SVGALIB_NHSYNC          0x2     /* Negative hsync polarity. */
#define SVGALIB_PVSYNC          0x4     /* Positive vsync polarity. */
#define SVGALIB_NVSYNC          0x8     /* Negative vsync polarity. */
#define SVGALIB_INTERLACED      0x10    /* Mode has interlaced timing. */
#define SVGALIB_DOUBLESCAN      0x20    /* Mode uses VGA doublescan (see note). */
#if 0
#define SVGALIB_HADJUSTED       0x40    /* Horizontal CRTC timing adjusted. */
#define SVGALIB_VADJUSTED       0x80    /* Vertical CRTC timing adjusted. */
#define SVGALIB_USEPROGRCLOCK   0x100   /* A programmable clock is used. */
#endif
#define SVGALIB_TVMODE          0x200   /* Enable the TV output */
#define SVGALIB_TVPAL           0x400   /* Use the PAL format */
#define SVGALIB_TVNTSC          0x800   /* Use the NTSC format */

video_error svgalib_mode_set(const svgalib_video_mode* mode) 
{
	int res;
	int flags;
	vga_modeinfo* modeinfo;
	unsigned colors;
	unsigned bytes_per_pixel;
	unsigned bytes_per_scanline;

	assert( svgalib_is_active() && !svgalib_mode_is_active() );

	log_std(("video:svgalib: svgalib_mode_set()\n"));

	flags = 0;
	if (crtc_is_doublescan(&mode->crtc))
		flags |= SVGALIB_DOUBLESCAN;
	if (crtc_is_interlace(&mode->crtc))
		flags |= SVGALIB_INTERLACED;
	if (crtc_is_nhsync(&mode->crtc))
		flags |= SVGALIB_NHSYNC;
	else
		flags |= SVGALIB_PHSYNC;
	if (crtc_is_nvsync(&mode->crtc))
		flags |= SVGALIB_NVSYNC;
	else
		flags |= SVGALIB_PVSYNC;
	if (crtc_is_tvpal(&mode->crtc))
		flags |= SVGALIB_TVMODE | SVGALIB_TVPAL;
	if (crtc_is_tvntsc(&mode->crtc))
		flags |= SVGALIB_TVMODE | SVGALIB_TVNTSC;

	res = vga_addtiming(mode->crtc.pixelclock / 1000, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, flags);
	if (res != 1) {
		video_error_description_set("Error in vga_addtiming()");
		return -1;
	}

	video_log("video:svgalib: vga_addtiming(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n",
		mode->crtc.pixelclock / 1000, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, flags
	);

	switch (mode->bits_per_pixel) {
		case 8 :
			bytes_per_pixel = 1;
			colors = 1 << 8;
		break;
		case 15 :
			bytes_per_pixel = 2;
			colors = 1 << 15;
		break;
		case 16 :
			bytes_per_pixel = 2;
			colors = 1 << 16;
		break;
		case 24 :
			bytes_per_pixel = 3;
			colors = 1 << 24;
		break;
		case 32 :
			bytes_per_pixel = 4;
			colors = 1 << 24;
		break;
		default:
			video_error_description_set("Invalid bit depth");
			return -1;
	}
	bytes_per_scanline = bytes_per_pixel * mode->crtc.hde;

	res = vga_addmode(mode->crtc.hde, mode->crtc.vde, colors, bytes_per_scanline, bytes_per_pixel);
	if (res<0) {
		video_error_description_set("Error in vga_addmode()");
		return -1;
	}

	video_log("video:svgalib: vga_addmode(%d,%d,%d,%d,%d) = %d\n",mode->crtc.hde,mode->crtc.vde,colors,bytes_per_scanline,bytes_per_pixel,res);

	if (!vga_hasmode(res)) {
		video_error_description_set("Error in vga_hasmode()");
		return -1;
	}

	svgalib_state.mode_number = res;

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		video_error_description_set("Error in vga_getmodeinfo()");
		return -1;
	}

	if ((modeinfo->flags & CAPABLE_LINEAR) == 0) {
		video_error_description_set("Linear mode not supported");
		return -1;
	}

	video_log("video:svgalib: vga_setmode(%d)\n",svgalib_state.mode_number);
	res = vga_setmode(svgalib_state.mode_number);
	if (res != 0) {
		video_error_description_set("Error in vga_setmode()");
		return -1;
	}

	res = vga_setlinearaddressing();
	if (res <= 0) {
		video_error_description_set("Error in vga_setlinearaddressing()");
		return -1;
	}
	svgalib_state.memory_size = res;

	svgalib_state.ptr = vga_getgraphmem();

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		video_error_description_set("Error in vga_getmodeinfo()");
		return -1;
	}
	svgalib_state.bytes_per_pixel = modeinfo->bytesperpixel;
	svgalib_state.bytes_per_scanline = modeinfo->linewidth;
	switch (modeinfo->colors) {
		case 1 << 8 :
			svgalib_state.red_len = 0;
			svgalib_state.red_pos = 0;
			svgalib_state.green_len = 0;
			svgalib_state.green_pos = 0;
			svgalib_state.blue_len = 0;
			svgalib_state.blue_pos = 0;
		break;
		case 1 << 15 :
			if ((modeinfo->flags & RGB_MISORDERED) == 0) {
				svgalib_state.red_len = 5;
				svgalib_state.red_pos = 10;
				svgalib_state.green_len = 5;
				svgalib_state.green_pos = 5;
				svgalib_state.blue_len = 5;
				svgalib_state.blue_pos = 0;
			} else {
				svgalib_state.red_len = 5;
				svgalib_state.red_pos = 0;
				svgalib_state.green_len = 5;
				svgalib_state.green_pos = 5;
				svgalib_state.blue_len = 5;
				svgalib_state.blue_pos = 10;
			}
		break;
		case 1 << 16 :
			svgalib_state.red_len = 5;
			svgalib_state.red_pos = 11;
			svgalib_state.green_len = 6;
			svgalib_state.green_pos = 5;
			svgalib_state.blue_len = 5;
			svgalib_state.blue_pos = 0;
		break;
		case 1 << 24 :
			svgalib_state.red_len = 8;
			svgalib_state.red_pos = 16;
			svgalib_state.green_len = 8;
			svgalib_state.green_pos = 8;
			svgalib_state.blue_len = 8;
			svgalib_state.blue_pos = 0;
		break;
	}

	svgalib_write_line = svgalib_linear_write_line;

	svgalib_state.mode_active = 1;

	return 0;
}

video_error svgalib_mode_change(const svgalib_video_mode* mode) {
	assert( svgalib_is_active() && svgalib_mode_is_active() );

	log_std(("video:svgalib: svgalib_mode_change()\n"));

	svgalib_state.mode_active = 0; /* fake done */
	return svgalib_mode_set(mode);
}

void svgalib_mode_done(video_bool restore) {
	assert( svgalib_is_active() && svgalib_mode_is_active() );

	log_std(("video:svgalib: svgalib_mode_done()\n"));

	svgalib_state.mode_active = 0;

	if (restore) {
		/* a mode reset is required otherwise the other drivers are not able to set the mode */
		video_log("video:svgalib: vga_setmode(TEXT)\n");
		vga_setmode(TEXT);
	}
}

unsigned svgalib_virtual_x(void) {
	unsigned size = svgalib_state.bytes_per_scanline / svgalib_state.bytes_per_pixel;
	size = size & ~0x7;
	return size;
}

unsigned svgalib_virtual_y(void) {
	return svgalib_state.memory_size / svgalib_state.bytes_per_scanline;
}

unsigned svgalib_adjust_bytes_per_page(unsigned bytes_per_page) {
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned svgalib_bytes_per_scanline(void) {
	return svgalib_state.bytes_per_scanline;
}

video_rgb_def svgalib_rgb_def(void) {
	return video_rgb_def_make(svgalib_state.red_len,svgalib_state.red_pos,svgalib_state.green_len,svgalib_state.green_pos,svgalib_state.blue_len,svgalib_state.blue_pos);
}

void svgalib_wait_vsync(void) {
	assert(svgalib_is_active() && svgalib_mode_is_active());
	vga_waitretrace();
}

video_error svgalib_scroll(unsigned offset, video_bool waitvsync) {
	assert(svgalib_is_active() && svgalib_mode_is_active());

	if (waitvsync)
		vga_waitretrace();

	vga_setdisplaystart(offset);

	return 0;
}

video_error svgalib_scanline_set(unsigned byte_length) {
	vga_modeinfo* modeinfo;
	assert(svgalib_is_active() && svgalib_mode_is_active());

	vga_setlogicalwidth(byte_length);

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		video_error_description_set("Error in vga_getmodeinfo()");
		return -1;
	}
	svgalib_state.bytes_per_pixel = modeinfo->bytesperpixel;
	svgalib_state.bytes_per_scanline = modeinfo->linewidth;	

	return 0;
}

video_error svgalib_palette8_set(const video_color* palette, unsigned start, unsigned count, video_bool waitvsync) {
	if (waitvsync)
		vga_waitretrace();

	while (count) {
		vga_setpalette(start, palette->red >> 2, palette->green >> 2, palette->blue >> 2);
		++palette;
		++start;
		--count;
	}

	return 0;
}

#define DRIVER(mode) ((svgalib_video_mode*)(&mode->driver_mode))

video_error svgalib_mode_import(video_mode* mode, const svgalib_video_mode* svgalib_mode)
{
	strcpy(mode->name, svgalib_mode->crtc.name);

	*DRIVER(mode) = *svgalib_mode;

	mode->driver = &video_svgalib_driver;
	mode->flags = VIDEO_FLAGS_ASYNC_SETPAGE
		| VIDEO_FLAGS_MEMORY_LINEAR
		| (mode->flags & VIDEO_FLAGS_USER_MASK);
	switch (svgalib_mode->bits_per_pixel) {
		case 8 : mode->flags |= VIDEO_FLAGS_INDEX_PACKED | VIDEO_FLAGS_TYPE_GRAPHICS; break;
		default: mode->flags |= VIDEO_FLAGS_INDEX_RGB | VIDEO_FLAGS_TYPE_GRAPHICS; break;
	}

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->bits_per_pixel = svgalib_mode->bits_per_pixel;
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

video_error svgalib_mode_generate(svgalib_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags)
{
	assert( svgalib_is_active() );

	if (video_mode_generate_check("svgalib",svgalib_flags(),8,2048,crtc,bits,flags)!=0)
		return -1;

	mode->crtc = *crtc;
	mode->bits_per_pixel = bits;

	return 0;
}

#define COMPARE(a,b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int svgalib_mode_compare(const svgalib_video_mode* a, const svgalib_video_mode* b) {
	COMPARE(a->bits_per_pixel,b->bits_per_pixel);
	return video_crtc_compare(&a->crtc,&b->crtc);
}

void svgalib_default(void) {
}

void svgalib_reg(struct conf_context* context) {
	assert( !svgalib_is_active() );
}

video_error svgalib_load(struct conf_context* context) {
	assert( !svgalib_is_active() );
	return 0;
}

/***************************************************************************/
/* Driver */

static video_error svgalib_mode_set_void(const void* mode) {
	return svgalib_mode_set((const svgalib_video_mode*)mode);
}

static video_error svgalib_mode_change_void(const void* mode) {
	return svgalib_mode_change((const svgalib_video_mode*)mode);
}

static video_error svgalib_mode_import_void(video_mode* mode, const void* svgalib_mode) {
	return svgalib_mode_import(mode, (const svgalib_video_mode*)svgalib_mode);
}

static video_error svgalib_mode_generate_void(void* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	return svgalib_mode_generate((svgalib_video_mode*)mode,crtc,bits,flags);
}

static int svgalib_mode_compare_void(const void* a, const void* b) {
	return svgalib_mode_compare((const svgalib_video_mode*)a, (const svgalib_video_mode*)b);
}

static unsigned svgalib_mode_size(void) {
	return sizeof(svgalib_video_mode);
}

video_driver video_svgalib_driver = {
	"svgalib",
	DEVICE,
	svgalib_load,
	svgalib_reg,
	svgalib_init,
	svgalib_done,
	svgalib_flags,
	svgalib_mode_set_void,
	svgalib_mode_change_void,
	svgalib_mode_done,
	svgalib_virtual_x,
	svgalib_virtual_y,
	0,
	0,
	svgalib_bytes_per_scanline,
	svgalib_adjust_bytes_per_page,
	svgalib_rgb_def,
	0,
	0,
	&svgalib_write_line,
	svgalib_wait_vsync,
	svgalib_scroll,
	svgalib_scanline_set,
	svgalib_palette8_set,
	0,
	svgalib_mode_size,
	0,
	svgalib_mode_generate_void,
	svgalib_mode_import_void,
	svgalib_mode_compare_void,
	0
};

