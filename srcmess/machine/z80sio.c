/***************************************************************************

    Z80 SIO implementation

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "z80sio.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define VERBOSE		1

#if VERBOSE
#define VPRINTF(x) logerror x
#else
#define VPRINTF(x)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* interrupt states */
#define INT_CHB_TRANSMIT	0x00		/* not confirmed */
#define INT_CHB_STATUS		0x01
#define INT_CHB_RECEIVE		0x02
#define INT_CHB_ERROR		0x03

#define INT_CHA_TRANSMIT	0x04		/* not confirmed */
#define INT_CHA_STATUS		0x05
#define INT_CHA_RECEIVE		0x06
#define INT_CHA_ERROR		0x07

/* SIO write register 0 */
#define SIO_WR0_RESET_MASK				0xc0		/* D7-D6: Reset control */
#define SIO_WR0_RESET_NULL					0x00	/*  00 = NULL code */
#define SIO_WR0_RESET_RX_CRC				0x40	/*  01 = Reset Rx CRC checker */
#define SIO_WR0_RESET_TX_CRC				0x80	/*  10 = Reset Tx CRC generator */
#define SIO_WR0_RESET_TX_LATCH				0xc0	/*  11 = Reset Tx Underrun/EOM latch */
#define SIO_WR0_COMMAND_MASK			0x38		/* D5-D3: Command */
#define SIO_WR0_COMMAND_NULL				0x00	/*  000 = NULL code */
#define SIO_WR0_COMMAND_SET_ABORT			0x08	/*  001 = Set abort (SDLC) */
#define SIO_WR0_COMMAND_RES_STATUS_INT		0x10	/*  010 = reset ext/status interrupts */
#define SIO_WR0_COMMAND_CH_RESET			0x18	/*  011 = Channel reset */
#define SIO_WR0_COMMAND_ENA_RX_INT			0x20	/*  100 = Enable int on next Rx character */
#define SIO_WR0_COMMAND_RES_TX_INT			0x28	/*  101 = Reset Tx int pending */
#define SIO_WR0_COMMAND_RES_ERROR			0x30	/*  110 = Error reset */
#define SIO_WR0_COMMAND_RETI				0x38	/*  111 = Return from int (CH-A only) */
#define SIO_WR0_REGISTER_MASK			0x07		/* D2-D0: Register select (0-7) */

/* SIO write register 1 */
#define SIO_WR1_READY_WAIT_ENA			0x80		/* D7 = READY/WAIT enable */
#define SIO_WR1_READY_WAIT_FUNCTION		0x40		/* D6 = READY/WAIT function */
#define SIO_WR1_READY_WAIT_ON_RT		0x20		/* D5 = READY/WAIT on R/T */
#define SIO_WR1_RXINT_MASK				0x18		/* D4-D3 = Rx int control */
#define SIO_WR1_RXINT_DISABLE				0x00	/*  00 = Rx int disable */
#define SIO_WR1_RXINT_FIRST					0x08	/*  01 = Rx int on first character */
#define SIO_WR1_RXINT_ALL_PARITY			0x10	/*  10 = int on all Rx characters (parity affects vector) */
#define SIO_WR1_RXINT_ALL_NOPARITY			0x18	/*  11 = int on all Rx characters (parity ignored) */
#define SIO_WR1_STATUS_AFFECTS_VECTOR	0x04		/* D2 = Status affects vector (CH-B only) */
#define SIO_WR1_TXINT_ENABLE			0x02		/* D1 = Tx int enable */
#define SIO_WR1_STATUSINT_ENABLE		0x01		/* D0 = Ext int enable */

/* SIO write register 2 (CH-B only) */
#define SIO_WR2_INT_VECTOR_MASK			0xff		/* D7-D0 = interrupt vector */

/* SIO write register 3 */
#define SIO_WR3_RX_DATABITS_MASK		0xc0		/* D7-D6 = Rx Data bits */
#define SIO_WR3_RX_DATABITS_5				0x00	/*  00 = Rx 5 bits/character */
#define SIO_WR3_RX_DATABITS_7				0x40	/*  01 = Rx 7 bits/character */
#define SIO_WR3_RX_DATABITS_6				0x80	/*  10 = Rx 6 bits/character */
#define SIO_WR3_RX_DATABITS_8				0xc0	/*  11 = Rx 8 bits/character */
#define SIO_WR3_AUTO_ENABLES			0x20		/* D5 = Auto enables */
#define SIO_WR3_ENTER_HUNT_PHASE		0x10		/* D4 = Enter hunt phase */
#define SIO_WR3_RX_CRC_ENABLE			0x08		/* D3 = Rx CRC enable */
#define SIO_WR3_ADDR_SEARCH_MODE		0x04		/* D2 = Address search mode (SDLC) */
#define SIO_WR3_SYNC_LOAD_INHIBIT		0x02		/* D1 = Sync character load inhibit */
#define SIO_WR3_RX_ENABLE				0x01		/* D0 = Rx enable */

