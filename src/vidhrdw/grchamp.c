/*  Video hardware for Taito Grand Champion */

/* updated by Hans Andersson, dec 2005     */
#include <math.h>
#include "driver.h"
#include "includes/grchamp.h"

#define FOG_SIZE 70

UINT8 grchamp_videoreg0;
static UINT8 grchamp_player_xpos;
UINT8 grchamp_player_ypos;
int  grchamp_collision;

static UINT8 grchamp_tile_number;
static UINT8 grchamp_rain_xpos;
static UINT8 grchamp_rain_ypos;

static int bg_palette_bank;
static mame_bitmap *work_bitmap;

UINT8 grchamp_vreg1[0x10];	/* background control registers */
UINT8 *grchamp_videoram;	/* background tilemaps */
UINT8 *grchamp_radar;		/* bitmap for radar */

static tilemap *bg_tilemap[3];

WRITE8_HANDLER( grchamp_player_xpos_w )
{
	grchamp_player_xpos = data;
}

WRITE8_HANDLER( grchamp_player_ypos_w )
{
	grchamp_player_ypos = data;
}

WRITE8_HANDLER( grchamp_tile_select_w )
{
	/* tile select: bits 4..7:rain; bits 0..3:player car */
	grchamp_tile_number = data;
}

WRITE8_HANDLER( grchamp_rain_xpos_w )
{
	grchamp_rain_xpos = data;
}

WRITE8_HANDLER( grchamp_rain_ypos_w )
{
	grchamp_rain_ypos = data;
}

static void add_fog(UINT8 inc, UINT8 *col)
{
	int add = *col + inc;

	if(add > 240)
		add = 240;
	*col = (UINT8) add;
}


PALETTE_INIT( grchamp )
{
	int i,j;
	UINT8 r,g,b;
	static const UINT8 rg_bits[8] = {0,42,67,92,126,139,148,156}; // Calculated values as below
	static const UINT8 b_bits[4]  = {0,67,126,148}; // Calculated values as below

	/* for r & g DAC is 100, 270, 470 Ohm wired together, connected to 100 Ohm pull-up and finally 100 Ohm in series */
    /* for b DAC is 100, 270 Ohm wired together, connected to 100 Ohm pull-up and finally 100 Ohm in series */

	for( i=0; i<0x20; i++ )
	{
		UINT8 data = *color_prom++;
		// data = BBGGGRRR
		/* red component */
		r = rg_bits[data & 7];
		/* green component */
		g = rg_bits[(data & 56) >> 3];
		/* blue component */
		b = b_bits[(data & 192) >> 6];
		palette_set_color(i,r,g,b);
	}

	/* The two color proms uses the same DAC to controll RGB, but second prom has less resolution, */
	/* Meaning on is the same as all bits are on */

	for( i=0; i<0x20; i++ )
	{
		int data = *color_prom++;
		r = (data&4)?0:rg_bits[7];
		g = (data&2)?0:rg_bits[7];
		b = (data&1)?0:b_bits[3];

		//Bit 3 indicate that the pixel is an object
		//and is used in the collision check (on the real board)
		//objects[i]=(data&8)?0:1;

		palette_set_color(i+0x20,r,g,b);
	}

	/* Fog pens*/
	for ( j=0; j<FOG_SIZE; j++ )
	{
		int fog_level;
		fog_level=250*(1-exp((double)-(FOG_SIZE-j+1)/20));
		//logerror("fog_level %d\n",fog_level);
		for( i=0; i<0x40; i++ )
		{
			palette_get_color(i, &r, &g, &b);

			add_fog(fog_level, &r);
			add_fog(fog_level, &g);
			add_fog(fog_level, &b);
			palette_set_color(i+(j+1)*0x40,r,g,b);
		}
	}

	palette_set_shadow_mode(1); // highlights
	palette_set_highlight_method(2); // 2=addition

	gfx_drawmode_table[0] = DRAWMODE_NONE; // - transparent
	for( i=1; i<0x20; i++ )
		gfx_drawmode_table[i] = DRAWMODE_SHADOW; // destination is changed through palette_shadow_table[]
}

