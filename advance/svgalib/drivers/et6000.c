                       /*  ET6000 driver  Don Secrest  version 1.4         */
                       /*  version using svgalib_pci_find Feb 10, 2001     */
#include <stdlib.h>    /*  Testing modeline update. Works for linux-2.2.15 */
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "interfac.h"
#include "vgaregs.h"
#include "vgapci.h"

#ifdef DB6K
#include <time.h>
FILE *outf;
static int compareregs(void);
static int textset = 0;
static void prtmodeinfo(vga_modeinfo *m);
/* static char *not_written = "ET6000 subroutine %s is not written yet.\n"; */
static unsigned char *textstore = 0;
static unsigned char *vgatextstore = 0;
void fdumpregs(FILE *fd,unsigned char regs[],int n);
/* #define DNOTW(a) fprintf(outf,not_written,a) */
#define DPRT(a) fprintf(outf,a)
#else
/* #define DNOTW(a) */
#define DPRT(a)
#endif  /* DB6K */

#ifdef DBG  /* DBG is for debugging the code to use modelines. */
FILE *fdm;
#endif

#define SEG_SELECT 0x3CD

static unsigned long base1;
/* The following are set by init so that they may be restored to their  */
/* state in setmode.                                                    */
static unsigned char pci40std,pci42std,pci57std,pci58std,pci59std=0;

#define LMODEMIN 9    /* No mode below this has been tested for linear    */
#define LMODEMAX 146  /* No mode above this tested.  These should change. */

static long et6000_mapped_mem = 0;

static CardSpecs *cardspecs;
static int et6ksav = VGA_MISCOUTPUT + 1;
static int ET6000_TOTAL_REGS = VGA_MISCOUTPUT + 14;
static char mem_type;
static int et6000_memory;
static int et6000_linear_base;
static int et6000_interlaced(int mode); 
static unsigned comp_lmn(unsigned clock,float fac);
static int et6000_chiptype = 0; 
static int x3_4,x3_5,x3_8 = 0,x3_a; /* On init these get set according */
				    /*  to color */
union longword {
  unsigned char b[4];
  unsigned short w[2];
  unsigned long l;
};

