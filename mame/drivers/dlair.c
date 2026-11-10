// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*
 * Dragon's Lair, Cinematronics US board, revision F2.
 * Z80 at 4 MHz, AY-3-8910 at 2 MHz, Pioneer LD-V1000 laserdisc player.
 */

#include "driver.h"
#include "machine/ldv1000.h"
#include "sound/ay8910.h"
#include "sound/custom.h"

static UINT8 last_misc, disc_data;

static WRITE8_HANDLER( misc_w )
{
	UINT8 diff = data ^ last_misc;
	last_misc = data;
	coin_counter_w(0, (~data >> 4) & 1);
	/* The data latch reaches the player only on the falling edge of bit 5. */
	if ((diff & 0x20) && !(data & 0x20)) ldv1000_write(disc_data);
}
static WRITE8_HANDLER( disc_w ) { disc_data = data; }
static READ8_HANDLER( disc_r ) { return ldv1000_read(); }
static READ8_HANDLER( system_r )
{
	return (readinputport(3) & 0x3f) | (ldv1000_status_strobe() << 6) | (!ldv1000_ready() << 7);
}

static UINT8 digits[16];
static const UINT8 led_map[16] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7c,0x07,0x7f,0x67,0x77,0x7c,0x39,0x5e,0x79,0x00 };
static WRITE8_HANDLER( led1_w ) { digits[offset & 7] = led_map[data & 15]; }
static WRITE8_HANDLER( led2_w ) { digits[8 | (offset & 7)] = led_map[data & 15]; }

static ADDRESS_MAP_START( dlair_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xa7ff) AM_MIRROR(0x1800) AM_RAM
	AM_RANGE(0xc000, 0xc000) AM_MIRROR(0x1fc7) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0xc008, 0xc008) AM_MIRROR(0x1fc7) AM_READ(input_port_2_r)
	AM_RANGE(0xc010, 0xc010) AM_MIRROR(0x1fc7) AM_READ(system_r)
	AM_RANGE(0xc020, 0xc020) AM_MIRROR(0x1fc7) AM_READ(disc_r)
	AM_RANGE(0xe000, 0xe000) AM_MIRROR(0x1fc7) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xe008, 0xe008) AM_MIRROR(0x1fc7) AM_WRITE(misc_w)
	AM_RANGE(0xe010, 0xe010) AM_MIRROR(0x1fc7) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xe020, 0xe020) AM_MIRROR(0x1fc7) AM_WRITE(disc_w)
	AM_RANGE(0xe030, 0xe037) AM_MIRROR(0x1fc0) AM_WRITE(led2_w)
	AM_RANGE(0xe038, 0xe03f) AM_MIRROR(0x1fc0) AM_WRITE(led1_w)
ADDRESS_MAP_END

INPUT_PORTS_START( dlair )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x04, 0x00, "Difficulty Mode" )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x00, "Engineering Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "2 Credits/Free play" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Pay as you go" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Sound Every 8 Attracts" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unlimited Dirks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "LD Player" )
	PORT_DIPSETTING(    0x08, "LD-V1000" )
	PORT_DIPNAME( 0x60, 0x60, "Pay as you go options" )
	PORT_DIPSETTING(    0x00, "PAYG1" )
	PORT_DIPSETTING(    0x20, "PAYG2" )
	PORT_DIPSETTING(    0x40, "PAYG3" )
	PORT_DIPSETTING(    0x60, "PAYG4" )
	PORT_DIPNAME( 0x90, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Increase after 5" )
	PORT_DIPSETTING(    0x10, "Increase after 9" )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x90, DEF_STR( Easy ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_SPECIAL )
INPUT_PORTS_END

static MACHINE_START( dlair ) { return ldv1000_init(); }
static MACHINE_RESET( dlair )
{
	last_misc = disc_data = 0;
	memset(digits, 0, sizeof(digits));
	ldv1000_reset();
}
static INTERRUPT_GEN( dlair_field ) { ldv1000_field(); }
static VIDEO_UPDATE( dlair ) { ldv1000_video(bitmap, cliprect); }
static const struct AY8910interface ay_interface = { input_port_0_r, input_port_1_r, 0, 0 };

static MACHINE_DRIVER_START( dlair )
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(dlair_mem, 0)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold, TIME_IN_HZ(16000000.0 / 8 / 16 / 16 / 16 / 16))
	MDRV_CPU_VBLANK_INT(dlair_field, 1)
	MDRV_MACHINE_START(dlair)
	MDRV_MACHINE_RESET(dlair)
	MDRV_FRAMES_PER_SECOND(60000.0 / 1001)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(720, 524)
	MDRV_VISIBLE_AREA(0, 703, 44, 523)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_VIDEO_UPDATE(dlair)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.33)
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(ldv1000_sound)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( dlair )
	ROM_REGION( 0xa000, REGION_CPU1, 0 )
	ROM_LOAD( "dl_f2_u1.bin", 0x0000, 0x2000,  CRC(f5ea3b9d) SHA1(c0cafff8b2982125fd3314ffc66681e47f027fc9) )
	ROM_LOAD( "dl_f2_u2.bin", 0x2000, 0x2000,  CRC(dcc1dff2) SHA1(614ca8f6c5b6fa1d590f6b80d731377faa3a65a9) )
	ROM_LOAD( "dl_f2_u3.bin", 0x4000, 0x2000,  CRC(ab514e5b) SHA1(29d1015b951f0f2d4e5257497f3bf007c5e2262c) )
	ROM_LOAD( "dl_f2_u4.bin", 0x6000, 0x2000,  CRC(f5ec23d2) SHA1(71149e2d359cc5944fbbb53dd7d0c2b42fbc9bb4) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "dlair", 0, SHA1(c3ee2e27aa4847bf3884fc0d18f26f315456aa9e) )
ROM_END

GAME( 1983, dlair, 0, dlair, dlair, 0, ROT0, "Cinematronics", "Dragon's Lair (US Rev. F2)", 0 )
