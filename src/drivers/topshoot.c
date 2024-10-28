/*

Sun Mixing board, looks like a hacked up Genesis clone.

 Driver by David Haywood
 Inputs by Mariusz Wojcieszek

Top Shooter - (c)1995  - older board, look more like an actual hacked cart system, has an MCU

*/

#define MASTER_CLOCK		53693100

/*

TOP SHOOTER - Sun Mixing Co. Ltd. 1995

To me it seems like an original cartridge-based arcade board
hacked to use an external ROM board and a standard JAMMA
connector, but of course, I can be wrong.


   UPPER BOARD

   _________________________________________________________
   |            ___________  ___________  _____      __    |
   | 74LS245P  |U14 Empty | |U12 ROM1  |  |IC1|      |B|   |
   | 74LS245P  |__________| |__________|  |___|            |
   | 74LS245P   ___________  ___________    _____________  |
 __|           |U13 Empty | |U11 ROM2  |   | AT89C51    |  |
 |_ J          |__________| |__________|   |____________|  |_
 |_ A           ______________________              _____  |_ J
 |_ M          | U10 MC68000P10       |             |OSC|  |_ P
 |_ M          | Motorola             |                    |_ 2
 |_ A          |______________________|            74HC00P |_
 |_  74LS245P   ______________________           ________  |
 |_            | U9 Empty             |          |HM6116L  |
 |_            |                      |          |_______| |_ J
 |_            |______________________|                    |_ P
 |_  74LS245P                           TD62oo3AP 74LS373P |_ 3
 |_                                            __________  |
 |_  74LS245P                                  |GALv20V8B| |
 |_                                    ______              |
 |_               _____                |DIPS|              |_ P
   |             |U24  |                                   |_ 1
   | 74LS245P                                              |
   | TD62oo3AP                                             |
   |                                                       |
   |_            97              ____________         _____|
     |_|_|_|_|_|_|_|_|_|_|_|_|_|_|           |_|_|_|_|


  IC1 = Surface scracthed out, don't know what is it
  U24 = Surface scratched out, seems like a PROM
 DIPS = Fixed as: 00001000
 ROMS = Toshiba TC574000AD

  JP2, JP3 and P1 connects both boards, also another
  on-board connector is used, see notes for the 68K socket
  for the lower board.


   LOWER BOARD

   _________________________________________________________
   |                                     ____ ____         |
   |  ___                                | I| | I|         |
   |  |I|                                | C| | C|         |
   |  |C|                                | 3| | 2|         |
   |  |1|                                |__| |__|         |
   |  |3|                                                  |__
   |   _                _________________________           __|
   |  |_|               |||||||||||||||||||||||||           __|
   |  IC14              ---------- SLOT ---------           __|
   |               ______________________                   __|
   |              |                      |                  __|
   |  ___         | 68K (to upper board) |   _______        __|
   |  |I|         |______________________|   |SE-94|        __|
   |  |C|                                    |JDDB |      _|
   |  |1|           _______                  |_____|      |
   |  |2|           |SE-93|                    IC4        |
   |                |JDDA |                               |
   |                |_____|                ___________    |_
   |                  IC8                  |Z8400A PS|     |
   |                                       |_________|     |
   |                  ______         _________  _________  |
   |                  | OSC|         | IC11  |  | IC7   |  |
   |            _____________        |_______|  |_______|  |
   |    RST    |            |           CN5        CN6     |
   |___________|            |______________________________|


   IC3 = IC2 = Winbond W24257V
   IC7  = 6264LD 9440
   IC11 = SE-95 JDDC
   IC12 = Sony CXA1634P
   IC13 = Sony CXA1145P
   IC14 = GL358 N16

   RST is a reset button.

   OSC = 53.693175 MHz

   CN5 and CN6 are 9-pin connectors... serial ports?

   There are two wires soldered directly to two connectors
   of the slot, going to the upper board (via P1).

   The whole upper board is plugged using the 68000 socket,
   there is no 68K on the lower board.

   There is an edge connector, but it isn't JAMMA.

   "HK-986 (KINYO)" is written on the PCB, near the slot.

*/

#include "driver.h"
#include "genesis.h"


