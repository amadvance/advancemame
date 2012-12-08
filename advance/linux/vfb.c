/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004, 2005 Andrea Mazzoleni
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

#include "vfb.h"
#include "video.h"
#include "log.h"
#include "mode.h"
#include "oslinux.h"
#include "error.h"
#include "snstring.h"
#include "target.h"

#include <linux/fb.h>

/* FrameBuffer extension from DirectFB */
#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, int)
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

/***************************************************************************/
/* State */

enum fb_wait_enum {
	fb_wait_detect,
	fb_wait_none,
	fb_wait_ext,
	fb_wait_api,
	fb_wait_vga
};

typedef struct fb_internal_struct {
	adv_bool active;
	adv_bool mode_active;
	int fd; /**< File handle */

	struct fb_var_screeninfo oldinfo; /**< Old variable info. */
	struct fb_fix_screeninfo fixinfo; /**< Fixed info. */
	struct fb_var_screeninfo varinfo; /**< Variable info. */

	unsigned index;
	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;
	unsigned char* ptr;

	unsigned flags;

	double freq; /**< Expected vertical frequency. */

	enum fb_wait_enum wait; /**< Wait mode. */
	unsigned wait_error; /**< Wait try with error. */
	target_clock_t wait_last; /**< Last vsync. */

} fb_internal;

#define WAIT_ERROR_MAX 2 /**< Max number of errors of consecutive allowed. */

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
	assert(fb_is_active());
	return fb_state.flags;
}

static unsigned char* fb_linear_write_line(unsigned y)
{
	return fb_state.ptr + fb_state.bytes_per_scanline * y;
}

static void fb_log(struct fb_fix_screeninfo* fix, struct fb_var_screeninfo* var)
{
	if (fix) {
		log_std(("video:fb: fix info\n"));
		log_std(("video:fb: smem_start:%08xh, smem_len:%08xh\n", (unsigned)fix->smem_start, (unsigned)fix->smem_len));
		log_std(("video:fb: mmio_start:%08xh, mmio_len:%08xh\n", (unsigned)fix->mmio_start, (unsigned)fix->mmio_len));
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
		unsigned clock;

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
		log_std(("video:fb: nonstd:%d, activate:%xh\n", (unsigned)var->nonstd, (unsigned)var->activate));
		log_std(("video:fb: height:%d, width:%d\n", var->height, var->width));
		log_std(("video:fb: accel_flags:%d\n", var->accel_flags));

		if (var->pixclock)
			clock = (unsigned)(1000000000000LL / var->pixclock);
		else
			clock = 0;

		log_std(("video:fb: pixclock:%d (%d Hz), left:%d, right:%d, upper:%d, lower:%d, hsync:%d, vsync:%d\n",
			(unsigned)var->pixclock,
			clock,
			(unsigned)var->left_margin,
			(unsigned)var->right_margin,
			(unsigned)var->upper_margin,
			(unsigned)var->lower_margin,
			(unsigned)var->hsync_len,
			(unsigned)var->vsync_len
		));
		log_std(("video:fb: sync:%xh", (unsigned)var->sync));
		if ((var->sync & FB_SYNC_HOR_HIGH_ACT) != 0)
			log_std((" phsync"));
		if ((var->sync & FB_SYNC_VERT_HIGH_ACT) != 0)
			log_std((" pvsync"));
		if ((var->sync & FB_SYNC_EXT) != 0)
			log_std((" external_sync"));
		if ((var->sync & FB_SYNC_COMP_HIGH_ACT) != 0)
			log_std((" composite_sync"));
		if ((var->sync & FB_SYNC_BROADCAST) != 0)
			log_std((" broadcast_sync"));
		if ((var->sync & FB_SYNC_ON_GREEN) != 0)
			log_std((" sync_on_green"));
		log_std(("\n"));
		log_std(("video:fb: vmode:%xh", (unsigned)var->vmode));
		if (var->vmode & FB_VMODE_INTERLACED)
			log_std((" interlace"));
		if (var->vmode & FB_VMODE_DOUBLE)
			log_std((" doublescan"));
		log_std(("\n"));
	}
}

