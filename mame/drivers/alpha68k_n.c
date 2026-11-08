// license:BSD-3-Clause
// copyright-holders:Pierpaolo Prazzoli, Bryan McPhail, Stephane Humbert, Angelo Salese, David Haywood
/***************************************************************************

    SNK/Alpha 68000 N based board games

    Super Stingray, Kyros and Mahjong Block Jongbou hardware.

***************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "sound/2203intf.h"

PALETTE_INIT( kyros );
VIDEO_UPDATE( kyros );
VIDEO_UPDATE( sstingry );
void alpha68k_flipscreen_w(int flip);
void kyros_video_banking(int *bank, int data);
void jongbou_video_banking(int *bank, int data);
void (*alpha68k_video_banking)(int *bank, int data);
static int microcontroller_id;

static UINT16 *shared_ram;
static int coin_id, is_jongbou, coin_latch;
static unsigned trigstate, deposits1, deposits2, credits;

static MACHINE_RESET( alpha68k_n )
{
	trigstate = deposits1 = deposits2 = credits = coin_latch = 0;
	alpha68k_flipscreen_w(0);
}

static WRITE16_HANDLER( n_microcontroller_w )
{
	logerror("%04x:  Alpha write trigger at %04x (%04x)\n",activecpu_get_pc(),offset,data);

	if (offset==0x2d && ACCESSING_LSB)
		alpha68k_flipscreen_w(data & 1);
}

static READ16_HANDLER( kyros_dip_r )
{
	return readinputportbytag("IN1")<<8;
}

static READ16_HANDLER( jongbou_inputs_r )
{
	UINT8 inp1 = readinputportbytag("IN3");
	UINT8 inp2 = readinputportbytag("IN4");
	inp1 = ((inp1 & 0x01) << 3) + ((inp1 & 0x02) << 1) + ((inp1 & 0x04) >> 1) + ((inp1 & 0x08) >> 3);
	inp2 = ((inp2 & 0x01) << 3) + ((inp2 & 0x02) << 1) + ((inp2 & 0x04) >> 1) + ((inp2 & 0x08) >> 3);
	return readinputportbytag("IN0") | inp1 | inp2 << 4;
}

static WRITE16_HANDLER( kyros_sound_w )
{
	if(ACCESSING_MSB)
		soundlatch_w(0, (data>>8)&0xff);
}

static READ16_HANDLER( kyros_alpha_trigger_r )
{

	static unsigned coinvalue=0, microcontroller_data=0;
	static UINT8 coinage1[8][2]={{1,1},{1,5},{1,3},{2,3},{1,2},{1,6},{1,4},{3,2}};
	static UINT8 coinage2[8][2]={{1,1},{5,1},{3,1},{7,1},{2,1},{6,1},{4,1},{8,1}};

	int source=shared_ram[offset];

	switch (offset) {
		case 0x22:
			shared_ram[0x22] = (source&0xff00)|(credits&0x00ff);
			return 0;
		case 0x29:
			trigstate++;
			if ((readinputportbytag("IN2")&0x3)==3) coin_latch=0;
			if ((readinputportbytag("IN2")&0x1)==0 && !coin_latch)
			{
				shared_ram[0x29] = (source&0xff00)|(coin_id&0xff);
				shared_ram[0x22] = (source&0xff00)|0x0;
				coin_latch=1;

				coinvalue = (~readinputportbytag("IN1")>>1) & 7;
				deposits1++;
				if (deposits1 == coinage1[coinvalue][0])
				{
					credits = coinage1[coinvalue][1];
					deposits1 = 0;
				}
				else
					credits = 0;
			}
			else if ((readinputportbytag("IN2")&0x2)==0 && !coin_latch)
			{
				shared_ram[0x29] = (source&0xff00)|(coin_id>>8);
				shared_ram[0x22] = (source&0xff00)|0x0;
				coin_latch=1;

				coinvalue = (~readinputportbytag("IN1")>>1) & 7;
				deposits2++;
				if (deposits2 == coinage2[coinvalue][0])
				{
					credits = coinage2[coinvalue][1];
					deposits2 = 0;
				}
				else
					credits = 0;
			}
			else
			{
				if (microcontroller_id == 0x00ff || is_jongbou)
				{
					if (trigstate >= 12 || is_jongbou)
					{
						trigstate = 0;
						microcontroller_data = 0x21;
					}
					else
						microcontroller_data = 0x00;
				}
				else
					microcontroller_data = 0x00;

				shared_ram[0x29] = (source&0xff00)|microcontroller_data;
			}
			return 0;
		case 0xff:
			shared_ram[0xff] = (source&0xff00)|microcontroller_id;
			break;
	}

	logerror("%04x:  Alpha read trigger at %04x\n",activecpu_get_pc(),offset);

	return 0;
}

static ADDRESS_MAP_START( kyros_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x020000, 0x020fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x040000, 0x041fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x060000, 0x060001) AM_READ(MRA16_RAM)
	AM_RANGE(0x080000, 0x0801ff) AM_READ(kyros_alpha_trigger_r)
	AM_RANGE(0x0c0000, 0x0c0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x0e0000, 0x0e0001) AM_READ(kyros_dip_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kyros_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x020000, 0x020fff) AM_WRITE(MWA16_RAM) AM_BASE(&shared_ram)
	AM_RANGE(0x040000, 0x041fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16)
	AM_RANGE(0x060000, 0x060001) AM_WRITE(MWA16_RAM) AM_BASE(&videoram16)
	AM_RANGE(0x080000, 0x0801ff) AM_WRITE(n_microcontroller_w)
	AM_RANGE(0x0e0000, 0x0e0001) AM_WRITE(kyros_sound_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kyros_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kyros_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(soundlatch_clear_w)
	AM_RANGE(0xe004, 0xe004) AM_WRITE(DAC_0_signed_data_w)
	AM_RANGE(0xe006, 0xe00e) AM_WRITE(MWA8_NOP)

ADDRESS_MAP_END

static ADDRESS_MAP_START( sstingry_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc100, 0xc100) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sstingry_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xc102, 0xc102) AM_WRITE(soundlatch_clear_w)
	AM_RANGE(0xc104, 0xc104) AM_WRITE(DAC_0_signed_data_w)
	AM_RANGE(0xc106, 0xc10e) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jongbou_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( kyros_sound_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x10, 0x10) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(YM2203_write_port_1_w)
	AM_RANGE(0x81, 0x81) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x90, 0x90) AM_WRITE(YM2203_write_port_2_w)
	AM_RANGE(0x91, 0x91) AM_WRITE(YM2203_control_port_2_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jongbou_sound_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(soundlatch_clear_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

#define ALPHA68K_PLAYER_INPUT_SWAP_LR_LSB( player, button3, start, active ) \
	PORT_BIT( 0x0001, active, IPT_JOYSTICK_UP    ) PORT_PLAYER(player) \
	PORT_BIT( 0x0002, active, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(player) \
	PORT_BIT( 0x0004, active, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(player) \
	PORT_BIT( 0x0008, active, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(player) \
	PORT_BIT( 0x0010, active, IPT_BUTTON1        ) PORT_PLAYER(player) \
	PORT_BIT( 0x0020, active, IPT_BUTTON2        ) PORT_PLAYER(player) \
	PORT_BIT( 0x0040, active, button3            ) PORT_PLAYER(player) \
	PORT_BIT( 0x0080, active, start )

#define ALPHA68K_PLAYER_INPUT_SWAP_LR_MSB( player, button3, start, active ) \
	PORT_BIT( 0x0100, active, IPT_JOYSTICK_UP    ) PORT_PLAYER(player) \
	PORT_BIT( 0x0200, active, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(player) \
	PORT_BIT( 0x0400, active, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(player) \
	PORT_BIT( 0x0800, active, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(player) \
	PORT_BIT( 0x1000, active, IPT_BUTTON1        ) PORT_PLAYER(player) \
	PORT_BIT( 0x2000, active, IPT_BUTTON2        ) PORT_PLAYER(player) \
	PORT_BIT( 0x4000, active, button3            ) PORT_PLAYER(player) \
	PORT_BIT( 0x8000, active, start )

#define ALPHA68K_COINAGE_BITS_1TO3 \
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )	\
	PORT_DIPSETTING(    0x0e, "A 1C/1C B 1C/1C" )	\
	PORT_DIPSETTING(    0x06, "A 1C/2C B 2C/1C" )	\
	PORT_DIPSETTING(    0x0a, "A 1C/3C B 3C/1C" )	\
	PORT_DIPSETTING(    0x02, "A 1C/4C B 4C/1C" )	\
	PORT_DIPSETTING(    0x0c, "A 1C/5C B 5C/1C" )	\
	PORT_DIPSETTING(    0x04, "A 1C/6C B 6C/1C" )	\
	PORT_DIPSETTING(    0x08, "A 2C/3C B 7C/1C" )	\
	PORT_DIPSETTING(    0x00, "A 3C/2C B 8C/1C" )

#define ALPHA68K_MCU \
	PORT_START_TAG("IN2")  \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )\

INPUT_PORTS_START( sstingry )
	PORT_START_TAG("IN0")
	ALPHA68K_PLAYER_INPUT_SWAP_LR_LSB( 1, IPT_UNKNOWN, IPT_START1, IP_ACTIVE_HIGH )
	ALPHA68K_PLAYER_INPUT_SWAP_LR_MSB( 2, IPT_UNKNOWN, IPT_START2, IP_ACTIVE_HIGH )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	ALPHA68K_COINAGE_BITS_1TO3
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	ALPHA68K_MCU
INPUT_PORTS_END

INPUT_PORTS_START( kyros )
	PORT_START_TAG("IN0")
	ALPHA68K_PLAYER_INPUT_SWAP_LR_LSB( 1, IPT_UNKNOWN, IPT_START1, IP_ACTIVE_HIGH )
	ALPHA68K_PLAYER_INPUT_SWAP_LR_MSB( 2, IPT_UNKNOWN, IPT_START2, IP_ACTIVE_HIGH )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	ALPHA68K_COINAGE_BITS_1TO3
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	ALPHA68K_MCU
INPUT_PORTS_END

INPUT_PORTS_START( jongbou )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0fff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "A 1C/1C B 1C/5C" )
	PORT_DIPSETTING(    0x02, "A 1C/2C B 1C/3C" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000 - 60000" )
	PORT_DIPSETTING(    0x04, "Every 30000" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, "Show Girls" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	ALPHA68K_MCU

	PORT_START_TAG("IN3")
	PORT_BIT( 0xff, 0, IPT_DIAL ) PORT_MINMAX(0, 15) PORT_SENSITIVITY(50) PORT_KEYDELTA(20)

	PORT_START_TAG("IN4")
	PORT_BIT( 0xff, 0, IPT_DIAL ) PORT_MINMAX(0, 15) PORT_SENSITIVITY(50) PORT_KEYDELTA(20) PORT_PLAYER(2)
INPUT_PORTS_END

static const gfx_layout sting_layout1 =
{
	8,8,
	1024,
	3,
	{ 4, 0+(0x10000*4), 4+(0x8000*8) },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout sting_layout2 =
{
	8,8,
	1024,
	3,
	{ 0, 0+(0x28000*8), 4+(0x28000*8) },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout sting_layout3 =
{
	8,8,
	1024,
	3,
	{ 0, 0+(0x10000*8), 4+(0x10000*8) },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout kyros_char_layout1 =
{
	8,8,
	0x8000/16,
	3,
	{ 4,0x8000*8,0x8000*8+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout kyros_char_layout2 =
{
	8,8,
	0x8000/16,
	3,
	{ 0,0x10000*8,0x10000*8+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout jongbou_layout1 =
{
	8,8,
	1024,
	3,
	{ 4, 0x8000*8, 4+0x8000*8 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_layout jongbou_layout2 =
{
	8,8,
	1024,
	3,
	{ 0, 0x10000*8, 4+0x10000*8 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static const gfx_decode sstingry_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &sting_layout1,  0, 32 },
	{ REGION_GFX1, 0x00000, &sting_layout2,  0, 32 },
	{ REGION_GFX1, 0x10000, &sting_layout1,  0, 32 },
	{ REGION_GFX1, 0x10000, &sting_layout3,  0, 32 },
	{ -1 }
};

static const gfx_decode kyros_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x00000, &kyros_char_layout2,  0, 32 },
	{ REGION_GFX1, 0x18000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x18000, &kyros_char_layout2,  0, 32 },
	{ REGION_GFX1, 0x30000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x30000, &kyros_char_layout2,  0, 32 },
	{ REGION_GFX1, 0x48000, &kyros_char_layout1,  0, 32 },
	{ REGION_GFX1, 0x48000, &kyros_char_layout2,  0, 32 },
	{ -1 }
};

static const gfx_decode jongbou_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &jongbou_layout1, 0, 32 },
	{ REGION_GFX1, 0x00000, &jongbou_layout2, 0, 32 },
	{ REGION_GFX1, 0x18000, &jongbou_layout1, 0, 32 },
	{ REGION_GFX1, 0x18000, &jongbou_layout2, 0, 32 },
	{ REGION_GFX1, 0x04000, &jongbou_layout1, 0, 32 },
	{ REGION_GFX1, 0x04000, &jongbou_layout2, 0, 32 },
	{ REGION_GFX1, 0x1c000, &jongbou_layout1, 0, 32 },
	{ REGION_GFX1, 0x1c000, &jongbou_layout2, 0, 32 },
	{ -1 }
};

static struct AY8910interface ay8910_interface =
{
	soundlatch_r
};

static INTERRUPT_GEN( alpha68k_interrupt )
{
	if (cpu_getiloops() == 0)
		cpunum_set_input_line(0, 1, HOLD_LINE);
	else
		cpunum_set_input_line(0, 2, HOLD_LINE);
}

static MACHINE_DRIVER_START( sstingry )

	MDRV_CPU_ADD(M68000, 6000000)
	MDRV_CPU_PROGRAM_MAP(kyros_readmem,kyros_writemem)
	MDRV_CPU_VBLANK_INT(alpha68k_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)

	MDRV_CPU_PROGRAM_MAP(sstingry_sound_readmem,sstingry_sound_writemem)
	MDRV_CPU_IO_MAP(0,kyros_sound_writeport)

	MDRV_CPU_VBLANK_INT(irq0_line_hold, 2)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, TIME_IN_HZ(4000))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(alpha68k_n)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(sstingry_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_COLORTABLE_LENGTH(256)
	MDRV_PALETTE_INIT(kyros)

	MDRV_VIDEO_UPDATE(sstingry)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.35)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.35)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kyros )

	MDRV_CPU_ADD(M68000, 6000000)
	MDRV_CPU_PROGRAM_MAP(kyros_readmem,kyros_writemem)
	MDRV_CPU_VBLANK_INT(alpha68k_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)

	MDRV_CPU_PROGRAM_MAP(kyros_sound_readmem,kyros_sound_writemem)
	MDRV_CPU_IO_MAP(0,kyros_sound_writeport)

	MDRV_CPU_VBLANK_INT(irq0_line_hold, 2)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, TIME_IN_HZ(4000))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(alpha68k_n)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(kyros_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_PALETTE_INIT(kyros)
	MDRV_VIDEO_UPDATE(kyros)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.35)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.35)

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.90)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jongbou )

	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(kyros_readmem,kyros_writemem)
	MDRV_CPU_VBLANK_INT(alpha68k_interrupt,17)

	MDRV_CPU_ADD(Z80, 4000000)

	MDRV_CPU_PROGRAM_MAP(jongbou_sound_map,0)
	MDRV_CPU_IO_MAP(jongbou_sound_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 160)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(alpha68k_n)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(jongbou_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)

	MDRV_PALETTE_INIT(kyros)
	MDRV_VIDEO_UPDATE(kyros)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	/* Leave headroom when all three AY channels reach full volume. */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

