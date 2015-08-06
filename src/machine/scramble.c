/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/7474.h"
#include "machine/8255ppi.h"
#include "includes/galaxian.h"
#include "includes/cclimber.h"



static int irq_line;
static mame_timer *int_timer;

static UINT8 moonwar_port_select;
static UINT8 kingball_speech_dip;
static UINT8 kingball_sound;

static UINT8 cavelon_bank;
static UINT8 _4in1_bank;
static UINT8 gmgalax_selected_game;






static void galaxian_7474_9M_2_callback(void)
{
	/* Q bar clocks the other flip-flop,
       Q is VBLANK (not visible to the CPU) */
	TTL7474_clock_w(1, TTL7474_output_comp_r(0));
	TTL7474_update(1);
}

static void galaxian_7474_9M_1_callback(void)
{
	/* Q goes to the NMI line */
	cpunum_set_input_line(0, irq_line, TTL7474_output_r(1) ? CLEAR_LINE : ASSERT_LINE);
}

static const struct TTL7474_interface galaxian_7474_9M_2_intf =
{
	galaxian_7474_9M_2_callback
};

static const struct TTL7474_interface galaxian_7474_9M_1_intf =
{
	galaxian_7474_9M_1_callback
};


WRITE8_HANDLER( galaxian_nmi_enable_w )
{
	TTL7474_preset_w(1, data);
	TTL7474_update(1);
}


static void interrupt_timer(int param)
{
	/* 128V, 64V and 32V go to D */
	TTL7474_d_w(0, (param & 0xe0) != 0xe0);

	/* 16V clocks the flip-flop */
	TTL7474_clock_w(0, param & 0x10);

	param = (param + 0x10) & 0xff;

	timer_adjust(int_timer, cpu_getscanlinetime(param), param, 0);

	TTL7474_update(0);
}


static void machine_reset_common( int line )
{
	irq_line = line;

	/* initalize main CPU interrupt generator flip-flops */
	TTL7474_config(0, &galaxian_7474_9M_2_intf);
	TTL7474_preset_w(0, 1);
	TTL7474_clear_w (0, 1);

	TTL7474_config(1, &galaxian_7474_9M_1_intf);
	TTL7474_clear_w (1, 1);
	TTL7474_d_w     (1, 0);
	TTL7474_preset_w(1, 0);

	/* start a timer to generate interrupts */
	int_timer = timer_alloc(interrupt_timer);
	timer_adjust(int_timer, cpu_getscanlinetime(0), 0, 0);
}

MACHINE_RESET( galaxian )
{
	machine_reset_common(INPUT_LINE_NMI);
}

MACHINE_RESET( devilfsg )
{
	machine_reset_common(0);
}

MACHINE_RESET( scramble )
{
	machine_reset_galaxian();

	if (cpu_gettotalcpu() > 1)
	{
		scramble_sh_init();
	}
}

MACHINE_RESET( sfx )
{
	machine_reset_scramble();

	sfx_sh_init();
}

MACHINE_RESET( explorer )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	RAM[0x47ff] = 0; /* If not set, it doesn't reset after the 1st time */

	machine_reset_galaxian();
}

WRITE8_HANDLER( galaxian_coin_lockout_w )
{
	coin_lockout_global_w(~data & 1);
}


WRITE8_HANDLER( galaxian_coin_counter_w )
{
	coin_counter_w(offset, data & 0x01);
}

WRITE8_HANDLER( galaxian_coin_counter_1_w )
{
	coin_counter_w(1, data & 0x01);
}

WRITE8_HANDLER( galaxian_coin_counter_2_w )
{
	coin_counter_w(2, data & 0x01);
}


WRITE8_HANDLER( galaxian_leds_w )
{
	set_led_status(offset,data & 1);
}


static READ8_HANDLER( scrambls_input_port_2_r )
{
	UINT8 res;


	res = readinputport(2);

/*logerror("%04x: read IN2\n",activecpu_get_pc());*/

	/* avoid protection */
	if (activecpu_get_pc() == 0x00e4) res &= 0x7f;

	return res;
}

static READ8_HANDLER( ckongs_input_port_1_r )
{
	return (readinputport(1) & 0xfc) | ((readinputport(2) & 0x06) >> 1);
}

static READ8_HANDLER( ckongs_input_port_2_r )
{
	return (readinputport(2) & 0xf9) | ((readinputport(1) & 0x03) << 1);
}


static WRITE8_HANDLER( moonwar_port_select_w )
{
	moonwar_port_select = data & 0x10;
}

static READ8_HANDLER( moonwar_input_port_0_r )
{
	UINT8 sign;
	UINT8 delta;

	delta = (moonwar_port_select ? readinputport(3) : readinputport(4));

	sign = (delta & 0x80) >> 3;
	delta &= 0x0f;

	return ((readinputport(0) & 0xe0) | delta | sign );
}


/* the coinage DIPs are spread accross two input ports */
static READ8_HANDLER( stratgyx_input_port_2_r )
{
	return (readinputport(2) & ~0x06) | ((readinputport(4) << 1) & 0x06);
}

static READ8_HANDLER( stratgyx_input_port_3_r )
{
	return (readinputport(3) & ~0x03) | ((readinputport(4) >> 2) & 0x03);
}


static READ8_HANDLER( darkplnt_input_port_1_r )
{
	static const UINT8 remap[] = {0x03, 0x02, 0x00, 0x01, 0x21, 0x20, 0x22, 0x23,
							  0x33, 0x32, 0x30, 0x31, 0x11, 0x10, 0x12, 0x13,
							  0x17, 0x16, 0x14, 0x15, 0x35, 0x34, 0x36, 0x37,
							  0x3f, 0x3e, 0x3c, 0x3d, 0x1d, 0x1c, 0x1e, 0x1f,
							  0x1b, 0x1a, 0x18, 0x19, 0x39, 0x38, 0x3a, 0x3b,
							  0x2b, 0x2a, 0x28, 0x29, 0x09, 0x08, 0x0a, 0x0b,
							  0x0f, 0x0e, 0x0c, 0x0d, 0x2d, 0x2c, 0x2e, 0x2f,
							  0x27, 0x26, 0x24, 0x25, 0x05, 0x04, 0x06, 0x07 };
	UINT8 val;

	val = readinputport(1);

	return ((val & 0x03) | (remap[val >> 2] << 2));
}



