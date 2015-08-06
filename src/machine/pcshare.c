/***************************************************************************

    machine/pc.c

    Functions to emulate general aspects of the machine
    (RAM, ROM, interrupts, I/O ports)

    The information herein is heavily based on
    'Ralph Browns Interrupt List'
    Release 52, Last Change 20oct96

    TODO:
    clean up (maybe split) the different pieces of hardware
    PIC, PIT, DMA... add support for LPT, COM (almost done)
    emulation of a serial mouse on a COM port (almost done)
    support for Game Controller port at 0x0201
    support for XT harddisk (under the way, see machine/pc_hdc.c)
    whatever 'hardware' comes to mind,
    maybe SoundBlaster? EGA? VGA?

***************************************************************************/

#include "driver.h"
#include "memconv.h"
#include "machine/8255ppi.h"

#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/mc146818.h"
#include "machine/pcshare.h"

#include "machine/8237dma.h"
#include "machine/pckeybrd.h"

#define VERBOSE_DBG 0       /* general debug messages */
#if VERBOSE_DBG
#define DBG_LOG(N,M,A) \
	if(VERBOSE_DBG>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(n,m,a)
#endif

#define VERBOSE_JOY 0		/* JOY (joystick port) */

#if VERBOSE_JOY
#define LOG(LEVEL,N,M,A)  \
#define JOY_LOG(N,M,A) \
	if(VERBOSE_JOY>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define JOY_LOG(n,m,a)
#endif

#define FDC_DMA 2


static mame_timer *pc_keyboard_timer;

static void pc_keyb_timer(int param);

#define LOG_PORT80 0



/* ---------------------------------------------------------------------- */

static void pc_timer0_w(int state)
{
	pic8259_set_irq_line(0, 0, state);
}



/*
 * timer0   heartbeat IRQ
 * timer1   DRAM refresh (ignored)
 * timer2   PIO port C pin 4 and speaker polling
 */
static const struct pit8253_config pc_pit8253_config =
{
	TYPE8253,
	{
		{
			4772720/4,				/* heartbeat IRQ */
			pc_timer0_w,
			NULL
		}, {
			4772720/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			NULL //pc_sh_speaker_change_clock
		}
	}
};

static const struct pit8253_config pc_pit8254_config =
{
	TYPE8254,
	{
		{
			4772720/4,				/* heartbeat IRQ */
			pc_timer0_w,
			NULL
		}, {
			4772720/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4772720/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			NULL //pc_sh_speaker_change_clock
		}
	}
};

/*************************************************************************
 *
 *      PC DMA stuff
 *
 *************************************************************************/

static UINT8 dma_offset[2][4];
static UINT8 at_pages[0x10];
static offs_t pc_page_offset_mask;



READ8_HANDLER(pc_page_r)
{
	return 0xFF;
}



WRITE8_HANDLER(pc_page_w)
{
	switch(offset % 4) {
	case 1:
		dma_offset[0][2] = data;
		break;
	case 2:
		dma_offset[0][3] = data;
		break;
	case 3:
		dma_offset[0][0] = dma_offset[0][1] = data;
		break;
	}
}



READ8_HANDLER(at_page8_r)
{
	UINT8 data = at_pages[offset % 0x10];

	switch(offset % 8) {
	case 1:
		data = dma_offset[(offset / 8) & 1][2];
		break;
	case 2:
		data = dma_offset[(offset / 8) & 1][3];
		break;
	case 3:
		data = dma_offset[(offset / 8) & 1][1];
		break;
	case 7:
		data = dma_offset[(offset / 8) & 1][0];
		break;
	}
	return data;
}



WRITE8_HANDLER(at_page8_w)
{
	at_pages[offset % 0x10] = data;

	if (LOG_PORT80 && (offset == 0))
		logerror(" at_page8_w(): Port 80h <== 0x%02x (PC=0x%08x)\n", data, (unsigned) activecpu_get_reg(REG_PC));

	switch(offset % 8) {
	case 1:
		dma_offset[(offset / 8) & 1][2] = data;
		break;
	case 2:
		dma_offset[(offset / 8) & 1][3] = data;
		break;
	case 3:
		dma_offset[(offset / 8) & 1][1] = data;
		break;
	case 7:
		dma_offset[(offset / 8) & 1][0] = data;
		break;
	}
}



READ32_HANDLER(at_page32_r)
{
	return read32le_with_read8_handler(at_page8_r, offset, mem_mask);
}



WRITE32_HANDLER(at_page32_w)
{
	write32le_with_write8_handler(at_page8_w, offset, data, mem_mask);
}



static UINT8 pc_dma_read_byte(int channel, offs_t offset)
{
	UINT8 result;
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& pc_page_offset_mask;

	cpuintrf_push_context(0);
	result = program_read_byte(page_offset + offset);
	cpuintrf_pop_context();

	return result;
}



static void pc_dma_write_byte(int channel, offs_t offset, UINT8 data)
{
	offs_t page_offset = (((offs_t) dma_offset[0][channel]) << 16)
		& pc_page_offset_mask;

	cpuintrf_push_context(0);
	program_write_byte(page_offset + offset, data);
	cpuintrf_pop_context();
}



static struct dma8237_interface pc_dma =
{
	0,
	TIME_IN_USEC(1),

	pc_dma_read_byte,
	pc_dma_write_byte,

	//{ 0, 0, pc_fdc_dack_r, pc_hdc_dack_r },
	//{ 0, 0, pc_fdc_dack_w, pc_hdc_dack_w },
	//pc_fdc_set_tc_state
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	0
};



/* ----------------------------------------------------------------------- */

static void pc_pic_set_int_line(int which, int interrupt)
{
	switch(which)
	{
		case 0:
			/* Master */
			cpunum_set_input_line(0, 0, interrupt ? HOLD_LINE : CLEAR_LINE);
			break;

		case 1:
			/* Slave */
			pic8259_set_irq_line(0, 2, interrupt);
			break;
	}
}



void init_pc_common(UINT32 flags)
{
	/* PIT */
	if (flags & PCCOMMON_TIMER_8254)
		pit8253_init(1, &pc_pit8254_config);
	else if (flags & PCCOMMON_TIMER_8253)
		pit8253_init(1, &pc_pit8253_config);

	/* PC-XT keyboard */
	if (flags & PCCOMMON_KEYBOARD_AT)
		at_keyboard_init(AT_KEYBOARD_TYPE_AT);
	else
		at_keyboard_init(AT_KEYBOARD_TYPE_PC);
	at_keyboard_set_scan_code_set(1);

	/* PIC */
	pic8259_init(2, pc_pic_set_int_line);

	/* DMA */
	if (flags & PCCOMMON_DMA8237_AT)
	{
		dma8237_init(2);
		dma8237_config(0, &pc_dma);
		pc_page_offset_mask = 0xFF0000;
	}
	else
	{
		dma8237_init(1);
		dma8237_config(0, &pc_dma);
		pc_page_offset_mask = 0x0F0000;
	}

	pc_keyboard_timer = mame_timer_alloc(pc_keyb_timer);
}

/*
   keyboard seams to permanently sent data clocked by the mainboard
   clock line low for longer means "resync", keyboard sends 0xaa as answer
   will become automatically 0x00 after a while
*/

static struct {
	UINT8 data;
	int on;
} pc_keyb= { 0 };

UINT8 pc_keyb_read(void)
{
	return pc_keyb.data;
}



static void pc_keyb_timer(int param)
{
	at_keyboard_reset();
	pc_keyboard();
}



void pc_keyb_set_clock(int on)
{
	mame_time keyb_delay = double_to_mame_time(1/200.0);

	on = on ? 1 : 0;

	if (pc_keyb.on != on)
	{
		if (on)
			mame_timer_adjust(pc_keyboard_timer, keyb_delay, 0, time_zero);
		else
			mame_timer_reset(pc_keyboard_timer, time_never);

		pc_keyb.on = on;
	}
}

void pc_keyb_clear(void)
{
	pc_keyb.data = 0;
}

void pc_keyboard(void)
{
	int data;

	at_keyboard_polling();

	if (pc_keyb.on)
	{
		if ( (data=at_keyboard_read())!=-1) {
			pc_keyb.data = data;
			DBG_LOG(1,"KB_scancode",("$%02x\n", pc_keyb.data));
			pic8259_set_irq_line(0, 1, 1);
			pic8259_set_irq_line(0, 1, 0);
		}
	}
}
