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

#ifndef __SCRVBE_H
#define __SCRVBE_H

#include "scrvga.h"
#include "rgb.h"

#include <assert.h>

typedef struct vbe_VbeInfoBlock_struct {
	uint8 VESASignature[4] __attribute__ ((packed)); /* 'VESA' 4 byte signature */
	uint16 VESAVersion __attribute__ ((packed)); /* VBE version number */
	uint32 OemStringPtr __attribute__ ((packed)); /* Pointer to OEM string */
	uint32 Capabilities __attribute__ ((packed)); /* Capabilities of video card */
	uint32 VideoModePtr __attribute__ ((packed)); /* Pointer to supported modes */
	uint16 TotalMemory __attribute__ ((packed));  /* Number of 64kb memory blocks */

	/* VBE 2.0 extension information */
	uint16 OemSoftwareRev __attribute__ ((packed)); /* OEM Software revision number */
	uint32 OemVendorNamePtr __attribute__ ((packed)); /* Pointer to Vendor Name string */
	uint32 OemProductNamePtr __attribute__ ((packed)); /* Pointer to Product Name string */
	uint32 OemProductRevPtr __attribute__ ((packed)); /* Pointer to Product Revision str */
	uint8 Reserved[222] __attribute__ ((packed)); /* Pad to 256 byte block size */
	uint8 OemDATA[256]  __attribute__ ((packed)); /* Scratch pad for OEM data */
} vbe_VbeInfoBlock __attribute__ ((packed));

/* Flags for VbeInfoBlock.Capabilities */
#define vbe8BitDAC 0x0001 /* DAC width is switchable to 8 bit */
#define vbeNonVGA 0x0002 /* Controller is non-VGA */
#define vbeBlankRAMDAC 0x0004 /* Programmed DAC with blank bit */
#define vbeHWStereoSync 0x0008 /* Hardware stereo signalling */
#define vbeEVCStereoSync 0x0010 /* HW stereo sync via EVC connector */

/* Mode information block */
typedef struct vbe_ModeInfoBlock_struct {
	uint16 ModeAttributes __attribute__ ((packed)); /* Mode attributes */
	uint8 WinAAttributes __attribute__ ((packed)); /* Window A attributes */
	uint8 WinBAttributes __attribute__ ((packed)); /* Window B attributes */
	uint16 WinGranularity __attribute__ ((packed)); /* Window granularity in k */
	uint16 WinSize __attribute__ ((packed)); /* Window size in k */
	uint16 WinASegment __attribute__ ((packed)); /* Window A segment */
	uint16 WinBSegment __attribute__ ((packed)); /* Window B segment */
	uint32 WinFuncPtr __attribute__ ((packed)); /* Pointer to window function */
	uint16 BytesPerScanLine __attribute__ ((packed)); /* Bytes per scanline */
	uint16 XResolution __attribute__ ((packed)); /* Horizontal resolution */
	uint16 YResolution __attribute__ ((packed)); /* Vertical resolution */
	uint8 XCharSize __attribute__ ((packed)); /* Character cell width */
	uint8 YCharSize __attribute__ ((packed)); /* Character cell height */
	uint8 NumberOfPlanes __attribute__ ((packed)); /* Number of memory planes */
	uint8 BitsPerPixel __attribute__ ((packed)); /* Bits per pixel */
	uint8 NumberOfBanks __attribute__ ((packed)); /* Number of CGA style banks */
	uint8 MemoryModel __attribute__ ((packed)); /* Memory model type */
	uint8 BankSize __attribute__ ((packed)); /* Size of CGA style banks */
	uint8 NumberOfImagePages __attribute__ ((packed)); /* Number of images pages */
	uint8 ReservedPage __attribute__ ((packed)); /* Reserved */
	uint8 RedMaskSize __attribute__ ((packed)); /* Size of direct color red mask */
	uint8 RedFieldPosition __attribute__ ((packed)); /* Bit posn of lsb of red mask */
	uint8 GreenMaskSize __attribute__ ((packed)); /* Size of direct color green mask */
	uint8 GreenFieldPosition __attribute__ ((packed)); /* Bit posn of lsb of green mask */
	uint8 BlueMaskSize __attribute__ ((packed)); /* Size of direct color blue mask */
	uint8 BlueFieldPosition __attribute__ ((packed)); /* Bit posn of lsb of blue mask */
	uint8 RsvdMaskSize __attribute__ ((packed)); /* Size of direct color res mask */
	uint8 RsvdFieldPosition __attribute__ ((packed)); /* Bit posn of lsb of res mask */
	uint8 DirectColorModeInfo __attribute__ ((packed)); /* Direct color mode attributes */

	/* VBE 2.0 extensions information */
	uint32 PhysBasePtr __attribute__ ((packed)); /* Physical address for linear buf */
	uint32 OffScreenMemOffset __attribute__ ((packed)); /* Pointer to start of offscreen mem */
	uint16 OffScreenMemSize __attribute__ ((packed)); /* Amount of offscreen mem in 1K's */

	/* VBE 3.0 extensions */
	uint16 LinBytesPerScanLine __attribute__ ((packed)); /* Bytes per scanline */
	uint8 BnkNumberOfPages __attribute__ ((packed)); /* Number of images pages (banked) */
	uint8 LinNumberOfPages __attribute__ ((packed)); /* Number of images pages (linear) */
	uint8 LinRedMaskSize __attribute__ ((packed)); /* Size of direct color red mask */
	uint8 LinRedFieldPosition __attribute__ ((packed)); /* Bit posn of lsb of red mask */
	uint8 LinGreenMaskSize __attribute__ ((packed)); /* Size of direct color green mask */
	uint8 LinGreenFieldPosition __attribute__ ((packed)); /* Bit posn of lsb of green mask */
	uint8 LinBlueMaskSize __attribute__ ((packed)); /* Size of direct color blue mask */
	uint8 LinBlueFieldPosition __attribute__ ((packed));    /* Bit posn of lsb of blue mask */
	uint8 LinRsvdMaskSize __attribute__ ((packed)); /* Size of direct color res mask */
	uint8 LinRsvdFieldPosition __attribute__ ((packed));    /* Bit posn of lsb of res mask */
	uint32 MaxPixelClock __attribute__ ((packed)); /* Maximum pixel clock */

	uint8 Reserved[190] __attribute__ ((packed)); /* Pad to 256 byte block size */
} vbe_ModeInfoBlock;

