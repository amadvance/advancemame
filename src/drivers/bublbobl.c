/***************************************************************************

Tokio          (c) 1986 Taito
Bubble Bobble  (c) 1986 Taito

driver by Chris Moore, Nicola Salmoria
also based on Tokio driver by Marcelo de G. Malheiros <malheiro@dca.fee.unicamp.br>


Main clock: XTAL = 24 MHz
Horizontal video frequency: HSYNC = XTAL/4/384 = 15.625 kHz
Video frequency: VSYNC = HSYNC/264 = 59.185606 Hz
VBlank duration: 1/VSYNC * (40/264) = 2560 us

****************************************************************************

Bubble Bobble ROM info

CPU Board
---------
           | Taito  |Romstar | ?????  |Romstar |
           |        |        |missing |mode sel|
17  CU1    | A78-01 |   ->   |   ->   |   ->   |   protection mcu (JPH1011P)
49  PAL1   | A78-02 |   ->   |   ->   |   ->   |   address decoder
43  PAL2   | A78-03 |   ->   |   ->   |   ->   |   address decoder
12  PAL3   | A78-04 |   ->   |   ->   |   ->   |   address decoder
53  empty  |        |        |        |        |   main prg
52  ROM1   | A78-05 | A78-21 | A78-22 | A78-24 |   main prg
51  ROM2   | A78-06 |   ->   | A78-23 | A78-25 |   main prg
46  ROM4   | A78-07 |   ->   |   ->   |   ->   |   sound prg
37  ROM3   | A78-08 |   ->   |   ->   |   ->   |   sub prg

Video Board
-----------
12  ROM1   | A78-09 |   ->   |   ->   |   ->   |   gfx
13  ROM2   | A78-10 |   ->   |   ->   |   ->   |   gfx
14  ROM3   | A78-11 |   ->   |   ->   |   ->   |   gfx
15  ROM4   | A78-12 |   ->   |   ->   |   ->   |   gfx
16  ROM5   | A78-13 |   ->   |   ->   |   ->   |   gfx
17  ROM6   | A78-14 |   ->   |   ->   |   ->   |   gfx
18  empty  |        |        |        |        |   gfx
19  empty  |        |        |        |        |   gfx
30  ROM7   | A78-15 |   ->   |   ->   |   ->   |   gfx
31  ROM8   | A78-16 |   ->   |   ->   |   ->   |   gfx
32  ROM9   | A78-17 |   ->   |   ->   |   ->   |   gfx
33  ROM10  | A78-18 |   ->   |   ->   |   ->   |   gfx
34  ROM11  | A78-19 |   ->   |   ->   |   ->   |   gfx
35  ROM12  | A78-20 |   ->   |   ->   |   ->   |   gfx
36  empty  |        |        |        |        |   gfx
37  empty  |        |        |        |        |   gfx
41  ROM13  | A71-25 |   ->   |   ->   |   ->   |   video timing


Bobble Bobble memory map

most of the address decoding is done by various PALs which haven't been read, so
the memory map is inferred by program behaviour

CPU #1

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0xxxxxxxxxxxxxxx R   xxxxxxxx ROM 51    program ROM
10xxxxxxxxxxxxxx R   xxxxxxxx ROM 52/53 program ROM (banked)
110xxxxxxxxxxxxx R/W xxxxxxxx VRAM      video RAM
11100xxxxxxxxxxx R/W xxxxxxxx WORK      RAM shared with CPU #2
11101xxxxxxxxxxx R/W xxxxxxxx WORK      RAM shared with CPU #2
11110xxxxxxxxxxx R/W xxxxxxxx WORK      RAM shared with CPU #2
1111100xxxxxxxxx R/W xxxxxxxx COLOR     palette RAM
111110100-----00   W xxxxxxxx SOUND     command for sound CPU
111110100-----01   W --------           n.c.
111110100-----10   W --------           n.c.
111110100-----11   W -------x SRESET    reset sound CPU and sound chips
111110100-----00 R   xxxxxxxx           answer from sound CPU (not used)
111110100-----01 R   -------x           message pending from sound CPU to CPU #1 (not used)
111110100-----01 R   ------x-           message pending from CPU #1 to sound CPU (not used)
111110100-----10 R                      n.c.
111110100-----11 R                      n.c.
111110101-------   W -------- TRES?     watchdog reset
1111101100------   W -------- NMIRQ     trigger NMI on CPU #2 (not used)
1111101101------   W -----xxx           ROM bank
1111101101------   W ----x---           n.c.
1111101101------   W ---x---- SBRES     reset CPU #2
1111101101------   W --x----- SEQRES    reset MCU
1111101101------   W -x------ BLACK     blank screen
1111101101------   W x------- VHINV     flip screen
1111101110------   W --------           n.c.
1111101111------   W --------           n.c.
111110111-------   W --------           n.c.
111111xxxxxxxxxx R/W xxxxxxxx MCRAM     RAM shared with MCU


CPU #2

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0xxxxxxxxxxxxxxx R   xxxxxxxx ROM 37    program ROM
111xxxxxxxxxxxxx R/W xxxxxxxx WORK      RAM shared with CPU #1 [1]

[1] The last 2kB could be used exclusively by this CPU, but they are not used at all.


Sound CPU

Address          Dir Data     Name      Description
---------------- --- -------- --------- -----------------------
0xxxxxxxxxxxxxxx R   xxxxxxxx ROM 46    program ROM
1000xxxxxxxxxxxx R/W xxxxxxxx RAM 47    work RAM
1001-----------x R/W xxxxxxxx FMA       YM2203
1010-----------x R/W xxxxxxxx FMB       YM3526
1011----------00 R   xxxxxxxx           command from CPU #1
1011----------01 R   -------x           message pending from sound CPU to CPU #1
1011----------01 R   ------x-           message pending from CPU #1 to sound CPU
1011----------10 R                      n.c.
1011----------11 R                      n.c.
1011----------00   W xxxxxxxx           answer to CPU #1
1011----------01   W --------           sound NMI enable
1011----------10   W --------           sound NMI disable
1011----------11   W                    n.c.
111------------- R   xxxxxxxx           space for diagnostic ROM?


****************************************************************************


Tokio
1986, Taito (Romstar License)

PCB Layout                                                                   G Pinout
----------                                                                   --------
                                                                   Component Side  Solder Side
Top Board                                                         ----------------|----------------
                                                                       Ground   1 | A  Ground
J1100069A CPU BOARD                                                 Video Red   2 | B  Video Ground
K1100157A                                                         Video Green   3 | C  Video Blue
M4300053A (sticker)                                                Video Sync   4 | D
  |------------------------------------------------------|          Speaker +   5 | E  Speaker -
  |   VOL      TL074          YM2203              Z80A   |                Key   6 | F  Key
|-|                                                      |                      7 | H
|     MB3731  PC010SA                      A71_07.IC10   |            Coin Sw   8 | J
|H                                                       |         Coin Meter   9 | K
|     TL7700                                      2016   |                     10 | L
|-|                                                      |            Service  11 | M  Slam Switch
  |   MC68705P5                           A71_06-1.IC8  |-|          1P Start  12 | N  2P Start
|-| (A71_24.IC57)                                       | |                    13 | P
|                                           A71_05.IC7  | |                    14 | R
|                                                       | |          1P Right  15 | S  2P Right
|                                           A71_04.IC6  | |          1P Left   16 | T  2P Left
|   PC030CM                   PAL                       | |                    17 | U
|                                         A71_28-1.IC5  | |                    18 | V
|G                                                      | |                    19 | W
|                                         A71_27-1.IC4  | |                    20 | X
|                                                       | |           1P Fire  21 | Y  2P Fire
|                                                       | |                    22 | Z
|                                                 Z80B  | |
|-|                                                     |-|
  |                                                      |                   H Pinout
  |                    2016                       Z80B   |                   --------
  |                                                      |                   1   Ground
  |                    2016                              |                   2   Ground
  |                                         A71_01.IC1   |                   3   Ground
  |   DSWB(8) DSWA(8)  2016                              |                   4   Ground
  |------------------------------------------------------|                   5   +5VDC
Notes:                                                                       6   +5VDC
        H - 12-pin Connector for Power Input                                 7   +5VDC
        G - 22-Way Edge Connector                                            8   -5VDC
      PAL - MMI PAL16L8B stamped 'A71-26' (DIP20)                            9   +12VDC
  PC030CM - Taito Custom Ceramic IC, possibly contains a smt logic IC,       10  Key
            smt resistors and smt capacitors (SIP20)                         11  +12VDC
  PC010SA - Taito Custom Ceramic IC, possibly contains                       12  +12VDC
            smt resistors and smt capacitors, sound DAC (SIP14)
   MB3731 - Fujitsu MB3731 18W Power Amplifier (SIP12)
   TL7700 - Texas Instruments TL7700 Supply-Voltage Supervisor/Power-On Reset IC (DIP8)
MC68705P5 - Motorola MC68705P5 Microcontroller with 2K Internal EPROM,
            clock input 3.000MHz [24/8] (DIP28)
     2016 - Toshiba TMM2016BP-10 2K x8 SRAM (DIP24)
     Z80A - Zilog Z8400APS Z80A CPU, clock input 3.000MHz [24/8] (DIP40)
     Z80B - Zilog Z8400BPS Z80B CPU, clock input 6.000MHz [24/4] (DIP40)
   YM2203 - Yamaha YM2203 Sound Chip, clock input 3.000MHz [24/8] (DIP40)
    VSync - 60Hz

     ROMs - (All EPROMs are 27C256)
            A71_07.IC10    Sound CPU Program

            A71_06-1.IC8 \
            A71_05.IC7   |
            A71_04.IC6   | Main CPU Program
            A71_28-1.IC5 |
            A71_27-1.IC4 /

            A71_01.IC1     Sub CPU Program

            A71_24.IC57    68705 Microcontroller Program (Protected, Not Dumped)



Bottom Board

J1100070A VIDEO BOARD
K1100158A
K1100172A (sticker)
  |------------------------------------------------------|                   T Pinout
  |                                     24MHz            |                   --------
  |                                                      |         Component Side  Solder Side
  |                                     2018             |        ----------------|-----------
  |                                     2018             |             Ground   1 | A  Ground
  |                     A71_25.IC41                      |             Ground   2 | B  Ground
  |                                     2018             |             Ground   3 | C  Ground
  |                                     2018            |-|            Ground   4 | D  Ground
  |                                                     | |                     5 | E
|-|                                                     | |                     6 | F
|                                                       | |                     7 | H
|    A71_08.IC12  A71_16.IC30                           | |                     8 | J
|                                                       | |                     9 | K
|T   A71_09.IC13  A71_17.IC31                           | |                    10 | L
|                                                       | |                    11 | M
|    A71_10.IC14  A71_18.IC32                           | |                    12 | N
|                                                       | |                    13 | P
|    A71_11.IC15  A71_19.IC33                           | |                    14 | R
|-|                                     2018            | |               +5V  15 | S  +5V
  |  A71_12.IC16  A71_20.IC34           2018            |-|               +5V  16 | T  +5V
  |                                                      |                +5V  17 | U  +5V
  |  A71_13.IC17  A71_21.IC35           2018             |                +5V  18 | V  +5V
  |                                     2018             |
  |  A71_14.IC18  A71_22.IC36           PC040DA          |
  |                                     PC040DA          |
  |  A71_15.IC19  A71_23.IC37           PC040DA          |
  |------------------------------------------------------|
Notes:
        T - 36-Way Edge Connector
  PC040DA - Taito Custom Ceramic IC, Video DAC (SIP19)
     2018 - Toshiba TMM2018D-45 2K x8 SRAM (DIP24)

     ROMs -
            A71_25.IC41    MMI 63S141 Bipolar PROM (DIP16)
            All EPROMs are 27C256


****************************************************************************


Notes:
- The coin inputs are handled by a custom called PC030. It would be responsible of
  handling the coin counters.
- There is a weird dip switch in Bubble Bobble (SWB #7). When it is on, the game
  takes the player score and the level number (increased by 1) and writes them,
  byte by byte, to $F7FE and $F7FF ($F7FF receives the same data but with the
  bit order reversed). After doing that, it sometimes hangs because it expects
  the value at $F7FF to change.
  This is done by routines $0F26 (player 1) and $0F74 (player 2).
  Frankly I don't know what this could be. The schematics don't show anything
  special there. A debug feature seems unlikely - why care about the score?
  Could it be provision for some kind of externally controlled redemption scheme?
- "Attract Sound" in Tokio is a very relative term - it just plays a very short
  sound every four rounds of demo play.

TODO:
- the MCU is actually a 6801U4, which has additional features like internal timers
  (similar to the HD63701). I've just mapped the I/O ports since that's the only
    thing required for normal operation, but the program does use some of the
    additional features in its special "test" mode.
- emulate the CPU #1 <-> sound CPU communication status flags (which are not used)
- why does emulating the sound CPU reset port (fa03) cause sound to stop working?
- tokio: doesn't work due to missing MCU protection emulation.
- tokio: sound support is probably incomplete. There are a couple of unknown
  accesses done by the CPU, including to the YM2203 I/O ports. At the
  very least, there should be some filters.

***************************************************************************/

