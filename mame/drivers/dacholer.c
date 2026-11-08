// license:BSD-3-Clause
// copyright-holders:Pierpaolo Prazzoli
/******************************************************************************

    Dacholer    (c) 1983 Nichibutsu
    Kick Boy    (c) 1983 Nichibutsu

    Driver by Pierpaolo Prazzoli

    TODO:
    - Verify background color and audio clocks.

******************************************************************************/

#include "driver.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"
#include "vidhrdw/res_net.h"

static UINT8 *bgvideoram,*fgvideoram;
static int bg_bank;
static UINT8 scroll_x, scroll_y, snd_ack;
static UINT8 snd_interrupt_enable, music_interrupt_enable, msm_data;
static int msm_toggle;

static tilemap *bg_tilemap,*fg_tilemap;

static WRITE8_HANDLER( background_w )
{
	if (bgvideoram[offset] != data)
	{
		bgvideoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

static WRITE8_HANDLER( foreground_w )
{
	if (fgvideoram[offset] != data)
	{
		fgvideoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}

static WRITE8_HANDLER( bg_bank_w )
{
	if((data & 3) != bg_bank)
	{
		bg_bank = data & 3;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	flip_screen_set(data & 0xc); // probably one bit for flipx and one for flipy
}

static WRITE8_HANDLER( coins_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);

	set_led_status(0, data & 4);
	set_led_status(1, data & 8);
}

static WRITE8_HANDLER( snd_irq_w )
{
	snd_interrupt_enable = data;
}

static WRITE8_HANDLER( music_irq_w )
{
	music_interrupt_enable = data;
}

static WRITE8_HANDLER( snd_w )
{
	soundlatch_w(offset, data);
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static READ8_HANDLER( dswa_r )
{
	return (readinputport(3) & 0xef) | ((snd_ack & 1) << 4);
}

static WRITE8_HANDLER( snd_ack_w )
{
	snd_ack = data;
}

static WRITE8_HANDLER( adpcm_w )
{
	msm_data = data;
	msm_toggle = 0;
}

static WRITE8_HANDLER( main_irq_ack_w )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);
}

static WRITE8_HANDLER( bg_scroll_x_w )
{
	scroll_x = data;
}

static WRITE8_HANDLER( bg_scroll_y_w )
{
	scroll_y = data;
}

static INTERRUPT_GEN( sound_irq )
{
	if (music_interrupt_enable == 1)
		cpunum_set_input_line_and_vector(1, 0, HOLD_LINE, 0x30);
}

static void adpcm_int(int param)
{
	if (snd_interrupt_enable == 1 || (!snd_interrupt_enable && msm_toggle))
	{
		MSM5205_data_w(0, msm_data >> 4);
		msm_data <<= 4;
		msm_toggle ^= 1;
		if (!msm_toggle)
			cpunum_set_input_line_and_vector(1, 0, HOLD_LINE, 0x38);
	}
}

static const struct MSM5205interface msm_interface =
{
	adpcm_int,
	MSM5205_S96_4B
};

static MACHINE_RESET( dacholer )
{
	bg_bank = 0;
	scroll_x = scroll_y = snd_ack = 0;
	snd_interrupt_enable = music_interrupt_enable = msm_data = 0;
	msm_toggle = 0;
	flip_screen_set(0);
	if (bg_tilemap)
		tilemap_mark_all_tiles_dirty(bg_tilemap);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8800, 0x97ff) AM_RAM
	AM_RANGE(0xc000, 0xc3ff) AM_MIRROR(0x400) AM_RAM AM_WRITE(background_w) AM_BASE(&bgvideoram)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_WRITE(foreground_w) AM_BASE(&fgvideoram)
	AM_RANGE(0xe000, 0xe0ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( itaten_main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x9fff) AM_ROM
	AM_RANGE(0xa000, 0xb7ff) AM_RAM
	AM_RANGE(0xc000, 0xc3ff) AM_MIRROR(0x400) AM_RAM AM_WRITE(background_w) AM_BASE(&bgvideoram)
	AM_RANGE(0xd000, 0xd3ff) AM_RAM AM_WRITE(foreground_w) AM_BASE(&fgvideoram)
	AM_RANGE(0xe000, 0xe0ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
ADDRESS_MAP_END


static ADDRESS_MAP_START( main_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(dswa_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_4_r)
	AM_RANGE(0x20, 0x20) AM_WRITE(coins_w)
	AM_RANGE(0x21, 0x21) AM_WRITE(bg_bank_w)
	AM_RANGE(0x22, 0x22) AM_WRITE(bg_scroll_x_w)
	AM_RANGE(0x23, 0x23) AM_WRITE(bg_scroll_y_w)
	AM_RANGE(0x24, 0x24) AM_WRITE(main_irq_ack_w)
	AM_RANGE(0x27, 0x27) AM_WRITE(snd_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( itaten_main_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_1_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_2_r)
	AM_RANGE(0x03, 0x03) AM_READ(input_port_3_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_4_r)
	AM_RANGE(0x20, 0x20) AM_WRITE(coins_w)
	AM_RANGE(0x21, 0x21) AM_WRITE(bg_bank_w)
	AM_RANGE(0x22, 0x22) AM_WRITE(bg_scroll_x_w)
	AM_RANGE(0x23, 0x23) AM_WRITE(bg_scroll_y_w)
	AM_RANGE(0x24, 0x24) AM_WRITE(main_irq_ack_w)
	AM_RANGE(0x27, 0x27) AM_WRITE(snd_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( snd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0xd000, 0xe7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( itaten_snd_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0xe000, 0xe7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( snd_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(soundlatch_r, soundlatch_clear_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(music_irq_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(snd_irq_w)
	AM_RANGE(0x0c, 0x0c) AM_WRITE(snd_ack_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(adpcm_w)
	AM_RANGE(0x86, 0x86) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x87, 0x87) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8a, 0x8a) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x8b, 0x8b) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8e, 0x8e) AM_WRITE(AY8910_write_port_2_w)
	AM_RANGE(0x8f, 0x8f) AM_WRITE(AY8910_control_port_2_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( itaten_snd_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(soundlatch_r, soundlatch_clear_w)
	AM_RANGE(0x86, 0x86) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x87, 0x87) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x8a, 0x8a) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x8b, 0x8b) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x8e, 0x8e) AM_WRITE(AY8910_write_port_2_w)
	AM_RANGE(0x8f, 0x8f) AM_WRITE(AY8910_control_port_2_w)
ADDRESS_MAP_END

INPUT_PORTS_START( dacholer )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x3c, 0x3c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x3c, "20k 70k then every 50k" )
	PORT_DIPSETTING(    0x38, "30k 80k then every 50k" )
	PORT_DIPSETTING(    0x34, "40k 90k then every 50k" )
	PORT_DIPSETTING(    0x2c, "20k 90k then every 70k" )
	PORT_DIPSETTING(    0x28, "30k 100k then every 70k" )
	PORT_DIPSETTING(    0x24, "40k 110k then every 70k" )
	PORT_DIPSETTING(    0x1c, "20k 120k then every 100k" )
	PORT_DIPSETTING(    0x18, "30k 130k then every 100k" )
	PORT_DIPSETTING(    0x14, "40k 140k then every 100k" )
	PORT_DIPSETTING(    0x0c, "20k only" )
	PORT_DIPSETTING(    0x08, "30k only" )
	PORT_DIPSETTING(    0x04, "40k only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( kickboy )
	PORT_INCLUDE(dacholer)
	PORT_MODIFY("DSWA")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "99 (Cheat)" )
	PORT_DIPNAME( 0x3c, 0x3c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x3c, "20k 50k then every 30k" )
	PORT_DIPSETTING(    0x38, "30k 60k then every 30k" )
	PORT_DIPSETTING(    0x34, "40k 70k then every 30k" )
	PORT_DIPSETTING(    0x2c, "20k 70k then every 50k" )
	PORT_DIPSETTING(    0x28, "30k 80k then every 50k" )
	PORT_DIPSETTING(    0x24, "40k 90k then every 50k" )
	PORT_DIPSETTING(    0x1c, "20k 90k then every 70k" )
	PORT_DIPSETTING(    0x18, "30k 100k then every 70k" )
	PORT_DIPSETTING(    0x14, "40k 110k then every 70k" )
	PORT_DIPSETTING(    0x0c, "20k only" )
	PORT_DIPSETTING(    0x08, "30k only" )
	PORT_DIPSETTING(    0x04, "40k only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
INPUT_PORTS_END

INPUT_PORTS_START( itaten )
	PORT_START_TAG("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "30k then every 50k" )
	PORT_DIPSETTING(    0x20, "60k then every 50k" )
	PORT_DIPSETTING(    0x10, "30k then every 90k" )
	PORT_DIPSETTING(    0x00, "60k then every 90k" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )
INPUT_PORTS_END

static void get_bg_tile_info(int tile_index)
{
	SET_TILE_INFO(1,bgvideoram[tile_index] + bg_bank * 0x100,0,0);
}

static void get_fg_tile_info(int tile_index)
{
	SET_TILE_INFO(0,fgvideoram[tile_index],0,0);
}

VIDEO_START( dacholer )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,32,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!bg_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect )
{
	int offs,code,attr,sx,sy,flipx,flipy;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		code = spriteram[offs+1];
		attr = spriteram[offs+2];

		flipx = attr & 0x10;
		flipy = attr & 0x20;

		sx = (spriteram[offs+3] - 128) + 256 * (attr & 0x01);
		sy = 255 - spriteram[offs];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[2],
				code,
				0,
				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE(dacholer)
{
	tilemap_set_scrollx(bg_tilemap, 0, flip_screen ? 256 - scroll_x : scroll_x);
	tilemap_set_scrolly(bg_tilemap, 0, flip_screen ? 256 - scroll_y : scroll_y);
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4,0,12,8,20,16,28,24,36,32,44,40,52,48,60,56 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	16*16*4
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 1 },
	{ REGION_GFX2, 0, &charlayout,  16, 1 },
	{ REGION_GFX3, 0, &spritelayout, 16, 1 },
	{ -1 }
};


static const gfx_decode itaten_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 1 },
	{ REGION_GFX2, 0, &charlayout,    0, 1 },
	{ REGION_GFX3, 0, &spritelayout, 16, 1 },
	{ -1 }
};

static PALETTE_INIT( dacholer )
{
	static const int resistances_rg[3] = { 1000, 470, 220 };
	static const int resistances_b[2] = { 470, 220 };
	double weights_rg[3], weights_b[2];
	int i;

	compute_resistor_weights(0, 255, -1.0,
		3, resistances_rg, weights_rg, 0, 0,
		2, resistances_b, weights_b, 0, 0,
		0, 0, 0, 0, 0);
	for (i = 0; i < 32; i++)
	{
		int value = color_prom[i];
		int r = combine_3_weights(weights_rg, ((value >> 0) & 1), ((value >> 1) & 1), ((value >> 2) & 1));
		int g = combine_3_weights(weights_rg, ((value >> 3) & 1), ((value >> 4) & 1), ((value >> 5) & 1));
		int b = combine_2_weights(weights_b, ((value >> 6) & 1), ((value >> 7) & 1));
		palette_set_color(i, r, g, b);
	}
}

static MACHINE_DRIVER_START( dacholer )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 16000000/4)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 1)

	MDRV_CPU_ADD_TAG("sound", Z80, 19968000/8)
	MDRV_CPU_PROGRAM_MAP(snd_map, 0)
	MDRV_CPU_IO_MAP(snd_io_map, 0)
	MDRV_CPU_VBLANK_INT(sound_irq, 1)

	MDRV_MACHINE_RESET(dacholer)
	MDRV_INTERLEAVE(10)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-1-16)
	MDRV_PALETTE_LENGTH(32)
	MDRV_PALETTE_INIT(dacholer)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(dacholer)
	MDRV_VIDEO_UPDATE(dacholer)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 19968000/16)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

	MDRV_SOUND_ADD(AY8910, 19968000/16)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

	MDRV_SOUND_ADD(AY8910, 19968000/16)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

	MDRV_SOUND_ADD_TAG("msm", MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( itaten )
	MDRV_IMPORT_FROM(dacholer)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(itaten_main_map, 0)
	MDRV_CPU_IO_MAP(itaten_main_io_map, 0)
	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_PROGRAM_MAP(itaten_snd_map, 0)
	MDRV_CPU_IO_MAP(itaten_snd_io_map, 0)
	MDRV_CPU_VBLANK_INT(0, 0)
	MDRV_GFXDECODE(itaten_gfxdecodeinfo)
	MDRV_SOUND_REMOVE("msm")
MACHINE_DRIVER_END

ROM_START( dacholer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dp_1.5k", 0x0000, 0x2000, CRC(8b73a441) SHA1(6de9e4845b9063af8df42aa82ad536737190582c) )
	ROM_LOAD( "dp_2.5l", 0x2000, 0x2000, CRC(9499289f) SHA1(bcfe554eb1f8e686d193050c18278b6bf93f179f) )
	ROM_LOAD( "dp_3.5m", 0x4000, 0x2000, CRC(39d37281) SHA1(daaf84079dd18dd854946e066e2dcde994bcbba4) )
	ROM_LOAD( "dp_4.5n", 0x6000, 0x2000, CRC(bb781ea4) SHA1(170966c4bcd0246968850d908a69f81ea1e136d5) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "ds_1.6g", 0x0000, 0x2000, CRC(cc3a4b68) SHA1(29344dc10c5d236f9a452196b3809565b4101327) )
	ROM_LOAD( "ds_2.6h", 0x2000, 0x2000, CRC(aa18e126) SHA1(e6af334188d0edbc37a7fb4a00a325b2039172b7) )
	ROM_LOAD( "ds_3.6j", 0x4000, 0x2000, CRC(3b0131c7) SHA1(338ca2c2c7480e1cd0bb15ee6b90d683ce06f0fd) )
	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dc_7.12j", 0x0000, 0x2000, CRC(fd649d36) SHA1(77d78eef44f348b635dbc0711e662a5236c00d51) )
	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dc_3.13a", 0x0000, 0x2000, CRC(9cca0fd2) SHA1(3ca1b4cca9611232df1195ae6ac172a79c8368c3) )
	ROM_LOAD( "dc_2.12a", 0x2000, 0x2000, CRC(c1322b27) SHA1(8022f59b8ae10a7a911563b01bffc2d5646108a5) )
	ROM_LOAD( "dc_1.11a", 0x4000, 0x2000, CRC(9e1e7198) SHA1(7a75da1ae09f6cf095976b48f462ede42625b244) )
	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "dc_5.2d", 0x0000, 0x2000, CRC(dd4818f0) SHA1(718236932248512f8779f640e0367b5d92e6497e) )
	ROM_LOAD( "dc_4.1d", 0x2000, 0x2000, CRC(7f338ae0) SHA1(9206ed044feb44c55990803cdf608dd899e976ff) )
	ROM_LOAD( "dc_6.3d", 0x4000, 0x2000, CRC(0a6d4ec4) SHA1(419ea1f6ead3afb2de98432d9f8ead7447842a1e) )
	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "dc.13d",  0x0000, 0x0020, CRC(d273abe5) SHA1(219bcba7f3e961f6b2cfbf48ac6ae6b6d80b974c) )
	ROM_LOAD( "af-2.1h", 0x0020, 0x0020, CRC(e1cac297) SHA1(f15326d04d006d9d029a6565aebf9daf3657bc2a) )
	ROM_LOAD( "af-1.3n", 0x0040, 0x0020, CRC(5638e485) SHA1(5d892111936a8eb7646c03a17300069be9a2b442) )
ROM_END

ROM_START( kickboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "k_1.5k", 0x0000, 0x2000, CRC(525746f1) SHA1(4044f880f271f77b56b2d8964ab97d34fb507c7a) )
	ROM_LOAD( "k_2.5l", 0x2000, 0x2000, CRC(9d091725) SHA1(827cea1c371094720b47fda271945cee20c9d956) )
	ROM_LOAD( "k_3.5m", 0x4000, 0x2000, CRC(d61b6ff6) SHA1(071ab4c05ed54526144f2ba751c111e8c4bdc61a) )
	ROM_LOAD( "k_4.5n", 0x6000, 0x2000, CRC(a8985bfe) SHA1(a8e466a7df381dfc8dd2e3483eba0215bfec7551) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "k_1.6g", 0x0000, 0x2000, CRC(cc3a4b68) SHA1(29344dc10c5d236f9a452196b3809565b4101327) )
	ROM_LOAD( "k_2.6h", 0x2000, 0x2000, CRC(aa18e126) SHA1(e6af334188d0edbc37a7fb4a00a325b2039172b7) )
	ROM_LOAD( "k_3.6j", 0x4000, 0x2000, CRC(3b0131c7) SHA1(338ca2c2c7480e1cd0bb15ee6b90d683ce06f0fd) )
	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "k_7.12j", 0x0000, 0x2000, CRC(22be46e8) SHA1(d92b3913d8eba881c69acd1d85ca73ee58489fae) )
	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "k_3.13a", 0x0000, 0x2000, CRC(7eac2a64) SHA1(b4a44770bbded59cd572ac5d0ae178affc8cdab8) )
	ROM_LOAD( "k_2.12a", 0x2000, 0x2000, CRC(b8829572) SHA1(01009ec63449c809608923fd9dcecd82b29c5d6d) )
	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "k_5.2d", 0x0000, 0x2000, CRC(4b769a1c) SHA1(fde17dcd4b7cda9cc54572e81bc2f0e48c19277d) )
	ROM_LOAD( "k_4.1d", 0x2000, 0x2000, CRC(45199750) SHA1(a04b4d6d0defa613d269625b089d28dc68d5b73a) )
	ROM_LOAD( "k_6.3d", 0x4000, 0x2000, CRC(d1795506) SHA1(e0f7a64e301cf43c4739031461dba16aa44100a1) )
	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "k.13d",   0x0000, 0x0020, CRC(82f87a36) SHA1(5dc2059eb5b6cd541b014347c36198b8838d98fa) )
	ROM_LOAD( "af-2.1h", 0x0020, 0x0020, CRC(e1cac297) SHA1(f15326d04d006d9d029a6565aebf9daf3657bc2a) )
	ROM_LOAD( "af-1.3n", 0x0040, 0x0020, CRC(5638e485) SHA1(5d892111936a8eb7646c03a17300069be9a2b442) )
