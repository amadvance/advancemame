// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
/*
    Super Free Kick / Spinkick by HEC (Haesung Enterprise Co.)

    Hacked MSX2 home computer hardware. Romset contains
    modified ( (c) strings removed and patched boot sequence)
    MSX2 bios. Yamaha VDP v9938 is hidden in huge epoxy block.
    There's also an additional Z80 to drive sound.

    Board # CBK1029

    Z8400A (x2)
    UM82C55A-PC
    YM2203C
    21.47727 MHz XTAL

    DIPSW-1
    ------------------------------------------------------------------
        DipSwitch Title   | Function | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
    ------------------------------------------------------------------
        Cabinet Style     | Upright  |off|                           |
                          | Cocktail |on |                           |
    ------------------------------------------------------------------
        Stage Select      |   Off    |   |off|                       |
                          |   On     |   |on |                       |
    ------------------------------------------------------------------
        Freeze Screen     |   Off    |       |off|                   |
                          |   On     |       |on |                   |
    ------------------------------------------------------------------
       Test / Game Mode   |   Game   |           |off|               |
                          |   Test   |           |on |               |
    ------------------------------------------------------------------
        Allow Continue    |   Off    |               |off|           |
                          |   On     |               |on |           |
    ------------------------------------------------------------------
                          | 1cn/1cr  |                   |off|off|off|
                          | 1cn/2cr  |                   |on |off|off|
                          | 1cn/3cr  |                   |off|on |off|
            Coinage       | 1cn/5cr  |                   |on |on |off|
                          | 2cn/1cr  |                   |off|off|on |
                          | 2cn/3cr  |                   |on |off|on |
                          | 3cn/1cr  |                   |off|on |on |
                          | 3cn/2cr  |                   |on |on |on |
    ------------------------------------------------------------------

    DIPSW-2
    ------------------------------------------------------------------
        DipSwitch Title   | Function | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
    ------------------------------------------------------------------
          No Comment      |   N/A    |off|                           |
    ------------------------------------------------------------------
         Demo Sounds      |   Yes    |   |off|                       |
                          |   No     |   |on |                       |
    ------------------------------------------------------------------
                          |    1     |       |off|off|               |
        Players Count     |    2     |       |on |off|               |
                          |    3     |       |off|on |               |
                          |    5     |       |on |on |               |
    -----------------------------------------------------------------
                          |   None   |               |off|off|       |
             Bonus        |Every 20K |               |on |off|       |
                          |20K & 50K |               |off|on |       |
                          |Every 50K |               |on |on |       |
    ------------------------------------------------------------------
                          |   Easy   |                       |off|off|
          Difficulty      |  Normal  |                       |on |off|
                          |   Hard   |                       |off|on |
                          |  V.Hard  |                       |on |on |
    ------------------------------------------------------------------
*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/v9938.h"
#include "sound/2203intf.h"

#define MASTER_CLOCK 21477272

static UINT8 primary_slot_reg;
static UINT8 input_mux;
static UINT8 rom_bank[8];
static UINT8 ram[0x4000];
static UINT8 empty[0x2000];

static void update_banks(void)
{
	UINT8 *bios = memory_region(REGION_USER4);
	UINT8 *cartridge = memory_region(REGION_USER3);
	UINT8 *extrom = memory_region(REGION_USER2);
	UINT8 *banked = memory_region(REGION_USER1);
	int blk;

	/* each 8KB block is mapped according to the slot selected for its 16KB page */
	for (blk = 0; blk < 8; ++blk) {
		int slot = (primary_slot_reg >> ((blk >> 1) * 2)) & 3;
		UINT8 *ptr = empty;

		if (slot == 0 && blk < 4)
			ptr = bios + blk * 0x2000;
		else if (slot == 0 && blk < 6)
			ptr = cartridge + 0x4000 + (blk - 4) * 0x2000;
		else if (slot == 1 && blk < 2)
			ptr = extrom + 0x4000 + blk * 0x2000;
		else if (slot == 2)
			ptr = banked + rom_bank[blk] * 0x2000;
		else if (slot == 3 && blk >= 6)
			ptr = ram + (blk - 6) * 0x2000;

		memory_set_bankptr(blk + 1, ptr);
	}
}

static WRITE8_HANDLER( mem_w )
{
	int slot = (primary_slot_reg >> ((offset >> 14) * 2)) & 3;

	/* the mapper decodes the writes only when its slot is selected, the ROM slots ignore them */
	if (slot == 2) {
		rom_bank[offset >> 13] = data & 0xf;
		update_banks();
	} else if (slot == 3 && offset >= 0xc000) {
		ram[offset - 0xc000] = data;
	}
}

