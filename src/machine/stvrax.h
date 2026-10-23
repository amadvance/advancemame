#include "cpu/adsp2100/adsp2100.h"
#include "sound/dmadac.h"

static UINT32 *rax_program;
static UINT16 rax_ram[5][0x2000], rax_regs[32], rax_latch;
static int rax_bank, rax_rombank, rax_overlay, rax_pf;
static int rax_ireg, rax_base, rax_size, rax_inc;
static mame_timer *rax_dma_timer, *rax_audio_timer;

static READ16_HANDLER(rax_ram_r)
{
	return rax_ram[rax_overlay ? 1 + rax_bank : 0][offset];
}
static WRITE16_HANDLER(rax_ram_w)
{
	COMBINE_DATA(&rax_ram[rax_overlay ? 1 + rax_bank : 0][offset]);
}
static void rax_dmovlay(int value) { rax_overlay = value; }
static void rax_dma_done(int address)
{
	rax_regs[4] = 0;
	rax_regs[2] = address & 0x3fff;
	rax_regs[3] = (rax_regs[3] & 0xff) | ((address >> 6) & 0xff00);
	if (rax_regs[3] & 8)
		cpunum_set_input_line(3, INPUT_LINE_RESET, PULSE_LINE);
	else
		cpunum_set_input_line(3, ADSP2181_BDMA, PULSE_LINE);
}
static READ16_HANDLER(rax_control_r)
{
	return offset == 5 ? rax_pf : rax_regs[offset];
}
static WRITE16_HANDLER(rax_control_w)
{
	COMBINE_DATA(&rax_regs[offset]);
	if (offset == 1 || offset == 2 || offset == 4) rax_regs[offset] &= 0x3fff;
	if (offset == 3) rax_regs[3] &= 0xff0f;
	if (offset == 4)
	{
		UINT8 *rom = memory_region(REGION_USER2);
		UINT32 address = ((rax_regs[3] >> 8) << 14) | rax_regs[2];
		int count = rax_regs[4], type = rax_regs[3] & 3;
		int direction = rax_regs[3] & 4;
		int i;
		for (i = 0; i < count; i++)
		{
			UINT32 location = (rax_rombank * 0x400000 + address) & 0xffffff;
			UINT32 value = rom[location];
			int width = type == 0 ? 3 : type == 1 ? 2 : 1;
			int j;
			for (j = 1; j < width; j++) value = (value << 8) | rom[(location + j) & 0xffffff];
			if (!direction)
			{
				if (type == 0) program_write_dword_32le((rax_regs[1] & 0x3fff) * 4, value);
				else data_write_word_16le((rax_regs[1] & 0x3fff) * 2, type == 2 ? value << 8 : value);
			}
			address += width;
			rax_regs[1] = (rax_regs[1] + 1) & 0x3fff;
		}
		timer_adjust(rax_dma_timer, TIME_IN_HZ(16670000) * count, address, 0);
	}
	if ((offset == 19 && !(data & 2)) || (offset == 31 && !(data & 0x1000)))
	{
		timer_adjust(rax_audio_timer, TIME_NEVER, 0, 0);
		dmadac_enable(0, 2, 0);
	}
}
static READ16_HANDLER(rax_host_r)
{
	cpunum_set_input_line(3, ADSP2181_IRQL0, CLEAR_LINE);
	return rax_latch;
}
static WRITE16_HANDLER(rax_host_w) { rax_pf = 0; }
static WRITE16_HANDLER(rax_bank_w) { rax_bank = data & 3; }
static WRITE16_HANDLER(rax_rombank_w) { rax_rombank = data & 3; }

