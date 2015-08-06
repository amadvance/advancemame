/*

    INTER GAME POKER might be another game than a poker

    1990 By Inter Games

    board info :

    CPU G65SC02P
    MC68B45P
    AY3-8910
    MC6821P x2

    RAM

    6116
    KM6264AL-10

    Crystal : 16.000 Mhz

    There are two software which run on the same board :

    IG1.RUN,JN1.CH1,JN1.CH2 (ver 9044???)

    IGxx.RUN,JNxx.CH1,JNxx.CH2 this one has a NVram instead of 6116, ram = DS1220Y

    ---

    Bonus Card is a Bulgarian software for Jolly Poker Board

    CPU MC6845P

    Sound CPU YM2149F

    Other MC6821P

    Crystal 16.000

    ---

    Magic Card is a Bulgarian software for Jolly Poker Board

    Warning the board has a special CPU with CM602 on it (used by original game)

    CPU MC6845P

    Sound CPU YM2149F

    Other MC6821P

    Crystal 16.000

    ---

    DATTL
    http://www.dattl.at/

    TAB-Austria
    http://www.tab.at/TABUE/English.html

    Fun World
    http://www.funworld.com/default_eng.htm

    Impera
    http://www.impera.at/var/cccpage/home/set_language/en/end/index.html

    ---

    AGEMAME Driver by Curt Coder, with a lot of help from Peter Trauner


    NOTES:

    - It takes 48 seconds for the bonuscrd/poker4/poker8 games to boot up
      after the initial screen is displayed!!!
    - The default DIP switch settings must be used when first booting up the games
      to allow them to complete the NVRAM initialization.
    - jollycrd/jollypkr: Start game, press and hold Service1 & Service2, press
      reset (F3), release Service1/2 and press reset (F3) again. Now the NVRAM
      has been initialized.

Any fixes for this driver should be forwarded to the AGEMAME forum at (http://www.mameworld.info)


    *** Extra hardware info ***    (added by Roberto Fresca)

    Jolly Poker, TAB Austria
    ------------------------

    Pinouts:
    --------

    X1-1   GND                    X1-A   GND
    X1-2   GND                    X1-B   GND
    X1-3   GND                    X1-C   GND
    X1-4   +5V                    X1-D   +5V
    X1-5   +12V                   X1-E   +12V
    X1-6   NC                     X1-F   NC
    X1-7   NC                     X1-G   NC
    X1-8   NC                     X1-H   NC
    X1-9   NC                     X1-I   Coin 1
    X1-10  Coin 2                 X1-J   Pay Out SW
    X1-11  Hold 3                 X1-K   NC
    X1-12  NC                     X1-L   GND
    X1-13  Hold 4                 X1-M   Remote
    X1-14  Bookkeeping SW         X1-N   GND
    X1-15  Hold 2                 X1-O   Cancel
    X1-16  Hold 1                 X1-P   Hold 5
    X1-17  Start                  X1-Q
    X1-18  Hopper Out             X1-R
    X1-19  Red                    X1-S   Green
    X1-20  Blue                   X1-T   Sync
    X1-21  GND                    X1-U   Speaker GND
    X1-22  Speaker +              X1-V   Management SW


    X2-1   GND                    X2-A   GND
    X2-2   NC                     X2-B   NC
    X2-3   Start                  X2-C   NC
    X2-4   NC                     X2-D   NC
    X2-5   NC                     X2-E   NC
    X2-6   Lamp Start             X2-F   Lamp Hold 1+3
    X2-7   Lamp Hold 2            X2-G   Lamp Hold 4
    X2-8   Lamp Hold 5            X2-H   Lamp Cancel
    X2-9   NC                     X2-I   NC
    X2-10  Counter In             X2-J   Counter Out
    X2-11  Hopper Count           X2-K   Hopper Drive
    X2-12  Counter Remote         X2-L
    X2-13                         X2-M
    X2-14                         X2-N
    X2-15  NC                     X2-O
    X2-16                         X2-P
    X2-17                         X2-Q   Coin Counter
    X2-18                         X2-R


    Dip Switch:
    -----------

         ON                 OFF

    1    Hopper             Manual Payout SW
    2    Auto Hold          No Auto Hold
    3    Joker              Without Joker
    4    Dattl Insert       TAB Insert
    5    5 Points/Coin      10 Points/Coin     Coin 1
    6    5 Points/Coin      10 Points/Coin     Coin 2
    7    10 Points/Pulse    100 Points/Pulse   Remote
    8    Play               Keyboard Test

    -------------------------------------------------


    Royal Card (set 1)
    ------------------

    - 1x HM6264LP
    - 1x HD4650SP
    - 1x HM6116LP
    - 1x R65C02P2 (main)
    - 1x WB5300 (labeled YM8910)(sound)
    - 1x EF6821P
    - 1x oscillator 16.000 MHz

    - 1x D27256 (1)
    - 1x S27C256 (2)
    - 1x TMS27C256 (r2)
    - 2x PEEL18CV8 (1 protected)
    - 1x PALCE16V8H (protected)
    - 1x PROM N82S147AN

    - 1x 8 switches dip
    - 1x 22x2 edge connector
    - 1x 18x2 edge connector
    - 1x trimmer (volume)

    PEEL18CV8 fuse map:

    Device    PEEL18CV8
    *
    QF2696*
    F0*
    L000000 111011111111111111111111110111111111*
    L000036 110111110111111111111111111111111111*
    L000072 000000000000000000000000000000000000*
    L000108 000000000000000000000000000000000000*
    L000144 000000000000000000000000000000000000*
    L000180 000000000000000000000000000000000000*
    L000216 000000000000000000000000000000000000*
    L000252 000000000000000000000000000000000000*
    L000288 111011111111110111111111111111111111*
    L000324 110111111111111111110111111111111111*
    L000360 000000000000000000000000000000000000*
    L000396 000000000000000000000000000000000000*
    L000432 000000000000000000000000000000000000*
    L000468 000000000000000000000000000000000000*
    L000504 000000000000000000000000000000000000*
    L000540 000000000000000000000000000000000000*
    L000576 111011111111111111011111111111111111*
    L000612 110111111111111111111111111101111111*
    L000648 000000000000000000000000000000000000*
    L000684 000000000000000000000000000000000000*
    L000720 000000000000000000000000000000000000*
    L000756 000000000000000000000000000000000000*
    L000792 000000000000000000000000000000000000*
    L000828 000000000000000000000000000000000000*
    L000864 111011111111111111111101111111111111*
    L000900 110111111111011111111111111111111111*
    L000936 000000000000000000000000000000000000*
    L000972 000000000000000000000000000000000000*
    L001008 000000000000000000000000000000000000*
    L001044 000000000000000000000000000000000000*
    L001080 000000000000000000000000000000000000*
    L001116 000000000000000000000000000000000000*
    L001152 111011111111111111111111111111111111*
    L001188 111101111111111111111111111111111111*
    L001224 000000000000000000000000000000000000*
    L001260 000000000000000000000000000000000000*
    L001296 000000000000000000000000000000000000*
    L001332 000000000000000000000000000000000000*
    L001368 000000000000000000000000000000000000*
    L001404 000000000000000000000000000000000000*
    L001440 111011111111111111111111111111011111*
    L001476 110111111111111101111111111111111111*
    L001512 000000000000000000000000000000000000*
    L001548 000000000000000000000000000000000000*
    L001584 000000000000000000000000000000000000*
    L001620 000000000000000000000000000000000000*
    L001656 000000000000000000000000000000000000*
    L001692 000000000000000000000000000000000000*
    L001728 111011111111111111111111111111111101*
    L001764 110111111111111111111111011111111111*
    L001800 000000000000000000000000000000000000*
    L001836 000000000000000000000000000000000000*
    L001872 000000000000000000000000000000000000*
    L001908 000000000000000000000000000000000000*
    L001944 000000000000000000000000000000000000*
    L001980 000000000000000000000000000000000000*
    L002016 111011111111111111111111111111111111*
    L002052 111111111111111111111111111111110111*
    L002088 000000000000000000000000000000000000*
    L002124 000000000000000000000000000000000000*
    L002160 000000000000000000000000000000000000*
    L002196 000000000000000000000000000000000000*
    L002232 000000000000000000000000000000000000*
    L002268 000000000000000000000000000000000000*
    L002304 111111111111111111111111111111111111*
    L002340 111111111111111111111111111111111111*
    L002376 000000000000000000000000000000000000*
    L002412 000000000000000000000000000000000000*
    L002448 000000000000000000000000000000000000*
    L002484 000000000000000000000000000000000000*
    L002520 000000000000000000000000000000000000*
    L002556 000000000000000000000000000000000000*
    L002592 000000000000000000000000000000000000*
    L002628 000000000000000000000000000000000000*
    L002664 01000100010001000100010001000100*
    C4E67*


    Royal Card (set 2)
    ------------------

    - CPU 1x R65C02P2 (main)
    - 1x MC68B45P (CRT controller)
    - 2x MC68B21CP (Peripheral interface adapter)
    - 1x oscillator 16.000MHz
    - ROMs 3x TMS 27C512
    - 1x PALCE16V8H
    - 1x prom AM27S29APC

    Note 1x 28x2 connector (maybe NOT jamma)
    1x 10x2 connector
    1x 3 legs connector (solder side)
    1x 8 dipswitches
    1x trimmer (volume)


    *** Extra Notes by Roberto Fresca ***

    - Added Cuore Uno, Elephant Family and Royal Card.
    - Added some clones.
    - Cleaned up and renamed all sets. Made parent-clone relationship.
    - Corrected CPU freq (2 MHz) in cuoreuno and elephfam (both have R65c02P2).
      (I suspect more games must have their CPU running at 2 MHz).
    - Corrected videoram and colorram offset in cuoreuno and elephfam.
    - To initialize the NVRAM in cuoreuno and elephfam:
      Start game, press and hold Service1 & Service2, press reset (F3),
      release Service1 & Service2 and press reset (F3) again.

*/

