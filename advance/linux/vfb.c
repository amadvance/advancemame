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

#include "vfb.h"
#include "video.h"
#include "os.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <linux/fb.h>
#include <vga.h> /* for waitretrace */

/***************************************************************************/
/* State */

typedef struct fb_internal_struct {
	video_bool active;
	video_bool mode_active;
	int fd; /** File handle */

	struct fb_var_screeninfo oldinfo; /** Old variable info */
	struct fb_fix_screeninfo fixinfo; /** Fixed info */
	struct fb_var_screeninfo varinfo; /** Variable info */

	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;
	unsigned char* ptr;
} fb_internal;

static fb_internal fb_state;

unsigned char* (*fb_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

static unsigned char* fb_linear_write_line(unsigned y) {
	return fb_state.ptr + fb_state.bytes_per_scanline * y;
}

static void fb_log(void) {
	double v;

	video_log("video:fb: id %s\n",fb_state.fixinfo.id);
	video_log("video:fb: smem_start:%08x, smem_len:%08x\n",(unsigned)fb_state.fixinfo.smem_start,(unsigned)fb_state.fixinfo.smem_len);
	video_log("video:fb: mmio_start:%08x, mmio_len:%08x\n",(unsigned)fb_state.fixinfo.mmio_start,(unsigned)fb_state.fixinfo.mmio_len);
	video_log("video:fb: type:%d, type_aux:%d, visual:%d\n",(unsigned)fb_state.fixinfo.type,(unsigned)fb_state.fixinfo.type_aux,(unsigned)fb_state.fixinfo.visual);
	video_log("video:fb: xpanstep:%d, ypanstep:%d, ywrapstep:%d\n",(unsigned)fb_state.fixinfo.xpanstep,(unsigned)fb_state.fixinfo.ypanstep,(unsigned)fb_state.fixinfo.ywrapstep);
	video_log("video:fb: line_length:%d\n",(unsigned)fb_state.fixinfo.line_length);
	video_log("video:fb: accel:%d\n",(unsigned)fb_state.fixinfo.accel);
	video_log("video:fb: xres:%d, yres:%d\n", (unsigned)fb_state.varinfo.xres, (unsigned)fb_state.varinfo.yres);
	video_log("video:fb: xres_virtual:%d, yres_virtual:%d\n", (unsigned)fb_state.varinfo.xres_virtual, (unsigned)fb_state.varinfo.yres_virtual);
	video_log("video:fb: xoffset:%d, yoffset:%d\n", (unsigned)fb_state.varinfo.xoffset, (unsigned)fb_state.varinfo.yoffset);
	video_log("video:fb: bits_per_pixel:%d, grayscale:%d\n", (unsigned)fb_state.varinfo.bits_per_pixel, (unsigned)fb_state.varinfo.grayscale);
	video_log("video:fb: nonstd:%d, activate:%x\n", (unsigned)fb_state.varinfo.nonstd, (unsigned)fb_state.varinfo.activate);
	video_log("video:fb: height:%d, width:%d\n",fb_state.varinfo.height,fb_state.varinfo.width);
	video_log("video:fb: accel_flags:%d\n",fb_state.varinfo.accel_flags);
	video_log("video:fb: pixclock:%d, left:%d, right:%d, upper:%d, lower:%d, hsync:%d, vsync:%d\n",
		(unsigned)fb_state.varinfo.pixclock,
		(unsigned)fb_state.varinfo.left_margin,
		(unsigned)fb_state.varinfo.right_margin,
		(unsigned)fb_state.varinfo.upper_margin,
		(unsigned)fb_state.varinfo.lower_margin,
		(unsigned)fb_state.varinfo.hsync_len,
		(unsigned)fb_state.varinfo.vsync_len
	);
	video_log("video:fb: sync:%x, vmode:%x\n",(unsigned)fb_state.varinfo.sync,(unsigned)fb_state.varinfo.vmode);

	v = 1000000000000LL / (double)fb_state.varinfo.pixclock;
	v /= fb_state.varinfo.xres + fb_state.varinfo.left_margin + fb_state.varinfo.right_margin + fb_state.varinfo.hsync_len;
	v /= fb_state.varinfo.yres + fb_state.varinfo.upper_margin + fb_state.varinfo.lower_margin + fb_state.varinfo.vsync_len;

	video_log("video:fb: expected vclock:%g\n", v);
}

/***************************************************************************/
/* Public */

static device DEVICE[] = {
	{ "auto", -1, "Console Frame Buffer automatic detection" },
};

video_error fb_init(int device_id) {
	const char* fb;

	assert( !fb_is_active() );

	log_std(("video:fb: fb_init()\n"));

	fb = getenv("FRAMEBUFFER");
	if (!fb)
                fb = "/dev/fb0";

	if (access(fb,W_OK)!=0) {
		video_log("video:fb: no access at the frame buffer %s",fb);
		return -1;
	}

	fb_state.fd = open(fb, O_RDWR);
	if (fb_state.fd < 0) {
		video_log("video:fb: error opening %s",fb);
		return -1;
	}

	fb_state.active = 1;

	return 0;
}

void fb_done(void) {
	assert(fb_is_active() && !fb_mode_is_active() );

	log_std(("video:fb: fb_done()\n"));

	close(fb_state.fd);

	fb_state.active = 0;
}

video_bool fb_is_active(void) {
	return fb_state.active != 0;
}

video_bool fb_mode_is_active(void) {
	return fb_state.mode_active != 0;
}

unsigned fb_flags(void) {
	assert( fb_is_active() );
	return VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL;
}

video_error fb_mode_set(const fb_video_mode* mode)
{
	assert( fb_is_active() && !fb_mode_is_active() );

	log_std(("video:fb: fb_mode_set()\n"));

	/* get the current info */
	if (ioctl(fb_state.fd, FBIOGET_VSCREENINFO, &fb_state.oldinfo) != 0) {
		video_error_description_set("Error in FBIOGET_VSCREENINFO");
		return -1;
	}

	memset(&fb_state.varinfo,0,sizeof(fb_state.varinfo));

	fb_state.varinfo.xres = mode->crtc.hde;
	fb_state.varinfo.yres = mode->crtc.vde;
	fb_state.varinfo.xres_virtual = mode->crtc.hde;
	fb_state.varinfo.yres_virtual = 2 * mode->crtc.vde;
	fb_state.varinfo.xoffset = 0;
	fb_state.varinfo.yoffset = 0;
	fb_state.varinfo.bits_per_pixel = mode->bits_per_pixel;
	fb_state.varinfo.grayscale = 0;
	switch (mode->bits_per_pixel) {
		case 8 :
		break;
		case 15 :
			fb_state.varinfo.red.length = 5;
			fb_state.varinfo.red.offset = 10;
			fb_state.varinfo.green.length = 5;
			fb_state.varinfo.green.offset = 5;
			fb_state.varinfo.blue.length = 5;
			fb_state.varinfo.blue.offset = 0;
		break;
		case 16 :
			fb_state.varinfo.red.length = 5;
			fb_state.varinfo.red.offset = 11;
			fb_state.varinfo.green.length = 6;
			fb_state.varinfo.green.offset = 5;
			fb_state.varinfo.blue.length = 5;
			fb_state.varinfo.blue.offset = 0;
		break;
		case 24 :
                case 32 :
			fb_state.varinfo.red.length = 8;
			fb_state.varinfo.red.offset = 16;
			fb_state.varinfo.green.length = 8;
			fb_state.varinfo.green.offset = 8;
			fb_state.varinfo.blue.length = 8;
			fb_state.varinfo.blue.offset = 0;
		break;
	}
	fb_state.varinfo.nonstd = 0;
	fb_state.varinfo.activate = FB_ACTIVATE_NOW;
	fb_state.varinfo.height = 0;
	fb_state.varinfo.width = 0;
	fb_state.varinfo.accel_flags = FB_ACCEL_NONE;
	fb_state.varinfo.pixclock = (unsigned)(1000000000000LL / mode->crtc.pixelclock);
	fb_state.varinfo.left_margin = mode->crtc.ht - mode->crtc.hre;
	fb_state.varinfo.right_margin = mode->crtc.hrs - mode->crtc.hde;
	fb_state.varinfo.upper_margin = mode->crtc.vt - mode->crtc.vre;;
	fb_state.varinfo.lower_margin = mode->crtc.vrs - mode->crtc.vde;
	fb_state.varinfo.hsync_len = mode->crtc.hre - mode->crtc.hrs;
	fb_state.varinfo.vsync_len = mode->crtc.vre - mode->crtc.vrs;;

	fb_state.varinfo.sync = 0;
	if (crtc_is_nhsync(&mode->crtc))
		fb_state.varinfo.sync |= FB_SYNC_HOR_HIGH_ACT;
	if (crtc_is_nvsync(&mode->crtc))
		fb_state.varinfo.sync |= FB_SYNC_VERT_HIGH_ACT;
	if (crtc_is_tvpal(&mode->crtc) || crtc_is_tvntsc(&mode->crtc))
		fb_state.varinfo.sync |= FB_SYNC_BROADCAST;

	fb_state.varinfo.vmode = 0;
	if (crtc_is_doublescan(&mode->crtc)) {
		fb_state.varinfo.vmode |= FB_VMODE_DOUBLE;
		fb_state.varinfo.upper_margin /= 2;
		fb_state.varinfo.lower_margin /= 2;
		fb_state.varinfo.vsync_len /= 2;
	}
	if (crtc_is_interlace(&mode->crtc)) {
		fb_state.varinfo.vmode |= FB_VMODE_INTERLACED;
	}

	fb_log();

	video_log("video:fb: ioctl(FBIOPUT_VSCREENINFO)\n");

	/* set the mode */
	if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, &fb_state.varinfo) != 0) {
		video_error_description_set("Error in FBIOPUT_VSCREENINFO");
		return -1;
	}

	/* get the fixed info */
	if (ioctl(fb_state.fd, FBIOGET_FSCREENINFO, &fb_state.fixinfo) != 0) {
		video_error_description_set("Error in FBIOGET_FSCREENINFO");
		return -1;
	}

	/* get the variable info */
	if (ioctl(fb_state.fd, FBIOGET_VSCREENINFO, &fb_state.varinfo) != 0) {
		video_error_description_set("Error in FBIOGET_VSCREENINFO");
		return -1;
	}

	fb_log();

	fb_write_line = fb_linear_write_line;

	fb_state.bytes_per_pixel = (fb_state.varinfo.bits_per_pixel + 7) / 8;
	fb_state.bytes_per_scanline = fb_state.fixinfo.line_length;

	fb_state.ptr = mmap(0,
		fb_state.fixinfo.smem_len,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_FIXED,
		fb_state.fd,
		0
	);

	if (fb_state.ptr == MAP_FAILED) {
		return -1;
	}

	fb_state.mode_active = 1;

	return 0;
}

