/*
 * icd2061a.c
 *
 * support for the ICD 2061A programmable clockchip and compatibles 
 * outside of the DAC
 *
 * Rev history:
 * Andreas Arens Dec 95: Created
 *
 * Andreas Arens Feb 15 1996: A few minor fixes
 */

#include "timing.h"
#include "libvga.h"
#include "ramdac/ramdac.h"
#include "clockchi.h"
#include "svgadriv.h"
#include "vga.h"

/* 
 * ATTENTION: The ICD 2061A does not support reading of the currently selected
 * pixelclock. XFree86 also fails to restore this value correctly, but always
 * estores a 45 MHz pixelclock. My standard text mode (132x25) uses 40 MHz,
 * which is the value selected here.
 * You can use the SVGATextMode-1.0 'clockprobe' tool right after boot to
 * determine the value used with your card and modify here, but since 40 MHz
 * is the VESA suggested pixelclock for a 70 Hz 132x25 mode, the value here
 * seems fine. Note that 80xXX modes use 25 or 28 MHz clocks, which are fixed
 * and not affected by this. This might not be true for Diamond boards using
 * the DCS2824-0 clockchip, which is an ICD 2061A clone.
 */
#define I2061A_DEFAULT_TEXT_FREQUENCY	(40000L)	/* kHz */

/*
 * Clockchip code is derived from I2051Aalt.c in XFree86/common_hw which
 * in turn is derived from code available from the STB bulletin board.
 * A number of modifications have been made to fit this into SVGAlib.
 */

#define I2061A_CRYSTAL_FREQUENCY       (14.31818 * 2.0)

static double I2061A_range[15] =
{50.0, 51.0, 53.2, 58.5, 60.7, 64.4, 66.8, 73.5,
 75.6, 80.9, 83.2, 91.5, 100.0, 120.0, 120.0000001};

static long I2061A_SelectClock(long frequency)
				/* in KHz */
{
    unsigned int m;
    int i;
    double freq, fvco;
    double dev, devx;
    double delta, deltax;
    double f0;
    unsigned int p, q;
    unsigned int bestp = 0, bestq = 0, bestm = 0, besti = 0;

    freq = ((double) frequency) / 1000.0;
    if (freq > I2061A_range[13])
	freq = I2061A_range[13];
    else if (freq < 7.0)
	freq = 7.0;

    /*
     *  Calculate values to load into ICD 2061A clock chip to set frequency
     */
    delta = 999.0;
    dev = 999.0;

    for (m = 0; m < 8; m++) {
	fvco = freq * (1 << m);
	if (fvco < 50.0 || fvco > 120.0)
	    continue;

	f0 = fvco / I2061A_CRYSTAL_FREQUENCY;

	for (q = 14; q <= 71; q++) {	/* q={15..71}:Constraint 2 on page 14 */
	    p = (int) (f0 * q + 0.5);
	    if (p < 4 || p > 130)	/* p={4..130}:Constraint 5 on page 14 */
		continue;
	    deltax = (double) (p) / (double) (q) - f0;
	    if (deltax < 0)
		deltax = -deltax;
	    if (deltax <= delta) {
		for (i = 13; i >= 0; i--)
		    if (fvco >= I2061A_range[i])
			break;
		devx = (fvco - (I2061A_range[i] + I2061A_range[i + 1]) / 2) / fvco;
		if (devx < 0)
		    devx = -devx;
		if (deltax < delta || devx < dev) {
		    delta = deltax;
		    dev = devx;
		    bestp = p;
		    bestq = q;
		    bestm = m;
		    besti = i;
		}
	    }
	}
    }
    return ((((((long) besti << 7) | (bestp - 3)) << 3) | bestm) << 7) | (bestq - 2);
}

static int I2061A_GetClock(long dwv)
{
    int clock_q = (dwv & 0x7f) + 2;
    int clock_m = (dwv >> 7) & 7;
    int clock_p = ((dwv >> 10) & 0x7f) + 3;
    double fvco;

    fvco = I2061A_CRYSTAL_FREQUENCY / (1 << clock_m);
    return (int) (((fvco * clock_p) / clock_q) * 1000);
}

/* needs some delay for really fast cpus */
#define wrt_clk_bit(v) port_out_r(MIS_W, v), (void)port_in(crtcaddr), (void)port_in(crtcaddr)

