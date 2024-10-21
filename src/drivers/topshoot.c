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

/* Sun Mixing Hardware, very close to actual Genesis */
GAME( 1995, topshoot,  0,        topshoot, topshoot, topshoot, ROT0, "Sun Mixing",  "Top Shooter", 0 )
GAME( 1996, sbubsm,    0,        sbubsm,   sbubsm,   sbubsm,   ROT0, "Sun Mixing",  "Super Bubble Bobble (Sun Mixing, Mega Drive clone hardware)", 0 )
