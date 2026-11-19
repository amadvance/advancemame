// license:BSD-3-Clause
// copyright-holders:Luca Elia
/*************************************************************************************************************

                                              -= Sigma B-98 Hardware =-

                                                 driver by Luca Elia

CPU     :   TAXAN KY-80 (Yamaha)
Video   :   TAXAN KY-3211
Sound   :   YMZ280B
NVRAM   :   93C46 and/or battery backed RAM

Graphics are made of sprites only.
Each sprite is composed of X x Y tiles and can be zoomed / shrunk and rotated.
Tiles can be 16x16x4 or 16x16x8.

Dumped games:                           ROMs:

1997 Minna Atsumare! Dodge Hero         B9802
1997 Itazura Daisuki! Sushimaru Kun     B9803
1997 GeGeGe no Kitarou Youkai Slot      B9804
1997 Burning Sanrinsya                  B9805
1997 PEPSI Man                          B9806
1998 Transformers Beast Wars II         B9808
1997 Uchuu Tokkyuu Medalian             B9809
2000 Minna Ganbare! Dash Hero           B9811

--------------------------------------------------------------------------------------

To Do:

- gegege checks the EEPROM output after reset, and wants a timed 0->1 transition or locks up while
  saving setting in service mode. The checks are patched out in the ROMs.
- dashhero does not acknowledge the button bashing correctly, it's very hard to win (a slower pace works better!)

*************************************************************************************************************/

#include <math.h>
#include "driver.h"
#include "machine/eeprom.h"
#include "machine/ticket.h"
#include "sound/ymz280b.h"
#include "vidhrdw/generic.h"

static UINT8 *nvram;
static UINT8 vregs[0x22];
static mame_bitmap *sprite_bitmap;

static UINT8 reg, reg2;
static UINT8 rombank, rambank;
static UINT8 c0, c6;

#define NVRAM_SIZE 0x1000


/***************************************************************************

    Video

***************************************************************************/

static VIDEO_START( sigmab98 )
{
	sprite_bitmap = auto_bitmap_alloc(512, 512);
	buffered_spriteram = auto_malloc(spriteram_size);
	memset(buffered_spriteram, 0, spriteram_size);
	return 0;
}

static int integer_part(int x)
{
	return (x + 0x8000) >> 16;
}