static unsigned int cursor_colors[16*2];
static unsigned int *cursors[16];
static int cursor_loaded[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int cursor_format[16];
static int active_cursor = -1;
static int cursor_doublescan = 0; /* Used by et6000_cursor */

/* The following are in the order of apearance in struct DriverSpecs as are */
/* the program entries.                                                     */

static int  et6000_saveregs(unsigned char regs[]);
static void et6000_setregs(const unsigned char regs[], int mode);
static void et6000_unlock(void);
static void et6000_lock(void);
static int  et6000_test(void);
static int  et6000_init(int force, int par1, int par2);
static void et6000_setpage(int page);
static void et6000_setrdpage(int page);
static void et6000_setwrpage(int page);
static int  et6000_setmode(int mode, int prv_mode);
static int  et6000_modeavailable(int mode);
static void et6000_setdisplaystart(int address);
static void et6000_setlogicalwidth(int width);
static void et6000_getmodeinfo(int mode,vga_modeinfo *modeinfo);
/* Obsolete blit functions left out here. */
/* static int  et6000_ext_set(unsigned what, va_list params); */
/* static int  et6000_accel(unsigned operation, va_list params); */
static int  et6000_linear(int op, int param);
/* Two empty spaces here */
static int  et6000_cursor(int cmd,int p1,int p2,int p3,int p4,void *p5);

/*
** End of the DriverSpecs function declarations         
*/

/* Function table (exported) */

DriverSpecs __svgalib_et6000_driverspecs =
{
    et6000_saveregs,
    et6000_setregs,
    et6000_unlock,
    et6000_lock,
    et6000_test,
    et6000_init,
    et6000_setpage,
    et6000_setrdpage,
    et6000_setwrpage,
    et6000_setmode,
    et6000_modeavailable,
    et6000_setdisplaystart,
    et6000_setlogicalwidth,
    et6000_getmodeinfo,
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    NULL,                       /* et6000_ext_set */
    NULL,                       /* et6000_accel */
    et6000_linear,
    NULL,			/* accelspecs */
    NULL,			/* emulation */
    et6000_cursor,              /* cursor */
};
#ifdef DBG
/* programs to debug the modeline stuff. */
static void pmodeinfo(ModeInfo *In)
{
  fprintf(fdm,"Modeinfo\nWidth %d  height %d\n",In->width,In->height);
  fprintf(fdm,"Bpp %d,  bpp %d,  cB %d\n",In->bytesPerPixel,In->bitsPerPixel,
	 In->colorBits);
  fprintf(fdm,"RGB %d,  %d,  %d\n",In->redWeight,In->greenWeight,
	  In->blueWeight);
  fprintf(fdm,"Offset RGB %d,  %d,  %d\n",In->redOffset,In->greenOffset,
	  In->blueOffset);
  fprintf(fdm,"Line width %d,  real width %d,  real height %d\n",In->lineWidth,
	  In->realWidth,In->realHeight);
}
static void pmodetiming(ModeTiming *Tg)
{
  int i,*mt;
  mt = (int *) Tg;
  fprintf(fdm,"Mode timing\n");
  for(i = 0;i < 20;i++)
    {
      fprintf(fdm,"0x%04x  ",*(mt + i));
	if((i+1)%5 == 0) fprintf(fdm,"\n");
    }
}
#endif  /* Modeline routine debug. */

/* Called from setmode */
static void et6000_initializemode(unsigned char *moderegs, 
				  ModeTiming *modetiming,ModeInfo *modeinfo,
				  int mode)
{
  int tmp,tmptot,tmpss,tmpse,tmpbs,tmpbe,offset,i;
  float fac;

  fac = 1.0;
  __svgalib_setup_VGA_registers(moderegs,modetiming,modeinfo);
  offset = modeinfo->lineWidth >>3;
  if(modetiming->flags & DOUBLESCAN)
    moderegs[VGA_CR17] = 0xa3;
  else
    moderegs[VGA_CR17] = 0xab;
  if((modeinfo->bytesPerPixel == 2) || (modetiming->flags & DOUBLESCAN))
      moderegs[VGA_CR14] = 0x40;
  else
      moderegs[VGA_CR14] = 0x60;
  moderegs[VGA_CR13] = offset & 0xff;
  moderegs[VGA_AR10] = 0x01;     /* This is correct. May 20 1999 */
  moderegs[VGA_AR11] = 0x00;
  for(i = 0;i < 13;i++)
    moderegs[et6ksav + i] = 0x00;

  tmp    = (modetiming->CrtcHDisplay >> 3) - 1;
  tmptot = (modetiming->CrtcHTotal >> 3) -5;
  tmpss  = (modetiming->CrtcHSyncStart >> 3);
  tmpbs  = (modetiming->CrtcHSyncStart >> 3) - 1;
  tmpse  = offset;                                 /*    3F      */
  moderegs[et6ksav+8] = ((tmptot & 0x100) >> 8) |  /* 0 Htot (8) */
                        ((tmp  &  0x100)  >> 7) |  /* 1 HDsp (8) */
                        ((tmpbs &  0x100) >> 6) |  /* 2 HBs  (8) */
                        ((tmpss &  0x100) >> 4) |  /* 4 HSs  (8) */
                        ((tmpse &  0x200) >> 3) |  /* 6 offs (9) */
                        ((tmpse &  0x100) >> 1);   /* 7 ofss (8) */
  tmp    = modetiming->CrtcVDisplay -1;
  tmptot = modetiming->CrtcVTotal - 2;
  tmpbs  = modetiming->CrtcVSyncStart -1;
  tmpss  = modetiming->CrtcVSyncStart;
  tmpbe  = 0x00;  /* (10) */                        /*     35      */
  moderegs[et6ksav+5] = ((tmpbs & 0x400)  >> 10) |   /* 0 VBs  (10) */
                        ((tmptot & 0x400) >> 9 ) |   /* 1 Vtot (10) */
                        ((tmp   &  0x400) >> 8 ) |   /* 2 VDsp (10) */
                        ((tmpss &  0x400) >> 7 ) |   /* 3 Vsst (10) */
                        ( tmpbe >> 6);               /* 4 splt (10) */
  if(modetiming->flags & INTERLACED)
    moderegs[et6ksav + 5] |= 0xa0;  /* bit 7 of cr35 and bit 1 of 57 */

  switch(modeinfo->bitsPerPixel)
  {
  case 8:
    moderegs[et6ksav + 12] = 0x00;        /* et6ksav   Reg   */
    moderegs[et6ksav +  6] = 0x04;        /*    0     PCI 58 */
    moderegs[et6ksav] &= 0xfe;            /*    1     PCI 42 */
    break;                                /*    2     CR18   */
  case 15:                                /*    3     CR33   */
  case 16:                                /*    4     CR34   */
    moderegs[et6ksav + 12] = 0x90;        /*    5     CR35   */
    moderegs[et6ksav +  6] = 0x08;        /*    6     PCI 59 */
    if(modeinfo->greenWeight == 5)        /*    7     AR17   */
      moderegs[et6ksav] = 0x00;           /*    8     CR3F   */
    else                                  /*    9     PCI 40 */
      moderegs[et6ksav] = 0x02;           /*   10     PCI 13 */
    break;                                /*   11     3CD    */
  case 24:                                /*   12     AR16   */
    moderegs[et6ksav + 12] = 0xa0;
    moderegs[et6ksav  + 9] = 0x06;
    moderegs[et6ksav  + 6] = 0x02;
    moderegs[et6ksav  + 2] = 0xff;
    moderegs[et6ksav  + 1] = 0x02;
    fac = 3.0;
    break;
  case 32:
    moderegs[et6ksav + 12] = 0xb0;
    moderegs[et6ksav  + 9] = 0x06;
    moderegs[et6ksav +  6] = 0x02;
    moderegs[et6ksav  + 2] = 0xff;
    moderegs[et6ksav +  1] = 0x03;
    fac = 4.0;
    break;
  default:
    moderegs[et6ksav + 12] = 0x00;
    moderegs[et6ksav +  6] = 0x04;
    moderegs[et6ksav] &= 0xfe;
    moderegs[VGA_GR5] = 0x00;  /* 16 color mode not 0x02 */
    break;
  }

  modetiming->selectedClockNo=6; /* use clock 6 */

  if(modetiming->selectedClockNo > 3)
    moderegs[et6ksav + 4] |= 2;
  moderegs[VGA_MISCOUTPUT] = (moderegs[VGA_MISCOUTPUT] & 0xf3) |
                             ((modetiming->selectedClockNo & 0x03) << 2);
   tmp=comp_lmn(modetiming->pixelClock,fac);
   port_out_r(base1 | 0x68,6);    
   port_out_r(base1 | 0x69,tmp&0xff);
   port_out_r(base1 | 0x69,tmp>>8);
  /* because timing is giving the wrong */
  /* clock. FIX THIS WHEN CLOCK IS OK.  */
}

static int  et6000_saveregs(unsigned char regs[]) /* Called frm vga_saveregs */
{
  int i;

  et6000_unlock();
#ifdef DB6K
  { 
    fprintf(outf,"In saveregs, regs at %x\n",(int) regs);
  }
#endif
  /* Save extended CRT registers. */
  for(i = 0; i < 3;i++) {
    port_out(0x33 + i,__svgalib_CRT_I);
                                /* regs 30 and 31 are readonly and no 32   */
    regs[EXT + 3 + i] = port_in(__svgalib_CRT_D);
                                        /* 36h and 37 do not exist on et6k */
  }
  port_out(0x3f,__svgalib_CRT_I);
  regs[EXT + 8] = port_in(__svgalib_CRT_D);
  port_out(0x18,__svgalib_CRT_I);
  regs[EXT + 2] = port_in(__svgalib_CRT_D);
                                        /* ET6000 only.               */
  /* Extended sequencer register 7 in EXT+9 doesen't exist on et6000. */
  /* Also EXT+10 doesn't exist. There is no 0x3c3 on et6000.          */
  regs[EXT + 11] = port_in(SEG_SELECT);
  regs[EXT + 10] = port_in(base1 | 0x13);  /* I am using EXT+10 for the   */
                                       /* linear map map location.    */
                                   /* Reset flip flop. Page 137       */
                                   /* of the et6000 manual. These two */
                                   /* regs 10 & 11 are 0 for linear.  */
  regs[EXT] = port_in(base1 | 0x58);   /* Offset 58 describes 16bpp       */
  regs[EXT + 6] = port_in(base1 | 0x59); /* bit1 = 0  555 RGB             */
                                     /*        1  565 RGB             */
                                   /* Offset 59 bits <32> set bpp.    */
                                   /* ---------------------           */
                                   /* | 3 | 2 |           |           */
                                   /* =====================           */
                                   /* | 0 | 0 | 24    bpp |           */
                                   /* | 0 | 1 |  8    bpp |           */
                                   /* | 1 | 0 | 15/16 bpp |           */
  port_in(__svgalib_IS1_R);        /* ---------------------           */
  __svgalib_delay();                   
  port_out(0x16,ATT_IW);           
  __svgalib_delay();
  regs[EXT + 12] = port_in(ATT_R); 
  __svgalib_delay();
  port_in(__svgalib_IS1_R);
  __svgalib_delay();
  port_out(0x17,ATT_IW);
  __svgalib_delay();
  regs[EXT + 7] = port_in(ATT_R);
  port_out(0x06,SEQ_I);
  regs[EXT + 5] = ((port_in(base1 | 0x57) & 2) << 4); /* bit 1 of 57 to bit5  */
  regs[EXT + 9] = port_in(base1 | 0x40);   /* Sets memory mapping.            */
  regs[EXT + 1] = port_in(base1 | 0x42);   /* Saved for future use.           */
#ifdef DB6K
  fprintf(outf,"Leaving saveregs.\n");
#endif
  return(13);                          /* ET6000 requires 13 EXT regs.    */
                                       /* We may find that it needs more! */
}

/* Set chipset-specific registers */

static void et6000_setregs(const unsigned char regs[], int mode)
{
  int i;
  unsigned char save;
  et6000_linear(LINEAR_DISABLE,0);  /* Turn off linear. */
#ifdef DB6K
  fprintf(outf,"In setregs for regs at %x, mode = %d\n",(int) regs,mode);
#endif

  et6000_unlock();
  /* Write some et6000 specific registers. */
  port_out(regs[EXT+11],SEG_SELECT);
  if(regs[EXT + 10])                       /* I am using EXT+10 for linear  */
    port_out_r(base1 | 0x13,regs[EXT + 10]);     /*  modes                        */
  port_out_r(base1 | 0x40,regs[EXT + 9]);
  /* Unprotect writing CRT reg 0x35 */
  port_out(0x11,__svgalib_CRT_I);
  save = port_in(__svgalib_CRT_D);
  port_out(save &0x7F,__svgalib_CRT_D);
#ifdef DB6K
  fprintf(outf,"After first block\n");
#endif

  /* Write extended CRT regs */            
  for(i = 0;i < 3;i++) {
    port_out(0x33 + i,__svgalib_CRT_I);
    port_out(regs[EXT + 3 + i],__svgalib_CRT_D);
  }
  port_out(0x3F,__svgalib_CRT_I);
  port_out(regs[EXT + 8],__svgalib_CRT_D);
  port_out(0x18,__svgalib_CRT_I);
  port_out(regs[EXT + 2],__svgalib_CRT_D);
                                      /* See saveregs for these registers. */
  port_out_r(base1 | 0x58,(vga_getcolors() == (1 << 16))? (regs[EXT] | 0x02):
                                                        regs[EXT]);
  port_out_r(base1 | 0x59,regs[EXT + 6]);
  port_out_r(base1 | 0x57,((port_in(base1 | 0x57) | ((regs[EXT + 5] & 0x20) >> 4))));

    /* For True Color modes we must devide the MCLK by 3. */
    port_out_r(base1 | 0x42,regs[EXT + 1]);
    port_out_r(base1 | 0x40,regs[EXT + 9]);
#ifdef DB6K
    fprintf(outf,"after second block\n");
#endif
  /* Original value for reg 0x11. */
  port_out(0x11,__svgalib_CRT_I);
  port_out(save,__svgalib_CRT_D);

  /* Write extended attribute register. */
  port_in(__svgalib_IS1_R);   /* Reset flip flop. Page 137 of et6000 chip */
  __svgalib_delay();          /* manual.                                  */
  port_out(0x16,ATT_IW);
  __svgalib_delay();
  port_out(regs[EXT + 12],ATT_IW);
  __svgalib_delay();
  port_in(__svgalib_IS1_R);
  __svgalib_delay();
  port_out(0x17,ATT_IW);
  __svgalib_delay();
  port_out(regs[EXT + 7],ATT_IW);
  /* I don't have an external DAC so I didn't include the last 3 regs.     */
#ifdef DB6K
  fprintf(outf,"End of setregs\n");
#endif
}

static void et6000_unlock(void)
{
  if(x3_8)
    port_out_r(x3_8,0xa0);
    else
      fprintf(stderr,"et6000_unlock called when et6000 was not initialized.\n");
}

static void et6000_lock(void)
{
  if(x3_8) {
#ifdef DB6K
    fprintf(outf,"Lock called\n");
#endif
    return;                /* et4000 does this, I don't know why! */
    port_out_r(x3_8,0);          /* I think it is because we lose root  */
  }                        /* privalege after initialize and can  */
  else                     /* no longer unlock registers.         */
    fprintf(stderr,"et6000_lock called when et6000 was not initialized.\n");
}

static int  et6000_test(void)
{
  if(et6000_chiptype)
    return(1);
  else {
    et6000_init(0,0,0);
    DPRT("In et6000 test\n");
    if(et6000_chiptype)
      return(1);
    return(0);
    }
}
/* Routines needed for modline calculation. */
static int et6000_map_clock(int bpp, int clock)
{
  return(clock);
}

static int et6000_match_programable_clock(int clock)
{
  return(clock);
}

static int et6000_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
  return(htiming);
}
/* End of modeline subroutines. */

