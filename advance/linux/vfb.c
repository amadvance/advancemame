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
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "portable.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <linux/fb.h>

/***************************************************************************/
/* State */

typedef struct fb_internal_struct {
	adv_bool active;
	adv_bool mode_active;
	int fd; /** File handle */

	struct fb_var_screeninfo oldinfo; /** Old variable info */
	struct fb_fix_screeninfo fixinfo; /** Fixed info */
	struct fb_var_screeninfo varinfo; /** Variable info */

	unsigned index;
	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;
	unsigned char* ptr;

	unsigned flags;
} fb_internal;

static fb_internal fb_state;

unsigned char* (*fb_write_line)(unsigned y);

static adv_device DEVICE[] = {
{ "auto", -1, "Frame Buffer video" },
{ 0, 0, 0 }
};

/***************************************************************************/
/* Functions */

static adv_bool fb_is_active(void)
{
	return fb_state.active != 0;
}

static adv_bool fb_mode_is_active(void)
{
	return fb_state.mode_active != 0;
}

static unsigned fb_flags(void)
{
	assert( fb_is_active() );
	return fb_state.flags;
}

static unsigned char* fb_linear_write_line(unsigned y)
{
	return fb_state.ptr + fb_state.bytes_per_scanline * y;
}

static void fb_log(struct fb_fix_screeninfo* fix, struct fb_var_screeninfo* var)
{
	double v;

	if (fix) {
		log_std(("video:fb: fix info\n"));
		log_std(("video:fb: id %s\n", fix->id));
		log_std(("video:fb: smem_start:%08x, smem_len:%08x\n", (unsigned)fix->smem_start, (unsigned)fix->smem_len));
		log_std(("video:fb: mmio_start:%08x, mmio_len:%08x\n", (unsigned)fix->mmio_start, (unsigned)fix->mmio_len));
		log_std(("video:fb: type:%d, type_aux:%d\n", (unsigned)fix->type, (unsigned)fix->type_aux));
		switch (fix->visual) {
			case FB_VISUAL_TRUECOLOR :
				log_std(("video:fb: visual:%d FB_VISUAL_TRUECOLOR\n", (unsigned)fix->visual));
				break;
			case FB_VISUAL_PSEUDOCOLOR :
				log_std(("video:fb: visual:%d FB_VISUAL_PSEUDOCOLOR\n", (unsigned)fix->visual));
				break;
			case FB_VISUAL_DIRECTCOLOR :
				log_std(("video:fb: visual:%d FB_VISUAL_DIRECTCOLOR\n", (unsigned)fix->visual));
				break;
			default:
				log_std(("video:fb: visual:%d\n", (unsigned)fix->visual));
				break;
		}
		log_std(("video:fb: xpanstep:%d, ypanstep:%d, ywrapstep:%d\n", (unsigned)fix->xpanstep, (unsigned)fix->ypanstep, (unsigned)fix->ywrapstep));
		log_std(("video:fb: line_length:%d\n", (unsigned)fix->line_length));
		log_std(("video:fb: accel:%d\n", (unsigned)fix->accel));
	}

	if (var) {
		log_std(("video:fb: variable info\n"));
		log_std(("video:fb: xres:%d, yres:%d\n", (unsigned)var->xres, (unsigned)var->yres));
		log_std(("video:fb: xres_virtual:%d, yres_virtual:%d\n", (unsigned)var->xres_virtual, (unsigned)var->yres_virtual));
		log_std(("video:fb: xoffset:%d, yoffset:%d\n", (unsigned)var->xoffset, (unsigned)var->yoffset));
		log_std(("video:fb: bits_per_pixel:%d, grayscale:%d\n", (unsigned)var->bits_per_pixel, (unsigned)var->grayscale));
		log_std(("video:fb: red %d:%d green %d:%d blue %d:%d transp %d:%d\n",
			(unsigned)var->red.length, (unsigned)var->red.offset,
			(unsigned)var->green.length, (unsigned)var->green.offset,
			(unsigned)var->blue.length, (unsigned)var->blue.offset,
			(unsigned)var->transp.length, (unsigned)var->transp.offset
		));
		log_std(("video:fb: nonstd:%d, activate:%x\n", (unsigned)var->nonstd, (unsigned)var->activate));
		log_std(("video:fb: height:%d, width:%d\n", var->height, var->width));
		log_std(("video:fb: accel_flags:%d\n", var->accel_flags));
		log_std(("video:fb: pixclock:%d, left:%d, right:%d, upper:%d, lower:%d, hsync:%d, vsync:%d\n",
			(unsigned)var->pixclock,
			(unsigned)var->left_margin,
			(unsigned)var->right_margin,
			(unsigned)var->upper_margin,
			(unsigned)var->lower_margin,
			(unsigned)var->hsync_len,
			(unsigned)var->vsync_len
		));
		log_std(("video:fb: sync:%x", (unsigned)var->sync));
		if ((var->sync & FB_SYNC_HOR_HIGH_ACT) == 0)
			log_std((" nhsync"));
		if ((var->sync & FB_SYNC_VERT_HIGH_ACT) == 0)
			log_std((" nvsync"));
		if ((var->sync & FB_SYNC_EXT) != 0)
			log_std((" external_sync"));
		if ((var->sync & FB_SYNC_COMP_HIGH_ACT) != 0)
			log_std((" composite_sync"));
		if ((var->sync & FB_SYNC_BROADCAST) != 0)
			log_std((" broadcast_sync"));
		if ((var->sync & FB_SYNC_ON_GREEN) != 0)
			log_std((" sync_on_green"));
		log_std(("\n"));
		log_std(("video:fb: vmode:%x", (unsigned)var->vmode));
		if (var->vmode & FB_VMODE_INTERLACED)
			log_std((" interlace"));
		if (var->vmode & FB_VMODE_DOUBLE)
			log_std((" doublescan"));
		log_std(("\n"));
		log_std(("video:fb: reserved %x:%x:%x:%x:%x:%x\n",
			(unsigned)var->reserved[0],
			(unsigned)var->reserved[1],
			(unsigned)var->reserved[2],
			(unsigned)var->reserved[3],
			(unsigned)var->reserved[4],
			(unsigned)var->reserved[5]
		));
		v = 1000000000000LL / (double)var->pixclock;
		v /= var->xres + var->left_margin + var->right_margin + var->hsync_len;
		v /= var->yres + var->upper_margin + var->lower_margin + var->vsync_len;
		log_std(("video:fb: expected vclock:%g\n", v));
	}
}

