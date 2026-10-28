// license:BSD-3-Clause
// copyright-holders:R. Belmont, Andrew Gardner, Ryan Holtz
/*
    Polygonet Commanders / Poly-Net Warriors
    68EC020, DSP56156, Z80, K054539, K053936 and K056230.

    The two polygon planes use independent double buffers.  Port C selects
    the DSP data banks; writes to the page registers latch completed spans.
    Network registers currently provide local-board operation only.
*/

#include "driver.h"
#include <math.h>

#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "cpu/dsp56k/dsp56k.h"
#include "sound/k054539.h"
#include "machine/eeprom.h"
#include "vidhrdw/generic.h"

VIDEO_START(polygonet_vh_start);
VIDEO_UPDATE(polygonet_vh_screenrefresh);

READ32_HANDLER( polygonet_ttl_ram_r );
WRITE32_HANDLER( polygonet_ttl_ram_w );
READ32_HANDLER( polygonet_roz_ram_r );
WRITE32_HANDLER( polygonet_roz_ram_w );

static UINT8 sys1;
static mame_timer *sound_timer;
static UINT8 sound_timer_enable, sound_timer_state;
static UINT8 sound_active, sound_volume;

static void sound_volume_update(void)
{
	float gain = pow(2.0, ((int)sound_volume - 40) / 10.0);
	sndti_set_output_gain(SOUND_K054539, 0, 0, (sound_active & 2) ? gain : 0);
	sndti_set_output_gain(SOUND_K054539, 0, 1, (sound_active & 1) ? gain : 0);
}
static UINT16 psac_ctrl[16];

static READ32_HANDLER( polygonet_palette_r )
{
	return paletteram32[offset];
}

