/***************************************************************************

Toki

driver by Jarek Parchanski


Coin inputs are handled by the sound CPU, so they don't work with sound
disabled. Use the service switch instead.


TODO
----

Does the bootleg use a 68000 @ 10MHz ? This causes some bad slow-
downs at the floating monkey machine (round 1), so set to 12 MHz
for now. Even at 12 this slowdown still happens a little.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"
#include "sound/3812intf.h"
#include "sound/msm5205.h"
#include "sound/3812intf.h"

extern UINT16 *toki_background1_videoram16;
extern UINT16 *toki_background2_videoram16;
extern UINT16 *toki_sprites_dataram16;
extern UINT16 *toki_scrollram16;

INTERRUPT_GEN( toki_interrupt );
VIDEO_START( toki );
VIDEO_EOF( toki );
VIDEO_EOF( tokib );
VIDEO_UPDATE( toki );
VIDEO_UPDATE( tokib );
WRITE16_HANDLER( toki_background1_videoram16_w );
WRITE16_HANDLER( toki_background2_videoram16_w );
WRITE16_HANDLER( toki_control_w );
WRITE16_HANDLER( toki_foreground_videoram16_w );


static WRITE16_HANDLER( tokib_soundcommand16_w )
{
	soundlatch_w(0,data & 0xff);
	cpunum_set_input_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( pip16_r )
{
	return ~0;
}


static int msm5205next;

static void toki_adpcm_int (int data)
{
	static int toggle=0;

	MSM5205_data_w (0,msm5205next);
	msm5205next>>=4;

	toggle ^= 1;
	if (toggle)
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static WRITE8_HANDLER( toki_adpcm_control_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);


	/* the code writes either 2 or 3 in the bottom two bits */
	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	memory_set_bankptr(1,&RAM[bankaddress]);

	MSM5205_reset_w(0,data & 0x08);
}

static WRITE8_HANDLER( toki_adpcm_data_w )
{
	msm5205next = data;
}


/*****************************************************************************/

static ADDRESS_MAP_START( toki_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060000, 0x06d7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06d800, 0x06dfff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06e000, 0x06e7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06e800, 0x06efff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06f000, 0x06f7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06f800, 0x06ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x080000, 0x08000d) AM_READ(seibu_main_word_r)
	AM_RANGE(0x0c0000, 0x0c0001) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c0002, 0x0c0003) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0c0004, 0x0c0005) AM_READ(input_port_3_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( toki_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060000, 0x06d7ff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x06d800, 0x06dfff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x06e000, 0x06e7ff) AM_WRITE(paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x06e800, 0x06efff) AM_WRITE(toki_background1_videoram16_w) AM_BASE(&toki_background1_videoram16)
	AM_RANGE(0x06f000, 0x06f7ff) AM_WRITE(toki_background2_videoram16_w) AM_BASE(&toki_background2_videoram16)
	AM_RANGE(0x06f800, 0x06ffff) AM_WRITE(toki_foreground_videoram16_w) AM_BASE(&videoram16)
	AM_RANGE(0x080000, 0x08000d) AM_WRITE(seibu_main_word_w)
	AM_RANGE(0x0a0000, 0x0a005f) AM_WRITE(toki_control_w) AM_BASE(&toki_scrollram16)
ADDRESS_MAP_END

/* In the bootleg, sound and sprites are remapped to 0x70000 */
static ADDRESS_MAP_START( tokib_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060000, 0x06dfff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06e000, 0x06e7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06e800, 0x06efff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06f000, 0x06f7ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x06f800, 0x06ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x072000, 0x072001) AM_READ(watchdog_reset16_r)   /* probably */
	AM_RANGE(0x0c0000, 0x0c0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0c0002, 0x0c0003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x0c0004, 0x0c0005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x0c000e, 0x0c000f) AM_READ(pip16_r)  /* sound related, if we return 0 the code writes */
				/* the sound command quickly followed by 0 and the */
				/* sound CPU often misses the command. */
ADDRESS_MAP_END

static ADDRESS_MAP_START( tokib_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060000, 0x06dfff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x06e000, 0x06e7ff) AM_WRITE(paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x06e800, 0x06efff) AM_WRITE(toki_background1_videoram16_w) AM_BASE(&toki_background1_videoram16)
	AM_RANGE(0x06f000, 0x06f7ff) AM_WRITE(toki_background2_videoram16_w) AM_BASE(&toki_background2_videoram16)
	AM_RANGE(0x06f800, 0x06ffff) AM_WRITE(toki_foreground_videoram16_w) AM_BASE(&videoram16)
	AM_RANGE(0x071000, 0x071001) AM_WRITE(MWA16_NOP)	/* sprite related? seems another scroll register */
				/* gets written the same value as 75000a (bg2 scrollx) */
	AM_RANGE(0x071804, 0x071807) AM_WRITE(MWA16_NOP)	/* sprite related, always 01be0100 */
	AM_RANGE(0x07180e, 0x071e45) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x075000, 0x075001) AM_WRITE(tokib_soundcommand16_w)
	AM_RANGE(0x075004, 0x07500b) AM_WRITE(MWA16_RAM) AM_BASE(&toki_scrollram16)
