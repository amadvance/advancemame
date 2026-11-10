// license:BSD-3-Clause
// copyright-holders:Aaron Giles,Ernesto Corvi
/* Stern Cliff Hanger with Pioneer PR-8210 and TMS9128 video overlay. */
#include "driver.h"
#include "hash.h"
#include "cpu/i8039/i8039.h"
#include "machine/ldpr8210.h"
#include "machine/ldv1000.h"
#include "sound/custom.h"
#include "sound/discrete.h"
#include "vidhrdw/tms9928a.h"

static UINT8 port_bank;
static UINT32 philips_code;
static mame_bitmap *overlay;

static WRITE8_HANDLER( bank_w )
{
	port_bank = data == 0x0f ? 0 : data & 15;
}

static READ8_HANDLER( bank_r )
{
	return port_bank < 7 ? readinputport(port_bank) : 0xff;
}

static READ8_HANDLER( code_r ) { return philips_code >> (8 * offset); }
static READ8_HANDLER( irq_ack_r )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	return 0;
}

static WRITE8_HANDLER( disc_w ) { pr8210_serial_w(data & 1); }
static WRITE8_HANDLER( counter_w ) { coin_counter_w(0, data & 0x40); }
static WRITE8_HANDLER( sound_w )
{
	discrete_sound_w(NODE_01, data & 1);
	discrete_sound_w(NODE_02, (data >> 1) & 1);
}

static ADDRESS_MAP_START( main_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xe000, 0xe7ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xe800, 0xefff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x44, 0x44) AM_WRITE(TMS9928A_vram_w)
	AM_RANGE(0x45, 0x45) AM_READ(TMS9928A_vram_r)
	AM_RANGE(0x46, 0x46) AM_WRITE(sound_w)
	AM_RANGE(0x50, 0x52) AM_READ(code_r)
	AM_RANGE(0x53, 0x53) AM_READ(irq_ack_r)
	AM_RANGE(0x54, 0x54) AM_WRITE(TMS9928A_register_w)
	AM_RANGE(0x55, 0x55) AM_READ(TMS9928A_register_r)
	AM_RANGE(0x57, 0x57) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x60, 0x60) AM_WRITE(bank_w)
	AM_RANGE(0x62, 0x62) AM_READ(bank_r)
	AM_RANGE(0x64, 0x64) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x66, 0x66) AM_WRITE(disc_w)
	AM_RANGE(0x68, 0x68) AM_WRITE(counter_w)
	AM_RANGE(0x6a, 0x6a) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x6e, 0x6f) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pr_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0, 0x7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pr_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0, 0x1ff) AM_READWRITE(pr8210_io_r, pr8210_io_w)
ADDRESS_MAP_END

static void vdp_irq(int state)
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, state ? ASSERT_LINE : CLEAR_LINE);
}

static const TMS9928a_interface vdp = { TMS99x8A, 0x4000, vdp_irq };

static void vbi_irq(int line)
{
	philips_code = pr8210_field_code(line);
	if (philips_code & 0x800000)
		cpunum_set_input_line(0, 0, ASSERT_LINE);
}

static INTERRUPT_GEN( cliffhgr_field )
{
	pr8210_field();
	/* The board latches lines 17 and 18 separately, before active video. */
	timer_set(TIME_IN_USEC(17 * 63.555), 17, vbi_irq);
	timer_set(TIME_IN_USEC(18 * 63.555), 18, vbi_irq);
	TMS9928A_interrupt();
}

static MACHINE_START( cliffhgr )
{
	const char *expected = CRC(120fa83b) SHA1(b514326ca1f52d6d89056868f9d17eabd4e3f31d);
	char actual[HASH_BUF_SIZE];
	UINT8 *rom = memory_region(REGION_CPU2);
	mame_file *file = mame_fopen("pr8210", "pr-8210_mcu_ud6005a.bin", FILETYPE_ROM, 0);

	if (!file)
		fatalerror("Missing PR-8210 player firmware");
	if (mame_fsize(file) != 0x800 || mame_fread(file, rom, 0x800) != 0x800)
		fatalerror("Invalid PR-8210 player firmware length");
	mame_fclose(file);
	hash_compute(actual, rom, 0x800, HASH_CRC | HASH_SHA1);
	if (!hash_data_is_equal(actual, expected, 0))
		fatalerror("Incorrect PR-8210 player firmware");

	overlay = auto_bitmap_alloc_depth(286, 243, 32);
	return pr8210_init();
}

