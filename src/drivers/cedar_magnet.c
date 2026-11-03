// license:BSD-3-Clause
// copyright-holders:David Haywood,AJR,Curt Coder
/***************************************************************************

    Magnet System by EFO SA (Electronica Funcional Operativa SA).
    Based on Cedar hardware.

    Master, two plane, sprite and sound boards each contain a Z80.
    Programs and graphics are loaded from protected 240 RPM floppy disks.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80ctc.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"

#define CED_SOUND 1
#define CED_PLANE0 2
#define CED_PLANE1 3
#define CED_SPRITE 4
#define CED_DISK_SIZE 0xf0000

static UINT8 main_ram[0x40000];
static UINT8 *sound_ram, *plane_ram[2];
static UINT8 sprite_ram[0x40000];
static UINT8 plane_fb[2][0x10000], sprite_fb[0x10000];
static UINT8 palette_rgb[3][0x400];
static UINT8 master_pa, master_pb, master_sound;
static UINT8 ram_palbank, palette_address, memory_config;
static int main_config_installed;
static int board_running[5];
static mame_timer *board_timers[5][4];
static int protection_address[2];
static UINT8 plane_control[2], plane_flip[2], plane_scrollx[2], plane_scrolly[2];
static UINT8 plane_x[2], plane_y[2];
static UINT8 sprite_bank, sprite_busbank, sprite_scrollx, sprite_scrolly, sprite_size, sprite_control, sprite_codehi, sprite_codelo;
static UINT8 sprite_x, sprite_y, sprite_gfxbank, sprite_high_write;
static UINT8 floppy_command, floppy_data, floppy_status, floppy_track;
static int floppy_sector, floppy_offset;

/* PIO interrupts are not connected on these boards; only the port modes and
   direction masks affect the bus, video and CPU control signals. */
static struct
{
	UINT8 mode, direction, output, control, next;
} pio[9][2];

static READ8_HANDLER( other_cpu_r );
static WRITE8_HANDLER( other_cpu_w );
static READ8_HANDLER( other_cpu_high_r );
static WRITE8_HANDLER( other_cpu_high_w );
static void sound_reset(void);

static void update_main_banks(void)
{
	UINT8 *ram = main_ram + (ram_palbank & 3) * 0x10000;
	UINT8 *rom = memory_region(REGION_CPU1);
	int config = memory_config;

	memory_set_bankptr(1, config == 3 ? rom : ram);
	memory_set_bankptr(2, config == 3 ? rom + 0xc000 : ram + 0xc000);
	memory_set_bankptr(3, config == 0 || config == 2 ? ram + 0xe000 : rom + 0xe000);

	if (main_config_installed == config)
		return;

	/* Install the bus window only in configuration 2.  Keeping ordinary RAM
	   banked lets instruction fetches use the CPU core's direct memory path. */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0xbfff, 0, 0,
		config == 3 ? MWA8_NOP : MWA8_BANK1);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0,
		config == 2 ? other_cpu_r : MRA8_BANK2);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xdfff, 0, 0,
		config == 2 ? other_cpu_w : config == 3 ? MWA8_NOP : MWA8_BANK2);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0,
		config == 2 ? other_cpu_high_r : MRA8_BANK3);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0,
		config == 2 ? other_cpu_high_w : config == 0 ? MWA8_BANK3 : MWA8_NOP);
	main_config_installed = config;
}

/* The upper half of the shared bus window must retain its 0x2000 offset. */
static READ8_HANDLER( other_cpu_high_r ) { return other_cpu_r(offset + 0x2000); }
static WRITE8_HANDLER( other_cpu_high_w ) { other_cpu_w(offset + 0x2000, data); }

static void board_line_callback(int param)
{
	int cpu = param >> 2;
	int operation = param & 3;
	if (operation < 2)
	{
		cpunum_set_input_line(cpu, INPUT_LINE_RESET, operation == 0 ? ASSERT_LINE : CLEAR_LINE);
		if (cpu == CED_SOUND && operation == 0)
			sound_reset();
	}
	else
	{
		board_running[cpu] = operation == 3;
		cpunum_set_input_line(cpu, INPUT_LINE_HALT, operation == 2 ? ASSERT_LINE : CLEAR_LINE);
	}
}

static void board_control(int cpu, int old_data, int data)
{
	if (old_data != data)
	{
		int reset = (data & 4) ? 0 : 1;
		int halt = (data & 2) ? 3 : 2;
		/* RESET settles before HALT; simultaneous changes otherwise let a
		   newly uploaded processor execute stale state for one timeslice. */
		timer_adjust(board_timers[cpu][reset], TIME_IN_USEC(1), (cpu << 2) | reset, 0);
		timer_adjust(board_timers[cpu][halt], TIME_IN_USEC(2), (cpu << 2) | halt, 0);
	}
}

