/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003 Andrea Mazzoleni
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
 */

#include "vbe.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>

#include "svgalib.h"

/****************************************************************************/
/* Types */

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned long

typedef struct realptr_struct {
	uint16 off __attribute__ ((packed));
	uint16 seg __attribute__ ((packed));
} realptr __attribute__ ((packed));

/****************************************************************************/
/* VBE */

/* If defined the data in the VESAInfoBlock point to a low 1M memory block */
/* Otherwise the data is allocated in the OemDATA entry of the structure */
/* #define VBE_LOW_DATA */

#define VBE_CUSTOM_MODE 0x120

struct vbe_VbeInfoBlock_struct {
	uint8 VESASignature[4] __attribute__ ((packed)); /* 'VESA' 4 byte signature */
	uint16 VESAVersion __attribute__ ((packed)); /* VBE version number */
	realptr OemStringPtr __attribute__ ((packed)); /* Pointer to OEM string */
	uint32 Capabilities __attribute__ ((packed)); /* Capabilities of video card */
	realptr VideoModePtr __attribute__ ((packed)); /* Pointer to supported modes */
	uint16 TotalMemory __attribute__ ((packed));  /* Number of 64kb memory blocks */

	/* VBE 2.0 extension information */
	uint16 OemSoftwareRev __attribute__ ((packed)); /* OEM Software revision number */
	realptr OemVendorNamePtr __attribute__ ((packed)); /* Pointer to Vendor Name string */
	realptr OemProductNamePtr __attribute__ ((packed)); /* Pointer to Product Name string */
	realptr OemProductRevPtr __attribute__ ((packed)); /* Pointer to Product Revision str */
	uint8 Reserved[222] __attribute__ ((packed)); /* Pad to 256 byte block size */
	uint8 OemDATA[256] __attribute__ ((packed)); /* Scratch pad for OEM data */
}  __attribute__ ((packed));

typedef struct vbe_VbeInfoBlock_struct vbe_VbeInfoBlock;

/* Mode information block */
struct vbe_ModeInfoBlock_struct {
	uint16 ModeAttributes __attribute__ ((packed)); /* Mode attributes */
	uint8 WinAAttributes __attribute__ ((packed)); /* Window A attributes */
	uint8 WinBAttributes __attribute__ ((packed)); /* Window B attributes */
	uint16 WinGranularity __attribute__ ((packed)); /* Window granularity in k */
	uint16 WinSize __attribute__ ((packed)); /* Window size in k */
	uint16 WinASegment __attribute__ ((packed)); /* Window A segment */
	uint16 WinBSegment __attribute__ ((packed)); /* Window B segment */
	realptr WinFuncPtr __attribute__ ((packed)); /* Pointer to window function */
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
} __attribute__ ((packed));

typedef struct vbe_ModeInfoBlock_struct vbe_ModeInfoBlock;

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
/* ADVVBE */

#define MODE_MAX 512

typedef struct mode_info_struct {
	unsigned hde, hrs, hre, ht;
	unsigned vde, vrs, vre, vt;
	unsigned pixelclock;
	int doublescan;
	int interlace;
	int nvsync;
	int nhsync;
	unsigned number;
	unsigned bits_per_pixel;
} mode_info;

struct state_context {
	/** Version of the VBE. */
	unsigned version;

	/** Video mode vector. */
	mode_info mode_map[MODE_MAX];

	/** Number of video mode defined. */
	unsigned mode_max;

	/** VBE video mode number list. */
	uint16 mode_list[MODE_MAX+1];

	/** Active video mode. */
	mode_info* mode;

	unsigned mode_number_generator;

	/** Dos memory buffer. */
	realptr dos_buffer;

	unsigned char saved[ADV_SVGALIB_STATE_SIZE];

	unsigned bytes_per_scanline;
	unsigned bytes_per_pixel;
	unsigned scroll;

	unsigned char palette[768];
};

struct state_context state;

struct mode_number_struct {
	unsigned number;
	unsigned x;
	unsigned y;
	unsigned bits;
};

