// license:BSD-3-Clause
// copyright-holders:Ville Linde, Phil Bennett
/*  Konami M2 Hardware

    Preliminary driver by Ville Linde

    Games on this hardware:
    =======================

    Game                 | Konami ID
    ---------------------------------
    Evil Night           | GN810
    Heat of Eleven '98   | GX703
    Tobe! Polystars      | GX623
    Battle Tryst         | G?636

*/

/*
Tobe! PolyStars
Konami, 1997

This game runs on 3DO-based Konami M2 hardware.

PCB Layout
----------

Top Board

[M]DFUP0882ZAM1
FZ-20B1AK 7BKSA03500 (sticker)
|---------------------------------------------------|
|            |--------------------|    |----------| |
|            |--------------------|    |----------| |
|    2902             |---|  |--------|             |
| AK4309 CY2292S|---| |*2 |  |  3DO   |  |-------|  |
|               |*1 | |---|  |        |  |IBM    |  |
|        18MHz  |---|        |        |  |POWERPC|  |
|                            |        |  |602    |  |
|                            |--------|  |-------|  |
|    D4516161  D4516161                             |
|                                 |---|  |-------|  |
|DSW                    |-------| |*3 |  |IBM    |  |
|                       |       | |---|  |POWERPC|  |
|    D4516161  D4516161 |  *4   |        |602    |  |
|                       |       |        |-------|  |
|                       |-------|                   |
|---------------------------------------------------|
Notes:
      AK4309  - Asahi Kasei Microsystems AK4309-VM Digital to Analog Converter (SOIC24)
      2902    - Japan Radio Co. JRC2902 Quad Operational Amplifier (SOIC14)
      CY2292S - Cypress CY2292S Three-PLL General-Purpose EPROM Programmable Clock Generator (SOIC16)
                XTALIN - 18.000MHz, XTALOUT - 18.000MHz, XBUF - 18.000MHz, CPUCLK - 25.2000MHz
                CLKA - , CLKB -  , CLKC - 16.9345MHz, CLKD -
      3DO     - 9701 B861131 VY21118- CDE2 3DO 02473-001-0F (QFP208)
      *1      - [M] JAPAN ASUKA 9651HX001 044 (QFP44)
      *2      - Motorola MC44200FT
      *3      - [M] BIG BODY 2 BU6244KS 704 157 (QFP56)
      *4      - Unknown BGA chip (Graphics Engine, with heatsink attached)
      DSW     - 2 position dip switch


Bottom Board

PWB403045B (C) 1997 KONAMI CO., LTD.
|----------------------------------------------------------|
|           CN16    |--------------------|    |----------| |
|LA4705             |--------------------|    |----------| |
|       NJM5532D                    9.83MHz                |
|                                   19.66MHz               |
|J                |--------|   93C46.7K                    |-|
|A                | 058232 |                623B01.8Q      | |
|M                |--------|   |------|                    | |
|M       |------|              |003461|                    | |
|A       |056879|              |      |                    | |CN15
|        |      |              |------|                    | |
| TEST   |------|                                          | |
|                                                          | |
|   DSW                                                    | |
|                                                          |-|
|                                                          |
|----------------------------------------------------------|
Notes:
      056879   - Konami custom IC, location 10E (QFP120)
      058232   - Konami custom ceramic flat pack IC, DAC?
      003461   - Konami custom IC, location 11K (QFP100)
      CN16     - 4 pin connector for CD-DA in from CDROM
      CN15     - Standard (PC-compatible) 40 pin IDE CDROM flat cable connector and 4 pin power plug connector,
                 connected to Panasonic CR-583 8-speed CDROM drive.
                 CDROM disc contains 1 data track & 24 audio tracks. Total Capacity: 581 MBytes
                 Konami part number: 003894
                 Software revision: 623JAA02
      LA4705   - LA4705 Power Amplifier
      623B01.8Q- 16MBit MASKROM. Location 8Q (DIP42)
      93C46.7K - 128bytes x8bit Serial EEPROM. Location 7K (DIP8)
      DSW      - 8 position dip switch

*/

#include "driver.h"
#include "includes/konamim2.h"
#include "cdrom.h"
#include "sound/cdda.h"
#include "cpu/powerpc/ppc.h"
#include "machine/eeprom.h"
#include "machine/timekpr.h"
#include "sound/ymz280b.h"

static UINT64 *main_ram;

static UINT32 vdl0_address;
static UINT32 vdl1_address;

VIDEO_START( m2 )
{
	return 0;
}

VIDEO_UPDATE( m2 )
{
	int i, j;

	UINT32 fb_start = 0xffffffff;
	if (vdl0_address != 0)
	{
		fb_start = *(UINT32*)&main_ram[(vdl0_address - 0x40000000) / 8] - 0x40000000;
	}



	if (fb_start <= 0x800000)
	{
		UINT16 *frame = (UINT16*)&main_ram[fb_start/8];
		for (j=0; j < 384; j++)
		{
			UINT16 *fb = &frame[(j*512)];
			UINT16 *d = bitmap->line[j];
			for (i=0; i < 512; i++)
			{
				d[i^3] = *fb++;
			}
		}
	}
	else
	{
		fillbitmap(bitmap, 0, cliprect);
	}
}

static READ64_HANDLER(unk1_r)
{
	return U64(0xffffffffffffffff);
	//return 0;
}

static READ64_HANDLER(unk2_r)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)0xa5 << 32;
	}
	return 0;
}

static UINT64 unk3 = -1;
static READ64_HANDLER(unk3_r)
{
	//return U64(0xffffffffffffffff);
	return unk3;
}

static READ64_HANDLER(unk4_r)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return ((UINT64)0 << (13+32)) | ((UINT64)5 << (10+32));
	}
	return 0;
}

static int counter1 = 0;
static READ64_HANDLER(unk30000_r)
{
	counter1++;
	return (UINT64)(counter1 & 0x7f) << 32;
}

static READ64_HANDLER(unk4000280_r)
{
	//return (UINT64)(0x03640000) << 32;
	//return (UINT64)(0x0364ffff) << 32;
	UINT32 sys_config = 0x03640000;

	sys_config |= 0x00000000;		// Bit 0:       PAL/NTSC switch (default is selected by encoder)
	sys_config |= 0 << 2;			// Bit 2-3:     Video Encoder (0 = MEIENC, 1 = VP536, 2 = BT9103, 3 = DENC)
	sys_config |= 0 << 11;			// Bit 11-12:   Country
	sys_config |= 3 << 15;
	sys_config |= 1 << 29;			// Bit 29-30:   Audio chip (1 = CS4216, 3 = Asahi AK4309)

	return (UINT64)(sys_config) << 32;

}

static WRITE64_HANDLER(unk4000010_w)
{
	if ((data & 0xff) == 0xd)
	{
		printf("\n");
	}
	else
	{
		printf("%c", (UINT8)(data & 0xff));
	}
}

static WRITE64_HANDLER(unk4000418_w)
{
}

static WRITE64_HANDLER(reset_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		if (data & U64(0x100000000))
		{
			cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
			unk3 = 0;
		}
	}
}

static WRITE64_HANDLER(video_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		vdl0_address = (UINT32)(data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		vdl1_address = (UINT32)(data);
	}
}

