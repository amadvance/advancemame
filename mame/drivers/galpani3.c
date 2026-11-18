// license:BSD-3-Clause
// copyright-holders:David Haywood, Uki
/*
    Gals Panic 3
    (c) Kaneko 1995

    Driver by David Haywood

    Original Skeleton driver by David Haywood
    Early Progress by Sebastien Volpe

Check done by main code, as part of EEPROM data:
'Gals Panic 3 v0.96 95/08/29(Tue)'

 Sprites are from Supernova
 Backgrounds are 3x bitmap layers + some kind of priority / mask layer
 The bitmaps have blitter devices to decompress RLE rom data into them

*/

/*

Gals Panic 3 (JPN Ver.)
(c)1995 Kaneko

CPU:    68000-16
Sound:  YMZ280B-F
OSC:    28.6363MHz
        33.3333MHz
EEPROM: 93C46
Chips.: GRAP2 x3                <- R/G/B Chips?
        APRIO-GL
        BABY004
        GCNT2
        TBSOP01                 <- ToyBox NEC uPD78324 series MCU with 32K internal rom
        CG24173 6186            <- Sprites, see suprnova.c
        CG24143 4181            <- ^


G3P0J1.71     prg.
G3P1J1.102

GP340000.123  chr.
GP340100.122
GP340200.121
GP340300.120
G3G0J0.101
G3G1J0.100

G3D0X0.134

GP320000.1    OBJ chr.

GP310000.41   sound data
GP310100.40


--- Team Japump!!! ---
Dumped by Uki
10/22/2000

*/

/*
Differences from MAME:
- The Toybox MCU is simulated with the shared Kaneko 16 helpers.
- The three GRAP2 chips are implemented inline in this driver.
- The framebuffer brightness registers only enable or disable a layer,
  there is no alpha blending.
*/

#include "driver.h"
#include "sound/ymz280b.h"
#include "includes/kaneko16.h"

extern void skns_sprite_kludge(int x, int y);
extern void skns_draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, UINT32 *ram, int ram_size, int gfxregion, UINT32 *regs, int encode_priority);

/***************************************************************************

 video

***************************************************************************/

#define GRAP2_PEN_BASE 0x4000
#define GRAP2_PEN_COUNT 0x101

static UINT16 *galpani3_sprregs, *galpani3_spriteram;
static UINT16 *galpani3_priority_buffer;
static UINT32 *galpani3_spriteram32, *galpani3_spc_regs;
static UINT16 galpani3_priority_scrollx, galpani3_priority_scrolly;
static mame_bitmap *galpani3_sprite_bitmap;

struct grap2_chip
{
	UINT16 *framebuffer;
	UINT16 *palette;
	UINT16 bgcol;
	UINT16 scrollx, scrolly;
	UINT16 enable;
	UINT16 bright1, bright2;
	UINT16 address[2];
	UINT16 regs2;
	UINT8 regs1_i;
};

static struct grap2_chip grap2[3];

static void grap2_set_pen(int chip, int index, UINT16 data)
{
	palette_set_color(GRAP2_PEN_BASE + chip * GRAP2_PEN_COUNT + index,
		pal5bit(data >> 5), pal5bit(data >> 10), pal5bit(data >> 0));
}

static void grap2_do_rle(struct grap2_chip *g, UINT32 address, UINT32 dstaddress)
{
	const UINT8 *rom = memory_region(REGION_GFX2);
	UINT32 size = memory_region_length(REGION_GFX2);
	int rle_count = 0;
	int normal_count = 0;
	UINT8 data = 0;

	while (dstaddress < 0x40000)
	{
		if (rle_count == 0 && normal_count == 0)
		{
			data = address < size ? rom[address] : 0xff;
			++address;

			if (data & 0x80)
			{
				normal_count = (data & 0x7f) + 1;
			}
			else
			{
				rle_count = (data & 0x7f) + 1;
				data = address < size ? rom[address] : 0xff;
				++address;
			}
		}
		else if (rle_count)
		{
			g->framebuffer[dstaddress++] = data;
			--rle_count;
		}
		else
		{
			data = address < size ? rom[address] : 0xff;
			++address;
			g->framebuffer[dstaddress++] = data;
			--normal_count;
		}
	}
}

