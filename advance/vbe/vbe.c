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
 */

#include "tsr.h"
#include "lib.h"

#include "card.h"
#include "board.h"

extern void far * int_10_old;

/***************************************************************************/
/* VBE */

/* If defined the data in the VESAInfoBlock point to a low 1M memory block */
/* Otherwise the data is allocated in the OemDATA entry of the structure */
/* #define VBE_LOW_DATA */

/* OEM information */
#define OEM_VENDOR_STR "Andrea Mazzoleni"
#define OEM_PRODUCT_STR "AdvanceVBE"
#define OEM_VERSION_NUM 0x000C
#define OEM_VERSION_STR "0.12 " __DATE__
#define OEM_STR OEM_VENDOR_STR " " OEM_PRODUCT_STR " " OEM_VERSION_STR

#define VBE_CUSTOM_MODE 0x140

typedef struct vbe_VbeInfoBlock_struct {
	uint8 VESASignature[4] ; /* 'VESA' 4 byte signature */
	uint16 VESAVersion; /* VBE version number */
	char far* OemStringPtr; /* Pointer to OEM string */
	uint32 Capabilities; /* Capabilities of video card */
	uint16 far* VideoModePtr; /* Pointer to supported modes */
	uint16 TotalMemory;  /* Number of 64kb memory blocks */

	/* VBE 2.0 extension information */
	uint16 OemSoftwareRev; /* OEM Software revision number */
	char far* OemVendorNamePtr; /* Pointer to Vendor Name string */
	char far* OemProductNamePtr; /* Pointer to Product Name string */
	char far* OemProductRevPtr; /* Pointer to Product Revision str */
	uint8 Reserved[222]; /* Pad to 256 byte block size */
	uint8 OemDATA[256]; /* Scratch pad for OEM data */
} vbe_VbeInfoBlock;

/* Mode information block */
typedef struct vbe_ModeInfoBlock_struct {
	uint16 ModeAttributes; /* Mode attributes */
	uint8 WinAAttributes; /* Window A attributes */
	uint8 WinBAttributes; /* Window B attributes */
	uint16 WinGranularity; /* Window granularity in k */
	uint16 WinSize; /* Window size in k */
	uint16 WinASegment; /* Window A segment */
	uint16 WinBSegment; /* Window B segment */
	uint32 WinFuncPtr; /* Pointer to window function */
	uint16 BytesPerScanLine; /* Bytes per scanline */
	uint16 XResolution; /* Horizontal resolution */
	uint16 YResolution; /* Vertical resolution */
	uint8 XCharSize; /* Character cell width */
	uint8 YCharSize; /* Character cell height */
	uint8 NumberOfPlanes; /* Number of memory planes */
	uint8 BitsPerPixel; /* Bits per pixel */
	uint8 NumberOfBanks; /* Number of CGA style banks */
	uint8 MemoryModel; /* Memory model type */
	uint8 BankSize; /* Size of CGA style banks */
	uint8 NumberOfImagePages; /* Number of images pages */
	uint8 ReservedPage; /* Reserved */
	uint8 RedMaskSize; /* Size of direct color red mask */
	uint8 RedFieldPosition; /* Bit posn of lsb of red mask */
	uint8 GreenMaskSize; /* Size of direct color green mask */
	uint8 GreenFieldPosition; /* Bit posn of lsb of green mask */
	uint8 BlueMaskSize; /* Size of direct color blue mask */
	uint8 BlueFieldPosition; /* Bit posn of lsb of blue mask */
	uint8 RsvdMaskSize; /* Size of direct color res mask */
	uint8 RsvdFieldPosition; /* Bit posn of lsb of res mask */
	uint8 DirectColorModeInfo; /* Direct color mode attributes */

	/* VBE 2.0 extensions information */
	uint32 PhysBasePtr; /* Physical address for linear buf */
	uint32 OffScreenMemOffset; /* Pointer to start of offscreen mem */
	uint16 OffScreenMemSize; /* Amount of offscreen mem in 1K's */

	/* VBE 3.0 extensions */
	uint16 LinBytesPerScanLine; /* Bytes per scanline */
	uint8 BnkNumberOfPages; /* Number of images pages (banked) */
	uint8 LinNumberOfPages; /* Number of images pages (linear) */
	uint8 LinRedMaskSize; /* Size of direct color red mask */
	uint8 LinRedFieldPosition; /* Bit posn of lsb of red mask */
	uint8 LinGreenMaskSize; /* Size of direct color green mask */
	uint8 LinGreenFieldPosition; /* Bit posn of lsb of green mask */
	uint8 LinBlueMaskSize; /* Size of direct color blue mask */
	uint8 LinBlueFieldPosition;    /* Bit posn of lsb of blue mask */
	uint8 LinRsvdMaskSize; /* Size of direct color res mask */
	uint8 LinRsvdFieldPosition;    /* Bit posn of lsb of res mask */
	uint32 MaxPixelClock; /* Maximum pixel clock */

	uint8 Reserved[190]; /* Pad to 256 byte block size */
} vbe_ModeInfoBlock;