/*
    Sprite entry (0x10 bytes):

    00      color (low 4 bits)
    01      bit 6 = 256 colors, bit 4 = flip x, bit 3 = flip y, bit 2 = enabled, bits 1-0 = priority (0 = front)
    02-03   tile code
    04-05   bits 15-11 = x tiles - 1, bits 9-0 = x
    06-07   bits 15-11 = y tiles - 1, bits 9-0 = y
    08-09   destination x delta (0x100 = no zoom)
    0a-0b   destination y delta
    0c-0d   source x
    0e-0f   source y
*/
static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int pri_mask)
{
	UINT8 *end = buffered_spriteram - 0x10;
	UINT8 *s = end + 0x1000;

	for (; s != end; s -= 0x10)
	{
		int color, gfx, code, nx, ny, dstx, dsty, dstdx, dstdy, srcx, srcy;
		int x0, x1, dx, flipx, y0, y1, dy, flipy, x, y;
		int z, srcdzz, dstdxx, dstdyx, dstdxy, dstdyy;
		int src_max_x, src_max_y;
		UINT16 penmask;
		rectangle sprite_cliprect;

		if ((s[0x01] & 0x04) == 0)
			continue;

		if (((1 << (s[0x01] & 0x03)) & pri_mask) == 0)
			continue;

		color = s[0x00] & 0xf;
		gfx = (s[0x01] & 0x40) ? 1 : 0;
		code = s[0x02] * 256 + s[0x03];

		nx = ((s[0x04] & 0xf8) >> 3) + 1;
		dstx = (s[0x04] & 0x03) * 256 + s[0x05];

		ny = ((s[0x06] & 0xf8) >> 3) + 1;
		dsty = (s[0x06] & 0x03) * 256 + s[0x07];

		dstdx = s[0x08] * 256 + s[0x09];
		dstdy = s[0x0a] * 256 + s[0x0b];

		srcx = s[0x0c] * 256 + s[0x0d];
		srcy = s[0x0e] * 256 + s[0x0f];

		dstx = (dstx & 0x01ff) - (dstx & 0x0200);
		dsty = (dsty & 0x01ff) - (dsty & 0x0200);

		flipx = s[0x01] & 0x10;
		flipy = s[0x01] & 0x08;

		if (flipx) { x0 = nx - 1; x1 = -1; dx = -1; }
		else       { x0 = 0;      x1 = nx; dx = +1; }

		if (flipy) { y0 = ny - 1; y1 = -1; dy = -1; }
		else       { y0 = 0;      y1 = ny; dy = +1; }

		/* fast path, no zoom, rotation or offset */
		if (dstdx == 0x100 && !dstdy && !srcx && !srcy)
		{
			for (y = y0; y != y1; y += dy)
			{
				for (x = x0; x != x1; x += dx)
				{
					drawgfx(bitmap, Machine->gfx[gfx],
							code++, color,
							flipx, flipy,
							dstx + x * 16, dsty + y * 16,
							cliprect, TRANSPARENCY_PEN, 0);
				}
			}
			continue;
		}

		/* the sprite is first drawn without transformation in a temporary bitmap */
		sprite_cliprect.min_x = 0;
		sprite_cliprect.max_x = nx * 16 - 1;
		sprite_cliprect.min_y = 0;
		sprite_cliprect.max_y = ny * 16 - 1;
		for (y = y0; y != y1; y += dy)
		{
			for (x = x0; x != x1; x += dx)
			{
				drawgfx(sprite_bitmap, Machine->gfx[gfx],
						code++, color,
						flipx, flipy,
						x * 16, y * 16,
						&sprite_cliprect, TRANSPARENCY_NONE, 0);
			}
		}

		dstdx = (dstdx & 0x7fff) - (dstdx & 0x8000);
		dstdy = (dstdy & 0x7fff) - (dstdy & 0x8000);
		srcx = (srcx & 0x7fff) - (srcx & 0x8000);
		srcy = (srcy & 0x7fff) - (srcy & 0x8000);
		dstdy = -dstdy;

		/* 16.16 fixed point */
		dstx <<= 16;
		dsty <<= 16;

		z = (int)(sqrt((double)(dstdx * dstdx + dstdy * dstdy)) + 0.5);
		if (!z)
			z = 0x100;
		srcdzz = z << 8;

		dstdxx = (dstdx << 16) / z;
		dstdyx = (dstdy << 16) / z;

		dstdxy = -dstdyx;
		dstdyy = dstdxx;

		srcx = (-srcx << 8) / z;
		srcy = (-srcy << 8) / z;

		dstx += srcx * dstdxx;
		dsty += srcx * dstdyx;

		dstx += srcy * dstdxy;
		dsty += srcy * dstdyy;

		/* 2x2 supersampling avoids holes in the destination */
		srcdzz /= 2;
		dstdxx /= 2;
		dstdyx /= 2;
		dstdxy /= 2;
		dstdyy /= 2;

		penmask = gfx ? 0xff : 0x0f;
		src_max_x = sprite_cliprect.max_x;
		src_max_y = sprite_cliprect.max_y;

		srcy = 0;
		for (;;)
		{
			int dstx_prev = dstx;
			int dsty_prev = dsty;
			int fy = integer_part(srcy);

			if (fy > src_max_y)
				break;
			if (fy >= 0)
			{
				srcx = 0;
				for (;;)
				{
					int fx = integer_part(srcx);

					if (fx > src_max_x)
						break;
					if (fx >= 0)
					{
						int px = integer_part(dstx);
						int py = integer_part(dsty);

						if (px >= cliprect->min_x && px <= cliprect->max_x && py >= cliprect->min_y && py <= cliprect->max_y)
						{
							UINT16 pen = ((UINT16 *)sprite_bitmap->line[fy])[fx];
							if (pen & penmask)
								((UINT16 *)bitmap->line[py])[px] = pen;
						}
					}

					srcx += srcdzz;
					dstx += dstdxx;
					dsty += dstdyx;
				}
			}

			srcy += srcdzz;

			dstx = dstx_prev + dstdxy;
			dsty = dsty_prev + dstdyy;
		}
	}
}

static VIDEO_UPDATE( sigmab98 )
{
	fillbitmap(bitmap, Machine->pens[0x100], cliprect);

	/* from the back (priority 3) to the front (priority 0) */
	draw_sprites(bitmap, cliprect, 8);
	draw_sprites(bitmap, cliprect, 4);
	draw_sprites(bitmap, cliprect, 2);
	draw_sprites(bitmap, cliprect, 1);
}

