// license:BSD-3-Clause
// copyright-holders:Luca Elia
/***************************************************************************

                      -= IGS Lord Of Gun =-

                    driver by   Luca Elia (l.elia@tin.it)
           skeleton driver by   David Haywood
            code decrypted by   unknown


CPU     :   68000
Sound   :   Z80 + M6295 [+ M6295] + YM3812
NVRAM   :   93C46

-----------------------------------------------------------------------------------
Year + Game                PCB    FM Sound  Chips                         Notes
-----------------------------------------------------------------------------------
1994  Lord Of Gun (World)  T0062  YM3812    IGS005 IGS006 IGS007 IGS008   Lightguns
1994  Lord Of Gun (USA)    T0076  YM3812    IGS005 IGS006 IGS007 IGS008   Lightguns
1994  Alien Challenge      NO-0085 YMF278B IGS005 IGS006 IGS007 IGS008   Not encrypted
-----------------------------------------------------------------------------------

To do:

- Protection emulation instead of patching the roms
- Priorities

***************************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "machine/eeprom.h"
#include "sound/3812intf.h"
#include "sound/okim6295.h"
#include "sound/ymf278b.h"

// Variables defined in vidhrdw:

extern UINT16 *lordgun_vram_0, *lordgun_scroll_x_0, *lordgun_scroll_y_0;
extern UINT16 *lordgun_vram_1, *lordgun_scroll_x_1, *lordgun_scroll_y_1;
extern UINT16 *lordgun_vram_2, *lordgun_scroll_x_2, *lordgun_scroll_y_2;
extern UINT16 *lordgun_vram_3, *lordgun_scroll_x_3, *lordgun_scroll_y_3;
extern UINT16 *lordgun_scrollram;
static UINT16 *lordgun_priority_ram, lordgun_priority;
static UINT8 aliencha_dip_sel, aliencha_protection_data;
extern int lordgun_whitescreen;

extern struct
{
	int		scr_x,	scr_y;
	UINT16	hw_x,	hw_y;
}	lordgun_gun[2];

// Functions defined in vidhrdw:

WRITE16_HANDLER( lordgun_vram_0_w );
WRITE16_HANDLER( lordgun_vram_1_w );
WRITE16_HANDLER( lordgun_vram_2_w );
WRITE16_HANDLER( lordgun_vram_3_w );

void lordgun_update_gun(int i);

VIDEO_START( lordgun );
VIDEO_UPDATE( lordgun );

/***************************************************************************

    Code Decryption

***************************************************************************/

static DRIVER_INIT( lordgun )
{
	int i;
	UINT16 *src = (UINT16 *)memory_region(REGION_CPU1);

	int rom_size = 0x100000;

	for(i=0; i<rom_size/2; i++) {
		UINT16 x = src[i];

		if((i & 0x0120) == 0x0100 || (i & 0x0a00) == 0x0800)
			x ^= 0x0010;

		src[i] = x;
	}
}

/***************************************************************************

    Protection

***************************************************************************/

static WRITE16_HANDLER( lordgun_protection_w )
{
	switch (offset & 0x60)
	{
		case 0x00/2:	// increment counter
			aliencha_protection_data = (aliencha_protection_data + 1) & 0x1f;
			break;
		case 0xc0/2:	// reset protection device
			aliencha_protection_data = 0;
			break;
	}
}

static READ16_HANDLER( lordgun_protection_r )
{
	UINT8 x;
	switch (offset & 0x60)
	{
		case 0x40/2:	// bitswap and xor counter
			x = aliencha_protection_data;
			aliencha_protection_data = ((((x >> 0) | (x >> 1)) & 1) << 4) |
				(((~x >> 2) & 1) << 3) |
				((((~x >> 4) | (x >> 0)) & 1) << 2) |
				(((x >> 3) & 1) << 1) |
				((((~x >> 0) | (x >> 2)) & 1) << 0);
			return 0;
		case 0x80/2:	// return value if conditions are met
			if ((aliencha_protection_data & 0x11) == 0x01) return 0x10;
			if ((aliencha_protection_data & 0x06) == 0x02) return 0x10;
			if ((aliencha_protection_data & 0x09) == 0x08) return 0x10;
			return 0;
	}
	return 0;
}

/***************************************************************************

    Memory Maps - Main

***************************************************************************/

static WRITE8_HANDLER(fake_w)
{
}
static WRITE8_HANDLER(fake2_w)
{
//  ui_popup("%02x",data);
}

static READ8_HANDLER( lordgun_eeprom_r )
{
	return readinputport(0) | ((EEPROM_read_bit() & 1) << 7);
}

static WRITE8_HANDLER( lordgun_eeprom_w )
{
	static UINT8 old;
	int i;

	if (data & ~0xfd)
	{
//      ui_popup("EE: %02x", data);
		logerror("PC %06X - Unknown EEPROM bit written %02X\n",activecpu_get_pc(),data);
	}

	coin_counter_w(0, data & 0x01);

	// Update light guns positions
	for (i = 0; i < 2; i++)
		if ( (data & (0x04 << i)) && !(old & (0x04 << i)) )
			lordgun_update_gun(i);

	// latch the bit
	EEPROM_write_bit(data & 0x40);

	// reset line asserted: reset.
	EEPROM_set_cs_line((data & 0x10) ? CLEAR_LINE : ASSERT_LINE );

	// clock line asserted: write latch or select next bit to read
	EEPROM_set_clock_line((data & 0x20) ? ASSERT_LINE : CLEAR_LINE );

	lordgun_whitescreen = data & 0x80;

	old = data;
}

static READ8_HANDLER( aliencha_dip_r )
{
	switch (aliencha_dip_sel & 0x70)
	{
		case 0x30: return readinputport(0);
		case 0x60: return readinputport(1);
		case 0x50: return readinputport(2);
	}
	return 0xff;
}

static READ8_HANDLER( aliencha_eeprom_r )
{
	return (EEPROM_read_bit() & 1) | (readinputport(3) & 0xfe);
}

