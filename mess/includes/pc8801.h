// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

	includes/pc8801.h

	NEC PC-8801 series driver

***************************************************************************/

#ifndef _PC8801_H_
#define _PC8801_H_

#include "driver.h"
#include "machine/8255ppi.h"

/* CRTC structure */
struct crtc_t
{
	UINT8 cmd;
	UINT8 param_count;
	UINT8 cursor_on;
	UINT8 status;
	UINT8 irq_mask;
	UINT8 param[8][5];
	UINT8 inverse;
};

/* Mouse structure */
struct mouse_t
{
	UINT8 phase;
	UINT8 x, y;
	double time;
};

/* Palette RAM */
struct palram_t
{
	UINT8 r, g, b;
};

/* Global state declarations */
extern UINT8 *pc8801_wram;
extern UINT8 *pc8801_hiwram;
extern UINT8 *pc8801_gvram;
extern UINT8 *pc8801_ewram;
extern UINT8 *pc8801_gfx_rom;
extern UINT8 *pc8801_n80_rom;
extern UINT8 *pc8801_n88_rom;
extern UINT8 *pc8801_n88_sub_rom;
extern UINT8 *pc8801_kanji_rom;

extern int pc8801_is_24KHz;
extern UINT8 pc8801_ext_rom_bank;
extern UINT8 pc8801_gfx_ctrl;
extern UINT8 pc8801_vram_sel;
extern UINT8 pc8801_misc_ctrl;
extern UINT8 pc8801_device_ctrl_data;
extern UINT8 pc8801_window_offset_bank;
extern UINT8 pc8801_layer_mask;
extern UINT16 pc8801_dma_counter[4];
extern UINT16 pc8801_dma_address[4];
extern UINT8 pc8801_alu_reg[3];
extern UINT8 pc8801_dmac_mode;
extern UINT8 pc8801_dmac_ff;
extern UINT8 pc8801_alu_ctrl1;
extern UINT8 pc8801_alu_ctrl2;
extern UINT8 pc8801_extram_mode;
extern UINT8 pc8801_extram_bank;
extern UINT32 pc8801_extram_size;
extern UINT8 pc8801_txt_width;
extern UINT8 pc8801_txt_color;
extern UINT32 pc8801_knj_addr[2];
extern UINT8 pc8801_fdc_irq_opcode;

extern struct crtc_t pc8801_crtc;
extern struct mouse_t pc8801_mouse;
extern struct palram_t pc8801_palram[8];

/* Machine lifecycle */
DRIVER_INIT( pc8801 );
MACHINE_RESET( pc88srl );
MACHINE_RESET( pc88srh );
void pc8801_init_5fd(void);
INTERRUPT_GEN( pc8801_interrupt );
void pc88sr_sound_interrupt(int irq);

/* Memory and I/O handlers */
READ8_HANDLER( pc8801_mem_r );
WRITE8_HANDLER( pc8801_mem_w );

READ8_HANDLER( pc8801_ctrl_r );
WRITE8_HANDLER( pc8801_ctrl_w );
READ8_HANDLER( pc8801_ext_rom_bank_r );
WRITE8_HANDLER( pc8801_ext_rom_bank_w );
WRITE8_HANDLER( pc8801_txt_cmt_ctrl_w );
WRITE8_HANDLER( pc8801_gfx_ctrl_w );
READ8_HANDLER( pc8801_misc_ctrl_r );
WRITE8_HANDLER( pc8801_misc_ctrl_w );
READ8_HANDLER( pc8801_window_bank_r );
WRITE8_HANDLER( pc8801_window_bank_w );
WRITE8_HANDLER( pc8801_window_bank_inc_w );
READ8_HANDLER( pc8801_extram_mode_r );
WRITE8_HANDLER( pc8801_extram_mode_w );
READ8_HANDLER( pc8801_extram_bank_r );
WRITE8_HANDLER( pc8801_extram_bank_w );
WRITE8_HANDLER( pc8801_irq_level_w );
WRITE8_HANDLER( pc8801_irq_mask_w );
READ8_HANDLER( pc8801_kanji_r );
WRITE8_HANDLER( pc8801_kanji_w );
READ8_HANDLER( pc8801_kanji_lv2_r );
WRITE8_HANDLER( pc8801_kanji_lv2_w );
WRITE8_HANDLER( pc8801_rtc_w );
READ8_HANDLER( pc8801_opn_porta_r );
READ8_HANDLER( pc8801_opn_portb_r );

/* Sub CPU (FDC) handlers */
READ8_HANDLER( pc8801fd_nec765_tc );
WRITE8_HANDLER( pc8801_upd765_mc_w );
WRITE8_HANDLER( pc8801_fdc_irq_vector_w );
WRITE8_HANDLER( pc8801_fdc_drive_mode_w );

/* 8255 PPI configuration */
extern const ppi8255_interface pc8801_8255_config;

/* Video functions */
PALETTE_INIT( pc8801 );
VIDEO_START( pc8801 );
VIDEO_UPDATE( pc8801 );

int pc8801_pixel_clock(void);
READ8_HANDLER( pc8801_alu_r );
WRITE8_HANDLER( pc8801_alu_w );
WRITE8_HANDLER( pc8801_alu_ctrl1_w );
WRITE8_HANDLER( pc8801_alu_ctrl2_w );
READ8_HANDLER( pc8801_vram_select_r );
WRITE8_HANDLER( pc8801_vram_select_w );
WRITE8_HANDLER( pc8801_bgpal_w );
WRITE8_HANDLER( pc8801_palram_w );
WRITE8_HANDLER( pc8801_layer_masking_w );
READ8_HANDLER( pc8801_crtc_param_r );
WRITE8_HANDLER( pc88_crtc_param_w );
READ8_HANDLER( pc8801_crtc_status_r );
WRITE8_HANDLER( pc88_crtc_cmd_w );
READ8_HANDLER( pc8801_dmac_r );
WRITE8_HANDLER( pc8801_dmac_w );
READ8_HANDLER( pc8801_dmac_status_r );
WRITE8_HANDLER( pc8801_dmac_mode_w );

#endif /* _PC8801_H_ */