static READ8_HANDLER( other_cpu_r )
{
	int address = offset + ((master_sound & 0x0c) >> 2) * 0x4000;
	UINT8 result = 0;
	if (((master_pb >> 4) & 7) == 1) result |= sprite_ram[address + sprite_busbank * 0x10000];
	if ((master_pa & 7) == 1) result |= plane_ram[0][address];
	if ((master_pb & 7) == 1) result |= plane_ram[1][address];
	if (((master_sound >> 4) & 7) == 1) result |= sound_ram[address];
	return result;
}

static WRITE8_HANDLER( other_cpu_w )
{
	int address = offset + ((master_sound & 0x0c) >> 2) * 0x4000;
	if (((master_pb >> 4) & 7) == 1) sprite_ram[address + sprite_busbank * 0x10000] = data;
	if ((master_pa & 7) == 1) plane_ram[0][address] = data;
	if ((master_pb & 7) == 1) plane_ram[1][address] = data;
	if (((master_sound >> 4) & 7) == 1) sound_ram[address] = data;
}

static UINT8 pio_input(int chip, int port)
{
	UINT8 result = 0;
	if (chip == 0 && port == 0)
	{
		result = (master_pa & ~0x18) | (readinputportbytag("COIN1") << 3);
		if (!cpu_getvblank()) result |= 0x10;
		if (!board_running[CED_PLANE0]) result &= ~1;
	}
	else if (chip == 0 && port == 1)
	{
		result = (master_pb & ~0x80) | (readinputportbytag("COIN2") << 7);
		if (!board_running[CED_PLANE1]) result &= ~1;
		if (!board_running[CED_SPRITE]) result &= ~0x10;
	}
	else if (chip == 1 && port == 1)
	{
		result = master_sound;
		if (!board_running[CED_SOUND]) result &= ~0x10;
	}
	return result;
}

static void pio_output(int chip, int port, UINT8 data)
{
	if (chip == 0 && port == 0)
	{
		board_control(CED_PLANE0, master_pa & 7, data & 7);
		master_pa = data;
		memory_config = (data >> 5) & 3;
		update_main_banks();
	}
	else if (chip == 0 && port == 1)
	{
		board_control(CED_PLANE1, master_pb & 7, data & 7);
		board_control(CED_SPRITE, (master_pb >> 4) & 7, (data >> 4) & 7);
		master_pb = data;
	}
	else if (chip == 1 && port == 1)
	{
		board_control(CED_SOUND, (master_sound >> 4) & 7, (data >> 4) & 7);
		master_sound = data;
	}
	else if (chip >= 2 && chip <= 5)
	{
		int plane = (chip - 2) / 2;
		if (!(chip & 1))
		{
			if (!port) plane_control[plane] = data;
			else plane_flip[plane] = data & 1;
		}
		else
		{
			if (!port) plane_scrollx[plane] = data;
			else plane_scrolly[plane] = data;
		}
	}
	else if (chip == 6)
	{
		if (!port) sprite_size = data;
		else sprite_control = data;
	}
	else if (chip == 7)
	{
		if (!port) sprite_scrollx = data;
		else sprite_scrolly = data;
	}
	else if (chip == 8)
	{
		if (!port) sprite_x = data;
		else if (sprite_high_write)
		{
			/* Graphics source selection follows the code-high write; it must
			   not bank the command list out from under the sprite CPU. */
			sprite_gfxbank = data & 3;
			sprite_high_write = 0;
		}
		else
		{
			/* The master loads graphics through a separate bus bank while
			   the sprite processor continues running its startup code. */
			sprite_busbank = data & 3;
			sprite_bank = (data >> 2) & 3;
			memory_set_bankptr(4, sprite_ram + sprite_bank * 0x10000);
		}
	}
}

static UINT8 pio_read(int chip, int offset)
{
	int port = (offset >> 1) & 1;
	if (offset & 1)
		return (pio[chip][0].control & 0xc0) | (pio[chip][1].control >> 4);
	if (pio[chip][port].mode == 0) return pio[chip][port].output;
	if (pio[chip][port].mode == 3)
		return (pio_input(chip, port) & pio[chip][port].direction) |
			(pio[chip][port].output & ~pio[chip][port].direction);
	return pio_input(chip, port);
}

