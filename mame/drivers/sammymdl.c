// license:BSD-3-Clause
// copyright-holders:Luca Elia
/*************************************************************************************************************

                                               -= Sammy Medal Games =-

                                                 driver by Luca Elia

CPU     :   Kawasaki KL5C80A12CFP (Z80 Compatible High Speed Microcontroller)
Video   :   TAXAN KY-3211
Sound   :   OKI M9810B
NVRAM   :   93C46 and battery backed RAM

Cartridge based system. Carts contain just some 16Mb flash eeproms.

Info from Tatsuya Fujita:

According to some news articles for the AOU show 2002 and 2003 the correct system name
is "Treasure Fall" (despite the cart label is "Treasure Hall").

Dumped games:

1999 Dokidoki Kingyo Sukui
1999 Shatekids
2000 Animal Catch
2000 Itazura Monkey
2000 Pye-nage Taikai
2000 Taihou de Doboon
2001 Hae Hae Ka Ka Ka
2001 Kotekitai Slot
2002 Gun Kids
2003 Go Go Cowboy (EN, prize)
2003 Wantouchable

--------------------------------------------------------------------------------------

PCB:

  Sammy AM3AHF-01 SC MAIN PCB VER2 (Etched)
  MAIN PCB VER2 VM12-6001-0 (Sticker)

CPU:

  KAWASAKI KL5C80A12CFP (@U1) - Z80 Compatible High Speed Microcontroller
  XTAL 20 MHz  (@X1)
  MX29F040TC-12 VM1211L01 (@U2) - 4M-bit [512kx8] CMOS Equal Sector Flash Memory
  BSI BS62LV256SC-70      (@U4) - Very Low Power/Voltage CMOS SRAM 32K X 8 bit
  (or Toshiba TC55257DFL-70L)

Video:

  TAXAN KY-3211 (@U17)
  OKI M548262-60 (@U18) - 262144-Word x 8-Bit Multiport DRAM
  XTAL 27 MHz (@X3)

Sound:

  OKI M9810B (@U11)
  XTAL 4.09 MHz (@X2)
  Trimmer (@VR1)
  Toshiba TA7252AP (@U16) - 5.9W Audio Power Amplifier

Other:

  Xilinx XC9536 VM1212F01 (@U5) - In-System Programmable CPLD
  MX29F0?TC (@U3) - Empty 32 Pin DIP Socket
  M5295A (@U8) - Watchdog Timer (Near CUT-DEBUG MODE Jumper)
  M93C46MN6T (@U6) - Serial EEPROM
  RTC8564 (@U7) - not populated
  SN75C1168 (@U10) - Dual RS-422 Transceiver
  Cell Battery (@BAT)
  25 Pin Edge Connector
  56 Pin Cartridge Connector
  6 Pin Connector - +5V (1), GND (2), TCLK (3), TDO (4), TDI (5), TMS (6)

On Go Go Cowboy, U2 and U10 are unpopulated, but U3 is occupied by a
ST M27C4001-10F1 EPROM.

--------------------------------------------------------------------------------------

Notes:

- "BACKUP RAM NG" error: in test mode, choose "SET MODE" -> "RAM CLEAR" and keep the button pressed for long.

*************************************************************************************************************/

#include <math.h>
#include "driver.h"
#include "cpu/z80/z80daisy.h"
#include "machine/eeprom.h"
#include "machine/ticket.h"
#include "sound/okim9810.h"


/***************************************************************************

    Hardware layout

    The CPU is a Z80 with a 1 MB physical address space reached through a
    MMU with 1 KB pages. The memory is emulated as a flat 1 MB array and the
    CPU sees it through a translation table, used both for the data accesses
    and for the opcode fetches.

***************************************************************************/

#define PHYS_SIZE 0x100000

enum
{
	PAGE_ROM,		/* read only, also used for the unmapped pages */
	PAGE_RAM,
	PAGE_PALETTE,	/* writes update the palette */
	PAGE_VREGS		/* video registers */
};

enum
{
	VARIANT_ANIMALC,
	VARIANT_ITAZURAM,
	VARIANT_PYENAGET,
	VARIANT_TDOBOON,
	VARIANT_HAEKAKA,
	VARIANT_GOCOWBOY
};

static UINT8 *physmem;
static UINT8 page_type[PHYS_SIZE >> 10];

static int variant;
static UINT32 nvram_base, nvram_size;

static UINT16 mmu_a[4];
static UINT8 mmu_b[5];
static UINT32 mmu_base[64];

static UINT8 vblank_reg;

static UINT8 porta_data[2], porta_direction[2];
static UINT8 portb_data[3], portb_direction;

static mame_bitmap *sprite_bitmap;


/***************************************************************************

    KP69 interrupt controller

***************************************************************************/

static struct
{
	UINT16 levels, irr, isr, imr, ler, pgr;
	UINT8 ivr;
	int ivr_written;
	int illegal;
	int active;
} kp69;

static int kp69_top_level(UINT16 mask)
{
	int level = 15;

	while (!(mask & (1 << level)))
		--level;

	return level;
}

static int kp69_int_active(void)
{
	if (kp69.illegal)
		return 0;

	/* compare the priority of the pending request with the ones in service */
	if ((kp69.irr & kp69.pgr) != 0 || (kp69.isr & kp69.pgr) != 0)
		return (kp69.irr & ~kp69.isr & kp69.pgr) > (kp69.isr & kp69.pgr);
	else
		return (kp69.irr & ~kp69.isr) > kp69.isr;
}

static void kp69_set_int(int active)
{
	if (kp69.active != active)
	{
		kp69.active = active;
		cpunum_set_input_line(0, 0, active ? ASSERT_LINE : CLEAR_LINE);
	}
}

