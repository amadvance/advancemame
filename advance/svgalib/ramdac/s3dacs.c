/*
 * s3dacs.c:
 * 
 * RAMDAC definitions for the S3-SDAC (86C716), S3-GENDAC, and Trio64.
 *
 * These contain S3-specific code.
 */

#include <stdio.h>
#include "libvga.h"
#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

/* SDAC/GENDAC registers */
#if defined(INCLUDE_S3_SDAC_DAC) || defined(INCLUDE_S3_GENDAC_DAC)
#define SDAC_COMMAND		0	/* Register offsets into state. */
#define GENDAC_COMMAND		0
#define SDAC_PLL_WRITEINDEX	1
#define SDAC_PLL_READINDEX	2
#define SDAC_PLL_M		3	/* f2 programmed clock */
#define SDAC_PLL_N1_N2		4
#define SDAC_PLL_CONTROL	5

#define SDAC_STATESIZE 6	/* 6 registers. */
#define GENDAC_STATESIZE 6
#endif

#if defined(INCLUDE_S3_SDAC_DAC_TEST) || defined(INCLUDE_S3_GENDAC_DAC_TEST)
static int GENDAC_SDAC_probe(void)
{
/* Taken from XFree86, accel/s3.c. */
/* Return 1 if GENDAC found, 2 if SDAC, 0 otherwise. */
    /* probe for S3 GENDAC or SDAC */
    /*
     * S3 GENDAC and SDAC have two fixed read only PLL clocks
     *     CLK0 f0: 25.255MHz   M-byte 0x28  N-byte 0x61
     *     CLK0 f1: 28.311MHz   M-byte 0x3d  N-byte 0x62
     * which can be used to detect GENDAC and SDAC since there is no chip-id
     * for the GENDAC.
     *
     * NOTE: for the GENDAC on a MIRO 10SD (805+GENDAC) reading PLL values
     * for CLK0 f0 and f1 always returns 0x7f (but is documented "read only")
     */

    unsigned char saveCR55, savelut[6];
    int i;
    long clock01, clock23;

    saveCR55 = __svgalib_inCR(0x55);
    __svgalib_outbCR(0x55, saveCR55 & ~1);

    port_out_r(0x3c7, 0);
    for (i = 0; i < 2 * 3; i++)	/* save first two LUT entries */
	savelut[i] = port_in(0x3c9);
    port_out_r(0x3c8, 0);
    for (i = 0; i < 2 * 3; i++)	/* set first two LUT entries to zero */
	port_out_r(0x3c9, 0);

    __svgalib_outbCR(0x55, saveCR55 | 1);

    port_out_r(0x3c7, 0);
    for (i = clock01 = 0; i < 4; i++)
	clock01 = (clock01 << 8) | (port_in(0x3c9) & 0xff);
    for (i = clock23 = 0; i < 4; i++)
	clock23 = (clock23 << 8) | (port_in(0x3c9) & 0xff);

    __svgalib_outbCR(0x55, saveCR55 & ~1);

    port_out_r(0x3c8, 0);
    for (i = 0; i < 2 * 3; i++)	/* restore first two LUT entries */
	port_out_r(0x3c9, savelut[i]);

    __svgalib_outbCR(0x55, saveCR55);

    if (clock01 == 0x28613d62 ||
	(clock01 == 0x7f7f7f7f && clock23 != 0x7f7f7f7f)) {

	port_in(0x3c8);		/* dactopel */

	port_in(0x3c6);
	port_in(0x3c6);
	port_in(0x3c6);

	/* the forth read will show the SDAC chip ID and revision */
	if (((i = port_in(0x3c6)) & 0xf0) == 0x70) {
	    return 2;		/* SDAC found. */
	} else {
	    return 1;		/* GENDAC found. */
	}
	port_in(0x3c8);		/* dactopel */
    }
    return 0;
}
#endif

