/*
 i740 chipset driver
  written by Matan Ziv-Av.
  Only tested on a Hercules Terminator 2x/I
  Interlaced modes don't work yet.
*/
#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "i740_reg.h"
#include "vgammvga.h"

typedef struct {
    unsigned char DisplayControl;
    unsigned char PixelPipeCfg0;
    unsigned char PixelPipeCfg1;
    unsigned char PixelPipeCfg2;
    unsigned char VideoClk2_M;
    unsigned char VideoClk2_N;
    unsigned char VideoClk2_MN_MSBs;
    unsigned char VideoClk2_DivisorSel;
    unsigned char PLLControl;
    unsigned char AddressMapping;
    unsigned char IOControl;
    unsigned char BitBLTControl;
    unsigned char ExtVertTotal;
    unsigned char ExtVertDispEnd;
    unsigned char ExtVertSyncStart;
    unsigned char ExtVertBlankStart;
    unsigned char ExtHorizTotal;
    unsigned char ExtHorizBlank;
    unsigned char ExtOffset;
    unsigned char InterlaceControl;
    unsigned char ExtStartAddr;
    unsigned char ExtStartAddrHi;    
    unsigned char CursorControl;
    unsigned char CursorBaseAddrLo;
    unsigned char CursorBaseAddrHi;
    unsigned char CursorXLo;
    unsigned char CursorXHi;
    unsigned char CursorYLo;
    unsigned char CursorYHi;
    unsigned int  LMI_FIFO_Watermark; 
} vgaI740Rec, *vgaI740Ptr;

#define I740REG_SAVE(i) (VGA_TOTAL_REGS+i)
#define I740_TOTAL_REGS (VGA_TOTAL_REGS + sizeof(vgaI740Rec))

static int i740_init(int, int, int);
static void i740_unlock(void);
static void i740_lock(void);

static int i740_memory, I740HasSGRAM;
static int i740_is_linear, i740_linear_base, i740_mmio_base;

static CardSpecs *cardspecs;

static void i740_outxr_io(int index, int val)
{
    port_out(index, XRX);
    port_out(val, XRX+1);
}

static int i740_inxr_io(int index)
{
    port_out(index, XRX);
    return port_in(XRX+1);
}

static void i740_outxr_mm(int index, int val)
{
    *(__svgalib_vgammbase+XRX)=index;
    *(__svgalib_vgammbase+XRX+1)=val;
}

static int i740_inxr_mm(int index)
{
    *(__svgalib_vgammbase+XRX)=index;
    return *(__svgalib_vgammbase+XRX+1);
}

static void (*i740_outxr)(int,int)=i740_outxr_io;
static int (*i740_inxr)(int)=i740_inxr_io;

static void i740_setpage(int page)
{
   i740_outxr(14,page);
}

static int __svgalib_i740_inlinearmode(void)
{
return i740_is_linear;
}

/* Fill in chipset specific mode information */

static void i740_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = i740_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 8184;
    modeinfo->startaddressrange = i740_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(i740_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_i740_inlinearmode())
	    modeinfo->flags |= IS_LINEAR;
    }
}

/* Read and save chipset-specific registers */