/** VESA 1.2 default numbering. */
static struct mode_number_struct mode_number_map[] = {
{ 0x100, 640, 400, 8 },
{ 0x101, 640, 480, 8 },
{ 0x103, 800, 600, 8 },
{ 0x105, 1024, 768, 8 },
{ 0x107, 1280, 1024, 8 },
{ 0x10D, 320, 200, 15 },
{ 0x10E, 320, 200, 16 },
{ 0x10F, 320, 200, 24 },
{ 0x110, 640, 480, 15 },
{ 0x111, 640, 480, 16 },
{ 0x112, 640, 480, 24 },
{ 0x113, 800, 600, 15 },
{ 0x114, 800, 600, 16 },
{ 0x115, 800, 600, 24 },
{ 0x116, 1024, 768, 15 },
{ 0x117, 1024, 768, 16 },
{ 0x118, 1024, 768, 24 },
{ 0x119, 1280, 1024, 15 },
{ 0x11A, 1280, 1024, 16 },
{ 0x11B, 1280, 1024, 24 },
{0, 0, 0, 0}
};

unsigned mode_find_number(unsigned x, unsigned y, unsigned bits)
{
	unsigned i;
	unsigned number;
	for(i=0;mode_number_map[i].number;++i)
		if (mode_number_map[i].x==x && mode_number_map[i].y==y && mode_number_map[i].bits==bits)
			break;
	if (!mode_number_map[i].number)
		return state.mode_number_generator++;
	number = mode_number_map[i].number;
	for(i=0;i<state.mode_max;++i)
		if (state.mode_map[i].number == number)
			break;
	if (i<state.mode_max)
		return state.mode_number_generator++;
	return number;
}

int mode_insert(mode_info* mode_ptr, int bits_per_pixel)
{
	unsigned number;

	if (state.mode_max == MODE_MAX) {
		printf("Too many modelines.\n");
		return -1;
	}

	number = mode_find_number(mode_ptr->hde, mode_ptr->vde, bits_per_pixel);

	/* the VBE mode set is limited to 9 bit */
	if ((number & vbeModeMask) != number) {
		printf("Too many modes.\n");
		return -1;
	}

	memcpy(state.mode_map + state.mode_max, mode_ptr, sizeof(mode_info));

	state.mode_map[state.mode_max].bits_per_pixel = bits_per_pixel;
	state.mode_map[state.mode_max].number = number;

	state.mode_list[state.mode_max] = number;

	++state.mode_max;

	return 0;
}

int modeline_load(mode_info* mode_ptr)
{
	char* s;
	char* name;
	char* endp;

	/* skip the name */
	name = strtok(0, " \t\r\n");
	if (!name) goto err;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;

	mode_ptr->pixelclock = strtod(s, &endp) * 1E6;
	if (*endp) goto err_name;
	if (!mode_ptr->pixelclock) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->hde = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->hrs = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->hre = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->ht = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vde = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vrs = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vre = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	s = strtok(0, " \t\r\n");
	if (!s) goto err_name;
	mode_ptr->vt = strtol(s, &endp, 10);
	if (*endp) goto err_name;

	mode_ptr->doublescan = 0;
	mode_ptr->interlace = 0;
	mode_ptr->nhsync = 0;
	mode_ptr->nvsync = 0;

	while ((s = strtok(0, " \t\r\n"))!=0) {
		if (s[0]=='#')
			break;
		else if (strcmp(s, "doublescan")==0)
			mode_ptr->doublescan = 1;
		else if (strcmp(s, "interlaced")==0 || strcmp(s, "interlace")==0)
			mode_ptr->interlace = 1;
		else if (strcmp(s, "-hsync")==0)
			mode_ptr->nhsync = 1;
		else if (strcmp(s, "-vsync")==0)
			mode_ptr->nvsync = 1;
	}

	return 0;

err_name:
	printf("Error reading the modeline %s.\n", name);
	return -1;
err:
	printf("Error reading a modeline.\n");
	return -1;
}