INPUT_PORTS_START( topshoot ) /* Top Shooter Input Ports */
	PORT_START
	PORT_BIT( 0x4f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_IMPULSE(1) PORT_NAME("Bet") 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_IMPULSE(1) PORT_NAME("Start")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_IMPULSE(1) PORT_NAME("Fire")
	
	PORT_START
	PORT_BIT( 0xe7, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_IMPULSE(1) PORT_NAME("Test mode down")
	
	PORT_START
	PORT_BIT( 0xfd, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( sbubsm )
	// the bit ordering in the ports is strange here because this is being read through shared RAM, the MCU presumably reads the real inputs then scrambles them in RAM for the 68k to sort out
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(10)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	// no service mode here?
INPUT_PORTS_END

INPUT_PORTS_START( aladbl )
	PORT_START /* Joypad 1 (3 button + start) NOT READ DIRECTLY */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Throw") // a
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Sword") // b
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Jump")  // c

	PORT_START /* Joypad 2 (3 button + start) Not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* 3rd I/O port */

	/* As I don't know how it is on real hardware, this is more a guess than anything */
	PORT_START /* MCU hooked up via readinputport (3) */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )          /* code at 0x1b2a50 - unsure if there are so many settings */
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )         /* to avoid it being changed and corrupting Coinage settings */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )       /* code at 0x1b2680 */
	PORT_DIPSETTING(    0x10, DEF_STR( Easy ) )             /* "PRACTICE" */
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )           /* "NORMAL" */
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )             /* "DIFFICULT" */
	PORT_BIT( 0x100, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)     /* needed to avoid credits getting mad */
INPUT_PORTS_END

INPUT_PORTS_START( barek3 )
	PORT_START	/* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_NAME("P1 Button A") // a
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Button B") // b
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Button C") // c

	PORT_START	/* IN1 player 2 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_NAME("P2 Button A") // a
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Button B") // b
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Button C") // c

	PORT_START
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
INPUT_PORTS_END

INPUT_PORTS_START( barek2ch )
	PORT_START	/* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START	/* IN1 player 2 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT(  0x3f, IP_ACTIVE_LOW, IPT_UNUSED ) // apparently no use for these
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, "SW1:4" )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, "SW1:5" )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, "SW1:6" )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, "SW1:7" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, "SW1:8" )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "SW2:1" ) // at least some of the first 3 seem to control difficulty (enemies attack later / less frequently by switching these)
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "SW2:2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "SW2:3" )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xe0, 0xe0, "Starting Level" )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_DIPSETTING(    0x00, "8" )

	PORT_START // present on PCB but there doesn't seem to be any read for them
	PORT_DIPNAME( 0x01, 0x01, "SW3:1" )
    PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x02, 0x02, "SW3:2" )
    PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x04, 0x04, "SW3:3" )
    PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x08, 0x08, "SW3:4" )
    PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x10, 0x10, "SW3:5" )
    PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x20, 0x20, "SW3:6")
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x40, 0x40, "SW3:7" )
    PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x80, 0x80, "SW3:8" )
    PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( sonic2mb )
    PORT_START  /* IN0 player 1 controller */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	
	PORT_START /* Joypad 2 (3 button + start) Not used */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	
    PORT_START /* 3rd I/O port */
	
	PORT_START /* DSW via readinputport 3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00fc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(          0x0300, 0x0200, DEF_STR( Lives ) )
	PORT_DIPSETTING(       0x0000, "1" )
	PORT_DIPSETTING(       0x0100, "2" )
	PORT_DIPSETTING(       0x0200, "3" )
	PORT_DIPSETTING(       0x0300, "4" )
	PORT_DIPNAME(  0x3c00, 0x2000, "Timer Speed" ) 
	PORT_DIPSETTING(       0x3c00, "0 (Slowest)" )
	PORT_DIPSETTING(       0x3800, "1" )
	PORT_DIPSETTING(       0x3400, "2" )
	PORT_DIPSETTING(       0x3000, "3" )
	PORT_DIPSETTING(       0x2c00, "4" )
	PORT_DIPSETTING(       0x2800, "5" )
	PORT_DIPSETTING(       0x2400, "6" )
	PORT_DIPSETTING(       0x2000, "7" )
	PORT_DIPSETTING(       0x1c00, "8" )
	PORT_DIPSETTING(       0x1800, "9" )
	PORT_DIPSETTING(       0x1400, "10" )
	PORT_DIPSETTING(       0x1000, "11" )
	PORT_DIPSETTING(       0x0c00, "12" )
	PORT_DIPSETTING(       0x0800, "13" )
	PORT_DIPSETTING(       0x0400, "14" )
	PORT_DIPSETTING(       0x0000, "15 (Fastest)" )
    PORT_DIPNAME( 0x4000,  0x4000, "SW1:7" )
    PORT_DIPSETTING(       0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
    PORT_DIPNAME( 0x8000,  0x8000, "SW1:8" )
    PORT_DIPSETTING(       0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static ADDRESS_MAP_START( topshoot_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)					/* Cartridge Program Rom */