static WRITE8_HANDLER( palette_w )
{
	int color = offset >> 1;
	int data16;

	paletteram[offset] = data;
	data16 = (paletteram[color * 2] << 8) | paletteram[color * 2 + 1];
	palette_set_color(color, pal5bit(data16 >> 10), pal5bit(data16 >> 5), pal5bit(data16));
}

static WRITE8_HANDLER( vregs_w )
{
	vregs[offset] = data;

	/* background color */
	if (offset == 0x1b || offset == 0x1d)
	{
		int x = (vregs[0x1d] << 8) + vregs[0x1b];
		palette_set_color(0x100, pal5bit(x >> 10), pal5bit(x >> 5), pal5bit(x));
	}
}

static READ8_HANDLER( vregs_r )
{
	return vregs[offset];
}

static READ8_HANDLER( d013_r )
{
	return (cpu_getvblank() ? 0x20 : 0) | 0x04;
}

static READ8_HANDLER( d021_r )
{
	return 0;
}


/***************************************************************************

    Bank switching

***************************************************************************/

static void set_rombank(int entry, int data, int max)
{
	if (data >= max)
		logerror("%04x: unknown rom bank = %02x\n", activecpu_get_pc(), data);
	else
		memory_set_bank(1, entry);
}

static void set_rambank(int entry)
{
	memory_set_bank(2, entry);
}

/* Minna Atsumare! Dodge Hero */

static WRITE8_HANDLER( dodghero_regs_w )
{
	if (offset == 0)
	{
		reg = data;
		return;
	}

	switch (reg)
	{
		case 0x1f:
			rombank = data;
			set_rombank(data * 2, data, 0x18);
			break;
		case 0x9f:
			rombank = data;
			set_rombank(data * 2 + 1, data, 0x18);
			break;
		default:
			logerror("%04x: unknown reg written: %02x = %02x\n", activecpu_get_pc(), reg, data);
	}
}

static READ8_HANDLER( dodghero_regs_r )
{
	if (offset == 0)
		return reg;

	switch (reg)
	{
		case 0x1f:
		case 0x9f:
			return rombank;
		default:
			logerror("%04x: unknown reg read: %02x\n", activecpu_get_pc(), reg);
			return 0x00;
	}
}

static WRITE8_HANDLER( dodghero_regs2_w )
{
	if (offset == 0)
	{
		reg2 = data;
		return;
	}

	switch (reg2)
	{
		case 0x37:
			rambank = data;
			switch (data)
			{
				case 0x32: set_rambank(0); break;
				case 0x36: set_rambank(1); break;
				default:
					logerror("%04x: unknown ram bank = %02x\n", activecpu_get_pc(), data);
			}
			break;
		default:
			logerror("%04x: unknown reg2 written: %02x = %02x\n", activecpu_get_pc(), reg2, data);
	}
}

static READ8_HANDLER( dodghero_regs2_r )
{
	if (offset == 0)
		return reg2;

	switch (reg2)
	{
		case 0x37:
			return rambank;
		default:
			logerror("%04x: unknown reg2 read: %02x\n", activecpu_get_pc(), reg2);
			return 0x00;
	}
}

/* GeGeGe no Kitarou Youkai Slot and the other games */

static WRITE8_HANDLER( gegege_regs_w )
{
	if (offset == 0)
	{
		reg = data;
		return;
	}

	switch (reg)
	{
		case 0x1f:
			rombank = data;
			set_rombank(data, data, 0x18);
			break;
		default:
			logerror("%04x: unknown reg written: %02x = %02x\n", activecpu_get_pc(), reg, data);
	}
}

static READ8_HANDLER( gegege_regs_r )
{
	if (offset == 0)
		return reg;

	switch (reg)
	{
		case 0x1f:
			return rombank;
		default:
			logerror("%04x: unknown reg read: %02x\n", activecpu_get_pc(), reg);
			return 0x00;
	}
}

static WRITE8_HANDLER( gegege_regs2_w )
{
	if (offset == 0)
	{
		reg2 = data;
		return;
	}

	switch (reg2)
	{
		case 0xb5:
			rambank = data;
			switch (data)
			{
				case 0x32: set_rambank(0); break;
				case 0x36: set_rambank(1); break;
				default:
					logerror("%04x: unknown ram bank = %02x\n", activecpu_get_pc(), data);
			}
			break;
		default:
			logerror("%04x: unknown reg2 written: %02x = %02x\n", activecpu_get_pc(), reg2, data);
	}
}

