static int __svgalib_nv3_inmisc(void)
{
   return v_readb(NV_PVGA0+MIS_R);
}

static void __svgalib_nv3_outmisc(int i)
{
   v_writeb(i, NV_PVGA0+MIS_W);
}

static int __svgalib_nv3_incrtc(int i)
{
   v_writeb(i, NV_PVGA1+__svgalib_CRT_I);
   return v_readb(NV_PVGA1+__svgalib_CRT_D);
}

static void __svgalib_nv3_outcrtc(int i, int d)
{
   v_writeb(i, NV_PVGA1+__svgalib_CRT_I);
   v_writeb(d, NV_PVGA1+__svgalib_CRT_D);
}

static int __svgalib_nv3_inseq(int index)
{
   v_writeb(index, NV_PVGA0+SEQ_I);
   return v_readb(NV_PVGA0+SEQ_D);
}

static void __svgalib_nv3_outseq(int index, int val)
{
   v_writeb(index, NV_PVGA0+SEQ_I);
   v_writeb(val, NV_PVGA0+SEQ_D);
}

static int __svgalib_nv3_ingra(int index)
{
   v_writeb(index, NV_PVGA0+GRA_I);
   return v_readb(NV_PVGA0+GRA_D);
}

static void __svgalib_nv3_outgra(int index, int val)
{
   v_writeb(index, NV_PVGA0+GRA_I);
   v_writeb(val, NV_PVGA0+GRA_D);
}

static int __svgalib_nv3_inis1(void)
{
    return v_readb(NV_PVGA1+__svgalib_IS1_R);
}

static int __svgalib_nv3_inatt(int index)
{
    __svgalib_nv3_inis1();
    v_writeb(index, NV_PVGA1+ATT_IW);
    return v_readb(NV_PVGA1+ATT_R);
}

static void __svgalib_nv3_outatt(int index, int val)
{
    __svgalib_nv3_inis1();
    v_writeb(index, NV_PVGA1+ATT_IW);
    v_writeb(val, NV_PVGA1+ATT_IW);
}

static void __svgalib_nv3_attscreen(int i)
{
    __svgalib_nv3_inis1();
    v_writeb(i, NV_PVGA1+ATT_IW);
}

static void __svgalib_nv3_inpal(int i, int *r, int *g, int *b)
{
    v_writeb(i, NV_PVGA2+PEL_IR);
    *r=v_readb(NV_PVGA2+PEL_D);
    *g=v_readb(NV_PVGA2+PEL_D);
    *b=v_readb(NV_PVGA2+PEL_D);
}

static void __svgalib_nv3_outpal(int i, int r, int g, int b)
{
    v_writeb(i, NV_PVGA2+PEL_IW);
    v_writeb(r, NV_PVGA2+PEL_D);
    v_writeb(g, NV_PVGA2+PEL_D);
    v_writeb(b, NV_PVGA2+PEL_D);
}

static void mapio(void)
{
    __svgalib_inmisc=__svgalib_nv3_inmisc;
    __svgalib_outmisc=__svgalib_nv3_outmisc;
    __svgalib_incrtc=__svgalib_nv3_incrtc;
    __svgalib_outcrtc=__svgalib_nv3_outcrtc;
    __svgalib_inseq=__svgalib_nv3_inseq;
    __svgalib_outseq=__svgalib_nv3_outseq;
    __svgalib_ingra=__svgalib_nv3_ingra;
    __svgalib_outgra=__svgalib_nv3_outgra;
    __svgalib_inatt=__svgalib_nv3_inatt;
    __svgalib_outatt=__svgalib_nv3_outatt;
    __svgalib_attscreen=__svgalib_nv3_attscreen;
    __svgalib_inis1=__svgalib_nv3_inis1;
    __svgalib_inpal=__svgalib_nv3_inpal;
    __svgalib_outpal=__svgalib_nv3_outpal;
}

