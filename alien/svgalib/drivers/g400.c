/*
Matrox Mystique(1064)/G100/G200/G400/G450 chipset driver 

Based on the XFree86 (4.1.0) mga driver.

Tested only on G450 and Mystique. 

TODO: SDRAM, reference frequency checking.

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
#include "mga.h"
#define SECONDCRTC 0
#include "mga_g450.c"
#include "vgammvga.h"

#define SKREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define G400_TOTAL_REGS (VGA_TOTAL_REGS + 32 + 176 + 4 + 32)

enum { ID_1064 = 0, ID_G100, ID_G200, ID_G400, ID_G450 };

static int g400_init(int, int, int);
static void g400_unlock(void);
static void g400_lock(void);

static int g400_memory, id, g400_pciposition;
static int g400_is_linear, g400_linear_base, g400_mmio_base;

static int HasSDRAM;

static CardSpecs *cardspecs;

static int g400_inExt(int i) {
    v_writeb(i, 0x1fde);
    return v_readb(0x1fdf);
}

static void g400_outExt(int i, int d) {
    v_writeb(i, 0x1fde);
    v_writeb(d, 0x1fdf);
}

static int g400_inDAC(int i) {
    v_writeb(i, 0x3c00);
    return v_readb(0x3c0a);
}

static void g400_outDAC(int i, int d) {
    v_writeb(i, 0x3c00);
    v_writeb(d, 0x3c0a);
}

static void inpal(int i, int *r, int *g, int *b)
{
    v_writeb(i, 0x3c03);
    *r = v_readb(0x3c01);
    *g = v_readb(0x3c01);
    *b = v_readb(0x3c01);
}

static void outpal(int i, int r, int g, int b)
{
    v_writeb(i, 0x3c00);
    v_writeb(r, 0x3c01);
    v_writeb(g, 0x3c01);
    v_writeb(b, 0x3c01);
}

static void g400_setpage(int page)
{
    g400_outExt(4,page);
}

static int __svgalib_g400_inlinearmode(void)
{
    return g400_is_linear;
}

/* Fill in chipset specific mode information */

static void g400_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = g400_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 8184;
    modeinfo->startaddressrange = g400_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
	if(g400_linear_base)modeinfo->flags |= CAPABLE_LINEAR;
        if (__svgalib_g400_inlinearmode())
	    modeinfo->flags |= IS_LINEAR | LINEAR_MODE;
    }
}

