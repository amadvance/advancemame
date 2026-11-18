// license:LGPL-2.1+
// copyright-holders:David Haywood, Angelo Salese, ElSemi, Andrew Gardner

/* Hyper NeoGeo 64 */

#define MASTER_CLOCK	50000000
#include "driver.h"
#include "includes/hng64.h"
#include "cpu/mips/mips3.h"
#include "machine/pic8259.h"
#include "machine/pit8253.h"
#include "sound/custom.h"
#include <time.h>

static UINT32 *rombase;
static UINT32 *hng_mainram;
static UINT32 *hng_cart;
static UINT32 *hng64_dualport;
static UINT32 *hng64_sram;
static UINT8 *hng64_soundram;


static UINT32 no_machine_error_code;
static UINT8 hng64_mcu_mode;
static int hng64_samurai_io;
static double hng64_io_command_time;
static int hng64_io_test;
static UINT32 hng64_prev_coin;
static UINT32 hng64_irq_pending;
static UINT32 *hng64_sysregs;
static time_t hng64_rtc_epoch;
static UINT8 hng64_rtc_ctrl[3];

static UINT32 *hng64_dl;

static UINT32 *hng64_com_ram;

static UINT8 *hng64_com_virtual_mem;
static UINT8 *hng64_com_op_base;

static UINT8 *hng64_com_mmu_mem;

static WRITE32_HANDLER( hng64_videoram_w )
{
	COMBINE_DATA(&hng64_videoram[offset]);
	hng64_tile_dirty(offset);
}

static READ32_HANDLER( hng64_com_r )
{

	return hng64_com_ram[offset] ;
}

static WRITE32_HANDLER( hng64_com_w )
{

	COMBINE_DATA(&hng64_com_ram[offset]);
}

static UINT32 *hng64_com_status;
static UINT8 hng64_com_shared[8];
static mame_timer *hng64_com_timer;

static void hng64_com_ready(int param)
{
	/* Standalone cabinets proceed after the network discovery timeout. */
	hng64_com_status[0] |= 1;
}

static WRITE32_HANDLER( hng64_pal_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	b = ((paletteram32[offset] & 0x000000ff) >>0);
	g = ((paletteram32[offset] & 0x0000ff00) >>8);
	r = ((paletteram32[offset] & 0x00ff0000) >>16);

	palette_set_color(offset,r,g,b);
}

static void hng64_set_irq(UINT32 pending)
{
	hng64_irq_pending |= pending;
	cpunum_set_input_line(0, 0, hng64_irq_pending ? ASSERT_LINE : CLEAR_LINE);
}

static READ32_HANDLER( hng64_irq_r )
{
	int level;

	if (offset != 1)
		return 0xffffffff;

	for (level = 0; level < 32; ++level)
		if (hng64_irq_pending & ((UINT32)1 << level))
			return level;

	return 0;
}

static WRITE32_HANDLER( hng64_irq_w )
{
	if (offset == 7)
	{
		hng64_irq_pending &= ~(data & ~mem_mask);
		hng64_set_irq(0);
	}
}

static READ32_HANDLER( hng64_port_read )
{
	switch (offset * 4)
	{
		case 0x001c: return 0;
		case 0x1084: return 2;
	}
	return hng64_sysregs[offset];
}

static WRITE32_HANDLER( hng64_port_write )
{
	COMBINE_DATA(&hng64_sysregs[offset]);
	if (offset == 0x1084 / 4 && ACCESSING_LSB32)
	{
		if (hng64_mcu_mode != 0x0b && (data & 0xff) == 0x0b)
		{
			hng64_io_command_time = timer_get_time();
			hng64_io_test = 0;
		}
		hng64_mcu_mode = data;
	}
}

static UINT32 hng_dma_start, hng_dma_dst;
static INT32 hng_dma_len;
static void hng64_do_dma (void)
{

	while (hng_dma_len >= 0)
	{
		UINT32 dat;

		dat = program_read_dword(hng_dma_start);
		program_write_dword(hng_dma_dst,dat);
		hng_dma_start+=4;
		hng_dma_dst+=4;
		hng_dma_len--;
	}

}

static READ32_HANDLER( hng_dma_status_r )
{
	return 0;
}

static WRITE32_HANDLER( hng_dma_start_w )
{
	logerror ("DMA Start Write %08x\n",data);
	COMBINE_DATA(&hng_dma_start);
}

static WRITE32_HANDLER( hng_dma_dst_w )
{
	logerror ("DMA Dst Write %08x\n",data);
	COMBINE_DATA(&hng_dma_dst);
}

static WRITE32_HANDLER( hng_dma_len_w )
{
	logerror ("DMA Len Write %08x\n",data);
	COMBINE_DATA(&hng_dma_len);
	hng64_do_dma();

}

static READ32_HANDLER( hng64_sram_r )
{

	return hng64_sram[offset];
}

static WRITE32_HANDLER( hng64_sram_w )
{

	COMBINE_DATA (&hng64_sram[offset]);
}

static WRITE32_HANDLER( hng64_dualport_w )
{
	COMBINE_DATA(&hng64_dualport[offset]);

	/* The I/O board returns two patterns after the startup test command. */
	if (!offset && hng64_samurai_io && hng64_mcu_mode == 0x0b && !(data & 0xff000000))
	{
		if (hng64_io_test == 0 && mem_mask == 0x00ffffff)
		{
			hng64_io_test = 1;
			hng64_io_command_time = timer_get_time();
		}
		else if (hng64_io_test == 1 && !mem_mask)
			hng64_io_test = 2;
	}
	else if (!offset && !hng64_samurai_io && !(mem_mask & 0xff000000) && (data & 0xff000000) == 0x0a000000)
		hng64_io_test = 2;
}

static READ32_HANDLER( hng64_dualport_r )
{
	if (hng64_mcu_mode == 0x0c)
		return hng64_dualport[offset];

	switch (offset * 4)
	{
		case 0x00:
			if (hng64_io_test == 2)
				break;
			if (hng64_samurai_io)
			{
				double elapsed = timer_get_time() - hng64_io_command_time;
				if (hng64_io_test == 1)
				{
					if (elapsed < 1.0 / 60)
						return 0x5500;
					if (elapsed < 5.5)
						return 0xaa00;
					return 0x400;
				}
				if (hng64_mcu_mode == 0x0b && elapsed >= 2.0 / 60)
					return 0x300;
				return 0;
			}
			return 0x400;
		case 0x04:
			if (no_machine_error_code != 0x03000000)
				return readinputportbytag("SYSTEM");
			break;
		case 0x08:
			if (no_machine_error_code != 0x03000000)
				return readinputportbytag("P1_P2");
			break;
		case 0x10:
			if (no_machine_error_code == 0x03000000)
				return readinputportbytag("D_IN");
			break;
		case 0x14:
			if (no_machine_error_code == 0x02000000)
				return (UINT32)readinputportbytag("DRIVE") << 8;
			break;
		case 0x18:
			if (no_machine_error_code == 0x02000000 || no_machine_error_code == 0x03000000)
				return ((UINT32)readinputportbytag("AN0") << 24)
					| ((UINT32)readinputportbytag("AN1") << 16)
					| ((UINT32)readinputportbytag("AN2") << 8)
					| readinputportbytag("AN3");
			break;
		case 0x1c:
			if (no_machine_error_code == 0x03000000)
				return (UINT32)readinputportbytag("AN4") * 0x01000100
					| (UINT32)readinputportbytag("AN5") * 0x00010001;
			break;
		case 0x600: return no_machine_error_code;
	}

	return hng64_dualport[offset];
}

static WRITE32_HANDLER( dl_w )
{
	COMBINE_DATA(&hng64_dl[offset]);
}

static void hng64_fifo_done(int param)
{
	hng64_set_irq(0x0008);
}

static void hng64_queue_packet(const UINT32 *packet)
{
	if (hng64_packet_count < HNG64_MAX_PACKETS)
		memcpy(hng64_packets[hng64_packet_count++], packet, 8 * sizeof(*packet));
}

static void hng64_queue_translation(const UINT32 *packet)
{
	UINT32 expanded[8] = { packet[0], packet[1], packet[2],
		(packet[3] & 0xffff0000) | 0x7fff, 0, 0x7fff, 0, 0x7fff };
	expanded[0] = (expanded[0] & 0xffff) | 0x01000000;
	hng64_queue_packet(expanded);
}

static WRITE32_HANDLER( hng64_fifo_w )
{
	int item;
	for (item = 0; item < 0x80; item += 8)
	{
		UINT32 *packet = hng64_dl + item;
		switch (packet[0] >> 16)
		{
			case 0:
				item = 0x80;
				break;
			case 0x102:
				hng64_queue_translation(packet);
				if ((packet[3] & 0xffff) == 1 && packet[4] >> 16 == 0x102)
					hng64_queue_translation(packet + 4);
				break;
			default:
				hng64_queue_packet(packet);
				break;
		}
	}
	timer_set(TIME_IN_SEC(4096.0 / MASTER_CLOCK), 0, hng64_fifo_done);
}

static READ32_HANDLER( hng64_fifo_status_r )
{
	return 0;
}

static WRITE32_HANDLER( tcram_w )
{
	COMBINE_DATA (&hng64_tcram[offset]) ;

}

static READ32_HANDLER( tcram_r )
{

	/* The BIOS waits on both edges of this status bit during video startup. */
	if (offset == 0x48 / 4)
		return cpu_getvblank() ? 0xffffffff : 0;
	return hng64_tcram[offset];
}

static UINT16 hng64_main_latch[2], hng64_audio_latch[2];
static UINT8 hng64_sound_bank[16], hng64_v53_regs[0x20];

struct hng64_dma_channel
{
	UINT32 address;
	UINT16 count;
	UINT8 mode;
};

static struct hng64_dma_channel hng64_sound_dma[4];
static UINT8 hng64_dma_select, hng64_dma_mask, hng64_dma_status, hng64_dma_request;

static READ32_HANDLER( hng64_soundram_r )
{
	const UINT8 *ram = hng64_soundram + offset * 4;
	return ((UINT32)ram[0] << 24) | ((UINT32)ram[1] << 16) | ((UINT32)ram[2] << 8) | ram[3];
}

static WRITE32_HANDLER( hng64_soundram_w )
{
	UINT8 *ram = hng64_soundram + offset * 4;
	UINT32 word = (hng64_soundram_r(offset, 0) & mem_mask) | (data & ~mem_mask);
	ram[0] = word >> 24;
	ram[1] = word >> 16;
	ram[2] = word >> 8;
	ram[3] = word;
}

static void hng64_sound_pic_irq(int which, int state)
{
	cpunum_set_input_line(1, 0, state);
}

static int hng64_sound_pic_ack(int line)
{
	return pic8259_acknowledge(0);
}

static void hng64_sound_timer2(int state)
{
	pic8259_set_irq_line(0, 2, state);
}

static const struct pit8253_config hng64_sound_pit =
{
	TYPE8254,
	{ { 8000000, NULL, NULL }, { 8000000, NULL, NULL }, { 8000000, hng64_sound_timer2, NULL } }
};

static void hng64_audio_set_bank(unsigned slot, UINT8 bank)
{
	hng64_sound_bank[slot] = bank & 31;
	memory_set_bankptr(slot + 1, hng64_soundram + (hng64_sound_bank[slot] << 16));
}

static void hng64_sound_reset(void)
{
	int bank;
	memset(hng64_v53_regs, 0, sizeof(hng64_v53_regs));
	memset(hng64_sound_dma, 0, sizeof(hng64_sound_dma));
	memset(hng64_main_latch, 0, sizeof(hng64_main_latch));
	memset(hng64_audio_latch, 0, sizeof(hng64_audio_latch));
	hng64_dma_select = hng64_dma_status = hng64_dma_request = 0;
	hng64_dma_mask = 0x0f;
	for (bank = 0; bank < 16; ++bank)
		hng64_audio_set_bank(bank, 31);
	pit8253_reset(0);
	cpunum_set_irq_callback(1, hng64_sound_pic_ack);
	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
}