static void kp69_set_input(int level, int state)
{
	if (!(kp69.levels & (1 << level)) && state)
	{
		kp69.levels |= 1 << level;

		/* masked interrupts cannot be requested */
		if (!(kp69.irr & (1 << level)) && !(kp69.imr & (1 << level)))
		{
			UINT16 old_ints = kp69.irr | kp69.isr;
			kp69.irr |= 1 << level;
			if (!kp69.illegal && (1 << level) > ((kp69.pgr & (1 << level)) ? old_ints & kp69.pgr : old_ints))
				kp69_set_int(1);
		}
	}
	else if ((kp69.levels & (1 << level)) && !state)
	{
		kp69.levels &= ~(1 << level);

		/* only the level triggered interrupts follow the input */
		if (!(kp69.ler & (1 << level)) && (kp69.irr & (1 << level)))
		{
			kp69.irr &= ~(1 << level);
			if (!kp69.illegal)
				kp69_set_int(kp69_int_active());
		}
	}
}

static void kp69_set_imr(UINT16 data)
{
	UINT16 old_irr = kp69.irr;

	kp69.imr = data;
	kp69.irr = (kp69.irr & ~data & kp69.ler) | (kp69.levels & ~data & ~kp69.ler);
	if (kp69.irr != old_irr)
		kp69_set_int(kp69_int_active());
}

static void kp69_set_ler(UINT16 data)
{
	UINT16 old_irr = kp69.irr;

	kp69.irr = (kp69.levels & ~data) | (kp69.irr & kp69.ler & data);
	kp69.ler = data;
	if (kp69.irr != old_irr)
		kp69_set_int(kp69_int_active());
}

static void kp69_set_pgr(UINT16 data)
{
	if (kp69.pgr != data)
	{
		kp69.pgr = data;
		if (!kp69.illegal && kp69.isr != 0)
			kp69_set_int(kp69_int_active());
	}
}

static void kp69_reset(int param)
{
	kp69.ler = 0;
	kp69.imr = 0xffff;
	kp69.irr = 0;
	kp69.isr = 0;
	kp69.illegal = 0;
	kp69.pgr = 0;
	kp69.ivr_written = 0;
	kp69_set_int(0);
}

static int kp69_irq_state(int param)
{
	return kp69.active ? (Z80_DAISY_INT | Z80_DAISY_IEO) : Z80_DAISY_IEO;
}

static int kp69_irq_ack(int param)
{
	int level = -1;

	/* restrict to the high priority interrupts if any of them is pending */
	if ((kp69.irr & kp69.pgr) != 0)
	{
		level = kp69_top_level(kp69.irr & kp69.pgr);
		if ((1 << level) < (kp69.isr & kp69.pgr))
			level = -1;
	}
	else if (kp69.irr != 0 && (kp69.isr & kp69.pgr) == 0)
	{
		level = kp69_top_level(kp69.irr);
		if ((1 << level) < kp69.isr)
			level = -1;
	}

	if (level != -1)
	{
		if (kp69.ler & (1 << level))
			kp69.irr &= ~(1 << level);

		kp69.isr |= 1 << level;
		kp69_set_int(0);

		return kp69.ivr | (level << 1);
	}

	/* spurious interrupt, same vector as the level 0 but nothing is in service */
	kp69.illegal = 1;
	kp69_set_int(0);
	return kp69.ivr;
}

static void kp69_irq_reti(int param)
{
	if (kp69.illegal)
	{
		kp69.illegal = 0;
	}
	else if (kp69.isr != 0)
	{
		kp69.isr &= ~(1 << kp69_top_level((kp69.isr & kp69.pgr) != 0 ? (kp69.isr & kp69.pgr) : kp69.isr));
	}
	else
	{
		return;
	}

	kp69_set_int(kp69_int_active());
}

