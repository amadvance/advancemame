/***************************************************************************

Toshiba T5182 die map, by Jonathan Gevaryahu AKA Lord Nightmare,
with assistance from Kevin Horton.
T5182 supplied by Tomasz 'Dox' Slanina

Die Diagram:
|------------------------|
\ ROM  RAM  Z80    A     |
/ B    C    D   E  F  G  |
|------------------------|

The ROM is a 23128 wired as a 2364 by tying a13 to /ce
The RAM is a 2016
The Z80 is a ...Z80. go figure.
Subdie A is a 7408 quad AND gate
Subdie B is a 74245 bidirectional bus transciever
Subdie C is a 74245 bidirectional bus transciever
Subdie D is a 74245 bidirectional bus transciever
Subdie E is a 74138 1 to 8 decoder/demultiplexer with active low outputs
Subdie F is a 74138 1 to 8 decoder/demultiplexer with active low outputs
Subdie G is a 7408 quad AND gate
Thanks to Kevin Horton for working out most of the logic gate types
from the diagram.

                       ______________________
                     _|*                     |_
               GND  |_|1                   50|_| Vcc
                     _|                      |_
                A8  |_|2                   49|_| A7
                     _|                      |_
                A9  |_|3                   48|_| A6
                     _|                      |_
               A10  |_|4                   47|_| A5
                     _|                      |_
               A11  |_|5                   46|_| A4
                     _|       TOSHIBA        |_
               A12  |_|6       T5182       45|_| A3
                     _|                      |_
               A13  |_|7                   44|_| A2
                     _|     JAPAN  8612      |_
               A14  |_|8                   43|_| A1
                     _|                      |_
               A15  |_|9                   42|_| A0
                     _|                      |_
                D4  |_|10                  41|_| D3
                     _|                      |_
                D5  |_|11                  40|_| D2
                     _|                      |_
                D6  |_|12                  39|_| D1
                     _|                      |_
                D7  |_|13                  38|_| D0
                     _|                      |_
         I/O /EN 2  |_|14                  37|_|  I/O /EN 1
                     _|                      |_
         I/O /EN 3  |_|15                  36|_|  I/O /EN 0
                     _|                      |_
         I/O /EN 4  |_|16                  35|_|  /EN 0x8000-0xFFFF
                     _|                      |_
         I/O /EN 5  |_|17                  34|_|  /EN 0x4000-0x7FFF
                     _|                      |_
  Z80 phi clock in  |_|18                  33|_|  N/C
                     _|                      |_
          Z80 /INT  |_|19                  32|_|  Z80 /RESET
                     _|                      |_
          Z80 /NMI  |_|20                  31|_|  Z80 /BUSRQ Test pin
                     _|                      |_
 Shorted to pin 22  |_|21                  30|_|  74245 'A'+'B' DIR Test pin
                     _|                      |_
 /EN 0x0000-0x1fff  |_|22                  29|_|  Z80 /BUSAK Test pin
                     _|                      |_
Z80 /MREQ Test pin  |_|23                  28|_|  Z80 /WR
                     _|                      |_
Z80 /IORQ Test pin  |_|24                  27|_|  Z80 /RD
                     _|                      |_
               GND  |_|25                  26|_|  Vcc
                      |______________________|

Based on sketch made by Tormod
Note: all pins marked as 'Test pin' are disabled internally and cannot be used without removing the chip cover and soldering together test pads.
Note: pins 21 and 22 are both shorted together, and go active (low) while the internal rom is being read. The internal rom can be disabled by pulling /IORQ or /MREQ low,
      but both of those test pins are disabled, and also one would have to use the DIR test pin at the same time to feed the z80 a new internal rom (this is PROBABLY how
      seibu prototyped the rom, they had an external rom connected to this enable, and the internal rom disabled somehow) This pin CAN however be used as an indicator as
      to when the internal rom is being read, allowing one to snoop the address and data busses without fear of getting ram data as opposed to rom.

Z80 Memory Map:
0x0000-0x1FFF - Internal ROM, also external space 0 (which is effectively disabled)
0x2000-0x3fff - Internal RAM, repeated/mirrored 4 times
0x4000-0x7fff - external space 1 (used for communication shared memory?)
0x8000-0xFFFF - external space 2 (used for sound rom)

I/O map:
FEDCBA9876543210
xxxxxxxxx000xxxx i/o /EN 0 goes low
xxxxxxxxx001xxxx i/o /EN 1 goes low
xxxxxxxxx010xxxx i/o /EN 2 goes low
xxxxxxxxx011xxxx i/o /EN 3 goes low
xxxxxxxxx100xxxx i/o /EN 4 goes low
xxxxxxxxx101xxxx i/o /EN 5 goes low
xxxxxxxxx110xxxx i/o /EN 6\__ these two are unbonded pins, so are useless.
xxxxxxxxx111xxxx i/o /EN 7/

IMPORTANT: the data lines for the external rom on darkmist are scrambled on the SEI8608B board as such:
CPU:     ROM:
D0       D0
D1       D6
D2       D5
D3       D4
D4       D3
D5       D2
D6       D1
D7       D7
Only the data lines are scrambled, the address lines are not.
These lines are NOT scrambled to the ym2151 or anything else, just the external rom.

***************************************************************************/

