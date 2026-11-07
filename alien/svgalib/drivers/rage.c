/*
ATI Rage chipset 

By Matan Ziv-Av (matan@svgalib.org)

Please report to me any problem with the driver 

It works on my Xpert98 AGP 8MB SDRAM.

The driver assumes a PCI (or AGP) device, i.e a device that registers on the 
PCI bus.

This driver should work (maybe with small changes) on all Mach64 cards 
that use internal Clock and DAC. (Mach64 CT,ET,VT,GT (Rage),Rage II, 
Rage II+DVD, and Rage Pro according to the XFree86 Documentation.
  
If you have an older Mach64 card, it should be easy to adapt this driver
to work on your card.

The test only tests for a Mach64 card, not for internal DAC.

This driver is based on The XFree86 Mach64 Server and ati driver for the 
SVGA server. That code is :

Copyright 1994 through 1998 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca

*/

#include <stdlib.h>
#include <stdio.h>		/* for printf */
#include <string.h>		/* for memset */
#include <unistd.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"

/* New style driver interface. */
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "rage.h"

static int    ATIIOPortCRTC_H_TOTAL_DISP, ATIIOPortCRTC_H_SYNC_STRT_WID,
              ATIIOPortCRTC_V_TOTAL_DISP, ATIIOPortCRTC_V_SYNC_STRT_WID,
              ATIIOPortCRTC_OFF_PITCH,ATIIOPortCRTC_INT_CNTL,
              ATIIOPortCRTC_GEN_CNTL, ATIIOPortDSP_CONFIG, ATIIOPortDSP_ON_OFF,
              ATIIOPortOVR_CLR,
              ATIIOPortCLOCK_CNTL, ATIIOPortBUS_CNTL, ATIIOPortMEM_INFO,
              ATIIOPortMEM_VGA_WP_SEL, ATIIOPortMEM_VGA_RP_SEL,
              ATIIOPortDAC_REGS, ATIIOPortDAC_CNTL, ATIIOPortDAC_DATA,
              ATIIOPortDAC_READ, ATIIOPortDAC_WRITE,ATIIOPortDAC_MASK,
              ATIIOPortGEN_TEST_CNTL, ATIIOPortCONFIG_CNTL,
              ATIIOPortCONFIG_STATUS64_0, ATIIOPortCUR_HORZ_VERT_OFF,
              ATIIOPortCUR_HORZ_VERT_POSN, ATIIOPortCUR_CLR0,
              ATIIOPortCUR_CLR1, ATIIOPortCUR_OFFSET;

#include "rageio.c"

static void ATIAccessMach64PLLReg(const int Index, const int Write)
{
    int clock_cntl1 = rage_inb(ATIIOPortCLOCK_CNTL + 1) &
        ~GetByte(PLL_WR_EN | PLL_ADDR, 1);

    /* Set PLL register to be read or written */
    rage_outb(ATIIOPortCLOCK_CNTL + 1, clock_cntl1 |
        GetByte(SetBits(Index, PLL_ADDR) | SetBits(Write, PLL_WR_EN), 1));
}

#define DAC_SIZE 768

typedef struct {
        unsigned int  
               crtc_h_total_disp, crtc_h_sync_strt_wid,
               crtc_v_total_disp, crtc_v_sync_strt_wid,
               crtc_off_pitch, crtc_gen_cntl, 
               crtc_vline_crnt_vline, dsp_config, 
               dsp_on_off, ovr_clr, 
               clock_cntl, bus_cntl, 
               mem_vga_wp_sel, mem_vga_rp_sel,
               dac_cntl, config_cntl, 
               banks, planes,
               dac_read, dac_write, 
               dac_mask, mem_info,
               mem_buf_cntl, shared_cntl, 
               shared_mem_config, crtc_int_cntl,
               gen_test_cntl, misc_options, 
               mem_bndry, mem_cfg,
               cur_clr0, cur_clr1, 
               cur_offset, cur_horz_vert_posn, 
               cur_horz_vert_off;
        unsigned char PLL[32];
        unsigned char DAC[DAC_SIZE];
        unsigned char extdac[16];
        int ics2595;
        unsigned char atib[16]; 
       } ATIHWRec, *ATIHWPtr;

#define RAGE_TOTAL_REGS (VGA_TOTAL_REGS + sizeof(ATIHWRec))

extern unsigned char __svgalib_ragedoubleclock;
static int rage_init(int, int, int);
static void rage_unlock(void);
static void rage_lock(void);
static int rage_memory,rage_chiptyperev;
static int rage_is_linear=0 , rage_linear_base;
static int ATIIODecoding;
static int ATIIOBase;
static int postdiv[8]={1,2,4,8,3,0,6,12};
static int rage_bpp;
static int maxM, minM, Madj, minN, maxN, Nadj;
static double fref;
static int ATIClockToProgram;
static int ATIChip, ATIMemoryType;
static int rage_dac, rage_clock;

static CardSpecs *cardspecs;

static void rage_ChipID(void)
{
   int ATIChipType, ATIChipClass, ATIChipRevision, ATIChipVersion,
       ATIChipFoundry;
    unsigned int IO_Value = rage_inl(ATIIOPort(CONFIG_CHIP_ID));
    ATIChipType     = GetBits(IO_Value, 0xFFFFU);
    ATIChipClass    = GetBits(IO_Value, CFG_CHIP_CLASS);
    ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REV);
    ATIChipVersion  = GetBits(IO_Value, CFG_CHIP_VERSION);
    ATIChipFoundry  = GetBits(IO_Value, CFG_CHIP_FOUNDRY);
    switch (ATIChipType)
    {
        case 0x00D7U:
            ATIChipType = 0x4758U;
        case 0x4758U:
            switch (ATIChipRevision)
            {
                case 0x00U:
                    ATIChip = ATI_CHIP_88800GXC;
                    break;

                case 0x01U:
                    ATIChip = ATI_CHIP_88800GXD;
                    break;

                case 0x02U:
                    ATIChip = ATI_CHIP_88800GXE;
                    break;

                case 0x03U:
                    ATIChip = ATI_CHIP_88800GXF;
                    break;

                default:
                    ATIChip = ATI_CHIP_88800GX;
                    break;
            }
            break;

        case 0x0057U:
            ATIChipType = 0x4358U;
        case 0x4358U:
            ATIChip = ATI_CHIP_88800CX;
            break;

        case 0x0053U:
            ATIChipType = 0x4354U;
        case 0x4354U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264CT;
            break;

        case 0x0093U:
            ATIChipType = 0x4554U;
        case 0x4554U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264ET;
            break;

        case 0x02B3U:
            ATIChipType = 0x5654U;
        case 0x5654U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264VT;
            /* Some early GT's are detected as VT's */
/*            if (ExpectedChipType && (ATIChipType != ExpectedChipType))
            {
                if (ExpectedChipType == 0x4754U)
                    ATIChip = ATI_CHIP_264GT;
                else 
                    ErrorF("Mach64 chip type probe discrepancy detected:\n"
                           " PCI=0x%04X;  CHIP_ID=0x%04X.\n",
                           ExpectedChipType, ATIChipType);
            }
            else */if (ATIChipVersion)
                ATIChip = ATI_CHIP_264VTB;
            break;

        case 0x00D3U:
            ATIChipType = 0x4754U;
        case 0x4754U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            if (!ATIChipVersion)
                ATIChip = ATI_CHIP_264GT;
            else
                ATIChip = ATI_CHIP_264GTB;
            break;

        case 0x02B4U:
            ATIChipType = 0x5655U;
        case 0x5655U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264VT3;
            break;

        case 0x00D4U:
            ATIChipType = 0x4755U;
        case 0x4755U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264GTDVD;
            break;

        case 0x0166U:
            ATIChipType = 0x4C47U;
        case 0x4C47U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264LT;
            break;

        case 0x0315U:
            ATIChipType = 0x5656U;
        case 0x5656U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264VT4;
            break;

        case 0x00D5U:
            ATIChipType = 0x4756U;
        case 0x4756U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264GT2C;
            break;

        case 0x00D6U:
        case 0x00D9U:
            ATIChipType = 0x4757U;
        case 0x4757U:
        case 0x475AU:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264GT2C;
            break;

        case 0x00CEU:
        case 0x00CFU:
        case 0x00D0U:
            ATIChipType = 0x4750U;
        case 0x4749U:
        case 0x4750U:
        case 0x4751U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264GTPRO;
            break;

        case 0x00C7U:
        case 0x00C9U:
            ATIChipType = 0x4742U;
        case 0x4742U:
        case 0x4744U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264GTPRO;
            break;

        case 0x0168U:
        case 0x016FU:
            ATIChipType = 0x4C50U;
        case 0x4C49U:
        case 0x4C50U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264LTPRO;
            break;

        case 0x0161U:
        case 0x0163U:
            ATIChipType = 0x4C42U;
        case 0x4C42U:
        case 0x4C44U:
            ATIChipRevision = GetBits(IO_Value, CFG_CHIP_REVISION);
            ATIChip = ATI_CHIP_264LTPRO;
            break;

        default:
            ATIChip = ATI_CHIP_Mach64;
            break;
    }
};

