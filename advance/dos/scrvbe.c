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

#include "scrvbe.h"
#include "scrvga.h"
#include "log.h"
#include "error.h"
#include "snstring.h"

#include "map.h"

#include <dpmi.h>
#include <sys/farptr.h>
#include <sys/nearptr.h>
#include <go32.h>

/* If defined correct some bogus bios */
#define VBE_BOGUS

/* If defined uses a private stack */
/* #define VBE_STACK */

/* VBE Internal Mode Information */
vbe_internal vbe_state;

/* Video mode list */
#define VBE_MODE_MAX 512
unsigned vbe_mode_map[VBE_MODE_MAX];

/* Return the offset for accessing in writing the video memory */
unsigned char* (*vbe_write_line)(unsigned y);

#ifdef VBE_STACK

static unsigned short vbe_stack_ss;
static unsigned short vbe_stack_sp;
static int vbe_stack_selector;

static void vbe_int(unsigned num, __dpmi_regs* r)
{
	assert(vbe_stack_ss);

	__dpmi_int_ss = vbe_stack_ss;
	__dpmi_int_sp = vbe_stack_sp;
	__dpmi_int_flags = 0;

	__dpmi_int(num, r);
}

#else

static void vbe_int(unsigned num, __dpmi_regs* r)
{
	__dpmi_int(num, r);
}

#endif

/***************************************************************************/
/* Scanline */

static unsigned vbe_adjust_scanline_size(unsigned size)
{
	assert(vbe_mode_is_active());
#ifdef VBE_BOGUS
	/* align at 16 bytes */
	return (size + 0xF) & ~0xF;
#else
	/* align at 4 bytes */
	return (size + 3) & ~0x3;
#endif
}

/* Get scanlines information
 * out:
 *   *byte_length length in byte
 *   *pixel_length length in pixel
 *   *scanlines number of scanlines
 */
static adv_error vbe_scanline_get(unsigned* byte_length, unsigned* pixel_length, unsigned* scanlines)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_mode_is_active());

	r.x.ax = 0x4F06;
	r.x.bx = 0x01;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F06/01 (GetScanlineLength)", (unsigned)r.x.ax);
		return r.x.ax;
	}

	*byte_length = r.x.bx;
	*pixel_length = r.x.cx;
	*scanlines = r.x.dx;

	return 0;
}

/* Set the length of the scanline in byte
 * return:
 *   0 on successfull
 * note:
 *   Update correctly      vbe_state
 *   Length is incremented to an acceptable value
 */
adv_error vbe_scanline_set(unsigned byte_length)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_mode_is_active());

	/* adjust */
	byte_length = vbe_adjust_scanline_size(byte_length);
	if (!byte_length) {
		error_set("Invalid byte length for vbe_scanline_set");
		return -1;
	}

	if (vbe_state.info.VESAVersion < 0x200) {
		r.x.ax = 0x4F06;
		r.x.bx = 0x00;
		r.x.cx = byte_length / vbe_state.bytes_per_pixel; /* in pixel unit */
	} else {
		r.x.ax = 0x4F06;
		r.x.bx = 0x02;
		r.x.cx = byte_length; /* in byte unit */
	}

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F06/02 (SetScanlineLength)", (unsigned)r.x.ax);
		return r.x.ax;
	}

	/* update vbe_state */
	vbe_state.bytes_per_scanline = r.x.bx;
	vbe_state.virtual_x = r.x.cx;
	vbe_state.virtual_y = r.x.dx;

	log_std(("vbe: scanline_set, request:%d [bytes], result: %d [bytes], %d [pixel]\n", byte_length, vbe_state.bytes_per_scanline, vbe_state.virtual_x));

	return 0;
}

/* Set the length of the scanline in pixel
 * return:
 *   0 on successfull
 * note:
 *   Length is incremented to an acceptable value
 */
adv_error vbe_scanline_pixel_set(unsigned pixel_length)
{
	return vbe_scanline_set(pixel_length * vbe_state.bytes_per_pixel);
}

adv_error vbe_scanline_pixel_request(unsigned pixel_length)
{
	if (pixel_length > vbe_state.virtual_x) {
		if (vbe_scanline_pixel_set(pixel_length) != 0) {
			return -1;
		}
	}

	/* retest */
	if (pixel_length > vbe_state.virtual_x) {
		error_set("Unexpected virtual x size");
		return -1;
	}

	return 0;
}

/***************************************************************************/
/* Linear frame buffer */

static unsigned char* vbe_linear_write_line(unsigned y)
{
	return vbe_state.linear_pointer + y * vbe_state.bytes_per_scanline;
}

/* Initialize the vbe linear framebuffer
 * return:
 *      0 on successfull
 */