WRITE8_HANDLER( grchamp_videoram_w )
{
	if( grchamp_videoram[offset]!=data )
	{
		grchamp_videoram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap[offset/0x800], offset%0x800 );
	}
}

static void get_bg0_tile_info( int offset )
{
	int tile_number = grchamp_videoram[offset];
	SET_TILE_INFO(
			1,
			tile_number,
			bg_palette_bank,
			0)
}

static void get_bg1_tile_info( int offset )
{
	int tile_number = grchamp_videoram[offset+0x800]+256;
	SET_TILE_INFO(
			1,
			tile_number,
			bg_palette_bank,
			0)
}

static void get_bg2_tile_info( int offset )
{
	int tile_number = grchamp_videoram[offset+0x800*2]+256*2;
	SET_TILE_INFO(
			1,
			tile_number,
			0,
			0)
}

static UINT32 get_memory_offset( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	int offset = (31-row)*32;
	offset += 31-(col%32);
	if( col/32 ) offset += 0x400;
	return offset;
}


VIDEO_START( grchamp )
{
	work_bitmap = auto_bitmap_alloc( 32,32 );
	if( !work_bitmap )
		return 1;

	bg_tilemap[0] = tilemap_create(get_bg0_tile_info,get_memory_offset,TILEMAP_OPAQUE,8,8,64,32);
	if(!bg_tilemap[0])
		return 1;
	bg_tilemap[1] = tilemap_create(get_bg1_tile_info,get_memory_offset,TILEMAP_TRANSPARENT,8,8,64,32);
	if(!bg_tilemap[1])
		return 1;
	bg_tilemap[2] = tilemap_create(get_bg2_tile_info,get_memory_offset,TILEMAP_TRANSPARENT,8,8,64,32);
	if(!bg_tilemap[2])
		return 1;

	tilemap_set_transparent_pen( bg_tilemap[1], 0 );
	tilemap_set_transparent_pen( bg_tilemap[2], 0 );
	return 0;
}

static void draw_text( mame_bitmap *bitmap, const rectangle *cliprect )
{
	const gfx_element *gfx = Machine->gfx[0];
	const UINT8 *source = videoram;
	int bank = (grchamp_videoreg0&0x20)?256:0;
	int offs;
	for( offs=0; offs<0x400; offs++ )
	{
		int col = offs%32;
		int row = offs/32;
		int scroll = colorram[col*2]-1;
		int attributes = colorram[col*2+1];
		int tile_number = source[offs];

		drawgfx(bitmap,
			gfx,
			bank + tile_number,
			attributes,
			0,0, /* no flip */
			8*col,
			(8*row-scroll)&0xff,
			cliprect,
			(col==31)?TRANSPARENCY_NONE:TRANSPARENCY_PEN,0);
	}
}

static void draw_background( mame_bitmap *bitmap, const rectangle *cliprect)
{
	int dx = -48;
	int dy = 16;
	int attributes = grchamp_vreg1[0x3];
		/*  ----xxxx    Analog Tachometer output
        **  ---x----    palette select
        **  --x-----    enables msb of bg#3 xscroll
        **  xx------    unused
        */

	int color = (attributes&0x10)?1:0;
	if( color != bg_palette_bank )
	{
		bg_palette_bank = color;
		tilemap_mark_all_tiles_dirty( ALL_TILEMAPS );
	}

	tilemap_set_scrollx( bg_tilemap[0], 0, dx-(grchamp_vreg1[0x0]+grchamp_vreg1[0x1]*256) );
	tilemap_set_scrolly( bg_tilemap[0], 0, dy - grchamp_vreg1[0x2] );
	tilemap_set_scrollx( bg_tilemap[1], 0, dx-(grchamp_vreg1[0x5]+grchamp_vreg1[0x6]*256) );
	tilemap_set_scrolly( bg_tilemap[1], 0, dy - grchamp_vreg1[0x7] );
	tilemap_set_scrollx( bg_tilemap[2], 0, dx-(grchamp_vreg1[0x9]+ ((attributes&0x20)?256:(grchamp_vreg1[0xa]*256))));
	tilemap_set_scrolly( bg_tilemap[2], 0, dy - grchamp_vreg1[0xb] );

	tilemap_draw(bitmap,cliprect,bg_tilemap[0],0,0); //Left lane
	tilemap_draw(bitmap,cliprect,bg_tilemap[1],0,0); //Right lane
	tilemap_draw(bitmap,cliprect,bg_tilemap[2],0,0); //Center lane
}

