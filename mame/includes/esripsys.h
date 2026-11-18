// license:BSD-3-Clause
// copyright-holders:Philip Bennett

/* Entertainment Sciences RIP System hardware */

#ifndef ESRIPSYS_H
#define ESRIPSYS_H

#define ESRIPSYS_PIXEL_CLOCK	(25000000 / 2)
#define ESRIPSYS_HTOTAL			(512 + 141 + 2)
#define ESRIPSYS_HBLANK_START	(512)
#define ESRIPSYS_VTOTAL			(384 + 20)
#define ESRIPSYS_VBLANK_START	(384)

#define CMOS_RAM_SIZE			(2048)
#define FDT_RAM_SIZE			(2048 * sizeof(UINT16))

enum
{
	ESRIPSYS_GAME_CPU  = 0,
	ESRIPSYS_FRAME_CPU = 1,
	ESRIPSYS_VIDEO_CPU = 2,
	ESRIPSYS_SOUND_CPU = 3,
};

extern int esripsys_hblank;
extern UINT8 *esripsys_pal_ram;
extern int esripsys_frame_vbl;
extern int esripsys__12sel;
extern int esripsys_video_firq_en;
extern int esripsys_scanline;

VIDEO_START( esripsys );
VIDEO_UPDATE( esripsys );

WRITE8_HANDLER( esripsys_bg_intensity_w );

void esripsys_video_reset(void);

int esripsys_draw(int l, int r, int fig, int attr, int addr, int col, int x_scale, int line_latch);

#endif
