/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Extended for svgalib by Harm Hanemaayer and Hartmut Schirmer */

#ifndef VGA_H
#define VGA_H

#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SVGALIB_VER 0x010900

#define TEXT 	     0		/* Compatible with VGAlib v1.2 */
#define G320x200x16  1
#define G640x200x16  2
#define G640x350x16  3
#define G640x480x16  4
#define G320x200x256 5
#define G320x240x256 6
#define G320x400x256 7
#define G360x480x256 8
#define G640x480x2   9

#define G640x480x256 10
#define G800x600x256 11
#define G1024x768x256 12

#define G1280x1024x256 13	/* Additional modes. */

#define G320x200x32K 14
#define G320x200x64K 15
#define G320x200x16M 16
#define G640x480x32K 17
#define G640x480x64K 18
#define G640x480x16M 19
#define G800x600x32K 20
#define G800x600x64K 21
#define G800x600x16M 22
#define G1024x768x32K 23
#define G1024x768x64K 24
#define G1024x768x16M 25
#define G1280x1024x32K 26
#define G1280x1024x64K 27
#define G1280x1024x16M 28

#define G800x600x16 29
#define G1024x768x16 30
#define G1280x1024x16 31

#define G720x348x2 32		/* Hercules emulation mode */

#define G320x200x16M32 33	/* 32-bit per pixel modes. */
#define G640x480x16M32 34
#define G800x600x16M32 35
#define G1024x768x16M32 36
#define G1280x1024x16M32 37

/* additional resolutions */
#define G1152x864x16 38
#define G1152x864x256 39
#define G1152x864x32K 40
#define G1152x864x64K 41
#define G1152x864x16M 42
#define G1152x864x16M32 43

#define G1600x1200x16 44
#define G1600x1200x256 45
#define G1600x1200x32K 46
#define G1600x1200x64K 47
#define G1600x1200x16M 48
#define G1600x1200x16M32 49

#define G320x240x256V 50
#define G320x240x32K 51
#define G320x240x64K 52
#define G320x240x16M 53
#define G320x240x16M32 54

#define G400x300x256 55
#define G400x300x32K 56
#define G400x300x64K 57
#define G400x300x16M 58
#define G400x300x16M32 59

#define G512x384x256 60
#define G512x384x32K 61
#define G512x384x64K 62
#define G512x384x16M 63
#define G512x384x16M32 64

#define G960x720x256 65
#define G960x720x32K 66
#define G960x720x64K 67
#define G960x720x16M 68
#define G960x720x16M32 69

#define G1920x1440x256 70
#define G1920x1440x32K 71
#define G1920x1440x64K 72
#define G1920x1440x16M 73
#define G1920x1440x16M32 74

/* The following modes have been introduced by SciTech Display Doctor */

#define G320x400x256V 75
#define G320x400x32K 76
#define G320x400x64K 77
#define G320x400x16M 78
#define G320x400x16M32 79

#define G640x400x256 80
#define G640x400x32K 81
#define G640x400x64K 82
#define G640x400x16M 83
#define G640x400x16M32 84

#define G320x480x256 85
#define G320x480x32K 86
#define G320x480x64K 87
#define G320x480x16M 88
#define G320x480x16M32 89

#define G720x540x256 90
#define G720x540x32K 91
#define G720x540x64K 92
#define G720x540x16M 93
#define G720x540x16M32 94

#define G848x480x256 95
#define G848x480x32K 96
#define G848x480x64K 97
#define G848x480x16M 98
#define G848x480x16M32 99

#define G1072x600x256 100
#define G1072x600x32K 101
#define G1072x600x64K 102
#define G1072x600x16M 103
#define G1072x600x16M32 104

#define G1280x720x256 105
#define G1280x720x32K 106
#define G1280x720x64K 107
#define G1280x720x16M 108
#define G1280x720x16M32 109

#define G1360x768x256 110
#define G1360x768x32K 111
#define G1360x768x64K 112
#define G1360x768x16M 113
#define G1360x768x16M32 114

#define G1800x1012x256 115
#define G1800x1012x32K 116
#define G1800x1012x64K 117
#define G1800x1012x16M 118
#define G1800x1012x16M32 119

#define G1920x1080x256 120
#define G1920x1080x32K 121
#define G1920x1080x64K 122
#define G1920x1080x16M 123
#define G1920x1080x16M32 124

#define G2048x1152x256 125
#define G2048x1152x32K 126
#define G2048x1152x64K 127
#define G2048x1152x16M 128
#define G2048x1152x16M32 129

