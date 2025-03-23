/*
    Great Swordsman (Taito) 1984
    Joshi Volleyball (Taito) 1983

TODO:

-joshi vollyball
   -The incomplete graphic
   -The implementation of DAC sound ?
   -MCU code DUMP and emulation
   -The true interrupt circuit of SUB CPU
   -unknown ROM (BANK ROM of sub-cpu ? )

Credits:
- Steve Ellenoff: Original emulation and Mame driver
- Jarek Parchanski: Dip Switch Fixes, Color improvements, ADPCM Interface code
- Tatsuyuki Satoh: sound improvements, NEC 8741 emulation,adpcm improvements,
            josvollyvall 8741 emulation
- Charlie Miltenberger: sprite colors improvements & precious hardware
            information and screenshots

Trick:
If you want fight with ODILION swordsman patch program for 1st CPU
at these addresses, otherwise you won't never fight with him.

        ROM[0x2256] = 0
        ROM[0x2257] = 0
        ROM[0x2258] = 0
        ROM[0x2259] = 0
        ROM[0x225A] = 0


There are 3 Z80s and two AY-3-8910s..

Prelim memory map (last updated 6/15/98)
*****************************************
GS1     z80 Main Code   (8K)    0000-1FFF
Gs2     z80 Game Data   (8K)    2000-3FFF
Gs3     z80 Game Data   (8K)    4000-5FFF
Gs4     z80 Game Data   (8K)    6000-7FFF
Gs5     z80 Game Data   (4K)    8000-8FFF
Gs6     Sprites         (8K)
Gs7     Sprites         (8K)
Gs8     Sprites         (8K)
Gs10    Tiles           (8K)
Gs11    Tiles           (8K)
Gs12    3rd z80 CPU &   (8K)
        ADPCM Samples?
Gs13    ADPCM Samples?  (8K)
Gs14    ADPCM Samples?  (8K)
Gs15    2nd z80 CPU     (8K)    0000-1FFF
Gs16    2nd z80 Data    (8K)    2000-3FFF
*****************************************

**********
*Main Z80*
**********

    9000 - 9fff Work Ram
        982e - 982e Free play
        98e0 - 98e0 Coin Input
        98e1 - 98e1 Player 1 Controls
        98e2 - 98e2 Player 2 Controls
        9c00 - 9c30 (Hi score - Scores)
        9c78 - 9cd8 (Hi score - Names)
        9e00 - 9e7f Sprites in working ram!
        9e80 - 9eff Sprite X & Y in working ram!

    a000 - afff Sprite RAM & Video Attributes
        a000 - a37F ???
        a380 - a77F Sprite Tile #s
        a780 - a7FF Sprite Y & X positions
        a980 - a980 Background Tile Bank Select
        ab00 - ab00 Background Tile Y-Scroll register
        ab80 - abff Sprite Attributes(X & Y Flip)

    b000 - b7ff Screen RAM
    b800 - ffff not used?!

PORTS:
7e 8741-#0 data port
7f 8741-#1 command / status port

*************
*2nd Z80 CPU*
*************
0000 - 3FFF ROM CODE
4000 - 43FF WORK RAM

write
6000 adpcm sound command for 3rd CPU

PORTS:
00 8741-#2 data port
01 8741-#2 command / status port
20 8741-#3 data port
21 8741-#3 command / status port
40 8741-#1 data port
41 8741-#1 command / status port

read:
60 fake port #0 ?
61 ay8910-#0 read port
data / ay8910-#0 read
80 fake port #1 ?
81 ay8910-#1 read port

write:
60 ay8910-#0 controll port
61 ay8910-#0 data port
80 ay8910-#1 controll port
81 ay8910-#1 data port
   ay8910-A  : NMI controll ?
a0 unknown
e0 unknown (watch dog?)

*************
*3rd Z80 CPU*
*************
0000-5fff ROM

read:
a000 adpcm sound command

write:
6000 MSM5205 reset and data

*************
I8741 communication data

reg: 0->1 (main->2nd) /     : (1->0) 2nd->main :
 0 : DSW.2 (port)           : DSW.1(port)
 1 : DSW.1                  : DSW.2
 2 : IN0 / sound error code :
 3 : IN1 / ?                :
 4 : IN2                    :
 4 : IN3                    :
 5 :                        :
 6 :                        : DSW0?
 7 :                        : ?

******************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/tait8741.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"


extern WRITE8_HANDLER( gsword_charbank_w );
extern WRITE8_HANDLER( gsword_videoctrl_w );
extern WRITE8_HANDLER( gsword_videoram_w );
extern WRITE8_HANDLER( gsword_scroll_w );

extern PALETTE_INIT( josvolly );
extern PALETTE_INIT( gsword );
extern VIDEO_START( gsword );
extern VIDEO_UPDATE( gsword );

extern size_t gsword_spritexy_size;

extern UINT8 *gsword_spritexy_ram;
extern UINT8 *gsword_spritetile_ram;
extern UINT8 *gsword_spriteattrib_ram;

static int coins;
static int fake8910_0,fake8910_1;

static int gsword_nmi_enable;

static UINT8 *gsword_cpu2_ram;
static int gsword_protect_hack;

#if 0
static int gsword_coins_in(void)
{
	/* emulate 8741 coin slot */
	if (readinputport(4)&0xc0)
	{
		logerror("Coin In\n");
		return 0x80;
	}
	logerror("NO Coin\n");
	return 0x00;
}
#endif

