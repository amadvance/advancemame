/***************************************************************************
MPU4 highly preliminary driver, based on
Bellfruit scorpion1 driver, (under heavy construction !!!)

  28-04-2006: El Condor
  20-05-2004: Re-Animator

--- Board Setup ---

The MPU4 BOARD is the driver board, originally designed to run Fruit Machines made by the Barcrest Group, but later
licensed to other firms as a general purpose unit (even some old Photo-Me booths used the unit).

This original board uses a 1Mhz 6809 CPU, and a number of PIA6821 chips for multiplexing inputs and the like. (all wired
through the one IRQ)

A 6840PTM is used for internal timing, one of it's functions is to act with an AY8913 chip as a crude DAC device.

A MPU4 GAME CARD (cartridge) plugs into the MPU4 board containing the game, and a protection PAL (the 'characteriser').
This PAL, as well as protecting the games, also controlled some of the lamp address matrix for many games, and acted as
an anti-tampering device which helped to prevent the hacking of certain titles in a manner which broke UK gaming laws.

One of the advantages of the hardware setup was that the developer could change the nature of the game card
up to a point, adding extra lamp support, different amounts of RAM, and (in many cases) an OKI MSM6376 and related PIA for
improved ADPCM sample support (This was eventually endorsed in the most recent official 'MOD' of the board)

For the Barcrest MPU4 Video system, the cartridge contains the MPU4 video bios in the usual ROM space, interface chips to connect
an additional Video board, and a 6850 serial IO to communicate with the video board. This version of the game card does not
have the OKI chip.

The VIDEO BOARD is driven by a 10mhz 68000 processor, and contains a 6840PTM, 6850 serial IO (the other end of the
communications), an SAA1099 for stereo sound and SCN2674 gfx chip

The VIDEO CARTRIDGE plugs into the video board, and contains the program roms for the video based game. Like the MPU4 game
card, in some cases an extra OKI sound chip is added to the video board's game card,as well as extra RAM.
There is a protection chip (similar to and replacing the MPU4 Characteriser, which is often fed question data to descramble
(unknown how it works).

A decent schematic for the MPU4 board, amongst other info is available,
see http://www.mameworld.net/agemame/techinfo/mpu4.php .

No video card schematics ever left the PCB factory, but the map we have seems pretty decent.

Additional: 68k HALT line is tied to the reset circuit of the MPU4.

Emulating the standalone MPU4 is a priority, unless this works, the video won't run.

Everything here is preliminary...  the boards are quite fussy with regards their self tests
and the timing may have to be perfect for them to function correctly.  (as the comms are
timer driven, the video is capable of various raster effects etc.)

Datasheets are available for the main components, The AGEMAME site mirrors a few of the harder-to-find ones.

--- Preliminary MPU4 Memorymap  ---

(NV) indicates an item which is not present on the video version, which has a Comms card instead.

   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+--------------------------------------------------------------------------
 0000-07FF |R/W| D D D D D D D D | 2k RAM
-----------+---+-----------------+--------------------------------------------------------------------------
 0800      | ? |                 | Characteriser (Security PAL) (NV)
-----------+---+-----------------+--------------------------------------------------------------------------
 0880      |R/W| D D D D D D D D | PIA6821 on soundboard (Oki MSM6376@16MHz,(NV)
                                   port A = ??
                                   port B (882)
                                          b7 = 0 if OKI busy
                                               1 if OKI ready
                                          b6 = ??
                                          b5 = volume control clock
                                          b4 = volume control direction (0= up, 1 = down)
                                          b3 = ??
                                          b2 = ??
                                          b1 = ??
                                          b0 = ??
-----------+---+-----------------+--------------------------------------------------------------------------
 0850 ?    | W | ??????????????? | page latch (NV?)
-----------+---+-----------------+--------------------------------------------------------------------------
 08C0      |   |                 | MC6840 on sound board(NV?)
-----------+---+-----------------+--------------------------------------------------------------------------
 0900-     |R/W| D D D D D D D D | MC6840 PTM IC2

                                     IRQ line connected to CPU
  Output1 ---> Clock2

               Output2 --+-> Clock3
                         |
                         |   Output3 ---> 'to audio amp' ??
                         |
                         +--------> CA1 IC3 (


-----------+---+-----------------+--------------------------------------------------------------------------
 0A00-0A03 |R/W| D D D D D D D D | PIA6821 IC3 port A Lamp Drives 1,2,3,4,6,7,8,9 (sic)(IC14)

                                            CA1 <= output2 from PTM6840 (IC2)
                                            CA2 => alpha data

                                            port B Lamp Drives 10,11,12,13,14,15,16,17 (sic)(IC13)

                                            CB2 => alpha reset (clock on Dutch systems)
-----------+---+-----------------+--------------------------------------------------------------------------
 0B00-0B03 |R/W| D D D D D D D D | PIA6821 IC4 port A = data for 7seg leds (pins 10 - 17, via IC32)

                                               CA1 INPUT, 50 Hz input (used to generate IRQ)
                                               CA2 OUTPUT, connected to pin2 74LS138 CE for multiplexer
                                                          (B on LED strobe multiplexer)

                                               port B
                                                      PB7 = INPUT, serial port Receive data (Rx)
                                                      PB6 = INPUT, reel A sensor
                                                      PB5 = INPUT, reel B sensor
                                                      PB4 = INPUT, reel C sensor
                                                      PB3 = INPUT, reel D sensor
                                                      PB2 = INPUT, Connected to CA1 (50Hz signal)
                                                      PB1 = INPUT, undercurrent sense
                                                      PB0 = INPUT, overcurrent  sense

                                               CB1 INPUT,  used to generate IRQ on edge of serial input line
                                               CB2 OUTPUT, enable signal for reel optics
-----------+---+-----------------+--------------------------------------------------------------------------
 0C00-0C03 |R/W| D D D D D D D D | PIA6821 IC5 port A

                                                      PA0-PA7, INPUT AUX1 connector

                                               CA2  OUTPUT, serial port Transmit line
                                               CA1  not connected
                                               IRQA connected to IRQ of CPU

                                               port B

                                                      PB0-PB7 INPUT, AUX2 connector

                                               CB1  INPUT,  connected to PB7 (Aux2 connector pin 4)

                                               CB2  OUTPUT, AY8910 chip select line
                                               IRQB connected to IRQ of CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 0D00-0D03 |R/W| D D D D D D D D | PIA6821 IC6

                                    port A

                                          PA0 - PA7 (INPUT/OUTPUT) data port AY8910 sound chip

                                          CA1 INPUT,  not connected
                                          CA2 OUTPUT, BC1 pin AY8910 sound chip
                                          IRQA , connected to IRQ CPU

                                    port B

                                          PB0-PB3 OUTPUT, reel A
                                          PB4-PB7 OUTPUT, reel B

                                          CB1 INPUT,  not connected
                                          CB2 OUTPUT, B01R pin AY8910 sound chip
                                          IRQB , connected to IRQ CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 0E00-0E03 |R/W| D D D D D D D D | PIA6821 IC7

                                    port A

                                          PA0-PA3 OUTPUT, reel C
                                          PA4-PA7 OUTPUT, reel D
                                          CA1     INPUT,  not connected
                                          CA2     OUTPUT, A on LED strobe multiplexer
                                          IRQA , connected to IRQ CPU

                                    port B

                                          PB0-BP7 OUTPUT mech meter or reel E + F
                                          CB1     INPUT, not connected
                                          CB2     OUTPUT,enable mech meter latch
                                          IRQB , connected to IRQ CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 0F00-0F03 |R/W| D D D D D D D D | PIA6821 IC8

                                   port A

                                          PA0-PA7 INPUT  multiplexed inputs data

                                          CA1     INPUT, not connected
                                          CA2    OUTPUT, C on LED strobe multiplexer
                                          IRQA           connected to IRQ CPU

                                   port B

                                          PB0-PB7 OUTPUT  triacs outputs connector PL6
                                          used for slides / hoppers

                                          CB1     INPUT, not connected
                                          CB2    OUTPUT, pin1 alpha display PL7 (clock signal)
                                          IRQB           connected to IRQ CPU

-----------+---+-----------------+--------------------------------------------------------------------------
 1000-FFFF | R | D D D D D D D D | ROM (can be banked switched by ??? in 8 banks of 64 k )
-----------+---+-----------------+--------------------------------------------------------------------------

 4000, appears to be something special?

 TODO: - get better memorymap.
       - Get MPU4 board working properly, so that video layer will operate.
       - Confirm that MC6850 emulation is sufficient.
       - Confirm that MC6840 emulation actually works (it appears to, but it could do with another view).

*/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/6821pia.h"
#include "machine/6840ptm.h"
#include "machine/6850acia.h"
#include "machine/74148.h"

// MPU4
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "sound/saa1099.h"
//#include "vidhrdw/awpvid.h" Fruit Machines Only
#include "machine/lamps.h"
#include "machine/steppers.h" // stepper motor
#include "machine/vacfdisp.h"  // vfd
#include "machine/mmtr.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#define LOGSTUFF logerror

#define LOG_IC3(x)	logerror x

#define LOG_IC8(x)	//logerror x



void draw_7segment_led_temp(mame_bitmap *bitmap, int x, int y, UINT8 value, int col_on, int col_off)
{

	plot_box(bitmap, x-1, y-1, 7, 11, 0);

	/* Top */
	plot_box(bitmap, x+1, y+0, 3, 1, (value & 0x40) ? col_on : col_off);
	/* Middle */
	plot_box(bitmap, x+1, y+4, 3, 1, (value & 0x01) ? col_on : col_off);
	/* Bottom */
	plot_box(bitmap, x+1, y+8, 3, 1, (value & 0x08) ? col_on : col_off);
	/* Top Left */
	plot_box(bitmap, x+0, y+1, 1, 3, (value & 0x02) ? col_on : col_off);
	/* Top Right */
	plot_box(bitmap, x+4, y+1, 1, 3, (value & 0x20) ? col_on : col_off);
	/* Bottom Left */
	plot_box(bitmap, x+0, y+5, 1, 3, (value & 0x04) ? col_on : col_off);
	/* Bottom Right */
	plot_box(bitmap, x+4, y+5, 1, 3, (value & 0x10) ? col_on : col_off);
}

// local prototypes ///////////////////////////////////////////////////////


// local vars /////////////////////////////////////////////////////////////

static int alpha_data_line;
static int alpha_clock;
static int ay8910_address;
//static int expansion_latch;
//static int global_volume;

static int signal_50hz;
static int ic4_input_b;
static int IC23G2A;
static int IC23G23;
static int IC23G1;

static UINT8 m6840_irq_state;
extern UINT8 m6850_irq_state; // referenced in machine/6850acia.c
static UINT8 scn2674_irq_state;
static void update_irq(void);


// user interface stuff ///////////////////////////////////////////////////

static UINT8 lamps[224];		// 224 multiplexed lamps  (2 8X8 matrices) PIAs take care of this in some form
//static UINT8 inputs[32];      // 32  multiplexed inputs - but a further 8 possible per AUX.
								// Two connectors 'orange' (sampled every 8ms) and 'black' (sampled every 16ms)
								// Each connector carries two banks of eight inputs and two enable signals

static int optic_pattern;
static UINT16 lamp_strobe;
static UINT8  lamp_data;