static WRITE8_HANDLER( aliencha_eeprom_w )
{
	lordgun_whitescreen = !(data & 0x02);
	coin_counter_w(0, data & 0x08);
	coin_counter_w(1, data & 0x10);
	EEPROM_write_bit(data & 0x80);
	EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
	EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE8_HANDLER( aliencha_dip_w ) { aliencha_dip_sel = data; }

static READ16_HANDLER( aliencha_protection_r )
{
	UINT8 x;
	switch (offset & 0x60)
	{
		case 0x00/2:
			aliencha_protection_data = (aliencha_protection_data - 1) & 0x1f;
			return 0;
		case 0x40/2:
			x = aliencha_protection_data;
			aliencha_protection_data = ((((x >> 3) ^ (x >> 2)) & 1) << 4) |
				((((x >> 2) ^ (x >> 1)) & 1) << 3) |
				((((x >> 1) ^ x) & 1) << 2) |
				((((x >> 4) ^ x) & 1) << 1) |
				(((x >> 4) ^ (x >> 3)) & 1);
			return 0;
		case 0x80/2:
			return ((aliencha_protection_data & 0x11) == 0 ||
				(aliencha_protection_data & 0x06) != 0x06 ||
				(aliencha_protection_data & 0x18) == 0) ? 0x20 : 0;
	}
	return 0;
}

static WRITE16_HANDLER( aliencha_protection_w )
{
	if ((offset & 0x60) == 0xc0/2)
		aliencha_protection_data = 0;
}

static WRITE16_HANDLER( lordgun_priority_w )
{
	COMBINE_DATA(&lordgun_priority);
//  ui_popup("PR: %04x", data);
}

static READ16_HANDLER( lordgun_ppi8255_0_r )	{	return ppi8255_0_r(offset);	}
static READ16_HANDLER( lordgun_ppi8255_1_r )	{	return ppi8255_1_r(offset);	}

static WRITE16_HANDLER( lordgun_ppi8255_0_w )	{	ppi8255_0_w(offset, data & 0xff);	}
static WRITE16_HANDLER( lordgun_ppi8255_1_w )	{	ppi8255_1_w(offset, data & 0xff);	}

static READ16_HANDLER( lordgun_gun_0_x_r )		{ return lordgun_gun[0].hw_x; }
static READ16_HANDLER( lordgun_gun_0_y_r )		{ return lordgun_gun[0].hw_y; }
static READ16_HANDLER( lordgun_gun_1_x_r )		{ return lordgun_gun[1].hw_x; }
static READ16_HANDLER( lordgun_gun_1_y_r )		{ return lordgun_gun[1].hw_y; }


static WRITE16_HANDLER( lordgun_soundlatch_w )
{
	if (ACCESSING_LSB)	soundlatch_w (0, (data >> 0) & 0xff);
	if (ACCESSING_MSB)	soundlatch2_w(0, (data >> 8) & 0xff);

	cpunum_set_input_line(1, INPUT_LINE_NMI, PULSE_LINE);
}

static ADDRESS_MAP_START( lordgun_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x210000, 0x21ffff) AM_RAM AM_BASE(&lordgun_priority_ram)	// PRIORITY
	AM_RANGE(0x300000, 0x30ffff) AM_READWRITE(MRA16_RAM, lordgun_vram_0_w) AM_BASE(&lordgun_vram_0)	// DISPLAY
	AM_RANGE(0x310000, 0x313fff) AM_READWRITE(MRA16_RAM, lordgun_vram_1_w) AM_BASE(&lordgun_vram_1)	// DISPLAY
	AM_RANGE(0x314000, 0x317fff) AM_READWRITE(MRA16_RAM, lordgun_vram_2_w) AM_BASE(&lordgun_vram_2)	// DISPLAY
	AM_RANGE(0x318000, 0x319fff) AM_READWRITE(MRA16_RAM, lordgun_vram_3_w) AM_BASE(&lordgun_vram_3)	// DISPLAY
	AM_RANGE(0x31c000, 0x31c7ff) AM_RAM AM_BASE(&lordgun_scrollram)		// LINE
	AM_RANGE(0x400000, 0x4007ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)	// ANIMATOR
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM, paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x502000, 0x502001) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_0)
	AM_RANGE(0x502200, 0x502201) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_1)
	AM_RANGE(0x502400, 0x502401) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_2)
	AM_RANGE(0x502600, 0x502601) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_3)
	AM_RANGE(0x502800, 0x502801) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_0)
	AM_RANGE(0x502a00, 0x502a01) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_1)
	AM_RANGE(0x502c00, 0x502c01) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_2)
	AM_RANGE(0x502e00, 0x502e01) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_3)
	AM_RANGE(0x503000, 0x503001) AM_WRITE(lordgun_priority_w)
	AM_RANGE(0x503800, 0x503801) AM_READ(lordgun_gun_0_x_r)
	AM_RANGE(0x503a00, 0x503a01) AM_READ(lordgun_gun_1_x_r)
	AM_RANGE(0x503c00, 0x503c01) AM_READ(lordgun_gun_0_y_r)
	AM_RANGE(0x503e00, 0x503e01) AM_READ(lordgun_gun_1_y_r)
	AM_RANGE(0x504000, 0x504001) AM_WRITE(lordgun_soundlatch_w)
	AM_RANGE(0x506000, 0x506007) AM_READWRITE(lordgun_ppi8255_0_r, lordgun_ppi8255_0_w)
	AM_RANGE(0x508000, 0x508007) AM_READWRITE(lordgun_ppi8255_1_r, lordgun_ppi8255_1_w)
	AM_RANGE(0x50a900, 0x50a9ff) AM_READWRITE(lordgun_protection_r, lordgun_protection_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( aliencha_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x210000, 0x21ffff) AM_RAM AM_BASE(&lordgun_priority_ram)	// PRIORITY
	AM_RANGE(0x300000, 0x30ffff) AM_READWRITE(MRA16_RAM, lordgun_vram_0_w) AM_BASE(&lordgun_vram_0)	// DISPLAY
	AM_RANGE(0x310000, 0x313fff) AM_READWRITE(MRA16_RAM, lordgun_vram_1_w) AM_BASE(&lordgun_vram_1)	// DISPLAY
	AM_RANGE(0x314000, 0x317fff) AM_READWRITE(MRA16_RAM, lordgun_vram_2_w) AM_BASE(&lordgun_vram_2)	// DISPLAY
	AM_RANGE(0x318000, 0x319fff) AM_READWRITE(MRA16_RAM, lordgun_vram_3_w) AM_BASE(&lordgun_vram_3)	// DISPLAY
	AM_RANGE(0x31c000, 0x31c7ff) AM_RAM AM_BASE(&lordgun_scrollram)		// LINE
	AM_RANGE(0x400000, 0x4007ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)	// ANIMATOR
	AM_RANGE(0x500000, 0x500fff) AM_READWRITE(MRA16_RAM, paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x502000, 0x502001) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_0)
	AM_RANGE(0x502200, 0x502201) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_1)
	AM_RANGE(0x502400, 0x502401) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_2)
	AM_RANGE(0x502600, 0x502601) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_x_3)
	AM_RANGE(0x502800, 0x502801) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_0)
	AM_RANGE(0x502a00, 0x502a01) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_1)
	AM_RANGE(0x502c00, 0x502c01) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_2)
	AM_RANGE(0x502e00, 0x502e01) AM_WRITE(MWA16_RAM) AM_BASE(&lordgun_scroll_y_3)
	AM_RANGE(0x503000, 0x503001) AM_WRITE(lordgun_priority_w)
	AM_RANGE(0x504000, 0x504001) AM_WRITE(lordgun_soundlatch_w)
	AM_RANGE(0x506000, 0x506007) AM_READWRITE(lordgun_ppi8255_0_r, lordgun_ppi8255_0_w)
	AM_RANGE(0x508000, 0x508007) AM_READWRITE(lordgun_ppi8255_1_r, lordgun_ppi8255_1_w)
	AM_RANGE(0x50b900, 0x50b9ff) AM_READWRITE(aliencha_protection_r, aliencha_protection_w)
