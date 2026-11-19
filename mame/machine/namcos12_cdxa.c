// license:BSD-3-Clause
// copyright-holders:windyfairy
/***************************************************************************

  Namco System 12 CDXA board

***************************************************************************/

#include "driver.h"
#include "cdrom.h"
#include "cpu/sh2/sh2.h"
#include "includes/namcos12_cdxa.h"
#include "includes/psx.h"
#include "sound/custom.h"

#define S12_CDXA_CPU 2

/* CDXA board: the PSX uploads the SH program through the 16-bit RAM window. */
static UINT32 *s12_cdxa_cram, *s12_cdxa_sram;
static cdrom_file *s12_cdxa_disc;
static UINT8 s12_cdxa_regs[8], s12_cdxa_packet[12], s12_cdxa_sector[2352];
static UINT32 s12_cdxa_lba, s12_cdxa_blocks;
static UINT8 s12_cdxa_read_flags;
static int s12_cdxa_sram_enabled, s12_cdxa_ide_psx, s12_cdxa_ide_sh2;
static int s12_cdxa_busy, s12_cdxa_packet_pos, s12_cdxa_pos, s12_cdxa_size;

#define S12_CDXA_SECTOR_SIZE 2048
#define S12_CDXA_DRQ 0x08
#define S12_CDXA_SEEK_COMPLETE 0x10
#define S12_CDXA_READY 0x40

static void s12_cdxa_reset_drive(void)
{
	memset(s12_cdxa_regs, 0, sizeof(s12_cdxa_regs));
	s12_cdxa_regs[7] = S12_CDXA_READY;
	s12_cdxa_blocks = 0;
	s12_cdxa_packet_pos = -1;
	s12_cdxa_pos = s12_cdxa_size = 0;
}

static void s12_cdxa_transfer(int size)
{
	s12_cdxa_pos = 0;
	s12_cdxa_size = size;
	s12_cdxa_regs[4] = size & 0xff;
	s12_cdxa_regs[5] = size >> 8;
	s12_cdxa_regs[2] = size ? 2 : 3;
	s12_cdxa_regs[7] = S12_CDXA_READY | (size ? S12_CDXA_DRQ : 0);
}

static void s12_cdxa_read_sector(void)
{
	UINT32 sector = cdrom_phys_frame_to_chd(s12_cdxa_disc, s12_cdxa_lba++);
	--s12_cdxa_blocks;
	if(cdrom_read_data(s12_cdxa_disc, sector, 1, s12_cdxa_sector, CD_TRACK_MODE2_RAW))
	{
		/* The drive includes the four trailing bytes of a raw Form 2 sector. */
		int size = (s12_cdxa_sector[18] & 0x20) ? 2328 : S12_CDXA_SECTOR_SIZE;
		int start = (s12_cdxa_read_flags & 0x40) ? 16 : 24;
		if(start == 16) size += 8;
		memmove(s12_cdxa_sector, s12_cdxa_sector + start, size);
		s12_cdxa_transfer(size);
	}
	else
	{
		s12_cdxa_blocks = 0;
		s12_cdxa_transfer(0);
		s12_cdxa_regs[1] = 0x50;
		s12_cdxa_regs[7] |= 1;
	}
}

static void s12_cdxa_exec_packet(void)
{
	const UINT8 *packet = s12_cdxa_packet;
	s12_cdxa_packet_pos = -1;
	s12_cdxa_transfer(0);
	switch(packet[0])
	{
	case 0x00: /* TEST UNIT READY */
	case 0xbb: /* SET CD SPEED */
		break;
	case 0x2b: /* SEEK */
		s12_cdxa_regs[7] |= S12_CDXA_SEEK_COMPLETE;
		break;
	case 0x28: /* READ (10) */
	case 0xbe: /* READ CD */
		s12_cdxa_lba = ((UINT32)packet[2] << 24) | (packet[3] << 16) | (packet[4] << 8) | packet[5];
		s12_cdxa_read_flags = packet[0] == 0xbe ? packet[9] : 0x10;
		s12_cdxa_blocks = packet[0] == 0xbe ?
			(packet[6] << 16) | (packet[7] << 8) | packet[8] : (packet[7] << 8) | packet[8];
		if(s12_cdxa_blocks && s12_cdxa_disc) s12_cdxa_read_sector();
		break;
	case 0x03:
		memset(s12_cdxa_sector, 0, 18);
		s12_cdxa_sector[0] = 0x70;
		s12_cdxa_sector[7] = 10;
		s12_cdxa_transfer(MIN(packet[4], 18));
		break;
	default:
		s12_cdxa_regs[1] = 0x50;
		s12_cdxa_regs[7] |= 1;
		break;
	}
}