#include "driver.h"
#include "t5182.h"
#include "sound/2151intf.h"


UINT8 *t5182_sharedram;


enum
{
	VECTOR_INIT,
	YM2151_ASSERT,
	YM2151_CLEAR,
	YM2151_ACK,
	CPU_ASSERT,
	CPU_CLEAR
};

static int irqstate;

static void setirq_callback(int param)
{
	int cpunum;

	switch(param)
	{
		case YM2151_ASSERT:
			irqstate |= 1|4;
			break;

		case YM2151_CLEAR:
			irqstate &= ~1;
			break;

		case YM2151_ACK:
			irqstate &= ~4;
			break;

		case CPU_ASSERT:
			irqstate |= 2;	// also used by t5182_sharedram_semaphore_main_r
			break;

		case CPU_CLEAR:
			irqstate &= ~2;
			break;
	}

	cpunum = mame_find_cpu_index(CPUTAG_T5182);

	if (cpunum == -1)
		return;

	if (irqstate == 0)	/* no IRQs pending */
		cpunum_set_input_line(cpunum,0,CLEAR_LINE);
	else	/* IRQ pending */
		cpunum_set_input_line(cpunum,0,ASSERT_LINE);
}



WRITE8_HANDLER( t5182_sound_irq_w )
{
	timer_set(TIME_NOW,CPU_ASSERT,setirq_callback);
}

static WRITE8_HANDLER( t5182_ym2151_irq_ack_w )
{
	timer_set(TIME_NOW,YM2151_ACK,setirq_callback);
}

static WRITE8_HANDLER( t5182_cpu_irq_ack_w )
{
	timer_set(TIME_NOW,CPU_CLEAR,setirq_callback);
}

static void t5182_ym2151_irq_handler(int irq)
{
	if (irq)
		timer_set(TIME_NOW,YM2151_ASSERT,setirq_callback);
	else
		timer_set(TIME_NOW,YM2151_CLEAR,setirq_callback);
}



int semaphore_main, semaphore_snd;

READ8_HANDLER(t5182_sharedram_semaphore_snd_r)
{
	return semaphore_snd;
}

WRITE8_HANDLER(t5182_sharedram_semaphore_main_acquire_w)
{
	semaphore_main = 1;
}

WRITE8_HANDLER(t5182_sharedram_semaphore_main_release_w)
{
	semaphore_main = 0;
}

static WRITE8_HANDLER(t5182_sharedram_semaphore_snd_acquire_w)
{
	semaphore_snd = 1;
}

static WRITE8_HANDLER(t5182_sharedram_semaphore_snd_release_w)
{
	semaphore_snd = 0;
}

static READ8_HANDLER(t5182_sharedram_semaphore_main_r)
{
	return semaphore_main | (irqstate & 2);
}


struct YM2151interface t5182_ym2151_interface =
{
	t5182_ym2151_irq_handler
};




	// 4000-407F    RAM shared with main CPU
	// 4000 output queue length
	// 4001-4020 output queue
	// answers:
	//  80XX finished playing sound XX
	//  A0XX short contact on coin slot XX (coin error)
	//  A1XX inserted coin in slot XX
	// 4021 input queue length
	// 4022-4041 input queue
	// commands:
	//  80XX play sound XX
	//  81XX stop sound XX
	//  82XX stop all voices associated with timer A/B/both where XX = 01/02/03
	//  84XX play sound XX if it isn't already playing
	//  90XX reset
	//  A0XX
	// rest unused
ADDRESS_MAP_START( t5182_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM	// internal ROM
	AM_RANGE(0x2000, 0x27ff) AM_RAM	// internal RAM
	AM_RANGE(0x4000, 0x40ff) AM_RAM AM_BASE(&t5182_sharedram)
	AM_RANGE(0x8000, 0xffff) AM_ROM	// external ROM
ADDRESS_MAP_END


	// 00  W YM2151 address
	// 01 RW YM2151 data
	// 10  W semaphore for shared RAM: set as in use
	// 11  W semaphore for shared RAM: set as not in use
	// 12  W clear IRQ from YM2151
	// 13  W clear IRQ from main CPU
	// 20 R  flags bit 0 = main CPU is accessing shared RAM????  bit 1 = main CPU generated IRQ
	// 30 R  coin inputs (bits 0 and 1, active high)
	// 40  W external ROM banking? (the only 0 bit enables a ROM)
	// 50  W test mode status flags (bit 0 = ROM test fail, bit 1 = RAM test fail, bit 2 = YM2151 IRQ not received)
ADDRESS_MAP_START( t5182_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(t5182_sharedram_semaphore_snd_acquire_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(t5182_sharedram_semaphore_snd_release_w)
	AM_RANGE(0x12, 0x12) AM_WRITE(t5182_ym2151_irq_ack_w)
	AM_RANGE(0x13, 0x13) AM_WRITE(t5182_cpu_irq_ack_w)
	AM_RANGE(0x20, 0x20) AM_READ(t5182_sharedram_semaphore_main_r)
//    AM_RANGE(0x30, 0x30) AM_READ(port_tag_to_handler8(T5182COINPORT)) // incompatable code
ADDRESS_MAP_END