static ADDRESS_MAP_START( m2_main, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00020000, 0x00020007) AM_READ(unk4_r)
//  AM_RANGE(0x00020000, 0x00020007) AM_WRITENOP
	AM_RANGE(0x00030000, 0x00030007) AM_READ(unk30000_r)
	AM_RANGE(0x00030010, 0x00030017) AM_WRITE(video_w)
	AM_RANGE(0x04000010, 0x04000017) AM_WRITE(unk4000010_w)
	AM_RANGE(0x04000018, 0x0400001f) AM_READ(unk1_r)
	AM_RANGE(0x04000020, 0x04000027) AM_WRITE(reset_w)
	AM_RANGE(0x04000418, 0x0400041f) AM_WRITE(unk4000418_w)
	AM_RANGE(0x04000208, 0x0400020f) AM_READ(unk3_r)
	AM_RANGE(0x04000280, 0x04000287) AM_READ(unk4000280_r)
//  AM_RANGE(0x10000000, 0x10000007) AM_RAM
	AM_RANGE(0x20000000, 0x201fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x40000000, 0x407fffff) AM_RAM AM_BASE(&main_ram)
	AM_RANGE(0xff000000, 0xff000fff) AM_RAM//AM_READ(unk1_r)
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END

INPUT_PORTS_START( m2 )
INPUT_PORTS_END


static ppc_config ppc602_config =
{
	PPC_MODEL_602,
	0x20,				/* Multiplier 2, Bus = 33MHz, Core = 66MHz */
	BUS_FREQUENCY_33MHZ
};

static INTERRUPT_GEN(m2)
{

	//cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);
}

static MACHINE_DRIVER_START( m2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC602, 33000000)	/* actually PPC602, 66MHz */
	MDRV_CPU_CONFIG(ppc602_config)
	MDRV_CPU_PROGRAM_MAP(m2_main, 0)
	MDRV_CPU_VBLANK_INT(m2, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 511, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(m2)
	MDRV_VIDEO_UPDATE(m2)

MACHINE_DRIVER_END


#define M2_RAM_BASE 0x40000000
#define M2_VTOTAL 260
#define M2_VSTART 20
#define M2_VINT0 0x80000000
#define M2_VINT1 0x00008000
#define M2_BIOBUS_DATAWIDTH_16 0x0800
#define M2_BIOBUS_READ_IO 0xe000

static UINT64 *m2_ram;
static UINT32 m2_ram_size;
static UINT32 m2_memcfg, m2_refresh;
static UINT32 m2_pb_status, m2_pb_enable;
static UINT32 m2_vint, m2_vdl, m2_video_reset;
static UINT32 m2_cde_status, m2_cde_enable, m2_boot_block, m2_visa, m2_serial;
static UINT16 m2_sio;
static UINT32 m2_biobus_setup[8];
static struct m2_dma_channel
{
	UINT32 control, source, destination, length;
	mame_timer *timer;
} m2_dma[2];
static mame_timer *m2_vint_timer[2], *m2_id_timer;
static int m2_guns, m2_irq_state;
static const char *const m2_gun_x_tag[] = { "GUNX1", "GUNX2", "GUNX3" };
static const char *const m2_gun_y_tag[] = { "GUNY1", "GUNY2", "GUNY3" };

static UINT16 m2_gun_x(unsigned gun)
{
	UINT16 x = readinputportbytag(m2_gun_x_tag[gun]);

	return x == 639 ? 0 : x;
}

void m2_irq(UINT32 bit, int state)
{
	if (state)
		m2_pb_status |= bit;
	else
		m2_pb_status &= ~bit;

	state = (m2_pb_status & m2_pb_enable) != 0;
	if (state != m2_irq_state)
	{
		m2_irq_state = state;
		cpunum_set_input_line(0, INPUT_LINE_IRQ0, state ? ASSERT_LINE : CLEAR_LINE);
	}
}

static UINT32 m2_powerbus_r(offs_t offset)
{
	switch (offset)
	{
		case 0x40: return m2_pb_enable;
		case 0x50: return m2_pb_status;
	}
	return 0;
}

static void m2_powerbus_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	data &= ~mem_mask;
	switch (offset)
	{
		case 0x040: m2_pb_enable |= data; break;
		case 0x440: m2_pb_enable &= ~data; break;
		case 0x060: if (data == 1) m2_pb_status |= 1; break;
		case 0x460: m2_pb_status &= ~data; break;
	}
	m2_irq(0, 0);
}

static UINT32 m2_memctl_r(offs_t offset)
{
	switch (offset)
	{
		case 0: return m2_memcfg;
		case 4: return m2_refresh;
	}
	return 0;
}

static void m2_memctl_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	switch (offset)
	{
		case 0:
			COMBINE_DATA(&m2_memcfg);
			break;
		case 4:
			COMBINE_DATA(&m2_refresh);
			if (m2_refresh & 0x00c00000)
				cpunum_set_input_line(1, INPUT_LINE_IRQ0,
					(m2_refresh & 0x00200000) ? CLEAR_LINE : ASSERT_LINE);
			break;
	}
}

static UINT32 m2_dsp_r(offs_t offset)
{
	return m2_dspp_read(offset >> 2);
}

static void m2_dsp_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	m2_dspp_write(offset >> 2, data & ~mem_mask);
}

static UINT32 m2_cpuid_r(offs_t offset)
{
	return (cpu_getactivecpu() == 1 ? 0x80000000 : 0) |
		(readinputportbytag("DSW") & 1 ? 0x40000000 : 0);
}

static void m2_schedule_vint(int index)
{
	unsigned line = index ? (m2_vint >> 4) & 0x7ff : (m2_vint >> 20) & 0x7ff;

	if (line)
	{
		int scanline = (line - 1 + M2_VTOTAL - M2_VSTART) % M2_VTOTAL;
		double delay = cpu_getscanlinetime(scanline);
		timer_adjust(m2_vint_timer[index], delay, index, 1.0 / Machine->drv->frames_per_second);
	}
	else
		timer_enable(m2_vint_timer[index], 0);
}

static void m2_raise_vint(int index)
{
	m2_vint |= index ? M2_VINT1 : M2_VINT0;
	m2_irq(1U << (24 - index), 1);
}

static UINT32 m2_vdu_r(offs_t offset)
{
	switch (offset)
	{
		case 0: return (((cpu_getscanline() + M2_VSTART) % M2_VTOTAL) + 1) << 3;
		case 4: return m2_vint;
	}
	return 0;
}

static void m2_vdu_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	UINT32 old;

	switch (offset)
	{
		case 4:
		case 0x404:
			old = m2_vint;
			if (offset & 0x400)
				m2_vint &= ~(data & ~mem_mask);
			else
				COMBINE_DATA(&m2_vint);
			if ((old ^ m2_vint) & 0x7ff00000)
				m2_schedule_vint(0);
			if ((old ^ m2_vint) & 0x00007ff0)
				m2_schedule_vint(1);
			if ((old & M2_VINT0) && !(m2_vint & M2_VINT0))
				m2_irq(1U << 24, 0);
			if ((old & M2_VINT1) && !(m2_vint & M2_VINT1))
				m2_irq(1U << 23, 0);
			break;
		case 0x10: COMBINE_DATA(&m2_vdl); break;
		case 0x34: COMBINE_DATA(&m2_video_reset); break;
	}
}

UINT32 m2_ram_dword(UINT32 address)
{
	UINT32 offset = address - M2_RAM_BASE;
	return offset < m2_ram_size ? m2_ram[offset >> 3] >> (32 - (offset & 4) * 8) : 0;
}