#if defined(INCLUDE_S3_SDAC_DAC) || defined(INCLUDE_S3_GENDAC_DAC)
static void GENDAC_SDAC_init(void)
{
    unsigned char val;
    int m, n, n1, n2, MCLK;
    val = __svgalib_inCR(0x55);
    __svgalib_outbCR(0x55, val | 0x01);

    port_out_r(0x3C7, 10);		/* Read MCLK. */
    m = port_in(0x3C9);
    n = port_in(0x3C9);

    __svgalib_outbCR(0x55, val);		/* Restore CR55. */

    m &= 0x7f;
    n1 = n & 0x1f;
    n2 = (n >> 5) & 0x03;
    /* Calculate MCLK in kHz. */
    MCLK = 14318 * (m + 2) / (n1 + 2) / (1 << n2);
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: S3-GENDAC/SDAC: MCLK = %d.%03d MHz\n",
	       MCLK / 1000, MCLK % 1000);
}
#endif


#if defined(INCLUDE_S3_SDAC_DAC) || defined(INCLUDE_S3_GENDAC_DAC) || defined(INCLUDE_S3_TRIO64_DAC)
/*
 * From XFree86 common_hw/S3gendac.c and S3gendac.h.
 * 
 * Progaming of the S3 gendac programable clocks, from the S3 Gendac
 * programing documentation by S3 Inc. 
 * Jon Tombs <jon@esix2.us.es>
 * 
 * Returns nonzero if success, 0 if failure.
 */
#define BASE_FREQ	     14.31818	/* MHz */

#define DEBUG_FINDCLOCK 0

static int S3dacsFindClock(int freq_in, int min_n2, int freq_min, int freq_max,
		     int *best_m_out, int *best_n1_out, int *best_n2_out)
{
    double ffreq_in, ffreq_min, ffreq_max;
    double ffreq_out, diff, best_diff;
    unsigned int m;
    unsigned char n1, n2;
    unsigned char best_n1 = 16 + 2, best_n2 = 2, best_m = 125 + 2;

#if DEBUG_FINDCLOCK
    fprintf(stderr,"S3dacsFindClock: Trying to match clock of %0.3f MHz\n", freq_in / 1000.0);
#endif
    
    ffreq_in = freq_in / 1000.0 / BASE_FREQ;
    ffreq_min = freq_min / 1000.0 / BASE_FREQ;
    ffreq_max = freq_max / 1000.0 / BASE_FREQ;

    /* Check if getting freq_in is possible at all */
    if (freq_in < freq_min / 8) {
#if DEBUG_FINDCLOCK
	fprintf(stderr,"S3dacsFindClock: %0.3f MHz is too low (lowest is %0.3f MHz)\n",
	       freq_in / 1000.0, freq_min / 1000.0 / 8);
#endif
	return 0;
    }  
    if (freq_in > freq_max / (1 << min_n2)) {
#if DEBUG_FINDCLOCK
	fprintf(stderr,"S3dacsFindClock: %0.3f MHz is too high (highest is %0.3f MHz)\n",
	       freq_in / 1000.0, freq_max / 1000.0 / (1 << min_n2));
#endif
	return 0;
    }

    /* work out suitable timings */
    best_diff = ffreq_in;
    for (n2 = min_n2; n2 <= 3; n2++) {
	for (n1 = 1 + 2; n1 <= 31 + 2; n1++) {
	    m = (int) (ffreq_in * n1 * (1 << n2) + 0.5);
	    if (m < 1 + 2 || m > 127 + 2)
		continue;
	    ffreq_out = (double) (m) / (double) (n1);
	    if ((ffreq_out >= ffreq_min) && (ffreq_out <= ffreq_max)) {
		diff = ffreq_in - ffreq_out / (1 << n2);
		if (diff < 0.0)
		    diff = -diff;
		if (diff < best_diff) {
		    best_diff = diff;
		    best_m = m;
		    best_n1 = n1;
		    best_n2 = n2;
		}
	    }
	}
    }

#if DEBUG_FINDCLOCK
    fprintf(stderr,"S3dacsFindClock: clock wanted %1.6f MHz, found %1.6f MHz (m %d, n1 %d, n2 %d)\n",
	   freq_in / 1000.0,
	   best_m / ((double) best_n1 * (1 << best_n2)) * BASE_FREQ,
	   best_m, best_n1, best_n2);
#endif
    
    *best_m_out = best_m;
    *best_n1_out = best_n1;
    *best_n2_out = best_n2;
    
    return 1;
}
#endif

