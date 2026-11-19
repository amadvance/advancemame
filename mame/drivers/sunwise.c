// license:BSD-3-Clause
// copyright-holders:Quench, Yochizo, David Haywood
/***************************************************************************

Name        Board No      Maker         Game name
----------------------------------------------------------------------------
pwrkick     SW931201-1    Sunwise       Power Kick
burgkids    SW931201-1    Sunwise       Burger Kids
pyutakun    SW931201-1    Sunwise       Pyuuta-kun
othldrby    S951060-VGP   Sunwise       Othello Derby
monkichi    S951060-VGP   Sunwise       Monkichicchi no Fuwafuwa Puzzle  (has a subboard instead of mask ROMs)

The medal games run in a cabinet named "JOYM".

Notes on Power Kick coin inputs:
- The 10 yen input is "Key In" according to the bookkeeping screen, but is
  an otherwise normal coin input with a counter and a lockout (sharing the
  latter with the "medal" coin).
- The 100 yen input never adds any credits except in "Coin Function Check,"
  instead dispensing its value into the hopper immediately.

To reset the NVRAM in Othello Derby, hold P1 Button 1 down while booting.

Differences from MAME:
- The video chip is emulated by the simplified othldrby video code instead
  of the full GP9001 emulation.
- The calendar chip reads the host clock and ignores writes.

Notes:
- Sprite/tile priorities are NOT orthogonal to sprite/sprite priorities:
  sprites with a higher priority appear over sprites with a lower priority,
  regardless of their order in the sprite list. Therefore, the current
  implementation is correct.

***************************************************************************/

#include "driver.h"
#include "sound/okim6295.h"
#include "machine/ticket.h"
#include <time.h>


WRITE16_HANDLER( othldrby_videoram_addr_w );
READ16_HANDLER( othldrby_videoram_r );
WRITE16_HANDLER( othldrby_videoram_w );
WRITE16_HANDLER( othldrby_vreg_addr_w );
WRITE16_HANDLER( othldrby_vreg_w );

VIDEO_START( othldrby );
VIDEO_EOF( othldrby );
VIDEO_UPDATE( othldrby );



static READ16_HANDLER( pip )
{
	static int toggle = 0xff;
	return toggle ^= 1;
}

static READ16_HANDLER( pap )
{
	return rand();
}


static WRITE16_HANDLER( oki_bankswitch_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static WRITE16_HANDLER( coinctrl_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,data & 1);
		coin_counter_w(1,data & 2);
		coin_lockout_w(0,~data & 4);
		coin_lockout_w(1,~data & 8);
	}
}

static WRITE16_HANDLER( pwrkick_coin_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0, (data >> 1) & 1); /* medal */
		coin_counter_w(1, (data >> 3) & 1); /* 10 yen */
		coin_counter_w(2, data & 1); /* 100 yen */
		ticket_dispenser_w(0, data & 0x80);
	}
}

static WRITE16_HANDLER( pwrkick_coin_lockout_w )
{
	if (ACCESSING_LSB)
	{
		coin_lockout_w(0, ~data & 4);
		coin_lockout_w(1, ~data & 4);
		coin_lockout_w(2, ~data & 2);
	}
}

static READ16_HANDLER( pwrkick_in2_r )
{
	return (readinputport(3) & ~0x08) | ((ticket_dispenser_r(0) >> 4) & 0x08);
}

static READ16_HANDLER( monkichi_oki_status_r )
{
	/* the game polls this status waiting for a state that never happens */
	return 0x00;
}

static WRITE16_HANDLER( calendar_w )
{
}

