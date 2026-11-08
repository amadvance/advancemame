// license:BSD-3-Clause
// copyright-holders:Roberto Fresca, Grull Osgo, Curt Coder, AJR
/***************************************************************************

  Paracaidista
  Video Game / Electrogame, 1979.

  Driver by Roberto Fresca & Grull Osgo.
  Intel 8085, 8155 RAM/I/O/timer and 8257 DMA controller.

***************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/dac.h"

#define TIMER_CLOCK (6553600.0 / 16)

static UINT8 *paraca_vram;
static UINT8 port_command, port_output[3], timer_status;
static UINT16 count_length, count_loaded;
static int count_phase, timer_running, timer_output;
static void *count_timer, *pulse_timer;
static UINT16 dma_registers[8];
static UINT8 dma_mode, dma_high;

static void sound_update(void)
{
	static const UINT16 levels[4] = { 65535, 32768, 26214, 13107 };
	DAC_data_16_w(0, timer_output ? levels[(port_output[2] >> 1) & 3] : 0);
}
static void set_timer_output(int state)
{
	timer_output = state;
	sound_update();
}
static UINT16 timer_count(void)
{
	if (timer_running)
	{
		unsigned count = ((unsigned)(timer_timeleft(count_timer) * TIMER_CLOCK) + 1) * 2;
		if (count > (count_loaded & 0x3ffe))
			count = count_loaded & 0x3ffe;
		return count | (count_phase ? 0 : 1);
	}
	return count_length;
}
static void stop_count(void)
{
	if (timer_running)
		count_loaded = (count_loaded & 0xc000) | timer_count();
	timer_running = 0;
	timer_adjust(count_timer, TIME_NEVER, 0, 0);
	timer_adjust(pulse_timer, TIME_NEVER, 0, 0);
	set_timer_output(1);
}
static void reload_count(void)
{
	int count = count_length & 0x3fff;
	count_loaded = count_length;
	if (count < 2)
	{
		stop_count();
		return;
	}
	timer_running = 1;
	count_phase = 0;
	timer_adjust(count_timer, ((count + 1) / 2) / TIMER_CLOCK, 0, 0);
	set_timer_output(1);
}
static void pulse_tick(int param)
{
	set_timer_output(0);
}
static void count_tick(int param)
{
	if (!count_phase)
	{
		int half = (count_loaded & 0x3ffe) / 2;
		count_phase = 1;
		timer_adjust(count_timer, half / TIMER_CLOCK, 0, 0);
		if (!(count_loaded & 0x8000))
			set_timer_output(0);
		else
			timer_adjust(pulse_timer, (half - 1) / TIMER_CLOCK, 0, 0);
	}
	else
	{
		timer_status |= 0x40;
		set_timer_output(1);
		if (!(count_loaded & 0x4000) || (port_command & 0xc0) == 0x80)
			stop_count();
		else
			reload_count();
	}
}
static void output_port(int port)
{
	if (port == 1)
	{
		set_led_status(0, port_output[1] & 1);
		set_led_status(1, (port_output[1] >> 1) & 1);
	}
	else if (port == 2)
		sound_update();
}
static int port_is_output(int port)
{
	if (port < 2)
		return (port_command >> port) & 1;
	return (port_command & 0x0c) == 0x0c;
}
static READ8_HANDLER( ppi_r )
{
	UINT8 result;
	switch (offset & 7)
	{
		case 0:
			result = timer_status;
			timer_status &= ~0x40;
			return result;
		case 1:
			return port_is_output(0) ? port_output[0] : readinputport(2 + ((port_output[2] >> 3) & 1));
		case 2: return port_output[1];
		case 3: return port_output[2] | 0xc0;
		case 4: return timer_count();
		case 5: return (timer_count() >> 8 & 0x3f) | (count_loaded >> 8 & 0xc0);
	}
	return 0;
}
static WRITE8_HANDLER( ppi_w )
{
	int port;
	switch (offset & 7)
	{
		case 0:
		{
			UINT8 old = port_command;
			port_command = data;
			for (port = 0; port < 3; ++port)
				if (port_is_output(port) && (old != port_command))
					output_port(port);
			switch (data & 0xc0)
			{
				case 0x40: stop_count(); break;
				case 0xc0:
					/* A running counter accepts the new length at its next reload. */
					if (!timer_running)
						reload_count();
					break;
			}
			break;
		}
		case 1:
		case 2:
		case 3:
			port = (offset & 7) - 1;
			port_output[port] = port == 2 ? data & 0x3f : data;
			if (port_is_output(port))
				output_port(port);
			break;
		case 4: count_length = (count_length & 0xff00) | data; break;
		case 5: count_length = (count_length & 0x00ff) | (data << 8); break;
	}
}

