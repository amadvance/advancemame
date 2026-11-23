// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/*----------- defined in drivers/micro3d.c -----------*/

extern UINT16 *micro3d_sprite_vram;
extern UINT16 dpyadr;
extern int dpyadrscan;

/*----------- defined in vidhrdw/micro3d.c -----------*/

void changecolor_BBBBBRRRRRGGGGGG(pen_t color,int data);
WRITE16_HANDLER( paletteram16_BBBBBRRRRRGGGGGG_word_w );

VIDEO_START(micro3d);
VIDEO_UPDATE(micro3d);

void micro3d_video_reset(void);
void micro3d_vblank_buffers(void);
WRITE16_HANDLER( micro3d_creg_w );
WRITE16_HANDLER( micro3d_xfer3dk_w );
WRITE32_HANDLER( micro3d_fifo_w );
WRITE32_HANDLER( micro3d_alt_fifo_w );
READ32_HANDLER( micro3d_pipe_r );

/*----------- defined in sndhrdw/micro3d.c -----------*/

struct CustomSound_interface;
void *micro3d_noise_start(int clock, const struct CustomSound_interface *config);
void micro3d_noise_reset(void *token);
void micro3d_noise_sh_w(int which, UINT8 data);
void micro3d_noise_dac_w(UINT8 data);