/* Values for ModeInfoBlock.MemoryModel */
#define vbeMemTXT 0 /* Text mode memory model */
#define vbeMemCGA 1 /* CGA style mode */
#define vbeMemHGC 2 /* Hercules graphics style mode */
#define vbeMemPL 3 /* 16 color VGA style planar mode */
#define vbeMemPK 4 /* Packed pixel mode */
#define vbeMemX 5 /* Non-chain 4, 256 color (ModeX) */
#define vbeMemRGB 6 /* Direct color RGB mode */
#define vbeMemYUV 7 /* Direct color YUV mode */

/* Flags for combining with video modes during mode set */
#define vbeDontClear 0x8000 /* Dont clear display memory */
#define vbeLinearBuffer 0x4000 /* Enable linear framebuffer mode */
#define vbeRefreshCtrl 0x0800 /* Use refresh rate control */
#define vbeModeMask 0x01FF /* Mask for VBE mode numbers */

/* Flags for the mode attributes returned by VBE_getModeInfo. If */
/* vbeMdNonBanked is set to 1 and vbeMdLinear is also set to 1, then only */
/* the linear framebuffer mode is available. */
#define vbeMdAvailable 0x0001 /* Video mode is available */
#define vbeMdTTYOutput 0x0004 /* TTY BIOS output is supported */
#define vbeMdColorMode 0x0008 /* Mode is a color video mode */
#define vbeMdGraphMode 0x0010 /* Mode is a graphics mode */
#define vbeMdNonVGA 0x0020 /* Mode is not VGA compatible */
#define vbeMdNonBanked 0x0040 /* Banked mode is not supported */
#define vbeMdLinear 0x0080 /* Linear mode supported */
#define vbeMdDoubleScan 0x0100 /* Double scan mode supported */
#define vbeMdInterlace 0x0200 /* Iterlaced mode supported */
#define vbeMdTripleBuffer 0x0400 /* Hardware triple buffering support */
#define vbeMdStereoscopic 0x0800 /* Hardware stereoscopic display support */

/* Flags for the DirecColorInfo returned by VBE_getModeInfo. */
#define vbeDcRampProgrammable 0x0001
#define vbeDcRsvdUsable 0x0002

