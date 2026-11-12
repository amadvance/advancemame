// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Chris Hardy
/**************************************************************************************************

Filetto (c) 1990 Novarmatic

TODO:
- Add a proper FDC device;
- UM5100 sample clock;
- buzzer sound;

===================================================================================================
The PCB is a un-modified IBM-PC with a CGA adapter & a prototyping card that controls the
interface between the pc and the Jamma connectors.Additionally there's also a UM5100 sound
chip for the sound.
PCB Part Number: S/N 90289764 NOVARXT
PCB Contents:
1x UMC 8923S-UM5100 voice processor (upper board)
1x MMI PAL16L8ACN-940CRK9 (upper board)
1x AMD AMPAL16R8APC-8804DM (upper board)
1x AMD P8088-1 main processor 8.000MHz (lower board)
1x Proton PT8010AF PLCC 28.636MHz (lower board)
1x UMC 8928LP-UM8272A floppy disk controller (lower board)
1x UMC 8935CS-UM82C11 Printer Adapter Interface (lower board)
1x UMC 8936CS-UM8250B Programmable asynchronous communications element (lower board)
1x UMC 8937NS-UM82C8167 Real Time Clock (lower board)
1x Yamaha V6363 CMDC QFP (lower board)
There isn't any keyboard found connected to the pcb.

===================================================================================================
The software of this game can be extracted with a normal Windows program extractor.
The files names are:
-command.com  (1)
-ibmbio.com   (1)
-ibmdos.com   (1)
-ansi.sys     (1)
-config.sys   (2)
-autoexec.bat (3)
-x.exe        (4)
(1)This is an old Italian version of MS-DOS (v3.30 18th March 1987).
(2)Contains "device=ansi.sys",it's an hook-up for the graphics used by the BIOS.
(3)It has an Echo off (as you can notice from the game itself) and then the loading of the
main program (x.exe).
(4)The main program,done in plain Basic with several Italian comments in it.The date of
the main program is 9th October 1990.

**************************************************************************************************/

#include "driver.h"
#include "cpu/i86/i88intf.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "machine/8237dma.h"
#include "machine/pcshare.h"
#include "sound/hc55516.h"

static UINT8 *cga_vram;
static UINT8 cga_mode;
static UINT8 cga_color;
static UINT8 cga_index;
static UINT32 cga_frame;
static UINT8 cga_regs[0x20];

static int bank_last;
static UINT8 disk_data[2];
static UINT8 port_b_data;
static UINT8 fdc_status;
static UINT8 fdc_clr_status;
static UINT8 voice;
static UINT8 voice_bit;
static UINT32 voice_addr;
static mame_timer *voice_timer;

static const UINT8 cga_palette[16][3] =
{
	{ 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0xaa }, { 0x00, 0xaa, 0x00 }, { 0x00, 0xaa, 0xaa },
	{ 0xaa, 0x00, 0x00 }, { 0xaa, 0x00, 0xaa }, { 0xaa, 0x55, 0x00 }, { 0xaa, 0xaa, 0xaa },
	{ 0x55, 0x55, 0x55 }, { 0x55, 0x55, 0xff }, { 0x55, 0xff, 0x55 }, { 0x55, 0xff, 0xff },
	{ 0xff, 0x55, 0x55 }, { 0xff, 0x55, 0xff }, { 0xff, 0xff, 0x55 }, { 0xff, 0xff, 0xff }
};

static PALETTE_INIT( filetto )
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
	const UINT8 *font = memory_region(REGION_USER2) + 0x1800;
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
					int pen = Machine->pens[(bits & (0x80 >> c)) ? fg : bg];

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
				plot_pixel(bitmap, x, y, Machine->pens[(line[x >> 3] & (0x80 >> (x & 7))) ? fg : 0]);
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
				int pen = Machine->pens[pal[(line[x >> 2] >> ((~x & 3) * 2)) & 3]];

				plot_pixel(bitmap, x * 2, y, pen);
				plot_pixel(bitmap, x * 2 + 1, y, pen);
			}
		}
	}
}

static VIDEO_UPDATE( filetto )
{
	++cga_frame;
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	if (cga_mode & 0x08)
	{
		if (cga_mode & 0x02)
			cga_draw_graphics(bitmap);
		else
			cga_draw_text(bitmap);
	}
}

static READ8_HANDLER( disk_iobank_r )
{
	if (offset == 1)
		return readinputport(1);

	return disk_data[offset];
}