static int mode_load(const char* file)
{
	char buffer[256];
	unsigned i;
	FILE* f;

	f = fopen(file, "r");
	if (!f) {
		printf("Configuration file %s not found.\n", file);
		return -1;
	}

	state.mode_max = 0;

	while (fgets(buffer, sizeof(buffer), f)) {
		char* s = strtok(buffer, " \t\r\n");

		if (s && strcmp(s, "device_video_modeline")==0) {
			mode_info info;
			if (modeline_load(&info)!=0) {
				fclose(f);
				return -1;
			}

			if (adv_svgalib_state.has_bit8 && mode_insert(&info, 8) != 0) {
				fclose(f);
				return -1;
			}
			if (adv_svgalib_state.has_bit15 && mode_insert(&info, 15) != 0) {
				fclose(f);
				return -1;
			}
			if (adv_svgalib_state.has_bit16 && mode_insert(&info, 16) != 0) {
				fclose(f);
				return -1;
			}
			if (adv_svgalib_state.has_bit24 && mode_insert(&info, 24) != 0) {
				fclose(f);
				return -1;
			}
			if (adv_svgalib_state.has_bit32 && mode_insert(&info, 32) != 0) {
				fclose(f);
				return -1;
			}
		}
	}

	fclose(f);

	if (!state.mode_max) {
		printf("No modelines found.\n");
		return -1;
	}

	/* sort of mode_list */
	for(i=1;i<state.mode_max;++i) {
		unsigned j;
		for(j=state.mode_max-1;j>=i;--j) {
			if (state.mode_list[j-1]>state.mode_list[j]) {
				unsigned t = state.mode_list[j-1];
				state.mode_list[j-1] = state.mode_list[j];
				state.mode_list[j] = t;
			}
		}
	}

	/* end of the list marker */
	state.mode_list[state.mode_max] = 0xFFFF;

	return 0;
}

static int probe_callback(unsigned bus_device_func, unsigned vendor, unsigned device, void* _arg)
{
	unsigned dw;
	unsigned base_class;
	unsigned subsys_card;
	unsigned subsys_vendor;

	if (adv_svgalib_pci_read_dword(bus_device_func, 0x8, &dw)!=0)
		return 0;

	base_class = (dw >> 24) & 0xFF;
	if (base_class != 0x3 /* PCI_CLASS_DISPLAY */)
		return 0;

	*(int*)_arg = 1;

	if (adv_svgalib_pci_read_dword(bus_device_func, 0x2c, &dw)!=0)
		return 0;

	subsys_vendor = dw & 0xFFFF;
	subsys_card = (dw >> 16) & 0xFFFF;

	printf("VendorID %04x, DeviceID %04x, Bus %d, Device %d\n", vendor, device, bus_device_func >> 8, bus_device_func & 0xFF);

	return 0;
}

static int driver_init(void)
{
	int found;

	printf("\n");

	if (adv_svgalib_init(0) != 0) {
		printf("Error initializing SVGALIB.\n");
		return -1;
	}

	if (adv_svgalib_detect("auto") != 0) {
		printf("Unsupported video board.\n");
		return -1;
	}

	printf("Board\n");
	found = 0;
	adv_svgalib_pci_scan_device(probe_callback, &found);
	if (!found)
		printf("ISA (?)\n");
	printf("\n");
	
	printf("Driver\n");
	printf("Name : %s\n", adv_svgalib_driver_get());

	printf("Bit depth : ");
	if (adv_svgalib_state.has_bit8) printf("8 ");
	if (adv_svgalib_state.has_bit15) printf("15 ");
	if (adv_svgalib_state.has_bit16) printf("16 ");
	if (adv_svgalib_state.has_bit24) printf("24 ");
	if (adv_svgalib_state.has_bit32) printf("32 ");
	printf("\n");
	if (adv_svgalib_state.has_interlace)
		printf("Interlace : yes\n");
	else
		printf("Interlace : no\n");
	printf("Linear memory : %08x, %d Mbyte\n", adv_svgalib_linear_base_get(), adv_svgalib_linear_size_get() / (1024*1024));
	if (adv_svgalib_mmio_size_get()) {
		printf("MMIO memory : %08x, %d byte\n", adv_svgalib_mmio_base_get(), adv_svgalib_mmio_size_get());
	}

	return 0;
}

