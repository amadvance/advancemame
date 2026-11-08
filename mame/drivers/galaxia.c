// license:BSD-3-Clause
// copyright-holders:David Haywood, hap, Nicola Salmoria, Peter Trauner
/* Galxaia */

/*

Galaxia by Zaccaria (1980)

Taken from an untested board.

1K byte files were 2708 or equivalent.
512 byte file is the 82S130 colour PROM.

This is not a direct pirate of Galaxians as you might think from the name.
The game uses a Signetics 2650A CPU with three 40-pin 2636 chips. I have
no idea what 2636's are but I am hoping they are something to do with the
sound since the board has no apparent sound circuitry. The video hardware
looks like it's similar to Galaxians (2 x 2114, 2 x 2101, 2 x EPROM) but
there is no attack RAM and the graphics EPROMS are 2708. The graphics EPROMS
do contain Galaxian-like graphics...

---

rom mapping is still wrong, correct this before anything else

*/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "streams.h"
#include "sound/custom.h"

UINT8 *galaxia_video;

VIDEO_START( galaxia )
{
	return 0;
}

VIDEO_UPDATE( galaxia )
{
	int x,y, count;

	fillbitmap(bitmap,Machine->pens[0],cliprect);
	count = 0;

	for (y=0;y<256/8;y++)
	{
		for (x=0;x<256/8;x++)
		{
			int tile = galaxia_video[count];
			drawgfx(bitmap,Machine->gfx[0],tile,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_NONE,0);
			count++;
		}

	}



}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1400, 0x1fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5000, 0x6fff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x1400, 0x14ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1500, 0x15ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1600, 0x16ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1700, 0x17ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x18ff) AM_WRITE(MWA8_RAM) AM_BASE(&galaxia_video)
	AM_RANGE(0x1900, 0x19ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1a00, 0x1aff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1b00, 0x1bff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1c00, 0x1cff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1d00, 0x1dff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1e00, 0x1eff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1f00, 0x1fff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x02, 0x02) AM_READ(input_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
ADDRESS_MAP_END


INPUT_PORTS_START( galaxia )
	PORT_START	/*  */

	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};





static MACHINE_DRIVER_START( galaxia )
	/* basic machine hardware */
	MDRV_CPU_ADD(S2650,2000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
//  MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(galaxia)
	MDRV_VIDEO_UPDATE(galaxia)
MACHINE_DRIVER_END


static UINT8 astrowar_video[0x400], astrowar_color[0x400];
static UINT8 astrowar_pvi[0x100];
static UINT8 *astrowar_bullet;
static UINT8 astrowar_collision, astrowar_stars_on, astrowar_scroll;
static mame_bitmap *astrowar_background, *astrowar_sprites;
static sound_stream *astrowar_stream;
static mame_bitmap *astrowar_skipped_bitmap;
static int astrowar_last_scanline;
static VIDEO_UPDATE( astrowar );

static void astrowar_partial_update(int scanline)
{
	if (skip_this_frame())
	{
		rectangle clip = Machine->visible_area;
		/* Collision latches must advance even when the display skips a frame. */
		if (clip.min_y < astrowar_last_scanline) clip.min_y = astrowar_last_scanline;
		if (clip.max_y > scanline) clip.max_y = scanline;
		if (clip.min_y <= clip.max_y)
		{
			video_update_astrowar(0, astrowar_skipped_bitmap, &clip);
			astrowar_last_scanline = clip.max_y + 1;
		}
	}
	else
		force_partial_update(scanline);
}

static VIDEO_EOF( astrowar )
{
	astrowar_partial_update(Machine->visible_area.max_y);
	astrowar_last_scanline = 0;
}

static int astrowar_sound_pos, astrowar_sound_period;
static int astrowar_stars_scroll, astrowar_star_count;
static struct { int x, y, color; } astrowar_stars[256];

static void astrowar_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int i;
	for (i = 0; i < samples; ++i)
	{
		outputs[0][i] = astrowar_pvi[0xc7] && astrowar_sound_pos <= astrowar_sound_period / 2 ? 0x7fff : 0;
		if (++astrowar_sound_pos > astrowar_sound_period)
			astrowar_sound_pos = 0;
	}
}

static void *astrowar_sound_start(int clock, const struct CustomSound_interface *config)
{
	astrowar_stream = stream_create(0, 1, Machine->sample_rate, 0, astrowar_sound_update);
	return astrowar_stream;
}

static struct CustomSound_interface astrowar_sound_interface =
{
	astrowar_sound_start
};

