// license:BSD-3-Clause
// copyright-holders:Ville Linde
/*  Konami GTI Club System

    Driver by Ville Linde


    Hardware overview:

    GN672 CPU board:
    ----------------
        IBM PowerPC 403GA at 32MHz (main CPU)
        Motorola MC68EC000 at 16MHz (sound CPU)
        Konami K056800 (MIRAC), sound system interface
        Konami K056230 (LANC), LAN interface
        Ricoh RF5c400 sound chip

    GN678 GFX board:
    ----------------
        Analog Devices ADSP-21062 SHARC DSP at 36MHz
        Konami K001604 (2D tilemaps + 2x ROZ)
        2x KS10071 (custom 3D pixel unit)
        2x KS10081 (custom 3D texel unit)



    Hardware configurations:

    Game                           | ID        | CPU PCB      | CG Board(s)    | notes
    -----------------------------------------------------------------------------------------------
    GTI Club                       | GX688     | GN672(A)     | GN678(B)       |
    Operation: Thunder Hurricane   | GX680     | GN672(A)     | GN678(B)       | GN680(E) I/O board
    Solar Assault                  | GX792     | GN672(A)     | GN678(B)       |

    Hang Pilot                     | GN685     | GN672(A)     | 2x ??          | 3dfx-based CG boards


Konami 'GTI Club' Hardware
Konami, 1996-1998

Known games on this hardware include....

Game                        (C)      Year
-----------------------------------------
GTI Club: Rally Cote D'Azur Konami   1996
Jet Wave / Wave Shark       Konami   1996 (video board only)
Operation Thunder Hurricane Konami   1997
Solar Assault               Konami   1997
Solar Assault : Revised     Konami   1998


PCB Layouts
-----------

Top Board

GN672 PWB(A)3020088
Konami 1996
|--------------------------------------------------------------|
|                                    DRAM1M   MASKROM.2S       |
|CN13            NJM5532    PAL(002616)                        |
|                    SM5877                   MASKROM.5S       |
|       AN7395S  NJM5532                                       |
|                                   RF5C400   MASKROM.7S       |
|                                                              |
|             056800            SRAM256K      MASKROM.9S       |
|   058232                      SRAM256K                       |
|       RESET_SW        EPROM.13K                   MASKROM.12U|
|5                                                             |
|6                                                  MASKROM.14U|
|W  TEST_SW              68EC000FN16                           |
|A                                         EPROM.19R  EPROM.19U|
|Y                                         EPROM.19R  EPROM.19U|
|              33.868MHz                                       |
|                     PAL(002248)                  |---------| |
|                     PAL(002249)                  |PPC403GA | |
|             93C56                                |         | |
|LED        PAL(002247)      DRAM4MX16             |         | |
|                                                  |---------| |
|           056230           DRAM4MX16                         |
|SRAM64K                                                       |
|                   64MHz            MACH111                   |
|CN4                                                           |
|        PAL(056787A)                                          |
|CN5                                                           |
|DSW(4)                        CN12                            |
|--------------------------------------------------------------|
Notes:
         DRAM1M - OKI M514256 1Mx4 DRAM (SOJ26/20)
        SRAM64K - 8kx8 SRAM (DIP28)
       SRAM256K - Fujitsu 84256 32kx8 SRAM (DIP28)
      DRAM4MX16 - Hitachi HM514260 4Mx16 DRAM (SOJ42)
        RF5C400 - Ricoh RF5C400 PCM 32Ch, 44.1 kHz Stereo, 3D Effect Spatializer, clock input 16.934MHz (33.868/2)
         056800 - Konami Custom (QFP80)
         056230 - Konami Custom (QFP80)
         058232 - Konami Custom Ceramic Package (SIL14, D/A filter?)
        MACH111 - AMD MACH111 PLCC44 CPLD (stamped '002246')
        68EC000 - Motorola MC68EC000, running at 16.0MHz (64/4)
          93C56 - EEPROM (DIP8)
       PPC403GA - IBM PowerPC 403GA CPU, clock input 32.0MHz (64/2) (QFP160)
       SM5877AM - Nippon Precision Circuits 3rd Order 2-Channel D/A Converter (SOIC24)
      NJM5532AN - Dual Low-Noise High-Speed Audio OP Amp (DIP8)
        AN7395S - Panasonic AN7395S Spatializer Audio Processor IC for 3D surround (SOIC20)
          CN4/5 - Network connectors
           CN12 - DIN96 connector joining to lower PCB
           CN13 - Audio OUT connector
            LED - Alpha-numeric 7-segment LED

ROM Usage
---------
                            |--------------------------------------- ROM Locations --------------------------------------|
Game                        13K     2S      5S      7S      9S      12U     14U     19R       19U       21R       21U
--------------------------------------------------------------------------------------------------------------------------
GTI Club: Rally Cote D'Azur 688A07  688A12  688A11  688A10  688A09  688A06  688A05  688AAA04  688AAA02  688AAA03  688AAA01
Jet Wave                    - see note -
Hang Pilot                  685A07  -       -       685A10  685A09  685A06  685A05  685JAB04  685JAB02  685JAB03  685JAB01
Operation Thunder Hurricane 680A07  680A12  680A11  680A10  680A09  680A06  680A05  680UAA04  680UAA02  680UAA03  680UAA01
Solar Assault               792A07  792A12  792A11  792A10  792A09  792A06  792A05  792UAA04  792UAA02  792UAA03  792UAA01
Solar Assault : Revised     - N/A -

Note : Jet Wave uses the lower board (GN678) from GTI Club, but it uses a different top board (ZR107 PWB(A)300769A)
Check zr107.cpp for details on the top board.

Operation Thunder Hurricane uses an additional top board gun/analog controls. Analog inputs are controlled by two CCD
cameras, one from each gun. This specific variation uses a K056230 for networking between the cpu board to receive
the analog values that way. Teraburst uses a different variation of this I/O board replacing the K056230 with a K056800 (see konami/hornet.cpp).

GN680 PWB(E)403381B
|------------------------------------------|
|CN11  CN12    CN8      CN9    CN10  DSW(4)|
|                 NRPS11     NRPS11        |
|                                          |
|                        LM1881   LM1881   |
|                                          |
|LED(x4)                                   |
|                                          |
|           68EC000FN16  8464              |
|    RESET_SW            8464              |
|32MHz                           680C22.20K|
|8464                 PAL(002962)          |
|CN4   056230         PAL(002961)          |
|   PAL(056787A)      PAL(002960)          |
|CN5                                       |
|------------------------------------------|
Notes:
      68EC000 @ 16MHz (32/2)
      CN11/12 - Power connectors
      CN8/9   - 6-pin analog control connectors (to CCD cameras)
      CN10    - 4-pin power connector for IR emitters
      CN4/5   - Pin jack/network connectors (to cpu board)
      NRPS11  - Idec NRPS11 PC Board circuit protector
      LM1881  - Video sync separator (DIP8)
      056230  - Konami Custom (QFP80)



Bottom Board

GN678 PWB(B)302009A
Konami 1996
|-------------------------------------------------------------------------------------------|
|CN4      MASKROM.2D      |--------|  SDR4M16  SDR4M16      |--------|  SDR4M16  SDR4M16    |
|                         |KS10081 |                        |KS10081 |                      |
|         MASKROM.4D      |        |                        |        |               SDR4M16|
|                         |--------|     |----------|       |--------|    |----------|      |
|CN2      MASKROM.6D                     | KS10071  | SDR4M16             | KS10071  |      |
|                                        |          |                     |          |      |
|         MASKROM.9D                     |          | SDR4M16             |          |      |
|                                        |----------|                     |----------|      |
|         MASKROM.11D                                                                SDR4M16|
|                                          SDR4M16                          SDR4M16         |
|         MASKROM.13D                                                                       |
|                                                      MC88916                              |
|         MASKROM.16D                                         PAL(002304)                   |
| MC44200                                                                 PAL(002303)       |
|         MASKROM.18D                                                                       |
|                                                                                           |
|                                                                         36MHz             |
|                                AM7203  AM7203  AM7203  AM7203         |-------------|     |
|                                                  256KSRAM  256KSRAM   |ANALOG       |     |
|                                                                       |DEVICES      |     |
|              PAL(002305)  64KSRAM   64KSRAM      256KSRAM  256KSRAM   |ADSP-21062   |     |
|                                                                       |SHARC        |     |
|         |--------|     MACH110                                        |KS-160X      |     |
|1MSRAM   |KONAMI  |                                                    |-------------|     |
|1MSRAM   |001604  |                                                                        |
|1MSRAM   |        |                                                        1MSRAM  1MSRAM  |
|1MSRAM   |--------|                                                                        |
|1MSRAM       256KSRAM                                                      1MSRAM  1MSRAM  |
|1MSRAM   256KSRAM 256KSRAM                    CN1                                          |
|-------------------------------------------------------------------------------------------|
Notes:
     SDR4M16 - Fujitsu 81141622-015 4M SDRAM (TSOP50)
      1MSRAM - Sharp LH521007 128kx8 SRAM (SOJ32)
    256KSRAM - Cypress CY7C199 32kx8 SRAM (SOJ28)
     64KSRAM - Cypress CY7C185 8kx8 SRAM (DIP28)
     KS10071 - Konami Custom video chip
     KS10081 - Konami Custom video chip
      001604 - Konami Custom (QFP208)
   MC44200FT - Motorola MC44200FT 3 Channel Video D/A Converter (QFP44)
     MACH110 - AMD MACH110 or MACH111 PLCC44 CPLD (Jet Wave stamped '002302')
                                                  (GTI Club stamped '003161')
                                                  (Thund.Hurr. stamped '003161')
      AM7203 - AMD AM7203 FIFO (PLCC32)
     MC88916 - Motorola MC88916 Low Skew CMOS PLL Clock Driver
         CN1 - 96 Pin joining connector to upper PCB
         CN2 - 8-Pin 24kHz RGB OUT
         CN4 - 6-Pin Power Connector

ROM Usage
---------
                            |---------------------- ROM Locations -----------------------|
Game                        2D      4D      6D      9D      11D     13D     16D     18D
------------------------------------------------------------------------------------------
GTI Club: Rally Cote D'Azur -       688A16  -       688A15  -       688A14  -       688A13
Jet Wave                    -       678A16  -       678A15  -       678A14  -       678A13
Operation Thunder Hurricane -       680A16  -       680A15  -       680A14  -       680A13
Solar Assault               -       792A16  -       792A15  -       792A14  -       792A13
Solar Assault : Revised     - N/A -

Hang Pilot (uses an unknown but similar video board)                12W             4W
                            -       -       -       -       -       678A14  -       678A13

*/

