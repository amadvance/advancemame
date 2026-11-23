// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    Microprose Games 3D hardware

    driver by Phil Bennett

    Games supported:
        * F-15 Strike Eagle [3 sets]
        * B.O.T.S.S.: Battle of the Solar System [2 sets]
        * Tank Battle (prototype)
        * Super Tank Attack (prototype)

    TODO:
        * DS1215 Phantom time chip
        * DS1267 Volume control

    Known issues:
        * Games are running too smoothly!
        * F-15 controls need tweaking
        * botss will hit a divide-by-zero error if the throttle is
          not calibrated first in service mode. (clone is ok)

****************************************************************************/
/*
    Differences from MAME:

    The MC68901 MFP and the MC68681 DUART are simplified models of the
    features used by the games: the MFP timers run at fixed rates, the DUART
    channel B exchanges whole characters with the 8031 serial port, paced at
    9600 baud, and channel A only prints the host monitor output.

    The Z8530 SCC of the Dr. Math monitor port only reports an idle
    transmitter and no carrier.

    The 68000 interrupt levels raised by the DUART, the MFP and the Am29000
    are kept pending until acknowledged, as the 68000 interface acknowledges
    a single level at a time.

    The 2D layer is drawn from the DPYADR latched at VBLANK instead of a
    TMS34010 scanline callback.
*/

#include "driver.h"
#include <stdio.h>
//#include "machine/ds1215.h"
#include "includes/micro3d.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "cpu/i8051/i8051.h"
#include "cpu/am29000/am29000.h"
#include "cpu/m68000/m68000.h"
#include "sound/2151intf.h"
#include "sound/upd7759.h"
#include "sound/custom.h"

#define M68681_CLK 3686400
#define M68901_CLK 4000000
#define M68901_XTAL1 4000000
#define TIMERA 250
#define TIMERB 250
#define TIMERC 250
#define TIMERD 250
#define DEBUG 1

/*====================================================================*/
/* Each CPU (bar 80C31) communicates with a computer plugged into the */
/* corresponding monitor port.                                        */
/* Status information is reported, and tests can be performed.        */
/*                                                                    */
/* We print this useful stuff out to the text prompt.                 */
/*====================================================================*/

#ifdef DEBUG
#define HOST_MONITOR_DISPLAY 0
#define VGB_MONITOR_DISPLAY 0
#define DRMATH_MONITOR_DISPLAY 0
#endif

UINT16 *micro3d_sprite_vram;
static UINT16 *shared_ram;
static UINT32 *mac_sram;
static mame_timer *mac_done_timer;
static mame_timer *sound_tx_timer;
static mame_timer *adc_timer;
static void adc_done_callback(int param);
static void sound_tx_callback(int param);
static void mac_done_callback(int param);
static UINT16 *m68681_base;
static UINT16 *m68901_base;
UINT16 dpyadr;
int dpyadrscan;

static struct {
    UINT16 MR1A;
    UINT16 MR2A;
    UINT16 SRA;
    UINT16 CSRA;
    UINT16 CRA;
    UINT16 RBA;
    UINT16 TBA;
    UINT16 IPCR;
    UINT16 ACR;
    UINT16 ISR;
    UINT16 IMR;
    UINT16 CUR;
    UINT16 CTUR;
    UINT16 CLR;
    UINT16 CTLR;
    UINT16 MR1B;
    UINT16 MR2B;
    UINT16 SRB;
    UINT16 CSRB;
    UINT16 CRB;
    UINT16 RBB;
    UINT16 TBB;
    UINT16 IVR;
    UINT16 IPORT;
    UINT16 OPCR;
    UINT16 OPR;

    int MRA_ptr;
    int MRB_ptr;
} M68681;

static UINT8 ti_uart[9];
static int ti_uart_mode_cycle=0;
static int ti_uart_sync_cycle=0;

void m68901_int_gen(int source);

/* 68901 */
enum{   TMRB=0,TXERR,TBE,RXERR,RBF,TMRA,GPIP6,GPIP7,     // A Registers
    	GPIP0,GPIP1,GPIP2,GPIP3,TMRD,TMRC,GPIP4,GPIP5    // B Registers
};

/* TI UART */
enum{   RX=0,TX,STATUS,SYN1,SYN2,DLE,MODE1,MODE2,COMMAND
};


/*
 * The 0.106 68000 interface keeps a single interrupt level: acknowledging a
 * level clears it, and with it any other level raised meanwhile. The DUART
 * (3), the MFP (4) and the Am29000 (5) requests are kept pending here until
 * the 68000 acknowledges each of them, otherwise a Dr. Math request arriving
 * during an MFP interrupt is lost and the host waits for it forever.
 */
static int host_irq_pending;
static int sound_rx_enabled;

static void host_update_irq(void)
{
	if (host_irq_pending & (1 << 5))
		cpunum_set_input_line(0, 5, ASSERT_LINE);
	else if (host_irq_pending & (1 << 4))
		cpunum_set_input_line(0, 4, ASSERT_LINE);
	else if (host_irq_pending & (1 << 3))
		cpunum_set_input_line(0, 3, ASSERT_LINE);
	else
		cpunum_set_input_line(0, 3, CLEAR_LINE);
}

static void host_irq(int level)
{
	host_irq_pending |= 1 << level;
	host_update_irq();
}

/* 68681 interrupt status: TxRDYA, RxRDYA, TxRDYB, RxRDYB */
static UINT8 duart_isr(void)
{
	return ((M68681.SRA & 0x0400) ? 0x01 : 0) |
	       ((M68681.SRA & 0x0100) ? 0x02 : 0) |
	       ((M68681.SRB & 0x0400) ? 0x10 : 0) |
	       ((M68681.SRB & 0x0100) ? 0x20 : 0);
}

/* the 68681 interrupt output is a level: it stays active while an unmasked
   condition is set, the transmitters ask for data as long as they are enabled */
static void duart_update_irq(void)
{
	if (duart_isr() & M68681.IMR)
		host_irq(3);
}

static int host_irq_callback(int line)
{
	host_irq_pending &= ~(1 << line);
	if (line == 3 && (duart_isr() & M68681.IMR))
		host_irq_pending |= 1 << 3;
	host_update_irq();

	/* the DUART supplies its vector, the others are autovectored */
	if (line == 3)
		return M68681.IVR;
	return MC68000_INT_ACK_AUTOVECTOR;
}

/* 68681 channel B is connected to the 8031 serial port */
int data_to_i8031(void)
{
     return M68681.TBB;
}

/*
 * The 8031 core latches a received byte as soon as its receive line is
 * signalled: the bytes the host writes back to back are queued and sent one
 * per character time, otherwise each one overwrites the previous before the
 * 8031 reads it and the sound board never answers the host commands.
 */
#define SOUND_TX_FIFO_SIZE 64

static UINT8 sound_tx_fifo[SOUND_TX_FIFO_SIZE];
static int sound_tx_head;
static int sound_tx_count;

static void sound_tx_callback(int param)
{
     if (sound_tx_count == 0)
          return;

     M68681.TBB = sound_tx_fifo[sound_tx_head];
     sound_tx_head = (sound_tx_head + 1) % SOUND_TX_FIFO_SIZE;
     --sound_tx_count;
     cpunum_set_input_line(2, I8051_RX_LINE, ASSERT_LINE);

     /* one character at 9600 baud */
     if (sound_tx_count != 0)
          timer_adjust(sound_tx_timer, TIME_IN_USEC(1042), 0, 0);
}

static void sound_tx_w(UINT8 data)
{
     if (sound_tx_count < SOUND_TX_FIFO_SIZE)
     {
          sound_tx_fifo[(sound_tx_head + sound_tx_count) % SOUND_TX_FIFO_SIZE] = data;
          ++sound_tx_count;
     }

     /* start sending if the line is idle */
     if (sound_tx_count == 1)
          timer_adjust(sound_tx_timer, TIME_IN_USEC(1042), 0, 0);
}

void data_from_i8031(int data)
{
     /* the DUART drops the characters received while its receiver is disabled */
     if (!sound_rx_enabled)
          return;

     M68681.RBB  = data<<8;                         // Put into receive buffer.
     M68681.SRB |= 0x0100;                          // Set Receiver B ready.
     duart_update_irq();
}


/* Pinched from coolpool.c */
WRITE16_HANDLER( micro3d_34010_io_register_w )
{
	if (offset == REG_DPYADR || offset == REG_DPYTAP)
		force_partial_update(cpu_getscanline());
	tms34010_io_register_w(offset, data, mem_mask);

	if (offset == REG_DPYADR)
	{
		dpyadr = ~data & 0xfffc;
		dpyadrscan = cpu_getscanline() + 1;
	}
}

/* Also pinched from coolpool.c */
INTERRUPT_GEN( micro3d_tms_vblank )
{
	/* dpyadr is latched from dpystrt at the beginning of VBLANK every frame */
	cpuintrf_push_context(0);

	/* due to timing issues, we sometimes set the DPYADR just before we get here;
       in order to avoid trouncing that value, we look for the last scanline */
	if (dpyadrscan < Machine->visible_area.max_y)
		dpyadr = ~tms34010_io_register_r(REG_DPYSTRT, 0) & 0xfffc;
	dpyadrscan = 0;

	cpuintrf_pop_context();
}


static MACHINE_RESET( micro3d )
{
        i8051_set_serial_tx_callback(data_from_i8031);
        i8051_set_serial_rx_callback(data_to_i8031);
        host_irq_pending = 0;
        sound_rx_enabled = 0;
        M68681.OPR = 0;
        cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
        sound_tx_head = 0;
        sound_tx_count = 0;
        timer_adjust(sound_tx_timer, TIME_NEVER, 0, 0);
        cpunum_set_irq_callback(0, host_irq_callback);
        micro3d_video_reset();
        cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
        cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);
        timer_adjust(mac_done_timer, TIME_NEVER, 0, 0);
        ti_uart[STATUS]=1;
        M68681.SRA=0x0500;
        M68681.SRB=0x0500;
}

/*
 * The botss rev. 1.1a code checks the hardware version reading 0x140000 and
 * 0x180000 and expecting the last one read on bit 7 of inputs A and B.
 * Without it, it stops with "SYSTEM ERROR#126: INCOMPATIBLE HARDWARE".
 */
static UINT16 botss_latch;

static READ16_HANDLER( botss_140000_r )
{
	botss_latch = 0;
	return 0xffff;
}

static READ16_HANDLER( botss_180000_r )
{
	botss_latch = 1;
	return 0xffff;
}

static READ16_HANDLER( botss_inputs_a_b_r )
{
	return (input_port_0_word_r(0, 0) & ~0x0080) | (botss_latch << 7);
}