static READ8_HANDLER( gegege_regs2_r )
{
	if (offset == 0)
		return reg2;

	switch (reg2)
	{
		case 0xb5:
			return rambank;
		default:
			logerror("%04x: unknown reg2 read: %02x\n", activecpu_get_pc(), reg2);
			return 0x00;
	}
}

/* Minna Ganbare! Dash Hero */

static WRITE8_HANDLER( dashhero_regs2_w )
{
	if (offset == 0)
	{
		reg2 = data;
		return;
	}

	switch (reg2)
	{
		case 0x75:
		case 0xb5:
		case 0xf5:
			rambank = data;
			switch (data)
			{
				case 0x32: set_rambank(0); break;
				case 0x34: set_rambank(1); break;
				case 0x36: set_rambank(2); break;
				case 0x39: set_rambank(3); break;
				default:
					logerror("%04x: unknown ram bank = %02x\n", activecpu_get_pc(), data);
			}
			break;
		default:
			logerror("%04x: unknown reg2 written: %02x = %02x\n", activecpu_get_pc(), reg2, data);
	}
}

static READ8_HANDLER( dashhero_regs2_r )
{
	if (offset == 0)
		return reg2;

	switch (reg2)
	{
		case 0x75:
		case 0xb5:
		case 0xf5:
			return rambank;
		default:
			logerror("%04x: unknown reg2 read: %02x\n", activecpu_get_pc(), reg2);
			return 0x00;
	}
}


/***************************************************************************

    Outputs and inputs

***************************************************************************/

/* serial EEPROM access, the clock is the rising edge of bit 4 */
static WRITE8_HANDLER( eeprom_w )
{
	EEPROM_write_bit((data & 0x40) >> 6);
	EEPROM_set_cs_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
	EEPROM_set_clock_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

	c0 = data;
}

static READ8_HANDLER( eeprom_r )
{
	return (readinputport(0) & 0x7f) | (EEPROM_read_bit() << 7);
}

static READ8_HANDLER( button_r )
{
	int hopper = (ticket_dispenser_r(0) & 0x80) ? 0x04 : 0x00;

	return (readinputport(1) & ~0x04) | hopper;
}

static WRITE8_HANDLER( c4_w )
{
	set_led_status(0, data & 0x10);
}

static WRITE8_HANDLER( c6_w )
{
	coin_lockout_w(0, (~data) & 0x02);
	coin_counter_w(0, data & 0x04);

	/* the sprites are buffered on the rising edge of bit 3 */
	if ((data & 0x08) && !(c6 & 0x08))
		buffer_spriteram(spriteram, spriteram_size);

	set_led_status(1, data & 0x10);
	set_led_status(2, data & 0x20);

	c6 = data;
}

static WRITE8_HANDLER( c8_w )
{
	ticket_dispenser_w(0, (!(data & 0x02) && (data & 0x01)) ? 0x00 : 0x80);
}


/***************************************************************************

    Memory maps

***************************************************************************/

