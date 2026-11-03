// license:BSD-3-Clause
// copyright-holders:R. Belmont, Sergey Svishchev
/*
    Nippel Super Games ][
    An arcade board designed to run modified games from Apple ][.
*/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "sound/dac.h"

static UINT8 *superga2_ram;
static UINT8 page2;
static UINT8 speaker_state;
static UINT8 artifact_map[32];

static MACHINE_START( superga2 )
{
	state_save_register_global(page2);
	state_save_register_global(speaker_state);
	return 0;
}

static MACHINE_RESET( superga2 )
{
	/* The board starts directly in the RAM image; no Apple firmware is fitted. */
	memset(superga2_ram, 0, 0xc000);
	memcpy(superga2_ram + 0x1100, memory_region(REGION_CPU1), 0x8000);
	page2 = 0;
	speaker_state = 0;
	DAC_data_w(0, 0);
}

static WRITE8_HANDLER( superga2_speaker_w )
{
	speaker_state ^= 1;
	DAC_data_w(0, speaker_state ? 255 : 0);
}

static READ8_HANDLER( superga2_speaker_r )
{
	superga2_speaker_w(offset, 0);
	return 0xff;
}

static WRITE8_HANDLER( superga2_switch_w )
{
	if ((offset & 0x0e) == 4 && page2 != (offset & 1))
	{
		/* Finish the visible scanlines before changing the display page. */
		force_partial_update(cpu_getscanline());
		page2 = offset & 1;
	}
}

static READ8_HANDLER( superga2_switch_r )
{
	superga2_switch_w(offset, 0);
	return 0xff;
}

static READ8_HANDLER( superga2_vector_r )
{
	return (offset & 1) ? 0x11 : 0x00;
}

static ADDRESS_MAP_START( superga2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_RAM AM_BASE(&superga2_ram)
	AM_RANGE(0xc000, 0xc00f) AM_READ(input_port_0_r) AM_WRITENOP
	AM_RANGE(0xc030, 0xc03f) AM_READWRITE(superga2_speaker_r, superga2_speaker_w)
	AM_RANGE(0xc050, 0xc05f) AM_READWRITE(superga2_switch_r, superga2_switch_w)
	AM_RANGE(0xfffc, 0xffff) AM_READ(superga2_vector_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( superga2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
INPUT_PORTS_END

static PALETTE_INIT( superga2 )
{
	static const UINT8 colors[8][3] = {
		{0, 0, 0}, {0xff, 0x44, 0xfd}, {0x14, 0xf5, 0x3c}, {255, 255, 255},
		{0, 0, 0}, {0x14, 0xcf, 0xfd}, {0xff, 0x6a, 0x3c}, {255, 255, 255}
	};
	int i, j;
	for (i = 0; i < 8; ++i)
		palette_set_color(i, colors[i][0], colors[i][1], colors[i][2]);
	for (i = 0; i < 8; ++i)
		for (j = 0; j < 2; ++j)
		{
			int c;
			if (i & 2)
				c = (i & 5) ? 3 : (j ? 2 : 1);
			else
				c = ((i & 5) == 5) ? (j ? 1 : 2) : 0;
			artifact_map[j * 8 + i] = c;
			artifact_map[16 + j * 8 + i] = c + 4;
		}
}

static VIDEO_UPDATE( superga2 )
{
	int y, col, b;
	const UINT8 *vram = superga2_ram + (page2 ? 0x4000 : 0x2000);
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT8 row[42];
		int address = ((y & 7) << 10) | (((y >> 3) & 7) << 7) | ((y >> 6) * 40);
		row[0] = row[41] = 0;
		memcpy(row + 1, vram + address, 40);
		for (col = 0; col < 40; ++col)
		{
			UINT32 bits = (row[col] & 0x7f) | ((row[col + 1] & 0x7f) << 7) | ((row[col + 2] & 0x7f) << 14);
			const UINT8 *map = artifact_map + ((row[col + 1] & 0x80) ? 16 : 0);
			for (b = 0; b < 7; ++b)
			{
				int x = col * 14 + b * 2;
				int color = map[((bits >> (b + 6)) & 7) | (((b ^ col) & 1) << 3)];
				if (x >= cliprect->min_x && x <= cliprect->max_x)
					plot_pixel(bitmap, x, y, Machine->pens[color]);
				if (x + 1 >= cliprect->min_x && x + 1 <= cliprect->max_x)
					plot_pixel(bitmap, x + 1, y, Machine->pens[color]);
			}
		}
	}
}

static MACHINE_DRIVER_START( superga2 )
	MDRV_CPU_ADD(M6502, 1021800)
	MDRV_CPU_PROGRAM_MAP(superga2_map, 0)
	MDRV_FRAMES_PER_SECOND(1021800.0 / (65 * 262))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 70 * 65 / 1021800))
	MDRV_MACHINE_START(superga2)
	MDRV_MACHINE_RESET(superga2)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(560, 262)
	MDRV_VISIBLE_AREA(0, 559, 0, 191)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(superga2)
	MDRV_VIDEO_UPDATE(superga2)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( kuzmich )
	ROM_REGION(0x8000, REGION_CPU1, 0)
	ROM_LOAD("ke.bin", 0x0000, 0x8000, CRC(102d246b) SHA1(492dcdf0cc31190a97057a69010e2c9c23b6e59d))
ROM_END

GAME( 199?, kuzmich, 0, superga2, superga2, 0, ROT0, "Nippel", "Kuzmich-Egorych", GAME_IMPERFECT_COLORS | GAME_SUPPORTS_SAVE )