/* Read and save chipset-specific registers */
static int g400_saveregs(unsigned char regs[])
{ 
    int i;
    unsigned int *iregs=(unsigned int *)(regs+VGA_TOTAL_REGS);

    g400_unlock();		

    iregs[4] = __svgalib_pci_read_config_dword(g400_pciposition, 0x40);

    if(id>ID_1064) iregs[5] = __svgalib_pci_read_config_dword(g400_pciposition, 0x50);

    if(id>=ID_G400) iregs[6] = __svgalib_pci_read_config_dword(g400_pciposition, 0x54);

    for(i=0;i<0xb0;i++) regs[VGA_TOTAL_REGS + 32 + i]=g400_inDAC(i);
    for(i=0;i<9;i++) regs[VGA_TOTAL_REGS + i]=g400_inExt(i);

    regs[VGA_TOTAL_REGS + 15] = 1;

    iregs[7]=INREG(0x1e54);
    iregs[52]=INREG(0x3c0c);

    iregs[53]=INREG(MGAREG_C2DATACTL);
    iregs[54]=INREG(MGAREG_C2HPARAM);
    iregs[55]=INREG(MGAREG_C2HSYNC);
    iregs[56]=INREG(MGAREG_C2VPARAM);
    iregs[57]=INREG(MGAREG_C2VSYNC);
    iregs[58]=INREG(MGAREG_C2OFFSET);
    iregs[59]=INREG(MGAREG_C2STARTADD0);
    iregs[60]=INREG(MGAREG_C2CTL);

    return G400_TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void g400_setregs(const unsigned char regs[], int mode)
{  
    int i;
    unsigned int t;
    unsigned int *iregs=(unsigned int *)(regs+VGA_TOTAL_REGS);

inrestore=1;
	
    g400_unlock();		
    for(i=0;i<0x50;i++) {
#if 0
        if( (i> 0x03) && (i!=0x07) && (i!=0x0b) && (i!=0x0f) &&
            (i< 0x13) && (i> 0x17) && (i!=0x1b) && (i!=0x1c) &&
            (i< 0x1f) && (i> 0x29) && (i< 0x30) && (i> 0x37) &&
            (i!=0x39) && (i!=0x3b) && (i!=0x3f) && (i!=0x41) &&
            (i!=0x43) && (i!=0x47) && (i!=0x4b) 
            			 				)
#endif
    if( (id!=ID_G450) || (regs[VGA_TOTAL_REGS + 15]) || (i<0x4c) || (i>0x4e) )
            g400_outDAC(i,regs[VGA_TOTAL_REGS + 32 + i]);
    }

    t = __svgalib_pci_read_config_dword(g400_pciposition, 0x40);
    t &= ~OPTION1_MASK;
    t |= iregs[4]&OPTION1_MASK;
    __svgalib_pci_write_config_dword(g400_pciposition, 0x40, t);

    if(id>ID_1064) {
        t = __svgalib_pci_read_config_dword(g400_pciposition, 0x50);
        t &= ~OPTION2_MASK;
        t |= iregs[5]&OPTION2_MASK;
        __svgalib_pci_write_config_dword(g400_pciposition, 0x50, t);
    }
    
    if(id>=ID_G400) {
        t = __svgalib_pci_read_config_dword(g400_pciposition, 0x54);
        t &= ~OPTION3_MASK;
        t |= iregs[6]&OPTION3_MASK;
        __svgalib_pci_write_config_dword(g400_pciposition, 0x54, t);
    }

    OUTREG(0x1e54, iregs[7]);
    OUTREG(0x3c0c, iregs[52]);

    for(i=0;i<6;i++) g400_outExt(i, regs[VGA_TOTAL_REGS + i]);

inrestore=0;
	
}


/* Return nonzero if mode is available */

static int g400_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (g400_memory * 1024 < info->ydim * info->xbytes)
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

#define MGA_MIN_VCO_FREQ     50000
#define MGA_MAX_VCO_FREQ    310000

static double
MGACalcClock (long f_out,
		int *best_m, int *best_n, int *p, int *s )
{
	int m, n;
	double f_pll, f_vco;
	double m_err, calc_f;
	double ref_freq;
	int feed_div_min, feed_div_max;
	int in_div_min, in_div_max;
	int post_div_max;

	*best_m = 0;
	*best_n = 0;

	switch( id )
	{
	case ID_1064:
		ref_freq     = 14318.18;
		feed_div_min = 100;
		feed_div_max = 127;
		in_div_min   = 1;
		in_div_max   = 31;
		post_div_max = 7;
		break;
	case ID_G400:
	case ID_G450:
		ref_freq     = 27050.5;
		feed_div_min = 7;
		feed_div_max = 127;
		in_div_min   = 1;
		in_div_max   = 31;
		post_div_max = 7;
		break;
	case ID_G100:
	case ID_G200:
	default:
		if ( 0 /* pMga->Bios2.PinID && (pMga->Bios2.VidCtrl & 0x20) */ )
			ref_freq = 14318.18;
		else
			ref_freq = 27050.5;
		feed_div_min = 7;
		feed_div_max = 127;
		in_div_min   = 1;
		in_div_max   = 6;
		post_div_max = 7;
		break;
	}

	/* Make sure that f_min <= f_out */
	if ( f_out < ( MGA_MIN_VCO_FREQ / 8))
		f_out = MGA_MIN_VCO_FREQ / 8;

	/*
	 * f_pll = f_vco / (p+1)
	 * Choose p so that MGA_MIN_VCO_FREQ   <= f_vco <= MGA_MAX_VCO_FREQ  
	 * we don't have to bother checking for this maximum limit.
	 */
	f_vco = ( double ) f_out;
	for ( *p = 0; *p <= post_div_max && f_vco < MGA_MIN_VCO_FREQ;
		*p = *p * 2 + 1, f_vco *= 2.0);

	/* Initial amount of error for frequency maximum */
	m_err = f_out;

	/* Search for the different values of ( m ) */
	for ( m = in_div_min ; m <= in_div_max ; m++ )
	{
		/* see values of ( n ) which we can't use */
		for ( n = feed_div_min; n <= feed_div_max; n++ )
		{ 
			calc_f = ref_freq * (n + 1) / (m + 1) ;

			/*
			 * Pick the closest frequency.
			 */
			if ( abs(calc_f - f_vco) < m_err ) {
				m_err = abs(calc_f - f_vco);
				*best_m = m;
				*best_n = n;
			}
		}
	}
	
	/* Now all the calculations can be completed */
	f_vco = ref_freq * (*best_n + 1) / (*best_m + 1);

	/* Adjustments for filtering pll feed back */
	if ( (50000.0 <= f_vco)
	&& (f_vco < 100000.0) )
		*s = 0;	
	if ( (100000.0 <= f_vco)
	&& (f_vco < 140000.0) )
		*s = 1;	
	if ( (140000.0 <= f_vco)
	&& (f_vco < 180000.0) )
		*s = 2;	
	if ( (180000.0 <= f_vco) )
		*s = 3;	

	f_pll = f_vco / ( *p + 1 );

	return f_pll;
}

static int MGASetPCLK(long f_out, unsigned char *initDAC )
{
	/* Pixel clock values */
	int m, n, p, s;

	/* The actual frequency output by the clock */
	double f_pll;

	if(id==ID_G450) {
		G450SetPLLFreq(f_out);
		return 1;
	}

	/* Do the calculations for m, n, p and s */
	f_pll = MGACalcClock(f_out, &m, &n, &p, &s );

	/* Values for the pixel clock PLL registers */
        initDAC[ MGA1064_PIX_PLLC_M ] = m & 0x1F;
	initDAC[ MGA1064_PIX_PLLC_N ] = n & 0x7F;
        initDAC[ MGA1064_PIX_PLLC_P ] = (p & 0x07) | ((s & 0x03) << 3);
        return 0;
}


static void g400_initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
	const static unsigned char initDac[] = {
	/* 0x00: */	   0,    0,    0,    0,    0,    0, 0x00,    0,
	/* 0x08: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x10: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x18: */	0x00,    0, 0xC9, 0xFF, 0xBF, 0x20, 0x1F, 0x20,
	/* 0x20: */	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x28: */	0x00, 0x00, 0x00, 0x00,    0,    0,    0, 0x40,
	/* 0x30: */	0x00, 0xB0, 0x00, 0xC2, 0x34, 0x14, 0x02, 0x83,
	/* 0x38: */	0x00, 0x93, 0x00, 0x77, 0x00, 0x00, 0x00, 0x3A,
	/* 0x40: */	   0,    0,    0,    0,    0,    0,    0,    0,
	/* 0x48: */	   0,    0,    0,    0,    0,    0,    0,    0
	};
        unsigned char initDAC[0x50];
	int hd, hs, he, ht, vd, vs, ve, vt, wd;
	int i;
	int weight555 = 0;
        int MGABppShft = 0;

        memcpy(initDAC, initDac, 0x50);
        __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

	switch(id)
	{
		case ID_1064:
			initDAC[ MGA1064_SYS_PLL_M ] = 0x04;
			initDAC[ MGA1064_SYS_PLL_N ] = 0x44;
			initDAC[ MGA1064_SYS_PLL_P ] = 0x18;
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16)  = 0x5F094F21;
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 4) = 0x00000000;
			break;
 		case ID_G100:
			initDAC[ MGAGDAC_XVREFCTRL ] = 0x03;
			if(HasSDRAM) {
				initDAC[ MGA1064_SYS_PLL_M ] = 0x01;
				initDAC[ MGA1064_SYS_PLL_N ] = 0x0E;
				initDAC[ MGA1064_SYS_PLL_P ] = 0x18;
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) = 0x404991a9;
			} else {
				initDAC[ MGA1064_SYS_PLL_M ] = 0x04;
				initDAC[ MGA1064_SYS_PLL_N ] = 0x16;
				initDAC[ MGA1064_SYS_PLL_P ] = 0x08;
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) = 0x4049d121;
			}
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 4) = 0x0000007;
			break;
		case ID_G400:
			if( 0 /* pMga->Dac.maxPixelClock == 360000 */) {  /* G400 MAX */
				initDAC[ MGA1064_SYS_PLL_M ] = 0x02;
				initDAC[ MGA1064_SYS_PLL_N ] = 0x1B;
				initDAC[ MGA1064_SYS_PLL_P ] = 0x18;
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 8) = 0x019B8419;
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) = 0x5053C120;
			} else {
				initDAC[ MGA1064_SYS_PLL_M ] = 0x13;
				initDAC[ MGA1064_SYS_PLL_N ] = 0x7A;
				initDAC[ MGA1064_SYS_PLL_P ] = 0x08;
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 8) = 0x0190a421;
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) = 0x50044120;
			} 
			if(HasSDRAM)
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) &= ~(1 << 14);
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 4) = 0x01003000;
			break;
		case ID_G450:
			initDAC[ 0x2c ] = 0x05;
			initDAC[ 0x2d ] = 0x23;
			initDAC[ 0x2e ] = 0x40;
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 0) = 0x40341160;
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 4) = 0x01003000;
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 8) = 0x0190a421;
			if(!HasSDRAM)
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 0) |= (1 << 14);
			break;        
		case ID_G200:
		default:
			initDAC[ MGA1064_SYS_PLL_M ] = 0x04;
			initDAC[ MGA1064_SYS_PLL_N ] = 0x18;
			initDAC[ MGA1064_SYS_PLL_P ] = 0x08;
			*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 4) = 0x00008000;
			if(HasSDRAM)
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) = 0x40499121;
			else
				*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) = 0x4049cd21;
			break;
	}

	*(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16) &= ~0x20000000;
        *(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 12)=INREG(0x1e54);

	switch(modeinfo->bitsPerPixel)
	{
	case 8:
		initDAC[ 0x19 ] = 0;
                initDAC[ 0x1e ] &= ~8;
                MGABppShft=0;
		break;
        case 15:
	case 16:
		initDAC[ 0x19 ] = 2;
                MGABppShft=1;
		if ( modeinfo->greenWeight==5 ) {
			weight555 = 1;
			initDAC[ 0x19 ] = 1;
		}
#if __BYTE_ORDER == __BIG_ENDIAN
                *(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 12) &= ~0x300; 
                /* the aperture is little endian */
                *(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 12) |=  0x100;
#endif
		break;
	case 24:
                MGABppShft=0;
		initDAC[ 0x19 ] = 3;
		break;
	case 32:
                MGABppShft=2;
		initDAC[ 0x19 ] = 7;
#if __BYTE_ORDER == __BIG_ENDIAN
                *(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 12) &= ~0x300; 
                /* the aperture is little endian */
                *(unsigned int *)(moderegs + VGA_TOTAL_REGS + 16 + 12) |=  0x200;
#endif
		break;
	}
		
	/*
	 * Here all of the other fields of 'newVS' get filled in.
	 */
	hd = (modetiming->CrtcHDisplay	>> 3)	- 1;
	hs = (modetiming->CrtcHSyncStart>> 3)	- 1;
	he = (modetiming->CrtcHSyncEnd	>> 3)	- 1;
	ht = (modetiming->CrtcHTotal	>> 3)	- 1;
	vd = modetiming->CrtcVDisplay		- 1;
	vs = modetiming->CrtcVSyncStart		- 1;
	ve = modetiming->CrtcVSyncEnd		- 1;
	vt = modetiming->CrtcVTotal		- 2;
	
	/* HTOTAL & 0xF equal to 0xE in 8bpp or 0x4 in 24bpp causes strange
	 * vertical stripes
	 */  
	if((ht & 0x0F) == 0x0E || (ht & 0x0F) == 0x04)
		ht++;
		
	if (modeinfo->bitsPerPixel == 24)
		wd = (modeinfo->width * 3) >> (4 - MGABppShft);
	else
		wd = modeinfo->width >> (4 - MGABppShft);

	moderegs[VGA_TOTAL_REGS + 0] = 0;
	moderegs[VGA_TOTAL_REGS + 5] = 0;
	
	if (modetiming->flags & INTERLACED)
	{
		moderegs[VGA_TOTAL_REGS + 0] = 0x80;
		moderegs[VGA_TOTAL_REGS + 5] = (hs + he - ht) >> 1;
		wd <<= 1;
		vt &= 0xFFFE;
	}

	moderegs[VGA_TOTAL_REGS + 0]	|= (wd & 0x300) >> 4;
	moderegs[VGA_TOTAL_REGS + 1]	= (((ht - 4) & 0x100) >> 8) |
				((hd & 0x100) >> 7) |
				((hs & 0x100) >> 6) |
				(ht & 0x40);
	moderegs[VGA_TOTAL_REGS + 2]	= ((vt & 0x400) >> 10) |
				((vt & 0x800) >> 10) |
				((vd & 0x400) >> 8) |
				((vd & 0x400) >> 7) |
				((vd & 0x800) >> 7) |
				((vs & 0x400) >> 5) |
				((vs & 0x800) >> 5);
	if (modeinfo->bitsPerPixel == 24)
		moderegs[VGA_TOTAL_REGS + 3]	= (((1 << MGABppShft) * 3) - 1) | 0x80;
	else
		moderegs[VGA_TOTAL_REGS + 3]	= ((1 << MGABppShft) - 1) | 0x80;

	moderegs[VGA_TOTAL_REGS + 3] &= 0xE7;	/* ajv - bits 4-5 MUST be 0 or bad karma happens */

	moderegs[VGA_TOTAL_REGS + 4]	= 0;
		
	moderegs[0]	= ht - 4;
	moderegs[1]	= hd;
	moderegs[2]	= hd;
	moderegs[3]	= (ht & 0x1F) | 0x80;
	moderegs[4]	= hs;
	moderegs[5]	= ((ht & 0x20) << 2) | (he & 0x1F);
	moderegs[6]	= vt & 0xFF;
	moderegs[7]	= ((vt & 0x100) >> 8 ) |
				((vd & 0x100) >> 7 ) |
				((vs & 0x100) >> 6 ) |
				((vd & 0x100) >> 5 ) |
				0x10 |
				((vt & 0x200) >> 4 ) |
				((vd & 0x200) >> 3 ) |
				((vs & 0x200) >> 2 );
	moderegs[9]	= ((vd & 0x200) >> 4) | 0x40; 
	moderegs[16] = vs & 0xFF;
	moderegs[17] = (ve & 0x0F) | 0x20;
	moderegs[18] = vd & 0xFF;
	moderegs[19] = wd & 0xFF;
	moderegs[21] = vd & 0xFF;
	moderegs[22] = (vt + 1) & 0xFF;

	if (modetiming->flags & DOUBLESCAN)
		moderegs[9] |= 0x80;
    
	moderegs[59] |= 0x0C;

        if(id==ID_G450) OUTREG(MGAREG_ZORG, 0);

        if(MGASetPCLK( modetiming->pixelClock , initDAC))
            moderegs[VGA_TOTAL_REGS + 15] = 0;

	for (i = 0; i < sizeof(initDAC); i++)
	{
	    moderegs[VGA_TOTAL_REGS + 32 + i] = initDAC[i]; 
	}

    return ;
}