UINT16 m2_ram_word(UINT32 address)
{
	UINT32 offset = address - M2_RAM_BASE;
	return offset < m2_ram_size ? (m2_ram[offset >> 3] >> (48 - (offset & 6) * 8)) & 0xffff : 0;
}

UINT8 m2_ram_byte(UINT32 address)
{
	UINT32 offset = address - M2_RAM_BASE;
	return offset < m2_ram_size ? m2_ram[offset >> 3] >> ((7 - (offset & 7)) * 8) : 0;
}

void m2_ram_write_byte(UINT32 address, UINT8 data)
{
	UINT32 offset = address - M2_RAM_BASE;
	unsigned shift = (7 - (offset & 7)) * 8;

	if (offset < m2_ram_size)
	{
		UINT64 *word = &m2_ram[offset >> 3];
		*word = (*word & ~(U64(0xff) << shift)) | ((UINT64)data << shift);
	}
}

void m2_ram_write_word(UINT32 address, UINT16 data)
{
	UINT32 offset = address - M2_RAM_BASE;
	unsigned shift = (6 - (offset & 6)) * 8;

	if (offset < m2_ram_size)
	{
		UINT64 *word = &m2_ram[offset >> 3];
		*word = (*word & ~(U64(0xffff) << shift)) | ((UINT64)data << shift);
	}
}

void m2_ram_write_dword(UINT32 address, UINT32 data)
{
	UINT32 offset = address - M2_RAM_BASE;
	unsigned shift = (4 - (offset & 4)) * 8;

	if (offset < m2_ram_size)
	{
		UINT64 *word = &m2_ram[offset >> 3];
		*word = (*word & ~(U64(0xffffffff) << shift)) | ((UINT64)data << shift);
	}
}

static VIDEO_UPDATE( m2_board )
{
	UINT32 address = m2_vdl;
	unsigned y = 0, start = 0, width = 0, xrepeat = 1, yrepeat = 1, pixel_bytes = 2;

	fillbitmap(bitmap, 0, cliprect);
	if (m2_video_reset & 3)
		return;

	/* Each display-list header covers at least one line. Clip output even when the list points outside RAM. */
	while (y < M2_VTOTAL && address - M2_RAM_BASE < m2_ram_size)
	{
		UINT32 control = m2_ram_dword(address);
		UINT32 source = m2_ram_dword(address + 4);
		UINT32 next = m2_ram_dword(address + 12);
		unsigned words = (control >> 9) & 0x3f;
		unsigned lines = control & 0x1ff;
		unsigned stride = (control >> 24) << 5;
		unsigned word, line;

		for (word = 4; word < words; ++word)
		{
			UINT32 command = m2_ram_dword(address + 4 * word);
			switch (command >> 29)
			{
				case 5:
					if (command & 0x20000) start = (command >> 18) & 0x7ff;
					if (command & 0x20) width = (command >> 6) & 0x7ff;
					if (command & 4) xrepeat = (command & 0x10) ? 2 : 1;
					if (command & 2) yrepeat = (command & 8) ? 2 : 1;
					break;
				case 6:
					if (command & 0x1000) pixel_bytes = (command & 0x800000) ? 4 : 2;
					break;
			}
		}

		if (!lines)
			lines = M2_VTOTAL - y;
		for (line = 0; line < lines && y < M2_VTOTAL; ++line)
		{
			unsigned repeat = (control & 0x200000) ? yrepeat : 1;
			while (repeat-- && y < M2_VTOTAL)
			{
				if ((control & 0x200000) && y >= cliprect->min_y && y <= cliprect->max_y)
				{
					UINT32 *pixels = bitmap->line[y];
					unsigned end = start + width * xrepeat;
					unsigned x = MAX(start, cliprect->min_x);

					end = MIN(end, cliprect->max_x + 1);
					for (; x < end; ++x)
					{
						UINT32 pixel = source + (x - start) / xrepeat * pixel_bytes;
						if (pixel_bytes == 4)
							pixels[x] = m2_ram_dword(pixel) & 0xffffff;
						else
							{
							UINT16 rgb = m2_ram_word(pixel);
							pixels[x] = MAKE_RGB(pal5bit(rgb >> 10), pal5bit(rgb >> 5), pal5bit(rgb));
						}
					}
				}
				++y;
			}
			source += stride;
		}
		address = next;
	}

	if (m2_guns)
	{
		const rectangle *visible = &Machine->visible_area;
		unsigned gun;

		for (gun = 0; gun < ARRAY_LENGTH(m2_gun_x_tag); ++gun)
		{
			UINT16 x = m2_gun_x(gun);

			if (x)
				draw_crosshair(bitmap, visible->min_x + x,
					visible->min_y + readinputportbytag(m2_gun_y_tag[gun]), cliprect, gun);
		}
	}
}

static void m2_cde_irq(void)
{
	m2_irq(1U << 3, (m2_cde_status & m2_cde_enable) != 0);
}

static void m2_id_ready(int param)
{
	m2_cde_status |= 0x00100000;
	m2_cde_irq();
}

static void m2_dma_complete(int channel)
{
	m2_dma[channel].control &= ~0x80;
	m2_cde_status |= channel ? 0x100 : 0x80;
	m2_cde_irq();
}

static void m2_dma_start(int channel)
{
	struct m2_dma_channel *dma = &m2_dma[channel];
	UINT32 setup = m2_biobus_setup[(dma->source >> 26) & 7];
	UINT32 width = setup & M2_BIOBUS_DATAWIDTH_16 ? 2 : 1;
	UINT32 increment = setup & M2_BIOBUS_READ_IO ? 0 : width;

	/* The address is held for I/O DMA so each word comes from the ATAPI data register. */
	if ((dma->control & 0x400) || dma->source < 0x20000000 || dma->source >= 0x40000000 ||
		dma->destination - M2_RAM_BASE >= m2_ram_size ||
		dma->length > m2_ram_size - (dma->destination - M2_RAM_BASE) ||
		((dma->destination | dma->source | dma->length) & (width - 1)))
	{
		m2_cde_status |= channel ? 0x00400000 : 0x00200000;
		m2_dma_complete(channel);
		return;
	}

	while (dma->length)
	{
		if (width == 2)
			program_write_word_64be(dma->destination, program_read_word_64be(dma->source));
		else
			program_write_byte_64be(dma->destination, program_read_byte_64be(dma->source));
		dma->source += increment;
		dma->destination += width;
		dma->length -= width;
	}
	/* Report completion after the initiating CPU write has retired. */
	timer_adjust(dma->timer, TIME_IN_NSEC(10), channel, TIME_NEVER);
}

static UINT32 m2_cde_r(offs_t offset)
{
	switch (offset)
	{
		case 0x000: return 0x00010000;
		case 0x00c: return m2_serial;
		case 0x018: return m2_cde_status;
		case 0x01c: return m2_cde_enable;
		case 0x204: return 0x80;
		case 0x208: return m2_boot_block;
		case 0x224: return 0xffffffff;
		case 0x280: return 0x63659000;
		case 0x284: return m2_visa;
		case 0x2a0: return 0x20;
		case 0x1000: return m2_dma[0].control;
		case 0x1020: return m2_dma[1].control;
	}
	return 0;
}

