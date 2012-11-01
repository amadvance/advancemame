/*
Skeleton chipset driver 
*/

#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "v2kregs.h"

static int io_base;

#include "renditio.h"

typedef struct {
    uint8_t  memendian;
    uint8_t  mode;
    uint8_t  bankselect;
    uint32_t sclkpll;
    uint32_t dramctl;
    uint32_t pclkpll;
    uint32_t crtchorz;
    uint32_t crtcvert;
    uint32_t crtcctl;
    uint32_t framebasea;
    uint32_t crtcoffset;
    uint8_t  dac[17];
} hwregs, *hwregsptr;

#define TOTAL_REGS (VGA_TOTAL_REGS + sizeof(hwregs))

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory=0, id;
static int is_linear, linear_base;

static CardSpecs *cardspecs;

static void setpage(int page)
{
    OUT(BANKSELECT+2, page);
}

static int __svgalib_inlinearmode(void)
{
return is_linear;
}

/* Fill in chipset specific mode information */

static void getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int saveregs(unsigned char regs[])
{ 
    int i;
    hwregsptr r;
    
    r=(hwregsptr)(regs+VGA_TOTAL_REGS);

    r->memendian= IN(MEMENDIAN);
    r->mode	= IN(MODEREG);
    r->bankselect=IN(BANKSELECT+2);
    r->sclkpll	= IN32(SCLKPLL);
    r->dramctl	= IN32(DRAMCTL);
    r->pclkpll	= IN32(PCLKPLL);
    r->crtchorz	= IN32(CRTCHORZ);
    r->crtcvert	= IN32(CRTCVERT);
    r->crtcctl	= IN32(CRTCCTL);
    r->framebasea=IN32(FRAMEBASEA);
    r->crtcoffset=IN32(CRTCOFFSET);

    for(i=0;i<16;i++) r->dac[i]=IN(PALETTE+i);
    OUT(DACRAMWRITEADR, 1);
    r->dac[16]=IN(DACSTATUS);
    OUT(DACRAMWRITEADR, r->dac[0]);

    return TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    hwregsptr r;
    
    r=(hwregsptr)(regs+VGA_TOTAL_REGS);
    unlock();
    
    OUT(MODEREG, r->mode);
    OUT(MEMENDIAN, r->memendian);
    OUT(BANKSELECT+2, r->bankselect);
    OUT32(SCLKPLL, r->sclkpll);
    usleep(500);
    OUT32(DRAMCTL, r->dramctl);
    OUT32(PCLKPLL, r->pclkpll);
    usleep(500);
    
    OUT(DACCOMMAND0, r->dac[DACCOMMAND0-PALETTE]);
    OUT(DACCOMMAND1, r->dac[DACCOMMAND1-PALETTE]);
    OUT(DACCOMMAND2, r->dac[DACCOMMAND2-PALETTE]);
    OUT(DACRAMWRITEADR, 1);
    OUT(DACSTATUS, r->dac[16]);
    OUT(DACPIXELMSK, r->dac[2]);
    
    OUT32(CRTCHORZ, r->crtchorz);
    OUT32(CRTCVERT, r->crtcvert);
    OUT32(FRAMEBASEA, r->framebasea);
    OUT32(CRTCOFFSET, r->crtcoffset);
    OUT32(CRTCCTL, r->crtcctl);
}


/* Return nonzero if mode is available */

static int modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    if(modeinfo->bitsPerPixel==24) {
        free(modeinfo);
        return 0;
    }

    modetiming = malloc(sizeof(ModeTiming));

    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 0;
    }

    free(modetiming);
    free(modeinfo);
    return SVGADRV;
}

#define V2_MIN_VCO_FREQ  125
#define V2_MAX_VCO_FREQ  250
#define V2_REF_FREQ      14.31818 /* Eh, is this right? */
#define V2_MIN_PCF_FREQ  1
#define V2_MAX_PCF_FREQ  3

static double V2200CalcClock(double target, int *m, int *n, int *p)
{
    double mindiff = 1e10;
    double vco, pcf, diff, freq;
    int mm, nn, pp;

    for (pp=1; pp<=0x0f; pp++)
        for (nn=1; nn<=0x3f; nn++)
            for (mm=1; mm<=0xff; mm++) {
                vco = V2_REF_FREQ*mm/nn;
                if ((vco < V2_MIN_VCO_FREQ) || (vco > V2_MAX_VCO_FREQ))
                    continue;
                pcf = V2_REF_FREQ/nn;
                if ((pcf < V2_MIN_PCF_FREQ) || (pcf > V2_MAX_PCF_FREQ))
                    continue;
                freq = vco/pp;
                diff = target-freq;
                if(diff<0) diff=-diff;
                if (diff < mindiff) {
                    *m = mm; *n = nn; *p = pp;
                    mindiff = diff;
                }
            }

    vco = V2_REF_FREQ * *m / *n;
    pcf = V2_REF_FREQ / *n;
    freq = vco / *p;

    return freq;
}