static int rage_probe(void)
{
    unsigned int i=rage_inl(ATIIOPort(SCRATCH_REG0));
	rage_outl(ATIIOPort(SCRATCH_REG0),0x55555555);
    if(rage_inl(ATIIOPort(SCRATCH_REG0))!=0x55555555) {
       rage_outl(ATIIOPort(SCRATCH_REG0),i);
       return 0;
    };
    rage_outl(ATIIOPort(SCRATCH_REG0),0xaaaaaaaa);
    if(rage_inl(ATIIOPort(SCRATCH_REG0))!=0xaaaaaaaa) {
       rage_outl(ATIIOPort(SCRATCH_REG0),i);
       return 0;
    };
    rage_outl(ATIIOPort(SCRATCH_REG0),i);

    return 1;
};

static int
ATIDivide(int Numerator, int Denominator, int Shift, const int RoundingKind)
{
    int Multiplier, Divider;
    int Rounding = 0;                           /* Default to floor */

    /* Deal with right shifts */
    if (Shift < 0)
    {
        Divider = (Numerator - 1) ^ Numerator;
        Multiplier = 1 << (-Shift);
        if (Divider > Multiplier)
            Divider = Multiplier;
        Numerator /= Divider;
        Denominator *= Multiplier / Divider;
        Shift = 0;
    }

    if (!RoundingKind)                          /* Nearest */
        Rounding = Denominator >> 1;
    else if (RoundingKind > 0)                  /* Ceiling */
        Rounding = Denominator - 1;

    return ((Numerator / Denominator) << Shift) +
            ((((Numerator % Denominator) << Shift) + Rounding) / Denominator);
}

static void rage_setpage(int page)
{
    page*=2;
    rage_outl(ATIIOPortMEM_VGA_WP_SEL, page | ((page+1)<<16));
    rage_outl(ATIIOPortMEM_VGA_RP_SEL, page | ((page+1)<<16));
}

static void rage_setrdpage(int page)
{
    page*=2;
    rage_outl(ATIIOPortMEM_VGA_RP_SEL, page | ((page+1)<<16));
}

static void rage_setwrpage(int page)
{
    page*=2;
    rage_outl(ATIIOPortMEM_VGA_WP_SEL, page | ((page+1)<<16));
}

static int __svgalib_rage_inlinearmode(void)
{
return rage_is_linear;
}

/* Fill in chipset specific mode information */

