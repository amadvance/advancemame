// license:BSD-3-Clause
// copyright-holders:Mariusz Wojcieszek
/*

    United Amusements PC-Engine based hardware

    The system consists of a stock PC-Engine console, JAMMA interface board,
    and several cables to connect certain pins from the PCE backplane connector
    and pad connector to the interface board.

    In 1989 United Amusements (a large operator of arcades in the US at that
    time) developed a JAMMA interface for the PC-Engine with NEC's blessing. NEC
    pulled funding for the project before mass production began, and it never
    took off.

    Driver notes:
    - game time is controlled using software loop - with current clock it takes lots
      of time until credit expires.
    - tone played by jamma if board is not emulated

    Blazing Lazers dump notes:
    An EPROM-based HuCard manufactured by NEC (PCB ID: PWD-703) is used to
    store the game program. It supports four uPD27C2001 (256Kx8) EPROMs as
    well as 27C010s for total ROM capacity of 1MB down to 128K in various
    configurations.

    The board came with three EPROMs in the following configuration:

    IC1 is a 27C010 mapped to 000000-01FFFF
    IC2 is a 27C010 mapped to 020000-03FFFF
    IC3 is a 27C010 mapped to 080000-09FFFF
    IC4 is unpopulated

    The software was a modified version of "Blazing Lazers", identical to the
    USA retail version with the region check (offsets 0 through $18) patched
    with NOP to remove it.

    Keith Courage In Alpha Zones: dump was made from PC-Engine game dump of US version,
     with region check nop'ed out.
    Alien Crush & Pac_Land: dumps made from PC-Engine dumps of JP versions

*/

#include "driver.h"
#include "vidhrdw/huc6270.h"
#include "cpu/h6280/h6280.h"
#include "sound/c6280.h"

#define PCE_MAIN_CLOCK		21477270
#define JOY_CLOCK			0x01
#define JOY_SIGNATURE		0x70

static UINT8 jamma_if_control_latch;
static int joystick_data_select;
static int flipper_buttons;
static int jump_on_up;

