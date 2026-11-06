// license:BSD-3-Clause
// copyright-holders:Pierpaolo Prazzoli, Phil Bennett
/*
    Eolith 32 bits hardware: Vega system
    Crazy War, Eolith, 2002.
*/

#include "driver.h"
#include "cpu/e132xs/e132xs.h"
#include "cpu/i8051/i8051.h"
#include "machine/at28c16.h"
#include "vidhrdw/generic.h"
#include "includes/eolith.h"
#include "sound/custom.h"

static UINT8 *vram;
static int vbuffer, qs_bank;

static READ32_HANDLER( vega_vram_r )
{
	UINT8 *source = vram + vbuffer * 0x14000 + offset * 4;
	return ((UINT32)source[0] << 24) | (source[1] << 16) | (source[2] << 8) | source[3];
}

static WRITE32_HANDLER( vega_vram_w )
{
	UINT8 *dest = vram + vbuffer * 0x14000 + offset * 4;
	int i;
	for (i = 0; i < 4; ++i)
	{
		int shift = 24 - i * 8;
		UINT8 value = data >> shift;
		UINT8 mask = mem_mask >> shift;
		/* Transparency belongs to each byte, including within word and dword writes. */
		if (value != 0xff)
			dest[i] = (dest[i] & mask) | (value & ~mask);
	}
}

static WRITE32_HANDLER( vega_palette_w )
{
	UINT16 value;
	COMBINE_DATA(&paletteram32[offset]);
	value = paletteram32[offset];
	palette_set_color(offset, pal5bit(value >> 10), pal5bit(value >> 5), pal5bit(value));
}

static READ32_HANDLER( vega_eeprom_r )
{
	int shift = (offset & 3) * 8;
	return (at28c16_32le_0_r(offset >> 2, ~(0xffU << shift)) >> shift) & 0xff;
}

static WRITE32_HANDLER( vega_eeprom_w )
{
	int shift = (offset & 3) * 8;
	/* Only the low data byte is connected; EEPROM addresses advance once per dword. */
	if (ACCESSING_LSB32)
		at28c16_32le_0_w(offset >> 2, (data & 0xff) << shift, ~(0xffU << shift));
}

static WRITE32_HANDLER( vega_misc_w )
{
	if (ACCESSING_LSB32)
		vbuffer = data & 1;
}

static READ32_HANDLER( vega_system_r )
{
	int scanline = cpu_getscanline();
	if (scanline < 240 && activecpu_get_pc() == 0x00008cf8)
		cpu_spinuntil_time(cpu_getscanlinetime(240));
	return (readinputport(0) & ~0x40) | (scanline < 240 ? 0x40 : 0);
}

static WRITE32_HANDLER( vega_sound_w )
{
	if (ACCESSING_LSB32)
	{
		soundlatch_w(0, data);
		cpunum_set_input_line(1, I8051_INT1_LINE, ASSERT_LINE);
		cpu_boost_interleave(0, TIME_IN_USEC(250));
	}
}

static READ8_HANDLER( qs_rom_r )
{
	return memory_region(REGION_CPU2)[qs_bank * 0x10000 + offset + 0x100];
}

static READ8_HANDLER( qs_command_r )
{
	return soundlatch_r(0);
}

static WRITE8_HANDLER( qs_p3_w )
{
	qs_bank = data & 7;
	if (!(data & 0x20))
		cpunum_set_input_line(1, I8051_INT1_LINE, CLEAR_LINE);
}

