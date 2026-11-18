// license:BSD-3-Clause
// copyright-holders:Philip Bennett

/*
 * Entertainment Sciences Real-Time Image Processor System
 *
 * On first boot, sweep both stick axes, center the stick and press
 * keypad 1 to save the range. Use keypad 1 to exit each operator menu.
 */

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/esrip/esrip.h"
#include "sound/dac.h"
#include "sound/5220intf.h"
#include "esripsys.h"

static UINT8 g_iodata;
static UINT8 g_ioaddr;
static UINT8 coin_latch;
static UINT8 keypad_status;
static UINT8 g_status;
static UINT8 f_status;
static int io_firq_status;
static UINT8 cmos_ram_a2_0;
static UINT8 cmos_ram_a10_3;
static UINT8 *cmos_ram;
static int startup_key_pending;

static UINT8 u56a;
static UINT8 u56b;
static UINT8 g_to_s_latch1;
static UINT8 g_to_s_latch2;
static UINT8 s_to_g_latch1;
static UINT8 s_to_g_latch2;
static UINT8 dac_msb;
static UINT8 dac_vol;

static UINT8 *fdt_a;
static UINT8 *fdt_b;
static int fasel;
static int fbsel;

/* The sound board supplies the 6809 E clock to the PTM. Its external
   clocks and timer outputs are disconnected. */
#define ESRIPSYS_E_CLOCK (8000000 / 4)

static struct
{
	mame_timer *timer[3];
	UINT16 latch[3];
	UINT16 counter[3];
	UINT8 control[3];
	UINT8 status;
	UINT8 status_read;
	UINT8 write_msb;
	UINT8 read_lsb;
} sound_ptm;

static void sound_ptm_irq(void)
{
	int i;
	int irq = 0;

	for (i = 0; i < 3; ++i)
		if ((sound_ptm.control[i] & 0x40) && (sound_ptm.status & (1 << i)))
			irq = 1;

	cpunum_set_input_line(ESRIPSYS_SOUND_CPU, M6809_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static int sound_ptm_clock(int channel)
{
	return ESRIPSYS_E_CLOCK / ((channel == 2 && (sound_ptm.control[2] & 1)) ? 8 : 1);
}

static void sound_ptm_reload(int channel)
{
	int count = sound_ptm.latch[channel];
	sound_ptm.counter[channel] = count;

	if ((sound_ptm.control[0] & 1) || !(sound_ptm.control[channel] & 2))
	{
		timer_adjust(sound_ptm.timer[channel], TIME_NEVER, channel, 0);
		return;
	}

	if (sound_ptm.control[channel] & 4)
		count = ((count >> 8) + 1) * ((count & 0xff) + 1);
	else
		++count;

	timer_adjust(sound_ptm.timer[channel], TIME_IN_HZ(sound_ptm_clock(channel)) * count, channel, 0);
}

static void sound_ptm_timeout(int channel)
{
	sound_ptm.status |= 1 << channel;
	sound_ptm.status_read &= ~(1 << channel);
	sound_ptm_irq();
	sound_ptm_reload(channel);
}

static READ8_HANDLER( sound_ptm_r )
{
	int channel;
	int count;

	if (offset == 0)
		return 0;

	if (offset == 1)
	{
		int i;
		int status = sound_ptm.status;

		sound_ptm.status_read |= status;
		for (i = 0; i < 3; ++i)
			if ((sound_ptm.control[i] & 0x40) && (status & (1 << i)))
				return status | 0x80;
		return status;
	}

	if (offset & 1)
		return sound_ptm.read_lsb;

	channel = (offset - 2) / 2;
	count = sound_ptm.counter[channel];
	if (!(sound_ptm.control[0] & 1) && (sound_ptm.control[channel] & 2))
	{
		count = (int)(timer_timeleft(sound_ptm.timer[channel]) * sound_ptm_clock(channel));
		if (sound_ptm.control[channel] & 4)
		{
			int divisor = (sound_ptm.counter[channel] & 0xff) + 1;
			count = ((count / divisor) << 8) | (count % divisor);
		}
	}

	/* Acknowledge only a timeout already observed in the status register. */
	if (sound_ptm.status_read & (1 << channel))
	{
		sound_ptm.status &= ~(1 << channel);
		sound_ptm.status_read &= ~(1 << channel);
		sound_ptm_irq();
	}
	sound_ptm.read_lsb = count & 0xff;

	return count >> 8;
}

static WRITE8_HANDLER( sound_ptm_w )
{
	int channel;

	if (offset < 2)
	{
		int changed;
		channel = offset ? 1 : ((sound_ptm.control[1] & 1) ? 0 : 2);
		changed = sound_ptm.control[channel] ^ data;
		sound_ptm.control[channel] = data;
		if (channel == 0 && (changed & 1))
		{
			sound_ptm.status = sound_ptm.status_read = 0;
			for (channel = 0; channel < 3; ++channel)
				sound_ptm_reload(channel);
		}
		else if ((changed & 6) || (channel == 2 && (changed & 1)))
			sound_ptm_reload(channel);
		sound_ptm_irq();
	}
	else if (!(offset & 1))
		sound_ptm.write_msb = data;
	else
	{
		channel = (offset - 2) / 2;
		sound_ptm.latch[channel] = (sound_ptm.write_msb << 8) | data;
		sound_ptm.status &= ~(1 << channel);
		sound_ptm.status_read &= ~(1 << channel);
		sound_ptm_irq();
		if (!(sound_ptm.control[channel] & 0x10) || (sound_ptm.control[0] & 1))
			sound_ptm_reload(channel);
	}
}

static MACHINE_RESET( esripsys )
{
	int i;

	fasel = 0;
	fbsel = 1;
	startup_key_pending = 1;
	esripsys_video_reset();

	sound_ptm.control[0] = 1;
	sound_ptm.control[1] = sound_ptm.control[2] = 0;
	sound_ptm.status = sound_ptm.status_read = 0;
	sound_ptm.read_lsb = sound_ptm.write_msb = 0;
	for (i = 0; i < 3; ++i)
	{
		sound_ptm.latch[i] = 0xffff;
		sound_ptm_reload(i);
	}
	sound_ptm_irq();
}

static WRITE8_HANDLER( uart_w )
{
	if ((offset & 1) == 0)
		logerror("%c", data);
}

static READ8_HANDLER( uart_r )
{
	/* The diagnostic serial port has no receiver attached. */
	return (offset & 1) ? 0x05 : 0;
}

static READ8_HANDLER( g_status_r )
{
	int bank4 = BIT(esrip_get_status(ESRIPSYS_VIDEO_CPU), 2);
	int vblank = (esripsys_scanline >= ESRIPSYS_VBLANK_START);

	return (!vblank << 7) | (bank4 << 6) | (f_status & 0x2f);
}

static WRITE8_HANDLER( g_status_w )
{
	int bankaddress;
	UINT8* const ROM = memory_region(REGION_CPU1);

	g_status = data;

	bankaddress = 0x10000 + (data & 0x03) * 0x10000;
	memory_set_bankptr(1, &ROM[bankaddress]);

	cpunum_set_input_line(ESRIPSYS_FRAME_CPU, M6809_FIRQ_LINE, data & 0x10 ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(ESRIPSYS_FRAME_CPU, INPUT_LINE_NMI,  data & 0x80 ? CLEAR_LINE : ASSERT_LINE);

	cpunum_set_input_line(ESRIPSYS_VIDEO_CPU, INPUT_LINE_RESET, data & 0x40 ? CLEAR_LINE : ASSERT_LINE);

	if (!(data & 0x20))
		cpunum_set_input_line(ESRIPSYS_GAME_CPU, M6809_IRQ_LINE, CLEAR_LINE);
}

static READ8_HANDLER( f_status_r )
{
	int vblank = (esripsys_scanline >= ESRIPSYS_VBLANK_START);
	UINT8 rip_status = esrip_get_status(ESRIPSYS_VIDEO_CPU);

	rip_status = (rip_status & 0x18) | (BIT(rip_status, 6) << 1) |  BIT(rip_status, 7);

	return (!vblank << 7) | (fbsel << 6) | (esripsys_frame_vbl << 5) | rip_status;
}

static WRITE8_HANDLER( f_status_w )
{
	f_status = data;
}

static void delayed_bank_swap(int param)
{
	fasel ^= 1;
	fbsel ^= 1;
}

static WRITE8_HANDLER( frame_w )
{
	timer_set(TIME_NOW, 0, delayed_bank_swap);
	esripsys_frame_vbl = 1;
}

static READ8_HANDLER( fdt_r )
{
	if (!fasel)
		return fdt_b[offset];
	else
		return fdt_a[offset];
}

static WRITE8_HANDLER( fdt_w )
{
	if (!fasel)
		fdt_b[offset] = data;
	else
		fdt_a[offset] = data;
}

static READ16_HANDLER( fdt_rip_r )
{
	offset = (offset & 0x7ff) << 1;

	if (!fasel)
		return (fdt_a[offset] << 8) | fdt_a[offset + 1];
	else
		return (fdt_b[offset] << 8) | fdt_b[offset + 1];
}

static WRITE16_HANDLER( fdt_rip_w )
{
	offset = (offset & 0x7ff) << 1;

	if (!fasel)
	{
		fdt_a[offset + 0] = data >> 8;
		fdt_a[offset + 1] = data & 0xff;
	}
	else
	{
		fdt_b[offset + 0] = data >> 8;
		fdt_b[offset + 1] = data & 0xff;
	}
}

static UINT8 rip_status_in(void)
{
	int vpos = esripsys_scanline;
	UINT8 _vblank = !(vpos >= ESRIPSYS_VBLANK_START);

	return	_vblank
			| (esripsys_hblank << 1)
			| (esripsys__12sel << 2)
			| (fbsel << 4)
			| ((vpos & 1) << 5)
			| (f_status & 0x80);
}

static WRITE8_HANDLER( g_iobus_w )
{
	g_iodata = data;
}

static READ8_HANDLER( g_iobus_r )
{
	switch (g_ioaddr & 0x7f)
	{
		case 0:
			return s_to_g_latch2 & 0x3f;
		case 3:
			return s_to_g_latch1;
		case 5:
			return cmos_ram[(cmos_ram_a10_3 << 3) | (cmos_ram_a2_0 & 3)];
		case 8:
		{
			int keypad = readinputportbytag("KEYPAD_B") | keypad_status;

			if (startup_key_pending)
			{
				/* These revisions sample * once before the emulator has polled input.
				   Present that key for the first read when the power-on tests are disabled. */
				if (!readinputportbytag_safe("BOOT", 1))
					keypad &= ~0x10;
				startup_key_pending = 0;
			}
			keypad_status = 0;
			io_firq_status = 0;
			return keypad;
		}
		case 9:
		{
			return readinputportbytag("KEYPAD_A");
		}
		case 0xa:
		{
			int coins =  coin_latch | (readinputportbytag("COINS") & 0x30);
			coin_latch = 0;
			io_firq_status = 0;
			return coins;
		}
		case 0x10:
			return readinputportbytag("IO_1");
		case 0x11:
			return readinputportbytag("JOYSTICK_X");
		case 0x12:
			return readinputportbytag("JOYSTICK_Y");
		case 0x16:
			return io_firq_status;
		case 0x18:
			return readinputportbytag("IO_2");

		case 0x19:
		case 0x1a:
			return 0xff;
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x58:
		case 0x59:
		case 0x5a:
		case 0x5b:
		case 0x5c:
		case 0x5d:
		case 0x5e:

			return 0xff;
		default:
		{
			logerror("Unknown I/O read (%x)\n", g_ioaddr & 0x7f);
			return 0xff;
		}
	}
}

static WRITE8_HANDLER( g_ioadd_w )
{
	g_ioaddr = data;

	if ((data & 0x80) == 0)
	{
		switch (g_ioaddr & 0x7f)
		{
			case 0x00:
			{
				g_to_s_latch1 = g_iodata;
				break;
			}
			case 0x02:
			{
				cpunum_set_input_line(ESRIPSYS_SOUND_CPU, INPUT_LINE_NMI, g_iodata & 4 ? CLEAR_LINE : ASSERT_LINE);

				if (!(g_to_s_latch2 & 1) && (g_iodata & 1))
				{

					u56a = 1;

					cpunum_set_input_line(ESRIPSYS_SOUND_CPU, M6809_IRQ_LINE, ASSERT_LINE);
				}

				if (g_iodata & 2)
					u56b = 0;

				g_to_s_latch2 = g_iodata;

				break;
			}
			case 0x04:
			{
				cmos_ram[(cmos_ram_a10_3 << 3) | (cmos_ram_a2_0 & 3)] = g_iodata;
				break;
			}
			case 0x06:
			{
				cmos_ram_a10_3 = g_iodata;
				break;
			}
			case 0x07:
			{
				cmos_ram_a2_0 = g_iodata;
				break;
			}
			case 0x0b:
			{

				break;
			}
			case 0x14:
			{
				break;
			}
			case 0x15:
			{
				esripsys_video_firq_en = g_iodata & 1;
				break;
			}
			default:
			{
				logerror("Unknown I/O write to %x with %x\n", g_ioaddr, g_iodata);
			}
		}
	}
}

static void keypad_interrupt(void *param, UINT32 oldval, UINT32 newval)
{
	if (oldval & ~newval)
	{
		io_firq_status |= 2;
		keypad_status |= 0x20;
		cpunum_set_input_line(ESRIPSYS_GAME_CPU, M6809_FIRQ_LINE, HOLD_LINE);
	}
}

static void coin_interrupt(void *param, UINT32 oldval, UINT32 newval)
{
	if (newval & ~oldval)
	{
		io_firq_status |= 2;
		coin_latch = (newval & ~oldval) << 2;
		cpunum_set_input_line(ESRIPSYS_GAME_CPU, M6809_FIRQ_LINE, HOLD_LINE);
	}
}

INPUT_PORTS_START( turbosub )
	PORT_START_TAG("KEYPAD_A")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_0_PAD) PORT_NAME("Keypad 0")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_1_PAD) PORT_NAME("Keypad 1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_2_PAD) PORT_NAME("Keypad 2")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_3_PAD) PORT_NAME("Keypad 3")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_4_PAD) PORT_NAME("Keypad 4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_5_PAD) PORT_NAME("Keypad 5")

	PORT_START_TAG("KEYPAD_B")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_6_PAD) PORT_NAME("Keypad 6")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_7_PAD) PORT_NAME("Keypad 7")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER )  PORT_CODE(KEYCODE_8_PAD) PORT_NAME("Keypad 8")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_9_PAD) PORT_NAME("Keypad 9")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_ASTERISK) PORT_NAME("Keypad *")

	PORT_START_TAG("COINS")
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("IO_1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Neutralizer")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Fire")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IO_2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("JOYSTICK_X")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(30)

	PORT_START_TAG("JOYSTICK_Y")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(70) PORT_KEYDELTA(30)
