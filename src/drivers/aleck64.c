/* 'Aleck64' and similar boards */
/* N64 based hardware */
/*

If you want to boot eleven beat on any n64 emu ?(tested on nemu, 1964
and project64) patch the rom :
write 0 to offset $67b,$67c,$67d,$67e

*/

/*

Eleven Beat World Tournament
Hudson Soft, 1998

This game runs on Nintendo 64-based hardware which is manufactured by Seta.
It's very similar to the hardware used for 'Magical Tetris Featuring Mickey'
(Seta E90 main board) except the game software is contained in a cart that
plugs into a slot on the main board. The E92 board also has more RAM than
the E90 board.
The carts are not compatible with standard N64 console carts.

PCB Layout
----------

          Seta E92 Mother PCB
         |---------------------------------------------|
       --|     VOL_POT                                 |
       |R|TA8139S                                      |
  RCA  --|  TA8201         BU9480                      |
 AUDIO   |                                             |
 PLUGS --|           AMP-NUS                           |
       |L|                                             |
       --|                 BU9480                      |
         |  TD62064                                    |
         |           UPD555  4050                      |
         |                                             |
         |    AD813  DSW1    TC59S1616AFT-10           |
         |J          DSW2    TC59S1616AFT-10           |
         |A       4.9152MHz                            |
         |M                                            |
         |M                                            |
         |A                    SETA                    |
         |                     ST-0043                 |
         |      SETA                          NINTENDO |
         |      ST-0042                       CPU-NUS A|
         |                                             |
         |                                             |
         |                                             |
         |                     14.31818MHz             |
         |X                                            |
         |   MAX232                            NINTENDO|
         |X          RDRAM18-NUS  RDRAM18-NUS  RCP-NUS |
         |           RDRAM18-NUS  RDRAM18-NUS          |
         |X   LVX125                                   |
         |                     14.705882MHz            |
         |X  PIF-NUS                                   |
         |            -------------------------------  |
         |   O        |                             |  |
         |            -------------------------------  |
         |---------------------------------------------|

Notes:
      Hsync      : 15.73kHz
      VSync      : 60Hz
      O          : Push-button reset switch
      X          : Connectors for special (Aleck64) digital joysticks
      CPU-NUS A  : Labelled on the PCB as "VR4300"

The cart contains:
                   CIC-NUS-5101: Boot protection chip
                   BK4D-NUS    : Similar to the save chip used in N64 console carts
                   NUS-ZHAJ.U3 : 64Mbit 28 pin DIP serial MASKROM

      - RCA audio plugs output stereo sound. Regular mono sound is output
        via the standard JAMMA connector also.

      - ALL components are listed for completeness. However, many are power or
        logic devices that most people need not be concerned about :-)

*/



/*

Magical Tetris Challenge Featuring Mickey
Capcom, 1998

This game runs on Nintendo 64-based hardware which is manufactured
by Seta. On bootup, it has the usual Capcom message....


Magical Tetris Challenge
        Featuring Mickey

       981009

        JAPAN


On bootup, they also mention 'T.L.S' (Temporary Landing System), which seems
to be the hardware system, designed by Arika Co. Ltd.


PCB Layout
----------

          Seta E90 Main PCB Rev. B
         |--------------------------------------------|
       --|     VOL_POT                       *        |
       |R|TA8139S                        TET-01M.U5   |
  RCA  --|  TA8201         BU9480                     |
 AUDIO   |                                SETA        |
 PLUGS --|           AMP-NUS              ST-0039     |
       |L|                      42.95454MHz           |
       --|                 BU9480                     |
         |  TD62064                   QS32X384        |
         |           UPD555  4050            QS32X384 |
         |                                            |
         |    AD813                                   |
         |J                                           |
         |A            4.9152MHz                      |
         |M                                           |
         |M                                           |
         |A                    SETA                   |
         |      SETA           ST-0035                |
         |      ST-0042                    NINTENDO   |
         |                     MX8330      CPU-NUS A  |
         |                     14.31818MHz            |
         |                                            |
         |X       AT24C01.U34  NINTENDO               |
         |                     RDRAM18-NUS            |
         |X                                           |
         |   MAX232            NINTENDO     NINTENDO  |
         |X          LT1084    RDRAM18-NUS  RCP-NUS   |
         |    LVX125     MX8330                       |
         |X  PIF-NUS       14.31818MHz                |
         |   O   CIC-NUS-5101  BK4D-NUS   NUS-CZAJ.U4 |
         |--------------------------------------------|

Notes:
      Hsync      : 15.73kHz
      VSync      : 60Hz
      *          : Unpopulated socket for 8M - 32M 42 pin DIP MASKROM
      O          : Push-button reset switch
      X          : Connectors for special (Aleck64?) digital joysticks
      CPU-NUS A  : Labelled on the PCB as "VR4300"
      BK4D-NUS   : Similar to the save chip used in N64 console carts

      ROMs
      ----
      TET-01M.U5 : 8Mbit 42 pin MASKROM
      NUS-CZAJ.U4: 128Mbit 28 pin DIP serial MASKROM
      AT24C01.U34: 128bytes x 8 bit serial EEPROM

      - RCA audio plugs output stereo sound. Regular mono sound is output
        via the standard JAMMA connector also.

      - ALL components are listed for completeness. However, many are power or
        logic devices that most people need not be concerned about :-)

      - The Seta/N64 Aleck64 hardware is similar also, but instead of the hich capacity
        serial MASKROM being on the main board, it's in a cart that plugs into a slot.

*/