/* Flags for combining with video modes during mode set */
#define vbeDontClear 0x8000 /* Dont clear display memory */
#define vbeLinearBuffer 0x4000 /* Enable linear framebuffer mode */
#define vbeRefreshCtrl 0x0800 /* Use refresh rate control */
#define vbeModeMask 0x03FF /* Mask for VBE mode numbers */

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

/* VBE 3.0 CRTC Info Block */
typedef struct vbe_CRTCInfoBlock_struct {
	uint16 HorizontalTotal; /* Horizontal total in pixels */
	uint16 HorizontalSyncStart; /* Horizontal sync start in pixels */
	uint16 HorizontalSyncEnd; /* Horizontal sync end in pixels */
	uint16 VerticalTotal; /* Vertical total in lines */
	uint16 VerticalSyncStart; /* Vertical sync start in lines */
	uint16 VerticalSyncEnd; /* Vertical sync end in lines */
	uint8 Flags; /* Flags (Interlaced, Double Scan etc) */
	uint32 PixelClock; /* Pixel clock in units of Hz */
	uint16 RefreshRate; /* Refresh rate in units of 0.01 Hz */
	uint8 Reserved[40]; /* Remainder of ModeInfoBlock */
} vbe_CRTCInfoBlock;

/* Flags for CRTC Flags */
#define vbeCRTCDoubleScan 0x01 /* Enable double scanned mode */
#define vbeCRTCInterlaced 0x02 /* Enable interlaced mode */
#define vbeCRTCHorizontalSyncNegative 0x04 /* Horizontal sync is negative */
#define vbeCRTCVerticalSyncNegative 0x08 /* Vertical sync is negative */

/* VBE version */
static unsigned vbe_version;

int vbe_detect(vbe_VbeInfoBlock STACK_PTR* info) {
	regs_32 regs;

	memcpy(&info->VESASignature,"VBE2",4);

	regs_32_zero(&regs);
	regs.x.ax = 0x4f00;
	regs.x.es = FP_SEG(info);
	regs.x.di = FP_OFF(info);
	int_32_addr_call(int_10_old,&regs);

	if (regs.x.ax != 0x004f)
		return -1;

	vbe_version = info->VESAVersion;

	return 0;
}

int vbe_mode_info_get(vbe_ModeInfoBlock STACK_PTR* info, unsigned mode) {
	regs_32 regs;

	regs_32_zero(&regs);
	regs.x.ax = 0x4f01;
	regs.x.cx = mode & vbeModeMask;
	regs.x.es = FP_SEG(info);
	regs.x.di = FP_OFF(info);
	int_32_addr_call(int_10_old,&regs);
	if (regs.x.ax != 0x004f)
		return -1;

	return 0;
}

int vbe_scanline_set(unsigned bytes_per_scanline, unsigned pixel_per_scanline) {
	regs_32 regs;

	regs_32_zero(&regs);
	regs.x.ax = 0x4f06;
	if (vbe_version >= 0x200) {
		regs.h.bl = 2;
		regs.x.cx = bytes_per_scanline;
	} else {
		regs.h.bl = 0;
		regs.x.cx = pixel_per_scanline;
	}
	int_32_addr_call(int_10_old,&regs);
	if (regs.x.ax != 0x004f)
		return -1;

	return 0;
}

/***************************************************************************/
/* ADVVBE */

#define MODE_MAX 128

typedef struct mode_info_struct {
	card_crtc param;
	card_mode current;
	card_mode old;
	unsigned effective_mode;
	unsigned reported_mode;
	int require_set_scanline_size;
} mode_info;

static mode_info mode_map[MODE_MAX];

/* number of video mode defined */
static unsigned mode_max;

/* active video mode */
static mode_info* mode_active;

typedef struct driver_info_struct {
	const char* name;
	int require_vbe3;
	int (*detect)(void);
	const char* (*driver)(void);
	int (*set)(const card_crtc STACK_PTR*, const card_mode STACK_PTR*, const card_mode STACK_PTR*);
	void (*reset)(void);
} driver_info;

