/*
 * VGAlib version 1.2 - (c) 1993 Tommy Frandsen
 *
 * This library is free software; you can redistribute it and/or
 * modify it without any restrictions. This library is distributed
 * in the hope that it will be useful, but without any warranty.
 *
 * Multi-chipset support Copyright (C) 1993 Harm Hanemaayer
 * S3 805,868 support Copyright (C) 1995 Stephen Lee
 */

/*
 * Mar 1999 (Eduardo ...)
 * Recognizes Trio3D as Trio64
 *
 * Sep 1997 (Greg Alexander):
 * Recognizes S3Trio64V2/DX cards as Trio64's.
 *
 * Feb 1996 (Stephen Lee):
 * 968/IBMRGB support.  Only 256 colors for now.
 * can now save more than 10 DAC registers (IBMRGB has 256!)
 * Trio64 patch from Moto Kawamura <kawamura@mmp.cl.nec.co.jp>.
 * Changed handling of CR34 for VGA modes at Andreas' suggestion.
 * Changes to s3_saveregs() and s3_setregs() to make them more safe against
 *   lockups.
 * 16 color mode should work on the 868/SDAC.
 * SDAC 4/8bpp doesn't seem to do pixel multiplexing.
 * 
 * Dec 1995 (Stephen Lee):
 * Fixed color problem with 868 (CR43 again!).  Could somebody find the
 * value that works with Trio64?
 * 
 * Nov 1995 (Stephen Lee):
 * Linear addressing mode partially works (but is very alpha).
 * Merged in Andreas Arens' <ari@av.rwth-aachen.de> patch for the 928.
 * 
 * Sep 1995 (Stephen Lee):
 * 16 Colors works on my 805, should work on other cards too.
 * 
 * Alternate banking scheme for 864+.  If you have problems, try undefining
 * S3_LINEAR_MODE_BANKING_864.
 * 
 * 8 bit color *really* works.  Took me 3 months to bag this sucker.
 * 
 * SVGA 8 bit color modes works.  320x200x256 is not really 'packed-pixel',
 * it occupies 256K per page.  There is no SVGA 320x200x256 mode; I cannot
 * get the display (timing?) right.
 * 
 * Aug 1995 (Stephen Lee):
 * Added "Dacspeed" parsing.
 * Added support for CLUT8_8 on ATT20C490/498.
 * Improved support for S3-801/805.
 * 15/16/24 bit colors works on the 805 + ATT20C490 I tested.
 * Newer chipsets are recognized (but no support coded in yet).
 * Should recognize memory size correctly on S3-924.
 * 
 * Dec 1994 (Harm Hanemaayer):
 * Partially rewritten using new SVGA-abstracted interface.
 * Based on XFree86 code (accel/s3/s3.c and s3init.c).
 * Goal is to have support for the S3-864 + S3-SDAC (which I can test).
 * 80x with GENDAC might also be supported.
 * Also, 640x480x256 should work on cards that have standard 25 and 28 MHz
 * clocks.
 *
 * XFree86-equivalent clock select is now supported plus some
 * industry-standard RAMDACs.
 *
 * Remaining problems:
 * * Okay, okay, so 256 color still isn't fully working on the 805.  I'm
 *   trying to get a fix for it.
 * 
 * * The DCLK limit for 864/868 is a bit too relaxed.  If you see noise at
 *   the highest resolutions when the screen is drawing it is possibly due
 *   to this.  (How about changing MCLK?)
 * 
 * * Horizontal Total is limited to 4088 which makes some modes unavailable
 *   (e.g. 800x600x16M with HTotal > 1022).  Should experiment with
 *   CR43.7?
 * 
 * * Some 864 problems are now fixed -- XF86_S3 seems to program the
 *   linewidth in bytes doubled for the S3-864 with > 1024K, which
 *   caused problems for this driver. There's still interference
 *   though when writing to video memory in the higher resolutions.
 * 
 * * XXXX results of malloc() are not checked: should fix sometime.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "ramdac/ramdac.h"
#include "clockchi/clockchi.h"
#include "vgaregs.h"
#include "interfac.h"
#include "8514a.h"
#include "vgapci.h"

/* no acceleration as of now. */
#undef S3_USE_GRAPHIC_ENGINE

/* kludge packed pixel for 320x200x256 */
/* XXXX doesn't really work */
#undef S3_KLUDGE_PAGE_MODE

/* use alternate 'linear' banking method for 864+ */
#undef S3_LINEAR_MODE_BANKING_864

#ifdef __alpha__		/* no good for alpha's */
#undef S3_LINEAR_MODE_BANKING_864
#endif

/*
 * supports linear buffer.
 *
 * XXXX does not work with console switching and might be incompatible with
 *      S3_LINEAR_MODE_BANKING_864.
 */
#define S3_LINEAR_SUPPORT

/* supports 16 colors */
#define S3_16_COLORS

/*
 * zero wait state + (ramdac?) FIFO for 864 & 805,
 * twice as fast but might not work on some cards.
 */
#undef S3_0_WAIT_805_864

enum {
    S3_911, S3_924, S3_801, S3_805, S3_928, S3_864, S3_964, S3_TRIO32,
    S3_TRIO64, S3_866, S3_868, S3_968, S3_765
};

static const char *s3_chipname[] =
{"911", "924", "801", "805", "928",
 "864", "964", "Trio32", "Trio64", "866", "868", "968", "Trio64V+"};

#define S3_CR(n)	(EXT + (0x##n) - 0x30)

#define S3_CR30		S3_CR(30)
#define S3_CR31		S3_CR(31)
#define S3_CR32		S3_CR(32)
#define S3_CR33		S3_CR(33)
#define S3_CR34		S3_CR(34)
#define S3_CR35		S3_CR(35)
#define S3_CR3A		S3_CR(3A)
#define S3_CR3B		S3_CR(3B)
#define S3_CR3C		S3_CR(3C)
#define S3_CR40		S3_CR(40)
#define S3_CR42		S3_CR(42)
#define S3_CR43		S3_CR(43)
#define S3_CR44		S3_CR(44)
#define S3_CR50		S3_CR(50)	/* 801+ */
#define S3_CR51		S3_CR(51)
#define S3_CR53		S3_CR(53)
#define S3_CR54		S3_CR(54)
#define S3_CR55		S3_CR(55)
#define S3_CR58		S3_CR(58)
#define S3_CR59		S3_CR(59)
#define S3_CR5A		S3_CR(5A)
#define S3_CR5D		S3_CR(5D)
#define S3_CR5E		S3_CR(5E)
#define S3_CR60		S3_CR(60)
#define S3_CR61		S3_CR(61)
#define S3_CR62		S3_CR(62)
#define S3_CR67		S3_CR(67)
#define S3_CR6A		S3_CR(6A)
#define S3_CR6D		S3_CR(6D)

/* For debugging, these (non-)registers are read also (but never written). */

#define S3_CR36		S3_CR(36)
#define S3_CR37		S3_CR(37)
#define S3_CR38		S3_CR(38)
#define S3_CR39		S3_CR(39)
#define S3_CR3D		S3_CR(3D)
#define S3_CR3E		S3_CR(3E)
#define S3_CR3F		S3_CR(3F)
#define S3_CR45		S3_CR(45)
#define S3_CR46		S3_CR(46)
#define S3_CR47		S3_CR(47)
#define S3_CR48		S3_CR(48)
#define S3_CR49		S3_CR(49)
#define S3_CR4A		S3_CR(4A)
#define S3_CR4B		S3_CR(4B)
#define S3_CR4C		S3_CR(4C)
#define S3_CR4D		S3_CR(4D)
#define S3_CR4E		S3_CR(4E)
#define S3_CR4F		S3_CR(4F)
#define S3_CR52		S3_CR(52)
#define S3_CR56		S3_CR(56)
#define S3_CR57		S3_CR(57)
#define S3_CR5B		S3_CR(5B)
#define S3_CR5C		S3_CR(5C)
#define S3_CR5F		S3_CR(5F)
#define S3_CR63		S3_CR(63)
#define S3_CR64		S3_CR(64)
#define S3_CR65		S3_CR(65)
#define S3_CR66		S3_CR(66)
#define S3_CR6E		S3_CR(6E)
#define S3_CR6F		S3_CR(6F)

/* Trio extended SR registers */