static INTERRUPT_GEN( uapce_interrupt )
{
	int ret = 0;

	/* bump current scanline */
	vdc.curline = (vdc.curline + 1) % VDC_LPF;
	vdc.current_segment_line++;

	if (vdc.curline == 0)
	{
		vdc.top_blanking = (vdc.vdc_data[VPR].b.l & 0x1F) + 1;
		vdc.top_overscan = vdc.vdc_data[VPR].b.h + 2;
		vdc.current_bitmap_line = 0;
		vdc.current_segment = STATE_TOPBLANK;
		vdc.current_segment_line = 0;
		vdc.active_lines = vdc.physical_height = 1 + (vdc.vdc_data[VDW].w & 0x01FF);
		vdc.bottomfill = vdc.vdc_data[VCR].b.l;
	}

	switch (vdc.current_segment)
	{
	case STATE_TOPBLANK:
		if (vdc.current_segment_line == vdc.top_blanking)
		{
			vdc.current_segment = STATE_TOPFILL;
			vdc.current_segment_line = 0;
		}
		break;
	case STATE_TOPFILL:
		if (vdc.current_segment_line == vdc.top_overscan)
		{
			vdc.current_segment = STATE_ACTIVE;
			vdc.current_segment_line = 0;
			vdc.y_scroll = vdc.vdc_data[BYR].w;
		}
		break;
	case STATE_ACTIVE:
		if (vdc.current_segment_line == vdc.active_lines)
		{
			vdc.current_segment_line = 0;
			if (vdc.bottomfill != 0)
			{
				vdc.current_segment = STATE_BOTTOMFILL;
				vdc.status |= VDC_VD;
				if (vdc.vdc_data[CR].w & CR_VR)
					ret = 1;

				/* VRAM > SATB DMA if enabled or DVSSR was written */
				if ((vdc.vdc_data[DCR].w & DCR_DSR) || vdc.dvssr_write)
				{
					vdc.dvssr_write = 0;
					memcpy(&vdc.sprite_ram, &vdc.vram[vdc.vdc_data[DVSSR].w << 1], 512);
					vdc.status |= VDC_DS;
					if (vdc.vdc_data[DCR].w & DCR_DSC)
						ret = 1;
				}
			}
			else
				vdc.current_segment = STATE_TOPBLANK;
		}
		break;
	case STATE_BOTTOMFILL:
		if (vdc.current_segment_line == vdc.bottomfill)
		{
			vdc.current_segment_line = 0;
			vdc.current_segment = STATE_TOPBLANK;
		}
		break;
	}

	/* draw a line of the display */
	if (vdc.curline >= FIRST_VISIBLE && vdc.curline < LAST_VISIBLE)
	{
		switch (vdc.current_segment)
		{
		default:
			draw_black_line(vdc.current_bitmap_line);
			break;
		case STATE_TOPFILL:
		case STATE_BOTTOMFILL:
			draw_overscan_line(vdc.current_bitmap_line);
			break;
		case STATE_ACTIVE:
			pce_refresh_line(vdc.current_bitmap_line, vdc.current_segment_line);
			break;
		}
		vdc.current_bitmap_line++;
	}
	else
	{
		/* rendering can take place here, but it's never on screen */
		switch (vdc.current_segment)
		{
		default:
			draw_black_line(243);
			break;
		case STATE_TOPFILL:
		case STATE_BOTTOMFILL:
			draw_overscan_line(243);
			break;
		case STATE_ACTIVE:
			pce_refresh_line(243, vdc.current_segment_line);
			break;
		}
	}

	/* generate interrupt on line compare if necessary */
	if (vdc.vdc_data[CR].w & CR_RC)
	{
		if (vdc.curline == (((vdc.vdc_data[RCR].w - 64) + vdc.top_blanking + vdc.top_overscan) % 263))
		{
			vdc.status |= VDC_RR;
			ret = 1;
		}
	}

	/* handle frame events */
	if (vdc.curline == 261 && vdc.current_segment != STATE_BOTTOMFILL)
	{
		vdc.status |= VDC_VD;

		if ((vdc.vdc_data[DCR].w & DCR_DSR) || vdc.dvssr_write)
		{
			vdc.dvssr_write = 0;
			memcpy(&vdc.sprite_ram, &vdc.vram[vdc.vdc_data[DVSSR].w << 1], 512);
			vdc.status |= VDC_DS;
			if (vdc.vdc_data[DCR].w & DCR_DSC)
				ret = 1;
		}

		if (vdc.vdc_data[CR].w & CR_VR)
			ret = 1;
	}

	if (ret)
		cpunum_set_input_line(0, 0, HOLD_LINE);
}

static WRITE8_HANDLER( pce_joystick_w )
{
	h6280io_set_buffer(data);

	/* do we want buttons or direction? */
	joystick_data_select = data & JOY_CLOCK;
}

static READ8_HANDLER( pce_joystick_r )
{
	int data = readinputportbytag("JOY");

	/* Alien Crush flippers are driven by left and button I, so the two generic buttons are routed to them */
	if (flipper_buttons)
	{
		int flip = readinputportbytag("FLIP");

		data &= ~((flip & 0x01) ? 0 : 0x80);
		data &= ~((flip & 0x02) ? 0 : 0x01);
	}

	/* Pac-Land reads left and right from the buttons and jumps on any direction bit, so the jump button is routed to all of them */
	if (jump_on_up && (data & 0x10))
		data &= 0x0f;
	else if (jump_on_up)
		data |= 0xf0;

	/* the RUN key is connected to the START1 switch only when enabled by the control latch */
	if (!(jamma_if_control_latch & 0x10))
		data |= 0x08;

	if (joystick_data_select)
		data >>= 4;

	return (data & 0x0f) | JOY_SIGNATURE;
}

static WRITE8_HANDLER( jamma_if_control_latch_w )
{
	UINT8 diff = data ^ jamma_if_control_latch;
	jamma_if_control_latch = data;

	if (diff & 0x40)
		cpunum_set_input_line(0, INPUT_LINE_RESET, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);

	coin_counter_w(0, (data >> 5) & 1);
}

static READ8_HANDLER( jamma_if_read_coin )
{
	return readinputportbytag("COIN");
}

static READ8_HANDLER( jamma_if_control_latch_r )
{
	return jamma_if_control_latch & 0x08;
}