#if defined(INCLUDE_S3_SDAC_DAC) || defined(INCLUDE_S3_GENDAC_DAC)
static int GENDAC_SDAC_match_programmable_clock(int desiredclock)
{
    int min_m, min_n1, n2;
    
    /* Note: For ICS5342, min_n2 parameter should be one. */
    if (!S3dacsFindClock(desiredclock, 0, 100000, 250000, &min_m, &min_n1, &n2))
	return 0;

    return ((float) (min_m) / (float) (min_n1) / (1 << n2)) * BASE_FREQ * 1000;
}

#if 0				/* Retained for reference. */
static void setdacpll(reg, data1, data2)
int reg;
unsigned char data1;
unsigned char data2;
{
    unsigned char tmp, tmp1;
    int vgaCRIndex = vgaIOBase + 4;
    int vgaCRReg = vgaIOBase + 5;

    /* set RS2 via CR55, yuck */
    tmp = __svgalib_inCR(0x55) & 0xFC;
    __svgalib_outCR(tmp | 0x01);
    tmp1 = port_in(GENDAC_INDEX);

    port_out_r(GENDAC_INDEX, reg);
    port_out_r(GENDAC_DATA, data1);
    port_out_r(GENDAC_DATA, data2);

    /* Now clean up our mess */
    port_out_r(GENDAC_INDEX, tmp1);
    __svgalib_outbCR(0x55, tmp);
}
#endif

static void GENDAC_SDAC_initialize_clock_state(unsigned char *regs, int freq)
{
    int min_m, min_n1, n2;
    int n, m;

    if (!S3dacsFindClock(freq, 0, 100000, 250000, &min_m, &min_n1, &n2)) {
	fprintf(stderr,"Bad dot clock %0.3f MHz.\n", freq / 1000.0);
	return;
    }
    
    n = (min_n1 - 2) | (n2 << 5);
    m = min_m - 2;
    regs[SDAC_PLL_M] = m;
    regs[SDAC_PLL_N1_N2] = n;
    if (__svgalib_driver_report)
	fprintf(stderr,"Initializing DAC PLL values; 0x%02X, 0x%02X.\n", m, n);
}

static void GENDAC_SDAC_savestate(unsigned char *regs)
{
    unsigned char tmp;
    tmp = __svgalib_inCR(0x55);
    __svgalib_outbCR(0x55, tmp | 1);

    regs[SDAC_COMMAND] = port_in(0x3c6);
    regs[SDAC_PLL_WRITEINDEX] = port_in(0x3c8);	/* PLL write index */
    regs[SDAC_PLL_READINDEX] = port_in(0x3c7);	/* PLL read index */
    port_out_r(0x3c7, 2);		/* index to f2 reg */
    regs[SDAC_PLL_M] = port_in(0x3c9);	/* f2 PLL M divider */
    regs[SDAC_PLL_N1_N2] = port_in(0x3c9);	/* f2 PLL N1/N2 divider */
    port_out_r(0x3c7, 0x0e);		/* index to PLL control */
    regs[SDAC_PLL_CONTROL] = port_in(0x3c9);	/* PLL control */

    __svgalib_outbCR(0x55, tmp & ~1);
}