static WRITE8_HANDLER( disk_iobank_w )
{
	/*
	The BIOS does a single out $0310,$F0 on reset, then two outs to
	select the ROM bank visible at $C0000-$CFFFF:

	X1  X2

	$F0 $F2 = m0
	$F1 $F2 = m1
	$F0 $F3 = m2
	$F1 $F3 = m3
	*/
	int bank = 0;

	if (data != 0xf0)
	{
		if (bank_last == 0xf0 && data == 0xf2)
			bank = 0;
		else if (bank_last == 0xf1 && data == 0xf2)
			bank = 1;
		else if (bank_last == 0xf0 && data == 0xf3)
			bank = 2;
		else if (bank_last == 0xf1 && data == 0xf3)
			bank = 3;
	}

	/* the low nibble values below $10 select the voice sample */
	if (!(data & 0xf0))
	{
		int bit = (data >> 1) - 2;

		voice &= ~(1 << bit);
		voice |= (data & 1) << bit;
	}

	memory_set_bankptr(1, memory_region(REGION_USER1) + 0x10000 * bank);

	bank_last = data;
	disk_data[offset] = data;
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
	pit8253_0_gate_w(2, port_b_data & 1);
	port_b_data = data;
	pit8253_0_gate_w(2, data & 1);
}

/* Only enough of the 765 is emulated to let the BIOS pass its checks */
#define FDC_WRITE 0x40
#define FDC_READ 0x00

static READ8_HANDLER( fdc765_status_r )
{
	UINT8 res = fdc_status | 0x80;

	++fdc_clr_status;
	if (fdc_clr_status == 0x10)
	{
		fdc_status = 0;
		fdc_clr_status = 0;
	}
	return res;
}

static READ8_HANDLER( fdc765_data_r )
{
	fdc_status = FDC_READ;
	pic8259_set_irq_line(0, 6, 0);
	return 0xc0;
}

static WRITE8_HANDLER( fdc765_data_w )
{
	fdc_status = FDC_WRITE;
}

static WRITE8_HANDLER( fdc_dor_w )
{
	pic8259_set_irq_line(0, 6, 1);
}

static void voice_tick(int param)
{
	hc55516_digit_w(0, (memory_region(REGION_USER3)[voice_addr] >> voice_bit) & 1);
	hc55516_clock_w(0, 1);
	hc55516_clock_w(0, 0);

	if (voice_bit == 0)
	{
		++voice_addr;
		voice_bit = 8;
		if (!(voice_addr % 0x8000))
			mame_timer_adjust(voice_timer, time_never, 0, time_zero);
	}
	--voice_bit;
}

static WRITE8_HANDLER( voice_start_w )
{
	/* hand tuned, should be between ~30 and 44.1 kHz */
	mame_timer_adjust(voice_timer, time_zero, 0, double_to_mame_time(1.0 / 44100));
	voice_bit = 7;
	voice_addr = ((voice & 3) | (((voice >> 4) & 1) << 2)) * 0x8000;
}

static WRITE8_HANDLER( cga_vram_w )
{
	cga_vram[offset] = data;
}

static READ8_HANDLER( cga_vram_r )
{
	return cga_vram[offset];
}