ADDRESS_MAP_END

/***************************************************************************

    Memory Maps - Sound

***************************************************************************/

static ADDRESS_MAP_START( lordgun_soundmem_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_RAM
ADDRESS_MAP_END

static WRITE8_HANDLER( lordgun_okibank_w )
{
	OKIM6295_set_bank_base(0, (data & 2) ? 0x40000 : 0);
	if (data & ~3)	logerror("%04x: unknown okibank bits %02x\n", activecpu_get_pc(), data);
//  ui_popup("OKI %x", data);
}

static ADDRESS_MAP_START( lordgun_soundio_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x1000, 0x1000) AM_WRITE( YM3812_control_port_0_w )
	AM_RANGE(0x1001, 0x1001) AM_WRITE( YM3812_write_port_0_w )
	AM_RANGE(0x2000, 0x2000) AM_READWRITE( OKIM6295_status_0_r, OKIM6295_data_0_w )
	AM_RANGE(0x3000, 0x3000) AM_READ( soundlatch2_r )
	AM_RANGE(0x4000, 0x4000) AM_READ( soundlatch_r )
	AM_RANGE(0x5000, 0x5000) AM_READ( MRA8_NOP )
	AM_RANGE(0x6000, 0x6000) AM_WRITE( lordgun_okibank_w )
ADDRESS_MAP_END


static ADDRESS_MAP_START( aliencha_soundio_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x3000, 0x3000) AM_READ(soundlatch2_r)
	AM_RANGE(0x4000, 0x4000) AM_READ(soundlatch_r)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x7000, 0x7000) AM_READWRITE(YMF278B_status_port_0_r, YMF278B_control_port_0_A_w)
	AM_RANGE(0x7001, 0x7001) AM_WRITE(YMF278B_data_port_0_A_w)
	AM_RANGE(0x7002, 0x7002) AM_WRITE(YMF278B_control_port_0_B_w)
	AM_RANGE(0x7003, 0x7003) AM_WRITE(YMF278B_data_port_0_B_w)
	AM_RANGE(0x7004, 0x7004) AM_WRITE(YMF278B_control_port_0_C_w)
	AM_RANGE(0x7005, 0x7005) AM_WRITE(YMF278B_data_port_0_C_w)
	AM_RANGE(0x7400, 0x7400) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x7800, 0x7800) AM_READWRITE(OKIM6295_status_1_r, OKIM6295_data_1_w)
ADDRESS_MAP_END

/***************************************************************************

    Graphics Layout

***************************************************************************/

static const gfx_layout lordgun_8x8x6_layout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{	RGN_FRAC(2,3)+8, RGN_FRAC(2,3)+0,
		RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+0,
		RGN_FRAC(0,3)+8, RGN_FRAC(0,3)+0 },
	{ STEP8(0,1) },
	{ STEP8(0,8*2) },
	8*8*2
};

static const gfx_layout lordgun_16x16x6_layout =
{
	16,16,
	RGN_FRAC(1,3),
	6,
	{	RGN_FRAC(2,3)+8, RGN_FRAC(2,3)+0,
		RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+0,
		RGN_FRAC(0,3)+8, RGN_FRAC(0,3)+0 },
	{ STEP8(0,1),STEP8(8*16*2,1) },
	{ STEP16(0,8*2) },
	16*16*2
};

static const gfx_layout lordgun_32x32x6_layout =
{
	32,32,
	RGN_FRAC(1,3),
	6,
	{	RGN_FRAC(2,3)+8, RGN_FRAC(2,3)+0,
		RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+0,
		RGN_FRAC(0,3)+8, RGN_FRAC(0,3)+0 },
	{ STEP8(0,1),STEP8(8*32*2,1),STEP8(8*32*2*2,1),STEP8(8*32*2*3,1) },
	{ STEP16(0,8*2),STEP16(16*8*2,8*2) },
	32*32*2
};

