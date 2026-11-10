// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Ryan Holtz
/* Sega Time Traveler with a serial Pioneer LD-V4200 player. */
#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/laserdsc.h"
#include "sound/custom.h"
#include "sound/dac.h"

#define CPU_CLOCK 8000000
#define TIMER_CLOCK (CPU_CLOCK / 4)
#define UART_CLOCK 768000
#define LD_COMMAND(a,b) ((((a) & 255) << 8) | ((b) & 255))

enum { PARK, PAUSE, STILL, PLAY, FORWARD, REVERSE, SEARCH };

static UINT8 uart[8], divisor[2], rx_data, rx_ready, tx_busy;
static char command[64], reply[32];
static int command_length, reply_pos;
static mame_timer *tx_timer, *rx_timer;
static int mode, picture, field, target, stop_picture, seek_fields;
static int video_enabled, audio_mask, speed, speed_fraction, reply_on_stop;
static UINT8 *eeprom;
static UINT8 panel[16];
static UINT8 peripheral[256];
static UINT16 timer_control[2], timer_count[2];
static int timer_pending, timer_in_service;
static mame_timer *board_timer[2];

static double serial_byte_time(void)
{
	int divisor_value = divisor[0] | (divisor[1] << 8);
	return TIME_IN_HZ((double)UART_CLOCK / (160 * (divisor_value ? divisor_value : 10)));
}

static void receive_byte(int param)
{
	if (!reply[reply_pos])
		return;
	if (rx_ready)
	{
		timer_adjust(rx_timer, serial_byte_time(), 0, 0);
		return;
	}
	rx_data = reply[reply_pos++];
	rx_ready = 1;
	timer_adjust(rx_timer, serial_byte_time(), 0, 0);
}

static void send_reply(const char *text)
{
	strcpy(reply, text);
	reply_pos = 0;
	timer_adjust(rx_timer, serial_byte_time(), 0, 0);
}

static void move_to(int track)
{
	laserdisc_raw_advance(track - laserdisc_raw_track());
	picture = laserdisc_raw_track() + 1;
}

static int execute_command(int code, int value)
{
	switch (code)
	{
	case LD_COMMAND('S','A'):
	case LD_COMMAND('P','A'):
		mode = PAUSE;
		break;
	case LD_COMMAND('S','T'):
		mode = STILL;
		break;
	case LD_COMMAND('S','E'):
	{
		int distance;
		if (value < 0) return 0;
		/* The CAV picture following the requested address is the seek target.
		   Allow the game to poll for completion while the player settles. */
		target = value;
		seek_fields = 4;
		for (distance = abs(target - laserdisc_raw_track()); distance > 2; distance >>= 1)
			++seek_fields;
		mode = SEARCH;
		return 2;
	}
	case LD_COMMAND('P','L'):
	case LD_COMMAND('M','F'):
	case LD_COMMAND('M','R'):
		mode = code == LD_COMMAND('P','L') ? PLAY :
			code == LD_COMMAND('M','F') ? FORWARD : REVERSE;
		speed_fraction = 0;
		if (value >= 0)
		{
			stop_picture = value + 1;
			reply_on_stop = 1;
			return 2;
		}
		break;
	case LD_COMMAND('S','F'):
	case LD_COMMAND('S','R'):
		move_to(laserdisc_raw_track() + (code == LD_COMMAND('S','F') ? 1 : -1));
		mode = STILL;
		stop_picture = -1;
		break;
	case LD_COMMAND('S','P'):
		speed = value < 1 ? 1 : value > 255 ? 255 : value;
		break;
	case LD_COMMAND('S','M'):
		stop_picture = value + 1;
		break;
	case LD_COMMAND('A','D'):
		audio_mask = value & 3;
		break;
	case LD_COMMAND('V','D'):
		video_enabled = value == 1;
		break;
	case LD_COMMAND('F','R'):
	case LD_COMMAND('D','S'):
	case LD_COMMAND('K','L'):
	case LD_COMMAND('C','M'):
	case LD_COMMAND('R','C'):
	case LD_COMMAND('C','L'):
		break;
	default:
		logerror("LD-V4200: unsupported command %c%c\n", code >> 8, code & 255);
		return 0;
	}
	return 1;
}

