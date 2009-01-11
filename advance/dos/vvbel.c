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

#include "vvbel.h"
#include "scrvbe.h"
#include "video.h"
#include "log.h"
#include "error.h"
#include "snstring.h"
#include "osdos.h"

#include "board.h"

/***************************************************************************/
/* State */

/* Driver capability */
#define VBELINE_FLAGS_REQUIRES_VBE3_SET VIDEO_DRIVER_FLAGS_INTERNAL_BIT0 /* Require a vbe3 mode set */
#define VBELINE_FLAGS_REQUIRES_VBE_SAVE (VIDEO_DRIVER_FLAGS_INTERNAL_BIT0 << 1) /* Require a mode save/restore with the vbe services */

typedef struct vbeline_chipset_struct {
	const char* name; /* Driver name */
	unsigned flags; /* Driver capability */
	int (*detect)(void);
	int (*set)(const card_crtc*, const card_mode*, const card_mode*);
	void (*reset)(void);
} vbeline_chipset;

typedef struct vbeline_internal_struct {
	adv_bool active; /* !=0 if present */
	vbeline_chipset* driver;
	adv_bool mode_active; /* !=0 if mode set */
	adv_bool enable_vbe; /**< !=0 if vbe_init() was called */

	unsigned size_x;
	unsigned size_y;
	unsigned virtual_x;
	unsigned virtual_y;

	void* state_ptr;
	unsigned state_size;

	unsigned flags;
} vbeline_internal;

static vbeline_internal vbeline_state;

unsigned char* (*vbeline_write_line)(unsigned y);

/***************************************************************************/
/* Options */

#define VBELINE_OPTION_CHOICE_BIGGER 0 /* Bigger mode */
#define VBELINE_OPTION_CHOICE_SMALLER 1 /* Smaller mode */
#define VBELINE_OPTION_CHOICE_SMALLER_UPTO640 2
#define VBELINE_OPTION_CHOICE_BIGGER_UPTO640 3
#define VBELINE_OPTION_CHOICE_320 4
#define VBELINE_OPTION_CHOICE_400 5
#define VBELINE_OPTION_CHOICE_512 6
#define VBELINE_OPTION_CHOICE_640 7
#define VBELINE_OPTION_CHOICE_800 8

struct vbeline_option_struct {
	adv_bool initialized;
	double clock_16bit_multiplier;
	double clock_8bit_multiplier;
	double clock_multiplier;
	int choice;
};

static struct vbeline_option_struct vbeline_option;

/***************************************************************************/
/* Card interface */

#define VIDEO_DRIVER_FLAGS_MODE_12BYTES \
	VIDEO_DRIVER_FLAGS_MODE_PALETTE8 | VIDEO_DRIVER_FLAGS_MODE_BGR15 | VIDEO_DRIVER_FLAGS_MODE_BGR16

static adv_device DEVICE[] = {
{ "auto", 1, "VBELINE video" },
{ "laguna", 2, "Cirrus Laguna" },
{ "3dfx", 3, "3dfx" },
{ "savage", 4, "S3 Savage" },
{ "sis", 5, "SiS" },
{ "matrox", 6, "Matrox" },
{ "r128", 7, "ATI Rage 128" },
{ "neomagic", 8, "NeoMagic" },
{ "s3", 9, "S3" },
{ "trident", 10, "Trident" },
{ "ati", 11, "ATI" },
{ "cirrus", 12, "Cirrus Logic" },
{ "vbe3", 13, "VESA VBE3" },
{ 0, 0, 0 }
};