//  AM_RANGE(0x200000, 0x20007f) AM_READ(MRA16_RAM)
//  AM_RANGE(0x200040, 0x200041) AM_READ(input_port_0_word_r)       // ??
//  AM_RANGE(0x200050, 0x200051) AM_READ(input_port_0_word_r)       // ??
	AM_RANGE(0x202000, 0x2023ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x400004, 0x400005) AM_READ(input_port_0_word_r)       // ??
	AM_RANGE(0xa10000, 0xa1001f) AM_READ(input_port_0_word_r)
	AM_RANGE(0xa11100, 0xa11101) AM_READ(input_port_0_word_r)		// ??
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(genesis_68k_to_z80_r)
	AM_RANGE(0xc00000, 0xc0001f) AM_READ(genesis_vdp_r)				/* VDP Access */
	AM_RANGE(0xe00000, 0xe1ffff) AM_READ(MRA16_BANK3)
	AM_RANGE(0xfe0000, 0xfeffff) AM_READ(MRA16_BANK4)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)					/* Main Ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( topshoot_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)					/* Cartridge Program Rom */
//  AM_RANGE(0x200000, 0x20007f) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x2023ff) AM_WRITE(MWA16_RAM) // tested
	AM_RANGE(0xa10000, 0xa1001f) AM_WRITE(genesis_io_w) AM_BASE(&genesis_io_ram)				/* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_WRITE(genesis_ctrl_w)
	AM_RANGE(0xa00000, 0xa0ffff) AM_WRITE(megaplay_68k_to_z80_w)
	AM_RANGE(0xc00000, 0xc0001f) AM_WRITE(genesis_vdp_w)				/* VDP Access */
	AM_RANGE(0xfe0000, 0xfeffff) AM_WRITE(MWA16_BANK4)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM) AM_BASE(&genesis_68k_ram)/* Main Ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sbubsm_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)					/* Cartridge Program Rom */
	AM_RANGE(0x202000, 0x2023ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(genesis_68k_to_z80_r)
	AM_RANGE(0xc00000, 0xc0001f) AM_READ(genesis_vdp_r)				/* VDP Access */
	AM_RANGE(0xe00000, 0xe1ffff) AM_READ(MRA16_BANK3)
	AM_RANGE(0xfe0000, 0xfeffff) AM_READ(MRA16_BANK4)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)					/* Main Ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sbubsm_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)					/* Cartridge Program Rom */
//  AM_RANGE(0x200000, 0x20007f) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x2023ff) AM_WRITE(MWA16_RAM) // tested
	AM_RANGE(0xa10000, 0xa1001f) AM_WRITE(genesis_io_w) AM_BASE(&genesis_io_ram)				/* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_WRITE(genesis_ctrl_w)
	AM_RANGE(0xa00000, 0xa0ffff) AM_WRITE(megaplay_68k_to_z80_w)
	AM_RANGE(0xc00000, 0xc0001f) AM_WRITE(genesis_vdp_w)				/* VDP Access */
//	AM_RANGE(0xc00010, 0xc00017) AM_WRITE(sn76489_w) // this is handled elsewhere circa 106					/* SN76489 Access */
    AM_RANGE(0xfe0000, 0xfeffff) AM_WRITE(MWA16_BANK4)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM) AM_BASE(&genesis_68k_ram)/* Main Ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( barek2ch_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA16_ROM)					/* Cartridge Program Rom */
	AM_RANGE(0x202000, 0x2023ff) AM_READ(MRA16_RAM)
	AM_RANGE(0xa10000, 0xa1001f) AM_READ(genesis_68000_io_r)				/* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_READ(genesis_ctrl_r)
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(genesis_68k_to_z80_r)
	AM_RANGE(0xc00000, 0xc0001f) AM_READ(genesis_vdp_r)				/* VDP Access */
	AM_RANGE(0xe00000, 0xe1ffff) AM_READ(MRA16_BANK3)
	AM_RANGE(0xfe0000, 0xfeffff) AM_READ(MRA16_BANK4)
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)					/* Main Ram */
ADDRESS_MAP_END