static void m2_cde_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	struct m2_dma_channel *dma = &m2_dma[(offset >> 5) & 1];
	UINT32 old;

	if ((offset & ~0x438) == 0x240)
	{
		UINT32 *reg = &m2_biobus_setup[(offset >> 3) & 7];
		if (offset & 0x400)
			*reg &= ~(data & ~mem_mask);
		else
			COMBINE_DATA(reg);
		return;
	}

	switch (offset)
	{
		case 0x00c: COMBINE_DATA(&m2_serial); break;
		case 0x40c: m2_serial &= ~(data & ~mem_mask); break;
		case 0x014:
			m2_cde_status |= 0x10000000;
			m2_cde_irq();
			break;
		case 0x018: COMBINE_DATA(&m2_cde_status); m2_cde_irq(); break;
		case 0x418: m2_cde_status &= ~(data & ~mem_mask); m2_cde_irq(); break;
		case 0x01c: m2_cde_enable |= data & ~mem_mask; m2_cde_irq(); break;
		case 0x41c: m2_cde_enable &= ~(data & ~mem_mask); m2_cde_irq(); break;
		case 0x020:
			if (data & ~mem_mask & 1)
			{
				m2_boot_block = 0;
				cpunum_set_input_line(cpu_getactivecpu(), INPUT_LINE_RESET, PULSE_LINE);
			}
			break;
		case 0x220: timer_adjust(m2_id_timer, TIME_IN_USEC(250), 0, TIME_NEVER); break;
		case 0x284: COMBINE_DATA(&m2_visa); break;
		case 0x684: m2_visa &= ~(data & ~mem_mask); break;
		case 0x1000:
		case 0x1020:
			old = dma->control;
			COMBINE_DATA(&dma->control);
			if (!(old & 0x200) && (dma->control & 0x200))
			{
				timer_enable(dma->timer, 0);
				dma->control = dma->source = dma->destination = dma->length = 0;
			}
			else if (!(old & 0x80) && (dma->control & 0x80))
				m2_dma_start((offset >> 5) & 1);
			break;
		case 0x1400:
		case 0x1420:
			dma->control &= ~(data & ~mem_mask);
			break;
		case 0x1004:
		case 0x1024: COMBINE_DATA(&dma->source); break;
		case 0x1008:
		case 0x1028: COMBINE_DATA(&dma->destination); break;
		case 0x100c:
		case 0x102c: COMBINE_DATA(&dma->length); break;
	}
}

