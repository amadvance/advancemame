static int __svgalib_rendition_inmisc(void)
{
   return 0;
}

static void __svgalib_rendition_outmisc(int i)
{
}

static int __svgalib_rendition_incrtc(int i)
{
   return 0;
}

static void __svgalib_rendition_outcrtc(int i, int d)
{
}

static int __svgalib_rendition_inseq(int index)
{
    return 0;
}

static void __svgalib_rendition_outseq(int index, int val)
{
}

static int __svgalib_rendition_ingra(int index)
{
    return 0;
}

static void __svgalib_rendition_outgra(int index, int val)
{
}

static int __svgalib_rendition_inis1(void)
{
   return 0;
}

static int __svgalib_rendition_inatt(int index)
{
    return 0;
}

static void __svgalib_rendition_outatt(int index, int val)
{
}

static void __svgalib_rendition_attscreen(int i)
{
}

static void __svgalib_rendition_inpal(int i, int *r, int *g, int *b)
{
    OUT(DACRAMREADADR, i );
    *r=IN(DACRAMDATA);
    *g=IN(DACRAMDATA);
    *b=IN(DACRAMDATA);
}

static void __svgalib_rendition_outpal(int i, int r, int g, int b)
{
    OUT(DACRAMWRITEADR, i );
    OUT(DACRAMDATA, r );
    OUT(DACRAMDATA, g );
    OUT(DACRAMDATA, b );
}

static void rendition_mapio(void)
{
    if(__svgalib_secondary) {
        __svgalib_inmisc=__svgalib_rendition_inmisc;
        __svgalib_outmisc=__svgalib_rendition_outmisc;
        __svgalib_incrtc=__svgalib_rendition_incrtc;
        __svgalib_outcrtc=__svgalib_rendition_outcrtc;
        __svgalib_inseq=__svgalib_rendition_inseq;
        __svgalib_outseq=__svgalib_rendition_outseq;
        __svgalib_ingra=__svgalib_rendition_ingra;
        __svgalib_outgra=__svgalib_rendition_outgra;
        __svgalib_inatt=__svgalib_rendition_inatt;
        __svgalib_outatt=__svgalib_rendition_outatt;
        __svgalib_attscreen=__svgalib_rendition_attscreen;
        __svgalib_inis1=__svgalib_rendition_inis1;
    }
    __svgalib_inpal=__svgalib_rendition_inpal;
    __svgalib_outpal=__svgalib_rendition_outpal;
}

