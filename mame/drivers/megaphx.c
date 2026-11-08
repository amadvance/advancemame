// license:BSD-3-Clause
// copyright-holders:David Haywood, Dirk Best
/*
Dinamic / Inder arcade hardware
- Mega Phoenix
- Hammer Boy

68000 main CPU, TMS34010 video, PIC16C54 inputs,
Z80 / CTC sound board with four DAC channels.
*/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "cpu/pic16c5x/pic16c5x.h"
#include "machine/8255ppi.h"
#include "machine/z80ctc.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "sound/dac.h"
#include "vidhrdw/tlc34076.h"

static UINT16 *mainram, *vram;
static UINT8 command_pending, ppi_clock, ppi_data, pic_clock, pic_data;
static UINT16 dsw_shift;
static UINT8 dsw_control;
static int shift_full, video_bpp;
static UINT32 display_base;
static int display_rowbytes, display_scanline;
static UINT8 sound_data, sound_sent, sound_back, sound_bank[4], sound_volume[4], sound_sample[4];

static READ8_HANDLER( pic_a_r )
{
	command_pending = 0;
	return ppi_data | (ppi_clock << 3);
}
static WRITE8_HANDLER( pic_a_w )
{
	pic_data = data & 1;
	pic_clock = (data >> 2) & 1;
}
static READ8_HANDLER( pic_b_r )
{
	return command_pending | ((dsw_shift >> 14) & 2) | ((readinputport(2) << 4) & 0x30);
}
static WRITE8_HANDLER( pic_b_w )
{
	if (!(data & 4))
		dsw_shift = (readinputport(4) << 8) | readinputport(3);
	else if ((data & 8) && !(dsw_control & 8))
		dsw_shift <<= 1;
	dsw_control = data;
}
static READ8_HANDLER( ppi_c_r )
{
	return (pic_data << 1) | (pic_clock << 3);
}
static WRITE8_HANDLER( ppi_c_w )
{
	/* The PPI starts with floating outputs before firmware configures port C. */
	if (data == 0xff)
		return;
	if (data & 0x10)
		command_pending = 1;
	ppi_data = (data >> 6) & 1;
	ppi_clock = data >> 7;
}
static const ppi8255_interface ppi_interface =
{
	1, { input_port_0_r }, { input_port_1_r }, { ppi_c_r },
	{ 0 }, { 0 }, { ppi_c_w }
};
static READ16_HANDLER( ppi_r ) { return ppi8255_0_r(offset); }
static WRITE16_HANDLER( ppi_w )
{
	if (ACCESSING_LSB)
		ppi8255_0_w(offset, data);
}

static READ16_HANDLER( host_r ) { return tms34010_host_r(1, offset); }
static WRITE16_HANDLER( host_w )
{
	/* Byte writes must preserve the other half of the shared host registers. */
	if (offset != TMS34010_HOST_DATA && mem_mask)
	{
		UINT16 old = tms34010_host_r(1, offset);
		data = (old & mem_mask) | (data & ~mem_mask);
	}
	tms34010_host_w(1, offset, data);
	cpu_boost_interleave(0, TIME_IN_USEC(50));
}
static void video_irq(int state)
{
	cpunum_set_input_line(0, 4, state ? ASSERT_LINE : CLEAR_LINE);
}
static void to_shiftreg(offs_t address, UINT16 *shiftreg)
{
	if (!shift_full)
	{
		memcpy(shiftreg, &vram[(address & ~0x1fff) >> 4], 0x400);
		shift_full = 1;
	}
}
static void from_shiftreg(offs_t address, UINT16 *shiftreg)
{
	memcpy(&vram[(address & ~0x1fff) >> 4], shiftreg, 0x400);
	shift_full = 0;
}
static void display_changed(UINT32 address, int rowbytes, int scanline)
{
	force_partial_update(scanline - 1);
	display_base = address;
	display_rowbytes = rowbytes;
	display_scanline = scanline;
}
static struct tms34010_config tms_config =
{
	1, video_irq, to_shiftreg, from_shiftreg, display_changed, 0
};
static VIDEO_UPDATE( megaphx )
{
	int x, y;
	if (tms34010_io_display_blanked(1))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT32 address = display_base + (y - display_scanline) * display_rowbytes;
		UINT16 *row = &vram[(address >> 4) & 0x3ff00];
		int column = (address >> 4) & 0xff;
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			int pixels = 16 / video_bpp;
			int screen_x = x - Machine->visible_area.min_x;
			int value = row[(column + screen_x / pixels) & 0xff];
			int pen = (value >> ((screen_x % pixels) * video_bpp)) & ((1 << video_bpp) - 1);
			plot_pixel(bitmap, x, y, Machine->pens[pen]);
		}
	}
}

