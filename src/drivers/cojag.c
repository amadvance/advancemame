/***************************************************************************

    Atari CoJag hardware

    driver by Aaron Giles

    Games supported:
        * Area 51 (3 Sets)
        * Maximum Force (2 Sets)
        * Area 51/Maximum Force Duo (2 Sets)
        * Vicious Circle

    In the future:
        * Fishin' Frenzy
        * Freeze

    To do:
        * map out unused RAM per-game via MRA8_NOP/MWA8_NOP

    Note: There is believed to be a 68020 version of Maximum Force
            (not confirmed or dumped)

****************************************************************************

    Area51/Maximum Force (c)1997 Atari Games
    Maximum Force
    A055451

    Components:
    sdt79r3041-20j
    Atari Jaguar CPU V1.0 6sc880hf106
    Atari Jaguar DSP V1.0 sc414201ft (has Motorolla logo)
    Altera epm7128elc84-15 marked A-21652
    VIA vt83c461 IDE controller
    Actel a1010b marked A-22096 near IDE and gun inputs
    Dallas ds1232s watchdog
    52MHz osc near Altera PLCC
    40MHz osc near 79R3041
    14.318180MHz osc near Jag DSP
    12x hm514260cj7 RAM (near Jaguar CPU/DSP)
    4x  sdt71256 RAM (near Boot ROMs's)
    Atmel atf16v8b marked a-21647 (near Jag CPU)
    Altera ep22lc-10 marked A-21648 (near Jag DSP)
    ICT 22cv10aj marked A-21649 (near Jag CPU)
    ICT 22cv10aj marked A-21650 (near Jag CPU)
    ICT 22cv10aj marked A-21651 (near Jag CPU)
    tea6320t
    AKM ak4310vm
    tda1554q amplifier
    Microchip 28c16a-15 BRAM

    ROM's:
    27c4001
    R3K MAX/A51 KIT
    LL
    (c)1997 Atari
    V 1.0

    27c4001
    R3K MAX/A51 KIT
    LH
    (c)1997 Atari
    V 1.0

    27c4001
    R3K MAX/A51 KIT
    HL
    (c)1997 Atari
    V 1.0

    27c4001
    R3K MAX/A51 KIT
    HH
    (c)1997 Atari
    V 1.0

    Jumpers:
    jsp1 (1/2 connected= w/sub 2/3 connected= normal speaker)
    jsp2 (1/2 connected= w/sub 2/3 connected= normal speaker)
    jamaud (1/2 connected=stereo 2/3 connected=mono)
    jimpr (1/2 connected=hi video R impedance 2/3 connected=lo)
    jimpg (1/2 connected=hi video G impedance 2/3 connected=lo)
    jimpb (1/2 connected=hi video B impedance 2/3 connected=lo)

    Connectors:
    idea  standard IDE connector
    ideb laptop size IDE connector
    jgun1 8 pin gun input
    jgun2 8 pin gun input
    xtracoin 1 6 pin (coin3/4 bills?)
    jvupdn 3 pin (?)
    jsync 3 pin (?)
    JAMMA
    jspkr left/right/subwoofer output
    hdpower 4 pin PC power connector for HD

****************************************************************************

    Memory map (TBA)

    ========================================================================
    MAIN CPU
    ========================================================================

    ------------------------------------------------------------
    000000-3FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 0
    400000-7FFFFF   R/W   xxxxxxxx xxxxxxxx   DRAM 1
    F00000-F000FF   R/W   xxxxxxxx xxxxxxxx   Tom Internal Registers
    F00400-F005FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table A
    F00600-F007FF   R/W   xxxxxxxx xxxxxxxx   CLUT - color lookup table B
    F00800-F00D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer A
    F01000-F0159F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer B
    F01800-F01D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - line buffer currently selected
    F02000-F021FF   R/W   xxxxxxxx xxxxxxxx   GPU control registers
    F02200-F022FF   R/W   xxxxxxxx xxxxxxxx   Blitter registers
    F03000-F03FFF   R/W   xxxxxxxx xxxxxxxx   Local GPU RAM
    F08800-F08D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer A
    F09000-F0959F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer B
    F09800-F09D9F   R/W   xxxxxxxx xxxxxxxx   LBUF - 32-bit access to line buffer currently selected
    F0B000-F0BFFF   R/W   xxxxxxxx xxxxxxxx   32-bit access to local GPU RAM
    F10000-F13FFF   R/W   xxxxxxxx xxxxxxxx   Jerry
    F14000-F17FFF   R/W   xxxxxxxx xxxxxxxx   Joysticks and GPIO0-5
    F18000-F1AFFF   R/W   xxxxxxxx xxxxxxxx   Jerry DSP
    F1B000-F1CFFF   R/W   xxxxxxxx xxxxxxxx   Local DSP RAM
    F1D000-F1DFFF   R     xxxxxxxx xxxxxxxx   Wavetable ROM
    ------------------------------------------------------------

***************************************************************************/


