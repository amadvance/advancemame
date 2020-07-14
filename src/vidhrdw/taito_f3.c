/***************************************************************************

   Taito F3 Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************

Brief overview:

    4 scrolling layers (512x512 or 1024x512) of 4/5/6 bpp tiles.
    1 scrolling text layer (512x512, characters generated in vram), 4bpp chars.
    1 scrolling pixel layer (512x256 pixels generated in pivot ram), 4bpp pixels.
    2 sprite banks (for double buffering of sprites)
    Sprites can be 4, 5 or 6 bpp
    Sprite scaling.
    Rowscroll on all playfields
    Line by line zoom on all playfields
    Column scroll on all playfields
    Line by line sprite and playfield priority mixing

Notes:
    All special effects are controlled by an area in 'line ram'.  Typically
    there are 256 values, one for each line of the screen (including clipped
    lines at the top of the screen).  For example, at 0x8000 in lineram,
    there are 4 sets of 256 values (one for each playfield) and each value
    is the scale control for that line in the destination bitmap (screen).
    Therefore each line can have a different zoom value for special effects.

    This also applies to playfield priority, rowscroll, column scroll, sprite
    priority and VRAM/pivot layers.

    However - at the start of line ram there are also sets of 256 values
    controlling each effect - effects can be selectively applied to individual
    playfields or only certain lines out of the 256 can be active - in which
    case the last allowed value can be latched (typically used so a game can
    use one zoom or row value over the whole playfield).

    The programmers of some of these games made strange use of flipscreen -
    some games have all their graphics flipped in ROM, and use the flipscreen
    bit to display them correctly!.

    Most games display 232 scanlines, but some use lineram effects to clip
    themselves to 224 or less.

****************************************************************************

Line ram memory map:

    Here 'playfield 1' refers to the first playfield in memory, etc

    0x0000: Column line control ram (256 lines)
        100x:   Where bit 0 of x enables effect on playfield 1
                Where bit 1 of x enables effect on playfield 2
                Where bit 2 of x enables effect on playfield 3
                Where bit 3 of x enables effect on playfield 4
    0x0200: Line control ram for 0x5000 section.
    0x0400: Line control ram for 0x6000 section.
        180x:   Where bit 0 of x latches sprite alpha value
                Where bit 1 of x latches tilemap alpha value
    0x0600: Sprite control ram
        1c0x:   Where x enables sprite control word for that line
    0x0800: Zoom line control ram (256 lines)
        200x:   Where bit 0 of x enables effect on playfield 1
                Where bit 1 of x enables effect on playfield 2
                Where bit 2 of x enables effect on playfield 3
                Where bit 3 of x enables effect on playfield 4
    0x0a00: Assumed unused.
    0x0c00: Rowscroll line control ram (256 lines)
        280x:   Where bit 0 of x enables effect on playfield 1
                Where bit 1 of x enables effect on playfield 2
                Where bit 2 of x enables effect on playfield 3
                Where bit 3 of x enables effect on playfield 4
    0x0e00: Priority line control ram (256 lines)
        2c0x:   Where bit 0 of x enables effect on playfield 1
                Where bit 1 of x enables effect on playfield 2
                Where bit 2 of x enables effect on playfield 3
                Where bit 3 of x enables effect on playfield 4

    0x4000: Column scroll & clipping info

    0x5000: Clip plane 0 (low bits)
    0x5200: Clip plane 1 (low bits)
    0x5400: Unused?
    0x5600: Unused?

    0x6000: Sync register
    0x6004: Sprite alpha control
        0xff00: VRAM/Pixel layer control
            0xa000 enables pixel layer for this line (Disables VRAM layer)
            0x2000 enables garbage pixels for this line (maybe another pixel bank?) [unemulated]
            0x0800 seems to set the vram layer to be opaque [unemulated]
            Effect of other bits is unknown.
        0x00c0: Alpha mode for sprites with pri value 0x00
        0x0030: Alpha mode for sprites with pri value 0x00
        0x000c: Alpha mode for sprites with pri value 0x00
        0x0003: Alpha mode for sprites with pri value 0x00
    0x6200: Alpha blending control

    0x6400 - controls X zoom of tile - each tile collapses to 16 single colour lines
        xxx1 - affects bottom playfield
        xxx2 -
        xxx4 -
        xxx8 - affects top playfield
        xxfx - x zoom - 0 = 1st pixel 16 times
                        1 = 1st pixel 8, then 2nd 8
                        8 = 2 per pixel

        (these effects only known to be used on spcinvdj title screen and riding fight - not emulated)

        1xxx = interpret palette ram for this playfield line as 15 bit and bilinear filter framebuffer(??)
        3xxx = interpret palette ram for this playfield line as 15 bit
        7xxx = interpret palette ram for this playfield line as 24 bit palette
        8xxx = interpret palette ram for this playfield line as 21 bit palette

        (effect not emulated)

    0x6600: Background - background palette entry (under all tilemaps) (takes part in alpha blending)

        (effect not emulated)

    0x7000: Pivot/vram layer enable
    0x7200: Cram layer priority
    0x7400: Sprite clipping
    0x7600: Sprite priority values
        0xf000: Relative priority for sprites with pri value 0xc0
        0x0f00: Relative priority for sprites with pri value 0x80
        0x00f0: Relative priority for sprites with pri value 0x40
        0x000f: Relative priority for sprites with pri value 0x00

    0x8000: Playfield 1 scale (1 word per line, 256 lines, 0x80 = default no scale)
    0x8200: Playfield 2/4 scale
    0x8400: Playfield 3 scale
    0x8600: Playfield 2/4 scale
        0x00ff = Y scale (0x80 = no scale, > 0x80 is zoom in, < 0x80 is zoom out [0xc0 is half height of 0x80])
        0xff00 = X scale (0 = no scale, > 0 = zoom in, [0x8000 would be double length])

        Playfield 2 & 4 registers seem to be interleaved, playfield 2 Y zoom is stored where you would
        expect playfield 4 y zoom to be and vice versa.

    0x9000: Palette add (can affect opacity) [ not emulated ]

    0xa000: Playfield 1 rowscroll (1 word per line, 256 lines)
    0xa200: Playfield 2 rowscroll
    0xa400: Playfield 3 rowscroll
    0xa600: Playfield 4 rowscroll

    0xb000: Playfield 1 priority (1 word per line, 256 lines)
    0xb200: Playfield 2 priority
    0xb400: Playfield 3 priority
    0xb600: Playfield 4 priority
        0xf000 = Blend mode, 0x3000 = Normal, 0x7000 = Alpha A, 0xb000 = Alpha B, others disable line
        0x0800 = Clip line (totally disable line)
        0x0400 = Assumed unused
        0x0200 = If set enable clip plane 1
        0x0100 = If set enable clip plane 0
        0x00c0 = Assumed unused
        0x0020 = Enable clip inverse mode for plane 1
        0x0010 = Enable clip inverse mode for plane 0
        0x000f = Layer priority

    0xc000 - 0xffff: Unused.

    When sprite priority==playfield priority sprite takes precedence (Bubble Symph title)

****************************************************************************

    F3 sprite format:

    Word 0: 0xffff      Tile number (LSB)
    Word 1: 0xff00      X zoom
            0x00ff      Y zoom
    Word 2: 0x03ff      X position
    Word 3: 0x03ff      Y position
    Word 4: 0xf000      Sprite block controls
            0x0800      Sprite block start
            0x0400      Use same colour on this sprite as block start
            0x0200      Y flip
            0x0100      X flip
            0x00ff      Colour
    Word 5: 0xffff      Tile number (MSB), probably only low bits used
    Word 6: 0x8000      If set, jump to sprite location in low bits
            0x03ff      Location to jump to.
    Word 7: 0xffff      Unused?  Always zero?

****************************************************************************

    Playfield control information (0x660000-1f):

    Word 0- 3: X scroll values for the 4 playfields.
    Word 4- 7: Y scroll values for the 4 playfields.
    Word 8-11: Unused.  Always zero.
    Word   12: Pixel + VRAM playfields X scroll
    Word   13: Pixel + VRAM playfields Y scroll
    Word   14: Unused. Always zero.
    Word   15: If set to 0x80, then 1024x512 playfields are used, else 512x512

    top bits unused - writing to low 4 bits is desync or blank tilemap

Playfield tile info:
    0xc000 0000 - X/Y Flip
    0x3000 ffff - Tile index
    0x0dff 0000 - Colour (top 2 bits use not clear, always set)
    0x0200 0000 - Alpha blend mode

***************************************************************************/

#include "driver.h"
#include "taito_f3.h"

#define DARIUSG_KLUDGE
//#define DEBUG_F3 1

static tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static tilemap *pixel_layer,*vram_layer;
static UINT32 *spriteram32_buffered;
static int vram_dirty[256];
static int pivot_changed,vram_changed;
static UINT32 f3_control_0[8];
static UINT32 f3_control_1[8];
static int flipscreen;

static UINT8 *pivot_dirty;

static UINT32 *f3_pf_data_1,*f3_pf_data_2,*f3_pf_data_3,*f3_pf_data_4;

UINT32 *f3_vram,*f3_line_ram;
UINT32 *f3_pf_data,*f3_pivot_ram;

static int f3_skip_this_frame;

/* Game specific data, some of this can be
removed when the software values are figured out */
struct F3config
{
	int name;
	int extend;
	int sprite_lag;
};

static const struct F3config *f3_game_config;

static const struct F3config f3_config_table[] =
{
	/* Name    Extend  Sprite Lag */
	{ RINGRAGE,  0,       2 },
	{ ARABIANM,  0,       2 },
	{ RIDINGF,   1,       1 },
	{ GSEEKER,   0,       1 },
	{ TRSTAR,    1,       0 },
	{ GUNLOCK,   1,       2 },
	{ TWINQIX,   1,       1 },
	{ SCFINALS,  0,       1 },
	{ LIGHTBR,   1,       2 },
	{ KAISERKN,  0,       2 },
	{ DARIUSG,   0,       2 },
	{ BUBSYMPH,  1,       1 },
	{ SPCINVDX,  1,       1 },
	{ QTHEATER,  1,       1 },
	{ HTHERO95,  0,       1 },
	{ SPCINV95,  0,       1 },
	{ EACTION2,  1,       2 },
	{ QUIZHUHU,  1,       1 },
	{ PBOBBLE2,  0,       1 },
	{ GEKIRIDO,  0,       1 },
	{ KTIGER2,   0,       0 },
	{ BUBBLEM,   1,       1 },
	{ CLEOPATR,  0,       1 },
	{ PBOBBLE3,  0,       1 },
	{ ARKRETRN,  1,       1 },
	{ KIRAMEKI,  0,       1 },
	{ PUCHICAR,  1,       1 },
	{ PBOBBLE4,  0,       1 },
	{ POPNPOP,   1,       1 },
	{ LANDMAKR,  1,       1 },
	{ RECALH,    1,       1 },
	{ COMMANDW,  1,       1 },
	{0}
};


struct tempsprite
{
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int pri;
};
static struct tempsprite *spritelist;
static const struct tempsprite *sprite_end;

static void get_sprite_info(const UINT32 *spriteram32_ptr);
static int sprite_lag=1;
static UINT8 sprite_pri_usage=0;

struct f3_playfield_line_inf
{
	int alpha_mode[256];
	int pri[256];

	/* use for draw_scanlines */
	UINT16 *src[256],*src_s[256],*src_e[256];
	UINT8 *tsrc[256],*tsrc_s[256];
	int x_count[256];
	UINT32 x_zoom[256];
	UINT32 clip0[256];
	UINT32 clip1[256];
};

struct f3_spritealpha_line_inf
{
	UINT16 alpha_level[256];
	UINT16 spri[256];
	UINT16 sprite_alpha[256];
	UINT32 sprite_clip0[256];
	UINT32 sprite_clip1[256];
	INT16 clip0_l[256];
	INT16 clip0_r[256];
	INT16 clip1_l[256];
	INT16 clip1_r[256];
};

/*
alpha_mode
---- --xx    0:disable 1:nomal 2:alpha 7000 3:alpha b000
---1 ----    alpha level a
--1- ----    alpha level b
1--------    opaque line
*/

static struct f3_playfield_line_inf *pf_line_inf;
static struct f3_spritealpha_line_inf *sa_line_inf;


static mame_bitmap *pri_alp_bitmap;
/*
pri_alp_bitmap
---- ---1    sprite priority 0
---- --1-    sprite priority 1
---- -1--    sprite priority 2
---- 1---    sprite priority 3
---1 ----    alpha level a 7000
--1- ----    alpha level b 7000
-1-- ----    alpha level a b000
1--- ----    alpha level b b000
1111 1111    opaque pixel
*/
static int f3_alpha_level_2as=127;
static int f3_alpha_level_2ad=127;
static int f3_alpha_level_3as=127;
static int f3_alpha_level_3ad=127;
static int f3_alpha_level_2bs=127;
static int f3_alpha_level_2bd=127;
static int f3_alpha_level_3bs=127;
static int f3_alpha_level_3bd=127;

static void init_alpha_blend_func(void);

static int width_mask=0x1ff;
static int twidth_mask=0x1f,twidth_mask_bit=5;
static UINT8 *tile_opaque_sp;
static UINT8 *tile_opaque_pf;


/******************************************************************************/