vbeline_chipset cards[] =
{
	{ "laguna", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, laguna_detect, laguna_set, laguna_reset }, /* Laguna driver */
	{ "3dfx", (VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL & ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) | VIDEO_DRIVER_FLAGS_MODE_12BYTES, tdfx_detect, tdfx_set, tdfx_reset }, /* 3dfx driver */
	{ "savage", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, savage_detect, savage_set, savage_reset}, /* Savage driver */
	{ "sis", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, sis_detect, sis_set, sis_reset }, /* Sis driver */
	{ "matrox", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, matrox_detect, matrox_set, matrox_reset }, /* Matrox driver */
	{ "r128", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES | VBELINE_FLAGS_REQUIRES_VBE_SAVE, r128_detect, r128_set, r128_reset }, /* R128 driver */
	{ "neomagic", (VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL & ~VIDEO_DRIVER_FLAGS_PROGRAMMABLE_INTERLACE) | VIDEO_DRIVER_FLAGS_MODE_12BYTES, neomagic_detect, neomagic_set, neomagic_reset }, /* NeoMagic driver */
	{ "s3", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, s3_detect, s3_set, s3_reset }, /* S3 driver */
	{ "trident", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, trident_detect, trident_set, trident_reset }, /* Trident driver */
	{ "ati", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, ati_detect, ati_set, ati_reset } , /* ATI driver */
	{ "cirrus", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES, cirrus_detect, cirrus_set, cirrus_reset }, /* Cirrus driver */
	{ "vbe3", VIDEO_DRIVER_FLAGS_PROGRAMMABLE_ALL | VIDEO_DRIVER_FLAGS_MODE_12BYTES | VBELINE_FLAGS_REQUIRES_VBE3_SET, vbe3_detect, vbe3_set, vbe3_reset }, /* VBE3 driver */
	{ 0, 0, 0, 0, 0 }
};

#define CARD_MAX (sizeof(cards)/sizeof(cards[0]) - 1)

static void vbeline_card_old_setup(card_mode* co, const vbe_ModeInfoBlock* info)
{

	/* a lot of values are NOT initialized */
	memset(co, 0, sizeof(card_mode));
	co->width = info->XResolution;
	co->height = info->YResolution;
	co->bits_per_pixel = info->BitsPerPixel;
}

static void vbeline_card_setup(card_crtc* cp, card_mode* cm, const vbe_ModeInfoBlock* info, const adv_crtc* crtc, unsigned mode)
{

	/* a lot of values are NOT initialized */
	memset(cm, 0, sizeof(card_mode));
	cm->width = crtc->hde;
	cm->height = crtc->vde;
	cm->bits_per_pixel = info->BitsPerPixel;

	/* crtc */
	cp->HDisp = crtc->hde;
	cp->HSStart = crtc->hrs;
	cp->HSEnd = crtc->hre;
	cp->HTotal = crtc->ht;

	cp->VDisp = crtc->vde;
	cp->VSStart = crtc->vrs;
	cp->VSEnd = crtc->vre;
	cp->VTotal = crtc->vt;

	if (crtc_is_doublescan(crtc)) {
		cp->VDisp *= 2;
		cp->VSStart *= 2;
		cp->VSEnd *= 2;
		cp->VTotal *= 2;
	}
	if (crtc_is_interlace(crtc)) {
		cp->VDisp /= 2;
		cp->VSStart /= 2;
		cp->VSEnd /= 2;
		cp->VTotal /= 2;
	}

	/* blank */
	cp->HBStart = cp->HDisp;
	if (cp->HBStart > cp->HSStart)
		cp->HBStart = cp->HSStart;
	cp->HBEnd = cp->HSEnd;
	cp->VBStart = cp->VDisp;
	if (cp->VBStart > cp->VSStart)
		cp->VBStart = cp->VSStart;
	cp->VBEnd = cp->VSEnd;

	cp->hpolarity = crtc_is_nhsync(crtc);
	cp->vpolarity = crtc_is_nvsync(crtc);
	cp->doublescan = crtc_is_doublescan(crtc);
	cp->interlace = crtc_is_interlace(crtc);
	cp->interlaceratio = 50;

	cp->dotclockHz = crtc->pixelclock;

	log_std_modeline_cb(("card: mode_set modeline", cp->dotclockHz, cp->HDisp, cp->HBStart, cp->HSStart, cp->HSEnd, cp->HBEnd, cp->HTotal, cp->VDisp, cp->VBStart, cp->VSStart, cp->VSEnd, cp->VBEnd, cp->VTotal, cp->hpolarity, cp->vpolarity, cp->doublescan, cp->interlace));
}

void card_log(const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	log_va(text, arg);
	va_end(arg);
}

/***************************************************************************/
/* Public */

static void vbeline_probe(void)
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
		vbeline_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_PALETTE8;
	if (!has15bit)
		vbeline_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR15;
	if (!has16bit)
		vbeline_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR16;
	if (!has24bit)
		vbeline_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR24;
	if (!has32bit)
		vbeline_state.flags &= ~VIDEO_DRIVER_FLAGS_MODE_BGR32;
}