static void pio_write(int chip, int offset, UINT8 data)
{
	int port = (offset >> 1) & 1;
	if (offset & 1)
	{
		if (pio[chip][port].next)
		{
			if (pio[chip][port].next == 1) pio[chip][port].direction = data;
			pio[chip][port].next = 0;
		}
		else if (data & 1)
		{
			if ((data & 15) == 15)
			{
				pio[chip][port].mode = data >> 6;
				if ((data >> 6) == 3) pio[chip][port].next = 1;
				if ((data >> 6) == 0) pio_output(chip, port, pio[chip][port].output);
			}
			else if ((data & 15) == 7)
			{
				pio[chip][port].control = data;
				if (data & 0x10) pio[chip][port].next = 2;
			}
			else if ((data & 15) == 3)
				pio[chip][port].control = (pio[chip][port].control & 0x7f) | (data & 0x80);
		}
	}
	else
	{
		pio[chip][port].output = data;
		if (pio[chip][port].mode != 1)
			pio_output(chip, port, pio[chip][port].mode == 3 ?
				pio[chip][port].direction | (data & ~pio[chip][port].direction) : data);
	}
}

static READ8_HANDLER( main_pio0_r ) { return pio_read(0, offset); }
static WRITE8_HANDLER( main_pio0_w ) { pio_write(0, offset, data); }
static READ8_HANDLER( main_pio1_r ) { return pio_read(1, offset); }
static WRITE8_HANDLER( main_pio1_w ) { pio_write(1, offset, data); }
static READ8_HANDLER( plane0_pio_r ) { return pio_read(2 + offset / 4, offset & 3); }
static WRITE8_HANDLER( plane0_pio_w ) { pio_write(2 + offset / 4, offset & 3, data); }
static READ8_HANDLER( plane1_pio_r ) { return pio_read(4 + offset / 4, offset & 3); }
static WRITE8_HANDLER( plane1_pio_w ) { pio_write(4 + offset / 4, offset & 3, data); }
static READ8_HANDLER( sprite_pio_r ) { return pio_read(6 + offset / 4, offset & 3); }
static WRITE8_HANDLER( sprite_pio_w ) { pio_write(6 + offset / 4, offset & 3, data); }

static void plane_video_write(int plane, int offset, UINT8 data)
{
	switch (offset)
	{
	case 0:
		plane_fb[plane][plane_y[plane] * 256 + plane_x[plane]] = data;
		/* The two eight-bit counters wrap independently. */
		if (plane_control[plane] & 1) ++plane_x[plane];
		else ++plane_y[plane];
		break;
	case 1: plane_x[plane] = data; break;
	case 2: plane_y[plane] = data; break;
	}
}

static WRITE8_HANDLER( plane0_video_w ) { plane_video_write(0, offset, data); }
static WRITE8_HANDLER( plane1_video_w ) { plane_video_write(1, offset, data); }

static void sprite_blit(void)
{
	int x, y;
	int size = (sprite_size & 0x7f) < 4 ? 8 << (sprite_size & 3) : 0;
	int source = (sprite_codehi << 8) | sprite_codelo;
	int erase = source == 0;
	int source_end;
	source &= ~0x3f;
	source_end = (source | (size * size - 1)) + 1;
	for (y = 0; y < size && source < source_end; ++y)
		for (x = 0; x < size; ++x)
		{
			int xpos = (sprite_x + x) & 0xff;
			int ypos = (sprite_y + y) & 0xff;
			UINT8 pixel = sprite_ram[(source++ + sprite_gfxbank * 0x10000) & 0x3ffff];
			if (!(sprite_control & 2)) pixel = mame_rand();
			if (!erase && ((!(sprite_control & 0x40) && xpos >= 191) ||
				((sprite_control & 0x40) && xpos < 64))) continue;
			if (erase || pixel) sprite_fb[ypos * 256 + xpos] = erase ? 0 : pixel;
		}
}

static WRITE8_HANDLER( sprite_code_w ) { sprite_codelo = data; sprite_blit(); }
static WRITE8_HANDLER( sprite_codehi_w ) { sprite_codehi = data; sprite_high_write = 1; }
static WRITE8_HANDLER( sprite_y_w ) { sprite_y = data; }
static WRITE8_HANDLER( sprite_irq_ack_w ) { cpunum_set_input_line(CED_SPRITE, 0, CLEAR_LINE); }
static WRITE8_HANDLER( sprite_pixel_w ) { sprite_fb[sprite_y * 256 + sprite_x] = data; }