static WRITE32_HANDLER( polygonet_palette_w )
{
	UINT32 color;
	COMBINE_DATA(&paletteram32[offset]);
	color = paletteram32[offset];
	palette_set_color(offset, (color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

static READ32_HANDLER( polygonet_psac_ctrl_r )
{
	return (psac_ctrl[offset * 2] << 16) | psac_ctrl[offset * 2 + 1];
}

static WRITE32_HANDLER( polygonet_psac_ctrl_w )
{
	UINT32 value = polygonet_psac_ctrl_r(offset, mem_mask);
	COMBINE_DATA(&value);
	psac_ctrl[offset * 2] = value >> 16;
	psac_ctrl[offset * 2 + 1] = value;
}

static UINT16 dsp_share[0x2000];
static UINT16 *dsp_common;
static UINT16 dsp_ab0[0x6000];
static UINT16 dsp_a6[0x1000];
static UINT16 dsp_a7[0x1000];
static UINT16 dsp_ae[0x2000];
static UINT16 dsp_a8[8 * 0x4000];
static UINT16 dsp_b6[4 * 0x1000];
static UINT16 dsp_b7[4 * 0x1000];
static UINT16 dsp_b8[8 * 0x8000];
static UINT16 dsp_portc;
static UINT16 poly_bitmap[2][2][256][384];
static UINT8 render_buf[2], display_buf[2];

static void process_polys(int page)
{
	static const unsigned info_offsets[2][2] = { { 0, 0x2000 }, { 0x1000, 0x3000 } };
	unsigned bank = info_offsets[page][render_buf[page]];
	UINT16 *b6 = dsp_b6 + bank;
	UINT16 *b7 = dsp_b7 + bank;
	UINT16 (*bitmap)[384] = poly_bitmap[page][render_buf[page]];
	unsigned i;
	if (b6[0] == 6 && b6[1] == 6)
		return;
	if (b7[1] & 0x8000)
		memset(bitmap, 0, sizeof(poly_bitmap[0][0]));
	for (i = 0; i < 0x1000 && (b7[i + 1] & 0x8000); i += 2)
	{
		int y_start = (INT16)b6[i] >> 5;
		int y_end = (INT16)b6[i + 1] >> 5;
		unsigned start = (page ? 0x10000 : 0) + (render_buf[page] ? 0x20000 : 0) + (b7[i + 1] & 0x7fff);
		int y;
		if (y_start > y_end)
		{
			int temp = y_start;
			y_start = y_end;
			y_end = temp;
		}
		for (y = y_start; y < y_end; ++y, ++start)
		{
			UINT16 by = y + 128;
			int x_start, x_end, x;
			if (by >= 256 || start + 0x8000 >= 8 * 0x8000)
				continue;
			x_start = (INT16)dsp_b8[start] >> 5;
			x_end = (INT16)dsp_b8[start + 0x8000] >> 5;
			for (x = x_start; x <= x_end; ++x)
			{
				UINT16 bx = x + 192;
				if (bx < 384 && !bitmap[by][bx])
					bitmap[by][bx] = 0x8000 | (b7[i] & 0x7fff);
			}
		}
	}
}

void polygonet_draw_polys(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int x, y;
	for (y = cliprect->min_y; y <= cliprect->max_y && y < 256; y++)
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			if (x >= 64 && x < 448)
			{
				UINT16 a = poly_bitmap[0][display_buf[0]][y][x - 64];
				UINT16 b = poly_bitmap[1][display_buf[1]][y][x - 64];
				if (a & 0x7fff) plot_pixel(bitmap, x, y, Machine->pens[a]);
				else if (b & 0x7fff) plot_pixel(bitmap, x, y, Machine->pens[b]);
			}
}

static PALETTE_INIT( polygonet )
{
	int i;
	for (i = 0; i < 32768; i++)
	{
		int r = (i >> 10) & 31, g = (i >> 5) & 31, b = i & 31;
		palette_set_color(32768 + i, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
	}
}

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER(nvram_handler)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			EEPROM_load(file);
		}
		else
		{
			EEPROM_set_data(memory_region(REGION_USER1), 0x80);
		}
	}
}

static READ32_HANDLER( polygonet_eeprom_r )
{
	return (readinputport(0) << 24) | (readinputport(1) << 16) |
		((readinputport(2) & ~3) << 8) | (EEPROM_read_bit() << 8) |
		0x0200 | readinputport(3);
}

static WRITE32_HANDLER( polygonet_eeprom_w )
{
	if (ACCESSING_MSB32)
	{
		EEPROM_write_bit(data & 0x01000000);
		EEPROM_set_cs_line((data & 0x02000000) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04000000) ? ASSERT_LINE : CLEAR_LINE);
	}
	if (!(mem_mask & 0x00ff0000))
	{
		sys1 = data >> 16;
		coin_counter_w(0, sys1 & 1);
		coin_counter_w(1, sys1 & 2);
		if (!(sys1 & 0x20))
			cpunum_set_input_line(0, MC68000_IRQ_5, CLEAR_LINE);
	}
}

/* TTL tile readback for ROM test */
static READ32_HANDLER( ttl_rom_r )
{
	UINT32 *ROM;

	ROM = (UINT32 *)memory_region(REGION_GFX1);

	return ROM[offset];
}

/* PSAC2 tile readback for ROM test */
static READ32_HANDLER( psac_rom_r )
{
	UINT32 *ROM;

	ROM = (UINT32 *)memory_region(REGION_GFX2);

	return ROM[offset];
}

static INTERRUPT_GEN(polygonet_interrupt)
{
	if (sys1 & 0x20)
		cpunum_set_input_line(0, MC68000_IRQ_5, ASSERT_LINE);
}

static READ32_HANDLER( sound_r )
{
	return offset == 2 ? soundlatch3_r(0) << 8 : 0;
}

static WRITE32_HANDLER( sound_w )
{
	if (offset == 0)
	{
		if (!(mem_mask & 0xff000000)) sound_active = data >> 24;
		if (!(mem_mask & 0x0000ff00)) sound_volume = 0;
		if (!(mem_mask & 0x000000ff) && (data & 0xff) && sound_volume < 64) sound_volume++;
		sound_volume_update();
	}
	if (offset == 1)
	{
		if (!(mem_mask & 0x0000ff00))
			soundlatch_w(0, (data >> 8) & 0xff);
		if (!(mem_mask & 0x000000ff))
			soundlatch2_w(0, data & 0xff);
	}
}

static WRITE32_HANDLER( sound_irq_w )
{
	cpunum_set_input_line(2, 0, HOLD_LINE);
}

