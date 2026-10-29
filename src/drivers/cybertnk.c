// license:LGPL-2.1+
// copyright-holders:Angelo Salese, David Haywood
#include "driver.h"
#include "sound/3812intf.h"
static tilemap *tilemap0_tilemap,*tilemap1_tilemap,*tilemap2_tilemap;
static UINT16 *spr_ram,*tilemap0_vram,*tilemap1_vram,*tilemap2_vram;
static UINT16 *tilemap0scroll,*tilemap1scroll,*tilemap2scroll,*roadram,*sharedram;
static UINT8 mux_data;
static void get_tilemap0_tile_info(int tile_index)
{
	int code = tilemap0_vram[tile_index];
	int pal = (code & 0xe000) >> 13;
	pal     |=(code & 0x1c00) >> 7;

	SET_TILE_INFO(
			0,
			code & 0x1fff,
			pal,
			0);
}

static void get_tilemap1_tile_info(int tile_index)
{
	int code = tilemap1_vram[tile_index];
	int pal = (code & 0xe000) >> 13;
	pal     |=(code & 0x1c00) >> 7;

	SET_TILE_INFO(
			1,
			code & 0x1fff,
			pal,
			0);
}

static void get_tilemap2_tile_info(int tile_index)
{
	int code = tilemap2_vram[tile_index];
	int pal = (code & 0xe000) >> 13;
	pal     |=(code & 0x1c00) >> 7;

	SET_TILE_INFO(
			2,
			code & 0x1fff,
			pal,
			0);
}

static VIDEO_START( cybertnk )
{
	tilemap0_tilemap = tilemap_create(get_tilemap0_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,32);
	tilemap_set_transparent_pen(tilemap0_tilemap,0);

	tilemap1_tilemap = tilemap_create(get_tilemap1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,32);
	tilemap_set_transparent_pen(tilemap1_tilemap,0);

	tilemap2_tilemap = tilemap_create(get_tilemap2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,32);
	tilemap_set_transparent_pen(tilemap2_tilemap,0);
	return 0;
}

static void draw_road(mame_bitmap *bitmap, const rectangle *cliprect, int screen_shift, int pri)
{
	int i;
	const gfx_element *gfx = Machine->gfx[3];

	for (i=0;i<0x1000/4;i+=4)
	{
		UINT16 param1 = roadram[i+2];
		UINT16 param2 = roadram[i+1];
		UINT16 param3 = roadram[i+0];

		int col = (param2 & 0x3f);

		if ((param2&0x80) == pri)
		{
			drawgfx(bitmap,gfx,param1,col,0,0,-param3+screen_shift,i/4,cliprect,TRANSPARENCY_PEN,0);
		}

	}
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int screen_shift)
{
	const UINT32 *sprrom = (UINT32*)memory_region(REGION_USER1);
	const pen_t *paldata = Machine->pens;

	int miny = cliprect->min_y;
	int	maxy = cliprect->max_y;
	int minx = cliprect->min_x;
	int	maxx = cliprect->max_x;
	UINT16* dest;

	for(int offs=0;offs<0x1000/2;offs+=8)
	{
		if ((spr_ram[offs+0x0] & 8) == 0)
			continue;

		int x = (spr_ram[offs+0x5] & 0x3ff);
		if (x&0x200) x-=0x400;

		int y = (spr_ram[offs+0x2] & 0x1ff);
		if (y&0x100) y-=0x200;

		UINT32 spr_offs = (((spr_ram[offs+0x0] & 7) << 16) | (spr_ram[offs+0x1]));
		int xsize = ((spr_ram[offs+0x6] & 0x000f)+1) << 3;
		int ysize = (spr_ram[offs+0x4] & 0x00ff)+1;
		int fx = (spr_ram[offs+0x5] & 0x8000) >> 15;
		int zoom = (spr_ram[offs+0x6] & 0xff00) >> 8;

		int col_bank = (spr_ram[offs+0x0] & 0xff00) >> 8;

		int xf = 0;
		int yf = 0;
		int xz = 0;
		int yz = 0;

		for(int yi = 0;yi < ysize;yi++)
		{
			xf = xz = 0;

			int yy = y+yz;

			if ((yy>=miny) && (yy<=maxy))
			{
				dest = (UINT16*)bitmap->line[yy];

				int start,end,inc;

				if (!fx)
				{
					start = 0;
					end = xsize;
					inc = 8;
				}
				else
				{
					start = xsize-8;
					end = -8;
					inc = -8;
				}

				for(int xi=start;xi != end;xi+=inc)
				{
					UINT32 color;

					color = sprrom[(spr_offs+xi/8) & 0x7ffff];

					UINT16 dot;
					int x_dec;

					int shift_pen = 0;

					x_dec = 0;

					while(x_dec < 8)
					{
						if (!fx)
						{
							dot = (color >> shift_pen) & 0xf;
						}
						else
						{
							dot = (color >> (28-shift_pen)) & 0xf;
						}

						if (dot != 0)
						{

							int xx = (x+xz)+screen_shift;
							if ((xx>=minx) && (xx<=maxx))
							{

									dest[xx] = paldata[col_bank << 4 | dot];
							}

						}
						xf+=zoom;
						if(xf >= 0x100)
						{
							xz++;
							xf-=0x100;
						}
						else
						{
							shift_pen += 4;
							x_dec++;
							if(xf >= 0x80) { xz++; xf-=0x80; }
						}
					}
				}
			}

			yf+=zoom;
			if(yf >= 0x100)
			{
				yi--;
				yz++;
				yf-=0x100;
			}
			else
			{
				spr_offs += xsize/8;
				if(yf >= 0x80) { yz++; yf-=0x80; }
			}
		}
	}

}

