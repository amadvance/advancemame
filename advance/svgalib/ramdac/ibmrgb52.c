/*
 * IBMRGB52x.c:
 * 
 * RAMDAC definitions for IBM's RGB52x PaletteDAC.
 *
 * Portion of this file is derived from XFree86's source code.
 * [insert XFree86's copyright here].
 */

#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

#include "ibmrgb52.h"

#define IBMRGB52x_STATESIZE	0x101

static int IBMRGB52x_dacspeed = 170000;		/* assuming 170MHz DAC */
static int IBMRGB52x_fref = 16000;	/* assuming 16MHz refclock */
static int IBMRGB52x_clk = 2;	/* use clock 2 */

#ifdef INCLUDE_IBMRGB52x_DAC_TEST
/* 
 * s3IBMRGB_Probe() from XFree86.
 * 
 * returns 0x01xx for 525, 0x02xx for 524/528, where xx = revision.
 */
static int IBMRGB52x_probe(void)
{
    unsigned char CR43, CR55, dac[3], lut[6];
    unsigned char ilow, ihigh, id, rev, id2, rev2;
    int i, j;
    int ret = 0;

    port_out(0x43, 0x3D4);
    CR43 = port_in(0x3D5);
    port_out(CR43 & ~0x02, 0x3D5);

    port_out(0x55, 0x3D4);
    CR55 = port_in(0x3D5);
    port_out(CR55 & ~0x03, 0x3D5);

    /* save DAC and first LUT entries */
    for (i = 0; i < 3; i++)
	dac[i] = port_in(IBMRGB_PIXEL_MASK + i);
    for (i = j = 0; i < 2; i++) {
	port_out(i, IBMRGB_READ_ADDR);
	lut[j++] = port_in(IBMRGB_RAMDAC_DATA);
	lut[j++] = port_in(IBMRGB_RAMDAC_DATA);
	lut[j++] = port_in(IBMRGB_RAMDAC_DATA);
    }

    port_out(0x55, 0x3D4);
    port_out((CR55 & ~0x03) | 0x01, 0x3D5);	/* set RS2 */

    /* read ID and revision */
    ilow = port_in(IBMRGB_INDEX_LOW);
    ihigh = port_in(IBMRGB_INDEX_HIGH);
    port_out(0, IBMRGB_INDEX_HIGH);	/* index high */
    port_out(IBMRGB_rev, IBMRGB_INDEX_LOW);
    rev = port_in(IBMRGB_INDEX_DATA);
    port_out(IBMRGB_id, IBMRGB_INDEX_LOW);
    id = port_in(IBMRGB_INDEX_DATA);

    /* known IDs:  
       1 = RGB525
       2 = RGB524, RGB528 
     */

    if (id >= 1 && id <= 2) {
	/* check if ID and revision are read only */
	port_out(IBMRGB_rev, IBMRGB_INDEX_LOW);
	port_out(~rev, IBMRGB_INDEX_DATA);
	port_out(IBMRGB_id, IBMRGB_INDEX_LOW);
	port_out(~id, IBMRGB_INDEX_DATA);
	port_out(IBMRGB_rev, IBMRGB_INDEX_LOW);
	rev2 = port_in(IBMRGB_INDEX_DATA);
	port_out(IBMRGB_id, IBMRGB_INDEX_LOW);
	id2 = port_in(IBMRGB_INDEX_DATA);

	if (id == id2 && rev == rev2) {		/* IBM RGB52x found */
	    ret = (id << 8) | rev;
	} else {
	    port_out(IBMRGB_rev, IBMRGB_INDEX_LOW);
	    port_out(rev, IBMRGB_INDEX_DATA);
	    port_out(IBMRGB_id, IBMRGB_INDEX_LOW);
	    port_out(id, IBMRGB_INDEX_DATA);
	}
    }
    port_out(ilow, IBMRGB_INDEX_LOW);
    port_out(ihigh, IBMRGB_INDEX_HIGH);

    port_out(0x55, 0x3D4);
    port_out(CR55 & ~0x03, 0x3D5);	/* reset RS2 */

    /* restore DAC and first LUT entries */
    for (i = j = 0; i < 2; i++) {
	port_out(i, IBMRGB_WRITE_ADDR);
	port_out(lut[j++], IBMRGB_RAMDAC_DATA);
	port_out(lut[j++], IBMRGB_RAMDAC_DATA);
	port_out(lut[j++], IBMRGB_RAMDAC_DATA);
    }
    for (i = 0; i < 3; i++)
	port_out(dac[i], IBMRGB_PIXEL_MASK + i);

    port_out(0x43, 0x3D4);
    port_out(CR43, 0x3D5);
    port_out(0x55, 0x3D4);
    port_out(CR55, 0x3D5);

    return ret;
}
#else
#define IBMRGB52x_probe 0
#endif

