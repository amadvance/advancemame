// license:BSD-3-Clause
// copyright-holders:David Haywood
/***************************************************************************

XaviX 2000 type CPU hardware titles (2nd XaviX generation?)

These use the SSD 2000 NEC 85605-621 type CPU.

This CPU type adds extra opcodes that don't appear to be present in the 97/98 types.
It does not appear to support the bitmap modes or 16-bit ROMs found in the 2002 type.

It is possible some games in the regular XaviX drivers use this type, there
does not appear to be a way to tell unless the extra opcodes are used or the CPU
has been decapped.

Differences from MAME:
- The cartridge slot of Duel Masters is not emulated, the game runs from the internal ROM.
- The Super XaviX bitmap layer and extended registers are not emulated.
- The sound is generated at the sound sample rate instead of the one of the
  sound sequencer.

***************************************************************************/

#include "driver.h"
#include "includes/xavix.h"


/*************************************
 *
 *  Memory maps
 *
 *************************************/

static READ8_HANDLER( lightgun_r )
{
	return 0xff;
}

static ADDRESS_MAP_START( extbus_8mb_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x7fffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_8mb_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x7fffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_4mb_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x3fffff) AM_MIRROR(0x400000) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_4mb_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x3fffff) AM_MIRROR(0x400000) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_2mb_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x1fffff) AM_MIRROR(0x600000) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_2mb_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x1fffff) AM_MIRROR(0x600000) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_duelmast_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x7fffff) AM_READ(MRA8_ROM)
	AM_RANGE(0x408000, 0x40ffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( extbus_duelmast_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x7fffff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x408000, 0x40ffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lowbus_readmem, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x4fff) AM_READ(xavix_memoryemu_txarray_r)
	AM_RANGE(0x6000, 0x67ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6800, 0x68ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6900, 0x69ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6a00, 0x6a1f) AM_READ(MRA8_RAM)
	AM_RANGE(0x6fc8, 0x6fcf) AM_READ(xavix_tmap1_regs_r)
	AM_RANGE(0x6fd0, 0x6fd7) AM_READ(xavix_tmap2_regs_r)
	AM_RANGE(0x6fd8, 0x6fd8) AM_READ(xavix_spriteregs_r)
	AM_RANGE(0x6fe0, 0x6fe0) AM_READ(xavix_spritefragment_dma_status_r)
	AM_RANGE(0x6fe8, 0x6fe8) AM_READ(xavix_arena_start_r)
	AM_RANGE(0x6fe9, 0x6fe9) AM_READ(xavix_arena_end_r)
	AM_RANGE(0x6fea, 0x6fea) AM_READ(xavix_arena_control_r)
	AM_RANGE(0x6ff0, 0x6ff2) AM_READ(MRA8_RAM)
	AM_RANGE(0x6ff8, 0x6ff8) AM_READ(xavix_dispctrl_6ff8_r)
	AM_RANGE(0x6ff9, 0x6ff9) AM_READ(xavix_pal_ntsc_r)
	AM_RANGE(0x6ffa, 0x6ffb) AM_READ(MRA8_RAM)
	AM_RANGE(0x6ffc, 0x6fff) AM_READ(lightgun_r)
	AM_RANGE(0x7400, 0x757f) AM_READ(MRA8_RAM)
	AM_RANGE(0x75f0, 0x75f1) AM_READ(xavix_sound_voice_startstop_r)
	AM_RANGE(0x75f2, 0x75f3) AM_READ(xavix_sound_voice_updateenv_r)
	AM_RANGE(0x75f4, 0x75f5) AM_READ(xavix_sound_voice_status_r)
	AM_RANGE(0x75f6, 0x75f6) AM_READ(xavix_sound_volume_r)
	AM_RANGE(0x75f7, 0x75f7) AM_READ(xavix_sound_regbase_r)
	AM_RANGE(0x75f8, 0x75f8) AM_READ(xavix_sound_cyclerate_r)
	AM_RANGE(0x75f9, 0x75f9) AM_READ(xavix_sound_mixer_r)
	AM_RANGE(0x75fa, 0x75fd) AM_READ(xavix_sound_tp_r)
	AM_RANGE(0x75fe, 0x75fe) AM_READ(xavix_sound_irq_status_r)
	AM_RANGE(0x75ff, 0x75ff) AM_READ(xavix_sound_dac_control_r)
	AM_RANGE(0x7900, 0x7902) AM_READ(xavix_extintrf_790x_r)
	AM_RANGE(0x7980, 0x7980) AM_READ(xavix_rom_dmastat_r)
	AM_RANGE(0x7981, 0x7987) AM_READ(MRA8_RAM)
	AM_RANGE(0x7a00, 0x7a00) AM_READ(xavix_io0_data_r)
	AM_RANGE(0x7a01, 0x7a01) AM_READ(xavix_io1_data_r)
	AM_RANGE(0x7a02, 0x7a02) AM_READ(xavix_io0_direction_r)
	AM_RANGE(0x7a03, 0x7a03) AM_READ(xavix_io1_direction_r)
	AM_RANGE(0x7a80, 0x7a80) AM_READ(xavix_ioevent_enable_r)
	AM_RANGE(0x7a81, 0x7a81) AM_READ(xavix_ioevent_irqstate_r)
	AM_RANGE(0x7b00, 0x7b01) AM_READ(xavix_anport_r)
	AM_RANGE(0x7b10, 0x7b11) AM_READ(xavix_anport2_r)
	AM_RANGE(0x7b80, 0x7b80) AM_READ(xavix_adc_7b80_r)
	AM_RANGE(0x7b81, 0x7b81) AM_READ(xavix_adc_7b81_r)
	AM_RANGE(0x7c00, 0x7c00) AM_READ(xavix_timer_status_r)
	AM_RANGE(0x7c01, 0x7c01) AM_READ(xavix_timer_baseval_r)
	AM_RANGE(0x7c02, 0x7c02) AM_READ(xavix_timer_freq_r)
	AM_RANGE(0x7c03, 0x7c03) AM_READ(xavix_timer_curval_r)
	AM_RANGE(0x7ff0, 0x7ff1) AM_READ(xavix_barrel_r)
	AM_RANGE(0x7ff2, 0x7ff4) AM_READ(xavix_mult_param_r)
	AM_RANGE(0x7ff5, 0x7ff6) AM_READ(xavix_mult_r)
	AM_RANGE(0x7ffa, 0x7ffa) AM_READ(xavix_nmi_vector_lo_r)
	AM_RANGE(0x7ffb, 0x7ffb) AM_READ(xavix_nmi_vector_hi_r)
	AM_RANGE(0x7ffc, 0x7ffc) AM_READ(xavix_irq_source_r)
	AM_RANGE(0x7ffe, 0x7ffe) AM_READ(xavix_irq_vector_lo_r)
	AM_RANGE(0x7fff, 0x7fff) AM_READ(xavix_irq_vector_hi_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lowbus_writemem, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_RAM) AM_BASE(&xavix_mainram)
	AM_RANGE(0x4000, 0x4fff) AM_WRITE(xavix_memoryemu_txarray_w)
	AM_RANGE(0x6000, 0x67ff) AM_WRITE(xavix_spriteram_w) AM_BASE(&xavix_fragment_sprite)
	AM_RANGE(0x6800, 0x68ff) AM_WRITE(xavix_palram_sh_w) AM_BASE(&xavix_palram_sh)
	AM_RANGE(0x6900, 0x69ff) AM_WRITE(xavix_palram_l_w) AM_BASE(&xavix_palram_l)
	AM_RANGE(0x6a00, 0x6a1f) AM_WRITE(MWA8_RAM) AM_BASE(&xavix_segment_regs)
	AM_RANGE(0x6fc8, 0x6fcf) AM_WRITE(xavix_tmap1_regs_w)
	AM_RANGE(0x6fd0, 0x6fd7) AM_WRITE(xavix_tmap2_regs_w)
	AM_RANGE(0x6fd8, 0x6fd8) AM_WRITE(xavix_spriteregs_w)
	AM_RANGE(0x6fe0, 0x6fe0) AM_WRITE(xavix_spritefragment_dma_trg_w)
	AM_RANGE(0x6fe1, 0x6fe2) AM_WRITE(xavix_spritefragment_dma_params_1_w)
	AM_RANGE(0x6fe5, 0x6fe6) AM_WRITE(xavix_spritefragment_dma_params_2_w)
	AM_RANGE(0x6fe8, 0x6fe8) AM_WRITE(xavix_arena_start_w)
	AM_RANGE(0x6fe9, 0x6fe9) AM_WRITE(xavix_arena_end_w)
	AM_RANGE(0x6fea, 0x6fea) AM_WRITE(xavix_arena_control_w)
	AM_RANGE(0x6ff0, 0x6ff2) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x6ff8, 0x6ff8) AM_WRITE(xavix_dispctrl_6ff8_w)
	AM_RANGE(0x6ffa, 0x6ffa) AM_WRITE(xavix_dispctrl_posirq_x_w) AM_BASE(&xavix_posirq_x)
	AM_RANGE(0x6ffb, 0x6ffb) AM_WRITE(xavix_dispctrl_posirq_y_w) AM_BASE(&xavix_posirq_y)
	AM_RANGE(0x7400, 0x757f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x75f0, 0x75f1) AM_WRITE(xavix_sound_voice_startstop_w)
	AM_RANGE(0x75f2, 0x75f3) AM_WRITE(xavix_sound_voice_updateenv_w)
	AM_RANGE(0x75f6, 0x75f6) AM_WRITE(xavix_sound_volume_w)
	AM_RANGE(0x75f7, 0x75f7) AM_WRITE(xavix_sound_regbase_w)
	AM_RANGE(0x75f8, 0x75f8) AM_WRITE(xavix_sound_cyclerate_w)
	AM_RANGE(0x75f9, 0x75f9) AM_WRITE(xavix_sound_mixer_w)
	AM_RANGE(0x75fa, 0x75fd) AM_WRITE(xavix_sound_tp_w)
	AM_RANGE(0x75fe, 0x75fe) AM_WRITE(xavix_sound_irq_status_w)
	AM_RANGE(0x75ff, 0x75ff) AM_WRITE(xavix_sound_dac_control_w)
	AM_RANGE(0x7810, 0x7810) AM_WRITE(xavix_slotreg_7810_w)
	AM_RANGE(0x7900, 0x7902) AM_WRITE(xavix_extintrf_790x_w)
	AM_RANGE(0x7980, 0x7980) AM_WRITE(xavix_rom_dmatrg_w)
	AM_RANGE(0x7981, 0x7983) AM_WRITE(xavix_rom_dmasrc_w) AM_BASE(&xavix_rom_dma_src)
	AM_RANGE(0x7984, 0x7985) AM_WRITE(xavix_rom_dmadst_w) AM_BASE(&xavix_rom_dma_dst)
	AM_RANGE(0x7986, 0x7987) AM_WRITE(xavix_rom_dmalen_w) AM_BASE(&xavix_rom_dma_len)
	AM_RANGE(0x7a00, 0x7a00) AM_WRITE(xavix_io0_data_w)
	AM_RANGE(0x7a01, 0x7a01) AM_WRITE(xavix_io1_data_w)
	AM_RANGE(0x7a02, 0x7a02) AM_WRITE(xavix_io0_direction_w)
	AM_RANGE(0x7a03, 0x7a03) AM_WRITE(xavix_io1_direction_w)
	AM_RANGE(0x7a80, 0x7a80) AM_WRITE(xavix_ioevent_enable_w)
	AM_RANGE(0x7a81, 0x7a81) AM_WRITE(xavix_ioevent_irqack_w)
	AM_RANGE(0x7b00, 0x7b01) AM_WRITE(xavix_anport_w)
	AM_RANGE(0x7b10, 0x7b11) AM_WRITE(xavix_anport_w)
	AM_RANGE(0x7b80, 0x7b80) AM_WRITE(xavix_adc_7b80_w)
	AM_RANGE(0x7b81, 0x7b81) AM_WRITE(xavix_adc_7b81_w)
	AM_RANGE(0x7c00, 0x7c00) AM_WRITE(xavix_timer_control_w)
	AM_RANGE(0x7c01, 0x7c01) AM_WRITE(xavix_timer_baseval_w)
	AM_RANGE(0x7c02, 0x7c02) AM_WRITE(xavix_timer_freq_w)
	AM_RANGE(0x7ff0, 0x7ff1) AM_WRITE(xavix_barrel_w)
	AM_RANGE(0x7ff2, 0x7ff4) AM_WRITE(xavix_mult_param_w)
	AM_RANGE(0x7ff5, 0x7ff6) AM_WRITE(xavix_mult_w)
	AM_RANGE(0x7ff9, 0x7ff9) AM_WRITE(xavix_vector_enable_w)
	AM_RANGE(0x7ffa, 0x7ffa) AM_WRITE(xavix_nmi_vector_lo_w)
	AM_RANGE(0x7ffb, 0x7ffb) AM_WRITE(xavix_nmi_vector_hi_w)
	AM_RANGE(0x7ffc, 0x7ffc) AM_WRITE(xavix_irq_source_w)
	AM_RANGE(0x7ffe, 0x7ffe) AM_WRITE(xavix_irq_vector_lo_w)
	AM_RANGE(0x7fff, 0x7fff) AM_WRITE(xavix_irq_vector_hi_w)
