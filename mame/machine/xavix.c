// license:BSD-3-Clause
// copyright-holders:David Haywood, Angelo Salese, ramacat
/***************************************************************************

    xavix.c

    XaviX TV games, machine hardware

***************************************************************************/

#include "driver.h"
#include "includes/xavix.h"
#include "machine/i2cmem.h"

int xavix_io_type = XAVIX_IO_PLAIN;
int xavix_i2c_size = 0x200;
int xavix_anport_type = XAVIX_ANPORT_NONE;

UINT8 *xavix_mainram;
UINT8 *xavix_fragment_sprite;
UINT8 *xavix_palram_sh;
UINT8 *xavix_palram_l;
UINT8 *xavix_segment_regs;
UINT8 *xavix_posirq_x;
UINT8 *xavix_posirq_y;
UINT8 *xavix_rom_dma_src;
UINT8 *xavix_rom_dma_dst;
UINT8 *xavix_rom_dma_len;

UINT8 xavix_video_ctrl;
UINT8 xavix_arena_start;
UINT8 xavix_arena_end;
UINT8 xavix_arena_control;

/* the number of ports and their order in the input ports definitions */
#define PORT_IN0		0
#define PORT_IN1		1
#define PORT_AN0		2
#define PORT_MOUSE0X	10
#define PORT_REGION		14

static UINT8 *rgn;
static UINT32 rgnlen;

static UINT8 irqsource;
static UINT8 vectorenable;
static UINT8 nmi_vector_lo_data;
static UINT8 nmi_vector_hi_data;
static UINT8 irq_vector_lo_data;
static UINT8 irq_vector_hi_data;
static UINT8 extbusctrl[3];
static UINT8 ioevent_enable;
static UINT8 ioevent_active;
static UINT8 io0_data;
static UINT8 io1_data;
static UINT8 io0_direction;
static UINT8 io1_direction;
static UINT8 timer_control;
static UINT8 timer_freq;
static UINT8 timer_currentval;
static UINT8 timer_baseval;
static UINT8 sound_irqstatus;
static UINT8 soundreg16_0[2];
static UINT8 sound_regbase;
static UINT8 sound_tp[4];
static UINT8 cyclerate;
static UINT8 adc_control;
static UINT8 adc_inlatch;

/* the math unit */
static UINT8 math_mpr;
static UINT8 math_mpd;
static UINT8 math_multresults[2];
static UINT8 math_mad;
static UINT8 math_sgn_mpd;
static UINT8 math_sgn_mpr;

static mame_timer *interrupt_timer;
static mame_timer *freq_timer;
static mame_timer *sound_timer[4];
static double freq_timer_start_time;
static double freq_timer_rate;
static UINT8 freq_timer_start_val;

static void update_irqs(void)
{
	cpunum_set_input_line(0, XAVIX_IRQ_LINE, irqsource != 0 ? ASSERT_LINE : CLEAR_LINE);
}

/* the interrupt vectors of the CPU */
static int get_vectors(int which, int half)
{
	if (vectorenable == 0)
		return -1;

	if (which == 0)
	{
		if (half == 0)
			return nmi_vector_hi_data;
		else
			return nmi_vector_lo_data;
	}
	else
	{
		if (half == 0)
			return irq_vector_hi_data;
		else
			return irq_vector_lo_data;
	}
}

struct xavix_config xavix_cpu_config =
{
	get_vectors
};

/***************************************************************************

    Memory bypass for the video and the sound

***************************************************************************/

INLINE UINT8 lowbus_bypass(UINT16 adr)
{
	adr &= 0x7fff;
	if (adr < 0x4000)
		return xavix_mainram[adr];
	else if (adr < 0x5000)
		return xavix_txarray_r(adr & 0xfff);
	else if (adr >= 0x6000 && adr < 0x6800)
		return xavix_fragment_sprite[adr & 0x7ff];
	else if (adr >= 0x6800 && adr < 0x6900)
		return xavix_palram_sh[adr & 0xff];
	else if (adr >= 0x6900 && adr < 0x6a00)
		return xavix_palram_l[adr & 0xff];
	else if (adr >= 0x6a00 && adr < 0x6a20)
		return xavix_segment_regs[adr & 0x1f];
	return 0x00;
}

