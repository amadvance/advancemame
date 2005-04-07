/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (c) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

#ifndef _DRIVER_H
#define _DRIVER_H

#include <stdio.h>
#include <stdarg.h>
#include "vga.h"
#include "libvga.h"
#include "timing.h"
#include "accel.h"
#include "memio.h"

#define MAX_REGS 5000 /* VESA needs a lot of storage space */

extern int inrestore;

typedef struct {
    void (*savepalette)(unsigned char *red, unsigned char *green, unsigned char *blue);
    void (*restorepalette)(const unsigned char *red,
	    const unsigned char *green, const unsigned char *blue);
    int  (*setpalette)(int index, int red, int green, int blue);
    void (*getpalette)(int index, int *red, int *green, int *blue);
    void (*savefont)(void);
    void (*restorefont)(void);
    int (*screenoff)(void);
    int (*screenon)(void);
    void (*waitretrace)(void);
    } Emulation;

typedef struct {
/* Basic functions. */
    int (*saveregs) (unsigned char regs[]);
    void (*setregs) (const unsigned char regs[], int mode);
    void (*unlock) (void);
    void (*lock) (void);
    int (*test) (void);
    int (*init) (int force, int par1, int par2);
    void (*__svgalib_setpage) (int page);
    void (*__svgalib_setrdpage) (int page);
    void (*__svgalib_setwrpage) (int page);
    int (*setmode) (int mode, int prv_mode);
    int (*modeavailable) (int mode);
    void (*setdisplaystart) (int address);
    void (*setlogicalwidth) (int width);
    void (*getmodeinfo) (int mode, vga_modeinfo * modeinfo);
/* Obsolete blit functions. */
    void (*bitblt) (int srcaddr, int destaddr, int w, int h, int pitch);
    void (*imageblt) (void *srcaddr, int destaddr, int w, int h, int pitch);
    void (*fillblt) (int destaddr, int w, int h, int pitch, int c);
    void (*hlinelistblt) (int ymin, int n, int *xmin, int *xmax, int pitch, int c);
    void (*bltwait) (void);
/* Other functions. */
    int (*ext_set) (unsigned what, va_list params);
    int (*accel) (unsigned operation, va_list params);
    int (*linear) (int op, int param);
    AccelSpecs *accelspecs;
    Emulation *emul;
    int (*cursor)(int cmd, int p1, int p2, int p3, int p4, void *p5);
    int disabled;
} DriverSpecs;

extern DriverSpecs __svgalib_vga_driverspecs;
extern DriverSpecs __svgalib_neo_driverspecs;
extern DriverSpecs __svgalib_cirrus_driverspecs;
extern DriverSpecs __svgalib_et4000_driverspecs;
extern DriverSpecs __svgalib_tvga8900_driverspecs;
extern DriverSpecs __svgalib_oak_driverspecs;
extern DriverSpecs __svgalib_ega_driverspecs;
extern DriverSpecs __svgalib_s3_driverspecs;
extern DriverSpecs __svgalib_r128_driverspecs;
extern DriverSpecs __svgalib_mach32_driverspecs;
extern DriverSpecs __svgalib_et3000_driverspecs;
extern DriverSpecs __svgalib_gvga6400_driverspecs;
extern DriverSpecs __svgalib_ark_driverspecs;
extern DriverSpecs __svgalib_ati_driverspecs;
extern DriverSpecs __svgalib_ali_driverspecs;
extern DriverSpecs __svgalib_mach64_driverspecs;
extern DriverSpecs __svgalib_chips_driverspecs;
extern DriverSpecs __svgalib_apm_driverspecs;
extern DriverSpecs __svgalib_nv3_driverspecs;
extern DriverSpecs __svgalib_nv3_19_driverspecs;
extern DriverSpecs __svgalib_et6000_driverspecs;
extern DriverSpecs __svgalib_vesa_driverspecs;
extern DriverSpecs __svgalib_mx_driverspecs;
extern DriverSpecs __svgalib_paradise_driverspecs;
extern DriverSpecs __svgalib_rage_driverspecs;
extern DriverSpecs __svgalib_banshee_driverspecs;
extern DriverSpecs __svgalib_sis_driverspecs;
extern DriverSpecs __svgalib_i740_driverspecs;
extern DriverSpecs __svgalib_i810_driverspecs;
extern DriverSpecs __svgalib_laguna_driverspecs;
extern DriverSpecs __svgalib_fbdev_driverspecs;
extern DriverSpecs __svgalib_r128_driverspecs;
extern DriverSpecs __svgalib_g400_driverspecs;
extern DriverSpecs __svgalib_savage_driverspecs;
extern DriverSpecs __svgalib_savage_18_driverspecs;
extern DriverSpecs __svgalib_mil_driverspecs;
extern DriverSpecs __svgalib_trident_driverspecs;
extern DriverSpecs __svgalib_rendition_driverspecs;
extern DriverSpecs __svgalib_g450c2_driverspecs;
extern DriverSpecs __svgalib_pm2_driverspecs;
extern DriverSpecs __svgalib_unichrome_driverspecs;