#include "driver.h"
#include "cpu/mips/r3000.h"
#include "cpu/jaguar/jaguar.h"
#include "machine/idectrl.h"
#include "sound/dac.h"
#include "jaguar.h"



/*************************************
 *
 *  Global variables
 *
 *************************************/

UINT32 *jaguar_shared_ram;
UINT32 *jaguar_gpu_ram;
UINT32 *jaguar_gpu_clut;
UINT32 *jaguar_dsp_ram;
UINT32 *jaguar_wave_rom;
UINT8 cojag_is_r3000;



/*************************************
 *
 *  Local variables
 *
 *************************************/

static UINT32 misc_control_data;
static UINT8 eeprom_enable;

static UINT32 *rom_base;

static struct ide_interface ide_intf =
{
	jaguar_external_int
};



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_RESET( cojag )
{
	/* 68020 only: copy the interrupt vectors into RAM */
	if (!cojag_is_r3000)
		memcpy(jaguar_shared_ram, rom_base, 0x10);

	/* clear any spinuntil stuff */
	jaguar_gpu_resume();
	jaguar_dsp_resume();

	/* halt the CPUs */
	jaguargpu_ctrl_w(1, G_CTRL, 0, 0);
	jaguardsp_ctrl_w(2, D_CTRL, 0, 0);

	/* init the sound system */
	cojag_sound_reset();

	/* reset the IDE controller */
	ide_controller_reset(0);
}



/*************************************
 *
 *  Misc. control bits
 *
 *************************************/

static READ32_HANDLER( misc_control_r )
{
	/*  D7    = board reset (low)
        D6    = audio must & reset (high)
        D5    = volume control data (invert on write)
        D4    = volume control clock
        D0    = shared memory select (0=XBUS) */

	return misc_control_data ^ 0x20;
}


static WRITE32_HANDLER( misc_control_w )
{
	logerror("%08X:misc_control_w(%02X)\n", activecpu_get_previouspc(), data);

	/*  D7    = board reset (low)
        D6    = audio must & reset (high)
        D5    = volume control data (invert on write)
        D4    = volume control clock
        D0    = shared memory select (0=XBUS) */

	/* handle resetting the DSPs */
	if (!(data & 0x80))
	{
		/* clear any spinuntil stuff */
		jaguar_gpu_resume();
		jaguar_dsp_resume();

		/* halt the CPUs */
		jaguargpu_ctrl_w(1, G_CTRL, 0, 0);
		jaguardsp_ctrl_w(2, D_CTRL, 0, 0);
	}

	COMBINE_DATA(&misc_control_data);
}



/*************************************
 *
 *  32-bit access to the GPU
 *
 *************************************/

static READ32_HANDLER( gpuctrl_r )
{
	return jaguargpu_ctrl_r(1, offset);
}


static WRITE32_HANDLER( gpuctrl_w )
{
	jaguargpu_ctrl_w(1, offset, data, mem_mask);
}



/*************************************
 *
 *  32-bit access to the DSP
 *
 *************************************/

static READ32_HANDLER( dspctrl_r )
{
	return jaguardsp_ctrl_r(2, offset);
}


static WRITE32_HANDLER( dspctrl_w )
{
	jaguardsp_ctrl_w(2, offset, data, mem_mask);
}



/*************************************
 *
 *  Input ports
 *
 *************************************/

static READ32_HANDLER( jamma_r )
{
	return readinputport(0) | (readinputport(1) << 16);
}


static READ32_HANDLER( status_r )
{
	// D23-20 = /SER-4-1
	// D19-16 = COINR4-1
	// D7     = /VSYNCNEQ
	// D6     = /S-TEST
	// D5     = /VOLUMEUP
	// D4     = /VOLUMEDOWN
	// D3-D0  = ACTC4-1
	return readinputport(2) | (readinputport(2) << 16);
}



/*************************************
 *
 *  Output ports
 *
 *************************************/

static WRITE32_HANDLER( latch_w )
{
	logerror("%08X:latch_w(%X)\n", activecpu_get_previouspc(), data);
}



/*************************************
 *
 *  EEPROM access
 *
 *************************************/

static READ32_HANDLER( eeprom_data_r )
{
	if (cojag_is_r3000)
		return generic_nvram32[offset] | 0xffffff00;
	else
		return generic_nvram32[offset] | 0x00ffffff;
}


static WRITE32_HANDLER( eeprom_enable_w )
{
	eeprom_enable = 1;
}