static MACHINE_RESET( cliffhgr )
{
	port_bank = philips_code = 0;
	TMS9928A_reset();
	pr8210_reset();
}

static VIDEO_UPDATE( cliffhgr )
{
	const rectangle area = { 3, 282, 15, 230 };
	int x, y;

	ldv1000_video(bitmap, cliprect);
	TMS9928A_draw_overlay(overlay, &area);
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT32 *dest = bitmap->line[y];
		const UINT32 *src = overlay->line[15 + (y - 44) * 216 / 480];
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			UINT32 pixel = src[3 + x * 280 / 704];
			if (pixel != MAKE_RGB(1, 2, 3))
				dest[x] = pixel;
		}
	}
}

INPUT_PORTS_START( cliffhgr )
	PORT_START_TAG("BANK0")
	PORT_BIT ( 0x3F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )  /* SW2 on CPU PCB */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )  /* SW1 on CPU PCB */

	PORT_START_TAG("BANK1")
	PORT_DIPNAME( 0xc0, 0xc0, "Should Have Hint" )      PORT_DIPLOCATION("E11:7,8")
	PORT_DIPSETTING(    0xc0, "Never" )
	PORT_DIPSETTING(    0x80, "After 1st Player Mistake" )
	PORT_DIPSETTING(    0x40, "After 2nd Player Mistake" )
	PORT_DIPSETTING(    0x00, "After 3rd Player Mistake" )
	PORT_DIPNAME( 0x20, 0x00, "Action/Stick Hints" )        PORT_DIPLOCATION("E11:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Display Score and Lives During Animation" )  PORT_DIPLOCATION("E11:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Regular Length Scenes" ) PORT_DIPLOCATION("E11:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "No Hanging Scene" )      PORT_DIPLOCATION("E11:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )        PORT_DIPLOCATION("E11:1,2")
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )

	PORT_START_TAG("BANK2")
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("F11:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("F11:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_B ) )       PORT_DIPLOCATION("F11:5,6")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )

	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )      PORT_DIPLOCATION("F11:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )      PORT_DIPLOCATION("F11:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )       PORT_DIPLOCATION("F11:1,2")
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )

	PORT_START_TAG("BANK3")
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Allow_Continue ) )   PORT_DIPLOCATION("G11:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Short Scenes" )          PORT_DIPLOCATION("G11:7")
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("G11:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Disc Test" )             PORT_DIPLOCATION("G11:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Player Immortality" )    PORT_DIPLOCATION("G11:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )    PORT_DIPLOCATION("G11:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Switch Test" )           PORT_DIPLOCATION("G11:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "Service Index" )         PORT_DIPLOCATION("G11:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("BANK4")
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )      PORT_DIPLOCATION("H11:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )      PORT_DIPLOCATION("H11:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      PORT_DIPLOCATION("H11:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )      PORT_DIPLOCATION("H11:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0f, 0x0f, "Move Difficulty" )       PORT_DIPLOCATION("H11:1,2,3,4")
	PORT_DIPSETTING(    0x0f, "0 (Easiest)" )
	PORT_DIPSETTING(    0x0e, "1" )
	PORT_DIPSETTING(    0x0d, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x0b, "4" )
	PORT_DIPSETTING(    0x0a, "5" )
	PORT_DIPSETTING(    0x09, "6" )
	PORT_DIPSETTING(    0x08, "7" )
	PORT_DIPSETTING(    0x07, "8" )
	PORT_DIPSETTING(    0x06, "9" )
	PORT_DIPSETTING(    0x05, "10" )
	PORT_DIPSETTING(    0x04, "11" )
	PORT_DIPSETTING(    0x03, "12" )
	PORT_DIPSETTING(    0x02, "13" )
	PORT_DIPSETTING(    0x01, "14" )
	PORT_DIPSETTING(    0x00, "15 (Hardest)" )

	PORT_START_TAG("BANK5")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START_TAG("BANK6")
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

#define CLIFF_ENABLE_SND_1  NODE_01
#define CLIFF_ENABLE_SND_2  NODE_02

static const struct discrete_555_desc desc_cliffhgr_555 =
{
	DISC_555_OUT_ENERGY | DISC_555_OUT_DC,
	5,  /* B+ voltage of 555 */
	DEFAULT_555_VALUES
};

static const struct discrete_mixer_desc desc_cliffhgr_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	{ 1.0/(1.0/RES_K(4.7)+1.0/RES_K(10))+RES_K(10), 1.0/(1.0/RES_K(5.1)+1.0/RES_K(12))+RES_K(10) },
	{ 0, 0 },
	{ CAP_U(4.7), CAP_U(4.7) },
	0,
	RES_K(10),
	0,
	CAP_U(4.7),
	0,
	8
};