static void rage_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = rage_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = rage_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;
    modeinfo->flags |= HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(rage_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_rage_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */

static int rage_saveregs(unsigned char regs[])
{ 
  ATIHWPtr save;
  int i;

  save=(ATIHWPtr)(regs+VGA_TOTAL_REGS);

  save->crtc_gen_cntl = rage_inl(ATIIOPortCRTC_GEN_CNTL);

  save->crtc_h_total_disp = rage_inl(ATIIOPortCRTC_H_TOTAL_DISP);
  save->crtc_h_sync_strt_wid = rage_inl(ATIIOPortCRTC_H_SYNC_STRT_WID);
  save->crtc_v_total_disp = rage_inl(ATIIOPortCRTC_V_TOTAL_DISP);
  save->crtc_v_sync_strt_wid = rage_inl(ATIIOPortCRTC_V_SYNC_STRT_WID);
  save->crtc_off_pitch = rage_inl(ATIIOPortCRTC_OFF_PITCH);

  save->ovr_clr = rage_inl(ATIIOPortOVR_CLR);

  save->clock_cntl = rage_inl(ATIIOPortCLOCK_CNTL);

  save->bus_cntl = rage_inl(ATIIOPortBUS_CNTL);

  save->mem_vga_wp_sel = rage_inl(ATIIOPortMEM_VGA_WP_SEL);
  save->mem_vga_rp_sel = rage_inl(ATIIOPortMEM_VGA_RP_SEL);

  save->dac_cntl = rage_inl(ATIIOPortDAC_CNTL); /* internal DAC */

  save->config_cntl = rage_inl(ATIIOPortCONFIG_CNTL);

  save->mem_info = rage_inl(ATIIOPortMEM_INFO);

  save->crtc_int_cntl=rage_inl(ATIIOPortCRTC_INT_CNTL);
  save->crtc_gen_cntl=rage_inl(ATIIOPortCRTC_GEN_CNTL);
  save->gen_test_cntl=rage_inl(ATIIOPortGEN_TEST_CNTL);  

  if((ATIChip>=ATI_CHIP_264VTB)&&(ATIIODecoding==BLOCK_IO)) {
    save->dsp_on_off=rage_inl(ATIIOPortDSP_ON_OFF);
    save->dsp_config=rage_inl(ATIIOPortDSP_CONFIG);
  };

  ATIIOPortDAC_DATA = ATIIOPortDAC_REGS + 1;
  ATIIOPortDAC_MASK = ATIIOPortDAC_REGS + 2;
  ATIIOPortDAC_READ = ATIIOPortDAC_REGS + 3;
  ATIIOPortDAC_WRITE = ATIIOPortDAC_REGS + 0;
  save->dac_read = rage_inb(ATIIOPortDAC_READ);
  save->dac_write = rage_inb(ATIIOPortDAC_WRITE);
  save->dac_mask = rage_inb(ATIIOPortDAC_MASK);

  rage_outl(ATIIOPortCRTC_GEN_CNTL, save->crtc_gen_cntl | 0x1000000);
  
  switch(rage_clock){
     case -1: /* internal clock */
        for(i=6;i<12;i++)
           save->PLL[i]=ATIGetMach64PLLReg(i); /* internal Clock */
        break;
     case 4: /* CH8398 */        
        i=rage_inl(ATIIOPortDAC_CNTL);
        rage_outl(ATIIOPortDAC_CNTL,(i&~3)|3);
        rage_outb(ATIIOPortDAC_READ, ATIClockToProgram);
        save->PLL[0]=rage_inb(ATIIOPortDAC_DATA);
        save->PLL[1]=rage_inb(ATIIOPortDAC_DATA);
        rage_outl(ATIIOPortDAC_CNTL,i);        
        break;
  };

  rage_outb(ATIIOPortDAC_MASK, 0xFFU);
  rage_outb(ATIIOPortDAC_READ, 0x00U);

  for (i = 0;  i<DAC_SIZE;  i++)
      save->DAC[i] =rage_inb(ATIIOPortDAC_DATA);

  switch(rage_dac){
      case -1:
          save->cur_clr0=rage_inl(ATIIOPortCUR_CLR0);
          save->cur_clr1=rage_inl(ATIIOPortCUR_CLR1);
          save->cur_offset=rage_inl(ATIIOPortCUR_OFFSET);
          save->cur_horz_vert_posn=rage_inl(ATIIOPortCUR_HORZ_VERT_POSN);
          save->cur_horz_vert_off=rage_inl(ATIIOPortCUR_HORZ_VERT_OFF);
          break;
      case 4: /* CH8398 */
          i=rage_inl(ATIIOPortDAC_CNTL);
          rage_outl(ATIIOPortDAC_CNTL,(i&~3)|3);
          save->extdac[4]=rage_inb(ATIIOPortDAC_WRITE);
          save->extdac[5]=rage_inb(ATIIOPortDAC_DATA);
          save->extdac[7]=rage_inb(ATIIOPortDAC_READ);
          save->extdac[6]=rage_inb(ATIIOPortDAC_MASK); 
          rage_outl(ATIIOPortDAC_CNTL,i);          
          break;
       case 5:
          i=rage_inl(ATIIOPortDAC_CNTL);
          rage_outl(ATIIOPortDAC_CNTL,(i&~3)|2);
          save->extdac[8]=rage_inb(ATIIOPortDAC_WRITE);
          save->extdac[10]=rage_inb(ATIIOPortDAC_MASK);
          save->extdac[11]=rage_inb(ATIIOPortDAC_READ);
          rage_outl(ATIIOPortDAC_CNTL,(i&~3)|3);
          save->extdac[12]=rage_inb(ATIIOPortDAC_WRITE);
          rage_outl(ATIIOPortDAC_CNTL,i);
       break;
  };

  if(ATIChip<ATI_CHIP_264CT) 
	  for(i=6;i<7;i++) {
  		  rage_outb(0x1ce, 0xb0+i);
     	  save->atib[i]=rage_inb(0x1cf);
	  }

  rage_outl(ATIIOPortCRTC_GEN_CNTL, save->crtc_gen_cntl);

  return RAGE_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void rage_setregs(const unsigned char regs[], int mode)
{  
    ATIHWPtr restore;
    int Index;    
    int i;

    restore=(ATIHWPtr)(regs+VGA_TOTAL_REGS);

    rage_outl(ATIIOPortCRTC_GEN_CNTL, restore->crtc_gen_cntl & ~CRTC_EN );

    /* Load Mach64 CRTC registers */
    rage_outl(ATIIOPortCRTC_H_TOTAL_DISP, restore->crtc_h_total_disp);
    rage_outl(ATIIOPortCRTC_H_SYNC_STRT_WID, restore->crtc_h_sync_strt_wid);
    rage_outl(ATIIOPortCRTC_V_TOTAL_DISP, restore->crtc_v_total_disp);
    rage_outl(ATIIOPortCRTC_V_SYNC_STRT_WID, restore->crtc_v_sync_strt_wid);
    rage_outl(ATIIOPortCRTC_OFF_PITCH, restore->crtc_off_pitch);
    /* Set pixel clock */
    rage_outl(ATIIOPortCLOCK_CNTL, restore->clock_cntl);

    /* Load overscan registers */
    rage_outl(ATIIOPortOVR_CLR, restore->ovr_clr);

    /* Finalize CRTC setup and turn on the screen */
    rage_outl(ATIIOPortCRTC_GEN_CNTL, restore->crtc_gen_cntl);

    /* Aperture setup */
    rage_outl(ATIIOPortBUS_CNTL, restore->bus_cntl);

    rage_outl(ATIIOPortMEM_VGA_WP_SEL, restore->mem_vga_wp_sel);
    rage_outl(ATIIOPortMEM_VGA_RP_SEL, restore->mem_vga_rp_sel);

    rage_outl(ATIIOPortCONFIG_CNTL, restore->config_cntl);

    if((ATIChip>=ATI_CHIP_264VTB)&&(ATIIODecoding==BLOCK_IO)) {
       rage_outl(ATIIOPortDSP_ON_OFF, restore->dsp_on_off);
       rage_outl(ATIIOPortDSP_CONFIG, restore->dsp_config);
    };
    
    rage_outl(ATIIOPortMEM_INFO, restore->mem_info);
    rage_outl(ATIIOPortCRTC_INT_CNTL,restore->crtc_int_cntl);
    rage_outl(ATIIOPortCRTC_GEN_CNTL,restore->crtc_gen_cntl);
    rage_outl(ATIIOPortGEN_TEST_CNTL,restore->gen_test_cntl);

    rage_outl(ATIIOPortCRTC_GEN_CNTL, restore->crtc_gen_cntl | 0x1000000);

    switch(rage_clock){
       case -1: /* internal clock */
          i=ATIGetMach64PLLReg(PLL_VCLK_CNTL) | PLL_VCLK_RESET;
          ATIPutMach64PLLReg(PLL_VCLK_CNTL,i);
    
          ATIPutMach64PLLReg(PLL_VCLK_POST_DIV, restore->PLL[PLL_VCLK_POST_DIV]);
          ATIPutMach64PLLReg(PLL_XCLK_CNTL, restore->PLL[PLL_XCLK_CNTL]);
          ATIPutMach64PLLReg(PLL_VCLK0_FB_DIV+ATIClockToProgram, restore->PLL[PLL_VCLK0_FB_DIV+ATIClockToProgram]);
    
          i&= ~PLL_VCLK_RESET;
          ATIPutMach64PLLReg(PLL_VCLK_CNTL,i);
          break;
       case 4: /* CH8398 */
          i=rage_inl(ATIIOPortDAC_CNTL);
          rage_outl(ATIIOPortDAC_CNTL,(i&~3)|3);
          rage_outb(ATIIOPortDAC_WRITE, ATIClockToProgram);
          rage_outb(ATIIOPortDAC_DATA, restore->PLL[0]);
          rage_outb(ATIIOPortDAC_DATA, restore->PLL[1]);
          rage_outl(ATIIOPortDAC_CNTL,i);          
          break;
    };

    /* make sure the dac is in 8 bit or 6 bit mode, as needed */
    rage_outl(ATIIOPortDAC_CNTL, restore->dac_cntl); 
 
    rage_outb(ATIIOPortDAC_MASK, 0xFFU);
    rage_outb(ATIIOPortDAC_WRITE, 0x00U);
    for (Index = 0;  Index < DAC_SIZE;  Index++)
      rage_outb(ATIIOPortDAC_DATA, restore->DAC[Index]);

    switch(rage_dac){
      case -1:
          rage_outl(ATIIOPortCUR_CLR0, restore->cur_clr0);
          rage_outl(ATIIOPortCUR_CLR1, restore->cur_clr1);
          rage_outl(ATIIOPortCUR_OFFSET, restore->cur_offset);
          rage_outl(ATIIOPortCUR_HORZ_VERT_POSN, restore->cur_horz_vert_posn);
          rage_outl(ATIIOPortCUR_HORZ_VERT_OFF, restore->cur_horz_vert_off);
          break;
      case 4: /* CH8398 */
          i=rage_inl(ATIIOPortDAC_CNTL);
          rage_outl(ATIIOPortDAC_CNTL,(i&~3)|3);
          rage_outb(ATIIOPortDAC_MASK, restore->extdac[6]);
          rage_outl(ATIIOPortDAC_CNTL,i);          
          break;
       case 5:
          i=rage_inl(ATIIOPortDAC_CNTL);
          rage_outl(ATIIOPortDAC_CNTL,(i&0xfffffffc)|2);
          rage_outb(ATIIOPortDAC_WRITE,restore->extdac[8]);
          rage_outb(ATIIOPortDAC_MASK,restore->extdac[10]);
          rage_outb(ATIIOPortDAC_READ,restore->extdac[11]);
          rage_outl(ATIIOPortDAC_CNTL,(i&0xfffffffc)|3);
          rage_outb(ATIIOPortDAC_WRITE,(rage_inb(ATIIOPortDAC_WRITE)&0x80)|restore->extdac[12]);
       break;
    };

    rage_outb(ATIIOPortDAC_READ, restore->dac_read);
    rage_outb(ATIIOPortDAC_WRITE, restore->dac_write);
    rage_outl(ATIIOPortDAC_CNTL, restore->dac_cntl);
    
    if(ATIChip<ATI_CHIP_264CT)
		for(i=6;i<7;i++) {
    		rage_outb(0x1ce, 0xb0 + i);
       		rage_outb(0x1cf, restore->atib[i]);
    	}
    rage_outl(ATIIOPortCRTC_GEN_CNTL, restore->crtc_gen_cntl);

};
/* Return nonzero if mode is available */

static int rage_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (rage_memory * 1024 < info->ydim * info->xbytes)
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


static int comp_lmn(unsigned clock, int *n, int *mp, int *lp);
static int rage_map_clock(int bpp, int clock);
static int ClacDSP(int n, int l, int bpp, unsigned int *conf, unsigned int *onoff);
/* Local, called by rage_setmode(). */
static void rage_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
    int m,n,l,i;
    ATIHWPtr ATINewHWPtr;
    int 
        horizDisplay    = (modetiming->CrtcHDisplay/8)   - 1,
        horizStart      = (modetiming->CrtcHSyncStart/8) - 1,
        horizWidth      = (modetiming->CrtcHSyncEnd/8) - (modetiming->CrtcHSyncStart/8),
        horizTotal      = (modetiming->CrtcHTotal/8)	   - 1,
        vertDisplay     = modetiming->CrtcVDisplay -1,
        vertStart       = modetiming->CrtcVSyncStart -1,
        vertWidth       = modetiming->CrtcVSyncEnd - modetiming->CrtcVSyncStart,
        vertTotal       = modetiming->CrtcVTotal -1;
    
    ATINewHWPtr=(ATIHWPtr)(moderegs+VGA_TOTAL_REGS); 

    moderegs[GRA+0]=0;
    moderegs[GRA+1]=0;
    moderegs[GRA+2]=0;
    moderegs[GRA+3]=0;
    moderegs[GRA+4]=0;
    moderegs[GRA+5]=0x10;
    moderegs[GRA+6]=1;
    moderegs[GRA+7]=0;
    moderegs[GRA+8]=0xff;
    moderegs[SEQ+0]=0x3;
    moderegs[SEQ+1]=0x0;
    moderegs[SEQ+2]=0x0F;
    moderegs[SEQ+3]=0x0;
    moderegs[SEQ+4]=0x0A;
    moderegs[ATT+0x10]=0x0c;
    moderegs[ATT+0x11]=0x11;
    moderegs[ATT+0x12]=0xf;
    moderegs[ATT+0x13]=0x13;
    moderegs[ATT+0x14]=0;
    moderegs[VGA_MISCOUTPUT]=0x27;

    ATINewHWPtr->clock_cntl=ATIClockToProgram;

    ATINewHWPtr->crtc_int_cntl=(rage_inl(ATIIOPortCRTC_INT_CNTL) & ~CRTC_INT_ENS) 
       				| CRTC_INT_ACKS /*0x80000074 */;

    ATINewHWPtr->shared_cntl=0;
    ATINewHWPtr->gen_test_cntl=0;
    
    ATINewHWPtr->mem_info=rage_inl(ATIIOPortMEM_INFO);
    
    if(ATIChip<ATI_CHIP_264CT)
      ATINewHWPtr->mem_info &= ~(CTL_MEM_BNDRY | CTL_MEM_BNDRY_EN) ;

    ATINewHWPtr->PLL[PLL_VCLK_POST_DIV]=ATIGetMach64PLLReg(PLL_VCLK_POST_DIV);
    ATINewHWPtr->PLL[PLL_XCLK_CNTL]=ATIGetMach64PLLReg(PLL_XCLK_CNTL);

    for(i=0;i<256;i++)ATINewHWPtr->DAC[i*3]=ATINewHWPtr->DAC[i*3+1]=
                      ATINewHWPtr->DAC[i*3+2]=i;

    ATINewHWPtr->crtc_off_pitch=SetBits(modeinfo->width >> 3, CRTC_PITCH);
  
    ATINewHWPtr->bus_cntl = (rage_inl(ATIIOPortBUS_CNTL) &
        ~BUS_HOST_ERR_INT_EN) | BUS_HOST_ERR_INT;
    if (ATIChip < ATI_CHIP_264VTB)
        ATINewHWPtr->bus_cntl = (ATINewHWPtr->bus_cntl &
            ~(BUS_FIFO_ERR_INT_EN | BUS_ROM_DIS)) |
            (SetBits(15, BUS_FIFO_WS) | BUS_FIFO_ERR_INT);
    else
        ATINewHWPtr->bus_cntl |= BUS_APER_REG_DIS;

    ATINewHWPtr->dac_cntl=rage_inl(ATIIOPortDAC_CNTL);

    if (modeinfo->bitsPerPixel>8)
            ATINewHWPtr->dac_cntl |= DAC_8BIT_EN;
      else
            ATINewHWPtr->dac_cntl &= ~DAC_8BIT_EN;

    ATINewHWPtr->config_cntl= rage_inl(ATIIOPortCONFIG_CNTL) | CFG_MEM_VGA_AP_EN | 2;

    ATINewHWPtr->ovr_clr=0;

    if (!comp_lmn(rage_map_clock(modeinfo->bitsPerPixel, modetiming->pixelClock),&n,&m,&l))
       return;
    
    switch(rage_clock) {
       case 5:
             ATINewHWPtr->ics2595=n|(l<<9);
         break;
       case 4: /* CH8398 */
           for(i=0;i<16;i++)ATINewHWPtr->atib[i]=0;
           ATINewHWPtr->atib[6]=0x04;           
           ATINewHWPtr->PLL[0]=n;
           ATINewHWPtr->PLL[1]=m | (l<<6);
           ATINewHWPtr->clock_cntl = 0x40 + ATIClockToProgram;
           break;
       case -1:
            ATINewHWPtr->PLL[PLL_VCLK0_FB_DIV+ATIClockToProgram]=n;
            ATINewHWPtr->PLL[PLL_VCLK_POST_DIV]&=0xfc<<(ATIClockToProgram<<1);
            ATINewHWPtr->PLL[PLL_VCLK_POST_DIV]|=(l&3)<<(ATIClockToProgram<<1);
            ATINewHWPtr->PLL[PLL_XCLK_CNTL]&=~(0x10<<ATIClockToProgram);
            ATINewHWPtr->PLL[PLL_XCLK_CNTL]|=((l>>2)<<4)<<ATIClockToProgram;
    };

    if(horizWidth>0x1f) {
        horizWidth=0x1f;
    } else if(horizWidth<1) {
        horizWidth=1;
        if(horizStart>horizDisplay)horizStart++;
    }

    ATINewHWPtr->crtc_h_total_disp =
          SetBits(horizTotal, CRTC_H_TOTAL) |
          SetBits(horizDisplay, CRTC_H_DISP);
    ATINewHWPtr->crtc_h_sync_strt_wid =
          SetBits(horizStart, CRTC_H_SYNC_STRT) |
          SetBits(0, CRTC_H_SYNC_DLY) |     
          SetBits(GetBits(horizStart, 0x0100U),CRTC_H_SYNC_STRT_HI) |
          SetBits(horizWidth,CRTC_H_SYNC_WID);
    if (modetiming->flags & NHSYNC)
          ATINewHWPtr->crtc_h_sync_strt_wid |= CRTC_H_SYNC_POL;

    ATINewHWPtr->crtc_v_total_disp =
          SetBits(vertTotal, CRTC_V_TOTAL) |
          SetBits(vertDisplay, CRTC_V_DISP);
    ATINewHWPtr->crtc_v_sync_strt_wid =
          SetBits(vertStart, CRTC_V_SYNC_STRT) |
          SetBits(vertWidth,CRTC_V_SYNC_WID);
    if (modetiming->flags & NVSYNC)
          ATINewHWPtr->crtc_v_sync_strt_wid |= CRTC_V_SYNC_POL;

    ATINewHWPtr->crtc_gen_cntl = rage_inl(ATIIOPortCRTC_GEN_CNTL) &
            ~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
              CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_CSYNC_EN |
              CRTC_PIX_BY_2_EN | CRTC_DISPLAY_DIS | CRTC_VGA_XOVERSCAN |
              CRTC_PIX_WIDTH | CRTC_BYTE_PIX_ORDER | CRTC_FIFO_LWM |
              CRTC_VGA_128KAP_PAGING | CRTC_VFC_SYNC_TRISTATE |
              CRTC_LOCK_REGS |          
              CRTC_SYNC_TRISTATE | CRTC_DISP_REQ_EN |
              CRTC_VGA_TEXT_132 | CRTC_CUR_B_TEST);
    ATINewHWPtr->crtc_gen_cntl |= CRTC_EXT_DISP_EN | CRTC_EN |
          CRTC_VGA_LINEAR | CRTC_CNT_EN | CRTC_BYTE_PIX_ORDER;

    switch (modeinfo->bitsPerPixel)
        {
            case 1:
                ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_1BPP;
                break;
            case 4:
                ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_4BPP;
                break;
            case 8:
                ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_8BPP;
                break;
            case 16:
               	switch(modeinfo->colorBits)
                   {	
                        case 15:
                           ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_15BPP;
                           break;
                        case 16:
                           ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_16BPP;
                           break;
                   };
                break;
            case 24:
                ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_24BPP;
                break;
            case 32:
                ATINewHWPtr->crtc_gen_cntl |= CRTC_PIX_WIDTH_32BPP;
                break;
            default:
                break;
        }

    if (modetiming->flags & DOUBLESCAN)
            ATINewHWPtr->crtc_gen_cntl |= CRTC_DBL_SCAN_EN;
    if (modetiming->flags & INTERLACED)
            ATINewHWPtr->crtc_gen_cntl |= CRTC_INTERLACE_EN;

    switch(rage_dac){
       case 4: /* CH8398 */
          moderegs[VGA_MISCOUTPUT]=0x6f;
          switch(modeinfo->bitsPerPixel) {
             case 8: ATINewHWPtr->extdac[6]=0x04; break;
             case 16: 
                if(modeinfo->colorBits==15) ATINewHWPtr->extdac[6]=0x14; 
                   else ATINewHWPtr->extdac[6]=0x34; 
                break;
             case 24: ATINewHWPtr->extdac[6]=0xb4; break;
          };
          break;
       case 5: /* 68860/68880 */
          ATINewHWPtr->extdac[8]=2;
          ATINewHWPtr->extdac[10]=0x1d;
          switch(modeinfo->bitsPerPixel) {
             case 8: ATINewHWPtr->extdac[11]=0x83; break;
             case 15: ATINewHWPtr->extdac[11]=0xa0; break;
             case 16: ATINewHWPtr->extdac[11]=0xa1; break;
             case 24: ATINewHWPtr->extdac[11]=0xc0; break;
             case 32: ATINewHWPtr->extdac[11]=0xe3; break;
          };
          ATINewHWPtr->extdac[12]=0x60;
          if(modeinfo->bitsPerPixel==8)ATINewHWPtr->extdac[12]=0x61;
          if(rage_memory<1024)ATINewHWPtr->extdac[12]|=0x04;
          if(rage_memory==1024)ATINewHWPtr->extdac[12]|=0x08;
          if(rage_memory>1024)ATINewHWPtr->extdac[12]|=0x0c;
       break;       
    };

    rage_bpp = modeinfo->bytesPerPixel; 

    if((ATIChip>=ATI_CHIP_264VTB)&&(ATIIODecoding==BLOCK_IO))
        ClacDSP(n, l, modeinfo->bitsPerPixel, &ATINewHWPtr->dsp_config, &ATINewHWPtr->dsp_on_off);
    if (ATIChip < ATI_CHIP_264VTB)
        ATINewHWPtr->crtc_gen_cntl |= CRTC_FIFO_LWM;

}