static adv_error vbe_linear_init(void)
{
	unsigned long linear;

	assert(vbe_mode_is_active());
	assert(!vbe_linear_is_active());

	if (vbe_state.info.VESAVersion < 0x200) {
		error_set("Linear frame buffer is avaliable only with VBE2 or greather");
		return -1;
	}

	if (map_create_linear_mapping(&linear, vbe_state.mode_info.PhysBasePtr, vbe_state.info.TotalMemory * 0x10000) != 0)
		return -1;

	vbe_state.linear_pointer = (unsigned char*)(__djgpp_conventional_base + linear);

	/* enable write function */
	vbe_write_line = vbe_linear_write_line;

	/* enable */
	vbe_state.linear_active = 1;

	return 0;
}

/* Deinitialize the vbe linear framebuffer */
static void vbe_linear_done(void)
{
	assert(vbe_linear_is_active());

	map_remove_linear_mapping(vbe_state.mode_info.PhysBasePtr, vbe_state.info.TotalMemory * 0x10000);

	/* disable write function */
	vbe_write_line = 0;

	/* disable */
	vbe_state.linear_active = 0;
}

/***************************************************************************/
/* Protect mode */

static inline void vbe_pm_call(uint32 addr, uint32 ebx, uint32 ecx, uint32 edx)
{
	/* From VBE 3.0 Specifications */
	/* Note: Currently undefined registers may be destroyed with the */
	/* exception of ESI, EBP, DS and SS. */
	__asm__ __volatile__ (
		"pushw %%es ;"
		"andl %%eax, %%eax ;"
		"jz 0f ;"
		"movw %%ax, %%es ;"
		"0: ;"
		"call *%%edi ;"
		"popw %%es ;"
		: /* no output */
		:
		"a" (vbe_state.pm_mmio_selector),
		"b" (ebx),
		"c" (ecx),
		"d" (edx),
		"D" (addr)
		:
#ifdef VBE_BOGUS
		"cc", "esi", "ebp"
#else
		"cc"
#endif
	);
}

static inline void vbe_pm_call_ptr(uint32 addr, uint32 ebx, uint32 ecx, uint32 edx, uint32 es, uint32 edi)
{
	__asm__ __volatile__ (
		"pushw %%es ;"
		"pushw %%ds ;"
		"movw %%ax, %%es ;"
		"movl %4, %%eax ;"
		"andl %%eax, %%eax ;"
		"jz 0f ;"
		"movw %%ax, %%ds ;"
		"0: ;"
		"call *%0 ;"
		"popw %%ds ;"
		"popw %%es ;"
		: /* no output */
		:
		"r" (addr),
		"b" (ebx),
		"c" (ecx),
		"d" (edx),
		"g" (vbe_state.pm_mmio_selector),
		"a" (es),
		"D" (edi)
		:
#ifdef VBE_BOGUS
		"cc" /*, "esi", "ebp" */
#else
		"cc"
#endif
	);
}

/* Initialize the protect mode interface
 * return:
 *   ==0 ok
 * note:
 *   Call after the mode set
 */
static adv_error vbe_pm_init(void)
{
	__dpmi_regs r;
	unsigned len;

	memset(&r, 0, sizeof(r));

	assert(!vbe_pm_is_active());
	assert(vbe_mode_is_active());

	vbe_state.pm_switcher = 0;
	vbe_state.pm_scroller = 0;
	vbe_state.pm_palette = 0;

	/* disable memory mapped IO */
	vbe_state.pm_mmio_selector = 0;
	vbe_state.pm_mmio_address = 0;
	vbe_state.pm_mmio_size = 0;

	if (vbe_state.info.VESAVersion < 0x200) {
		error_set("Protect mode interface avaliable only with VBE2 or greather");
		return -1;
	}

	r.x.ax = 0x4F0A;
	r.x.bx = 0;
	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F0A/00 (GetProtectModeInterface)", (unsigned)r.x.ax);
		return r.x.ax;
	}

	len = r.x.cx;
	vbe_state.pm_info = malloc(len);
	assert(vbe_state.pm_info);

	dosmemget(r.x.es*16+r.x.di, len, vbe_state.pm_info);

#ifdef VBE_BOGUS
	/* Now do a sanity check on the information we recieve to ensure */
	/* that is is correct. Some BIOS return totally bogus information */
	/* in here (some Matrox, Trident Blade3D) */
	if (vbe_state.pm_info->setWindow >= len ||
		vbe_state.pm_info->setDisplayStart >= len ||
		vbe_state.pm_info->setPalette >= len ||
		vbe_state.pm_info->IOPrivInfo >= len) {
		if (vbe_state.pm_info->setWindow >= len)
			error_set("Invalid setWindow addr");
		if (vbe_state.pm_info->setDisplayStart >= len)
			error_set("Invalid setDisplayStart addr");
		if (vbe_state.pm_info->setPalette >= len)
			error_set("Invalid setPalette addr");
		if (vbe_state.pm_info->IOPrivInfo >= len)
			error_set("Invalid IOPrivInfo addr");
		error_set("Inconsistent information from the VESA call 4F0A/00 (GetProtectModeInterface)");

		free(vbe_state.pm_info);
		vbe_state.pm_info = 0;
		return -1;
	}
