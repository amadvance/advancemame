/****************************************************************************

    Forty-Love (c) Taito 1984

    driver by Jaroslaw Burczynski

****************************************************************************/

/*
    TO DO:
    - sprites graphics decoding could be changed to only use
      color codes 8-15 (now it decodes all 64 colors). Perhaps the same
      applies to character graphics (colors 0-7 only),

    - sprite memory needs to be buffered ?

    - controls may be wrong (BUTTON 3 - not used ?)

    - palette has 1024 colors, but only first 256 are used at the moment

    - pixel layer needs priority ?
*/

/*
Game                     : 40-0  (Forty-Love Tennis game)
Manufacturer             : Taito
Game number              : A30
Vintage                  : 1984
Game serial/model number : M4300006B ?

I dont have the wiring harness for this board, so dont know if it works.
One GFX ROM is bad though.
See A30-26.u23\A30-26.txt for details about the bad ROM.

This is a four board system - Main, Video, ROM, and Sound boards.

Details:
  Main board:    J1100004A / K1100010A <--label with K1100026A covers these
    1 x NEC D780C-1 (Z80) CPU
    1 x Motorola M68705P5S MCU
    1 x Mitsubishi M5517P SRAM chip        1 x (8bit x 2048) used
    1 x Fujitsu MB14241 ??? chip           ( DAC ????? )
    4 x Fujitsu MB81416-10 DRAM chips      4 x (4bit x 16384) 1/2 used
    1 x TD62003P (lamps/LEDs driver)
    1 x 8MHz xtal
    3 x 8way DSW

  Sound board:   J1100005A / K1100011A
    1 x NEC D780C-1 (Z80) CPU
    1 x Mitsubishi M5517P SRAM chip        1 x (8bit x 2048) used
    1 x Yamaha YM2149
    1 x OKI M5232
    1 x Fujitsu MB3731 Audio amp
    1 x 8MHz xtal

  Video board:   J1100008A / K1100025A
    4 x AMD AM93422 RAM chips              4 x (4bit x 256)     1/2 used
    2 x Mitsubishi M5517P SRAM chips       2 x (8bit x 2048)    1/2 used
    6 x Mitsubishi M53357P (=LS157)
    1 x 18.432MHz xtal

  ROM board:     J9100005A / K9100008A


ROMS:       Programmer   Device           Legend:
             Checksum                ___
A30-08.u08     AD5C       2764          \
A30-09.u09     C1E4       2764           \
A30-10.u10     C6B1       2764            \ Sound Board ROMs
A30-11.u37     ACD8       2764            /
A30-12.u38     B7C4       2764           /
A30-13.u39     6B43       2764       ___/
A30-14.u41                M68705P5S  ___> Main board MCU
A30-15.u03     2F6E       6353          \
A30-16.u01     2E97       6353           \ Video board BPROMs
A30-17.u02     2CD3       6353           / All read as 82S137
A30-18.u13     15E5       7643       ___/
A30-19.ic1     C88C       2764          \
A30-20.ic2     7B40       2764           \  ROM board ROMs.
A30-21.ic3     E2B4       2764            \ These are Program ROMs for
A30-22.ic4     8937       2764            / the main board
A30-23.ic5     A6F6       2764           /
A30-24.ic6     75DC       2764       ___/
A30-25.u22     1903       2764          \
A30-26.u23      ??        2764  BAD !    \
A30-27.u24     005B       2764            \
A30-28.u25     279F       2764             \ Video board GFX ROMs
A30-29.u62     BAA4       2764             /
A30-30.u63     0BB6       2764            /
A30-31.u64     461C       2764           /
A30-32.u65     E764       2764       ___/

Notes,
Programmer used: HiLo All-08A programmer.

Q and P connectors, provide connection between the main and video board,
via ribbon cables.
The following are board layouts, and component locations.

     Main-Board    J1100004A / K1100010A <--label with K1100026A covers these
 +--------------------------------------------------+
 |                           DSW1 DSW2 DSW3         |
 |  5517       Z80                                  |
 =                                                  |
 =  ROMskt                                          |
 P                                                  ==
 =  ROMskt     68705P5 (A30-14)                     ==
 =                                                  ==  Wiring harness
 |  ROMskt     MB14241            TD62003           ==  connector
 |                                                  ==
 =                                                  ==
 =                                                  ==
 Q       81416                 Ribbon    =          |
 =       81416                 cable     =          ===  Wiring harness
 =       81416                 to sound  S          ===  connector
 |       81416                 board --> =          ===
 | 8MHz                                  =          |
 +--------------------------------------------------+

     ROM board     J9100005A / K9100008A

 +------------------------+
 |                        |
 | ROMskt  A30-23  A30-24 |
 |                        |
 | ROMskt  A30-21  A30-22 |
 |                        |
 | ROMskt  A30-19  A30-20 |
 +------------------------+


     Video Board   J1100008A / K1100025A
 +--------------------------------------------------+
 |          A30-32                         A30-15   |
 |                           A30-28 A30-18 A30-17   |
 =          A30-31                         A30-16   |
 =                           A30-27                 |
 Q          A30-30                                  |
 =                           A30-26                 ==
 =18.432MHz A30-29                                  ==
 |     53357      53357      A30-25                 ==  Wiring harness
 |                                                  ==  connector
 =     53357  5517                                  ==
 =     53357                           53357        ==
 P     53357                     93422              |
 =                               93422              |
 =                               93422              |
 |            5517               93422              |
 |                                                  |
 +--------------------------------------------------+

     Sound Board   J1100005A / K1100011A
     (The following representation is rotated 90 degrees anti-clockwise)
                               _______  Ribbon cable
 +------------------------+   \|/       to main board
 | A30-13  A30-14  A30-15 +----------+
 |                          |||S|||  |
 |                  Z80              |
 | YM2149                     MB3731 |
 |                  A30-08           |
 |                  A30-09           |
 | M5232            A30-10           |
 |                  5517             |
 |                                   |
 |                                   |
 | 8MHz                              |
 +-----------------------------------+

     Side view

       --Sound board-------------------
    -----ROM board---
 --------Main board--------------------------------
 --------Video board-------------------------------

Details by Quench

*************************************************************

FieldDay by Taito

Same board as 40-Love

M4300048A

18.432 mhz crystal
2x m5m5517
4x am93422 (2101)

A17-15->18 6353 1024x4 prom

M4300049A (relabeled J1100004A/K1100010A)

8 mhz crystal
4x 4416
1x m5m5517
z80c

a17_14 protection processor. 28 Pin Motorolla 15-00011-01 DA68235 (Labeled 8909)
Next to MB14241

Rom Daughterboard

K9100013A (relabeled J9100005A/K9100008A)

Sound Board

K1100024A (relabeled J1100005A/K1100011A)

8 Mhz crystal
DZ80C
YM2149
Oki M5232 (6532?)

Notes - Has jumper setting for 122HZ or 61HZ)

*/