#include "driver.h"
#include "sound/2203intf.h"
#include "sound/3812intf.h"

/* vidhrdw/bublbobl.c */
extern UINT8 *bublbobl_objectram;
extern size_t bublbobl_objectram_size;
VIDEO_UPDATE( bublbobl );

/* machine/bublbobl.c */
extern UINT8 *bublbobl_mcu_sharedram;
WRITE8_HANDLER( bublbobl_bankswitch_w );
WRITE8_HANDLER( tokio_bankswitch_w );
WRITE8_HANDLER( tokio_videoctrl_w );
WRITE8_HANDLER( bublbobl_nmitrigger_w );
READ8_HANDLER( tokio_mcu_r );
READ8_HANDLER( tokiob_mcu_r );
WRITE8_HANDLER( bublbobl_sound_command_w );
WRITE8_HANDLER( bublbobl_sh_nmi_disable_w );
WRITE8_HANDLER( bublbobl_sh_nmi_enable_w );
READ8_HANDLER( boblbobl_ic43_a_r );
WRITE8_HANDLER( boblbobl_ic43_a_w );
READ8_HANDLER( boblbobl_ic43_b_r );
WRITE8_HANDLER( boblbobl_ic43_b_w );

READ8_HANDLER( bublbobl_mcu_ddr1_r );
WRITE8_HANDLER( bublbobl_mcu_ddr1_w );
READ8_HANDLER( bublbobl_mcu_ddr2_r );
WRITE8_HANDLER( bublbobl_mcu_ddr2_w );
READ8_HANDLER( bublbobl_mcu_ddr3_r );
WRITE8_HANDLER( bublbobl_mcu_ddr3_w );
READ8_HANDLER( bublbobl_mcu_ddr4_r );
WRITE8_HANDLER( bublbobl_mcu_ddr4_w );
READ8_HANDLER( bublbobl_mcu_port1_r );
WRITE8_HANDLER( bublbobl_mcu_port1_w );
READ8_HANDLER( bublbobl_mcu_port2_r );
WRITE8_HANDLER( bublbobl_mcu_port2_w );
READ8_HANDLER( bublbobl_mcu_port3_r );
WRITE8_HANDLER( bublbobl_mcu_port3_w );
READ8_HANDLER( bublbobl_mcu_port4_r );
WRITE8_HANDLER( bublbobl_mcu_port4_w );