static int  et6000_init(int force, int par1, int par2)
{
  unsigned char mem_cfg_reg;
  unsigned long buf[64];  /* 256 bytes of the pci bus registers. */

#ifdef DB6K
#include <sys/stat.h>
  char dboutfile[30];
  if(et6000_chiptype) return(1);
  sprintf(dboutfile,"D%ld.txt",time(NULL));
  outf = fopen(dboutfile,"w");
  if(!outf){
    fprintf(stderr,"Can't open a debug output file in et6000_init.\n");
    exit(1);
  }
  else
    {
      fprintf(outf,"et6000 init entered.\n");
      chmod(dboutfile,0666);
    }
#endif

  if(et6000_chiptype)
    return(1);             /* An attempt to reenter init. */
  /* Check for ET6000 chiptype.                                            */
  /* Get PCI and VL configuration space.  The VL bus has not been checked. */
  /* I have a PCI bus.                                                     */

    /* Test color mode or mono mode */
    x3_4 = __svgalib_CRT_I;       /* set in vga.c */
    x3_5 = __svgalib_CRT_D;
    x3_a = __svgalib_IS1_R;
    if(__svgalib_CRT_I == CRT_IC)
      x3_8 = 0x3d8;
    else
      x3_8 = 0x3b8;

    et6000_linear_base = 0;
		if(__svgalib_pci_find_vendor_vga(0x100c,buf,0) == 0 &&
		   (buf[0] == 0x3208100c))
		  {
DPRT("Found et6000\n");
		et6000_chiptype = ET6000;   /* Defined in vga.h */
		  }
		else 
		  {
#ifdef DB6K 
		   fprintf(outf,"Can't find et6000 chipset. buf[0] = 0x%8lx\n",
			(unsigned long) buf[0]);
#endif
		    return(0);
		  }

		et6000_unlock();            /* Set key. */
		mem_type = port_in(MIS_W) & 1;  /* 0 = DRAM, 1 = MDRAM */

                base1 = buf[0x5] & ~0xFF; 
		              /* 14 hex. The start of confg.  */
		              /* 3C2 is the input status register.  It has
			      ** things like the monitor ID and the video
			      ** memory type.  The two low order bits are
			      ** both zero for DRAM and both 1 for MDRAM.
			      ** Offset 0x45 is the memory configuration
			      ** register which gives the memory size depending
			      ** on the type.  Aparently one can't have 1Mbyte
			      ** of MDRAM.
			      */
		if(!pci59std) {
		  /* Saving pcibus registers so they may be restored. */
		  pci40std = port_in(base1 | 0x40);
		  pci42std = port_in(base1 | 0x42);
		  pci57std = port_in(base1 | 0x57);
		  pci58std = port_in(base1 | 0x58);
		  pci59std = port_in(base1 | 0x59);
		}
		mem_cfg_reg = port_in(base1 | 0x45) & 3;
#ifdef DB6K
		{
		  int i;
		  for(i = 0;i < 64;i+=4)
			fprintf(outf,"%08x     %08lx  %08lx  %08lx  %08lx\n",
				(i/4) << 4,buf[i+3],buf[i+2],buf[i+1],buf[i]);
		  fprintf(outf,"\npci40std 0x%02x  pci42std 0x%02x  pci58std "
                  "0x%02x  pci59std 0x%02x\n"
		  "mem_cfg_reg 0x%02x mem_type 0x%02x\n",pci40std,
			  pci42std,pci58std,pci59std,mem_cfg_reg,mem_type);
		}
#endif
		if(mem_type)
		  {   /* MDRAM type */
		    if(mem_cfg_reg == 1)
		      et6000_memory = 4;      /* 4 MBytes of MDRAM */
		    else
		      et6000_memory = 2;      /* 2 MBytes of MDRAM */
		  }
		else   /* DRAM */
		  {
		    if(mem_cfg_reg == 2)
		      et6000_memory = 4;      /* 4 MBytes of DRAM */
		    else if(mem_cfg_reg == 1)
		      et6000_memory = 2;      /* 2 MBytes of DRAM */
		    else
		      et6000_memory = 1;      /* 1 MByte or DRAM  */
		  }
    et6000_linear_base = buf[4] & 0xff000000;
    __svgalib_linear_mem_size = et6000_memory*0x100000;
    __svgalib_linear_mem_base = et6000_linear_base;
    /* Banked memory is set in vga.c */
    port_out_r(base1 | 0x68,0xa);   /* Set clocka */
    port_out_r(base1 | 0x69,0x24);
    port_out_r(base1 | 0x69,0x21);
    if(__svgalib_driver_report)
      fprintf(stderr,"Using Tseng ET6000 driver (%d MBytes %sDRAM)\n",
		       et6000_memory,mem_type ? "M" : "");
#ifdef DB6K
    else
      fprintf(outf,"NO driver report. %d MBytes %sDRAM\n",et6000_memory,
	      mem_type ?  "M" : "");
#endif
    __svgalib_driverspecs = &__svgalib_et6000_driverspecs;
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = et6000_memory << 10;
    cardspecs->maxPixelClock4bpp = 75000;
    cardspecs->maxPixelClock8bpp = 160000;
    cardspecs->maxPixelClock16bpp = 160000;
    cardspecs->maxPixelClock24bpp = 160000;
    cardspecs->maxPixelClock32bpp = 160000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 2040;
    cardspecs->nClocks = 0;
    cardspecs->clocks = NULL;
    cardspecs->mapClock = et6000_map_clock;
    cardspecs->matchProgrammableClock = et6000_match_programable_clock;
    cardspecs->mapHorizontalCrtc = et6000_map_horizontal_crtc;
    et6000_lock();
#ifdef DBG
    {
      int i,*csp;
      fdm = fopen("regset.txt","w");
      if(!fdm)
	{
	  fprintf(stderr,"Can't open register setting file.\n");
	  exit(1);
	}
      fprintf(fdm,"CardSpecs\n");
      csp = (int* ) cardspecs;
      for(i = 1;i < 13; i++)
	{
	  fprintf(fdm,"%d,  ",*(csp+i));
	  if(i % 3 == 0) fprintf(fdm,"\n");
	}
      fprintf(fdm,"videoMemory %d\n",cardspecs->videoMemory);
    }
#endif  /* DBG */
    return(0);
}