static WRITE8_HANDLER( rambank_palette_w ) { ram_palbank = data; update_main_banks(); }
static WRITE8_HANDLER( palette_address_w ) { palette_address = data; }
static WRITE8_HANDLER( palette_upload_w )
{
	int bank = ((ram_palbank & 0xc0) >> 6) | (ram_palbank & 0x3c);
	int address = bank * 256 + palette_address;
	int component = address >= 0x2400 && address < 0x2800 ? 0 :
		address >= 0x2800 && address < 0x2c00 ? 1 :
		address >= 0x3000 && address < 0x3400 ? 2 : -1;
	if (component >= 0)
	{
		int index = address & 0x3ff;
		palette_rgb[component][index] = data & 15;
		palette_set_color(index ^ 0xff, palette_rgb[0][index] * 17,
			palette_rgb[1][index] * 17, palette_rgb[2][index] * 17);
	}
}

static READ8_HANDLER( zero_r ) { return 0; }
static READ8_HANDLER( port19_r ) { return 4; }
static READ8_HANDLER( protection_r ) { return 1; }

static int disk_address(void)
{
	return floppy_track * 0x3000 + ((floppy_command >> 1) & 1) * 0x1800 + floppy_sector * 0x400 + floppy_offset;
}

static void disk_advance(void)
{
	floppy_status |= 5;
	if (++floppy_offset == 0x400)
	{
		floppy_status &= ~5;
		floppy_offset = 0;
		++floppy_sector;
	}
}

static READ8_HANDLER( floppy_r )
{
	switch (offset)
	{
	case 0: return floppy_status;
	case 1: return floppy_track;
	case 3:
		if ((floppy_command & 0xf0) == 0x90)
		{
			int address = disk_address();
			UINT8 result = address >= 0 && address < CED_DISK_SIZE ? memory_region(REGION_USER1)[address] : 0xff;
			disk_advance();
			return result;
		}
		return 0xff;
	}
	return 0;
}

static WRITE8_HANDLER( floppy_w )
{
	switch (offset)
	{
	case 0:
		floppy_command = data;
		switch (data & 0xf0)
		{
		case 0x00: floppy_status = 6; floppy_track = 0; break;
		case 0x10: floppy_track = floppy_data; break;
		case 0x90:
		case 0xb0: floppy_status |= 7; floppy_offset = 0; break;
		}
		break;
	case 2: floppy_sector = data - 200; break;
	case 3:
		floppy_data = data;
		if ((floppy_command & 0xf0) == 0xb0)
		{
			int address = disk_address();
			/* Invalid geometry must never overwrite another emulated device.
			   The image includes the protected sectors with their full 1K data. */
			if (address >= 0 && address < CED_DISK_SIZE) memory_region(REGION_USER1)[address] = data;
			disk_advance();
		}
		break;
	}
}

static NVRAM_HANDLER( cedar_magnet )
{
	/* The writable disk image holds both game code and persistent settings.
	   A first run must retain its ROM contents rather than zero-filling it. */
	if (read_or_write) mame_fwrite(file, memory_region(REGION_USER1), CED_DISK_SIZE);
	else if (file) mame_fread(file, memory_region(REGION_USER1), CED_DISK_SIZE);
}

static UINT8 sound_command, sound_pending, sound_ctc_irq[2], ay1_porta;
static UINT8 fifo[16], fifo_read, fifo_count, fifo_shift_pending;
static mame_timer *fifo_timer, *adpcm_timer, *clock_restart_timer;
static mame_timer *counter_timer[8];
static UINT8 counter_mode[8], counter_wait[8], counter_active[8];
static int counter_constant[8], counter_clock[8];

static void sound_update_irq(void)
{
	cpunum_set_input_line(CED_SOUND, 0, sound_ctc_irq[0] || sound_ctc_irq[1] || sound_pending ? ASSERT_LINE : CLEAR_LINE);
}

static void ctc0_irq(int state) { sound_ctc_irq[0] = state; sound_update_irq(); }
static void ctc1_irq(int state) { sound_ctc_irq[1] = state; sound_update_irq(); }
static int latch_irq_state(int param) { return sound_pending ? Z80_DAISY_INT : 0; }
static int latch_irq_ack(int param) { return 0xff; }
static void latch_irq_reti(int param) { }

