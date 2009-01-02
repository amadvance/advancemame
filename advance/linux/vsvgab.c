/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
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

#include "vsvgab.h"
#include "video.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "snstring.h"

#include <vga.h>

/***************************************************************************/
/* State */

typedef struct svgalib_internal_struct {
	adv_bool active;
	adv_bool mode_active;
	unsigned mode_number;
	unsigned memory_size;
	unsigned index;
	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;
	unsigned char* ptr;
	unsigned red_len;
	unsigned red_pos;
	unsigned green_len;
	unsigned green_pos;
	unsigned blue_len;
	unsigned blue_pos;
	unsigned flags;
} svgalib_internal;

static svgalib_internal svgalib_state;

unsigned char* (*svgalib_write_line)(unsigned y);

static adv_device DEVICE[] = {
{ "auto", -1, "SVGALIB video" },
{ 0, 0, 0 }
};

/***************************************************************************/
/* Functions */

static adv_bool svgalib_is_active(void)
{
	return svgalib_state.active != 0;
}

static adv_bool svgalib_mode_is_active(void)
{
	return svgalib_state.mode_active != 0;
}

static unsigned svgalib_flags(void)
{
	assert(svgalib_is_active());
	return svgalib_state.flags;
}

static unsigned char* svgalib_linear_write_line(unsigned y)
{
	return svgalib_state.ptr + svgalib_state.bytes_per_scanline * y;
}

