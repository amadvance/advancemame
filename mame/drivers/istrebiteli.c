// license:BSD-3-Clause
// copyright-holders:MetalliC
/*************************************************************************

    Istrebiteli driver by MetalliC

    Backported from MAME src/mame/ussr/istrebiteli.cpp

    TODO:
      hardware-like noise sound generation
      accurate sprite collision

    how to play:
      insert one or more coins, each coin gives 2 minutes of play time, then press 1 or 2 player game start
      hit enemy 15 or more times to get bonus game

    test mode:
      insert 12 or more coins then press 2 player start

    notes:
      dumped PCB is early game version, has several bugs, possible test/prototype.
      later version was seen in St.Petersburg arcade museum, CPU board has single 8Kx8 ROM.

    Differences from MAME:
      Motogonki is not backported.
      Tilemap is replaced by direct drawing of the 16 characters.
      Coin counter is updated at VBLANK instead of on the input change.

**************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/custom.h"
#include "streams.h"

static UINT8 coins;
static UINT8 coin_prev;
static UINT8 spr_ctrl[2];
static UINT8 spr_collision[2];
static UINT8 spr_xy[8];
static UINT8 tileram[16];

/*************************************
 *
 *  Sound
 *
 *************************************/

/* 8 MHz / 2 / 256 clock, the stream runs at half of it */
#define SOUND_RATE 7812

static sound_stream *sound_channel;
static UINT16 snd_rom_cnt;
static UINT8 snd_rom_incr;
static UINT8 snd_sample_num;
static UINT8 snd_rom_out_en;
static UINT8 snd_prev_data;

static void istrebiteli_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	const UINT8 *rom = memory_region(REGION_SOUND1);
	int i;

	for (i = 0; i < samples; ++i)
	{
		int smpl = 0;

		if (snd_rom_out_en)
			smpl = (rom[snd_rom_cnt] >> snd_sample_num) & 1;

		/* below is huge guess */
		if ((snd_prev_data & 0x40) == 0)	/* b6 noise enable ? */
			smpl &= mame_rand() & 1;
		smpl *= (snd_prev_data & 0x80) ? 1000 : 4000;	/* b7 volume ? */

		outputs[0][i] = smpl;
		snd_rom_cnt = (snd_rom_cnt + snd_rom_incr) & 0x1ff;
	}
}

static void istrebiteli_sound_reset(void *token)
{
	snd_rom_cnt = 0;
	snd_rom_incr = 0;
	snd_sample_num = 0;
	snd_rom_out_en = 0;
	snd_prev_data = 0;
}

static void *istrebiteli_sound_start(int clock, const struct CustomSound_interface *config)
{
	istrebiteli_sound_reset(0);
	sound_channel = stream_create(0, 1, SOUND_RATE, 0, istrebiteli_sound_update);
	return sound_channel;
}

static const struct CustomSound_interface istrebiteli_sound_interface =
{
	istrebiteli_sound_start, 0, istrebiteli_sound_reset, 0
};

static WRITE8_HANDLER( sound_w )
{
	coin_lockout_w(0, data & 1);
	if (data & 1)
		coins = 0;

	stream_update(sound_channel, 0);

	snd_sample_num = (data >> 2) & 7;
	snd_rom_out_en = (data & 0x20) ? 0 : 1;

	if (data & 2)
	{
		snd_rom_cnt = 0;
		snd_rom_incr = 0;
	}
	else
		snd_rom_incr = 1;

	snd_prev_data = data;
}

/*************************************
 *
 *  Video
 *
 *************************************/

static PALETTE_INIT( istrebiteli )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0x00, 0x00, 0xff);
	palette_set_color(2, 0xff, 0xff, 0xff);
	palette_set_color(3, 0x00, 0x00, 0xff);
	colortable[0] = 0;
	colortable[1] = 1;
	colortable[2] = 2;
	colortable[3] = 3;
}

static VIDEO_UPDATE( istrebiteli )
{
	int i;
	int code;

	fillbitmap(bitmap, Machine->pens[1], cliprect);

	/* the 16 characters are drawn in a fixed row */
	for (i = 0; i < 16; ++i)
		drawgfx(bitmap, Machine->gfx[0], tileram[i] & 0x1f, 0, 0, 0, 96 + i * 8, 64, cliprect, TRANSPARENCY_NONE, 0);

	drawgfx(bitmap, Machine->gfx[3], (spr_ctrl[0] & 0x40) ? 5 : 7, 0, 0, 0, spr_xy[4], spr_xy[5], cliprect, TRANSPARENCY_PEN, 1);
	drawgfx(bitmap, Machine->gfx[3], (spr_ctrl[1] & 0x40) ? 4 : 6, 0, 0, 0, spr_xy[6], spr_xy[7], cliprect, TRANSPARENCY_PEN, 1);

	code = (spr_ctrl[0] & 0x1f) + ((spr_ctrl[0] & 0x80) >> 2);
	drawgfx(bitmap, Machine->gfx[1], code, 0, 0, 0, spr_xy[0], spr_xy[1], cliprect, TRANSPARENCY_PEN, 1);
	code = (spr_ctrl[1] & 0x1f) + ((spr_ctrl[1] & 0x80) >> 2);
	drawgfx(bitmap, Machine->gfx[2], code, 0, 0, 0, spr_xy[2], spr_xy[3], cliprect, TRANSPARENCY_PEN, 1);
}