static READ16_HANDLER( calendar_r )
{
	time_t ltime;
	struct tm *today;


	time(&ltime);
	today = localtime(&ltime);

	switch (offset)
	{
		case 0:
			return ((today->tm_sec/10)<<4) + (today->tm_sec%10);
		case 1:
			return ((today->tm_min/10)<<4) + (today->tm_min%10);
		case 2:
			return ((today->tm_hour/10)<<4) + (today->tm_hour%10);
		case 3:
			return today->tm_wday;
		case 4:
			return ((today->tm_mday/10)<<4) + (today->tm_mday%10);
		case 5:
			return (today->tm_mon + 1);
		case 6:
			return (((today->tm_year%100)/10)<<4) + (today->tm_year%10);
		case 7:
		default:
			return 0;	/* status? the other registers are read only when bit 0 is clear */
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_READ(calendar_r)
	AM_RANGE(0x300004, 0x300007) AM_READ(othldrby_videoram_r)
	AM_RANGE(0x30000c, 0x30000d) AM_READ(pip)	// vblank?
	AM_RANGE(0x400000, 0x400fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x600000, 0x600001) AM_READ(OKIM6295_status_0_lsb_r)
	AM_RANGE(0x700000, 0x700001) AM_READ(pap)	// scanline???
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_0_word_r)
	AM_RANGE(0x700008, 0x700009) AM_READ(input_port_1_word_r)
	AM_RANGE(0x70000c, 0x70000d) AM_READ(input_port_2_word_r)
	AM_RANGE(0x700010, 0x700011) AM_READ(input_port_3_word_r)
	AM_RANGE(0x70001c, 0x70001d) AM_READ(input_port_4_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_WRITE(calendar_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(othldrby_videoram_addr_w)
	AM_RANGE(0x300004, 0x300007) AM_WRITE(othldrby_videoram_w)
	AM_RANGE(0x300008, 0x300009) AM_WRITE(othldrby_vreg_addr_w)
	AM_RANGE(0x30000c, 0x30000f) AM_WRITE(othldrby_vreg_w)
	AM_RANGE(0x400000, 0x400fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700030, 0x700031) AM_WRITE(oki_bankswitch_w)
	AM_RANGE(0x700034, 0x700035) AM_WRITE(coinctrl_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( monkichi_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_READ(calendar_r)
	AM_RANGE(0x300004, 0x300007) AM_READ(othldrby_videoram_r)
	AM_RANGE(0x30000c, 0x30000d) AM_READ(pip)	// vblank?
	AM_RANGE(0x400000, 0x400fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x600000, 0x600001) AM_READ(monkichi_oki_status_r)
	AM_RANGE(0x700000, 0x700001) AM_READ(pap)	// scanline???
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_0_word_r)
	AM_RANGE(0x700008, 0x700009) AM_READ(input_port_1_word_r)
	AM_RANGE(0x70000c, 0x70000d) AM_READ(input_port_2_word_r)
	AM_RANGE(0x700010, 0x700011) AM_READ(input_port_3_word_r)
	AM_RANGE(0x70001c, 0x70001d) AM_READ(input_port_4_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( monkichi_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x11ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_WRITE(calendar_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(othldrby_videoram_addr_w)
	AM_RANGE(0x300004, 0x300007) AM_WRITE(othldrby_videoram_w)
	AM_RANGE(0x300008, 0x300009) AM_WRITE(othldrby_vreg_addr_w)
	AM_RANGE(0x30000c, 0x30000f) AM_WRITE(othldrby_vreg_w)
	AM_RANGE(0x400000, 0x400fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700030, 0x700031) AM_WRITE(oki_bankswitch_w)
	AM_RANGE(0x700034, 0x700035) AM_WRITE(coinctrl_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pwrkick_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x20000f) AM_READ(calendar_r)
	AM_RANGE(0x300004, 0x300007) AM_READ(othldrby_videoram_r)
	AM_RANGE(0x30000c, 0x30000d) AM_READ(pip)	// vblank?
	AM_RANGE(0x400000, 0x400fff) AM_READ(paletteram16_word_r)
	AM_RANGE(0x600000, 0x600001) AM_READ(OKIM6295_status_0_lsb_r)
	AM_RANGE(0x700000, 0x700001) AM_READ(pap)	// scanline???
	AM_RANGE(0x700004, 0x700005) AM_READ(input_port_0_word_r)
	AM_RANGE(0x700008, 0x700009) AM_READ(input_port_1_word_r)
	AM_RANGE(0x70000c, 0x70000d) AM_READ(input_port_2_word_r)
	AM_RANGE(0x700014, 0x700015) AM_READ(pwrkick_in2_r)
	AM_RANGE(0x700018, 0x700019) AM_READ(input_port_5_word_r)
	AM_RANGE(0x70001c, 0x70001d) AM_READ(input_port_4_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pwrkick_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x200000, 0x20000f) AM_WRITE(calendar_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(othldrby_videoram_addr_w)
	AM_RANGE(0x300004, 0x300007) AM_WRITE(othldrby_videoram_w)
	AM_RANGE(0x300008, 0x300009) AM_WRITE(othldrby_vreg_addr_w)
	AM_RANGE(0x30000c, 0x30000f) AM_WRITE(othldrby_vreg_w)
	AM_RANGE(0x400000, 0x400fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(OKIM6295_data_0_lsb_w)
	AM_RANGE(0x700030, 0x700031) AM_WRITE(oki_bankswitch_w)
	AM_RANGE(0x700034, 0x700035) AM_WRITE(pwrkick_coin_w)
	AM_RANGE(0x700038, 0x700039) AM_WRITE(pwrkick_coin_lockout_w)
ADDRESS_MAP_END



INPUT_PORTS_START( othldrby )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* TEST */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( monkichi )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Time" ) /* based on first level */
	PORT_DIPSETTING(    0x00, "5 Minutes" )
	PORT_DIPSETTING(    0x02, "3 Minutes" )
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

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* TEST */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

#define SUNWISE_DSWA_COMMON \
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( Normal ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Hard ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( Very_Hard ) ) \
	PORT_DIPNAME( 0x5c, 0x00, "Payout" ) \
	PORT_DIPSETTING(    0x00, "110" ) \
	PORT_DIPSETTING(    0x04, "105" ) \
	PORT_DIPSETTING(    0x08, "100" ) \
	PORT_DIPSETTING(    0x0c, "95" ) \
	PORT_DIPSETTING(    0x10, "90" ) \
	PORT_DIPSETTING(    0x14, "85" ) \
	PORT_DIPSETTING(    0x18, "80" ) \
	PORT_DIPSETTING(    0x1c, "75" ) \
	PORT_DIPSETTING(    0x40, "70" ) \
	PORT_DIPSETTING(    0x44, "65" ) \
	PORT_DIPSETTING(    0x48, "60" ) \
	PORT_DIPSETTING(    0x4c, "55" ) \
	PORT_DIPSETTING(    0x50, "50" ) \
	PORT_DIPSETTING(    0x54, "45" ) \
	PORT_DIPSETTING(    0x58, "40" ) \
	PORT_DIPSETTING(    0x5c, "35" ) \
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define SUNWISE_PLAY_CREDIT \
	PORT_DIPNAME( 0x03, 0x00, "Play Credit" ) \
	PORT_DIPSETTING(    0x00, "10 Yen" ) \
	PORT_DIPSETTING(    0x01, "20 Yen" ) \
	PORT_DIPSETTING(    0x02, "30 Yen" ) \
	PORT_DIPSETTING(    0x03, "40 Yen" )

#define SUNWISE_SYSTEM_INPUTS \
	PORT_START /* IN2 */ \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_NAME("Coin 2 (10 Yen)") \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode ) ) PORT_TOGGLE \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("Down") \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* hopper status */ \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE3 ) PORT_NAME("Memory Reset") \
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	\
	PORT_START /* SYS */ \
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_NAME("Coin Exchange (100 Yen)") \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE4 ) PORT_NAME("Book") \
	PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 ) PORT_NAME("Attendant Key") \
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_NAME("Coin 1 (Medal)") \
	\
	PORT_START /* DSWC */ \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_START( pwrkick )
	PORT_START /* DSWA */
	SUNWISE_DSWA_COMMON
	PORT_DIPNAME( 0x80, 0x00, "Diagnostic" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* DSWB */
	SUNWISE_PLAY_CREDIT
	PORT_DIPNAME( 0x0c, 0x00, "Coin Exchange" )
	PORT_DIPSETTING(    0x00, "12" )
	PORT_DIPSETTING(    0x04, "10" )
	PORT_DIPSETTING(    0x08, "6" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, "Shot" )
	PORT_DIPSETTING(    0x20, "Auto" )
	PORT_DIPSETTING(    0x30, "S-Manual" )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("Left")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("Center")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("Right")
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	SUNWISE_SYSTEM_INPUTS
INPUT_PORTS_END

INPUT_PORTS_START( burgkids )
	PORT_START /* DSWA */
	SUNWISE_DSWA_COMMON
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* DSWB */
	SUNWISE_PLAY_CREDIT
	PORT_DIPNAME( 0x1c, 0x00, "Coin Exchange" )
	PORT_DIPSETTING(    0x00, "12" )
	PORT_DIPSETTING(    0x04, "11" )
	PORT_DIPSETTING(    0x08, "10" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x14, "4" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x1c, DEF_STR( Off ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("1")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("2")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("3")
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	SUNWISE_SYSTEM_INPUTS
INPUT_PORTS_END

INPUT_PORTS_START( pyutakun )
	PORT_START /* DSWA */
	SUNWISE_DSWA_COMMON
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* DSWB */
	SUNWISE_PLAY_CREDIT
	PORT_DIPNAME( 0x1c, 0x00, "Coin Exchange" )
	PORT_DIPSETTING(    0x00, "12" )
	PORT_DIPSETTING(    0x04, "11" )
	PORT_DIPSETTING(    0x08, "10" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x14, "4" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x1c, DEF_STR( Off ) )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	SUNWISE_SYSTEM_INPUTS
INPUT_PORTS_END



static const gfx_layout spritelayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, RGN_FRAC(0,2)+8, RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, RGN_FRAC(0,2)+8, RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	16*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0, 0x80 },
	{ REGION_GFX1, 0, &tilelayout,   0, 0x80 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( othldrby )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(othldrby)
	MDRV_VIDEO_EOF(othldrby)
	MDRV_VIDEO_UPDATE(othldrby)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( monkichi )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(monkichi_readmem,monkichi_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(othldrby)
	MDRV_VIDEO_EOF(othldrby)
	MDRV_VIDEO_UPDATE(othldrby)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pwrkick )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(pwrkick_readmem,pwrkick_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-12)*8-1, 1*8, 31*8-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(othldrby)
	MDRV_VIDEO_EOF(othldrby)
	MDRV_VIDEO_UPDATE(othldrby)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 16000000/4/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static DRIVER_INIT( pwrkick )
{
	ticket_dispenser_init(50, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);
}



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pwrkick )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "1.u61",        0x00000, 0x80000, CRC(118b5899) SHA1(7a1637a63eb17e3892d79aede5730013a1dc00f9) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2.u26",        0x000000, 0x080000, CRC(a190eaea) SHA1(2c7b8c8026873e0f591fbcbc2e72b196ef84e162) )
	ROM_LOAD( "3.u27",        0x080000, 0x080000, CRC(0b81e038) SHA1(8376617ae519a8ef604f20b26e941aa5b8066602) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "4.u33",        0x00000, 0x80000, CRC(3ab742f1) SHA1(ce8ca02ca57fd77872e421ce601afd017d3518a0) )
ROM_END

ROM_START( burgkids )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "ffk1.u61",     0x00000, 0x80000, CRC(ac96cb0d) SHA1(2ce5c06d61f3ff18b222619e41d09e46d44c5bab) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ffk2.u26",     0x000000, 0x080000, CRC(09f7b0ae) SHA1(f340f27a601ff89f143398263d822b8f340eea6e) )
	ROM_LOAD( "ffk3.u27",     0x080000, 0x080000, CRC(63c761bc) SHA1(f0ee1dc6aaeacff23e55d072102b814c7ef30550) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ffk4.u33",     0x00000, 0x80000, CRC(3b032d4f) SHA1(69056bf205aadf6c9fee56ce396b11a5187caa03) )
ROM_END

ROM_START( pyutakun )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sunwise_p1.u61", 0x00000, 0x80000, CRC(00907713) SHA1(0822044bcf476b3e8aaba752e503ee79459b34ed) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sunwise_p2.u26", 0x00000, 0x80000, CRC(180b8b13) SHA1(4a317bd0825f4e4383293220e775c0f807cdd80f) )
	ROM_LOAD( "sunwise_p3.u27", 0x80000, 0x80000, CRC(a23ccb8e) SHA1(5f2f23fa86817ff491058123d775ffebd7e98dee) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "sunwise_p4.u33", 0x00000, 0x40000, CRC(b1c5a8bc) SHA1(aa18165a9214f9ed0969da073e4b6092be3c5c1a) )
ROM_END

ROM_START( othldrby )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "db0.1",        0x00000, 0x80000, CRC(6b4008d3) SHA1(4cf838c47563ba482be8364b2e115569a4a06c83) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db0-r2",       0x000000, 0x200000, CRC(4efff265) SHA1(4cd239ff42f532495946cb52bd1fee412f84e192) )
	ROM_LOAD( "db0-r3",       0x200000, 0x200000, CRC(5c142b38) SHA1(5466a8b061a0f2545493de0f96fd4387beea276a) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "db0.4",        0x00000, 0x80000, CRC(a9701868) SHA1(9ee89556666d358e8d3915622573b3ba660048b8) )
ROM_END

ROM_START( monkichi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "rom1.u61",     0x00000, 0x80000, CRC(0d5085f5) SHA1(11885bc501b12aafd10a83599d6036e259383620) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom21.c4",     0x000000, 0x080000, CRC(eb0c113d) SHA1(ecbbb94c9bdddc3f5c76d478997d895a2d91024a) )
	ROM_LOAD( "rom22.c3",     0x080000, 0x080000, CRC(643f02c8) SHA1(555594332330c3e8bc01af2c757b7e4728e64589) )
	ROM_LOAD( "rom31.c6",     0x100000, 0x080000, CRC(8757ce90) SHA1(cffa4e16d09d2205939b3a26f15a410c7548c8ec) )
	ROM_LOAD( "rom32.c5",     0x180000, 0x080000, CRC(c5985dba) SHA1(b20c8429f3314dfcb8f8ce66647f4ef0bff9b030) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "rom4.u33",     0x00000, 0x80000, CRC(b148f4d0) SHA1(e53aef6460f1be730396bd3940aeaead215a8378) )
ROM_END


GAME( 1994, pwrkick,  0, pwrkick,  pwrkick,  pwrkick, ROT0, "Sunwise", "Power Kick (Japan)", 0 )
GAME( 1995, burgkids, 0, pwrkick,  burgkids, pwrkick, ROT0, "Sunwise", "Burger Kids (Japan)", 0 )
GAME( 1995, pyutakun, 0, pwrkick,  pyutakun, pwrkick, ROT0, "Sunwise", "Pyuuta-kun (Japan)", 0 )
GAME( 1995, othldrby, 0, othldrby, othldrby, 0,       ROT0, "Sunwise", "Othello Derby (Japan)", 0 )
GAME( 1995, monkichi, 0, monkichi, monkichi, 0,       ROT0, "Sunwise", "Monkichicchi no Fuwafuwa Puzzle (Japan, prototype?)", 0 )
