/*
 i810 chipset driver
  written by Matan Ziv-Av.
*/

/* TODO Implement GTT for MSDOS. It's used by the i810 driver */
/* #define USE_GTT 1 */

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
#include "i810_reg.h"
#include "i810_wma.c"
#include "vgammvga.h"
#include <sys/ioctl.h>
/* #include "svgalib_helper.h" */

typedef struct {
/* 00 */
    unsigned char DisplayControl;
    unsigned char PixelPipeCfg0;
    unsigned char PixelPipeCfg1;
    unsigned char PixelPipeCfg2;
    unsigned short VideoClk2_M;
    unsigned short VideoClk2_N;
/* 08 */
    unsigned char VideoClk2_DivisorSel;

    unsigned char AddressMapping;
    unsigned char IOControl;
    unsigned char BitBLTControl;

    unsigned char ExtVertTotal;
    unsigned char ExtVertDispEnd;
    unsigned char ExtVertSyncStart;
    unsigned char ExtVertBlankStart;
/* 10 */
    unsigned char ExtHorizTotal;
    unsigned char ExtHorizBlank;
    unsigned char ExtOffset;
    unsigned char InterlaceControl;
    unsigned char ExtStartAddr;
    unsigned char ExtStartAddrHi;    

    unsigned short CursorControl;
/* 18 */
    unsigned int CursorBaseAddr;
    unsigned char CursorXLo;
    unsigned char CursorXHi;
    unsigned char CursorYLo;
    unsigned char CursorYHi;
/* 20 */
    unsigned int  LMI_FIFO_Watermark; 
} vgaI810Rec, *vgaI810Ptr;

#define I810REG_SAVE(i) (VGA_TOTAL_REGS+i)
#define I810_TOTAL_REGS (VGA_TOTAL_REGS + sizeof(vgaI810Rec))

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory, lmfreq;
static int is_linear, linear_base, mmio_base;
static unsigned int cur_base;
static CardSpecs *cardspecs;

static void setpage(int page)
{
   __svgalib_outgra(0x11,page);
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
    modeinfo->maxlogicalwidth = 8184;
    modeinfo->startaddressrange = memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
#ifdef USE_GTT
	if(linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
#endif
    }
}

/* Read and save chipset-specific registers */