#define S3_SR(n)	(S3_CR6F + 1 + (0x##n) - 0x08)

#define S3_SR08		S3_SR(08)
#define S3_SR09		S3_SR(09)
#define S3_SR0A		S3_SR(0A)
#define S3_SR0D		S3_SR(0D)
#define S3_SR10		S3_SR(10)
#define S3_SR11		S3_SR(11)
#define S3_SR12		S3_SR(12)
#define S3_SR13		S3_SR(13)
#define S3_SR15		S3_SR(15)
#define S3_SR18		S3_SR(18)
#define S3_SR1D		S3_SR(1D)

#define S3_8514_OFFSET	(S3_SR1D + 1)

#define S3_8514_COUNT	(1)	/* number of 2-byte words */

#define S3_DAC_OFFSET	(S3_8514_OFFSET + (S3_8514_COUNT * 2))

#define S3_TOTAL_REGS	(S3_DAC_OFFSET + MAX_DAC_STATE)

/* 8514 regs */
#define S3_ADVFUNC_CNTL	0

static unsigned short s3_8514regs[S3_8514_COUNT] =
{
    /* default assuming text mode */
    0x0000U
};

/* flags used by this driver */
#define S3_LOCALBUS		0x01
#define S3_CLUT8_8		0x02
#define S3_OLD_STEPPING		0x04

static int s3_flags = 0;

static int s3_chiptype;
static int s3_memory;
static CardSpecs *cardspecs;
static DacMethods *dac_used;
static ClockChipMethods *clk_used;
static int dac_speed = 0;

int __svgalib_s3_s3Mclk = 0;

/* forward declaration. */
extern DriverSpecs __svgalib_s3_driverspecs;

static int s3_init(int, int, int);
static void s3_setpage(int page);
#ifdef S3_LINEAR_MODE_BANKING_864
static void s3_setpage864(int page);
#endif

#ifdef S3_LINEAR_SUPPORT
static int s3_cr40;
static int s3_cr54;
static int s3_cr58;
static int s3_cr59;
static int s3_cr5A;
static int s3_linear_opt = 0;
static int s3_linear_addr = 0;
static int s3_linear_base = 0;
static void s3_linear_enable(void);
static void s3_linear_disable(void);
#endif

static void nothing(void)
{
}

/*
 * Lock S3's registers.
 * There are more locks, but this should suffice.
 *
 * ARI: More complete Extended VGA Register Lock Documentation, as of Ferraro:
 *
 * Register     Bit     Controls Access To:             Function
 * CR33         1       CR7 bits 1 and 6                1=disable write protect
 *                                                        setting of CR11 bit 7
 * CR33         4       Ramdac Register                 1=disable writes
 * CR33         6       Palette/Overscan Registers      1=lock
 * CR34         5       Memory Configuration bit 5      1=lock
 * CR34         7       Misc Register bit 3-2 (Clock)   1=lock
 * CR35         4       Vertical Timing Registers       1=lock
 * CR35         5       Horizontal Timing Registers     1=lock
 * 
 * XXXX mostly, need to lock the enhanced command regs on the 805 (and 
 * probably below) to avoid display corruption.
 */
static void s3_lock(void)
{
    __svgalib_outCR(0x39, 0x00);		/* Lock system control regs. */
    __svgalib_outCR(0x38, 0x00);		/* Lock special regs. */
}

static void s3_lock_enh(void)
{
    if (s3_chiptype > S3_911)
	__svgalib_outCR(0x40, __svgalib_inCR(0x40) & ~0x01);    /* Lock enhanced command regs. */
    s3_lock();
}

/*
 * Unlock S3's registers.
 * There are more locks, but this should suffice.
 */
static void s3_unlock(void)
{
    __svgalib_outCR(0x38, 0x48);		/* Unlock special regs. */
    __svgalib_outCR(0x39, 0xA5);		/* Unlock system control regs. */
}

static void s3_unlock_enh(void)
{
    s3_unlock();
    if (s3_chiptype > S3_911)
	__svgalib_outCR(0x40, __svgalib_inCR(0x40) | 0x01);     /* Unlock enhanced command regs. */
}

/* 
 * Adjust the display width.  This is necessary for the graphics
 * engine if acceleration is used.  However it will require more
 * memory making some modes unavailable.
 */
static int s3_adjlinewidth(int oldwidth)
{
    if (s3_chiptype < S3_801)
	return 1024;
#ifdef S3_USE_GRAPHIC_ENGINE
    if (oldwidth <= 640)
	return 640;
    if (oldwidth <= 800)
	return 800;
    if (oldwidth <= 1024)
	return 1024;
    if (!(s3_flags & S3_OLD_STEPPING))
	if (oldwidth <= 1152)
	    return 1152;
    if (oldwidth <= 1280)
	return 1280;
    if (oldwidth <= 1600 && s3_chiptype >= S3_864)
	return 1600;

    return 2048;
#else
    return oldwidth;
#endif
}

/* Fill in chipset specific mode information */

static void s3_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    switch (modeinfo->colors) {
    case 16:			/* 4-plane 16 color mode */
	modeinfo->maxpixels = s3_memory * 1024 * 2;
	break;
    default:
	modeinfo->maxpixels = s3_memory * 1024 /
	    modeinfo->bytesperpixel;
    }

    /* Adjust line width (only for SVGA modes) */
    if (!IS_IN_STANDARD_VGA_DRIVER(mode))
	modeinfo->linewidth = s3_adjlinewidth(modeinfo->linewidth);

    modeinfo->maxlogicalwidth = 8184;
    if (s3_chiptype >= S3_801)
	modeinfo->startaddressrange = 0x3fffff;
    else
	modeinfo->startaddressrange = 0xfffff;

#ifdef S3_KLUDGE_PAGE_MODE
    if (mode == G320x200x256) {
	/* set page size to 256k. */
	modeinfo->startaddressrange /= 4;
	modeinfo->maxpixels /= 4;
    }
#else
    if (mode == G320x200x256) {
	/* disable page flipping. */
	/* modeinfo->startaddressrange = 0xffff; */
	modeinfo->startaddressrange = 0;
	modeinfo->maxpixels = 65536;
    }
#endif

    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;
    modeinfo->flags |= HAVE_EXT_SET;
#ifdef S3_LINEAR_SUPPORT
    if (modeinfo->bytesperpixel >= 1) {
    	modeinfo->flags |= CAPABLE_LINEAR;
    	if (s3_linear_addr)
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
#endif

    modeinfo->memory = s3_memory;
    modeinfo->chiptype = s3_chiptype;
}

/* 
 * XXX Part of this function should be implemented in ramdac.c,
 * but we just kludge it here for now.
 */
static int s3_ext_set(unsigned what, va_list params)
{
    int param2, old_values;
    unsigned char regs[10];

    /* only know this, for now */
    if (dac_used->id != ATT20C490 && dac_used->id != ATT20C498 &&
	dac_used->id != SIERRA_15025)
	return 0;

    param2 = va_arg(params, int);
    old_values = (s3_flags & S3_CLUT8_8) ? VGA_CLUT8 : 0;

    switch (what) {
    case VGA_EXT_AVAILABLE:
	switch (param2) {
	case VGA_AVAIL_SET:
	    return VGA_EXT_AVAILABLE | VGA_EXT_SET | VGA_EXT_CLEAR | VGA_EXT_RESET;
	case VGA_AVAIL_ACCEL:
	    return 0;
	case VGA_AVAIL_FLAGS:
	    return VGA_CLUT8;
	}
	break;

    case VGA_EXT_SET:
	if (param2 & VGA_CLUT8)
	    goto setclut8;

    case VGA_EXT_CLEAR:
	if (param2 & VGA_CLUT8)
	    goto clearclut8;

    case VGA_EXT_RESET:
	if (param2 & VGA_CLUT8) {
	  setclut8:
	    dac_used->saveState(regs);
	    if (regs[0] == 0x00) {	/* 8bpp, 6 bits/color */
		s3_flags |= S3_CLUT8_8;
		if (dac_used->id == SIERRA_15025)
		    regs[1] = 1;
		regs[0] = 0x02;
	    }
	    dac_used->restoreState(regs);
	    return old_values;
	} else {
	  clearclut8:
	    dac_used->saveState(regs);
	    if (regs[0] == 0x02) {	/* 8bpp, 8 bits/color */
		s3_flags &= ~S3_CLUT8_8;
		if (dac_used->id == SIERRA_15025)
		    regs[1] = 0;
		regs[0] = 0x00;
	    }
	    dac_used->restoreState(regs);
	    return old_values;
	}
    default:
        break;
    }
    return 0;
}