static int g400_setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;
    int i;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!g400_modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(G400_TOTAL_REGS);

    g400_initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */

    g400_setregs(moderegs, mode);		/* Set extended regs. */

    if(mode>=G640x480x256)
	switch(modeinfo->bitsPerPixel) {
            case 16:
                for(i=0;i<256;i++) outpal(i,i<<3,i<<(8-modeinfo->greenWeight),i<<3);
                break;
            case 24:
            case 32:
                for(i=0;i<256;i++) outpal(i,i,i,i);
                break;
        }

    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void g400_unlock(void)
{
    __svgalib_outcrtc(0x11, __svgalib_incrtc(0x11) &0x7f);
    g400_outExt(3, g400_inExt(3) & 0x7f);
}

static void g400_lock(void)
{
    __svgalib_outcrtc(0x11, __svgalib_incrtc(0x11)&0x7f);
    g400_outExt(3, g400_inExt(3) | 0x80);
}


#define VENDOR_ID 0x102b

/* Indentify chipset, initialize and return non-zero if detected */

static int g400_test(void)
{
    int found, id;
    unsigned long buf[64];
    
    found=__svgalib_pci_find_vendor_vga(VENDOR_ID,buf,0);
    
    if(found) return 0;
    
    id=(buf[0]>>16)&0xffff;
    
    if((id==0x51a)||(id==0x51e)||(id==0x520)||(id==0x521)||(id==0x525)||(id==0x1000)||(id==0x1001)
                  ||(id==0x2527) ){
       if (g400_init(0,0,0) != 0)
          return 0;
       return 1;
    };
    return 0;
}