ADDRESS_MAP_END


/*************************************
 *
 *  Input ports
 *
 *************************************/

static UINT32 unknown_random_r(void *param)
{
	return mame_rand() & 3;
}

#define XAVIX_DIPS(name) \
	PORT_DIPNAME( 0x01, 0x00, name ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

/* the analog ports of the ADC, the mouse ports and the region */
#define XAVIX_PORTS_AN(mouse0x, mouse0y, mouse1x, mouse1y) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_START \
	mouse0x \
	PORT_START \
	mouse0y \
	PORT_START \
	mouse1x \
	PORT_START \
	mouse1y \
	PORT_START \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

#define XAVIX_MOUSE_UNUSED PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

#define XAVIX_PORTS_TAIL XAVIX_PORTS_AN(XAVIX_MOUSE_UNUSED, XAVIX_MOUSE_UNUSED, XAVIX_MOUSE_UNUSED, XAVIX_MOUSE_UNUSED)

INPUT_PORTS_START( xavix )
	PORT_START
	XAVIX_DIPS( "IN0" )

	PORT_START
	XAVIX_DIPS( "IN1" )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( xavix_i2c )
	PORT_START
	XAVIX_DIPS( "IN0" )

	PORT_START
	XAVIX_DIPS( "IN1" )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( epo_ebox )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Select")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Back")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )

	PORT_START
	XAVIX_DIPS( "IN1" )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( epo_bowl )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( duelmast )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(unknown_random_r, 0)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( epo_sdb )
	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNUSED )

	XAVIX_PORTS_AN(
		PORT_BIT( 0xff, 0x01, IPT_AD_STICK_X ) PORT_MINMAX(0x01, 0xfe) PORT_SENSITIVITY(25) PORT_KEYDELTA(32) PORT_REVERSE PORT_PLAYER(1),
		PORT_BIT( 0xff, 0x01, IPT_AD_STICK_Y ) PORT_MINMAX(0x01, 0xfe) PORT_SENSITIVITY(25) PORT_KEYDELTA(32) PORT_PLAYER(1),
		PORT_BIT( 0xff, 0x01, IPT_AD_STICK_X ) PORT_MINMAX(0x01, 0xfe) PORT_SENSITIVITY(25) PORT_KEYDELTA(32) PORT_REVERSE PORT_PLAYER(2),
		PORT_BIT( 0xff, 0x01, IPT_AD_STICK_Y ) PORT_MINMAX(0x01, 0xfe) PORT_SENSITIVITY(25) PORT_KEYDELTA(32) PORT_PLAYER(2) )