static WRITE8_HANDLER( scramble_protection_w )
{
	/* nothing to do yet */
}

static READ8_HANDLER( scramble_protection_r )
{
	switch (activecpu_get_pc())
	{
	case 0x00a8: return 0xf0;
	case 0x00be: return 0xb0;
	case 0x0c1d: return 0xf0;
	case 0x0c6a: return 0xb0;
	case 0x0ceb: return 0x40;
	case 0x0d37: return 0x60;
	case 0x1ca2: return 0x00;  /* I don't think it's checked */
	case 0x1d7e: return 0xb0;
	default:
		logerror("%04x: read protection\n",activecpu_get_pc());
		return 0;
	}
}

static READ8_HANDLER( scrambls_protection_r )
{
	logerror("%04x: read protection\n",activecpu_get_pc());

	return 0x6f;
}


READ8_HANDLER( scramblb_protection_1_r )
{
	switch (activecpu_get_pc())
	{
	case 0x01da: return 0x80;
	case 0x01e4: return 0x00;
	default:
		logerror("%04x: read protection 1\n",activecpu_get_pc());
		return 0;
	}
}

READ8_HANDLER( scramblb_protection_2_r )
{
	switch (activecpu_get_pc())
	{
	case 0x01ca: return 0x90;
	default:
		logerror("%04x: read protection 2\n",activecpu_get_pc());
		return 0;
	}
}


READ8_HANDLER( jumpbug_protection_r )
{
	switch (offset)
	{
	case 0x0114:  return 0x4f;
	case 0x0118:  return 0xd3;
	case 0x0214:  return 0xcf;
	case 0x0235:  return 0x02;
	case 0x0311:  return 0x00;  /* not checked */
	default:
		logerror("Unknown protection read. Offset: %04X  PC=%04X\n",0xb000+offset,activecpu_get_pc());
	}

	return 0;
}


static WRITE8_HANDLER( theend_coin_counter_w )
{
	coin_counter_w(0, data & 0x80);
}


static READ8_HANDLER( mariner_protection_1_r )
{
	return 7;
}

static READ8_HANDLER( mariner_protection_2_r )
{
	return 3;
}


READ8_HANDLER( triplep_pip_r )
{
	logerror("PC %04x: triplep read port 2\n",activecpu_get_pc());
	if (activecpu_get_pc() == 0x015a) return 0xff;
	else if (activecpu_get_pc() == 0x0886) return 0x05;
	else return 0;
}

READ8_HANDLER( triplep_pap_r )
{
	logerror("PC %04x: triplep read port 3\n",activecpu_get_pc());
	if (activecpu_get_pc() == 0x015d) return 0x04;
	else return 0;
}


static READ8_HANDLER( checkmaj_protection_r )
{
	switch (activecpu_get_pc())
	{
	case 0x0f15:  return 0xf5;
	case 0x0f8f:  return 0x7c;
	case 0x10b3:  return 0x7c;
	case 0x10e0:  return 0x00;
	case 0x10f1:  return 0xaa;
	case 0x1402:  return 0xaa;
	default:
		logerror("Unknown protection read. PC=%04X\n",activecpu_get_pc());
	}

	return 0;
}


/* Zig Zag can swap ROMs 2 and 3 as a form of copy protection */
WRITE8_HANDLER( zigzag_sillyprotection_w )
{
	if (data)
	{
		/* swap ROM 2 and 3! */
		memory_set_bank(1, 1);
		memory_set_bank(2, 0);
	}
	else
	{
		memory_set_bank(1, 0);
		memory_set_bank(2, 1);
	}
}

DRIVER_INIT( zigzag )
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 2, &RAM[0x2000], 0x1000);
	memory_configure_bank(2, 0, 2, &RAM[0x2000], 0x1000);
	memory_set_bank(1, 0);
	memory_set_bank(2, 1);
}



static READ8_HANDLER( dingo_3000_r )
{
	return 0xaa;
}

static READ8_HANDLER( dingo_3035_r )
{
	return 0x8c;
}

static READ8_HANDLER( dingoe_3001_r )
{
	return 0xaa;
}


DRIVER_INIT( dingoe )
{
	offs_t i;
	UINT8 *rom = memory_region(REGION_CPU1);

	for (i = 0; i < 0x3000; i++)
	{
		UINT8 data_xor;

		/* XOR bit 2 with 4 and 5 with 0 */
		data_xor = BIT(rom[i], 2) << 4 | BIT(rom[i], 5) << 0;
		rom[i] ^= data_xor;


		/* Invert bit 1 */
		if (~rom[i] & 0x02)
			rom[i] = rom[i] | 0x02;
		else
			rom[i] = rom[i] & 0xfd;


		/* Swap bit0 with bit4 */
		if ((i & 0x0f) == 0x02 || (i & 0x0f) == 0x0a || (i & 0x0f) == 0x03 || (i & 0x0f) == 0x0b || (i & 0x0f) == 0x06 || (i & 0x0f) == 0x0e || (i & 0x0f) == 0x07 || (i & 0x0f) == 0x0f)	/* Swap Bit 0 and 4 */
			rom[i] = BITSWAP8(rom[i],7,6,5,0,3,2,1,4);
	}

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3001, 0x3001, 0, 0, dingoe_3001_r);	/* Protection check */

}