adv_error vbeline_init(int device_id, adv_output output, unsigned overlay_size, adv_cursor cursor)
{
	unsigned i;
	const char* name;
	const adv_device* j;

	(void)cursor;

	assert(!vbeline_is_active());

	if (sizeof(vbeline_video_mode) > MODE_DRIVER_MODE_SIZE_MAX)
		return -1;

	if (os_internal_brokenint10_active()) {
		error_set("Detected broken video BIOS.\n");
		return -1;
	}

	if (output != adv_output_auto && output != adv_output_fullscreen) {
		error_set("Only fullscreen output is supported.\n");
		return -1;
	}

	j = DEVICE;
	while (j->name && j->id != device_id)
		++j;
	if (!j->name)
		return -1;
	name = j->name;

	/* VBE is required */
	vbeline_state.enable_vbe = 0;
	if (!vbe_is_active()) {
		if (vbe_init() != 0) {
			log_std(("vbeline: a vbe bios is required\n"));
			return -1;
		}
		vbeline_state.enable_vbe = 1;
	}

	if (!vbeline_option.initialized) {
		vbeline_default();
	}

	vbeline_state.driver = 0;
	for(i=0;cards[i].name;++i) {
		if (strcmp(name, "auto")==0 || strcmp(name, cards[i].name)==0) {
			if ((cards[i].flags & VBELINE_FLAGS_REQUIRES_VBE3_SET)==0 || vbe_state.info.VESAVersion >= 0x300) {
				if (cards[i].detect && cards[i].detect() > 0) {
					log_std(("vbeline: found driver %s\n", cards[i].name));
					vbeline_state.driver = cards + i;
					vbeline_state.flags = vbeline_state.driver->flags;
					break;
				}
			}
		}
	}

	if (vbeline_state.driver == 0) {
		/* disable the vbe */
		if (vbeline_state.enable_vbe) {
			vbe_done();
			vbeline_state.enable_vbe = 0;
		}
		return -1;
	}

	vbeline_state.flags |= VIDEO_DRIVER_FLAGS_OUTPUT_FULLSCREEN
		| VIDEO_DRIVER_FLAGS_INTERNAL_DANGEROUSCHANGE;

	vbeline_probe();

	vbeline_state.active = 1;

	return 0;
}

void vbeline_done(void)
{
	assert(vbeline_is_active());
	assert(!vbeline_mode_is_active());

	if (vbeline_state.driver->reset)
		vbeline_state.driver->reset();

	if (vbeline_state.enable_vbe) {
		vbe_done();
		vbeline_state.enable_vbe = 0;
	}

	vbeline_state.active = 0;
}

adv_bool vbeline_is_active(void)
{
	return vbeline_state.active != 0;
}

adv_bool vbeline_mode_is_active(void)
{
	return vbeline_state.mode_active != 0;
}

const char* vbeline_driver_name(void)
{
	assert(vbeline_is_active());
	return vbeline_state.driver->name;
}

const char* vbeline_driver_description(void)
{
	assert(vbeline_is_active());
	return "";
}

unsigned vbeline_flags(void)
{
	assert(vbeline_is_active());
	return vbeline_state.flags;
}