static WRITE8_HANDLER( astrowar_pvi_w )
{
	offset &= (offset & 0xc0) == 0xc0 ? 0xcf : 0xff;
	if (offset == 0xc7)
	{
		stream_update(astrowar_stream, 0);
		astrowar_sound_pos = 0;
		astrowar_sound_period = Machine->sample_rate * (data + 1) / 7874;
	}
	astrowar_pvi[offset] = data;
}

static READ8_HANDLER( astrowar_pvi_r )
{
	UINT8 data;
	offset &= (offset & 0xc0) == 0xc0 ? 0xcf : 0xff;
	if (offset == 0xca || offset == 0xcb)
	{
		/* Latch collisions up to this scanline before acknowledging them. */
		astrowar_partial_update(cpu_getscanline());
		data = astrowar_pvi[offset];
		astrowar_pvi[offset] = 0;
		return data;
	}
	return astrowar_pvi[offset];
}

static READ8_HANDLER( astrowar_video_r )
{
	return activecpu_get_reg(S2650_FO) ? astrowar_video[offset] : astrowar_color[offset];
}

static WRITE8_HANDLER( astrowar_video_w )
{
	/* The CPU flag selects two physical RAM banks at the same address. */
	if (activecpu_get_reg(S2650_FO))
		astrowar_video[offset] = data;
	else
		astrowar_color[offset] = data;
}

static READ8_HANDLER( astrowar_collision_r )
{
	astrowar_partial_update(cpu_getscanline());
	return astrowar_collision;
}

static READ8_HANDLER( astrowar_collision_clear_r )
{
	astrowar_partial_update(cpu_getscanline());
	astrowar_collision = 0;
	return 0;
}

static WRITE8_HANDLER( astrowar_scroll_w )
{
	astrowar_partial_update(cpu_getscanline());
	astrowar_scroll = data;
}

static WRITE8_HANDLER( astrowar_control_w )
{
	astrowar_stars_on = data & 0x20;
}

static INTERRUPT_GEN( astrowar_interrupt )
{
	astrowar_stars_scroll = (astrowar_stars_scroll + 1) % 480;
	astrowar_pvi[0xcb] = 0;
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x03);
}

static MACHINE_RESET( astrowar )
{
	memset(astrowar_video, 0, sizeof(astrowar_video));
	memset(astrowar_color, 0, sizeof(astrowar_color));
	memset(astrowar_pvi, 0, sizeof(astrowar_pvi));
	astrowar_collision = astrowar_stars_on = astrowar_scroll = 0;
	astrowar_sound_pos = astrowar_sound_period = astrowar_stars_scroll = 0;
	astrowar_last_scanline = 0;
}

static PALETTE_INIT( astrowar )
{
	int i;
	for (i = 0; i < 8; ++i)
	{
		palette_set_color(i * 2, 0, 0, 0);
		palette_set_color(i * 2 + 1, (~i & 4) ? 255 : 0, (~i & 2) ? 255 : 0, (~i & 1) ? 255 : 0);
		palette_set_color(i + 16, (i & 1) ? 255 : 0, (i & 2) ? 255 : 0, (i & 4) ? 255 : 0);
	}
	for (i = 0; i < 64; ++i)
		palette_set_color(i + 24, ((i >> 4) & 3) * 85, ((i >> 2) & 3) * 85, (i & 3) * 85);
	palette_set_color(88, 255, 255, 255);
}

static VIDEO_START( astrowar )
{
	UINT32 generator = 0;
	int x, y;
	astrowar_background = auto_bitmap_alloc(256, 312);
	astrowar_skipped_bitmap = auto_bitmap_alloc(256, 312);
	astrowar_sprites = auto_bitmap_alloc(256, 312);
	astrowar_star_count = 0;
	for (y = 0; y < 272; ++y)
		for (x = 0; x < 480; ++x)
		{
			generator <<= 1;
			generator |= ((~generator >> 17) ^ (generator >> 5)) & 1;
			if ((generator & 0x100fe) == 0xfe && astrowar_star_count < 256)
			{
				astrowar_stars[astrowar_star_count].x = x;
				astrowar_stars[astrowar_star_count].y = y;
				astrowar_stars[astrowar_star_count++].color = (generator >> 8) & 63;
			}
		}
	return 0;
}