/* Return non-zero if mode is available */

static int s3_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeInfo *modeinfo;
    ModeTiming *modetiming;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    /* Enough memory? */
    info = &__svgalib_infotable[mode];
    if (s3_memory * 1024 < info->ydim * s3_adjlinewidth(info->xbytes))
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

/* 
 * save S3 registers.  Lock registers receive special treatment
 * so dumpreg will work under X.
 */
static int s3_saveregs(unsigned char regs[])
{
    unsigned char b, bmax;
    unsigned char cr38, cr39, cr40;

    cr38 = __svgalib_inCR(0x38);
    __svgalib_outCR(0x38, 0x48);		/* unlock S3 VGA regs (CR30-CR3B) */

    cr39 = __svgalib_inCR(0x39);
    __svgalib_outCR(0x39, 0xA5);		/* unlock S3 system control (CR40-CR4F) */
    /* and extended regs (CR50-CR6D) */

    cr40 = __svgalib_inCR(0x40);		/* unlock enhanced regs */
    __svgalib_outCR(0x40, cr40 | 0x01);

    /* retrieve values from private copy */
    memcpy(regs + S3_8514_OFFSET, s3_8514regs, S3_8514_COUNT * 2);

    /* get S3 VGA/Ext registers */
    bmax = 0x4F;
    if (s3_chiptype >= S3_801)
	bmax = 0x66;
    if (s3_chiptype >= S3_864)
	bmax = 0x6D;
    for (b = 0x30; b <= bmax; b++)
	regs[EXT + b - 0x30] = __svgalib_inCR(b);

    /* get S3 ext. SR registers */
    /* if (s3_chiptype >= S3_864) { */
    if (s3_chiptype == S3_TRIO32 || s3_chiptype == S3_TRIO64
    	|| s3_chiptype == S3_765) {/* SL: actually Trio32/64/V+ */
	regs[S3_SR08] = __svgalib_inSR(0x08);
	__svgalib_outSR(0x08, 0x06);	/* unlock extended seq regs */
	regs[S3_SR09] = __svgalib_inSR(0x09);
	regs[S3_SR0A] = __svgalib_inSR(0x0A);
	regs[S3_SR0D] = __svgalib_inSR(0x0D);
	regs[S3_SR10] = __svgalib_inSR(0x10);
	regs[S3_SR11] = __svgalib_inSR(0x11);
	regs[S3_SR12] = __svgalib_inSR(0x12);
	regs[S3_SR13] = __svgalib_inSR(0x13);
	regs[S3_SR15] = __svgalib_inSR(0x15);
	regs[S3_SR18] = __svgalib_inSR(0x18);
	__svgalib_outSR(0x08, regs[S3_SR08]);
    }

    dac_used->saveState(regs + S3_DAC_OFFSET);

    /* leave the locks the way we found it */
    __svgalib_outCR(0x40, regs[EXT + 0x40 - 0x30] = cr40);
    __svgalib_outCR(0x39, regs[EXT + 0x39 - 0x30] = cr39);
    __svgalib_outCR(0x38, regs[EXT + 0x38 - 0x30] = cr38);
#if 0
#include "ramdac/ibmrgb52.h"

    do {
	unsigned char m, n, df;

	fprintf(stderr,"pix_fmt = 0x%02X, 8bpp = 0x%02X, 16bpp = 0x%02X, 24bpp = 0x%02X, 32bpp = 0x%02X,\n"
	  "CR58 = 0x%02X, CR66 = 0x%02X, CR67 = 0x%02X, CR6D = 0x%02X\n",
	       regs[S3_DAC_OFFSET + IBMRGB_pix_fmt],
	       regs[S3_DAC_OFFSET + IBMRGB_8bpp],
	       regs[S3_DAC_OFFSET + IBMRGB_16bpp],
	       regs[S3_DAC_OFFSET + IBMRGB_24bpp],
	       regs[S3_DAC_OFFSET + IBMRGB_32bpp],
	       regs[S3_CR58],
	       regs[S3_CR66],
	       regs[S3_CR67],
	       regs[S3_CR6D]);

	m = regs[S3_DAC_OFFSET + IBMRGB_m0 + 4];
	n = regs[S3_DAC_OFFSET + IBMRGB_n0 + 4];
	df = m >> 6;
	m &= ~0xC0;

	fprintf(stderr,"m = 0x%02X %d, n = 0x%02X %d, df = 0x%02X %d, freq = %.3f\n",
	       m, m, n, n, df, df, ((m + 65.0) / n) / (8 >> df) * 16.0);
    } while (0);
#endif
    return S3_DAC_OFFSET - VGA_TOTAL_REGS + dac_used->stateSize;
}

/* Set chipset-specific registers */
static void s3_setregs(const unsigned char regs[], int mode)
{
    unsigned char b, bmax;
    /*
     * Right now, anything != 0x00 gets written in s3_setregs.
     * May change this into a bitmask later.
     */
    static unsigned char s3_regmask[] =
    {
	0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x00, 0x00,		/* CR30-CR37 */
	0x00, 0x00, 0x3A, 0x3B, 0x3C, 0x00, 0x00, 0x00,		/* CR38-CR3F */
	0x00, 0x00, 0x42, 0x43, 0x44, 0x45, 0x00, 0x00,		/* CR40-CR47 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,		/* CR48-CR4F */
	0x50, 0x51, 0x00, 0x00, 0x54, 0x55, 0x00, 0x00,		/* CR50-CR57 */
	0x58, 0x59, 0x5A, 0x00, 0x00, 0x5D, 0x5E, 0x00,		/* CR58-CR5F */
	0x60, 0x61, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00,		/* CR60-CR67 */
	0x00, 0x00, 0x6A, 0x00, 0x00, 0x00			/* CR68-CR6D */
    };

    s3_unlock_enh();

    /* save a private copy */
    memcpy(s3_8514regs, regs + S3_8514_OFFSET, S3_8514_COUNT * 2);
    /*
     * set this first, so if we segfault on this
     * we don't get a screwed up display
     */
    port_outw_r(ADVFUNC_CNTL, s3_8514regs[S3_ADVFUNC_CNTL]);

    /* get S3 VGA/Ext registers */
    bmax = 0x4F;
    if (s3_chiptype >= S3_801)
	bmax = 0x66;
    if (s3_chiptype >= S3_864)
	bmax = 0x6D;
    for (b = 0x30; b <= bmax; b++) {
	if (s3_regmask[b - 0x30])
	    __svgalib_outCR(b, regs[EXT + b - 0x30]);
    }

    if (dac_used->id != NORMAL_DAC) {
	unsigned char CR1;
	/* Blank the screen. */
	CR1 = __svgalib_inCR(0x01);
	__svgalib_outCR(0x01, CR1 | 0x20);

	__svgalib_outcrtc(0x55, __svgalib_inCR(0x55) | 1);
	__svgalib_outcrtc(0x66, regs[S3_CR66]);
	__svgalib_outcrtc(0x67, regs[S3_CR67]);	/* S3 pixmux. */

	dac_used->restoreState(regs + S3_DAC_OFFSET);

	__svgalib_outcrtc(0x6D, regs[S3_CR6D]);
	__svgalib_outcrtc(0x55, __svgalib_inCR(0x55) & ~1);

	__svgalib_outcrtc(0x01, CR1);	/* Unblank screen. */
    }
#ifdef S3_LINEAR_SUPPORT
    if (mode == TEXT && s3_linear_addr)
	s3_linear_disable();	/* make sure linear is off */
#endif

    /* restore CR38/39 (may lock other regs) */
    if (mode == TEXT) {
	/* restore lock registers as well */
	__svgalib_outCR(0x40, regs[S3_CR40]);
	__svgalib_outCR(0x39, regs[S3_CR39]);
	__svgalib_outCR(0x38, regs[S3_CR38]);
    } else
	s3_lock_enh();
}

/*
 * Initialize register state for a mode.
 */

