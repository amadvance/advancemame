// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
/*

Sliver
Hollow Corp., 1996

PCB Layout
----------

WS16-AJ-940820
|---------|-----------|------------------------------------|
|   TL084 |   i8031   |   KA-1                             |
|         |-----------||--------|                          |
|  AD-65    KA-2       |ACTEL   |   KA-6  KA-7  KA-8  KA-9 |
|                      |A1020B  |                          |
|           KA-3       |PL84C   |                          |
|               PAL    |        |                          |
|                      |--------|                          |
|             AT76C176                62256        62256   |
|                                                          |
|J             KM75C02                62256        62256   |
|A                                                         |
|M                 PAL                62256        62256   |
|M                                                         |
|A                                    62256        62256   |
|                                                  HY534256|
|   DSW1           PAL                 |-------|   HY534256|
|          PAL     PAL                 |ZORAN  |   HY534256|
|                            |-------| |ZR36011|   HY534256|
|   DSW2   62256    62256    |ZORAN  | |-------|           |
|          KA-4     KA-5     |ZR36050|                     |
|                            |-------|        |-------|    |
|        |------------------|16MHz            |FUJI   |    |
|        |                  |                 |MD0204 |    |
|        |    MC68000P10    |                 |-------|    |
|        |                  |           KA-12  KA-11  KA-10|
|24MHz   |------------------|                              |
|----------------------------------------------------------|
Notes:
      i8031    - Intel 8031 CPU, clock 8.000MHz (DIP40)
      68000    - Motorola MC68000 CPU, clock 12.000MHz (DIP64)
      AT76C176 - Atmel AT76C176 1-Channel 6-Bit AD/DA Convertor with Clamp Circuit (DIP28).
                 When removed, text and _some_ graphics turn black (palette related use)
                 This chip is compatible to Fujitsu MB40176
      A1020B   - Actel A1020B FPGA (PLCC84)
      ZR36050  - Zoran ZR36050PQC-21 DF4B9423G (QFP100)
      ZR36011  - Zoran ZR36011PQC JAPAN 079414 (QFP100)
      MD0204   - Fuji MD0204 JAPAN F39D110 (QFP128)
      62256    - 32K x8 SRAM (DIP28)
      HY534256 - Hyundai 256K x4 (1MBit) DRAM (DIP20)
      KM75C02  - Samsung KM75C02 FIFO RAM (DIP28)
      AD-65    - Clone OKI M6295 (QFP44), clock 1.000MHz, sample rate = 1000000Hz / 132
      VSync    - 60Hz


    The background images on this hardware are in JPEG format, the Zoran chips are
    hardware JPEG decompression chips.
    The JPEG at ROM offset 0x17f2e6 is truncated.

*/

#include "driver.h"
#include "cpu/i8051/i8051.h"
#include "sound/okim6295.h"
#include "nanojpeg.h"

#define FIFO_SIZE 1024
#define COMMAND_SIZE 8

static UINT16 fifo[FIFO_SIZE];
static UINT16 tempbuf[COMMAND_SIZE];
static int fptr, tmp_counter;
static int oki_bank;
static UINT8 dac_index, dac_component;
static UINT8 dac_colors[0x100][3];
static mame_bitmap *sliver_bitmap_fg;
static mame_bitmap *sliver_bitmap_bg;
static UINT16 jpeg1, jpeg2;
static UINT8 io_offset, io_reg[0x100];
static int jpeg_x, jpeg_y, jpeg_selected, jpeg_decoded;
static UINT32 jpeg_cached_addr;

static WRITE16_HANDLER( sliver_RAMDAC_offset_w )
{
	if (ACCESSING_LSB)
	{
		dac_index = data;
		dac_component = 0;
	}
}

static WRITE16_HANDLER( sliver_RAMDAC_color_w )
{
	if (ACCESSING_LSB)
	{
		dac_colors[dac_index][dac_component] = data & 0x3f;
		palette_set_color(dac_index, pal6bit(dac_colors[dac_index][0]),
			pal6bit(dac_colors[dac_index][1]), pal6bit(dac_colors[dac_index][2]));
		if (++dac_component == 3)
		{
			dac_component = 0;
			dac_index++;
		}
	}
}

