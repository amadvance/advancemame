/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* ARK driver written by Harm Hanemaayer. */

/*
 * Jan 1995:
 * Initial ARK driver. Should at least provide 25.175 and 28.322 MHz
 * dot clocks.
 * Sep 1996: Support 24bpp, but colors are wrong on ARK1000PV with AT&T
 *	     DAC. Recognize ARK2000MT.
 */

/* #define INCLUDE_HALVED_CLOCKS */

#ifdef INCLUDE_HALVED_CLOCKS
#define SVGA_STYLE_320x200x256
#endif

/* Use the COP in coordinate mode. */
/* #define COORDINATES */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"

#include "timing.h"
#include "ramdac/ramdac.h"
#include "vgaregs.h"
#include "interfac.h"
#include "accel.h"


/* Extended registers. */

#define ARK_SR10	VGA_TOTAL_REGS + 0
#define ARK_SR11	VGA_TOTAL_REGS + 1
#define ARK_SR12	VGA_TOTAL_REGS + 2
#define ARK_SR13	VGA_TOTAL_REGS + 3
#define ARK_SR14	VGA_TOTAL_REGS + 4
#define ARK_SR15	VGA_TOTAL_REGS + 5
#define ARK_SR16	VGA_TOTAL_REGS + 6
#define ARK_SR17	VGA_TOTAL_REGS + 7
#define ARK_SR18	VGA_TOTAL_REGS + 8
#define ARK_SR19	VGA_TOTAL_REGS + 9
#define ARK_SR1C	VGA_TOTAL_REGS + 10
#define ARK_SR1D	VGA_TOTAL_REGS + 11
#define ARK_CR40	VGA_TOTAL_REGS + 12
#define ARK_CR41	VGA_TOTAL_REGS + 13
#define ARK_CR42	VGA_TOTAL_REGS + 14
#define ARK_CR44	VGA_TOTAL_REGS + 15
#define ARK_CR46	VGA_TOTAL_REGS + 16
#define ARK_CR50	VGA_TOTAL_REGS + 17
#define ARK_PELMASK	VGA_TOTAL_REGS + 18

#define ARK_MEMORY_CONTROL		ARK_SR10
#define ARK_VIDEO_CLOCK_SELECT		ARK_SR11
#define ARK_VLBUS_CONTROL		ARK_SR12
#define ARK_PAGE_ADDRESS_LOW		ARK_SR13
#define ARK_PAGE_ADDRESS_HIGH		ARK_SR14
#define ARK_APERTURE_WRITE_INDEX	ARK_SR15
#define ARK_APERTURE_READ_INDEX		ARK_SR16
#define ARK_COP_FRAMEBUFFER_PITCH	ARK_SR17	/* Unused */
#define ARK_DISPLAY_FIFO_CONTROL	ARK_SR18
#define ARK_HOST_INTERFACE_TYPE		ARK_SR19	/* Read */
#define ARK_DPMS_CONTROL		ARK_SR1C
#define ARK_LOCK_REGISTER		ARK_SR1D
#define ARK_CRTC_VERTICAL_OVERFLOW	ARK_CR40
#define ARK_CRTC_HORIZONTAL_OVERFLOW	ARK_CR41
#define ARK_INTERLACE_RETRACE		ARK_CR42
#define ARK_VGA_ENHANCEMENT		ARK_CR44
#define ARK_PIXEL_CLOCK_CONTROL		ARK_CR46
#define ARK_CHIP_ID			ARK_CR50	/* Read */

#define ARK_DAC_OFFSET	VGA_TOTAL_REGS + 19
#define ARK_TOTAL_REGS	VGA_TOTAL_REGS + 19 + 10


#define ARK1000PV	0
#define ARK2000PV	1

#define VL	0
#define PCI	1

static int ark_chip;
static int ark_memory;
static int ark_bus;
static int ark_baseaddress, is_linear;	/* PCI base address. */
static CardSpecs *cardspecs;
static DacMethods *dac_used;
static int dac_speed = 0;

static int ark_init(int, int, int);


static void nothing(void)
{
}


/* Unlock. */

static void ark_unlock(void)
{
    /* Set bit 0 of SR1D. */
    __svgalib_outSR(0x1D, __svgalib_inSR(0x1D) | 0x01);
    /*
     * Also enable writing to CRTC 0-7. This lock seems to have side effects
     * on some extended registers on the ARK1000PV!
     */
    __svgalib_outCR(0x11, __svgalib_inCR(0x11) & ~0x80);
}

/* Fill in chipset specific mode information */

static void ark_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
#ifdef SVGA_STYLE_320x200x256
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	&& mode != G320x200x256) {
#else
    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
#endif
	__svgalib_vga_driverspecs.getmodeinfo(mode, modeinfo);
	return;
    }
    switch (modeinfo->colors) {
    case 16:			/* 4-plane 16 color mode */
	modeinfo->maxpixels = 65536 * 8;
	break;
    default:
	modeinfo->maxpixels = ark_memory * 1024 /
	    modeinfo->bytesperpixel;
	break;
    }
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = 0x1fffff;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;
    modeinfo->flags |= CAPABLE_LINEAR;
    if ((__svgalib_inSR(0x12) & 0x03) != 0)
	modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
}


/* Return non-zero if mode is available */

static int ark_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeInfo *modeinfo;
    ModeTiming *modetiming;

#ifdef SVGA_STYLE_320x200x256
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	&& mode != G320x200x256)
#else
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
#endif
	return __svgalib_vga_driverspecs.modeavailable(mode);

    /* Enough memory? */
    info = &__svgalib_infotable[mode];
    if (ark_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 0;
    }
    free(modetiming);
    free(modeinfo);

    return SVGADRV;
}


static int ark_saveregs(unsigned char regs[])
{
    ark_unlock();

    /* Save extended registers. */
    regs[ARK_SR10] = __svgalib_inSR(0x10);
    regs[ARK_SR11] = __svgalib_inSR(0x11);
    regs[ARK_SR12] = __svgalib_inSR(0x12);
    regs[ARK_SR13] = __svgalib_inSR(0x13);
    regs[ARK_SR14] = __svgalib_inSR(0x14);
    regs[ARK_SR15] = __svgalib_inSR(0x15);
    regs[ARK_SR16] = __svgalib_inSR(0x16);
    regs[ARK_SR18] = __svgalib_inSR(0x18);
    regs[ARK_SR19] = __svgalib_inSR(0x19);
    regs[ARK_SR1C] = __svgalib_inSR(0x1C);
    regs[ARK_SR1D] = __svgalib_inSR(0x1D);

    regs[ARK_CR40] = __svgalib_inCR(0x40);
    regs[ARK_CR41] = __svgalib_inCR(0x41);
    regs[ARK_CR42] = __svgalib_inCR(0x42);
    regs[ARK_CR44] = __svgalib_inCR(0x44);
    regs[ARK_CR46] = __svgalib_inCR(0x46);
    regs[ARK_CR50] = __svgalib_inCR(0x50);

    port_in(0x3C8);
    regs[ARK_PELMASK] = port_in(0x3C6);

    dac_used->saveState(regs + ARK_DAC_OFFSET);
    return ARK_DAC_OFFSET - VGA_TOTAL_REGS + dac_used->stateSize;
}