static UINT8  IC3ca1;

//static int   led_mux_strobe;
static UINT8 led_segs[8];
/*
LED Segments related to pins (5 is not connected):
   _9_
  |   |
  3   8
  |   |
   _2_
  |   |
  4   7
  |   |
   _6_1

8 display enables (pins 10 - 17)
*/
static int    input_strobe;	  // IC23 74LS138 A = CA2 IC7, B = CA2 IC4, C = CA2 IC8
static int    output_strobe;  // same

// \MPU4

static int mpu4_gfx_index;
static UINT16 * mpu4_vid_vidram;
static UINT8 * mpu4_vid_vidram_is_dirty;
static UINT16 * mpu4_vid_mainram;

static UINT8 scn2674_IR[16];
static UINT8 scn2675_IR_pointer;
static UINT8 scn2674_screen1_l;
static UINT8 scn2674_screen1_h;
static UINT8 scn2674_cursor_l;
static UINT8 scn2674_cursor_h;
static UINT8 scn2674_screen2_l;
static UINT8 scn2674_screen2_h;


/*************************************
 *
 *  Interrupt system
 *
 *************************************/

/* the interrupt system consists of a 74148 priority encoder
   with the following interrupt priorites.  A lower number
   indicates a lower priority:

    7 - Game Card
    6 - Game Card
    5 - Game Card
    4 - Game Card
    3 - 2674 AVDC
    2 - 6850 ACIA
    1 - 6840 PTM
    0 - Unused (no such IRQ on 68k)
*/

void update_mpu68_interrupts(void)
{
	int newstate = 0;

	if (m6840_irq_state)//1
		newstate = 1;
	if (m6850_irq_state)//2
		newstate = 2;
	if (scn2674_irq_state)//3
		newstate = 3;

	m6840_irq_state = 0;
	m6850_irq_state = 0;
	scn2674_irq_state = 0;

	/* set the new state of the IRQ lines */
	if (newstate)
		cpunum_set_input_line(1, newstate, HOLD_LINE);
	else
		cpunum_set_input_line(1, 7, CLEAR_LINE);
}


// MPU4
///////////////////////////////////////////////////////////////////////////
// called if board is reset ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static MACHINE_RESET( mpu4_vid )
{
	vfd_reset(0);	// reset display1

// reset stepper motors /////////////////////////////////////////////////
	{
		int pattern =0,i;

		for ( i = 0; i < 6; i++)
		{
			Stepper_reset_position(i);
			if ( Stepper_optic_state(i) ) pattern |= 1<<i;
		}
		optic_pattern = pattern;
	}

	//led_mux_strobe = 0;
	lamp_strobe    = 0;
	lamp_data      = 0;

	IC23G2A    = 0;
	IC23G23    = 0;
	IC23G1     = 1;

	uart1_status  = 0x02; // MC6850 transmit buffer empty !!!
	uart2_status  = 0x02; // MC6850 transmit buffer empty !!!
	vid_rx                = 0;
	vid_acia_triggered    = 0;
	vid_data_from_norm    = 0;
	norm_data_from_vid    = 0;

	#if 0
// init rom bank ////////////////////////////////////////////////////////

		{
			UINT8 *rom = memory_region(REGION_CPU1);

			memory_configure_bank(1, 0, 8, &rom[0x10000], 0x10000);

			memory_set_bank(1,data&0x03);//not correct
		}
	#endif
}

///////////////////////////////////////////////////////////////////////////

static void cpu0_irq(int state)
{
	cpunum_set_input_line(0, M6809_IRQ_LINE, PULSE_LINE);
}


///////////////////////////////////////////////////////////////////////////
#if 0
static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1,data & 0x03);//?
}
#endif

// IC2 6840 PTM handler ///////////////////////////////////////////////////////

static WRITE8_HANDLER( ic2_o1_callback )
{
	ptm6840_set_c2(0,data);

	// copy output value to IC2 c2
	// this output is the clock for timer2,
	// the output from timer2 is the input clock for timer3
	// the output from timer3 is used as a square wave for the audio output !!!

}

static WRITE8_HANDLER( ic2_o2_callback )
{
	pia_set_input_ca1(0, data); // copy output value to IC3 ca1
	IC3ca1 = data;
	ptm6840_set_c3(   0, data); // copy output value to IC2 c3
}

static WRITE8_HANDLER( ic2_o3_callback )
{
	pia_set_input_a(3, data);
}

static const ptm6840_interface ptm_ic2_intf =
{
	ic2_o1_callback, ic2_o2_callback, ic2_o3_callback,
	cpu0_irq
};

static void cpu1_irq(int state)
{
	m6840_irq_state = state;
	logerror("PTM IRQ triggered \n");
	update_mpu68_interrupts();
}

static const ptm6840_interface ptm_vid_intf =
{
	NULL, NULL, NULL,
	cpu1_irq
};

/***************************************************************************
    6821 PIA handlers
***************************************************************************/

static WRITE8_HANDLER( pia_ic3_porta_w )
{
	LOG_IC3(("%04x IC3 PIA Port A Set to %2x (lamp strobes 1 - 9)\n", activecpu_get_previouspc(),data));

	lamp_strobe = (lamp_strobe & 0xFF00) | data;
}

static WRITE8_HANDLER( pia_ic3_portb_w )
{
	LOG_IC3(("%04x IC3 PIA Port B Set to %2x  (lamp strobes 10 - 17)\n", activecpu_get_previouspc(),data));

	lamp_strobe = (lamp_strobe & 0x00FF) | (data<<8);
}

static READ8_HANDLER( pia_ic3_porta_r )
{
	LOG_IC3(("%04x IC3 PIA Read of Port A (lamp strobes 1 - 9)\n",activecpu_get_previouspc()));
	return lamp_strobe;
}

static READ8_HANDLER( pia_ic3_portb_r )
{
	LOG_IC3(("%04x IC3 PIA Read of Port B (lamp strobes 10 - 17)\n",activecpu_get_previouspc()));
	return lamp_strobe;
}

static WRITE8_HANDLER( pia_ic3_ca2_w )
{
	LOG_IC3(("%04x IC3 PIA Write CA (alpha data), %02X\n", activecpu_get_previouspc(),data&0xFF));

	alpha_data_line = data;
}


static WRITE8_HANDLER( pia_ic3_cb2_w )
{
	LOG_IC3(("%04x IC3 PIA Write CB (alpha reset), %02X\n",activecpu_get_previouspc(),data&0xff));

	if ( data & 1 ) vfd_reset(0);

}

static READ8_HANDLER( pia_ic3_ca1_r )
{
	LOG_IC3(("%04x IC3 PIA Read CA1 (PTM) %x \n",activecpu_get_previouspc(),IC3ca1));
	return IC3ca1;
}

static READ8_HANDLER( pia_ic3_ca2_r )
{
//  LOG_IC3(("%04x IC3 PIA Read CA2\n",activecpu_get_previouspc()));
	return alpha_data_line;//0;
}
static READ8_HANDLER( pia_ic3_cb1_r )
{
//  LOG_IC3(("%04x IC3 PIA Read CB1\n",activecpu_get_previouspc()));
	return 0;
}
static READ8_HANDLER( pia_ic3_cb2_r )
{
	LOG_IC3(("%04x IC3 PIA Read CB2\n",activecpu_get_previouspc()));
	return 0;
}


// IC3, lamp data lines + alpha numeric display

static const pia6821_interface pia_ic3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic3_porta_r, pia_ic3_portb_r, pia_ic3_ca1_r, pia_ic3_cb1_r, pia_ic3_ca2_r, pia_ic3_cb2_r,
	/*outputs: A/B,CA/B2       */ pia_ic3_porta_w, pia_ic3_portb_w, pia_ic3_ca2_w, pia_ic3_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic4_porta_w )
{
	if (IC23G2A ==0)
	{
		if (IC23G23 ==0)
		{
			if (IC23G1)
			{
				switch (input_strobe)
				{
					case 0x00:
					led_segs[0] = data;
					case 0x01:
					led_segs[1] = data;
					case 0x02:
					led_segs[2] = data;
					case 0x03:
					led_segs[3] = data;
					case 0x04:
					led_segs[4] = data;
					case 0x05:
					led_segs[5] = data;
					case 0x06:
					led_segs[6] = data;
					case 0x07:
					led_segs[7] = data;
				}
			}
		}
	}
//  int p,i;

	logerror("%04x IC4 PIA Port A Set to %2x\n", activecpu_get_previouspc(),data);

//  p = 1;
//  i = 0;
//  while ( p )
//  {
//      if ( led_mux_strobe & p ) led_segs[i] = data;
//      i++;
//      p <<= 1;
//  }
}

static WRITE8_HANDLER( pia_ic4_portb_w )
{
	logerror("%04x IC4 PIA Port B Set to %2x\n", activecpu_get_previouspc(),data);
}

static READ8_HANDLER( pia_ic4_porta_r )
{
	logerror("%04x IC4 PIA Read of Port A\n",activecpu_get_previouspc());
	return 0;
}

static READ8_HANDLER( pia_ic4_portb_r )
{
	logerror("%04x IC4 PIA Read of Port B\n",activecpu_get_previouspc());

	// if ( serial_data  ) ic4_input_b |=  0x80;
	// else                ic4_input_b &= ~0x80;

	if ( optic_pattern & 0x01 ) ic4_input_b |=  0x40; // reel A tab
	else                        ic4_input_b &= ~0x40;

	if ( optic_pattern & 0x02 ) ic4_input_b |=  0x20; // reel B tab
	else                        ic4_input_b &= ~0x20;

	if ( optic_pattern & 0x04 ) ic4_input_b |=  0x10; // reel C tab
	else                        ic4_input_b &= ~0x10;

	if ( optic_pattern & 0x08 ) ic4_input_b |=  0x08; // reel D tab
	else                        ic4_input_b &= ~0x08;

	// if ( lamp_overcurrent  ) ic4_input_b |= 0x02;
	// if ( lamp_undercurrent ) ic4_input_b |= 0x01;
	if (offset & 2)
	{
	return signal_50hz;
	}
	return ic4_input_b;
}

static READ8_HANDLER( pia_ic4_ca1_r )
{
	return signal_50hz?1:0;
}

static READ8_HANDLER( pia_ic4_cb1_r )
{
	return 0;
}

static READ8_HANDLER( pia_ic4_cb2_r )
{
	return 0;
}

static WRITE8_HANDLER( pia_ic4_ca2_w )
{
	LOG_IC3(("%04x IC4 PIA Write CA (input MUX strobe /LED B), %02X\n", activecpu_get_previouspc(),data&0xFF));

	if ( data )  input_strobe |=  0x02;
	else         input_strobe &= ~0x02;

	output_strobe = input_strobe; // same strobe lines are used for input and output
}


static WRITE8_HANDLER( pia_ic4_cb2_w )
{
	LOG_IC3(("%04x I43 PIA Write CB (T39, connector PL13 pin 2), %02X\n",activecpu_get_previouspc(),data&0xff));
}

// IC4, 7 seg leds, 50Hz timer reel sensors, current sensors


