#include "libvga.h"
#include <stdio.h>

int __svgalib_vga_inmisc(void)
{
   return port_in(MIS_R);
}

void __svgalib_vga_outmisc(int i)
{
   port_out_r(MIS_W,i);
}

int __svgalib_vga_incrtc(int i)
{
   port_out_r(__svgalib_CRT_I,i);
   return port_in(__svgalib_CRT_D);
}

void __svgalib_vga_outcrtc(int i, int d)
{
    port_out_r(__svgalib_CRT_I, i);
    port_out_r(__svgalib_CRT_D, d);
}

int __svgalib_vga_inseq(int index)
{
    port_out_r(SEQ_I, index);
    return port_in(SEQ_D);
}

void __svgalib_vga_outseq(int index, int val)
{
    port_out_r(SEQ_I, index);
    port_out_r(SEQ_D, val);
}

int __svgalib_vga_ingra(int index)
{
    port_out_r(GRA_I, index);
    return port_in(GRA_D);
}

void __svgalib_vga_outgra(int index, int val)
{
    port_out_r(GRA_I, index);
    port_out_r(GRA_D, val);
}

int __svgalib_vga_inis1(void)
{
   return port_in(__svgalib_IS1_R);
}

#ifdef NO_DELAY

int __svgalib_vga_inatt(int index)
{
    __svgalib_vga_inis1();
    port_out_r(ATT_IW, index);
    return port_in(ATT_R);
}

void __svgalib_vga_outatt(int index, int val)
{
    __svgalib_vga_inis1();
    port_out_r(ATT_IW, index);
    port_out_r(ATT_IW, val);
}

void __svgalib_vga_attscreen(int i)
{
    __svgalib_vga_inis1();
    port_out_r(ATT_IW, i);
}

void __svgalib_vga_inpal(int i, int *r, int *g, int *b)
{
    port_out_r(PEL_IR,i);
    *r=port_in(PEL_D);
    *g=port_in(PEL_D);
    *b=port_in(PEL_D);
}

void __svgalib_vga_outpal(int i, int r, int g, int b)
{

    port_out_r(PEL_IW,i);
    port_out_r(PEL_D,r);
    port_out_r(PEL_D,g);
    port_out_r(PEL_D,b);
}

#else /* NO_DELAY */

int __svgalib_vga_inatt(int index)
{
    __svgalib_delay();
    __svgalib_vga_inis1();
    __svgalib_delay();
    port_out_r(ATT_IW, index);
    __svgalib_delay();
    return port_in(ATT_R);
}

void __svgalib_vga_outatt(int index, int val)
{
    __svgalib_delay();
    __svgalib_vga_inis1();
    __svgalib_delay();
    port_out_r(ATT_IW, index);
    __svgalib_delay();
    port_out_r(ATT_IW, val);
}

void __svgalib_vga_attscreen(int i)
{
    __svgalib_delay();
    __svgalib_vga_inis1();
    __svgalib_delay();
    port_out_r(ATT_IW, i);
}

void __svgalib_vga_inpal(int i, int *r, int *g, int *b)
{
    port_out_r(PEL_IR,i);
    __svgalib_delay();
    *r=port_in(PEL_D);
    __svgalib_delay();
    *g=port_in(PEL_D);
    __svgalib_delay();
    *b=port_in(PEL_D);
}

void __svgalib_vga_outpal(int i, int r, int g, int b)
{

    port_out_r(PEL_IW,i);
    __svgalib_delay();
    port_out_r(PEL_D,r);
    __svgalib_delay();
    port_out_r(PEL_D,g);
    __svgalib_delay();
    port_out_r(PEL_D,b);
}

#endif /* NO_DELAY */
