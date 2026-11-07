#include "libvga.h"

extern int __svgalib_vga_inmisc(void);
extern void __svgalib_vga_outmisc(int i);
extern int __svgalib_vga_incrtc(int i);
extern void __svgalib_vga_outcrtc(int i, int d);
extern int __svgalib_vga_inseq(int i);
extern void __svgalib_vga_outseq(int i, int d);
extern int __svgalib_vga_ingra(int i);
extern void __svgalib_vga_outgra(int i, int d);
extern int __svgalib_vga_inis1(void);
extern int __svgalib_vga_inatt(int i);
extern void __svgalib_vga_outatt(int i, int d);
extern void __svgalib_vga_attscreen(int i);
extern void __svgalib_vga_inpal(int i, int *r, int *g, int *b);
extern void __svgalib_vga_outpal(int i, int r, int g, int b);
