/*

LIBDOS - SVGALIB port to MSDOS for the DJGPP compiler.

Copyright (C) 1999-2001 Andrea Mazzoleni

*/

#ifndef __LIBDOS_H
#define __LIBDOS_H

#include "card.h"

#ifndef __MSDOS__
#error This module is for MSDOS only
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

/**************************************************************************/
/* os */

#define MAP_SHARED 0x1
#define MAP_FIXED 0x2
#define MAP_FAILED ((void*)(-1))
#define PROT_READ 0
#define PROT_WRITE 0

void* mmap(void* start, unsigned length, int prot, int flags, int fd, unsigned offset);
int munmap(void* start, unsigned length);
int iopl(int perm);

/**************************************************************************/
/* driver */

#define INCLUDE_NV3_DRIVER
#define INCLUDE_TRIDENT_DRIVER
#define INCLUDE_RENDITION_DRIVER
#define INCLUDE_G400_DRIVER
#define INCLUDE_PM2_DRIVER
#define INCLUDE_SAVAGE_DRIVER
#define INCLUDE_MILLENNIUM_DRIVER
#define INCLUDE_R128_DRIVER
#define INCLUDE_BANSHEE_DRIVER
#define INCLUDE_SIS_DRIVER
#define INCLUDE_I740_DRIVER
/* #define INCLUDE_I810_DRIVER */ /* TODO No linear mode from 1.9.15. It requires special kernel support. */
#define INCLUDE_LAGUNA_DRIVER
#define INCLUDE_RAGE_DRIVER
#define INCLUDE_MX_DRIVER
/* #define INCLUDE_NEO_DRIVER */ /* Don't support the external screen, only the LCD */
/* #define INCLUDE_CHIPS_DRIVER */ /* Not imported */
/* #define INCLUDE_MACH64_DRIVER */ /* Not imported */
/* #define INCLUDE_MACH32_DRIVER */ /* Not imported */
/* #define INCLUDE_EGA_DRIVER */ /* Not a modeline driver */
#define INCLUDE_ET6000_DRIVER
/* #define INCLUDE_ET4000_DRIVER */ /* Not imported */
/* #define INCLUDE_TVGA_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_CIRRUS_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_OAK_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_PARADISE_DRIVER */ /* Not a modeline driver */
#define INCLUDE_S3_DRIVER
/* #define INCLUDE_ET3000_DRIVER */ /* Not a modeline driver */
#define INCLUDE_ARK_DRIVER
/* #define INCLUDE_GVGA6400_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_ATI_DRIVER */ /* Not a modeline driver */
/* #define INCLUDE_ALI_DRIVER */ /* Not a modeline driver */
#define INCLUDE_APM_DRIVER

/**************************************************************************/
/* ramdac */

#define INCLUDE_NORMAL_DAC
#define INCLUDE_S3_SDAC_DAC
#define INCLUDE_S3_GENDAC_DAC
#define INCLUDE_S3_TRIO64_DAC
#define INCLUDE_SIERRA_DAC
#define INCLUDE_SC15025_DAC
#define INCLUDE_ATT20C490_DAC
#define INCLUDE_ATT20C498_DAC
#define INCLUDE_ICW_DAC
#define INCLUDE_IBMRGB52x_DAC
#define INCLUDE_SC1148X_DAC
#define INCLUDE_ICS_GENDAC_DAC

/**************************************************************************/
/* ramdac test */

#define INCLUDE_NORMAL_DAC_TEST
#define INCLUDE_S3_SDAC_DAC_TEST
#define INCLUDE_S3_GENDAC_DAC_TEST
#define INCLUDE_S3_TRIO64_DAC_TEST
#define INCLUDE_SIERRA_DAC_TEST
#define INCLUDE_SC15025_DAC_TEST
#define INCLUDE_ATT20C490_DAC_TEST
#define INCLUDE_ATT20C498_DAC_TEST
#define INCLUDE_ICW_DAC_TEST
#define INCLUDE_IBMRGB52x_DAC_TEST
#define INCLUDE_SC1148X_DAC_TEST
#define INCLUDE_ICS_GENDAC_DAC_TEST

/**************************************************************************/
/* libdos */

void libdos_init(int divide_clock_with_sequencer);
void libdos_done(void);

void libdos_mode_init(unsigned pixelclock, unsigned hde, unsigned hrs, unsigned hre, unsigned ht, unsigned vde, unsigned vrs, unsigned vre, unsigned vt, int doublescan, int interlace, int hsync, int vsync, unsigned bits_per_pixel, int tvpal, int tvntsc);
void libdos_mode_done(void);

extern card_crtc libdos_crtc;
extern card_mode libdos_mode;
extern int libdos_mode_number;

#endif