#include "driver.h"
#include "sound/ay8910.h"
#include "vidhrdw/crtc6845.h"
#include "machine/6821pia.h"

static tilemap *bg_tilemap;

/* vidhrdw */

WRITE8_HANDLER( funworld_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE8_HANDLER( funworld_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int offs = tile_index;
	int attr = videoram[offs] + (colorram[offs] << 8);
	int code = attr & 0xfff;
	int color = attr >> 12;

	SET_TILE_INFO(0, code, color, 0)
}

static void jollypkr_get_bg_tile_info(int tile_index)
{
	int offs = tile_index;
	int attr = videoram[offs] + (colorram[offs] << 8);
	int code = attr & 0x1fff;
	int color = attr >> 12;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START(funworld)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 4, 8, 96, 29);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_START(jollypkr)
{
	bg_tilemap = tilemap_create(jollypkr_get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 4, 8, 96, 29);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_START(magiccrd)
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 4, 8, 112, 34);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE(funworld)
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
}

/* Read/Write Handlers */

WRITE8_HANDLER(funworld_lamp_a_w)
{
}

WRITE8_HANDLER(funworld_lamp_b_w)
{
}

/* Memory Maps */

static ADDRESS_MAP_START( funworld_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x2000, 0x2fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x3000, 0x3fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4000, 0x4000) AM_READNOP
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bonuscrd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x2000, 0x2fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x3000, 0x3fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( jollypkr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x2000, 0x2000) AM_READNOP
	AM_RANGE(0x6000, 0x6fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x7000, 0x7fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( magiccrd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x4000, 0x4fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x5fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x6000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( cuoreuno_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x0800, 0x0803) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE(0x0a00, 0x0a03) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE(0x0c00, 0x0c00) AM_READWRITE(AY8910_read_port_0_r, AY8910_control_port_0_w)
	AM_RANGE(0x0c01, 0x0c01) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0e00, 0x0e00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x0e01, 0x0e01) AM_READWRITE(crtc6845_register_r, crtc6845_register_w)
	AM_RANGE(0x6000, 0x6fff) AM_RAM AM_WRITE(funworld_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x7000, 0x7fff) AM_RAM	AM_WRITE(funworld_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( funworld )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold 1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Clear / Take")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold 5 / Stake")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold 4 / High")

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold 2 / Low")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold 3")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_CODE(KEYCODE_ENTER) PORT_NAME("Payout")

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, "Remote" )
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x02, "100 Points/Pulse" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x04, "10 Points/Coin" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x08, "10 Points/Coin" )
	PORT_DIPNAME( 0x10, 0x00, "Insert" )
	PORT_DIPSETTING(    0x00, "Dattl Insert" )
	PORT_DIPSETTING(    0x10, "TAB Insert" )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )
	PORT_DIPSETTING(    0x00, "With Joker" )    /* enables Five of a kind in jolly card/poker games. */
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )
	PORT_DIPNAME( 0x80, 0x00, "Payout" )
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

