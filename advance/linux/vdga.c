/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2001, 2002, 2003 Andrea Mazzoleni
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

/* -- INCOMPLETE -- */

/*
Two problems:
1) The XFree86 4.2.0 is buggy. Anyway, the following patch fix the AddModeline and ValidateModeline
functions. You need only to compile /usr/X11/lib/modules/extensions/libextmod.a.
2) The DGA scan the list of available modes only at the startup. In the DGAInit
function called by the video drivers. Any added modeline is not seen by DGAQueryModes.

The only solution is to don't use DGA at all. Simply use the normal
Xlib functions. The problem is that you lose the Vsync capability.
*/

/* Link flags: -L/usr/X11/lib -lX11 -lXext -lXxf86dga -lXxf86vm */

/* Patch for programs/Xserver/Xext/xf86vmode.c */
/*
--- xf86vmode.c.ori	2001-08-06 22:51:03.000000000 +0200
+++ xf86vmode.c	2002-08-18 16:07:53.000000000 +0200
@@ -752, 6 +751, 10 @@
     VidModeSetModeValue(mode, VIDMODE_V_SYNCEND, stuff->vsyncend);
     VidModeSetModeValue(mode, VIDMODE_V_TOTAL, stuff->vtotal);
     VidModeSetModeValue(mode, VIDMODE_FLAGS, stuff->flags);
+    VidModeSetModeValue(mode, VIDMODE_CLOCK, stuff->dotclock);
+    ((DisplayModePtr)mode)->VScan = 1;
+    ((DisplayModePtr)mode)->status = MODE_OK;
+    ((DisplayModePtr)mode)->type = M_T_CLOCK_CRTC_C;
 
     if (stuff->privsize)
 	ErrorF("AddModeLine - Privates in request have been ignored\n");
@@ -1108, 6 +1112, 8 @@
     VidModeSetModeValue(modetmp, VIDMODE_V_SYNCEND, stuff->vsyncend);
     VidModeSetModeValue(modetmp, VIDMODE_V_TOTAL, stuff->vtotal);
     VidModeSetModeValue(modetmp, VIDMODE_FLAGS, stuff->flags);
+    VidModeSetModeValue(modetmp, VIDMODE_CLOCK, stuff->dotclock);
+    ((DisplayModePtr)modetmp)->VScan = 1;
     if (stuff->privsize)
 	ErrorF("ValidateModeLine - Privates in request have been ignored\n");
 
*/

#include "vdga.h"
#include "video.h"
#include "log.h"
#include "osint.h"

#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/xf86dga.h>

#define DGA_Display ((Display*)os_internal_dga_get())
#define DGA_Screen DefaultScreen(DGA_Display)

/***************************************************************************/
/* State */

typedef struct dga_internal_struct {
	adv_bool active;
	adv_bool mode_active;

	XDGADevice* device; /**< DGA device of the video surface. */

	/* TODO */
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
} dga_internal;

static dga_internal dga_state;

unsigned char* (*dga_write_line)(unsigned y);

/***************************************************************************/
/* Internal */

/* TODO
static unsigned char* dga_linear_write_line(unsigned y)
{
	return dga_state.ptr + dga_state.bytes_per_scanline * y;
}
*/

/***************************************************************************/
/* Public */

static device DEVICE[] = {
{ "auto", -1, "DGA video" },
{ 0, 0, 0 }
};

adv_error dga_init(int device_id)
{
	/* assume that vga_init() is already called */
	assert(!dga_is_active());

	log_std(("video:dga: dga_init()\n"));

	if (!DGA_Display) {
		log_std(("video:dga: DGA not initialized\n"));
		return -1;
	}

	/* open access to the framebuffer */
	log_std(("video:dga: XDGAOpenFramebuffer()\n"));
	if (!XDGAOpenFramebuffer(DGA_Display, DGA_Screen)) {
		video_error_description_set("Unable to open the DGA Frame Buffer");
		return -1;
	}

	dga_state.active = 1;
	return 0;
}

void dga_done(void)
{
	assert(dga_is_active() && !dga_mode_is_active());

	log_std(("video:dga: dga_done()\n"));

	/* close up the frame buffer */
	log_std(("video:dga: XDGACloseFramebuffer()\n"));
	XDGACloseFramebuffer(DGA_Display, DGA_Screen);

	dga_state.active = 0;
}

adv_bool dga_is_active(void)
{
	return dga_state.active != 0;
}

