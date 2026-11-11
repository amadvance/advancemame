// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************
    Bell-Fruit/ATD RasterSpeed hardware
    Rise of the Robots and Football Crazy

    Development status (2026-11-11):

    All three sets are registered but remain NOT_WORKING. Gameplay and sound
    have not been validated. This is an unfinished implementation checkpoint.

    Implemented so far: shared i486/TMS320C31 memory, DSP bootstrap ROM access,
    display lists and palettes, serial DAC output, RTC/NVRAM, a partial NCR700
    SCRIPTS interpreter, and provisional SCC/trackball/BACTA handling. The DSP
    interrupt latch and SCSI READ/WRITE(10) support also required core changes.
    Football Crazy additionally exercises FNOP, FILD16, FMUL64 and FISTP64;
    their current x87 implementations do not constitute complete FPU support.

    Rise of the Robots reaches the credits. Earlier tests stopped in the SCSI
    wait loop at 0x1e09d after asynchronous reads. Comparing completion flags
    and instruction traces showed completion could precede entry to that loop.
    Adding transfer latency alone did not fix it. Replacing the host's full
    slice yield with a timed one-microsecond spin gets beyond that particular
    wait, but currently leaves a mostly black display with the credit counter.
    The video reader reaches its timeout/key-wait path at 0x2fa73/0x287ef.
    The older set reached the credits with the earlier implementation and
    still needs to be retested after the latest timing changes.

    Useful rotr diagnostics: pending flag 0x1d3e8, completion flag 0x1e09c,
    asynchronous busy flag 0x1db42; completion handler 0x1e519-0x1e543.
    The large READ(10) at LBA 0x2de23 requests 0x0b00 sectors. Compare the video
    ring at 0x691e8 and its read/write pointers with execution around 0x2fa40.
    Selected buffer samples matched the reference, but full transfer timing,
    residual registers and subsequent video stream processing remain unproven.

    Football Crazy loads from CD and passes the formerly unimplemented x87
    instructions, then displays "Ram error - All nvr cleared". Initializing
    its NVRAM to zero rather than 0xff did not resolve this. A clean reference
    run reaches the Bell-Fruit introduction. Investigate NVRAM accesses,
    checksums and the startup error/acknowledgement path before blaming saved
    settings; repeat with fresh NVRAM and configuration files.

    Remaining limitations include provisional SCSI phase/register semantics,
    incomplete UART interrupt behavior and volume control, reset/save-state
    coverage, and unverified audio. Builds and automatic PNG/WAV captures have
    been exercised, but none of these sets can yet be considered playable.
****************************************************************************/
#include "driver.h"
#include "cpu/i386/i386.h"
#include "cpu/tms32031/tms32031.h"
#include "machine/mc146818.h"
#include "machine/scsidev.h"
#include "cdrom.h"
#include "sound/dac.h"

static UINT32 *m_dram;
static UINT8 m_nvram8[0x8000], m_irq_status;
static UINT32 m_dlba, m_speedup_count, m_tms_io_regs[0x80];
static UINT16 m_paletteram[0x10000];
static UINT8 m_palette_number, m_dsp_ctrl_data, m_aux_port3_data, m_interrupt_mask;
static int m_is_football;
static mame_bitmap *m_update_bitmap;
static void *m_tms_timer1, *m_tms_tx_timer;
static void update_irq(UINT32 which, UINT32 state);
static void upload_palette(UINT32 word1, UINT32 word2);
static void scsi_init(void);
static READ32_HANDLER(scsi_r);
static READ32_HANDLER(uart_r);
static WRITE32_HANDLER(uart_w);
static void uart_init(void);
static WRITE32_HANDLER(scsi_w);
static READ32_HANDLER(rtc_r) { return mc146818_port_r(offset); }
static WRITE32_HANDLER(rtc_w) { if (!(mem_mask & 0xff)) mc146818_port_w(offset, data); }

#define SOUND_CLOCK             12288000
#define PLL_CLOCK               14318181
#define NVRAM_SIZE              0x8000
#define VIDEO_ADDR_MASK 0x3fffffff

#define USE_SPEEDUP_HACK        1

enum tms_regs
{
	DMA_GLOBAL_CTL      =   0x00,
	DMA_SOURCE_ADDR     =   0x04,
	DMA_DEST_ADDR       =   0x06,
	DMA_TRANSFER_COUNT  =   0x08,

	TIMER0_GLOBAL_CTL   =   0x20,
	TIMER0_COUNTER      =   0x24,
	TIMER0_PERIOD       =   0x28,

	TIMER1_GLOBAL_CTL   =   0x30,
	TIMER1_COUNTER      =   0x34,
	TIMER1_PERIOD       =   0x38,

	SPORT_GLOBAL_CTL    =   0x40,
	SPORT_TX_CTL        =   0x42,
	SPORT_RX_CTL        =   0x43,
	SPORT_TIMER_CTL     =   0x44,
	SPORT_TIMER_COUNTER =   0x45,
	SPORT_TIMER_PERIOD  =   0x46,
	SPORT_DATA_TX       =   0x48,
	SPORT_DATA_RX       =   0x4c,
};

enum irq_status
{
	IRQ_RTC     = 1,
	IRQ_UART    = 2,
	IRQ_DSP     = 4,
	IRQ_VBLANK  = 5,
	IRQ_SCSI    = 3,
};