static void print_debug_info(mame_bitmap *bitmap)
{
	int l[16];
	char buf[64*16];
	char *bufptr = buf;

	bufptr += sprintf(bufptr,"%04X %04X %04X %04X\n",f3_control_0[0]>>22,(f3_control_0[0]&0xffff)>>6,f3_control_0[1]>>22,(f3_control_0[1]&0xffff)>>6);
	bufptr += sprintf(bufptr,"%04X %04X %04X %04X\n",f3_control_0[2]>>23,(f3_control_0[2]&0xffff)>>7,f3_control_0[3]>>23,(f3_control_0[3]&0xffff)>>7);
	bufptr += sprintf(bufptr,"%04X %04X %04X %04X\n",f3_control_1[0]>>16,f3_control_1[0]&0xffff,f3_control_1[1]>>16,f3_control_1[1]&0xffff);
	bufptr += sprintf(bufptr,"%04X %04X %04X %04X\n",f3_control_1[2]>>16,f3_control_1[2]&0xffff,f3_control_1[3]>>16,f3_control_1[3]&0xffff);

	bufptr += sprintf(bufptr,"%04X %04X %04X %04X %04X %04X %04X %04X\n",spriteram32_buffered[0]>>16,spriteram32_buffered[0]&0xffff,spriteram32_buffered[1]>>16,spriteram32_buffered[1]&0xffff,spriteram32_buffered[2]>>16,spriteram32_buffered[2]&0xffff,spriteram32_buffered[3]>>16,spriteram32_buffered[3]&0xffff);
	bufptr += sprintf(bufptr,"%04X %04X %04X %04X %04X %04X %04X %04X\n",spriteram32_buffered[4]>>16,spriteram32_buffered[4]&0xffff,spriteram32_buffered[5]>>16,spriteram32_buffered[5]&0xffff,spriteram32_buffered[6]>>16,spriteram32_buffered[6]&0xffff,spriteram32_buffered[7]>>16,spriteram32_buffered[7]&0xffff);
	bufptr += sprintf(bufptr,"%04X %04X %04X %04X %04X %04X %04X %04X\n",spriteram32_buffered[8]>>16,spriteram32_buffered[8]&0xffff,spriteram32_buffered[9]>>16,spriteram32_buffered[9]&0xffff,spriteram32_buffered[10]>>16,spriteram32_buffered[10]&0xffff,spriteram32_buffered[11]>>16,spriteram32_buffered[11]&0xffff);

	l[0]=f3_line_ram[0x0040]&0xffff;
	l[1]=f3_line_ram[0x00c0]&0xffff;
	l[2]=f3_line_ram[0x0140]&0xffff;
	l[3]=f3_line_ram[0x01c0]&0xffff;
	bufptr += sprintf(bufptr,"Ctr1: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x0240]&0xffff;
	l[1]=f3_line_ram[0x02c0]&0xffff;
	l[2]=f3_line_ram[0x0340]&0xffff;
	l[3]=f3_line_ram[0x03c0]&0xffff;
	bufptr += sprintf(bufptr,"Ctr2: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x2c60]&0xffff;
	l[1]=f3_line_ram[0x2ce0]&0xffff;
	l[2]=f3_line_ram[0x2d60]&0xffff;
	l[3]=f3_line_ram[0x2de0]&0xffff;
	bufptr += sprintf(bufptr,"Pri : %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x2060]&0xffff;
	l[1]=f3_line_ram[0x20e0]&0xffff;
	l[2]=f3_line_ram[0x2160]&0xffff;
	l[3]=f3_line_ram[0x21e0]&0xffff;
	bufptr += sprintf(bufptr,"Zoom: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x2860]&0xffff;
	l[1]=f3_line_ram[0x28e0]&0xffff;
	l[2]=f3_line_ram[0x2960]&0xffff;
	l[3]=f3_line_ram[0x29e0]&0xffff;
	bufptr += sprintf(bufptr,"Line: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x1c60]&0xffff;
	l[1]=f3_line_ram[0x1ce0]&0xffff;
	l[2]=f3_line_ram[0x1d60]&0xffff;
	l[3]=f3_line_ram[0x1de0]&0xffff;
	bufptr += sprintf(bufptr,"Sprt: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x1860]&0xffff;
	l[1]=f3_line_ram[0x18e0]&0xffff;
	l[2]=f3_line_ram[0x1960]&0xffff;
	l[3]=f3_line_ram[0x19e0]&0xffff;
	bufptr += sprintf(bufptr,"Pivt: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x1060]&0xffff;
	l[1]=f3_line_ram[0x10e0]&0xffff;
	l[2]=f3_line_ram[0x1160]&0xffff;
	l[3]=f3_line_ram[0x11e0]&0xffff;
	bufptr += sprintf(bufptr,"Colm: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	l[0]=f3_line_ram[0x1460]&0xffff;
	l[1]=f3_line_ram[0x14e0]&0xffff;
	l[2]=f3_line_ram[0x1560]&0xffff;
	l[3]=f3_line_ram[0x15e0]&0xffff;
	bufptr += sprintf(bufptr,"5000: %04x %04x %04x %04x\n",l[0],l[1],l[2],l[3]);

	ui_draw_text(buf, 60, 40);
}

/******************************************************************************/

INLINE void get_tile_info(int tile_index, UINT32 *gfx_base)
{
	UINT32 tile=gfx_base[tile_index];
	UINT8 abtype=(tile>>(16+9))&0x1f;

	SET_TILE_INFO(
			1,
			tile&0xffff,
			(tile>>16)&0x1ff,
			TILE_FLIPYX( tile >> 30 ))
	tile_info.priority =  abtype&1;		/* alpha blending type */
}

static void get_tile_info1(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_1);
}

static void get_tile_info2(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_2);
}

static void get_tile_info3(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_3);
}

static void get_tile_info4(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_4);
}

static void get_tile_info_vram(int tile_index)
{
	int vram_tile;

	if (tile_index&1)
	   	vram_tile = (videoram32[tile_index>>1]&0xffff);
	else
		vram_tile = (videoram32[tile_index>>1]>>16);

	SET_TILE_INFO(
			0,
			vram_tile&0xff,
			(vram_tile>>9)&0x3f,
			0)

	tile_info.flags=0;
	if (vram_tile&0x0100) tile_info.flags|=TILE_FLIPX;
	if (vram_tile&0x8000) tile_info.flags|=TILE_FLIPY;
}

static void get_tile_info_pixel(int tile_index)
{
	int vram_tile,col_off;
	int y_offs=(f3_control_1[2]&0x1ff);
	if (flipscreen) y_offs+=0x100;

	/* Colour is shared with VRAM layer */
	if ((((tile_index%32)*8 + y_offs)&0x1ff)>0xff)
		col_off=0x800+((tile_index%32)*0x40)+((tile_index&0xfe0)>>5);
	else
		col_off=((tile_index%32)*0x40)+((tile_index&0xfe0)>>5);

	if (col_off&1)
	   	vram_tile = (videoram32[col_off>>1]&0xffff);
	else
		vram_tile = (videoram32[col_off>>1]>>16);

	SET_TILE_INFO(
			3,
			tile_index,
			(vram_tile>>9)&0x3f,
			0)

	tile_info.flags=0;
	if (vram_tile&0x0100) tile_info.flags|=TILE_FLIPX;
	if (vram_tile&0x8000) tile_info.flags|=TILE_FLIPY;
}

/******************************************************************************/

VIDEO_EOF( f3 )
{
	if (sprite_lag==2)
	{
		if (skip_this_frame() == 0)
		{
			get_sprite_info(spriteram32_buffered);
		}
		memcpy(spriteram32_buffered,spriteram32,spriteram_size);
	}
	else if (sprite_lag==1)
	{
		if (skip_this_frame() == 0)
		{
			get_sprite_info(spriteram32);
		}
	}
}

VIDEO_START( f3 )
{
	const struct F3config *pCFG=&f3_config_table[0];
	int tile;

	spritelist=0;
	spriteram32_buffered=0;
	pivot_dirty=0;
	pf_line_inf=0;
	pri_alp_bitmap=0;
	tile_opaque_sp=0;
	tile_opaque_pf=0;

	/* Setup individual game */
	do {
		if (pCFG->name==f3_game)
		{
			break;
		}
		pCFG++;
	} while(pCFG->name);

	f3_game_config=pCFG;

	if (f3_game_config->extend) {
		pf1_tilemap = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf2_tilemap = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf3_tilemap = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf4_tilemap = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);

		f3_pf_data_1=f3_pf_data+0x0000;
		f3_pf_data_2=f3_pf_data+0x0800;
		f3_pf_data_3=f3_pf_data+0x1000;
		f3_pf_data_4=f3_pf_data+0x1800;

		width_mask=0x3ff;
		twidth_mask=0x3f;
		twidth_mask_bit=6;

	} else {
		pf1_tilemap = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf2_tilemap = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf3_tilemap = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf4_tilemap = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

		f3_pf_data_1=f3_pf_data+0x0000;
		f3_pf_data_2=f3_pf_data+0x0400;
		f3_pf_data_3=f3_pf_data+0x0800;
		f3_pf_data_4=f3_pf_data+0x0c00;

		width_mask=0x1ff;
		twidth_mask=0x1f;
		twidth_mask_bit=5;
	}

	spriteram32_buffered = (UINT32 *)auto_malloc(0x10000);
	spritelist = auto_malloc(0x400 * sizeof(*spritelist));
	sprite_end = spritelist;
	vram_layer = tilemap_create(get_tile_info_vram,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);
	pixel_layer = tilemap_create(get_tile_info_pixel,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	pivot_dirty = (UINT8 *)auto_malloc(2048);
	pf_line_inf = auto_malloc(5 * sizeof(struct f3_playfield_line_inf));
	sa_line_inf = auto_malloc(1 * sizeof(struct f3_spritealpha_line_inf));
	pri_alp_bitmap = auto_bitmap_alloc_depth( Machine->drv->screen_width, Machine->drv->screen_height, -8 );
	tile_opaque_sp = (UINT8 *)auto_malloc(Machine->gfx[2]->total_elements);
	tile_opaque_pf = (UINT8 *)auto_malloc(Machine->gfx[1]->total_elements);

	if (!vram_layer || !pixel_layer || !pri_alp_bitmap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	tilemap_set_transparent_pen(vram_layer,0);
	tilemap_set_transparent_pen(pixel_layer,0);

	/* Palettes have 4 bpp indexes despite up to 6 bpp data */
	Machine->gfx[1]->color_granularity=16;
	Machine->gfx[2]->color_granularity=16;

	flipscreen = 0;
	memset(spriteram32_buffered,0,spriteram_size);
	memset(spriteram32,0,spriteram_size);

	state_save_register_global_array(f3_control_0);
	state_save_register_global_array(f3_control_1);

	for (tile = 0;tile < 256;tile++)
		vram_dirty[tile]=1;
	for (tile = 0;tile < 2048;tile++)
		pivot_dirty[tile]=1;

	f3_skip_this_frame=0;

	sprite_lag=f3_game_config->sprite_lag;

	init_alpha_blend_func();

	{
		const gfx_element *sprite_gfx = Machine->gfx[2];
		int c;

		for (c = 0;c < sprite_gfx->total_elements;c++)
		{
			int x,y;
			int chk_trans_or_opa=0;
			UINT8 *dp = sprite_gfx->gfxdata + c * sprite_gfx->char_modulo;
			for (y = 0;y < sprite_gfx->height;y++)
			{
				for (x = 0;x < sprite_gfx->width;x++)
				{
					if(!dp[x]) chk_trans_or_opa|=2;
					else	   chk_trans_or_opa|=1;
				}
				dp += sprite_gfx->line_modulo;
			}
			if(chk_trans_or_opa==1) tile_opaque_sp[c]=1;
			else					tile_opaque_sp[c]=0;
		}
	}


	{
		const gfx_element *pf_gfx = Machine->gfx[1];
		int c;

		for (c = 0;c < pf_gfx->total_elements;c++)
		{
			int x,y;
			int chk_trans_or_opa=0;
			UINT8 *dp = pf_gfx->gfxdata + c * pf_gfx->char_modulo;
			for (y = 0;y < pf_gfx->height;y++)
			{
				for (x = 0;x < pf_gfx->width;x++)
				{
					if(!dp[x]) chk_trans_or_opa|=2;
					else	   chk_trans_or_opa|=1;
				}
				dp += pf_gfx->line_modulo;
			}
			tile_opaque_pf[c]=chk_trans_or_opa;
		}
	}

	return 0;
}

/******************************************************************************/

WRITE32_HANDLER( f3_pf_data_w )
{
	COMBINE_DATA(&f3_pf_data[offset]);

	if (f3_game_config->extend) {
		if (offset<0x800) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0800);
		else if (offset<0x1800) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x1000);
		else if (offset<0x2000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0x1800);
	} else {
		if (offset<0x400) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x800) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0400);
		else if (offset<0xc00) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x0800);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0xc00);
	}
}

WRITE32_HANDLER( f3_control_0_w )
{
	COMBINE_DATA(&f3_control_0[offset]);
}

WRITE32_HANDLER( f3_control_1_w )
{
	COMBINE_DATA(&f3_control_1[offset]);
}

WRITE32_HANDLER( f3_videoram_w )
{
	int tile,col_off;
	COMBINE_DATA(&videoram32[offset]);

	tilemap_mark_tile_dirty(vram_layer,offset<<1);
	tilemap_mark_tile_dirty(vram_layer,(offset<<1)+1);

	if (offset>0x3ff) offset-=0x400;

	tile=offset<<1;
	col_off=((tile&0x3f)*32)+((tile&0xfc0)>>6);

	tilemap_mark_tile_dirty(pixel_layer,col_off);
	tilemap_mark_tile_dirty(pixel_layer,col_off+32);
}

WRITE32_HANDLER( f3_vram_w )
{
	COMBINE_DATA(&f3_vram[offset]);
	vram_dirty[offset/8]=1;
	vram_changed=1;
}

WRITE32_HANDLER( f3_pivot_w )
{
	COMBINE_DATA(&f3_pivot_ram[offset]);
	pivot_dirty[offset/8]=1;
	pivot_changed=1;
}

WRITE32_HANDLER( f3_lineram_w )
{
	/* DariusGX has an interesting bug at the start of Round D - the clearing of lineram
    (0xa000->0x0xa7ff) overflows into priority RAM (0xb000) and creates garbage priority
    values.  I'm not sure what the real machine would do with these values, and this
    emulation certainly doesn't like it, so I've chosen to catch the bug here, and prevent
    the trashing of priority ram.  If anyone has information on what the real machine does,
    please let me know! */
	if (f3_game==DARIUSG) {
		if (f3_skip_this_frame)
			return;
		if (offset==0xb000/4 && data==0x003f0000) {
			f3_skip_this_frame=1;
			return;
		}
	}

	COMBINE_DATA(&f3_line_ram[offset]);
}