INPUT_PORTS_END

INPUT_PORTS_START( ttv_lotr )
	PORT_START
	XAVIX_DIPS( "IN0" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(unknown_random_r, 0)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( epo_hamc )
	PORT_START
	XAVIX_DIPS( "IN0" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(unknown_random_r, 0)
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END

INPUT_PORTS_START( ttv_mx )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Accel")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Brake")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Pause")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_NAME("Motion Up")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_NAME("Motion Down")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_NAME("Motion Left")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_NAME("Motion Right")

	PORT_START
	XAVIX_DIPS( "IN1" )

	XAVIX_PORTS_TAIL
INPUT_PORTS_END


/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( xavix2000 )
	MDRV_CPU_ADD_TAG("main", XAVIX2000, XAVIX_MAIN_CLOCK)
	MDRV_CPU_CONFIG(xavix_cpu_config)
	MDRV_CPU_PROGRAM_MAP(extbus_8mb_readmem, extbus_8mb_writemem)
	MDRV_CPU_DATA_MAP(lowbus_readmem, lowbus_writemem)
	MDRV_CPU_VBLANK_INT(xavix_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)

	MDRV_MACHINE_START(xavix)
	MDRV_MACHINE_RESET(xavix)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(xavix)
	MDRV_VIDEO_UPDATE(xavix)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(xavix_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xavix2000_4mb )
	MDRV_IMPORT_FROM(xavix2000)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(extbus_4mb_readmem, extbus_4mb_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xavix2000_4mb_nv )
	MDRV_IMPORT_FROM(xavix2000_4mb)
	MDRV_NVRAM_HANDLER(xavix_nv)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xavix2000_i2c )
	MDRV_IMPORT_FROM(xavix2000)
	MDRV_NVRAM_HANDLER(xavix_i2c)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xavix2000_i2c_4mb )
	MDRV_IMPORT_FROM(xavix2000_i2c)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(extbus_4mb_readmem, extbus_4mb_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xavix2000_i2c_2mb )
	MDRV_IMPORT_FROM(xavix2000_i2c)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(extbus_2mb_readmem, extbus_2mb_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xavix2000_duelmast )
	MDRV_IMPORT_FROM(xavix2000_i2c)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(extbus_duelmast_readmem, extbus_duelmast_writemem)