/*
    Differences from MAME:

    The K056800, K056230 and the sound timer are implemented in the driver.
    The 68000 interrupt levels are combined in the driver, as the 0.106
    68000 interface keeps a single level. The SHARC FLAG1 input is asserted
    at vblank instead of the screen update. The polling of the empty task
    queue in the GTI Club main loop is skipped until the next interrupt,
    and the same is done for the task list in the Solar Assault main loop.
    When the default EEPROM holds only the magic number, the test switch is
    held at the first boot to let the game initialize it.

    The boot delay loop of GTI Club polling the time base consumes at once
    the cycles up to its next target.

    The GTI Club screen bitmap holds only the visible area, as the whole
    frame is copied for the video thread at every update.

    The two Hang Pilot monitors are stacked in a single screen, the first CG
    board on top. The K001604 and the palette of the board selected by the
    system register are mapped by handlers instead of a memory view, and the
    polling of the empty task queue in the main loop is skipped as well.
*/

#include "driver.h"
#include "cpu/powerpc/ppc.h"
#include "cpu/sharc/sharc.h"
#include "machine/eeprom.h"
#include "machine/adc1038.h"
#include "machine/konppc.h"
#include "vidhrdw/k001005.h"
#include "vidhrdw/k001604.h"
#include "vidhrdw/voodoo.h"
#include "sound/rf5c400.h"

#define PPC_CPU		0
#define SOUND_CPU	1
#define DSP_CPU		2
#define DSP2_CPU	3

static UINT32 *work_ram;
static UINT32 *sharc_dataram;
static UINT32 *sharc_dataram1;
static UINT32 *hangplt_paletteram[2];
static mame_bitmap *hangplt_screen_bitmap;
static UINT32 idle_pc;
static UINT32 idle_addr;
static UINT32 idle_lr;
static UINT32 tb_wait_pc;
static UINT32 tb_wait_lr;
static int eeprom_factory_init;

/* K056230 (LANC) */
static UINT32 lanc_ram[0x800];
static UINT8 lanc_status;
static int lanc_irq;

/* K056800 (MIRAC) */
static UINT8 host_to_snd_regs[4];
static UINT8 snd_to_host_regs[2];
static int snd_int_pending, snd_int_enabled;

/* the 68000 core keeps a single interrupt level, so the pending levels are tracked here */
static int sound_irq_enabled;
static int sound_irq_pending;

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	b = ((data >> 0) & 0x1f);
	g = ((data >> 5) & 0x1f);
	r = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);

	palette_set_color(offset, r, g, b);
}

/******************************************************************/

static UINT8 sysreg_read(int reg)
{
	switch (reg)
	{
		case 0:
			return readinputportbytag("IN0");
		case 1:
			return readinputportbytag("IN1");
		case 2:
			return adc1038_sars_read() << 7;
		case 3:
		{
			UINT8 r = readinputportbytag("IN3");

			/*
			 * A default EEPROM holding only the magic number is rejected by the
			 * game, which initializes it when booted with the test switch on.
			 * The switch is held at the first boot until the initialization.
			 */
			if (eeprom_factory_init && cpu_getcurrentframe() < 300)
				r &= ~0x80;
			return r;
		}
		case 4:
			/* bit 1 = EEPROM data out, bit 2 = ADC readout */
			return (EEPROM_read_bit() << 1) | (adc1038_do_read() << 2);
	}
	return 0;
}

static void sysreg_write(int reg, UINT8 data)
{
	switch (reg)
	{
		case 3:
			EEPROM_write_bit(data & 0x01);
			EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			/* the EEPROM core treats CLEAR_LINE as chip selected */
			EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
			break;

		case 4:
			if (data & 0x80)	/* CG Board 1 IRQ Ack */
				cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ1, CLEAR_LINE);
			if (data & 0x40)	/* CG Board 0 IRQ Ack */
				cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ0, CLEAR_LINE);

			adc1038_di_write(data & 0x01);
			adc1038_clk_write((data >> 1) & 0x01);

			set_cgboard_id((data >> 4) & 0x3);
			break;
	}
}

static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	int lane;

	/* the reads have side effects on the ADC, so only the accessed bytes are read */
	for (lane = 0; lane < 4; ++lane)
		if (!(mem_mask & (0xff000000 >> (lane * 8))))
			r |= sysreg_read((offset * 4) + lane) << (24 - lane * 8);

	return r;
}

static WRITE32_HANDLER( sysreg_w )
{
	int lane;

	for (lane = 0; lane < 4; ++lane)
		if (!(mem_mask & (0xff000000 >> (lane * 8))))
			sysreg_write((offset * 4) + lane, (data >> (24 - lane * 8)) & 0xff);
}

/******************************************************************/

static void sound_update_irq(void)
{
	if (sound_irq_pending & 2)
		cpunum_set_input_line(SOUND_CPU, 2, ASSERT_LINE);
	else if (sound_irq_pending & 1)
		cpunum_set_input_line(SOUND_CPU, 1, ASSERT_LINE);
	else
		cpunum_set_input_line(SOUND_CPU, 1, CLEAR_LINE);
}

static INTERRUPT_GEN( sound_irq )
{
	if (sound_irq_enabled)
	{
		sound_irq_pending |= 1;
		sound_update_irq();
	}
}

static WRITE16_HANDLER( soundtimer_en_w )
{
	sound_irq_enabled = !(data & 1);
}

static WRITE16_HANDLER( soundtimer_ack_w )
{
	sound_irq_pending &= ~1;
	sound_update_irq();
}

/* K056800 host side */

static READ32_HANDLER( ppc_sound_r )
{
	UINT32 rv = 0;
	int lane;

	for (lane = 0; lane < 4; ++lane)
	{
		int reg = ((offset * 4) + lane) & 7;

		if (!(mem_mask & (0xff000000 >> (lane * 8))) && reg < 2)
			rv |= snd_to_host_regs[reg] << (24 - lane * 8);
	}

	return rv;
}

static WRITE32_HANDLER( ppc_sound_w )
{
	int lane;

	for (lane = 0; lane < 4; ++lane)
	{
		int reg = ((offset * 4) + lane) & 7;
		int val = (data >> (24 - lane * 8)) & 0xff;

		if (mem_mask & (0xff000000 >> (lane * 8)))
			continue;

		if (reg < 4)
		{
			host_to_snd_regs[reg] = val;
		}
		else if (reg == 7)
		{
			/* sound interrupt */
			if (snd_int_enabled)
			{
				snd_int_pending = 1;
				sound_irq_pending |= 2;
				sound_update_irq();
			}
		}
	}
}

/* K056800 sound side */

static READ16_HANDLER( sndcomm68k_r )
{
	if ((offset & 7) < 4)
		return host_to_snd_regs[offset & 7];

	return 0;
}

static WRITE16_HANDLER( sndcomm68k_w )
{
	if (!ACCESSING_LSB16)
		return;

	switch (offset & 7)
	{
		case 0:
		case 1:
			snd_to_host_regs[offset & 7] = data;
			break;

		case 4:
			/* sound CPU interrupt control */
			snd_int_enabled = data & 1;

			if (snd_int_enabled)
			{
				if (snd_int_pending)
					sound_irq_pending |= 2;
			}
			else
			{
				snd_int_pending = 0;
				sound_irq_pending &= ~2;
			}
			sound_update_irq();
			break;
	}
}

/* K056230 (LANC), the network is not emulated */

static void lanc_update_irq(int state)
{
	if (lanc_irq != state)
	{
		lanc_irq = state;
		cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ2, state);
	}
}

static READ32_HANDLER( lanc_r )
{
	UINT32 r = 0;

	if (offset == 0 && !(mem_mask & 0xff000000))
		r |= lanc_status << 24;

	return r;
}

static WRITE32_HANDLER( lanc_w )
{
	if (offset == 0 && !(mem_mask & 0x00ff0000))
	{
		UINT8 value = (data >> 16) & 0xff;

		if (value & 0x20)
			lanc_update_irq(ASSERT_LINE);
		if (!(value & 0x01))
			lanc_update_irq(CLEAR_LINE);
	}
}

static READ32_HANDLER( lanc_ram_r )
{
	return lanc_ram[offset & 0x7ff];
}

static WRITE32_HANDLER( lanc_ram_w )
{
	COMBINE_DATA(&lanc_ram[offset & 0x7ff]);
}

/******************************************************************/