static void GENDAC_SDAC_restorestate(const unsigned char *regs)
{
    unsigned char tmp;

    /* set RS2 via CR55, yuck */
    tmp = __svgalib_inCR(0x55) & 0xFC;
    __svgalib_outbCR(0x55, tmp | 0x01);

#ifdef DEBUG
    do {
	int m, n1, n2, clk;

	m = regs[SDAC_PLL_M] & 0x7f;
	n1 = regs[SDAC_PLL_N1_N2] & 0x1f;
	n2 = (regs[SDAC_PLL_N1_N2] & 0x60) >> 5;

	clk = 14318 * (m + 2) / (n1 + 2) / (1 << n2);
	fprintf(stderr,"SDAC.restorestate, setting clock 0x%02X 0x%02X (%d.%3dMHz)\n",
	       regs[SDAC_PLL_M],
	       regs[SDAC_PLL_N1_N2], clk / 1000, clk % 1000);
    } while (0);
#endif

    port_out_r(0x3c6, regs[SDAC_COMMAND]);
    port_out_r(0x3c8, 2);		/* index to f2 reg */
    port_out_r(0x3c9, regs[SDAC_PLL_M]);	/* f2 PLL M divider */
    port_out_r(0x3c9, regs[SDAC_PLL_N1_N2]);	/* f2 PLL N1/N2 divider */
    port_out_r(0x3c8, 0x0e);		/* index to PLL control */
    port_out_r(0x3c9, regs[SDAC_PLL_CONTROL]);	/* PLL control */
    port_out_r(0x3c8, regs[SDAC_PLL_WRITEINDEX]);	/* PLL write index */
    port_out_r(0x3c7, regs[SDAC_PLL_READINDEX]);	/* PLL read index */

    __svgalib_outbCR(0x55, tmp);
}

#endif				/* defined(INCLUDE_S3_SDAC_DAC) || defined(INCLUDE_S3_GENDAC_DAC) */

/*
 * SDAC: 16-bit DAC, 110 MHz raw clock limit.
 *
 * The 135 MHz version supports pixel multiplexing in 8bpp modes with a
 * halved raw clock. (SL: at least mine doesn't.)
 */

#ifdef INCLUDE_S3_SDAC_DAC_TEST
static int SDAC_probe(void)
{
    return GENDAC_SDAC_probe() == 2;
}
#else
#define SDAC_probe 0
#endif

#ifdef INCLUDE_S3_SDAC_DAC
static int SDAC_map_clock(int bpp, int pixelclock)
{
    switch (bpp) {
    case 4:
    case 8:
#ifdef SDAC_8BPP_PIXMUX		/* SL: AFAIK it doesn't work */
	if (pixelclock >= 67500)
	    /* Use pixel multiplexing. */
	    return pixelclock / 2;
#endif
	break;
    case 24:
	return pixelclock * 3 / 2;
    case 32:
	return pixelclock * 2;
    }
    return pixelclock;
}

static int SDAC_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    switch (bpp) {
    case 16:
	return htiming * 2;
    case 24:
	return htiming * 3;
    case 32:
	return htiming * 4;
    }
    return htiming;
}

static void SDAC_initializestate(unsigned char *regs, int bpp, int colormode,
				 int pixelclock)
{
    int pixmux;			/* SDAC command register. */
    pixmux = 0;
    switch (colormode) {
    case CLUT8_6:
#ifdef SDAC_8BPP_PIXMUX
	if (pixelclock >= 67500)
	    pixmux = 0x10;
#endif
	break;
    case RGB16_555:
	pixmux = 0x30;
	break;
    case RGB16_565:
	pixmux = 0x50;
	break;
    case RGB24_888_B:
	/* Use 0x40 for 3 VCLK/pixel.  Change SDAC_map_clock and CR67 as well. */
	pixmux = 0x90;
	break;
    case RGB32_888_B:
	pixmux = 0x70;
	break;
    }
    regs[SDAC_COMMAND] = pixmux;
    GENDAC_SDAC_initialize_clock_state(regs,
				       SDAC_map_clock(bpp, pixelclock));
}

static void SDAC_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 110000);	/* most can do 135MHz. */
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed;
    cardspecs->maxPixelClock24bpp = dacspeed * 2 / 3;
    cardspecs->maxPixelClock32bpp = dacspeed / 2;
    cardspecs->mapClock = SDAC_map_clock;
    cardspecs->matchProgrammableClock = GENDAC_SDAC_match_programmable_clock;
    cardspecs->mapHorizontalCrtc = SDAC_map_horizontal_crtc;
    cardspecs->flags |= CLOCK_PROGRAMMABLE;
}