/* Set chipset-specific registers */

static void ark_setregs(const unsigned char regs[], int mode)
{
    ark_unlock();

    /* Write extended registers. */

    __svgalib_outCR(0x46, __svgalib_inCR(0x46) | 0x20);	/* Disable Clock Select Latch. */

    __svgalib_outSR(0x10, regs[ARK_SR10]);
    __svgalib_outSR(0x11, regs[ARK_SR11]);
    __svgalib_outSR(0x12, regs[ARK_SR12]);
    __svgalib_outSR(0x13, regs[ARK_SR13]);
    __svgalib_outSR(0x14, regs[ARK_SR14]);
    __svgalib_outSR(0x15, regs[ARK_SR15]);
    __svgalib_outSR(0x16, regs[ARK_SR16]);
    __svgalib_outSR(0x18, regs[ARK_SR18]);
    __svgalib_outSR(0x1C, regs[ARK_SR1C]);
    __svgalib_outSR(0x1D, regs[ARK_SR1D]);

    __svgalib_outCR(0x40, regs[ARK_CR40]);
    __svgalib_outCR(0x41, regs[ARK_CR41]);
    __svgalib_outCR(0x42, regs[ARK_CR42]);
    __svgalib_outCR(0x44, regs[ARK_CR44]);

    port_in(0x3C8);
    port_out_r(0x3C6, regs[ARK_PELMASK]);
    dac_used->restoreState(regs + ARK_DAC_OFFSET);

    __svgalib_outCR(0x46, regs[ARK_CR46]);
}


/*
 * Initialize register state for a mode.
 */

static void ark_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo)
{

    /* Get current values. */


    is_linear=0;
    ark_saveregs(moderegs);

    /* Set up the standard VGA registers for a generic SVGA. */
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    /* Set up the extended register values, including modifications */
    /* of standard VGA registers. */

    moderegs[VGA_CR13] = modeinfo->lineWidth >> 3;
/*      moderegs[VGA_CR14] = 0x40; */
/*      moderegs[VGA_CR17] = 0xA3; */
/*      moderegs[VGA_AR10] = 0x01; */
    moderegs[VGA_AR11] = 0x00;
    /* The following is actually a RAMDAC register. */
    moderegs[ARK_PELMASK] = 0xFF;

    moderegs[ARK_VIDEO_CLOCK_SELECT] &= ~0x0F;
    /* Set COP and Giant Shift pixel depth. */
    if (modeinfo->bytesPerPixel == 1)
	moderegs[ARK_VIDEO_CLOCK_SELECT] |= 0x6;
    if (modeinfo->bytesPerPixel == 2)
	moderegs[ARK_VIDEO_CLOCK_SELECT] |= 0xA;
    if (modeinfo->bytesPerPixel == 3) {
	moderegs[ARK_VIDEO_CLOCK_SELECT] |= 0x6;
//        moderegs[ARK_DPMS_CONTROL] &=0xe7;
//        moderegs[ARK_DPMS_CONTROL] |=0x18;
        
    }
    if (modeinfo->bytesPerPixel == 4)
	moderegs[ARK_VIDEO_CLOCK_SELECT] |= 0xE;

/* Framebuffer control. */
    /* Enable VESA Super VGA memory organisation. */
    /* Also enable Linear Addressing and COP (seems to be required). */
    moderegs[ARK_MEMORY_CONTROL] &= ~0x1F;
    moderegs[ARK_MEMORY_CONTROL] |= 0x1F;
    /* Map aperture at 0xA0000. */
    moderegs[ARK_PAGE_ADDRESS_LOW] = 0x0A;
    moderegs[ARK_PAGE_ADDRESS_HIGH] = 0x00;
    /* The following register is not VLB-specific, despite its name. */
    /* Set 64K aperture. */
    moderegs[ARK_VLBUS_CONTROL] &= ~0x03;
    moderegs[ARK_APERTURE_WRITE_INDEX] = 0;
    moderegs[ARK_APERTURE_READ_INDEX] = 0;

/* Display FIFO. */
    {
	int threshold;
	unsigned char val;
	threshold = 4;		/* A guess. */
	val = moderegs[ARK_DISPLAY_FIFO_CONTROL];
	if (ark_chip == ARK1000PV) {
	    val |= 0x08;	/* Enable full FIFO. */
	    val &= ~0x07;
	    val |= threshold;
	}
	if (ark_chip == ARK2000PV) {
            threshold=12;
	    val &= 0x40;
	    val |= 0x10;	/* 32-deep FIFO. */
	    val |= (threshold & 0x0E) >> 1;
	    if (threshold & 0x01)
		val |= 0x80;
	    if (threshold & 0x10)
		val |= 0x20;
	}
	moderegs[ARK_DISPLAY_FIFO_CONTROL] = val;
    }

/* CRTC timing. */
    {
	unsigned char val;
	/* Vertical Overflow. */
	val = 0;
	if ((modetiming->CrtcVTotal - 2) & 0x400)
	    val |= 0x80;
	if ((modetiming->CrtcVDisplay - 1) & 0x400)
	    val |= 0x40;
	/* VBlankStart is equal to VSyncStart + 1. */
	if (modetiming->CrtcVSyncStart & 0x400)
	    val |= 0x20;
	/* VRetraceStart is equal to VSyncStart + 1. */
	if (modetiming->CrtcVSyncStart & 0x400)
	    val |= 0x10;
	moderegs[ARK_CRTC_VERTICAL_OVERFLOW] = val;

	/* Horizontal Overflow. */
	val = moderegs[ARK_CRTC_HORIZONTAL_OVERFLOW];
	val &= 0x07;
	if ((modetiming->CrtcHTotal / 8 - 5) & 0x100)
	    val |= 0x80;
	if ((modetiming->CrtcHDisplay / 8 - 1) & 0x100)
	    val |= 0x40;
	/* HBlankStart is equal to HSyncStart - 1. */
	if ((modetiming->CrtcHSyncStart / 8 - 1) & 0x100)
	    val |= 0x20;
	/* HRetraceStart is equal to HSyncStart. */
	if ((modetiming->CrtcHSyncStart / 8) & 0x100)
	    val |= 0x10;
	if ((modeinfo->lineWidth / 8) & 0x100)
	    val |= 0x08;
	moderegs[ARK_CRTC_HORIZONTAL_OVERFLOW] = val;
    }
    /* No interlace, standard character clock. */
    moderegs[ARK_VGA_ENHANCEMENT] &= ~0x34;
    /* Enable RAMDAC access. */
    moderegs[ARK_VGA_ENHANCEMENT] &= ~0x01;
    if (modetiming->flags & INTERLACED) {
	/* Set mid-scan vertical retrace start. */
	moderegs[ARK_INTERLACE_RETRACE] =
	    (modetiming->CrtcHTotal / 8 - 5) / 2;
	moderegs[ARK_VGA_ENHANCEMENT] |= 0x04;	/* Interlaced. */
    }
/* Clocking. */
    /* Select 8 or 16-bit video output to RAMDAC on 2000PV. */
    if (ark_chip == ARK2000PV) {
	int dac16;
	moderegs[ARK_PIXEL_CLOCK_CONTROL] &= ~0x04;	/* 8-bit */
	dac16 = 0;
	if (modeinfo->bitsPerPixel == 8 &&
	    cardspecs->mapClock(8, modetiming->pixelClock)
	    == modetiming->pixelClock / 2)
	    /* Typically high resolution 8bpp (> 110 MHz). */
	    dac16 = 1;
	if (modeinfo->bitsPerPixel == 16 &&
	    cardspecs->mapClock(16, modetiming->pixelClock)
	    == modetiming->pixelClock)
	    /* 16bpp at pixel rate. */
	    dac16 = 1;
	/* Note: with an 8-bit DAC, 16bpp is Clock * 2. */
	/* 24bpp is always Clock * 3. */
	if (modeinfo->bitsPerPixel == 32 &&
	    cardspecs->mapClock(32, modetiming->pixelClock)
	    == modetiming->pixelClock * 2)
	    /* 32bpp at Clock * 2. */
	    dac16 = 1;
	/* Note: with an 8-bit dac, 32bpp is Clock * 4. */
	if (dac16)
	    moderegs[ARK_PIXEL_CLOCK_CONTROL] |= 0x04;	/* 16-bit */
    }
    if (modetiming->flags & USEPROGRCLOCK) 
        if(dac_used==&__svgalib_ICS_GENDAC_methods) {
	    moderegs[VGA_MISCOUTPUT] &= ~0x0C;
	    moderegs[VGA_MISCOUTPUT] |= 0x08;
        } else moderegs[VGA_MISCOUTPUT] |= 0x0C;	/* External clock select. */
    else {
	/* Program clock select. */
	moderegs[VGA_MISCOUTPUT] &= ~0x0C;
	moderegs[VGA_MISCOUTPUT] |=
	    (modetiming->selectedClockNo & 3) << 2;
	moderegs[ARK_VIDEO_CLOCK_SELECT] &= ~0xC0;
	moderegs[ARK_VIDEO_CLOCK_SELECT] |=
	    (modetiming->selectedClockNo & 0xC) << 4;
#ifdef INCLUDE_HALVED_CLOCKS
	if (modetiming->selectedClockNo & 0x10)
	    /* Set VGA divide clock by 2 bit. */
	    moderegs[VGA_SR1] |= 0x08;
#endif
    }

    if (dac_used->id != NORMAL_DAC) {
	dac_used->initializeState(&moderegs[ARK_DAC_OFFSET],
				  modeinfo->bitsPerPixel,
			   __svgalib_colorbits_to_colormode(modeinfo->bitsPerPixel,
						  modeinfo->colorBits),
				  modetiming->pixelClock);
    }
}


