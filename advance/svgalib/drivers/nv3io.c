static int __svgalib_nv3_inmisc(void)
{
   return *((unsigned char *)nvPVGA0Port+MIS_R);
}

static void __svgalib_nv3_outmisc(int i)
{
   *((unsigned char *)nvPVGA0Port+MIS_W) = i;
}

static int __svgalib_nv3_incrtc(int i)
{
   *((unsigned char *)nvPVGA1Port+__svgalib_CRT_I)=i;
   return *((unsigned char *)nvPVGA1Port+__svgalib_CRT_D);
}

static void __svgalib_nv3_outcrtc(int i, int d)
{
   *((unsigned char *)nvPVGA1Port+__svgalib_CRT_I)=i;
   *((unsigned char *)nvPVGA1Port+__svgalib_CRT_D)=d;
}

static int __svgalib_nv3_inseq(int index)
{
   *((unsigned char *)nvPVGA0Port+SEQ_I)=index;
   return *((unsigned char *)nvPVGA0Port+SEQ_D);
}

static void __svgalib_nv3_outseq(int index, int val)
{
   *((unsigned char *)nvPVGA0Port+SEQ_I)=index;
   *((unsigned char *)nvPVGA0Port+SEQ_D)=val;
}

static int __svgalib_nv3_ingra(int index)
{
   *((unsigned char *)nvPVGA0Port+GRA_I)=index;
   return *((unsigned char *)nvPVGA0Port+GRA_D);
}

static void __svgalib_nv3_outgra(int index, int val)
{
   *((unsigned char *)nvPVGA0Port+GRA_I)=index;
   *((unsigned char *)nvPVGA0Port+GRA_D)=val;
}

static int __svgalib_nv3_inis1(void)
{
    return *((unsigned char *)nvPVGA1Port+__svgalib_IS1_R);
}

static int __svgalib_nv3_inatt(int index)
{
    __svgalib_nv3_inis1();
    *((unsigned char *)nvPVGA1Port+ATT_IW)=index;
    return *((unsigned char *)nvPVGA1Port+ATT_R);
}

static void __svgalib_nv3_outatt(int index, int val)
{
    __svgalib_nv3_inis1();
    *((unsigned char *)nvPVGA1Port+ATT_IW)=index;
    *((unsigned char *)nvPVGA1Port+ATT_IW)=val;
}

static void __svgalib_nv3_attscreen(int i)
{
    __svgalib_nv3_inis1();
    *((unsigned char *)nvPVGA1Port+ATT_IW)=i;
}

static void __svgalib_nv3_inpal(int i, int *r, int *g, int *b)
{
    *((unsigned char *)nvPVGA2Port+PEL_IR)=i;
    *r=*((unsigned char *)nvPVGA2Port+PEL_D);
    *g=*((unsigned char *)nvPVGA2Port+PEL_D);
    *b=*((unsigned char *)nvPVGA2Port+PEL_D);
}

static void __svgalib_nv3_outpal(int i, int r, int g, int b)
{
    *((unsigned char *)nvPVGA2Port+PEL_IW)=i;
    *((unsigned char *)nvPVGA2Port+PEL_D)=r;
    *((unsigned char *)nvPVGA2Port+PEL_D)=g;
    *((unsigned char *)nvPVGA2Port+PEL_D)=b;
}

static void nv3_mapio(void)
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

