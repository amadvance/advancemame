// license:BSD-3-Clause
// copyright-holders:Roberto Fresca, Grull Osgo
/*
 * Potten's Poker stealth with Breakout front game.
 * PCB 5981-A, unknown manufacturer and year.
 *
 * Hold Up during reset to enable poker, then use Q for poker credits,
 * Start 2 to bet and Start 1 to deal. With no poker credits, Breakout runs.
 */

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "sound/ay8910.h"
#include "vidhrdw/tms9928a.h"

#define CPU_CLOCK (10738000 / 3)

static ADDRESS_MAP_START( pokerout_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pokerout_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0x40, 0x40) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x41, 0x41) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x42, 0x42) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x80, 0x80) AM_READ(input_port_0_r)
	AM_RANGE(0x90, 0x90) AM_READ(input_port_1_r)
	AM_RANGE(0xa0, 0xa0) AM_READ(input_port_2_r)
	AM_RANGE(0xc0, 0xc3) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START(pokerout)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY  PORT_NAME("Right / Small")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_8WAY  PORT_NAME("Down / Hold / Take")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER )   PORT_NAME("Spare C / Poker Credits")  PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER )   PORT_NAME("Reserved") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER )   PORT_NAME("Spare P / Poker Payout")  PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test 2")   PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )  PORT_NAME("Start 1 / Draw")

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("IN1-01") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("IN1-02") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("IN1-04") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("IN1-08") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY    PORT_NAME("Up / Cancel / Double-Up")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY  PORT_NAME("Left / Big")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )  PORT_NAME("Start 2 / Bet")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test 1") PORT_CODE(KEYCODE_9)

	PORT_START
	/* The boot checks require the fixed input value 0x7d on this port. */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME(0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(0x00, "10 Credits" )
	PORT_DIPSETTING(0x01, "20 Credits" )
	PORT_DIPSETTING(0x02, "50 Credits" )
	PORT_DIPSETTING(0x03, "100 Credits" )
	PORT_DIPNAME(0x0c, 0x0c, "Maximum Bet" )
	PORT_DIPSETTING(0x0c, "10" )
	PORT_DIPSETTING(0x08, "20" )
	PORT_DIPSETTING(0x04, "50" )
	PORT_DIPSETTING(0x00, "100" )
	PORT_DIPNAME(0x10, 0x10, "Arcade Mode")
	PORT_DIPSETTING(0x10, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x20, 0x20, DEF_STR(Unknown))
	PORT_DIPSETTING(0x20, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x40, 0x40, DEF_STR(Unknown))
	PORT_DIPSETTING(0x40, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x80, 0x80, DEF_STR(Unknown))
	PORT_DIPSETTING(0x80, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
INPUT_PORTS_END

static void pokerout_irq(int state)
{
	cpunum_set_input_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static z80ctc_interface ctc_interface = { CPU_CLOCK, 0, pokerout_irq, 0, 0, 0 };
static struct z80_irq_daisy_chain daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static MACHINE_START( pokerout )
{
	z80ctc_init(0, &ctc_interface);
	return 0;
}

static MACHINE_RESET( pokerout )
{
	z80ctc_reset(0);
}

static INTERRUPT_GEN( pokerout_vblank )
{
	/* The VDP status must advance even though its interrupt pin is disconnected. */
	TMS9928A_interrupt();
}

static const TMS9928a_interface vdp_interface = { TMS9929A, 0x4000, 0 };
static const struct AY8910interface psg_interface = { input_port_3_r, 0, 0, 0 };

static MACHINE_DRIVER_START( pokerout )
	MDRV_CPU_ADD(Z80, CPU_CLOCK)
	MDRV_CPU_CONFIG(daisy_chain)
	MDRV_CPU_PROGRAM_MAP(pokerout_mem, 0)
	MDRV_CPU_IO_MAP(pokerout_io, 0)
	MDRV_CPU_VBLANK_INT(pokerout_vblank, 1)
	MDRV_MACHINE_START(pokerout)
	MDRV_MACHINE_RESET(pokerout)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_TMS9928A(&vdp_interface)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, CPU_CLOCK / 2)
	MDRV_SOUND_CONFIG(psg_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START(pokerout)
	ROM_REGION(0x8000, REGION_CPU1, 0)
	ROM_LOAD("brk01_tms128a.6f", 0x0000, 0x4000, CRC(ccf2e9d7) SHA1(b48630d1e6c223aacf4f785b2a70c5c3ed781a51))
	ROM_LOAD("f_tms128a.8f",     0x4000, 0x4000, CRC(57a7bff2) SHA1(a466c881fcd2a339960936e21da7e2079e7d75ca))
ROM_END

GAME( 19??, pokerout, 0, pokerout, pokerout, 0, ROT0, "<unknown>", "Potten's Poker stealth with Breakout front game", 0 )