static unsigned char last_page = 0;

/* Bank switching function.  Set 64k bank number. */
static void et6000_setpage(int page)
{
#ifdef DB6KP
  fprintf(outf,"setpage %d\n",page);
#endif
    /* Set both read and write bank. 3cd */
    port_out(last_page = (page | ((page & 15) << 4)), SEG_SELECT);
	/* Write page4-5 to bits 0-1 of ext. bank register, */
	/* and to bits 4-5. */
    /* return;       Testing */
	port_out_r(0x3cb, ((port_in(0x3cb) & ~0x33) | (page >> 4) | (page & 0x30)));
}

static void et6000_setrdpage(int page)
{
  DPRT("In setrdpage.\n");
  last_page &= 0x0F;
  last_page |= (page << 4);
  port_out(last_page,SEG_SELECT);
  port_out_r(0x3cb,(port_in(0x3cb) & ~0x30) | (page & 0x30));
}

static void et6000_setwrpage(int page)
{
  DPRT("In setwrpage");
  last_page &= 0xF0;
  last_page |= (page | 0x0F);
  port_out(last_page,SEG_SELECT);
  port_out_r(0x3cb,(port_in(0x3cb) & ~0x03) | page >> 4);
}

static int  et6000_setmode(int mode, int prv_mode)
{

    unsigned char i,*moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

#ifdef DB6K
    fprintf(outf,"In setmode, mode = %d, prv_mode = %d\n",mode,prv_mode);
    compareregs();
#endif
	/* Standard dac behaviour */
    switch (et6000_modeavailable(mode)) {
    case STDVGADRV:
	/* Reset extended register that is set to non-VGA */
	/* compatible value for 132-column textodes (at */
	/* least on some cards). */
DPRT("STDVGADRV\n");
	et6000_unlock();
	port_out_r(__svgalib_CRT_I, 0x34);            /* 3#4 */
	i = port_in(__svgalib_CRT_D);               /* 3#5 */
	if ((i & 0x02) == 0x02)                 /* Clock0 select bit 2 */
	    port_out_r(__svgalib_CRT_D, (i & 0xFD));  /* Turn it off. */
	                              /* Make sure pci bus is set right. */
#ifdef DB6K
	fprintf(outf,"reg 42 = %2x  58 = %2x before.\n",port_in(base1 | 0x42)
		,port_in(base1 | 0x58));
#endif
	port_out_r(base1 | 0x40,pci40std);
	port_out_r(base1 | 0x42,pci42std);
	port_out_r(base1 | 0x57,pci57std);
	port_out_r(base1 | 0x58,pci58std);
	port_out_r(base1 | 0x59,pci59std);
	port_out_r(base1 | 0x46,(port_in(base1 | 0x46)&0xfe)); 
	  /* make sure cursor is off */
	if(mode == 5) cursor_doublescan = 1;
#ifdef DB6K
	fprintf(outf,"pci42 = %2x, pci58 = %2x, pci59 = %2x\n",pci42std,
		pci58std,pci59std);
	fprintf(outf,"reg 42 = %2x after.\n",port_in(base1 | 0x42));
#endif
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    case SVGADRV:
DPRT("SVGADRV\n");
	    break;
    default:
	return 1;		/* mode not available */
    }
    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);
    modetiming = malloc(sizeof(ModeTiming));
    if(__svgalib_getmodetiming(modetiming,modeinfo,cardspecs))
      {
	free(modetiming);
	free(modeinfo);
	return(1);
      }
    moderegs = malloc(ET6000_TOTAL_REGS);
    et6000_initializemode(moderegs,modetiming,modeinfo,mode);
    cursor_doublescan = modetiming->flags & DOUBLESCAN ?1:0;
