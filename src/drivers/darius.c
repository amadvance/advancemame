/***************************************************************************

Darius    (c) Taito 1986
======

David Graves, Jarek Burczynski

Sound panning and other enhancements: Hiromitsu Shioya

Sources:        MAME Rastan driver
            Raine source - invaluable for this driver -
            many thanks to Richard Bush and the Raine Team.

                *****

Rom Versions
------------

Darius appears to be a revision of Dariusj (as well as being
for a different sales area). It has continue facilities, missing
in Dariusj, and extra sprites which are used when continuing.
It also has 2 extra program roms.


Use of PC080SN
--------------

This game uses 3 x PC080SN for generating tilemaps. They must be
mapped somehow to a single memory block and set of scroll registers.
There is an additional text tilemap on top of this, to allow for
both background planes scrolling. This need is presumably what led
to the TC0100SCN tilemap chip, debuted in Ninja Warriors (c)1987.
(The TC0100SCN includes a separate text layer.)


ADPCM Z80
---------

This writes the rom area whenever an interrupt occurs. It has no ram
therefore no stack to store registers or return addresses: so the
standard Z80 writes to the stack become irrelevant.


Dumpers Notes
=============

Darius (Old JPN Ver.)
(c)1986 Taito

-----------------------
Sound Board
K1100228A
CPU     :Z80 x2
Sound   :YM2203C x2
OSC     :8.000MHz
-----------------------
A96_56.18
A96_57.33

-----------------------
M4300067A
K1100226A
CPU     :MC68000P8 x2
OSC     :16000.00KHz
-----------------------
A96_28.152
A96_29.185
A96_30.154
A96_31.187

A96_32.157
A96_33.190
A96_34.158
A96_35.191

A96_36.175
A96_37.196
A96_38.176
A96_39.197
A96_40.177
A96_41.198
A96_42.178
A96_43.199
A96_44.179
A96_45.200
A96_46.180
A96_47.201

-----------------------
K1100227A
OSC     :26686.00KHz
Other   :PC080SN x3
-----------------------
A96_48.103
A96_48.24
A96_48.63
A96_49.104
A96_49.25
A96_49.64
A96_50.105
A96_50.26
A96_50.65
A96_51.131
A96_51.47
A96_51.86
A96_52.132
A96_52.48
A96_52.87
A96_53.133
A96_53.49
A96_53.88

A96_54.142
A96_55.143

A96-24.163
A96-25.164
A96-26.165


TODO
====

When you add a coin there is temporary volume distortion of other
sounds.

***************************************************************************/

#include <stdarg.h>
#include <math.h>
#include "driver.h"
#include "vidhrdw/taitoic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/taitosnd.h"
#include "sound/2203intf.h"
#include "sound/msm5205.h"
#include "sound/flt_vol.h"


MACHINE_START( darius );
MACHINE_RESET( darius );

VIDEO_START( darius );
VIDEO_UPDATE( darius );

static UINT16 cpua_ctrl;
static UINT16 coin_word=0;

extern UINT16 *darius_fg_ram;
READ16_HANDLER ( darius_fg_layer_r );
WRITE16_HANDLER( darius_fg_layer_w );

static size_t sharedram_size;
static UINT16 *sharedram;


static READ16_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE16_HANDLER( sharedram_w )
{
	COMBINE_DATA(&sharedram[offset]);
}

static void parse_control( void )	/* assumes Z80 sandwiched between 68Ks */
{
	/* bit 0 enables cpu B */
	/* however this fails when recovering from a save state
       if cpu B is disabled !! */
	cpunum_set_input_line(2, INPUT_LINE_RESET, (cpua_ctrl &0x1) ? CLEAR_LINE : ASSERT_LINE);

}

static WRITE16_HANDLER( cpua_ctrl_w )
{
	if ((data &0xff00) && ((data &0xff) == 0))
		data = data >> 8;	/* for Wgp */
	cpua_ctrl = data;

	parse_control();

	logerror("CPU #0 PC %06x: write %04x to cpu control\n",activecpu_get_pc(),data);
}

static WRITE16_HANDLER( darius_watchdog_w )
{
	watchdog_reset_w(0,data);
}

static READ16_HANDLER( paletteram16_r )
{
	return paletteram16[offset];
}


/**********************************************************
                        GAME INPUTS
**********************************************************/

static READ16_HANDLER( darius_ioc_r )
{
	switch (offset)
	{
		case 0x01:
			return (taitosound_comm_r(0) & 0xff);	/* sound interface read */

		case 0x04:
			return input_port_0_word_r(0,mem_mask);	/* IN0 */

		case 0x05:
			return input_port_1_word_r(0,mem_mask);	/* IN1 */

		case 0x06:
			return input_port_2_word_r(0,mem_mask);	/* IN2 */

		case 0x07:
			return coin_word;	/* bits 3&4 coin lockouts, must return zero */

		case 0x08:
			return input_port_3_word_r(0,mem_mask);	/* DSW */
	}

logerror("CPU #0 PC %06x: warning - read unmapped ioc offset %06x\n",activecpu_get_pc(),offset);

	return 0xff;
}

static WRITE16_HANDLER( darius_ioc_w )
{
	switch (offset)
	{
		case 0x00:	/* sound interface write */

			taitosound_port_w (0, data & 0xff);
			return;

		case 0x01:	/* sound interface write */

			taitosound_comm_w (0, data & 0xff);
			return;

		case 0x28:	/* unknown, written by both cpus - always 0? */
//ui_popup(" address %04x value %04x",offset,data);
			return;

		case 0x30:	/* coin control */
			/* bits 7,5,4,0 used on reset */
			/* bit 4 used whenever bg is blanked ? */
			coin_lockout_w(0, ~data & 0x02);
			coin_lockout_w(1, ~data & 0x04);
			coin_counter_w(0, data & 0x08);
			coin_counter_w(1, data & 0x40);
			coin_word = data &0xffff;
//ui_popup(" address %04x value %04x",offset,data);
			return;
	}

logerror("CPU #0 PC %06x: warning - write unmapped ioc offset %06x with %04x\n",activecpu_get_pc(),offset,data);
}


/***********************************************************
                     MEMORY STRUCTURES
***********************************************************/