#endif

	/* assume cs==ds */
	vbe_state.pm_switcher = (unsigned)vbe_state.pm_info + vbe_state.pm_info->setWindow;
	vbe_state.pm_scroller = (unsigned)vbe_state.pm_info + vbe_state.pm_info->setDisplayStart;
	vbe_state.pm_palette = (unsigned)vbe_state.pm_info + vbe_state.pm_info->setPalette;

	/* enable */
	vbe_state.pm_active = 1;

	return 0;
}

static void vbe_pm_done(void)
{
	assert(vbe_pm_is_active());

	free(vbe_state.pm_info);

	/* disable */
	vbe_state.pm_active = 0;
}

/***************************************************************************/
/* text */

/* Return the offset for accessing in writing the video memory */
static unsigned char* vbe_text_write_line(unsigned y)
{
	assert(vbe_mode_is_active());
	return (unsigned char*)(0xA0000 + y * vbe_state.bytes_per_scanline + __djgpp_conventional_base);
}

/***************************************************************************/
/* Mode */

void vbe_mode_done(adv_bool restore)
{
	assert(vbe_mode_is_active());

	if (vbe_linear_is_active())
		vbe_linear_done();
	if (vbe_pm_is_active())
		vbe_pm_done();

	if (restore) {
		__dpmi_regs r;

		/* reset the text BIOS mode */
		r.x.ax = 0x0003;
		vbe_int(0x10, &r);
	}

	/* disable */
	vbe_state.mode_active = 0;
}

/* Compute shift and mask value */
static void vbe_rgb_data(int* shift, unsigned* mask, unsigned posbit, unsigned masksize)
{
	*mask = ((1 << masksize) - 1) << posbit;
	*shift = posbit + masksize - 8;
}

adv_color_def vbe_color_def(void)
{
	if (vbe_state.mode_info.MemoryModel == vbeMemPK) {
		return color_def_make(adv_color_type_palette);
	} else {
		return color_def_make_rgb_from_sizeshiftmask(vbe_state.bytes_per_pixel, vbe_state.rgb_red_shift, vbe_state.rgb_red_mask, vbe_state.rgb_green_shift, vbe_state.rgb_green_mask, vbe_state.rgb_blue_shift, vbe_state.rgb_blue_mask);
	}
}

/* Set one graphics mode
 * in:
 *   mode mode number with vbeClear or vbeLinear flags
 *   crtc CRTC information, ==0 for default
 * return:
 *   0 on successfull
 * note:
 *   update correctly      vbe_state
 *   initialize linear frambuffer if requested
 *   if fail, video mode can be bronken
 */