/*************************************
 *
 *  Machine
 *
 *************************************/

/* the coin switch is polled at VBLANK, the impulse lasts one frame */
static INTERRUPT_GEN( istrebiteli_interrupt )
{
	UINT8 coin = readinputport(3) & 1;

	if (!coin_prev && coin)
		++coins;
	coin_prev = coin;
}

/* real hardware does per-pixel sprite collision detection, this is an approximation */
static int collision_r(int id)
{
	if ((spr_ctrl[id] & 0x80) == 0)
	{
		int sx = spr_xy[0 + id * 2];
		int sy = spr_xy[1 + id * 2];
		int px = spr_xy[6 - id * 2] + 3;
		int py = spr_xy[7 - id * 2] + 3;

		if (sx > 56 && px >= sx && px < (sx + 8) && py >= sy && py < (sy + 8))
			spr_collision[id] |= 1;
	}
	return spr_collision[id] & 1;
}

static READ8_HANDLER( in0_r )
{
	return (readinputport(0) & ~0x20) | (collision_r(1) ? 0 : 0x20);
}

static READ8_HANDLER( in1_r )
{
	return (readinputport(1) & ~0x20) | (collision_r(0) ? 0 : 0x20);
}

static READ8_HANDLER( in2_r )
{
	UINT8 data = readinputport(2) & 0x03;

	data |= (coins << 2) & 0x3c;
	/* the screen is 256 pixels wide with the first 64 hidden by the horizontal blanking */
	if (cpu_gethorzbeampos() < 64)
		data |= 0x40;
	if (cpu_getvblank())
		data |= 0x80;
	return data;
}

static WRITE8_HANDLER( spr0_ctrl_w )
{
	spr_ctrl[0] = data;
	if (data & 0x80)
		spr_collision[0] = 0;
}

static WRITE8_HANDLER( spr1_ctrl_w )
{
	spr_ctrl[1] = data;
	if (data & 0x80)
		spr_collision[1] = 0;
}

static ppi8255_interface ppi8255_intf =
{
	2,
	{ in1_r,        NULL           },
	{ in0_r,        NULL           },
	{ NULL,         in2_r          },
	{ NULL,         spr0_ctrl_w    },
	{ NULL,         spr1_ctrl_w    },
	{ sound_w,      NULL           }
};

static MACHINE_RESET( istrebiteli )
{
	coins = 0;
	coin_prev = 0;
	memset(spr_ctrl, 0, sizeof(spr_ctrl));
	memset(spr_collision, 0, sizeof(spr_collision));
	memset(spr_xy, 0, sizeof(spr_xy));
	memset(tileram, 0, sizeof(tileram));
	ppi8255_init(&ppi8255_intf);
}

/* the CPU address and data buses are inverted on this PCB */
static READ8_HANDLER( ppi0_r )
{
	return ppi8255_r(0, offset ^ 3) ^ 0xff;
}

static WRITE8_HANDLER( ppi0_w )
{
	ppi8255_w(0, offset ^ 3, data ^ 0xff);
}

static READ8_HANDLER( ppi1_r )
{
	return ppi8255_r(1, offset ^ 3) ^ 0xff;
}

static WRITE8_HANDLER( ppi1_w )
{
	ppi8255_w(1, offset ^ 3, data ^ 0xff);
}

static WRITE8_HANDLER( tileram_w )
{
	tileram[offset ^ 15] = data;
}

static WRITE8_HANDLER( spr_xy_w )
{
	spr_xy[offset ^ 7] = data;
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1000, 0x13ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1000, 0x13ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xc0, 0xc3) AM_READ(ppi0_r)
	AM_RANGE(0xc4, 0xc7) AM_READ(ppi1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xb0, 0xbf) AM_WRITE(tileram_w)
	AM_RANGE(0xc0, 0xc3) AM_WRITE(ppi0_w)
	AM_RANGE(0xc4, 0xc7) AM_WRITE(ppi1_w)
	AM_RANGE(0xc8, 0xcf) AM_WRITE(spr_xy_w)
ADDRESS_MAP_END

INPUT_PORTS_START( istreb )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )	/* collision */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )	/* collision */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
INPUT_PORTS_END