static int saveregs(unsigned char regs[])
{ 
    vgaI810Ptr save;

    unlock();		
    save = (vgaI810Ptr)(regs+VGA_TOTAL_REGS);

    save->IOControl=__svgalib_incrtc(IO_CTNL);
    save->AddressMapping=__svgalib_ingra(ADDRESS_MAPPING);
    save->BitBLTControl=INREG8(BITBLT_CNTL);
    save->VideoClk2_M=INREG16(VCLK2_VCO_M);
    save->VideoClk2_N=INREG16(VCLK2_VCO_N);
    save->VideoClk2_DivisorSel=INREG8(VCLK2_VCO_DIV_SEL);

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

    save->PixelPipeCfg0=INREG8(PIXPIPE_CONFIG_0);
    save->PixelPipeCfg1=INREG8(PIXPIPE_CONFIG_1);
    save->PixelPipeCfg2=INREG8(PIXPIPE_CONFIG_2);
    save->DisplayControl=INREG8(DISPLAY_CNTL);

    save->CursorControl=INREG(CURSOR_CONTROL);
    save->CursorBaseAddr=INREG(CURSOR_BASEADDR);
    save->CursorXHi=INREG8(CURSOR_X_HI);
    save->CursorXLo=INREG8(CURSOR_X_LO);
    save->CursorYHi=INREG8(CURSOR_Y_HI);
    save->CursorYLo=INREG8(CURSOR_Y_LO);

    save->LMI_FIFO_Watermark = INREG(FWATER_BLC);

    return I810_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    int temp;
    vgaI810Ptr restore;

    unlock();		
    restore = (vgaI810Ptr)(regs+VGA_TOTAL_REGS);

    temp=__svgalib_inseq(0x01);
    __svgalib_outseq(0, 1);
    __svgalib_outseq(1, temp | 0x20);

    usleep(50000);

    temp=INREG8(DRAM_ROW_CNTL_HI);
    temp &= ~0x18;
    OUTREG8(DRAM_ROW_CNTL_HI, temp);

    usleep(1000); /* Wait 1 ms */

    /* Write the M, N and P values */
    OUTREG16(VCLK2_VCO_M, restore->VideoClk2_M);
    OUTREG16(VCLK2_VCO_N, restore->VideoClk2_N);
    OUTREG8(VCLK2_VCO_DIV_SEL, restore->VideoClk2_DivisorSel);


    temp = INREG(PIXPIPE_CONFIG_0);
    temp &= ~(DAC_8_BIT | HW_CURSOR_ENABLE); 
    temp |= (restore->PixelPipeCfg0 & (DAC_8_BIT | HW_CURSOR_ENABLE));
    OUTREG(PIXPIPE_CONFIG_0, temp);

    temp = __svgalib_incrtc(IO_CTNL);
    temp &= ~(EXTENDED_ATTR_CNTL|EXTENDED_CRTC_CNTL); 
    temp |= EXTENDED_CRTC_CNTL;
    __svgalib_outcrtc(IO_CTNL, temp);

    __svgalib_outcrtc(EXT_VERT_TOTAL, restore->ExtVertTotal);
    __svgalib_outcrtc(EXT_VERT_DISPLAY, restore->ExtVertDispEnd);
    __svgalib_outcrtc(EXT_VERT_SYNC_START, restore->ExtVertSyncStart);
    __svgalib_outcrtc(EXT_VERT_BLANK_START, restore->ExtVertBlankStart);
    __svgalib_outcrtc(EXT_HORIZ_TOTAL, restore->ExtHorizTotal);
    __svgalib_outcrtc(EXT_HORIZ_BLANK, restore->ExtHorizBlank);
    __svgalib_outcrtc(EXT_OFFSET, restore->ExtOffset);
    __svgalib_outcrtc(EXT_START_ADDR, restore->ExtStartAddr);
    __svgalib_outcrtc(EXT_START_ADDR_HI, restore->ExtStartAddrHi);

    OUTREG16(CURSOR_CONTROL,restore->CursorControl);
    OUTREG(CURSOR_BASEADDR,restore->CursorBaseAddr);
    OUTREG8(CURSOR_X_HI,restore->CursorXHi);
    OUTREG8(CURSOR_X_LO,restore->CursorXLo);
    OUTREG8(CURSOR_Y_HI,restore->CursorYHi);
    OUTREG8(CURSOR_Y_LO,restore->CursorYLo);

    temp=__svgalib_incrtc(INTERLACE_CNTL); 
    temp &= ~INTERLACE_ENABLE;
    temp |= restore->InterlaceControl;
    __svgalib_outcrtc(INTERLACE_CNTL, temp);

    temp = __svgalib_ingra(ADDRESS_MAPPING);
    temp &= 0xE0; /* Save reserved bits 7:5 */
    temp |= restore->AddressMapping;
    __svgalib_outgra(ADDRESS_MAPPING, temp);
    

    temp=INREG8(DRAM_ROW_CNTL_HI);
    temp &= ~0x18;
    temp |= 0x08;
    OUTREG8(DRAM_ROW_CNTL_HI, temp);

#if 0
    temp = INREG8(BITBLT_CNTL);
    temp &= ~COLEXP_MODE;
    temp |= restore->BitBLTControl;
    OUTREG8(BITBLT_CNTL, temp);
#endif

    temp = INREG8(DISPLAY_CNTL);
    temp &= ~(VGA_WRAP_MODE | GUI_MODE);
    temp |= restore->DisplayControl;
    OUTREG8(DISPLAY_CNTL, temp);

    temp = INREG8(PIXPIPE_CONFIG_0);
    temp &= 0x64; /* Save reserved bits 6:5,2 */
    temp |= restore->PixelPipeCfg0;
    OUTREG8(PIXPIPE_CONFIG_0, temp);

    temp = INREG8(PIXPIPE_CONFIG_2);
    temp &= 0xF3; /* Save reserved bits 7:4,1:0 */
    temp |= restore->PixelPipeCfg2;
    OUTREG8(PIXPIPE_CONFIG_2, temp);

    temp = INREG8(PIXPIPE_CONFIG_1);
    temp &= 0xe0;
    temp |= restore->PixelPipeCfg1;
    OUTREG8(PIXPIPE_CONFIG_1, temp);

#if 0
    temp = INREG(FWATER_BLC);
    temp &= ~(LM_BURST_LENGTH | LM_FIFO_WATERMARK | MM_BURST_LENGTH | MM_FIFO_WATERMARK);
    temp |= restore->LMI_FIFO_Watermark;
    OUTREG(FWATER_BLC, temp);
#endif

    temp=__svgalib_inseq(0x01);
    __svgalib_outseq(1, temp & ~0x20);
    __svgalib_outseq(0, 3);

    temp = __svgalib_incrtc(IO_CTNL);
    temp &= ~(EXTENDED_ATTR_CNTL|EXTENDED_CRTC_CNTL); 
    temp |= restore->IOControl;
    __svgalib_outcrtc(IO_CTNL, temp);

}