adv_error vbe_mode_set(unsigned mode, const vbe_CRTCInfoBlock* crtc)
{
	vbe_ModeInfoBlock info;
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	log_std(("vbe: mode_set %x\n", mode));

	assert(vbe_is_active());

	/* check support for CRTC */
	if (crtc && vbe_state.info.VESAVersion < 0x300) {
		error_set("CRTC programming is available only with VBE3 or greather");
		return -1;
	}

	/* check support for linear frame buffer */
	if ((mode & vbeLinearBuffer)!=0 && vbe_state.info.VESAVersion < 0x200) {
		error_set("Linear Frame Buffer is available only with VBE2 or greather");
		return -1;
	}

	/* get mode info */
	if (vbe_mode_info_get(&info, mode)!=0) {
		return -1;
	}

	if (vbe_mode_is_active())
		vbe_mode_done(1);

	r.x.ax = 0x4F02;
	if (crtc) {
		dosmemput(crtc, sizeof(vbe_CRTCInfoBlock), __tb);
		r.x.bx = mode | vbeRefreshCtrl;
		r.x.es = __tb / 16;
		r.x.di = 0;
	} else {
		r.x.bx = mode;
	}

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F02/%x (ModeSet)", (unsigned)r.x.ax, (unsigned)mode);
		return r.x.ax;
	}

	/* enable */
	vbe_state.mode_active = 1;

	/* update vbe_state.mode_info */
	vbe_state.mode_info = info;

	/* update vbe_state */
	vbe_state.mode = mode;
	vbe_state.scroll = 0;

	if (vbe_state.mode_info.ModeAttributes & vbeMdGraphMode) {
		/* graphics mode */
		vbe_state.size_x = vbe_state.mode_info.XResolution;
		vbe_state.size_y = vbe_state.mode_info.YResolution;

		vbe_state.bytes_per_pixel = (vbe_state.mode_info.BitsPerPixel + 7) / 8;
		if (vbe_scanline_get(&vbe_state.bytes_per_scanline, &vbe_state.virtual_x, &vbe_state.virtual_y)!=0) {
#ifdef VBE_BOGUS
			/* Retry setting */
			if (vbe_scanline_pixel_set(info.XResolution)!=0) {
				vbe_mode_done(1);
				return -1;
			}
#else
			vbe_mode_done();
			return -1;
#endif
		}

		if (vbe_state.mode_info.MemoryModel==vbeMemRGB || vbe_state.mode_info.MemoryModel==vbeMemYUV) {
			/* update color rgb */
			if (mode & vbeLinearBuffer) {
				vbe_rgb_data(&vbe_state.rgb_red_shift, &vbe_state.rgb_red_mask, vbe_state.mode_info.RedFieldPosition, vbe_state.mode_info.RedMaskSize);
				vbe_rgb_data(&vbe_state.rgb_green_shift, &vbe_state.rgb_green_mask, vbe_state.mode_info.GreenFieldPosition, vbe_state.mode_info.GreenMaskSize);
				vbe_rgb_data(&vbe_state.rgb_blue_shift, &vbe_state.rgb_blue_mask, vbe_state.mode_info.BlueFieldPosition, vbe_state.mode_info.BlueMaskSize);
			} else {
				vbe_rgb_data(&vbe_state.rgb_red_shift, &vbe_state.rgb_red_mask, vbe_state.mode_info.LinRedFieldPosition, vbe_state.mode_info.LinRedMaskSize);
				vbe_rgb_data(&vbe_state.rgb_green_shift, &vbe_state.rgb_green_mask, vbe_state.mode_info.LinGreenFieldPosition, vbe_state.mode_info.LinGreenMaskSize);
				vbe_rgb_data(&vbe_state.rgb_blue_shift, &vbe_state.rgb_blue_mask, vbe_state.mode_info.LinBlueFieldPosition, vbe_state.mode_info.LinBlueMaskSize);
			}
		} else {
			/* update color palette */
			vbe_state.palette_width = 6; /* VBE default */
		}

		/* initialize pm interface */
		if (vbe_state.info.VESAVersion >= 0x200) {
			if (vbe_pm_init() != 0) {
				/* ignore error, don't use pm interface */
			}
		}

		if (mode & vbeLinearBuffer) {
			/* initialize linear framebuffer */
			if (vbe_linear_init() != 0) {
				vbe_mode_done(1);
				return -1;
			}
		} else {
                       return -1;
		}
	} else {
		/* text mode */
		vbe_state.size_x = vbe_state.mode_info.XResolution * vbe_state.mode_info.XCharSize;
		vbe_state.size_y = vbe_state.mode_info.YResolution * vbe_state.mode_info.YCharSize;
		vbe_state.virtual_x = vbe_state.size_x;
		vbe_state.virtual_y = vbe_state.virtual_y;

		vbe_state.bytes_per_pixel = 0;
		vbe_state.bytes_per_scanline = vbe_state.mode_info.XResolution * 2;
		vbe_state.palette_width = 0;
		vbe_write_line = vbe_text_write_line;
	}

	return 0;
}

/* Get the current graphics mode
 */
adv_error vbe_mode_get(unsigned* mode)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active());

	r.x.ax = 0x4F03;
	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F03 (ModeGet)", (unsigned)r.x.ax);
		return r.x.ax;
	}

	*mode = r.x.bx & 0x7FFF; /* clear unused bits */

	return 0;
}

/***************************************************************************/
/* VBE */

static void dosstrzcpy(char* dst, unsigned segoff, unsigned len)
{
	unsigned ptr = ((segoff & 0xFFFF0000) >> 12) + (segoff & 0xFFFF);

	if (!segoff) {
		*dst = 0;
		return;
	}

	while (len > 1 && _farpeekb(_dos_ds, ptr) != 0x00) {
		*dst = _farpeekb(_dos_ds, ptr);
		++dst;
		--len;
		++ptr;
	}
	*dst = 0;
}

static void dosmodezcpy(unsigned* dst, unsigned segoff, unsigned len)
{
	unsigned ptr = ((segoff & 0xFFFF0000) >> 12) + (segoff & 0xFFFF);

	if (!segoff) {
		*dst = 0xFFFF;
		return;
	}

	while (len > 1 && _farpeekw(_dos_ds, ptr) != 0xFFFF) {
		*dst = _farpeekw(_dos_ds, ptr);
		++dst;
		--len;
		ptr += 2;
	}
	*dst = 0xFFFF;
}

/* Initialize the vbe system
 * return:
 *      0 on successfull
 */