static WRITE16_HANDLER( fifo_data_w )
{
	COMBINE_DATA(&tempbuf[tmp_counter]);
	if (++tmp_counter == COMMAND_SIZE)
	{
		if (fptr <= FIFO_SIZE - COMMAND_SIZE)
		{
			memcpy(&fifo[fptr], tempbuf, sizeof(tempbuf));
			fptr += COMMAND_SIZE;
		}
		tmp_counter = 0;
	}
}

static WRITE16_HANDLER( fifo_clear_w )
{
	fillbitmap(sliver_bitmap_fg, 0, NULL);
	fptr = tmp_counter = 0;
}

static WRITE16_HANDLER( fifo_flush_w )
{
	const UINT8 *rom = memory_region(REGION_GFX1);
	int ptr;

	for (ptr = 0; ptr + COMMAND_SIZE <= fptr; ptr += COMMAND_SIZE)
	{
		UINT32 romoffs = fifo[ptr] + (fifo[ptr+1] << 8) + (fifo[ptr+2] << 16);
		int w = fifo[ptr+3] + 1;
		int h = fifo[ptr+4] + 1;
		int x, y;

		if (fifo[ptr+7] != 0)
			continue;

		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{
				UINT8 pixel = rom[romoffs++ & 0x1fffff];
				int sx = fifo[ptr+5] + w - 1 - x;
				int sy = fifo[ptr+6] + h - 1 - y;

				if (pixel && sx >= 0 && sx < 384 && sy >= 0 && sy < 256)
					((UINT32 *)sliver_bitmap_fg->line[sy])[sx] = Machine->pens[pixel];
			}
		}
	}
}

static void render_jpeg(void)
{
	UINT32 addr = jpeg2 | ((UINT32)jpeg1 << 16);
	int width, height, components, x, y;
	UINT8 *pixels;

	fillbitmap(sliver_bitmap_bg, 0, NULL);
	if (!jpeg_selected)
		return;

	if (addr != jpeg_cached_addr)
	{
		jpeg_cached_addr = addr;
		jpeg_decoded = 0;
		if (addr < memory_region_length(REGION_GFX2))
		{
			nj_result_t result = njDecode(memory_region(REGION_GFX2) + addr,
				memory_region_length(REGION_GFX2) - addr);
			if (result == NJ_OK)
				jpeg_decoded = 1;
			else
				logerror("JPEG decode failed at %06x: %d\n", addr, result);
		}
	}
	if (!jpeg_decoded)
		return;

	width = njGetWidth();
	height = njGetHeight();
	components = njIsColor() ? 3 : 1;
	pixels = njGetImage();
	for (y = 0; y < height; y++)
	{
		int sy = y + 1 - 0x0e - jpeg_y;
		if (sy < 0 || sy >= 256)
			continue;
		for (x = 0; x < width; x++)
		{
			int sx = x - 0x45 + jpeg_x;
			const UINT8 *pixel = pixels + (y * width + x) * components;
			if (sx >= 0 && sx < 384)
			{
				UINT32 r = pixel[0];
				UINT32 g = pixel[components == 3 ? 1 : 0];
				UINT32 b = pixel[components == 3 ? 2 : 0];
				((UINT32 *)sliver_bitmap_bg->line[sy])[sx] =
					r * (direct_rgb_components[0] / 255) +
					g * (direct_rgb_components[1] / 255) +
					b * (direct_rgb_components[2] / 255);
			}
		}
	}
}

static WRITE16_HANDLER( jpeg1_w )
{
	COMBINE_DATA(&jpeg1);
}

static WRITE16_HANDLER( jpeg2_w )
{
	COMBINE_DATA(&jpeg2);
	jpeg_selected = 1;
	render_jpeg();
}

static WRITE16_HANDLER( io_offset_w )
{
	if (ACCESSING_LSB)
		io_offset = data;
}