static void sound_irq(int state)
{
	cpunum_set_input_line(2, 0, state ? ASSERT_LINE : CLEAR_LINE);
}
static z80ctc_interface ctc_interface = { 4000000, 0, sound_irq, 0, 0, 0 };
static struct z80_irq_daisy_chain sound_daisy[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};
static READ8_HANDLER( sound_vector_r )
{
	memory_set_bankptr(1, memory_region(REGION_SOUND1) + (sound_bank[offset / 2] & 7) * 0x8000);
	return memory_region(REGION_CPU3)[0x20 + offset];
}
static WRITE8_HANDLER( sound_bank_w ) { sound_bank[offset] = data; }
static WRITE8_HANDLER( dac_w )
{
	int channel = offset / 2;
	if (offset & 1)
		sound_volume[channel] = data;
	else
		sound_sample[channel] = data;
	/* The legacy 16-bit DAC API takes offset-binary input. */
	DAC_signed_data_16_w(channel, 0x8000 + (INT8)sound_sample[channel] * sound_volume[channel]);
}
static void sound_command_sync(int data)
{
	sound_data = data;
	sound_sent = 0xff;
}
static void sound_reply_sync(int data) { sound_back = data; }
static WRITE16_HANDLER( sound_command_w )
{
	/* Deliver mailbox transitions at a common CPU time so short ACK pulses survive. */
	timer_set(TIME_NOW, data & 0xff, sound_command_sync);
	cpu_boost_interleave(0, TIME_IN_USEC(250));
}
static READ16_HANDLER( sound_reply_r ) { return sound_back; }
static READ8_HANDLER( sound_command_r ) { return sound_data; }
static WRITE8_HANDLER( sound_reply_w )
{
	timer_set(TIME_NOW, data, sound_reply_sync);
	cpu_boost_interleave(0, TIME_IN_USEC(250));
}
static READ8_HANDLER( sound_pending_r )
{
	UINT8 result = sound_sent;
	sound_sent = 0;
	return result;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00ffff) AM_RAM AM_READ(MRA16_BANK2) AM_BASE(&mainram)
	AM_RANGE(0x040000, 0x040007) AM_READWRITE(host_r, host_w)
	AM_RANGE(0x050000, 0x050001) AM_WRITE(sound_command_w)
	AM_RANGE(0x050002, 0x050003) AM_READ(sound_reply_r)
	AM_RANGE(0x060000, 0x060007) AM_READWRITE(ppi_r, ppi_w)
	AM_RANGE(0x800000, 0x8fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0xfc0000, 0xffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END
static ADDRESS_MAP_START( tms_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_BASE(&vram)
	AM_RANGE(0x04000000, 0x0400003f) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)
	AM_RANGE(0x04000090, 0x0400009f) AM_WRITENOP
	AM_RANGE(0x7fc00000, 0x7fffffff) AM_RAM AM_MIRROR(0x80000000)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0020, 0x0027) AM_READ(sound_vector_r)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROMBANK(1)
ADDRESS_MAP_END
static ADDRESS_MAP_START( sound_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x07) AM_WRITE(dac_w)
	AM_RANGE(0x10, 0x13) AM_WRITE(sound_bank_w)
	AM_RANGE(0x20, 0x23) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x30, 0x30) AM_READWRITE(sound_command_r, sound_reply_w)
	AM_RANGE(0x31, 0x31) AM_READ(sound_pending_r)