static UINT16 s12_cdxa_ide_r(unsigned reg)
{
	UINT16 data;
	if(reg) return s12_cdxa_regs[reg];
	if(s12_cdxa_pos >= s12_cdxa_size) return 0xffff;
	data = s12_cdxa_sector[s12_cdxa_pos++];
	if(s12_cdxa_pos < s12_cdxa_size)
		data |= (UINT16)s12_cdxa_sector[s12_cdxa_pos++] << 8;
	if(s12_cdxa_pos == s12_cdxa_size)
	{
		if(s12_cdxa_blocks) s12_cdxa_read_sector();
		else s12_cdxa_transfer(0);
	}
	return data;
}

static void s12_cdxa_ide_w(unsigned reg, UINT16 data)
{
	static const char model[] = "TOSHIBA CD-ROM XM-6402B";
	unsigned i;

	if(reg == 0)
	{
		if(s12_cdxa_packet_pos >= 0)
		{
			s12_cdxa_packet[s12_cdxa_packet_pos++] = data;
			s12_cdxa_packet[s12_cdxa_packet_pos++] = data >> 8;
			if(s12_cdxa_packet_pos == sizeof(s12_cdxa_packet)) s12_cdxa_exec_packet();
		}
		return;
	}
	if(reg != 7)
	{
		s12_cdxa_regs[reg] = data;
		return;
	}

	s12_cdxa_reset_drive();
	switch(data & 0xff)
	{
	case 0xa0: /* PACKET */
		s12_cdxa_packet_pos = 0;
		s12_cdxa_regs[2] = 1;
		s12_cdxa_regs[7] |= S12_CDXA_DRQ;
		break;
	case 0xa1: /* IDENTIFY PACKET DEVICE */
		memset(s12_cdxa_sector, 0, 512);
		s12_cdxa_sector[1] = 0x85;
		memset(s12_cdxa_sector + 54, ' ', 40);
		for(i = 0; i < sizeof(model) - 1; i++)
			s12_cdxa_sector[54 + (i ^ 1)] = model[i];
		s12_cdxa_transfer(512);
		break;
	case 0xe5: /* CHECK POWER MODE */
		s12_cdxa_regs[2] = 0xff;
		break;
	case 0xe1: /* IDLE IMMEDIATE */
	case 0xef: /* SET FEATURES */
		break;
	default:
		s12_cdxa_regs[1] = 4;
		s12_cdxa_regs[7] |= 1;
		break;
	}
}

static READ32_HANDLER(s12_cdxa_ram_r)
{
	UINT32 data = s12_cdxa_sram_enabled ? s12_cdxa_sram[offset] : s12_cdxa_cram[offset];
	/* Each 16-bit access keeps its value across the two bus byte orders. */
	return (data >> 16) | (data << 16);
}

static WRITE32_HANDLER(s12_cdxa_ram_w)
{
	data = (data >> 16) | (data << 16);
	mem_mask = (mem_mask >> 16) | (mem_mask << 16);
	if(!s12_cdxa_sram_enabled) COMBINE_DATA(&s12_cdxa_cram[offset]);
	COMBINE_DATA(&s12_cdxa_sram[offset]);
}

static WRITE32_HANDLER(s12_cdxa_control_w)
{
	switch(offset)
	{
	case 0:
		if(ACCESSING_MSW32 && (data >> 16) == 1)
		{
			cpunum_set_input_line(S12_CDXA_CPU, INPUT_LINE_RESET, PULSE_LINE);
			cpunum_resume(S12_CDXA_CPU, SUSPEND_REASON_HALT);
		}
		break;
	case 4:
		if(ACCESSING_LSW32) s12_cdxa_ide_sh2 = (data & 0xffff) != 0;
		if(ACCESSING_MSW32) s12_cdxa_ide_psx = (data >> 16) != 0;
		break;
	case 6:
		if(ACCESSING_MSW32) s12_cdxa_sram_enabled = (data >> 16) != 0;
		break;
	case 7:
		if(ACCESSING_MSW32) s12_cdxa_busy = 0;
		break;
	}
}