static void s3_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo)
{
    /* Get current values. */
    s3_saveregs(moderegs);

    /* Set up the standard VGA registers for a generic SVGA. */
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    /* Set up the extended register values, including modifications */
    /* of standard VGA registers. */

    moderegs[VGA_SR0] = 0x03;
    moderegs[VGA_CR13] = modeinfo->lineWidth >> 3;
    moderegs[VGA_CR17] = 0xE3;

    if (modeinfo->lineWidth / modeinfo->bytesPerPixel == 2048)
	moderegs[S3_CR31] = 0x8F;
    else
	moderegs[S3_CR31] = 0x8D;
#ifdef S3_LINEAR_MODE_BANKING_864
    if (s3_chiptype >= S3_864) {
	/* moderegs[S3_ENHANCEDMODE] |= 0x01; */
	/* Enable enhanced memory mode. */
	moderegs[S3_CR31] |= 0x04;
	/* Enable banking via CR6A in linear mode. */
	moderegs[S3_CR31] |= 0x01;
    }
#endif
    moderegs[S3_CR32] = 0;
    moderegs[S3_CR33] = 0x20;
    moderegs[S3_CR34] = 0x10;	/* 1024 */
    moderegs[S3_CR35] = 0;
    /* Call cebank() here when setting registers. */
    if (modeinfo->bitsPerPixel >= 8) {
	moderegs[S3_CR3A] = 0xB5;
	if (s3_chiptype == S3_928)
	    /* ARI: Turn on CHAIN4 for 928, since __svgalib_setup_VGA_registers
							 initializes ModeX */
	    moderegs[VGA_CR14] = 0x60;
    } else {
	/* 16 color mode */
	moderegs[VGA_CR13] = modeinfo->lineWidth >> 1;
	moderegs[VGA_GR0] = 0x0F;
	moderegs[VGA_GR1] = 0x0F;
	moderegs[VGA_GR5] = 0x00;	/* write mode 0 */
	moderegs[VGA_AR11] = 0x00;
	moderegs[S3_CR3A] = 0x85;
    }

    moderegs[S3_CR3B] = (moderegs[VGA_CR0] + moderegs[VGA_CR4] + 1) / 2;
    moderegs[S3_CR3C] = moderegs[VGA_CR0] / 2;
    if (s3_chiptype == S3_911) {
	moderegs[S3_CR40] &= 0xF2;
	moderegs[S3_CR40] |= 0x09;
    } else if (s3_flags & S3_LOCALBUS) {
	moderegs[S3_CR40] &= 0xF2;
	/* Pegasus wants 0x01 for zero wait states. */
#ifdef S3_0_WAIT_805_864
	moderegs[S3_CR40] |= 0x09;	/* use fifo + 0 wait state */
#else
	moderegs[S3_CR40] |= 0x05;
#endif
    } else {
	moderegs[S3_CR40] &= 0xF6;
	moderegs[S3_CR40] |= 0x01;
    }

    if (modeinfo->bitsPerPixel >= 24) {
	/* 24/32 bit color */
	if (s3_chiptype == S3_864 || s3_chiptype == S3_964)
	    moderegs[S3_CR43] = 0x08;
	else if (s3_chiptype == S3_928 && dac_used->id == SIERRA_15025)
	    moderegs[S3_CR43] = 0x01;	/* ELSA Winner 1000 */
    } else if (modeinfo->bitsPerPixel >= 15) {
	/* 15/16 bit color */
	if (s3_chiptype <= S3_864 || s3_chiptype >= S3_866) {	/* XXXX Trio? */
	    moderegs[S3_CR43] = 0x08;
	    if (dac_used->id == IBMRGB52x)
		moderegs[S3_CR43] = 0x10;
	    else if (s3_chiptype == S3_928 && dac_used->id == SIERRA_15025)
		moderegs[S3_CR43] = 0x01;
	    if (s3_chiptype <= S3_924 && dac_used->id != NORMAL_DAC)
		moderegs[S3_CR43] = 0x01;

	} else
	    /* XXXX some DAC might need this; XF86 source says... */
	    moderegs[S3_CR43] = 0x09;
    } else {
	/* 4/8 bit color */
	moderegs[S3_CR43] = 0x00;
    }

    if (s3_chiptype >= S3_924 && s3_chiptype <= S3_928) {	/* different for 864+ */
	s3_8514regs[S3_ADVFUNC_CNTL] = 0x0002;
	if ((s3_chiptype == S3_928 && modeinfo->bitsPerPixel != 4) || !(s3_flags & S3_OLD_STEPPING))
	    s3_8514regs[S3_ADVFUNC_CNTL] |= 0x0001;
	if (modeinfo->bitsPerPixel == 4)
	    s3_8514regs[S3_ADVFUNC_CNTL] |= 0x0004;
#if 0
	/* 864 databook says it is for enhanced 4bpp */
	if (modeinfo->lineWidth > 640)
	    s3_8514regs[S3_ADVFUNC_CNTL] |= 0x0004;
#endif
    } else if (s3_chiptype == S3_968) {
	s3_8514regs[S3_ADVFUNC_CNTL] = 0x0002;
	if (modeinfo->bitsPerPixel == 4)
	    s3_8514regs[S3_ADVFUNC_CNTL] |= 0x0004;
#ifdef PIXEL_MULTIPLEXING
	else
	    s3_8514regs[S3_ADVFUNC_CNTL] |= 0x0001;
#endif
    } else if (modeinfo->lineWidth / modeinfo->bytesPerPixel == 1024)
    	s3_8514regs[S3_ADVFUNC_CNTL] = 0x0007;
    else
    	s3_8514regs[S3_ADVFUNC_CNTL] = 0x0003;

    moderegs[S3_CR44] = 0;
    /* Skip CR45, 'hi/truecolor cursor color enable'. */

    if (s3_chiptype >= S3_801) {
	int m, n;		/* for FIFO balancing */

	/* XXXX Not all chips support all widths. */
	moderegs[S3_CR50] &= ~0xF1;
	switch (modeinfo->bitsPerPixel) {
	case 16:
	    moderegs[S3_CR50] |= 0x10;
	    break;
	case 24:		/* XXXX 868/968 only */
	    if (s3_chiptype >= S3_868)
		moderegs[S3_CR50] |= 0x20;
	    break;
	case 32:
	    moderegs[S3_CR50] |= 0x30;
	    break;
	}

	switch (modeinfo->lineWidth / modeinfo->bytesPerPixel) {
	case 640:
	    moderegs[S3_CR50] |= 0x40;
	    break;
	case 800:
	    moderegs[S3_CR50] |= 0x80;
	    break;
	case 1152:
	    if (!(s3_flags & S3_OLD_STEPPING)) {
		moderegs[S3_CR50] |= 0x01;
		break;
	    }			/* else fall through */
	case 1280:
	    moderegs[S3_CR50] |= 0xC0;
	    break;
	case 1600:
	    moderegs[S3_CR50] |= 0x81;
	    break;
	    /* 1024/2048 no change. */
	}

	moderegs[S3_CR51] &= 0xC0;
	moderegs[S3_CR51] |= (modeinfo->lineWidth >> 7) & 0x30;

	/* moderegs[S3_CR53] |= 0x10; *//* Enable MMIO. */
	/* moderegs[S3_CR53] |= 0x20; *//* DRAM interleaving for S3_805i with 2MB */

	n = 0xFF;
	if (s3_chiptype >= S3_864 ||
	    s3_chiptype == S3_801 || s3_chiptype == S3_805) {
	    /* 
	     * CRT FIFO balancing for DRAM cards and 964/968
	     * in VGA mode.
	     */
	    int clock, mclk;
	    if (modeinfo->bitsPerPixel < 8) {
		clock = modetiming->pixelClock;
	    } else {
		clock = modetiming->pixelClock *
		    modeinfo->bytesPerPixel;
	    }
	    if (s3_memory < 2048 || s3_chiptype == S3_TRIO32)
		clock *= 2;
	    if (__svgalib_s3_s3Mclk > 0)
	    	mclk = __svgalib_s3_s3Mclk;
	    else if (s3_chiptype == S3_801 || s3_chiptype == S3_805)
		mclk = 50000;	/* Assumption. */
	    else
		mclk = 60000;	/* Assumption. */
	    m = (int) ((mclk / 1000.0 * .72 + 16.867) * 89.736 / (clock / 1000.0 + 39) - 21.1543);
	    if (s3_memory < 2048 || s3_chiptype == S3_TRIO32)
		m /= 2;
	    if (m > 31)
		m = 31;
	    else if (m < 0) {
		m = 0;
		n = 16;
	    }
	} else if (s3_memory == 512 || modetiming->HDisplay > 1200)
	    m = 0;
	else if (s3_memory == 1024)
	    m = 2;
	else
	    m = 20;
	
	moderegs[S3_CR54] = m << 3;
	moderegs[S3_CR60] = n;

	moderegs[S3_CR55] &= 0x08;
	moderegs[S3_CR55] |= 0x40;

#ifdef S3_LINEAR_MODE_BANKING_864
	if (s3_chiptype >= S3_864) {
	    if (modeinfo->bitsPerPixel >= 8) {
		/* Enable linear addressing. */
		moderegs[S3_CR58] |= 0x10;
		/* Set window size to 64K. */
		moderegs[S3_CR58] &= ~0x03;
		/* Assume CR59/5A are correctly set up for 0xA0000. */
		/* Set CR6A linear bank to zero. */
		moderegs[S3_CR6A] &= ~0x3F;
		/* use alternate __svgalib_setpage() function */
		__svgalib_s3_driverspecs.__svgalib_setpage = s3_setpage864;
	    } else {
		/* doesn't work for 4bpp. */
		__svgalib_s3_driverspecs.__svgalib_setpage = s3_setpage;
	    }
	}
#endif
#ifdef S3_LINEAR_SUPPORT
	moderegs[S3_CR59] = s3_cr59;
	moderegs[S3_CR5A] = s3_cr5A;
#endif

	/* Extended CRTC timing. */
	moderegs[S3_CR5E] =
	    (((modetiming->CrtcVTotal - 2) & 0x400) >> 10) |
	    (((modetiming->CrtcVDisplay - 1) & 0x400) >> 9) |
	    (((modetiming->CrtcVSyncStart) & 0x400) >> 8) |
	    (((modetiming->CrtcVSyncStart) & 0x400) >> 6) | 0x40;

	{
	    int i, j;
	    i = ((((modetiming->CrtcHTotal >> 3) - 5) & 0x100) >> 8) |
		((((modetiming->CrtcHDisplay >> 3) - 1) & 0x100) >> 7) |
		((((modetiming->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6) |
		((modetiming->CrtcHSyncStart & 0x800) >> 7);
	    if ((modetiming->CrtcHSyncEnd >> 3) - (modetiming->CrtcHSyncStart >> 3) > 64)
	    	i |= 0x08;
	    if ((modetiming->CrtcHSyncEnd >> 3) - (modetiming->CrtcHSyncStart >> 3) > 32)
	    	i |= 0x20;
	    j = ((moderegs[VGA_CR0] + ((i & 0x01) << 8) +
		  moderegs[VGA_CR4] + ((i & 0x10) << 4) + 1) / 2);
	    if (j - (moderegs[VGA_CR4] + ((i & 0x10) << 4)) < 4) {
	    	if (moderegs[VGA_CR4] + ((i & 0x10) << 4) + 4 <= moderegs[VGA_CR0] + ((i & 0x01) << 8))
	    	    j = moderegs[VGA_CR4] + ((i & 0x10) << 4) + 4;
	    	else
	    	    j = moderegs[VGA_CR0] + ((i & 0x01) << 8) + 1;
	    }
            
	    moderegs[S3_CR3B] = j & 0xFF;
	    i |= (j & 0x100) >> 2;
	    /* Interlace mode frame offset. */
	    moderegs[S3_CR3C] = (moderegs[VGA_CR0] + ((i & 0x01) << 8)) / 2;
	    moderegs[S3_CR5D] = (moderegs[S3_CR5D] & 0x80) | i;
	}

	{
	    int i;

	    if (modeinfo->bitsPerPixel < 8)
		i = modetiming->HDisplay / 4 + 1;
	    else
		i = modetiming->HDisplay *
		    modeinfo->bytesPerPixel / 4 + 1;

	    moderegs[S3_CR61] = (i >> 8) | 0x80;
	    moderegs[S3_CR62] = i & 0xFF;
	}
    }				/* 801+ */
    if (modetiming->flags & INTERLACED)
	moderegs[S3_CR42] |= 0x20;

    /*
     * Clock select works as follows:
     * Clocks 0 and 1 (VGA 25 and 28 MHz) can be selected via the
     * two VGA MiscOutput clock select bits.
     * If 0x3 is written to these bits, the selected clock index
     * is taken from the S3 clock select register at CR42. Clock
     * indices 0 and 1 should correspond to the VGA ones above,
     * and 3 is often 0 MHz, followed by extended clocks for a
     * total of mostly 16.
     */

    if (modetiming->flags & USEPROGRCLOCK)
	moderegs[VGA_MISCOUTPUT] |= 0x0C;	/* External clock select. */
    else if (modetiming->selectedClockNo < 2) {
	/* Program clock select bits 0 and 1. */
	moderegs[VGA_MISCOUTPUT] &= ~0x0C;
	moderegs[VGA_MISCOUTPUT] |=
	    (modetiming->selectedClockNo & 3) << 2;
    } else if (modetiming->selectedClockNo >= 2) {
	moderegs[VGA_MISCOUTPUT] |= 0x0C;
	/* Program S3 clock select bits. */
	moderegs[S3_CR42] &= ~0x1F;
	moderegs[S3_CR42] |=
	    modetiming->selectedClockNo;
    }
    if (s3_chiptype == S3_TRIO64 || s3_chiptype == S3_765) {
	moderegs[S3_CR33] &= ~0x08;
	if (modeinfo->bitsPerPixel == 16)
	    moderegs[S3_CR33] |= 0x08;
	/*
	 * The rest of the DAC/clocking is setup by the
	 * Trio64 code in the RAMDAC interface (ramdac.c).
	 */
    }
    if (dac_used->id != NORMAL_DAC) {
	int colormode;
	colormode = __svgalib_colorbits_to_colormode(modeinfo->bitsPerPixel,
					   modeinfo->colorBits);
	dac_used->initializeState(&moderegs[S3_DAC_OFFSET],
				  modeinfo->bitsPerPixel, colormode,
				  modetiming->pixelClock);

	if (dac_used->id == ATT20C490) {
	    int pixmux, invert_vclk, blank_delay;
	    pixmux = 0;
	    invert_vclk = 0;
	    blank_delay = 2;
	    if (colormode == CLUT8_6
		&& modetiming->pixelClock >= 67500) {
		pixmux = 0x00;
		invert_vclk = 1;
	    } else if (colormode == CLUT8_8)
		pixmux = 0x02;
	    else if (colormode == RGB16_555)
		pixmux = 0xa0;
	    else if (colormode == RGB16_565)
		pixmux = 0xc0;
	    else if (colormode == RGB24_888_B)
		pixmux = 0xe0;
	    moderegs[S3_CR67] = pixmux | invert_vclk;
	    moderegs[S3_CR6D] = blank_delay;
	}
	if (dac_used->id == S3_SDAC) {
	    int pixmux, invert_vclk, blank_delay;
	    pixmux = 0;
	    invert_vclk = 0;
	    blank_delay = 0;
	    if (colormode == CLUT8_6
		&& modetiming->pixelClock >= 67500) {
#ifdef SDAC_8BPP_PIXMUX
		/* x64 8bpp pixel multiplexing? */
		pixmux = 0x10;
		if (s3_chiptype != S3_866 && s3_chiptype != S3_868)
		    invert_vclk = 1;
		blank_delay = 2;
#endif
	    } else if (colormode == RGB16_555) {
		pixmux = 0x30;
		blank_delay = 2;
	    } else if (colormode == RGB16_565) {
		pixmux = 0x50;
		blank_delay = 2;
	    } else if (colormode == RGB24_888_B) {	/* XXXX 868/968 only */
		pixmux = 0x90;
		blank_delay = 2;
	    } else if (colormode == RGB32_888_B) {
		pixmux = 0x70;
		blank_delay = 2;
	    }
	    moderegs[S3_CR67] = pixmux | invert_vclk;
	    moderegs[S3_CR6D] = blank_delay;
	    /* Clock select. */
	    moderegs[S3_CR42] &= ~0x0F;
	    moderegs[S3_CR42] |= 0x02;
	}
	if (dac_used->id == IBMRGB52x) {
	    unsigned char pixmux, blank_delay, tmp;
	    tmp = 0;
	    pixmux = 0x11;
	    blank_delay = 0;
	    if (modeinfo->bitsPerPixel < 8 || colormode == RGB32_888_B)
		pixmux = 0x00;
	    moderegs[S3_CR58] |= 0x40;
	    moderegs[S3_CR65] = 0;
	    moderegs[S3_CR66] &= 0xf8;
	    moderegs[S3_CR66] |= tmp;
#ifdef PIXEL_MULTIPLEXING
	    moderegs[S3_CR67] = pixmux;
#endif
	    moderegs[S3_CR6D] = blank_delay;
	    /* Clock select. */
	    moderegs[S3_CR42] &= ~0x0F;
	    moderegs[S3_CR42] |= 0x02;
	}
    }
#ifdef S3_LINEAR_SUPPORT
    s3_cr58 = moderegs[S3_CR58];
    s3_cr40 = moderegs[S3_CR40];
    s3_cr54 = moderegs[S3_CR54];
#endif
    if (clk_used == &__svgalib_I2061A_clockchip_methods &&
	(modetiming->flags & USEPROGRCLOCK)) {
	/* Clock select. */
	moderegs[S3_CR42] &= ~0x0F;
	moderegs[S3_CR42] |= 0x02;
    }
    /* update the 8514 regs */
    memcpy(moderegs + S3_8514_OFFSET, s3_8514regs, S3_8514_COUNT * 2);
}


/* Set a mode */

static int s3_setmode(int mode, int prv_mode)
{
    ModeInfo *modeinfo;
    ModeTiming *modetiming;
    unsigned char* moderegs;
    int res;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
	/* Let the standard VGA driver set standard VGA modes. */
	res = __svgalib_vga_driverspecs.setmode(mode, prv_mode);
	if (res == 0) {
	    /*
	     * ARI: Turn off virtual size of 1024 - this fixes all problems
	     *      with standard modes, including 320x200x256.
	     * 
	     * SL:  Is this for 928 only?  Doesn't matter for 805.
             *
             * MZ:  Affects 765 as well, so I assume it is good for all chipsets.
	     */
	    s3_unlock();
	    __svgalib_outCR(0x34, __svgalib_inCR(0x34) & ~0x10);
	    s3_lock();
	}
	return res;
    }
    if (!s3_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }
    /* Adjust the display width. */
    modeinfo->lineWidth = s3_adjlinewidth(modeinfo->lineWidth);
    CI.xbytes = modeinfo->lineWidth;

    moderegs = malloc(S3_TOTAL_REGS);

    s3_initializemode(moderegs, modetiming, modeinfo);
    free(modeinfo);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    s3_setregs(moderegs, mode);	/* Set extended regs. */

    free(moderegs);

    return 0;
}


/* Indentify chipset; return non-zero if detected */

/* Some port I/O functions: */
static unsigned char rdinx(int port, unsigned char index)
{
    port_out_r(port, index);
    return port_in(port + 1);
}

static void wrinx(int port, unsigned char index, unsigned char val)
{
    port_out_r(port, index);
    port_out_r(port + 1, val);
}

/*
 * Returns true iff the bits in 'mask' of register 'port', index 'index'
 * are read/write.
 */
static int testinx2(int port, unsigned char index, unsigned char mask)
{
    unsigned char old, new1, new2;

    old = rdinx(port, index);
    wrinx(port, index, (old & ~mask));
    new1 = rdinx(port, index) & mask;
    wrinx(port, index, (old | mask));
    new2 = rdinx(port, index) & mask;
    wrinx(port, index, old);
    return (new1 == 0) && (new2 == mask);
}

static int s3_test(void)
{
    int vgaIOBase, vgaCRIndex, vgaCRReg;

    vgaIOBase = (port_in(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;
    vgaCRIndex = vgaIOBase + 4;
    vgaCRReg = vgaIOBase + 5;

    port_out_r(vgaCRIndex, 0x11);	/* for register CR11, (Vertical Retrace End) */
    port_out_r(vgaCRReg, 0x00);	/* set to 0 */

    port_out_r(vgaCRIndex, 0x38);	/* check if we have an S3 */
    port_out_r(vgaCRReg, 0x00);

    /* Make sure we can't write when locked */

    if (testinx2(vgaCRIndex, 0x35, 0x0f))
	return 0;

    port_out_r(vgaCRIndex, 0x38);	/* for register CR38, (REG_LOCK1) */
    port_out_r(vgaCRReg, 0x48);	/* unlock S3 register set for read/write */

    /* Make sure we can write when unlocked */

    if (!testinx2(vgaCRIndex, 0x35, 0x0f))
	return 0;

    if (s3_init(0, 0, 0))	/* type not OK */
	return 0;
    return 1;
}

/*
 * Bank switching function - set 64K bank number
 * 
 * XXXX locking and unlocking might hurt performance but is safer.
 */
static void s3_setpage(int page)
{
#ifdef S3_16_COLORS
    /* 
     * XXXX adjust the parameter for 4bpp (1bpp is ignored).  Shouldn't
     * need this, but either me or the drawing functions are making bad
     * assumptions about 4bpp.
     */
    if (infotable[CM].bytesperpixel == 0)
	page *= 4;
#endif
#ifdef S3_KLUDGE_PAGE_MODE
    /* adjust to use 256K pages */
    if (CM == G320x200x256)
	page *= 4;
#endif
    s3_unlock();
    port_out_r(CRT_IC, 0x35);
    port_out_r(CRT_DC, (port_in(CRT_DC) & 0xF0) | (page & 0x0F));
    if (s3_chiptype >= S3_801) {
	port_out_r(CRT_IC, 0x51);
	port_out_r(CRT_DC, (port_in(CRT_DC) & ~0x0C) | ((page & 0x30) >> 2));
    }
    port_in(CRT_DC);			/* ARI: Ferraro says: required for first generation 911 only */
    s3_lock();
}

/*
 * Bank switching function - set 64K bank number for 864+
 * (not for 4bpp)
 * 
 * XXXX locking and unlocking might hurt performance
 * (864 shouldn't need it).
 */
#ifdef S3_LINEAR_MODE_BANKING_864
static void s3_setpage864(int page)
{
    s3_unlock();
    /* "Linear" mode banking. */
    port_out_r(CRT_IC, 0x6A);
    port_out_r(CRT_DC, (port_in(CRT_DC) & ~0x3F) | page);
    s3_lock();
}

#endif

/*
 * Set display start address (not for 16 color modes).
 *
 * This works up to 4Mb (should be able to go higher).
 * 
 * XXXX locking and unlocking might hurt performance but is safer.
 */
static void s3_setdisplaystart(int address)
{
#ifdef S3_KLUDGE_PAGE_MODE
    /* adjust to use 256K pages */
    if (CM == G320x200x256)
	address *= 4;
#endif
    s3_unlock();
    port_outw_r(CRT_IC, 0x0d | ((address << 6) & 0xff00));	/* sa2-sa9 */
    port_outw_r(CRT_IC, 0x0c | ((address >> 2) & 0xff00));	/* sa10-sa17 */
    port_in(0x3da);			/* set ATC to addressing mode */
    port_out_r(ATT_IW, 0x13 + 0x20);	/* select ATC reg 0x13 */
    port_out_r(ATT_IW, (port_in(ATT_R) & 0xf0) | ((address & 3) << 1));
    /* write sa0-1 to bits 1-2 */

    port_out_r(CRT_IC, 0x31);
    port_out_r(CRT_DC, (port_in(CRT_DC) & ~0x30) | ((address & 0xc0000) >> 14));
    if (s3_chiptype >= S3_801) {
	port_out_r(CRT_IC, 0x51);
	port_out_r(CRT_DC, (port_in(CRT_DC) & ~0x03) | ((address & 0x300000) >> 20));
    }
    s3_lock();
}

/*
 * Set logical scanline length (Multiples of 8 to 8184).
 * CR43.2 should be 0 for this.
 */
static void s3_setlogicalwidth(int width)
{
    __svgalib_outCR(0x13, (width >> 3));	/* lw3-lw11 */
    __svgalib_outCR(0x51, (width & 0x300) >> 4);	/* lw12-lw13 */
}

#ifdef S3_LINEAR_SUPPORT
static void s3_linear_enable(void)
{
    s3_unlock();
    
    if (s3_chiptype > S3_924) {
    	int i;
    	port_out_r (CRT_IC, 0x40);
    	i = (s3_cr40 & 0xf6) | 0x0a;
    	port_out_r (CRT_DC, (unsigned char) i);
    	port_out_r (CRT_IC, 0x58);
    	port_out_r (CRT_DC, s3_linear_opt | s3_cr58);
    	if (s3_chiptype > S3_928) {
    	    port_out_r (CRT_IC, 0x54);
    	    port_out_r (CRT_DC, (s3_cr54 + 0x07));
    	}
    }
    
    s3_lock();
}

static void s3_linear_disable(void)
{
    s3_unlock();
    
    if (s3_chiptype > S3_924) {
    	if (s3_chiptype > S3_928) {
    	    port_out_r (CRT_IC, 0x54);
    	    port_out_r (CRT_DC, s3_cr54);
    	}
    	port_out_r (CRT_IC, 0x58);
    	port_out_r (CRT_DC, s3_cr58);
    	port_out_r (CRT_IC, 0x40);
    	port_out_r (CRT_DC, s3_cr40);
    }
    
    s3_lock();
}

/* Set linear addressing mode */

static int s3_linear(int op, int param)
{
    if (op == LINEAR_QUERY_BASE)
    	return s3_linear_base;
    if (op == LINEAR_QUERY_GRANULARITY) {
	switch (s3_memory) {
	case 4096:
	case 2048:
	case 1024:
	    return s3_memory * 1024;
	default:
	    return 1024 * 1024;
	}
    } else if (op == LINEAR_QUERY_RANGE)
	return 256;
    else if (op == LINEAR_ENABLE) {
	s3_setpage(0);
	s3_linear_enable();
	s3_linear_addr = param;
	return 0;
    } else if (op == LINEAR_DISABLE) {
	s3_setpage(0);
	s3_linear_disable();
	s3_linear_addr = 0;
	return 0;
    } else
	return -1;
}

#define S3_LINEAR_FUNC s3_linear
#else
#define S3_LINEAR_FUNC 0
#endif				/* S3_LINEAR_SUPPORT */

/* Function table (exported) */

DriverSpecs __svgalib_s3_driverspecs =
{
    s3_saveregs,		/* saveregs */
    s3_setregs,			/* setregs */
    (void (*)(void)) nothing,	/* unlock */
    (void (*)(void)) nothing,	/* lock */
    s3_test,
    s3_init,
    s3_setpage,
    (void (*)(int)) nothing,
    (void (*)(int)) nothing,
    s3_setmode,
    s3_modeavailable,
    s3_setdisplaystart,
    s3_setlogicalwidth,
    s3_getmodeinfo,
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    s3_ext_set,			/* extset */
    0,				/* accel */
    S3_LINEAR_FUNC,		/* linear */
    NULL,                       /* Accelspecs */
    NULL,                       /* Emulation */
};


/* S3-specific config file options. */

/*
 * Currently this only handles Clocks. It would a good idea to have
 * higher-level code process options like Clocks that are valid for
 * more than one driver driver (with better error detection etc.).
 */

static char *s3_config_options[] =
{
    "clocks", "ramdac", "dacspeed", "clockchip", NULL
};

static char *s3_process_option(int option, int mode, char **nptr)
{
/*
 * option is the number of the option string in s3_config_options,
 * mode seems to be a 'hardness' indicator for security.
 */
    if (option == 0) {		/* "Clocks" */
	/* Process at most 16 specified clocks. */
	cardspecs->clocks = malloc(sizeof(int) * 16);
	/* cardspecs->nClocks should be already be 0. */
	for (;;) {
	    char *ptr;
	    int freq;
	    ptr = __svgalib_token(nptr);
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
	ptr = __svgalib_token(nptr);
#ifdef INCLUDE_IBMRGB52x_DAC
	if (strcasecmp(ptr, "IBMRGB52x") == 0)
	    dac_used = &__svgalib_IBMRGB52x_methods;
#endif
#ifdef INCLUDE_SIERRA_DAC
	if (strcasecmp(ptr, "Sierra32K") == 0)
	    dac_used = &__svgalib_Sierra_32K_methods;
#endif
#ifdef INCLUDE_SC15025_DAC
	if (strcasecmp(ptr, "SC15025") == 0)
	    dac_used = &__svgalib_SC15025_methods;
#endif
#ifdef INCLUDE_S3_SDAC_DAC
	if (strcasecmp(ptr, "SDAC") == 0)
	    dac_used = &__svgalib_S3_SDAC_methods;
#endif
#ifdef INCLUDE_S3_GENDAC_DAC
	if (strcasecmp(ptr, "GenDAC") == 0)
	    dac_used = &__svgalib_S3_GENDAC_methods;
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

	if (clk_used)
	    clk_used->initialize(cardspecs, dac_used);
    }
    if (option == 2) {		/* "Dacspeed" */
	char *ptr;
	ptr = __svgalib_token(nptr);
	/*
	 * This doesn't protect against bad characters
	 * (atoi() doesn't detect errors).
	 */
	dac_speed = atoi(ptr) * 1000;
    }
    if (option == 3) {		/* "ClockChip" */
	char *ptr;
	long freq;
	ptr = strtok(NULL, " \t");
	if (strcasecmp(ptr, "ICD2061A") == 0 ||
	    strcasecmp(ptr, "DCS2824") == 0)	/* Diamond, compatible to icd2061a */
	    clk_used = &__svgalib_I2061A_clockchip_methods;
	clk_used->initialize(cardspecs, dac_used);
	ptr = strtok(NULL, " \t");
	if (ptr != NULL) {
	    freq = atof(ptr) * 1000L;
	    if (freq) {
		clk_used->TextFrequency = freq;
		ptr = strtok(NULL, " \t");
	    }
	}
	return ptr;
    }
    return __svgalib_token(nptr);
}


/* Initialize driver (called after detection) */
/* Derived from XFree86 SuperProbe and s3 driver. */

static DacMethods *dacs_to_probe[] =
{
#ifdef INCLUDE_S3_SDAC_DAC_TEST
    &__svgalib_S3_SDAC_methods,
#endif
#ifdef INCLUDE_S3_GENDAC_DAC_TEST
    &__svgalib_S3_GENDAC_methods,
#endif
#ifdef INCLUDE_ATT20C490_DAC_TEST
    &__svgalib_ATT20C490_methods,
#endif
#ifdef INCLUDE_SC15025_DAC_TEST
    &__svgalib_SC15025_methods,
#endif
#ifdef INCLUDE_SC1148X_DAC_TEST
    &__svgalib_SC1148X_methods,
#endif
#ifdef INCLUDE_IBMRGB52x_DAC_TEST
    &__svgalib_IBMRGB52x_methods,
#endif
    NULL};

static int s3_init(int force, int par1, int par2)
{
    int id, rev, config;

    s3_unlock();

    s3_flags = 0;		/* initialize */
    id = __svgalib_inCR(0x30);		/* Get chip id. */
    rev = id & 0x0F;
    if (id >= 0xE0) {
	id |= __svgalib_inCR(0x2E) << 8;
	rev |= __svgalib_inCR(0x2F) << 4;
    }
    if (force) {
	s3_chiptype = par1;	/* we already know the type */
	s3_memory = par2;
	/* ARI: can we really trust the user's specification, or should we ignore
	   it and probe ourselves ? */
	if (s3_chiptype == S3_801 || s3_chiptype == S3_805) {
	    if ((rev & 0x0F) < 2)
		s3_flags |= S3_OLD_STEPPING;	/* can't handle 1152 width */
	} else if (s3_chiptype == S3_928) {
	    if ((rev & 0x0F) < 4)	/* ARI: Stepping D or below */
		s3_flags |= S3_OLD_STEPPING;	/* can't handle 1152 width */
	}
    } else {
	s3_chiptype = -1;
	config = __svgalib_inCR(0x36);	/* get configuration info */
	switch (id & 0xf0) {
	case 0x80:
	    if (rev == 1) {
		s3_chiptype = S3_911;
		break;
	    }
	    if (rev == 2) {
		s3_chiptype = S3_924;
		break;
	    }
	    break;
	case 0xa0:
	    switch (config & 0x03) {
	    case 0x00:
	    case 0x01:
		/* EISA or VLB - 805 */
		s3_chiptype = S3_805;
		/* ARI: Test stepping: 0:B, 1:unknown, 2,3,4:C, 8:I, >=5:D */
		if ((rev & 0x0F) < 2)
		    s3_flags |= S3_OLD_STEPPING;	/* can't handle 1152 width */
		break;
	    case 0x03:
		/* ISA - 801 */
		s3_chiptype = S3_801;
		/* Stepping same as 805, just ISA */
		if ((rev & 0x0F) < 2)
		    s3_flags |= S3_OLD_STEPPING;	/* can't handle 1152 width */
		break;
	    }
	    break;
	case 0x90:
	    s3_chiptype = S3_928;
	    if ((rev & 0x0F) < 4)	/* ARI: Stepping D or below */
		s3_flags |= S3_OLD_STEPPING;	/* can't handle 1152 width */
	    break;
	case 0xB0:
	    /* 928P */
	    s3_chiptype = S3_928;
	    break;
	case 0xC0:
	    s3_chiptype = S3_864;
	    break;
	case 0xD0:
	    s3_chiptype = S3_964;
	    break;
	case 0xE0:
	    switch (id & 0xFFF0) {
	    case 0x10E0:
		s3_chiptype = S3_TRIO32;
		break;
	    case 0x3DE0: /* ViRGE/VX ID */
	    case 0x31E0: /* ViRGE ID */
	    case 0x01E0: /* S3Trio64V2/DX ... any others? */
            case 0x04E0:
	    case 0x11E0:
		if (rev & 0x0400)
		    s3_chiptype = S3_765;
		else
		    s3_chiptype = S3_TRIO64;
		break;
	    case 0x80E0:
		s3_chiptype = S3_866;
		break;
	    case 0x90E0:
		s3_chiptype = S3_868;
		break;
	    case 0xF0E0:	/* XXXX From data book; XF86 says 0xB0E0? */
		s3_chiptype = S3_968;
		break;
	    }
	}
	if (s3_chiptype == -1) {
	    fprintf(stderr,"svgalib: S3: Unknown chip id %02x\n",
		   id);
	    return -1;
	}
	if (s3_chiptype <= S3_924) {
	    if ((config & 0x20) != 0)
		s3_memory = 512;
	    else
		s3_memory = 1024;
	} else {
	    /* look at bits 5, 6 and 7 */
	    switch ((config & 0xE0) >> 5) {
	    case 0:
		s3_memory = 4096;
		break;
	    case 2:
		s3_memory = 3072;
		break;
	    case 3:
		s3_memory = 8192;
		break;
	    case 4:
		s3_memory = 2048;
		break;
	    case 5:
		s3_memory = 6144;
		break;
	    case 6:
		s3_memory = 1024;
		break;
	    case 7:
		s3_memory = 512;
		break;		/* Trio32 */
	    }
	}

	if ((config & 0x03) < 3)	/* XXXX 928P is ignored */
	    s3_flags |= S3_LOCALBUS;
    }

    if (__svgalib_driver_report) {
	fprintf(stderr,"svgalib: Using S3 driver (%s, %dK).\n", s3_chipname[s3_chiptype],
	       s3_memory);
	if (s3_flags & S3_OLD_STEPPING)
	    fprintf(stderr,"svgalib: Chip revision cannot handle modes with width 1152.\n");
	if (s3_chiptype > S3_TRIO64) {
	    fprintf(stderr,"svgalib: s3: chipsets newer than S3 Trio64 is not supported well yet.\n");
	}
    }
/* begin: Initialize cardspecs. */
#ifdef S3_LINEAR_SUPPORT
    if (s3_chiptype > S3_805) {
    	int found_pciconfig;
    	unsigned long pci_conf[64];
    	
    	found_pciconfig = __svgalib_pci_find_vendor_vga(0x5333, pci_conf, 0);
    	if (!found_pciconfig)
    	    s3_linear_base = pci_conf[4] & 0xFF800000;
    }
    
    s3_cr59 = s3_linear_base >> 24;
    s3_cr5A = (s3_linear_base >> 16);
    if (! (s3_cr59 | s3_cr5A)) {
    	s3_cr59 = __svgalib_inCR(0x59);
    	s3_cr5A = __svgalib_inCR(0x5A);
    	if (!s3_cr59) {
    	    s3_cr59 =  0xF3000000 >> 24;
    	    s3_cr5A = (0xF3000000 >> 16);
    	}
    	s3_linear_base = (s3_cr59 << 24) | (s3_cr5A << 16);
    }
    s3_linear_opt |= 0x10;
    switch (s3_memory) {
    	case 512 :
    	case 1024 : 
    	    s3_linear_opt |= 0x01;
    	    break;
    	case 2048 :
    	    s3_linear_opt |= 0x02;
    	    break;
    	case 3072 :
    	case 4096 :
    	case 6144 :
    	case 8192 :
    	    s3_linear_opt |= 0x03;
    	    break;
    	default :
    	    s3_linear_opt = 0x14;	/* like XFree */
    }
#endif
    
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = s3_memory;
    cardspecs->nClocks = 0;
    /* cardspecs->maxHorizontalCrtc = 2040; SL: kills 800x600x32k and above */
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->flags = INTERLACE_DIVIDE_VERT;

    /* Process S3-specific config file options. */
    __svgalib_read_options(s3_config_options, s3_process_option);

#ifdef INCLUDE_S3_TRIO64_DAC
    if ((s3_chiptype == S3_TRIO64 || s3_chiptype == S3_765) && dac_used == NULL)
	dac_used = &__svgalib_Trio64_methods;
#endif

    if (dac_used == NULL)
	dac_used = __svgalib_probeDacs(dacs_to_probe);
    else
	dac_used->initialize();


    if (dac_used == NULL) {
	/* Not supported. */
	fprintf(stderr,"svgalib: s3: Assuming normal VGA DAC.\n");
#ifdef INCLUDE_NORMAL_DAC
	dac_used = &__svgalib_normal_dac_methods;
	dac_used->initialize();
#else
	fprintf(stderr,"svgalib: Alas, normal VGA DAC support is not compiled in, goodbye.\n");
	return 1;
#endif
    }
    if (clk_used)
	clk_used->initialize(cardspecs, dac_used);

    dac_used->qualifyCardSpecs(cardspecs, dac_speed);

    /* Initialize standard clocks for unknown DAC. */
    if ((!(cardspecs->flags & CLOCK_PROGRAMMABLE))
	&& cardspecs->nClocks == 0) {
	/*
	 * Almost all cards have 25 and 28 MHz on VGA clocks 0 and 1,
	 * so use these for an unknown DAC, yielding 640x480x256.
	 */
	cardspecs->nClocks = 2;
	cardspecs->clocks = malloc(sizeof(int) * 2);
	cardspecs->clocks[0] = 25175;
	cardspecs->clocks[1] = 28322;
    }
    /* Limit pixel clocks according to chip specifications. */
    if (s3_chiptype == S3_864 || s3_chiptype == S3_868) {
	/* Limit max clocks according to 95 MHz DCLK spec. */
	/* SL: might just be 95000 for 4/8bpp since no pixmux'ing */
	LIMIT(cardspecs->maxPixelClock4bpp, 95000 * 2);
	LIMIT(cardspecs->maxPixelClock8bpp, 95000 * 2);
	LIMIT(cardspecs->maxPixelClock16bpp, 95000);
	/* see explanation below */
	LIMIT(cardspecs->maxPixelClock24bpp, 36000);
	/*
	 * The official 32bpp limit is 47500, but we allow
	 * 50 MHz for VESA 800x600 timing (actually the
	 * S3-864 doesn't have the horizontal timing range
	 * to run unmodified VESA 800x600 72 Hz timings).
	 */
	LIMIT(cardspecs->maxPixelClock32bpp, 50000);
    }
#ifndef S3_16_COLORS
    cardspecs->maxPixelClock4bpp = 0;	/* 16-color doesn't work. */
#endif

/* end: Initialize cardspecs. */

    __svgalib_driverspecs = &__svgalib_s3_driverspecs;

    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
#ifdef S3_LINEAR_SUPPORT
    __svgalib_linear_mem_base=s3_linear_base;
    __svgalib_linear_mem_size=s3_memory*0x400;
#endif

    return 0;
}
