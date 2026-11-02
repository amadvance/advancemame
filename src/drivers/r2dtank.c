// license:BSD-3-Clause
// copyright-holders:David Haywood, Pierpaolo Prazzoli
/*******************************************************************
R2D Tank (c) 1980 Sigma Ent. Inc.

driver by: David Haywood & Pierpaolo Prazzoli

from the readme
----------------------------------------------------
Orca board number OVG-17A

r2d1.1c is ROM #1 at board position 1C, and so on.

1 = 2716
2 = 2732
3 = 2732
4 = 2732
5 = 2716 Sound

CPU = 6809
other = HD46505SP
other = MB14282(x2)
other = HD468458SP
other = MB14282
other = MB14368
other = HD6821 (x2)
other = HD46802
other = M5L8226 (x2)
RAM = 4116 (x11)

Should be very similar to Sigma's Spiders hardware.
----------------------------------------------------
********************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"

static UINT8 *main_ram;

static UINT8 ay_selected;
static int display_sync;
static mame_timer *sync_timer;
static mame_timer *scanline_timer;

static void main_cpu_irq(int state)
{
	int irq = pia_get_irq_a(0) | pia_get_irq_b(0) | pia_get_irq_a(1) | pia_get_irq_b(1);
	cpunum_set_input_line(0, M6809_IRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE8_HANDLER( audio_command_w )
{
	soundlatch_w(0, ~data);
	cpunum_set_input_line(1, M6800_IRQ_LINE, HOLD_LINE);
}

static WRITE8_HANDLER( audio_answer_w )
{
	/* the sound program leaves this handshake pending at the end of an intermission */
	if (activecpu_get_pc() == 0xfb12)
		data = 0;
	soundlatch2_w(0, data);
	cpunum_set_input_line(0, M6809_IRQ_LINE, HOLD_LINE);
}

static WRITE8_HANDLER( ay_select_w )
{
	ay_selected = data;
}

static READ8_HANDLER( ay_port_r )
{
	UINT8 data = 0;
	if (ay_selected & 0x08) data = AY8910_read_port_0_r(0);
	if (ay_selected & 0x10) data = AY8910_read_port_1_r(0);
	return data;
}

static WRITE8_HANDLER( ay_port_w )
{
	if (ay_selected & 0x04)
	{
		if (ay_selected & 0x08) AY8910_control_port_0_w(0, data);
		if (ay_selected & 0x10) AY8910_control_port_1_w(0, data);
	}
	else
	{
		if (ay_selected & 0x08) AY8910_write_port_0_w(0, data);
		if (ay_selected & 0x10) AY8910_write_port_1_w(0, data);
	}
}

static READ8_HANDLER( main_input_r )
{
	return (readinputport(0) & 0x7f) | (display_sync ? 0x80 : 0);
}

static void sync_clear(int param)
{
	display_sync = 0;
	pia_set_input_ca1(0, 0);
}

static void scanline_callback(int scanline)
{
	if (scanline < crtc6845_vert_disp * (crtc6845_max_ras_addr + 1))
	{
		display_sync = 1;
		pia_set_input_ca1(0, 1);
		/* retrigger the 22 kohm / 10 nF monostable through horizontal blanking */
		timer_adjust(sync_timer, TIME_IN_SEC(0.68 * 22000 * 0.00000001), 0, 0);
	}
	scanline = (scanline + 1) % 276;
	timer_adjust(scanline_timer, cpu_getscanlinetime(scanline), scanline, 0);
}

static int r2dtank_video_flip;
WRITE8_HANDLER( r2dtank_video_flip_w )
{
	/*  0 -> flipped
        1 -> not flipped */

	r2dtank_video_flip = !data;
}

WRITE8_HANDLER( r2dtank_pia_0_w )
{
	pia_0_w(offset, ~data);
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x8000, 0x8003) AM_READ(pia_0_r)
	AM_RANGE(0x8004, 0x8004) AM_READ(soundlatch2_r)
	AM_RANGE(0xc000, 0xc007) AM_READ(MRA8_RAM)
	AM_RANGE(0xc800, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&main_ram)
	AM_RANGE(0x8000, 0x8003) AM_WRITE(r2dtank_pia_0_w)
	AM_RANGE(0x8004, 0x8004) AM_WRITE(audio_command_w)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0xb001, 0xb001) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0xc000, 0xc007) AM_WRITE(MWA8_RAM) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xc800, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_RAM
	AM_RANGE(0xd000, 0xd003) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0xf000, 0xf000) AM_READWRITE(soundlatch_r, audio_answer_w)
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( r2dtank )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE(0x04, IP_ACTIVE_LOW)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )

	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x08, "10000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Free_Play ) )

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )


INPUT_PORTS_END

PALETTE_INIT( r2dtank )
{
	int i;
	for (i = 0; i < 8; ++i)
		palette_set_color(i, (i & 4) ? 255 : 0, (i & 2) ? 255 : 0, (i & 1) ? 255 : 0);
}

VIDEO_UPDATE( r2dtank )
{
	int y, cx, bit;
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		int ra = y % (crtc6845_max_ras_addr + 1);
		int ma = crtc6845_start_addr + (y / (crtc6845_max_ras_addr + 1)) * crtc6845_horiz_disp;
		for (cx = 0; cx < crtc6845_horiz_disp && cx < 32; ++cx, ++ma)
		{
			int offs = ((ma << 3) & 0x1f00) | ((ra << 5) & 0xe0) | (ma & 0x1f);
			UINT8 data;
			int color;
			if (r2dtank_video_flip) offs ^= 0x1fff;
			data = main_ram[offs];
			color = (main_ram[0x4000 + offs] >> 5) & 7;
			for (bit = 0; bit < 8; ++bit)
			{
				int pixel = r2dtank_video_flip ? (data & (1 << bit)) : (data & (0x80 >> bit));
				plot_pixel(bitmap, cx * 8 + bit, y, Machine->pens[pixel ? color : 0]);
			}
		}
	}
}

static const struct AY8910interface ay_0_intf =
{
	input_port_3_r, 0, 0, 0
};

static const struct AY8910interface ay_1_intf =
{
	input_port_1_r, input_port_2_r, 0, 0
};

/* Declare PIA structure */

/* PIA 0, main CPU */
static const pia6821_interface pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ main_input_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, r2dtank_video_flip_w,
	/*irqs   : A/B             */ main_cpu_irq, main_cpu_irq
};

/* PIA 1, sound CPU */
static const pia6821_interface pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ ay_port_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ ay_port_w, ay_select_w, 0, 0,
	/*irqs   : A/B             */ main_cpu_irq, main_cpu_irq
};

MACHINE_START( r2dtank )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia_1_intf);
	sync_timer = timer_alloc(sync_clear);
	scanline_timer = timer_alloc(scanline_callback);
	state_save_register_global(ay_selected);
	state_save_register_global(display_sync);
	state_save_register_global(r2dtank_video_flip);
	return 0;
}

MACHINE_RESET( r2dtank )
{
	pia_reset();
	ay_selected = 0;
	display_sync = 0;
	timer_adjust(sync_timer, TIME_NEVER, 0, 0);
	timer_adjust(scanline_timer, cpu_getscanlinetime(0), 0, 0);
}

static MACHINE_DRIVER_START( r2dtank )
	MDRV_CPU_ADD(M6809,11200000/4)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)

	/* Sound CPU */
	MDRV_CPU_ADD(M6802,3579545/4)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_map,0)

	MDRV_FRAMES_PER_SECOND(5600000.0 / (360 * 276))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 360 * (276 - 224) / 5600000))

	MDRV_MACHINE_START(r2dtank)
	MDRV_MACHINE_RESET(r2dtank)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 276)
	MDRV_VISIBLE_AREA(0, 255, 0, 223)

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(r2dtank)

	MDRV_VIDEO_UPDATE(r2dtank)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 3579545/4)
	MDRV_SOUND_CONFIG(ay_0_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD(AY8910, 3579545/4)
	MDRV_SOUND_CONFIG(ay_1_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( r2dtank )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "r2d1.1c",      0xc800, 0x0800, CRC(20606a0f) SHA1(9a55e595c7ea332bdc89142338947be8a28a92a3) )
	ROM_LOAD( "r2d2.1a",      0xd000, 0x1000, CRC(7561c67f) SHA1(cccc7bbd7975db340fe571a4c31c25b41b2563b8) )
	ROM_LOAD( "r2d3.2c",      0xe000, 0x1000, CRC(fc53c538) SHA1(8f9a2edcf7a2cb2a8ddd084828b52f1bf45f434a) )
	ROM_LOAD( "r2d4.2a",      0xf000, 0x1000, CRC(56636225) SHA1(dcfc6e29b4c51a45cfbecf6790b7d88b89af433b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "r2d5.7l",      0xf800, 0x0800, CRC(c49bed15) SHA1(ffa635a65c024c532bb13fb91bbd3e54923e81bf) )
ROM_END

GAME( 1980, r2dtank, 0, r2dtank, r2dtank, 0, ROT270, "Sigma Enterprises Inc.", "R2D Tank", 0 )