static ADDRESS_MAP_START( gticlub_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_MIRROR(0x80000000) AM_RAM AM_BASE(&work_ram)		/* Work RAM */
	AM_RANGE(0x74000000, 0x740000ff) AM_MIRROR(0x80000000) AM_READWRITE(k001604_0_reg_r, k001604_0_reg_w)
	AM_RANGE(0x74010000, 0x7401ffff) AM_MIRROR(0x80000000) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74020000, 0x7403ffff) AM_MIRROR(0x80000000) AM_READWRITE(k001604_0_tile_r, k001604_0_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_MIRROR(0x80000000) AM_READWRITE(k001604_0_char_r, k001604_0_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x78040000, 0x7804000f) AM_MIRROR(0x80000000) AM_READWRITE(k001006_0_r, k001006_0_w)
	AM_RANGE(0x78080000, 0x7808000f) AM_MIRROR(0x80000000) AM_READWRITE(k001006_1_r, k001006_1_w)
	AM_RANGE(0x780c0000, 0x780c0003) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_MIRROR(0x80000000) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x7e008000, 0x7e009fff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_r, lanc_w)
	AM_RANGE(0x7e00a000, 0x7e00bfff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_ram_r, lanc_ram_w)
	AM_RANGE(0x7e00c000, 0x7e00c00f) AM_MIRROR(0x80000000) AM_READWRITE(ppc_sound_r, ppc_sound_w)
	AM_RANGE(0x7f000000, 0x7f3fffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER2, 0)	/* Data ROM */
	AM_RANGE(0x7f800000, 0x7f9fffff) AM_MIRROR(0x80000000) AM_ROM AM_SHARE(2)
	AM_RANGE(0x7fe00000, 0x7fffffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)	/* Program ROM */
ADDRESS_MAP_END

/* the K001604 and the palette of the CG board selected by the system register are visible */

static READ32_HANDLER( hangplt_k001604_reg_r )
{
	return get_cgboard_id() ? k001604_1_reg_r(offset, mem_mask) : k001604_0_reg_r(offset, mem_mask);
}

static WRITE32_HANDLER( hangplt_k001604_reg_w )
{
	if (get_cgboard_id())
		k001604_1_reg_w(offset, data, mem_mask);
	else
		k001604_0_reg_w(offset, data, mem_mask);
}

static READ32_HANDLER( hangplt_k001604_tile_r )
{
	return get_cgboard_id() ? k001604_1_tile_r(offset, mem_mask) : k001604_0_tile_r(offset, mem_mask);
}

static WRITE32_HANDLER( hangplt_k001604_tile_w )
{
	if (get_cgboard_id())
		k001604_1_tile_w(offset, data, mem_mask);
	else
		k001604_0_tile_w(offset, data, mem_mask);
}

static READ32_HANDLER( hangplt_k001604_char_r )
{
	return get_cgboard_id() ? k001604_1_char_r(offset, mem_mask) : k001604_0_char_r(offset, mem_mask);
}

static WRITE32_HANDLER( hangplt_k001604_char_w )
{
	if (get_cgboard_id())
		k001604_1_char_w(offset, data, mem_mask);
	else
		k001604_0_char_w(offset, data, mem_mask);
}

static READ32_HANDLER( hangplt_paletteram32_r )
{
	return hangplt_paletteram[get_cgboard_id()][offset];
}

static WRITE32_HANDLER( hangplt_paletteram32_w )
{
	int board = get_cgboard_id();
	int r,g,b;

	COMBINE_DATA(&hangplt_paletteram[board][offset]);
	data = hangplt_paletteram[board][offset];

	b = ((data >> 0) & 0x1f);
	g = ((data >> 5) & 0x1f);
	r = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);

	/* the second board uses the second half of the palette */
	palette_set_color((board * 0x4000) + offset, r, g, b);
}

static ADDRESS_MAP_START( hangplt_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_MIRROR(0x80000000) AM_RAM AM_BASE(&work_ram)		/* Work RAM */
	AM_RANGE(0x74000000, 0x740000ff) AM_MIRROR(0x80000000) AM_READWRITE(hangplt_k001604_reg_r, hangplt_k001604_reg_w)
	AM_RANGE(0x74010000, 0x7401ffff) AM_MIRROR(0x80000000) AM_READWRITE(hangplt_paletteram32_r, hangplt_paletteram32_w)
	AM_RANGE(0x74020000, 0x7403ffff) AM_MIRROR(0x80000000) AM_READWRITE(hangplt_k001604_tile_r, hangplt_k001604_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_MIRROR(0x80000000) AM_READWRITE(hangplt_k001604_char_r, hangplt_k001604_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x780c0000, 0x780c0003) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_MIRROR(0x80000000) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x7e008000, 0x7e009fff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_r, lanc_w)
	AM_RANGE(0x7e00a000, 0x7e00bfff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_ram_r, lanc_ram_w)
	AM_RANGE(0x7e00c000, 0x7e00c00f) AM_MIRROR(0x80000000) AM_READWRITE(ppc_sound_r, ppc_sound_w)
	AM_RANGE(0x7f000000, 0x7f3fffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER2, 0)	/* Data ROM */
	AM_RANGE(0x7f800000, 0x7f9fffff) AM_MIRROR(0x80000000) AM_ROM AM_SHARE(2)
	AM_RANGE(0x7fe00000, 0x7fffffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)	/* Program ROM */
ADDRESS_MAP_END

/**********************************************************************/

static ADDRESS_MAP_START( sound_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x300000, 0x30001f) AM_READWRITE(sndcomm68k_r, sndcomm68k_w)
	AM_RANGE(0x400000, 0x400fff) AM_READWRITE(RF5C400_0_r, RF5C400_0_w)		/* Ricoh RF5C400 */
	AM_RANGE(0x500000, 0x500001) AM_READWRITE(MRA16_NOP, soundtimer_en_w)
	AM_RANGE(0x600000, 0x600001) AM_READWRITE(MRA16_NOP, soundtimer_ack_w)
ADDRESS_MAP_END

/*****************************************************************************/

static WRITE16_HANDLER( gn680_sysctrl_w )
{
	/* bit 15 = watchdog toggle, lower 4 bits = LEDs? */
}

static ADDRESS_MAP_START( gn680_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x200000, 0x203fff) AM_RAM
	AM_RANGE(0x300000, 0x300001) AM_WRITE(gn680_sysctrl_w)
ADDRESS_MAP_END

/*****************************************************************************/

static READ32_HANDLER( dsp_dataram_r )
{
	return sharc_dataram[offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram_w )
{
	sharc_dataram[offset] = data;
}

static ADDRESS_MAP_START( sharc_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x400000, 0x41ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x500000, 0x5fffff) AM_READWRITE(dsp_dataram_r, dsp_dataram_w)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(k001005_r, k001005_w)
	AM_RANGE(0x700000, 0x7000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
ADDRESS_MAP_END

static READ32_HANDLER( dsp_dataram1_r )
{
	return sharc_dataram1[offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram1_w )
{
	sharc_dataram1[offset] = data;
}

static ADDRESS_MAP_START( hangplt_sharc0_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x0400000, 0x041ffff) AM_READWRITE(cgboard_0_shared_sharc_r, cgboard_0_shared_sharc_w)
	AM_RANGE(0x0500000, 0x05fffff) AM_READWRITE(dsp_dataram_r, dsp_dataram_w)
	AM_RANGE(0x1400000, 0x14fffff) AM_RAM
	AM_RANGE(0x2400000, 0x27fffff) AM_READWRITE(nwk_voodoo_0_r, voodoo_0_w)
	AM_RANGE(0x3400000, 0x34000ff) AM_READWRITE(cgboard_0_comm_sharc_r, cgboard_0_comm_sharc_w)
	AM_RANGE(0x3401000, 0x34fffff) AM_WRITE(nwk_voodoo_fifo_0_w)
	AM_RANGE(0x3500000, 0x3507fff) AM_READWRITE(cgboard_0_k033906_r, cgboard_0_k033906_w)
	AM_RANGE(0x3600000, 0x37fffff) AM_ROMBANK(5)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hangplt_sharc1_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x0400000, 0x041ffff) AM_READWRITE(cgboard_1_shared_sharc_r, cgboard_1_shared_sharc_w)
	AM_RANGE(0x0500000, 0x05fffff) AM_READWRITE(dsp_dataram1_r, dsp_dataram1_w)
	AM_RANGE(0x1400000, 0x14fffff) AM_RAM
	AM_RANGE(0x2400000, 0x27fffff) AM_READWRITE(nwk_voodoo_1_r, voodoo_1_w)
	AM_RANGE(0x3400000, 0x34000ff) AM_READWRITE(cgboard_1_comm_sharc_r, cgboard_1_comm_sharc_w)
	AM_RANGE(0x3401000, 0x34fffff) AM_WRITE(nwk_voodoo_fifo_1_w)
	AM_RANGE(0x3500000, 0x3507fff) AM_READWRITE(cgboard_1_k033906_r, cgboard_1_k033906_w)
	AM_RANGE(0x3600000, 0x37fffff) AM_ROMBANK(6)
ADDRESS_MAP_END

/*****************************************************************************/

INPUT_PORTS_START( gticlub )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Start/View")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("Shift Up")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("Shift Down")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("AT/MT Switch") PORT_TOGGLE
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x0b, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x03, 0x03, "Network ID" ) PORT_DIPLOCATION("SW:2,1")
	PORT_DIPSETTING( 0x03, "1" )
	PORT_DIPSETTING( 0x02, "2" )
	PORT_DIPSETTING( 0x01, "3" )
	PORT_DIPSETTING( 0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, "DIP3" ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIP4" ) PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_START_TAG("AN0")
	PORT_BIT( 0x3ff, 0x200, IPT_PADDLE ) PORT_NAME("Steering Wheel") PORT_MINMAX(0x004,0x3fb) PORT_SENSITIVITY(50) PORT_KEYDELTA(25)

	PORT_START_TAG("AN1")
	PORT_BIT( 0x3ff, 0x000, IPT_PEDAL ) PORT_NAME("Gas Pedal") PORT_MINMAX(0x000,0x25f) PORT_SENSITIVITY(50) PORT_KEYDELTA(25)

	PORT_START_TAG("AN2")
	PORT_BIT( 0x3ff, 0x000, IPT_PEDAL2 ) PORT_NAME("Brake Pedal") PORT_MINMAX(0x000,0x25f) PORT_SENSITIVITY(50) PORT_KEYDELTA(25)

	PORT_START_TAG("AN3")
	PORT_BIT( 0x3ff, 0x000, IPT_PEDAL3 ) PORT_NAME("Handbrake Lever") PORT_MINMAX(0x000,0x25f) PORT_SENSITIVITY(50) PORT_KEYDELTA(25)
INPUT_PORTS_END

INPUT_PORTS_START( slrasslt )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Start/View")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Trigger")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Missile")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Power Up")

	PORT_START_TAG("IN1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x01, 0x01, "DIP1" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIP2" ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIP3" ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIP4" ) PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_START_TAG("AN0")
	PORT_BIT( 0x3ff, 0x200, IPT_AD_STICK_Y ) PORT_MINMAX(0x008,0x3f7) PORT_SENSITIVITY(50) PORT_KEYDELTA(25)

	PORT_START_TAG("AN1")
	PORT_BIT( 0x3ff, 0x200, IPT_AD_STICK_X ) PORT_MINMAX(0x008,0x3f7) PORT_SENSITIVITY(50) PORT_KEYDELTA(25) PORT_REVERSE

	PORT_START_TAG("AN2")
	PORT_BIT( 0x3ff, 0x000, IPT_UNUSED )

	PORT_START_TAG("AN3")
	PORT_BIT( 0x3ff, 0x000, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( thunderh )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1) PORT_NAME("P1 Trigger")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1) PORT_NAME("P1 Bomb")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2) PORT_NAME("P2 Trigger")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2) PORT_NAME("P2 Bomb")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x01, 0x00, "DIP1" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIP2" ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIP3" ) PORT_DIPLOCATION("SW:3")
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIP4" ) PORT_DIPLOCATION("SW:4")
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( hangplt )
	PORT_INCLUDE( slrasslt )

	PORT_MODIFY("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Start/View")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("Select Left") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("Select Right") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_MODIFY("IN1")
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("Push limit switch")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Pull limit switch")

	PORT_MODIFY("IN3")
	PORT_DIPNAME( 0x01, 0x01, "Disable Machine Init" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Skip Post" ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_MODIFY("AN0")
	PORT_BIT( 0x3ff, 0x200, IPT_AD_STICK_X ) PORT_NAME("Rudder") PORT_MINMAX(0x1c5,0x24a) PORT_SENSITIVITY(25) PORT_KEYDELTA(10) PORT_REVERSE

	PORT_MODIFY("AN1")
	PORT_BIT( 0x3ff, 0x200, IPT_AD_STICK_Y ) PORT_NAME("Control Bar") PORT_MINMAX(0x100,0x2ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(20) PORT_REVERSE
INPUT_PORTS_END

static int adc1038_input_callback(int input)
{
	static const char *const tags[4] = { "AN0", "AN1", "AN2", "AN3" };

	if (input < 4)
		return readinputportbytag_safe(tags[input], 0);

	return 0;
}

/*****************************************************************************/

/* 93C56 EEPROM */
static struct EEPROM_interface eeprom_interface =
{
	8,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxxxx",	/* lock command */
	"*10011xxxxxx",	/* unlock command */
	1,				/* enable_multi_read */
	0				/* reset_delay */
};

static NVRAM_HANDLER( gticlub )
{
	if (read_or_write)
	{
		EEPROM_save(file);
	}
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			EEPROM_load(file);
		}
		else
		{
			const UINT16 *def = (const UINT16 *)memory_region(REGION_USER4);
			UINT8 eepdata[0x200];
			int i;

			/* the first word is still erased when only the magic number is set */
			eeprom_factory_init = (def[0] == 0xffff);

			memset(eepdata, 0xff, sizeof(eepdata));
			for (i = 0; i < 0x80; ++i)
			{
				eepdata[i * 2 + 0] = def[i] >> 8;
				eepdata[i * 2 + 1] = def[i] & 0xff;
			}
			EEPROM_set_data(eepdata, sizeof(eepdata));
		}
	}
}

