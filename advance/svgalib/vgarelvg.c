#include "libvga.h"

int __svgalib_io_reloc;

int __svgalib_rel_inmisc(void)
{
   return port_in(MIS_R+__svgalib_io_reloc);
}

void __svgalib_rel_outmisc(int i)
{
   port_out_r(MIS_W+__svgalib_io_reloc,i);
}

int __svgalib_rel_incrtc(int i)
{
   port_out_r(__svgalib_CRT_I+__svgalib_io_reloc,i);
   return port_in(__svgalib_CRT_D+__svgalib_io_reloc);
}

void __svgalib_rel_outcrtc(int i, int d)
{
    port_out_r(__svgalib_CRT_I+__svgalib_io_reloc, i);
    port_out_r(__svgalib_CRT_D+__svgalib_io_reloc, d);
}

int __svgalib_rel_inseq(int index)
{
    port_out_r(SEQ_I+__svgalib_io_reloc, index);
    return port_in(SEQ_D+__svgalib_io_reloc);
}

void __svgalib_rel_outseq(int index, int val)
{
    port_out_r(SEQ_I+__svgalib_io_reloc, index);
    port_out_r(SEQ_D+__svgalib_io_reloc, val);
}

int __svgalib_rel_ingra(int index)
{
    port_out_r(GRA_I+__svgalib_io_reloc, index);
    return port_in(GRA_D+__svgalib_io_reloc);
}

void __svgalib_rel_outgra(int index, int val)
{
    port_out_r(GRA_I+__svgalib_io_reloc, index);
    port_out_r(GRA_D+__svgalib_io_reloc, val);
}

int __svgalib_rel_inis1(void)
{
   return port_in(__svgalib_IS1_R+__svgalib_io_reloc);
}

#ifdef NO_DELAY

int __svgalib_rel_inatt(int index)
{
    __svgalib_rel_inis1();
    port_out_r(ATT_IW+__svgalib_io_reloc, index);
    return port_in(ATT_R+__svgalib_io_reloc);
}

void __svgalib_rel_outatt(int index, int val)
{
    __svgalib_rel_inis1();
    port_out_r(ATT_IW+__svgalib_io_reloc, index);
    port_out_r(ATT_IW+__svgalib_io_reloc, val);
}

void __svgalib_rel_attscreen(int i)
{
    __svgalib_rel_inis1();
    port_out_r(ATT_IW+__svgalib_io_reloc, i);
}

void __svgalib_rel_inpal(int i, int *r, int *g, int *b)
{
    port_out_r(PEL_IR+__svgalib_io_reloc,i);
    *r=port_in(PEL_D+__svgalib_io_reloc);
    *g=port_in(PEL_D+__svgalib_io_reloc);
    *b=port_in(PEL_D+__svgalib_io_reloc);
}

void __svgalib_rel_outpal(int i, int r, int g, int b)
{

    port_out_r(PEL_IW+__svgalib_io_reloc,i);
    port_out_r(PEL_D+__svgalib_io_reloc,r);
    port_out_r(PEL_D+__svgalib_io_reloc,g);
    port_out_r(PEL_D+__svgalib_io_reloc,b);
}

#else /* NO_DELAY */

int __svgalib_rel_inatt(int index)
{
    __svgalib_delay();
    __svgalib_rel_inis1();
    __svgalib_delay();
    port_out_r(ATT_IW+__svgalib_io_reloc, index);
    __svgalib_delay();
    return port_in(ATT_R+__svgalib_io_reloc);
}

void __svgalib_rel_outatt(int index, int val)
{
    __svgalib_delay();
    __svgalib_rel_inis1();
    __svgalib_delay();
    port_out_r(ATT_IW+__svgalib_io_reloc, index);
    __svgalib_delay();
    port_out_r(ATT_IW+__svgalib_io_reloc, val);
}

void __svgalib_rel_attscreen(int i)
{
    __svgalib_delay();
    __svgalib_rel_inis1();
    __svgalib_delay();
    port_out_r(ATT_IW+__svgalib_io_reloc, i);
}

void __svgalib_rel_inpal(int i, int *r, int *g, int *b)
{
    port_out_r(PEL_IR+__svgalib_io_reloc,i);
    __svgalib_delay();
    *r=port_in(PEL_D+__svgalib_io_reloc);
    __svgalib_delay();
    *g=port_in(PEL_D+__svgalib_io_reloc);
    __svgalib_delay();
    *b=port_in(PEL_D+__svgalib_io_reloc);
}

void __svgalib_rel_outpal(int i, int r, int g, int b)
{

    port_out_r(PEL_D+__svgalib_io_reloc,i);
    __svgalib_delay();
    port_out_r(PEL_D+__svgalib_io_reloc,r);
    __svgalib_delay();
    port_out_r(PEL_D+__svgalib_io_reloc,g);
    __svgalib_delay();
    port_out_r(PEL_D+__svgalib_io_reloc,b);
}

#endif /* NO_DELAY */

void __svgalib_rel_io_mapio(void)
{
    __svgalib_inmisc=__svgalib_rel_inmisc;
    __svgalib_outmisc=__svgalib_rel_outmisc;
    __svgalib_incrtc=__svgalib_rel_incrtc;
    __svgalib_outcrtc=__svgalib_rel_outcrtc;
    __svgalib_inseq=__svgalib_rel_inseq;
    __svgalib_outseq=__svgalib_rel_outseq;
    __svgalib_ingra=__svgalib_rel_ingra;
    __svgalib_outgra=__svgalib_rel_outgra;
    __svgalib_inatt=__svgalib_rel_inatt;
    __svgalib_outatt=__svgalib_rel_outatt;
    __svgalib_attscreen=__svgalib_rel_attscreen;
    __svgalib_inis1=__svgalib_rel_inis1;
    __svgalib_inpal=__svgalib_rel_inpal;
    __svgalib_outpal=__svgalib_rel_outpal;
}