DacMethods __svgalib_S3_SDAC_methods =
{
    S3_SDAC,
    "S3-SDAC (86C716)",
    DAC_HAS_PROGRAMMABLE_CLOCKS,
    SDAC_probe,
    GENDAC_SDAC_init,
    SDAC_qualify_cardspecs,
    GENDAC_SDAC_savestate,
    GENDAC_SDAC_restorestate,
    SDAC_initializestate,
    SDAC_STATESIZE
};
#endif


/* S3-GENDAC, 8-bit DAC. */

#ifdef INCLUDE_S3_GENDAC_DAC_TEST
static int GENDAC_probe(void)
{
    return GENDAC_SDAC_probe() == 1;
}
#else
#define GENDAC_probe 0
#endif

#ifdef INCLUDE_S3_GENDAC_DAC
static int GENDAC_map_clock(int bpp, int pixelclock)
{
    if (bpp == 16)
	return pixelclock * 2;
    if (bpp == 24)
	return pixelclock * 3;
    if (bpp == 32)
	return pixelclock * 4;
    return pixelclock;
}

static int GENDAC_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    /* XXXX Not sure. */
    if (bpp == 24)
	return htiming * 3;
    if (bpp == 16)
	return htiming * 2;
    return htiming;
}

static void GENDAC_initializestate(unsigned char *regs, int bpp, int colormode,
				   int pixelclock)
{
    int daccomm;		/* DAC command register. */
    daccomm = 0;
    if (colormode == RGB16_555)
	daccomm = 0x20;
    else if (colormode == RGB16_565)
	daccomm = 0x60;
    else if (colormode == RGB24_888_B)
	daccomm = 0x40;
    regs[GENDAC_COMMAND] = daccomm;
    GENDAC_SDAC_initialize_clock_state(regs,
				       GENDAC_map_clock(bpp, pixelclock));
}

static void GENDAC_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 110000);
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed / 2;
    cardspecs->maxPixelClock24bpp = dacspeed / 3;
    cardspecs->maxPixelClock32bpp = 0;
    cardspecs->mapClock = GENDAC_map_clock;
    cardspecs->matchProgrammableClock = GENDAC_SDAC_match_programmable_clock;
    cardspecs->mapHorizontalCrtc = GENDAC_map_horizontal_crtc;
    cardspecs->flags |= CLOCK_PROGRAMMABLE;
}

DacMethods __svgalib_S3_GENDAC_methods =
{
    S3_GENDAC,
    "S3-GENDAC (86C708)",
    DAC_HAS_PROGRAMMABLE_CLOCKS,
    GENDAC_probe,
    GENDAC_SDAC_init,
    GENDAC_qualify_cardspecs,
    GENDAC_SDAC_savestate,
    GENDAC_SDAC_restorestate,
    GENDAC_initializestate,
    GENDAC_STATESIZE
};
#endif


#ifdef INCLUDE_S3_TRIO64_DAC
/* S3-Trio64, 16-bit integrated DAC. */

#define	TRIO64_SR15		0
#define TRIO64_SR18		1
#define TRIO64_PLL_N1_N2	2
#define TRIO64_PLL_M		3
#define TRIO64_CR67		4
#define TRIO64_SRB		5
#define TRIO64_STATESIZE	6

/* Note: s3.c also defines CR67, but doesn't use it for the Trio64. */

extern int __svgalib_s3_s3Mclk;

static int Trio64_get_mclk(void)
{
    unsigned char sr8;
    int m, n, n1, n2;

    port_out_r(0x3c4, 0x08);
    sr8 = port_in(0x3c5);
    port_out_r(0x3c5, 0x06);

    port_out_r(0x3c4, 0x11);
    m = port_in(0x3c5);
    port_out_r(0x3c4, 0x10);
    n = port_in(0x3c5);

    port_out_r(0x3c4, 0x08);
    port_out_r(0x3c5, sr8);

    m &= 0x7f;
    n1 = n & 0x1f;
    n2 = (n >> 5) & 0x03;
    /* Calculate MCLK in kHz. */
    return ((1431818 * (m + 2)) / (n1 + 2) / (1 << n2) + 50) / 100;
}

