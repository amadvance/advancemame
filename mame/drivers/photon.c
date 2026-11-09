// license:BSD-3-Clause
// copyright-holders:Mariusz Wojcieszek, Miodrag Milanovic
/*
    Photon System
    Russian arcade system based on the PK8000 home computer.
*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/dac.h"

static UINT8 pk8000_text_start;
static UINT8 pk8000_chargen_start;
static UINT8 pk8000_video_start;
static UINT8 pk8000_color_start;

static UINT8 pk8000_video_mode;
static UINT8 pk8000_video_color;
static UINT8 pk8000_color[32];
static UINT8 pk8000_video_enable;

static READ8_HANDLER(pk8000_video_color_r)
{
	return pk8000_video_color;
}

static WRITE8_HANDLER(pk8000_video_color_w)
{
	pk8000_video_color = data;
}

static READ8_HANDLER(pk8000_text_start_r)
{
	return pk8000_text_start;
}

static WRITE8_HANDLER(pk8000_text_start_w)
{
	pk8000_text_start = data;
}

static READ8_HANDLER(pk8000_chargen_start_r)
{
	return pk8000_chargen_start;
}

static WRITE8_HANDLER(pk8000_chargen_start_w)
{
	pk8000_chargen_start = data;
}

static READ8_HANDLER(pk8000_video_start_r)
{
	return pk8000_video_start;
}

static WRITE8_HANDLER(pk8000_video_start_w)
{
	pk8000_video_start = data;
}

static READ8_HANDLER(pk8000_color_start_r)
{
	return pk8000_color_start;
}

static WRITE8_HANDLER(pk8000_color_start_w)
{
	pk8000_color_start = data;
}

static READ8_HANDLER(pk8000_color_r)
{
	return pk8000_color[offset];
}

static WRITE8_HANDLER(pk8000_color_w)
{
	pk8000_color[offset] = data;
}

static const UINT8 pk8000_palette[16 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0xc0, 0x00,
	0x00, 0xff, 0x00,
	0x00, 0x00, 0xc0,
	0x00, 0x00, 0xff,
	0x00, 0xc0, 0xc0,
	0x00, 0xff, 0xff,
	0xc0, 0x00, 0x00,
	0xff, 0x00, 0x00,
	0xc0, 0xc0, 0x00,
	0xff, 0xff, 0x00,
	0xc0, 0x00, 0xc0,
	0xff, 0x00, 0xff,
	0xc0, 0xc0, 0xc0,
	0xff, 0xff, 0xff,
};

static PALETTE_INIT( pk8000 )
{
	palette_set_colors(0, pk8000_palette, 16);
}

static void pk8000_video_update(mame_bitmap *bitmap, const rectangle *cliprect, UINT8 *videomem)
{
	int x,y,j,b;
	UINT16 offset = (pk8000_video_mode & 0xc0) << 8;
	rectangle my_rect;
	my_rect.min_x = 0;
	my_rect.max_x = 256+32-1;
	my_rect.min_y = 0;
	my_rect.max_y = 192+32-1;

	if (pk8000_video_enable) {
		fillbitmap(bitmap, Machine->pens[(pk8000_video_color >> 4) & 0x0f], &my_rect);

		if (BIT(pk8000_video_mode,4)==0){

			if (BIT(pk8000_video_mode,5)==0){

				for (y = 0; y < 24; y++)
				{
					for (x = 0; x < 32; x++)
					{
						UINT8 chr  = videomem[x +(y*32) + ((pk8000_text_start & 0x0f) << 10)+offset] ;
						UINT8 color= pk8000_color[chr>>3];
						for (j = 0; j < 8; j++) {
							UINT8 code = videomem[((chr<<3) + j) + ((pk8000_chargen_start & 0x0e) << 10)+offset];

							for (b = 0; b < 8; b++)
							{
								UINT8 col = (code >> b) & 0x01 ? (color & 0x0f) : ((color>>4) & 0x0f);
								plot_pixel(bitmap, x*8+(7-b)+16, (y*8)+j+16, Machine->pens[col]);
							}
						}
					}
				}
			} else {

				for (y = 0; y < 24; y++)
				{
					for (x = 0; x < 42; x++)
					{
						UINT8 chr = videomem[x +(y*64) + ((pk8000_text_start & 0x0e) << 10)+offset] ;
						for (j = 0; j < 8; j++) {
							UINT8 code = videomem[((chr<<3) + j) + ((pk8000_chargen_start  & 0x0e) << 10)+offset];
							for (b = 2; b < 8; b++)
							{
								UINT8 col = ((code >> b) & 0x01) ? (pk8000_video_color) & 0x0f : (pk8000_video_color>>4) & 0x0f;
								plot_pixel(bitmap, x*6+(7-b)+16+8, (y*8)+j+16, Machine->pens[col]);
							}
						}
					}
				}
			}
		} else {

			for (y = 0; y < 24; y++)
			{
				UINT16 off_color = (((~pk8000_color_start) & 0x08) << 10)+offset + ((y>>3)<<11);
				UINT16 off_code  = (((~pk8000_video_start) & 0x08) << 10)+offset + ((y>>3)<<11);
				for (x = 0; x < 32; x++)
				{
					UINT8 chr  = videomem[x +(y*32) + ((pk8000_chargen_start & 0x0e) << 10)+offset] ;
					for (j = 0; j < 8; j++) {
						UINT8 color= videomem[((chr<<3) + j)+off_color];
						UINT8 code = videomem[((chr<<3) + j)+off_code];

						for (b = 0; b < 8; b++)
						{
							UINT8 col = (code >> b) & 0x01 ? (color & 0x0f) : ((color>>4) & 0x0f);
							plot_pixel(bitmap, x*8+(7-b)+16, (y*8)+j+16, Machine->pens[col]);
						}
					}
				}
			}
		}
	} else {

		fillbitmap(bitmap, Machine->pens[0], &my_rect);
	}
    return;
}

static void pk8000_set_bank(UINT8 data)
{
	UINT8 *rom = memory_region(REGION_CPU1);
	/* RAM stays writable beneath the ROM selected for instruction reads. */
	UINT8 *ram = memory_region(REGION_CPU1);
	UINT8 block1 = data & 3;
	UINT8 block2 = (data >> 2) & 3;
	UINT8 block3 = (data >> 4) & 3;
	UINT8 block4 = (data >> 6) & 3;

	switch(block1) {
		case 0:
				memory_set_bankptr(1, rom + 0x10000);
				memory_set_bankptr(5, ram);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(1, ram);
				memory_set_bankptr(5, ram);
				break;
	}

	switch(block2) {
		case 0:
				memory_set_bankptr(2, rom + 0x14000);
				memory_set_bankptr(6, ram + 0x4000);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(2, ram + 0x4000);
				memory_set_bankptr(6, ram + 0x4000);
				break;
	}
	switch(block3) {
		case 0:
				memory_set_bankptr(3, rom + 0x18000);
				memory_set_bankptr(7, ram + 0x8000);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(3, ram + 0x8000);
				memory_set_bankptr(7, ram + 0x8000);
				break;
	}
	switch(block4) {
		case 0:
				memory_set_bankptr(4, rom + 0x1c000);
				memory_set_bankptr(8, ram + 0xc000);
				break;
		case 1: break;
		case 2: break;
		case 3:
				memory_set_bankptr(4, ram + 0xc000);
				memory_set_bankptr(8, ram + 0xc000);
				break;
	}
}
static WRITE8_HANDLER(pk8000_80_porta_w)
{
	pk8000_set_bank(data);
}