static void fb_preset(struct fb_var_screeninfo* var, unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool doublescan, adv_bool interlace, adv_bool nhsync, adv_bool nvsync, unsigned index, unsigned activate)
{
	memset(var, 0, sizeof(struct fb_var_screeninfo));

	var->xres = hde;
	var->yres = vde;
	var->xres_virtual = hde;
	var->yres_virtual = 2 * vde;
	var->xoffset = 0;
	var->yoffset = 0;
	var->grayscale = 0;
	switch (index) {
	case MODE_FLAGS_INDEX_PALETTE8 :
		var->bits_per_pixel = 8;
		var->red.length = 0;
		var->red.offset = 0;
		var->green.length = 0;
		var->green.offset = 0;
		var->blue.length = 0;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR15 :
		var->bits_per_pixel = 16; /* this is the real bits per pixel */
		var->red.length = 5;
		var->red.offset = 10;
		var->green.length = 5;
		var->green.offset = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR16 :
		var->bits_per_pixel = 16;
		var->red.length = 5;
		var->red.offset = 11;
		var->green.length = 6;
		var->green.offset = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR24 :
		var->bits_per_pixel = 24;
		var->red.length = 8;
		var->red.offset = 16;
		var->green.length = 8;
		var->green.offset = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR32 :
		var->bits_per_pixel = 32;
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

static adv_error fb_getfix(struct fb_fix_screeninfo* fix)
{
	log_std(("video:fb: ioctl(FBIOGET_FSCREENINFO)\n"));

	/* get fix info */
	if (ioctl(fb_state.fd, FBIOGET_FSCREENINFO, fix) != 0) {
		log_std(("ERROR:video:fb: ioctl(FBIOGET_FSCREENINFO) failed\n"));
		return -1;
	}

	return 0;
}

static adv_error fb_setpan(struct fb_var_screeninfo* var)
{
	log_debug(("video:fb: ioctl(FBIOPAN_DISPLAY)\n"));

	/* set the pan info */
	if (ioctl(fb_state.fd, FBIOPAN_DISPLAY, var) != 0) {
		log_std(("ERROR:video:fb: ioctl(FBIOPAN_DISPLAY) failed\n"));
		return -1;
	}

	return 0;
}

static adv_error fb_setvar(struct fb_var_screeninfo* var)
{
	log_std(("video:fb: ioctl(FBIOPUT_VSCREENINFO)\n"));

	/* set the variable info */
	if (ioctl(fb_state.fd, FBIOPUT_VSCREENINFO, var) != 0) {
		log_std(("ERROR:video:fb: ioctl(FBIOPUT_VSCREENINFO) failed\n"));
		return -1;
	}

	return 0;
}

static adv_error fb_getvar(struct fb_var_screeninfo* var, unsigned hint_index)
{
	unsigned r_mask;
	unsigned g_mask;
	unsigned b_mask;

	log_std(("video:fb: FBIOGET_VSCREENINFO\n"));

	/* get the variable info */
	if (ioctl(fb_state.fd, FBIOGET_VSCREENINFO, var) != 0) {
		log_std(("ERROR:video:fb: ioctl(FBIOGET_VSCREENINFO) failed\n"));
		return -1;
	}

	r_mask = ((1 << var->red.length) - 1) << var->red.offset;
	g_mask = ((1 << var->green.length) - 1) << var->green.offset;
	b_mask = ((1 << var->blue.length) - 1) << var->blue.offset;

	/* if overlapping */
	if ((r_mask & g_mask) != 0
		|| (r_mask & b_mask) != 0
		|| (b_mask & g_mask) != 0
	) {
		log_std(("ERROR:video:fb: overlapping RGB nibble %x/%x/%x\n", r_mask, g_mask, b_mask));
		switch (hint_index) {
		case MODE_FLAGS_INDEX_BGR15 :
			log_std(("video:fb: setting generic %s nibble\n", index_name(hint_index)));
			var->red.length = 5;
			var->red.offset = 10;
			var->green.length = 5;
			var->green.offset = 5;
			var->blue.length = 5;
			var->blue.offset = 0;
			break;
		case MODE_FLAGS_INDEX_BGR16 :
			log_std(("video:fb: setting generic %s nibble\n", index_name(hint_index)));
			var->red.length = 5;
			var->red.offset = 11;
			var->green.length = 6;
			var->green.offset = 5;
			var->blue.length = 5;
			var->blue.offset = 0;
			break;
		case MODE_FLAGS_INDEX_BGR24 :
		case MODE_FLAGS_INDEX_BGR32 :
			log_std(("video:fb: setting generic %s nibble\n", index_name(hint_index)));
			var->red.length = 8;
			var->red.offset = 16;
			var->green.length = 8;
			var->green.offset = 8;
			var->blue.length = 8;
			var->blue.offset = 0;
			break;
		}
	}

	return 0;
}

static adv_error fb_setcmap(struct fb_cmap* cmap)
{
	log_std(("video:fb: ioctl(FBIOPUTCMAP)\n"));

	if (ioctl(fb_state.fd, FBIOPUTCMAP, cmap) != 0) {
		log_std(("ERROR:video:fb: ioctl(FBIOPUTCMAP) failed\n"));
		return -1;
	}

	return 0;
}

static adv_error fb_test(struct fb_var_screeninfo* var, unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool doublescan, adv_bool interlace, adv_bool nhsync, adv_bool nvsync, unsigned index)
{
	log_std(("video:fb: test mode %dx%d %s\n", hde, vde, index_name(index)));

	fb_preset(var, pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, nhsync, nvsync, index, FB_ACTIVATE_TEST);

	if (fb_setvar(var) != 0)
		return -1;

	return 0;
}

static adv_error fb_test_auto(struct fb_var_screeninfo* var, unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, adv_bool doublescan, adv_bool interlace, adv_bool nhsync, adv_bool nvsync)
{
	unsigned index;

	if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) != 0) {
		index = MODE_FLAGS_INDEX_BGR16;
	} else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR15) != 0) {
		index = MODE_FLAGS_INDEX_BGR15;
	} else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR32) != 0) {
		index = MODE_FLAGS_INDEX_BGR32;
	} else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_PALETTE8) != 0) {
		index = MODE_FLAGS_INDEX_PALETTE8;
	} else if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR24) != 0) {
		index = MODE_FLAGS_INDEX_BGR24;
	} else {
		return -1;
	}

	log_std(("video:fb: test mode %dx%d %s\n", hde, vde, index_name(index)));

	fb_preset(var, pixelclock, hde, hrs, hre, ht, vde, vrs, vre, vt, doublescan, interlace, nhsync, nvsync, index, FB_ACTIVATE_TEST);

	if (fb_setvar(var) != 0)
		return -1;

	return 0;
}