void fb_mode_done(video_bool restore) {
	assert( fb_is_active() && fb_mode_is_active() );

	log_std(("video:fb: fb_mode_done()\n"));

	munmap(fb_state.ptr, fb_state.fixinfo.smem_len);

	if (restore) {
		if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, &fb_state.oldinfo) != 0) {
			video_error_description_set("Error in FBIOPUT_VSCREENINFO");
		}
	}

	fb_state.mode_active = 0;
}

unsigned fb_virtual_x(void) {
	assert(fb_is_active() && fb_mode_is_active());

	return fb_state.varinfo.xres_virtual;
}

unsigned fb_virtual_y(void) {
	assert(fb_is_active() && fb_mode_is_active());

	return fb_state.varinfo.yres_virtual;
}

unsigned fb_bytes_per_scanline(void) {
	return fb_state.bytes_per_scanline;
}

unsigned fb_adjust_bytes_per_page(unsigned bytes_per_page) {
	return bytes_per_page;
}

video_rgb_def fb_rgb_def(void) {
	assert(fb_is_active() && fb_mode_is_active());

	return video_rgb_def_make(fb_state.varinfo.red.length,fb_state.varinfo.red.offset,fb_state.varinfo.green.length,fb_state.varinfo.green.offset,fb_state.varinfo.blue.length,fb_state.varinfo.blue.offset);
}