#ifdef DBG
    fprintf(fdm,"In setmode.\n");
    pmodetiming(modetiming);
    fdumpregs(fdm,moderegs,ET6000_TOTAL_REGS);
    fprintf(fdm,"cursor_doublescan = %1d\n",cursor_doublescan);
#endif
	__svgalib_setregs(moderegs);
	et6000_setregs(moderegs,mode);
	free(moderegs);
	free(modeinfo);
	free(modetiming);
	return(0);
}

static int  et6000_modeavailable(int mode)
{
  int rtn;
  struct vgainfo *info;
  ModeTiming *modetiming;
  ModeInfo *modeinfo;


  if (IS_IN_STANDARD_VGA_DRIVER(mode))
    return(__svgalib_vga_driverspecs.modeavailable(mode));
  if(mode <= TEXT || mode > GLASTMODE)
    return(0);
  info = &__svgalib_infotable[mode];
  if(et6000_memory*1024*1024 < info->ydim*info->xbytes)
    return(0);       /* The xbytes is xdim*bytesperpixel
                     ** The DAC is built in and I should be able to create
		     **	anything I have memory for.  I will DISABLE_MODE what
		     ** I can't handle. */
  modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);
  modetiming = malloc(sizeof(ModeTiming));
#ifdef DBG
  fprintf(fdm,"Calling getmodetiming\n");
#endif
  if(__svgalib_getmodetiming(modetiming,modeinfo,cardspecs))
    rtn = 0;
  else if(modetiming->flags & INTERLACED)
      rtn = 0;
  else if(modetiming->flags & DOUBLESCAN && (modeinfo->bytesPerPixel == 1))
    rtn = 0;
  else
    rtn = SVGADRV;