static const pia6821_interface pia_ic4_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic4_porta_r, pia_ic4_portb_r, pia_ic4_ca1_r, pia_ic4_cb1_r, 0, pia_ic4_cb2_r,
	/*outputs: A/B,CA/B2       */ pia_ic4_porta_w, pia_ic4_portb_w, pia_ic4_ca2_w, pia_ic4_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic5_porta_w )
{

	int changed = lamp_data ^ data;

	lamp_data = data;

	logerror("%04x IC5 PIA Port A Set to %2x\n", activecpu_get_previouspc(), data);

	if ( changed & 0x80 )
	{
		if ( !(data & 0x80) )
		{
			int    i = 0;
			UINT16 p = 1;

			while ( p )
		{
			if ( lamp_strobe & p ) lamps[i<<3] = data;

			p <<= 1;
			i++;
		}

	Lamps_SetBrightness(0, 255, lamps);
		}
	}

}

static WRITE8_HANDLER( pia_ic5_portb_w )
{
	logerror("%04x IC5 PIA Port B Set to %2x\n", activecpu_get_previouspc(), data);
	pia_set_input_b(2, data);
//  led_mux_strobe = data;
}

static READ8_HANDLER( pia_ic5_porta_r )
{
	logerror("%04x IC5 PIA Read of Port A\n",activecpu_get_previouspc());
	return readinputportbytag("AUX1");
//  return 0;
}

static READ8_HANDLER( pia_ic5_portb_r )
{
	//int result = 0;
	logerror("%04x IC5 PIA Read of Port B (coin input AUX2 )\n",activecpu_get_previouspc());

	//result = coin_latch()
	return readinputportbytag("AUX2");
	//return result;
}

static READ8_HANDLER( pia_ic5_ca1_r )
{
	logerror("%04x IC5 PIA Read of CA1\n",activecpu_get_previouspc());
	return 0;
}

static READ8_HANDLER( pia_ic5_ca2_r )
{
	logerror("%04x IC5 PIA Read of CA2\n",activecpu_get_previouspc());
	return 0;
}

static READ8_HANDLER( pia_ic5_cb1_r )
{
	logerror("%04x IC5 PIA Read of CA1\n",activecpu_get_previouspc());
	return readinputportbytag("AUX2") & 0x40;
}

static READ8_HANDLER( pia_ic5_cb2_r )
{
	logerror("%04x IC5 PIA Read of CB2\n",activecpu_get_previouspc());
	return 0;
}

static WRITE8_HANDLER( pia_ic5_ca_w )
{
	logerror("%04x IC5 PIA Write CA2\n",activecpu_get_previouspc());
}

static WRITE8_HANDLER( pia_ic5_cb_w )
{
	logerror("%04x IC5 PIA Write CB2\n",activecpu_get_previouspc());
	//ay8912 chipsel
}

static const pia6821_interface pia_ic5_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic5_porta_r, pia_ic5_portb_r, 0, pia_ic5_cb1_r, pia_ic5_ca2_r, pia_ic5_cb2_r,//CB1 connected to PB7 Aux 2
	/*outputs: A/B,CA/B2       */ pia_ic5_porta_w, pia_ic5_portb_w, pia_ic5_ca_w,  pia_ic5_cb_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic6_ca2_w )
{
	logerror("%04x IC6 PIA write CA2 %2x (AY8912 register)\n", activecpu_get_previouspc(),data);

	ay8910_address = data;
	AY8910_write_port_0_w(0, data);
}

static WRITE8_HANDLER( pia_ic6_porta_w )
{
	logerror("%04x IC6 PIA Write A %2x\n", activecpu_get_previouspc(),data);

	//AY8910_control_port_0_w(0, data);
}

static WRITE8_HANDLER( pia_ic6_portb_w )
{
	logerror("%04x IC6 PIA Port B Set to %2x\n", activecpu_get_previouspc(),data);

	Stepper_update(0, (data >> 4) & 0x0F );
	Stepper_update(1, data        & 0x0F );
}

static READ8_HANDLER( pia_ic6_porta_r )
{
	logerror("%04x IC6 PIA Read of Port A\n",activecpu_get_previouspc());
	return 0;
//  return AY8910_read_port_0_r;
}

static READ8_HANDLER( pia_ic6_portb_r )
{
	logerror("%04x IC6 PIA Read of Port B\n",activecpu_get_previouspc());
	return 0;
}

static READ8_HANDLER( pia_ic6_cb1_r )
{
	return 0;
}

static const pia6821_interface pia_ic6_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic6_porta_r, pia_ic6_portb_r, 0, pia_ic6_cb1_r, 0, 0,
	/*outputs: A/B,CA/B2       */ AY8910_control_port_0_w, pia_ic6_portb_w, 0, 0,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic7_porta_w )
{
	logerror("%04x IC7 PIA Port A Set to %2x (Reel C and D)\n", activecpu_get_previouspc(),data);

	Stepper_update(2, (data >> 4) & 0x0F );
	Stepper_update(3, data        & 0x0F );

}

static WRITE8_HANDLER( pia_ic7_portb_w )
{
	logerror("%04x IC7 PIA Port B Set to %2x (Meters, Reel E and F)\n", activecpu_get_previouspc(),data);
//  Stepper_update(4, (data >> 4) & 0x0F );
//  Stepper_update(5, data        & 0x0F );

}

static READ8_HANDLER( pia_ic7_porta_r )
{
	logerror("%04x IC7 PIA Read of Port A\n",activecpu_get_previouspc());
	return 0;
}

static READ8_HANDLER( pia_ic7_portb_r )
{
	logerror("%04x IC7 PIA Read of Port B\n",activecpu_get_previouspc());
	return 0;
}

static READ8_HANDLER( pia_ic7_cb2_r )
{
	logerror("%04x IC7 PIA read CB2 (meter driver PL3)\n", activecpu_get_previouspc());
	return 0;
}

static WRITE8_HANDLER( pia_ic7_ca2_w )
{
	logerror("%04x IC7 PIA write CA2 %2x (input strobe bit 0 / LED A)\n", activecpu_get_previouspc(),data);

	if ( data ) input_strobe |=  0x01;
	else        input_strobe &= ~0x01;

	output_strobe = input_strobe; // same strobe lines are used for input and output
}

static WRITE8_HANDLER( pia_ic7_cb2_w )
{
	logerror("%04x IC7 PIA write CB2 %2x (meter driver PL3)\n", activecpu_get_previouspc(),data);
}

static const pia6821_interface pia_ic7_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic7_porta_r, pia_ic7_portb_r, 0, 0, 0, pia_ic7_cb2_r,
	/*outputs: A/B,CA/B2       */ pia_ic7_porta_w, pia_ic7_portb_w, pia_ic7_ca2_w, pia_ic7_cb2_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

static WRITE8_HANDLER( pia_ic8_porta_w )
{
	LOG_IC8(("%04x IC8 PIA Port A Set to %2x (MUX'd Inputs)\n", activecpu_get_previouspc(),data));
}

static WRITE8_HANDLER( pia_ic8_portb_w )
{
	LOG_IC8(("%04x IC8 PIA Port B Set to %2x (OUTPUT PORT, TRIACS)\n", activecpu_get_previouspc(),data));
}

static READ8_HANDLER( pia_ic8_porta_r )
{
	LOG_IC8(("%04x IC8 PIA Read of Port A (MUX input data)\n",activecpu_get_previouspc()));

	// strobe
	return readinputport(input_strobe&0x07);//inputs[ input_strobe&0x07 ];
}

static READ8_HANDLER( pia_ic8_portb_r )
{
	LOG_IC8(("%04x IC8 PIA Read of Port B\n (Triacs)",activecpu_get_previouspc()));
	return 0;
}

static READ8_HANDLER( pia_ic8_ca1_r )
{
	LOG_IC8(("%04x IC8 PIA Read of CA1 (Should be unconnected)\n",activecpu_get_previouspc()));
	return 0;
}

static READ8_HANDLER( pia_ic8_ca2_r )
{
	LOG_IC8(("%04x IC8 PIA Read of CA2 (LED Multiplexer 'C')\n",activecpu_get_previouspc()));
	return 0;
}

static WRITE8_HANDLER( pia_ic8_cb_w )
{
	LOG_IC8(("%04x IC8 PIA write CB (alpha clock) %02X\n", activecpu_get_previouspc(), data & 0xFF));

	if ( !alpha_clock && (data & 1) )
	{
		vfd_shift_data(0, alpha_data_line&0x01?0:1);
	}
	alpha_clock = data;
}

static WRITE8_HANDLER( pia_ic8_ca2_w )
{
	LOG_IC8(("%04x IC8 PIA write CA2 (input_strobe bit 2 / LED C) %02X\n", activecpu_get_previouspc(), data & 0xFF));

	if ( data ) input_strobe |=  0x04;
	else        input_strobe &= ~0x04;

	output_strobe = input_strobe; // same strobe lines are used for input and output
}

static const pia6821_interface pia_ic8_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_ic8_porta_r, pia_ic8_portb_r, 0, 0, pia_ic8_ca2_r, 0,
	/*outputs: A/B,CA/B2       */ pia_ic8_porta_w, pia_ic8_portb_w, pia_ic8_ca2_w, pia_ic8_cb_w,
	/*irqs   : A/B             */ cpu0_irq, cpu0_irq
};

// \MPU4
// SCN2674 AVDC emulation

/* the chip is actually more complex than this.. character aren't limited to 8 rows high... but I
 don't *think* the MPU4 stuff needs otherwise.. yet .. */

static const gfx_layout mpu4_vid_char_8x8_layout =
{
	8,8,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	8*32
};

/* double height */
static const gfx_layout mpu4_vid_char_8x16_layout =
{
	8,16,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 0*32, 1*32, 1*32, 2*32, 2*32, 3*32, 3*32, 4*32, 4*32, 5*32, 5*32, 6*32, 6*32, 7*32, 7*32},
	8*32
};

/* double width */
static const gfx_layout mpu4_vid_char_16x8_layout =
{
	16,8,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	8*32
};
/* double height & width */
static const gfx_layout mpu4_vid_char_16x16_layout =
{
	16,16,
	0x1000, // 0x1000 tiles (128k of GFX Ram, 0x20 bytes per tile)
	4,
	{ 0,8,16,24 },
	{ 0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7 },
	{ 0*32, 0*32, 1*32, 1*32, 2*32, 2*32, 3*32, 3*32, 4*32, 4*32, 5*32, 5*32, 6*32, 6*32, 7*32, 7*32},
	8*32
};

///////////////////////////////////////////////////////////////////////////

static UINT8 IR0_scn2674_double_ht_wd;
static UINT8 IR0_scn2674_scanline_per_char_row;
static UINT8 IR0_scn2674_sync_select;
static UINT8 IR0_scn2674_buffer_mode_select;
static UINT8 IR1_scn2674_interlace_enable;
static UINT8 IR1_scn2674_equalizing_constant;
static UINT8 IR2_scn2674_row_table;
static UINT8 IR2_scn2674_horz_sync_width;
static UINT8 IR2_scn2674_horz_back_porch;
static UINT8 IR3_scn2674_vert_front_porch;
static UINT8 IR3_scn2674_vert_back_porch;
static UINT8 IR4_scn2674_rows_per_screen;
static UINT8 IR4_scn2674_character_blink_rate;
static UINT8 IR5_scn2674_character_per_row;
static UINT8 IR8_scn2674_display_buffer_first_address_LSB;
static UINT8 IR9_scn2674_display_buffer_first_address_MSB;
static UINT8 IR9_scn2674_display_buffer_last_address;
static UINT8 IR10_scn2674_display_pointer_address_lower;
static UINT8 IR11_scn2674_display_pointer_address_upper;
static UINT8 IR12_scn2674_scroll_start;
static UINT8 IR12_scn2674_split_register_1;
static UINT8 IR13_scn2674_scroll_end;
static UINT8 IR13_scn2674_split_register_2;