int vbe_init(const char* config)
{
	_go32_dpmi_seginfo info;

	memset(&state, 0, sizeof(state));
	state.mode_max = 0;
	state.mode_number_generator = VBE_CUSTOM_MODE;
	state.version = 0x200;
	state.mode = 0;

	if (driver_init() != 0) {
		return -1;
	}

	if (mode_load(config) != 0) {
		return -1;
	}

	memset(&info, 0, sizeof(info));
	info.size = 4096 / 16;
	if (_go32_dpmi_allocate_dos_memory(&info) != 0) {
		printf("Error allocating DOS memory.\n");
		return -1;
	}
	state.dos_buffer.seg = info.rm_segment;
	state.dos_buffer.off = 0;

	return 0;
}

void vbe_done(void)
{
	adv_svgalib_done();
}

void adv_svgalib_log_va(const char *text, va_list arg)
{
}

/****************************************************************************/

static realptr oem_alloc(realptr* base, void* data, unsigned size)
{
	realptr r = *base;
	unsigned offset = base->seg * 16 + base->off;
	dosmemput(data, size, offset);
	base->off += size;
	return r;
}

static int mode_set(mode_info* mode)
{
	adv_svgalib_save(state.saved);

	adv_svgalib_set(mode->pixelclock, mode->hde, mode->hrs, mode->hre, mode->ht, mode->vde, mode->vrs, mode->vre, mode->vt, mode->doublescan, mode->interlace, mode->nhsync, mode->nvsync, mode->bits_per_pixel, 0, 0);

	state.mode = mode;
	state.bytes_per_pixel = (mode->bits_per_pixel + 7) / 8;
	state.bytes_per_scanline = state.bytes_per_pixel * mode->hde;
	state.scroll = 0;
	memset(state.palette, 0, sizeof(state.palette));

	return 0;
}

static void mode_unset(void)
{
	adv_svgalib_unset();

	adv_svgalib_restore(state.saved);

	state.mode = 0;
}

static void vga_service_00(_go32_dpmi_registers* r)
{
	if (state.mode) {
		mode_unset();
	}
}

static void vbe_service_4f00(_go32_dpmi_registers* r)
{
	vbe_VbeInfoBlock info;
	realptr info_addr;
	realptr oem_addr;
	realptr reserved_addr;
	int is_vbe2;
	unsigned size;

	info_addr.seg = r->x.es;
	info_addr.off = r->x.di;

	dosmemget(info_addr.seg * 16 + info_addr.off, 4, info.VESASignature);

	is_vbe2 = memcmp(info.VESASignature, "VBE2", 4)==0;

	if (is_vbe2) {
#ifdef VBE_LOW_DATA
		reserved_addr.seg = dos_buffer.seg;
		reserved_addr.off = dos_buffer.off;
#else
		if (state.mode_max <= sizeof(info.Reserved)/2 - 1) {
			reserved_addr.seg = info_addr.seg;
			reserved_addr.off = info_addr.off + 34;
		} else {
			/* too many mode to fit in the reserved buffer */
			reserved_addr.seg = state.dos_buffer.seg;
			reserved_addr.off = state.dos_buffer.off;
		}
#endif
		oem_addr.seg = info_addr.seg;
		oem_addr.off = info_addr.off + 256;
		size = 34;
		state.version = 0x200;
	} else {
		r->x.ax = 0x0100; /* not supported */
		return;
	}

	memcpy(info.VESASignature, "VESA", 4);
	info.VESAVersion = state.version;
	info.OemStringPtr = oem_alloc(&oem_addr, OEM_STR, strlen(OEM_STR)+1);
	info.Capabilities = 0;
	info.VideoModePtr = oem_alloc(&reserved_addr, &state.mode_list, 2*(state.mode_max+1));
	info.TotalMemory = adv_svgalib_linear_size_get() / (1 << 16);

	if (state.version >= 0x200) {
		info.OemSoftwareRev = OEM_VERSION_NUM;
		info.OemVendorNamePtr = oem_alloc(&oem_addr, OEM_VENDOR_STR, strlen(OEM_VENDOR_STR)+1);
		info.OemProductNamePtr = oem_alloc(&oem_addr, OEM_PRODUCT_STR, strlen(OEM_PRODUCT_STR)+1);
		info.OemProductRevPtr = oem_alloc(&oem_addr, OEM_VERSION_STR, strlen(OEM_VERSION_STR)+1);
	}

	dosmemput(&info, size, info_addr.seg * 16 + info_addr.off);

	r->x.ax = 0x004f; /* success */
}