#include <math.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "sound/msm5232.h"

/* in machine/buggychl.c */
READ8_HANDLER( buggychl_68705_portA_r );
WRITE8_HANDLER( buggychl_68705_portA_w );
WRITE8_HANDLER( buggychl_68705_ddrA_w );
READ8_HANDLER( buggychl_68705_portB_r );
WRITE8_HANDLER( buggychl_68705_portB_w );
WRITE8_HANDLER( buggychl_68705_ddrB_w );
READ8_HANDLER( buggychl_68705_portC_r );
WRITE8_HANDLER( buggychl_68705_portC_w );
WRITE8_HANDLER( buggychl_68705_ddrC_w );
WRITE8_HANDLER( buggychl_mcu_w );
READ8_HANDLER( buggychl_mcu_r );
READ8_HANDLER( buggychl_mcu_status_r );


extern VIDEO_START( fortyl );
extern VIDEO_UPDATE( fortyl );
extern PALETTE_INIT( fortyl );

extern WRITE8_HANDLER( fortyl_bg_videoram_w );
extern WRITE8_HANDLER( fortyl_bg_colorram_w );
extern READ8_HANDLER ( fortyl_bg_videoram_r );
extern READ8_HANDLER ( fortyl_bg_colorram_r );
extern WRITE8_HANDLER( fortyl_pixram_sel_w );
extern READ8_HANDLER( fortyl_pixram_r );
extern WRITE8_HANDLER( fortyl_pixram_w );

extern unsigned char *fortyl_video_ctrl;
extern int fortyl_pix_color[4];


static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
	else pending_nmi = 1;
}

static WRITE8_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

static WRITE8_HANDLER( nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE8_HANDLER( nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
		pending_nmi = 0;
	}
}



#if 0
static WRITE8_HANDLER( fortyl_coin_counter_w )
{
	coin_counter_w(offset,data);
}
#endif


static READ8_HANDLER( fortyl_mcu_r )
{
	return buggychl_mcu_r(offset);
}

static READ8_HANDLER( fortyl_mcu_status_r )
{
	return buggychl_mcu_status_r(offset);
}

static WRITE8_HANDLER( fortyl_mcu_w )
{
	buggychl_mcu_w(offset,data);
}

static WRITE8_HANDLER( bank_select_w )
{

	if ((data!=0x02) && (data!=0xfd))
	{
//      logerror("WRONG BANK SELECT = %x !!!!\n",data);
//      ui_popup("WRONG BANK SELECT = %x !!!!\n",data);
	}

	memory_set_bank( 1, data&1 );
}


static UINT8 pix1;
static UINT8 pix2[2];

static WRITE8_HANDLER( pix1_w )
{
//  if ( data > 7 )
//      logerror("pix1 = %2x\n",data);

	pix1 = data;
}
static WRITE8_HANDLER( pix2_w )
{
//  if ( (data!=0x00) && (data!=0xff) )
//      logerror("pix2 = %2x\n",data);

	pix2[0] = pix2[1];
	pix2[1] = data;
}

#if 0
static READ8_HANDLER( pix1_r )
{
	return pix1;
}
#endif

static READ8_HANDLER( pix2_r )
{
	int res;
	int d1 = pix1 & 7;

	res = (((pix2[1] << (d1+8)) | (pix2[0] << d1)) & 0xff00) >> 8;

	return res;
}


static DRIVER_INIT( undoukai )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 2, &ROM[0x10000], 0x2000);

	fortyl_pix_color[0] = 0x000;
	fortyl_pix_color[1] = 0x1e3;
	fortyl_pix_color[2] = 0x16c;
	fortyl_pix_color[3] = 0x1ec;
}

