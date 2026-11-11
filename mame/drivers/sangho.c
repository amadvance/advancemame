// license:BSD-3-Clause
// copyright-holders:David Haywood, Tomasz Slanina, Mariusz Wojcieszek
/*
    Sang Ho Soft 'Puzzle Star' PCB

    Each board contains a custom FGPA on a sub-board with
    a warning   "WARNING ! NO TOUCH..." printed on the PCB

    A battery is connected to the underside of the sub-board
    and if the battery dies the PCB is no-longer functional.

    It is possible that important game code is stored within
    the battery.

    The ROMs for "Puzzle Star" don't appear to have code at 0
    and all boards found so far have been dead.

    The Sexy Boom board was working, but it may only be a
    matter of time before that board dies too.

    It is thought that these games are based on MSX hardware
    as some of the Puzzle Star roms appear to be a hacked
    MSX BIOS.  If we're lucky then the FGPA may only contain
    Sang Ho's MSX simulation, rather than any specific game code.

    The FGPA is labeled 'Custom 3'

    There is another covered chip on the PCB labeled 'Custom 2'
    at U17.  It is unknown what this chip is.

    Custom 1 is underneath the sub-board and is a UM3567 which
    is a YM2413 compatible chip.

    *** the custom chip with the warning appears to control banking etc.

    TODO:
    - pzlestar title screen uses sprites with screen 12, has wrong colors
    - sexyboom slows down dramatically, presumably bankswitch related
*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/v9938.h"
#include "sound/2413intf.h"

#define RAM_SIZE 0x20000

static UINT8 ram[RAM_SIZE];
static UINT8 rom_dummy[0x4000];
static UINT8 rom_empty[0x4000];

static UINT8 pzlestar_mem_bank;
static UINT8 pzlestar_rom_bank;
static UINT8 sec_slot[4];
static UINT8 sexyboom_bank[8];

static void map_page(int page, UINT8 *read, UINT8 *write)
{
	memory_set_bankptr(page + 1, read);
	memory_set_bankptr(page + 5, write);
}

static void map_ram(int page, UINT8 *base)
{
	map_page(page, base, base);
}

/* read-only pages ignore the writes through a scratch buffer */
static void map_rom(int page, UINT8 *base)
{
	map_page(page, base, rom_dummy);
}

static void map_none(int page)
{
	map_page(page, rom_empty, rom_dummy);
}

/*
    slot 0 selects RAM
    slot 1 selects ?
    slot 2 selects code ROMs
    slot 3 selects data ROMs
*/
static void pzlestar_map_banks(void)
{
	UINT8 *user1 = memory_region(REGION_USER1);

	switch ((pzlestar_mem_bank >> 0) & 3) {
	case 0 : map_ram(0, ram); break;
	case 2 : map_rom(0, user1 + 0x10000); break;
	default : map_none(0); break;
	}

	switch ((pzlestar_mem_bank >> 2) & 3) {
	case 0 : map_ram(1, ram + 0x4000); break;
	case 2 : map_rom(1, user1 + 0x18000); break;
	case 3 : map_rom(1, user1 + 0x20000 + pzlestar_rom_bank * 0x8000 + 0x4000); break;
	default : map_none(1); break;
	}

	switch ((pzlestar_mem_bank >> 4) & 3) {
	case 0 : map_ram(2, ram + 0x8000); break;
	case 3 : map_rom(2, user1 + 0x20000 + pzlestar_rom_bank * 0x8000); break;
	default : map_none(2); break;
	}

	switch ((pzlestar_mem_bank >> 6) & 3) {
	case 0 : map_ram(3, ram + 0xc000); break;
	default : map_none(3); break;
	}
}

static WRITE8_HANDLER( pzlestar_bank_w )
{
	pzlestar_rom_bank = data;
	pzlestar_map_banks();
}

static WRITE8_HANDLER( pzlestar_mem_bank_w )
{
	pzlestar_mem_bank = data;
	pzlestar_map_banks();
}