/* Both PPCs expose big-endian 32-bit registers through their 64-bit program bus. */
#define M2_REG_READ(name) \
	static READ64_HANDLER( name##_r64 ) \
	{ \
		UINT64 result = 0; \
		if ((UINT32)(mem_mask >> 32) != 0xffffffff) \
			result = (UINT64)name##_r(offset * 8) << 32; \
		if ((UINT32)mem_mask != 0xffffffff) \
			result |= name##_r(offset * 8 + 4); \
		return result; \
	}

#define M2_REG_WRITE(name) \
	static WRITE64_HANDLER( name##_w64 ) \
	{ \
		if ((UINT32)(mem_mask >> 32) != 0xffffffff) \
			name##_w(offset * 8, data >> 32, mem_mask >> 32); \
		if ((UINT32)mem_mask != 0xffffffff) \
			name##_w(offset * 8 + 4, data, mem_mask); \
	}

M2_REG_READ(m2_powerbus)
M2_REG_WRITE(m2_powerbus)
M2_REG_READ(m2_memctl)
M2_REG_WRITE(m2_memctl)
M2_REG_READ(m2_cpuid)
M2_REG_READ(m2_dsp)
M2_REG_WRITE(m2_dsp)

static UINT32 m2_triangle_r(offs_t offset)
{
	return m2_te_read(offset >> 2);
}

static void m2_triangle_w(offs_t offset, UINT32 data, UINT32 mem_mask)
{
	m2_te_write(offset >> 2, data & ~mem_mask);
}

M2_REG_READ(m2_triangle)
M2_REG_WRITE(m2_triangle)
M2_REG_READ(m2_vdu)
M2_REG_WRITE(m2_vdu)
M2_REG_READ(m2_cde)
M2_REG_WRITE(m2_cde)

static WRITE64_HANDLER( m2_eeprom_w )
{
	if ((mem_mask & U64(0xff00000000000000)) == 0)
	{
		EEPROM_write_bit((data >> 53) & 1);
		EEPROM_set_cs_line((data & U64(0x0080000000000000)) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & U64(0x0040000000000000)) ? ASSERT_LINE : CLEAR_LINE);
	}
}

#define M2_ATAPI_DRQ 0x08
#define M2_ATAPI_READY 0x40
#define M2_ATAPI_ERROR 0x01

enum { M2_ATAPI_RECEIVE_NONE, M2_ATAPI_RECEIVE_PACKET, M2_ATAPI_RECEIVE_MODE };

static cdrom_file *m2_cd;
static struct
{
	UINT8 reg[8], data[2048], packet[12];
	UINT8 audio_control, audio_volume[2];
	unsigned pos, length;
	UINT32 lba, sectors;
	int irq, receiving;
} m2_atapi;

static void m2_atapi_finish(int sense)
{
	m2_atapi.receiving = M2_ATAPI_RECEIVE_NONE;
	m2_atapi.reg[1] = sense << 4;
	m2_atapi.reg[2] = 3;
	m2_atapi.reg[7] = M2_ATAPI_READY | (sense ? M2_ATAPI_ERROR : 0);
	m2_atapi.irq = 1;
}

static void m2_atapi_data(unsigned size)
{
	m2_atapi.pos = 0;
	m2_atapi.length = size;
	m2_atapi.reg[2] = 2;
	m2_atapi.reg[4] = size;
	m2_atapi.reg[5] = size >> 8;
	m2_atapi.reg[7] = M2_ATAPI_READY | M2_ATAPI_DRQ;
	m2_atapi.irq = 1;
}

static void m2_atapi_sector(void)
{
	UINT32 lba = cdrom_phys_frame_to_chd(m2_cd, m2_atapi.lba);

	if (!cdrom_read_data(m2_cd, lba, 1, m2_atapi.data, CD_TRACK_MODE1))
	{
		m2_atapi.sectors = 0;
		m2_atapi_finish(3);
		return;
	}
	++m2_atapi.lba;
	--m2_atapi.sectors;
	m2_atapi_data(2048);
}

static UINT32 m2_cd_be32(const UINT8 *buffer)
{
	return ((UINT32)buffer[0] << 24) | ((UINT32)buffer[1] << 16) |
		((UINT32)buffer[2] << 8) | buffer[3];
}

static void m2_cd_put32(UINT8 *buffer, UINT32 value)
{
	buffer[0] = value >> 24;
	buffer[1] = value >> 16;
	buffer[2] = value >> 8;
	buffer[3] = value;
}

static UINT32 m2_cd_binary_msf(UINT32 frame)
{
	return ((frame / 4500) << 16) | (((frame / 75) % 60) << 8) | (frame % 75);
}

static UINT32 m2_cd_msf(const UINT8 *msf)
{
	UINT32 frames = (msf[0] * 60 + msf[1]) * 75 + msf[2];
	return frames > 150 ? frames - 150 : 0;
}

static void m2_cd_play(UINT32 lba, UINT32 count)
{
	UINT32 start = cdrom_phys_frame_to_chd(m2_cd, lba);
	if (count && cdrom_get_track_type(m2_cd, cdrom_get_track_chd(m2_cd, start)))
	{
		cdrom_start_audio(m2_cd, start, count);
		m2_atapi_finish(0);
	}
	else
		m2_atapi_finish(5);
}

static unsigned m2_cd_toc(void)
{
	const UINT8 *cmd = m2_atapi.packet;
	UINT8 *data = m2_atapi.data;
	int tracks = cdrom_get_last_track(m2_cd);
	int track = cmd[6] ? cmd[6] : 1;
	unsigned length = 4;

	data[2] = 1;
	data[3] = tracks;
	for (; track <= tracks || track == 0xaa; ++track)
	{
		UINT32 addr = cdrom_get_phys_start_of_track(m2_cd, track == 0xaa ? tracks : track - 1);
		if (cmd[1] & 2)
			addr = m2_cd_binary_msf(addr + 150);
		data[length + 1] = cdrom_get_adr_control(m2_cd, track == 0xaa ? track : track - 1);
		data[length + 2] = track;
		m2_cd_put32(data + length + 4, addr);
		length += 8;
		if (track == 0xaa)
			break;
		if (track == tracks)
			track = 0xa9;
	}
	data[0] = (length - 2) >> 8;
	data[1] = length - 2;
	return length;
}

static void m2_cd_audio_mode_select(void)
{
	const UINT8 *data = m2_atapi.data;
	unsigned length = m2_atapi.length;
	unsigned offset = length >= 8 ? 8 + ((data[6] << 8) | data[7]) : length;

	/* Audio page parameters can follow a mode header or start at byte zero. */
	if (length >= 16 && data[0] == 0x0e && data[1] == 14)
		offset = 0;
	if (offset + 16 <= length && (data[offset] & 0x3f) == 0x0e && data[offset + 1] == 14)
	{
		m2_atapi.audio_control = data[offset + 2] & 2;
		m2_atapi.audio_volume[0] = data[offset + 9];
		m2_atapi.audio_volume[1] = data[offset + 11];
		sndti_set_output_gain(SOUND_CDDA, 0, 0, m2_atapi.audio_volume[0] / 255.0f);
		sndti_set_output_gain(SOUND_CDDA, 0, 1, m2_atapi.audio_volume[1] / 255.0f);
	}
	m2_atapi_finish(0);
}

static void m2_cd_audio_mode_sense(void)
{
	UINT8 *data = m2_atapi.data;
	unsigned offset = m2_atapi.packet[1] & 8 ? 0 : 8;

	if (offset)
	{
		data[1] = offset + 16 - 2;
		data[2] = cdrom_get_last_track(m2_cd) > 1 ? 3 : 1;
	}
	data[offset] = 0x8e;
	data[offset + 1] = 14;
	data[offset + 2] = 4 | m2_atapi.audio_control;
	data[offset + 8] = 1;
	data[offset + 9] = m2_atapi.audio_volume[0];
	data[offset + 10] = 2;
	data[offset + 11] = m2_atapi.audio_volume[1];
	data[offset + 12] = 4;
	data[offset + 13] = 0xff;
	data[offset + 14] = 8;
	data[offset + 15] = 0xff;
}

static void m2_atapi_packet(void)
{
	const UINT8 *cmd = m2_atapi.packet;
	UINT8 *data = m2_atapi.data;
	unsigned size = 0, alloc = (cmd[7] << 8) | cmd[8];
	UINT32 lba = m2_cd_be32(cmd + 2);

	m2_atapi.sectors = 0;
	memset(data, 0, sizeof(m2_atapi.data));
	switch (cmd[0])
	{
		case 0x00:
		case 0x1b:
		case 0x1e:
		case 0xbb:
			m2_atapi_finish(0);
			return;
		case 0x03:
			data[0] = 0x70;
			data[7] = 10;
			size = MIN(cmd[4], 18);
			break;
		case 0x12:
			data[0] = 5;
			data[1] = 0x80;
			data[3] = 2;
			data[4] = 31;
			memcpy(data + 8, "MATSHITACR-589          1.0 ", 28);
			size = MIN(cmd[4], 36);
			break;
		case 0x25:
			m2_cd_put32(data, cdrom_get_phys_start_of_track(m2_cd, cdrom_get_last_track(m2_cd)) - 1);
			m2_cd_put32(data + 4, 2048);
			size = 8;
			break;
		case 0x28:
		case 0xa8:
			cdrom_stop_audio(m2_cd);
			m2_atapi.lba = lba;
			m2_atapi.sectors = cmd[0] == 0x28 ? alloc : m2_cd_be32(cmd + 6);
			if (m2_atapi.sectors)
				m2_atapi_sector();
			else
				m2_atapi_finish(0);
			return;
		case 0x42:
			data[1] = cdrom_audio_active(m2_cd) ? (cdrom_audio_paused(m2_cd) ? 0x12 : 0x11) :
				(cdrom_audio_ended(m2_cd) ? 0x13 : 0x15);
			size = 4;
			if (cmd[2] & 0x40 && cmd[3] == 1)
			{
				UINT32 frame = cdrom_audio_active(m2_cd) ?
					cdrom_chd_frame_to_phys(m2_cd, cdrom_get_audio_lba(m2_cd)) : 0;
				unsigned track = cdrom_get_track_phys(m2_cd, frame);
				UINT32 relative = frame - cdrom_get_phys_start_of_track(m2_cd, track);

				data[3] = 12;
				data[4] = 1;
				data[5] = cdrom_get_adr_control(m2_cd, track);
				data[6] = track + 1;
				data[7] = 1;
				if (cmd[1] & 2)
				{
					frame = m2_cd_binary_msf(frame + 150);
					relative = m2_cd_binary_msf(relative);
				}
				m2_cd_put32(data + 8, frame);
				m2_cd_put32(data + 12, relative);
				size = 16;
			}
			size = MIN(size, alloc);
			break;
		case 0x43:
			size = MIN(m2_cd_toc(), alloc);
			break;
		case 0x45:
			m2_cd_play(lba, alloc);
			return;
		case 0x47:
		{
			UINT32 start = m2_cd_msf(cmd + 3);
			UINT32 end = m2_cd_msf(cmd + 6);
			m2_cd_play(start, end > start ? end - start : 0);
			return;
		}
		case 0x48:
		{
			unsigned start = cmd[4], end = cmd[7];
			unsigned tracks = cdrom_get_last_track(m2_cd);

			if (start && start <= tracks && end >= start && end <= tracks)
			{
				UINT32 first = cdrom_get_phys_start_of_track(m2_cd, start - 1);
				m2_cd_play(first, cdrom_get_phys_start_of_track(m2_cd, end) - first);
			}
			else
				m2_atapi_finish(5);
			return;
		}
		case 0x4b:
			cdrom_pause_audio(m2_cd, !(cmd[8] & 1));
			m2_atapi_finish(0);
			return;
		case 0x4e:
			cdrom_stop_audio(m2_cd);
			m2_atapi_finish(0);
			return;
		case 0x55:
			if (alloc && alloc <= sizeof(m2_atapi.data))
			{
				m2_atapi_data(alloc);
				m2_atapi.reg[2] = 0;
				m2_atapi.receiving = M2_ATAPI_RECEIVE_MODE;
			}
			else
				m2_atapi_finish(alloc ? 5 : 0);
			return;
		case 0x5a:
			if ((cmd[2] & 0x3f) != 0x0e)
			{
				m2_atapi_finish(5);
				return;
			}
			m2_cd_audio_mode_sense();
			size = MIN(alloc, sizeof(m2_atapi.data));
			break;
		default:
			logerror("Konami M2 ATAPI command %02x\n", cmd[0]);
			m2_atapi_finish(5);
			return;
	}

	if (size)
		m2_atapi_data(size);
	else
		m2_atapi_finish(0);
}

static void m2_atapi_reset(void)
{
	memset(&m2_atapi, 0, sizeof(m2_atapi));
	m2_atapi.reg[1] = 1;
	m2_atapi.reg[2] = 1;
	m2_atapi.reg[3] = 1;
	m2_atapi.reg[4] = 0x14;
	m2_atapi.reg[5] = 0xeb;
	m2_atapi.audio_volume[0] = m2_atapi.audio_volume[1] = 0xff;
	if (m2_cd)
	{
		cdrom_stop_audio(m2_cd);
		sndti_set_output_gain(SOUND_CDDA, 0, 0, 1.0f);
		sndti_set_output_gain(SOUND_CDDA, 0, 1, 1.0f);
	}
}

static void m2_atapi_command(UINT8 cmd)
{
	static const char model[] = "MATSHITA CR-589                         ";
	unsigned i;

	m2_atapi.irq = 0;
	m2_atapi.reg[1] = 0;
	m2_atapi.receiving = M2_ATAPI_RECEIVE_NONE;
	m2_atapi.sectors = 0;
	switch (cmd)
	{
		case 0x08:
			m2_atapi_reset();
			break;
		case 0xa0:
			m2_atapi.pos = 0;
			m2_atapi.receiving = M2_ATAPI_RECEIVE_PACKET;
			m2_atapi.reg[2] = 1;
			m2_atapi.reg[7] = M2_ATAPI_READY | M2_ATAPI_DRQ;
			break;
		case 0xa1:
			memset(m2_atapi.data, 0, 512);
			m2_atapi.data[1] = 0x85;
			memcpy(m2_atapi.data + 46, ".1 0    ", 8);
			for (i = 0; i < 40; ++i)
				m2_atapi.data[54 + i] = model[i ^ 1];
			m2_atapi.data[99] = 4;
			m2_atapi_data(512);
			break;
		case 0xef:
			m2_atapi_finish(0);
			break;
		default:
			m2_atapi_finish(5);
			break;
	}
}

static UINT16 m2_atapi_read(unsigned reg)
{
	UINT16 result;

	if (m2_atapi.reg[6] & 0x10)
		return 0;
	if (reg != 0)
	{
		if (reg == 7)
			m2_atapi.irq = 0;
		return m2_atapi.reg[reg] << 8;
	}

	if (!(m2_atapi.reg[7] & M2_ATAPI_DRQ) || m2_atapi.receiving)
		return 0;

	/* On the big-endian CPU bus the first byte of a packet or sector occupies the high lane. */
	result = m2_atapi.data[m2_atapi.pos++] << 8;
	if (m2_atapi.pos < m2_atapi.length)
		result |= m2_atapi.data[m2_atapi.pos++];
	if (m2_atapi.pos >= m2_atapi.length)
	{
		if (m2_atapi.sectors)
			m2_atapi_sector();
		else
			m2_atapi_finish(0);
	}
	return result;
}

static void m2_atapi_write(unsigned reg, UINT16 data, UINT16 mask)
{
	if (reg && (mask & 0xff00))
		return;
	if (reg == 6)
		m2_atapi.reg[6] = data >> 8;
	if (m2_atapi.reg[6] & 0x10)
		return;

	if (reg == 7)
		m2_atapi_command(data >> 8);
	else if (reg && (mask & 0xff00) == 0)
		m2_atapi.reg[reg] = data >> 8;
	else if (reg == 0 && m2_atapi.receiving == M2_ATAPI_RECEIVE_PACKET)
	{
		m2_atapi.packet[m2_atapi.pos++] = data >> 8;
		m2_atapi.packet[m2_atapi.pos++] = data;
		if (m2_atapi.pos == sizeof(m2_atapi.packet))
		{
			m2_atapi.receiving = M2_ATAPI_RECEIVE_NONE;
			m2_atapi_packet();
		}
	}
	else if (reg == 0 && m2_atapi.receiving == M2_ATAPI_RECEIVE_MODE)
	{
		m2_atapi.data[m2_atapi.pos++] = data >> 8;
		if (m2_atapi.pos < m2_atapi.length)
			m2_atapi.data[m2_atapi.pos++] = data;
		if (m2_atapi.pos == m2_atapi.length)
			m2_cd_audio_mode_select();
	}
}

static READ64_HANDLER( m2_atapi_r64 )
{
	UINT64 result = 0;
	int lane;
	for (lane = 0; lane < 4; ++lane)
	{
		int shift = 48 - lane * 16;
		if ((UINT16)(mem_mask >> shift) != 0xffff)
			result |= (UINT64)m2_atapi_read((offset * 4 + lane) & 7) << shift;
	}
	return result;
}

static WRITE64_HANDLER( m2_atapi_w64 )
{
	int lane;
	for (lane = 0; lane < 4; ++lane)
	{
		int shift = 48 - lane * 16;
		UINT16 mask = mem_mask >> shift;
		if (mask != 0xffff)
			m2_atapi_write((offset * 4 + lane) & 7, data >> shift, mask);
	}
}

static WRITE64_HANDLER( m2_atapi_reset_w )
{
	if (offset == 0 && (mem_mask & U64(0xff00000000000000)) == 0 &&
		!(data & U64(0x8000000000000000)))
		m2_atapi_reset();
}

static void m2_cd_close(void)
{
	CDDA_set_cdrom(0, NULL);
	cdrom_close(m2_cd);
	m2_cd = NULL;
}

static UINT16 m2_io_r(offs_t word)
{
	switch (word)
	{
		case 2: return readinputportbytag("DIP");
		case 3: return readinputportbytag("COIN") | (EEPROM_read_bit() << 8) | 0x1c00 |
			(m2_atapi.irq ? 0x2000 : 0);
		case 4: return readinputportbytag("PLAYERS");
		case 6: return readinputportbytag("TEST");
	}
	return 0xffff;
}

static READ64_HANDLER( m2_io_r64 )
{
	return ((UINT64)m2_io_r(offset * 4) << 48) |
		((UINT64)m2_io_r(offset * 4 + 1) << 32) |
		((UINT64)m2_io_r(offset * 4 + 2) << 16) |
		m2_io_r(offset * 4 + 3);
}

static WRITE64_HANDLER( m2_io_w64 )
{
	if (offset == 0 && (UINT16)mem_mask != 0xffff)
	{
		coin_counter_w(0, data & 0x0800);
		coin_counter_w(1, data & 0x1000);
	}
}

static READ64_HANDLER( m2_sio_r64 )
{
	return offset == 1 ? m2_sio : 0;
}

static WRITE64_HANDLER( m2_sio_w64 )
{
	if (offset == 1)
	{
		UINT16 mask = mem_mask;
		m2_sio = (m2_sio & mask) | (data & ~mask);
	}
}

static UINT16 m2_gun_r(offs_t word)
{
	unsigned gun;
	UINT16 x, value;

	if (!m2_guns)
		return word == 3 || word == 8 ? 0xffff : 0;
	if (word == 8)
		return readinputportbytag("TRIGGERS");
	if (word == 3)
		return 0xffff;
	if (word >= 7)
		return 0;

	gun = word & 3;
	x = m2_gun_x(gun);
	value = word < 4 || !x ? x : readinputportbytag(m2_gun_y_tag[gun]);
	return (value >> 8) | (value << 8);
}

static READ64_HANDLER( m2_gun_r64 )
{
	return ((UINT64)m2_gun_r(offset * 4) << 48) |
		((UINT64)m2_gun_r(offset * 4 + 1) << 32) |
		((UINT64)m2_gun_r(offset * 4 + 2) << 16) |
		m2_gun_r(offset * 4 + 3);
}

static READ64_HANDLER( m2_timekeeper_r64 )
{
	UINT64 data = 0;
	int lane;
	for (lane = 0; lane < 4; ++lane)
	{
		int shift = 48 - lane * 16;
		UINT16 mask = mem_mask >> shift;
		if (!(mask & 0xff00))
			data |= (UINT64)timekeeper_0_msb16_r(offset * 4 + lane, mask) << shift;
	}
	return data;
}

static WRITE64_HANDLER( m2_timekeeper_w64 )
{
	int lane;
	for (lane = 0; lane < 4; ++lane)
	{
		int shift = 48 - lane * 16;
		UINT16 mask = mem_mask >> shift;
		if (!(mask & 0xff00))
			timekeeper_0_msb16_w(offset * 4 + lane, data >> shift, mask);
	}
}

static READ64_HANDLER( m2_ymz_r64 )
{
	UINT64 result = 0;
	if (offset == 0)
	{
		if ((mem_mask & U64(0xff00000000000000)) == 0)
			result = (UINT64)YMZ280B_data_0_r(0) << 56;
		if ((mem_mask & U64(0x0000ff0000000000)) == 0)
			result |= (UINT64)YMZ280B_status_0_r(0) << 40;
	}
	return result;
}

static WRITE64_HANDLER( m2_ymz_w64 )
{
	if (offset == 0)
	{
		if ((mem_mask & U64(0xff00000000000000)) == 0)
			YMZ280B_register_0_w(0, data >> 56);
		if ((mem_mask & U64(0x0000ff0000000000)) == 0)
			YMZ280B_data_0_w(0, data >> 40);
	}
}

static NVRAM_HANDLER( m2_board )
{
	static struct EEPROM_interface eeprom = { 6, 16, "*110", "*101", "*111", "*10000xxxx", "*10011xxxx", 0, 0 };

	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom);
		if (file)
			EEPROM_load(file);
		else
			EEPROM_set_data(memory_region(REGION_USER2), memory_region_length(REGION_USER2));
	}

	if (m2_guns)
		nvram_handler_timekeeper_0(file, read_or_write);
}