static int rage_setmode(int mode, int prv_mode)
{

    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;
    ATIHWPtr ATINewHWPtr;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!rage_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));

    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(MAX_REGS);
    
    ATINewHWPtr=(ATIHWPtr)(moderegs+VGA_TOTAL_REGS); 

    rage_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);
    rage_setregs(moderegs, mode);	
    free(moderegs);

    free(modeinfo);

    return 0;
}


/* Unlock chipset-specific registers */

static void rage_unlock(void)
{
}

static void rage_lock(void)
{
}


/* Indentify chipset, initialize and return non-zero if detected */

static int rage_test(void)
{
   int i=0, found;
   unsigned long buf[64];
    
   found=__svgalib_pci_find_vendor_vga(0x1002,buf,0);
   if(found)return 0;
   
   if (rage_init(0,0,0) != 0)
       return 0;
   return 1;
   
   ATIIOBase=buf[5]&BLOCK_IO_BASE;
   
   if(ATIIOBase)
      {
         ATIIODecoding=BLOCK_IO;
         i=rage_probe();
      } else 
      {
         ATIIOBase=0x2EC; 	
         ATIIODecoding=SPARSE_IO;
         if(!(i=rage_probe())){
             ATIIOBase=0x1C8; 	
             if(!(i=rage_probe())){
                ATIIOBase=0x1CC;
                i=rage_probe();
             };
         };
      };

	  
    if(!i)return 0;  
   
    rage_init(0,0,0);
    return 1;
}

