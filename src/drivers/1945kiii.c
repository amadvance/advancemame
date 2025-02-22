/*

1945 K-3 driver
---------------

1945K-III
Oriental, 2000

This game is a straight rip-off of Psikyo's Strikers 1945 III.

PCB Layout
----------

ORIENTAL SOFT INC., -OPCX2-
|--------------------------------------------|
|    AD-65   SND-1.SU7            M16M-1.U62 |
|                     PAL                    |
|    AD-65   SND-2.SU4                       |
|                                 M16M-2.U63 |
|                                            |
|                    KM681000                |
|J                   KM681000     6116       |
|A                                           |
|M          62256    |-------|    6116       |
|M          62256    |SPR800E|               |
|A                   |OP-CX1 |    6116  6116 |
|    6116   PRG-1.U51|QFP208 |               |
|                    |-------|    6116  6116 |
|    6116   PRG-2.U52                        |
|                 |-----| |------|           |
|           PAL   |     | |QL2003| M16M-3.U61|
|           PAL   |68000| |PLCC84|           |
|DSW1 DSW2        |-----| |------| PAL       |
|             16MHz        27MHz             |
|--------------------------------------------|
Notes:
     68000 clock : 16.000MHz
    M6295 clocks : 1.000MHz (both), sample rate = 1000000 / 132
           VSync : 60Hz

*/

#include "driver.h"
#include "sound/okim6295.h"

static UINT16* k3_spriteram_1;
static UINT16* k3_spriteram_2;
static UINT16* k3_bgram;
static tilemap *k3_bg_tilemap;

static WRITE16_HANDLER( k3_bgram_w )
{
	if (k3_bgram[offset] != data)
	{
		COMBINE_DATA(&k3_bgram[offset]);
		tilemap_mark_tile_dirty(k3_bg_tilemap,offset);
	}
}

static void get_k3_bg_tile_info(int tile_index)
{
	int tileno;
	tileno = k3_bgram[tile_index];
	SET_TILE_INFO(1,tileno,0,0)
}

VIDEO_START(k3)
{
	k3_bg_tilemap = tilemap_create(get_k3_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,      16, 16, 32,32);
	return 0;
}

static void k3_draw_sprites ( mame_bitmap *bitmap, const rectangle *cliprect )
{
	const gfx_element *gfx = Machine->gfx[0];
	UINT16 *source = k3_spriteram_1;
	UINT16 *source2 = k3_spriteram_2;
	UINT16 *finish = source + 0x1000/2;

	while( source<finish )
	{
		int xpos, ypos;
		int tileno, color;
        xpos = (source[0] & 0xff00) >> 8 | (source2[0] & 0x0001) << 8;
		ypos = (source[0] & 0x00ff) >> 0;
		tileno = (source2[0] & 0x7ffe) >> 1;
		color = BIT(source2[0], 15) ? 0 : 1;

		drawgfx(bitmap,gfx, tileno,color,0,0,xpos,ypos,cliprect,TRANSPARENCY_PEN,0);
		drawgfx(bitmap,gfx, tileno,color,0,0,xpos,ypos-0x100,cliprect,TRANSPARENCY_PEN,0); // wrap
		drawgfx(bitmap,gfx, tileno,color,0,0,xpos-0x200,ypos,cliprect,TRANSPARENCY_PEN,0); // wrap
		drawgfx(bitmap,gfx, tileno,color,0,0,xpos-0x200,ypos-0x100,cliprect,TRANSPARENCY_PEN,0); // wrap

		source++;source2++;
	}
}

VIDEO_UPDATE(k3)
{
	tilemap_draw(bitmap,cliprect,k3_bg_tilemap,0,0);
	k3_draw_sprites(bitmap,cliprect);
}


WRITE16_HANDLER( k3_scrollx_w )
{
	tilemap_set_scrollx( k3_bg_tilemap,0, data);
}

WRITE16_HANDLER( k3_scrolly_w )
{
	tilemap_set_scrolly( k3_bg_tilemap,0, data);
}