#define G2048x1536x256 130
#define G2048x1536x32K 131
#define G2048x1536x64K 132
#define G2048x1536x16M 133
#define G2048x1536x16M32 134

#define G512x480x256 135
#define G512x480x32K 136
#define G512x480x64K 137
#define G512x480x16M 138
#define G512x480x16M32 139

#define G400x600x256 140
#define G400x600x32K 141
#define G400x600x64K 142
#define G400x600x16M 143
#define G400x600x16M32 144

#define G400x300x256X 145

#define G320x200x256V 146

#define __GLASTMODE G320x200x256V
#define GLASTMODE vga_lastmodenumber()

#define IS_IN_STANDARD_VGA_DRIVER(mode) ( \
	((mode) < G640x480x256) || ((mode) == G720x348x2) || \
	( ((mode) >= G400x300x256X) && ((mode) <= G400x300x256X) ) )

    extern int vga_version;

    extern int vga_setmode(int mode);
    extern int vga_hasmode(int mode);
    extern int vga_setflipchar(int c);

    extern int vga_clear(void);
    extern int vga_flip(void);

    extern int vga_getxdim(void);
    extern int vga_getydim(void);
    extern int vga_getcolors(void);

    extern int vga_setpalette(int index, int red, int green, int blue);
    extern int vga_getpalette(int index, int *red, int *green, int *blue);
    extern int vga_setpalvec(int start, int num, int *pal);
    extern int vga_getpalvec(int start, int num, int *pal);

    extern int vga_screenoff(void);
    extern int vga_screenon(void);

    extern int vga_setcolor(int color);
    extern int vga_drawpixel(int x, int y);
    extern int vga_drawline(int x1, int y1, int x2, int y2);
    extern int vga_drawscanline(int line, unsigned char *colors);
    extern int vga_drawscansegment(unsigned char *colors, int x, int y, int length);
    extern int vga_getpixel(int x, int y);	/* Added. */
    extern int vga_getscansegment(unsigned char *colors, int x, int y, int length);

    extern int vga_getch(void);

    extern int vga_dumpregs(void);


/* Extensions to VGAlib v1.2: */

/* blit flags */
#define HAVE_BITBLIT 1
#define HAVE_FILLBLIT 2
#define HAVE_IMAGEBLIT 4
#define HAVE_HLINELISTBLIT 8
#define HAVE_BLITWAIT 16