/* Hack? If $b003 is high, we'll check our "fake" speech dipswitch (marked as SLAM) */
static READ8_HANDLER( kingball_IN0_r )
{
	if (kingball_speech_dip)
		return (readinputport(0) & ~0x40) | ((readinputport(3) & 0x01) << 6);
	else
		return readinputport(0);
}

static READ8_HANDLER( kingball_IN1_r )
{
	/* bit 5 is the NOISE line from the sound circuit.  The code just verifies
       that it's working, doesn't actually use return value, so we can just use
       rand() */

	return (readinputport(1) & ~0x20) | (rand() & 0x20);
}

WRITE8_HANDLER( kingball_speech_dip_w )
{
	kingball_speech_dip = data;
}

WRITE8_HANDLER( kingball_sound1_w )
{
	kingball_sound = (kingball_sound & ~0x01) | data;
}

WRITE8_HANDLER( kingball_sound2_w )
{
	kingball_sound = (kingball_sound & ~0x02) | (data << 1);
	soundlatch_w (0, kingball_sound | 0xf0);
}



static READ8_HANDLER( azurian_IN1_r )
{
	return (readinputport(1) & ~0x40) | ((readinputport(3) & 0x01) << 6);
}

static READ8_HANDLER( azurian_IN2_r )
{
	return (readinputport(2) & ~0x04) | ((readinputport(3) & 0x02) << 1);
}


WRITE8_HANDLER( _4in1_bank_w )
{
	_4in1_bank = data & 0x03;
	galaxian_gfxbank_w(0, _4in1_bank);
	memory_set_bank(1, _4in1_bank);
}

READ8_HANDLER( _4in1_input_port_1_r )
{
	return (readinputport(1) & ~0xc0) | (readinputport(3+_4in1_bank) & 0xc0);
}

READ8_HANDLER( _4in1_input_port_2_r )
{
	return (readinputport(2) & 0x04) | (readinputport(3+_4in1_bank) & ~0xc4);
}


static void gmgalax_select_game(int game)
{
	gmgalax_selected_game = game;

	memory_set_bank(1, game);

	galaxian_gfxbank_w(0, gmgalax_selected_game);
}

READ8_HANDLER( gmgalax_input_port_0_r )
{
	return readinputport(gmgalax_selected_game ? 3 : 0);
}

READ8_HANDLER( gmgalax_input_port_1_r )
{
	return readinputport(gmgalax_selected_game ? 4 : 1);
}

READ8_HANDLER( gmgalax_input_port_2_r )
{
	return readinputport(gmgalax_selected_game ? 5 : 2);
}


static void cavelon_banksw(void)
{
	/* any read/write access in the 0x8000-0xffff region causes a bank switch.
       Only the lower 0x2000 is switched but we switch the whole region
       to keep the CPU core happy at the boundaries */

	cavelon_bank = !cavelon_bank;
	memory_set_bank(1, cavelon_bank);
}

static READ8_HANDLER( cavelon_banksw_r )
{
	cavelon_banksw();

	if      ((offset >= 0x0100) && (offset <= 0x0103))
		return ppi8255_0_r(offset - 0x0100);
	else if ((offset >= 0x0200) && (offset <= 0x0203))
		return ppi8255_1_r(offset - 0x0200);

	return 0xff;
}

static WRITE8_HANDLER( cavelon_banksw_w )
{
	cavelon_banksw();

	if      ((offset >= 0x0100) && (offset <= 0x0103))
		ppi8255_0_w(offset - 0x0100, data);
	else if ((offset >= 0x0200) && (offset <= 0x0203))
		ppi8255_1_w(offset - 0x0200, data);
}


READ8_HANDLER( hunchbks_mirror_r )
{
	return program_read_byte(0x1000+offset);
}

WRITE8_HANDLER( hunchbks_mirror_w )
{
	program_write_byte(0x1000+offset,data);
}


READ8_HANDLER(frogger_ppi8255_0_r)
{
	return ppi8255_0_r(offset >> 1);
}

READ8_HANDLER(frogger_ppi8255_1_r)
{
	return ppi8255_1_r(offset >> 1);
}

WRITE8_HANDLER(frogger_ppi8255_0_w)
{
	ppi8255_0_w(offset >> 1, data);
}

WRITE8_HANDLER(frogger_ppi8255_1_w)
{
	ppi8255_1_w(offset >> 1, data);
}


READ8_HANDLER(scobra_type2_ppi8255_0_r)
{
	return ppi8255_0_r(offset >> 2);
}

READ8_HANDLER(scobra_type2_ppi8255_1_r)
{
	return ppi8255_1_r(offset >> 2);
}

WRITE8_HANDLER(scobra_type2_ppi8255_0_w)
{
	ppi8255_0_w(offset >> 2, data);
}

WRITE8_HANDLER(scobra_type2_ppi8255_1_w)
{
	ppi8255_1_w(offset >> 2, data);
}


READ8_HANDLER(hustler_ppi8255_0_r)
{
	return ppi8255_0_r(offset >> 3);
}

READ8_HANDLER(hustler_ppi8255_1_r)
{
	return ppi8255_1_r(offset >> 3);
}

WRITE8_HANDLER(hustler_ppi8255_0_w)
{
	ppi8255_0_w(offset >> 3, data);
}

WRITE8_HANDLER(hustler_ppi8255_1_w)
{
	ppi8255_1_w(offset >> 3, data);
}


READ8_HANDLER(amidar_ppi8255_0_r)
{
	return ppi8255_0_r(offset >> 4);
}

READ8_HANDLER(amidar_ppi8255_1_r)
{
	return ppi8255_1_r(offset >> 4);
}

WRITE8_HANDLER(amidar_ppi8255_0_w)
{
	ppi8255_0_w(offset >> 4, data);
}

WRITE8_HANDLER(amidar_ppi8255_1_w)
{
	ppi8255_1_w(offset >> 4, data);
}