VIDEO_UPDATE( mpu4_vid )
{
	int i;

	int x,y,count = 0;

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	for (i=0; i < 0x1000; i++)
		if (mpu4_vid_vidram_is_dirty[i]==1)
		{
			decodechar(Machine->gfx[mpu4_gfx_index+0], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_8x8_layout);
			decodechar(Machine->gfx[mpu4_gfx_index+1], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_8x16_layout);
			decodechar(Machine->gfx[mpu4_gfx_index+2], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_16x8_layout);
			decodechar(Machine->gfx[mpu4_gfx_index+3], i, (UINT8 *)mpu4_vid_vidram, &mpu4_vid_char_16x16_layout);
			mpu4_vid_vidram_is_dirty[i]=0;
		}


	/* this is in main ram.. i think it must transfer it out of here??? */
//  count = 0x0018b6/2; // crmaze
//  count = 0x004950/2; // turnover

	/* we're in row table mode...thats why */

	for(y=0;y<=IR4_scn2674_rows_per_screen;y++)
	{
		int screen2_base = (scn2674_screen2_h<<8)|(scn2674_screen2_l);

		UINT16 rowbase = (mpu4_vid_mainram[1+screen2_base+(y*2)]<<8)|mpu4_vid_mainram[screen2_base+(y*2)];
		int dbl_size;
		int gfxregion = 0;

		dbl_size = (rowbase & 0xc000)>>14;  // ONLY if double size is enabled.. otherwise it can address more chars given more RAM

		if (dbl_size&2) gfxregion = 1;


		for(x=0;x<=IR5_scn2674_character_per_row;x++)
		{
			UINT16 tiledat;
			UINT16 colattr;

			tiledat = mpu4_vid_mainram[(rowbase+x)&0x7fff];
			colattr = tiledat >>12;
			tiledat &= 0x0fff;

			drawgfx(bitmap,Machine->gfx[gfxregion],tiledat,colattr,0,0,x*8,y*8,cliprect,TRANSPARENCY_NONE,0);

			count++;
		}

		if (dbl_size&2) y++; // skip a row?

	}

	ui_popup("%02x %02x %02x %02x %02x %02x",scn2674_screen1_l,scn2674_screen1_h,scn2674_cursor_l, scn2674_cursor_h,scn2674_screen2_l,scn2674_screen2_h);

	draw_7segment_led_temp(bitmap, 300, 504, led_segs[0], 4, 0);
	draw_7segment_led_temp(bitmap, 308, 504, led_segs[1], 4, 0);
	draw_7segment_led_temp(bitmap, 316, 504, led_segs[2], 4, 0);
	draw_7segment_led_temp(bitmap, 324, 504, led_segs[3], 4, 0);
	draw_7segment_led_temp(bitmap, 332, 504, led_segs[4], 4, 0);
	draw_7segment_led_temp(bitmap, 340, 504, led_segs[5], 4, 0);
	draw_7segment_led_temp(bitmap, 348, 504, led_segs[6], 4, 0);
	draw_7segment_led_temp(bitmap, 356, 504, led_segs[7], 4, 0);

}

READ16_HANDLER( mpu4_vid_vidram_r )
{
	return mpu4_vid_vidram[offset];
}

WRITE16_HANDLER( mpu4_vid_vidram_w )
{
	COMBINE_DATA(&mpu4_vid_vidram[offset]);
	offset <<= 1;
	mpu4_vid_vidram_is_dirty[offset/0x20]=1;
}

/*
SCN2674 - Advanced Video Display Controller (AVDC)  (Video Chip)

15 Initalization Registers (8-bit each)

-- fill me in later ---

IR0  ---- ----
IR1  ---- ----
IR2  ---- ----
IR3  ---- ----
IR4  ---- ----
IR5  ---- ----
IR6  ---- ----
IR7  ---- ----
IR8  ---- ----
IR9  ---- ----
IR10 ---- ----
IR11 ---- ----
IR12 ---- ----
IR13 ---- ----
IR14 ---- ----

*/

void scn2674_write_init_regs(UINT8 data)
{
	LOGSTUFF("scn2674_write_init_regs %02x %02x\n",scn2675_IR_pointer,data);

	scn2674_IR[scn2675_IR_pointer]=data;


	switch ( scn2675_IR_pointer) // display some debug info, set mame specific variables
	{
		case 0:
			IR0_scn2674_double_ht_wd = (data & 0x80)>>7;
			IR0_scn2674_scanline_per_char_row = (data & 0x78)>>3;
			IR0_scn2674_sync_select = (data&0x04)>>2;
			IR0_scn2674_buffer_mode_select = (data&0x03);

		   	LOGSTUFF("IR0 - Double Ht Wd %02x\n",IR0_scn2674_double_ht_wd);
		   	LOGSTUFF("IR0 - Scanlines per Character Row %02x\n",IR0_scn2674_scanline_per_char_row);
		   	LOGSTUFF("IR0 - Sync Select %02x\n",IR0_scn2674_sync_select);
		   	LOGSTUFF("IR0 - Buffer Mode Select %02x\n",IR0_scn2674_buffer_mode_select);
			break;

		case 1:
			IR1_scn2674_interlace_enable = (data&0x80)>>7;
			IR1_scn2674_equalizing_constant = (data&0x7f);

		   	LOGSTUFF("IR1 - Interlace Enable %02x\n",IR1_scn2674_interlace_enable);
			LOGSTUFF("IR1 - Equalizing Constant %02x\n",IR1_scn2674_equalizing_constant);
			break;

		case 2:
			IR2_scn2674_row_table = (data&0x80)>>7;
			IR2_scn2674_horz_sync_width = (data&0x78)>>3;
			IR2_scn2674_horz_back_porch = (data&0x07);

		   	LOGSTUFF("IR2 - Row Table %02x\n",IR2_scn2674_row_table);
			LOGSTUFF("IR2 - Horizontal Sync Width %02x\n",IR2_scn2674_horz_sync_width);
			LOGSTUFF("IR2 - Horizontal Back Porch %02x\n",IR2_scn2674_horz_back_porch);
			break;

		case 3:
			IR3_scn2674_vert_front_porch = (data&0xe0)>>5;
			IR3_scn2674_vert_back_porch = (data&0x1f)>>0;

		   	LOGSTUFF("IR3 - Vertical Front Porch %02x\n",IR3_scn2674_vert_front_porch);
		   	LOGSTUFF("IR3 - Vertical Back Porch %02x\n",IR3_scn2674_vert_back_porch);
			break;

		case 4:
		   	IR4_scn2674_rows_per_screen = data&0x7f;
			IR4_scn2674_character_blink_rate = (data & 0x80)>>7;

		   	LOGSTUFF("IR4 - Rows Per Screen %02x\n",IR4_scn2674_rows_per_screen);
		   	LOGSTUFF("IR4 - Character Blink Rate %02x\n",IR4_scn2674_character_blink_rate);
			break;

		case 5:
		   /* IR5 - Active Characters Per Row
             cccc cccc
             c = Characters Per Row */
		   	IR5_scn2674_character_per_row = data;
		   	LOGSTUFF("IR5 - Active Characters Per Row %02x\n",IR5_scn2674_character_per_row);
			break;

		case 6:
			break;

		case 7:
			break;

		case 8:
			IR8_scn2674_display_buffer_first_address_LSB = data;
		   	LOGSTUFF("IR8 - Display Buffer First Address LSB %02x\n",IR8_scn2674_display_buffer_first_address_LSB);
			break;

		case 9:
			IR9_scn2674_display_buffer_first_address_MSB = data & 0x0f;
			IR9_scn2674_display_buffer_last_address = (data & 0xf0)>>4;
		   	LOGSTUFF("IR9 - Display Buffer First Address MSB %02x\n",IR9_scn2674_display_buffer_first_address_MSB);
		   	LOGSTUFF("IR9 - Display Buffer Last Address %02x\n",IR9_scn2674_display_buffer_last_address);
			break;

		case 10:
			IR10_scn2674_display_pointer_address_lower = data;
		   	LOGSTUFF("IR10 - Display Pointer Address Lower %02x\n",IR10_scn2674_display_pointer_address_lower);
			break;

		case 11:
			IR11_scn2674_display_pointer_address_upper= data&0x3f;
		   	LOGSTUFF("IR11 - Display Pointer Address Lower %02x\n",IR11_scn2674_display_pointer_address_upper);
			break;

		case 12:
			IR12_scn2674_scroll_start = (data & 0x80)>>7;
			IR12_scn2674_split_register_1 = (data & 0x7f);
		   	LOGSTUFF("IR12 - Scroll Start %02x\n",IR12_scn2674_scroll_start);
		   	LOGSTUFF("IR12 - Split Register 1 %02x\n",IR12_scn2674_split_register_1);
			break;

		case 13:
			IR13_scn2674_scroll_end = (data & 0x80)>>7;
			IR13_scn2674_split_register_2 = (data & 0x7f);
		   	LOGSTUFF("IR13 - Scroll End %02x\n",IR13_scn2674_scroll_end);
		   	LOGSTUFF("IR13 - Split Register 2 %02x\n",IR13_scn2674_split_register_2);
			break;

		case 14:
			break;

		case 15: // not valid!
			break;


	}

	scn2675_IR_pointer++;
	if (scn2675_IR_pointer>14)scn2675_IR_pointer=14;
}

static UINT8 scn2674_irq_register = 0;
static UINT8 scn2674_status_register = 0;
static UINT8 scn2674_irq_mask = 0;
static UINT8 scn2674_gfx_enabled;
static UINT8 scn2674_display_enabled;
static UINT8 scn2674_cursor_enabled;