UINT8 xavix_read_full_data_sp_bypass(UINT32 adr)
{
	UINT8 databank = adr >> 16;

	if (databank >= 0x80 || (adr & 0xffff) >= 0x8000)
		return rgn[adr & (rgnlen - 1)];

	return lowbus_bypass(adr);
}

UINT8 xavix_sound_sample_read(UINT32 offset)
{
	return xavix_read_full_data_sp_bypass(offset);
}

/***************************************************************************

    DMA

***************************************************************************/

WRITE8_HANDLER( xavix_rom_dmatrg_w )
{
	/* the interrupt is acknowledged by the bit 0x80 */
	if (data & 0x80)
	{
		if (irqsource & 0x20)
		{
			irqsource &= ~0x20;
			update_irqs();
		}
	}

	if (data & 0x01)
	{
		UINT32 source = (xavix_rom_dma_src[2] << 16) | (xavix_rom_dma_src[1] << 8) | xavix_rom_dma_src[0];
		UINT16 dest = (xavix_rom_dma_dst[1] << 8) | xavix_rom_dma_dst[0];
		UINT16 len = (xavix_rom_dma_len[1] << 8) | xavix_rom_dma_len[0];
		int i;

		for (i = 0; i < len; ++i)
		{
			UINT8 dat = xavix_read_full_data_sp_bypass(source + i);
			xavix_write_full_data((dest + i) & 0xffff, dat);
		}

		if (data & 0x40)
		{
			irqsource |= 0x20;
			update_irqs();
		}

		xavix_rom_dma_len[0] = 0;
		xavix_rom_dma_len[1] = 0;
	}
}

WRITE8_HANDLER( xavix_rom_dmasrc_w )
{
	xavix_rom_dma_src[offset] = data;
}

WRITE8_HANDLER( xavix_rom_dmadst_w )
{
	xavix_rom_dma_dst[offset] = data;
}

WRITE8_HANDLER( xavix_rom_dmalen_w )
{
	xavix_rom_dma_len[offset] = data;
}

READ8_HANDLER( xavix_rom_dmastat_r )
{
	return 0x00;
}

/***************************************************************************

    Vectors and interrupts

***************************************************************************/

WRITE8_HANDLER( xavix_vector_enable_w )
{
	vectorenable = data;
}

READ8_HANDLER( xavix_nmi_vector_lo_r )
{
	return nmi_vector_lo_data;
}

WRITE8_HANDLER( xavix_nmi_vector_lo_w )
{
	nmi_vector_lo_data = data;
}

READ8_HANDLER( xavix_nmi_vector_hi_r )
{
	return nmi_vector_hi_data;
}

WRITE8_HANDLER( xavix_nmi_vector_hi_w )
{
	nmi_vector_hi_data = data;
}

READ8_HANDLER( xavix_irq_vector_lo_r )
{
	return irq_vector_lo_data;
}

WRITE8_HANDLER( xavix_irq_vector_lo_w )
{
	irq_vector_lo_data = data;
}

READ8_HANDLER( xavix_irq_vector_hi_r )
{
	return irq_vector_hi_data;
}

WRITE8_HANDLER( xavix_irq_vector_hi_w )
{
	irq_vector_hi_data = data;
}

READ8_HANDLER( xavix_irq_source_r )
{
	return irqsource;
}

WRITE8_HANDLER( xavix_irq_source_w )
{
}

/* external bus control */
READ8_HANDLER( xavix_extintrf_790x_r )
{
	return extbusctrl[offset];
}

WRITE8_HANDLER( xavix_extintrf_790x_w )
{
	extbusctrl[offset] = data;
}

READ8_HANDLER( xavix_ioevent_enable_r )
{
	return ioevent_enable;
}

WRITE8_HANDLER( xavix_ioevent_enable_w )
{
	ioevent_enable = data;
}

READ8_HANDLER( xavix_ioevent_irqstate_r )
{
	return ioevent_active;
}

WRITE8_HANDLER( xavix_ioevent_irqack_w )
{
	ioevent_active &= ~(data & 0x0f);

	if (!(ioevent_active & 0x0f))
		irqsource &= ~0x08;

	update_irqs();
}

WRITE8_HANDLER( xavix_slotreg_7810_w )
{
}

INTERRUPT_GEN( xavix_interrupt )
{
	if (xavix_video_ctrl & 0x20)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
		xavix_video_ctrl |= 0x80;
	}
}

