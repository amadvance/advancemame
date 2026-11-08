// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

Tora Tora (c) 1980 Game Plan

driver by Nicola Salmoria, qwijibo

TODO:
- The game reads unmapped ROM addresses. There's an empty
  socket for U3 on the board, which should map at 5000-57ff, however the
  game reads mostly from 4800-4fff, which would be U6 according to the
  schematics.

- The manual mentions dip switch settings and the schematics show the switches,
  the game reads them but ignores them, forcing 1C/1C and 3 lives.
  Maybe the dump is from a proto?

- Verify SN76477 frequencies and the fixed SLF input approximation

***************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "sound/sn76477.h"

static UINT8 timer_count;
static int timer_clk, pia_irq, clear_tv, dsw_enable;
static UINT8 sn_data[2], sn_vco[2];
static double sn_voltage[2];

static void update_irq(void)
{
	cpunum_set_input_line(0, 0, (pia_irq || (timer_count & 0x10)) ? ASSERT_LINE : CLEAR_LINE);
}

static void pia_irq_w(int state)
{
	pia_irq = state;
	update_irq();
}

static WRITE8_HANDLER( toratora_videoram_w )
{
	if (videoram[offset] != data)
	{
		int i;
		int y = offset / 32;
		int x = 8 * (offset % 32);
		videoram[offset] = data;
		for (i = 0; i < 8; i++)
		{
			plot_pixel(tmpbitmap, x + i, y, Machine->pens[(data >> (7 - i)) & 1]);
		}
	}
}

static WRITE8_HANDLER( toratora_clear_tv_w )
{
	clear_tv = 1;
}

static VIDEO_EOF( toratora )
{
	if (clear_tv)
	{
		memset(videoram, 0, 0x2000);
		fillbitmap(tmpbitmap, Machine->pens[0], NULL);
		clear_tv = 0;
	}
}

static READ8_HANDLER( porta_0_r )
{
	return readinputport(0);
}
static READ8_HANDLER( ca1_0_r )
{
	return (readinputport(1) >> 1) & 1;
}
static READ8_HANDLER( ca2_0_r )
{
	return readinputport(1) & 1;
}

static WRITE8_HANDLER( portb_0_w )
{
	coin_counter_w(0, (data | ~pia_get_ddr_b(0)) & 0x20);
}

static READ8_HANDLER( portb_1_r )
{
	return dsw_enable ? readinputport(2) : 0;
}

static void sn_update(int chip)
{
	static const double resistances[2][8] =
	{
		{ 0, 0, RES_K(435), RES_K(195), RES_K(95), RES_K(44), RES_K(20), 0 },
		{ 0, RES_K(435) + RES_M(242), RES_K(435), RES_K(195), RES_K(95), RES_K(44), RES_K(20), 0 }
	};
	int res = sn_data[chip] >> 5;
	int mixer = sn_data[chip] & 7;
	int fixed_slf = res == 7;

	/* Approximate the fixed-high SLF input with the available mixer modes. */
	if (fixed_slf)
	{
		if (mixer == 4) mixer = 2;
		if (mixer == 5) mixer = 3;
		if (mixer == 6) mixer = 0;
	}
	SN76477_mixer_w(chip, mixer);
	SN76477_envelope_1_w(chip, (sn_data[chip] >> 3) & 1);
	SN76477_envelope_2_w(chip, (sn_data[chip] >> 4) & 1);
	SN76477_set_slf_res(chip, resistances[chip][res]);
	SN76477_set_amplitude_res(chip, chip == 0 && res == 0 ? 0 : RES_K(47));
	SN76477_vco_w(chip, fixed_slf ? 0 : sn_vco[chip]);
	SN76477_set_vco_voltage(chip, fixed_slf && sn_vco[chip] ? 2.5 : sn_voltage[chip]);
}

static void sn_voltage_w(int chip, int data)
{
	sn_voltage[chip] = 2.35 * (data & 0x7f) / 128.0;
	SN76477_enable_w(chip, (data >> 7) & 1);
	sn_update(chip);
}

static WRITE8_HANDLER( sn1_voltage_w )
{
	sn_voltage_w(0, data);
}
static WRITE8_HANDLER( sn2_voltage_w )
{
	sn_voltage_w(1, data);
}
static WRITE8_HANDLER( sn1_w )
{
	sn_data[0] = data;
	sn_update(0);
}
static WRITE8_HANDLER( sn2_w )
{
	sn_data[1] = data;
	sn_update(1);
}
static WRITE8_HANDLER( sn1_vco_w )
{
	sn_vco[0] = data & 1;
	sn_update(0);
}
static WRITE8_HANDLER( sn2_vco_w )
{
	sn_vco[1] = data & 1;
	sn_update(1);
}

static WRITE8_HANDLER( dsw_enable_w )
{
	dsw_enable = !(data & 1);
}