READ8_HANDLER(mars_ppi8255_0_r)
{
	return ppi8255_0_r(((offset >> 2) & 0x02) | ((offset >> 1) & 0x01));
}

READ8_HANDLER(mars_ppi8255_1_r)
{
	return ppi8255_1_r(((offset >> 2) & 0x02) | ((offset >> 1) & 0x01));
}

WRITE8_HANDLER(mars_ppi8255_0_w)
{
	ppi8255_0_w(((offset >> 2) & 0x02) | ((offset >> 1) & 0x01), data);
}

WRITE8_HANDLER(mars_ppi8255_1_w)
{
	ppi8255_1_w(((offset >> 2) & 0x02) | ((offset >> 1) & 0x01), data);
}


static ppi8255_interface ppi8255_intf =
{
	2, 								/* 2 chips */
	{input_port_0_r, 0},			/* Port A read */
	{input_port_1_r, 0},			/* Port B read */
	{input_port_2_r, 0},			/* Port C read */
	{0, soundlatch_w},				/* Port A write */
	{0, scramble_sh_irqtrigger_w},	/* Port B write */
	{0, 0}, 						/* Port C write */
};

/* extra chip for sample latch */
static ppi8255_interface sfx_ppi8255_intf =
{
	3, 									/* 3 chips */
	{input_port_0_r, 0, soundlatch2_r},	/* Port A read */
	{input_port_1_r, 0, 0},				/* Port B read */
	{input_port_2_r, 0, 0},				/* Port C read */
	{0, soundlatch_w, 0},				/* Port A write */
	{0, scramble_sh_irqtrigger_w, 0},	/* Port B write */
	{0, 0, 0}, 							/* Port C write */
};


DRIVER_INIT( pisces )
{
	/* the coin lockout was replaced */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0, galaxian_gfxbank_w);
}

DRIVER_INIT( checkmaj )
{
	/* for the title screen */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3800, 0x3800, 0, 0, checkmaj_protection_r);
}

DRIVER_INIT( dingo )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3000, 0, 0, dingo_3000_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3035, 0x3035, 0, 0, dingo_3035_r);
}

DRIVER_INIT( kingball )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xa000, 0, 0, kingball_IN0_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xa800, 0, 0, kingball_IN1_r);

	state_save_register_global(kingball_speech_dip);
	state_save_register_global(kingball_sound);
}


static UINT8 decode_mooncrst(UINT8 data,offs_t addr)
{
	UINT8 res;

	res = data;
	if (BIT(data,1)) res ^= 0x40;
	if (BIT(data,5)) res ^= 0x04;
	if ((addr & 1) == 0)
		res = (res & 0xbb) | (BIT(res,6) << 2) | (BIT(res,2) << 6);
	return res;
}

DRIVER_INIT( mooncrsu )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xa002, 0, 0, galaxian_gfxbank_w);
}

DRIVER_INIT( mooncrst )
{
	offs_t i;
	UINT8 *rom = memory_region(REGION_CPU1);


	for (i = 0;i < memory_region_length(REGION_CPU1);i++)
		rom[i] = decode_mooncrst(rom[i],i);

	init_mooncrsu();
}

DRIVER_INIT( mooncrgx )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6002, 0, 0, galaxian_gfxbank_w);
}

DRIVER_INIT( moonqsr )
{
	offs_t i;
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *decrypt = auto_malloc(0x8000);

	memory_set_decrypted_region(0, 0x0000, 0x7fff, decrypt);

	for (i = 0;i < 0x8000;i++)
		decrypt[i] = decode_mooncrst(rom[i],i);
}

DRIVER_INIT( checkman )
{
/*
                     Encryption Table
                     ----------------
+---+---+---+------+------+------+------+------+------+------+------+
|A2 |A1 |A0 |D7    |D6    |D5    |D4    |D3    |D2    |D1    |D0    |
+---+---+---+------+------+------+------+------+------+------+------+
| 0 | 0 | 0 |D7    |D6    |D5    |D4    |D3    |D2    |D1    |D0^^D6|
| 0 | 0 | 1 |D7    |D6    |D5    |D4    |D3    |D2    |D1^^D5|D0    |
| 0 | 1 | 0 |D7    |D6    |D5    |D4    |D3    |D2^^D4|D1^^D6|D0    |
| 0 | 1 | 1 |D7    |D6    |D5    |D4^^D2|D3    |D2    |D1    |D0^^D5|
| 1 | 0 | 0 |D7    |D6^^D4|D5^^D1|D4    |D3    |D2    |D1    |D0    |
| 1 | 0 | 1 |D7    |D6^^D0|D5^^D2|D4    |D3    |D2    |D1    |D0    |
| 1 | 1 | 0 |D7    |D6    |D5    |D4    |D3    |D2^^D0|D1    |D0    |
| 1 | 1 | 1 |D7    |D6    |D5    |D4^^D1|D3    |D2    |D1    |D0    |
+---+---+---+------+------+------+------+------+------+------+------+

For example if A2=1, A1=1 and A0=0 then D2 to the CPU would be an XOR of
D2 and D0 from the ROM's. Note that D7 and D3 are not encrypted.

Encryption PAL 16L8 on cardridge
         +--- ---+
    OE --|   U   |-- VCC
 ROMD0 --|       |-- D0
 ROMD1 --|       |-- D1
 ROMD2 --|VER 5.2|-- D2
    A0 --|       |-- NOT USED
    A1 --|       |-- A2
 ROMD4 --|       |-- D4
 ROMD5 --|       |-- D5
 ROMD6 --|       |-- D6
   GND --|       |-- M1 (NOT USED)
         +-------+
Pin layout is such that links can replace the PAL if encryption is not used.

*/
	static const UINT8 xortable[8][4] =
	{
		{ 6,0,6,0 },
		{ 5,1,5,1 },
		{ 4,2,6,1 },
		{ 2,4,5,0 },
		{ 4,6,1,5 },
		{ 0,6,2,5 },
		{ 0,2,0,2 },
		{ 1,4,1,4 }
	};

	offs_t i;
	UINT8 *rom = memory_region(REGION_CPU1);


	for (i = 0; i < memory_region_length(REGION_CPU1); i++)
	{
		UINT8 data_xor;
		int line = i & 0x07;

		data_xor = (BIT(rom[i],xortable[line][0]) << xortable[line][1]) |
				   (BIT(rom[i],xortable[line][2]) << xortable[line][3]);

		rom[i] ^= data_xor;
	}
}