/* Flags for save/restore state calls */
#define vbeStHardware 0x0001 /* Save the hardware state */
#define vbeStBIOS 0x0002 /* Save the BIOS state */
#define vbeStDAC 0x0004 /* Save the DAC state */
#define vbeStSVGA 0x0008 /* Save the SuperVGA state */
#define vbeStAll 0x000F /* Save all states */

/* VBE 2.0 Protected Mode Interface, optional in VBE 3.0 */
typedef struct vbe_PMInterface_struct
{
	uint16 setWindow __attribute__ ((packed)); /* Offset of Set Window call */
	uint16 setDisplayStart __attribute__ ((packed)); /* Offset of Set Display Start call */
	uint16 setPalette __attribute__ ((packed)); /* Offset of Set Primary Palette */
	uint16 IOPrivInfo __attribute__ ((packed)); /* Offset of I/O priveledge info */
} vbe_PMInterface;

/* VBE 3.0 Protect Mode Info Block */
typedef struct vbe_PMInfoBlock_struc {
	uint8 Signature __attribute__ ((packed)); /* 'PMID' PM Info Block Signature */
	uint16 EntryPoint __attribute__ ((packed)); /* Offset of PM entry point within BIOS */
	uint16 PMInitialize __attribute__ ((packed)); /* Offset of PM initialization entry point */
	uint16 BIOSDataSel __attribute__ ((packed)); /* dw Selector to BIOS data area emulation block */
	uint16 A0000Sel __attribute__ ((packed)); /* Selector to access A0000h physical mem */
	uint16 B0000Sel __attribute__ ((packed)); /* Selector to access B0000h physical mem */
	uint16 B8000Sel __attribute__ ((packed)); /* Selector to access B8000h physical mem */
	uint16 CodeSegSel __attribute__ ((packed)); /* Selector to access code segment as data */
	uint8 InProtectMode __attribute__ ((packed)); /*  Set to 1 when in protected mode */
	uint8 Checksum __attribute__ ((packed)); /* Checksum byte for structure */
} vbe_PMInfoBlock;

/* VBE 3.0 CRTC Info Block */
typedef struct vbe_CRTCInfoBlock_struct {
	uint16 HorizontalTotal __attribute__ ((packed)); /* Horizontal total in pixels */
	uint16 HorizontalSyncStart __attribute__ ((packed)); /* Horizontal sync start in pixels */
	uint16 HorizontalSyncEnd __attribute__ ((packed)); /* Horizontal sync end in pixels */
	uint16 VerticalTotal __attribute__ ((packed)); /* Vertical total in lines */
	uint16 VerticalSyncStart __attribute__ ((packed)); /* Vertical sync start in lines */
	uint16 VerticalSyncEnd __attribute__ ((packed)); /* Vertical sync end in lines */
	uint8 Flags __attribute__ ((packed)); /* Flags (Interlaced, Double Scan etc) */
	uint32 PixelClock __attribute__ ((packed)); /* Pixel clock in units of Hz */
	uint16 RefreshRate __attribute__ ((packed)); /* Refresh rate in units of 0.01 Hz */
	uint8 Reserved[40] __attribute__ ((packed)); /* Remainder of ModeInfoBlock */
} vbe_CRTCInfoBlock;

/* Flags for CRTC Flags */
#define vbeCRTCDoubleScan 0x01 /* Enable double scanned mode */
#define vbeCRTCInterlaced 0x02 /* Enable interlaced mode */
#define vbeCRTCHorizontalSyncNegative 0x04 /* Horizontal sync is negative */
#define vbeCRTCVerticalSyncNegative 0x08 /* Vertical sync is negative */

/***************************************************************************/
/* Private, please don't use directly */

/* VBE Internal Mode Information */
typedef struct vbe_internal_struct {
	adv_bool active; /* !=0 if vbe present */
	adv_bool mode_active; /* !=0 if mode set */
	unsigned mode; /* Actual mode */

	/* Scroll position offset in bytes */
	unsigned scroll;

	/* Size */
	unsigned size_x;
	unsigned size_y;
	unsigned virtual_x;
	unsigned virtual_y;
	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;

	/* Linear frame buffer */
	adv_bool linear_active; /* !=0 if linear frame buffer active */
	unsigned char* linear_pointer; /* ptr at the linear frame buffer */

	/* Protect Mode Interface VBE 2.0 */
	adv_bool pm_active; /* !=0 if protect mode interface active */
	vbe_PMInterface* pm_info;

	/* PM MMIO selector */
	unsigned pm_mmio_selector; /* ==0 if not present */
	unsigned pm_mmio_address;
	unsigned pm_mmio_size;

	/* PM Address for call */
	unsigned pm_switcher; /* ==0 if not present */
	unsigned pm_scroller; /* ==0 if not present */
	unsigned pm_palette; /* ==0 if not present */

	/* RGB color */
	unsigned rgb_red_mask;
	unsigned rgb_green_mask;
	unsigned rgb_blue_mask;
	int rgb_red_shift;
	int rgb_green_shift;
	int rgb_blue_shift;

	/* Palette color */
	unsigned palette_width;

	/* VBE Information */
	vbe_VbeInfoBlock info;

	/* VBE Mode Information */
	vbe_ModeInfoBlock mode_info;
} vbe_internal;