static driver_info driver_map[]=
{
//	{"laguna", 0, detectLaguna, driverLaguna, setLaguna, resetLaguna}, /* Laguna driver */
	{"3dfx", 0, tdfx_detect, tdfx_driver, tdfx_set, tdfx_reset}, /* 3dfx driver */
	{"savage", 0, savage_detect, savage_driver, savage_set, savage_reset}, /* Savage driver */
	{"sis", 0, sis_detect, sis_driver, sis_set, sis_reset}, /* SiS6326 driver */
//	{"ati", 0, detectATI, driverATI, setATI, resetATI} , /* ATI driver */
//	{"matrox", 0, matrox_detect, matrox_driver, matrox_set, matrox_reset}, /* Matrox driver */
//	{"r128", 0, detectR128, driverR128, setR128, resetR128}, /* R128 driver */
	{"neomagic", 0, neomagic_detect, neomagic_driver, neomagic_set, neomagic_reset}, /* NeoMagic driver */
	{"s3", 0, s3_detect, s3_driver, s3_set, s3_reset}, /* S3 driver */
	{"trident", 0, trident_detect, trident_driver, trident_set, trident_reset}, /* Trident driver */
	{"cirrus", 0, cirrus_detect, cirrus_driver, cirrus_set, cirrus_reset}, /* Cirrus driver */
	{"vbe3", 1, vbe3_detect, vbe3_driver, vbe3_set, vbe3_reset}, /* VBE3 driver */
	{0,0,0,0,0,0}
};

/* active video driver */
static driver_info* driver_active;

/* vbe video mode list */
static uint16 vbe_mode_list[MODE_MAX+1];

static char driver_name[16];

struct mode_number_struct {
	unsigned number;
	unsigned x;
	unsigned y;
	unsigned bits;
};

/* List of the VBE mode */
#define ORI_MODE_MAX 128
static unsigned ori_mode_list[ORI_MODE_MAX];

/* VESA 1.2 default numbering */
static struct mode_number_struct mode_number_map[] = {
{ 0x100,640,400,8 },
{ 0x101,640,480,8 },
{ 0x103,800,600,8 },
{ 0x105,1024,768,8 },
{ 0x107,1280,1024,8 },
{ 0x10D,320,200,15 },
{ 0x10E,320,200,16 },
{ 0x10F,320,200,24 },
{ 0x110,640,480,15 },
{ 0x111,640,480,16 },
{ 0x112,640,480,24 },
{ 0x113,800,600,15 },
{ 0x114,800,600,16 },
{ 0x115,800,600,24 },
{ 0x116,1024,768,15 },
{ 0x117,1024,768,16 },
{ 0x118,1024,768,24 },
{ 0x119,1280,1024,15 },
{ 0x11A,1280,1024,16 },
{ 0x11B,1280,1024,24 },
{0,0,0,0}
};

static unsigned mode_number_generator = VBE_CUSTOM_MODE;

unsigned mode_find_number(unsigned x, unsigned y, unsigned bits) {
	unsigned i;
	unsigned number;
	for(i=0;mode_number_map[i].number;++i)
		if (mode_number_map[i].x==x && mode_number_map[i].y==y && mode_number_map[i].bits==bits)
			break;
	if (!mode_number_map[i].number)
		return mode_number_generator++;
	number = mode_number_map[i].number;
	for(i=0;i<mode_max;++i)
		if (mode_map[i].reported_mode == number)
			break;
	if (i<mode_max)
		return mode_number_generator++;
	return number;
}

static int search_target_mode(unsigned req_x, unsigned req_y, unsigned bits) {
	unsigned smaller_best_mode = 0; /* assignement to prevent warning */
	unsigned smaller_best_size_x = 0; /* assignement to prevent warning */
	unsigned smaller_best_size_y = 0; /* assignement to prevent warning */
	int smaller_best_set = 0;

	unsigned bigger_best_mode = 0; /* assignement to prevent warning */
	unsigned bigger_best_size_x = 0; /* assignement to prevent warning */
	unsigned bigger_best_size_y = 0; /* assignement to prevent warning */
	int bigger_best_set = 0;

	unsigned i = 0;

	while (ori_mode_list[i]!=0xFFFF) {
		unsigned mode;
		vbe_ModeInfoBlock info;

		mode = ori_mode_list[i];

		if (vbe_mode_info_get(&info, mode) == 0
			&& info.BitsPerPixel == bits
			&& info.NumberOfPlanes == 1) {

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
		}

		++i;
	}

	if (smaller_best_set)
		return smaller_best_mode;
	if (bigger_best_set)
		return bigger_best_mode;

	return -1;
}

int mode_insert(mode_info far* mode_ptr, int bits_per_pixel) {
	vbe_ModeInfoBlock info;
	int mode;
	if (mode_max >= MODE_MAX)
		return -1;

	mode_ptr->current.bits_per_pixel = bits_per_pixel;
	mode_ptr->old.bits_per_pixel = bits_per_pixel;
	mode = search_target_mode(mode_ptr->current.width,mode_ptr->current.height,mode_ptr->current.bits_per_pixel);
	if (mode < 0) {
		return -1;
	}

	mode_ptr->effective_mode = mode;
	if (vbe_mode_info_get(&info,mode_ptr->effective_mode)!=0) {
		return -1;
	}

	if (mode_ptr->current.width > info.XResolution) {
		mode_ptr->current.bytes_per_scanline = (mode_ptr->current.width * mode_ptr->current.bytes_per_pixel + 0xF) & ~0xF;
		mode_ptr->require_set_scanline_size = 1;
	} else {
		mode_ptr->current.bytes_per_scanline = info.BytesPerScanLine;
		mode_ptr->require_set_scanline_size = 0;
	}

	mode_ptr->old.width = info.XResolution;
	mode_ptr->old.height = info.YResolution;
	mode_ptr->current.bytes_per_pixel = mode_ptr->old.bytes_per_pixel = (info.BitsPerPixel + 7) / 8;
	mode_ptr->old.bytes_per_scanline = info.BytesPerScanLine;

	mode_ptr->reported_mode = mode_find_number(mode_ptr->current.width,mode_ptr->current.height,mode_ptr->current.bits_per_pixel);

	/* copy */
	memcpy(mode_map + mode_max,mode_ptr,sizeof(mode_info));

	vbe_mode_list[mode_max] = mode_ptr->reported_mode;

	/* next */
	++mode_max;

	return 0;
}