#include "driver.h"
#include "cpu/mips/mips3.h"
#include "cpu/rsp/rsp.h"
#include "streams.h"
#include "sound/custom.h"
#include "includes/n64.h"

static READ32_HANDLER( aleck_dips_r )
{
	//return 0xff0fffff;
	if (offset == 0)
	{
		return (readinputport(3) << 16) | 0xffff;
	}
	else if (offset == 1)
	{
		return (readinputport(4) << 16) | 0xffff;
	}
	return 0;
}

static ADDRESS_MAP_START( n64_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE(&rdram)				// RDRAM
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_SHARE(1)					// RSP DMEM
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_SHARE(2)					// RSP IMEM
	AM_RANGE(0x04040000, 0x040fffff) AM_READWRITE(n64_sp_reg_r, n64_sp_reg_w)	// RSP
	AM_RANGE(0x04100000, 0x041fffff) AM_READWRITE(n64_dp_reg_r, n64_dp_reg_w)	// RDP
	AM_RANGE(0x04300000, 0x043fffff) AM_READWRITE(n64_mi_reg_r, n64_mi_reg_w)	// MIPS Interface
	AM_RANGE(0x04400000, 0x044fffff) AM_READWRITE(n64_vi_reg_r, n64_vi_reg_w)	// Video Interface
	AM_RANGE(0x04500000, 0x045fffff) AM_READWRITE(n64_ai_reg_r, n64_ai_reg_w)	// Audio Interface
	AM_RANGE(0x04600000, 0x046fffff) AM_READWRITE(n64_pi_reg_r, n64_pi_reg_w)	// Peripheral Interface
	AM_RANGE(0x04700000, 0x047fffff) AM_READWRITE(n64_ri_reg_r, n64_ri_reg_w)	// RDRAM Interface
	AM_RANGE(0x04800000, 0x048fffff) AM_READWRITE(n64_si_reg_r, n64_si_reg_w)	// Serial Interface
	AM_RANGE(0x10000000, 0x13ffffff) AM_ROM AM_REGION(REGION_USER2, 0)	// Cartridge
	AM_RANGE(0x1fc00000, 0x1fc007bf) AM_ROM AM_REGION(REGION_USER1, 0)	// PIF ROM
	AM_RANGE(0x1fc007c0, 0x1fc007ff) AM_READWRITE(n64_pif_ram_r, n64_pif_ram_w)

	AM_RANGE(0xc0800000, 0xc08fffff) AM_READWRITE(aleck_dips_r, MWA32_NOP)
	AM_RANGE(0xd0000000, 0xd08fffff) AM_NOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( rsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x04000000, 0x04000fff) AM_RAM AM_BASE(&rsp_dmem) AM_SHARE(1)
	AM_RANGE(0x04001000, 0x04001fff) AM_RAM AM_BASE(&rsp_imem) AM_SHARE(2)
ADDRESS_MAP_END

INPUT_PORTS_START( aleck64 )
	PORT_START_TAG("P1")
		PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)			// Button A
		PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)			// Button B
		PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)			// Button Z
		PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START1 ) 							// Start
		PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)		// Joypad Up
		PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)	// Joypad Down
		PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)	// Joypad Left
		PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)	// Joypad Right
		PORT_BIT( 0x00c0, IP_ACTIVE_HIGH, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)			// Pan Left
		PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_PLAYER(1)			// Pan Right
		PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_PLAYER(1)			// C Button Up
		PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_PLAYER(1)			// C Button Down
		PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_PLAYER(1)			// C Button Left
		PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_PLAYER(1)			// C Button Right

	PORT_START_TAG("P1_ANALOG_X")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START_TAG("P1_ANALOG_Y")
		PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0xff,0x00) PORT_SENSITIVITY(30) PORT_KEYDELTA(30) PORT_PLAYER(1)

	PORT_START
		PORT_DIPNAME( 0x8000, 0x8000, "DIPSW1 #8" )
		PORT_DIPSETTING( 0x8000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x4000, 0x4000, "DIPSW1 #7" )
		PORT_DIPSETTING( 0x4000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x2000, 0x2000, "DIPSW1 #6" )
		PORT_DIPSETTING( 0x2000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x1000, 0x1000, "DIPSW1 #5" )
		PORT_DIPSETTING( 0x1000, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0800, 0x0800, "DIPSW1 #4" )
		PORT_DIPSETTING( 0x0800, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0400, 0x0400, "DIPSW1 #3" )
		PORT_DIPSETTING( 0x0400, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0200, 0x0200, "DIPSW1 #2" )
		PORT_DIPSETTING( 0x0200, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0100, 0x0100, "DIPSW1 #1" )
		PORT_DIPSETTING( 0x0100, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0080, 0x0080, "Test Mode" )
		PORT_DIPSETTING( 0x0080, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0040, 0x0040, "DIPSW2 #7" )
		PORT_DIPSETTING( 0x0040, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0020, 0x0020, "DIPSW2 #6" )
		PORT_DIPSETTING( 0x0020, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0010, 0x0010, "DIPSW2 #5" )
		PORT_DIPSETTING( 0x0010, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0008, 0x0008, "DIPSW2 #4" )
		PORT_DIPSETTING( 0x0008, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0004, 0x0004, "DIPSW2 #3" )
		PORT_DIPSETTING( 0x0004, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0002, 0x0002, "DIPSW2 #2" )
		PORT_DIPSETTING( 0x0002, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
		PORT_DIPNAME( 0x0001, 0x0001, "DIPSW2 #1" )
		PORT_DIPSETTING( 0x0001, DEF_STR( Off ) )
		PORT_DIPSETTING( 0x0000, DEF_STR( On ) )

	PORT_START
		PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
		PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
		PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_7)
		PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
		PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