void scn2674_write_command(UINT8 data)
{
	UINT8 oprand;

	LOGSTUFF("scn2674_write_command %02x\n",data);

	if (data==0x00)
	{
		// master reset
		LOGSTUFF("master reset %02x\n",data);
	}

	if ((data&0xf0)==0x10)
	{
		// set IR pointer
		LOGSTUFF("set IR pointer %02x\n",data);

		oprand = data & 0x0f;
		scn2675_IR_pointer=oprand;

	}

	/*** ANY COMBINATION OF THESE ARE POSSIBLE */

	if ((data&0xe3)==0x22)
	{
		// Disable GFX
		LOGSTUFF("disable GFX %02x\n",data);
		scn2674_gfx_enabled = 0;
	}

	if ((data&0xe3)==0x23)
	{
		// Enable GFX
		LOGSTUFF("enable GFX %02x\n",data);
		scn2674_gfx_enabled = 1;
	}

	if ((data&0xe9)==0x28)
	{
		// Display off
		oprand = data & 0x04;

		scn2674_display_enabled = 0;

		if (oprand)
			LOGSTUFF("display OFF - float DADD bus %02x\n",data);
		else
			LOGSTUFF("display OFF - no float DADD bus %02x\n",data);
	}

	if ((data&0xe9)==0x29)
	{
		// Display on
		oprand = data & 0x04;

		scn2674_display_enabled = 1;

		if (oprand)
			LOGSTUFF("display ON - next field %02x\n",data);
		else
			LOGSTUFF("display ON - next scanline %02x\n",data);

	}

	if ((data&0xf1)==0x30)
	{
		// Cursor Off
		LOGSTUFF("cursor off %02x\n",data);
		scn2674_cursor_enabled = 0;
	}

	if ((data&0xf1)==0x31)
	{
		// Cursor On
		LOGSTUFF("cursor on %02x\n",data);
		scn2674_cursor_enabled = 1;
	}

	/*** END ***/

	if ((data&0xe0)==0x40)
	{
		// Reset Interrupt / Status bit
		oprand = data & 0x1f;
		LOGSTUFF("reset interrupt / status bit %02x\n",data);

		LOGSTUFF("Split 2   IRQ: %d Reset\n",(data>>0)&1);
		LOGSTUFF("Ready     IRQ: %d Reset\n",(data>>1)&1);
		LOGSTUFF("Split 1   IRQ: %d Reset\n",(data>>2)&1);
		LOGSTUFF("Line Zero IRQ: %d Reset\n",(data>>3)&1);
		LOGSTUFF("V-Blank   IRQ: %d Reset\n",(data>>4)&1);

		scn2674_irq_register &= ((data & 0x1f)^0x1f);
		scn2674_status_register &= ((data & 0x1f)^0x1f);

		if(data&0x10) //cpunum_set_input_line(1, 3, ASSERT_LINE); // maybe .. or maybe it just changes the register
		{
		scn2674_irq_state = 1; //need to check priorities
		update_mpu68_interrupts();
		}
		else
		scn2674_irq_state = 0;
	}

	if ((data&0xe0)==0x80)
	{
		// Disable Interrupt
		oprand = data & 0x1f;
		LOGSTUFF("disable interrupt %02x\n",data);
		LOGSTUFF("Split 2   IRQ: %d Disabled\n",(data>>0)&1);
		LOGSTUFF("Ready     IRQ: %d Disabled\n",(data>>1)&1);
		LOGSTUFF("Split 1   IRQ: %d Disabled\n",(data>>2)&1);
		LOGSTUFF("Line Zero IRQ: %d Disabled\n",(data>>3)&1);
		LOGSTUFF("V-Blank   IRQ: %d Disabled\n",(data>>4)&1);

//      scn2674_irq_mask &= ((data & 0x1f)^0x1f); // disables.. doesn't enable?

		if (data&0x01) scn2674_irq_mask&=0xfe;
		if (data&0x02) scn2674_irq_mask&=0xfd;
		if (data&0x04) scn2674_irq_mask&=0xfb;
		if (data&0x08) scn2674_irq_mask&=0xf7;
		if (data&0x10) scn2674_irq_mask&=0xef;



	}

	if ((data&0xe0)==0x60)
	{
		// Enable Interrupt
		LOGSTUFF("enable interrupt %02x\n",data);
		LOGSTUFF("Split 2   IRQ: %d Enabled\n",(data>>0)&1);
		LOGSTUFF("Ready     IRQ: %d Enabled\n",(data>>1)&1);
		LOGSTUFF("Split 1   IRQ: %d Enabled\n",(data>>2)&1);
		LOGSTUFF("Line Zero IRQ: %d Enabled\n",(data>>3)&1);
		LOGSTUFF("V-Blank   IRQ: %d Enabled\n",(data>>4)&1);

		scn2674_irq_mask |= (data & 0x1f);  // enables .. doesn't disable?

	}

	/**** Delayed Commands ****/

	if (data == 0xa4)
	{
		// read at pointer address
		LOGSTUFF("read at pointer address %02x\n",data);
	}

	if (data == 0xa2)
	{
		// write at pointer address
		LOGSTUFF("write at pointer address %02x\n",data);
	}

	if (data == 0xa9)
	{
		// increase cursor address
		LOGSTUFF("increase cursor address %02x\n",data);
	}

	if (data == 0xac)
	{
		// read at cursor address
		LOGSTUFF("read at cursor address %02x\n",data);
	}

	if (data == 0xaa)
	{
		// write at cursor address
		LOGSTUFF("write at cursor address %02x\n",data);
	}

	if (data == 0xad)
	{
		// read at cursor address + incrememnt
		LOGSTUFF("read at cursor address+incrememnt %02x\n",data);
	}

	if (data == 0xab)
	{
		// write at cursor address + incrememnt
		LOGSTUFF("write at cursor address+incrememnt %02x\n",data);
	}

	if (data == 0xbb)
	{
		// write from cursor address to pointer address
		LOGSTUFF("write from cursor address to pointer address %02x\n",data);
	}

	if (data == 0xbd)
	{
		// read from cursor address to pointer address
		LOGSTUFF("read from cursor address to pointer address %02x\n",data);
	}

}

READ16_HANDLER( mpu4_vid_scn2674_r )
{
	/*
    Offset:  Purpose
     0       Interrupt Register
     1       Status Register
     2       Screen Start 1 Lower Register
     3       Screen Start 1 Upper Register
     4       Cursor Address Lower Register
     5       Cursor Address Upper Register
     6       Screen Start 2 Lower Register
     7       Screen Start 2 Upper Register
    */

	switch (offset)
	{

		/*  Status / Irq Register

            --RV ZSRs

            -- = ALWAYS 0
            R  = RDFLG (Status Register Only)
            V  = Vblank
            Z  = Line Zero
            S  = Split 1
            R  = Ready
            s  = Split 2
        */
		case 0:
			printf("Read Irq Register %06x\n",activecpu_get_pc());
	//      return scn2674_irq_register|0x08;
	//      return 0x04;
			return scn2674_irq_register;

		case 1:
			printf("Read Status Register %06x\n",activecpu_get_pc());
			return mame_rand();//scn2674_irq_register;

		case 2: printf("Read Screen1_l Register %06x\n",activecpu_get_pc());return scn2674_screen1_l;
		case 3: printf("Read Screen1_h Register %06x\n",activecpu_get_pc());return scn2674_screen1_h;
		case 4: printf("Read Cursor_l Register %06x\n",activecpu_get_pc());return scn2674_cursor_l;
		case 5: printf("Read Cursor_h Register %06x\n",activecpu_get_pc());return scn2674_cursor_h;
		case 6:	printf("Read Screen2_l Register %06x\n",activecpu_get_pc());return scn2674_screen2_l;
		case 7: printf("Read Screen2_h Register %06x\n",activecpu_get_pc());return scn2674_screen2_h;
	}

	return 0xffff;
}

WRITE16_HANDLER( mpu4_vid_scn2674_w )
{
	/*
    Offset:  Purpose
     0       Initialization Registers
     1       Command Register
     2       Screen Start 1 Lower Register
     3       Screen Start 1 Upper Register
     4       Cursor Address Lower Register
     5       Cursor Address Upper Register
     6       Screen Start 2 Lower Register
     7       Screen Start 2 Upper Register
    */

	data &=0x00ff; // it's an 8-bit chip on a 16-bit board, feel the cheapness.

	switch (offset)
	{
		case 0:
			scn2674_write_init_regs(data);
			break;

		case 1:
			scn2674_write_command(data);
			break;

		case 2: scn2674_screen1_l = data; break;
		case 3: scn2674_screen1_h = data; break;
		case 4: scn2674_cursor_l  = data; break;
		case 5: scn2674_cursor_h  = data; break;
		case 6:	scn2674_screen2_l = data; break;
		case 7: scn2674_screen2_h = data; break;
	}
}

READ16_HANDLER( mpu4_vid_unmap_r )
{
	//logerror("Data %s Offset %s", data, offset);
	return mame_rand();
}

WRITE16_HANDLER( mpu4_vid_unmap_w )
{
	logerror("WData %d WOffset %d", data, offset);
}