static WRITE32_HANDLER( eeprom_data_w )
{
//  if (eeprom_enable)
	{
		if (cojag_is_r3000)
			generic_nvram32[offset] = data & 0x000000ff;
		else
			generic_nvram32[offset] = data & 0xff000000;
	}
//  else
//      logerror("%08X:error writing to disabled EEPROM\n", activecpu_get_previouspc());
	eeprom_enable = 0;
}



/*************************************
 *
 *  GPU synchronization & speedup
 *
 *************************************/

/*
    Explanation:

    The GPU generally sits in a tight loop waiting for the main CPU to store
    a jump address into a specific memory location. This speedup is designed
    to catch that loop, which looks like this:

        load    (r28),r21
        jump    (r21)
        nop

    When nothing is pending, the GPU keeps the address of the load instruction
    at (r28) so that it loops back on itself. When the main CPU wants to execute
    a command, it stores an alternate address to (r28).

    Even if we don't optimize this case, we do need to detect when a command
    is written to the GPU in order to improve synchronization until the GPU
    has finished. To do this, we start a temporary high frequency timer and
    run it until we get back to the spin loop.
*/

static UINT32 *gpu_jump_address;
static UINT8 gpu_command_pending;
static UINT32 gpu_spin_pc;

static void gpu_sync_timer(int param)
{
	/* if a command is still pending, and we haven't maxed out our timer, set a new one */
	if (gpu_command_pending && param < 1000)
		timer_set(TIME_IN_USEC(50), ++param, gpu_sync_timer);
}


static WRITE32_HANDLER( gpu_jump_w )
{
	/* update the data in memory */
	COMBINE_DATA(gpu_jump_address);
	logerror("%08X:GPU jump address = %08X\n", activecpu_get_previouspc(), *gpu_jump_address);

	/* if the GPU is suspended, release it now */
	jaguar_gpu_resume();

	/* start the sync timer going, and note that there is a command pending */
	timer_set(TIME_NOW, 0, gpu_sync_timer);
	gpu_command_pending = 1;
}


static READ32_HANDLER( gpu_jump_r )
{
	/* if the current GPU command is just pointing back to the spin loop, and */
	/* we're reading it from the spin loop, we can optimize */
	if (*gpu_jump_address == gpu_spin_pc && activecpu_get_previouspc() == gpu_spin_pc)
	{
#if ENABLE_SPEEDUP_HACKS
		/* spin if we're allowed */
		jaguar_gpu_suspend();
#endif

		/* no command is pending */
		gpu_command_pending = 0;
	}

	/* return the current value */
	return *gpu_jump_address;
}



/*************************************
 *
 *  Main CPU speedup (R3000 games)
 *
 *************************************/

/*
    Explanation:

    Instead of sitting in a tight loop, the CPU will run the random number
    generator over and over while waiting for an interrupt. In order to catch
    that, we snoop the memory location it is polling, and see if it is read
    at least 5 times in a row, each time less than 200 cycles apart. If so,
    we assume it is spinning. Also, by waiting for 5 iterations, we let it
    crank through some random numbers, just not several thousand every frame.
*/

#if ENABLE_SPEEDUP_HACKS

static UINT32 *main_speedup;
static int main_speedup_hits;
static UINT32 main_speedup_last_cycles;
static UINT32 main_speedup_max_cycles;

static READ32_HANDLER( cojagr3k_main_speedup_r )
{
	UINT32 curcycles = activecpu_gettotalcycles();

	/* if it's been less than main_speedup_max_cycles cycles since the last time */
	if (curcycles - main_speedup_last_cycles < main_speedup_max_cycles)
	{
		/* increment the count; if we hit 5, we can spin until an interrupt comes */
		if (main_speedup_hits++ > 5)
		{
			cpu_spinuntil_int();
			main_speedup_hits = 0;
		}
	}

	/* if it's been more than main_speedup_max_cycles cycles, reset our count */
	else
		main_speedup_hits = 0;

	/* remember the last cycle count */
	main_speedup_last_cycles = curcycles;

	/* return the real value */
	return *main_speedup;
}

#endif



/*************************************
 *
 *  Main CPU speedup (Area 51)
 *
 *************************************/

/*
    Explanation:

    Very similar to the R3000 code, except we need to verify that the value in
    *main_speedup is actually 0.
*/

#if ENABLE_SPEEDUP_HACKS