static int i740_saveregs(unsigned char regs[])
{ 
    vgaI740Ptr save;

    i740_unlock();		
    save = (vgaI740Ptr)(regs+VGA_TOTAL_REGS);

    save->IOControl=i740_inxr(IO_CTNL);
    save->AddressMapping=i740_inxr(ADDRESS_MAPPING);
    save->BitBLTControl=i740_inxr(BITBLT_CNTL);
    save->VideoClk2_M=i740_inxr(VCLK2_VCO_M);
    save->VideoClk2_N=i740_inxr(VCLK2_VCO_N);
    save->VideoClk2_MN_MSBs=i740_inxr(VCLK2_VCO_MN_MSBS);
    save->VideoClk2_DivisorSel=i740_inxr(VCLK2_VCO_DIV_SEL);
    save->PLLControl=i740_inxr(PLL_CNTL);

    save->ExtVertTotal = __svgalib_incrtc(EXT_VERT_TOTAL);
    save->ExtVertDispEnd = __svgalib_incrtc(EXT_VERT_DISPLAY);
    save->ExtVertSyncStart = __svgalib_incrtc(EXT_VERT_SYNC_START);
    save->ExtVertBlankStart = __svgalib_incrtc(EXT_VERT_BLANK_START);
    save->ExtHorizTotal = __svgalib_incrtc(EXT_HORIZ_TOTAL);
    save->ExtHorizBlank = __svgalib_incrtc(EXT_HORIZ_BLANK);
    save->ExtOffset = __svgalib_incrtc(EXT_OFFSET);
    save->InterlaceControl = __svgalib_incrtc(INTERLACE_CNTL);
    save->ExtStartAddr = __svgalib_incrtc(EXT_START_ADDR);
    save->ExtStartAddrHi = __svgalib_incrtc(EXT_START_ADDR_HI);

    save->PixelPipeCfg0=i740_inxr(PIXPIPE_CONFIG_0);
    save->PixelPipeCfg1=i740_inxr(PIXPIPE_CONFIG_1);
    save->PixelPipeCfg2=i740_inxr(PIXPIPE_CONFIG_2);
    save->DisplayControl=i740_inxr(DISPLAY_CNTL);

    save->CursorControl=i740_inxr(CURSOR_CONTROL);
    save->CursorBaseAddrHi=i740_inxr(CURSOR_BASEADDR_HI);
    save->CursorBaseAddrLo=i740_inxr(CURSOR_BASEADDR_LO);
    save->CursorXHi=i740_inxr(CURSOR_X_HI);
    save->CursorXLo=i740_inxr(CURSOR_X_LO);
    save->CursorYHi=i740_inxr(CURSOR_Y_HI);
    save->CursorYLo=i740_inxr(CURSOR_Y_LO);

    save->LMI_FIFO_Watermark = INREG(FWATER_BLC);

    return I740_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void i740_setregs(const unsigned char regs[], int mode)
{  
    int temp;
    vgaI740Ptr restore;

    i740_unlock();		
    restore = (vgaI740Ptr)(regs+VGA_TOTAL_REGS);

    i740_outxr(DRAM_EXT_CNTL, DRAM_REFRESH_DISABLE);

    usleep(1000); /* Wait 1 ms */

    /* Write the M, N and P values */
    i740_outxr(VCLK2_VCO_M, restore->VideoClk2_M);
    i740_outxr(VCLK2_VCO_N, restore->VideoClk2_N);
    i740_outxr(VCLK2_VCO_MN_MSBS, restore->VideoClk2_MN_MSBs);
    i740_outxr(VCLK2_VCO_DIV_SEL, restore->VideoClk2_DivisorSel);

    temp = i740_inxr(PIXPIPE_CONFIG_0);
    temp &= ~(DAC_8_BIT | HW_CURSOR_ENABLE); 
    temp |= (restore->PixelPipeCfg0 & (DAC_8_BIT | HW_CURSOR_ENABLE));
    i740_outxr(PIXPIPE_CONFIG_0, temp);

    __svgalib_outcrtc(EXT_VERT_TOTAL, restore->ExtVertTotal);

    __svgalib_outcrtc(EXT_VERT_DISPLAY, restore->ExtVertDispEnd);

    __svgalib_outcrtc(EXT_VERT_SYNC_START, restore->ExtVertSyncStart);

    __svgalib_outcrtc(EXT_VERT_BLANK_START, restore->ExtVertBlankStart);

    __svgalib_outcrtc(EXT_HORIZ_TOTAL, restore->ExtHorizTotal);

    __svgalib_outcrtc(EXT_HORIZ_BLANK, restore->ExtHorizBlank);

    __svgalib_outcrtc(EXT_OFFSET, restore->ExtOffset);

    __svgalib_outcrtc(EXT_START_ADDR, restore->ExtStartAddr);
    __svgalib_outcrtc(EXT_START_ADDR_HI, restore->ExtStartAddrHi);

    i740_outxr(CURSOR_CONTROL,restore->CursorControl);
    i740_outxr(CURSOR_BASEADDR_HI,restore->CursorBaseAddrHi);
    i740_outxr(CURSOR_BASEADDR_LO,restore->CursorBaseAddrLo);
    i740_outxr(CURSOR_X_HI,restore->CursorXHi);
    i740_outxr(CURSOR_X_LO,restore->CursorXLo);
    i740_outxr(CURSOR_Y_HI,restore->CursorYHi);
    i740_outxr(CURSOR_Y_LO,restore->CursorYLo);

    temp=__svgalib_incrtc(INTERLACE_CNTL); 
    temp &= ~INTERLACE_ENABLE;
    temp |= restore->InterlaceControl;
    __svgalib_outcrtc(INTERLACE_CNTL, temp);

    temp = i740_inxr(ADDRESS_MAPPING);
    temp &= 0xE0; /* Save reserved bits 7:5 */
    temp |= restore->AddressMapping;
    i740_outxr(ADDRESS_MAPPING, temp);

    temp = i740_inxr(IO_CTNL);
    temp &= ~(EXTENDED_ATTR_CNTL|EXTENDED_CRTC_CNTL); 
    temp |= restore->IOControl;
    i740_outxr(IO_CTNL, temp);

    temp = i740_inxr(BITBLT_CNTL);
    temp &= ~COLEXP_MODE;
    temp |= restore->BitBLTControl;
    i740_outxr(BITBLT_CNTL, temp);

    temp = i740_inxr(DISPLAY_CNTL);
    temp &= ~(VGA_WRAP_MODE | GUI_MODE);
    temp |= restore->DisplayControl;
    i740_outxr(DISPLAY_CNTL, temp);

    temp = i740_inxr(PIXPIPE_CONFIG_0);
    temp &= 0x64; /* Save reserved bits 6:5,2 */
    temp |= restore->PixelPipeCfg0;
    i740_outxr(PIXPIPE_CONFIG_0, temp);

    temp = i740_inxr(PIXPIPE_CONFIG_2);
    temp &= 0xF3; /* Save reserved bits 7:4,1:0 */
    temp |= restore->PixelPipeCfg2;
    i740_outxr(PIXPIPE_CONFIG_2, temp);

    temp = i740_inxr(PLL_CNTL);
    temp &= ~PLL_MEMCLK_SEL;
#if 1
    temp = restore->PLLControl; /* To fix the 2.3X BIOS problem */
#else
    temp |= restore->PLLControl;
#endif
    i740_outxr(PLL_CNTL, temp);

    temp = i740_inxr(PIXPIPE_CONFIG_1);
    temp &= ~DISPLAY_COLOR_MODE;
    temp |= restore->PixelPipeCfg1;
    i740_outxr(PIXPIPE_CONFIG_1, temp);

    temp = INREG(FWATER_BLC);
    temp &= ~(LMI_BURST_LENGTH | LMI_FIFO_WATERMARK);
    temp |= restore->LMI_FIFO_Watermark;
    OUTREG(FWATER_BLC, temp);

    /* Turn on DRAM Refresh */
    i740_outxr(DRAM_EXT_CNTL, DRAM_REFRESH_60HZ);

    usleep(50000);

}

/* Return nonzero if mode is available */

static int i740_modeavailable(int mode)
{
    struct info *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (i740_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

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

static unsigned int I740CalcFIFO(double freq, int bpp)
{
    /*
     * Would like to calculate these values automatically, but a generic
     * algorithm does not seem possible.  Note: These FIFO water mark
     * values were tested on several cards and seem to eliminate the
     * all of the snow and vertical banding, but fine adjustments will
     * probably be required for other cards.
     */

    unsigned int wm = 0x18120000;

    switch (bpp) {
    case 8:
	if (I740HasSGRAM) {
	    if      (freq > 200) wm = 0x18120000;
	    else if (freq > 175) wm = 0x16110000;
	    else if (freq > 135) wm = 0x120E0000;
	    else                 wm = 0x100D0000;
	} else {
	    if      (freq > 200) wm = 0x18120000;
	    else if (freq > 175) wm = 0x16110000;
	    else if (freq > 135) wm = 0x120E0000;
	    else                 wm = 0x100D0000;
	}
	break;
    case 16:
	if (I740HasSGRAM) {
	    if      (freq > 140) wm = 0x2C1D0000;
	    else if (freq > 120) wm = 0x2C180000;
	    else if (freq > 100) wm = 0x24160000;
	    else if (freq >  90) wm = 0x18120000;
	    else if (freq >  50) wm = 0x16110000;
	    else if (freq >  32) wm = 0x13100000;
	    else                 wm = 0x120E0000;
	} else {
	    if      (freq > 160) wm = 0x28200000;
	    else if (freq > 140) wm = 0x2A1E0000;
	    else if (freq > 130) wm = 0x2B1A0000;
	    else if (freq > 120) wm = 0x2C180000;
	    else if (freq > 100) wm = 0x24180000;
	    else if (freq >  90) wm = 0x18120000;
	    else if (freq >  50) wm = 0x16110000;
	    else if (freq >  32) wm = 0x13100000;
	    else                 wm = 0x120E0000;
	}
	break;
    case 24:
	if (I740HasSGRAM) {
	    if      (freq > 130) wm = 0x31200000;
	    else if (freq > 120) wm = 0x2E200000;
	    else if (freq > 100) wm = 0x2C1D0000;
	    else if (freq >  80) wm = 0x25180000;
	    else if (freq >  64) wm = 0x24160000;
	    else if (freq >  49) wm = 0x18120000;
	    else if (freq >  32) wm = 0x16110000;
	    else                 wm = 0x13100000;
	} else {
	    if      (freq > 120) wm = 0x311F0000;
	    else if (freq > 100) wm = 0x2C1D0000;
	    else if (freq >  80) wm = 0x25180000;
	    else if (freq >  64) wm = 0x24160000;
	    else if (freq >  49) wm = 0x18120000;
	    else if (freq >  32) wm = 0x16110000;
	    else                 wm = 0x13100000;
	}
	break;
    case 32:
	if (I740HasSGRAM) {
	    if      (freq >  80) wm = 0x2A200000;
	    else if (freq >  60) wm = 0x281A0000;
	    else if (freq >  49) wm = 0x25180000;
	    else if (freq >  32) wm = 0x18120000;
	    else                 wm = 0x16110000;
	} else {
	    if      (freq >  80) wm = 0x29200000;
	    else if (freq >  60) wm = 0x281A0000;
	    else if (freq >  49) wm = 0x25180000;
	    else if (freq >  32) wm = 0x18120000;
	    else                 wm = 0x16110000;
	}
	break;
    }

    return wm;
}

#define MAX_VCO_FREQ 450.0
#define TARGET_MAX_N 30
#define REF_FREQ 66.66666666667

#define CALC_VCLK(m,n,p,d) \
    (double)m / ((double)n * (1 << p)) * (4 << (d << 1)) * REF_FREQ

static void
I740CalcVCLK(double freq ,int *M, int *N, int *MN, int *DIVSEL )
{
    int m, n, p, d;
    double f_out;
    double f_err;
    double f_vco;
    int m_best = 0, n_best = 0, p_best = 0, d_best = 0;
    double f_target = freq;
    double err_max = 0.005;
    double err_target = 0.001;
    double err_best = 999999.0;

    p_best = p = log(MAX_VCO_FREQ/f_target)/log((double)2);
    d_best = d = 0;

    f_vco = f_target * (1 << p);

    n = 2;
    do {
	n++;
	m = f_vco / (REF_FREQ / (double)n) / (double)4.0 + 0.5;
	if (m < 3) m = 3;
	f_out = CALC_VCLK(m,n,p,d);
	f_err = 1.0 - (f_target/f_out);
	if (fabs(f_err) < err_max) {
	    m_best = m;
	    n_best = n;
	    err_best = f_err;
	}
    } while ((fabs(f_err) >= err_target) &&
	     ((n <= TARGET_MAX_N) || (fabs(err_best) > err_max)));

    if (fabs(f_err) < err_target) {
	m_best = m;
        n_best = n;
    }

    *M     = (m_best-2) & 0xFF;
    *N     = (n_best-2) & 0xFF;
    *MN    = ((((n_best-2) >> 4) & VCO_N_MSBS) |
       	      (((m_best-2) >> 8) & VCO_M_MSBS));
    *DIVSEL= ((p_best << 4) | (d_best ? 4 : 0) | REF_DIV_1);

#if 0
    fprintf(stderr,"Setting dot clock to %.6lf MHz "
	   "[ %02X %02X %02X ] "
	   "[ %d %d %d %d ]\n",
	   CALC_VCLK(m_best,n_best,p_best,d_best),
	   new->VideoClk2_M,
	   new->VideoClk2_N,
	   new->VideoClk2_DivisorSel,
	   m_best, n_best, p_best, d_best);
#endif
}

/* Local, called by i740_setmode(). */
static void i740_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{    
    vgaI740Ptr new;
    int n, m, mn, divsel;
    
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    new = (vgaI740Ptr)(moderegs+VGA_TOTAL_REGS);

    moderegs[0x13] = modeinfo->lineWidth >> 3;
    new->ExtOffset = modeinfo->lineWidth >> 11;

    switch (modeinfo->bitsPerPixel) {
        case 8:
            new->PixelPipeCfg1 = DISPLAY_8BPP_MODE;
	break;
        case 15:
        case 16:
	    if (modeinfo->greenWeight == 5) {
	        new->PixelPipeCfg1 = DISPLAY_15BPP_MODE;
	    } else {
	        new->PixelPipeCfg1 = DISPLAY_16BPP_MODE;
	    }
	    break;
        case 24:
	    new->PixelPipeCfg1 = DISPLAY_24BPP_MODE;
	    break;
        case 32:
	    new->PixelPipeCfg1 = DISPLAY_32BPP_MODE;
	    break;
        default:
	    break;
    }

    if (modeinfo->bitsPerPixel>8)
	new->PixelPipeCfg0 = DAC_8_BIT;
    else
	new->PixelPipeCfg0 = DAC_6_BIT;

    new->PixelPipeCfg2 = 0;

    /* Turn on Extended VGA Interpretation */
    new->IOControl = EXTENDED_CRTC_CNTL;

    /* Turn on linear and page mapping */
    new->AddressMapping = LINEAR_MODE_ENABLE | PAGE_MAPPING_ENABLE;

    /* Turn on GUI mode */
    new->DisplayControl = HIRES_MODE;

    /* Set the MCLK freq */
    if (0)
	new->PLLControl = PLL_MEMCLK__66667KHZ; /*  66 MHz */
    else
	new->PLLControl = PLL_MEMCLK_100000KHZ; /* 100 MHz -- use as default */

    /* Calculate the extended CRTC regs */
    new->ExtVertTotal = (modetiming->CrtcVTotal - 2) >> 8;
    new->ExtVertDispEnd = (modetiming->CrtcVDisplay - 1) >> 8;
    new->ExtVertSyncStart = modetiming->CrtcVSyncStart >> 8;
    new->ExtVertBlankStart = modetiming->CrtcVSyncStart >> 8;
    new->ExtHorizTotal = ((modetiming->CrtcHTotal >> 3) - 5) >> 8;
    new->ExtHorizBlank = ((modetiming->CrtcHSyncEnd >> 3) & 0x40) >> 6;

    /* Turn on interlaced mode if necessary */
    if (modetiming->flags & INTERLACED)
	new->InterlaceControl = INTERLACE_ENABLE;
    else
	new->InterlaceControl = INTERLACE_DISABLE;

    moderegs[0x11] = 0;

    I740CalcVCLK(modetiming->pixelClock/1000, &m, &n, &mn, &divsel);
    
    new->VideoClk2_M = m;
    new->VideoClk2_N = n; 
    new->VideoClk2_MN_MSBs = mn;
    new->VideoClk2_DivisorSel = divsel;

    /* Since we program the clocks ourselves, always use VCLK2. */
    moderegs[59] |= 0x0C;

    new->ExtStartAddr=0;
    new->ExtStartAddrHi=0;

    /* Calculate the FIFO Watermark and Burst Length. */
    new->LMI_FIFO_Watermark = I740CalcFIFO(modetiming->pixelClock/1000,modeinfo->bitsPerPixel);

    new->CursorControl= CURSOR_ORIGIN_DISPLAY | CURSOR_MODE_32_4C_AX;

    return ;
}


static int i740_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!i740_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = calloc(I740_TOTAL_REGS,1);

    i740_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    i740_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void i740_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    
}

static void i740_lock(void)
{
}


#define VENDOR_ID 0x8086
#define I740_ID_PCI 0x00d1
#define I740_ID_AGP 0x7800

/* Indentify chipset, initialize and return non-zero if detected */

static int i740_test(void)
{
    int found;
    long buf[64];
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    
    if(!found&&
       ((((buf[0]>>16)&0xffff)==I740_ID_PCI)||
        (((buf[0]>>16)&0xffff)==I740_ID_AGP))){
          i740_init(0,0,0);
          return 1;
    };
    
    return 0;
}

/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void i740_setdisplaystart(int address)
{ 
  address=address >> 2;
  __svgalib_outcrtc(0x0c, address & 0xFF00);
  __svgalib_outcrtc(0x0d, address & 0x00FF);
  __svgalib_outcrtc(EXT_START_ADDR_HI,(address&0x3fc00000)>>22);
  __svgalib_outcrtc(EXT_START_ADDR,((address&0x3f0000)>>16)|EXT_START_ADDR_ENABLE);
}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void i740_setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    __svgalib_outcrtc(0x13,offset&0xff);
    __svgalib_outcrtc(EXT_OFFSET, offset>>8);
}

static int i740_linear(int op, int param)
{
    if (op==LINEAR_ENABLE){i740_is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){i740_is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return i740_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static unsigned char cur_colors[16*6];

static int i740_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j, tmp;
    unsigned long *b3;
    unsigned long l1, l2, l3, l4;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            i740_outxr(PIXPIPE_CONFIG_0,i740_inxr(PIXPIPE_CONFIG_0)&~HW_CURSOR_ENABLE); 
            break;
        case CURSOR_SHOW:
            i740_outxr(PIXPIPE_CONFIG_0,i740_inxr(PIXPIPE_CONFIG_0)|HW_CURSOR_ENABLE); 
            break;
        case CURSOR_POSITION:
            i740_outxr(CURSOR_X_LO,p1&0xff);
            i740_outxr(CURSOR_X_HI,(p1&0x700)>>8);
            i740_outxr(CURSOR_Y_LO,p2&0xff);
            i740_outxr(CURSOR_Y_HI,(p2&0x700)>>8);
            break;
        case CURSOR_SELECT:
            i=i740_memory*1024-(p1+1)*4096;
            if(i>4096*1024)i-=4096*1024;
            tmp=i740_inxr(PIXPIPE_CONFIG_0);
            i740_outxr(PIXPIPE_CONFIG_0,tmp| EXTENDED_PALETTE);
            __svgalib_outpal(4,cur_colors[p1*6],cur_colors[p1*6+1],cur_colors[p1*6+2]);
            __svgalib_outpal(5,cur_colors[p1*6+3],cur_colors[p1*6+4],cur_colors[p1*6+5]);
            i740_outxr(PIXPIPE_CONFIG_0,tmp);
            i740_outxr(CURSOR_BASEADDR_LO,i>>8); 
            i740_outxr(CURSOR_BASEADDR_HI,i>>16); 
            break;
        case CURSOR_IMAGE:
            i=i740_memory*1024-(p1+1)*4096;
            if(i>4096*1024)i-=4096*1024;
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    cur_colors[p1*6]=(p3&0xff0000)>>16;
                    cur_colors[p1*6+1]=(p3&0xff00)>>8;
                    cur_colors[p1*6+2]=p3&0xff;
                    cur_colors[p1*6+3]=(p4&0xff0000)>>16;
                    cur_colors[p1*6+4]=(p4&0xff00)>>8;
                    cur_colors[p1*6+5]=p4&0xff;
                    for(j=0;j<16;j++) {
                        l1=*(b3+2*j);
                        l2=~(*(b3+32+2*j));
                        l3=*(b3+2*j+1);
                        l4=~(*(b3+32+2*j+1));
                        /*change endianess */
                        l1=(l1<<24)|(l1>>24)|((l1>>8)&0xff00)|((l1<<8)&0xff0000);
                        l2=(l2<<24)|(l2>>24)|((l2>>8)&0xff00)|((l2<<8)&0xff0000);
                        l3=(l3<<24)|(l3>>24)|((l3>>8)&0xff00)|((l3<<8)&0xff0000);
                        l4=(l4<<24)|(l4>>24)|((l4>>8)&0xff00)|((l4<<8)&0xff0000);
                        *(unsigned long *)(LINEAR_POINTER+i+16*j)=l2;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+4)=l4;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+8)=l1;
                        *(unsigned long *)(LINEAR_POINTER+i+16*j+12)=l3;
                    }
                break;
            }
            break;
    }
    return 0;
}       