static inline unsigned vbe_rgb_nibble(unsigned value, int shift, unsigned mask)
{
	return (shift >= 0 ? value << shift : value >> -shift) & mask;
}

static inline unsigned vbe_rgb_invnibble(unsigned value, int shift, unsigned mask)
{
	value &= mask;
	return shift >= 0 ? value >> shift : value << -shift;
}

/* VBE Internal Mode Information */
extern vbe_internal vbe_state;

/***************************************************************************/
/* Public */

/* Return the offset for accessing in writing the video memory */
extern unsigned char* (*vbe_write_line)(unsigned y);

static inline adv_bool vbe_is_active(void)
{
	return vbe_state.active != 0;
}

static inline adv_bool vbe_mode_is_active(void)
{
	return vbe_state.mode_active != 0;
}

static inline adv_bool vbe_linear_is_active(void)
{
	return vbe_state.linear_active != 0;
}

static inline unsigned vbe_virtual_x(void)
{
	return vbe_state.virtual_x;
}

static inline unsigned vbe_virtual_y(void)
{
	return vbe_state.virtual_y;
}

static inline unsigned vbe_bytes_per_scanline(void)
{
	return vbe_state.bytes_per_scanline;
}

unsigned vbe_adjust_bytes_per_page(unsigned bytes_per_page);

adv_color_def vbe_color_def(void);

static inline adv_bool vbe_pm_is_active(void)
{
	return vbe_state.pm_active != 0;
}

static inline unsigned vbe_font_size_x(void)
{
	assert(vbe_mode_is_active());
	return vbe_state.mode_info.XCharSize;
}

static inline unsigned vbe_font_size_y(void)
{
	assert(vbe_mode_is_active());
	return vbe_state.mode_info.YCharSize;
}

adv_error vbe_init(void);
void vbe_done(void);
adv_error vbe_mode_info_get(vbe_ModeInfoBlock* info, unsigned mode);
adv_error vga_as_vbe_mode_info_get(vbe_ModeInfoBlock* info, unsigned mode);
adv_error vbe_mode_set(unsigned mode, const vbe_CRTCInfoBlock* crtc);
adv_error vbe_mode_get(unsigned* mode);
void vbe_mode_done(adv_bool restore);

adv_error vbe_scanline_set(unsigned byte_length);
adv_error vbe_scanline_pixel_set(unsigned pixel_length);
adv_error vbe_scanline_pixel_request(unsigned pixel_length);

adv_error vbe_scroll(unsigned offset, adv_bool waitvsync);
adv_error vbe_request_scroll(unsigned offset);
adv_error vbe_poll_scroll(unsigned* done);

adv_error vbe_pixelclock_get(unsigned* pixelclock, unsigned mode);
adv_error vbe_pixelclock_getnext(unsigned* pixelclock, unsigned mode);
adv_error vbe_pixelclock_getpred(unsigned* pixelclock, unsigned mode);

void vbe_wait_vsync(void);

typedef struct vbe_mode_iterator_struct {
	long mode_ptr;
} vbe_mode_iterator;

void vbe_mode_iterator_begin(vbe_mode_iterator* vmi);
unsigned vbe_mode_iterator_get(vbe_mode_iterator* vmi);
adv_bool vbe_mode_iterator_end(vbe_mode_iterator* vmi);
void vbe_mode_iterator_next(vbe_mode_iterator* vmi);

adv_error vbe_palette_set(const adv_color_rgb* palette, unsigned start, unsigned count, adv_bool waitvsync);

adv_error vbe_restore_state(unsigned size, void* ptr);
adv_error vbe_save_state(unsigned* psize, void** pptr);

#endif