static ADDRESS_MAP_START( barek2ch_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA16_ROM)					/* Cartridge Program Rom */
//  AM_RANGE(0x200000, 0x20007f) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x2023ff) AM_WRITE(MWA16_RAM) // tested
	AM_RANGE(0xa10000, 0xa1001f) AM_WRITE(genesis_68000_io_w) AM_BASE(&genesis_io_ram)				/* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_WRITE(genesis_ctrl_w)
	AM_RANGE(0xa00000, 0xa0ffff) AM_WRITE(megaplay_68k_to_z80_w)
	AM_RANGE(0xc00000, 0xc0001f) AM_WRITE(genesis_vdp_w)				/* VDP Access */
//	AM_RANGE(0xc00010, 0xc00017) AM_WRITE(sn76489_w)					/* SN76489 Access */
    AM_RANGE(0xfe0000, 0xfeffff) AM_WRITE(MWA16_BANK4)
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM) AM_BASE(&genesis_68k_ram)/* Main Ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( barek3_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_READ(MRA16_ROM)					/* Cartridge Program Rom */
	AM_RANGE(0xa10000, 0xa1001f) AM_READ(genesis_68000_io_r)				/* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_READ(genesis_ctrl_r)
	AM_RANGE(0xa00000, 0xa0ffff) AM_READ(genesis_68k_to_z80_r)
	AM_RANGE(0xc00000, 0xc0001f) AM_READ(genesis_vdp_r)				/* VDP Access */
	AM_RANGE(0xfe0000, 0xfeffff) AM_READ(MRA16_BANK3)				/* Main Ram */
	AM_RANGE(0xff0000, 0xffffff) AM_READ(MRA16_RAM)					/* Main Ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( barek3_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x3fffff) AM_WRITE(MWA16_ROM)					/* Cartridge Program Rom */
	AM_RANGE(0xa10000, 0xa1001f) AM_WRITE(genesis_68000_io_w) AM_BASE(&genesis_io_ram)				/* Genesis Input */
	AM_RANGE(0xa11000, 0xa11203) AM_WRITE(genesis_ctrl_w)
	AM_RANGE(0xa00000, 0xa0ffff) AM_WRITE(megaplay_68k_to_z80_w)
	AM_RANGE(0xc00000, 0xc0001f) AM_WRITE(genesis_vdp_w)				/* VDP Access */
//	AM_RANGE(0xc00010, 0xc00017) AM_WRITE(sn76489_w)					/* SN76489 Access */
	AM_RANGE(0xfe0000, 0xfeffff) AM_WRITE(MWA16_BANK3)				/* Main Ram */
	AM_RANGE(0xff0000, 0xffffff) AM_WRITE(MWA16_RAM) AM_BASE(&genesis_68k_ram)/* Main Ram */
ADDRESS_MAP_END