/* SIO write register 4 */
#define SIO_WR4_CLOCK_MODE_MASK			0xc0		/* D7-D6 = Clock mode */
#define SIO_WR4_CLOCK_MODE_x1				0x00	/*  00 = x1 clock mode */
#define SIO_WR4_CLOCK_MODE_x16				0x40	/*  01 = x16 clock mode */
#define SIO_WR4_CLOCK_MODE_x32				0x80	/*  10 = x32 clock mode */
#define SIO_WR4_CLOCK_MODE_x64				0xc0	/*  11 = x64 clock mode */
#define SIO_WR4_SYNC_MODE_MASK			0x30		/* D5-D4 = Sync mode */
#define SIO_WR4_SYNC_MODE_8BIT				0x00	/*  00 = 8 bit sync character */
#define SIO_WR4_SYNC_MODE_16BIT				0x10	/*  01 = 16 bit sync character */
#define SIO_WR4_SYNC_MODE_SDLC				0x20	/*  10 = SDLC mode (01111110 flag)  */
#define SIO_WR4_SYNC_MODE_EXTERNAL			0x30	/*  11 = External sync mode */
#define SIO_WR4_STOPBITS_MASK			0x0c		/* D3-D2 = Stop bits */
#define SIO_WR4_STOPBITS_SYNC				0x00	/*  00 = Sync modes enable */
#define SIO_WR4_STOPBITS_1					0x04	/*  01 = 1 stop bit/character */
#define SIO_WR4_STOPBITS_15					0x08	/*  10 = 1.5 stop bits/character */
#define SIO_WR4_STOPBITS_2					0x0c	/*  11 = 2 stop bits/character */
#define SIO_WR4_PARITY_EVEN				0x02		/* D1 = Parity even/odd */
#define SIO_WR4_PARITY_ENABLE			0x01		/* D0 = Parity enable */

/* SIO write register 5 */
#define SIO_WR5_DTR						0x80		/* D7 = DTR */
#define SIO_WR5_TX_DATABITS_MASK		0x60		/* D6-D5 = Tx Data bits */
#define SIO_WR5_TX_DATABITS_5				0x00	/*  00 = Tx 5 bits/character */
#define SIO_WR5_TX_DATABITS_7				0x20	/*  01 = Tx 7 bits/character */
#define SIO_WR5_TX_DATABITS_6				0x40	/*  10 = Tx 6 bits/character */
#define SIO_WR5_TX_DATABITS_8				0x60	/*  11 = Tx 8 bits/character */
#define SIO_WR5_SEND_BREAK				0x10		/* D4 = Send break */
#define SIO_WR5_TX_ENABLE				0x08		/* D3 = Tx Enable */
#define SIO_WR5_CRC16_SDLC				0x04		/* D2 = CRC-16/SDLC */
#define SIO_WR5_RTS						0x02		/* D1 = RTS */
#define SIO_WR5_TX_CRC_ENABLE			0x01		/* D0 = Tx CRC enable */

/* SIO write register 6  */
#define SIO_WR6_SYNC_7_0_MASK			0xff		/* D7-D0 = Sync bits 7-0 */

/* SIO write register 7 */
#define SIO_WR7_SYNC_15_8_MASK			0xff		/* D7-D0 = Sync bits 15-8 */

/* SIO read register 0 */
#define SIO_RR0_BREAK_ABORT				0x80		/* D7 = Break/abort */
#define SIO_RR0_TX_UNDERRUN				0x40		/* D6 = Tx underrun/EOM */
#define SIO_RR0_CTS						0x20		/* D5 = CTS */
#define SIO_RR0_SYNC_HUNT				0x10		/* D4 = Sync/hunt */
#define SIO_RR0_DCD						0x08		/* D3 = DCD */
#define SIO_RR0_TX_BUFFER_EMPTY			0x04		/* D2 = Tx buffer empty */
#define SIO_RR0_INT_PENDING				0x02		/* D1 = int pending (CH-A only) */
#define SIO_RR0_RX_CHAR_AVAILABLE		0x01		/* D0 = Rx character available */