static const gfx_decode lordgun_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lordgun_16x16x6_layout,  0x000, 0x400/0x40  },	// [0] Sprites
	{ REGION_GFX2, 0, &lordgun_8x8x6_layout,    0x500, 0x100/0x40  },	// [1] Tilemap 0
	{ REGION_GFX3, 0, &lordgun_16x16x6_layout,  0x600, 0x200/0x40  },	// [2] Tilemap 1
	{ REGION_GFX3, 0, &lordgun_32x32x6_layout,  0x700, 0x100/0x40  },	// [3] Tilemap 2
	{ REGION_GFX2, 0, &lordgun_8x8x6_layout,    0x400, 0x400/0x40  },	// [4] Tilemap 3
	{ -1 } /* end of array */
};


/***************************************************************************

    Input Ports

***************************************************************************/

INPUT_PORTS_START( lordgun )
	PORT_START	// IN0
	PORT_DIPNAME( 0x01, 0x01, "Stage Select" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Guns" )
	PORT_DIPSETTING(    0x02, "IGS" )		// x table offset  = 0x25
	PORT_DIPSETTING(    0x00, "Konami" )	// "" = 0x2c
	PORT_DIPNAME( 0x04, 0x04, "Ranking Music" )
	PORT_DIPSETTING(    0x04, "Exciting" )
	PORT_DIPSETTING(    0x00, "Tender" )
	PORT_DIPNAME( 0x08, 0x08, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_SERVICE_NO_TOGGLE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	// eeprom

	PORT_START	// IN1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE2 ) // game cheat: skip stage

	PORT_START	// IN2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	//IN 4
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1   ) PORT_IMPULSE(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2   ) PORT_IMPULSE(5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN5
	PORT_BIT( 0x1ff, 0x100, IPT_LIGHTGUN_X ) PORT_MINMAX(0x3c,0x1d8) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// IN6
	PORT_BIT( 0x1ff, 0x100, IPT_LIGHTGUN_X ) PORT_MINMAX(0x3c,0x1d8) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START	// IN7
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,224) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// IN8
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,224) PORT_SENSITIVITY(35) PORT_KEYDELTA(10) PORT_PLAYER(2)
INPUT_PORTS_END

INPUT_PORTS_START( aliencha )
	PORT_START	// DIP1
	PORT_DIPNAME( 0x01, 0x01, "Credits To Start" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )

	PORT_START	// DIP2
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, "Round Time" )
	PORT_DIPSETTING(    0x00, "32 s" )
	PORT_DIPSETTING(    0x08, "40 s" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Join" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START	// DIP3
	PORT_DIPNAME( 0x03, 0x03, "Buttons" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Vs. Rounds" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )

	PORT_START	// SERVICE
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// P1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1         )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1        ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2        ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3        ) PORT_PLAYER(1)

	PORT_START	// P2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2         )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1        ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2        ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON3        ) PORT_PLAYER(2)

	PORT_START	// COIN
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1          )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2          )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4        ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON5        ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6        ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4        ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON5        ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON6        ) PORT_PLAYER(2)
INPUT_PORTS_END

/***************************************************************************

    Machine Drivers

***************************************************************************/

static ppi8255_interface ppi8255_intf =
{
	2,	// 2 chips
	{ lordgun_eeprom_r,		input_port_1_r },		// Port A read
	{ 0,					input_port_2_r },		// Port B read
	{ input_port_3_r,		input_port_4_r },		// Port C read
	{ fake_w,				fake_w },				// Port A write
	{ lordgun_eeprom_w,		fake_w },				// Port B write
	{ fake2_w,				fake_w },				// Port C write
};

static NVRAM_HANDLER( lordgun )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);
		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(memory_region(REGION_USER1), memory_region_length(REGION_USER1));
	}
}

static MACHINE_RESET( lordgun )
{
	aliencha_protection_data = 0;
	ppi8255_init(&ppi8255_intf);
}

static void soundirq(int state)
{
	cpunum_set_input_line(1, 0, state);
}

static struct YM3812interface lordgun_ym3812_interface =
{
	soundirq
};