#define combineNMP(N, M, P) \
    (((uint32_t)(M-2)<<10) | ((uint32_t)P<<8) | (uint32_t)(N-2))

#define v2kcombineNMP(N, M, P) (((uint32_t)N<<13) | ((uint32_t)P<<9) | (uint32_t)M)

#define CTL(ldbl, hsynchi, vsynchi) \
  (((ldbl) ? CRTCCTL_LINEDOUBLE : 0) \
  |((hsynchi) ? CRTCCTL_HSYNCHI : 0) \
  |((vsynchi) ? CRTCCTL_VSYNCHI : 0) \
  |(CRTCCTL_VSYNCENABLE | CRTCCTL_HSYNCENABLE))

#define HORZ(fp, sy, bp, ac) \
(((((uint32_t)(((fp)>>3)-1))&7)<<21)|((((uint32_t)(((sy)>>3)-1))&0x1F)<<16)|\
((((uint32_t)(((bp)>>3)-1))&0x3f)<<9)|((((uint32_t)(((ac)>>3)-1))&0xff)))

#define VERT(fp, sy, bp, ac) \
(((((uint32_t)(fp-1))&0x3f)<<20)|((((uint32_t)(sy-1))&0x7)<<17)|\
((((uint32_t)(bp-1))&0x3f)<<11)|((((uint32_t)(ac-1))&0x7ff)))

#define HORZAC(crtchorz) \
  (((((uint32_t)crtchorz)&CRTCHORZ_ACTIVE_MASK)+1)<<3)

#define HORZBP(crtchorz) \
  ((((((uint32_t)crtchorz)&CRTCHORZ_BACKPORCH_MASK)>>9)+1)<<3)

#define HORZSY(crtchorz) \
  ((((((uint32_t)crtchorz)&CRTCHORZ_SYNC_MASK)>>16)+1)<<3)

#define HORZFP(crtchorz) \
  ((((((uint32_t)crtchorz)&CRTCHORZ_FRONTPORCH_MASK)>>21)+1)<<3)

#define VERTAC(crtcvert) \
  ((((uint32_t)crtcvert)&CRTCVERT_ACTIVE_MASK)+1)

#define VERTBP(crtcvert) \
  (((((uint32_t)crtcvert)&CRTCVERT_BACKPORCH_MASK)>>11)+1)

#define VERTSY(crtcvert) \
  (((((uint32_t)crtcvert)&CRTCVERT_SYNC_MASK)>>17)+1)

#define VERTFP(crtcvert) \
  (((((uint32_t)crtcvert)&CRTCVERT_FRONTPORCH_MASK)>>20)+1)