/* Set a mode */

static void init_acceleration_specs_for_mode(AccelSpecs * accelspecs, int bpp,
					     int width_in_pixels);

static void arkaccel_init(AccelSpecs * accelspecs, int bpp,
			  int width_in_pixels);

static int ark_setmode(int mode, int prv_mode)
{
    ModeInfo *modeinfo;
    ModeTiming *modetiming;
    unsigned char *moderegs;

#ifdef SVGA_STYLE_320x200x256
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	&& mode != G320x200x256)
#else
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
#endif
	/* Let the standard VGA driver set standard VGA modes. */
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    if (!ark_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }
    moderegs = malloc(ARK_TOTAL_REGS);

    ark_initializemode(moderegs, modetiming, modeinfo);
    free(modetiming);

    ark_setregs(moderegs, mode);	/* Set extended regs. */
    __svgalib_setregs(moderegs);	/* Set standard regs. */
    free(moderegs);

    __svgalib_InitializeAcceleratorInterface(modeinfo);

    init_acceleration_specs_for_mode(__svgalib_driverspecs->accelspecs,
				     modeinfo->bitsPerPixel,
			  modeinfo->lineWidth / modeinfo->bytesPerPixel);

    arkaccel_init(__svgalib_driverspecs->accelspecs,
		  modeinfo->bitsPerPixel,
		  modeinfo->lineWidth / modeinfo->bytesPerPixel);

    free(modeinfo);
    return 0;
}


/* Indentify chipset; return non-zero if detected */

static int ark_test(void)
{
    if (ark_init(0, 0, 0))
	return 0;
    return 1;
}


/* Bank switching function - set 64K bank number */

static void ark_setpage(int page)
{
    __svgalib_outseq(0x15, page);
    __svgalib_outseq(0x16, page);
}


/* Set display start address (not for 16 color modes) */

static void ark_setdisplaystart(int address)
{
    __svgalib_outcrtc(0x0d, (address >> 2) & 0xff);
    __svgalib_outcrtc(0x0c, (address >> 10) & 0xff);
    port_in(0x3da);			/* set ATC to addressing mode */
    port_out_r(ATT_IW, 0x13 + 0x20);	/* select ATC reg 0x13 */
    port_out_r(ATT_IW, (port_in(ATT_R) & 0xf0) | ((address & 3) << 1));
    /* write sa0-1 to bits 1-2 */
    __svgalib_outcrtc(0x40, (address >> 18) & 0x07);
}


/* Set logical scanline length (usually multiple of 8) */
/* Multiples of 8 to 2040 */

static void ark_setlogicalwidth(int width)
{
    __svgalib_outcrtc(0x13, (width >> 3));
    port_out_r(__svgalib_CRT_I, 0x41);
    port_out_r(__svgalib_CRT_D, (port_in(__svgalib_CRT_D) & ~0x08) | ((width >> 3) & 0x100));
}