static int update_screen(mame_bitmap *bitmap, const rectangle *cliprect, int screen_shift)
{

	tilemap_set_scrolldx(tilemap0_tilemap,screen_shift, screen_shift);
	tilemap_set_scrolldx(tilemap1_tilemap,screen_shift, screen_shift);
	tilemap_set_scrolldx(tilemap2_tilemap,screen_shift, screen_shift);

	tilemap_set_scrolly(tilemap1_tilemap,0,tilemap1scroll[2]);
	tilemap_set_scrolly(tilemap2_tilemap,0,tilemap2scroll[2]);

	tilemap_set_scrollx(tilemap1_tilemap,0,tilemap1scroll[0]);
	tilemap_set_scrollx(tilemap2_tilemap,0,tilemap2scroll[0]);

	fillbitmap(bitmap,get_black_pen(),cliprect);

	draw_road(bitmap,cliprect,screen_shift, 0x00);

	tilemap_draw(bitmap,cliprect,tilemap2_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,tilemap1_tilemap,0,0);

	draw_road(bitmap,cliprect,screen_shift, 0x80);

	draw_sprites(bitmap,cliprect,screen_shift);

	tilemap_draw(bitmap,cliprect,tilemap0_tilemap,0,0);

	return 0;
}

static VIDEO_UPDATE( cybertnk ) { update_screen(bitmap,cliprect,0); }

static WRITE16_HANDLER( tilemap0_vram_w )
{
	COMBINE_DATA(&tilemap0_vram[offset]);
	tilemap_mark_tile_dirty(tilemap0_tilemap,offset);
}

static WRITE16_HANDLER( tilemap1_vram_w )
{
	COMBINE_DATA(&tilemap1_vram[offset]);
	tilemap_mark_tile_dirty(tilemap1_tilemap,offset);
}

static WRITE16_HANDLER( tilemap2_vram_w )
{
	COMBINE_DATA(&tilemap2_vram[offset]);
	tilemap_mark_tile_dirty(tilemap2_tilemap,offset);
}

static WRITE16_HANDLER( sound_cmd_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpunum_set_input_line(2,0,HOLD_LINE);
	}
}

static WRITE16_HANDLER( mux_w )
{
	if (ACCESSING_LSB)
		mux_data=data & 0x60;
}