static void fb_preset(struct fb_var_screeninfo* var, unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool doublescan, adv_bool interlace, adv_bool nhsync, adv_bool nvsync, unsigned bits_per_pixel, unsigned activate)
{
	memset(var, 0, sizeof(struct fb_var_screeninfo));

	var->xres = hde;
	var->yres = vde;
	var->xres_virtual = hde;
	var->yres_virtual = 2 * vde;
	var->xoffset = 0;
	var->yoffset = 0;
	var->bits_per_pixel = bits_per_pixel;
	var->grayscale = 0;
	switch (bits_per_pixel) {
	case 8 :
		break;
	case 15 :
		var->red.length = 5;
		var->red.offset = 10;
		var->green.length = 5;
		var->green.offset = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		break;
	case 16 :
		var->red.length = 5;
		var->red.offset = 11;
		var->green.length = 6;
		var->green.offset = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		break;
	case 24 :
	case 32 :
		var->red.length = 8;
		var->red.offset = 16;
		var->green.length = 8;
		var->green.offset = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		break;
	}
	var->nonstd = 0;
	var->activate = activate;
	var->height = 0;
	var->width = 0;
	var->accel_flags = FB_ACCEL_NONE;
	var->pixclock = (unsigned)(1000000000000LL / pixelclock);
	var->left_margin = ht - hre;
	var->right_margin = hrs - hde;
	var->upper_margin = vt - vre;
	var->lower_margin = vrs - vde;
	var->hsync_len = hre - hrs;
	var->vsync_len = vre - vrs;

	var->sync = 0;
	if (!nhsync)
		var->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (!nvsync)
		var->sync |= FB_SYNC_VERT_HIGH_ACT;

	var->vmode = 0;
	if (doublescan) {
		var->vmode |= FB_VMODE_DOUBLE;
		var->upper_margin /= 2;
		var->lower_margin /= 2;
		var->vsync_len /= 2;
	}
	if (interlace) {
		var->vmode |= FB_VMODE_INTERLACED;
	}
}