/* Set display start address (not for 16 color modes) */

static void rage_setdisplaystart(int address)
{ 
    unsigned int t;
    
    address>>=3;
    t=rage_inl(ATIIOPortCRTC_OFF_PITCH);
    rage_outl(ATIIOPortCRTC_OFF_PITCH,(t&~CRTC_OFFSET)|address);

}

static void rage_setlogicalwidth(int width)
{   
    unsigned int t;
    
    if(rage_bpp>0)width=width/rage_bpp;
    t=rage_inl(ATIIOPortCRTC_OFF_PITCH);
    rage_outl(ATIIOPortCRTC_OFF_PITCH,(t&~CRTC_PITCH)|((width>>3)<<22));

}

static int rage_linear(int op, int param)
{
if (op==LINEAR_ENABLE){ rage_is_linear=1; return 0;}
if (op==LINEAR_DISABLE){ rage_is_linear=0; return 0;}
if (op==LINEAR_QUERY_BASE) return rage_linear_base;
if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
    else return -1;		/* Unknown function. */
}

static int rage_match_programmable_clock(int clock)
{
return clock ;
}

static int rage_map_clock(int bpp, int clock)
{
   if((bpp==24)&&(rage_clock==4))clock += (clock>>1);
   return clock ;
}

static int rage_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static int pal=1, palette[768];

