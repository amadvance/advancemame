// license:BSD-3-Clause
// copyright-holders:Luca Elia, David Haywood

/* Y-BOX arcade hardware with a Mega Drive-compatible VDP and an M6295. */

#include "driver.h"
#include "genesis.h"

#define MASTER_CLOCK		53693175

static unsigned int prot_count;
static UINT16 prot_value;
static UINT8 sound_bus_requested;
static UINT8 sound_in_reset;

INPUT_PORTS_START( puckpkmn )
	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(10)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("UNK")

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x28, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x38, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x28, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )

	PORT_START_TAG("DSW2")
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( jzth )
	PORT_INCLUDE( puckpkmn )

	PORT_MODIFY("P2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_MODIFY("P1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_MODIFY("UNK")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( songjang )
	PORT_INCLUDE( jzth )

	PORT_MODIFY("UNK")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

static READ16_HANDLER( ybox_YM3438_r )
{
	return	YM3438_status_port_0_A_r(0) << 8;
}

static WRITE16_HANDLER( ybox_YM3438_w )
{
	switch (offset)
	{
		case 0:
			if (ACCESSING_MSB)	YM3438_control_port_0_A_w	(0,	(data >> 8) & 0xff);
			else 				YM3438_data_port_0_A_w		(0,	(data >> 0) & 0xff);
			break;
		case 1:
			if (ACCESSING_MSB)	YM3438_control_port_0_B_w	(0,	(data >> 8) & 0xff);
			else 				YM3438_data_port_0_B_w		(0,	(data >> 0) & 0xff);
			break;
	}
}

static READ16_HANDLER( puckpkmn_prot_r )
{
	return 0x000e;
}

static READ16_HANDLER( puckpkmn_id_r )
{
	return 0x3100;
}

static READ16_HANDLER( jingling_id_r )
{
	return 0x3400;
}

/* Each read ends the current protection command sequence. */
static READ16_HANDLER( jzth_prot_r )
{
	unsigned int count = prot_count;

	prot_count = 0;
	switch (count)
	{
		case 4: return 0x0004;
		case 5: return 0x0005;
		case 6: return 0x000e;
		default: return 0x000f;
	}
}

static WRITE16_HANDLER( jzth_prot_w )
{
	++prot_count;
}

static READ16_HANDLER( ybox_prot_latch_r )
{
	return prot_value;
}

static WRITE16_HANDLER( ybox_prot_latch_w )
{
	/* The program writes the high byte at one address and reads it back at another. */
	COMBINE_DATA(&prot_value);
	prot_value &= 0xff00;
}

static READ16_HANDLER( songjang_id_r )
{
	return 0x0002;
}

static READ16_HANDLER( shuifeng_id_r )
{
	return 0x0031;
}

static READ16_HANDLER( ybox_vdp_r )
{
	UINT16 data = genesis_vdp_r(offset, mem_mask);

	/* The legacy beam position is scaled to the active width, so expose HBLANK
	   in the last fifth of each line for the palette writes synchronized to it. */
	if ((offset == 2 || offset == 3) && cpu_gethorzbeampos() >= (Machine->drv->screen_width * 4) / 5)
		data |= 0x0004;
	return data;
}

static READ16_HANDLER( ybox_sound_bus_r )
{
	return (sound_bus_requested && !sound_in_reset) ? 0 : 0x0100;
}

static WRITE16_HANDLER( ybox_sound_bus_w )
{
	if (ACCESSING_MSB)
	{
		sound_bus_requested = (data & 0x0100) != 0;
		cpunum_set_input_line(1, INPUT_LINE_HALT, sound_bus_requested ? ASSERT_LINE : CLEAR_LINE);
	}
}

static WRITE16_HANDLER( ybox_sound_reset_w )
{
	if (ACCESSING_MSB)
	{
		sound_in_reset = (data & 0x0100) == 0;
		cpunum_set_input_line(1, INPUT_LINE_RESET, sound_in_reset ? ASSERT_LINE : CLEAR_LINE);
	}
}

static WRITE16_HANDLER( ybox_sound_ram_w )
{
	if (ACCESSING_MSB)
		genesis_z80_ram[offset * 2] = data >> 8;
	else
		genesis_z80_ram[offset * 2 + 1] = data;
}

static ADDRESS_MAP_START( ybox_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM AM_BASE(&genesis_z80_ram)
	AM_RANGE(0x4000, 0x7fff) AM_READWRITE(genesis_z80_r, genesis_z80_w)
	AM_RANGE(0x8000, 0xffff) AM_READ(genesis_z80_bank_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ybox_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_ROM
	AM_RANGE(0x700010, 0x700011) AM_READ(input_port_0_word_r)
	AM_RANGE(0x700012, 0x700013) AM_READ(input_port_1_word_r)
	AM_RANGE(0x700014, 0x700015) AM_READ(input_port_2_word_r)
	AM_RANGE(0x700016, 0x700017) AM_READ(input_port_3_word_r)
	AM_RANGE(0x700018, 0x700019) AM_READ(input_port_4_word_r)
	AM_RANGE(0x700022, 0x700023) AM_READWRITE(OKIM6295_status_0_lsb_r, OKIM6295_data_0_lsb_w)
	AM_RANGE(0xa00000, 0xa00551) AM_WRITE(MWA16_RAM)
	AM_RANGE(0xa04000, 0xa04003) AM_READWRITE(ybox_YM3438_r, ybox_YM3438_w)
	AM_RANGE(0xa10002, 0xa10005) AM_READWRITE(MRA16_NOP, MWA16_NOP)
	AM_RANGE(0xa11100, 0xa11101) AM_READWRITE(MRA16_NOP, MWA16_NOP)
	AM_RANGE(0xa11200, 0xa11201) AM_WRITE(MWA16_NOP)
	AM_RANGE(0xc00000, 0xc0001f) AM_MIRROR(0x100000) AM_READWRITE(ybox_vdp_r, genesis_vdp_w)
	AM_RANGE(0xe00000, 0xe0ffff) AM_MIRROR(0x1f0000) AM_RAM
ADDRESS_MAP_END

static MACHINE_RESET( ybox )
{
	machine_reset_genesis();
	if (Machine->drv->cpu[1].cpu_type == CPU_Z80)
	{
		sound_bus_requested = 0;
		sound_in_reset = 1;
		cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
		cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
	}
	prot_count = 0;
	prot_value = 0;
}

static struct YM3438interface ym3438_intf =
{
	genesis_irq2_interrupt		/* IRQ handler */
};

static MACHINE_DRIVER_START( ybox )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000, MASTER_CLOCK/7)
	MDRV_CPU_PROGRAM_MAP(ybox_map,0)
	MDRV_CPU_VBLANK_INT(genesis_vblank_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_MACHINE_START(genesis)
	MDRV_MACHINE_RESET(ybox)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(320,224)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(genesis)
	MDRV_VIDEO_UPDATE(genesis)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3438, MASTER_CLOCK/7)
	MDRV_SOUND_CONFIG(ym3438_intf)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)

	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ybox_z80 )
	MDRV_IMPORT_FROM(ybox)
	MDRV_CPU_ADD_TAG("sound", Z80, MASTER_CLOCK/15)
	MDRV_CPU_PROGRAM_MAP(ybox_sound_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	MDRV_INTERLEAVE(100)
MACHINE_DRIVER_END

static DRIVER_INIT( ybox )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	size_t len = memory_region_length(REGION_CPU1);
	size_t i;

	for (i = 0; i < len; i++)
		rom[i] = BITSWAP8(rom[i],1,4,2,0,7,5,3,6);
}