INPUT_PORTS_END

INPUT_PORTS_START( turbosub_boot )
	PORT_INCLUDE( turbosub )

	PORT_START_TAG("BOOT")
	PORT_DIPNAME( 0x01, 0x00, "Power-on Hardware Tests" )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x01, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( turbosubb8 )
	PORT_INCLUDE( turbosub_boot )

	PORT_MODIFY("IO_1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )
INPUT_PORTS_END

static READ8_HANDLER( s_200e_r )
{
	return g_to_s_latch1;
}

static WRITE8_HANDLER( s_200e_w )
{
	s_to_g_latch1 = data;
}

static WRITE8_HANDLER( s_200f_w )
{
	UINT8* const ROM = memory_region(REGION_SOUND1);
	int rombank = data & 0x20 ? 0x2000 : 0;

	if (s_to_g_latch2 & 0x40)
	{
		u56a = 0;
		cpunum_set_input_line(ESRIPSYS_SOUND_CPU, M6809_IRQ_LINE, CLEAR_LINE);
	}

	if (!(s_to_g_latch2 & 0x80) && (data & 0x80))
		u56b = 1;

	memory_set_bankptr(2, &ROM[0x0000 + rombank]);
	memory_set_bankptr(3, &ROM[0x4000 + rombank]);
	memory_set_bankptr(4, &ROM[0x8000 + rombank]);

	s_to_g_latch2 = data;
}

static READ8_HANDLER( s_200f_r )
{
	return (g_to_s_latch2 & 0xfc) | (u56b << 1) | u56a;
}

static READ8_HANDLER( speech_r )
{
	if (offset == 0)
	{

		UINT8 status = tms5220_status_r(0);

		status = (status >> 5) & 7;
		return (!tms5220_ready_r() << 7) | (!tms5220_int_r() << 6) | status;
	}

	return 0xff;
}

static WRITE8_HANDLER( speech_w )
{
	if (offset == 0)
		tms5220_data_w(0, data);
}

static WRITE8_HANDLER( control_w )
{
	logerror("Sound control write: %.2x (PC:0x%.4x)\n", data, activecpu_get_previouspc());
}

static WRITE8_HANDLER( dac_w )
{
	if (offset == 0)
	{
		dac_msb = data & 3;
	}
	else
	{
		UINT16 dac_data = (dac_msb << 8) | data;

		/* The volume DAC scales both halves of the bipolar output. */
		DAC_signed_data_16_w(0, 0x8000 + (dac_vol * (dac_data - 512)) / 4);
	}
}

static WRITE8_HANDLER( volume_dac_w )
{
	dac_vol = data;
}

ADDRESS_MAP_START( game_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x4000, 0x42ff) AM_RAM AM_BASE(&esripsys_pal_ram)
	AM_RANGE(0x4300, 0x4300) AM_WRITE(esripsys_bg_intensity_w)
	AM_RANGE(0x4400, 0x47ff) AM_NOP
	AM_RANGE(0x4800, 0x4bff) AM_READWRITE(g_status_r, g_status_w)
	AM_RANGE(0x4c00, 0x4fff) AM_READWRITE(g_iobus_r, g_iobus_w)
	AM_RANGE(0x5000, 0x53ff) AM_WRITE(g_ioadd_w)
	AM_RANGE(0x5400, 0x57ff) AM_NOP
	AM_RANGE(0x5c00, 0x5fff) AM_READWRITE(uart_r, uart_w)
	AM_RANGE(0x6000, 0xdfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

ADDRESS_MAP_START( frame_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x4000, 0x4fff) AM_READWRITE(fdt_r, fdt_w)
	AM_RANGE(0x6000, 0x6000) AM_READWRITE(f_status_r, f_status_w)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(frame_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

ADDRESS_MAP_START( sound_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0fff) AM_RAM
	AM_RANGE(0x2008, 0x2009) AM_READWRITE(speech_r, speech_w)
	AM_RANGE(0x200a, 0x200b) AM_WRITE(dac_w)
	AM_RANGE(0x200c, 0x200c) AM_WRITE(volume_dac_w)
	AM_RANGE(0x200d, 0x200d) AM_WRITE(control_w)
	AM_RANGE(0x200e, 0x200e) AM_READWRITE(s_200e_r, s_200e_w)
	AM_RANGE(0x200f, 0x200f) AM_READWRITE(s_200f_r, s_200f_w)
	AM_RANGE(0x2020, 0x2027) AM_READWRITE(sound_ptm_r, sound_ptm_w)
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_BANK3)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_BANK4)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

ADDRESS_MAP_START( video_cpu_map, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x000, 0xfff) AM_ROM
ADDRESS_MAP_END

static DRIVER_INIT( esripsys )
{
	UINT8* const ROM = memory_region(REGION_SOUND1);
	int i;

	fdt_a = auto_malloc(FDT_RAM_SIZE);
	fdt_b = auto_malloc(FDT_RAM_SIZE);
	cmos_ram = auto_malloc(CMOS_RAM_SIZE);

	for (i = 0; i < 3; ++i)
		sound_ptm.timer[i] = timer_alloc(sound_ptm_timeout);

	memset(fdt_a, 0, FDT_RAM_SIZE);
	memset(fdt_b, 0, FDT_RAM_SIZE);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x10000);

	input_port_set_changed_callback(0, 0x3f, keypad_interrupt, NULL);
	input_port_set_changed_callback(1, 0x1f, keypad_interrupt, NULL);
	input_port_set_changed_callback(2, 0x30, coin_interrupt, NULL);

	memory_set_bankptr(2, &ROM[0x0000]);
	memory_set_bankptr(3, &ROM[0x4000]);
	memory_set_bankptr(4, &ROM[0x8000]);

	state_save_register_global_pointer(fdt_a, FDT_RAM_SIZE);
	state_save_register_global_pointer(fdt_b, FDT_RAM_SIZE);
	state_save_register_global_pointer(cmos_ram, CMOS_RAM_SIZE);

	state_save_register_global(g_iodata);
	state_save_register_global(g_ioaddr);
	state_save_register_global(coin_latch);
	state_save_register_global(keypad_status);
	state_save_register_global(g_status);
	state_save_register_global(f_status);
	state_save_register_global(io_firq_status);
	state_save_register_global(cmos_ram_a2_0);
	state_save_register_global(cmos_ram_a10_3);

	state_save_register_global(u56a);
	state_save_register_global(u56b);
	state_save_register_global(g_to_s_latch1);
	state_save_register_global(g_to_s_latch2);
	state_save_register_global(s_to_g_latch1);
	state_save_register_global(s_to_g_latch2);
	state_save_register_global(dac_msb);
	state_save_register_global(dac_vol);

	state_save_register_global(fasel);
	state_save_register_global(fbsel);
	state_save_register_global(startup_key_pending);
}

static NVRAM_HANDLER( esripsys )
{
	if (read_or_write)
		mame_fwrite(file, cmos_ram, CMOS_RAM_SIZE);
	else if (file)
		mame_fread(file, cmos_ram, CMOS_RAM_SIZE);
	else
		memset(cmos_ram, 0x00, CMOS_RAM_SIZE);
}

static esrip_config rip_config =
{
	fdt_rip_r,
	fdt_rip_w,
	rip_status_in,
	esripsys_draw,
	REGION_PROMS
};