static DRIVER_INIT( 40love )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	memory_configure_bank(1, 0, 2, &ROM[0x10000], 0x2000);

	#if 0
		/* character ROM hack
            to show a white line on the opponent side */

		UINT8 *ROM = memory_region(REGION_GFX2);
		int adr = 0x10 * 0x022b;
		ROM[adr+0x000a] = 0x00;
		ROM[adr+0x000b] = 0x00;
		ROM[adr+0x400a] = 0x00;
		ROM[adr+0x400b] = 0x00;
	#endif

	fortyl_pix_color[0] = 0x000;
	fortyl_pix_color[1] = 0x1e3;
	fortyl_pix_color[2] = 0x16c;
	fortyl_pix_color[3] = 0x1ec;
}

/***************************************************************************/

static UINT8 snd_data;
static UINT8 snd_flag;

static READ8_HANDLER( from_snd_r )
{
	snd_flag = 0;
	return snd_data;
}

static READ8_HANDLER( snd_flag_r )
{
	return snd_flag | 0xfd;
}

static WRITE8_HANDLER( to_main_w )
{
	snd_data = data;
	snd_flag = 2;
}

/***************************************************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)	/* M5517P on main board */
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_BANK1)

	AM_RANGE(0x9000, 0x97ff) AM_READ(fortyl_bg_videoram_r)	/* #1 M5517P on video board */
	AM_RANGE(0x9800, 0x983f) AM_READ(MRA8_RAM)				/* video control area */
	AM_RANGE(0x9840, 0x987f) AM_READ(MRA8_RAM)				/* sprites part 1 */
	AM_RANGE(0x9880, 0x98bf) AM_READ(fortyl_bg_colorram_r)	/* background attributes */
	AM_RANGE(0x98c0, 0x98ff) AM_READ(MRA8_RAM)				/* sprites part 2 */

	AM_RANGE(0xc000, 0xffff) AM_READ(fortyl_pixram_r)


	AM_RANGE(0x8800, 0x8800) AM_READ(fortyl_mcu_r)
	AM_RANGE(0x8801, 0x8801) AM_READ(fortyl_mcu_status_r)

	//AM_RANGE(0x8802, 0x8802) AM_READ(pix1_r) // this one is here just for debugging
	AM_RANGE(0x8803, 0x8803) AM_READ(pix2_r) // and this one _is_ used.

	AM_RANGE(0x8804, 0x8804) AM_READ(from_snd_r)
	AM_RANGE(0x8805, 0x8805) AM_READ(snd_flag_r)
	AM_RANGE(0x8807, 0x8807) AM_READ(MRA8_NOP) /* unknown */

	AM_RANGE(0x8808, 0x8808) AM_READ(input_port_2_r)
	AM_RANGE(0x8809, 0x8809) AM_READ(input_port_4_r)
	AM_RANGE(0x880a, 0x880a) AM_READ(input_port_3_r)
	AM_RANGE(0x880b, 0x880b) AM_READ(input_port_5_r)
	AM_RANGE(0x880c, 0x880c) AM_READ(input_port_0_r)
	AM_RANGE(0x880d, 0x880d) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)	/* M5517P on main board */
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_ROM)

	AM_RANGE(0x9000, 0x97ff) AM_WRITE(fortyl_bg_videoram_w) AM_BASE(&videoram)		/* #1 M5517P on video board */
	AM_RANGE(0x9800, 0x983f) AM_WRITE(MWA8_RAM) AM_BASE(&fortyl_video_ctrl)			/* video control area */
	AM_RANGE(0x9840, 0x987f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)	/* sprites part 1 */
	AM_RANGE(0x9880, 0x98bf) AM_WRITE(fortyl_bg_colorram_w) AM_BASE(&colorram)		/* background attributes (2 bytes per line) */
	AM_RANGE(0x98c0, 0x98ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2) AM_SIZE(&spriteram_2_size)/* sprites part 2 */

	AM_RANGE(0xc000, 0xffff) AM_WRITE(fortyl_pixram_w) /* banked pixel layer */


	AM_RANGE(0x8800, 0x8800) AM_WRITE(fortyl_mcu_w)
	AM_RANGE(0x8801, 0x8801) AM_WRITE(pix1_w)		//pixel layer related
	AM_RANGE(0x8802, 0x8802) AM_WRITE(bank_select_w)
	AM_RANGE(0x8803, 0x8803) AM_WRITE(pix2_w)		//pixel layer related
	AM_RANGE(0x8804, 0x8804) AM_WRITE(sound_command_w)
//  AM_RANGE(0x8805, 0x8805) AM_WRITE(MWA8_NOP /*sound_reset*/) //????
	AM_RANGE(0x880c, 0x880c) AM_WRITE(fortyl_pixram_sel_w) /* pixram bank select */
	AM_RANGE(0x880d, 0x880d) AM_WRITE(MWA8_NOP) /* unknown */

ADDRESS_MAP_END