#if 0
// left for reference. The 68705 was from a bootleg, the original MCU is a 6801U4
INTERRUPT_GEN( bublbobl_m68705_interrupt );
READ8_HANDLER( bublbobl_68705_portA_r );
WRITE8_HANDLER( bublbobl_68705_portA_w );
WRITE8_HANDLER( bublbobl_68705_ddrA_w );
READ8_HANDLER( bublbobl_68705_portB_r );
WRITE8_HANDLER( bublbobl_68705_portB_w );
WRITE8_HANDLER( bublbobl_68705_ddrB_w );
#endif

#if 0 // doesn't work for some reason
static WRITE8_HANDLER(soundcpu_reset_w)
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, (data & 0x01) ? ASSERT_LINE : CLEAR_LINE);
}
#endif


static ADDRESS_MAP_START( master_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xdcff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xdd00, 0xdfff) AM_RAM AM_BASE(&bublbobl_objectram) AM_SIZE(&bublbobl_objectram_size)
	AM_RANGE(0xe000, 0xf7ff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xf800, 0xf9ff) AM_RAM AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_be_w) AM_BASE(&paletteram)
	AM_RANGE(0xfa00, 0xfa00) AM_WRITE(bublbobl_sound_command_w)
//  AM_RANGE(0xfa03, 0xfa03) AM_WRITE(soundcpu_reset_w) // doesn't work for some reason
	AM_RANGE(0xfa80, 0xfa80) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xfb40, 0xfb40) AM_WRITE(bublbobl_bankswitch_w)
	AM_RANGE(0xfc00, 0xffff) AM_RAM AM_BASE(&bublbobl_mcu_sharedram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xe000, 0xf7ff) AM_RAM AM_SHARE(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x9001, 0x9001) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0xa000, 0xa000) AM_READWRITE(YM3526_status_port_0_r, YM3526_control_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(YM3526_write_port_0_w)
	AM_RANGE(0xb000, 0xb000) AM_READ(soundlatch_r) AM_WRITENOP
	AM_RANGE(0xb001, 0xb001) AM_WRITE(bublbobl_sh_nmi_enable_w) AM_READNOP
	AM_RANGE(0xb002, 0xb002) AM_WRITE(bublbobl_sh_nmi_disable_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM	// space for diagnostic ROM?
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0000) AM_READWRITE(bublbobl_mcu_ddr1_r,  bublbobl_mcu_ddr1_w)
	AM_RANGE(0x0001, 0x0001) AM_READWRITE(bublbobl_mcu_ddr2_r,  bublbobl_mcu_ddr2_w)
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(bublbobl_mcu_port1_r, bublbobl_mcu_port1_w)
	AM_RANGE(0x0003, 0x0003) AM_READWRITE(bublbobl_mcu_port2_r, bublbobl_mcu_port2_w)
	AM_RANGE(0x0004, 0x0004) AM_READWRITE(bublbobl_mcu_ddr3_r,  bublbobl_mcu_ddr3_w)
	AM_RANGE(0x0005, 0x0005) AM_READWRITE(bublbobl_mcu_ddr4_r,  bublbobl_mcu_ddr4_w)
	AM_RANGE(0x0006, 0x0006) AM_READWRITE(bublbobl_mcu_port3_r, bublbobl_mcu_port3_w)
	AM_RANGE(0x0007, 0x0007) AM_READWRITE(bublbobl_mcu_port4_r, bublbobl_mcu_port4_w)
	AM_RANGE(0x0040, 0x00ff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

#if 0
// left for reference. The 68705 was from a bootleg, the original MCU is a 6801U4
static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x000, 0x000) AM_READWRITE(bublbobl_68705_portA_r, bublbobl_68705_portA_w)
	AM_RANGE(0x001, 0x001) AM_READWRITE(bublbobl_68705_portB_r, bublbobl_68705_portB_w)
	AM_RANGE(0x002, 0x002) AM_READ(input_port_0_r)	// COIN
	AM_RANGE(0x004, 0x004) AM_WRITE(bublbobl_68705_ddrA_w)
	AM_RANGE(0x005, 0x005) AM_WRITE(bublbobl_68705_ddrB_w)
	AM_RANGE(0x006, 0x006) AM_WRITENOP // ???
	AM_RANGE(0x010, 0x07f) AM_RAM
	AM_RANGE(0x080, 0x7ff) AM_ROM
ADDRESS_MAP_END
#endif

static ADDRESS_MAP_START( bootleg_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xdcff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xdd00, 0xdfff) AM_RAM AM_BASE(&bublbobl_objectram) AM_SIZE(&bublbobl_objectram_size)
	AM_RANGE(0xe000, 0xf7ff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xf800, 0xf9ff) AM_RAM AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_be_w) AM_BASE(&paletteram)
	AM_RANGE(0xfa00, 0xfa00) AM_WRITE(bublbobl_sound_command_w)
	AM_RANGE(0xfa03, 0xfa03) AM_WRITENOP // sound cpu reset
	AM_RANGE(0xfa80, 0xfa80) AM_WRITENOP // ???
	AM_RANGE(0xfb40, 0xfb40) AM_WRITE(bublbobl_bankswitch_w)
	AM_RANGE(0xfc00, 0xfcff) AM_RAM
	AM_RANGE(0xfd00, 0xfdff) AM_RAM
	AM_RANGE(0xfe00, 0xfe03) AM_READWRITE(boblbobl_ic43_a_r,boblbobl_ic43_a_w)
	AM_RANGE(0xfe80, 0xfe83) AM_READWRITE(boblbobl_ic43_b_r,boblbobl_ic43_b_w)
	AM_RANGE(0xff00, 0xff00) AM_READ(input_port_0_r)
	AM_RANGE(0xff01, 0xff01) AM_READ(input_port_1_r)
	AM_RANGE(0xff02, 0xff02) AM_READ(input_port_2_r)
	AM_RANGE(0xff03, 0xff03) AM_READ(input_port_3_r)
	AM_RANGE(0xff94, 0xff94) AM_WRITENOP // ???
	AM_RANGE(0xff98, 0xff98) AM_WRITENOP // ???
