#include "libvga.h"
#include <stdio.h>

int __svgalib_vga_inmisc(void)
{
   return port_in(MIS_R);
}

void __svgalib_vga_outmisc(int i)
{
   outb(MIS_W,i);
}

int __svgalib_vga_incrtc(int i)
{
   outb(__svgalib_CRT_I,i);
   return port_in(__svgalib_CRT_D);
}

void __svgalib_vga_outcrtc(int i, int d)
{
    outb(__svgalib_CRT_I, i);
    outb(__svgalib_CRT_D, d);
}

int __svgalib_vga_inseq(int index)
{
    outb(SEQ_I, index);
    return port_in(SEQ_D);
}

void __svgalib_vga_outseq(int index, int val)
{
    outb(SEQ_I, index);
    outb(SEQ_D, val);
}

int __svgalib_vga_ingra(int index)
{
    outb(GRA_I, index);
    return port_in(GRA_D);
}

void __svgalib_vga_outgra(int index, int val)
{
    outb(GRA_I, index);
    outb(GRA_D, val);
}

int __svgalib_vga_inis1(void)
{
   return port_in(__svgalib_IS1_R);
}

#ifdef NO_DELAY

int __svgalib_vga_inatt(int index)
{
    __svgalib_vga_inis1();
    outb(ATT_IW, index);
    return port_in(ATT_R);
}

void __svgalib_vga_outatt(int index, int val)
{
    __svgalib_vga_inis1();
    outb(ATT_IW, index);
    outb(ATT_IW, val);
}

void __svgalib_vga_attscreen(int i)
{
    __svgalib_vga_inis1();
    outb(ATT_IW, i);
}

void __svgalib_vga_inpal(int i, int *r, int *g, int *b)
{
    outb(PEL_IR,i);
    *r=port_in(PEL_D);
    *g=port_in(PEL_D);
    *b=port_in(PEL_D);
}

void __svgalib_vga_outpal(int i, int r, int g, int b)
{

    outb(PEL_IW,i);
    outb(PEL_D,r);
    outb(PEL_D,g);
    outb(PEL_D,b);
}

#else /* NO_DELAY */

int __svgalib_vga_inatt(int index)
{
    __svgalib_delay();
    __svgalib_vga_inis1();
    __svgalib_delay();
    outb(ATT_IW, index);
    __svgalib_delay();
    return port_in(ATT_R);
}

void __svgalib_vga_outatt(int index, int val)
{
    __svgalib_delay();
    __svgalib_vga_inis1();
    __svgalib_delay();
    outb(ATT_IW, index);
    __svgalib_delay();
    outb(ATT_IW, val);
}

void __svgalib_vga_attscreen(int i)
{
    __svgalib_delay();
    __svgalib_vga_inis1();
    __svgalib_delay();
    outb(ATT_IW, i);
}

void __svgalib_vga_inpal(int i, int *r, int *g, int *b)
{
    outb(PEL_IR,i);
    __svgalib_delay();
    *r=port_in(PEL_D);
    __svgalib_delay();
    *g=port_in(PEL_D);
    __svgalib_delay();
    *b=port_in(PEL_D);
}

void __svgalib_vga_outpal(int i, int r, int g, int b)
{

    outb(PEL_IW,i);
    __svgalib_delay();
    outb(PEL_D,r);
    __svgalib_delay();
    outb(PEL_D,g);
    __svgalib_delay();
    outb(PEL_D,b);
}

#endif /* NO_DELAY */
