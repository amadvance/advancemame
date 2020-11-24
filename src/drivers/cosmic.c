/***************************************************************************

Universal board numbers (found on the schematics)

Cosmic Guerilla - 7907A
Cosmic Alien    - 7910
Magical Spot    - 8013
Magical Spot II - 8013
Devil Zone      - 8022

***************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/samples.h"
#include "sound/dac.h"


PALETTE_INIT( panic );
PALETTE_INIT( cosmica );
PALETTE_INIT( cosmicg );
PALETTE_INIT( magspot2 );
PALETTE_INIT( nomnlnd );
VIDEO_UPDATE( panic );
VIDEO_UPDATE( magspot2 );
VIDEO_UPDATE( devzone );
VIDEO_UPDATE( cosmica );
VIDEO_UPDATE( cosmicg );
VIDEO_UPDATE( nomnlnd );
WRITE8_HANDLER( cosmic_color_register_w );
WRITE8_HANDLER( cosmic_background_enable_w );


static unsigned int pixel_clock = 0;



/* Schematics show 12 triggers for discrete sound circuits */

static WRITE8_HANDLER( panic_sound_output_w )
{
	static int sound_enabled=1;

	/* Sound Enable / Disable */

	if (offset == 11)
	{
		int count;
		if (data == 0)
			for(count=0; count<9; count++) sample_stop(count);

		sound_enabled = data;
	}

	if (sound_enabled)
	{
		switch (offset)
		{
		case 0:  if (data) sample_start(0, 0, 0); break;	/* Walk */
		case 1:  if (data) sample_start(0, 5, 0); break;	/* Enemy Die 1 */
		case 2:
			if (data)										/* Drop 1 */
			{
				if (!sample_playing(1))
				{
					sample_stop(2);
					sample_start(1, 3, 0);
				}
			}
			else
				sample_stop(1);
			break;

		case 3:
			if (data && !sample_playing(6))		 		/* Oxygen */
				sample_start(6, 9, 1);
			break;

		case 4:  break;										/* Drop 2 */
		case 5:  if (data) sample_start(0, 5, 0); break;	/* Enemy Die 2 (use same sample as 1) */
		case 6:
			if (data && !sample_playing(1) && !sample_playing(3))	/* Hang */
				sample_start(2, 2, 0);
			break;

		case 7:
			if (data)										/* Escape */
			{
				sample_stop(2);
				sample_start(3, 4, 0);
			}
			else
				sample_stop(3);
			break;

		case 8:  if (data) sample_start(0, 1, 0); break;	/* Stairs */
		case 9:
			if (data)										/* Extend */
				sample_start(4, 8, 0);
			else
				sample_stop(4);
			break;

		case 10: DAC_data_w(0, data); break;				/* Bonus */
		case 15: if (data) sample_start(0, 6, 0); break;	/* Player Die */
		case 16: if (data) sample_start(5, 7, 0); break;	/* Enemy Laugh */
		case 17: if (data) sample_start(0, 10, 0); break;	/* Coin - Not triggered by software */
		}
	}

	#ifdef MAME_DEBUG
	logerror("Sound output %x=%x\n",offset,data);
	#endif
}

WRITE8_HANDLER( panic_sound_output2_w )
{
	panic_sound_output_w(offset+15, data);
}

WRITE8_HANDLER( cosmicg_output_w )
{
	static int march_select;
	static int gun_die_select;
	static int sound_enabled;

	/* Sound Enable / Disable */

	if (offset == 12)
	{
		int count;

		sound_enabled = data;
		if (data == 0)
			for(count=0;count<9;count++) sample_stop(count);
	}

	if (sound_enabled)
	{
		switch (offset)
		{
		/* The schematics show a direct link to the sound amp  */
		/* as other cosmic series games, but it never seems to */
		/* be used for anything. It is implemented for sake of */
		/* completness. Maybe it plays a tune if you win ?     */
		case 1:  DAC_data_w(0, -data); break;
		case 2:  if (data) sample_start (0, march_select, 0); break;	/* March Sound */
		case 3:  march_select = (march_select & 0xfe) | data; break;
		case 4:  march_select = (march_select & 0xfd) | (data << 1); break;
		case 5:  march_select = (march_select & 0xfb) | (data << 2); break;

		case 6:
			if (data)							/* Killer Attack (crawly thing at bottom of screen) */
				sample_start(1, 8, 1);
			else
				sample_stop(1);
			break;

		case 7:
			if (data)								/* Bonus Chance & Got Bonus */
			{
				sample_stop(4);
				sample_start(4, 10, 0);
			}
			break;

		case 8:
			if (data)
			{
				if (!sample_playing(4)) sample_start(4, 9, 1);
			}
			else
				sample_stop(4);
			break;

		case 9:  if (data) sample_start(3, 11, 0); break;	/* Got Ship */
//		case 11: watchdog_reset_w(0, 0); break;			/* Watchdog */
		case 13: if (data) sample_start(8, 13-gun_die_select, 0); break;  /* Got Monster / Gunshot */
		case 14: gun_die_select = data; break;
		case 15: if (data) sample_start(5, 14, 0); break;	/* Coin Extend (extra base) */
		}
	}

	#ifdef MAME_DEBUG
	if (offset != 11) logerror("Output %x=%x\n",offset,data);
	#endif
}

static WRITE8_HANDLER( cosmica_sound_output_w )
{
	static int  sound_enabled=1;
	static int dive_bomb_b_select=0;

	/* Sound Enable / Disable */
	if (offset == 11)
	{
		int count;
		if (data == 0)
			for(count=0; count<13; count++) sample_stop(count);
		else
		{
			sample_start(0, 0, 1); /*Background Noise*/
		}

		sound_enabled = data;
	}

	if (sound_enabled)
	{
		switch (offset)
		{
		case 0:  if (data) sample_start(1, 2, 0); break; /*Dive Bombing Type A*/

		case 2:	 /*Dive Bombing Type B (Main Control)*/
			if (data)
			{
				switch(dive_bomb_b_select)
				{
				case 2:
					if (sample_playing(2))
						sample_stop(2);
					sample_start(2, 3, 0);
					break;

				case 3:
					if (sample_playing(3))
						sample_stop(3);
					sample_start(3, 4, 0);
					break;

				case 4:
					if (sample_playing(4))
						sample_stop(4);
					sample_start(4, 5, 0);
					break;

				case 5:
					if (sample_playing(5))
						sample_stop(5);
					sample_start(5, 6, 0);
					break;

				case 6:
					if (sample_playing(6))
						sample_stop(6);
					sample_start(6, 7, 0);
					break;

				case 7:
					if (sample_playing(7))
						sample_stop(7);
					sample_start(7, 8, 0);
					break;
				}
			}
			break;

		case 3:  /*Dive Bombing Type B (G.S.B)*/
			if (data)
				dive_bomb_b_select |= 0x04;
			else
				dive_bomb_b_select &= 0xFB;
			break;

		case 4:  /*Dive Bombing Type B (M.S.B)*/
			if (data)
				dive_bomb_b_select |= 0x02;
			else
				dive_bomb_b_select &= 0xFD;

			break;

		case 5:  /*Dive Bombing Type B (L.S.B)*/
			if (data)
				dive_bomb_b_select |= 0x01;
			else
				dive_bomb_b_select &= 0xFE;
			break;

		case 6:  if (data) sample_start(8, 9, 0); break; /*Fire Control*/

		case 7:  if (data) sample_start(9, 10, 0); break; /*Small Explosion*/

		case 8:  if (data) sample_start(10, 11, 0); break; /*Loud Explosion*/

		case 9:  /*Extend Sound control*/
			if (data)
				sample_start(11, 1, 1);
			else
				sample_stop(11);
			break;

		case 12:
			if (data) sample_start(11,12, 0); break; /*Insert Coin*/
		}
	}

	#ifdef MAME_DEBUG
	logerror("Sound output %x=%x\n",offset,data);
	#endif
}

