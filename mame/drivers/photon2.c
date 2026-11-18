// license:BSD-3-Clause
// copyright-holders:Mariusz Wojcieszek
/*
    Photon IK-3 system

    Russian arcade system based on ZX Spectrum home computer clone "Leningrad-1".
*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/dac.h"

#define PHOTON2_WIDTH 352
#define PHOTON2_HEIGHT 296
#define PHOTON2_BORDER 48

static UINT8 *photon2_vram;
static UINT8 photon2_border;
static UINT8 photon2_flash_frame;
static UINT8 photon2_nmi_enabled;
static mame_timer *photon2_scanline_timer;

static WRITE8_HANDLER( photon2_bank_w )
{
	int bank = 0;

	switch (data)
	{
		case 0: break;
		case 1: bank = 1; break;
		case 5: bank = 2; break;
		default: logerror("Photon IK-3: unknown bank value %02x\n", data); break;
	}

	memory_set_bank(1, bank);
}

static WRITE8_HANDLER( photon2_fe_w )
{
	photon2_border = data & 7;
	DAC_0_data_w(0, (data & 0x10) ? 0xff : 0);
}

static WRITE8_HANDLER( photon2_ppi_c_w )
{
	photon2_nmi_enabled = !(data & 0x20);
}

static READ8_HANDLER( photon2_io_r )
{
	if ((offset & 0x81) == 0x01)
		return ppi8255_0_r((offset >> 5) & 3);
	return 0xff;
}

static WRITE8_HANDLER( photon2_io_w )
{
	switch (offset & 0x81)
	{
		case 0x00: photon2_bank_w(0, data); break;
		case 0x01: ppi8255_0_w((offset >> 5) & 3, data); break;
		case 0x80: photon2_fe_w(0, data); break;
	}
}

static ADDRESS_MAP_START( photon2_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x4000, 0x5aff) AM_RAM AM_BASE(&photon2_vram)
	AM_RANGE(0x5b00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( photon2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0xff) AM_READWRITE(photon2_io_r, photon2_io_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( photon2 )
	PORT_START_TAG("JOY")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_BUTTON1)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START_TAG("COIN")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_COIN1) PORT_IMPULSE(1)
	PORT_DIPNAME(0x0e, 0x0e, "Time per Coin")
	PORT_DIPSETTING(0x00, "1:00")
	PORT_DIPSETTING(0x02, "1:30")
	PORT_DIPSETTING(0x04, "2:00")
	PORT_DIPSETTING(0x06, "2:30")
	PORT_DIPSETTING(0x08, "3:00")
	PORT_DIPSETTING(0x0a, "4:00")
	PORT_DIPSETTING(0x0c, "5:00")
	PORT_DIPSETTING(0x0e, "6:00")
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

static INPUT_PORTS_START( black )
	PORT_INCLUDE(photon2)

	PORT_MODIFY("JOY")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_8WAY
INPUT_PORTS_END

static const ppi8255_interface photon2_ppi =
{
	1,
	{ input_port_0_r },
	{ 0 },
	{ input_port_1_r },
	{ 0 },
	{ 0 },
	{ photon2_ppi_c_w }
};

static void photon2_scanline(int scanline)
{
	int next_scanline;

	if (scanline == 0)
	{
		if (++photon2_flash_frame == 50)
			photon2_flash_frame = 0;
		if (photon2_nmi_enabled)
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
		next_scanline = PHOTON2_HEIGHT / 2;
	}
	else
	{
		cpunum_set_input_line(0, 0, HOLD_LINE);
		next_scanline = 0;
	}

	mame_timer_adjust(photon2_scanline_timer, cpu_getscanlinetime_mt(next_scanline), next_scanline, time_zero);
}

static MACHINE_START( photon2 )
{
	ppi8255_init(&photon2_ppi);
	memory_configure_bank(1, 0, 3, memory_region(REGION_CPU1), 0x4000);
	photon2_scanline_timer = mame_timer_alloc(photon2_scanline);

	state_save_register_item("photon2", 0, photon2_border);
	state_save_register_item("photon2", 0, photon2_flash_frame);
	state_save_register_item("photon2", 0, photon2_nmi_enabled);
	return 0;
}

static MACHINE_RESET( photon2 )
{
	memory_set_bank(1, 0);
	ppi8255_0_w(3, 0x9b);
	photon2_fe_w(0, 0);
	photon2_flash_frame = 0;
	mame_timer_adjust(photon2_scanline_timer, cpu_getscanlinetime_mt(0), 0, time_zero);
}

static PALETTE_INIT( photon2 )
{
	int i;

	for (i = 0; i < 16; i++)
	{
		int level = (i & 8) ? 0xff : 0xbf;
		palette_set_color(i, (i & 2) ? level : 0, (i & 4) ? level : 0, (i & 1) ? level : 0);
	}
}

static VIDEO_UPDATE( photon2 )
{
	int y;

	fillbitmap(bitmap, Machine->pens[photon2_border], cliprect);

	for (y = MAX(0, cliprect->min_y - PHOTON2_BORDER); y <= MIN(191, cliprect->max_y - PHOTON2_BORDER); y++)
	{
		/* A character's eight scanlines are separated by 256 bytes in video RAM. */
		const UINT8 *pixels = photon2_vram + ((y & 0xc0) << 5) + ((y & 7) << 8) + ((y & 0x38) << 2);
		const UINT8 *attrs = photon2_vram + 0x1800 + (y >> 3) * 32;
		UINT16 *dest = bitmap->line[y + PHOTON2_BORDER];
		int x;

		for (x = MAX(0, cliprect->min_x - PHOTON2_BORDER); x <= MIN(255, cliprect->max_x - PHOTON2_BORDER); )
		{
			int attr = attrs[x >> 3];
			int ink = (attr & 7) | ((attr >> 3) & 8);
			int paper = (attr >> 3) & 15;
			int bits = pixels[x >> 3];
			int end = MIN(x | 7, cliprect->max_x - PHOTON2_BORDER);

			if ((attr & 0x80) && photon2_flash_frame >= 25)
				bits ^= 0xff;
			for (; x <= end; x++)
				dest[x + PHOTON2_BORDER] = Machine->pens[(bits & (0x80 >> (x & 7))) ? ink : paper];
		}
	}
}