INPUT_PORTS_START( magiccrd )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Hold 1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("Clear / Take")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("Hold 5 / Stake")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("Hold 4 / High")

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Hold 2 / Low")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Hold 3")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_CODE(KEYCODE_ENTER) PORT_NAME("Payout")

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "State" )
	PORT_DIPSETTING(    0x00, "Keyboard Test" )
	PORT_DIPSETTING(    0x01, "Play" )
	PORT_DIPNAME( 0x02, 0x00, "Remote" )
	PORT_DIPSETTING(    0x00, "10 Points/Pulse" )
	PORT_DIPSETTING(    0x02, "100 Points/Pulse" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x04, "10 Points/Coin" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, "5 Points/Coin" )
	PORT_DIPSETTING(    0x08, "10 Points/Coin" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Joker" )
	PORT_DIPSETTING(    0x00, "With Joker" )
	PORT_DIPSETTING(    0x20, "Without Joker" )
	PORT_DIPNAME( 0x40, 0x00, "Hold" )
	PORT_DIPSETTING(    0x00, "Auto Hold" )
	PORT_DIPSETTING(    0x40, "No Auto Hold" )
	PORT_DIPNAME( 0x80, 0x00, "Payout" )
	PORT_DIPSETTING(    0x00, "Hopper" )
	PORT_DIPSETTING(    0x80, "Manual Payout SW" )
INPUT_PORTS_END

/* Graphics Layouts */

static const gfx_layout charlayout =
{
	4,
	8,
	0x1000,
	4,
	{ 0, 4, 0x8000*8, 0x8000*8+4 },
	{ 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*4*2
};

static const gfx_layout magiccrd_charlayout =
{
    4,
	8,
	0x2000,
	2,
	{ 0, 4 },
	{ 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*4*2
};

/* Graphics Decode Information */

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout, 0, 16 },
	{ -1 }
};

static const gfx_decode magiccrd_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &magiccrd_charlayout, 0, 16 },
	{ -1 }
};