ROM_END

ROM_START( itaten )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.5k",  0x0000, 0x2000, CRC(84c8a010) SHA1(52d78ac70b3d5e905a11efd76acd99810c56e467) )
	ROM_LOAD( "2.5l",  0x2000, 0x2000, CRC(19946038) SHA1(74f76096e676535ead4386755fce853caac7673b) )
	ROM_LOAD( "3.5m",  0x4000, 0x2000, CRC(4f9e26fd) SHA1(33062724c46108611c9db16fdbf2cb9feed7e213) )
	ROM_LOAD( "4.1f",  0x6000, 0x2000, CRC(35f85aeb) SHA1(ecd8f62e304d1277332a5a2b7ec6aace9f77d8ad) )
	ROM_LOAD( "5.1e",  0x8000, 0x2000, CRC(6cf30924) SHA1(5e82e9aa0811ec1b853d300368c5ceec44938363) )
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "6.6g",  0x0000, 0x2000, CRC(dfcb1a3e) SHA1(cee0906cfbddd0254a947737da1cfbe47c445c32) )
	ROM_LOAD( "7.6h",  0x2000, 0x1000, CRC(844e78d6) SHA1(11b48af650809f8504b56e9a8e53c9f043782c5f) )
	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "16.12j", 0x0000, 0x2000, CRC(8af2bfb8) SHA1(6744db0deb4fda7920fcfddf7f9c1ed6681d3622) )
	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11.13a", 0x0000, 0x2000, CRC(ed3279d5) SHA1(e4bcae8038739c588f896ff35fa95288979fa683) )
	ROM_LOAD( "10.12a", 0x2000, 0x2000, CRC(d2b60e5d) SHA1(c833ac6e5d4d0a244ace600f5f02c6f43d3bd34b) )
	ROM_LOAD( "9.11a",  0x4000, 0x2000, CRC(919cac5e) SHA1(9602ad7618b5e4d93fa22676aa855256d4690dd1) )
	ROM_LOAD( "8.10a",  0x6000, 0x2000, CRC(c32b0859) SHA1(1bb00de55742a1f2cbbf3970b43b122114b0911b) )
	ROM_REGION( 0x8000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "13.2d",  0x0000, 0x2000, CRC(d32559f5) SHA1(a4f05b1c8c48aad367ff675c29a9a1828c71b693) )
	ROM_LOAD( "12.1d",  0x2000, 0x2000, CRC(f0f64636) SHA1(a3354be74460e45453fea62c8dd910f98b5d2fb5) )
	ROM_LOAD( "14.3d",  0x4000, 0x2000, CRC(8c532c74) SHA1(c95786c81f82f7211f4411a9f39fd4ba4def9073) )
	ROM_LOAD( "15.4d",  0x6000, 0x2000, CRC(d119b483) SHA1(c1e403369bfbda0233ec5764fc703522e9f312a7) )
	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "af-3.13d", 0x0000, 0x0020, CRC(875429ba) SHA1(7186e1fb15806d60fd3e704be8db94c7b6c8c058) )
	ROM_LOAD( "af-2.1h",  0x0020, 0x0020, CRC(e1cac297) SHA1(f15326d04d006d9d029a6565aebf9daf3657bc2a) )
	ROM_LOAD( "af-1.3n",  0x0040, 0x0020, CRC(5638e485) SHA1(5d892111936a8eb7646c03a17300069be9a2b442) )
ROM_END

GAME( 1983, dacholer, 0, dacholer, dacholer, 0, ROT0, "Nichibutsu", "Dacholer", 0 )
GAME( 1983, kickboy,  0, dacholer, kickboy,  0, ROT0, "Nichibutsu", "Kick Boy", 0 )
GAME( 1984, itaten,   0, itaten,   itaten,   0, ROT0, "Nichibutsu / Alice", "Itazura Tenshi (Japan)", 0 )