adv_bool dga_mode_is_active(void)
{
	return dga_state.mode_active != 0;
}

unsigned dga_flags(void)
{
	assert(dga_is_active());
	return VIDEO_DRIVER_FLAGS_MODE_GRAPH_ALL | VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL;
}

/* From programs/Xserver/hw/xfree86/common/xf86str.h */
typedef enum {
	V_PHSYNC = 0x0001,
	V_NHSYNC = 0x0002,
	V_PVSYNC = 0x0004,
	V_NVSYNC = 0x0008,
	V_INTERLACE = 0x0010,
	V_DBLSCAN = 0x0020,
	V_CSYNC = 0x0040,
	V_PCSYNC = 0x0080,
	V_NCSYNC = 0x0100,
	V_HSKEW = 0x0200,
	V_BCAST = 0x0400,
	V_PIXMUX = 0x1000,
	V_DBLCLK = 0x2000,
	V_CLKDIV2 = 0x4000
} ModeFlags;

static void dga_print(void)
{
	unsigned i;
	XDGAMode* modes;
	int num_modes;
	int num_modelines;
	XF86VidModeModeInfo** modelines;

	log_std(("video:dga: XF86VidModeGetAllModeLines()\n"));
	if (!XF86VidModeGetAllModeLines(DGA_Display, DGA_Screen, &num_modelines, &modelines)) {
		log_std(("video:dga: XF86VidModeGetAllModeLines() failed\n"));
		video_error_description_set("Unable to list the available DGA modelines");
		return;
	}

	for(i=0;i<num_modelines;++i) {
		video_log_modeline_c("video:dga: modeline ", modelines[i]->dotclock * 1000, modelines[i]->hdisplay, modelines[i]->hsyncstart, modelines[i]->hsyncend, modelines[i]->htotal, modelines[i]->vdisplay, modelines[i]->vsyncstart, modelines[i]->vsyncend, modelines[i]->vtotal, (modelines[i]->flags & V_NHSYNC) != 0, (modelines[i]->flags & V_NVSYNC) != 0, (modelines[i]->flags & V_DBLSCAN) != 0, (modelines[i]->flags & V_INTERLACE) != 0);
		log_std(("video:dga: hskew: %d, flags:0x%x\n", modelines[i]->hskew, modelines[i]->flags));
	}

	XFree(modelines);

	log_std(("video:dga: XDGAQueryModes()\n"));
	modes = XDGAQueryModes(DGA_Display, DGA_Screen, &num_modes);
	if (!modes) {
		log_std(("video:dga: XDGAQueryModes() failed\n"));
		video_error_description_set("Unable to list the available DGA modes");
		return;
	}

	for(i=0;i<num_modes;++i) {
		log_std(("video:dga: mode num:%d, name:%s viewport:%dx%d, bits:%d, vclock:%g\n", (int)modes[i].num, (const char*)modes[i].name, (int)modes[i].viewportWidth, (int)modes[i].viewportHeight, (int)modes[i].bitsPerPixel, (double)modes[i].verticalRefresh));
	}

	XFree(modes);
}