static WRITE16_HANDLER( io_data_w )
{
	if (ACCESSING_LSB)
	{
		int x, y;
		io_reg[io_offset] = data;
		y = io_reg[0x1a] + (io_reg[0x1b] << 8) - io_reg[0x20];
		x = io_reg[0x1e] + (io_reg[0x1f] << 8);
		if (x != jpeg_x || y != jpeg_y)
		{
			jpeg_x = x;
			jpeg_y = y;
			render_jpeg();
		}
	}
}

static WRITE16_HANDLER( sound_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0, data & 0xff);
		cpunum_set_input_line(1, I8051_INT0_LINE, CLEAR_LINE);
		cpunum_set_input_line(1, I8051_INT0_LINE, ASSERT_LINE);
	}
}

static READ8_HANDLER( sliver_soundlatch_r )
{
	cpunum_set_input_line(1, I8051_INT0_LINE, CLEAR_LINE);
	return soundlatch_r(0);
}

static WRITE8_HANDLER( oki_setbank )
{
	int bank = (data ^ 0xff) & 3;
	if (bank != oki_bank)
	{
		UINT8 *sound = memory_region(REGION_SOUND1);
		memcpy(sound + 0x20000, sound + 0x100000 + 0x20000 * bank, 0x20000);
		oki_bank = bank;
	}
}