static READ16_HANDLER( mux_r )
{
	static const char *names[]={"TRAVERSE","ELEVATE","ACCEL","HANDLE"};
	return readinputportbytag(names[mux_data>>5]);
}

static READ16_HANDLER( dsw1_r ) { return readinputportbytag("DSW1"); }
static READ16_HANDLER( dsw2_r ) { return readinputportbytag("DSW2"); }
static READ16_HANDLER( in0_r ) { return readinputportbytag("IN0"); }
static READ16_HANDLER( in1_r ) { return readinputportbytag("IN1"); }
static READ16_HANDLER( shared_r ) { return sharedram[offset]; }
static WRITE16_HANDLER( shared_w ) { COMBINE_DATA(&sharedram[offset]); }

static ADDRESS_MAP_START(master_mem,ADDRESS_SPACE_PROGRAM,16)
	AM_RANGE(0x000000,0x03ffff) AM_ROM
	AM_RANGE(0x080000,0x087fff) AM_RAM
	AM_RANGE(0x0a0000,0x0a0fff) AM_RAM AM_BASE(&spr_ram)
	AM_RANGE(0x0c0000,0x0c1fff) AM_READWRITE(MRA16_RAM,tilemap0_vram_w) AM_BASE(&tilemap0_vram)
	AM_RANGE(0x0c4000,0x0c5fff) AM_READWRITE(MRA16_RAM,tilemap1_vram_w) AM_BASE(&tilemap1_vram)
	AM_RANGE(0x0c8000,0x0c9fff) AM_READWRITE(MRA16_RAM,tilemap2_vram_w) AM_BASE(&tilemap2_vram)
	AM_RANGE(0x0e0000,0x0e0fff) AM_RAM AM_BASE(&sharedram)
	AM_RANGE(0x100000,0x107fff) AM_READWRITE(MRA16_RAM,paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x110000,0x110001) AM_WRITE(sound_cmd_w)
	AM_RANGE(0x110002,0x110003) AM_READ(dsw1_r) AM_WRITENOP
	AM_RANGE(0x110004,0x110005) AM_READ(MRA16_NOP)
	AM_RANGE(0x110006,0x110007) AM_READWRITE(in0_r,mux_w)
	AM_RANGE(0x110008,0x110009) AM_READ(in1_r) AM_WRITENOP
	AM_RANGE(0x11000a,0x11000b) AM_READ(dsw2_r)
	AM_RANGE(0x11000c,0x11000d) AM_WRITENOP
	AM_RANGE(0x110040,0x110045) AM_RAM AM_BASE(&tilemap0scroll)
	AM_RANGE(0x110048,0x11004d) AM_RAM AM_BASE(&tilemap1scroll)
	AM_RANGE(0x110080,0x110085) AM_RAM AM_BASE(&tilemap2scroll)
	AM_RANGE(0x1100d4,0x1100d5) AM_READ(mux_r)
ADDRESS_MAP_END
static ADDRESS_MAP_START(slave_mem,ADDRESS_SPACE_PROGRAM,16)
	AM_RANGE(0x000000,0x01ffff) AM_ROM
	AM_RANGE(0x020000,0x020001) AM_READ(MRA16_NOP)
	AM_RANGE(0x07fff8,0x07fffd) AM_READ(MRA16_NOP)
	AM_RANGE(0x080000,0x083fff) AM_RAM
	AM_RANGE(0x0c0000,0x0c0fff) AM_RAM AM_BASE(&roadram)
	AM_RANGE(0x100000,0x100fff) AM_READWRITE(shared_r,shared_w)
	AM_RANGE(0x140000,0x140003) AM_NOP