static MACHINE_DRIVER_START( esripsys )
	MDRV_CPU_ADD(M6809, ESRIPSYS_E_CLOCK)
	MDRV_CPU_PROGRAM_MAP(game_cpu_map, 0)

	MDRV_CPU_ADD(M6809, ESRIPSYS_E_CLOCK)
	MDRV_CPU_PROGRAM_MAP(frame_cpu_map, 0)

	MDRV_CPU_ADD(ESRIP, 40000000 / 4)
	MDRV_CPU_PROGRAM_MAP(video_cpu_map, 0)
	MDRV_CPU_CONFIG(rip_config)

	MDRV_CPU_ADD(M6809, ESRIPSYS_E_CLOCK)
	MDRV_CPU_PROGRAM_MAP(sound_cpu_map, 0)

	MDRV_INTERLEAVE(100)
	MDRV_FRAMES_PER_SECOND((double)ESRIPSYS_PIXEL_CLOCK / (ESRIPSYS_HTOTAL * ESRIPSYS_VTOTAL))
	MDRV_VBLANK_DURATION(0)
	MDRV_NVRAM_HANDLER(esripsys)
	MDRV_MACHINE_RESET(esripsys)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(ESRIPSYS_HBLANK_START, ESRIPSYS_VBLANK_START)
	MDRV_VISIBLE_AREA(0, ESRIPSYS_HBLANK_START - 1, 0, ESRIPSYS_VBLANK_START - 1)
	MDRV_PALETTE_LENGTH(0)
	MDRV_VIDEO_START(esripsys)
	MDRV_VIDEO_UPDATE(esripsys)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(TMS5220, 640000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START( turbosub )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "turbosub.u85",    0x18000, 0x4000, CRC(eabb9509) SHA1(cbfb6c5becb3fe1b4ed729e92a0f4029a5df7d67) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "turbosub.u82",    0x10000, 0x2000, CRC(de32eb6f) SHA1(90bf31a5adf261d47b4f52e93b5e97f343b7ebf0) )
	ROM_CONTINUE(                0x20000, 0x2000 )
	ROM_LOAD( "turbosub.u81",    0x12000, 0x2000, CRC(9ae09613) SHA1(9b5ada4a21473b30be98bcc461129b6ed4e0bb11) )
	ROM_CONTINUE(                0x22000, 0x2000 )
	ROM_LOAD( "turbosub.u87",    0x14000, 0x2000, CRC(ad2284f7) SHA1(8e11b8ad0a98dd1fe6ec8f7ea9e6e4f4a45d8a1b) )
	ROM_CONTINUE(                0x24000, 0x2000 )
	ROM_LOAD( "turbosub.u86",    0x16000, 0x2000, CRC(4f51e6fd) SHA1(8f51ac6412aace29279ce7b02cad45ed681c2065) )
	ROM_CONTINUE(                0x26000, 0x2000 )

	ROM_LOAD( "turbosub.u80",    0x30000, 0x2000, CRC(ff2e2870) SHA1(45f91d63ad91585482c9dd05290b204b007e3f44) )
	ROM_CONTINUE(                0x40000, 0x2000 )
	ROM_LOAD( "turbosub.u79",    0x32000, 0x2000, CRC(13680923) SHA1(14e3daa2178853cef1fd96a68305420c11fceb96) )
	ROM_CONTINUE(                0x42000, 0x2000 )
	ROM_LOAD( "turbosub.u84",    0x34000, 0x2000, CRC(7059842d) SHA1(c20a8accd3fc23bc4476e1d08798d7a80915d37c) )
	ROM_CONTINUE(                0x44000, 0x2000 )
	ROM_LOAD( "turbosub.u83",    0x36000, 0x2000, CRC(31b86fc6) SHA1(8e56e8a75f653c3c4da2c9f31f739894beb194db) )
	ROM_CONTINUE(                0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "turbosub.u63", 0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "turbosub.u66",   0xe000, 0x2000, CRC(8db3bcdb) SHA1(e6ae324ba9dad4884e1cb3d67ce099a6f4739456) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "turbosub.u69", 0x0000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )
	ROM_LOAD( "turbosub.u68", 0x4000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "turbosub.u67", 0x8000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "turbosub.u44", 0x00000, 0x4000, CRC(eaa05860) SHA1(f649891dae9354b7f2e46e6a380b52a569229d64) )
	ROM_LOAD( "turbosub.u49", 0x10000, 0x4000, CRC(b4170ac2) SHA1(bdbfc43c891c8d525dcc46fb9d05602263ab69cd) )
	ROM_LOAD( "turbosub.u54", 0x20000, 0x4000, CRC(bebf98d8) SHA1(170502bb44fc6d6bf14d8dac4778b37888c14a7b) )
	ROM_LOAD( "turbosub.u59", 0x30000, 0x4000, CRC(9c1f4397) SHA1(94335f2db2650f8b7e24fc3f92a04b73325ab164) )

	ROM_LOAD( "turbosub.u43", 0x04000, 0x4000, CRC(5d76237c) SHA1(3d50347856039e43290497348447b1c4581f3a33) )
	ROM_LOAD( "turbosub.u48", 0x14000, 0x4000, CRC(cea4e036) SHA1(4afce4f2a09adf9c83ab7188c05cd7236dea16a3) )
	ROM_LOAD( "turbosub.u53", 0x24000, 0x4000, CRC(1352d58a) SHA1(76ae86c365dd4c9e1a6c5af91c01d31e7ee35f0f) )
	ROM_LOAD( "turbosub.u58", 0x34000, 0x4000, CRC(5024d83f) SHA1(a293d92a0ae01901b5618b0250d48e3ba631dfcb) )

	ROM_LOAD( "turbosub.u42", 0x08000, 0x4000, CRC(057a1c72) SHA1(5af89b128b7818550572d02e5ff724c415fa8b8b) )
	ROM_LOAD( "turbosub.u47", 0x18000, 0x4000, CRC(10def494) SHA1(a3ba691eb2b0d782162ffc6c081761965844a3a9) )
	ROM_LOAD( "turbosub.u52", 0x28000, 0x4000, CRC(070d07d6) SHA1(4c81310cd646641a380817fedffab66e76529c97) )
	ROM_LOAD( "turbosub.u57", 0x38000, 0x4000, CRC(5ddb0458) SHA1(d1169882397f364ca38fbd563250b33d13b1a7c6) )

	ROM_LOAD( "turbosub.u41", 0x0c000, 0x4000, CRC(014bb06b) SHA1(97276ba26b60c2907e59b92cc9de5251298579cf) )
	ROM_LOAD( "turbosub.u46", 0x1c000, 0x4000, CRC(3b866e2c) SHA1(c0dd4827a18eb9f4b1055d92544beed10f01fd86) )
	ROM_LOAD( "turbosub.u51", 0x2c000, 0x4000, CRC(43cdcb5c) SHA1(3dd966daa904d3be7be63c584ba033c0e7904d5c) )
	ROM_LOAD( "turbosub.u56", 0x3c000, 0x4000, CRC(6d116adf) SHA1(f808e28cef41dc86e43d8c12966037213da87c87) )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "turbosub.u4",  0x00000, 0x4000, CRC(08303604) SHA1(f075b645d89a2d91bd9b621748906a9f9890ee60) )
	ROM_LOAD( "turbosub.u14", 0x10000, 0x4000, CRC(83b26c8d) SHA1(2dfa3b45c44652d255c402511bb3810fffb0731d) )
	ROM_LOAD( "turbosub.u24", 0x20000, 0x4000, CRC(6bbb6cb3) SHA1(d513e547a05b34076bb8261abd51301ac5f3f5d4) )
	ROM_LOAD( "turbosub.u34", 0x30000, 0x4000, CRC(7b844f4a) SHA1(82467eb7e116f9f225711a1698c151945e1de6e4) )

	ROM_LOAD( "turbosub.u3",  0x04000, 0x4000, CRC(825ef29c) SHA1(affadd0976f793b8bdbcbc4768b7de27121e7b11) )
	ROM_LOAD( "turbosub.u13", 0x14000, 0x4000, CRC(350cc17a) SHA1(b98d16be997fc0576d3206f51f29ce3e257492d3) )
	ROM_LOAD( "turbosub.u23", 0x24000, 0x4000, CRC(b1531916) SHA1(805a23f40aa875f431e835fdaceba87261c14155) )
	ROM_LOAD( "turbosub.u33", 0x34000, 0x4000, CRC(0d5130cb) SHA1(7e4e4e5ea50c581a60d15964571464029515c720) )

	ROM_LOAD( "turbosub.u2",  0x08000, 0x4000, CRC(a8b8c032) SHA1(20512a3a1f8b9c0361e6f5a7e9a50605be3ae650) )
	ROM_LOAD( "turbosub.u12", 0x18000, 0x4000, CRC(a2c4badf) SHA1(267af1be6261833211270af25045e306efffee80) )
	ROM_LOAD( "turbosub.u22", 0x28000, 0x4000, CRC(97b7cf0e) SHA1(888fb2f384a5cba8a6f7569886eb6dc27e2b024f) )
	ROM_LOAD( "turbosub.u32", 0x38000, 0x4000, CRC(b286710e) SHA1(5082db13630ba0967006619027c39ee3607b838d) )

	ROM_LOAD( "turbosub.u1",  0x0c000, 0x4000, CRC(88b0a7a9) SHA1(9012c8059cf60131efa6a0432accd87813187206) )
	ROM_LOAD( "turbosub.u11", 0x1c000, 0x4000, CRC(9f0ff723) SHA1(54b52b4ebc32f10aa32c799ac819928290e70455) )
	ROM_LOAD( "turbosub.u21", 0x2c000, 0x4000, CRC(b4122fe2) SHA1(50e8b488a7b7f739336b60a3fd8a5b14f5010b75) )
	ROM_LOAD( "turbosub.u31", 0x3c000, 0x4000, CRC(3fa15c78) SHA1(bf5cb85fc26b5045ad5acc944c917b068ace2c49) )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "turbosub.u9",  0x00000, 0x4000, CRC(9a03eadf) SHA1(25ee1ebe52f030b2fa09d76161e46540c91cbc4c) )
	ROM_LOAD( "turbosub.u19", 0x10000, 0x4000, CRC(498253b8) SHA1(dd74d4f9f19d8a746415baea604116faedb4fb31) )
	ROM_LOAD( "turbosub.u29", 0x20000, 0x4000, CRC(809c374f) SHA1(d3849eed8441e4641ffcbca7c83ee3bb16681a0b) )
	ROM_LOAD( "turbosub.u39", 0x30000, 0x4000, CRC(3e4e0681) SHA1(ac834f6823ffe835d6f149e79c1d31ae2b89e85d) )

	ROM_LOAD( "turbosub.u8",  0x04000, 0x4000, CRC(01118737) SHA1(3a8e998b80dffe82296170273dcbbe9870c5b695) )
	ROM_LOAD( "turbosub.u18", 0x14000, 0x4000, CRC(39fd8e57) SHA1(392f8a8cf58fc4813de840775d9c53561488152d) )
	ROM_LOAD( "turbosub.u28", 0x24000, 0x4000, CRC(0628586d) SHA1(e37508c2812e1c98659aaba9c495e7396842614e) )
	ROM_LOAD( "turbosub.u38", 0x34000, 0x4000, CRC(7d597a7e) SHA1(2f48faf75406ab3ff0b954040b74e68b7ca6f7a5) )

	ROM_LOAD( "turbosub.u7",  0x08000, 0x4000, CRC(50eea315) SHA1(567dbb3cb3a75a7507f4cb4748c7dd878e69d6b7) )
	ROM_LOAD( "turbosub.u17", 0x18000, 0x4000, CRC(8a9e19e6) SHA1(19067e153c0002edfd4a756f92ad75d9a0cbc3dd) )
	ROM_LOAD( "turbosub.u27", 0x28000, 0x4000, CRC(1c81a8d9) SHA1(3d13d1ccd7ec3dddf2a27600eb64b5be386e868c) )
	ROM_LOAD( "turbosub.u37", 0x38000, 0x4000, CRC(59f978cb) SHA1(e99d6378de941cad92e9702fcb18aea87acd371f) )

	ROM_LOAD( "turbosub.u6",  0x0c000, 0x4000, CRC(841e00bd) SHA1(f777cc8dd8dd7c8baa2007355a76db782a218efc) )
	ROM_LOAD( "turbosub.u16", 0x1c000, 0x4000, CRC(d3b63d81) SHA1(e86dd64825f6d9e7bebc26413f524a8962f68f2d) )
	ROM_LOAD( "turbosub.u26", 0x2c000, 0x4000, CRC(867cfe32) SHA1(549e4e557d63dfab8e8c463916512a1b422ce425) )
	ROM_LOAD( "turbosub.u36", 0x3c000, 0x4000, CRC(0d8ebc21) SHA1(7ae65edae05869376caa975ff2c778a08e8ad8a2) )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