static int ark_linear(int op, int param)
{
    if (op == LINEAR_ENABLE) {
	int size;
	__svgalib_outSR(0x13, param >> 16);
	__svgalib_outSR(0x14, param >> 24);
	switch (ark_memory) {
	case 1024:
	    size = 1;
	    break;
	case 2048:
	    size = 2;
	    break;
	case 4096:
	default:
	    size = 3;
	}
	__svgalib_outSR(0x12, (__svgalib_inSR(0x12) & ~0x03) | size);
        is_linear=1;
	return 0;
    }
    if (op == LINEAR_DISABLE) {
	__svgalib_outSR(0x12, __svgalib_inSR(0x12) & ~0x03);	/* 64K map. */
	__svgalib_outSR(0x13, 0xA0);	/* Revert to 0xA0000. */
	__svgalib_outSR(0x14, 0x00);
        is_linear=0;
	return 0;
    }
    if (op == LINEAR_QUERY_BASE) {
	if (ark_bus == PCI && param == 0)
	    /* Return PCI base address. */
	    return ark_baseaddress;
	if (ark_bus == VL) {
	    if (param == 0)
		return 0x08000000;	/* 128MB */
	    if (param == 1)
		return 0x04000000;	/* 64MB */
	    if (param == 2)
		return 0x80000000;	/* 2048MB */
	    if (param == 3)
		return 0x02000000;	/* 32MB */
	}
	return -1;
    }
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY)
	return 0;		/* No granularity or range. */
    else
	return -1;		/* Unknown function. */
}

static struct {
    unsigned char c8;
    unsigned short c15;
    unsigned short c16;
    unsigned int c32;
} cursor_colors[16*2];

static int pal=1, palette[768];

static int findcolor(int rgb) {
   int i,j,k,l=0;

   if(pal)vga_getpalvec(0,256,palette);
   pal=0;
   k=0xffffff;
   for(i=0;i<256;i++) {
      j=((rgb&0xff)-(palette[i*3+2]<<2))*((rgb&0xff)-(palette[i*3+2]<<2))+
        (((rgb>>8)&0xff)-(palette[i*3+1]<<2))*(((rgb>>8)&0xff)-(palette[i*3+1]<<2))+
        (((rgb>>16)&0xff)-(palette[i*3]<<2))*(((rgb>>16)&0xff)-(palette[i*3]<<2));
      if(j==0) {
         return i;
      }
      if(j<k) {
         k=j;
         l=i;
      }
   }
   return l;
}

static int cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned char *b3, *b4;
    
    switch(cmd){
        case CURSOR_INIT:
            return 0; /* does not work */
        case CURSOR_HIDE:
            __svgalib_outseq(0x20, __svgalib_inseq(0x20) & 0xe7);
            break;
        case CURSOR_SHOW:
            switch(CI.colors) {
                case 256:
                    __svgalib_outseq(0x20, 0x18);
                    break;
                case 32768: case 65536:
                    __svgalib_outseq(0x20, 0x19);
                    break;
                case 1<<24:
                    __svgalib_outseq(0x20, 0x1a);
                    break;
            }
            __svgalib_outseq(0x20, __svgalib_inseq(0x20) | 0x18);
            break;
        case CURSOR_POSITION:
            __svgalib_outseq(0x2c, 0);
            __svgalib_outseq(0x2d, 0);
            __svgalib_outseq(0x22, p1&0xff);
            __svgalib_outseq(0x21, (p1>>8)&0xff);
            __svgalib_outseq(0x24, p2&0xff);
            __svgalib_outseq(0x23, (p2>>8)&0xff);
            break;
        case CURSOR_SELECT:
            i=ark_memory-(p1+1);
            switch(CI.colors) {
                case 256:
                   __svgalib_outseq(0x26,cursor_colors[p1*2+1].c8);
                   __svgalib_outseq(0x29,cursor_colors[p1*2].c8);
                   break;
                case 32768:
                   __svgalib_outseq(0x27,cursor_colors[p1*2+1].c15>>8);
                   __svgalib_outseq(0x26,cursor_colors[p1*2+1].c15&0xff);
                   __svgalib_outseq(0x2a,cursor_colors[p1*2].c15>>8);
                   __svgalib_outseq(0x29,cursor_colors[p1*2].c15&0xff);
                   break;
                case 65536:
                   __svgalib_outseq(0x27,cursor_colors[p1*2+1].c16>>8);
                   __svgalib_outseq(0x26,cursor_colors[p1*2+1].c16&0xff);
                   __svgalib_outseq(0x2a,cursor_colors[p1*2].c16>>8);
                   __svgalib_outseq(0x29,cursor_colors[p1*2].c16&0xff);
                   break;
                case (1<<24):
                   __svgalib_outseq(0x28,cursor_colors[p1*2+1].c32>>16);
                   __svgalib_outseq(0x27,(cursor_colors[p1*2+1].c32>>8)&0xff);
                   __svgalib_outseq(0x26,cursor_colors[p1*2+1].c32&0xff);
                   __svgalib_outseq(0x2b,cursor_colors[p1*2].c32>>16);
                   __svgalib_outseq(0x2a,(cursor_colors[p1*2].c32>>8)&0xff);
                   __svgalib_outseq(0x29,cursor_colors[p1*2].c32&0xff);
                   break;
            }
            __svgalib_outseq(0x25, (i>>8)&0x3f);
            __svgalib_outseq(0x25, 63);
            break;
        case CURSOR_IMAGE:
            i=ark_memory*1024-(p1+1)*1024;
            i=2048*1024-256;
            b3=(unsigned char *)p5;
            b4=LINEAR_POINTER+i;
            if (!is_linear){
	        int size;
	        __svgalib_outSR(0x13, ark_baseaddress >> 16);
	        __svgalib_outSR(0x14, ark_baseaddress >> 24);
	        switch (ark_memory) {
	        case 1024:
	            size = 1;
	            break;
	        case 2048:
	            size = 2;
	            break;
	        case 4096:
	        default:
	            size = 3;
	        }
	        __svgalib_outSR(0x12, (__svgalib_inSR(0x12) & ~0x03) | size);
            };
            cursor_colors[p1*2].c8=findcolor(p3);
            cursor_colors[p1*2].c32=p3;
            cursor_colors[p1*2].c16=((p3&0xf80000)>>8)|((p3&0xfc00)>>5)|((p3&0xf8)>>3);
            cursor_colors[p1*2].c15=((p3&0xf80000)>>9)|((p3&0xf800)>>5)|((p3&0xf8)>>3);
            cursor_colors[p1*2+1].c8=findcolor(p4);
            cursor_colors[p1*2+1].c32=p4;
            cursor_colors[p1*2+1].c16=((p4&0xf80000)>>8)|((p4&0xfc00)>>5)|((p4&0xf8)>>3);
            cursor_colors[p1*2+1].c15=((p4&0xf80000)>>9)|((p4&0xf800)>>5)|((p4&0xf8)>>3);
            switch(p2) {
                case 0:
                    for(j=0;j<32;j++) for(i=0;i<4;i++){
                        *b4=*b3;
                        b4++;
                        *b4=*(b3+32);
                        b4++;
                        b3++;
                    }
                break;
            }
            if (!is_linear){
	        __svgalib_outseq(0x12, __svgalib_inseq(0x12) & ~0x03);	/* 64K map. */
	        __svgalib_outseq(0x13, 0xA0);	/* Revert to 0xA0000. */
                __svgalib_outseq(0x14, 0x00);
            };
            break;
    }
    return 0;
}       