/* PIA Interfaces */

static const pia6821_interface pia0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static const pia6821_interface pia1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, input_port_3_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

/* Sound Interface */

static struct AY8910interface ay8910_intf =
{
	0,	// portA in
	0,	// portB in
	funworld_lamp_a_w,	// portA out
	funworld_lamp_b_w	// portB out
};

/* Machine Drivers */

static MACHINE_DRIVER_START( funworld )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", M65SC02, 3000000)	// ???
	MDRV_CPU_PROGRAM_MAP(funworld_map, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(96*4, 30*8) // 124x30
	MDRV_VISIBLE_AREA(0*4, 96*4-1, 0*8, 29*8-1) // 96x29

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_VIDEO_START(funworld)
	MDRV_VIDEO_UPDATE(funworld)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 4000000)	// ???
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bonuscrd )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(bonuscrd_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jollypkr )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(jollypkr_map, 0)

	MDRV_GFXDECODE(magiccrd_gfxdecodeinfo)
	MDRV_VIDEO_START(jollypkr)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magiccrd )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_REPLACE("main", M65C02, 3000000) // ???
	MDRV_CPU_PROGRAM_MAP(magiccrd_map, 0)

	MDRV_SCREEN_SIZE(112*4, 34*8) // 123x36
	MDRV_VISIBLE_AREA(0*4, 112*4-1, 0*8, 34*8-1) // 112x34

	MDRV_GFXDECODE(magiccrd_gfxdecodeinfo)
	MDRV_VIDEO_START(magiccrd)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cuoreuno )
	MDRV_IMPORT_FROM(funworld)

	MDRV_CPU_REPLACE("main", M65C02, 2000000) // original cpu = R65C02P2 (2mhz)
	MDRV_CPU_PROGRAM_MAP(cuoreuno_map, 0)

	MDRV_GFXDECODE(magiccrd_gfxdecodeinfo)