/*****************************************************************************/

VIDEO_START( gticlub )
{
	if (k001006_start())
		return 1;

	if (k001005_start(DSP_CPU, 512, 384))
		return 1;

	if (k001604_start(0, 0))
		return 1;

	/* the screen holds only the visible area, which starts at 40,28 in the screen timing */
	k001604_set_layer_offset(0, 40, 28);
	return 0;
}

VIDEO_UPDATE( gticlub )
{
	k001604_draw_back_layer(0, bitmap, cliprect);

	if (cgboard_output_3d_enabled())
		k001005_draw(bitmap, cliprect);

	k001604_draw_front_layer(0, bitmap, cliprect);
}

/* the two monitors are stacked in a single screen */
#define HANGPLT_SCREEN_WIDTH	512
#define HANGPLT_SCREEN_HEIGHT	384

static void hangplt_voodoo_vblank_0(int state)
{
	if (state)
		cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ0, ASSERT_LINE);
}

static void hangplt_voodoo_vblank_1(int state)
{
	if (state)
		cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ1, ASSERT_LINE);
}

VIDEO_START( hangplt )
{
	int i;

	if (voodoo_start(0, VOODOO_1, 2, 2, 2) || voodoo_start(1, VOODOO_1, 2, 2, 2))
		return 1;

	hangplt_screen_bitmap = auto_bitmap_alloc(HANGPLT_SCREEN_WIDTH, HANGPLT_SCREEN_HEIGHT);
	if (!hangplt_screen_bitmap)
		return 1;

	for (i = 0; i < 2; ++i)
	{
		voodoo_set_fixed_screen(i, 1);

		hangplt_paletteram[i] = auto_malloc(0x4000 * sizeof(UINT32));
		memset(hangplt_paletteram[i], 0, 0x4000 * sizeof(UINT32));

		if (k001604_start(i, i * 0x4000))
			return 1;

		/* origin of the visible area in the screen timing of each monitor */
		k001604_set_layer_offset(i, 44, 27);
	}

	voodoo_set_vblank_callback(0, hangplt_voodoo_vblank_0);
	voodoo_set_vblank_callback(1, hangplt_voodoo_vblank_1);

	return 0;
}

VIDEO_UPDATE( hangplt )
{
	rectangle clip;
	int i;

	clip.min_x = 0;
	clip.max_x = HANGPLT_SCREEN_WIDTH - 1;
	clip.min_y = 0;
	clip.max_y = HANGPLT_SCREEN_HEIGHT - 1;

	for (i = 0; i < 2; ++i)
	{
		/* the voodoo framebuffer starts at the top left of each monitor */
		voodoo_update(i, hangplt_screen_bitmap, &clip);
		k001604_draw_front_layer(i, hangplt_screen_bitmap, &clip);
		copybitmap(bitmap, hangplt_screen_bitmap, 0, 0, 0, i * HANGPLT_SCREEN_HEIGHT, cliprect, TRANSPARENCY_NONE, 0);
	}
}

/*****************************************************************************/

/* PowerPC interrupts

    IRQ0:  Vblank
    IRQ2:  LANC
    DMA0

*/
static INTERRUPT_GEN( gticlub_vblank )
{
	cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ0, ASSERT_LINE);
	cpunum_set_input_line(DSP_CPU, SHARC_INPUT_FLAG1, ASSERT_LINE);
}

static ppc_config gticlub_ppc_cfg =
{
	PPC_MODEL_403GA
};

static sharc_config sharc_cfg =
{
	BOOT_MODE_EPROM
};

static struct RF5C400interface rf5c400_interface =
{
	REGION_SOUND1
};

