/*
 * normal.c:
 * 
 * RAMDAC definition for normal VGA DAC.
 * Max dot clock is set at 80 MHz.
 */

#include <stdlib.h>
#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

#ifdef INCLUDE_NORMAL_DAC_TEST
static int normal_dac_probe(void)
{
    return 1;
}
#else
#define normal_dac_probe 0
#endif

#ifdef INCLUDE_NORMAL_DAC
static void normal_dac_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using Normal VGA RAMDAC.\n");
}

static int normal_dac_map_clock(int bpp, int pixelclock)
{
    return pixelclock;
}

static int normal_dac_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    return htiming;
}

static void normal_dac_savestate(unsigned char *regs)
{
}

static void normal_dac_restorestate(const unsigned char *regs)
{
}

static void normal_dac_initializestate(unsigned char *regs, int bpp, int colormode,
				       int pixelclock)
{
    /* Nothing to do. */
}

static void normal_dac_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 80000);
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = 0;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 0;
    cardspecs->mapClock = normal_dac_map_clock;
    cardspecs->mapHorizontalCrtc = normal_dac_map_horizontal_crtc;
}

DacMethods __svgalib_normal_dac_methods =
{
    NORMAL_DAC,
    "Normal VGA DAC",
    0,
    normal_dac_probe,
    normal_dac_init,
    normal_dac_qualify_cardspecs,
    normal_dac_savestate,
    normal_dac_restorestate,
    normal_dac_initializestate,
    0				/* State size. */
};
#endif
