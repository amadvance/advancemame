// license:BSD-3-Clause
// copyright-holders:Mariusz Wojcieszek
/*
    Atari Tomcat prototype hardware

    No sound program was prepared for this prototype.
*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms32010/tms32010.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"
#include "machine/timekpr.h"

static UINT16 *shared_ram;
static UINT16 *vector_words;
static UINT8 nvram[0x800];
static int dsp_bio;
static int dsp_idle;
static UINT8 adc_channel;
static UINT8 adc_start;
static UINT8 adc_result;
static void *adc_timer;

static void adc_done(int channel)
{
	adc_result = adc_channel < 2 ? readinputport(adc_channel + 1) : 0xff;
}

static WRITE16_HANDLER( adc_w )
{
	if (ACCESSING_LSB)
	{
		adc_channel = data & 7;
		/* The CPU reads the SAR while conversion is running, before EOC. */
		if (!adc_start && (data & 8))
			timer_adjust(adc_timer, TIME_IN_HZ(12096000.0 / (16 * 2)), 0, 0);
		adc_start = data & 8;
	}
}

static READ16_HANDLER( adc_r )
{
	return ACCESSING_LSB ? adc_result : 0;
}

static READ16_HANDLER( inputs_r )
{
	int result = readinputport(0);
	if (avgdvg_done()) result |= 1;
	return ACCESSING_MSB ? result << 8 : 0;
}

static READ16_HANDLER( dsp_status_r )
{
	return dsp_idle ? 0 : 0x200;
}

static READ16_HANDLER( dsp_start_r )
{
	dsp_bio = 1;
	/* Keep the command stable until the DSP has consumed the BIO handshake. */
	cpunum_suspend(0, SUSPEND_REASON_SPIN, 1);
	return 0;
}

static READ16_HANDLER( dsp_bio_r )
{
	if (activecpu_get_pc() == 1)
	{
		if (!dsp_idle)
		{
			dsp_idle = 1;
			dsp_bio = 0;
		}
		return !dsp_bio;
	}
	if (activecpu_get_pc() == 3 && dsp_bio)
	{
		dsp_idle = 0;
		dsp_bio = 0;
		cpunum_resume(0, SUSPEND_REASON_SPIN);
		return 0;
	}
	return !dsp_bio;
}

static WRITE16_HANDLER( irq_ack_w )
{
	cpunum_set_input_line(0, 1, CLEAR_LINE);
}

static WRITE16_HANDLER( main_latch_w )
{
	int state = (offset >> 3) & 1;
	switch (offset & 7)
	{
		case 0: set_led_status(0, !state); break;
		case 1: set_led_status(1, !state); break;
		case 2:
			if (state) dsp_bio = 0;
			cpunum_set_input_line(1, INPUT_LINE_RESET, state ? CLEAR_LINE : ASSERT_LINE);
			break;
	}
}

static READ16_HANDLER( nvram_r )
{
	return timekeeper_0_msb16_r(offset, mem_mask) | (ACCESSING_LSB ? nvram[offset] : 0);
}

static WRITE16_HANDLER( nvram_w )
{
	timekeeper_0_msb16_w(offset, data, mem_mask);
	if (ACCESSING_LSB) nvram[offset] = data;
}

static NVRAM_HANDLER( tomcat )
{
	nvram_handler_timekeeper_0(file, read_or_write);
	if (read_or_write)
		mame_fwrite(file, nvram, sizeof(nvram));
	else if (file)
		mame_fread(file, nvram, sizeof(nvram));
	else
		memset(nvram, 0, sizeof(nvram));
}

static MACHINE_START( tomcat )
{
	/* The writable DSP program waits on BIO until the main CPU loads a command. */
	shared_ram[0] = shared_ram[2] = 0xf600;
	shared_ram[1] = shared_ram[3] = 0;
	adc_timer = timer_alloc(adc_done);
	state_save_register_global_array(nvram);
	state_save_register_global(dsp_bio);
	state_save_register_global(dsp_idle);
	state_save_register_global(adc_channel);
	state_save_register_global(adc_start);
	state_save_register_global(adc_result);
	return 0;
}

static MACHINE_RESET( tomcat )
{
	dsp_bio = dsp_idle = 0;
	adc_start = 0;
	adc_result = 0x80;
	cpunum_resume(0, SUSPEND_REASON_SPIN);
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00ffff) AM_ROM
	AM_RANGE(0x402000, 0x402001) AM_READWRITE(adc_r, adc_w)
	AM_RANGE(0x404000, 0x404001) AM_READWRITE(inputs_r, avgdvg_go_word_w)
	AM_RANGE(0x406000, 0x406001) AM_WRITE(avgdvg_reset_word_w)
	AM_RANGE(0x408000, 0x408001) AM_READ(dsp_status_r) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x40a000, 0x40a001) AM_READWRITE(dsp_start_r, irq_ack_w)
	AM_RANGE(0x40e000, 0x40e01f) AM_WRITE(main_latch_w)
	AM_RANGE(0x800000, 0x803fff) AM_RAM AM_BASE(&vector_words) AM_SIZE(&vectorram_size)
	AM_RANGE(0xffa000, 0xffbfff) AM_RAM AM_BASE(&shared_ram) AM_SHARE(1)
	AM_RANGE(0xffc000, 0xffcfff) AM_RAM
	AM_RANGE(0xffd000, 0xffdfff) AM_READWRITE(nvram_r, nvram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_SHARE(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp_io_map, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(TMS32010_BIO, TMS32010_BIO) AM_READ(dsp_bio_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( tomcat )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Diagnostic")
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Right Fire")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Left Fire")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Right Thumb")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Left Thumb")

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(70) PORT_KEYDELTA(30)
	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(30)
INPUT_PORTS_END

static MACHINE_DRIVER_START( tomcat )
	MDRV_CPU_ADD(M68010, 12096000/2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_PERIODIC_INT(irq1_line_assert, TIME_IN_HZ(5*60))
	MDRV_CPU_ADD(TMS32010, 16000000/TMS32010_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(dsp_map, 0)
	MDRV_CPU_IO_MAP(dsp_io_map, 0)
	MDRV_MACHINE_START(tomcat)
	MDRV_MACHINE_RESET(tomcat)
	MDRV_NVRAM_HANDLER(tomcat)
	MDRV_FRAMES_PER_SECOND(40)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 280, 0, 250)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_VIDEO_START(avg_tomcat)
	MDRV_VIDEO_UPDATE(vector)
MACHINE_DRIVER_END

ROM_START( tomcat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rom1k.bin", 0x00001, 0x8000, CRC(5535a1ff) SHA1(b9807c749a8e6b5ddec3ff494130abda09f0baab) )
	ROM_LOAD16_BYTE( "rom2k.bin", 0x00000, 0x8000, CRC(021a01d2) SHA1(01d99aab54ad57a664e8aaa91296bb879fc6e422) )
	ROM_REGION( 0x100, REGION_PROMS, 0 )
	ROM_LOAD( "136021-105.1l", 0x0000, 0x0100, CRC(82fc3eb2) SHA1(184231c7baef598294860a7d2b8a23798c5c7da6) )
ROM_END

static DRIVER_INIT( tomcat )
{
	vectorram = (UINT8 *)vector_words;
	timekeeper_init(0, TIMEKEEPER_M48T02, 0);
}

GAME( 1985, tomcat, 0, tomcat, tomcat, tomcat, ROT0, "Atari", "TomCat (prototype)", 0 )