/* Return nonzero if mode is available */

static int modeavailable(int mode)
{
    struct info *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (memory * 1024 < info->ydim * info->xbytes)
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

#define MAX_VCO_FREQ 600000
#define TARGET_MAX_N 30
#define REF_FREQ 24000

#define CALC_VCLK(m,n,p) \
    ((m * 4 * REF_FREQ / n ) >> p) 

static void
I810CalcVCLK(int freq, int *M, int *N, int *DIVSEL )
{
    int m, n, p;
    int f_out;
    int f_err, f_best;
    int f_vco;
    int m_best = 0, n_best = 0, p_best = 0;
    int f_target = freq;
    int err_max = 5000;
    int err_target = 100;
    int err_best = 999999;

    p=1;
    while((1<<p)*freq*2 < MAX_VCO_FREQ) p++;
    if(p>5)p=5;
    p_best=p;

    f_vco = f_target * (1 << p);

    n = 2;
    do {
        n++;
        m = f_vco * n / REF_FREQ / 4;
        if (m < 3) m = 3;
        f_out = CALC_VCLK(m,n,p);
        f_err = f_target-f_out;
        if (f_err < err_max) {
            m_best = m;
            n_best = n;
            f_best = f_out;
            err_best = f_err;
        }
    } while ((f_err >= err_target) &&
             ((n <= TARGET_MAX_N) || (err_best > err_max)));

    if (f_err < err_target) {
        m_best = m;
        n_best = n;
    }

    *M     = (m_best-2) & 0x3FF;
    *N     = (n_best-2) & 0x3FF;
    *DIVSEL= p_best << 4;
}

/* Local, called by setmode(). */
static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{    
    vgaI810Ptr new;
    int n, m, divsel;
    
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    new = (vgaI810Ptr)(moderegs+VGA_TOTAL_REGS);

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
    new->PixelPipeCfg1 |= 0x10;

    /* Turn on Extended VGA Interpretation */
    new->IOControl = EXTENDED_CRTC_CNTL;

    /* Turn on linear and page mapping */
#ifdef USE_GTT
    new->AddressMapping =  LINEAR_MODE_ENABLE | 8 ;
    new->DisplayControl =  1;
#else
    new->AddressMapping = PACKED_MODE_ENABLE | PAGE_MAPPING_ENABLE ;
    new->DisplayControl =  2;
#endif
    
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

    moderegs[VGA_AR11] = 0;

    I810CalcVCLK(modetiming->pixelClock, &m, &n, &divsel);
    
    new->VideoClk2_M = m;
    new->VideoClk2_N = n; 
    new->VideoClk2_DivisorSel = divsel;

    /* Since we program the clocks ourselves, always use VCLK2. */
    moderegs[59] |= 0x0C;

    new->ExtStartAddr=0;
    new->ExtStartAddrHi=0;

    /* Calculate the FIFO Watermark and Burst Length. */
    new->LMI_FIFO_Watermark = I810CalcWatermark(modetiming->pixelClock/1000,modeinfo->bitsPerPixel, lmfreq, 0);

    new->CursorControl= CURSOR_ORIGIN_DISPLAY | CURSOR_MODE_32_4C_AX;

    return ;
}

static void setdisplaystart(int address);

static int setmode(int mode, int prv_mode)
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

    moderegs = malloc(I810_TOTAL_REGS);

    initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    setdisplaystart(0);

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


#define VENDOR_ID 0x8086
#define ID10	0x7121
#define ID10DC	0x7123
#define ID10E	0x7125
#define ID15	0x1132

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    int found;
    long buf[64];
    int id;
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    id=buf[0]>>16;
    
    if( !found && ((id==ID10) || (id==ID10DC) || (id==ID10E) || (id==ID15)) ){
          init(0,0,0);
          return 1;
    };
    
    return 0;
}

/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void setdisplaystart(int address)
{ 
  address=address >> 2;
  __svgalib_outcrtc(0x0c, address & 0xFF00);
  __svgalib_outcrtc(0x0d, address & 0x00FF);
  __svgalib_outcrtc(EXT_START_ADDR_HI,(address&0x3fc00000)>>22);
  __svgalib_outcrtc(EXT_START_ADDR,((address&0x3f0000)>>16)|EXT_START_ADDR_ENABLE);
}

/* Set logical scanline length (usually multiple of 8) */