static int irq_callback(int irqline);
static MACHINE_RESET( rastersp )
{
	memory_set_bankptr(1, m_dram);
	memory_set_bankptr(2, &m_dram[0x10000/4]);
	memory_set_bankptr(3, m_dram);
	cpunum_set_irq_callback(0, irq_callback);
	m_irq_status = 0;
	m_dsp_ctrl_data = 0;
	timer_adjust(m_tms_tx_timer, TIME_NEVER, 0, 0);
	timer_adjust(m_tms_timer1, TIME_NEVER, 0, 0);
	m_dlba = 0;

	cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);

	cpunum_set_input_line(1, TMS32031_IRQ1, ASSERT_LINE);
	cpunum_set_input_line(1, TMS32031_IRQ1, CLEAR_LINE);

	m_tms_io_regs[SPORT_GLOBAL_CTL] = 0;
}

VIDEO_START( rastersp )
{
	m_update_bitmap = auto_bitmap_alloc(320, 240);
	return 0;
}

static WRITE32_HANDLER( dpylist_w )
{
	UINT32 word1 = 0;
	m_dlba = data;

	if (!((m_dlba >> 7) & 1))
	{
		fillbitmap(m_update_bitmap, 0, 0);
		return;
	}

	UINT32 dpladdr = (m_dlba & ~0xff) >> 6;

	int y = 0;
	int x = 0;
	UINT16 *bmpptr = (UINT16 *)m_update_bitmap->line[0];

	while (y < 240)
	{
		UINT32 word2 = m_dram[dpladdr/4];
		dpladdr += 4;

		if (!(word2 & 0x80000000))
		{

			word1 = word2;
		}
		else
		{

			if (word2 & 0x40000000)
			{
				m_palette_number &= 0x0f;
				m_palette_number |= ((word2 >> 8) & 0xf0);
			}
			else
			{
				m_palette_number &= 0xf0;
				m_palette_number |= ((word2 >> 12) & 0x0f);
			}

			if ((word2 & 0xbe000000) == 0x94000000)
			{

				UINT32 srcaddr = word1 >> 8;
				UINT32 pixels = (word2 >> 16) & 0x1ff;

				UINT16* palptr = &m_paletteram[m_palette_number*256];
				UINT8* srcptr = (UINT8 *)m_dram;

				UINT32 acc = srcaddr << 8;

				INT32 incr = word2 & 0xfff;

				incr |= ~((incr & 0x800) - 1) & ~0xff;

				while (y < 240 && pixels)
				{
					while (x < 320 && pixels)
					{
						*bmpptr++ = palptr[srcptr[BYTE_XOR_LE(acc >> 8)]];
						acc = (acc + incr) & VIDEO_ADDR_MASK;

						--pixels;
						++x;
					}

					if (x >= 320)
					{
						x = 0;
						++y;
						if (y < 240) bmpptr = (UINT16 *)m_update_bitmap->line[y];
					}
				}
			}
			else if ((word2 & 0xbe000000) == 0x84000000)
			{

				UINT32 srcaddr = word1 >> 8;
				UINT32 pixels = (word2 >> 16) & 0x1ff;

				UINT16* srcptr = (UINT16 *)m_dram;

				UINT32 acc = srcaddr << 8;

				INT32 incr = word2 & 0xfff;

				incr |= ~((incr & 0x800) - 1) & ~0xff;

				while (y < 240 && pixels)
				{
					while (x < 320 && pixels)
					{
						*bmpptr++ = srcptr[WORD_XOR_LE(acc >> 9)];
						acc = (acc + incr) & VIDEO_ADDR_MASK;

						--pixels;
						++x;
					}

					if (x >= 320)
					{
						x = 0;
						++y;
						if (y < 240) bmpptr = (UINT16 *)m_update_bitmap->line[y];
					}
				}
			}
			else if ((word2 & 0xbe000000) == 0x8c000000)
			{
				upload_palette(word1, word2);
			}
			else
			{
				fatalerror("Unknown control words! (%x, %x)\n", word1, word2);
			}
		}
	}
}

static void upload_palette(UINT32 word1, UINT32 word2)
{

	if (word1 & 3)
		fatalerror("Unalligned palette address! (%x, %x)\n", word1, word2);

	UINT32 addr = word1 >> 8;
	UINT32 entries = (word2 >> 16) & 0x1ff;
	UINT32 index = m_palette_number * 256;

	while (entries--)
	{
		UINT32 data = m_dram[addr / 4];
		m_paletteram[index + ((data >> 16) & 0xff)] = data & 0xffff;
		addr += 4;
	}
}

