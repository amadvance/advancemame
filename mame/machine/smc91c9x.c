// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    SMC91C9X ethernet controller implementation

    by Aaron Giles

***************************************************************************

    Notes:
        * only loopback mode really works

**************************************************************************/

#include "driver.h"
#include "smc91c9x.h"
#include <zlib.h>



/*************************************
 *
 *  Debugging constants
 *
 *************************************/

#define LOG_ETHERNET		(0)
#define DISPLAY_STATS		(0)



/*************************************
 *
 *  Ethernet constants
 *
 *************************************/

#define ETHER_BUFFER_SIZE	(2048)
#define ETHER_BUFFERS		(18)

/* Ethernet registers - bank 0 */
#define EREG_TCR			(0*8 + 0)
#define EREG_EPH_STATUS		(0*8 + 1)
#define EREG_RCR			(0*8 + 2)
#define EREG_COUNTER		(0*8 + 3)
#define EREG_MIR			(0*8 + 4)
#define EREG_MCR			(0*8 + 5)
#define EREG_BANK			(0*8 + 7)

/* Ethernet registers - bank 1 */
#define EREG_CONFIG			(1*8 + 0)
#define EREG_BASE			(1*8 + 1)
#define EREG_IA0_1			(1*8 + 2)
#define EREG_IA2_3			(1*8 + 3)
#define EREG_IA4_5			(1*8 + 4)
#define EREG_GENERAL_PURP	(1*8 + 5)
#define EREG_CONTROL		(1*8 + 6)

/* Ethernet registers - bank 2 */
#define EREG_MMU_COMMAND	(2*8 + 0)
#define EREG_PNR_ARR		(2*8 + 1)
#define EREG_FIFO_PORTS		(2*8 + 2)
#define EREG_POINTER		(2*8 + 3)
#define EREG_DATA_0			(2*8 + 4)
#define EREG_DATA_1			(2*8 + 5)
#define EREG_INTERRUPT		(2*8 + 6)

/* Ethernet registers - bank 3 */
#define EREG_MT0_1			(3*8 + 0)
#define EREG_MT2_3			(3*8 + 1)
#define EREG_MT4_5			(3*8 + 2)
#define EREG_MT6_7			(3*8 + 3)
#define EREG_MGMT			(3*8 + 4)
#define EREG_REVISION		(3*8 + 5)
#define EREG_ERCV			(3*8 + 6)

/* Ethernet MMU commands */
#define ECMD_NOP			0
#define ECMD_ALLOCATE		2
#define ECMD_RESET_MMU		4
#define ECMD_REMOVE_RX		6
#define ECMD_REMOVE_TX		7
#define ECMD_REMOVE_RELEASE_RX	8
#define ECMD_RELEASE_PACKET	10
#define ECMD_ENQUEUE_PACKET	12
#define ECMD_RESET_FIFOS	14

/* Ethernet interrupt bits */
#define EINT_RCV			0x01
#define EINT_TX				0x02
#define EINT_TX_EMPTY		0x04
#define EINT_ALLOC			0x08
#define EINT_RX_OVRN		0x10
#define EINT_EPH			0x20
#define EINT_ERCV			0x40

/* Ethernet register names */
static const char *ethernet_regname[64] =
{
	"TCR", "EPH STATUS", "RCR", "COUNTER", "MIR", "MCR", "(0.6)", "BANK",
	"CONFIG", "BASE", "IA0-1", "IA2-3", "IA4-5", "GENERAL PURPOSE", "CONTROL", "BANK",
	"MMU COMMAND", "PNR ARR", "FIFO PORTS", "POINTER", "DATA", "DATA", "INTERRUPT", "BANK",
	"MT0-1", "MT2-3", "MT4-5", "MT6-7", "MGMT", "REVISION", "ERCV", "BANK",
	"(4.0)", "(4.1)", "(4.2)", "(4.3)", "(4.4)", "(4.5)", "(4.6)", "BANK",
	"(5.0)", "(5.1)", "(5.2)", "(5.3)", "(5.4)", "(5.5)", "(5.6)", "BANK",
	"(6.0)", "(6.1)", "(6.2)", "(6.3)", "(6.4)", "(6.5)", "(6.6)", "BANK",
	"(7.0)", "(7.1)", "(7.2)", "(7.3)", "(7.4)", "(7.5)", "(7.6)", "BANK"
};