static adv_error fb_test(struct fb_var_screeninfo* var, unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool doublescan, adv_bool interlace, adv_bool nhsync, adv_bool nvsync, unsigned bits_per_pixel)
{
	log_std(("video:fb: test bit depth %d\n", bits_per_pixel));

	fb_preset(var, pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, nhsync, nvsync, bits_per_pixel, FB_ACTIVATE_TEST);
	if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, var) != 0)
		return -1;

	return 0;
}

static adv_error fb_test_auto(struct fb_var_screeninfo* var, unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool doublescan, adv_bool interlace, adv_bool nhsync, adv_bool nvsync)
{
	unsigned bits_per_pixel;

	if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) != 0)
		bits_per_pixel = 16;
	else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR15) != 0)
		bits_per_pixel = 15;
	else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR32) != 0)
		bits_per_pixel = 32;
	else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_PALETTE8) != 0)
		bits_per_pixel = 8;
	else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR24) != 0)
		bits_per_pixel = 24;
	else
		return -1;

	log_std(("video:fb: test mode %d bits\n", bits_per_pixel));

	fb_preset(var, pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, nhsync, nvsync, bits_per_pixel, FB_ACTIVATE_TEST);

	if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, var) != 0)
		return -1;

	return 0;
}

static adv_error fb_detect(void)
{
	struct fb_var_screeninfo var;

	/* test bit depth */
	log_std(("video:fb: test bits depths\n"));
	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, 8) != 0
		|| (var.bits_per_pixel != 8)) {
		log_std(("video:fb: disable 8 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, 15) != 0
		|| (var.bits_per_pixel != 15)) {
		log_std(("video:fb: disable 15 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR15;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, 16) != 0
		|| (var.bits_per_pixel != 16)) {
		log_std(("video:fb: disable 16 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR16;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, 24) != 0
		|| (var.bits_per_pixel != 24)) {
		log_std(("video:fb: disable 24 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR24;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, 32) != 0
		|| (var.bits_per_pixel != 32)) {
		log_std(("video:fb: disable 32 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR32;
	}

	/* test interlace modes */
	log_std(("video:fb: test iterlace modes\n"));
	if (fb_test_auto(&var, 40280300, 1024, 1048, 1200, 1280, 768, 784, 787, 840, 0, 1, 1, 1) != 0
		|| (var.vmode & FB_VMODE_INTERLACED) == 0) {
		log_std(("video:fb: disable interlace modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
	}

	if (strstr(fb_state.fixinfo.id, "GeForce")!=0) {
		log_std(("video:fb: disable interlace modes, not supported by the GeForge hardware\n"));
		/* the GeForge hardware doesn't support interlace */
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
	}

	/* test doublescan modes */
	log_std(("video:fb: test doublescan modes\n"));
	if (fb_test_auto(&var, 12676000, 320, 328, 376, 400, 240, 245, 246, 262, 1, 0, 1, 1) != 0
		|| (var.vmode & FB_VMODE_DOUBLE) == 0) {
		log_std(("video:fb: disable doublescan modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
	}

	if (strstr(fb_state.fixinfo.id, "nVidia")!=0) {
		log_std(("video:fb: disable doublescan modes, not supported by the nVidia driver\n"));
		/* the Linux 2.4.20/2.4.21 driver doesn't support doublescan */
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
	}

	return 0;
}

adv_error fb_init(int device_id, adv_output output, unsigned zoom_size, adv_cursor cursor)
{
	const char* fb;
	(void)cursor;
	(void)zoom_size;

	assert( !fb_is_active() );

	log_std(("video:fb: fb_init()\n"));

	if (sizeof(fb_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (getenv("DISPLAY")) {
		log_std(("video:fb: DISPLAY set\n"));
		error_nolog_cat("fb: Unsupported in X\n");
		return -1;
	}

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		log_std(("video:fb: Only fullscreen output is supported\n"));
		error_nolog_cat("fb: Only fullscreen output is supported\n");
		return -1;
	}

	fb = getenv("FRAMEBUFFER");
	if (!fb)
		fb = "/dev/fb0";

	if (access(fb, R_OK | W_OK)!=0) {
		log_std(("video:fb: R/W access denied at the frame buffer %s\n", fb));
		error_nolog_cat("fb: R/W access denied at the frame buffer %s\n", fb);
		return -1;
	}

	fb_state.fd = open(fb, O_RDWR);
	if (fb_state.fd < 0) {
		log_std(("video:fb: Error opening the frame buffer %s\n", fb));
		error_nolog_cat("fb: Error opening the frame buffer %s\n", fb);
		return -1;
	}

	/* get the fixed info */
	if (ioctl(fb_state.fd, FBIOGET_FSCREENINFO, &fb_state.fixinfo) != 0) {
		error_set("Error in FBIOGET_FSCREENINFO");
		goto err_close;
	}

	/* get the variable info */
	if (ioctl(fb_state.fd, FBIOGET_VSCREENINFO, &fb_state.varinfo) != 0) {
		error_set("Error in FBIOGET_VSCREENINFO");
		goto err_close;
	}

	fb_log(&fb_state.fixinfo, &fb_state.varinfo);

	fb_state.flags = VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL
		| VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;

	if (fb_detect() != 0) {
		goto err_close;
	}

	fb_state.active = 1;

	return 0;

err_close:
	close(fb_state.fd);
	return -1;
}

void fb_done(void)
{
	assert(fb_is_active() && !fb_mode_is_active() );

	log_std(("video:fb: fb_done()\n"));

	close(fb_state.fd);

	fb_state.active = 0;
}

adv_error fb_mode_set(const fb_video_mode* mode)
{
	assert( fb_is_active() && !fb_mode_is_active() );

	log_std(("video:fb: fb_mode_set()\n"));

	/* get the current info */
	if (ioctl(fb_state.fd, FBIOGET_VSCREENINFO, &fb_state.oldinfo) != 0) {
		error_set("Error in FBIOGET_VSCREENINFO");
		return -1;
	}

	fb_preset(&fb_state.varinfo,
		mode->crtc.pixelclock,
		mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht,
		mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt,
		crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc), crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc),
		index_bits_per_pixel(mode->index), FB_ACTIVATE_NOW
	);

	log_std(("video:fb: FBIOPUT_VSCREENINFO\n"));

	/* set the mode */
	if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, &fb_state.varinfo) != 0) {
		error_set("Error in FBIOPUT_VSCREENINFO");
		return -1;
	}

	log_std(("video:fb: FBIOGET_FSCREENINFO\n"));

	/* get the fixed info */
	if (ioctl(fb_state.fd, FBIOGET_FSCREENINFO, &fb_state.fixinfo) != 0) {
		error_set("Error in FBIOGET_FSCREENINFO");
		return -1;
	}

	log_std(("video:fb: FBIOGET_VSCREENINFO\n"));

	/* get the variable info */
	if (ioctl(fb_state.fd, FBIOGET_VSCREENINFO, &fb_state.varinfo) != 0) {
		error_set("Error in FBIOGET_VSCREENINFO");
		return -1;
	}

	fb_log(&fb_state.fixinfo, &fb_state.varinfo);

	if (fb_state.fixinfo.visual == FB_VISUAL_DIRECTCOLOR) {
		unsigned red_l = 1 << fb_state.varinfo.red.length;
		unsigned green_l = 1 << fb_state.varinfo.green.length;
		unsigned blue_l = 1 << fb_state.varinfo.blue.length;
		__u16* red_map;
		__u16* green_map;
		__u16* blue_map;
		__u16* trasp_map;
		unsigned l;
		struct fb_cmap cmap;
		unsigned i;

		l = red_l;
		if (l < green_l)
			l = green_l;
		if (l < blue_l)
			l = blue_l;

		log_std(("video:fb: set ramp %d:%d:%d %d\n", red_l, green_l, blue_l, l));

		red_map = malloc(sizeof(__u16) * l);
		green_map = malloc(sizeof(__u16) * l);
		blue_map = malloc(sizeof(__u16) * l);
		trasp_map = malloc(sizeof(__u16) * l);

		for (i=0;i<l;++i) {
			red_map[i] = 65535 * i / (red_l-1);
			green_map[i] = 65535 * i / (green_l-1);
			blue_map[i] = 65535 * i / (blue_l-1);
			trasp_map[i] = 0;
		}

		log_std(("video:fb: limit ramp %d:%d:%d %d:%d:%d\n", (unsigned)red_map[0], (unsigned)green_map[0], (unsigned)blue_map[0], (unsigned)red_map[red_l - 1], (unsigned)green_map[green_l - 1], (unsigned)blue_map[blue_l - 1]));

		cmap.start = 0;
		cmap.len = l;
		cmap.red = red_map;
		cmap.green = green_map;
		cmap.blue = blue_map;
		cmap.transp = trasp_map;

		log_std(("video:fb: FBIOPUTCMAP\n"));

		if (ioctl(fb_state.fd, FBIOPUTCMAP, &cmap) != 0) {
			error_set("Error in FBIOPUTCMAP");
			return -1;
		}

		free(red_map);
		free(green_map);
		free(blue_map);
		free(trasp_map);
	}

	fb_write_line = fb_linear_write_line;

	fb_state.bytes_per_pixel = (fb_state.varinfo.bits_per_pixel + 7) / 8;
	fb_state.bytes_per_scanline = fb_state.fixinfo.line_length;
	fb_state.index = mode->index;

	fb_state.ptr = mmap(0,
		fb_state.fixinfo.smem_len,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_FIXED,
		fb_state.fd,
		0
	);

	if (fb_state.ptr == MAP_FAILED) {
		error_set("Error in mmap");
		return -1;
	}

	/* disable cursor "tput civis" */
	fputs("\033[?1c", stdout);
	fflush(stdout);

	fb_state.mode_active = 1;

	return 0;
}

void fb_mode_done(adv_bool restore)
{
	assert( fb_is_active() && fb_mode_is_active() );

	log_std(("video:fb: fb_mode_done()\n"));

	/* restore cursor "tput cnorm" */
	fputs("\033[?0c", stdout);
	fflush(stdout);

#if 0
	/* clear screen "tput clear" */
	fputs("\033[H\033[J", stdout);
	fflush(stdout);
#endif

	munmap(fb_state.ptr, fb_state.fixinfo.smem_len);

	if (restore) {
		if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, &fb_state.oldinfo) != 0) {
			error_set("Error in FBIOPUT_VSCREENINFO");
		}
	}

	fb_state.mode_active = 0;
}

unsigned fb_virtual_x(void)
{
	assert(fb_is_active() && fb_mode_is_active());

	return fb_state.varinfo.xres_virtual;
}

unsigned fb_virtual_y(void)
{
	assert(fb_is_active() && fb_mode_is_active());

	return fb_state.varinfo.yres_virtual;
}

unsigned fb_bytes_per_scanline(void)
{
	return fb_state.bytes_per_scanline;
}

unsigned fb_adjust_bytes_per_page(unsigned bytes_per_page)
{
	return bytes_per_page;
}

adv_color_def fb_color_def(void)
{
	assert(fb_is_active() && fb_mode_is_active());

	return color_def_make_from_rgb_sizelenpos(
		fb_state.bytes_per_pixel,
		fb_state.varinfo.red.length, fb_state.varinfo.red.offset,
		fb_state.varinfo.green.length, fb_state.varinfo.green.offset,
		fb_state.varinfo.blue.length, fb_state.varinfo.blue.offset
	);
}

void fb_wait_vsync(void)
{
	struct fb_vblank blank;

	assert(fb_is_active() && fb_mode_is_active());

	if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
		log_std(("ERROR:video:fb: FBIOGET_VBLANK not supported\n"));
		/* not supported */
		return;
	}

	if ((blank.flags & FB_VBLANK_HAVE_COUNT) != 0) {
		/* favorite choice because you cannot lose sync, generally is irq driven  */
		unsigned start = blank.count;
		log_debug(("video:fb: using FB_VBLANK_HAVE_COUNT\n"));
		while (start == blank.count) {
			if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
				return;
			}
		}
	} else if ((blank.flags & FB_VBLANK_HAVE_VSYNC) != 0) {
		log_debug(("video:fb: using FB_VBLANK_HAVE_VSYNC\n"));
		while ((blank.flags & FB_VBLANK_VSYNCING) == 0) {
			if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
				return;
			}
		}
	} else if ((blank.flags & FB_VBLANK_HAVE_VBLANK) != 0) {
		log_debug(("video:fb: using FB_VBLANK_HAVE_VBLANK\n"));
		while ((blank.flags & FB_VBLANK_VBLANKING) == 0) {
			if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
				return;
			}
		}
	} else {
		log_std(("video:fb: VBLANK unusable\n"));
	}
}

adv_error fb_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(fb_is_active() && fb_mode_is_active());

	if (waitvsync)
		fb_wait_vsync();

	fb_state.varinfo.yoffset = offset / fb_state.bytes_per_scanline;
	fb_state.varinfo.xoffset = (offset % fb_state.bytes_per_scanline) / fb_state.bytes_per_pixel;

        if (ioctl(fb_state.fd, FBIOPAN_DISPLAY, &fb_state.varinfo) != 0) {
		error_set("Error in FBIOPAN_DISPLAY");
		return -1;
	}

	return 0;
}

adv_error fb_scanline_set(unsigned byte_length)
{
	assert(fb_is_active() && fb_mode_is_active());

	return -1;
}

adv_error fb_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
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
		error_set("Error in FBIOPUTCMAP");
		return -1;
	}

	return 0;
}

#define DRIVER(mode) ((fb_video_mode*)(&mode->driver_mode))

adv_error fb_mode_import(adv_mode* mode, const fb_video_mode* fb_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, fb_mode->crtc.name);

	*DRIVER(mode) = *fb_mode;

	mode->driver = &video_fb_driver;
	mode->flags = MODE_FLAGS_SCROLL_ASYNC
		| MODE_FLAGS_MEMORY_LINEAR
		| (mode->flags & MODE_FLAGS_USER_MASK)
		| fb_mode->index;
	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

adv_error fb_mode_generate(fb_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	assert( fb_is_active() );

	if (crtc_is_fake(crtc)) {
		error_nolog_cat("fb: Not programmable modes not supported\n");
		return -1;
	}

	if (video_mode_generate_check("fb", fb_flags(), 8, 2048, crtc, flags)!=0)
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

int fb_mode_compare(const fb_video_mode* a, const fb_video_mode* b)
{
	COMPARE(a->index, b->index);
	return crtc_compare(&a->crtc, &b->crtc);
}

void fb_default(void)
{
}

void fb_reg(adv_conf* context)
{
	assert( !fb_is_active() );
}

adv_error fb_load(adv_conf* context)
{
	assert( !fb_is_active() );
	return 0;
}

void fb_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:fb: fb_crtc_container_insert_default()\n"));

	crtc_container_insert_default_modeline_svga(cc);
}

/***************************************************************************/
/* Driver */

static adv_error fb_mode_set_void(const void* mode)
{
	return fb_mode_set((const fb_video_mode*)mode);
}

static adv_error fb_mode_import_void(adv_mode* mode, const void* fb_mode)
{
	return fb_mode_import(mode, (const fb_video_mode*)fb_mode);
}

static adv_error fb_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return fb_mode_generate((fb_video_mode*)mode, crtc, flags);
}

static int fb_mode_compare_void(const void* a, const void* b)
{
	return fb_mode_compare((const fb_video_mode*)a, (const fb_video_mode*)b);
}

static unsigned fb_mode_size(void)
{
	return sizeof(fb_video_mode);
}

adv_video_driver video_fb_driver = {
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
	fb_color_def,
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
	fb_crtc_container_insert_default
};