static READ32_HANDLER(s12_cdxa_serial_status_r)
{
	return 0x00200000;
}

static READ32_HANDLER(s12_cdxa_psx_ide_r)
{
	UINT32 data = 0xffffffff;
	if(s12_cdxa_ide_psx)
	{
		if(ACCESSING_LSW32) data = (data & 0xffff0000) | s12_cdxa_ide_r(offset * 2);
		if(ACCESSING_MSW32) data = (data & 0xffff) | ((UINT32)s12_cdxa_ide_r(offset * 2 + 1) << 16);
	}
	return data;
}

static WRITE32_HANDLER(s12_cdxa_psx_ide_w)
{
	if(s12_cdxa_ide_psx)
	{
		if(ACCESSING_LSW32) s12_cdxa_ide_w(offset * 2, data);
		if(ACCESSING_MSW32) s12_cdxa_ide_w(offset * 2 + 1, data >> 16);
	}
}

static READ32_HANDLER(s12_cdxa_sh2_ide_r)
{
	return s12_cdxa_ide_sh2 ? (UINT32)s12_cdxa_ide_r(offset >> 12) << 16 : 0xffffffff;
}

static WRITE32_HANDLER(s12_cdxa_sh2_ide_w)
{
	if(s12_cdxa_ide_sh2 && ACCESSING_MSW32) s12_cdxa_ide_w(offset >> 12, data >> 16);
}

static READ32_HANDLER(s12_cdxa_status_r)
{
	return s12_cdxa_busy ? 0 : 0x08000000;
}

static WRITE32_HANDLER(s12_cdxa_irq_w)
{
	s12_cdxa_busy = 1;
	psx_irq_set(0x400);
}

static READ32_HANDLER(s12_cdxa_sci_status_r)
{
	return 0x84000000;
}

/* Channel 0 feeds the two 16-bit DAC channels through the synchronous SCI. */
#define S12_CDXA_AUDIO_RATE 37800
#define S12_CDXA_SH_CLOCK 14745600
static sound_stream *s12_cdxa_stream;
static mame_timer *s12_cdxa_dma_timer, *s12_cdxa_mtu_timer;
static UINT32 s12_cdxa_dma[4], s12_cdxa_cd_dma[4];
static UINT16 s12_cdxa_dmaor;
static UINT32 s12_cdxa_ipr, s12_cdxa_mtu[0x20], s12_cdxa_mtu_start[3];

static void s12_cdxa_audio_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int i;
	for(i = 0; i < samples; i++)
	{
		UINT32 data = 0;
		if((s12_cdxa_dmaor & 1) && (s12_cdxa_dma[3] & 3) == 1 && s12_cdxa_dma[2] >= 4 &&
		   s12_cdxa_dma[1] == 0xffff81a3 && s12_cdxa_dma[0] < 0x40000)
		{
			data = s12_cdxa_cram[s12_cdxa_dma[0] / 4];
			s12_cdxa_dma[0] += 4;
			s12_cdxa_dma[2] -= 4;
		}
		outputs[0][i] = (INT16)(data >> 16);
		outputs[1][i] = (INT16)data;
	}
}

static void *s12_cdxa_audio_start(int clock, const struct CustomSound_interface *config)
{
	s12_cdxa_stream = stream_create(0, 2, clock, NULL, s12_cdxa_audio_update);
	return s12_cdxa_stream;
}

static struct CustomSound_interface s12_cdxa_audio_interface =
{
	s12_cdxa_audio_start
};

static void s12_cdxa_dma_done(int param)
{
	stream_update(s12_cdxa_stream, 0);
	s12_cdxa_dma[0] += s12_cdxa_dma[2];
	s12_cdxa_dma[2] = 0;
	s12_cdxa_dma[3] |= 2;
	if((s12_cdxa_dma[3] & 4) && (s12_cdxa_ipr >> 28))
		cpunum_set_input_line_and_vector(S12_CDXA_CPU, s12_cdxa_ipr >> 28, HOLD_LINE, 72);
}