static int vbe3_mode_preset(const card_crtc* ccp, vbe_ModeInfoBlock* info, vbe_CRTCInfoBlock* crtc)
{
	if ((info->ModeAttributes & vbeMdAvailable) == 0) {
		error_set("VBE report that the selected mode isn't avaliable");
		return -1;
	}

	if (ccp->interlace && (info->ModeAttributes & vbeMdInterlace)==0) {
		error_set("VBE report that the selected mode doesn't support interlace");
		return -1;
	}

	if (ccp->doublescan && (info->ModeAttributes & vbeMdDoubleScan)==0) {
		error_set("VBE report that the selected mode doesn't support doublescan");
		return -1;
	}

	if (ccp->interlace && ccp->doublescan) {
		error_set("VBE doesn't support interlaced and doublescan at the same time");
		return -1;
	}

	memset(crtc, 0, sizeof(vbe_CRTCInfoBlock));

	/* if the resulting mode is smaller and if later sync values are set */
	/* use fake value to prevent a possible mode set failure if the */
	/* specified values are smaller than the fixed resolution */

	if (ccp->HDisp < info->XResolution) {
		/* use fake values, correct values are set later */
		crtc->HorizontalSyncStart = info->XResolution * 41 / 40;
		crtc->HorizontalSyncEnd = info->XResolution * 10 / 9;
		crtc->HorizontalTotal = info->XResolution * 5 / 4;
	} else {
		crtc->HorizontalTotal = ccp->HTotal;
		crtc->HorizontalSyncStart = ccp->HSStart;
		crtc->HorizontalSyncEnd = ccp->HSEnd;
	}
	if (ccp->VDisp < info->YResolution) {
		/* use fake values, correct values are set later */
		crtc->VerticalSyncStart = info->YResolution * 31 / 30;
		crtc->VerticalSyncEnd = info->YResolution * 21 / 20;
		crtc->VerticalTotal = info->YResolution * 16 / 15;

		/* VBE3 requires doubled values for doublescan mode */
		/* but NOT half values for interlaced modes */
		if (ccp->doublescan) {
			crtc->VerticalTotal *= 2;
			crtc->VerticalSyncStart *= 2;
			crtc->VerticalSyncEnd *= 2;
		}
	} else {
		crtc->VerticalTotal = ccp->VTotal;
		crtc->VerticalSyncStart = ccp->VSStart;
		crtc->VerticalSyncEnd = ccp->VSEnd;

		/* VBE3 requires doubled values for doublescan mode */
		/* but NOT half values for interlaced modes */
		if (ccp->interlace) {
			crtc->VerticalTotal *= 2;
			crtc->VerticalSyncStart *= 2;
			crtc->VerticalSyncEnd *= 2;
		}
	}

	crtc->Flags = 0;
	if (ccp->doublescan)
		crtc->Flags |= vbeCRTCDoubleScan;
	if (ccp->interlace)
		crtc->Flags |= vbeCRTCInterlaced;
	if (ccp->hpolarity)
		crtc->Flags |= vbeCRTCHorizontalSyncNegative;
	if (ccp->vpolarity)
		crtc->Flags |= vbeCRTCVerticalSyncNegative;

	crtc->PixelClock = ccp->dotclockHz;
	crtc->RefreshRate = (long)crtc->PixelClock * 100 / ((long)crtc->HorizontalTotal * crtc->VerticalTotal);

	log_std(("vbeline: mode_preset %dx%d (%.2f %d %d %d %d %d %d)\n", (unsigned)info->XResolution, (unsigned)info->YResolution, (double)crtc->PixelClock/1E6, (unsigned)crtc->HorizontalSyncStart, (unsigned)crtc->HorizontalSyncEnd, (unsigned)crtc->HorizontalTotal, (unsigned)crtc->VerticalSyncStart, (unsigned)crtc->VerticalSyncEnd, (unsigned)crtc->VerticalTotal));

	return 0;
}

