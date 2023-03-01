/***************************************************************************************
 
Heavy Unit
Kaneko/Taito 1988
 
Driver based on djboy.c / airbustr.c
 
This game runs on Kaneko hardware. The game is similar to R-Type.
 
PCB Layout
----------
M6100391A
M6100392A  880210204
KNA-001
|----------------------------------------------------|
|                                                    |
|           6116                          6116       |
|      15Mhz                              6116       |
|                                 PAL                |
|           B73_09.2P             B73_11.5P          |
|                                                    |
|                                                    |
|                                 Z80-1   DSW1 DSW2 J|
|                                                   A|
|     16MHz                                         M|
|                                                   M|
|       12MHz                       6264  MERMAID   A|
| B73_05.1H                                          |
| B73_04.1F B73_08.2F  6116              Z80-2       |
| B73_03.1D                       Z80-3  B73_12.7E   |
| B73_02.1C B73_07.2C  PANDORA    B73_10.5C  6116    |
| B73_01.1B B73_06.2B 4164 4164   6264 PAL  YM3014   |
|                     4164 4164   PAL       YM2203   |
|----------------------------------------------------|
 
Notes:
      Z80-1 clock  : 6.000MHz
      Z80-2 clock  : 6.000MHz
      Z80-3 clock  : 6.000MHz
      YM2203 clock : 3.000MHz
      VSync        : 58Hz
      HSync        : 15.59kHz
               \-\ : KANEKO 1988. DIP40 8751 MCU
      MERMAID    | : pin 18,19 = 6.000MHz (main clock)
                 | : pin 30 = 1.000MHz (prog/ale)
               /-/ : pin 22 = 111.48Hz (port 2 bit 1)
 
      PANDORA      : KANEKO PX79480FP-3 PANDORA-CHIP (C) KANEKO 1988
 
 
***************************************************************************************/

#include "driver.h"
#include "sound/2203intf.h"
#include "cpu/z80/z80.h"
#include "cpu/i8051/i8051.h"


	/* Video */
static tilemap *bg_tilemap;
UINT16 hvyunit_scrollx, hvyunit_scrolly, port0_data;
static int flipscreen;
UINT8 *hvyunit_videoram, *hvyunit_colorram;
WRITE8_HANDLER( hvyunit_scrollx_w );
WRITE8_HANDLER( hvyunit_scrolly_w );
WRITE8_HANDLER( hvyunit_videoram_w );
WRITE8_HANDLER( hvyunit_colorram_w );
WRITE8_HANDLER ( pandora_spriteram_w );
READ8_HANDLER( pandora_spriteram_r );
VIDEO_START( hvyunit );
VIDEO_UPDATE( hvyunit );
VIDEO_EOF( hvyunit );


// pandora draw
void pandora_start(UINT8 region, int x, int y);
void pandora_update(mame_bitmap *bitmap, const rectangle *cliprect);
void pandora_eof();
void pandora_set_clear_bitmap(int clear);
void pandora_set_bg_pen( int pen );
UINT8* pandora_spriteram;
UINT8 pandora_region;
static mame_bitmap *pandora_sprites_bitmap; /* bitmap to render sprites to, Pandora seems to be frame'buffered' */
int	pandora_bg_pen;
int pandora_clear_bitmap;
int pandora_xoffset, pandora_yoffset;


	/* Mermaid */
static UINT8 data_to_mermaid;
static UINT8 data_to_z80;
static UINT8 mermaid_to_z80_full;
static UINT8 z80_to_mermaid_full;
static UINT8 mermaid_int0_l;
static UINT8 mermaid_p[4];

static UINT8 *sharedram;

static READ8_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE8_HANDLER( sharedram_w )
{
	sharedram[offset] = data;
}


/*************************************
 *
 *  Initialisation
 *
 *************************************/

static MACHINE_RESET( hvyunit )
{
	mermaid_int0_l = 1;
	mermaid_to_z80_full = 0;
	z80_to_mermaid_full = 0;

}

/*************************************
 *
 *  Video hardware
 *
 *************************************/


void pandora_set_bg_pen( int pen )
{
	pandora_bg_pen = pen;
}