static void astrowar_draw_sprite(UINT8 *attr, int color, int x, int y, int expand, int object, const rectangle *cliprect)
{
	int sx, sy, ex, ey;
	for (sy = 0; sy < 10; ++sy)
		for (sx = 0; sx < 8; ++sx)
			if ((attr[sy] << sx) & 0x80)
				for (ey = 0; ey <= expand; ++ey)
					for (ex = 0; ex <= expand; ++ex)
					{
						int tx = x + sx * (expand + 1) + ex;
						int ty = y + sy * (expand + 1) + ey;
						if (tx >= cliprect->min_x && tx <= cliprect->max_x && ty >= cliprect->min_y && ty <= cliprect->max_y)
							((UINT16 *)astrowar_sprites->line[ty])[tx] |= color | (0x100 << object);
					}
}

static VIDEO_UPDATE( astrowar )
{
	static const int offsets[4] = { 0, 0x10, 0x20, 0x40 };
	static const int collision_bits[4][4] = { {0,0x20,0x10,8}, {0,0,4,2}, {0,0,0,1}, {0,0,0,0} };
	int x, y, i, j;
	rectangle bg_clip = *cliprect;
	if (bg_clip.max_y > 255) bg_clip.max_y = 255;
	fillbitmap(bitmap, 0, cliprect);
	fillbitmap(astrowar_background, 0, cliprect);
	fillbitmap(astrowar_sprites, 0, cliprect);
	if (astrowar_stars_on)
		for (i = 0; i < astrowar_star_count; ++i)
		{
			x = ((astrowar_stars[i].x + astrowar_stars_scroll) >> 1) % 240;
			y = astrowar_stars[i].y;
			if (((y >> 4) ^ (y >> 8) ^ (x >> 5)) & 1)
				if (x >= cliprect->min_x && x <= cliprect->max_x && y >= cliprect->min_y && y <= cliprect->max_y)
					((UINT16 *)bitmap->line[y])[x] = 24 + astrowar_stars[i].color;
		}
	for (i = 0; i < 0x400; ++i)
	{
		x = i & 31;
		y = ((i >> 5) * 8 - ((x >= 4 && x < 24) ? astrowar_scroll : 0)) & 255;
		drawgfx(astrowar_background, Machine->gfx[0], astrowar_video[i], astrowar_color[i] & 7, 0, 0,
			x * 8, y, &bg_clip, TRANSPARENCY_PEN, 0);
		if (y > 248)
			drawgfx(astrowar_background, Machine->gfx[0], astrowar_video[i], astrowar_color[i] & 7, 0, 0,
				x * 8, y - 256, &bg_clip, TRANSPARENCY_PEN, 0);
	}
	copybitmap(bitmap, astrowar_background, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
	for (i = 0; i < 4; ++i)
	{
		UINT8 *attr = astrowar_pvi + offsets[i];
		int color = (astrowar_pvi[0xc1 + (i >> 1)] >> ((i & 1) ? 0 : 3)) & 7;
		int expand = (astrowar_pvi[0xc0] >> (i * 2)) & 3;
		if (attr[0x0a] == 0xff) continue;
		y = attr[0x0c] + 3;
		astrowar_draw_sprite(attr, color, attr[0x0a] - 8, y, expand, i, cliprect);
		if (attr[0x0b] != 0xff && attr[0x0d] != 0xfe)
			while (y < 255)
			{
				y += 10 + attr[0x0d];
				astrowar_draw_sprite(attr, color, attr[0x0b] - 8, y, expand, i, cliprect);
			}
	}
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];
		UINT16 *bg = (UINT16 *)astrowar_background->line[y];
		UINT16 *sprites = (UINT16 *)astrowar_sprites->line[y];
		int bullet = y < 256 ? (astrowar_bullet[y] ^ 0xff) - 8 : 247;
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			if (bullet != 247 && x <= bullet && x > bullet - 4)
			{
				if (bg[x] & 1) astrowar_collision |= 2;
				dest[x] = 88;
			}
		}
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			/* The sprite generator has a slower horizontal clock than the tile circuit. */
			int sx = x * 256 / 196;
			int sx2 = (x * 512 + 196) / 392;
			int pixel = sprites[x];
			if (sx2 > cliprect->max_x) break;
			if (pixel & 0xf00)
			{
				if ((bg[sx] | bg[sx2]) & 1) astrowar_collision |= 1;
				dest[sx] = dest[sx2] = (pixel & 7) + 16;
				for (i = 0; i < 3; ++i)
					for (j = i + 1; j < 4; ++j)
						if ((pixel & (0x100 << i)) && (pixel & (0x100 << j)))
							astrowar_pvi[0xcb] |= collision_bits[i][j];
			}
		}
	}
}