static DRIVER_INIT( micro3d )
{
	mac_done_timer = timer_alloc(mac_done_callback);
	sound_tx_timer = timer_alloc(sound_tx_callback);
	adc_timer = timer_alloc(adc_done_callback);

	/* The Am29000 program seems to rely on RAM from 0x00470000 onwards being
       non-zero on a reset, otherwise the 3D object data doesn't get uploaded! */
	cpuintrf_push_context(3);
	data_write_dword_32be(0x00470000, 0xa5a5a5a5);
	cpuintrf_pop_context();

	/* TODO? BOTSS sometimes crashes when starting a stage because the 68000
       overwrites memory in use by the Am29000. Slowing down the 68000 slightly
       avoids this. */
	cpunum_set_clockscale(0, 0.945);
}

static DRIVER_INIT( botss )
{
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x140000, 0x140001, 0, 0, botss_140000_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x180000, 0x180001, 0, 0, botss_180000_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x940000, 0x940001, 0, 0, botss_inputs_a_b_r);

	init_micro3d();
}

INPUT_PORTS_START( f15se )
	PORT_START	/* INPUTS_A_B */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, "Shared Memory Handshake Test")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Dr. Math Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Burn-in Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, "Manufacturing Tests")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Host Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Decoy")
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* INPUTS_C_D */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Missile")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Target Select")
	PORT_BIT( 0x88ff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* VGB_SW */
	PORT_DIPNAME( 0x0008, 0x0008, "VGB Monitor Mode")
	PORT_DIPSETTING(    0x0008, DEF_STR(Off) )
	PORT_DIPSETTING(    0x0000, DEF_STR(On) )

	PORT_START	/* SOUND_SW */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Sound PCB Test SW") PORT_CODE(KEYCODE_F1)

	PORT_START_TAG("JOYSTICK_X")
	PORT_BIT( 0x1ff, 0xa6, IPT_AD_STICK_X ) PORT_MINMAX(0x000,0x14c) PORT_SENSITIVITY(25) PORT_KEYDELTA(50) PORT_REVERSE

	PORT_START_TAG("JOYSTICK_Y")
	PORT_BIT( 0x1ff, 0xa6, IPT_AD_STICK_Y ) PORT_MINMAX(0x000,0x14c) PORT_SENSITIVITY(25) PORT_KEYDELTA(50)

	PORT_START_TAG("THROTTLE")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Z ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_NAME("Throttle")
INPUT_PORTS_END

INPUT_PORTS_START( botss )
	PORT_START	/* INPUTS_A_B */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, "Shared Memory Handshake Test")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Dr. Math Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Burn-in Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, "Manufacturing Tests")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* hardware check, see botss_inputs_a_b_r */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Shield")
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0x7800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* INPUTS_C_D */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Blaster")
	PORT_BIT( 0xccff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* VGB_SW */
	PORT_DIPNAME( 0x0008, 0x0008, "VGB Monitor Mode")
	PORT_DIPSETTING(    0x0008, DEF_STR(Off) )
	PORT_DIPSETTING(    0x0000, DEF_STR(On) )

	PORT_START	/* SOUND_SW */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Sound PCB Test SW") PORT_CODE(KEYCODE_F1)

	PORT_START_TAG("JOYSTICK_X")
	PORT_BIT( 0x1ff, 0xa6, IPT_AD_STICK_X ) PORT_MINMAX(0x000,0x14c) PORT_SENSITIVITY(25) PORT_KEYDELTA(50) PORT_REVERSE

	PORT_START_TAG("JOYSTICK_Y")
	PORT_BIT( 0x1ff, 0xa6, IPT_AD_STICK_Y ) PORT_MINMAX(0x000,0x14c) PORT_SENSITIVITY(25) PORT_KEYDELTA(50)

	PORT_START_TAG("THROTTLE")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Z ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_NAME("Throttle")
INPUT_PORTS_END

INPUT_PORTS_START( botss11 )
	PORT_START	/* INPUTS_A_B */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, "Shared Memory Handshake Test")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Dr. Math Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Burn-in Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, "Manufacturing Tests")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Host Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Shield")
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* INPUTS_C_D */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Throttle Up")
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Throttle Down")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Blaster")
	PORT_BIT( 0xccf5, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* VGB_SW */
	PORT_DIPNAME( 0x0008, 0x0008, "VGB Monitor Mode")
	PORT_DIPSETTING(    0x0008, DEF_STR(Off) )
	PORT_DIPSETTING(    0x0000, DEF_STR(On) )

	PORT_START	/* SOUND_SW */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Sound PCB Test SW") PORT_CODE(KEYCODE_F1)
INPUT_PORTS_END

INPUT_PORTS_START( tankbatl )
	PORT_START	/* INPUTS_A_B */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, "Shared Memory Handshake Test")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, "Dr. Math Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, "Burn-in Tests" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0000, "Manufacturing Tests")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, "Host Monitor Mode")
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )
	PORT_SERVICE( 0x0400, IP_ACTIVE_LOW )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0xc000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* INPUTS_C_D */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START	/* VGB_SW */
	PORT_DIPNAME( 0x0008, 0x0008, "VGB Monitor Mode")
	PORT_DIPSETTING(    0x0008, DEF_STR(Off) )
	PORT_DIPSETTING(    0x0000, DEF_STR(On) )

	PORT_START	/* SOUND_SW */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Sound PCB Test SW") PORT_CODE(KEYCODE_F1)
INPUT_PORTS_END



void tms_interrupt(int state)
{
   m68901_int_gen(GPIP4);
}

void micro3d_vblank(void)
{
   micro3d_vblank_buffers();
   m68901_int_gen(GPIP7);
}


/*************************************
 *
 *  Math unit
 *
 *************************************/

#define MAC_CLK 10000000
#define VTXROM_FMT(x)       (((x) << 14) | ((x) & (1 << 15) ? 0xc0000000 : 0))

static UINT32 sram_r_addr;
static UINT32 sram_w_addr;
static UINT32 vtx_addr;
static UINT32 mrab11;
static UINT32 mac_stat;
static UINT32 mac_inst;

static WRITE32_HANDLER( shared_w )
{
	shared_ram[offset * 2 + 1] = data & 0xffff;
	shared_ram[offset * 2 + 0] = data >> 16;
}

static READ32_HANDLER( shared_r )
{
	return (shared_ram[offset * 2] << 16) | shared_ram[offset * 2 + 1];
}

static WRITE32_HANDLER( drmath_int_w )
{
	host_irq(5);
}

static WRITE32_HANDLER( drmath_intr2_ack )
{
	cpunum_set_input_line(3, AM29000_INTR2, CLEAR_LINE);
}

static INT64 normalised_multiply(INT32 a, INT32 b)
{
	INT64 result = (INT64)a * (INT64)b;
	return result >> 14;
}

static void mac_done_callback(int param)
{
	cpunum_set_input_line(3, AM29000_INTR0, ASSERT_LINE);
	mac_stat = 0;
}

static WRITE32_HANDLER( mac1_w )
{
	vtx_addr = (data & 0x3ffff);
	sram_w_addr = (data >> 18) & 0xfff;
}

static READ32_HANDLER( mac2_r )
{
	return (mac_inst << 1) | mac_stat;
}

static WRITE32_HANDLER( mac2_w )
{
	UINT32 i;
	UINT32 cnt = data & 0xff;
	UINT32 inst = (data >> 8) & 0x1f;
	UINT32 mac_cycles = 1;
	const UINT16 *rom = (const UINT16 *)memory_region(REGION_USER3);

	mac_stat = (data >> 13) & 1;
	mac_inst = inst & 0x7;
	mrab11 = (data >> 18) & (1 << 11);
	sram_r_addr = (data >> 18) & 0xfff;

	if ((data >> 14) & 1)
		cpunum_set_input_line(3, AM29000_INTR0, CLEAR_LINE);

	switch (inst)
	{
		case 0x00: break;
		case 0x04: break;

		case 0x0b: cnt += 0x100;
		case 0x0a: cnt += 0x100;
		case 0x09: cnt += 0x100;
		case 0x08:
		{
			for (i = 0; i <= cnt; ++i)
			{
				INT64 acc;
				INT32 x, y, z;

				x = VTXROM_FMT(rom[vtx_addr]);   vtx_addr++;
				y = VTXROM_FMT(rom[vtx_addr]);   vtx_addr++;
				z = VTXROM_FMT(rom[vtx_addr]);   vtx_addr++;

				acc  = normalised_multiply(mac_sram[mrab11 + 0x7f0], x);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f1], y);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f2], z);
				acc += mac_sram[mrab11 + 0x7f3];
				mac_sram[sram_w_addr++] = acc;

				acc  = normalised_multiply(mac_sram[mrab11 + 0x7f4], x);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f5], y);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f6], z);
				acc += mac_sram[mrab11 + 0x7f7];
				mac_sram[sram_w_addr++] = acc;

				acc  = normalised_multiply(mac_sram[mrab11 + 0x7f8], x);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f9], y);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7fa], z);
				acc += mac_sram[mrab11 + 0x7fb];
				mac_sram[sram_w_addr++] = acc;

				mac_cycles = 16 * cnt;
			}

			break;
		}
		case 0x0e: cnt += 0x100;
		case 0x0d: cnt += 0x100;
		case 0x0c:
		{
			for (i = 0; i <= cnt; ++i)
			{
				INT64 acc;
				INT32 x, y, z;

				x = VTXROM_FMT(rom[vtx_addr]);   vtx_addr++;
				y = VTXROM_FMT(rom[vtx_addr]);   vtx_addr++;
				z = VTXROM_FMT(rom[vtx_addr]);   vtx_addr++;

				acc  = normalised_multiply(mac_sram[mrab11 + 0x7f0], x);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f1], y);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f2], z);
				mac_sram[sram_w_addr++] = acc;

				acc  = normalised_multiply(mac_sram[mrab11 + 0x7f4], x);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f5], y);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f6], z);
				mac_sram[sram_w_addr++] = acc;

				acc  = normalised_multiply(mac_sram[mrab11 + 0x7f8], x);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7f9], y);
				acc += normalised_multiply(mac_sram[mrab11 + 0x7fa], z);
				mac_sram[sram_w_addr++] = acc;

				mac_cycles = 12 * cnt;
			}
			break;
		}
		case 0x0f:
		{
			for (i = 0; i <= cnt; ++i, vtx_addr += 4)
			{
				mac_sram[sram_w_addr++] = VTXROM_FMT(rom[vtx_addr + 0]);
				mac_sram[sram_w_addr++] = VTXROM_FMT(rom[vtx_addr + 1]);
				mac_sram[sram_w_addr++] = VTXROM_FMT(rom[vtx_addr + 2]);
				mac_sram[sram_w_addr++] = VTXROM_FMT(rom[vtx_addr + 3]);
			}

			mac_cycles = 8 * cnt;
			break;
		}
		/* Dot product of SRAM vectors with single SRAM vector */
		case 0x11: cnt += 0x100;
		case 0x10:
		{
			INT32 x2 = mac_sram[mrab11 + 0x7fc];
			INT32 y2 = mac_sram[mrab11 + 0x7fd];
			INT32 z2 = mac_sram[mrab11 + 0x7fe];

			for (i = 0; i <= cnt; ++i)
			{
				INT64 dp;
				INT32 x1, y1, z1;

				x1 = mac_sram[sram_r_addr++];
				y1 = mac_sram[sram_r_addr++];
				z1 = mac_sram[sram_r_addr++];

				dp = (INT64)x1 * (INT64)x2 + (INT64)y1 * (INT64)y2 + (INT64)z1 * (INT64)z2;
				mac_sram[sram_w_addr++] = dp >> 32;
				mac_sram[sram_w_addr++] = dp & 0xffffffff;
				mac_sram[sram_w_addr++] = 0;
			}

			mac_cycles = 10 * cnt;
			break;
		}
		/* Dot product of SRAM vectors with SRAM vectors */
		case 0x16: cnt += 0x100;
		case 0x15: cnt += 0x100;
		case 0x14:
		{
			for (i = 0; i <= cnt; ++i)
			{
				INT64 dp;
				INT32 x1, y1, z1;
				INT32 x2, y2, z2;

				x1 = mac_sram[sram_r_addr++];
				y1 = mac_sram[sram_r_addr++];
				z1 = mac_sram[sram_r_addr++];

				x2 = mac_sram[vtx_addr++];
				y2 = mac_sram[vtx_addr++];
				z2 = mac_sram[vtx_addr++];

				dp = (INT64)x1 * (INT64)x2 + (INT64)y1 * (INT64)y2 + (INT64)z1 * (INT64)z2;
				mac_sram[sram_w_addr++] = dp >> 32;
				mac_sram[sram_w_addr++] = dp & 0xffffffff;
				mac_sram[sram_w_addr++] = 0;
			}

			mac_cycles = 10 * cnt;
			break;
		}
		default:
			logerror("Unknown MAC instruction : %x\n", inst);
			break;
	}

	/* TODO: Calculate a better estimate for timing */
	if (mac_stat)
		timer_adjust(mac_done_timer, mac_cycles * TIME_IN_HZ(MAC_CLK), 0, 0);
}