static void s12_cdxa_schedule_dma(void)
{
	if((s12_cdxa_dmaor & 1) && (s12_cdxa_dma[3] & 3) == 1 &&
	   s12_cdxa_dma[1] == 0xffff81a3 && s12_cdxa_dma[2])
		timer_adjust(s12_cdxa_dma_timer, TIME_IN_HZ(S12_CDXA_AUDIO_RATE) * (s12_cdxa_dma[2] / 4), 0, 0);
	else
		timer_adjust(s12_cdxa_dma_timer, TIME_NEVER, 0, 0);
}

static READ32_HANDLER(s12_cdxa_dma_r)
{
	stream_update(s12_cdxa_stream, 0);
	return s12_cdxa_dma[offset];
}

static WRITE32_HANDLER(s12_cdxa_dma_w)
{
	stream_update(s12_cdxa_stream, 0);
	COMBINE_DATA(&s12_cdxa_dma[offset]);
	if(offset == 3) s12_cdxa_schedule_dma();
}

static READ32_HANDLER(s12_cdxa_cd_dma_r)
{
	return s12_cdxa_cd_dma[offset];
}

static WRITE32_HANDLER(s12_cdxa_cd_dma_w)
{
	UINT32 count, dest;

	COMBINE_DATA(&s12_cdxa_cd_dma[offset]);
	if(offset != 3 || !(s12_cdxa_dmaor & 1) || (s12_cdxa_cd_dma[3] & 3) != 1)
		return;

	/* The drive supplies a burst of 16-bit words from a fixed data port. */
	count = s12_cdxa_cd_dma[2] & 0xffff;
	dest = s12_cdxa_cd_dma[1] & ~1;
	if(!count) count = 0x10000;
	while(count--)
	{
		program_write_word_32be(dest, program_read_word_32be(s12_cdxa_cd_dma[0] & ~1));
		dest += 2;
	}
	s12_cdxa_cd_dma[1] = dest;
	s12_cdxa_cd_dma[2] = 0;
	s12_cdxa_cd_dma[3] |= 2;
	if((s12_cdxa_cd_dma[3] & 4) && ((s12_cdxa_ipr >> 24) & 15))
		cpunum_set_input_line_and_vector(S12_CDXA_CPU, (s12_cdxa_ipr >> 24) & 15, HOLD_LINE, 76);
}

static WRITE32_HANDLER(s12_cdxa_dmaor_w)
{
	if(ACCESSING_MSW32)
	{
		stream_update(s12_cdxa_stream, 0);
		s12_cdxa_dmaor = data >> 16;
		s12_cdxa_schedule_dma();
	}
}

static READ32_HANDLER(s12_cdxa_ipr_r)
{
	return s12_cdxa_ipr;
}

static WRITE32_HANDLER(s12_cdxa_ipr_w)
{
	COMBINE_DATA(&s12_cdxa_ipr);
}

static void s12_cdxa_mtu_compare(int param)
{
	int priority = (s12_cdxa_ipr >> 4) & 15;

	s12_cdxa_mtu[0x11] |= 0x00010000;
	s12_cdxa_mtu[0x11] &= 0xffff0000;
	s12_cdxa_mtu_start[1] = cpunum_gettotalcycles(S12_CDXA_CPU);
	if((s12_cdxa_mtu[0x11] & 0x01000000) && priority)
		cpunum_set_input_line_and_vector(S12_CDXA_CPU, priority, HOLD_LINE, 96);
}

static UINT16 s12_cdxa_mtu_counter(int channel, UINT32 cycles)
{
	unsigned reg = 8 + 8 * channel;
	unsigned control = s12_cdxa_mtu[reg] >> 24;
	UINT32 count = s12_cdxa_mtu[reg + 1] & 0xffff;

	if((s12_cdxa_mtu[0] & (0x01000000 << channel)) && (control & 7) <= 3)
	{
		count += (cycles - s12_cdxa_mtu_start[channel]) >> (2 * (control & 3));
		if((control & 0x60) == 0x20)
			count %= (s12_cdxa_mtu[reg + 2] >> 16) + 1;
	}
	return count;
}