static READ32_HANDLER( dsp_host_r )
{
	UINT32 value = 0;
	if (!(mem_mask & 0xff000000)) value |= dsp56k_host_read(1, offset * 2) << 24;
	if (!(mem_mask & 0x0000ff00)) value |= dsp56k_host_read(1, offset * 2 + 1) << 8;
	return value;
}

static WRITE32_HANDLER( dsp_host_w )
{
	if (!(mem_mask & 0xff000000)) dsp56k_host_write(1, offset * 2, data >> 24);
	if (!(mem_mask & 0x0000ff00)) dsp56k_host_write(1, offset * 2 + 1, data >> 8);
}

static WRITE32_HANDLER( dsp_control_w )
{
	if (ACCESSING_MSB32)
		cpunum_set_input_line(1, DSP56K_IRQ_RESET, (data & 0x01000000) ? CLEAR_LINE : ASSERT_LINE);
}

static READ32_HANDLER( dsp_shared_r )
{
	return (dsp_share[offset * 2] << 16) | dsp_share[offset * 2 + 1];
}

static WRITE32_HANDLER( dsp_shared_w )
{
	UINT32 value = dsp_shared_r(offset, mem_mask);
	COMBINE_DATA(&value);
	dsp_share[offset * 2] = value >> 16;
	dsp_share[offset * 2 + 1] = value;
}

static void dsp_portc_w(UINT16 data)
{
	dsp_portc = data;
}

static UINT16 *dsp_data_ptr(unsigned address)
{
	unsigned b67 = ((dsp_portc >> 6) & 2) | ((dsp_portc >> 8) & 1);
	unsigned b8 = (b67 << 1) | (dsp_portc & 1);
	if (address < 0x6000) return dsp_ab0 + address;
	if (dsp_portc & 2)
	{
		if (address < 0x7000) return dsp_b6 + b67 * 0x1000 + address - 0x6000;
		if (address < 0x8000) return dsp_b7 + b67 * 0x1000 + address - 0x7000;
		return dsp_b8 + b8 * 0x8000 + address - 0x8000;
	}
	if (address < 0x7000) return dsp_a6 + address - 0x6000;
	if (address < 0x8000) return dsp_a7 + address - 0x7000;
	if (address < 0xc000) return dsp_a8 + ((dsp_portc >> 2) & 7) * 0x4000 + address - 0x8000;
	if (address < 0xe000) return dsp_share + address - 0xc000;
	return dsp_ae + address - 0xe000;
}

static READ16_HANDLER( dsp_data_r )
{
	return *dsp_data_ptr(offset);
}

static WRITE16_HANDLER( dsp_data_w )
{
	UINT16 *dest = dsp_data_ptr(offset);
	UINT16 old = *dest;
	COMBINE_DATA(dest);
	if (!(dsp_portc & 2))
	{
		if (offset >= 0x7000 && offset < 0x8000)
			dsp_common[offset - 0x7000] = *dest;
		if (offset >= 0xe000 && offset < 0xe002 && ((old ^ *dest) & 1))
		{
			render_buf[offset - 0xe000] = *dest & 1;
			display_buf[offset - 0xe000] = old & 1;
			process_polys(offset - 0xe000);
		}
	}
}

static READ16_HANDLER( dsp_boot_r )
{
	return 0x7fff;
}

static READ32_HANDLER( network_r )
{
	return 0x08000000;
}

static WRITE32_HANDLER( network_w )
{
	if (!(mem_mask & 0x00ff0000))
	{
		UINT8 control = data >> 16;
		if (control & 0x20) cpunum_set_input_line(0, MC68000_IRQ_3, ASSERT_LINE);
		if (!(control & 1)) cpunum_set_input_line(0, MC68000_IRQ_3, CLEAR_LINE);
	}
}