adv_error vbe_init(void)
{
	__dpmi_regs r;
	char oem_string[256];
	char oem_vendor[256];
	char oem_product[256];
	char oem_product_rev[256];
	unsigned i;

	memset(&r, 0, sizeof(r));

#ifdef VBE_STACK
	{
		int ret_selector_or_max;
		unsigned size = 8192;
		int ret;

		ret = __dpmi_allocate_dos_memory(size / 16, &ret_selector_or_max);
		if (ret == -1) {
			log_std(("vbe: error allocationg the VBE stack\n"));
			error_set("Error allocating the VBE stack");
			return -1;
		}

		vbe_stack_ss = ret;
		vbe_stack_sp = size - 18;
		vbe_stack_selector = ret_selector_or_max;
	}
#endif

	assert(!vbe_is_active());

	/* initialize with all zero */
	memset(&vbe_state.info, 0, sizeof(vbe_VbeInfoBlock));

	/* request VBE 2.0+ */
	memcpy(&vbe_state.info.VESASignature, "VBE2", 4);

	dosmemput(&vbe_state.info, sizeof(vbe_VbeInfoBlock), __tb);

	r.x.ax = 0x4F00;
	r.x.es = __tb / 16;
	r.x.di = 0;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		unsigned char sign[4];
		dosmemget(__tb, 4, &sign);
		log_std(("vbe: not found, status %04x, sign %02x%02x%02x%02x \n", (unsigned)r.x.ax, (unsigned)sign[0], (unsigned)sign[1], (unsigned)sign[2], (unsigned)sign[3]));
		error_set("Error %x in the VESA call 4F00 (GetVesaInfo)", (unsigned)r.x.ax);
		return -1;
	}

	dosmemget(__tb, sizeof(vbe_VbeInfoBlock), &vbe_state.info);

	/* reject too old bios */
	if (vbe_state.info.VESAVersion < 0x102) {
		log_std(("vbe: version lower than 1.2\n"));
		error_set("The VESA BIOS is of a version prior 1.2");
		return -1;
	}

#ifdef VBE_BOGUS
	/* check for bogus BIOSes */
	if (vbe_state.info.VESAVersion >= 0x200 && vbe_state.info.OemVendorNamePtr == 0)
		vbe_state.info.VESAVersion = 0x102;
#endif

	/* bufferize all the dynamic info */
	dosmodezcpy(vbe_mode_map, vbe_state.info.VideoModePtr, VBE_MODE_MAX);
	dosstrzcpy(oem_string, vbe_state.info.OemStringPtr, sizeof(oem_string));
	if (vbe_state.info.VESAVersion >= 0x200) {
		dosstrzcpy(oem_vendor, vbe_state.info.OemVendorNamePtr, sizeof(oem_vendor));
		dosstrzcpy(oem_product, vbe_state.info.OemProductNamePtr, sizeof(oem_product));
		dosstrzcpy(oem_product_rev, vbe_state.info.OemProductRevPtr, sizeof(oem_product_rev));
	} else {
		sncpy(oem_vendor, sizeof(oem_vendor), "");
		sncpy(oem_product, sizeof(oem_product), "");
		sncpy(oem_product_rev, sizeof(oem_product_rev), "");
	}

	log_std(("vbe: version %d.%d\n", (unsigned)vbe_state.info.VESAVersion >> 8, (unsigned)vbe_state.info.VESAVersion & 0xFF));
	log_std(("vbe: memory %d\n", (unsigned)vbe_state.info.TotalMemory * 0x10000));
	if (vbe_state.info.Capabilities & vbeNonVGA)
		log_std(("vbe: controller is NOT VGA compatible\n"));
	else
		log_std(("vbe: controller is VGA compatible\n"));

	log_std(("vbe: oem %s\n", oem_string));

	if (vbe_state.info.VESAVersion >= 0x200) {
		log_std(("vbe: oem software rev %d.%d\n", vbe_state.info.OemSoftwareRev >> 8, vbe_state.info.OemSoftwareRev & 0xFF));
		log_std(("vbe: oem vendor name ptr %s\n", oem_vendor));
		log_std(("vbe: oem product name ptr %s\n", oem_product));
		log_std(("vbe: oem product rev ptr %s\n", oem_product_rev));
	}

	/* enable */
	vbe_state.active = 1;

	/* print modes */
	for(i=0;vbe_mode_map[i]!=0xFFFF;++i) {
		vbe_ModeInfoBlock info;
		if (vbe_mode_info_get(&info, vbe_mode_map[i])!=0) {
			log_std(("vbe: error getting info for mode %x\n", (unsigned)vbe_mode_map[i]));
		} else {
			if (!(info.ModeAttributes & vbeMdGraphMode)) {
				log_std(("vbe: mode %4xh %4dx%4d text\n", vbe_mode_map[i], info.XResolution, info.YResolution));
			} else if (info.MemoryModel != vbeMemPK && info.MemoryModel != vbeMemRGB) {
				log_std(("vbe: mode %4xh %4dx%4d %2d not packed/rgb\n", vbe_mode_map[i], info.XResolution, info.YResolution, info.BitsPerPixel));
			} else if (info.NumberOfPlanes > 1) {
				log_std(("vbe: mode %4xh %4dx%4d %2d planar\n", vbe_mode_map[i], info.XResolution, info.YResolution, info.BitsPerPixel));
			} else {
				const char* flag_doublescan = info.ModeAttributes & vbeMdDoubleScan ? " doublescan": "";
				const char* flag_interlace = info.ModeAttributes & vbeMdInterlace ? " interlace": "";
				const char* flag_triple = info.ModeAttributes & vbeMdTripleBuffer ? " triple": "";
				const char* flag_linear = info.ModeAttributes & vbeMdLinear ? " linear": "";
				const char* flag_banked = !(info.ModeAttributes & vbeMdNonBanked) ? " banked": "";
				const char* flag_vga = !(info.ModeAttributes & vbeMdNonVGA) ? " vga": "";
				log_std(("vbe: mode %4xh %4dx%4d %2d%s%s%s%s%s%s \n", vbe_mode_map[i], info.XResolution, info.YResolution, info.BitsPerPixel, flag_doublescan, flag_interlace, flag_triple, flag_linear, flag_banked, flag_vga));
			}
		}
	}

	return 0;
}