static WRITE32_HANDLER( area51_main_speedup_w )
{
	UINT32 curcycles = activecpu_gettotalcycles();

	/* store the data */
	COMBINE_DATA(main_speedup);

	/* if it's been less than 400 cycles since the last time */
	if (*main_speedup == 0 && curcycles - main_speedup_last_cycles < 400)
	{
		/* increment the count; if we hit 5, we can spin until an interrupt comes */
		if (main_speedup_hits++ > 5)
		{
			cpu_spinuntil_int();
			main_speedup_hits = 0;
		}
	}

	/* if it's been more than 400 cycles, reset our count */
	else
		main_speedup_hits = 0;

	/* remember the last cycle count */
	main_speedup_last_cycles = curcycles;
}


/*
    Explanation:

    The Area 51/Maximum Force duo writes to a non-aligned address, so our check
    against 0 must handle that explicitly.
*/

static WRITE32_HANDLER( area51mx_main_speedup_w )
{
	UINT32 curcycles = activecpu_gettotalcycles();

	/* store the data */
	COMBINE_DATA(&main_speedup[offset]);

	/* if it's been less than 450 cycles since the last time */
	if (((main_speedup[0] << 16) | (main_speedup[1] >> 16)) == 0 && curcycles - main_speedup_last_cycles < 450)
	{
		/* increment the count; if we hit 5, we can spin until an interrupt comes */
		if (main_speedup_hits++ > 10)
		{
			cpu_spinuntil_int();
			main_speedup_hits = 0;
		}
	}

	/* if it's been more than 450 cycles, reset our count */
	else
		main_speedup_hits = 0;

	/* remember the last cycle count */
	main_speedup_last_cycles = curcycles;
}

#endif



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( r3000_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x04000000, 0x047fffff) AM_RAM AM_BASE(&jaguar_shared_ram) AM_SHARE(1)
	AM_RANGE(0x04e00000, 0x04e003ff) AM_READWRITE(ide_controller32_0_r, ide_controller32_0_w)
	AM_RANGE(0x04f00000, 0x04f003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0x04f00400, 0x04f007ff) AM_RAM AM_BASE(&jaguar_gpu_clut) AM_SHARE(2)
	AM_RANGE(0x04f02100, 0x04f021ff) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0x04f02200, 0x04f022ff) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0x04f03000, 0x04f03fff) AM_MIRROR(0x00008000) AM_RAM AM_BASE(&jaguar_gpu_ram) AM_SHARE(3)
	AM_RANGE(0x04f10000, 0x04f103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0x04f16000, 0x04f1600b) AM_READ(cojag_gun_input_r)	// GPI02
	AM_RANGE(0x04f17000, 0x04f17003) AM_READ(status_r)			// GPI03
	AM_RANGE(0x04f17800, 0x04f17803) AM_WRITE(latch_w)	// GPI04
	AM_RANGE(0x04f17c00, 0x04f17c03) AM_READ(jamma_r)			// GPI05
	AM_RANGE(0x04f1a100, 0x04f1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0x04f1a140, 0x04f1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0x04f1b000, 0x04f1cfff) AM_RAM AM_BASE(&jaguar_dsp_ram) AM_SHARE(4)

	AM_RANGE(0x06000000, 0x06000003) AM_READWRITE(misc_control_r, misc_control_w)
	AM_RANGE(0x10000000, 0x1007ffff) AM_RAM
	AM_RANGE(0x12000000, 0x120fffff) AM_RAM		// tested in self-test only?
	AM_RANGE(0x14000004, 0x14000007) AM_WRITE(watchdog_reset32_w)
	AM_RANGE(0x16000000, 0x16000003) AM_WRITE(eeprom_enable_w)
	AM_RANGE(0x18000000, 0x18001fff) AM_READWRITE(eeprom_data_r, eeprom_data_w) AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x1fc00000, 0x1fdfffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_BASE(&rom_base)
ADDRESS_MAP_END


static ADDRESS_MAP_START( m68020_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x7fffff) AM_RAM AM_BASE(&jaguar_shared_ram) AM_SHARE(1)
	AM_RANGE(0x800000, 0x9fffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_BASE(&rom_base)
	AM_RANGE(0xa00000, 0xa1ffff) AM_RAM
	AM_RANGE(0xa20000, 0xa21fff) AM_READWRITE(eeprom_data_r, eeprom_data_w) AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xa30000, 0xa30003) AM_WRITE(watchdog_reset32_w)
	AM_RANGE(0xa40000, 0xa40003) AM_WRITE(eeprom_enable_w)
	AM_RANGE(0xb70000, 0xb70003) AM_READWRITE(misc_control_r, misc_control_w)
	AM_RANGE(0xc00000, 0xdfffff) AM_READ(MRA32_BANK2)
	AM_RANGE(0xe00000, 0xe003ff) AM_READWRITE(ide_controller32_0_r, ide_controller32_0_w)
	AM_RANGE(0xf00000, 0xf003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0xf00400, 0xf007ff) AM_RAM AM_BASE(&jaguar_gpu_clut) AM_SHARE(2)
	AM_RANGE(0xf02100, 0xf021ff) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0xf02200, 0xf022ff) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0xf03000, 0xf03fff) AM_MIRROR(0x008000) AM_RAM AM_BASE(&jaguar_gpu_ram) AM_SHARE(3)
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0xf16000, 0xf1600b) AM_READ(cojag_gun_input_r)	// GPI02
	AM_RANGE(0xf17000, 0xf17003) AM_READ(status_r)			// GPI03