adv_error vbeline_mode_set(const vbeline_video_mode* mode)
{
	unsigned mode_request = mode->mode;
	adv_crtc crtc_request = mode->crtc;
	vbe_CRTCInfoBlock vbe3_crtc_request;
	vbe_ModeInfoBlock vbe_info_request;

	card_crtc cp; /* requested crtc values */
	card_mode cm; /* requested mode info */
	card_mode co; /* current mode info */

	assert(vbeline_is_active());
	assert(!vbeline_mode_is_active());

	log_std(("vbeline: mode_set number 0x%x\n", mode->mode));
	log_std_modeline_c(("vbeline: mode_set modeline", mode->crtc.pixelclock, mode->crtc.hde, mode->crtc.hrs, mode->crtc.hre, mode->crtc.ht, mode->crtc.vde, mode->crtc.vrs, mode->crtc.vre, mode->crtc.vt, crtc_is_nhsync(&mode->crtc), crtc_is_nvsync(&mode->crtc), crtc_is_doublescan(&mode->crtc), crtc_is_interlace(&mode->crtc)));
	log_std(("vbeline: expected vert clock: %.2f Hz\n", crtc_vclock_get(&mode->crtc)));

	if ((vbeline_flags() & VBELINE_FLAGS_REQUIRES_VBE_SAVE)!=0) {
		log_std(("vbeline: save video board state\n"));
		if (vbe_save_state(&vbeline_state.state_size, &vbeline_state.state_ptr) != 0) {
			return -1;
		}
	}

	if (vbe_mode_info_get(&vbe_info_request, mode_request)!=0)
		return -1;

	if (vbeline_option.clock_multiplier!=1.0) {
		log_std(("vbeline: clock multiplier: %.2f\n", vbeline_option.clock_multiplier));
		crtc_request.pixelclock *= vbeline_option.clock_multiplier;
	}

	if (vbe_info_request.BitsPerPixel==8 && vbeline_option.clock_8bit_multiplier!=1.0) {
		log_std(("vbeline: 8bit_clock multiplier: %.2f\n", vbeline_option.clock_8bit_multiplier));
		crtc_request.pixelclock *= vbeline_option.clock_8bit_multiplier;
	}

	if ((vbe_info_request.BitsPerPixel==15 || vbe_info_request.BitsPerPixel==16) && vbeline_option.clock_16bit_multiplier!=1.0) {
		log_std(("vbeline: 16bit_clock multiplier: %.2f\n", vbeline_option.clock_16bit_multiplier));
		crtc_request.pixelclock *= vbeline_option.clock_16bit_multiplier;
	}

	/* setup the card crtc info */
	vbeline_card_old_setup(&co, &vbe_info_request);
	vbeline_card_setup(&cp, &cm, &vbe_info_request, &crtc_request, mode_request);

	if ((vbeline_flags() & VBELINE_FLAGS_REQUIRES_VBE3_SET)!=0) {
		if (vbe_pixelclock_get(&crtc_request.pixelclock, mode_request)!=0) {
			return -1;
		}
		if (vbe3_mode_preset(&cp, &vbe_info_request, &vbe3_crtc_request)!=0) {
			return -1;
		}
		if (vbe_mode_set(mode_request, &vbe3_crtc_request)!=0) {
			return -1;
		}
	} else {
		if (vbe_mode_set(mode_request, 0)!=0) {
			return -1;
		}
	}

	if (vbe_scanline_pixel_request(crtc_request.hde) != 0) {
		vbe_mode_done(1);
		return -1;
	}

	if (vbeline_state.driver->set(&cp, &cm, &co) <= 0) {
		log_std(("vbeline: error in the video driver mode set\n"));
		vbe_mode_done(1);
		error_set("Error in the video driver mode set");
		return -1;
	}

	/* adjust resolution */
	vbeline_state.size_x = vbe_state.size_x = crtc_request.hde;
	vbeline_state.size_y = vbe_state.size_y = crtc_request.vde;
	/* adjust virtual size */
	vbeline_state.virtual_x = vbe_state.virtual_x;
	vbeline_state.virtual_y = vbe_state.virtual_y;
	/* write handler */
	vbeline_write_line = vbe_write_line;

	vbeline_state.mode_active = 1;

	return 0;
}

void vbeline_mode_done(adv_bool restore)
{
	assert(vbeline_is_active());
	assert(vbeline_mode_is_active());

	if ((vbeline_flags() & VBELINE_FLAGS_REQUIRES_VBE_SAVE)!=0) {
		log_std(("vbeline: restore video board state\n"));
		vbe_restore_state(vbeline_state.state_size, vbeline_state.state_ptr);
	}

	vbe_mode_done(restore);

	vbeline_state.mode_active = 0;
}

adv_error vbeline_mode_change(const vbeline_video_mode* mode)
{
	vbeline_mode_done(0);

	return vbeline_mode_set(mode);
}

unsigned vbeline_virtual_x(void)
{
	return vbeline_state.virtual_x;
}

unsigned vbeline_virtual_y(void)
{
	return vbeline_state.virtual_y;
}

void vbeline_wait_vsync(void)
{
	vbe_wait_vsync();
}

adv_error vbeline_palette8_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
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

#define CARD_PIXELCLOCK_DELTA (10*1000)
#define CARD_PIXELCLOCK_MAX (250*1000*1000)
#define CARD_PIXELCLOCK_MIN (1*1000*1000)

int vbeline_pixelclock_getnext(unsigned* pixelclock, unsigned mode)
{
	assert(vbeline_is_active());
	if ((vbeline_state.flags & VBELINE_FLAGS_REQUIRES_VBE3_SET)!=0) {
		return vbe_pixelclock_getnext(pixelclock, mode);
	} else {
		/* assume all clocks avaliable */
		if (*pixelclock + CARD_PIXELCLOCK_DELTA < CARD_PIXELCLOCK_MAX)
			*pixelclock += CARD_PIXELCLOCK_DELTA;
		return 0;
	}
}