ROM_START( turbosubc7 )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "mem6u85.bin", 0x18000, 0x4000, CRC(30016c8b) SHA1(0cd2dd7052de0eaa451ff8b0b2224180764c26de) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "mem6u82.bin", 0x10000, 0x2000, CRC(ecb01643) SHA1(32571ed9f2289b7943beb3e518e460c6552bbde7) )
	ROM_CONTINUE(            0x20000, 0x2000 )
	ROM_LOAD( "mem6u81.bin", 0x12000, 0x2000, CRC(3938bc3d) SHA1(0b6d770bdad3d40051d214efa38a8900dcd506dd) )
	ROM_CONTINUE(            0x22000, 0x2000 )
	ROM_LOAD( "mem6u87.bin", 0x14000, 0x2000, CRC(3398ddfe) SHA1(c2339440931d994f4aecf7943ba46c4e337d5bce) )
	ROM_CONTINUE(            0x24000, 0x2000 )
	ROM_LOAD( "mem6u86.bin", 0x16000, 0x2000, CRC(e4835206) SHA1(727a758a1810a1f97d75f063aac98393a5473c72) )
	ROM_CONTINUE(            0x26000, 0x2000 )

	ROM_LOAD( "mem6u80.bin", 0x30000, 0x2000, CRC(02cffdce) SHA1(18483921274eb1963ad7a64daea1d4190e5c141d) )
	ROM_CONTINUE(            0x40000, 0x2000 )
	ROM_LOAD( "mem6u79.bin", 0x32000, 0x2000, CRC(2a756db2) SHA1(c530c9a2f41de331d0d32928303c05c3312037b4) )
	ROM_CONTINUE(            0x42000, 0x2000 )
	ROM_LOAD( "mem6u84.bin", 0x34000, 0x2000, CRC(51a7f19b) SHA1(7a174b11b6f84768e3d4c14ce39974bbb3aea02d) )
	ROM_CONTINUE(            0x44000, 0x2000 )
	ROM_LOAD( "mem6u83.bin", 0x36000, 0x2000, CRC(eef7963a) SHA1(1f2f7f8fb1d68abd91f94967bb7e283004661d6d) )
	ROM_CONTINUE(            0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pcb4u63.bin", 0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "mem6u66.bin", 0xc000, 0x4000, CRC(5091bf3d) SHA1(7ab872cef1562a45f7533c16bbbae8772673465b) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "mem6u69.bin", 0x0000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )
	ROM_LOAD( "mem6u68.bin", 0x4000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "mem6u67.bin", 0x8000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "mem6u44.bin", 0x00000, 0x4000, CRC(0dbcf4a8) SHA1(aa104aa9c9a6182e46663c69193c1f414b7e2270) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "mem6u49.bin", 0x10000, 0x4000, CRC(68cf6096) SHA1(557ac00bf06878856b1e79f709d401e7a7ae50b9) )
	ROM_CONTINUE(            0x18000, 0x4000 )
	ROM_LOAD( "mem6u54.bin", 0x20000, 0x4000, CRC(561ed51e) SHA1(db4d1bb834216e6c235bc3e91f60e1cab7883769) )
	ROM_CONTINUE(            0x28000, 0x4000 )
	ROM_LOAD( "mem6u59.bin", 0x30000, 0x4000, CRC(fff98687) SHA1(f64e2c4b2fb7b2c85e7be81168169d5d5111382a) )
	ROM_CONTINUE(            0x38000, 0x4000 )

	ROM_LOAD( "mem6u43.bin", 0x04000, 0x4000, CRC(420b5bcb) SHA1(74e25f022d5ad3fdda58af5530182bd0a6db6c0c) )
	ROM_CONTINUE(            0x0c000, 0x4000 )
	ROM_LOAD( "mem6u48.bin", 0x14000, 0x4000, CRC(03c67463) SHA1(e1d8b43588948a76d48f4882be522cdcb1254bad) )
	ROM_CONTINUE(            0x1c000, 0x4000 )
	ROM_LOAD( "mem6u53.bin", 0x24000, 0x4000, CRC(5b5c4fc8) SHA1(f222631fcd515772a21af41badb3aead2043e484) )
	ROM_CONTINUE(            0x2c000, 0x4000 )
	ROM_LOAD( "mem6u58.bin", 0x34000, 0x4000, CRC(3e02ef5b) SHA1(1bd7ac2d5340198d7142c03501a6718995f28a67) )
	ROM_CONTINUE(            0x3c000, 0x4000 )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "mem6u04.bin", 0x00000, 0x4000, CRC(a42581e8) SHA1(ffab2ae5a36095ba1a71b4d1fc88589c27f819bb) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "mem6u14.bin", 0x10000, 0x4000, CRC(52b53a20) SHA1(add08ea5cb47cdcc7e8db5e94bb97aedccbc0be6) )
	ROM_CONTINUE(            0x18000, 0x4000 )
	ROM_LOAD( "mem6u24.bin", 0x20000, 0x4000, CRC(6642da40) SHA1(6ded7c04d2d57db92c243cc5af6861cb21b782b6) )
	ROM_CONTINUE(            0x28000, 0x4000 )
	ROM_LOAD( "mem6u34.bin", 0x30000, 0x4000, CRC(6e230a0a) SHA1(6855ce817feb9bda777c2d07a362722a03288a7b) )
	ROM_CONTINUE(            0x38000, 0x4000 )

	ROM_LOAD( "mem6u03.bin", 0x04000, 0x4000, CRC(ed5193ce) SHA1(00544213f604a1e7562f407c3e7ac79cba358942) )
	ROM_CONTINUE(            0x0c000, 0x4000 )
	ROM_LOAD( "mem6u13.bin", 0x14000, 0x4000, CRC(26e71525) SHA1(ba820aeb7e113439764c254e91ca83023eaf751e) )
	ROM_CONTINUE(            0x1c000, 0x4000 )
	ROM_LOAD( "mem6u23.bin", 0x24000, 0x4000, CRC(8ce207c5) SHA1(d3148f27c8285a05a77d222246208161c95a4cde) )
	ROM_CONTINUE(            0x2c000, 0x4000 )
	ROM_LOAD( "mem6u33.bin", 0x34000, 0x4000, CRC(ad12a7ae) SHA1(3f39d039c56cb96d065de4fecca98b17ab4cce3d) )
	ROM_CONTINUE(            0x3c000, 0x4000 )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "mem6u09.bin", 0x00000, 0x4000, CRC(117811ec) SHA1(9b6bef611f265e54bbc120726c3b99149cb3ca37) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "mem6u19.bin", 0x10000, 0x4000, CRC(5c9f6c06) SHA1(50973ea0675a037747ef9bb1360ec741d43a0743) )
	ROM_CONTINUE(            0x18000, 0x4000 )
	ROM_LOAD( "mem6u29.bin", 0x20000, 0x4000, CRC(e6414c30) SHA1(ec13ae40d0ad7f702c5a41bfca57b3dfef000c13) )
	ROM_CONTINUE(            0x28000, 0x4000 )
	ROM_LOAD( "mem6u39.bin", 0x30000, 0x4000, CRC(f61c0b65) SHA1(6872a775212ca36283e517ba7247f2b380fc8dd5) )
	ROM_CONTINUE(            0x38000, 0x4000 )

	ROM_LOAD( "mem6u08.bin", 0x04000, 0x4000, CRC(b3fb8861) SHA1(de0ebba8ad82dae88f934f91c745e10538e399c7) )
	ROM_CONTINUE(            0x0c000, 0x4000 )
	ROM_LOAD( "mem6u18.bin", 0x14000, 0x4000, CRC(4adff11d) SHA1(7217490fa7c1c339e0b4a865007fad44b3f026c3) )
	ROM_CONTINUE(            0x1c000, 0x4000 )
	ROM_LOAD( "mem6u28.bin", 0x24000, 0x4000, CRC(7702b849) SHA1(ba1e73a51d855c360fb5501b686f5c168246e18d) )
	ROM_CONTINUE(            0x2c000, 0x4000 )
	ROM_LOAD( "mem6u38.bin", 0x34000, 0x4000, CRC(138dbe03) SHA1(338a6ec2e0072f81a70d99ef4ddeb8410e3cdea6) )
	ROM_CONTINUE(            0x3c000, 0x4000 )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

ROM_START( turbosubc6 )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "u85", 0x18000, 0x4000, CRC(d37ccb06) SHA1(445df1caa4dd4901e474bb0903bf28e536edf493) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "u82", 0x10000, 0x2000, CRC(1596a100) SHA1(0a53d4b79245f2a51de87ec4de6db525aa342f2c) )
	ROM_CONTINUE(    0x20000, 0x2000 )
	ROM_LOAD( "u81", 0x12000, 0x2000, CRC(1c8e053d) SHA1(397f04fdf7c5dfaa33a396b1b41c015a86537ef6) )
	ROM_CONTINUE(    0x22000, 0x2000 )
	ROM_LOAD( "u87", 0x14000, 0x2000, CRC(c80d0512) SHA1(aedd829edd2cb214fa30ae2fe25ad7590c86971b) )
	ROM_CONTINUE(    0x24000, 0x2000 )
	ROM_LOAD( "u86", 0x16000, 0x2000, CRC(8af137d3) SHA1(12768d14b18401d07a793de3412da059b5a33699) )
	ROM_CONTINUE(    0x26000, 0x2000 )

	ROM_LOAD( "u80", 0x30000, 0x2000, CRC(6f53b658) SHA1(39841e4b0a6809ad061a07adcdb8d92fd7652959) )
	ROM_CONTINUE(    0x40000, 0x2000 )
	ROM_LOAD( "u79", 0x32000, 0x2000, CRC(aa6f1db6) SHA1(70cacedb57f3c5646181e26c355f87f1cea1d651) )
	ROM_CONTINUE(    0x42000, 0x2000 )
	ROM_LOAD( "u84", 0x34000, 0x2000, CRC(e856323f) SHA1(d973f8efa3a1f5907b8c09b58043d7b41ff3f0c1) )
	ROM_CONTINUE(    0x44000, 0x2000 )
	ROM_LOAD( "u83", 0x36000, 0x2000, CRC(056fc173) SHA1(426bcea3c2420b8df036122ebb6fc80af89e63d2) )
	ROM_CONTINUE(    0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "u63",    0xe000, 0x2000, CRC(e85216d4) SHA1(7f61a93c52a31782116e9825d0aefa58ca3720b9) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "turbosub.u66", 0xe000, 0x2000, CRC(8db3bcdb) SHA1(e6ae324ba9dad4884e1cb3d67ce099a6f4739456) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "turbosub.u69", 0x0000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )
	ROM_LOAD( "turbosub.u68", 0x4000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "turbosub.u67", 0x8000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "turbosub.u44", 0x00000, 0x4000, CRC(eaa05860) SHA1(f649891dae9354b7f2e46e6a380b52a569229d64) )
	ROM_LOAD( "turbosub.u49", 0x10000, 0x4000, CRC(b4170ac2) SHA1(bdbfc43c891c8d525dcc46fb9d05602263ab69cd) )
	ROM_LOAD( "turbosub.u54", 0x20000, 0x4000, CRC(bebf98d8) SHA1(170502bb44fc6d6bf14d8dac4778b37888c14a7b) )
	ROM_LOAD( "turbosub.u59", 0x30000, 0x4000, CRC(9c1f4397) SHA1(94335f2db2650f8b7e24fc3f92a04b73325ab164) )

	ROM_LOAD( "turbosub.u43", 0x04000, 0x4000, CRC(5d76237c) SHA1(3d50347856039e43290497348447b1c4581f3a33) )
	ROM_LOAD( "turbosub.u48", 0x14000, 0x4000, CRC(cea4e036) SHA1(4afce4f2a09adf9c83ab7188c05cd7236dea16a3) )
	ROM_LOAD( "turbosub.u53", 0x24000, 0x4000, CRC(1352d58a) SHA1(76ae86c365dd4c9e1a6c5af91c01d31e7ee35f0f) )
	ROM_LOAD( "turbosub.u58", 0x34000, 0x4000, CRC(5024d83f) SHA1(a293d92a0ae01901b5618b0250d48e3ba631dfcb) )

	ROM_LOAD( "turbosub.u42", 0x08000, 0x4000, CRC(057a1c72) SHA1(5af89b128b7818550572d02e5ff724c415fa8b8b) )
	ROM_LOAD( "turbosub.u47", 0x18000, 0x4000, CRC(10def494) SHA1(a3ba691eb2b0d782162ffc6c081761965844a3a9) )
	ROM_LOAD( "turbosub.u52", 0x28000, 0x4000, CRC(070d07d6) SHA1(4c81310cd646641a380817fedffab66e76529c97) )
	ROM_LOAD( "turbosub.u57", 0x38000, 0x4000, CRC(5ddb0458) SHA1(d1169882397f364ca38fbd563250b33d13b1a7c6) )

	ROM_LOAD( "turbosub.u41", 0x0c000, 0x4000, CRC(014bb06b) SHA1(97276ba26b60c2907e59b92cc9de5251298579cf) )
	ROM_LOAD( "turbosub.u46", 0x1c000, 0x4000, CRC(3b866e2c) SHA1(c0dd4827a18eb9f4b1055d92544beed10f01fd86) )
	ROM_LOAD( "turbosub.u51", 0x2c000, 0x4000, CRC(43cdcb5c) SHA1(3dd966daa904d3be7be63c584ba033c0e7904d5c) )
	ROM_LOAD( "turbosub.u56", 0x3c000, 0x4000, CRC(6d116adf) SHA1(f808e28cef41dc86e43d8c12966037213da87c87) )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "turbosub.u4",  0x00000, 0x4000, CRC(08303604) SHA1(f075b645d89a2d91bd9b621748906a9f9890ee60) )
	ROM_LOAD( "turbosub.u14", 0x10000, 0x4000, CRC(83b26c8d) SHA1(2dfa3b45c44652d255c402511bb3810fffb0731d) )
	ROM_LOAD( "turbosub.u24", 0x20000, 0x4000, CRC(6bbb6cb3) SHA1(d513e547a05b34076bb8261abd51301ac5f3f5d4) )
	ROM_LOAD( "turbosub.u34", 0x30000, 0x4000, CRC(7b844f4a) SHA1(82467eb7e116f9f225711a1698c151945e1de6e4) )

	ROM_LOAD( "turbosub.u3",  0x04000, 0x4000, CRC(825ef29c) SHA1(affadd0976f793b8bdbcbc4768b7de27121e7b11) )
	ROM_LOAD( "turbosub.u13", 0x14000, 0x4000, CRC(350cc17a) SHA1(b98d16be997fc0576d3206f51f29ce3e257492d3) )
	ROM_LOAD( "turbosub.u23", 0x24000, 0x4000, CRC(b1531916) SHA1(805a23f40aa875f431e835fdaceba87261c14155) )
	ROM_LOAD( "turbosub.u33", 0x34000, 0x4000, CRC(0d5130cb) SHA1(7e4e4e5ea50c581a60d15964571464029515c720) )

	ROM_LOAD( "turbosub.u2",  0x08000, 0x4000, CRC(a8b8c032) SHA1(20512a3a1f8b9c0361e6f5a7e9a50605be3ae650) )
	ROM_LOAD( "turbosub.u12", 0x18000, 0x4000, CRC(a2c4badf) SHA1(267af1be6261833211270af25045e306efffee80) )
	ROM_LOAD( "turbosub.u22", 0x28000, 0x4000, CRC(97b7cf0e) SHA1(888fb2f384a5cba8a6f7569886eb6dc27e2b024f) )
	ROM_LOAD( "turbosub.u32", 0x38000, 0x4000, CRC(b286710e) SHA1(5082db13630ba0967006619027c39ee3607b838d) )

	ROM_LOAD( "turbosub.u1",  0x0c000, 0x4000, CRC(88b0a7a9) SHA1(9012c8059cf60131efa6a0432accd87813187206) )
	ROM_LOAD( "turbosub.u11", 0x1c000, 0x4000, CRC(9f0ff723) SHA1(54b52b4ebc32f10aa32c799ac819928290e70455) )
	ROM_LOAD( "turbosub.u21", 0x2c000, 0x4000, CRC(b4122fe2) SHA1(50e8b488a7b7f739336b60a3fd8a5b14f5010b75) )
	ROM_LOAD( "turbosub.u31", 0x3c000, 0x4000, CRC(3fa15c78) SHA1(bf5cb85fc26b5045ad5acc944c917b068ace2c49) )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "turbosub.u9",  0x00000, 0x4000, CRC(9a03eadf) SHA1(25ee1ebe52f030b2fa09d76161e46540c91cbc4c) )
	ROM_LOAD( "turbosub.u19", 0x10000, 0x4000, CRC(498253b8) SHA1(dd74d4f9f19d8a746415baea604116faedb4fb31) )
	ROM_LOAD( "turbosub.u29", 0x20000, 0x4000, CRC(809c374f) SHA1(d3849eed8441e4641ffcbca7c83ee3bb16681a0b) )
	ROM_LOAD( "turbosub.u39", 0x30000, 0x4000, CRC(3e4e0681) SHA1(ac834f6823ffe835d6f149e79c1d31ae2b89e85d) )

	ROM_LOAD( "turbosub.u8",  0x04000, 0x4000, CRC(01118737) SHA1(3a8e998b80dffe82296170273dcbbe9870c5b695) )
	ROM_LOAD( "turbosub.u18", 0x14000, 0x4000, CRC(39fd8e57) SHA1(392f8a8cf58fc4813de840775d9c53561488152d) )
	ROM_LOAD( "turbosub.u28", 0x24000, 0x4000, CRC(0628586d) SHA1(e37508c2812e1c98659aaba9c495e7396842614e) )
	ROM_LOAD( "turbosub.u38", 0x34000, 0x4000, CRC(7d597a7e) SHA1(2f48faf75406ab3ff0b954040b74e68b7ca6f7a5) )

	ROM_LOAD( "turbosub.u7",  0x08000, 0x4000, CRC(50eea315) SHA1(567dbb3cb3a75a7507f4cb4748c7dd878e69d6b7) )
	ROM_LOAD( "turbosub.u17", 0x18000, 0x4000, CRC(8a9e19e6) SHA1(19067e153c0002edfd4a756f92ad75d9a0cbc3dd) )
	ROM_LOAD( "turbosub.u27", 0x28000, 0x4000, CRC(1c81a8d9) SHA1(3d13d1ccd7ec3dddf2a27600eb64b5be386e868c) )
	ROM_LOAD( "turbosub.u37", 0x38000, 0x4000, CRC(59f978cb) SHA1(e99d6378de941cad92e9702fcb18aea87acd371f) )

	ROM_LOAD( "turbosub.u6",  0x0c000, 0x4000, CRC(841e00bd) SHA1(f777cc8dd8dd7c8baa2007355a76db782a218efc) )
	ROM_LOAD( "turbosub.u16", 0x1c000, 0x4000, CRC(d3b63d81) SHA1(e86dd64825f6d9e7bebc26413f524a8962f68f2d) )
	ROM_LOAD( "turbosub.u26", 0x2c000, 0x4000, CRC(867cfe32) SHA1(549e4e557d63dfab8e8c463916512a1b422ce425) )
	ROM_LOAD( "turbosub.u36", 0x3c000, 0x4000, CRC(0d8ebc21) SHA1(7ae65edae05869376caa975ff2c778a08e8ad8a2) )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