/* other flags */
#define HAVE_RWPAGE 		   1	/* vga_setreadpage() / vga_setwritepage() available */
#define IS_INTERLACED		   2	/* mode is interlaced */
#define IS_MODEX			   4	/* ModeX style 256 colors */
#define IS_DYNAMICMODE		   8	/* Dynamic defined mode */
#define CAPABLE_LINEAR		  16	/* Can go to linear addressing mode. */
#define IS_LINEAR			  32	/* Linear addressing can be used. */
#define EXT_INFO_AVAILABLE	  64	/* Returned modeinfo contains valid extended fields */
#define RGB_MISORDERED		 128	/* Mach32 32bpp uses 0BGR instead of BGR0. */
#define HAVE_EXT_SET		 256	/* vga_ext_set() available */
#define LINEAR_MODE			 512	/* Linear mode is enabled */
#define IOCTL_SETDISPLAY	1024	/* The card supports ioctl method for setting display
										start at next vertical blank time */

    typedef struct {
	int width;
	int height;
	int bytesperpixel;
	int colors;
	int linewidth;		/* scanline width in bytes */
	int maxlogicalwidth;	/* maximum logical scanline width */
	int startaddressrange;	/* changeable bits set */
	int maxpixels;		/* video memory / bytesperpixel */
	int haveblit;		/* mask of blit functions available */
	int flags;		/* other flags */

	/* Extended fields: */

	int chiptype;		/* Chiptype detected */
	int memory;		/* videomemory in KB */
	int linewidth_unit;	/* Use only a multiple of this as parameter for set_logicalwidth and
				   set_displaystart */
	char *linear_aperture;	/* points to mmap secondary mem aperture of card (NULL if unavailable) */
	int aperture_size;	/* size of aperture in KB if size>=videomemory. 0 if unavail */
	void (*set_aperture_page) (int page);
	/* if aperture_size<videomemory select a memory page */
	void *extensions;	/* points to copy of eeprom for mach32 */
	/* depends from actual driver/chiptype.. etc. */
    } vga_modeinfo;

    typedef struct {
        int version;
        int size;
        int chipset;
        long physmem;
		long physmemsize; /* Will there be a card with more than 4GB? */
		void * linearmem;
    } vga_cardinfo;

    extern vga_cardinfo *vga_getcardinfo(void);
    extern vga_modeinfo *vga_getmodeinfo(int mode);
    extern int vga_getdefaultmode(void);
    extern int vga_getcurrentmode(void);
    extern int vga_getcurrentchipset(void);
    extern char *vga_getmodename(int mode);
    extern int vga_getmodenumber(char *name);
    extern int vga_lastmodenumber(void);
    extern int vga_getoptmode(int x, int y, int colors, int bytesperpixel, int c);

    extern unsigned char *graph_mem;
    extern unsigned char *vga_getgraphmem(void);

    extern void vga_setpage(int p);
    extern void vga_setreadpage(int p);
    extern void vga_setwritepage(int p);
    extern void vga_setlogicalwidth(int w);
    extern void vga_setdisplaystart(int a);
    extern void vga_waitretrace(void);
    extern int vga_claimvideomemory(int n);
    extern void vga_disabledriverreport(void);
    extern int vga_setmodeX(void);
    extern int vga_init(void);	/* Used to return void in svgalib <= 1.12. */
    extern int vga_initf(int);
	extern void vga_norevokeprivs(void);
    extern int vga_getmousetype(void);
    extern int vga_getmonitortype(void);
    extern void vga_setmousesupport(int s);
    extern void vga_lockvc(void);
    extern void vga_unlockvc(void);
    extern int vga_getkey(void);
    extern int vga_oktowrite(void);
    extern void vga_copytoplanar256(unsigned char *virtualp, int pitch,
				  int voffset, int vpitch, int w, int h);
    extern void vga_copytoplanar16(unsigned char *virtualp, int pitch,
				   int voffset, int vpitch, int w, int h);
    extern void vga_copytoplane(unsigned char *virtualp, int pitch,
		       int voffset, int vpitch, int w, int h, int plane);
    extern int vga_setlinearaddressing(void);
    extern void vga_safety_fork(void (*shutdown_routine) (void));

    extern int vga_simple_init(void);
    extern void vga_chipset_saveregs(unsigned char *);
    extern void vga_chipset_setregs(unsigned char *);

#ifdef EGA			/* Kernel headers may define this. */
#undef EGA
#endif

#define UNDEFINED	0
#define VGA		1
#define ET4000		2
#define CIRRUS		3
#define TVGA8900	4
#define OAK		5
#define EGA		6
#define S3		7
#define ET3000		8
#define MACH32		9
#define GVGA6400	10
#define ARK		11
#define ATI		12
#define ALI		13
#define MACH64		14
#define CHIPS		15
#define APM             16
#define NV3		17
#define ET6000		18
#define VESA		19
#define MX              20
#define PARADISE	21
#define RAGE		22
#define BANSHEE		23
#define SIS		24
#define I740		25
#define NEOMAGIC	26
#define LAGUNA		27
#define FBDEV		28
#define G400		29
#define R128		30
#define SAVAGE		31
#define MILLENNIUM	32
#define I810		33
#define TRIDENT		34
#define RENDITION 	35
#define G450C2		36
#define PM2			37
#define UNICHROME	38
    
    /* Hor. sync: */