/* Function table (exported) */
DriverSpecs __svgalib_ark_driverspecs =
{
    ark_saveregs,		/* saveregs */
    ark_setregs,		/* setregs */
    ark_unlock,			/* unlock */
    nothing,			/* lock */
    ark_test,
    ark_init,
    ark_setpage,
    (void (*)(int)) nothing,
    (void (*)(int)) nothing,
    ark_setmode,
    ark_modeavailable,
    ark_setdisplaystart,
    ark_setlogicalwidth,
    ark_getmodeinfo,
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    0,				/* extset */
    0,
    ark_linear,
    NULL,			/* accelspecs */
    NULL,                       /* Emulation */
    cursor,
};

/* ARK-specific config file options. */

/*
 * Currently this only handles Clocks. It would a good idea to have
 * higher-level code process options like Clocks that are valid for
 * more than one driver driver (with better error detection etc.).
 */

static char *ark_config_options[] =
{
    "clocks", "ramdac", "dacspeed", NULL
};

static char *ark_process_option(int option, int mode, char** dummy)
{
/*
 * option is the number of the option string in ark_config_options,
 * mode seems to be a security indicator.
 */
    if (option == 0) {		/* "Clocks" */
	/* Process at most 16 specified clocks. */
	cardspecs->clocks = malloc(sizeof(int) * 16);
	/* cardspecs->nClocks should already be 0. */
	for (;;) {
	    char *ptr;
	    int freq;
	    ptr = strtok(NULL, " ");
	    if (ptr == NULL)
		break;
	    /*
	     * This doesn't protect against bad characters
	     * (atof() doesn't detect errors).
	     */
	    freq = atof(ptr) * 1000;
	    cardspecs->clocks[cardspecs->nClocks] = freq;
	    cardspecs->nClocks++;
	    if (cardspecs->nClocks == 16)
		break;
	}
    }
    if (option == 1) {		/* "Ramdac" */
	char *ptr;
	DacMethods *old_dac_used;
	ptr = strtok(NULL, " ");
	old_dac_used = dac_used;
#ifdef INCLUDE_ICS_GENDAC_DAC
	if (strcasecmp(ptr, "ICSgendac") == 0)
	    dac_used = &__svgalib_ICS_GENDAC_methods;
#endif
#ifdef INCLUDE_SIERRA_DAC
	if (strcasecmp(ptr, "Sierra32K") == 0)
	    dac_used = &__svgalib_Sierra_32K_methods;
#endif
#ifdef INCLUDE_ATT20C490_DAC
	if (strcasecmp(ptr, "ATT20C490") == 0)
	    dac_used = &__svgalib_ATT20C490_methods;
#endif
#ifdef INCLUDE_ATT20C498_DAC
	if (strcasecmp(ptr, "ATT20C498") == 0)
	    dac_used = &__svgalib_ATT20C498_methods;
#endif
#ifdef INCLUDE_NORMAL_DAC
	if (strcasecmp(ptr, "Normal") == 0)	/* force normal VGA dac */
	    dac_used = &__svgalib_normal_dac_methods;
#endif
	if (old_dac_used != dac_used)
	    dac_used->initialize();
    }
    if (option == 2) {		/* "Dacspeed" */
	char *ptr;
	ptr = strtok(NULL, " ");
	/*
	 * This doesn't protect against bad characters
	 * (atoi() doesn't detect errors).
	 */
	dac_speed = atoi(ptr) * 1000;
    }
    return strtok(NULL, " ");
}


/* Initialize driver (called after detection) */

static DacMethods *dacs_to_probe[] =
{
#ifdef INCLUDE_ICS_GENDAC_DAC_TEST
    &__svgalib_ICS_GENDAC_methods,
#endif
#ifdef INCLUDE_ATT20C490_DAC_TEST
    &__svgalib_ATT20C490_methods,
#endif
#ifdef INCLUDE_ATT20C498_DAC_TEST
    &__svgalib_ATT20C498_methods,
#endif
#ifdef INCLUDE_SIERRA_DAC_TEST
    &__svgalib_Sierra_32K_methods,
#endif
    NULL};

static char *ark_chipname[] =
{"ARK1000PV", "ARK2000PV"};

