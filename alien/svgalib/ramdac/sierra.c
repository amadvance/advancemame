/*
 * sierra.c:
 * 
 * RAMDAC definition for basic Sierra, SC15025 and SC1148x.
 */

#include <stdlib.h>
#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

/*
 * RAMDAC definition for basic Sierra-type DAC
 * that can do 32K (5-5-5) color mode (16bpp) with doubled VCLK.
 * A value of 0x80 is written to the Hidden DAC register for this mode.
 */

#ifdef INCLUDE_SIERRA_DAC_TEST
static int Sierra_32K_probe(void)
{
    /* Should return 1 for any Sierra-type DAC. */
    return 0;
}
#else
#define Sierra_32K_probe 0
#endif

#ifdef INCLUDE_SIERRA_DAC
static void Sierra_32K_init(void)
{
    /* Should probe the exact DAC type. */
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using Sierra 32K DAC.\n");
}

static int Sierra_32K_map_clock(int bpp, int pixelclock)
{
    if (bpp == 16)
	return pixelclock * 2;
    return pixelclock;
}

static int Sierra_32K_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    if (bpp == 16)
	return htiming * 2;
    return htiming;
}
#endif

#if defined(INCLUDE_SIERRA_DAC) || defined(INCLUDE_ICW_DAC) || \
    defined(INCLUDE_ATT20C490_DAC) || defined(INCLUDE_ATT20C498_DAC)
void __svgalib_Sierra_32K_savestate(unsigned char *regs)
{
    _ramdac_dactocomm();
    regs[0] = port_in(PEL_MSK);
}

void __svgalib_Sierra_32K_restorestate(const unsigned char *regs)
{
    _ramdac_dactocomm();
    port_out_r(PEL_MSK, regs[0]);
}

#endif

#ifdef INCLUDE_SIERRA_DAC
static void Sierra_32K_initializestate(unsigned char *regs, int bpp, int colormode,
				       int pixelclock)
{
    regs[0] = 0;
    if (colormode == RGB16_555)
	regs[0] = 0x80;
}

static void Sierra_32K_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 80000);
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed / 2;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 0;
    cardspecs->mapClock = Sierra_32K_map_clock;
    cardspecs->mapHorizontalCrtc = Sierra_32K_map_horizontal_crtc;
    cardspecs->flags |= NO_RGB16_565;
}

DacMethods __svgalib_Sierra_32K_methods =
{
    SIERRA_32K,
    "Sierra 32K colors VGA DAC",
    0,
    Sierra_32K_probe,
    Sierra_32K_init,
    Sierra_32K_qualify_cardspecs,
    __svgalib_Sierra_32K_savestate,
    __svgalib_Sierra_32K_restorestate,
    Sierra_32K_initializestate,
    1				/* State size. */
};
#endif


/*
 * RAMDAC definition for Sierra 15025/26
 */

#ifdef INCLUDE_SC15025_DAC_TEST
static unsigned char SC15025_Rev;

static int SC15025_probe(void)
{
    unsigned char c, id[4];
    int i, flag = 0;

    _ramdac_dactocomm();
    c = port_in(PEL_MSK);
    _ramdac_setcomm(c | 0x10);
    for (i = 0; i < 4; i++) {
	port_out_r(PEL_IR, 0x9 + i);
	id[i] = port_in(PEL_IW);
    }
    _ramdac_setcomm(c);
    _ramdac_dactopel();
    if (id[0] == 'S' &&		/* Sierra */
	((id[1] << 8) | id[2]) == 15025) {	/* unique for the SC 15025/26 */
	flag = 1;
	SC15025_Rev = id[3];
    }
    return flag;
}
#else
#define SC15025_probe 0
#define SC15025_Rev ' '
#endif

#ifdef INCLUDE_SC15025_DAC
static void SC15025_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using Sierra 15025/26%c truecolor DAC.\n", SC15025_Rev);
}

