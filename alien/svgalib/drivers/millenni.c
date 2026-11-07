/*
Matrox Millennium / Millenium II driver 
*/
#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "svgadriv.h"
#include "timing.h"
#include "vgaregs.h"
#include "interfac.h"
#include "vgapci.h"
#include "vgammvga.h"
#include "mga.h"

static int mil_init(int, int, int);
static void mil_unlock(void);
static void mil_lock(void);

static int mil_memory=0, mil_pciposition;
static int mil_is_linear, mil_linear_base, mil_mmio_base, mil_chiptype;
static int interleave, rounding;

static CardSpecs *cardspecs;

static void mil_outext(int ind, int data) {
    OUTREG8(0x1fde, ind);
    OUTREG8(0x1fdf, data);
}

static int mil_inext(int ind) {
    OUTREG8(0x1fde, ind);
    return INREG8(0x1fdf);
}

static void mil_setpage(int page)
{
	mil_outext(4, page);
}

static int __svgalib_mil_inlinearmode(void)
{
return mil_is_linear;
}

/* Fill in chipset specific mode information */

static void mil_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;
    if(modeinfo->bytesperpixel==3)return;

    if(modeinfo->linewidth & rounding) {
       modeinfo->linewidth = (modeinfo->linewidth | rounding) + 1;
    }

    modeinfo->maxpixels = mil_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = mil_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(mil_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_mil_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

const static unsigned char MGADACregs[] = {
	0x0F, 0x18, 0x19, 0x1A, 0x1C,   0x1D, 0x1E, 0x2A, 0x2B, 0x30,
	0x31, 0x32, 0x33, 0x34, 0x35,   0x36, 0x37, 0x38, 0x39, 0x3A,
	0x06
};

   
#define DACREGSIZE sizeof(MGADACregs)
/*
 * initial values of ti3026 registers
 */
const static unsigned char MGADACbpp8[DACREGSIZE] = {
	0x06, 0x80, 0x4b, 0x25, 0x00,   0x00, 0x04, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0x00, 0x00,    0, 0x00,
	0x00
};
const static unsigned char MGADACbpp16[DACREGSIZE] = {
	0x07, 0x45, 0x53, 0x15, 0x00,   0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0x00, 0x10,    0, 0x00,
	0x00
};
const static unsigned char MGADACbpp24[DACREGSIZE] = {
	0x06, 0x56, 0x5b, 0x25, 0x00,   0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0x00, 0x10,    0, 0x00,
	0x00
};
const static unsigned char MGADACbpp32[DACREGSIZE] = {
	0x07, 0x46, 0x5b, 0x05, 0x00,   0x00, 0x2C, 0x00, 0x1E, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0x00, 0x10,    0, 0x00,
	0x00
};

#define TI_MIN_VCO_FREQ  110000
#define TI_MAX_VCO_FREQ  220000
#define TI_MAX_MCLK_FREQ 100000
#define TI_REF_FREQ      14318.18

static double
MGATi3026CalcClock (
   long f_out, long f_max,
   int *m, int *n, int *p
){
	int best_m = 0, best_n = 0;
	double f_pll, f_vco;
	double m_err, inc_m, calc_m;

	/* Make sure that f_min <= f_out <= f_max */
	if ( f_out < ( TI_MIN_VCO_FREQ / 8 ))
		f_out = TI_MIN_VCO_FREQ / 8;
	if ( f_out > f_max )
		f_out = f_max;

	/*
	 * f_pll = f_vco / 2 ^ p
	 * Choose p so that TI_MIN_VCO_FREQ <= f_vco <= TI_MAX_VCO_FREQ
	 * Note that since TI_MAX_VCO_FREQ = 2 * TI_MIN_VCO_FREQ
	 * we don't have to bother checking for this maximum limit.
	 */
	f_vco = ( double ) f_out;
	for ( *p = 0; *p < 3 && f_vco < TI_MIN_VCO_FREQ; ( *p )++ )
		f_vco *= 2.0;

	/*
	 * We avoid doing multiplications by ( 65 - n ),
	 * and add an increment instead - this keeps any error small.
	 */
	inc_m = f_vco / ( TI_REF_FREQ * 8.0 );

	/* Initial value of calc_m for the loop */
	calc_m = inc_m + inc_m + inc_m;

	/* Initial amount of error for an integer - impossibly large */
	m_err = 2.0;

	/* Search for the closest INTEGER value of ( 65 - m ) */
	for ( *n = 3; *n <= 25; ( *n )++, calc_m += inc_m ) {

		/* Ignore values of ( 65 - m ) which we can't use */
		if ( calc_m < 3.0 || calc_m > 64.0 )
			continue;

		/*
		 * Pick the closest INTEGER (has smallest fractional part).
		 * The optimizer should clean this up for us.
		 */
		if (( calc_m - ( int ) calc_m ) < m_err ) {
			m_err = calc_m - ( int ) calc_m;
			best_m = ( int ) calc_m;
			best_n = *n;
		}
	}
	
	/* 65 - ( 65 - x ) = x */
	*m = 65 - best_m;
	*n = 65 - best_n;

	/* Now all the calculations can be completed */
	f_vco = 8.0 * TI_REF_FREQ * best_m / best_n;
	f_pll = f_vco / ( 1 << *p );

	return f_pll;
}

static void 
MGATi3026SetPCLK( MGARegPtr pReg, long f_out, int bpp )
{
	/* Pixel clock values */
	int m, n, p;

	/* Loop clock values */
	int lm, ln, lp, lq;
	double z;

	/* The actual frequency output by the clock */
	double f_pll;

	long f_max = TI_MAX_VCO_FREQ;

	/* Do the calculations for m, n, and p */
	f_pll = MGATi3026CalcClock( f_out, f_max, & m, & n, & p );

	/* Values for the pixel clock PLL registers */
	pReg->DacClk[ 0 ] = ( n & 0x3f ) | 0xc0;
	pReg->DacClk[ 1 ] = ( m & 0x3f );
	pReg->DacClk[ 2 ] = ( p & 0x03 ) | 0xb0;

	/*
	 * Now that the pixel clock PLL is setup,
	 * the loop clock PLL must be setup.
	 */

	/*
	 * First we figure out lm, ln, and z.
	 * Things are different in packed pixel mode (24bpp) though.
	 */
	 if ( bpp == 24 ) {

		/* ln:lm = ln:3 */
		lm = 65 - 3;

#if 0
		/* Check for interleaved mode */
		if ( bpp == 2 )
			/* ln:lm = 4:3 */
			ln = 65 - 4;
		else
#endif
			/* ln:lm = 8:3 */
			ln = 65 - 8;

		/* Note: this is actually 100 * z for more precision */
		z = ( 11000 * ( 65 - ln )) / (( f_pll / 1000 ) * ( 65 - lm ));
	}
	else {
		/* ln:lm = ln:4 */
		lm = 65 - 4;

		/* Note: bpp = bytes per pixel */
		ln = 65 - 4 * ( 64 / 8 ) / bpp;

		/* Note: this is actually 100 * z for more precision */
		z = (( 11000 / 4 ) * ( 65 - ln )) / ( f_pll / 1000 );
	}

	/*
	 * Now we choose dividers lp and lq so that the VCO frequency
	 * is within the operating range of 110 MHz to 220 MHz.
	 */

	/* Assume no lq divider */
	lq = 0;

	/* Note: z is actually 100 * z for more precision */
	if ( z <= 200.0 )
		lp = 0;
	else if ( z <= 400.0 )
		lp = 1;
	else if ( z <= 800.0 )
		lp = 2;
	else if ( z <= 1600.0 )
		lp = 3;
	else {
		lp = 3;
		lq = ( int )( z / 1600.0 );
	}
 
	/* Values for the loop clock PLL registers */
	if ( bpp == 24 ) {
		/* Packed pixel mode values */
		pReg->DacClk[ 3 ] = ( ln & 0x3f ) | 0x80;
		pReg->DacClk[ 4 ] = ( lm & 0x3f ) | 0x80;
		pReg->DacClk[ 5 ] = ( lp & 0x03 ) | 0xf8;
 	} else {
		/* Non-packed pixel mode values */
		pReg->DacClk[ 3 ] = ( ln & 0x3f ) | 0xc0;
		pReg->DacClk[ 4 ] = ( lm & 0x3f );
		pReg->DacClk[ 5 ] = ( lp & 0x03 ) | 0xf0;
	}
	pReg->DacRegs[ 18 ] = lq | 0x38;

}

/*
 * MGA3026Init -- for mga2064 with ti3026
 *
 * The 'mode' parameter describes the video mode.	The 'mode' structure 
 * as well as the 'vga256InfoRec' structure can be dereferenced for
 * information that is needed to initialize the mode.	The 'new' macro
 * (see definition above) is used to simply fill in the structure.
 */
static int BppShifts[4]={0,1,0,2};

static int
MGA3026Init(unsigned char *regs, ModeTiming * modetiming, ModeInfo * modeinfo)
{
	int hd, hs, he, ht, vd, vs, ve, vt, wd;
	int i, BppShift, index_1d = 0;
	const unsigned char* initDAC;
	MGARegPtr pReg;

        pReg = (MGARegPtr)(regs+60);

	BppShift = BppShifts[(modeinfo->bitsPerPixel >> 3) - 1];

        pReg->opmode=0;
        
	switch(modeinfo->bitsPerPixel)
	{
	case 16:
		initDAC = MGADACbpp16;
#if __BYTE_ORDER == __BIG_ENDIAN
                pReg->opmode &= ~0x300; /* the aperture is little endian */
                pReg->opmode |=  0x100;
#endif
		break;
	case 24:
		initDAC = MGADACbpp24;
		break;
	case 32:
            	initDAC = MGADACbpp32;
#if __BYTE_ORDER == __BIG_ENDIAN
                pReg->opmode &= ~0x300; /* the aperture is little endian */
                pReg->opmode |=  0x200;
#endif
		break;
	case 8:
	default:
		initDAC = MGADACbpp8;
		break;
        }
	
	/* Allocate the DacRegs space if not done already */
	for (i = 0; i < DACREGSIZE; i++) {
	    pReg->DacRegs[i] = initDAC[i]; 
	    if (MGADACregs[i] == 0x1D)
		index_1d = i;
	}

	if ( (modeinfo->bitsPerPixel == 16) && (modeinfo->greenWeight == 5) ) {
	    pReg->DacRegs[1] &= ~0x01;
	}

	if ( modeinfo->bitsPerPixel == 24 ) {
	    int silicon_rev;
	  /* we need to set DacRegs[0] differently based on the silicon
	   * revision of the 3026 RAMDAC, as per page 2-14 of tvp3026.pdf.
	   * If we have rev A silicon, we want 0x07; rev B silicon wants
	   * 0x06.
	   */
	    silicon_rev = inTi3026(TVP3026_SILICON_REV);
	  
	    if(silicon_rev <= 0x20) {
	      /* rev A */
	      pReg->DacRegs[0] = 0x07;
	    } else {
	      /* rev B */
	      pReg->DacRegs[0] = 0x06;
	    }
	}
        
        if(interleave) pReg->DacRegs[2]+=1;
	
        /*
	 * Here all of the MGA registers get filled in.
	 */
	hd = (modetiming->CrtcHDisplay	>> 3)	- 1;
	hs = (modetiming->CrtcHSyncStart>> 3)	- 1;
	he = (modetiming->CrtcHSyncEnd	>> 3)	- 1;
	ht = (modetiming->CrtcHTotal	>> 3)	- 1;
	vd = modetiming->CrtcVDisplay		- 1;
	vs = modetiming->CrtcVSyncStart		- 1;
	ve = modetiming->CrtcVSyncEnd		- 1;
	vt = modetiming->CrtcVTotal		- 2;

        /* HTOTAL & 0x7 equal to 0x6 in 8bpp or 0x4 in 24bpp causes strange
         * vertical stripes
         */
        if((ht & 0x07) == 0x06 || (ht & 0x07) == 0x04)
                ht++;

        if (modeinfo->bitsPerPixel == 24)
		wd = (modeinfo->width * 3) >> (4 - BppShift);
	else
		wd = modeinfo->width >> (4 - BppShift);	

        if(wd&7) {
           wd=(wd|7)+1;
        }

	pReg->ExtVga[0] = 0;
	pReg->ExtVga[5] = 0;
	
	if (modetiming->flags & INTERLACED)
	{
		pReg->ExtVga[0] = 0x80;
		pReg->ExtVga[5] = (hs + he - ht) >> 1;
		wd <<= 1;
		vt &= 0xFFFE;
		
		/* enable interlaced cursor */
		pReg->DacRegs[20] |= 0x20;
	}

	pReg->ExtVga[0]	|= (wd & 0x300) >> 4;
	pReg->ExtVga[1]	= (((ht - 4) & 0x100) >> 8) |
				((hd & 0x100) >> 7) |
				((hs & 0x100) >> 6) |
				(ht & 0x40);
	pReg->ExtVga[2]	= ((vt & 0xc00) >> 10) |
				((vd & 0x400) >> 8) |
				((vd & 0xc00) >> 7) |
				((vs & 0xc00) >> 5);
	if (modeinfo->bitsPerPixel == 24)
		pReg->ExtVga[3]	= (((1 << BppShift) * 3) - 1) | 0x80;
	else
		pReg->ExtVga[3]	= ((1 << BppShift) - 1) | 0x80;

	/* Set viddelay (CRTCEXT3 Bits 3-4). */
	pReg->ExtVga[3] |= (mil_memory == 8192 ? 0x10
        		     : mil_memory == 2048 ? 0x08 : 0x00);

	pReg->ExtVga[4]	= 0;
		
	regs[0]	= ht - 4;
	regs[1]	= hd;
	regs[2]	= hd;
	regs[3]	= (ht & 0x1F) | 0x80;
	regs[4]	= hs;
	regs[5]	= ((ht & 0x20) << 2) | (he & 0x1F);
	regs[6]	= vt & 0xFF;
	regs[7]	= ((vt & 0x100) >> 8 ) |
				((vd & 0x100) >> 7 ) |
				((vs & 0x100) >> 6 ) |
				((vd & 0x100) >> 5 ) |
				0x10 |
				((vt & 0x200) >> 4 ) |
				((vd & 0x200) >> 3 ) |
				((vs & 0x200) >> 2 );
	regs[9]	= ((vd & 0x200) >> 4) | 0x40; 
	regs[16] = vs & 0xFF;
	regs[17] = (ve & 0x0F) | 0x20;
	regs[18] = vd & 0xFF;
	regs[19] = wd & 0xFF;
	regs[21] = vd & 0xFF;
	regs[22] = (vt + 1) & 0xFF;

	if (modetiming->flags & DOUBLESCAN)
		regs[9] |= 0x80;
    
	/* Per DDK vid.c line 75, sync polarity should be controlled
	 * via the TVP3026 RAMDAC register 1D and so MISC Output Register
	 * should always have bits 6 and 7 set. */

	regs[59] |= 0xC0;
	if ((modetiming->flags & (PHSYNC | NHSYNC)) &&
	    (modetiming->flags & (PVSYNC | NVSYNC)))
	{
	    if (modetiming->flags & PHSYNC)
		pReg->DacRegs[index_1d] |= 0x01;
	    if (modetiming->flags & PVSYNC)
		pReg->DacRegs[index_1d] |= 0x02;
	}
	else
	{
	  int VDisplay = modetiming->CrtcVDisplay;
	  if (modetiming->flags & DOUBLESCAN)
	    VDisplay *= 2;
	  if      (VDisplay < 400)
		  pReg->DacRegs[index_1d] |= 0x01; /* +hsync -vsync */
	  else if (VDisplay < 480)
		  pReg->DacRegs[index_1d] |= 0x02; /* -hsync +vsync */
	  else if (VDisplay < 768)
		  pReg->DacRegs[index_1d] |= 0x00; /* -hsync -vsync */
	  else
		  pReg->DacRegs[index_1d] |= 0x03; /* +hsync +vsync */
	}
	
	pReg->Option = 0x402C0100;  /* fine for 2064 and 2164 */
        
        if(interleave) {
            pReg->Option |= 0x1000;
        } else {
            pReg->Option &= ~0x1000;
        }
	/* must always have the pci retries on but rely on 
	   polling to keep them from occuring */
	pReg->Option &= ~0x20000000;

	regs[59] |= 0x0C; 
	/* XXX Need to check the first argument */
	MGATi3026SetPCLK( pReg, modetiming->pixelClock, 1 << BppShift );

	/* this one writes registers rather than writing to the 
	   mgaReg->ModeReg and letting Restore write to the hardware
	   but that's no big deal since we will Restore right after
	   this function */

#if 0
	MGATi3026SetMCLK(pScrn, MGAdac->MemoryClock);
#endif

	/* This disables the VGA memory aperture */
        regs[59] |= 0x02;
	return 1;
}

/*
 * MGA3026Restore -- for mga2064 with ti3026
 *
 * This function restores a video mode.	 It basically writes out all of
 * the registers that have previously been saved in the vgaMGARec data 
 * structure.
 */
static void 
MGA3026Restore(MGARegPtr mgaReg)
{
	int i;

	/*
	 * Code is needed to get things back to bank zero.
	 */
	for (i = 0; i < 6; i++)
		mil_outext(i, mgaReg->ExtVga[i]);

	i = __svgalib_pci_read_config_dword(mil_pciposition, PCI_OPTION_REG);
	i &= ~OPTION_MASK;
        i |= mgaReg->Option&OPTION_MASK;
	__svgalib_pci_write_config_dword(mil_pciposition, PCI_OPTION_REG, i);

	/* select pixel clock PLL as clock source */
	outTi3026(TVP3026_CLK_SEL, 0, mgaReg->DacRegs[3]);
	
	/* set loop and pixel clock PLL PLLEN bits to 0 */
	outTi3026(TVP3026_PLL_ADDR, 0, 0x2A);
	outTi3026(TVP3026_LOAD_CLK_DATA, 0, 0);
	outTi3026(TVP3026_PIX_CLK_DATA, 0, 0);
	 
	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(TVP3026_PIX_CLK_DATA, 0, mgaReg->DacClk[i]);

#if 0
	if (vgaReg->MiscOutReg & 0x08) {
		/* poll until pixel clock PLL LOCK bit is set */
		outTi3026(TVP3026_PLL_ADDR, 0, 0x3F);
		while ( ! (inTi3026(TVP3026_PIX_CLK_DATA) & 0x40) );
	}
#else
        usleep(1);
#endif
	/* set Q divider for loop clock PLL */
	outTi3026(TVP3026_MCLK_CTL, 0, mgaReg->DacRegs[18]);
	
	/* program loop PLL */
	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(TVP3026_LOAD_CLK_DATA, 0, mgaReg->DacClk[i]);

#if 0
	if ((vgaReg->MiscOutReg & 0x08) && ((mgaReg->DacClk[3] & 0xC0) == 0xC0) ) {
		/* poll until loop PLL LOCK bit is set */
		outTi3026(TVP3026_PLL_ADDR, 0, 0x3F);
		while ( ! (inTi3026(TVP3026_LOAD_CLK_DATA) & 0x40) );
	}
#else
        usleep(1);
#endif

	/*
	 * restore other DAC registers
	 */
	for (i = 0; i < DACREGSIZE; i++)
		outTi3026(MGADACregs[i], 0, mgaReg->DacRegs[i]);

        OUTREG(0x1e54, mgaReg->opmode);

}

/*
 * MGA3026Save -- for mga2064 with ti3026
 *
 * This function saves the video state.
 */
static void
MGA3026Save(MGARegPtr mgaReg)
{
	int i;
	
	mil_setpage(0);
	
	/*
	 * The port I/O code necessary to read in the extended registers 
	 * into the fields of the vgaMGARec structure.
	 */
	for (i = 0; i < 6; i++)
	{
		mgaReg->ExtVga[i] = mil_inext(i);
	}

	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 0; i < 3; i++)
		outTi3026(TVP3026_PIX_CLK_DATA, 0, mgaReg->DacClk[i] =
					inTi3026(TVP3026_PIX_CLK_DATA));

	outTi3026(TVP3026_PLL_ADDR, 0, 0x00);
	for (i = 3; i < 6; i++)
		outTi3026(TVP3026_LOAD_CLK_DATA, 0, mgaReg->DacClk[i] =
					inTi3026(TVP3026_LOAD_CLK_DATA));
	for (i = 0; i < DACREGSIZE; i++)
		mgaReg->DacRegs[i]	 = inTi3026(MGADACregs[i]);
	
	mgaReg->Option = __svgalib_pci_read_config_dword(mil_pciposition, PCI_OPTION_REG);
        mgaReg->opmode = INREG(0x1e54);
}


