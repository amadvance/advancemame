static inline unsigned char laguna_readb(int a) {
    return *(MMIO_POINTER+a);
}

static inline unsigned short laguna_readw(int a) {
    return *(unsigned short *)(MMIO_POINTER+a);
}

static inline unsigned int laguna_readl(int a) {
    return *(unsigned int *)(MMIO_POINTER+a);
}

static inline void laguna_writeb(int a, unsigned char d) {
    *(MMIO_POINTER+a)=d;
}

static inline void laguna_writew(int a, unsigned short d) {
    *(unsigned short *)(MMIO_POINTER+a)=d;
}

static inline void laguna_writel(int a, unsigned int d) {
    *(unsigned int *)(MMIO_POINTER+a)=d;
}

static int __svgalib_laguna_inmisc(void)
{
    return laguna_readb(0x80);
}

static void __svgalib_laguna_outmisc(int i)
{
    laguna_writeb(0x80,i);
}

static int __svgalib_laguna_incrtc(int i)
{
    if(i>0x1e)return 0;
    return laguna_readb(i*4);
}

static void __svgalib_laguna_outcrtc(int i, int d)
{
    if(i>0x1e)return;
    laguna_writeb(i*4,d);
}

static int __svgalib_laguna_inseq(int index)
{
    switch(index) {
        case 0x0e:
            return laguna_readb(0x84);
            break;
        case 0x18:
            return laguna_readb(0x90);
            break;
        case 0x19:
            return laguna_readb(0x94);
            break;
        case 0x1a:
            return laguna_readb(0x98);
            break;
        case 0x1e:
            return laguna_readb(0x88);
            break;
        default:
            return 0;
    }
}

static void __svgalib_laguna_outseq(int index, int val)
{
    switch(index) {
        case 0x0e:
            laguna_writeb(0x84,val);
            break;
        case 0x18:
            laguna_writeb(0x90,val);
            break;
        case 0x19:
            laguna_writeb(0x94,val);
            break;
        case 0x1a:
            laguna_writeb(0x98,val);
            break;
        case 0x1e:
            laguna_writeb(0x88,val);
            break;
    }
}

static int __svgalib_laguna_ingra(int index)
{
    return 0;
}

static void __svgalib_laguna_outgra(int index, int val)
{
}

static int __svgalib_laguna_inis1(void)
{
    return 0;
}

static int __svgalib_laguna_inatt(int index)
{
    return 0;
}

static void __svgalib_laguna_outatt(int index, int val)
{
}

static void __svgalib_laguna_attscreen(int i)
{
    laguna_writeb(0x7c,i);
}

static void __svgalib_laguna_inpal(int i, int *r, int *g, int *b)
{
    laguna_writeb(0xa4,i);
    *r=laguna_readb(0xac);
    *g=laguna_readb(0xac);
    *b=laguna_readb(0xac);
}

static void __svgalib_laguna_outpal(int i, int r, int g, int b)
{
    laguna_writeb(0xa8,i);
    laguna_writeb(0xac,r);
    laguna_writeb(0xac,g);
    laguna_writeb(0xac,b);
}

static void laguna_mapio(void)
{
    __svgalib_inmisc=__svgalib_laguna_inmisc;
    __svgalib_outmisc=__svgalib_laguna_outmisc;
    __svgalib_incrtc=__svgalib_laguna_incrtc;
    __svgalib_outcrtc=__svgalib_laguna_outcrtc;
    __svgalib_inseq=__svgalib_laguna_inseq;
    __svgalib_outseq=__svgalib_laguna_outseq;
    __svgalib_ingra=__svgalib_laguna_ingra;
    __svgalib_outgra=__svgalib_laguna_outgra;
    __svgalib_inatt=__svgalib_laguna_inatt;
    __svgalib_outatt=__svgalib_laguna_outatt;
    __svgalib_attscreen=__svgalib_laguna_attscreen;
    __svgalib_inis1=__svgalib_laguna_inis1;
    __svgalib_inpal=__svgalib_laguna_inpal;
    __svgalib_outpal=__svgalib_laguna_outpal;
}