static void vbe_service_4f01(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F01h   Return VBE mode information
               CX   =         Mode number
            ES:DI   =         Pointer to ModeInfoBlock structure

     Output:   AX   =         VBE Return Status
*/

	unsigned i;
	unsigned number;
	vbe_ModeInfoBlock info;
	unsigned size;
	mode_info* mode;

	number = r->x.cx;
	for(i=0;i<state.mode_max;++i) {
		if (state.mode_map[i].number == number)
			break;
	}
	if (i == state.mode_max) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	mode = state.mode_map + i;
	size = 256;

	memset(&info, 0, sizeof(info));
	info.ModeAttributes = vbeMdAvailable | vbeMdColorMode | vbeMdGraphMode | vbeMdNonBanked | vbeMdLinear;
	info.WinAAttributes = 0;
	info.WinBAttributes = 0;
	info.WinGranularity = 0;
	info.WinSize = 0;
	info.WinASegment = 0;
	info.WinBSegment = 0;
	info.WinFuncPtr.seg = 0;
	info.WinFuncPtr.off = 0;
	info.BytesPerScanLine = ((mode->bits_per_pixel + 7) / 8) * mode->hde;
	info.XResolution = mode->hde;
	info.YResolution = mode->vde;
	info.XCharSize = 0;
	info.YCharSize = 0;
	info.NumberOfPlanes = 1;
	info.BitsPerPixel = mode->bits_per_pixel;
	info.NumberOfBanks = 0;
	if (mode->bits_per_pixel == 8)
		info.MemoryModel = vbeMemPK;
	else
		info.MemoryModel = vbeMemRGB;
	info.BankSize = 0;
	info.NumberOfImagePages = adv_svgalib_linear_size_get() / (info.YResolution * (unsigned)info.BytesPerScanLine) - 1;
	info.ReservedPage = 1;
	switch (info.BitsPerPixel) {
		case 15 :
			info.RedMaskSize = 5;
			info.RedFieldPosition = 10;
			info.GreenMaskSize = 5;
			info.GreenFieldPosition = 5;
			info.BlueMaskSize = 5;
			info.BlueFieldPosition = 0;
			info.RsvdMaskSize = 1;
			info.RsvdFieldPosition = 15;
			break;
		case 16 :
			info.RedMaskSize = 5;
			info.RedFieldPosition = 11;
			info.GreenMaskSize = 6;
			info.GreenFieldPosition = 5;
			info.BlueMaskSize = 5;
			info.BlueFieldPosition = 0;
			info.RsvdMaskSize = 0;
			info.RsvdFieldPosition = 0;
			break;
		case 24 :
			info.RedMaskSize = 8;
			info.RedFieldPosition = 16;
			info.GreenMaskSize = 8;
			info.GreenFieldPosition = 8;
			info.BlueMaskSize = 8;
			info.BlueFieldPosition = 0;
			info.RsvdMaskSize = 0;
			info.RsvdFieldPosition = 0;
			break;
		case 32 :
			info.RedMaskSize = 8;
			info.RedFieldPosition = 16;
			info.GreenMaskSize = 8;
			info.GreenFieldPosition = 8;
			info.BlueMaskSize = 8;
			info.BlueFieldPosition = 0;
			info.RsvdMaskSize = 8;
			info.RsvdFieldPosition = 24;
			break;
		default:
			info.RedMaskSize = 0;
			info.RedFieldPosition = 0;
			info.GreenMaskSize = 0;
			info.GreenFieldPosition = 0;
			info.BlueMaskSize = 0;
			info.BlueFieldPosition = 0;
			info.RsvdMaskSize = 0;
			info.RsvdFieldPosition = 0;
			break;
	}
	info.DirectColorModeInfo = vbeDcRsvdUsable;

	if (state.version >= 0x200) {
		info.PhysBasePtr = adv_svgalib_linear_base_get();
		info.OffScreenMemOffset = 0;
		info.OffScreenMemSize = 0;
	}

	dosmemput(&info, size, r->x.es * 16 + r->x.di);

	r->x.ax = 0x004f; /* success */
}

