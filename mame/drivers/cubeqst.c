// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/* Simutrek Cube Quest, 1983. */

#include "driver.h"
#include "hash.h"
#include "cpu/i8039/i8039.h"
#include "cpu/cubeqcpu/cubeqcpu.h"
#include "machine/ldpr8210.h"
#include "machine/laserdsc.h"
#include "sound/custom.h"
#include "sound/dac.h"

static UINT8 io_latch, reset_latch;
static int video_field;
static UINT16 *cube_palette;
static UINT32 colormap[65536];

static WRITE16_HANDLER( cube_palette_w )
{
	force_partial_update(cpu_getscanline());
	COMBINE_DATA(&cube_palette[offset]);
}
static READ16_HANDLER( disc_r ) { return simutrek_status(); }
static WRITE16_HANDLER( disc_w ) { simutrek_write(data & 0xff); }
static WRITE16_HANDLER( disc_audio_w ) { simutrek_audio(data & 1); }
static WRITE16_HANDLER( disc_control_w ) { simutrek_video(data & 1); }
static READ16_HANDLER( line_r ) { return cpu_getscanline(); }
static READ16_HANDLER( chop_r ) { return (readinputport(1) << 8) | readinputport(2); }
static READ16_HANDLER( io_r ) { return readinputport(0) & ((io_latch & 0x80) ? ~0x4e0 : ~0); }
static WRITE16_HANDLER( io_w ) { io_latch = data; }
static void swap_banks(int param) { cubeqcpu_swap_banks(); }
static WRITE16_HANDLER( reset_w )
{
	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(2, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, (data & 2) ? CLEAR_LINE : ASSERT_LINE);
	/* Synchronize bank changes after all processors have reached this scheduler boundary. */
	if (!(reset_latch & 1) && (data & 1)) timer_set(TIME_NOW, 0, swap_banks);
	if (!(data & 4))
	{
		cpunum_set_input_line(4, INPUT_LINE_RESET, PULSE_LINE);
		cpunum_set_input_line(5, INPUT_LINE_RESET, PULSE_LINE);
		simutrek_reset();
	}
	reset_latch = data;
}
void cubeqcpu_dac_w(UINT16 data)
{
	int channel = ((data & 1) ? 0 : 8) + ((data >> 1) & 7);
	DAC_signed_data_16_w(channel, ((data >> 4) ^ 0x800) << 4);
}

static ADDRESS_MAP_START( cubeqst_mem, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(18) )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x020000, 0x027fff) AM_READWRITE(cubeqcpu_rotram_r, cubeqcpu_rotram_w)
	AM_RANGE(0x028000, 0x028fff) AM_READWRITE(cubeqcpu_sndram_r, cubeqcpu_sndram_w)
	AM_RANGE(0x038000, 0x038001) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x038002, 0x038003) AM_READWRITE(chop_r, disc_audio_w)
	AM_RANGE(0x038008, 0x038009) AM_READWRITE(line_r, reset_w)
	AM_RANGE(0x03800e, 0x03800f) AM_READWRITE(disc_r, disc_w)
	AM_RANGE(0x03c800, 0x03c9ff) AM_RAM AM_WRITE(cube_palette_w) AM_BASE(&cube_palette)
	AM_RANGE(0x03cc00, 0x03cc01) AM_WRITE(disc_control_w)
	AM_RANGE(0x03e000, 0x03efff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x03f000, 0x03ffff) AM_RAM