static READ8_HANDLER(pk8000_80_portb_r)
{
	return 0xff;
}

static WRITE8_HANDLER(pk8000_80_portc_w)
{
	DAC_data_w(0, BIT(data,7) ? 255 : 0);
}

static READ8_HANDLER(pk8000_84_porta_r)
{
	return pk8000_video_mode;
}

static WRITE8_HANDLER(pk8000_84_porta_w)
{
	pk8000_video_mode = data;
}

static WRITE8_HANDLER(pk8000_84_portc_w)
{
	pk8000_video_enable = BIT(data,4);
}
static const ppi8255_interface photon_ppi = {
	2,
	{ 0, pk8000_84_porta_r },
	{ pk8000_80_portb_r, 0 },
	{ 0, 0 },
	{ pk8000_80_porta_w, pk8000_84_porta_w },
	{ 0, 0 },
	{ pk8000_80_portc_w, pk8000_84_portc_w }
};

static ADDRESS_MAP_START(pk8000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS(AMEF_UNMAP(1))
	AM_RANGE( 0x0000, 0x3fff ) AM_READ(MRA8_BANK1) AM_WRITE(MWA8_BANK5)
	AM_RANGE( 0x4000, 0x7fff ) AM_READ(MRA8_BANK2) AM_WRITE(MWA8_BANK6)
	AM_RANGE( 0x8000, 0xbfff ) AM_READ(MRA8_BANK3) AM_WRITE(MWA8_BANK7)
	AM_RANGE( 0xc000, 0xffff ) AM_READ(MRA8_BANK4) AM_WRITE(MWA8_BANK8)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pk8000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_FLAGS(AMEF_UNMAP(1))
	AM_RANGE(0x80, 0x83) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0x84, 0x87) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0x88, 0x88) AM_READWRITE(pk8000_video_color_r,pk8000_video_color_w)
	AM_RANGE(0x8c, 0x8c) AM_READ(input_port_0_r)
	AM_RANGE(0x8d, 0x8d) AM_READ(input_port_1_r)
	AM_RANGE(0x90, 0x90) AM_READWRITE(pk8000_text_start_r,pk8000_text_start_w)
	AM_RANGE(0x91, 0x91) AM_READWRITE(pk8000_chargen_start_r,pk8000_chargen_start_w)
	AM_RANGE(0x92, 0x92) AM_READWRITE(pk8000_video_start_r,pk8000_video_start_w)
	AM_RANGE(0x93, 0x93) AM_READWRITE(pk8000_color_start_r,pk8000_color_start_w)
	AM_RANGE(0xa0, 0xbf) AM_READWRITE(pk8000_color_r,pk8000_color_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( photon )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_COIN1)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START
	PORT_BIT(0xff, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END

static INTERRUPT_GEN( pk8000_interrupt )
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xff);
}