static struct z80_irq_daisy_chain daisy_chain[] =
{
	{ kp69_reset, kp69_irq_state, kp69_irq_ack, kp69_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static READ8_HANDLER( kp69_r )
{
	switch (offset)
	{
		case 0: return kp69.isr & 0xff;
		case 1: return kp69.isr >> 8;
		case 2: return kp69.imr & 0xff;
		default: return kp69.imr >> 8;
	}
}

static WRITE8_HANDLER( kp69_w )
{
	switch (offset)
	{
		/* until the vector is written the same registers address LER instead of PGR */
		case 0:
			if (kp69.ivr_written)
				kp69_set_pgr((kp69.pgr & 0xff00) | data);
			else
				kp69_set_ler((kp69.ler & 0xff00) | data);
			break;
		case 1:
			if (kp69.ivr_written)
				kp69_set_pgr((data << 8) | (kp69.pgr & 0x00ff));
			else
				kp69_set_ler((data << 8) | (kp69.ler & 0x00ff));
			break;
		case 2:
			kp69_set_imr((kp69.imr & 0xff00) | data);
			break;
		default:
			if (kp69.ivr_written)
			{
				kp69_set_imr((data << 8) | (kp69.imr & 0x00ff));
			}
			else
			{
				kp69.ivr = data & 0xe0;
				kp69.ivr_written = 1;
			}
			break;
	}
}


/***************************************************************************

    MMU

***************************************************************************/

static OPBASE_HANDLER( sammymdl_opbase )
{
	unsigned page = (address & 0xffff) >> 10;

	/* the opcodes are fetched directly from the physical memory */
	opcode_base = opcode_arg_base = physmem + (((page << 10) + mmu_base[page]) & (PHYS_SIZE - 1)) - (page << 10);
	opcode_mask = 0xffff;
	opcode_memory_min = page << 10;
	opcode_memory_max = (page << 10) | 0x3ff;

	return ~0;
}

static void mmu_remap(void)
{
	int n = 4;
	UINT32 base = 0xf0000; /* the last area is fixed */
	int i;

	for (i = 0x3f; i != 0; --i)
	{
		while (n != 0 && mmu_b[n] >= i)
		{
			--n;
			base = (UINT32)mmu_a[n] << 10;
		}
		mmu_base[i] = base;
	}

	/* the page under the program counter may have changed */
	memory_set_opbase(activecpu_get_pc());
}

static void mmu_reset(void)
{
	int i;

	for (i = 0; i < 4; ++i)
		mmu_a[i] = 0;
	for (i = 1; i <= 4; ++i)
		mmu_b[i] = 0x3f;
	for (i = 0; i < 64; ++i)
		mmu_base[i] = 0;
}

static READ8_HANDLER( mmu_r )
{
	int n = (offset >> 1) + 1;

	if (offset & 1)
		return n == 4 ? 0xf0 : (mmu_a[n] & 0x3fc) >> 2;
	else
		return (n == 4 ? 0 : (mmu_a[n] & 0x003) << 6) | mmu_b[n];
}

static WRITE8_HANDLER( mmu_w )
{
	int n = (offset >> 1) + 1;

	if (offset & 1)
	{
		if (n != 4)
			mmu_a[n] = (data << 2) | (mmu_a[n] & 0x003);
	}
	else
	{
		mmu_b[n] = data & 0x3f;
		if (n != 4)
			mmu_a[n] = (mmu_a[n] & 0x3fc) | ((data & 0xc0) >> 6);
	}

	mmu_remap();
}

static UINT8 vregs_r(int offset);
static void vregs_w(int offset, UINT8 data);
static void palette_w(int offset, UINT8 data);

static READ8_HANDLER( memory_r )
{
	UINT32 phys = (offset + mmu_base[offset >> 10]) & (PHYS_SIZE - 1);

	if (page_type[phys >> 10] == PAGE_VREGS)
		return vregs_r(phys & 0x3ff);

	return physmem[phys];
}

static WRITE8_HANDLER( memory_w )
{
	UINT32 phys = (offset + mmu_base[offset >> 10]) & (PHYS_SIZE - 1);

	switch (page_type[phys >> 10])
	{
		case PAGE_RAM:
			physmem[phys] = data;
			break;
		case PAGE_PALETTE:
			palette_w(phys & 0x3ff, data);
			break;
		case PAGE_VREGS:
			vregs_w(phys & 0x3ff, data);
			break;
	}
}

static void map_pages(UINT32 start, UINT32 end, int type)
{
	UINT32 page;

	for (page = start >> 10; page <= end >> 10; ++page)
		page_type[page] = type;
}


/***************************************************************************

    Video

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

***************************************************************************/

#define SPRITERAM (physmem + 0x70000)

static int integer_part(int x)
{
	return (x + 0x8000) >> 16;
}

static VIDEO_START( sammymdl )
{
	sprite_bitmap = auto_bitmap_alloc(512, 512);
	return 0;
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect, int pri_mask)
{
	UINT8 *end = SPRITERAM - 0x10;
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

static VIDEO_UPDATE( sammymdl )
{
	fillbitmap(bitmap, Machine->pens[0x100], cliprect);

	/* from the back (priority 3) to the front (priority 0) */
	draw_sprites(bitmap, cliprect, 8);
	draw_sprites(bitmap, cliprect, 4);
	draw_sprites(bitmap, cliprect, 2);
	draw_sprites(bitmap, cliprect, 1);
}

static void palette_w(int offset, UINT8 data)
{
	UINT8 *palette = physmem + 0x72000;

	if (offset < 0x200)
	{
		int color = offset >> 1;
		int data16;

		palette[offset] = data;
		data16 = (palette[color * 2] << 8) | palette[color * 2 + 1];
		palette_set_color(color, pal5bit(data16 >> 10), pal5bit(data16 >> 5), pal5bit(data16));
	}
}

static UINT8 vregs_r(int offset)
{
	if (offset > 0x21)
		return 0;

	if (offset == 0x13)
	{
		switch (variant)
		{
			case VARIANT_ANIMALC:
				return (vblank_reg & 0x02) | 0x14;
			case VARIANT_GOCOWBOY:
				break;
			default:
				return cpu_getvblank() ? 0 : 0x1c;
		}
	}

	return physmem[0x73000 + offset];
}

static void vregs_w(int offset, UINT8 data)
{
	if (offset > 0x21)
		return;

	switch (variant)
	{
		case VARIANT_ANIMALC:
			if (offset == 0x11)
				return;
			if (offset == 0x13)
			{
				vblank_reg = data & 0x03;
				return;
			}
			break;
		case VARIANT_ITAZURAM:
			if (offset == 0x11 || offset == 0x13)
				return;
			break;
	}

	physmem[0x73000 + offset] = data;

	/* background color */
	if (offset == 0x1b || offset == 0x1d)
	{
		int x = (physmem[0x73000 + 0x1d] << 8) + physmem[0x73000 + 0x1b];
		palette_set_color(0x100, pal5bit(x >> 10), pal5bit(x >> 5), pal5bit(x));
	}
}


/***************************************************************************

    I/O

***************************************************************************/

static const UINT8 portb_masks[4] = { 0x00, 0x0f, 0xf0, 0xff };

static UINT8 portb_dir_mask(int n)
{
	return portb_masks[(portb_direction >> (n * 2)) & 3];
}

static void coin_counters(UINT8 data)
{
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);
	coin_counter_w(2, data & 0x04);
}

static void hopper_motors(UINT8 data)
{
	ticket_dispenser_0_w(0, ((data & 0x10) && !(data & 0x20)) ? 0x80 : 0x00);
	ticket_dispenser_1_w(0, ((data & 0x40) && !(data & 0x80)) ? 0x80 : 0x00);
}

static void port_out(int port, UINT8 data)
{
	switch (port)
	{
		case 0:
			EEPROM_write_bit((data & 0x40) ? 1 : 0);
			/* the eeprom core takes the inverted chip select as a reset line */
			EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
			EEPROM_set_clock_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);
			break;
		case 3:
			if (variant == VARIANT_HAEKAKA)
				coin_counter_w(0, data & 0x01);
			else
				coin_counters(data);
			break;
		case 4:
			if (variant == VARIANT_GOCOWBOY)
				hopper_motors(data);
			break;
	}
}

static UINT8 port_in(int port)
{
	UINT8 ret;

	switch (port)
	{
		case 0:
			return EEPROM_read_bit() ? 0x80 : 0;
		case 1:
			/* the hopper sensor is active low */
			ret = readinputport(1);
			if (variant != VARIANT_GOCOWBOY && (ticket_dispenser_0_r(0) & 0x80))
				ret &= ~0x20;
			return ret;
		case 2:
			ret = readinputport(0);
			if (variant == VARIANT_GOCOWBOY)
			{
				if (ticket_dispenser_0_r(0) & 0x80)
					ret &= ~0x04;
				if (ticket_dispenser_1_r(0) & 0x80)
					ret &= ~0x08;
			}
			return ret;
	}

	return 0;
}

static READ8_HANDLER( porta_r )
{
	int n = offset >> 1;
	UINT8 data, dir;

	if (offset & 1)
		return porta_direction[n];

	data = porta_data[n];
	dir = porta_direction[n];
	if (dir != 0xff)
		data = (data & dir) | (port_in(n) & ~dir);
	return data;
}

static WRITE8_HANDLER( porta_w )
{
	int n = offset >> 1;
	UINT8 dir = porta_direction[n];

	if (offset & 1)
	{
		if (dir == data)
			return;
		porta_direction[n] = dir = data;
		data = porta_data[n];
	}
	else
	{
		UINT8 old_data = porta_data[n];
		porta_data[n] = data;
		if (((old_data ^ data) & dir) == 0)
			return;
	}

	port_out(n, data & dir);
}

static READ8_HANDLER( portb_r )
{
	UINT8 data = portb_data[offset];
	UINT8 dir = portb_dir_mask(offset);

	if (dir != 0xff)
		data = (data & dir) | (port_in(2 + offset) & ~dir);
	return data;
}

static void portb_update_output(int n)
{
	port_out(2 + n, portb_data[n] & portb_dir_mask(n));
}

static WRITE8_HANDLER( portb_w )
{
	UINT8 old_data = portb_data[offset];

	portb_data[offset] = data;
	if (((old_data ^ data) & portb_dir_mask(offset)) != 0)
		portb_update_output(offset);
}

static READ8_HANDLER( portb_control_r )
{
	return portb_direction;
}

static WRITE8_HANDLER( portb_control_w )
{
	if ((data & 0xc0) == 0xc0)
	{
		/* bit set/reset */
		int b = (data >> 1) & 7;
		int n = (data >> 4) & 3;

		if (n < 3)
		{
			if (((portb_data[n] >> b) & 1) != (data & 1))
			{
				portb_data[n] ^= 1 << b;
				if ((portb_direction >> (n * 2 + (b < 4 ? 0 : 1))) & 1)
					portb_update_output(n);
			}
		}
		else if ((data & 0x0d) != 0x0d)
		{
			if (((portb_direction >> b) & 1) != (data & 1))
			{
				portb_direction ^= 1 << b;
				portb_update_output(b >> 1);
			}
		}
	}
	else if ((data & 0xc0) == 0)
	{
		UINT8 old_dir = portb_direction;

		portb_direction = data;
		if (((data ^ old_dir) & 0x03) != 0)
			portb_update_output(0);
		if (((data ^ old_dir) & 0x0c) != 0)
			portb_update_output(1);
		if (((data ^ old_dir) & 0x30) != 0)
			portb_update_output(2);
	}
}

static WRITE8_HANDLER( hopper_w )
{
	if (variant != VARIANT_GOCOWBOY)
		ticket_dispenser_0_w(0, (!(data & 0x01) && (data & 0x02)) ? 0x80 : 0x00);
}




#define TIMER_CLOCK 10000000.0

/***************************************************************************

    Timer A (two channels, only the frequency divide modes are working)

***************************************************************************/

static void *timera_count_timer[2], *timera_pulse_timer[2];
static UINT8 timera_status[2], timera_tmp[2];
static UINT16 timera_count[2], timera_cr[2], timera_or[2];
static int timera_enabled[2], timera_started[2], timera_read_msb[2], timera_write_msb[2];

static void timera_set_out(int n, int state)
{
	if (((timera_status[n] >> 6) & 1) != state)
	{
		if (state)
			timera_status[n] |= 0x40;
		else
			timera_status[n] &= 0xbf;
		kp69_set_input(11 + n, state);
	}
}

static void timera_start(int n, UINT32 count)
{
	timera_enabled[n] = 1;
	timer_adjust(timera_count_timer[n], (count + 1.0) / TIMER_CLOCK, n, TIME_NEVER);
}

static void timera_stop(int n)
{
	timera_enabled[n] = 0;
	timer_adjust(timera_count_timer[n], TIME_NEVER, n, TIME_NEVER);
	timer_adjust(timera_pulse_timer[n], TIME_NEVER, n, TIME_NEVER);
}

static UINT16 timera_value(int n)
{
	if (timera_enabled[n])
	{
		double clocks = timer_timeleft(timera_count_timer[n]) * TIMER_CLOCK;
		return clocks > 0xffff ? 0xffff : (UINT16)clocks;
	}
	return timera_count[n];
}

static void timera_reload(int n)
{
	timera_count[n] = (timera_status[n] & 0x20) ? 0xffff : timera_cr[n];
	if (timera_status[n] & 1)
	{
		if (timera_count[n] != 0)
			timera_start(n, timera_count[n]);
	}
	timera_started[n] = 1;

	/* the output is low before the pulse */
	if (((timera_status[n] >> 3) & 7) == 0 && (timera_status[n] & 4))
		timera_set_out(n, 0);
}

static void timera_pulse_off(int n)
{
	timera_set_out(n, (timera_status[n] & 0x10) && !(timera_status[n] & 2));
}

static void timera_underflow(int n)
{
	UINT8 status = timera_status[n];

	timera_enabled[n] = 0;

	if (status & 0x20)
	{
		timera_count[n] = 0xffff;
		if (status & 1)
			timera_start(n, timera_count[n]);
	}
	else if (status & 0x10)
	{
		/* pulse modes stop and wait for the retrigger */
		timera_set_out(n, !(status & 0x40));
		if ((status & 1) && (status & 8))
			timer_adjust(timera_pulse_timer[n], 1 / TIMER_CLOCK, n, TIME_NEVER);
		timera_started[n] = 0;
	}
	else if (status & 8)
	{
		/* alternating reload for the pwm mode */
		timera_count[n] = ((status & 0x40) ? ~timera_cr[n] : timera_cr[n]) & ((0x40 << (status & 0x06)) - 1);
		if (status & 1)
			timera_start(n, timera_count[n]);
		timera_set_out(n, !(status & 0x40));
	}
	else
	{
		/* frequency divide, automatic reload */
		timera_count[n] = timera_cr[n];
		if (status & 1)
		{
			timera_start(n, timera_count[n]);
			if (status & 4)
				timer_adjust(timera_pulse_timer[n], 1 / TIMER_CLOCK, n, TIME_NEVER);
		}
		timera_set_out(n, (status & 4) || !(status & 0x40));
	}
}

static void timera_reset(void)
{
	int n;

	for (n = 0; n < 2; ++n)
	{
		timera_stop(n);
		timera_count[n] = timera_cr[n] = timera_or[n] = 0xffff;
		timera_status[n] = 0x01;
		timera_read_msb[n] = timera_write_msb[n] = 0;
		timera_started[n] = 0;
		timera_set_out(n, 0);
	}
}

static void timera_control(int n, UINT8 data)
{
	switch ((data >> 3) & 7)
	{
		case 0:
			timera_status[n] = data & 0x3d;
			timera_cr[n] = timera_count[n] = 0xffff;
			timera_started[n] = 0;
			timera_set_out(n, 0);
			timera_stop(n);
			break;
		case 1:
			timera_status[n] = data & 0x3f;
			timera_cr[n] = 0xffff;
			timera_count[n] = (0x40 << (timera_status[n] & 0x06)) - 1;
			timera_started[n] = 0;
			timera_set_out(n, 0);
			timera_stop(n);
			break;
		case 2:
		case 3:
			timera_status[n] = data & 0x3f;
			timera_cr[n] = timera_count[n] = 0xffff;
			timera_started[n] = 0;
			timera_set_out(n, !(data & 2));
			timera_stop(n);
			break;
		case 4:
		case 5:
			timera_status[n] = data & 0x3f;
			timera_cr[n] = timera_count[n] = 0xffff;
			timera_started[n] = 0;
			timera_set_out(n, 0);
			timera_stop(n);
			break;
		case 7:
			if ((data & 3) == 0)
			{
				/* counter latch command */
				timera_or[n] = timera_value(n);
			}
			else
			{
				if (data & 1)
					timera_status[n] &= 0x7f;
				if (data & 2)
					timera_read_msb[n] = timera_write_msb[n] = 0;
			}
			break;
	}
}

static READ8_HANDLER( timera_r )
{
	int n = offset >> 1;

	if (offset & 1)
		return timera_status[n] & ((timera_status[n] & 0x20) ? 0xbf : 0xff);
	else
	{
		UINT8 data = ((timera_status[n] & 0x20) ? timera_cr[n] : timera_or[n]) >> (timera_read_msb[n] ? 8 : 0);
		timera_read_msb[n] = !timera_read_msb[n];
		return data;
	}
}

static WRITE8_HANDLER( timera_w )
{
	int n = offset >> 1;

	if (offset & 1)
		timera_control(n, data);
	else if (timera_write_msb[n])
	{
		timera_cr[n] = (data << 8) | timera_tmp[n];

		/* the initial count of the frequency divide and pwm modes starts the timer */
		if (!timera_started[n] && ((timera_status[n] >> 4) & 3) == 0)
			timera_reload(n);

		timera_write_msb[n] = 0;
	}
	else
	{
		timera_tmp[n] = data;
		timera_write_msb[n] = 1;
	}
}

/***************************************************************************

    Timer B (three channels, only the interrupt strobes are used)

***************************************************************************/


static void *timerb_timer[3];
static UINT8 timerb_status[3], timerb_tmp[3];
static UINT16 timerb_cr[3], timerb_last[3];
static int timerb_started, timerb_rw;

static void scanline_pulse(int level);

static double timerb_period(int n, UINT32 count)
{
	int prescale = (timerb_status[n] & 2) ? 4 : (timerb_status[n] & 1) ? 16 : 256;

	return prescale * (count + 1.0) / TIMER_CLOCK;
}

static UINT16 timerb_count(int n)
{
	int prescale = (timerb_status[n] & 2) ? 4 : (timerb_status[n] & 1) ? 16 : 256;

	if (!(timerb_started & (1 << n)))
		return timerb_last[n];

	return (UINT16)(timer_timeleft(timerb_timer[n]) * TIMER_CLOCK / prescale);
}

static void timerb_reload(int n)
{
	double period = timerb_period(n, timerb_cr[n]);

	timerb_started |= 1 << n;
	timer_adjust(timerb_timer[n], period, n, (timerb_status[n] & 4) ? period : TIME_NEVER);
}

static void timerb_expired(int n)
{
	scanline_pulse(13 + n);

	/* one shot and watchdog modes stop at the underflow */
	if (!(timerb_status[n] & 4))
	{
		timerb_last[n] = 0xffff;
		timerb_started &= ~(1 << n);
	}
}

static void timerb_reset(void)
{
	int n;

	for (n = 0; n < 3; ++n)
	{
		timer_adjust(timerb_timer[n], TIME_NEVER, n, TIME_NEVER);
		timerb_status[n] = 0;
		timerb_cr[n] = 0xffff;
		timerb_last[n] = 0xffff;
	}
	timerb_started = 0;
	timerb_rw = 0;
}

static READ8_HANDLER( timerb_r )
{
	int n = offset >> 1;
	UINT16 count;

	if (offset & 1)
	{
		timerb_rw &= ~(1 << n);
		return timerb_status[n];
	}

	if (timerb_rw & (1 << n))
	{
		timerb_rw &= ~(1 << n);
		return timerb_tmp[n];
	}

	/* the high byte is latched by the first read */
	count = timerb_count(n);
	timerb_rw |= 1 << n;
	timerb_tmp[n] = count >> 8;
	return count & 0xff;
}

static WRITE8_HANDLER( timerb_w )
{
	int n = offset >> 1;

	if (offset & 1)
	{
		/* the count is stopped before changing the mode */
		if (timerb_started & (1 << n))
		{
			timerb_last[n] = timerb_count(n);
			timer_adjust(timerb_timer[n], TIME_NEVER, n, TIME_NEVER);
			timerb_started &= ~(1 << n);
		}
		timerb_status[n] = data & 0x1f;
	}
	else if (timerb_rw & (1 << n))
	{
		timerb_rw &= ~(1 << n);
		timerb_cr[n] = (data << 8) | timerb_tmp[n];

		/* automatic retrigger in one shot and continuous modes */
		if (!(timerb_status[n] & 8) || !(timerb_started & (1 << n)))
			timerb_reload(n);
	}
	else if ((timerb_status[n] & 0x0c) == 0x08)
	{
		timerb_reload(n);
	}
	else
	{
		timerb_rw |= 1 << n;
		timerb_tmp[n] = data;
	}
}

/***************************************************************************

    Memory maps

***************************************************************************/

static ADDRESS_MAP_START( sammymdl_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(memory_r, memory_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sammymdl_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x07) AM_READWRITE(mmu_r, mmu_w)
	AM_RANGE(0x20, 0x25) AM_READWRITE(timerb_r, timerb_w)
	AM_RANGE(0x28, 0x2b) AM_READWRITE(timera_r, timera_w)
	AM_RANGE(0x2c, 0x2f) AM_READWRITE(porta_r, porta_w)
	AM_RANGE(0x30, 0x32) AM_READWRITE(portb_r, portb_w)
	AM_RANGE(0x33, 0x33) AM_READWRITE(portb_control_r, portb_control_w)
	AM_RANGE(0x34, 0x37) AM_READWRITE(kp69_r, kp69_w)
	AM_RANGE(0x90, 0x90) AM_READWRITE(OKIM9810_status_0_r, OKIM9810_data_0_w)
	AM_RANGE(0x91, 0x91) AM_WRITE(OKIM9810_tmp_0_w)
	AM_RANGE(0x92, 0x92) AM_READ(OKIM9810_status_0_r)
	AM_RANGE(0xb0, 0xb0) AM_WRITE(hopper_w)
	AM_RANGE(0xc0, 0xc0) AM_WRITE(MWA8_NOP)
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

INPUT_PORTS_START( sammymdl )
	PORT_START_TAG("BUTTON")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_IMPULSE(5) PORT_NAME("Medal")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* hopper */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( haekaka )
	PORT_START_TAG("BUTTON")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5) PORT_NAME("Medal")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* hopper */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( gocowboy )
	PORT_START_TAG("BUTTON")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(20)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* small hopper sensor */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* large hopper sensor */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Meter Switch")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Reset Switch")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("COIN")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

    Machine drivers