void pandora_set_clear_bitmap(int clear)
{
	pandora_clear_bitmap = clear;
}

void pandora_update(mame_bitmap *bitmap, const rectangle *cliprect)
{
	if (!pandora_sprites_bitmap)
	{
		printf("ERROR: pandora_update with no pandora_sprites_bitmap\n");
		return;
	}

	copybitmap(bitmap,pandora_sprites_bitmap,0,0,0,0,cliprect,TRANSPARENCY_PEN,0);
}


void pandora_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{

	int sx=0, sy=0, x=0, y=0, offs;


	/*
     * Sprite Tile Format
     * ------------------
     *
     * Byte | Bit(s)   | Use
     * -----+-76543210-+----------------
     *  0-2 | -------- | unused
     *  3   | xxxx.... | Palette Bank
     *  3   | .......x | XPos - Sign Bit
     *  3   | ......x. | YPos - Sign Bit
     *  3   | .....x.. | Use Relative offsets
     *  4   | xxxxxxxx | XPos
     *  5   | xxxxxxxx | YPos
     *  6   | xxxxxxxx | Sprite Number (low 8 bits)
     *  7   | ....xxxx | Sprite Number (high 4 bits)
     *  7   | x....... | Flip Sprite Y-Axis
     *  7   | .x...... | Flip Sprite X-Axis
     */

	for (offs = 0;offs < 0x1000;offs += 8)
	{
		int dx = pandora_spriteram[offs+4];
		int dy = pandora_spriteram[offs+5];
		int tilecolour = pandora_spriteram[offs+3];
		int attr = pandora_spriteram[offs+7];
		int flipx =   attr & 0x80;
		int flipy =  (attr & 0x40) << 1;
		int tile  = ((attr & 0x3f) << 8) + (pandora_spriteram[offs+6] & 0xff);

		if (tilecolour & 1) dx |= 0x100;
		if (tilecolour & 2) dy |= 0x100;

		if (tilecolour & 4)
		{
			x += dx;
			y += dy;
		}
		else
		{
			x = dx;
			y = dy;
		}

		if (flipscreen)
		{
			sx = 240 - x;
			sy = 240 - y;
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sx = x;
			sy = y;
		}

		/* global offset */
		sx+=pandora_xoffset;
		sy+=pandora_yoffset;

		sx &=0x1ff;
		sy &=0x1ff;

		if (sx&0x100) sx-=0x200;
		if (sy&0x100) sy-=0x200;

		drawgfx(bitmap,Machine->gfx[pandora_region],
				tile,
				(tilecolour & 0xf0) >> 4,
				flipx, flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

void pandora_eof()
{
	rectangle clip;

	/* draw top of screen */
	clip.min_x = Machine->visible_area.min_x;
	clip.max_x = Machine->visible_area.max_x;
	clip.min_y = Machine->visible_area.min_y;
	clip.max_y = Machine->visible_area.max_y;

	if (!pandora_spriteram)
	{
		printf("ERROR: pandora_eof with no pandora_spriteram\n");
		return;
	}

	// the games can disable the clearing of the sprite bitmap, to leave sprite trails
	if (pandora_clear_bitmap) fillbitmap(pandora_sprites_bitmap,pandora_bg_pen,&clip);

	pandora_draw(pandora_sprites_bitmap, &clip);
}

void pandora_start(UINT8 region, int x, int y)
{
	pandora_region = region;
	pandora_xoffset = x;
	pandora_yoffset = y;
	pandora_bg_pen = 0;
	pandora_spriteram = auto_malloc(0x1000);
	memset(pandora_spriteram,0x00, 0x1000);

	pandora_sprites_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	pandora_clear_bitmap = 0;
}


WRITE8_HANDLER ( pandora_spriteram_w )
{
	// it's either hooked up oddly on this, or on the 16-bit games
	// either way, we swap the address lines so that the spriteram is in the same format
	offset = BITSWAP16(offset,  15,14,13,12, 11,   7,6,5,4,3,2,1,0,   10,9,8  );

	if (!pandora_spriteram)
	{
		printf("ERROR: pandora_spriteram_w with no pandora_spriteram\n");
		return;
	}

	if (offset>=0x1000)
	{
		logerror("pandora_spriteram_w write past spriteram, offset %04x %02x\n",offset,data);
		return;
	}
	pandora_spriteram[offset] = data;
}

READ8_HANDLER( pandora_spriteram_r )
{
	// it's either hooked up oddly on this, or ont the 16-bit games
	// either way, we swap the address lines so that the spriteram is in the same format
	offset = BITSWAP16(offset,  15,14,13,12, 11,  7,6,5,4,3,2,1,0,  10,9,8  );

	if (!pandora_spriteram)
	{
		printf("ERROR: pandora_spriteram_r with no pandora_spriteram\n");
		return 0x00;
	}

	if (offset>=0x1000)
	{
		logerror("pandora_spriteram_r read past spriteram, offset %04x\n",offset );
		return 0x00;
	}
	return pandora_spriteram[offset];
}


static void get_bg_tile_info(int tile_index)
{
	int attr = hvyunit_colorram[tile_index];
	int code = hvyunit_videoram[tile_index] + ((attr & 0x0f) << 8);
	int color = (attr >> 4);

	SET_TILE_INFO(1, code, color, 0);
}

VIDEO_START( hvyunit )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16, 16, 32, 32);

	if(!bg_tilemap)
		return 1;

    pandora_start(0,0,0);

	return 0;

}

VIDEO_UPDATE( hvyunit )
{
#define SX_POS	96
#define SY_POS	0

	tilemap_set_scrollx(bg_tilemap, 0, ((port0_data & 0x40) << 2) + hvyunit_scrollx + SX_POS); // TODO
	tilemap_set_scrolly(bg_tilemap, 0, ((port0_data & 0x80) << 1) + hvyunit_scrolly + SY_POS); // TODO
	fillbitmap(bitmap,get_black_pen(),cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	pandora_draw( bitmap, cliprect );

    flipscreen = 0;
}

VIDEO_EOF( hvyunit )
{
    pandora_eof();
}


/*************************************
 *
 *  Master CPU handlers
 *
 *************************************/

static WRITE8_HANDLER( trigger_nmi_on_slave_cpu )
{
	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}


static WRITE8_HANDLER( master_bankswitch_w )
{
    int bank = data & 7;
	memory_set_bankptr( 1, memory_region(REGION_CPU1) + (bank * 0x4000) );
}



static WRITE8_HANDLER( mermaid_data_w )
{
	data_to_mermaid = data;
	z80_to_mermaid_full = 1;
	mermaid_int0_l = 0;
	cpunum_set_input_line(3, INPUT_LINE_IRQ0, ASSERT_LINE);
}

static READ8_HANDLER( mermaid_data_r )
{
	mermaid_to_z80_full = 0;
	return data_to_z80;
}

static READ8_HANDLER( mermaid_status_r )
{
	return (!mermaid_to_z80_full << 2) | (z80_to_mermaid_full << 3);
}


/*************************************
 *
 *  Slave CPU handlers
 *
 *************************************/

static WRITE8_HANDLER( trigger_nmi_on_sound_cpu2 )
{
	soundlatch_w(0, data);
	cpunum_set_input_line(2, INPUT_LINE_NMI, PULSE_LINE);
}

WRITE8_HANDLER( hvyunit_videoram_w )
{
	if( hvyunit_videoram[offset] != data)
	{
		hvyunit_videoram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap, offset);
	}
}

WRITE8_HANDLER( hvyunit_colorram_w )
{
	if( hvyunit_colorram[offset] != data)
	{
		hvyunit_colorram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap, offset);
	}
}