static ADDRESS_MAP_START( dodghero_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xa7ff) AM_ROMBANK(1)
	AM_RANGE(0xa800, 0xb7ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xc800, 0xc9ff) AM_RAM AM_WRITE(palette_w) AM_BASE(&paletteram)
	AM_RANGE(0xd001, 0xd07f) AM_RAM
	AM_RANGE(0xd813, 0xd813) AM_READ(d013_r)
	AM_RANGE(0xd821, 0xd821) AM_READ(d021_r)
	AM_RANGE(0xd800, 0xd821) AM_READWRITE(vregs_r, vregs_w)
	AM_RANGE(0xd800, 0xdfff) AM_RAMBANK(2)
	AM_RANGE(0xe000, 0xefff) AM_RAM AM_BASE(&nvram)
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( dodghero_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YMZ280B_data_0_r, YMZ280B_register_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YMZ280B_status_0_r, YMZ280B_data_0_w)
	AM_RANGE(0xa0, 0xa1) AM_READWRITE(dodghero_regs_r, dodghero_regs_w)
	AM_RANGE(0xa4, 0xa5) AM_READWRITE(dodghero_regs2_r, dodghero_regs2_w)
	AM_RANGE(0xc0, 0xc0) AM_READWRITE(eeprom_r, eeprom_w)
	AM_RANGE(0xc2, 0xc2) AM_READ(button_r)
	AM_RANGE(0xc4, 0xc4) AM_READWRITE(input_port_2_r, c4_w)
	AM_RANGE(0xc6, 0xc6) AM_WRITE(c6_w)
	AM_RANGE(0xc8, 0xc8) AM_WRITE(c8_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( gegege_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_ROMBANK(1)
	AM_RANGE(0xa000, 0xafff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xc000, 0xc1ff) AM_RAM AM_WRITE(palette_w) AM_BASE(&paletteram)
	AM_RANGE(0xc800, 0xc87f) AM_RAM
	AM_RANGE(0xd013, 0xd013) AM_READ(d013_r)
	AM_RANGE(0xd021, 0xd021) AM_READ(d021_r)
	AM_RANGE(0xd000, 0xd021) AM_READWRITE(vregs_r, vregs_w)
	AM_RANGE(0xd800, 0xdfff) AM_RAMBANK(2)
	AM_RANGE(0xe000, 0xefff) AM_RAM AM_BASE(&nvram)
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( gegege_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YMZ280B_data_0_r, YMZ280B_register_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YMZ280B_status_0_r, YMZ280B_data_0_w)
	AM_RANGE(0xa0, 0xa1) AM_READWRITE(gegege_regs_r, gegege_regs_w)
	AM_RANGE(0xa4, 0xa5) AM_READWRITE(gegege_regs2_r, gegege_regs2_w)
	AM_RANGE(0xc0, 0xc0) AM_READWRITE(eeprom_r, eeprom_w)
	AM_RANGE(0xc2, 0xc2) AM_READ(button_r)
	AM_RANGE(0xc4, 0xc4) AM_READWRITE(input_port_2_r, c4_w)
	AM_RANGE(0xc6, 0xc6) AM_WRITE(c6_w)
	AM_RANGE(0xc8, 0xc8) AM_WRITE(c8_w)
	AM_RANGE(0xe5, 0xe5) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dashhero_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(YMZ280B_data_0_r, YMZ280B_register_0_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(YMZ280B_status_0_r, YMZ280B_data_0_w)
	AM_RANGE(0xa0, 0xa1) AM_READWRITE(gegege_regs_r, gegege_regs_w)
	AM_RANGE(0xa4, 0xa5) AM_READWRITE(dashhero_regs2_r, dashhero_regs2_w)
	AM_RANGE(0xc0, 0xc0) AM_READWRITE(eeprom_r, eeprom_w)
	AM_RANGE(0xc2, 0xc2) AM_READ(button_r)
	AM_RANGE(0xc4, 0xc4) AM_READWRITE(input_port_2_r, c4_w)
	AM_RANGE(0xc6, 0xc6) AM_WRITE(c6_w)
	AM_RANGE(0xc8, 0xc8) AM_WRITE(c8_w)
	AM_RANGE(0xe5, 0xe5) AM_READ(MRA8_NOP)
ADDRESS_MAP_END


/***************************************************************************

    Graphics layout

***************************************************************************/

static const gfx_layout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ 4*1,4*0, 4*3,4*2, 4*5,4*4, 4*7,4*6, 4*9,4*8, 4*11,4*10, 4*13,4*12, 4*15,4*14 },
	{ STEP16(0,16*4) },
	16*16*4
};

static const gfx_layout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP16(0,8) },
	{ STEP16(0,16*8) },
	16*16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0, 0x100/16 },
	{ REGION_GFX1, 0, &layout_16x16x8, 0, 0x100/256 },
	{ -1 }
};


/***************************************************************************

    Input ports

***************************************************************************/

/* 1 button (plus bet and payout) */
INPUT_PORTS_START( sigma_1b )
	PORT_START_TAG("EEPROM")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* EEPROM data out */

	PORT_START_TAG("BUTTON")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN1 ) PORT_IMPULSE(5) PORT_NAME("Medal")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* hopper */
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 ) PORT_NAME("Bet")
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START_TAG("PAYOUT")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_START2 ) PORT_NAME("Payout")
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END

/* 3 buttons (plus bet and payout) */
INPUT_PORTS_START( sigma_3b )
	PORT_INCLUDE( sigma_1b )

	PORT_MODIFY("BUTTON")
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON3 )
INPUT_PORTS_END

/* 5 buttons (plus bet and payout) */
INPUT_PORTS_START( sigma_5b )
	PORT_INCLUDE( sigma_1b )

	PORT_MODIFY("BUTTON")
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_BUTTON3 )

	PORT_MODIFY("PAYOUT")
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON5 )
INPUT_PORTS_END

/* joystick (plus bet and payout) */
INPUT_PORTS_START( sigma_js )
	PORT_INCLUDE( sigma_1b )

	PORT_MODIFY("BUTTON")
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT )

	PORT_MODIFY("PAYOUT")
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT )
INPUT_PORTS_END