static ADDRESS_MAP_START( polygonet_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_READ(MRA32_ROM)
	AM_RANGE(0x200000, 0x21ffff) AM_READ(polygonet_palette_r)
	AM_RANGE(0x400000, 0x40001f) AM_READ(polygonet_psac_ctrl_r)
	AM_RANGE(0x440000, 0x440fff) AM_READ(polygonet_roz_ram_r)
	AM_RANGE(0x480000, 0x480003) AM_READ(polygonet_eeprom_r)
	AM_RANGE(0x500000, 0x503fff) AM_READ(dsp_shared_r)
	AM_RANGE(0x506000, 0x50600f) AM_READ(dsp_host_r)
	AM_RANGE(0x540000, 0x540fff) AM_READ(polygonet_ttl_ram_r)
	AM_RANGE(0x541000, 0x54101f) AM_READ(MRA32_RAM)
	AM_RANGE(0x580000, 0x5807ff) AM_READ(MRA32_RAM)
	AM_RANGE(0x580800, 0x580803) AM_READ(network_r)
	AM_RANGE(0x600000, 0x60000f) AM_READ(sound_r)
	AM_RANGE(0x700000, 0x73ffff) AM_READ(psac_rom_r)
	AM_RANGE(0x780000, 0x79ffff) AM_READ(ttl_rom_r)
	AM_RANGE(0xff8000, 0xffffff) AM_READ(MRA32_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( polygonet_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_WRITE(MWA32_ROM)
	AM_RANGE(0x200000, 0x21ffff) AM_WRITE(polygonet_palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0x400000, 0x40001f) AM_WRITE(polygonet_psac_ctrl_w)
	AM_RANGE(0x440000, 0x440fff) AM_WRITE(polygonet_roz_ram_w)
	AM_RANGE(0x4C0000, 0x4C0003) AM_WRITE(polygonet_eeprom_w)
	AM_RANGE(0x500000, 0x503fff) AM_WRITE(dsp_shared_w)
	AM_RANGE(0x504000, 0x504003) AM_WRITE(dsp_control_w)
	AM_RANGE(0x506000, 0x50600f) AM_WRITE(dsp_host_w)
	AM_RANGE(0x540000, 0x540fff) AM_WRITE(polygonet_ttl_ram_w)
	AM_RANGE(0x541000, 0x54101f) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x580000, 0x5807ff) AM_WRITE(MWA32_RAM)	// A21K
	AM_RANGE(0x580800, 0x580803) AM_WRITE(network_w)
	AM_RANGE(0x600000, 0x60000f) AM_WRITE(sound_w)
	AM_RANGE(0x640000, 0x640003) AM_WRITE(sound_irq_w)
	AM_RANGE(0x680000, 0x680003) AM_WRITE(watchdog_reset32_w)
	AM_RANGE(0xff8000, 0xffffff) AM_WRITE(MWA32_RAM)
ADDRESS_MAP_END


/**********************************************************************************/

static ADDRESS_MAP_START( dsp_program_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM AM_BASE(&dsp56k_program_ram)
	AM_RANGE(0x7000, 0x7fff) AM_RAM AM_BASE(&dsp_common)
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0xc000, 0xc000) AM_READ(dsp_boot_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp_data_map, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0000, 0xffbf) AM_READWRITE(dsp_data_r, dsp_data_w)
	AM_RANGE(0xffc0, 0xffff) AM_READWRITE(dsp56k_peripheral_r, dsp56k_peripheral_w)
ADDRESS_MAP_END

static int cur_sound_region;

static void reset_sound_region(void)
{
	memory_set_bankptr(2, memory_region(REGION_CPU3) + cur_sound_region*0x4000);
}

static WRITE8_HANDLER( sound_bankswitch_w )
{
	cur_sound_region = data & 7;

	reset_sound_region();
}

static void audio_interrupt(int param)
{
	if (sound_timer_enable)
	{
		sound_timer_state ^= 1;
		cpunum_set_input_line(2, INPUT_LINE_NMI, sound_timer_state ? ASSERT_LINE : CLEAR_LINE);
	}
}

static WRITE8_HANDLER( sound_pcm_w )
{
	K054539_0_w(offset, data);
	if (offset == 0x227)
	{
		double period = TIME_IN_HZ((38 + data) * (48000.0 / 14400.0) * 2);
		timer_adjust(sound_timer, period, 0, period);
		sound_timer_state = 0;
		cpunum_set_input_line(2, INPUT_LINE_NMI, CLEAR_LINE);
	}
	if (offset == 0x22f)
	{
		sound_timer_enable = data & 0x20;
		if (!sound_timer_enable)
		{
			sound_timer_state = 0;
			cpunum_set_input_line(2, INPUT_LINE_NMI, CLEAR_LINE);
		}
	}
}

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK2)
	AM_RANGE(0xc000, 0xdfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe000, 0xe22f) AM_READ(K054539_0_r)
	AM_RANGE(0xe230, 0xe3ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe400, 0xe7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf002, 0xf002) AM_READ(soundlatch_r)
	AM_RANGE(0xf003, 0xf003) AM_READ(soundlatch2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe22f) AM_WRITE(sound_pcm_w)
	AM_RANGE(0xe230, 0xe3ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe400, 0xe7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(soundlatch3_w)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(sound_bankswitch_w)
	AM_RANGE(0xfff1, 0xfff3) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static struct K054539interface k054539_interface =
{
	REGION_SOUND1
};

/**********************************************************************************/

static const gfx_layout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
	  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
 	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &bglayout,     0x0000, 64 },
	{ -1 } /* end of array */
};