DRIVER_INIT( gteikob2 )
{
	init_pisces();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7006, 0x7006, 0, 0, gteikob2_flip_screen_x_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7007, 0x7007, 0, 0, gteikob2_flip_screen_y_w);
}

DRIVER_INIT( azurian )
{
	init_pisces();

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6800, 0x6800, 0, 0, azurian_IN1_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7000, 0x7000, 0, 0, azurian_IN2_r);
}

DRIVER_INIT( 4in1 )
{
	offs_t i;
	UINT8 *RAM = memory_region(REGION_CPU1);

	/* Decrypt Program Roms */
	for (i = 0; i < memory_region_length(REGION_CPU1); i++)
		RAM[i] = RAM[i] ^ (i & 0xff);

	/* games are banked at 0x0000 - 0x3fff */
	memory_configure_bank(1, 0, 4, &RAM[0x10000], 0x4000);

	_4in1_bank_w(0, 0); /* set the initial CPU bank */

	state_save_register_global(_4in1_bank);
}

DRIVER_INIT( mshuttle )
{
	static const UINT8 convtable[8][16] =
	{
		/* -1 marks spots which are unused and therefore unknown */
		{ 0x40,0x41,0x44,0x15,0x05,0x51,0x54,0x55,0x50,0x00,0x01,0x04,  -1,0x10,0x11,0x14 },
		{ 0x45,0x51,0x55,0x44,0x40,0x11,0x05,0x41,0x10,0x14,0x54,0x50,0x15,0x04,0x00,0x01 },
		{ 0x11,0x14,0x10,0x00,0x44,0x05,  -1,0x04,0x45,0x15,0x55,0x50,  -1,0x01,0x54,0x51 },
		{ 0x14,0x01,0x11,0x10,0x50,0x15,0x00,0x40,0x04,0x51,0x45,0x05,0x55,0x54,  -1,0x44 },
		{ 0x04,0x10,  -1,0x40,0x15,0x41,0x50,0x50,0x11,  -1,0x14,0x00,0x51,0x45,0x55,0x01 },
		{ 0x44,0x45,0x00,0x51,  -1,  -1,0x15,0x11,0x01,0x10,0x04,0x55,0x05,0x40,0x50,0x41 },
		{ 0x51,0x00,0x01,0x05,0x04,0x55,0x54,0x50,0x41,  -1,0x11,0x15,0x14,0x10,0x44,0x40 },
		{ 0x05,0x04,0x51,0x01,  -1,  -1,0x55,  -1,0x00,0x50,0x15,0x14,0x44,0x41,0x40,0x54 },
	};

	cclimber_decode(convtable);
}

DRIVER_INIT( scramble_ppi )
{
	ppi8255_init(&ppi8255_intf);
}

DRIVER_INIT( scobra )
{
	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa803, 0xa803, 0, 0, scramble_background_enable_w);
}

DRIVER_INIT( atlantis )
{
	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6803, 0x6803, 0, 0, scramble_background_enable_w);
}

DRIVER_INIT( scramble )
{
	init_atlantis();

	ppi8255_set_portCread (1, scramble_protection_r);
	ppi8255_set_portCwrite(1, scramble_protection_w);
}

DRIVER_INIT( scrambls )
{
	init_atlantis();

	ppi8255_set_portCread(0, scrambls_input_port_2_r);
	ppi8255_set_portCread(1, scrambls_protection_r);
	ppi8255_set_portCwrite(1, scramble_protection_w);
}

DRIVER_INIT( theend )
{
	init_scramble_ppi();

	ppi8255_set_portCwrite(0, theend_coin_counter_w);
}

DRIVER_INIT( stratgyx )
{
	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xb000, 0, 0, scramble_background_green_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb002, 0xb002, 0, 0, scramble_background_blue_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb00a, 0xb00a, 0, 0, scramble_background_red_w);

	ppi8255_set_portCread(0, stratgyx_input_port_2_r);
	ppi8255_set_portCread(1, stratgyx_input_port_3_r);
}

DRIVER_INIT( tazmani2 )
{
	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb002, 0xb002, 0, 0, scramble_background_enable_w);
}

DRIVER_INIT( amidar )
{
	init_scramble_ppi();

	/* Amidar has a the DIP switches connected to port C of the 2nd 8255 */
	ppi8255_set_portCread(1, input_port_3_r);
}

DRIVER_INIT( ckongs )
{
	init_scramble_ppi();

	ppi8255_set_portBread(0, ckongs_input_port_1_r);
	ppi8255_set_portCread(0, ckongs_input_port_2_r);
}

DRIVER_INIT( mariner )
{
	init_scramble_ppi();

	/* extra ROM */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5800, 0x67ff, 0, 0, MRA8_BANK1);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5800, 0x67ff, 0, 0, MWA8_ROM);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x5800);

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9008, 0x9008, 0, 0, mariner_protection_2_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb401, 0xb401, 0, 0, mariner_protection_1_r);

	/* ??? (it's NOT a background enable) */
	/*memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6803, 0x6803, 0, 0, MWA8_NOP);*/
}