static void transmit_byte(int data)
{
	int i, result = 1;

	tx_busy = 0;
	if (data == ' ' || data == '\n')
		return;
	if (data != '\r')
	{
		if (command_length < sizeof(command))
			command[command_length++] = data >= 'a' && data <= 'z' ? data - 'a' + 'A' : data;
		return;
	}
	if (!command_length)
		return;

	logerror("LD-V4200 command: %.*s\n", command_length, command);
	for (i = 0; i < command_length && result; )
	{
		int value = -1;
		int status;
		if (command[i] >= '0' && command[i] <= '9')
		{
			value = 0;
			while (i < command_length && command[i] >= '0' && command[i] <= '9' && value < 1000000)
				value = value * 10 + command[i++] - '0';
		}
		if (i + 1 >= command_length)
		{
			result = 0;
			break;
		}
		status = execute_command(LD_COMMAND(command[i], command[i + 1]), value);
		if (status != 1) result = status;
		i += 2;
	}
	command_length = 0;
	if (result != 2)
		send_reply(result ? "R\r" : "E04\r");
}

static READ8_HANDLER( uart_r )
{
	if ((uart[3] & 0x80) && offset < 2)
		return divisor[offset];

	switch (offset)
	{
	case 0: rx_ready = 0; return rx_data;
	case 2: return 1;
	case 5: return (tx_busy ? 0 : 0x60) | rx_ready;
	case 6: return 0xb0;
	default: return uart[offset];
	}
}

static WRITE8_HANDLER( uart_w )
{
	if ((uart[3] & 0x80) && offset < 2)
		divisor[offset] = data;
	else if (offset == 0)
	{
		tx_busy = 1;
		timer_adjust(tx_timer, serial_byte_time(), data, 0);
	}
	else
		uart[offset] = data;
}

static int playing(void) { return mode == PLAY || mode == FORWARD || mode == REVERSE; }

static INTERRUPT_GEN( timetrv_field )
{
	int delta = 0;
	UINT32 code;

	if (mode == SEARCH && !--seek_fields)
	{
		move_to(target);
		mode = STILL;
		send_reply("R\r");
	}

	laserdisc_raw_field_stereo(field, video_enabled && (mode == STILL || playing()),
		playing() ? audio_mask : 0);
	code = laserdisc_raw_code(0);
	if ((code & 0xf80000) == 0xf80000)
	{
		int shift;
		picture = 0;
		for (shift = 16; shift >= 0; shift -= 4)
			picture = picture * 10 + ((code >> shift) & (shift == 16 ? 7 : 15));
	}

	field ^= 1;
	if (field || !playing())
		return;

	if (mode == PLAY)
		delta = 1;
	else
	{
		speed_fraction += speed;
		delta = speed_fraction / 60;
		speed_fraction %= 60;
		if (mode == REVERSE) delta = -delta;
	}

	/* Stop at the marked picture even when multispeed playback skips tracks. */
	if (stop_picture >= 0 && ((mode != REVERSE && picture + delta >= stop_picture) ||
		(mode == REVERSE && picture + delta <= stop_picture)))
	{
		move_to(stop_picture - 1);
		stop_picture = -1;
		mode = STILL;
		if (reply_on_stop) send_reply("R\r");
		reply_on_stop = 0;
	}
	else
		move_to(laserdisc_raw_track() + delta);
}

/* The legacy 80188 core supplies the instruction set, but omits its timers.
   This board relocates the peripheral block to I/O 0x0100 during startup. */
static UINT16 peripheral_word(int offset)
{
	return peripheral[offset] | (peripheral[offset + 1] << 8);
}

static void timer_irq(void)
{
	int enabled = !(peripheral[0x32] & 8) && (peripheral[0x32] & 7) <= (peripheral[0x2a] & 7);
	cpunum_set_input_line(0, 0, timer_pending && enabled && !timer_in_service ? ASSERT_LINE : CLEAR_LINE);
}

static int timer_ack(int line)
{
	int which = (timer_pending & 1) ? 0 : 1;

	timer_pending &= ~(1 << which);
	timer_in_service = 1;
	timer_irq();
	return which ? 0x12 : 8;
}

static void timer_schedule(int which)
{
	UINT16 control = timer_control[which];
	int limit = peripheral_word((control & 0x1000 ? 0x54 : 0x52) + which * 8);

	if (control & 0x8000)
	{
		if (!limit) limit = 0x10000;
		timer_adjust(board_timer[which], TIME_IN_SEC((double)(limit > timer_count[which] ? limit - timer_count[which] : 1) / TIMER_CLOCK), which, 0);
	}
	else
		timer_adjust(board_timer[which], TIME_NEVER, which, 0);
}