//  MDRV_VIDEO_START(jollypkr)
	MDRV_VIDEO_START(funworld)
	MDRV_VIDEO_UPDATE(funworld)

MACHINE_DRIVER_END

/* ROMs */

ROM_START( jollycrd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jolycard.run", 0x8000, 0x8000, CRC(f531dd10) SHA1(969191fbfeff4349afef619d9241ef5186e6d57f) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jolycard.ch1", 0x0000, 0x8000, CRC(0f24f39d) SHA1(ac1f6a8a4a2a37cbc0d45c15187b33c25371bffb) )
	ROM_LOAD( "jolycard.ch2", 0x8000, 0x8000, CRC(c512b103) SHA1(1f4e78e97855afaf0332fb75e1b5571aafd01c29) )
ROM_END

ROM_START( jolycdcr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jollypkr.003", 0x8000, 0x8000, CRC(ea7340b4) SHA1(7dd468f28a488a4781521809d06db1d7917048ad) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jolycard.ch1", 0x0000, 0x8000, CRC(0f24f39d) SHA1(ac1f6a8a4a2a37cbc0d45c15187b33c25371bffb) )
	ROM_LOAD( "jolycard.ch2", 0x8000, 0x8000, CRC(c512b103) SHA1(1f4e78e97855afaf0332fb75e1b5571aafd01c29) )
ROM_END

ROM_START( jolycdit )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "jn.bin", 0x8000, 0x8000, CRC(6ae00ed0) SHA1(5921c2882aeb5eadd0e04a477fa505ad35e9d98c) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin", 0x0000, 0x8000, CRC(43bcb2df) SHA1(5022bc3a0b852a7cd433e25c3c90a720e6328261) )
	ROM_LOAD( "2.bin", 0x8000, 0x8000, CRC(46805150) SHA1(63687ac44f6ace6d8924b2629536bcc7d3979ed2) )
ROM_END