ROM_START( sstingry )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ss_05.rom",  0x0000,  0x4000, CRC(bfb28d53) SHA1(64a1b8627529ed13074bb949cb104077eb3eac1f) )
	ROM_LOAD16_BYTE( "ss_07.rom",  0x0001,  0x4000, CRC(eb1b65c5) SHA1(cffc4df82b7950358dd28f6a492e0aefaff73048) )
	ROM_LOAD16_BYTE( "ss_04.rom",  0x8000,  0x4000, CRC(2e477a79) SHA1(0af9238979c8a740ba49776cd65ffbc024339621) )
	ROM_LOAD16_BYTE( "ss_06.rom",  0x8001,  0x4000, CRC(597620cb) SHA1(5549df4843e029df17ce5de2159cc82bd985804b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "ss_01.rom",       0x0000,  0x4000, CRC(fef09a92) SHA1(77b6aded1eed1bd5e6ffb25b56b62b10b7b9a304) )
	ROM_LOAD( "ss_02.rom",       0x4000,  0x4000, CRC(ab4e8c01) SHA1(d96e7f97945fff48fb7b4661fdb575ac7ff77445) )

	ROM_REGION( 0x0400, REGION_CPU3, 0 )
	ROM_LOAD( "d8748.bin",       0x0000, 0x0400, CRC(7fcbfc30) SHA1(6d087a3d44e475b6c8260a5134952097f26459b7) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ss_12.rom",       0x00000, 0x4000, CRC(74caa9e9) SHA1(9f0874b2fcdf45acb941bd56b44bf2b9b08641e9) )
	ROM_LOAD( "ss_08.rom",       0x08000, 0x4000, CRC(32368925) SHA1(af26f73d33936410063de3164ec80f45bed487c7) )
	ROM_LOAD( "ss_13.rom",       0x10000, 0x4000, CRC(13da6203) SHA1(afa778c26da1adfdc8b2e2a1c7b2b46944b5d008) )
	ROM_LOAD( "ss_10.rom",       0x18000, 0x4000, CRC(2903234a) SHA1(552295ec60469227883eafb6756c86abc20455b5) )
	ROM_LOAD( "ss_11.rom",       0x20000, 0x4000, CRC(d134302e) SHA1(4c020ff41458c738596ab7a094d4a33a6dda64bf) )
	ROM_LOAD( "ss_09.rom",       0x28000, 0x4000, CRC(6f9d938a) SHA1(eb6934f8eaa7b22441ec4470280f228fe5f134a3) )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "ic92",            0x0000, 0x0100, CRC(e7ce1179) SHA1(36835c46c1c3f820df39c59c16c362db07b32dc9) )
	ROM_LOAD( "ic93",            0x0100, 0x0100, CRC(9af8a375) SHA1(abb8b094a2df41acea688f87004207dc35233db5) )
	ROM_LOAD( "ic91",            0x0200, 0x0100, CRC(c3965079) SHA1(6b1f22afd2a849f0003ddcad344079e8043681f9) )
	ROM_LOAD( "ssprom2.bin",     0x0300, 0x0100, CRC(c2205b71) SHA1(a7db60ac7d559fe53a35264fab17f1d5e48d3f10) )
	ROM_LOAD( "ssprom1.bin",     0x0400, 0x0100, CRC(1003186c) SHA1(e50b60036d6b32a4d524c92d35c4d9901ee7ec0e) )
ROM_END

ROM_START( kyros )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2.10c", 0x00000,  0x4000, CRC(4bd030b1) SHA1(e503dae8e12995ab0a551022a848a62315908e8b) )
	ROM_CONTINUE   (          0x10000,  0x4000 )
	ROM_LOAD16_BYTE( "1.13c", 0x00001,  0x4000, CRC(75cfbc5e) SHA1(2a70c56fd7192279157df8294743038a7ed7e68d) )
	ROM_CONTINUE   (          0x10001,  0x4000 )
	ROM_LOAD16_BYTE( "4.10b", 0x08000,  0x4000, CRC(be2626c2) SHA1(c3b01ec4b65172560a993b37421df6a61b780e43) )
	ROM_CONTINUE   (          0x18000,  0x4000 )
	ROM_LOAD16_BYTE( "3.13b", 0x08001,  0x4000, CRC(fb25e71a) SHA1(fab8fcbd2c5a8600d6e8577de4875e409cad723b) )
	ROM_CONTINUE   (          0x18001,  0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "2s.1f",      0x00000, 0x4000, CRC(800ceb27) SHA1(4daa1b8adcad7a90cfd5d20704a7c431673c4995) )
	ROM_LOAD( "1s.1d",      0x04000, 0x8000, CRC(87d3e719) SHA1(4b8b1b600c7c1de3a77030001e7e6f0ff118f294) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.9pr",  0x00000, 0x8000, CRC(c5290944) SHA1(ec97482dc59220002780ae4d02be4cd172cf65ac) )
	ROM_LOAD( "11.11m", 0x08000, 0x8000, CRC(fbd44f1e) SHA1(d095544ea76674a7ad17c1b8c88614e65890281c) )
	ROM_LOAD( "12.11n", 0x10000, 0x8000, CRC(10fed501) SHA1(71c0b4b94f86046745105307938f6e2c5661e2a1) )
	ROM_LOAD( "9.9s",   0x18000, 0x8000, CRC(dd40ca33) SHA1(91a1d8b6b69fb0d27ed315cd2591f352360bc8e7) )
	ROM_LOAD( "13.11p", 0x20000, 0x8000, CRC(e6a02030) SHA1(0de58f8cc69dc76d4b0a45fba04972634a4021a6) )
	ROM_LOAD( "14.11r", 0x28000, 0x8000, CRC(722ad23a) SHA1(0e1be976c5a406e33236def5a0dce73911ebac28) )
	ROM_LOAD( "15.3t",  0x30000, 0x8000, CRC(045fdda4) SHA1(ac25368e446e6dcfb3ed244e7d6d699f917c202d) )
	ROM_LOAD( "17.7t",  0x38000, 0x8000, CRC(7618ec00) SHA1(7346ba41fd2b04e404225726ede2e42e62ca7901) )
	ROM_LOAD( "18.9t",  0x40000, 0x8000, CRC(0ee74171) SHA1(1fda8a1066eb7dafeaeffebfe718b408b34f1767) )
	ROM_LOAD( "16.5t",  0x48000, 0x8000, CRC(2cf14824) SHA1(07f6b232c4ca6c42b3f75443b0328653f5a3f71d) )
	ROM_LOAD( "19.11t", 0x50000, 0x8000, CRC(4f336306) SHA1(83e0c021d2732d3199c70ac433a31863075a5a72) )
	ROM_LOAD( "20.13t", 0x58000, 0x8000, CRC(a165d06b) SHA1(ef7f38a71c2f3fc836b28f0eac1c14a3877f0802) )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114l.5r", 0x000, 0x100, CRC(3628bf36) SHA1(33b7b873a6e5da59f535d754f8c8257c0f4d0a31) )
	ROM_LOAD( "mb7114l.4r", 0x100, 0x100, CRC(850704e4) SHA1(8a9da9efc7bc6a037d4cd27152b853a7839ccd67) )
	ROM_LOAD( "mb7114l.6r", 0x200, 0x100, CRC(a54f60d7) SHA1(af039dee847913cb79f85d7abf4846322bba2e5b) )
	ROM_LOAD( "mb7114l.5p", 0x300, 0x100, CRC(1cc53765) SHA1(2b665f3e24ddb3ab591273f027ff7740f1c97e27) )
	ROM_LOAD( "mb7114l.6p", 0x400, 0x100, CRC(b0d6971f) SHA1(4aa2e9a89f9ea7487433e54ef4aa95a632477c4f) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "0.1t",      0x0000,0x2000, CRC(5d0acb4c) SHA1(52fcdcb2bf6d6ada04aa447b5526c39848bf587f) )
ROM_END

ROM_START( kyrosj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2j.10c",0x00000,  0x4000, CRC(b324c11b) SHA1(9330ee0db8555a3623118c7bc5363b4f6fa87dbc) )
	ROM_CONTINUE   (          0x10000,  0x4000 )
	ROM_LOAD16_BYTE( "1j.13c",0x00001,  0x4000, CRC(8496241b) SHA1(474cdce735dcc2ff2111ae2f4cd11c0d27a4b4fc) )
	ROM_CONTINUE   (          0x10001,  0x4000 )
	ROM_LOAD16_BYTE( "4.10a", 0x08000,  0x4000, CRC(0187f59d) SHA1(3bc1b811cb29aa33c38bc8c76e066c8b37104167) )
	ROM_CONTINUE   (          0x18000,  0x4000 )
	ROM_LOAD16_BYTE( "3.13a", 0x08001,  0x4000, CRC(ab97941d) SHA1(014a55540e1777de5bee23e59773dbbd7efa8f91) )
	ROM_CONTINUE   (          0x18001,  0x4000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "2s.1f",      0x00000, 0x4000, CRC(800ceb27) SHA1(4daa1b8adcad7a90cfd5d20704a7c431673c4995) )
	ROM_LOAD( "1s.1d",      0x04000, 0x8000, CRC(87d3e719) SHA1(4b8b1b600c7c1de3a77030001e7e6f0ff118f294) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8.9r",   0x00000, 0x8000, CRC(d8203284) SHA1(7dede410239be6b674644fa76c91dd01837f841f) )
	ROM_LOAD( "11.12m", 0x08000, 0x8000, CRC(a2f9738c) SHA1(31be81274bf70674bf0c32fcddbacf0f58d8f897) )
	ROM_LOAD( "12.11n", 0x10000, 0x8000, CRC(10fed501) SHA1(71c0b4b94f86046745105307938f6e2c5661e2a1) )
	ROM_LOAD( "9j.9s",  0x18000, 0x8000, CRC(3e725349) SHA1(79c431d83a0f0d5e0d69086f54f6e60a42b69e14) )
	ROM_LOAD( "13.11p", 0x20000, 0x8000, CRC(e6a02030) SHA1(0de58f8cc69dc76d4b0a45fba04972634a4021a6) )
	ROM_LOAD( "14.12r", 0x28000, 0x8000, CRC(39d07db9) SHA1(05c0785eea29bc6329892f1e8f0bd37327163080) )
	ROM_LOAD( "15.3t",  0x30000, 0x8000, CRC(045fdda4) SHA1(ac25368e446e6dcfb3ed244e7d6d699f917c202d) )
	ROM_LOAD( "17.7t",  0x38000, 0x8000, CRC(7618ec00) SHA1(7346ba41fd2b04e404225726ede2e42e62ca7901) )
	ROM_LOAD( "18.9t",  0x40000, 0x8000, CRC(0ee74171) SHA1(1fda8a1066eb7dafeaeffebfe718b408b34f1767) )
	ROM_LOAD( "16j.5t", 0x48000, 0x8000, CRC(e1566679) SHA1(3653c3160798bea203e1c7713043729b356a7358) )
	ROM_LOAD( "19.11t", 0x50000, 0x8000, CRC(4f336306) SHA1(83e0c021d2732d3199c70ac433a31863075a5a72) )
	ROM_LOAD( "20j.13t",0x58000, 0x8000, CRC(0624b4c0) SHA1(a8ebdb1f9b7fd0b78102b54523e8680aaa8bcf42) )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114l.5r", 0x000, 0x100, CRC(3628bf36) SHA1(33b7b873a6e5da59f535d754f8c8257c0f4d0a31) )
	ROM_LOAD( "mb7114l.4r", 0x100, 0x100, CRC(850704e4) SHA1(8a9da9efc7bc6a037d4cd27152b853a7839ccd67) )
	ROM_LOAD( "mb7114l.6r", 0x200, 0x100, CRC(a54f60d7) SHA1(af039dee847913cb79f85d7abf4846322bba2e5b) )
	ROM_LOAD( "mb7114l.5p", 0x300, 0x100, CRC(1cc53765) SHA1(2b665f3e24ddb3ab591273f027ff7740f1c97e27) )
	ROM_LOAD( "mb7114l.6p", 0x400, 0x100, CRC(b0d6971f) SHA1(4aa2e9a89f9ea7487433e54ef4aa95a632477c4f) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "0j.1t",      0x0000,0x2000, CRC(a34ecb29) SHA1(60a0b0cfcd2d9830bc112774bac700ded40d4afb) )
ROM_END

ROM_START( jongbou )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p2.a13", 0x00000, 0x10000, CRC(ee59e67a) SHA1(d73f15994879c645a8021dcd4f53948bcbd0748e) )
	ROM_LOAD16_BYTE( "p1.a15", 0x00001, 0x10000, CRC(1ab6803e) SHA1(a217138332d61b8f5996ead0280c970481db9abe) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "p7.i1", 0x00000, 0x8000, CRC(88d74794) SHA1(98dbbb4d88c1e96a0e251e39ef43b02bd68e0bba) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "p6.l15", 0x00000, 0x08000, CRC(1facee65) SHA1(6c98338c616e53106960063d0d31483131b492b0) )
	ROM_CONTINUE(0x18000, 0x8000)
	ROM_LOAD( "p5.k15", 0x20000, 0x10000, CRC(db0ad6bb) SHA1(c2ce0e78a4be9314f4f14ea87f521a79bab3697c) )
	ROM_LOAD( "p4.j15", 0x08000, 0x10000, CRC(56842cfa) SHA1(141ed992332540487cec951eab61c18be994b618) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "p3.i15", 0x0000, 0x2000, CRC(8c09cd2a) SHA1(317764e0f5af29e78fd764bdf28579bf6be5630f) )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "r.k2",  0x0000, 0x0100, CRC(0563235a) SHA1(c337a9a15c1a27012a963fc4e1345605aaa1401f) )
	ROM_LOAD( "g.k1",  0x0100, 0x0100, CRC(81fc51f2) SHA1(92df86898a1cc1fa2faf620466737f4e1cf83a58) )
	ROM_LOAD( "b.k3",  0x0200, 0x0100, CRC(6dfeba56) SHA1(abf569c400dc4366a0c7e483dbb672c089692c7e) )
	ROM_LOAD( "h.l9",  0x0300, 0x0100, CRC(e6e93b0b) SHA1(f64ff63699451910982a1a44c94ccd2c18fd389e) )
	ROM_LOAD( "l.l10", 0x0400, 0x0100, CRC(51676dac) SHA1(685d14f448501a63cc9fa063f65842caddad8f39) )
ROM_END

ROM_START( jongbou2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "j.b1_ver.2.13a.27c512", 0x00000, 0x10000, CRC(22e18446) SHA1(e6f6f06a99c66dcf8b3e05b85c35597a26b57aa2) )
	ROM_LOAD16_BYTE( "j.b2_ver.2.16a.27c512", 0x00001, 0x10000, CRC(c30d0030) SHA1(56d3aacf7f53970c21abd6ac5592a5b29ac946c8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "j.b7.1i.27256", 0x00000, 0x8000, CRC(88d74794) SHA1(98dbbb4d88c1e96a0e251e39ef43b02bd68e0bba) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "alpha.mcu", 0x000, 0x1000, NO_DUMP )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "j.b6.16l.27c512",  0x00000, 0x08000, CRC(71c53f95) SHA1(0e12d03f2bbcff14816f739026e40939c9f68bab) )
	ROM_CONTINUE(0x18000,0x8000)
	ROM_LOAD( "j.b5.16k.27c512",  0x20000, 0x10000, CRC(d68b6412) SHA1(6734f5eb31fbf7b5b3edf1d8c4369cc74aea74ab) )
	ROM_LOAD( "j.b24.16j.27c512", 0x08000, 0x10000, CRC(7dad0bda) SHA1(bea00314036a43806c07c39a0d3f7020990ded09) )

	ROM_REGION( 0x500, REGION_PROMS, 0 )
	ROM_LOAD( "2.l2.82s129a",  0x0000, 0x0100, CRC(aa5c9b97) SHA1(2758d393aa210fab0af9f5fd817edbd5c71a88f2) )
	ROM_LOAD( "1.l1.82s129a",  0x0100, 0x0100, CRC(15e45cf2) SHA1(f0712b43e8423f811a1691b1f51683a7af58f868) )
	ROM_LOAD( "3.l3.82s129a",  0x0200, 0x0100, CRC(90de80ca) SHA1(d58c5fed42ac71b84bb51e6acfe1d65158532387) )

	ROM_LOAD( "4.l9.82s129a",  0x0300, 0x0100, CRC(a2c6204a) SHA1(3bb342eced4abc0d7404b00343b3770dd7a0003e) )
	ROM_LOAD( "5.l10.82s129a", 0x0400, 0x0100, CRC(f7cdebee) SHA1(20cb28e2fb9507ef1aaa9c05f22ca589756eb49d) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "j.b3.16i.2764", 0x00000, 0x2000, CRC(d13a1c02) SHA1(7ad4b96f5ce05e0ca5b55c486b3870d756e6bded) )
ROM_END

static DRIVER_INIT( sstingry )
{
	is_jongbou = 0;
	microcontroller_id=0x00ff;
	coin_id=0x22|(0x22<<8);
}

static DRIVER_INIT( kyros )
{
	is_jongbou = 0;
	microcontroller_id=0x0012;
	coin_id=0x22|(0x22<<8);

	alpha68k_video_banking = kyros_video_banking;
}

static DRIVER_INIT( jongbou )
{
	is_jongbou = 1;
	microcontroller_id=0x00ff;
	coin_id=0x23|(0x24<<8);

	alpha68k_video_banking = jongbou_video_banking;

	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x0c0000, 0x0c0001, 0, 0, jongbou_inputs_r);
}

static DRIVER_INIT( jongbou2 )
{
	init_jongbou();
	/* The sequel uses the same timer protocol with a different boot ID. */
	microcontroller_id = 0x0012;
}

GAME( 1986, sstingry, 0,        sstingry,      sstingry, sstingry, ROT90, "Alpha Denshi Co.",   "Super Stingray", 0 )
GAME( 1987, kyros,    0,        kyros,         kyros,    kyros,    ROT90, "World Games Inc",    "Kyros", 0 )
GAME( 1986, kyrosj,   kyros,    kyros,         kyros,    kyros,    ROT90, "Alpha Denshi Co.",   "Kyros No Yakata (Japan)", 0 )
GAME( 1987, jongbou,  0,        jongbou,       jongbou,  jongbou,  ROT90, "SNK",                "Mahjong Block Jongbou (Japan)", 0 )
GAME( 1987, jongbou2, 0, jongbou, jongbou, jongbou2, ROT90, "SNK", "Mahjong Block Jongbou 2 (Japan)", 0 )