static DRIVER_INIT( puckpkmn )
{
	init_ybox();
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4b2476, 0x4b2477, 0, 0, puckpkmn_id_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x70001c, 0x70001d, 0, 0, puckpkmn_prot_r);
}

static DRIVER_INIT( jingling )
{
	init_ybox();
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x4b2476, 0x4b2477, 0, 0, jingling_id_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x70001c, 0x70001d, 0, 0, puckpkmn_prot_r);
}

static void ybox_install_sound(void)
{
	/* The game uploads a sound program and submits commands through Z80 RAM. */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa00000, 0xa01fff, 0, 0, genesis_68k_to_z80_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa00000, 0xa01fff, 0, 0x2000, ybox_sound_ram_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa11100, 0xa11101, 0, 0, ybox_sound_bus_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa11100, 0xa11101, 0, 0, ybox_sound_bus_w);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xa11200, 0xa11201, 0, 0, ybox_sound_reset_w);
	state_save_register_global(sound_bus_requested);
	state_save_register_global(sound_in_reset);
}

static DRIVER_INIT( jzth )
{
	init_ybox();
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x710000, 0x710001, 0, 0, jzth_prot_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x710000, 0x710001, 0, 0, jzth_prot_w);
	state_save_register_global(prot_count);
	ybox_install_sound();
}