#ifdef INCLUDE_IBMRGB52x_DAC
static void IBMRGBSetClock(long freq, int clk, long dacspeed, long fref,
		      int *best_m_out, int *best_n_out, int *best_df_out)
{
    volatile double ffreq, fdacspeed, ffref;
    volatile int df, n, m, max_n, min_df;
    volatile int best_m = 69, best_n = 17, best_df = 0;
    volatile double diff, mindiff;

#define FREQ_MIN   16250	/* 1000 * (0+65) / 4 */
#define FREQ_MAX  dacspeed

    if (freq < FREQ_MIN)
	ffreq = FREQ_MIN / 1000.0;
    else if (freq > FREQ_MAX)
	ffreq = FREQ_MAX / 1000.0;
    else
	ffreq = freq / 1000.0;

    fdacspeed = dacspeed / 1e3;
    ffref = fref / 1e3;

    ffreq /= ffref;
    ffreq *= 16;
    mindiff = ffreq;

    if (freq <= dacspeed / 4)
	min_df = 0;
    else if (freq <= dacspeed / 2)
	min_df = 1;
    else
	min_df = 2;

    for (df = 0; df < 4; df++) {
	ffreq /= 2;
	mindiff /= 2;
	if (df < min_df)
	    continue;

	/* the remaining formula is  ffreq = (m+65) / n */

	if (df < 3)
	    max_n = fref / 1000 / 2;
	else
	    max_n = fref / 1000;
	if (max_n > 31)
	    max_n = 31;

	for (n = 2; n <= max_n; n++) {
	    m = (int) (ffreq * n + 0.5) - 65;
	    if (m < 0)
		m = 0;
	    else if (m > 63)
		m = 63;

	    diff = (m + 65.0) / n - ffreq;
	    if (diff < 0)
		diff = -diff;

	    if (diff < mindiff) {
		mindiff = diff;
		best_n = n;
		best_m = m;
		best_df = df;
	    }
	}
    }

#ifdef DEBUG
    fprintf(stderr,"clk %d, setting to %f, m 0x%02x %d, n 0x%02x %d, df %d\n", clk,
	   ((best_m + 65.0) / best_n) / (8 >> best_df) * ffref,
	   best_m, best_m, best_n, best_n, best_df);
#endif
    *best_m_out = best_m;
    *best_n_out = best_n;
    *best_df_out = best_df;
}

static void IBMRGB52x_init(void)
{
    unsigned char tmp, CR55;
#ifdef INCLUDE_IBMRGB52x_DAC_TEST
    int idrev;

    idrev = IBMRGB52x_probe();
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using IBM RGB 52%d PaletteDAC, revision %d.\n",
	       (idrev >> 8) == 1 ? 5 : 4,
	       idrev & 0xff);
#else
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using IBM RGB 52x PaletteDAC.\n");
#endif
    /* set RS2 */
    port_out(0x55, 0x3D4);
    CR55 = port_in(0x3D5) & 0xFC;
    port_out(CR55 | 0x01, 0x3D5);

    tmp = port_in(IBMRGB_INDEX_CONTROL);
    port_out(tmp & ~0x01, IBMRGB_INDEX_CONTROL);	/* turn off auto-increment */
    port_out(0, IBMRGB_INDEX_HIGH);	/* reset index high */

    __svgalib_outCR(0x55, CR55);
}

static int IBMRGB52x_match_programmable_clock(int desiredclock)
{
    int m, n, df;

    IBMRGBSetClock(desiredclock, IBMRGB52x_clk, IBMRGB52x_dacspeed,
		   IBMRGB52x_fref, &m, &n, &df);

    return ((m + 65.0) / n) / (8 >> df) * IBMRGB52x_fref;
}

static void IBMRGB52x_initialize_clock_state(unsigned char *regs, int freq)
{
    int m, n, df;

    IBMRGBSetClock(freq, IBMRGB52x_clk, IBMRGB52x_dacspeed,
		   IBMRGB52x_fref, &m, &n, &df);

    if (__svgalib_driver_report)
	fprintf(stderr,"clk %d, setting to %.3f, m 0x%02x %d, n 0x%02x %d, df %d\n",
	       IBMRGB52x_clk, ((m + 65.0) / n) / (8 >> df) * IBMRGB52x_fref / 1000,
	       m, m, n, n, df);

    regs[IBMRGB_misc_clock] |= 0x01;
    regs[IBMRGB_m0 + 2 * IBMRGB52x_clk] = (df << 6) | (m & 0x3f);
    regs[IBMRGB_n0 + 2 * IBMRGB52x_clk] = n;
    regs[IBMRGB_pll_ctrl2] &= 0xf0;
    regs[IBMRGB_pll_ctrl2] |= IBMRGB52x_clk;
    regs[IBMRGB_pll_ctrl1] &= 0xf8;
    regs[IBMRGB_pll_ctrl1] |= 0x03;
}

static void IBMRGB52x_savestate(unsigned char *regs)
{
    int i;
    unsigned char tmp;

    /* set RS2 */
    port_out(0x55, 0x3D4);
    tmp = port_in(0x3D5) & 0xFC;
    port_out(tmp | 0x01, 0x3D5);

    for (i = 0; i < 0x100; i++) {
	port_out(i, IBMRGB_INDEX_LOW);	/* high index is set to 0 */
	regs[i] = port_in(IBMRGB_INDEX_DATA);
    }
    regs[0x100] = __svgalib_inCR(0x22);

    __svgalib_outCR(0x55, tmp);
}