DISCRETE_SOUND_START(cliffhgr_discrete)
	DISCRETE_INPUT_LOGIC(CLIFF_ENABLE_SND_1)
	DISCRETE_INPUT_LOGIC(CLIFF_ENABLE_SND_2)

	DISCRETE_555_ASTABLE(NODE_03, 1, RES_K(24), RES_K(10), CAP_U(0.047), &desc_cliffhgr_555)
	DISCRETE_555_ASTABLE(NODE_04, 1, RES_K(24), RES_K(10), CAP_U(0.1), &desc_cliffhgr_555)

	DISCRETE_ONOFF(NODE_05,CLIFF_ENABLE_SND_1,NODE_03)
	DISCRETE_ONOFF(NODE_06,CLIFF_ENABLE_SND_2,NODE_04)

	DISCRETE_RCFILTER(NODE_07,1,NODE_05,1.0/(1.0/RES_K(4.7)+1.0/RES_K(10)), CAP_U(0.1))
	DISCRETE_RCFILTER(NODE_08,1,NODE_06,1.0/(1.0/RES_K(5.1)+1.0/RES_K(12)), CAP_U(0.1))

	DISCRETE_GAIN(NODE_09,NODE_07,RES_K(10)/(RES_K(4.7)+RES_K(10)))
	DISCRETE_GAIN(NODE_10,NODE_08,RES_K(12)/(RES_K(5.1)+RES_K(12)))

	DISCRETE_MIXER2(NODE_11,1,NODE_09,NODE_10,&desc_cliffhgr_mixer)

	DISCRETE_OUTPUT(NODE_11,32767/9)
DISCRETE_SOUND_END

static MACHINE_DRIVER_START( cliffhgr )
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(main_mem, 0)
	MDRV_CPU_IO_MAP(main_io, 0)
	MDRV_CPU_VBLANK_INT(cliffhgr_field, 1)
	MDRV_CPU_ADD(I8039, 4410000 / I8039_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(pr_mem, 0)
	MDRV_CPU_IO_MAP(pr_io, 0)
	MDRV_INTERLEAVE(256)
	MDRV_MACHINE_START(cliffhgr)
	MDRV_MACHINE_RESET(cliffhgr)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_TMS9928A(&vdp)
	MDRV_FRAMES_PER_SECOND(60000.0 / 1001)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(720, 524)
	MDRV_VISIBLE_AREA(0, 703, 44, 523)
	MDRV_VIDEO_UPDATE(cliffhgr)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(ldv1000_sound)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
	MDRV_SOUND_ADD(DISCRETE, 0)
	MDRV_SOUND_CONFIG(cliffhgr_discrete)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
MACHINE_DRIVER_END

ROM_START( cliffhgr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cliff_u1.bin",   0x0000, 0x2000, CRC(a86ec38f) SHA1(bfca1b1c084f5b7b1e0ccb2f3616ecea1340f04c) )
	ROM_LOAD( "cliff_u2.bin",   0x2000, 0x2000, CRC(b8d33b6b) SHA1(02778f87a78199129c758a8fb0629b9ba74cab99) )
	ROM_LOAD( "cliff_u3.bin",   0x4000, 0x2000, CRC(75a64cd2) SHA1(18fe4d8885b59ec8b8c28b5d7141a27164c982ac) )
	ROM_LOAD( "cliff_u4.bin",   0x6000, 0x2000, CRC(906b2af1) SHA1(65fadd2fec90f47c91ac4928f342c79ab8bc6ef0) )
	ROM_LOAD( "cliff_u5.bin",   0x8000, 0x2000, CRC(5922e710) SHA1(10637baba4d16dc333aeb0ab88ee251f44e1a115) )

	ROM_REGION( 0x800, REGION_CPU2, ROMREGION_ERASE00 )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "cliffhgr", 0, SHA1(4442995c824d7891a2a19c607bb3301d696fbdc8) )
ROM_END

GAME( 1983, cliffhgr, 0, cliffhgr, cliffhgr, 0, ROT0, "Stern Electronics", "Cliff Hanger (set 1)", 0 )