static void setlogicalwidth(int width)
{   
#ifdef USE_GTT
    int offset = width >> 3;
#else
    int offset = width >> 4;
#endif
 
    __svgalib_outcrtc(0x13,offset&0xff);
    __svgalib_outcrtc(EXT_OFFSET, offset>>8);

}

static int linear(int op, int param)
{
    if (op==LINEAR_ENABLE){is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static unsigned char cur_colors[16*6];

static int cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j, tmp;
    unsigned long *b3;
    unsigned long l1, l2, l3, l4;

    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            OUTREG8(PIXPIPE_CONFIG_0,INREG8(PIXPIPE_CONFIG_0)&~HW_CURSOR_ENABLE); 
            break;
        case CURSOR_SHOW:
            OUTREG8(PIXPIPE_CONFIG_0,INREG8(PIXPIPE_CONFIG_0)|HW_CURSOR_ENABLE); 
            break;
        case CURSOR_POSITION:
            OUTREG8(CURSOR_X_LO,p1&0xff);
            OUTREG8(CURSOR_X_HI,(p1&0x700)>>8);
            OUTREG8(CURSOR_Y_LO,p2&0xff);
            OUTREG8(CURSOR_Y_HI,(p2&0x700)>>8);
            break;
        case CURSOR_SELECT:
            tmp=INREG8(PIXPIPE_CONFIG_0);
            OUTREG8(PIXPIPE_CONFIG_0,tmp| EXTENDED_PALETTE);
            OUTREG8(0x3c6, 0xff);
            OUTREG8(0x3c8, 0x04);
            for(i=0;i<6;i++) 
                OUTREG8(0x3c9, cur_colors[p1*6+i]);
            OUTREG8(PIXPIPE_CONFIG_0,tmp);
            OUTREG(CURSOR_BASEADDR,cur_base+(15-p1)*256); 
            break;
        case CURSOR_IMAGE:
            i=memory*1024-(p1+1)*256;
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

DriverSpecs __svgalib_i810_driverspecs =
{
    saveregs,
    setregs,
    unlock,
    lock,
    test,
    init,
#ifdef USE_GTT
    __svgalib_emul_setpage,
#else
    setpage,
#endif
    NULL,
    NULL,
    setmode,
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
    cursor
};

/* Initialize chipset (called after detection) */
static int init(int force, int par1, int par2)
{
    unsigned long buf[64];
    unsigned int id;
    int found=0;
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    linear_base=0;
    id=buf[0]>>16;
    
    if( !found && ((id==ID10) || (id==ID10DC) || (id==ID10E) || (id==ID15)) ){
       linear_base=buf[4]&0xffffff00;
       mmio_base=buf[5]&0xffffff00;
    } else return -1; 
    ;

    __svgalib_mmio_base=mmio_base;
    __svgalib_mmio_size=512*0x400;
    map_mmio();
    __svgalib_vgammbase=0;
    __svgalib_mm_io_mapio();
    
    if (force) {
	memory = par1;
    } else {
        memory = 512;
    };

#ifdef USE_GTT
    if(*(unsigned int *)(__svgalib_vgammbase + 0x2020) & 1) {
       /* GTT already set */
       memory = 4096;
    } else {
        memory = 4096;
        ioctl(__svgalib_mem_fd, SVGALIB_HELPER_IOCGI810GTT, &t);
        *(unsigned int *)(__svgalib_vgammbase + 0x2020) = t | 1;
        for(i=0;i<1024;i++) {
            t=i;
            ioctl(__svgalib_mem_fd, SVGALIB_HELPER_IOCGI810GTTE, &t);
            *(unsigned int *)(__svgalib_vgammbase + 0x10000 + 4*i) = t;
        }
    }
    cur_base = (memory>>2)-1;
    ioctl(__svgalib_mem_fd, SVGALIB_HELPER_IOCGI810GTTE, &cur_base);
    cur_base &= ~1 ;
    __svgalib_banked_mem_base=linear_base;
    __svgalib_modeinfo_linearset |= IS_LINEAR;
#else
    linear_base=0;
    __svgalib_banked_mem_base=0xa0000;
#endif

    if(buf[20]&0x10) {
        lmfreq=133;
    } else {
        lmfreq=100;
    }

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using I810 driver, %iKB.\n",memory);
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 203000;	
    cardspecs->maxPixelClock16bpp = 163000;	
    cardspecs->maxPixelClock24bpp = 128000;
    cardspecs->maxPixelClock32bpp = 86000;
    cardspecs->flags = CLOCK_PROGRAMMABLE | INTERLACE_DIVIDE_VERT;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks = 0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_i810_driverspecs;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=memory*0x400;
    return 0;
}