/* SIO read register 1 */
#define SIO_RR1_END_OF_FRAME			0x80		/* D7 = End of frame (SDLC) */
#define SIO_RR1_CRC_FRAMING_ERROR		0x40		/* D6 = CRC/Framing error */
#define SIO_RR1_RX_OVERRUN_ERROR		0x20		/* D5 = Rx overrun error */
#define SIO_RR1_PARITY_ERROR			0x10		/* D4 = Parity error */
#define SIO_RR1_IFIELD_BITS_MASK		0x0e		/* D3-D1 = I field bits */
													/*  100 = 0 in prev, 3 in 2nd prev */
													/*  010 = 0 in prev, 4 in 2nd prev */
													/*  110 = 0 in prev, 5 in 2nd prev */
													/*  001 = 0 in prev, 6 in 2nd prev */
													/*  101 = 0 in prev, 7 in 2nd prev */
													/*  011 = 0 in prev, 8 in 2nd prev */
													/*  111 = 1 in prev, 8 in 2nd prev */
													/*  000 = 2 in prev, 8 in 2nd prev */
#define SIO_RR1_ALL_SENT				0x01		/* D0 = All sent */

/* SIO read register 2 (CH-B only) */
#define SIO_RR2_VECTOR_MASK				0xff		/* D7-D0 = Interrupt vector */



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _z80sio
{
	UINT8 		regs[2][8];			/* 8 writeable registers */
	UINT8 		status[2][4];		/* 3 readable registers */
	UINT8		int_state[8];		/* interrupt states */
	UINT8		int_on_next_rx[2];	/* interrupt on next rx? */
	UINT8		inbuf[2], outbuf[2];/* input and output buffers */
	void (*irq_cb)(int state);
	write8_handler dtr_changed_cb;
	write8_handler rts_changed_cb;
	write8_handler break_changed_cb;
	write8_handler transmit_cb;
};
typedef struct _z80sio z80sio;



/***************************************************************************
    GLOBALS
***************************************************************************/

static z80sio sios[MAX_SIO];


/*

    Interrupt priorities:
        Ch A receive
        Ch A transmit
        Ch A external/status
        Ch B receive
        Ch B transmit
        Ch B external/status


    Initial configuration (both channels):
        005D:sio_reg_w(0,4) = 44
                    01 = x16 clock mode
                    00 = 8 bit sync character
                    01 = 1 stop bit/character
                    Parity odd
                    Parity disabled

        005D:sio_reg_w(0,3) = C1
                    11 = Rx 8 bits/character
                    No auto enables
                    No enter hunt phase
                    No Rx CRC enable
                    No address search mode
                    No sync character load inhibit
                    Rx enable

        005D:sio_reg_w(0,5) = 68
                    DTR = 0
                    11 = Tx 8 bits/character
                    No send break
                    Tx enable
                    SDLC
                    No RTS
                    No CRC enable

        005D:sio_reg_w(0,2) = 40
                    Vector = 0x40

        005D:sio_reg_w(0,1) = 1D
                    No READY/WAIT
                    No READY/WAIT function
                    No READY/WAIT on R/T
                    11 = int on all Rx characters (parity ignored)
                    Status affects vector
                    No Tx int enable
                    Ext int enable

*/


static void interrupt_check(z80sio *sio)
{
	/* if we have a callback, update it with the current state */
	if (sio->irq_cb)
		(*sio->irq_cb)((z80sio_irq_state(sio - sios) & Z80_DAISY_INT) ? ASSERT_LINE : CLEAR_LINE);
}



/***************************************************************************
    INITIALIZATION/CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    z80sio_init - initialize a single SIO chip
-------------------------------------------------*/

void z80sio_init(int which, z80sio_interface *intf)
{
	z80sio *sio = sios + which;

	assert(which < MAX_SIO);

	memset(sio, 0, sizeof(*sio));

	sio->irq_cb = intf->irq_cb;
	sio->dtr_changed_cb = intf->dtr_changed_cb;
	sio->rts_changed_cb = intf->rts_changed_cb;
	sio->break_changed_cb = intf->break_changed_cb;
	sio->transmit_cb = intf->transmit_cb;

	z80sio_reset(which);
}


/*-------------------------------------------------
    reset_channel - reset a single SIO channel
-------------------------------------------------*/