MACHINE_DRIVER_END


/*************************************
 *
 *  ROM definitions
 *
 *************************************/


ROM_START( epo_sdb )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "superdashball.bin", 0x000000, 0x400000, CRC(a004a764) SHA1(47a96822d4d7d6a0f6be5cd729c3747dbab65979) )
ROM_END

ROM_START( epo_ebox )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "exciteboxing.bin", 0x000000, 0x400000, CRC(e25ae4f5) SHA1(7f7b613f0ab8f43f5cad0d13de538921e77cae9c) )
ROM_END

ROM_START( epo_bowl )
	ROM_REGION( 0x200000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "bowling.bin", 0x000000, 0x200000, CRC(d34f8d9e) SHA1(ebe3792172dc43904b9226beb27f1da89d2388cc) )
ROM_END

ROM_START( epo_golf )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "golf.bin", 0x000000, 0x400000, CRC(d1f231cf) SHA1(9421836a6bc4af9ee1fc7a402d62b2fb4dbcdefc) )
ROM_END

ROM_START( epo_hamc )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "hamster.u1", 0x000000, 0x400000, CRC(b1177813) SHA1(ed01096ebb63b72267ad7e0b2115224bbab64011) )
ROM_END

ROM_START( ban_omt )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "otmj.bin", 0x000000, 0x400000, CRC(1c1dc6fb) SHA1(d0cf1345b765d66ca9a0870ee6d0e3ccd84a8c0b) )
ROM_END