static READ8_HANDLER( dma_r )
{
	unsigned reg = offset >> 2;
	UINT8 data = 0;
	/* With no request inputs connected, the controller never starts a transfer. */
	if (reg < 8)
	{
		data = dma_registers[reg] >> (dma_high * 8);
		if ((reg & 1) && dma_high)
			data &= 0x3f;
		dma_high ^= 1;
	}
	return data;
}
static WRITE8_HANDLER( dma_w )
{
	unsigned reg = offset >> 2;
	if (reg < 8)
	{
		unsigned mask = 0xff << (dma_high * 8);
		dma_registers[reg] = (dma_registers[reg] & ~mask) | (data << (dma_high * 8));
		if ((dma_mode & 0x80) && (reg == 4 || reg == 5))
			dma_registers[reg + 2] = dma_registers[reg];
		dma_high ^= 1;
	}
	else if (reg == 8)
		dma_mode = data;
}
static READ8_HANDLER( muxed_input_r )
{
	return readinputport((port_output[2] >> 5) & 1);
}
static INTERRUPT_GEN( paraca_interrupt )
{
	cpunum_set_input_line(0, I8085_RST65_LINE, HOLD_LINE);
}
static PALETTE_INIT( paraca )
{
	palette_set_color(0, 0, 0, 0);
	palette_set_color(1, 255, 255, 255);
}
static VIDEO_UPDATE( paraca )
{
	int x, y;
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			plot_pixel(bitmap, x, y, Machine->pens[(paraca_vram[y * 32 + x / 8] >> (7 - (x & 7))) & 1]);
}
static MACHINE_START( paraca )
{
	count_timer = timer_alloc(count_tick);
	pulse_timer = timer_alloc(pulse_tick);
	return 0;
}
static MACHINE_RESET( paraca )
{
	port_command = timer_status = count_phase = timer_running = 0;
	count_length = count_loaded = 0;
	memset(port_output, 0, sizeof(port_output));
	memset(dma_registers, 0, sizeof(dma_registers));
	dma_mode = dma_high = 0;
	stop_count();
}
static ADDRESS_MAP_START( paraca_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(15) )
	AM_RANGE(0x0000, 0x23ff) AM_ROM
	AM_RANGE(0x3400, 0x342f) AM_READWRITE(dma_r, dma_w)
	AM_RANGE(0x3800, 0x3800) AM_READ(input_port_4_r)
	AM_RANGE(0x3c00, 0x3c00) AM_READ(muxed_input_r)
	AM_RANGE(0x3e00, 0x3eff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x4000, 0x5fff) AM_RAM AM_BASE(&paraca_vram)
	AM_RANGE(0x6000, 0x60ff) AM_RAM
	AM_RANGE(0x7001, 0x7001) AM_READWRITE(MRA8_NOP, MWA8_NOP)
ADDRESS_MAP_END
static ADDRESS_MAP_START( paraca_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x60, 0x67) AM_READWRITE(ppi_r, ppi_w)
ADDRESS_MAP_END

static INPUT_PORTS_START(paracaidista)
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2)  PORT_PLAYER(2) PORT_CODE(KEYCODE_S) PORT_NAME("P2 Long Fire")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2)  PORT_PLAYER(1) PORT_CODE(KEYCODE_X) PORT_NAME("P1 Long Fire")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_G) PORT_NAME("IN1-10")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_H) PORT_NAME("IN1-20")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_J) PORT_NAME("IN1-40")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_K) PORT_NAME("IN1-80")

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1)  PORT_PLAYER(2) PORT_CODE(KEYCODE_A) PORT_NAME("P2 Short Fire")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1)  PORT_PLAYER(1) PORT_CODE(KEYCODE_Z) PORT_NAME("P1 Short Fire")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_T) PORT_NAME("IN0-10")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_Y) PORT_NAME("IN0-20")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_U) PORT_NAME("IN0-40")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_CODE(KEYCODE_I) PORT_NAME("IN0-80")

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_PLAYER(1) PORT_NAME("P1 Spinner")

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00, 0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(5) PORT_CENTERDELTA(0) PORT_PLAYER(2) PORT_NAME("P2 Spinner")


	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "03" )
	PORT_DIPSETTING(    0x01, "04" )
	PORT_DIPSETTING(    0x02, "05" )
	PORT_DIPSETTING(    0x03, "06" )


	PORT_DIPNAME( 0x0c, 0x0c, "Coin A" )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )

	PORT_DIPNAME( 0xa0, 0xa0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )


	PORT_DIPNAME( 0x10, 0x10, "DSW #7" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW #8" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( paraca )
	MDRV_CPU_ADD(8085A, 6553600/2)
	MDRV_CPU_PROGRAM_MAP(paraca_map, 0)
	MDRV_CPU_IO_MAP(paraca_io, 0)
	MDRV_CPU_VBLANK_INT(paraca_interrupt, 1)
	MDRV_MACHINE_START(paraca)
	MDRV_MACHINE_RESET(paraca)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 207)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(paraca)
	MDRV_VIDEO_UPDATE(paraca)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( paraca )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1.a3", 0x0000, 0x0400, CRC(7068953a) SHA1(ff91d719ca2e05bf71d555c1f4da37debe35e679) )
	ROM_LOAD( "rom2.a2", 0x0400, 0x0400, CRC(99a58854) SHA1(ab223873cf451a4a42a747c801705b4084c09abd) )
	ROM_LOAD( "rom3.a1", 0x0800, 0x0400, CRC(a6e9939d) SHA1(8067db64f1479912bbffb56131a253c16b82d874) )
	ROM_LOAD( "rom4.b3", 0x0C00, 0x0400, CRC(f30df1bd) SHA1(24030a34f8557969f99c25a7876b49a176bf0bf3) )
	ROM_LOAD( "rom5.b2", 0x1000, 0x0400, CRC(298c863b) SHA1(7a079ae63e308641bf971004060dcd33e54e75cb) )
	ROM_LOAD( "rom6.b1", 0x1400, 0x0400, CRC(b314be9d) SHA1(264af4312ae4262a0105e556430070e299750712) )
	ROM_LOAD( "rom7.c3", 0x1800, 0x0400, CRC(f2ae0152) SHA1(719242732811b9b183469c619195b147828fa539) )
	ROM_LOAD( "rom8.c2", 0x1c00, 0x0400, CRC(a311057c) SHA1(5c2d43b3982d54653bb9170dd148d0eaf967bae6) )
	ROM_LOAD( "rom9.c1", 0x2000, 0x0400, CRC(64f44bb3) SHA1(11b70a1fd6457d0d04eb647bcedcf63d14212cde) )
ROM_END

GAME( 1979, paraca, 0, paraca, paracaidista, 0, ROT0, "Video Game / Electrogame", "Paracaidista", 0 )
