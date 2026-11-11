// license:BSD-3-Clause
// copyright-holders:Mariusz Wojcieszek, hap
/*
    Penguin Adventure bootleg (tagged 'Screen', 1988)
    Original release was on MSX, by Konami in 1986. There is no official arcade release of this game.

    This seems to be the MSX version hacked to run on cheap Korean(?) bootleg hardware.
    Bosses are at wrong stages when compared to the original, probably to make the game more
    difficult early on. This is also the cause of some gfx glitches when reaching a boss.

    Basic components include.....
    Z80 @ 3.579533MHz [10.7386/3]
    TMS9128 @ 10.7386MHz
    AY-3-8910 @ 1.789766MHz [10.7386/6]
    8255
    4416 RAM x2
    4164 RAM x8
    10.7386 XTAL
    10 position DIPSW
    NOTE! switches 1, 3 & 5 must be ON or the game will not boot.

    TODO:
    - pengadvb: add dipswitch
    - pengadvb: A timer apparently expires when beating stage 4 (signalled by a long beeping sound).
      Player needs to insert another credit and press start button (?) in order to continue.
*/

#include "driver.h"
#include "vidhrdw/tms9928a.h"
#include "sound/ay8910.h"

static UINT8 primary_slot_reg;
static UINT8 ppi_port_c;
static UINT8 megarom_bank[4];
static UINT8 ram[0x4000];
static UINT8 empty[0x2000];

static void update_banks(void)
{
	UINT8 *bios = memory_region(REGION_CPU1);
	UINT8 *game = memory_region(REGION_USER1);
	int blk;

	/* each 8KB block is mapped according to the slot selected for its 16KB page */
	for (blk = 0; blk < 8; ++blk) {
		int slot = (primary_slot_reg >> ((blk >> 1) * 2)) & 3;
		UINT8 *ptr = empty;

		if (slot == 0 && blk < 4)
			ptr = bios + blk * 0x2000;
		else if (slot == 1 && blk >= 2 && blk < 6)
			ptr = game + megarom_bank[blk - 2] * 0x2000;
		else if (slot == 3 && blk >= 6)
			ptr = ram + (blk - 6) * 0x2000;

		memory_set_bankptr(blk + 1, ptr);
	}
}

static WRITE8_HANDLER( megarom_w )
{
	/* the writes are decoded by the mapper only when the megarom slot is selected */
	int blk = offset >> 13;
	if (((primary_slot_reg >> (((offset + 0x4000) >> 14) * 2)) & 3) == 1) {
		megarom_bank[blk] = data & 0xf;
		update_banks();
	}
}

static WRITE8_HANDLER( ram_w )
{
	if ((primary_slot_reg >> 6) == 3)
		ram[offset] = data;
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
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x4000, 0xbfff) AM_WRITE(megarom_w)
	AM_RANGE(0xc000, 0xffff) AM_WRITE(ram_w)
ADDRESS_MAP_END

static READ8_HANDLER( ppi_r )
{
	switch (offset) {
	case 0 :
		return primary_slot_reg;
	case 1 :
		switch (ppi_port_c & 0x0f) {
		case 0x00 : return readinputport(1);
		case 0x01 : return readinputport(2);
		case 0x04 : return readinputport(3);
		}
		return 0xff;
	case 2 :
		return ppi_port_c;
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
		ppi_port_c = data;
		break;
	case 3 :
		if (data & 0x80) {
			/* mode set clears the output latches */
			ppi_port_c = 0;
			primary_slot_reg = 0;
			update_banks();
		} else {
			int bit = (data >> 1) & 7;
			if (data & 1)
				ppi_port_c |= 1 << bit;
			else
				ppi_port_c &= ~(1 << bit);
		}
		break;
	}
}

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x98, 0x98) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x99, 0x99) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xa0, 0xa0) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xa1, 0xa1) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xa2, 0xa2) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0xa8, 0xab) AM_READWRITE(ppi_r, ppi_w)
ADDRESS_MAP_END

