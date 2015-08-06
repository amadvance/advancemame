#ifndef _KONPPC_H
#define _KONPPC_H

#define CGBOARD_TYPE_ZR107		0
#define CGBOARD_TYPE_GTICLUB	1
#define CGBOARD_TYPE_NWKTR		2
#define CGBOARD_TYPE_HORNET		3

void init_konami_cgboard(int board_id, int type);
void set_cgboard_id(int board_id);
void set_cgboard_texture_bank(int);

READ32_HANDLER( cgboard_dsp_comm_r_ppc );
WRITE32_HANDLER( cgboard_dsp_comm_w_ppc );
READ32_HANDLER( cgboard_dsp_shared_r_ppc );
WRITE32_HANDLER( cgboard_dsp_shared_w_ppc );

READ32_HANDLER( cgboard_dsp_comm_r_sharc );
WRITE32_HANDLER( cgboard_dsp_comm_w_sharc );
READ32_HANDLER( cgboard_dsp_shared_r_sharc );
WRITE32_HANDLER( cgboard_dsp_shared_w_sharc );

void draw_7segment_led(mame_bitmap *bitmap, int x, int y, UINT8 value);

#endif