static READ8_HANDLER( pzlestar_mem_bank_r )
{
	return pzlestar_mem_bank;
}

/* secondary slot register of the currently selected primary slot of page 3 */
static READ8_HANDLER( sec_slot_r )
{
	return sec_slot[pzlestar_mem_bank >> 6] ^ 0xff;
}

static WRITE8_HANDLER( sec_slot_w )
{
	sec_slot[pzlestar_mem_bank >> 6] = data;
}

static void sexyboom_map_bank(int page)
{
	UINT8 *user1 = memory_region(REGION_USER1);
	UINT8 banknum = sexyboom_bank[page * 2];
	UINT8 banktype = sexyboom_bank[page * 2 + 1];

	if (banktype == 0) {
		if (banknum & 0x80)
			map_ram(page, ram + (banknum & 0x7f) * 0x4000);
		else
			map_rom(page, user1 + 0x4000 * banknum);
	} else if (banktype == 0x82) {
		map_rom(page, user1 + 0x20000 + banknum * 0x4000);
	} else if (banktype == 0x80) {
		map_rom(page, user1 + 0x120000 + banknum * 0x4000);
	}
}

static WRITE8_HANDLER( sexyboom_bank_w )
{
	sexyboom_bank[offset] = data;
	sexyboom_map_bank(offset >> 1);
}

static ADDRESS_MAP_START( pzlestar_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK5)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK6)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK7)
	AM_RANGE(0xc000, 0xfffe) AM_READWRITE(MRA8_BANK4, MWA8_BANK8)
	AM_RANGE(0xffff, 0xffff) AM_READWRITE(sec_slot_r, sec_slot_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sexyboom_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READWRITE(MRA8_BANK1, MWA8_BANK5)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(MRA8_BANK2, MWA8_BANK6)
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(MRA8_BANK3, MWA8_BANK7)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(MRA8_BANK4, MWA8_BANK8)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pzlestar_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7c, 0x7c) AM_WRITE(YM2413_register_port_0_w)
	AM_RANGE(0x7d, 0x7d) AM_WRITE(YM2413_data_port_0_w)
	AM_RANGE(0x91, 0x91) AM_WRITE(pzlestar_bank_w)
	AM_RANGE(0x98, 0x98) AM_READWRITE(v9938_vram_r, v9938_vram_w)
	AM_RANGE(0x99, 0x99) AM_READWRITE(v9938_status_r, v9938_command_w)
	AM_RANGE(0x9a, 0x9a) AM_WRITE(v9938_palette_w)
	AM_RANGE(0x9b, 0x9b) AM_WRITE(v9938_register_w)
	AM_RANGE(0xa0, 0xa0) AM_READ(input_port_0_r)
	AM_RANGE(0xa1, 0xa1) AM_READ(input_port_1_r)
	AM_RANGE(0xa8, 0xa8) AM_READWRITE(pzlestar_mem_bank_r, pzlestar_mem_bank_w)
	AM_RANGE(0xf7, 0xf7) AM_READ(input_port_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sexyboom_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7c, 0x7c) AM_WRITE(YM2413_register_port_0_w)
	AM_RANGE(0x7d, 0x7d) AM_WRITE(YM2413_data_port_0_w)
	AM_RANGE(0xa0, 0xa0) AM_READ(input_port_0_r)
	AM_RANGE(0xa1, 0xa1) AM_READ(input_port_1_r)
	AM_RANGE(0xf0, 0xf0) AM_READWRITE(v9938_vram_r, v9938_vram_w)
	AM_RANGE(0xf1, 0xf1) AM_READWRITE(v9938_status_r, v9938_command_w)
	AM_RANGE(0xf2, 0xf2) AM_WRITE(v9938_palette_w)
	AM_RANGE(0xf3, 0xf3) AM_WRITE(v9938_register_w)
	AM_RANGE(0xf7, 0xf7) AM_READ(input_port_2_r)
	AM_RANGE(0xf8, 0xff) AM_WRITE(sexyboom_bank_w)
