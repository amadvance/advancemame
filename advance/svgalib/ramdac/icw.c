/*
 * icw.c:
 *
 * RAMDAC definition for IC Works DACs.
 * This version only supports the 16-bit ZoomDAC (w30C516), which 
 * is compatible with the AT&T 20C498.
 * This DAC exists in 110, 135 and 170 MHz versions.
 * It can do packed 24-bit color (BG-RB-GR).
 * The 170 MHz version has a PCLK limit of 135 MHz
 * (170 pixel clock for 16-bit path for 8bpp LUT).
 */

#include <stdlib.h>
#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

#ifdef INCLUDE_ICW_DAC_TEST
static int ICW_probe(void)
{
    unsigned char mi, di;

    _ramdac_dactocomm();
    port_in(PEL_MSK);		/* Control register 0. */
    mi = port_in(PEL_MSK);		/* Manufacturer ID. */
    di = port_in(PEL_MSK);		/* Device ID. */
    if (mi == 0x84) {
	if (di == 0x98)
	    return 1;
	fprintf(stderr,"svgalib: ICW_probe: Unknown IC Works DAC.\n");
    }
    return 0;
}
#else
#define ICW_probe 0
#endif

#ifdef INCLUDE_ICW_DAC
static void ICW_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using IC Works DAC (AT&T20C498-compatible).\n");
}

static int ICW_map_clock(int bpp, int pixelclock)
{
    if (bpp == 8 && pixelclock > 80000)
	/* Use 16-bit path, clock doubling at RAMDAC. */
	return pixelclock / 2;
    if (bpp == 16)
	return pixelclock;
    if (bpp == 24)
	/* Use the packed 24-bit mode. */
	return pixelclock * 3 / 2;
    if (bpp == 32)
	return pixelclock * 2;
    return pixelclock;
}

static int ICW_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    /* Not sure. */
    if (bpp == 8 && pixelclock > 80000)
	/* Use 16-bit path, clock doubling at RAMDAC. */
	return htiming / 2;
    if (bpp == 24)
	return htiming * 3 / 2;
    if (bpp == 32)
	return htiming * 2;
    return htiming;
}

static void ICW_initializestate(unsigned char *regs, int bpp, int colormode,
				int pixelclock)
{
    regs[0] = 0;
    if (colormode == CLUT8_8)
	regs[0] = 0x02;
    if (colormode == RGB16_555)
	regs[0] = 0x10;
    if (colormode == RGB16_565)
	regs[0] = 0x30;
    if (colormode == RGB24_888_B)
	/* Packed mode. */
	regs[0] = 0xB0;
    if (colormode == RGB32_888_B)
	regs[0] = 0x50;
}

static void ICW_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 110000);
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed;
    cardspecs->maxPixelClock24bpp = dacspeed * 2 / 3;
    cardspecs->maxPixelClock32bpp = dacspeed / 2;
    cardspecs->mapClock = ICW_map_clock;
    cardspecs->mapHorizontalCrtc = ICW_map_horizontal_crtc;
}

DacMethods __svgalib_ICW_methods =
{
    IC_WORKS,
    "IC Works DAC",
    0,
    ICW_probe,
    ICW_init,
    ICW_qualify_cardspecs,
    __svgalib_Sierra_32K_savestate,
    __svgalib_Sierra_32K_restorestate,
    ICW_initializestate,
    1				/* State size. */
};
#endif