/* ?? */
static struct mips3_config config =
{
	16384,				/* code cache size */
	8192,				/* data cache size */
	62500000			/* system clock */
};

static INTERRUPT_GEN( n64_vblank )
{
	signal_rcp_interrupt(VI_INTERRUPT);
}

MACHINE_RESET( aleck64 )
{
	n64_machine_reset();
}

MACHINE_DRIVER_START( aleck64 )
	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, 93750000)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(n64_map, 0)
	MDRV_CPU_VBLANK_INT( n64_vblank, 1 )

	MDRV_CPU_ADD(RSP, 62500000)
	MDRV_CPU_PROGRAM_MAP(rsp_map, 0)

	MDRV_MACHINE_RESET( aleck64 )

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(640, 525)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(n64)
	MDRV_VIDEO_UPDATE(n64)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(n64_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(0, "right", 1.00)
MACHINE_DRIVER_END

DRIVER_INIT( aleck64 )
{
	UINT8 *rom = memory_region(REGION_USER2);

	rom[0x67c] = 0;
	rom[0x67d] = 0;
	rom[0x67e] = 0;
	rom[0x67f] = 0;
}

ROM_START( 11beat )
	ROM_REGION32_BE( 0x800, REGION_USER1, ROMREGION_ERASE00 )
		// PIF Boot ROM - not dumped

	ROM_REGION32_BE( 0x4000000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "nus-zhaj.u3", 0x000000, 0x0800000,  CRC(95258ba2) SHA1(0299b8fb9a8b1b24428d0f340f6bf1cfaf99c672) )
ROM_END

ROM_START( mtetrisc )
	ROM_REGION32_BE( 0x800, REGION_USER1, ROMREGION_ERASE00 )
		// PIF Boot ROM - not dumped

	ROM_REGION32_BE( 0x4000000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "nus-zcaj.u4", 0x000000, 0x1000000,  CRC(ec4563fc) SHA1(4d5a30873a5850cf4cd1c0bdbe24e1934f163cd0) )

	ROM_REGION32_BE( 0x100000, REGION_USER3, 0 )
	ROM_LOAD ( "tet-01m.u5", 0x000000, 0x100000,  CRC(f78f859b) SHA1(b07c85e0453869fe43792f42081f64a5327e58e6) )

	ROM_REGION32_BE( 0x80, REGION_USER4, 0 )
	ROM_LOAD ( "at24c01.u34", 0x000000, 0x80,  CRC(ba7e503f) SHA1(454aa4fdde7d8694d1affaf25cd750fa678686bb) )
ROM_END

GAME( 1998, 11beat,   0,  aleck64, aleck64, aleck64, ROT0, "Hudson", "Eleven Beat", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1998, mtetrisc, 0,  aleck64, aleck64, aleck64, ROT0, "Capcom", "Magical Tetris Challenge (981009 Japan)", GAME_NOT_WORKING|GAME_NO_SOUND )