***************************************************************************/

static void vblank_end(int level)
{
	kp69_set_input(level, 0);
}

static void scanline_pulse(int level)
{
	kp69_set_input(level, 1);
	kp69_set_input(level, 0);
}

static INTERRUPT_GEN( sammymdl_interrupt )
{
	switch (variant)
	{
		case VARIANT_ANIMALC:
			kp69_set_input(0, 1);
			timer_set(TIME_IN_USEC(2500), 0, vblank_end);
			break;
		case VARIANT_ITAZURAM:
		case VARIANT_GOCOWBOY:
			scanline_pulse(0);
			timer_set(cpu_getscanlinetime(128), 1, scanline_pulse);
			break;
		default:
			kp69_set_input(2, 1);
			timer_set(TIME_IN_USEC(2500), 2, vblank_end);
			break;
	}
}

static MACHINE_RESET( sammymdl )
{
	int i;

	mmu_reset();
	vblank_reg = 0;

	for (i = 0; i < 2; ++i)
		porta_data[i] = porta_direction[i] = 0;
	for (i = 0; i < 3; ++i)
		portb_data[i] = 0;
	portb_direction = 0;

	timera_reset();
	timerb_reset();
	kp69_reset(0);
	kp69.levels = 0;
	kp69.ivr = 0;

	memory_set_opbase_handler(0, sammymdl_opbase);
	catch_nextBranch();
}