ADDRESS_MAP_END
static ADDRESS_MAP_START( pic_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READWRITE(pic_a_r, pic_a_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(pic_b_r, pic_b_w)
ADDRESS_MAP_END

static void disable_bootrom(int param)
{
	memory_set_bankptr(2, mainram);
}
static MACHINE_START( megaphx )
{
	ppi8255_init(&ppi_interface);
	z80ctc_init(0, &ctc_interface);
	return 0;
}
static MACHINE_RESET( megaphx )
{
	int channel;
	ppi8255_0_w(3, 0x9b);
	z80ctc_reset(0);
	command_pending = ppi_clock = ppi_data = pic_clock = pic_data = 0;
	dsw_shift = dsw_control = shift_full = 0;
	display_base = display_rowbytes = display_scanline = 0;
	sound_data = sound_sent = sound_back = 0;
	memset(sound_bank, 0, sizeof(sound_bank));
	memset(sound_volume, 0, sizeof(sound_volume));
	memset(sound_sample, 0, sizeof(sound_sample));
	for (channel = 0; channel < 4; ++channel)
		DAC_signed_data_16_w(channel, 0x8000);
	memory_set_bankptr(1, memory_region(REGION_SOUND1));
	tlc34076_reset(8);
	/* The reset overlay supplies vectors until the CPU enters the high ROM area. */
	memory_set_bankptr(2, memory_region(REGION_USER1));
	timer_set(TIME_IN_CYCLES(50, 0), 0, disable_bootrom);
}

static INPUT_PORTS_START( megaphx )
	PORT_START_TAG("P1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON2)        PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON3)        PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON1)        PORT_PLAYER(1)

	PORT_START_TAG("P2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON2)        PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_BUTTON3)        PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON1)        PORT_PLAYER(2)

	PORT_START_TAG("START")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START2)

	PORT_START_TAG("DSW1")
	PORT_DIPNAME(0x07, 0x03, DEF_STR( Coin_A ))
	PORT_DIPSETTING(   0x07, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(   0x06, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(   0x05, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(   0x04, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(   0x03, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(   0x02, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(   0x01, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_4C ))
	PORT_DIPNAME(0x38, 0x18, DEF_STR( Coin_B ))
	PORT_DIPSETTING(   0x38, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(   0x30, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(   0x28, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(   0x20, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(   0x18, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(   0x10, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(   0x08, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(   0x00, DEF_STR( 1C_4C ))
	PORT_DIPNAME(0xc0, 0x80, DEF_STR( Lives ))
	PORT_DIPSETTING(   0x00, "2")
	PORT_DIPSETTING(   0x40, "3")
	PORT_DIPSETTING(   0x80, "4")
	PORT_DIPSETTING(   0xc0, "5")

	PORT_START_TAG("DSW2")
	PORT_DIPNAME(0x01, 0x00, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(   0x01, DEF_STR( Off ))
	PORT_DIPSETTING(   0x00, DEF_STR( On ))
	PORT_SERVICE(0x02, IP_ACTIVE_HIGH)
	PORT_DIPNAME(0x1c, 0x10, DEF_STR( Difficulty))
	PORT_DIPSETTING(   0x00, "0")
	PORT_DIPSETTING(   0x04, "1")
	PORT_DIPSETTING(   0x08, "2")
	PORT_DIPSETTING(   0x0c, "3")
	PORT_DIPSETTING(   0x10, "4")
	PORT_DIPSETTING(   0x14, "5")
	PORT_DIPSETTING(   0x18, "6")
	PORT_DIPSETTING(   0x1c, "7")
	PORT_DIPNAME(0x20, 0x20, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x20, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x40, 0x40, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x40, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x80, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
INPUT_PORTS_END

static INPUT_PORTS_START( hamboy )
	PORT_INCLUDE(megaphx)

	PORT_MODIFY("DSW1")
	PORT_DIPNAME(0x07, 0x06, DEF_STR( Coin_A ))
	PORT_DIPSETTING(   0x00, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(   0x03, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(   0x05, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(   0x01, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(   0x06, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(   0x02, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(   0x04, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(   0x07, DEF_STR( 1C_4C ))
	PORT_DIPNAME(0x38, 0x30, DEF_STR( Coin_B ))
	PORT_DIPSETTING(   0x00, DEF_STR( 5C_1C ))
	PORT_DIPSETTING(   0x18, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(   0x28, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(   0x08, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(   0x30, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(   0x10, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(   0x20, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(   0x38, DEF_STR( 1C_4C ))
	PORT_DIPNAME(0xc0, 0x80, DEF_STR( Lives ))
	PORT_DIPSETTING(   0x00, "1")
	PORT_DIPSETTING(   0x40, "2")
	PORT_DIPSETTING(   0x80, "3")
	PORT_DIPSETTING(   0xc0, "4")

	PORT_MODIFY("DSW2")
	PORT_DIPNAME(0x01, 0x00, DEF_STR( Demo_Sounds ))
	PORT_DIPSETTING(   0x01, DEF_STR( Off ))
	PORT_DIPSETTING(   0x00, DEF_STR( On ))
	PORT_SERVICE(0x02, IP_ACTIVE_HIGH)
	PORT_DIPNAME(0x04, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x04, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x08, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x08, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x10, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x10, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x20, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x20, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x40, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x40, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
	PORT_DIPNAME(0x80, 0x80, DEF_STR( Unknown ))
	PORT_DIPSETTING( 0x80, DEF_STR( Off ))
	PORT_DIPSETTING( 0, DEF_STR( On ))
INPUT_PORTS_END

static MACHINE_DRIVER_START( megaphx )
	MDRV_CPU_ADD(M68000, 16000000/2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(tms_map, 0)
	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_CONFIG(sound_daisy)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_io, 0)
	MDRV_CPU_ADD(PIC16C54, 6000000/PIC16C5x_CLOCK_DIVIDER)
	MDRV_CPU_IO_MAP(pic_io, 0)
	MDRV_INTERLEAVE(1000)
	MDRV_MACHINE_START(megaphx)
	MDRV_MACHINE_RESET(megaphx)
	MDRV_FRAMES_PER_SECOND(40000000.0 / (12 * 205 * 315))
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(410, 315)
	MDRV_VISIBLE_AREA(20, 357, 44, 289)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_UPDATE(megaphx)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hamboy )
	MDRV_IMPORT_FROM(megaphx)
	MDRV_FRAMES_PER_SECOND(40000000.0 / (12 * 201 * 313))
	MDRV_SCREEN_SIZE(402, 313)
	MDRV_VISIBLE_AREA(28, 351, 66, 265)
MACHINE_DRIVER_END

ROM_START( megaphx )
	ROM_REGION16_BE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "mph6.u32", 0x000001, 0x20000, CRC(b99703d4) SHA1(393b6869e71d4c61060e66e0e9e36a1e6ca345d1) )
	ROM_LOAD16_BYTE( "mph7.u21", 0x000000, 0x20000, CRC(f11e7449) SHA1(1017142d10011d68e49d3ccdb1ac4e815c03b17a) )

	ROM_REGION16_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "mph0.u38", 0x000001, 0x20000, CRC(b63dd20f) SHA1(c8ce5985a6ba49428d66a49d9d623ccdfce422c2) )
	ROM_LOAD16_BYTE( "mph1.u27", 0x000000, 0x20000, CRC(4dcbf44b) SHA1(a8fa49ecd033f1aeb323e0032ddcf5f8f9463ac0) )
	ROM_LOAD16_BYTE( "mph2.u37", 0x040001, 0x20000, CRC(a0f69c27) SHA1(d0c5c241d94a1f03f51e7e517e2f9dec6abcf75a) )
	ROM_LOAD16_BYTE( "mph3.u26", 0x040000, 0x20000, CRC(4db84cc5) SHA1(dd74acd4b32c7e7553554ac0f9ba13503358e869) )
	ROM_LOAD16_BYTE( "mph4.u36", 0x080001, 0x20000, CRC(c8e0725e) SHA1(b3af315b9a94a692e81e0dbfd4035036c2af4f50) )
	ROM_LOAD16_BYTE( "mph5.u25", 0x080000, 0x20000, CRC(c95ccb69) SHA1(9d14cbfafd943f6ff461a7f373170a35e36eb695) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "sonido_mph1.u39", 0x00000, 0x20000, CRC(f5e65557) SHA1(5ae759c2bcef96fbda42f088c02b6dec208030f3) )
	ROM_LOAD( "sonido_mph2.u38", 0x20000, 0x20000, CRC(7444d0f9) SHA1(9739b48993bccea5530533b67808d13d6155ffe3) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 )
	ROM_LOAD( "sonido_mph0.u35", 0x000000, 0x2000,  CRC(abc1b140) SHA1(8384a162d85cf9ea870d22f44b1ca64001c6a083) )

	ROM_REGION( 0x100000, REGION_CPU4, 0 )
	ROM_LOAD( "pic16c54-xt.bin", 0x000000, 0x430,  CRC(21f396fb) SHA1(c8badb9b3681e684bced0ced1de4c3a15641de8b) )
	ROM_FILL(0x2c, 1, 0x01)

	ROM_REGION( 0x1000, REGION_USER3, 0 )
	ROM_LOAD( "p31_u31_palce16v8h-25.jed", 0x000, 0xbd4, BAD_DUMP CRC(05ef04b7) SHA1(330dd81a832b6675fb0473868c26fe9bec2da854) )
	ROM_LOAD( "p40_u29_palce16v8h-25.jed", 0x000, 0xbd4, BAD_DUMP CRC(44b7e51c) SHA1(b8b34f3b319d664ec3ad72ed87d9f65701f183a5) )
ROM_END

ROM_START( hamboy )
	ROM_REGION16_BE( 0x40000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "hb8.u32", 0x000001, 0x20000, CRC(4f7b142a) SHA1(e6e6cb05672e4f99def69be2f4cbc56f5d37f226) )
	ROM_LOAD16_BYTE( "hb9.u21", 0x000000, 0x20000, CRC(138e294f) SHA1(671b34395f0773889ddf6aa1f4291df981d1b059) )

	ROM_REGION16_BE( 0x100000, REGION_USER2, 0 )
	ROM_LOAD16_BYTE( "hb0.u38", 0x000001, 0x20000, CRC(b946a47f) SHA1(7f78a198fa3c5a00c124ab62473da4cddc0ac31f) )
	ROM_LOAD16_BYTE( "hb1.u27", 0x000000, 0x20000, CRC(890e1571) SHA1(e4a50a1849e9bc9853070da160e042f13737d8d2) )
	ROM_LOAD16_BYTE( "hb2.u37", 0x040001, 0x20000, CRC(b71b0aad) SHA1(fb2525a1581e6aa9a60ce76a09947b8d1941951c) )
	ROM_LOAD16_BYTE( "hb3.u26", 0x040000, 0x20000, CRC(1d0d61b9) SHA1(d747c8c31a81364d85ac00c50cefd695868d916d) )
	ROM_LOAD16_BYTE( "hb4.u36", 0x080001, 0x20000, CRC(9b81948e) SHA1(9e8bbee7f19e97d81205add7dbe89b353c6ab25a) )
	ROM_LOAD16_BYTE( "hb5.u25", 0x080000, 0x20000, CRC(23885e08) SHA1(f0233e3e007715d1b0b94fd52ffb597d667dc818) )
	ROM_LOAD16_BYTE( "hb6.u35", 0x0c0001, 0x20000, CRC(0c479648) SHA1(a07947b0b4e526b853782545bfdf73effa6c6579) )
	ROM_LOAD16_BYTE( "hb7.u24", 0x0c0000, 0x20000, CRC(297a6944) SHA1(7c5c66412db7905b0302a4e451fc20c20233990e) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "sonido_hammerboy_0.u39", 0x00000, 0x20000, CRC(8d94ac97) SHA1(3447e4b5670880a9b222cba84f5630e8ed42c2d3) )
	ROM_LOAD( "sonido_hammerboy_1.u38", 0x20000, 0x20000, CRC(f92e5098) SHA1(10c869c7b1250a119bf201bfc8c586e9340c2a66) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 )
	ROM_LOAD( "hammerboy.u35", 0x000000, 0x2000,  CRC(cd22f2a4) SHA1(c5cf5b1ce528412493e2b5f565ed38e3e9123d37) )

	ROM_REGION( 0x100000, REGION_CPU4, 0 )
	ROM_LOAD( "pic16c54-xt.bin", 0x000000, 0x430, CRC(21f396fb) SHA1(c8badb9b3681e684bced0ced1de4c3a15641de8b) )
	ROM_FILL(0x2c, 1, 0x01)
ROM_END

static DRIVER_INIT( megaphx )
{
	video_bpp = 8;
	pic16c5x_config(0x0ff9);
}
static DRIVER_INIT( hamboy )
{
	video_bpp = 4;
	pic16c5x_config(0x0ff9);
}

GAME( 1991, megaphx, 0, megaphx, megaphx, megaphx, ROT0, "Dinamic / Inder", "Mega Phoenix", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1990, hamboy, 0, hamboy, hamboy, hamboy, ROT0, "Dinamic / Inder", "Hammer Boy", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