static ADDRESS_MAP_START( genesis_z80_readmem, ADDRESS_SPACE_PROGRAM, 8 )
 	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_BANK1)
 	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_BANK2) /* mirror */
	AM_RANGE(0x4000, 0x7fff) AM_READ(genesis_z80_r)
	AM_RANGE(0x8000, 0xffff) AM_READ(genesis_z80_bank_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( genesis_z80_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_BANK1) AM_BASE(&genesis_z80_ram)
 	AM_RANGE(0x2000, 0x3fff) AM_WRITE(MWA8_BANK2) /* mirror */
	AM_RANGE(0x4000, 0x7fff) AM_WRITE(genesis_z80_w)
 // AM_RANGE(0x8000, 0xffff) AM_WRITE(genesis_z80_bank_w)
ADDRESS_MAP_END


static MACHINE_DRIVER_START( genesis_base )
	/*basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, MASTER_CLOCK / 7)
	MDRV_CPU_VBLANK_INT(genesis_vblank_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, MASTER_CLOCK / 15)
	MDRV_CPU_PROGRAM_MAP(genesis_z80_readmem, genesis_z80_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1) /* from vdp at scanline 0xe0 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int)(((262. - 224.) / 262.) * 1000000. / 60.))

	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_START(genesis)
	MDRV_MACHINE_RESET(genesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(320,224)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(genesis)
	MDRV_VIDEO_UPDATE(genesis)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3438, MASTER_CLOCK/7)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( topshoot )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( genesis_base )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(topshoot_readmem,topshoot_writemem)

	/* video hardware */
	MDRV_VIDEO_START(genesis)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sbubsm )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( genesis_base )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sbubsm_readmem,sbubsm_writemem)

	/* video hardware */
	MDRV_VIDEO_START(genesis)
	MDRV_VISIBLE_AREA(0, 319, 0, 223)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( barek2ch )

	MDRV_IMPORT_FROM( genesis_base )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(barek2ch_readmem,barek2ch_writemem)
	
	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( barek3 )

	MDRV_IMPORT_FROM( genesis_base )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(barek3_readmem,barek3_writemem)
	
	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, MASTER_CLOCK/15)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( topshoot ) /* Top Shooter (c)1995 Sun Mixing */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tc574000ad_u11_2.bin", 0x000000, 0x080000, CRC(b235c4d9) SHA1(fbb308a5f6e769f3277824cb6a3b50c308969ac2) )
	ROM_LOAD16_BYTE( "tc574000ad_u12_1.bin", 0x000001, 0x080000, CRC(e826f6ad) SHA1(23ec8bb608f954d3b915f061e7076c0c63b8259e) )
ROM_END

ROM_START( sbubsm )  /* Super Bubble Bobble (c)1996 Sun Mixing */
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u11.bin", 0x000000, 0x080000, CRC(4f9337ea) SHA1(b245eb615f80afd25e29b2efdddb7f61c1deff6b) )
	ROM_LOAD16_BYTE( "u12.bin", 0x000001, 0x080000, CRC(f5374835) SHA1(3a97910f5f7327ec7ad6425dfdfa72c86196ed33) )
	
	ROM_REGION( 0x1000, REGION_CPU2, 0 ) /* could be the same as topshoot (same PCB) */
//	ROM_LOAD( "89c51.bin", 0x0000, 0x1000, NO_DUMP )
ROM_END

ROM_START( barek2ch ) // all 27c4001
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "u14", 0x000001, 0x080000, CRC(b0ee177f) SHA1(d63e6ee30fe7f4aaab098d3920eabc456730b2c5) )
	ROM_LOAD16_BYTE( "u15", 0x000000, 0x080000, CRC(09264195) SHA1(c5439731d932c90a57d68c4d82c9ebed8a01bd53) )
	ROM_LOAD16_BYTE( "u16", 0x100001, 0x080000, CRC(6c814fc4) SHA1(edaf5117b19d3fb40218c5f7c4b5099c9189f1be) )
	ROM_LOAD16_BYTE( "u17", 0x100000, 0x080000, CRC(cae1922e) SHA1(811c2164b6c467a49af4b0d22f151cd13c9efbc9) )
ROM_END

ROM_START( barek3mb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) // 68000 Code
	ROM_LOAD16_BYTE( "6.u19", 0x000000, 0x080000,  CRC(2de19519) SHA1(f5fcef1da8b5370e399f0451382e3c6e7754c9c8) )
	ROM_LOAD16_BYTE( "3.u18", 0x000001, 0x080000,  CRC(db900e82) SHA1(172a4fe01a0ffd1ea3aed74f2c58234fd55b876d) )
	ROM_LOAD16_BYTE( "4.u15", 0x100000, 0x080000,  CRC(6353b4b1) SHA1(9f89a2f02170496ca798b89e37e1f2bae0e9155d) )
	ROM_LOAD16_BYTE( "1.u14", 0x100001, 0x080000,  CRC(24d31e12) SHA1(64c1b968e1ee5d0355d902e280f33e4466f27b07) )
	ROM_LOAD16_BYTE( "5.u17", 0x200000, 0x080000,  CRC(0feb974f) SHA1(ed1a25b6f1669dc6061d519985b6373fa89176c7) )
	ROM_LOAD16_BYTE( "2.u16", 0x200001, 0x080000,  CRC(bba4a585) SHA1(32c59729943d7b4c1a39f2a2b0dae9ce16991e9c) )