ROM_START( ttv_sw )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "jedi.bin", 0x000000, 0x800000, CRC(51cae5fd) SHA1(1ed8d556f31b4182259ca8c766d60c824d8d9744) )
ROM_END

ROM_START( ttv_swj )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "lightsaber.bin", 0x000000, 0x800000, CRC(a5c22ed0) SHA1(406f0bccb01cd4a26fe4a5675d7ebecc78c58147) )
ROM_END

ROM_START( ttv_lotr )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "lotr.bin", 0x000000, 0x800000, CRC(a034ecd5) SHA1(264a9d4327af0a075841ad6129db67d82cf741f1) )
ROM_END

ROM_START( ttv_mx )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "mxdirtrebel.bin", 0x000000, 0x800000, CRC(e64bf1a1) SHA1(137f97d7d857697a13e0c8984509994dc7bc5fc5) )
ROM_END

ROM_START( tom_jump )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "tom_jump.bin", 0x000000, 0x800000, CRC(20bf5c17) SHA1(bca7535baa6a54ad3ee0929bd3b74a22cb5139da) )
ROM_END

ROM_START( drgqst )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "dragonquest.bin", 0x000000, 0x800000, CRC(3d24413f) SHA1(1677e81cedcf349de7bf091a232dc82c6424efba) )
ROM_END