READ8_HANDLER( xavix_dispctrl_6ff8_r )
{
	return xavix_video_ctrl;
}

WRITE8_HANDLER( xavix_dispctrl_6ff8_w )
{
	UINT8 old_vid;

	if (data & 0x40)
	{
		irqsource &= ~0x40;
		update_irqs();
	}

	if (data & 0x80)
		cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);

	old_vid = xavix_video_ctrl;

	xavix_video_ctrl = data & 0x3f;

	/* disabling the raster IRQ clears the pending one, or the other IRQs are blocked */
	if ((old_vid & 0x10) != (xavix_video_ctrl & 0x10))
	{
		if (!(xavix_video_ctrl & 0x10))
		{
			xavix_video_ctrl &= ~0x40;
			irqsource &= ~0x40;
			update_irqs();
		}
	}
}

static double raster_time(void)
{
	return cpu_getscanlinetime(xavix_posirq_y[0] % Machine->drv->screen_height);
}

static void interrupt_gen(int param)
{
	if (xavix_video_ctrl & 0x10)
	{
		xavix_video_ctrl |= 0x40;
		irqsource |= 0x40;
		update_irqs();

		force_partial_update(cpu_getscanline());
	}

	/* the raster interrupt repeats at every frame */
	timer_adjust(interrupt_timer, raster_time(), 0, 0);
}

WRITE8_HANDLER( xavix_dispctrl_posirq_x_w )
{
	xavix_posirq_x[0] = data;
}

WRITE8_HANDLER( xavix_dispctrl_posirq_y_w )
{
	xavix_posirq_y[0] = data;

	timer_adjust(interrupt_timer, raster_time(), 0, 0);
}

READ8_HANDLER( xavix_arena_start_r )
{
	return xavix_arena_start;
}

WRITE8_HANDLER( xavix_arena_start_w )
{
	xavix_arena_start = data;
}

READ8_HANDLER( xavix_arena_end_r )
{
	return xavix_arena_end;
}

WRITE8_HANDLER( xavix_arena_end_w )
{
	xavix_arena_end = data;
}

READ8_HANDLER( xavix_arena_control_r )
{
	/* the bit 0x40 toggles at every read */
	xavix_arena_control ^= 0x40;
	return xavix_arena_control;
}

WRITE8_HANDLER( xavix_arena_control_w )
{
	xavix_arena_control = data;

	if (data & 0x80)
		xavix_arena_control &= ~0x80;
}

/***************************************************************************

    I/O ports

***************************************************************************/

static UINT8 read_io0(UINT8 direction)
{
	return readinputport(PORT_IN0);
}

static UINT8 read_io1(UINT8 direction)
{
	UINT8 ret = readinputport(PORT_IN1);

	if (xavix_io_type == XAVIX_IO_I2C_DUELMAST)
	{
		/* values returned at the specific addresses of the startup code */
		switch (activecpu_get_pc())
		{
		case 0x3c01: return 0x00;
		case 0x3c06: return 0x02;
		case 0x3c14: return 0x04;
		case 0x3c19: return 0x00;
		case 0x3c25: return 0x04;
		case 0x3c2a: return 0x00;
		}
	}

	if (xavix_io_type != XAVIX_IO_PLAIN)
	{
		ret &= ~0x08;
		ret |= i2cmem_read_sda() << 3;
	}

	return ret;
}

static void write_io1(UINT8 data, UINT8 direction)
{
	if (xavix_io_type != XAVIX_IO_PLAIN)
	{
		i2cmem_write_sda((direction & 0x08) ? ((data >> 3) & 1) : 1);
		i2cmem_write_scl((direction & 0x10) ? ((data >> 4) & 1) : 0);
	}
}

READ8_HANDLER( xavix_io0_data_r )
{
	UINT8 ret = read_io0(io0_direction) & ~io0_direction;
	ret |= io0_data & io0_direction;
	return ret;
}

READ8_HANDLER( xavix_io1_data_r )
{
	UINT8 ret = read_io1(io1_direction) & ~io1_direction;
	ret |= io1_data & io1_direction;
	return ret;
}

READ8_HANDLER( xavix_io0_direction_r )
{
	return io0_direction;
}

READ8_HANDLER( xavix_io1_direction_r )
{
	return io1_direction;
}

WRITE8_HANDLER( xavix_io0_data_w )
{
	io0_data = data;
}