static WRITE8_HANDLER( slave_bankswitch_w )
{
	int bank = data & 0x03;
	port0_data = data;
	memory_set_bankptr( 2, memory_region(REGION_CPU2) + (bank * 0x4000) );
}

WRITE8_HANDLER( hvyunit_scrollx_w)
{
	hvyunit_scrollx=data;
}

WRITE8_HANDLER( hvyunit_scrolly_w)
{
	hvyunit_scrolly=data;
}


static WRITE8_HANDLER( coin_count_w )
{
	coin_counter_w(0, data & 1);
	coin_counter_w(1, data & 2);
}


/*************************************
 *
 *  Sound CPU handlers
 *
 *************************************/

static WRITE8_HANDLER( sound_bankswitch_w )
{
	int bank = data & 0x03;
	memory_set_bankptr( 3, memory_region(REGION_CPU3) + (bank * 0x4000) );
}


/*************************************
 *
 *  Protection MCU handlers
 *
 *************************************/

static READ8_HANDLER( mermaid_p0_r )
{
	// ?
	return 0;
}

static WRITE8_HANDLER( mermaid_p0_w )
{
	if (!BIT(mermaid_p[0], 1) && BIT(data, 1))
	{
		mermaid_to_z80_full = 1;
		data_to_z80 = mermaid_p[1];
	}

	if (BIT(data, 0) == 1)
		z80_to_mermaid_full = 0;

	mermaid_p[0] = data;
}