static adv_error fb_detect(void)
{
	struct fb_var_screeninfo var;

	/* test bit depth */
	log_std(("video:fb: test bits depths\n"));
	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, MODE_FLAGS_INDEX_PALETTE8) != 0
		|| (var.bits_per_pixel != 8)) {
		log_std(("video:fb: disable palette8 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, MODE_FLAGS_INDEX_BGR15) != 0
		|| (var.bits_per_pixel != 16 || var.green.length != 5)) {
		log_std(("video:fb: disable bgr15 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR15;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, MODE_FLAGS_INDEX_BGR16) != 0
		|| (var.bits_per_pixel != 16 || var.green.length != 6)) {
		log_std(("video:fb: disable bgr16 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR16;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, MODE_FLAGS_INDEX_BGR24) != 0
		|| (var.bits_per_pixel != 24 || var.green.length != 8)) {
		log_std(("video:fb: disable bgr24 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR24;
	}

	if (fb_test(&var, 25175200, 640, 656, 752, 800, 480, 490, 492, 525, 0, 0, 1, 1, MODE_FLAGS_INDEX_BGR32) != 0
		|| (var.bits_per_pixel != 32 || var.green.length != 8)) {
		log_std(("video:fb: disable bgr32 bits modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR32;
	}

	/* test interlace modes */
	log_std(("video:fb: test interlace modes\n"));
	if (fb_test_auto(&var, 40280300, 1024, 1048, 1200, 1280, 768, 784, 787, 840, 0, 1, 1, 1) != 0
		|| (var.vmode & FB_VMODE_INTERLACED) == 0) {
		log_std(("video:fb: disable interlaced modes, not supported\n"));
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE;
	}

	if (strstr(fb_state.fixinfo.id, "GeForce")!=0) {
		log_std(("video:fb: disable interlaced modes, not supported by the GeForce hardware\n"));
		/* the GeForce hardware doesn't support interlace */
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
		/* the Linux 2.4.20/2.4.21/2.4.22/2.4.23/2.4.24/2.4.25 driver doesn't support doublescan */
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
	}

	return 0;
}

adv_error fb_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	const char* fb;
	char id_buffer[64];
	char* term;

	(void)cursor;
	(void)overlay_size;

	assert(!fb_is_active());

	log_std(("video:fb: fb_init()\n"));

	if (sizeof(fb_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (os_internal_wm_active()) {
		error_set("Unsupported in X. Try with the SDL library.\n");
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

	fb = getenv("FRAMEBUFFER");
	if (fb && fb[0]) {
		fb_state.fd = open(fb, O_RDWR);
	} else {
		fb = "/dev/fb0";
		fb_state.fd = open(fb, O_RDWR);
		if (fb_state.fd < 0 && errno == ENOENT) {
			fb = "/dev/fb/0";
			fb_state.fd = open(fb, O_RDWR);
		}
	}
	if (fb_state.fd < 0) {
		if (errno == ENODEV) {
			error_set("Video board not supported. Error %d (%s).\n", errno, strerror(errno));
		} else {
			error_set("Error opening the frame buffer %s. Error %d (%s).\n", fb, errno, strerror(errno));
		}
		return -1;
	}

	/* get the fixed info */
	if (fb_getfix(&fb_state.fixinfo) != 0) {
		error_set("Error getting the fixed video mode information.\n");
		goto err_close;
	}

	/* get the variable info */
	if (fb_getvar(&fb_state.varinfo, 0) != 0) {
		error_set("Error getting the variable video mode information.\n");
		goto err_close;
	}

	/* copy the id in a safe way, it may be not 0 terminated */
	sncpyn(id_buffer, sizeof(id_buffer), fb_state.fixinfo.id, sizeof(fb_state.fixinfo.id));

	log_std(("video:fb: id %s\n", id_buffer));

	fb_log(&fb_state.fixinfo, &fb_state.varinfo);

	if (strcmp(id_buffer, "VESA VGA")==0) {
		error_set("The 'vesafb' FrameBuffer driver doesn't allow the creation of new video modes.\n");
		goto err_close;
	}

	fb_state.flags = VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32
		| VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL
		| VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;

	if (fb_detect() != 0) {
		goto err_close;
	}

	if ((fb_state.flags & (VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32)) == 0) {
		error_set("This '%s' FrameBuffer driver doesn't seem to allow the creation of new video modes.\n", id_buffer);
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
	assert(fb_is_active() && !fb_mode_is_active());

	log_std(("video:fb: fb_done()\n"));

	close(fb_state.fd);

	fb_state.active = 0;
}

static adv_error fb_setup_color(void)
{
	/* set the color information in direct color modes */
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
			if (i < red_l)
				red_map[i] = 65535 * i / (red_l-1);
			else
				red_map[i] = 65535;
			if (i < green_l)
				green_map[i] = 65535 * i / (green_l-1);
			else
				green_map[i] = 65535;
			if (i < blue_l)
				blue_map[i] = 65535 * i / (blue_l-1);
			else
				blue_map[i] = 65535;
			trasp_map[i] = 0;
		}

		log_std(("video:fb: limit ramp %d:%d:%d %d:%d:%d\n", (unsigned)red_map[0], (unsigned)green_map[0], (unsigned)blue_map[0], (unsigned)red_map[red_l - 1], (unsigned)green_map[green_l - 1], (unsigned)blue_map[blue_l - 1]));

		cmap.start = 0;
		cmap.len = l;
		cmap.red = red_map;
		cmap.green = green_map;
		cmap.blue = blue_map;
		cmap.transp = trasp_map;

		if (fb_setcmap(&cmap) != 0) {
			free(red_map);
			free(green_map);
			free(blue_map);
			free(trasp_map);
			return -1;
		}

		free(red_map);
		free(green_map);
		free(blue_map);
		free(trasp_map);
	}

	return 0;
}

adv_error fb_mode_set(const fb_video_mode* mode)
{
	unsigned req_xres;
	unsigned req_yres;
	unsigned req_bits_per_pixel;

	assert(fb_is_active() && !fb_mode_is_active());

	log_std(("video:fb: fb_mode_set()\n"));

	log_std(("video:fb: get old\n"));

	/* get the current info */
	if (fb_getvar(&fb_state.oldinfo, 0) != 0) {
		error_set("Error getting the variable video mode information.\n");
		goto err;
	}

	fb_log(0, &fb_state.oldinfo);

	fb_preset(&fb_state.varinfo,
		mode->crtc.pixelclock,
		mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht,
		mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt,
		crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc), crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc),
		mode->index, FB_ACTIVATE_NOW
	);

	log_std(("video:fb: set new\n"));

	fb_log(0, &fb_state.varinfo);

	/* save the minimun required data */
	req_xres = fb_state.varinfo.xres;
	req_yres = fb_state.varinfo.yres;
	req_bits_per_pixel = fb_state.varinfo.bits_per_pixel;

	/* set the mode */
	if (fb_setvar(&fb_state.varinfo) != 0) {
		error_set("Error setting the variable video mode information.\n");
		goto err;
	}

	log_std(("video:fb: get new\n"));

	/* get the fixed info */
	if (fb_getfix(&fb_state.fixinfo) != 0) {
		error_set("Error getting the fixed video mode information.\n");
		goto err_restore;
	}

	/* get the variable info */
	if (fb_getvar(&fb_state.varinfo, mode->index) != 0) {
		error_set("Error getting the variable video mode information.\n");
		goto err_restore;
	}

	fb_state.freq = fb_state.varinfo.pixclock;
	fb_state.freq *= fb_state.varinfo.xres + fb_state.varinfo.left_margin + fb_state.varinfo.right_margin + fb_state.varinfo.hsync_len;
	fb_state.freq *= fb_state.varinfo.yres + fb_state.varinfo.upper_margin + fb_state.varinfo.lower_margin + fb_state.varinfo.vsync_len;
	if (fb_state.freq != 0) {
		fb_state.freq = 1000000000000LL / fb_state.freq;
	}

	log_std(("video:fb: frequency %g\n", fb_state.freq));

	fb_log(&fb_state.fixinfo, &fb_state.varinfo);

	/* check the validity of the resulting video mode */
	if (req_xres > fb_state.varinfo.xres
		|| req_yres > fb_state.varinfo.yres
		|| req_bits_per_pixel != fb_state.varinfo.bits_per_pixel
	) {
		log_std(("ERROR:video:fb: request for mode %dx%d %d bits resulted in mode %dx%dx %d bits\n", req_xres, req_yres, req_bits_per_pixel, fb_state.varinfo.xres, fb_state.varinfo.yres, fb_state.varinfo.bits_per_pixel));
		error_set("Error setting the requested video mode.\n");
		goto err_restore;
	}
	if (req_xres != fb_state.varinfo.xres
		|| req_yres != fb_state.varinfo.yres
	) {
		/* allow bigger modes */
		log_std(("WARNING:video:fb: request for mode %dx%d resulted in mode %dx%dx\n", req_xres, req_yres, fb_state.varinfo.xres, fb_state.varinfo.yres));
	}

	if (fb_setup_color() != 0) {
		error_set("Error setting the color information.\n");
		goto err_restore;
	}

	fb_write_line = fb_linear_write_line;

	fb_state.bytes_per_pixel = (fb_state.varinfo.bits_per_pixel + 7) / 8;
	fb_state.bytes_per_scanline = fb_state.fixinfo.line_length;
	fb_state.index = mode->index;

	fb_state.ptr = mmap(0,
		fb_state.fixinfo.smem_len,
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		fb_state.fd,
		0
	);

	if (fb_state.ptr == MAP_FAILED) {
		error_set("Error mapping the video memory.\n");
		goto err_restore;
	}

	fb_state.wait_last = 0;
	fb_state.wait = fb_wait_detect; /* reset the wait mode */
	fb_state.wait_error = 0;

	fb_state.mode_active = 1;

	return 0;

err_restore:
	fb_setvar(&fb_state.oldinfo); /* ignore error */
err:
	return -1;
}

void fb_mode_done(adv_bool restore)
{
	assert(fb_is_active() && fb_mode_is_active());

	log_std(("video:fb: fb_mode_done()\n"));

	if (munmap(fb_state.ptr, fb_state.fixinfo.smem_len) != 0) {
		log_std(("ERROR:video:fb: munmap failed\n"));
		/* ignore error */
	}

	if (restore) {
		log_std(("video:fb: restore old\n"));

		fb_log(0, &fb_state.oldinfo);

		fb_setvar(&fb_state.oldinfo);
		/* ignore error */
	} else {
		/* ensure to have the correct color, the keyboard driver */
		/* when resetting the console changes some colors */
		fb_setup_color();
		/* ignore error */
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

	switch (fb_state.index) {
	case MODE_FLAGS_INDEX_BGR15 :
	case MODE_FLAGS_INDEX_BGR16 :
	case MODE_FLAGS_INDEX_BGR24 :
	case MODE_FLAGS_INDEX_BGR32 :
		return color_def_make_rgb_from_sizelenpos(
			fb_state.bytes_per_pixel,
			fb_state.varinfo.red.length, fb_state.varinfo.red.offset,
			fb_state.varinfo.green.length, fb_state.varinfo.green.offset,
			fb_state.varinfo.blue.length, fb_state.varinfo.blue.offset
		);
	default:
		return color_def_make_from_index(fb_state.index);
	}
}

/**
 * Wait a vsync using the FB extension.
 */
static adv_error fb_wait_vsync_ext(void)
{
	assert(fb_is_active() && fb_mode_is_active());

	log_debug(("video:fb: ioctl(FBIO_WAITFORVSYNC)\n"));

	if (ioctl(fb_state.fd, FBIO_WAITFORVSYNC, 0) != 0) {
		log_std(("WARNING:video:fb: ioctl(FBIO_WAITFORVSYNC) failed\n"));
		/* it may be not supported, it isn't an error */
		return -1;
	}

	return 0;
}

/**
 * Wait a vsync using the FB API.
 */
static adv_error fb_wait_vsync_api(void)
{
	struct fb_vblank blank;

	assert(fb_is_active() && fb_mode_is_active());

	log_debug(("video:fb: ioctl(FBIOGET_VBLANK)\n"));

	if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
		log_std(("WARNING:video:fb: ioctl(FBIOGET_VBLANK) failed\n"));
		/* it may be not supported, it isn't an error */
		return -1;
	}

	if ((blank.flags & FB_VBLANK_HAVE_COUNT) != 0) {
		/* favorite choice because you cannot lose sync, it should be irq driven */
		unsigned start = blank.count;
		log_debug(("video:fb: using FB_VBLANK_HAVE_COUNT\n"));
		while (start == blank.count) {
			if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
				return -1;
			}
		}
	} else if ((blank.flags & FB_VBLANK_HAVE_VSYNC) != 0) {
		log_debug(("video:fb: using FB_VBLANK_HAVE_VSYNC\n"));
		while ((blank.flags & FB_VBLANK_VSYNCING) == 0) {
			if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
				return -1;
			}
		}
	} else if ((blank.flags & FB_VBLANK_HAVE_VBLANK) != 0) {
		log_debug(("video:fb: using FB_VBLANK_HAVE_VBLANK\n"));
		while ((blank.flags & FB_VBLANK_VBLANKING) == 0) {
			if (ioctl(fb_state.fd, FBIOGET_VBLANK, &blank) != 0) {
				return -1;
			}
		}
	} else {
		log_std(("WARNING:video:fb: VBLANK unusable\n"));
		return -1;
	}

	return 0;
}

/**
 * Upper loop limit for the vsync wait.
 * Any port read take approximatively 0.5 - 1.5us.
 */
#define VSYNC_LIMIT 100000

/**
 * Wait a vsync using the VGA registers.
 */
static adv_error fb_wait_vsync_vga(void)
{
	unsigned counter;

	assert(fb_is_active() && fb_mode_is_active());

	counter = 0;

	while ((target_port_get(0x3da) & 0x8) != 0) {
		if (counter > VSYNC_LIMIT) {
			log_std(("ERROR:fb: wait timeout\n"));
			return -1;
		}
		++counter;
	}

	while ((target_port_get(0x3da) & 0x8) == 0) {
		if (counter > VSYNC_LIMIT) {
			log_std(("ERROR:fb: wait timeout\n"));
			return -1;
		}
		++counter;
	}

	fb_state.wait_last = target_clock();

	return 0;
}

void fb_wait_vsync(void)
{
	switch (fb_state.wait) {
	case fb_wait_ext :
		if (fb_wait_vsync_ext() != 0) {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		} else {
			fb_state.wait_error = 0;
		}
		break;
	case fb_wait_api :
		if (fb_wait_vsync_api() != 0) {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		} else {
			fb_state.wait_error = 0;
		}
		break;
	case fb_wait_vga :
		if (fb_wait_vsync_vga() != 0) {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		} else {
			fb_state.wait_error = 0;
		}
		break;
	case fb_wait_detect:
		if (fb_wait_vsync_ext() == 0) {
			fb_state.wait = fb_wait_ext;
			fb_state.wait_error = 0;
		} else if (fb_wait_vsync_api() == 0) {
			fb_state.wait = fb_wait_api;
			fb_state.wait_error = 0;
		} else if (fb_wait_vsync_vga() == 0) {
			fb_state.wait = fb_wait_vga;
			fb_state.wait_error = 0;
		} else {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		}
		break;
	case fb_wait_none:
		break;
	}
}

adv_error fb_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(fb_is_active() && fb_mode_is_active());

	if (waitvsync)
		fb_wait_vsync();

	fb_state.varinfo.yoffset = offset / fb_state.bytes_per_scanline;
	fb_state.varinfo.xoffset = (offset % fb_state.bytes_per_scanline) / fb_state.bytes_per_pixel;

        if (fb_setpan(&fb_state.varinfo) != 0) {
		error_set("Error panning the video.\n");
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

	if (fb_setcmap(&cmap) != 0) {
		error_set("Error setting the color information.\n");
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
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC | MODE_FLAGS_RETRACE_SET_ASYNC
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
	assert(fb_is_active());

	if (crtc_is_fake(crtc)) {
		error_nolog_set("Not programmable modes are not supported.\n");
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

void fb_reg(adv_conf* context)
{
	assert(!fb_is_active());
}

adv_error fb_load(adv_conf* context)
{
	assert(!fb_is_active());
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
	fb_mode_size,
	0,
	fb_mode_generate_void,
	fb_mode_import_void,
	fb_mode_compare_void,
	fb_crtc_container_insert_default
};

/***************************************************************************/
/* Internal interface */

int os_internal_fb_is_video_active(void)
{
	return fb_is_active();
}

int os_internal_fb_is_video_mode_active(void)
{
	return fb_is_active() && fb_mode_is_active();
}