static void install_protection_latch(read16_handler id_r)
{
	init_ybox();
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x4fffff, 0, 0, ybox_prot_latch_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x4fffff, 0, 0, ybox_prot_latch_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x70001c, 0x70001d, 0, 0, id_r);
	state_save_register_global(prot_value);
}

static DRIVER_INIT( songjang )
{
	install_protection_latch(songjang_id_r);
	ybox_install_sound();
}

static DRIVER_INIT( shuifeng )
{
	install_protection_latch(shuifeng_id_r);
	ybox_install_sound();
}

ROM_START( puckpkmn )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "200061.u5", 0x000000, 0x080000, CRC(502a5093) SHA1(6dc1c79d52ebb653cb2e4388f74fd975ec323566) )
	ROM_LOAD16_BYTE( "200060.u4", 0x000001, 0x080000, CRC(5f160c18) SHA1(5a5ce1b9a81afe836e435e9d6f16cf57b63cbd31) )
	ROM_LOAD16_BYTE( "200063.u8", 0x100000, 0x080000, CRC(0c29781e) SHA1(db442f9b588608b2ac04d65fd830103296599a6a) )
	ROM_LOAD16_BYTE( "200062.u7", 0x100001, 0x080000, CRC(00bbf9a9) SHA1(924c1ed85090c497ce89528082c15d1548a854a0) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "206295.u3", 0x00000, 0x40000, CRC(7b066bac) SHA1(429616e21c672b07e0705bc63234249cac3af56f) )
ROM_END

ROM_START( jingling )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "b2.u59", 0x000000, 0x080000, CRC(3fbea2c7) SHA1(89f3770ae92c62714f0795ddd2f311a9532eb25a) )
	ROM_IGNORE(0x080000)
	ROM_LOAD16_BYTE( "b1.u59", 0x000001, 0x080000, CRC(dc7b4254) SHA1(8ba5c5e8123e62e9af091971d0d0401d2df49350) )
	ROM_IGNORE(0x080000)
	ROM_LOAD16_BYTE( "b4.u59", 0x100000, 0x080000, CRC(375c9f80) SHA1(9b0eb729e95c22355e4117eec596f90e10282492) )
	ROM_IGNORE(0x080000)
	ROM_LOAD16_BYTE( "b3.u59", 0x100001, 0x080000, CRC(d5487df6) SHA1(d1d3d717e184a4e8e067665bbbe94e7cf45db478) )
	ROM_IGNORE(0x080000)

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "a.u3", 0x00000, 0x80000, CRC(77891c9b) SHA1(66f28b418a480a89ddb3fae3a7c2fe702c62364c) )
ROM_END

ROM_START( puckpkmnb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "puckpoke.u5", 0x000000, 0x080000, CRC(fd334b91) SHA1(cf8bf6645a4082ea4392937e169b1686c9c7e246) )
	ROM_LOAD16_BYTE( "puckpoke.u4", 0x000001, 0x080000, CRC(839cc76b) SHA1(e15662a7175db7a8e222dda176a8ed92e0d56e9d) )
	ROM_LOAD16_BYTE( "puckpoke.u8", 0x100000, 0x080000, CRC(7936bec8) SHA1(4b350105abe514fbfeabae1c6f3aeee695c3d07a) )
	ROM_LOAD16_BYTE( "puckpoke.u7", 0x100001, 0x080000, CRC(96b66bdf) SHA1(3cc2861ad9bc232cbe683e01b58090f832d03db5) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "puckpoke.u3", 0x00000, 0x40000, CRC(7b066bac) SHA1(429616e21c672b07e0705bc63234249cac3af56f) )
ROM_END