static ADDRESS_MAP_START( sliver_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x100001) AM_WRITE(sliver_RAMDAC_offset_w)
	AM_RANGE(0x100002, 0x100003) AM_WRITE(sliver_RAMDAC_color_w)
	AM_RANGE(0x100004, 0x100005) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x300004, 0x300005) AM_WRITE(io_offset_w)
	AM_RANGE(0x300006, 0x300007) AM_WRITE(io_data_w)
	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x400002, 0x400003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x400004, 0x400005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x400006, 0x400007) AM_WRITE(fifo_data_w)
	AM_RANGE(0x400008, 0x400009) AM_WRITE(fifo_clear_w)
	AM_RANGE(0x40000a, 0x40000b) AM_WRITE(fifo_flush_w)
	AM_RANGE(0x40000c, 0x40000d) AM_WRITE(jpeg1_w)
	AM_RANGE(0x40000e, 0x40000f) AM_WRITE(jpeg2_w)
	AM_RANGE(0x400010, 0x400015) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x400016, 0x400017) AM_WRITE(sound_w)
	AM_RANGE(0x400018, 0x400019) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xff0000, 0xffffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundmem_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundmem_data, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0100, 0x0100) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x0101, 0x0101) AM_READ(sliver_soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( soundmem_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0001, 0x0001) AM_WRITE(oki_setbank)
ADDRESS_MAP_END

static VIDEO_START( sliver )
{
	sliver_bitmap_fg = auto_bitmap_alloc(512, 256);
	sliver_bitmap_bg = auto_bitmap_alloc(512, 256);
	if (!sliver_bitmap_fg || !sliver_bitmap_bg)
		return 1;
	fillbitmap(sliver_bitmap_fg, 0, NULL);
	fillbitmap(sliver_bitmap_bg, 0, NULL);
	njInit();
	add_exit_callback(njDone);
	return 0;
}

static VIDEO_UPDATE( sliver )
{
	copybitmap(bitmap, sliver_bitmap_bg, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	copybitmap(bitmap, sliver_bitmap_fg, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
}

static MACHINE_RESET( sliver )
{
	fptr = tmp_counter = 0;
	njDone();
	jpeg1 = jpeg2 = 0;
	jpeg_x = jpeg_y = jpeg_selected = jpeg_decoded = 0;
	jpeg_cached_addr = ~0U;
	io_offset = 0;
	memset(io_reg, 0, sizeof(io_reg));
	fillbitmap(sliver_bitmap_bg, 0, NULL);
	dac_index = dac_component = 0;
	memset(fifo, 0, sizeof(fifo));
	memset(tempbuf, 0, sizeof(tempbuf));
	memset(dac_colors, 0, sizeof(dac_colors));
	oki_bank = -1;
	oki_setbank(0, 0xff);
	cpunum_set_input_line(1, I8051_INT0_LINE, CLEAR_LINE);
	fillbitmap(sliver_bitmap_fg, 0, NULL);
}

static INTERRUPT_GEN( sliver_obj_interrupt )
{
	cpunum_set_input_line(0, 3, HOLD_LINE);
}

static INPUT_PORTS_START( sliver )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0xffa4, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0030, 0x0020, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0030, "2" )
	PORT_DIPSETTING(      0x0020, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Game Time" )
	PORT_DIPSETTING(      0x0000, "Longer" )
	PORT_DIPSETTING(      0x0040, "Long" )
	PORT_DIPSETTING(      0x00c0, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0080, "Short" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0400, 0x0400, "2 Player Mode" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Yes ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Draw Insert" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Joystick Input Mode" )
	PORT_DIPSETTING(      0x2000, "Step" )
	PORT_DIPSETTING(      0x0000, "Continuous" )
	PORT_DIPNAME( 0x4000, 0x4000, "Game Paused (Test)" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( sliver )
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(sliver_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(sliver_obj_interrupt,TIME_IN_HZ(60))
	MDRV_MACHINE_RESET(sliver)

	MDRV_CPU_ADD(I8051, 8000000)
	MDRV_CPU_PROGRAM_MAP(soundmem_prg,0)
	MDRV_CPU_DATA_MAP(soundmem_data,0)
	MDRV_CPU_IO_MAP(soundmem_io,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)


	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 384-1-16, 0*8, 240-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(sliver)
	MDRV_VIDEO_UPDATE(sliver)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.6)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.6)
MACHINE_DRIVER_END


ROM_START( sliver )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ka-4.bin", 0x00001, 0x20000, CRC(4906367f) SHA1(cc030930ffe7018ba6c362cab136798d027db7d8) )
	ROM_LOAD16_BYTE( "ka-5.bin", 0x00000, 0x20000, CRC(f260dabc) SHA1(3727cb8aa652809386075b39a1d85d5b20973702) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "ka-1.bin", 0x000000, 0x10000, CRC(56e616a2) SHA1(f8952aba62ae0410e300d99e95dc8b752543af1e) )

	ROM_REGION( 0x180000, REGION_SOUND1, 0 )
	ROM_LOAD( "ka-2.bin", 0x000000, 0x20000, CRC(3df96eb0) SHA1(ec3dfc29da08f6525a1c708839f83094a6784f72) )
	ROM_LOAD( "ka-3.bin", 0x100000, 0x80000, CRC(33ee929c) SHA1(a652ad68c547248ef5fa1ed8006b7ac7aef76383) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "ka-6.bin", 0x000001, 0x80000, CRC(bd182316) SHA1(a22db9f73a2865f59630183c14201aeede821642) )
	ROM_LOAD16_BYTE( "ka-7.bin", 0x100001, 0x40000, CRC(1c5d6fb9) SHA1(372533264eb41a5f57b2a59eb039adb6334f36c5) )
	ROM_LOAD16_BYTE( "ka-8.bin", 0x000000, 0x80000, CRC(dbfd7489) SHA1(4a7b07d041dce04a8d8d6688698164f988baefc9) )
	ROM_LOAD16_BYTE( "ka-9.bin", 0x100000, 0x40000, CRC(71f044ba) SHA1(bd88bfaa0249de9fd8eb8bd25eae0126744a9046) )

	ROM_REGION( 0x180000, REGION_GFX2, 0 )
	ROM_LOAD( "ka-10.bin", 0x000000, 0x80000, CRC(a6824271) SHA1(2eefa4e61491f7b72ccde744fa6f88a1a3c60c92) )
	ROM_LOAD( "ka-11.bin", 0x080000, 0x80000, CRC(4ae121ff) SHA1(ece7cc07483801a0d436def977d72dc7b1a07c8f) )
	ROM_LOAD( "ka-12.bin", 0x100000, 0x80000, CRC(0901e142) SHA1(68ebd38beeedf53414a831c01813881feee33446) )
ROM_END

GAME( 1996, sliver, 0, sliver, sliver, 0, ROT0, "Hollow Corp", "Sliver (set 1)", GAME_IMPERFECT_GRAPHICS )
