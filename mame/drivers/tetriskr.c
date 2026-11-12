// license:BSD-3-Clause
// copyright-holders:Angelo Salese, David Haywood, Tomasz Slanina
/******************************************************************************************

Korean Tetris Arcade game
(blantant rip-off of the Mirrorsoft/Andromeda Software Tetris PC-XT)

original tetriunk.c by David Haywood & Tomasz Slanina

TODO:
- tetriskr can store inputs read during the timer irq. If ds is 0x40 when the irq is taken
  it will corrupt the BIOS data area which can lead to corrupt graphics
- buzzer sound;

******************************************************************************************/

#include "driver.h"
#include "cpu/i86/i88intf.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"
#include "machine/pcshare.h"

static UINT8 *cga_vram;
static UINT8 cga_mode;
static UINT8 cga_color;
static UINT8 cga_index;
static UINT32 cga_frame;
static UINT8 cga_regs[0x20];
static UINT8 bg_bank;
static UINT8 port_b_data;

static const UINT8 cga_palette[16][3] =
{
	{ 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0xaa }, { 0x00, 0xaa, 0x00 }, { 0x00, 0xaa, 0xaa },
	{ 0xaa, 0x00, 0x00 }, { 0xaa, 0x00, 0xaa }, { 0xaa, 0x55, 0x00 }, { 0xaa, 0xaa, 0xaa },
	{ 0x55, 0x55, 0x55 }, { 0x55, 0x55, 0xff }, { 0x55, 0xff, 0x55 }, { 0x55, 0xff, 0xff },
	{ 0xff, 0x55, 0x55 }, { 0xff, 0x55, 0xff }, { 0xff, 0xff, 0x55 }, { 0xff, 0xff, 0xff }
};

static PALETTE_INIT( tetriskr )
{
	int i;

	for (i = 0; i < 16; ++i)
		palette_set_color(i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2]);
}

static READ8_HANDLER( cga_status_r )
{
	UINT8 res = 0;

	if (cpu_getvblank())
		res |= 0x08;
	if (cpu_getvblank() || cpu_gethorzbeampos() >= 640)
		res |= 0x01;

	return res;
}

static WRITE8_HANDLER( bg_bank_w )
{
	bg_bank = (data & 0x0f) ^ 8;
}

static WRITE8_HANDLER( cga_mode_w )
{
	if (offset == 0)
		cga_mode = data;
	else
		cga_color = data;
}

static READ8_HANDLER( cga_regs_r )
{
	if (offset == 1 && cga_index < 0x20)
		return cga_regs[cga_index];

	return 0xff;
}

static WRITE8_HANDLER( cga_regs_w )
{
	if (offset == 0)
		cga_index = data;
	else if (cga_index < 0x20)
		cga_regs[cga_index] = data;
}

static void cga_draw_text(mame_bitmap *bitmap)
{
	const UINT8 *font = memory_region(REGION_USER1) + 0x1800;
	int columns = (cga_mode & 1) ? 80 : 40;
	int blink = (cga_mode & 0x20) && ((cga_frame & 0x10) != 0);
	int start = (((cga_regs[12] << 8) | cga_regs[13]) * 2) & 0x3fff;
	int x, y, r, c;

	for (y = 0; y < 25; ++y)
	{
		for (x = 0; x < columns; ++x)
		{
			int offs = (start + (y * columns + x) * 2) & 0x3fff;
			int code = cga_vram[offs];
			int attr = cga_vram[offs + 1];
			int fg = attr & 0x0f;
			int bg = (attr >> 4) & 0x0f;

			if (cga_mode & 0x20)
			{
				bg &= 7;
				if ((attr & 0x80) && blink)
					fg = bg;
			}

			for (r = 0; r < 8; ++r)
			{
				int bits = font[code * 8 + r];

				for (c = 0; c < 8; ++c)
				{
					int color = (bits & (0x80 >> c)) ? fg : bg;
					int pen = Machine->pens[color];

					if (!color)
						continue;

					if (columns == 80)
					{
						plot_pixel(bitmap, x * 8 + c, y * 8 + r, pen);
					}
					else
					{
						plot_pixel(bitmap, x * 16 + c * 2, y * 8 + r, pen);
						plot_pixel(bitmap, x * 16 + c * 2 + 1, y * 8 + r, pen);
					}
				}
			}
		}
	}
}