static void vbe_service_4f02(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F02h     Set VBE Mode
               BX   =           Desired Mode to set
                    D0-D8  =    Mode number
                    D9-D13 =    Reserved (must be 0)
                    D14    = 0  Use windowed frame buffer model
                           = 1  Use linear/flat frame buffer model
                    D15    = 0  Clear display memory
                           = 1  Don't clear display memory

     Output:   AX   =           VBE Return Status
*/

	unsigned i;
	unsigned number;
	mode_info* mode;

	number = r->x.bx & vbeModeMask;
	for(i=0;i<state.mode_max;++i) {
		if (state.mode_map[i].number == number)
			break;
	}
	if (i == state.mode_max) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	mode = state.mode_map + i;

	if ((r->x.bx & vbeLinearBuffer) == 0) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	/* TODO support the clear bit */

	if (state.mode) {
		mode_unset();
	}

	if (mode_set(mode) != 0) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	r->x.ax = 0x004f; /* success */
	return;
}

static void vbe_service_4f03(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F03h   Return current VBE Mode

     Output:   AX   =         VBE Return Status
               BX   =         Current VBE mode
                    D0-D13 =  Mode number
                    D14  = 0  Windowed frame buffer model
                         = 1  Linear/flat frame buffer model
                    D15  = 0  Memory cleared at last mode set
                         = 1  Memory not cleared at last mode set
*/

	if (!state.mode) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	r->x.bx = state.mode->number | vbeLinearBuffer;

	r->x.ax = 0x004f; /* success */
}

static void vbe_service_4f04(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F04h   Save/Restore state
               DL   = 00h         Return save/restore state buffer size
                    = 01h         Save state
                    = 02h         Restore state
               CX   =         Requested states
                    D0=           Save/restore controller hardware state
                    D1=           Save/restore BIOS data state
                    D2=           Save/restore DAC state
                    D3=           Save/restore Register state
               ES:BX=         Pointer to buffer  (if DL <> 00h)

     Output:   AX   =         VBE Return Status
               BX   =         Number of 64-byte blocks to hold the state
                              buffer  (if DL=00h)

*/
	if (r->h.dl == 0) {
		r->x.bx = ADV_SVGALIB_STATE_SIZE;
		r->x.ax = 0x004f; /* success */
		return;
	}

	/* TODO save and restore the BIOS state */
	/* TODO save and restore the DAC state separatly */

	if (r->h.dl == 1) {
		if ((r->x.cx & 0x1) != 0 || (r->x.cx & 0x4) != 0 || (r->x.cx & 0x8) != 0) {
			unsigned char regs[ADV_SVGALIB_STATE_SIZE];
			adv_svgalib_save(regs);
			dosmemput(regs, ADV_SVGALIB_STATE_SIZE, r->x.es * 16 + r->x.bx);
		}
		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.dl == 2) {
		if ((r->x.cx & 0x1) != 0 || (r->x.cx & 0x4) != 0 || (r->x.cx & 0x8) != 0) {
			unsigned char regs[ADV_SVGALIB_STATE_SIZE];
			dosmemget(r->x.es * 16 + r->x.bx, ADV_SVGALIB_STATE_SIZE, regs);
			adv_svgalib_restore(regs);
		}
		r->x.ax = 0x004f; /* success */
		return;
	}

	r->x.ax = 0x014f; /* error */
}

