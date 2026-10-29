// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina, Angelo Salese
/****************************************************************************
    Time Attacker

    driver by Tomasz Slanina analog[at]op.pl

    Z80A,
    xtal 8MHz,
    dipsw 8-position x2,
    volume pots x6,
    2114 ram x5,
    ne555 + 7910CG
    no proms

    TODO:
    - verify video offsets, brick blinking and colors on hardware
    - emulate the Epson melody IC; sound currently uses samples

****************************************************************************/

#include "driver.h"
#include "sound/samples.h"


static tilemap *tmap;

static UINT8 *tattack_ram;
static UINT8 ball_regs[2], paddle_reg, paddle_ysize;
static int bottom_edge_enable, bricks_color_bank;

static void draw_box(mame_bitmap *bitmap, const rectangle *cliprect,
	int x, int y, int w, int h, int pen)
{
	rectangle rect;
	rect.min_x = MAX(x, cliprect->min_x);
	rect.max_x = MIN(x + w - 1, cliprect->max_x);
	rect.min_y = MAX(y, cliprect->min_y);
	rect.max_y = MIN(y + h - 1, cliprect->max_y);
	if (rect.min_x <= rect.max_x && rect.min_y <= rect.max_y)
		fillbitmap(bitmap, Machine->pens[pen], &rect);
}

static void draw_gameplay(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int ram_base = 0x40 + (tattack_ram[0x33] & 0x10);
	int offs, bit;

	for (offs = ram_base; offs < ram_base + 0x0e; offs++)
	{
		for (bit = 7; bit >= 0; bit--)
		{
			if (!(tattack_ram[offs] & (1 << bit)))
				continue;
			if ((tattack_ram[0x33] & 3) == 3 && bit == tattack_ram[0x2c] &&
				(offs & 0x0f) == tattack_ram[0x2b])
				continue;
			draw_box(bitmap, cliprect, bit * 4 + 152, (offs & 0x0f) * 16 + 16,
				3, 15, bricks_color_bank ? 3 : (bit & 4 ? 7 : 5));
		}
	}
	if (!bottom_edge_enable)
		draw_box(bitmap, cliprect, 38, paddle_reg, 4, paddle_ysize, 15);
	draw_box(bitmap, cliprect, ball_regs[0] - 10, ball_regs[1], 3, 3, 15);
	draw_box(bitmap, cliprect, 0, 16, 216, 4, 15);
	draw_box(bitmap, cliprect, 216, 16, 6, 226, 15);
	draw_box(bitmap, cliprect, 0, 238, 216, 4, 15);
	if (bottom_edge_enable)
		draw_box(bitmap, cliprect, 38, 16, 4, 226, 15);
}

static WRITE8_HANDLER( paddle_w )
{
	paddle_reg = data;
}

static WRITE8_HANDLER( ball_w )
{
	ball_regs[offset] = data;
}

static WRITE8_HANDLER( brick_dma_w )
{
	bricks_color_bank = (data >> 6) & 1;
	flip_screen_set(!(data & 0x20));
	paddle_ysize = data & 0x10 ? 8 : 16;
	bottom_edge_enable = (data >> 3) & 1;
}

static WRITE8_HANDLER( sound_w )
{
	int i;
	for (i = 0; i < 4; i++)
		if ((data & (1 << i)) && !sample_playing(i))
			sample_start(i, i, 0);
}

static MACHINE_RESET( tattack )
{
	ball_regs[0] = ball_regs[1] = paddle_reg = 0;
	paddle_ysize = 16;
	bottom_edge_enable = bricks_color_bank = 0;
	flip_screen_set(0);
}



static void get_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	int color=colorram[tile_index];

	if((color&1 ) || (color>15) )
		logerror("COLOR %i\n",color);

	color>>=1;

	SET_TILE_INFO(
		0,
		code,
		color,
		0)
}

VIDEO_UPDATE( tattack )
{
	tilemap_mark_all_tiles_dirty(tmap);
	tilemap_draw(bitmap,cliprect,tmap, 0,0);
	draw_gameplay(bitmap, cliprect);
}

VIDEO_START( tattack )
{
		tmap = tilemap_create( get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32 );
		return tmap ? 0 : 1;
}