WRITE8_HANDLER( xavix_io1_data_w )
{
	io1_data = data;
	write_io1((data & io1_direction) | (read_io1(io1_direction) & ~io1_direction), io1_direction);
}

WRITE8_HANDLER( xavix_io0_direction_w )
{
	io0_direction = data;
	xavix_io0_data_w(0, io0_data);
}

WRITE8_HANDLER( xavix_io1_direction_w )
{
	io1_direction = data;

	/* the serial EEPROM requires to write again the data */
	xavix_io1_data_w(0, io1_data);
}

/***************************************************************************

    Analog ports and ADC

***************************************************************************/

static UINT8 anport_read(int index)
{
	if (xavix_anport_type == XAVIX_ANPORT_MOUSE)
		return (readinputport(PORT_MOUSE0X + index) ^ 0x7f) + 1;
	return 0xff;
}

READ8_HANDLER( xavix_anport_r )
{
	return anport_read(offset);
}

READ8_HANDLER( xavix_anport2_r )
{
	return anport_read(offset + 2);
}

WRITE8_HANDLER( xavix_anport_w )
{
}

READ8_HANDLER( xavix_adc_7b80_r )
{
	return adc_inlatch;
}

WRITE8_HANDLER( xavix_adc_7b80_w )
{
}

WRITE8_HANDLER( xavix_adc_7b81_w )
{
	adc_control = data;

	switch (adc_control & 0x13)
	{
	case 0x00: adc_inlatch = readinputport(PORT_AN0 + 0); break;
	case 0x01: adc_inlatch = readinputport(PORT_AN0 + 1); break;
	case 0x02: adc_inlatch = readinputport(PORT_AN0 + 2); break;
	case 0x03: adc_inlatch = readinputport(PORT_AN0 + 3); break;
	case 0x10: adc_inlatch = readinputport(PORT_AN0 + 4); break;
	case 0x11: adc_inlatch = readinputport(PORT_AN0 + 5); break;
	case 0x12: adc_inlatch = readinputport(PORT_AN0 + 6); break;
	case 0x13: adc_inlatch = readinputport(PORT_AN0 + 7); break;
	}
}

READ8_HANDLER( xavix_adc_7b81_r )
{
	return mame_rand() & 0xff;
}

/***************************************************************************

    Math unit

***************************************************************************/

static void do_math(int mul_shf)
{
	UINT16 result;
	UINT8 mpd = math_mpd;
	UINT8 mulinp;

	if (!mul_shf)
		mulinp = math_mpr;
	else
		mulinp = 1 << (math_mpr & 0x7);

	if (math_sgn_mpd && (mpd & 0x80) && math_sgn_mpr && (mulinp & 0x80))
		result = mpd * mulinp - (mpd << 8) - (mulinp << 8);
	else if (math_sgn_mpd && (mpd & 0x80))
		result = mpd * mulinp - (mulinp << 8);
	else if (math_sgn_mpr && (mulinp & 0x80))
		result = mpd * mulinp - (mpd << 8);
	else
		result = mpd * mulinp;

	if (math_mad)
	{
		UINT16 oldresult = (math_multresults[1] << 8) | math_multresults[0];
		result = oldresult + result;
	}

	math_multresults[1] = (result >> 8) & 0xff;
	math_multresults[0] = result & 0xff;
}

READ8_HANDLER( xavix_barrel_r )
{
	if (offset == 0)
		return (math_sgn_mpd ? 0x80 : 0x00) | (math_mpr & 0x40) | (math_mpr & 0x07);

	return (math_mpr & 0x40) ?
		math_multresults[1] ^ math_multresults[0] : (math_mpr & 0x08) ?
		math_multresults[1] : math_multresults[0];
}

WRITE8_HANDLER( xavix_barrel_w )
{
	if (offset == 0)
	{
		math_mad = 0;
		math_sgn_mpd = data & 0x80;
		math_sgn_mpr = 0;
		math_mpr = data;
		do_math(1);
	}
	else
	{
		math_mpd = data;
	}
}

READ8_HANDLER( xavix_mult_r )
{
	return math_multresults[offset];
}

WRITE8_HANDLER( xavix_mult_w )
{
	math_multresults[offset] = data;
}