VIDEO_START( mpu4_vid )
{
	/* if anything uses tile sizes other than 8x8 we can't really do it this way.. we'll have to draw tiles by hand.
      maybe we will anyway, but for now we don't need to */

	mpu4_vid_vidram = auto_malloc (0x20000);
	mpu4_vid_vidram_is_dirty = auto_malloc(0x1000);

	memset(mpu4_vid_vidram_is_dirty,1,0x1000);
	memset(mpu4_vid_vidram,0,0x20000);

 	/* find first empty slot to decode gfx */
	for (mpu4_gfx_index = 0; mpu4_gfx_index < MAX_GFX_ELEMENTS; mpu4_gfx_index++)
		if (Machine->gfx[mpu4_gfx_index] == 0)
			break;

	if (mpu4_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	/* create the char set (gfx will then be updated dynamically from RAM) */
	Machine->gfx[mpu4_gfx_index+0] = allocgfx(&mpu4_vid_char_8x8_layout);
	Machine->gfx[mpu4_gfx_index+1] = allocgfx(&mpu4_vid_char_8x16_layout);
	Machine->gfx[mpu4_gfx_index+2] = allocgfx(&mpu4_vid_char_16x8_layout);
	Machine->gfx[mpu4_gfx_index+3] = allocgfx(&mpu4_vid_char_16x16_layout);

	/* set the color information */
	Machine->gfx[mpu4_gfx_index+0]->colortable = Machine->pens;
	Machine->gfx[mpu4_gfx_index+0]->total_colors = Machine->drv->total_colors / 16;
	Machine->gfx[mpu4_gfx_index+1]->colortable = Machine->pens;
	Machine->gfx[mpu4_gfx_index+1]->total_colors = Machine->drv->total_colors / 16;
	Machine->gfx[mpu4_gfx_index+2]->colortable = Machine->pens;
	Machine->gfx[mpu4_gfx_index+2]->total_colors = Machine->drv->total_colors / 16;
	Machine->gfx[mpu4_gfx_index+3]->colortable = Machine->pens;
	Machine->gfx[mpu4_gfx_index+3]->total_colors = Machine->drv->total_colors / 16;

	scn2675_IR_pointer = 0;

	return 0;
}

/* palette support is very preliminary */
UINT8 ef9369_palette[32];
UINT8 ef9369_counter;

WRITE16_HANDLER( ef9369_data_w )
{
	int color;
	int r,g,b;
	UINT16 coldat;

	data &=0x00ff;

	ef9369_palette[ef9369_counter] = data;

	color = ef9369_counter/2;
	coldat = (ef9369_palette[color*2+1]<<8)|ef9369_palette[color*2];

	r = (coldat & 0x000f)>>0;
	b = (coldat & 0x00f0)>>4;
	g = (coldat & 0x0f00)>>8;

	palette_set_color(color,r<<4,g<<4,b<<4);

	ef9369_counter++;
	if (ef9369_counter>31) ef9369_counter = 31;
}

WRITE16_HANDLER( ef9369_address_w )
{
	data &=0x00ff;

	ef9369_counter = data*2; // = 0?

	if (ef9369_counter>30) ef9369_counter = 30;

}

/* 6840 emulation, see cchasm.c? */
#if 0
READ16_HANDLER( mpu4_vid_6840_r )
{
	return 0xffff;
}

WRITE16_HANDLER( mpu4_vid_6840_w )
{
	logerror("WData %d WOffset %d", data, offset);
}


/* 6850 emulation */

UINT8 mpu4_vid_6850_status_register;
UINT8 mpu4_vid_6850_received_data;
UINT8 mpu4_vid_6850_send_data;

/*
Status Register (R/O)

hex  bit
0x01 0 - RDRF (Receive Data Register Full)
0x02 1 - TDRE (Transmit Data Register Empty)
0x04 2 - DCD  (Data Carrier Detect)
0x08 3 - CTS  (Clear To Send)
0x10 4 - FE   (Framing Error)
0x20 5 - OVRN (Receiver Overrun)
0x40 6 - PE   (Parity Error)
0x80 7 - IRQ  (Interrupt Request)

Control Register (W/O)

hex  bit
0x01 0 - CR0  (Counter Divide Select 1)
0x02 1 - CR1  (Counter Divide Select 2)
0x04 2 - CR2  (Word Select 1)
0x08 3 - CR3  (Word Select 2)
0x10 4 - CR4  (Word Select 3)
0x20 5 - CR5  (Transmit Control 1)
0x40 6 - CR6  (Transmit Control 2)
0x80 7 - CR7  (Receive Interrupt Enable)

*/

READ16_HANDLER( mpu4_vid_6850_r )
{
//  data &=0x00ff;
	switch (offset)
	{
		case 0: return mpu4_vid_6850_status_register;
		case 1: return mpu4_vid_6850_received_data;
	}

	return 0xffff;
}

void mpu4_vid_6850_control_register_w (UINT8 data)
{

}

void mpu4_vid_6850_data_w (UINT8 data)
{
	mpu4_vid_6850_send_data = data;
}


WRITE16_HANDLER ( mpu4_vid_6850_w )
{
	data &=0x00ff;

	switch (offset)
	{
		case 0: mpu4_vid_6850_control_register_w(data);break;
		case 1: mpu4_vid_6850_data_w(data);break;
	}
	printf("6850 write to %02x = %02x\n",offset,data);
}
#endif


static ADDRESS_MAP_START( mpu4_vid_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x7fffff) AM_ROM

//  AM_RANGE(0x600000, 0x63ffff) AM_RAM? In expanded games (mating)

	AM_RANGE(0x800000, 0x80ffff) AM_RAM AM_BASE(&mpu4_vid_mainram) // mainram / char address ram?

	/* what is here, the sound chip? Assume so */
	AM_RANGE(0x900000, 0x900001) AM_WRITE(saa1099_control_port_0_lsb_w)
	AM_RANGE(0x900002, 0x900003) AM_WRITE(saa1099_write_port_0_lsb_w)

	/* the palette chip */
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(ef9369_data_w)
	AM_RANGE(0xa00002, 0xa00003) AM_WRITE(ef9369_address_w)
	AM_RANGE(0xa00004, 0xa0000f) //AM_READWRITE(mpu4_vid_unmap_r, mpu4_vid_unmap_w)

	AM_RANGE(0xb00000, 0xb0000f) AM_READWRITE(mpu4_vid_scn2674_r, mpu4_vid_scn2674_w)

	AM_RANGE(0xc00000, 0xc1ffff) AM_READWRITE(mpu4_vid_vidram_r, mpu4_vid_vidram_w)

	/* comms with the MPU4? - disabling this gives MPU4 communication breakdown */
    AM_RANGE(0xff8000, 0xff8001) AM_READ(  vidcard_uart_ctrl_r ) // 6850 compatible uart control reg read
    AM_RANGE(0xff8000, 0xff8001) AM_WRITE( vidcard_uart_ctrl_w )// 6850 compatible uart control reg write
    AM_RANGE(0xff8002, 0xff8003) AM_READ(  vidcard_uart_rx_r )  // 6850 compatible uart read  data
    AM_RANGE(0xff8002, 0xff8003) AM_WRITE( vidcard_uart_tx_w )  // 6850 compatible uart write data

	AM_RANGE(0xff9000, 0xff900f) AM_READ(  ptm6840_1_r16u)  // 6840PTM IC2
	AM_RANGE(0xff9000, 0xff900f) AM_WRITE( ptm6840_1_w16)  // 6840PTM IC2

	/* characterizer??? */
//  AM_RANGE(0xffd000, 0xffd00f) AM_RAM // crmaze et al???

ADDRESS_MAP_END

/* TODO: Fix up MPU4 map*/
static ADDRESS_MAP_START( mpu4_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)

	AM_RANGE(0x0800, 0x0800) AM_READ( mpu4_uart_ctrl_r)	// video uart control reg read
	AM_RANGE(0x0800, 0x0800) AM_WRITE(mpu4_uart_ctrl_w)	// video uart control reg write
	AM_RANGE(0x0801, 0x0801) AM_READ( mpu4_uart_rx_r)		// video uart receive  reg
	AM_RANGE(0x0801, 0x0801) AM_WRITE(mpu4_uart_tx_w)		// video uart transmit reg

	//AM_RANGE(0x0880, 0x0880) AM_READ(uart1stat_r)     // Could be a UART datalogger is here.
	//AM_RANGE(0x0880, 0x0880) AM_WRITE(uart1ctrl_w)    // Or a PIA?
	//AM_RANGE(0x0881, 0x0881) AM_READ(uart1data_r)
	//AM_RANGE(0x0881, 0x0881) AM_WRITE(uart1data_w)

	AM_RANGE(0x0900, 0x0907) AM_READ( ptm6840_0_r)  	// 6840PTM IC2
	AM_RANGE(0x0900, 0x0907) AM_WRITE(ptm6840_0_w)

	AM_RANGE(0x0A00, 0x0A03) AM_WRITE(pia_0_w)			// PIA6821 IC3
	AM_RANGE(0x0A00, 0x0A03) AM_READ( pia_0_r)

	AM_RANGE(0x0B00, 0x0B03) AM_WRITE(pia_1_w)			// PIA6821 IC4
	AM_RANGE(0x0B00, 0x0B03) AM_READ( pia_1_r)

	AM_RANGE(0x0C00, 0x0C03) AM_WRITE(pia_2_w)			// PIA6821 IC5
	AM_RANGE(0x0C00, 0x0C03) AM_READ( pia_2_r)

	AM_RANGE(0x0D00, 0x0D03) AM_WRITE(pia_3_w)			// PIA6821 IC6
	AM_RANGE(0x0D00, 0x0D03) AM_READ( pia_3_r)

	AM_RANGE(0x0E00, 0x0E03) AM_WRITE(pia_4_w)			// PIA6821 IC7
	AM_RANGE(0x0E00, 0x0E03) AM_READ( pia_4_r)

	AM_RANGE(0x0F00, 0x0F03) AM_WRITE(pia_5_w)			// PIA6821 IC8
	AM_RANGE(0x0F00, 0x0F03) AM_READ( pia_5_r)

	//AM_RANGE(0x1000, 0xFFFF) AM_READ(MRA8_ROM)        // 64k ROM

	AM_RANGE(0x1000, 0x3FFF) AM_RAM
	AM_RANGE(0x4000, 0x40FF) AM_RAM   // it actually runs code from here...
	AM_RANGE(0x4100, 0xBFFF) AM_RAM

	//AM_RANGE(0xBE00, 0xBFFF) AM_RAM //00 written on startup
	AM_RANGE(0xC000, 0xFFFF) AM_ROM

ADDRESS_MAP_END

INPUT_PORTS_START( mpu4_vid )

	PORT_START_TAG("IN1A")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("00")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("01")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("02")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("03")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("04")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("05")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("06")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("07")

	PORT_START_TAG("IN1B")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("08")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("09")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("10")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("11")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("12")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("13")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("14")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("15")

	PORT_START_TAG("IN2A")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("16")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("17")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("18")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("19")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("20")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("21")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("22")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("23")

	PORT_START_TAG("IN2B")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("24")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("25")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("26")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("27")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("28")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("29")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("30")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("31")

	PORT_START_TAG("AUX1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("7")

	PORT_START_TAG("AUX2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("0")
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("1")
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("2")
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("4")
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("5")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("6")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("7")

	//PORT_BIT( 0x01, IP_ACTIVE_LOW,IPT_OTHER) PORT_NAME("Refill Key") PORT_CODE(KEYCODE_R) PORT_TOGGLE
	//PORT_BIT( 0x02, IP_ACTIVE_LOW,IPT_OTHER) PORT_NAME("Bookkeeping") PORT_CODE(KEYCODE_F1) PORT_TOGGLE
	//2X8 dips
INPUT_PORTS_END

/*

IRQ  1  MC68A40 PTM   // On VDU board
IRQ  2  MC68A50 ACIA  // On MPU4 cart?
IRQ  3  SCN2674 AVDC  // On VDU board
IRQ 4-7 Program Card

*/

INTERRUPT_GEN(mpu4_vid_irq)
{
//  if (cpu_getiloops()&1)
//      cpunum_set_input_line(1, 1, HOLD_LINE);
//  else
//  LOGSTUFF("scn2674_irq_mask %02x",scn2674_irq_mask);

	if (cpu_getiloops()==0) // vbl
	{
	//  if (scn2674_display_enabled) // ?
		{
			if (scn2674_irq_mask&0x10)
			{
				LOGSTUFF("vblank irq\n");
				scn2674_irq_state = 1; //need to check priorities
				update_mpu68_interrupts();
 				//cpunum_set_input_line(1, 3, ASSERT_LINE);

				scn2674_irq_register |= 0x10;
			}
		}
		scn2674_status_register |= 0x10;

	}
}


MACHINE_START( mpu4_vid )
{
	pia_config(0, PIA_STANDARD_ORDERING, &pia_ic3_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia_ic4_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &pia_ic5_intf);
	pia_config(3, PIA_STANDARD_ORDERING, &pia_ic6_intf);
	pia_config(4, PIA_STANDARD_ORDERING, &pia_ic7_intf);
	pia_config(5, PIA_STANDARD_ORDERING, &pia_ic8_intf);

	//pia_config(6, PIA_ALTERNATE_ORDERING, &pia_gameboard_intf);

	pia_reset();

//  pia_set_input_ca1(2, 1);    // set IC5 CA1
//  pia_set_input_ca2(2, 1);    // set IC5 CA2

//  pia_set_input_b(6, 0x80);

//  pia_set_input_cb1(0, 1);    // IC3
//  pia_set_input_ca1(0, 1);    // set IC3 CA1
//  pia_set_input_ca2(0, 1);    // set IC5 CA2
//  pia_set_input_ca1(0, 1);    // set IC3 CA1

// setup ptm ////////////////////////////////////////////////////////////

	ptm6840_config(0, &ptm_ic2_intf );
	ptm6840_config(1, &ptm_vid_intf );

// setup 224 lamps //////////////////////////////////////////////////////

	Lamps_init(224);

// setup 8 mechanical meters ////////////////////////////////////////////

	Mechmtr_init(8);

// setup 4 default 96 half step reels ///////////////////////////////////

	Stepper_init(0, STEPPER_48STEP_REEL);
	Stepper_init(1, STEPPER_48STEP_REEL);
	Stepper_init(2, STEPPER_48STEP_REEL);
	Stepper_init(3, STEPPER_48STEP_REEL);

// setup the standard oki MSC1937 display ///////////////////////////////

//  vfd_init(0, VFDTYPE_MSC1937);   // ?

	return 0;
}

// generate a 50 Hz signal (based on an RC time) //////////////////////////

static INTERRUPT_GEN( gen_50hz )
{
  signal_50hz = signal_50hz?0:1;

  // update IC4 input port b

  if ( signal_50hz ) ic4_input_b |=  0x04;
  else               ic4_input_b &= ~0x04;

  pia_set_input_ca1(1,signal_50hz);		  // signal is connected to IC4 CA2
  pia_set_input_b(  1, ic4_input_b);	  // signal is connected to IC4 port
}

static MACHINE_DRIVER_START( mpu4_vid )

	MDRV_CPU_ADD_TAG("main", M6809, 1000000 )
	MDRV_CPU_PROGRAM_MAP(mpu4_map,0)
	MDRV_CPU_PERIODIC_INT(gen_50hz, TIME_IN_HZ(50) )		  // generate 50 hz signal

	MDRV_CPU_ADD_TAG("video", M68000, 10000000 )
	MDRV_CPU_PROGRAM_MAP(mpu4_vid_map,0)
	MDRV_CPU_VBLANK_INT(mpu4_vid_irq,1)
//  MDRV_CPU_PERIODIC_INT(irq1_line_hold,TIME_IN_HZ(100)) // 6840 IRQ.. WRONG! The 6840 should be doing this itself.

	MDRV_NVRAM_HANDLER(generic_0fill)					// confirm

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 63*8-1, 0*8, 37*8-1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_MACHINE_START(mpu4_vid)
	MDRV_MACHINE_RESET(mpu4_vid)
	MDRV_VIDEO_START (mpu4_vid)
	MDRV_VIDEO_UPDATE(mpu4_vid)

	MDRV_PALETTE_LENGTH(16)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")// Present on all
	MDRV_SOUND_ADD(SAA1099, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.00)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.00)

MACHINE_DRIVER_END

/*
   It appears there were two different games with the same name.

   One is non-video, and uses only the 6809, and the other is the one we're interested in (the older one)

   Some of the dumps available seem to confuse the two. */

#define VID_BIOS \
	ROM_LOAD("vid.p1",  0x0000, 0x10000,  CRC(e996bc18) SHA1(49798165640627eb31024319353da04380787b10))

ROM_START( bctvidbs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS
ROM_END

ROM_START( crmaze ) // this set runs in MFME, so should be OK */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "cm3.p1",  0x000000, 0x80000,  CRC(2d2edee5) SHA1(0281ec97aaaaf4c7969340bd5995ac1541dbad54) )
	ROM_LOAD16_BYTE( "cm3.p2",  0x000001, 0x80000,  CRC(c223d7b9) SHA1(da9d730716a30d0e93f2a02c1efa7f19457ae010) )
	ROM_LOAD16_BYTE( "cm3.p3",  0x100000, 0x80000,  CRC(2959c77b) SHA1(8de533bfad48ad19a635dddcafa2a0825133b4de) )
	ROM_LOAD16_BYTE( "cm3.p4",  0x100001, 0x80000,  CRC(b7873e9a) SHA1(a71fac883e02d5f49aee0a20f92dbdb00640ce8d) )
	ROM_LOAD16_BYTE( "cm3.p5",  0x200000, 0x80000,  CRC(c8375070) SHA1(da2ba6591d8765f896c40d6526da8e945d02a182) )
	ROM_LOAD16_BYTE( "cm3.p6",  0x200001, 0x80000,  CRC(1ea36938) SHA1(43f62935b21232d23f662e1e124663267edb1283) )
	ROM_LOAD16_BYTE( "cm3.p7",  0x300000, 0x80000,  CRC(9de3802e) SHA1(ec792f115a0708d68046ba0beb314b7e1f1eb422) )
	ROM_LOAD16_BYTE( "cm3.p8",  0x300001, 0x80000,  CRC(1e6e60b0) SHA1(5e71714747073dd89852a84585642388ee440325) )
	ROM_LOAD16_BYTE( "cm3.p9",  0x400000, 0x80000,  CRC(bfba55a7) SHA1(22eb9b1f9fe83d3b424fd521b68e2976a1940df9) )
	ROM_LOAD16_BYTE( "cm3.pa",  0x400001, 0x80000,  CRC(07edda81) SHA1(e94525be03f30e407051992925bb0d693f3d809b) )

	/* Does Crystal Maze really have a OKI sound roms? these were only in 2 sets */
	/* This are Ed Tudor Pole samples, the game graphics are Richard O'Brien - Fruit machine perhaps? */
//  ROM_REGION( 0x200000, REGION_SOUND1, 0 )
//  ROM_LOAD( "crmsnd.p1",  0x000000, 0x080000,  CRC(e05cdf96) SHA1(c85c7b31b775e3cc2d7f943eb02ff5ebae6c6080) )  // two sets differed by one byte 72C0: 73 / A7
//  ROM_LOAD( "crmsnd.p2",  0x080000, 0x080000,  CRC(11da0781) SHA1(cd63834bf5d5034c2473372bfcc4930c300333f7) )

	/* there were also many 128kb (64k, duplicate halves) roms in the big set... nothing else has them, maybe they're not important? */
	/* they only seem to differ in the FFxx region.. what are they? NVRAM dumps? load one just in the remote chance we need it.. */
	/* Appears to be the result of someone getting confused when dumping the AWP version*/
//  ROM_LOAD( "crmc.p1",  0x080000, 0x020000,  CRC(58631e6d) SHA1(cffecd4c4ca46aa0ccfbaf7592d58da0428cf143) )
ROM_END

ROM_START( crmazea )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "am1z.p1",  0x000000, 0x80000,  CRC(d2fdda6d) SHA1(96c27dedc3cf1dd478e6169200844d418a276f14) )
	ROM_LOAD16_BYTE( "am1z.p2",  0x000001, 0x80000,  CRC(1637170f) SHA1(fd17a0e7794f01bf4ad7a16b185f87cb060c70ab) )
	ROM_LOAD16_BYTE( "am1g.p1",  0x100000, 0x80000,  CRC(e8cf8203) SHA1(e9f42e5c18b97807f51284ad2416346578ed73c4) )
	ROM_LOAD16_BYTE( "am1g.p2",  0x100001, 0x80000,  CRC(7b036151) SHA1(7b0040c296059b1e1798ddedf0ecb4582d67ee70) )
	ROM_LOAD16_BYTE( "am1g.p3",  0x200000, 0x80000,  CRC(48f17b20) SHA1(711c46fcfd86ded8ff7da883188d70560d20e42f) )
	ROM_LOAD16_BYTE( "am1g.p4",  0x200001, 0x80000,  CRC(2b3d9a97) SHA1(7468fffd90d840d245a70475b42308f1e48c5017) )
	ROM_LOAD16_BYTE( "am1g.p5",  0x300000, 0x80000,  CRC(68286bb1) SHA1(c307e3ad1e0fd92314216c8e554aafa949559452) )
	ROM_LOAD16_BYTE( "am1g.p6",  0x300001, 0x80000,  CRC(a6b498ad) SHA1(117e1a4ec7e2d3c7d530c5a56cbc1d24b0ddc747) )
	ROM_LOAD16_BYTE( "am1g.p7",  0x400000, 0x80000,  CRC(15882699) SHA1(b29a331e51a37554323b91330a7c2b62b33a943a) )
	ROM_LOAD16_BYTE( "am1g.p8",  0x400001, 0x80000,  CRC(6f0f855b) SHA1(ab411d1af0f88049a6c435bafd4b1fa63f5519b1) )
ROM_END

ROM_START( crmazeb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "am2z.p1",  0x000000, 0x80000,  CRC(f27e02f0) SHA1(8637904c201ece4f08ad63b4fd6d06a860fa762f) )
	ROM_LOAD16_BYTE( "am2z.p2",  0x000001, 0x80000,  CRC(4d24f482) SHA1(9e3687db9d0233e56999017f3ed59ec543bce303) )
	ROM_LOAD16_BYTE( "am2g.p1",  0x100000, 0x80000,  CRC(115402db) SHA1(250f2eded1b88a1abf82febb009eadbb90936f8a) )
	ROM_LOAD16_BYTE( "am2g.p2",  0x100001, 0x80000,  CRC(5d804fbb) SHA1(8dc02eb9329f9c29d4bcc9a0315ae96085625d3e) )
	ROM_LOAD16_BYTE( "am2g.p3",  0x200000, 0x80000,  CRC(5ead0c06) SHA1(35d9aefc60e2c391e32f8119a6dc44434d91c09e) )
	ROM_LOAD16_BYTE( "am2g.p4",  0x200001, 0x80000,  CRC(de4fb542) SHA1(4bf8f8f6850fd819d91827d3c474bd488e61e5ac) )
	ROM_LOAD16_BYTE( "am2g.p5",  0x300000, 0x80000,  CRC(80b01ce2) SHA1(4a3a4bcff4bd9affd1a5eeca5781b6af05bbcc16) )
	ROM_LOAD16_BYTE( "am2g.p6",  0x300001, 0x80000,  CRC(3e134ecc) SHA1(1f8cdce62e693eb07c4620b64cc467339c0563de) )
	ROM_LOAD16_BYTE( "am2g.p7",  0x400000, 0x80000,  CRC(6eb36f1d) SHA1(08b9ec184d64bdbdfa61d3e991a3647e74a7756f) )
	ROM_LOAD16_BYTE( "am2g.p8",  0x400001, 0x80000,  CRC(dda353ef) SHA1(56a5b43f0b0bd9dbf348946a5758ebe63eadb8cf) )
ROM_END

ROM_START( turnover )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "tos.p1",  0x000000, 0x010000,  CRC(4044dbeb) SHA1(3aa553055c56f14564b1e33e1c1975337e639f70) )
	ROM_LOAD16_BYTE( "to.p2",   0x000001, 0x010000,  CRC(4bc4659a) SHA1(0e282134c4fe4e8c1cc7b16957903179e23c7abc) )
	ROM_LOAD16_BYTE( "to.p3",   0x020000, 0x010000,  CRC(273c7c14) SHA1(71feb555a05a0ff1ec674505cab72d93c9fbdf65) )
	ROM_LOAD16_BYTE( "to.p4",   0x020001, 0x010000,  CRC(83d29546) SHA1(cef90455b9d8a92424fe1aa10f20fd075d0e3091) )
	ROM_LOAD16_BYTE( "to.p5",   0x040000, 0x010000,  CRC(dceac511) SHA1(7a6d65464e23d832943f771c4cf580aabc6f0e44) )
	ROM_LOAD16_BYTE( "to.p6",   0x040001, 0x010000,  CRC(54c6afb7) SHA1(b724b87b6f4e47d220310b38c97be2fa73dcd617) )
	ROM_LOAD16_BYTE( "to.p7",   0x060000, 0x010000,  CRC(acf19542) SHA1(ad46ffb3c2c078a8e3712eff27aa61f0d1a7c059) )
	ROM_LOAD16_BYTE( "to.p8",   0x060001, 0x010000,  CRC(a5ca385d) SHA1(8df26a33ea7f5b577761c6f9d2fa4eaed74661f8) )
	ROM_LOAD16_BYTE( "to.p9",   0x080000, 0x010000,  CRC(6e85fde3) SHA1(14868d58829e13987e66f52e1899c4385987a87b) )
	ROM_LOAD16_BYTE( "to.p10",  0x080001, 0x010000,  CRC(fadd11a2) SHA1(2b2fbb0769ef6035688d495464f3ea3bc8c7c660) )
	ROM_LOAD16_BYTE( "to.p11",  0x0a0000, 0x010000,  CRC(2d72a61a) SHA1(ce455ab6fea452f96a3ad365178e0e5a0b437867) )
	ROM_LOAD16_BYTE( "to.p12",  0x0a0001, 0x010000,  CRC(a14eedb6) SHA1(219b887a334ff28a88ed2e50f0caff4b510cd549) )
	ROM_LOAD16_BYTE( "to.p13",  0x0c0000, 0x010000,  CRC(3f66ef6b) SHA1(60be6d3f8da1f3084db15ac1bb2470e55c0271de) )
	ROM_LOAD16_BYTE( "to.p14",  0x0c0001, 0x010000,  CRC(127ba65d) SHA1(e34dcd19efd31dc712daac940277bb17694ea61a) )
	ROM_LOAD16_BYTE( "to.p15",  0x0e0000, 0x010000,  CRC(ad787e31) SHA1(314ba312adfc71e4b3b2d52355ec692c192b74eb) )
	ROM_LOAD16_BYTE( "to.p16",  0x0e0001, 0x010000,  CRC(e635c942) SHA1(08f8b5fdb738647bc0b49938da05533be42a2d60) )
ROM_END

ROM_START( skiltrek )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "st.p1",  0x000000, 0x010000,  CRC(d9de47a5) SHA1(625bf40780203293fc34cd8cea8278b4b4a52a75) )
	ROM_LOAD16_BYTE( "st.p2",  0x000001, 0x010000,  CRC(b62575c2) SHA1(06d75e8a364750663d329650720021279e195236) )
	ROM_LOAD16_BYTE( "st.p3",  0x020000, 0x010000,  CRC(9506da76) SHA1(6ef28ab8ec1af455be8ecfab20243f0823dca7c1) )
	ROM_LOAD16_BYTE( "st.p4",  0x020001, 0x010000,  CRC(6ab447bc) SHA1(d01c209dbf4d19a6a7f878fa54ff1cb51e7dcba5) )
	ROM_LOAD16_BYTE( "st.q1",  0x040000, 0x010000,  CRC(4faca475) SHA1(69b498c543600b8e37ab0ed1863ba57845648f3c) )
	ROM_LOAD16_BYTE( "st.q2",  0x040001, 0x010000,  CRC(9f2c5938) SHA1(85527c4c0b7a1e66576d56607d89750fab082580) )
	ROM_LOAD16_BYTE( "st.q3",  0x060000, 0x010000,  CRC(6b6cb194) SHA1(aeac5dcc0827c17e758e3e821ae8a78a3a16ddce) )
	ROM_LOAD16_BYTE( "st.q4",  0x060001, 0x010000,  CRC(ec57bc17) SHA1(d9f522739dbb190fb941ca654299bbedbb8fb703) )
	ROM_LOAD16_BYTE( "st.q5",  0x080000, 0x010000,  CRC(7740a88b) SHA1(d9a683d3e0d6c1b4b59520f90f825124b7a61168) )
	ROM_LOAD16_BYTE( "st.q6",  0x080001, 0x010000,  CRC(95e97796) SHA1(f1a8de0ad02aca31f79a4fe8ba5044546163e3c4) )
	ROM_LOAD16_BYTE( "st.q7",  0x0a0000, 0x010000,  CRC(f3b8fe7f) SHA1(52d5be3f8cab419103f4727d0fb9d30f34c8f651) )
	ROM_LOAD16_BYTE( "st.q8",  0x0a0001, 0x010000,  CRC(b85e75a2) SHA1(b7b03b090c0ec6d92e9a25abb7fec0507356bdfc) )
	ROM_LOAD16_BYTE( "st.q9",  0x0c0000, 0x010000,  CRC(835f6001) SHA1(2cd9084c102d74bcb578c8ea22bbc9ea58f0ceab) )
	ROM_LOAD16_BYTE( "st.qa",  0x0c0001, 0x010000,  CRC(3fc62a0e) SHA1(0628de4b962d3fcca3757cd4e89b3005c9bfd218) )
