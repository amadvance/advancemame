#include "libvga.h"
#include <stdio.h>

static int __svgalib_r128_inmisc(void)
{
   return 0;
}

static void __svgalib_r128_outmisc(int i)
{
}

static int __svgalib_r128_incrtc(int i)
{
   return 0;
}

static void __svgalib_r128_outcrtc(int i, int d)
{
}

static int __svgalib_r128_inseq(int index)
{
    return 0;
}

static void __svgalib_r128_outseq(int index, int val)
{
}

static int __svgalib_r128_ingra(int index)
{
    return 0;
}

static void __svgalib_r128_outgra(int index, int val)
{
}

static int __svgalib_r128_inis1(void)
{
   return 0;
}

static int __svgalib_r128_inatt(int index)
{
    return 0;
}

static void __svgalib_r128_outatt(int index, int val)
{
}

static void __svgalib_r128_attscreen(int i)
{
}

static void __svgalib_r128_inpal(int i, int *r, int *g, int *b)
{
    volatile int rgb;
    if(chiptype==Radeon) {
        OUTREG(R128_PALETTE_INDEX, i );
        rgb=INREG(R128_PALETTE_DATA);
        *r=(rgb>>16) & 0xff;
        *g=(rgb>>8) & 0xff;
        *b=rgb & 0xff;
        if(dac6bits) {
            *r>>=2;
            *g>>=2;
            *b>>=2;
        }
    } else {
        OUTREG(R128_PALETTE_INDEX, i<<16 );
        rgb=INREG(R128_PALETTE_DATA);
        *r=(rgb>>16) & 0xff;
        *g=(rgb>>8) & 0xff;
        *b=rgb & 0xff;
    }
}

static void __svgalib_r128_outpal(int i, int r, int g, int b)
{
    OUTREG(R128_PALETTE_INDEX, i );
    if((chiptype==Radeon) && dac6bits) {
        r<<=2;
        g<<=2;
        b<<=2;
    }
    OUTREG(R128_PALETTE_DATA, b | (g<<8) | (r<<16) );
}

static void r128_mapio(void)
{
#ifndef __PPC
    if(__svgalib_secondary) {
#endif
        __svgalib_inmisc=__svgalib_r128_inmisc;
        __svgalib_outmisc=__svgalib_r128_outmisc;
        __svgalib_incrtc=__svgalib_r128_incrtc;
        __svgalib_outcrtc=__svgalib_r128_outcrtc;
        __svgalib_inseq=__svgalib_r128_inseq;
        __svgalib_outseq=__svgalib_r128_outseq;
        __svgalib_ingra=__svgalib_r128_ingra;
        __svgalib_outgra=__svgalib_r128_outgra;
        __svgalib_inatt=__svgalib_r128_inatt;
        __svgalib_outatt=__svgalib_r128_outatt;
        __svgalib_attscreen=__svgalib_r128_attscreen;
        __svgalib_inis1=__svgalib_r128_inis1;
#ifndef __PPC
    }
#endif
    __svgalib_inpal=__svgalib_r128_inpal;
    __svgalib_outpal=__svgalib_r128_outpal;
}