DRIVER_INIT( frogger )
{
	offs_t A;
	UINT8 *ROM;


	init_scramble_ppi();


	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	ROM = memory_region(REGION_CPU2);
	for (A = 0;A < 0x0800;A++)
		ROM[A] = BITSWAP8(ROM[A],7,6,5,4,3,2,0,1);

	/* likewise, the 2nd gfx ROM has data lines D0 and D1 swapped. Decode it. */
	ROM = memory_region(REGION_GFX1);
	for (A = 0x0800;A < 0x1000;A++)
		ROM[A] = BITSWAP8(ROM[A],7,6,5,4,3,2,0,1);
}

DRIVER_INIT( froggers )
{
	offs_t A;
	UINT8 *ROM;


	init_scramble_ppi();

	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	ROM = memory_region(REGION_CPU2);
	for (A = 0;A < 0x0800;A++)
		ROM[A] = BITSWAP8(ROM[A],7,6,5,4,3,2,0,1);
}

DRIVER_INIT( devilfsh )
{
	offs_t i;
	UINT8 *RAM;


	init_scramble_ppi();


	/* Address lines are scrambled on the main CPU */

	/* A0 -> A2 */
	/* A1 -> A0 */
	/* A2 -> A3 */
	/* A3 -> A1 */

	RAM = memory_region(REGION_CPU1);
	for (i = 0; i < 0x10000; i += 16)
	{
		offs_t j;
		UINT8 swapbuffer[16];

		for (j = 0; j < 16; j++)
		{
			offs_t new = BITSWAP8(j,7,6,5,4,2,0,3,1);

			swapbuffer[j] = RAM[i + new];
		}

		memcpy(&RAM[i], swapbuffer, 16);
	}
}

DRIVER_INIT( mars )
{
	init_devilfsh();

	/* extra port */
	ppi8255_set_portCread(1, input_port_3_r);
}

DRIVER_INIT( hotshock )
{
	/* protection??? The game jumps into never-neverland here. I think
       it just expects a RET there */
	memory_region(REGION_CPU1)[0x2ef9] = 0xc9;
}

DRIVER_INIT( cavelon )
{
	UINT8 *ROM = memory_region(REGION_CPU1);

	init_scramble_ppi();

	/* banked ROM */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, MRA8_BANK1);
	memory_configure_bank(1, 0, 2, &ROM[0x00000], 0x10000);
	cavelon_banksw();

	/* A15 switches memory banks */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, cavelon_banksw_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, cavelon_banksw_w);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2000, 0, 0, MWA8_NOP);	/* ??? */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3800, 0x3801, 0, 0, MWA8_NOP);  /* looks suspicously like
                                                               an AY8910, but not sure */
	state_save_register_global(cavelon_bank);
}

DRIVER_INIT( moonwar )
{
	init_scramble_ppi();

	/* special handler for the spinner */
	ppi8255_set_portAread (0, moonwar_input_port_0_r);
	ppi8255_set_portCwrite(0, moonwar_port_select_w);

	state_save_register_global(moonwar_port_select);
}

DRIVER_INIT( darkplnt )
{
	init_scramble_ppi();

	/* special handler for the spinner */
	ppi8255_set_portBread(0, darkplnt_input_port_1_r);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb00a, 0xb00a, 0, 0, darkplnt_bullet_color_w);
}

DRIVER_INIT( mimonkey )
{
	static const UINT8 xortable[16][16] =
	{
		{ 0x03,0x03,0x05,0x07,0x85,0x00,0x85,0x85,0x80,0x80,0x06,0x03,0x03,0x00,0x00,0x81 },
		{ 0x83,0x87,0x03,0x87,0x06,0x00,0x06,0x04,0x02,0x00,0x84,0x84,0x04,0x00,0x01,0x83 },
		{ 0x82,0x82,0x84,0x02,0x04,0x00,0x00,0x03,0x82,0x00,0x06,0x80,0x03,0x00,0x81,0x07 },
		{ 0x06,0x06,0x82,0x81,0x85,0x00,0x04,0x07,0x81,0x05,0x04,0x00,0x03,0x00,0x82,0x84 },
		{ 0x07,0x07,0x80,0x07,0x07,0x00,0x85,0x86,0x00,0x07,0x06,0x04,0x85,0x00,0x86,0x85 },
		{ 0x81,0x83,0x02,0x02,0x87,0x00,0x86,0x03,0x04,0x06,0x80,0x05,0x87,0x00,0x81,0x81 },
		{ 0x01,0x01,0x00,0x07,0x07,0x00,0x01,0x01,0x07,0x07,0x06,0x00,0x06,0x00,0x07,0x07 },
		{ 0x80,0x87,0x81,0x87,0x83,0x00,0x84,0x01,0x01,0x86,0x86,0x80,0x86,0x00,0x86,0x86 },
		{ 0x03,0x03,0x05,0x07,0x85,0x00,0x85,0x85,0x80,0x80,0x06,0x03,0x03,0x00,0x00,0x81 },
		{ 0x83,0x87,0x03,0x87,0x06,0x00,0x06,0x04,0x02,0x00,0x84,0x84,0x04,0x00,0x01,0x83 },
		{ 0x82,0x82,0x84,0x02,0x04,0x00,0x00,0x03,0x82,0x00,0x06,0x80,0x03,0x00,0x81,0x07 },
		{ 0x06,0x06,0x82,0x81,0x85,0x00,0x04,0x07,0x81,0x05,0x04,0x00,0x03,0x00,0x82,0x84 },
		{ 0x07,0x07,0x80,0x07,0x07,0x00,0x85,0x86,0x00,0x07,0x06,0x04,0x85,0x00,0x86,0x85 },
		{ 0x81,0x83,0x02,0x02,0x87,0x00,0x86,0x03,0x04,0x06,0x80,0x05,0x87,0x00,0x81,0x81 },
		{ 0x01,0x01,0x00,0x07,0x07,0x00,0x01,0x01,0x07,0x07,0x06,0x00,0x06,0x00,0x07,0x07 },
		{ 0x80,0x87,0x81,0x87,0x83,0x00,0x84,0x01,0x01,0x86,0x86,0x80,0x86,0x00,0x86,0x86 }
	};

	unsigned char *ROM = memory_region(REGION_CPU1);
	int A, ctr = 0, line, col;

	for( A = 0; A < 0x4000; A++ )
	{
		line = (ctr & 0x07) | ((ctr & 0x200) >> 6);
		col = ((ROM[A] & 0x80) >> 4) | (ROM[A] & 0x07);
		ROM[A] = ROM[A] ^ xortable[line][col];
		ctr++;
	}

	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa804, 0xa804, 0, 0, scramble_background_enable_w);
}