#ifdef DBG
  pmodeinfo(modeinfo);
  pmodetiming(modetiming);
  fprintf(fdm,"Mode %d %s.\n",mode,rtn?"OK":"failure");
#endif
  free(modetiming);
  free(modeinfo);
  return(rtn);
}

static void et6000_setdisplaystart(int address)
{
  /* DPRT("setdisplaystart\n");
   #ifdef DB6K
  fprintf(outf,"address = 0x%08x\n",address);
  #endif */
  port_outw_r(x3_4, 0x0d +(((address >>2) & 0x00ff) << 8));  /* ds0-7  */
  port_outw_r(x3_4, 0x0c + ((address >>2) & 0xff00));        /* ds0 15 */
  port_out_r(x3_4, 0x33);
  port_out_r(x3_5, (port_in(x3_5) & 0xf0)  | ((address & 0xf0000) >> 18 ));
                                                      /* ds 16-19 */
}

static void et6000_setlogicalwidth(int width)
{
  DPRT("setlogicalwidth\n");
#ifdef DB6K
  fprintf(outf,"width = 0x%08x\n",width);
#endif
  port_outw_r(x3_4,0x13 + (((width >> 3 ) & 0xff) << 8));       /* lw3-10 */
  port_out_r(x3_4 ,0x3f);
  port_out_r(x3_5,(port_in(x3_5) & 0x3f) | ((width & 0x1800) >> 3)); /* lw11-12 */
}

static void et6000_getmodeinfo(int mode,vga_modeinfo *modeinfo)
{
  switch(modeinfo->colors){
  case 16:
    modeinfo->maxpixels = 65536*8;
    break;
  default:
    if(modeinfo->bytesperpixel > 0)
      modeinfo->maxpixels = et6000_memory*1024*1024/
	modeinfo->bytesperpixel;
    else
      modeinfo->maxpixels = et6000_memory*1024*1024;
    break;
  }
  modeinfo->maxlogicalwidth = 4088;              /* Why? */
  modeinfo->startaddressrange = 0xfffff*et6000_memory;
           /* Mask for the startaddress */
  if(mode == G320x200x256)
    modeinfo->startaddressrange = 0;
  modeinfo->haveblit = 0;
  modeinfo->memory = et6000_memory*1024;         /* In kbytes. */
  modeinfo->flags |= HAVE_RWPAGE;
  if(et6000_interlaced(mode))
    modeinfo->flags |= IS_INTERLACED;
#ifdef DB6K
  fprintf(outf,"Getmodeinfo mode = %d, colors = %d, linearset = 0x%x " 
          ,mode,modeinfo->colors,__svgalib_modeinfo_linearset);
#endif
    if((mode > LMODEMIN) && (mode < LMODEMAX)) {
      modeinfo->flags |= (__svgalib_modeinfo_linearset | CAPABLE_LINEAR); 
#ifdef DB6K
      fprintf(outf,"flag = 0x%x\n",modeinfo->flags);
#endif
    }
  modeinfo->chiptype = et6000_chiptype;            /* is set in vgamisc.c  */
  modeinfo->linewidth_unit = 8;
  modeinfo->aperture_size = et6000_memory*1024;
#ifdef DB6K
  fprintf(outf,"\nModeinfo at the end of the subroutine:\n");
  prtmodeinfo(modeinfo);
  fprintf(outf,"Text mode errors = %d\n",compareregs());
}

static void prtmodeinfo(vga_modeinfo *m)
{
  fprintf(outf,"width\t\t\t%4d\nheight\t\t\t%4d\nbytesperpixel\t\t%4d\
\ncolors\t\t\t%4d\nlinewidth\t\t%4d\nmaxlogicalwidth\t\t%4d\
\nstartaddressrange\t%4d\nmaxpixels\t\t%4d\nhaveblit\t\t%4d\nflags\t\t\t%4x\
\nchiptype\t\t%4d\nmemory\t\t\t%4dkbytes\nlinewidth_unit\t\t%4d\
\nlinear_aperture\t\t%s\naperture_size\t\t%4d\nset_aperture_page\t%4x\
\nextensions\t\t%4s\n",
	  m->width,m->height,m->bytesperpixel,m->colors,m->linewidth,
	  m->maxlogicalwidth,m->startaddressrange,m->maxpixels,m->haveblit,
	  m->flags,m->chiptype,m->memory,m->linewidth_unit,
	  m->linear_aperture ? "available":"not available",m->aperture_size,
	  m->set_aperture_page ? (int) *m->set_aperture_page :(int) 0,
	  m->extensions ? "yes" : "no");
}

