/*
 * btdacs.c:
 * 
 * RAMDAC definition for Bt485 
 *
 * NON-FUNCTIONAL
 */

#include <stdlib.h>
#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

/*
 * RAMDAC definition for industry-standard AT&T20C498 DAC with 16-bit
 * pixel port, and compatibles.
 * Differently rated versions exist, such as 80, 110, 135 and 170 MHz.
 * This code assumes the DAC is actually connected with a 16-bit path.
 * (an example of a 498-compatible DAC being used with a 8-bit path
 * is the Hercules Stingray Pro/V with the IC Works ZoomDAC).
 */

#ifdef INCLUDE_BT485_DAC_TEST
static int bt485_probe(void)
{
    return 0;
}
#else
#define bt485_probe 0
#endif

#ifdef INCLUDE_BT485_DAC
static void bt485_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using BT485 DAC, 135 MHz rated.\n");
}

static int bt485_map_clock(int bpp, int pixelclock)
{
    return pixelclock;
    
    if (bpp == 8 && pixelclock > 80000)
	/* Use 16-bit path, clock doubling at RAMDAC. */
	return pixelclock / 2;
    if (bpp == 16)
	return pixelclock;
    if (bpp == 32)
	return pixelclock * 2;
    return pixelclock;
}

static int bt485_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    return htiming

    /* Not sure. */
    if (bpp == 8 && pixelclock > 80000)
	/* Use 16-bit path, clock doubling at RAMDAC. */
	return htiming / 2;
    if (bpp == 32)
	return htiming * 2;
    return htiming;
}

static void bt485_initializestate(unsigned char *regs, int bpp, int colormode,
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

static void 485_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 135000);
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = dacspeed ;
    cardspecs->mapClock = bt485_map_clock;
    cardspecs->mapHorizontalCrtc = att20c498_map_horizontal_crtc;
}

DacMethods __svgalib_BT485_methods =
{
    BT485,
    "BT485 DAC",
    0,
    bt485_probe,
    bt485_init,
    bt485_qualify_cardspecs,
    __svgalib_Sierra_32K_savestate,
    __svgalib_Sierra_32K_restorestate,
    bt485_initializestate,
    1				/* State size. */
};
#endif