static ADDRESS_MAP_START( darius_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x080000, 0x08ffff) AM_READ(MRA16_RAM)		/* main RAM */
	AM_RANGE(0xc00000, 0xc0001f) AM_READ(darius_ioc_r)	/* inputs, sound */
	AM_RANGE(0xd00000, 0xd0ffff) AM_READ(PC080SN_word_0_r)	/* tilemaps */
	AM_RANGE(0xd80000, 0xd80fff) AM_READ(paletteram16_r)	/* palette */
	AM_RANGE(0xe00100, 0xe00fff) AM_READ(MRA16_RAM)		/* sprite ram */
	AM_RANGE(0xe01000, 0xe02fff) AM_READ(sharedram_r)
	AM_RANGE(0xe08000, 0xe0ffff) AM_READ(darius_fg_layer_r)	/* front tilemap */
	AM_RANGE(0xe10000, 0xe10fff) AM_READ(MRA16_RAM)		/* ??? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x080000, 0x08ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x0a0000, 0x0a0001) AM_WRITE(cpua_ctrl_w)
	AM_RANGE(0x0b0000, 0x0b0001) AM_WRITE(darius_watchdog_w)
	AM_RANGE(0xc00000, 0xc0007f) AM_WRITE(darius_ioc_w)	/* coin ctr & lockout, sound */
	AM_RANGE(0xd00000, 0xd0ffff) AM_WRITE(PC080SN_word_0_w)
	AM_RANGE(0xd20000, 0xd20003) AM_WRITE(PC080SN_yscroll_word_0_w)
	AM_RANGE(0xd40000, 0xd40003) AM_WRITE(PC080SN_xscroll_word_0_w)
	AM_RANGE(0xd50000, 0xd50003) AM_WRITE(PC080SN_ctrl_word_0_w)
	AM_RANGE(0xd80000, 0xd80fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xe00100, 0xe00fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe01000, 0xe02fff) AM_WRITE(sharedram_w) AM_BASE(&sharedram) AM_SIZE(&sharedram_size)
	AM_RANGE(0xe08000, 0xe0ffff) AM_WRITE(darius_fg_layer_w) AM_BASE(&darius_fg_ram)
	AM_RANGE(0xe10000, 0xe10fff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_cpub_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x040000, 0x04ffff) AM_READ(MRA16_RAM)	/* local RAM */
	AM_RANGE(0xe01000, 0xe02fff) AM_READ(sharedram_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_cpub_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x040000, 0x04ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0xc00000, 0xc0007f) AM_WRITE(darius_ioc_w)	/* only writes $c00050 (?) */
	AM_RANGE(0xd80000, 0xd80fff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w)
	AM_RANGE(0xe00100, 0xe00fff) AM_WRITE(spriteram16_w)	/* some writes */
	AM_RANGE(0xe01000, 0Xe02fff) AM_WRITE(sharedram_w)
	AM_RANGE(0xe08000, 0xe0ffff) AM_WRITE(darius_fg_layer_w)	/* a few writes */
ADDRESS_MAP_END


/*****************************************************
                        SOUND
*****************************************************/

static INT32 banknum = -1;
static UINT8 adpcm_command = 0;
static UINT8 nmi_enable = 0;

static void reset_sound_region(void)
{
	memory_set_bankptr( STATIC_BANK1, memory_region(REGION_CPU2) + (banknum * 0x8000) + 0x10000 );
//  memory_set_bankptr( 1, memory_region(REGION_CPU2) + (banknum * 0x8000) + 0x10000 );

}

static WRITE8_HANDLER( sound_bankswitch_w )
{
		banknum = data &0x03;
		reset_sound_region();
//      banknum = data;
//      reset_sound_region();
}

static WRITE8_HANDLER( adpcm_command_w )
{
	adpcm_command = data;
	/* logerror("#ADPCM command write =%2x\n",data); */
}

#if 0
static WRITE8_HANDLER( display_value )
{
	ui_popup("d800=%x",data);
}
#endif


/*****************************************************
               Sound mixer/pan control
*****************************************************/

static UINT32 darius_def_vol[0x10];

#define DARIUS_VOL_MAX    (3*2 + 2)
#define DARIUS_PAN_MAX    (2 + 2 + 1)	/* FM 2port + PSG 2port + DA 1port */
static UINT8 darius_vol[DARIUS_VOL_MAX];
static UINT8 darius_pan[DARIUS_PAN_MAX];

static void update_fm0( void )
{
	int left, right;
	left  = (        darius_pan[0]  * darius_vol[6])>>8;
	right = ((0xff - darius_pan[0]) * darius_vol[6])>>8;
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, 6) >= 0)
		flt_volume_set_volume(6, left / 100.0);
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, 7) >= 0)
		flt_volume_set_volume(7, right / 100.0); /* FM #0 */
}

static void update_fm1( void )
{
	int left, right;
	left  = (        darius_pan[1]  * darius_vol[7])>>8;
	right = ((0xff - darius_pan[1]) * darius_vol[7])>>8;
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, 14) >= 0)
		flt_volume_set_volume(14, left / 100.0);
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, 15) >= 0)
		flt_volume_set_volume(15, right / 100.0); /* FM #1 */
}

static void update_psg0( int port )
{
	int left, right;
	left  = (        darius_pan[2]  * darius_vol[port])>>8;
	right = ((0xff - darius_pan[2]) * darius_vol[port])>>8;
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, port*2+0) >= 0)
		flt_volume_set_volume(port*2+0, left / 100.0);
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, port*2+1) >= 0)
		flt_volume_set_volume(port*2+1, right / 100.0);
}

static void update_psg1( int port )
{
	int left, right;
	left  = (        darius_pan[3]  * darius_vol[port + 3])>>8;
	right = ((0xff - darius_pan[3]) * darius_vol[port + 3])>>8;
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, port*2+0+8) >= 0)
		flt_volume_set_volume(port*2+0+8, left / 100.0);
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, port*2+1+8) >= 0)
		flt_volume_set_volume(port*2+1+8, right / 100.0);
}

static void update_da( void )
{
	int left, right;
	left  = darius_def_vol[(darius_pan[4]>>4)&0x0f];
	right = darius_def_vol[(darius_pan[4]>>0)&0x0f];
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, 16) >= 0)
		flt_volume_set_volume(16, left / 100.0);
	if (sndti_to_sndnum(SOUND_FILTER_VOLUME, 17) >= 0)
		flt_volume_set_volume(17, right / 100.0); /* FM #1 */
}

static WRITE8_HANDLER( darius_fm0_pan )
{
	darius_pan[0] = data&0xff;  /* data 0x00:right 0xff:left */
	update_fm0();
}

static WRITE8_HANDLER( darius_fm1_pan )
{
	darius_pan[1] = data&0xff;
	update_fm1();
}

static WRITE8_HANDLER( darius_psg0_pan )
{
	darius_pan[2] = data&0xff;
	update_psg0( 0 );
	update_psg0( 1 );
	update_psg0( 2 );
}

static WRITE8_HANDLER( darius_psg1_pan )
{
	darius_pan[3] = data&0xff;
	update_psg1( 0 );
	update_psg1( 1 );
	update_psg1( 2 );
}

static WRITE8_HANDLER( darius_da_pan )
{
	darius_pan[4] = data&0xff;
	update_da();
}

/**** Mixer Control ****/

static WRITE8_HANDLER( darius_write_portA0 )
{
	// volume control FM #0 PSG #0 A
	//ui_popup(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
	//ui_popup(" A0 %02x A1 %02x B0 %02x B1 %02x", port[0], port[1], port[2], port[3] );
	darius_vol[0] = darius_def_vol[(data>>4)&0x0f];
	darius_vol[6] = darius_def_vol[(data>>0)&0x0f];
	update_fm0();
	update_psg0( 0 );
}