static READ8_HANDLER( ppi_r )
{
	switch (offset) {
	case 0 :
		return primary_slot_reg;
	case 1 :
		switch (input_mux & 0x0f) {
		case 0 : return readinputport(0);
		case 1 : return readinputport(1);
		case 2 : return BITSWAP8(readinputport(2), 4,5,6,7,3,2,1,0);
		case 3 : return readinputport(3);
		case 4 : return readinputport(4);
		}
		return 0xff;
	case 2 :
		return input_mux;
	}
	return 0xff;
}

static WRITE8_HANDLER( ppi_w )
{
	switch (offset) {
	case 0 :
		if (data != primary_slot_reg) {
			primary_slot_reg = data;
			update_banks();
		}
		break;
	case 2 :
		input_mux = data;
		break;
	case 3 :
		if (data & 0x80) {
			/* mode set clears the output latches */
			input_mux = 0;
			primary_slot_reg = 0;
			update_banks();
		} else {
			int bit = (data >> 1) & 7;
			if (data & 1)
				input_mux |= 1 << bit;
			else
				input_mux &= ~(1 << bit);
		}
		break;
	}
}

static ADDRESS_MAP_START( program_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_BANK2)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_BANK3)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_BANK4)
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK5)
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_BANK6)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_BANK7)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_BANK8)
	AM_RANGE(0x0000, 0xffff) AM_WRITE(mem_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x98, 0x98) AM_READWRITE(v9938_vram_r, v9938_vram_w)
	AM_RANGE(0x99, 0x99) AM_READWRITE(v9938_status_r, v9938_command_w)
	AM_RANGE(0x9a, 0x9a) AM_WRITE(v9938_palette_w)
	AM_RANGE(0x9b, 0x9b) AM_WRITE(v9938_register_w)
	AM_RANGE(0xa0, 0xa7) AM_WRITE(soundlatch_w)
	AM_RANGE(0xa8, 0xab) AM_READWRITE(ppi_r, ppi_w)
	AM_RANGE(0xb4, 0xb5) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch_r)
	AM_RANGE(0x04, 0x04) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
ADDRESS_MAP_END