VIDEO_UPDATE( rastersp )
{
	copybitmap(bitmap, m_update_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
}

static int irq_callback(int irqline)
{
	UINT8 vector = 0;

	if (m_irq_status & (1 << IRQ_UART))
	{ vector = 10; }
	else if (m_irq_status & (1 << IRQ_SCSI))
	{
		vector = 11;
	}
	else if (m_irq_status & (1 << IRQ_DSP))
	{
		update_irq(IRQ_DSP, CLEAR_LINE);
		vector = 12;
	}
	else if (m_irq_status & (1 << IRQ_VBLANK))
	{
		update_irq(IRQ_VBLANK, CLEAR_LINE);
		vector = 13;
	}
	else
	{
		fatalerror("Unknown x86 IRQ (m_irq_status = %x)", m_irq_status);
	}

	return vector;
}

static void update_irq(UINT32 which, UINT32 state)
{
	UINT32 mask = 1 << which;

	if (state)
		m_irq_status |= mask;
	else
		m_irq_status &= ~mask;

	cpunum_set_input_line(0, 0, m_irq_status ? HOLD_LINE : CLEAR_LINE);
}

static void scsi_irq(int state)
{
	update_irq(IRQ_SCSI, state);
	if (state && (m_dsp_ctrl_data & 0x20)) {
		cpunum_set_input_line(1, TMS32031_IRQ0, ASSERT_LINE);
		cpunum_set_input_line(1, TMS32031_IRQ0, CLEAR_LINE);

	}
}

static INTERRUPT_GEN( vblank_irq )
{
	update_irq(IRQ_VBLANK, ASSERT_LINE);
}

static WRITE32_HANDLER( port1_w )
{

	if ((data & 0x20) && (m_dsp_ctrl_data & 0x20))
	{
		cpunum_set_input_line(1, TMS32031_IRQ2, ASSERT_LINE);
		cpunum_set_input_line(1, TMS32031_IRQ2, CLEAR_LINE);
		/* The host polls its reply before a normal scheduling slice expires. */
		cpu_boost_interleave(TIME_IN_USEC(1), TIME_IN_MSEC(1));
		cpu_spinuntil_time(TIME_IN_USEC(1));

	}
}

static WRITE32_HANDLER( port2_w )
{

}

static WRITE32_HANDLER( port3_w )
{

}

static WRITE32_HANDLER( nvram_w )
{
	if (mem_mask & 0xff) return;
	offset *= 4;

	if ((offset & 0xc000) || !(offset & 0x2000))
	{
		logerror("Unmapped NVRAM write to offset: %x", offset);
	}

	offset &= ~0x2000;

	UINT32 addr = ((offset & 0x00f0000) >> 5) | ((offset & 0x1fff) / 4);

	m_nvram8[addr] = data & 0xff;
}

static READ32_HANDLER( nvram_r )
{
	offset *= 4;

	if ((offset & 0xc000) || !(offset & 0x2000))
	{
		logerror("Unmapped NVRAM read from offset: %x", offset);
	}

	offset &= ~0x2000;

	UINT32 addr = ((offset & 0x00f0000) >> 5) | ((offset & 0x1fff) / 4);

	return m_nvram8[addr];
}

static WRITE32_HANDLER( cyrix_cache_w )
{

}

static void tms_tx_timer(int param)
{

	if (m_tms_io_regs[SPORT_GLOBAL_CTL] & (1 << 3))
	{
		UINT32 data = m_tms_io_regs[SPORT_DATA_TX];

		INT16 ldata = data & 0xffff;
		INT16 rdata = data >> 16;

		DAC_signed_data_16_w(0, 0x8000 + ldata);
		DAC_signed_data_16_w(1, 0x8000 + rdata);
	}

	m_tms_io_regs[SPORT_GLOBAL_CTL] &= ~(1 << 3);

	m_tms_io_regs[SPORT_GLOBAL_CTL] |= (1 << 1);

	if (m_tms_io_regs[SPORT_GLOBAL_CTL] & (1 << 23))
	{
		cpunum_set_input_line(1, TMS32031_XINT0, ASSERT_LINE);
		cpunum_set_input_line(1, TMS32031_XINT0, CLEAR_LINE);
	}
}

static void tms_timer1(int param)
{
}

static READ32_HANDLER( tms32031_control_r )
{
	UINT32 val = m_tms_io_regs[offset];

	switch (offset)
	{
		case TIMER1_COUNTER:
		{
			double elapsed = timer_timeelapsed(m_tms_timer1);
			val = m_tms_io_regs[TIMER1_PERIOD] - (UINT32)(elapsed * 33330000 / 4);

			break;
		}
		default:
		{
			logerror("TMS32031: Unhandled I/O read: %x\n", offset);
		}
	}

	return val;
}

static WRITE32_HANDLER( tms32031_control_w )
{
	UINT32 old = m_tms_io_regs[offset];

	m_tms_io_regs[offset] = data;

	switch (offset)
	{
		case TIMER1_GLOBAL_CTL:
		{

			double period = TIME_IN_HZ(33330000 / 4);

			period *= m_tms_io_regs[TIMER1_PERIOD];

			timer_adjust(m_tms_timer1, period, 0, period);

			break;
		}
		case SPORT_GLOBAL_CTL:
		{
			if (!(data & (1 << 26)))
			{

				timer_adjust(m_tms_tx_timer, TIME_NEVER, 0, 0);
			}
			else if (!(old & (1 << 26)) && (data & (1 << 26)))
			{

				double period = TIME_IN_HZ(SOUND_CLOCK / 512);

				m_tms_io_regs[SPORT_GLOBAL_CTL] |= (1 << 1);

				m_tms_io_regs[SPORT_GLOBAL_CTL] &= ~(1 << 3);

				timer_adjust(m_tms_tx_timer, period, 0, period);
			}

			break;
		}
		case SPORT_DATA_TX:
		{

			m_tms_io_regs[SPORT_GLOBAL_CTL] &= ~(1 << 1);

			m_tms_io_regs[SPORT_GLOBAL_CTL] |= (1 << 3);

			break;
		}
		default:
		{
			logerror("TMS32031: Unhandled I/O write: %x %x\n", offset, data);
		}
	}
}

static WRITE32_HANDLER( dsp_unk_w )
{

}

static WRITE32_HANDLER( dsp_486_int_w )
{
	update_irq(IRQ_DSP, ASSERT_LINE);
}

static WRITE32_HANDLER( dsp_ctrl_w )
{
	m_dsp_ctrl_data = data;

	cpunum_set_input_line(0, INPUT_LINE_RESET, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE32_HANDLER( dsp_speedup_w )
{

	if (activecpu_get_pc() == 0x809c23)
	{
		INT32 cycles_left = activecpu_get_icount();
		data += cycles_left / 6;
		cpu_spinuntil_int();
	}

	m_speedup_count = data;
}

static READ32_HANDLER( dsp_speedup_r )
{
	return m_speedup_count;
}

static READ32_HANDLER(aux_r)
{
	if (!m_is_football) return 0xffffffff;
	if (offset == 0) return readinputportbytag("AUX_PORT0");
	if (offset == 1) return (readinputportbytag("AUX_PORT1") & ~0x40) | ((m_aux_port3_data & 0xbc) ? 0x40 : 0);
	return 0xffffffff;
}
static WRITE32_HANDLER(aux_w)
{
	if (offset == 3) m_aux_port3_data = data;
}

static READ32_HANDLER(interrupt_ctrl_r) { return m_interrupt_mask; }
static WRITE32_HANDLER(interrupt_ctrl_w) { if (!(mem_mask & 0xff)) m_interrupt_mask = data; }
static READ32_HANDLER(interrupt_status_r) { return 0; }

static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&m_dram)
	AM_RANGE(0x01000000, 0x01000013) AM_READWRITE(aux_r, aux_w)
	AM_RANGE(0x01000014, 0x010bffff) AM_NOP
	AM_RANGE(0x010c0000, 0x010cffff) AM_NOP
	AM_RANGE(0x02200800, 0x02200803) AM_READWRITE(interrupt_ctrl_r, interrupt_ctrl_w)
	AM_RANGE(0x02208000, 0x02208fff) AM_READWRITE(scsi_r, scsi_w)
	AM_RANGE(0x0220e000, 0x0220e003) AM_WRITE(dpylist_w)
	AM_RANGE(0x02200000, 0x022fffff) AM_READWRITE(nvram_r, nvram_w)
	AM_RANGE(0xfff00000, 0xffffffff) AM_READWRITE(MRA32_BANK3, MWA32_BANK3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( football_cpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM AM_BASE(&m_dram)
	AM_RANGE(0x01000000, 0x01000013) AM_READWRITE(aux_r, aux_w)
	AM_RANGE(0x01000014, 0x010bffff) AM_NOP
	AM_RANGE(0x010c0000, 0x010cffff) AM_NOP
	AM_RANGE(0x02200800, 0x02200803) AM_READWRITE(interrupt_ctrl_r, interrupt_ctrl_w)
	AM_RANGE(0x02208000, 0x02208fff) AM_READWRITE(scsi_r, scsi_w)
	AM_RANGE(0x0220e000, 0x0220e003) AM_WRITE(dpylist_w)
	AM_RANGE(0x02200000, 0x022fffff) AM_READWRITE(nvram_r, nvram_w)
	AM_RANGE(0xfff00000, 0xffffffff) AM_READWRITE(MRA32_BANK3, MWA32_BANK3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x0000, 0x0003) AM_READ(interrupt_status_r)
	AM_RANGE(0x0020, 0x0023) AM_WRITE(cyrix_cache_w)
	AM_RANGE(0x1000, 0x1003) AM_READ(input_port_0_dword_r) AM_WRITE(port1_w)
	AM_RANGE(0x1004, 0x1007) AM_READ(input_port_1_dword_r) AM_WRITE(port2_w)
	AM_RANGE(0x1008, 0x100b) AM_READ(input_port_2_dword_r) AM_WRITE(port3_w)
	AM_RANGE(0x100c, 0x100f) AM_READ(input_port_5_dword_r)
	AM_RANGE(0x1010, 0x1013) AM_READ(input_port_4_dword_r)
	AM_RANGE(0x1014, 0x1017) AM_READ(input_port_3_dword_r)
	AM_RANGE(0x4000, 0x4007) AM_READWRITE(rtc_r, rtc_w)
	AM_RANGE(0x6000, 0x600f) AM_READWRITE(uart_r, uart_w)
ADDRESS_MAP_END

static WRITE32_HANDLER(dsp_low_w)
{
	COMBINE_DATA(&m_dram[offset]);
}

static ADDRESS_MAP_START( dsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	/* The internal boot ROM overlays reads while writes still reach shared DRAM. */
	AM_RANGE(0x000000, 0x000fff) AM_ROM AM_REGION(REGION_USER4, 0) AM_WRITE(dsp_low_w)
	AM_RANGE(0x0, 0xfffff) AM_READWRITE(MRA32_BANK1, MWA32_BANK1)
	AM_RANGE(0x400000, 0x40ffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x808000, 0x80807f) AM_READWRITE(tms32031_control_r, tms32031_control_w)
	AM_RANGE(0x880402, 0x880402) AM_WRITE(dsp_unk_w)
	AM_RANGE(0x883c00, 0x883c00) AM_WRITE(dsp_486_int_w)
	AM_RANGE(0xc00000, 0xc03fff) AM_READWRITE(MRA32_BANK2, MWA32_BANK2)
	AM_RANGE(0xc80000, 0xc80000) AM_WRITE(dsp_ctrl_w)
	AM_RANGE(0xfc0000, 0xffffff) AM_READWRITE(MRA32_BANK3, MWA32_BANK3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( football_dsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	/* The internal boot ROM overlays reads while writes still reach shared DRAM. */
	AM_RANGE(0x000000, 0x000fff) AM_ROM AM_REGION(REGION_USER4, 0) AM_WRITE(dsp_low_w)
	AM_RANGE(0x0, 0x1fffff) AM_READWRITE(MRA32_BANK1, MWA32_BANK1)
	AM_RANGE(0x400000, 0x40ffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x808000, 0x80807f) AM_READWRITE(tms32031_control_r, tms32031_control_w)
	AM_RANGE(0x880402, 0x880402) AM_WRITE(dsp_unk_w)
	AM_RANGE(0x883c00, 0x883c00) AM_WRITE(dsp_486_int_w)
	AM_RANGE(0xc00000, 0xc03fff) AM_READWRITE(MRA32_BANK2, MWA32_BANK2)
	AM_RANGE(0xc80000, 0xc80000) AM_WRITE(dsp_ctrl_w)
	AM_RANGE(0xfc0000, 0xffffff) AM_READWRITE(MRA32_BANK3, MWA32_BANK3)
ADDRESS_MAP_END

INPUT_PORTS_START( rotr )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START_TAG("COMMON")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("EXTRA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_PLAYER(2)

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Setup Disk" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )

	PORT_DIPNAME( 0x3c, 0x00, "Load PROG" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x14, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_DIPSETTING(    0x1c, "7" )
	PORT_DIPSETTING(    0x20, "8" )
	PORT_DIPSETTING(    0x24, "9" )
	PORT_DIPSETTING(    0x28, "A" )
	PORT_DIPSETTING(    0x2c, "B" )
	PORT_DIPSETTING(    0x30, "C" )
	PORT_DIPSETTING(    0x34, "D" )
	PORT_DIPSETTING(    0x38, "E" )
	PORT_DIPSETTING(    0x3c, "F" )

	PORT_DIPNAME( 0x40, 0x00, "Reserved" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_DIPNAME( 0x80, 0x00, "Enable Cache" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, "Debug Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	PORT_DIPNAME( 0x0e, 0x00, "FPGA File Source" )
	PORT_DIPSETTING(    0x00, "Serial PROMs" )
	PORT_DIPSETTING(    0x0e, "Cable" )

	PORT_DIPNAME( 0x70, 0x50, "Clock speed" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "8" )
	PORT_DIPSETTING(    0x20, "16" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPSETTING(    0x40, "25" )
	PORT_DIPSETTING(    0x50, "33" )
	PORT_DIPSETTING(    0x60, "40" )
	PORT_DIPSETTING(    0x70, "50" )

	PORT_DIPNAME( 0x80, 0x00, "SCSI bus terminated" )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
INPUT_PORTS_END

INPUT_PORTS_START( fbcrazy )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("COMMON")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )   PORT_NAME("Select") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("EXTRA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )   PORT_NAME("Refill Key")   PORT_CODE(KEYCODE_R) PORT_TOGGLE
	PORT_DIPNAME( 0x20, 0x00, "4 6 Cashbox Door" )
	PORT_DIPSETTING(    0x00, "CLOSED" )
	PORT_DIPSETTING(    0x20, "OPEN" )
	PORT_DIPNAME( 0x40, 0x00, "4 7 Back Door" )
	PORT_DIPSETTING(    0x00, "CLOSED" )
	PORT_DIPSETTING(    0x40, "OPEN" )
	PORT_DIPNAME( 0x80, 0x00, "4 8 Front Door" )
	PORT_DIPSETTING(    0x00, "CLOSED" )
	PORT_DIPSETTING(    0x80, "OPEN" )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Setup Disk" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )

	PORT_DIPNAME( 0x3c, 0x00, "Load PROG" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x14, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_DIPSETTING(    0x1c, "7" )
	PORT_DIPSETTING(    0x20, "8" )
	PORT_DIPSETTING(    0x24, "9" )
	PORT_DIPSETTING(    0x28, "A" )
	PORT_DIPSETTING(    0x2c, "B" )
	PORT_DIPSETTING(    0x30, "C" )
	PORT_DIPSETTING(    0x34, "D" )
	PORT_DIPSETTING(    0x38, "E" )
	PORT_DIPSETTING(    0x3c, "F" )

	PORT_DIPNAME( 0x40, 0x00, "Reserved" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )

	PORT_DIPNAME( 0x80, 0x00, "Enable Cache" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, "Debug Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	PORT_DIPNAME( 0x0e, 0x00, "FPGA File Source" )
	PORT_DIPSETTING(    0x00, "Serial PROMs" )
	PORT_DIPSETTING(    0x0e, "Cable" )

	PORT_DIPNAME( 0x70, 0x50, "Clock speed" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "8" )
	PORT_DIPSETTING(    0x20, "16" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPSETTING(    0x40, "25" )
	PORT_DIPSETTING(    0x50, "33" )
	PORT_DIPSETTING(    0x60, "40" )
	PORT_DIPSETTING(    0x70, "50" )

	PORT_DIPNAME( 0x80, 0x00, "SCSI bus terminated" )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )

	PORT_START_TAG("AUX_PORT0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(3) PORT_NAME("10p")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(3) PORT_NAME("20p")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(3) PORT_NAME("50p")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 ) PORT_IMPULSE(3) PORT_NAME("1 Pound")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN5 ) PORT_IMPULSE(3) PORT_NAME("5P")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Pay tube level") PORT_CODE(KEYCODE_T) PORT_TOGGLE
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("AUX_PORT1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x00, "Pay unit fitted" )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("TRACK_X")
	PORT_BIT( 0xfff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START_TAG("TRACK_Y")
	PORT_BIT( 0xfff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)
INPUT_PORTS_END

static UINT8 uart_regs[2][16], uart_pointer[2];
static UINT8 uart_fifo[2][16], uart_head[2], uart_count[2];

static void uart_irq(void)
{
	int pending = (uart_count[0] && (uart_regs[0][1] & 0x18)) ||
	    (uart_count[1] && (uart_regs[1][1] & 0x18));
	update_irq(IRQ_UART, pending && ((uart_regs[0][9] | uart_regs[1][9]) & 8));
}

static void uart_receive(int channel, UINT8 data)
{
	if (uart_count[channel] < 16)
	{
		uart_fifo[channel][(uart_head[channel] + uart_count[channel]) & 15] = data;
		++uart_count[channel];
	}
	uart_irq();
}

static void logger_ack(int param)
{
	uart_receive(0, 6);
}

static void trackball_tick(int param)
{
	static int oldx, oldy;
	int x = readinputportbytag("TRACK_X"), y = readinputportbytag("TRACK_Y");
	int dx = (x - oldx + 2048) % 4096 - 2048;
	int dy = (y - oldy + 2048) % 4096 - 2048;
	oldx = x;
	oldy = y;
	if (uart_regs[1][5] & 2)
	{
		dx = dx < -127 ? -127 : dx > 127 ? 127 : dx;
		dy = dy < -127 ? -127 : dy > 127 ? 127 : dy;
		uart_receive(1, 0x40 | ((dy >> 4) & 12) | ((dx >> 6) & 3));
		uart_receive(1, dx & 63);
		uart_receive(1, dy & 63);
	}
}

static READ32_HANDLER(uart_r)
{
	int channel = offset < 2 ? 1 : 0;
	UINT8 result = 0;
	if (offset & 1)
	{
		if (uart_count[channel])
		{
			result = uart_fifo[channel][uart_head[channel]];
			uart_head[channel] = (uart_head[channel] + 1) & 15;
			--uart_count[channel];
		}
	}
	else
	{
		int reg = uart_pointer[channel];
		uart_pointer[channel] = 0;
		if (reg == 0) result = 0x2c | (uart_count[channel] ? 1 : 0);
		else if (reg == 1) result = 1;
		else if (reg == 3) result = (uart_count[0] ? 0x20 : 0) | (uart_count[1] ? 4 : 0);
		else result = uart_regs[channel][reg];
	}
	uart_irq();
	return result;
}

static WRITE32_HANDLER(uart_w)
{
	int channel = offset < 2 ? 1 : 0;
	if (mem_mask & 0xff) return;
	if (offset & 1)
	{
		/* The statistics logger acknowledges each nonzero protocol byte. */
		if (channel == 0 && m_is_football && (data & 0xff)) timer_set(TIME_IN_HZ(120), 0, logger_ack);
	}
	else if (uart_pointer[channel])
	{
		int reg = uart_pointer[channel];
		uart_regs[channel][reg] = data;
		uart_pointer[channel] = 0;
		if (reg == 9)
		{
			int reset = data >> 6;
			if (reset == 3 || reset == (channel ? 1 : 2))
			{
				uart_head[channel] = uart_count[channel] = 0;
			}
		}
	}
	else
	{
		uart_pointer[channel] = (data & 7) | ((data & 0x18) == 8 ? 8 : 0);
	}
	uart_irq();
}

static void uart_init(void)
{
	memset(uart_regs, 0, sizeof(uart_regs));
	memset(uart_count, 0, sizeof(uart_count));
	memset(uart_head, 0, sizeof(uart_head));
	memset(uart_pointer, 0, sizeof(uart_pointer));
	if (m_is_football) timer_pulse(TIME_IN_HZ(32), 0, trackball_tick);
}

static UINT32 scsi_regs[16];
static void *scsi_disk;
static pSCSIDispatch scsi_dispatch;
static UINT8 scsi_phase, scsi_last;
static int scsi_remaining;
static int scsi_generation;
static UINT8 scsi_command[16];

static void scsi_update_irq(void)
{
	UINT8 dstat = scsi_regs[3], sstat = scsi_regs[3] >> 8;
	int pending = ((dstat & (scsi_regs[14] >> 8)) ? 1 : 0) |
	    ((sstat & (scsi_regs[0] >> 24)) ? 2 : 0);
	scsi_regs[8] = pending << 8;
	scsi_irq(pending != 0);
}

static void scsi_run(int param)
{
	int budget = 10000;
	if (param != scsi_generation) return;
	cpuintrf_push_context(0);
	while (--budget)
	{
		UINT32 pc = scsi_regs[11];
		UINT32 inst = program_read_dword(pc);
		UINT32 arg = program_read_dword(pc + 4);
		UINT32 count = inst & 0xffffff;
		UINT8 op = inst >> 24;

		int type = (op >> 3) & 7;
		scsi_regs[9] = inst;
		scsi_regs[10] = scsi_regs[12] = arg;
		scsi_regs[11] += 8;
		if ((op >> 6) == 0)
		{
			UINT8 buffer[0x10000];
			UINT32 i;
			if (op & 0x20) arg = program_read_dword(arg);
			if ((op & 7) != scsi_phase)
			{
				scsi_regs[3] |= 0x8000;
				break;
			}
			switch (scsi_phase)
			{
				case 6:
					scsi_phase = 2;
					break;
				case 2:
					memset(buffer, 0, 16);
					for (i = 0; i < count && i < 16; ++i) buffer[i] = program_read_byte(arg + i);
					memcpy(scsi_command, buffer, sizeof(scsi_command));
					scsi_remaining = scsi_dispatch(SCSIOP_EXEC_COMMAND, scsi_disk, 0, buffer);
					/* Commands without a data phase complete directly with status. */
					if (buffer[0] == 0 || buffer[0] == 0x1b) scsi_remaining = 0;
					if (buffer[0] == 0x12) scsi_remaining = buffer[4] < 36 ? buffer[4] : 36;
					if (buffer[0] == 0x1a) scsi_remaining = buffer[4] < 12 ? buffer[4] : 12;
					if (buffer[0] == 0x15) scsi_remaining = buffer[4];
					if (m_is_football && buffer[0] == 0x43) scsi_remaining = (buffer[7] << 8) | buffer[8];
					scsi_phase = scsi_remaining ? ((buffer[0] == 0x0a || buffer[0] == 0x2a || buffer[0] == 0x15) ? 0 : 1) : 3;
					/* Completion must follow the host's asynchronous command submission. */
					scsi_regs[9] &= 0xff000000;
					cpuintrf_pop_context();
					timer_set(TIME_IN_USEC(100 + scsi_remaining / 5), scsi_generation, scsi_run);
					return;
				case 0:
				case 1:
					if (!count) count = scsi_remaining;
					while (count)
					{
						UINT32 chunk = count > sizeof(buffer) ? sizeof(buffer) : count;
						if (scsi_phase == 1)
						{
							memset(buffer, 0, sizeof(buffer));
							if (scsi_command[0] == 0x1a)
							{
								buffer[0] = 11;
								buffer[3] = 8;
								buffer[10] = m_is_football ? 8 : 2;
							}
							else if (m_is_football && scsi_command[0] == 0x43)
							{
								cdrom_file *cd;
								UINT32 leadout;
								scsi_dispatch(SCSIOP_GET_DEVICE, scsi_disk, 0, (UINT8 *)&cd);
								leadout = cdrom_get_track_start(cd, 0xaa, 0);
								buffer[1] = 18;
								buffer[2] = buffer[3] = 1;
								buffer[5] = buffer[13] = 0x14;
								buffer[6] = 1;
								buffer[14] = 0xaa;
								buffer[16] = leadout >> 24;
								buffer[17] = leadout >> 16;
								buffer[18] = leadout >> 8;
								buffer[19] = leadout;
							}
							else scsi_dispatch(SCSIOP_READ_DATA, scsi_disk, chunk, buffer);
							for (i = 0; i < chunk; ++i) program_write_byte(arg + i, buffer[i]);
						}
						else
						{
							for (i = 0; i < chunk; ++i) buffer[i] = program_read_byte(arg + i);
							scsi_dispatch(SCSIOP_WRITE_DATA, scsi_disk, chunk, buffer);
						}
						scsi_last = buffer[chunk - 1];
						count -= chunk;
						arg += chunk;
						scsi_remaining -= chunk;
					}
					if (scsi_remaining <= 0) scsi_phase = 3;
					break;
				case 3:
					program_write_byte(arg, 0);
					scsi_last = 0;
					scsi_phase = 7;
					break;
				case 7:
					program_write_byte(arg, 0);
					scsi_last = 0;
					break;
			}
			scsi_regs[9] &= 0xff000000;
		}
		else if ((op >> 6) == 1)
		{
			if (type == 0)
			{
				scsi_phase = (op & 1) ? 6 : 2;
				scsi_regs[0] = (scsi_regs[0] & 0xff00ffff) | (inst & 0xff0000);
			}
			else if (type == 1) scsi_phase = 0;
			else if (type != 3 && type != 4)
			{
				scsi_regs[3] |= 1;
				break;
			}
		}
		else if ((op >> 6) == 2)
		{
			int jump = 1;
			if (inst & (1 << 17)) jump &= (op & 7) == scsi_phase;
			if (inst & (1 << 18)) jump &= (inst & 0xff) == scsi_last;
			if (!(inst & (1 << 19))) jump = !jump;
			if (jump)
			{
				if (type == 0) scsi_regs[11] = arg;
				else if (type == 1) { scsi_regs[7] = scsi_regs[11]; scsi_regs[11] = arg; }
				else if (type == 2) scsi_regs[11] = scsi_regs[7];
				else if (type == 3) { scsi_regs[3] |= 4; break; }
			}
		}
		else { scsi_regs[3] |= 1; break; }
	}
	cpuintrf_pop_context();
	if (!budget) timer_set(TIME_IN_USEC(100), scsi_generation, scsi_run);
	scsi_update_irq();
}

static READ32_HANDLER(scsi_r)
{
	UINT32 value = scsi_regs[offset & 15];
	if (offset == 2) value = (value & 0x00ffffff) | ((0xa0 | scsi_phase) << 24);
	if (offset == 3)
	{
		scsi_regs[3] &= mem_mask | 0xffff0080;
		scsi_update_irq();
	}
	return value;
}

static WRITE32_HANDLER(scsi_w)
{
	UINT32 reg = offset & 15;
	COMBINE_DATA(&scsi_regs[reg]);
	if (reg == 0 && (data & 0x20) && !(mem_mask & 0xff))
	{
		scsi_regs[0] &= ~0x20;
		scsi_regs[3] |= 0x1000;
	}
	if (reg == 14 && (scsi_regs[14] & 0x01000000))
	{
		++scsi_generation;
		memset(scsi_regs, 0, sizeof(scsi_regs));
		scsi_regs[3] = 0x80;
		scsi_regs[5] = 0x0021f000;
	}
	if (reg == 11 && !(mem_mask & 0xff000000)) { ++scsi_generation; timer_set(TIME_IN_USEC(10), scsi_generation, scsi_run); }
	scsi_update_irq();
}

static void scsi_init(void)
{
	scsi_dispatch = !strcmp(Machine->gamedrv->name, "fbcrazy") ? scsicd_dispatch : scsihd_dispatch;
	scsi_dispatch(SCSIOP_ALLOC_INSTANCE, &scsi_disk, 0, 0);
	++scsi_generation;
	memset(scsi_regs, 0, sizeof(scsi_regs));
	scsi_regs[3] = 0x80;
	scsi_regs[5] = 0x0021f000;
}

static NVRAM_HANDLER( rastersp )
{
	if (read_or_write) mame_fwrite(file, m_nvram8, sizeof(m_nvram8));
	else if (file) mame_fread(file, m_nvram8, sizeof(m_nvram8));
	else memcpy(m_nvram8, memory_region(REGION_USER3), sizeof(m_nvram8));
}

static PALETTE_INIT( rastersp )
{
	int i;
	for (i = 0; i < 65536; ++i)
		palette_set_color(i, pal5bit(i >> 11), pal6bit(i >> 5), pal5bit(i));
}

static struct tms32031_config tms_config = { 0x400000, 0, 0, 0 };
static MACHINE_DRIVER_START( rastersp )
	MDRV_CPU_ADD_TAG("maincpu", I486, 33330000)
	MDRV_CPU_PROGRAM_MAP(cpu_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)
	MDRV_CPU_ADD_TAG("dsp", TMS32031, 33330000)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_VBLANK_INT(vblank_irq, 1)
	MDRV_CPU_PROGRAM_MAP(dsp_map, 0)
	MDRV_MACHINE_RESET(rastersp)
	MDRV_NVRAM_HANDLER(rastersp)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(65536)
	MDRV_PALETTE_INIT(rastersp)
	MDRV_VIDEO_START(rastersp)
	MDRV_VIDEO_UPDATE(rastersp)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fbcrazy )
	MDRV_IMPORT_FROM(rastersp)
	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(football_cpu_map, 0)
	MDRV_CPU_MODIFY("dsp")
	MDRV_CPU_PROGRAM_MAP(football_dsp_map, 0)
MACHINE_DRIVER_END

static DRIVER_INIT( rastersp )
{
	memory_set_bankptr(1, m_dram);
	memory_set_bankptr(2, &m_dram[0x10000/4]);
	memory_set_bankptr(3, m_dram);
	m_is_football = !strcmp(Machine->gamedrv->name, "fbcrazy");
	uart_init();
	m_tms_timer1 = timer_alloc(tms_timer1);
	m_tms_tx_timer = timer_alloc(tms_tx_timer);
	mc146818_init(MC146818_STANDARD);
	scsi_init();
	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0x809923, 0x809923, 0, 0, dsp_speedup_r);
	memory_install_write32_handler(1, ADDRESS_SPACE_PROGRAM, 0x809923, 0x809923, 0, 0, dsp_speedup_w);
}
ROM_START( rotr )
	ROM_REGION32_LE(0x4000, REGION_USER4, 0)
	ROM_LOAD( "c31boot.bin", 0, 0x4000, BAD_DUMP CRC(bddc2763) SHA1(96b2170ecee5bec5abaa1741bb2d3b6096ecc262) )

	ROM_REGION32_LE(0x40000, REGION_USER1, ROMREGION_ERASE00)
	ROM_LOAD32_BYTE( "rasterspeed2.1_bootrom4.u10", 0x00000, 0x10000, CRC(6da142d1) SHA1(e2dbd479034677726fc26fd1ba85c4458d89286c) )

	ROM_REGION(0x8000, REGION_PROMS, 0)
	ROM_LOAD( "17128dpc.u72", 0x0000, 0x4000, CRC(5ddf6ee3) SHA1(f3d15b649b5641374a9e14877cea84ba9c57ef3c) )
	ROM_LOAD( "17128dpc.u73", 0x2000, 0x4000, CRC(9e274cea) SHA1(e974cad4e4b965bf2c9df7d3d0b4eec64629eeb0) )

	ROM_REGION(0x8000, REGION_USER3, 0)
	ROM_LOAD( "rotr.nv", 0x0000, 0x8000, CRC(62543517) SHA1(a4bf3431cdab956839bb155c4a8c140d30e5c7ec) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "rotr", 0, SHA1(d67d7feb52d8c7ba1d2a190a40d97e84871f2d80) )
ROM_END

ROM_START( rotra )
	ROM_REGION32_LE(0x4000, REGION_USER4, 0)
	ROM_LOAD( "c31boot.bin", 0, 0x4000, BAD_DUMP CRC(bddc2763) SHA1(96b2170ecee5bec5abaa1741bb2d3b6096ecc262) )

	ROM_REGION32_LE(0x40000, REGION_USER1, ROMREGION_ERASE00)
	ROM_LOAD32_BYTE( "rasterspeed2.1_bootrom4.u10", 0x00000, 0x10000, CRC(6da142d1) SHA1(e2dbd479034677726fc26fd1ba85c4458d89286c) )

	ROM_REGION(0x8000, REGION_PROMS, 0)
	ROM_LOAD( "17128dpc.u72", 0x0000, 0x4000, CRC(5ddf6ee3) SHA1(f3d15b649b5641374a9e14877cea84ba9c57ef3c) )
	ROM_LOAD( "17128dpc.u73", 0x2000, 0x4000, CRC(9e274cea) SHA1(e974cad4e4b965bf2c9df7d3d0b4eec64629eeb0) )

	ROM_REGION(0x8000, REGION_USER3, 0)
	ROM_LOAD( "rotr.nv", 0x0000, 0x8000, CRC(62543517) SHA1(a4bf3431cdab956839bb155c4a8c140d30e5c7ec) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "rotra", 0, SHA1(570d402e5e9bba123edf7dfa9db7a0e6bdb23823) )
ROM_END

ROM_START( fbcrazy )
	ROM_REGION32_LE(0x4000, REGION_USER4, 0)
	ROM_LOAD( "c31boot.bin", 0, 0x4000, BAD_DUMP CRC(bddc2763) SHA1(96b2170ecee5bec5abaa1741bb2d3b6096ecc262) )

	ROM_REGION32_LE(0x40000, REGION_USER1, ROMREGION_ERASE00)
	ROM_LOAD32_BYTE( "95751937.bin", 0x0000, 0x010000, CRC(4a99ee11) SHA1(335398ebc64bbfe86e2652ac080a5943dd413928) )

	ROM_REGION(0x8000, REGION_PROMS, ROMREGION_ERASE00 )

	ROM_REGION(0x8000, REGION_USER3, ROMREGION_ERASE00 )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "95100303", 0, SHA1(9ba47c96de27ec2bea9c6624d78d309b67705406)  )
ROM_END

GAME( 1994, rotr,    0,    rastersp, rotr, rastersp, ROT0, "BFM/Mirage", "Rise of the Robots (prototype)", GAME_NOT_WORKING )
GAME( 1994, rotra,   rotr, rastersp, rotr, rastersp, ROT0, "BFM/Mirage", "Rise of the Robots (prototype, older)", GAME_NOT_WORKING )
GAME( 1997, fbcrazy, 0,    fbcrazy, fbcrazy, rastersp, ROT0, "BFM", "Football Crazy (Video Quiz)", GAME_NOT_WORKING )