static MACHINE_DRIVER_START( lordgun )
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(lordgun_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 5000000)
	MDRV_CPU_PROGRAM_MAP(lordgun_soundmem_map,0)
	MDRV_CPU_IO_MAP(lordgun_soundio_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_RESET(lordgun)

	MDRV_NVRAM_HANDLER(lordgun)

	MDRV_GFXDECODE(lordgun_gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 0x100)
	MDRV_VISIBLE_AREA(0,0x1c0-1, 0,0xe0-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(lordgun)
	MDRV_VIDEO_UPDATE(lordgun)

	// sound hardware
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM3812, 3579545)
	MDRV_SOUND_CONFIG(lordgun_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)	// 5MHz/132 can't be right!
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END


static ppi8255_interface aliencha_ppi8255_intf =
{
	2,
	{ aliencha_dip_r, input_port_4_r },
	{ 0, input_port_5_r },
	{ aliencha_eeprom_r, input_port_6_r },
	{ fake2_w, fake_w },
	{ aliencha_eeprom_w, fake_w },
	{ aliencha_dip_w, fake_w },
};

static MACHINE_RESET( aliencha )
{
	aliencha_dip_sel = 0;
	aliencha_protection_data = 0;
	lordgun_whitescreen = 0;
	ppi8255_init(&aliencha_ppi8255_intf);
}

static struct YMF278B_interface aliencha_ymf278b_interface =
{
	REGION_SOUND3,
	soundirq
};

static MACHINE_DRIVER_START( aliencha )
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(aliencha_map,0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_ADD_TAG("sound", Z80, 5000000)
	MDRV_CPU_PROGRAM_MAP(lordgun_soundmem_map,0)
	MDRV_CPU_IO_MAP(aliencha_soundio_map,0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(aliencha)
	MDRV_NVRAM_HANDLER(93C46)
	MDRV_GFXDECODE(lordgun_gfxdecodeinfo)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x200, 0x100)
	MDRV_VISIBLE_AREA(0,0x1c0-1, 0,0xe0-1)
	MDRV_PALETTE_LENGTH(0x800)
	MDRV_VIDEO_START(lordgun)
	MDRV_VIDEO_UPDATE(lordgun)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(YMF278B, 33868800)
	MDRV_SOUND_CONFIG(aliencha_ymf278b_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
	MDRV_SOUND_ADD(OKIM6295, 1000000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
	MDRV_SOUND_ADD(OKIM6295, 1000000)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END

/***************************************************************************

    ROMs Loading

***************************************************************************/

/*
Lord of Gun
IGS, 1994

PCB Layout
----------

IGSPCB NO. T0076
--------------------------------------------------------
| YM3014           62256      IGS008  IGS006   IGST003 |
| YM3812      6295 62256                       IGST002 |
|       3.57945MHz 62256                       IGST001 |
|                  62256                               |
|6116 LORDGUN.100                              IGSB003 |
|     Z80               62256                  IGSB002 |
|LORDGUN.90                                    IGSB001 |
|J    PAL              6116                            |
|A    PAL              6116                       6116 |
|M                          IGS003                6116 |
|M   68000P10 PAL                                 6116 |
|A                          PAL     PAL           6116 |
|                           PAL     6116               |
|                           PAL     6116        IGS007 |
|                           PAL     6116         20MHz |
|       DSW1(4)                     6116 PAL           |
|             62256    62256          IGSA001 IGSA004  |
|      8255          LORDGUN.10       IGSA002 IGSA005  |
|93C46 8255          LORDGUN.4        IGSA003 IGSA006  |
--------------------------------------------------------

HW Notes:
      68k clock: 10.000MHz
      Z80 clock: 5.000MHz
          VSync: 60Hz
          HSync: 15.15kHz
   YM3812 clock: 3.57945MHz
 OKI 6295 clock: 5.000MHz
  OKI 6295 pin7: HI

  All frequencies are checked with my frequency counter (i.e. they are not guessed)

  IGST* are 8M devices
  IGSA* and IGSB* are 16M devices
  LORDGUN.90 is 27C512
  LORDGUN.100 \
  LORDGUN.10  | 27C040
  LORDGUN.4   /

-----

Lord of Gun (c) 1994 IGS

PCB: IGSPCB NO.T0076

  Main: MC68000P10 10MHz
   Sub: Zilog Z0840006PCS (Z80 6MHz)
 Sound: OKI M6295, Yamaha YM3812-F + Y3014B-F
   OSC: 20.000 MHz, Unmarked OSC for sound chips
EEPROM: NMC 9346N

1 Push Button - Test/Setup Mode

Custom chips:
IGS 005 (144 Pin PQFP)
IGS 006 (144 Pin PQFP)
IGS 007 (144 Pin PQFP)
IGS 008 (160 Pin PQFP)

lg_u122.m3 - Labelled as "LORD GUN U122-M3" MX 27C4000
lg_u144.m3 - Labelled as "LORD GUN U144-M3" MX 27C4000

lordgun.u90  - Labelled as "LORD GUN U90"  27C512
lordgunu.100 - Labelled as "LORD GUN U100" MX 27C4000

Surface mounted ROMs (42 pin DIP)

2 Unmarked ROM(?) chips

IGS A001
IGS A002
IGS A003
IGS A004
IGS A005
IGS A006

IGS B001
IGS B002
IGS B003

IGS T001
IGS T002
IGS T003

DIP Switch-1 (4 Position DIP)
--------------------------------------------------
    DipSwitch Title   | Function | 1 | 2 | 3 | 4 |
--------------------------------------------------
       Game Mode      |  Arcade  |off|           |
                      |  Street  |on |           |
--------------------------------------------------
       Selection      |   IGS    |   |off|       |
        of Guns       |  Konami  |   |on |       |
--------------------------------------------------
       Ranking        | Exciting |       |off|   |
      Background      |  Tender  |       |on |   |
--------------------------------------------------
      Coin Slots      | Seperate |           |off|
                      |  Common  |           |on |
--------------------------------------------------
     Settings Upon Shipping      |off|off|off|off|
--------------------------------------------------

Game modes explained:
 In "Arcade Mode" players could play this game by entering each scene in a
  pre-defined order.
 In "Street Mode" this game now presents 10 selectable scenes for players,
  not 4 any more.  After all scenes are passed (except training courses),
  players can enter the last scene; the Headquarters

                       Lord of Gun JAMMA Pinout

                          Main Jamma Connector
            Solder Side            |             Parts Side
------------------------------------------------------------------
             GND             | A | 1 |             GND
             GND             | B | 2 |             GND
             +5              | C | 3 |             +5
             +5              | D | 4 |             +5
                             | E | 5 |
             +12             | F | 6 |             +12
------------ KEY ------------| H | 7 |------------ KEY -----------
                             | J | 8 |       Coin Counter
                             | K | 9 |
        Speaker (-)          | L | 10|        Speaker (+)
                             | M | 11|
        Video Green          | N | 12|        Video Red
        Video Sync           | P | 13|        Video Blue
                             | R | 14|        Video GND
                             | S | 15|        Test Switch
        Coin Switch 2        | T | 16|        Coin Switch 1
        Start Player 2       | U | 17|        Start Player 1
                             | V | 18|
                             | W | 19|
                             | X | 20|
                             | Y | 21|
                             | Z | 22|
                             | a | 23|
                             | b | 24|
                             | c | 25|
                             | d | 26|
             GND             | e | 27|             GND
             GND             | f | 28|             GND


NOTE: Speakers should be connected serially to Speaker (+) and Speaker (-).
      You must avoid connecting speakers parallelly or connecting speakers
      to Speaker (+) and GND, to keep the amplifier from being damaged or
      from malfunctioning.

 JP1: Player 1 Gun Connector Pinout

   1| +5 Volts - RED Wire    (Manual says "VCC")
   2| Trigger  - White Wire
   3| Ground   - Black Wire
   4| Gun OPTO - Blue Wire   (Manual says "HIT")

 JP2: Player 2 Gun Connector Pinout

   1| +5 Volts - RED Wire    (Manual says "VCC")
   2| Trigger  - White Wire
   3| Ground   - Black Wire
   4| Gun OPTO - Blue Wire   (Manual says "HIT")
*/

ROM_START( lordgun )	// World set, excluding USA, Canada, Mexico, Japan & Taiwan
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) // 68000
	ROM_LOAD16_BYTE( "lord_gun_u144-ch.u144", 0x00000, 0x80000, CRC(ea54ee18) SHA1(5ce3fe0d46c524562a627fe7418dab9753cf74b3) ) // hand written label  LORD/GUN   U144-CH
	ROM_LOAD16_BYTE( "lord_gun_u122-ch.u122", 0x00001, 0x80000, CRC(969a0348) SHA1(6ce064c7c42c51969ea1271c6f75fc602602cfa3) ) // hand written label  LORD/GUN   U122-CH

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) // Z80
	ROM_LOAD( "lord_gun_160.u160", 0x00000, 0x10000, CRC(d59b5e28) SHA1(36696058684d69306f463ed543c8b0195bafa21e) ) // == lord_gun_u90.u90 - 1xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )	// Sprites
	ROM_LOAD( "igs_a001.u22",             0x000000, 0x200000, CRC(400abe33) SHA1(20de1eb626424ea41bd55eb3cecd6b50be744ee0) ) // == igs_a001.u14, different U location
	ROM_LOAD16_BYTE( "lord_gun_u24.u24",  0x200000, 0x080000, CRC(454a5b11) SHA1(2e2e16e5f61c6e4648252ef172409efabbaa0b80) ) // == 1/2 IGS A004 even
	ROM_LOAD16_BYTE( "lord_gun_u23.u23",  0x200001, 0x080000, CRC(a0d7aada) SHA1(3c6d4053e0930dcfdd43c7eb85ca2ba2297c136f) ) // == 1/2 IGS A004 odd
	ROM_LOAD16_BYTE( "lord_gun_u7.u7",    0x300000, 0x080000, CRC(95ef3894) SHA1(0962e2545f7d0551a8272d87ea21401d2c5b8d80) )
	ROM_LOAD16_BYTE( "lord_gun_u14.u14",  0x300001, 0x080000, CRC(dc8a77a1) SHA1(0247acda7556c300e1baa9b726ec835523456761) )
	ROM_LOAD( "igs_a002.u21",             0x400000, 0x200000, CRC(a4810e38) SHA1(c31fe641feab2c93795fc35bf71d4f37af1056d4) ) // == igs_a002.u9, different U location
	ROM_LOAD16_BYTE( "lord_gun_u5.u5",    0x600000, 0x080000, CRC(63aa10c3) SHA1(743207237409319aec79dc2a486736247a582ca9) ) // == 1/2 IGS A005 even
	ROM_LOAD16_BYTE( "lord_gun_u13.u13",  0x600001, 0x080000, CRC(478e248c) SHA1(3b2c4cdf262fe766015e8c12a4fe356da8d88f84) ) // == 1/2 IGS A005 odd
	ROM_LOAD16_BYTE( "lord_gun_u4.u4",    0x700000, 0x080000, CRC(d203c24e) SHA1(d90ba0ea0f823a8013a5a2be50e1c36af96f9efd) )
	ROM_LOAD16_BYTE( "lord_gun_u11.u11",  0x700001, 0x080000, CRC(72277dcd) SHA1(89f6ad1f6813751fcccc15ee5ab6ce63b93a9b13) )
	ROM_LOAD( "igs_a003.u20",             0x800000, 0x200000, CRC(649e48d9) SHA1(ce346154024cf13f3e40000ceeb4c2003cd35894) ) // == igs_a003.u3, different U location
	ROM_LOAD16_BYTE( "lord_gun_u12.u12",  0xa00000, 0x080000, CRC(a2a55d65) SHA1(aad9ee5b9a93a5cd14a088c4bade9bab34f9e206) ) // == 1/2 IGS A006 even
	ROM_LOAD16_BYTE( "lord_gun_u6.u6",    0xa00001, 0x080000, CRC(fe649605) SHA1(80c22099ed630bdc825ebd910bcc48caad72e647) ) // == 1/2 IGS A006 odd
	ROM_LOAD16_BYTE( "lord_gun_u10.u10",  0xb00000, 0x080000, CRC(eea39e5e) SHA1(806a97a08d0108509d30732b507e1064215295c6) )
	ROM_LOAD16_BYTE( "lord_gun_u3.u3",    0xb00001, 0x080000, CRC(233782f8) SHA1(ef2049aadbcf5c409275ecfbbe75bdade5b087d4) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )	// Tilemaps 0 & 3
	ROM_LOAD( "igs_t001.u8",  0x000000, 0x100000, CRC(36dd96f3) SHA1(4e70eb807160e7ed1b19d7f38df3a38021f42d9b) ) // same data as lordgunu set, different U location
	ROM_LOAD( "igs_t002.u18", 0x100000, 0x100000, CRC(816a7665) SHA1(f2f2624ab262c957f84c657cfc432d14c61b19e8) )
	ROM_LOAD( "igs_t003.u19", 0x200000, 0x100000, CRC(cbfee543) SHA1(6fad8ef8d683f709f6ff2b16319447516c372fc8) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	// Tilemaps 1 & 2
	ROM_LOAD( "igs_b001.u1", 0x000000, 0x200000, CRC(3096de1c) SHA1(d010990d21cfda9cb8ab5b4bc0e329c23b7719f5) ) // same data as lordgunu set, different U location
	ROM_LOAD( "igs_b002.u2", 0x200000, 0x200000, CRC(2234531e) SHA1(58a82e31a1c0c1a4dd026576319f4e7ecffd140e) )
	ROM_LOAD( "igs_b003.u9", 0x400000, 0x200000, CRC(6cbf21ac) SHA1(ad25090a00f291aa48929ffa01347cc53e0051f8) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD( "lord_gun_u161-3.u161", 0x00000, 0x80000, CRC(b4e0fa07) SHA1(f5f33fe3f3a124f4737751fda3ea409fceeec0be) ) // == lord_gun_u100.u100

	ROM_REGION( 0x80, REGION_USER1, 0 ) // Default eeprom
	ROM_LOAD16_WORD( "eeprom", 0x00, 0x80, CRC(0dad0e43) SHA1(c216d1f19228e103b78e5acb30a66dab3804ac70) )
ROM_END

ROM_START( lordgunu )	// USA, Canada & Mexico
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) // 68000
	ROM_LOAD16_BYTE( "lord_gun_u10.u10", 0x00000, 0x80000, CRC(acda77ef) SHA1(7cd8580419e2f62a3b5a1e4a6020a3ef978ff1e8) ) // == LORD GUN U144-M3?
	ROM_LOAD16_BYTE( "lord_gun_u4.u4",   0x00001, 0x80000, CRC(a1a61254) SHA1(b0c5aa656024cfb9be28a11061656159e7b72d00) ) // == LORD GUN U122-M3?

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) // Z80
	ROM_LOAD( "lord_gun_u90.u90", 0x00000, 0x10000, CRC(d59b5e28) SHA1(36696058684d69306f463ed543c8b0195bafa21e) ) // labeled as LORD/GUN U90 - 1xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )	// Sprites
	ROM_LOAD( "igs_a001.u14", 0x000000, 0x200000, CRC(400abe33) SHA1(20de1eb626424ea41bd55eb3cecd6b50be744ee0) )
	ROM_LOAD( "igs_a004.u13", 0x200000, 0x200000, CRC(52687264) SHA1(28444cf6b5662054e283992857e0827a2ca15b83) )
	ROM_LOAD( "igs_a002.u9",  0x400000, 0x200000, CRC(a4810e38) SHA1(c31fe641feab2c93795fc35bf71d4f37af1056d4) )
	ROM_LOAD( "igs_a005.u8",  0x600000, 0x200000, CRC(e32e79e3) SHA1(419f9b501e5a37d763ece9322271e61035b50217) )
	ROM_LOAD( "igs_a003.u3",  0x800000, 0x200000, CRC(649e48d9) SHA1(ce346154024cf13f3e40000ceeb4c2003cd35894) )
	ROM_LOAD( "igs_a006.u2",  0xa00000, 0x200000, CRC(39288eb6) SHA1(54d157f0e151f6665f4288b4d09bd65571005132) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )	// Tilemaps 0 & 3
	ROM_LOAD( "igs_t001.u108", 0x000000, 0x100000, CRC(36dd96f3) SHA1(4e70eb807160e7ed1b19d7f38df3a38021f42d9b) )
	ROM_LOAD( "igs_t002.u114", 0x100000, 0x100000, CRC(816a7665) SHA1(f2f2624ab262c957f84c657cfc432d14c61b19e8) )
	ROM_LOAD( "igs_t003.u119", 0x200000, 0x100000, CRC(cbfee543) SHA1(6fad8ef8d683f709f6ff2b16319447516c372fc8) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	// Tilemaps 1 & 2
	ROM_LOAD( "igs_b001.u82", 0x000000, 0x200000, CRC(3096de1c) SHA1(d010990d21cfda9cb8ab5b4bc0e329c23b7719f5) )
	ROM_LOAD( "igs_b002.u91", 0x200000, 0x200000, CRC(2234531e) SHA1(58a82e31a1c0c1a4dd026576319f4e7ecffd140e) )
	ROM_LOAD( "igs_b003.u97", 0x400000, 0x200000, CRC(6cbf21ac) SHA1(ad25090a00f291aa48929ffa01347cc53e0051f8) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD( "lord_gun_u100.u100", 0x00000, 0x80000, CRC(b4e0fa07) SHA1(f5f33fe3f3a124f4737751fda3ea409fceeec0be) ) // labeled as LORD/GUN U100

	ROM_REGION( 0x80, REGION_USER1, 0 ) // Default eeprom
	ROM_LOAD16_WORD( "eeprom", 0x00, 0x80, CRC(0dad0e43) SHA1(c216d1f19228e103b78e5acb30a66dab3804ac70) )
ROM_END

ROM_START( aliencha )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD( "igsc0102.u81", 0x00000, 0x200000, CRC(e3432be3) SHA1(d3597c885571d4a996afaaf29c78da123798371e) )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "hfh_s.u86", 0x00000, 0x10000, CRC(5728a9ed) SHA1(e5a9e4a1a2cc6c848b08608bc8727bc739270873) )

	ROM_REGION( 0x300000, REGION_GFX2, 0 )
	ROM_LOAD( "igst0101.u9",  0x000000, 0x100000, CRC(2ce12d7b) SHA1(aa93a82e5f4015c46bb705efb2051b62cd5d7e04) )
	ROM_LOAD( "igst0102.u10", 0x100000, 0x100000, CRC(542a76a0) SHA1(6947b50a024d0053c1eaf9da8c90652bab875142) )
	ROM_LOAD( "igst0103.u11", 0x200000, 0x100000, CRC(adf5698a) SHA1(4b798f8acc5d7581c7e0989260863ae0ca654acd) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD( "igsb0101.u8", 0x000000, 0x200000, CRC(5c995f7e) SHA1(4f08cf13e313c6802c924b914c73cab4b450da61) )
	ROM_LOAD( "igsb0102.u7", 0x200000, 0x200000, CRC(a2ae9baf) SHA1(338ee260c33448568f138ca00e1d4edda4da018f) )
	ROM_LOAD( "igsb0103.u6", 0x400000, 0x200000, CRC(11b927af) SHA1(2f15e5cea1b86cde3b679bdd0f3d79672d0ddd3e) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROM_LOAD( "igsa0101.u3", 0x000000, 0x400000, CRC(374d07c4) SHA1(87e9bfe32cbfe9964ba7253847fbd14aa3c8ed20) )
	ROM_LOAD( "igsa0102.u2", 0x400000, 0x400000, CRC(dbeee7ac) SHA1(e0eb0d73d9230aa6f69f5ac25d44fa19affebe88) )
	ROM_LOAD( "igsa0103.u1", 0x800000, 0x400000, CRC(e5f19041) SHA1(c92a29bbbcb9a1f63364c665e3e0f9679add4389) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "hfh_g.u65", 0x00000, 0x40000, CRC(ec469b57) SHA1(ba1668078987ad51f47bcd3e61c51a0cf2545350) )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )
	ROM_LOAD( "hfh_g.u66", 0x00000, 0x40000, CRC(7cfcd98e) SHA1(3b03123160adfd3404a9e0c4c68420930e80ae48) )

	ROM_REGION( 0x200000, REGION_SOUND3, 0 )
	ROM_LOAD( "yrw801-m", 0x000000, 0x200000, CRC(2a9d8d43) SHA1(32760893ce06dbe3930627755ba065cc3d8ec6ca) )
ROM_END

ROM_START( alienchac )
	ROM_REGION( 0x300000, REGION_CPU1, 0 ) // 68000
	ROM_LOAD16_BYTE( "hfh_p.u80", 0x000000, 0x080000, CRC(5175ebdc) SHA1(4a0bdda0f8291f895f888bfd45328b2b124b9051) )
	ROM_LOAD16_BYTE( "hfh_p.u79", 0x000001, 0x080000, CRC(42ad978c) SHA1(eccb96e7170902b37989c8f207e1a821f29b2475) )
	ROM_LOAD( "igsc0101.u81",     0x100000, 0x200000, CRC(704c48cf) SHA1(908017b206622680ab766251294806c8c8c9388b) ) // bytes 0x000000-0x0fffff == 0x100000-0x1fffff of IGSC0102

	ROM_REGION( 0x010000, REGION_CPU2, 0 ) // Z80
	ROM_LOAD( "alien_u-86.u86", 0x00000, 0x10000, CRC(5728a9ed) SHA1(e5a9e4a1a2cc6c848b08608bc8727bc739270873) ) // labeled as  ALIEN  U-86, == hfh_s.u86

	ROM_REGION( 0x300000, REGION_GFX2, 0 ) // Tilemaps 0 & 3
	ROM_LOAD( "igst0101.u9",  0x000000, 0x100000, CRC(2ce12d7b) SHA1(aa93a82e5f4015c46bb705efb2051b62cd5d7e04) )
	ROM_LOAD( "igst0102.u10", 0x100000, 0x100000, CRC(542a76a0) SHA1(6947b50a024d0053c1eaf9da8c90652bab875142) )
	ROM_LOAD( "igst0103.u11", 0x200000, 0x100000, CRC(adf5698a) SHA1(4b798f8acc5d7581c7e0989260863ae0ca654acd) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 ) // Tilemaps 1 & 2
	ROM_LOAD( "igsb0101.u8",  0x000000, 0x200000, CRC(5c995f7e) SHA1(4f08cf13e313c6802c924b914c73cab4b450da61) )
	ROM_LOAD( "igsb0102.u7",  0x200000, 0x200000, CRC(a2ae9baf) SHA1(338ee260c33448568f138ca00e1d4edda4da018f) )
	ROM_LOAD( "igsb0103.u6",  0x400000, 0x200000, CRC(11b927af) SHA1(2f15e5cea1b86cde3b679bdd0f3d79672d0ddd3e) )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 ) // Sprites
	ROM_LOAD( "igsa0101.u3",  0x000000, 0x400000, CRC(374d07c4) SHA1(87e9bfe32cbfe9964ba7253847fbd14aa3c8ed20) )
	ROM_LOAD( "igsa0102.u2",  0x400000, 0x400000, CRC(dbeee7ac) SHA1(e0eb0d73d9230aa6f69f5ac25d44fa19affebe88) )
	ROM_LOAD( "igsa0103.u1",  0x800000, 0x400000, CRC(e5f19041) SHA1(c92a29bbbcb9a1f63364c665e3e0f9679add4389) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) // Samples
	ROM_LOAD( "alien_u65.u65", 0x00000, 0x40000, CRC(ec469b57) SHA1(ba1668078987ad51f47bcd3e61c51a0cf2545350) ) // labeled as  ALIEN  U65, == hfh_g.u65

	ROM_REGION( 0x40000, REGION_SOUND2, 0 ) // Samples
	ROM_LOAD( "alien_u66.u66", 0x00000, 0x40000, CRC(7cfcd98e) SHA1(3b03123160adfd3404a9e0c4c68420930e80ae48) ) // labeled as  ALIEN  U66, == hfh_g.u66

	ROM_REGION( 0x200000, REGION_SOUND3, 0 ) // Samples (Standard Yamaha YRW801 2MB samples ROM)
	ROM_LOAD( "yrw801-m", 0x000000, 0x200000, CRC(2a9d8d43) SHA1(32760893ce06dbe3930627755ba065cc3d8ec6ca) ) // Not dumped from PCB, but is a standard samples rom
ROM_END

/***************************************************************************

    Game Drivers

***************************************************************************/

GAME( 1994, lordgun,   0,        lordgun,  lordgun,  lordgun, ROT0, "IGS", "Lord of Gun (World)",     GAME_IMPERFECT_GRAPHICS ) // Excludes USA, Canada, Mexico, Japan & Taiwan
GAME( 1994, lordgunu,  lordgun,  lordgun,  lordgun,  lordgun, ROT0, "IGS", "Lord of Gun (USA)",       GAME_IMPERFECT_GRAPHICS ) // USA, Canada & Mexico
GAME( 1994, aliencha,  0,        aliencha, aliencha, 0,       ROT0, "IGS", "Alien Challenge (World)", 0 )
GAME( 1994, alienchac, aliencha, aliencha, aliencha, 0,       ROT0, "IGS", "Alien Challenge (China)", 0 )