static void cga_draw_graphics(mame_bitmap *bitmap)
{
	int x, y;

	if (cga_mode & 0x10)
	{
		/* 640x200 monochrome */
		int fg = cga_color & 0x0f;

		for (y = 0; y < 200; ++y)
		{
			const UINT8 *line = cga_vram + (y & 1) * 0x2000 + (y >> 1) * 80;

			for (x = 0; x < 640; ++x)
				if (line[x >> 3] & (0x80 >> (x & 7)))
					plot_pixel(bitmap, x, y, Machine->pens[fg]);
		}
	}
	else
	{
		/* 320x200 four colors */
		int pal[4];
		int intensity = (cga_color & 0x10) ? 8 : 0;

		pal[0] = cga_color & 0x0f;
		if (cga_mode & 0x04)
		{
			pal[1] = 3 + intensity;
			pal[2] = 4 + intensity;
			pal[3] = 7 + intensity;
		}
		else if (cga_color & 0x20)
		{
			pal[1] = 3 + intensity;
			pal[2] = 5 + intensity;
			pal[3] = 7 + intensity;
		}
		else
		{
			pal[1] = 2 + intensity;
			pal[2] = 4 + intensity;
			pal[3] = 6 + intensity;
		}

		for (y = 0; y < 200; ++y)
		{
			const UINT8 *line = cga_vram + (y & 1) * 0x2000 + (y >> 1) * 80;

			for (x = 0; x < 320; ++x)
			{
				int color = (line[x >> 2] >> ((~x & 3) * 2)) & 3;
				int pen = Machine->pens[pal[color]];

				if (!color)
					continue;

				plot_pixel(bitmap, x * 2, y, pen);
				plot_pixel(bitmap, x * 2 + 1, y, pen);
			}
		}
	}
}

static void cga_draw_background(mame_bitmap *bitmap)
{
	const UINT8 *bg = memory_region(REGION_USER2);
	int x, y, p;

	for (y = 0; y < 200; ++y)
	{
		for (x = 0; x < 320; ++x)
		{
			int offs = (y / 8) * 40 + (x / 8) + (y % 8) * 0x400 + bg_bank * 0x2000 + 1;
			int color = 0;
			int pen;

			for (p = 0; p < 4; ++p)
				color |= ((bg[offs + p * 0x20000] >> (7 - (x % 8))) & 1) << p;

			pen = Machine->pens[color];
			plot_pixel(bitmap, x * 2, y, pen);
			plot_pixel(bitmap, x * 2 + 1, y, pen);
		}
	}
}

static VIDEO_UPDATE( tetriskr )
{
	++cga_frame;
	cga_draw_background(bitmap);

	if (cga_mode & 0x08)
	{
		if (cga_mode & 0x02)
			cga_draw_graphics(bitmap);
		else
			cga_draw_text(bitmap);
	}
}

static READ8_HANDLER( port_a_r )
{
	/* a harmless keyboard error occurs without this */
	return 0xaa;
}

static READ8_HANDLER( port_b_r )
{
	return port_b_data;
}

static READ8_HANDLER( port_c_r )
{
	return 0x00;
}

static WRITE8_HANDLER( port_b_w )
{
	port_b_data = data;
	pit8253_0_gate_w(2, data & 1);
}

static WRITE8_HANDLER( cga_vram_w )
{
	cga_vram[offset] = data;
}

static READ8_HANDLER( cga_vram_r )
{
	return cga_vram[offset];
}

static ADDRESS_MAP_START( tetriskr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0xb8000, 0xbbfff) AM_READWRITE(cga_vram_r, cga_vram_w) AM_BASE(&cga_vram)
	AM_RANGE(0xbc000, 0xbffff) AM_READWRITE(cga_vram_r, cga_vram_w)
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tetriskr_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x000f) AM_READWRITE(dma8237_0_r, dma8237_0_w)
	AM_RANGE(0x0020, 0x0021) AM_READWRITE(pic8259_0_r, pic8259_0_w)
	AM_RANGE(0x0040, 0x0043) AM_READWRITE(pit8253_0_r, pit8253_0_w)
	AM_RANGE(0x0060, 0x0060) AM_READ(port_a_r)
	AM_RANGE(0x0061, 0x0061) AM_READWRITE(port_b_r, port_b_w)
	AM_RANGE(0x0062, 0x0062) AM_READ(port_c_r)
	AM_RANGE(0x0080, 0x0083) AM_READWRITE(pc_page_r, pc_page_w)
	AM_RANGE(0x03b4, 0x03b5) AM_READWRITE(cga_regs_r, cga_regs_w)
	AM_RANGE(0x03c0, 0x03c0) AM_WRITE(bg_bank_w)
	AM_RANGE(0x03c8, 0x03c8) AM_READ(input_port_0_r)
	AM_RANGE(0x03c9, 0x03c9) AM_READ(input_port_1_r)
	AM_RANGE(0x03d4, 0x03d5) AM_READWRITE(cga_regs_r, cga_regs_w)
	AM_RANGE(0x03d8, 0x03d9) AM_WRITE(cga_mode_w)
	AM_RANGE(0x03da, 0x03da) AM_READ(cga_status_r)