void fb_wait_vsync(void) {
	struct fb_vblank blank;

	assert(fb_is_active() && fb_mode_is_active());

	do {
		if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
			/* not supported */
			vga_waitretrace();
			return;
		}

		if ((blank.flags & FB_VBLANK_HAVE_VSYNC) == 0) {
			/* not supported */
			vga_waitretrace();
			return;
		}
	} while ((blank.flags & FB_VBLANK_VSYNCING) == 0);
}

video_error fb_scroll(unsigned offset, video_bool waitvsync) {
	assert(fb_is_active() && fb_mode_is_active());

	if (waitvsync)
		fb_wait_vsync();

	fb_state.varinfo.yoffset = offset / fb_state.bytes_per_scanline;
	fb_state.varinfo.xoffset = (offset % fb_state.bytes_per_scanline) / fb_state.bytes_per_pixel;

        if (ioctl(fb_state.fd, FBIOPAN_DISPLAY, &fb_state.varinfo) != 0) {
		video_error_description_set("Error in FBIOPAN_DISPLAY");
		return -1;
	}

	return 0;
}

video_error fb_scanline_set(unsigned byte_length) {
	assert(fb_is_active() && fb_mode_is_active());

	return -1;
}

video_error fb_palette8_set(const video_color* palette, unsigned start, unsigned count, video_bool waitvsync) {
	__u16 r[256], g[256], b[256], t[256];
	struct fb_cmap cmap;
	unsigned i;

	if (waitvsync)
		fb_wait_vsync();

	for (i=0;i<count;++i) {
		r[i] = (palette[i].red << 8) | palette[i].red;
		g[i] = (palette[i].green << 8) | palette[i].green;
		b[i] = (palette[i].blue << 8) | palette[i].blue;
		t[i] = 0;
	}

	cmap.start = start;
	cmap.len = count;
	cmap.red = r;
	cmap.green = g;
	cmap.blue = b;
	cmap.transp = t;

	if (ioctl(fb_state.fd, FBIOPUTCMAP, &cmap) != 0) {
		video_error_description_set("Error in FBIOPUTCMAP");
		return -1;
	}

	return 0;
}