ROM_START( epo_mini )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "minimoni.u1", 0x000000, 0x400000, CRC(2adb01ee) SHA1(987218b6799195ba15adf39885c1d177c381ec26) )
ROM_END

ROM_START( tak_chq )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "choroq.u2", 0x000000, 0x400000, CRC(ffd2eb95) SHA1(a30884da5554483ebfd0009cf5dd1768be8a99cb) )
ROM_END

ROM_START( ban_onep )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "onepiece.bin", 0x000000, 0x800000, CRC(c5cb5a5f) SHA1(db85f6cc48d77c5a4967b9b8e2999167e3dfc8c8) )
ROM_END

ROM_START( duelmast )
	ROM_REGION( 0x800000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "duelmasters.u4", 0x000000, 0x200000, CRC(2f11fcd7) SHA1(d8849c74833e77b8b309e845523f2cdc7ac68054) )
	ROM_RELOAD( 0x200000, 0x200000 )
	ROM_RELOAD( 0x400000, 0x200000 )
	ROM_RELOAD( 0x600000, 0x200000 )
ROM_END

ROM_START( tom_dpgm )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "disney.bin", 0x000000, 0x400000, CRC(1dc181b3) SHA1(fa30069d17705f27e4ff45e7f6ccf06986e138f3) )
ROM_END

ROM_START( epo_es2j )
	ROM_REGION( 0x400000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "es2j.u3", 0x000000, 0x400000, CRC(840aecb1) SHA1(ad52449ffc13af5f4c67b2c3cf438e7ecd80b9fb) )
ROM_END


/*************************************
 *
 *  Game drivers
 *
 *************************************/

#define NOT_WORKING_SOUND (GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)