ADDRESS_MAP_END


static ADDRESS_MAP_START( tokio_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(1)
	AM_RANGE(0xc000, 0xdcff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xdd00, 0xdfff) AM_RAM AM_BASE(&bublbobl_objectram) AM_SIZE(&bublbobl_objectram_size)
	AM_RANGE(0xe000, 0xf7ff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xf800, 0xf9ff) AM_RAM AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_be_w) AM_BASE(&paletteram)
	AM_RANGE(0xfa00, 0xfa00) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xfa03, 0xfa03) AM_READ(input_port_0_r)
	AM_RANGE(0xfa04, 0xfa04) AM_READ(input_port_1_r)
	AM_RANGE(0xfa05, 0xfa05) AM_READ(input_port_2_r)
	AM_RANGE(0xfa06, 0xfa06) AM_READ(input_port_3_r)
	AM_RANGE(0xfa07, 0xfa07) AM_READ(input_port_4_r)
	AM_RANGE(0xfa80, 0xfa80) AM_WRITE(tokio_bankswitch_w)
	AM_RANGE(0xfb00, 0xfb00) AM_WRITE(tokio_videoctrl_w)
	AM_RANGE(0xfb80, 0xfb80) AM_WRITE(bublbobl_nmitrigger_w)
	AM_RANGE(0xfc00, 0xfc00) AM_READNOP AM_WRITE(bublbobl_sound_command_w) // ???
	AM_RANGE(0xfe00, 0xfe00) AM_READ(tokio_mcu_r) AM_WRITENOP // ???
ADDRESS_MAP_END

static ADDRESS_MAP_START( tokio_slave_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x97ff) AM_RAM AM_SHARE(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tokio_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_READ(soundlatch_r) AM_WRITENOP	// ???
	AM_RANGE(0x9800, 0x9800) AM_READNOP	// ???
	AM_RANGE(0xa000, 0xa000) AM_WRITE(bublbobl_sh_nmi_disable_w)
	AM_RANGE(0xa800, 0xa800) AM_WRITE(bublbobl_sh_nmi_enable_w)
	AM_RANGE(0xb000, 0xb000) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0xb001, 0xb001) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM	// space for diagnostic ROM?
ADDRESS_MAP_END



INPUT_PORTS_START( bublbobl )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL )	// output: coin lockout
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	// output: select 1-way or 2-way coin counter
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	// output: trigger IRQ on main CPU (jumper switchable to vblank)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL )	// output: select read or write shared RAM

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x05, 0x04, "Mode" )
	PORT_DIPSETTING(    0x04, "Game, English" )
	PORT_DIPSETTING(    0x05, "Game, Japanese" )
	PORT_DIPSETTING(    0x01, "Test (Grid and Inputs)" )
	PORT_DIPSETTING(    0x00, "Test (RAM and Sound)" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20K 80K 300K" )
	PORT_DIPSETTING(    0x0c, "30K 100K 400K" )
	PORT_DIPSETTING(    0x04, "40K 200K 500K" )
	PORT_DIPSETTING(    0x00, "50K 250K 500K" )
	// then more bonus lives at 1M 2M 3M 4M 5M - for all dip switch settings
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	// must be off (see notes)
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "ROM Type" )	// will hang on startup if set to wrong type
	PORT_DIPSETTING(    0x80, "IC52=512kb, IC53=none" )
	PORT_DIPSETTING(    0x00, "IC52=256kb, IC53=256kb" )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( boblbobl )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x05, 0x04, "Mode" )
	PORT_DIPSETTING(    0x04, "Game, English" )
	PORT_DIPSETTING(    0x05, "Game, Japanese" )
	PORT_DIPSETTING(    0x01, "Test (Grid and Inputs)" )
	PORT_DIPSETTING(    0x00, "Test (RAM and Sound)" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20K 80K 300K" )
	PORT_DIPSETTING(    0x0c, "30K 100K 400K" )
	PORT_DIPSETTING(    0x04, "40K 200K 500K" )
	PORT_DIPSETTING(    0x00, "50K 250K 500K" )
	// then more bonus lives at 1M 2M 3M 4M 5M - for all dip switch settings
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, "Monster Speed" )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x80, DEF_STR( High ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Very_High ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT ) // ???
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( sboblbob )
	PORT_INCLUDE( boblbobl )

	PORT_MODIFY( "DSW0" )
	PORT_DIPNAME( 0x01, 0x00, "Game" )
	PORT_DIPSETTING(    0x01, "Bobble Bobble" )
	PORT_DIPSETTING(    0x00, "Super Bobble Bobble" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )

	PORT_MODIFY( "DSW1" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "100 (Cheat)")
INPUT_PORTS_END