WRITE32_HANDLER(stv_rax_w)
{
	if (!(mem_mask & 0xffff0000))
	{
		rax_latch = data >> 16;
		cpunum_set_input_line(3, ADSP2181_IRQL0, ASSERT_LINE);
		cpu_boost_interleave(TIME_IN_USEC(1), TIME_IN_USEC(5));
	}
}
static void rax_audio(int param)
{
	INT16 samples[0x2000];
	int i, address, count;
	if (rax_inc <= 0) return;
	count = rax_size / (4 * rax_inc);
	if (count > 0x2000) count = 0x2000;
	cpuintrf_push_context(3);
	address = activecpu_get_reg(ADSP2100_I0 + rax_ireg);
	for (i = 0; i < count; i++, address += rax_inc)
		samples[i] = data_read_word_16le((address & 0x3fff) * 2);
	dmadac_transfer(0, 2, 1, 2, count / 2, samples);
	if (address >= rax_base + rax_size) address = rax_base;
	activecpu_set_reg(ADSP2100_I0 + rax_ireg, address);
	cpuintrf_pop_context();
}
static void rax_tx(int port, INT32 data)
{
	int mreg;
	double frequency, period;
	if (port != 0 || !(rax_regs[31] & 0x1000) || !(rax_regs[19] & 2)) return;
	rax_ireg = (rax_regs[19] >> 9) & 7;
	mreg = ((rax_regs[19] >> 7) & 3) | (rax_ireg & 4);
	rax_inc = (INT16)activecpu_get_reg(ADSP2100_M0 + mreg);
	rax_size = activecpu_get_reg(ADSP2100_L0 + rax_ireg);
	rax_base = activecpu_get_reg(ADSP2100_I0 + rax_ireg) - rax_inc;
	activecpu_set_reg(ADSP2100_I0 + rax_ireg, rax_base);
	frequency = 16670000.0 / ((rax_regs[21] + 1) * 16);
	dmadac_set_frequency(0, 2, frequency);
	dmadac_enable(0, 2, 1);
	if (rax_inc > 0 && rax_size)
	{
		period = rax_size / (frequency * 8 * rax_inc);
		timer_adjust(rax_audio_timer, period, 0, period);
	}
}
static ADDRESS_MAP_START(rax_program_map, ADDRESS_SPACE_PROGRAM, 32)
	AM_RANGE(0x0000, 0x3fff) AM_RAM AM_BASE(&rax_program)
ADDRESS_MAP_END
static ADDRESS_MAP_START(rax_data_map, ADDRESS_SPACE_DATA, 16)
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(rax_ram_r, rax_ram_w)
	AM_RANGE(0x2000, 0x3fdf) AM_RAM
	AM_RANGE(0x3fe0, 0x3fff) AM_READWRITE(rax_control_r, rax_control_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START(rax_io_map, ADDRESS_SPACE_IO, 16)
	AM_RANGE(0x0000, 0x0000) AM_WRITE(rax_bank_w)
	AM_RANGE(0x0001, 0x0001) AM_WRITE(rax_rombank_w)
	AM_RANGE(0x0003, 0x0003) AM_READWRITE(rax_host_r, rax_host_w)
ADDRESS_MAP_END
static MACHINE_START(batmanfr)
{
	rax_dma_timer = timer_alloc(rax_dma_done);
	rax_audio_timer = timer_alloc(rax_audio);
	return machine_start_stv();
}
static MACHINE_RESET(batmanfr)
{
	UINT8 *rom = memory_region(REGION_USER2);
	int i;
	machine_reset_stv();
	memset(rax_regs, 0, sizeof(rax_regs));
	memset(rax_ram, 0, sizeof(rax_ram));
	rax_bank = rax_rombank = rax_overlay = 0;
	rax_pf = 1;
	for (i = 0; i < 32; i++)
		rax_program[i] = (rom[i*3] << 16) | (rom[i*3+1] << 8) | rom[i*3+2];
	cpunum_set_info_fct(3, CPUINFO_PTR_ADSP2100_TX_HANDLER, (genf *)rax_tx);
	cpunum_set_info_fct(3, CPUINFO_PTR_ADSP2100_DMOVLAY_HANDLER, (genf *)rax_dmovlay);
	timer_adjust(rax_dma_timer, TIME_NEVER, 0, 0);
	timer_adjust(rax_audio_timer, TIME_NEVER, 0, 0);
}
static MACHINE_DRIVER_START(batmanfr)
	MDRV_IMPORT_FROM(stv)
	MDRV_CPU_ADD(ADSP2181, 16670000)
	MDRV_CPU_PROGRAM_MAP(rax_program_map, 0)
	MDRV_CPU_DATA_MAP(rax_data_map, 0)
	MDRV_CPU_IO_MAP(rax_io_map, 0)
	MDRV_MACHINE_START(batmanfr)
	MDRV_MACHINE_RESET(batmanfr)
	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 1.0)
	MDRV_SOUND_ADD(DMADAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 1.0)
MACHINE_DRIVER_END