ROM_END

ROM_START( aladmdb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "m1.bin", 0x000001, 0x080000,  CRC(5e2671e4) SHA1(54705c7614fc7b5a1065478fa41f51dd1d8045b7) )
	ROM_LOAD16_BYTE( "m2.bin", 0x000000, 0x080000,  CRC(142a0366) SHA1(6c94aa9936cd11ccda503b52019a6721e64a32f0) )
	ROM_LOAD16_BYTE( "m3.bin", 0x100001, 0x080000,  CRC(0feeeb19) SHA1(bd567a33077ab9997871d21736066140d50e3d70) )
	ROM_LOAD16_BYTE( "m4.bin", 0x100000, 0x080000,  CRC(bc712661) SHA1(dfd554d000399e17b4ddc69761e572195ed4e1f0))
ROM_END

ROM_START( sonic2mb )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) // 68000 Code
	ROM_LOAD16_BYTE( "m1", 0x000001, 0x080000,  CRC(7b40aa24) SHA1(247882cd1f412366d61aeb4d85bbeefd5f108e1d) )
	ROM_LOAD16_BYTE( "m2", 0x000000, 0x080000,  CRC(84b3f758) SHA1(19846b9d951db6f78f3e155d33f1b6349fb87f1a) )
ROM_END

static READ16_HANDLER( vdp_fake_r )
{
	return rand();
}

static READ16_HANDLER(sbubsm_200051_r)
{
	return -0x5b;
}

static READ16_HANDLER(sbubsm_400000_r)
{
	logerror("%s: sbubsm_400000_r\n");
	return 0x5500;
}


static READ16_HANDLER(sbubsm_400002_r)
{
	logerror("%s: sbubsm_400002_r\n");
	return 0x0f00;
}


DRIVER_INIT(sbubsm)
{
	// needed to boot, somme kind of hardware ident?
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, sbubsm_400000_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x400002, 0x400003, 0, 0, sbubsm_400002_r );
	
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200050, 0x200051, 0, 0, sbubsm_200051_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200042, 0x200043, 0, 0, input_port_0_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200044, 0x200045, 0, 0, input_port_1_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200046, 0x200047, 0, 0, input_port_2_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200048, 0x200049, 0, 0, input_port_3_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x20007e, 0x20007f, 0, 0, input_port_4_word_r);

	memory_set_bankptr(3, memory_region(REGION_CPU1) );
//	memory_set_bankptr(4, &genesis_68k_ram[0]);
	memory_set_bankptr(4, genesis_68k_ram ); //correct for 106.??
}

DRIVER_INIT(topshoot)
{
	/* hack -- fix vdp emulation instead */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xC00004, 0xC00005, 0, 0, vdp_fake_r);
	
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200050, 0x200051, 0, 0, sbubsm_200051_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200042, 0x200043, 0, 0, input_port_0_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200044, 0x200045, 0, 0, input_port_1_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200046, 0x200047, 0, 0, input_port_2_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200048, 0x200049, 0, 0, input_port_3_word_r);

	memory_set_bankptr(3, memory_region(REGION_CPU1) );
	memory_set_bankptr(4, genesis_68k_ram );
}

DRIVER_INIT( barek3 )
{
	UINT8 *rom = memory_region(REGION_CPU1);
    int x;

	for (x = 0x00001; x < 0x300000; x += 2)
	{
		rom[x] = BITSWAP8(rom[x], 6,2,4,0,7,1,3,5);
	}

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x380070, 0x380071, 0, 0, input_port_2_word_r);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x380078, 0x380079, 0, 0, input_port_3_word_r);

	genesis_region = 0x00; /* read via io */
}

WRITE16_HANDLER( aladbl_w )
{
    /*
    Values returned from the log file :
      - aladbl_w : 1b2a6c - data = 6600 (each time a coin is inserted)
      - aladbl_w : 1b2a82 - data = 0000 (each time a coin is inserted)
      - aladbl_w : 1b2d18 - data = aa00 (only once on reset)
      - aladbl_w : 1b2d42 - data = 0000 (only once on reset)
    */
	logerror("aladbl_w : %06x - data = %04x\n",activecpu_get_pc(),data);
}