static int ark_init(int force, int par1, int par2)
{
    ark_unlock();

    if (force) {
	ark_chip = par1;	/* we already know the type */
	ark_memory = par2;
    } else {
	unsigned char id, val;
	id = __svgalib_inCR(0x50) >> 3;
	if (id == 0x12)
	    ark_chip = ARK1000PV;
	else if ((id == 0x13) || (id == 0x14) || (id == 0x20))
	    ark_chip = ARK2000PV;
	else {
	    fprintf(stderr,"svgalib: ark: Unknown chiptype %d.\n",
		   id);
	    return -1;
	}
	val = __svgalib_inSR(0x10);
	if (ark_chip == ARK1000PV)
	    if ((val & 0x40) == 0)
		ark_memory = 1024;
	    else
		ark_memory = 2048;
	else if ((val & 0xC0) == 0)
	    ark_memory = 1024;
	else if ((val & 0xC0) == 0x40)
	    ark_memory = 2048;
	else
	    ark_memory = 4096;
    }


/* begin: Initialize cardspecs. */
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = ark_memory;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->nClocks = 0;
    cardspecs->flags = INTERLACE_DIVIDE_VERT;

    /* Process ARK-specific config file options. */
    __svgalib_read_options(ark_config_options, ark_process_option);


    if (dac_used == NULL) {
        dac_used=__svgalib_probeDacs(dacs_to_probe);
    }

    if (dac_used == NULL) {
	/* Not supported. */
        fprintf(stderr,"svgalib: ark: Assuming normal VGA DAC.\n");

#ifdef INCLUDE_NORMAL_DAC
        dac_used = &__svgalib_normal_dac_methods;
#else
        fprintf(stderr,"svgalib: Alas, normal VGA DAC support is not compiled in, goodbye.\n");
        return 1;
#endif
    }
    dac_used->qualifyCardSpecs(cardspecs, dac_speed);

    /* Initialize standard clocks for unknown clock device. */
    if (!(dac_used->flags & CLOCK_PROGRAMMABLE)
	&& cardspecs->nClocks == 0) {
#ifdef INCLUDE_HALVED_CLOCKS
	int i;
	cardspecs->nClocks = 32;
	cardspecs->clocks = malloc(sizeof(int) * 32);
#else
	cardspecs->nClocks = 2;
	cardspecs->clocks = malloc(sizeof(int) * 2);
#endif
	cardspecs->clocks[0] = 25175;
	cardspecs->clocks[1] = 28322;
#ifdef INCLUDE_HALVED_CLOCKS
	for (i = 2; i < 16; i++)
	    cardspecs->clocks[i] = 0;
	for (i = 16; i < 32; i++)
	    cardspecs->clocks[i] = cardspecs->clocks[i - 16] / 2;
#endif
    }
    /* Limit pixel clocks according to chip specifications. */
    if (ark_chip == ARK1000PV) {
	/* Limit max clocks according to 120 MHz DCLK spec. */
	/* 8-bit DAC. */
	LIMIT(cardspecs->maxPixelClock4bpp, 120000);
	LIMIT(cardspecs->maxPixelClock8bpp, 120000);
	LIMIT(cardspecs->maxPixelClock16bpp, 120000 / 2);
	LIMIT(cardspecs->maxPixelClock24bpp, 120000 / 3);
	LIMIT(cardspecs->maxPixelClock32bpp, 120000 / 4);
    }
    if (ark_chip == ARK2000PV) {
	/* Limit max clocks according to 120 MHz DCLK spec. */
	/* Assume 16-bit DAC. */
	LIMIT(cardspecs->maxPixelClock4bpp, 120000);
	LIMIT(cardspecs->maxPixelClock8bpp, 120000 * 2);
	LIMIT(cardspecs->maxPixelClock16bpp, 120000);
	LIMIT(cardspecs->maxPixelClock24bpp, 120000 / 3);
	LIMIT(cardspecs->maxPixelClock32bpp, 120000 / 2);
    }
    cardspecs->maxPixelClock4bpp = 0;	/* 16-color modes don't work. */
/* end: Initialize cardspecs. */

/* Initialize accelspecs structure. */
    __svgalib_ark_driverspecs.accelspecs = malloc(sizeof(AccelSpecs));
    __svgalib_clear_accelspecs(__svgalib_ark_driverspecs.accelspecs);
    __svgalib_ark_driverspecs.accelspecs->flags = ACCELERATE_ANY_LINEWIDTH;
    /* Map memory-mapped I/O register space. */

    if (__svgalib_driver_report) {
	char *bustype;
	if (__svgalib_inSR(0x19) & 0x80) {
	    ark_bus = VL;
	    bustype = "VL bus";
	} else {
	    ark_bus = PCI;
	    bustype = "PCI";
	    ark_baseaddress = (__svgalib_inSR(0x13) << 16) +
		(__svgalib_inSR(0x14) << 24);
	}
	fprintf(stderr,"svgalib: Using ARK driver (%s, %dK, %s).",
	       ark_chipname[ark_chip], ark_memory, bustype);
#if 0
	if (ark_bus == PCI)
	    fprintf(stderr," Base address = 0x%08X.", ark_baseaddress);
#endif
	fprintf(stderr,"\n");
    }
    __svgalib_driverspecs = &__svgalib_ark_driverspecs;
    __svgalib_mmio_base = 0xb8000;
    __svgalib_mmio_size = 4096;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    if (ark_bus == PCI)__svgalib_linear_mem_base=ark_baseaddress;
    __svgalib_linear_mem_size=ark_memory*0x400;

    return 0;
}


/* ARK Logic acceleration functions implementation. */

/* We use linear COP addresses (no coordinates) to allow acceleration */
/* in any linewidth. */

/* Macros for memory-mapped I/O COP register access. */

/* MMIO addresses (offset from 0xb8000). */

#define BACKGROUNDCOLOR	0x00
#define FOREGROUNDCOLOR	0x02
#define COLORMIXSELECT	0x18
#define WRITEPLANEMASK	0x1A
#define STENCILPITCH	0x60
#define SOURCEPITCH	0x62
#define DESTPITCH	0x64
#define STENCILADDR	0x68
#define STENCILX	0x68
#define STENCILY	0x6A
#define SOURCEADDR	0x6C
#define SOURCEX		0x6C
#define SOURCEY		0x6E
#define DESTADDR	0x70
#define DESTX		0x70
#define DESTY		0x72
#define WIDTH		0x74
#define HEIGHT		0x76
#define BITMAPCONFIG	0x7C
#define COMMAND		0x7E

/* Flags for COMMAND register. */

#define DRAWNSTEP		0x0000
#define LINEDRAW		0x1000
#define BITBLT			0x2000
#define TEXTBITBLT		0x3000
#define USEPLANEMASK		0x0000
#define DISABLEPLANEMASK	0x0800
#define PATTERN8X8		0x0400
#define SELECTBGCOLOR		0x0000
#define BACKGROUNDBITMAP	0x0200
#define SELECTFGCOLOR		0x0000
#define FOREGROUNDBITMAP	0x0100
#define STENCILONES		0x0000
#define STENCILGENERATED	0x0040
#define STENCILBITMAP		0x0080
#define LINEDRAWALL		0x0000
#define LINESKIPFIRST		0x0010
#define LINESKIPLAST		0x0020
#define ENABLECLIPPING		0x0000
#define DISABLECLIPPING		0x0008
#define RIGHTANDDOWN		0x0000
#define RIGHTANDUP		0x0002
#define LEFTANDDOWN		0x0004
#define LEFTANDUP		0x0006

/* Flags for Bitmap Configuration register. */

#define SWAPNIBLES		0x2000
#define SWAPBITS		0x1000
#define SYSTEMSTENCIL		0x0200
#define LINEARSTENCILADDR	0x0100
#define SYSTEMSOURCE		0x0020
#define LINEARSOURCEADDR	0x0010
#define	SYSTEMDEST		0x0002
#define LINEARDESTADDR		0x0001

#define SETBACKGROUNDCOLOR(c) \
	*(unsigned short *)(MMIO_POINTER + BACKGROUNDCOLOR) = c;