/***************************************************************************

    Machine drivers

***************************************************************************/

static MACHINE_RESET( sigmab98 )
{
	rombank = 0;
	memory_set_bank(1, 0);

	rambank = 0;
	memory_set_bank(2, 0);
}

static INTERRUPT_GEN( sigmab98_vblank_interrupt )
{
	cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x5a);
}

static NVRAM_HANDLER( sigmab98 )
{
	if (read_or_write)
	{
		EEPROM_save(file);
		mame_fwrite(file, nvram, NVRAM_SIZE);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C46);

		if (file)
		{
			EEPROM_load(file);
			mame_fread(file, nvram, NVRAM_SIZE);
		}
		else
			memset(nvram, 0, NVRAM_SIZE);
	}
}

/* dashhero doesn't keep the battery backed RAM between sessions */
static NVRAM_HANDLER( dashhero )
{
	nvram_handler_93C46(file, read_or_write);
}

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0
};

static MACHINE_DRIVER_START( sigmab98 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 10000000)
	MDRV_CPU_PROGRAM_MAP(gegege_map, 0)
	MDRV_CPU_IO_MAP(gegege_io_map, 0)
	MDRV_CPU_VBLANK_INT(sigmab98_vblank_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)
	MDRV_MACHINE_RESET(sigmab98)
	MDRV_NVRAM_HANDLER(sigmab98)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0x100)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xf0-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100 + 1)

	MDRV_VIDEO_START(sigmab98)
	MDRV_VIDEO_UPDATE(sigmab98)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dodghero )
	MDRV_IMPORT_FROM(sigmab98)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(dodghero_map, 0)
	MDRV_CPU_IO_MAP(dodghero_io_map, 0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dashhero )
	MDRV_IMPORT_FROM(sigmab98)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_IO_MAP(dashhero_io_map, 0)
	MDRV_NVRAM_HANDLER(dashhero)
MACHINE_DRIVER_END


/***************************************************************************

    ROMs

***************************************************************************/