static READ32_HANDLER(s12_cdxa_mtu_r)
{
	if(offset >= 9 && (offset & 7) == 1)
		return (s12_cdxa_mtu[offset] & 0xffff0000) |
			s12_cdxa_mtu_counter((offset - 9) / 8, cpunum_gettotalcycles(S12_CDXA_CPU));
	return s12_cdxa_mtu[offset];
}

static WRITE32_HANDLER(s12_cdxa_mtu_w)
{
	UINT32 cycles = cpunum_gettotalcycles(S12_CDXA_CPU);
	UINT32 previous = s12_cdxa_mtu[offset];

	if(offset == 0 && !(mem_mask & 0xff000000))
	{
		int channel;
		for(channel = 0; channel < 3; channel++)
		{
			UINT32 enable = 0x01000000 << channel;
			if((previous ^ data) & enable)
			{
				int reg = 9 + channel * 8;
				s12_cdxa_mtu[reg] = (s12_cdxa_mtu[reg] & 0xffff0000) |
					s12_cdxa_mtu_counter(channel, cycles);
				s12_cdxa_mtu_start[channel] = cycles;
			}
		}
	}
	else if(offset >= 9 && (offset & 7) == 1 && ACCESSING_LSW32)
	{
		int channel = (offset - 9) / 8;
		s12_cdxa_mtu[offset] = (previous & 0xffff0000) | s12_cdxa_mtu_counter(channel, cycles);
		s12_cdxa_mtu_start[channel] = cycles;
	}
	COMBINE_DATA(&s12_cdxa_mtu[offset]);

	/* The DMA ISR starts MTU channel 1 to run the audio refill ISR after it returns. */
	if(offset == 0 && ((previous ^ s12_cdxa_mtu[0]) & 0x02000000))
	{
		if(s12_cdxa_mtu[0] & 0x02000000)
		{
			int divider = 1 << ((s12_cdxa_mtu[0x10] >> 24 & 3) * 2);
			int ticks = ((s12_cdxa_mtu[0x12] >> 16) - (s12_cdxa_mtu[0x11] & 0xffff)) & 0xffff;
			timer_adjust(s12_cdxa_mtu_timer, (ticks + 1) * divider * TIME_IN_HZ(S12_CDXA_SH_CLOCK), 0, 0);
		}
		else
			timer_adjust(s12_cdxa_mtu_timer, TIME_NEVER, 0, 0);
	}
}

static ADDRESS_MAP_START(s12_cdxa_sh2_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x0003ffff) AM_RAM AM_BASE(&s12_cdxa_cram)
	AM_RANGE(0x00400000, 0x0040ffff) AM_RAM AM_BASE(&s12_cdxa_sram)
	AM_RANGE(0x0041601c, 0x0041601f) AM_WRITE(s12_cdxa_irq_w)
	AM_RANGE(0x0041c000, 0x0041c003) AM_READ(s12_cdxa_status_r) AM_WRITENOP
	AM_RANGE(0x00c00000, 0x00c1ffff) AM_READWRITE(s12_cdxa_sh2_ide_r, s12_cdxa_sh2_ide_w)
	AM_RANGE(0xffff81a4, 0xffff81a7) AM_READ(s12_cdxa_sci_status_r)
	AM_RANGE(0xffff8240, 0xffff82bf) AM_READWRITE(s12_cdxa_mtu_r, s12_cdxa_mtu_w)
	AM_RANGE(0xffff834c, 0xffff834f) AM_READWRITE(s12_cdxa_ipr_r, s12_cdxa_ipr_w)
	AM_RANGE(0xffff86b0, 0xffff86b3) AM_WRITE(s12_cdxa_dmaor_w)
	AM_RANGE(0xffff86c0, 0xffff86cf) AM_READWRITE(s12_cdxa_dma_r, s12_cdxa_dma_w)
	AM_RANGE(0xffff86d0, 0xffff86df) AM_READWRITE(s12_cdxa_cd_dma_r, s12_cdxa_cd_dma_w)
	AM_RANGE(0xffff8100, 0xffff87ff) AM_RAM
	AM_RANGE(0xfffff000, 0xffffffff) AM_RAM