INPUT_PORTS_START( pengadvb )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xee, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( pengadvb2 )
	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xa4, 0xa4, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x84, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xa4, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x24, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static void vdp_irq(int state)
{
	cpunum_set_input_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const TMS9928a_interface vdp_config =
{
	TMS9929A, 0x4000, vdp_irq
};

static const struct AY8910interface psg_config =
{
	input_port_0_r, 0, 0, 0
};

static INTERRUPT_GEN( video_interrupt )
{
	TMS9928A_interrupt();
}

static MACHINE_START( pengadvb )
{
	state_save_register_global(primary_slot_reg);
	state_save_register_global(ppi_port_c);
	state_save_register_global_array(megarom_bank);
	state_save_register_global_array(ram);
	state_save_register_func_postload(update_banks);
	state_save_register_func_postload(TMS9928A_post_load);
	return 0;
}

static MACHINE_RESET( pengadvb )
{
	int i;
	primary_slot_reg = 0;
	ppi_port_c = 0;
	for (i = 0; i < 4; ++i)
		megarom_bank[i] = i;
	update_banks();
	TMS9928A_reset();
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static MACHINE_DRIVER_START( pengadvb )
	MDRV_CPU_ADD(Z80, 10738635 / 3)
	MDRV_CPU_PROGRAM_MAP(program_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)
	MDRV_CPU_VBLANK_INT(video_interrupt, 1)
	MDRV_MACHINE_START(pengadvb)
	MDRV_MACHINE_RESET(pengadvb)

	MDRV_FRAMES_PER_SECOND(10738635.0 / 2 / 342 / 313)
	MDRV_VBLANK_DURATION(4458)
	MDRV_TMS9928A(&vdp_config)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 10738635 / 6)
	MDRV_SOUND_CONFIG(psg_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static void decrypt(int region)
{
	UINT8 *mem = memory_region(region);
	UINT32 size = memory_region_length(region);
	UINT8 *buf = auto_malloc(size);
	UINT32 i;

	/* data lines swap */
	for (i = 0; i < size; ++i)
		mem[i] = BITSWAP8(mem[i],7,6,5,3,4,2,1,0);

	/* address line swap, from a copy to not overwrite unread bytes */
	memcpy(buf, mem, size);
	for (i = 0; i < size; ++i)
		mem[i] = buf[BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,5,11,10,9,8,7,6,12,4,3,2,1,0)];
}

static DRIVER_INIT( pengadvb )
{
	decrypt(REGION_CPU1);
	decrypt(REGION_USER1);
}

ROM_START( pengadvb )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "rom.u5", 0x00000, 0x8000, CRC(d21950d2) SHA1(0b1815677f17a680ba63c3839bea2d451813eec8) )

	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "rom.u7",  0x00000, 0x8000, CRC(d4b4a4a4) SHA1(59f9299182fd8aedc7a4e9b0ddd685f2a71c033f) )
	ROM_LOAD( "rom.u8",  0x08000, 0x8000, CRC(eada2232) SHA1(f4182f0921b621acd8be6077eb9639b31a97e907) )
	ROM_LOAD( "rom.u9",  0x10000, 0x8000, CRC(6478c561) SHA1(6f9a794a5bb51e96552f6d1e9fa6515659d25933) )
	ROM_LOAD( "rom.u10", 0x18000, 0x8000, CRC(5c48360f) SHA1(0866e20969f57b7b7c59df8f7ca203f18c7c9870) )
ROM_END

ROM_START( pengadvb2 )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "7l",  0x00000, 0x8000, CRC(9645ab69) SHA1(7a15b501d2c357b9fda83c811b0d728df318ceb2) )

	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "7a",  0x00000, 0x8000, CRC(8434344c) SHA1(c3002df12fb5395506a16abcefbcb4f5cbe3eb6a) )
	ROM_LOAD( "7c",  0x08000, 0x8000, CRC(0274f6eb) SHA1(185b3819357abf65988971e9deece3b5c67dd1d0) )
	ROM_LOAD( "7d",  0x10000, 0x8000, CRC(8cb1f223) SHA1(ff5db3c373e6d919b4e8e06c3e4607b150f31964) )
	ROM_LOAD( "7e",  0x18000, 0x8000, CRC(60764899) SHA1(a75e59c2ecf8cebdb99708cb18390157ad7b6993) )
ROM_END

GAME( 1988, pengadvb,  0,        pengadvb, pengadvb,  pengadvb, ROT0, "bootleg (Screen)", "Penguin Adventure (bootleg of MSX version, encrypted)", GAME_SUPPORTS_SAVE )
GAME( 1988, pengadvb2, pengadvb, pengadvb, pengadvb2, 0,        ROT0, "bootleg (Comet)", "Penguin Adventure (bootleg of MSX version, not encrypted)", GAME_SUPPORTS_SAVE )