static void reset_channel(z80sio *sio, int ch)
{
	sio->status[ch][0] = SIO_RR0_TX_BUFFER_EMPTY;
	sio->status[ch][1] = 0x00;
	sio->status[ch][2] = 0x00;
	sio->int_on_next_rx[ch] = 0;
	sio->int_state[0 + 4*ch] = 0;
	sio->int_state[1 + 4*ch] = 0;
	sio->int_state[2 + 4*ch] = 0;
	sio->int_state[3 + 4*ch] = 0;
	interrupt_check(sio);
}


/*-------------------------------------------------
    z80sio_reset - reset a single SIO chip
-------------------------------------------------*/

void z80sio_reset(int which)
{
	z80sio *sio = sios + which;
	int ch;

	assert(which < MAX_SIO);

	/* loop over channels */
	for (ch = 0; ch < 2; ch++)
		reset_channel(sio, ch);
}



/***************************************************************************
    CONTROL REGISTER READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    z80sio_c_w - write to a control register
-------------------------------------------------*/

void z80sio_c_w(int which, int ch, UINT8 data)
{
	z80sio *sio = sios + which;
	int reg = sio->regs[ch][0] & 7;
	UINT8 old = sio->regs[ch][reg];

	if (reg != 0 || (reg & 0xf8))
		VPRINTF(("%04X:sio_reg_w(%c,%d) = %02X\n", activecpu_get_pc(), 'A' + ch, reg, data));

	/* write a new value to the selected register */
	sio->regs[ch][reg] = data;

	/* clear the register number for the next write */
	if (reg != 0)
		sio->regs[ch][0] &= ~7;

	/* switch off the register for live state changes */
	switch (reg)
	{
		/* SIO write register 0 */
		case 0:
			switch (data & SIO_WR0_COMMAND_MASK)
			{
				case SIO_WR0_COMMAND_CH_RESET:
					VPRINTF(("%04X:SIO reset channel %c\n", activecpu_get_pc(), 'A' + ch));
					reset_channel(sio, ch);
					break;

				case SIO_WR0_COMMAND_RES_STATUS_INT:
					sio->int_state[INT_CHA_STATUS - 4*ch] &= ~Z80_DAISY_INT;
					interrupt_check(sio);
					break;

				case SIO_WR0_COMMAND_ENA_RX_INT:
					sio->int_on_next_rx[ch] = TRUE;
					interrupt_check(sio);
					break;

				case SIO_WR0_COMMAND_RES_TX_INT:
					sio->int_state[INT_CHA_TRANSMIT - 4*ch] &= ~Z80_DAISY_INT;
					interrupt_check(sio);
					break;

				case SIO_WR0_COMMAND_RES_ERROR:
					sio->int_state[INT_CHA_ERROR - 4*ch] &= ~Z80_DAISY_INT;
					interrupt_check(sio);
					break;
			}
			break;

		/* SIO write register 5 */
		case 5:
			if (((old ^ data) & SIO_WR5_DTR) && sio->dtr_changed_cb)
				(*sio->dtr_changed_cb)(ch, (data & SIO_WR5_DTR) != 0);
			if (((old ^ data) & SIO_WR5_SEND_BREAK) && sio->break_changed_cb)
				(*sio->break_changed_cb)(ch, (data & SIO_WR5_SEND_BREAK) != 0);
			if (((old ^ data) & SIO_WR5_RTS) && sio->rts_changed_cb)
				(*sio->rts_changed_cb)(ch, (data & SIO_WR5_RTS) != 0);
			break;
	}
}


/*-------------------------------------------------
    z80sio_c_r - read from a control register
-------------------------------------------------*/

UINT8 z80sio_c_r(int which, int ch)
{
	z80sio *sio = sios + which;
	int reg = sio->regs[ch][0] & 7;
	UINT8 result = sio->status[ch][reg];

	/* switch off the register for live state changes */
	switch (reg)
	{
		/* SIO read register 0 */
		case 0:
			result &= ~SIO_RR0_INT_PENDING;
			if (z80sio_irq_state(which) & Z80_DAISY_INT)
				result |= SIO_RR0_INT_PENDING;
			break;
	}

	VPRINTF(("%04X:sio_reg_r(%c,%d) = %02x\n", activecpu_get_pc(), 'A' + ch, reg, sio->status[ch][reg]));

	return sio->status[ch][reg];
}




