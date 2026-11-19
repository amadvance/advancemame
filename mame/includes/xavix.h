// license:BSD-3-Clause
// copyright-holders:David Haywood, Angelo Salese, ramacat
/***************************************************************************

    xavix.h

    XaviX TV games

***************************************************************************/

#ifndef XAVIX_H
#define XAVIX_H

#include "cpu/xavix/xavix.h"

/* NTSC clock for regular XaviX */
#define XAVIX_MAIN_CLOCK 21477272

/* variants of the input and output ports of the games */
#define XAVIX_IO_PLAIN				0
#define XAVIX_IO_I2C				1	/* serial EEPROM on the bits 3 and 4 of the port 1 */
#define XAVIX_IO_I2C_DUELMAST		2	/* serial EEPROM and boot hacks of Duel Masters */

/* variants of the analog ports */
#define XAVIX_ANPORT_NONE			0
#define XAVIX_ANPORT_MOUSE			1	/* the mouse ports are returned by the analog ports */

extern int xavix_io_type;
extern int xavix_i2c_size;
extern int xavix_anport_type;

/* state shared with the video and the sound */
extern UINT8 *xavix_mainram;
extern UINT8 *xavix_fragment_sprite;
extern UINT8 *xavix_palram_sh;
extern UINT8 *xavix_palram_l;
extern UINT8 *xavix_segment_regs;
extern UINT8 *xavix_posirq_x;
extern UINT8 *xavix_posirq_y;
extern UINT8 *xavix_rom_dma_src;
extern UINT8 *xavix_rom_dma_dst;
extern UINT8 *xavix_rom_dma_len;
extern UINT8 xavix_video_ctrl;
extern UINT8 xavix_arena_start;
extern UINT8 xavix_arena_end;
extern UINT8 xavix_arena_control;
extern UINT8 xavix_spritereg;
extern UINT8 xavix_tmap1_regs[8];
extern UINT8 xavix_tmap2_regs[8];
extern int xavix_sprite_xhigh_ignore_hack;

/* machine */
extern struct xavix_config xavix_cpu_config;

MACHINE_START( xavix );
MACHINE_RESET( xavix );
INTERRUPT_GEN( xavix_interrupt );
NVRAM_HANDLER( xavix_nv );
NVRAM_HANDLER( xavix_i2c );
DRIVER_INIT( xavix );
DRIVER_INIT( xavix_sdb );
DRIVER_INIT( xavix_i2c_24c02 );
DRIVER_INIT( xavix_i2c_24c04 );
DRIVER_INIT( xavix_i2c_24c08 );
DRIVER_INIT( xavix_duelmast );

UINT8 xavix_read_full_data_sp_bypass(UINT32 adr);
UINT8 xavix_sound_regram_read(offs_t offset);
void xavix_sound_regram_write(offs_t offset, UINT8 data);
UINT8 xavix_sound_sample_read(UINT32 offset);

WRITE8_HANDLER( xavix_rom_dmatrg_w );
WRITE8_HANDLER( xavix_rom_dmasrc_w );
WRITE8_HANDLER( xavix_rom_dmadst_w );
WRITE8_HANDLER( xavix_rom_dmalen_w );
READ8_HANDLER( xavix_rom_dmastat_r );
WRITE8_HANDLER( xavix_vector_enable_w );
READ8_HANDLER( xavix_nmi_vector_lo_r );
WRITE8_HANDLER( xavix_nmi_vector_lo_w );
READ8_HANDLER( xavix_nmi_vector_hi_r );
WRITE8_HANDLER( xavix_nmi_vector_hi_w );
READ8_HANDLER( xavix_irq_vector_lo_r );
WRITE8_HANDLER( xavix_irq_vector_lo_w );
READ8_HANDLER( xavix_irq_vector_hi_r );
WRITE8_HANDLER( xavix_irq_vector_hi_w );
READ8_HANDLER( xavix_irq_source_r );
WRITE8_HANDLER( xavix_irq_source_w );
READ8_HANDLER( xavix_extintrf_790x_r );
WRITE8_HANDLER( xavix_extintrf_790x_w );
READ8_HANDLER( xavix_ioevent_enable_r );
WRITE8_HANDLER( xavix_ioevent_enable_w );
READ8_HANDLER( xavix_ioevent_irqstate_r );
WRITE8_HANDLER( xavix_ioevent_irqack_w );
WRITE8_HANDLER( xavix_slotreg_7810_w );
READ8_HANDLER( xavix_dispctrl_6ff8_r );
WRITE8_HANDLER( xavix_dispctrl_6ff8_w );
WRITE8_HANDLER( xavix_dispctrl_posirq_x_w );
WRITE8_HANDLER( xavix_dispctrl_posirq_y_w );
READ8_HANDLER( xavix_arena_start_r );
WRITE8_HANDLER( xavix_arena_start_w );
READ8_HANDLER( xavix_arena_end_r );
WRITE8_HANDLER( xavix_arena_end_w );
READ8_HANDLER( xavix_arena_control_r );
WRITE8_HANDLER( xavix_arena_control_w );
READ8_HANDLER( xavix_io0_data_r );
READ8_HANDLER( xavix_io1_data_r );
WRITE8_HANDLER( xavix_io0_data_w );
WRITE8_HANDLER( xavix_io1_data_w );
READ8_HANDLER( xavix_io0_direction_r );
READ8_HANDLER( xavix_io1_direction_r );
WRITE8_HANDLER( xavix_io0_direction_w );
WRITE8_HANDLER( xavix_io1_direction_w );
READ8_HANDLER( xavix_timer_status_r );
WRITE8_HANDLER( xavix_timer_control_w );
READ8_HANDLER( xavix_timer_baseval_r );
WRITE8_HANDLER( xavix_timer_baseval_w );
READ8_HANDLER( xavix_timer_freq_r );
WRITE8_HANDLER( xavix_timer_freq_w );
READ8_HANDLER( xavix_timer_curval_r );
READ8_HANDLER( xavix_anport_r );
READ8_HANDLER( xavix_anport2_r );
WRITE8_HANDLER( xavix_anport_w );
READ8_HANDLER( xavix_adc_7b80_r );
WRITE8_HANDLER( xavix_adc_7b80_w );
READ8_HANDLER( xavix_adc_7b81_r );
WRITE8_HANDLER( xavix_adc_7b81_w );
READ8_HANDLER( xavix_barrel_r );
WRITE8_HANDLER( xavix_barrel_w );
READ8_HANDLER( xavix_mult_param_r );
WRITE8_HANDLER( xavix_mult_param_w );
READ8_HANDLER( xavix_mult_r );
WRITE8_HANDLER( xavix_mult_w );