adv_error dga_mode_set(const dga_video_mode* mode)
{
	XF86VidModeModeInfo modeline;
	Status st;
	unsigned i;
	XDGAMode* modes;
	int num_modes;
	int num;

	log_std(("video:dga: dga_mode_set()\n"));

	dga_print();

	modeline.flags = 0;
	if (crtc_is_doublescan(&mode->crtc))
		modeline.flags |= V_DBLSCAN;
	if (crtc_is_interlace(&mode->crtc))
		modeline.flags |= V_INTERLACE;
	if (crtc_is_nhsync(&mode->crtc))
		modeline.flags |= V_NHSYNC;
	else
		modeline.flags |= V_PHSYNC;
	if (crtc_is_nvsync(&mode->crtc))
		modeline.flags |= V_NVSYNC;
	else
		modeline.flags |= V_PVSYNC;
	modeline.dotclock = mode->crtc.pixelclock / 1000;
	modeline.hdisplay = mode->crtc.hde;
	modeline.hsyncstart = mode->crtc.hrs;
	modeline.hsyncend = mode->crtc.hre;
	modeline.htotal = mode->crtc.ht;
	modeline.hskew = 0;
	modeline.vdisplay = mode->crtc.vde;
	modeline.vsyncstart = mode->crtc.vrs;
	modeline.vsyncend = mode->crtc.vre;
	modeline.vtotal = mode->crtc.vt;
	modeline.privsize = 0;
	modeline.private = 0;

	video_log_modeline_c("video:dga: new modeline ", modeline.dotclock * 1000, modeline.hdisplay, modeline.hsyncstart, modeline.hsyncend, modeline.htotal, modeline.vdisplay, modeline.vsyncstart, modeline.vsyncend, modeline.vtotal, (modeline.flags & V_NHSYNC) != 0, (modeline.flags & V_NVSYNC) != 0, (modeline.flags & V_DBLSCAN) != 0, (modeline.flags & V_INTERLACE) != 0);

	log_std(("video:dga: XF86VidModeValidateModeLine()\n"));
	st = XF86VidModeValidateModeLine(DGA_Display, DGA_Screen, &modeline);
	if (st != 0) {
		log_std(("video:dga: XF86VidModeValidateModeLine() failed status:%d\n", (int)st));
		video_error_description_set("Unable to validate the DGA modeline");
		return -1;
	}

	log_std(("video:dga: XF86VidModeAddModeLine()\n"));
	if (!XF86VidModeAddModeLine(DGA_Display, DGA_Screen, &modeline, 0)) {
		log_std(("video:dga: XF86VidModeAddModeLine() failed\n"));
		video_error_description_set("Unable to add DGA modelines");
		return -1;
	}

	dga_print();

	log_std(("video:dga: XDGAQueryModes()\n"));
	modes = XDGAQueryModes(DGA_Display, DGA_Screen, &num_modes);
	if (!modes) {
		log_std(("video:dga: XDGAQueryModes() failed\n"));
		video_error_description_set("Unable to list the available DGA modes");
		return -1;
	}

	for(i=0;i<num_modes;++i) {
		if (modes[i].viewportWidth == mode->crtc.hde && modes[i].viewportHeight == mode->crtc.vde && mode->bits_per_pixel == modes[i].bitsPerPixel) {
			num = i;
			log_std(("video:dga: match mode num:%d, name:%s viewport:%dx%d, bits:%d, vclock:%g\n", (int)modes[i].num, (const char*)modes[i].name, (int)modes[i].viewportWidth, (int)modes[i].viewportHeight, (int)modes[i].bitsPerPixel, (double)modes[i].verticalRefresh));
			break;
		}
	}
	if (i==num_modes) {
		log_std(("video:dga: no mode match\n"));
		video_error_description_set("Unable to create the correct video mode");
	}

	XFree(modes);

	return -1;
}

adv_error dga_mode_change(const dga_video_mode* mode)
{
	assert(dga_is_active() && dga_mode_is_active());

	log_std(("video:dga: dga_mode_change()\n"));

	XFree(dga_state.device);

	dga_state.mode_active = 0; /* fake done */
	return dga_mode_set(mode);
}

void dga_mode_done(adv_bool restore)
{
	assert(dga_is_active() && dga_mode_is_active());

	log_std(("video:dga: dga_mode_done()\n"));

	XFree(dga_state.device);

	video_log("video:dga: XDGASetMode(0)\n");
	XDGASetMode(DGA_Display, DGA_Screen, 0);
	/* TODO XFree(dga_state.device); ?? */

	dga_state.mode_active = 0;
}

unsigned dga_virtual_x(void)
{
	unsigned size = dga_state.bytes_per_scanline / dga_state.bytes_per_pixel;
	size = size & ~0x7;
	return size;
}

unsigned dga_virtual_y(void)
{
	return dga_state.memory_size / dga_state.bytes_per_scanline;
}

unsigned dga_adjust_bytes_per_page(unsigned bytes_per_page)
{
	bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
	return bytes_per_page;
}

unsigned dga_bytes_per_scanline(void)
{
	return dga_state.bytes_per_scanline;
}

video_rgb_def dga_rgb_def(void)
{
	return video_rgb_def_make(dga_state.red_len, dga_state.red_pos, dga_state.green_len, dga_state.green_pos, dga_state.blue_len, dga_state.blue_pos);
}

void dga_wait_vsync(void)
{
	assert(dga_is_active() && dga_mode_is_active());
/* TODO
	vga_waitretrace();
*/
}

adv_error dga_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(dga_is_active() && dga_mode_is_active());
/* TODO
	if (waitvsync)
		vga_waitretrace();

	vga_setdisplaystart(offset);
*/
	return 0;
}

