/*
 * ics_gendac.c:
 * 
 * This works only with ARK, since it has ARK specific code.
 */

#include <stdio.h>
#include "libvga.h"
#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

/* SDAC/GENDAC registers */
#define SDAC_COMMAND		0
#define GENDAC_COMMAND		0
#define SDAC_PLL_WRITEINDEX	1
#define SDAC_PLL_READINDEX	2
#define SDAC_PLL_M		3	/* f2 programmed clock */
#define SDAC_PLL_N1_N2		4
#define SDAC_PLL_CONTROL	5

#define GENDAC_STATESIZE 6

static void GENDAC_init(void)
{
}


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

static int GENDAC_match_programmable_clock(int desiredclock)
{
    int min_m, min_n1, n2;
    
    /* Note: For ICS5342, min_n2 parameter should be one. */
    if (!S3dacsFindClock(desiredclock, 0, 100000, 250000, &min_m, &min_n1, &n2))
	return 0;

    return ((float) (min_m) / (float) (min_n1) / (1 << n2)) * BASE_FREQ * 1000;
}

static void GENDAC_initialize_clock_state(unsigned char *regs, int freq)
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
#if 0
    if (__svgalib_driver_report)
	fprintf(stderr,"Initializing DAC PLL values; 0x%02X, 0x%02X.\n", m, n);
#endif
}

static void GENDAC_savestate(unsigned char *regs)
{
    unsigned char tmp;
    tmp = __svgalib_inSR(0x1c);
    __svgalib_outSR(0x1c, tmp | 0x80);

    regs[SDAC_COMMAND] = port_in(0x3c6);
    regs[SDAC_PLL_WRITEINDEX] = port_in(0x3c8);	/* PLL write index */
    regs[SDAC_PLL_READINDEX] = port_in(0x3c7);	/* PLL read index */
    port_out_r(0x3c7, 2);		/* index to f2 reg */
    regs[SDAC_PLL_M] = port_in(0x3c9);	/* f2 PLL M divider */
    regs[SDAC_PLL_N1_N2] = port_in(0x3c9);	/* f2 PLL N1/N2 divider */
    port_out_r(0x3c7, 0x0e);		/* index to PLL control */
    regs[SDAC_PLL_CONTROL] = port_in(0x3c9);	/* PLL control */

    __svgalib_outSR(0x1c, tmp & ~0x80);
}

static void GENDAC_restorestate(const unsigned char *regs)
{
    unsigned char tmp;

    tmp = __svgalib_inseq(0x1c);
    __svgalib_outseq(0x1c, tmp | 0x80);

    port_out_r(0x3c6, regs[SDAC_COMMAND]);
    port_out_r(0x3c8, 2);		/* index to f2 reg */
    port_out_r(0x3c9, regs[SDAC_PLL_M]);	/* f2 PLL M divider */
    port_out_r(0x3c9, regs[SDAC_PLL_N1_N2]);	/* f2 PLL N1/N2 divider */
    port_out_r(0x3c8, 0x0e);		/* index to PLL control */
    port_out_r(0x3c9, regs[SDAC_PLL_CONTROL]);	/* PLL control */
    port_out_r(0x3c8, regs[SDAC_PLL_WRITEINDEX]);	/* PLL write index */
    port_out_r(0x3c7, regs[SDAC_PLL_READINDEX]);	/* PLL read index */

    __svgalib_outseq(0x1c, tmp);
}

/*
 * SDAC: 16-bit DAC, 110 MHz raw clock limit.
 *
 * The 135 MHz version supports pixel multiplexing in 8bpp modes with a
 * halved raw clock. (SL: at least mine doesn't.)
 */

static int GENDAC_probe(void)
{
    int i;
    port_in(0x3c6);
    port_in(0x3c6);
    port_in(0x3c6);
    i=port_in(0x3c6);
    if(i==177) return 1;
    return 0;
}

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
    GENDAC_initialize_clock_state(regs,
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
    cardspecs->matchProgrammableClock = GENDAC_match_programmable_clock;
    cardspecs->mapHorizontalCrtc = GENDAC_map_horizontal_crtc;
    cardspecs->flags |= CLOCK_PROGRAMMABLE;
}

DacMethods __svgalib_ICS_GENDAC_methods =
{
    S3_GENDAC,
    "ICS-GENDAC (5342)",
    DAC_HAS_PROGRAMMABLE_CLOCKS,
    GENDAC_probe,
    GENDAC_init,
    GENDAC_qualify_cardspecs,
    GENDAC_savestate,
    GENDAC_restorestate,
    GENDAC_initializestate,
    GENDAC_STATESIZE
};