INPUT_PORTS_START( sfkick )
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x71, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_DIAL ) PORT_SENSITIVITY(25) PORT_KEYDELTA(20) PORT_REVERSE

	PORT_START /* bitswapped at read: 76543210 -> 45673210 */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Cabinet ) ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x10, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x01, 0x01, "Stage Select" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Freeze" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Test Mode" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Allow_Continue ) ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8c, 0x8c, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW1:6,8,7")
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x84, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x8c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x88, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )

	PORT_START /* bitswapped at read: 76543210 -> 45673210 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x41, 0x01, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW2:3,4")
	PORT_DIPSETTING(    0x41, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x82, 0x02, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SW2:5,6")
	PORT_DIPSETTING(    0x80, "Every 20,000" )
	PORT_DIPSETTING(    0x02, "20,000 & 50,000" )
	PORT_DIPSETTING(    0x00, "Every 50,000" )
	PORT_DIPSETTING(    0x82, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SW2:7,8")
	PORT_DIPSETTING(    0x0c, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
INPUT_PORTS_END

static void irqhandler(int irq)
{
	cpunum_set_input_line_and_vector(1, 0, irq ? ASSERT_LINE : CLEAR_LINE, 0xff);
}

static struct YM2203interface ym2203_interface =
{
	0, 0, 0, 0, irqhandler
};

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

static VIDEO_START( sfkick )
{
	v9938_init(MODEL_V9938, 0x80000, vdp_irq);
	return 0;
}

static MACHINE_START( sfkick )
{
	state_save_register_global(primary_slot_reg);
	state_save_register_global(input_mux);
	state_save_register_global_array(rom_bank);
	state_save_register_global_array(ram);
	state_save_register_func_postload(update_banks);
	return 0;
}

static MACHINE_RESET( sfkick )
{
	int i;

	primary_slot_reg = 0;
	input_mux = 0;
	for (i = 0; i < 8; ++i)
		rom_bank[i] = i;
	memset(empty, 0xff, sizeof(empty));
	update_banks();
	v9938_reset();
}

static MACHINE_DRIVER_START( sfkick )
	MDRV_CPU_ADD(Z80, MASTER_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(program_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)
	MDRV_CPU_VBLANK_INT(vdp_interrupt, 262)

	MDRV_CPU_ADD(Z80, MASTER_CLOCK / 6)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_io_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_START(sfkick)
	MDRV_MACHINE_RESET(sfkick)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512 + 32, (212 + 28) * 2)
	MDRV_VISIBLE_AREA(0, 512 + 32 - 1, 0, (212 + 28) * 2 - 1)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)
	MDRV_PALETTE_INIT(v9938)
	MDRV_VIDEO_START(sfkick)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(YM2203, MASTER_CLOCK / 6)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.25)
	MDRV_SOUND_ROUTE(1, "mono", 0.25)
	MDRV_SOUND_ROUTE(2, "mono", 0.25)
	MDRV_SOUND_ROUTE(3, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( sfkick )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sfkick1.c5", 0x00000, 0x8000, CRC(2f5e3b7a) SHA1(d2ff566b415ab10c0681fa1eb221a56e3c137ecf) )

	ROM_REGION( 0x20000, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "sfkick2.a7", 0x00000, 0x8000, CRC(1dcaec5e) SHA1(7e063d46fb6606df2d772866cc55f207035b98c4) )
	ROM_LOAD( "sfkick3.c7", 0x08000, 0x8000, CRC(639d3cf2) SHA1(950fd28058d32e4532eb6e99454dcaef092a955e) )
	ROM_LOAD( "sfkick4.d7", 0x10000, 0x8000, CRC(ee1b344e) SHA1(d33fbad017cc4838192e9c540621537edb7e8dc4) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )
	ROM_LOAD( "sfkick5.h7", 0x00000, 0x8000, CRC(8e8bd9cf) SHA1(f493de40147fdd67a48d4c90b01170fbd6ea074e) )

	ROM_REGION( 0x8000, REGION_USER3, 0 )
	ROM_LOAD( "sfkick6.j7", 0x0000, 0x8000, CRC(7a91ac4b) SHA1(afc5e2c2fe0cd108235ac6ae2775cc9a0b1c9f76) )

	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "sfkick7.l7", 0x00000, 0x8000, CRC(8cd94c63) SHA1(e6dba66c8716593b8ab88f79f7205211938d1598) )
ROM_END

ROM_START( sfkicka )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "c130.bin", 0x00000, 0x8000, CRC(2f5e3b7a) SHA1(d2ff566b415ab10c0681fa1eb221a56e3c137ecf) )

	ROM_REGION( 0x20000, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "c145.bin", 0x00000, 0x8000, CRC(1dcaec5e) SHA1(7e063d46fb6606df2d772866cc55f207035b98c4) )
	ROM_LOAD( "c146.bin", 0x08000, 0x8000, CRC(57afc4c6) SHA1(ee28b3f74e3175c22f542855b09f1673d048b1fa) )
	ROM_LOAD( "c147.bin", 0x10000, 0x8000, CRC(ee1b344e) SHA1(d33fbad017cc4838192e9c540621537edb7e8dc4) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )
	ROM_LOAD( "c149.bin", 0x00000, 0x8000, CRC(2edbf61f) SHA1(23dcff43faf222a4b69001312ce4b1c920e2f4c2) )

	ROM_REGION( 0x8000, REGION_USER3, 0 )
	ROM_LOAD( "c150.bin", 0x0000, 0x8000, CRC(20412918) SHA1(b0fefa957b20373ffb84d9ff97a2e84a9a3af56c) )

	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "c151.bin", 0x00000, 0x8000, CRC(8cd94c63) SHA1(e6dba66c8716593b8ab88f79f7205211938d1598) )
ROM_END

ROM_START( spinkick )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "spinkick.r1", 0x00000, 0x8000, CRC(2f5e3b7a) SHA1(d2ff566b415ab10c0681fa1eb221a56e3c137ecf) )

	ROM_REGION( 0x20000, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "spinkick.r2", 0x00000, 0x8000, CRC(1dcaec5e) SHA1(7e063d46fb6606df2d772866cc55f207035b98c4) )
	ROM_LOAD( "spinkick.r3", 0x08000, 0x8000, CRC(e86a194a) SHA1(19a02375ec463e795770403c3e948d754919458b) )
	ROM_LOAD( "spinkick.r4", 0x10000, 0x8000, CRC(ee1b344e) SHA1(d33fbad017cc4838192e9c540621537edb7e8dc4) )

	ROM_REGION( 0x8000, REGION_USER2, 0 )
	ROM_LOAD( "spinkick.r5", 0x00000, 0x8000, CRC(8e8bd9cf) SHA1(f493de40147fdd67a48d4c90b01170fbd6ea074e) )

	ROM_REGION( 0x8000, REGION_USER3, 0 )
	ROM_LOAD( "spinkick.r6", 0x0000, 0x8000, CRC(7a91ac4b) SHA1(afc5e2c2fe0cd108235ac6ae2775cc9a0b1c9f76) )

	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "spinkick.r7", 0x00000, 0x8000, CRC(8cd94c63) SHA1(e6dba66c8716593b8ab88f79f7205211938d1598) )
ROM_END

GAME( 1988, sfkick,   0,      sfkick, sfkick, 0, ROT90, "Haesung/HJ Corp", "Super Free Kick (set 1)", GAME_SUPPORTS_SAVE )
GAME( 198?, sfkicka,  sfkick, sfkick, sfkick, 0, ROT90, "Haesung",         "Super Free Kick (set 2)", GAME_SUPPORTS_SAVE )
GAME( 1988, spinkick, sfkick, sfkick, sfkick, 0, ROT90, "Haesung/Seojin",  "Hec's Spinkick",          GAME_SUPPORTS_SAVE )