//  AM_RANGE(0xf17800, 0xf17803) AM_WRITE(latch_w)  // GPI04
	AM_RANGE(0xf17c00, 0xf17c03) AM_READ(jamma_r)			// GPI05
	AM_RANGE(0xf1a100, 0xf1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0xf1a140, 0xf1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0xf1b000, 0xf1cfff) AM_RAM AM_BASE(&jaguar_dsp_ram) AM_SHARE(4)
ADDRESS_MAP_END



/*************************************
 *
 *  GPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( gpu_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x7fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xe00000, 0xe003ff) AM_READWRITE(ide_controller32_0_r, ide_controller32_0_w)
	AM_RANGE(0xf00000, 0xf003ff) AM_READWRITE(jaguar_tom_regs32_r, jaguar_tom_regs32_w)
	AM_RANGE(0xf00400, 0xf007ff) AM_RAM AM_SHARE(2)
	AM_RANGE(0xf02100, 0xf021ff) AM_READWRITE(gpuctrl_r, gpuctrl_w)
	AM_RANGE(0xf02200, 0xf022ff) AM_READWRITE(jaguar_blitter_r, jaguar_blitter_w)
	AM_RANGE(0xf03000, 0xf03fff) AM_RAM AM_SHARE(3)
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
ADDRESS_MAP_END



/*************************************
 *
 *  DSP memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( dsp_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x7fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xf10000, 0xf103ff) AM_READWRITE(jaguar_jerry_regs32_r, jaguar_jerry_regs32_w)
	AM_RANGE(0xf1a100, 0xf1a13f) AM_READWRITE(dspctrl_r, dspctrl_w)
	AM_RANGE(0xf1a140, 0xf1a17f) AM_READWRITE(jaguar_serial_r, jaguar_serial_w)
	AM_RANGE(0xf1b000, 0xf1cfff) AM_RAM AM_SHARE(4)
	AM_RANGE(0xf1d000, 0xf1dfff) AM_ROM AM_BASE(&jaguar_wave_rom)
ADDRESS_MAP_END



/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( area51 )
	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfe00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )			// s-test
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SPECIAL )	// vsyncneq
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* gun triggers */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )	// gun data valid
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )	// gun data valid
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( vcircle )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )			// s-test
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SPECIAL )	// vsyncneq
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_HIGH, IPT_SPECIAL )	// coin returns
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static struct r3000_config config =
{
	0,		/* 1 if we have an FPU, 0 otherwise */
	4096,	/* code cache size */
	4096	/* data cache size */
};


static struct jaguar_config gpu_config =
{
	jaguar_gpu_cpu_int
};


static struct jaguar_config dsp_config =
{
	jaguar_dsp_cpu_int
};