/* Read and save chipset-specific registers */
static int mil_saveregs(unsigned char regs[])
{ 
    mil_unlock();
    MGA3026Save((MGARegPtr)(regs+60));    
    
    return MIL_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void mil_setregs(const unsigned char regs[], int mode)
{  
    mil_unlock();		
    MGA3026Restore((MGARegPtr)(regs+60));
}


/* Return nonzero if mode is available */

static int mil_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];

    if (mil_memory * 1024 < info->ydim * info->xbytes)
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

/* Local, called by mil_setmode(). */

static void mil_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);
    MGA3026Init(moderegs, modetiming, modeinfo);

    return ;
}


static int mil_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;
    int i;
    
    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!mil_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(MIL_TOTAL_REGS + 60);

    mil_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    mil_setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    if(mode>=G640x480x256)
        for(i=0;i<256;i++) __svgalib_outpal(i,i,i,i);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void mil_unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
}

static void mil_lock(void)
{
}


#define VENDOR_ID 0x102b

/* Indentify chipset, initialize and return non-zero if detected */

static int mil_test(void)
{
    unsigned long buf[64];
    int found;
    int id;
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    id=(buf[0]>>16)&0xffff;
    if(found)return 0;
    switch(id) {
        case 0x519:
        case 0x51b:
        case 0x51f:
            if (mil_init(0,0,0) != 0)
                return 0;
            return 1;
            break;
        default:
            return 0;
    }
}