static MACHINE_DRIVER_START( photon2 )
	MDRV_CPU_ADD(Z80, 3500000)
	MDRV_CPU_PROGRAM_MAP(photon2_mem, 0)
	MDRV_CPU_IO_MAP(photon2_io, 0)

	MDRV_MACHINE_START(photon2)
	MDRV_MACHINE_RESET(photon2)

	MDRV_FRAMES_PER_SECOND(50.08)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(PHOTON2_WIDTH, PHOTON2_HEIGHT)
	MDRV_VISIBLE_AREA(0, PHOTON2_WIDTH - 1, 0, PHOTON2_HEIGHT - 1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(photon2)
	MDRV_VIDEO_UPDATE(photon2)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( kok )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "kok00.bin", 0x0000, 0x2000, CRC(ff790d0b) SHA1(26dce26e43c15fd90d99abf25a86ca55ed13de94) )
	ROM_LOAD( "kok01.bin", 0x2000, 0x2000, CRC(bf81811e) SHA1(26f073e49f126a70008256ea74394fbf11649503) )
	ROM_LOAD( "kok10.bin", 0x4000, 0x2000, CRC(73fc7b92) SHA1(226abcb40aa3b8cfa96bc4ac89ba62b79ee79b2a) )
	ROM_LOAD( "kok11.bin", 0x6000, 0x2000, CRC(7de0f54a) SHA1(3ee73f8e133ff3356e0ee8d1918b99d66f5ba53f) )
ROM_END

ROM_START( black )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "black1.bin", 0x0000, 0x2000, CRC(8b02b314) SHA1(2e2de2b21634538515d4d6ca0930c4e3e5d1e80f) )
	ROM_LOAD( "black2.bin", 0x2000, 0x2000, CRC(ed93469a) SHA1(5e23da3f649f5e20f7c8450bfa5d7d0b190f892c) )
	ROM_LOAD( "black5.bin", 0x4000, 0x2000, CRC(f7c0baf5) SHA1(0d0a6f8b7f9bf65be61c8c78270a8c6e60fa3fe9) )
	ROM_LOAD( "black6.bin", 0x6000, 0x2000, CRC(1f60bc18) SHA1(fd1a902c51e01dfc6fa42dac94d25566ce5bb3d7) )
	ROM_LOAD( "black3.bin", 0x8000, 0x2000, CRC(784ea7f4) SHA1(f3008ad180ad14e0728bf0ba78fe85302ef2ff85) )
	ROM_LOAD( "black4.bin", 0xa000, 0x2000, CRC(20281f74) SHA1(83df590e21a44fa07d4bc76818a8d0d0c4de42b3) )
ROM_END

ROM_START( brod )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "brod00.bin", 0x0000, 0x2000, CRC(cbd6653f) SHA1(18b9134529a1e56c6e90f3bcef5102d5d4b352e3) )
	ROM_FILL( 0x2000, 0x2000, 0x00 )
	ROM_LOAD( "brod10.bin", 0x4000, 0x2000, CRC(9c25d44a) SHA1(f78c7e5b4e6f9fe34f81dc574ca335f70b61e68d) )
	ROM_LOAD( "brod11.bin", 0x6000, 0x2000, CRC(f6505a16) SHA1(3b2ccca78fd83855003cc752766df83b19f89364) )
	ROM_LOAD( "brod12.bin", 0x8000, 0x2000, CRC(94e53d47) SHA1(698415c5e25528e3b1dcab7471cc98c1dc9cb335) )
	ROM_LOAD( "brod13.bin", 0xa000, 0x2000, CRC(1177cd17) SHA1(58c5c09a7b857ce6311339c4d0f4d8c1a7e232a3) )
ROM_END

GAME( 19??, kok,   0, photon2, photon2, 0, ROT0, "bootleg", "Povar / Sobrat' Buran / Agroprom (Arcade multi-game bootleg of ZX Spectrum 'Cookie', 'Jetpac' & 'Pssst')", GAME_SUPPORTS_SAVE )
GAME( 19??, black, 0, photon2, black,   0, ROT0, "bootleg", "Czernyj Korabl (Arcade bootleg of ZX Spectrum 'Blackbeard')", GAME_SUPPORTS_SAVE )
GAME( 19??, brod,  0, photon2, black,   0, ROT0, "bootleg", "Brodjaga (Arcade bootleg of ZX Spectrum 'Inspector Gadget and the Circus of Fear')", GAME_SUPPORTS_SAVE )