static const pia6821_interface pia0_intf =
{
	porta_0_r, NULL, ca1_0_r, NULL, ca2_0_r, NULL,
	NULL, portb_0_w, NULL, NULL,
	pia_irq_w, pia_irq_w
};

static const pia6821_interface pia1_intf =
{
	NULL, portb_1_r, NULL, NULL, NULL, NULL,
	sn1_voltage_w, sn1_w, sn1_vco_w, dsw_enable_w,
	NULL, NULL
};

static const pia6821_interface pia2_intf =
{
	NULL, NULL, NULL, NULL, NULL, NULL,
	sn2_voltage_w, sn2_w, sn2_vco_w, NULL,
	NULL, NULL
};

static MACHINE_START( toratora )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &pia2_intf);
	return 0;
}

static MACHINE_RESET( toratora )
{
	int i;
	timer_count = 0;
	timer_clk = pia_irq = clear_tv = 0;
	dsw_enable = 1;
	memset(sn_data, 0, sizeof(sn_data));
	memset(sn_vco, 0, sizeof(sn_vco));
	memset(sn_voltage, 0, sizeof(sn_voltage));
	pia_reset();
	pia_set_input_ca1(0, 0);
	pia_set_input_ca2(0, 0);
	for (i = 0; i < 2; i++)
	{
		SN76477_enable_w(i, 1);
		sn_update(i);
	}
	update_irq();
}

static INTERRUPT_GEN( toratora_timer )
{
	activecpu_adjust_icount(-64);
	pia_set_input_ca2(0, readinputport(1) & 1);
	pia_set_input_ca1(0, (readinputport(1) >> 1) & 1);
	timer_clk = !timer_clk;
	if (timer_clk)
	{
		timer_count++;
		update_irq();
		if (!timer_count)
			mame_schedule_soft_reset();
	}
}

static READ8_HANDLER( toratora_timer_r )
{
	return timer_count;
}

static WRITE8_HANDLER( toratora_clear_timer_w )
{
	timer_count = 0;
	update_irq();
}

static ADDRESS_MAP_START( mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_READWRITE(MRA8_RAM, toratora_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xa000, 0xf049) AM_NOP
	AM_RANGE(0xf04a, 0xf04a) AM_WRITE(toratora_clear_tv_w)
	AM_RANGE(0xf04b, 0xf04b) AM_READWRITE(toratora_timer_r, toratora_clear_timer_w)
	AM_RANGE(0xf04c, 0xf09f) AM_NOP
	AM_RANGE(0xf0a0, 0xf0a3) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0xf0a4, 0xf0a7) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xf0a8, 0xf0ab) AM_READWRITE(pia_2_r, pia_2_w)
	AM_RANGE(0xf0ac, 0xf7ff) AM_NOP
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static INPUT_PORTS_START( toratora )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static struct SN76477interface sn76477_intf =
{
	RES_K(47), RES_K(470), CAP_P(470), RES_M(2),
	CAP_U(0.2), RES_K(3.3), RES_K(47), RES_K(50),
	0, CAP_U(0.1), RES_K(51), 5.0,
	RES_K(10), CAP_U(1.0), CAP_U(0.1), RES_K(100)
};

static MACHINE_DRIVER_START( toratora )
	MDRV_CPU_ADD(M6800, 5185000/8)
	MDRV_CPU_PROGRAM_MAP(mem,0)
	MDRV_CPU_PERIODIC_INT(toratora_timer,TIME_IN_HZ(500))
	MDRV_MACHINE_START(toratora)
	MDRV_MACHINE_RESET(toratora)
	MDRV_FRAMES_PER_SECOND(5185000.0/296/291)
	MDRV_VBLANK_DURATION(1000000.0 * 51 * 296 / 5185000)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 291)
	MDRV_VISIBLE_AREA(0,255,8,247)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
	MDRV_VIDEO_EOF(toratora)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END

ROM_START( toratora )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tora.u1",      0x1000, 0x0800, CRC(413c743a) SHA1(a887dfaaee557327a1699bb424488b934dab8612) )
	ROM_LOAD( "tora.u10",     0x1800, 0x0800, CRC(dc771b1c) SHA1(1bd81decb4d0a854878227c52d45ac0eea0602ec) )
	ROM_LOAD( "tora.u2",      0x2000, 0x0800, CRC(c574c664) SHA1(9f41a53ca51d04e5bec7525fe83c5f4bdfcf128d) )
	ROM_LOAD( "tora.u9",      0x2800, 0x0800, CRC(b67aa11f) SHA1(da9e77255640a4b32eed2be89b686b98a248bd72) )
	ROM_LOAD( "tora.u11",     0xf800, 0x0800, CRC(55135d6f) SHA1(c48f180a9d6e894aafe87b2daf74e9a082f4600e) )
ROM_END

GAME( 1980, toratora, 0, toratora, toratora, 0, ROT90, "Game Plan", "Tora Tora (prototype?)", GAME_IMPERFECT_SOUND )