static ADDRESS_MAP_START( undoukai_readmem, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xa000, 0xa7ff) AM_READ(MRA8_RAM)	/* M5517P on main board */

	AM_RANGE(0xb000, 0xb7ff) AM_READ(fortyl_bg_videoram_r)	/* #1 M5517P on video board */
	AM_RANGE(0xb800, 0xb83f) AM_READ(MRA8_RAM)				/* video control area */
	AM_RANGE(0xb840, 0xb87f) AM_READ(MRA8_RAM)				/* sprites part 1 */
	AM_RANGE(0xb880, 0xb8bf) AM_READ(fortyl_bg_colorram_r)	/* background attributes */
	AM_RANGE(0xb8c0, 0xb8ff) AM_READ(MRA8_RAM)				/* sprites part 2 */

	AM_RANGE(0xc000, 0xffff) AM_READ(fortyl_pixram_r)


	AM_RANGE(0xa800, 0xa800) AM_READ(fortyl_mcu_r)
	AM_RANGE(0xa801, 0xa801) AM_READ(fortyl_mcu_status_r)

	//AM_RANGE(0xa802, 0xa802) AM_READ(pix1_r) // this one is here just for debugging
	AM_RANGE(0xa803, 0xa803) AM_READ(pix2_r) // and this one _is_ used.

	AM_RANGE(0xa804, 0xa804) AM_READ(from_snd_r)
	AM_RANGE(0xa805, 0xa805) AM_READ(snd_flag_r)
	AM_RANGE(0xa807, 0xa807) AM_READ(MRA8_NOP) /* unknown */

	AM_RANGE(0xa808, 0xa808) AM_READ(input_port_2_r)
	AM_RANGE(0xa809, 0xa809) AM_READ(input_port_4_r)
	AM_RANGE(0xa80a, 0xa80a) AM_READ(input_port_3_r)
	AM_RANGE(0xa80b, 0xa80b) AM_READ(input_port_5_r)
	AM_RANGE(0xa80c, 0xa80c) AM_READ(input_port_0_r)
	AM_RANGE(0xa80d, 0xa80d) AM_READ(input_port_1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( undoukai_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xa7ff) AM_WRITE(MWA8_RAM)	/* M5517P on main board */

	AM_RANGE(0xa800, 0xa800) AM_WRITE(fortyl_mcu_w)
	AM_RANGE(0xa801, 0xa801) AM_WRITE(pix1_w)		//pixel layer related
	AM_RANGE(0xa802, 0xa802) AM_WRITE(bank_select_w)
	AM_RANGE(0xa803, 0xa803) AM_WRITE(pix2_w)		//pixel layer related
	AM_RANGE(0xa804, 0xa804) AM_WRITE(sound_command_w)
	AM_RANGE(0xa805, 0xa805) AM_WRITE(MWA8_NOP) /*sound_reset*/	//????
	AM_RANGE(0xa807, 0xa807) AM_WRITE(MWA8_NOP) /* unknown */
	AM_RANGE(0xa80c, 0xa80c) AM_WRITE(fortyl_pixram_sel_w) /* pixram bank select */
	AM_RANGE(0xa80d, 0xa80d) AM_WRITE(MWA8_NOP) /* unknown */

	AM_RANGE(0xb000, 0xb7ff) AM_WRITE(fortyl_bg_videoram_w) AM_BASE(&videoram)		/* #1 M5517P on video board */
	AM_RANGE(0xb800, 0xb83f) AM_WRITE(MWA8_RAM) AM_BASE(&fortyl_video_ctrl)			/* video control area */
	AM_RANGE(0xb840, 0xb87f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)	/* sprites part 1 */
	AM_RANGE(0xb880, 0xb8bf) AM_WRITE(fortyl_bg_colorram_w) AM_BASE(&colorram)		/* background attributes (2 bytes per line) */
	AM_RANGE(0xb8e0, 0xb8ff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram_2) AM_SIZE(&spriteram_2_size)/* sprites part 2 */

	AM_RANGE(0xc000, 0xffff) AM_WRITE(fortyl_pixram_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readmem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xd800, 0xd800) AM_READ(soundlatch_r)
	AM_RANGE(0xda00, 0xda00) AM_READ(MRA8_NOP) /* unknown */
	AM_RANGE(0xde00, 0xde00) AM_READ(MRA8_NOP) /* unknown */
	AM_RANGE(0xe000, 0xefff) AM_READ(MRA8_ROM)	/* space for diagnostics ROM */
ADDRESS_MAP_END

static int vol_ctrl[16];

static MACHINE_RESET( ta7630 )
{
	int i;

	double db			= 0.0;
	double db_step		= 1.50;	/* 1.50 dB step (at least, maybe more) */
	double db_step_inc	= 0.125;
	for (i=0; i<16; i++)
	{
		double max = 100.0 / pow(10.0, db/20.0 );
		vol_ctrl[ 15-i ] = max;
		/*logerror("vol_ctrl[%x] = %i (%f dB)\n",15-i,vol_ctrl[ 15-i ],db);*/
		db += db_step;
		db_step += db_step_inc;
	}

	/* for (i=0; i<8; i++)
        logerror("SOUND Chan#%i name=%s\n", i, mixer_get_name(i) ); */
/*
  channels 0-2 AY#0
  channels 3,4 MSM5232 group1,group2
*/
}

static UINT8 snd_ctrl0=0;
static UINT8 snd_ctrl1=0;
static UINT8 snd_ctrl2=0;
static UINT8 snd_ctrl3=0;

static WRITE8_HANDLER( sound_control_0_w )
{
	snd_ctrl0 = data & 0xff;
//  ui_popup("SND0 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);

	/* this definitely controls main melody voice on 2'-1 and 4'-1 outputs */
	sndti_set_output_gain(SOUND_MSM5232, 0, 0, vol_ctrl[ (snd_ctrl0>>4) & 15 ] / 100.0);	/* group1 from msm5232 */

}
static WRITE8_HANDLER( sound_control_1_w )
{
	snd_ctrl1 = data & 0xff;
//  ui_popup("SND1 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);
	sndti_set_output_gain(SOUND_MSM5232, 0, 1, vol_ctrl[ (snd_ctrl1>>4) & 15 ] / 100.0);	/* group2 from msm5232 */
}

static WRITE8_HANDLER( sound_control_2_w )
{
	int i;

	snd_ctrl2 = data & 0xff;
//  ui_popup("SND2 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);

	for (i=0; i<3; i++)
		sndti_set_output_gain(SOUND_AY8910, 0, i, vol_ctrl[ (snd_ctrl2>>4) & 15 ] / 100.0);	/* ym2149f all */
}

static WRITE8_HANDLER( sound_control_3_w ) /* unknown */
{
	snd_ctrl3 = data & 0xff;
//  ui_popup("SND3 0=%02x 1=%02x 2=%02x 3=%02x", snd_ctrl0, snd_ctrl1, snd_ctrl2, snd_ctrl3);
}

static ADDRESS_MAP_START( writemem_sound, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc800, 0xc800) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xc801, 0xc801) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xca00, 0xca0d) AM_WRITE(MSM5232_0_w)
	AM_RANGE(0xcc00, 0xcc00) AM_WRITE(sound_control_0_w)
	AM_RANGE(0xce00, 0xce00) AM_WRITE(sound_control_1_w)
	AM_RANGE(0xd800, 0xd800) AM_WRITE(to_main_w)
	AM_RANGE(0xda00, 0xda00) AM_WRITE(nmi_enable_w)
	AM_RANGE(0xdc00, 0xdc00) AM_WRITE(nmi_disable_w)
	AM_RANGE(0xde00, 0xde00) AM_WRITE(DAC_0_signed_data_w)		/* signed 8-bit DAC */
	AM_RANGE(0xe000, 0xefff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( mcu_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_READ(buggychl_68705_portA_r)
	AM_RANGE(0x0001, 0x0001) AM_READ(buggychl_68705_portB_r)
	AM_RANGE(0x0002, 0x0002) AM_READ(buggychl_68705_portC_r)
	AM_RANGE(0x0010, 0x007f) AM_READ(MRA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(11) )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(buggychl_68705_portA_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(buggychl_68705_portB_w)
	AM_RANGE(0x0002, 0x0002) AM_WRITE(buggychl_68705_portC_w)
	AM_RANGE(0x0004, 0x0004) AM_WRITE(buggychl_68705_ddrA_w)
	AM_RANGE(0x0005, 0x0005) AM_WRITE(buggychl_68705_ddrB_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(buggychl_68705_ddrC_w)
	AM_RANGE(0x0010, 0x007f) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0080, 0x07ff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END


INPUT_PORTS_START( 40love )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, "DSW1 Unknown 0" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DSW1 Unknown 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x18, "4" )
	PORT_DIPNAME( 0x20, 0x00, "DSW1 Unknown 5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW2") /* All OK */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START_TAG("DSW3")
	PORT_DIPNAME( 0x03, 0x00, "DSW3 Unknown 0" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x01, "01" )
	PORT_DIPSETTING(    0x02, "02" )
	PORT_DIPSETTING(    0x03, "03" )
	PORT_DIPNAME( 0x0c, 0x0c, "DSW3 Unknown 1" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x04, "04" )
	PORT_DIPSETTING(    0x08, "08" )
	PORT_DIPSETTING(    0x0c, "0c" )
	PORT_DIPNAME( 0x10, 0x10, "Display Credit Settings" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Score points to: (Cheat)")
	PORT_DIPSETTING(    0x40, "Winner" )
	PORT_DIPSETTING(    0x00, "Human" )
	PORT_DIPNAME( 0x80, 0x00, "Coin Door Type" )
	PORT_DIPSETTING(    0x00, "Single Slot" )
	PORT_DIPSETTING(    0x80, "Double Slot" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	//??
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	//??
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_COIN1 )	//OK
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,IPT_COIN2 )	//OK
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )	//OK
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )	//OK
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )	//OK
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )	//OK

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_COCKTAIL
INPUT_PORTS_END