ADDRESS_MAP_END

INPUT_PORTS_START( sexyboom )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easiest ) )
	PORT_DIPSETTING(    0x05, "Easiest (duplicate)" )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, "Easy (duplicate)" )
	PORT_DIPSETTING(    0x07, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Harder ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, "Display Numbers on Tiles" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

INPUT_PORTS_START( pzlestar )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Display Numbers on Tiles" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

static void vdp_irq(int state)
{
	cpunum_set_input_line(0, 0, state ? HOLD_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN( vdp_interrupt )
{
	v9938_set_sprite_limit(0);
	v9938_set_resolution(RENDER_AUTO);
	v9938_interrupt();
}

static VIDEO_START( sangho )
{
	v9938_init(MODEL_V9958, 0x20000, vdp_irq);
	return 0;
}

static MACHINE_START( sangho )
{
	state_save_register_global_array(ram);
	state_save_register_global(pzlestar_mem_bank);
	state_save_register_global(pzlestar_rom_bank);
	state_save_register_global_array(sec_slot);
	state_save_register_global_array(sexyboom_bank);
	return 0;
}

static MACHINE_RESET( pzlestar )
{
	pzlestar_mem_bank = 2;
	pzlestar_rom_bank = 0;
	memset(sec_slot, 0, sizeof(sec_slot));
	pzlestar_map_banks();
	v9938_reset();
}

static MACHINE_RESET( sexyboom )
{
	static const UINT8 init[8] = { 0x00, 0x00, 0x01, 0x00, 0x80, 0x00, 0x80, 0x01 };
	int i;

	memcpy(sexyboom_bank, init, sizeof(init));
	for (i = 0; i < 4; ++i)
		sexyboom_map_bank(i);
	v9938_reset();
}

static MACHINE_DRIVER_START( pzlestar )
	MDRV_CPU_ADD(Z80, 21477272 / 6)
	MDRV_CPU_PROGRAM_MAP(pzlestar_map, 0)
	MDRV_CPU_IO_MAP(pzlestar_io_map, 0)
	MDRV_CPU_VBLANK_INT(vdp_interrupt, 262)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_START(sangho)
	MDRV_MACHINE_RESET(pzlestar)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512 + 32, (212 + 28) * 2)
	MDRV_VISIBLE_AREA(0, 512 + 32 - 1, 0, (212 + 28) * 2 - 1)
	MDRV_PALETTE_LENGTH(19780)
	MDRV_COLORTABLE_LENGTH(19780)
	MDRV_PALETTE_INIT(v9958)
	MDRV_VIDEO_START(sangho)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2413, 21477272 / 6)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sexyboom )
	MDRV_CPU_ADD(Z80, 21477272 / 6)
	MDRV_CPU_PROGRAM_MAP(sexyboom_map, 0)
	MDRV_CPU_IO_MAP(sexyboom_io_map, 0)
	MDRV_CPU_VBLANK_INT(vdp_interrupt, 262)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_START(sangho)
	MDRV_MACHINE_RESET(sexyboom)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512 + 32, (212 + 28) * 2)
	MDRV_VISIBLE_AREA(0, 512 + 32 - 1, 0, (212 + 28) * 2 - 1)
	MDRV_PALETTE_LENGTH(19780)
	MDRV_COLORTABLE_LENGTH(19780)
	MDRV_PALETTE_INIT(v9958)
	MDRV_VIDEO_START(sangho)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2413, 21477272 / 6)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( pzlestar )
	ROM_REGION( 0x20000*16, REGION_USER1, 0 )
	ROM_LOAD( "rom01.bin", 0x000000, 0x20000, CRC(0b327a3b) SHA1(450fd27f9844b9f0b710c1886985bd67cac2716f) )
	ROM_LOAD( "rom02.bin", 0x020000, 0x20000, CRC(dc859437) SHA1(e9fe5aab48d80e8857fc679ff7e35298ce4d47c8) )
	ROM_LOAD( "rom03.bin", 0x040000, 0x20000, CRC(f92b5624) SHA1(80be9000fc4326d790801d02959550aada111548) )
	ROM_LOAD( "rom04.bin", 0x060000, 0x20000, CRC(929e7491) SHA1(fb700d3e1d50fefa9b85ccd3702a9854df53a210) )
	ROM_LOAD( "rom05.bin", 0x080000, 0x20000, CRC(8c6f71e5) SHA1(3597b03fe61216256437c56c583d55c7d59b5525) )
	ROM_LOAD( "rom06.bin", 0x0a0000, 0x20000, CRC(84599227) SHA1(d47c6cdbf3b64f83627c768059148e31f8de1f36) )
	ROM_LOAD( "rom08.bin", 0x0c0000, 0x20000, CRC(18d2bfe2) SHA1(cb92ee51d061bc053e296fcba10708f69ba12a61) )
	ROM_LOAD( "rom07.bin", 0x0e0000, 0x20000, CRC(6f64cc35) SHA1(3e3270834ad31e8240748c2b61f9b8f138d22f68) )
	ROM_LOAD( "rom09.bin", 0x100000, 0x20000, CRC(19a31115) SHA1(fa6ead5c8bf6be21d07797f74fcba13f0d041937) )
	ROM_LOAD( "rom10.bin", 0x120000, 0x20000, CRC(c003328b) SHA1(5172e2c48e118ac9f9b9dd4f4df8804245047b33) )
	ROM_LOAD( "rom11.bin", 0x140000, 0x20000, CRC(d36c1f92) SHA1(42b412c1ab99cb14f2e15bd80fede34c0df414b9) )
	ROM_LOAD( "rom12.bin", 0x160000, 0x20000, CRC(baa82727) SHA1(ed3dd1befa615003204f903472ef1af1eb702c38) )
	ROM_LOAD( "rom13.bin", 0x180000, 0x20000, CRC(8b4b6a2c) SHA1(4b9c188260617ccce94cbf6cccb45aab455af09b) )