static int findcolor(int rgb) {
   int i,j,k,l=0;

   if(pal)vga_getpalvec(0,256,palette);
   pal=0;
   k=0xffffff;
   for(i=0;i<256;i++) {
      j=((rgb&0xff)-(palette[i*3+2]<<2))*((rgb&0xff)-(palette[i*3+2]<<2))+
        (((rgb>>8)&0xff)-(palette[i*3+1]<<2))*(((rgb>>8)&0xff)-(palette[i*3+1]<<2))+
        (((rgb>>16)&0xff)-(palette[i*3]<<2))*(((rgb>>16)&0xff)-(palette[i*3]<<2));
      if(j==0) {
         return i;
      }
      if(j<k) {
         k=j;
         l=i;
      }
   }
   return l;
}

static unsigned int cur_colors[16*2];

static int rage_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j, t;
    unsigned long *b3;
    unsigned long l1, l2, k, c, l;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            t=rage_inl(ATIIOPortGEN_TEST_CNTL);
            rage_outl(ATIIOPortGEN_TEST_CNTL,t&~GEN_CUR_EN);
            break;
        case CURSOR_SHOW:
            t=rage_inl(ATIIOPortGEN_TEST_CNTL);
            rage_outl(ATIIOPortGEN_TEST_CNTL,t|GEN_CUR_EN);
            break;
        case CURSOR_POSITION:
            rage_outl(ATIIOPortCUR_HORZ_VERT_POSN,p1|(p2<<16));
            rage_outl(ATIIOPortCUR_HORZ_VERT_OFF,0);
        break;
        case CURSOR_SELECT:
            i=rage_memory*1024-(p1+1)*4096;
            rage_outl(ATIIOPortCUR_OFFSET,i>>3);
            rage_outl(ATIIOPortCUR_CLR0,cur_colors[p1*2]);
            rage_outl(ATIIOPortCUR_CLR1,cur_colors[p1*2+1]);
            break;
        case CURSOR_IMAGE:
            i=rage_memory*1024-(p1+1)*4096;
            b3=(unsigned long *)p5;
            switch(p2) {
                case 0:
                    cur_colors[p1*2]=p3<<8 | findcolor(p3);
                    cur_colors[p1*2+1]=p4<<8 | findcolor(p4);
                    for(j=0;j<32;j++) {
                        l1=*(b3+j);
                        l2=~(*(b3+32+j));
                        for(k=0;k<8;k++) {
                            c=0;
                            for(l=0;l<4;l++) {
                                c>>=1;
                                c|=(l1>>31)<<7;
                                c>>=1;
                                c|=(l2>>31)<<7;
                                if(c&128)c&=0xbf;
                                l1<<=1;
                                l2<<=1;
                            }
                            *(LINEAR_POINTER+i+16*j+k)=c;
                            *(LINEAR_POINTER+i+16*j+k+8)=0xaa;
                        }
                    }
                    memset(LINEAR_POINTER+i+512,0xaa,512);
                break;
            }
            break;
    }
    return 0;
}       

/* Function table (exported) */
DriverSpecs __svgalib_rage_driverspecs =
{
    rage_saveregs,
    rage_setregs,
    rage_unlock,
    rage_lock,
    rage_test,
    rage_init,
    rage_setpage,
    rage_setrdpage,
    rage_setwrpage,
    rage_setmode,
    rage_modeavailable,
    rage_setdisplaystart,
    rage_setlogicalwidth,
    rage_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    rage_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    rage_cursor
};

/* Initialize chipset (called after detection) */