static int compareregs(void)
{
  int n,i;

  n = 0;
    if(textset == 0) {
      textstore = malloc(MAX_REGS);
      vgatextstore = malloc(MAX_REGS);
      textset = 1;
    }
    if(textset == 1) {
      vga_gettextmoderegs(textstore);
      fputc('C',outf);
      if(textstore[25] == 1) 
	textset = 2;
    }
    if(textset == 2) {
	vga_gettextmoderegs(vgatextstore);
      fprintf(outf,"textstore = %x and vgatextstore = %x.\n",(int) textstore,
	   (int) vgatextstore);
      for(i = 0;i < 73;i++) {
	if(textstore[i] != vgatextstore[i]) {
	  fprintf(outf,"register %d text = %2x, vgatext = %2x\n",i,
		textstore[i],vgatextstore[i]);
	  n++;
	}
      }
      if(vgatextstore[25] == 0)
	textset = 1;
    }
  return(n);
}
#else
}
#endif
/*
static int  et6000_ext_set(unsigned what, va_list params)
{
  DNOTW("ext_set");
  return(0);
}

static int  et6000_accel(unsigned operation, va_list params)
{
  DNOTW("accel");
  return(0);
}
*/
static int  et6000_linear(int op, int param)
{
#ifdef DB6K
  fprintf(outf,"linear op = %d, param = 0x%0x ",op,param);
#endif
  if(op == LINEAR_QUERY_BASE) {
#ifdef DB6K
    fprintf(outf, "QUERY\n");
#endif                                    
                              /* Possible error with & */
    if((vga_getmodeinfo(vga_getcurrentmode())->flags & CAPABLE_LINEAR))
      return(et6000_linear_base);
    else                    /* frame_ptr was determined at init time */
      return(-1);
  }
  else if(op == LINEAR_ENABLE)
    {
#ifdef DB6K
      fprintf(outf,"ENABLE, param = 0x%x\n",param);
#endif
      et6000_mapped_mem = param;
      port_out(0,SEG_SELECT);
      port_out(0,0x3cb);
      port_out(5,GRA_I);
      port_out(0x40,GRA_D);  /* Set linear graphics. Don't touch this */
                             /* on disable. */
      port_out_r(base1 | 0x40, 0x09);
      /* Set memory relocation and linear. */
      return(0);
    }
  else if(op == LINEAR_DISABLE)
    {
#ifdef DB6K
      fprintf(outf,"DISABLE\n");
#endif
      port_out_r(base1 | 0x40, 0);
      /* Turn off memory relocation and linear. */
      __svgalib_modeinfo_linearset = 0;  /* In vga.c */
      return(0);
    }
    else if(op == LINEAR_QUERY_RANGE) {
#ifdef DB6K
      fprintf(outf,"RANGE\n");
#endif
    return(-1);
    }
    else if(op == LINEAR_QUERY_GRANULARITY) {
#ifdef DB6K
      fprintf(outf,"GRANULARITY\n");
#endif
    return(et6000_memory*1024*1024);
    }
#ifdef DB6K
  fprintf(outf,"UNKNOWN function\n");
#endif
    return(-1);
}
static int et6000_cursor(int cmd,int p1,int p2,int p3,int p4,void *p5)
{
  int i,j;

#ifdef DB6K
  if(cmd != 2 && cmd != 3)
  fprintf(outf,"In et6000_cursor cmd = %d\n",cmd);
  fflush(outf);
#endif
  switch(cmd)
    {
    case CURSOR_INIT:
      for(i = 0;i < 16;i++)
	cursors[i] = NULL;
      break;

    case CURSOR_HIDE:
      port_out_r(base1 | 0x46,port_in(base1 | 0x46)&0xfe);
      break;

    case CURSOR_SHOW:
      if(active_cursor == -1)
	{
	  fprintf(stderr,"No active cursor\n");
	  return(-1);
	}
      port_out_r(base1 | 0x46,port_in(base1 | 0x46) | 0x01);
      break;

    case CURSOR_POSITION:
      port_out_r(base1 | 0x84,p1&0xff);
      port_out_r(base1 | 0x85,p1 >> 8);
      port_out_r(base1 | 0x86,(p2<<cursor_doublescan)&0xff);
      /* double for double scan */
      port_out_r(base1 | 0x87,(p2<<cursor_doublescan) >> 8);
      break;

    case CURSOR_IMAGE:
      switch(p2)
	{
	case 0:
	  if(cursors[p1] != NULL)
	    free(cursors[p1]);
	  cursors[p1] = malloc(256);
	  memcpy(cursors[p1],p5,256);
	  cursor_format[p1] = 0;
	  break;

	case 1:
	  if(cursors[p1] != NULL)
	    free(cursors[p1]);
	  cursors[p1] = malloc(4096);
	  memcpy(cursors[p1],p5,4096);
	  cursor_format[p1] = 1;
	  break;
	}
      cursor_colors[p1*2] = p3;
      cursor_colors[p1*2 + 1] = p4;
      cursor_loaded[p1] = 1;
      break;

    case CURSOR_SELECT:
      if(active_cursor == p1)
	return(0);
      if(cursor_loaded[p1])
	{
	  int c = 64, cl[2],sptptr,sptr,k,n,l,cf,islinear,col=0;
	  unsigned char c0=0,cr;
	  unsigned char f,f1,cb,*cu;

	  if(vga_getmodeinfo(vga_getcurrentmode())->flags & IS_LINEAR)
	    islinear = 1;
	  else
	    islinear = 0;
#ifdef DB6K
		
		fprintf(outf,"cursor_loaded[%d] = %d\n",p1,cursor_loaded[p1]);
		{
		  unsigned short pat[8] = {0x0055,0xaa55,0xff55,0x0033,0x0022,
					   0x00aa,0xffaa,0x0000};
		  int i,j;
		  if(islinear)
		    {
		      fprintf(outf,"Islinear = %d , 16*et6000_memory = %d\n",
			      islinear,16*et6000_memory);
		      /* Put something here for linear. */
		    }
		  else
		    {
		      fprintf(outf,"islinear = %d\n",islinear);
		      et6000_setpage((16*et6000_memory) - 1);
		    }
		  for(i = 0;i < 0x8;i++)
		    for(j = 0;j < 0x2000;j+=2)
		      gr_writew(pat[i],i*0x2000 + j + islinear*
				65536*(16*et6000_memory - 1));
		  if(!islinear) et6000_setpage(0);
		}
#endif
	  for(j = 0;j < 2;j++)
	    {
	      cl[j] = 0;
	      col = cursor_colors[p1*2 + j];
	      for(i = 0;i < 3;i++)
		{
		  cf = (col << i*8) & 0xff0000;
		  if(c > cf) c0 = 0;
		  else if(2*c > cf) c0 = 1;
		  else if(3*c > cf) c0 = 2;
		  else c0 = 3;
		  cl[j] |= c0;
		  cl[j] = cl[j] << 2;
#ifdef DB6K
		  fprintf(outf,"f = 0x%02x \n",cf);
#endif
		}
	      cl[j] = cl[j] >> 2;
#ifdef DB6K
		fprintf(outf,"Col = 0x%x, Color %d = 0x%x\n c0 = 0x%x\n"
			,col,j,cl[j],c0);
#endif
	    }
	  port_out_r(base1 | 0x67,9);  /* set register 9 for colors */
	  port_out_r(base1 | 0x69,cl[0]); /* Sprite color 0 */
	  port_out_r(base1 | 0x69,cl[1]); /* Sprite color 1 */
	  /*  sptr = sptptr = __svgalib_linear_mem_size/2 - 1024; */
	  sptptr = 0x7ff00;
	  sptr = 64512 - 128 - 512*(cursor_doublescan - 1) + 
	    (islinear)*65536*(16*et6000_memory - 2); 
	  /* 64512 = 65536 - 16*64  */
	  if(islinear)
	    {
#ifdef DB6K
	      fprintf(outf,"Linear sptr = %d\n",sptr);
#endif
	      /* Put something here for linear */
	    }
	  else
	    {
#ifdef DB6K
	      fprintf(outf,"Paged sptr = %d\n",sptr);
#endif

	      et6000_setpage((16*et6000_memory) - 1);
	    }
	  cr = 0;
	  cb = 0;
	  l = 0;
	  cu = (unsigned char *) cursors[p1];
	  for(k = 0;k < 32;k++)
	    {
	      for(n = 3;n >= 0;n--)
		{
		  f = *(cu + (32 + k)*4 + n);
		  f1 = *(cu + k*4 + n);
		  for(j = 0;j < 8;j++)
		    {
		      if(f&0x01)
			{
			  if(f1&0x01) cr |= 0x01;
			  else
			    cr |= 0x00;
			}
		      else cr |= 0x02;
		      cb = cb << 2;
		      cb |= cr;
		      cr = 0;
		      f  = f >> 1;
		      f1 = f1 >> 1;
		      if(!((j+1) % 4))
		      {
			gr_writeb(cb,sptr + 16*8 + 9 + l - j/4);
			cb = 0;
		      }
		    }
		  l += 2;
		}
	      sptr += 8;
	    }
	    
	  port_out_r(x3_4,0x0E); /* CRTC Reg E bits 16-19 of sprite add. */
	  port_out_r(x3_5,(sptptr >> 16) & 0x0f);
	  port_out_r(x3_4,0x0F);
	  port_out_r(x3_5,(sptptr >> 8)&0xff);
	  port_out_r(base1 | 0x82,0x20);  /* set cursor size. */
	  port_out_r(base1 | 0x83,0x20*(cursor_doublescan-1));
	  active_cursor = p1;
#ifdef DB6K
	  port_out_r(x3_4,0x0E);
	  fprintf(outf,"sptptr = 0x%08x  0E = 0x%x ",
		  sptptr,port_in(x3_5));
	  port_out_r(x3_4,0x0F);
	  fprintf(outf,"0F = 0x%x\n",port_in(x3_5));
	  *(LINEAR_POINTER) = 0x26;
	  fprintf(outf," = 0x%02x\n",
		  *(LINEAR_POINTER));
	  if(!islinear) et6000_setpage((16*et6000_memory) - 1);
	  for(i = 0;i < 64;i++)
	    {
	      for(j = 0;j < 16;j++)
		fprintf(outf,"%02x",
		    gr_readb(64512 - 128 + 16*i + j + islinear*65536*
		      ((16*et6000_memory) - 1) - 512*(cursor_doublescan-1)));
	      fprintf(outf,"\n");
	    }
	  fprintf(outf,"LINEAR_POINTER %08x, GM = %08x, islinear = %d\n",
		  (unsigned int)  LINEAR_POINTER,GM,islinear);
	  if(!islinear)et6000_setpage(0);
#endif		      
	  if(!islinear)et6000_setpage(0);
	}
      else 
	{
	  fprintf(stderr,"Cursor %d's immage has not been loaded when selected!\n",
		 p1);
	 return(-1);
	}
      break;
    }
  return(0);
}