READ8_HANDLER( xavix_mult_param_r )
{
	if (offset == 0)
		return math_mad | (math_sgn_mpd ? 0x02 : 0x00) | math_sgn_mpr;
	else if (offset == 1)
		return math_mpd;
	return math_mpr;
}

WRITE8_HANDLER( xavix_mult_param_w )
{
	if (offset == 0)
	{
		math_mad = data & 0x80;
		math_sgn_mpd = data & 0x02;
		math_sgn_mpr = data & 0x01;
	}
	else if (offset == 1)
	{
		math_mpd = data;
	}
	else if (offset == 2)
	{
		math_mpr = data;
		do_math(0);
	}
}

/***************************************************************************

    Timer

***************************************************************************/

READ8_HANDLER( xavix_timer_baseval_r )
{
	return timer_baseval;
}

READ8_HANDLER( xavix_timer_status_r )
{
	return timer_control;
}

/* the timer counts down from the base value, and it is expired when it wraps */
static void timer_start(void)
{
	int divide = 1 << ((timer_freq & 0x0f) + 1);

	freq_timer_rate = (double)XAVIX_MAIN_CLOCK / divide;
	freq_timer_start_val = timer_currentval;
	freq_timer_start_time = timer_get_time();

	timer_adjust(freq_timer, (timer_currentval + 1) / freq_timer_rate, 0, 0);
}

WRITE8_HANDLER( xavix_timer_control_w )
{
	timer_control = data;

	/* the interrupt is acknowledged by writing the top bit */
	if (data & 0x80)
	{
		if (irqsource & 0x10)
		{
			irqsource &= ~0x10;
			update_irqs();
		}
	}

	if (data & 0x01)
	{
		timer_currentval = timer_baseval;
		timer_start();
	}
	else
	{
		timer_adjust(freq_timer, TIME_NEVER, 0, 0);
	}
}

WRITE8_HANDLER( xavix_timer_baseval_w )
{
	timer_baseval = data;
}

READ8_HANDLER( xavix_timer_freq_r )
{
	return timer_freq;
}

READ8_HANDLER( xavix_timer_curval_r )
{
	/* while the timer is running the value is computed from the elapsed time */
	if ((timer_control & 0x01) && timer_currentval != 0xff)
	{
		double ticks = (timer_get_time() - freq_timer_start_time) * freq_timer_rate;

		if (ticks <= freq_timer_start_val)
			return freq_timer_start_val - (UINT8)ticks;
	}
	return timer_currentval;
}

WRITE8_HANDLER( xavix_timer_freq_w )
{
	/* the prescale of the main clock, from /2 to /65536 */
	timer_freq = data & 0x0f;
}

static void freq_timer_done(int param)
{
	timer_currentval = 0xff;

	if (timer_control & 0x40)
	{
		irqsource |= 0x10;
		timer_control |= 0x80;
		update_irqs();
	}
}

/***************************************************************************

    Sound registers

***************************************************************************/

static void refresh_sound_irq_state(void)
{
	UINT8 enable = sound_irqstatus & 0x0f;
	UINT8 pending = (sound_irqstatus >> 4) & 0x0f;

	if (enable & pending)
		irqsource |= 0x80;
	else
		irqsource &= ~0x80;
}

static void reprogram_sound_timer(int index)
{
	UINT8 mask = 1 << index;
	UINT8 tempo = sound_tp[index];
	double frequency;

	if (!(sound_irqstatus & mask) || tempo == 0)
	{
		timer_adjust(sound_timer[index], TIME_NEVER, index, 0);
		return;
	}

	frequency = xavix_sound_tempo_tick_hz(tempo);
	if (frequency <= 0.0)
	{
		timer_adjust(sound_timer[index], TIME_NEVER, index, 0);
		return;
	}

	timer_adjust(sound_timer[index], TIME_IN_HZ(frequency), index, TIME_IN_HZ(frequency));
}

static void sound_timer_done(int param)
{
	UINT8 enable_mask = 1 << param;

	if (!((sound_irqstatus >> param) & 1))
		return;

	sound_irqstatus |= (enable_mask << 4);
	refresh_sound_irq_state();
	update_irqs();
}

UINT8 xavix_sound_regram_read(offs_t offset)
{
	/* the page 0 would be the zero page, and it's not valid */
	if ((sound_regbase & 0x3f) != 0x00)
	{
		UINT16 memorybase = (sound_regbase & 0x3f) << 8;
		return xavix_mainram[memorybase + offset];
	}
	return 0x00;
}