/***************************************************************************
    DATA REGISTER READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    transmit_complete - timer callback that is
    signalled when the character has finished
    transmitting
-------------------------------------------------*/

static void transmit_complete(int param)
{
	z80sio *sio = sios + (param >> 1);
	int ch = param & 1;

	VPRINTF(("sio_transmit_complete(%c) = %02x\n", 'A' + ch, sio->outbuf[ch]));

	/* actually transmit the character */
	if (sio->transmit_cb)
		(*sio->transmit_cb)(ch, sio->outbuf[ch]);

	/* update the status register */
	sio->status[ch][0] |= SIO_RR0_TX_BUFFER_EMPTY;

	/* set the transmit buffer empty interrupt if enabled */
	if (sio->regs[ch][1] & SIO_WR1_TXINT_ENABLE)
	{
		sio->int_state[INT_CHA_TRANSMIT - 4*ch] |= Z80_DAISY_INT;
		interrupt_check(sio);
	}
}


/*-------------------------------------------------
    z80sio_d_w - write to a data register
-------------------------------------------------*/

void z80sio_d_w(int which, int ch, UINT8 data)
{
	z80sio *sio = sios + which;

	VPRINTF(("%04X:sio_data_w(%c) = %02X\n", activecpu_get_pc(), 'A' + ch, data));

	/* if tx not enabled, just ignore it */
	if (!(sio->regs[ch][5] & SIO_WR5_TX_ENABLE))
		return;

	/* update the status register */
	sio->status[ch][0] &= ~SIO_RR0_TX_BUFFER_EMPTY;

	/* reset the transmit interrupt */
	sio->int_state[INT_CHA_TRANSMIT - 4*ch] &= ~Z80_DAISY_INT;
	interrupt_check(sio);

	/* stash the character */
	sio->outbuf[ch] = data;

	/* fix me - should use the baud rate and number of bits */
	timer_set(TIME_IN_HZ(9600/10), which * 2 + ch, transmit_complete);
}


/*-------------------------------------------------
    z80sio_d_r - read from a data register
-------------------------------------------------*/

UINT8 z80sio_d_r(int which, int ch)
{
	z80sio *sio = sios + which;

	/* update the status register */
	sio->status[ch][0] &= ~SIO_RR0_RX_CHAR_AVAILABLE;

	/* reset the receive interrupt */
	sio->int_state[INT_CHA_RECEIVE - 4*ch] &= ~Z80_DAISY_INT;
	interrupt_check(sio);

	VPRINTF(("%04X:sio_data_r(%c) = %02X\n", activecpu_get_pc(), 'A' + ch, sio->inbuf[ch]));

	return sio->inbuf[ch];
}



/***************************************************************************
    CONTROL LINE READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    z80sio_get_dtr - return the state of the DTR
    line
-------------------------------------------------*/

int z80sio_get_dtr(int which, int ch)
{
	z80sio *sio = sios + which;
	return ((sio->regs[ch][5] & SIO_WR5_DTR) != 0);
}


/*-------------------------------------------------
    z80sio_get_rts - return the state of the RTS
    line
-------------------------------------------------*/

int z80sio_get_rts(int which, int ch)
{
	z80sio *sio = sios + which;
	return ((sio->regs[ch][5] & SIO_WR5_RTS) != 0);
}


/*-------------------------------------------------
    z80sio_set_cts - set the state of the CTS
    line
-------------------------------------------------*/

static void change_input_line(int param)
{
	z80sio *sio = sios + ((param >> 1) & 0x3f);
	UINT8 line = (param >> 8) & 0xff;
	int state = (param >> 7) & 1;
	int ch = param & 1;
	UINT8 old;

	VPRINTF(("sio_change_input_line(%c, %s) = %d\n", 'A' + ch, (line == SIO_RR0_CTS) ? "CTS" : "DCD", state));

	/* remember the old value */
	old = sio->status[ch][0];

	/* set the bit in the status register */
	sio->status[ch][0] &= ~line;
	if (state)
		sio->status[ch][0] |= line;

	/* if state change interrupts are enabled, signal */
	if (((old ^ sio->status[ch][0]) & line) && (sio->regs[ch][1] & SIO_WR1_STATUSINT_ENABLE))
	{
		sio->int_state[INT_CHA_STATUS - 4*ch] |= Z80_DAISY_INT;
		interrupt_check(sio);
	}
}


/*-------------------------------------------------
    z80sio_set_cts - set the state of the CTS
    line
-------------------------------------------------*/