static void timer_elapsed(int which)
{
	UINT16 control = timer_control[which] | 0x20;

	timer_count[which] = 0;
	if (control & 0x2000)
	{
		timer_pending |= 1 << which;
		timer_irq();
	}
	if (!which)
		DAC_data_w(0, !(control & 2) || (control & 0x1000) ? 255 : 0);

	if ((control & 2) && !(control & 0x1000))
		control |= 0x1000;
	else
	{
		control &= ~0x1000;
		if (!(control & 1)) control &= ~0x8000;
	}
	timer_control[which] = control;
	timer_schedule(which);
}

static READ8_HANDLER( peripheral_r )
{
	int reg = offset & ~1;
	int which = (reg & 8) != 0;
	UINT16 value;

	switch (reg)
	{
	case 0x2c: value = timer_in_service; break;
	case 0x2e: value = timer_pending != 0; break;
	case 0x30: value = timer_pending; break;
	case 0x50:
	case 0x58:
		value = timer_count[which];
		if (timer_control[which] & 0x8000)
			value += timer_timeelapsed(board_timer[which]) * TIMER_CLOCK;
		break;
	case 0x56:
	case 0x5e: value = timer_control[which]; break;
	default: return peripheral[offset];
	}
	return value >> (8 * (offset & 1));
}

static WRITE8_HANDLER( peripheral_w )
{
	int reg = offset & ~1;
	int which = (reg & 8) != 0;
	UINT16 value;
	peripheral[offset] = data;
	if (!(offset & 1))
		return;

	value = peripheral_word(reg);
	switch (reg)
	{
	case 0x22: timer_in_service = 0; timer_irq(); break;
	case 0x2a:
	case 0x32: timer_irq(); break;
	case 0x2c: timer_in_service = value & 1; timer_irq(); break;
	case 0x30: timer_pending = value & 3; timer_irq(); break;
	case 0x50:
	case 0x58: timer_count[which] = value; timer_schedule(which); break;
	case 0x52:
	case 0x54:
	case 0x5a:
	case 0x5c: timer_schedule(which); break;
	case 0x56:
	case 0x5e:
		if (timer_control[which] & 0x8000)
			timer_count[which] += timer_timeelapsed(board_timer[which]) * TIMER_CLOCK;
		if (!(value & 0x4000))
			value = (value & ~0x8000) | (timer_control[which] & 0x8000);
		timer_control[which] = (value & 0xa02f) | ((value & 2) ? timer_control[which] & 0x1000 : 0);
		if (!which && !(timer_control[which] & 0x8000)) DAC_data_w(0, 0);
		timer_schedule(which);
		break;
	}
}

static WRITE8_HANDLER( lamps_w )
{
	set_led_status(0, data & 0x10);
	set_led_status(1, data & 8);
	set_led_status(2, data & 0x20);
}

static const ppi8255_interface ppi =
{
	2,
	{ input_port_0_r, input_port_2_r },
	{ input_port_1_r, input_port_3_r },
	{ 0, input_port_4_r },
	{ 0 }, { 0 }, { lamps_w }
};

static READ8_HANDLER( panel_r ) { return panel[offset]; }
static WRITE8_HANDLER( panel_w ) { panel[offset] = data; }
static READ8_HANDLER( panel_high_r ) { return panel[offset + 8]; }
static WRITE8_HANDLER( panel_high_w ) { panel[offset + 8] = data; }

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x10000, 0x107ff) AM_RAM AM_BASE(&eeprom)
	AM_RANGE(0xc0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0100, 0x01ff) AM_READWRITE(peripheral_r, peripheral_w)
	AM_RANGE(0x1000, 0x1003) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x1080, 0x1083) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0x1100, 0x1107) AM_READWRITE(uart_r, uart_w)
	AM_RANGE(0x1180, 0x1187) AM_READWRITE(panel_r, panel_w)
	AM_RANGE(0x1200, 0x1207) AM_READWRITE(panel_high_r, panel_high_w)
	AM_RANGE(0xff00, 0xffff) AM_RAM
ADDRESS_MAP_END

static NVRAM_HANDLER( timetrv )
{
	if (read_or_write)
		mame_fwrite(file, eeprom, 0x800);
	else
	{
		memset(eeprom, 0xff, 0x800);
		if (file) mame_fread(file, eeprom, 0x800);
	}
}

static MACHINE_START( timetrv )
{
	board_timer[0] = timer_alloc(timer_elapsed);
	board_timer[1] = timer_alloc(timer_elapsed);
	cpunum_set_irq_callback(0, timer_ack);
	tx_timer = timer_alloc(transmit_byte);
	rx_timer = timer_alloc(receive_byte);
	return ldv1000_init();
}