#define DRIVER(mode) ((fb_video_mode*)(&mode->driver_mode))

video_error fb_mode_import(video_mode* mode, const fb_video_mode* fb_mode)
{
	strcpy(mode->name, fb_mode->crtc.name);

	*DRIVER(mode) = *fb_mode;

	mode->driver = &video_fb_driver;
	mode->flags = VIDEO_FLAGS_ASYNC_SETPAGE
		| VIDEO_FLAGS_MEMORY_LINEAR
		| (mode->flags & VIDEO_FLAGS_USER_MASK);
	switch (fb_mode->bits_per_pixel) {
		case 8 : mode->flags |= VIDEO_FLAGS_INDEX_PACKED | VIDEO_FLAGS_TYPE_GRAPHICS; break;
		default: mode->flags |= VIDEO_FLAGS_INDEX_RGB | VIDEO_FLAGS_TYPE_GRAPHICS; break;
	}

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->bits_per_pixel = fb_mode->bits_per_pixel;
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

video_error fb_mode_generate(fb_video_mode* mode, const video_crtc* crtc, unsigned bits, unsigned flags)
{
	assert( fb_is_active() );

	if (video_mode_generate_check("fb",fb_flags(),8,2048,crtc,bits,flags)!=0)
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

int fb_mode_compare(const fb_video_mode* a, const fb_video_mode* b) {
	COMPARE(a->bits_per_pixel,b->bits_per_pixel);
	return video_crtc_compare(&a->crtc,&b->crtc);
}

void fb_default(void) {
}

void fb_reg(struct conf_context* context) {
	assert( !fb_is_active() );
}

video_error fb_load(struct conf_context* context) {
	assert( !fb_is_active() );
	return 0;
}

/***************************************************************************/
/* Driver */

static video_error fb_mode_set_void(const void* mode) {
	return fb_mode_set((const fb_video_mode*)mode);
}

static video_error fb_mode_import_void(video_mode* mode, const void* fb_mode) {
	return fb_mode_import(mode, (const fb_video_mode*)fb_mode);
}

static video_error fb_mode_generate_void(void* mode, const video_crtc* crtc, unsigned bits, unsigned flags) {
	return fb_mode_generate((fb_video_mode*)mode,crtc,bits,flags);
}

static int fb_mode_compare_void(const void* a, const void* b) {
	return fb_mode_compare((const fb_video_mode*)a, (const fb_video_mode*)b);
}

static unsigned fb_mode_size(void) {
	return sizeof(fb_video_mode);
}

video_driver video_fb_driver = {
	"fb",
	DEVICE,
	fb_load,
	fb_reg,
	fb_init,
	fb_done,
	fb_flags,
	fb_mode_set_void,
	0,
	fb_mode_done,
	fb_virtual_x,
	fb_virtual_y,
	0,
	0,
	fb_bytes_per_scanline,
	fb_adjust_bytes_per_page,
	fb_rgb_def,
	0,
	0,
	&fb_write_line,
	fb_wait_vsync,
	fb_scroll,
	fb_scanline_set,
	fb_palette8_set,
	0,
	fb_mode_size,
	0,
	fb_mode_generate_void,
	fb_mode_import_void,
	fb_mode_compare_void,
	0
};