ROM_END

ROM_START( mating )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "matd.p1",  0x000000, 0x080000,  CRC(e660909f) SHA1(0acd990264fd7faf1f91a796d2438e8c2c7b83d1) )
	ROM_LOAD16_BYTE( "matd.p2",  0x000001, 0x080000,  CRC(a4c7e9b4) SHA1(30148c0257181bb88159e02d2b7cd79995ee84a7) )
	ROM_LOAD16_BYTE( "matg.p3",  0x100000, 0x080000,  CRC(571f4e8e) SHA1(51babacb5d9fb1cc9e1e56a3b2a355597d04f178) )
	ROM_LOAD16_BYTE( "matg.p4",  0x100001, 0x080000,  CRC(52d8657b) SHA1(e44e1db13c4abd4fedcd72df9dce1df594f74e44) )
	ROM_LOAD16_BYTE( "matg.p5",  0x200000, 0x080000,  CRC(9f0c9552) SHA1(8b1197f20853e18841a8f64fd5ff58cdd0bd1dbd) )
	ROM_LOAD16_BYTE( "matg.p6",  0x200001, 0x080000,  CRC(59f2b6a8) SHA1(4921cf1fc4c3bc50d2598b63726f61f68b41658c) )
	ROM_LOAD16_BYTE( "matg.p7",  0x300000, 0x080000,  CRC(64c0031a) SHA1(a519addd5d8f4696967ec84c163d28cb81ff9f32) )
	ROM_LOAD16_BYTE( "matg.p8",  0x300001, 0x080000,  CRC(22370dae) SHA1(72b1686b458750b5ee9dfe5599c308329d2c79d5) )
	ROM_LOAD16_BYTE( "matq.p9",  0x400000, 0x040000,  CRC(2d42e982) SHA1(80e476d5d65662059daa93a2fd383aecb74903c1) )
	ROM_LOAD16_BYTE( "matq.p10", 0x400001, 0x040000,  CRC(90364c3c) SHA1(6a4d2a3dd2cf9040887503888e6f38341578ad64) )

	/* Mating Game has an extra OKI sound chip */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "matsnd.p1",  0x000000, 0x080000,  CRC(f16df9e3) SHA1(fd9b82d73e18e635a9ea4aabd8c0b4aa2c8c6fdb) )
	ROM_LOAD( "matsnd.p2",  0x080000, 0x080000,  CRC(0c041621) SHA1(9156bf17ef6652968d9fbdc0b2bde64d3a67459c) )
	ROM_LOAD( "matsnd.p3",  0x100000, 0x080000,  CRC(c7435af9) SHA1(bd6080afaaaecca0d65e6d4125b46849aa4d1f33) )
	ROM_LOAD( "matsnd.p4",  0x180000, 0x080000,  CRC(d7e65c5b) SHA1(5575fb9f948158f2e94c986bf4bca9c9ee66a489) )