WRITE16_HANDLER( k3_soundbanks_w )
{
	OKIM6295_set_bank_base(0, (data & 4) ? 0x40000 : 0);
	OKIM6295_set_bank_base(1, (data & 2) ? 0x40000 : 0);
}

WRITE16_HANDLER( flagrall_soundbanks_w )
{

	//data &= mem_mask;
	
	// 0x0200 on startup
	// 0x0100 on startup

	// 0x80 - ?
	// 0x40 - ?
	// 0x20 - toggles, might trigger vram -> buffer transfer?
	// 0x10 - unknown, always on?
	// 0x08 - ?
	// 0x06 - oki bank
	// 0x01 - ?

	if (data & 0xfcc9)
		ui_popup("unk control %04x", data & 0xfcc9);
		
    OKIM6295_set_bank_base(0, 0x40000 * ((data & 0x6)>>1) ); 


static ADDRESS_MAP_START( k3_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x2003ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x240000, 0x240fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x280000, 0x280fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x2c0000, 0x2c07ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x2c0800, 0x2c0fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x440000, 0x440001) AM_READ(input_port_1_word_r)
	AM_RANGE(0x480000, 0x480001) AM_READ(input_port_2_word_r)
	AM_RANGE(0x4c0000, 0x4c0001) AM_READ(OKIM6295_status_1_msb_r)
	AM_RANGE(0x500000, 0x500001) AM_READ(OKIM6295_status_0_msb_r)
	AM_RANGE(0x8c0000, 0x8cffff) AM_READ(MRA16_RAM)// not used?
ADDRESS_MAP_END

static ADDRESS_MAP_START( k3_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0009CE, 0x0009CF) AM_WRITE(MWA16_NOP) // bug in code? (clean up log)
        AM_RANGE(0x0009D0, 0x0009D1) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x0009D2, 0x0009D3) AM_WRITE(MWA16_NOP) // bug in code? (clean up log)

	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM) // ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM) // Main Ram
	AM_RANGE(0x200000, 0x2003ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16) // palette
	AM_RANGE(0x240000, 0x240fff) AM_WRITE(MWA16_RAM) AM_BASE(&k3_spriteram_1)
	AM_RANGE(0x280000, 0x280fff) AM_WRITE(MWA16_RAM) AM_BASE(&k3_spriteram_2)
	AM_RANGE(0x2c0000, 0x2c07ff) AM_WRITE(k3_bgram_w) AM_BASE(&k3_bgram)
	AM_RANGE(0x2c0800, 0x2c0fff) AM_WRITE(MWA16_RAM)// or does k3 have a bigger tilemap? (flagrall is definitely 32x32 tiles)
	AM_RANGE(0x340000, 0x340001) AM_WRITE(k3_scrollx_w)
	AM_RANGE(0x380000, 0x380001) AM_WRITE(k3_scrolly_w)
	AM_RANGE(0x3c0000, 0x3c0001) AM_WRITE(k3_soundbanks_w)
	AM_RANGE(0x4c0000, 0x4c0001) AM_WRITE(OKIM6295_data_1_msb_w)
	AM_RANGE(0x500000, 0x500001) AM_WRITE(OKIM6295_data_0_msb_w)
	AM_RANGE(0x8c0000, 0x8cffff) AM_WRITE(MWA16_RAM) // not used?
ADDRESS_MAP_END


