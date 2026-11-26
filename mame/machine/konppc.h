// license:BSD-3-Clause
// copyright-holders:Ville Linde
#ifndef _KONPPC_H
#define _KONPPC_H

#define CGBOARD_TYPE_ZR107		0
#define CGBOARD_TYPE_GTICLUB	1
#define CGBOARD_TYPE_NWKTR		2
#define CGBOARD_TYPE_HORNET		3
#define CGBOARD_TYPE_HANGPLT	4

void init_konami_cgboard(int board_id, int type);
void set_cgboard_id(int board_id);
int get_cgboard_id(void);
int cgboard_output_3d_enabled(void);
void set_cgboard_texture_bank(int);

READ32_HANDLER( cgboard_dsp_comm_r_ppc );
WRITE32_HANDLER( cgboard_dsp_comm_w_ppc );
READ32_HANDLER( cgboard_dsp_shared_r_ppc );
WRITE32_HANDLER( cgboard_dsp_shared_w_ppc );

READ32_HANDLER( cgboard_dsp_comm_r_sharc );
WRITE32_HANDLER( cgboard_dsp_comm_w_sharc );
READ32_HANDLER( cgboard_dsp_shared_r_sharc );
WRITE32_HANDLER( cgboard_dsp_shared_w_sharc );

/* SHARC side of a specific board, used by Hang Pilot */
READ32_HANDLER( cgboard_0_comm_sharc_r );
WRITE32_HANDLER( cgboard_0_comm_sharc_w );
READ32_HANDLER( cgboard_0_shared_sharc_r );
WRITE32_HANDLER( cgboard_0_shared_sharc_w );
READ32_HANDLER( cgboard_0_k033906_r );
WRITE32_HANDLER( cgboard_0_k033906_w );
READ32_HANDLER( nwk_voodoo_0_r );
WRITE32_HANDLER( nwk_voodoo_fifo_0_w );

READ32_HANDLER( cgboard_1_comm_sharc_r );
WRITE32_HANDLER( cgboard_1_comm_sharc_w );
READ32_HANDLER( cgboard_1_shared_sharc_r );
WRITE32_HANDLER( cgboard_1_shared_sharc_w );
READ32_HANDLER( cgboard_1_k033906_r );
WRITE32_HANDLER( cgboard_1_k033906_w );
READ32_HANDLER( nwk_voodoo_1_r );
WRITE32_HANDLER( nwk_voodoo_fifo_1_w );

void draw_7segment_led(mame_bitmap *bitmap, int x, int y, UINT8 value);

#endif
