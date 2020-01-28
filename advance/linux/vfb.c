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

#ifdef USE_VC
#include "interface/vmcs_host/vc_tvservice.h"
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

struct fb_option_struct {
	adv_bool initialized;
	unsigned hdmi_pclock_low;
	unsigned dpi_pclock_low;
	int fast_set;
};

static struct fb_option_struct fb_option;

typedef struct fb_internal_struct {
	adv_bool active;
	adv_bool mode_active;
	int fd; /**< File handle */

	struct fb_var_screeninfo oldinfo; /**< Old variable info. */
	struct fb_fix_screeninfo fixinfo; /**< Fixed info. */
	struct fb_var_screeninfo varinfo; /**< Variable info. */

	adv_bool is_raspberry; /**< If it's a Raspberry Pi. */
#ifdef USE_VC
	TV_DISPLAY_STATE_T oldstate; /**< Raspberry VideoCore state. */
	char oldtimings[128]; /**< Raspberry hdmi_timings. */
	adv_bool old_need_restore; /**< Raspberry needs to restore the old timings. */
#endif

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

static int fb_is_equal(struct fb_var_screeninfo* a, struct fb_var_screeninfo* b)
{
	if (a->xres != b->xres || a->yres != b->yres)
		return 0;
	if (a->xres_virtual != b->xres_virtual || a->yres_virtual != b->yres_virtual)
		return 0;
	if (a->bits_per_pixel != b->bits_per_pixel)
		return 0;
	return 1;
}

static void fb_log(struct fb_fix_screeninfo* fix, struct fb_var_screeninfo* var)
{
	if (fix) {
		log_std(("video:fb: fix info\n"));
		log_std(("video:fb: smem_start:%08xh, smem_len:%08xh\n", (unsigned)fix->smem_start, (unsigned)fix->smem_len));
		log_std(("video:fb: mmio_start:%08xh, mmio_len:%08xh\n", (unsigned)fix->mmio_start, (unsigned)fix->mmio_len));
		log_std(("video:fb: type:%d, type_aux:%d\n", (unsigned)fix->type, (unsigned)fix->type_aux));
		switch (fix->visual) {
		case FB_VISUAL_TRUECOLOR:
			log_std(("video:fb: visual:%d FB_VISUAL_TRUECOLOR\n", (unsigned)fix->visual));
			break;
		case FB_VISUAL_PSEUDOCOLOR:
			log_std(("video:fb: visual:%d FB_VISUAL_PSEUDOCOLOR\n", (unsigned)fix->visual));
			break;
		case FB_VISUAL_DIRECTCOLOR:
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
	var->yres_virtual = vde;
	var->xoffset = 0;
	var->yoffset = 0;
	var->grayscale = 0;
	switch (index) {
	case MODE_FLAGS_INDEX_PALETTE8:
		var->bits_per_pixel = 8;
		var->red.length = 0;
		var->red.offset = 0;
		var->green.length = 0;
		var->green.offset = 0;
		var->blue.length = 0;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR15:
		var->bits_per_pixel = 16; /* this is the real bits per pixel */
		var->red.length = 5;
		var->red.offset = 10;
		var->green.length = 5;
		var->green.offset = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR16:
		var->bits_per_pixel = 16;
		var->red.length = 5;
		var->red.offset = 11;
		var->green.length = 6;
		var->green.offset = 5;
		var->blue.length = 5;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR24:
		var->bits_per_pixel = 24;
		var->red.length = 8;
		var->red.offset = 16;
		var->green.length = 8;
		var->green.offset = 8;
		var->blue.length = 8;
		var->blue.offset = 0;
		break;
	case MODE_FLAGS_INDEX_BGR32:
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
	if (pixelclock)
		var->pixclock = (unsigned)(1000000000000LL / pixelclock);
	else
		var->pixclock = 0;
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
	log_std(("video:fb: ioctl(FBIOPUT_VSCREENINFO) %ux%u %u bits\n", var->xres, var->yres, var->bits_per_pixel));

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
		case MODE_FLAGS_INDEX_BGR15:
			log_std(("video:fb: setting generic %s nibble\n", index_name(hint_index)));
			var->red.length = 5;
			var->red.offset = 10;
			var->green.length = 5;
			var->green.offset = 5;
			var->blue.length = 5;
			var->blue.offset = 0;
			break;
		case MODE_FLAGS_INDEX_BGR16:
			log_std(("video:fb: setting generic %s nibble\n", index_name(hint_index)));
			var->red.length = 5;
			var->red.offset = 11;
			var->green.length = 6;
			var->green.offset = 5;
			var->blue.length = 5;
			var->blue.offset = 0;
			break;
		case MODE_FLAGS_INDEX_BGR24:
		case MODE_FLAGS_INDEX_BGR32:
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

	if (strstr(fb_state.fixinfo.id, "GeForce") != 0) {
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

	if (strstr(fb_state.fixinfo.id, "nVidia") != 0) {
		log_std(("video:fb: disable doublescan modes, not supported by the nVidia driver\n"));
		/* the Linux 2.4.20/2.4.21/2.4.22/2.4.23/2.4.24/2.4.25 driver doesn't support doublescan */
		fb_state.flags &= ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN;
	}

	return 0;
}

#ifdef USE_VC
void vc_log(TV_DISPLAY_STATE_T* state)
{
	log_std(("video:vc: tv info\n"));

	log_std(("video:vc: state %08x:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		state->state,
		state->state & VC_HDMI_UNPLUGGED ? " HDMI_UNPLUGGED" : "",
		state->state & VC_HDMI_ATTACHED ? " HDMI_ATTACHED" : "",
		state->state & VC_HDMI_DVI ? " HDMI_DVI" : "",
		state->state & VC_HDMI_HDMI ? " HDMI_HDMI" : "",
		state->state & VC_HDMI_HDCP_UNAUTH ? " HDMI_HDCP_UNAUTH" : "",
		state->state & VC_HDMI_HDCP_AUTH ? " HDMI_HDCP_AUTH" : "",
		state->state & VC_HDMI_HDCP_KEY_DOWNLOAD ? " HDMI_HDCP_KEY_DOWNLOAD" : "",
		state->state & VC_HDMI_HDCP_SRM_DOWNLOAD ? " HDMI_HDCP_SRM_DOWNLOAD" : "",
		state->state & VC_HDMI_CHANGING_MODE ? " HDMI_CHANGING_MODE" : "",
		state->state & VC_SDTV_UNPLUGGED ? " SDTV_UNPLUGGED" : "",
		state->state & VC_SDTV_ATTACHED ? " SDTV_ATTACHED" : "",
		state->state & VC_SDTV_NTSC ? " SDTV_NTSC" : "",
		state->state & VC_SDTV_PAL ? " SDTV_PAL" : "",
		state->state & VC_SDTV_CP_INACTIVE ? " SDTV_CP_INACTIVE" : "",
		state->state & VC_SDTV_CP_ACTIVE ? " SDTV_CP_ACTIVE" : "",
		state->state & VC_LCD_ATTACHED_DEFAULT ? " LCD_ATTACHED_DEFAULT" : ""
		));

	if (state->state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
		log_std(("video:vc: HDMI state %08x\n", state->display.hdmi.state));
		log_std(("video:vc: %ux%u\n", state->display.hdmi.width, state->display.hdmi.height));
		log_std(("video:vc: framerate %u\n", state->display.hdmi.frame_rate));
		log_std(("video:vc: scanmode %s\n", state->display.hdmi.scan_mode ? "interlaced" : "progressive"));
		switch (state->display.hdmi.group) {
		case HDMI_RES_GROUP_INVALID: log_std(("video:vc: group INVALID\n")); break;
		case HDMI_RES_GROUP_CEA: log_std(("video:vc: group CEA\n")); break;
		case HDMI_RES_GROUP_DMT: log_std(("video:vc: group DMT\n")); break;
		case HDMI_RES_GROUP_CEA_3D: log_std(("video:vc: group CEA_3D\n")); break;
		default: log_std(("video:vc: group UNKNOWN %u\n", state->display.hdmi.group)); break;
		}
		log_std(("video:vc: mode %u\n", state->display.hdmi.mode));
		log_std(("video:vc: pixel_rep %u\n", state->display.hdmi.pixel_rep));
		switch (state->display.hdmi.aspect_ratio) {
		case HDMI_ASPECT_4_3: log_std(("video:vc: aspect_ratio 4:3\n")); break;
		case HDMI_ASPECT_14_9: log_std(("video:vc: aspect_ratio 14:9\n")); break;
		case HDMI_ASPECT_16_9: log_std(("video:vc: aspect_ratio 16:9\n")); break;
		case HDMI_ASPECT_5_4: log_std(("video:vc: aspect_ratio 5:4\n")); break;
		case HDMI_ASPECT_16_10: log_std(("video:vc: aspect_ratio 16:10\n")); break;
		case HDMI_ASPECT_15_9: log_std(("video:vc: aspect_ratio 15:9\n")); break;
		case HDMI_ASPECT_64_27: log_std(("video:vc: aspect_ratio 64:27\n")); break;
		default: log_std(("video:vc: aspect_ratio UNKNOWN %u\n", state->display.hdmi.aspect_ratio)); break;
		}
		log_std(("video:vc: display_option.aspect %u\n", state->display.hdmi.display_options.aspect));
		log_std(("video:vc: display_option.vertical_bar_present %u\n", state->display.hdmi.display_options.vertical_bar_present));
		log_std(("video:vc: display_option.left_bar_width %u\n", state->display.hdmi.display_options.left_bar_width));
		log_std(("video:vc: display_option.right_bar_width %u\n", state->display.hdmi.display_options.right_bar_width));
		log_std(("video:vc: display_option.horizontal_bar_present %u\n", state->display.hdmi.display_options.horizontal_bar_present));
		log_std(("video:vc: display_option.top_bar_height %u\n", state->display.hdmi.display_options.top_bar_height));
		log_std(("video:vc: display_option.bottom_bar_height %u\n", state->display.hdmi.display_options.bottom_bar_height));
		switch (state->display.hdmi.pixel_encoding) {
		case HDMI_PIXEL_ENCODING_RGB_LIMITED: log_std(("video:vc: pixel_encoding RGB lim\n")); break;
		case HDMI_PIXEL_ENCODING_RGB_FULL: log_std(("video:vc: pixel_encoding RGB full\n")); break;
		case HDMI_PIXEL_ENCODING_YCbCr444_LIMITED: log_std(("video:vc: pixel_encoding YCbCr444 lim\n")); break;
		case HDMI_PIXEL_ENCODING_YCbCr444_FULL: log_std(("video:vc: pixel_encoding YCbCr444 full\n")); break;
		case HDMI_PIXEL_ENCODING_YCbCr422_LIMITED: log_std(("video:vc: pixel_encoding YCbCr422 lim\n")); break;
		case HDMI_PIXEL_ENCODING_YCbCr422_FULL: log_std(("video:vc: pixel_encoding YCbCr422 full\n")); break;
		default: log_std(("video:vc: pixel_encoding UNKNOWN %u\n", state->display.hdmi.pixel_encoding)); break;
		}
		switch (state->display.hdmi.format_3d) {
		case 0: log_std(("video:vc: format_3d none\n")); break;
		case HDMI_3D_FORMAT_SBS_HALF: log_std(("video:vc: format_3d SbS\n")); break;
		case HDMI_3D_FORMAT_TB_HALF: log_std(("video:vc: format_3d T&B\n")); break;
		case HDMI_3D_FORMAT_FRAME_PACKING: log_std(("video:vc: format_3d FP\n")); break;
		case HDMI_3D_FORMAT_FRAME_SEQUENTIAL: log_std(("video:vc: format_3d FS\n")); break;
		default: log_std(("video:vc: format_3d UNKNOWN %u\n", state->display.hdmi.format_3d)); break;
		}
	} else if (state->state & (VC_SDTV_NTSC | VC_SDTV_PAL)) {
		log_std(("video:vc: SDTV state %08x\n", state->display.sdtv.state));
		log_std(("video:vc: %ux%u\n", state->display.sdtv.width, state->display.sdtv.height));
		log_std(("video:vc: framerate %u\n", state->display.sdtv.frame_rate));
		log_std(("video:vc: scanmode %s\n", state->display.sdtv.scan_mode ? "interlaced" : "progressive"));
		log_std(("video:vc: mode %08x\n", state->display.sdtv.mode));
		switch (state->display.sdtv.mode & SDTV_MODE_FORMAT_MASK) {
		case SDTV_MODE_NTSC: log_std(("video:vc: mode format NTSC\n")); break;
		case SDTV_MODE_NTSC_J: log_std(("video:vc: mode format NTSC_J\n")); break;
		case SDTV_MODE_PAL: log_std(("video:vc: mode format PAL\n")); break;
		case SDTV_MODE_PAL_M: log_std(("video:vc: mode format PAL_M\n")); break;
		}
		switch (state->display.sdtv.mode & SDTV_MODE_OUTPUT_MASK) {
		case SDTV_COLOUR_RGB: log_std(("video:vc: mode output RGB\n")); break;
		case SDTV_COLOUR_YPRPB: log_std(("video:vc: mode output YPRPB\n")); break;
		}
		switch (state->display.sdtv.display_options.aspect) {
		case SDTV_ASPECT_4_3: log_std(("video:vc: aspect_ratio 4:3\n")); break;
		case SDTV_ASPECT_14_9: log_std(("video:vc: aspect_ratio 14:9\n")); break;
		case SDTV_ASPECT_16_9: log_std(("video:vc: aspect_ratio 16:9\n")); break;
		default: log_std(("video:vc: aspect_ratio UNKNOWN %u\n", state->display.sdtv.display_options.aspect)); break;
		}
		log_std(("video:vc: colour %08x\n", state->display.sdtv.colour));
		switch (state->display.sdtv.colour) {
		case SDTV_COLOUR_RGB: log_std(("video:vc: colour RGB\n")); break;
		case SDTV_COLOUR_YPRPB: log_std(("video:vc: colour YPRPB\n")); break;
		default: log_std(("video:vc: colour UNKNOWN %u\n", state->display.sdtv.colour)); break;
		}
		switch (state->display.sdtv.cp_mode) {
		case SDTV_CP_MACROVISION_TYPE1: log_std(("video:vc: cp_mode Macrovision type 1\n")); break;
		case SDTV_CP_MACROVISION_TYPE2: log_std(("video:vc: cp_mode Macrovision type 2\n")); break;
		case SDTV_CP_MACROVISION_TYPE3: log_std(("video:vc: cp_mode Macrovision type 3\n")); break;
		case SDTV_CP_MACROVISION_TEST1: log_std(("video:vc: cp_mode Macrovision test 1\n")); break;
		case SDTV_CP_MACROVISION_TEST2: log_std(("video:vc: cp_mode Macrovision test 2\n")); break;
		case SDTV_CP_CGMS_COPYFREE: log_std(("video:vc: cp_mode CGMS copy free\n")); break;
		case SDTV_CP_CGMS_COPYNOMORE: log_std(("video:vc: cp_mode CGMS copy no more\n")); break;
		case SDTV_CP_CGMS_COPYONCE: log_std(("video:vc: cp_mode CGMS copy once\n")); break;
		case SDTV_CP_CGMS_COPYNEVER: log_std(("video:vc: cp_mode CGMS copy never\n")); break;
		case SDTV_CP_WSS_COPYFREE: log_std(("video:vc: cp_mode WSS copy free\n")); break;
		case SDTV_CP_WSS_COPYRIGHT_COPYFREE: log_std(("video:vc: cp_mode WSS (c) copy free\n")); break;
		case SDTV_CP_WSS_NOCOPY: log_std(("video:vc: cp_mode WSS no copy\n")); break;
		case SDTV_CP_WSS_COPYRIGHT_NOCOPY: log_std(("video:vc: cp_mode WSS (c) no copy\n")); break;
		default: log_std(("video:vc: cp_mode UNKNOWN %u\n", state->display.sdtv.cp_mode)); break;
		}
	} else if (state->state & VC_LCD_ATTACHED_DEFAULT) {
		/* print from the HDMI struct like tvservice */
		log_std(("video:vc: LCD state %08x\n", state->display.hdmi.state));
		log_std(("video:vc: %ux%u\n", state->display.hdmi.width, state->display.hdmi.height));
		log_std(("video:vc: framerate %u\n", state->display.hdmi.frame_rate));
		log_std(("video:vc: scanmode %s\n", state->display.hdmi.scan_mode ? "interlaced" : "progressive"));
	} else {
		/* print from the HDMI struct like tvservice */
		log_std(("video:vc: TV OFF state %08x\n", state->display.hdmi.state));
	}

	log_std_dump(("video:vc: dump", state, sizeof(*state)));
}
#endif

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

	if (target_wm()) {
		error_set("Unsupported in X. You need to exit the X graphics enviroment, or rebuild with the SDL library.\n");
		return -1;
	}

	if (!fb_option.initialized)
		fb_default();

	log_std(("video:fb: device_hdmi_pclock_low %u\n", fb_option.hdmi_pclock_low));
	log_std(("video:fb: devide_dpi_pclock_low %u\n", fb_option.dpi_pclock_low));

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
			error_set("Video board not supported.\n");
		} else {
			error_set("Error opening the frame buffer %s. %s.\n", fb, strerror(errno));
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

	/* set the size like the original video mode */
	target_size_set(fb_state.varinfo.xres, fb_state.varinfo.yres);

	fb_log(&fb_state.fixinfo, &fb_state.varinfo);

	if (strcmp(id_buffer, "VESA VGA") == 0) {
		error_set("The 'vesafb' FrameBuffer driver doesn't allow the creation of new video modes.\n");
		goto err_close;
	}

	if (strcmp(id_buffer, "DRM emulated") == 0) {
		error_set("The FrameBuffer driver is emulated and doesn't allow the creation of new video modes. Please comment the option dtoverlay=vc4-fkms-v3d on /boot/config to enable a real framebuffer.\n");
		goto err_close;
	}

	/* check if it's a Raspberry Pi 3 or Pi 4 */
	fb_state.is_raspberry = strstr(id_buffer, "BCM2708") != 0;

	if (fb_state.is_raspberry) {
		char* opt;
		char cmd[256];

		log_std(("video:fb: detected Raspberry Pi/BCM2708 hardware\n"));

#ifdef USE_VC
		if (output != adv_output_auto && output != adv_output_overlay && output != adv_output_fullscreen) {
			error_set("Only fullscreen and overlay output are supported.\n");
			return -1;
		}
#else
		if (output != adv_output_auto && output != adv_output_overlay) {
			error_set("Only overlay output is supported.\n");
			return -1;
		}
#endif

		/* exclude BGR15 not supported by the Raspberry hardware */
		fb_state.flags = VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR16 | VIDEO_DRIVER_FLAGS_MODE_BGR24 | VIDEO_DRIVER_FLAGS_MODE_BGR32;

		if (output == adv_output_auto || output == adv_output_overlay)
			fb_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY;
#ifdef USE_VC /* programmable modes are available only with VideoCore */
		if (output == adv_output_auto || output == adv_output_fullscreen)
			fb_state.flags |= VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN;
#endif

		/*
		 * Force the FrameBuffer to fill-out all the screen.
		 * Otherwise if the resolution chosen doesn't have square pixels
		 * the FrameBuffer add black bands to adjust the aspect ratio.
		 *
		 * See:
		 * "Could the framebuffer driver be updated to provide the ability to"
		 * "create custom screen modes whose pixels have an arbitrary aspect ratio?"
		 * https://github.com/raspberrypi/firmware/issues/638
		 *
		 * Note that from Nov 2016, the new option "framebuffer_aspect=-1"
		 * has the same effect of "sdtv_aspect=1" that gets deprecated
		 * and it could be removed in future version.
		 */
		snprintf(cmd, sizeof(cmd), "vcgencmd get_config sdtv_aspect set 1");
		log_std(("video:fb: run \"%s\"\n", cmd));
		opt = target_system(cmd);
		if (opt) {
			log_std(("video:fb: vcgencmd result \"%s\"\n", opt));
			free(opt);
		}

#ifdef USE_VC
		/* keep track if we change timings */
		fb_state.old_need_restore = 0;
		fb_state.oldtimings[0] = 0;

		/* get current info */
		if (vc_tv_get_display_state(&fb_state.oldstate) != 0) {
			error_set("Failed to call VideoCore vc_tv_get_display_state().\n");
			goto err_close;
		}

		vc_log(&fb_state.oldstate);

		/* set aspect */
		if (fb_state.oldstate.state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
			switch (fb_state.oldstate.display.hdmi.aspect_ratio) {
			case HDMI_ASPECT_4_3: target_aspect_set(4, 3); break;
			case HDMI_ASPECT_14_9: target_aspect_set(14, 9); break;
			case HDMI_ASPECT_16_9: target_aspect_set(16, 9); break;
			case HDMI_ASPECT_5_4: target_aspect_set(5, 4); break;
			case HDMI_ASPECT_16_10: target_aspect_set(16, 10); break;
			case HDMI_ASPECT_15_9: target_aspect_set(15, 9); break;
			case HDMI_ASPECT_64_27: target_aspect_set(64, 27); break;
			default: break;
			}
		} else if (fb_state.oldstate.state & (VC_SDTV_NTSC | VC_SDTV_PAL)) {
			switch (fb_state.oldstate.display.sdtv.display_options.aspect) {
			case SDTV_ASPECT_4_3: target_aspect_set(4, 3); break;
			case SDTV_ASPECT_14_9: target_aspect_set(14, 9); break;
			case SDTV_ASPECT_16_9: target_aspect_set(16, 9); break;
			default: break;
			}
		}
#endif
	} else {
		if (output != adv_output_auto && output != adv_output_fullscreen) {
			error_set("Only fullscreen output is supported.\n");
			return -1;
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
	}

	/* set the preferred bit depth */
	switch (fb_state.varinfo.bits_per_pixel) {
	case 16:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) != 0)
			fb_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR16;
		break;
	case 24:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR24) != 0)
			fb_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR24;
		break;
	case 32:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR32) != 0)
			fb_state.flags |= VIDEO_DRIVER_FLAGS_DEFAULT_BGR32;
		break;
	}

	log_std(("video:fb: size %ux%u\n", target_size_x(), target_size_y()));
	log_std(("video:fb: aspect %ux%u\n", target_aspect_x(), target_aspect_y()));

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

		for (i = 0; i < l; ++i) {
			if (i < red_l)
				red_map[i] = 65535 * i / (red_l - 1);
			else
				red_map[i] = 65535;
			if (i < green_l)
				green_map[i] = 65535 * i / (green_l - 1);
			else
				green_map[i] = 65535;
			if (i < blue_l)
				blue_map[i] = 65535 * i / (blue_l - 1);
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

/**
 * Set the requested timings and call tvservice to load them.
 */
#ifdef USE_VC
static int fb_raspberry_settiming(const adv_crtc* crtc, unsigned* size_x, unsigned* size_y)
{
	char* opt;
	char cmd[256];
	int ret;
	unsigned drive;
	adv_crtc copy;

	/*
	 * If we have not yet saved the timinigs, do it now.
	 *
	 * We delay this operation because in VC_LCD_ATTACHED_DEFAULT
	 * mode it makes the screen black, and you need the
	 * "fbset -depth 8 && fbset -depth 16" sequence to restore it.
	 *
	 * At this point we are going anyway to change video mode,
	 * and then we can safely go black.
	 */
	if (!fb_state.old_need_restore) {
		/* get current timings */
		snprintf(cmd, sizeof(cmd), "vcgencmd hdmi_timings");
		log_std(("video:fb: run \"%s\"\n", cmd));
		opt = target_system(cmd);
		if (opt) {
			char* split;
			log_std(("video:fb: vcgencmd result \"%s\"\n", opt));

			split = strchr(opt, '=');
			if (split) {
				++split;
				snprintf(fb_state.oldtimings, sizeof(fb_state.oldtimings), "%s", split);
				log_std(("video:fb: hdmi_timings %s\n", fb_state.oldtimings));
			}

			free(opt);
		}

		/* we are going to change the timings */
		fb_state.old_need_restore = 1;
	}

	*size_x = crtc->hde;
	*size_y = crtc->vde;

	copy = *crtc;

	/*
	 * Simulate Doublescan modes duplicating the vertical size.
	 *
	 * The dispmanx layer will take care of the scaling.
	 */
	if (crtc_is_doublescan(&copy)) {
		crtc_singlescan_set(&copy);
		copy.vde *= 2;
		copy.vrs *= 2;
		copy.vre *= 2;
		copy.vt *= 2;
	}

	/*
	 * In DPI mode there are strong limitation on low pixel clocks,
	 * allowing only the values: 4.8 MHz, 6.4 MHz, 9.6MHz and 19.2 MHz.
	 *
	 * Avoid this limitation changing the mode width,
	 * until we reach a valid pixel clock.
	 *
	 * The dispmanx layer will take care of the scaling.
	 */
	if (fb_state.oldstate.state & VC_LCD_ATTACHED_DEFAULT) {
		if (copy.pixelclock == 4800000
			|| copy.pixelclock == 6400000
			|| copy.pixelclock == 9600000
			|| copy.pixelclock == 19200000
		) {
			/* these exact values are OK */
		} else {
			/*
			 * Increse the x size until we reach a pixelclock of 31.25 MHz
			 * plus some safety margin for error precision.
			 *
			 * Note that the Horizontal and Vertical clocks remain the same.
			 */
			unsigned factor = 0;
			while (copy.pixelclock <= fb_option.dpi_pclock_low + 50000) {
				++factor;
				log_std(("video:fb: adjust DPI modeline to increase by factor %u / 4\n", factor));
				copy.hde += crtc->hde / 4;
				copy.hrs += crtc->hrs / 4;
				copy.hre += crtc->hre / 4;
				copy.ht += crtc->ht / 4;
				copy.pixelclock = crtc->pixelclock + crtc->pixelclock * factor / 4;
			}
		}
	}

	/*
	 * In HDMI/DVI mode there is a "supposed" low limit of 25 MHz for DPI clock.
	 *
	 * See: Please can the ability to modify HDMI timings on the fly
	 * https://github.com/raspberrypi/firmware/issues/637
	 *
	 * "HDMI pixel clock should be between 25Mhz and 162MHz."
	 * "For lower resolutions/framerates pixel doubling and a higher clock should be used."
	 * "For DPI the minimum pixel clock is 31.25MHz."
	 *
	 * For sure the values 4.8 MHz, 6.4 MHz, 9.6MHz and 19.2 MHz
	 * don't work. If they are used, the next pixel clock measure
	 * with:
	 *
	 *   "vcgencmd measure_clock pixel"
	 *
	 * always reports 0.
	 */
	if (fb_state.oldstate.state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
		/*
		 * Increase the x size until we reach the configured pixelclock
		 * plus some safety margin for error precision.
		 *
		 * Note that the Horizontal and Vertical clocks remain the same.
		 */
		unsigned factor = 0;
		while (copy.pixelclock <= fb_option.hdmi_pclock_low + 50000) {
			++factor;
			log_std(("video:fb: adjust HDMI/DVI modeline to increase by factor %u / 4\n", factor));
			copy.hde += crtc->hde / 4;
			copy.hrs += crtc->hrs / 4;
			copy.hre += crtc->hre / 4;
			copy.ht += crtc->ht / 4;
			copy.pixelclock = crtc->pixelclock + crtc->pixelclock * factor / 4;
		}

		/* avoid the values we know to not work */
		if (copy.pixelclock == 4800000
			|| copy.pixelclock == 6400000
			|| copy.pixelclock == 9600000
			|| copy.pixelclock == 19200000
		) {
			/* this small change is enough */
			copy.pixelclock += 1000;
		}
	}

	/*
	 * Configure the Raspberry HDMI timing.
	 *
	 * See:
	 * "Please can the ability to modify HDMI timings on the fly"
	   " (i.e. without having to carry out a reboot) be added to the firmware drivers"
	 * https://github.com/raspberrypi/firmware/issues/637
	 *
	 * Format for hdmi_timings:
	 * # <h_active_pixels> <h_sync_polarity> <h_front_porch> <h_sync_pulse> <h_back_porch>
	 * # <v_active_lines> <v_sync_polarity> <v_front_porch> <v_sync_pulse> <v_back_porch>
	 * # <v_sync_offset_a> <v_sync_offset_b> <pixel_rep> <framerate> <interlaced> <pixel_freq> <aspect>
	 * # aspect ratio: 1=4:3, 2=14:9, 3=16:9, 4=5:4, 5=16:10, 6=15:9, 7=21:9, 8=64:27
	 *
	 * Example: vcgencmd hdmi_timings 640 0 16 64 120 480 0 1 3 16 0 0 0 75 0 31500000 1
	 *
	 */
	snprintf(cmd, sizeof(cmd),
		"vcgencmd hdmi_timings "
		"%u %u %u %u %u "
		"%u %u %u %u %u "
		"%u %u %u %u %u %u %u",
		copy.hde, (int)crtc_is_nhsync(&copy), copy.hrs - copy.hde, copy.hre - copy.hrs, copy.ht - copy.hre,
		copy.vde, (int)crtc_is_nvsync(&copy), copy.vrs - copy.vde, copy.vre - copy.vrs, copy.vt - copy.vre,
		0, 0, 0, (unsigned)floor(crtc_vclock_get(&copy) + 0.5), (int)crtc_is_interlace(&copy), (unsigned)copy.pixelclock, 1
	);

	log_std(("video:fb: run \"%s\"\n", cmd));
	opt = target_system(cmd);
	if (!opt)
		return -1;
	log_std(("video:fb: vcgencmd result \"%s\"\n", opt));
	free(opt);

	/* set the video mode */
	if (fb_state.oldstate.state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
		if (fb_state.oldstate.state & VC_HDMI_HDMI)
			drive = HDMI_MODE_HDMI;
		else
			drive = HDMI_MODE_DVI;
		log_std(("video:vc: vc_tv_hdmi_power_on_explicit(%s, DMT, 87)\n", drive == HDMI_MODE_HDMI ? "HDMI" : "DVI"));
		ret = vc_tv_hdmi_power_on_explicit(drive, HDMI_RES_GROUP_DMT, 87);
		if (ret != 0) {
			log_std(("ERROR:video:vc: vc_tv_hdmi_power_on_explicit() failed\n"));
			return -1;
		}
	} else if (fb_state.oldstate.state & VC_LCD_ATTACHED_DEFAULT) {
		/* in LCD mode the timings are enabled directly by the vcgencmd call */
	} else {
		/* in TV mode (VC_SDTV_NTSC | VC_SDTV_PAL) the timings are never enabled! This is just a fake support! */
		log_std(("ERROR:video:vc: unsupported state %x, don't know how to enable timings\n", fb_state.oldstate.state));
	}

	return 0;
}
#endif

/**
 * Restore the original timing and call tvservice to load them.
 */
#ifdef USE_VC
static void fb_raspberry_restoretiming(unsigned* size_x, unsigned* size_y)
{
	char* opt;
	char cmd[256];
	int ret;
	unsigned drive;

	/* nothing to do if nothing was set */
	if (!fb_state.old_need_restore)
		return;

	/* if we have original timings, restore them */
	if (fb_state.oldtimings[0]) {
		snprintf(cmd, sizeof(cmd), "vcgencmd hdmi_timings %s", fb_state.oldtimings);
		log_std(("video:fb: run \"%s\"\n", cmd));
		opt = target_system(cmd);
		if (opt) {
			log_std(("video:fb: vcgencmd result \"%s\"\n", opt));
			free(opt);
		}
		/* ignore error */
	}

	if (fb_state.oldstate.state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
		*size_x = fb_state.oldstate.display.hdmi.width;
		*size_y = fb_state.oldstate.display.hdmi.height;

		if (fb_state.oldstate.state & VC_HDMI_HDMI)
			drive = HDMI_MODE_HDMI;
		else
			drive = HDMI_MODE_DVI;
		log_std(("video:vc: vc_tv_hdmi_power_on_explicit(%s, %s, %u)\n",
			drive == HDMI_MODE_HDMI ? "HDMI" : "DVI",
			fb_state.oldstate.display.hdmi.group == HDMI_RES_GROUP_CEA ? "CEA" : "DMT",
			fb_state.oldstate.display.hdmi.mode
			));
		ret = vc_tv_hdmi_power_on_explicit(drive, fb_state.oldstate.display.hdmi.group, fb_state.oldstate.display.hdmi.mode);
		if (ret != 0) {
			log_std(("ERROR:video:vc: vc_tv_hdmi_power_on_explicit() failed\n"));
			/* ignore error */
		}
	} else if (fb_state.oldstate.state & (VC_SDTV_NTSC | VC_SDTV_PAL)) {
		*size_x = fb_state.oldstate.display.sdtv.width;
		*size_y = fb_state.oldstate.display.sdtv.height;

		log_std(("video:vc: vc_tv_sdtv_power_on(%u, %u)\n",
			fb_state.oldstate.display.sdtv.mode,
			fb_state.oldstate.display.sdtv.display_options.aspect
			));
		ret = vc_tv_sdtv_power_on(fb_state.oldstate.display.sdtv.mode, &fb_state.oldstate.display.sdtv.display_options);
		if (ret != 0) {
			log_std(("ERROR:video:vc: vc_tv_sdtv_power_on() failed\n"));
			/* ignore error */
		}
	} else if (fb_state.oldstate.state & VC_LCD_ATTACHED_DEFAULT) {
		/* in LCD mode the timings are enabled directly by the vcgencmd call */
		*size_x = fb_state.oldstate.display.hdmi.width;
		*size_y = fb_state.oldstate.display.hdmi.height;
	} else {
		log_std(("ERROR:video:vc: unsupported state %x, don't know how to restore timings\n", fb_state.oldstate.state));
	}
}
#endif

/**
 * Set the timings and video mode.
 *
 * If crtc==0 and var==0 restore the original ones, otherwise the requested ones.
 *
 * This function verifies that dispmanx acknowledges the change checking the "dst:" information.
 * If this doesn't happen the mode set is retried again and again.
 *
 * This multi retry process is required to be able to restore the video mode
 * after a failed attempt to set the clock (when measure_clock pixel/dpi are both 0).
 */
static int fb_raspberry_setvar_and_wait(const adv_crtc* crtc, struct fb_var_screeninfo* var)
{
#ifdef USE_VC
	char* opt;
	char cmd[256];
	unsigned size_x;
	unsigned size_y;
	int count;
	int ret;
	struct fb_var_screeninfo alt;
	adv_bool fail;
	TV_DISPLAY_STATE_T state;
	unsigned event_counter;
	target_clock_t start;
	unsigned wait_ms;
	adv_bool wait_for_event;

	size_x = 0;
	size_y = 0;

	/* get the reference event coutner */
	event_counter = target_vc_get_event();

	/* set or restore the timings */
	if (crtc != 0 && var != 0) {
		if (fb_raspberry_settiming(crtc, &size_x, &size_y) != 0)
			return -1;
		wait_for_event = 1;
	} else {
		var = &fb_state.oldinfo;
		fb_raspberry_restoretiming(&size_x, &size_y);
		wait_for_event = 0;
	}

	count = 1;
	start = target_clock();
loop:
	fail = 0;
	log_std(("video:fb: set retry %u\n", count));

	/*
	 * Wait some time after the tvservice command to allow
	 * the console driver to react.
	 *
	 * Without this wait, the next video mode change has effect,
	 * but the the screen remain black, even if the right video mode
	 * is set.
	 *
	 * Note that this delay is also required when using the
	 * workaround of changing VT with "chvt 2; chvt 1", or resetting
	 * the video mode with "fbset -depth 8; fbset -depth 16".
	 *
	 * A 100ms delay is enough, but we wait more for safety.
	 * As soon the notification event is received, the wait
	 * can be interrupted.
	 *
	 * See:
	 * "Please can the ability to modify HDMI timings on the fly"
	 * https://github.com/raspberrypi/firmware/issues/637
	 */
	if (count == 1 || wait_for_event) {
		log_std(("video:fb: wait for vc event\n"));
		target_vc_wait_event(event_counter + 1, 500);
	} else {
		/*
		 * When restoring the video mode we don't care about the event
		 * but we want a shorter wait to trigger faster the mode change.
		 *
		 * Note that just waiting doesn't work. You relly have to retry
		 * to set the video mode over and over.
		 */
		log_std(("video:fb: wait 100 ms\n"));
		target_usleep(100 * 1000);
	}

	/* set an alternate mode with a different bits per pixel */
	/* there are cases where this is really required */
	alt = *var;
	if (alt.bits_per_pixel == 16) {
		alt.bits_per_pixel = 8;
	} else {
		alt.bits_per_pixel = 16;
	}
	ret = fb_setvar(&alt);
	if (ret != 0) {
		fail = 1;
		log_std(("ERROR:video:vc: alterante mode set FAILED, we are going to retry\n"));
	}

	/* set the effective mode */
	ret = fb_setvar(var);
	if (ret != 0) {
		fail = 1;
		log_std(("ERROR:video:vc: mode set FAILED, we are going to retry\n"));
	}

	/* get tvservice status */
	log_std(("video:vc: vc_tv_get_display_state()\n"));
	if (vc_tv_get_display_state(&state) == 0) {
		vc_log(&state);

		if (state.state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
			if (state.display.hdmi.pixel_encoding == 0) {
				log_std(("ERROR:video:vc: pixel_encoding is INVALID, we are going to retry\n"));
				fail = 1;
			}
		}
	} else {
		log_std(("ERROR:video:vc: vc_tv_get_display_state() FAILED, we are going to retry\n"));
		fail = 1;
	}

	/*
	 * Disabled as in some cases the command hangs with the message:
	 *
	 * $ vcgencmd dispmanx_list
	 * vchi_msg_dequeue -> -1(90)
	 *
	 * And then it requires a manual Ctrl+C to continue
	 *
	 * See: https://sourceforge.net/p/advancemame/discussion/313511/thread/b8bfe1b8/?limit=25#7438
	 *
	 * This was reproduced with: "advmame puckman -display_resize none" with an empty advmame.rc
	 * or with other vertical games like "gunsmoke". But strangely not with "pengo" or "1942" that
	 * have the exact same size.
	 *
	 * The problem disappeared afer removing the panning support in fb_scroll().
	 * Likely the additional fb_setvar() calls were happening at an unfortunate time.
	 *
	 * Still keep disabled the logging to avoid further potential issues.
	 */
#if 0
	/* log dispmanx */
	snprintf(cmd, sizeof(cmd), "vcgencmd dispmanx_list");
	log_std(("video:fb: run \"%s\"\n", cmd));
	opt = target_system(cmd);
	if (opt) {
		log_std(("video:fb: vcgencmd result \"%s\"\n", opt));
		free(opt);
	}
#endif

	if (fail) {
		target_clock_t now = target_clock();

		/*
		 * Retry for 15 seconds.
		 *
		 * When try to restore the original video mode after a refused one
		 * it could take up to 8/9 seconds.
		 */
		if (now < start + 15 * TARGET_CLOCKS_PER_SEC) {
			++count;
			goto loop;
		}

		log_std(("ERROR:video:fb: set TIMEOUT after %u retries\n", count));
		return -1;
	}

	return 0;
#else
	(void)crtc;
	return fb_setvar(var);
#endif
}

adv_error fb_mode_set(const fb_video_mode* mode)
{
	unsigned req_xres;
	unsigned req_yres;
	unsigned req_bits_per_pixel;
	adv_bool is_raspberry_active;

	assert(fb_is_active() && !fb_mode_is_active());

	log_std(("video:fb: fb_mode_set(%ux%u %s, %s)\n", mode->crtc.hde, mode->crtc.vde, index_name(mode->index), crtc_is_fake(&mode->crtc) ? "fake" : "programmable"));

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

	/* if raspberry needs special processing */
	is_raspberry_active = fb_state.is_raspberry && !crtc_is_fake(&mode->crtc);

	/* if it's a programmable mode on Raspberry */
	if (is_raspberry_active) {
		if (fb_raspberry_setvar_and_wait(&mode->crtc, &fb_state.varinfo) != 0) {
			error_set("Error setting the variable video mode information.\n");
			goto err_restore;
		}
	} else {
		/* if the mode is equal, don't set it again */
		if (!fb_option.fast_set || !fb_is_equal(&fb_state.varinfo, &fb_state.oldinfo)) {
			if (fb_setvar(&fb_state.varinfo) != 0) {
				error_set("Error setting the variable video mode information.\n");
				goto err_restore;
			}
		} else {
			log_std(("video:fb: fast set because equal\n"));
		}
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

	if (is_raspberry_active) {
		/* log the effective pixel clock */
		char* opt;
		char cmd[256];
		unsigned pclock = 0;
		unsigned dpiclock = 0;
		unsigned index;

		/* pixel clock for HDMI (known lower limit of 25.00 MHz) */
		snprintf(cmd, sizeof(cmd), "vcgencmd measure_clock pixel");
		log_std(("video:fb: run \"%s\"\n", cmd));
		opt = target_system(cmd);
		if (opt) {
			log_std(("video:fb: vcgencmd result \"%s\"\n", opt));
			if (sscanf(opt, "frequency(%u)=%u", &index, &pclock) == 2) {
				log_std(("video:fb: p_clock %u\n", pclock));
			}
			free(opt);
		}

		/* pixel clock for DPI (known lower limit of 31.25 MHz) */
		snprintf(cmd, sizeof(cmd), "vcgencmd measure_clock dpi");
		log_std(("video:fb: run \"%s\"\n", cmd));
		opt = target_system(cmd);
		if (opt) {
			log_std(("video:fb: vcgencmd result \"%s\"\n", opt));
			if (sscanf(opt, "frequency(%u)=%u", &index, &dpiclock) == 2) {
				log_std(("video:fb: dpi_clock %u\n", dpiclock));
			}
			free(opt);
		}

#ifdef USE_VC
		/* check if the clock is set */
		if (fb_state.oldstate.state & (VC_HDMI_HDMI | VC_HDMI_DVI)) {
			if (pclock == 0) {
				error_set("Failed to set the requested pixel clock.\n");
				goto err_restore;
			}
		} else if (fb_state.oldstate.state & VC_LCD_ATTACHED_DEFAULT) {
			if (dpiclock == 0) {
				error_set("Failed to set the requested dpi clock.\n");
				goto err_restore;
			}
		} else {
			/* in TV mode (VC_SDTV_NTSC | VC_SDTV_PAL) the timings are never enabled! This is just a fake support! */
			log_std(("ERROR:video:vc: unsupported state %x, don't know how to check timings\n", fb_state.oldstate.state));
		}
#endif
	}

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
	log_std(("video:fb: restore after error\n"));
	if (is_raspberry_active) {
		fb_raspberry_setvar_and_wait(0, 0); /* ignore error */
	} else {
		fb_setvar(&fb_state.oldinfo); /* ignore error */
	}
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
		adv_bool is_raspberry_active;

		log_std(("video:fb: restore old\n"));

		fb_log(0, &fb_state.oldinfo);

		/* if raspberry needs special processing */
		is_raspberry_active = fb_state.is_raspberry;

		if (is_raspberry_active) {
			fb_raspberry_setvar_and_wait(0, 0); /* ignore error */
		} else {
			fb_setvar(&fb_state.oldinfo); /* ignore error */
		}
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
	case MODE_FLAGS_INDEX_BGR15:
	case MODE_FLAGS_INDEX_BGR16:
	case MODE_FLAGS_INDEX_BGR24:
	case MODE_FLAGS_INDEX_BGR32:
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

#ifdef __i386__ /* the direct access works only on VGA boards and then only on Intel */
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
#endif

void fb_wait_vsync(void)
{
	switch (fb_state.wait) {
	case fb_wait_ext:
		if (fb_wait_vsync_ext() != 0) {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		} else {
			fb_state.wait_error = 0;
		}
		break;
	case fb_wait_api:
		if (fb_wait_vsync_api() != 0) {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		} else {
			fb_state.wait_error = 0;
		}
		break;
#ifdef __i386__
	case fb_wait_vga:
		if (fb_wait_vsync_vga() != 0) {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		} else {
			fb_state.wait_error = 0;
		}
		break;
#endif
	case fb_wait_detect:
		if (fb_wait_vsync_ext() == 0) {
			fb_state.wait = fb_wait_ext;
			fb_state.wait_error = 0;
		} else if (fb_wait_vsync_api() == 0) {
			fb_state.wait = fb_wait_api;
			fb_state.wait_error = 0;
#ifdef __i386__
		} else if (fb_wait_vsync_vga() == 0) {
			fb_state.wait = fb_wait_vga;
			fb_state.wait_error = 0;
#endif
		} else {
			++fb_state.wait_error;
			if (fb_state.wait_error > WAIT_ERROR_MAX)
				fb_state.wait = fb_wait_none;
		}
		break;
	default:
		break;
	}
}

adv_error fb_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(fb_is_active() && fb_mode_is_active());

	if (offset != 0) {
		error_set("Multiple buffer not supported.\n");
		return -1;
	}

	if (waitvsync)
		fb_wait_vsync();

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

	for (i = 0; i < count; ++i) {
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
	mode->flags = MODE_FLAGS_RETRACE_WAIT_SYNC
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

	switch (flags & MODE_FLAGS_INDEX_MASK) {
	case MODE_FLAGS_INDEX_PALETTE8:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_PALETTE8) == 0) {
			error_nolog_set("Index mode not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR15:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR15) == 0) {
			error_nolog_set("Index mode not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR16:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR16) == 0) {
			error_nolog_set("Index mode not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR24:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR24) == 0) {
			error_nolog_set("Index mode not supported.\n");
			return -1;
		}
		break;
	case MODE_FLAGS_INDEX_BGR32:
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_MODE_BGR32) == 0) {
			error_nolog_set("Index mode not supported.\n");
			return -1;
		}
		break;
	default:
		error_nolog_set("Index mode not supported.\n");
		return -1;
	}

	if (crtc_is_fake(crtc)) {
		/* rescale */
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_OUTPUT_OVERLAY) == 0) {
			error_nolog_set("Not programmable modes are not supported.\n");
			return -1;
		}
	} else {
		/* programmable */
		if ((fb_state.flags & VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN) == 0) {
			error_nolog_set("Programmable modes not supported.\n");
			return -1;
		}
		if (!crtc_is_valid(crtc)) {
			error_nolog_set("Invalid programmable mode.\n");
			return -1;
		}
		if (crtc_is_doublescan(crtc)) {
			if ((fb_state.flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_DOUBLESCAN) == 0) {
				error_nolog_set("Doublescan mode not supported.\n");
				return -1;
			}
		}
		if (crtc_is_interlace(crtc)) {
			if ((fb_state.flags & VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) == 0) {
				error_nolog_set("Interlace mode not supported.\n");
				return -1;
			}
		}
	}

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
	fb_option.hdmi_pclock_low = 0;
	fb_option.dpi_pclock_low = 31250000;

	fb_option.initialized = 1;
}

void fb_reg(adv_conf* context)
{
	assert(!fb_is_active());

	conf_int_register_default(context, "device_hdmi_pclock_low", 0);
	conf_int_register_default(context, "device_dpi_pclock_low", 31250000);
	conf_bool_register_default(context, "device_fb_fastset", 0);
}

adv_error fb_load(adv_conf* context)
{
	assert(!fb_is_active());

	fb_option.hdmi_pclock_low = conf_int_get_default(context, "device_hdmi_pclock_low");
	fb_option.dpi_pclock_low = conf_int_get_default(context, "device_dpi_pclock_low");
	fb_option.fast_set = conf_bool_get_default(context, "device_fb_fastset");

	fb_option.initialized = 1;

	return 0;
}

void fb_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:fb: fb_crtc_container_insert_default()\n"));

	/* no default mode because or we are fullscreen programmable, or overlay with generated */
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