static WRITE16_HANDLER( reset_w )
{
	cpunum_set_input_line(3, INPUT_LINE_RESET, (data & 0x0100) ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x0200) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE16_HANDLER( host_drmath_int_w )
{
	cpunum_set_input_line(3, AM29000_INTR2, ASSERT_LINE);
	cpu_boost_interleave(0, TIME_IN_USEC(10));
}


/* 68901 related gubbins. Re-write me please */

static void timera_int(int param)
{
//      timer_set(TIME_IN_SEC(((m68901_base[0xf]>>8) & 0xff)* 200/M68901_CLK),0,timera_int);     // Set the timer again.
        timer_set(TIME_IN_USEC(1000),0,timera_int);     // Set the timer again.
        m68901_int_gen(TMRA);           // Fire an interrupt.
}

static void timerb_int(int param)
{
        timer_set(TIME_IN_SEC(((m68901_base[0x10]>>8) & 0xff) * 200/M68901_CLK),0,timerb_int);
        m68901_int_gen(TMRB);           // Fire an interrupt.
}

static void timerc_int(int param)
{
        timer_set(TIME_IN_SEC(((m68901_base[0x11]>>8) & 0xff)* 200/M68901_CLK),0,timerc_int);
        m68901_int_gen(TMRC);           // Fire an interrupt.
}

static void timerd_int(int param)
{
        timer_set(TIME_IN_USEC(250),0,timerd_int);
        m68901_int_gen(TMRD);           // Fire an interrupt.
}


/* Called by anything that generates a MFD interrupt */
/* Requires: MFD line number */

void m68901_int_gen(int source)
{
// logerror("M68901 interrupt %d requested.\n",source);

int IEN_REG=3+(int)(source / 8);
int IPEND_REG=5+(int)(source / 8);       // Makes things a little clearer.
int IMASK_REG=9+(int)(source / 8);
int bit=1 << (source-8*(int)(source/8));

       if(m68901_base[IEN_REG] & (bit<<8))                // If interrupt is enabled by MFD, set interrupt pending bit.
       {
              m68901_base[IPEND_REG] = m68901_base[IPEND_REG] | (bit<<8);
            //  logerror("M68901 interrupt %d now pending (%d) \n",source, IPEND_REG);
       }

       if(m68901_base[IMASK_REG] & (bit<<8))              // If interrupt is not masked by MFD, trigger a 68k INT
       {
                host_irq(4);
            //    logerror("M68901 interrupt %d serviced.\n",source);
       }
}

/*************************************
 *
 *  Analog controls
 *
 *************************************/

/* the joystick encoders give 12 bit two's complement values centered on 0 */
static UINT16 joystick_r(const char *tag)
{
	return (readinputportbytag_safe(tag, 0xa6) - 0xa6) & 0xfff;
}

static READ16_HANDLER( encoder_h_r )
{
	UINT16 x_encoder = joystick_r("JOYSTICK_X");
	UINT16 y_encoder = joystick_r("JOYSTICK_Y");

	return (y_encoder & 0xf00) | ((x_encoder & 0xf00) >> 8);
}

static READ16_HANDLER( encoder_l_r )
{
	UINT16 x_encoder = joystick_r("JOYSTICK_X");
	UINT16 y_encoder = joystick_r("JOYSTICK_Y");

	return ((y_encoder & 0xff) << 8) | (x_encoder & 0xff);
}

/* ADC0844: channel 1 is the throttle, channel 2 the volume control */
static UINT8 adc_channel;
static UINT8 adc_result;

static UINT8 adc_clamp(int value)
{
	if (value < 0)
		return 0;
	if (value > 0xff)
		return 0xff;
	return value;
}

static int adc_input(int ch)
{
	switch (ch)
	{
		case 1: return readinputportbytag_safe("THROTTLE", 0);
		case 2: return 0xff;
	}
	return 0;
}

static void adc_done_callback(int param)
{
	switch (adc_channel)
	{
		/* differential */
		case 0x00:
		case 0x08: adc_result = adc_clamp(0xff - (adc_input(2) - adc_input(1))); break;
		case 0x01:
		case 0x09: adc_result = adc_clamp(0xff - (adc_input(1) - adc_input(2))); break;
		case 0x02:
		case 0x0a: adc_result = adc_clamp(0xff - (adc_input(4) - adc_input(3))); break;
		case 0x03:
		case 0x0b: adc_result = adc_clamp(0xff - (adc_input(3) - adc_input(4))); break;
		/* single-ended */
		case 0x04: adc_result = adc_input(1); break;
		case 0x05: adc_result = adc_input(2); break;
		case 0x06: adc_result = adc_input(3); break;
		case 0x07: adc_result = adc_input(4); break;
		/* pseudo-differential */
		case 0x0c: adc_result = adc_clamp(0xff - (adc_input(4) - adc_input(1))); break;
		case 0x0d: adc_result = adc_clamp(0xff - (adc_input(4) - adc_input(2))); break;
		case 0x0e: adc_result = adc_clamp(0xff - (adc_input(4) - adc_input(3))); break;
		/* undefined */
		case 0x0f: adc_result = 0x00; break;
	}

	/* the conversion end is wired to the MFP general purpose input 3 */
	m68901_int_gen(GPIP3);
}

static READ16_HANDLER( adc_r )
{
	return adc_result;
}

static WRITE16_HANDLER( adc_w )
{
	if (ACCESSING_LSB)
	{
		adc_channel = data & 0x0f;
		timer_adjust(adc_timer, TIME_IN_USEC(40), 0, 0);
	}
}



static WRITE16_HANDLER( m68901_w )
{

UINT8 value = (data>>8)& 0xff;
m68901_base[offset]=data;

switch(offset)
{

        case 0x0c:    logerror("Timer A Control: %x\n",value);
                      break;
        case 0x0d:    logerror("Timer B Control: %x\n",value);
                      break;
        case 0x0e:    logerror("Timer C/D Control: %x\n",value);
                      break;

        case 0x0f:    logerror("Timer A Data:%4x\n",value);                                                       // Timer A Data Register
                     timer_set(TIME_IN_USEC(1000),0,timera_int);
                      break;

        case 0x10:    logerror("Timer B Data:%4x\n",value);                                                           // Timer B Data Register
//                    timer_set(TIME_IN_SEC(value * 200/M68901_CLK),0,timerb_int);
                      break;

        case 0x11:    logerror("Timer C Data:%4x\n",value);                                                        // Timer C Data Register
//                      timer_set(TIME_IN_SEC(value * 200/M68901_CLK),0,timerc_int);
                      break;

        case 0x12:    logerror("Timer D Data:%4x\n",value);
                      timer_set(TIME_IN_USEC(500),0,timerd_int);                 // Timer D Data Register
                      break;

}

}



/* I should really re-write all this. */

/*
 * 68681 output port: the pins are the complement of OPR.
 * 5: /8031 reset
 * 7: status LED
 */
static void duart_output_update(void)
{
	UINT8 pins = ~M68681.OPR;

	set_led_status(1, !(pins & 0x80));
	cpunum_set_input_line(2, INPUT_LINE_RESET, (pins & 0x20) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE16_HANDLER( m68681_w )
{

UINT8 value = (data>>8)& 0xff;

switch(offset)
{
      case 0x00:        break;                                // Mode Register A

      case 0x01:        M68681.CSRA = value;
                        break;
                                                              // Clock-select register A
      case 0x02:        M68681.CRA = value;
                        break;                                // Command Register A

      case 0x03:        M68681.TBA = value;                   // Fill transmit buffer
                        M68681.SRA |=0x0400;                  // Data has been sent - TX ready for more.
                        duart_update_irq();

#if HOST_MONITOR_DISPLAY
                        printf("%c",value);                    // Port A - Monitor
#endif
                        break;

      case 0x04:        break;

      case 0x05:        //printf("IMR=%x\n",M68681.IMR);
                        M68681.IMR = value;                    // Interrupt Mask Reg
                        duart_update_irq();
                        break;

      case 0x06:        break;

      case 0x07:        break;

      case 0x08:        break;

      case 0x09:        break;

      case 0x0a:        /* Command Register B: receiver enable/disable */
                        if ((value & 0x03) == 0x01)
                              sound_rx_enabled = 1;
                        else if ((value & 0x03) == 0x02)
                              sound_rx_enabled = 0;
                        break;

      case 0x0b:        sound_tx_w(value);                    //  Fill transmit buffer
                        M68681.SRB |=0x0400;                   // Data has been sent - TX ready for more.
                        // Write to sound board
                        duart_update_irq();
                        break;

      case 0x0c:        //printf("IVR: %d",value);
                        M68681.IVR = value;                   // Interrupt vector register.
                        break;

      case 0x0d:        break;
      case 0x0e:        /* set output port bits: the pins go low */
                        M68681.OPR |= value;
                        duart_output_update();
                        break;

      case 0x0f:        /* reset output port bits: the pins go high */
                        M68681.OPR &= ~value;
                        duart_output_update();
                        break;

      default:          m68681_base[offset]=data;
}
}

static READ16_HANDLER( m68681_r )
{

switch(offset)
{
//      case 0x00:
        case 0x01:     return M68681.SRA;                              // Status Register A
//      case 0x02:     return

        case 0x03:     M68681.SRA&=~0x100;
                       return M68681.RBA;                              // RX A - Monitor Port

        case 0x05:     return duart_isr() << 8;                        // Interrupt Status Register

        case 0x09:     return M68681.SRB;                              // Status Register B

        case 0x0b:     M68681.SRB&=~0x0100;                            // No longer have data.
                       return M68681.RBB;
                                                                       // RX B - Monitor Port
}

return 0;
}


static WRITE16_HANDLER( ti_uart_w )
{
// Need to initalise values to 0 on reset.
switch(offset)
{
        case 0x0: ti_uart[TX] = data;                  // Pointless really - nothing happens to it.
#if VGB_MONITOR_DISPLAY
                  printf("%c",data);

#endif
                  ti_uart[STATUS]|=1;                  // Transmit regiser empty.
                  break;

        case 0x1: if(!ti_uart_mode_cycle)
                  {
                        ti_uart[MODE1] = data;
                        ti_uart_mode_cycle=1;
                  }
                  else
                  {
                        ti_uart[MODE2] = data;
                        ti_uart_mode_cycle=0;
                  }
                  break;

        case 0x2: if(ti_uart_sync_cycle==0)
                            {
                                  ti_uart[SYN1] = data;
                                  ti_uart_mode_cycle=1;
                            }
                            else if(ti_uart_sync_cycle==1)
                            {
                                  ti_uart[SYN2] = data;
                                  ti_uart_mode_cycle=2;
                            }
                            else
                            {
                                 ti_uart[DLE] = data;
                                 ti_uart_mode_cycle=0;
                            }
                            break;

        case 0x3: ti_uart[COMMAND] = data;
                  ti_uart_mode_cycle = ti_uart_sync_cycle=0;
                  break;
}

}


static READ16_HANDLER( ti_uart_r )
{

switch(offset)
{
        case 0x0: ti_uart[STATUS]^=2;
                  return ti_uart[RX];

        case 0x1: if(!ti_uart_mode_cycle)
                            {
                                  ti_uart_mode_cycle=1;
                                  return ti_uart[MODE1];
                            }
                            else
                            {
                                  ti_uart_mode_cycle=0;
                                  return ti_uart[MODE2];
                            }
        case 0x2: return ti_uart[STATUS];

        case 0x3: ti_uart_mode_cycle = ti_uart_sync_cycle=0;
                  return ti_uart[COMMAND];

        default:  logerror("Unknown TI UART access.\n");
                  return 0;
}

}

/* Zilog 8530 SCC of the Dr. Math monitor port. Only the status the
   monitor polls is modelled: transmitter always ready, nothing received
   and no carrier detect, as no monitor is connected. */
static UINT8 am_uart_ptr[2];

static WRITE32_HANDLER( am_uart_w )
{
       int ch = (offset >> 1) & 1;

       if (offset & 1)
       {
              /* monitor output is not shown */
              return;
       }

       if (am_uart_ptr[ch] == 0)
              am_uart_ptr[ch] = data & 7;
       else
              am_uart_ptr[ch] = 0;
}

static READ32_HANDLER( am_uart_r )
{
       int ch = (offset >> 1) & 1;
       UINT8 reg = am_uart_ptr[ch];

       if (offset & 1)
              return 0x00;

       am_uart_ptr[ch] = 0;

       switch (reg)
       {
              case 0:  return 0x04;   /* transmit buffer empty */
              case 1:  return 0x01;   /* all sent */
              default: return 0x00;
       }
}


static WRITE16_HANDLER( mystery_w )
{
       ui_popup("Write to 900000: %x",data);
}

static WRITE16_HANDLER( mystery2_w )
{
       ui_popup("TMS Write to e00000: %x",data);
}

static WRITE16_HANDLER( mystery3_w )
{
       ui_popup("TMS Write to 2600000: %x",data);
}


static READ16_HANDLER( tms_host_r )
{
	return tms34010_host_r(1, offset);
}

static WRITE16_HANDLER( tms_host_w )
{
	tms34010_host_w(1, offset, data);
}

static WRITE16_HANDLER( reset_slave )
{
        if(data & 0x0020)
        {
//          cpu_set_input_line(1, CLEAR_LINE);
                logerror("TMS Running\n");
	}
	else
	{
//                cpu_set_input_line(1, ASSERT_LINE);
        	logerror("TMS Reset\n");
        }
}



/* The DS1215 phantom timer IC is mapped to the lower 32kB SRAM address space */
/* Serial Read/write operations are conducted via the 0th data bit */

/* Reset peripherals flip-flop MIGHT be 900000 */

static ADDRESS_MAP_START( hostmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x13ffff) AM_ROM                           /* 68000 Code ROM and Dr. Math object data */
	AM_RANGE(0x200000, 0x20ffff) AM_RAM                           /* Battery-backed SRAM (64kB) & DS1215 */
	AM_RANGE(0x800000, 0x83ffff) AM_RAM AM_BASE(&shared_ram)      /* 68000/AM29000 shared RAM (256kB) */
	AM_RANGE(0x900000, 0x900001) AM_WRITE(host_drmath_int_w)      /* 68000 to Am29000 interrupt */
	AM_RANGE(0x920000, 0x920001) AM_READ(input_port_1_word_r)     /* User inputs C and D */
	AM_RANGE(0x940000, 0x940001) AM_READ(input_port_0_word_r)     /* User inputs A and B */
	AM_RANGE(0x960000, 0x960001) AM_WRITE(reset_w)                 /* Am29000 and TMS34010 reset */
	AM_RANGE(0x980000, 0x980001) AM_READWRITE(adc_r, adc_w)        /* ADC0844 */
	AM_RANGE(0x9a0000, 0x9a0007) AM_READWRITE(tms_host_r, tms_host_w)  /* TMS34010 Interface */
	AM_RANGE(0x9c0000, 0x9c0001) AM_NOP                            /* Lamps */
	AM_RANGE(0x9e0000, 0x9e00cf) AM_READWRITE(MRA16_RAM, m68901_w) AM_BASE(&m68901_base)  /* 68901 Multifunction Peripheral */
	AM_RANGE(0xa00000, 0xa000cf) AM_READWRITE(m68681_r, m68681_w) AM_BASE(&m68681_base)   /* 68681 UART */
	AM_RANGE(0xa20000, 0xa20001) AM_READ(encoder_h_r)             /* XY joystick input, high bits */
	AM_RANGE(0xa40002, 0xa40003) AM_READ(encoder_l_r)             /* XY joystick input, low bits */
ADDRESS_MAP_END


// 2600000: Write 1E and 9E?
static ADDRESS_MAP_START( vgbmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM AM_BASE(&micro3d_sprite_vram)  // 2 Banks
	AM_RANGE(0x00800000, 0x00bfffff) AM_RAM                         /* 512kB Main DRAM */
	AM_RANGE(0x00c00000, 0x00c0000f) AM_READ(input_port_2_word_r)   /* TI Monitor Mode switch */
	AM_RANGE(0x00e00000, 0x00e0000f) AM_WRITE(micro3d_xfer3dk_w)  /* XFER3DK */
	AM_RANGE(0x02000000, 0x0200ffff) AM_READWRITE(MRA16_RAM, paletteram16_BBBBBRRRRRGGGGGG_word_w) AM_BASE(&paletteram16) //      AM_RANGE(0x02010000, 0x027fffff) AM_RAM                         // ??????????? Mirror of VRAM???
	AM_RANGE(0x02600000, 0x0260000f) AM_WRITE(micro3d_creg_w)       /* CREG */
	AM_RANGE(0x02c00000, 0x02c0003f) AM_READ(ti_uart_r)            /* SCN UART */
	AM_RANGE(0x02e00000, 0x02e0003f) AM_WRITE(ti_uart_w)
	AM_RANGE(0x03800000, 0x03dfffff) AM_ROM AM_REGION(REGION_GFX1, 0)                          /* 2D Graphics ROMs */
	AM_RANGE(0x03e00000, 0x03ffffff) AM_ROM AM_REGION(REGION_USER1, 0)                         /* 128kB Program ROM */
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, micro3d_34010_io_register_w)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)                         /* 128kB Program ROM (Mirror - for interrupt vectors) */
ADDRESS_MAP_END


static ADDRESS_MAP_START( drmath_prg, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_ROM AM_REGION(REGION_USER2, 0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( drmath_data, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0x00400000, 0x004fffff) AM_RAM
	AM_RANGE(0x00500000, 0x005fffff) AM_RAM
	AM_RANGE(0x00800000, 0x0083ffff) AM_READWRITE(shared_r, shared_w)
	AM_RANGE(0x00a00000, 0x00a00003) AM_WRITE(drmath_int_w)
	AM_RANGE(0x01000000, 0x01000003) AM_WRITE(mac1_w)
	AM_RANGE(0x01000004, 0x01000007) AM_READWRITE(mac2_r, mac2_w)
	AM_RANGE(0x01200000, 0x01203fff) AM_RAM AM_BASE(&mac_sram)
	AM_RANGE(0x01400000, 0x01400003) AM_READWRITE(micro3d_pipe_r, micro3d_fifo_w)
	AM_RANGE(0x01600000, 0x01600003) AM_WRITE(drmath_intr2_ack)
	AM_RANGE(0x01800000, 0x01800003) AM_WRITE(micro3d_alt_fifo_w)
	AM_RANGE(0x03fffff0, 0x03ffffff) AM_READWRITE(am_uart_r, am_uart_w)
ADDRESS_MAP_END


/*====================================================================*/
/* The 8031 ports are mapped as follows:                              */
/*                                                                    */
/* P1.0 = SHMUX-A  O         P3.0                                     */
/* P1.1 = SHMUX-B  O         P3.1                                     */
/* P1.2 = SHMUX-C  O         P3.2 = SPCHBANK O                        */
/* P1.3 = SHMUX-E  O         P3.3 = SP-BUSY- I                        */
/* P1.4 = VOLDQ    O         P3.4 = RESET    O                        */
/* P1.5 = VOLCK    O         P3.5 = WDO      O                        */
/* P1.6 = VOLRST   O         P3.6                                     */
/* P1.7 = SELFTEST I         P3.7                                     */
/*====================================================================*/

static WRITE8_HANDLER( micro3d_upd7759_w )
{
	upd7759_0_port_w(0, data);
	upd7759_0_start_w(0, 0);
	upd7759_0_start_w(0, 1);
}

static UINT8 sound_port_latch[4];

static WRITE8_HANDLER(sound_io_w)
{
	switch (offset)
	{
		case 0x01:
			sound_port_latch[1] = data;
			micro3d_noise_sh_w((data >> 2) & 1, data);
			break;

		case 0x03:
			sound_port_latch[3] = data;
			upd7759_set_bank_base(0, (data & 0x04) ? 0x20000 : 0);
			upd7759_0_reset_w(0, (data & 0x10) ? 0 : 1);
			break;
	}
}

static READ8_HANDLER(sound_io_r)
{
	switch (offset)
	{
		case 0x01:  return (sound_port_latch[1] & 0x7f) | (readinputport(3) & 0x80);   /* Test push switch */
		case 0x03:  return (sound_port_latch[3] & 0xf7) | (upd7759_0_busy_r(0) ? 0x08 : 0);
		default:    return 0;
	}
}

static WRITE8_HANDLER( snd_dac_a )
{
	micro3d_noise_dac_w(data);
}

static WRITE8_HANDLER( snd_dac_b )
{
	/* TODO: This controls upd7759 volume */
}

static struct CustomSound_interface noise_interface =
{
	micro3d_noise_start,
	NULL,
	micro3d_noise_reset
};

static ADDRESS_MAP_START( soundmem_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM                    // 32kB Code EPROM
ADDRESS_MAP_END


/* FFXX - 00XX    */

static ADDRESS_MAP_START( soundmem_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM                                  /* 2Kb RAM */
	AM_RANGE(0xfd00, 0xfd00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xfd01, 0xfd01) AM_READWRITE(YM2151_status_port_0_r,YM2151_data_port_0_w)
	AM_RANGE(0xfe00, 0xfe00) AM_WRITE(micro3d_upd7759_w)
	AM_RANGE(0xff00, 0xff00) AM_WRITE(snd_dac_a)
	AM_RANGE(0xff01, 0xff01) AM_WRITE(snd_dac_b)
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundmem_io, ADDRESS_SPACE_IO, 8 )
         AM_RANGE(0x00, 0xff) AM_READWRITE(sound_io_r,sound_io_w)
ADDRESS_MAP_END



static struct upd7759_interface upd7759_interface =
{
	REGION_SOUND1
};

static struct tms34010_config vgb_config =
{
	0,					/* halt on reset ????? - check this */
	tms_interrupt,				/* Generate interrupt */
	NULL,
	NULL,
};


static MACHINE_DRIVER_START( micro3d )
	MDRV_CPU_ADD(M68000, 32000000/2)
	MDRV_CPU_PROGRAM_MAP(hostmem,0)
	MDRV_CPU_VBLANK_INT(micro3d_vblank,1)

 	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(vgb_config)
	MDRV_CPU_PROGRAM_MAP(vgbmem,0)
	MDRV_CPU_VBLANK_INT(micro3d_tms_vblank,1)

	MDRV_CPU_ADD(I8051, 11059000)
	MDRV_CPU_PROGRAM_MAP(soundmem_prg,0)
	MDRV_CPU_DATA_MAP(soundmem_data,0)
	MDRV_CPU_IO_MAP(soundmem_io,0)

	MDRV_CPU_ADD(AM29000, 32000000/2)
	MDRV_CPU_PROGRAM_MAP(drmath_prg,0)
	MDRV_CPU_DATA_MAP(drmath_data,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(micro3d)
	MDRV_INTERLEAVE(50)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 575, 0, 399)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(micro3d)
	MDRV_VIDEO_UPDATE(micro3d)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd7759_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.35)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.35)

	MDRV_SOUND_ADD(YM2151, 3579545)
	MDRV_SOUND_ROUTE(0, "left", 0.35)
	MDRV_SOUND_ROUTE(1, "right", 0.35)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(noise_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(noise_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END


ROM_START( f15se )
	// Host PCB (MPG DW-00011C-0011-01)
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) // 68000 code
	ROM_LOAD16_BYTE( "110-00001-600.u67", 0x000001, 0x20000, CRC(da507ec7) SHA1(24fb80aa7157a97b53a6efa8f2477aacd0cd621f) )
	ROM_LOAD16_BYTE( "110-00001-601.u91", 0x000000, 0x20000, CRC(f47648c6) SHA1(a5eac6fcde52ebf750cfcdcf151fcbd859146296) )
	ROM_LOAD16_BYTE( "110-00001-602.u68", 0x040001, 0x20000, CRC(099ad6ba) SHA1(cb51b4d9eeb7dbc3c967d1c224957f2e70cd11f9) )
	ROM_LOAD16_BYTE( "110-00001-603.u92", 0x040000, 0x20000, CRC(71a4cb4d) SHA1(5b47046bd1c551b041f20d0a8f58ccf68ebf2409) )
	ROM_LOAD16_BYTE( "110-00001-004.u69", 0x080001, 0x20000, CRC(81671ce1) SHA1(51ff641ccbc9dea640a62944910abe73d796b062) )
	ROM_LOAD16_BYTE( "110-00001-005.u93", 0x080000, 0x20000, CRC(bdaa7db5) SHA1(52cd832cdd44e609e8cd269469b806e2cd27d63d) )
	ROM_LOAD16_BYTE( "110-00001-006.u70", 0x0c0001, 0x20000, CRC(251e92d2) SHA1(a20279089af1f738ba912f90a4d048d4e58795fe) )
	ROM_LOAD16_BYTE( "110-00001-007.u94", 0x0c0000, 0x20000, CRC(36e06cba) SHA1(5ffee5da6f475978be10fa5e1a2c24f00497ea5f) )
	ROM_LOAD16_BYTE( "110-00001-008.u71", 0x100001, 0x20000, CRC(d96fd4e2) SHA1(001af758da437e955b4ee914eabeb9739ebc4454) )
	ROM_LOAD16_BYTE( "110-00001-009.u95", 0x100000, 0x20000, CRC(33e3b473) SHA1(66deda79ba94f0ed722b399b3fc6062dcdd1a6c9) )

	// Dr Math PCB (MPG 010-00002-001)
	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD64_BYTE( "110-00001-222.u134", 0x00000, 0x08000, CRC(1aa391a4) SHA1(8bf80c0d0d9a85cbec9d8e18f6b5fc83989c86ce) )
	ROM_LOAD64_BYTE( "110-00001-225.u126", 0x00001, 0x08000, CRC(bf6eb5a1) SHA1(94320581917ce36ae50ea0ecb53af9e12999e2b1) )
	ROM_LOAD64_BYTE( "110-00001-223.u114", 0x00002, 0x08000, CRC(25f6fdf3) SHA1(664b55e69b1e251241defff465fff4a1b1b5f5e3) )
	ROM_LOAD64_BYTE( "110-00001-224.u107", 0x00003, 0x08000, CRC(dea6b1c9) SHA1(e64c31b49a58c396e38a5f7818dd734ab66f4e80) )
	ROM_LOAD64_BYTE( "110-00001-218.u135", 0x00004, 0x08000, CRC(d868c4cb) SHA1(0b4f6ad13592d1ea17104002ef0a45aacc61749b) )
	ROM_LOAD64_BYTE( "110-00001-221.u127", 0x00005, 0x08000, CRC(a04b638b) SHA1(6681e2cdf256af18c0829057afcbe9762329bdd0) )
	ROM_LOAD64_BYTE( "110-00001-219.u115", 0x00006, 0x08000, CRC(7b4ed618) SHA1(57751bd08f0a400347cfcd39726754a2c79b43e6) )
	ROM_LOAD64_BYTE( "110-00001-220.u108", 0x00007, 0x08000, CRC(00138de4) SHA1(3388fb2d504d41c4e504ccb3aafe5fae62d4f6e2) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "110-00001-014.u153", 0x00001, 0x20000, CRC(5ca7713f) SHA1(ac7b9629684b99ecfb1945176b06eb6be284ba93) )
	ROM_LOAD16_BYTE( "110-00001-015.u154", 0x00000, 0x20000, CRC(beae31bb) SHA1(1ab80a6b99eea6d5bf9b1bce58ecca13042c77a6) )
	ROM_LOAD16_BYTE( "110-00001-016.u167", 0x40001, 0x20000, CRC(5db4f677) SHA1(25a6fe4c562e4fa4225aa4687dd41920b614e591) )
	ROM_LOAD16_BYTE( "110-00001-017.u160", 0x40000, 0x20000, CRC(47f9a868) SHA1(7c8a9355893e4a3f3846fd05e0237ffd1404ffee) )

	// Video Graphics PCB (MPG DW-010-00003-001)
	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "110-00002-101.u101", 0x00000, 0x20000, CRC(e99fac71) SHA1(98d1d2134fabc1bad637cbe42cbe9cdc20b32126) )
	ROM_LOAD16_BYTE( "110-00002-104.u97",  0x00001, 0x20000, CRC(78b9b7c7) SHA1(4bce993dd3aea126e3a9d42ee8c68b8ab47fdba7) )

	ROM_REGION16_LE( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "110-00002-105.u124", 0x00000, 0x20000, CRC(7b1852f0) SHA1(d21525e59b3112313ea9783ac3dd988a4c1d5f87) )
	ROM_LOAD16_BYTE( "110-00002-106.u121", 0x00001, 0x20000, CRC(9d031636) SHA1(b7c7b57d547f2ce2eeb97126e961f3b5f35823f7) )
	ROM_LOAD16_BYTE( "110-00002-107.u130", 0x40000, 0x20000, CRC(15326070) SHA1(ec4484d4515694742d3fd3b944f342f052463988) )
	ROM_LOAD16_BYTE( "110-00002-108.u133", 0x40001, 0x20000, CRC(ca0e86d8) SHA1(a7b4b02d100a7875d5a184cdb76d507e926d1ca3) )
	ROM_LOAD16_BYTE( "110-00002-103.u114", 0x80000, 0x20000, CRC(4d8e8f54) SHA1(d8a23b5fd00ab919dc6d63fc72824d1293073813) )
	ROM_LOAD16_BYTE( "110-00002-102.u108", 0x80001, 0x20000, CRC(f6488e31) SHA1(d2f9304cc59f5523007592ae76ddd56107cc29e8) )

	// Sound PCB (MPG 010-00018-002)
	ROM_REGION( 0x08000, REGION_CPU3, 0 )
	ROM_LOAD( "110-00004-001.u2", 0x000000, 0x08000, CRC(705685a9) SHA1(311f7cac126a19e8bd555ebf31ff4ec4680ddfa4) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "110-00003-001.u17", 0x000000, 0x40000, CRC(af84b635) SHA1(844e5987a66e9e3ab2d2fe05b93a4da3512776bb) )
ROM_END


ROM_START( f15se21 )
	// Host PCB (MPG DW-00011C-0011-01)
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) // 68000 code
	ROM_LOAD16_BYTE( "110-00001-500.u67", 0x000001, 0x20000, CRC(6c26806d) SHA1(7cfd2b3b92b0fc6627c92a2013a317ca5abc66a0) )
	ROM_LOAD16_BYTE( "110-00001-501.u91", 0x000000, 0x20000, CRC(81f02bf7) SHA1(09976746fe4d9c88bd8840f6e7addb09226aa54b) )
	ROM_LOAD16_BYTE( "110-00001-502.u68", 0x040001, 0x20000, CRC(1eb945e5) SHA1(aba3ff038f2ca0f1200be5710073825ce80e3656) )
	ROM_LOAD16_BYTE( "110-00001-503.u92", 0x040000, 0x20000, CRC(21fcb974) SHA1(56f78ce652e2bf432fbba8cda8c800f02dad84bb) )
	ROM_LOAD16_BYTE( "110-00001-004.u69", 0x080001, 0x20000, CRC(81671ce1) SHA1(51ff641ccbc9dea640a62944910abe73d796b062) )
	ROM_LOAD16_BYTE( "110-00001-005.u93", 0x080000, 0x20000, CRC(bdaa7db5) SHA1(52cd832cdd44e609e8cd269469b806e2cd27d63d) )
	ROM_LOAD16_BYTE( "110-00001-006.u70", 0x0c0001, 0x20000, CRC(251e92d2) SHA1(a20279089af1f738ba912f90a4d048d4e58795fe) )
	ROM_LOAD16_BYTE( "110-00001-007.u94", 0x0c0000, 0x20000, CRC(36e06cba) SHA1(5ffee5da6f475978be10fa5e1a2c24f00497ea5f) )
	ROM_LOAD16_BYTE( "110-00001-008.u71", 0x100001, 0x20000, CRC(d96fd4e2) SHA1(001af758da437e955b4ee914eabeb9739ebc4454) )
	ROM_LOAD16_BYTE( "110-00001-009.u95", 0x100000, 0x20000, CRC(33e3b473) SHA1(66deda79ba94f0ed722b399b3fc6062dcdd1a6c9) )

	// Video Graphics PCB (MPG DW-010-00003-001)
	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "001.vgb", 0x000000, 0x20000, CRC(810c142d) SHA1(d37e5ecd716dda65d43cec7bca524c59d3dc9803) )
	ROM_LOAD16_BYTE( "004.vgb", 0x000001, 0x20000, CRC(b69e1260) SHA1(1a2b69ea7c96b0293b24d87ea46bd4b1d4c56a66) )

	ROM_REGION16_LE( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "110-00002-105.u124", 0x00000, 0x20000, CRC(7b1852f0) SHA1(d21525e59b3112313ea9783ac3dd988a4c1d5f87) )
	ROM_LOAD16_BYTE( "110-00002-106.u121", 0x00001, 0x20000, CRC(9d031636) SHA1(b7c7b57d547f2ce2eeb97126e961f3b5f35823f7) )
	ROM_LOAD16_BYTE( "110-00002-107.u130", 0x40000, 0x20000, CRC(15326070) SHA1(ec4484d4515694742d3fd3b944f342f052463988) )
	ROM_LOAD16_BYTE( "110-00002-108.u133", 0x40001, 0x20000, CRC(ca0e86d8) SHA1(a7b4b02d100a7875d5a184cdb76d507e926d1ca3) )
	ROM_LOAD16_BYTE( "110-00002-103.u114", 0x80000, 0x20000, CRC(4d8e8f54) SHA1(d8a23b5fd00ab919dc6d63fc72824d1293073813) )
	ROM_LOAD16_BYTE( "110-00002-102.u108", 0x80001, 0x20000, CRC(f6488e31) SHA1(d2f9304cc59f5523007592ae76ddd56107cc29e8) )

	// Dr Math PCB (MPG 010-00002-001)
	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD64_BYTE( "122.dth", 0x00000, 0x08000, CRC(9d2032cf) SHA1(8430816756ea92bbe86b94eaa24a6071bf0ef879) )
	ROM_LOAD64_BYTE( "125.dth", 0x00001, 0x08000, CRC(7718487c) SHA1(609106f55601f84095b64ce2484107779da89149) )
	ROM_LOAD64_BYTE( "123.dth", 0x00002, 0x08000, CRC(54d5544f) SHA1(d039ee39991b947a7483111359ab245fc104e060) )
	ROM_LOAD64_BYTE( "124.dth", 0x00003, 0x08000, CRC(7be96646) SHA1(a6733f75c0404282d71e8c1a287546ef4d9d42ad) )
	ROM_LOAD64_BYTE( "118.dth", 0x00004, 0x08000, CRC(cc895c20) SHA1(140ef47536914fe1441778e759894c2cdd893276) )
	ROM_LOAD64_BYTE( "121.dth", 0x00005, 0x08000, CRC(392e5c43) SHA1(455cf3bb3c16217e58d6eea51d8f49a5bed1955e) )
	ROM_LOAD64_BYTE( "119.dth", 0x00006, 0x08000, CRC(b1c966e5) SHA1(9703bb1f9bdf6a779b59daebb39df2926727fa76) )
	ROM_LOAD64_BYTE( "120.dth", 0x00007, 0x08000, CRC(5fb9836d) SHA1(d511aa9f02972a7f475c82c6f57d1f3fd4f118fa) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "110-00001-014.u153", 0x00001, 0x20000, CRC(5ca7713f) SHA1(ac7b9629684b99ecfb1945176b06eb6be284ba93) )
	ROM_LOAD16_BYTE( "110-00001-015.u154", 0x00000, 0x20000, CRC(beae31bb) SHA1(1ab80a6b99eea6d5bf9b1bce58ecca13042c77a6) )
	ROM_LOAD16_BYTE( "110-00001-016.u167", 0x40001, 0x20000, CRC(5db4f677) SHA1(25a6fe4c562e4fa4225aa4687dd41920b614e591) )
	ROM_LOAD16_BYTE( "110-00001-017.u160", 0x40000, 0x20000, CRC(47f9a868) SHA1(7c8a9355893e4a3f3846fd05e0237ffd1404ffee) )

	// Sound PCB (MPG 010-00018-002)
	ROM_REGION( 0x08000, REGION_CPU3, 0 )
	ROM_LOAD( "110-00004-001.u2", 0x000000, 0x08000, CRC(705685a9) SHA1(311f7cac126a19e8bd555ebf31ff4ec4680ddfa4) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "110-00003-001.u17", 0x000000, 0x40000, CRC(af84b635) SHA1(844e5987a66e9e3ab2d2fe05b93a4da3512776bb) )
ROM_END

ROM_START( botss )
	// Host PCB (MPG DW-00011C-0011-01)
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) // 68000 code
	ROM_LOAD16_BYTE( "110-00153-100.u67", 0x000001, 0x20000, CRC(338aa9c3) SHA1(3d10329a5df80ab1761fd3953eb3872a72f26bef) )
	ROM_LOAD16_BYTE( "110-00153-101.u91", 0x000000, 0x20000, CRC(3278279e) SHA1(570935988c776283cdcd5aa13d71a75f0a466099) )
	ROM_LOAD16_BYTE( "110-00153-102.u68", 0x040001, 0x20000, CRC(258d2687) SHA1(5b765d14a0a8dc4ef58453bbb7780068c168d268) )
	ROM_LOAD16_BYTE( "110-00153-103.u92", 0x040000, 0x20000, CRC(1dde7ffa) SHA1(5363af00c8c896eccfad38ace5dfeb3bf96a73a1) )
	ROM_LOAD16_BYTE( "110-00013-004.u69", 0x080001, 0x20000, CRC(72a607ca) SHA1(1afc85380be12c429808c48f1502736a4c8b98e5) )
	ROM_LOAD16_BYTE( "110-00013-005.u93", 0x080000, 0x20000, CRC(f37680ae) SHA1(51f1ee805b7d1b2b078c612c572e12846de623b9) )
	ROM_LOAD16_BYTE( "110-00013-006.u70", 0x0c0001, 0x20000, CRC(57a1c728) SHA1(2bdc831be739ada0f4f4adec7974da453878db0e) )
	ROM_LOAD16_BYTE( "110-00013-007.u94", 0x0c0000, 0x20000, CRC(4c9e16af) SHA1(1f8acc9bb85fe1bf459b4358b9bf9cf9847e6a36) )
	ROM_LOAD16_BYTE( "110-00013-008.u71", 0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "110-00013-009.u95", 0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )

	// Dr Math PCB (MPG 010-00002-001)
	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD64_BYTE( "110-00013-122.u134", 0x000000, 0x08000, CRC(bf60c487) SHA1(5ce80e89d9a24b627b0e97bf36a4e71c2eff4324) )
	ROM_LOAD64_BYTE( "110-00013-125.u126", 0x000001, 0x08000, CRC(b0dccf4a) SHA1(e8bfd622c006985b724cdbd3ad14c33e9ed27c6c) )
	ROM_LOAD64_BYTE( "110-00013-123.u114", 0x000002, 0x08000, CRC(04ba6ed1) SHA1(012be71c6b955beda2bd0ff376dcaab51b226723) )
	ROM_LOAD64_BYTE( "110-00013-224.u107", 0x000003, 0x08000, CRC(220db5d3) SHA1(3bfbe0eb97282c4ce449fd44e8e141de74f08eb0) )
	ROM_LOAD64_BYTE( "110-00013-118.u135", 0x000004, 0x08000, CRC(2903e682) SHA1(027ed6524e9d4490632f10aeb22150c2fbc4eec2) )
	ROM_LOAD64_BYTE( "110-00013-121.u127", 0x000005, 0x08000, CRC(198a636b) SHA1(356b8948aafb98cb5e6ee7b5ad6ea9e5998265e5) )
	ROM_LOAD64_BYTE( "110-00013-219.u115", 0x000006, 0x08000, CRC(9c9dbac1) SHA1(4c66971884190598e128684ece2e15a1c80b94ed) )
	ROM_LOAD64_BYTE( "110-00013-120.u108", 0x000007, 0x08000, CRC(dafa173a) SHA1(a19980b92a5e74ebe395be36313701fdb527a46a) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "110-00013-014.u153", 0x00001, 0x20000, CRC(0eee0557) SHA1(8abe52cad31e59cf814fd9f64f4e42ddb4aa8c93) )
	ROM_LOAD16_BYTE( "110-00013-015.u154", 0x00000, 0x20000, CRC(68564122) SHA1(410d2db74e574774b2eadd7fdf891feef5d8a93f) )
	ROM_LOAD16_BYTE( "110-00013-016.u167", 0x40001, 0x20000, CRC(60c6cb26) SHA1(0e2bf65793715e12d8fd7f87fd3336a9d00ee7e6) )
	ROM_LOAD16_BYTE( "110-00013-017.u160", 0x40000, 0x20000, CRC(d8b89379) SHA1(aa08e111c1505a4ad55b14659f8e21fd39cfcb16) )

	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "110-00023-201.u101", 0x000000, 0x20000, CRC(7dc05f7d) SHA1(4d202b229cf4690d92491311e9ff14034b19c35c) )
	ROM_LOAD16_BYTE( "110-00023-204.u97",  0x000001, 0x20000, CRC(925fd08a) SHA1(fb06413debbffcd63b018f374f25b0d8e419c739) )

	// Video Graphics PCB (MPG DW-010-00002-002)
	ROM_REGION16_LE( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "110-00023-205.u124", 0x000000, 0x20000, CRC(5482e0c4) SHA1(492afac1862f2899cd734d1e57ca978ed6a906d5) )
	ROM_LOAD16_BYTE( "110-00023-206.u121", 0x000001, 0x20000, CRC(a55e5d19) SHA1(86fbcb425103ae9fff381357339af349848fc3f2) )
	ROM_LOAD16_BYTE( "110-00023-207.u130", 0x040000, 0x20000, CRC(0d8cf60f) SHA1(d8021c6bc15beb5a0e6c86b91f8ed0389b1311a5) )
	ROM_LOAD16_BYTE( "110-00023-208.u133", 0x040001, 0x20000, CRC(a4db3137) SHA1(ef266cc17e33a2c63cda3332e266bf943e464e7f) )
	ROM_LOAD16_BYTE( "110-00023-203.u114", 0x080000, 0x20000, CRC(b1dacbb1) SHA1(323531b6919eed4a963d6aad871f1fd34203e698) )
	ROM_LOAD16_BYTE( "110-00023-202.u108", 0x080001, 0x20000, CRC(ac0d3179) SHA1(f4c67d59d913ead0f8a6d42e2ca66857ebf01602) )

	// Sound PCB (MPG 010-00018-002)
	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "110-00014-001.u2", 0x000000, 0x08000, CRC(307fcb6d) SHA1(0cf63a39ac8920be6532974311804529d7218545) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "110-00013-001.u17", 0x000000, 0x40000, CRC(015a0b17) SHA1(f229c9aa59f0e6b25b818f9513997a8685e33982) )

	ROM_REGION( 0x10000, REGION_PLDS, 0 )
	// Host
	ROM_LOAD( "120-00001-306.u77", 0x000000, 0x00310, CRC(60282a45) SHA1(8621b64fa00fa556c09d7d1566480cd442a8e655) ) // AmPAL23S8-20

	// Dr. Math
	ROM_LOAD( "mac1_u173_a.bin", 0x000000, 0x014c7, CRC(78040232) SHA1(c9adc1db76b4ee5ee08f4a11caae77993b23cc30) ) // EPS448
	ROM_LOAD( "mac2_u166_a.bin", 0x000000, 0x014c7, CRC(c85c4c66) SHA1(fab07ad0611de7d2c2af9b6fa262d574e238bd9f) ) // EPS448