adv_error svgalib_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	int res;
	int chipset;
	char* term;

	(void)cursor;
	(void)overlay_size;

	/* assume that vga_init() is already called */
	assert(!svgalib_is_active());

	log_std(("video:svgalib: svgalib_init()\n"));

	if (sizeof(svgalib_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (os_internal_wm_active()) {
		error_set("Unsupported in X.\n");
		return -1;
	}

	if (!os_internal_svgalib_get()) {
		error_set("Unsupported without the svgalib library.\n");
		return -1;
	}

	term = getenv("TERM");
	if (!term || strcmp(term, "linux")!=0) {
		error_set("Works only with TERM=linux terminals.\n");
		return -1;
	}

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	/* check the version of the SVGALIB */
	res = vga_setmode(-1);
	if (res < 0 || res < 0x1911) { /* 1.9.11 */
		error_set("You need SVGALIB version 1.9.x or 2.0.x. Please upgrade.\n");
		return -1;
	}

	chipset = vga_getcurrentchipset();
	log_std(("video:svgalib: chipset number %d\n", chipset));

	/* allow only a subset of drivers */
	switch (chipset) {
	case NV3 :
	case TRIDENT :
	case RENDITION :
	case G400 :
	case SAVAGE :
	case MILLENNIUM :
	case R128 :
	case BANSHEE :
	case SIS :
	case I740 :
/*	case I810 : */ /* Version 1.9.17 doesn't work with most recent Intel boards */
	case LAGUNA :
	case RAGE :
	case MX :
	case ET6000 :
	case S3 :
	case ARK :
	case APM :
		break;
	default:
		error_set("Video board unsupported.\n");
		return -1;
	}

	svgalib_state.flags = VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL
		| VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN
		| VIDEO_DRIVER_FLAGS_INTERNAL_DANGEROUSCHANGE;

	switch (chipset) {
	case BANSHEE :
		/* the banshee driver doesn't work in 16 bit mode, only with the 15 bit (at version 1.9.17) */
		log_std(("video:svgalib: disable 16 bit modes not supported by this driver\n"));
		svgalib_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR16;
		break;
	}

	switch (chipset) {
	case NV3 : /* TNT support interlace, but GeForce not */
	case RENDITION :
	case PM2 :
	case I740 :
	case I810 :
	case APM :
		log_std(("video:svgalib: disable interlace modes not supported by this driver\n"));
		svgalib_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
		break;
	};

	svgalib_state.active = 1;
	return 0;
}

void svgalib_done(void)
{
	assert(svgalib_is_active() && !svgalib_mode_is_active());

	log_std(("video:svgalib: svgalib_done()\n"));

	svgalib_state.active = 0;
}

/* Flags copied from svgalib/src/timings.h */
#define SVGALIB_PHSYNC 0x1 /* Positive hsync polarity. */
#define SVGALIB_NHSYNC 0x2 /* Negative hsync polarity. */
#define SVGALIB_PVSYNC 0x4 /* Positive vsync polarity. */
#define SVGALIB_NVSYNC 0x8 /* Negative vsync polarity. */
#define SVGALIB_INTERLACED 0x10 /* Mode has interlaced timing. */
#define SVGALIB_DOUBLESCAN 0x20 /* Mode uses doublescan. */
#define SVGALIB_TVMODE 0x200 /* Enable the TV output. */
#define SVGALIB_TVPAL 0x400 /* Use the PAL format. */
#define SVGALIB_TVNTSC 0x800 /* Use the NTSC format. */
#define SVGALIB_FORCE 0x1000 /* Force the use of this modeline. */

adv_error svgalib_mode_set(const svgalib_video_mode* mode) 
{
	int res;
	int flags;
	vga_modeinfo* modeinfo;
	unsigned colors;
	unsigned bytes_per_pixel;
	unsigned bytes_per_scanline;

	assert(svgalib_is_active() && !svgalib_mode_is_active());

	log_std(("video:svgalib: svgalib_mode_set()\n"));

	flags = SVGALIB_FORCE;
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

	res = vga_addtiming(mode->crtc.pixelclock / 1000, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, flags);
	if (res != 1) {
		error_set("Function vga_addtiming() failed.\n");
		return -1;
	}

	log_std(("video:svgalib: vga_addtiming(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d)\n",
		mode->crtc.pixelclock / 1000, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, flags
	));

	svgalib_state.index = mode->index;
	switch (mode->index) {
		case MODE_FLAGS_INDEX_PALETTE8 :
			bytes_per_pixel = 1;
			colors = 1 << 8;
		break;
		case MODE_FLAGS_INDEX_BGR15 :
			bytes_per_pixel = 2;
			colors = 1 << 15;
		break;
		case MODE_FLAGS_INDEX_BGR16 :
			bytes_per_pixel = 2;
			colors = 1 << 16;
		break;
		case MODE_FLAGS_INDEX_BGR24 :
			bytes_per_pixel = 3;
			colors = 1 << 24;
		break;
		case MODE_FLAGS_INDEX_BGR32 :
			bytes_per_pixel = 4;
			colors = 1 << 24;
		break;
		default:
			error_set("Invalid index mode.\n");
			return -1;
	}
	bytes_per_scanline = bytes_per_pixel * mode->crtc.hde;

	res = vga_addmode(mode->crtc.hde, mode->crtc.vde, colors, bytes_per_scanline, bytes_per_pixel);
	if (res<0) {
		error_set("Function vga_addmode() failed.\n");
		return -1;
	}

	log_std(("video:svgalib: vga_addmode(%d, %d, %d, %d, %d) = %d\n", mode->crtc.hde, mode->crtc.vde, colors, bytes_per_scanline, bytes_per_pixel, res));

	if (!vga_hasmode(res)) {
		if (crtc_is_interlace(&mode->crtc)) {
			error_set("Function vga_hasmode() failed. Have you adjusted\nthe HorizSync and VertRefresh in /etc/vga/libvga.config ?\nTry also disabling the interlaced modes.\n");
		} else {
			error_set("Function vga_hasmode() failed. Have you adjusted\nthe HorizSync and VertRefresh in /etc/vga/libvga.config ?\n");
		}
		log_std(("video:svgalib: Error in vga_hasmode(%d)\n", res));
		log_std(("video:svgalib: Have you adjusted the HorizSync and VertRefresh in /etc/vga/libvga.config ?\n"));
		return -1;
	}

	svgalib_state.mode_number = res;

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		error_set("Function vga_getmodeinfo() failed.\n");
		return -1;
	}

	if ((modeinfo->flags & CAPABLE_LINEAR) == 0) {
		error_set("Linear mode not supported.\n");
		return -1;
	}

	log_std(("video:svgalib: vga_setmode(%d)\n", svgalib_state.mode_number));
	res = vga_setmode(svgalib_state.mode_number);
	if (res != 0) {
		error_set("Function vga_setmode() failed.\n");
		return -1;
	}

	res = vga_setlinearaddressing();
	if (res <= 0) {
		error_set("Function vga_setlinearaddressing() failed.\n");
		return -1;
	}
	svgalib_state.memory_size = res;

	/* claim two screen of memory, not more */
	if (svgalib_state.memory_size > bytes_per_scanline * mode->crtc.vde * 2)
		svgalib_state.memory_size = bytes_per_scanline * mode->crtc.vde * 2;

	log_std(("video:svgalib: using %d bytes\n", svgalib_state.memory_size));
	vga_claimvideomemory(svgalib_state.memory_size);

	svgalib_state.ptr = vga_getgraphmem();

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		error_set("Function vga_getmodeinfo() failed.\n");
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

adv_error svgalib_mode_change(const svgalib_video_mode* mode)
{
	assert(svgalib_is_active() && svgalib_mode_is_active());

	log_std(("video:svgalib: svgalib_mode_change()\n"));

	svgalib_state.mode_active = 0; /* fake done */
	return svgalib_mode_set(mode);
}

void svgalib_mode_done(adv_bool restore)
{
	assert(svgalib_is_active() && svgalib_mode_is_active());

	log_std(("video:svgalib: svgalib_mode_done()\n"));

	svgalib_state.mode_active = 0;

	if (restore) {
		char* term;

		/* a mode reset is required otherwise the other drivers are not able to set the mode */
		log_std(("video:svgalib: vga_setmode(TEXT)\n"));
		vga_setmode(TEXT);

		/* refresh the screen, SVGALIB doesn't save and restore the video memory */
		term = getenv("TERM");
		if (term && strcmp(term, "linux")==0) {
			/* refresh the screen, partial output of "tput flash" */
			fputs("\033[?5h\033[?5l", stdout);
			fflush(stdout);
		}
	}
}

unsigned svgalib_virtual_x(void)
{
	unsigned size = svgalib_state.bytes_per_scanline / svgalib_state.bytes_per_pixel;
	size = size & ~0x7;
	return size;
}

unsigned svgalib_virtual_y(void)
{
	return svgalib_state.memory_size / svgalib_state.bytes_per_scanline;
}

unsigned svgalib_adjust_bytes_per_page(unsigned bytes_per_page)
{
	return bytes_per_page;
}

unsigned svgalib_bytes_per_scanline(void)
{
	return svgalib_state.bytes_per_scanline;
}

adv_color_def svgalib_color_def(void)
{
	return color_def_make_from_index(svgalib_state.index);
}

void svgalib_wait_vsync(void)
{
	assert(svgalib_is_active() && svgalib_mode_is_active());

	vga_waitretrace();
}

adv_error svgalib_scroll(unsigned offset, adv_bool waitvsync)
{
	vga_modeinfo* modeinfo;
	adv_bool sync_flag;

	assert(svgalib_is_active() && svgalib_mode_is_active());

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		error_set("Function vga_getmodeinfo() failed.\n");
		return -1;
	}

#ifdef IOCTL_SETDISPLAY /* IOCTL_SETDISPLAY is present only from SVGALIB 1.9.19 */
	if ((modeinfo->flags & IOCTL_SETDISPLAY) != 0) {
		sync_flag = 1;
	} else {
		sync_flag = 0;
	}
#else
	sync_flag = 0;
#endif

	if (sync_flag) {
		/* if the change is syncronous, the display function must be called before the wait */
		vga_setdisplaystart(offset);

		if (waitvsync)
			vga_waitretrace();
	} else {
		/* if the change is asyncronous, the display function must be called after the wait */
		if (waitvsync)
			vga_waitretrace();

		vga_setdisplaystart(offset);
	}

	return 0;
}