/* Set display start address (not for 16 color modes) */

static void g400_setdisplaystart(int address)
{ 
  address=address >> 3;
  __svgalib_outcrtc(0x0c, (address & 0xFF00)>>8);
  __svgalib_outcrtc(0x0d,  address & 0x00FF);
  g400_outExt(0, (g400_inExt(0)&0xb0) | ((address&0xf0000)>>16) | ((address&0x100000)>>14));
}


/* Set logical scanline length (usually multiple of 8) */

static void g400_setlogicalwidth(int width)
{   
    int offset = width >> 4;
 
    __svgalib_outcrtc(0x13,offset&0xff);
    g400_outExt(0,(g400_inExt(0)&0xcf) | ((offset&0x300)>>4));
}

static int g400_linear(int op, int param)
{
    if (op==LINEAR_ENABLE){g400_is_linear=1; return 0;};
    if (op==LINEAR_DISABLE){g400_is_linear=0; return 0;};
    if (op==LINEAR_QUERY_BASE) return g400_linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
        else return -1;		/* Unknown function. */
}

static int g400_match_programmable_clock(int clock)
{
return clock ;
}

static int g400_map_clock(int bpp, int clock)
{
return clock ;
}

static int g400_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

static unsigned int cur_colors[16*2];

static int g400_cursor( int cmd, int p1, int p2, int p3, int p4, void *p5) {
    int i, j;
    unsigned char *b3;
    
    switch(cmd){
        case CURSOR_INIT:
            return 1;
        case CURSOR_HIDE:
            g400_outDAC(6,0);
            break;
        case CURSOR_SHOW:
            g400_outDAC(6,3);
            break;
        case CURSOR_POSITION:
            p1+=64;
            p2+=64;
            *(MMIO_POINTER + 0x3c0c) = p1&0xff;
            *(MMIO_POINTER + 0x3c0d) = (p1>>8)&0x0f;
            *(MMIO_POINTER + 0x3c0e) = p2&0xff;
            *(MMIO_POINTER + 0x3c0f) = (p2>>8)&0x0f;
        break;
        case CURSOR_SELECT:
            i=g400_memory*1024-(p1+1)*4096;
            while(i>16384*1024)i-=8192*1024;
            g400_outDAC(4,(i>>10)&0xff);
            g400_outDAC(5,(i>>18)&0x3f);
            
            g400_outDAC(8, (cur_colors[p1*2]>>16)&0xff);
            g400_outDAC(9, (cur_colors[p1*2]>>8)&0xff);
            g400_outDAC(10, cur_colors[p1*2]&0xff);
            g400_outDAC(12,(cur_colors[p1*2+1]>>16)&0xff);
            g400_outDAC(13,(cur_colors[p1*2+1]>>8)&0xff);
            g400_outDAC(14, cur_colors[p1*2+1]&0xff);
            break;
        case CURSOR_IMAGE:
            i=g400_memory*1024-(p1+1)*4096;
            while(i>16384*1024)i-=8192*1024;
            b3=(unsigned char *)p5;
            switch(p2) {
                case 0:
                    cur_colors[p1*2]=p3;
                    cur_colors[p1*2+1]=p4;
                    for(j=0;j<32;j++) {
                        *(LINEAR_POINTER+i+16*j)=0;
                        *(LINEAR_POINTER+i+16*j+1)=0;
                        *(LINEAR_POINTER+i+16*j+2)=0;
                        *(LINEAR_POINTER+i+16*j+3)=0;
                        *(LINEAR_POINTER+i+16*j+4)=*(b3+4*j+0);
                        *(LINEAR_POINTER+i+16*j+5)=*(b3+4*j+1);
                        *(LINEAR_POINTER+i+16*j+6)=*(b3+4*j+2);
                        *(LINEAR_POINTER+i+16*j+7)=*(b3+4*j+3);
                        *(LINEAR_POINTER+i+16*j+8)=0;
                        *(LINEAR_POINTER+i+16*j+9)=0;
                        *(LINEAR_POINTER+i+16*j+10)=0;
                        *(LINEAR_POINTER+i+16*j+11)=0;
                        *(LINEAR_POINTER+i+16*j+12)=*(b3+4*j+128+0);
                        *(LINEAR_POINTER+i+16*j+13)=*(b3+4*j+128+1);
                        *(LINEAR_POINTER+i+16*j+14)=*(b3+4*j+128+2);
                        *(LINEAR_POINTER+i+16*j+15)=*(b3+4*j+128+3);
                    }
                    memset(LINEAR_POINTER+i+512,0,512);
                break;
            }
            break;
        default:
            return -1;
    }
    return 0;
}       