int vbeline_pixelclock_getpred(unsigned* pixelclock, unsigned mode)
{
	assert(vbeline_is_active());
	if ((vbeline_state.flags & VBELINE_FLAGS_REQUIRES_VBE3_SET)!=0) {
		return vbe_pixelclock_getpred(pixelclock, mode);
	} else {
		/* assume all clocks avaliable */
		if (*pixelclock > CARD_PIXELCLOCK_MIN - CARD_PIXELCLOCK_DELTA)
			*pixelclock -= CARD_PIXELCLOCK_DELTA;
		return 0;
	}
}

unsigned vbeline_mode_size(void)
{
	return sizeof(vbeline_video_mode);
}

#define DRIVER(mode) ((vbeline_video_mode*)(&mode->driver_mode))

adv_error vbeline_mode_import(adv_mode* mode, const vbeline_video_mode* vbeline_mode)
{
	vbe_ModeInfoBlock info;

	sncpy(mode->name, MODE_NAME_MAX, vbeline_mode->crtc.name);
	*DRIVER(mode) = *vbeline_mode;

	if (!vbeline_is_active()) {
		error_nolog_set("VBE CRTC programming isn't avaliable on VBE version prior of 3 or without the dedicated driver");
		return -1;
	}

	if (vbe_mode_info_get(&info, DRIVER(mode)->mode)!=0) {
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
				error_nolog_set("Linear frame buffer isn't available on VBE version prior 2");
				return -1;
			}
			if ((info.ModeAttributes & vbeMdLinear) == 0) {
				error_nolog_set("Linear frame buffer isn't available in mode %x", DRIVER(mode)->mode);
				return -1;
			}
		} else {
			if ((info.ModeAttributes & vbeMdNonBanked) != 0) {
				error_nolog_set("Banked frame buffer isn't available in mode %x", DRIVER(mode)->mode);
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

	if ((vbeline_state.flags & VBELINE_FLAGS_REQUIRES_VBE3_SET)!=0) {
		if (vbe_pixelclock_get(&DRIVER(mode)->crtc.pixelclock, DRIVER(mode)->mode)!=0) {
			return -1;
		}
	} else {
		/* assume all clocks available */
	}

	mode->driver = &video_vbeline_driver;
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

	mode->size_x = DRIVER(mode)->crtc.hde;
	mode->size_y = DRIVER(mode)->crtc.vde;
	mode->vclock = crtc_vclock_get(&DRIVER(mode)->crtc);
	mode->hclock = crtc_hclock_get(&DRIVER(mode)->crtc);
	mode->scan = crtc_scan_get(&DRIVER(mode)->crtc);

	return 0;
}

static adv_error vbeline_search_target_mode(unsigned req_x, unsigned req_y, unsigned bits, unsigned model)
{
	unsigned smaller_best_mode = 0; /* assignement to prevent warning */
	unsigned smaller_best_size_x = 0; /* assignement to prevent warning */
	unsigned smaller_best_size_y = 0; /* assignement to prevent warning */
	int smaller_best_set = 0;

	unsigned bigger_best_mode = 0; /* assignement to prevent warning */
	unsigned bigger_best_size_x = 0; /* assignement to prevent warning */
	unsigned bigger_best_size_y = 0; /* assignement to prevent warning */
	int bigger_best_set = 0;

	unsigned size320_best_mode = 0;
	int size320_best_set = 0;
	unsigned size400_best_mode = 0;
	int size400_best_set = 0;
	unsigned size512_best_mode = 0;
	int size512_best_set = 0;
	unsigned size640_best_mode = 0;
	int size640_best_set = 0;
	unsigned size800_best_mode = 0;
	int size800_best_set = 0;

	unsigned flags = vbeMdAvailable | vbeMdGraphMode | vbeMdLinear;

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
			&& info.MemoryModel == model) {

			if (info.XResolution >= req_x && info.YResolution >= req_y) {
				if (!bigger_best_set
					|| (info.XResolution <= bigger_best_size_x && info.YResolution <= bigger_best_size_y)) {
					bigger_best_set = 1;
					bigger_best_mode = mode;
					bigger_best_size_x = info.XResolution;
					bigger_best_size_y = info.YResolution;
				}
			}

			if (info.XResolution <= req_x && info.YResolution <= req_y) {
				if (!smaller_best_set
					|| (info.XResolution >= smaller_best_size_x && info.YResolution >= smaller_best_size_y)) {
					smaller_best_set = 1;
					smaller_best_mode = mode;
					smaller_best_size_x = info.XResolution;
					smaller_best_size_y = info.YResolution;
				}
			}

			if (info.XResolution == 320 && info.YResolution == 240) {
				if (!size320_best_set) {
					size320_best_set = 1;
					size320_best_mode = mode;
				}
			}

			if (info.XResolution == 400 && info.YResolution == 300) {
				if (!size400_best_set) {
					size400_best_set = 1;
					size400_best_mode = mode;
				}
			}

			if (info.XResolution == 512 && info.YResolution == 384) {
				if (!size512_best_set) {
					size512_best_set = 1;
					size512_best_mode = mode;
				}
			}

			if (info.XResolution == 640 && info.YResolution == 480) {
				if (!size640_best_set) {
					size640_best_set = 1;
					size640_best_mode = mode;
				}
			}

			if (info.XResolution == 800 && info.YResolution == 600) {
				if (!size800_best_set) {
					size800_best_set = 1;
					size800_best_mode = mode;
				}
			}
		}

		vbe_mode_iterator_next(&i);
	}

	switch (vbeline_option.choice) {
		case VBELINE_OPTION_CHOICE_SMALLER :
			if (smaller_best_set)
				return smaller_best_mode;
			if (bigger_best_set)
				return bigger_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_SMALLER_UPTO640 :
			if (smaller_best_set && smaller_best_size_x <= 640)
				return smaller_best_mode;
			if (bigger_best_set && bigger_best_size_x <= 640)
				return bigger_best_mode;
			if (size640_best_set)
				return size640_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_BIGGER :
			if (bigger_best_set)
				return bigger_best_mode;
			if (smaller_best_set)
				return smaller_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_BIGGER_UPTO640 :
			if (bigger_best_set && bigger_best_size_x <= 640)
				return bigger_best_mode;
			if (smaller_best_set && smaller_best_size_x <= 640)
				return smaller_best_mode;
			if (size640_best_set)
				return size640_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_800 :
			if (size800_best_set)
				return size800_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_640 :
			if (size640_best_set)
				return size640_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_512 :
			if (size512_best_set)
				return size512_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_400 :
			if (size400_best_set)
				return size400_best_mode;
			return -1;
		case VBELINE_OPTION_CHOICE_320 :
			if (size320_best_set)
				return size320_best_mode;
			return -1;
	}

	return -1;
}

