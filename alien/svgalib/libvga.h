
/* SVGAlib, Copyright 1993 Harm Hanemaayer */
/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* Internal definitions. */

#ifndef _LIBVGA_H
#define _LIBVGA_H

#include <string.h>
#include <stdlib.h>

#include "svgaint.h"

/* --------------------- Macro definitions shared by library modules */

/* VGA index register ports */
#define CRT_IC  0x3D4		/* CRT Controller Index - color emulation */
#define CRT_IM  0x3B4		/* CRT Controller Index - mono emulation */
#define ATT_IW  0x3C0		/* Attribute Controller Index & Data Write Register */
#define GRA_I   0x3CE		/* Graphics Controller Index */
#define SEQ_I   0x3C4		/* Sequencer Index */
#define PEL_IW  0x3C8		/* PEL Write Index */
#define PEL_IR  0x3C7		/* PEL Read Index */

/* VGA data register ports */
#define CRT_DC  0x3D5		/* CRT Controller Data Register - color emulation */
#define CRT_DM  0x3B5		/* CRT Controller Data Register - mono emulation */
#define ATT_R   0x3C1		/* Attribute Controller Data Read Register */
#define GRA_D   0x3CF		/* Graphics Controller Data Register */
#define SEQ_D   0x3C5		/* Sequencer Data Register */
#define MIS_R   0x3CC		/* Misc Output Read Register */
#define MIS_W   0x3C2		/* Misc Output Write Register */
#define IS1_RC  0x3DA		/* Input Status Register 1 - color emulation */
#define IS1_RM  0x3BA		/* Input Status Register 1 - mono emulation */
#define PEL_D   0x3C9		/* PEL Data Register */
#define PEL_MSK 0x3C6		/* PEL mask register */

/* 8514/MACH regs we need outside of the mach32 driver.. */
#define PEL8514_D	0x2ED
#define PEL8514_IW	0x2EC
#define PEL8514_IR	0x2EB
#define PEL8514_MSK	0x2EA

/* EGA-specific registers */

#define GRA_E0	0x3CC		/* Graphics enable processor 0 */
#define GRA_E1	0x3CA		/* Graphics enable processor 1 */

/* standard VGA indexes max counts */
#define CRT_C   24		/* 24 CRT Controller Registers */
#define ATT_C   21		/* 21 Attribute Controller Registers */
#define GRA_C   9		/* 9  Graphics Controller Registers */
#define SEQ_C   5		/* 5  Sequencer Registers */
#define MIS_C   1		/* 1  Misc Output Register */

/* VGA registers saving indexes */
#define CRT     0		/* CRT Controller Registers start */
#define ATT     (CRT+CRT_C)	/* Attribute Controller Registers start */
#define GRA     (ATT+ATT_C)	/* Graphics Controller Registers start */
#define SEQ     (GRA+GRA_C)	/* Sequencer Registers */
#define MIS     (SEQ+SEQ_C)	/* General Registers */
#define EXT     (MIS+MIS_C)	/* SVGA Extended Registers */

/* Shorthands for chipset (driver) specific calls */
#define chipset_saveregs __svgalib_driverspecs->saveregs
#define chipset_setregs __svgalib_driverspecs->setregs
#define chipset_unlock __svgalib_driverspecs->unlock
#define chipset_test __svgalib_driverspecs->test
#define chipset_setpage __svgalib_driverspecs->__svgalib_setpage
#define chipset_setmode __svgalib_driverspecs->setmode
#define chipset_modeavailable __svgalib_driverspecs->modeavailable
#define chipset_getmodeinfo __svgalib_driverspecs->getmodeinfo
#define chipset_cursor __svgalib_driverspecs->cursor

/* Shorthands for internal variables and functions */
#define CI	__svgalib_cur_info
#define SM	__svgalib_sparse_mem
#define GM	__svgalib_graph_mem
#define CM	__svgalib_cur_mode
#define VMEM	__svgalib_videomemoryused
#define DREP	__svgalib_driver_report
#define CRITICAL __svgalib_critical
#define COL	__svgalib_cur_color
#define CHIPSET __svgalib_chipset
#define SCREENON __svgalib_screenon
#define MODEX 	__svgalib_modeX
#define MODEFLAGS __svgalib_modeflags
#define infotable __svgalib_infotable

#define SVGADRV		 2
#define STDVGADRV	 1
#define STDVGAMODE(mode) (chipset_modeavailable(mode) == STDVGADRV)
#define SVGAMODE(mode)   (chipset_modeavailable(mode) == SVGADRV)

#define GRAPH_BASE 0xA0000
#define FONT_BASE  0xA0000
#define GRAPH_SIZE 0x10000
#define FONT_SIZE  (0x2000 * 4) /* 2.0.x kernel can use 2 512 char. fonts */
#define GPLANE16   G640x350x16

/* graphics mode information */
struct vgainfo {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
};

/* --------------------- Variable definitions shared by library modules */

#define BANKED_POINTER __svgalib_banked_pointer
#define LINEAR_POINTER __svgalib_linear_pointer
#define MMIO_POINTER __svgalib_mmio_pointer

extern int __svgalib_CRT_I;		/* current CRT index register address */
extern int __svgalib_CRT_D;		/* current CRT data register address */
extern int __svgalib_IS1_R;		/* current input status register address */
extern uint8_t * BANKED_POINTER, * LINEAR_POINTER;
extern uint8_t *MMIO_POINTER;
extern uint8_t *SPARSE_MMIO;
extern unsigned long __svgalib_banked_mem_base, __svgalib_banked_mem_size;
extern unsigned long __svgalib_mmio_base, __svgalib_mmio_size;
extern unsigned long __svgalib_linear_mem_base, __svgalib_linear_mem_size;
extern unsigned long __svgalib_mmio_base, __svgalib_mmio_size;
extern struct vgainfo CI;		/* current video parameters */
extern int COL;			/* current color            */
extern int CM;			/* current video mode       */
extern struct vgainfo infotable[];
extern int SCREENON;		/* screen visible if != 0 */
extern unsigned long __svgalib_graph_base;
extern unsigned char *GM;	/* graphics memory frame */
extern int MODEX;		/* TRUE after vga_setmodeX() */
extern int MODEFLAGS;		/* copy of flags of current modeinfo->flags */

