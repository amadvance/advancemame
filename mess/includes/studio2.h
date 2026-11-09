// license:BSD-3-Clause
// copyright-holders:Curt Coder
#ifndef STUDIO2_H
#define STUDIO2_H

extern int studio2_get_vsync(void);
extern void studio2_video_sc_w(int state);
extern void studio2_video_dma_w(UINT8 data);
extern void studio2_video_reset(void);

extern int studio2_in_n(int n);

extern VIDEO_START( studio2 );
extern VIDEO_UPDATE( studio2 );

#endif