static WRITE32_HANDLER( hng64_sound_enable_w )
{
	if ((mem_mask & 0xffff0000) == 0)
	{
		switch (data >> 16)
		{
			case 0x55aa:
				cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
				cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE);
				break;
			case 0xaa55:
				hng64_sound_reset();
				break;
		}
	}
}

static READ32_HANDLER( hng64_main_sound_r )
{
	switch (offset)
	{
		case 0: return ((UINT32)hng64_main_latch[0] << 16) | hng64_main_latch[1];
		case 1: return ((UINT32)hng64_audio_latch[0] << 16) | hng64_audio_latch[1];
	}
	return 0;
}

static WRITE32_HANDLER( hng64_main_sound_w )
{
	if (offset == 0)
	{
		UINT32 word = (hng64_main_sound_r(0, 0) & mem_mask) | (data & ~mem_mask);
		hng64_main_latch[0] = word >> 16;
		hng64_main_latch[1] = word;
	}
	else if (offset == 2 && (data & ~mem_mask & 0x10000))
	{
		pic8259_set_irq_line(0, 5, 1);
		cpu_spinuntil_time(TIME_IN_USEC(5));
	}
}

static READ8_HANDLER( hng64_sound_comms_r )
{
	switch (offset >> 1)
	{
		case 0: case 1: return hng64_audio_latch[offset >> 1] >> ((offset & 1) * 8);
		case 2: case 3: return hng64_main_latch[(offset >> 1) - 2] >> ((offset & 1) * 8);
	}
	return 0;
}

static WRITE8_HANDLER( hng64_sound_comms_w )
{
	if (offset < 4)
	{
		int shift = (offset & 1) * 8;
		UINT16 *word = &hng64_audio_latch[offset >> 1];
		*word = (*word & ~(0xff << shift)) | (data << shift);
	}
	else if ((offset >> 1) == 5)
		pic8259_set_irq_line(0, 5, 0);
}

static WRITE8_HANDLER( hng64_sound_bank_w )
{
	if (!(offset & 1))
		hng64_audio_set_bank(offset >> 1, data);
}

static UINT8 *hng64_dma_mem(UINT32 address)
{
	return hng64_soundram + ((UINT32)hng64_sound_bank[(address >> 16) & 15] << 16) + (address & 0xffff);
}

static void hng64_dma_service(void)
{
	struct hng64_dma_channel *dma = &hng64_sound_dma[3];
	UINT32 remaining = (UINT32)dma->count + 1;

	/* Only channel 3 is wired to the sample DSP on this board. */
	if (!(hng64_dma_request & 8) || (hng64_dma_mask & 8))
		return;

	while (remaining--)
	{
		UINT32 addr = dma->address;
		if ((dma->mode & 12) == 8)
			hng64_dsp_dma_w(*hng64_dma_mem(addr) | (*hng64_dma_mem(addr + 1) << 8));
		else if ((dma->mode & 12) == 4)
		{
			UINT16 data = hng64_dsp_dma_r();
			*hng64_dma_mem(addr) = data;
			*hng64_dma_mem(addr + 1) = data >> 8;
		}

		dma->address += (dma->mode & 0x20) ? -1 : 1;
		--dma->count;
	}

	hng64_dma_status |= 8;
	hng64_dma_mask |= 8;
}

void hng64_sound_drq(int state)
{
	if (state)
		hng64_dma_request |= 8;
	else
		hng64_dma_request &= ~8;
	hng64_dma_service();
}

static READ8_HANDLER( hng64_dma_r )
{
	struct hng64_dma_channel *dma = &hng64_sound_dma[hng64_dma_select];

	switch (offset)
	{
		case 1: return 1 << hng64_dma_select;
		case 2: case 3: return dma->count >> ((offset - 2) * 8);
		case 4: case 5: case 6: case 7: return dma->address >> ((offset - 4) * 8);
		case 10: return dma->mode;
		case 11:
		{
			UINT8 status = hng64_dma_status;
			hng64_dma_status = 0;
			return status;
		}
		case 14: return hng64_dma_request;
		case 15: return hng64_dma_mask;
	}
	return 0;
}

static WRITE8_HANDLER( hng64_dma_w )
{
	struct hng64_dma_channel *dma = &hng64_sound_dma[hng64_dma_select];
	int shift;

	switch (offset)
	{
		case 0:
			if (data & 1)
			{
				hng64_dma_mask = 0x0f;
				hng64_dma_status = hng64_dma_request = 0;
			}
			break;
		case 1: hng64_dma_select = data & 3; break;
		case 2: case 3:
			shift = (offset - 2) * 8;
			dma->count = (dma->count & ~(0xff << shift)) | (data << shift);
			break;
		case 4: case 5: case 6: case 7:
			shift = (offset - 4) * 8;
			dma->address = (dma->address & ~((UINT32)0xff << shift)) | ((UINT32)data << shift);
			break;
		case 10: dma->mode = data; break;
		case 14: hng64_dma_request = data; hng64_dma_service(); break;
		case 15: hng64_dma_mask = data; hng64_dma_service(); break;
	}
}

static int hng64_v53_peripheral(unsigned port, unsigned reg, unsigned count, unsigned step)
{
	unsigned base = (hng64_v53_regs[0x1c] << 8) | (hng64_v53_regs[reg] & ~(count * step - 1));
	unsigned delta = port - base;
	if (hng64_v53_regs[0x1c] == 0xff || delta >= count * step)
		return -1;
	if (step == 2 && (delta & 1) != (hng64_v53_regs[reg] & 1))
		return -1;
	return delta / step;
}

static READ8_HANDLER( hng64_v53_io_r )
{
	unsigned step = (hng64_v53_regs[0x1e] & 1) ? 1 : 2;
	unsigned enabled = hng64_v53_regs[0x1d];
	int reg;
	if (offset >= 0xffe0)
		return hng64_v53_regs[offset - 0xffe0];
	if ((enabled & 1) && (reg = hng64_v53_peripheral(offset, 0x1b, 16, 1)) >= 0)
		return hng64_dma_r(reg);
	if ((enabled & 2) && (reg = hng64_v53_peripheral(offset, 0x1a, 2, step)) >= 0)
		return pic8259_0_r(reg);
	if ((enabled & 4) && (reg = hng64_v53_peripheral(offset, 0x19, 4, step)) >= 0)
		return pit8253_0_r(reg);
	if (offset < 0x10)
		return hng64_dsp_r(offset);
	if (offset >= 0x100 && offset < 0x110)
		return hng64_sound_comms_r(offset - 0x100);
	return 0xff;
}

static WRITE8_HANDLER( hng64_v53_io_w )
{
	unsigned step = (hng64_v53_regs[0x1e] & 1) ? 1 : 2;
	unsigned enabled = hng64_v53_regs[0x1d];
	int reg;
	if (offset >= 0xffe0)
	{
		hng64_v53_regs[offset - 0xffe0] = data;
		if (offset == 0xfff0)
			for (reg = 0; reg < 3; ++reg)
				pit8253_set_clockin(0, reg, (data & (4 << reg)) ? 0 : 8000000 >> (data & 3));
	}
	else if ((enabled & 1) && (reg = hng64_v53_peripheral(offset, 0x1b, 16, 1)) >= 0)
		hng64_dma_w(reg, data);
	else if ((enabled & 2) && (reg = hng64_v53_peripheral(offset, 0x1a, 2, step)) >= 0)
		pic8259_0_w(reg, data);
	else if ((enabled & 4) && (reg = hng64_v53_peripheral(offset, 0x19, 4, step)) >= 0)
		pit8253_0_w(reg, data);
	else if (offset < 0x10)
		hng64_dsp_w(offset, data);
	else if (offset >= 0x100 && offset < 0x110)
		hng64_sound_comms_w(offset - 0x100, data);
	else if (offset >= 0x200 && offset < 0x220)
		hng64_sound_bank_w(offset - 0x200, data);
}

static READ32_HANDLER( hng64_rtc_r )
{
	int reg = (offset >> 1) & 15;
	time_t now;
	const struct tm *clock;
	int fields[6];

	if (!(offset & 1))
		return 0xffffffff;

	/* The board supplies this extra bit on the system log enable register. */
	if (reg >= 13)
		return hng64_rtc_ctrl[reg - 13] | (reg == 13 ? 0x10 : 0);

	now = hng64_rtc_epoch + (time_t)timer_get_time();
	clock = localtime(&now);
	if (reg == 12)
		return clock->tm_wday;

	fields[0] = clock->tm_sec;
	fields[1] = clock->tm_min;
	fields[2] = clock->tm_hour;
	fields[3] = clock->tm_mday;
	fields[4] = clock->tm_mon + 1;
	fields[5] = clock->tm_year % 100;
	return reg & 1 ? fields[reg >> 1] / 10 : fields[reg >> 1] % 10;
}

static WRITE32_HANDLER( hng64_rtc_w )
{
	int reg = (offset >> 1) & 15;
	if ((offset & 1) && reg >= 13 && ACCESSING_LSB32)
		hng64_rtc_ctrl[reg - 13] = data & 15;
}

static void hng64_clear_sprite_words(offs_t offset, UINT32 mask, int odd)
{
	UINT32 *sprite = hng64_spriteram + offset * 16;
	int i;

	for (i = 0; i < 2; ++i)
	{
		if ((mask & (0xffff0000U >> (16 * i))) != (0xffff0000U >> (16 * i)))
		{
			UINT32 *entry = sprite + i * 8;
			entry[odd] = entry[odd + 4] = entry[odd + 6] = 0;
			if (!odd)
				entry[2] = 0;
		}
	}
}

static WRITE32_HANDLER( hng64_sprite_clear_even_w )
{
	hng64_clear_sprite_words(offset, mem_mask, 0);
}

static WRITE32_HANDLER( hng64_sprite_clear_odd_w )
{
	hng64_clear_sprite_words(offset, mem_mask, 1);
}

static READ32_HANDLER( hng64_texture_wrap_r )
{
	const UINT8 *wrap = hng64_texture_wrap + offset * 4;
	return ((UINT32)wrap[0] << 24) | ((UINT32)wrap[1] << 16) | ((UINT32)wrap[2] << 8) | wrap[3];
}

static WRITE32_HANDLER( hng64_texture_wrap_w )
{
	UINT32 value = (hng64_texture_wrap_r(offset, 0) & mem_mask) | (data & ~mem_mask);
	UINT8 *wrap = hng64_texture_wrap + offset * 4;
	wrap[0] = value >> 24;
	wrap[1] = value >> 16;
	wrap[2] = value >> 8;
	wrap[3] = value;
}