WRITE32_HANDLER( f3_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

	/* 12 bit palette games - there has to be a palette select bit somewhere */
	if (f3_game==SPCINVDX || f3_game==RIDINGF || f3_game==ARABIANM || f3_game==RINGRAGE) {
		b = 15 * ((paletteram32[offset] >> 4) & 0xf);
		g = 15 * ((paletteram32[offset] >> 8) & 0xf);
		r = 15 * ((paletteram32[offset] >> 12) & 0xf);
	}

	/* This is weird - why are only the sprites and VRAM palettes 21 bit? */
	else if (f3_game==CLEOPATR) {
		if (offset<0x100 || offset>0x1000) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* Another weird couple - perhaps this is alpha blending related? */
	else if (f3_game==TWINQIX || f3_game==RECALH) {
		if (offset>0x1c00) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* All other games - standard 24 bit palette */
	else {
	 	r = (paletteram32[offset] >>16) & 0xff;
		g = (paletteram32[offset] >> 8) & 0xff;
		b = (paletteram32[offset] >> 0) & 0xff;
	}

	palette_set_color(offset,r,g,b);
}

/******************************************************************************/

static UINT8 add_sat[256][256];

static const UINT8 *alpha_s_1_1;
static const UINT8 *alpha_s_1_2;
static const UINT8 *alpha_s_1_4;
static const UINT8 *alpha_s_1_5;
static const UINT8 *alpha_s_1_6;
static const UINT8 *alpha_s_1_8;
static const UINT8 *alpha_s_1_9;
static const UINT8 *alpha_s_1_a;

static const UINT8 *alpha_s_2a_0;
static const UINT8 *alpha_s_2a_4;
static const UINT8 *alpha_s_2a_8;

static const UINT8 *alpha_s_2b_0;
static const UINT8 *alpha_s_2b_4;
static const UINT8 *alpha_s_2b_8;

static const UINT8 *alpha_s_3a_0;
static const UINT8 *alpha_s_3a_1;
static const UINT8 *alpha_s_3a_2;

static const UINT8 *alpha_s_3b_0;
static const UINT8 *alpha_s_3b_1;
static const UINT8 *alpha_s_3b_2;

static UINT32 dval;
static UINT8 pval;
static UINT8 tval;
static UINT8 pdest_2a = 0x10;
static UINT8 pdest_2b = 0x20;
static int tr_2a = 0;
static int tr_2b = 1;
static UINT8 pdest_3a = 0x40;
static UINT8 pdest_3b = 0x80;
static int tr_3a = 0;
static int tr_3b = 1;

static int (*dpix_n[8][16])(UINT32 s_pix);
static int (**dpix_lp[5])(UINT32 s_pix);
static int (**dpix_sp[9])(UINT32 s_pix);

/*============================================================================*/

#define SET_ALPHA_LEVEL(d,s)			\
{										\
	int level = s;						\
	if(level == 0) level = -1;			\
	d = drawgfx_alpha_cache.alpha[level+1];		\
}

INLINE void f3_alpha_set_level(void)
{
//  SET_ALPHA_LEVEL(alpha_s_1_1, f3_alpha_level_2ad)
	SET_ALPHA_LEVEL(alpha_s_1_1, 255-f3_alpha_level_2as)
//  SET_ALPHA_LEVEL(alpha_s_1_2, f3_alpha_level_2bd)
	SET_ALPHA_LEVEL(alpha_s_1_2, 255-f3_alpha_level_2bs)
	SET_ALPHA_LEVEL(alpha_s_1_4, f3_alpha_level_3ad)
//  SET_ALPHA_LEVEL(alpha_s_1_5, f3_alpha_level_3ad*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_1_5, f3_alpha_level_3ad*(255-f3_alpha_level_2as)/255)
//  SET_ALPHA_LEVEL(alpha_s_1_6, f3_alpha_level_3ad*f3_alpha_level_2bd/255)
	SET_ALPHA_LEVEL(alpha_s_1_6, f3_alpha_level_3ad*(255-f3_alpha_level_2bs)/255)
	SET_ALPHA_LEVEL(alpha_s_1_8, f3_alpha_level_3bd)
//  SET_ALPHA_LEVEL(alpha_s_1_9, f3_alpha_level_3bd*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_1_9, f3_alpha_level_3bd*(255-f3_alpha_level_2as)/255)
//  SET_ALPHA_LEVEL(alpha_s_1_a, f3_alpha_level_3bd*f3_alpha_level_2bd/255)
	SET_ALPHA_LEVEL(alpha_s_1_a, f3_alpha_level_3bd*(255-f3_alpha_level_2bs)/255)

	SET_ALPHA_LEVEL(alpha_s_2a_0, f3_alpha_level_2as)
	SET_ALPHA_LEVEL(alpha_s_2a_4, f3_alpha_level_2as*f3_alpha_level_3ad/255)
	SET_ALPHA_LEVEL(alpha_s_2a_8, f3_alpha_level_2as*f3_alpha_level_3bd/255)

	SET_ALPHA_LEVEL(alpha_s_2b_0, f3_alpha_level_2bs)
	SET_ALPHA_LEVEL(alpha_s_2b_4, f3_alpha_level_2bs*f3_alpha_level_3ad/255)
	SET_ALPHA_LEVEL(alpha_s_2b_8, f3_alpha_level_2bs*f3_alpha_level_3bd/255)

	SET_ALPHA_LEVEL(alpha_s_3a_0, f3_alpha_level_3as)
	SET_ALPHA_LEVEL(alpha_s_3a_1, f3_alpha_level_3as*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_3a_2, f3_alpha_level_3as*f3_alpha_level_2bd/255)

	SET_ALPHA_LEVEL(alpha_s_3b_0, f3_alpha_level_3bs)
	SET_ALPHA_LEVEL(alpha_s_3b_1, f3_alpha_level_3bs*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_3b_2, f3_alpha_level_3bs*f3_alpha_level_2bd/255)
}
#undef SET_ALPHA_LEVEL

/*============================================================================*/

#ifdef LSB_FIRST
#define COLOR1 0
#define COLOR2 1
#define COLOR3 2
#else
#define COLOR1 3
#define COLOR2 2
#define COLOR3 1
#endif



INLINE void f3_alpha_blend32_s( const UINT8 *alphas, UINT32 s )
{
	UINT8 *sc = (UINT8 *)&s;
	UINT8 *dc = (UINT8 *)&dval;
	dc[COLOR1] = alphas[sc[COLOR1]];
	dc[COLOR2] = alphas[sc[COLOR2]];
	dc[COLOR3] = alphas[sc[COLOR3]];
}

INLINE void f3_alpha_blend32_d( const UINT8 *alphas, UINT32 s )
{
	UINT8 *sc = (UINT8 *)&s;
	UINT8 *dc = (UINT8 *)&dval;
	dc[COLOR1] = add_sat[dc[COLOR1]][alphas[sc[COLOR1]]];
	dc[COLOR2] = add_sat[dc[COLOR2]][alphas[sc[COLOR2]]];
	dc[COLOR3] = add_sat[dc[COLOR3]][alphas[sc[COLOR3]]];
}

/*============================================================================*/

INLINE void f3_alpha_blend_1_1( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_1,s);}
INLINE void f3_alpha_blend_1_2( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_2,s);}
INLINE void f3_alpha_blend_1_4( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_4,s);}
INLINE void f3_alpha_blend_1_5( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_5,s);}
INLINE void f3_alpha_blend_1_6( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_6,s);}
INLINE void f3_alpha_blend_1_8( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_8,s);}
INLINE void f3_alpha_blend_1_9( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_9,s);}
INLINE void f3_alpha_blend_1_a( UINT32 s ){f3_alpha_blend32_d(alpha_s_1_a,s);}

INLINE void f3_alpha_blend_2a_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_2a_0,s);}
INLINE void f3_alpha_blend_2a_4( UINT32 s ){f3_alpha_blend32_d(alpha_s_2a_4,s);}
INLINE void f3_alpha_blend_2a_8( UINT32 s ){f3_alpha_blend32_d(alpha_s_2a_8,s);}

INLINE void f3_alpha_blend_2b_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_2b_0,s);}
INLINE void f3_alpha_blend_2b_4( UINT32 s ){f3_alpha_blend32_d(alpha_s_2b_4,s);}
INLINE void f3_alpha_blend_2b_8( UINT32 s ){f3_alpha_blend32_d(alpha_s_2b_8,s);}

INLINE void f3_alpha_blend_3a_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_3a_0,s);}
INLINE void f3_alpha_blend_3a_1( UINT32 s ){f3_alpha_blend32_d(alpha_s_3a_1,s);}
INLINE void f3_alpha_blend_3a_2( UINT32 s ){f3_alpha_blend32_d(alpha_s_3a_2,s);}

INLINE void f3_alpha_blend_3b_0( UINT32 s ){f3_alpha_blend32_s(alpha_s_3b_0,s);}
INLINE void f3_alpha_blend_3b_1( UINT32 s ){f3_alpha_blend32_d(alpha_s_3b_1,s);}
INLINE void f3_alpha_blend_3b_2( UINT32 s ){f3_alpha_blend32_d(alpha_s_3b_2,s);}

/*============================================================================*/

static int dpix_1_noalpha(UINT32 s_pix) {dval = s_pix; return 1;}
static int dpix_ret1(UINT32 s_pix) {return 1;}
static int dpix_ret0(UINT32 s_pix) {return 0;}
static int dpix_1_1(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_1(s_pix); return 1;}
static int dpix_1_2(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_2(s_pix); return 1;}
static int dpix_1_4(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_4(s_pix); return 1;}
static int dpix_1_5(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_5(s_pix); return 1;}
static int dpix_1_6(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_6(s_pix); return 1;}
static int dpix_1_8(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_8(s_pix); return 1;}
static int dpix_1_9(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_9(s_pix); return 1;}
static int dpix_1_a(UINT32 s_pix) {if(s_pix) f3_alpha_blend_1_a(s_pix); return 1;}

static int dpix_2a_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2a_0(s_pix);
	else	  dval = 0;
	if(pdest_2a) {pval |= pdest_2a;return 0;}
	return 1;
}
static int dpix_2a_4(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2a_4(s_pix);
	if(pdest_2a) {pval |= pdest_2a;return 0;}
	return 1;
}
static int dpix_2a_8(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2a_8(s_pix);
	if(pdest_2a) {pval |= pdest_2a;return 0;}
	return 1;
}

static int dpix_3a_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3a_0(s_pix);
	else	  dval = 0;
	if(pdest_3a) {pval |= pdest_3a;return 0;}
	return 1;
}
static int dpix_3a_1(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3a_1(s_pix);
	if(pdest_3a) {pval |= pdest_3a;return 0;}
	return 1;
}
static int dpix_3a_2(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3a_2(s_pix);
	if(pdest_3a) {pval |= pdest_3a;return 0;}
	return 1;
}

static int dpix_2b_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2b_0(s_pix);
	else	  dval = 0;
	if(pdest_2b) {pval |= pdest_2b;return 0;}
	return 1;
}
static int dpix_2b_4(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2b_4(s_pix);
	if(pdest_2b) {pval |= pdest_2b;return 0;}
	return 1;
}
static int dpix_2b_8(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_2b_8(s_pix);
	if(pdest_2b) {pval |= pdest_2b;return 0;}
	return 1;
}

static int dpix_3b_0(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3b_0(s_pix);
	else	  dval = 0;
	if(pdest_3b) {pval |= pdest_3b;return 0;}
	return 1;
}
static int dpix_3b_1(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3b_1(s_pix);
	if(pdest_3b) {pval |= pdest_3b;return 0;}
	return 1;
}
static int dpix_3b_2(UINT32 s_pix)
{
	if(s_pix) f3_alpha_blend_3b_2(s_pix);
	if(pdest_3b) {pval |= pdest_3b;return 0;}
	return 1;
}