ROM_END

ROM_START( sprpuzzle )
	ROM_REGION( 0x20000*16, REGION_USER1, 0 )
	ROM_LOAD( "01.rom1",  0x000000, 0x20000, CRC(c494d996) SHA1(755a8e49845f3168130a83a4c7363d3ee9da359b) )
	ROM_LOAD( "02.rom2",  0x020000, 0x20000, CRC(7ee2974e) SHA1(36a91b690f1228cbf66b5b3733823056632e9ef3) )
	ROM_LOAD( "03.rom3",  0x040000, 0x20000, CRC(340edac9) SHA1(47ffc4553cb34ff932d3d54d5cefe82571c6ddbf) )
	ROM_LOAD( "04.rom4",  0x060000, 0x20000, CRC(25f76d7f) SHA1(caff03ba4ca9ad44e488618141c4e1f43a0cdc48) )
	ROM_LOAD( "05.rom5",  0x080000, 0x20000, CRC(3a3dda85) SHA1(b174cf87be5dd7a7430ff29c70c8093c577f4033) )
	ROM_LOAD( "06.rom6",  0x0a0000, 0x20000, CRC(d0428a82) SHA1(4409c2ebd2f70828286769c9367cbccac483cdaf) )
	ROM_LOAD( "07.rom7",  0x0c0000, 0x20000, CRC(2d2e4df2) SHA1(8ec36c8c021c2b9d9be7b61e09e31a7a18a06dad) )
	ROM_LOAD( "08.rom8",  0x0e0000, 0x20000, CRC(283ba870) SHA1(98f35d95caf58595f002d57a4bafc39b6d67ed1a) )
	ROM_LOAD( "09.rom9",  0x100000, 0x20000, CRC(a78310f4) SHA1(7a14cabd371d6ba4e335f0e00135de3dd8a4e642) )
	ROM_LOAD( "10.rom10", 0x120000, 0x20000, CRC(b20fabd2) SHA1(a6a3bac1ac19e1ecd2fd0aeb17fbf16ffa07df13) )
	ROM_LOAD( "11.rom11", 0x140000, 0x20000, CRC(e4aa16bc) SHA1(c5889b813ceb7a1c0deb8a9d4d006932b266a482) )
	ROM_LOAD( "12.rom12", 0x160000, 0x20000, CRC(cd8b6b5d) SHA1(ffddc7781e13146e198ad12a9c89504f270857d8) )
