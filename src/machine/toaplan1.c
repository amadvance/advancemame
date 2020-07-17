/***************************************************************************
                ToaPlan game hardware from 1988-1991
                ------------------------------------
 ***************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms32010/tms32010.h"
#include "includes/toaplan1.h"


#define CLEAR 0
#define ASSERT 1


static int toaplan1_coin_count; /* coin count increments on startup ? , so dont count it */
static int toaplan1_intenable;
static int demonwld_dsp_on;
static int demonwld_dsp_BIO;
static int dsp_execute;							/* Demon world */
static unsigned int dsp_addr_w, main_ram_seg;	/* Demon world */


int toaplan1_unk_reset_port;

UINT8 *toaplan1_sharedram;



INTERRUPT_GEN( toaplan1_interrupt )
{
	if (toaplan1_intenable)
		cpunum_set_input_line(0, 4, HOLD_LINE);
}

WRITE16_HANDLER( toaplan1_intenable_w )
{
	if (ACCESSING_LSB)
	{
		toaplan1_intenable = data & 0xff;
	}
}


WRITE16_HANDLER( demonwld_dsp_addrsel_w )
{
	/* This sets the main CPU RAM address the DSP should */
	/*  read/write, via the DSP IO port 0 */
	/* Top three bits of data need to be shifted left 9 places */
	/*  to select which memory bank from main CPU address */
	/*  space to use */
	/* Lower thirteen bits of this data is shifted left one position */
	/*  to move it to an even address word boundary */

	main_ram_seg = ((data & 0xe000) << 9);
	dsp_addr_w   = ((data & 0x1fff) << 1);
	logerror("DSP PC:%04x IO write %04x (%08x) at port 0\n",activecpu_get_previouspc(),data,main_ram_seg + dsp_addr_w);
}

READ16_HANDLER( demonwld_dsp_r )
{
	/* DSP can read data from main CPU RAM via DSP IO port 1 */

	UINT16 input_data = 0;
	switch (main_ram_seg) {
		case 0xc00000:	cpuintrf_push_context(0); input_data = program_read_word(main_ram_seg + dsp_addr_w); cpuintrf_pop_context(); break;
		default:		logerror("DSP PC:%04x Warning !!! IO reading from %08x (port 1)\n",activecpu_get_previouspc(),main_ram_seg + dsp_addr_w);
	}
	logerror("DSP PC:%04x IO read %04x at %08x (port 1)\n",activecpu_get_previouspc(),input_data,main_ram_seg + dsp_addr_w);
	return input_data;
}

WRITE16_HANDLER( demonwld_dsp_w )
{
	/* Data written to main CPU RAM via DSP IO port 1 */
	dsp_execute = 0;
	switch (main_ram_seg) {
		case 0xc00000:	if ((dsp_addr_w < 3) && (data == 0)) dsp_execute = 1;
						cpuintrf_push_context(0);
						program_write_word(main_ram_seg + dsp_addr_w, data);
						cpuintrf_pop_context(); break;
		default:		logerror("DSP PC:%04x Warning !!! IO writing to %08x (port 1)\n",activecpu_get_previouspc(),main_ram_seg + dsp_addr_w);
	}
	logerror("DSP PC:%04x IO write %04x at %08x (port 1)\n",activecpu_get_previouspc(),data,main_ram_seg + dsp_addr_w);
}

WRITE16_HANDLER( demonwld_dsp_bio_w )
{
	/* data 0xffff  means inhibit BIO line to DSP and enable */
	/*              communication to main processor */
	/*              Actually only DSP data bit 15 controls this */
	/* data 0x0000  means set DSP BIO line active and disable */
	/*              communication to main processor*/
	logerror("DSP PC:%04x IO write %04x at port 3\n",activecpu_get_previouspc(),data);
	if (data & 0x8000) {
		demonwld_dsp_BIO = CLEAR_LINE;
	}
	if (data == 0) {
		if (dsp_execute) {
			logerror("Turning 68000 on\n");
			cpunum_set_input_line(0, INPUT_LINE_HALT, CLEAR_LINE);
			dsp_execute = 0;
		}
		demonwld_dsp_BIO = ASSERT_LINE;
	}
}