static ADDRESS_MAP_START( vega_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM
	AM_RANGE(0x80000000, 0x80013fff) AM_READWRITE(vega_vram_r, vega_vram_w)
	AM_RANGE(0xfc000000, 0xfc0000ff) AM_READWRITE(vega_eeprom_r, vega_eeprom_w)
	AM_RANGE(0xfc200000, 0xfc2003ff) AM_RAM AM_WRITE(vega_palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0xfc400000, 0xfc40005b) AM_WRITENOP
	AM_RANGE(0xfc600000, 0xfc600003) AM_WRITE(vega_sound_w)
	AM_RANGE(0xfca00000, 0xfca00003) AM_WRITE(vega_misc_w)
	AM_RANGE(0xfcc00000, 0xfcc00003) AM_READ(vega_system_r)
	AM_RANGE(0xfce00000, 0xfce00003) AM_READ(input_port_1_dword_r)
	AM_RANGE(0xfd000000, 0xfeffffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_CPU1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qs_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( qs_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x00ff) AM_RAM
	AM_RANGE(0x0100, 0xffff) AM_READ(qs_rom_r)
	AM_RANGE(0x0200, 0x0211) AM_WRITE(eolith_qs_wave_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( qs_ports, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(1, 1) AM_READ(qs_command_r)
	AM_RANGE(2, 2) AM_WRITE(eolith_qs_p2_w)
	AM_RANGE(3, 3) AM_WRITE(qs_p3_w)
ADDRESS_MAP_END

INPUT_PORTS_START( crazywar )
	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x00000020, IP_ACTIVE_LOW )
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xffffff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xffff0000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static VIDEO_START( vega )
{
	vram = auto_malloc(0x14000 * 2);
	memset(vram, 0, 0x14000 * 2);
	return 0;
}

static VIDEO_UPDATE( vega )
{
	int x, y;
	UINT8 *source = vram + (vbuffer ^ 1) * 0x14000;
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT16 *dest = bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			dest[x] = Machine->pens[source[y * 320 + x]];
	}
}

static NVRAM_HANDLER( vega )
{
	int offset;
	nvram_handler_at28c16_0(file, read_or_write);
	/* Unprogrammed EEPROM cells read high, including settings the game has not written. */
	if (!read_or_write && !file)
		for (offset = 0; offset < 0x800 / 4; ++offset)
			at28c16_32le_0_w(offset, 0xffffffff, 0);
}

static MACHINE_RESET( vega )
{
	vbuffer = qs_bank = 0;
	eolith_qs_reset();
	/* Configure the external address pins after the sound CPU finishes resetting. */
	timer_set(TIME_NOW, 1, eolith_qs_configure);
}

static MACHINE_DRIVER_START( vega )
	MDRV_CPU_ADD(GMS30C2132, 55000000)
	MDRV_CPU_PROGRAM_MAP(vega_map, 0)
	MDRV_CPU_ADD(I8052, 24000000)
	MDRV_CPU_PROGRAM_MAP(qs_map, 0)
	MDRV_CPU_DATA_MAP(qs_data, 0)
	MDRV_CPU_IO_MAP(qs_ports, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_RESET(vega)
	MDRV_NVRAM_HANDLER(vega)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 262)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(vega)
	MDRV_VIDEO_UPDATE(vega)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 24000000)
	MDRV_SOUND_CONFIG(eolith_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( crazywar )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "u7",         0x00000, 0x80000, CRC(697c2505) SHA1(c787007f05d2ddf1706e15e9d9ef9b2479708f12) )

	ROM_REGION32_BE( 0x2000000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD32_WORD_SWAP( "00", 0x0000000, 0x200000, CRC(fbb917ae) SHA1(1fd975cda06b3cb748503b7c8009e6184b46af3f) )
	ROM_LOAD32_WORD_SWAP( "01", 0x0000002, 0x200000, CRC(59308556) SHA1(bc8c28531fca009be5b7b3b1a4a9b3ebcc9d3c3a) )
	ROM_LOAD32_WORD_SWAP( "02", 0x0400000, 0x200000, CRC(34813167) SHA1(d04c71164b36af78425dcd637e60aee45c39a1ba) )
	ROM_LOAD32_WORD_SWAP( "03", 0x0400002, 0x200000, CRC(7fcb0a53) SHA1(f74e0512b5d4854d0c4b04bf8c917f8dccb4dc0f) )
	ROM_LOAD32_WORD_SWAP( "04", 0x0800000, 0x200000, CRC(f8eb8ce5) SHA1(a631f6979a9df2fda622483256ea569c6b4d1586) )
	ROM_LOAD32_WORD_SWAP( "05", 0x0800002, 0x200000, CRC(14d854df) SHA1(5527fb1a12193e27a3fad5ca7f4e3027f462ee50) )
	ROM_LOAD32_WORD_SWAP( "06", 0x0c00000, 0x200000, CRC(31c67f0a) SHA1(7a587bb86bc6450c66016c82efe047f2d350d586) )
	ROM_LOAD32_WORD_SWAP( "07", 0x0c00002, 0x200000, CRC(dddf93d2) SHA1(c982f18c4bd242885a6150252c9c2fa4a07ebf4b) )
	ROM_LOAD32_WORD_SWAP( "08", 0x1000000, 0x200000, CRC(dc37bcb9) SHA1(144050056905e3dce08795d1a4ac17a45f2a1fec) )
	ROM_LOAD32_WORD_SWAP( "09", 0x1000002, 0x200000, CRC(86ba59cc) SHA1(566cc6527188e24a6eae4a64131deca7e2140ada) )
	ROM_LOAD32_WORD_SWAP( "10", 0x1400000, 0x200000, CRC(524bf126) SHA1(85a27a74ba4caaf3ab1e1f0e8e8b516bb0182ae7) )
	ROM_LOAD32_WORD_SWAP( "11", 0x1400002, 0x200000, CRC(613b2764) SHA1(7a7c85c8cf1cba74e2e98a3b77d5ea44bb76a563) )
	ROM_LOAD32_WORD_SWAP( "12", 0x1800000, 0x200000, CRC(3c81d117) SHA1(76d6728f8c55e68c84d68ff2f242684bde30f4dd) )
	ROM_LOAD32_WORD_SWAP( "13", 0x1800002, 0x200000, CRC(b86545a0) SHA1(4aaa23c37d776647f3288ba541cefa79ddbd962d) )
	ROM_LOAD32_WORD_SWAP( "14", 0x1c00000, 0x200000, CRC(38ede322) SHA1(9496685a1280885a61a568047c4a8c2cd70d1b83) )
	ROM_LOAD32_WORD_SWAP( "15", 0x1c00002, 0x200000, CRC(d35e630a) SHA1(8c220f1baddd39cc978e3e5a874cc58e78b74c62) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 )
	ROM_LOAD( "bgm.u84",      0x000000, 0x080000, CRC(13aa7778) SHA1(131f74e1b73dd7a7038864593dc7ca24af0ffc30) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )
	ROM_LOAD( "effect.u85",   0x000000, 0x100000, CRC(9159fcc6) SHA1(2be9a197a51303a0da9484dced12a3f6d3b0d867) )
	ROM_LOAD( "qs1001a.u86",  0x200000, 0x080000, CRC(d13c6407) SHA1(57b14f97c7d4f9b5d9745d3571a0b7115fbe3176) )
ROM_END

GAME( 2002, crazywar, 0, vega, crazywar, 0, ROT0, "Eolith", "Crazy War", 0 )