#define SETFOREGROUNDCOLOR(c) \
	*(unsigned short *)(MMIO_POINTER + FOREGROUNDCOLOR) = c;

#define SETCOLORMIXSELECT(m) \
	*(unsigned short *)(MMIO_POINTER + COLORMIXSELECT) = m;

#define SETWRITEPLANEMASK(m) \
	*(unsigned short *)(MMIO_POINTER + WRITEPLANEMASK) = m;

#define SETSTENCILPITCH(p) \
	*(unsigned short *)(MMIO_POINTER + STENCILPITCH) = p;

#define SETSOURCEPITCH(p) \
	*(unsigned short *)(MMIO_POINTER + SOURCEPITCH) = p;

#define SETDESTPITCH(p) \
	*(unsigned short *)(MMIO_POINTER + DESTPITCH) = p;

#define SETSTENCILADDR(p) \
	*(unsigned int *)(MMIO_POINTER + STENCILADDR) = p;

#define SETSOURCEADDR(p) \
	*(unsigned int *)(MMIO_POINTER + SOURCEADDR) = p;

#define SETSOURCEXY(x, y) \
	*(unsigned int *)(MMIO_POINTER + SOURCEADDR) = (y << 16) + x;

#define SETSOURCEX(x) \
	*(unsigned short *)(MMIO_POINTER + SOURCEX) = x;

#define SETSOURCEY(y) \
	*(unsigned short *)(MMIO_POINTER + SOURCEY) = y;

#define SETDESTADDR(p) \
	*(unsigned int *)(MMIO_POINTER + DESTADDR) = p;

#define SETDESTXY(x, y) \
	*(unsigned int *)(MMIO_POINTER + DESTADDR) = (y << 16) + x;

#define SETDESTX(x) \
	*(unsigned short *)(MMIO_POINTER + DESTX) = x;

#define SETDESTY(y) \
	*(unsigned short *)(MMIO_POINTER + DESTY) = y;

#define SETWIDTH(p) \
	*(unsigned short *)(MMIO_POINTER + WIDTH) = p - 1;

#define SETHEIGHT(p) \
	*(unsigned short *)(MMIO_POINTER + HEIGHT) = p - 1;

#define SETBITMAPCONFIG(p) \
	*(unsigned short *)(MMIO_POINTER + BITMAPCONFIG) = p;

#define SETCOMMAND(p) \
	*(unsigned short *)(MMIO_POINTER + COMMAND) = p;

#define COPISBUSY() (port_in(0x3CB) & 0x40)

#define WAITUNTILFINISHED() \
	for (;;) { \
		if (!COPISBUSY()) \
			break; \
	}

static void arkaccel_init(AccelSpecs * accelspecs, int bpp, int width_in_pixels)
{
#ifdef COORDINATES
    int pitch;
#endif
    SETCOLORMIXSELECT(0x0303);	/* Copy source. */
    SETWRITEPLANEMASK(0xFFFF);
    SETSTENCILPITCH(width_in_pixels);
    SETSOURCEPITCH(width_in_pixels);
    SETDESTPITCH(width_in_pixels);
#ifdef COORDINATES
    SETBITMAPCONFIG(0);
    switch (width_in_pixels) {
    case 640:
	pitch = 0;
	break;
    case 800:
	pitch = 1;
	break;
    case 1024:
	pitch = 2;
	break;
    case 1280:
	pitch = 4;
	break;
    case 1600:
	pitch = 5;
	break;
    case 2048:
	pitch = 6;
	break;
    }
    __svgalib_outSR(0x17, (__svgalib_inSR(0x17) & ~0x07) | pitch);
#else
    SETBITMAPCONFIG(LINEARSTENCILADDR | LINEARSOURCEADDR |
		    LINEARDESTADDR);
#endif
}

#define FINISHBACKGROUNDBLITS() \
	if (__svgalib_accel_mode & BLITS_IN_BACKGROUND) \
		WAITUNTILFINISHED();

void __svgalib_arkaccel_FillBox(int x, int y, int width, int height)
{
    int destaddr;
    destaddr = BLTPIXADDRESS(x, y);
    FINISHBACKGROUNDBLITS();
    SETDESTADDR(destaddr);
    SETWIDTH(width);
    SETHEIGHT(height);
    SETCOMMAND(SELECTBGCOLOR | SELECTFGCOLOR | STENCILONES |
	       DISABLEPLANEMASK | DISABLECLIPPING | BITBLT);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}

void __svgalib_arkaccel_coords_FillBox(int x, int y, int width, int height)
{
    FINISHBACKGROUNDBLITS();
    SETDESTXY(x, y);
    SETWIDTH(width);
    SETHEIGHT(height);
    SETCOMMAND(SELECTBGCOLOR | SELECTFGCOLOR | STENCILONES |
	       DISABLEPLANEMASK | DISABLECLIPPING | BITBLT);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}

void __svgalib_arkaccel_ScreenCopy(int x1, int y1, int x2, int y2, int width,
			 int height)
{
    int srcaddr, destaddr, dir;
    srcaddr = BLTPIXADDRESS(x1, y1);
    destaddr = BLTPIXADDRESS(x2, y2);
    dir = RIGHTANDDOWN;
    if ((y1 < y2 || (y1 == y2 && x1 < x2))
	&& y1 + height > y2) {
	srcaddr += (height - 1) * __svgalib_accel_screenpitch + width - 1;
	destaddr += (height - 1) * __svgalib_accel_screenpitch + width - 1;
	dir = LEFTANDUP;
    }
    FINISHBACKGROUNDBLITS();
    SETSOURCEADDR(srcaddr);
    SETDESTADDR(destaddr);
    SETWIDTH(width);
    SETHEIGHT(height);
    SETCOMMAND(BACKGROUNDBITMAP | FOREGROUNDBITMAP |
	       STENCILONES | DISABLEPLANEMASK | DISABLECLIPPING |
	       BITBLT | dir);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}

void __svgalib_arkaccel_coords_ScreenCopy(int x1, int y1, int x2, int y2, int width,
				int height)
{
    int dir;
    dir = RIGHTANDDOWN;
    if ((y1 < y2 || (y1 == y2 && x1 < x2))
	&& y1 + height > y2) {
	y1 += height - 1;
	y2 += height - 1;
	x1 += width - 1;
	x2 += width - 1;
	dir = LEFTANDUP;
    }
    FINISHBACKGROUNDBLITS();
    SETSOURCEXY(x1, y1);
    SETDESTXY(x2, y2);
    SETWIDTH(width);
    SETHEIGHT(height);
    SETCOMMAND(BACKGROUNDBITMAP | FOREGROUNDBITMAP |
	       STENCILONES | DISABLEPLANEMASK | DISABLECLIPPING |
	       BITBLT | dir);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}