static ADDRESS_MAP_START( mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x4000, 0x4000) AM_READ(input_port_3_r)
	AM_RANGE(0x5000, 0x53ff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x6000, 0x6000) AM_READ(input_port_1_r)
	AM_RANGE(0x7000, 0x73ff) AM_RAM AM_BASE(&colorram)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_2_r)
	AM_RANGE(0xc000, 0xc000) AM_READWRITE(input_port_0_r, sound_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(brick_dma_w)
	AM_RANGE(0xc002, 0xc002) AM_WRITENOP
	AM_RANGE(0xc005, 0xc005) AM_WRITE(paddle_w)
	AM_RANGE(0xc006, 0xc007) AM_WRITE(ball_w)
	AM_RANGE(0xe000, 0xe3ff) AM_RAM AM_BASE(&tattack_ram)
ADDRESS_MAP_END

static INPUT_PORTS_START( tattack )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "1-01" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "1-02" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "1-03" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "1-04" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, "Game Time" )
	PORT_DIPSETTING(    0x00, "2:00" )
	PORT_DIPSETTING(    0x01, "2:05" )
	PORT_DIPSETTING(    0x02, "2:10" )
	PORT_DIPSETTING(    0x03, "2:15" )
	PORT_DIPSETTING(    0x04, "2:20" )
	PORT_DIPSETTING(    0x05, "2:25" )
	PORT_DIPSETTING(    0x06, "2:30" )
	PORT_DIPSETTING(    0x07, "2:35" )
	PORT_DIPSETTING(    0x08, "2:40" )
	PORT_DIPSETTING(    0x09, "2:45" )
	PORT_DIPSETTING(    0x0a, "2:50" )
	PORT_DIPSETTING(    0x0b, "2:55" )
	PORT_DIPSETTING(    0x0c, "3:00" )
	PORT_DIPSETTING(    0x0d, "3:05" )
	PORT_DIPSETTING(    0x0e, "3:10" )
	PORT_DIPSETTING(    0x0f, "3:15" )
	PORT_DIPNAME( 0x10, 0x10, "Blinking Brick Awards 30 Seconds" )
	PORT_DIPSETTING(    0x00, "Once Only" )
	PORT_DIPSETTING(    0x10, "No Limit" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x80, "7" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Infinite ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Oil Zones" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "Game Mode" )
	PORT_DIPSETTING(    0x04, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, "Hit 5 Bricks Then Game Over" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Enemies" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, "Show When 40 Bricks Remaining" )
	PORT_DIPSETTING(    0x20, "Show When 20 Bricks Remaining" )
	PORT_DIPSETTING(    0x30, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Enemy Delay" )
	PORT_DIPSETTING(    0x40, "Appear In Last 30 Seconds" )
	PORT_DIPSETTING(    0x00, "Disable" )
	PORT_DIPNAME( 0x80, 0x80, "Oil Zone Delay" )
	PORT_DIPSETTING(    0x80, "Appear In Last 30 Seconds" )
	PORT_DIPSETTING(    0x00, "Disable" )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PADDLE ) PORT_MINMAX(0,0xff) PORT_SENSITIVITY(2) PORT_KEYDELTA(10) PORT_CENTERDELTA(0)
INPUT_PORTS_END


static gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};



static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0     , &charlayout,  0, 8 },
	{ -1 } /* end of array */
};

static PALETTE_INIT( tattack  )
{
	int i,r,g,b;
	for(i=0;i<8;i++)
	{
		if(i)
		{
			r=(i&1)?0xff:0;
			g=(i&2)?0xff:0;
			b=(i&4)?0xff:0;
		}
		else
			r=g=b=128;

		palette_set_color(2*i,0x00,0x00,0xff);
		palette_set_color(2*i+1,r,g,b);
	}
}


static const char *tattack_sample_names[] =
{
	"*tattack",
	"paddle_hit",
	"wall_hit",
	"brick_destroy",
	"win_bgm",
	NULL
};

static struct Samplesinterface tattack_samples_interface =
{
	4,
	tattack_sample_names
};

static MACHINE_DRIVER_START( tattack )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000 / 2)	/* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(mem, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	MDRV_MACHINE_RESET(tattack)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(24, 256-32-1, 13, 256-11-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(tattack )

	MDRV_VIDEO_UPDATE(tattack)
	MDRV_VIDEO_START(tattack)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(tattack_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.6)

MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tattack )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "rom.9a",		0x0000, 0x1000, CRC(47120994) SHA1(b6e90abbc50cba77df4c0aaf50d1f97b99e33b6d) )

	ROM_REGION( 0x800, REGION_USER1, 0 )
	ROM_LOAD( "7910cq", 0x000, 0x800, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX1, 0 )
	ROM_LOAD( "rom.6c",		0x0000, 0x1000, CRC(88ce45cf) SHA1(c7a43bfc9e9c2aeb75a98f723558bc88e53401a7) )

ROM_END

GAME( 1983?, tattack, 0, tattack, tattack, 0, ROT270, "Shonan", "Time Attacker", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_COLORS | GAME_NO_COCKTAIL )
