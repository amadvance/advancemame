// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/* Pioneer PR-8210 with Simutrek parallel interface and two MCS-48 controllers. */
#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "machine/laserdsc.h"
#include "machine/ldpr8210.h"

static struct
{
	UINT8 frame[7], text[17], control, latchdisplay, portb, display, porta, vbi1, vbi2;
} pia;
static UINT8 p1, p2, sp2, command, last_command, control;
static int data_ready, vsync, field, control_this, control_next, external_audio, video_on;
static UINT16 accumulator;
static UINT32 vbi_code, vbi_line16;
static double last_bit, first_bit, last_command_time, slider_time;
static double scan_speed, scan_fraction;
static mame_timer *vsync_timer, *irq_timer;

static int focus_on(void) { return !(p1 & 8); }
static int spindle_on(void) { return !(p1 & 0x10); }
static int laser_on(void) { return !(p2 & 1); }

static void update_slider(void)
{
	double now = timer_get_time();
	scan_fraction += (now - slider_time) * scan_speed * (60000.0 / 1001);
	if (scan_fraction >= 1 || scan_fraction <= -1)
	{
		int delta = scan_fraction;
		laserdisc_raw_advance(delta);
		scan_fraction -= delta;
	}
	slider_time = now;
}

static void serial_w(UINT8 data)
{
	UINT8 prev = control;
	control = data;
	if (!prev && data)
	{
		double now = timer_get_time(), delta = now - last_bit;
		last_bit = now;
		if (now - first_bit > 11520.0 / 455000 || delta > 4096.0 / 455000)
		{
			first_bit = now;
			accumulator = 0x5555;
		}
		accumulator = (accumulator << 1) | (delta >= 600.0 / 455000);
		if ((accumulator & 0x383) == 0x80)
		{
			UINT8 value = (accumulator >> 2) & 0x1f;
			pia.porta = BITSWAP8(value, 0,1,2,3,4,5,6,7);
			if (pia.porta == last_command && now < last_command_time + 46080.0 / 455000)
				pia.porta = 0;
			else last_command = pia.porta;
			last_command_time = now;
			first_bit = now - 11520.0 / 455000;
		}
	}
}

static void pr_port1_w(UINT8 data)
{
	UINT8 prev = p1;
	int direction = (data & 0x80) ? 1 : -1;
	update_slider();
	p1 = data;
	if (!(data & 1) && (prev & 1) && !control_this) laserdisc_raw_advance(direction);
	scan_speed = (data & 2) ? 0 : ((data & 4) ? 2000 / 30 : 4000 / 30) * direction;
}

static void pr_port2_w(UINT8 data)
{
	p2 = data;
	cpunum_set_input_line(4, 0, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
}

static void sim_port2_w(UINT8 data)
{
	UINT8 prev = sp2;
	sp2 = data;
	if ((data ^ prev) & 0x20) serial_w((data & 0x20) != 0);
	if (!(data & 0x10) && (prev & 0x10)) laserdisc_raw_advance((data & 8) ? 1 : -1);
	control_next = (~data >> 2) & 1;
}

READ8_HANDLER( pr8210_io_r )
{
	switch (offset)
	{
	case I8039_t0: return !vsync;
	case I8039_t1: return 1;
	case I8039_bus:
		update_slider();
		return (!laserdisc_raw_at_limit() ? 0x40 : 0) | (!focus_on() ? 0x10 : 0) | (!spindle_on() ? 0x0a : 0);
	}
	if (offset >= 0x20 && offset <= 0x26) return pia.frame[offset - 0x20];
	if (offset == 0xa0) return pia.porta;
	if (offset == 0xc0) return pia.vbi1;
	if (offset == 0xe0) return pia.vbi2;
	return 0xff;
}

WRITE8_HANDLER( pr8210_io_w )
{
	if (offset == I8039_p1) pr_port1_w(data);
	else if (offset == I8039_p2) pr_port2_w(data);
	else if (offset >= 0x20 && offset <= 0x30) pia.text[offset - 0x20] = data;
	else if (offset == 0x40) pia.control = data;
	else if (offset == 0x60) pia.portb = data;
	else if (offset == 0x80) pia.display = data & 1;
}

READ8_HANDLER( simutrek_io_r )
{
	if (offset == I8039_p2) return (p1 & 0x20) ? 0 : 0x80;
	if (offset == I8039_t0) return data_ready;
	if (offset < 0x100) { data_ready = 0; return command; }
	return 0xff;
}
WRITE8_HANDLER( simutrek_io_w ) { if (offset == I8039_p2) sim_port2_w(data); }

static void vsync_off(int param) { vsync = 0; }
static void irq_off(int param) { cpunum_set_input_line(5, 0, CLEAR_LINE); }
static void latch_command(int param) { command = param; data_ready = 1; }

static void process_vbi(int param)
{
	UINT32 code = vbi_code, line16 = vbi_line16;
	pia.vbi1 = pia.vbi2 = 0xff;
	if (focus_on() && laser_on())
	{
		if (code == 0x88ffff) pia.vbi1 &= ~1;
		if (code == 0x80eeee) pia.vbi1 &= ~2;
		if (line16 == 0x82cfff) pia.vbi1 &= ~4;
		if ((code & 0xf00000) == 0xf00000)
		{
			int i;
			pia.vbi1 &= ~0x10;
			for (i = 0; i < 5; ++i) pia.frame[2+i] = 0xf0 | ((code >> (16-i*4)) & (i ? 15 : 7));
		}
		if ((code & 0xf00fff) == 0x800ddd)
		{
			pia.vbi2 &= ~1;
			pia.frame[0] = 0xf0 | ((code >> 16) & 7);
			pia.frame[1] = 0xf0 | ((code >> 12) & 15);
		}
	}
}

int simutrek_init(void)
{
	vsync_timer = timer_alloc(vsync_off);
	irq_timer = timer_alloc(irq_off);
	return ldv1000_init();
}
void simutrek_reset(void)
{
	memset(&pia, 0, sizeof(pia));
	p1 = p2 = sp2 = command = last_command = control = 0;
	data_ready = 1;
	vsync = field = control_this = control_next = 0;
	external_audio = video_on = 0;
	scan_speed = scan_fraction = 0;
	slider_time = last_bit = first_bit = last_command_time = timer_get_time();
	laserdisc_raw_reset();
}
void simutrek_write(UINT8 data) { timer_set(TIME_NOW, data, latch_command); }
UINT8 simutrek_status(void) { return (data_ready ? 1 : 0) | ((sp2 & 3) == 3 ? 2 : 0); }
void simutrek_audio(int enabled) { external_audio = enabled; }
void simutrek_video(int enabled) { video_on = enabled; }

void simutrek_field(void)
{
	update_slider();
	if (field == 1) { control_this = control_next; control_next = 0; }
	vsync = 1;
	timer_adjust(vsync_timer, TIME_IN_USEC(4 * 63.555), 0, 0);
	/* VBI must reach the firmware before it samples the picture-number registers. */
	laserdisc_raw_field(field, video_on && !(p1 & 0x20), external_audio);
	vbi_code = laserdisc_raw_code(0);
	vbi_line16 = laserdisc_raw_code(16);
	timer_set(TIME_IN_USEC(19 * 63.555), 0, process_vbi);
	if (data_ready)
	{
		cpunum_set_input_line(5, 0, ASSERT_LINE);
		timer_adjust(irq_timer, TIME_IN_USEC(63.555), 0, 0);
	}
	if (field && spindle_on()) laserdisc_raw_advance(1);
	field ^= 1;
}