int modeline_load(mode_info far* mode_ptr) {
	char far* s;

	/* skip the name */
	s = strtok(0," \t\n\r");
	if (!s) return -1;

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	if (!(s[0]>='0' && s[0]<='9'))
		return -1;
	mode_ptr->param.dotclockHz = strtold(s,1000000L);
	if (!mode_ptr->param.dotclockHz)
		return -1;

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.HDisp = mode_ptr->current.width = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.HSStart = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.HSEnd = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.HTotal = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.VDisp = mode_ptr->current.height = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.VSStart = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.VSEnd = strtol(s,10);

	s = strtok(0," \t\n\r");
	if (!s) return -1;
	mode_ptr->param.VTotal = strtol(s,10);

	while ((s = strtok(0," \t\n\r"))!=0) {
		if (s[0]=='#')
			break;
		else if (strcmp(s,"doublescan")==0)
			mode_ptr->param.doublescan = 1;
		else if (strcmp(s,"interlaced")==0 || strcmp(s,"interlace")==0)
			mode_ptr->param.interlace = 1;
		else if (strcmp(s,"-hsync")==0)
			mode_ptr->param.hpolarity = 1;
		else if (strcmp(s,"-vsync")==0)
			mode_ptr->param.vpolarity = 1;
	}

	/* convert the format */
	if (mode_ptr->param.doublescan) {
		mode_ptr->param.VDisp *= 2;
		mode_ptr->param.VBStart *= 2;
		mode_ptr->param.VSStart *= 2;
		mode_ptr->param.VSEnd *= 2;
		mode_ptr->param.VBEnd *= 2;
		mode_ptr->param.VTotal *= 2;
	}

	if (mode_ptr->param.interlace) {
		mode_ptr->param.VDisp /= 2;
		mode_ptr->param.VBStart /= 2;
		mode_ptr->param.VSStart /= 2;
		mode_ptr->param.VSEnd /= 2;
		mode_ptr->param.VBEnd /= 2;
		mode_ptr->param.VTotal /= 2;
	}

	mode_ptr->param.interlaceratio = 50;

	/* blank */
	mode_ptr->param.HBStart = mode_ptr->param.HDisp + 1;
	if (mode_ptr->param.HBStart > mode_ptr->param.HSStart)
		mode_ptr->param.HBStart = mode_ptr->param.HSStart;
	mode_ptr->param.HBEnd = mode_ptr->param.HSEnd + 1;
	mode_ptr->param.VBStart = mode_ptr->param.VDisp + 1;
	if (mode_ptr->param.VBStart > mode_ptr->param.VSStart)
		mode_ptr->param.VBStart = mode_ptr->param.VSStart;
	mode_ptr->param.VBEnd = mode_ptr->param.VSEnd + 1;

	return 0;
}

int mode_load(const char far* file) {
	char buffer[256];
	unsigned i;

	int handle = open(file,O_RDONLY);
	if (handle == -1) {
		cputs("Configuration file ");
		cputs(file);
		cputs(" not found\r\n");
		return 0;
	}

	mode_max = 0;

	while (gets(handle, buffer, sizeof(buffer))) {
		char far* s = strtok(buffer," \t\n\r");

		if (s && strcmp(s,"device_video_modeline")==0) {
			mode_info info;
			memset(&info,0,sizeof(mode_info));
			if (modeline_load(&info)==0) {
				mode_insert(&info,8);
				mode_insert(&info,15);
				mode_insert(&info,16);
			}
		} else if (s && strcmp(s,"device_video")==0) {
			s = strtok(0," \t\n\r");
			while (s) {
				if (memcmp(s,"vbeline/",8)==0) {
					strcpy(driver_name,s+8);
				}
				s = strtok(0," \t\n\r");
			}
		}
	}

	close(handle);

	/* sort of vbe_mode_list */
	for(i=1;i<mode_max;++i) {
		unsigned j;
		for(j=mode_max-1;j>=i;--j) {
			if (vbe_mode_list[j-1]>vbe_mode_list[j]) {
				unsigned t = vbe_mode_list[j-1];
				vbe_mode_list[j-1] = vbe_mode_list[j];
				vbe_mode_list[j] = t;
			}
		}
	}

	/* end of the list marker */
	vbe_mode_list[mode_max] = 0xFFFF;

	if (!mode_max) {
		cputs("No modelines found.\r\n");
		return 0;
	}

	return 1;
}