static MACHINE_RESET( timetrv )
{
	ppi8255_init(&ppi);
	memset(panel, 0x20, sizeof(panel));
	memset(uart, 0, sizeof(uart));
	memset(divisor, 0, sizeof(divisor));
	timer_adjust(tx_timer, TIME_NEVER, 0, 0);
	timer_adjust(rx_timer, TIME_NEVER, 0, 0);
	command_length = reply_pos = rx_data = rx_ready = tx_busy = 0;
	reply[0] = 0;
	picture = field = target = seek_fields = audio_mask = speed_fraction = reply_on_stop = 0;
	mode = PARK;
	stop_picture = -1;
	video_enabled = 1;
	speed = 60;
	memset(peripheral, 0, sizeof(peripheral));
	peripheral[0x2a] = 7;
	peripheral[0x32] = 15;
	memset(timer_control, 0, sizeof(timer_control));
	memset(timer_count, 0, sizeof(timer_count));
	timer_pending = timer_in_service = 0;
	timer_schedule(0);
	timer_schedule(1);
	DAC_data_w(0, 0);
	laserdisc_raw_reset();
	move_to(0);
}

static VIDEO_UPDATE( timetrv )
{
	ldv1000_video(bitmap, cliprect);
}

INPUT_PORTS_START( timetrv )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BILL1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Attack")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Reversal Cube")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ) )  PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )  PORT_DIPLOCATION("SW1:3,4")
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_BIT( 0xf0, 0x00, IPT_UNUSED )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, "Max Reversal Cubes" ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x01, "36" )
	PORT_DIPNAME( 0x0e, 0x04, "Reversal Cube Cost" ) PORT_DIPLOCATION("SW2:2,3,4")
	PORT_DIPSETTING(    0x00, "1 Coin / 1 Cube" )
	PORT_DIPSETTING(    0x02, "2 Coins / 1 Cube" )
	PORT_DIPSETTING(    0x04, "1 Coins / 2 Cubes" )
	PORT_DIPSETTING(    0x0a, "2 Coins / 3 Cubes" )
	PORT_DIPSETTING(    0x08, "1 Coin / 3 Cubes" )
	PORT_DIPSETTING(    0x0e, "1/2 Coins / 1/3 Cubes" )
	PORT_DIPSETTING(    0x0c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Unknown ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Level_Select ) ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "Devil Behavior" ) PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x00, "Devil Can Take Lives" )
	PORT_DIPSETTING(    0x20, "Devil Never Takes Lives" )
	PORT_DIPSETTING(    0x40, "Devil Not In Game" )
	PORT_DIPSETTING(    0x60, DEF_STR( Unknown ) )
	PORT_BIT( 0x80, 0x80, IPT_UNUSED )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) ) PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW3:2")
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW3:4,3")
	PORT_DIPSETTING(    0x0c, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xe0, 0x60, "Bill Multiplier" ) PORT_DIPLOCATION("SW3:6,7,8")
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x60, "4" )
	PORT_DIPSETTING(    0xe0, "8" )
	PORT_DIPSETTING(    0x40, "10" )
	PORT_DIPSETTING(    0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Unknown ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( timetrv )
	MDRV_CPU_ADD(I188, CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io, 0)
	MDRV_CPU_VBLANK_INT(timetrv_field, 1)
	MDRV_MACHINE_START(timetrv)
	MDRV_MACHINE_RESET(timetrv)
	MDRV_NVRAM_HANDLER(timetrv)
	MDRV_FRAMES_PER_SECOND(60000.0 / 1001)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(720, 524)
	MDRV_VISIBLE_AREA(0, 703, 44, 523)
	MDRV_VIDEO_UPDATE(timetrv)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(ldv1000_sound)
	MDRV_SOUND_ROUTE(0, "mono", 0.4)
	MDRV_SOUND_ROUTE(1, "mono", 0.4)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( timetrv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "tt061891.bin",   0xc0000, 0x40000, CRC(a3d44219) SHA1(7c5003b6d3df1e472db45abd725e7d3d43f0dfb4) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "timetrv", 0, SHA1(8abb5e6aa58ab49477ef89f507264d35454f99d3) )
ROM_END

GAME( 1991, timetrv, 0, timetrv, timetrv, 0, ORIENTATION_FLIP_Y, "Virtual Image Productions (Sega license)", "Time Traveler", 0 )