ROM_START( turbosubc8 )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "tsc8.u85", 0x18000, 0x4000, CRC(f27d048e) SHA1(a98dc42fb8eb7ee320c7b77f9be272448ec8ec9b) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "tsc8.u82", 0x10000, 0x2000, CRC(059c48de) SHA1(5735967287d9b6a48bb474cde0fd00252b11cf90) )
	ROM_CONTINUE(         0x20000, 0x2000 )
	ROM_LOAD( "tsc8.u81", 0x12000, 0x2000, CRC(3676ab4e) SHA1(bc357a00b59987d041ed59712531e248b9e0c7dd) )
	ROM_CONTINUE(         0x22000, 0x2000 )
	ROM_LOAD( "tsc8.u87", 0x14000, 0x2000, CRC(1a1c4d84) SHA1(8e26ac98464f68f1a32028f96ce84893e93ecca0) )
	ROM_CONTINUE(         0x24000, 0x2000 )
	ROM_LOAD( "tsc8.u86", 0x16000, 0x2000, CRC(84ed1936) SHA1(cb4f2b404dee67a45151269513446ee21f701f0a) )
	ROM_CONTINUE(         0x26000, 0x2000 )

	ROM_LOAD( "tsc8.u80", 0x30000, 0x2000, CRC(9c24469b) SHA1(3f05da94e5736d8efcf660b654f0e94f26d3836b) )
	ROM_CONTINUE(         0x40000, 0x2000 )
	ROM_LOAD( "tsc8.u79", 0x32000, 0x2000, CRC(a547128b) SHA1(fb09814fdefbb7a91a3f459a951df5d694e8541d) )
	ROM_CONTINUE(         0x42000, 0x2000 )
	ROM_LOAD( "tsc8.u84", 0x34000, 0x2000, CRC(f3b5353d) SHA1(88fb01e5ab72bc01aa1f82526c7454db17fa4391) )
	ROM_CONTINUE(         0x44000, 0x2000 )
	ROM_LOAD( "tsc8.u83", 0x36000, 0x2000, CRC(48ae5676) SHA1(bd3bd025d82a3b034413e230e837f713bdf23c72) )
	ROM_CONTINUE(         0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pcb4u63.bin", 0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "mem6u66.bin", 0xc000, 0x4000, CRC(5091bf3d) SHA1(7ab872cef1562a45f7533c16bbbae8772673465b) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "mem6u69.bin", 0x0000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )
	ROM_LOAD( "mem6u68.bin", 0x4000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "mem6u67.bin", 0x8000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "mem6u44.bin", 0x00000, 0x4000, CRC(0dbcf4a8) SHA1(aa104aa9c9a6182e46663c69193c1f414b7e2270) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "mem6u49.bin", 0x10000, 0x4000, CRC(68cf6096) SHA1(557ac00bf06878856b1e79f709d401e7a7ae50b9) )
	ROM_CONTINUE(            0x18000, 0x4000 )
	ROM_LOAD( "mem6u54.bin", 0x20000, 0x4000, CRC(561ed51e) SHA1(db4d1bb834216e6c235bc3e91f60e1cab7883769) )
	ROM_CONTINUE(            0x28000, 0x4000 )
	ROM_LOAD( "mem6u59.bin", 0x30000, 0x4000, CRC(fff98687) SHA1(f64e2c4b2fb7b2c85e7be81168169d5d5111382a) )
	ROM_CONTINUE(            0x38000, 0x4000 )

	ROM_LOAD( "mem6u43.bin", 0x04000, 0x4000, CRC(420b5bcb) SHA1(74e25f022d5ad3fdda58af5530182bd0a6db6c0c) )
	ROM_CONTINUE(            0x0c000, 0x4000 )
	ROM_LOAD( "mem6u48.bin", 0x14000, 0x4000, CRC(03c67463) SHA1(e1d8b43588948a76d48f4882be522cdcb1254bad) )
	ROM_CONTINUE(            0x1c000, 0x4000 )
	ROM_LOAD( "mem6u53.bin", 0x24000, 0x4000, CRC(5b5c4fc8) SHA1(f222631fcd515772a21af41badb3aead2043e484) )
	ROM_CONTINUE(            0x2c000, 0x4000 )
	ROM_LOAD( "mem6u58.bin", 0x34000, 0x4000, CRC(3e02ef5b) SHA1(1bd7ac2d5340198d7142c03501a6718995f28a67) )
	ROM_CONTINUE(            0x3c000, 0x4000 )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "mem6u04.bin", 0x00000, 0x4000, CRC(a42581e8) SHA1(ffab2ae5a36095ba1a71b4d1fc88589c27f819bb) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "mem6u14.bin", 0x10000, 0x4000, CRC(52b53a20) SHA1(add08ea5cb47cdcc7e8db5e94bb97aedccbc0be6) )
	ROM_CONTINUE(            0x18000, 0x4000 )
	ROM_LOAD( "mem6u24.bin", 0x20000, 0x4000, CRC(6642da40) SHA1(6ded7c04d2d57db92c243cc5af6861cb21b782b6) )
	ROM_CONTINUE(            0x28000, 0x4000 )
	ROM_LOAD( "mem6u34.bin", 0x30000, 0x4000, CRC(6e230a0a) SHA1(6855ce817feb9bda777c2d07a362722a03288a7b) )
	ROM_CONTINUE(            0x38000, 0x4000 )

	ROM_LOAD( "mem6u03.bin", 0x04000, 0x4000, CRC(ed5193ce) SHA1(00544213f604a1e7562f407c3e7ac79cba358942) )
	ROM_CONTINUE(            0x0c000, 0x4000 )
	ROM_LOAD( "mem6u13.bin", 0x14000, 0x4000, CRC(26e71525) SHA1(ba820aeb7e113439764c254e91ca83023eaf751e) )
	ROM_CONTINUE(            0x1c000, 0x4000 )
	ROM_LOAD( "mem6u23.bin", 0x24000, 0x4000, CRC(8ce207c5) SHA1(d3148f27c8285a05a77d222246208161c95a4cde) )
	ROM_CONTINUE(            0x2c000, 0x4000 )
	ROM_LOAD( "mem6u33.bin", 0x34000, 0x4000, CRC(ad12a7ae) SHA1(3f39d039c56cb96d065de4fecca98b17ab4cce3d) )
	ROM_CONTINUE(            0x3c000, 0x4000 )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "mem6u09.bin", 0x00000, 0x4000, CRC(117811ec) SHA1(9b6bef611f265e54bbc120726c3b99149cb3ca37) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "mem6u19.bin", 0x10000, 0x4000, CRC(5c9f6c06) SHA1(50973ea0675a037747ef9bb1360ec741d43a0743) )
	ROM_CONTINUE(            0x18000, 0x4000 )
	ROM_LOAD( "mem6u29.bin", 0x20000, 0x4000, CRC(e6414c30) SHA1(ec13ae40d0ad7f702c5a41bfca57b3dfef000c13) )
	ROM_CONTINUE(            0x28000, 0x4000 )
	ROM_LOAD( "mem6u39.bin", 0x30000, 0x4000, CRC(f61c0b65) SHA1(6872a775212ca36283e517ba7247f2b380fc8dd5) )
	ROM_CONTINUE(            0x38000, 0x4000 )

	ROM_LOAD( "mem6u08.bin", 0x04000, 0x4000, CRC(b3fb8861) SHA1(de0ebba8ad82dae88f934f91c745e10538e399c7) )
	ROM_CONTINUE(            0x0c000, 0x4000 )
	ROM_LOAD( "mem6u18.bin", 0x14000, 0x4000, CRC(4adff11d) SHA1(7217490fa7c1c339e0b4a865007fad44b3f026c3) )
	ROM_CONTINUE(            0x1c000, 0x4000 )
	ROM_LOAD( "mem6u28.bin", 0x24000, 0x4000, CRC(7702b849) SHA1(ba1e73a51d855c360fb5501b686f5c168246e18d) )
	ROM_CONTINUE(            0x2c000, 0x4000 )
	ROM_LOAD( "mem6u38.bin", 0x34000, 0x4000, CRC(138dbe03) SHA1(338a6ec2e0072f81a70d99ef4ddeb8410e3cdea6) )
	ROM_CONTINUE(            0x3c000, 0x4000 )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