static void draw_player_car( mame_bitmap *bitmap, const rectangle *cliprect )
{
	drawgfx( bitmap,
		Machine->gfx[2],
		grchamp_tile_number&0xf,
		1, /* color = red */
		0,0, /* flip */
		256-grchamp_player_xpos,
		240-grchamp_player_ypos,
		cliprect,
		TRANSPARENCY_PEN, 0 );
}

static int collision_check( mame_bitmap *bitmap, int which )
{
	int bgcolor = Machine->pens[0];
	int sprite_transp = Machine->pens[0x24];
	const rectangle *clip = &Machine->visible_area;
	int y0 = 240-grchamp_player_ypos;
	int x0 = 256-grchamp_player_xpos;
	int x,y,sx,sy;
	int pixel;
	int result = 0;

	if( which==0 )
	{
		/* draw the current player sprite into a work bitmap */
		drawgfx( work_bitmap,
			Machine->gfx[2],
			grchamp_tile_number&0xf,
			1, /* color */
			0,0,
			0,0,
			0,
			TRANSPARENCY_NONE, 0 );
	}

	for( y = 0; y <32; y++ )
	{
		for( x = 0; x<32; x++ )
		{
			pixel = read_pixel(work_bitmap,x,y);
			if( pixel != sprite_transp ){
				sx = x+x0;
				sy = y+y0;
				if( (sx >= clip->min_x) && (sx <= clip->max_x) &&
					(sy >= clip->min_y) && (sy <= clip->max_y) )
				{
					// Collision check uses only 16 pens!
					pixel = read_pixel(bitmap, sx, sy) % 16;
					if( pixel != bgcolor )
					{
						result = 1; /* flag collision */
						/*  wipe this pixel, so collision checks with the
                        **  next layer work */
						plot_pixel( bitmap, sx, sy, bgcolor );
					}
				}
			}

        }
	}
	return result?(1<<which):0;
}

static void draw_rain( mame_bitmap *bitmap, const rectangle *cliprect ){
	const gfx_element *gfx = Machine->gfx[4];
	int tile_number = grchamp_tile_number>>4;
	if( tile_number ){
		int scrollx = grchamp_rain_xpos;
		int scrolly = grchamp_rain_ypos;
		int sx,sy;

		palette_set_highlight_factor32(1.7);

		for( sy=0; sy<256; sy+=16 ){
			for( sx=0; sx<256; sx+=16 ){
				drawgfx( bitmap,
					gfx,
					tile_number,
					1,
					0,0,
					(sx+scrollx)&0xff,(sy+scrolly)&0xff,
					cliprect,
					TRANSPARENCY_PEN_TABLE, 0 );
			}
		}
	}
}

static void draw_fog( mame_bitmap *bitmap, const rectangle *cliprect, int fog ){
	int x,y,pen,offs;

	// Emulation of analog fog effect
	for(x=0;x<100;x++)
	{
		offs = 0x40;
		if(x > (100-FOG_SIZE-1))
			offs = 0x40*(x-(100-FOG_SIZE-1));
		for(y=16;y<240;y++)
		{
			pen = read_pixel(bitmap, x, y);
			plot_pixel(bitmap,x, y, pen + offs);

		}
	}
}