void z80sio_set_cts(int which, int ch, int state)
{
	/* operate deferred */
	timer_set(TIME_NOW, (SIO_RR0_CTS << 8) + (state != 0) * 0x80 + which * 2 + ch, change_input_line);
}


/*-------------------------------------------------
    z80sio_set_dcd - set the state of the DCD
    line
-------------------------------------------------*/

void z80sio_set_dcd(int which, int ch, int state)
{
	/* operate deferred */
	timer_set(TIME_NOW, (SIO_RR0_DCD << 8) + (state != 0) * 0x80 + which * 2 + ch, change_input_line);
}


/*-------------------------------------------------
    z80sio_receive_data - receive data on the
    input lines
-------------------------------------------------*/

void z80sio_receive_data(int which, int ch, UINT8 data)
{
	z80sio *sio = sios + which;

	VPRINTF(("sio_receive_data(%c) = %02x\n", 'A' + ch, data));

	/* if rx not enabled, just ignore it */
	if (!(sio->regs[ch][3] & SIO_WR3_RX_ENABLE))
	{
		VPRINTF(("  (ignored because receive is disabled)\n"));
		return;
	}

	/* stash the data and update the status */
	sio->inbuf[ch] = data;
	sio->status[ch][0] |= SIO_RR0_RX_CHAR_AVAILABLE;

	/* update our interrupt state */
	switch (sio->regs[ch][1] & SIO_WR1_RXINT_MASK)
	{
		case SIO_WR1_RXINT_FIRST:
			if (!sio->int_on_next_rx[ch])
				break;

		case SIO_WR1_RXINT_ALL_NOPARITY:
		case SIO_WR1_RXINT_ALL_PARITY:
			sio->int_state[INT_CHA_RECEIVE - 4*ch] |= Z80_DAISY_INT;
			interrupt_check(sio);
			break;
	}
	sio->int_on_next_rx[ch] = FALSE;
}



/***************************************************************************
    DAISY CHAIN INTERFACE
***************************************************************************/

static const UINT8 int_priority[] =
{
	INT_CHA_RECEIVE,
	INT_CHA_TRANSMIT,
	INT_CHA_STATUS,
	INT_CHA_ERROR,
	INT_CHB_RECEIVE,
	INT_CHB_TRANSMIT,
	INT_CHB_STATUS,
	INT_CHB_ERROR
};


int z80sio_irq_state(int which)
{
	z80sio *sio = sios + which;
	int state = 0;
	int i;

	VPRINTF(("sio IRQ state = B:%d%d%d%d A:%d%d%d%d\n",
				sio->int_state[0], sio->int_state[1], sio->int_state[2], sio->int_state[3],
				sio->int_state[4], sio->int_state[5], sio->int_state[6], sio->int_state[7]));

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		int inum = int_priority[i];

		/* if we're servicing a request, don't indicate more interrupts */
		if (sio->int_state[inum] & Z80_DAISY_IEO)
		{
			state |= Z80_DAISY_IEO;
			break;
		}
		state |= sio->int_state[inum];
	}

	return state;
}


int z80sio_irq_ack(int which)
{
	z80sio *sio = sios + which;
	int i;

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		int inum = int_priority[i];

		/* find the first channel with an interrupt requested */
		if (sio->int_state[inum] & Z80_DAISY_INT)
		{
			VPRINTF(("sio IRQAck %d\n", inum));

			/* clear interrupt, switch to the IEO state, and update the IRQs */
			sio->int_state[inum] = Z80_DAISY_IEO;
			interrupt_check(sio);
			return sio->regs[1][2] + inum * 2;
		}
	}

	logerror("z80sio_irq_ack: failed to find an interrupt to ack!\n");
	return sio->regs[1][2];
}


void z80sio_irq_reti(int which)
{
	z80sio *sio = sios + which;
	int i;

	/* loop over all interrupt sources */
	for (i = 0; i < 8; i++)
	{
		int inum = int_priority[i];

		/* find the first channel with an IEO pending */
		if (sio->int_state[inum] & Z80_DAISY_IEO)
		{
			VPRINTF(("sio IRQReti %d\n", inum));

			/* clear the IEO state and update the IRQs */
			sio->int_state[inum] &= ~Z80_DAISY_IEO;
			interrupt_check(sio);
			return;
		}
	}

	logerror("z80sio_irq_reti: failed to find an interrupt to clear IEO on!\n");
}
