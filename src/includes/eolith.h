#ifndef EOLITH_H
#define EOLITH_H

WRITE32_HANDLER(eolith_sound_w);
WRITE8_HANDLER(eolith_sound_bank_w);
READ8_HANDLER(eolith_sound_cmd_r);
WRITE8_HANDLER(eolith_qs_wave_w);
WRITE8_HANDLER(eolith_qs_p2_w);
READ8_HANDLER(eolith_qs_port_r);
void eolith_sound_reset(void);
extern struct CustomSound_interface eolith_sound_interface;

READ32_HANDLER(eolith_vram_r);
WRITE32_HANDLER(eolith_vram_w);
VIDEO_START(eolith);
VIDEO_UPDATE(eolith);
VIDEO_UPDATE(hidctch3);
void hidctch3_pen_position(int player, int *x, int *y);

extern int eolith_buffer;

#endif
