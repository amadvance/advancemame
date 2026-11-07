#include "libvga.h"
#include "memio.h"

unsigned long __svgalib_vgammbase;

int __svgalib_mm_inmisc(void)
{
   return v_readb(__svgalib_vgammbase+MIS_R);
}

void __svgalib_mm_outmisc(int i)
{
   v_writeb(i, __svgalib_vgammbase+MIS_W);
}

int __svgalib_mm_incrtc(int i)
{
   v_writeb(i, __svgalib_vgammbase+__svgalib_CRT_I);
   return v_readb(__svgalib_vgammbase+__svgalib_CRT_D);
}

void __svgalib_mm_outcrtc(int i, int d)
{
   v_writeb(i, __svgalib_vgammbase+__svgalib_CRT_I);
   v_writeb(d, __svgalib_vgammbase+__svgalib_CRT_D);
}

int __svgalib_mm_inseq(int i)
{
   v_writeb(i, __svgalib_vgammbase+SEQ_I);
   return v_readb(__svgalib_vgammbase+SEQ_D);
}

void __svgalib_mm_outseq(int i, int val)
{
   v_writeb(i, __svgalib_vgammbase+SEQ_I);
   v_writeb(val, __svgalib_vgammbase+SEQ_D);
}

int __svgalib_mm_ingra(int index)
{
   v_writeb(index, __svgalib_vgammbase+GRA_I);
   return v_readb(__svgalib_vgammbase+GRA_D);
}

void __svgalib_mm_outgra(int index, int val)
{
   v_writeb(index, __svgalib_vgammbase+GRA_I);
   v_writeb(val, __svgalib_vgammbase+GRA_D);
}

int __svgalib_mm_inis1(void)
{
   return v_readb(__svgalib_vgammbase+__svgalib_IS1_R);
}

int __svgalib_mm_inatt(int index)
{
    __svgalib_mm_inis1();
    v_writeb(index, __svgalib_vgammbase+ATT_IW);
    return v_readb(__svgalib_vgammbase+ATT_R);
}

void __svgalib_mm_outatt(int index, int val)
{
    __svgalib_mm_inis1();
    v_writeb(index, __svgalib_vgammbase+ATT_IW);
    v_writeb(val, __svgalib_vgammbase+ATT_IW);
}

void __svgalib_mm_attscreen(int i)
{
    __svgalib_mm_inis1();
    v_writeb(i, __svgalib_vgammbase+ATT_IW);
}

void __svgalib_mm_inpal(int i, int *r, int *g, int *b)
{
    v_writeb(i, __svgalib_vgammbase+PEL_IR);
    *r=v_readb(__svgalib_vgammbase+PEL_D);
    *g=v_readb(__svgalib_vgammbase+PEL_D);
    *b=v_readb(__svgalib_vgammbase+PEL_D);
}

void __svgalib_mm_outpal(int i, int r, int g, int b)
{
    v_writeb(i, __svgalib_vgammbase+PEL_IW);
    v_writeb(r, __svgalib_vgammbase+PEL_D);
    v_writeb(g, __svgalib_vgammbase+PEL_D);
    v_writeb(b, __svgalib_vgammbase+PEL_D);
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