/*************************************
 *
 *  Structures
 *
 *************************************/

struct smc91c94_data
{
	void (*irq_handler)(int state);

	/* raw register data and masks */
	UINT16			reg[64];
	UINT16			regmask[64];

	/* IRQ information */
	UINT8			irq_state;

	/* packet memory and its allocation by packet number */
	UINT8			buffer[ETHER_BUFFER_SIZE * ETHER_BUFFERS];
	UINT32			alloc_tx;
	UINT32			alloc_rx;

	/* fifos of the packet numbers completed, read through FIFO PORTS */
	UINT8			comp_tx[ETHER_BUFFERS];
	UINT8			comp_tx_count;
	UINT8			comp_rx[ETHER_BUFFERS];
	UINT8			comp_rx_count;

	/* counters */
	UINT32			sent;
	UINT32			recd;
};



/*************************************
 *
 *  Local variables
 *
 *************************************/

static struct smc91c94_data ethernet;



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void update_ethernet_irq(void);



/*************************************
 *
 *  Initialization
 *
 *************************************/

void smc91c94_init(struct smc91c9x_interface *config)
{
	ethernet.irq_handler = config->irq_handler;
}



/*************************************
 *
 *  Reset
 *
 *************************************/

void smc91c94_reset(void)
{
	void (*saved_handler)(int) = ethernet.irq_handler;
	memset(&ethernet, 0, sizeof(ethernet));
	ethernet.irq_handler = saved_handler;

	ethernet.reg[EREG_TCR]			= 0x0000;	ethernet.regmask[EREG_TCR]			= 0xbd87;
	ethernet.reg[EREG_EPH_STATUS]	= 0x4000;	ethernet.regmask[EREG_EPH_STATUS]	= 0x0000;
	ethernet.reg[EREG_RCR]			= 0x0000;	ethernet.regmask[EREG_RCR]			= 0xc307;
	ethernet.reg[EREG_COUNTER]		= 0x0000;	ethernet.regmask[EREG_COUNTER]		= 0x0000;
	ethernet.reg[EREG_MIR]			= 0x1212;	ethernet.regmask[EREG_MIR]			= 0x0000;
	ethernet.reg[EREG_MCR]			= 0x3300;	ethernet.regmask[EREG_MCR]			= 0x00ff;
	ethernet.reg[EREG_BANK]			= 0x3330;	ethernet.regmask[EREG_BANK]			= 0x0007;

	ethernet.reg[EREG_CONFIG]		= 0x0030;	ethernet.regmask[EREG_CONFIG]		= 0x17c6;
	ethernet.reg[EREG_BASE]			= 0x1866;	ethernet.regmask[EREG_BASE]			= 0xfffe;
	ethernet.reg[EREG_IA0_1]		= 0x0000;	ethernet.regmask[EREG_IA0_1]		= 0xffff;
	ethernet.reg[EREG_IA2_3]		= 0x0000;	ethernet.regmask[EREG_IA2_3]		= 0xffff;
	ethernet.reg[EREG_IA4_5]		= 0x0000;	ethernet.regmask[EREG_IA4_5]		= 0xffff;
	ethernet.reg[EREG_GENERAL_PURP]	= 0x0000;	ethernet.regmask[EREG_GENERAL_PURP]	= 0xffff;
	ethernet.reg[EREG_CONTROL]		= 0x0100;	ethernet.regmask[EREG_CONTROL]		= 0x68e7;

	ethernet.reg[EREG_MMU_COMMAND]	= 0x0000;	ethernet.regmask[EREG_MMU_COMMAND]	= 0x00e7;
	ethernet.reg[EREG_PNR_ARR]		= 0x8000;	ethernet.regmask[EREG_PNR_ARR]		= 0x00ff;
	ethernet.reg[EREG_FIFO_PORTS]	= 0x8080;	ethernet.regmask[EREG_FIFO_PORTS]	= 0x0000;
	ethernet.reg[EREG_POINTER]		= 0x0000;	ethernet.regmask[EREG_POINTER]		= 0xf7ff;
	ethernet.reg[EREG_DATA_0]		= 0x0000;	ethernet.regmask[EREG_DATA_0]		= 0xffff;
	ethernet.reg[EREG_DATA_1]		= 0x0000;	ethernet.regmask[EREG_DATA_1]		= 0xffff;
	ethernet.reg[EREG_INTERRUPT]	= 0x0004;	ethernet.regmask[EREG_INTERRUPT]	= 0x7f00;

	ethernet.reg[EREG_MT0_1]		= 0x0000;	ethernet.regmask[EREG_MT0_1]		= 0xffff;
	ethernet.reg[EREG_MT2_3]		= 0x0000;	ethernet.regmask[EREG_MT2_3]		= 0xffff;
	ethernet.reg[EREG_MT4_5]		= 0x0000;	ethernet.regmask[EREG_MT4_5]		= 0xffff;
	ethernet.reg[EREG_MT6_7]		= 0x0000;	ethernet.regmask[EREG_MT6_7]		= 0xffff;
	ethernet.reg[EREG_MGMT]			= 0x3030;	ethernet.regmask[EREG_MGMT]			= 0x0f0f;
	ethernet.reg[EREG_REVISION]		= 0x3345;	ethernet.regmask[EREG_REVISION]		= 0x0000;
	ethernet.reg[EREG_ERCV]			= 0x331f;	ethernet.regmask[EREG_ERCV]			= 0x009f;

	update_ethernet_irq();
}