/* sound registers */
READ8_HANDLER( xavix_sound_voice_startstop_r );
WRITE8_HANDLER( xavix_sound_voice_startstop_w );
READ8_HANDLER( xavix_sound_voice_updateenv_r );
WRITE8_HANDLER( xavix_sound_voice_updateenv_w );
READ8_HANDLER( xavix_sound_voice_status_r );
READ8_HANDLER( xavix_sound_volume_r );
WRITE8_HANDLER( xavix_sound_volume_w );
READ8_HANDLER( xavix_sound_regbase_r );
WRITE8_HANDLER( xavix_sound_regbase_w );
READ8_HANDLER( xavix_sound_cyclerate_r );
WRITE8_HANDLER( xavix_sound_cyclerate_w );
READ8_HANDLER( xavix_sound_mixer_r );
WRITE8_HANDLER( xavix_sound_mixer_w );
READ8_HANDLER( xavix_sound_tp_r );
WRITE8_HANDLER( xavix_sound_tp_w );
READ8_HANDLER( xavix_sound_irq_status_r );
WRITE8_HANDLER( xavix_sound_irq_status_w );
READ8_HANDLER( xavix_sound_dac_control_r );
WRITE8_HANDLER( xavix_sound_dac_control_w );

/* video */
VIDEO_START( xavix );
VIDEO_UPDATE( xavix );
WRITE8_HANDLER( xavix_palram_sh_w );
WRITE8_HANDLER( xavix_palram_l_w );
WRITE8_HANDLER( xavix_spriteram_w );
READ8_HANDLER( xavix_tmap1_regs_r );
WRITE8_HANDLER( xavix_tmap1_regs_w );
READ8_HANDLER( xavix_tmap2_regs_r );
WRITE8_HANDLER( xavix_tmap2_regs_w );
READ8_HANDLER( xavix_spriteregs_r );
WRITE8_HANDLER( xavix_spriteregs_w );
READ8_HANDLER( xavix_pal_ntsc_r );
WRITE8_HANDLER( xavix_spritefragment_dma_params_1_w );
WRITE8_HANDLER( xavix_spritefragment_dma_params_2_w );
WRITE8_HANDLER( xavix_spritefragment_dma_trg_w );
READ8_HANDLER( xavix_spritefragment_dma_status_r );
READ8_HANDLER( xavix_memoryemu_txarray_r );
WRITE8_HANDLER( xavix_memoryemu_txarray_w );
UINT8 xavix_txarray_r(UINT16 offset);
void xavix_video_reset(void);

/* sound */
extern struct CustomSound_interface xavix_sound_interface;
void xavix_sound_enable_voice(int voice, int update_only);
void xavix_sound_disable_voice(int voice);
int xavix_sound_is_voice_enabled(int voice);
void xavix_sound_set_tempo(int index, UINT8 value);
void xavix_sound_set_cyclerate(UINT8 value);
double xavix_sound_tempo_tick_hz(UINT8 tempo);
UINT8 xavix_sound_volume_get(void);
void xavix_sound_volume_set(UINT8 data);
UINT8 xavix_sound_mixer_get(void);
void xavix_sound_mixer_set(UINT8 data);
UINT8 xavix_sound_dac_control_get(void);
void xavix_sound_dac_control_set(UINT8 data);
void xavix_sound_set_default_tempo(UINT8 tempo);

#endif