static INTERRUPT_GEN( panic_interrupt )
{
	if (cpu_getiloops() != 0)
	{
		/* Coin insert - Trigger Sample */

		/* mostly not noticed since sound is */
		/* only enabled if game in progress! */

		if ((input_port_3_r(0) & 0xc0) != 0xc0)
			panic_sound_output_w(17,1);

		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xcf);	/* RST 08h */
	}
	else
	{
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0xd7);	/* RST 10h */
	}
}

static INTERRUPT_GEN( cosmica_interrupt )
{
	pixel_clock = (pixel_clock + 2) & 0x3f;

	if (pixel_clock == 0)
	{
		if (readinputport(3) & 1)	/* Left Coin */
			cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static INTERRUPT_GEN( cosmicg_interrupt )
{
	/* Insert Coin */

	/* R Nabet : fixed to make this piece of code sensible.
	I assumed that the interrupt request lasted for as long as the coin was "sensed".
	It makes sense and works fine, but I cannot be 100% sure this is correct,
	as I have no Cosmic Guerilla console :-) . */

	if ((readinputport(2) & 1)) /* Coin */
	{
		/* on tms9980, a 6 on the interrupt bus means level 4 interrupt */
		cpunum_set_input_line_and_vector(0, 0, ASSERT_LINE, 6);
	}
	else
	{
		cpunum_set_input_line(0, 0, CLEAR_LINE);
	}
}

static INTERRUPT_GEN( magspot2_interrupt )
{
	/* Coin 1 causes an IRQ, Coin 2 an NMI */
	if (input_port_4_r(0) & 0x01)
	{
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
	else if (input_port_4_r(0) & 0x02)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static INTERRUPT_GEN( nomnlnd_interrupt )
{
	/* Coin causes an NMI */
	if (input_port_4_r(0) & 0x01)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}



static READ8_HANDLER( cosmica_pixel_clock_r )
{
	return pixel_clock;
}

static READ8_HANDLER( cosmicg_port_0_r )
{
	/* The top four address lines from the CRTC are bits 0-3 */

	return (input_port_0_r(0) & 0xf0) | ((cpu_getscanline() & 0xf0) >> 4);
}

static READ8_HANDLER( magspot2_coinage_dip_r )
{
	return (input_port_5_r(0) & (1 << (7 - offset))) ? 0 : 1;
}


/* Has 8 way joystick, remap combinations to missing directions */

static READ8_HANDLER( nomnlnd_port_0_1_r )
{
	int control;
	int fire = input_port_3_r(0);

	if (offset)
		control = input_port_1_r(0);
	else
		control = input_port_0_r(0);

	/* If firing - stop tank */

	if ((fire & 0xc0) == 0) return 0xff;

	/* set bit according to 8 way direction */

	if ((control & 0x82) == 0 ) return 0xfe;	/* Up & Left */
	if ((control & 0x0a) == 0 ) return 0xfb;	/* Down & Left */
	if ((control & 0x28) == 0 ) return 0xef;	/* Down & Right */
	if ((control & 0xa0) == 0 ) return 0xbf;	/* Up & Right */

	return control;
}



static WRITE8_HANDLER( flip_screen_w )
{
	flip_screen_set(data&0x80);
}


static ADDRESS_MAP_START( panic_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_READ(input_port_0_r) /* IN1 */
	AM_RANGE(0x6801, 0x6801) AM_READ(input_port_1_r) /* IN2 */
	AM_RANGE(0x6802, 0x6802) AM_READ(input_port_2_r) /* DSW */
	AM_RANGE(0x6803, 0x6803) AM_READ(input_port_3_r) /* IN0 */
ADDRESS_MAP_END

static ADDRESS_MAP_START( panic_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x6000, 0x601f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x7000, 0x700b) AM_WRITE(panic_sound_output_w)
	AM_RANGE(0x700c, 0x700e) AM_WRITE(cosmic_color_register_w)
	AM_RANGE(0x700f, 0x700f) AM_WRITE(flip_screen_w)
	AM_RANGE(0x7800, 0x7801) AM_WRITE(panic_sound_output2_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( cosmica_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x5fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x6800, 0x6800) AM_READ(input_port_0_r) /* IN1 */
	AM_RANGE(0x6801, 0x6801) AM_READ(input_port_1_r) /* IN2 */
	AM_RANGE(0x6802, 0x6802) AM_READ(input_port_2_r) /* DSW */
	AM_RANGE(0x6803, 0x6803) AM_READ(cosmica_pixel_clock_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmica_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x5fff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x6000, 0x601f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x7000, 0x700b) AM_WRITE(cosmica_sound_output_w)
	AM_RANGE(0x700c, 0x700d) AM_WRITE(cosmic_color_register_w)
	AM_RANGE(0x700f, 0x700f) AM_WRITE(flip_screen_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( cosmicg_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmicg_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x2000, 0x3fff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmicg_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(cosmicg_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cosmicg_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x15) AM_WRITE(cosmicg_output_w)
	AM_RANGE(0x16, 0x17) AM_WRITE(cosmic_color_register_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( magspot2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x3800, 0x3807) AM_READ(magspot2_coinage_dip_r)
	AM_RANGE(0x5000, 0x5000) AM_READ(input_port_0_r)
	AM_RANGE(0x5001, 0x5001) AM_READ(input_port_1_r)
	AM_RANGE(0x5002, 0x5002) AM_READ(input_port_2_r)
	AM_RANGE(0x5003, 0x5003) AM_READ(input_port_3_r)
	AM_RANGE(0x6000, 0x7fff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( magspot2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x401f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0x4800, 0x4800) AM_WRITE(DAC_0_data_w)
	AM_RANGE(0x480c, 0x480d) AM_WRITE(cosmic_color_register_w)
	AM_RANGE(0x480f, 0x480f) AM_WRITE(flip_screen_w)
	AM_RANGE(0x6000, 0x7fff) AM_WRITE(MWA8_RAM) AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END


INPUT_PORTS_START( panic )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
/* 0x06 and 0x07 disabled */
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_3C ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

INPUT_PORTS_END

INPUT_PORTS_START( cosmica )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
  //PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "5000" )
	PORT_DIPSETTING(    0x20, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	/* The coin slots are not memory mapped.  Coin causes a NMI, */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */

	PORT_START	/* FAKE */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

INPUT_PORTS_END

/* These are used for the CR handling - This can be used to */
/* from 1 to 16 bits from any bit offset between 0 and 4096 */

/* Offsets are in BYTES, so bits 0-7 are at offset 0 etc.   */

INPUT_PORTS_START( cosmicg )
	PORT_START /* 4-7 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_SPECIAL )	/* pixel clock */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY

	PORT_START /* 8-15 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_2WAY PORT_COCKTAIL
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPSETTING(    0x20, "1500" )
	PORT_DIPSETTING(    0x30, "2000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x80, "5" )

	PORT_START      /* Hard wired settings */

	/* The coin slots are not memory mapped. Coin causes INT 4  */
	/* This fake input port is used by the interrupt handler    */
	/* to be notified of coin insertions. */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )

	/* This dip switch is not read by the program at any time   */
	/* but is wired to enable or disable the flip screen output */

	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )

	/* This odd setting is marked as shown on the schematic,    */
	/* and again, is not read by the program, but wired into    */
	/* the watchdog circuit. The book says to leave it off      */

	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( magspot2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_DIPNAME( 0xc0, 0x40, "Bonus Game" )
	PORT_DIPSETTING(    0x40, "5000" )
	PORT_DIPSETTING(    0x80, "10000" )
	PORT_DIPSETTING(    0xc0, "15000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "2000" )
	PORT_DIPSETTING(    0x02, "3000" )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x1e, IP_ACTIVE_LOW, IPT_UNUSED )		/* always HI */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* reads what was written to 4808.  Probably not used?? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coins */
	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)

	/* Fake port to handle coinage dip switches. Each bit goes to 3800-3807 */
	PORT_START	/* IN5 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
INPUT_PORTS_END

INPUT_PORTS_START( devzone )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "4000" )
	PORT_DIPSETTING(    0x02, "6000" )
	PORT_DIPSETTING(    0x03, "8000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0c, "Use Coin A & B" )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coins */
	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START	/* IN5 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
INPUT_PORTS_END


INPUT_PORTS_START( devzone2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "2000" )
	PORT_DIPSETTING(    0x02, "3000" )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0c, "Use Coin A & B" )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coins */
	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START	/* IN5 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
INPUT_PORTS_END


INPUT_PORTS_START( nomnlnd )
	PORT_START	/* Controls - Remapped for game */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x55, IP_ACTIVE_LOW, IPT_SPECIAL )	/* diagonals */

	PORT_START	/* IN1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x55, IP_ACTIVE_LOW, IPT_SPECIAL )	/* diagonals */

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "2000" )
	PORT_DIPSETTING(    0x02, "3000" )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x1e, IP_ACTIVE_LOW, IPT_UNUSED )		/* always HI */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* reads what was written to 4808.  Probably not used?? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coin */
	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
INPUT_PORTS_END


INPUT_PORTS_START( nomnlndg )
	PORT_START	/* Controls - Remapped for game */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x55, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x55, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "3000" )
	PORT_DIPSETTING(    0x02, "5000" )
	PORT_DIPSETTING(    0x03, "8000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
//  PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x1e, IP_ACTIVE_LOW, IPT_UNUSED )		/* always HI */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SPECIAL )	/* reads what was written to 4808.  Probably not used?? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	/* Fake port to handle coin */
	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
INPUT_PORTS_END


static const gfx_layout cosmic_spritelayout16 =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{  0*8+0,  0*8+1,  0*8+2,  0*8+3,  0*8+4,  0*8+5,  0*8+6,  0*8+7,
	  16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_layout cosmic_spritelayout32 =
{
	32,32,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0*32*8+0, 0*32*8+1, 0*32*8+2, 0*32*8+3, 0*32*8+4, 0*32*8+5, 0*32*8+6, 0*32*8+7,
	  1*32*8+0, 1*32*8+1, 1*32*8+2, 1*32*8+3, 1*32*8+4, 1*32*8+5, 1*32*8+6, 1*32*8+7,
	  2*32*8+0, 2*32*8+1, 2*32*8+2, 2*32*8+3, 2*32*8+4, 2*32*8+5, 2*32*8+6, 2*32*8+7,
	  3*32*8+0, 3*32*8+1, 3*32*8+2, 3*32*8+3, 3*32*8+4, 3*32*8+5, 3*32*8+6, 3*32*8+7 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	   8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
	  24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8 },
	128*8
};


static const gfx_decode panic_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &cosmic_spritelayout16,  0, 8 },
	{ REGION_GFX1, 0, &cosmic_spritelayout32,  0, 8 },
	{ -1 } /* end of array */
};

static const gfx_decode cosmica_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &cosmic_spritelayout16,  0, 16 },
	{ REGION_GFX1, 0, &cosmic_spritelayout32,  0, 16 },
	{ -1 } /* end of array */
};


static const char *panic_sample_names[] =
{
	"*panic",
	"walk.wav",
	"upordown.wav",
	"trapped.wav",
	"falling.wav",
	"escaping.wav",
	"ekilled.wav",
	"death.wav",
	"elaugh.wav",
	"extral.wav",
	"oxygen.wav",
	"coin.wav",
	0		/* end of array */
};

static struct Samplesinterface panic_samples_interface =
{
	9,	/* 9 channels */
	panic_sample_names
};

static const char *cosmicg_sample_names[] =
{
	"*cosmicg",
	"cg_m0.wav",	/* 8 Different pitches of March Sound */
	"cg_m1.wav",
	"cg_m2.wav",
	"cg_m3.wav",
	"cg_m4.wav",
	"cg_m5.wav",
	"cg_m6.wav",
	"cg_m7.wav",
	"cg_att.wav",	/* Killer Attack */
	"cg_chnc.wav",	/* Bonus Chance  */
	"cg_gotb.wav",	/* Got Bonus - have not got correct sound for */
	"cg_dest.wav",	/* Gun Destroy */
	"cg_gun.wav",	/* Gun Shot */
	"cg_gotm.wav",	/* Got Monster */
	"cg_ext.wav",	/* Coin Extend */
	0		/* end of array */
};

static struct Samplesinterface cosmicg_samples_interface =
{
	9,	/* 9 channels */
	cosmicg_sample_names
};

static const char *cosmica_sample_names[] =
{
	"*cosmica",
	"backgr.wav",
	"extend.wav",
	"divea.wav",
	"diveb1.wav",
	"diveb2.wav",
	"diveb3.wav",
	"diveb4.wav",
	"diveb5.wav",
	"diveb6.wav",
	"fire.wav",
	"loudexp.wav",
	"smallexp.wav",
	"coin.wav",
	0		/* end of array */
};

static struct Samplesinterface cosmica_samples_interface =
{
	13,	/* 12 channels */
	cosmica_sample_names
};

static MACHINE_DRIVER_START( cosmic )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80,10816000/6)	/* 1.802 MHz*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 28*8-1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( panic )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(cosmic)
	MDRV_CPU_MODIFY("main")

	MDRV_CPU_PROGRAM_MAP(panic_readmem,panic_writemem)
	MDRV_CPU_VBLANK_INT(panic_interrupt,2)

	/* video hardware */
	MDRV_GFXDECODE(panic_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(8*4)

	MDRV_PALETTE_INIT(panic)
	MDRV_VIDEO_UPDATE(panic)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(panic_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cosmica )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(cosmic)
	MDRV_CPU_MODIFY("main")

	MDRV_CPU_PROGRAM_MAP(cosmica_readmem,cosmica_writemem)
	MDRV_CPU_VBLANK_INT(cosmica_interrupt,32)

	/* video hardware */
	MDRV_GFXDECODE(cosmica_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(16*4)

	MDRV_PALETTE_INIT(cosmica)
	MDRV_VIDEO_UPDATE(cosmica)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(cosmica_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cosmicg )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS9980, 1228500)
	/* 9.828 MHz Crystal */
	/* R Nabet : huh ? This would imply the crystal frequency is somehow divided by 2 before being
		fed to the tms9904 or tms9980.  Also, I have never heard of a tms9900/9980 operating under
		1.5MHz.  So, if someone can check this... */
	MDRV_CPU_PROGRAM_MAP(cosmicg_readmem,cosmicg_writemem)
	MDRV_CPU_IO_MAP(cosmicg_readport,cosmicg_writeport)
	MDRV_CPU_VBLANK_INT(cosmicg_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 4*8, 28*8-1)
	MDRV_PALETTE_LENGTH(16)

	MDRV_PALETTE_INIT(cosmicg)
	MDRV_VIDEO_UPDATE(cosmicg)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(cosmicg_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( magspot2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(cosmic)
	MDRV_CPU_MODIFY("main")

	MDRV_CPU_PROGRAM_MAP(magspot2_readmem,magspot2_writemem)
	MDRV_CPU_VBLANK_INT(magspot2_interrupt,1)

	/* video hardware */
	MDRV_GFXDECODE(panic_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(8*4)

	MDRV_PALETTE_INIT(magspot2)
	MDRV_VIDEO_UPDATE(magspot2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( devzone )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(magspot2)

	/* video hardware */
	MDRV_VIDEO_UPDATE(devzone)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nomnlnd )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(cosmic)
	MDRV_CPU_MODIFY("main")

	MDRV_CPU_PROGRAM_MAP(magspot2_readmem,magspot2_writemem)
	MDRV_CPU_VBLANK_INT(nomnlnd_interrupt,1)

	/* video hardware */
	MDRV_GFXDECODE(panic_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(8*4)

	MDRV_PALETTE_INIT(nomnlnd)
	MDRV_VIDEO_UPDATE(nomnlnd)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( panic )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "spe1",         0x0000, 0x0800, CRC(70ac0888) SHA1(bdc6dfb74b4643df36cae60923f9759751340c86) )
	ROM_LOAD( "spe2",         0x0800, 0x0800, CRC(2b910c48) SHA1(9ebb15694e068a4d8769ec5d312af1148818d472) )
	ROM_LOAD( "spe3",         0x1000, 0x0800, CRC(03810148) SHA1(768418bc0a3a5bc9f7ec07b8edd4099da69efac6) )
	ROM_LOAD( "spe4",         0x1800, 0x0800, CRC(119bbbfd) SHA1(2b3722300b1eebe1bffa4a4e39fceb45aefde24f) )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, CRC(5b80f277) SHA1(b060e57c88679f547153aed041a5554dc26a83aa) )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, CRC(b73babf0) SHA1(229944a6b3653601bc20afea5a9aec787fd95ce0) )
	ROM_LOAD( "spe7",         0x3000, 0x0800, CRC(2894106e) SHA1(625896225b0ec03ac12f3e8b97e801cb743f37e7) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spcpanic.11",  0x0000, 0x0800, CRC(acea9df4) SHA1(7de2a82da8160ad1a01c32a516d10c19dc306051) )
	ROM_LOAD( "spcpanic.12",  0x0800, 0x0800, CRC(e83423d0) SHA1(eba1129537869f1ecb5afeeae19db19b134865f6) )
	ROM_LOAD( "spcpanic.10",  0x1000, 0x0800, CRC(c9631c2d) SHA1(e5ab95e19c1b22a798a70a1a6599bc1f5e853c60) )
	ROM_LOAD( "spcpanic.9",   0x1800, 0x0800, CRC(eec78b4c) SHA1(efd21d0a26b988a490c45315a7a121607f74d147) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.sp",    0x0000, 0x0020, CRC(35d43d2f) SHA1(2ce164c92ed7ba3ee26a907f0c5969ec3decca01) )

	ROM_REGION( 0x0800, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "spcpanic.8",   0x0000, 0x0800, CRC(7da0b321) SHA1(b450cc02de9cc27e3f336c626221c90c6961b51e) )
ROM_END

ROM_START( panic2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "spcpanic.1",   0x0000, 0x0800, CRC(405ae6f9) SHA1(92000f5f9bc1384ebae36dd30e715764747504d8) )
	ROM_LOAD( "spcpanic.2",   0x0800, 0x0800, CRC(b6a286c5) SHA1(b33beb1fbe622e9c90888d25d018fd5bef6cb65b) )
	ROM_LOAD( "spcpanic.3",   0x1000, 0x0800, CRC(85ae8b2e) SHA1(a5676d38e3c0ea0aeedc29bea0c04086e51da67f) )
	ROM_LOAD( "spcpanic.4",   0x1800, 0x0800, CRC(b6d4f52f) SHA1(431e5ef00768a633d17449a888ac9ce46975272d) )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, CRC(5b80f277) SHA1(b060e57c88679f547153aed041a5554dc26a83aa) )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, CRC(b73babf0) SHA1(229944a6b3653601bc20afea5a9aec787fd95ce0) )
	ROM_LOAD( "spcpanic.7",   0x3000, 0x0800, CRC(fc27f4e5) SHA1(80064ccfb810d11f6d7d79bfd991adb2eb2f1c16) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spcpanic.11",  0x0000, 0x0800, CRC(acea9df4) SHA1(7de2a82da8160ad1a01c32a516d10c19dc306051) )
	ROM_LOAD( "spcpanic.12",  0x0800, 0x0800, CRC(e83423d0) SHA1(eba1129537869f1ecb5afeeae19db19b134865f6) )
	ROM_LOAD( "spcpanic.10",  0x1000, 0x0800, CRC(c9631c2d) SHA1(e5ab95e19c1b22a798a70a1a6599bc1f5e853c60) )
	ROM_LOAD( "spcpanic.9",   0x1800, 0x0800, CRC(eec78b4c) SHA1(efd21d0a26b988a490c45315a7a121607f74d147) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.sp",    0x0000, 0x0020, CRC(35d43d2f) SHA1(2ce164c92ed7ba3ee26a907f0c5969ec3decca01) )

	ROM_REGION( 0x0800, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "spcpanic.8",   0x0000, 0x0800, CRC(7da0b321) SHA1(b450cc02de9cc27e3f336c626221c90c6961b51e) )
ROM_END

ROM_START( panic3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "panica.1",     0x0000, 0x0800, CRC(289720ce) SHA1(8601bda95ac32a55f17fe9c723796bfe8b2b2fa7) )
	ROM_LOAD( "spcpanic.2",   0x0800, 0x0800, CRC(b6a286c5) SHA1(b33beb1fbe622e9c90888d25d018fd5bef6cb65b) )
	ROM_LOAD( "spcpanic.3",   0x1000, 0x0800, CRC(85ae8b2e) SHA1(a5676d38e3c0ea0aeedc29bea0c04086e51da67f) )
	ROM_LOAD( "spcpanic.4",   0x1800, 0x0800, CRC(b6d4f52f) SHA1(431e5ef00768a633d17449a888ac9ce46975272d) )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, CRC(5b80f277) SHA1(b060e57c88679f547153aed041a5554dc26a83aa) )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, CRC(b73babf0) SHA1(229944a6b3653601bc20afea5a9aec787fd95ce0) )
	ROM_LOAD( "panica.7",     0x3000, 0x0800, CRC(3641cb7f) SHA1(94a5108233cf9517f782759bb396e4eab58b8551) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spcpanic.11",  0x0000, 0x0800, CRC(acea9df4) SHA1(7de2a82da8160ad1a01c32a516d10c19dc306051) )
	ROM_LOAD( "spcpanic.12",  0x0800, 0x0800, CRC(e83423d0) SHA1(eba1129537869f1ecb5afeeae19db19b134865f6) )
	ROM_LOAD( "spcpanic.10",  0x1000, 0x0800, CRC(c9631c2d) SHA1(e5ab95e19c1b22a798a70a1a6599bc1f5e853c60) )
	ROM_LOAD( "spcpanic.9",   0x1800, 0x0800, CRC(eec78b4c) SHA1(efd21d0a26b988a490c45315a7a121607f74d147) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.sp",    0x0000, 0x0020, CRC(35d43d2f) SHA1(2ce164c92ed7ba3ee26a907f0c5969ec3decca01) )

	ROM_REGION( 0x0800, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "spcpanic.8",   0x0000, 0x0800, CRC(7da0b321) SHA1(b450cc02de9cc27e3f336c626221c90c6961b51e) )
ROM_END

ROM_START( panich )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sph1",         0x0000, 0x0800, CRC(f6e9c6ef) SHA1(90b5bba0fd726e4c6618793467eba8c18c63fd43) )
	ROM_LOAD( "sph2",         0x0800, 0x0800, CRC(58dbc49b) SHA1(f716e8cdbb7eb456bd7f2996241b5ebd03086de3) )
	ROM_LOAD( "sph3",         0x1000, 0x0800, CRC(c4f275ad) SHA1(446be24dc99e46f3c69cf2cfb657958053857b7d) )
	ROM_LOAD( "sph4",         0x1800, 0x0800, CRC(6e7785de) SHA1(d12326791cbcae37980e240e2bfc20d7618f3ef5) )
	ROM_LOAD( "sph5",         0x2000, 0x0800, CRC(1916c9b8) SHA1(ab4a353340f152d6ba181555ee211afeb7877509) )
	ROM_LOAD( "sph6",         0x2800, 0x0800, CRC(54b92314) SHA1(970ebae831ea0a1958b8d711ebc5956ef4f932fe) )
	ROM_LOAD( "sph7",         0x3000, 0x0800, CRC(8600b881) SHA1(2eed176de531f44d10b7755141621050d72ad7ac) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spcpanic.11",  0x0000, 0x0800, CRC(acea9df4) SHA1(7de2a82da8160ad1a01c32a516d10c19dc306051) )
	ROM_LOAD( "spcpanic.12",  0x0800, 0x0800, CRC(e83423d0) SHA1(eba1129537869f1ecb5afeeae19db19b134865f6) )
	ROM_LOAD( "spcpanic.10",  0x1000, 0x0800, CRC(c9631c2d) SHA1(e5ab95e19c1b22a798a70a1a6599bc1f5e853c60) )
	ROM_LOAD( "spcpanic.9",   0x1800, 0x0800, CRC(eec78b4c) SHA1(efd21d0a26b988a490c45315a7a121607f74d147) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.sp",    0x0000, 0x0020, CRC(35d43d2f) SHA1(2ce164c92ed7ba3ee26a907f0c5969ec3decca01) )

	ROM_REGION( 0x0800, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "spcpanic.8",   0x0000, 0x0800, CRC(7da0b321) SHA1(b450cc02de9cc27e3f336c626221c90c6961b51e) )
ROM_END

ROM_START( panicger )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "spacepan.001", 0x0000, 0x0800, CRC(a6d9515a) SHA1(20fe6fa4cb10e83f97b77e19d9d4f883aba73d1a) )
	ROM_LOAD( "spacepan.002", 0x0800, 0x0800, CRC(cfc22663) SHA1(44036a69ca3463759c56637c3435a3305b102879) )
	ROM_LOAD( "spacepan.003", 0x1000, 0x0800, CRC(e1f36893) SHA1(689b77b4df15dc980d35cf245aca1affe46d6b21) )
	ROM_LOAD( "spacepan.004", 0x1800, 0x0800, CRC(01be297c) SHA1(d22856ef192d8239a3520f16bbe5a6f7f4c3adc8) )
	ROM_LOAD( "spacepan.005", 0x2000, 0x0800, CRC(e0d54805) SHA1(5852f69cee9a8f9984b175268bcfafe4f3f124ba) )
	ROM_LOAD( "spacepan.006", 0x2800, 0x0800, CRC(aae1458e) SHA1(79dd5992b81f316cf86efdb2809b7002e824e0e7) )
	ROM_LOAD( "spacepan.007", 0x3000, 0x0800, CRC(14e46e70) SHA1(f49f09a12b796f7a7713d872ecd12e246c56c261) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spcpanic.11",  0x0000, 0x0800, CRC(acea9df4) SHA1(7de2a82da8160ad1a01c32a516d10c19dc306051) )
	ROM_LOAD( "spcpanic.12",  0x0800, 0x0800, CRC(e83423d0) SHA1(eba1129537869f1ecb5afeeae19db19b134865f6) )
	ROM_LOAD( "spcpanic.10",  0x1000, 0x0800, CRC(c9631c2d) SHA1(e5ab95e19c1b22a798a70a1a6599bc1f5e853c60) )
	ROM_LOAD( "spcpanic.9",   0x1800, 0x0800, CRC(eec78b4c) SHA1(efd21d0a26b988a490c45315a7a121607f74d147) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.sp",    0x0000, 0x0020, CRC(35d43d2f) SHA1(2ce164c92ed7ba3ee26a907f0c5969ec3decca01) )

	ROM_REGION( 0x0800, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "spcpanic.8",   0x0000, 0x0800, CRC(7da0b321) SHA1(b450cc02de9cc27e3f336c626221c90c6961b51e) )
ROM_END

ROM_START( cosmica )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ca.e3",        0x0000, 0x0800, CRC(535ee0c5) SHA1(3ec3056b7fabe07ef49a9179114aa74be44a943e) )
	ROM_LOAD( "ca.e4",        0x0800, 0x0800, CRC(ed3cf8f7) SHA1(6ba1d98d82400519e844b950cb2fb1274c06d89a) )
	ROM_LOAD( "ca.e5",        0x1000, 0x0800, CRC(6a111e5e) SHA1(593be409bc969cece2ff88623e53c166b4dc43cd) )
	ROM_LOAD( "ca.e6",        0x1800, 0x0800, CRC(c9b5ca2a) SHA1(3384b98954b6bc9a64e753b95757f61ce1d3c52e) )
	ROM_LOAD( "ca.e7",        0x2000, 0x0800, CRC(43666d68) SHA1(e44492360a77d93aeaaaa0f38f4ac19732998559) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "ca.n2",        0x0000, 0x0800, CRC(aa6c6079) SHA1(af4ab73e9e1c189290b26bf42adb511d5a347df9) )
	ROM_LOAD( "ca.n1",        0x0800, 0x0800, CRC(431e866c) SHA1(b007cd3cc856360a0247bd78bb49d173f5cef321) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ca.d9",        0x0000, 0x0020, CRC(dfb60f19) SHA1(d510327ff3492f098659c551f7245835f61a2959) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "ca.e2",        0x0000, 0x0400, CRC(ea4ee931) SHA1(d0a4afda4b493efb40286c2d67bf56a2a8b8da9d) )

	ROM_REGION( 0x0400, REGION_USER2, 0 ) /* starfield generator */
	ROM_LOAD( "ca.sub",       0x0000, 0x0400, CRC(acbd4e98) SHA1(d33fe8bdc77bb18a3ffb369ea692210d1b890771) )
ROM_END

ROM_START( cosmica2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ca.e3",        0x0000, 0x0800, CRC(535ee0c5) SHA1(3ec3056b7fabe07ef49a9179114aa74be44a943e) )
	ROM_LOAD( "c3.bin",       0x0800, 0x0400, CRC(699c849e) SHA1(90a58ab8ede9c31eec3df1f8f251b59858f85eb6) )
	ROM_LOAD( "d4.bin",       0x0c00, 0x0400, CRC(168e38da) SHA1(63c5f8346861aa7c70ad58a05977c7af413cbfaf) )
	ROM_LOAD( "ca.e5",        0x1000, 0x0800, CRC(6a111e5e) SHA1(593be409bc969cece2ff88623e53c166b4dc43cd) )
	ROM_LOAD( "ca.e6",        0x1800, 0x0800, CRC(c9b5ca2a) SHA1(3384b98954b6bc9a64e753b95757f61ce1d3c52e) )
	ROM_LOAD( "i9.bin",       0x2000, 0x0400, CRC(3bb57720) SHA1(2d1edcad57767a4fa2c7713726ed0cb1203f6fbc) )
	ROM_LOAD( "j0.bin",       0x2400, 0x0400, CRC(4ff70f45) SHA1(791499be62a7b91bde75e7a7ab6c546f5fb63027) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "ca.n2",        0x0000, 0x0800, CRC(aa6c6079) SHA1(af4ab73e9e1c189290b26bf42adb511d5a347df9) )
	ROM_LOAD( "ca.n1",        0x0800, 0x0800, CRC(431e866c) SHA1(b007cd3cc856360a0247bd78bb49d173f5cef321) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ca.d9",        0x0000, 0x0020, CRC(dfb60f19) SHA1(d510327ff3492f098659c551f7245835f61a2959) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "ca.e2",        0x0000, 0x0400, CRC(ea4ee931) SHA1(d0a4afda4b493efb40286c2d67bf56a2a8b8da9d) )

	ROM_REGION( 0x0400, REGION_USER2, 0 ) /* starfield generator */
	ROM_LOAD( "ca.sub",       0x0000, 0x0400, CRC(acbd4e98) SHA1(d33fe8bdc77bb18a3ffb369ea692210d1b890771) )
ROM_END

ROM_START( cosmicg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )  /* 8k for code */
	ROM_LOAD( "cosmicg1.bin", 0x0000, 0x0400, CRC(e1b9f894) SHA1(bab7fd9b3db145a889542653191905b6efc5ce75) )
	ROM_LOAD( "cosmicg2.bin", 0x0400, 0x0400, CRC(35c75346) SHA1(4e50eaa0b50ab04802dc63992ad2600c227301ad) )
	ROM_LOAD( "cosmicg3.bin", 0x0800, 0x0400, CRC(82a49b48) SHA1(4cf9f684f3eb18b99a88ca879bb7083b1334f0cc) )
	ROM_LOAD( "cosmicg4.bin", 0x0c00, 0x0400, CRC(1c1c934c) SHA1(011b2b3ec4d31869fda13a3654c7bc51f3ce4dc2) )
	ROM_LOAD( "cosmicg5.bin", 0x1000, 0x0400, CRC(b1c00fbf) SHA1(136267f75e2d5b445695cabef4538f986e6f1b10) )
	ROM_LOAD( "cosmicg6.bin", 0x1400, 0x0400, CRC(f03454ce) SHA1(32c87f369475c7154fe3243d2c7be4a25444e530) )
	ROM_LOAD( "cosmicg7.bin", 0x1800, 0x0400, CRC(f33ebae7) SHA1(915bca53d5356e12c94ec765103ceced7306d1dd) )
	ROM_LOAD( "cosmicg8.bin", 0x1c00, 0x0400, CRC(472e4990) SHA1(d5797b9d89446aa6533f7515e6a5fc8368d82f91) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "cosmicg9.bin", 0x0000, 0x0400, CRC(689c2c96) SHA1(ddfdc3fd29c56fdebd3b1c3443a7c39f567d5355) )
ROM_END

/* rom 9 not dumped according to readme? */
ROM_START( magspot )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ms1.bin",      0x0000, 0x0800, CRC(59e9019d) SHA1(3c64ae956ec4eed988018b89c986ad8f6f065fe0) )
	ROM_LOAD( "ms2.bin",      0x0800, 0x0800, CRC(98b913b1) SHA1(2ce86f5069e2664e2ea44bda567ca26432fd59f7) )
	ROM_LOAD( "ms3.bin",      0x1000, 0x0800, CRC(ea58c124) SHA1(7551c14ed9563e3aed7220cc03f7bca4029b3a4e) )
	ROM_LOAD( "ms5.bin",      0x1800, 0x0800, CRC(4302a658) SHA1(9590be8db27b7122c87cfb27f8e09c2ecbf6fbd0) )
	ROM_LOAD( "ms4.bin",      0x2000, 0x0800, CRC(088582ab) SHA1(ad2d86184b4a6ee74464d1df40f4e841434c46c8) )
	ROM_LOAD( "ms6.bin",      0x2800, 0x0800, CRC(e6bf492c) SHA1(ada3a33c54b6c02f3fb9590181fceefafdc429bc) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "ms8.bin",      0x0000, 0x0800, CRC(9e1d63a2) SHA1(d8642e515871da44880e105e6891c4b25222744f) )
	ROM_LOAD( "ms7.bin",      0x0800, 0x0800, CRC(1ab338d3) SHA1(4e3bf93f94119fd10c40953245cec735db8417fb) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ms.d9",        0x0000, 0x0020, CRC(36e2aa2a) SHA1(4813b013cb8260157858e3adc7323efc6654e170) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "ms.e2",        0x0000, 0x0400, CRC(89f23ebd) SHA1(a56bda82f8be8e541a50d2a411ada89a6d9c0373) )
ROM_END

ROM_START( magspot2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ms.e3",        0x0000, 0x0800, CRC(c0085ade) SHA1(ab60ba7c0e45ea2576d935135e930e2fdf165867) )
	ROM_LOAD( "ms.e4",        0x0800, 0x0800, CRC(d534a68b) SHA1(fd3b5e619b22a8c53e3c6f5f5351068a3f26eb61) )
	ROM_LOAD( "ms.e5",        0x1000, 0x0800, CRC(25513b2a) SHA1(c7f3d9a53cb7e7cf523ff710c333dbc744088e31) )
	ROM_LOAD( "ms.e7",        0x1800, 0x0800, CRC(8836bbc4) SHA1(9da6c1b4e9a446108bc324e7fc280bfaeaf50504) )
	ROM_LOAD( "ms.e6",        0x2000, 0x0800, CRC(6a08ab94) SHA1(5d9272a5304546cef6668c975e815f6750bcfa15) )
	ROM_LOAD( "ms.e8",        0x2800, 0x0800, CRC(77c6d109) SHA1(bb265bd56d4d597d2ef75d169d5d30db1499e3be) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "ms.n2",        0x0000, 0x0800, CRC(9e1d63a2) SHA1(d8642e515871da44880e105e6891c4b25222744f) )
	ROM_LOAD( "ms.n1",        0x0800, 0x0800, CRC(1ab338d3) SHA1(4e3bf93f94119fd10c40953245cec735db8417fb) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ms.d9",        0x0000, 0x0020, CRC(36e2aa2a) SHA1(4813b013cb8260157858e3adc7323efc6654e170) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "ms.e2",        0x0000, 0x0400, CRC(89f23ebd) SHA1(a56bda82f8be8e541a50d2a411ada89a6d9c0373) )
ROM_END

ROM_START( devzone )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "dv1.e3",       0x0000, 0x0800, CRC(c70faf00) SHA1(d3f0f071e6c7552724eba64a7182637dae4438c7) )
	ROM_LOAD( "dv2.e4",       0x0800, 0x0800, CRC(eacfed61) SHA1(493c0d21fd1574b12978dd1f52e8735df6c1732c) )
	ROM_LOAD( "dv3.e5",       0x1000, 0x0800, CRC(7973317e) SHA1(d236e3dad8c991c32a2550e561518b522a4580bc) )
	ROM_LOAD( "dv5.e7",       0x1800, 0x0800, CRC(b71a3989) SHA1(aad14021ee569e221ea632416d6a006e60dd94e5) )
	ROM_LOAD( "dv4.e6",       0x2000, 0x0800, CRC(a58c5b8c) SHA1(7ff08007aedd2ff1d7ef64263da92a5b77ae2dc4) )
	ROM_LOAD( "dv6.e8",       0x2800, 0x0800, CRC(3930fb67) SHA1(919883e833d6caa8fe7c3ceaa184575a3b4932b6) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "dv8.n2",       0x0000, 0x0800, CRC(da1cbec1) SHA1(08a668f19c68335f4fc9f98cd53b44047dd8aad9) )
	ROM_LOAD( "dv7.n1",       0x0800, 0x0800, CRC(e7562fcf) SHA1(0a0833dbb8d4be69fbf8897aa3e045a87ae42024) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ms.d9",        0x0000, 0x0020, CRC(36e2aa2a) SHA1(4813b013cb8260157858e3adc7323efc6654e170) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "dz9.e2",       0x0000, 0x0400, CRC(693855b6) SHA1(1c29d72be511c1d38b30b9534d647d0813b2ef57) )

	ROM_REGION( 0x0800, REGION_USER2, 0 ) /* grid horizontal line positions */
	ROM_LOAD( "ic12.sub",     0x0000, 0x0800, CRC(f61c1c45) SHA1(9016710409ae2bccfc60f8e3d1131c125333c034) )

	ROM_REGION( 0x0020, REGION_USER3, 0 ) /* grid vertical line positions */
	ROM_LOAD( "ic1.sub",      0x0000, 0x0020, CRC(df974878) SHA1(2ef2e1b771923f9a0bfe1841444de61200298605) )
ROM_END

ROM_START( devzone2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p10_1.e3",     0x0000, 0x0800, BAD_DUMP CRC(38bd45a4) SHA1(192eee64ff53c20fb5b369703b52a5bb3976ba1d)  )
	ROM_LOAD( "my4_2.e4",     0x0800, 0x0800, BAD_DUMP CRC(e1637800) SHA1(3705ce1f02f3fefec0285f5db6a7606e6cec1bac)  )
	ROM_LOAD( "ms6_3.e5",     0x1000, 0x0800, BAD_DUMP CRC(c1952e2f) SHA1(d42f0f547e989a71254957e5e634ac359e72bb14)  )
	ROM_LOAD( "mx6_5.e7",     0x1800, 0x0800, BAD_DUMP CRC(c5394215) SHA1(8c970f6a8d34963bc4848f2bef90cee850c9c28d)  )
	ROM_LOAD( "my1_4.e6",     0x2000, 0x0800, BAD_DUMP CRC(5d965d93) SHA1(49fe79e4b5cec1c7aa2f8e1eb750b39bb7dda16c)  )
	ROM_LOAD( "mz7_6.e8",     0x2800, 0x0800, BAD_DUMP CRC(8504e8c9) SHA1(40e08ff38673544c734a9fc19b38edaa8cc74f23)  )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "my8_8.n2",     0x0000, 0x0800, CRC(18abce02) SHA1(5cac11f4e6f1a4801bd02007399a906cdff66b85) )
	ROM_LOAD( "mx3_7.n1",     0x0800, 0x0800, CRC(c089c9e3) SHA1(2fb725338a19d5d4f9e445e7d46d105b8db9733c) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "ms.d9",        0x0000, 0x0020, CRC(36e2aa2a) SHA1(4813b013cb8260157858e3adc7323efc6654e170) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "dz9.e2",       0x0000, 0x0400, CRC(693855b6) SHA1(1c29d72be511c1d38b30b9534d647d0813b2ef57) )

	ROM_REGION( 0x0800, REGION_USER2, 0 ) /* grid horizontal line positions */
	ROM_LOAD( "ic12.sub",     0x0000, 0x0800, CRC(f61c1c45) SHA1(9016710409ae2bccfc60f8e3d1131c125333c034) )

	ROM_REGION( 0x0020, REGION_USER3, 0 ) /* grid vertical line positions */
	ROM_LOAD( "ic1.sub",      0x0000, 0x0020, CRC(df974878) SHA1(2ef2e1b771923f9a0bfe1841444de61200298605) )
ROM_END

ROM_START( nomnlnd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.bin",        0x0000, 0x0800, CRC(ba117ba6) SHA1(7399e7ac8a585ed6502ea0d740850b1ed2dc5bcd) )
	ROM_LOAD( "2.bin",        0x0800, 0x0800, CRC(e5ed654f) SHA1(c26dc12ade6dc63392945ec0caca229d936f7f89) )
	ROM_LOAD( "3.bin",        0x1000, 0x0800, CRC(7fc42724) SHA1(0f8fdfad0a2557b9dd99ae3890c37bbc5c59bc89) )
	ROM_LOAD( "5.bin",        0x1800, 0x0800, CRC(9cc2f1d9) SHA1(453c67b613550c84364f445705019188bb580d64) )
	ROM_LOAD( "4.bin",        0x2000, 0x0800, CRC(0e8cd46a) SHA1(14cf9017e408b862a4ed63bb8acd37064b3919a8) )
	ROM_LOAD( "6.bin",        0x2800, 0x0800, CRC(ba472ba5) SHA1(49be1500b3805a19c7210e53ad5c2c4a5876bf4e) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "nml8.n2",      0x0000, 0x0800, CRC(739009b4) SHA1(bbabd6ce7b1ded025f20120adaebdb97fb755ef0) )
	ROM_LOAD( "nml7.n1",      0x0800, 0x0800, CRC(d08ed22f) SHA1(33f450b6f63110bf804105280dc679f1591422f6) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "nml.d9",       0x0000, 0x0020, CRC(65e911f9) SHA1(6420a03195f63edeed17cc3a235e46e3f88d2037) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "nl9.e2",       0x0000, 0x0400, CRC(9e05f14e) SHA1(76fc0b2b12cc9a0a64b539d2e75edefdb4a2ae61) )

	ROM_REGION( 0x0800, REGION_USER2, 0 ) /* tree + river */
	ROM_LOAD( "nl10.ic4",     0x0000, 0x0400, CRC(5b13f64e) SHA1(b04d2423fb443d46fff69c031b0312d956a5b789) )
	ROM_LOAD( "nl11.ic7",     0x0400, 0x0400, CRC(e717b241) SHA1(6d234a75514e22d484dc027db5bb85cf8b58f4f2) )
ROM_END

ROM_START( nomnlndg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "nml1.e3",      0x0000, 0x0800, CRC(e212ed91) SHA1(135c20fc97790769d5e1619d7ac844a1d3f6aace) )
	ROM_LOAD( "nml2.e4",      0x0800, 0x0800, CRC(f66ef3d8) SHA1(c42a325dd952cda074ef2857e7fa5154f0b7c7ce) )
	ROM_LOAD( "nml3.e5",      0x1000, 0x0800, CRC(d422fc8a) SHA1(18cafc462ce0800fea2af277439827dc1f4fc91b) )
	ROM_LOAD( "nml5.e7",      0x1800, 0x0800, CRC(d58952ac) SHA1(1c82a49cc1f0203e6436c5292ebd6e9004bd6a84) )
	ROM_LOAD( "nml4.e6",      0x2000, 0x0800, CRC(994c9afb) SHA1(c8e6af30d9b2cb5ca52fa325c6ac9a41413d067c) )
	ROM_LOAD( "nml6.e8",      0x2800, 0x0800, CRC(01ed2d8c) SHA1(bfa31e9100a1f9276c521ed8699e1cb0d067e0fa) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "nml8.n2",      0x0000, 0x0800, CRC(739009b4) SHA1(bbabd6ce7b1ded025f20120adaebdb97fb755ef0) )
	ROM_LOAD( "nml7.n1",      0x0800, 0x0800, CRC(d08ed22f) SHA1(33f450b6f63110bf804105280dc679f1591422f6) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "nml.d9",       0x0000, 0x0020, CRC(65e911f9) SHA1(6420a03195f63edeed17cc3a235e46e3f88d2037) )

	ROM_REGION( 0x0400, REGION_USER1, 0 ) /* color map */
	ROM_LOAD( "nl9.e2",       0x0000, 0x0400, CRC(9e05f14e) SHA1(76fc0b2b12cc9a0a64b539d2e75edefdb4a2ae61) )

	ROM_REGION( 0x0800, REGION_USER2, 0 ) /* tree + river */
	ROM_LOAD( "nl10.ic4",     0x0000, 0x0400, CRC(5b13f64e) SHA1(b04d2423fb443d46fff69c031b0312d956a5b789) )
	ROM_LOAD( "nl11.ic7",     0x0400, 0x0400, CRC(e717b241) SHA1(6d234a75514e22d484dc027db5bb85cf8b58f4f2) )
ROM_END


static DRIVER_INIT( cosmicg )
{
	/* Program ROMs have data pins connected different from normal */

	offs_t offs;

	for (offs =0; offs < memory_region_length(REGION_CPU1); offs++)
	{
		UINT8 scrambled = memory_region(REGION_CPU1)[offs];

		UINT8 normal = (scrambled >> 3 & 0x11)
		               | (scrambled >> 1 & 0x22)
		               | (scrambled << 1 & 0x44)
		               | (scrambled << 3 & 0x88);

		memory_region(REGION_CPU1)[offs] = normal;
	}


	/* Patch to avoid crash - Seems like duff romcheck routine */
	/* I would expect it to be bitrot, but have two romsets    */
	/* from different sources with the same problem!           */
	memory_region(REGION_CPU1)[0x1e9e] = 0x04;
	memory_region(REGION_CPU1)[0x1e9f] = 0xc0;
}


static DRIVER_INIT( devzone )
{
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4807, 0x4807, 0, 0, cosmic_background_enable_w);
}


static DRIVER_INIT( nomnlnd )
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x5001, 0, 0, nomnlnd_port_0_1_r);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4807, 0x4807, 0, 0, cosmic_background_enable_w);
}


GAME( 1979, cosmicg,  0,       cosmicg,  cosmicg,  cosmicg, ROT270, "Universal", "Cosmic Guerilla", GAME_NO_COCKTAIL )
GAME( 1979, cosmica,  0,       cosmica,  cosmica,  0,       ROT270, "Universal", "Cosmic Alien", 0 )
GAME( 1979, cosmica2, cosmica, cosmica,  cosmica,  0,       ROT270, "Universal", "Cosmic Alien (older)", 0 )
GAME( 1980, panic,    0,       panic,    panic,    0,       ROT270, "Universal", "Space Panic (version E)", 0 )
GAME( 1980, panic2,   panic,   panic,    panic,    0,       ROT270, "Universal", "Space Panic (set 2)", 0 )
GAME( 1980, panic3,   panic,   panic,    panic,    0,       ROT270, "Universal", "Space Panic (set 3)", 0 )
GAME( 1980, panich,   panic,   panic,    panic,    0,       ROT270, "Universal", "Space Panic (harder)", 0 )
GAME( 1980, panicger, panic,   panic,    panic,    0,       ROT270, "Universal (ADP Automaten license)", "Space Panic (German)", 0 )
GAME( 1980, magspot,  0,       magspot2, magspot2, 0,       ROT270, "Universal", "Magical Spot", GAME_IMPERFECT_SOUND )
GAME( 1980, magspot2, 0,       magspot2, magspot2, 0,       ROT270, "Universal", "Magical Spot II", GAME_IMPERFECT_SOUND )
GAME( 1980, devzone,  0,       devzone,  devzone,  devzone, ROT270, "Universal", "Devil Zone", GAME_IMPERFECT_SOUND )
GAME( 1980, devzone2, devzone, devzone,  devzone2, devzone, ROT270, "Universal", "Devil Zone (easier)", GAME_IMPERFECT_SOUND )
GAME( 1980, nomnlnd,  0,       nomnlnd,  nomnlnd,  nomnlnd, ROT270, "Universal", "No Man's Land", GAME_IMPERFECT_SOUND )
GAME( 1980, nomnlndg, nomnlnd, nomnlnd,  nomnlndg, nomnlnd, ROT270, "Universal (Gottlieb license)", "No Man's Land (Gottlieb)", GAME_IMPERFECT_SOUND )