ROM_START( turbosubb8 )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "tsb8.u85", 0x18000, 0x4000, CRC(cd920709) SHA1(50c7755be4c37b1b59660c28b859293a71017bc3) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "tsb8.u82", 0x10000, 0x2000, CRC(9af7c67c) SHA1(55e243c05989cbba5a60404f699292adf4e389e6) )
	ROM_CONTINUE(         0x20000, 0x2000 )
	ROM_LOAD( "tsb8.u81", 0x12000, 0x2000, CRC(4ba55138) SHA1(515ad97e958fe5bf7f8a896eb2a52abc6bb27c29) )
	ROM_CONTINUE(         0x22000, 0x2000 )
	ROM_LOAD( "tsb8.u87", 0x14000, 0x2000, CRC(392c57c3) SHA1(8d69c9d08cc54ce321164d8067bed33ff37e0c11) )
	ROM_CONTINUE(         0x24000, 0x2000 )
	ROM_LOAD( "tsb8.u86", 0x16000, 0x2000, CRC(0bcaa52e) SHA1(0a379adedfc9254b26e4d952093ebba27cadf5fa) )
	ROM_CONTINUE(         0x26000, 0x2000 )

	ROM_LOAD( "tsb8.u80", 0x30000, 0x2000, CRC(e7efa4bf) SHA1(0468ac565b8783a6374eba6c941c273d32f0dd49) )
	ROM_CONTINUE(         0x40000, 0x2000 )
	ROM_LOAD( "tsb8.u79", 0x32000, 0x2000, CRC(c74702dc) SHA1(82d4242fdb1100d6fbec4a60933d629cc4e3db11) )
	ROM_CONTINUE(         0x42000, 0x2000 )
	ROM_LOAD( "tsb8.u84", 0x34000, 0x2000, CRC(5af1f8fc) SHA1(a0389b84101e2bc498e1efe64900ae23e121a883) )
	ROM_CONTINUE(         0x44000, 0x2000 )
	ROM_LOAD( "tsb8.u83", 0x36000, 0x2000, CRC(24c3877a) SHA1(8e287a61c01a684af858d1ad6f0082137437a6c1) )
	ROM_CONTINUE(         0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "turbosub.u63", 0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "tsb8.u66",   0xc000, 0x4000, CRC(5091bf3d) SHA1(7ab872cef1562a45f7533c16bbbae8772673465b) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "tsb8.u69", 0x0000, 0x4000, CRC(6dda8721) SHA1(c69e2ef1527559bc6d54a954c6b623bff832006c) )
	ROM_LOAD( "tsb8.u68", 0x4000, 0x4000, CRC(db86b699) SHA1(69059305f5650fc94bccd22c1486417554ac9c41) )
	ROM_LOAD( "tsb8.u67", 0x8000, 0x4000, CRC(bff729ea) SHA1(b95e37737a1dc8480c32e8abeff36b7b56fa12a1) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "tsb8.u44", 0x00000, 0x4000, CRC(bc1f9dea) SHA1(a581b8c75c08f73571c98122c0c0b3e05757db84) )
	ROM_LOAD( "tsb8.u49", 0x10000, 0x4000, CRC(4f512477) SHA1(c91d44f6259dca0e28f21bccce3e68906a31695d) )
	ROM_LOAD( "tsb8.u54", 0x20000, 0x4000, CRC(6d811d60) SHA1(b8945e9a975ada89053f482a8ec0320ac659238a) )
	ROM_LOAD( "tsb8.u59", 0x30000, 0x4000, CRC(0500f3d0) SHA1(459c45d4d552c02cabf9b1e1d927415b9b04ff32) )

	ROM_LOAD( "tsb8.u43", 0x04000, 0x4000, CRC(38fa7f84) SHA1(d4f7d42742e265973a40094657011cff201cbbe7) )
	ROM_LOAD( "tsb8.u48", 0x14000, 0x4000, CRC(e2b73a98) SHA1(e46158e4730f3d7f83e322680e4e5abf4c866af8) )
	ROM_LOAD( "tsb8.u53", 0x24000, 0x4000, CRC(cb2100a8) SHA1(cc7e646b2fb66e685fb2d1bf86ec2fc84358c431) )
	ROM_LOAD( "tsb8.u58", 0x34000, 0x4000, CRC(255df9d9) SHA1(aa89ffe4f0780977f829b27c35d2c34806450ebf) )

	ROM_LOAD( "tsb8.u42", 0x08000, 0x4000, CRC(00bfe74c) SHA1(314a6ed28d07e24d56eeef43f550f4bdf4cc789a) )
	ROM_LOAD( "tsb8.u47", 0x18000, 0x4000, CRC(eabc8e08) SHA1(44097f6320417f9538dcee45dec4d0402e4d8ab9) )
	ROM_LOAD( "tsb8.u52", 0x28000, 0x4000, CRC(e81f381f) SHA1(eae0fd3e14b32e8a32c656090e5bbb7dc55ab256) )
	ROM_LOAD( "tsb8.u57", 0x38000, 0x4000, CRC(76880da6) SHA1(ebbe4a1f84d441172984750c7b66c1a13fe736cc) )

	ROM_LOAD( "tsb8.u41", 0x0c000, 0x4000, CRC(93210509) SHA1(9dd1a850bf75349f6f11c51ccea21d28dd2a1251) )
	ROM_LOAD( "tsb8.u46", 0x1c000, 0x4000, CRC(e8f23e3e) SHA1(e2d95e7cbad46fd20d19592de0c6d26e5e7073e3) )
	ROM_LOAD( "tsb8.u51", 0x2c000, 0x4000, CRC(88fb898c) SHA1(e0b73a909104a64195a58394c21834266ad48744) )
	ROM_LOAD( "tsb8.u56", 0x3c000, 0x4000, CRC(9398c41a) SHA1(84730b1e9b489b35df14fc55367f5647ee29b57a) )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "tsb8.u4",  0x00000, 0x4000, CRC(41178df4) SHA1(b0c14def4f0f11678345eedd05be02e1b636e32a) )
	ROM_LOAD( "tsb8.u14", 0x10000, 0x4000, CRC(7ca6ea75) SHA1(e8d21a7e8e28736b0e876571794ea64721bde7a5) )
	ROM_LOAD( "tsb8.u24", 0x20000, 0x4000, CRC(d73de17f) SHA1(91608fc137f1f1383e55d3e02c10b0627677af57) )
	ROM_LOAD( "tsb8.u34", 0x30000, 0x4000, CRC(383978eb) SHA1(99a4cf9abd987692e962ac267d7446fadea04a61) )

	ROM_LOAD( "tsb8.u3",  0x04000, 0x4000, CRC(7d43d0d9) SHA1(06da1ccc079328841e9275fb8a6529598f978b1a) )
	ROM_LOAD( "tsb8.u13", 0x14000, 0x4000, CRC(af47888e) SHA1(8a95432c40620d31a2812b38059a38785101767a) )
	ROM_LOAD( "tsb8.u23", 0x24000, 0x4000, CRC(cc6250fc) SHA1(5dcf148d4eae4b76d2402f93dd2c740ac35365ed) )
	ROM_LOAD( "tsb8.u33", 0x34000, 0x4000, CRC(76352bb4) SHA1(9ef653b25b0bdc36e8b2b5e162a965373a47fed6) )

	ROM_LOAD( "tsb8.u2",  0x08000, 0x4000, CRC(f379b9d6) SHA1(3456b51372a184c102bd9e67a88c152f00cc5b75) )
	ROM_LOAD( "tsb8.u12", 0x18000, 0x4000, CRC(781be78a) SHA1(4fd30559e15f1c5bedfd267e0a8f496a67365eeb) )
	ROM_LOAD( "tsb8.u22", 0x28000, 0x4000, CRC(9665210b) SHA1(41af3357b242bcddf0fa81b9dbcce93d51117177) )
	ROM_LOAD( "tsb8.u32", 0x38000, 0x4000, CRC(3139bcaa) SHA1(cd5410d5ed72a7daa686ef8b79b0aab4a6029d9a) )

	ROM_LOAD( "tsb8.u1",  0x0c000, 0x4000, CRC(0f82059a) SHA1(6fcaa5a85f63a7fbe7a775349fa45d1de86664b6) )
	ROM_LOAD( "tsb8.u11", 0x1c000, 0x4000, CRC(fb9957ea) SHA1(636a118500f531408144ec6bdd06f6ab124f70b3) )
	ROM_LOAD( "tsb8.u21", 0x2c000, 0x4000, CRC(2fe2dea4) SHA1(74f3ac4159b488a8908a70e486d8f19729c1cc6e) )
	ROM_LOAD( "tsb8.u31", 0x3c000, 0x4000, CRC(ec2fc941) SHA1(996856127a54c81944c0b5615affe6ab9ffd1c25) )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "tsb8.u9",  0x00000, 0x4000, CRC(8efe1d3b) SHA1(47671be16922177fa78325ffeccdf09326b66298) )
	ROM_LOAD( "tsb8.u19", 0x10000, 0x4000, CRC(aedfadf8) SHA1(b85706af6ea4d604671345016d8101979311e7f6) )
	ROM_LOAD( "tsb8.u29", 0x20000, 0x4000, CRC(6e33feca) SHA1(5c6dfef9788712afcef2f8d53c816103f58b877d) )
	ROM_LOAD( "tsb8.u39", 0x30000, 0x4000, CRC(6bd55a6e) SHA1(4c7dacfd58fc8e308b758ebe1f36254b061d3fa8) )

	ROM_LOAD( "tsb8.u8",  0x04000, 0x4000, CRC(0fe731d0) SHA1(81fe7b499947b9aa11f5ff68ae8433ca1fd64776) )
	ROM_LOAD( "tsb8.u18", 0x14000, 0x4000, CRC(5e5dfcc6) SHA1(20a43917093fb7639b1cbe91623a5072addd2926) )
	ROM_LOAD( "tsb8.u28", 0x24000, 0x4000, CRC(a132c340) SHA1(0bc8dda5b184a784b42a67e9a23bb9a2894a4c4f) )
	ROM_LOAD( "tsb8.u38", 0x34000, 0x4000, CRC(d6bd41ea) SHA1(95cf80b5b717aba974ac025fa4e5d70cb68b3243) )

	ROM_LOAD( "tsb8.u7",  0x08000, 0x4000, CRC(ba1cec21) SHA1(3b375a654c443787fa30a9dc9e34e4586d32a303) )
	ROM_LOAD( "tsb8.u17", 0x18000, 0x4000, CRC(7b0bee27) SHA1(1958c4b8a12bdda90b93bc67e16ba23f8f0313bb) )
	ROM_LOAD( "tsb8.u27", 0x28000, 0x4000, CRC(393ce06e) SHA1(e45d9f71667b133310e23976ddc55c5d50639edf) )
	ROM_LOAD( "tsb8.u37", 0x38000, 0x4000, CRC(b3455eeb) SHA1(733ab1f43d463946399b7f4df6d393ccd40409f1) )

	ROM_LOAD( "tsb8.u6",  0x0c000, 0x4000, CRC(11b91b26) SHA1(abb114c489eebefbbb6653178f4d4f7235d7710a) )
	ROM_LOAD( "tsb8.u16", 0x1c000, 0x4000, CRC(1e36ac5e) SHA1(e32525dc66d974d68cde51471f6943fff78d07d3) )
	ROM_LOAD( "tsb8.u26", 0x2c000, 0x4000, CRC(2603a199) SHA1(8863ae79a227d763135b75467345027efd59590a) )
	ROM_LOAD( "tsb8.u36", 0x3c000, 0x4000, CRC(87f7cd28) SHA1(59f3cf5bc7c8278aae29b9a3da3b41ec1900065d) )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