READ16_HANDLER ( demonwld_BIO_r )
{
	return demonwld_dsp_BIO;
}


static void demonwld_dsp(int enable)
{
	demonwld_dsp_on = enable;
	if (enable)
	{
		logerror("Turning DSP on and 68000 off\n");
		cpunum_set_input_line(2, INPUT_LINE_HALT, CLEAR_LINE);
		cpunum_set_input_line(2, 0, ASSERT_LINE); /* TMS32010 INT */
		cpunum_set_input_line(0, INPUT_LINE_HALT, ASSERT_LINE);
	}
	else
	{
		logerror("Turning DSP off\n");
		cpunum_set_input_line(2, 0, CLEAR_LINE); /* TMS32010 INT */
		cpunum_set_input_line(2, INPUT_LINE_HALT, ASSERT_LINE);
	}
}
static void demonwld_restore_dsp(void)
{
	demonwld_dsp(demonwld_dsp_on);
}

WRITE16_HANDLER( demonwld_dsp_ctrl_w )
{
#if 0
	logerror("68000:%08x  Writing %08x to %08x.\n",activecpu_get_pc() ,data ,0xe0000a + offset);
#endif

	if (ACCESSING_LSB)
	{
		switch (data)
		{
			case 0x00:	demonwld_dsp(1); break;	/* Enable the INT line to the DSP */
			case 0x01:	demonwld_dsp(0); break;	/* Inhibit the INT line to the DSP */
			default:	logerror("68000:%04x  Writing unknown command %08x to %08x\n",activecpu_get_previouspc() ,data ,0xe0000a + offset); break;
		}
	}
	else
	{
		logerror("68000:%04x  Writing unknown command %08x to %08x\n",activecpu_get_previouspc() ,data ,0xe0000a + offset);
	}
}


READ16_HANDLER( samesame_port_6_word_r )
{
	/* Bit 0x80 is secondary CPU (HD647180) ready signal */
	logerror("PC:%04x Warning !!! IO reading from $14000a\n",activecpu_get_previouspc());
	return (0x80 | input_port_6_word_r(0,0)) & 0xff;
}


READ16_HANDLER( toaplan1_shared_r )
{
	return toaplan1_sharedram[offset] & 0xff;
}

WRITE16_HANDLER( toaplan1_shared_w )
{
	if (ACCESSING_LSB)
	{
		toaplan1_sharedram[offset] = data & 0xff;
	}
}


WRITE16_HANDLER( toaplan1_reset_sound )
{
	/* Reset the secondary CPU and sound chip during soft resets */

	if (ACCESSING_LSB && (data == 0))
	{
		logerror("PC:%04x  Resetting Sound CPU and Sound chip (%08x)\n",activecpu_get_previouspc(),data);
		if (Machine->drv->sound[0].sound_type == SOUND_YM3812)
			sndti_reset(SOUND_YM3812, 0);
		if (Machine->drv->cpu[1].cpu_type == CPU_Z80)
			cpunum_set_input_line(1, INPUT_LINE_RESET, PULSE_LINE);
	}
}


WRITE8_HANDLER( rallybik_coin_w )
{
	switch (data) {
		case 0x08: if (toaplan1_coin_count) { coin_counter_w(0,1); coin_counter_w(0,0); } break;
		case 0x09: if (toaplan1_coin_count) { coin_counter_w(2,1); coin_counter_w(2,0); } break;
		case 0x0a: if (toaplan1_coin_count) { coin_counter_w(1,1); coin_counter_w(1,0); } break;
		case 0x0b: if (toaplan1_coin_count) { coin_counter_w(3,1); coin_counter_w(3,0); } break;
		case 0x0c: coin_lockout_w(0,1); coin_lockout_w(2,1); break;
		case 0x0d: coin_lockout_w(0,0); coin_lockout_w(2,0); break;
		case 0x0e: coin_lockout_w(1,1); coin_lockout_w(3,1); break;
		case 0x0f: coin_lockout_w(1,0); coin_lockout_w(3,0); toaplan1_coin_count=1; break;
		default:   logerror("PC:%04x  Writing unknown data (%04x) to coin count/lockout port\n",activecpu_get_previouspc(),data); break;
	}
}