static MACHINE_START( m2_board )
{
	m2_cd = cdrom_open(get_disk_handle(0));
	if (!m2_cd)
		return 1;
	add_exit_callback(m2_cd_close);
	CDDA_set_cdrom(0, m2_cd);

	m2_vint_timer[0] = timer_alloc(m2_raise_vint);
	m2_vint_timer[1] = timer_alloc(m2_raise_vint);
	m2_id_timer = timer_alloc(m2_id_ready);
	m2_dma[0].timer = timer_alloc(m2_dma_complete);
	m2_dma[1].timer = timer_alloc(m2_dma_complete);
	m2_te_init();
	m2_dspp_init();
	return 0;
}

static MACHINE_RESET( m2_board )
{
	int channel;

	m2_memcfg = (m2_guns ? 5 : 2) * 0x2400;
	m2_refresh = 0;
	m2_pb_enable = m2_pb_status = 0;
	m2_irq_state = 0;
	memset(m2_biobus_setup, 0, sizeof(m2_biobus_setup));
	for (channel = 0; channel < 2; ++channel)
	{
		struct m2_dma_channel *dma = &m2_dma[channel];
		dma->control = dma->source = dma->destination = dma->length = 0;
		timer_enable(dma->timer, 0);
	}
	m2_vint = m2_vdl = m2_video_reset = 0;
	m2_cde_status = m2_cde_enable = m2_visa = m2_serial = 0;
	m2_boot_block = 1;
	m2_sio = 0;
	m2_te_reset();
	m2_dspp_reset();
	m2_atapi_reset();
	timer_enable(m2_vint_timer[0], 0);
	timer_enable(m2_vint_timer[1], 0);
	timer_enable(m2_id_timer, 0);
}