adv_error vbeline_mode_generate(vbeline_video_mode* mode, const adv_crtc* crtc, unsigned flags)
{
	int number;
	unsigned model;
	unsigned bits;

	assert(vbeline_is_active());

	if (crtc_is_fake(crtc)) {
		error_nolog_set("Not programmable modes are not supported.\n");
		return -1;
	}

	if (video_mode_generate_check("vbeline", vbeline_flags(), 8, 2048, crtc, flags)!=0)
		return -1;

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
			error_nolog_set("Invalid index mode.\n");
			assert(0);
			return -1;
	}

	mode->crtc = *crtc;

	number = vbeline_search_target_mode(mode->crtc.hde, mode->crtc.vde, bits, model);
	if (number < 0 && model == vbeMemRGB) {
		model = vbeMemPK; /* the packed mode is better than RGB */
		number = vbeline_search_target_mode(mode->crtc.hde, mode->crtc.vde, bits, model);
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

int vbeline_mode_compare(const vbeline_video_mode* a, const vbeline_video_mode* b)
{
	COMPARE(a->mode, b->mode);
	return crtc_compare(&a->crtc, &b->crtc);
}

void vbeline_default(void)
{
	vbeline_option.clock_16bit_multiplier = 1.0;
	vbeline_option.clock_8bit_multiplier = 1.0;
	vbeline_option.clock_multiplier = 1.0;
	vbeline_option.choice = VBELINE_OPTION_CHOICE_BIGGER;

	vbeline_option.initialized = 1;
}

static adv_conf_enum_int OPTION_CHOICE[] = {
{ "bigger", VBELINE_OPTION_CHOICE_BIGGER },
{ "smaller", VBELINE_OPTION_CHOICE_SMALLER },
{ "smaller_upto640", VBELINE_OPTION_CHOICE_SMALLER_UPTO640 },
{ "bigger_upto640", VBELINE_OPTION_CHOICE_BIGGER_UPTO640 },
{ "800", VBELINE_OPTION_CHOICE_800 },
{ "640", VBELINE_OPTION_CHOICE_640 },
{ "512", VBELINE_OPTION_CHOICE_512 },
{ "400", VBELINE_OPTION_CHOICE_400 },
{ "320", VBELINE_OPTION_CHOICE_320 }
};

static adv_conf_enum_string OPTION_DRIVER[CARD_MAX + 2];

void vbeline_reg(adv_conf* context)
{
	unsigned i;

	assert(!vbeline_is_active());

	OPTION_DRIVER[0].value = "auto";
	OPTION_DRIVER[1].value = "none";
	for(i=0;cards[i].name;++i)
		OPTION_DRIVER[i+2].value = cards[i].name;

	conf_int_register_enum_default(context, "device_vbeline_mode", conf_enum(OPTION_CHOICE), VBELINE_OPTION_CHOICE_BIGGER);
	conf_float_register_limit_default(context, "device_vbeline_clock_multiplier", 0.25, 4.0, 1.0);
	conf_float_register_limit_default(context, "device_vbeline_8bit_clock_multiplier", 0.25, 4.0, 1.0);
	conf_float_register_limit_default(context, "device_vbeline_16bit_clock_multiplier", 0.25, 4.0, 1.0);
}

adv_error vbeline_load(adv_conf* context)
{

	/* Options must be loaded before initialization */
	assert(!vbeline_is_active());

	vbeline_option.choice = conf_int_get_default(context, "device_vbeline_mode");
	vbeline_option.clock_multiplier = conf_float_get_default(context, "device_vbeline_clock_multiplier");
	vbeline_option.clock_8bit_multiplier = conf_float_get_default(context, "device_vbeline_8bit_clock_multiplier");
	vbeline_option.clock_16bit_multiplier = conf_float_get_default(context, "device_vbeline_16bit_clock_multiplier");

	log_std(("vbeline: load vbeline_mode %d\n", vbeline_option.choice));
	log_std(("vbeline: load vbeline_clock_multiplier %g\n", (double)vbeline_option.clock_multiplier));
	log_std(("vbeline: load vbeline_8bit_clock_multiplier %g\n", (double)vbeline_option.clock_8bit_multiplier));
	log_std(("vbeline: load vbeline_16bit_clock_multiplier %g\n", (double)vbeline_option.clock_16bit_multiplier));

	vbeline_option.initialized = 1;

	return 0;
}

void vbeline_crtc_container_insert_default(adv_crtc_container* cc)
{
	log_std(("video:vbeline: vbeline_crtc_container_insert_default()\n"));

	crtc_container_insert_default_modeline_svga(cc);
}

/***************************************************************************/
/* Driver */

static adv_error vbeline_mode_change_void(const void* mode)
{
	return vbeline_mode_change((const vbeline_video_mode*)mode);
}

static adv_error vbeline_mode_set_void(const void* mode)
{
	return vbeline_mode_set((const vbeline_video_mode*)mode);
}

static adv_error vbeline_mode_import_void(adv_mode* mode, const void* vbeline_mode)
{
	return vbeline_mode_import(mode, (const vbeline_video_mode*)vbeline_mode);
}

static adv_error vbeline_mode_generate_void(void* mode, const adv_crtc* crtc, unsigned flags)
{
	return vbeline_mode_generate((vbeline_video_mode*)mode, crtc, flags);
}

static int vbeline_mode_compare_void(const void* a, const void* b)
{
	return vbeline_mode_compare((const vbeline_video_mode*)a, (const vbeline_video_mode*)b);
}

adv_video_driver video_vbeline_driver = {
	"vbeline",
	DEVICE,
	vbeline_load,
	vbeline_reg,
	vbeline_init,
	vbeline_done,
	vbeline_flags,
	vbeline_mode_set_void,
	vbeline_mode_change_void,
	vbeline_mode_done,
	vbeline_virtual_x,
	vbeline_virtual_y,
	vbe_font_size_x,
	vbe_font_size_y,
	vbe_bytes_per_scanline,
	vbe_adjust_bytes_per_page,
	vbe_color_def,
	0,
	0,
	&vbeline_write_line,
	vbeline_wait_vsync,
	vbe_scroll,
	vbe_scanline_set,
	vbeline_palette8_set,
	vbeline_mode_size,
	0,
	vbeline_mode_generate_void,
	vbeline_mode_import_void,
	vbeline_mode_compare_void,
	vbeline_crtc_container_insert_default
};

