/*
 * ramdac.c:
 * 
 * This file contains RAMDAC definitions of type DacMethods for
 * various DACs.
 *
 * Note that the restoreState function is the only function that
 * should program the DAC registers; the initializeState function
 * should merely define the values that will be written in a
 * subsequent call of the restore funtion.
 */

#include <stdlib.h>
#include <stdio.h>
#include "libvga.h"

#include "timing.h"
#include "vgaregs.h"
#include "svgadriv.h"		/* for __svgalib_driver_report */
#include "ramdac.h"

/*
 * The following function probes the DACs in daclist, which must be
 * terminated by NULL. It returns the detected DAC if successful, NULL
 * otherwise. The detected DAC is also initialized.
 */

DacMethods *__svgalib_probeDacs(DacMethods ** dacs_to_probe)
{
    /* Probe for a RAMDAC. */
    for (;;) {
	DacMethods *dac;
	dac = *dacs_to_probe;
	if (dac == NULL)
	    /* None found. */
	    return NULL;
	if (dac->probe()) {
	    dac->initialize();
	    return dac;
	}
	dacs_to_probe++;
    }
}

int __svgalib_setDacSpeed(int dacspeed, int defspeed)
{
    if (!dacspeed) {
	if (__svgalib_driver_report)
	    fprintf(stderr,"svgalib: Assuming %dMHz DAC.\n", defspeed / 1000);
	dacspeed = defspeed;
    } else {
	if (__svgalib_driver_report)
	    fprintf(stderr,"svgalib: DAC speed set to %dMHz.\n", dacspeed / 1000);
    }
    return dacspeed;
}

#ifndef __OPTIMIZE__	/* otherwise inlined from ramdac.h */
void _ramdac_dactocomm(void)
{
    port_in(PEL_IW);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
    port_in(PEL_MSK);
}

void _ramdac_dactopel(void)
{
    port_in(PEL_IW);
}

unsigned char _ramdac_setcomm(unsigned char data)
{
    _ramdac_dactocomm();
    port_out_r(PEL_MSK, data);
    _ramdac_dactocomm();
    return port_in(PEL_MSK);
}
#endif

/*
 * List of all DACs.
 */

DacMethods *__svgalib_all_dacs[] =
{
#ifdef INCLUDE_NORMAL_DAC
    &__svgalib_normal_dac_methods,
#endif
#ifdef INCLUDE_S3_SDAC_DAC
    &__svgalib_S3_SDAC_methods,
#endif
#ifdef INCLUDE_S3_GENDAC_DAC
    &__svgalib_S3_GENDAC_methods,
#endif
#ifdef INCLUDE_S3_TRIO64_DAC
    &__svgalib_Trio64_methods,
#endif
#ifdef INCLUDE_SIERRA_DAC
    &__svgalib_Sierra_32K_methods,
#endif
#ifdef INCLUDE_SC15025_DAC
    &__svgalib_SC15025_methods,
#endif
#ifdef INCLUDE_ATT20C490_DAC
    &__svgalib_ATT20C490_methods,
#endif
#ifdef INCLUDE_ATT20C498_DAC
    &__svgalib_ATT20C498_methods,
#endif
#ifdef INCLUDE_ICW_DAC
    &__svgalib_ICW_methods,
#endif
#ifdef INCLUDE_SC1148X_DAC
    &__svgalib_SC1148X_methods,
#endif
#ifdef INCLUDE_ICS_GENDAC_DAC
    &__svgalib_ICS_GENDAC_methods,
#endif
    NULL
};