static ADDRESS_MAP_START( m2_board, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00010000, 0x0001ffff) AM_READWRITE(m2_powerbus_r64, m2_powerbus_w64)
	AM_RANGE(0x00020000, 0x0002ffff) AM_READWRITE(m2_memctl_r64, m2_memctl_w64)
	AM_RANGE(0x00030000, 0x0003ffff) AM_READWRITE(m2_vdu_r64, m2_vdu_w64)
	AM_RANGE(0x00040000, 0x0004ffff) AM_READWRITE(m2_triangle_r64, m2_triangle_w64)
	AM_RANGE(0x00060000, 0x0006ffff) AM_READWRITE(m2_dsp_r64, m2_dsp_w64)
	AM_RANGE(0x000c0000, 0x000c3fff) AM_READWRITE(m2_te_tram_r, m2_te_tram_w)
	AM_RANGE(0x04000000, 0x04ffffff) AM_READWRITE(m2_cde_r64, m2_cde_w64)
	AM_RANGE(0x10000000, 0x10000007) AM_READ(m2_cpuid_r64)
	AM_RANGE(0x20000000, 0x201fffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x37400000, 0x37400007) AM_WRITE(m2_eeprom_w)
	AM_RANGE(0x37600000, 0x3760000f) AM_WRITE(m2_atapi_reset_w)
	AM_RANGE(0x37a00020, 0x37a0003f) AM_READ(m2_gun_r64)
	AM_RANGE(0x37c00010, 0x37c0001f) AM_READWRITE(m2_sio_r64, m2_sio_w64)
	AM_RANGE(0x37e00000, 0x37e0000f) AM_READWRITE(m2_io_r64, m2_io_w64)
	AM_RANGE(0x3f000000, 0x3f00000f) AM_MIRROR(0x00fffff0) AM_READWRITE(m2_atapi_r64, m2_atapi_w64)
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( m2_polystar, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x40000000, 0x407fffff) AM_RAM AM_SHARE(1) AM_BASE(&m2_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( m2_evilngt, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x36c00000, 0x36c03fff) AM_READWRITE(m2_timekeeper_r64, m2_timekeeper_w64)
	AM_RANGE(0x3e800000, 0x3e80000f) AM_READWRITE(m2_ymz_r64, m2_ymz_w64)
	AM_RANGE(0x40000000, 0x40ffffff) AM_RAM AM_SHARE(1) AM_BASE(&m2_ram)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( m2_board )
	MDRV_CPU_ADD_TAG("main", PPC602, M2_CLOCK)
	MDRV_CPU_CONFIG(ppc602_config)
	MDRV_CPU_ADD_TAG("sub", PPC602, M2_CLOCK)
	MDRV_CPU_CONFIG(ppc602_config)
	MDRV_INTERLEAVE(512)

	MDRV_MACHINE_START(m2_board)
	MDRV_MACHINE_RESET(m2_board)
	MDRV_NVRAM_HANDLER(m2_board)

	MDRV_FRAMES_PER_SECOND(59.360001)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(768, M2_VTOTAL)
	MDRV_VISIBLE_AREA(126, 765, M2_VSTART, 259)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_VIDEO_UPDATE(m2_board)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CDDA, 0)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "right", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(0, "right", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( m2_polystar )
	MDRV_IMPORT_FROM(m2_board)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(m2_board, m2_polystar)
	MDRV_CPU_MODIFY("sub")
	MDRV_CPU_PROGRAM_MAP(m2_board, m2_polystar)
MACHINE_DRIVER_END

static const struct YMZ280Binterface m2_ymz = { REGION_SOUND1 };

static MACHINE_DRIVER_START( m2_evilngt )
	MDRV_IMPORT_FROM(m2_board)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(m2_board, m2_evilngt)
	MDRV_CPU_MODIFY("sub")
	MDRV_CPU_PROGRAM_MAP(m2_board, m2_evilngt)
	MDRV_SOUND_ADD(YMZ280B, 16934400)
	MDRV_SOUND_CONFIG(m2_ymz)
	MDRV_SOUND_ROUTE(0, "left", 0.5)
	MDRV_SOUND_ROUTE(1, "right", 0.5)
MACHINE_DRIVER_END

static DRIVER_INIT( m2_polystar )
{
	m2_guns = 0;
	m2_ram_size = 0x800000;
}

static DRIVER_INIT( m2_evilngt )
{
	m2_guns = 1;
	m2_ram_size = 0x1000000;
	timekeeper_init(0, TIMEKEEPER_M48T58, memory_region(REGION_USER3));
}


static INPUT_PORTS_START( polystar )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Video Res" )
	PORT_DIPSETTING(    0x00, "High Res" )
	PORT_DIPSETTING(    0x01, "Low Res" )

	PORT_START_TAG("DIP")
	PORT_DIPNAME( 0x01, 0x01, "Sound Output" )
	PORT_DIPSETTING(    0x01, "Mono" )
	PORT_DIPSETTING(    0x00, "Stereo" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PLAYERS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("TRIGGERS")
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("TEST")
	PORT_SERVICE_NO_TOGGLE( 0x0004, IP_ACTIVE_LOW )

	PORT_START_TAG("COIN")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_BIT(0xec, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

static INPUT_PORTS_START( evilngt )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, "Video Res" )
	PORT_DIPSETTING(    0x00, "High Res" )
	PORT_DIPSETTING(    0x01, "Low Res" )
	PORT_START_TAG("DIP")
	PORT_BIT( 0x1, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("PLAYERS")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("GUNX1")
	PORT_BIT( 0xffff, 0x0000, IPT_LIGHTGUN_X ) PORT_MINMAX(0, 639) PORT_SENSITIVITY(50) PORT_KEYDELTA(20) PORT_PLAYER(1)

	PORT_START_TAG("GUNY1")
	PORT_BIT( 0xffff, 0x0000, IPT_LIGHTGUN_Y ) PORT_MINMAX(0, 239) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START_TAG("GUNX2")
	PORT_BIT( 0xffff, 0x0000, IPT_LIGHTGUN_X ) PORT_MINMAX(0, 639) PORT_SENSITIVITY(50) PORT_KEYDELTA(20) PORT_PLAYER(2)

	PORT_START_TAG("GUNY2")
	PORT_BIT( 0xffff, 0x0000, IPT_LIGHTGUN_Y ) PORT_MINMAX(0, 239) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START_TAG("GUNX3")
	PORT_BIT( 0xffff, 0x0000, IPT_LIGHTGUN_X ) PORT_MINMAX(0, 639) PORT_SENSITIVITY(50) PORT_KEYDELTA(20) PORT_PLAYER(3)

	PORT_START_TAG("GUNY3")
	PORT_BIT( 0xffff, 0x0000, IPT_LIGHTGUN_Y ) PORT_MINMAX(0, 239) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(3)

	PORT_START_TAG("TRIGGERS")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) PORT_NAME("P1 Gun Trigger")
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Gun Trigger")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3) PORT_NAME("P3 Gun Trigger")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) PORT_NAME("Gun Reload")
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Gun Reload")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3) PORT_NAME("Gun Reload")
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("TEST")
	PORT_SERVICE_NO_TOGGLE( 0x0004, IP_ACTIVE_LOW )
	PORT_BIT( 0xfffb, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("COIN")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_COIN2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_BIT(0xec, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

ROM_START(polystar)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("623b01.8q", 0x000000, 0x200000, CRC(bd879f93) SHA1(e2d63bfbd2b15260a2664082652442eadea3eab6))

	ROM_REGION( 0x80, REGION_USER2, 0 )
	ROM_LOAD( "93c46.7k", 0x000000, 0x000080, CRC(fab5a203) SHA1(153e22aa8cfce80b77ba200957685f796fc99b1c) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "623jaa02", 0, BAD_DUMP SHA1(e7d9e628a3e0e085e084e4e3630fa5e3a7345547) )
ROM_END

ROM_START(btltryst)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "btltryst", 0, MD5(4286d25a896d1450705f742cccd26ef2) SHA1(becc606b8480f6a09365b611565d83cfdc82b0b3))
ROM_END

ROM_START(heatof11)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* timekeeper eeprom */
	ROM_LOAD( "dallas.5e",  0x000000, 0x002000, CRC(5b74eafd) SHA1(afbf5f1f5a27407fd6f17c764bbb7fae4ab779f5) )

	ROM_REGION16_BE( 0x80, REGION_USER3, 0 ) /* eeprom default contents */
	ROM_LOAD( "93c46.7k",  0x000000, 0x000080, CRC(e7029938) SHA1(ae41340dbcb600debe246629dc36fb371d1a5b05) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "703eaa02", 0, SHA1(f8a87eacfdbbd22659f39c7a72e3895f0a7697b7) )
ROM_END

ROM_START(evilngt)
	ROM_REGION64_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_WORD( "636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b) )

	ROM_REGION( 0x80, REGION_USER2, 0 )
	ROM_LOAD( "93c46.7k", 0x000000, 0x000080, CRC(e632f292) SHA1(85c5292cd4002a272126d06bb50fb1bd38476936) )

	ROM_REGION( 0x2000, REGION_USER3, 0 )
	ROM_LOAD( "m48t58y.9n", 0x000000, 0x002000, CRC(e887ca1f) SHA1(54205f01b1ceba1d5f4d979fc30be1add8116e90) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "810a03.16h", 0x000000, 0x400000, CRC(05112d3a) SHA1(0df2a167b7bc08a32d983b71614d59834efbfb59) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE_READONLY( "810uba02", 0, SHA1(e570470c1cbfe187d5bba8125616412f386264ba) )
ROM_END

ROM_START(totlvice)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x100000, REGION_USER2, 0 ) /* sound rom on sub board */
	ROM_LOAD( "639jaa02.bin",  0x000000, 0x100000, CRC(c6163818) SHA1(b6f8f2d808b98610becc0a5be5443ece3908df0b) )
ROM_END

static DRIVER_INIT( btltryst )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	rom[(0x16508^4) / 4] = 0x60000000;
}

static DRIVER_INIT( m2 )
{
}


GAME( 1997, polystar,	0,	m2_polystar, polystar, m2_polystar,	ROT0,	"Konami",	"Tobe! Polystars (ver JAA)", GAME_IMPERFECT_SOUND )
GAME( 1997, totlvice,	0,	m2, m2, m2,				ROT0,	"Konami",	"Total Vice (ver JAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, btltryst,	0,	m2,	m2,	btltryst,		ROT0,	"Konami",	"Battle Tryst (ver JAC)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, heatof11,	0,	m2,	m2,	m2,				ROT0,	"Konami",	"Heat of Eleven '98 (ver EAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, evilngt,	0,	m2_evilngt, evilngt, m2_evilngt,	ROT0,	"Konami",	"Evil Night (ver UBA)", 0 )