/* Deinitialize the vbe system */
void vbe_done(void)
{
	assert(vbe_is_active());

	if (vbe_mode_is_active()) {
		vbe_mode_done(1);
	}

	assert(!vbe_mode_is_active());
	assert(!vbe_linear_is_active());
	assert(!vbe_pm_is_active());

	/* disable */
	vbe_state.active = 0;
}

unsigned vbe_adjust_bytes_per_page(unsigned bytes_per_page)
{
	return bytes_per_page = (bytes_per_page + 0xFFFF) & ~0xFFFF;
}

/* Get information for a vbe mode
 * return:
 *   0 on successfull
 * note:
 *   VBE 3.0 information are always filled, also if VBE is less than 3.0
 */
adv_error vbe_mode_info_get(vbe_ModeInfoBlock* info, unsigned mode)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active());

	memset(info, 0, sizeof(vbe_ModeInfoBlock));

	dosmemput(info, sizeof(vbe_ModeInfoBlock), __tb);

	r.x.ax = 0x4F01;
	r.x.cx = mode & vbeModeMask;
	r.x.es = __tb / 16;
	r.x.di = 0;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F01/%x (GetModeInfo)", (unsigned)r.x.ax, (unsigned)mode);
		return -1;
	}

	dosmemget(__tb, sizeof(vbe_ModeInfoBlock), info);

#ifdef VBE_BOGUS
	/* support old VESA implementation */
	if (vbe_state.info.VESAVersion < 0x200) {
		
		/* convert strange mode */
		if (info->MemoryModel == vbeMemPK && info->BitsPerPixel > 8) {
			switch (info->BitsPerPixel) {
				case 15:
					info->MemoryModel = vbeMemRGB;
					info->RedMaskSize = 5;
					info->RedFieldPosition = 10;
					info->GreenMaskSize = 5;
					info->GreenFieldPosition = 5;
					info->BlueMaskSize = 5;
					info->BlueFieldPosition = 0;
					info->RsvdMaskSize = 1;
					info->RsvdFieldPosition = 15;
					break;
				case 16:
					info->MemoryModel = vbeMemRGB;
					info->RedMaskSize = 5;
					info->RedFieldPosition = 11;
					info->GreenMaskSize = 5;
					info->GreenFieldPosition = 5;
					info->BlueMaskSize = 5;
					info->BlueFieldPosition = 0;
					info->RsvdMaskSize = 0;
					info->RsvdFieldPosition = 0;
				break;
				case 24:
					info->MemoryModel = vbeMemRGB;
					info->RedMaskSize = 8;
					info->RedFieldPosition = 16;
					info->GreenMaskSize = 8;
					info->GreenFieldPosition = 8;
					info->BlueMaskSize = 8;
					info->BlueFieldPosition = 0;
					info->RsvdMaskSize = 0;
					info->RsvdFieldPosition = 0;
					break;
			}
		}

		/* detect 15 bits info */
		if (info->BitsPerPixel==16 && info->RsvdMaskSize == 1)
			info->BitsPerPixel = 15;
	}
#endif

	/* extend information to VBE 3.0 format */
	if (vbe_state.info.VESAVersion < 0x300) {
		info->LinBytesPerScanLine = info->BytesPerScanLine;
		info->BnkNumberOfPages = info->NumberOfImagePages;
		info->LinNumberOfPages = info->NumberOfImagePages;
		info->LinRedMaskSize = info->RedMaskSize;
		info->LinRedFieldPosition = info->RedFieldPosition;
		info->LinGreenMaskSize = info->GreenMaskSize;
		info->LinGreenFieldPosition = info->GreenFieldPosition;
		info->LinBlueMaskSize = info->BlueMaskSize;
		info->LinBlueFieldPosition = info->BlueFieldPosition;
		info->LinRsvdMaskSize = info->RsvdMaskSize;
		info->LinRsvdFieldPosition = info->RsvdFieldPosition;
		info->MaxPixelClock = 0;
	}

	return 0;
}