static ADDRESS_MAP_START( astrowar_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_READ(MRA8_ROM)
	AM_RANGE(0x1400, 0x14ff) AM_MIRROR(0x6000) AM_READ(MRA8_RAM)
	AM_RANGE(0x1500, 0x15ff) AM_MIRROR(0x6000) AM_READ(astrowar_pvi_r)
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0x6000) AM_READ(astrowar_video_r)
	AM_RANGE(0x1c00, 0x1cff) AM_MIRROR(0x6000) AM_READ(MRA8_RAM)
	AM_RANGE(0x2000, 0x33ff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( astrowar_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x1400, 0x14ff) AM_MIRROR(0x6000) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1500, 0x15ff) AM_MIRROR(0x6000) AM_WRITE(astrowar_pvi_w)
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0x6000) AM_WRITE(astrowar_video_w)
	AM_RANGE(0x1c00, 0x1cff) AM_MIRROR(0x6000) AM_WRITE(MWA8_RAM) AM_BASE(&astrowar_bullet)
ADDRESS_MAP_END

static ADDRESS_MAP_START( astrowar_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xf8) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_MIRROR(0xf8) AM_READ(input_port_1_r)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xf8) AM_READ(input_port_2_r)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xf8) AM_READ(input_port_3_r)
	AM_RANGE(S2650_CTRL_PORT, S2650_CTRL_PORT) AM_READ(astrowar_collision_r)
	AM_RANGE(S2650_DATA_PORT, S2650_DATA_PORT) AM_READ(astrowar_collision_clear_r)
	AM_RANGE(S2650_SENSE_PORT, S2650_SENSE_PORT) AM_READ(input_port_4_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( astrowar_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xf8) AM_WRITE(astrowar_scroll_w)
	AM_RANGE(S2650_CTRL_PORT, S2650_CTRL_PORT) AM_WRITE(astrowar_control_w)
ADDRESS_MAP_END

static const gfx_layout astrowar_tiles =
{
	8,8, RGN_FRAC(1,1), 1, { 0 },
	{ 0,1,2,3,4,5,6,7 }, { 0,8,16,24,32,40,48,56 }, 64
};

static const gfx_decode astrowar_gfxdecode[] =
{
	{ REGION_GFX1, 0, &astrowar_tiles, 0, 8 },
	{ -1 }
};

static MACHINE_DRIVER_START( astrowar )
	MDRV_CPU_ADD(S2650, 14318181 / 8)
	MDRV_CPU_PROGRAM_MAP(astrowar_readmem,astrowar_writemem)
	MDRV_CPU_IO_MAP(astrowar_readport,astrowar_writeport)
	MDRV_CPU_VBLANK_INT(astrowar_interrupt,1)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(3500)
	MDRV_MACHINE_RESET(astrowar)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256,312)
	MDRV_VISIBLE_AREA(0,231,0,271)
	MDRV_GFXDECODE(astrowar_gfxdecode)
	MDRV_PALETTE_LENGTH(89)
	MDRV_PALETTE_INIT(astrowar)
	MDRV_VIDEO_START(astrowar)
	MDRV_VIDEO_UPDATE(astrowar)
	MDRV_VIDEO_EOF(astrowar)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(CUSTOM,0)
	MDRV_SOUND_CONFIG(astrowar_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.25)
MACHINE_DRIVER_END