static void SC15025_initializestate(unsigned char *regs, int bpp, int colormode,
				    int pixelclock)
{
    regs[0] = 0;
    regs[1] = 0;
    regs[2] = 0;
    if (colormode == RGB16_555) {
	regs[0] = 0x80;
	regs[1] = 1;
    } else if (colormode == RGB16_565) {
	regs[0] = 0xC0;
	regs[1] = 1;
    } else if (colormode == RGB32_888_B) {
	regs[0] = 0x40;
	regs[1] = 1;
	regs[2] = 1;
    }
    /* ARI: FIXME: regs[1] should be 1 for CLUT8_8 */
    /*      also: OR 8 to regs[0] to enable gamma correction */
}

static void SC15025_savestate(unsigned char *regs)
{
    _ramdac_dactocomm();
    regs[0] = port_in(PEL_MSK);
    _ramdac_setcomm(regs[0] | 0x10);
    _ramdac_dactocomm();
    port_out_r(PEL_IR, 8);
    regs[1] = port_in(PEL_IW);	/* Aux control */
    port_out_r(PEL_IR, 16);
    regs[2] = port_in(PEL_IW);	/* Pixel Repack */
    _ramdac_setcomm(regs[0]);
}

static void SC15025_restorestate(const unsigned char *regs)
{
    unsigned char c;

    _ramdac_dactocomm();
    c = port_in(PEL_MSK);
    _ramdac_setcomm(c | 0x10);
    _ramdac_dactocomm();
    port_out_r(PEL_IR, 8);
    port_out_r(PEL_IW, regs[1]);	/* Aux control */
    port_out_r(PEL_IR, 16);
    port_out_r(PEL_IW, regs[2]);	/* Pixel Repack */
    _ramdac_setcomm(c);
    _ramdac_setcomm(regs[0]);
}

static int SC15025_map_clock(int bpp, int pixelclock)
{
    if (bpp == 32)
	return pixelclock * 2;
    return pixelclock;
}

static int SC15025_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
    if (bpp == 16)
	return htiming * 2;
    if (bpp == 32)
	return htiming * 4;
    return htiming;
}

static void SC15025_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 110000);
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed / 2;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = dacspeed / 3;
    cardspecs->mapClock = SC15025_map_clock;
    cardspecs->mapHorizontalCrtc = SC15025_map_horizontal_crtc;
}

DacMethods __svgalib_SC15025_methods =
{
    SIERRA_15025,
    "Sierra SC15025/6 DAC",
    0,
    SC15025_probe,
    SC15025_init,
    SC15025_qualify_cardspecs,
    SC15025_savestate,
    SC15025_restorestate,
    SC15025_initializestate,
    3				/* State size. */
};
#endif

/*
 * RAMDAC definition for Sierra 1148x Series.
 * 11482, 83, and 84 (Mark 2) can do 32K (5-5-5) color mode (16bpp).
 * 11485, 87, and 89 (Mark 3) additionally can do 64K (5-6-5) color mode,
 * but are not autodetected since they cannot be distinguished from Mark 2.
 * 11486 really is a Sierra 32K dac, and should have been set by user.
 * 
 * Note that these dacs are different from 'Sierra 32K', since they can be
 * detected as such, while there are clones that work compatible to the
 * Sierra dacs, but cannot be autodetected. To avoid such dacs to fail
 * the type 'Sierra 32K' still refers to them, while this new type 
 * 'SC1148x Series' refers to original Sierra dacs.
 *
 * ATTENTION: THIS TEST MUST BE LAST IN CHAIN, SINCE MANY BETTER DACS
 * IMPLEMENT 32K MODES COMPATIBLE TO THIS ONE AND WOULD BE DETECTED AS
 * SIERRA!
 *
 */