void mode_print(void) {
	unsigned i;

	cputs("\r\nAvailable modes:\r\n");
	for(i=0;i<mode_max;++i) {
		unsigned v;

		cputs("        ");
		cputu(mode_map[i].reported_mode,4,' ',16);
		cputs("h ");
		cputu(mode_map[i].current.width,4,' ',10);
		cputs("x");
		cputu(mode_map[i].current.height,4,' ',10);
		cputs(" ");
		cputu(mode_map[i].current.bits_per_pixel,2,' ',10);
		cputs(" bit ");

		v = mode_map[i].param.dotclockHz / (mode_map[i].param.HTotal * 100L);
		cputu(v / 10,2,' ',10);
		cputs(".");
		cputu(v % 10,1,' ',10);
		cputs(" kHz/");

		v = 10L * mode_map[i].param.dotclockHz / (mode_map[i].param.HTotal * (long)mode_map[i].param.VTotal);
		cputu(v / 10,2,' ',10);
		cputs(".");
		cputu(v % 10,1,' ',10);
		cputs(" Hz ");

		cputs("\r\n");
	}
}

int vbe_init(const char far* file) {
	vbe_VbeInfoBlock info;
	unsigned i;

	mode_active = 0;

	if (vbe_detect(&info)!=0) {
		cputs("VBE BIOS not found!\r\n");
		return 0;
	}

	/* save the mode list */
	for(i=0;i<ORI_MODE_MAX-1 && info.VideoModePtr[i]!=0xFFFF;++i) {
		ori_mode_list[i] = info.VideoModePtr[i];
	}
	ori_mode_list[i] = 0xFFFF;

	if (!mode_load(file)) {
		return 0;
	}

	for(driver_active = driver_map;driver_active->name;++driver_active) {
		if (!driver_active->require_vbe3 || vbe_version >= 0x300) {
			if (!*driver_name || strcmp(driver_name,"auto")==0 || strcmp(driver_name,driver_active->name)==0) {
				if (driver_active->detect()) {
					break;
				}
			}
		}
	}

	if (!driver_active->name) {
		driver_active = 0;
		if (*driver_name) {
			cputs("Video board not supported.\r\nTry removing the `device_video vbeline/");
			cputs(driver_name);
			cputs("' option from the configuration file.\r\n");
		} else {
			cputs("Video board not supported.\r\n");
		}
		return 0;
	}

	return 1;
}

void vbe_done(void) {
	if (driver_active) {
		driver_active->reset();
		driver_active = 0;
	}
}

void far* oem_alloc(uint8 far * far * base, void far* data, unsigned size) {
	uint8 far* r = *base;
	memcpy(r,data,size);
	*base += size;
	return r;
}

#ifdef VBE_LOW_DATA
static uint8 buffer_data[256];
#endif

void vbe_service_4f00(regs_32 far * regs) {
	vbe_VbeInfoBlock far * info = MK_FP(regs->x.es,regs->x.di);
	int vbe2 = memcmp(info->VESASignature,"VBE2",4)==0;
	int_32_addr_call(int_10_old,regs);
	if (regs->x.ax == 0x004f) {
#ifdef VBE_LOW_DATA
		uint8 far* base_data = buffer_data;
#else
		uint8 far* base_data = info->OemDATA;
#endif
		if (vbe2) {
			info->VideoModePtr = (uint16 far*)oem_alloc(&base_data,&vbe_mode_list,2*(mode_max+1));
			info->OemStringPtr = (char far*)oem_alloc(&base_data,OEM_STR,strlen(OEM_STR)+1);
			if (info->VESAVersion >= 0x200) {
				info->OemSoftwareRev = OEM_VERSION_NUM;
				info->OemVendorNamePtr = (char far*)oem_alloc(&base_data,OEM_VENDOR_STR,strlen(OEM_VENDOR_STR)+1);
				info->OemProductNamePtr = (char far*)oem_alloc(&base_data,OEM_PRODUCT_STR,strlen(OEM_PRODUCT_STR)+1);
				info->OemProductRevPtr = (char far*)oem_alloc(&base_data,OEM_VERSION_STR,strlen(OEM_VERSION_STR)+1);
			}
		} else {
			info->VideoModePtr = (uint16 far*)&vbe_mode_list;
			info->OemStringPtr = OEM_STR;
		}
		/* max supported is VBE 2.0 */
		if (info->VESAVersion > 0x200)
			info->VESAVersion = 0x200;
	}
}