static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
    int m,n,p, pixelformat = 0, offset, fifosize;
    hwregsptr r;
    
    r=(hwregsptr)(moderegs+VGA_TOTAL_REGS);
    
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    r->mode = NATIVE_MODE | VESA_MODE;

    switch(modeinfo->bitsPerPixel) {
        case 8:
            r->memendian= MEMENDIAN_END;
	    r->dac[6]= BT485_CR0_EXTENDED_REG_ACCESS;
	    r->dac[8]= BT485_CR1_8BPP | BT485_CR1_PIXEL_PORT_AB;
	    r->dac[9]= BT485_PIXEL_INPUT_GATE | BT485_DISABLE_CURSOR;
            pixelformat = V_PIXFMT_8I;
            break;
        case 16:
            r->memendian= MEMENDIAN_HW;
	    r->dac[6] =  BT485_CR0_EXTENDED_REG_ACCESS | BT485_CR0_8_BIT_DAC;
	    r->dac[8] =  BT485_CR1_16BPP |
                         BT485_CR1_1_TO_1_16BPP | BT485_CR1_PIXEL_PORT_AB;
	    r->dac[9] =  BT485_PIXEL_INPUT_GATE | BT485_DISABLE_CURSOR;
            if(modeinfo->greenWeight == 6) {
                r->dac[8] |= BT485_CR1_BYPASS_CLUT | BT485_CR1_565_16BPP;
                pixelformat = V_PIXFMT_565;
            } else {
                int i;
                pixelformat = V_PIXFMT_1555;        
                for(i=0; i<256; i++)vga_setpalette(i, i>>2, i>>1, i>>2);
            }
            break;
        case 32:
            r->memendian= MEMENDIAN_NO;
	    r->dac[6] =  BT485_CR0_EXTENDED_REG_ACCESS | BT485_CR0_8_BIT_DAC;
	    r->dac[8] =  BT485_CR1_24BPP | BT485_CR1_BYPASS_CLUT |
                         BT485_CR1_PIXEL_PORT_AB;
	    r->dac[9] =  BT485_PIXEL_INPUT_GATE | BT485_DISABLE_CURSOR;
            pixelformat = V_PIXFMT_8888;
            break;
    }
    
    r->dac[16]=0;
    r->dac[2]=0xff;
    
    r->bankselect=0;

    r->sclkpll	= IN32(SCLKPLL);
    r->dramctl	= (IN32(DRAMCTL) & 0xc7ff) | 0x330000;

    V2200CalcClock(modetiming->pixelClock/1000.0, &m, &n, &p);

    r->pclkpll	= v2kcombineNMP(n, m, p);
    
    r->crtchorz	= HORZ(modetiming->HSyncStart-modetiming->HDisplay,
                               modetiming->HSyncEnd-modetiming->HSyncStart,
                               modetiming->HTotal-modetiming->HSyncEnd,
                               modetiming->HDisplay);

    r->crtcvert	= VERT(modetiming->VSyncStart-modetiming->VDisplay,
                               modetiming->VSyncEnd-modetiming->VSyncStart,
                               modetiming->VTotal-modetiming->VSyncEnd,
                               modetiming->VDisplay);
    r->crtcctl	= CTL(modetiming->flags&DOUBLESCAN, 0, 0)
                        | pixelformat
                        | CRTCCTL_VIDEOFIFOSIZE128
                        | CRTCCTL_HSYNCENABLE
                        | CRTCCTL_VSYNCENABLE
                        | CRTCCTL_VIDEOENABLE;
    
    r->framebasea=0;

    fifosize=128;
    offset=modeinfo->lineWidth%fifosize;
    if((modeinfo->lineWidth%fifosize)==0) offset += fifosize; 
    r->crtcoffset = offset;

    return ;
}


static int local_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(TOTAL_REGS);

    initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
}

static void lock(void)
{
}

#define VENDOR_ID 0x1163
#define CARD_ID 0x2000

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    unsigned long buf[64];
    int found;
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    id=(buf[0]>>16)&0xffff;
    if(found)return 0;
    switch(id) {
        case CARD_ID:
            init(0,0,0);
            return 1;
            break;
        default:
            return 0;
    }
}


/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void setdisplaystart(int address)
{ 
    OUT(FRAMEBASEA, address);
}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void setlogicalwidth(int width)
{   
    int sw, fifosize, offset;
    
    fifosize=128;
    sw=CI.bytesperpixel*CI.xdim;
    
    offset=width - sw + (sw%fifosize);
    
    if((sw%fifosize)==0) offset += fifosize;
     
    OUT(CRTCOFFSET, offset);
}

static int linear(int op, int param)
{
    if (op==LINEAR_ENABLE){is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int match_programmable_clock(int clock)
{
return clock ;
}

static int map_clock(int bpp, int clock)
{
return clock ;
}

static int map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

/* Function table (exported) */

DriverSpecs __svgalib_rendition_driverspecs =
{
    saveregs,
    setregs,
    unlock,
    lock,
    test,
    init,
    setpage,
    NULL,
    NULL,
    local_setmode,
    modeavailable,
    setdisplaystart,
    setlogicalwidth,
    getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
};

/* Initialize chipset (called after detection) */

static int init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0;
    unsigned int mmio_base;

    unlock();
    if (force) {
	memory = par1;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    linear_base=0;
    id=(buf[0]>>16)&0xffff;
    if(!found) {
        switch(id) {
            case CARD_ID:
                break;
            default:
                return 1;
        }
    }
    
    linear_base=buf[4]&0xffff0000;
    io_base=buf[5]&0xff00;
    mmio_base=buf[6]&0xffff0000;
    
    rendition_mapio();

    if(!memory) {
        unsigned char *m;

        m=mmap(0,4*1024*1024,PROT_READ|PROT_WRITE,MAP_SHARED,__svgalib_mem_fd,linear_base);
        memory=memorytest(m, 4);
        munmap(m, 4*1024*1024);
    }    

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Rendition driver, %iKB.\n",memory);
    };
    __svgalib_modeinfo_linearset |= IS_LINEAR;
	
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock4bpp = 0;	
    cardspecs->maxPixelClock8bpp = 170000;	
    cardspecs->maxPixelClock16bpp = 170000;	
    cardspecs->maxPixelClock24bpp = 0;
    cardspecs->maxPixelClock32bpp = 170000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->nClocks =0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_rendition_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=memory*0x400;
    __svgalib_mmio_base = mmio_base;
    __svgalib_mmio_size = 256*1024;
    return 0;
}