INPUT_PORTS_START( tokio )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x01, "Enemies" )
	PORT_DIPSETTING(    0x01, "Few (Easy)" )
	PORT_DIPSETTING(    0x00, "Many (Hard)" )
	PORT_DIPNAME( 0x02, 0x02, "Enemy Shots" )
	PORT_DIPSETTING(    0x02, "Few (Easy)" )
	PORT_DIPSETTING(    0x00, "Many (Hard)" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100K 400K" )
	PORT_DIPSETTING(    0x08, "200K 400K" )
	PORT_DIPSETTING(    0x04, "300K 400K" )
	PORT_DIPSETTING(    0x00, "400K 400K" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "99 (Cheat)")	// 6 in original version
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Japanese ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	// data ready from MCU
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8, 8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2), RGN_FRAC(1,2)+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 }
};



#define MAIN_XTAL 	24000000
#define HSYNC 		MAIN_XTAL / 4 / 384
#define VSYNC		HSYNC / 264
#define VBLANK		1 / VSYNC * (40 / 264)

// handler called by the 2203 emulator when the internal timers cause an IRQ
static void irqhandler(int irq)
{
	cpunum_set_input_line(2, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	0,
	0,
	0,
	0,
	irqhandler
};



static MACHINE_DRIVER_START( tokio )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	// 6 MHz
	MDRV_CPU_PROGRAM_MAP(tokio_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	// 6 MHz
	MDRV_CPU_PROGRAM_MAP(tokio_slave_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	/* audio CPU */	// 3 MHz
	MDRV_CPU_PROGRAM_MAP(tokio_sound_map, 0) // NMIs are triggered by the main CPU, IRQs are triggered by the YM2203

	MDRV_FRAMES_PER_SECOND(VSYNC)	// 59.185606 Hz
	MDRV_VBLANK_DURATION(VBLANK) 	// 2560 us
	MDRV_INTERLEAVE(100) // 100 CPU slices per frame - a high value to ensure proper synchronization of the CPUs

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(bublbobl)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, MAIN_XTAL/8)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.08)
	MDRV_SOUND_ROUTE(1, "mono", 0.08)
	MDRV_SOUND_ROUTE(2, "mono", 0.08)
	MDRV_SOUND_ROUTE(3, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bublbobl )
	// basic machine hardware
	MDRV_CPU_ADD_TAG("main", Z80, MAIN_XTAL/4)	// 6 MHz
	MDRV_CPU_PROGRAM_MAP(master_map, 0)
	// IRQs are triggered by the MCU

	MDRV_CPU_ADD(Z80, MAIN_XTAL/4)	// 6 MHz
	MDRV_CPU_PROGRAM_MAP(slave_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80, MAIN_XTAL/8)
	/* audio CPU */	// 3 MHz
	MDRV_CPU_PROGRAM_MAP(sound_map, 0) // IRQs are triggered by the YM2203

	MDRV_CPU_ADD_TAG("mcu", M6801, 4000000/4)	// actually 6801U4  // xtal is 4MHz, divided by 4 internally
	MDRV_CPU_PROGRAM_MAP(mcu_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse, 1) // comes from the same clock that latches the INT pin on the second Z80

#if 0
	// left for reference. The 68705 was from a bootleg, the original MCU is a 6801U4
	MDRV_CPU_ADD_TAG("mcu", M68705, 4000000/2)	// xtal is 4MHz, I think it's divided by 2 internally
	MDRV_CPU_PROGRAM_MAP(mcu_map, 0)
	MDRV_CPU_VBLANK_INT(bublbobl_m68705_interrupt, 2) // ??? should come from the same clock which latches the INT pin on the second Z80
#endif

	MDRV_FRAMES_PER_SECOND(VSYNC)	// 59.185606 Hz
	MDRV_VBLANK_DURATION(VBLANK) 	// 2560 us
	MDRV_INTERLEAVE(100) // 100 CPU slices per frame - a high value to ensure proper synchronization of the CPUs

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(bublbobl)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, MAIN_XTAL/8)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(YM3526, MAIN_XTAL/8)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( boblbobl )
	MDRV_IMPORT_FROM(bublbobl)

	// basic machine hardware
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(bootleg_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)	// interrupt mode 1, unlike Bubble Bobble

	MDRV_CPU_REMOVE("mcu")
MACHINE_DRIVER_END