/*************************************
 *
 *  Internal IRQ handling
 *
 *************************************/

static void update_ethernet_irq(void)
{
	UINT8 mask;
	UINT8 state;

	/* the tx and rcv interrupts are pending while the completion fifos are not empty */
	if (ethernet.comp_tx_count)
		ethernet.reg[EREG_INTERRUPT] |= EINT_TX;
	else
		ethernet.reg[EREG_INTERRUPT] &= ~EINT_TX;
	if (ethernet.comp_rx_count)
		ethernet.reg[EREG_INTERRUPT] |= EINT_RCV;
	else
		ethernet.reg[EREG_INTERRUPT] &= ~EINT_RCV;

	mask = ethernet.reg[EREG_INTERRUPT] >> 8;
	state = ethernet.reg[EREG_INTERRUPT] & 0xff;

	/* update the IRQ state */
	ethernet.irq_state = ((mask & state) != 0);
	if (ethernet.irq_handler)
		(*ethernet.irq_handler)(ethernet.irq_state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *  Draw the stats
 *
 *************************************/

static void update_stats(void)
{
	if (DISPLAY_STATS)
		ui_popup("Sent:%d  Rec'd:%d", ethernet.sent, ethernet.recd);
}




/*************************************
 *
 *  Complete an enqueued packet
 *
 *************************************/

static int address_filter(const UINT8 *buf, UINT16 *status)
{
	int i;

	/* promiscuous mode */
	if (ethernet.reg[EREG_RCR] & 0x0002)
		return 1;

	if (buf[0] & 1)
	{
		UINT32 crc;
		int hash = 0;

		/* broadcast */
		if (buf[0] == 0xff && buf[1] == 0xff && buf[2] == 0xff && buf[3] == 0xff && buf[4] == 0xff && buf[5] == 0xff)
		{
			*status |= 0x4000;
			return 1;
		}

		/* multicast, the hash is from the upper 6 bits of the inverted and reflected crc */
		crc = crc32(0, buf, 6);
		for (i = 0; i < 6; ++i)
			hash |= ((~crc >> i) & 1) << (5 - i);
		if ((ethernet.reg[EREG_RCR] & 0x0004) || ((ethernet.reg[EREG_MT0_1 + hash / 16] >> (hash % 16)) & 1))
		{
			*status |= (hash << 1) | 0x0001;
			return 1;
		}
		return 0;
	}

	/* unicast, the address registers are little endian like the frame bytes */
	for (i = 0; i < 6; ++i)
		if (buf[i] != ((ethernet.reg[EREG_IA0_1 + i / 2] >> (8 * (i & 1))) & 0xff))
			return 0;
	return 1;
}


static int alloc_packet(int tx)
{
	int i;

	for (i = 0; i < ETHER_BUFFERS; ++i)
		if (!((ethernet.alloc_tx | ethernet.alloc_rx) & (1 << i)))
		{
			if (tx)
				ethernet.alloc_tx |= 1 << i;
			else
				ethernet.alloc_rx |= 1 << i;
			return i;
		}
	return -1;
}


static void release_packet(int packet_num)
{
	ethernet.alloc_tx &= ~(1 << packet_num);
	ethernet.alloc_rx &= ~(1 << packet_num);
}


static void pop_fifo(UINT8 *fifo, UINT8 *count)
{
	if (*count)
	{
		--*count;
		memmove(&fifo[0], &fifo[1], *count);
	}
}


static void mmu_reset(void)
{
	ethernet.alloc_tx = 0;
	ethernet.alloc_rx = 0;
	ethernet.comp_tx_count = 0;
	ethernet.comp_rx_count = 0;
	update_ethernet_irq();
}


static void receive_packet(const UINT8 *buf, int length)
{
	UINT16 status = 0;
	UINT8 *packet;
	int packet_num;
	int buf_length;

	if (!(ethernet.reg[EREG_RCR] & 0x0100) || !address_filter(buf, &status))
		return;

	packet_num = alloc_packet(0);
	if (packet_num < 0)
	{
		ethernet.reg[EREG_INTERRUPT] |= EINT_ALLOC;
		return;
	}
	packet = &ethernet.buffer[packet_num * ETHER_BUFFER_SIZE];

	/* strip the crc */
	if (ethernet.reg[EREG_RCR] & 0x0200)
		length -= 4;
	memcpy(&packet[4], buf, length);

	/* set the control byte at the end */
	if (length & 1)
	{
		status |= 0x1000;
		packet[4 + length] = 0x60;
		buf_length = length + 5;
	}
	else
	{
		packet[4 + length] = 0x00;
		packet[5 + length] = 0x40;
		buf_length = length + 6;
	}

	packet[0] = status;
	packet[1] = status >> 8;
	packet[2] = buf_length;
	packet[3] = buf_length >> 8;

	ethernet.comp_rx[ethernet.comp_rx_count++] = packet_num;
}


static void transmit_packet(int packet_num)
{
	UINT8 *tx = &ethernet.buffer[packet_num * ETHER_BUFFER_SIZE];
	UINT8 frame[ETHER_BUFFER_SIZE + 64];
	int length = ((tx[3] << 8) | tx[2]) & 0x7ff;
	UINT8 control = tx[length - 1];

	/* remove the control byte and the optional pad, then the status and length words */
	length -= (control & 0x20) ? 1 : 2;
	length -= 4;
	memcpy(frame, &tx[4], length);

	/* pad to the minimum frame size */
	if (ethernet.reg[EREG_TCR] & 0x0080)
		while (length < 60)
			frame[length++] = 0x00;

	/* append the crc */
	if ((control & 0x10) || !(ethernet.reg[EREG_TCR] & 0x0100))
	{
		UINT32 crc = crc32(0, frame, length);
		frame[length++] = crc;
		frame[length++] = crc >> 8;
		frame[length++] = crc >> 16;
		frame[length++] = crc >> 24;
	}

	/* there are no other machines on the cable, so the controller receives only its own
	   frames, in loopback and in full duplex: Rush uses them to detect that it's alone,
	   otherwise it waits forever for the other linked cabinets */
	if (ethernet.reg[EREG_TCR] & 0x2802)
		receive_packet(frame, length);

	ethernet.sent++;
	update_stats();

	/* the transmission always succeeds */
	ethernet.reg[EREG_EPH_STATUS] = (ethernet.reg[EREG_EPH_STATUS] & 0x4000) | 0x0001;
	if (frame[0] == 0xff && frame[1] == 0xff && frame[2] == 0xff && frame[3] == 0xff && frame[4] == 0xff && frame[5] == 0xff)
		ethernet.reg[EREG_EPH_STATUS] |= 0x0040;
	ethernet.reg[EREG_INTERRUPT] |= EINT_TX_EMPTY;

	if (ethernet.reg[EREG_CONTROL] & 0x0800)
	{
		/* auto release */
		release_packet(packet_num);
	}
	else
	{
		/* store the status in the transmit buffer and wait for the cpu to remove it */
		tx[0] = ethernet.reg[EREG_EPH_STATUS];
		tx[1] = ethernet.reg[EREG_EPH_STATUS] >> 8;
		ethernet.comp_tx[ethernet.comp_tx_count++] = packet_num;
	}

	update_ethernet_irq();
}



/*************************************
 *
 *  MMU command processing
 *
 *************************************/

static void process_command(UINT16 data)
{
	switch ((data >> 4) & 0xf)
	{
		case ECMD_NOP:
			if (LOG_ETHERNET)
				logerror("   NOP\n");
			break;

		case ECMD_ALLOCATE:
		{
			int packet_num = alloc_packet(1);
			if (LOG_ETHERNET)
				logerror("   ALLOCATE MEMORY FOR TX (%d) = %d\n", (data & 7), packet_num);
			if (packet_num >= 0)
			{
				ethernet.reg[EREG_PNR_ARR] &= ~0xff00;
				ethernet.reg[EREG_PNR_ARR] |= packet_num << 8;
				ethernet.reg[EREG_INTERRUPT] |= EINT_ALLOC;
				update_ethernet_irq();
			}
			break;
		}

		case ECMD_RESET_MMU:
			if (LOG_ETHERNET)
				logerror("   RESET MMU\n");
			mmu_reset();
			break;

		case ECMD_REMOVE_TX:
			if (LOG_ETHERNET)
				logerror("   REMOVE FRAME FROM TX FIFO\n");
			pop_fifo(ethernet.comp_tx, &ethernet.comp_tx_count);
			break;

		case ECMD_REMOVE_RX:
		case ECMD_REMOVE_RELEASE_RX:
			if (LOG_ETHERNET)
				logerror("   REMOVE FRAME FROM RX FIFO\n");
			if (ethernet.comp_rx_count)
			{
				/* only the remove and release command frees the packet memory */
				if (((data >> 4) & 0xf) == ECMD_REMOVE_RELEASE_RX)
					release_packet(ethernet.comp_rx[0]);
				pop_fifo(ethernet.comp_rx, &ethernet.comp_rx_count);
			}
			update_ethernet_irq();
			ethernet.recd++;
			update_stats();
			break;

		case ECMD_RELEASE_PACKET:
			if (LOG_ETHERNET)
				logerror("   RELEASE SPECIFIC PACKET\n");
			release_packet(ethernet.reg[EREG_PNR_ARR] & 0x1f);
			break;

		case ECMD_ENQUEUE_PACKET:
			if (LOG_ETHERNET)
				logerror("   ENQUEUE TX PACKET\n");
			if (ethernet.reg[EREG_TCR] & 0x0001)
				transmit_packet(ethernet.reg[EREG_PNR_ARR] & 0x1f);
			break;

		case ECMD_RESET_FIFOS:
			if (LOG_ETHERNET)
				logerror("   RESET TX FIFOS\n");
			ethernet.comp_tx_count = 0;
			update_ethernet_irq();
			break;
	}
	ethernet.reg[EREG_MMU_COMMAND] |= 0x0001;
}



/* the data register accesses the first received packet or the selected packet */
static UINT8 *packet_buffer(void)
{
	int packet_num = (ethernet.reg[EREG_POINTER] & 0x8000) ? ethernet.comp_rx[0] : (ethernet.reg[EREG_PNR_ARR] & 0x1f);
	return &ethernet.buffer[(packet_num % ETHER_BUFFERS) * ETHER_BUFFER_SIZE];
}



/*************************************
 *
 *  Core read handler
 *
 *************************************/

READ16_HANDLER( smc91c94_r )
{
	UINT32 result = ~0;

	/* determine the effective register */
	offset %= 8;
	if (offset != EREG_BANK)
		offset += 8 * (ethernet.reg[EREG_BANK] & 7);
	result = ethernet.reg[offset];

	switch (offset)
	{
		case EREG_MMU_COMMAND:
			ethernet.reg[EREG_MMU_COMMAND] &= ~0x0001;
			break;

		case EREG_PNR_ARR:
			if (!(mem_mask & 0xff00))
			{
				ethernet.reg[EREG_INTERRUPT] &= ~0x0008;
				update_ethernet_irq();
			}
			break;

		case EREG_FIFO_PORTS:
			result = ethernet.comp_tx_count ? ethernet.comp_tx[0] : 0x80;
			result |= (ethernet.comp_rx_count ? ethernet.comp_rx[0] : 0x80) << 8;
			break;

		case EREG_DATA_0:	/* data register */
		case EREG_DATA_1:	/* data register */
		{
			UINT8 *buffer = packet_buffer();
			int addr = ethernet.reg[EREG_POINTER] & 0x7ff;
			result = buffer[addr++];
			if (!(mem_mask & 0xff00))
				result |= buffer[addr++] << 8;
			if (ethernet.reg[EREG_POINTER] & 0x4000)
				ethernet.reg[EREG_POINTER] = (ethernet.reg[EREG_POINTER] & ~0x7ff) | (addr & 0x7ff);
			break;
		}
	}

	if (LOG_ETHERNET && offset != EREG_BANK)
		logerror("%08X:ethernet_r(%s) = %04X & %04X\n", activecpu_get_pc(), ethernet_regname[offset], result, mem_mask ^ 0xffff);
	return result;
}



/*************************************
 *
 *  Core write handler
 *
 *************************************/

WRITE16_HANDLER( smc91c94_w )
{
	UINT16 olddata;

	/* determine the effective register */
	offset %= 8;
	if (offset != EREG_BANK)
		offset += 8 * (ethernet.reg[EREG_BANK] & 7);

	/* update the data generically */
	olddata = ethernet.reg[offset];
	mem_mask |= ~ethernet.regmask[offset];
	COMBINE_DATA(&ethernet.reg[offset]);

	if (LOG_ETHERNET && offset != 7)
		logerror("%08X:ethernet_w(%s) = %04X & %04X\n", activecpu_get_pc(), ethernet_regname[offset], data, mem_mask ^ 0xffff);

	/* handle it */
	switch (offset)
	{
		case EREG_TCR:		/* transmit control register */
			if (LOG_ETHERNET)
			{
				if (data & 0x2000) logerror("   EPH LOOP\n");
				if (data & 0x1000) logerror("   STP SQET\n");
				if (data & 0x0800) logerror("   FDUPLX\n");
				if (data & 0x0400) logerror("   MON_CSN\n");
				if (data & 0x0100) logerror("   NOCRC\n");
				if (data & 0x0080) logerror("   PAD_EN\n");
				if (data & 0x0004) logerror("   FORCOL\n");
				if (data & 0x0002) logerror("   LOOP\n");
				if (data & 0x0001) logerror("   TXENA\n");
			}
			break;

		case EREG_RCR:		/* receive control register */
			if (data & 0x8000)
				smc91c94_reset();
			else if ((olddata & 0x0100) && !(data & 0x0100))
			{
				/* disabling the receiver drops the received packets */
				ethernet.comp_rx_count = 0;
				update_ethernet_irq();
			}
			if (LOG_ETHERNET)
			{
				if (data & 0x8000) logerror("   SOFT RST\n");
				if (data & 0x4000) logerror("   FILT_CAR\n");
				if (data & 0x0200) logerror("   STRIP CRC\n");
				if (data & 0x0100) logerror("   RXEN\n");
				if (data & 0x0004) logerror("   ALMUL\n");
				if (data & 0x0002) logerror("   PRMS\n");
				if (data & 0x0001) logerror("   RX_ABORT\n");
			}
			break;

		case EREG_CONFIG:		/* configuration register */
			if (LOG_ETHERNET)
			{
				if (data & 0x1000) logerror("   NO WAIT\n");
				if (data & 0x0400) logerror("   FULL STEP\n");
				if (data & 0x0200) logerror("   SET SQLCH\n");
				if (data & 0x0100) logerror("   AUI SELECT\n");
				if (data & 0x0080) logerror("   16 BIT\n");
				if (data & 0x0040) logerror("   DIS LINK\n");
				if (data & 0x0004) logerror("   INT SEL1\n");
				if (data & 0x0002) logerror("   INT SEL0\n");
			}
			break;

		case EREG_BASE:		/* base address register */
			if (LOG_ETHERNET)
			{
				logerror("   base = $%04X\n", (data & 0xe000) | ((data & 0x1f00) >> 3));
				logerror("   romsize = %d\n", ((data & 0xc0) >> 6));
				logerror("   romaddr = $%05X\n", ((data & 0x3e) << 13));
			}
			break;

		case EREG_CONTROL:		/* control register */
			if (LOG_ETHERNET)
			{
				if (data & 0x4000) logerror("   RCV_BAD\n");
				if (data & 0x2000) logerror("   PWRDN\n");
				if (data & 0x0800) logerror("   AUTO RELEASE\n");
				if (data & 0x0080) logerror("   LE ENABLE\n");
				if (data & 0x0040) logerror("   CR ENABLE\n");
				if (data & 0x0020) logerror("   TE ENABLE\n");
				if (data & 0x0004) logerror("   EEPROM SELECT\n");
				if (data & 0x0002) logerror("   RELOAD\n");
				if (data & 0x0001) logerror("   STORE\n");
			}
			break;

		case EREG_MMU_COMMAND:	/* command register */
			process_command(data);
			break;

		case EREG_DATA_0:	/* data register */
		case EREG_DATA_1:	/* data register */
		{
			UINT8 *buffer = packet_buffer();
			int addr = ethernet.reg[EREG_POINTER] & 0x7ff;
			buffer[addr++] = data;
			if (!(mem_mask & 0xff00))
				buffer[addr++] = data >> 8;
			if (ethernet.reg[EREG_POINTER] & 0x4000)
				ethernet.reg[EREG_POINTER] = (ethernet.reg[EREG_POINTER] & ~0x7ff) | (addr & 0x7ff);
			break;
		}

		case EREG_INTERRUPT:
			/* acknowledging the tx interrupt removes the packet from the completion fifo */
			if (data & EINT_TX)
				pop_fifo(ethernet.comp_tx, &ethernet.comp_tx_count);
			ethernet.reg[EREG_INTERRUPT] &= ~(data & (EINT_ERCV | EINT_RX_OVRN | EINT_TX_EMPTY | EINT_TX));
			update_ethernet_irq();
			break;
	}
}