static ADDRESS_MAP_START( hng_map, ADDRESS_SPACE_PROGRAM, 32 )

	AM_RANGE(0x00000000, 0x00ffffff) AM_RAM AM_BASE(&hng_mainram)
	AM_RANGE(0x04000000, 0x05ffffff) AM_WRITENOP AM_ROM AM_REGION(REGION_USER3,0) AM_BASE(&hng_cart)

	AM_RANGE(0x1f700000, 0x1f7010ff) AM_READWRITE(hng64_port_read, hng64_port_write) AM_BASE(&hng64_sysregs)
	AM_RANGE(0x1f701100, 0x1f70111f) AM_READWRITE(hng64_irq_r, hng64_irq_w)

	AM_RANGE(0x1F701204, 0x1F701207) AM_WRITE(hng_dma_start_w)
	AM_RANGE(0x1F701214, 0x1F701217) AM_WRITE(hng_dma_dst_w)
	AM_RANGE(0x1F701224, 0x1F701227) AM_WRITE(hng_dma_len_w)
	AM_RANGE(0x1f701254, 0x1f701257) AM_READ(hng_dma_status_r)
	AM_RANGE(0x1f702100, 0x1f70217f) AM_READWRITE(hng64_rtc_r, hng64_rtc_w)

	AM_RANGE(0x1F800000, 0x1F803fff) AM_READWRITE(hng64_sram_r, hng64_sram_w) AM_BASE(&hng64_sram)

	AM_RANGE(0x1F808000, 0x1F8087ff) AM_READWRITE(hng64_dualport_r, hng64_dualport_w) AM_BASE(&hng64_dualport)

	AM_RANGE(0x1fc00000, 0x1fc7ffff) AM_WRITENOP AM_ROM AM_REGION(REGION_USER1, 0) AM_BASE(&rombase)

	AM_RANGE(0x20000000, 0x2000bfff) AM_RAM AM_BASE(&hng64_spriteram)
	AM_RANGE(0x2000d800, 0x2000e3ff) AM_WRITE(hng64_sprite_clear_even_w)
	AM_RANGE(0x2000e400, 0x2000efff) AM_WRITE(hng64_sprite_clear_odd_w)
	AM_RANGE(0x20010000, 0x20010013) AM_RAM AM_BASE(&hng64_spriteregs)
	AM_RANGE(0x20100000, 0x2017ffff) AM_READWRITE(MRA32_RAM, hng64_videoram_w) AM_BASE(&hng64_videoram)
	AM_RANGE(0x20190000, 0x20190037) AM_RAM AM_BASE(&hng64_videoregs)
	AM_RANGE(0x20200000, 0x20203fff) AM_READWRITE(MRA32_RAM,hng64_pal_w) AM_BASE(&paletteram32)
	AM_RANGE(0x20208000, 0x2020805f) AM_READWRITE(tcram_r, tcram_w) AM_BASE(&hng64_tcram)
	AM_RANGE(0x20300000, 0x203001ff) AM_WRITE(dl_w) AM_BASE(&hng64_dl)
	AM_RANGE(0x20300200, 0x20300203) AM_WRITE(hng64_fifo_w)
	AM_RANGE(0x20300210, 0x20300217) AM_WRITENOP
	AM_RANGE(0x20300218, 0x2030021b) AM_READ(hng64_fifo_status_r)

	AM_RANGE(0x30000000, 0x3000000f) AM_RAM AM_BASE(&hng64_fbregs)
	AM_RANGE(0x30000010, 0x3000002f) AM_READWRITE(hng64_texture_wrap_r, hng64_texture_wrap_w)
	AM_RANGE(0x30100000, 0x3015ffff) AM_RAM
	AM_RANGE(0x30200000, 0x3025ffff) AM_RAM

	AM_RANGE(0x60000000, 0x601fffff) AM_NOP
	AM_RANGE(0x60200000, 0x603fffff) AM_READWRITE(hng64_soundram_r, hng64_soundram_w)
	AM_RANGE(0x68000000, 0x6800000f) AM_READWRITE(hng64_main_sound_r, hng64_main_sound_w)
	AM_RANGE(0x6f000000, 0x6f000003) AM_WRITE(hng64_sound_enable_w)

	AM_RANGE(0xc0000000, 0xc0000fff) AM_READWRITE(hng64_com_r, hng64_com_w) AM_BASE(&hng64_com_ram)
	AM_RANGE(0xc0001000, 0xc0001007) AM_RAM AM_BASE(&hng64_com_status)

ADDRESS_MAP_END

static OPBASE_HANDLER( KL5C80_opbase_handler )
{
	opcode_base = opcode_arg_base = hng64_com_op_base;
	return ~0;
}

static unsigned KL5C80_page_base(unsigned page)
{
	int region;

	for (region = 3; region >= 0; --region)
	{
		unsigned boundary = hng64_com_mmu_mem[region * 2] & 0x3f;
		if (page > boundary)
		{
			unsigned base = region == 3 ? 0x3c0 :
				(hng64_com_mmu_mem[region * 2 + 1] << 2) |
				(hng64_com_mmu_mem[region * 2] >> 6);
			return ((page + base) & 0x3ff) << 10;
		}
	}
	return page << 10;
}

static void KL5C80_virtual_mem_sync(void)
{
	unsigned page;

	/* Opcode fetches bypass the memory handlers, so keep a translated view. */
	for (page = 0; page < 64; ++page)
		memcpy(hng64_com_op_base + (page << 10),
			hng64_com_virtual_mem + KL5C80_page_base(page), 0x400);
}

static void KL5C80_init(void)
{
	memset(hng64_com_mmu_mem, 0, 8);
	hng64_com_mmu_mem[0] = hng64_com_mmu_mem[2] =
		hng64_com_mmu_mem[4] = hng64_com_mmu_mem[6] = 0x3f;
	hng64_com_mmu_mem[7] = 0xf0;
}

static READ8_HANDLER( hng64_comm_memory_r )
{
	return hng64_com_virtual_mem[KL5C80_page_base(offset >> 10) | (offset & 0x3ff)];
}

static WRITE8_HANDLER( hng64_comm_memory_w )
{
	unsigned base = KL5C80_page_base(offset >> 10);
	unsigned within_page = offset & 0x3ff;

	if (base >= 0x40000)
	{
		unsigned page;
		hng64_com_virtual_mem[base | within_page] = data;
		for (page = 0; page < 64; ++page)
			if (KL5C80_page_base(page) == base)
				hng64_com_op_base[(page << 10) | within_page] = data;
	}
}

static READ8_HANDLER( hng64_comm_io_mmu_r )
{
	return hng64_com_mmu_mem[offset];
}

static WRITE8_HANDLER( hng64_comm_io_mmu_w )
{
	if (offset != 7)
	{
		hng64_com_mmu_mem[offset] = offset == 6 ? data & 0x3f : data;
		KL5C80_virtual_mem_sync();
	}
}

static ADDRESS_MAP_START( hng_comm_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(hng64_comm_memory_r, hng64_comm_memory_w)
ADDRESS_MAP_END

static READ8_HANDLER( hng64_comm_shared_r )
{
	return hng64_com_shared[offset] | (offset == 4);
}

static WRITE8_HANDLER( hng64_comm_shared_w )
{
	hng64_com_shared[offset] = data;
}

static WRITE8_HANDLER( hng64_comm_bank_w )
{
	memcpy(hng64_com_virtual_mem + 0x20000,
		memory_region(REGION_USER2) + (data & 3) * 0x20000, 0x20000);
	KL5C80_virtual_mem_sync();
}

static ADDRESS_MAP_START( hng_comm_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0x07) AM_READWRITE(hng64_comm_io_mmu_r, hng64_comm_io_mmu_w) AM_BASE(&hng64_com_mmu_mem)
	AM_RANGE(0x50, 0x57) AM_READWRITE(hng64_comm_shared_r, hng64_comm_shared_w)
	AM_RANGE(0x72, 0x72) AM_WRITE(hng64_comm_bank_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hng_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAMBANK(1)
	AM_RANGE(0x10000, 0x1ffff) AM_RAMBANK(2)
	AM_RANGE(0x20000, 0x2ffff) AM_RAMBANK(3)
	AM_RANGE(0x30000, 0x3ffff) AM_RAMBANK(4)
	AM_RANGE(0x40000, 0x4ffff) AM_RAMBANK(5)
	AM_RANGE(0x50000, 0x5ffff) AM_RAMBANK(6)
	AM_RANGE(0x60000, 0x6ffff) AM_RAMBANK(7)
	AM_RANGE(0x70000, 0x7ffff) AM_RAMBANK(8)
	AM_RANGE(0x80000, 0x8ffff) AM_RAMBANK(9)
	AM_RANGE(0x90000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xaffff) AM_RAMBANK(11)
	AM_RANGE(0xb0000, 0xbffff) AM_RAMBANK(12)
	AM_RANGE(0xc0000, 0xcffff) AM_RAMBANK(13)
	AM_RANGE(0xd0000, 0xdffff) AM_RAMBANK(14)
	AM_RANGE(0xe0000, 0xeffff) AM_RAMBANK(15)
	AM_RANGE(0xf0000, 0xfffff) AM_RAMBANK(16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hng_sound_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(hng64_v53_io_r, hng64_v53_io_w)
ADDRESS_MAP_END

static INPUT_PORTS_START( hng64 )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x0000ffff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE_NO_TOGGLE(0x80000000, IP_ACTIVE_HIGH)

	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static INPUT_PORTS_START( hng64_drive )
	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE(0x80000000, IP_ACTIVE_HIGH)
	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("BGM 1")
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("BGM 2")
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("BGM 3")
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("BGM 4")
	PORT_START_TAG("DRIVE")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("View 1")
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("View 2")
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Shift Down")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_NAME("Shift Up")
	PORT_START_TAG("AN0")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(60) PORT_NAME("Handle")
	PORT_START_TAG("AN1")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(60) PORT_REVERSE PORT_NAME("Accelerator")
	PORT_START_TAG("AN2")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(50) PORT_KEYDELTA(60) PORT_REVERSE PORT_NAME("Brake")
	PORT_START_TAG("AN3")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( bbust2 )
	PORT_START_TAG("D_IN")
	PORT_BIT( 0x000000ff, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_COIN3 ) PORT_IMPULSE(1)
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_SERVICE2 )
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_SERVICE3 )
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_UNUSED )

		PORT_START_TAG("AN0")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(7) PORT_REVERSE PORT_PLAYER(1)
	PORT_START_TAG("AN1")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(7) PORT_REVERSE PORT_PLAYER(1)
	PORT_START_TAG("AN2")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(7) PORT_REVERSE PORT_PLAYER(2)
	PORT_START_TAG("AN3")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(7) PORT_REVERSE PORT_PLAYER(2)
	PORT_START_TAG("AN4")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(7) PORT_REVERSE PORT_PLAYER(3)
	PORT_START_TAG("AN5")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(7) PORT_REVERSE PORT_PLAYER(3)
INPUT_PORTS_END

static const gfx_layout hng64_8x8x4_tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 24, 28, 8, 12, 16, 20, 0, 4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static const gfx_layout hng64_8x8x8_tilelayout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 24,     8,     16,     0,
		256+24, 256+8, 256+16, 256+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	16*32
};

static const gfx_layout hng64_16x16x4_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 24,     28,     8,     12,     16,     20,     0,     4,
		256+24, 256+28, 256+8, 256+12, 256+16, 256+20, 256+0, 256+4 },
	{ 0*32,  1*32,  2*32,  3*32,  4*32,  5*32,  6*32,  7*32,
		16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

static const gfx_layout hng64_16x16x8_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 24,      8,      16,      0,
		256+24,  256+8,  256+16,  256+0,
		1024+24, 1024+8, 1024+16, 1024+0,
		1280+24, 1280+8, 1280+16, 1280+0, },
	{ 0*32,  1*32,  2*32,  3*32,  4*32,  5*32,  6*32,  7*32,
		16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	64*32
};

static const gfx_layout hng64_4_16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{  56,60, 24, 28,  48,52,   16, 20,40, 44, 8,12,  32,36,    0,4 },

	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};

static const gfx_layout hng64_16_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },

	{ 56, 24, 48,16,  40, 8,  32, 0,
	 1024+56, 1024+24, 1024+48,1024+16,  1024+40, 1024+8,  1024+32, 1024+0 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,8*64,9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	32*64
};

static const gfx_decode hng64_gfxdecode[] =
{
	{ REGION_GFX1, 0, &hng64_8x8x4_tilelayout, 0x0, 0x100 },
	{ REGION_GFX1, 0, &hng64_8x8x8_tilelayout, 0x0, 0x10 },
	{ REGION_GFX1, 0, &hng64_16x16x4_tilelayout, 0x0, 0x100 },
	{ REGION_GFX1, 0, &hng64_16x16x8_tilelayout, 0x0, 0x10 },
	{ REGION_GFX2, 0, &hng64_4_16_layout, 0x0, 0x100 },
	{ REGION_GFX2, 0, &hng64_16_layout, 0x0, 0x10 },
	{ -1 }
};

