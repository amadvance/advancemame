#include "libvga.h"

unsigned char *__svgalib_vgammbase;

int __svgalib_mm_inmisc(void)
{
   return *((volatile unsigned char *)__svgalib_vgammbase+MIS_R);
}

void __svgalib_mm_outmisc(int i)
{
   *((volatile unsigned char *)__svgalib_vgammbase+MIS_W) = i;
}

int __svgalib_mm_incrtc(int i)
{
   *((volatile unsigned char *)__svgalib_vgammbase+__svgalib_CRT_I)=i;
   return *((volatile unsigned char *)__svgalib_vgammbase+__svgalib_CRT_D);
}

void __svgalib_mm_outcrtc(int i, int d)
{
   *((volatile unsigned char *)__svgalib_vgammbase+__svgalib_CRT_I)=i;
   *((volatile unsigned char *)__svgalib_vgammbase+__svgalib_CRT_D)=d;
}

int __svgalib_mm_inseq(int index)
{
   *((volatile unsigned char *)__svgalib_vgammbase+SEQ_I)=index;
   return *((volatile unsigned char *)__svgalib_vgammbase+SEQ_D);
}

void __svgalib_mm_outseq(int index, int val)
{
   *((volatile unsigned char *)__svgalib_vgammbase+SEQ_I)=index;
   *((volatile unsigned char *)__svgalib_vgammbase+SEQ_D)=val;
}

int __svgalib_mm_ingra(int index)
{
   *((volatile unsigned char *)__svgalib_vgammbase+GRA_I)=index;
   return *((volatile unsigned char *)__svgalib_vgammbase+GRA_D);
}

void __svgalib_mm_outgra(int index, int val)
{
   *((volatile unsigned char *)__svgalib_vgammbase+GRA_I)=index;
   *((volatile unsigned char *)__svgalib_vgammbase+GRA_D)=val;
}

int __svgalib_mm_inis1(void)
{
   return *((volatile unsigned char *)__svgalib_vgammbase+__svgalib_IS1_R);
}

int __svgalib_mm_inatt(int index)
{
    __svgalib_mm_inis1();
    *((volatile unsigned char *)__svgalib_vgammbase+ATT_IW)=index;
    return *((volatile unsigned char *)__svgalib_vgammbase+ATT_R);
}

void __svgalib_mm_outatt(int index, int val)
{
    __svgalib_mm_inis1();
    *((volatile unsigned char *)__svgalib_vgammbase+ATT_IW)=index;
    *((volatile unsigned char *)__svgalib_vgammbase+ATT_IW)=val;
}

void __svgalib_mm_attscreen(int i)
{
    __svgalib_mm_inis1();
    *((volatile unsigned char *)__svgalib_vgammbase+ATT_IW)=i;
}

void __svgalib_mm_inpal(int i, int *r, int *g, int *b)
{
    *((volatile unsigned char *)__svgalib_vgammbase+PEL_IR)=i;
    *r=*((volatile unsigned char *)__svgalib_vgammbase+PEL_D);
    *g=*((volatile unsigned char *)__svgalib_vgammbase+PEL_D);
    *b=*((volatile unsigned char *)__svgalib_vgammbase+PEL_D);
}

void __svgalib_mm_outpal(int i, int r, int g, int b)
{
    *((volatile unsigned char *)__svgalib_vgammbase+PEL_IW)=i;
    *((volatile unsigned char *)__svgalib_vgammbase+PEL_D)=r;
    *((volatile unsigned char *)__svgalib_vgammbase+PEL_D)=g;
    *((volatile unsigned char *)__svgalib_vgammbase+PEL_D)=b;
}

void __svgalib_mm_io_mapio(void)
{
    __svgalib_inmisc=__svgalib_mm_inmisc;
    __svgalib_outmisc=__svgalib_mm_outmisc;
    __svgalib_incrtc=__svgalib_mm_incrtc;
    __svgalib_outcrtc=__svgalib_mm_outcrtc;
    __svgalib_inseq=__svgalib_mm_inseq;
    __svgalib_outseq=__svgalib_mm_outseq;
    __svgalib_ingra=__svgalib_mm_ingra;
    __svgalib_outgra=__svgalib_mm_outgra;
    __svgalib_inatt=__svgalib_mm_inatt;
    __svgalib_outatt=__svgalib_mm_outatt;
    __svgalib_attscreen=__svgalib_mm_attscreen;
    __svgalib_inis1=__svgalib_mm_inis1;
    __svgalib_inpal=__svgalib_mm_inpal;
    __svgalib_outpal=__svgalib_mm_outpal;
}
