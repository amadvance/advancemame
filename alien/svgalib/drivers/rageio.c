#include "libvga.h"
#include <stdio.h>

static int __svgalib_rage_inmisc(void)
{
   return 0;
}

static void __svgalib_rage_outmisc(int i)
{
}

static int __svgalib_rage_incrtc(int i)
{
   return 0;
}

static void __svgalib_rage_outcrtc(int i, int d)
{
}

static int __svgalib_rage_inseq(int index)
{
    return 0;
}

static void __svgalib_rage_outseq(int index, int val)
{
}

static int __svgalib_rage_ingra(int index)
{
    return 0;
}

static void __svgalib_rage_outgra(int index, int val)
{
}

static int __svgalib_rage_inis1(void)
{
   return 0;
}

static int __svgalib_rage_inatt(int index)
{
    return 0;
}

static void __svgalib_rage_outatt(int index, int val)
{
}

static void __svgalib_rage_attscreen(int i)
{
}

static void __svgalib_rage_inpal(int i, int *r, int *g, int *b)
{
    rage_outb(ATIIOPortDAC_READ, i);
    *r=rage_inb(ATIIOPortDAC_DATA);
    *g=rage_inb(ATIIOPortDAC_DATA);
    *b=rage_inb(ATIIOPortDAC_DATA);
}

static void __svgalib_rage_outpal(int i, int r, int g, int b)
{
    rage_outb(ATIIOPortDAC_WRITE, i);
    rage_outb(ATIIOPortDAC_DATA,r);
    rage_outb(ATIIOPortDAC_DATA,g);
    rage_outb(ATIIOPortDAC_DATA,b);
}

static void rage_mapio(void)
{
#ifndef __PPC
    if(__svgalib_secondary) {
#endif
        __svgalib_inmisc=__svgalib_rage_inmisc;
        __svgalib_outmisc=__svgalib_rage_outmisc;
        __svgalib_incrtc=__svgalib_rage_incrtc;
        __svgalib_outcrtc=__svgalib_rage_outcrtc;
        __svgalib_inseq=__svgalib_rage_inseq;
        __svgalib_outseq=__svgalib_rage_outseq;
        __svgalib_ingra=__svgalib_rage_ingra;
        __svgalib_outgra=__svgalib_rage_outgra;
        __svgalib_inatt=__svgalib_rage_inatt;
        __svgalib_outatt=__svgalib_rage_outatt;
        __svgalib_attscreen=__svgalib_rage_attscreen;
        __svgalib_inis1=__svgalib_rage_inis1;
#ifndef __PPC
    }
#endif
    __svgalib_inpal=__svgalib_rage_inpal;
    __svgalib_outpal=__svgalib_rage_outpal;
}