static READ8_HANDLER( mermaid_p1_r )
{
	if (BIT(mermaid_p[0], 0) == 0)
		return data_to_mermaid;
	else
		return 0; // ?
}

static WRITE8_HANDLER( mermaid_p1_w )
{
	if (data == 0xff)
	{
		mermaid_int0_l = 1;
		cpunum_set_input_line(3, INPUT_LINE_IRQ0, CLEAR_LINE);
	}

	mermaid_p[1] = data;
}

static READ8_HANDLER( mermaid_p2_r )
{

	switch ((mermaid_p[0] >> 2) & 3)
	{
        case 0: return readinputportbytag("IN1");
		case 1: return readinputportbytag("IN2");
		case 2: return readinputportbytag("IN0");
		default: return 0xff;
	}
}

static WRITE8_HANDLER( mermaid_p2_w )
{
	mermaid_p[2] = data;
}

static READ8_HANDLER( mermaid_p3_r )
{
	UINT8 dsw = 0;
    UINT8 dsw1 = readinputportbytag("DSW1");
	UINT8 dsw2 = readinputportbytag("DSW2");

	switch ((mermaid_p[0] >> 5) & 3)
	{
		case 0: dsw = (BIT(dsw2, 4) << 3) | (BIT(dsw2, 0) << 2) | (BIT(dsw1, 4) << 1) | BIT(dsw1, 0); break;
		case 1: dsw = (BIT(dsw2, 5) << 3) | (BIT(dsw2, 1) << 2) | (BIT(dsw1, 5) << 1) | BIT(dsw1, 1); break;
		case 2: dsw = (BIT(dsw2, 6) << 3) | (BIT(dsw2, 2) << 2) | (BIT(dsw1, 6) << 1) | BIT(dsw1, 2); break;
		case 3: dsw = (BIT(dsw2, 7) << 3) | (BIT(dsw2, 3) << 2) | (BIT(dsw1, 7) << 1) | BIT(dsw1, 3); break;
	}

	return (dsw << 4) | (mermaid_int0_l << 2) | (mermaid_to_z80_full << 3);
}

static WRITE8_HANDLER( mermaid_p3_w )
{
	mermaid_p[3] = data;
	cpunum_set_input_line(1, INPUT_LINE_RESET, data & 2 ? CLEAR_LINE : ASSERT_LINE);
}

char master_ram[0xfff+1];

static READ8_HANDLER( master_ram_r )
{
	return master_ram[offset];
}

static WRITE8_HANDLER( master_ram_w )
{
	master_ram[offset] = data;
}

char sl_ram[0xd7ff - 0xd200+1];

static READ8_HANDLER( sl_ram_r )
{
	return sl_ram[offset];
}

static WRITE8_HANDLER( sl_ram_w )
{
	sl_ram[offset] = data;
}


char sl_ram2[0xdfff - 0xda00+1];


static READ8_HANDLER( sl_ram2_r )
{
	return sl_ram2[offset];
}

static WRITE8_HANDLER( sl_ram2_w )
{
	sl_ram2[offset] = data;
}

char snd_ram[0x7ff+1];


static READ8_HANDLER( snd_ram_r )
{
	return snd_ram[offset];
}

static WRITE8_HANDLER( snd_ram_w )
{
	snd_ram[offset] = data;
}

