// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/****************************************

	Namco Universal System 16 hardware
	Libble Rabble (1983) / Toypop (1986)

	TODO:
	- verify the PAL decoding between the two main CPU maps;
	- proper sprite DMA;
	- flip screen and remaining outputs.

****************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "machine/namcoio.h"
#include "sound/namco.h"

#define MASTER_CLOCK 6144000
#define HTOTAL 384
#define VTOTAL 264
#define VBSTART 224

extern UINT8 *toypop_master_workram;
extern UINT8 *toypop_fgvram, *toypop_fgattr;
extern UINT16 *toypop_bgvram;
extern UINT8 toypop_pal_bank;
PALETTE_INIT( toypop );
VIDEO_START( toypop );
VIDEO_UPDATE( toypop );

static UINT8 *toypop_slave_sharedram;
static UINT8 toypop_master_irq_enable, toypop_slave_irq_enable;
static mame_timer *toypop_scanline_timer;

static READ8_HANDLER( irq_enable_r )
{
	toypop_master_irq_enable = 1;
	return 0;
}

static WRITE8_HANDLER( irq_disable_w )
{
	toypop_master_irq_enable = 0;
}

static WRITE8_HANDLER( irq_ctrl_w )
{
	/* Address bit 11 selects enable (0x7000) or disable (0x7800). */
	toypop_master_irq_enable = (offset & 0x800) ? 0 : 1;
}