static int i740_match_programmable_clock(int clock)
{
return clock ;
}

static int i740_map_clock(int bpp, int clock)
{
return clock ;
}

static int i740_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

/* Function table (exported) */

DriverSpecs __svgalib_i740_driverspecs =
{
    i740_saveregs,
    i740_setregs,
    i740_unlock,
    i740_lock,
    i740_test,
    i740_init,
    i740_setpage,
    NULL,
    NULL,
    i740_setmode,
    i740_modeavailable,
    i740_setdisplaystart,
    i740_setlogicalwidth,
    i740_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    i740_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    i740_cursor
};

/* Initialize chipset (called after detection) */
static int i740_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0;
    int temp;
    
    i740_inxr=i740_inxr_io;
    i740_outxr=i740_outxr_io;

    i740_unlock();

    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    i740_linear_base=0;
    if(!found&&((((buf[0]>>16)&0xffff)==I740_ID_PCI)||(((buf[0]>>16)&0xffff)==I740_ID_AGP))){
       i740_linear_base=buf[4]&0xffffff00;
       i740_mmio_base=buf[5]&0xffffff00;
    };

    __svgalib_vgammbase=mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_SHARED,__svgalib_mem_fd,i740_mmio_base);
    __svgalib_mm_io_mapio();
    i740_inxr=i740_inxr_mm;
    i740_outxr=i740_outxr_mm;
    
    if (force) {
	i740_memory = par1;
/*        i740_chiptype = par2;*/
    } else {
        if((i740_inxr(DRAM_ROW_TYPE)&DRAM_ROW_1) == DRAM_ROW_1_SDRAM)
            i740_memory=i740_inxr(DRAM_ROW_BNDRY_1)*1024; else
            i740_memory=i740_inxr(DRAM_ROW_BNDRY_0)*1024; 
    };

    temp=i740_inxr(DRAM_ROW_CNTL_LO);
    I740HasSGRAM = !((temp & DRAM_RAS_TIMING) || (temp & DRAM_RAS_PRECHARGE));

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using I740 driver, %iKB.\n",i740_memory);
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = i740_memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 203000;	
    cardspecs->maxPixelClock16bpp = 163000;	
    cardspecs->maxPixelClock24bpp = 128000;
    cardspecs->maxPixelClock32bpp = 86000;
    cardspecs->flags = CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = i740_map_clock;
    cardspecs->mapHorizontalCrtc = i740_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=i740_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_i740_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=i740_linear_base;
    __svgalib_linear_mem_size=i740_memory*0x400;
    __svgalib_mmio_base=i740_mmio_base;
    __svgalib_mmio_size=512*0x400;
    return 0;
}