static const gfx_layout char_layout =
{
	8,16,
	32,
	1,
	{ 0 },
	{ 7,6,5,4,3,2,1,0 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	8*16
};

static const gfx_layout sprite_layout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 7*8,6*8,5*8,4*8,3*8,2*8,1*8,0*8 },
	8*8
};

static const gfx_layout projectile_layout =
{
	16,16,
	8,
	1,
	{ 0 },
	{ 15*8,14*8,13*8,12*8,11*8,10*8,9*8,8*8,7*8,6*8,5*8,4*8,3*8,2*8,1*8,0*8 },
	{ 15*128,14*128,13*128,12*128,11*128,10*128,9*128,8*128,7*128,6*128,5*128,4*128,3*128,2*128,1*128,0*128 },
	1
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &char_layout, 0, 2 },
	{ REGION_GFX2, 0x0000, &sprite_layout, 2, 2 },
	{ REGION_GFX2, 0x0000, &sprite_layout, 0, 2 },
	{ REGION_GFX2, 0x0200, &projectile_layout, 0, 2 },
	{ -1 }
};

static DRIVER_INIT( istreb )
{
	UINT8 *gfx = memory_region(REGION_GFX2);
	UINT8 temp[64];
	int offs, spnum, dot;

	for (offs = 0; offs < 0x200; offs += 0x40)
	{
		memset(temp, 0, sizeof(temp));
		for (spnum = 0; spnum < 8; ++spnum)
			for (dot = 0; dot < 64; ++dot)
				temp[(dot >> 3) + spnum * 8] |= ((gfx[offs + dot] >> spnum) & 1) << (dot & 7);
		memcpy(&gfx[offs], temp, sizeof(temp));
	}
}

static MACHINE_DRIVER_START( istreb )
	MDRV_CPU_ADD(8080, 8000000 / 4 / 2)	/* KR580VM80A */
	MDRV_CPU_PROGRAM_MAP(readmem, writemem)
	MDRV_CPU_IO_MAP(readport, writeport)
	MDRV_CPU_VBLANK_INT(istrebiteli_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(8000000.0 / 2 / (256 * 312))
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(istrebiteli)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 312)
	MDRV_VISIBLE_AREA(64, 255, 0, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(istrebiteli)
	MDRV_VIDEO_UPDATE(istrebiteli)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(istrebiteli_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

ROM_START( istreb )
	ROM_REGION( 0x1000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_LOAD( "002-ia12.bin",   0x000, 0x200, CRC(de0bce75) SHA1(ca284e8220d0d55c1a4dd3e951b53404f40fc873) )
	ROM_LOAD( "002-ia9.bin",    0x200, 0x200, CRC(e9a93ee7) SHA1(63c2001140d2b30657fceca97a639b1acbf614c2) )
	ROM_LOAD( "002-ib11-2.bin", 0x400, 0x200, CRC(4bb8b875) SHA1(230193e08586f4585fe98b2b31c4c8aa57a83e70) )
	ROM_LOAD( "002-ib9.bin",    0x600, 0x200, CRC(4eb948b5) SHA1(e9926591d1b0c528630b54956993c01139c58913) )
	ROM_LOAD( "002-ib13.bin",   0x800, 0x200, CRC(4fec5b14) SHA1(72b01c28882d567cad6924e05849438e5fe7a133) )

	ROM_REGION( 0x200, REGION_GFX1, 0 )
	ROM_LOAD( "003-g8.bin", 0x000, 0x200, CRC(5cd7ad47) SHA1(2142711c8a3640b7aa258a2059cfb0f14297a5ac) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )
	ROM_LOAD( "001-g4.bin",  0x000, 0x200, CRC(ca3c531b) SHA1(8295167895d51e626b6d5946b565d5e8b8466ac0) )
	ROM_LOAD( "001-g9.bin",  0x000, 0x200, CRC(ca3c531b) SHA1(8295167895d51e626b6d5946b565d5e8b8466ac0) )
	ROM_LOAD( "001-a11.bin", 0x200, 0x100, CRC(4e05b7dd) SHA1(335e975ae9e8f775c1ac07f60420680ad878c3ae) )
	ROM_LOAD( "001-b11.bin", 0x200, 0x100, CRC(4e05b7dd) SHA1(335e975ae9e8f775c1ac07f60420680ad878c3ae) )

	ROM_REGION( 0x200, REGION_SOUND1, 0 )
	ROM_LOAD( "003-w3.bin", 0x000, 0x200, CRC(54eb4893) SHA1(c7a4724045c645ab728074ed7fef1882d9776005) )
ROM_END

GAME( 198?, istreb, 0, istreb, istreb, istreb, ROT0, "Terminal", "Istrebiteli", GAME_IMPERFECT_SOUND )