ROM_START( jolycdat )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "bonucard.cpu", 0x4000, 0x4000, CRC(da342100) SHA1(451fa6074aad19e9efd148c3d18115a20a3d344a) )
	ROM_CONTINUE(			  0xc000, 0x4000 )
	ROM_RELOAD( 			  0x8000, 0x4000 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jolycard.ch1", 0x0000, 0x8000, CRC(0f24f39d) SHA1(ac1f6a8a4a2a37cbc0d45c15187b33c25371bffb) )
	ROM_LOAD( "jolycard.ch2", 0x8000, 0x8000, CRC(c512b103) SHA1(1f4e78e97855afaf0332fb75e1b5571aafd01c29) )
ROM_END

ROM_START( jolycdab )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ig1poker.run", 0x8000, 0x8000, CRC(c96e6542) SHA1(ed6c0cf9fe8597dba9149b2225320d8d9c39219a) )
	ROM_RELOAD(				  0x4000, 0x4000 )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "jn1poker.ch1", 0x0000, 0x8000, CRC(d0a87f58) SHA1(6b7925557c4e40a1ebe52ecd14391cdd5e00b59a) )
	ROM_LOAD( "jn1poker.ch2", 0x8000, 0x8000, CRC(8d78e43d) SHA1(15c60f8e0cd88518b0dc72b92aff6d8d4b2149cf) )
ROM_END

ROM_START( bigdeal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "poker4.001", 0x8000, 0x8000, CRC(bb0198c1) SHA1(6e7d42beb5723a4368ae3788f83b448f86e5653d) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "poker4.002", 0x0000, 0x8000, CRC(5f4e12d8) SHA1(014b2364879faaf4922cdb82ee07692389f20c2d) )
	ROM_LOAD( "poker4.003", 0x8000, 0x8000, CRC(8c33a15f) SHA1(a1c8451c99a23eeffaedb21d1a1b69f54629f8ab) )
ROM_END

ROM_START( bigdealb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "poker8.003", 0x8000, 0x8000, CRC(09c93745) SHA1(a64e96ef3489bc37c2c642f49e62cfef371de6f1) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "poker4.002", 0x0000, 0x8000, CRC(5f4e12d8) SHA1(014b2364879faaf4922cdb82ee07692389f20c2d) )
	ROM_LOAD( "poker4.003", 0x8000, 0x8000, CRC(8c33a15f) SHA1(a1c8451c99a23eeffaedb21d1a1b69f54629f8ab) )
ROM_END

ROM_START( cuoreuno )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cuore1a.u2", 0x8000, 0x8000, CRC(6e112184) SHA1(283ac534fc1cb33d11bbdf3447630333f2fc957f) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cuore1b.u21", 0x0000, 0x8000, CRC(14eca2b8) SHA1(35cba415800c6cd3e6ed9946057f33510ad2bfc9) )
	ROM_LOAD( "cuore1c.u22", 0x8000, 0x8000, CRC(253fac84) SHA1(1ad104ab8e8d73df6397a840a4b26565b245d7a3) )
ROM_END

ROM_START( elephfam )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "eleph_a.u2", 0x8000, 0x10000, CRC(8392b842) SHA1(74c850c734ca8174167b2f826b9b1ac902669392) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "eleph_c.u22", 0x0000, 0x10000, CRC(4b909bf3) SHA1(a822b12126bc58af6d3f999ab2117370015a039b) )
	ROM_LOAD( "eleph_b.u21", 0x10000, 0x10000, CRC(e3612670) SHA1(beb65f7d2bd6d7bc68cfd876af51910cf6417bd0) )
ROM_END