#if 0
static void Trio64_set_mclk(int khz)
/* Doesn't work.  Locks computer up.  Why? */
{
    int sr8;
    int min_m, min_n1, n2;
    
    if (!S3dacsFindClock(khz, 0, 40000, 70000, &min_m, &min_n1, &n2)) {
	fprintf(stderr,"Bad MCLK %0.3f MHz.\n", khz / 1000.0);
	return;
    }

    fprintf(stderr,"%0.3f MHz MCLK, m = %d, n = %d, r = %d\n", khz / 1000.0, min_m - 2, min_n1 - 2, n2);
    port_out_r(0x3C4, 0x08);
    sr8 = port_in(0x3C5);
    port_out_r(0x3C5, 0x06);		/* Unlock. */

    port_out_r(0x3c4, 0x15);
    port_out_r(0x3c5, port_in(0x3c5) & ~0x20);

    /* MCLK. */
    __svgalib_outSR(0x10, (min_n1 - 2) | (n2 << 5));
    __svgalib_outSR(0x11, min_m - 2);

    port_out_r(0x3c4, 0x15);
    port_out_r(0x3c5, port_in(0x3c5) | 0x20);
    port_out_r(0x3c5, port_in(0x3c5) & ~0x20);
    
    __svgalib_outSR(0x08, sr8);
}
#endif

static void Trio64_init(void)
{
    int mclk;

    mclk = Trio64_get_mclk();
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: RAMDAC: Trio64: MCLK = %0.3f MHz\n",
	       mclk / 1000.0);
    __svgalib_s3_s3Mclk = mclk;
}

static int Trio64_map_clock(int bpp, int pixelclock)
{
    if (bpp == 8 && pixelclock >= 67500) /* use pixel doubling */
	return pixelclock / 2;
    if (bpp == 24)
	return pixelclock * 3;
    return pixelclock;
}

static int Trio64_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    if (bpp == 16)
	return htiming * 2;
    /* Normal mapping for 8bpp and 32bpp. */
    return htiming;
}

static void Trio64_initialize_clock_state(unsigned char *regs, int freq)
{
    int min_m, min_n1, n2;
    int n, m;
    
    if (!S3dacsFindClock(freq, 0, 130000, 270000, &min_m, &min_n1, &n2)) {
	fprintf(stderr,"Bad dot clock %0.3f MHz.\n", freq / 1000.0);
	return;
    }
    
    n = (min_n1 - 2) | (n2 << 5);
    m = min_m - 2;
    regs[TRIO64_PLL_M] = m;
    regs[TRIO64_PLL_N1_N2] = n;
}

static int Trio64_match_programmable_clock(int desiredclock)
{
    int min_m, min_n1, n2;

    if (!S3dacsFindClock(desiredclock, 0, 130000, 270000, &min_m, &min_n1, &n2))
	return 0;
    
    return ((float) (min_m) / (float) (min_n1) / (1 << n2)) * BASE_FREQ * 1000;
}

static void Trio64_initializestate(unsigned char *regs, int bpp, int colormode,
				   int pixelclock)
{
    int pixmux, reserved_CR67_1;
    
    regs[TRIO64_SR15] &= ~0x50;
    regs[TRIO64_SR18] &= ~0x80;
    pixmux = 0;
    reserved_CR67_1 = 0;
    if (bpp == 8 && pixelclock >= 67500) {
	pixmux = 0x10;
	reserved_CR67_1 = 2;
	regs[TRIO64_SR15] |= 0x50;
	regs[TRIO64_SR18] |= 0x80;
    } else if (bpp == 16) {
	/* moderegs[S3_CR33] |= 0x08; *//* done in s3.c. */
	if (colormode == RGB16_555)
	    pixmux = 0x30;
	else
	    pixmux = 0x50;
	reserved_CR67_1 = 2;
    } else if (colormode == RGB24_888_B) {
	/* remember to adjust SRB as well. */
	pixmux = 0x00;
    } else if (colormode == RGB32_888_B) {
	pixmux = 0xD0;		/* 32-bit color, 2 VCLKs/pixel. */
	reserved_CR67_1 = 2;
    }
    regs[TRIO64_CR67] = pixmux | reserved_CR67_1;

    Trio64_initialize_clock_state(regs, pixelclock);
}