adv_error svgalib_scanline_set(unsigned byte_length)
{
	vga_modeinfo* modeinfo;
	assert(svgalib_is_active() && svgalib_mode_is_active());

	vga_setlogicalwidth(byte_length);

	modeinfo = vga_getmodeinfo(svgalib_state.mode_number);
	if (!modeinfo) {
		error_set("Function vga_getmodeinfo() failed.\n");
		return -1;
	}
	svgalib_state.bytes_per_pixel = modeinfo->bytesperpixel;
	svgalib_state.bytes_per_scanline = modeinfo->linewidth;

	return 0;
}

adv_error svgalib_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
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

adv_error svgalib_mode_import(adv_mode* mode, const svgalib_video_mode* svgalib_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, svgalib_mode->crtc.name);

	*DRIVER(mode) = *svgalib_mode;

	mode->driver = &video_svgalib_driver;
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC | MODE_FLAGS_RETRACE_SET_ASYNC
		| (mode->flags & MODE_FLAGS_USER_MASK)
		| svgalib_mode->index;
	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

adv_error svgalib_mode_generate(svgalib_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	assert(svgalib_is_active());

	if (crtc_is_fake(crtc)) {
		error_nolog_set("Not programmable modes are not supported.\n");
		return -1;
	}

	if (video_mode_generate_check("svgalib", svgalib_flags(), 8, 2048, crtc, flags)!=0)
		return -1;

	mode->crtc = *crtc;
	mode->index = flags & MODE_FLAGS_INDEX_MASK;

	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int svgalib_mode_compare(const svgalib_video_mode* a, const svgalib_video_mode* b)
{
	COMPARE(a->index, b->index);
	return crtc_compare(&a->crtc, &b->crtc);
}

void svgalib_reg(adv_conf* context)
{
	assert(!svgalib_is_active());
}

adv_error svgalib_load(adv_conf* context)
{
	assert(!svgalib_is_active());
	return 0;
}

void svgalib_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:svgalib: svgalib_crtc_container_insert_default()\n"));

	crtc_container_insert_default_modeline_svga(cc);
}