static struct EEPROM_interface eeprom_interface_93C46_8bit =
{
	7,				/* address bits */
	8,				/* data bits */
	"*110",			/* read */
	"*101",			/* write */
	"*111",			/* erase */
	"*10000xxxxx",	/* lock */
	"*10011xxxxx",	/* unlock */
	1,
	1				/* the games wait for the busy state after a write */
};

static NVRAM_HANDLER( sammymdl )
{
	if (read_or_write)
	{
		EEPROM_save(file);
		mame_fwrite(file, physmem + nvram_base, nvram_size);
	}
	else
	{
		EEPROM_init(&eeprom_interface_93C46_8bit);

		if (file)
		{
			EEPROM_load(file);
			mame_fread(file, physmem + nvram_base, nvram_size);
		}
		else
		{
			memset(physmem + nvram_base, 0, nvram_size);

			/* the backup ram test of the BIOS fails, and keeps failing on the next boots,
			   without this signature: start with a state that passes it */
			if (variant != VARIANT_GOCOWBOY)
			{
				physmem[nvram_base] = 0xaa;
				memcpy(physmem + nvram_base + 1, "SAMMY CORPORATION", 17);
			}

			/* the eeprom of Go Go Cowboy has factory defaults */
			if (variant == VARIANT_GOCOWBOY)
				EEPROM_set_data(memory_region(REGION_USER2), 0x80);
		}
	}
}