static MACHINE_RESET( polygonet )
{
	cur_sound_region = 0;
	reset_sound_region();
	sys1 = 0;
	sound_timer_enable = sound_timer_state = 0;
	sound_active = sound_volume = 0;
	timer_adjust(sound_timer, TIME_NEVER, 0, TIME_NEVER);
	cpunum_set_input_line(2, INPUT_LINE_NMI, CLEAR_LINE);
	sound_volume_update();
	dsp_portc = 0;
	memset(render_buf, 0, sizeof(render_buf));
	memset(display_buf, 0, sizeof(display_buf));
	memset(poly_bitmap, 0, sizeof(poly_bitmap));
	cpunum_set_input_line(1, DSP56K_IRQ_RESET, ASSERT_LINE);
	cpunum_set_input_line(1, DSP56K_IRQ_MODA, ASSERT_LINE);
	cpunum_set_input_line(1, DSP56K_IRQ_MODB, CLEAR_LINE);
}

MACHINE_DRIVER_START( plygonet )
	MDRV_CPU_ADD(M68EC020, 16000000)	/* 16 MHz (xtal is 32.0 MHz) */
	MDRV_CPU_PROGRAM_MAP(polygonet_readmem,polygonet_writemem)
	MDRV_CPU_VBLANK_INT(polygonet_interrupt, 1)

	MDRV_CPU_ADD(DSP56156,40000000)
	MDRV_CPU_PROGRAM_MAP(dsp_program_map, 0)
	MDRV_CPU_DATA_MAP(dsp_data_map, 0)


	MDRV_CPU_ADD_TAG( "sound", Z80, 8000000 )
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)

	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_RESET(polygonet)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_NVRAM_HANDLER(nvram_handler)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(64, 64+384-1, 0, 32*8-1 )
	MDRV_PALETTE_LENGTH(65536)
	MDRV_PALETTE_INIT(polygonet)

	MDRV_VIDEO_START(polygonet_vh_start)
	MDRV_VIDEO_UPDATE(polygonet_vh_screenrefresh)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)

MACHINE_DRIVER_END