ROM_END

ROM_START( matinga )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for BIOS */
	VID_BIOS

	ROM_REGION( 0x800000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "mats.p1",  0x000000, 0x080000,  CRC(ebc7ea7e) SHA1(11015489a803ba5c8dbdafd632424bbd6080aece) ) // 3 bytes changed from above set...
	ROM_LOAD16_BYTE( "mats.p2",  0x000001, 0x080000,  CRC(a4c7e9b4) SHA1(30148c0257181bb88159e02d2b7cd79995ee84a7) ) // Just enough to disable the data logger
	ROM_LOAD16_BYTE( "matg.p3",  0x100000, 0x080000,  CRC(571f4e8e) SHA1(51babacb5d9fb1cc9e1e56a3b2a355597d04f178) )
	ROM_LOAD16_BYTE( "matg.p4",  0x100001, 0x080000,  CRC(52d8657b) SHA1(e44e1db13c4abd4fedcd72df9dce1df594f74e44) )
	ROM_LOAD16_BYTE( "matg.p5",  0x200000, 0x080000,  CRC(9f0c9552) SHA1(8b1197f20853e18841a8f64fd5ff58cdd0bd1dbd) )
	ROM_LOAD16_BYTE( "matg.p6",  0x200001, 0x080000,  CRC(59f2b6a8) SHA1(4921cf1fc4c3bc50d2598b63726f61f68b41658c) )
	ROM_LOAD16_BYTE( "matg.p7",  0x300000, 0x080000,  CRC(64c0031a) SHA1(a519addd5d8f4696967ec84c163d28cb81ff9f32) )
	ROM_LOAD16_BYTE( "matg.p8",  0x300001, 0x080000,  CRC(22370dae) SHA1(72b1686b458750b5ee9dfe5599c308329d2c79d5) )
	ROM_LOAD16_BYTE( "matq.p9",  0x400000, 0x040000,  CRC(2d42e982) SHA1(80e476d5d65662059daa93a2fd383aecb74903c1) )
	ROM_LOAD16_BYTE( "matq.p10", 0x400001, 0x040000,  CRC(90364c3c) SHA1(6a4d2a3dd2cf9040887503888e6f38341578ad64) )

	/* Mating Game has an extra OKI sound chip */
	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "matsnd.p1",  0x000000, 0x080000,  CRC(f16df9e3) SHA1(fd9b82d73e18e635a9ea4aabd8c0b4aa2c8c6fdb) )
	ROM_LOAD( "matsnd.p2",  0x080000, 0x080000,  CRC(0c041621) SHA1(9156bf17ef6652968d9fbdc0b2bde64d3a67459c) )
	ROM_LOAD( "matsnd.p3",  0x100000, 0x080000,  CRC(c7435af9) SHA1(bd6080afaaaecca0d65e6d4125b46849aa4d1f33) )
	ROM_LOAD( "matsnd.p4",  0x180000, 0x080000,  CRC(d7e65c5b) SHA1(5575fb9f948158f2e94c986bf4bca9c9ee66a489) )
ROM_END

/*    YEAR   NAME    PARENT   MACHINE   INPUT     INIT   MONITOR COMPANY      FULLNAME                                                          FLAGS (0 if none)  */

GAME( 199?, bctvidbs,0,       mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "MPU4 Video Firmware",												NOT_A_DRIVER )

GAME( 1994, crmaze,  bctvidbs,mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "The Crystal Maze: Team Challenge (SWP)",							GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1994, crmazea, crmaze,  mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "The Crystal Maze (AMLD version SWP)",								GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1994, crmazeb, crmaze,  mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "The Crystal Maze - Now Featuring Ocean Zone (AMLD Version SWP)",	GAME_NOT_WORKING|GAME_NO_SOUND ) // unprotected?
GAME( 1990, turnover,bctvidbs,mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "Turnover",														GAME_NOT_WORKING|GAME_NO_SOUND ) // unprotected?
GAME( 1992, skiltrek,bctvidbs,mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "Skill Trek",														GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 199?, mating,  bctvidbs,mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "The Mating Game (Datapak)",										GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 199?, matinga, mating,  mpu4_vid, mpu4_vid, 0,     ROT0,   "Barcrest", "The Mating Game (Standard)",										GAME_NOT_WORKING|GAME_NO_SOUND )
