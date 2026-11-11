// license:BSD-3-Clause
// copyright-holders:hap, Mariusz Wojcieszek
/*
    Brazilian bootleg board from 1989. Forte II Games, Industria Brasileira.

    Hardware is based on MSX1, excluding i8255 PPI:
    - 64KB RAM, largely unused
    - 64KB EPROM (2764-15, contains hacked BIOS and game ROM)
    - Z80 @ 3.58MHz
    - GI AY-3-8910
    - TI TMS9928A
    - no dipswitches
*/

#include "driver.h"
#include "vidhrdw/tms9928a.h"
#include "sound/ay8910.h"

static UINT8 input_mask;

static READ8_HANDLER( input_r )
{
	return readinputport(0) | (input_mask & 0x7f);
}

static WRITE8_HANDLER( input_mask_w )
{
	/* Keep coin detection active while the program locks the player controls. */
	input_mask = data;
}

static ADDRESS_MAP_START( program_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) | AMEF_UNMAP(1) )
	AM_RANGE(0x98, 0x98) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x99, 0x99) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xa0, 0xa0) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xa1, 0xa1) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xa2, 0xa2) AM_READ(AY8910_read_port_0_r)
	/* The BIOS still accesses the PPI ports even though the board has no PPI. */
	AM_RANGE(0xa8, 0xab) AM_NOP
ADDRESS_MAP_END

INPUT_PORTS_START( pesadelo )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

static void vdp_irq(int state)
{
	/* Reading VDP status acknowledges the interrupt and clears this line. */
	cpunum_set_input_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const TMS9928a_interface vdp_config =
{
	TMS99x8A, 0x4000, vdp_irq
};

static const struct AY8910interface psg_config =
{
	input_r, 0, 0, input_mask_w
};

static INTERRUPT_GEN( video_interrupt )
{
	TMS9928A_interrupt();
}

static MACHINE_START( forte2 )
{
	state_save_register_global(input_mask);
	state_save_register_func_postload(TMS9928A_post_load);
	return 0;
}

static MACHINE_RESET( forte2 )
{
	input_mask = 0xff;
	TMS9928A_reset();
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static MACHINE_DRIVER_START( pesadelo )
	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_PROGRAM_MAP(program_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)
	MDRV_CPU_VBLANK_INT(video_interrupt, 1)
	MDRV_MACHINE_START(forte2)
	MDRV_MACHINE_RESET(forte2)

	MDRV_FRAMES_PER_SECOND(10738635.0 / 2 / 342 / 262)
	MDRV_VBLANK_DURATION(4458)
	MDRV_TMS9928A(&vdp_config)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 3579545 / 2)
	MDRV_SOUND_CONFIG(psg_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static DRIVER_INIT( pesadelo )
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *buf = auto_malloc(0x10000);
	int i;

	/* Decode from a separate buffer so permuted writes cannot overwrite unread bytes. */
	memcpy(buf, rom, 0x10000);
	for (i = 0; i < 0x10000; ++i)
		rom[BITSWAP16(i,11,9,8,13,14,15,12,7,6,5,4,3,2,1,0,10)] =
			BITSWAP8(buf[i],3,5,6,7,0,4,2,1);
}

ROM_START( pesadelo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "epr2764.15", 0x00000, 0x10000, CRC(1ae2f724) SHA1(12880dd7ad82acf04861843fb9d4f0f926d18f6b) )
ROM_END

GAME( 1989, pesadelo, 0, pesadelo, pesadelo, pesadelo, ROT0, "bootleg (Forte II Games)", "Pesadelo (bootleg of Konami Knightmare)", GAME_SUPPORTS_SAVE )