static ADDRESS_MAP_START( flagrall_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x2003ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x240000, 0x240fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x280000, 0x280fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x2c0000, 0x2c07ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x2c0800, 0x2c0fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x400000, 0x400001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x440000, 0x440001) AM_READ(input_port_1_word_r)
	AM_RANGE(0x480000, 0x480001) AM_READ(input_port_2_word_r)
	AM_RANGE(0x4c0000, 0x4c0001) AM_READ(OKIM6295_status_0_lsb_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( flagrall_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0009CE, 0x0009CF) AM_WRITE(MWA16_NOP) // bug in code? (clean up log)
        AM_RANGE(0x0009D0, 0x0009D1) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x0009D2, 0x0009D3) AM_WRITE(MWA16_NOP) // bug in code? (clean up log)

	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM) // ROM
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM) // Main Ram
	AM_RANGE(0x200000, 0x2003ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16) // palette
	AM_RANGE(0x240000, 0x240fff) AM_WRITE(MWA16_RAM) AM_BASE(&k3_spriteram_1)
	AM_RANGE(0x280000, 0x280fff) AM_WRITE(MWA16_RAM) AM_BASE(&k3_spriteram_2)
	AM_RANGE(0x2c0000, 0x2c07ff) AM_WRITE(k3_bgram_w) AM_BASE(&k3_bgram)
	AM_RANGE(0x2c0800, 0x2c0fff) AM_WRITE(MWA16_RAM)// or does k3 have a bigger tilemap? (flagrall is definitely 32x32 tiles)
	AM_RANGE(0x340000, 0x340001) AM_WRITE(k3_scrollx_w)
	AM_RANGE(0x380000, 0x380001) AM_WRITE(k3_scrolly_w)
	AM_RANGE(0x3c0000, 0x3c0001) AM_WRITE(flagrall_soundbanks_w)
	AM_RANGE(0x4c0000, 0x4c0001) AM_WRITE(OKIM6295_data_0_lsb_w)
ADDRESS_MAP_END