WRITE8_HANDLER( toaplan1_coin_w )
{
	logerror("Z80 writing %02x to coin control\n",data);
	/* This still isnt too clear yet. */
	/* Coin C has no coin lock ? */
	/* Are some outputs for lights ? (no space on JAMMA for it though) */

	switch (data) {
		case 0xee: coin_counter_w(1,1); coin_counter_w(1,0); break; /* Count slot B */
		case 0xed: coin_counter_w(0,1); coin_counter_w(0,0); break; /* Count slot A */
	/* The following are coin counts after coin-lock active (faulty coin-lock ?) */
		case 0xe2: coin_counter_w(1,1); coin_counter_w(1,0); coin_lockout_w(1,1); break;
		case 0xe1: coin_counter_w(0,1); coin_counter_w(0,0); coin_lockout_w(0,1); break;

		case 0xec: coin_lockout_global_w(0); break;	/* ??? count games played */
		case 0xe8: break;	/* ??? Maximum credits reached with coin/credit ratio */
		case 0xe4: break;	/* ??? Reset coin system */

		case 0x0c: coin_lockout_global_w(0); break;	/* Unlock all coin slots */
		case 0x08: coin_lockout_w(2,0); break;	/* Unlock coin slot C */
		case 0x09: coin_lockout_w(0,0); break;	/* Unlock coin slot A */
		case 0x0a: coin_lockout_w(1,0); break;	/* Unlock coin slot B */

		case 0x02: coin_lockout_w(1,1); break;	/* Lock coin slot B */
		case 0x01: coin_lockout_w(0,1); break;	/* Lock coin slot A */
		case 0x00: coin_lockout_global_w(1); break;	/* Lock all coin slots */
		default:   logerror("PC:%04x  Writing unknown data (%04x) to coin count/lockout port\n",activecpu_get_previouspc(),data); break;
	}
}

WRITE16_HANDLER( samesame_coin_w )
{
	if (ACCESSING_LSB)
	{
		toaplan1_coin_w(offset, data & 0xff);
	}
	if (ACCESSING_MSB && (data&0xff00))
	{
		logerror("PC:%04x  Writing unknown MSB data (%04x) to coin count/lockout port\n",activecpu_get_previouspc(),data);
	}
}


MACHINE_RESET( toaplan1 )
{
	toaplan1_intenable = 0;
	toaplan1_coin_count = 0;
	toaplan1_unk_reset_port = 0;
	coin_lockout_global_w(0);
}
void toaplan1_driver_savestate(void)
{
	state_save_register_global(toaplan1_intenable);
	state_save_register_global(toaplan1_coin_count);
	state_save_register_global(toaplan1_unk_reset_port);
}

MACHINE_RESET( zerozone )	/* Hack for ZeroWing and OutZone. See the video driver */
{
	machine_reset_toaplan1();
	toaplan1_unk_reset_port = 1;
}

MACHINE_RESET( demonwld )
{
	machine_reset_toaplan1();
	dsp_addr_w = 0;
	main_ram_seg = 0;
	dsp_execute = 0;
}
void demonwld_driver_savestate(void)
{
	state_save_register_global(demonwld_dsp_on);
	state_save_register_global(dsp_addr_w);
	state_save_register_global(main_ram_seg);
	state_save_register_global(demonwld_dsp_BIO);
	state_save_register_global(dsp_execute);
	state_save_register_func_postload(demonwld_restore_dsp);
}

MACHINE_RESET( vimana )
{
	machine_reset_toaplan1();
}