void vbe_service_4f01(regs_32 far* regs) {
	vbe_ModeInfoBlock far* info;
	mode_info* mode_ptr;

	/* VGA mode */
	if ((regs->x.cx & 0x100) == 0) {
		int_32_addr_call(int_10_old,regs);
		return;
	}

	for(mode_ptr = mode_map;mode_ptr!=mode_map + mode_max;++mode_ptr)
		if (mode_ptr->reported_mode == regs->x.cx)
			break;
	if (mode_ptr == mode_map + mode_max) {
		regs->x.ax = 0x014f;
		return;
	}

	info = MK_FP(regs->x.es,regs->x.di);

	regs->x.cx = mode_ptr->effective_mode;
	int_32_addr_call(int_10_old,regs);
	regs->x.cx = mode_ptr->reported_mode;

	if (regs->x.ax != 0x004f)
		return;

	info->XResolution = mode_ptr->current.width;
	info->YResolution = mode_ptr->current.height;
	info->BytesPerScanLine = mode_ptr->current.bytes_per_scanline;
}

int vbe_service_4f02_preset(const card_crtc STACK_PTR* ccp, vbe_ModeInfoBlock STACK_PTR* info, vbe_CRTCInfoBlock STACK_PTR* crtc) {
	if ((info->ModeAttributes & vbeMdAvailable) == 0) {
		CARD_LOG(("vbeline: VBE report that the selected mode isn't avaliable\n"));
		return -1;
	}

	if (ccp->interlace && (info->ModeAttributes & vbeMdInterlace)==0) {
		CARD_LOG(("vbeline: VBE report that the selected mode doesn't support interlace\n"));
		return -1;
	}

	if (ccp->doublescan && (info->ModeAttributes & vbeMdDoubleScan)==0) {
		CARD_LOG(("vbeline: VBE report that the selected mode doesn't support doublescan\n"));
		return -1;
	}

	if (ccp->interlace && ccp->doublescan) {
		CARD_LOG(("vbeline: VBE doesn't support interlaced and doublescan at the same time\n"));
		return -1;
	}

	memset(crtc,0,sizeof(vbe_CRTCInfoBlock));

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
	crtc->RefreshRate = (long)crtc->PixelClock * 100 / ((long)ccp->HTotal * ccp->VTotal);

	CARD_LOG(("vbeline: mode_set_vbe3 %dx%d (%04x %.2f %d %d %d %d %d %d)\n",(unsigned)info->XResolution,(unsigned)info->YResolution,(unsigned)mode,(double)crtc->PixelClock/1E6,(unsigned)crtc->HorizontalSyncStart,(unsigned)crtc->HorizontalSyncEnd,(unsigned)crtc->HorizontalTotal,(unsigned)crtc->VerticalSyncStart,(unsigned)crtc->VerticalSyncEnd,(unsigned)crtc->VerticalTotal));

	return 0;
}

void vbe_service_4f02(regs_32 far* regs) {
	mode_info* mode_ptr;
	unsigned req_mode;
	unsigned req_flag;

	/* VGA mode */
	if ((regs->x.bx & 0x100) == 0) {
		mode_active = 0;
		int_32_addr_call(int_10_old,regs);
		return;
	}

	req_mode = regs->x.bx & vbeModeMask;
	req_flag = regs->x.bx & ~vbeModeMask;

	for(mode_ptr = mode_map;mode_ptr!=mode_map + mode_max;++mode_ptr)
		if (mode_ptr->reported_mode == req_mode)
			break;
	if (mode_ptr == mode_map + mode_max) {
		regs->x.ax = 0x014f;
		return;
	}

	/* active mode */
	mode_active = mode_ptr;

	if (driver_active->require_vbe3) {
		STACK_DECL vbe_CRTCInfoBlock crtc;
		STACK_DECL vbe_ModeInfoBlock info;

		if (vbe_mode_info_get(&info,mode_active->effective_mode)!=0) {
			mode_active = 0;
			regs->x.ax = 0x014f;
			return;
		}

		if (vbe_service_4f02_preset(&mode_active->param,&info,&crtc)!=0) {
			mode_active = 0;
			regs->x.ax = 0x014f;
			return;
		}

		regs->x.bx = mode_active->effective_mode | req_flag | vbeRefreshCtrl;
		regs->x.es = FP_SEG(&crtc);
		regs->x.di = FP_OFF(&crtc);

		int_32_addr_call(int_10_old,regs);
	} else {
		regs->x.bx = mode_active->effective_mode | req_flag;
		int_32_addr_call(int_10_old,regs);
	}

	regs->x.bx = req_mode | req_flag;

	if (regs->x.ax != 0x004f) {
		mode_active = 0;
		return;
	}

	if (mode_active->require_set_scanline_size) {
		if (vbe_scanline_set(mode_active->current.bytes_per_scanline,mode_active->current.width)!=0) {
			mode_active = 0;
			regs->x.ax = 0x014f;
			return;
		}
	}

	driver_active->set(&mode_active->param,&mode_active->current,&mode_active->old);
}