ROM_START( royalcrd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "r2.bin", 0x8000, 0x8000, CRC(25dfe0dc) SHA1(1a857a910d0c34b6b5bfc2b6ea2e08ed8ed0cae0) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin", 0x0000, 0x8000, CRC(41f7a0b3) SHA1(9aff2b8832d2a4f868daa9849a0bfe5e44f88fc0) )
	ROM_LOAD( "2.bin", 0x8000, 0x8000, CRC(85e77661) SHA1(7d7a765c1bfcfeb9eb91d2519b22d734f20eab24) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "n82s147.bin",    0x0000, 0x0200, CRC(8bc86f48) SHA1(4c677ab9314a1f571e35104b22659e6811aeb194) )
ROM_END

ROM_START( royalcdb )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "rc.bin", 0x8000, 0x10000, CRC(8a9a6dd6) SHA1(04c3f9f17d5404ac1414c51ef8f930df54530e72) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1a.bin", 0x0000, 0x10000, CRC(8a66f22c) SHA1(67d6e8f8f5a0fd979dc498ba2cc67cf707ccdf95) )
	ROM_LOAD( "2a.bin", 0x10000, 0x10000, CRC(3af71cf8) SHA1(3a0ce0d0abebf386573c5936545dada1d3558e55) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "n82s147.bin",    0x0000, 0x0200, CRC(8bc86f48) SHA1(4c677ab9314a1f571e35104b22659e6811aeb194) )
ROM_END

ROM_START( magiccrd )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
    ROM_LOAD( "magicard.004", 0x0000, 0x8000,  CRC(f6e948b8) SHA1(7d5983015a508ab135ccbf69b7f3c526c229e3ef) ) // only last 16kbyte visible?
	ROM_LOAD( "magicard.01",  0x8000, 0x10000, CRC(c94767d4) SHA1(171ac946bdf2575f9e4a31e534a8e641597af519) ) // 1ST AND 2ND HALF IDENTICAL

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "magicard.03",  0x0000, 0x8000, CRC(733da697) SHA1(45122c64d5a371ec91cecc67b7faf179078e714d) )
	ROM_LOAD( "magicard.001", 0x8000, 0x8000, CRC(685f7a67) SHA1(fc65a9d26a5cbbe2fa08dc0f27212dae2d8bcbdc) ) // bad dump?, or sprite plane
ROM_END

/* Driver Initialization */

static DRIVER_INIT( funworld )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia1_intf);
}

/* Game Drivers */

//    YEAR  NAME      PARENT    MACHINE   INPUT     INIT            COMPANY        FULLNAME
GAME( 1985, jollycrd, 0,        funworld, funworld, funworld, ROT0, "TAB-Austria", "Jolly Card (Austria)", GAME_WRONG_COLORS )
GAME( 1993, jolycdcr, jollycrd, jollypkr, funworld, funworld, ROT0, "Soft Design", "Jolly Card (Croatia)", GAME_WRONG_COLORS )
GAME( 199?, jolycdit, jollycrd, funworld, funworld, funworld, ROT0, "bootleg?",    "Jolly Card (Italia, bad dump?)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1986, jolycdat, jollycrd, bonuscrd, funworld, funworld, ROT0, "Fun World",   "Jolly Card (Austria, Fun World)", GAME_WRONG_COLORS )
GAME( 1990, jolycdab, jollycrd, funworld, funworld, funworld, ROT0, "Inter Games", "Jolly Card (Austria, Fun World, bootleg)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1986, bigdeal,  0,        bonuscrd, funworld, funworld, ROT0, "Fun World",   "Big Deal (Hungary, set 1)", GAME_WRONG_COLORS )
GAME( 1986, bigdealb, bigdeal,  bonuscrd, funworld, funworld, ROT0, "Fun World",   "Big Deal (Hungary, set 2)", GAME_WRONG_COLORS )
GAME( 1997, cuoreuno, 0,        cuoreuno, funworld, funworld, ROT0, "bootleg?",    "Cuore Uno (Italia)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1997, elephfam, 0,        cuoreuno, funworld, funworld, ROT0, "bootleg?",    "Elephant Family (Italia)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1991, royalcrd, 0,        funworld, funworld, funworld, ROT0, "TAB-Austria", "Royal Card (Austria, set 1)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1991, royalcdb, royalcrd, funworld, funworld, funworld, ROT0, "TAB-Austria", "Royal Card (Austria, set 2)", GAME_WRONG_COLORS | GAME_NOT_WORKING )
GAME( 1996, magiccrd, 0,        magiccrd, magiccrd, funworld, ROT0, "Impera",      "Magic Card II (Bulgaria)", GAME_WRONG_COLORS )