ROM_START( dodghero )
	ROM_REGION( 0x22000, REGION_CPU1, 0 )
	ROM_LOAD( "b9802-1d.ic7", 0x00000, 0x20000, CRC(093492e3) SHA1(d4dd104dc2410d97add532eea031cbc1ede3b0b1) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "b9802-2.ic12",  0x00000, 0x80000, CRC(bb810ab8) SHA1(02bb1bb9b6dd0d24401c8a8c579f5ebcba963d8f) )
	ROM_LOAD( "b9802-3a.ic13", 0x80000, 0x80000, CRC(8792e487) SHA1(c5ed8059cd40a00656016b33762a04b9bedd7f06) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9802-5.ic16", 0x00000, 0x80000, CRC(4840bdbd) SHA1(82f286ef848df9b6dcb5ff3b3aaa11d8e93e995b) )
	ROM_LOAD( "b9802-6.ic26", 0x80000, 0x80000, CRC(d83d8537) SHA1(9a5afdc68417db828a09188d653552452930b136) )
ROM_END

ROM_START( sushimar )
	ROM_REGION( 0x22000, REGION_CPU1, 0 )
	ROM_LOAD( "b9803-1c.ic7", 0x00000, 0x20000, CRC(8ad3b7be) SHA1(14d8cec6723f230d4167de91b5b1103fe40755bc) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "b9803-2.ic12",  0x00000, 0x80000, CRC(cae710a4) SHA1(c0511412d8feaa032b8bcd72074522d1b90f22b2) )
	ROM_LOAD( "b9803-03.ic13", 0x80000, 0x80000, CRC(f69f37f6) SHA1(546045b50dbc3ef45fc4dd1c7f2f6a23dfdc53d8) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9803-5a.ic16", 0x00000, 0x80000, CRC(da3f36aa) SHA1(0caffbe6726afd41763f25378f8820724aa7bbce) )
ROM_END

/*
  GeGeGe no Kitarou Youkai Slot
  (C) 1997 Banpresto, Sigma
  PCB:
    (c) 1997 Sigma B-98-1 MAIN PCB
    970703 (Sticker)
  CPU:
    TAXAN KY-80 YAMAHA 9650 AZGC (@IC1)
    XTAL ?? (@X1)
  Video:
    TAXAN KY-3211 9722 AZGC (@IC11)
    XTAL 27.000 MHz (@XOSC1)
    M548262-60 (@IC24) - 262144-Word x 8-Bit Multiport DRAM
  Sound:
    YAMAHA YMZ280B-F (@IC14)
    XTAL ?? (@X2)
    Trimmer
  Other:
    93C46AN EEPROM (@IC5)
    MAX232CPE (@IC6)
    Battery (@BAT)
*/
ROM_START( gegege )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b9804-1.ic7", 0x00000, 0x20000, CRC(f8b4f855) SHA1(598bd9f91123e9ab539ce3f33779bff2d072e731) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "b9804-2.ic12", 0x00000, 0x80000, CRC(4211079d) SHA1(d601c623fb909f1346fd02b8fb37b67956e2cd4e) )
	ROM_LOAD( "b9804-3.ic13", 0x80000, 0x80000, CRC(54aeb2aa) SHA1(ccf939111f6288a889846d51bab47ff4e992c542) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9804-5.ic16", 0x00000, 0x80000, CRC(ddd7984c) SHA1(3558c495776671ffd3cd5c665b87827b3959b360) )
ROM_END

ROM_START( b3rinsya )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b9805-1c.ic7", 0x00000, 0x20000, CRC(a8cde2f4) SHA1(74d1f3f1710084d788a71dec0366f2c3f756fdf8) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "b9805-2.ic12", 0x00000, 0x80000, CRC(7ec2e957) SHA1(1eb9095663d4f8f8f0c77f151918af1978332b3d) )
	ROM_LOAD( "b9805-3.ic13", 0x80000, 0x80000, CRC(449d0848) SHA1(63e91e4be8b58a6ebf1777ed5a9c23416bacba48) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9805-5.ic16", 0x00000, 0x80000, CRC(f686f886) SHA1(ab68d12c5cb3a9fbc8a178739f39a2ff3104a0a1) )
ROM_END

ROM_START( pepsiman )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b9806-1a.ic7", 0x00000, 0x20000, CRC(3152fe90) SHA1(98a8ae1bd3a4381cec11ba8b3e9cdad71c7bd05a) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "b9806-2.ic12", 0x00000, 0x80000, CRC(82f650ea) SHA1(c0b214fdc39329e2136707bc195d470d4b613509) )
	ROM_LOAD( "b9806-3.ic13", 0x80000, 0x80000, CRC(07dc548e) SHA1(9419c0cac289a9894cce1a10924f40e146e2ff8a) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9806-5.ic16", 0x00000, 0x80000, CRC(6d405dfb) SHA1(e65ffe1279680097894754e379d7ad638657eb49) )
ROM_END

ROM_START( tbeastw2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b9808-1b.ic7", 0x00000, 0x20000, CRC(65f7e079) SHA1(d421da3c99d62d3228e1b9c1cfb2de51f0fcc56e) )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )
	ROM_LOAD( "b9808-2.ic12", 0x000000, 0x80000, CRC(dda5c2d2) SHA1(1bb21e7251df93b0f502b716e958d81f4e4e46dd) )
	ROM_LOAD( "b9808-3.ic13", 0x080000, 0x80000, CRC(80df49c6) SHA1(14342be3a176cdf015c0ac07a4f1c109862c67aa) )
	ROM_LOAD( "b9808-4.ic17", 0x100000, 0x80000, CRC(d90961ea) SHA1(c2f226a528238eafc1ba37200da4ee6ce9b54325) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9808-5.ic16", 0x00000, 0x80000, CRC(762c6d5f) SHA1(0d4e35b7f346c8cc0c49163474f34c1fc462998a) )
	ROM_LOAD( "b9808-6.ic26", 0x80000, 0x80000, CRC(9ed759c9) SHA1(963db80b8a107ce9292bbc776ba91bc76ad82d5b) )
ROM_END

ROM_START( ucytokyu )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b9809-1.ic7", 0x00000, 0x20000, CRC(5be6adff) SHA1(7248157111be2ae23df7d51f5d071cc3b9fd79b4) )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )
	ROM_LOAD( "b9809-2.ic12", 0x000000, 0x80000, CRC(18f342b3) SHA1(09d62bb3597259e0fbae2c0f4ed163685a4a9dd9) )
	ROM_LOAD( "b9809-3.ic13", 0x080000, 0x80000, CRC(88a2a52a) SHA1(0dd10d4fa88d1a54150729026495a70dbe67bae0) )
	ROM_LOAD( "b9809-4.ic17", 0x100000, 0x80000, CRC(ea74eacd) SHA1(279fa1d2bc7bfafbafecd0e0758a47345ca95140) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "b9809-5.ic16", 0x00000, 0x80000, CRC(470006e6) SHA1(34c82fae7364eb5288de5c8128d72d7e5772c526) )
	ROM_LOAD( "b9809-6.ic26", 0x80000, 0x80000, CRC(4e2d5fdf) SHA1(af1357b0f6a407890ecad26a18d2b4e223802693) )
ROM_END

ROM_START( dashhero )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "b098111-0101.ic7", 0x00000, 0x20000, CRC(46488393) SHA1(898bafbf5273b368cf963d863fb93e9fa0da816f) )

	ROM_REGION( 0x180000, REGION_GFX1, 0 )
	ROM_LOAD( "b98114-0100.ic12", 0x000000, 0x80000, CRC(067625ea) SHA1(f9dccfc85adbb840da7512db0c88f554b453d2d2) )
	ROM_LOAD( "b98115-0100.ic13", 0x080000, 0x80000, CRC(d6f0b89d) SHA1(33b5f2f6529fd9a145ccb1b4deffabf5fa0d46cb) )
	ROM_LOAD( "b98116-0100.ic17", 0x100000, 0x80000, CRC(c0dbe953) SHA1(a75e202a0c1be988b8fd7d4ee23ebc82f6110e5f) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b098112-0100.ic16", 0x00000, 0x80000, CRC(26e5d6f5) SHA1(6fe6a26e51097886db58a6619b12a73cd21e7130) )
ROM_END


/***************************************************************************

    Driver init

***************************************************************************/

/* the games poll the EEPROM output right after a reset with timing loops, they are skipped */
static void patch_eeprom_timing(int a, int b)
{
	UINT8 *rom = memory_region(REGION_CPU1);

	rom[0x8138] = 0x00;
	rom[0x8139] = 0x00;
	rom[a] = 0x00;
	rom[b] = 0x00;
}

static void init_banks(int rom_entries, int rom_stride, int ram_entries)
{
	UINT8 *rom = memory_region(REGION_CPU1);
	UINT8 *bankedram = auto_malloc(0x800 * ram_entries);

	memset(bankedram, 0, 0x800 * ram_entries);

	memory_configure_bank(1, 0, rom_entries, rom + 0x8000, rom_stride);
	memory_configure_bank(2, 0, ram_entries, bankedram, 0x800);

	ticket_dispenser_init(200, TICKET_MOTOR_ACTIVE_LOW, TICKET_STATUS_ACTIVE_LOW);
}

static DRIVER_INIT( dodghero )
{
	init_banks(0x18 * 2, 0x800, 2);
}

static DRIVER_INIT( gegege )
{
	patch_eeprom_timing(0x8164, 0x8165);
	init_banks(0x18, 0x1000, 2);
}

static DRIVER_INIT( dashhero )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	rom[0x8138] = 0x00;
	rom[0x8139] = 0x00;
	rom[0x8162] = 0x00;
	rom[0x8163] = 0x00;
	init_banks(0x18, 0x1000, 4);
}


/***************************************************************************

    Game drivers

***************************************************************************/

GAME( 1997, dodghero, 0, dodghero, sigma_1b, dodghero, ROT0, "Sigma",             "Minna Atsumare! Dodge Hero",           0 )
GAME( 1997, sushimar, 0, dodghero, sigma_3b, dodghero, ROT0, "Sigma",             "Itazura Daisuki! Sushimaru Kun",       0 )
GAME( 1997, gegege,   0, sigmab98, sigma_1b, gegege,   ROT0, "Sigma / Banpresto", "GeGeGe no Kitarou Youkai Slot",        0 )
GAME( 1997, b3rinsya, 0, sigmab98, sigma_5b, gegege,   ROT0, "Sigma",             "Burning Sanrinsya - Burning Tricycle", 0 )
GAME( 1997, pepsiman, 0, sigmab98, sigma_3b, gegege,   ROT0, "Sigma",             "PEPSI Man",                            0 )
GAME( 1998, tbeastw2, 0, sigmab98, sigma_3b, gegege,   ROT0, "Sigma / Transformer Production Company / Takara", "Transformers Beast Wars II", 0 )
GAME( 1997, ucytokyu, 0, sigmab98, sigma_js, gegege,   ROT0, "Sigma",             "Uchuu Tokkyuu Medalian",               0 )
GAME( 2000, dashhero, 0, sigmab98, sigma_1b, dashhero, ROT0, "Sigma",             "Minna Ganbare! Dash Hero",             GAME_NOT_WORKING )
