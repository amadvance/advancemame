/*
 * attdacs.c:
 * 
 * RAMDAC definition for industry-standard AT&T20C490/498 DACs and 
 * compatibles.
 */

#include <stdlib.h>
#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

/*
 * RAMDAC definition for industry-standard AT&T20C490 DAC with 8-bit pixel
 * port, and compatibles.
 * These RAMDACs can do 32K and 64K color mode (16bpp) with doubled VCLK
 * and 16M (8-8-8) truecolor with tripled VCLK.
 * 0xA0 is written to the Hidden DAC register for 32K, 0xC0 for 64K and
 * 0xE0 for 16M.
 */

#ifdef INCLUDE_ATT20C490_DAC_TEST
static int att20c490_probe(void)
{
    unsigned char oldcomm, notcomm, oldpel, v;
    int flag = 0;

    _ramdac_dactocomm();
    oldcomm = port_in(PEL_MSK);
    _ramdac_dactopel();
    oldpel = port_in(PEL_MSK);

    notcomm = ~oldcomm;
    port_out_r(PEL_MSK, notcomm);
    _ramdac_dactocomm();
    v = port_in(PEL_MSK);
    if (v != notcomm) {
	if ((_ramdac_setcomm(0xe0) & 0xe0) == 0xe0) {
	    if ((_ramdac_setcomm(0x60) & 0xe0) == 0) {
		if ((_ramdac_setcomm(2) & 2) > 0)
		    flag = 1;	/* 20c490 */
		else
		    flag = 1;	/* 20c493 */
	    } else {
		_ramdac_setcomm(oldcomm);
		if (port_in(PEL_MSK) == notcomm)
		    if (_ramdac_setcomm(0xFF) == 0xFF)
			flag = 1;	/* 20c491/20c492 */
	    }
	}
    }
    _ramdac_dactocomm();
    port_out_r(PEL_MSK, oldcomm);
    _ramdac_dactopel();
    port_out_r(PEL_MSK, oldpel);
    return flag;
}
#else
#define att20c490_probe 0
#endif

#ifdef INCLUDE_ATT20C490_DAC
static void att20c490_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using AT&T20C490-compatible truecolor DAC.\n");
#if 0
    dactocomm();
    port_in(PEL_MSK);			/* Skip command register. */
    fprintf(stderr,"svgalib: DAC Manufacturer ID = 0x%02X, ", port_in(PEL_MSK));
    fprintf(stderr,"Device ID = 0x%02X.\n", port_in(PEL_MSK));
#endif
}

static int __svgalib_att20c490_map_clock(int bpp, int pixelclock)
{
    if (bpp == 16)
	return pixelclock * 2;
    if (bpp == 24)
	return pixelclock * 3;
    return pixelclock;
}

static int __svgalib_att20c490_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    if (bpp == 16)
	return htiming * 2;
    if (bpp == 24)
	return htiming * 3;
    return htiming;
}


static void att20c490_initializestate(unsigned char *regs, int bpp, int colormode,
				      int pixelclock)
{
    regs[0] = 0;
    if (colormode == RGB16_555)
	regs[0] = 0xA0;
    if (colormode == RGB16_565)
	regs[0] = 0xC0;
    if (colormode == RGB24_888_B)
	regs[0] = 0xE0;
}

static void att20c490_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 80000);
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed / 2;
    cardspecs->maxPixelClock24bpp = dacspeed / 3;
    cardspecs->maxPixelClock32bpp = 0;
    cardspecs->mapClock = __svgalib_att20c490_map_clock;
    cardspecs->mapHorizontalCrtc = __svgalib_att20c490_map_horizontal_crtc;
}

DacMethods __svgalib_ATT20C490_methods =
{
    ATT20C490,
    "AT&T-compatible truecolor DAC, 80 MHz rated",
    0,
    att20c490_probe,
    att20c490_init,
    att20c490_qualify_cardspecs,
    __svgalib_Sierra_32K_savestate,
    __svgalib_Sierra_32K_restorestate,
    att20c490_initializestate,
    1				/* State size. */
};
#endif

/*
 * RAMDAC definition for industry-standard AT&T20C498 DAC with 16-bit
 * pixel port, and compatibles.
 * Differently rated versions exist, such as 80, 110, 135 and 170 MHz.
 * This code assumes the DAC is actually connected with a 16-bit path.
 * (an example of a 498-compatible DAC being used with a 8-bit path
 * is the Hercules Stingray Pro/V with the IC Works ZoomDAC).
 */

#ifdef INCLUDE_ATT20C498_DAC_TEST
static int att20c498_probe(void)
{
    return 0;
}
#else
#define att20c498_probe 0
#endif

#ifdef INCLUDE_ATT20C498_DAC
static void att20c498_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using AT&T20C498-compatible DAC, 80 MHz rated.\n");
}

static int att20c498_map_clock(int bpp, int pixelclock)
{
    if (bpp == 8 && pixelclock > 80000)
	/* Use 16-bit path, clock doubling at RAMDAC. */
	return pixelclock / 2;
    if (bpp == 16)
	return pixelclock;
    if (bpp == 32)
	return pixelclock * 2;
    return pixelclock;
}

static int att20c498_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    /* Not sure. */
    if (bpp == 8 && pixelclock > 80000)
	/* Use 16-bit path, clock doubling at RAMDAC. */
	return htiming / 2;
    if (bpp == 32)
	return htiming * 2;
    return htiming;
}

static void att20c498_initializestate(unsigned char *regs, int bpp, int colormode,
				      int pixelclock)
{
    regs[0] = 0;
    if (colormode == CLUT8_8)
	regs[0] = 0x02;
    if (colormode == RGB16_555)
	regs[0] = 0x10;
    if (colormode == RGB16_565)
	regs[0] = 0x30;
    if (colormode == RGB32_888_B)
	regs[0] = 0x50;
}

static void att20c498_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 110000);
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = dacspeed / 2;
    cardspecs->mapClock = att20c498_map_clock;
    cardspecs->mapHorizontalCrtc = att20c498_map_horizontal_crtc;
}

DacMethods __svgalib_ATT20C498_methods =
{
    ATT20C498,
    "AT&T20C498 DAC",
    0,
    att20c498_probe,
    att20c498_init,
    att20c498_qualify_cardspecs,
    __svgalib_Sierra_32K_savestate,
    __svgalib_Sierra_32K_restorestate,
    att20c498_initializestate,
    1				/* State size. */
};
#endif