static DRIVER_INIT( hng64 )
{
	UINT8 *tiles = memory_region(REGION_GFX1);
	UINT32 size = memory_region_length(REGION_GFX1);
	UINT8 *buffer = malloc(size);
	UINT32 i;

	if (!buffer)
		fatalerror("Unable to allocate scroll tile buffer");

	/* Pair the two 32-byte planes before the graphics decoder runs. */
	for (i = 0; i < size / 2; i += 32)
	{
		memcpy(buffer + i * 2, tiles + i + size / 2, 32);
		memcpy(buffer + i * 2 + 32, tiles + i, 32);
	}
	memcpy(tiles, buffer, size);
	free(buffer);

	hng64_com_timer = timer_alloc(hng64_com_ready);
	hng64_roadedge_3d = !strcmp(Machine->gamedrv->name, "roadedge");
	hng64_samurai_3d = hng64_samurai_io;
	hng64_rtc_epoch = time(NULL);
	hng64_rtc_ctrl[2] = 4;
	hng64_soundram = auto_malloc(0x200000);
	memset(hng64_soundram, 0, 0x200000);
	pic8259_init(1, hng64_sound_pic_irq);
	pit8253_init(1, &hng64_sound_pit);
	hng64_com_virtual_mem = auto_malloc(0x100000);
	hng64_com_op_base = auto_malloc(0x10000);
}

static NVRAM_HANDLER( hng64 )
{
	if (read_or_write)
		mame_fwrite(file, hng64_sram, 0x4000);
	else if (file)
		mame_fread(file, hng64_sram, 0x4000);
	else if (memory_region(REGION_USER7))
	{
		const UINT8 *data = memory_region(REGION_USER7);
		int i;

		/* The defaults are byte dumps of the big-endian SRAM. */
		for (i = 0; i < 0x4000 / 4; ++i)
			hng64_sram[i] = ((UINT32)data[4*i] << 24) | ((UINT32)data[4*i+1] << 16)
				| ((UINT32)data[4*i+2] << 8) | data[4*i+3];
	}
	else
		memset(hng64_sram, 0, 0x4000);
}

static DRIVER_INIT(hng64_shoot)
{
	no_machine_error_code = 0x03000000;
	init_hng64();
}

static DRIVER_INIT(hng64_fght)
{
	no_machine_error_code=0x01000000;
	init_hng64();
}

static DRIVER_INIT(hng64_samurai)
{
	hng64_samurai_io = 1;
	init_hng64_fght();
}

static DRIVER_INIT(hng64_race)
{
	no_machine_error_code=0x02000000;
	init_hng64();
}

static struct mips3_config config =
{
	16384,
	16384
};

static void network_irq(int param)
{
	hng64_set_irq(0x0800);
}

static INTERRUPT_GEN( irq_start )
{
	if (hng64_io_test == 2 && hng64_mcu_mode != 0x0c && no_machine_error_code != 0x03000000)
	{
		UINT32 coin = readinputportbytag("SYSTEM") & 0x00030000;
		UINT32 pressed = coin & ~hng64_prev_coin;
		hng64_prev_coin = coin;
		if (pressed)
		{
			hng64_dualport[0x400 / 4] = 0x00000103;
			hng64_dualport[0x404 / 4] = pressed;
			hng64_set_irq(0x00020000);
		}
	}
	hng64_set_irq(0x0001);
	timer_set(cpu_getscanlinetime(480), 0, network_irq);
}

static MACHINE_RESET(hyperneo)
{
	hng64_sound_reset();

	KL5C80_init();

	memcpy(hng64_com_virtual_mem, memory_region(REGION_USER2), 0x20000);
	memcpy(hng64_com_virtual_mem + 0x20000, memory_region(REGION_USER2), 0x20000);
	memset(hng64_com_virtual_mem + 0x40000, 0, 0xc0000);

	KL5C80_virtual_mem_sync();
	memory_set_opbase_handler(2, KL5C80_opbase_handler);

	cpunum_set_input_line(2, INPUT_LINE_RESET, PULSE_LINE);

	hng64_video_reset();
	memset(hng64_texture_wrap, 8, sizeof(hng64_texture_wrap));
	hng64_irq_pending = 0;
	hng64_mcu_mode = 0;
	hng64_io_test = 0;
	hng64_prev_coin = 0;
	hng64_io_command_time = timer_get_time();
	memset(hng64_com_status, 0, 8);
	memset(hng64_com_shared, 0, sizeof(hng64_com_shared));
	timer_adjust(hng64_com_timer, 8.0, 0, 0);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 0);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x00000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x00ffffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, hng_mainram);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 0);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 1);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x04000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x05ffffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, hng_cart);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 1);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 2);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x1fc00000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x1fc7ffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rombase);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 1);
}

static MACHINE_DRIVER_START( hng64 )

	MDRV_CPU_ADD(R4600BE, MASTER_CLOCK)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(hng_map, 0)
	MDRV_CPU_VBLANK_INT(irq_start,1)

	MDRV_CPU_ADD(V33,16000000)
	MDRV_CPU_PROGRAM_MAP(hng_sound_map,0)
	MDRV_CPU_IO_MAP(hng_sound_io_map,0)

	MDRV_CPU_ADD(Z80,MASTER_CLOCK/4)
	MDRV_CPU_PROGRAM_MAP(hng_comm_map,0)
	MDRV_CPU_IO_MAP(hng_comm_io_map, 0)

	MDRV_FRAMES_PER_SECOND(59.430077)
	MDRV_VBLANK_DURATION(2549)

	MDRV_GFXDECODE(hng64_gfxdecode)
	MDRV_MACHINE_RESET(hyperneo)
	MDRV_NVRAM_HANDLER(hng64)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)

	MDRV_SCREEN_SIZE(768, 528)
	MDRV_VISIBLE_AREA(0, 511, 0, 447)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(hng64)
	MDRV_VIDEO_UPDATE(hng64)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 33868800)
	MDRV_SOUND_CONFIG(hng64_dsp)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
	MDRV_SOUND_ROUTE(8, "left", 0.5)
	MDRV_SOUND_ROUTE(8, "right", 0.5)
	MDRV_SOUND_ROUTE(9, "left", 0.5)
	MDRV_SOUND_ROUTE(9, "right", 0.5)
MACHINE_DRIVER_END

SYSTEM_BIOS_START( hng64 )
	SYSTEM_BIOS_ADD( 0, "export", "Export" )
	SYSTEM_BIOS_ADD( 1, "japan", "Japan" )
	SYSTEM_BIOS_ADD( 2, "us", "USA" )
	SYSTEM_BIOS_ADD( 3, "korea", "Korea" )
SYSTEM_BIOS_END

#define ROM_LOAD_HNG64_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash,  ROM_BIOS((bios) + 1))

#define HNG64_BIOS \
	ROM_REGION32_BE( 0x0100000, REGION_USER1, 0 ) \
	ROM_LOAD_HNG64_BIOS( 1, "brom1.bin",         0x00000, 0x080000, CRC(a30dd3de) SHA1(3e2fd0a56214e6f5dcb93687e409af13d065ea30) ) \
	ROM_LOAD_HNG64_BIOS( 2, "bios_us.bin",       0x00000, 0x080000,  CRC(ab5948d6) SHA1(f8b940c1ae5ce2d3b2cd0c9bfaf6e5b063cec06e) ) \
	ROM_LOAD_HNG64_BIOS( 0, "bios_export.bin",   0x00000, 0x080000, CRC(bbf07ec6) SHA1(5656aa077f6a6d43953f15b5123eea102a9d5313) ) \
	ROM_LOAD_HNG64_BIOS( 3, "bios_korea.bin",    0x00000, 0x080000, CRC(ac953e2e) SHA1(f502188ef252b7c9d04934c4b525730a116de48b) ) \
	ROM_REGION( 0x080000, REGION_USER2, 0 ) \
	ROM_LOAD ( "from1.bin", 0x000000, 0x080000,  CRC(6b933005) SHA1(e992747f46c48b66e5509fe0adf19c91250b00c7) ) \
	ROM_REGION( 0x0100000, REGION_USER4, 0 )   \
	ROM_LOAD ( "rom1.bin",  0x000000, 0x01ff32,  CRC(4a6832dc) SHA1(ae504f7733c2f40450157cd1d3b85bc83fac8569) ) \
	ROM_REGION( 0x10000, REGION_USER5, 0 )   \
	ROM_LOAD ( "tmp87ph40an.bin",  0x8000, 0x8000,  CRC(b70df21f) SHA1(5b742e8a0bbf4c0ae4f4398d34c7058fb24acc92) ) \
	ROM_REGION( 0x100, REGION_USER6, 0 )   \
	ROM_LOAD( "lvs-ioj-br9020f.u2", 0x000, 0x100, CRC(78b7020d) SHA1(2b8549532ef5e1e8102dbe71af55fdfb27ccbba6) ) \
	ROM_LOAD( "lvs-igx-br9020f.u3", 0x000, 0x100, CRC(af9f4287) SHA1(6df0e35c77dbfee2fab7ff490dcd651db420e367) ) \
	ROM_LOAD( "lvs-jam-br9020f.u3", 0x000, 0x100, CRC(dabec5d2) SHA1(19c5be89c57387d6ea563b3dc55674d0692af98e) ) \

ROM_START( hng64 )

	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_ERASEFF )
	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_ERASEFF )
	ROM_REGION( 0x1000000, REGION_GFX3, ROMREGION_ERASEFF )
	ROM_REGION( 0x1000000, REGION_GFX5, ROMREGION_ERASEFF )
	ROM_REGION( 0x1000000, REGION_GFX6, ROMREGION_ERASEFF )
	ROM_REGION( 0x1000000, REGION_GFX7, ROMREGION_ERASEFF )
	ROM_REGION16_LE( 0x0c00000, REGION_GFX4, ROMREGION_ERASEFF )
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF )
ROM_END