ROM_END

ROM_START( sexyboom )
	ROM_REGION( 0x20000*16, REGION_USER1, 0 )
	ROM_LOAD( "rom1.bin",  0x000000, 0x20000, CRC(7827a079) SHA1(a48e7c7d16e50de24c8bf77883230075c1fad858) )
	ROM_LOAD( "rom2.bin",  0x020000, 0x20000, CRC(7028aa61) SHA1(77d5e5945b90d3e15ba2c1364b76f6643247592d) )
	ROM_LOAD( "rom3.bin",  0x040000, 0x20000, CRC(340edac9) SHA1(47ffc4553cb34ff932d3d54d5cefe82571c6ddbf) )
	ROM_LOAD( "rom4.bin",  0x060000, 0x20000, CRC(25f76d7f) SHA1(caff03ba4ca9ad44e488618141c4e1f43a0cdc48) )
	ROM_LOAD( "rom5.bin",  0x080000, 0x20000, CRC(3a3dda85) SHA1(b174cf87be5dd7a7430ff29c70c8093c577f4033) )
	ROM_LOAD( "rom6.bin",  0x0a0000, 0x20000, CRC(d0428a82) SHA1(4409c2ebd2f70828286769c9367cbccac483cdaf) )
	ROM_LOAD( "rom7.bin",  0x0c0000, 0x20000, CRC(2d2e4df2) SHA1(8ec36c8c021c2b9d9be7b61e09e31a7a18a06dad) )
	ROM_LOAD( "rom8.bin",  0x0e0000, 0x20000, CRC(283ba870) SHA1(98f35d95caf58595f002d57a4bafc39b6d67ed1a) )
	ROM_LOAD( "rom9.bin",  0x100000, 0x20000, CRC(a78310f4) SHA1(7a14cabd371d6ba4e335f0e00135de3dd8a4e642) )
	ROM_LOAD( "rom10.bin", 0x120000, 0x20000, CRC(b20fabd2) SHA1(a6a3bac1ac19e1ecd2fd0aeb17fbf16ffa07df13) )
	ROM_LOAD( "rom11.bin", 0x140000, 0x20000, CRC(e4aa16bc) SHA1(c5889b813ceb7a1c0deb8a9d4d006932b266a482) )
	ROM_LOAD( "rom12.bin", 0x160000, 0x20000, CRC(cd8b6b5d) SHA1(ffddc7781e13146e198ad12a9c89504f270857d8) )
ROM_END


static DRIVER_INIT( pzlestar )
{
	UINT8 *rom = memory_region(REGION_USER1);

	/* patch looping check */
	rom[0x12ca7] = 0x00;
	rom[0x12ca8] = 0x00;
}

GAME( 1991, pzlestar,  0,        pzlestar, pzlestar, pzlestar, ROT270, "Sang Ho Soft", "Puzzle Star (Sang Ho Soft)", GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
GAME( 1992, sexyboom,  0,        sexyboom, sexyboom, 0,        ROT270, "Sang Ho Soft", "Sexy Boom", 0 )
GAME( 1991, sprpuzzle, sexyboom, sexyboom, sexyboom, 0,        ROT270, "Sang Ho Soft", "Super Puzzle (Sang Ho Soft)", 0 )