/* Set display start address (not for 16 color modes) */

static void mil_setdisplaystart(int address)
{ 
  address=address >> (2+interleave);
  __svgalib_outcrtc(0x0d,address&0xff);
  __svgalib_outcrtc(0x0c,(address>>8)&0xff);
  mil_outext(0, (mil_inext(0) & 0xf0) | ((address&0xf0000)>>16));
}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void mil_setlogicalwidth(int width)
{   
    int wd; 
    
    wd = width >> (3+interleave);
    if(wd&7) {
       wd=(wd|7)+1;
    }
    __svgalib_outcrtc(0x13,wd&0xff);
    mil_outext(0, (mil_inext(0) & 0xcf) | ((wd&0x300)>>4));
}

static int mil_linear(int op, int param)
{
    if (op==LINEAR_ENABLE){mil_is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){mil_is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return mil_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int mil_match_programmable_clock(int clock)
{
return clock ;
}

static int mil_map_clock(int bpp, int clock)
{
return clock ;
}

static int mil_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

/* Function table (exported) */

DriverSpecs __svgalib_mil_driverspecs =
{
    mil_saveregs,
    mil_setregs,
    mil_unlock,
    mil_lock,
    mil_test,
    mil_init,
    mil_setpage,
    NULL,
    NULL,
    mil_setmode,
    mil_modeavailable,
    mil_setdisplaystart,
    mil_setlogicalwidth,
    mil_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    mil_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
};

static void __svgalib_mil_inpal(int i, int *r, int *g, int *b)
{
    outTi3026dreg(TVP3026_RADR_PAL,i);
    *r=inTi3026dreg(TVP3026_COL_PAL);
    *g=inTi3026dreg(TVP3026_COL_PAL);
    *b=inTi3026dreg(TVP3026_COL_PAL);
}

static void __svgalib_mil_outpal(int i, int r, int g, int b)
{

    outTi3026dreg(TVP3026_WADR_PAL,i);
    outTi3026dreg(TVP3026_COL_PAL,r);
    outTi3026dreg(TVP3026_COL_PAL,g);
    outTi3026dreg(TVP3026_COL_PAL,b);
}

/* Initialize chipset (called after detection) */

static int mil_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0, id;

    if (force) {
	mil_memory = par1;
        mil_chiptype = par2;
    } else {
    };

    found=__svgalib_pci_find_vendor_vga_pos(VENDOR_ID,buf,0);
    mil_linear_base=0;
    id=(buf[0]>>16)&0xffff;
    if(found != -1) {
        mil_pciposition=found;
        switch(id) {
            case 0x519: /* Millennium */
            case 0x51b: /* Millennium II */
            case 0x51f: /* Millennium II */
                break;
            
            default:
                return 1;
        }
    } else {
        return 1;
    }

    if(id==0x519) {
        mil_linear_base = buf[5]&0xffffff00;
        mil_mmio_base = buf[4]&0xffffff00;
    } else {
        mil_linear_base = buf[4]&0xffffff00;
        mil_mmio_base = buf[5]&0xffffff00;
    }

    if(!mil_memory) {
        map_linear(mil_linear_base, 8*1024*1024);
        if (LINEAR_POINTER == MAP_FAILED) {
            mil_memory=8*1024;
        } else {
            mil_memory=memorytest(LINEAR_POINTER, 8);
            unmap_linear(8*1024*1024);
        }
    }

    __svgalib_mmio_base=mil_mmio_base;
    __svgalib_mmio_size=16384;
    map_mmio();
    __svgalib_vgammbase = 0x1c00;
    __svgalib_mm_io_mapio();

    __svgalib_inpal=__svgalib_mil_inpal;
    __svgalib_outpal=__svgalib_mil_outpal;    

    if(mil_memory > 2048 ) {
        interleave=1;
        rounding=127;
    } else {
        interleave=0;
        rounding=63;
        BppShifts[0]++;
        BppShifts[1]++;
        BppShifts[2]++;
        BppShifts[3]++;
    }

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Millennium driver, %iKB.\n",mil_memory);
    };
    
	__svgalib_modeinfo_linearset |= IS_LINEAR;
	
    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = mil_memory;
    cardspecs->maxPixelClock4bpp = 0;	
    cardspecs->maxPixelClock8bpp = 220000;	
    cardspecs->maxPixelClock16bpp = 220000;	
    cardspecs->maxPixelClock24bpp = 220000;
    cardspecs->maxPixelClock32bpp = 220000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4080;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = mil_map_clock;
    cardspecs->mapHorizontalCrtc = mil_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=mil_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_mil_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=mil_linear_base;
    __svgalib_linear_mem_size=mil_memory*0x400;
    __svgalib_mmio_base=mil_mmio_base;
    __svgalib_mmio_size=16384;
    return 0;
}