static struct z80_irq_daisy_chain sound_daisy[] =
{
	{ 0, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 1 },
	{ 0, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ 0, latch_irq_state, latch_irq_ack, latch_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static void counter_schedule(int channel)
{
	double period = TIME_NEVER;
	if (counter_active[channel] && counter_clock[channel] && (counter_mode[channel] & 0x40))
		period = (double)counter_constant[channel] / counter_clock[channel];
	timer_adjust(counter_timer[channel], period, channel, period == TIME_NEVER ? 0 : period);
}

static void counter_tick(int channel)
{
	z80ctc_trg_w(channel / 4, channel & 3, 1);
	z80ctc_trg_w(channel / 4, channel & 3, 0);
}

static WRITE8_HANDLER( sound_ctc_w )
{
	int which = offset / 4;
	int channel = offset & 3;
	if (counter_wait[offset])
	{
		counter_wait[offset] = 0;
		counter_constant[offset] = data ? data : 256;
		counter_active[offset] = 1;
		/* The fixed external clocks are batched at terminal counts.  The
		   CTC still arbitrates interrupts, including daisy-chain priorities,
		   without scheduling millions of individual clock edges per second. */
		z80ctc_w(which, channel, (counter_mode[offset] & 0x40) && channel != 3 ? 1 : data);
		counter_schedule(offset);
		if (!(counter_mode[offset] & 0x40) && counter_clock[offset]) counter_tick(offset);
	}
	else
	{
		if (data & 1)
		{
			counter_mode[offset] = data;
			counter_wait[offset] = (data & 4) != 0;
			if (data & 2) counter_active[offset] = 0;
			counter_schedule(offset);
		}
		z80ctc_w(which, channel, data);
	}
}

static READ8_HANDLER( sound_ctc_r )
{
	if (counter_active[offset] && counter_clock[offset] && (counter_mode[offset] & 0x40))
		return (int)(timer_timeleft(counter_timer[offset]) * counter_clock[offset] + 1) & 0xff;
	return z80ctc_r(offset / 4, offset & 3);
}

static void fifo_shift(int param)
{
	fifo_shift_pending = 0;
	if (fifo_count)
	{
		fifo_read = (fifo_read + 1) & 15;
		--fifo_count;
		if (fifo_count) MSM5205_data_w(0, fifo[fifo_read]);
	}
	if (!fifo_count) z80ctc_trg_w(0, 3, 1);
}

static void adpcm_clock(int param) { MSM5205_vclk_w(0, 0); }

static WRITE8_HANDLER( ctc0_z2_w )
{
	if (data)
	{
		fifo_shift_pending = 1;
		timer_adjust(fifo_timer, TIME_IN_USEC(4), 0, 0);
		timer_adjust(adpcm_timer, TIME_IN_USEC(32), 0, 0);
		z80ctc_trg_w(0, 3, 0);
		MSM5205_vclk_w(0, 1);
	}
}

static void clock_restart(int param)
{
	counter_clock[0] = 2000000;
	counter_schedule(0);
}

static WRITE8_HANDLER( ctc0_z0_w )
{
	if (data)
	{
		counter_clock[0] = 0;
		counter_schedule(0);
		timer_adjust(clock_restart_timer, TIME_IN_USEC(32), 0, 0);
	}
}

static z80ctc_interface ctc0_interface = { 4000000, 0, ctc0_irq, ctc0_z0_w, 0, ctc0_z2_w };
static z80ctc_interface ctc1_interface = { 4000000, 0, ctc1_irq, 0, 0, 0 };

static WRITE8_HANDLER( sound_command_w )
{
	sound_command = data;
	sound_pending = 1;
	sound_update_irq();
}

static READ8_HANDLER( sound_command_r )
{
	sound_pending = 0;
	sound_update_irq();
	return sound_command;
}

static WRITE8_HANDLER( adpcm_fifo_w )
{
	if (fifo_count < 16)
	{
		fifo[(fifo_read + fifo_count) & 15] = data & 15;
		if (!fifo_count)
		{
			MSM5205_data_w(0, data);
			if (!fifo_shift_pending) z80ctc_trg_w(0, 3, 0);
		}
		++fifo_count;
	}
}

static WRITE8_HANDLER( ay1_porta_w )
{
	MSM5205_reset_w(0, data & 1);
	if (data & 1)
	{
		fifo_count = fifo_read = 0;
		if (!fifo_shift_pending) z80ctc_trg_w(0, 3, 1);
	}
	if ((data & 0x60) != (ay1_porta & 0x60))
	{
		counter_clock[2] = data & 0x40 ? 1000000 : data & 0x20 ? 500000 : data & 0x10 ? 250000 : 0;
		counter_schedule(2);
	}
	ay1_porta = data;
}

static struct AY8910interface ay1_interface = { 0, 0, ay1_porta_w, 0 };
static struct MSM5205interface adpcm_interface = { 0, MSM5205_SEX_4B };

static void sound_reset(void)
{
	int i;
	sound_pending = sound_command = 0;
	fifo_count = fifo_read = ay1_porta = fifo_shift_pending = 0;
	for (i = 0; i < 8; ++i)
	{
		counter_mode[i] = 2;
		counter_wait[i] = counter_active[i] = 0;
		counter_constant[i] = 256;
		counter_clock[i] = i < 2 ? 2000000 : i >= 4 && i < 7 ? 1000000 : 0;
		timer_adjust(counter_timer[i], TIME_NEVER, 0, 0);
	}
	timer_adjust(fifo_timer, TIME_NEVER, 0, 0);
	timer_adjust(adpcm_timer, TIME_NEVER, 0, 0);
	timer_adjust(clock_restart_timer, TIME_NEVER, 0, 0);
	z80ctc_reset(0);
	z80ctc_reset(1);
}

static void protection_update(void)
{
	int pattern, i;
	for (pattern = 0; pattern < 2; ++pattern)
	{
		int address = protection_address[pattern];
		if (address < 0)
		{
			UINT8 first = pattern ? 0x10 : 0x7f;
			UINT8 second = pattern ? 0xdd : 0xc8;
			/* Code arrives in RAM from disk.  Cache the two locations once
			   found, then reapply the protection bypass after any reload. */
			for (i = 0; i < 0x40000 - 4; ++i)
				if (main_ram[i] == first && main_ram[i + 1] == second &&
					main_ram[i + 2] == 0x3e && main_ram[i + 3] == 0xff)
				{
					protection_address[pattern] = i + 2;
					break;
				}
		}
		else if (main_ram[address] == 0x3e && main_ram[address + 1] == 0xff)
			main_ram[address] = 0xc9;
	}
}

static INTERRUPT_GEN( cedar_irq )
{
	protection_update();
	cpunum_set_input_line(0, 0, HOLD_LINE);
	cpunum_set_input_line(CED_PLANE0, 0, HOLD_LINE);
	cpunum_set_input_line(CED_PLANE1, 0, HOLD_LINE);
	/* Acknowledge is explicit on the sprite board; an early clear can
	   queue another request before a long drawing handler has finished. */
	cpunum_set_input_line(CED_SPRITE, 0, ASSERT_LINE);
}

static MACHINE_START( cedar_magnet )
{
	int cpu, i;
	for (cpu = 1; cpu < 5; ++cpu)
		for (i = 0; i < 4; ++i) board_timers[cpu][i] = timer_alloc(board_line_callback);
	for (i = 0; i < 8; ++i) counter_timer[i] = timer_alloc(counter_tick);
	fifo_timer = timer_alloc(fifo_shift);
	adpcm_timer = timer_alloc(adpcm_clock);
	clock_restart_timer = timer_alloc(clock_restart);
	z80ctc_init(0, &ctc0_interface);
	z80ctc_init(1, &ctc1_interface);
	return 0;
}

static MACHINE_RESET( cedar_magnet )
{
	int cpu, i, port;
	for (i = 0; i < 9; ++i)
		for (port = 0; port < 2; ++port)
		{
			memset(&pio[i][port], 0, sizeof(pio[i][port]));
			pio[i][port].mode = 1;
		}
	for (cpu = 1; cpu < 5; ++cpu)
	{
		board_running[cpu] = 0;
		for (i = 0; i < 4; ++i) timer_adjust(board_timers[cpu][i], TIME_NEVER, 0, 0);
		cpunum_set_input_line(cpu, INPUT_LINE_HALT, ASSERT_LINE);
	}
	sound_reset();
	master_pa = 0xff;
	master_pb = master_sound = 0;
	ram_palbank = 3;
	palette_address = 0;
	memory_config = 3;
	main_config_installed = -1;
	update_main_banks();
	sprite_bank = sprite_busbank = sprite_high_write = sprite_gfxbank = 0;
	sprite_size = 0xff;
	memory_set_bankptr(4, sprite_ram);
	floppy_command = floppy_data = floppy_status = floppy_track = 0;
	floppy_sector = floppy_offset = 0;
	protection_address[0] = protection_address[1] = -1;
}

static VIDEO_START( cedar_magnet ) { return 0; }

static VIDEO_UPDATE( cedar_magnet )
{
	int x, y, plane;
	int palette = ram_palbank & 0xc0;
	fillbitmap(bitmap, get_black_pen(), cliprect);
	for (plane = 1; plane >= 0; --plane)
		if (plane_control[plane] & 0x40)
			for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
			{
				UINT16 *dest = bitmap->line[y];
				UINT8 *src = plane_fb[plane] + (((y + plane_scrolly[plane]) ^ (plane_flip[plane] ? 0xff : 0)) & 0xff) * 256;
				for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
				{
					UINT8 pixel = src[((x + plane_scrollx[plane]) ^ (plane_flip[plane] ? 0xff : 0)) & 0xff];
					if (pixel) dest[x] = Machine->pens[pixel + palette * 4];
				}
			}
	if (sprite_size & 0x40)
		for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		{
			int flip = sprite_control & 4 ? 0xff : 0;
			UINT16 *dest = bitmap->line[y];
			UINT8 *src = sprite_fb + (((y + sprite_scrolly) ^ flip) & 0xff) * 256;
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			{
				UINT8 pixel = src[((x + sprite_scrollx) ^ flip) & 0xff];
				if (pixel) dest[x] = Machine->pens[pixel + palette * 4];
			}
		}
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_RAMBANK(1)
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK(2)
	AM_RANGE(0xe000, 0xffff) AM_RAMBANK(3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x18, 0x18) AM_READ(zero_r) AM_WRITENOP
	AM_RANGE(0x19, 0x19) AM_READ(port19_r) AM_WRITENOP
	AM_RANGE(0x1a, 0x1a) AM_READ(zero_r)
	AM_RANGE(0x1b, 0x1b) AM_WRITENOP
	AM_RANGE(0x20, 0x23) AM_READWRITE(main_pio0_r, main_pio0_w)
	AM_RANGE(0x40, 0x43) AM_READWRITE(main_pio1_r, main_pio1_w)
	AM_RANGE(0x60, 0x63) AM_READWRITE(floppy_r, floppy_w)
	AM_RANGE(0x64, 0x64) AM_READ(input_port_2_r)
	AM_RANGE(0x68, 0x68) AM_READ(input_port_3_r)
	AM_RANGE(0x6c, 0x6c) AM_READ(input_port_4_r)
	AM_RANGE(0x70, 0x70) AM_WRITE(rambank_palette_w)
	AM_RANGE(0x74, 0x74) AM_WRITE(palette_upload_w)
	AM_RANGE(0x78, 0x78) AM_READWRITE(zero_r, palette_address_w)
	AM_RANGE(0x7c, 0x7c) AM_READ(protection_r)
	AM_RANGE(0xff, 0xff) AM_WRITE(sound_command_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&sound_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0x07) AM_READWRITE(sound_ctc_r, sound_ctc_w)
	AM_RANGE(0x08, 0x08) AM_WRITE(adpcm_fifo_w)
	AM_RANGE(0x0c, 0x0c) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0d, 0x0d) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x10, 0x10) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x11, 0x11) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0x14, 0x14) AM_READ(sound_command_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( plane0_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&plane_ram[0])
ADDRESS_MAP_END
static ADDRESS_MAP_START( plane1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&plane_ram[1])
ADDRESS_MAP_END
static ADDRESS_MAP_START( plane0_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0xc0, 0xc7) AM_READWRITE(plane0_pio_r, plane0_pio_w)
	AM_RANGE(0xcc, 0xcf) AM_WRITE(plane0_video_w)
ADDRESS_MAP_END
static ADDRESS_MAP_START( plane1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0xc0, 0xc7) AM_READWRITE(plane1_pio_r, plane1_pio_w)
	AM_RANGE(0xcc, 0xcf) AM_WRITE(plane1_video_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sprite_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_RAMBANK(4)
	ADDRESS_MAP_END
static ADDRESS_MAP_START( sprite_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x80, 0x80) AM_WRITE(sprite_code_w)
	AM_RANGE(0x84, 0x84) AM_WRITE(sprite_codehi_w)
	AM_RANGE(0x88, 0x88) AM_WRITE(sprite_y_w)
	AM_RANGE(0x8c, 0x8c) AM_WRITE(sprite_pixel_w)
	AM_RANGE(0x9c, 0x9c) AM_WRITE(sprite_irq_ack_w)
	AM_RANGE(0xc0, 0xcb) AM_READWRITE(sprite_pio_r, sprite_pio_w)
ADDRESS_MAP_END

static MACHINE_DRIVER_START( cedar_magnet )
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io, 0)
	MDRV_CPU_VBLANK_INT(cedar_irq, 1)
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_io, 0)
	MDRV_CPU_CONFIG(sound_daisy)
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(plane0_map, 0)
	MDRV_CPU_IO_MAP(plane0_io, 0)
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(plane1_map, 0)
	MDRV_CPU_IO_MAP(plane1_io, 0)
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sprite_map, 0)
	MDRV_CPU_IO_MAP(sprite_io, 0)
	MDRV_MACHINE_START(cedar_magnet)
	MDRV_MACHINE_RESET(cedar_magnet)
	MDRV_NVRAM_HANDLER(cedar_magnet)
	MDRV_INTERLEAVE(1000)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(5000)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 247, 0, 191)
	MDRV_PALETTE_LENGTH(0x400)
	MDRV_VIDEO_START(cedar_magnet)
	MDRV_VIDEO_UPDATE(cedar_magnet)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	/* Reserve mixing headroom for both PSGs and the sample DAC. */
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.12)
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay1_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.12)
	MDRV_SOUND_ADD(MSM5205, 500000)
	MDRV_SOUND_CONFIG(adpcm_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.24)