ROM_START( tokio )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* main CPU */
	ROM_LOAD( "a71-02.4",     0x00000, 0x8000, CRC(d556c908) SHA1(d5d8afb7f7888d77aa9a372dfbab75fbd0358cc3) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a71-03.5",     0x10000, 0x8000, CRC(69dacf44) SHA1(ee8c33702749c0e2562951f9f80c897d3fbd7dd7) )
	ROM_LOAD( "a71-04.6",     0x18000, 0x8000, CRC(a0a4ce0e) SHA1(c49bdcd85c760a5e7327d1b424772e1560f1a318) )
	ROM_LOAD( "a71-05.7",     0x20000, 0x8000, CRC(6da0b945) SHA1(6c80b8333dd95657f99e6ba5b6e877733ac02a8c) )
	ROM_LOAD( "a71-06.8",     0x28000, 0x8000, CRC(447d6779) SHA1(5b329b221357a9cea777415d409a6423529a925c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* video CPU */
	ROM_LOAD( "a71-01.1",     0x00000, 0x8000, CRC(0867c707) SHA1(7129974f1252b28e9e338bd3c7fcb87210dcf412) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* audio CPU */
	ROM_LOAD( "a71-07.10",    0x0000, 0x08000, CRC(f298cc7b) SHA1(ebf5c804aa07b7f198ec3e1f8d1e111cd89ebdf3) )

	ROM_REGION( 0x0800, REGION_CPU4, 0 )	/* 2k for the microcontroller (68705P5) */
	ROM_LOAD( "a71-24.57",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a71-08.12",    0x00000, 0x8000, CRC(0439ab13) SHA1(84142220a6a29f0e34f7c7c751b583bf394df8ce) )    /* 1st plane */
	ROM_LOAD( "a71-09.13",    0x08000, 0x8000, CRC(edb3d2ff) SHA1(0c6e4bbc786a097f9d99220e72f98c1c795a7292) )
	ROM_LOAD( "a71-10.14",    0x10000, 0x8000, CRC(69f0888c) SHA1(1704ab6339981195cd09d581e83094c75037d18e) )
	ROM_LOAD( "a71-11.15",    0x18000, 0x8000, CRC(4ae07c31) SHA1(452d1eb5a70e7853791cd05e4578c1454477bdec) )
	ROM_LOAD( "a71-12.16",    0x20000, 0x8000, CRC(3f6bd706) SHA1(b03c534a95b71941331d3ffd9aa7069b5f05687e) )
	ROM_LOAD( "a71-13.17",    0x28000, 0x8000, CRC(f2c92aaa) SHA1(7dfdc473794a298032405ba918df8085b0bbe174) )
	ROM_LOAD( "a71-14.18",    0x30000, 0x8000, CRC(c574b7b2) SHA1(9839adce60c0017ae3997603a2aece511af226d2) )
	ROM_LOAD( "a71-15.19",    0x38000, 0x8000, CRC(12d87e7f) SHA1(327a80f08207ee66721738f7e1c53f75b5659be0) )
	ROM_LOAD( "a71-16.30",    0x40000, 0x8000, CRC(0bce35b6) SHA1(3f0496db6681c7be1e36ba41296115d158d7457a) )    /* 2nd plane */
	ROM_LOAD( "a71-17.31",    0x48000, 0x8000, CRC(deda6387) SHA1(40f0be3a71b0a03f0275da72f4124424b162318a) )
	ROM_LOAD( "a71-18.32",    0x50000, 0x8000, CRC(330cd9d7) SHA1(919f78036b760938d6aa72754be1a615f568b470) )
	ROM_LOAD( "a71-19.33",    0x58000, 0x8000, CRC(fc4b29e0) SHA1(d11393a24b5c6c04f5058b299e4b0fc773a03e4b) )
	ROM_LOAD( "a71-20.34",    0x60000, 0x8000, CRC(65acb265) SHA1(2ef940f994e76d4387be6e0d53a565813cc59636) )
	ROM_LOAD( "a71-21.35",    0x68000, 0x8000, CRC(33cde9b2) SHA1(9b227ab609e3c7c6be90c29739a57ea4959cd68e) )
	ROM_LOAD( "a71-22.36",    0x70000, 0x8000, CRC(fb98eac0) SHA1(57615c3934de5510eeeb0ba16024abda8ee95303) )
	ROM_LOAD( "a71-23.37",    0x78000, 0x8000, CRC(30bd46ad) SHA1(6e1618ed237c769d1a8d329fbd7a9f7216993215) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( tokiou )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* main CPU */
	ROM_LOAD( "a71-27-1.4",   0x00000, 0x8000, CRC(8c180896) SHA1(bc8aeb42da4bae7db6f65b9874224f60a9bc4500) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a71-28-1.5",   0x10000, 0x8000, CRC(1b447527) SHA1(6939e6c1b8492825d18f4e96f39ff45f4c96eea2) )
	ROM_LOAD( "a71-04.6",     0x18000, 0x8000, CRC(a0a4ce0e) SHA1(c49bdcd85c760a5e7327d1b424772e1560f1a318) )
	ROM_LOAD( "a71-05.7",     0x20000, 0x8000, CRC(6da0b945) SHA1(6c80b8333dd95657f99e6ba5b6e877733ac02a8c) )
	ROM_LOAD( "a71-06-1.8",   0x28000, 0x8000, CRC(56927b3f) SHA1(33fb4e71b95664ecff1f35f6782a14101982a56d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* video CPU */
	ROM_LOAD( "a71-01.1",     0x00000, 0x8000, CRC(0867c707) SHA1(7129974f1252b28e9e338bd3c7fcb87210dcf412) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* audio CPU */
	ROM_LOAD( "a71-07.10",    0x0000, 0x08000, CRC(f298cc7b) SHA1(ebf5c804aa07b7f198ec3e1f8d1e111cd89ebdf3) )

	ROM_REGION( 0x0800, REGION_CPU4, 0 )	/* 2k for the microcontroller (68705P5) */
	ROM_LOAD( "a71-24.57",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a71-08.12",    0x00000, 0x8000, CRC(0439ab13) SHA1(84142220a6a29f0e34f7c7c751b583bf394df8ce) )    /* 1st plane */
	ROM_LOAD( "a71-09.13",    0x08000, 0x8000, CRC(edb3d2ff) SHA1(0c6e4bbc786a097f9d99220e72f98c1c795a7292) )
	ROM_LOAD( "a71-10.14",    0x10000, 0x8000, CRC(69f0888c) SHA1(1704ab6339981195cd09d581e83094c75037d18e) )
	ROM_LOAD( "a71-11.15",    0x18000, 0x8000, CRC(4ae07c31) SHA1(452d1eb5a70e7853791cd05e4578c1454477bdec) )
	ROM_LOAD( "a71-12.16",    0x20000, 0x8000, CRC(3f6bd706) SHA1(b03c534a95b71941331d3ffd9aa7069b5f05687e) )
	ROM_LOAD( "a71-13.17",    0x28000, 0x8000, CRC(f2c92aaa) SHA1(7dfdc473794a298032405ba918df8085b0bbe174) )
	ROM_LOAD( "a71-14.18",    0x30000, 0x8000, CRC(c574b7b2) SHA1(9839adce60c0017ae3997603a2aece511af226d2) )
	ROM_LOAD( "a71-15.19",    0x38000, 0x8000, CRC(12d87e7f) SHA1(327a80f08207ee66721738f7e1c53f75b5659be0) )
	ROM_LOAD( "a71-16.30",    0x40000, 0x8000, CRC(0bce35b6) SHA1(3f0496db6681c7be1e36ba41296115d158d7457a) )    /* 2nd plane */
	ROM_LOAD( "a71-17.31",    0x48000, 0x8000, CRC(deda6387) SHA1(40f0be3a71b0a03f0275da72f4124424b162318a) )
	ROM_LOAD( "a71-18.32",    0x50000, 0x8000, CRC(330cd9d7) SHA1(919f78036b760938d6aa72754be1a615f568b470) )
	ROM_LOAD( "a71-19.33",    0x58000, 0x8000, CRC(fc4b29e0) SHA1(d11393a24b5c6c04f5058b299e4b0fc773a03e4b) )
	ROM_LOAD( "a71-20.34",    0x60000, 0x8000, CRC(65acb265) SHA1(2ef940f994e76d4387be6e0d53a565813cc59636) )
	ROM_LOAD( "a71-21.35",    0x68000, 0x8000, CRC(33cde9b2) SHA1(9b227ab609e3c7c6be90c29739a57ea4959cd68e) )
	ROM_LOAD( "a71-22.36",    0x70000, 0x8000, CRC(fb98eac0) SHA1(57615c3934de5510eeeb0ba16024abda8ee95303) )
	ROM_LOAD( "a71-23.37",    0x78000, 0x8000, CRC(30bd46ad) SHA1(6e1618ed237c769d1a8d329fbd7a9f7216993215) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( tokiob )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "2",            0x00000, 0x8000, CRC(f583b1ef) SHA1(a97b36299b51792953516224191f11decc579a38) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a71-03.5",     0x10000, 0x8000, CRC(69dacf44) SHA1(ee8c33702749c0e2562951f9f80c897d3fbd7dd7) )
	ROM_LOAD( "a71-04.6",     0x18000, 0x8000, CRC(a0a4ce0e) SHA1(c49bdcd85c760a5e7327d1b424772e1560f1a318) )
	ROM_LOAD( "a71-05.7",     0x20000, 0x8000, CRC(6da0b945) SHA1(6c80b8333dd95657f99e6ba5b6e877733ac02a8c) )
	ROM_LOAD( "6",            0x28000, 0x8000, CRC(1490e95b) SHA1(a73e1857a1029156f0b5f7f7fe34a37870e72209) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* video CPU */
	ROM_LOAD( "a71-01.1",     0x00000, 0x8000, CRC(0867c707) SHA1(7129974f1252b28e9e338bd3c7fcb87210dcf412) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* audio CPU */
	ROM_LOAD( "a71-07.10",    0x0000, 0x08000, CRC(f298cc7b) SHA1(ebf5c804aa07b7f198ec3e1f8d1e111cd89ebdf3) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a71-08.12",    0x00000, 0x8000, CRC(0439ab13) SHA1(84142220a6a29f0e34f7c7c751b583bf394df8ce) )    /* 1st plane */
	ROM_LOAD( "a71-09.13",    0x08000, 0x8000, CRC(edb3d2ff) SHA1(0c6e4bbc786a097f9d99220e72f98c1c795a7292) )
	ROM_LOAD( "a71-10.14",    0x10000, 0x8000, CRC(69f0888c) SHA1(1704ab6339981195cd09d581e83094c75037d18e) )
	ROM_LOAD( "a71-11.15",    0x18000, 0x8000, CRC(4ae07c31) SHA1(452d1eb5a70e7853791cd05e4578c1454477bdec) )
	ROM_LOAD( "a71-12.16",    0x20000, 0x8000, CRC(3f6bd706) SHA1(b03c534a95b71941331d3ffd9aa7069b5f05687e) )
	ROM_LOAD( "a71-13.17",    0x28000, 0x8000, CRC(f2c92aaa) SHA1(7dfdc473794a298032405ba918df8085b0bbe174) )
	ROM_LOAD( "a71-14.18",    0x30000, 0x8000, CRC(c574b7b2) SHA1(9839adce60c0017ae3997603a2aece511af226d2) )
	ROM_LOAD( "a71-15.19",    0x38000, 0x8000, CRC(12d87e7f) SHA1(327a80f08207ee66721738f7e1c53f75b5659be0) )
	ROM_LOAD( "a71-16.30",    0x40000, 0x8000, CRC(0bce35b6) SHA1(3f0496db6681c7be1e36ba41296115d158d7457a) )    /* 2nd plane */
	ROM_LOAD( "a71-17.31",    0x48000, 0x8000, CRC(deda6387) SHA1(40f0be3a71b0a03f0275da72f4124424b162318a) )
	ROM_LOAD( "a71-18.32",    0x50000, 0x8000, CRC(330cd9d7) SHA1(919f78036b760938d6aa72754be1a615f568b470) )
	ROM_LOAD( "a71-19.33",    0x58000, 0x8000, CRC(fc4b29e0) SHA1(d11393a24b5c6c04f5058b299e4b0fc773a03e4b) )
	ROM_LOAD( "a71-20.34",    0x60000, 0x8000, CRC(65acb265) SHA1(2ef940f994e76d4387be6e0d53a565813cc59636) )
	ROM_LOAD( "a71-21.35",    0x68000, 0x8000, CRC(33cde9b2) SHA1(9b227ab609e3c7c6be90c29739a57ea4959cd68e) )
	ROM_LOAD( "a71-22.36",    0x70000, 0x8000, CRC(fb98eac0) SHA1(57615c3934de5510eeeb0ba16024abda8ee95303) )
	ROM_LOAD( "a71-23.37",    0x78000, 0x8000, CRC(30bd46ad) SHA1(6e1618ed237c769d1a8d329fbd7a9f7216993215) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

// left for reference. The 68705 was from a bootleg, the original MCU is a 6801U4
//  ROM_LOAD( "68705.bin",    0x0000, 0x0800, CRC(78caa635) SHA1(a756e45b25b007843ba4f2204cad6081cf7260e9) )    /* from a pirate board */


/*

bublbobl.zip - a78-05-1.52
TAITO CORPORATION 1986
ALL RIGHTS RESERVED
VER 0.1 4.SEP,1986 SUMMER

Name          Size    CRC32       Chip Type
-------------------------------------------
a78-05-1.52    65536  0x9f8ee242  Fujitsu MBM27C512
a78-06-1.51    32768  0x567934b6  Intel D27256

*/

ROM_START( bublbobl )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06-1.51",    0x00000, 0x08000, CRC(567934b6) SHA1(b0c4d49fd551f465d148c25c3e80b278835e2f0d) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-05-1.52",    0x10000, 0x10000, CRC(9f8ee242) SHA1(924150d4e7e087a9b2b0a294c2d0e9903a266c6c) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/*
bublbob1.zip - a78-05.52
TAITO CORPORATION 1986
ALL RIGHTS RESERVED
VER 0.018.AUG,1986 SUMMER
*/

ROM_START( bublbob1 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06.51",    0x00000, 0x08000, CRC(32c8305b) SHA1(6bf69b3edfbefd33cd670a762b4bf0b39629a220) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-05.52",    0x10000, 0x10000, CRC(53f4bc6e) SHA1(15a2e6d83438d4136b154b3d90dd2cf9f1ce572c) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/*
bublbobr.zip - a78-24.52
1986 TAITO AMERICA CORP.
LICENSED TO ROMSTAR FOR U.S.A.
VER 5.1 8.NOV,1986 SUMMER
*/

ROM_START( bublbobr )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-25.51",    0x00000, 0x08000, CRC(2d901c9d) SHA1(72504225d3a26212e8f35508a79200eeb91138b6) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-24.52",    0x10000, 0x10000, CRC(b7afedc4) SHA1(6e4c8712f1fdf000e231cfd622dd3b514c61a6fd) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

/*
bubbobr1.zip - a78-21.52
1986 TAITO AMERICA CORP.
LICENSED TO ROMSTAR FOR U.S.A.
VER 1.0 26.AUG,1986 SUMMER
*/

ROM_START( bubbobr1 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "a78-06.51",    0x00000, 0x08000, CRC(32c8305b) SHA1(6bf69b3edfbefd33cd670a762b4bf0b39629a220) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "a78-21.52",    0x10000, 0x10000, CRC(2844033d) SHA1(6ac0b09d0325990cf18935f35b0adbc033758947) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 64k for the MCU */
	ROM_LOAD( "a78-01.17",    0xf000, 0x1000, CRC(b1bfb53d) SHA1(31b8f31acd3aa394acd80db362774749842e1285) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( boblbobl )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "bb3",          0x00000, 0x08000, CRC(01f81936) SHA1(a48489a13bfd01949e7fd273029d9cb8bfd7be48) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "bb5",          0x10000, 0x08000, CRC(13118eb1) SHA1(5a5da40c2cc82420f70bc58ffa32de1088c6c82f) )
	ROM_LOAD( "bb4",          0x18000, 0x08000, CRC(afda99d8) SHA1(304324074ae726501bbb08e683850639d69939fb) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END

ROM_START( sboblbob )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "bbb-3.rom",    0x00000, 0x08000, CRC(f304152a) SHA1(103d9beddccef289ed739d28ebda69bbad3d42f9) )
    /* ROMs banked at 8000-bfff */
	ROM_LOAD( "bb5",          0x10000, 0x08000, CRC(13118eb1) SHA1(5a5da40c2cc82420f70bc58ffa32de1088c6c82f) )
	ROM_LOAD( "bbb-4.rom",    0x18000, 0x08000, CRC(94c75591) SHA1(7698bc4b7d20e554a73a489cd3a15ae61b350e37) )
	/* 20000-2ffff empty */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "a78-08.37",    0x0000, 0x08000, CRC(ae11a07b) SHA1(af7a335c8da637103103cc274e077f123908ebb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the third CPU */
	ROM_LOAD( "a78-07.46",    0x0000, 0x08000, CRC(4f9a26e8) SHA1(3105b34b88a7134493c2b3f584729f8b0407a011) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "a78-09.12",    0x00000, 0x8000, CRC(20358c22) SHA1(2297af6c53d5807bf90a8e081075b8c72a994fc5) )    /* 1st plane */
	ROM_LOAD( "a78-10.13",    0x08000, 0x8000, CRC(930168a9) SHA1(fd358c3c3b424bca285f67a1589eb98a345ff670) )
	ROM_LOAD( "a78-11.14",    0x10000, 0x8000, CRC(9773e512) SHA1(33c1687ee575d66bf0e98add45d06da827813765) )
	ROM_LOAD( "a78-12.15",    0x18000, 0x8000, CRC(d045549b) SHA1(0c12077d3ddc2ce6aa45a0224ad5540f3f218446) )
	ROM_LOAD( "a78-13.16",    0x20000, 0x8000, CRC(d0af35c5) SHA1(c5a89f4d73acc0db86654540b3abfd77b3757db5) )
	ROM_LOAD( "a78-14.17",    0x28000, 0x8000, CRC(7b5369a8) SHA1(1307b26d80e6f36ebe6c442bebec41d20066eaf9) )
	/* 0x30000-0x3ffff empty */
	ROM_LOAD( "a78-15.30",    0x40000, 0x8000, CRC(6b61a413) SHA1(44eddf12fb46fceca2addbe6da929aaea7636b13) )    /* 2nd plane */
	ROM_LOAD( "a78-16.31",    0x48000, 0x8000, CRC(b5492d97) SHA1(d5b045e3ebaa44809757a4220cefb3c6815470da) )
	ROM_LOAD( "a78-17.32",    0x50000, 0x8000, CRC(d69762d5) SHA1(3326fef4e0bd86681a3047dc11886bb171ecb609) )
	ROM_LOAD( "a78-18.33",    0x58000, 0x8000, CRC(9f243b68) SHA1(32dce8d311a4be003693182a999e4053baa6bb0a) )
	ROM_LOAD( "a78-19.34",    0x60000, 0x8000, CRC(66e9438c) SHA1(b94e62b6fbe7f4e08086d0365afc5cff6e0ccafd) )
	ROM_LOAD( "a78-20.35",    0x68000, 0x8000, CRC(9ef863ad) SHA1(29f91b5a3765e4d6e6c3382db1d8d8297b6e56c8) )
	/* 0x70000-0x7ffff empty */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "a71-25.41",    0x0000, 0x0100, CRC(2d0f8545) SHA1(089c31e2f614145ef2743164f7b52ae35bc06808) )	/* video timing */
ROM_END



static DRIVER_INIT( bublbobl )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	/* in Bubble Bobble, bank 0 has code falling from 7fff to 8000, */
	/* so I have to copy it there because bank switching wouldn't catch it */
	memcpy(ROM + 0x08000, ROM + 0x10000, 0x4000);
}

static DRIVER_INIT( tokio )
{
	extern int bublbobl_video_enable;

	/* preemptively enable video, the bit is not mapped for this game and */
	/* I don't know if it even has it. */
	bublbobl_video_enable = 1;
}

static DRIVER_INIT( tokiob )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xfe00, 0xfe00, 0, 0, tokiob_mcu_r );

	init_tokio();
}



GAME( 1986, tokio,    0,        tokio,    tokio,    tokio,    ROT90, "Taito Corporation", "Tokio / Scramble Formation", GAME_NOT_WORKING )
GAME( 1986, tokiou,   tokio,    tokio,    tokio,    tokio,    ROT90, "Taito America Corporation (Romstar license)", "Tokio / Scramble Formation (US)", GAME_NOT_WORKING )
GAME( 1986, tokiob,   tokio,    tokio,    tokio,    tokiob,   ROT90, "bootleg", "Tokio / Scramble Formation (bootleg)", 0 )

GAME( 1986, bublbobl, 0,        bublbobl, bublbobl, bublbobl, ROT0,  "Taito Corporation", "Bubble Bobble", 0 )
GAME( 1986, bublbob1, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "Taito Corporation", "Bubble Bobble (older)", 0 )
GAME( 1986, bublbobr, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "Taito America Corporation (Romstar license)", "Bubble Bobble (US with mode select)", 0 )
GAME( 1986, bubbobr1, bublbobl, bublbobl, bublbobl, bublbobl, ROT0,  "Taito America Corporation (Romstar license)", "Bubble Bobble (US)", 0 )

GAME( 1986, boblbobl, bublbobl, boblbobl, boblbobl, bublbobl, ROT0,  "bootleg", "Bobble Bobble", 0 )
GAME( 1986, sboblbob, bublbobl, boblbobl, sboblbob, bublbobl, ROT0,  "bootleg", "Super Bobble Bobble", 0 )