/***************************************************************************/
/* Driver */

static adv_error svgalib_mode_set_void(const void* mode)
{
	return svgalib_mode_set((const svgalib_video_mode*)mode);
}

static adv_error svgalib_mode_change_void(const void* mode)
{
	return svgalib_mode_change((const svgalib_video_mode*)mode);
}

static adv_error svgalib_mode_import_void(adv_mode* mode, const void* svgalib_mode)
{
	return svgalib_mode_import(mode, (const svgalib_video_mode*)svgalib_mode);
}

static adv_error svgalib_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return svgalib_mode_generate((svgalib_video_mode*)mode, crtc, flags);
}

static int svgalib_mode_compare_void(const void* a, const void* b)
{
	return svgalib_mode_compare((const svgalib_video_mode*)a, (const svgalib_video_mode*)b);
}

static unsigned svgalib_mode_size(void)
{
	return sizeof(svgalib_video_mode);
}

adv_video_driver video_svgalib_driver = {
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
	svgalib_color_def,
	0,
	0,
	&svgalib_write_line,
	svgalib_wait_vsync,
	svgalib_scroll,
	svgalib_scanline_set,
	svgalib_palette8_set,
	svgalib_mode_size,
	0,
	svgalib_mode_generate_void,
	svgalib_mode_import_void,
	svgalib_mode_compare_void,
	svgalib_crtc_container_insert_default
};

/***************************************************************************/
/* Internal interface */

int os_internal_svgalib_is_video_active(void)
{
	return svgalib_is_active();
}

int os_internal_svgalib_is_video_mode_active(void)
{
	return svgalib_is_active() && svgalib_mode_is_active();
}