static struct OKIM9810interface okim9810_intf =
{
	REGION_SOUND1
};

static MACHINE_DRIVER_START( sammymdl )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 20000000)
	MDRV_CPU_CONFIG(daisy_chain)
	MDRV_CPU_PROGRAM_MAP(sammymdl_map, 0)
	MDRV_CPU_IO_MAP(sammymdl_io_map, 0)
	MDRV_CPU_VBLANK_INT(sammymdl_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)
	MDRV_MACHINE_RESET(sammymdl)
	MDRV_NVRAM_HANDLER(sammymdl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0x100)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xf0-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100 + 1)

	MDRV_VIDEO_START(sammymdl)
	MDRV_VIDEO_UPDATE(sammymdl)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(OKIM9810, 4096000)
	MDRV_SOUND_CONFIG(okim9810_intf)
	MDRV_SOUND_ROUTE(0, "left", 0.80)
	MDRV_SOUND_ROUTE(1, "right", 0.80)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tdoboon )
	MDRV_IMPORT_FROM(sammymdl)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0+4, 0xf0+4-1)
MACHINE_DRIVER_END


/***************************************************************************

    ROMs loading

***************************************************************************/

SYSTEM_BIOS_START( sammymdl )
	SYSTEM_BIOS_ADD( 0, "v5", "IPL Ver. 5.0" )
	SYSTEM_BIOS_ADD( 1, "v4", "IPL Ver. 4.0" )
	SYSTEM_BIOS_ADD( 2, "v1", "IPL Ver. 1.0" )