GAME( 2002, epo_ebox, 0,       xavix2000_4mb_nv,   epo_ebox, xavix,            ROT0, "Epoch / SSD Company LTD",       "Excite Boxing (Japan)", NOT_WORKING_SOUND )
GAME( 2002, epo_bowl, 0,       xavix2000_i2c_2mb,  epo_bowl, xavix_i2c_24c04,  ROT0, "Epoch / SSD Company LTD",       "Excite Bowling (Japan)", NOT_WORKING_SOUND )
GAME( 2003, epo_golf, 0,       xavix2000_i2c_4mb,  ttv_lotr, xavix_i2c_24c04,  ROT0, "Epoch / SSD Company LTD",       "Super Shot! Excite Golf (Japan)", NOT_WORKING_SOUND )
GAME( 2002, epo_hamc, 0,       xavix2000_4mb,      epo_hamc, xavix,            ROT0, "Epoch / SSD Company LTD",       "Tottoko Hamtaro - Ham Ham Dai Circus! (Japan)", NOT_WORKING_SOUND )
GAME( 2003, epo_mini, 0,       xavix2000_i2c_4mb,  ttv_lotr, xavix_i2c_24c08,  ROT0, "Epoch / SSD Company LTD",       "mini-moni Party! Rhythm de Pyon! (Japan)", NOT_WORKING_SOUND )
GAME( 2006, epo_es2j, 0,       xavix2000_4mb,      xavix,    xavix,            ROT0, "Epoch / SSD Company LTD",       "Card Scan! Excite Stage Soccer Nippon Daihyou Team (Japan)", NOT_WORKING_SOUND )
GAME( 2003, duelmast, 0,       xavix2000_duelmast, duelmast, xavix_duelmast,   ROT0, "Takara / SSD Company LTD",      "Duel Masters: Duel Station (Japan)", NOT_WORKING_SOUND )
GAME( 2004, epo_sdb,  0,       xavix2000_4mb_nv,   epo_sdb,  xavix_sdb,        ROT0, "Epoch / SSD Company LTD",       "Super Dash Ball (Japan)", GAME_IMPERFECT_SOUND )
GAME( 2005, ttv_sw,   0,       xavix2000_i2c,      ttv_lotr, xavix_i2c_24c02,  ROT0, "Tiger / SSD Company LTD",       "Star Wars Saga Edition - Lightsaber Battle Game", NOT_WORKING_SOUND )
GAME( 2005, ttv_swj,  ttv_sw,  xavix2000_i2c,      ttv_lotr, xavix_i2c_24c02,  ROT0, "Tomy / SSD Company LTD",        "Star Wars Saga Edition - Lightsaber Battle Game (Japan)", NOT_WORKING_SOUND )
GAME( 2005, ttv_lotr, 0,       xavix2000_i2c,      ttv_lotr, xavix_i2c_24c02,  ROT0, "Tiger / SSD Company LTD",       "Lord of the Rings - Warrior of Middle-Earth", NOT_WORKING_SOUND )
GAME( 2005, ttv_mx,   0,       xavix2000_i2c,      ttv_mx,   xavix_i2c_24c04,  ROT0, "Tiger / SSD Company LTD",       "MX Dirt Rebel", GAME_IMPERFECT_SOUND )
GAME( 2005, tom_jump, 0,       xavix2000_i2c,      ttv_mx,   xavix_i2c_24c04,  ROT0, "Tomy / SSD Company LTD",        "IDATEN Jump: Gekisou IDATEN Battle (Japan)", GAME_IMPERFECT_SOUND )
GAME( 2003, drgqst,   0,       xavix2000_i2c,      ttv_lotr, xavix_i2c_24c08,  ROT0, "Square Enix / SSD Company LTD", "Kenshin Dragon Quest: Yomigaerishi Densetsu no Ken (Japan)", NOT_WORKING_SOUND )
GAME( 2003, tak_chq,  0,       xavix2000_i2c_4mb,  xavix_i2c, xavix_i2c_24c04, ROT0, "Takara / SSD Company LTD",      "Choro-Q Byun Byun Racer (Japan)", NOT_WORKING_SOUND )
GAME( 2004, ban_onep, 0,       xavix2000_i2c,      ttv_lotr, xavix_i2c_24c04,  ROT0, "Bandai / SSD Company LTD",      "Let's! TV Play Taikan Kakutou One Piece Punch Battle - Kaizoku Ou ni Kimi ga Naru (Japan)", NOT_WORKING_SOUND )
GAME( 2005, ban_omt,  0,       xavix2000_i2c_4mb,  ttv_lotr, xavix_i2c_24c04,  ROT0, "Bandai / SSD Company LTD",      "Let's! TV Play Touin Ougi Onmyou Taisenki: Mezase Saikyou Toushinshi (Japan)", NOT_WORKING_SOUND )
GAME( 2004, tom_dpgm, 0,       xavix2000_i2c_4mb,  ttv_lotr, xavix_i2c_24c08,  ROT0, "Tomy / SSD Company LTD",        "Disney Princess Kirakira Mahou no Lesson (Japan)", NOT_WORKING_SOUND )