static int rage_init(int force, int par1, int par2)
{
   unsigned long buf[64];
   int found=0;
   int i,j;
   unsigned char *BIOS;

   static int videoRamSizes[] = { 0 , 256 , 512 , 1024, 2*1024 ,
        	4*1024 , 6*1024 , 8*1024 , 12*1024 , 16*1024 , 0 };
   
   rage_unlock();
   if (force) {
		rage_memory = par1;
        rage_chiptyperev = par2;
   } else {

   };

   found=__svgalib_pci_find_vendor_vga(0x1002,buf,0);
   rage_linear_base=0;
   if (!found){
      rage_linear_base=buf[4]&0xffffff00ul;
#if 0
	  // This uses IO
	   ATIIOBase=buf[5]&BLOCK_IO_BASE;
#else
	  // This uses MMIO
	  __svgalib_mmio_base=buf[6]&0xffffff00ul;
	  if(__svgalib_mmio_base==0) {
	  		__svgalib_mmio_base=rage_linear_base+0x007ff000;
			ATIIOBase=0xc00;
	  } else {
	  		ATIIOBase=1024;
	  }
	  __svgalib_mmio_size=4096;
	  map_mmio();	  
#endif
   };

   if(ATIIOBase)
      {
         ATIIODecoding=BLOCK_IO;
         i=rage_probe();
      } else 
      {
         ATIIOBase=0x2EC; 	
         ATIIODecoding=SPARSE_IO;
         if(!(i=rage_probe())){
             ATIIOBase=0x1C8; 	
             if(!(i=rage_probe())){
                ATIIOBase=0x1CC;
                i=rage_probe();
             };
         };
      };

   if(found || !i){
      return -1;
   };

   rage_chiptyperev=(buf[0]&0xffff0000) | (buf[2]&0xff);

   rage_ChipID();

   ATIIOPortCRTC_H_TOTAL_DISP=ATIIOPort(CRTC_H_TOTAL_DISP);
   ATIIOPortCRTC_H_SYNC_STRT_WID=ATIIOPort(CRTC_H_SYNC_STRT_WID);
   ATIIOPortCRTC_V_TOTAL_DISP=ATIIOPort(CRTC_V_TOTAL_DISP);
   ATIIOPortCRTC_V_SYNC_STRT_WID=ATIIOPort(CRTC_V_SYNC_STRT_WID);
   ATIIOPortCRTC_OFF_PITCH=ATIIOPort(CRTC_OFF_PITCH);
   ATIIOPortCRTC_INT_CNTL=ATIIOPort(CRTC_INT_CNTL);
   ATIIOPortCRTC_GEN_CNTL=ATIIOPort(CRTC_GEN_CNTL);
   ATIIOPortDSP_CONFIG=ATIIOPort(DSP_CONFIG);
   ATIIOPortDSP_ON_OFF=ATIIOPort(DSP_ON_OFF);
   ATIIOPortOVR_CLR=ATIIOPort(OVR_CLR);
   ATIIOPortCLOCK_CNTL=ATIIOPort(CLOCK_CNTL);
   ATIIOPortBUS_CNTL=ATIIOPort(BUS_CNTL);
   ATIIOPortMEM_INFO=ATIIOPort(MEM_INFO);
   ATIIOPortMEM_VGA_WP_SEL=ATIIOPort(MEM_VGA_WP_SEL);
   ATIIOPortMEM_VGA_RP_SEL=ATIIOPort(MEM_VGA_RP_SEL);
   ATIIOPortDAC_REGS=ATIIOPort(DAC_REGS);
   ATIIOPortDAC_CNTL=ATIIOPort(DAC_CNTL);
   ATIIOPortGEN_TEST_CNTL=ATIIOPort(GEN_TEST_CNTL);
   ATIIOPortCONFIG_CNTL=ATIIOPort(CONFIG_CNTL);
   ATIIOPortCONFIG_STATUS64_0=ATIIOPort(CONFIG_STATUS64_0);
   ATIIOPortCUR_HORZ_VERT_OFF=ATIIOPort(CUR_HORZ_VERT_OFF);
   ATIIOPortCUR_HORZ_VERT_POSN=ATIIOPort(CUR_HORZ_VERT_POSN);
   ATIIOPortCUR_CLR0=ATIIOPort(CUR_CLR0);
   ATIIOPortCUR_CLR1=ATIIOPort(CUR_CLR1);
   ATIIOPortCUR_OFFSET=ATIIOPort(CUR_OFFSET);
   i = rage_inl(ATIIOPort(MEM_INFO));
   j = rage_inl(ATIIOPort(CONFIG_STATUS64_0));
   if(ATIChip<ATI_CHIP_264CT) {
     ATIMemoryType=GetBits(j,CFG_MEM_TYPE);
   } else {
     ATIMemoryType=GetBits(j,CFG_MEM_TYPE_T);
   };
   if(ATIChip>=ATI_CHIP_264VTB) {
     i = GetBits(i, CTL_MEM_SIZEB);
     if (i < 8) rage_memory = (i + 1) * 512;
       else if (i < 12) rage_memory = (i - 3) * 1024;
         else rage_memory = (i - 7) * 2048;
   } else {
     i = GetBits(i, CTL_MEM_SIZE);
     rage_memory = videoRamSizes[i+2];
   };

   rage_mapio();

   if (__svgalib_driver_report) {
        fprintf(stderr,"Using RAGE driver, %iKB.   ChipID:%i MemType:%i\n",rage_memory,ATIChip,ATIMemoryType);
        if(rage_dac)fprintf(stderr,"Using external DAC:%i\n",rage_dac);
   }
   if(ATIChip>=ATI_CHIP_264GTPRO) /* enable offset double buffering */
   		rage_outl(ATIIOPort(HW_DEBUG), rage_inl(ATIIOPort(HW_DEBUG))|0x400);

   cardspecs = malloc(sizeof(CardSpecs));
   cardspecs->videoMemory = rage_memory;
   switch(ATIChip){
      case ATI_CHIP_264GTPRO:
      case ATI_CHIP_264LTPRO:
         cardspecs->maxPixelClock8bpp = 230000;	
         cardspecs->maxPixelClock16bpp = 230000;	
         cardspecs->maxPixelClock24bpp = 230000;
         cardspecs->maxPixelClock32bpp = 230000;
         break;
      case ATI_CHIP_264GTDVD:
      case ATI_CHIP_264LT:
      case ATI_CHIP_264VT4:
      case ATI_CHIP_264GT2C:
      case ATI_CHIP_264VT3:
         cardspecs->maxPixelClock8bpp = 200000;	
         cardspecs->maxPixelClock16bpp = 200000;	
         cardspecs->maxPixelClock24bpp = 200000;
         cardspecs->maxPixelClock32bpp = 200000;
         break;
      case ATI_CHIP_264VTB:
      case ATI_CHIP_264GTB:
         cardspecs->maxPixelClock8bpp = 170000;	
         cardspecs->maxPixelClock16bpp = 170000;	
         cardspecs->maxPixelClock24bpp = 170000;
         cardspecs->maxPixelClock32bpp = 170000;
         break;
      default:
         cardspecs->maxPixelClock8bpp = 135000;	
         cardspecs->maxPixelClock16bpp =135000;	
         cardspecs->maxPixelClock24bpp = 60000;
         cardspecs->maxPixelClock32bpp = 60000;
   };
   __svgalib_modeinfo_linearset |= IS_LINEAR;
   cardspecs->flags = CLOCK_PROGRAMMABLE;
   cardspecs->maxHorizontalCrtc = 4088;
   cardspecs->nClocks = 0;
   cardspecs->maxPixelClock4bpp = 0;
   cardspecs->mapClock = rage_map_clock;
   cardspecs->mapHorizontalCrtc = rage_map_horizontal_crtc;
   cardspecs->matchProgrammableClock=rage_match_programmable_clock;
   __svgalib_driverspecs = &__svgalib_rage_driverspecs;
   __svgalib_banked_mem_base=0xa0000;
   __svgalib_banked_mem_size=0x10000;
   __svgalib_linear_mem_base=rage_linear_base;
   __svgalib_linear_mem_size=rage_memory*0x400;

#define BIOSWord(x) ((int)BIOS[x]+256*(int)BIOS[x+1])

#if 0
   if(biosparams) {
	  fref=biosparam[1];
	  ATIClockToProgram=biosparam[0];
	  rage_dac = -1;
	  rage_clock = -1;
	  maxM=minM=ATIGetMach64PLLReg(PLL_REF_DIV);
	  minN=2;
	  maxN=255;
	  Nadj=0;
	  Madj=0;
	  fref*=2; /* X says double for all chips */
   } else
#endif
	{
#if 0
	   BIOS=mmap(0,32*1024,PROT_READ|PROT_WRITE,MAP_SHARED,__svgalib_mem_fd,0xe4100000);
#else
	   BIOS=mmap(0,32*1024,PROT_READ|PROT_WRITE,MAP_SHARED,__svgalib_mem_fd,0xc0000);
#endif

	   i=BIOSWord(0x48);
	   j=BIOSWord(i+0x10);

	   ATIClockToProgram=BIOS[j+6];

	   if(ATIChip>=ATI_CHIP_264CT){
		  rage_dac = -1;
		  rage_clock = -1;
		  maxM=minM=ATIGetMach64PLLReg(PLL_REF_DIV);
		  minN=2;
		  maxN=255;
		  switch (BIOSWord(j+0x08)/10) {
			 case 143:
				fref=14318; 
				break;
			 case 286:
				fref=28636;
				break;
			 default:
				fref=BIOSWord(j+0x08)*10;
				break;
		  }
		  Nadj=0;
		  Madj=0;
		  fref*=2; /* X says double for all chips */
		  
		  if(__svgalib_ragedoubleclock)fref/=2; /* just in case */
		  if (__svgalib_driver_report) {
			  fprintf(stderr,"Rage: BIOS reports base frequency=%.3fMHz  Denominator=%3i\n",
					  fref/1000,maxM);
		  }
	   } else {
		  rage_dac=(j>>9)&7;
		  switch(rage_dac) {
			  case 4:
				  rage_clock=4;
				  Nadj=8;
				  minN=64;
				  maxN=263;
				  maxM=34;
				  minM=23;
				  Madj=2;
				  for(i=0;i<3;i++)postdiv[i]=1<<i;
				  for(i=3;i<8;i++)postdiv[i]=0;
				  fref=14318;
				  cardspecs->maxPixelClock32bpp = 0;
				  break;
			  case 5:
				  rage_clock=5;
				  maxM=minM=46;
				  Madj=0;
				  minN=257;
				  maxN=512;
				  Nadj=257;
				  for(i=0;i<4;i++)postdiv[i]=1<<i;
				  for(i=4;i<8;i++)postdiv[i]=0;
				  fref=14318;
				  break;
		  };
		  if (__svgalib_driver_report) {
			  fprintf(stderr,"Rage: BIOS reports RamDAC=%i\n",rage_dac);
		  }
	   };

	   munmap(BIOS,32768);
	}

   return 0;
}

#define WITHIN(v,c1,c2) (((v) >= (c1)) && ((v) <= (c2)))