INPUT_PORTS_START( undoukai )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, DEF_STR( None ) )
	PORT_DIPSETTING(    0x00, "100000 200000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Players ) )
	PORT_DIPSETTING(    0x20, "1 or 2" )
	PORT_DIPSETTING(    0x00, "1 to 4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START_TAG("DSW2") /*All OK */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START_TAG("DSW3") /* & START */
	PORT_BIT(           0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(           0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Freeze" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Display Credit Settings" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "No Qualify (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Coin Door Type" )
	PORT_DIPSETTING(    0x00, "Single Slot" )
	PORT_DIPSETTING(    0x80, "Double Slot" )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH,IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static const gfx_layout char_layout =
{
	8,8,
	0x400,
	4,
	{ 2*0x2000*8+0, 2*0x2000*8+4, 0,4 },
	{ 3,2,1,0, 11,10,9,8 },
	{ 0*8,2*8,4*8,6*8,8*8,10*8,12*8,14*8 },
	16*8
};

static const gfx_layout sprite_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0,4 },

	{ 3,2,1,0, 11,10,9,8,
		16*8+3, 16*8+2, 16*8+1, 16*8+0, 16*8+11, 16*8+10, 16*8+9, 16*8+8 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &char_layout, 0, 64 },
	{ REGION_GFX1, 0, &sprite_layout, 0, 64 },
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	0,
	0,
	sound_control_2_w,
	sound_control_3_w
};

static struct MSM5232interface msm5232_interface =
{
	{ 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6, 1.0e-6 }	/* 1.0 uF capacitors (verified on real PCB) */
};

/*******************************************************************************/

static MACHINE_DRIVER_START( 40love )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2) /* OK */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */ /* OK */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* source/number of IRQs is unknown */

	MDRV_CPU_ADD(M68705,18432000/6) /* OK */
	MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* high interleave to ensure proper synchronization of CPUs */
	MDRV_MACHINE_RESET(ta7630)	/* init machine */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(128,128+255, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(fortyl)
	MDRV_VIDEO_START(fortyl)
	MDRV_VIDEO_UPDATE(fortyl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(MSM5232, 8000000/4)
	MDRV_SOUND_CONFIG(msm5232_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( undoukai )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2)
	MDRV_CPU_PROGRAM_MAP(undoukai_readmem,undoukai_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,8000000/2)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* source/number of IRQs is unknown */

        MDRV_CPU_ADD(M68705,18432000/6)
        MDRV_CPU_PROGRAM_MAP(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* high interleave to ensure proper synchronization of CPUs */
	MDRV_MACHINE_RESET(ta7630)	/* init machine */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(128,128+255, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(fortyl)
	MDRV_VIDEO_START(fortyl)
	MDRV_VIDEO_UPDATE(fortyl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)

	MDRV_SOUND_ADD(MSM5232, 8000000/4)
	MDRV_SOUND_CONFIG(msm5232_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)
MACHINE_DRIVER_END

/*******************************************************************************/

ROM_START( 40love )
	ROM_REGION( 0x14000, REGION_CPU1, 0 ) /* Z80 main CPU */
	ROM_LOAD( "a30-19.ic1", 0x00000, 0x2000, CRC(7baca598) SHA1(b1767f5af9b3f484afb4423afe1f9c15db92c2ac) )
	ROM_LOAD( "a30-20.ic2", 0x02000, 0x2000, CRC(a7b4f2cc) SHA1(67f570874fa0feb21f2a9a0712fadf78ebaad91c) )
	ROM_LOAD( "a30-21.ic3", 0x04000, 0x2000, CRC(49a372e8) SHA1(7c15fac65369d2e90b432c0f5c8e1d7295c379d1) )
	ROM_LOAD( "a30-22.ic4", 0x06000, 0x2000, CRC(0c06d2b3) SHA1(e5b0c8e57b0a6d131496e168023e12bacc17e93e) )
	ROM_LOAD( "a30-23.ic5", 0x10000, 0x2000, CRC(6dcd186e) SHA1(c8d88a2f35ba77ea822bdd8133033c8eb0bb5f72) )	/* banked at 0xa000 */
	ROM_LOAD( "a30-24.ic6", 0x12000, 0x2000, CRC(590c20c8) SHA1(93689d6a299dfbe33ffec42d13378091d8589b34) )	/* banked at 0xa000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 sound CPU */
	ROM_LOAD( "a30-08.u08", 0x0000, 0x2000, CRC(2fc42ee1) SHA1(b56e5f9acbcdc476252e188f41ad7249dba6f8e1) )
	ROM_LOAD( "a30-09.u09", 0x2000, 0x2000, CRC(3a75abce) SHA1(ad2df26789d38196c0677c22ab8f176e99604b18) )
	ROM_LOAD( "a30-10.u10", 0x4000, 0x2000, CRC(393c4b5b) SHA1(a8e1dd5c33e929bc832cccc13b85ecd13fff1eb2) )
	ROM_LOAD( "a30-11.u37", 0x6000, 0x2000, CRC(11b2c6d2) SHA1(d55690512a37c4df2386a845e0cfb14f8052295b) )
	ROM_LOAD( "a30-12.u38", 0x8000, 0x2000, CRC(f7afd475) SHA1(dd09d5ca7fec5e0454f9efb8ebc722561010f124) )
	ROM_LOAD( "a30-13.u39", 0xa000, 0x2000, CRC(e806630f) SHA1(09022aae88ea0171a0aacf3260fa3a95e8faeb21) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "a30-14"    , 0x0000, 0x0800, CRC(c4690279) SHA1(60bc77e03b9be434bb97a374a2fedeb8d049a660) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a30-25.u22", 0x0000, 0x2000, CRC(15e594cf) SHA1(d2d506a55f6ac2c191e5d5b3127021cde366c71c) )
	ROM_LOAD( "415f.26", 0x2000, 0x2000, BAD_DUMP CRC(3a45a205) SHA1(0939ecaabbb9be2a0719ef252e3f244299734ba6)  )	/* this actually seems good, but we need to find another one to verify */
	ROM_LOAD( "a30-27.u24", 0x4000, 0x2000, CRC(57c67f6f) SHA1(293e5bfa7c859886abd70f78fe2e4b13a3fce3f5) )
	ROM_LOAD( "a30-28.u25", 0x6000, 0x2000, CRC(d581d067) SHA1(ce132cf2503917f0846b838c6ce4ad4183181bf9) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a30-29.u62", 0x0000, 0x2000, CRC(02deaf40) SHA1(fb424a40bd9d959664a6d1ddf477fc16e694b9fa) )
	ROM_LOAD( "a30-30.u63", 0x2000, 0x2000, CRC(439f3731) SHA1(4661149baa8472989cc8ac85c51e55df69957d99) )
	ROM_LOAD( "a30-31.u64", 0x4000, 0x2000, CRC(7ed70e81) SHA1(f90a3ce701ebe746803cf01ea1f6725c552007de) )
	ROM_LOAD( "a30-32.u65", 0x6000, 0x2000, CRC(0434655b) SHA1(261c5e60e830967564c053dc1d40fbf1e7194fc8) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "a30-15.u03", 0x0000, 0x0400, CRC(55e38cc7) SHA1(823a6d7f29eadf5d12702d782d4297b0d4c65a0e) )	/* red */
	ROM_LOAD( "a30-16.u01", 0x0400, 0x0400, CRC(13997e20) SHA1(9fae1cf633409a88263dc66a17b1c2eeccd05f4f) )	/* green */
	ROM_LOAD( "a30-17.u02", 0x0800, 0x0400, CRC(5031f2f3) SHA1(1836d82fdc9f39cb318a791af2a935c27baabfd7) )	/* blue */
	ROM_LOAD( "a30-18.u13", 0x0c00, 0x0400, CRC(78697c0f) SHA1(31382ed4c0d44024f7f57a9de6407527f4d5b0d1) )	/* ??? */

ROM_END

ROM_START( fieldday )
	ROM_REGION( 0x14000, REGION_CPU1, 0 ) /* Z80 main CPU  */
	ROM_LOAD( "a17_44.bin", 0x00000, 0x2000, CRC(d59812e1) SHA1(f3e7e2f09fba5964c92813cd652aa093fe3e4415) )
	ROM_LOAD( "a17_45.bin", 0x02000, 0x2000, CRC(828bfb9a) SHA1(0be24ec076b715d65e9c8e01e3be76628e4f60ed) )
	ROM_LOAD( "a23_05.bin", 0x04000, 0x2000, CRC(2670cad3) SHA1(8ba3a6b788fa4e997f9153226f6f13b32fc33124) )
	ROM_LOAD( "a23_06.bin", 0x06000, 0x2000, CRC(737ce7de) SHA1(52a46fe14978e217de81dcd529d16d62fb5a4e46) )
	ROM_LOAD( "a23_07.bin", 0x10000, 0x2000, CRC(ee2fb306) SHA1(f2b0a6af279b459fe61d56ba4d36d519318376fb) )
	ROM_LOAD( "a23_08.bin", 0x12000, 0x2000, CRC(1ed2f1ad) SHA1(e3cf954dd2c34759147d0c85da7a716a8eb0e820) )
	ROM_COPY( REGION_CPU1 , 0x10000, 0x8000, 0x2000 ) /* to avoid 'bank bug' */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 sound CPU */
	ROM_LOAD( "a17_24.bin", 0x0000, 0x2000, CRC(6bac6b7f) SHA1(eb95192204a868737d609b789312ac37c31d3071) )
	ROM_LOAD( "a17_25.bin", 0x2000, 0x2000, CRC(570b90b1) SHA1(2a8c3bebd15655ffbfeaf40c2db90292afbb11ef) )
	ROM_LOAD( "a17_26.bin", 0x4000, 0x2000, CRC(7a8ea7f4) SHA1(1d9d2b54645266f95aa89cdbec6f82d4ac20d6e4) )
	ROM_LOAD( "a17_27.bin", 0x6000, 0x2000, CRC(e10594d9) SHA1(3df15b8b0c7af9fed93eca27237e15e6a7a8f835) )
	ROM_LOAD( "a17_28.bin", 0x8000, 0x2000, CRC(1a4d1dae) SHA1(fbc3c55ad9f15ead432c136eec648fe22e523ea7) )
	ROM_LOAD( "a17_29.bin", 0xa000, 0x2000, CRC(3c540007) SHA1(549e7ff260214c538913ff548dcb088987845911) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
        ROM_LOAD( "a17_14.bin", 0x0000, 0x0800, CRC(a686894d) SHA1(0578747a1869db98ee1962822491103f3f082cba) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a17_36.bin", 0x0000, 0x2000, CRC(e3dd51f7) SHA1(95a97ea925c5bc7bdc00887e6d17d817b36befc4) )
	ROM_LOAD( "a17_37.bin", 0x2000, 0x2000, CRC(1623f71f) SHA1(f5df7498b9a08e82ea11cb1b1fcdabca48cbf33a) )
	ROM_LOAD( "a17_38.bin", 0x4000, 0x2000, CRC(ca9f74db) SHA1(a002f1dfa9497793bfb18292e7a71ae12d70fb88) )
	ROM_LOAD( "a17_39.bin", 0x6000, 0x2000, CRC(fb6c667c) SHA1(da56be8d997db199588ee22fae30cc6d87e80704) )


	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a23_09.bin", 0x0000, 0x2000, CRC(1e430be5) SHA1(9296e1a0d820bb218578d55b739b4fc5fdafb125) )
	ROM_LOAD( "a23_10.bin", 0x2000, 0x2000, CRC(ee2e54f0) SHA1(0a92fa39696a8005f9441131b6d98205b7c26e7b) )
	ROM_LOAD( "a23_11.bin", 0x4000, 0x2000, CRC(6d37f15c) SHA1(3eb9a2e230d88f2871e6972a01d8e7cc7db1b123) )
	ROM_LOAD( "a23_12.bin", 0x6000, 0x2000, CRC(86da42d2) SHA1(aa79cd954c96217ca2daf37addac168f8cca24f9) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "a17-15.10v", 0x0000, 0x0400, CRC(9df472b7) SHA1(0cd9dd735238daf8e8228ba9481df57fb8925328) )	/* red */
	ROM_LOAD( "a17-16.8v",  0x0400, 0x0400, CRC(3bf1ff5f) SHA1(a0453851aefa9acdba4a86aaca8c442cb8550987) )	/* green */
	ROM_LOAD( "a17-17.9v",  0x0800, 0x0400, CRC(c42ae956) SHA1(057ce3783305c98622f7dfc0ee7d4882137a2ef8) )	/* blue */
	ROM_LOAD( "a17-18.23v", 0x0c00, 0x0400, CRC(3023a1da) SHA1(08ce4c6e99d04b358d66f0588852311d07183619) )	/* ??? */
ROM_END

ROM_START( undoukai )
	ROM_REGION( 0x14000, REGION_CPU1, 0 ) /* Z80 main CPU  */
	ROM_LOAD( "a17-01.70c", 0x00000, 0x4000, CRC(6ce324d9) SHA1(9c5207ac897eaae5a6aa1a05a918c9cb58544664) )
	ROM_LOAD( "a17-02.71c", 0x04000, 0x4000, CRC(055c7ef1) SHA1(f974bd441b8e3621ac5f8d36104791c97051a97a) )
	ROM_LOAD( "a17-03.72c", 0x10000, 0x4000, CRC(9034a5c5) SHA1(bc3dae0dee08b6989275ac220fc76bfe61367154) ) /* banked at 0x8000 */
	ROM_COPY( REGION_CPU1 , 0x10000, 0x8000, 0x2000 ) /* to avoid 'bank bug' */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 sound CPU */
	ROM_LOAD( "a17-08.8s",  0x0000, 0x2000, CRC(2545aa0e) SHA1(190ef99890251e1e49b14ffd28f2badb4d0d8fbe) )
	ROM_LOAD( "a17-09.9s",  0x2000, 0x2000, CRC(57e2cdbb) SHA1(ae6187d62fb36a37be06040e0fd85e0252cdf750) )
	ROM_LOAD( "a17-10.10s", 0x4000, 0x2000, CRC(38a288fe) SHA1(af4979cae59ca2569a3663132451b9b554552a79) )
	ROM_LOAD( "a17-11.37s", 0x6000, 0x2000, CRC(036d6969) SHA1(20e03dab14d44bf3c7c6aace3b28b2826581d1c7) )
	ROM_LOAD( "a17-12.38s", 0x8000, 0x2000, CRC(cb7e6dcd) SHA1(5286c6d340c1d465caebae5dd7e3d4ff8b7f8f5e) )
	ROM_LOAD( "a17-13.39s", 0xa000, 0x2000, CRC(0a40930e) SHA1(8c4b9fa0aed67a3e269c2136ef81791fc8acd1da) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 2k for the microcontroller */
        ROM_LOAD( "a17_14.bin", 0x0000, 0x0800, CRC(a686894d) SHA1(0578747a1869db98ee1962822491103f3f082cba) )
 
	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a17-04.18v", 0x0000, 0x4000, CRC(84dabee2) SHA1(698f12ee4201665988248853dafbf4b16dfc6517) )
	ROM_LOAD( "a17-05.19v", 0x4000, 0x4000, CRC(10bf3451) SHA1(23ebb1409c90d225ff5a13ad23d4dff1acaf904a) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a17-06.59v", 0x0000, 0x4000, CRC(50f28ad9) SHA1(001555ad123ac85000999b1aa39c1b2568e26f46) )
	ROM_LOAD( "a17-07.60v", 0x4000, 0x4000, CRC(7a4b4238) SHA1(8e58803645e61a7144a659d403f318a8899d36e2) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "a17-15.10v", 0x0000, 0x0400, CRC(9df472b7) SHA1(0cd9dd735238daf8e8228ba9481df57fb8925328) )	/* red */
	ROM_LOAD( "a17-16.8v",  0x0400, 0x0400, CRC(3bf1ff5f) SHA1(a0453851aefa9acdba4a86aaca8c442cb8550987) )	/* green */
	ROM_LOAD( "a17-17.9v",  0x0800, 0x0400, CRC(c42ae956) SHA1(057ce3783305c98622f7dfc0ee7d4882137a2ef8) )	/* blue */
	ROM_LOAD( "a17-18.23v", 0x0c00, 0x0400, CRC(3023a1da) SHA1(08ce4c6e99d04b358d66f0588852311d07183619) )	/* ??? */
ROM_END

GAME( 1984, 40love,   0,        40love,   40love,   40love,   ROT0, "Taito Corporation", "Forty-Love", GAME_IMPERFECT_GRAPHICS )
GAME( 1984, fieldday, 0,        undoukai, undoukai, undoukai, ROT0, "Taito Corporation", "Field Day", 0 )
GAME( 1984, undoukai, fieldday, undoukai, undoukai, undoukai, ROT0, "Taito Corporation", "The Undoukai (Japan)", 0 )