/* ATTENTION: This assumes CRTC registers and S3 registers to be UNLOCKED! */
static void I2061A_init_clock(unsigned long setup)
{
    unsigned char nclk[2], clk[2];
    unsigned short restore42;
    unsigned short oldclk;
    unsigned short bitval;
    int i;
    unsigned char c;
    unsigned short crtcaddr = (port_in(MIS_R) & 0x01) ? CRT_IC : CRT_IM;

    oldclk = port_in(MIS_R);

    port_out_r(crtcaddr, 0x42);
    restore42 = port_in(crtcaddr + 1);

    port_outw_r(SEQ_I, 0x0100);

    port_out_r(SEQ_I, 1);
    c = port_in(SEQ_D);
    port_out_r(SEQ_D, 0x20 | c);

    port_out_r(crtcaddr, 0x42);
    port_out_r(crtcaddr + 1, 0x03);

    port_outw_r(SEQ_I, 0x0300);

    nclk[0] = oldclk & 0xF3;
    nclk[1] = nclk[0] | 0x08;
    clk[0] = nclk[0] | 0x04;
    clk[1] = nclk[0] | 0x0C;

    port_out_r(crtcaddr, 0x42);
    (void) port_in(crtcaddr + 1);

    port_outw_r(SEQ_I, 0x0100);

    wrt_clk_bit(oldclk | 0x08);
    wrt_clk_bit(oldclk | 0x0C);
    for (i = 0; i < 5; i++) {
	wrt_clk_bit(nclk[1]);
	wrt_clk_bit(clk[1]);
    }
    wrt_clk_bit(nclk[1]);
    wrt_clk_bit(nclk[0]);
    wrt_clk_bit(clk[0]);
    wrt_clk_bit(nclk[0]);
    wrt_clk_bit(clk[0]);
    for (i = 0; i < 24; i++) {
	bitval = setup & 0x01;
	setup >>= 1;
	wrt_clk_bit(clk[1 - bitval]);
	wrt_clk_bit(nclk[1 - bitval]);
	wrt_clk_bit(nclk[bitval]);
	wrt_clk_bit(clk[bitval]);
    }
    wrt_clk_bit(clk[1]);
    wrt_clk_bit(nclk[1]);
    wrt_clk_bit(clk[1]);

    port_out_r(SEQ_I, 1);
    c = port_in(SEQ_D);
    port_out_r(SEQ_D, 0xDF & c);

    port_out_r(crtcaddr, 0x42);
    port_out_r(crtcaddr + 1, restore42);

    port_out_r(MIS_W, oldclk);

    port_outw_r(SEQ_I, 0x0300);

    vga_waitretrace();
    vga_waitretrace();
    vga_waitretrace();
    vga_waitretrace();
    vga_waitretrace();
    vga_waitretrace();
    vga_waitretrace();		/* 0.10 second delay... */
}

static int I2061A_match_programmable_clock(int desiredclock)
{
    long dvw;

    dvw = I2061A_SelectClock((long) desiredclock);
    if (dvw)
	return I2061A_GetClock(dvw);
    return 0;
}

static void I2061A_saveState(unsigned char *regs)
{
    long *dvwp;

    if (__svgalib_I2061A_clockchip_methods.DAC_saveState)
	__svgalib_I2061A_clockchip_methods.DAC_saveState(regs);

    dvwp = (long *) (regs + __svgalib_I2061A_clockchip_methods.DAC_stateSize);
    *dvwp = I2061A_SelectClock(__svgalib_I2061A_clockchip_methods.TextFrequency);
}

static void I2061A_restoreState(const unsigned char *regs)
{
    unsigned int clknum = 2;
    long *dvwp;

    if (__svgalib_I2061A_clockchip_methods.DAC_restoreState)
	__svgalib_I2061A_clockchip_methods.DAC_restoreState(regs);
    dvwp = (long *) (regs + __svgalib_I2061A_clockchip_methods.DAC_stateSize);
    if (*dvwp) {
	/*
	 * Write ICD 2061A clock chip - assumes S3 to be unlocked!
	 */
	I2061A_init_clock(((unsigned long) *dvwp) | (((long) clknum) << 21));
    }
}

static void I2061A_initializeState(unsigned char *regs, int bpp, int colormode, int pixelclock)
{
    long *dvwp;

    if (__svgalib_I2061A_clockchip_methods.DAC_initializeState)
	__svgalib_I2061A_clockchip_methods.DAC_initializeState(regs, bpp, colormode, pixelclock);

    dvwp = (long *) (regs + __svgalib_I2061A_clockchip_methods.DAC_stateSize);

    if (bpp > 16)
	pixelclock *= 4;
    else if (bpp > 8)
	pixelclock *= 2;

    *dvwp = I2061A_SelectClock((long) pixelclock);
}

/* This functions patches the DacMethod to route through the ClockChip Method */
static void I2061A_init(CardSpecs * cardspecs, DacMethods * DAC)
{
    if (DAC && !__svgalib_I2061A_clockchip_methods.DAC_initializeState) {
	if (__svgalib_driver_report)
	    fprintf(stderr,"svgalib: Using ICD2061A or compatible clockchip.\n");
	__svgalib_I2061A_clockchip_methods.DAC_initializeState = DAC->initializeState;
	__svgalib_I2061A_clockchip_methods.DAC_saveState = DAC->saveState;
	__svgalib_I2061A_clockchip_methods.DAC_restoreState = DAC->restoreState;
	__svgalib_I2061A_clockchip_methods.DAC_stateSize = DAC->stateSize;
	DAC->initializeState = I2061A_initializeState;
	DAC->saveState = I2061A_saveState;
	DAC->restoreState = I2061A_restoreState;
	DAC->stateSize += sizeof(long);
	cardspecs->matchProgrammableClock = I2061A_match_programmable_clock;
	cardspecs->flags |= CLOCK_PROGRAMMABLE;
    }
}

ClockChipMethods __svgalib_I2061A_clockchip_methods =
{
    I2061A_init,
    I2061A_saveState,
    I2061A_restoreState,
    I2061A_initializeState,
    NULL,			/* DAC function save area */
    NULL,
    NULL,
    I2061A_DEFAULT_TEXT_FREQUENCY,
    0,
};