static int
comp_lmn(unsigned clock, int *np, int *mp, int *lp)
{
  int     n, m, l;
  double  fvco;
  double  fout;
  double  fvco_goal;

  for (m = minM; m <= maxM; m++)
  {
    for (l = 0;(l < 8); l++) if(postdiv[l])
    {
      for (n = minN; n <= maxN; n++)
      {
        fout = ((double)(n) * fref)/((double)(m) * (postdiv[l]));
        fvco_goal = (double)clock * (double)(postdiv[l]);
        fvco = fout * (double)(postdiv[l]);
        if (!WITHIN(fvco, 0.995*fvco_goal, 1.005*fvco_goal))
          continue;
        *lp=l;
        *np=n-Nadj;
        *mp=m-Madj;
        return 1 ;
      }
    }
  }

  return 0;
}

static int ClacDSP(int n, int l, int bpp, unsigned int *conf, unsigned int *onoff)
{
    int ATIDisplayFIFODepth;
    int Multiplier, Divider, vshift, xshift;
    int dsp_precision, dsp_on, dsp_off, dsp_xclks;
    unsigned int dsp_config, dsp_on_off, vga_dsp_config, vga_dsp_on_off;
    unsigned int tmp;
    int ATIXCLKFeedbackDivider,
        ATIXCLKReferenceDivider,
        ATIXCLKPostDivider;

    int ATIXCLKMaxRASDelay,
        ATIXCLKPageFaultDelay,
        ATIDisplayLoopLatency;

    int IO_Value;

    IO_Value = ATIGetMach64PLLReg(PLL_XCLK_CNTL);
    ATIXCLKPostDivider = GetBits(IO_Value, PLL_XCLK_SRC_SEL);
    ATIXCLKReferenceDivider = 1 ;
    switch (ATIXCLKPostDivider)
    {
        case 0: case 1: case 2: case 3:
            break;

        case 4:
            ATIXCLKReferenceDivider = 3;
            ATIXCLKPostDivider = 0;
            break;

        default:
            return -1;
    }

    ATIXCLKPostDivider -= GetBits(IO_Value, PLL_MFB_TIMES_4_2B);
    ATIXCLKFeedbackDivider = ATIGetMach64PLLReg(PLL_MCLK_FB_DIV);

    IO_Value = rage_inl(ATIIOPortMEM_INFO);
    tmp = GetBits(IO_Value, CTL_MEM_TRP);
    ATIXCLKPageFaultDelay = GetBits(IO_Value, CTL_MEM_TRCD) +
        GetBits(IO_Value, CTL_MEM_TCRD) + tmp + 2;
    ATIXCLKMaxRASDelay = GetBits(IO_Value, CTL_MEM_TRAS) + tmp + 2;
    ATIDisplayFIFODepth = 32;

    if ( ATIChip < ATI_CHIP_264VT4 )
    {
        ATIXCLKPageFaultDelay += 2;
        ATIXCLKMaxRASDelay += 3;
        ATIDisplayFIFODepth = 24;
    }
        

    switch (ATIMemoryType)
    {
        case MEM_264_DRAM:
            if (rage_memory <= 1024)
                ATIDisplayLoopLatency = 10;
            else
            {
                ATIDisplayLoopLatency = 8;
                ATIXCLKPageFaultDelay += 2;
            }
            break;

        case MEM_264_EDO:
        case MEM_264_PSEUDO_EDO:
            if (rage_memory <= 1024)
                ATIDisplayLoopLatency = 9;
            else
            {
                ATIDisplayLoopLatency = 8;
                ATIXCLKPageFaultDelay++;
            }
            break;

        case MEM_264_SDRAM:
            if (rage_memory <= 1024)
                ATIDisplayLoopLatency = 11;
            else
            {
                ATIDisplayLoopLatency = 10;
                ATIXCLKPageFaultDelay++;
            }
            break;

        case MEM_264_SGRAM:
            ATIDisplayLoopLatency = 8;
            ATIXCLKPageFaultDelay += 3;
            break;

        default:                
            ATIDisplayLoopLatency = 11;
            ATIXCLKPageFaultDelay += 3;
            break;
    }

    if (ATIXCLKMaxRASDelay <= ATIXCLKPageFaultDelay)
        ATIXCLKMaxRASDelay = ATIXCLKPageFaultDelay + 1;

    /* Allow BIOS to override */
    dsp_config = rage_inl(ATIIOPortDSP_CONFIG);
    dsp_on_off = rage_inl(ATIIOPortDSP_ON_OFF);
    vga_dsp_config = rage_inl(ATIIOPort(VGA_DSP_CONFIG));
    vga_dsp_on_off = rage_inl(ATIIOPort(VGA_DSP_ON_OFF));

    if (dsp_config)
        ATIDisplayLoopLatency = GetBits(dsp_config, DSP_LOOP_LATENCY);

    if ((!dsp_on_off && (ATIChip < ATI_CHIP_264GTPRO)) ||
        ((dsp_on_off == vga_dsp_on_off) &&
         (!dsp_config || !((dsp_config ^ vga_dsp_config) & DSP_XCLKS_PER_QW))))
    {
        if (ATIDivide(GetBits(vga_dsp_on_off, VGA_DSP_OFF),
                      GetBits(vga_dsp_config, VGA_DSP_XCLKS_PER_QW), 5, 1) > 24)
            ATIDisplayFIFODepth = 32;
        else
            ATIDisplayFIFODepth = 24;
    }

#   define Maximum_DSP_PRECISION ((int)GetBits(DSP_PRECISION, DSP_PRECISION))

    Multiplier = ATIXCLKFeedbackDivider * postdiv[l];
    Divider = n * ATIXCLKReferenceDivider;

    Divider *= bpp >> 2;

    vshift = (6 - 2) - ATIXCLKPostDivider;

    tmp = ATIDivide(Multiplier * ATIDisplayFIFODepth, Divider, vshift, 1);
    for (dsp_precision = -5;  tmp;  dsp_precision++)
        tmp >>= 1;
    if (dsp_precision < 0)
        dsp_precision = 0;
    else if (dsp_precision > Maximum_DSP_PRECISION)
        dsp_precision = Maximum_DSP_PRECISION;

    xshift = 6 - dsp_precision;
    vshift += xshift;

    dsp_off = ATIDivide(Multiplier * (ATIDisplayFIFODepth - 1), Divider, vshift, -1) - 
              ATIDivide(1, 1, vshift - xshift, 1);

    dsp_on = ATIDivide(Multiplier, Divider, vshift, 1);
    tmp = ATIDivide(ATIXCLKMaxRASDelay, 1, xshift, 1);
    if (dsp_on < tmp)
        dsp_on = tmp;
    dsp_on += (tmp*2) + ATIDivide(ATIXCLKPageFaultDelay, 1, xshift, 1);


    /* Calculate rounding factor and apply it to dsp_on */
    tmp = ((1 << (Maximum_DSP_PRECISION - dsp_precision)) - 1) >> 1;
    dsp_on = ((dsp_on + tmp) / (tmp + 1)) * (tmp + 1);

    if (dsp_on >= ((dsp_off / (tmp + 1)) * (tmp + 1))) {
        dsp_on = dsp_off - ATIDivide(Multiplier, Divider, vshift, -1);
        dsp_on = (dsp_on / (tmp + 1)) * (tmp + 1);
    }

    /* Last but not least:  dsp_xclks */
    dsp_xclks = ATIDivide(Multiplier, Divider, vshift + 5, 1);

    *onoff = SetBits(dsp_on, DSP_ON) | SetBits(dsp_off, DSP_OFF);
    *conf  = SetBits(dsp_precision, DSP_PRECISION) |
                     SetBits(dsp_xclks, DSP_XCLKS_PER_QW) |
                     SetBits(ATIDisplayLoopLatency, DSP_LOOP_LATENCY);

    return 0;
}