ADDRESS_MAP_END
static ADDRESS_MAP_START( rotate_mem, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0, 0xfff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( bitslice_mem, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0, 0x7ff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( pr_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0, 0x7ff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( sim_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0, 0x3ff) AM_ROM
ADDRESS_MAP_END
static ADDRESS_MAP_START( pr_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0, 0x1ff) AM_READWRITE(pr8210_io_r, pr8210_io_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( sim_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0, 0x1ff) AM_READWRITE(simutrek_io_r, simutrek_io_w)
ADDRESS_MAP_END

INPUT_PORTS_START( cubeqst )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Cube")
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Right Fire")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Left Fire")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("Free Game")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)
INPUT_PORTS_END

static void load_firmware(const char *set, const char *name, int region, const char *hash)
{
	char actual[HASH_BUF_SIZE];
	mame_file *file = mame_fopen(set, name, FILETYPE_ROM, 0);
	UINT32 length = memory_region_length(region);
	/* Device firmware lives in its own ROM set, independent of the game's clone tree. */
	if (!file) fatalerror("Missing laserdisc firmware: %s/%s", set, name);
	if (mame_fsize(file) != length || mame_fread(file, memory_region(region), length) != length)
		fatalerror("Invalid laserdisc firmware length: %s", name);
	mame_fclose(file);
	hash_compute(actual, memory_region(region), length, HASH_CRC | HASH_SHA1);
	if (!hash_data_is_equal(actual, hash, 0)) fatalerror("Incorrect laserdisc firmware: %s", name);
}
static MACHINE_START( cubeqst )
{
	int i;
	load_firmware("pr8210", "pr-8210_mcu_ud6005a.bin", REGION_CPU5,
		CRC(120fa83b) SHA1(b514326ca1f52d6d89056868f9d17eabd4e3f31d));
	load_firmware("simutrek", "laser_player_interface_d8748_a308.bin", REGION_CPU6,
		CRC(eed3e728) SHA1(1eb3467f1c41553375b2c21952cd593b167f5416));
	for (i = 0; i < 65536; ++i)
	{
		int y = ((i >> 12) & 15) * 2;
		colormap[i] = MAKE_RGB(y * ((i >> 8) & 7), y * ((i >> 4) & 7), y * (i & 7));
	}
	return simutrek_init();
}
static MACHINE_RESET( cubeqst )
{
	io_latch = reset_latch = video_field = 0;
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);
	simutrek_reset();
}
static INTERRUPT_GEN( cubeqst_field )
{
	cpunum_set_input_line(0, video_field ? 6 : 5, HOLD_LINE);
	video_field ^= 1;
	simutrek_field();
}
static VIDEO_UPDATE( cubeqst )
{
	UINT32 *stack = cubeqcpu_stack();
	int y, i, x;
	UINT8 depth_buffer[512];
	UINT16 overlay[320 * 249];
	ldv1000_video(bitmap, cliprect);
	/* Resolve depth in the native graphics domain before applying the genlock scale and offset. */
	for (y = 0; y < 249; ++y)
	{
		int count = cubeqcpu_ptr(y);
		UINT16 *row = overlay + y * 320;
		for (x = 0; x < 320; ++x) row[x] = 255;
		memset(depth_buffer, 0xff, sizeof(depth_buffer));
		for (i = 0; i < count; i += 2)
		{
			UINT32 e1 = stack[(y << 7) | (i & 0x7f)], e2 = stack[(y << 7) | ((i+1) & 0x7f)];
			int h1, h2, depth, color;
			if (e1 & (1 << 19)) { h1 = (e2 >> 8) & 0x1ff; depth = e2 & 255; h2 = (e1 >> 8) & 0x1ff; color = e1 & 255; }
			else if (e2 & (1 << 19)) { h1 = (e1 >> 8) & 0x1ff; depth = e1 & 255; h2 = (e2 >> 8) & 0x1ff; color = e2 & 255; }
			else continue;
			for (x = h1; x <= h2 && x < 320; ++x)
				if (depth_buffer[x] >= depth) { depth_buffer[x] = depth; row[x] = cube_palette[color]; }
		}
	}
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		int sy = ((y - 44) * 1000 + 15840) * 249 / 494400;
		if (sy >= 249) continue;
		UINT32 *dest = (UINT32 *)bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			int sx = (x * 1000 - 1408) * 320 / 704000;
			UINT16 color = overlay[sy * 320 + MAX(0, sx)];
			if (!(color & 8)) dest[x] = colormap[color];
		}
	}
}
static MACHINE_DRIVER_START( cubeqst )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(cubeqst_mem, 0)
	MDRV_CPU_VBLANK_INT(cubeqst_field, 1)
	MDRV_CPU_ADD(CQUESTROT, 5000000)
	MDRV_CPU_PROGRAM_MAP(rotate_mem, 0)
	MDRV_CPU_ADD(CQUESTLIN, 5000000)
	MDRV_CPU_PROGRAM_MAP(bitslice_mem, 0)
	MDRV_CPU_ADD(CQUESTSND, 5000000)
	MDRV_CPU_PROGRAM_MAP(bitslice_mem, 0)
	/* This core expects machine cycles; the oscillator is divided by 15. */
	MDRV_CPU_ADD(I8039, 4410000 / I8039_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(pr_mem, 0)
	MDRV_CPU_IO_MAP(pr_io, 0)
	MDRV_CPU_ADD(I8048, 6000000 / I8039_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(sim_mem, 0)
	MDRV_CPU_IO_MAP(sim_io, 0)
	MDRV_INTERLEAVE(801)
	MDRV_MACHINE_START(cubeqst)
	MDRV_MACHINE_RESET(cubeqst)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_FRAMES_PER_SECOND(60000.0 / 1001)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(720, 524)
	MDRV_VISIBLE_AREA(0, 703, 44, 523)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_VIDEO_UPDATE(cubeqst)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(ldv1000_sound)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.125)
MACHINE_DRIVER_END

ROM_START( cubeqst )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "eprom_bd_2764.1a", 0x000000, 0x02000, CRC(870a28f5) SHA1(6db2020fa10d03bfaf7bd2892ba7266e744567e9) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.1b", 0x000001, 0x02000, CRC(0c29b2d2) SHA1(1c67dbfaeb6ae1e6ccd870a815bd51ebc520a34c) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.2a", 0x004000, 0x02000, CRC(f03a76be) SHA1(794a9e5a4fddc16950836468157ea252a82c0771) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.2b", 0x004001, 0x02000, CRC(c995b786) SHA1(0370e0be81ceea647ea9e27eac22b6c2354e695b) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.3a", 0x008000, 0x02000, CRC(dedeabf3) SHA1(d06138336bdbd5ebee44188e92e5721ae63715ad) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.3b", 0x008001, 0x02000, CRC(7353e970) SHA1(9566335868352d3c652621809b812ede309ebded) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.4a", 0x00c000, 0x02000, CRC(3f8ae830) SHA1(13411f7bee31ae13a2e139ad576e48130838c472) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.4b", 0x00c001, 0x02000, CRC(6d4fc7cf) SHA1(8f95e2f9e313413ea6b9bcd1ec5d8c11962584db) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.5a", 0x010000, 0x02000, CRC(ea1a92d0) SHA1(71a6820c67aad951d05571cd29f52c2cebe78c3c) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.5b", 0x010001, 0x02000, CRC(d12ed62f) SHA1(3e897ef88f51d6b2010944b50f1b5387801d92ce) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.6a", 0x014000, 0x02000, CRC(6d31f22b) SHA1(36ecfb1c88be9a100a272564d8ff8df02a501f88) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.6b", 0x014001, 0x02000, CRC(2110f613) SHA1(e12e3b902e9d755fc55ff67851c905e40482a98f) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.7a", 0x018000, 0x02000, CRC(8b624074) SHA1(592891b384bdab3f851799e50f51b5f04c880490) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.7b", 0x018001, 0x02000, CRC(f31f2e81) SHA1(b4ae0726ac849662fb1878094a956aa3a9fe94a2) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.8a", 0x01c000, 0x02000, CRC(8ac5ab0c) SHA1(0cf6593c53184686d7020acd27e2c26aab29137c) )
	ROM_LOAD16_BYTE( "eprom_bd_2764.8b", 0x01c001, 0x02000, CRC(39804640) SHA1(0f09cf6ef73a9199318278df15983a0cc01ece0d) )

	ROM_REGION64_BE( 0x1000, REGION_CPU2, 0)
	ROMX_LOAD( "rotate_video_board_82s131.8h",  0x0, 0x200, CRC(1dd1cc65) SHA1(3c83d48f90ba70e81c0d85fb182527afe1f1f68d), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.9h",  0x0, 0x200, CRC(2bbf0aa1) SHA1(1a4ef24a8dc27ac3b01f176aac9b1c1c194bd67b), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.10f", 0x1, 0x200, CRC(89b234eb) SHA1(23d84873b4a5088130fff960b3caded329ea70b5), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.10h", 0x1, 0x200, CRC(d86561b2) SHA1(ebc5374568d9b598237d98f0fb95b68b36a3ddff), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.7h",  0x2, 0x200, CRC(4e0909e2) SHA1(e97dcbd23e0d9eccc138e62334e016b1b333dec3), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12k", 0x2, 0x200, CRC(84db792f) SHA1(03858d92f4ad5aae39ad1925738df958cc4dd020), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12j", 0x3, 0x200, CRC(661350ba) SHA1(f1ccc31c7ce1d71f6128c575d508aa04a4a5e15f), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12d", 0x3, 0x200, CRC(c60762ec) SHA1(d113eb1ce0db4d10a55057332fb17852b312396c), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12e", 0x4, 0x200, CRC(605b924c) SHA1(899103f20ccec68e432c4abf11f554b0600f8b3c), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12c", 0x4, 0x200, CRC(fbb881b7) SHA1(b8c1592cfa082fa7dc4abd5392816ba923760cbf), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12h", 0x5, 0x200, CRC(6fb1d6f0) SHA1(5a0bf681aedde073f0164264620085e9f40eca9e), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "rotate_video_board_82s131.12f", 0x5, 0x200, CRC(5c00d30e) SHA1(ad7bbcce9eff348f78cb6e0782faa175ca928c11), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))

	ROM_REGION64_BE( 0x800, REGION_CPU3, 0)
	ROMX_LOAD( "line_drawer_82s129_u27.6b", 0x0, 0x100, CRC(a9777c19) SHA1(dcb83cc5fce3ff9760dfdfce078895ef71558f67), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u28.3b", 0x0, 0x100, CRC(2248790c) SHA1(8473276bcc154a440745b714a31c89c459fef411), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u29.3c", 0x1, 0x100, CRC(6accc743) SHA1(a5263a021c594117c14a3d4fbc207a4043001172), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u30.3f", 0x1, 0x100, CRC(ba02ac36) SHA1(89473146f04f60bbed6644603ddc646b3d4e04eb), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u31.2j", 0x2, 0x100, CRC(bbf50d89) SHA1(f49d4322123c3d4a7d6f664b7164dfe24ddd6eed), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u5.3g",  0x2, 0x100, CRC(fa5c239b) SHA1(bbf526362af263aefea87341538157f4ecad686f), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u6.2g",  0x3, 0x100, CRC(59874b50) SHA1(d304b4546c187b8c73aaf8887f0037426e60dd5f), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u7.3h",  0x3, 0x100, CRC(886956d6) SHA1(ee150a9f99f3177fcc94be5af3f87433f39b311f), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u8.3j",  0x4, 0x100, CRC(8d2b17ad) SHA1(f7408104f75e292539aaec688e220c143f762f67), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "line_drawer_82s129_u9.3k",  0x4, 0x100, CRC(0a678253) SHA1(06d7a8f9556eb156ae03184772e76c84ab8d75ea), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))

	ROM_REGION64_BE( 0x800, REGION_CPU4, 0)
	ROMX_LOAD( "mother_sounds_82s129.7e",  0x0, 0x100, CRC(cf89ad06) SHA1(2f04ca6d57a8c425f51a288f553841db680f5a7e), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.7f",  0x0, 0x100, CRC(5208036d) SHA1(ec29af4c10a098fb25424d1a5187a7bd2ec9fb92), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.11f", 0x1, 0x100, CRC(af327360) SHA1(2b45358a39daaf5721b5a94c4fd55bd3d0d90ec3), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.11e", 0x1, 0x100, CRC(b50d3e43) SHA1(9f2c071c6940767d20a563f6f2c9224e1896a5bf), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.9f",  0x2, 0x100, CRC(cd93e3c9) SHA1(f0c11db65ddf2096f6df424b7747242d9e3155da), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.10f", 0x2, 0x100, CRC(0e8b8df7) SHA1(37426ffdaf2d21273ac7e88789206fdafb14bc83), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.8f",  0x3, 0x100, CRC(07deae27) SHA1(c6a2d7cbf4ea4120f43ae088a3de90bb7a42d4a4), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.10e", 0x3, 0x100, CRC(2aaf765e) SHA1(fafe96834f5323fca71b8ab0c013f45c5c47182d), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.9e",  0x4, 0x100, CRC(598687e7) SHA1(c5045ddaab7123ff0a4c8f4c2489f9d70b63fc76), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(7))
	ROMX_LOAD( "mother_sounds_82s129.8e",  0x4, 0x100, CRC(68de17ed) SHA1(efefcb4ccdd012b767c4765304c6022b0c091066), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(7))

	ROM_REGION16_BE( 0x1000, REGION_SOUND1, 0)
	ROMX_LOAD( "mother_sounds_82s185.17f", 0x0, 0x800, CRC(0f49d40e) SHA1(40340833ab27ccb5b60baf44ad01930f204f5318), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1) )
	ROMX_LOAD( "mother_sounds_82s185.19f", 0x0, 0x800, CRC(a041ce92) SHA1(9bc92992de22b830e479933c50650c7dc23f5713), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1) )
	ROMX_LOAD( "mother_sounds_82s185.16f", 0x1, 0x800, CRC(75b1749f) SHA1(71da8224e72fb5250e2097666a47314d0a818ee9), ROM_NIBBLE | ROM_SHIFT_NIBBLE_LO | ROM_SKIP(1) )
	ROMX_LOAD( "mother_sounds_82s185.18f", 0x1, 0x800, CRC(ae4c78ae) SHA1(76fdb8eab372065882caaa5b93ce9e21efe54da5), ROM_NIBBLE | ROM_SHIFT_NIBBLE_HI | ROM_SKIP(1) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )

	ROM_LOAD( "rotate_video_board_82s129.4h", 0x000, 0x100, CRC(6ed1b8d3) SHA1(c392855e1ff8637e551891aefd6c6a3bbcf273a5) )
	ROM_LOAD( "rotate_video_board_82s129.5h", 0x100, 0x100, CRC(32d839d4) SHA1(7b82e5c1ea5a351eba60c19560a10e4702ae2c22) )

	ROM_LOAD( "fill_board_82s129.2p",  0x200, 0x100, CRC(c525b091) SHA1(bd172ed3d15f410d09017207ec68faf1d1cb4512) )
	ROM_LOAD( "fill_board_82s129.3p",  0x300, 0x100, CRC(7ed6c192) SHA1(b02bbcd0e04d3a6c54bec755df55fe6f97d9513f) )
	ROM_LOAD( "fill_board_82s129.4p",  0x400, 0x100, CRC(6b572b73) SHA1(4a065cb05c12ce34e5598341e0de0cc571b2d387) )
	ROM_LOAD( "fill_board_82s129.5p",  0x200, 0x100, CRC(c525b091) SHA1(bd172ed3d15f410d09017207ec68faf1d1cb4512) )
	ROM_LOAD( "fill_board_82s129.6p",  0x300, 0x100, CRC(7ed6c192) SHA1(b02bbcd0e04d3a6c54bec755df55fe6f97d9513f) )
	ROM_LOAD( "fill_board_82s129.7p",  0x400, 0x100, CRC(6b572b73) SHA1(4a065cb05c12ce34e5598341e0de0cc571b2d387) )
	ROM_LOAD( "fill_board_82s129.9p",  0x200, 0x100, CRC(c525b091) SHA1(bd172ed3d15f410d09017207ec68faf1d1cb4512) )
	ROM_LOAD( "fill_board_82s129.10p", 0x300, 0x100, CRC(7ed6c192) SHA1(b02bbcd0e04d3a6c54bec755df55fe6f97d9513f) )
	ROM_LOAD( "fill_board_82s129.11p", 0x400, 0x100, CRC(6b572b73) SHA1(4a065cb05c12ce34e5598341e0de0cc571b2d387) )
	ROM_LOAD( "fill_board_82s129.12p", 0x200, 0x100, CRC(c525b091) SHA1(bd172ed3d15f410d09017207ec68faf1d1cb4512) )
	ROM_LOAD( "fill_board_82s129.13p", 0x300, 0x100, CRC(7ed6c192) SHA1(b02bbcd0e04d3a6c54bec755df55fe6f97d9513f) )
	ROM_LOAD( "fill_board_82s129.14p", 0x400, 0x100, CRC(6b572b73) SHA1(4a065cb05c12ce34e5598341e0de0cc571b2d387) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cubeqst", 0, SHA1(d0e24bb5d0ae424e0816110ec7d6b21189044d57) )
	ROM_REGION( 0x800, REGION_CPU5, ROMREGION_ERASE00 )
	ROM_REGION( 0x400, REGION_CPU6, ROMREGION_ERASE00 )
ROM_END

GAME( 1983, cubeqst, 0, cubeqst, cubeqst, 0, ROT0, "Simutrek", "Cube Quest (01/04/84)", 0 )