/* Programs not exported. */

static int et6000_interlaced(int mode)
{
  ModeTiming *modetiming;
  ModeInfo   *modeinfo;
  int        rtn;

  DPRT("In interlaced\n");
#ifdef DB6K
  fprintf(outf,"Text mode errors = %d\n",compareregs());
#endif
    if (et6000_modeavailable(mode) != SVGADRV)
	return 0;
    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);
    modetiming = malloc(sizeof(ModeTiming));
    if(__svgalib_getmodetiming(modetiming,modeinfo,cardspecs))
       rtn = 0;
    else
      rtn = modeinfo->flags & INTERLACED;
#ifdef DB6K
    fprintf(outf,"Interlaced %s\n",rtn?"Yes":"No");
#endif
    free(modetiming);
    free(modeinfo);
    return(rtn);
}

#define WITHIN(v,c1,c2) (((v) >= (c1)) && ((v) <= (c2)))

static unsigned comp_lmn(unsigned clock,float fac){
  int     m, n1, n2;
  double  fvco;
  double  fout;
  double  fmax, fmin;
  double  fref;
  double  fvco_goal;

  fmax = 230000.0;
  fmin = 48000.0;

/* fmin, fmax and limits for n and m are guesses */

  fref = 14318; 

  for (n1 = 4; n1 <= 33; n1++)
  {
    for (n2 = 0; n2 <= 3; n2++)
    {
      for (m = 42; m <= 115; m++)
      {
        fout = ((double)(m) * fref)/((double)(n1) * (1 << n2));
        fvco_goal = (double)clock * (double)(1 << n2)*fac;
        fvco = fout * (double)(1 << n2);
        if (!WITHIN(fvco, 0.995*fvco_goal, 1.005*fvco_goal))
          continue;
        if (!WITHIN(fvco, fmin, fmax))
          continue;
#if 0
	fprintf(stderr,"clock=%i m=%i n1=%i n2=%i\n ",clock,m,n1,n2);
#endif
        return (m - 2) | ((n1 - 2)<<8) | (n2 << 13) ;
      }
    }
  }
  fprintf(stderr,"ET6000: Illegal clock\n");
  return 0;
}