INPUT_PORTS_START( astrowar )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Hard ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "High Score" )
	PORT_DIPSETTING(    0x00, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, "Random" )
	PORT_DIPNAME( 0x06, 0x00, "Random H.S." )
	PORT_DIPSETTING(    0x00, DEF_STR( Low ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, "Medium-High" )
	PORT_DIPSETTING(    0x06, DEF_STR( High ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPSETTING(    0x08, "3000" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPSETTING(    0x18, "7000" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

ROM_START( galaxia )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "08h.bin", 0x00000, 0x0400, CRC(f3b4ffde) SHA1(15b004e7821bfc145158b1e9435f061c524f6b86) )
	ROM_LOAD( "10h.bin", 0x00400, 0x0400, CRC(6d07fdd4) SHA1(d7d4b345a055275d59951788569db370bccd5195) )
	ROM_LOAD( "08i.bin", 0x00800, 0x0400, CRC(45b88599) SHA1(3b79c21db1aa9d80fac81ac5a554e438805febd1) )
	ROM_LOAD( "10i.bin", 0x00c00, 0x0400, CRC(c0baa654) SHA1(80e0880c32ad285fbce0f7f552268b964b97cab3) )

	ROM_LOAD( "11h.bin", 0x05000, 0x0400, CRC(1520eb3d) SHA1(3683174da701e1124af0f9c2ee4a9a84f3fea33a) )
	ROM_LOAD( "13h.bin", 0x05400, 0x0400, CRC(c4482770) SHA1(aee983cc3d80989f49aea4138961bb623039484a) )
	ROM_LOAD( "11i.bin", 0x05800, 0x0400, CRC(4456808a) SHA1(f9e8cfdde0e17f13f1be297b2b4503ccc959b33c) )
	ROM_LOAD( "13i.bin", 0x05c00, 0x0400, CRC(cf653b9a) SHA1(fef5943de60cb5ba2459fc6ae7419e29c96a76cd) )
	ROM_LOAD( "11l.bin", 0x06000, 0x0400, CRC(50c6a645) SHA1(46638907bc393df6be25fc7461d73047d1746ffc) )
	ROM_LOAD( "13l.bin", 0x06400, 0x0400, CRC(3a9c38c7) SHA1(d1e934092b69c0f3f9636eba05a1d8a6d9588e6b) )

	ROM_REGION( 0x0800, REGION_GFX1, 0 )
	ROM_LOAD( "01d.bin", 0x00000, 0x0400, CRC(2dd50aab) SHA1(758d7a5383c9a1ee134d99e3f7025819cfbe0e0f) )
	ROM_LOAD( "03d.bin", 0x00400, 0x0400, CRC(1dc30185) SHA1(e3c75eecb80b376ece98f602e1b9587487841824) )

	ROM_REGION( 0x80000, REGION_PROMS, 0 )
	ROM_LOAD( "11o", 0x00000, 0x0200, CRC(ae816417) SHA1(9497857d13c943a2735c3b85798199054e613b2c) )
ROM_END

ROM_START( astrowar )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "astro.8h",  0x00000, 0x0400, CRC(b0ec246c) SHA1(f9123b5e317938655f5e8b3f8a5810d0b2b7c7af) )
	ROM_LOAD( "astro.10h", 0x00400, 0x0400, CRC(090d360f) SHA1(528ddcdc30a5a291bd8850ff6f134fcc19af562f) )
	ROM_LOAD( "astro.11h", 0x00800, 0x0400, CRC(72ab1378) SHA1(50743c64c4775076aa6f1d8ab2e05c14884bf0ba) )
	ROM_LOAD( "astro.13h", 0x00c00, 0x0400, CRC(2dc4c895) SHA1(831afbfd4ebfd6522ab0758222bc6f9826148a5d) )
	ROM_LOAD( "astro.8i",  0x01000, 0x0400, CRC(ab87fbfc) SHA1(34b670f96c260f186c643e588995ae5d80377784) )
	ROM_LOAD( "astro.10i", 0x02000, 0x0400, CRC(533675c1) SHA1(69cc066e1874a135a53a21b7b2461bda456504f1) )
	ROM_LOAD( "astro.11i", 0x02400, 0x0400, CRC(59cf8901) SHA1(e849d4c99350b7e3453c156d91618b71b5be1163) )
	ROM_LOAD( "astro.13i", 0x02800, 0x0400, CRC(8d1575e0) SHA1(3d7f65ecf786704ebcd20cfaa2479ea24fd4e739) )
	ROM_LOAD( "astro.11l", 0x02c00, 0x0400, CRC(29f52f57) SHA1(5cb50b82e09c537eeaeae167351fca686fde8228) )
	ROM_LOAD( "astro.13l", 0x03000, 0x0400, CRC(882cdb87) SHA1(062ee8d296316cbce2eb62e72774aa4181e9847d) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "astro.1d",  0x00000, 0x0400, CRC(6053f834) SHA1(e0b76800c241b3c8010c09869cecbc109b25310a) )
	ROM_LOAD( "astro.3d",  0x00400, 0x0400, CRC(822505aa) SHA1(f9d3465e14bb850a286f8b4f42aa0a4044413b67) )
ROM_END

GAME( 1979, galaxia, 0, galaxia, galaxia, 0, ROT90, "Zaccaria", "Galaxia", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1980, astrowar, 0, astrowar, astrowar, 0, ROT90, "Zaccaria / Zelco", "Astro Wars (set 1)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