static void vbe_service_4f05(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F05h   VBE Display Window Control
               BH   = 00h          Set memory window
                    = 01h          Get memory window
               BL   =         Window number
                    = 00h          Window A
                    = 01h          Window B
               DX   =         Window number in video memory in window
                              granularity units  (Set Memory Window only)

     Output:   AX   =         VBE Return Status
               DX   =         Window number in window granularity units
                                   (Get Memory Window only)
*/
	r->x.ax = 0x024f; /* invalid in this mode */
}

static void vbe_service_4f06(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F06h   VBE Set/Get Logical Scan Line Length
               BL   = 00h          Set Scan Line Length in Pixels
                    = 01h          Get Scan Line Length
                    = 02h          Set Scan Line Length in Bytes
                    = 03h          Get Maximum Scan Line Length
               CX   =         If BL=00h  Desired Width in Pixels
                              If BL=02h  Desired Width in Bytes
                              (Ignored for Get Functions)

     Output:   AX   =         VBE Return Status
               BX   =         Bytes Per Scan Line
               CX   =         Actual Pixels Per Scan Line
                              (truncated to nearest complete pixel)
               DX   =         Maximum Number of Scan Lines
*/
	if (!state.mode) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	if (r->h.bl == 0) {
		state.bytes_per_scanline = r->x.cx * state.bytes_per_pixel;
		adv_svgalib_scanline_set(state.bytes_per_scanline);
		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 1) {
		r->x.bx = state.bytes_per_scanline;
		r->x.cx = state.bytes_per_scanline / state.bytes_per_pixel;
		r->x.dx = adv_svgalib_linear_size_get() / state.bytes_per_scanline;
		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 2) {
		state.bytes_per_scanline = r->x.cx;
		adv_svgalib_scanline_set(state.bytes_per_scanline);
		r->x.bx = state.bytes_per_scanline;
		r->x.cx = state.bytes_per_scanline / state.bytes_per_pixel;
		r->x.dx = adv_svgalib_linear_size_get() / state.bytes_per_scanline;
		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 3) {
		r->x.cx = adv_svgalib_linear_size_get() / (state.mode->hde * state.bytes_per_pixel);
		if (r->x.cx > 1024)
			r->x.cx = 1024;
		r->x.bx = r->x.cx * state.bytes_per_pixel;
		r->x.dx = adv_svgalib_linear_size_get() / (r->x.cx * state.bytes_per_pixel);
		r->x.ax = 0x004f; /* success */
		return;
	}

	r->x.ax = 0x014f; /* error */
}

static void vbe_service_4f07(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F07h   VBE Set/Get Display Start Control
               BH   = 00h          Reserved and must be 00h
               BL   = 00h          Set Display Start
                    = 01h          Get Display Start
                    = 80h          Set Display Start during Vertical Retrace
               CX   =         First Displayed Pixel In Scan Line
                              (Set Display Start only)
               DX   =         First Displayed Scan Line (Set Display Start
     only)

     Output:   AX   =         VBE Return Status
               BH   =         00h Reserved and will be 0 (Get Display Start
     only)
               CX   =         First Displayed Pixel In Scan Line (Get Display
                              Start only)
               DX   =         First Displayed Scan Line (Get Display
                              Start only)
*/
	if (!state.mode) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	if (r->h.bh != 0) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	if (r->h.bl == 0 || r->h.bl == 0x80) {
		if (r->h.bl == 0x80)
			adv_svgalib_wait_vsync();
		state.scroll = r->x.dx * state.bytes_per_scanline + r->x.cx * state.bytes_per_pixel;
		adv_svgalib_scroll_set(state.scroll);
		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 1) {
		r->x.dx = state.scroll / state.bytes_per_scanline;
		r->x.cx = (state.scroll % state.bytes_per_scanline) / state.bytes_per_pixel;
		r->x.ax = 0x004f; /* success */
		return;
	}

	r->x.ax = 0x014f; /* error */
}