static DRIVER_INIT(photon)
{
	/* The power-on RAM pattern is also read by the music and game-option code. */
	memset(memory_region(REGION_CPU1), 0xff, 0x10000);
}

static MACHINE_RESET(pk8000)
{
	ppi8255_init(&photon_ppi);
	pk8000_set_bank(0);
}

static VIDEO_START( photon )
{
	return 0;
}

static VIDEO_UPDATE( photon )
{
	pk8000_video_update(bitmap, cliprect, memory_region(REGION_CPU1));
}

static MACHINE_DRIVER_START( photon )

	MDRV_CPU_ADD(8080, 1780000)
	MDRV_CPU_PROGRAM_MAP(pk8000_mem, 0)
	MDRV_CPU_IO_MAP(pk8000_io, 0)
	MDRV_CPU_VBLANK_INT(pk8000_interrupt, 1)

	MDRV_MACHINE_RESET(pk8000)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(288, 224)
	MDRV_VISIBLE_AREA(0, 287, 0, 223)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(pk8000)

	MDRV_VIDEO_START(photon)
	MDRV_VIDEO_UPDATE(photon)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( phtetris )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "foton_tetris.bin", 0x10000, 0x4000, BAD_DUMP CRC(a8af10bb) SHA1(5e2ea9a5d38399cbe156638eea73a3d25c442f77) )
ROM_END

ROM_START( phpython )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "foton_piton.bin", 0x10000, 0x1800, BAD_DUMP CRC(4eac925a) SHA1(26f9a18c7aed31b7daacdc003bafb60a5e6d6300) )
ROM_END

ROM_START( phklad )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "klad.bin", 0x10000, 0x4000, BAD_DUMP CRC(49cc7d65) SHA1(d966cfc1d973a533df8044a71fad37f7177da554) )
ROM_END

GAME( 19??,  phtetris, 0,      photon, photon, photon, ROT0, "<unknown>", "Tetris (Photon System)", 0 )
GAME( 1989?, phpython,  0,     photon, photon, photon, ROT0, "<unknown>", "Python (Photon System)", 0 )
GAME( 19??,  phklad,   0,      photon, photon, photon, ROT0, "<unknown>", "Klad / Labyrinth (Photon System)", 0 )