void __svgalib_arkaccel_DrawHLineList(int ymin, int n, int *xmin, int *xmax)
{
    int y, destaddr;
    FINISHBACKGROUNDBLITS();
    SETHEIGHT(1);
    y = ymin;
    destaddr = BLTPIXADDRESS(0, ymin);
    while (n > 0) {
	/*
	 * We don't wait for previous commands to finish.
	 * The ARK databook isn't specific on this, but
	 * I assume the chip will correctly force waits when
	 * the command FIFO is full.
	 */
	int x, w;
	x = *xmin;
	SETDESTADDR(destaddr + x);
	w = *xmax - x;
	if (w > 0) {
	    SETWIDTH(w);
	    SETCOMMAND(SELECTBGCOLOR | SELECTFGCOLOR | STENCILONES
		       | DISABLEPLANEMASK | DISABLECLIPPING | BITBLT);
	}
	xmin++;
	xmax++;
	destaddr += __svgalib_accel_screenpitch;
	n--;
    }
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}

void __svgalib_arkaccel_coords_DrawHLineList(int ymin, int n, int *xmin, int *xmax)
{
    int y;
    FINISHBACKGROUNDBLITS();
    SETHEIGHT(1);
    y = ymin;
    while (n > 0) {
	/*
	 * We don't wait for previous commands to finish.
	 * The ARK databook isn't specific on this, but
	 * I assume the chip will correctly force waits when
	 * the command FIFO is full.
	 */
	int x, w;
	x = *xmin;
	SETDESTXY(x, y);
	w = *xmax - x;
	if (w > 0) {
	    SETWIDTH(w);
	    SETCOMMAND(SELECTBGCOLOR | SELECTFGCOLOR | STENCILONES
		       | DISABLEPLANEMASK | DISABLECLIPPING | BITBLT);
	}
	xmin++;
	xmax++;
	n--;
    }
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	WAITUNTILFINISHED();
}


void __svgalib_arkaccel_SetFGColor(int fg)
{
    SETFOREGROUNDCOLOR(fg);
}

void __svgalib_arkaccel_SetBGColor(int fg)
{
    SETBACKGROUNDCOLOR(fg);
}

void __svgalib_arkaccel_PutBitmap(int x, int y, int w, int h, void *bitmap)
{
    int destaddr, count;
#ifndef COORDINATES
    destaddr = BLTPIXADDRESS(x, y);
#endif
    /*
     * Calculate number of bytes to transfer.
     * The ARK chip requires the bitmap scanlines to be aligned
     * to 32-bit words, with correct bit order, so no conversion
     * is required.
     */
    count = (((w + 31) & ~0x1F) / 8) * h;
    FINISHBACKGROUNDBLITS();
#ifdef COORDINATES
    SETDESTXY(x, y);
#else
    SETDESTADDR(destaddr);
#endif
    SETWIDTH(w);
    SETHEIGHT(h);
    SETBITMAPCONFIG(LINEARSTENCILADDR | LINEARSOURCEADDR |
		    LINEARDESTADDR | SYSTEMSTENCIL);
    SIGNALBLOCK;
    SETCOMMAND(SELECTBGCOLOR | SELECTFGCOLOR | STENCILBITMAP
	       | DISABLEPLANEMASK | DISABLECLIPPING | BITBLT);
    while (count >= 65536) {
	memcpy(__svgalib_graph_mem, bitmap, 65536);
	count -= 65536;
	bitmap += 65536;
    }
    if (count > 0)
	memcpy(__svgalib_graph_mem, bitmap, count);
    SIGNALUNBLOCK;
    WAITUNTILFINISHED();
}

static unsigned char ark_rop_map[] =
{
    0x3,			/* COPY */
    0x7,			/* OR */
    0x1,			/* AND */
    0x6,			/* XOR */
    0xA				/* INVERT */
};

void __svgalib_arkaccel_SetRasterOp(int r)
{
    /* Write rop at bits 0-3 and 8-11. */
    SETCOLORMIXSELECT((int) ark_rop_map[r] * 0x0101);
}

void __svgalib_arkaccel_Sync(void)
{
    WAITUNTILFINISHED();
}

/*
 * Set up accelerator interface for pixels of size bpp and scanline width
 * of width_in_pixels.
 */

static void init_acceleration_specs_for_mode(AccelSpecs * accelspecs, int bpp,
					     int width_in_pixels)
{
    accelspecs->operations = 0;
    accelspecs->ropOperations = 0;
    accelspecs->transparencyOperations = 0;
    accelspecs->ropModes = (1<<ROP_COPY) | (1<<ROP_OR) | (1<<ROP_AND) |
				(1<<ROP_XOR) | (1<<ROP_INVERT);
    accelspecs->transparencyModes = 0;
#ifdef COORDINATES
    if (width_in_pixels != 640 && width_in_pixels != 800
	&& width_in_pixels != 1024 && width_in_pixels != 1280
	&& width_in_pixels != 1600 && width_in_pixels != 2048)
	return;
#endif
    accelspecs->operations |= ACCELFLAG_SETMODE | ACCELFLAG_SYNC
	| ACCELFLAG_SETRASTEROP;
    if (bpp == 8 || bpp == 16) {
	accelspecs->operations |=
	    ACCELFLAG_FILLBOX | ACCELFLAG_SETFGCOLOR |
	    ACCELFLAG_SCREENCOPY |
	    ACCELFLAG_SETBGCOLOR | ACCELFLAG_PUTBITMAP;
	accelspecs->ropOperations |=
	    ACCELFLAG_FILLBOX | ACCELFLAG_SCREENCOPY;
    }
    /* Set the function pointers; availability is handled by flags. */
#ifdef COORDINATES
    accelspecs->FillBox = __svgalib_arkaccel_coords_FillBox;
    accelspecs->ScreenCopy = __svgalib_arkaccel_coords_ScreenCopy;
#else
    accelspecs->FillBox = __svgalib_arkaccel_FillBox;
    accelspecs->ScreenCopy = __svgalib_arkaccel_ScreenCopy;
#endif
    accelspecs->SetFGColor = __svgalib_arkaccel_SetFGColor;
    accelspecs->SetRasterOp = __svgalib_arkaccel_SetRasterOp;
    accelspecs->Sync = __svgalib_arkaccel_Sync;
    accelspecs->SetBGColor = __svgalib_arkaccel_SetBGColor;
    accelspecs->PutBitmap = __svgalib_arkaccel_PutBitmap;
}