static WRITE8_HANDLER( slave_halt_ctrl_w )
{
	cpunum_set_input_line(1,INPUT_LINE_RESET,(offset & 0x800) ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE8_HANDLER( sound_halt_ctrl_w )
{
	cpunum_set_input_line(2,INPUT_LINE_RESET,(offset & 0x800) ? ASSERT_LINE : CLEAR_LINE);
}

static READ16_HANDLER( slave_shared_r )
{
	/* Only the low byte is connected; the unused lane reads zero. */
	return toypop_slave_sharedram[offset];
}

static WRITE16_HANDLER( slave_shared_w )
{
	if(ACCESSING_LSB)
		toypop_slave_sharedram[offset] = data;
}

static WRITE16_HANDLER( slave_irq_enable_w )
{
	/* A 16-bit handler receives word offsets: bit 18 is byte address bit 19. */
	toypop_slave_irq_enable = (offset & 0x40000) ? 0 : 1;
}

static UINT8 bg_rmw_byte_r(int offset)
{
	UINT16 pixels = toypop_bgvram[offset];
	return ((pixels & 0x0f00) >> 4) | (pixels & 0x000f);
}

static void bg_rmw_byte_w(int offset,UINT8 data)
{
	toypop_bgvram[offset] = (data & 0x0f) | ((data & 0xf0) << 4);
}

static READ16_HANDLER( bg_rmw_r )
{
	return (bg_rmw_byte_r(offset * 2) << 8) | bg_rmw_byte_r(offset * 2 + 1);
}

static WRITE16_HANDLER( bg_rmw_w )
{
	/* Each byte lane expands into a separate background word. */
	if(ACCESSING_MSB)
		bg_rmw_byte_w(offset * 2,data >> 8);
	if(ACCESSING_LSB)
		bg_rmw_byte_w(offset * 2 + 1,data);
}

static READ8_HANDLER( sound_shared_r )
{
	return namco_soundregs[offset];
}

static WRITE8_HANDLER( sound_shared_w )
{
	if(offset < 0x40)
		namco_15xx_w(offset,data);
	else
		namco_soundregs[offset] = data;
}

static WRITE8_HANDLER( pal_bank_w )
{
	toypop_pal_bank = offset & 1;
}

static READ8_HANDLER( coins_r )    { return readinputportbytag("COINS"); }
static READ8_HANDLER( p1_right_r ) { return readinputportbytag("P1_RIGHT"); }
static READ8_HANDLER( p2_right_r ) { return readinputportbytag("P2_RIGHT"); }
static READ8_HANDLER( buttons_r )  { return readinputportbytag("BUTTONS"); }
static READ8_HANDLER( dipA_l )     { return readinputportbytag("DSW1"); }
static READ8_HANDLER( dipA_h )     { return readinputportbytag("DSW1") >> 4; }
static READ8_HANDLER( dipB_l )     { return readinputportbytag("DSW2"); }
static READ8_HANDLER( dipB_h )     { return readinputportbytag("DSW2") >> 4; }
static READ8_HANDLER( p1_left_r )  { return readinputportbytag("P1_LEFT"); }
static READ8_HANDLER( p2_left_r )  { return readinputportbytag("P2_LEFT"); }
static READ8_HANDLER( service_r )  { return readinputportbytag("SERVICE"); }

static WRITE8_HANDLER( flip_w )
{
	flip_screen_set(data & 1);
}

static const struct namcoio_interface io58xx =
{
	{ coins_r, p1_right_r, p2_right_r, buttons_r },
	{ NULL, NULL }
};

static const struct namcoio_interface io56xx_1 =
{
	{ dipA_h, dipB_l, dipB_h, dipA_l },
	{ flip_w, NULL }
};

static const struct namcoio_interface io56xx_2 =
{
	{ NULL, p1_left_r, p2_left_r, service_r },
	{ NULL, NULL }
};

static DRIVER_INIT( namcos16 )
{
	namcoio_init(0,NAMCOIO_58XX,&io58xx);
	namcoio_init(1,NAMCOIO_56XX,&io56xx_1);
	namcoio_init(2,NAMCOIO_56XX,&io56xx_2);
}



static ADDRESS_MAP_START( master_liblrabl_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x63ff) AM_READWRITE(sound_shared_r,sound_shared_w)
	AM_RANGE(0x6800, 0x682f) AM_READWRITE(namcoio_r,namcoio_w)
	AM_RANGE(0x7000, 0x7fff) AM_READWRITE(MRA8_NOP,irq_ctrl_w)
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_BASE(&toypop_fgvram)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_BASE(&toypop_fgattr)
	AM_RANGE(0x0800, 0x1fff) AM_RAM AM_BASE(&toypop_master_workram)
	AM_RANGE(0x2800, 0x2fff) AM_RAM AM_BASE(&toypop_slave_sharedram)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(slave_halt_ctrl_w)
	AM_RANGE(0x9000, 0x9fff) AM_WRITE(sound_halt_ctrl_w)
	AM_RANGE(0xa000, 0xa001) AM_WRITE(pal_bank_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( master_toypop_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x6000, 0x602f) AM_READWRITE(namcoio_r,namcoio_w)
	AM_RANGE(0x6800, 0x6bff) AM_READWRITE(sound_shared_r,sound_shared_w)
	AM_RANGE(0x7000, 0x7000) AM_READWRITE(irq_enable_r,irq_disable_w)
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_BASE(&toypop_fgvram)
	AM_RANGE(0x0400, 0x07ff) AM_RAM AM_BASE(&toypop_fgattr)
	AM_RANGE(0x0800, 0x1fff) AM_RAM AM_BASE(&toypop_master_workram)
	AM_RANGE(0x2800, 0x2fff) AM_RAM AM_BASE(&toypop_slave_sharedram)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(slave_halt_ctrl_w)
	AM_RANGE(0x9000, 0x9fff) AM_WRITE(sound_halt_ctrl_w)
	AM_RANGE(0xa000, 0xa001) AM_WRITE(pal_bank_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x007fff) AM_ROM
	AM_RANGE(0x080000, 0x0bffff) AM_RAM
	AM_RANGE(0x100000, 0x100fff) AM_READWRITE(slave_shared_r,slave_shared_w)
	AM_RANGE(0x180000, 0x187fff) AM_READWRITE(bg_rmw_r,bg_rmw_w)
	AM_RANGE(0x190000, 0x1dffff) AM_RAM AM_BASE(&toypop_bgvram)
	AM_RANGE(0x300000, 0x3fffff) AM_WRITE(slave_irq_enable_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_READWRITE(sound_shared_r,sound_shared_w) AM_BASE(&namco_soundregs)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static void master_scanline(int scanline)
{
	int chip;

	if(scanline == VBSTART)
	{
		if(toypop_master_irq_enable)
			cpunum_set_input_line(0,M6809_IRQ_LINE,HOLD_LINE);
	}
	else
	{
		/* The I/O core schedules the custom command after its response delay. */
		for(chip = 0; chip < 3; chip++)
			namcoio_set_irq_line(chip,PULSE_LINE);
	}

	scanline = (scanline == 0) ? VBSTART : 0;
	timer_adjust(toypop_scanline_timer,cpu_getscanlinetime(scanline),scanline,0);
}

static INTERRUPT_GEN( slave_vblank_irq )
{
	if(toypop_slave_irq_enable)
		cpunum_set_input_line(1,6,HOLD_LINE);
}

static MACHINE_START( namcos16 )
{
	toypop_scanline_timer = timer_alloc(master_scanline);
	state_save_register_global(toypop_master_irq_enable);
	state_save_register_global(toypop_slave_irq_enable);
	state_save_register_global(toypop_pal_bank);
	return 0;
}

static void hold_subcpus(int param)
{
	cpunum_set_input_line(1,INPUT_LINE_RESET,ASSERT_LINE);
	cpunum_set_input_line(2,INPUT_LINE_RESET,ASSERT_LINE);
}

static MACHINE_RESET( namcos16 )
{
	int chip;
	toypop_master_irq_enable = 0;
	toypop_slave_irq_enable = 0;
	toypop_pal_bank = 0;
	for(chip = 0; chip < 3; chip++)
		namcoio_set_reset_line(chip,PULSE_LINE);
	/* CPU reset callbacks run after the driver callback, so assert afterwards. */
	timer_set(TIME_NOW,0,hold_subcpus);
	timer_adjust(toypop_scanline_timer,TIME_NOW,0,0);
}

static INPUT_PORTS_START( liblrabl )
	/* The inputs are not memory mapped, they are handled by three I/O chips. */
	PORT_START_TAG("P1_RIGHT")  /* 58XX #0 pins 22-29 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT ) PORT_8WAY

	PORT_START_TAG("P2_RIGHT")  /* 58XX #0 pins 22-29 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("P1_LEFT")   /* 56XX #2 pins 22-29 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_8WAY

	PORT_START_TAG("P2_LEFT")   /* 56XX #2 pins 22-29 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_8WAY PORT_COCKTAIL

	PORT_START_TAG("BUTTONS")   /* 58XX #0 pins 30-33 and 38-41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("COINS") /* 58XX #0 pins 30-33 and 38-41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("DSW1")  /* 56XX #1 pins 22-29 */
	/* default setting: all OFF */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SWA:8,7")
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SWA:6,5,4")
	// bonus scores for common
	PORT_DIPSETTING(    0x1c, "40k 120k 200k 400k 600k 1m" )
	PORT_DIPSETTING(    0x0c, "40k 140k 250k 400k 700k 1m" )
	// bonus scores for 1, 2 or 3 lives
	PORT_DIPSETTING(    0x14, "50k 150k 300k 500k 700k 1m" ) PORT_CONDITION("DSW1", 0x03, PORTCOND_NOTEQUALS, 0x01)
	PORT_DIPSETTING(    0x04, "40k 120k and every 120k" )    PORT_CONDITION("DSW1", 0x03, PORTCOND_NOTEQUALS, 0x01)
	PORT_DIPSETTING(    0x18, "40k 150k and every 150k" )    PORT_CONDITION("DSW1", 0x03, PORTCOND_NOTEQUALS, 0x01)
	PORT_DIPSETTING(    0x08, "50k 150k 300k" )              PORT_CONDITION("DSW1", 0x03, PORTCOND_NOTEQUALS, 0x01)
	PORT_DIPSETTING(    0x10, "40k 120k 200k" )              PORT_CONDITION("DSW1", 0x03, PORTCOND_NOTEQUALS, 0x01)
	// bonus scores for 5 lives
	PORT_DIPSETTING(    0x14, "40k 120k" )                   PORT_CONDITION("DSW1", 0x03, PORTCOND_EQUALS, 0x01)
	PORT_DIPSETTING(    0x04, "50k 150k" )                   PORT_CONDITION("DSW1", 0x03, PORTCOND_EQUALS, 0x01)
	PORT_DIPSETTING(    0x18, "50k 150k and every 150k" )    PORT_CONDITION("DSW1", 0x03, PORTCOND_EQUALS, 0x01)
	PORT_DIPSETTING(    0x08, "60k 200k and every 200k" )    PORT_CONDITION("DSW1", 0x03, PORTCOND_EQUALS, 0x01)
	PORT_DIPSETTING(    0x10, "50k" )                        PORT_CONDITION("DSW1", 0x03, PORTCOND_EQUALS, 0x01)
	// bonus scores for common
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SWA:3,2,1")
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_6C ) )

	PORT_START_TAG("DSW2")  /* 56XX #1 pins 30-33 and 38-41 */
	PORT_DIPNAME( 0x01, 0x01, "Freeze" ) PORT_DIPLOCATION("SWB:8")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Rack Test" ) PORT_DIPLOCATION("SWB:7")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SWB:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SWB:5,4")
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x20, 0x20, "Practice" ) PORT_DIPLOCATION("SWB:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SWB:2,1")
	PORT_DIPSETTING(    0xc0, "A" )
	PORT_DIPSETTING(    0x40, "B" )
	PORT_DIPSETTING(    0x80, "C" )
	PORT_DIPSETTING(    0x00, "D" )

	PORT_START_TAG("SERVICE")   /* 56XX #2 pins 30-33 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
INPUT_PORTS_END

static INPUT_PORTS_START( toypop )
	/* The inputs are not memory mapped, they are handled by three I/O chips. */
	PORT_START_TAG("P1_RIGHT")  /* 58XX #0 pins 22-29 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY

	PORT_START_TAG("P2_RIGHT")  /* 58XX #0 pins 22-29 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)

	PORT_START_TAG("BUTTONS")   /* 58XX #0 pins 30-33 and 38-41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("COINS") /* 58XX #0 pins 30-33 and 38-41 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("DSW1")  /* 56XX #1 pins 22-29 */
	/* default setting: all OFF */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) ) PORT_DIPLOCATION("SWA:8,7")
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) ) PORT_DIPLOCATION("SWA:6,5")
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) PORT_DIPLOCATION("SWA:4,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) ) PORT_DIPLOCATION("SWA:2")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START_TAG("DSW2")  /* 56XX #1 pins 30-33 and 38-41 */
	PORT_DIPNAME( 0x01, 0x01, "Freeze" ) PORT_DIPLOCATION("SWB:8")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Level_Select ) ) PORT_DIPLOCATION("SWB:7")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "2 Players Game" ) PORT_DIPLOCATION("SWB:6")
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Credits" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) PORT_DIPLOCATION("SWB:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Entering" ) PORT_DIPLOCATION("SWB:4")    // buy in
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) ) PORT_DIPLOCATION("SWB:3,2")
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x60, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Bonus_Life ) ) PORT_DIPLOCATION("SWB:1")
	PORT_DIPSETTING(    0x80, "Every 15000 points" )
	PORT_DIPSETTING(    0x00, "Every 20000 points" )

	PORT_START_TAG("P1_LEFT")   /* 56XX #2 pins 22-29 */
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("P2_LEFT")   /* 56XX #2 pins 22-29 */
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("SERVICE")   /* 56XX #2 pins 30-33 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) // would be Cabinet, but this game has no cocktail mode
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE )    // service mode again
INPUT_PORTS_END


static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
	24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	32 * 8, 33 * 8, 34 * 8, 35 * 8, 36 * 8, 37 * 8, 38 * 8, 39 * 8 },
	64*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 128 },
	{ REGION_GFX2, 0, &spritelayout, 128*4,  64 },
	{ -1 }
};

static const struct namco_interface namco_interface =
{
	8,
	REGION_SOUND1
};

static MACHINE_DRIVER_START( liblrabl )
	MDRV_CPU_ADD_TAG("main",M6809,MASTER_CLOCK/4)
	MDRV_CPU_PROGRAM_MAP(master_liblrabl_map,0)

	MDRV_CPU_ADD(M68000,MASTER_CLOCK)
	MDRV_CPU_PROGRAM_MAP(slave_map,0)
	MDRV_CPU_VBLANK_INT(slave_vblank_irq,1)

	MDRV_CPU_ADD(M6809,MASTER_CLOCK/4)
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,TIME_IN_HZ(60))

	MDRV_FRAMES_PER_SECOND((double)MASTER_CLOCK / (HTOTAL * VTOTAL))
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_START(namcos16)
	MDRV_MACHINE_RESET(namcos16)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(HTOTAL,VTOTAL)
	MDRV_VISIBLE_AREA(0,287,0,223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(128*4+64*4+16*2)
	MDRV_PALETTE_INIT(toypop)
	MDRV_VIDEO_START(toypop)
	MDRV_VIDEO_UPDATE(toypop)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(NAMCO_15XX,24000)
	MDRV_SOUND_CONFIG(namco_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( toypop )
	MDRV_IMPORT_FROM(liblrabl)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(master_toypop_map,0)
MACHINE_DRIVER_END

ROM_START( liblrabl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "5b.rom",   0x8000, 0x4000, CRC(da7a93c2) SHA1(fe4a02cdab66722eb7b8cf58825f899b1949a6a2) )
	ROM_LOAD( "5c.rom",   0xc000, 0x4000, CRC(6cae25dc) SHA1(de74317a7d5de1865d096c377923a764be5e6879) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "2c.rom",   0xe000, 0x2000, CRC(7c09e50a) SHA1(5f004d60bbb7355e008a9cda137b28bc2192b8ef) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE("8c.rom",    0x0000, 0x4000, CRC(a00cd959) SHA1(cc5621103c31cfbc65941615cab391db0f74e6ce) )
	ROM_LOAD16_BYTE("10c.rom",   0x0001, 0x4000, CRC(09ce209b) SHA1(2ed46d6592f8227bac8ab54963d9a300706ade47) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5p.rom",   0x0000, 0x2000, CRC(3b4937f0) SHA1(06d9de576f1c2262c34aeb91054e68c9298af688) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "9t.rom",   0x0000, 0x4000, CRC(a88e24ca) SHA1(eada133579f19de09255084dcdc386311606a335) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "lr1-3.1r", 0x0000, 0x0100, CRC(f3ec0d07) SHA1(b0aad1fb6df79f202889600f486853995352f9c2) )
	ROM_LOAD( "lr1-2.1s", 0x0100, 0x0100, CRC(2ae4f702) SHA1(838fdca9e91fea4f64a59880ac47c48973bb8fbf) )
	ROM_LOAD( "lr1-1.1t", 0x0200, 0x0100, CRC(7601f208) SHA1(572d070ca387b780030ed5de38a8970b7cc14349) )
	ROM_LOAD( "lr1-5.5l", 0x0300, 0x0100, CRC(940f5397) SHA1(825a7bd78a8a08d30bad2e4890ae6e9ad88b36b8) )
	ROM_LOAD( "lr1-6.2p", 0x0400, 0x0200, CRC(a6b7f850) SHA1(7cfde16dfd5c4d5b876b4fbe4f924f1385932a93) )

	ROM_REGION( 0x0100, REGION_SOUND1, 0 )
	ROM_LOAD( "lr1-4.3d", 0x0000, 0x0100, CRC(16a9166a) SHA1(847cbaf7c88616576c410177e066ae1d792ac0ba) )
ROM_END

ROM_START( toypop )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tp1-2.5b", 0x8000, 0x4000, CRC(87469620) SHA1(2ee257486c9c044386ac7d0cd4a90583eaeb3e97) )
	ROM_LOAD( "tp1-1.5c", 0xc000, 0x4000, CRC(dee2fd6e) SHA1(b2c12008d6d3e7544ba3c12a52a6abf9181842c8) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "tp1-3.2c", 0xe000, 0x2000, CRC(5f3bf6e2) SHA1(d1b3335661b9b23cb10001416c515b77b5e783e9) )

	ROM_REGION( 0x8000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE("tp1-4.8c",  0x0000, 0x4000, CRC(76997db3) SHA1(5023a2f20a5f2c9baff130f6832583493c71f883) )
	ROM_LOAD16_BYTE("tp1-5.10c", 0x0001, 0x4000, CRC(37de8786) SHA1(710365e34c05d01815844c414518f93234b6160b) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp1-7.5p", 0x0000, 0x2000, CRC(95076f9e) SHA1(1e3d32b21f6d46591ec3921aba51f672d64a9023) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp1-6.9t", 0x0000, 0x4000, CRC(481ffeaf) SHA1(c51735ad3a1dbb46ad414408b54554e9223b2219) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "tp1-3.1r", 0x0000, 0x0100, CRC(cfce2fa5) SHA1(b42aa0f34d885389d2650bf7a0531b95703b8a28) )
	ROM_LOAD( "tp1-2.1s", 0x0100, 0x0100, CRC(aeaf039d) SHA1(574560526100d38635aecd71eb73499c4f57d586) )
	ROM_LOAD( "tp1-1.1t", 0x0200, 0x0100, CRC(08e7cde3) SHA1(5261aca6834d635d17f8afaa8e35848930030ba4) )
	ROM_LOAD( "tp1-4.5l", 0x0300, 0x0100, CRC(74138973) SHA1(2e21dbb1b19dd089da52e70fcb0ca91336e004e6) )
	ROM_LOAD( "tp1-5.2p", 0x0400, 0x0200, CRC(4d77fa5a) SHA1(2438910314b23ecafb553230244f3931861ad2da) )

	ROM_REGION( 0x0100, REGION_SOUND1, 0 )
	ROM_LOAD( "tp1-6.3d", 0x0000, 0x0100, CRC(16a9166a) SHA1(847cbaf7c88616576c410177e066ae1d792ac0ba) )
ROM_END

GAME( 1983, liblrabl, 0,     liblrabl, liblrabl, namcos16,   ROT0,   "Namco", "Libble Rabble", GAME_NO_COCKTAIL )
GAME( 1986, toypop,   0,     toypop,   toypop,   namcos16,   ROT0,   "Namco", "Toypop", GAME_NO_COCKTAIL )