extern DriverSpecs *__svgalib_driverspecs;
extern DriverSpecs *__svgalib_driverspecslist[];

enum {
    CHIPSET_SAVEREGS = 0, CHIPSET_SETREGS, CHIPSET_UNLOCK, CHIPSET_LOCK,
    CHIPSET_TEST, CHIPSET_INIT, CHIPSET_SETPAGE, CHIPSET_SETRDPAGE,
    CHIPSET_SETWRPAGE, CHIPSET_SETMODE,
    CHIPSET_MODEAVAILABLE, CHIPSET_SETDISPLAYSTART,
    CHIPSET_SETLOGICALWIDTH, CHIPSET_GETMODEINFO,
    CHIPSET_BITBLT, CHIPSET_IMAGEBLT, CHIPSET_FILLBLT,
    CHIPSET_HLINELISTBLT, CHIPSET_BLTWAIT,
    CHIPSET_EXT_SET, CHIPSET_ACCEL, CHIPSET_LINEAR
};

enum {
    LINEAR_QUERY_BASE, LINEAR_QUERY_GRANULARITY, LINEAR_QUERY_RANGE,
    LINEAR_ENABLE, LINEAR_DISABLE
};

enum { CURSOR_INIT, CURSOR_HIDE, CURSOR_SHOW, CURSOR_POSITION, 
       CURSOR_SELECT, CURSOR_IMAGE, CURSOR_SAVE
};

typedef struct {
/* refresh ranges in Hz */
    unsigned min;
    unsigned max;
} RefreshRange;

extern int __svgalib_CRT_I;
extern int __svgalib_CRT_D;
extern int __svgalib_IS1_R;
extern int __svgalib_driver_report;	/* driverreport */
extern int __svgalib_videomemoryused;	/* videomemoryused */
extern int __svgalib_critical;
extern int __svgalib_chipset;
extern RefreshRange __svgalib_horizsync;
extern RefreshRange __svgalib_vertrefresh;
extern int __svgalib_bandwidth;
extern int __svgalib_grayscale;
extern int __svgalib_modeinfo_linearset;
extern const int __svgalib_max_modes;

void __svgalib_read_options(char **commands, char *(*func) (int ind, int mode, char **nptr));
char *__svgalib_token(char **nptr);
/* ----------------------------------------------------------------------
   ** A modetable holds a pair of values 
   ** for each mode :
   **
   **    <mode number> <pointer to registers>
   **
   ** the last entry is marked by 
   **  
   **    <any number>  <NULL>
*/

typedef struct {
    unsigned short mode_number;
    const unsigned char *regs;
} ModeTable;

#define DISABLE_MODE	  ((unsigned char *)1)
#define OneModeEntry(res) {G##res,g##res##_regs}
#define DisableEntry(res) {G##res,DISABLE_MODE}
#define END_OF_MODE_TABLE { 0, NULL }

extern const unsigned char *__svgalib_mode_in_table(const ModeTable * modes, int mode);
#define LOOKUPMODE __svgalib_mode_in_table

/* ---------------------------------------------------------------------- */

extern int __svgalib_hicolor(int dac_type, int mode);
/* Enters hicolor mode - 0 for no hi, 1 for 15 bit, 2 for 16, 3 for 24 */
/* For any modes it doesn't know about, etc, it attempts to turn hicolor off. */

#define STD_DAC		0
#define HI15_DAC	1
#define HI16_DAC	2
#define TC24_DAC	3

/* ----------------------------------------------------------------------
   ** regextr.h  -  extract graphics modes and register information
   **               from C source file
 */

extern void __svgalib_readmodes(FILE * inp, ModeTable ** modes, int *dac, unsigned *clocks);

#endif