static WRITE8_HANDLER( darius_write_portA1 )
{
	// volume control FM #1 PSG #1 A
	//ui_popup(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
	darius_vol[3] = darius_def_vol[(data>>4)&0x0f];
	darius_vol[7] = darius_def_vol[(data>>0)&0x0f];
	update_fm1();
	update_psg1( 0 );
}

static WRITE8_HANDLER( darius_write_portB0 )
{
	// volume control PSG #0 B/C
	//ui_popup(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
	darius_vol[1] = darius_def_vol[(data>>4)&0x0f];
	darius_vol[2] = darius_def_vol[(data>>0)&0x0f];
	update_psg0( 1 );
	update_psg0( 2 );
}

static WRITE8_HANDLER( darius_write_portB1 )
{
	// volume control PSG #1 B/C
	//ui_popup(" pan %02x %02x %02x %02x %02x", darius_pan[0], darius_pan[1], darius_pan[2], darius_pan[3], darius_pan[4] );
	darius_vol[4] = darius_def_vol[(data>>4)&0x0f];
	darius_vol[5] = darius_def_vol[(data>>0)&0x0f];
	update_psg1( 1 );
	update_psg1( 2 );
}


/*****************************************************
           Sound memory structures / ADPCM
*****************************************************/

static ADDRESS_MAP_START( darius_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x8000, 0x8fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x9000, 0x9000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x9001, 0x9001) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0xa000, 0xa000) AM_READ(YM2203_status_port_1_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(YM2203_read_port_1_r)
	AM_RANGE(0xb000, 0xb000) AM_READ(MRA8_NOP)
	AM_RANGE(0xb001, 0xb001) AM_READ(taitosound_slave_comm_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x8fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x9000, 0x9000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x9001, 0x9001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(YM2203_write_port_1_w)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xb001, 0xb001) AM_WRITE(taitosound_slave_comm_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(darius_fm0_pan)
	AM_RANGE(0xc400, 0xc400) AM_WRITE(darius_fm1_pan)
	AM_RANGE(0xc800, 0xc800) AM_WRITE(darius_psg0_pan)
	AM_RANGE(0xcc00, 0xcc00) AM_WRITE(darius_psg1_pan)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(darius_da_pan)
	AM_RANGE(0xd400, 0xd400) AM_WRITE(adpcm_command_w)	/* ADPCM command for second Z80 to read from port 0x00 */
//  AM_RANGE(0xd800, 0xd800) AM_WRITE(display_value)    /* ??? */
	AM_RANGE(0xdc00, 0xdc00) AM_WRITE(sound_bankswitch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_sound2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READ(MRA8_ROM)
	/* yes, no RAM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_sound2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_NOP)	/* writes rom whenever interrupt occurs - as no stack */
	/* yes, no RAM */
ADDRESS_MAP_END


static void darius_adpcm_int (int data)
{
	if (nmi_enable)
	{
		cpunum_set_input_line(3, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static struct MSM5205interface msm5205_interface =
{
	darius_adpcm_int,	/* interrupt function */
	MSM5205_S48_4B		/* 8KHz   */
};

static READ8_HANDLER( adpcm_command_read )
{
	/* logerror("read port 0: %02x  PC=%4x\n",adpcm_command, activecpu_get_pc() ); */
	return adpcm_command;
}

static READ8_HANDLER( readport2 )
{
	return 0;
}

static READ8_HANDLER( readport3 )
{
	return 0;
}

static WRITE8_HANDLER ( adpcm_nmi_disable )
{
	nmi_enable = 0;
	/* logerror("write port 0: NMI DISABLE  PC=%4x\n", data, activecpu_get_pc() ); */
}

static WRITE8_HANDLER ( adpcm_nmi_enable )
{
	nmi_enable = 1;
	/* logerror("write port 1: NMI ENABLE   PC=%4x\n", activecpu_get_pc() ); */
}

static WRITE8_HANDLER( adpcm_data_w )
{
	MSM5205_data_w (0,   data         );
	MSM5205_reset_w(0, !(data & 0x20) );	/* my best guess, but it could be output enable as well */
}

static ADDRESS_MAP_START( darius_sound2_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(adpcm_command_read)
	AM_RANGE(0x02, 0x02) AM_READ(readport2)	/* ??? */
	AM_RANGE(0x03, 0x03) AM_READ(readport3)	/* ??? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( darius_sound2_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(adpcm_nmi_disable)
	AM_RANGE(0x01, 0x01) AM_WRITE(adpcm_nmi_enable)
	AM_RANGE(0x02, 0x02) AM_WRITE(adpcm_data_w)
ADDRESS_MAP_END


/***********************************************************
                      INPUT PORTS, DIPs
***********************************************************/


#define TAITO_COINAGE_WORLD_16 \
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )

#define TAITO_COINAGE_JAPAN_16 \
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )

#define TAITO_DIFFICULTY_16 \
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(      0x0200, DEF_STR( Easy ) ) \
	PORT_DIPSETTING(      0x0300, DEF_STR( Medium ) ) \
	PORT_DIPSETTING(      0x0100, DEF_STR( Hard ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )

#define DARIUS_PLAYERS_INPUT( player ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(player) PORT_8WAY \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(player) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(player) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define DARIUS_SYSTEM_INPUT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )  \
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )


INPUT_PORTS_START( darius )
	DARIUS_PLAYERS_INPUT( 1 )

	DARIUS_PLAYERS_INPUT( 2 )

	DARIUS_SYSTEM_INPUT

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Autofire" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, "Fast" )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_16
	TAITO_DIFFICULTY_16
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "every 600k" )
	PORT_DIPSETTING(      0x0c00, "600k only" )
	PORT_DIPSETTING(      0x0400, "800k only" )
	PORT_DIPSETTING(      0x0000, DEF_STR( None ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "4" )
	PORT_DIPSETTING(      0x1000, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( dariuse )
	DARIUS_PLAYERS_INPUT( 1 )

	DARIUS_PLAYERS_INPUT( 2 )

	DARIUS_SYSTEM_INPUT

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Autofire" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, "Fast" )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_16
	TAITO_DIFFICULTY_16
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "every 600k" )
	PORT_DIPSETTING(      0x0c00, "600k only" )
	PORT_DIPSETTING(      0x0400, "800k only" )
	PORT_DIPSETTING(      0x0000, DEF_STR( None ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "4" )
	PORT_DIPSETTING(      0x1000, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( dariusj )
	DARIUS_PLAYERS_INPUT( 1 )

	DARIUS_PLAYERS_INPUT( 2 )

	DARIUS_SYSTEM_INPUT

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Autofire" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, "Fast" )
	PORT_SERVICE( 0x0004, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_16
	TAITO_DIFFICULTY_16
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "every 600k" )
	PORT_DIPSETTING(      0x0c00, "600k only" )
	PORT_DIPSETTING(      0x0400, "800k only" )
	PORT_DIPSETTING(      0x0000, DEF_STR( None ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x3000, "3" )
	PORT_DIPSETTING(      0x2000, "4" )
	PORT_DIPSETTING(      0x1000, "5" )
	PORT_DIPSETTING(      0x0000, "6" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/**************************************************************
                           GFX DECODING
**************************************************************/

static const gfx_layout tilelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
        { 24, 8, 16, 0 },       /* pixel bits separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  0+ 32*8, 1+ 32*8, 2+ 32*8, 3+ 32*8, 4+ 32*8, 5+ 32*8, 6+ 32*8, 7+ 32*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  64*8 + 0*32, 64*8 + 1*32, 64*8 + 2*32, 64*8 + 3*32,
	  64*8 + 4*32, 64*8 + 5*32, 64*8 + 6*32, 64*8 + 7*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_layout char2layout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,1),
	2,	/* 2 bits per pixel */
	{ 0, 8 },	/* pixel bits separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 32 consecutive bytes */
};

static const gfx_decode darius_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,   0, 256 },	/* sprites */
	{ REGION_GFX1, 0, &charlayout,   0, 256 },	/* scr tiles */
	{ REGION_GFX3, 0, &char2layout,  0, 256 },	/* top layer scr tiles */
	{ -1 } /* end of array */
};


/**************************************************************
                        YM2203 (SOUND)
**************************************************************/

/* handler called by the YM2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)	/* assumes Z80 sandwiched between 68Ks */
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface_1 =
{
	0,		/* portA read */
	0,
	darius_write_portA0,	/* portA write */
	darius_write_portB0,	/* portB write */
	irqhandler
};

static struct YM2203interface ym2203_interface_2 =
{
	0,		/* portA read */
	0,
	darius_write_portA1,	/* portA write */
	darius_write_portB1		/* portB write */
};


/***********************************************************
                       MACHINE DRIVERS
***********************************************************/

static MACHINE_DRIVER_START( darius )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,16000000/2)	/* 8 MHz ? */
	MDRV_CPU_PROGRAM_MAP(darius_readmem,darius_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */	/* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(darius_sound_readmem,darius_sound_writemem)

	MDRV_CPU_ADD(M68000,16000000/2)	/* 8 MHz ? */
	MDRV_CPU_PROGRAM_MAP(darius_cpub_readmem,darius_cpub_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2) /* 4 MHz ? */
	/* audio CPU */	/* ADPCM player using MSM5205 */
	MDRV_CPU_PROGRAM_MAP(darius_sound2_readmem,darius_sound2_writemem)
	MDRV_CPU_IO_MAP(darius_sound2_readport,darius_sound2_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame ? */

	MDRV_MACHINE_START(darius)
	MDRV_MACHINE_RESET(darius)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_ASPECT_RATIO(4,3)
	MDRV_SCREEN_SIZE(108*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 108*8-1, 1*8, 29*8-1)
	MDRV_GFXDECODE(darius_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*2)

	MDRV_VIDEO_START(darius)
	MDRV_VIDEO_UPDATE(darius)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface_1)
	MDRV_SOUND_ROUTE(0, "filter.2203.0.0l", 0.08)
	MDRV_SOUND_ROUTE(0, "filter.2203.0.0r", 0.08)
	MDRV_SOUND_ROUTE(1, "filter.2203.0.1l", 0.08)
	MDRV_SOUND_ROUTE(1, "filter.2203.0.1r", 0.08)
	MDRV_SOUND_ROUTE(2, "filter.2203.0.2l", 0.08)
	MDRV_SOUND_ROUTE(2, "filter.2203.0.2r", 0.08)
	MDRV_SOUND_ROUTE(3, "filter.2203.0.3l", 0.60)
	MDRV_SOUND_ROUTE(3, "filter.2203.0.3r", 0.60)

	MDRV_SOUND_ADD(YM2203, 4000000)
	MDRV_SOUND_CONFIG(ym2203_interface_2)
	MDRV_SOUND_ROUTE(0, "filter.2203.1.0l", 0.08)
	MDRV_SOUND_ROUTE(0, "filter.2203.1.0r", 0.08)
	MDRV_SOUND_ROUTE(1, "filter.2203.1.1l", 0.08)
	MDRV_SOUND_ROUTE(1, "filter.2203.1.1r", 0.08)
	MDRV_SOUND_ROUTE(2, "filter.2203.1.2l", 0.08)
	MDRV_SOUND_ROUTE(2, "filter.2203.1.2r", 0.08)
	MDRV_SOUND_ROUTE(3, "filter.2203.1.3l", 0.60)
	MDRV_SOUND_ROUTE(3, "filter.2203.1.3r", 0.60)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "msm5205.l", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "msm5205.r", 1.0)

	MDRV_SOUND_ADD_TAG("filter.2203.0.0l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.0r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.1l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.1r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.2l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.2r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.3l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.0.3r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD_TAG("filter.2203.1.0l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.0r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.1l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.1r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.2l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.2r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.3l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("filter.2203.1.3r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD_TAG("msm5205.l", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD_TAG("msm5205.r", FILTER_VOLUME, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


/***************************************************************************
                                  DRIVERS
***************************************************************************/

ROM_START( darius )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_FILL( 0x00000, 0x60000, 0xffffffff )
	ROM_LOAD16_BYTE( "da-59.bin",   0x00000, 0x10000, CRC(11aab4eb) SHA1(92f795e96a940e8d94abbf429ba4ac119992b991) )
	ROM_LOAD16_BYTE( "da-58.bin",   0x00001, 0x10000, CRC(5f71e697) SHA1(bf959cf82e8e8ba950ab40d9c008ad5de01385aa) )
	ROM_LOAD16_BYTE( "da-61.bin",   0x20000, 0x10000, CRC(4736aa9b) SHA1(05e549d96a053e6b3bc34359267adcd73f98dd4a) )
	ROM_LOAD16_BYTE( "da-66.bin",   0x20001, 0x10000, CRC(4ede5f56) SHA1(88c06aef4b0a3e29fa30c24a57f2d3a05fc9f021) )
	ROM_LOAD16_BYTE( "a96_31.187",  0x40000, 0x10000, CRC(e9bb5d89) SHA1(a5d08129c32b97e2cce84496945766fd32b6506e) )	/* 2 data roms */
	ROM_LOAD16_BYTE( "a96_30.154",  0x40001, 0x10000, CRC(9eb5e127) SHA1(50e2fe5ec7f79ecf1fb5107298da13ef5ab37162) )

   	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "a96_57.33",  0x00000, 0x10000, CRC(33ceb730) SHA1(05070ea503ac57ff8445145d6f97115f7aad90a5) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "a96_33-1.190", 0x00000, 0x10000, CRC(ff186048) SHA1(becb00d2cc69a6d4e839086bd3d902f4e6a99aa6) )
	ROM_LOAD16_BYTE( "a96_32-1.157", 0x00001, 0x10000, CRC(d9719de8) SHA1(9e907cfb5cbe6abebccfbd065d02e7a71c5aa494) )
	ROM_LOAD16_BYTE( "a96_35-1.191", 0x20000, 0x10000, CRC(b3280193) SHA1(f4bad066c16682f9267752c50a31ef64b312f11e) )
	ROM_LOAD16_BYTE( "a96_34-1.158", 0x20001, 0x10000, CRC(ca3b2573) SHA1(4da0d8536e546ea46b2374318e25c30305f4c977) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* second Z80 driving the ADPCM chip */
	ROM_LOAD( "a96_56.18",      0x00000, 0x10000, CRC(292ef55c) SHA1(67bfe3693e43daece06d4795645d54cd66419e5b) )		/* Z80 prog + ADPCM samples */

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	/* There are THREE of each SCR gfx rom on the actual board,
       making a complete set for every PC080SN tilemap chip */
	ROM_LOAD16_BYTE( "a96_48.24",    0x00000, 0x10000, CRC(39c9b3aa) SHA1(43a91d916c5a09207dfa37413feb5025636f37ae) )	/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_49.25",    0x20000, 0x10000, CRC(37a7d88a) SHA1(cede0d810d74ec460dcc4b391bb1acd5a669a7b4) )
	ROM_LOAD16_BYTE( "a96_50.26",    0x40000, 0x10000, CRC(75d738e4) SHA1(634606da46136ab605f5477af5639a20e39b44c4) )
	ROM_LOAD16_BYTE( "a96_51.47",    0x00001, 0x10000, CRC(1bf8f0d3) SHA1(7f36e69336260958282eb663fe71b56410f0ee42) )
	ROM_LOAD16_BYTE( "a96_52.48",    0x20001, 0x10000, CRC(2d9b2128) SHA1(9b72936fbd9dca6ef8302ac6c40a1cec019cebb5) )
	ROM_LOAD16_BYTE( "a96_53.49",    0x40001, 0x10000, CRC(0173484c) SHA1(41d70039bda0965afe89251696ceaec7b7f40c24) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "a96_44.179",   0x00000, 0x10000, CRC(bbc18878) SHA1(7732ab2a3002f8b500615377dab42ac75451cb3b) )	/* 16x16 sprites */
	ROM_LOAD32_BYTE( "a96_45.200",   0x00001, 0x10000, CRC(616cdd8b) SHA1(74e0c483a68d984a689ea1381ed3a9da2f8a410a) )
	ROM_LOAD32_BYTE( "a96_46.180",   0x00002, 0x10000, CRC(fec35418) SHA1(f0f401c3634e91b81cb8484b7b03f350d382e889) )
	ROM_LOAD32_BYTE( "a96_47.201",   0x00003, 0x10000, CRC(8df9286a) SHA1(4a197e4c38d1750cc316b8710f4a0fef4316be14) )

	ROM_LOAD32_BYTE( "a96_40.177",   0x40000, 0x10000, CRC(b699a51e) SHA1(5fd751dd44618743dc8a3df04cf0a987753a868b) )
	ROM_LOAD32_BYTE( "a96_41.198",   0x40001, 0x10000, CRC(97128a3a) SHA1(257ddd1ba71e6beeaf18e0c5d7006d1d2b6a5edf) )
	ROM_LOAD32_BYTE( "a96_42.178",   0x40002, 0x10000, CRC(7f55ee0f) SHA1(d9ba7b8fbf59308a08613d67e92da6829f6b6db3) )
	ROM_LOAD32_BYTE( "a96_43.199",   0x40003, 0x10000, CRC(c7cad469) SHA1(dbd37aa10f12e4950f8ec6bcd7d150fa55e64742) )

	ROM_LOAD32_BYTE( "da-62.bin",    0x80000, 0x10000, CRC(9179862c) SHA1(be94c7d213a34baf82f974ee1092aba44b072623) )
	ROM_LOAD32_BYTE( "da-63.bin",    0x80001, 0x10000, CRC(fa19cfff) SHA1(58a3ae3270ebe5a162cd62df06da7199843707cf) )
	ROM_LOAD32_BYTE( "da-64.bin",    0x80002, 0x10000, CRC(814c676f) SHA1(a6a64e65a3c163ecfede14b48ea70c20050248c3) )
	ROM_LOAD32_BYTE( "da-65.bin",    0x80003, 0x10000, CRC(14eee326) SHA1(41760fada2a5e34ee6c9250af927baf650d9cfc4) )

 	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE )			/* 8x8 SCR tiles */
	/* There's only one of each of these on a real board */
	ROM_LOAD16_BYTE( "a96_54.143",   0x0000, 0x4000, CRC(51c02ae2) SHA1(27d2a6c649d047da1f22758569cb36531e3bf8bc) )
	ROM_LOAD16_BYTE( "a96_55.144",   0x0001, 0x4000, CRC(771e4d98) SHA1(0e8ce5d569775883f4bc777b9bd49eb23ba7b42e) )

 	ROM_REGION( 0x1000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "a96-24.163",   0x0000, 0x0400, CRC(0fa8be7f) SHA1(079686b5d65b4b966591090d8c0e13e66dc5beca) )	/* proms, currently unused */
	ROM_LOAD16_BYTE( "a96-25.164",   0x0400, 0x0400, CRC(265508a6) SHA1(f8ee1c658b33ae76d8a457a4042d9b4b58247823) )
	ROM_LOAD16_BYTE( "a96-26.165",   0x0800, 0x0400, CRC(4891b9c0) SHA1(1f550a9a4ad3ca379f88f5865ed1b281c7b87f31) )
ROM_END

ROM_START( dariusj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_FILL( 0x00000, 0x60000, 0xffffffff )
	ROM_LOAD16_BYTE( "a96_29-1.185", 0x00000, 0x10000, CRC(75486f62) SHA1(818b095f2c6cc5764161c3e14ba70fe1c4b2f724) )
	ROM_LOAD16_BYTE( "a96_28-1.152", 0x00001, 0x10000, CRC(fb34d400) SHA1(b14517384f5eadca8b73833bcd81374614b928d4) )
	/* middle area is empty */
	ROM_LOAD16_BYTE( "a96_31.187",   0x40000, 0x10000, CRC(e9bb5d89) SHA1(a5d08129c32b97e2cce84496945766fd32b6506e) )	/* 2 data roms */
	ROM_LOAD16_BYTE( "a96_30.154",   0x40001, 0x10000, CRC(9eb5e127) SHA1(50e2fe5ec7f79ecf1fb5107298da13ef5ab37162) )

   	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "a96_57.33",  0x00000, 0x10000, CRC(33ceb730) SHA1(05070ea503ac57ff8445145d6f97115f7aad90a5) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "a96_33-1.190", 0x00000, 0x10000, CRC(ff186048) SHA1(becb00d2cc69a6d4e839086bd3d902f4e6a99aa6) )
	ROM_LOAD16_BYTE( "a96_32-1.157", 0x00001, 0x10000, CRC(d9719de8) SHA1(9e907cfb5cbe6abebccfbd065d02e7a71c5aa494) )
	ROM_LOAD16_BYTE( "a96_35-1.191", 0x20000, 0x10000, CRC(b3280193) SHA1(f4bad066c16682f9267752c50a31ef64b312f11e) )
	ROM_LOAD16_BYTE( "a96_34-1.158", 0x20001, 0x10000, CRC(ca3b2573) SHA1(4da0d8536e546ea46b2374318e25c30305f4c977) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* second Z80 driving the ADPCM chip */
	ROM_LOAD( "a96_56.18",      0x00000, 0x10000, CRC(292ef55c) SHA1(67bfe3693e43daece06d4795645d54cd66419e5b) )		/* Z80 prog + ADPCM samples */

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "a96_48.24",    0x00000, 0x10000, CRC(39c9b3aa) SHA1(43a91d916c5a09207dfa37413feb5025636f37ae) )	/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_49.25",    0x20000, 0x10000, CRC(37a7d88a) SHA1(cede0d810d74ec460dcc4b391bb1acd5a669a7b4) )
	ROM_LOAD16_BYTE( "a96_50.26",    0x40000, 0x10000, CRC(75d738e4) SHA1(634606da46136ab605f5477af5639a20e39b44c4) )
	ROM_LOAD16_BYTE( "a96_51.47",    0x00001, 0x10000, CRC(1bf8f0d3) SHA1(7f36e69336260958282eb663fe71b56410f0ee42) )
	ROM_LOAD16_BYTE( "a96_52.48",    0x20001, 0x10000, CRC(2d9b2128) SHA1(9b72936fbd9dca6ef8302ac6c40a1cec019cebb5) )
	ROM_LOAD16_BYTE( "a96_53.49",    0x40001, 0x10000, CRC(0173484c) SHA1(41d70039bda0965afe89251696ceaec7b7f40c24) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "a96_44.179",   0x00000, 0x10000, CRC(bbc18878) SHA1(7732ab2a3002f8b500615377dab42ac75451cb3b) )	/* 16x16 sprites */
	ROM_LOAD32_BYTE( "a96_45.200",   0x00001, 0x10000, CRC(616cdd8b) SHA1(74e0c483a68d984a689ea1381ed3a9da2f8a410a) )
	ROM_LOAD32_BYTE( "a96_46.180",   0x00002, 0x10000, CRC(fec35418) SHA1(f0f401c3634e91b81cb8484b7b03f350d382e889) )
	ROM_LOAD32_BYTE( "a96_47.201",   0x00003, 0x10000, CRC(8df9286a) SHA1(4a197e4c38d1750cc316b8710f4a0fef4316be14) )

	ROM_LOAD32_BYTE( "a96_40.177",   0x40000, 0x10000, CRC(b699a51e) SHA1(5fd751dd44618743dc8a3df04cf0a987753a868b) )
	ROM_LOAD32_BYTE( "a96_41.198",   0x40001, 0x10000, CRC(97128a3a) SHA1(257ddd1ba71e6beeaf18e0c5d7006d1d2b6a5edf) )
	ROM_LOAD32_BYTE( "a96_42.178",   0x40002, 0x10000, CRC(7f55ee0f) SHA1(d9ba7b8fbf59308a08613d67e92da6829f6b6db3) )
	ROM_LOAD32_BYTE( "a96_43.199",   0x40003, 0x10000, CRC(c7cad469) SHA1(dbd37aa10f12e4950f8ec6bcd7d150fa55e64742) )

	ROM_LOAD32_BYTE( "a96_36.175",   0x80000, 0x10000, CRC(af598141) SHA1(f3b888bcbd4560cca48187055cbe4107e2b392a6) )
	ROM_LOAD32_BYTE( "a96_37.196",   0x80001, 0x10000, CRC(b48137c8) SHA1(03e98a93f4fa19dfe77da244c002abc84b936a22) )
	ROM_LOAD32_BYTE( "a96_38.176",   0x80002, 0x10000, CRC(e4f3e3a7) SHA1(0baa8a672516bcc4f17f40f429ac3d227de16625) )
	ROM_LOAD32_BYTE( "a96_39.197",   0x80003, 0x10000, CRC(ea30920f) SHA1(91d47b10886d6c243bc676435e300cb3b5fcca33) )

 	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE )			/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_54.143",   0x0000, 0x4000, CRC(51c02ae2) SHA1(27d2a6c649d047da1f22758569cb36531e3bf8bc) )
	ROM_LOAD16_BYTE( "a96_55.144",   0x0001, 0x4000, CRC(771e4d98) SHA1(0e8ce5d569775883f4bc777b9bd49eb23ba7b42e) )

 	ROM_REGION( 0x1000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "a96-24.163",   0x0000, 0x0400, CRC(0fa8be7f) SHA1(079686b5d65b4b966591090d8c0e13e66dc5beca) )	/* proms, currently unused */
	ROM_LOAD16_BYTE( "a96-25.164",   0x0400, 0x0400, CRC(265508a6) SHA1(f8ee1c658b33ae76d8a457a4042d9b4b58247823) )
	ROM_LOAD16_BYTE( "a96-26.165",   0x0800, 0x0400, CRC(4891b9c0) SHA1(1f550a9a4ad3ca379f88f5865ed1b281c7b87f31) )
ROM_END

ROM_START( dariuso )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_FILL( 0x00000, 0x60000, 0xffffffff )
	ROM_LOAD16_BYTE( "a96-29.185",   0x00000, 0x10000, CRC(f775162b) SHA1(a17e570c2ba4daf0a3526b45c324c822faac0c8d) )
	ROM_LOAD16_BYTE( "a96-28.152",   0x00001, 0x10000, CRC(4721d667) SHA1(fa9a109054a818f836452215204ce91f2b166ddb) )
	/* middle area is empty */
	ROM_LOAD16_BYTE( "a96_31.187",   0x40000, 0x10000, CRC(e9bb5d89) SHA1(a5d08129c32b97e2cce84496945766fd32b6506e) )	/* 2 data roms */
	ROM_LOAD16_BYTE( "a96_30.154",   0x40001, 0x10000, CRC(9eb5e127) SHA1(50e2fe5ec7f79ecf1fb5107298da13ef5ab37162) )

   	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "a96_57.33",  0x00000, 0x10000, CRC(33ceb730) SHA1(05070ea503ac57ff8445145d6f97115f7aad90a5) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "a96-33.190",   0x00000, 0x10000, CRC(d2f340d2) SHA1(d9175bf4dda5707afb3c57d3b6affe0305084c71) )
	ROM_LOAD16_BYTE( "a96-32.157",   0x00001, 0x10000, CRC(044c9848) SHA1(5293e9e83fd38d0d14e4f3b3a342d88e27ee44d6) )
	ROM_LOAD16_BYTE( "a96-35.191",   0x20000, 0x10000, CRC(b8ed718b) SHA1(8951f9c3c971c5621ec98b63fb27d44f30304c70) )
	ROM_LOAD16_BYTE( "a96-34.158",   0x20001, 0x10000, CRC(7556a660) SHA1(eaa82f3e1f827616ff25e22673d6d2ee54f0ad4c) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* second Z80 driving the ADPCM chip */
	ROM_LOAD( "a96_56.18",      0x00000, 0x10000, CRC(292ef55c) SHA1(67bfe3693e43daece06d4795645d54cd66419e5b) )		/* Z80 prog + ADPCM samples */

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "a96_48.24",    0x00000, 0x10000, CRC(39c9b3aa) SHA1(43a91d916c5a09207dfa37413feb5025636f37ae) )	/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_49.25",    0x20000, 0x10000, CRC(37a7d88a) SHA1(cede0d810d74ec460dcc4b391bb1acd5a669a7b4) )
	ROM_LOAD16_BYTE( "a96_50.26",    0x40000, 0x10000, CRC(75d738e4) SHA1(634606da46136ab605f5477af5639a20e39b44c4) )
	ROM_LOAD16_BYTE( "a96_51.47",    0x00001, 0x10000, CRC(1bf8f0d3) SHA1(7f36e69336260958282eb663fe71b56410f0ee42) )
	ROM_LOAD16_BYTE( "a96_52.48",    0x20001, 0x10000, CRC(2d9b2128) SHA1(9b72936fbd9dca6ef8302ac6c40a1cec019cebb5) )
	ROM_LOAD16_BYTE( "a96_53.49",    0x40001, 0x10000, CRC(0173484c) SHA1(41d70039bda0965afe89251696ceaec7b7f40c24) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "a96_44.179",   0x00000, 0x10000, CRC(bbc18878) SHA1(7732ab2a3002f8b500615377dab42ac75451cb3b) )	/* 16x16 sprites */
	ROM_LOAD32_BYTE( "a96_45.200",   0x00001, 0x10000, CRC(616cdd8b) SHA1(74e0c483a68d984a689ea1381ed3a9da2f8a410a) )
	ROM_LOAD32_BYTE( "a96_46.180",   0x00002, 0x10000, CRC(fec35418) SHA1(f0f401c3634e91b81cb8484b7b03f350d382e889) )
	ROM_LOAD32_BYTE( "a96_47.201",   0x00003, 0x10000, CRC(8df9286a) SHA1(4a197e4c38d1750cc316b8710f4a0fef4316be14) )

	ROM_LOAD32_BYTE( "a96_40.177",   0x40000, 0x10000, CRC(b699a51e) SHA1(5fd751dd44618743dc8a3df04cf0a987753a868b) )
	ROM_LOAD32_BYTE( "a96_41.198",   0x40001, 0x10000, CRC(97128a3a) SHA1(257ddd1ba71e6beeaf18e0c5d7006d1d2b6a5edf) )
	ROM_LOAD32_BYTE( "a96_42.178",   0x40002, 0x10000, CRC(7f55ee0f) SHA1(d9ba7b8fbf59308a08613d67e92da6829f6b6db3) )
	ROM_LOAD32_BYTE( "a96_43.199",   0x40003, 0x10000, CRC(c7cad469) SHA1(dbd37aa10f12e4950f8ec6bcd7d150fa55e64742) )

	ROM_LOAD32_BYTE( "a96_36.175",   0x80000, 0x10000, CRC(af598141) SHA1(f3b888bcbd4560cca48187055cbe4107e2b392a6) )
	ROM_LOAD32_BYTE( "a96_37.196",   0x80001, 0x10000, CRC(b48137c8) SHA1(03e98a93f4fa19dfe77da244c002abc84b936a22) )
	ROM_LOAD32_BYTE( "a96_38.176",   0x80002, 0x10000, CRC(e4f3e3a7) SHA1(0baa8a672516bcc4f17f40f429ac3d227de16625) )
	ROM_LOAD32_BYTE( "a96_39.197",   0x80003, 0x10000, CRC(ea30920f) SHA1(91d47b10886d6c243bc676435e300cb3b5fcca33) )

 	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE )			/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_54.143",   0x0000, 0x4000, CRC(51c02ae2) SHA1(27d2a6c649d047da1f22758569cb36531e3bf8bc) )
	ROM_LOAD16_BYTE( "a96_55.144",   0x0001, 0x4000, CRC(771e4d98) SHA1(0e8ce5d569775883f4bc777b9bd49eb23ba7b42e) )

 	ROM_REGION( 0x1000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "a96-24.163",   0x0000, 0x0400, CRC(0fa8be7f) SHA1(079686b5d65b4b966591090d8c0e13e66dc5beca) )	/* proms, currently unused */
	ROM_LOAD16_BYTE( "a96-25.164",   0x0400, 0x0400, CRC(265508a6) SHA1(f8ee1c658b33ae76d8a457a4042d9b4b58247823) )
	ROM_LOAD16_BYTE( "a96-26.165",   0x0800, 0x0400, CRC(4891b9c0) SHA1(1f550a9a4ad3ca379f88f5865ed1b281c7b87f31) )
ROM_END

ROM_START( dariuse )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_FILL( 0x00000, 0x60000, 0xffffffff )
	ROM_LOAD16_BYTE( "dae-68.bin",   0x00000, 0x10000, CRC(ed721127) SHA1(8127f4a9b26b5fb83a381235eef0577d60d1cfd7) )
	ROM_LOAD16_BYTE( "dae-67.bin",   0x00001, 0x10000, CRC(b99aea8c) SHA1(859ada7c472ab2ac308faa775066e79ed1f4ad71) )
	/* middle area is empty */
	ROM_LOAD16_BYTE( "dae-70.bin",   0x40000, 0x10000, CRC(54590b31) SHA1(2b89846f14a5cb19b58ab4999bc5ae11671bbb5a) )	/* 2 data roms */
	ROM_LOAD16_BYTE( "a96_30.154",   0x40001, 0x10000, CRC(9eb5e127) SHA1(50e2fe5ec7f79ecf1fb5107298da13ef5ab37162) )	// dae-69.bin

   	ROM_REGION( 0x30000, REGION_CPU2, 0 )	/* Z80 sound cpu */
	ROM_LOAD( "a96_57.33",  0x00000, 0x10000, CRC(33ceb730) SHA1(05070ea503ac57ff8445145d6f97115f7aad90a5) )

	ROM_REGION( 0x80000, REGION_CPU3, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "dae-72.bin",   0x00000, 0x10000, CRC(248ca2cc) SHA1(43b29146d8e2c62dd1fb7dc842fd441a360f2453) )
	ROM_LOAD16_BYTE( "dae-71.bin",   0x00001, 0x10000, CRC(65dd0403) SHA1(8036c35ce5df0727cccb9ece3bfac9577160d4fd) )
	ROM_LOAD16_BYTE( "dae-74.bin",   0x20000, 0x10000, CRC(0ea31f60) SHA1(c9e7eaf8bf3abbef944b7de407d5d5ddaac93e31) )
	ROM_LOAD16_BYTE( "dae-73.bin",   0x20001, 0x10000, CRC(27036a4d) SHA1(426dccb8f559d39460c97bfd4354c74a59af172e) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* second Z80 driving the ADPCM chip */
	ROM_LOAD( "a96_56.18",      0x00000, 0x10000, CRC(292ef55c) SHA1(67bfe3693e43daece06d4795645d54cd66419e5b) )		/* Z80 prog + ADPCM samples */

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "a96_48.24",    0x00000, 0x10000, CRC(39c9b3aa) SHA1(43a91d916c5a09207dfa37413feb5025636f37ae) )	/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_49.25",    0x20000, 0x10000, CRC(37a7d88a) SHA1(cede0d810d74ec460dcc4b391bb1acd5a669a7b4) )
	ROM_LOAD16_BYTE( "a96_50.26",    0x40000, 0x10000, CRC(75d738e4) SHA1(634606da46136ab605f5477af5639a20e39b44c4) )
	ROM_LOAD16_BYTE( "a96_51.47",    0x00001, 0x10000, CRC(1bf8f0d3) SHA1(7f36e69336260958282eb663fe71b56410f0ee42) )
	ROM_LOAD16_BYTE( "a96_52.48",    0x20001, 0x10000, CRC(2d9b2128) SHA1(9b72936fbd9dca6ef8302ac6c40a1cec019cebb5) )
	ROM_LOAD16_BYTE( "a96_53.49",    0x40001, 0x10000, CRC(0173484c) SHA1(41d70039bda0965afe89251696ceaec7b7f40c24) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD32_BYTE( "a96_44.179",   0x00000, 0x10000, CRC(bbc18878) SHA1(7732ab2a3002f8b500615377dab42ac75451cb3b) )	/* 16x16 sprites */
	ROM_LOAD32_BYTE( "a96_45.200",   0x00001, 0x10000, CRC(616cdd8b) SHA1(74e0c483a68d984a689ea1381ed3a9da2f8a410a) )
	ROM_LOAD32_BYTE( "a96_46.180",   0x00002, 0x10000, CRC(fec35418) SHA1(f0f401c3634e91b81cb8484b7b03f350d382e889) )
	ROM_LOAD32_BYTE( "a96_47.201",   0x00003, 0x10000, CRC(8df9286a) SHA1(4a197e4c38d1750cc316b8710f4a0fef4316be14) )

	ROM_LOAD32_BYTE( "a96_40.177",   0x40000, 0x10000, CRC(b699a51e) SHA1(5fd751dd44618743dc8a3df04cf0a987753a868b) )
	ROM_LOAD32_BYTE( "a96_41.198",   0x40001, 0x10000, CRC(97128a3a) SHA1(257ddd1ba71e6beeaf18e0c5d7006d1d2b6a5edf) )
	ROM_LOAD32_BYTE( "a96_42.178",   0x40002, 0x10000, CRC(7f55ee0f) SHA1(d9ba7b8fbf59308a08613d67e92da6829f6b6db3) )
	ROM_LOAD32_BYTE( "a96_43.199",   0x40003, 0x10000, CRC(c7cad469) SHA1(dbd37aa10f12e4950f8ec6bcd7d150fa55e64742) )

	ROM_LOAD32_BYTE( "a96_36.175",   0x80000, 0x10000, CRC(af598141) SHA1(f3b888bcbd4560cca48187055cbe4107e2b392a6) )
	ROM_LOAD32_BYTE( "a96_37.196",   0x80001, 0x10000, CRC(b48137c8) SHA1(03e98a93f4fa19dfe77da244c002abc84b936a22) )
	ROM_LOAD32_BYTE( "a96_38.176",   0x80002, 0x10000, CRC(e4f3e3a7) SHA1(0baa8a672516bcc4f17f40f429ac3d227de16625) )
	ROM_LOAD32_BYTE( "a96_39.197",   0x80003, 0x10000, CRC(ea30920f) SHA1(91d47b10886d6c243bc676435e300cb3b5fcca33) )

 	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE )			/* 8x8 SCR tiles */
	ROM_LOAD16_BYTE( "a96_54.143",   0x0000, 0x4000, CRC(51c02ae2) SHA1(27d2a6c649d047da1f22758569cb36531e3bf8bc) )
	ROM_LOAD16_BYTE( "a96_55.144",   0x0001, 0x4000, CRC(771e4d98) SHA1(0e8ce5d569775883f4bc777b9bd49eb23ba7b42e) )

 	ROM_REGION( 0x1000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "a96-24.163",   0x0000, 0x0400, CRC(0fa8be7f) SHA1(079686b5d65b4b966591090d8c0e13e66dc5beca) )	/* proms, currently unused */
	ROM_LOAD16_BYTE( "a96-25.164",   0x0400, 0x0400, CRC(265508a6) SHA1(f8ee1c658b33ae76d8a457a4042d9b4b58247823) )
	ROM_LOAD16_BYTE( "a96-26.165",   0x0800, 0x0400, CRC(4891b9c0) SHA1(1f550a9a4ad3ca379f88f5865ed1b281c7b87f31) )
ROM_END


static DRIVER_INIT( darius )
{
//  taitosnd_setz80_soundcpu( 2 );

	cpua_ctrl = 0xff;

	banknum = -1;
}


MACHINE_START( darius )
{
	state_save_register_global(cpua_ctrl);
	state_save_register_func_postload(parse_control);

	// (there are other sound vars that may need saving too) //
	state_save_register_global(banknum);
	state_save_register_global(adpcm_command);
	state_save_register_global(nmi_enable);
	state_save_register_func_postload(reset_sound_region);
	return 0;
}


MACHINE_RESET( darius )
{
	int  i;

	/**** setup sound bank image ****/
	unsigned char *RAM = memory_region(REGION_CPU2);

	for( i = 3; i >= 0; i-- ){
		memcpy( RAM + 0x8000*i + 0x10000, RAM,            0x4000 );
		memcpy( RAM + 0x8000*i + 0x14000, RAM + 0x4000*i, 0x4000 );
	}
	memory_set_bankptr(1, RAM);

	sound_global_enable( 1 );	/* mixer enabled */

	for( i = 0; i < DARIUS_VOL_MAX; i++ ){
		darius_vol[i] = 0x00;	/* min volume */
	}
	for( i = 0; i < DARIUS_PAN_MAX; i++ ){
		darius_pan[i] = 0x80;	/* center */
	}
	for( i = 0; i < 0x10; i++ ){
		//logerror( "calc %d = %d\n", i, (int)(100.0f / (float)pow(10.0f, (32.0f - (i * (32.0f / (float)(0xf)))) / 20.0f)) );
		darius_def_vol[i] = (int)(100.0f / (float)pow(10.0f, (32.0f - (i * (32.0f / (float)(0xf)))) / 20.0f));
	}
}


GAME( 1986, darius,   0,        darius,   darius,   darius,   ROT0, "Taito Corporation Japan", "Darius (World)", 0 )
GAME( 1986, dariusj,  darius,   darius,   dariusj,  darius,   ROT0, "Taito Corporation", "Darius (Japan)", 0 )
GAME( 1986, dariuso,  darius,   darius,   dariusj,  darius,   ROT0, "Taito Corporation", "Darius (Japan old version)", 0 )
GAME( 1986, dariuse,  darius,   darius,   dariuse,  darius,   ROT0, "Taito Corporation", "Darius (Extra) (Japan)", 0 )