ADDRESS_MAP_END

/*****************************************************************************/

static ADDRESS_MAP_START( tokib_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xec00, 0xec00) AM_READ(YM3812_status_port_0_r)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf800, 0xf800) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tokib_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(toki_adpcm_control_w)	/* MSM5205 + ROM bank */
	AM_RANGE(0xe400, 0xe400) AM_WRITE(toki_adpcm_data_w)
	AM_RANGE(0xec00, 0xec00) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0xec01, 0xec01) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0xec08, 0xec08) AM_WRITE(YM3812_control_port_0_w)	/* mirror address, it seems */
	AM_RANGE(0xec09, 0xec09) AM_WRITE(YM3812_write_port_0_w)	/* mirror address, it seems */
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

/*****************************************************************************/

INPUT_PORTS_START( toki )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Joysticks" )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPSETTING(      0x0000, "Infinite (Cheat)")
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "50000 150000" )
	PORT_DIPSETTING(      0x0000, "70000 140000 210000" )
	PORT_DIPSETTING(      0x0c00, "70000" )
	PORT_DIPSETTING(      0x0400, "100000 200000" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tokib )
	PORT_START_TAG("DSW0")
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Joysticks" )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPSETTING(      0x0000, "Infinite (Cheat)")
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "50000 150000" )
	PORT_DIPSETTING(      0x0000, "70000 140000 210000" )
	PORT_DIPSETTING(      0x0c00, "70000" )
	PORT_DIPSETTING(      0x0400, "100000 200000" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*****************************************************************************/

static const gfx_layout toki_charlayout =
{
	8,8,
	4096,
	4,
	{ 4096*16*8+0, 4096*16*8+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout toki_tilelayout =
{
	16,16,
	4096,
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static const gfx_layout toki_spritelayout =
{
	16,16,
	8192,
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static const gfx_decode toki_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &toki_charlayout,  16*16, 16 },
	{ REGION_GFX2, 0, &toki_spritelayout, 0*16, 16 },
	{ REGION_GFX3, 0, &toki_tilelayout,  32*16, 16 },
	{ REGION_GFX4, 0, &toki_tilelayout,  48*16, 16 },
	{ -1 } /* end of array */
};

static const gfx_layout tokib_charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{4096*8*8*3,4096*8*8*2,4096*8*8*1,4096*8*8*0 }, /* planes */
	{ 0, 1,  2,  3,  4,  5,  6,  7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},		/* y bit */
	8*8
};

static const gfx_layout tokib_tilelayout =
{
	16,16,	/* 16 by 16 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 4096*16*16*3,4096*16*16*2,4096*16*16*1,4096*16*16*0 },	/* planes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  0x8000*8+0, 0x8000*8+1, 0x8000*8+2, 0x8000*8+3, 0x8000*8+4,
	  0x8000*8+5, 0x8000*8+6, 0x8000*8+7 }, 			/* x bit */
	{
	  0,8,16,24,32,40,48,56,
	  0x10000*8+ 0, 0x10000*8+ 8, 0x10000*8+16, 0x10000*8+24, 0x10000*8+32,
	  0x10000*8+40, 0x10000*8+48, 0x10000*8+56 },			/* y bit */
	8*8
};

static const gfx_layout tokib_spriteslayout =
{
	16,16,	/* 16 by 16 */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 8192*16*16*3,8192*16*16*2,8192*16*16*1,8192*16*16*0 },	/* planes */
	{	 0, 	1,	   2,	  3,	 4, 	5,	   6,	  7,
	 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },	/* x bit */
	{ 0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120 },		/* y bit */
	16*16
};

static const gfx_decode tokib_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tokib_charlayout,	16*16, 16 },
	{ REGION_GFX2, 0, &tokib_spriteslayout,  0*16, 16 },
	{ REGION_GFX3, 0, &tokib_tilelayout,	32*16, 16 },
	{ REGION_GFX4, 0, &tokib_tilelayout,	48*16, 16 },
	{ -1 } /* end of array */
};