ADDRESS_MAP_END
static ADDRESS_MAP_START(sound_mem,ADDRESS_SPACE_PROGRAM,8)
	AM_RANGE(0x0000,0x7fff) AM_ROM
	AM_RANGE(0x8000,0x9fff) AM_RAM
	AM_RANGE(0xa000,0xa000) AM_READWRITE(Y8950_status_port_0_r,Y8950_control_port_0_w)
	AM_RANGE(0xa001,0xa001) AM_READWRITE(soundlatch_r,Y8950_write_port_0_w)
	AM_RANGE(0xa005,0xa006) AM_NOP
	AM_RANGE(0xc000,0xc000) AM_READWRITE(Y8950_status_port_1_r,Y8950_control_port_1_w)
	AM_RANGE(0xc001,0xc001) AM_READWRITE(Y8950_read_port_1_r,Y8950_write_port_1_w)
ADDRESS_MAP_END
static INPUT_PORTS_START( cybertnk )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Machine Gun 1 (Bomb)")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("P1 Cannon 1 (Bomb)")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x0800, IP_ACTIVE_LOW )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Machine Gun 2 (Fire)")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Cannon 2 (Fire)")
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("TRAVERSE")
	PORT_BIT( 0xff, 0x00, IPT_AD_STICK_X) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_PLAYER(2) PORT_REVERSE PORT_NAME("P2 Machine Gun X")

	PORT_START_TAG("ELEVATE")
	PORT_BIT( 0xff, 0x00, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_PLAYER(2) PORT_REVERSE PORT_NAME("P2 Machine Gun Y")

	PORT_START_TAG("ACCEL")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_PLAYER(1) PORT_NAME("P1 Accelerate")

	PORT_START_TAG("HANDLE")
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(100) PORT_KEYDELTA(4) PORT_PLAYER(1) PORT_REVERSE PORT_NAME("P1 Handle")

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x0004, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Very_Easy ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Very_Hard ) )

	PORT_DIPNAME( 0x0010, 0x0000, "Coin B Value" )
	PORT_DIPSETTING(      0x0010, "Set by Dipswitches" )
	PORT_DIPSETTING(      0x0000, "Same Value as Coin A" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE(0x0040, IP_ACTIVE_LOW)
	PORT_DIPNAME( 0x0080, 0x0080, "2 Credits to Start" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0f00, 0x0f00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0f00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0e00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x0d00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0b00, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0a00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0900, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf000, 0xf000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x5000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0xf000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x7000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0xd000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0xb000, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x9000, DEF_STR( 1C_7C ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BIT(	  0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static const gfx_layout tile_8x8x4 =
{
	8,8,
	RGN_FRAC(1,4),
    4,
    { RGN_FRAC(3,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(0,4) },
    { STEP8(0,1) },
    { STEP8(0,8) },
    8*8
};

static const UINT32 xoffsets[] =
{
	STEP16(0,4), STEP16(64,4), STEP16(128,4), STEP16(192,4),
	STEP16(256,4), STEP16(320,4), STEP16(384,4), STEP16(448,4),
	STEP16(512,4), STEP16(576,4), STEP16(640,4), STEP16(704,4),
	STEP16(768,4), STEP16(832,4), STEP16(896,4), STEP16(960,4),
	STEP16(1024,4), STEP16(1088,4), STEP16(1152,4), STEP16(1216,4),
	STEP16(1280,4), STEP16(1344,4), STEP16(1408,4), STEP16(1472,4),
	STEP16(1536,4), STEP16(1600,4), STEP16(1664,4), STEP16(1728,4),
	STEP16(1792,4), STEP16(1856,4), STEP16(1920,4), STEP16(1984,4),
	STEP16(2048,4), STEP16(2112,4), STEP16(2176,4), STEP16(2240,4),
	STEP16(2304,4), STEP16(2368,4), STEP16(2432,4), STEP16(2496,4),
	STEP16(2560,4), STEP16(2624,4), STEP16(2688,4), STEP16(2752,4),
	STEP16(2816,4), STEP16(2880,4), STEP16(2944,4), STEP16(3008,4),
	STEP16(3072,4), STEP16(3136,4), STEP16(3200,4), STEP16(3264,4),
	STEP16(3328,4), STEP16(3392,4), STEP16(3456,4), STEP16(3520,4),
	STEP16(3584,4), STEP16(3648,4), STEP16(3712,4), STEP16(3776,4),
	STEP16(3840,4), STEP16(3904,4), STEP16(3968,4), STEP16(4032,4),
};
static const gfx_layout roadlayout =
{
	1024,1,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	EXTENDED_XOFFS,
	{ 0 },
	1024*4,
	xoffsets
};

static const gfx_decode gfxdecodeinfo[] = {
 { REGION_GFX1,0,&tile_8x8x4,0x1400,64 },
 { REGION_GFX2,0,&tile_8x8x4,0x1800,64 },
 { REGION_GFX3,0,&tile_8x8x4,0x1c00,64 },
 { REGION_GFX4,0,&roadlayout,0x1000,64 },
 { -1 }
};
static struct Y8950interface y8950_interface1={0,REGION_SOUND1};
static struct Y8950interface y8950_interface2={0,REGION_SOUND2};
static MACHINE_DRIVER_START( cybertnk )
	MDRV_CPU_ADD(M68000,20000000/2)
	MDRV_CPU_PROGRAM_MAP(master_mem,0)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(M68000,20000000/2)
	MDRV_CPU_PROGRAM_MAP(slave_mem,0)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1)

	MDRV_CPU_ADD(Z80,3579500)
	MDRV_CPU_PROGRAM_MAP(sound_mem,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_INTERLEAVE(1000)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x4000)

	MDRV_VIDEO_START(cybertnk)
	MDRV_VIDEO_UPDATE(cybertnk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(Y8950, 3579500)
	MDRV_SOUND_CONFIG(y8950_interface1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)

	MDRV_SOUND_ADD(Y8950, 3579500)
	MDRV_SOUND_CONFIG(y8950_interface2)

	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( cybertnk )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "p1a.37",   0x00000, 0x20000, CRC(be1abd16) SHA1(6ad01516301b44899971000c36f7e21070c3d2da) )
	ROM_LOAD16_BYTE( "p2a.36",   0x00001, 0x20000, CRC(5290c89a) SHA1(5a11671505214c20770e2938dab1ee82a030b457) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "subl",   0x00000, 0x10000, CRC(3814a2eb) SHA1(252800b21f5cfada34ef5208cda33088daab132b) )
	ROM_LOAD16_BYTE( "subh",   0x00001, 0x10000, CRC(1af7ad58) SHA1(450c65289729d74cd4d17e11be16469246e61b7d) )

	ROM_REGION( 0x8000, REGION_CPU3, 0 )
	ROM_LOAD( "ss5.37",    0x0000, 0x8000, CRC(c3ba160b) SHA1(cfbfcad443ff83cd4e707f045a650417aca03d85) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_ERASEFF )
	ROM_LOAD( "ss1.10",    0x00000, 0x20000, CRC(27d1cf94) SHA1(26246f217192bcfa39692df6d388640d385e9ed9) )
	ROM_LOAD( "ss3.11",    0x20000, 0x20000, CRC(a327488e) SHA1(b55357101e392f50f0cf75cf496a3ff4b79b2633) )

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_ERASEFF )
	ROM_LOAD( "ss2.31",    0x00000, 0x20000, CRC(27d1cf94) SHA1(26246f217192bcfa39692df6d388640d385e9ed9) )
	ROM_LOAD( "ss4.32",    0x20000, 0x20000, CRC(a327488e) SHA1(b55357101e392f50f0cf75cf496a3ff4b79b2633) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_LOAD( "s09", 0x00000, 0x10000, CRC(69e6470c) SHA1(8e7db6988366cae714fff72449623a7977af1db1) )
	ROM_LOAD( "s10", 0x10000, 0x10000, CRC(77230f44) SHA1(b79fc841fa784d23855e4085310cee435c11348f) )
	ROM_LOAD( "s11", 0x20000, 0x10000, CRC(bfda980d) SHA1(1f975fdd2cfdc345eeb03fbc26fc1be1b2d7737e) )
	ROM_LOAD( "s12", 0x30000, 0x10000, CRC(8a11fcfa) SHA1(a406ac9cf841dd9d829cb83bfe8feb5128a3e77e) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 )
	ROM_LOAD( "s01", 0x00000, 0x10000, CRC(6513452c) SHA1(95ed2da8f90e16c50716011577606a7dc93ba65e) )
	ROM_LOAD( "s02", 0x10000, 0x10000, CRC(3a270e3b) SHA1(97c8282d4d782c9d2fcfb5e5dabbe1ca88978f5c) )
	ROM_LOAD( "s03", 0x20000, 0x10000, CRC(584eff66) SHA1(308ec058693ce3ce34b058a8dbeedf342134311c) )
	ROM_LOAD( "s04", 0x30000, 0x10000, CRC(51ba5402) SHA1(c4522c4562ce0514bef3257e323bcc255b635544) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "s05", 0x00000, 0x10000, CRC(bddb6008) SHA1(bacb822bac4893eee0648a19ce449e5559d32b5e) )
	ROM_LOAD( "s06", 0x10000, 0x10000, CRC(d65b0fa5) SHA1(ce398a52ad408778fd910c42a9618194b862becf) )
	ROM_LOAD( "s07", 0x20000, 0x10000, CRC(70220567) SHA1(44b48ded8581a6d78b27a3af833f62413ff31c76) )
	ROM_LOAD( "s08", 0x30000, 0x10000, CRC(988c4fcb) SHA1(68d32be70605ad5415f2b6aeabbd92e269f0c9af) )

	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "c01.93" , 0x180001, 0x20000, CRC(b5ee3de2) SHA1(77b9a2818f36826891e510e8550f1025bacfa496) )
	ROM_LOAD32_BYTE( "c02.92" , 0x180000, 0x20000, CRC(1f857d79) SHA1(f410d50970c10814b80baab27cbe69965bf0ccc0) )
	ROM_LOAD32_BYTE( "c03.91" , 0x180003, 0x20000, CRC(d70a93e2) SHA1(e64bb10c58b27def4882f3006784be56de11b812) )
	ROM_LOAD32_BYTE( "c04.90" , 0x180002, 0x20000, CRC(04d6fdc2) SHA1(56f8091c1a010014e951f5f47084e1400006123e) )

	ROM_LOAD32_BYTE( "c05.102", 0x100001, 0x20000, CRC(3f537490) SHA1(12d6545d29dda9f88019040fa33c73a22a2a213b) )
	ROM_LOAD32_BYTE( "c06.101", 0x100000, 0x20000, CRC(ff69c6a4) SHA1(badd20d26ba771780aebf733e1fbd1d37aa66f9b) )
	ROM_LOAD32_BYTE( "c07.100", 0x100003, 0x20000, CRC(5e8eba75) SHA1(6d0c1916517802acf808c8edc8e0b6074bdc90be) )
	ROM_LOAD32_BYTE( "c08.98" , 0x100002, 0x20000, CRC(f0820ddd) SHA1(7fb6c7d66ff96148f14921bc8d0cc0c65ffce4c4) )

	ROM_LOAD32_BYTE( "c09.109", 0x080001, 0x20000, CRC(080f87c3) SHA1(aedebc22ff03d4cc710e71ca14e09c7808f59c72) )
	ROM_LOAD32_BYTE( "c10.108", 0x080000, 0x20000, CRC(777c6a62) SHA1(4684d1c5d88b37ecb20002b7aa4814bf566e7d4b) )
	ROM_LOAD32_BYTE( "c11.107", 0x080003, 0x20000, CRC(330ca5a1) SHA1(4409da231a5abcec8c7d2d66eefdfd2019a322db) )
	ROM_LOAD32_BYTE( "c12.106", 0x080002, 0x20000, CRC(c1ec8e61) SHA1(09f2f4ddc100e5675c9bd82c200718fb0b69655e) )

	ROM_LOAD32_BYTE( "c13.119", 0x000001, 0x20000, CRC(4e22a7e0) SHA1(69cc7dd528b8af0c28b448285768a3ed079099ba) )
	ROM_LOAD32_BYTE( "c14.118", 0x000000, 0x20000, CRC(bdbd6232) SHA1(94b0741d5eced558723dda32a89aa2b747cdcbbd) )
	ROM_LOAD32_BYTE( "c15.117", 0x000003, 0x20000, CRC(f163d768) SHA1(e54e31a6f956f7de52b59bcdd0cd4ac1662b5664) )
	ROM_LOAD32_BYTE( "c16.116", 0x000002, 0x20000, CRC(5e5017c4) SHA1(586cd729630f00cbaf10d1036edebed1672bc532) )

	ROM_REGION( 0x40000, REGION_GFX4, 0 )
	ROM_LOAD16_BYTE( "road_chl" , 0x000001, 0x20000, CRC(862b109c) SHA1(9f81918362218ddc0a6bf0a5317c5150e514b699) )
	ROM_LOAD16_BYTE( "road_chh" , 0x000000, 0x20000, CRC(9dedc988) SHA1(10bae1be0e35320872d4994f7e882cd1de988c90) )

	ROM_REGION( 0x30000, REGION_USER3, 0 )
	ROM_LOAD( "t1",   0x00000, 0x08000, CRC(24890512) SHA1(2a6c9d39ca0c1c8316e85d9f565f6b3922d596b2) )
	ROM_LOAD( "t2",   0x08000, 0x08000, CRC(5a10480d) SHA1(f17598442091dae14abe3505957d94793f3ed886) )
	ROM_LOAD( "t3",   0x10000, 0x08000, CRC(454af4dc) SHA1(e5b18a37715e50db2243432564f5a04fb39dea60) )
	ROM_LOAD( "t4",   0x18000, 0x08000, CRC(0e1ef6a9) SHA1(d230841bbee6d07bab05aa8d37ec2409fc6278bc) )

	ROM_REGION( 0x10000, REGION_USER4, 0 )

	ROM_LOAD( "t5",   0x00000, 0x08000, CRC(12eb51bc) SHA1(35708eb456207ebee498c70dd82340b364797c56) )
	ROM_LOAD( "t6",   0x08000, 0x08000, CRC(12eb51bc) SHA1(35708eb456207ebee498c70dd82340b364797c56) )

	ROM_REGION( 0x280, REGION_PROMS, 0 )
	ROM_LOAD( "ic2",  0x0000, 0x0100, CRC(aad2a447) SHA1(a12923027e3093bd6d358af44d35d2e8e588dd1a) )
	ROM_LOAD( "ic15", 0x0100, 0x0100, CRC(5f8c2c00) SHA1(50162503ac0ee9395377d7e45a84672a9493fb7d) )
	ROM_LOAD( "ic19", 0x0200, 0x0020, CRC(bd15cd71) SHA1(e0946d12eebd5db8707d965be157914d70f7472b) )
	ROM_LOAD( "ic20", 0x0220, 0x0020, CRC(2f237563) SHA1(b0081c1cc6e357a6f10ab1ff357bd4e989ec7fb3) )
	ROM_LOAD( "ic29", 0x0240, 0x0020, CRC(95b32c0f) SHA1(5a19f441ced983bacbf3bc1aaee94ca768166447) )
	ROM_LOAD( "ic30", 0x0260, 0x0020, CRC(2bb6033f) SHA1(eb994108734d7d04f8e293eca21bb3051a63cfe9) )
ROM_END

DRIVER_INIT( cybertnk )
{
	UINT32 *spr = (UINT32*)memory_region(REGION_USER1);

	for (int x = 0; x< 0x200000/4;x++)
	{

		spr[x] = BITSWAP32(spr[x],  27,26,25,24,   19,18,17,16,  11,10,9,8,  3,2,1,0, 31,30,29,28,   23,22,21,20,   15,14,13,12,   7,6,5,4 );
	}

}

GAME(1988,cybertnk,0,cybertnk,cybertnk,cybertnk,ROT0,"Coreland","Cyber Tank (v1.4)",GAME_IMPERFECT_GRAPHICS)