adv_error vga_as_vbe_mode_info_get(vbe_ModeInfoBlock* info, unsigned mode)
{
	memset(info, 0, sizeof(vbe_ModeInfoBlock));
	if (mode == 0x13) {
		info->ModeAttributes = vbeMdAvailable | vbeMdTTYOutput | vbeMdColorMode | vbeMdGraphMode | vbeMdDoubleScan;
		info->BytesPerScanLine = 320;
		info->XResolution = 320;
		info->YResolution = 200;
		info->NumberOfPlanes = 1;
		info->BitsPerPixel = 8;
		info->MemoryModel = vbeMemPK;
	} else if (mode == 0x3) {
		info->ModeAttributes = vbeMdAvailable | vbeMdTTYOutput | vbeMdColorMode | vbeMdDoubleScan;
		info->BytesPerScanLine = 160;
		info->XResolution = 80;
		info->YResolution = 25;
		info->XCharSize = 9;
		info->YCharSize = 16;
		info->NumberOfPlanes = 1;
		info->BitsPerPixel = 4;
		info->MemoryModel = vbeMemTXT;
	} else {
      		error_set("VBE emulation only for mode VGA 13h, 03h");
		return -1;
	}

	return 0;
}


adv_error vbe_scroll(unsigned offset, adv_bool waitvsync)
{
	assert(vbe_is_active() && vbe_mode_is_active());

#ifndef VBE_BOGUS
	/* On my clean hardware always wait a vsync, but with software
	driver work ok */
	if (vbe_state.pm_active && vbe_state.pm_scroller) {
		unsigned bx, cx, dx;
		bx = waitvsync ? 0x80 : 0x0;
		/* assume no planar mode (don't divide offset by 4) */
		cx = (offset >> 2) & 0xFFFF;
		dx = offset >> 18;
		/* use lower 2 bit if supported */
		if (vbe_state.info.VESAVersion >= 0x300) {
			dx |= (offset & 0x3) << 14;
		}
		vbe_pm_call(vbe_state.pm_scroller, bx, cx, dx);
	} else
#endif
	{
		__dpmi_regs r;

		memset(&r, 0, sizeof(r));

		r.x.ax = 0x4F07;
		if (vbe_state.info.VESAVersion >= 0x300) {
			r.x.bx = waitvsync ? 0x82 : 0x2;
			r.d.ecx = offset;
		} else {
			r.x.bx = waitvsync ? 0x80 : 0x0;
			r.x.cx = (offset % vbe_state.bytes_per_scanline) / vbe_state.bytes_per_pixel;
			r.x.dx = offset / vbe_state.bytes_per_scanline;
		}
		vbe_int(0x10, &r);
		if (r.x.ax!=0x004F) {
			error_set("Error %x in the VESA call 4F07 (ScrollSet)", (unsigned)r.x.ax);
			return -1;
		}
	}

	vbe_state.scroll = offset;

	return 0;
}

adv_error vbe_request_scroll(unsigned offset)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active() && vbe_mode_is_active());

	r.x.ax = 0x4F07;
	r.x.bx = 0x02;
	r.d.ecx = offset;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F07/02 (ScrollRequest)", (unsigned)r.x.ax);
		return -1;
	}

	vbe_state.scroll = offset;

	return 0;
}

/*
 * note:
 *   This function is not supported by protected mode interface
 */
adv_error vbe_poll_scroll(unsigned* done)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active() && vbe_mode_is_active());

	r.x.ax = 0x4F07;
	r.x.bx = 0x04;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F07/04 (ScrollPoll)", (unsigned)r.x.ax);
		return -1;
	}
	
	*done = r.h.bl;

	return 0;
}

/* Wait a vsync */
void vbe_wait_vsync(void)
{
	assert(vbe_is_active() && vbe_mode_is_active());

	/* if possible use vga version */
	if ((vbe_state.info.Capabilities & vbeNonVGA)==0) {
		vga_wait_vsync();
	} else {
		vbe_scroll(vbe_state.scroll, 1);
	}
}

/* Value used for incrementing/decrementin pixelclock */
#define VBE_PIXELCLOCK_DELTA (10*1000)
#define VBE_PIXELCLOCK_MAX (250*1000*1000)
#define VBE_PIXELCLOCK_MIN (1*1000*1000)

/* Return the next avaliable pixel clock */
adv_error vbe_pixelclock_getnext(unsigned* pixelclock, unsigned mode)
{
	unsigned newpixelclock = *pixelclock + VBE_PIXELCLOCK_DELTA;
	while (newpixelclock < VBE_PIXELCLOCK_MAX) {
		unsigned testpixelclock = newpixelclock;
		if (vbe_pixelclock_get(&testpixelclock, mode)!=0) {
			return -1;
		}
		if (testpixelclock > *pixelclock) {
			*pixelclock = testpixelclock;
			return 0;
		}
		newpixelclock += VBE_PIXELCLOCK_DELTA;
	}
	return -1;
}