MACHINE_DRIVER_START( cojagr3k )

	/* basic machine hardware */
	MDRV_CPU_ADD(R3000BE, 66000000/2)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(r3000_map,0)

	MDRV_CPU_ADD(JAGUARGPU, 52000000/2)
	MDRV_CPU_CONFIG(gpu_config)
	MDRV_CPU_PROGRAM_MAP(gpu_map,0)

	MDRV_CPU_ADD(JAGUARDSP, 52000000/2)
	MDRV_CPU_CONFIG(dsp_config)
	MDRV_CPU_PROGRAM_MAP(dsp_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cojag)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(65534)

	MDRV_VIDEO_START(cojag)
	MDRV_VIDEO_UPDATE(cojag)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( r3knarrow )
	MDRV_IMPORT_FROM(cojagr3k)

	/* video hardware */
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( cojag68k )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 50000000/2)
	MDRV_CPU_PROGRAM_MAP(m68020_map,0)

	MDRV_CPU_ADD(JAGUARGPU, 52000000/2)
	MDRV_CPU_CONFIG(gpu_config)
	MDRV_CPU_PROGRAM_MAP(gpu_map,0)

	MDRV_CPU_ADD(JAGUARDSP, 52000000/2)
	MDRV_CPU_CONFIG(dsp_config)
	MDRV_CPU_PROGRAM_MAP(dsp_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(cojag)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(65534)

	MDRV_VIDEO_START(cojag)
	MDRV_VIDEO_UPDATE(cojag)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definition(s)
 *
 *  Date Information comes from either
 *   ROM labels or from the Self-Test
 *   as "Main"
 *
 *************************************/

ROM_START( area51t ) /* 68020 based, Area51 Time Warner License  Date: Nov 15, 1995 */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for 68020 code */
	ROM_LOAD32_BYTE( "3h.bin", 0x00000, 0x80000, CRC(e70a97c4) SHA1(39dabf6bf3dc6f717a587f362d040bfb332be9e1) )
	ROM_LOAD32_BYTE( "3p.bin", 0x00001, 0x80000, CRC(e9c9f4bd) SHA1(7c6c50372d45dca8929767241b092339f3bab4d2) )
	ROM_LOAD32_BYTE( "3m.bin", 0x00002, 0x80000, CRC(6f135a81) SHA1(2d9660f240b14481e8c46bc98713e9dc12035063) )
	ROM_LOAD32_BYTE( "3k.bin", 0x00003, 0x80000, CRC(94f50c14) SHA1(a54552e3ac5c4f481ba4f2fc7d724534576fe76c) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "area51t", 0, MD5(60d051da941d76aafd47c862e3b6e209) SHA1(fee528eef8a256f87af299499ecf5817218f5202) )
ROM_END

ROM_START( area51a ) /* 68020 based, Area51 Atari Games License  Date: Oct 25, 1995 */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for 68020 code */
	ROM_LOAD32_BYTE( "3h", 0x00000, 0x80000, CRC(116d37e6) SHA1(5d36cae792dd349faa77cd2d8018722a28ee55c1) )
	ROM_LOAD32_BYTE( "3p", 0x00001, 0x80000, CRC(eb10f539) SHA1(dadc4be5a442dd4bd17385033056555e528ed994) )
	ROM_LOAD32_BYTE( "3m", 0x00002, 0x80000, CRC(c6d8322b) SHA1(90cf848a4195c51b505653cc2c74a3b9e3c851b8) )
	ROM_LOAD32_BYTE( "3k", 0x00003, 0x80000, CRC(729eb1b7) SHA1(21864b4281b1ad17b2903e3aa294e4be74161e80) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "area51", 0, MD5(130b330eff59403f8fc3433ff501852b) SHA1(9ea749404c9a5d44f407cdb8803293ec0d61410d) )
ROM_END

ROM_START( area51 ) /* R3000 based, labeled as "Area51 2-C"  Date: Nov 11 1996 */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for IDT 79R3041 code */
	ROM_LOAD32_BYTE( "a51_2-c.hh", 0x00000, 0x80000, CRC(13af6a1e) SHA1(69da54ed6886e825156bbcc256e8d7abd4dc1ff8) )
	ROM_LOAD32_BYTE( "a51_2-c.hl", 0x00001, 0x80000, CRC(8ab6649b) SHA1(9b4945bc04f8a73161638a2c5fa2fd84c6fd31b4) )
	ROM_LOAD32_BYTE( "a51_2-c.lh", 0x00002, 0x80000, CRC(a6524f73) SHA1(ae377a6803a4f7d1bbcc111725af121a3e82317d) )
	ROM_LOAD32_BYTE( "a51_2-c.ll", 0x00003, 0x80000, CRC(471b15d2) SHA1(4b5f45ee140b03a6be61475cae1c2dbef0f07457) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "area51", 0, MD5(130b330eff59403f8fc3433ff501852b) SHA1(9ea749404c9a5d44f407cdb8803293ec0d61410d) )
ROM_END

ROM_START( maxforce ) /* R3000 based, labeled as "Maximum Force 5-23-97 v1.05" */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for IDT 79R3041 code */
	ROM_LOAD32_BYTE( "maxf_105.hh", 0x00000, 0x80000, CRC(ec7f8167) SHA1(0cf057bfb1f30c2c9621d3ed25021e7ba7bdd46e) )
	ROM_LOAD32_BYTE( "maxf_105.hl", 0x00001, 0x80000, CRC(3172611c) SHA1(00f14f871b737c66c20f95743740d964d0be3f24) )
	ROM_LOAD32_BYTE( "maxf_105.lh", 0x00002, 0x80000, CRC(84d49423) SHA1(88d9a6724f1118f2bbef5dfa27accc2b65c5ba1d) )
	ROM_LOAD32_BYTE( "maxf_105.ll", 0x00003, 0x80000, CRC(16d0768d) SHA1(665a6d7602a7f2f5b1f332b0220b1533143d56b1) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "maxforce", 0, MD5(b0a214c7b3f8ba9d592396332fc974c9) SHA1(59d77280afdb2d1f801ee81786aa7d3166ec2695) )
ROM_END


ROM_START( maxf_102 ) /* R3000 based, labeled as "Maximum Force 2-27-97 v1.02" */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for IDT 79R3041 code */
	ROM_LOAD32_BYTE( "maxf_102.hh", 0x00000, 0x80000, CRC(8ff7009d) SHA1(da22eae298a6e0e36f503fa091ac3913423dcd0f) )
	ROM_LOAD32_BYTE( "maxf_102.hl", 0x00001, 0x80000, CRC(96c2cc1d) SHA1(b332b8c042b92c736131c478cefac1c3c2d2673b) )
	ROM_LOAD32_BYTE( "maxf_102.lh", 0x00002, 0x80000, CRC(459ffba5) SHA1(adb40db6904e84c17f32ac6518fd2e994da7883f) )
	ROM_LOAD32_BYTE( "maxf_102.ll", 0x00003, 0x80000, CRC(e491be7f) SHA1(cbe281c099a4aa87067752d68cf2bb0ab3900531) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "maxforce", 0, MD5(b0a214c7b3f8ba9d592396332fc974c9) SHA1(59d77280afdb2d1f801ee81786aa7d3166ec2695) )
ROM_END


ROM_START( area51mx )	/* 68020 based, Labeled as "68020 MAX/A51 KIT 2.0" Date: Apr 22, 1998 */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* 2MB for 68020 code */
	ROM_LOAD32_BYTE( "area51mx.3h", 0x00000, 0x80000, CRC(47cbf30b) SHA1(23377bcc65c0fc330d5bc7e76e233bae043ac364) )
	ROM_LOAD32_BYTE( "area51mx.3p", 0x00001, 0x80000, CRC(a3c93684) SHA1(f6b3357bb69900a176fd6bc6b819b2f57b7d0f59) )
	ROM_LOAD32_BYTE( "area51mx.3m", 0x00002, 0x80000, CRC(d800ac17) SHA1(3d515c8608d8101ee9227116175b3c3f1fe22e0c) )
	ROM_LOAD32_BYTE( "area51mx.3k", 0x00003, 0x80000, CRC(0e78f308) SHA1(adc4c8e441eb8fe525d0a6220eb3a2a8791a7289) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "area51mx", 0, MD5(fce1a0954759fa22e50747959716823d) SHA1(7e629045eb5baa8cd522273befffbf8520828938) )
ROM_END


ROM_START( a51mxr3k ) /* R3000 based, Labeled as "R3K Max/A51 Kit Ver 1.0" */
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for IDT 79R3041 code */
	ROM_LOAD32_BYTE( "a51mxr3k.hh", 0x00000, 0x80000, CRC(a984dab2) SHA1(debb3bc11ff49e87a52e89a69533a1bab7db700e) )
	ROM_LOAD32_BYTE( "a51mxr3k.hl", 0x00001, 0x80000, CRC(0af49d74) SHA1(c19f26056a823fd32293e9a7b3ea868640eabf49) )
	ROM_LOAD32_BYTE( "a51mxr3k.lh", 0x00002, 0x80000, CRC(d7d94dac) SHA1(2060a74715f36a0d7f5dd0855eda48ad1f20f095) )
	ROM_LOAD32_BYTE( "a51mxr3k.ll", 0x00003, 0x80000, CRC(ece9e5ae) SHA1(7e44402726f5afa6d1670b27aa43ad13d21c4ad9) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "area51mx", 0, MD5(fce1a0954759fa22e50747959716823d) SHA1(7e629045eb5baa8cd522273befffbf8520828938) )
ROM_END


ROM_START( vcircle )
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 )	/* 2MB for R3000 code */
	ROM_LOAD32_BYTE( "hh", 0x00000, 0x80000, CRC(7276f5f5) SHA1(716287e370a4f300b1743103f8031afc82de38ca) )
	ROM_LOAD32_BYTE( "hl", 0x00001, 0x80000, CRC(146060a1) SHA1(f291989f1f0ef228757f1990fb14da5ff8f3cf8d) )
	ROM_LOAD32_BYTE( "lh", 0x00002, 0x80000, CRC(be4b2ef6) SHA1(4332b3036e9cb12685e914d085d9a63aa856f0be) )
	ROM_LOAD32_BYTE( "ll", 0x00003, 0x80000, CRC(ba8753eb) SHA1(0322e0e37d814a38d08ba191b1a97fb1a55fe461) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "vcircle", 0, MD5(fc316bd92363573d60083514223c6816) SHA1(f1d3e3d081d10ec42d07cd695d52b44812264983) )
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

