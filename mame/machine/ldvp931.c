// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/* Philips 22VP931 parallel interface and DATIC video timing. */
#include "driver.h"
#include "hash.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m6809/m6809.h"
#include "machine/laserdsc.h"
#include "machine/ldvp931.h"

static UINT8 input_data, output_data, datic, port1, av_enable;
static int input_pending, output_pending, erp, strobe, field, audio_mask;
static int direction, half_track, scan_speed;
static double scan_time, scan_fraction;
static UINT32 codes[3];
static mame_timer *tracking_timer, *vbi_timer, *irq_timer, *strobe_timer, *erp_timer;

static void scan_update(void)
{
	double now = timer_get_time();
	int delta;

	scan_fraction += (now - scan_time) * scan_speed;
	scan_time = now;
	delta = scan_fraction;
	if (delta)
	{
		laserdisc_raw_advance(delta);
		scan_fraction -= delta;
	}
}

static void track_tick(int param)
{
	half_track ^= 1;
	if (half_track == (direction > 0))
		laserdisc_raw_advance(direction);
}

static void irq_off(int param) { cpunum_set_input_line(2, 0, CLEAR_LINE); }
static void strobe_off(int param) { strobe = 0; }
static void erp_off(int param) { erp = 0; }

static void vbi_tick(int param)
{
	int line = param >> 2;
	int phase = param & 3;
	UINT32 code = line < 3 ? codes[line] : 0;

	if (!phase)
	{
		cpunum_set_input_line(2, 0, ASSERT_LINE);
		timer_adjust(irq_timer, TIME_IN_USEC(5.58), 0, 0);
	}
	else if (code)
	{
		datic = code >> (8 * (3 - phase));
		strobe = 1;
		timer_adjust(strobe_timer, TIME_IN_USEC(5), 0, 0);
	}

	if (param < 12)
		timer_adjust(vbi_timer, TIME_IN_USEC(63.555 / 4), param + 1, 0);
}

static void command_latch(int data)
{
	input_data = data;
	input_pending = 1;
}

void vp931_write(UINT8 data)
{
	timer_set(TIME_NOW, data, command_latch);
}

UINT8 vp931_read(void)
{
	output_pending = 0;
	return output_data;
}

int vp931_ready(void) { return !input_pending; }
int vp931_pending(void) { return output_pending; }
void vp931_audio(int mask) { audio_mask = mask; }

READ8_HANDLER( vp931_io_r )
{
	switch (offset)
	{
	case I8039_p1: return erp ? 0 : 0x40;
	case I8039_p2: return (!output_pending << 5) | (!input_pending << 4);
	case I8039_t0: return strobe;
	case I8039_t1: return half_track;
	}

	if (offset < 0x100)
	{
		switch (offset & 0x30)
		{
		case 0x00: return 0;
		case 0x10: return 0;
		case 0x20: return datic;
		case 0x30: input_pending = 0; return input_data;
		}
	}
	return 0xff;
}

WRITE8_HANDLER( vp931_io_w )
{
	if (offset == I8039_p1)
	{
		if (data & 1)
			direction = 0;
		else if (!direction)
		{
			if (!(data & 4)) direction = -1;
			else if (!(data & 2)) direction = 1;
			half_track = direction < 0;
		}
		if (!direction)
			timer_adjust(tracking_timer, TIME_NEVER, 0, 0);
		else if ((port1 ^ data) & 0x11)
			timer_adjust(tracking_timer, TIME_IN_USEC(100), 0, TIME_IN_USEC((data & 0x10) ? 60 : 10));
		port1 = data;
	}
	else if (offset < 0x100)
	{
		switch (offset & 0x30)
		{
		case 0x00: av_enable = data; break;
		case 0x10:
			scan_update();
			scan_speed = (data & 2) ? 0 : ((data & 4) ? 4000 : 2000);
			if (data & 1) scan_speed = -scan_speed;
			break;
		case 0x30:
			output_pending = 1;
			output_data = data;
			cpunum_set_input_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
			break;
		}
	}
}

int vp931_init(void)
{
	char actual[HASH_BUF_SIZE];
	UINT8 *rom = memory_region(REGION_CPU3);
	mame_file *file = mame_fopen("22vp931", "at-6-1_a.bin", FILETYPE_ROM, 0);

	if (!file)
		fatalerror("Missing 22VP931 player firmware");
	if (mame_fsize(file) != 0x800 || mame_fread(file, rom, 0x800) != 0x800)
		fatalerror("Invalid 22VP931 player firmware length");
	mame_fclose(file);
	hash_compute(actual, rom, 0x800, HASH_CRC | HASH_SHA1);
	if (!hash_data_is_equal(actual, CRC(e11b3c8d) SHA1(ea2d7f6a044ed085ce5e09d8b1b1a21c37f0e9b8), 0))
		fatalerror("Incorrect 22VP931 player firmware");

	tracking_timer = timer_alloc(track_tick);
	vbi_timer = timer_alloc(vbi_tick);
	irq_timer = timer_alloc(irq_off);
	strobe_timer = timer_alloc(strobe_off);
	erp_timer = timer_alloc(erp_off);
	return ldv1000_init();
}

void vp931_reset(void)
{
	input_data = output_data = datic = port1 = av_enable = 0;
	input_pending = output_pending = erp = strobe = field = 0;
	direction = half_track = scan_speed = 0;
	scan_time = timer_get_time();
	scan_fraction = 0;
	timer_adjust(tracking_timer, TIME_NEVER, 0, 0);
	timer_adjust(vbi_timer, TIME_NEVER, 0, 0);
	timer_adjust(irq_timer, TIME_NEVER, 0, 0);
	timer_adjust(strobe_timer, TIME_NEVER, 0, 0);
	timer_adjust(erp_timer, TIME_NEVER, 0, 0);
	cpunum_set_input_line(2, 0, CLEAR_LINE);
	laserdisc_raw_reset();
}

void vp931_field(void)
{
	int i;

	scan_update();
	laserdisc_raw_field_stereo(field, av_enable & 1, (av_enable >> 1) & audio_mask);
	/* Save the current field's codes before moving to the next track. */
	for (i = 0; i < 3; ++i)
		codes[i] = laserdisc_raw_code(16 + i);
	if (field)
		laserdisc_raw_advance(1);
	field ^= 1;
	erp = 1;
	timer_adjust(erp_timer, TIME_IN_USEC(15 * 63.555), 0, 0);
	timer_adjust(vbi_timer, TIME_IN_USEC(16 * 63.555), 0, 0);
}