ROM_END

ROM_START( botss11 )
	// Host PCB (MPG DW-00011C-0011-02)
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) // 68000 code
	ROM_LOAD16_BYTE( "110-00013-300.u67", 0x000001, 0x20000, CRC(7f74362a) SHA1(41611ba8e6eb5d6b3dfe88e1cede7d9fb5472e40) )
	ROM_LOAD16_BYTE( "110-00013-301.u91", 0x000000, 0x20000, CRC(a8100d1e) SHA1(69d3cac6f67563c0796560f7b874d7660720027d) )
	ROM_LOAD16_BYTE( "110-00013-302.u68", 0x040001, 0x20000, CRC(af865ee4) SHA1(f00bce49401431bc749208399329d9f92457186b) )
	ROM_LOAD16_BYTE( "110-00013-303.u92", 0x040000, 0x20000, CRC(15182619) SHA1(e95dcce11c0651c8e85fc0c658029f48eea35fb8) )
	ROM_LOAD16_BYTE( "110-00013-104.u69", 0x080001, 0x20000, CRC(72a607ca) SHA1(1afc85380be12c429808c48f1502736a4c8b98e5) )
	ROM_LOAD16_BYTE( "110-00013-105.u93", 0x080000, 0x20000, CRC(f37680ae) SHA1(51f1ee805b7d1b2b078c612c572e12846de623b9) )
	ROM_LOAD16_BYTE( "110-00013-106.u70", 0x0c0001, 0x20000, CRC(57a1c728) SHA1(2bdc831be739ada0f4f4adec7974da453878db0e) )
	ROM_LOAD16_BYTE( "110-00013-107.u94", 0x0c0000, 0x20000, CRC(4c9e16af) SHA1(1f8acc9bb85fe1bf459b4358b9bf9cf9847e6a36) )
	ROM_LOAD16_BYTE( "110-00013-108.u71", 0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "110-00013-109.u95", 0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )

	// Dr Math PCB (MPG 010-00002-001)
	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD64_BYTE( "110-00013-122.u134", 0x000000, 0x08000, CRC(bf60c487) SHA1(5ce80e89d9a24b627b0e97bf36a4e71c2eff4324) )
	ROM_LOAD64_BYTE( "110-00013-125.u126", 0x000001, 0x08000, CRC(b0dccf4a) SHA1(e8bfd622c006985b724cdbd3ad14c33e9ed27c6c) )
	ROM_LOAD64_BYTE( "110-00013-123.u114", 0x000002, 0x08000, CRC(04ba6ed1) SHA1(012be71c6b955beda2bd0ff376dcaab51b226723) )
	ROM_LOAD64_BYTE( "110-00013-124.u107", 0x000003, 0x08000, CRC(220db5d3) SHA1(3bfbe0eb97282c4ce449fd44e8e141de74f08eb0) )
	ROM_LOAD64_BYTE( "110-00013-018.u135", 0x000004, 0x08000, CRC(2903e682) SHA1(027ed6524e9d4490632f10aeb22150c2fbc4eec2) )
	ROM_LOAD64_BYTE( "110-00013-121.u127", 0x000005, 0x08000, CRC(198a636b) SHA1(356b8948aafb98cb5e6ee7b5ad6ea9e5998265e5) )
	ROM_LOAD64_BYTE( "110-00013-119.u115", 0x000006, 0x08000, CRC(9c9dbac1) SHA1(4c66971884190598e128684ece2e15a1c80b94ed) )
	ROM_LOAD64_BYTE( "110-00013-120.u108", 0x000007, 0x08000, CRC(dafa173a) SHA1(a19980b92a5e74ebe395be36313701fdb527a46a) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "110-00013-014.u153", 0x00001, 0x20000, CRC(0eee0557) SHA1(8abe52cad31e59cf814fd9f64f4e42ddb4aa8c93) )
	ROM_LOAD16_BYTE( "110-00013-015.u154", 0x00000, 0x20000, CRC(68564122) SHA1(410d2db74e574774b2eadd7fdf891feef5d8a93f) )
	ROM_LOAD16_BYTE( "110-00013-016.u167", 0x40001, 0x20000, CRC(60c6cb26) SHA1(0e2bf65793715e12d8fd7f87fd3336a9d00ee7e6) )
	ROM_LOAD16_BYTE( "110-00013-017.u160", 0x40000, 0x20000, CRC(d8b89379) SHA1(aa08e111c1505a4ad55b14659f8e21fd39cfcb16) )

	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "110-00023-101.u101", 0x000000, 0x20000, CRC(6aada23d) SHA1(85dbf9b20e4f17cb21922637763654d6cae80dfd) )
	ROM_LOAD16_BYTE( "110-00023-104.u97",  0x000001, 0x20000, CRC(715cac9d) SHA1(2aa0c563dc1fe4d02fa1ecbaed16f720f899fdc4) )

	// Video Graphics PCB (MPG DW-010-00002-002)
	ROM_REGION16_LE( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "110-00023-105.u124", 0x000000, 0x20000, CRC(5482e0c4) SHA1(492afac1862f2899cd734d1e57ca978ed6a906d5) )
	ROM_LOAD16_BYTE( "110-00023-106.u121", 0x000001, 0x20000, CRC(a55e5d19) SHA1(86fbcb425103ae9fff381357339af349848fc3f2) )
	ROM_LOAD16_BYTE( "110-00023-107.u130", 0x040000, 0x20000, CRC(006487b6) SHA1(f8bc6abad13df099da1708bd22f239703e407b21) )
	ROM_LOAD16_BYTE( "110-00023-108.u133", 0x040001, 0x20000, CRC(e4587ba1) SHA1(1323b4be5a526ae182ee38e96fccd263a4cecc37) )
	ROM_LOAD16_BYTE( "110-00023-103.u114", 0x080000, 0x20000, CRC(4e486e70) SHA1(04ee16cfadd43dbe9ed5bd8330c21a718d63a8f4) )
	ROM_LOAD16_BYTE( "110-00023-102.u108", 0x080001, 0x20000, CRC(441e8490) SHA1(6cfe30cea3fa297b71e881fbddad6d65a96e4386) )

	// Sound PCB (MPG 010-00018-002)
	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "110-00014-001.u2", 0x000000, 0x08000, CRC(307fcb6d) SHA1(0cf63a39ac8920be6532974311804529d7218545) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "110-00013-001.u17", 0x000000, 0x40000, CRC(015a0b17) SHA1(f229c9aa59f0e6b25b818f9513997a8685e33982) )