SYSTEM_BIOS_END

/* Archive prefixes keep the shared BIOS in sammymdl.zip without changing game parents. */
#define SAMMYMDL_BIOS_LOAD(name,offset,length,hash,flags) \
	BIOSX_LOAD("sammymdl", name, offset, length, hash, flags)

#define SAMMYMDL_BIOS(load) \
	load( "vm1211l01.u2",   0x000000, 0x080000, CRC(c3c74dc5) SHA1(07352e6dba7514214e778ba39e1ca773e4698858), ROM_BIOS(1) ) \
	load( "mt201l04.u012",  0x000000, 0x080000, CRC(c8c6d25f) SHA1(5be39fa72b65f2e455ccc146dbab58d24ab46505), ROM_BIOS(2) ) \
	load( "vm1201l01.u012", 0x000000, 0x080000, CRC(21b2ccbd) SHA1(c3ceea196e5c1c6a9e03cf3a22e9f25f0099a38e), ROM_BIOS(3) )

ROM_START( sammymdl )
	ROM_BIOS_DEFAULT( "v5" )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	SAMMYMDL_BIOS( ROMX_LOAD )
ROM_END

/* the BIOS patches are meant for the v5 one */
#define SAMMYMDL_CART(oki_name, oki_crc, spr_name, spr_crc) \
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) \
	SAMMYMDL_BIOS( SAMMYMDL_BIOS_LOAD ) \
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF ) \
	ROM_LOAD( oki_name, 0x00000, 0x200000, oki_crc ) \
	ROM_REGION( 0x40000, REGION_USER1, 0 ) \
	ROM_COPY( REGION_SOUND1, 0x1c0000, 0x00000, 0x40000 ) \
	ROM_REGION( 0x200000, REGION_GFX1, 0 ) \
	ROM_LOAD( spr_name, 0x00000, 0x200000, spr_crc )

ROM_START( kingyosu )
	SAMMYMDL_CART( "vx1302l02.u021", CRC(45e43e3c) SHA1(71ada38c1f65738ca75ca75d2272f310ba43c72c), "vx1301l01.u016", CRC(25dfbf4c) SHA1(8f9552cf54bdf7490055ebdb23fa758f5cddafb5) )
ROM_END

ROM_START( shatekds )
	SAMMYMDL_CART( "29f016a.u015", CRC(7c779286) SHA1(db688ac2cad14c642e6da770212217a466e1bb64), "mx29f1610tc.u016", CRC(6e7d2c7d) SHA1(747d54eee26631966a91fd26b8b4beb32b67ddb7) )
ROM_END

ROM_START( animalc )
	SAMMYMDL_CART( "vx2302l01.u021", CRC(84cf123b) SHA1(d8b425c93ff1a560e3f92c70d7eb93a05c3581af), "vx2301l01.u016", CRC(4ae14ff9) SHA1(1273d15ea642452fecacff572655cd3ab47a5884) )
ROM_END

ROM_START( itazuram )
	SAMMYMDL_CART( "vx2002l01.u021", CRC(ddbdd2f3) SHA1(91f67a938929be0261442e066e3d2c03b5e9f06a), "vx2001l01.u016", CRC(9ee95222) SHA1(7154d43ef312a48a882207ca37e1c61e8b215a9b) )
ROM_END

ROM_START( pyenaget )
	SAMMYMDL_CART( "vx1802l01.u021", CRC(7a22a657) SHA1(2a98085862fd958209253c5401e41eae4f7c06ea), "vx1801l01.u016", CRC(c4607403) SHA1(f4f4699442afccc5ed4354447f91b1bee36ae3e5) )
ROM_END

ROM_START( tdoboon )
	SAMMYMDL_CART( "em4210l01.u021", CRC(3523e314) SHA1(d07c5d17d3f285be4cde810547f427e84f98968f), "em4209l01.u016", CRC(aca220fa) SHA1(7db441add16af554700e597fd9926b6ccd19d628) )
ROM_END

ROM_START( haekaka )
	SAMMYMDL_CART( "em4208l01.u021", CRC(d23bb748) SHA1(38d5b6c4b2cd470b3a68574aeca3f9fa9032245e), "em4207l01.u016", CRC(3876961c) SHA1(3d842c1f63ea5aa7e799967928b86c5fabb4e65e) )
ROM_END

ROM_START( kotekita )
	SAMMYMDL_CART( "vx2302l01.u021", CRC(37faa4a9) SHA1(619135d74052fb2272ce78976c2aa59a65244f12), "vx2301l01.u016", CRC(e471c1ef) SHA1(4ddf3261d8736888ebbdbd0d755aad75987b3b3d) )
ROM_END

ROM_START( gunkids )
	SAMMYMDL_CART( "vx2302l01.u021", CRC(3d989a45) SHA1(f4e1dc13bfe9ef8bf733735b6647946dda6962f2), "vx2301l01.u016", CRC(5e356b68) SHA1(0e4e28b02dcb5ff7d2a7139c5cdf31cbd08167f4) )
ROM_END

ROM_START( wantouch )
	SAMMYMDL_CART( "vx2302l01.u021", CRC(fe00ac87) SHA1(97c3defafd04f48984e5f2fe34528ada024355c5), "vx2301l01.u016", CRC(cd8d4478) SHA1(224e8e0e9a49f10515e0ebf9a5e97d8adc7eed13) )
ROM_END