/*****************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE


static struct MSM5205interface msm5205_interface =
{
	toki_adpcm_int,	/* interrupt function */
	MSM5205_S96_4B	/* 4KHz               */
};


static MACHINE_DRIVER_START( toki ) /* KOYO 20.000MHz near the cpu */

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2) 	/* 10 MHz Toshiba TMP68000P-10 */
	MDRV_CPU_PROGRAM_MAP(toki_readmem,toki_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(20000000/5)	/* 4MHz Zilog Z0840004PSC */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(toki_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(toki)
	MDRV_VIDEO_EOF(toki)
	MDRV_VIDEO_UPDATE(toki)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE(14318180/4,8000,1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tokib )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 10MHz causes bad slowdowns with monkey machine rd1 */
	MDRV_CPU_PROGRAM_MAP(tokib_readmem,tokib_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)/* VBL (could be level1, same vector) */

	MDRV_CPU_ADD(Z80, 4000000)	/* verified with PCB */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(tokib_sound_readmem,tokib_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(tokib_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(toki)
	MDRV_VIDEO_EOF(tokib)
	MDRV_VIDEO_UPDATE(tokib)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM3812, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tokij )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "tokijp.006",   0x00000, 0x20000, CRC(03d726b1) SHA1(bbe3a1ea1943cd73b821b3de4d5bf3dfbffd2168) )
	ROM_LOAD16_BYTE( "tokijp.004",   0x00001, 0x20000, CRC(54a45e12) SHA1(240538c8b010bb6e1e7fea2ed2fb1d5f9bc64b2b) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( tokia )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "tokijp.006",   0x00000, 0x20000, CRC(03d726b1) SHA1(bbe3a1ea1943cd73b821b3de4d5bf3dfbffd2168) )
	ROM_LOAD16_BYTE( "4c.10k",       0x00001, 0x20000, CRC(b2c345c5) SHA1(ff8ff31551e835e29192d7ddd3e1601968b3e2c5) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( toki )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "l10_6.bin",    0x00000, 0x20000, CRC(94015d91) SHA1(8b8d7c589eff038467f55e81ffd450f726c5a8b5) )
	ROM_LOAD16_BYTE( "k10_4e.bin",   0x00001, 0x20000, CRC(531bd3ef) SHA1(2e561f92f5c5f2da16c4791274ccbd421b9b0a05) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( tokiu )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "6b.10m",       0x00000, 0x20000, CRC(3674d9fe) SHA1(7c610bee23b0f7e6a9e3d5d72d6084e025eb89ec) )
	ROM_LOAD16_BYTE( "14.10k",       0x00001, 0x20000, CRC(bfdd48af) SHA1(3e48375019471a51f0c00d3444b0c1d37d2f8e92) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( tokib )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "toki.e3",      0x00000, 0x20000, CRC(ae9b3da4) SHA1(14eabbd0b3596528e96e4399dde03f5817eddbaa) )
	ROM_LOAD16_BYTE( "toki.e5",      0x00001, 0x20000, CRC(66a5a1d6) SHA1(9a8330d19234863952b0a5dce3f5ad28fcabaa31) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for code + 32k for banked data */
	ROM_LOAD( "toki.e1",      0x00000, 0x8000, CRC(2832ef75) SHA1(c15dc67a1251230fe79625b582c255678f3714d8) )
	ROM_CONTINUE(             0x10000, 0x8000 ) /* banked at 8000-bfff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e21",     0x000000, 0x08000, CRC(bb8cacbd) SHA1(05cdd2efe63de30dec2e5d2948567cee22e82a63) )   /* chars */
	ROM_LOAD( "toki.e13",     0x008000, 0x08000, CRC(052ad275) SHA1(0f4a9c752348cf5fb43d706bacbcd3e5937441e7) )
	ROM_LOAD( "toki.e22",     0x010000, 0x08000, CRC(04dcdc21) SHA1(3b74019d764a13ffc155f154522c6fe60cf1c5ea) )
	ROM_LOAD( "toki.e7",      0x018000, 0x08000, CRC(70729106) SHA1(e343c02d139d20a54e837e65b6a964e202f5811e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e26",     0x000000, 0x20000, CRC(a8ba71fc) SHA1(331d7396b6e862e32bb6a0d62c25fc201203b951) )   /* sprites */
	ROM_LOAD( "toki.e28",     0x020000, 0x20000, CRC(29784948) SHA1(9e17e57e2cb65a0aff61385c6d3a97b52474b6e7) )
	ROM_LOAD( "toki.e34",     0x040000, 0x20000, CRC(e5f6e19b) SHA1(77dc5cf961c8062b86ebeb896ad2075c3bfa2205) )
	ROM_LOAD( "toki.e36",     0x060000, 0x20000, CRC(96e8db8b) SHA1(9a0421fc57af27a8886e35b7a1a873aa06a112af) )
	ROM_LOAD( "toki.e30",     0x080000, 0x20000, CRC(770d2b1b) SHA1(27e57f21b462e36a10ffa2d4384955047b84190c) )
	ROM_LOAD( "toki.e32",     0x0a0000, 0x20000, CRC(c289d246) SHA1(596eda73b073e8fc3053734c780e7e2604fb5ca3) )
	ROM_LOAD( "toki.e38",     0x0c0000, 0x20000, CRC(87f4e7fb) SHA1(07d6bf00b1145a11f3d3f0af4425a3c5baeca3db) )
	ROM_LOAD( "toki.e40",     0x0e0000, 0x20000, CRC(96e87350) SHA1(754947f71261d8358e158fa9c8fcfd242cd58bc3) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e23",     0x000000, 0x10000, CRC(feb13d35) SHA1(1b78ce1e48d16e58ad0721b30ab87765ded7d24e) )   /* tiles 1 */
	ROM_LOAD( "toki.e24",     0x010000, 0x10000, CRC(5b365637) SHA1(434775b0614d904beaf40d7e00c1eaf59b704cb1) )
	ROM_LOAD( "toki.e15",     0x020000, 0x10000, CRC(617c32e6) SHA1(a80f93c83a06acf836e638e4ad2453692622015d) )
	ROM_LOAD( "toki.e16",     0x030000, 0x10000, CRC(2a11c0f0) SHA1(f9b1910c4932f5b95e5a9a8e8d5376c7210bcde7) )
	ROM_LOAD( "toki.e17",     0x040000, 0x10000, CRC(fbc3d456) SHA1(dd10455f2e6c415fb5e39fb239904c499b38ca3e) )
	ROM_LOAD( "toki.e18",     0x050000, 0x10000, CRC(4c2a72e1) SHA1(52a31f88e02e1689c2fffbbd86cbccd0bdab7dcc) )
	ROM_LOAD( "toki.e8",      0x060000, 0x10000, CRC(46a1b821) SHA1(74d9762aef3891463dc100d1bc2d4fdc3c1d163f) )
	ROM_LOAD( "toki.e9",      0x070000, 0x10000, CRC(82ce27f6) SHA1(db29396a336098664f48e3c04930b973a6ffe969) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e25",     0x000000, 0x10000, CRC(63026cad) SHA1(c8f3898985d99f2a61d4e17eba66b5989a23d0d7) )   /* tiles 2 */
	ROM_LOAD( "toki.e20",     0x010000, 0x10000, CRC(a7f2ce26) SHA1(6b12b3bd872112b42d91ce3c0d5bc95c0fc0f5b5) )
	ROM_LOAD( "toki.e11",     0x020000, 0x10000, CRC(48989aa0) SHA1(109c68c9f0966862194226cecc8b269d9307dd25) )
	ROM_LOAD( "toki.e12",     0x030000, 0x10000, CRC(c2ad9342) SHA1(7c9b5c14c8061e1a57797b79677741b1b98e64fa) )
	ROM_LOAD( "toki.e19",     0x040000, 0x10000, CRC(6cd22b18) SHA1(8281cfd46738448b6890c50c64fb72941e169bee) )
	ROM_LOAD( "toki.e14",     0x050000, 0x10000, CRC(859e313a) SHA1(18ac471a72b3ed42ba74456789adbe323f723660) )
	ROM_LOAD( "toki.e10",     0x060000, 0x10000, CRC(e15c1d0f) SHA1(d0d571dd1055d7307379850313216da86b0704e6) )
	ROM_LOAD( "toki.e6",      0x070000, 0x10000, CRC(6f4b878a) SHA1(4560b1e705a0eb9fad7fdc11fadf952ff67eb264) )
ROM_END


static DRIVER_INIT( toki )
{
	seibu_sound_decrypt(REGION_CPU2,0x2000);
}


DRIVER_INIT( tokib )
{
	unsigned char *temp = malloc (65536 * 2);
	int i, offs;

	/* invert the sprite data in the ROMs */
	for (i = 0; i < memory_region_length(REGION_GFX2); i++)
		memory_region(REGION_GFX2)[i] ^= 0xff;

	/* merge background tile graphics together */
	if (temp)
	{
		for (offs = 0; offs < memory_region_length(REGION_GFX3); offs += 0x20000)
		{
			unsigned char *base = &memory_region(REGION_GFX3)[offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}
		for (offs = 0; offs < memory_region_length(REGION_GFX4); offs += 0x20000)
		{
			unsigned char *base = &memory_region(REGION_GFX4)[offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}

		free (temp);
	}
}



GAME( 1989, toki,  0,    toki,  toki,  toki,  ROT0, "Tad", "Toki (World set 1)", 0 )
GAME( 1989, tokia, toki, toki,  toki,  toki,  ROT0, "Tad", "Toki (World set 2)", 0 )
GAME( 1989, tokij, toki, toki,  toki,  toki,  ROT0, "Tad", "JuJu Densetsu (Japan)", 0 )
GAME( 1989, tokiu, toki, toki,  toki,  toki,  ROT0, "Tad (Fabtek license)", "Toki (US)", 0 )
GAME( 1989, tokib, toki, tokib, tokib, tokib, ROT0, "bootleg", "Toki (bootleg)", 0 )