#ifdef INCLUDE_SC1148X_DAC_TEST
static int SC1148X_probe(void)
{
    unsigned char oc, op, tmp, tmp2;
    int flag = 0;

    _ramdac_dactopel();
    tmp = port_in(PEL_MSK);
    do {
	tmp2 = tmp;
	tmp = port_in(PEL_MSK);
    } while (tmp2 != tmp);
    port_in(PEL_IW);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
    for (tmp2 = 9; tmp != 0x8E && tmp2 > 0; tmp2--)
	tmp = port_in(PEL_MSK);
    if (tmp != 0x8E) {
        _ramdac_dactocomm();
        oc = port_in(PEL_MSK);
        _ramdac_dactopel();
        op = port_in(PEL_MSK);
        tmp = oc ^ 0xFF;
        port_out_r(PEL_MSK, tmp);
        _ramdac_dactocomm();
        tmp2 = port_in(PEL_MSK);
        if (tmp2 != tmp) {
            tmp = _ramdac_setcomm(tmp = oc ^ 0x60);
            if ((tmp & 0xe0) == (tmp2 & 0xe0)) {
                tmp = port_in(PEL_MSK);
                _ramdac_dactopel();
                if (tmp != port_in(PEL_MSK))
		    flag = 1; /* Sierra Mark 2 or 3 */
	    } else {
		/* We have a Sierra SC11486 */
#ifdef INCLUDE_SIERRA_DAC
		flag = 1;
		/* We do some ugly trickery here to patch SC1148X Series
		    descriptor with values from Sierra 32K descriptor, since
		    this is what whe really have detected! */
		__svgalib_SC1148X_methods.id = SIERRA_32K;
		__svgalib_SC1148X_methods.name = __svgalib_Sierra_32K_methods.name;
		__svgalib_SC1148X_methods.initialize = __svgalib_Sierra_32K_methods.initialize;
		__svgalib_SC1148X_methods.qualifyCardSpecs = __svgalib_Sierra_32K_methods.qualifyCardSpecs ;
		__svgalib_SC1148X_methods.initializeState = __svgalib_Sierra_32K_methods.initializeState ;
#endif
	    }
	    _ramdac_dactocomm();
	    port_out_r(PEL_MSK, oc);
	}
	_ramdac_dactopel();
	port_out_r(PEL_MSK, op);
    } else {
	_ramdac_dactopel();
	/* Diamond SS2410 */
    }
    return flag;
}
#else
#define SC1148x_probe 0
#endif

#ifdef INCLUDE_SC1148X_DAC
static void SC1148X_init(void)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"svgalib: Using Sierra 1148x series 32K DAC.\n");
}

static void SC1148X_initializestate(unsigned char *regs, int bpp, int colormode,
				       int pixelclock)
{
    regs[0] = 0;
    if (colormode == RGB16_555)
	regs[0] = 0xA0;
    /* Mark 3 (not autodetected) */
    else if (colormode == RGB16_565)
	regs[0] = 0xE0;
}

static void SC1148X_qualify_cardspecs(CardSpecs * cardspecs, int dacspeed)
{
    dacspeed = __svgalib_setDacSpeed(dacspeed, 110000);
    cardspecs->maxPixelClock4bpp = dacspeed;
    cardspecs->maxPixelClock8bpp = dacspeed;
    cardspecs->maxPixelClock16bpp = dacspeed / 2;
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 0;
    cardspecs->mapClock = Sierra_32K_map_clock;
    cardspecs->mapHorizontalCrtc = Sierra_32K_map_horizontal_crtc;
    cardspecs->flags |= NO_RGB16_565;	/* Mark 3 (11485,87, and higher) can */
}

DacMethods __svgalib_SC1148X_methods =
{
    SIERRA_1148X,
    "Sierra SC1148x series 32K colors VGA DAC",
    0,
    SC1148X_probe,
    SC1148X_init,
    SC1148X_qualify_cardspecs,
    __svgalib_Sierra_32K_savestate,
    __svgalib_Sierra_32K_restorestate,
    SC1148X_initializestate,
    1				/* State size. */
};
#endif
