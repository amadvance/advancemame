// license:BSD-3-Clause
// copyright-holders:Roberto Fresca, Angelo Salese
/*
  Coco Loco
  Petaco S.A. (Spain)
  PCB 112-020: R6502AP, AY-3-8910, 20 MHz crystal.
*/

#include "driver.h"
#include "sound/ay8910.h"

#define MASTER_CLOCK 20000000

static UINT8 *cocoloco_vram;
static UINT8 video_bank;

static PALETTE_INIT( cocoloco )
{
	int i;
	for (i = 0; i < 16; i++) palette_set_color(i, 0, 0, 0);
	for (i = 0; i < 16; i += 2)
		palette_set_color(i, pal2bit(i & 3), pal2bit((i >> 1) & 3), pal2bit((i >> 2) & 3));
	palette_set_color(0x01, 0xff, 0xff, 0x00);
	palette_set_color(0x02, 0x00, 0xff, 0x00);
	palette_set_color(0x04, 0x00, 0x7f, 0xff);
	palette_set_color(0x08, 0xff, 0x7f, 0x00);
	palette_set_color(0x0a, 0x00, 0xff, 0xff);
	palette_set_color(0x0c, 0xff, 0x00, 0x00);
	palette_set_color(0x0e, 0xff, 0xff, 0xff);
}

static VIDEO_START( cocoloco )
{
	cocoloco_vram = auto_malloc(0x8000);
	memset(cocoloco_vram, 0, 0x8000);
	state_save_register_global_pointer(cocoloco_vram, 0x8000);
	state_save_register_global(video_bank);
	return 0;
}

static VIDEO_UPDATE( cocoloco )
{
	int x, y;
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	for (y = MAX(1, cliprect->min_y); y <= cliprect->max_y; y++)
	{
		UINT16 *dest = bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int pos = (x >> 3) * 256 + 256 - y;
			int bit = x & 7;
			int color = ((cocoloco_vram[pos] >> bit) & 1)
				| (((cocoloco_vram[pos | 0x2000] >> bit) & 1) << 1)
				| (((cocoloco_vram[pos | 0x4000] >> bit) & 1) << 2)
				| (((cocoloco_vram[pos | 0x6000] >> bit) & 1) << 3);
			dest[x] = Machine->pens[color];
		}
	}
}

static READ8_HANDLER( vram_r )
{
	return cocoloco_vram[offset] | cocoloco_vram[offset | 0x2000]
		| cocoloco_vram[offset | 0x4000] | cocoloco_vram[offset | 0x6000];
}
static WRITE8_HANDLER( vram_w )
{
	/* Each write replaces all four planes, including those not selected. */
	cocoloco_vram[offset] = video_bank == 0 ? data : 0;
	cocoloco_vram[offset | 0x2000] = video_bank & 2 ? data : 0;
	cocoloco_vram[offset | 0x4000] = video_bank & 4 ? data : 0;
	cocoloco_vram[offset | 0x6000] = video_bank & 8 ? data : 0;
}
static WRITE8_HANDLER( video_bank_w ) { video_bank = data; }
static WRITE8_HANDLER( coin_counter_w_handler ) { coin_counter_w(0, data & 8); }

static void coin_changed(void *param, UINT32 oldval, UINT32 newval)
{
	/* The NMI handler must sample the coin bit before its impulse expires. */
	if (newval) cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}
static MACHINE_START( cocoloco )
{
	input_port_set_changed_callback(0, 1, coin_changed, 0);
	return 0;
}
static MACHINE_RESET( cocoloco ) { video_bank = 0; }

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(vram_r, vram_w)
	AM_RANGE(0x6001, 0x6001) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x6002, 0x6002) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x6003, 0x6003) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8003, 0x8003) AM_WRITE(video_bank_w)
	AM_RANGE(0x8005, 0x8005) AM_WRITE(coin_counter_w_handler)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa005, 0xa005) AM_WRITENOP
	AM_RANGE(0xd000, 0xffff) AM_ROM
ADDRESS_MAP_END

static INPUT_PORTS_START( cocoloco )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Select / Speed-Up Button")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Char Speed" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPSETTING(    0x01, "Slow" )
	PORT_DIPNAME( 0x02, 0x00, "Monsters Speed" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPSETTING(    0x02, "Slow" )
	PORT_DIPNAME( 0x0c, 0x00, "Monsters: Time before go out" )
	PORT_DIPSETTING(    0x00, "Long" )
	PORT_DIPSETTING(    0x04, "Medium-Long" )
	PORT_DIPSETTING(    0x08, "Medium-Short" )
	PORT_DIPSETTING(    0x0c, "Short" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000 Points" )
	PORT_DIPSETTING(    0x04, "15000 Points" )
	PORT_DIPSETTING(    0x08, "20000 Points" )
	PORT_DIPSETTING(    0x0c, "30000 Points" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, "Monsters" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, "Vitamin Time" )
	PORT_DIPSETTING(    0x00, "Long" )
	PORT_DIPSETTING(    0x40, "Medium-Long" )
	PORT_DIPSETTING(    0x80, "Medium-Short" )
	PORT_DIPSETTING(    0xc0, "Short" )
INPUT_PORTS_END

static struct AY8910interface ay_interface = { input_port_1_r, input_port_2_r };

static MACHINE_DRIVER_START( cocoloco )
	MDRV_CPU_ADD(M6502, MASTER_CLOCK / 16)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_FRAMES_PER_SECOND(MASTER_CLOCK / 4.0 / 384 / 262)
	MDRV_VBLANK_DURATION(6 * 384 * 1000000.0 / (MASTER_CLOCK / 4))
	MDRV_MACHINE_START(cocoloco)
	MDRV_MACHINE_RESET(cocoloco)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(cocoloco)
	MDRV_VIDEO_START(cocoloco)
	MDRV_VIDEO_UPDATE(cocoloco)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, MASTER_CLOCK / 8)
	MDRV_SOUND_CONFIG(ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( cocoloco )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "coco1-c.b1", 0xd000, 0x0800, CRC(f2a699a9) SHA1(7032af9b416665df2f17bb308a297d65c29a09fb) )
	ROM_LOAD( "coco1-d.b1", 0xd800, 0x0800, CRC(08e00d09) SHA1(82b3982df81fb374bd385935c3ca61330f1a673f) )
	ROM_LOAD( "coco1-e.c1", 0xe000, 0x0800, CRC(2adbf6ff) SHA1(f2d3f33b7d8812974765303f3ea5230e3b7bed37) )
	ROM_LOAD( "coco1-f.c1", 0xe800, 0x0800, CRC(2ada04a0) SHA1(8d0402d7f77e382afebecf5e8f7b2aac4567b9e7) )
	ROM_LOAD( "coco1-g.e1", 0xf000, 0x0800, CRC(454fbc8e) SHA1(edf22d1939a3a14f30f14c021e47aa9404fa4c75) )
	ROM_LOAD( "coco1-h.e1", 0xf800, 0x0800, CRC(b6d0ebea) SHA1(a8f09558f71dfe0d300a6bb946dcb3bf6393c02b) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "prom.c10",   0x0000, 0x0100, CRC(fea8a821) SHA1(c744cac6af7621524fc3a2b0a9a135a32b33c81b) )
ROM_END

GAME( 1981, cocoloco, 0, cocoloco, cocoloco, 0, ROT90, "Petaco S.A.", "Coco Loco (set 1)", GAME_SUPPORTS_SAVE )