#include "cpu/z80/z80.h"

/* CPU 2 memory hack */
/* (402E) timeout upcount must be under 0AH                         */
/* (4004,4005) clear down counter , if (4004,4005)==0 then (402E)=0 */
static READ8_HANDLER( gsword_hack_r )
{
	UINT8 data = gsword_cpu2_ram[offset + 4];

	/*if(offset==1)printf("CNT %02X%02X\n",gsword_cpu2_ram[5],gsword_cpu2_ram[4]); */

	/* speedup timeout cound down */
	if(gsword_protect_hack)
	{
		switch(offset)
		{
		case 0: return data & 0x7f;
		case 1: return 0x00;
		}
	}
	return data;
}

static READ8_HANDLER( gsword_8741_2_r )
{
	switch (offset)
	{
	case 0x01: /* start button , coins */
		return readinputport(0);
	case 0x02: /* Player 1 Controller */
		return readinputport(1);
	case 0x04: /* Player 2 Controller */
		return readinputport(3);
//  default:
//      logerror("8741-2 unknown read %d PC=%04x\n",offset,activecpu_get_pc());
	}
	/* unknown */
	return 0;
}

static READ8_HANDLER( gsword_8741_3_r )
{
	switch (offset)
	{
	case 0x01: /* start button  */
		return readinputport(2);
	case 0x02: /* Player 1 Controller? */
		return readinputport(1);
	case 0x04: /* Player 2 Controller? */
		return readinputport(3);
	}
	/* unknown */
//  logerror("8741-3 unknown read %d PC=%04x\n",offset,activecpu_get_pc());
	return 0;
}

static struct TAITO8741interface gsword_8741interface=
{
	4,         /* 4 chips */
	{ TAITO8741_MASTER,TAITO8741_SLAVE,TAITO8741_PORT,TAITO8741_PORT },  /* program mode */
	{ 1,0,0,0 },							     /* serial port connection */
	{ input_port_7_r,input_port_6_r,gsword_8741_2_r,gsword_8741_3_r }    /* port handler */
};

MACHINE_RESET( gsword )
{
	int i;

	for(i=0;i<4;i++) TAITO8741_reset(i);
	coins = 0;

	/* snd CPU mask NMI during reset phase */
	gsword_nmi_enable   = 0;
	gsword_protect_hack = 0;

	TAITO8741_start(&gsword_8741interface);
}

MACHINE_RESET( josvolly )
{
	josvolly_8741_reset();
}

#if 0
static INTERRUPT_GEN( josvolly_snd_interrupt )
{
	if(josvolly_nmi_enable)
	{
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
		josvolly_nmi_enable = 0;
	}
}
#endif