ROM_START( jzth )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "s.y.u5", 0x000000, 0x080000, CRC(a4a526b5) SHA1(85d0299caf91ff50b6870f845b9aacbd358ed81f) )
	ROM_LOAD16_BYTE( "s.y.u4", 0x000001, 0x080000, CRC(c16654eb) SHA1(dca4b772a3b9caa7be3fa01511c401b591c2e6f3) )
	ROM_LOAD16_BYTE( "s.y.u8", 0x100000, 0x080000, CRC(b62e1068) SHA1(2484ae49a4a2a2c551b3b84bbc0b4e40e5d281e7) )
	ROM_LOAD16_BYTE( "s.y.u7", 0x100001, 0x080000, CRC(27fe424c) SHA1(14bee8c16aac3d5b04123c994167531f817634fd) )
	ROM_LOAD16_BYTE( "s.y.u64", 0x200000, 0x080000, CRC(62f52886) SHA1(07fc9765274c03eff4a09f48a0b1b2b2afc6078e) )
	ROM_LOAD16_BYTE( "s.y.u63", 0x200001, 0x080000, CRC(a6a32c8c) SHA1(d0c779751e4af459e9bf63e55c5e2b19a243b70d) )
	ROM_LOAD16_BYTE( "s.y.u66", 0x300000, 0x080000, CRC(fa4a09f5) SHA1(67d77c91a994ecb8b29e7661c3a12e84a64eb837))
	ROM_LOAD16_BYTE( "s.y.u65", 0x300001, 0x080000, CRC(de64e526) SHA1(e3b3e5c95b8ae36c0c57f8c9a6f55084464c4c05) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "s.y.u3", 0x00000, 0x40000, CRC(38eef2f2) SHA1(2f750dbf71fea0622e8493f0a8be7c43555ed5cf) )
	ROM_CONTINUE(0x40000,0x40000)
ROM_END

ROM_START( songjang )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rom.u5", 0x000000, 0x080000, CRC(3de4aea5) SHA1(a4fd0799a6fd86ba3f0c4b4277190f4c332e48d6) )
	ROM_LOAD16_BYTE( "rom.u4", 0x000001, 0x080000, CRC(dc6cfc29) SHA1(e15d6e2187befbbd791c5046cb6d156801095203) )
	ROM_LOAD16_BYTE( "rom.u8", 0x100000, 0x080000, CRC(d4d2b262) SHA1(2023128b187a8cd1a3f021861da559b5ad48e984) )
	ROM_LOAD16_BYTE( "rom.u7", 0x100001, 0x080000, CRC(1e24f2c9) SHA1(562249b08a8fe317a606c16ce07a8009a0c5418a) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom.u3", 0x00000, 0x80000, CRC(44287ab4) SHA1(42de2010e73c036a2a0e01c9dd3c1c8aef1a54a1) )
ROM_END

ROM_START( shuifeng )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "shuihufengyunzhuan.u5", 0x000000, 0x080000, CRC(73208a1a) SHA1(1eff253fe48f9e553a238c4cfd41cc5e5839393f) )
	ROM_LOAD16_BYTE( "shuihufengyunzhuan.u4", 0x000001, 0x080000, CRC(e633c02c) SHA1(7ff1091a5b2e7d5e0e616509854fa0058d3cdcf7) )
	ROM_LOAD16_BYTE( "shuihufengyunzhuan.u8", 0x100000, 0x080000, CRC(31814eb0) SHA1(3a6b13931d02a0c3b8bd97dc51ac5771a97bd092) )
	ROM_LOAD16_BYTE( "shuihufengyunzhuan.u7", 0x100001, 0x080000, CRC(51caef9f) SHA1(252dbb29ae878aaf28cb4586e02d823296eaf378) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "shuihufengyunzhuan-oki.u3", 0x00000, 0x80000, CRC(44287ab4) SHA1(42de2010e73c036a2a0e01c9dd3c1c8aef1a54a1) )
ROM_END

GAME( 2000, puckpkmn,  0, ybox,      puckpkmn, puckpkmn, ROT0, "Sun Mixing",  "Puckman Pockimon Genie 2000",           0 )
GAME( 2000, jingling,  puckpkmn, ybox,      puckpkmn, jingling, ROT0, "IBS Co. Ltd", "Jingling Jiazu Genie 2000",             0 )
GAME( 2000, puckpkmnb, puckpkmn, ybox,      puckpkmn, ybox,     ROT0, "bootleg",     "Puckman Pockimon Genie 2000 (bootleg)", 0 )
GAME( 2000, jzth,      0, ybox_z80,  jzth,     jzth,     ROT0, "<unknown>",   "Juezhan Tianhuang",                     GAME_IMPERFECT_SOUND )
GAME( 200?, songjang,  0, ybox_z80,  songjang, songjang, ROT0, "WAH LAP",     "Songjiangyanyi Final",                  GAME_IMPERFECT_GRAPHICS )
GAME( 1999, shuifeng,  0, ybox_z80,  jzth,     shuifeng, ROT0, "WAH LAP",     "Shuihu Feng Yun Zhuan",                 GAME_IMPERFECT_GRAPHICS )
