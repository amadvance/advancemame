#include "libvga.h"
#include <stdio.h>

static int __svgalib_pm2_inmisc(void)
{
   return 0;
}

static void __svgalib_pm2_outmisc(int i)
{
}

static int __svgalib_pm2_incrtc(int i)
{
   return 0;
}

static void __svgalib_pm2_outcrtc(int i, int d)
{
}

static int __svgalib_pm2_inseq(int index)
{
    return 0;
}

static void __svgalib_pm2_outseq(int index, int val)
{
}

static int __svgalib_pm2_ingra(int index)
{
    return 0;
}

static void __svgalib_pm2_outgra(int index, int val)
{
}

static int __svgalib_pm2_inis1(void)
{
   return 0;
}

static int __svgalib_pm2_inatt(int index)
{
    return 0;
}

static void __svgalib_pm2_outatt(int index, int val)
{
}

static void __svgalib_pm2_attscreen(int i)
{
}

static void __svgalib_pm2_inpal(int i, int *r, int *g, int *b)
{
    Permedia2ReadAddress(i);
    *r=Permedia2ReadData();
    *g=Permedia2ReadData();
    *b=Permedia2ReadData();
}

static void __svgalib_pm2_outpal(int i, int r, int g, int b)
{
    Permedia2WriteAddress(i);
    Permedia2WriteData(r);
    Permedia2WriteData(g);
    Permedia2WriteData(b);
}

static void pm2_mapio(void)
{
#ifndef __PPC
    if(__svgalib_secondary) {
#endif
        __svgalib_inmisc=__svgalib_pm2_inmisc;
        __svgalib_outmisc=__svgalib_pm2_outmisc;
        __svgalib_incrtc=__svgalib_pm2_incrtc;
        __svgalib_outcrtc=__svgalib_pm2_outcrtc;
        __svgalib_inseq=__svgalib_pm2_inseq;
        __svgalib_outseq=__svgalib_pm2_outseq;
        __svgalib_ingra=__svgalib_pm2_ingra;
        __svgalib_outgra=__svgalib_pm2_outgra;
        __svgalib_inatt=__svgalib_pm2_inatt;
        __svgalib_outatt=__svgalib_pm2_outatt;
        __svgalib_attscreen=__svgalib_pm2_attscreen;
        __svgalib_inis1=__svgalib_pm2_inis1;
#ifndef __PPC
    }
#endif
    __svgalib_inpal=__svgalib_pm2_inpal;
    __svgalib_outpal=__svgalib_pm2_outpal;
}

