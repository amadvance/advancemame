// license:LGPL-2.1+
// copyright-holders:David Haywood, Angelo Salese, ElSemi, Andrew Gardner

#ifndef HNG64_H
#define HNG64_H

extern UINT32 *hng64_spriteram, *hng64_spriteregs, *hng64_videoregs, *hng64_videoram, *hng64_tcram;
#define HNG64_MAX_PACKETS 512
extern UINT32 hng64_packets[HNG64_MAX_PACKETS][8];
extern unsigned hng64_packet_count;

extern UINT8 hng64_texture_wrap[32];
extern UINT32 *hng64_fbregs;
extern int hng64_roadedge_3d, hng64_samurai_3d;
void hng64_video_reset(void);
void hng64_tile_dirty(unsigned offset);

VIDEO_START(hng64);
VIDEO_UPDATE(hng64);

READ8_HANDLER(hng64_dsp_r);
WRITE8_HANDLER(hng64_dsp_w);
UINT16 hng64_dsp_dma_r(void);
void hng64_dsp_dma_w(UINT16 data);
void hng64_sound_drq(int state);
extern const struct CustomSound_interface hng64_dsp;

#endif