void xavix_sound_regram_write(offs_t offset, UINT8 data)
{
	if ((sound_regbase & 0x3f) != 0x00)
	{
		UINT16 memorybase = (sound_regbase & 0x3f) << 8;
		xavix_mainram[memorybase + offset] = data;
	}
}

READ8_HANDLER( xavix_sound_voice_startstop_r )
{
	return soundreg16_0[offset];
}

WRITE8_HANDLER( xavix_sound_voice_startstop_w )
{
	int i;

	for (i = 0; i < 8; ++i)
	{
		int voice_state = (data >> i) & 1;
		int old_voice_state = (soundreg16_0[offset] >> i) & 1;

		if (voice_state != old_voice_state)
		{
			int voice = offset * 8 + i;

			if (voice_state)
				xavix_sound_enable_voice(voice, 0);
			else
				xavix_sound_disable_voice(voice);
		}
	}
	soundreg16_0[offset] = data;
}

READ8_HANDLER( xavix_sound_voice_updateenv_r )
{
	return 0x00;
}

WRITE8_HANDLER( xavix_sound_voice_updateenv_w )
{
	int i;

	for (i = 0; i < 8; ++i)
	{
		if ((data >> i) & 1)
			xavix_sound_enable_voice(offset * 8 + i, 1);
	}
}

READ8_HANDLER( xavix_sound_voice_status_r )
{
	UINT8 ret = 0x00;
	int i;

	for (i = 0; i < 8; ++i)
	{
		if (xavix_sound_is_voice_enabled(offset * 8 + i))
			ret |= 1 << i;
	}
	return ret;
}

READ8_HANDLER( xavix_sound_regbase_r )
{
	return sound_regbase & 0x3f;
}

WRITE8_HANDLER( xavix_sound_regbase_w )
{
	/* upper 6 bits of the RAM address where the registers of the voices live */
	sound_regbase = data & 0x3f;
}

READ8_HANDLER( xavix_sound_cyclerate_r )
{
	return cyclerate;
}

WRITE8_HANDLER( xavix_sound_cyclerate_w )
{
	cyclerate = data;
	xavix_sound_set_cyclerate(data);
}

READ8_HANDLER( xavix_sound_volume_r )
{
	return xavix_sound_volume_get();
}

WRITE8_HANDLER( xavix_sound_volume_w )
{
	xavix_sound_volume_set(data);
}

READ8_HANDLER( xavix_sound_mixer_r )
{
	return xavix_sound_mixer_get();
}

WRITE8_HANDLER( xavix_sound_mixer_w )
{
	xavix_sound_mixer_set(data);
}

READ8_HANDLER( xavix_sound_dac_control_r )
{
	return xavix_sound_dac_control_get();
}

WRITE8_HANDLER( xavix_sound_dac_control_w )
{
	xavix_sound_dac_control_set(data);
}

/* the tempo registers */
READ8_HANDLER( xavix_sound_tp_r )
{
	return sound_tp[offset];
}

WRITE8_HANDLER( xavix_sound_tp_w )
{
	sound_tp[offset] = data;
	xavix_sound_set_tempo(offset, data);
	reprogram_sound_timer(offset);
}

READ8_HANDLER( xavix_sound_irq_status_r )
{
	return sound_irqstatus;
}

WRITE8_HANDLER( xavix_sound_irq_status_w )
{
	UINT8 old_enable = sound_irqstatus & 0x0f;
	UINT8 clear_mask = (data >> 4) & 0x0f;
	UINT8 new_enable;
	UINT8 changed;
	int t;

	if (clear_mask)
		sound_irqstatus &= ~(clear_mask << 4);

	new_enable = data & 0x0f;
	sound_irqstatus = (sound_irqstatus & 0xf0) | new_enable;

	changed = old_enable ^ new_enable;
	if (changed)
	{
		for (t = 0; t < 4; ++t)
			if (changed & (1 << t))
				reprogram_sound_timer(t);
	}

	refresh_sound_irq_state();
	update_irqs();
}

/***************************************************************************

    Machine

***************************************************************************/

MACHINE_START( xavix )
{
	int i;

	interrupt_timer = timer_alloc(interrupt_gen);
	freq_timer = timer_alloc(freq_timer_done);

	for (i = 0; i < 4; ++i)
		sound_timer[i] = timer_alloc(sound_timer_done);

	rgn = memory_region(REGION_CPU1);
	rgnlen = memory_region_length(REGION_CPU1);

	return 0;
}