INPUT_PORTS_START( polygonet )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x00, "Player Color/Network ID" )
	PORT_DIPSETTING(    0x00, "Red" )
	PORT_DIPSETTING(    0x10, "Yellow" )
	PORT_DIPSETTING(    0x20, "Green" )
	PORT_DIPSETTING(    0x30, "Blue" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

INPUT_PORTS_START( polynetw )
	PORT_INCLUDE( polygonet )

	PORT_MODIFY("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static DRIVER_INIT(polygonet)
{
	dsp56k_set_portc(1, dsp_portc_w);
	sound_timer = timer_alloc(audio_interrupt);
	cur_sound_region = 0;
	sys1 = 0;
	K053936_0_ctrl = psac_ctrl;
	K053936_wraparound_enable(0, 1);
	reset_sound_region();
}

ROM_START( plygonet )
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_BYTE( "305uaa01.4k", 0x000003, 512*1024, CRC(8bdb6c95) SHA1(e981833842f8fd89b9726901fbe2058444204792) )
	ROM_LOAD32_BYTE( "305uaa02.2k", 0x000002, 512*1024, CRC(4d7e32b3) SHA1(25731526535036972577637d186f02ae467296bd) )
	ROM_LOAD32_BYTE( "305uaa03.2h", 0x000001, 512*1024, CRC(36e4e3fe) SHA1(e8fcad4f196c9b225a0fbe70791493ff07c648a9) )
	ROM_LOAD32_BYTE( "305uaa04.4h", 0x000000, 512*1024, CRC(d8394e72) SHA1(eb6bcf8aedb9ba5843204ab8aacb735cbaafb74d) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD("305b05.7b", 0x000000, 0x20000, CRC(2d3d9654) SHA1(784a409df47cee877e507b8bbd3610d161d63753) )

	ROM_REGION32_BE( 0x20000, REGION_GFX1, 0 )
	ROMX_LOAD( "305b06.18g", 0x000000, 0x20000, CRC(decd6e42) SHA1(4c23dcb1d68132d3381007096e014ee4b6007086), ROM_GROUPDWORD | ROM_REVERSE )

	ROM_REGION32_BE( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "305b07.20d", 0x000000, 0x40000, CRC(e4320bc3) SHA1(b0bb2dac40d42f97da94516d4ebe29b1c3d77c37) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )
	ROM_LOAD( "305b08.2e", 0x000000, 0x200000, CRC(874607df) SHA1(763b44a80abfbc355bcb9be8bf44373254976019) )

	ROM_REGION( 0x80, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "plygonet.nv", 0x0000, 0x0080, CRC(627748ac) SHA1(ea1b06739fee235b049ff8daffff7d43cb093112) )
ROM_END

ROM_START( polynetw )
	ROM_REGION( 0x200000, REGION_CPU1, 0)
	ROM_LOAD32_BYTE( "305jaa01.4k", 0x000003, 0x080000, CRC(ea889bd9) SHA1(102e7c0f0c064662c0f6137ad5da97a9ccd49a97) )
	ROM_LOAD32_BYTE( "305jaa02.2k", 0x000002, 0x080000, CRC(d0710379) SHA1(cf0970d63e8d021edf2d404838c658a5b7cb8fb8) )
	ROM_LOAD32_BYTE( "305jaa03.2h", 0x000001, 0x080000, CRC(278b5928) SHA1(2ea96054e2ef637731cd64f2bef0b5b2bbe7e24f) )
	ROM_LOAD32_BYTE( "305jaa04.4h", 0x000000, 0x080000, CRC(b069353b) SHA1(12fbe2df09328bb7193e89a49d84a61eab5bfdcb) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD( "305jaa05.7b", 0x000000, 0x020000, CRC(06053db6) SHA1(c7d43c2650d949ee552a49db93dece842c17e68d) )

	ROM_REGION32_BE( 0x20000, REGION_GFX1, 0 )
	ROMX_LOAD( "305a06.18g", 0x000000, 0x020000, CRC(4b9b7e9c) SHA1(8c3c0f1ec7e26fd9552f6da1e6bdd7ff4453ba57), ROM_GROUPDWORD | ROM_REVERSE )

	ROM_REGION32_BE( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "305a07.20d", 0x000000, 0x020000, CRC(0959283b) SHA1(482caf96e8e430b87810508b1a1420cd3b58f203) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "305a08.2e", 0x000000, 0x200000, CRC(7ddb8a52) SHA1(3199b347fc433ffe0de8521001df77672d40771e) )
	ROM_LOAD( "305a09.3e", 0x200000, 0x200000, CRC(6da1be58) SHA1(d63ac16ac551193ff8a6036724fb59e1d702e06b) )

	ROM_REGION( 0x80, REGION_USER1, ROMREGION_ERASEFF )
	ROM_LOAD( "polynetw.nv", 0x0000, 0x0080, CRC(8f39d644) SHA1(8733e1a288ba20c4b04b3aedde52801d05cebdf9) )
ROM_END

GAME( 1993, plygonet, 0, plygonet, polygonet, polygonet, ROT90, "Konami", "Polygonet Commanders (ver UAA)", GAME_IMPERFECT_GRAPHICS )
GAME( 1993, polynetw, 0, plygonet, polynetw, polygonet, ROT90, "Konami", "Poly-Net Warriors (ver JAA)", GAME_IMPERFECT_GRAPHICS )