static void cojag_common_init(UINT8 crosshair, UINT16 gpu_jump_offs, UINT16 spin_pc)
{
	/* copy over the ROM */
	cojag_is_r3000 = (Machine->drv->cpu[0].cpu_type == CPU_R3000BE);
	cojag_draw_crosshair = crosshair;

	/* install synchronization hooks for GPU */
	if (cojag_is_r3000)
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x04f0b000 + gpu_jump_offs, 0x04f0b003 + gpu_jump_offs, 0, 0, gpu_jump_w);
	else
		memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xf0b000 + gpu_jump_offs, 0xf0b003 + gpu_jump_offs, 0, 0, gpu_jump_w);
	memory_install_read32_handler(1, ADDRESS_SPACE_PROGRAM, 0xf03000 + gpu_jump_offs, 0xf03003 + gpu_jump_offs, 0, 0, gpu_jump_r);
	gpu_jump_address = &jaguar_gpu_ram[gpu_jump_offs/4];
	gpu_spin_pc = 0xf03000 + spin_pc;

	/* init the sound system and install DSP speedups */
	cojag_sound_init();

	/* spin up the hard disk */
	ide_controller_init(0, &ide_intf);
}


static DRIVER_INIT( area51a )
{
	cojag_common_init(1, 0x5c4, 0x5a0);

#if ENABLE_SPEEDUP_HACKS
	/* install speedup for main CPU */
	main_speedup = memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xa02030, 0xa02033, 0, 0, area51_main_speedup_w);