void vbe_service_4f03(regs_32 far * regs) {
	regs->x.bx = 0;	/* this solves some problems on my S3 */

	int_32_addr_call(int_10_old,regs);
	if (regs->x.ax != 0x004f)
		return;

	if (mode_active && regs->x.bx == mode_active->effective_mode)
		regs->x.bx = mode_active->reported_mode;
}

/***************************************************************************/
/* calls */

#define VBE_SERVICE_CALL_MAX 16

static unsigned long vbe_service_call[VBE_SERVICE_CALL_MAX];

static const char* vbe_service_name[VBE_SERVICE_CALL_MAX] = {
/* 00 */ "Return VBE Controller Information",
/* 01 */ "Return VBE Mode Information",
/* 02 */ "Set VBE Mode",
/* 03 */ "Return current VBE Mode",
/* 04 */ "Save/Restore state",
/* 05 */ "Display Window Control",
/* 06 */ "Set/Get Logical Scan Line Length",
/* 07 */ "Set/Get Display Start",
/* 08 */ "Set/Get DAC Palette Format",
/* 09 */ "Set/Get Palette Data",
/* 0A */ "Return VBE Protected Mode Interface",
/* 0B */ /* "Get/Set Pixel Clock" */ 0, /* Only VBE3 */
0,
0,
0,
0
};

/***************************************************************************/
/* trace */

#ifndef NDEBUG
#define VBE_TRACE_MAX 128

/* trace */
struct trace {
	unsigned in_ax;
	unsigned in_bx;
	unsigned in_cx;
	unsigned in_dx;
	unsigned out_ax;
	unsigned out_bx;
};

static struct trace vbe_trace_map[VBE_TRACE_MAX];
static unsigned vbe_trace_mac = 0;
#endif

/***************************************************************************/
/* stack */

/* private stack */
#define STACK_SIZE (1024+512)
static uint8 stack_private[STACK_SIZE];
static uint16 stack_old_ss;
static uint16 stack_old_sp;

#ifndef NDEBUG
static unsigned stack_private_min = STACK_SIZE;

void debug_stack_clear(void) {
	memset(stack_private,0,sizeof(stack_private));
}

void debug_stack_check(void) {
	unsigned i;
	for(i=0;i<STACK_SIZE;++i)
		if (stack_private[i])
			break;
	if (i<stack_private_min)
		stack_private_min = i;
}
#endif

/***************************************************************************/
/* trap */

/* used for temporaney storing the regs pointer in int_10_trap */
static regs_32 far * int_10_regs;

void int_10_trap_internal(void) {
	if (int_10_regs->h.al < VBE_SERVICE_CALL_MAX)
		++vbe_service_call[int_10_regs->h.al];

#ifndef NDEBUG
	if (vbe_trace_mac < VBE_TRACE_MAX) {
		vbe_trace_map[vbe_trace_mac].in_ax = int_10_regs->x.ax;
		vbe_trace_map[vbe_trace_mac].in_bx = int_10_regs->x.bx;
		vbe_trace_map[vbe_trace_mac].in_cx = int_10_regs->x.cx;
		vbe_trace_map[vbe_trace_mac].in_dx = int_10_regs->x.dx;
	}
#endif

	switch (int_10_regs->x.ax) {
		case 0x4f00 : vbe_service_4f00(int_10_regs); break;
		case 0x4f01 : vbe_service_4f01(int_10_regs); break;
		case 0x4f02 : vbe_service_4f02(int_10_regs); break;
		case 0x4f03 : vbe_service_4f03(int_10_regs); break;
		default:
			int_32_addr_call(int_10_old,int_10_regs);
	}

#ifndef NDEBUG
	if (vbe_trace_mac < VBE_TRACE_MAX) {
		vbe_trace_map[vbe_trace_mac].out_ax = int_10_regs->x.ax;
		vbe_trace_map[vbe_trace_mac].out_bx = int_10_regs->x.bx;
		vbe_trace_mac++;
	}
#endif
}

void int_10_trap(regs_32 far * regs) {
#ifndef NDEBUG
	asm pushad
	debug_stack_clear();
#endif

	/* this global variable is mandatory because the pointer is in the */
	/* stack but now we are changing it */
	int_10_regs = regs;

	/* save the stack */
	asm mov ax,ss
	asm mov bx,sp
	asm mov [stack_old_ss],ax
	asm mov [stack_old_sp],bx

	/* change the stack */
	_BX = (FP_OFF(stack_private) + sizeof(stack_private) - 2) & 0xFFFE;
	asm mov ax,ds
	asm mov ss,ax
	asm mov sp,bx

	int_10_trap_internal();

	/* restore the stack */
	asm mov ax,[stack_old_ss]
	asm mov bx,[stack_old_sp]
	asm mov ss,ax
	asm mov sp,bx

#ifndef NDEBUG
	debug_stack_check();
	asm popad
#endif
}