static void Trio64_savestate(unsigned char *regs)
{
    unsigned char sr8;
    port_out_r(0x3C4, 0x08);
    sr8 = port_in(0x3C5);
    port_out_r(0x3C5, 0x06);		/* Unlock. */

    regs[TRIO64_SR15] = __svgalib_inSR(0x15);
    regs[TRIO64_SR18] = __svgalib_inSR(0x18);
    regs[TRIO64_PLL_N1_N2] = __svgalib_inSR(0x12);
    regs[TRIO64_PLL_M] = __svgalib_inSR(0x13);
    regs[TRIO64_CR67] = __svgalib_inCR(0x67);

    __svgalib_outSR(0x08, sr8);
}

static void Trio64_restorestate(const unsigned char *regs)
{
    unsigned char sr8, tmp;

    port_out_r(0x3C4, 0x08);
    sr8 = port_in(0x3C5);
    port_out_r(0x3C5, 0x06);		/* Unlock. */

    __svgalib_outCR(0x67, regs[TRIO64_CR67]);

    __svgalib_outSR(0x15, regs[TRIO64_SR15]);
    __svgalib_outSR(0x18, regs[TRIO64_SR18]);

    /* Clock. */
    __svgalib_outSR(0x12, regs[TRIO64_PLL_N1_N2]);
    __svgalib_outSR(0x13, regs[TRIO64_PLL_M]);

#if 0
    /*
     * XFree86 XF86_S3 (common_hw/gendac.c) has this, but it looks
     * incorrect, it should flip the bit by writing to 0x3c5, not
     * 0x3c4.
     */
    port_out_r(0x3c4, 0x15);
    tmp = port_in(0x3c5);
    port_out_r(0x3c4, tmp & ~0x20);
    port_out_r(0x3c4, tmp | 0x20);
    port_out_r(0x3c4, tmp & ~0x20);
#else
    port_out_r(0x3c4, 0x15);
    tmp = port_in(0x3c5);
    port_out_r(0x3c5, tmp & ~0x20);
    port_out_r(0x3c5, tmp | 0x20);
    port_out_r(0x3c5, tmp & ~0x20);
#endif
    
    __svgalib_outSR(0x08, sr8);
}


static void Trio64_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    if (dacspeed) {
	if (__svgalib_driver_report)
	    fprintf(stderr,"svgalib: using 'dacspeed' not recommended for this RAMDAC.\n");
	cardspecs->maxPixelClock4bpp = dacspeed;
	cardspecs->maxPixelClock8bpp = 135000;
	cardspecs->maxPixelClock16bpp = dacspeed;
	cardspecs->maxPixelClock24bpp = 0; /* dacspeed / 3; *//* How to program? */
	cardspecs->maxPixelClock32bpp = 50000;
    } else {
	cardspecs->maxPixelClock4bpp = 80000;
	cardspecs->maxPixelClock8bpp = 135000;
	cardspecs->maxPixelClock16bpp = 80000;
	cardspecs->maxPixelClock24bpp = 0; /* 25000; *//* How to program? */
	cardspecs->maxPixelClock32bpp = 50000;
    }
    cardspecs->mapClock = Trio64_map_clock;
    cardspecs->matchProgrammableClock = Trio64_match_programmable_clock;
    cardspecs->mapHorizontalCrtc = Trio64_map_horizontal_crtc;
    cardspecs->flags |= CLOCK_PROGRAMMABLE;
}

DacMethods __svgalib_Trio64_methods =
{
    TRIO64,
    "S3-Trio64 internal DAC",
    DAC_HAS_PROGRAMMABLE_CLOCKS,
    NULL,			/* probe */
    Trio64_init,
    Trio64_qualify_cardspecs,
    Trio64_savestate,
    Trio64_restorestate,
    Trio64_initializestate,
    TRIO64_STATESIZE
};
#endif