ADDRESS_MAP_END

INPUT_PORTS_START( tetriskr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Starting Level" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Starting Bomb" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
INPUT_PORTS_END

static int tetriskr_irq_callback(int irqline)
{
	return pic8259_acknowledge(0);
}

static MACHINE_RESET( tetriskr )
{
	port_b_data = 0;
	bg_bank = 0;
	cga_mode = 0;
	cga_color = 0;
	memset(cga_regs, 0, sizeof(cga_regs));

	dma8237_reset();
	cpunum_set_irq_callback(0, tetriskr_irq_callback);
}

static DRIVER_INIT( tetriskr )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
}

static MACHINE_DRIVER_START( tetriskr )
	MDRV_CPU_ADD(I88, 14318181/3)
	MDRV_CPU_PROGRAM_MAP(tetriskr_map, 0)
	MDRV_CPU_IO_MAP(tetriskr_io, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(tetriskr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_VISIBLE_AREA(0, 639, 0, 199)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(tetriskr)
	MDRV_VIDEO_UPDATE(tetriskr)
MACHINE_DRIVER_END

ROM_START( tetriskr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "b-10.u10", 0xf0000, 0x10000, CRC(efc2a0f6) SHA1(5f0f1e90237bee9b78184035a32055b059a91eb3) )
	/* patch to work around the input bug mentioned above */
	ROM_FILL( 0xf1bdb, 1, 0xba )
	ROM_FILL( 0xf1bdc, 1, 0x00 )
	ROM_FILL( 0xf1bdd, 1, 0x01 )
	ROM_FILL( 0xf1bde, 1, 0x8e )
	ROM_FILL( 0xf1bdf, 1, 0xda )

	ROM_REGION( 0x2000, REGION_USER1, ROMREGION_ERASE00 ) /* CGA character generator */
	ROM_LOAD( "b-3.u36", 0x1800, 0x0800, CRC(1a636f9a) SHA1(a356cc57914d0c9b9127670b55d1f340e64b1ac9) )

	ROM_REGION( 0x80001, REGION_USER2, ROMREGION_INVERT | ROMREGION_ERASEFF )
	ROM_LOAD( "b-1.u59", 0x00000, 0x10000, CRC(4719d986) SHA1(6e0499944b968d96fbbfa3ead6237d69c769d634) )
	ROM_LOAD( "b-2.u58", 0x10000, 0x10000, CRC(599e1154) SHA1(14d99f90b4fedeab0ac24ffa9b1fd9ad0f0ba699) )
	ROM_LOAD( "b-4.u54", 0x20000, 0x10000, CRC(e112c450) SHA1(dfdecfc6bd617ec520b7563b7caf44b79d498bd3) )
	ROM_LOAD( "b-5.u53", 0x30000, 0x10000, CRC(050b7650) SHA1(5981dda4ed43b6e81fbe48bfba90a8775d5ecddf) )
	ROM_LOAD( "b-6.u49", 0x40000, 0x10000, CRC(d596ceb0) SHA1(8c82fb638688971ef11159a6b240253e63f0949d) )
	ROM_LOAD( "b-7.u48", 0x50000, 0x10000, CRC(79336b6c) SHA1(7a95875f3071bdc3ee25c0e6a5a3c00ef02dc977) )
	ROM_LOAD( "b-8.u44", 0x60000, 0x10000, CRC(1f82121a) SHA1(106da0f39f1260d0761217ed0a24c1611bfd7f05) )
	ROM_LOAD( "b-9.u43", 0x70000, 0x10000, CRC(4ea22349) SHA1(14dfd3dbd51f8bd6f3290293b8ea1c165e8cf7fd) )
ROM_END

GAME( 1988?, tetriskr, 0, tetriskr, tetriskr, tetriskr, ROT0, "bootleg", "Tetris (Korean bootleg of Mirrorsoft PC-XT Tetris)", GAME_IMPERFECT_SOUND )
