// license:BSD-3-Clause
// copyright-holders:David Haywood, ???
/* PGM2 hardware, converted from MAME 0.287 to AdvanceMAME C. */
#ifndef PGM2_H
#define PGM2_H
#include "driver.h"
extern UINT32 *pgm2_sp_videoram, *pgm2_fg_videoram, *pgm2_bg_videoram;
extern UINT32 *pgm2_sp_zoom, *pgm2_lineram, *pgm2_bgscroll, *pgm2_fgscroll, *pgm2_vidmode;
extern UINT32 pgm2_spritekey;
VIDEO_START(pgm2);
VIDEO_UPDATE(pgm2);
void pgm2_draw_sprites(void);
WRITE32_HANDLER(pgm2_fg_videoram_w);
WRITE32_HANDLER(pgm2_bg_videoram_w);
#endif