MACHINE_DRIVER_END

static DRIVER_INIT( cedar_magnet )
{
	int i;
	UINT8 *rom = memory_region(REGION_CPU1);
	for (i = 0x2000; i < 0x10000; i += 0x2000) memcpy(rom + i, rom, 0x2000);
}


INPUT_PORTS_START( cedar_magnet )
	PORT_START_TAG("COIN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START_TAG("COIN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START_TAG("P1_IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START_TAG("P2_IN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("TEST")
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


#define BIOS_ROM \
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) \
	ROM_LOAD( "magnet-master-vid-e03.bin", 0x00000, 0x02000, CRC(86c4a4f0) SHA1(6db1a006b2e0b2a7cc9748ade881debb098b6757) )

ROM_START( cedmag )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )

ROM_END

ROM_START( mag_boob )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "boobykid.img", 0x00000, 0xde000, BAD_DUMP CRC(3196ffb4) SHA1(99732f74bb907ed6a93ed097c7b211c709d8bf85) )
ROM_END

ROM_START( mag_burn )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "theburningcavern 31_3_87.img", 0x00000, 0xf0000, CRC(c95911f8) SHA1(eda3bdbbcc3e00a7da83253209e832855c2968b1) )
ROM_END

ROM_START( mag_day )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "adayinspace 31_3_87.img", 0x00000, 0xf0000, CRC(bc65302d) SHA1(6ace68a0b5f7a07a8f5c318c5359011074e7f2ec) )
ROM_END