static void vbe_service_4f08(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F08h   VBE Set/Get Palette Format
               BL   = 00h          Set DAC Palette Format
                    = 01h          Get DAC Palette Format
               BH   =         Desired bits of color per primary
                              (Set DAC Palette Format only)

     Output:   AX   =         VBE Return Status
               BH   =         Current number of bits of color per primary
*/
	if (!state.mode) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	if (r->h.bl == 0) {
		if (r->h.bh != 6) {
			r->h.bh = 6;
			r->x.ax = 0x014f; /* error */
			return;
		}

		r->h.bh = 6;
		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 1) {
		r->h.bh = 6;
		r->x.ax = 0x004f; /* success */
		return;
	}

	r->x.ax = 0x014f; /* error */
}

static void vbe_service_4f09(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F09h   VBE Load/Unload Palette Data
               BL   = 00h          Set Palette Data
                    = 01h          Get Palette Data
                    = 02h          Set Secondary Palette Data
                    = 03h          Get Secondary Palette Data
                    = 80h          Set Palette Data during Vertical Retrace
                                with Blank Bit on
               CX   =         Number of palette registers to update
               DX   =         First palette register to update
               ES:DI=         Table of palette values (see below for
     format)

     Output:   AX   =         VBE Return Status
*/
	if (!state.mode) {
		r->x.ax = 0x014f; /* error */
		return;
	}

	if (r->h.bl == 0 || r->h.bl == 0x80) {
		unsigned i;
		unsigned char* rgb;

		if (r->x.cx + r->x.dx >= 256) {
			r->x.ax = 0x014f; /* error */
			return;
		}

		if (r->h.bl == 0x80)
			adv_svgalib_wait_vsync();

		dosmemget(r->x.es * 16 + r->x.di, r->x.cx * 3, state.palette + r->x.dx * 3);

		rgb = state.palette + r->x.dx * 3;
		for(i=0;i<r->x.cx;++i) {
			adv_svgalib_palette_set(r->x.dx + i, rgb[0] << 2, rgb[1] << 2, rgb[2] << 2);
			rgb += 3;
		}

		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 1) {
		if (r->x.cx + r->x.dx >= 256) {
			r->x.ax = 0x014f; /* error */
			return;
		}

		dosmemput(state.palette + r->x.dx * 3, r->x.cx * 3, r->x.es * 16 + r->x.di);

		r->x.ax = 0x004f; /* success */
		return;
	}

	if (r->h.bl == 2 || r->h.bl == 3) {
		r->x.ax = 0x024f; /* invalid in this mode */
		return;
	}

	r->x.ax = 0x014f; /* error */
}

static void vbe_service_4f0A(_go32_dpmi_registers* r)
{
/*
     Input:    AX   = 4F0Ah   VBE 2.0 Protected Mode Interface
               BL   = 00h          Return protected mode table

     Output:   AX   =         Status
               ES   =         Real Mode Segment of Table
               DI   =         Offset of Table
               CX   =         Length of Table including protected mode code
                              (for copying purposes)
*/

	r->x.ax = 0x0100; /* not supported */
}

void vbe_service(_go32_dpmi_registers* r)
{
	switch (r->x.ax) {
	case 0x4f00 : vbe_service_4f00(r); break;
	case 0x4f01 : vbe_service_4f01(r); break;
	case 0x4f02 : vbe_service_4f02(r); break;
	case 0x4f03 : vbe_service_4f03(r); break;
	case 0x4f04 : vbe_service_4f04(r); break;
	case 0x4f05 : vbe_service_4f05(r); break;
	case 0x4f06 : vbe_service_4f06(r); break;
	case 0x4f07 : vbe_service_4f07(r); break;
	case 0x4f08 : vbe_service_4f08(r); break;
	case 0x4f09 : vbe_service_4f09(r); break;
	case 0x4f0A : vbe_service_4f0A(r); break;
	default:
		r->x.ax = 0x0100; /* not supported */
		break;
	}
}

void vga_service(_go32_dpmi_registers* r)
{
	switch (r->h.ah) {
	case 0x00 : vga_service_00(r); break;
	}
}