static INTERRUPT_GEN( gsword_snd_interrupt )
{
	if(gsword_nmi_enable)
	{
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static WRITE8_HANDLER( gsword_nmi_set_w )
{
/*  printf("AY write %02X\n",data);*/

	gsword_protect_hack = (data&0x80) ? 0 : 1;
#if 0
	/* An actual circuit isn't known. */
	/* write ff,02,ff,fe, 17 x 0d,0f */
	gsword_nmi_enable = ((data>>7) & (data&1) &1) == 0;


#else
	switch(data)
	{
	case 0xff:
		gsword_nmi_enable = 0; /* NMI must be disable */
		break;
	case 0x02:
		gsword_nmi_enable = 0; /* ANY */
		break;
	case 0x0d:
		gsword_nmi_enable = 1;
		break;
	case 0x0f:
		gsword_nmi_enable = 1; /* NMI must be enable */
		break;
	case 0xfe:
		gsword_nmi_enable = 1; /* NMI must be enable */
		break;
	}
	/* bit1= nmi disable , for ram check */
	logerror("NMI controll %02x\n",data);
#endif
}

static WRITE8_HANDLER( josvolly_nmi_enable_w )
{
	josvolly_nmi_enable = 1;
}

static WRITE8_HANDLER( gsword_AY8910_control_port_0_w )
{
	AY8910_control_port_0_w(offset,data);
	fake8910_0 = data;
}
static WRITE8_HANDLER( gsword_AY8910_control_port_1_w )
{
	AY8910_control_port_1_w(offset,data);
	fake8910_1 = data;
}

static READ8_HANDLER( gsword_fake_0_r )
{
	return fake8910_0+1;
}
static READ8_HANDLER( gsword_fake_1_r )
{
	return fake8910_1+1;
}

WRITE8_HANDLER( gsword_adpcm_data_w )
{
	MSM5205_data_w (0,data & 0x0f); /* bit 0..3 */
	MSM5205_reset_w(0,(data>>5)&1); /* bit 5    */
	MSM5205_vclk_w(0,(data>>4)&1);  /* bit 4    */
}

WRITE8_HANDLER( adpcm_soundcommand_w )
{
	soundlatch_w(0,data);
	cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);
}

static ADDRESS_MAP_START( cpu1_map, ADDRESS_SPACE_PROGRAM , 8 )
	AM_RANGE(0x0000, 0x8fff) AM_ROM
	AM_RANGE(0x9000, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xa37f) AM_RAM
	AM_RANGE(0xa380, 0xa3ff) AM_RAM AM_BASE(&gsword_spritetile_ram)
	AM_RANGE(0xa400, 0xa77f) AM_RAM
	AM_RANGE(0xa780, 0xa7ff) AM_RAM AM_BASE(&gsword_spritexy_ram) AM_SIZE(&gsword_spritexy_size)
	AM_RANGE(0xa980, 0xa980) AM_WRITE(gsword_charbank_w)
	AM_RANGE(0xaa80, 0xaa80) AM_WRITE(gsword_videoctrl_w)	/* flip screen, char palette bank */
	AM_RANGE(0xab00, 0xab00) AM_WRITE(gsword_scroll_w)
	AM_RANGE(0xab80, 0xabff) AM_WRITE(MWA8_RAM) AM_BASE(&gsword_spriteattrib_ram)
	AM_RANGE(0xb000, 0xb7ff) AM_RAM AM_WRITE(gsword_videoram_w) AM_BASE(&videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu1_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7e, 0x7f) AM_WRITE(TAITO8741_0_w)  AM_READ(TAITO8741_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( josvolly_cpu1_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x7e, 0x7f) AM_WRITE(josvolly_8741_0_w)  AM_READ(josvolly_8741_0_r)
ADDRESS_MAP_END

//
static ADDRESS_MAP_START( cpu2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM AM_BASE(&gsword_cpu2_ram)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(adpcm_soundcommand_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x01) AM_READWRITE(TAITO8741_2_r,TAITO8741_2_w)
	AM_RANGE(0x20, 0x21) AM_READWRITE(TAITO8741_3_r,TAITO8741_3_w)
	AM_RANGE(0x40, 0x41) AM_READWRITE(TAITO8741_1_r,TAITO8741_1_w)
	AM_RANGE(0x60, 0x60) AM_WRITE(gsword_AY8910_control_port_0_w) AM_READ(gsword_fake_0_r)
	AM_RANGE(0x61, 0x61) AM_WRITE(AY8910_write_port_0_w)          AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x80, 0x80) AM_WRITE(gsword_AY8910_control_port_1_w) AM_READ(gsword_fake_1_r)
	AM_RANGE(0x81, 0x81) AM_WRITE(AY8910_write_port_1_w)          AM_READ(AY8910_read_port_1_r)
//
	AM_RANGE(0xe0, 0xe0) AM_READ(MRA8_NOP) /* ?? */
	AM_RANGE(0xa0, 0xa0) AM_WRITE(MWA8_NOP) /* ?? */
	AM_RANGE(0xe0, 0xe0) AM_WRITE(MWA8_NOP) /* watch dog ?*/
ADDRESS_MAP_END

//

static ADDRESS_MAP_START( cpu3_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8000) AM_WRITE(gsword_adpcm_data_w)
	AM_RANGE(0xa000, 0xa000) AM_READ(soundlatch_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( josvolly_cpu2_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM AM_BASE(&gsword_cpu2_ram)

	/* 8000 to 8003 looks MCU */
	AM_RANGE(0x8000, 0x8000) AM_READ(input_port_1_r) // 1PL
	AM_RANGE(0x8001, 0x8001) AM_READ(input_port_2_r) // 2PL / ACK
	AM_RANGE(0x8002, 0x8002) AM_READ(input_port_0_r) // START

//  AM_RANGE(0x6000, 0x6000) AM_WRITE(adpcm_soundcommand_w)
	AM_RANGE(0xA000, 0xA001) AM_WRITE(josvolly_8741_1_w) AM_READ(josvolly_8741_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( josvolly_cpu2_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(gsword_AY8910_control_port_0_w) AM_READ(gsword_fake_0_r)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_write_port_0_w)          AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x40, 0x40) AM_WRITE(gsword_AY8910_control_port_1_w) AM_READ(gsword_fake_1_r)
	AM_RANGE(0x41, 0x41) AM_WRITE(AY8910_write_port_1_w)          AM_READ(AY8910_read_port_1_r)

	AM_RANGE(0x81, 0x81) AM_WRITE(josvolly_nmi_enable_w)
	AM_RANGE(0xC1, 0xC1) AM_NOP // irq clear

ADDRESS_MAP_END

INPUT_PORTS_START( gsword )
	PORT_START	/* IN0 (8741-2 port1?) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_START	/* IN1 (8741-2 port2?) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_START	/* IN2 (8741-3 port1?) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_START	/* IN3  (8741-3 port2?) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_START	/* IN4 (coins) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START	/* DSW0 */
	/* NOTE: Switches 0 & 1, 6,7,8 not used      */
	/*   Coins configurations were handled   */
	/*   via external hardware & not via program */
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_5C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Fencing Difficulty" )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x10, 0x00, "Kendo Difficulty" )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x20, 0x00, "Roman Difficulty" )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "255 (Cheat)" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x00, "First Stage" )
	PORT_DIPSETTING(    0x00, "Fencing" )
	PORT_DIPSETTING(    0x10, "Kendo" )
	PORT_DIPSETTING(    0x20, "Roman" )
//  PORT_DIPSETTING(    0x30, "Kendo" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( josvolly )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0c, 0x00, "DIP1-0c(982E)" )
	PORT_DIPSETTING(    0x0c, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPNAME( 0x30, 0x00, "DIP1-30(982A)" )
	PORT_DIPSETTING(    0x30, "96H" )
	PORT_DIPSETTING(    0x20, "78H" )
	PORT_DIPSETTING(    0x10, "5AH" )
	PORT_DIPSETTING(    0x00, "3CH" )
	PORT_DIPNAME( 0x40, 0x40, "TEST_MODE" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIP1-80(982C)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW2 */
//  PORT_DIPNAME( 0x01, 0x00, "DSW2-0" )
//  PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x01, DEF_STR( On ) )
//  PORT_DIPNAME( 0x02, 0x00, "DSW2-1($9831)" )
//  PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
//  PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0C, 0x0C, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
//  PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
//  PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static const gfx_layout gsword_text =
{
	8,8,    /* 8x8 characters */
	1024,	/* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8    /* every char takes 16 bytes */
};

static const gfx_layout gsword_sprites1 =
{
	16,16,   /* 16x16 sprites */
	64*2,    /* 128 sprites */
	2,       /* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8     /* every sprite takes 64 bytes */
};

static const gfx_layout gsword_sprites2 =
{
	32,32,    /* 32x32 sprites */
	64,       /* 64 sprites */
	2,       /* 2 bits per pixel */
	{ 0, 4 }, /* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 72*8+0, 72*8+1, 72*8+2, 72*8+3,
			80*8+0, 80*8+1, 80*8+2, 80*8+3, 88*8+0, 88*8+1, 88*8+2, 88*8+3},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			128*8, 129*8, 130*8, 131*8, 132*8, 133*8, 134*8, 135*8,
			160*8, 161*8, 162*8, 163*8, 164*8, 165*8, 166*8, 167*8 },
	64*8*4    /* every sprite takes (64*8=16x6)*4) bytes */
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gsword_text,         0, 64 },
	{ REGION_GFX2, 0, &gsword_sprites1,  64*4, 64 },
	{ REGION_GFX3, 0, &gsword_sprites2,  64*4, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	0,
	0,
	gsword_nmi_set_w, /* portA write */
	0
};

static struct MSM5205interface msm5205_interface =
{
	0,				/* interrupt function */
	MSM5205_SEX_4B	/* vclk input mode    */
};

static MACHINE_DRIVER_START( gsword )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(cpu1_map,0)
	MDRV_CPU_IO_MAP(cpu1_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_PROGRAM_MAP(cpu2_map,0)
	MDRV_CPU_IO_MAP(cpu2_io_map,0)
	MDRV_CPU_VBLANK_INT(gsword_snd_interrupt,4)

	MDRV_CPU_ADD(Z80, 3000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(cpu3_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(200) /* Allow time for 2nd cpu to interleave*/

	MDRV_MACHINE_RESET(gsword)

#if 1
	/* to MCU timeout champbbj */
	MDRV_INTERLEAVE(100)
#endif

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+64*4)

	MDRV_PALETTE_INIT(gsword)
	MDRV_VIDEO_START(gsword)
	MDRV_VIDEO_UPDATE(gsword)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( josvolly )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18000000/6) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu1_map,0)
	MDRV_CPU_IO_MAP(josvolly_cpu1_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_CPU_ADD(Z80, 12000000/4) /* ? */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(josvolly_cpu2_map,0)
	MDRV_CPU_IO_MAP(josvolly_cpu2_io_map,0)
//  MDRV_CPU_VBLANK_INT(gsword_snd_interrupt,1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(josvolly)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+64*4)

	MDRV_PALETTE_INIT(josvolly)
	MDRV_VIDEO_START(gsword)
	MDRV_VIDEO_UPDATE(gsword)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)

#if 0
	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
#endif
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

// ac10-* ROM labels were written using a typewriter. The board is a Taito original however.
ROM_START( gsword )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64K for main CPU */
	ROM_LOAD( "ac10-01.2c",   0x0000, 0x2000, CRC(511b9389) SHA1(d24a083e812663522a06138dcc3aa60e48d27434) )
	ROM_LOAD( "ac1-2.2d",     0x2000, 0x2000, CRC(d772accf) SHA1(08028c6f026c118cc375ecff5c24dcb549475633) )
	ROM_LOAD( "ac10-03.2e",   0x4000, 0x2000, CRC(413a0ce6) SHA1(3dde7889db9f449aec5a05a4a3d27e12786df869) )
	ROM_LOAD( "ac1-4.2f",     0x6000, 0x2000, CRC(ca9d206d) SHA1(887eedc4e10218bf149c84399edd5d1e32c85051) )
	ROM_LOAD( "ac1-5.2h",     0x8000, 0x1000, CRC(2a892326) SHA1(a2cd91263714480c2569d3bbc73d62d222175e89) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for 2nd CPU */
	ROM_LOAD( "ac10-15.5h",   0x0000, 0x2000, CRC(b74e9d43) SHA1(d6e9e05e2e652c9d467dba1f1501d2a7ec8f851c) )
	ROM_LOAD( "ac0-16.7h",    0x2000, 0x2000, CRC(10accc10) SHA1(311961bfe852582a9c66aaecf9bc4c8f0ac7fccf) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64K for 3nd z80 */
	ROM_LOAD( "ac10-12.3a",   0x0000, 0x2000, CRC(56eac59f) SHA1(22bde858ddcafad3f731030c39fd525458ecdbdd) )
	ROM_LOAD( "ac10-13.4a",   0x2000, 0x2000, CRC(3a920eaa) SHA1(256fafda0d522dee993b6840e60532f11a705345) )
	ROM_LOAD( "ac10-14.3d",   0x4000, 0x2000, CRC(819db933) SHA1(5e8b10d94ca6ba608a074bd5f30f14b95122fe85) )
	ROM_LOAD( "ac10-17.4d",   0x6000, 0x2000, CRC(87817985) SHA1(370399a4622958829ca6d1545e614b121f09c2c0) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 8741 */
	ROM_LOAD( "aa-013.5a",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x10000, REGION_CPU5, 0 )	/* 8741 */
	ROM_LOAD( "aa-016.9c",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )	/* 8741 */
	ROM_LOAD( "aa-017.9g",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ac1-10.9n",    0x0000, 0x2000, CRC(517c571b) SHA1(05572a8ea416922da50143936fda9ba038f0b91e) )	/* tiles */
	ROM_LOAD( "ac1-11.9p",    0x2000, 0x2000, CRC(7a1d8a3a) SHA1(3f90be9ddba3cf7a879fd69ac67c2b67fd63b9ee) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ac1-6.9e",     0x0000, 0x2000, CRC(1b0a3cb7) SHA1(0b0f17b9844d7310b46110559e09cfc3b50bb38b) )	/* sprites */

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ac0-7.9f",     0x0000, 0x2000, CRC(ef5f28c6) SHA1(85d943e5c5136d9458118f676b0c79fcf3aaf0c4) )
	ROM_LOAD( "ac0-8.9h",     0x2000, 0x2000, CRC(46824b30) SHA1(f6880b1c31ae795e3781d16ee96145df1db60328) )

	ROM_REGION( 0x0360, REGION_PROMS, 0 )
	ROM_LOAD( "ac0-1.11c",    0x0000, 0x0100, CRC(5c4b2adc) SHA1(0a6fdd60bdbd56bb7573147e4a976e5d0ddf43b5) )	/* palette low bits */
	ROM_LOAD( "ac0-2.11cd",   0x0100, 0x0100, CRC(966bda66) SHA1(05439508113b3e51a16ee87d3f4691aa8901ebcb) )	/* palette high bits */
	ROM_LOAD( "ac0-3.8c",     0x0200, 0x0100, CRC(dae13f77) SHA1(d4d105542955e806311987dd3c4ffce1e13caf91) )	/* sprite lookup table */
	ROM_LOAD( "003.4e",       0x0300, 0x0020, CRC(43a548b8) SHA1(d01529d7f8f5101232cdf3490fdb2c61bf179181) )	/* address decoder? not used */
	ROM_LOAD( "004.4d",       0x0320, 0x0020, CRC(43a548b8) SHA1(d01529d7f8f5101232cdf3490fdb2c61bf179181) )	/* address decoder? not used */
	ROM_LOAD( "005.3h",       0x0340, 0x0020, CRC(e8d6dec0) SHA1(d15cba9a4b24255d41046b15c2409391ab13ce95) )	/* address decoder? not used */
ROM_END

ROM_START( gsword2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64K for main CPU */
	ROM_LOAD( "ac1-1.2c",     0x0000, 0x2000, CRC(565c4d9e) SHA1(17b86e86ab95aeb458b8368c8c04666a1ccd9eee) )
	ROM_LOAD( "ac1-2.2d",     0x2000, 0x2000, CRC(d772accf) SHA1(08028c6f026c118cc375ecff5c24dcb549475633) )
	ROM_LOAD( "ac1-3.2e",     0x4000, 0x2000, CRC(2cee1871) SHA1(df099209c56f2807e4fdb83c625368f5e7e583e5) )
	ROM_LOAD( "ac1-4.2f",     0x6000, 0x2000, CRC(ca9d206d) SHA1(887eedc4e10218bf149c84399edd5d1e32c85051) )
	ROM_LOAD( "ac1-5.2h",     0x8000, 0x1000, CRC(2a892326) SHA1(a2cd91263714480c2569d3bbc73d62d222175e89) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for 2nd CPU */
	ROM_LOAD( "ac0-15.5h",    0x0000, 0x2000, CRC(1aa4690e) SHA1(7b0dbc38f3e6af2c9efa44b6759a3cdd9adc992d) )
	ROM_LOAD( "ac0-16.7h",    0x2000, 0x2000, CRC(10accc10) SHA1(311961bfe852582a9c66aaecf9bc4c8f0ac7fccf) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64K for 3nd z80 */
	ROM_LOAD( "ac0-12.3a",    0x0000, 0x2000, CRC(a6589068) SHA1(9385abe2449c5c5bac8f49d2afd140acea1791c3) )
	ROM_LOAD( "ac0-13.4a",    0x2000, 0x2000, CRC(4ee79796) SHA1(3353625903f63910a18fae0a9568a96d75592328) )
	ROM_LOAD( "ac0-14.3d",    0x4000, 0x2000, CRC(455364b6) SHA1(ebabf077d1ba113c13e7620d61720ed141acb5ad) )
	/* 6000-7fff empty */

	ROM_REGION( 0x10000, REGION_CPU4, 0 )	/* 8741 */
	ROM_LOAD( "aa-013.5a",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x10000, REGION_CPU5, 0 )	/* 8741 */
	ROM_LOAD( "aa-016.9c",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x10000, REGION_CPU6, 0 )	/* 8741 */
	ROM_LOAD( "aa-017.9g",    0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ac1-10.9n",    0x0000, 0x2000, CRC(517c571b) SHA1(05572a8ea416922da50143936fda9ba038f0b91e) )	/* tiles */
	ROM_LOAD( "ac1-11.9p",    0x2000, 0x2000, CRC(7a1d8a3a) SHA1(3f90be9ddba3cf7a879fd69ac67c2b67fd63b9ee) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ac1-6.9e",     0x0000, 0x2000, CRC(1b0a3cb7) SHA1(0b0f17b9844d7310b46110559e09cfc3b50bb38b) )	/* sprites */

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ac0-7.9f",     0x0000, 0x2000, CRC(ef5f28c6) SHA1(85d943e5c5136d9458118f676b0c79fcf3aaf0c4) )
	ROM_LOAD( "ac0-8.9h",     0x2000, 0x2000, CRC(46824b30) SHA1(f6880b1c31ae795e3781d16ee96145df1db60328) )

	ROM_REGION( 0x0360, REGION_PROMS, 0 )
	ROM_LOAD( "ac0-1.11c",    0x0000, 0x0100, CRC(5c4b2adc) SHA1(0a6fdd60bdbd56bb7573147e4a976e5d0ddf43b5) )	/* palette low bits */
	ROM_LOAD( "ac0-2.11cd",   0x0100, 0x0100, CRC(966bda66) SHA1(05439508113b3e51a16ee87d3f4691aa8901ebcb) )	/* palette high bits */
	ROM_LOAD( "ac0-3.8c",     0x0200, 0x0100, CRC(dae13f77) SHA1(d4d105542955e806311987dd3c4ffce1e13caf91) )	/* sprite lookup table */
	ROM_LOAD( "003.4e",       0x0300, 0x0020, CRC(43a548b8) SHA1(d01529d7f8f5101232cdf3490fdb2c61bf179181) )	/* address decoder? not used */
	ROM_LOAD( "004.4d",       0x0320, 0x0020, CRC(43a548b8) SHA1(d01529d7f8f5101232cdf3490fdb2c61bf179181) )	/* address decoder? not used */
	ROM_LOAD( "005.3h",       0x0340, 0x0020, CRC(e8d6dec0) SHA1(d15cba9a4b24255d41046b15c2409391ab13ce95) )	/* address decoder? not used */
ROM_END

ROM_START( josvolly )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64K for main CPU */
	ROM_LOAD( "aa2-1.2c",     0x0000, 0x2000, CRC(27f740a5) SHA1(3e038386e743fdf718e795a944ff4b631a492958) )
	ROM_LOAD( "aa1-2.2d",     0x2000, 0x2000, CRC(3e02e3e1) SHA1(cc0aee321cf5232438cd6e38635c9060056ad361) )
	ROM_LOAD( "aa0-3.2e",     0x4000, 0x2000, CRC(72843ffe) SHA1(fe70727bbcb0622df81eca2969c1a85398767479) )
	ROM_LOAD( "aa1-4.2f",     0x6000, 0x2000, CRC(22c1466e) SHA1(d86093903e473252c35170e35d7f9ee34194086d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64K for 2nd CPU */
	ROM_LOAD( "aa3-12.2h",    0x0000, 0x1000, CRC(3796bbf6) SHA1(8741f556ddb06e7779d1e8abc3d06688881f8269) )
	ROM_LOAD( "aa0-13.2j",    0x2000, 0x2000, CRC(58cc89ac) SHA1(9785ec27e593b3e249da7a1b6b025c6d573e28f9) )

	ROM_REGION( 0x04000, REGION_USER1, 0 )	/* music data and samples - not sure where it's mapped */
	ROM_LOAD( "aa0-14.4j",    0x0000, 0x2000, CRC(436fe91f) SHA1(feb29501090c6db911e13ce6e9935ba004b0ce7e) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "aa0-10.9n",    0x0000, 0x2000, CRC(207c4f42) SHA1(4cf2922d55cfc9e68cc07c3252ea3b5619b8aca5) )	/* tiles */
	ROM_LOAD( "aa1-11.9p",    0x2000, 0x1000, CRC(c130464a) SHA1(9d23577b8aaaffeefff3d8f93668d1b2bd0ba3d9) )
    ROM_RELOAD(               0x3000, 0x1000 ) // title screen data is actually read from here

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "aa0-6.9e",     0x0000, 0x2000, CRC(c2c2401a) SHA1(ef987d53d9e502277086f39b455174d3539572e6) )	/* sprites */

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "aa0-7.9f",     0x0000, 0x2000, CRC(da836231) SHA1(209723778b705dba8206b56c3b8f0996f02ba8d5) )
	ROM_LOAD( "aa0-8.9h",     0x2000, 0x2000, CRC(a0426d57) SHA1(d029408e005ea57f4902c081203f3d3980a5f927) )

	ROM_REGION( 0x0460, REGION_PROMS, 0 )
	ROM_LOAD( "a1.10k",       0x0000, 0x0100, CRC(09f7b56a) SHA1(9b82d1d4ebab14b366dc0ca95c933e37811ac155) )	/* palette red? */
	ROM_LOAD( "a2.9k",        0x0100, 0x0100, CRC(852eceac) SHA1(6ed7011b45cf767d6503b92d29a14a7b8e099a76) )	/* palette green? */
	ROM_LOAD( "a3.9j",        0x0200, 0x0100, CRC(1312718b) SHA1(4a7d7eae4d8ea085eead46758832fddac7aff0b0) )	/* palette blue? */
	ROM_LOAD( "a4.8c",        0x0300, 0x0100, CRC(1dcec967) SHA1(4d36842c2fd929a6508a58bc8ea7e0372296e575) )	/* sprite lookup table */
	ROM_LOAD( "003.4e",       0x0400, 0x0020, CRC(43a548b8) SHA1(d01529d7f8f5101232cdf3490fdb2c61bf179181) )	/* address decoder? not used */
	ROM_LOAD( "004.4d",       0x0420, 0x0020, CRC(43a548b8) SHA1(d01529d7f8f5101232cdf3490fdb2c61bf179181) )	/* address decoder? not used */
	ROM_LOAD( "005.3h",       0x0440, 0x0020, CRC(e8d6dec0) SHA1(d15cba9a4b24255d41046b15c2409391ab13ce95) )	/* address decoder? not used */
ROM_END

static DRIVER_INIT( gsword )
{
#if 0
	UINT8 *ROM2 = memory_region(REGION_CPU2);
	ROM2[0x1da] = 0xc3; /* patch for rom self check */

	ROM2[0x71e] = 0;    /* patch for sound protection or time out function */
	ROM2[0x71f] = 0;
#endif
#if 1
	/* hack for sound protection or time out function */
	memory_install_read8_handler(1, ADDRESS_SPACE_PROGRAM, 0x4004, 0x4005, 0, 0, gsword_hack_r);
#endif
}

static DRIVER_INIT( gsword2 )
{
#if 0
	UINT8 *ROM2 = memory_region(REGION_CPU2);

	ROM2[0x1da] = 0xc3; /* patch for rom self check */
	ROM2[0x726] = 0;    /* patch for sound protection or time out function */
	ROM2[0x727] = 0;
#endif
#if 1
	/* hack for sound protection or time out function */
	memory_install_read8_handler(1, ADDRESS_SPACE_PROGRAM, 0x4004, 0x4005, 0, 0, gsword_hack_r);
#endif
}

static DRIVER_INIT(josvolly)
{
}

GAME( 1984, gsword,   0,      gsword,   gsword,   gsword,   ROT0,  "Taito Corporation", "Great Swordsman (World?)", 0 )
GAME( 1984, gsword2,  gsword, gsword,   gsword,   gsword2,  ROT0,  "Taito Corporation", "Great Swordsman (Japan?)", 0 )
GAME( 1983, josvolly, 0,      josvolly, josvolly, josvolly, ROT90, "Taito Corporation", "Joshi Volleyball", GAME_UNEMULATED_PROTECTION|GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_SOUND )