ROM_START( roadedge )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "001pr01b.81", 0x0000002, 0x400000, CRC(effbac30) SHA1(c1bddf3e511a8950f65ac7e452f81dbc4b7fd977) )
	ROM_LOAD32_WORD_SWAP( "001pr02b.82", 0x0000000, 0x400000, CRC(b9aa4ad3) SHA1(9ab3c896dbdc45560b7127486e2db6ca3b15a057) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "001sc01a.41", 0x0000000, 0x400000, CRC(084395a1) SHA1(8bfea8fd3981fd45dcc04bd74840a5948aaf06a8) )
	ROM_LOAD16_BYTE( "001sc02a.42", 0x0000001, 0x400000, CRC(51dd19e3) SHA1(eeb3634294a049a357a75ee00aa9fce65b737395) )
	ROM_LOAD16_BYTE( "001sc03a.43", 0x0800000, 0x400000, CRC(0b6f3e19) SHA1(3b6dfd0f0633b0d8b629815920edfa39d92336bf) )
	ROM_LOAD16_BYTE( "001sc04a.44", 0x0800001, 0x400000, CRC(256c8c1c) SHA1(85935eea3722ec92f8d922f527c2e049c4185aa3) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "001sp01a.53",0x0000000, 0x400000, CRC(7a469453) SHA1(3738ca76f538243bb23ffd23a42b2a0558882889) )
	ROM_LOAD32_BYTE( "001sp02a.54",0x0000001, 0x400000, CRC(6b9a3de0) SHA1(464c652f7b193326e3a871dfe751dd83c14284eb) )
	ROM_LOAD32_BYTE( "001sp03a.55",0x0000002, 0x400000, CRC(efbbd391) SHA1(7447c481ba6f9ba154d48a4b160dd24157891d35) )
	ROM_LOAD32_BYTE( "001sp04a.56",0x0000003, 0x400000, CRC(1a0eb173) SHA1(a69b786a9957197d1cc950ab046c57c18ca07ea7) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "001tx01a.1", 0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.2", 0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.3", 0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.4", 0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "001tx01a.5", 0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.6", 0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.7", 0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.8", 0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "001tx01a.9", 0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.10",0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.11",0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.12",0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "001tx01a.13",0x0000000, 0x400000, CRC(f6539bb9) SHA1(57fc5583d56846be93d6f5784acd20fc149c70a5) )
	ROM_LOAD( "001tx02a.14",0x0400000, 0x400000, CRC(f1d139d3) SHA1(f120243f4d55f38b10bf8d1aa861cdc546a24c80) )
	ROM_LOAD( "001tx03a.15",0x0800000, 0x400000, CRC(22a375bd) SHA1(d55b62843d952930db110bcf3056a98a04a7adf4) )
	ROM_LOAD( "001tx04a.16",0x0c00000, 0x400000, CRC(288a5bd5) SHA1(24e05db681894eb31cdc049cf42c1f9d7347bd0c) )

	ROM_REGION16_LE( 0x0c00000, REGION_GFX4, 0 )
	ROMX_LOAD( "001vt01a.17", 0x0000000, 0x400000, CRC(1a748e1b) SHA1(376d40baa3b94890d4740045d053faf208fe43db), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "001vt02a.18", 0x0000002, 0x400000, CRC(449f94d0) SHA1(2228690532d82d2661285aeb4260689b027597cb), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "001vt03a.19", 0x0000004, 0x400000, CRC(50ac8639) SHA1(dd2d3689466990a7c479bb8f11bd930ea45e47b5), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "001sd01a.77", 0x0000000, 0x400000, CRC(a851da99) SHA1(2ba24feddafc5fadec155cdb7af305fdffcf6690) )
	ROM_LOAD( "001sd02a.78", 0x0400000, 0x400000, CRC(ca5cec15) SHA1(05e91a602728a048d61bf86aa8d43bb4186aeac1) )
ROM_END

ROM_START( sams64 )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "002-pro1a.81", 0x0000002, 0x400000, CRC(e5b907c5) SHA1(83637ffaa9031d41a5bed3397a519d1dfa8052cb) )
	ROM_LOAD32_WORD_SWAP( "002-pro2a.82", 0x0000000, 0x400000, CRC(803ed2eb) SHA1(666db47886a316e68b911311e5db3bc0f5b8a34d) )
	ROM_LOAD32_WORD_SWAP( "002-pro3a.83", 0x0800002, 0x400000, CRC(582156a7) SHA1(a7bbbd472a53072cbfaed5d41d4265123c9e3f3d) )
	ROM_LOAD32_WORD_SWAP( "002-pro4a.84", 0x0800000, 0x400000, CRC(5a8291e9) SHA1(ec1e5a5a0ba37393e8b93d78b4ac855109d45ec9) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "002-sc01a.41", 0x0000000, 0x400000, CRC(77c3df69) SHA1(813d57814acccd2c04c951e58ac87cf7413bdf58) )
	ROM_LOAD16_BYTE( "002-sc02a.42", 0x0000001, 0x400000, CRC(60065174) SHA1(624c2e20abb53b2466df4ce2ffa9e20273798e92) )
	ROM_LOAD16_BYTE( "002-sc05a.45", 0x0800000, 0x400000, CRC(fd242bee) SHA1(b1fad97987da21c77d6c460bbed6f0dd18905ed4) )
	ROM_LOAD16_BYTE( "002-sc06a.46", 0x0800001, 0x400000, CRC(87afc297) SHA1(47d5eaae88ce501fbbd5a2d7305c1d6acadfb13e) )
	ROM_LOAD16_BYTE( "002-sc03a.43", 0x1000000, 0x400000, CRC(5d4a5289) SHA1(7a1576fdd344825cb05866c156d17b18f562a336) )
	ROM_LOAD16_BYTE( "002-sc04a.44", 0x1000001, 0x400000, CRC(aa5536fa) SHA1(09a50a29561ac97c564243da879bd7c4cf8c3cee) )
	ROM_LOAD16_BYTE( "002-sc07a.47", 0x1800000, 0x400000, CRC(e01e8a95) SHA1(b132214ef2b33a46cb605ea8f2193e77d9464881) )
	ROM_LOAD16_BYTE( "002-sc08a.48", 0x1800001, 0x400000, CRC(a17464d0) SHA1(2e6b73b1e0983b2b01455b0f4d6dc7c3845adb69) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "002-sp01a.53",0x0000000, 0x400000, CRC(c73cf9b4) SHA1(7c34fa1bc03cd366d473dbf3e316a6434ee5ec60) )
	ROM_LOAD32_BYTE( "002-sp02a.54",0x0000001, 0x400000, CRC(04b0ecc8) SHA1(893e522324dd41dfcd2217974a6740e6bc3ea1d3) )
	ROM_LOAD32_BYTE( "002-sp03a.55",0x0000002, 0x400000, CRC(13c80b74) SHA1(ad6c1690ebcde0d8237201ea43eb162cd5308ccb) )
	ROM_LOAD32_BYTE( "002-sp04a.56",0x0000003, 0x400000, CRC(b1a6a06d) SHA1(1b11ee7cec46d0c99dc6310ee8221fa2de33c359) )
	ROM_LOAD32_BYTE( "002-sp05a.57",0x1000000, 0x400000, CRC(fa71e825) SHA1(adfa8b5a8ec703d4f04285c47f2618a294c90ec5) )
	ROM_LOAD32_BYTE( "002-sp06a.58",0x1000001, 0x400000, CRC(1bcfe48e) SHA1(8d85b1eb33fea48e5c6597d2fcbec903ecdad9d9) )
	ROM_LOAD32_BYTE( "002-sp07a.59",0x1000002, 0x400000, CRC(a5049bd7) SHA1(123e32c22f53d6e55ee1d1deb4ab40891004c6fd) )
	ROM_LOAD32_BYTE( "002-sp08a.60",0x1000003, 0x400000, CRC(c2e57813) SHA1(e7a21df1f94ed959a53da9dc4667863ee77bf676) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "002-tx01a.1", 0x0000000, 0x400000, CRC(233749b5) SHA1(7c93681bbd5f4246e0dc50d26108f04e9b248d0d) )
	ROM_LOAD( "002-tx02a.2", 0x0400000, 0x400000, CRC(d5074be2) SHA1(c33e9b9f0d21ad5ad31d8f988b3c7378d374fc1b) )
	ROM_LOAD( "002-tx03a.3", 0x0800000, 0x400000, CRC(68c313f7) SHA1(90ce8d0d19a994647c7167e3b256ff31647e575a) )
	ROM_LOAD( "002-tx04a.4", 0x0c00000, 0x400000, CRC(f7dac24f) SHA1(1215354f28cbeb9fc38f6a7acae450ad5f34bb6a) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "002-tx01a.5", 0x0000000, 0x400000, CRC(233749b5) SHA1(7c93681bbd5f4246e0dc50d26108f04e9b248d0d) )
	ROM_LOAD( "002-tx02a.6", 0x0400000, 0x400000, CRC(d5074be2) SHA1(c33e9b9f0d21ad5ad31d8f988b3c7378d374fc1b) )
	ROM_LOAD( "002-tx03a.7", 0x0800000, 0x400000, CRC(68c313f7) SHA1(90ce8d0d19a994647c7167e3b256ff31647e575a) )
	ROM_LOAD( "002-tx04a.8", 0x0c00000, 0x400000, CRC(f7dac24f) SHA1(1215354f28cbeb9fc38f6a7acae450ad5f34bb6a) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "002-tx01a.9",  0x0000000, 0x400000, CRC(233749b5) SHA1(7c93681bbd5f4246e0dc50d26108f04e9b248d0d) )
	ROM_LOAD( "002-tx02a.10", 0x0400000, 0x400000, CRC(d5074be2) SHA1(c33e9b9f0d21ad5ad31d8f988b3c7378d374fc1b) )
	ROM_LOAD( "002-tx03a.11", 0x0800000, 0x400000, CRC(68c313f7) SHA1(90ce8d0d19a994647c7167e3b256ff31647e575a) )
	ROM_LOAD( "002-tx04a.12", 0x0c00000, 0x400000, CRC(f7dac24f) SHA1(1215354f28cbeb9fc38f6a7acae450ad5f34bb6a) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "002-tx01a.13", 0x0000000, 0x400000, CRC(233749b5) SHA1(7c93681bbd5f4246e0dc50d26108f04e9b248d0d) )
	ROM_LOAD( "002-tx02a.14", 0x0400000, 0x400000, CRC(d5074be2) SHA1(c33e9b9f0d21ad5ad31d8f988b3c7378d374fc1b) )
	ROM_LOAD( "002-tx03a.15", 0x0800000, 0x400000, CRC(68c313f7) SHA1(90ce8d0d19a994647c7167e3b256ff31647e575a) )
	ROM_LOAD( "002-tx04a.16", 0x0c00000, 0x400000, CRC(f7dac24f) SHA1(1215354f28cbeb9fc38f6a7acae450ad5f34bb6a) )

	ROM_REGION16_LE( 0x1800000, REGION_GFX4, 0 )
	ROMX_LOAD( "002-vt01a.17", 0x0000000, 0x400000, CRC(403fd7fd) SHA1(9bdadbeb4cd13c4c4e89a1c233af9eaaa46f8fdf), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "002-vt02a.18", 0x0000002, 0x400000, CRC(e1885905) SHA1(6b16083c50e887aebe2baf95bf56697c239970f2), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "002-vt03a.19", 0x0000004, 0x400000, CRC(2074a6a6) SHA1(9a5e8259d1e19d2b43878c24ca06afba5ee5e316), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "002-vt04a.20", 0x0c00000, 0x400000, CRC(aefc4d94) SHA1(f9d8222d4320ccf9f3c7c0ef307e03c8f34ea530), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "002-vt05a.21", 0x0c00002, 0x400000, CRC(d32ee9cb) SHA1(a768dfc15899924eb05eccbf8e85cb29c7b60396), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "002-vt06a.22", 0x0c00004, 0x400000, CRC(13bf3636) SHA1(7c704bf66b571350207bccc7a2d6ed1ec9de4cd5), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "002-sd01a.77", 0x0000000, 0x400000, CRC(6215036b) SHA1(ded71dce98b7f7ef78ef32d966a292bbf0d15332) )
	ROM_LOAD( "002-sd02a.78", 0x0400000, 0x400000, CRC(32b28310) SHA1(5b80750a66c12b035b493d06e3842741a3334d0f) )
	ROM_LOAD( "002-sd03a.79", 0x0800000, 0x400000, CRC(53591413) SHA1(36c7efa1aced0ca38b3ce7b95af28755973698f3) )
ROM_END