ROM_END

ROM_START( tankbatl )
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) // 68000 code
	ROM_LOAD16_BYTE( "lo_u67",    0x000001, 0x20000, CRC(97aabac0) SHA1(12a0719d3332a63e912161200b0a942c27c1f5da) )
	ROM_LOAD16_BYTE( "le_u91",    0x000000, 0x20000, CRC(977f90d9) SHA1(530fa5c32b1f28e2b90d20d98cc453cb290c0ad2) )
	ROM_LOAD16_BYTE( "ho_u68",    0x040001, 0x20000, CRC(8f76f4ac) SHA1(f6c1d4c933a373b153eee7d9f3016c985acaa281) )
	ROM_LOAD16_BYTE( "he_u92",    0x040000, 0x20000, CRC(1ea1db7c) SHA1(ecaa1bd3d70489a5ba0d96c6935c2959f57467b2) )
	ROM_LOAD16_BYTE( "b00_o.u69", 0x080001, 0x20000, CRC(393718e5) SHA1(f956f8bd946f53a032af16011dc69f66fb3f095c) )
	ROM_LOAD16_BYTE( "b00_e.u93", 0x080000, 0x20000, CRC(aedea0ef) SHA1(a81c3518c7a1e21f2fa2ad29c30346f727069257) )
	ROM_LOAD16_BYTE( "b01_o.u70", 0x0c0001, 0x20000, CRC(e895167d) SHA1(677cbf1be32c1f0c76a0e1527db66eb037d7e9df) )
	ROM_LOAD16_BYTE( "b01_e.u94", 0x0c0000, 0x20000, CRC(823bba4d) SHA1(6668e972b1435aac43f9b21cc40fc3adec0d285f) )
	ROM_LOAD16_BYTE( "host.71",   0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "host.95",   0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )

	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD64_BYTE( "s24e_u.134", 0x00000, 0x08000, CRC(0a41756b) SHA1(8681aaf8eeda7acdff967a773290c4b2c17cbe30) )
	ROM_LOAD64_BYTE( "s16e_u.126", 0x00001, 0x08000, CRC(d24654cd) SHA1(88d3624f23c669dc902136c822b1f4732104c9c1) )
	ROM_LOAD64_BYTE( "s08e_u.114", 0x00002, 0x08000, CRC(765da5d7) SHA1(d489581bd12d7fca42570ee7a12d922be2528c1e) )
	ROM_LOAD64_BYTE( "s00e_u.107", 0x00003, 0x08000, CRC(558918cc) SHA1(7e61639ab4af88f888f4aa481dd01db7de3829da) )
	ROM_LOAD64_BYTE( "s24o_u.135", 0x00004, 0x08000, CRC(f89bab5f) SHA1(e79e71d0a5e7ba933952c5d41f6afb633da06e8a) )
	ROM_LOAD64_BYTE( "s16o_u.127", 0x00005, 0x08000, CRC(53ba1a3f) SHA1(333734fff41b98abfa7b2904692cb128ab1f90a3) )
	ROM_LOAD64_BYTE( "s08o_u.115", 0x00006, 0x08000, CRC(af1eae4a) SHA1(44f272b472f546ffff7d8f82e29c5d80b472b1c3) )
	ROM_LOAD64_BYTE( "s00o_u.108", 0x00007, 0x08000, CRC(9cadc977) SHA1(e95f60d9df422511bae6a6c4a20f813d77a894a4) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "pb0o_u.153", 0x00001, 0x20000, CRC(bcd7ddad) SHA1(3982756b6f0821df77918dd0d00807a90dbfb595) )
	ROM_LOAD16_BYTE( "pb0e_u.154", 0x00000, 0x20000, CRC(d84e7c71) SHA1(2edb13c1f96f35c7934dad380e06035335ccbb48) )
	ROM_LOAD16_BYTE( "pb1o_u.167", 0x40001, 0x20000, CRC(e4a65313) SHA1(f2df5cc87aa388d3273705562ab2d7c937a0a866) )
	ROM_LOAD16_BYTE( "pb1e_u.160", 0x40000, 0x20000, CRC(9d9d1395) SHA1(9d937eac8d7e7bea40a69b596ba2c01753b97565) )

	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "3el_u101", 0x000000, 0x20000, CRC(130e1a18) SHA1(c31af5c5a403da588142ccbea79d3aa253ac6519) )
	ROM_LOAD16_BYTE( "3eh_u97",  0x000001, 0x20000, CRC(0fdcab16) SHA1(afc21747e1624f3ab87b289b5f4a498141062445) )

	ROM_REGION16_LE( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "38l_u124", 0x000000, 0x20000, CRC(4e084daa) SHA1(f65f51d8d7c6b46aa844b37b212dab11c786d856) )
	ROM_LOAD16_BYTE( "38h_u121", 0x000001, 0x20000, CRC(3628c8c1) SHA1(760eda076ec46af5b954548036da5230a5c86371) )
	ROM_LOAD16_BYTE( "3al_u130", 0x040000, 0x20000, CRC(8a5386e3) SHA1(361f6abdb88cf51d5ec5ce6882986296dd274d3b) )
	ROM_LOAD16_BYTE( "3ah_u133", 0x040001, 0x20000, CRC(7e674ac1) SHA1(81f1d87e62faf94a44aca7e41a32edf5c7c145ec) )
	ROM_LOAD16_BYTE( "3cl_u114", 0x080000, 0x20000, CRC(bc04b0e6) SHA1(d08fddd52f2c1a565a80f5d4ff8b07f1c5f01a01) )
	ROM_LOAD16_BYTE( "3ch_u108", 0x080001, 0x20000, CRC(7cb688af) SHA1(6be495ae0ed74739f62de65386810864c9ffaaee) )

	ROM_REGION( 0x08000, REGION_CPU3, 0 )
	ROM_LOAD( "sound.u2", 0x000000, 0x08000, CRC(77190a90) SHA1(a36a5a8457cc1c325e6318b083e5e271e163f7cb) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound.u17", 0x000000, 0x40000, CRC(d033ef6c) SHA1(0404473c87b5b52e39ab3824b159a2d98159bbea) )
ROM_END

ROM_START( stankatk )
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) // 68000 code
	ROM_LOAD16_BYTE( "110-00200-100.u67", 0x000001, 0x20000, CRC(55636cd4) SHA1(d1de9bf409d13308cafbf21b73f8c54c93fa69ef) )
	ROM_LOAD16_BYTE( "110-00200-101.u91", 0x000000, 0x20000, CRC(6fdeb287) SHA1(ad62ab95195ed4ae171dd670d9c7be37d71ddb42) )
	ROM_LOAD16_BYTE( "110-00200-102.u68", 0x040001, 0x20000, CRC(57e86fab) SHA1(92f7cb62f1ccd426174ef152150fd57c70ae1040) )
	ROM_LOAD16_BYTE( "110-00200-103.u92", 0x040000, 0x20000, CRC(450eebaa) SHA1(5347a06b5ec7a5b483d3b4354e2697436b805c04) )
	ROM_LOAD16_BYTE( "110-00200-104.u69", 0x080001, 0x20000, CRC(393718e5) SHA1(f956f8bd946f53a032af16011dc69f66fb3f095c) )
	ROM_LOAD16_BYTE( "110-00200-105.u93", 0x080000, 0x20000, CRC(aedea0ef) SHA1(a81c3518c7a1e21f2fa2ad29c30346f727069257) )
	ROM_LOAD16_BYTE( "110-00200-106.u70", 0x0c0001, 0x20000, CRC(e895167d) SHA1(677cbf1be32c1f0c76a0e1527db66eb037d7e9df) )
	ROM_LOAD16_BYTE( "110-00200-107.u94", 0x0c0000, 0x20000, CRC(823bba4d) SHA1(6668e972b1435aac43f9b21cc40fc3adec0d285f) )
	ROM_LOAD16_BYTE( "110-00013-108.u71", 0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "110-00013-109.u95", 0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )

	ROM_REGION32_BE( 0x100000, REGION_USER2, 0 )
	ROMX_LOAD( "s24e_u.134", 0x00000, 0x08000, CRC(0a41756b) SHA1(8681aaf8eeda7acdff967a773290c4b2c17cbe30), ROM_SKIP(7) )
	ROMX_LOAD( "s16e_u.126", 0x00001, 0x08000, CRC(d24654cd) SHA1(88d3624f23c669dc902136c822b1f4732104c9c1), ROM_SKIP(7) )
	ROMX_LOAD( "s08e_u.114", 0x00002, 0x08000, CRC(765da5d7) SHA1(d489581bd12d7fca42570ee7a12d922be2528c1e), ROM_SKIP(7) )
	ROMX_LOAD( "s00e_u.107", 0x00003, 0x08000, CRC(558918cc) SHA1(7e61639ab4af88f888f4aa481dd01db7de3829da), ROM_SKIP(7) )
	ROMX_LOAD( "s24o_u.135", 0x00004, 0x08000, CRC(f89bab5f) SHA1(e79e71d0a5e7ba933952c5d41f6afb633da06e8a), ROM_SKIP(7) )
	ROMX_LOAD( "s16o_u.127", 0x00005, 0x08000, CRC(53ba1a3f) SHA1(333734fff41b98abfa7b2904692cb128ab1f90a3), ROM_SKIP(7) )
	ROMX_LOAD( "s08o_u.115", 0x00006, 0x08000, CRC(af1eae4a) SHA1(44f272b472f546ffff7d8f82e29c5d80b472b1c3), ROM_SKIP(7) )
	ROMX_LOAD( "s00o_u.108", 0x00007, 0x08000, CRC(9cadc977) SHA1(e95f60d9df422511bae6a6c4a20f813d77a894a4), ROM_SKIP(7) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 )
	ROM_LOAD16_BYTE( "pb0o_u.153", 0x00001, 0x20000, CRC(bcd7ddad) SHA1(3982756b6f0821df77918dd0d00807a90dbfb595) )
	ROM_LOAD16_BYTE( "pb0e_u.154", 0x00000, 0x20000, CRC(d84e7c71) SHA1(2edb13c1f96f35c7934dad380e06035335ccbb48) )
	ROM_LOAD16_BYTE( "pb1o_u.167", 0x40001, 0x20000, CRC(e4a65313) SHA1(f2df5cc87aa388d3273705562ab2d7c937a0a866) )
	ROM_LOAD16_BYTE( "pb1e_u.160", 0x40000, 0x20000, CRC(9d9d1395) SHA1(9d937eac8d7e7bea40a69b596ba2c01753b97565) )

	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "110-00200-101.u101", 0x000000, 0x20000, CRC(4529a175) SHA1(8a2d7c6821aab08dc39a937ba4858e1b34b443b1) )
	ROM_LOAD16_BYTE( "110-00200-104.u97",  0x000001, 0x20000, CRC(f472c387) SHA1(0ef26597763df29e5e4d3b3065986efbab3192c6) )

	ROM_REGION16_LE( 0xc0000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "110-00200-105.u124", 0x000000, 0x20000, CRC(9fe560b0) SHA1(a1d1527114e7d07429ca3a08c2cbe64a492c64ce) )
	ROM_LOAD16_BYTE( "110-00200-106.u121", 0x000001, 0x20000, CRC(8768c603) SHA1(cc7e6cd0b9cd286b67195f4de8d86c3192a90e04) )
	ROM_LOAD16_BYTE( "110-00200-107.u130", 0x040000, 0x20000, CRC(af0e30bb) SHA1(04d8ce727335f6aac75b730cffd5cf5437ce145e) )
	ROM_LOAD16_BYTE( "110-00200-108.u133", 0x040001, 0x20000, CRC(4abdea5b) SHA1(03cad4dd0a252eefc60bada1e3b88c9c2534209e) )
	ROM_LOAD16_BYTE( "110-00200-103.u114", 0x080000, 0x20000, CRC(0888a2f5) SHA1(d9c23a348d46a378e45e077379fa4167308e6a73) )
	ROM_LOAD16_BYTE( "110-00200-108.u108", 0x080001, 0x20000, CRC(0691f9be) SHA1(2a3f70f387bcab09cf851bfcbbf20541484ea533) )

	ROM_REGION( 0x08000, REGION_CPU3, 0 )
	ROM_LOAD( "sound.u2", 0x000000, 0x08000, CRC(77190a90) SHA1(a36a5a8457cc1c325e6318b083e5e271e163f7cb) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "sound.u17", 0x000000, 0x40000, CRC(d033ef6c) SHA1(0404473c87b5b52e39ab3824b159a2d98159bbea) )