#define MON640_60	0	/* 31.5 KHz (standard VGA) */
#define MON800_56	1	/* 35.1 KHz (old SVGA) */
#define MON1024_43I	2	/* 35.5 KHz (low-end SVGA, 8514) */
#define MON800_60	3	/* 37.9 KHz (SVGA) */
#define MON1024_60	4	/* 48.3 KHz (SVGA non-interlaced) */
#define MON1024_70	5	/* 56.0 KHz (SVGA high frequency) */
#define MON1024_72	6

    extern void vga_setchipset(int c);
    extern void vga_setchipsetandfeatures(int c, int par1, int par2);
    extern void vga_disablechipset(int c);
    extern void vga_gettextfont(void *font);
    extern void vga_puttextfont(void *font);
    extern void vga_settextmoderegs(void *regs);
    extern void vga_gettextmoderegs(void *regs);

    extern int vga_white(void);
    extern int vga_setegacolor(int c);
    extern int vga_setrgbcolor(int r, int g, int b);

    extern void vga_bitblt(int srcaddr, int destaddr, int w, int h, int pitch);
    extern void vga_imageblt(void *srcaddr, int destaddr, int w, int h, int pitch);
    extern void vga_fillblt(int destaddr, int w, int h, int pitch, int c);
    extern void vga_hlinelistblt(int ymin, int n, int *xmin, int *xmax, int pitch, int c);
    extern void vga_blitwait(void);
    extern int vga_ext_set(unsigned what,...);
    extern int vga_accel(unsigned operation,...);

    extern int vga_initcursor(int);
    extern void vga_showcursor(int);
    extern void vga_setcursorposition(int, int);
    extern void vga_selectcursor(int);
    extern void vga_setcursorimage(int, int, int, int, unsigned char *);

    extern int vga_setcrtcregs(unsigned char *);
    extern int vga_getcrtcregs(unsigned char *);

    extern int vga_addtiming(int pixelClock,
       			      int HDisplay,		
                              int HSyncStart,
                              int HSyncEnd,
                              int HTotal,
                              int VDisplay,
                              int VSyncStart,
                              int VSyncEnd,
                              int VTotal,
                              int flags);

    extern int vga_changetiming(int pixelClock,
       			      int HDisplay,		
                              int HSyncStart,
                              int HSyncEnd,
                              int HTotal,
                              int VDisplay,
                              int VSyncStart,
                              int VSyncEnd,
                              int VTotal,
                              int flags);

   extern int vga_getcurrenttiming(int *pixelClock,
       			      int *HDisplay,		
                              int *HSyncStart,
                              int *HSyncEnd,
                              int *HTotal,
                              int *VDisplay,
                              int *VSyncStart,
                              int *VSyncEnd,
                              int *VTotal,
                              int *flags);

   extern int vga_addmode(int xdim, int ydim, int cols, 
                          int xbytes, int bytespp);

   extern int vga_guesstiming(int x, int y, int clue, int arg);

   extern void vga_dpms(int mode);

/* Valid values for what in vga_ext_set: */
#define VGA_EXT_AVAILABLE	0	/* supported flags */
#define VGA_EXT_SET		1	/* set flag(s) */
#define VGA_EXT_CLEAR		2	/* clear flag(s) */
#define VGA_EXT_RESET		3	/* set/clear flag(s) */
#define VGA_EXT_PAGE_OFFSET	4	/* set an offset for all subsequent vga_set*page() calls */
    /* Like: vga_ext_set(VGA_EXT_PAGE_OFFSET, 42);           */
    /* returns the previous offset value.                    */
#define VGA_EXT_FONT_SIZE	5	/* the (maximal) size of the font buffer */

/* Valid params for VGA_EXT_AVAILABLE: */
#define VGA_AVAIL_SET		0	/* vga_ext_set sub funcs */
#define VGA_AVAIL_ACCEL		1	/* vga_accel sub funcs */
#define VGA_AVAIL_FLAGS		2	/* known flags for VGA_EXT_SET */
#define VGA_AVAIL_ROP		3	/* vga_accel ROP sub funcs */
#define VGA_AVAIL_TRANSPARENCY	4	/* vga_accel TRANSPARENCY sub funcs */
#define VGA_AVAIL_ROPMODES	5	/* vga_accel ROP modes supported funcs */
#define VGA_AVAIL_TRANSMODES	6	/* vga_accel TRANSPARENCY modes supported */

/* Known flags to vga_ext_set() */
#define VGA_CLUT8		1	/* 8 bit DAC entries */

/* Acceleration interface. */

/* Accel operations. */
#define ACCEL_FILLBOX			1	/* Simple solid fill. */
#define ACCEL_SCREENCOPY		2	/* Simple screen-to-screen BLT. */
#define ACCEL_PUTIMAGE			3	/* Straight image transfer. */
#define ACCEL_DRAWLINE			4	/* General line draw. */
#define ACCEL_SETFGCOLOR		5	/* Set foreground color. */
#define ACCEL_SETBGCOLOR		6	/* Set background color. */
#define ACCEL_SETTRANSPARENCY		7	/* Set transparency mode. */
#define ACCEL_SETRASTEROP		8	/* Set raster-operation. */
#define ACCEL_PUTBITMAP			9	/* Color-expand bitmap. */
#define ACCEL_SCREENCOPYBITMAP		10	/* Color-expand from screen. */
#define ACCEL_DRAWHLINELIST		11	/* Draw horizontal spans. */
#define ACCEL_SETMODE			12	/* Set blit strategy. */
#define ACCEL_SYNC			13	/* Wait for blits to finish. */
#define ACCEL_SETOFFSET			14	/* Set screen offset */
#define ACCEL_SCREENCOPYMONO		15	/* Monochrome screen-to-screen BLT. */
#define ACCEL_POLYLINE			16	/* Draw multiple lines. */
#define ACCEL_POLYHLINE			17	/* Draw multiple horizontal spans. */
#define ACCEL_POLYFILLMODE		18	/* Set polygon mode. */