READ16_HANDLER( aladbl_r )
{
	if (activecpu_get_pc()==0x1b2a56)
	{
		static UINT16 mcu_port;

		mcu_port = readinputport(3);

		if(mcu_port & 0x100)
			return ((mcu_port & 0x0f) | 0x100); // coin inserted, calculate the number of coins
		else
			return (0x100); //MCU status, needed if you fall into a pitfall
	}
	if (activecpu_get_pc()==0x1b2a72) return 0x0000;
	if (activecpu_get_pc()==0x1b2d24) return (readinputport(3) & 0x00f0) | 0x1200;    // difficulty
	if (activecpu_get_pc()==0x1b2d4e) return 0x0000;

	logerror("aladbl_r : %06x\n",activecpu_get_pc());

	return 0x0000;
}

#define ENERGY_CONSOLE_MODE 1

DRIVER_INIT( aladbl )
{
/*
 * Game does a check @ 1afc00 with work ram fff57c that makes it play like it was intended (i.e. 8 energy hits instead of 2)
*/
	#if ENERGY_CONSOLE_MODE
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	rom[0x1afc08/2] = 0x6600;
	#endif

	// 220000 = writes to mcu? 330000 = reads?
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x220000, 0x220001, 0, 0, aladbl_w);
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x330000, 0x330001, 0, 0, aladbl_r);
	
	genesis_region = 0x00; /* read via io */
}

DRIVER_INIT( barek2ch )
{
	UINT16 *src = (UINT16 *)memory_region(REGION_CPU1);
	int i;
	
	for (i = 0x000000; i < 0x200000 / 2; i++)
		src[i] = BITSWAP16(src[i], 8, 11, 10, 13, 12, 14, 15, 9, 7, 6, 5, 4, 3, 2, 1, 0);

	src[0x06 / 2] = 0x0210; // TODO: why is this needed?

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x380070, 0x380071, 0, 0, input_port_2_word_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x380078, 0x380079, 0, 0, input_port_3_word_r );
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x38007a, 0x38007b, 0, 0, input_port_4_word_r );
	
	genesis_region = 0x00; /* read via io */
		
	memory_set_bankptr(3, memory_region(REGION_CPU1) );
	memory_set_bankptr(4, genesis_68k_ram );
}

DRIVER_INIT( sonic2mb )
{
   /* 100000 = writes to unpopulated MCU? */
	install_mem_write16_handler(0, 0x100000, 0x100001, aladbl_w);
    install_mem_read16_handler(0, 0x300000, 0x300001, input_port_3_word_r);
	
	genesis_region = 0x00; /* read via io */
		
	memory_set_bankptr(3, memory_region(REGION_CPU1) );
	memory_set_bankptr(4, genesis_68k_ram );
}


/* Sun Mixing Hardware, very close to actual Genesis */
GAME( 1995, topshoot,  0,        topshoot, topshoot, topshoot, ROT0, "Sun Mixing",     "Top Shooter", 0 )
GAME( 1996, sbubsm,    0,        sbubsm,   sbubsm,   sbubsm,   ROT0, "Sun Mixing",     "Super Bubble Bobble (Sun Mixing, Mega Drive clone hardware)", 0 )

/* Bootlegs Using Genesis Hardware */
/*
GAME( 1993, aladmdb,   0,        barek3,   aladbl,   aladbl,   ROT0, "bootleg / Sega", "Aladdin (bootleg of Japanese Megadrive version)", 0 )
GAME( 1993, sonic2mb,  0,        barek2ch, sonic2mb, sonic2mb, ROT0, "bootleg / Sega", "Sonic The Hedgehog 2 (bootleg of Mega Drive version)", 0 )
GAME( 1994, barek2ch,  0,        barek2ch, barek2ch, barek2ch, ROT0, "bootleg / Sega", "Bare Knuckle II (Chinese bootleg of Megadrive version)", 0 )
GAME( 1994, barek3mb,  0,        barek3,   barek3,   barek3,   ROT0, "bootleg / Sega", "Bare Knuckle III (bootleg of Megadrive version)", 0 ) 
*/