/* Function table (exported) */

DriverSpecs __svgalib_g400_driverspecs =
{
    g400_saveregs,
    g400_setregs,
    g400_unlock,
    g400_lock,
    g400_test,
    g400_init,
    g400_setpage,
    NULL,
    NULL,
    g400_setmode,
    g400_modeavailable,
    g400_setdisplaystart,
    g400_setlogicalwidth,
    g400_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    g400_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
    g400_cursor,
};

/* Initialize chipset (called after detection) */

static int g400_init(int force, int par1, int par2)
{
    unsigned long buf[64];
    int found=0;
    int pci_id;
    int max_mem = 8;
    char *ids[]={"Mystique", "G100", "G200", "G400", "G450"};

    if (force) {
	g400_memory = par1;
    } else {
	g400_memory = 0;
    };

    found=__svgalib_pci_find_vendor_vga_pos(VENDOR_ID,buf,0);
    
    if(found==-1) {
        return -1;
    }

    g400_pciposition=found;
    pci_id=(buf[0]>>16)&0xffff;
	
    switch(pci_id) {
		case 0x525:
			if((buf[11]&0xffff0000) == 0x07c00000)
				id = ID_G450; else id = ID_G400;
			max_mem=32;
			break;
		case 0x520:
		case 0x521:
			id = ID_G200;
			max_mem=16;
			break;
		case 0x51a:
		case 0x51e:
			id = ID_1064;
			break;
		case 0x2527:
			id = ID_G450;
		default:
			id = ID_G100;
	}

    g400_linear_base = buf[4]&0xffffff00;
    g400_mmio_base = buf[5]&0xffffff00;
        
    if(id == ID_1064){
        if(__svgalib_pci_read_aperture_len(g400_pciposition, 0) < 1024*1024) {
            g400_linear_base = buf[5]&0xffffff00;
            g400_mmio_base = buf[4]&0xffffff00;
        }
    }

    __svgalib_mmio_base=g400_mmio_base;
    __svgalib_mmio_size=16384;
    map_mmio();
    __svgalib_vgammbase = 0x1c00;
    __svgalib_mm_io_mapio();

    if(!g400_memory) {
        map_linear(g400_linear_base, max_mem*1024*1024);
        if (LINEAR_POINTER == MAP_FAILED) {
            g400_memory=max_mem*1024;
        } else {
            g400_memory=memorytest(LINEAR_POINTER,max_mem);
            unmap_linear(max_mem*1024*1024);
        }
    }

    __svgalib_modeinfo_linearset |= IS_LINEAR;
    
    __svgalib_inpal=inpal;
    __svgalib_outpal=outpal;    

    HasSDRAM=(buf[0x10]&(1<<14))?0:1;
    
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Matrox %s driver, %iKB S%cRAM.\n",ids[id],
           	g400_memory, HasSDRAM?'D':'G');
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = g400_memory;
    cardspecs->maxPixelClock4bpp = 75000;
    switch(id) {
        default:
            cardspecs->maxPixelClock8bpp = 250000;
            cardspecs->maxPixelClock16bpp = 250000;
            cardspecs->maxPixelClock24bpp = 250000;
            cardspecs->maxPixelClock32bpp = 250000;
            break;
    }
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4095;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = g400_map_clock;
    cardspecs->mapHorizontalCrtc = g400_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=g400_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_g400_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=g400_linear_base;
    __svgalib_linear_mem_size=g400_memory*0x400;
    __svgalib_mmio_base=g400_mmio_base;
    __svgalib_mmio_size=16384;
    return 0;
}