extern int __svgalib_mem_fd;
extern int __svgalib_tty_fd;
extern int __svgalib_nosigint;
extern int __svgalib_mouse_fd;
extern int __svgalib_kbd_fd;
extern int __svgalib_runinbackground;
extern int __svgalib_vgacolormode;
extern int biosparams, biosparam[16];

extern unsigned char __svgalib_novga;
extern unsigned char __svgalib_textprog;
extern unsigned char __svgalib_secondary;
extern unsigned char __svgalib_emulatepage;
extern unsigned char __svgalib_novccontrol;
extern unsigned char __svgalib_m_ignore_dx;
extern unsigned char __svgalib_m_ignore_dy;
extern unsigned char __svgalib_m_ignore_dz;

extern char *__joystick_devicenames[4];

extern int __svgalib_cursor_status;

/* --------------------- Function definitions shared by library modules */

extern int (*__svgalib_inmisc)(void);
extern void (*__svgalib_outmisc)(int);
extern int (*__svgalib_incrtc)(int);
extern void (*__svgalib_outcrtc)(int,int);
extern int (*__svgalib_inseq)(int);
extern void (*__svgalib_outseq)(int,int);
extern int  (*__svgalib_ingra)(int);
extern void (*__svgalib_outgra)(int,int);
extern int  (*__svgalib_inatt)(int);
extern void (*__svgalib_outatt)(int,int);
extern void (*__svgalib_attscreen)(int);
extern void (*__svgalib_inpal)(int,int*,int*,int*);
extern void (*__svgalib_outpal)(int,int,int,int);
extern int (*__svgalib_inis1)(void);

extern int __svgalib_setregs(const unsigned char *regs);
extern int __svgalib_saveregs(unsigned char *regs);
extern void __svgalib_dumpregs(const unsigned char regs[], int n);
extern void __svgalib_get_perm(void);
extern int __svgalib_getchipset(void);
extern int __svgalib_name2number(char *modename);
extern void __svgalib_delay(void);
extern int __svgalib_addmode(int xdim, int ydim, int cols, int xbytes, int bytespp);
extern void __svgalib_waitvtactive(void);
extern void __svgalib_open_devconsole(void);
extern void (*__svgalib_mouse_eventhandler) (int, int, int, int, int, int, int);
extern void (*__svgalib_keyboard_eventhandler) (int, int);
extern void __joystick_flip_vc(int acquire);
extern char *__svgalib_TextProg_argv[16]; /* should be enough */
extern char *__svgalib_TextProg;
extern int __svgalib_VESA_savebitmap;
extern int __svgalib_VESA_textmode;
extern unsigned char __svgalib_vesatext;
extern int __svgalib_mapkeyname(const char *keyname);
extern void __svgalib_mouse_update_keymap(void);
extern int __svgalib_vgacolor(void);
extern void __svgalib_cursor_restore(void);
extern void map_mmio(void);
extern void map_mem(void);
extern void map_linear(unsigned long, unsigned long);
extern void unmap_linear(unsigned long);
extern void __svgalib_emul_setpage(int);

#if 0 
/* remove this part ? */
extern void __svgalib_releasevt_signal(int n);
extern void __svgalib_acquirevt_signal(int n);
#endif

#define gr_readb(off)		(((volatile uint8_t *)GM)[(off)])
#define gr_readw(off)		(*(volatile uint16_t*)((GM)+(off)))
#define gr_readl(off)		(*(volatile uint32_t*)((GM)+(off)))
#define gr_writeb(v,off)	(GM[(off)] = (v))
#define gr_writew(v,off)	(*(uint16_t*)((GM)+(off)) = (v))
#define gr_writel(v,off)	(*(uint32_t*)((GM)+(off)) = (v))

extern void port_out(int value, int port);
extern void port_outw(int value, int port);
extern void port_outl(int value, int port);
extern void port_rep_outb(unsigned char* string, int length, int port);

extern int port_in(int port);
extern int port_inw(int port);
extern int port_inl(int port);

/* Note that the arguments of outb/w are reversed compared with the */
/* kernel sources. The XFree86 drivers also use this format. */
#define port_out_r(port, value) port_out(value, port)
#define port_outw_r(port, value) port_outw(value, port)
#define port_outl_r(port, value) port_outl(value, port)

/* Background things */

extern unsigned char *__svgalib_give_graph_red(void);
extern unsigned char *__svgalib_give_graph_green(void);
extern unsigned char *__svgalib_give_graph_blue(void);
 
#define zero_sa_mask(maskptr) memset(maskptr, 0, sizeof(sigset_t))

#if 1

#define SVGALIB_ACQUIRE_SIG SIGUSR2
#define SVGALIB_RELEASE_SIG SIGUSR1

#else

#define SVGALIB_ACQUIRE_SIG SIGUNUSED
#define SVGALIB_RELEASE_SIG SIGPROF

#endif

//#define DEBUG_TRACE

#ifdef DEBUG_TRACE
#define DTP(x) fprintf x
#else
#define DTP(x)
#endif

#ifdef DEBUG
#define DPRINTF(args...) fprintf(stderr, args)
#else
#define DPRINTF(...)
#endif

#endif /* _LIBVGA_H */