static int dpix_2_0(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_2b)		{f3_alpha_blend_2b_0(s_pix);if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{f3_alpha_blend_2a_0(s_pix);if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	else
	{
		if(tr2==tr_2b)		{dval = 0;if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{dval = 0;if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	return 0;
}
static int dpix_2_4(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_2b)		{f3_alpha_blend_2b_4(s_pix);if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{f3_alpha_blend_2a_4(s_pix);if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	else
	{
		if(tr2==tr_2b)		{if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	return 0;
}
static int dpix_2_8(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_2b)		{f3_alpha_blend_2b_8(s_pix);if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{f3_alpha_blend_2a_8(s_pix);if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	else
	{
		if(tr2==tr_2b)		{if(pdest_2b) pval |= pdest_2b;else return 1;}
		else if(tr2==tr_2a)	{if(pdest_2a) pval |= pdest_2a;else return 1;}
	}
	return 0;
}

static int dpix_3_0(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_3b)		{f3_alpha_blend_3b_0(s_pix);if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{f3_alpha_blend_3a_0(s_pix);if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	else
	{
		if(tr2==tr_3b)		{dval = 0;if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{dval = 0;if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	return 0;
}
static int dpix_3_1(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_3b)		{f3_alpha_blend_3b_1(s_pix);if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{f3_alpha_blend_3a_1(s_pix);if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	else
	{
		if(tr2==tr_3b)		{if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	return 0;
}
static int dpix_3_2(UINT32 s_pix)
{
	UINT8 tr2=tval&1;
	if(s_pix)
	{
		if(tr2==tr_3b)		{f3_alpha_blend_3b_2(s_pix);if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{f3_alpha_blend_3a_2(s_pix);if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	else
	{
		if(tr2==tr_3b)		{if(pdest_3b) pval |= pdest_3b;else return 1;}
		else if(tr2==tr_3a)	{if(pdest_3a) pval |= pdest_3a;else return 1;}
	}
	return 0;
}

INLINE void dpix_1_sprite(UINT32 s_pix)
{
	if(s_pix)
	{
		UINT8 p1 = pval&0xf0;
		if     (p1==0x10)	f3_alpha_blend_1_1(s_pix);
		else if(p1==0x20)	f3_alpha_blend_1_2(s_pix);
		else if(p1==0x40)	f3_alpha_blend_1_4(s_pix);
		else if(p1==0x50)	f3_alpha_blend_1_5(s_pix);
		else if(p1==0x60)	f3_alpha_blend_1_6(s_pix);
		else if(p1==0x80)	f3_alpha_blend_1_8(s_pix);
		else if(p1==0x90)	f3_alpha_blend_1_9(s_pix);
		else if(p1==0xa0)	f3_alpha_blend_1_a(s_pix);
	}
}

INLINE void dpix_bg(UINT32 bgcolor)
{
	UINT8 p1 = pval&0xf0;
	if(!p1)			dval = bgcolor;
	else if(p1==0x10)	f3_alpha_blend_1_1(bgcolor);
	else if(p1==0x20)	f3_alpha_blend_1_2(bgcolor);
	else if(p1==0x40)	f3_alpha_blend_1_4(bgcolor);
	else if(p1==0x50)	f3_alpha_blend_1_5(bgcolor);
	else if(p1==0x60)	f3_alpha_blend_1_6(bgcolor);
	else if(p1==0x80)	f3_alpha_blend_1_8(bgcolor);
	else if(p1==0x90)	f3_alpha_blend_1_9(bgcolor);
	else if(p1==0xa0)	f3_alpha_blend_1_a(bgcolor);
}

/******************************************************************************/

static void init_alpha_blend_func(void)
{
	int i,j;

	dpix_n[0][0x0]=dpix_1_noalpha;
	dpix_n[0][0x1]=dpix_1_noalpha;
	dpix_n[0][0x2]=dpix_1_noalpha;
	dpix_n[0][0x3]=dpix_1_noalpha;
	dpix_n[0][0x4]=dpix_1_noalpha;
	dpix_n[0][0x5]=dpix_1_noalpha;
	dpix_n[0][0x6]=dpix_1_noalpha;
	dpix_n[0][0x7]=dpix_1_noalpha;
	dpix_n[0][0x8]=dpix_1_noalpha;
	dpix_n[0][0x9]=dpix_1_noalpha;
	dpix_n[0][0xa]=dpix_1_noalpha;
	dpix_n[0][0xb]=dpix_1_noalpha;
	dpix_n[0][0xc]=dpix_1_noalpha;
	dpix_n[0][0xd]=dpix_1_noalpha;
	dpix_n[0][0xe]=dpix_1_noalpha;
	dpix_n[0][0xf]=dpix_1_noalpha;

	dpix_n[1][0x0]=dpix_1_noalpha;
	dpix_n[1][0x1]=dpix_1_1;
	dpix_n[1][0x2]=dpix_1_2;
	dpix_n[1][0x3]=dpix_ret1;
	dpix_n[1][0x4]=dpix_1_4;
	dpix_n[1][0x5]=dpix_1_5;
	dpix_n[1][0x6]=dpix_1_6;
	dpix_n[1][0x7]=dpix_ret1;
	dpix_n[1][0x8]=dpix_1_8;
	dpix_n[1][0x9]=dpix_1_9;
	dpix_n[1][0xa]=dpix_1_a;
	dpix_n[1][0xb]=dpix_ret1;
	dpix_n[1][0xc]=dpix_ret1;
	dpix_n[1][0xd]=dpix_ret1;
	dpix_n[1][0xe]=dpix_ret1;
	dpix_n[1][0xf]=dpix_ret1;

	dpix_n[2][0x0]=dpix_2a_0;
	dpix_n[2][0x1]=dpix_ret0;
	dpix_n[2][0x2]=dpix_ret0;
	dpix_n[2][0x3]=dpix_ret0;
	dpix_n[2][0x4]=dpix_2a_4;
	dpix_n[2][0x5]=dpix_ret0;
	dpix_n[2][0x6]=dpix_ret0;
	dpix_n[2][0x7]=dpix_ret0;
	dpix_n[2][0x8]=dpix_2a_8;
	dpix_n[2][0x9]=dpix_ret0;
	dpix_n[2][0xa]=dpix_ret0;
	dpix_n[2][0xb]=dpix_ret0;
	dpix_n[2][0xc]=dpix_ret0;
	dpix_n[2][0xd]=dpix_ret0;
	dpix_n[2][0xe]=dpix_ret0;
	dpix_n[2][0xf]=dpix_ret0;

	dpix_n[3][0x0]=dpix_3a_0;
	dpix_n[3][0x1]=dpix_3a_1;
	dpix_n[3][0x2]=dpix_3a_2;
	dpix_n[3][0x3]=dpix_ret0;
	dpix_n[3][0x4]=dpix_ret0;
	dpix_n[3][0x5]=dpix_ret0;
	dpix_n[3][0x6]=dpix_ret0;
	dpix_n[3][0x7]=dpix_ret0;
	dpix_n[3][0x8]=dpix_ret0;
	dpix_n[3][0x9]=dpix_ret0;
	dpix_n[3][0xa]=dpix_ret0;
	dpix_n[3][0xb]=dpix_ret0;
	dpix_n[3][0xc]=dpix_ret0;
	dpix_n[3][0xd]=dpix_ret0;
	dpix_n[3][0xe]=dpix_ret0;
	dpix_n[3][0xf]=dpix_ret0;

	dpix_n[4][0x0]=dpix_2b_0;
	dpix_n[4][0x1]=dpix_ret0;
	dpix_n[4][0x2]=dpix_ret0;
	dpix_n[4][0x3]=dpix_ret0;
	dpix_n[4][0x4]=dpix_2b_4;
	dpix_n[4][0x5]=dpix_ret0;
	dpix_n[4][0x6]=dpix_ret0;
	dpix_n[4][0x7]=dpix_ret0;
	dpix_n[4][0x8]=dpix_2b_8;
	dpix_n[4][0x9]=dpix_ret0;
	dpix_n[4][0xa]=dpix_ret0;
	dpix_n[4][0xb]=dpix_ret0;
	dpix_n[4][0xc]=dpix_ret0;
	dpix_n[4][0xd]=dpix_ret0;
	dpix_n[4][0xe]=dpix_ret0;
	dpix_n[4][0xf]=dpix_ret0;

	dpix_n[5][0x0]=dpix_3b_0;
	dpix_n[5][0x1]=dpix_3b_1;
	dpix_n[5][0x2]=dpix_3b_2;
	dpix_n[5][0x3]=dpix_ret0;
	dpix_n[5][0x4]=dpix_ret0;
	dpix_n[5][0x5]=dpix_ret0;
	dpix_n[5][0x6]=dpix_ret0;
	dpix_n[5][0x7]=dpix_ret0;
	dpix_n[5][0x8]=dpix_ret0;
	dpix_n[5][0x9]=dpix_ret0;
	dpix_n[5][0xa]=dpix_ret0;
	dpix_n[5][0xb]=dpix_ret0;
	dpix_n[5][0xc]=dpix_ret0;
	dpix_n[5][0xd]=dpix_ret0;
	dpix_n[5][0xe]=dpix_ret0;
	dpix_n[5][0xf]=dpix_ret0;

	dpix_n[6][0x0]=dpix_2_0;
	dpix_n[6][0x1]=dpix_ret0;
	dpix_n[6][0x2]=dpix_ret0;
	dpix_n[6][0x3]=dpix_ret0;
	dpix_n[6][0x4]=dpix_2_4;
	dpix_n[6][0x5]=dpix_ret0;
	dpix_n[6][0x6]=dpix_ret0;
	dpix_n[6][0x7]=dpix_ret0;
	dpix_n[6][0x8]=dpix_2_8;
	dpix_n[6][0x9]=dpix_ret0;
	dpix_n[6][0xa]=dpix_ret0;
	dpix_n[6][0xb]=dpix_ret0;
	dpix_n[6][0xc]=dpix_ret0;
	dpix_n[6][0xd]=dpix_ret0;
	dpix_n[6][0xe]=dpix_ret0;
	dpix_n[6][0xf]=dpix_ret0;

	dpix_n[7][0x0]=dpix_3_0;
	dpix_n[7][0x1]=dpix_3_1;
	dpix_n[7][0x2]=dpix_3_2;
	dpix_n[7][0x3]=dpix_ret0;
	dpix_n[7][0x4]=dpix_ret0;
	dpix_n[7][0x5]=dpix_ret0;
	dpix_n[7][0x6]=dpix_ret0;
	dpix_n[7][0x7]=dpix_ret0;
	dpix_n[7][0x8]=dpix_ret0;
	dpix_n[7][0x9]=dpix_ret0;
	dpix_n[7][0xa]=dpix_ret0;
	dpix_n[7][0xb]=dpix_ret0;
	dpix_n[7][0xc]=dpix_ret0;
	dpix_n[7][0xd]=dpix_ret0;
	dpix_n[7][0xe]=dpix_ret0;
	dpix_n[7][0xf]=dpix_ret0;

	for(i=0;i<256;i++)
		for(j=0;j<256;j++)
			add_sat[i][j] = (i + j < 256) ? i + j : 255;
}

/******************************************************************************/

#define GET_PIXMAP_POINTER(pf_num) \
{ \
	const struct f3_playfield_line_inf *line_tmp=line_t[pf_num]; \
	src##pf_num=line_tmp->src[y]; \
	src_s##pf_num=line_tmp->src_s[y]; \
	src_e##pf_num=line_tmp->src_e[y]; \
	tsrc##pf_num=line_tmp->tsrc[y]; \
	tsrc_s##pf_num=line_tmp->tsrc_s[y]; \
	x_count##pf_num=line_tmp->x_count[y]; \
	x_zoom##pf_num=line_tmp->x_zoom[y]; \
	clip_al##pf_num=line_tmp->clip0[y]&0xffff; \
	clip_ar##pf_num=line_tmp->clip0[y]>>16; \
	clip_bl##pf_num=line_tmp->clip1[y]&0xffff; \
	clip_br##pf_num=line_tmp->clip1[y]>>16; \
}

#define CULC_PIXMAP_POINTER(pf_num) \
{ \
	x_count##pf_num += x_zoom##pf_num; \
	if(x_count##pf_num>>16) \
	{ \
		x_count##pf_num &= 0xffff; \
		src##pf_num++; \
		tsrc##pf_num++; \
		if(src##pf_num==src_e##pf_num) {src##pf_num=src_s##pf_num; tsrc##pf_num=tsrc_s##pf_num;} \
	} \
}

/*============================================================================*/

INLINE void f3_drawscanlines(
		mame_bitmap *bitmap,int xsize,INT16 *draw_line_num,
		const struct f3_playfield_line_inf **line_t,
		const int *sprite,
		UINT32 orient,
		int skip_layer_num)
{
	pen_t *clut = &Machine->remapped_colortable[0];
	UINT32 bgcolor=clut[0];
	int length;

	const int x=46;

	UINT32 sprite_noalp_0=sprite[0]&0x100;
	UINT32 sprite_noalp_1=sprite[1]&0x100;
	UINT32 sprite_noalp_2=sprite[2]&0x100;
	UINT32 sprite_noalp_3=sprite[3]&0x100;
	UINT32 sprite_noalp_4=sprite[4]&0x100;
	UINT32 sprite_noalp_5=sprite[5]&0x100;

	static UINT16 *src0=0,*src_s0=0,*src_e0=0,clip_al0=0,clip_ar0=0,clip_bl0=0,clip_br0=0;
	static UINT8 *tsrc0=0,*tsrc_s0=0;
	static UINT32 x_count0=0,x_zoom0=0;

	static UINT16 *src1=0,*src_s1=0,*src_e1=0,clip_al1=0,clip_ar1=0,clip_bl1=0,clip_br1=0;
	static UINT8 *tsrc1=0,*tsrc_s1=0;
	static UINT32 x_count1=0,x_zoom1=0;

	static UINT16 *src2=0,*src_s2=0,*src_e2=0,clip_al2=0,clip_ar2=0,clip_bl2=0,clip_br2=0;
	static UINT8 *tsrc2=0,*tsrc_s2=0;
	static UINT32 x_count2=0,x_zoom2=0;

	static UINT16 *src3=0,*src_s3=0,*src_e3=0,clip_al3=0,clip_ar3=0,clip_bl3=0,clip_br3=0;
	static UINT8 *tsrc3=0,*tsrc_s3=0;
	static UINT32 x_count3=0,x_zoom3=0;

	static UINT16 *src4=0,*src_s4=0,*src_e4=0,clip_al4=0,clip_ar4=0,clip_bl4=0,clip_br4=0;
	static UINT8 *tsrc4=0,*tsrc_s4=0;
	static UINT32 x_count4=0,x_zoom4=0;

	UINT16 clip_als=0, clip_ars=0, clip_bls=0, clip_brs=0;

	UINT8 *dstp0,*dstp;

	int yadv = bitmap->rowpixels;
	int i=0,y=draw_line_num[0];
	int ty = y;

	if (orient & ORIENTATION_FLIP_Y)
	{
		ty = bitmap->height - 1 - ty;
		yadv = -yadv;
	}

	dstp0 = (UINT8 *)pri_alp_bitmap->line[ty] + x;

	pdest_2a = f3_alpha_level_2ad ? 0x10 : 0;
	pdest_2b = f3_alpha_level_2bd ? 0x20 : 0;
	tr_2a =(f3_alpha_level_2as==0 && f3_alpha_level_2ad==255) ? -1 : 0;
	tr_2b =(f3_alpha_level_2bs==0 && f3_alpha_level_2bd==255) ? -1 : 1;
	pdest_3a = f3_alpha_level_3ad ? 0x40 : 0;
	pdest_3b = f3_alpha_level_3bd ? 0x80 : 0;
	tr_3a =(f3_alpha_level_3as==0 && f3_alpha_level_3ad==255) ? -1 : 0;
	tr_3b =(f3_alpha_level_3bs==0 && f3_alpha_level_3bd==255) ? -1 : 1;

	{
		UINT32 *dsti0,*dsti;
		dsti0 = (UINT32 *)bitmap->line[ty] + x;
		while(1)
		{
			int cx=0;

			clip_als=sa_line_inf->sprite_clip0[y]&0xffff;
			clip_ars=sa_line_inf->sprite_clip0[y]>>16;
			clip_bls=sa_line_inf->sprite_clip1[y]&0xffff;
			clip_brs=sa_line_inf->sprite_clip1[y]>>16;

			length=xsize;
			dsti = dsti0;
			dstp = dstp0;

			switch(skip_layer_num)
			{
				case 0: GET_PIXMAP_POINTER(0)
				case 1: GET_PIXMAP_POINTER(1)
				case 2: GET_PIXMAP_POINTER(2)
				case 3: GET_PIXMAP_POINTER(3)
				case 4: GET_PIXMAP_POINTER(4)
			}

			while (1)
			{
				pval=*dstp;
				if (pval!=0xff)
				{
					UINT8 sprite_pri;
					switch(skip_layer_num)
					{
						case 0: if(cx>=clip_als && cx<clip_ars && !(cx>=clip_bls && cx<clip_brs) && (sprite_pri=sprite[0]&pval))
								{
									if(sprite_noalp_0) break;
									if(!dpix_sp[sprite_pri]) break;
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if (cx>=clip_al0 && cx<clip_ar0 && !(cx>=clip_bl0 && cx<clip_br0)) {tval=*tsrc0;if(tval&0xf0) if(dpix_lp[0][pval>>4](clut[*src0])) {*dsti=dval;break;}}
						case 1: if(cx>=clip_als && cx<clip_ars && !(cx>=clip_bls && cx<clip_brs) && (sprite_pri=sprite[1]&pval))
								{
									if(sprite_noalp_1) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if (cx>=clip_al1 && cx<clip_ar1 && !(cx>=clip_bl1 && cx<clip_br1)) {tval=*tsrc1;if(tval&0xf0) if(dpix_lp[1][pval>>4](clut[*src1])) {*dsti=dval;break;}}
						case 2: if(cx>=clip_als && cx<clip_ars && !(cx>=clip_bls && cx<clip_brs) && (sprite_pri=sprite[2]&pval))
								{
									if(sprite_noalp_2) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if (cx>=clip_al2 && cx<clip_ar2 && !(cx>=clip_bl2 && cx<clip_br2)) {tval=*tsrc2;if(tval&0xf0) if(dpix_lp[2][pval>>4](clut[*src2])) {*dsti=dval;break;}}
						case 3: if(cx>=clip_als && cx<clip_ars && !(cx>=clip_bls && cx<clip_brs) && (sprite_pri=sprite[3]&pval))
								{
									if(sprite_noalp_3) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if (cx>=clip_al3 && cx<clip_ar3 && !(cx>=clip_bl3 && cx<clip_br3)) {tval=*tsrc3;if(tval&0xf0) if(dpix_lp[3][pval>>4](clut[*src3])) {*dsti=dval;break;}}

						case 4: if(cx>=clip_als && cx<clip_ars && !(cx>=clip_bls && cx<clip_brs) && (sprite_pri=sprite[4]&pval))
								{
									if(sprite_noalp_4) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if (cx>=clip_al4 && cx<clip_ar4 && !(cx>=clip_bl4 && cx<clip_br4)) {
									tval=*tsrc4;
									if(tval&0xf0)
									{
										if(dpix_lp[4][pval>>4](clut[*src4]))
										{
											*dsti=dval;
											break;
										}
									}
								}


						case 5: if(cx>=clip_als && cx<clip_ars && !(cx>=clip_bls && cx<clip_brs) && (sprite_pri=sprite[5]&pval))
								{
									if(sprite_noalp_5) break;
									if(!dpix_sp[sprite_pri])
									{
										if(!(pval&0xf0)) break;
										else {dpix_1_sprite(*dsti);*dsti=dval;break;}
									}
									if(dpix_sp[sprite_pri][pval>>4](*dsti)) {*dsti=dval;break;}
								}
								if(!bgcolor) {if(!(pval&0xf0)) {*dsti=0;break;}}
								else dpix_bg(bgcolor);
								*dsti=dval;
					}
				}

				if(!(--length)) break;
				dsti++;
				dstp++;
				cx++;

				switch(skip_layer_num)
				{
					case 0: CULC_PIXMAP_POINTER(0)
					case 1: CULC_PIXMAP_POINTER(1)
					case 2: CULC_PIXMAP_POINTER(2)
					case 3: CULC_PIXMAP_POINTER(3)
					case 4: CULC_PIXMAP_POINTER(4)
				}
			}

			i++;
			if(draw_line_num[i]<0) break;
			if(draw_line_num[i]==y+1)
			{
				dsti0 += yadv;
				dstp0 += yadv;
				y++;
				continue;
			}
			else
			{
				int dy=(draw_line_num[i]-y)*yadv;
				dsti0 += dy;
				dstp0 += dy;
				y=draw_line_num[i];
			}
		}
	}
}
#undef GET_PIXMAP_POINTER
#undef CULC_PIXMAP_POINTER

/******************************************************************************/

static void visible_tile_check(struct f3_playfield_line_inf *line_t,
								int line,
								UINT32 x_index_fx,UINT32 y_index,
								UINT32 *f3_pf_data_n)
{
	UINT32 *pf_base;
	int i,trans_all,tile_index,tile_num;
	int alpha_type,alpha_mode;
	int opaque_all;
	int total_elements;

	if(!(alpha_mode=line_t->alpha_mode[line])) return;

	total_elements=Machine->gfx[1]->total_elements;

	tile_index=x_index_fx>>16;
	tile_num=(((line_t->x_zoom[line]*320+(x_index_fx & 0xffff)+0xffff)>>16)+(tile_index%16)+15)/16;
	tile_index/=16;

	if (flipscreen)
	{
		pf_base=f3_pf_data_n+((31-(y_index/16))<<twidth_mask_bit);
		tile_index=(twidth_mask-tile_index)-tile_num+1;
	}
	else pf_base=f3_pf_data_n+((y_index/16)<<twidth_mask_bit);


	trans_all=1;
	opaque_all=1;
	alpha_type=0;
	for(i=0;i<tile_num;i++)
	{
		UINT32 tile=pf_base[(tile_index)&twidth_mask];
		if(tile&0xffff)
		{
			trans_all=0;
			if(opaque_all)
			{
				if(tile_opaque_pf[(tile&0xffff)%total_elements]!=1) opaque_all=0;
			}

			if(alpha_mode==1)
			{
				if(!opaque_all) return;
			}
			else
			{
				if(alpha_type!=3)
				{
					if((tile>>(16+9))&1) alpha_type|=2;
					else				 alpha_type|=1;
				}
				else if(!opaque_all) break;
			}
		}
		else if(opaque_all) opaque_all=0;

		tile_index++;
	}

	if(trans_all)	{line_t->alpha_mode[line]=0;return;}

	if(alpha_mode>1)
	{
		line_t->alpha_mode[line]|=alpha_type<<4;
	}

	if(opaque_all)
		line_t->alpha_mode[line]|=0x80;
}

/******************************************************************************/

static void calculate_clip(int y, UINT16 pri, UINT32* clip0, UINT32* clip1, int* line_enable)
{
	const struct f3_spritealpha_line_inf *sa_line_t=&sa_line_inf[0];

	switch (pri)
	{
	case 0x0100: /* Clip plane 1 enable */
		{
			if (sa_line_t->clip0_l[y] > sa_line_t->clip0_r[y])
				*line_enable=0;
			else
				*clip0=(sa_line_t->clip0_l[y]) | (sa_line_t->clip0_r[y]<<16);
			*clip1=0;
		}
		break;
	case 0x0110: /* Clip plane 1 enable, inverted */
		{
			*clip1=(sa_line_t->clip0_l[y]) | (sa_line_t->clip0_r[y]<<16);
			*clip0=0x7fff0000;
		}
		break;
	case 0x0200: /* Clip plane 2 enable */
		{
			if (sa_line_t->clip1_l[y] > sa_line_t->clip1_r[y])
				*line_enable=0;
			else
				*clip0=(sa_line_t->clip1_l[y]) | (sa_line_t->clip1_r[y]<<16);
			*clip1=0;
		}
		break;
	case 0x0220: /* Clip plane 2 enable, inverted */
		{
			*clip1=(sa_line_t->clip1_l[y]) | (sa_line_t->clip1_r[y]<<16);
			*clip0=0x7fff0000;
		}
		break;
	case 0x0300: /* Clip plane 1 & 2 enable */
		{
			int clipl=0,clipr=0;

			if (sa_line_t->clip1_l[y] > sa_line_t->clip0_l[y])
				clipl=sa_line_t->clip1_l[y];
			else
				clipl=sa_line_t->clip0_l[y];

			if (sa_line_t->clip1_r[y] < sa_line_t->clip0_r[y])
				clipr=sa_line_t->clip1_r[y];
			else
				clipr=sa_line_t->clip0_r[y];

			if (clipl > clipr)
				*line_enable=0;
			else
				*clip0=(clipl) | (clipr<<16);
			*clip1=0;
		}
		break;
	case 0x0310: /* Clip plane 1 & 2 enable, plane 1 inverted */
		{
			if (sa_line_t->clip1_l[y] > sa_line_t->clip1_r[y])
				line_enable=0;
			else
				*clip0=(sa_line_t->clip1_l[y]) | (sa_line_t->clip1_r[y]<<16);

			*clip1=(sa_line_t->clip0_l[y]) | (sa_line_t->clip0_r[y]<<16);
		}
		break;
	case 0x0320: /* Clip plane 1 & 2 enable, plane 2 inverted */
		{
			if (sa_line_t->clip0_l[y] > sa_line_t->clip0_r[y])
				line_enable=0;
			else
				*clip0=(sa_line_t->clip0_l[y]) | (sa_line_t->clip0_r[y]<<16);

			*clip1=(sa_line_t->clip1_l[y]) | (sa_line_t->clip1_r[y]<<16);
		}
		break;
	case 0x0330: /* Clip plane 1 & 2 enable, both inverted */
		{
			int clipl=0,clipr=0;

			if (sa_line_t->clip1_l[y] < sa_line_t->clip0_l[y])
				clipl=sa_line_t->clip1_l[y];
			else
				clipl=sa_line_t->clip0_l[y];

			if (sa_line_t->clip1_r[y] > sa_line_t->clip0_r[y])
				clipr=sa_line_t->clip1_r[y];
			else
				clipr=sa_line_t->clip0_r[y];

			if (clipl > clipr)
				*line_enable=0;
			else
				*clip1=(clipl) | (clipr<<16);
			*clip0=0x7fff0000;
		}
		break;
	default:
		// ui_popup("Illegal clip mode");
		break;
	}
}

static void get_spritealphaclip_info(void)
{
	struct f3_spritealpha_line_inf *line_t=&sa_line_inf[0];

	int y,y_end,y_inc;

	int spri_base,clip_base_low,clip_base_high,inc;

	UINT16 spri=0;
	UINT16 sprite_clip=0;
	UINT16 clip0_low=0, clip0_high=0, clip1_low=0;
	int alpha_level=0;
	UINT16 sprite_alpha=0;

	if (flipscreen)
	{
		spri_base=0x77fe;
		clip_base_low=0x51fe;
		clip_base_high=0x45fe;
		inc=-2;
		y=255;
		y_end=-1;
		y_inc=-1;

	}
	else
	{
		spri_base=0x7600;
		clip_base_low=0x5000;
		clip_base_high=0x4400;
		inc=2;
		y=0;
		y_end=256;
		y_inc=1;
	}

	while(y!=y_end)
	{
		/* The zoom, column and row values can latch according to control ram */
		if (y&1)
		{
			if (f3_line_ram[0x080+(y>>1)]&1)
				clip0_low=(f3_line_ram[clip_base_low/4]>> 0)&0xffff;
			if (f3_line_ram[0x000+(y>>1)]&4)
				clip0_high=(f3_line_ram[clip_base_high/4]>> 0)&0xffff;
			if (f3_line_ram[0x080+(y>>1)]&2)
				clip1_low=(f3_line_ram[(clip_base_low+0x200)/4]>> 0)&0xffff;

			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x8)
				spri=f3_line_ram[spri_base/4]&0xffff;
			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x4)
				sprite_clip=f3_line_ram[(spri_base-0x200)/4]&0xffff;
			if (f3_line_ram[(0x0400/4)+(y>>1)]&0x1)
				sprite_alpha=f3_line_ram[(spri_base-0x1600)/4]&0xffff;
			if (f3_line_ram[(0x0400/4)+(y>>1)]&0x2)
				alpha_level=f3_line_ram[(spri_base-0x1400)/4]&0xffff;
		}
		else
		{
			if (f3_line_ram[0x080+(y>>1)]&0x10000)
				clip0_low=(f3_line_ram[clip_base_low/4]>>16)&0xffff;
			if (f3_line_ram[0x000+(y>>1)]&0x40000)
				clip0_high=(f3_line_ram[clip_base_high/4]>>16)&0xffff;
			if (f3_line_ram[0x080+(y>>1)]&0x20000)
				clip1_low=(f3_line_ram[(clip_base_low+0x200)/4]>>16)&0xffff;

			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x80000)
				spri=f3_line_ram[spri_base/4]>>16;
			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x40000)
				sprite_clip=f3_line_ram[(spri_base-0x200)/4]>>16;
			if (f3_line_ram[(0x0400/4)+(y>>1)]&0x10000)
				sprite_alpha=f3_line_ram[(spri_base-0x1600)/4]>>16;
			if (f3_line_ram[(0x0400/4)+(y>>1)]&0x20000)
				alpha_level=f3_line_ram[(spri_base-0x1400)/4]>>16;
		}

		line_t->alpha_level[y]=alpha_level;
		line_t->spri[y]=spri;
		line_t->sprite_alpha[y]=sprite_alpha;
		line_t->clip0_l[y]=((clip0_low&0xff)|((clip0_high&0x1000)>>4)) - 47;
		line_t->clip0_r[y]=(((clip0_low&0xff00)>>8)|((clip0_high&0x2000)>>5)) - 47;
		line_t->clip1_l[y]=((clip1_low&0xff)|((clip0_high&0x4000)>>6)) - 47;
		line_t->clip1_r[y]=(((clip1_low&0xff00)>>8)|((clip0_high&0x8000)>>7)) - 47;
		if (line_t->clip0_l[y]<0) line_t->clip0_l[y]=0;
		if (line_t->clip0_r[y]<0) line_t->clip0_r[y]=0;
		if (line_t->clip1_l[y]<0) line_t->clip1_l[y]=0;
		if (line_t->clip1_r[y]<0) line_t->clip1_r[y]=0;

		/* Evaluate sprite clipping */
		if (sprite_clip&0x080)
		{
			line_t->sprite_clip0[y]=0x7fff7fff;
			line_t->sprite_clip1[y]=0;
		}
		else if (sprite_clip&0x33)
		{
			int line_enable=1;
			calculate_clip(y, ((sprite_clip&0x33)<<4), &line_t->sprite_clip0[y], &line_t->sprite_clip1[y], &line_enable);
			if (line_enable==0)
				line_t->sprite_clip0[y]=0x7fff7fff;
		}
		else
		{
			line_t->sprite_clip0[y]=0x7fff0000;
			line_t->sprite_clip1[y]=0;
		}

		spri_base+=inc;
		clip_base_low+=inc;
		clip_base_high+=inc;
		y +=y_inc;
	}
}

/* sx and sy are 16.16 fixed point numbers */
static void get_line_ram_info(tilemap *tmap, int sx, int sy, int pos, UINT32 *f3_pf_data_n)
{
	struct f3_playfield_line_inf *line_t=&pf_line_inf[pos];
	const mame_bitmap *srcbitmap;
	const mame_bitmap *transbitmap;

	int y,y_start,y_end,y_inc;
	int line_base,zoom_base,col_base,pri_base,inc;

	int line_enable;
	int colscroll=0,x_offset=0,line_zoom=0;
	UINT32 _y_zoom[256];
	UINT16 pri=0;
	int bit_select0=0x10000<<pos;
	int bit_select1=1<<pos;

	int _colscroll[256];
	UINT32 _x_offset[256];
	int y_index_fx;

	sx+=((46<<16));

	if (flipscreen)
	{
		line_base=0xa1fe + (pos*0x200);
		zoom_base=0x81fe;// + (pos*0x200);
		col_base =0x41fe + (pos*0x200);
		pri_base =0xb1fe + (pos*0x200);
		inc=-2;
		y_start=255;
		y_end=-1;
		y_inc=-1;

		if (f3_game_config->extend)	sx=-sx+((188-512)<<16); else sx=-sx+(188<<16); /* Adjust for flipped scroll position */
		y_index_fx=-sy-(256<<16); /* Adjust for flipped scroll position */
	}
	else
	{
		line_base=0xa000 + (pos*0x200);
		zoom_base=0x8000;// + (pos*0x200);
		col_base =0x4000 + (pos*0x200);
		pri_base =0xb000 + (pos*0x200);
		inc=2;
		y_start=0;
		y_end=256;
		y_inc=1;

		y_index_fx=sy;
	}

	y=y_start;
	while(y!=y_end)
	{
		/* The zoom, column and row values can latch according to control ram */
		if (y&1)
		{
			if (f3_line_ram[0x300+(y>>1)]&bit_select1)
				x_offset=(f3_line_ram[line_base/4]&0xffff)<<10;
			if (f3_line_ram[0x380+(y>>1)]&bit_select1)
				pri=f3_line_ram[pri_base/4]&0xffff;

			// Zoom for playfields 1 & 3 is interleaved, as is the latch select
			switch (pos)
			{
			case 0:
				if (f3_line_ram[0x200+(y>>1)]&bit_select1)
					line_zoom=f3_line_ram[(zoom_base+0x000)/4]&0xffff;
				break;
			case 1:
				if (f3_line_ram[0x200+(y>>1)]&0x2)
					line_zoom=((f3_line_ram[(zoom_base+0x200)/4]&0xffff)&0xff00) | (line_zoom&0x00ff);
				if (f3_line_ram[0x200+(y>>1)]&0x8)
					line_zoom=((f3_line_ram[(zoom_base+0x600)/4]&0xffff)&0x00ff) | (line_zoom&0xff00);
				break;
			case 2:
				if (f3_line_ram[0x200+(y>>1)]&bit_select1)
					line_zoom=f3_line_ram[(zoom_base+0x400)/4]&0xffff;
				break;
			case 3:
				if (f3_line_ram[0x200+(y>>1)]&0x8)
					line_zoom=((f3_line_ram[(zoom_base+0x600)/4]&0xffff)&0xff00) | (line_zoom&0x00ff);
				if (f3_line_ram[0x200+(y>>1)]&0x2)
					line_zoom=((f3_line_ram[(zoom_base+0x200)/4]&0xffff)&0x00ff) | (line_zoom&0xff00);
				break;
			default:
				break;
			}

			// Column scroll only affects playfields 2 & 3
			if (pos>=2 && f3_line_ram[0x000+(y>>1)]&bit_select1)
				colscroll=(f3_line_ram[col_base/4]>> 0)&0x3ff;
		}
		else
		{
			if (f3_line_ram[0x300+(y>>1)]&bit_select0)
				x_offset=(f3_line_ram[line_base/4]&0xffff0000)>>6;

			if (f3_line_ram[0x380+(y>>1)]&bit_select0)
				pri=(f3_line_ram[pri_base/4]>>16)&0xffff;

			// Zoom for playfields 1 & 3 is interleaved, as is the latch select
			switch (pos)
			{
			case 0:
				if (f3_line_ram[0x200+(y>>1)]&bit_select0)
					line_zoom=f3_line_ram[(zoom_base+0x000)/4]>>16;
				break;
			case 1:
				if (f3_line_ram[0x200+(y>>1)]&0x20000)
					line_zoom=((f3_line_ram[(zoom_base+0x200)/4]>>16)&0xff00) | (line_zoom&0x00ff);
				if (f3_line_ram[0x200+(y>>1)]&0x80000)
					line_zoom=((f3_line_ram[(zoom_base+0x600)/4]>>16)&0x00ff) | (line_zoom&0xff00);
				break;
			case 2:
				if (f3_line_ram[0x200+(y>>1)]&bit_select0)
					line_zoom=f3_line_ram[(zoom_base+0x400)/4]>>16;
				break;
			case 3:
				if (f3_line_ram[0x200+(y>>1)]&0x80000)
					line_zoom=((f3_line_ram[(zoom_base+0x600)/4]>>16)&0xff00) | (line_zoom&0x00ff);
				if (f3_line_ram[0x200+(y>>1)]&0x20000)
					line_zoom=((f3_line_ram[(zoom_base+0x200)/4]>>16)&0x00ff) | (line_zoom&0xff00);
				break;
			default:
				break;
			}

			// Column scroll only affects playfields 2 & 3
			if (pos>=2 && f3_line_ram[0x000+(y>>1)]&bit_select0)
				colscroll=(f3_line_ram[col_base/4]>>16)&0x3ff;
		}

		if (!pri || (!flipscreen && y<24) || (flipscreen && y>231) ||
			(pri&0xc000)==0xc000 || !(pri&0x2000)/**/)
 			line_enable=0;
		else if(pri&0x4000)	//alpha1
			line_enable=2;
		else if(pri&0x8000)	//alpha2
			line_enable=3;
		/* special case when the blend mode is "normal" but the 6200 area is used. Might be missing a flag */
		else if((pri&0x3000) && (f3_line_ram[0x6230/4]!= 0)  && (pos == 2) &&
		  (((f3_line_ram[(0x6200/4) + (y>>1)] >> 4) &0xf) != 0xb) && (f3_game == EACTION2))
		  {
			line_enable=0x22;		
		  }
		else
			line_enable=1;

		_colscroll[y]=colscroll;
		_x_offset[y]=(x_offset&0xffff0000) - (x_offset&0x0000ffff);
		_y_zoom[y] = (line_zoom&0xff) << 9;

		/* The football games use values in the range 0x200-0x3ff where the crowd should be drawn - !?

            Until this is understood we disable those lines (which would usually wrap and show the grass
            area.
        */
		if (colscroll&0x200)
			pri|=0x0800;

		/* Evaluate clipping */
		if (pri&0x0800)
			line_enable=0;
		else if (pri&0x0330)
		{
			//fast path todo - remove line enable
			calculate_clip(y, pri&0x0330, &line_t->clip0[y], &line_t->clip1[y], &line_enable);
		}
		else
		{
			/* No clipping */
			line_t->clip0[y]=0x7fff0000;
			line_t->clip1[y]=0;
		}

		line_t->x_zoom[y]=0x10000 - (line_zoom&0xff00);
		line_t->alpha_mode[y]=line_enable;
		line_t->pri[y]=pri;

		zoom_base+=inc;
		line_base+=inc;
		col_base +=inc;
		pri_base +=inc;
		y +=y_inc;
	}

	/* set pixmap pointer */
	srcbitmap = tilemap_get_pixmap(tmap);
	transbitmap = tilemap_get_transparency_bitmap(tmap);

	y=y_start;
	while(y!=y_end)
	{
		UINT32 x_index_fx;
		UINT32 y_index;

		if(line_t->alpha_mode[y]!=0)
		{
			UINT16 *src_s;
			UINT8 *tsrc_s;

			x_index_fx = (sx+_x_offset[y]-(10*0x10000)+(10*line_t->x_zoom[y]))&((width_mask<<16)|0xffff);
			y_index = ((y_index_fx>>16)+_colscroll[y])&0x1ff;

			/* check tile status */
			visible_tile_check(line_t,y,x_index_fx,y_index,f3_pf_data_n);

			/* If clipping enabled for this line have to disable 'all opaque' optimisation */
			if (line_t->clip0[y]!=0x7fff0000 || line_t->clip1[y]!=0)
				line_t->alpha_mode[y]&=~0x80;
			
			if ((pos ==1) && ((((f3_line_ram[(0x6200/4) + (y>>1)]) >> 4) &0xf) > 0xb)  && (f3_game == EACTION2)) line_t->alpha_mode[y] = 0x22;  /*hack*/

			/* set pixmap index */
			line_t->x_count[y]=x_index_fx & 0xffff; // Fractional part
			line_t->src_s[y]=src_s=(unsigned short *)srcbitmap->line[y_index];
			line_t->src_e[y]=&src_s[width_mask+1];
			line_t->src[y]=&src_s[x_index_fx>>16];

			line_t->tsrc_s[y]=tsrc_s=(unsigned char *)transbitmap->line[y_index];
			line_t->tsrc[y]=&tsrc_s[x_index_fx>>16];
		}

		y_index_fx += _y_zoom[y];
		y +=y_inc;
	}
}

static void get_vram_info(tilemap *vram_tilemap, tilemap *pixel_tilemap, int sx, int sy)
{
	const struct f3_spritealpha_line_inf *sprite_alpha_line_t=&sa_line_inf[0];
	struct f3_playfield_line_inf *line_t=&pf_line_inf[4];
	const mame_bitmap *srcbitmap_pixel, *srcbitmap_vram;
	const mame_bitmap *transbitmap_pixel, *transbitmap_vram;

	int y,y_start,y_end,y_inc;
	int pri_base,inc;

	int line_enable;

	UINT16 pri=0;

	const int vram_width_mask=0x1ff;

	if (flipscreen)
	{
		pri_base =0x73fe;
		inc=-2;
		y_start=255;
		y_end=-1;
		y_inc=-1;
	}
	else
	{
		pri_base =0x7200;
		inc=2;
		y_start=0;
		y_end=256;
		y_inc=1;

	}

	y=y_start;
	while(y!=y_end)
	{
		/* The zoom, column and row values can latch according to control ram */
		if (y&1)
		{
			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x2)
				pri=(f3_line_ram[pri_base/4]&0xffff);
		}
		else
		{
			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x20000)
				pri=(f3_line_ram[pri_base/4]&0xffff0000)>>16;
		}

		if (!pri || (!flipscreen && y<24) || (flipscreen && y>231) ||
			(pri&0xc000)==0xc000 || !(pri&0x2000)/**/)
 			line_enable=0;
		else if(pri&0x4000)	//alpha1
			line_enable=2;
		else if(pri&0x8000)	//alpha2
			line_enable=3;
		else
			line_enable=1;

		line_t->pri[y]=pri;

		/* Evaluate clipping */
		if (pri&0x0800)
			line_enable=0;
		else if (pri&0x0330)
		{
			//fast path todo - remove line enable
			calculate_clip(y, pri&0x0330, &line_t->clip0[y], &line_t->clip1[y], &line_enable);
		}
		else
		{
			/* No clipping */
			line_t->clip0[y]=0x7fff0000;
			line_t->clip1[y]=0;
		}

		line_t->x_zoom[y]=0x10000;
		line_t->alpha_mode[y]=line_enable;
		if (line_t->alpha_mode[y]>1)
			line_t->alpha_mode[y]|=0x10;

		pri_base +=inc;
		y +=y_inc;
	}

	sx&=0x1ff;

	/* set pixmap pointer */
	srcbitmap_pixel = tilemap_get_pixmap(pixel_tilemap);
	transbitmap_pixel = tilemap_get_transparency_bitmap(pixel_tilemap);
	srcbitmap_vram = tilemap_get_pixmap(vram_tilemap);
	transbitmap_vram = tilemap_get_transparency_bitmap(vram_tilemap);

	y=y_start;
	while(y!=y_end)
	{
		if(line_t->alpha_mode[y]!=0)
		{
			UINT16 *src_s;
			UINT8 *tsrc_s;

			// These bits in control ram indicate whether the line is taken from
			// the VRAM tilemap layer or pixel layer.
			const int usePixelLayer=((sprite_alpha_line_t->sprite_alpha[y]&0xa000)==0xa000);

			/* set pixmap index */
			line_t->x_count[y]=0xffff;
			if (usePixelLayer)
				line_t->src_s[y]=src_s=(unsigned short *)srcbitmap_pixel->line[sy&0xff];
			else
				line_t->src_s[y]=src_s=(unsigned short *)srcbitmap_vram->line[sy&0x1ff];
			line_t->src_e[y]=&src_s[vram_width_mask+1];
			line_t->src[y]=&src_s[sx];

			if (usePixelLayer)
				line_t->tsrc_s[y]=tsrc_s=(unsigned char *)transbitmap_pixel->line[sy&0xff];
			else
				line_t->tsrc_s[y]=tsrc_s=(unsigned char *)transbitmap_vram->line[sy&0x1ff];
			line_t->tsrc[y]=&tsrc_s[sx];
		}

		sy++;
		y += y_inc;
	}
}

/******************************************************************************/

static void f3_scanline_draw(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int i,j,y,ys,ye;
	int y_start,y_end,y_start_next,y_end_next;
	UINT8 draw_line[256];
	INT16 draw_line_num[256];

	UINT32 rot=0;

	if (flipscreen)
	{
		rot=ORIENTATION_FLIP_Y;
		ys=0;
		ye=232;
	}
	else
	{
		ys=24;
		ye=256;
	}

	y_start=ys;
	y_end=ye;
	memset(draw_line,0,256);

	while(1)
	{
		static int alpha_level_last=-1;
		int pos;
		int pri[5],alpha_mode[5],alpha_mode_flag[5],alpha_level;
		UINT16 sprite_alpha;
		UINT8 sprite_alpha_check;
		UINT8 sprite_alpha_all_2a;
		int spri;
		int alpha;
		int layer_tmp[5];

		int count_skip_layer=0;
		int sprite[6]={0,0,0,0,0,0};
		const struct f3_playfield_line_inf *line_t[5];


		/* find same status of scanlines */
		pri[0]=pf_line_inf[0].pri[y_start];
		pri[1]=pf_line_inf[1].pri[y_start];
		pri[2]=pf_line_inf[2].pri[y_start];
		pri[3]=pf_line_inf[3].pri[y_start];
		pri[4]=pf_line_inf[4].pri[y_start];
		alpha_mode[0]=pf_line_inf[0].alpha_mode[y_start];
		alpha_mode[1]=pf_line_inf[1].alpha_mode[y_start];
		alpha_mode[2]=pf_line_inf[2].alpha_mode[y_start];
		alpha_mode[3]=pf_line_inf[3].alpha_mode[y_start];
		alpha_mode[4]=pf_line_inf[4].alpha_mode[y_start];
		alpha_level=sa_line_inf[0].alpha_level[y_start];
		spri=sa_line_inf[0].spri[y_start];
		sprite_alpha=sa_line_inf[0].sprite_alpha[y_start];

		draw_line[y_start]=1;
		draw_line_num[i=0]=y_start;
		y_start_next=-1;
		y_end_next=-1;
		for(y=y_start+1;y<y_end;y++)
		{
			if(!draw_line[y])
			{
				if(pri[0]!=pf_line_inf[0].pri[y]) y_end_next=y+1;
				else if(pri[1]!=pf_line_inf[1].pri[y]) y_end_next=y+1;
				else if(pri[2]!=pf_line_inf[2].pri[y]) y_end_next=y+1;
				else if(pri[3]!=pf_line_inf[3].pri[y]) y_end_next=y+1;
				else if(pri[4]!=pf_line_inf[4].pri[y]) y_end_next=y+1;
				else if(alpha_mode[0]!=pf_line_inf[0].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[1]!=pf_line_inf[1].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[2]!=pf_line_inf[2].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[3]!=pf_line_inf[3].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_mode[4]!=pf_line_inf[4].alpha_mode[y]) y_end_next=y+1;
				else if(alpha_level!=sa_line_inf[0].alpha_level[y]) y_end_next=y+1;
				else if(spri!=sa_line_inf[0].spri[y]) y_end_next=y+1;
				else if(sprite_alpha!=sa_line_inf[0].sprite_alpha[y]) y_end_next=y+1;
				else
				{
					draw_line[y]=1;
					draw_line_num[++i]=y;
					continue;
				}

				if(y_start_next<0) y_start_next=y;
			}
		}
		y_end=y_end_next;
		y_start=y_start_next;
		draw_line_num[++i]=-1;

		/* alpha blend */
		alpha_mode_flag[0]=alpha_mode[0]&~3;
		alpha_mode_flag[1]=alpha_mode[1]&~3;
		alpha_mode_flag[2]=alpha_mode[2]&~3;
		alpha_mode_flag[3]=alpha_mode[3]&~3;
		alpha_mode_flag[4]=alpha_mode[4]&~3;
		alpha_mode[0]&=3;
		alpha_mode[1]&=3;
		alpha_mode[2]&=3;
		alpha_mode[3]&=3;
		alpha_mode[4]&=3;
		if( alpha_mode[0]>1 ||
			alpha_mode[1]>1 ||
			alpha_mode[2]>1 ||
			alpha_mode[3]>1 ||
			alpha_mode[4]>1 ||
			(sprite_alpha&0xff) != 0xff  )
		{
			/* set alpha level */
			if(alpha_level!=alpha_level_last)
			{
				int al_s,al_d;
				int a=alpha_level;
				int b=(a>>8)&0xf;
				int c=(a>>4)&0xf;
				int d=(a>>0)&0xf;
				a>>=12;

				/* b000 7000 */
				al_s = ( (15-d)*256) / 8;
				al_d = ( (15-b)*256) / 8;
				if(al_s>255) al_s = 255;
				if(al_d>255) al_d = 255;
				f3_alpha_level_3as = al_s;
				f3_alpha_level_3ad = al_d;
				f3_alpha_level_2as = al_d;
				f3_alpha_level_2ad = al_s;

				al_s = ( (15-c)*256) / 8;
				al_d = ( (15-a)*256) / 8;
				if(al_s>255) al_s = 255;
				if(al_d>255) al_d = 255;
				f3_alpha_level_3bs = al_s;
				f3_alpha_level_3bd = al_d;
				f3_alpha_level_2bs = al_d;
				f3_alpha_level_2bd = al_s;

				f3_alpha_set_level();
				alpha_level_last=alpha_level;
			}

			/* set sprite alpha mode */
			sprite_alpha_check=0;
			sprite_alpha_all_2a=1;
			dpix_sp[1]=0;
			dpix_sp[2]=0;
			dpix_sp[4]=0;
			dpix_sp[8]=0;
			for(i=0;i<4;i++)	/* i = sprite priority offset */
			{
				UINT8 sprite_alpha_mode=(sprite_alpha>>(i*2))&3;
				UINT8 sftbit=1<<i;
				if(sprite_pri_usage&sftbit)
				{
					if(sprite_alpha_mode==1)
					{
						if(f3_alpha_level_2as==0 && f3_alpha_level_2ad==255)
							sprite_pri_usage&=~sftbit;  // Disable sprite priority block
						else
						{
							dpix_sp[1<<i]=dpix_n[2];
							sprite_alpha_check|=sftbit;
						}
					}
					else if(sprite_alpha_mode==2)
					{
						if(sprite_alpha&0xff00)
						{
							if(f3_alpha_level_3as==0 && f3_alpha_level_3ad==255) sprite_pri_usage&=~sftbit;
							else
							{
								dpix_sp[1<<i]=dpix_n[3];
								sprite_alpha_check|=sftbit;
								sprite_alpha_all_2a=0;
							}
						}
						else
						{
							if(f3_alpha_level_3bs==0 && f3_alpha_level_3bd==255) sprite_pri_usage&=~sftbit;
							else
							{
								dpix_sp[1<<i]=dpix_n[5];
								sprite_alpha_check|=sftbit;
								sprite_alpha_all_2a=0;
							}
						}
					}
				}
			}


			/* check alpha level */
			for(i=0;i<5;i++)	/* i = playfield num (pos) */
			{
				int alpha_type = (alpha_mode_flag[i]>>4)&3;

				if(alpha_mode[i]==2)
				{
					if(alpha_type==1)
					{
						if     (f3_alpha_level_2as==0   && f3_alpha_level_2ad==255) alpha_mode[i]=0;
						else if(f3_alpha_level_2as==255 && f3_alpha_level_2ad==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==2)
					{
						if     (f3_alpha_level_2bs==0   && f3_alpha_level_2bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_2as==255 && f3_alpha_level_2ad==0 &&
								f3_alpha_level_2bs==255 && f3_alpha_level_2bd==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==3)
					{
						if     (f3_alpha_level_2as==0   && f3_alpha_level_2ad==255 &&
								f3_alpha_level_2bs==0   && f3_alpha_level_2bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_2as==255 && f3_alpha_level_2ad==0   &&
								f3_alpha_level_2bs==255 && f3_alpha_level_2bd==0  ) alpha_mode[i]=1;
					}
				}
				else if(alpha_mode[i]==3)
				{
					if(alpha_type==1)
					{
						if     (f3_alpha_level_3as==0   && f3_alpha_level_3ad==255) alpha_mode[i]=0;
						else if(f3_alpha_level_3as==255 && f3_alpha_level_3ad==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==2)
					{
						if     (f3_alpha_level_3bs==0   && f3_alpha_level_3bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_3as==255 && f3_alpha_level_3ad==0 &&
								f3_alpha_level_3bs==255 && f3_alpha_level_3bd==0  ) alpha_mode[i]=1;
					}
					else if(alpha_type==3)
					{
						if     (f3_alpha_level_3as==0   && f3_alpha_level_3ad==255 &&
								f3_alpha_level_3bs==0   && f3_alpha_level_3bd==255) alpha_mode[i]=0;
						else if(f3_alpha_level_3as==255 && f3_alpha_level_3ad==0   &&
								f3_alpha_level_3bs==255 && f3_alpha_level_3bd==0  ) alpha_mode[i]=1;
					}
				}
			}

			if (	(alpha_mode[0]==1 || alpha_mode[0]==2 || !alpha_mode[0]) &&
					(alpha_mode[1]==1 || alpha_mode[1]==2 || !alpha_mode[1]) &&
					(alpha_mode[2]==1 || alpha_mode[2]==2 || !alpha_mode[2]) &&
					(alpha_mode[3]==1 || alpha_mode[3]==2 || !alpha_mode[3]) &&
					(alpha_mode[4]==1 || alpha_mode[4]==2 || !alpha_mode[4]) &&
					sprite_alpha_all_2a						)
			{
				int alpha_type = (alpha_mode_flag[0] | alpha_mode_flag[1] | alpha_mode_flag[2] | alpha_mode_flag[3])&0x30;
				if(		(alpha_type==0x10 && f3_alpha_level_2as==255) ||
						(alpha_type==0x20 && f3_alpha_level_2as==255 && f3_alpha_level_2bs==255) ||
						(alpha_type==0x30 && f3_alpha_level_2as==255 && f3_alpha_level_2bs==255)	)
				{
					if(alpha_mode[0]>1) alpha_mode[0]=1;
					if(alpha_mode[1]>1) alpha_mode[1]=1;
					if(alpha_mode[2]>1) alpha_mode[2]=1;
					if(alpha_mode[3]>1) alpha_mode[3]=1;
					if(alpha_mode[4]>1) alpha_mode[4]=1;
					sprite_alpha_check=0;
					dpix_sp[1]=0;
					dpix_sp[2]=0;
					dpix_sp[4]=0;
					dpix_sp[8]=0;
				}
			}
		}
		else
		{
			sprite_alpha_check=0;
			dpix_sp[1]=0;
			dpix_sp[2]=0;
			dpix_sp[4]=0;
			dpix_sp[8]=0;
		}



		/* set scanline priority */
		{
			int pri_max_opa=-1;
			for(i=0;i<5;i++)	/* i = playfield num (pos) */
			{
				int p0=pri[i];
				int pri_sl1=p0&0x0f;

				layer_tmp[i]=i + (pri_sl1<<3);

				if(!alpha_mode[i])
				{
					layer_tmp[i]|=0x80;
					count_skip_layer++;
				}
				else if(alpha_mode[i]==1 && (alpha_mode_flag[i]&0x80))
				{
					if(layer_tmp[i]>pri_max_opa) pri_max_opa=layer_tmp[i];
				}
			}

			if(pri_max_opa!=-1)
			{
				if(pri_max_opa>layer_tmp[0]) {layer_tmp[0]|=0x80;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[1]) {layer_tmp[1]|=0x80;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[2]) {layer_tmp[2]|=0x80;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[3]) {layer_tmp[3]|=0x80;count_skip_layer++;}
				if(pri_max_opa>layer_tmp[4]) {layer_tmp[4]|=0x80;count_skip_layer++;}
			}
		}


		/* sort layer_tmp */
		for(i=0;i<4;i++)
		{
			for(j=i+1;j<5;j++)
			{
				if(layer_tmp[i]<layer_tmp[j])
				{
					int temp = layer_tmp[i];
					layer_tmp[i] = layer_tmp[j];
					layer_tmp[j] = temp;
				}
			}
		}


		/* check sprite & layer priority */
		{
			int l0,l1,l2,l3,l4;
			int pri_sp[5];

			l0=layer_tmp[0]>>3;
			l1=layer_tmp[1]>>3;
			l2=layer_tmp[2]>>3;
			l3=layer_tmp[3]>>3;
			l4=layer_tmp[4]>>3;

			pri_sp[0]=spri&0xf;
			pri_sp[1]=(spri>>4)&0xf;
			pri_sp[2]=(spri>>8)&0xf;
			pri_sp[3]=spri>>12;

			for(i=0;i<4;i++)	/* i = sprite priority offset */
			{
				int sp,sflg=1<<i;
				if(!(sprite_pri_usage & sflg)) continue;
				sp=pri_sp[i];

				/*
                    sprite priority==playfield priority
                        BUBSYMPH (title)       ---> sprite
                        DARIUSG (ZONE V' BOSS) ---> playfield
                */

				if (f3_game == BUBSYMPH ) sp++;		//BUBSYMPH (title)

					 if(		  sp>l0) sprite[0]|=sflg;
				else if(sp<=l0 && sp>l1) sprite[1]|=sflg;
				else if(sp<=l1 && sp>l2) sprite[2]|=sflg;
				else if(sp<=l2 && sp>l3) sprite[3]|=sflg;
				else if(sp<=l3 && sp>l4) sprite[4]|=sflg;
				else if(sp<=l4		   ) sprite[5]|=sflg;
			}
		}


		/* draw scanlines */
		alpha=0;
		for(i=count_skip_layer;i<5;i++)
		{
			pos=layer_tmp[i]&7;
			line_t[i]=&pf_line_inf[pos];

			if(sprite[i]&sprite_alpha_check) alpha=1;
			else if(!alpha) sprite[i]|=0x100;

			if(alpha_mode[pos]>1)
			{
				int alpha_type=(((alpha_mode_flag[pos]>>4)&3)-1)*2;
				dpix_lp[i]=dpix_n[alpha_mode[pos]+alpha_type];
				alpha=1;
			}
			else
			{
				if(alpha) dpix_lp[i]=dpix_n[1];
				else	  dpix_lp[i]=dpix_n[0];
			}
		}
		if(sprite[5]&sprite_alpha_check) alpha=1;
		else if(!alpha) sprite[5]|=0x100;

		f3_drawscanlines(bitmap,320,draw_line_num,line_t,sprite,rot,count_skip_layer);
		if(y_start<0) break;
	}
}

/******************************************************************************/

#define PSET_T					\
	c = *source;				\
	if(c)						\
	{							\
		p=*pri;					\
		if(!p || p==0xff)		\
		{						\
			*dest = pal[c];		\
			*pri = pri_dst;		\
		}						\
	}

#define PSET_O					\
	p=*pri;						\
	if(!p || p==0xff)			\
	{							\
		*dest = pal[*source];	\
		*pri = pri_dst;			\
	}

#define NEXT_P					\
	source += dx;				\
	dest++;						\
	pri++;

INLINE void f3_drawgfx( mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code,
		unsigned int color,
		int flipx,int flipy,
		int sx,int sy,
		const rectangle *clip,
		UINT8 pri_dst)
{
	rectangle myclip;

	pri_dst=1<<pri_dst;

	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}


	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
		int source_base = (code % gfx->total_elements) * 16;

		{
			/* compute sprite increment per screen pixel */
			int dx = 1;
			int dy = 1;

			int ex = sx+16;
			int ey = sy+16;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = 15;
				dx = -1;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = 15;
				dy = -1;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx && ey>sy)
			{ /* skip if inner loop doesn't draw anything */
//              if (dest_bmp->depth == 32)
				{
					int y=ey-sy;
					int x=(ex-sx-1)|(tile_opaque_sp[code % gfx->total_elements]<<4);
					UINT8 *source0 = gfx->gfxdata + (source_base+y_index) * 16 + x_index_base;
					UINT32 *dest0 = (UINT32 *)dest_bmp->line[sy]+sx;
					UINT8 *pri0 = (UINT8 *)pri_alp_bitmap->line[sy]+sx;
					int yadv = dest_bmp->rowpixels;
					dy=dy*16;
					while(1)
					{
						UINT8 *source = source0;
						UINT32 *dest = dest0;
						UINT8 *pri = pri0;

						switch(x)
						{
							int c;
							UINT8 p;
							case 31: PSET_O NEXT_P
							case 30: PSET_O NEXT_P
							case 29: PSET_O NEXT_P
							case 28: PSET_O NEXT_P
							case 27: PSET_O NEXT_P
							case 26: PSET_O NEXT_P
							case 25: PSET_O NEXT_P
							case 24: PSET_O NEXT_P
							case 23: PSET_O NEXT_P
							case 22: PSET_O NEXT_P
							case 21: PSET_O NEXT_P
							case 20: PSET_O NEXT_P
							case 19: PSET_O NEXT_P
							case 18: PSET_O NEXT_P
							case 17: PSET_O NEXT_P
							case 16: PSET_O break;

							case 15: PSET_T NEXT_P
							case 14: PSET_T NEXT_P
							case 13: PSET_T NEXT_P
							case 12: PSET_T NEXT_P
							case 11: PSET_T NEXT_P
							case 10: PSET_T NEXT_P
							case  9: PSET_T NEXT_P
							case  8: PSET_T NEXT_P
							case  7: PSET_T NEXT_P
							case  6: PSET_T NEXT_P
							case  5: PSET_T NEXT_P
							case  4: PSET_T NEXT_P
							case  3: PSET_T NEXT_P
							case  2: PSET_T NEXT_P
							case  1: PSET_T NEXT_P
							case  0: PSET_T
						}

						if(!(--y)) break;
						source0 += dy;
						dest0+=yadv;
						pri0+=yadv;
					}
				}
			}
		}
	}
}
#undef PSET_T
#undef PSET_O
#undef NEXT_P


INLINE void f3_drawgfxzoom( mame_bitmap *dest_bmp,const gfx_element *gfx,
		unsigned int code,
		unsigned int color,
		int flipx,int flipy,
		int sx,int sy,
		const rectangle *clip,
		int scalex, int scaley,
		UINT8 pri_dst)
{
	rectangle myclip;

	pri_dst=1<<pri_dst;

	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}


	if( gfx && gfx->colortable )
	{
		const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
//      int palBase = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)] - Machine->remapped_colortable;
		int source_base = (code % gfx->total_elements) * 16;

		{
			/* compute sprite increment per screen pixel */
			int dx = (16<<16)/scalex;
			int dy = (16<<16)/scaley;

			int ex = sx+scalex;
			int ey = sy+scaley;

			int x_index_base;
			int y_index;

			if( flipx )
			{
				x_index_base = (scalex-1)*dx;
				dx = -dx;
			}
			else
			{
				x_index_base = 0;
			}

			if( flipy )
			{
				y_index = (scaley-1)*dy;
				dy = -dy;
			}
			else
			{
				y_index = 0;
			}

			if( clip )
			{
				if( sx < clip->min_x)
				{ /* clip left */
					int pixels = clip->min_x-sx;
					sx += pixels;
					x_index_base += pixels*dx;
				}
				if( sy < clip->min_y )
				{ /* clip top */
					int pixels = clip->min_y-sy;
					sy += pixels;
					y_index += pixels*dy;
				}
				/* NS 980211 - fixed incorrect clipping */
				if( ex > clip->max_x+1 )
				{ /* clip right */
					int pixels = ex-clip->max_x-1;
					ex -= pixels;
				}
				if( ey > clip->max_y+1 )
				{ /* clip bottom */
					int pixels = ey-clip->max_y-1;
					ey -= pixels;
				}
			}

			if( ex>sx )
			{ /* skip if inner loop doesn't draw anything */
//              if (dest_bmp->depth == 32)
				{
					int y;
					for( y=sy; y<ey; y++ )
					{
						UINT8 *source = gfx->gfxdata + (source_base+(y_index>>16)) * 16;
						UINT32 *dest = (UINT32 *)dest_bmp->line[y];
						UINT8 *pri = pri_alp_bitmap->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = source[x_index>>16];
							if(c)
							{
								UINT8 p=pri[x];
								if (p == 0 || p == 0xff)
								{
									dest[x] = pal[c];
									pri[x] = pri_dst;
								}
							}
							x_index += dx;
						}
						y_index += dy;
					}
				}
			}
		}
	}
}


#define CALC_ZOOM(p)	{										\
	p##_addition = 0x100 - block_zoom_##p + p##_addition_left;	\
	p##_addition_left = p##_addition & 0xf;						\
	p##_addition = p##_addition >> 4;							\
	/*zoom##p = p##_addition << 12;*/							\
}

static void get_sprite_info(const UINT32 *spriteram32_ptr)
{
	const int min_x=Machine->visible_area.min_x,max_x=Machine->visible_area.max_x;
	const int min_y=Machine->visible_area.min_y,max_y=Machine->visible_area.max_y;
	int offs,spritecont,flipx,flipy,old_x,old_y,color,x,y;
	int sprite,global_x=0,global_y=0,subglobal_x=0,subglobal_y=0;
	int block_x=0, block_y=0;
	int last_color=0,last_x=0,last_y=0,block_zoom_x=0,block_zoom_y=0;
	int this_x,this_y;
	int y_addition=16, x_addition=16;
	int multi=0;
	int sprite_top;

	int x_addition_left = 8, y_addition_left = 8;

	struct tempsprite *sprite_ptr = spritelist;

	int total_sprites=0;

	color=0;
    flipx=flipy=0;
    old_y=old_x=0;
    y=x=0;

	sprite_top=0x1000;
	for (offs = 0; offs < sprite_top && (total_sprites < 0x400); offs += 4)
	{
		const int current_offs=offs; /* Offs can change during loop, current_offs cannot */

		/* Check if the sprite list jump command bit is set */
		if ((spriteram32_ptr[current_offs+3]>>16) & 0x8000) {
			UINT32 jump = (spriteram32_ptr[current_offs+3]>>16)&0x3ff;

			UINT32 new_offs=((offs&0x2000)|((jump<<4)/4));
			if (new_offs==offs)
				break;
			offs=new_offs - 4;
		}

		/* Check if special command bit is set */
		if (spriteram32_ptr[current_offs+1] & 0x8000) {
			UINT32 cntrl=(spriteram32_ptr[current_offs+2])&0xffff;
			flipscreen=cntrl&0x2000;

			/*  cntrl&0x1000 = disabled?  (From F2 driver, doesn't seem used anywhere)
                cntrl&0x0010 = ???
                cntrl&0x0020 = ???
            */

			/* Sprite bank select */
			if (cntrl&1) {
				offs=offs|0x2000;
				sprite_top=sprite_top|0x2000;
			}
		}

		/* Set global sprite scroll */
		if (((spriteram32_ptr[current_offs+1]>>16) & 0xf000) == 0xa000) {
			global_x = (spriteram32_ptr[current_offs+1]>>16) & 0xfff;
			if (global_x >= 0x800) global_x -= 0x1000;
			global_y = spriteram32_ptr[current_offs+1] & 0xfff;
			if (global_y >= 0x800) global_y -= 0x1000;
		}

		/* And sub-global sprite scroll */
		if (((spriteram32_ptr[current_offs+1]>>16) & 0xf000) == 0x5000) {
			subglobal_x = (spriteram32_ptr[current_offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_ptr[current_offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
		}

		if (((spriteram32_ptr[current_offs+1]>>16) & 0xf000) == 0xb000) {
			subglobal_x = (spriteram32_ptr[current_offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_ptr[current_offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
			global_y=subglobal_y;
			global_x=subglobal_x;
		}

		/* A real sprite to process! */
		sprite = (spriteram32_ptr[current_offs]>>16) | ((spriteram32_ptr[current_offs+2]&1)<<16);
		spritecont = spriteram32_ptr[current_offs+2]>>24;

/* These games either don't set the XY control bits properly (68020 bug?), or
    have some different mode from the others */
#ifdef DARIUSG_KLUDGE
		if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR || f3_game==RECALH)
			multi=spritecont&0xf0;
#endif

		/* Check if this sprite is part of a continued block */
		if (multi) {
			/* Bit 0x4 is 'use previous colour' for this block part */
			if (spritecont&0x4) color=last_color;
			else color=(spriteram32_ptr[current_offs+2]>>16)&0xff;

#ifdef DARIUSG_KLUDGE
			if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR || f3_game==RECALH) {
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					if (spritecont & 0x4) {
						x = block_x;
					} else {
						this_x = spriteram32_ptr[current_offs+1]>>16;
						if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

						if ((spriteram32_ptr[current_offs+1]>>16)&0x8000) {
							this_x+=0;
						} else if ((spriteram32_ptr[current_offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_x+=global_x;
						} else { /* Apply both scroll offsets */
							this_x+=global_x+subglobal_x;
						}

						x = block_x = this_x;
					}
					x_addition_left = 8;
					CALC_ZOOM(x)
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
					CALC_ZOOM(x)
				}

				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					if (spritecont & 0x4) {
						y = block_y;
					} else {
						this_y = spriteram32_ptr[current_offs+1]&0xffff;
						if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;

						if ((spriteram32_ptr[current_offs+1]>>16)&0x8000) {
							this_y+=0;
						} else if ((spriteram32_ptr[current_offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_y+=global_y;
						} else { /* Apply both scroll offsets */
							this_y+=global_y+subglobal_y;
						}

						y = block_y = this_y;
					}
					y_addition_left = 8;
					CALC_ZOOM(y)
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
					CALC_ZOOM(y)
				}
			} else
#endif
			{
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					x = block_x;
					x_addition_left = 8;
					CALC_ZOOM(x)
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
					CALC_ZOOM(x)
				}
				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					y = block_y;
					y_addition_left = 8;
					CALC_ZOOM(y)
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
					CALC_ZOOM(y)
				}
				/* Both zero = reread block latch? */
			}
		}
		/* Else this sprite is the possible start of a block */
		else {
			color = (spriteram32_ptr[current_offs+2]>>16)&0xff;
			last_color=color;

			/* Sprite positioning */
			this_y = spriteram32_ptr[current_offs+1]&0xffff;
			this_x = spriteram32_ptr[current_offs+1]>>16;
			if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;
			if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

			/* Ignore both scroll offsets for this block */
			if ((spriteram32_ptr[current_offs+1]>>16)&0x8000) {
				this_x+=0;
				this_y+=0;
			} else if ((spriteram32_ptr[current_offs+1]>>16)&0x4000) {
				/* Ignore subglobal (but apply global) */
				this_x+=global_x;
				this_y+=global_y;
			} else { /* Apply both scroll offsets */
				this_x+=global_x+subglobal_x;
				this_y+=global_y+subglobal_y;
			}

	        block_y = y = this_y;
            block_x = x = this_x;

			block_zoom_x=spriteram32_ptr[current_offs];
			block_zoom_y=(block_zoom_x>>8)&0xff;
			block_zoom_x&=0xff;

			x_addition_left = 8;
			CALC_ZOOM(x)

			y_addition_left = 8;
			CALC_ZOOM(y)
		}

		/* These features are common to sprite and block parts */
  		flipx = spritecont&0x1;
		flipy = spritecont&0x2;
		multi = spritecont&0x8;
		last_x=x;
		last_y=y;

		if (!sprite) continue;
		if (!x_addition || !y_addition) continue;

		if (flipscreen)
		{
			int tx,ty;

			tx = 512-x_addition-x;
			ty = 256-y_addition-y;

			if (tx+x_addition<=min_x || tx>max_x || ty+y_addition<=min_y || ty>max_y) continue;
			sprite_ptr->x = tx;
			sprite_ptr->y = ty;
			sprite_ptr->flipx = !flipx;
			sprite_ptr->flipy = !flipy;
		}
		else
		{
			if (x+x_addition<=min_x || x>max_x || y+y_addition<=min_y || y>max_y) continue;
			sprite_ptr->x = x;
			sprite_ptr->y = y;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
		}


		sprite_ptr->code = sprite;
		sprite_ptr->color = color;
		sprite_ptr->zoomx = x_addition;
		sprite_ptr->zoomy = y_addition;
		sprite_ptr->pri = (color & 0xc0) >> 6;
		sprite_ptr++;
		total_sprites++;
	}
	sprite_end = sprite_ptr;
}
#undef CALC_ZOOM


static void f3_drawsprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	const struct tempsprite *sprite_ptr;
	const gfx_element *sprite_gfx = Machine->gfx[2];

	sprite_ptr = sprite_end;
	sprite_pri_usage=0;
	while (sprite_ptr != spritelist)
	{
		int pri;
		sprite_ptr--;

		pri=sprite_ptr->pri;
		sprite_pri_usage|=1<<pri;

		if(sprite_ptr->zoomx==16 && sprite_ptr->zoomy==16)
			f3_drawgfx(bitmap,sprite_gfx,
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					cliprect,
					pri);
		else
			f3_drawgfxzoom(bitmap,sprite_gfx,
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					cliprect,
					sprite_ptr->zoomx,sprite_ptr->zoomy,
					pri);
	}
}

/******************************************************************************/

VIDEO_UPDATE( f3 )
{
	unsigned int sy_fix[5],sx_fix[5];
	int tile;

	f3_skip_this_frame=0;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Dynamically decode VRAM chars if dirty */
	if (vram_changed)
		for (tile = 0;tile < 256;tile++)
			if (vram_dirty[tile]) {
				decodechar(Machine->gfx[0],tile,(UINT8 *)f3_vram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				tilemap_mark_all_tiles_dirty(vram_layer); // TODO
				//tilemap_mark_tile_dirty(vram_layer,tile);
				vram_dirty[tile]=0;
			}

	/* Decode chars & mark tilemap dirty */
	if (pivot_changed)
		for (tile = 0;tile < 2048;tile++)
			if (pivot_dirty[tile]) {
				decodechar(Machine->gfx[3],tile,(UINT8 *)f3_pivot_ram,Machine->drv->gfxdecodeinfo[3].gfxlayout);
				tilemap_mark_tile_dirty(pixel_layer,tile);
				pivot_dirty[tile]=0;
			}
	pivot_changed=vram_changed=0;

	/* Setup scroll */
	sy_fix[0]=((f3_control_0[2]&0xffff0000)>> 7) + (1<<16);
	sy_fix[1]=((f3_control_0[2]&0x0000ffff)<< 9) + (1<<16);
	sy_fix[2]=((f3_control_0[3]&0xffff0000)>> 7) + (1<<16);
	sy_fix[3]=((f3_control_0[3]&0x0000ffff)<< 9) + (1<<16);
	sx_fix[0]=((f3_control_0[0]&0xffc00000)>> 6) - (6<<16);
	sx_fix[1]=((f3_control_0[0]&0x0000ffc0)<<10) - (10<<16);
	sx_fix[2]=((f3_control_0[1]&0xffc00000)>> 6) - (14<<16);
	sx_fix[3]=((f3_control_0[1]&0x0000ffc0)<<10) - (18<<16);
	sx_fix[4]=-(f3_control_1[2]>>16)+41;
	sy_fix[4]=-(f3_control_1[2]&0x1ff);

	sx_fix[0]-=((f3_control_0[0]&0x003f0000)>> 6)+0x0400-0x10000;
	sx_fix[1]-=((f3_control_0[0]&0x0000003f)<<10)+0x0400-0x10000;
	sx_fix[2]-=((f3_control_0[1]&0x003f0000)>> 6)+0x0400-0x10000;
	sx_fix[3]-=((f3_control_0[1]&0x0000003f)<<10)+0x0400-0x10000;

	if (flipscreen)
	{
		sy_fix[0]= 0x3000000-sy_fix[0];
		sy_fix[1]= 0x3000000-sy_fix[1];
		sy_fix[2]= 0x3000000-sy_fix[2];
		sy_fix[3]= 0x3000000-sy_fix[3];
		sx_fix[0]=-0x1a00000-sx_fix[0];
		sx_fix[1]=-0x1a00000-sx_fix[1];
		sx_fix[2]=-0x1a00000-sx_fix[2];
		sx_fix[3]=-0x1a00000-sx_fix[3];
		sx_fix[4]=-sx_fix[4] + 75;
		sy_fix[4]=-sy_fix[4];
	}

	fillbitmap(pri_alp_bitmap,0,cliprect);

	/* sprites */
	if (sprite_lag==0)
		get_sprite_info(spriteram32);

	/* Update sprite buffer */
	f3_drawsprites(bitmap,cliprect);

	/* Parse sprite, alpha & clipping parts of lineram */
	get_spritealphaclip_info();

	/* Parse playfield effects */
	get_line_ram_info(pf1_tilemap,sx_fix[0],sy_fix[0],0,f3_pf_data_1);
	get_line_ram_info(pf2_tilemap,sx_fix[1],sy_fix[1],1,f3_pf_data_2);
	get_line_ram_info(pf3_tilemap,sx_fix[2],sy_fix[2],2,f3_pf_data_3);
	get_line_ram_info(pf4_tilemap,sx_fix[3],sy_fix[3],3,f3_pf_data_4);
	get_vram_info(vram_layer,pixel_layer,sx_fix[4],sy_fix[4]);

	/* Draw final framebuffer */
	f3_scanline_draw(bitmap,cliprect);

//  print_debug_info(bitmap);
}