/***************************************************************************/
/* TSR */

uint16 TSR_RUTACK1 = 0xAD17;
uint16 TSR_RUTACK2 = 0x17BE;

int far _cdecl tsr_remote(void far * arg) {
	(void)arg;
	return 0;
}

uint16 _pascal tsr(int (*remote)(void far* arg), char far* args) {
	int arg_unload = 0;
	int arg_load = 0;
	char far* tok;
	char config[128];

	strcpy(config,"vbe.rc");

	tok = strtok(args," \t");
	while (tok) {
		if (optionmatch(tok,"l")) {
			arg_load = 1;
		} else if (optionmatch(tok,"u")) {
			arg_unload = 1;
		} else if (optionmatch(tok,"c")) {
			tok = strtok(0," \t");
			if (!tok) {
				cputs("Missing config file path\r\n");
				return TSR_FAILURE;
			}
			strcpy(config,tok);
		} else {
			cputs("Unknown option ");
			cputs(tok);
			cputs("\r\n");
			return TSR_FAILURE;
		}
		tok = strtok(0," \t");
	}

	if (arg_unload + arg_load > 1) {
		cputs("Incompatible commands\n\r");
		return TSR_FAILURE;
	}

	if (arg_unload) {
		if (remote == 0) {
			cputs(OEM_PRODUCT_STR " already unloaded\n\r");
			return TSR_FAILURE;
		}
		return TSR_UNLOAD;
	}

	if (arg_load) {
		if (remote != 0) {
			cputs(OEM_PRODUCT_STR " already loaded\n\r");
			return TSR_FAILURE;
		}

		if (!vbe_init(config))
			return TSR_FAILURE;

		return TSR_LOAD;
	}

	cputs(OEM_PRODUCT_STR " by " OEM_VENDOR_STR " v" OEM_VERSION_STR "\r\n");
	cputs(
"Usage:\n\r"
"    vbe [/l] [/u] [/c CONFIG]\n\r"
"Commands:\n\r"
"    /l         Load the TSR\n\r"
"    /u         Unload the TSR\n\r"
"Options:\n\r"
"    /c CONFIG  Use this config file instead of vbe.rc\n\r"
);

	return TSR_SUCCESS;
}

uint16 _pascal tsr_init_0(void) {
	cputs(OEM_PRODUCT_STR " by " OEM_VENDOR_STR " v" OEM_VERSION_STR "\r\n");

	cputs("\r\nUsing driver `");
	cputs(driver_active->name);
	cputs("' for `");
	cputs(driver_active->driver());
	cputs("'\r\n");

	mode_print();

	if (vbe_version < 0x200) {
		cputs("\r\nYou have a VBE version prior of 2.0. Probably you want\r\n");
		cputs("to install a software VBE BIOS before AdvanceVBE\r\n");
	}

	return TSR_SUCCESS;
}

uint16 _pascal tsr_init_1(void) {
	return TSR_SUCCESS;
}

void _pascal tsr_done_0(void) {
#ifndef NDEBUG
	cputs("stack_free_space ");
	cputu(stack_private_min,0,' ',10);
	cputs("\r\n");
#endif
	vbe_done();
}

void _pascal tsr_done_1(void) {
	unsigned i;

	cputs(OEM_PRODUCT_STR " unloaded\r\n");

	for(i=0;i<VBE_SERVICE_CALL_MAX;++i)
		if (vbe_service_call[i])
			break;

	if (i < VBE_SERVICE_CALL_MAX) {
		cputs("\r\nVBE calls:\r\n");
		for(i=0;i<VBE_SERVICE_CALL_MAX;++i) {
			if (vbe_service_call[i] || vbe_service_name[i]) {
				cputs("        4F");
				cputu(i,2,'0',16);
				cputs("h ");
				cputu(vbe_service_call[i],6,' ',10);
				if (vbe_service_name[i]) {
					cputs(" ");
					cputs(vbe_service_name[i]);
				}
				cputs("\r\n");
			}
		}
	}

#ifndef NDEBUG
	cputs("\r\n\r\nVBE trace:\r\n");
	for(i=0;i<vbe_trace_mac;++i) {
		cputs("        ");
		cputu(vbe_trace_map[i].in_ax,4,'0',16);
		cputs("h ");
		cputu(vbe_trace_map[i].in_bx,4,'0',16);
		cputs("h ");
		cputu(vbe_trace_map[i].in_cx,4,'0',16);
		cputs("h ");
		cputu(vbe_trace_map[i].in_dx,4,'0',16);
		cputs("h - ");
		cputu(vbe_trace_map[i].out_ax,4,'0',16);
		cputs("h ");
		cputu(vbe_trace_map[i].out_bx,4,'0',16);
		cputs("h ");
		cputs("\r\n");
	}
#endif
}