/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( master_memory, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc000, 0xcfff) AM_READWRITE(pandora_spriteram_r, pandora_spriteram_w)
	AM_RANGE(0xd000, 0xdfff) AM_READWRITE(master_ram_r, master_ram_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(sharedram_r, sharedram_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( master_io, ADDRESS_SPACE_IO, 8 )
    ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
    AM_RANGE(0x00, 0x00) AM_WRITE(master_bankswitch_w)
    AM_RANGE(0x01, 0x01) AM_WRITE(master_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(trigger_nmi_on_slave_cpu)
ADDRESS_MAP_END



static ADDRESS_MAP_START( slave_memory, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xc3ff) AM_RAM AM_WRITE(hvyunit_videoram_w) AM_BASE(&hvyunit_videoram)
	AM_RANGE(0xc400, 0xc7ff) AM_RAM AM_WRITE(hvyunit_colorram_w) AM_BASE(&hvyunit_colorram)
	AM_RANGE(0xd000, 0xd1ff) AM_RAM AM_WRITE(paletteram_xxxxRRRRGGGGBBBB_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0xd200, 0xd7ff) AM_READWRITE(sl_ram_r, sl_ram_w)
	AM_RANGE(0xd800, 0xd9ff) AM_RAM AM_WRITE(paletteram_xxxxRRRRGGGGBBBB_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0xda00, 0xdfff) AM_READWRITE(sl_ram2_r, sl_ram2_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(sharedram_r, sharedram_w) AM_BASE(&sharedram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( slave_io, ADDRESS_SPACE_IO, 8 )
    ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(slave_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(trigger_nmi_on_sound_cpu2)
	AM_RANGE(0x04, 0x04) AM_READWRITE(mermaid_data_r, mermaid_data_w)
	AM_RANGE(0x06, 0x06) AM_WRITE(hvyunit_scrolly_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(hvyunit_scrollx_w)
	AM_RANGE(0x0c, 0x0c) AM_READ(mermaid_status_r)
	AM_RANGE(0x0e, 0x0e) AM_WRITE(coin_count_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_memory, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK3)
	AM_RANGE(0xc000, 0xc7ff) AM_READWRITE(snd_ram_r, snd_ram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io, ADDRESS_SPACE_IO, 8 )
    ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITE(sound_bankswitch_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0x04, 0x04) AM_READ(soundlatch_r)
ADDRESS_MAP_END


/* I8051 memory handlers */
static ADDRESS_MAP_START( mcu_map, ADDRESS_SPACE_PROGRAM, 8 )
    AM_RANGE(0x0000, 0x0fff) AM_ROM // AM_RAM is I8071 but will work with I8051
ADDRESS_MAP_END

static ADDRESS_MAP_START( mcu_data_map, ADDRESS_SPACE_DATA, 8 )
	AM_RANGE(0x0000, 0x007f) AM_RAM
	AM_RANGE(0x0100, 0x01ff) AM_RAM /* SFR */
ADDRESS_MAP_END


static ADDRESS_MAP_START( mcu_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READWRITE(mermaid_p0_r, mermaid_p0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(mermaid_p1_r, mermaid_p1_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(mermaid_p2_r, mermaid_p2_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(mermaid_p3_r, mermaid_p3_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( hvyunit )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, ( "Coin Mode" ) )
	PORT_DIPSETTING(    0x08, ( "Mode 1" ) )
	PORT_DIPSETTING(    0x00, ( "Mode 2" ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) 
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) 
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) ) 
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
    PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) ) 
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) ) 

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Bonus" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "7" )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( hvyunitj )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Bonus" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "7" )
    PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout tile_layout =
{
    16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{
		0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,
		8*32+0*4,8*32+1*4,8*32+2*4,8*32+3*4,8*32+4*4,8*32+5*4,8*32+6*4,8*32+7*4
	},
	{
		0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
		16*32+0*32,16*32+1*32,16*32+2*32,16*32+3*32,16*32+4*32,16*32+5*32,16*32+6*32,16*32+7*32
	},
	4*8*32
};

static const gfx_decode hvyunit_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0x100, 16 }, /* sprite */
	{ REGION_GFX2, 0, &tile_layout, 0x000, 16 }, /* background tiles */
	{ -1 }
};



/*************************************
 *
 *  Machine driver
 *
 *************************************/

static INTERRUPT_GEN( hvyunit_interrupt )
{
	static int addr = 0xff;
	addr ^= 0x02;
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, addr);

}


/*************************************
 *
 *  Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( hvyunit )

	MDRV_CPU_ADD(Z80,6000000)
    MDRV_CPU_PROGRAM_MAP(master_memory, 0)
	MDRV_CPU_IO_MAP(0, master_io)
	MDRV_CPU_VBLANK_INT(hvyunit_interrupt, 2)

	MDRV_CPU_ADD(Z80,6000000)
    MDRV_CPU_PROGRAM_MAP(slave_memory, 0)
	MDRV_CPU_IO_MAP(slave_io, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(Z80,6000000)
    MDRV_CPU_PROGRAM_MAP(sound_memory,0)
	MDRV_CPU_IO_MAP(sound_io,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(I8051,6000000)
	MDRV_CPU_PROGRAM_MAP(mcu_map,0)
    MDRV_CPU_DATA_MAP(mcu_data_map,0) 
	MDRV_CPU_IO_MAP(mcu_io, 0)

	MDRV_MACHINE_RESET(hvyunit)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 240-1)
	MDRV_GFXDECODE(hvyunit_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(hvyunit)
	MDRV_VIDEO_UPDATE(hvyunit)
	MDRV_VIDEO_EOF(hvyunit)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 3000000)
	MDRV_SOUND_ROUTE(0, "mono", 0.80)
	MDRV_SOUND_ROUTE(1, "mono", 0.80)
	MDRV_SOUND_ROUTE(2, "mono", 0.80)
	MDRV_SOUND_ROUTE(3, "mono", 0.80)
MACHINE_DRIVER_END 


/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( hvyunit )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "b73_10.5c",  0x00000, 0x20000, CRC(ca52210f) SHA1(346951962aa5bbad641117dbd66f035dddc7c0bf) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "b73_11.5p",  0x00000, 0x10000, CRC(cb451695) SHA1(116fd59f96a54c22fae65eea9ee5e58cb9ce5074) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD( "b73_12.7e",  0x000000, 0x010000, CRC(d1d24fab) SHA1(ed0312535d0b136d79aa885b9e6eea19ebde6409) )

	ROM_REGION( 0x1000, REGION_CPU4, 0 )
	ROM_LOAD( "mermaid.bin",  0x0000, 0x0e00, CRC(88c5dd27) SHA1(5043fed7fd192891be7e4096f2c5daaae1538bc4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "b73_08.2f",  0x000000, 0x080000, CRC(f83dd808) SHA1(09d5f1e86fad3a0d2d3ac1845103d3f2833c6793) )
	ROM_LOAD( "b73_07.2c",  0x100000, 0x010000, CRC(5cffa42c) SHA1(687e047345039479b35d5099e56dbc1d57284ed9) )
	ROM_LOAD( "b73_06.2b",  0x120000, 0x010000, CRC(a98e4aea) SHA1(560fef03ad818894c9c7578c6282d55b646e8129) )
	ROM_LOAD( "b73_01.1b",  0x140000, 0x010000, CRC(3a8a4489) SHA1(a01d7300015f90ce6dd571ad93e7a58270a99e47) )
	ROM_LOAD( "b73_02.1c",  0x160000, 0x010000, CRC(025c536c) SHA1(075e95cc39e792049ae656404e7f7440df064391) )
	ROM_LOAD( "b73_03.1d",  0x180000, 0x010000, CRC(ec6020cf) SHA1(2973aa2dc3deb2f27c9f1bad07a7664bad95b3f2) )
	/*                      0x190000, 0x010000  no data */
	ROM_LOAD( "b73_04.1f",  0x1a0000, 0x010000, CRC(f7badbb2) SHA1(d824ab4aba94d7ca02401f4f6f34213143c282ec) )
	/*                      0x1b0000, 0x010000  no data */
	ROM_LOAD( "b73_05.1h",  0x1c0000, 0x010000, CRC(b8e829d2) SHA1(31102358500d7b58173d4f18647decf5db744416) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "b73_09.2p",  0x000000, 0x080000, CRC(537c647f) SHA1(941c0f4e251bc68e53d62e70b033a3a6c145bb7e) )
ROM_END

ROM_START( hvyunitj )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "b73_30.5c",  0x00000, 0x20000, CRC(600af545) SHA1(c52b9be2bae28848ad0818c296f000a1bda4fa4f) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "b73_14.5p",  0x00000, 0x10000, CRC(0dfb51d4) SHA1(0e6f3b3d4558f12fe1b1620f57a0f4ac2065fd1a) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD( "b73_12.7e",  0x000000, 0x010000, CRC(d1d24fab) SHA1(ed0312535d0b136d79aa885b9e6eea19ebde6409) )

	ROM_REGION( 0x1000, REGION_CPU4, 0 )
	ROM_LOAD( "mermaid.bin",  0x0000, 0x0e00, CRC(88c5dd27) SHA1(5043fed7fd192891be7e4096f2c5daaae1538bc4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* note, the rom ordering on this is different to the other Japan set */
	ROM_LOAD( "b73_08.2f",  0x000000, 0x080000, CRC(f83dd808) SHA1(09d5f1e86fad3a0d2d3ac1845103d3f2833c6793) )
	ROM_LOAD( "b73_07.2c",  0x100000, 0x010000, CRC(5cffa42c) SHA1(687e047345039479b35d5099e56dbc1d57284ed9) )
	ROM_LOAD( "b73_06.2b",  0x110000, 0x010000, CRC(a98e4aea) SHA1(560fef03ad818894c9c7578c6282d55b646e8129) )
	ROM_LOAD( "b73_01.1b",  0x120000, 0x010000, CRC(3a8a4489) SHA1(a01d7300015f90ce6dd571ad93e7a58270a99e47) )
	ROM_LOAD( "b73_02.1c",  0x130000, 0x010000, CRC(025c536c) SHA1(075e95cc39e792049ae656404e7f7440df064391) )
	ROM_LOAD( "b73_03.1d",  0x140000, 0x010000, CRC(ec6020cf) SHA1(2973aa2dc3deb2f27c9f1bad07a7664bad95b3f2) )
	ROM_LOAD( "b73_04.1f",  0x150000, 0x010000, CRC(f7badbb2) SHA1(d824ab4aba94d7ca02401f4f6f34213143c282ec) )
	ROM_LOAD( "b73_05.1h",  0x160000, 0x010000, CRC(b8e829d2) SHA1(31102358500d7b58173d4f18647decf5db744416) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "b73_09.2p",  0x000000, 0x080000, CRC(537c647f) SHA1(941c0f4e251bc68e53d62e70b033a3a6c145bb7e) )
ROM_END

ROM_START( hvyunitjo )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "b73_13.5c",  0x00000, 0x20000, CRC(e2874601) SHA1(7f7f3287113b8622eb365d04135d2d9c35d70554) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "b73_14.5p",  0x00000, 0x10000, CRC(0dfb51d4) SHA1(0e6f3b3d4558f12fe1b1620f57a0f4ac2065fd1a) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD( "b73_12.7e",  0x000000, 0x010000, CRC(d1d24fab) SHA1(ed0312535d0b136d79aa885b9e6eea19ebde6409) )

	ROM_REGION( 0x1000, REGION_CPU4, 0 )
	ROM_LOAD( "mermaid.bin",  0x0000, 0x0e00, CRC(88c5dd27) SHA1(5043fed7fd192891be7e4096f2c5daaae1538bc4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "b73_08.2f",  0x000000, 0x080000, CRC(f83dd808) SHA1(09d5f1e86fad3a0d2d3ac1845103d3f2833c6793) )
	ROM_LOAD( "b73_07.2c",  0x100000, 0x010000, CRC(5cffa42c) SHA1(687e047345039479b35d5099e56dbc1d57284ed9) )
	ROM_LOAD( "b73_06.2b",  0x120000, 0x010000, CRC(a98e4aea) SHA1(560fef03ad818894c9c7578c6282d55b646e8129) )
	ROM_LOAD( "b73_01.1b",  0x140000, 0x010000, CRC(3a8a4489) SHA1(a01d7300015f90ce6dd571ad93e7a58270a99e47) )
	ROM_LOAD( "b73_02.1c",  0x160000, 0x010000, CRC(025c536c) SHA1(075e95cc39e792049ae656404e7f7440df064391) )
	ROM_LOAD( "b73_03.1d",  0x180000, 0x010000, CRC(ec6020cf) SHA1(2973aa2dc3deb2f27c9f1bad07a7664bad95b3f2) )
	/*                      0x190000, 0x010000  no data */
	ROM_LOAD( "b73_04.1f",  0x1a0000, 0x010000, CRC(f7badbb2) SHA1(d824ab4aba94d7ca02401f4f6f34213143c282ec) )
	/*                      0x1b0000, 0x010000  no data */
	ROM_LOAD( "b73_05.1h",  0x1c0000, 0x010000, CRC(b8e829d2) SHA1(31102358500d7b58173d4f18647decf5db744416) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "b73_09.2p",  0x000000, 0x080000, CRC(537c647f) SHA1(941c0f4e251bc68e53d62e70b033a3a6c145bb7e) )
ROM_END

ROM_START( hvyunitu )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "b73_34.5c",  0x00000, 0x20000, CRC(05c30a90) SHA1(97cc0ded2896e0945d790247c284e5058c28c735) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "b73_35.6p",  0x00000, 0x10000, CRC(aed1669d) SHA1(d0539261d6128fa2d58b529e8383b6d1f3ccac77) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD( "b73_12.7e",  0x000000, 0x010000, CRC(d1d24fab) SHA1(ed0312535d0b136d79aa885b9e6eea19ebde6409) )

	ROM_REGION( 0x1000, REGION_CPU4, 0 )
	ROM_LOAD( "mermaid.bin",  0x0000, 0x0e00, CRC(88c5dd27) SHA1(5043fed7fd192891be7e4096f2c5daaae1538bc4) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "b73_08.2f",  0x000000, 0x080000, CRC(f83dd808) SHA1(09d5f1e86fad3a0d2d3ac1845103d3f2833c6793) )
	ROM_LOAD( "b73_28.2c",  0x100000, 0x020000, CRC(a02e08d6) SHA1(72764d4e8474aaac0674fd1c20278a706da7ade2) )
	ROM_LOAD( "b73_27.2b",  0x120000, 0x020000, CRC(8708f97c) SHA1(ccddc7f2fa53c5e35345c2db0520f515c512b723) )
	ROM_LOAD( "b73_25.0b",  0x140000, 0x020000, CRC(2f13f81e) SHA1(9d9c1869bf582a0bc0581cdf5b65237124b9e456) ) /* the data in first half of this actually differs slightly to the other sets, a 0x22 fill is replaced by 0xff on empty tiles */
	ROM_LOAD( "b73_26.0c",  0x160000, 0x010000, CRC(b8e829d2) SHA1(31102358500d7b58173d4f18647decf5db744416) ) /* == b73_05.1h, despite the different label */

	ROM_REGION( 0x80000, REGION_GFX2, 0 )
	ROM_LOAD( "b73_09.2p",  0x000000, 0x080000, CRC(537c647f) SHA1(941c0f4e251bc68e53d62e70b033a3a6c145bb7e) )
ROM_END



/*************************************
 *
 *  Game driver(s)
 *
 *************************************/


GAME( 1988, hvyunit,   0,       hvyunit, hvyunit,  0, ROT0, "Kaneko / Taito", "Heavy Unit (World)", 0 )
GAME( 1988, hvyunitj,  hvyunit, hvyunit, hvyunitj, 0, ROT0, "Kaneko / Taito", "Heavy Unit (Japan, Newer)", 0 )
GAME( 1988, hvyunitjo, hvyunit, hvyunit, hvyunitj, 0, ROT0, "Kaneko / Taito", "Heavy Unit (Japan, Older)", 0 )
GAME( 1988, hvyunitu,  hvyunit, hvyunit, hvyunitj, 0, ROT0, "Kaneko / Taito", "Heavy Unit -U.S.A. Version- (US)", 0 )