/* Corresponding bitmask. */
#define ACCELFLAG_FILLBOX		0x1	/* Simple solid fill. */
#define ACCELFLAG_SCREENCOPY		0x2	/* Simple screen-to-screen BLT. */
#define ACCELFLAG_PUTIMAGE		0x4	/* Straight image transfer. */
#define ACCELFLAG_DRAWLINE		0x8	/* General line draw. */
#define ACCELFLAG_SETFGCOLOR		0x10	/* Set foreground color. */
#define ACCELFLAG_SETBGCOLOR		0x20	/* Set background color. */
#define ACCELFLAG_SETTRANSPARENCY	0x40	/* Set transparency mode. */
#define ACCELFLAG_SETRASTEROP		0x80	/* Set raster-operation. */
#define ACCELFLAG_PUTBITMAP		0x100	/* Color-expand bitmap. */
#define ACCELFLAG_SCREENCOPYBITMAP	0x200	/* Color-exand from screen. */
#define ACCELFLAG_DRAWHLINELIST		0x400	/* Draw horizontal spans. */
#define ACCELFLAG_SETMODE		0x800	/* Set blit strategy. */
#define ACCELFLAG_SYNC			0x1000	/* Wait for blits to finish. */
#define ACCELFLAG_SETOFFSET		0x2000	/* Set screen offset */
#define ACCELFLAG_SCREENCOPYMONO	0x4000	/* Monochrome screen-to-screen BLT. */
#define ACCELFLAG_POLYLINE		0x8000	/* Draw multiple lines. */
#define ACCELFLAG_POLYHLINE		0x10000	/* Draw multiple horizontal spans. */
#define ACCELFLAG_POLYFILLMODE		0x20000	/* Set polygon mode. */

/* Mode for SetTransparency. */
#define DISABLE_TRANSPARENCY_COLOR	0
#define ENABLE_TRANSPARENCY_COLOR	1
#define DISABLE_BITMAP_TRANSPARENCY	2
#define ENABLE_BITMAP_TRANSPARENCY	3

/* Flags for SetMode (accelerator interface). */
#define BLITS_SYNC			0
#define BLITS_IN_BACKGROUND		0x1

#ifdef ROP_XOR
#undef ROP_XOR
#endif

/* Raster ops. */
#define ROP_COPY			0	/* Straight copy. */
#define ROP_OR				1	/* Source OR destination. */
#define ROP_AND				2	/* Source AND destination. */
#define ROP_XOR				3	/* Source XOR destination. */
#define ROP_INVERT			4	/* Invert destination. */

/* For the poly funcs */
#define ACCEL_START			1
#define ACCEL_END			2

/*
 * wait for keypress, mousemove, I/O, timeout. cf. select (3) for details on
 * all parameters execept which.
 * NULL is a valid argument for any of the ptrs.
 */

/*
    extern int vga_waitevent(int which, fd_set * in, fd_set * out, fd_set * except,
			     struct timeval *timeout);
*/

/*
 * valid values for what ( | is valid to combine them )
 */
#define VGA_MOUSEEVENT	1
#define VGA_KEYEVENT	2

/*
 * return value >= has bits set for mouse/keyboard events detected.
 * mouse and raw keyboard events are already handled and their bits removed
 * from *in when vga_waitevent returns.
 * VGA_KEYEVENT relates to vga_getch NOT vga_getkey.
 * return values < 0 signal errors. In this case check errno.
 */

/* Background running */
extern void vga_runinbackground(int stat, ...);
#define VGA_GOTOBACK -1
#define VGA_COMEFROMBACK -2
extern int vga_runinbackground_version(void);
extern void vga_waitvtactive(void);

#ifdef _SVGALIB_LRMI
typedef struct {
    int (*rm_init)(void);
    int (*rm_call)(struct vm86_regs *r);
    int (*rm_int)(int interrupt, struct vm86_regs *r);
    void * (*rm_alloc_real)(int size);
    void (*rm_free_real)(void *m);
} LRMI_callbacks;
extern int vga_set_LRMI_callbacks(LRMI_callbacks * LRMI);
#endif

#ifdef __cplusplus
}

#endif
#endif				/* VGA_H */