static READ8_HANDLER( jamma_if_read_dsw )
{
	int dsw = readinputportbytag("DSW");
	int bit;

	for (bit = 7; bit >= 0; --bit)
	{
		if (!(offset & (1 << bit)))
			return (dsw >> bit) & 1;
	}

	return 0;
}

static DRIVER_INIT( aliencr )
{
	flipper_buttons = 1;
}

static DRIVER_INIT( paclandp )
{
	jump_on_up = 1;
}

static MACHINE_RESET( uapce )
{
	jamma_if_control_latch = 0;
	joystick_data_select = 0;
}

static ADDRESS_MAP_START( pce_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000000, 0x09ffff) AM_ROM
	AM_RANGE(0x1f0000, 0x1f1fff) AM_RAM AM_MIRROR(0x6000)
	AM_RANGE(0x1fe000, 0x1fe3ff) AM_READWRITE(vdc_r, vdc_w)
	AM_RANGE(0x1fe400, 0x1fe7ff) AM_READWRITE(vce_r, vce_w)
	AM_RANGE(0x1fe800, 0x1febff) AM_READWRITE(C6280_r, C6280_0_w)
	AM_RANGE(0x1fec00, 0x1fefff) AM_READWRITE(H6280_timer_r, H6280_timer_w)
	AM_RANGE(0x1ff000, 0x1ff3ff) AM_READWRITE(pce_joystick_r, pce_joystick_w)
	AM_RANGE(0x1ff400, 0x1ff7ff) AM_READWRITE(H6280_irq_status_r, H6280_irq_status_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pce_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x03) AM_READWRITE(vdc_r, vdc_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x17ff) AM_WRITE(jamma_if_control_latch_w)
	AM_RANGE(0x1800, 0x1fff) AM_READ(jamma_if_read_dsw)
	AM_RANGE(0x2000, 0x27ff) AM_READ(jamma_if_read_coin)
	AM_RANGE(0x2800, 0x2fff) AM_READ(jamma_if_control_latch_r)
ADDRESS_MAP_END