static void draw_headlights( mame_bitmap *bitmap, const rectangle *cliprect, int fog )
{
	int x0 = 256-grchamp_player_xpos-64;
	int y0 = 240-grchamp_player_ypos-64;
	const gfx_element *gfx = Machine->gfx[5];
	int code=0;

	if(!fog)
	{
		code +=2;
		palette_set_highlight_factor32(1.7);
	}
	else
		palette_set_highlight_factor32(1.3);

	// TODO - fog headlights should have highlights without blue component
	// i.e if(fog) palette_set_shadow_dRGB32(1,10,10,0,0);, but that appears
	// not to be supported.

	drawgfx( bitmap,
			gfx,
			code+0,
			1,
			0,0,
			x0,
			y0,
			cliprect,
			TRANSPARENCY_PEN_TABLE, 0 );

	drawgfx( bitmap,
			gfx,
			code+1,
			1,
			0,0,
			x0,
			y0+64,
			cliprect,
			TRANSPARENCY_PEN_TABLE, 0 );

}

static void draw_radar( mame_bitmap *bitmap, const rectangle *cliprect ){
	const UINT8 *source = grchamp_radar;
	int color = Machine->pens[3];
	int offs;
	for( offs=0; offs<0x400; offs++ ){
		int data = source[offs];
		if( data ){
			int x = (offs%32)*8;
			int y = (offs/32)+16;
			int bit;
			for( bit=0; bit<8; bit++ ){
				if( data&0x80 )
					if ((x+bit) >= cliprect->min_x && (x+bit) <= cliprect->max_x &&
						y >= cliprect->min_y && y <= cliprect->max_y)
						plot_pixel( bitmap, x+bit, y, color );
				data <<= 1;
			}
		}
	}
}

static void draw_tachometer( mame_bitmap *bitmap, const rectangle *cliprect ){
/*
    int value = grchamp_vreg1[0x03]&0xf;
    int i;
    for( i=0; i<value; i++ ){
        drawgfx( bitmap, olduifont,
            '*',
            0,
            0,0,
            i*6+32,64,
            0,
            TRANSPARENCY_NONE, 0 );
    }
*/
}

static void draw_sprites( mame_bitmap *bitmap, const rectangle *cliprect){
	const gfx_element *gfx = Machine->gfx[3];
	int bank = (grchamp_videoreg0&0x20)?0x40:0x00;
	const UINT8 *source = spriteram;
	const UINT8 *finish = source+0x40;


	while( source<finish ){
		int sx = source[3];
		int sy = 240-source[0];
		int color = source[2];
		int code = source[1];
		drawgfx( bitmap, gfx,
			bank + (code&0x3f),
			color,
			code&0x40,
			code&0x80,
			sx,sy,
			cliprect,
			TRANSPARENCY_PEN, 0 );
		source += 4;
	}
}

VIDEO_UPDATE( grchamp ){
	int fog = grchamp_videoreg0&0x40;
	int tunnel = grchamp_videoreg0&0x10;

	draw_background( bitmap,cliprect ); /* 3 layers */

	grchamp_collision = collision_check( bitmap, 0 );
	draw_sprites( bitmap, cliprect); /* computer cars */
	grchamp_collision |= collision_check( bitmap, 1 );
    draw_player_car( bitmap,cliprect );

	draw_rain( bitmap,cliprect );
	draw_text( bitmap,cliprect );

	if( grchamp_videoreg0&0x80 )
		draw_radar( bitmap,cliprect );
	draw_tachometer( bitmap,cliprect );

	/* Highlight the visible area exposed by headlights shape */
	if( fog || tunnel ){
		draw_headlights( bitmap, cliprect, fog );
	}

	/* Draw fog last, since it will affect the whole bitmap */
	if( fog ){
		draw_fog( bitmap,cliprect, fog );
	}
}