DRIVER_INIT( mimonsco )
{
	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa804, 0xa804, 0, 0, scramble_background_enable_w);
}

DRIVER_INIT( mimonscr )
{
	init_scramble_ppi();

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6804, 0x6804, 0, 0, scramble_background_enable_w);
}


static int bit(int i,int n)
{
	return ((i >> n) & 1);
}


DRIVER_INIT( anteater )
{
	offs_t i;
	UINT8 *RAM;
	UINT8 *scratch;


	init_scobra();

	/*
    *   Code To Decode Lost Tomb by Mirko Buffoni
    *   Optimizations done by Fabio Buffoni
    */

	RAM = memory_region(REGION_GFX1);

	scratch = malloc(memory_region_length(REGION_GFX1));

	if (scratch)
	{
		memcpy(scratch, RAM, memory_region_length(REGION_GFX1));

		for (i = 0; i < memory_region_length(REGION_GFX1); i++)
		{
			int j;


			j = i & 0x9bf;
			j |= ( bit(i,4) ^ bit(i,9) ^ ( bit(i,2) & bit(i,10) ) ) << 6;
			j |= ( bit(i,2) ^ bit(i,10) ) << 9;
			j |= ( bit(i,0) ^ bit(i,6) ^ 1 ) << 10;

			RAM[i] = scratch[j];
		}

		free(scratch);
	}
}

DRIVER_INIT( rescue )
{
	offs_t i;
	UINT8 *RAM;
	UINT8 *scratch;


	init_scobra();

	/*
    *   Code To Decode Lost Tomb by Mirko Buffoni
    *   Optimizations done by Fabio Buffoni
    */

	RAM = memory_region(REGION_GFX1);

	scratch = malloc(memory_region_length(REGION_GFX1));

	if (scratch)
	{
		memcpy(scratch, RAM, memory_region_length(REGION_GFX1));

		for (i = 0; i < memory_region_length(REGION_GFX1); i++)
		{
			int j;


			j = i & 0xa7f;
			j |= ( bit(i,3) ^ bit(i,10) ) << 7;
			j |= ( bit(i,1) ^ bit(i,7) ) << 8;
			j |= ( bit(i,0) ^ bit(i,8) ) << 10;

			RAM[i] = scratch[j];
		}

		free(scratch);
	}
}

DRIVER_INIT( minefld )
{
	offs_t i;
	UINT8 *RAM;
	UINT8 *scratch;


	init_scobra();

	/*
    *   Code To Decode Minefield by Mike Balfour and Nicola Salmoria
    */

	RAM = memory_region(REGION_GFX1);

	scratch = malloc(memory_region_length(REGION_GFX1));

	if (scratch)
	{
		memcpy(scratch, RAM, memory_region_length(REGION_GFX1));

		for (i = 0; i < memory_region_length(REGION_GFX1); i++)
		{
			int j;


			j  = i & 0xd5f;
			j |= ( bit(i,3) ^ bit(i,7) ) << 5;
			j |= ( bit(i,2) ^ bit(i,9) ^ ( bit(i,0) & bit(i,5) ) ^
				 ( bit(i,3) & bit(i,7) & ( bit(i,0) ^ bit(i,5) ))) << 7;
			j |= ( bit(i,0) ^ bit(i,5) ^ ( bit(i,3) & bit(i,7) ) ) << 9;

			RAM[i] = scratch[j];
		}

		free(scratch);
	}
}

DRIVER_INIT( losttomb )
{
	offs_t i;
	UINT8 *RAM;
	UINT8 *scratch;


	init_scramble();

	/*
    *   Code To Decode Lost Tomb by Mirko Buffoni
    *   Optimizations done by Fabio Buffoni
    */

	RAM = memory_region(REGION_GFX1);

	scratch = malloc(memory_region_length(REGION_GFX1));

	if (scratch)
	{
		memcpy(scratch, RAM, memory_region_length(REGION_GFX1));

		for (i = 0; i < memory_region_length(REGION_GFX1); i++)
		{
			int j;


			j = i & 0xa7f;
			j |= ( (bit(i,1) & bit(i,8)) | ((1 ^ bit(i,1)) & (bit(i,10)))) << 7;
			j |= ( bit(i,7) ^ (bit(i,1) & ( bit(i,7) ^ bit(i,10) ))) << 8;
			j |= ( (bit(i,1) & bit(i,7)) | ((1 ^ bit(i,1)) & (bit(i,8)))) << 10;

			RAM[i] = scratch[j];
		}

		free(scratch);
	}
}

DRIVER_INIT( superbon )
{
	offs_t i;
	UINT8 *RAM;


	init_scramble();

	/* Deryption worked out by hand by Chris Hardy. */

	RAM = memory_region(REGION_CPU1);

	for (i = 0;i < 0x1000;i++)
	{
		/* Code is encrypted depending on bit 7 and 9 of the address */
		switch (i & 0x0280)
		{
		case 0x0000:
			RAM[i] ^= 0x92;
			break;
		case 0x0080:
			RAM[i] ^= 0x82;
			break;
		case 0x0200:
			RAM[i] ^= 0x12;
			break;
		case 0x0280:
			RAM[i] ^= 0x10;
			break;
		}
	}
}