INPUT_PORTS_START( uapce )
	PORT_START_TAG("JOY")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Button I") PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Button II") PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Select") PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("P1 Run") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x10, "Time" )
	PORT_DIPSETTING(    0x00, "Untimed Play" )
	PORT_DIPSETTING(    0x08, "1 minute Timed Play" )
	PORT_DIPSETTING(    0x10, "3 minute Timed Play" )
	PORT_DIPSETTING(    0x18, "5 minute Timed Play" )
	PORT_DIPSETTING(    0x20, "7 minute Timed Play" )
	PORT_DIPSETTING(    0x28, "10 minute Timed Play" )
	PORT_DIPSETTING(    0x30, "12 minute Timed Play" )
	PORT_DIPSETTING(    0x38, "20 minute Timed Play" )
	PORT_DIPNAME( 0x40, 0x40, "Buy-In Feature" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( aliencr )
	PORT_START_TAG("JOY")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Select") PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("P1 Run") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START_TAG("FLIP")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Left Flipper")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("P1 Right Flipper")

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x10, "Time" )
	PORT_DIPSETTING(    0x00, "Untimed Play" )
	PORT_DIPSETTING(    0x08, "1 minute Timed Play" )
	PORT_DIPSETTING(    0x10, "3 minute Timed Play" )
	PORT_DIPSETTING(    0x18, "5 minute Timed Play" )
	PORT_DIPSETTING(    0x20, "7 minute Timed Play" )
	PORT_DIPSETTING(    0x28, "10 minute Timed Play" )
	PORT_DIPSETTING(    0x30, "12 minute Timed Play" )
	PORT_DIPSETTING(    0x38, "20 minute Timed Play" )
	PORT_DIPNAME( 0x40, 0x40, "Buy-In Feature" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( paclandp )
	PORT_START_TAG("JOY")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("P1 Select") PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("P1 Run") PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("P1 Jump") PORT_PLAYER(1)
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x10, "Time" )
	PORT_DIPSETTING(    0x00, "Untimed Play" )
	PORT_DIPSETTING(    0x08, "1 minute Timed Play" )
	PORT_DIPSETTING(    0x10, "3 minute Timed Play" )
	PORT_DIPSETTING(    0x18, "5 minute Timed Play" )
	PORT_DIPSETTING(    0x20, "7 minute Timed Play" )
	PORT_DIPSETTING(    0x28, "10 minute Timed Play" )
	PORT_DIPSETTING(    0x30, "12 minute Timed Play" )
	PORT_DIPSETTING(    0x38, "20 minute Timed Play" )
	PORT_DIPNAME( 0x40, 0x40, "Buy-In Feature" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( uapce )
	/* basic machine hardware */
	MDRV_CPU_ADD(H6280, PCE_MAIN_CLOCK / 3)
	MDRV_CPU_PROGRAM_MAP(pce_mem, 0)
	MDRV_CPU_IO_MAP(pce_io, 0)
	MDRV_CPU_VBLANK_INT(uapce_interrupt, VDC_LPF)

	MDRV_CPU_ADD(Z80, 1400000)
	MDRV_CPU_PROGRAM_MAP(z80_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET(uapce)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(45*8, 32*8)
	MDRV_VISIBLE_AREA(52, 307, 8, 240)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(512)

	MDRV_VIDEO_START(pce)
	MDRV_VIDEO_UPDATE(pce)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(C6280, PCE_MAIN_CLOCK / 6)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(1, "right", 1.00)
MACHINE_DRIVER_END

ROM_START( blazlaz )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )
	ROM_LOAD( "ic1.bin", 0x000000, 0x020000, CRC(c86d44fe) SHA1(070512918e4d305db48bbda374eaff2d121909c5) )
	ROM_LOAD( "ic2.bin", 0x020000, 0x020000, CRC(fb813309) SHA1(60d1ea45717a04d776b6837377467e81431f9bc6) )
	ROM_LOAD( "ic3.bin", 0x080000, 0x020000, CRC(d30a2ecf) SHA1(8328b303e2ffeb694b472719e59044d41471725f) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "u1.bin", 0x0000, 0x800, CRC(f5e538a9) SHA1(19ac9525c9ad6bea1789cc9e63cdb7fe949867d9) )
ROM_END

ROM_START( keith )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )
	ROM_LOAD( "keith.ic1", 0x000000, 0x020000, BAD_DUMP CRC(d49d4e0d) SHA1(4ff8e7025dd1023e54a0f5bac52d7cecb7cb1430) )
	ROM_LOAD( "keith.ic2", 0x020000, 0x020000, BAD_DUMP CRC(133e5c8b) SHA1(8ebacd7b3887d10b28d8c672396c8850e8c0cdaf) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "u1.bin", 0x0000, 0x800, CRC(f5e538a9) SHA1(19ac9525c9ad6bea1789cc9e63cdb7fe949867d9) )
ROM_END

ROM_START( aliencr )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )
	ROM_LOAD( "aliencr.bin", 0x000000, 0x040000, CRC(60edf4e1) SHA1(a1e6ad82b66a82c25e0a9e25fb8be370f49c8c2d) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "u1.bin", 0x0000, 0x800, CRC(f5e538a9) SHA1(19ac9525c9ad6bea1789cc9e63cdb7fe949867d9) )
ROM_END

ROM_START( paclandp )
	ROM_REGION( 0x0a0000, REGION_CPU1, 0 )
	ROM_LOAD( "paclandp.bin", 0x000000, 0x040000, CRC(14fad3ba) SHA1(fc0166da82ed3cf4a4e06fc6c73fd3184ba8bb3b) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "u1.bin", 0x0000, 0x800, CRC(f5e538a9) SHA1(19ac9525c9ad6bea1789cc9e63cdb7fe949867d9) )
ROM_END

GAME( 1989, blazlaz,  0, uapce, uapce, 0, ROT0, "Hudson Soft", "Blazing Lazers (United Amusements PC Engine)", GAME_IMPERFECT_SOUND )
GAME( 1989, keith,    0, uapce, uapce, 0, ROT0, "Hudson Soft", "Keith Courage In Alpha Zones (United Amusements PC Engine)", GAME_IMPERFECT_SOUND )
GAME( 1989, aliencr,  0, uapce, aliencr, aliencr, ROT0, "Hudson Soft", "Alien Crush (United Amusements PC Engine)", GAME_IMPERFECT_SOUND )
GAME( 1989, paclandp, 0, uapce, paclandp, paclandp, ROT0, "Namco", "Pac-Land (United Amusements PC Engine)", GAME_IMPERFECT_SOUND )