ROM_START( turbosubc5s )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "tsc5s.u85", 0x18000, 0x4000, CRC(fac9b606) SHA1(0bff3b01e9efe9dfb7e971488f596c8d02af6f20) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "tsc5s.u82", 0x10000, 0x2000, CRC(fb29b769) SHA1(76a5ac7c011cf3c88c3b25e98bab24298a37c26b) )
	ROM_CONTINUE(          0x20000, 0x2000 )
	ROM_LOAD( "tsc5s.u81", 0x12000, 0x2000, CRC(f181c64e) SHA1(de92c6922b5ff6130eb0a4968c14783d35d3b4f5) )
	ROM_CONTINUE(          0x22000, 0x2000 )
	ROM_LOAD( "tsc5s.u87", 0x14000, 0x2000, CRC(7a0d8ea6) SHA1(9e08adec08c932fc4e93ee28cdb979b3aa8fd140) )
	ROM_CONTINUE(          0x24000, 0x2000 )
	ROM_LOAD( "tsc5s.u86", 0x16000, 0x2000, CRC(aa6a3b01) SHA1(f938968e7828a813b48077d0320c438827bf399b) )
	ROM_CONTINUE(          0x26000, 0x2000 )

	ROM_LOAD( "tsc5s.u80", 0x30000, 0x2000, CRC(48573252) SHA1(2a226cdd96f995de46e2d92f2808e8b063ec0fe3) )
	ROM_CONTINUE(          0x40000, 0x2000 )
	ROM_LOAD( "tsc5s.u79", 0x32000, 0x2000, CRC(456ea95d) SHA1(b2ed272a0b51459005b4c70cc4556f833d05c0dc) )
	ROM_CONTINUE(          0x42000, 0x2000 )
	ROM_LOAD( "tsc5s.u84", 0x34000, 0x2000, CRC(6c788f77) SHA1(45fb4907fb96d0c63ff90054099ac3bea64fadd3) )
	ROM_CONTINUE(          0x44000, 0x2000 )
	ROM_LOAD( "tsc5s.u83", 0x36000, 0x2000, CRC(e8419dbd) SHA1(2252a1d5ebdf504f75bf43492475c8f9d040caf2) )
	ROM_CONTINUE(          0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "turbosub.u63", 0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "turbosub.u66",   0xe000, 0x2000, CRC(8db3bcdb) SHA1(e6ae324ba9dad4884e1cb3d67ce099a6f4739456) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "turbosub.u69", 0x0000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )
	ROM_LOAD( "turbosub.u68", 0x4000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "turbosub.u67", 0x8000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "turbosub.u44", 0x00000, 0x4000, CRC(eaa05860) SHA1(f649891dae9354b7f2e46e6a380b52a569229d64) )
	ROM_LOAD( "turbosub.u49", 0x10000, 0x4000, CRC(b4170ac2) SHA1(bdbfc43c891c8d525dcc46fb9d05602263ab69cd) )
	ROM_LOAD( "turbosub.u54", 0x20000, 0x4000, CRC(bebf98d8) SHA1(170502bb44fc6d6bf14d8dac4778b37888c14a7b) )
	ROM_LOAD( "turbosub.u59", 0x30000, 0x4000, CRC(9c1f4397) SHA1(94335f2db2650f8b7e24fc3f92a04b73325ab164) )

	ROM_LOAD( "turbosub.u43", 0x04000, 0x4000, CRC(5d76237c) SHA1(3d50347856039e43290497348447b1c4581f3a33) )
	ROM_LOAD( "turbosub.u48", 0x14000, 0x4000, CRC(cea4e036) SHA1(4afce4f2a09adf9c83ab7188c05cd7236dea16a3) )
	ROM_LOAD( "turbosub.u53", 0x24000, 0x4000, CRC(1352d58a) SHA1(76ae86c365dd4c9e1a6c5af91c01d31e7ee35f0f) )
	ROM_LOAD( "turbosub.u58", 0x34000, 0x4000, CRC(5024d83f) SHA1(a293d92a0ae01901b5618b0250d48e3ba631dfcb) )

	ROM_LOAD( "turbosub.u42", 0x08000, 0x4000, CRC(057a1c72) SHA1(5af89b128b7818550572d02e5ff724c415fa8b8b) )
	ROM_LOAD( "turbosub.u47", 0x18000, 0x4000, CRC(10def494) SHA1(a3ba691eb2b0d782162ffc6c081761965844a3a9) )
	ROM_LOAD( "turbosub.u52", 0x28000, 0x4000, CRC(070d07d6) SHA1(4c81310cd646641a380817fedffab66e76529c97) )
	ROM_LOAD( "turbosub.u57", 0x38000, 0x4000, CRC(5ddb0458) SHA1(d1169882397f364ca38fbd563250b33d13b1a7c6) )

	ROM_LOAD( "turbosub.u41", 0x0c000, 0x4000, CRC(014bb06b) SHA1(97276ba26b60c2907e59b92cc9de5251298579cf) )
	ROM_LOAD( "turbosub.u46", 0x1c000, 0x4000, CRC(3b866e2c) SHA1(c0dd4827a18eb9f4b1055d92544beed10f01fd86) )
	ROM_LOAD( "turbosub.u51", 0x2c000, 0x4000, CRC(43cdcb5c) SHA1(3dd966daa904d3be7be63c584ba033c0e7904d5c) )
	ROM_LOAD( "turbosub.u56", 0x3c000, 0x4000, CRC(6d116adf) SHA1(f808e28cef41dc86e43d8c12966037213da87c87) )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "turbosub.u4",  0x00000, 0x4000, CRC(08303604) SHA1(f075b645d89a2d91bd9b621748906a9f9890ee60) )
	ROM_LOAD( "turbosub.u14", 0x10000, 0x4000, CRC(83b26c8d) SHA1(2dfa3b45c44652d255c402511bb3810fffb0731d) )
	ROM_LOAD( "turbosub.u24", 0x20000, 0x4000, CRC(6bbb6cb3) SHA1(d513e547a05b34076bb8261abd51301ac5f3f5d4) )
	ROM_LOAD( "turbosub.u34", 0x30000, 0x4000, CRC(7b844f4a) SHA1(82467eb7e116f9f225711a1698c151945e1de6e4) )

	ROM_LOAD( "turbosub.u3",  0x04000, 0x4000, CRC(825ef29c) SHA1(affadd0976f793b8bdbcbc4768b7de27121e7b11) )
	ROM_LOAD( "turbosub.u13", 0x14000, 0x4000, CRC(350cc17a) SHA1(b98d16be997fc0576d3206f51f29ce3e257492d3) )
	ROM_LOAD( "turbosub.u23", 0x24000, 0x4000, CRC(b1531916) SHA1(805a23f40aa875f431e835fdaceba87261c14155) )
	ROM_LOAD( "turbosub.u33", 0x34000, 0x4000, CRC(0d5130cb) SHA1(7e4e4e5ea50c581a60d15964571464029515c720) )

	ROM_LOAD( "turbosub.u2",  0x08000, 0x4000, CRC(a8b8c032) SHA1(20512a3a1f8b9c0361e6f5a7e9a50605be3ae650) )
	ROM_LOAD( "turbosub.u12", 0x18000, 0x4000, CRC(a2c4badf) SHA1(267af1be6261833211270af25045e306efffee80) )
	ROM_LOAD( "turbosub.u22", 0x28000, 0x4000, CRC(97b7cf0e) SHA1(888fb2f384a5cba8a6f7569886eb6dc27e2b024f) )
	ROM_LOAD( "turbosub.u32", 0x38000, 0x4000, CRC(b286710e) SHA1(5082db13630ba0967006619027c39ee3607b838d) )

	ROM_LOAD( "turbosub.u1",  0x0c000, 0x4000, CRC(88b0a7a9) SHA1(9012c8059cf60131efa6a0432accd87813187206) )
	ROM_LOAD( "turbosub.u11", 0x1c000, 0x4000, CRC(9f0ff723) SHA1(54b52b4ebc32f10aa32c799ac819928290e70455) )
	ROM_LOAD( "turbosub.u21", 0x2c000, 0x4000, CRC(b4122fe2) SHA1(50e8b488a7b7f739336b60a3fd8a5b14f5010b75) )
	ROM_LOAD( "turbosub.u31", 0x3c000, 0x4000, CRC(3fa15c78) SHA1(bf5cb85fc26b5045ad5acc944c917b068ace2c49) )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "turbosub.u9",  0x00000, 0x4000, CRC(9a03eadf) SHA1(25ee1ebe52f030b2fa09d76161e46540c91cbc4c) )
	ROM_LOAD( "turbosub.u19", 0x10000, 0x4000, CRC(498253b8) SHA1(dd74d4f9f19d8a746415baea604116faedb4fb31) )
	ROM_LOAD( "turbosub.u29", 0x20000, 0x4000, CRC(809c374f) SHA1(d3849eed8441e4641ffcbca7c83ee3bb16681a0b) )
	ROM_LOAD( "turbosub.u39", 0x30000, 0x4000, CRC(3e4e0681) SHA1(ac834f6823ffe835d6f149e79c1d31ae2b89e85d) )

	ROM_LOAD( "turbosub.u8",  0x04000, 0x4000, CRC(01118737) SHA1(3a8e998b80dffe82296170273dcbbe9870c5b695) )
	ROM_LOAD( "turbosub.u18", 0x14000, 0x4000, CRC(39fd8e57) SHA1(392f8a8cf58fc4813de840775d9c53561488152d) )
	ROM_LOAD( "turbosub.u28", 0x24000, 0x4000, CRC(0628586d) SHA1(e37508c2812e1c98659aaba9c495e7396842614e) )
	ROM_LOAD( "turbosub.u38", 0x34000, 0x4000, CRC(7d597a7e) SHA1(2f48faf75406ab3ff0b954040b74e68b7ca6f7a5) )

	ROM_LOAD( "turbosub.u7",  0x08000, 0x4000, CRC(50eea315) SHA1(567dbb3cb3a75a7507f4cb4748c7dd878e69d6b7) )
	ROM_LOAD( "turbosub.u17", 0x18000, 0x4000, CRC(8a9e19e6) SHA1(19067e153c0002edfd4a756f92ad75d9a0cbc3dd) )
	ROM_LOAD( "turbosub.u27", 0x28000, 0x4000, CRC(1c81a8d9) SHA1(3d13d1ccd7ec3dddf2a27600eb64b5be386e868c) )
	ROM_LOAD( "turbosub.u37", 0x38000, 0x4000, CRC(59f978cb) SHA1(e99d6378de941cad92e9702fcb18aea87acd371f) )

	ROM_LOAD( "turbosub.u6",  0x0c000, 0x4000, CRC(841e00bd) SHA1(f777cc8dd8dd7c8baa2007355a76db782a218efc) )
	ROM_LOAD( "turbosub.u16", 0x1c000, 0x4000, CRC(d3b63d81) SHA1(e86dd64825f6d9e7bebc26413f524a8962f68f2d) )
	ROM_LOAD( "turbosub.u26", 0x2c000, 0x4000, CRC(867cfe32) SHA1(549e4e557d63dfab8e8c463916512a1b422ce425) )
	ROM_LOAD( "turbosub.u36", 0x3c000, 0x4000, CRC(0d8ebc21) SHA1(7ae65edae05869376caa975ff2c778a08e8ad8a2) )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