DRIVER_INIT( hustler )
{
	offs_t A;


	init_scramble_ppi();


	for (A = 0;A < 0x4000;A++)
	{
		UINT8 xormask;
		int bits[8];
		int i;
		UINT8 *rom = memory_region(REGION_CPU1);


		for (i = 0;i < 8;i++)
			bits[i] = (A >> i) & 1;

		xormask = 0xff;
		if (bits[0] ^ bits[1]) xormask ^= 0x01;
		if (bits[3] ^ bits[6]) xormask ^= 0x02;
		if (bits[4] ^ bits[5]) xormask ^= 0x04;
		if (bits[0] ^ bits[2]) xormask ^= 0x08;
		if (bits[2] ^ bits[3]) xormask ^= 0x10;
		if (bits[1] ^ bits[5]) xormask ^= 0x20;
		if (bits[0] ^ bits[7]) xormask ^= 0x40;
		if (bits[4] ^ bits[6]) xormask ^= 0x80;

		rom[A] ^= xormask;
	}

	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	{
		UINT8 *rom = memory_region(REGION_CPU2);


		for (A = 0;A < 0x0800;A++)
			rom[A] = BITSWAP8(rom[A],7,6,5,4,3,2,0,1);
	}
}

DRIVER_INIT( billiard )
{
	offs_t A;


	init_scramble_ppi();


	for (A = 0;A < 0x4000;A++)
	{
		UINT8 xormask;
		int bits[8];
		int i;
		UINT8 *rom = memory_region(REGION_CPU1);


		for (i = 0;i < 8;i++)
			bits[i] = (A >> i) & 1;

		xormask = 0x55;
		if (bits[2] ^ ( bits[3] &  bits[6])) xormask ^= 0x01;
		if (bits[4] ^ ( bits[5] &  bits[7])) xormask ^= 0x02;
		if (bits[0] ^ ( bits[7] & !bits[3])) xormask ^= 0x04;
		if (bits[3] ^ (!bits[0] &  bits[2])) xormask ^= 0x08;
		if (bits[5] ^ (!bits[4] &  bits[1])) xormask ^= 0x10;
		if (bits[6] ^ (!bits[2] & !bits[5])) xormask ^= 0x20;
		if (bits[1] ^ (!bits[6] & !bits[4])) xormask ^= 0x40;
		if (bits[7] ^ (!bits[1] &  bits[0])) xormask ^= 0x80;

		rom[A] ^= xormask;

		rom[A] = BITSWAP8(rom[A],6,1,2,5,4,3,0,7);
	}

	/* the first ROM of the second CPU has data lines D0 and D1 swapped. Decode it. */
	{
		UINT8 *rom = memory_region(REGION_CPU2);


		for (A = 0;A < 0x0800;A++)
			rom[A] = BITSWAP8(rom[A],7,6,5,4,3,2,0,1);
	}
}

DRIVER_INIT( ladybugg )
{
/* Doesn't actually use the bank, but it mustn't have a coin lock! */
memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0, galaxian_gfxbank_w);
}

/************************************************************
 mr kougar protected main cpu - by HIGHWAYMAN
 mr kougar contains a steel module at location S7,
 this module contains a Z80c cpu with the following changes:
 IOREQ pin cut, RD & WR pins swapped and the following
 address lines swapped - a0-a2,a1-a0,a2-a3,a3-a1.
*************************************************************/

DRIVER_INIT( mrkougar )
{
	init_devilfsh();

	/* no sound enabled bit */
	ppi8255_set_portBwrite(1, mrkougar_sh_irqtrigger_w);
}

DRIVER_INIT( mrkougb )
{
	init_scramble_ppi();

	/* no sound enabled bit */
	ppi8255_set_portBwrite(1, mrkougar_sh_irqtrigger_w);
}

DRIVER_INIT( sfx )
{
	ppi8255_init(&sfx_ppi8255_intf);
}

DRIVER_INIT( gmgalax )
{
	/* games are banked at 0x0000 - 0x3fff */
	UINT8 *RAM=memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 2, &RAM[0x10000], 0x4000);

	state_save_register_global(gmgalax_selected_game);

	gmgalax_select_game(input_port_6_r(0) & 0x01);
}

static READ8_HANDLER( scorpion_prot_r )
{
	/* HACK! return register C */
	return activecpu_get_reg(Z80_C) & 0xff;
}

static READ8_HANDLER( scorpion_sound_status_r )
{
	return 1;
}

DRIVER_INIT( scorpion )
{
	ppi8255_init(&ppi8255_intf);

	ppi8255_set_portCread(1, scorpion_prot_r);

	/* extra ROM */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5800, 0x67ff, 0, 0, MRA8_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x5800);

	/* no background related */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6803, 0x6803, 0, 0, MWA8_NOP);

	memory_install_read8_handler(1, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3000, 0, 0, scorpion_sound_status_r);
}

DRIVER_INIT( ad2083 )
{
	UINT8 c;
	int i;

	UINT8 *ROM = memory_region(REGION_CPU1);

	for (i=0; i<memory_region_length(REGION_CPU1); i++)
	{
		c = ROM[i] ^ 0x35;
		c = BITSWAP8(c, 6,2,5,1,7,3,4,0); /* also swapped inside of the bigger module */
		ROM[i] = c;
	}
}

INTERRUPT_GEN( hunchbks_vh_interrupt )
{
	cpunum_set_input_line_and_vector(0,0,PULSE_LINE,0x03);
}

INTERRUPT_GEN( gmgalax_vh_interrupt )
{
	// reset the cpu if the selected game changed
	int new_game = input_port_6_r(0) & 0x01;

	if (gmgalax_selected_game != new_game)
	{
		gmgalax_select_game(new_game);

		/* Ghost Muncher never clears this */
		galaxian_stars_enable_w(0, 0);

		cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
	}
}