static ADDRESS_MAP_START( filetto_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAM
	AM_RANGE(0xb8000, 0xbbfff) AM_READWRITE(cga_vram_r, cga_vram_w) AM_BASE(&cga_vram)
	AM_RANGE(0xbc000, 0xbffff) AM_READWRITE(cga_vram_r, cga_vram_w)
	AM_RANGE(0xc0000, 0xcffff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( filetto_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x000f) AM_READWRITE(dma8237_0_r, dma8237_0_w)
	AM_RANGE(0x0020, 0x0021) AM_READWRITE(pic8259_0_r, pic8259_0_w)
	AM_RANGE(0x0040, 0x0043) AM_READWRITE(pit8253_0_r, pit8253_0_w)
	AM_RANGE(0x0060, 0x0060) AM_READ(port_a_r)
	AM_RANGE(0x0061, 0x0061) AM_READWRITE(port_b_r, port_b_w)
	AM_RANGE(0x0062, 0x0062) AM_READ(port_c_r)
	AM_RANGE(0x0080, 0x0083) AM_READWRITE(pc_page_r, pc_page_w)
	AM_RANGE(0x0201, 0x0201) AM_READ(input_port_2_r)
	AM_RANGE(0x0310, 0x0311) AM_READWRITE(disk_iobank_r, disk_iobank_w)
	AM_RANGE(0x0312, 0x0312) AM_READ(input_port_0_r)
	AM_RANGE(0x0313, 0x0313) AM_WRITE(voice_start_w)
	AM_RANGE(0x03b4, 0x03b5) AM_READWRITE(cga_regs_r, cga_regs_w)
	AM_RANGE(0x03d4, 0x03d5) AM_READWRITE(cga_regs_r, cga_regs_w)
	AM_RANGE(0x03d8, 0x03d9) AM_WRITE(cga_mode_w)
	AM_RANGE(0x03da, 0x03da) AM_READ(cga_status_r)
	AM_RANGE(0x03f2, 0x03f2) AM_WRITE(fdc_dor_w)
	AM_RANGE(0x03f4, 0x03f4) AM_READ(fdc765_status_r)
	AM_RANGE(0x03f5, 0x03f5) AM_READWRITE(fdc765_data_r, fdc765_data_w)
ADDRESS_MAP_END

INPUT_PORTS_START( filetto )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Extra Play" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, "Play at 6th match reached" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static int filetto_irq_callback(int irqline)
{
	return pic8259_acknowledge(0);
}

static MACHINE_START( filetto )
{
	voice_timer = mame_timer_alloc(voice_tick);
	return 0;
}

static MACHINE_RESET( filetto )
{
	bank_last = -1;
	voice = 0;
	fdc_status = 0;
	fdc_clr_status = 0;
	port_b_data = 0;
	cga_mode = 0;
	cga_color = 0;
	memset(cga_regs, 0, sizeof(cga_regs));

	memory_set_bankptr(1, memory_region(REGION_USER1));
	dma8237_reset();
	cpunum_set_irq_callback(0, filetto_irq_callback);
}

static DRIVER_INIT( filetto )
{
	init_pc_common(PCCOMMON_KEYBOARD_PC | PCCOMMON_DMA8237_PC | PCCOMMON_TIMER_8253);
}

static MACHINE_DRIVER_START( filetto )
	MDRV_CPU_ADD(I88, 14318181/3)
	MDRV_CPU_PROGRAM_MAP(filetto_map, 0)
	MDRV_CPU_IO_MAP(filetto_io, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(filetto)
	MDRV_MACHINE_RESET(filetto)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_VISIBLE_AREA(0, 639, 0, 199)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(filetto)
	MDRV_VIDEO_UPDATE(filetto)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(HC55516, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

ROM_START( filetto )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "u49.bin", 0xfc000, 0x2000, CRC(1be6948a) SHA1(9c433f63d347c211ee4663f133e8417221bc4bf0) )
	ROM_RELOAD(          0xf8000, 0x2000 )
	ROM_RELOAD(          0xf4000, 0x2000 )
	ROM_RELOAD(          0xf0000, 0x2000 )
	ROM_LOAD( "u55.bin", 0xfe000, 0x2000, CRC(1e455ed7) SHA1(786d18ce0ab1af45fc538a2300853e497488f0d4) )
	ROM_RELOAD(          0xfa000, 0x2000 )
	ROM_RELOAD(          0xf6000, 0x2000 )
	ROM_RELOAD(          0xf2000, 0x2000 )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* program data */
	ROM_LOAD( "m0.u1", 0x00000, 0x10000, CRC(2408289d) SHA1(eafc144a557a79b58bcb48545cb9c9778e61fcd3) )
	ROM_LOAD( "m1.u2", 0x10000, 0x10000, CRC(5b623114) SHA1(0d9a14e6b7f57ce4fa09762343b610a973910f58) )
	ROM_LOAD( "m2.u3", 0x20000, 0x10000, CRC(abc64869) SHA1(564fc9d90d241a7b7776160b3fd036fb08037355) )
	ROM_LOAD( "m3.u4", 0x30000, 0x10000, CRC(0c1e8a67) SHA1(f1b9280c65fcfcb5ec481cae48eb6f52d6cdbc9d) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* CGA character generator */
	ROM_LOAD( "u67.bin", 0x0000, 0x2000, CRC(09710122) SHA1(de84bdd9245df287bbd3bb808f0c3531d13a3545) )

	ROM_REGION( 0x40000, REGION_USER3, 0 ) /* UM5100 sample roms */
	ROM_LOAD( "v1.u15", 0x00000, 0x20000, CRC(613ddd07) SHA1(ebda3d559315879819cb7034b5696f8e7861fe42) )
	ROM_LOAD( "v2.u14", 0x20000, 0x20000, CRC(427e012e) SHA1(50514a6307e63078fe7444a96e39d834684db7df) )
ROM_END

GAME( 1990, filetto, 0, filetto, filetto, filetto, ROT0, "Novarmatic", "Filetto (v1.05 901009)", GAME_IMPERFECT_SOUND )