/* SL: not complete, need more work for 525. */
static void IBMRGB52x_restorestate(const unsigned char *regs)
{
    int i;
    unsigned char tmp;

    /* set RS2 */
    port_out(0x55, 0x3D4);
    tmp = port_in(0x3D5) & 0xFC;
    port_out(tmp | 0x01, 0x3D5);

    for (i = 0; i < 0x100; i++) {
	port_out(i, IBMRGB_INDEX_LOW);	/* high index is set to 0 */
	port_out(regs[i], IBMRGB_INDEX_DATA);
    }
    __svgalib_outCR(0x22, regs[0x100]);

    __svgalib_outCR(0x55, tmp);
}

static int IBMRGB52x_map_clock(int bpp, int pixelclock)
{
    return pixelclock;
}

static int IBMRGB52x_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
#ifdef PIXEL_MULTIPLEXING
    switch (bpp) {
    case 4:
	break;
    case 8:
	return htiming / 2;
    case 16:
	break;
    case 24:
	return htiming * 3 / 2;
    case 32:
	return htiming * 2;
    }
#endif
    return htiming;
}

static void IBMRGB52x_initializestate(unsigned char *regs, int bpp, int colormode,
				      int pixelclock)
{
    unsigned char tmp;

    regs[IBMRGB_misc_clock] = (regs[IBMRGB_misc_clock] & 0xf0) | 0x03;
    regs[IBMRGB_sync] = 0;
    regs[IBMRGB_hsync_pos] = 0;
    regs[IBMRGB_pwr_mgmt] = 0;
    regs[IBMRGB_dac_op] &= ~0x08;	/* no sync on green */
    regs[IBMRGB_dac_op] |= 0x02;	/* fast slew */
    regs[IBMRGB_pal_ctrl] = 0;
    regs[IBMRGB_misc1] &= ~0x43;
    regs[IBMRGB_misc1] |= 1;
#ifdef PIXEL_MULTIPLEXING
    if (bpp >= 8)
	regs[IBMRGB_misc2] = 0x43;	/* use SID bus? 0x47 for DAC_8_BIT */
#endif
    tmp = __svgalib_inCR(0x22);
    if (bpp <= 8)		/* and 968 */
	__svgalib_outCR(0x22, tmp | 0x08);
    else
	__svgalib_outCR(0x22, tmp & ~0x08);

    regs[IBMRGB_pix_fmt] &= ~0x07;
    switch (bpp) {
    case 4:
    case 8:
	regs[IBMRGB_pix_fmt] |= 0x03;
	regs[IBMRGB_8bpp] = 0x00;
	break;
    case 15:
	regs[IBMRGB_pix_fmt] |= 0x04;
	regs[IBMRGB_16bpp] = 0x02;
	break;
    case 16:
	regs[IBMRGB_pix_fmt] |= 0x04;
	regs[IBMRGB_16bpp] = 0x00;
	break;
    case 24:
	regs[IBMRGB_pix_fmt] |= 0x05;	/* SL: guess */
	regs[IBMRGB_24bpp] = 0x00;
	break;
    case 32:
	regs[IBMRGB_pix_fmt] |= 0x06;
	regs[IBMRGB_32bpp] = 0x00;
	break;
    }
    IBMRGB52x_initialize_clock_state(regs,
				   IBMRGB52x_map_clock(bpp, pixelclock));
}

static void IBMRGB52x_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    IBMRGB52x_dacspeed = __svgalib_setDacSpeed(dacspeed, 170000);	/* 220 MHz version exist also */
    cardspecs->maxPixelClock4bpp = IBMRGB52x_dacspeed;
    cardspecs->maxPixelClock8bpp = IBMRGB52x_dacspeed;
#ifdef PIXEL_MULTIPLEXING
    cardspecs->maxPixelClock16bpp = IBMRGB52x_dacspeed;
    cardspecs->maxPixelClock24bpp = IBMRGB52x_dacspeed * 3 / 2;
    cardspecs->maxPixelClock32bpp = IBMRGB52x_dacspeed / 2;
#else
    cardspecs->maxPixelClock16bpp = 0;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 0;
#endif
    cardspecs->mapClock = IBMRGB52x_map_clock;
    cardspecs->matchProgrammableClock = IBMRGB52x_match_programmable_clock;
    cardspecs->mapHorizontalCrtc = IBMRGB52x_map_horizontal_crtc;
    cardspecs->flags |= CLOCK_PROGRAMMABLE;
}

DacMethods __svgalib_IBMRGB52x_methods =
{
    IBMRGB52x,
    "IBM RGB 52x PaletteDAC",
    DAC_HAS_PROGRAMMABLE_CLOCKS,
    IBMRGB52x_probe,
    IBMRGB52x_init,
    IBMRGB52x_qualify_cardspecs,
    IBMRGB52x_savestate,
    IBMRGB52x_restorestate,
    IBMRGB52x_initializestate,
    IBMRGB52x_STATESIZE
};
#endif