ROM_START( gocowboy )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD( "go_go_cowboy_gpt_2c9c.u3", 0x00000, 0x80000, CRC(ad9b1de6) SHA1(24809ec3a579d28189a98190db70a33217e4f8bc) )

	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )
	ROM_LOAD( "em702l01.u021", 0x000000, 0x200000, CRC(4c4289fe) SHA1(517b5a1e9d91e7ed322b4792d863e7abda835d4a) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )
	ROM_LOAD( "em701l01.u016", 0x000000, 0x200000, CRC(c1f07320) SHA1(734717140e66ddcf0bded1489156c51cdaf1b50c) )

	ROM_REGION( 0x80, REGION_USER2, 0 )
	ROM_LOAD( "st93c46.u6", 0x00, 0x80, CRC(1af2d376) SHA1(afbe953f1a9ff0152fe1092a83482695dbe5e75d) )
ROM_END


/***************************************************************************

    Driver initialization

***************************************************************************/

static void init_memory(int type, UINT32 nvram_start, UINT32 nvram_len)
{
	UINT8 *bios = memory_region(REGION_CPU1);
	int i;

	physmem = auto_malloc(PHYS_SIZE);
	memset(physmem, 0, PHYS_SIZE);
	memset(page_type, PAGE_ROM, sizeof(page_type));

	variant = type;
	nvram_base = nvram_start;
	nvram_size = nvram_len;

	if (type == VARIANT_GOCOWBOY)
	{
		memcpy(physmem, bios, 0x60000);
	}
	else
	{
		memcpy(physmem, bios, 0x10000);
		memcpy(physmem + 0x10000, memory_region(REGION_USER1) + 0x400, 0x20000);
	}

	map_pages(nvram_start, nvram_start + nvram_len - 1, PAGE_RAM);
	map_pages(0x70000, 0x70fff, PAGE_RAM);
	map_pages(0xffe00, 0xfffff, PAGE_RAM);
	map_pages(0x72000, 0x721ff, PAGE_PALETTE);
	map_pages(0x72800, 0x7287f, PAGE_RAM);
	map_pages(0x73000, 0x73021, PAGE_VREGS);

	for (i = 0; i < 2; ++i)
	{
		timera_count_timer[i] = timer_alloc(timera_underflow);
		timera_pulse_timer[i] = timer_alloc(timera_pulse_off);
	}
	for (i = 0; i < 3; ++i)
		timerb_timer[i] = timer_alloc(timerb_expired);

	ticket_dispenser_init(200, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);
}

static void patch_bios_loop(void)
{
	/* force jump out of the BIOS loop */
	memory_region(REGION_CPU1)[0x005ac] = 0xc3;
}

static DRIVER_INIT( animalc )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	/* video timing loops */
	rom[0x015d9] = 0x00;
	rom[0x015da] = 0x00;
	rom[0x01605] = 0x00;
	rom[0x01606] = 0x00;
	rom[0x01750] = 0x00;
	rom[0x01751] = 0x00;

	patch_bios_loop();
	init_memory(VARIANT_ANIMALC, 0x60000, 0x4000);
	map_pages(0x6a000, 0x6ffff, PAGE_RAM);
}

static DRIVER_INIT( kingyosu )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	rom[0x015d9] = 0x00;
	rom[0x015da] = 0x00;
	rom[0x01605] = 0x00;
	rom[0x01606] = 0x00;
	rom[0x01750] = 0x00;
	rom[0x01751] = 0x00;

	patch_bios_loop();
	init_memory(VARIANT_ITAZURAM, 0x60000, 0x4000);
}

static DRIVER_INIT( itazuram )
{
	patch_bios_loop();
	init_memory(VARIANT_ITAZURAM, 0x60000, 0x4000);
}

static DRIVER_INIT( pyenaget )
{
	patch_bios_loop();
	init_memory(VARIANT_PYENAGET, 0x60000, 0x2000);
}

static DRIVER_INIT( tdoboon )
{
	patch_bios_loop();
	init_memory(VARIANT_TDOBOON, 0x60000, 0x2000);
}

static DRIVER_INIT( haekaka )
{
	patch_bios_loop();
	init_memory(VARIANT_HAEKAKA, 0x60000, 0x2000);
}

static DRIVER_INIT( gocowboy )
{
	init_memory(VARIANT_GOCOWBOY, 0x60000, 0x8000);
}


/***************************************************************************

    Game drivers

***************************************************************************/

DEVICEB( 1999, sammymdl,          sammymdl, "Sammy", "Sammy Medal Game System BIOS" )

GAMEB( 1999, kingyosu, 0, sammymdl, sammymdl, sammymdl, kingyosu, ROT0, "Sammy", "Dokidoki Kingyo Sukui", 0 )
GAMEB( 1999, shatekds, 0, sammymdl, sammymdl, sammymdl, animalc, ROT0, "Sammy", "Shatekids", 0 )
GAMEB( 2000, animalc, 0, sammymdl, sammymdl, sammymdl, animalc, ROT0, "Sammy", "Animal Catch", 0 )
GAMEB( 2000, itazuram, 0, sammymdl, sammymdl, sammymdl, itazuram, ROT0, "Sammy", "Itazura Monkey", 0 )
GAMEB( 2000, pyenaget, 0, sammymdl, sammymdl, sammymdl, pyenaget, ROT0, "Sammy", "Pye-nage Taikai", 0 )
GAMEB( 2000, tdoboon, 0, sammymdl, tdoboon, haekaka, tdoboon, ROT0, "Sammy", "Taihou de Doboon", 0 )
GAMEB( 2001, haekaka, 0, sammymdl, sammymdl, haekaka, haekaka, ROT0, "Sammy", "Hae Hae Ka Ka Ka", 0 )
GAMEB( 2001, kotekita, 0, sammymdl, sammymdl, sammymdl, haekaka, ROT0, "Sammy", "Kotekitai Slot", 0 )
GAMEB( 2002, gunkids, 0, sammymdl, sammymdl, sammymdl, animalc, ROT0, "Sammy", "Hayauchi Gun Kids", 0 )
GAMEB( 2003, wantouch, 0, sammymdl, sammymdl, sammymdl, animalc, ROT0, "Sammy", "Wantouchable", 0 )
GAME( 2003, gocowboy, 0, sammymdl, gocowboy, gocowboy, ROT0, "Sammy", "Go Go Cowboy (English, prize)", 0 )