ROM_START( mag_exzi )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "exzisus.img", 0x00000, 0xf0000, CRC(3705e9dc) SHA1(78c8010d224f5deb202a29bd273ea7dc85ddcdb4) )
ROM_END

ROM_START( mag_pdak )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "paris.dsk 31_3_87.img", 0x00000, 0xf0000, BAD_DUMP CRC(2c4ee9e1) SHA1(22c2b75c16aca95ecf2199451c1bd12dd3a3844c) )
ROM_END

ROM_START( mag_time )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, 0 )
	ROM_LOAD( "timescanner.img", 0x00000, 0xf0000, CRC(214c558c) SHA1(9c71fce35acaf17ac685f77aebb1b0a930060f0b) )
ROM_END

ROM_START( mag_war )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "war mission wm 4_6_87.img", 0x00000, 0xf0000, CRC(7c813520) SHA1(2ba5999709a52302aa367fb46199b331421a0d56) )
ROM_END

ROM_START( mag_xain )
	BIOS_ROM

	ROM_REGION( 0x100000, REGION_USER1, ROMREGION_ERASE00 )
	ROM_LOAD( "xain.img", 0x00000, 0xf0000, CRC(5647849f) SHA1(edd2f3f6359424583bf526bf4601476dc849e617) )
ROM_END

GAME( 1987, cedmag,   0,       cedar_magnet, cedar_magnet, cedar_magnet, ROT0,  "EFO SA / Cedar", "Magnet System",                         NOT_A_DRIVER )
GAME( 1987, mag_boob, cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT90, "EFO SA / Cedar", "Booby Kids (Magnet System)",            GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_burn, cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT0,  "EFO SA / Cedar", "The Burning Cavern (31/03/87)",         GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_day,  cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT90, "EFO SA / Cedar", "A Day In Space (31/03/87)",             GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_exzi, cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT0,  "EFO SA / Cedar", "Exzisus (EX 1.0, Magnet System)",       GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_pdak, cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT0,  "EFO SA / Cedar", "Paris Dakar (31/03/87, Spanish)",       GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_time, cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT90, "EFO SA / Cedar", "Time Scanner (TS 2.0, Magnet System)",  GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_war,  cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT90, "EFO SA / Cedar", "War Mission (WM 04/06/87)",             GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAME( 1987, mag_xain, cedmag,  cedar_magnet, cedar_magnet, cedar_magnet, ROT0,  "EFO SA / Cedar", "Xain'd Sleena (SC 3.0, Magnet System)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