static void gticlub_reset_common(void)
{
	lanc_status = 0x08;
	lanc_irq = CLEAR_LINE;
	sound_irq_enabled = 0;
	sound_irq_pending = 0;
	snd_int_pending = snd_int_enabled = 0;
	memset(host_to_snd_regs, 0, sizeof(host_to_snd_regs));
	memset(snd_to_host_regs, 0, sizeof(snd_to_host_regs));

	adc1038_reset();

	cpunum_set_input_line(DSP_CPU, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_RESET( gticlub )
{
	gticlub_reset_common();
}

static MACHINE_RESET( hangplt )
{
	gticlub_reset_common();

	cpunum_set_input_line(DSP2_CPU, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_DRIVER_START( gticlub )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(gticlub_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(gticlub_map, 0)
	MDRV_CPU_VBLANK_INT(gticlub_vblank, 1)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)
	MDRV_CPU_PERIODIC_INT(sound_irq, TIME_IN_HZ(33868800.0 / 2 / 384 / 128))	/* 344.5Hz (44100 / 128) */

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(gticlub)
	MDRV_MACHINE_RESET(gticlub)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(512, 384)
	MDRV_VISIBLE_AREA(0, 511, 0, 383)
	MDRV_PALETTE_LENGTH(16384)

	MDRV_VIDEO_START(gticlub)
	MDRV_VIDEO_UPDATE(gticlub)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(RF5C400, 0)
	MDRV_SOUND_CONFIG(rf5c400_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( thunderh )
	MDRV_IMPORT_FROM(gticlub)

	MDRV_CPU_ADD(M68000, 32000000/2)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(gn680_memmap, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hangplt )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(gticlub_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(hangplt_map, 0)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)
	MDRV_CPU_PERIODIC_INT(sound_irq, TIME_IN_HZ(33868800.0 / 2 / 384 / 128))	/* 344.5Hz (44100 / 128) */

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(hangplt_sharc0_map, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(hangplt_sharc1_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(gticlub)
	MDRV_MACHINE_RESET(hangplt)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(HANGPLT_SCREEN_WIDTH, HANGPLT_SCREEN_HEIGHT * 2)
	MDRV_VISIBLE_AREA(0, HANGPLT_SCREEN_WIDTH - 1, 0, HANGPLT_SCREEN_HEIGHT * 2 - 1)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(hangplt)
	MDRV_VIDEO_UPDATE(hangplt)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(RF5C400, 0)
	MDRV_SOUND_CONFIG(rf5c400_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

/*************************************************************************/

ROM_START( gticlub )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688eaa01.21u", 0x000000, 0x80000, CRC(824944ad) SHA1(a7bb86a2495e0579f5d82808aeed4895be2dbe3b) )
	ROM_LOAD32_BYTE("688eaa02.19u", 0x000001, 0x80000, CRC(88e7bfb9) SHA1(fc0e945291204ee0c82bbd2c81ff241e1565c6ae) )
	ROM_LOAD32_BYTE("688eaa03.21r", 0x000002, 0x80000, CRC(ea1c696b) SHA1(fd778afaa1de3a35b38a67b8e4c9a08fe9cf1b9e) )
	ROM_LOAD32_BYTE("688eaa04.19r", 0x000003, 0x80000, CRC(94fa2334) SHA1(04edf840f841b9713fa93e7ebb6aad2000b738c0) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d) )
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "688a07.13k", 0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "688a09.9s", 0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
	ROM_LOAD( "688a10.7s", 0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
	ROM_LOAD( "688a11.5s", 0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
	ROM_LOAD( "688a12.2s", 0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "688a13.18d", 0x000000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
	ROM_LOAD64_WORD( "688a14.13d", 0x000002, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
	ROM_LOAD64_WORD( "688a15.9d",  0x000004, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
	ROM_LOAD64_WORD( "688a16.4d",  0x000006, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )

	ROM_REGION16_BE( 0x100, REGION_USER4, 0 )	/* default EEPROM */
	ROM_LOAD16_WORD_SWAP( "93c56.24g", 0x0000, 0x0100, CRC(9564a685) SHA1(ec19f3d6e3a55eac4dab6da5ede7216f002b3186) )
ROM_END

ROM_START( gticlubu )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688uaa01.21u", 0x000000, 0x80000, CRC(4e2ea7ad) SHA1(cc517df7c4df098896a2a88843fef97c9beb46f3) )
	ROM_LOAD32_BYTE("688uaa02.19u", 0x000001, 0x80000, CRC(c0212ce1) SHA1(7716acfa1b1391e9d7a321ed46785c144d27fdd8) )
	ROM_LOAD32_BYTE("688uaa03.21r", 0x000002, 0x80000, CRC(030246fe) SHA1(70d3591159b07aaeca60141db44f7c28d1b2dac9) )
	ROM_LOAD32_BYTE("688uaa04.19r", 0x000003, 0x80000, CRC(9394e0b2) SHA1(9ff4ff22a307352bf127fc2b5ef9c56ecacf0aab) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d) )
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "688a07.13k", 0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "688a09.9s", 0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
	ROM_LOAD( "688a10.7s", 0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
	ROM_LOAD( "688a11.5s", 0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
	ROM_LOAD( "688a12.2s", 0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "688a13.18d", 0x000000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
	ROM_LOAD64_WORD( "688a14.13d", 0x000002, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
	ROM_LOAD64_WORD( "688a15.9d",  0x000004, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
	ROM_LOAD64_WORD( "688a16.4d",  0x000006, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )

	ROM_REGION16_BE( 0x100, REGION_USER4, 0 )	/* default EEPROM */
	ROM_LOAD16_WORD_SWAP("93c56.24g", 0x0000, 0x0100, CRC(9564a685) SHA1(ec19f3d6e3a55eac4dab6da5ede7216f002b3186))
ROM_END

ROM_START( gticluba )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688aaa01.21u", 0x000000, 0x80000, CRC(06a56474) SHA1(3a457b885a35e3ee030fd51d847bcf75fce46208) )
	ROM_LOAD32_BYTE("688aaa02.19u", 0x000001, 0x80000, CRC(3c1e714a) SHA1(557f8542b855b2b35f242c8db7396017aca6dbd8) )
	ROM_LOAD32_BYTE("688aaa03.21r", 0x000002, 0x80000, CRC(e060580b) SHA1(50242f3f3b949cc03082e4e75d9dcc89e17f0a75) )
	ROM_LOAD32_BYTE("688aaa04.19r", 0x000003, 0x80000, CRC(928c23cd) SHA1(cce54398e1e5b98bfb717839cc422f1f60502788) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d) )
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "688a07.13k", 0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "688a09.9s", 0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
	ROM_LOAD( "688a10.7s", 0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
	ROM_LOAD( "688a11.5s", 0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
	ROM_LOAD( "688a12.2s", 0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "688a13.18d", 0x000000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
	ROM_LOAD64_WORD( "688a14.13d", 0x000002, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
	ROM_LOAD64_WORD( "688a15.9d",  0x000004, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
	ROM_LOAD64_WORD( "688a16.4d",  0x000006, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )

	ROM_REGION16_BE( 0x100, REGION_USER4, 0 )	/* default EEPROM */
	ROM_LOAD16_WORD_SWAP("93c56.24g", 0x0000, 0x0100, CRC(9564a685) SHA1(ec19f3d6e3a55eac4dab6da5ede7216f002b3186))
ROM_END

ROM_START( gticlubj )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("688jaa01.21u", 0x000000, 0x80000, CRC(1492059c) SHA1(176dbd87f23f4cd8e1397e67da501738e20e5a57) )
	ROM_LOAD32_BYTE("688jaa02.19u", 0x000001, 0x80000, CRC(7896dd69) SHA1(a3ab7b872132a5e66238e414f4b497cf7beb8b1c) )
	ROM_LOAD32_BYTE("688jaa03.21r", 0x000002, 0x80000, CRC(94e2be50) SHA1(f206ac201903f3aae29196ab6fccdef104859346) )
	ROM_LOAD32_BYTE("688jaa04.19r", 0x000003, 0x80000, CRC(ff539bb6) SHA1(1a225eca4377d82a2b6cb99c1d16580b9ccf2f08) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d) )
	ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "688a07.13k", 0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "688a09.9s", 0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
	ROM_LOAD( "688a10.7s", 0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
	ROM_LOAD( "688a11.5s", 0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
	ROM_LOAD( "688a12.2s", 0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "688a13.18d", 0x000000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
	ROM_LOAD64_WORD( "688a14.13d", 0x000002, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
	ROM_LOAD64_WORD( "688a15.9d",  0x000004, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
	ROM_LOAD64_WORD( "688a16.4d",  0x000006, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )

	ROM_REGION16_BE( 0x100, REGION_USER4, 0 )	/* default EEPROM */
	ROM_LOAD16_WORD_SWAP("93c56.24g", 0x0000, 0x0100, CRC(9564a685) SHA1(ec19f3d6e3a55eac4dab6da5ede7216f002b3186))
ROM_END

ROM_START( thunderh )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "680eaa01.21u", 0x000000, 0x080000, CRC(796e2678) SHA1(8051a228aa6d1a3f1fef26de15f4fdb785c2c8ee) )
	ROM_LOAD32_BYTE( "680eaa02.19u", 0x000001, 0x080000, CRC(767e6db0) SHA1(0f29f56fe485f30100ce54e64bda5d5a124c1d09) )
	ROM_LOAD32_BYTE( "680eaa03.21r", 0x000002, 0x080000, CRC(5a5b59b5) SHA1(542c0722437f40829559b09120fde995246d52ae) )
	ROM_LOAD32_BYTE( "680eaa04.19r", 0x000003, 0x080000, CRC(4a973a5c) SHA1(1d84f6416c3b5a85d7ebfbc15fc08e0dd8dc2414) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "680a05.14u", 0x000000, 0x200000, CRC(0c9f334d) SHA1(99ac622a04a7140244d81031df69a796b6fd2657) )
	ROM_LOAD32_WORD_SWAP( "680a06.12u", 0x000002, 0x200000, CRC(83074217) SHA1(bbf782ac125cd98d9147ef4e0373bf61f74726f7) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "680a07.13k", 0x000000, 0x080000, CRC(12247a3e) SHA1(846cd9423efd3c9b17fce08393c6c83307d72f92) )

	ROM_REGION(0x20000, REGION_CPU4, 0)	/* GN680 program */
	ROM_LOAD16_WORD_SWAP( "680c22.20k", 0x000000, 0x020000, CRC(d93c0ee2) SHA1(4b58418cbb01b51e12d6e7c86b2c81cd35d86248) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "680a09.9s", 0x000000, 0x200000, CRC(71c2b049) SHA1(ce360172c8774b31edf16a80104c35b1caf26cd9) )
	ROM_LOAD( "680a10.7s", 0x200000, 0x200000, CRC(19882bf3) SHA1(7287da58853c84cbadbfb42bed37f2b0032c4b4d) )
	ROM_LOAD( "680a11.5s", 0x400000, 0x200000, CRC(0c74fe3f) SHA1(2e69f8d37552a74bbda65b134f747b4380ed33b0) )
	ROM_LOAD( "680a12.2s", 0x600000, 0x200000, CRC(b052919d) SHA1(a61c8eaf378ab7d780478db61217302d1b9f8f73) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "680a13.18d", 0x000000, 0x200000, CRC(233f9074) SHA1(78ce42c35407d61df37cc0d16cdee84f8de968fa) )
	ROM_LOAD64_WORD( "680a14.13d", 0x000002, 0x200000, CRC(9ae15033) SHA1(12e291114629632b81f53811a6c8666aff4e92f3) )
	ROM_LOAD64_WORD( "680a15.9d",  0x000004, 0x200000, CRC(dc47c86f) SHA1(71af9b21f1ecc063135f501b1561869ee910c236) )
	ROM_LOAD64_WORD( "680a16.4d",  0x000006, 0x200000, CRC(ea388143) SHA1(3de5314a009d702186d5e285c8edefdd48139eab) )

	ROM_REGION16_BE(0x100, REGION_USER4, 0)	/* default EEPROM */
	ROM_LOAD16_WORD_SWAP("93c56.24g", 0x0000, 0x0100, CRC(e6524263) SHA1(de6c614fad8049fa6cebe09722cadae656457d69))
ROM_END

ROM_START( thunderhu )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "680uaa01.21u", 0x000000, 0x080000, CRC(f2bb2ba1) SHA1(311e88d63179486014376c4af4ff0ef28673ee5a) )
	ROM_LOAD32_BYTE( "680uaa02.19u", 0x000001, 0x080000, CRC(52f617b5) SHA1(fda3133d3a7e04eb4432c69becdcf1872b3660d9) )
	ROM_LOAD32_BYTE( "680uaa03.21r", 0x000002, 0x080000, CRC(086a0574) SHA1(32fb93dbb93d2fe6af743ea4310b50a6cd03647d) )
	ROM_LOAD32_BYTE( "680uaa04.19r", 0x000003, 0x080000, CRC(85e1f8e3) SHA1(9172c54b6663f1bf390795068271198083a6860d) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "680a05.14u", 0x000000, 0x200000, CRC(0c9f334d) SHA1(99ac622a04a7140244d81031df69a796b6fd2657) )
	ROM_LOAD32_WORD_SWAP( "680a06.12u", 0x000002, 0x200000, CRC(83074217) SHA1(bbf782ac125cd98d9147ef4e0373bf61f74726f7) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "680a07.13k", 0x000000, 0x080000, CRC(12247a3e) SHA1(846cd9423efd3c9b17fce08393c6c83307d72f92) )

	ROM_REGION(0x20000, REGION_CPU4, 0)	/* GN680 program */
	ROM_LOAD16_WORD_SWAP( "680c22.20k", 0x000000, 0x020000, CRC(d93c0ee2) SHA1(4b58418cbb01b51e12d6e7c86b2c81cd35d86248) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "680a09.9s", 0x000000, 0x200000, CRC(71c2b049) SHA1(ce360172c8774b31edf16a80104c35b1caf26cd9) )
	ROM_LOAD( "680a10.7s", 0x200000, 0x200000, CRC(19882bf3) SHA1(7287da58853c84cbadbfb42bed37f2b0032c4b4d) )
	ROM_LOAD( "680a11.5s", 0x400000, 0x200000, CRC(0c74fe3f) SHA1(2e69f8d37552a74bbda65b134f747b4380ed33b0) )
	ROM_LOAD( "680a12.2s", 0x600000, 0x200000, CRC(b052919d) SHA1(a61c8eaf378ab7d780478db61217302d1b9f8f73) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "680a13.18d", 0x000000, 0x200000, CRC(233f9074) SHA1(78ce42c35407d61df37cc0d16cdee84f8de968fa) )
	ROM_LOAD64_WORD( "680a14.13d", 0x000002, 0x200000, CRC(9ae15033) SHA1(12e291114629632b81f53811a6c8666aff4e92f3) )
	ROM_LOAD64_WORD( "680a15.9d",  0x000004, 0x200000, CRC(dc47c86f) SHA1(71af9b21f1ecc063135f501b1561869ee910c236) )
	ROM_LOAD64_WORD( "680a16.4d",  0x000006, 0x200000, CRC(ea388143) SHA1(3de5314a009d702186d5e285c8edefdd48139eab) )

	ROM_REGION16_BE(0x100, REGION_USER4, 0)	/* default EEPROM */
	ROM_LOAD16_WORD_SWAP("93c56.24g", 0x0000, 0x0100, CRC(e6524263) SHA1(de6c614fad8049fa6cebe09722cadae656457d69))
ROM_END

ROM_START( slrasslt )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "792uaa01.21u", 0x000000, 0x080000, CRC(c73bf7fb) SHA1(ffe0fea155473827929339a9261a158287ce30a8) )
	ROM_LOAD32_BYTE( "792uaa02.19u", 0x000001, 0x080000, CRC(a940bb9b) SHA1(65a60157697a21cc2485c02c689c9addb3ac91f1) )
	ROM_LOAD32_BYTE( "792uaa03.21r", 0x000002, 0x080000, CRC(363e8411) SHA1(b9c70033d8e3de4b339b61a66172bfecb7c2b3ab) )
	ROM_LOAD32_BYTE( "792uaa04.19r", 0x000003, 0x080000, CRC(7910d99c) SHA1(e2114d369060528998b58331d590c086d306f541) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "792a05.14u", 0x000000, 0x200000, CRC(9a27edfc) SHA1(c028b6440eb1b0c814c4db45918e580662ac2d9a) )
	ROM_LOAD32_WORD_SWAP( "792a06.12u", 0x000002, 0x200000, CRC(c272f171) SHA1(df492287eadc5e8668fe46cfa3ed3ca77c57feca) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "792a07.10k", 0x000000, 0x080000, CRC(89a65ad1) SHA1(d814ef0b560c8e68da57ad5c6096e4fc05e9913e) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "792a09.9s", 0x000000, 0x200000, CRC(7d7ea427) SHA1(a9a311a7c17223cc87140fe2890e20a321464831) )
	ROM_LOAD( "792a10.7s", 0x200000, 0x200000, CRC(e585e5d9) SHA1(ec44ad324a66eeea4c45933dda5a8a9a4398879d) )
	ROM_LOAD( "792a11.5s", 0x400000, 0x200000, CRC(c9c3a04c) SHA1(f834659f67712c9fcd93b7407669d7f35517b790) )
	ROM_LOAD( "792a12.2s", 0x600000, 0x200000, CRC(da8fcdd5) SHA1(daa7b3a086ada69e93c3d7cd9130befc79e422dc) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "792a13.18d", 0x000000, 0x200000, CRC(16d6a134) SHA1(3f53f3c6759d7c5f40aa25a598df899fbac35a60) )
	ROM_LOAD64_WORD( "792a14.13d", 0x000002, 0x200000, CRC(cf57e830) SHA1(607b4dec3b8180a63e29d9dab1ca28d7226dda1e) )
	ROM_LOAD64_WORD( "792a15.9d",  0x000004, 0x200000, CRC(1c5531cb) SHA1(1b514f181c92e16d07bfe4719604f1e4caf15377) )
	ROM_LOAD64_WORD( "792a16.4d",  0x000006, 0x200000, CRC(df89e392) SHA1(af37c5460d43bf8d8a1ab4213c4528083a7363c2) )

	ROM_REGION16_BE(0x100, REGION_USER4, 0)	/* default EEPROM */
	ROM_LOAD16_WORD( "eeprom-slrasslt.bin", 0x0000, 0x0100, CRC(51eb4d93) SHA1(bc1359daccad80b0e16eb144a0bae715a4fb2e8d) )
ROM_END

ROM_START( slrassltj )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "792jaa01.21u", 0x000000, 0x080000, CRC(112717c6) SHA1(be5066e1aefef20b6eab2340abc1bdc3d7a5a6e3) )
	ROM_LOAD32_BYTE( "792jaa02.19u", 0x000001, 0x080000, CRC(c48582bd) SHA1(194dfd51704ed5eeecb2b56b6bbf651c7cf7701e) )
	ROM_LOAD32_BYTE( "792jaa03.21r", 0x000002, 0x080000, CRC(e691009d) SHA1(c8ae58fd280a18151b0e33511269c3685e30fe63) )
	ROM_LOAD32_BYTE( "792jaa04.19r", 0x000003, 0x080000, CRC(1e73a145) SHA1(e519d17d22b5a61570a9bf72ea840f6398928952) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "792a05.14u", 0x000000, 0x200000, CRC(9a27edfc) SHA1(c028b6440eb1b0c814c4db45918e580662ac2d9a) )
	ROM_LOAD32_WORD_SWAP( "792a06.12u", 0x000002, 0x200000, CRC(c272f171) SHA1(df492287eadc5e8668fe46cfa3ed3ca77c57feca) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "792a07.10k", 0x000000, 0x080000, CRC(89a65ad1) SHA1(d814ef0b560c8e68da57ad5c6096e4fc05e9913e) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "792a09.9s", 0x000000, 0x200000, CRC(7d7ea427) SHA1(a9a311a7c17223cc87140fe2890e20a321464831) )
	ROM_LOAD( "792a10.7s", 0x200000, 0x200000, CRC(e585e5d9) SHA1(ec44ad324a66eeea4c45933dda5a8a9a4398879d) )
	ROM_LOAD( "792a11.5s", 0x400000, 0x200000, CRC(c9c3a04c) SHA1(f834659f67712c9fcd93b7407669d7f35517b790) )
	ROM_LOAD( "792a12.2s", 0x600000, 0x200000, CRC(da8fcdd5) SHA1(daa7b3a086ada69e93c3d7cd9130befc79e422dc) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "792a13.18d", 0x000000, 0x200000, CRC(16d6a134) SHA1(3f53f3c6759d7c5f40aa25a598df899fbac35a60) )
	ROM_LOAD64_WORD( "792a14.13d", 0x000002, 0x200000, CRC(cf57e830) SHA1(607b4dec3b8180a63e29d9dab1ca28d7226dda1e) )
	ROM_LOAD64_WORD( "792a15.9d",  0x000004, 0x200000, CRC(1c5531cb) SHA1(1b514f181c92e16d07bfe4719604f1e4caf15377) )
	ROM_LOAD64_WORD( "792a16.4d",  0x000006, 0x200000, CRC(df89e392) SHA1(af37c5460d43bf8d8a1ab4213c4528083a7363c2) )

	ROM_REGION16_BE(0x100, REGION_USER4, 0)	/* default EEPROM */
	ROM_LOAD16_WORD( "eeprom-slrasslt.bin", 0x0000, 0x0100, CRC(407871d6) SHA1(17a311c412f450edb206750bf7d1055bd16a2135) )
ROM_END

ROM_START( slrassltj1 )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "672jaa_a01.21u", 0x000000, 0x080000, CRC(e2821f51) SHA1(20c6c2402ba2b564b8f77bcf452abe2d7e023417) )
	ROM_LOAD32_BYTE( "672jaa_a02.19u", 0x000001, 0x080000, CRC(e3ac7031) SHA1(268588ac6e80463e51a399f53b2396b23faaddba) )
	ROM_LOAD32_BYTE( "672jaa_a03.21r", 0x000002, 0x080000, CRC(52711d79) SHA1(8c89fbff9de21cc1e5f17c4ea08870faea648465) )
	ROM_LOAD32_BYTE( "672jaa_a04.19r", 0x000003, 0x080000, CRC(f7419454) SHA1(44cef7f1181cb9c11b013ab0b7e26aa1e95d3746) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "672a05.14u", 0x000000, 0x200000, CRC(f6f296e4) SHA1(2ba4ede36f3392aa53a730614272fa80df65281c) )
	ROM_LOAD32_WORD_SWAP( "672a06.12u", 0x000002, 0x200000, CRC(2126227f) SHA1(22615023025453772239b4b21e276fbcbb1cc2bf) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "672a07.10k", 0x000000, 0x080000, CRC(a757309d) SHA1(bc1c8b327815f70bc8bf94adafc8bb0c215e7d1f) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "672a09.9s", 0x000000, 0x200000, CRC(484355ef) SHA1(2af65565d10b058ab95888f36ff28bb7909181d5) )
	ROM_LOAD( "672a10.7s", 0x200000, 0x200000, CRC(1ec1d5d1) SHA1(ebf5417a117f352fff36806a748305270adb70d8) )
	ROM_LOAD( "672a11.5s", 0x400000, 0x200000, CRC(1126753e) SHA1(c7b91e2514329799944e1eb608b5b7e2eb87bea9) )
	ROM_LOAD( "672a12.2s", 0x600000, 0x200000, CRC(432ec7fd) SHA1(a671625400f3837cbe8ddb06dbc37c574d75e281) )

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
	ROM_LOAD64_WORD( "672a13.18d", 0x000000, 0x200000, CRC(ded2f06d) SHA1(851a896e156fb736bcb7cdfc7db2340bb819c092) )
	ROM_LOAD64_WORD( "672a14.13d", 0x000002, 0x200000, CRC(cd311cfc) SHA1(ead708eaa4e3f1f2b9a17c41ddd0fbc016911527) )
	ROM_LOAD64_WORD( "672a15.9d",  0x000004, 0x200000, CRC(7bb6c271) SHA1(9eb928a52e482b7718c723fa8a14b2c2faaf4425) )
	ROM_LOAD64_WORD( "672a16.4d",  0x000006, 0x200000, CRC(6fa5c0ee) SHA1(ee40179a46f8529fab1c58c05f413a516e7d53ff) )

	ROM_REGION16_BE(0x100, REGION_USER4, 0)	/* default EEPROM */
	ROM_LOAD16_WORD( "eeprom-slrassltj.bin", 0x0000, 0x0100, CRC(407871d6) SHA1(17a311c412f450edb206750bf7d1055bd16a2135) )
ROM_END

ROM_START( hangplt )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "685jab01.21u", 0x000000, 0x080000, CRC(f98a3e82) SHA1(94ebaa172b0e98c5cd08efaea5f56e707e5032b4) )
	ROM_LOAD32_BYTE( "685jab02.19u", 0x000001, 0x080000, CRC(20730cdc) SHA1(71b2cf7077ab7db875f9030e21afd05905f57ce5) )
	ROM_LOAD32_BYTE( "685jab03.21r", 0x000002, 0x080000, CRC(77fa2248) SHA1(a662b84945b3d268fed15952cc793d821233735e) )
	ROM_LOAD32_BYTE( "685jab04.19r", 0x000003, 0x080000, CRC(ab6773df) SHA1(91d3f849a1cc5fa4b2fbd876d53402a548198c41) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "685a05.14u", 0x000000, 0x200000, CRC(ba1c8f40) SHA1(ce4ed641c1d6d44447eaaada16f305f1d7fb9ee2) )
	ROM_LOAD32_WORD_SWAP( "685a06.12u", 0x000002, 0x200000, CRC(2429935c) SHA1(4da9e169adcac81ea1bc135d727c2bd13ad372fa) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "685a07.13k", 0x000000, 0x080000, CRC(5b72fd80) SHA1(a150837fa0d66dc0c3832495a4c8ce4f9b92cd98) )

	ROM_REGION(0x1000000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "685a09.9s", 0x000000, 0x400000, CRC(b8ae40aa) SHA1(eee27a8929e0e805f1045fd9638e661b36a1e3c7) )
	ROM_LOAD( "685a10.7s", 0x400000, 0x400000, CRC(fef3dc36) SHA1(566c7469fc452b5965a31fa42291082ec8e48a24) )

	ROM_REGION(0x1000000, REGION_USER5, ROMREGION_ERASE00)	/* texture roms */
	ROM_LOAD32_WORD( "685a13.4w",  0x000002, 0x400000, CRC(06329af4) SHA1(76cad9db604751ce48bb67bfd29e57bac0ee9a16) )
	ROM_LOAD32_WORD( "685a14.12w", 0x000000, 0x400000, CRC(87437739) SHA1(0d45637af40938a54d5efd29c125b0fafd55f9a4) )

	ROM_REGION16_BE( 0x100, REGION_USER4, 0 )	/* default EEPROM */
	ROM_LOAD( "hangplt.nv", 0x0000, 0x0100, CRC(30285221) SHA1(a08d06a0d7966f483e4c691a9bd5a98e48294aab) )
ROM_END

ROM_START( hangpltu )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE( "685uaa01.21u", 0x000000, 0x080000, CRC(83a5b866) SHA1(6859590f212c7debb19924f0174e4cd1bfc011bc) )
	ROM_LOAD32_BYTE( "685uaa02.19u", 0x000001, 0x080000, CRC(765906d6) SHA1(9085a2346756b3b628fef91b7afc131aba434654) )
	ROM_LOAD32_BYTE( "685uaa03.21r", 0x000002, 0x080000, CRC(cb0147a3) SHA1(7bcab760c01ea7e24f4ca5793e081aafa97f68a3) )
	ROM_LOAD32_BYTE( "685uaa04.19r", 0x000003, 0x080000, CRC(a5fda56b) SHA1(4d86f488f411ec16fa3be830206a44214941d1fe) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP( "685a05.14u", 0x000000, 0x200000, CRC(ba1c8f40) SHA1(ce4ed641c1d6d44447eaaada16f305f1d7fb9ee2) )
	ROM_LOAD32_WORD_SWAP( "685a06.12u", 0x000002, 0x200000, CRC(2429935c) SHA1(4da9e169adcac81ea1bc135d727c2bd13ad372fa) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
	ROM_LOAD16_WORD_SWAP( "685a07.13k", 0x000000, 0x080000, CRC(5b72fd80) SHA1(a150837fa0d66dc0c3832495a4c8ce4f9b92cd98) )

	ROM_REGION(0x1000000, REGION_SOUND1, 0)	/* sound roms */
	ROM_LOAD( "685a09.9s", 0x000000, 0x400000, CRC(b8ae40aa) SHA1(eee27a8929e0e805f1045fd9638e661b36a1e3c7) )
	ROM_LOAD( "685a10.7s", 0x400000, 0x400000, CRC(fef3dc36) SHA1(566c7469fc452b5965a31fa42291082ec8e48a24) )

	ROM_REGION(0x1000000, REGION_USER5, ROMREGION_ERASE00)	/* texture roms */
	ROM_LOAD32_WORD( "685a13.4w",  0x000002, 0x400000, CRC(06329af4) SHA1(76cad9db604751ce48bb67bfd29e57bac0ee9a16) )
	ROM_LOAD32_WORD( "685a14.12w", 0x000000, 0x400000, CRC(87437739) SHA1(0d45637af40938a54d5efd29c125b0fafd55f9a4) )

	ROM_REGION16_BE( 0x100, REGION_USER4, 0 )	/* default EEPROM */
	ROM_LOAD( "hangpltu.nv", 0x0000, 0x0100, CRC(cfea8a2b) SHA1(94162441d401c95b8c6a761d32b656ce22c3eeb6) )
ROM_END

static READ32_HANDLER( gticlub_idle_r )
{
	UINT32 data = work_ram[(0x260 / 4) + offset];

	/*
	 * The main loop calls a function that compares the head and tail of
	 * the task queue at 0x80000264, and the tasks are only added by the
	 * interrupt handlers. When the queue is empty, the polling is skipped
	 * until the next interrupt.
	 */
	if (offset == 2 && activecpu_get_pc() == idle_pc && work_ram[0x264 / 4] >= data)
		cpu_spinuntil_int();

	return data;
}

static void gticlub_install_idle_skip(void)
{
	/* lwz r0,0x160(r31); lwz r3,0x164(r31); cmpl cr0,0,r0,r3 */
	static const UINT32 sig[3] = { 0x801f0160, 0x807f0164, 0x7c001840 };
	const UINT32 *rom = (const UINT32 *)memory_region(REGION_USER1);
	UINT32 len = memory_region_length(REGION_USER1) / 4;
	UINT32 i;

	for (i = 0; i + 3 <= len; ++i)
	{
		if (rom[i] == sig[0] && rom[i + 1] == sig[1] && rom[i + 2] == sig[2])
		{
			/* the program ROM is mapped at 0xff800000, the skip triggers on the read of the queue tail */
			idle_pc = 0xff800000 + (i + 1) * 4;
			memory_install_read32_handler(PPC_CPU, ADDRESS_SPACE_PROGRAM, 0x00000260, 0x0000026f, 0, 0x80000000, gticlub_idle_r);
			return;
		}
	}
}

static READ32_HANDLER( slrasslt_idle_r )
{
	UINT32 ptr = work_ram[(idle_addr & 0xfffff) / 4];

	/*
	 * The main loop, decompressed in RAM, calls a function that loads the
	 * pointer to the task list and checks its 16 bit task count, which is
	 * only increased by the interrupt handlers. When it's zero, the polling
	 * is skipped until the next interrupt.
	 */
	if (activecpu_get_pc() == idle_pc)
	{
		UINT32 addr = ptr & 0xfffff;
		UINT32 word = work_ram[addr / 4];
		UINT16 count = (addr & 2) ? (word & 0xffff) : (word >> 16);

		if (count == 0)
			cpu_spinuntil_int();
	}

	return ptr;
}

static void slrasslt_install_idle_skip(UINT32 pc, UINT32 addr)
{
	idle_pc = pc;
	idle_addr = addr;
	memory_install_read32_handler(PPC_CPU, ADDRESS_SPACE_PROGRAM, addr & 0x7fffffff, (addr & 0x7fffffff) + 3, 0, 0x80000000, slrasslt_idle_r);
}

static READ32_HANDLER( hangplt_idle_r )
{
	UINT32 base = idle_addr & 0xfffff;
	UINT32 data = work_ram[(base + 0xa0) / 4];

	/*
	 * The main loop calls a function that compares the head and tail of
	 * the task queue, then it polls a 16 bit flag. Both are only changed by
	 * the interrupt handlers, so when the queue is empty and the flag is
	 * clear, the polling is skipped until the next interrupt. The return
	 * address selects the call from the main loop.
	 */
	if (activecpu_get_pc() == idle_pc && activecpu_get_reg(PPC_LR) == idle_lr)
	{
		UINT32 flag = work_ram[(base + 0xb2) / 4];

		if (work_ram[(base + 0x9c) / 4] >= data && ((base + 0xb2) & 2 ? (flag & 0xffff) : (flag >> 16)) == 0)
			cpu_spinuntil_int();
	}

	return data;
}

static void hangplt_install_idle_skip(UINT32 pc, UINT32 lr, UINT32 addr)
{
	idle_pc = pc;
	idle_lr = lr;
	idle_addr = addr;
	/* the handler covers the queue tail at offset 0xa0 of the task structure */
	memory_install_read32_handler(PPC_CPU, ADDRESS_SPACE_PROGRAM, (addr & 0x7fffffff) + 0xa0, (addr & 0x7fffffff) + 0xa3, 0, 0x80000000, hangplt_idle_r);
}

static UINT32 gticlub_tb_read_hook(UINT32 tblo)
{
	UINT32 now, target, r30, r31;

	if (activecpu_get_pc() != tb_wait_pc || activecpu_get_reg(PPC_LR) != tb_wait_lr)
		return 0;

	/*
	 * The boot delay loop waits until the time base plus r27 reaches r31,
	 * and kicks the watchdog every time it reaches r30. The cycles up to the
	 * nearest of the two are consumed at once, with the unsigned compares
	 * of the loop.
	 */
	now = tblo + activecpu_get_reg(PPC_R27);
	r30 = activecpu_get_reg(PPC_R30);
	r31 = activecpu_get_reg(PPC_R31);
	target = (r30 < r31) ? r30 : r31;

	if (now >= target)
		return 0;

	return target - now;
}

static void gticlub_install_tb_wait_skip(void)
{
	/* addc r3,r3,r27; cmpl cr0,0,r3,r30; cmpl cr1,0,r3,r31; addis r3,r30,1; bt cr1[lt],-0x24 */
	static const UINT32 sig[5] = { 0x7c63d814, 0x7c03f040, 0x7c83f840, 0x3c7e0001, 0x4184ffdc };
	const UINT32 *rom = (const UINT32 *)memory_region(REGION_USER1);
	UINT32 len = memory_region_length(REGION_USER1) / 4;
	UINT32 i;

	for (i = 1; i + 5 <= len; ++i)
	{
		if (rom[i] == sig[0] && rom[i + 1] == sig[1] && rom[i + 2] == sig[2] && rom[i + 3] == sig[3] && rom[i + 4] == sig[4])
		{
			/* the loop starts with a call to the function returning the time base low word */
			INT32 disp = ((INT32)(rom[i - 1] << 6)) >> 6 & ~3;
			UINT32 func = (i - 1) * 4 + disp;

			if ((rom[i - 1] >> 26) != 18 || func / 4 + 1 >= len || rom[func / 4] != 0x7c7df2a6 || rom[func / 4 + 1] != 0x4e800020)
				return;

			/* the program ROM is mapped at 0xff800000 */
			tb_wait_pc = 0xff800000 + func;
			tb_wait_lr = 0xff800000 + i * 4;
			ppc403_set_tb_read_hook(gticlub_tb_read_hook);
			return;
		}
	}
}

static void gticlub_init_common(int adc1038_gticlub_hack)
{
	init_konami_cgboard(0, CGBOARD_TYPE_GTICLUB);
	sharc_dataram = auto_malloc(0x100000 * sizeof(UINT32));
	adc1038_init(adc1038_input_callback, adc1038_gticlub_hack);
}

static DRIVER_INIT( gticlub )
{
	gticlub_init_common(1);
	gticlub_install_idle_skip();
	gticlub_install_tb_wait_skip();
}

static DRIVER_INIT( thunderh )
{
	gticlub_init_common(0);
}

static DRIVER_INIT( slrasslt )
{
	gticlub_init_common(0);
	/* lwz r29,0x0068(r2) with r2 = 0x80019c00 */
	slrasslt_install_idle_skip(0x800155e8, 0x80019c68);
}

static DRIVER_INIT( slrassltj1 )
{
	gticlub_init_common(0);
	/* lwz r29,0x005c(r2) with r2 = 0x8001b30c */
	slrasslt_install_idle_skip(0x80015ce4, 0x8001b368);
}

static void hangplt_init_common(void)
{
	init_konami_cgboard(0, CGBOARD_TYPE_HANGPLT);
	init_konami_cgboard(1, CGBOARD_TYPE_HANGPLT);
	/* banks 5 and 6 select the texture ROM half of each board */
	set_cgboard_texture_bank(5);
	memory_set_bankptr(5, memory_region(REGION_USER5));
	memory_set_bankptr(6, memory_region(REGION_USER5));
	sharc_dataram = auto_malloc(0x100000 * sizeof(UINT32));
	sharc_dataram1 = auto_malloc(0x100000 * sizeof(UINT32));
	adc1038_init(adc1038_input_callback, 0);
}

static DRIVER_INIT( hangplt )
{
	UINT32 *rom = (UINT32 *)memory_region(REGION_USER1);

	hangplt_init_common();
	/* lwz r0,0x00a0(r31) with r31 = 0x8000013c */
	hangplt_install_idle_skip(0xffe0c130, 0xffe024ac, 0x8000013c);

	rom[(0x153ac^4) / 4] = 0x4e800020;
	rom[(0x15428^4) / 4] = 0x4e800020;
}

static DRIVER_INIT( hangpltu )
{
	UINT32 *rom = (UINT32 *)memory_region(REGION_USER1);

	hangplt_init_common();
	/* lwz r0,0x00a0(r31) with r31 = 0x80000014 */
	hangplt_install_idle_skip(0xffe0c2a4, 0xffe0241c, 0x80000014);

	rom[(0x153d0^4) / 4] = 0x4e800020;
	rom[(0x15428^4) / 4] = 0x4e800020;
}

/*************************************************************************/

GAME( 1996, gticlub,    0,        gticlub,  gticlub,  gticlub,  ROT0, "Konami", "GTI Club: Rally Cote D'Azur (ver EAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, gticlubu,   gticlub,  gticlub,  gticlub,  gticlub,  ROT0, "Konami", "GTI Club: Rally Cote D'Azur (ver UAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, gticluba,   gticlub,  gticlub,  gticlub,  gticlub,  ROT0, "Konami", "GTI Club: Rally Cote D'Azur (ver AAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1996, gticlubj,   gticlub,  gticlub,  gticlub,  gticlub,  ROT0, "Konami", "GTI Club: Rally Cote D'Azur (ver JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, thunderh,   0,        thunderh, thunderh, thunderh, ROT0, "Konami", "Operation Thunder Hurricane (ver EAA)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, thunderhu,  thunderh, thunderh, thunderh, thunderh, ROT0, "Konami", "Operation Thunder Hurricane (ver UAA)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, slrasslt,   0,        gticlub,  slrasslt, slrasslt, ROT0, "Konami", "Solar Assault (ver UAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, slrassltj,  slrasslt, gticlub,  slrasslt, slrasslt, ROT0, "Konami", "Solar Assault Revised (ver JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, slrassltj1, slrasslt, gticlub,  slrasslt, slrassltj1, ROT0, "Konami", "Solar Assault (ver JAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, hangplt,    0,        hangplt,  hangplt,  hangplt,  ROT0, "Konami", "Hang Pilot (ver JAB)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1997, hangpltu,   hangplt,  hangplt,  hangplt,  hangpltu, ROT0, "Konami", "Hang Pilot (ver UAA)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