/* Return the pred avaliable pixel clock */
adv_error vbe_pixelclock_getpred(unsigned* pixelclock, unsigned mode)
{
	unsigned newpixelclock = *pixelclock - VBE_PIXELCLOCK_DELTA;
	while (newpixelclock > VBE_PIXELCLOCK_MIN) {
		unsigned testpixelclock = newpixelclock;
		if (vbe_pixelclock_get(&testpixelclock, mode)!=0) {
			return -1;
		}
		if (testpixelclock < *pixelclock) {
			*pixelclock = testpixelclock;
			return 0;
		}
		newpixelclock -= VBE_PIXELCLOCK_DELTA;
	}
	return -1;
}

/* Get a valid pixel clock
 * in:
 *   *pixelclock to test in Hz
 *   mode mode number (with vbeLinearBuffer if used for)
 * out:
 *   *pixelclock nearest pixelclock in Hz
 */
adv_error vbe_pixelclock_get(unsigned* pixelclock, unsigned mode)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active());

	r.x.ax = 0x4F0B;
	r.x.bx = 0x00;
	r.d.ecx = *pixelclock;
	r.x.dx = mode & vbeModeMask;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F0B/00 (PixelClockGet)", (unsigned)r.x.ax);
		return -1;
	}

	*pixelclock = r.d.ecx;

	return 0;
}

/* Set palette
 * in:
 *   palette palette to set in DAC format
 * note:
 *   Prefferred are not real-protected switch required method
 */
adv_error vbe_palette_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync)
{
	unsigned mode = waitvsync ? 0x80 : 0x0;

	assert(vbe_is_active() && vbe_mode_is_active());

	if (vbe_state.pm_active && vbe_state.pm_palette) {
		log_debug(("vbe: palette set (%d bit) with VBE pm call\n", (unsigned)vbe_state.palette_width));
		vbe_pm_call_ptr(vbe_state.pm_palette, mode, count, start, _my_ds(), (unsigned)palette);
	} else if ((vbe_state.info.Capabilities & vbeNonVGA)==0 && vbe_state.palette_width==6) {
		log_debug(("vbe: palette set (6 bit) with VGA registers\n"));
		vga_palette6_set(palette, start, count, waitvsync);
	} else {
		__dpmi_regs r;

		memset(&r, 0, sizeof(r));

		log_debug(("vbe: palette set (%d bit) with VBE bios\n", (unsigned)vbe_state.palette_width));

		dosmemput(palette, count * sizeof(adv_color_rgb), __tb);

		r.x.ax = 0x4F07;
		r.x.bx = mode;
		r.x.cx = count;
		r.x.dx = start;
		r.x.es = __tb / 16;
		r.x.di = 0;

		vbe_int(0x10, &r);
		if (r.x.ax!=0x004F) {
			error_set("Error %x in the VESA call 4F07 (PaletteSet)", (unsigned)r.x.ax);
			return -1;
		}
	}

	return 0;
}

void vbe_mode_iterator_begin(vbe_mode_iterator* vmi)
{
	assert(vbe_is_active());
	vmi->mode_ptr = 0;
}

unsigned vbe_mode_iterator_get(vbe_mode_iterator* vmi)
{
	return vbe_mode_map[vmi->mode_ptr];
}

adv_bool vbe_mode_iterator_end(vbe_mode_iterator* vmi)
{
	return vbe_mode_map[vmi->mode_ptr] == 0xFFFF;
}

void vbe_mode_iterator_next(vbe_mode_iterator* vmi)
{
	vmi->mode_ptr += 1;
}

adv_error vbe_save_state(unsigned* psize, void** pptr)
{
	__dpmi_regs r;
	unsigned size;
	void* ptr;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active());

	r.x.ax = 0x4F04;
	r.x.cx = 0xF;
	r.h.dl = 0x00;
	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F04/00 (SaveRestoreState)", (unsigned)r.x.ax);
		return -1;
	}

	size = r.x.bx * 64;
	if (size > 4096)
		log_std(("vbe: WARNING! save state size %d, probably to big for the DJGPP real mode buffer\n", size));

	r.x.ax = 0x4F04;
	r.x.cx = 0xF;
	r.h.dl = 0x01;
	r.x.es = __tb / 16;
	r.x.bx = 0;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F04/01 (SaveRestoreState)", (unsigned)r.x.ax);
		return -1;
	}

	ptr = (void*)malloc(size);
	dosmemget(__tb, size, ptr);

	*psize = size;
	*pptr = ptr;
	
	return 0;
}

adv_error vbe_restore_state(unsigned size, void* ptr)
{
	__dpmi_regs r;

	memset(&r, 0, sizeof(r));

	assert(vbe_is_active());

	dosmemput(ptr, size, __tb);

	r.x.ax = 0x4F04;
	r.x.cx = 0xF;
	r.h.dl = 0x02;
	r.x.es = __tb / 16;
	r.x.bx = 0;

	vbe_int(0x10, &r);
	if (r.x.ax!=0x004F) {
		error_set("Error %x in the VESA call 4F04/02 (SaveRestoreState)", (unsigned)r.x.ax);
		return -1;
	}

	return 0;
}