INPUT_PORTS_START( k3 )
	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfff0, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Are these used at all? */


	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x007,  0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0018, 0x0008, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0060, "3" )
	PORT_DIPSETTING(      0x0020, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( flagrall )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 ) // needed to use the continue feature even if it's not used to start the game
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME(  0x0003, 0x0003, DEF_STR( Coinage ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(       0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(       0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(       0x0002, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(  0x0010, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(       0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
	PORT_DIPNAME(  0x0020, 0x0020, "Dip Control" )
	PORT_DIPSETTING(       0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )		
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(  0x0080, 0x0080, "Picture Test" )
	PORT_DIPSETTING(       0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )

	PORT_DIPNAME(  0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(       0x0200, "1" )
	PORT_DIPSETTING(       0x0100, "2" )
	PORT_DIPSETTING(       0x0300, "3" )
	PORT_DIPSETTING(       0x0000, "5" )
	PORT_DIPNAME(  0x0400, 0x0400, "Bonus Type" )
	PORT_DIPSETTING (      0x0400, "0" )
	PORT_DIPSETTING(       0x0000, "1" )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(  0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(       0x0000, "Very_Hard" )
	PORT_DIPSETTING(       0x1000, "Hard" )
	PORT_DIPSETTING(       0x2000, "Easy" )
	PORT_DIPSETTING(       0x3000, "Normal" )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME(  0x8000, 0x8000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(       0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(       0x0000, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout k3_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	16*128,
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &k3_layout,   0x0, 2  }, /* bg tiles */
	{ REGION_GFX2, 0, &k3_layout,   0x0, 2  }, /* bg tiles */
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( flagrall )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(flagrall_readmem, flagrall_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(k3)
	MDRV_VIDEO_UPDATE(k3)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( k3 )
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(k3_readmem,k3_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(k3)
	MDRV_VIDEO_UPDATE(k3)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



ROM_START( 1945kiii )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "prg-1.u51", 0x00001, 0x80000, CRC(6b345f27) SHA1(60867fa0e2ea7ebdd4b8046315ee0c83e5cf0d74) )
	ROM_LOAD16_BYTE( "prg-2.u52", 0x00000, 0x80000, CRC(ce09b98c) SHA1(a06bb712b9cf2249cc535de4055b14a21c68e0c5) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "snd-1.su7", 0x00000, 0x80000, CRC(bbb7f0ff) SHA1(458cf3a0c2d42110bc2427db675226c6b8d30999) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "snd-2.su4", 0x00000, 0x80000, CRC(47e3952e) SHA1(d56524621a3f11981e4434e02f5fdb7e89fff0b4) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) // sprites
	ROM_LOAD32_WORD( "m16m-1.u62", 0x000000, 0x200000, CRC(0b9a6474) SHA1(6110ecb17d0fef25935986af9a251fc6e88e3993) )
	ROM_LOAD32_WORD( "m16m-2.u63", 0x000002, 0x200000, CRC(368a8c2e) SHA1(4b1f360c4a3a86d922035774b2c712be810ec548) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) // bg tiles
	ROM_LOAD( "m16m-3.u61", 0x00000, 0x200000, CRC(32fc80dd) SHA1(bee32493a250e9f21997114bba26b9535b1b636c) )
ROM_END

ROM_START( flagrall )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "11_u34.bin", 0x00001, 0x40000, CRC(24dd439d) SHA1(88857ad5ed69f29de86702dcc746d35b69b3b93d) )
	ROM_LOAD16_BYTE( "12_u35.bin", 0x00000, 0x40000, CRC(373b71a5) SHA1(be9ab93129e2ffd9bfe296c341dbdf47f1949ac7) )

        ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* Samples */
	// 3x banks
	ROM_LOAD( "13_su4.bin", 0x00000, 0x80000, CRC(7b0630b3) SHA1(c615e6630ffd12c122762751c25c249393bf7abd) )
	ROM_LOAD( "14_su6.bin", 0x80000, 0x40000, CRC(593b038f) SHA1(b00dcf321fe541ee52c34b79e69c44f3d7a9cd7c) )

	ROM_REGION( 0x300000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "1_u5.bin",  0x000000, 0x080000, CRC(9377704b) SHA1(ac516a8ba6d1a70086469504c2a46d47a1f4560b) )
	ROM_LOAD32_BYTE( "5_u6.bin",  0x000001, 0x080000, CRC(1ac0bd0c) SHA1(ab71bb84e61f5c7168601695f332a8d4a30d9948) )
	ROM_LOAD32_BYTE( "2_u7.bin",  0x000002, 0x080000, CRC(5f6db2b3) SHA1(84caa019d3b75be30a14d19ccc2f28e5e94028bd) )
	ROM_LOAD32_BYTE( "6_u8.bin",  0x000003, 0x080000, CRC(79e4643c) SHA1(274f2741f39c63e32f49c6a1a72ded1263bdcdaa) )
		
	ROM_LOAD32_BYTE( "3_u58.bin", 0x200000, 0x040000, CRC(c913df7d) SHA1(96e89ecb9e5f4d596d71d7ba35af7b2af4670342) )
	ROM_LOAD32_BYTE( "4_u59.bin", 0x200001, 0x040000, CRC(cb192384) SHA1(329b4c1a4dc388d9f4ce063f9a54cbf3b967682a) )
	ROM_LOAD32_BYTE( "7_u60.bin", 0x200002, 0x040000, CRC(f187a7bf) SHA1(f4ce9ac9fe376250fe426de6ee404fc7841ef08a) )
	ROM_LOAD32_BYTE( "8_u61.bin", 0x200003, 0x040000, CRC(b73fa441) SHA1(a5a3533563070c870276ead5e2f9cb9aaba303cc))

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD( "10_u102.bin", 0x00000, 0x80000, CRC(b1fd3279) SHA1(4a75581e13d43bef441ce81eae518c2f6bc1d5f8) )
	ROM_LOAD( "9_u103.bin",  0x80000, 0x80000, CRC(01e6d654) SHA1(821d61a5b16f5cb76e2a805c8504db1ef38c3a48) )
ROM_END


GAME( 2000, 1945kiii, 0, k3,       k3,       0, ROT270, "Oriental", "1945k III", 0 )
GAME( 1996, flagrall, 0, flagrall, flagrall, 0, ROT0,   "unknown",  "'96 Flag Rally", 0 )