MACHINE_RESET( xavix )
{
	int i;

	/* the games expect the RAM filled with 0xff */
	memset(xavix_mainram, 0xff, 0x4000);

	xavix_rom_dma_src[0] = 0;
	xavix_rom_dma_src[1] = 0;
	xavix_rom_dma_src[2] = 0;
	xavix_rom_dma_dst[0] = 0;
	xavix_rom_dma_dst[1] = 0;
	xavix_rom_dma_len[0] = 0;
	xavix_rom_dma_len[1] = 0;

	vectorenable = 0;
	nmi_vector_lo_data = 0;
	nmi_vector_hi_data = 0;
	irq_vector_lo_data = 0;
	irq_vector_hi_data = 0;

	xavix_video_ctrl = 0;
	xavix_arena_control = 0;
	xavix_arena_start = 0;
	xavix_arena_end = 0;

	cyclerate = 0x00;
	for (i = 0; i < 4; ++i)
		sound_tp[i] = 0x00;

	io0_data = 0x00;
	io1_data = 0x00;
	io0_direction = 0x00;
	io1_direction = 0x00;

	irqsource = 0x00;

	timer_control = 0x00;
	timer_freq = 0x00;
	timer_currentval = 0x00;
	timer_baseval = 0x00;

	ioevent_enable = 0x00;
	ioevent_active = 0x00;

	sound_irqstatus = 0x00;
	soundreg16_0[0] = 0x00;
	soundreg16_0[1] = 0x00;

	/* the default page of the sound registers, as the games don't set it */
	sound_regbase = 0x02;

	adc_control = 0x00;
	adc_inlatch = 0xff;

	math_mpr = 0;
	math_mpd = 0;
	math_mad = 0;
	math_sgn_mpd = 0;
	math_sgn_mpr = 0;
	math_multresults[0] = 0;
	math_multresults[1] = 0;

	extbusctrl[0] = 0x00;
	extbusctrl[1] = 0x00;
	extbusctrl[2] = 0x00;

	xavix_video_reset();
}

/* the last 0x1000 bytes of the RAM are backed up */
NVRAM_HANDLER( xavix_nv )
{
	if (read_or_write)
	{
		mame_fwrite(file, xavix_mainram + 0x3000, 0x1000);
	}
	else
	{
		if (file)
			mame_fread(file, xavix_mainram + 0x3000, 0x1000);
		else
			memset(xavix_mainram + 0x3000, 0xff, 0x1000);
	}
}

NVRAM_HANDLER( xavix_i2c )
{
	if (read_or_write)
	{
		mame_fwrite(file, i2cmem_get_data(), i2cmem_get_size());
	}
	else
	{
		i2cmem_init(xavix_i2c_size, 16);

		if (file)
			mame_fread(file, i2cmem_get_data(), i2cmem_get_size());
	}
}

static void init_common(int io_type, int i2c_size, int anport_type)
{
	xavix_io_type = io_type;
	xavix_i2c_size = i2c_size;
	xavix_anport_type = anport_type;

	/* the default tempo of the sound hardware, used to initialize its tempo registers */
	xavix_sound_set_default_tempo(0x80);
}

DRIVER_INIT( xavix )
{
	init_common(XAVIX_IO_PLAIN, 0x200, XAVIX_ANPORT_NONE);
}

DRIVER_INIT( xavix_sdb )
{
	init_common(XAVIX_IO_PLAIN, 0x200, XAVIX_ANPORT_MOUSE);
}

DRIVER_INIT( xavix_i2c_24c02 )
{
	init_common(XAVIX_IO_I2C, 0x100, XAVIX_ANPORT_NONE);
}

DRIVER_INIT( xavix_i2c_24c04 )
{
	init_common(XAVIX_IO_I2C, 0x200, XAVIX_ANPORT_NONE);
}

DRIVER_INIT( xavix_i2c_24c08 )
{
	init_common(XAVIX_IO_I2C, 0x400, XAVIX_ANPORT_NONE);
}

DRIVER_INIT( xavix_duelmast )
{
	init_common(XAVIX_IO_I2C_DUELMAST, 0x200, XAVIX_ANPORT_NONE);
}