#endif
}


static DRIVER_INIT( area51 )
{
	cojag_common_init(1, 0x0c0, 0x09e);

#if ENABLE_SPEEDUP_HACKS
	/* install speedup for main CPU */
	main_speedup_max_cycles = 120;
	main_speedup = memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x100062e8, 0x100062eb, 0, 0, cojagr3k_main_speedup_r);
#endif
}

static DRIVER_INIT( maxforce )
{
	cojag_common_init(1, 0x0c0, 0x09e);

	/* patch the protection */
	rom_base[0x220/4] = 0x03e00008;

#if ENABLE_SPEEDUP_HACKS
	/* install speedup for main CPU */
	main_speedup_max_cycles = 120;
	main_speedup = memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1000865c, 0x1000865f, 0, 0, cojagr3k_main_speedup_r);
#endif
}


static DRIVER_INIT( area51mx )
{
	cojag_common_init(1, 0x0c0, 0x09e);

	/* patch the protection */
	rom_base[0x418/4] = 0x4e754e75;

#if ENABLE_SPEEDUP_HACKS
	/* install speedup for main CPU */
	main_speedup = memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xa19550, 0xa19557, 0, 0, area51mx_main_speedup_w);
#endif
}


static DRIVER_INIT( a51mxr3k )
{
	cojag_common_init(1, 0x0c0, 0x09e);

	/* patch the protection */
	rom_base[0x220/4] = 0x03e00008;

#if ENABLE_SPEEDUP_HACKS
	/* install speedup for main CPU */
	main_speedup_max_cycles = 120;
	main_speedup = memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x10006f0c, 0x10006f0f, 0, 0, cojagr3k_main_speedup_r);
#endif
}


static DRIVER_INIT( vcircle )
{
	cojag_common_init(0, 0x5c0, 0x5a0);

#if ENABLE_SPEEDUP_HACKS
	/* install speedup for main CPU */
	main_speedup_max_cycles = 50;
	main_speedup = memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x12005b34, 0x12005b37, 0, 0, cojagr3k_main_speedup_r);
#endif
}



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1996, area51,   0,        r3knarrow, area51,   area51,   ROT0, "Atari Games", "Area 51 (R3000)", 0 )
GAME( 1995, area51t,  area51,   cojag68k,  area51,   area51a,  ROT0, "Time Warner", "Area 51 (Time Warner License)", 0 )
GAME( 1995, area51a,  area51,   cojag68k,  area51,   area51a,  ROT0, "Atari Games", "Area 51 (Atari Games License)", 0 )
GAME( 1996, maxforce, 0,        r3knarrow, area51,   maxforce, ROT0, "Atari Games", "Maximum Force v1.05", 0 )
GAME( 1996, maxf_102, maxforce, r3knarrow, area51,   maxforce, ROT0, "Atari Games", "Maximum Force v1.02", 0 )
GAME( 1998, area51mx, 0,        cojag68k,  area51,   area51mx, ROT0, "Atari Games", "Area 51 / Maximum Force Duo v2.0", 0 )
GAME( 1998, a51mxr3k, area51mx, r3knarrow, area51,   a51mxr3k, ROT0, "Atari Games", "Area 51 / Maximum Force Duo (R3000)", 0 )
GAME( 1996, vcircle,  0,        cojagr3k,  vcircle,  vcircle,  ROT0, "Atari Games", "Vicious Circle (prototype)", 0 )