ROM_END


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1991, f15se,    0,        micro3d, f15se,    micro3d,  ROT0, "Microprose Games Inc.", "F-15 Strike Eagle (rev. 2.2 02/25/91, 29K ver. 3.1f)", GAME_IMPERFECT_SOUND )
GAME( 1991, f15se21,  f15se,    micro3d, f15se,    micro3d,  ROT0, "Microprose Games Inc.", "F-15 Strike Eagle (rev. 2.1 02/04/91)", GAME_IMPERFECT_SOUND )
GAME( 1992, botss,    0,        micro3d, botss,    botss,    ROT0, "Microprose Games Inc.", "Battle of the Solar System (rev. 1.1a 7/23/92)", GAME_IMPERFECT_SOUND )
GAME( 1992, botss11,  botss,    micro3d, botss11,  micro3d,  ROT0, "Microprose Games Inc.", "Battle of the Solar System (rev. 1.1 3/24/92)", GAME_IMPERFECT_SOUND )
GAME( 1992, tankbatl, 0,        micro3d, tankbatl, micro3d,  ROT0, "Microprose Games Inc.", "Tank Battle (prototype rev. 4/21/92)", GAME_IMPERFECT_SOUND )
GAME( 1992, stankatk, 0,        micro3d, tankbatl, micro3d,  ROT0, "Microprose Games Inc.", "Super Tank Attack (prototype rev. 1.12)", GAME_IMPERFECT_SOUND )