#define GRAP2_HANDLERS(n) \
static WRITE16_HANDLER( grap2_##n##_scrollx_w ) { grap2[n].scrollx = data; } \
static WRITE16_HANDLER( grap2_##n##_scrolly_w ) { grap2[n].scrolly = data; } \
static WRITE16_HANDLER( grap2_##n##_enable_w ) { grap2[n].enable = data; } \
static WRITE16_HANDLER( grap2_##n##_bgcol_w ) \
{ \
	grap2[n].bgcol = data; \
	grap2_set_pen(n, 0x100, data); \
} \
static WRITE16_HANDLER( grap2_##n##_bright1_w ) { COMBINE_DATA(&grap2[n].bright1); } \
static WRITE16_HANDLER( grap2_##n##_bright2_w ) { COMBINE_DATA(&grap2[n].bright2); } \
static WRITE16_HANDLER( grap2_##n##_address_w ) { COMBINE_DATA(&grap2[n].address[offset]); } \
static WRITE16_HANDLER( grap2_##n##_regs2_w ) { COMBINE_DATA(&grap2[n].regs2); } \
static WRITE16_HANDLER( grap2_##n##_go_w ) \
{ \
	UINT32 address = grap2[n].address[1] | (grap2[n].address[0] << 16); \
	UINT32 dstaddress = (data & 0x1ff) | ((grap2[n].regs2 & 0x1ff) << 9); \
	if ((data & 0xf000) == 0x3000) \
		grap2_do_rle(&grap2[n], address, dstaddress); \
} \
static WRITE16_HANDLER( grap2_##n##_palette_w ) \
{ \
	COMBINE_DATA(&grap2[n].palette[offset]); \
	grap2_set_pen(n, offset, grap2[n].palette[offset]); \
} \
static READ16_HANDLER( grap2_##n##_regs_r ) \
{ \
	switch (offset) \
	{ \
		case 0x2: \
			return grap2[n].enable; \
		case 0xb: \
			grap2[n].regs1_i ^= 1; \
			return 0xfffe | ((~grap2[n].regs1_i) & 1); \
	} \
	return 0; \
}

GRAP2_HANDLERS(0)
GRAP2_HANDLERS(1)
GRAP2_HANDLERS(2)

static void galpani3_scanline(int scanline)
{
	int next;

	/* three interrupts per frame at fixed scanlines of the 512 lines frame */
	switch (scanline)
	{
		case 0:
			cpunum_set_input_line(0, 3, HOLD_LINE);
			next = 128;
			break;
		case 128:
			cpunum_set_input_line(0, 5, HOLD_LINE);
			next = 240;
			break;
		default:
			cpunum_set_input_line(0, 2, HOLD_LINE);
			next = 0;
			break;
	}

	timer_set(cpu_getscanlinetime(next), next, galpani3_scanline);
}

VIDEO_START(galpani3)
{
	int i;

	/* so we can use suprnova.c */
	skns_sprite_kludge(0, 0);

	galpani3_spriteram32 = auto_malloc(0x4000);
	galpani3_spc_regs = auto_malloc(0x40);
	memset(galpani3_spriteram32, 0, 0x4000);
	memset(galpani3_spc_regs, 0, 0x40);

	galpani3_sprite_bitmap = auto_bitmap_alloc(320, 240);

	for (i = 0; i < 3; ++i)
	{
		grap2[i].bgcol = 0;
	}

	return 0;
}

static void galpani3_draw_pixel(UINT16 *dst, int chip, UINT16 pixel)
{
	UINT16 col = pixel == 0x100 ? grap2[chip].bgcol : grap2[chip].palette[pixel];
	UINT16 alpha = (col & 0x8000) ? grap2[chip].bright2 : grap2[chip].bright1;

	/* no alpha blending, the brightness only shows or hides the pixel */
	if (alpha & 0xff)
		*dst = GRAP2_PEN_BASE + chip * GRAP2_PEN_COUNT + pixel;
}

VIDEO_UPDATE(galpani3)
{
	int x, y;

	fillbitmap(bitmap, get_black_pen(), cliprect);
	fillbitmap(galpani3_sprite_bitmap, 0, cliprect);

	skns_draw_sprites(galpani3_sprite_bitmap, cliprect, galpani3_spriteram32, 0x4000, REGION_GFX1, galpani3_spc_regs, 1);

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT16 *sprline = (UINT16 *)galpani3_sprite_bitmap->line[y];
		UINT16 *dst = (UINT16 *)bitmap->line[y];
		UINT16 *srcline1 = grap2[0].framebuffer + ((y + grap2[0].scrolly + 11) & 0x1ff) * 0x200;
		UINT16 *srcline2 = grap2[1].framebuffer + ((y + grap2[1].scrolly + 11) & 0x1ff) * 0x200;
		UINT16 *srcline3 = grap2[2].framebuffer + ((y + grap2[2].scrolly + 11) & 0x1ff) * 0x200;
		UINT16 *priline = galpani3_priority_buffer + ((y + galpani3_priority_scrolly + 11) & 0x1ff) * 0x200;

		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
		{
			UINT16 sprdat = sprline[x];
			UINT8 dat1 = srcline1[(x + grap2[0].scrollx + 67) & 0x1ff];
			UINT8 dat2 = srcline2[(x + grap2[1].scrollx + 67) & 0x1ff];
			UINT8 dat3 = srcline3[(x + grap2[2].scrollx + 67) & 0x1ff];
			UINT8 pridat = priline[(x + galpani3_priority_scrollx + 66) & 0x1ff];
			UINT16 *d = &dst[x];

#define SPRITE_PIXEL(pri) \
			if (((sprdat & 0xc000) == (pri)) && ((sprdat & 0xff) != 0)) \
				*d = sprdat & 0x3fff

			if (pridat == 0x0f) /* area drawn over */
			{
				SPRITE_PIXEL(0x0000);
				if (grap2[2].enable)
					galpani3_draw_pixel(d, 2, dat3);

				SPRITE_PIXEL(0x4000);
				if (dat1 && grap2[0].enable)
					galpani3_draw_pixel(d, 0, dat1);

				SPRITE_PIXEL(0x8000);
				if (dat2 && grap2[1].enable)
					galpani3_draw_pixel(d, 1, dat2);

				SPRITE_PIXEL(0xc000);
			}
			else if (pridat == 0xcf) /* the girl */
			{
				SPRITE_PIXEL(0x0000);
				if (grap2[0].enable)
					galpani3_draw_pixel(d, 0, 0x100);

				SPRITE_PIXEL(0x4000);
				if (grap2[1].enable)
					galpani3_draw_pixel(d, 1, 0x100);

				SPRITE_PIXEL(0x8000);
				if (dat3 && grap2[2].enable)
					galpani3_draw_pixel(d, 2, dat3);

				SPRITE_PIXEL(0xc000);
			}
			else if (pridat == 0x30) /* intro boxes */
			{
				SPRITE_PIXEL(0x0000);
				if (grap2[1].enable)
					galpani3_draw_pixel(d, 1, dat2);

				SPRITE_PIXEL(0x4000);
				if (dat1 && grap2[0].enable)
					galpani3_draw_pixel(d, 0, dat1);

				SPRITE_PIXEL(0x8000);
				if (dat3 && grap2[2].enable)
					galpani3_draw_pixel(d, 2, dat3);

				SPRITE_PIXEL(0xc000);
			}
			else
			{
				SPRITE_PIXEL(0x0000);
				if (grap2[0].enable)
					galpani3_draw_pixel(d, 0, dat1);

				SPRITE_PIXEL(0x4000);
				if (dat2 && grap2[1].enable)
					galpani3_draw_pixel(d, 1, dat2);

				SPRITE_PIXEL(0x8000);
				if (dat3 && grap2[2].enable)
					galpani3_draw_pixel(d, 2, dat3);

				SPRITE_PIXEL(0xc000);
			}
		}
	}
}


INPUT_PORTS_START( galpani3 )

	PORT_START	// IN0 - Player Controls
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // ?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN1 - Player Controls
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN ) // ?
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN2 - Coins
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1  ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2  ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE  ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT     )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED	)

	PORT_START	// IN3 - DSW provided by the MCU - $200386.b <- $400200
	PORT_DIPNAME( 0x0100, 0x0100, "Test Mode" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


WRITE16_HANDLER( galpani3_suprnova_sprite32_w )
{
	COMBINE_DATA(&galpani3_spriteram[offset]);

	/* put in galpani3_spriteram32 for suprnova.c */
	offset >>= 1;
	galpani3_spriteram32[offset] = (galpani3_spriteram[offset * 2 + 1] << 16) | (galpani3_spriteram[offset * 2]);
}

WRITE16_HANDLER( galpani3_suprnova_sprite32regs_w )
{
	COMBINE_DATA(&galpani3_sprregs[offset]);

	/* put in galpani3_spc_regs for suprnova.c */
	offset >>= 1;
	galpani3_spc_regs[offset] = (galpani3_sprregs[offset * 2 + 1] << 16) | (galpani3_sprregs[offset * 2]);
}

static WRITE16_HANDLER( galpani3_priority_scrollx_w )
{
	galpani3_priority_scrollx = data;
}

static WRITE16_HANDLER( galpani3_priority_scrolly_w )
{
	galpani3_priority_scrolly = data;
}


/***************************************************************************

                            Toybox MCU simulation

***************************************************************************/

static void galpani3_mcu_run(void)
{
	UINT16 mcu_command = kaneko16_mcu_ram[0x0010/2];
	UINT16 mcu_offset  = kaneko16_mcu_ram[0x0012/2] / 2;
	UINT16 mcu_data    = kaneko16_mcu_ram[0x0014/2];

	switch (mcu_command >> 8)
	{
		case 0x02:	// Read from NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0)) != 0)
			{
				mame_fread(f, &kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x42:	// Write to NVRAM
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1)) != 0)
			{
				mame_fwrite(f, &kaneko16_mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
		}
		break;

		case 0x03:	// DSW
			kaneko16_mcu_ram[mcu_offset] = readinputport(3);
		break;

		case 0x04:	// Protection
			toybox_handle_04_region(mcu_data, kaneko16_mcu_ram, REGION_CPU2);
		break;
	}
}

static ADDRESS_MAP_START( galpani3_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x17ffff) AM_ROM

	AM_RANGE(0x200000, 0x20ffff) AM_RAM // area [B] - Work RAM
	AM_RANGE(0x280000, 0x287fff) AM_RAM AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16) // area [A] - palette for sprites

	AM_RANGE(0x300000, 0x303fff) AM_RAM AM_BASE(&galpani3_spriteram) AM_WRITE(galpani3_suprnova_sprite32_w)
	AM_RANGE(0x380000, 0x38003f) AM_RAM AM_BASE(&galpani3_sprregs) AM_WRITE(galpani3_suprnova_sprite32regs_w)

	AM_RANGE(0x400000, 0x40ffff) AM_RAM AM_BASE(&kaneko16_mcu_ram) // area [C]

	AM_RANGE(0x580000, 0x580001) AM_WRITE(toybox_mcu_com0_w)
	AM_RANGE(0x600000, 0x600001) AM_WRITE(toybox_mcu_com1_w)
	AM_RANGE(0x680000, 0x680001) AM_WRITE(toybox_mcu_com2_w)
	AM_RANGE(0x700000, 0x700001) AM_WRITE(toybox_mcu_com3_w)
	AM_RANGE(0x780000, 0x780001) AM_READ(toybox_mcu_status_r)

#define GRAP2_MAP(base, n) \
	AM_RANGE(base + 0x000000, base + 0x0003ff) AM_RAM \
	AM_RANGE(base + 0x000400, base + 0x000401) AM_WRITE(grap2_##n##_scrollx_w) \
	AM_RANGE(base + 0x000800, base + 0x000bff) AM_RAM \
	AM_RANGE(base + 0x000c00, base + 0x000c01) AM_WRITE(grap2_##n##_scrolly_w) \
	AM_RANGE(base + 0x000c02, base + 0x000c03) AM_WRITE(grap2_##n##_enable_w) \
	AM_RANGE(base + 0x000c06, base + 0x000c07) AM_WRITE(grap2_##n##_bgcol_w) \
	AM_RANGE(base + 0x000c10, base + 0x000c11) AM_WRITE(grap2_##n##_bright1_w) \
	AM_RANGE(base + 0x000c12, base + 0x000c13) AM_WRITE(grap2_##n##_bright2_w) \
	AM_RANGE(base + 0x000c18, base + 0x000c1b) AM_WRITE(grap2_##n##_address_w) \
	AM_RANGE(base + 0x000c1c, base + 0x000c1d) AM_WRITE(grap2_##n##_regs2_w) \
	AM_RANGE(base + 0x000c1e, base + 0x000c1f) AM_WRITE(grap2_##n##_go_w) \
	AM_RANGE(base + 0x000c00, base + 0x000c1f) AM_READ(grap2_##n##_regs_r) \
	AM_RANGE(base + 0x080000, base + 0x0801ff) AM_RAM AM_WRITE(grap2_##n##_palette_w) AM_BASE(&grap2[n].palette) \
	AM_RANGE(base + 0x100000, base + 0x17ffff) AM_RAM AM_BASE(&grap2[n].framebuffer)

	GRAP2_MAP(0x800000, 0)
	GRAP2_MAP(0xa00000, 1)
	GRAP2_MAP(0xc00000, 2)

	// priority / alpha buffer
	AM_RANGE(0xe00000, 0xe7ffff) AM_RAM AM_BASE(&galpani3_priority_buffer) // area [J] - A area ? odd bytes only, initialized 00..ff,00..ff,..., then cleared
	AM_RANGE(0xe80000, 0xe80001) AM_WRITE(galpani3_priority_scrollx_w)
	AM_RANGE(0xe80002, 0xe80003) AM_WRITE(galpani3_priority_scrolly_w)

	AM_RANGE(0xf00000, 0xf00001) AM_NOP // ? written once (2nd opcode, $1.b)
	AM_RANGE(0xf00010, 0xf00011) AM_READ(input_port_0_word_r)
	AM_RANGE(0xf00012, 0xf00013) AM_READ(input_port_1_word_r)
	AM_RANGE(0xf00014, 0xf00015) AM_READ(input_port_2_word_r)
	AM_RANGE(0xf00016, 0xf00017) AM_NOP // ? read, but overwritten

	AM_RANGE(0xf00020, 0xf00021) AM_WRITE(YMZ280B_register_0_lsb_w)	// sound
	AM_RANGE(0xf00022, 0xf00023) AM_WRITE(YMZ280B_data_0_lsb_w)		//
	AM_RANGE(0xf00040, 0xf00041) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)	// watchdog
	AM_RANGE(0xf00050, 0xf00051) AM_NOP // ? written once (3rd opcode, $30.b)
ADDRESS_MAP_END


static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0	// irq ?
};

static MACHINE_RESET( galpani3 )
{
	int i;

	toybox_mcu_run = galpani3_mcu_run;
	toybox_mcu_init();

	for (i = 0; i < 3; ++i)
	{
		grap2[i].scrollx = 0;
		grap2[i].scrolly = 0;
		grap2[i].enable = 0;
		grap2[i].bright1 = 0;
		grap2[i].bright2 = 0;
		grap2[i].regs1_i = 0;
	}

	galpani3_priority_scrollx = 0;
	galpani3_priority_scrolly = 0;

	timer_set(cpu_getscanlinetime(0), 0, galpani3_scanline);
}

static MACHINE_DRIVER_START( galpani3 )
	MDRV_CPU_ADD_TAG("main", M68000, 28636363/2)
	MDRV_CPU_PROGRAM_MAP(galpani3_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_RESET(galpani3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(GRAP2_PEN_BASE + 3 * GRAP2_PEN_COUNT)

	MDRV_VIDEO_START(galpani3)
	MDRV_VIDEO_UPDATE(galpani3)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YMZ280B, 33333000 / 2)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "mono", 1.0)
	MDRV_SOUND_ROUTE(1, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( galpani3 ) /* All game text in English */
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "g3p0e0.u71",  0x000000, 0x080000, CRC(fa681118) SHA1(982b568a77ed620ba5708fec4c186d329d48cb48) )
	ROM_LOAD16_BYTE( "g3p1e0.u102", 0x000001, 0x080000, CRC(f1150f1b) SHA1(a6fb719937927a9a39c7a4888017c63c47c2dd6c) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites - RLE encoded */
	ROM_LOAD( "gp320000.1", 0x000000, 0x200000, CRC(a0112827) SHA1(0a6c78d71b75a1d78215aab3104176aa1769b14f) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* Backgrounds - RLE encoded */
	ROM_LOAD( "gp340000.123", 0x000000, 0x200000, CRC(a58a26b1) SHA1(832d70cce1b4f04fa50fc221962ff6cc4287cb92) )		// 19950414GROMACap
	ROM_LOAD( "gp340100.122", 0x200000, 0x200000, CRC(746fe4a8) SHA1(a5126ae9e83d556277d31b166296a708c311a902) )		// 19950414GROMBCap
	ROM_LOAD( "gp340200.121", 0x400000, 0x200000, CRC(e9bc15c8) SHA1(2c6a10e768709d1937d9206970553f4101ce9016) )		// 19950414GROMCCap
	ROM_LOAD( "gp340300.120", 0x600000, 0x200000, CRC(59062eef) SHA1(936977c20d83540c1e0f65d429c7ebea201ef991) )		// 19950414GROMDCap
	ROM_LOAD16_BYTE( "g3g0j0.101", 0xe00000, 0x040000, CRC(fbb1e0dc) SHA1(14f6377afd93054aa5dc38af235ae12b932e847f) )	// 19950523GROMECap
	ROM_LOAD16_BYTE( "g3g1j0.100", 0xe00001, 0x040000, CRC(18edb5f0) SHA1(5e2ed0105b3e6037f6116494d3b186a368824171) )	//

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "gp310100.40", 0x000000, 0x200000, CRC(6a0b1d12) SHA1(11fed80b96d07fddb27599743991c58c12c048e0) )
	ROM_LOAD( "gp310000.41", 0x200000, 0x100000, CRC(641062ef) SHA1(c8902fc46319eac94b3f95d18afa24bd895078d6) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* MCU Code? */
	ROM_LOAD16_WORD_SWAP( "g3d0x0.134", 0x000000, 0x020000, CRC(4ace10f9) SHA1(d19e4540d535ce10d23cb0844be03a3239b3402e) )
ROM_END

ROM_START( galpani3j ) /* Some game text in Japanese, but no "For use in Japan" type region notice */
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "g3p0j1.71",  0x000000, 0x080000, CRC(52893326) SHA1(78fdbf3436a4ba754d7608fedbbede5c719a4505) )
	ROM_LOAD16_BYTE( "g3p1j1.102", 0x000001, 0x080000, CRC(05f935b4) SHA1(81e78875585bcdadad1c302614b2708e60563662) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites - RLE encoded */
	ROM_LOAD( "gp320000.1", 0x000000, 0x200000, CRC(a0112827) SHA1(0a6c78d71b75a1d78215aab3104176aa1769b14f) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* Backgrounds - RLE encoded */
	ROM_LOAD( "gp340000.123", 0x000000, 0x200000, CRC(a58a26b1) SHA1(832d70cce1b4f04fa50fc221962ff6cc4287cb92) )		// 19950414GROMACap
	ROM_LOAD( "gp340100.122", 0x200000, 0x200000, CRC(746fe4a8) SHA1(a5126ae9e83d556277d31b166296a708c311a902) )		// 19950414GROMBCap
	ROM_LOAD( "gp340200.121", 0x400000, 0x200000, CRC(e9bc15c8) SHA1(2c6a10e768709d1937d9206970553f4101ce9016) )		// 19950414GROMCCap
	ROM_LOAD( "gp340300.120", 0x600000, 0x200000, CRC(59062eef) SHA1(936977c20d83540c1e0f65d429c7ebea201ef991) )		// 19950414GROMDCap
	ROM_LOAD16_BYTE( "g3g0j0.101", 0xe00000, 0x040000, CRC(fbb1e0dc) SHA1(14f6377afd93054aa5dc38af235ae12b932e847f) )	// 19950523GROMECap
	ROM_LOAD16_BYTE( "g3g1j0.100", 0xe00001, 0x040000, CRC(18edb5f0) SHA1(5e2ed0105b3e6037f6116494d3b186a368824171) )	//

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "gp310100.40", 0x000000, 0x200000, CRC(6a0b1d12) SHA1(11fed80b96d07fddb27599743991c58c12c048e0) )
	ROM_LOAD( "gp310000.41", 0x200000, 0x100000, CRC(641062ef) SHA1(c8902fc46319eac94b3f95d18afa24bd895078d6) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* MCU Code? */
	ROM_LOAD16_WORD_SWAP( "g3d0x0.134", 0x000000, 0x020000, CRC(4ace10f9) SHA1(d19e4540d535ce10d23cb0844be03a3239b3402e) )
ROM_END



DRIVER_INIT( galpani3 )
{
	toybox_decrypt_region(REGION_CPU2);
}

GAME( 1995, galpani3,  0,        galpani3, galpani3, galpani3, ROT90, "Kaneko", "Gals Panic 3 (Euro)", GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
GAME( 1995, galpani3j, galpani3, galpani3, galpani3, galpani3, ROT90, "Kaneko", "Gals Panic 3 (Japan)", GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
