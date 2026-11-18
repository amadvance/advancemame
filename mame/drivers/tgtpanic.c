// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/*******************************************************************************

Konami Target Panic (cabinet test PCB)

Hardware notes:
- PCB label: KONAMI PWB402613
- Z84C0008PEC Z80 CPU, 4.0MHz ceramic resonator
- simple bitmap video, no sound

*******************************************************************************/

#include "driver.h"

static UINT8 *tgtpanic_ram;
static UINT8 tgtpanic_color;

static WRITE8_HANDLER( tgtpanic_color_w )
{
	tgtpanic_color = data;
}

static MACHINE_START( tgtpanic )
{
	state_save_register_global(tgtpanic_color);
	return 0;
}

static MACHINE_RESET( tgtpanic )
{
	tgtpanic_color = 0;
}

static PALETTE_INIT( tgtpanic )
{
	int i;

	for (i = 0; i < 8; i++)
		palette_set_color(i, pal1bit(i >> 2), pal1bit(i >> 1), pal1bit(i));
}

static VIDEO_UPDATE( tgtpanic )
{
	pen_t colors[4];
	int x, y;

	colors[0] = Machine->pens[0];
	colors[1] = Machine->pens[7];
	colors[2] = Machine->pens[tgtpanic_color & 7];
	colors[3] = Machine->pens[(tgtpanic_color >> 4) & 7];

	/* Columns of four pixels occupy consecutive 128-byte blocks. */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		const UINT8 *row = tgtpanic_ram + (y >> 1);
		UINT16 *dest = bitmap->line[y];

		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			dest[x] = colors[(row[(x >> 2) << 7] >> ((x & 3) << 1)) & 3];
	}
}

static ADDRESS_MAP_START( tgtpanic_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAM AM_BASE(&tgtpanic_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tgtpanic_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(input_port_0_r, tgtpanic_color_w)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
ADDRESS_MAP_END

INPUT_PORTS_START( tgtpanic )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static MACHINE_DRIVER_START( tgtpanic )
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(tgtpanic_map, 0)
	MDRV_CPU_IO_MAP(tgtpanic_io, 0)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(300))

	MDRV_MACHINE_START(tgtpanic)
	MDRV_MACHINE_RESET(tgtpanic)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 191, 0, 191)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(tgtpanic)
	MDRV_VIDEO_UPDATE(tgtpanic)
MACHINE_DRIVER_END

ROM_START( tgtpanic )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "601_ja_a01.13e", 0x0000, 0x8000, CRC(ece71952) SHA1(0f9cbd8adac2b1950bc608d51f0f122399c8f00f) )
ROM_END

GAME( 1996, tgtpanic, 0, tgtpanic, tgtpanic, 0, ROT0, "Konami", "Target Panic", GAME_SUPPORTS_SAVE )