ROM_START( turbosubc5 )
	ROM_REGION( 0xc0000, REGION_USER1, 0)
	ROM_LOAD( "tsc5.u85", 0x18000, 0x4000, CRC(68fac402) SHA1(43f865964cd5de5d6d743af3250686504e3df159) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	ROM_LOAD( "tsc5.u82", 0x10000, 0x2000, CRC(2e46e200) SHA1(b52ced4bafde6b95f4b0c03be4a73e2c5f54f2a3) )
	ROM_CONTINUE(         0x20000, 0x2000 )
	ROM_LOAD( "tsc5.u81", 0x12000, 0x2000, CRC(8e8f17cf) SHA1(9f503cc0228c5837d9175bab6a0345abf962b619) )
	ROM_CONTINUE(         0x22000, 0x2000 )
	ROM_LOAD( "tsc5.u87", 0x14000, 0x2000, CRC(dddb8d38) SHA1(f7ca1638da75ad568ec236c629e3f382d078c028) )
	ROM_CONTINUE(         0x24000, 0x2000 )
	ROM_LOAD( "tsc5.u86", 0x16000, 0x2000, CRC(46623f0a) SHA1(88ad587385b10b895b44767ef17e6d1c14064638) )
	ROM_CONTINUE(         0x26000, 0x2000 )

	ROM_LOAD( "tsc5.u80", 0x30000, 0x2000, CRC(585a908b) SHA1(471c04a665fd52f9c52bb5a2eec647b371e1ec6b) )
	ROM_CONTINUE(         0x40000, 0x2000 )
	ROM_LOAD( "tsc5.u79", 0x32000, 0x2000, CRC(23c5b31f) SHA1(79a0534cdf3f8bdf6028f66438f002faec2a6285) )
	ROM_CONTINUE(         0x42000, 0x2000 )
	ROM_LOAD( "tsc5.u84", 0x34000, 0x2000, CRC(28d30203) SHA1(0f906b3468143370f9fee52d43af726da323115d) )
	ROM_CONTINUE(         0x44000, 0x2000 )
	ROM_LOAD( "tsc5.u83", 0x36000, 0x2000, CRC(081a8236) SHA1(18937a5b90db21c9c48619ea18213541bfc3653e) )
	ROM_CONTINUE(         0x46000, 0x2000 )

	ROM_COPY( REGION_USER1, 0x18000 + 0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "turbosub.u63", 0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION64_BE( 0x1000, REGION_CPU3, 0 )
	ROMX_LOAD( "27s29.u29", 0x0, 0x200, CRC(d580672b) SHA1(b56295a5b780ab5e8ff6817ebb084a8dfad8c281), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u28", 0x1, 0x200, CRC(f7976b87) SHA1(c19a1d375c497f1671170c7833952979819c3812), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u27", 0x2, 0x200, CRC(03ebd3ea) SHA1(109f5369bd36bcf0da5928b96566655c6895c737), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u21", 0x3, 0x200, CRC(e232384b) SHA1(cfc3acc86add06b4cb6addb3455d71123fb359ce), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u20", 0x4, 0x200, CRC(0a8e44d8) SHA1(2df46316510b2dbfd4c9913a1460c00d5572d586), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u19", 0x5, 0x200, CRC(de17e5f0) SHA1(3e14768374e1bda25183aee86a82d220b7f58ff9), ROM_SKIP(7) )
	ROMX_LOAD( "27s29.u18", 0x6, 0x200, CRC(e33ed0a4) SHA1(41edbdc7c022971ce14bd2f419c92714b796fad7), ROM_SKIP(7) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "turbosub.u66",   0xe000, 0x2000, CRC(8db3bcdb) SHA1(e6ae324ba9dad4884e1cb3d67ce099a6f4739456) )

	ROM_REGION( 0xc000, REGION_SOUND1, 0)
	ROM_LOAD( "turbosub.u69", 0x0000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )
	ROM_LOAD( "turbosub.u68", 0x4000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "turbosub.u67", 0x8000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )

	ROM_REGION( 0x40000, REGION_GFX1, 0)
	ROM_LOAD( "turbosub.u44", 0x00000, 0x4000, CRC(eaa05860) SHA1(f649891dae9354b7f2e46e6a380b52a569229d64) )
	ROM_LOAD( "turbosub.u49", 0x10000, 0x4000, CRC(b4170ac2) SHA1(bdbfc43c891c8d525dcc46fb9d05602263ab69cd) )
	ROM_LOAD( "turbosub.u54", 0x20000, 0x4000, CRC(bebf98d8) SHA1(170502bb44fc6d6bf14d8dac4778b37888c14a7b) )
	ROM_LOAD( "turbosub.u59", 0x30000, 0x4000, CRC(9c1f4397) SHA1(94335f2db2650f8b7e24fc3f92a04b73325ab164) )

	ROM_LOAD( "turbosub.u43", 0x04000, 0x4000, CRC(5d76237c) SHA1(3d50347856039e43290497348447b1c4581f3a33) )
	ROM_LOAD( "turbosub.u48", 0x14000, 0x4000, CRC(cea4e036) SHA1(4afce4f2a09adf9c83ab7188c05cd7236dea16a3) )
	ROM_LOAD( "turbosub.u53", 0x24000, 0x4000, CRC(1352d58a) SHA1(76ae86c365dd4c9e1a6c5af91c01d31e7ee35f0f) )
	ROM_LOAD( "turbosub.u58", 0x34000, 0x4000, CRC(5024d83f) SHA1(a293d92a0ae01901b5618b0250d48e3ba631dfcb) )

	ROM_LOAD( "turbosub.u42", 0x08000, 0x4000, CRC(057a1c72) SHA1(5af89b128b7818550572d02e5ff724c415fa8b8b) )
	ROM_LOAD( "turbosub.u47", 0x18000, 0x4000, CRC(10def494) SHA1(a3ba691eb2b0d782162ffc6c081761965844a3a9) )
	ROM_LOAD( "turbosub.u52", 0x28000, 0x4000, CRC(070d07d6) SHA1(4c81310cd646641a380817fedffab66e76529c97) )
	ROM_LOAD( "turbosub.u57", 0x38000, 0x4000, CRC(5ddb0458) SHA1(d1169882397f364ca38fbd563250b33d13b1a7c6) )

	ROM_LOAD( "turbosub.u41", 0x0c000, 0x4000, CRC(014bb06b) SHA1(97276ba26b60c2907e59b92cc9de5251298579cf) )
	ROM_LOAD( "turbosub.u46", 0x1c000, 0x4000, CRC(3b866e2c) SHA1(c0dd4827a18eb9f4b1055d92544beed10f01fd86) )
	ROM_LOAD( "turbosub.u51", 0x2c000, 0x4000, CRC(43cdcb5c) SHA1(3dd966daa904d3be7be63c584ba033c0e7904d5c) )
	ROM_LOAD( "turbosub.u56", 0x3c000, 0x4000, CRC(6d116adf) SHA1(f808e28cef41dc86e43d8c12966037213da87c87) )

	ROM_REGION( 0x40000, REGION_GFX2, 0)
	ROM_LOAD( "turbosub.u4",  0x00000, 0x4000, CRC(08303604) SHA1(f075b645d89a2d91bd9b621748906a9f9890ee60) )
	ROM_LOAD( "turbosub.u14", 0x10000, 0x4000, CRC(83b26c8d) SHA1(2dfa3b45c44652d255c402511bb3810fffb0731d) )
	ROM_LOAD( "turbosub.u24", 0x20000, 0x4000, CRC(6bbb6cb3) SHA1(d513e547a05b34076bb8261abd51301ac5f3f5d4) )
	ROM_LOAD( "turbosub.u34", 0x30000, 0x4000, CRC(7b844f4a) SHA1(82467eb7e116f9f225711a1698c151945e1de6e4) )

	ROM_LOAD( "turbosub.u3",  0x04000, 0x4000, CRC(825ef29c) SHA1(affadd0976f793b8bdbcbc4768b7de27121e7b11) )
	ROM_LOAD( "turbosub.u13", 0x14000, 0x4000, CRC(350cc17a) SHA1(b98d16be997fc0576d3206f51f29ce3e257492d3) )
	ROM_LOAD( "turbosub.u23", 0x24000, 0x4000, CRC(b1531916) SHA1(805a23f40aa875f431e835fdaceba87261c14155) )
	ROM_LOAD( "turbosub.u33", 0x34000, 0x4000, CRC(0d5130cb) SHA1(7e4e4e5ea50c581a60d15964571464029515c720) )

	ROM_LOAD( "turbosub.u2",  0x08000, 0x4000, CRC(a8b8c032) SHA1(20512a3a1f8b9c0361e6f5a7e9a50605be3ae650) )
	ROM_LOAD( "turbosub.u12", 0x18000, 0x4000, CRC(a2c4badf) SHA1(267af1be6261833211270af25045e306efffee80) )
	ROM_LOAD( "turbosub.u22", 0x28000, 0x4000, CRC(97b7cf0e) SHA1(888fb2f384a5cba8a6f7569886eb6dc27e2b024f) )
	ROM_LOAD( "turbosub.u32", 0x38000, 0x4000, CRC(b286710e) SHA1(5082db13630ba0967006619027c39ee3607b838d) )

	ROM_LOAD( "turbosub.u1",  0x0c000, 0x4000, CRC(88b0a7a9) SHA1(9012c8059cf60131efa6a0432accd87813187206) )
	ROM_LOAD( "turbosub.u11", 0x1c000, 0x4000, CRC(9f0ff723) SHA1(54b52b4ebc32f10aa32c799ac819928290e70455) )
	ROM_LOAD( "turbosub.u21", 0x2c000, 0x4000, CRC(b4122fe2) SHA1(50e8b488a7b7f739336b60a3fd8a5b14f5010b75) )
	ROM_LOAD( "turbosub.u31", 0x3c000, 0x4000, CRC(3fa15c78) SHA1(bf5cb85fc26b5045ad5acc944c917b068ace2c49) )

	ROM_REGION( 0x40000, REGION_GFX3, 0)
	ROM_LOAD( "turbosub.u9",  0x00000, 0x4000, CRC(9a03eadf) SHA1(25ee1ebe52f030b2fa09d76161e46540c91cbc4c) )
	ROM_LOAD( "turbosub.u19", 0x10000, 0x4000, CRC(498253b8) SHA1(dd74d4f9f19d8a746415baea604116faedb4fb31) )
	ROM_LOAD( "turbosub.u29", 0x20000, 0x4000, CRC(809c374f) SHA1(d3849eed8441e4641ffcbca7c83ee3bb16681a0b) )
	ROM_LOAD( "turbosub.u39", 0x30000, 0x4000, CRC(3e4e0681) SHA1(ac834f6823ffe835d6f149e79c1d31ae2b89e85d) )

	ROM_LOAD( "turbosub.u8",  0x04000, 0x4000, CRC(01118737) SHA1(3a8e998b80dffe82296170273dcbbe9870c5b695) )
	ROM_LOAD( "turbosub.u18", 0x14000, 0x4000, CRC(39fd8e57) SHA1(392f8a8cf58fc4813de840775d9c53561488152d) )
	ROM_LOAD( "turbosub.u28", 0x24000, 0x4000, CRC(0628586d) SHA1(e37508c2812e1c98659aaba9c495e7396842614e) )
	ROM_LOAD( "turbosub.u38", 0x34000, 0x4000, CRC(7d597a7e) SHA1(2f48faf75406ab3ff0b954040b74e68b7ca6f7a5) )

	ROM_LOAD( "turbosub.u7",  0x08000, 0x4000, CRC(50eea315) SHA1(567dbb3cb3a75a7507f4cb4748c7dd878e69d6b7) )
	ROM_LOAD( "turbosub.u17", 0x18000, 0x4000, CRC(8a9e19e6) SHA1(19067e153c0002edfd4a756f92ad75d9a0cbc3dd) )
	ROM_LOAD( "turbosub.u27", 0x28000, 0x4000, CRC(1c81a8d9) SHA1(3d13d1ccd7ec3dddf2a27600eb64b5be386e868c) )
	ROM_LOAD( "turbosub.u37", 0x38000, 0x4000, CRC(59f978cb) SHA1(e99d6378de941cad92e9702fcb18aea87acd371f) )

	ROM_LOAD( "turbosub.u6",  0x0c000, 0x4000, CRC(841e00bd) SHA1(f777cc8dd8dd7c8baa2007355a76db782a218efc) )
	ROM_LOAD( "turbosub.u16", 0x1c000, 0x4000, CRC(d3b63d81) SHA1(e86dd64825f6d9e7bebc26413f524a8962f68f2d) )
	ROM_LOAD( "turbosub.u26", 0x2c000, 0x4000, CRC(867cfe32) SHA1(549e4e557d63dfab8e8c463916512a1b422ce425) )
	ROM_LOAD( "turbosub.u36", 0x3c000, 0x4000, CRC(0d8ebc21) SHA1(7ae65edae05869376caa975ff2c778a08e8ad8a2) )

	ROM_REGION( 0x260, REGION_PROMS, 0)
	ROM_LOAD( "27s29.u123",    0x0000, 0x0200, CRC(b2e8770e) SHA1(849292a6b30bb0e6547ce3232438136897a651b0) )
	ROM_LOAD( "6331_snd.u2",   0x0200, 0x0020, CRC(f1328a5e) SHA1(44d4e802988415d24a0b9eaa38300f5add3a2727) )
	ROM_LOAD( "6331_rom.u74",  0x0220, 0x0020, CRC(7b72b34e) SHA1(bc4d67a6993beb36a161368428e648d0492ac436) )
	ROM_LOAD( "6331_vid.u155", 0x0240, 0x0020, CRC(63371737) SHA1(f08c03c81322c0de9ee64b4a9f11a1422c5bd463) )
ROM_END

GAME( 1985, turbosub,    0,        esripsys, turbosub_boot, esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSCA)",  0 )
GAME( 1985, turbosubc5,  turbosub, esripsys, turbosub_boot, esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSC5)",  0 )
GAME( 1985, turbosubc5s, turbosub, esripsys, turbosub_boot, esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSC5*)", 0 )
GAME( 1985, turbosubc6,  turbosub, esripsys, turbosub,      esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSC6)",  0 )
GAME( 1985, turbosubc7,  turbosub, esripsys, turbosub,      esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSC7)",  0 )
GAME( 1985, turbosubc8,  turbosub, esripsys, turbosub,      esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSC8)",  0 )
GAME( 1985, turbosubb8,  turbosub, esripsys, turbosubb8,    esripsys, ROT0, "Entertainment Sciences", "Turbo Sub (prototype rev. TSB8)",  0 )