ROM_START( xrally )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "003-pr01a.81", 0x0000002, 0x400000, CRC(4e160388) SHA1(08fba66d0f0dab47f7db5bc7d411f4fc0e8219c8) )
	ROM_LOAD32_WORD_SWAP( "003-pr02a.82", 0x0000000, 0x400000, CRC(c4dd4f18) SHA1(4db0e6d5cabd9e4f82d5905556174b9eff8ad4d9) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "003-sc01a.41", 0x0000000, 0x400000, CRC(bc608584) SHA1(fa4b618eb36f302f58cefea7c50618a8318927d6) )
	ROM_LOAD16_BYTE( "003-sc02a.42", 0x0000001, 0x400000, CRC(c810e9e2) SHA1(4f0d35d9b0af2a4b66253e467c0d30a519c904b6) )
	ROM_LOAD16_BYTE( "003-sc03a.43", 0x0800000, 0x400000, CRC(12724653) SHA1(5e40947086883d64db84ac51a1b29efa2f173f58) )
	ROM_LOAD16_BYTE( "003-sc04a.44", 0x0800001, 0x400000, CRC(b0062c4d) SHA1(73c75b59dc1463ad80f805191f4605a6b4b1c321) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "003-sp01a.53",0x0000000, 0x400000, CRC(12a329dc) SHA1(00929f3c460cce5a3657dec73d467731e59de564) )
	ROM_LOAD32_BYTE( "003-sp02a.54",0x0000001, 0x400000, CRC(ee9e5338) SHA1(681c2f34a2f292ce14fcbef4447ede7b949c7117) )
	ROM_LOAD32_BYTE( "003-sp03a.55",0x0000002, 0x400000, CRC(6fa8dff9) SHA1(500bd128e6568e9491e52676775e9239adc332fe) )
	ROM_LOAD32_BYTE( "003-sp04a.56",0x0000003, 0x400000, CRC(a98eec07) SHA1(de0c7db56b851daa369f37088bd536933372346f) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "003-tx01a.1", 0x0000000, 0x400000, CRC(83ea2178) SHA1(931898f57564b8b9975e06df5ccfd8c84fc2fbe3) )
	ROM_LOAD( "003-tx02a.2", 0x0400000, 0x400000, CRC(7912f4be) SHA1(bca44c1415a25f2349857b2246e3ee7abe709a84) )
	ROM_LOAD( "003-tx03a.3", 0x0800000, 0x400000, CRC(a319c94e) SHA1(14d720cdd8b9411fd82a7b4b33ee5dbfdd01c9f8) )
	ROM_LOAD( "003-tx04a.4", 0x0c00000, 0x400000, CRC(16d7805b) SHA1(4cc7b2375832c2f9f20fe882e604a2a52bf07f6f) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "003-tx01a.5", 0x0000000, 0x400000, CRC(83ea2178) SHA1(931898f57564b8b9975e06df5ccfd8c84fc2fbe3) )
	ROM_LOAD( "003-tx02a.6", 0x0400000, 0x400000, CRC(7912f4be) SHA1(bca44c1415a25f2349857b2246e3ee7abe709a84) )
	ROM_LOAD( "003-tx03a.7", 0x0800000, 0x400000, CRC(a319c94e) SHA1(14d720cdd8b9411fd82a7b4b33ee5dbfdd01c9f8) )
	ROM_LOAD( "003-tx04a.8", 0x0c00000, 0x400000, CRC(16d7805b) SHA1(4cc7b2375832c2f9f20fe882e604a2a52bf07f6f) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "003-tx01a.9",  0x0000000, 0x400000, CRC(83ea2178) SHA1(931898f57564b8b9975e06df5ccfd8c84fc2fbe3) )
	ROM_LOAD( "003-tx02a.10", 0x0400000, 0x400000, CRC(7912f4be) SHA1(bca44c1415a25f2349857b2246e3ee7abe709a84) )
	ROM_LOAD( "003-tx03a.11", 0x0800000, 0x400000, CRC(a319c94e) SHA1(14d720cdd8b9411fd82a7b4b33ee5dbfdd01c9f8) )
	ROM_LOAD( "003-tx04a.12", 0x0c00000, 0x400000, CRC(16d7805b) SHA1(4cc7b2375832c2f9f20fe882e604a2a52bf07f6f) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "003-tx01a.13", 0x0000000, 0x400000, CRC(83ea2178) SHA1(931898f57564b8b9975e06df5ccfd8c84fc2fbe3) )
	ROM_LOAD( "003-tx02a.14", 0x0400000, 0x400000, CRC(7912f4be) SHA1(bca44c1415a25f2349857b2246e3ee7abe709a84) )
	ROM_LOAD( "003-tx03a.15", 0x0800000, 0x400000, CRC(a319c94e) SHA1(14d720cdd8b9411fd82a7b4b33ee5dbfdd01c9f8) )
	ROM_LOAD( "003-tx04a.16", 0x0c00000, 0x400000, CRC(16d7805b) SHA1(4cc7b2375832c2f9f20fe882e604a2a52bf07f6f) )

	ROM_REGION16_LE( 0x0c00000, REGION_GFX4, 0 )
	ROMX_LOAD( "003-vt01a.17", 0x0000000, 0x400000, CRC(3e5e275d) SHA1(74f5ec88c258bc224e271f7abeb02d6485e27d8c), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "003-vt02a.18", 0x0000002, 0x400000, CRC(da7b956e) SHA1(c57cbb8c51145ae224faba5b6a1a7e61cb2bee64), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "003-vt03a.19", 0x0000004, 0x400000, CRC(4fe72cb7) SHA1(9f8e662f0656f201924834d1ee78498d4223745e), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "003-sd01a.77", 0x0000000, 0x400000, CRC(c43898ff) SHA1(0e49b87181b56c62a674d255d326f761942b99b1) )
	ROM_LOAD( "003-sd02a.78", 0x0400000, 0x400000, CRC(079a3d5a) SHA1(a97b052de69fee7d605cae30f5a228e6ffeabb26) )
	ROM_LOAD( "003-sd03a.79", 0x0800000, 0x400000, CRC(96c0991a) SHA1(01be872b3e307258236fe96a544417dd8a0bc8bd) )
ROM_END

ROM_START( bbust2 )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "004-pr01a.81", 0x0000002, 0x400000, CRC(7b836ece) SHA1(7a4a08251f1dd66c368ac203f5a006266e77f73d) )
	ROM_LOAD32_WORD_SWAP( "004-pr02a.82", 0x0000000, 0x400000, CRC(8c55a988) SHA1(d9a61ac3d8550ce0ee6aab374c9f024912163180) )
	ROM_LOAD32_WORD_SWAP( "004-pr03a.83", 0x0800002, 0x400000, CRC(f25a82dd) SHA1(74c0a03021ef424e0b9c3c818be297d2967b3012) )
	ROM_LOAD32_WORD_SWAP( "004-pr04a.84", 0x0800000, 0x400000, CRC(9258312b) SHA1(fabac42c8a033e85d503be56f266f9386adff10b) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "004-sc01a.41", 0x0000000, 0x400000, CRC(0b52987e) SHA1(3c7b0ce9416dea8db4cf63431166fcfa7c3bb168) )
	ROM_LOAD16_BYTE( "004-sc02a.42", 0x0000001, 0x400000, CRC(6b55309d) SHA1(87761deed6d842075bbe13abc444ac502274eeba) )
	ROM_LOAD16_BYTE( "004-sc03a.43", 0x0800000, 0x400000, CRC(17302f01) SHA1(5b6a927c520e421aa31b9162d3e47b06069b4bd0) )
	ROM_LOAD16_BYTE( "004-sc04a.44", 0x0800001, 0x400000, CRC(db31d73c) SHA1(8a6847e367e87a081cd1499294935c45f1fb4794) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "004-sp01a.53",0x0000000, 0x400000, CRC(72fe73c3) SHA1(82825705076c40558d414653386e3bf1d0693008) )
	ROM_LOAD32_BYTE( "004-sp02a.54",0x0000001, 0x400000, CRC(1ece1cff) SHA1(78d88e96df979a834b5af091d3feda8b9cd466e0) )
	ROM_LOAD32_BYTE( "004-sp03a.55",0x0000002, 0x400000, CRC(9049ab14) SHA1(0a19ccbd82f000eba19a0b407fa5765db0464cca) )
	ROM_LOAD32_BYTE( "004-sp04a.56",0x0000003, 0x400000, CRC(8f7fb914) SHA1(dd1709881bf1d9e233b4e794c0e2ce28d265f855) )
	ROM_LOAD32_BYTE( "004-sp05a.57",0x1000000, 0x400000, CRC(440ce760) SHA1(f6f256334c32fe7d25448fba73f8966c4c5b1cba) )
	ROM_LOAD32_BYTE( "004-sp06a.58",0x1000001, 0x400000, CRC(fc24d2e5) SHA1(073dcb21ec6cf9c6a81987a54c0e27a2db499341) )
	ROM_LOAD32_BYTE( "004-sp07a.59",0x1000002, 0x400000, CRC(bc580b81) SHA1(c668d0524fdc53c6ba2f3e5120f2dee7ce4279bb) )
	ROM_LOAD32_BYTE( "004-sp08a.60",0x1000003, 0x400000, CRC(d6c69bea) SHA1(24508c0ed0ca135316aec1c8239e8b755070384a) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "004-tx01a.1", 0x0000000, 0x400000, CRC(12a78a20) SHA1(a5c1c8841cd0cb5efbf7408d908fa10a743e5c6f) )
	ROM_LOAD( "004-tx02a.2", 0x0400000, 0x400000, CRC(a36c6c34) SHA1(3e4ad293b064a7c05aa23447ff5f17010cae2863) )
	ROM_LOAD( "004-tx03a.3", 0x0800000, 0x400000, CRC(f46377c0) SHA1(bfa6fc3ab89599a4443577d18578569ad55774bd) )
	ROM_LOAD( "004-tx04a.4", 0x0c00000, 0x400000, CRC(b5f0ef01) SHA1(646bfb17b9e81aecf8db33d3a021f7769b262eda) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "004-tx01a.5", 0x0000000, 0x400000, CRC(12a78a20) SHA1(a5c1c8841cd0cb5efbf7408d908fa10a743e5c6f) )
	ROM_LOAD( "004-tx02a.6", 0x0400000, 0x400000, CRC(a36c6c34) SHA1(3e4ad293b064a7c05aa23447ff5f17010cae2863) )
	ROM_LOAD( "004-tx03a.7", 0x0800000, 0x400000, CRC(f46377c0) SHA1(bfa6fc3ab89599a4443577d18578569ad55774bd) )
	ROM_LOAD( "004-tx04a.8", 0x0c00000, 0x400000, CRC(b5f0ef01) SHA1(646bfb17b9e81aecf8db33d3a021f7769b262eda) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "004-tx01a.9" , 0x0000000, 0x400000, CRC(12a78a20) SHA1(a5c1c8841cd0cb5efbf7408d908fa10a743e5c6f) )
	ROM_LOAD( "004-tx02a.10", 0x0400000, 0x400000, CRC(a36c6c34) SHA1(3e4ad293b064a7c05aa23447ff5f17010cae2863) )
	ROM_LOAD( "004-tx03a.11", 0x0800000, 0x400000, CRC(f46377c0) SHA1(bfa6fc3ab89599a4443577d18578569ad55774bd) )
	ROM_LOAD( "004-tx04a.12", 0x0c00000, 0x400000, CRC(b5f0ef01) SHA1(646bfb17b9e81aecf8db33d3a021f7769b262eda) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "004-tx01a.13", 0x0000000, 0x400000, CRC(12a78a20) SHA1(a5c1c8841cd0cb5efbf7408d908fa10a743e5c6f) )
	ROM_LOAD( "004-tx02a.14", 0x0400000, 0x400000, CRC(a36c6c34) SHA1(3e4ad293b064a7c05aa23447ff5f17010cae2863) )
	ROM_LOAD( "004-tx03a.15", 0x0800000, 0x400000, CRC(f46377c0) SHA1(bfa6fc3ab89599a4443577d18578569ad55774bd) )
	ROM_LOAD( "004-tx04a.16", 0x0c00000, 0x400000, CRC(b5f0ef01) SHA1(646bfb17b9e81aecf8db33d3a021f7769b262eda) )

	ROM_REGION16_LE( 0x0c00000, REGION_GFX4, 0 )
	ROMX_LOAD( "004-vt01a.17", 0x0000000, 0x400000, CRC(25ebbf9b) SHA1(b7c3fb9ee9cf75824d908e7a94970282f1845d5d), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "004-vt02a.18", 0x0000002, 0x400000, CRC(279fc216) SHA1(eb90cc347745491c1d1b1fb611fd6e227310731c), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "004-vt03a.19", 0x0000004, 0x400000, CRC(e0cf6a42) SHA1(dd09b3d05739cf030c820cd7dbaea2e7262764ab), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "004-sd01a.77", 0x0000000, 0x400000, CRC(2ef868bd) SHA1(0a1ef002efe6738698ebe98a1c3695b151fdd282) )
	ROM_LOAD( "004-sd02a.78", 0x0400000, 0x400000, CRC(07fb3135) SHA1(56cc8e29ba9b13f82a4c9248bff02e2b7a0c49b0) )
	ROM_LOAD( "004-sd03a.79", 0x0800000, 0x400000, CRC(42571f1d) SHA1(425cbd3f7c8aea1c0f057ea8f186acffb0091dc0) )