ADDRESS_MAP_END

static struct sh2_config s12_cdxa_sh2_config = { SH2_CONFIG_PROGRAM_PERIPHERALS };

static void s12_cdxa_exit(void)
{
	if(s12_cdxa_disc) cdrom_close(s12_cdxa_disc);
	s12_cdxa_disc = NULL;
}

void namcos12_cdxa_install(void)
{
	s12_cdxa_disc = cdrom_open(get_disk_handle(0));
	add_exit_callback(s12_cdxa_exit);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f7c0000, 0x1f7cffff, 0, 0, s12_cdxa_ram_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f7c0000, 0x1f7cffff, 0, 0, s12_cdxa_ram_w);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f7d6000, 0x1f7d601f, 0, 0, s12_cdxa_control_w);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f7d8008, 0x1f7d800b, 0, 0, s12_cdxa_serial_status_r);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f7e0000, 0x1f7e000f, 0, 0, s12_cdxa_psx_ide_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x1f7e0000, 0x1f7e000f, 0, 0, s12_cdxa_psx_ide_w);
}

void namcos12_cdxa_start(void)
{
	s12_cdxa_dma_timer = timer_alloc(s12_cdxa_dma_done);
	s12_cdxa_mtu_timer = timer_alloc(s12_cdxa_mtu_compare);
	state_save_register_global_array(s12_cdxa_regs);
	state_save_register_global_array(s12_cdxa_packet);
	state_save_register_global_array(s12_cdxa_sector);
	state_save_register_global(s12_cdxa_lba);
	state_save_register_global(s12_cdxa_blocks);
	state_save_register_global(s12_cdxa_read_flags);
	state_save_register_global(s12_cdxa_sram_enabled);
	state_save_register_global(s12_cdxa_ide_psx);
	state_save_register_global(s12_cdxa_ide_sh2);
	state_save_register_global(s12_cdxa_busy);
	state_save_register_global(s12_cdxa_packet_pos);
	state_save_register_global(s12_cdxa_pos);
	state_save_register_global(s12_cdxa_size);
	state_save_register_global_array(s12_cdxa_dma);
	state_save_register_global_array(s12_cdxa_cd_dma);
	state_save_register_global(s12_cdxa_dmaor);
	state_save_register_global(s12_cdxa_ipr);
	state_save_register_global_array(s12_cdxa_mtu);
	state_save_register_global_array(s12_cdxa_mtu_start);
}

void namcos12_cdxa_reset(void)
{
	s12_cdxa_sram_enabled = s12_cdxa_ide_psx = s12_cdxa_ide_sh2 = s12_cdxa_busy = 0;
	s12_cdxa_reset_drive();
	memset(s12_cdxa_dma, 0, sizeof(s12_cdxa_dma));
	memset(s12_cdxa_cd_dma, 0, sizeof(s12_cdxa_cd_dma));
	memset(s12_cdxa_mtu, 0, sizeof(s12_cdxa_mtu));
	memset(s12_cdxa_mtu_start, 0, sizeof(s12_cdxa_mtu_start));
	s12_cdxa_ipr = s12_cdxa_dmaor = 0;
	timer_adjust(s12_cdxa_dma_timer, TIME_NEVER, 0, 0);
	timer_adjust(s12_cdxa_mtu_timer, TIME_NEVER, 0, 0);
	cpunum_suspend(S12_CDXA_CPU, SUSPEND_REASON_HALT, 1);
}

MACHINE_DRIVER_START( namcos12_cdxa )
	MDRV_CPU_ADD_TAG( "cdxa", SH2, S12_CDXA_SH_CLOCK )
	MDRV_CPU_CONFIG( s12_cdxa_sh2_config )
	MDRV_CPU_PROGRAM_MAP( s12_cdxa_sh2_map, 0 )
	MDRV_SOUND_ADD( CUSTOM, S12_CDXA_AUDIO_RATE )
	MDRV_SOUND_CONFIG( s12_cdxa_audio_interface )
	MDRV_SOUND_ROUTE(0, "left", 0.30)
	MDRV_SOUND_ROUTE(1, "right", 0.30)
MACHINE_DRIVER_END