adv_error dga_scanline_set(unsigned byte_length)
{
	assert(dga_is_active() && dga_mode_is_active());
/* TODO
	vga_setlogicalwidth(byte_length);

	modeinfo = vga_getmodeinfo(dga_state.mode_number);
	if (!modeinfo) {
		video_error_description_set("Error in vga_getmodeinfo()");
		return -1;
	}
	dga_state.bytes_per_pixel = modeinfo->bytesperpixel;
	dga_state.bytes_per_scanline = modeinfo->linewidth;
*/

	return 0;
}

adv_error dga_palette8_set(const video_color* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
/* TODO
	if (waitvsync)
		vga_waitretrace();

	while (count) {
		vga_setpalette(start, palette->red >> 2, palette->green >> 2, palette->blue >> 2);
		++palette;
		++start;
		--count;
	}
*/

	return 0;
}

#define DRIVER(mode) ((dga_video_mode*)(&mode->driver_mode))

adv_error dga_mode_import(adv_mode* mode, const dga_video_mode* dga_mode)
{
	sncpy(mode->name, MODE_NAME_MAX, dga_mode->crtc.name);

	*DRIVER(mode) = *dga_mode;

	// TODO VIDEO_FLAGS_SYNC_SETPAGE ?
	mode->driver = &video_dga_driver;
	mode->flags = VIDEO_FLAGS_ASYNC_SETPAGE
		| VIDEO_FLAGS_MEMORY_LINEAR
		| (mode->flags & VIDEO_FLAGS_USER_MASK);
	switch (dga_mode->bits_per_pixel) {
		case 8 : mode->flags |= VIDEO_FLAGS_INDEX_PACKED | VIDEO_FLAGS_TYPE_GRAPHICS; break;
		default: mode->flags |= VIDEO_FLAGS_INDEX_RGB | VIDEO_FLAGS_TYPE_GRAPHICS; break;
	}

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->bits_per_pixel = dga_mode->bits_per_pixel;
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

adv_error dga_mode_generate(dga_video_mode* mode, const adv_crtc* crtc, unsigned bits, unsigned flags)
{
	assert(dga_is_active());

	if (video_mode_generate_check("dga", dga_flags(), 8, 2048, crtc, bits, flags)!=0)
		return -1;

	mode->crtc = *crtc;
	mode->bits_per_pixel = bits;

	return 0;
}

#define COMPARE(a, b) \
	if (a < b) \
		return -1; \
	if (a > b) \
		return 1

int dga_mode_compare(const dga_video_mode* a, const dga_video_mode* b)
{
	COMPARE(a->bits_per_pixel, b->bits_per_pixel);
	return video_crtc_compare(&a->crtc, &b->crtc);
}

void dga_default(void)
{
}

void dga_reg(adv_conf* context)
{
	assert(!dga_is_active());
}

adv_error dga_load(adv_conf* context)
{
	assert(!dga_is_active());
	return 0;
}

/***************************************************************************/
/* Driver */

static adv_error dga_mode_set_void(const void* mode)
{
	return dga_mode_set((const dga_video_mode*)mode);
}

static adv_error dga_mode_change_void(const void* mode)
{
	return dga_mode_change((const dga_video_mode*)mode);
}

static adv_error dga_mode_import_void(adv_mode* mode, const void* dga_mode)
{
	return dga_mode_import(mode, (const dga_video_mode*)dga_mode);
}

static adv_error dga_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned bits, unsigned flags)
{
	return dga_mode_generate((dga_video_mode*)mode, crtc, bits, flags);
}

static int dga_mode_compare_void(const void* a, const void* b)
{
	return dga_mode_compare((const dga_video_mode*)a, (const dga_video_mode*)b);
}

static unsigned dga_mode_size(void)
{
	return sizeof(dga_video_mode);
}

adv_video_driver video_dga_driver = {
	"dga",
	DEVICE,
	dga_load,
	dga_reg,
	dga_init,
	dga_done,
	dga_flags,
	dga_mode_set_void,
	dga_mode_change_void,
	dga_mode_done,
	dga_virtual_x,
	dga_virtual_y,
	0,
	0,
	dga_bytes_per_scanline,
	dga_adjust_bytes_per_page,
	dga_rgb_def,
	0,
	0,
	&dga_write_line,
	dga_wait_vsync,
	dga_scroll,
	dga_scanline_set,
	dga_palette8_set,
	0,
	dga_mode_size,
	0,
	dga_mode_generate_void,
	dga_mode_import_void,
	dga_mode_compare_void,
	0
};