ROM_END

ROM_START( sams64_2 )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "005pr01a.81", 0x0000002, 0x400000, CRC(a69d7700) SHA1(a580783a109bc3e24248d70bcd67f62dd7d8a5dd) )
	ROM_LOAD32_WORD_SWAP( "005pr02a.82", 0x0000000, 0x400000, CRC(38b9e6b3) SHA1(d1dad8247d920cc66854a0096e1c7845842d2e1c) )
	ROM_LOAD32_WORD_SWAP( "005pr03a.83", 0x0800002, 0x400000, CRC(0bc738a8) SHA1(79893b0e1c4a31e02ab385c4382684245975ae8f) )
	ROM_LOAD32_WORD_SWAP( "005pr04a.84", 0x0800000, 0x400000, CRC(6b504852) SHA1(fcdcab432162542d249818a6cd15b8f2e8230f97) )
	ROM_LOAD32_WORD_SWAP( "005pr05a.85", 0x1000002, 0x400000, CRC(32a743d3) SHA1(4088b930a1a4d6224a0939ef3942af1bf605cdb5) )
	ROM_LOAD32_WORD_SWAP( "005pr06a.86", 0x1000000, 0x400000, CRC(c09fa615) SHA1(697d6769c16b3c8f73a6df4a1e268ec40cb30d51) )
	ROM_LOAD32_WORD_SWAP( "005pr07a.87", 0x1800002, 0x400000, CRC(44286ad3) SHA1(1f890c74c0da0d34940a880468e68f7fb1417813) )
	ROM_LOAD32_WORD_SWAP( "005pr08a.88", 0x1800000, 0x400000, CRC(d094eb67) SHA1(3edc8d608c631a05223e1d05157cd3daf2d6597a) )

	ROM_REGION( 0x4000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "005sc01a.97",  0x0000000, 0x800000, CRC(7f11cda9) SHA1(5fbdabd8423e9723a6ec38f8503e6ca7f4f69fdd) )
	ROM_LOAD16_BYTE( "005sc02a.99",  0x0000001, 0x800000, CRC(87d1e1a7) SHA1(00f2ef46ce64ab715add8cd47745c57944286f81) )
	ROM_LOAD16_BYTE( "005sc05a.98",  0x1000000, 0x800000, CRC(4475a3f8) SHA1(f099baf766ee00d166cfa8402baa0b6ea25a0010) )
	ROM_LOAD16_BYTE( "005sc06a.100", 0x1000001, 0x800000, CRC(41c0fbbd) SHA1(1d9ac01c9499a6202ee59d15d498ec34edc05888) )
	ROM_LOAD16_BYTE( "005sc03a.101", 0x2000000, 0x800000, CRC(a5d4c535) SHA1(089a3cd07701f025024ce73b7b4d38063c33a59f) )
	ROM_LOAD16_BYTE( "005sc04a.103", 0x2000001, 0x800000, CRC(14930d77) SHA1(b4c613a8896e21fe2cac0595dd1ea30dc7fce0bd) )
	ROM_LOAD16_BYTE( "005sc07a.102", 0x3000000, 0x800000, CRC(3505b198) SHA1(2fdfdd5a1f6f31f5fb1c0af70047108d1df44af2) )
	ROM_LOAD16_BYTE( "005sc08a.104", 0x3000001, 0x800000, CRC(3139e413) SHA1(38210541379ddeba8c0b9ef8fa5430c0090db7c7) )

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "005sp01a.105",0x0000000, 0x800000, CRC(68eefee5) SHA1(d95bd7b549900500633af07544423b0062ac07ce) )
	ROM_LOAD32_BYTE( "005sp02a.109",0x0000001, 0x800000, CRC(5d9a49b9) SHA1(50768c496a3e0b4379e121349f32edec4f18652f) )
	ROM_LOAD32_BYTE( "005sp03a.113",0x0000002, 0x800000, CRC(9b6530fe) SHA1(398433b98578a6b4b950afc4d6318916376e0760) )
	ROM_LOAD32_BYTE( "005sp04a.117",0x0000003, 0x800000, CRC(d4e422ce) SHA1(9bfaa533ab3d014cdb0c535cf6952e01925cc30b) )
	ROM_LOAD32_BYTE( "005sp05a.106",0x2000000, 0x400000, CRC(d8b1fb26) SHA1(7da767d8e817c52afc416ccfe8caf30f66c233ef) )
	ROM_LOAD32_BYTE( "005sp06a.110",0x2000001, 0x400000, CRC(87ed72a0) SHA1(0d7db4dc9f15a0377a83f020ffbe81621ca77cff) )
	ROM_LOAD32_BYTE( "005sp07a.114",0x2000002, 0x400000, CRC(8eb3c173) SHA1(d5763c19a3e2fd93f7784d957e7401c9152c40de) )
	ROM_LOAD32_BYTE( "005sp08a.118",0x2000003, 0x400000, CRC(05486fbc) SHA1(747d9ae03ce999be4ab697753e93c90ea85b7d44) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "005tx01a.1", 0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.2", 0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.3", 0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.4", 0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "005tx01a.5", 0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.6", 0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.7", 0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.8", 0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "005tx01a.9", 0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.10",0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.11",0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.12",0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "005tx01a.13",0x0000000, 0x400000, CRC(05a4ceb7) SHA1(2dfc46a70c0a957ed0931a4c4df90c341aafff70) )
	ROM_LOAD( "005tx02a.14",0x0400000, 0x400000, CRC(b7094c69) SHA1(aed9a624166f6f1a2eb4e746c61f9f46f1929283) )
	ROM_LOAD( "005tx03a.15",0x0800000, 0x400000, CRC(34764891) SHA1(cd6ea663ae28b7f6ac1ede2f9922afbb35b915b4) )
	ROM_LOAD( "005tx04a.16",0x0c00000, 0x400000, CRC(6be50882) SHA1(1f99717cfa69076b258a0c52d66be007fd820374) )

	ROM_REGION16_LE( 0x1800000, REGION_GFX4, 0 )
	ROMX_LOAD( "005vt01a.17", 0x0000000, 0x400000, CRC(48a61479) SHA1(ef982b1ecc6dfca2ad989391afcc1b3d1e7fe652), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "005vt02a.18", 0x0000002, 0x400000, CRC(ba9100c8) SHA1(f7704fb8e5310ea7d0e6ae6b8935717ec9119b6d), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "005vt03a.19", 0x0000004, 0x400000, CRC(f54a28de) SHA1(c445cf7fee71a516065cf37e05b898208f48b17e), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "005vt04a.20", 0x0c00000, 0x400000, CRC(57ad79c7) SHA1(bc382317323c1f8a31b69ae3100d3bba6b5d0838), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "005vt05a.21", 0x0c00002, 0x400000, CRC(49c82bec) SHA1(09255279edb9a204bbe1cce8cef58d5c81e86d1f), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "005vt06a.22", 0x0c00004, 0x400000, CRC(7ba05b6c) SHA1(729c1d182d74998dd904b587a2405f55af9825e0), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "005sd01a.77", 0x0000000, 0x400000, CRC(8f68150f) SHA1(a1e5efdfd1ed29f81e25c8da669851ddb7b0c826) )
	ROM_LOAD( "005sd02a.78", 0x0400000, 0x400000, CRC(6b4da6a0) SHA1(8606c413c129635bdaaa37254edbfd19b10426bb) )
	ROM_LOAD( "005sd03a.79", 0x0800000, 0x400000, CRC(a529fab3) SHA1(8559d402c8f66f638590b8b57ec9efa775010c96) )
	ROM_LOAD( "005sd04a.80", 0x0c00000, 0x400000, CRC(dca95ead) SHA1(39afdfba0e5262b524f25706a96be00e5d14548e) )
ROM_END

ROM_START( fatfurwa )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "006pr01a.81", 0x0000002, 0x400000, CRC(3830efa1) SHA1(9d8c941ccb6cbe8d138499cf9d335db4ac7a9ec0) )
	ROM_LOAD32_WORD_SWAP( "006pr02a.82", 0x0000000, 0x400000, CRC(8d5de84e) SHA1(e3ae014263f370c2836f62ab323f1560cb3a9cf0) )
	ROM_LOAD32_WORD_SWAP( "006pr03a.83", 0x0800002, 0x400000, CRC(c811b458) SHA1(7d94e0df501fb086b2e5cf08905d7a3adc2c6472) )
	ROM_LOAD32_WORD_SWAP( "006pr04a.84", 0x0800000, 0x400000, CRC(de708d6c) SHA1(2c9848e7bbf61c574370f9ecab5f5a6ba63339fd) )

	ROM_REGION( 0x4000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "006sc01a.97", 0x0000000, 0x800000, CRC(f13dffad) SHA1(86363aeae176fd4204e446c13a028da919dc2069) )
	ROM_LOAD16_BYTE( "006sc02a.99", 0x0000001, 0x800000, CRC(be79d42a) SHA1(f3eb950a62e2df1de116af9434027439f1305e1f) )
	ROM_LOAD16_BYTE( "006sc05a.98", 0x1000000, 0x800000, CRC(0487297b) SHA1(d3fa4d691559327739c96717312faf09b498001d) )
	ROM_LOAD16_BYTE( "006sc06a.100",0x1000001, 0x800000, CRC(34a76c31) SHA1(be05dc75afb7cde65ba5d29c0e66a7b1b62c41cb) )
	ROM_LOAD16_BYTE( "006sc03a.101",0x2000000, 0x800000, CRC(16918b73) SHA1(ad0c751a301fe3c95fca19473869dfd55fb6b0de) )
	ROM_LOAD16_BYTE( "006sc04a.103",0x2000001, 0x800000, CRC(9b63cd98) SHA1(62519a3a531c4493a5a85dc01ca69413977120ca) )
	ROM_LOAD16_BYTE( "006sc07a.102",0x3000000, 0x800000, CRC(7a1c371e) SHA1(1cd4ad66dd007adc9ab0c29720cbf9955c7337e0) )
	ROM_LOAD16_BYTE( "006sc08a.104",0x3000001, 0x800000, CRC(88232ade) SHA1(4ae2a572c3525087f77c95185e8697a1fc720512) )

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "006sp01a.105",0x0000000, 0x800000, CRC(087b8c49) SHA1(bb1eb2baef7da91f904bf45414f21dd6bac30749) )
	ROM_LOAD32_BYTE( "006sp02a.109",0x0000001, 0x800000, CRC(da28631e) SHA1(ea7e2d9195cfa4f954f4d542296eec1323223653) )
	ROM_LOAD32_BYTE( "006sp03a.113",0x0000002, 0x800000, CRC(bb87b55b) SHA1(8644ebb356ae158244a6e03254b0212cb359e167) )
	ROM_LOAD32_BYTE( "006sp04a.117",0x0000003, 0x800000, CRC(2367a536) SHA1(304b5b7f7e5d41e69fbd4ac2a938c42f3766630e) )
	ROM_LOAD32_BYTE( "006sp05a.106",0x2000000, 0x800000, CRC(0eb8fd06) SHA1(c2b6fab1b0104910d7bb39d0a496ada39c5cc122) )
	ROM_LOAD32_BYTE( "006sp06a.110",0x2000001, 0x800000, CRC(dccc3f75) SHA1(fef8d259c17a78e2266fed965fba1e15f1cd01dd) )
	ROM_LOAD32_BYTE( "006sp07a.114",0x2000002, 0x800000, CRC(cd7baa1b) SHA1(4084f3a73aae623d69bd9de87cecf4a33b628b7f) )
	ROM_LOAD32_BYTE( "006sp08a.118",0x2000003, 0x800000, CRC(9c3044ac) SHA1(24b28bcc6be51ab3ff59c2894094cd03ec377d84) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "006tx01a.1", 0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.2", 0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.3", 0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.4", 0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "006tx01a.5", 0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.6", 0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.7", 0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.8", 0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "006tx01a.9", 0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.10",0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.11",0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.12",0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "006tx01a.13",0x0000000, 0x400000, CRC(ab4c1747) SHA1(2c097bd38f1a92c4b6534992f6bf29fd6dc2d265) )
	ROM_LOAD( "006tx02a.14",0x0400000, 0x400000, CRC(7854a229) SHA1(dba23c1b793dd0308ac1088c819543fff334a57e) )
	ROM_LOAD( "006tx03a.15",0x0800000, 0x400000, CRC(94edfbd1) SHA1(d4004bb1273e6091608856cb4b151e9d81d5ed30) )
	ROM_LOAD( "006tx04a.16",0x0c00000, 0x400000, CRC(82d61652) SHA1(28303ae9e2545a4cb0b5843f9e73407754f41e9e) )

	ROM_REGION16_LE( 0x0c00000, REGION_GFX4, 0 )
	ROMX_LOAD( "006vt01a.17", 0x0000000, 0x400000, CRC(5c20ed4c) SHA1(df679f518292d70b9f23d2bddabf975d56b96910), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "006vt02a.18", 0x0000002, 0x400000, CRC(150eb717) SHA1(9acb067346eb386256047c0f1d24dc8fcc2118ca), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "006vt03a.19", 0x0000004, 0x400000, CRC(021cfcaf) SHA1(fb8b5f50d3490b31f0a4c3e6d3ae1b98bae41c97), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "006sd01a.77", 0x0000000, 0x400000, CRC(790efb6d) SHA1(23ddd3ee8ae808e58cbcaf92a9ef56d3ca6289b5) )
	ROM_LOAD( "006sd02a.78", 0x0400000, 0x400000, CRC(f7f020c7) SHA1(b72fde4ff6384b80166a3cb67d31bf7afda750bc) )
	ROM_LOAD( "006sd03a.79", 0x0800000, 0x400000, CRC(1a678084) SHA1(f52efb6145102d289f332d8341d89a5d231ba003) )
	ROM_LOAD( "006sd04a.80", 0x0c00000, 0x400000, CRC(3c280a5c) SHA1(9d3fc78e18de45382878268db47ff9d9716f1505) )

	ROM_REGION( 0x4000, REGION_USER7, 0 )
	ROMX_LOAD( "default_nvram_japan",  0x00000, 0x4000, CRC(1f618d44) SHA1(c007c5f94b28b8c56c8c539d2f82336515c0ed84), ROM_BIOS(2) )
	ROMX_LOAD( "default_nvram_usa",    0x00000, 0x4000, CRC(f139f4a5) SHA1(6f7e2fc5d902c1499f3c55f9ca2ef7becc49103b), ROM_BIOS(3) )
	ROMX_LOAD( "default_nvram_others", 0x00000, 0x4000, CRC(bf1c3e4a) SHA1(454c6e5e505293bfdeb87d08e72420bba84c3b7b), ROM_BIOS(1) )
	ROMX_LOAD( "default_nvram_korea",  0x00000, 0x4000, CRC(e8fb68df) SHA1(3170e7465b93319c0550f35f8906b5bdc5332eec), ROM_BIOS(4) )
ROM_END

ROM_START( buriki )
	HNG64_BIOS

	ROM_REGION32_BE( 0x2000000, REGION_USER3, 0 )
	ROM_LOAD32_WORD_SWAP( "007pr01b.81", 0x0000002, 0x400000, CRC(a31202f5) SHA1(c657729b292d394ced021a0201a1c5608a7118ba) )
	ROM_LOAD32_WORD_SWAP( "007pr02b.82", 0x0000000, 0x400000, CRC(a563fed6) SHA1(9af9a021beb814e35df968abe5a99225a124b5eb) )
	ROM_LOAD32_WORD_SWAP( "007pr03a.83", 0x0800002, 0x400000, CRC(da5f6105) SHA1(5424cf5289cef66e301e968b4394e551918fe99b) )
	ROM_LOAD32_WORD_SWAP( "007pr04a.84", 0x0800000, 0x400000, CRC(befc7bce) SHA1(83d9ecf944e03a40cf25ee288077c2265d6a588a) )
	ROM_LOAD32_WORD_SWAP( "007pr05a.85", 0x1000002, 0x400000, CRC(013e28bc) SHA1(45e5ac45b42b26957c2877ac1042472c4b5ec914) )
	ROM_LOAD32_WORD_SWAP( "007pr06a.86", 0x1000000, 0x400000, CRC(0620fccc) SHA1(e0bffc56b019c79276a4ef5ec7354edda15b0889) )

	ROM_REGION( 0x4000000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "007sc01a.97", 0x0000000, 0x800000, CRC(4e8300db) SHA1(f1c9e6fddc10efc8f2a530027cca062f48b8c8d4) )
	ROM_LOAD16_BYTE( "007sc02a.99", 0x0000001, 0x800000, CRC(d5855944) SHA1(019c0bd2f8de7ffddd53df6581b40940262f0053) )
	ROM_LOAD16_BYTE( "007sc05a.98", 0x1000000, 0x400000, CRC(27f848c1) SHA1(2ee9cca4e68e56c7c17c8e2d7e0f55a34a5960bd) )
	ROM_LOAD16_BYTE( "007sc06a.100",0x1000001, 0x400000, CRC(c39e9b4c) SHA1(3c8a0494c2a6866ecc0df2c551619c57ee072440) )
	ROM_LOAD16_BYTE( "007sc03a.101",0x2000000, 0x800000, CRC(ff45c9b5) SHA1(ddcc2a10ccac62eb1f3671172ad1a4d163714fca) )
	ROM_LOAD16_BYTE( "007sc04a.103",0x2000001, 0x800000, CRC(e4cb59e9) SHA1(4e07ff374890217466a53d5bfb1fa99eb7402360) )
	ROM_LOAD16_BYTE( "007sc07a.102",0x3000000, 0x400000, CRC(753e7e3d) SHA1(39b2e9fd23878d8fc4f98fe88b466e963d8fc959) )
	ROM_LOAD16_BYTE( "007sc08a.104",0x3000001, 0x400000, CRC(b605928e) SHA1(558042b84115273fa581606daafba0e9688fa002) )

	ROM_REGION( 0x4000000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "007sp01a.105",0x0000000, 0x800000, CRC(160acae6) SHA1(37c15e1d2544ec6f3b61d06200345d6abdd28edf) )
	ROM_LOAD32_BYTE( "007sp02a.109",0x0000001, 0x800000, CRC(1a55331d) SHA1(0b03d5c7312e01874365b31f1ff3d9766abd00f1) )
	ROM_LOAD32_BYTE( "007sp03a.113",0x0000002, 0x800000, CRC(3f308444) SHA1(0acd52312c15a2ed3bacf60a2fd820cb09ebbb55) )
	ROM_LOAD32_BYTE( "007sp04a.117",0x0000003, 0x800000, CRC(6b81aa51) SHA1(55f7702e1d7a2bef7f050d0358de9036a0139877) )
	ROM_LOAD32_BYTE( "007sp05a.106",0x2000000, 0x400000, CRC(32d2fa41) SHA1(b16a0bbd397be2a8d532c85951b924e2e086a189) )
	ROM_LOAD32_BYTE( "007sp06a.110",0x2000001, 0x400000, CRC(b6f8d7f3) SHA1(70ce94f2193ee39218022da617413c42f6753574) )
	ROM_LOAD32_BYTE( "007sp07a.114",0x2000002, 0x400000, CRC(5caa1cc9) SHA1(3e40b10ea3bcf1239d0015da4be869632b805ddd) )
	ROM_LOAD32_BYTE( "007sp08a.118",0x2000003, 0x400000, CRC(7a158c67) SHA1(d66f4920a513208d45b908a1934d9afb894debf1) )

	ROM_REGION( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD( "007tx01a.1", 0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.2", 0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.3", 0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.4", 0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )

	ROM_REGION( 0x1000000, REGION_GFX5, 0 )
	ROM_LOAD( "007tx01a.5", 0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.6", 0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.7", 0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.8", 0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )

	ROM_REGION( 0x1000000, REGION_GFX6, 0 )
	ROM_LOAD( "007tx01a.9", 0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.10",0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.11",0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.12",0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )

	ROM_REGION( 0x1000000, REGION_GFX7, 0 )
	ROM_LOAD( "007tx01a.13",0x0000000, 0x400000, CRC(a7774075) SHA1(4f3da9af131a7efb0f0a5180da57c19c65fffb82) )
	ROM_LOAD( "007tx02a.14",0x0400000, 0x400000, CRC(bc05d5fd) SHA1(84e3fafcebdeb1e2ffae80785949c973a14055d8) )
	ROM_LOAD( "007tx03a.15",0x0800000, 0x400000, CRC(da9484fb) SHA1(f54b669a66400df00bf25436e5fd5c9bf68dbd55) )
	ROM_LOAD( "007tx04a.16",0x0c00000, 0x400000, CRC(02aa3f46) SHA1(1fca89c70586f8ebcdf669ecac121afa5cdf623f) )

	ROM_REGION16_LE( 0x0c00000, REGION_GFX4, 0 )
	ROMX_LOAD( "007vt01a.17", 0x0000000, 0x400000, CRC(f78a0376) SHA1(fde4ddd4bf326ae5f1ed10311c237b13b62e060c), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "007vt02a.18", 0x0000002, 0x400000, CRC(f365f608) SHA1(035fd9b829b7720c4aee6fdf204c080e6157994f), ROM_GROUPWORD | ROM_SKIP(4) )
	ROMX_LOAD( "007vt03a.19", 0x0000004, 0x400000, CRC(ba05654d) SHA1(b7fe532732c0af7860c8eded3c5abd304d74e08e), ROM_GROUPWORD | ROM_SKIP(4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "007sd01a.77", 0x0000000, 0x400000, CRC(1afb48c6) SHA1(b072d4fe72d6c5267864818d300b32e85b426213) )
	ROM_LOAD( "007sd02a.78", 0x0400000, 0x400000, CRC(c65f1dd5) SHA1(7f504c585a10c1090dbd1ac31a3a0db920c992a0) )
	ROM_LOAD( "007sd03a.79", 0x0800000, 0x400000, CRC(356f25c8) SHA1(5250865900894232960686f40c5da35b3868b78c) )
	ROM_LOAD( "007sd04a.80", 0x0c00000, 0x400000, CRC(dabfbbad) SHA1(7d58d5181705618e0e2d69c6fdb81b9b3d2b9e0f) )
ROM_END

BIOSB( 1997, hng64, 0, hng64, hng64, hng64, hng64, ROT0, "SNK", "Hyper NeoGeo 64 BIOS", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1997, roadedge, hng64, hng64, hng64, hng64_drive, hng64_race, ROT0, "SNK", "Roads Edge / Round Trip RV (rev.B)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1997, sams64, hng64, hng64, hng64, hng64, hng64_samurai, ROT0, "SNK", "Samurai Shodown 64 / Samurai Spirits / Paewang Jeonseol 64", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1998, xrally, hng64, hng64, hng64, hng64_drive, hng64_race, ROT0, "SNK", "Xtreme Rally / Off Beat Racer!", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1998, bbust2, hng64, hng64, hng64, bbust2, hng64_shoot, ROT0, "SNK / ADK", "Beast Busters: Second Nightmare", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1998, sams64_2, hng64, hng64, hng64, hng64, hng64_samurai, ROT0, "SNK", "Samurai Shodown 64: Warriors Rage / Samurai Spirits 2: Asura Zanmaden", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1998, fatfurwa, hng64, hng64, hng64, hng64, hng64_fght, ROT0, "SNK", "Fatal Fury: Wild Ambition / Garou Densetsu: Wild Ambition (rev.A)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEB( 1999, buriki, hng64, hng64, hng64, hng64, hng64_fght, ROT0, "SNK", "Buriki One: World Grapple Tournament '99 in Tokyo (rev.B)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
