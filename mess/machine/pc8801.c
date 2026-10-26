// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

	machine/pc8801.c

	NEC PC-8801 series machine emulation

***************************************************************************/

#include <time.h>
#include "driver.h"
#include "mscommon.h"
#include "timer.h"
#include "cpu/z80/z80.h"
#include "sound/beep.h"
#include "machine/8255ppi.h"
#include "machine/nec765.h"
#include "devices/flopdrv.h"
#include "includes/pc8801.h"

/* Global memory and ROM pointers */
UINT8 *pc8801_wram = NULL;
UINT8 *pc8801_hiwram = NULL;
UINT8 *pc8801_gvram = NULL;
UINT8 *pc8801_ewram = NULL;
UINT8 *pc8801_gfx_rom = NULL;
UINT8 *pc8801_n80_rom = NULL;
UINT8 *pc8801_n88_rom = NULL;
UINT8 *pc8801_n88_sub_rom = NULL;
UINT8 *pc8801_kanji_rom = NULL;

/* Global state variables */
int pc8801_is_24KHz = 0;
UINT8 pc8801_ext_rom_bank = 0xff;
UINT8 pc8801_gfx_ctrl = 0x31;
UINT8 pc8801_vram_sel = 3;
UINT8 pc8801_misc_ctrl = 0x80;
UINT8 pc8801_device_ctrl_data = 0;
UINT8 pc8801_window_offset_bank = 0x80;
UINT8 pc8801_layer_mask = 0;
UINT16 pc8801_dma_counter[4];
UINT16 pc8801_dma_address[4];
UINT8 pc8801_alu_reg[3];
UINT8 pc8801_dmac_mode = 0;
UINT8 pc8801_dmac_ff = 0;
UINT8 pc8801_alu_ctrl1 = 0;
UINT8 pc8801_alu_ctrl2 = 0;
UINT8 pc8801_extram_mode = 0;
UINT8 pc8801_extram_bank = 0;
UINT32 pc8801_extram_size = 0;
UINT8 pc8801_txt_width = 0;
UINT8 pc8801_txt_color = 2;
UINT32 pc8801_knj_addr[2];
UINT8 pc8801_fdc_irq_opcode = 0;

struct crtc_t pc8801_crtc;
struct mouse_t pc8801_mouse;
struct palram_t pc8801_palram[8];

/* Internal IRQ latch and mask */
static UINT8 pc8801_i8214_irq_level = 0;
static UINT8 pc8801_vrtc_irq_mask = 0;
static UINT8 pc8801_vrtc_irq_latch = 0;
static UINT8 pc8801_timer_irq_mask = 0;
static UINT8 pc8801_timer_irq_latch = 0;
static UINT8 pc8801_sound_irq_mask = 0;
static UINT8 pc8801_sound_irq_latch = 0;
static UINT8 pc8801_sound_irq_pending = 0;

/* Internal uPD1990A RTC state */
static UINT8 pc8801_rtc_c = 0;
static UINT8 pc8801_rtc_c_unlatched = 0;
static UINT8 pc8801_rtc_clk = 0;
static UINT8 pc8801_rtc_stb = 0;
static UINT8 pc8801_rtc_data_in = 0;
static UINT8 pc8801_rtc_data_out = 0;
static UINT8 pc8801_rtc_shift_reg[5];

/* 8255 PPI state */
static UINT8 pc8801_i8255_0_pc = 0;
static UINT8 pc8801_i8255_1_pc = 0;

static mame_timer *pc8801_rtc_timer = NULL;

/* 8255 PPI cross-CPU communication */
static READ8_HANDLER( load_8255_chip0_A ) { return ppi8255_get_portB(1); }
static READ8_HANDLER( load_8255_chip1_A ) { return ppi8255_get_portB(0); }
static READ8_HANDLER( load_8255_chip0_B ) { return ppi8255_get_portA(1); }
static READ8_HANDLER( load_8255_chip1_B ) { return ppi8255_get_portA(0); }
static READ8_HANDLER( load_8255_chip0_C ) { return (pc8801_i8255_1_pc >> 4) | (pc8801_i8255_0_pc & 0xf0); }
static READ8_HANDLER( load_8255_chip1_C )
{
	UINT8 result = (pc8801_i8255_0_pc >> 4) | (pc8801_i8255_1_pc & 0xf0);
	if (result == 0xF8)
		result = 0xF0;
	return result;
}
static WRITE8_HANDLER( write_8255_chip0_C ) { pc8801_i8255_0_pc = data; }
static WRITE8_HANDLER( write_8255_chip1_C ) { pc8801_i8255_1_pc = data; }

const ppi8255_interface pc8801_8255_config =
{
	2,
	{ load_8255_chip0_A, load_8255_chip1_A },
	{ load_8255_chip0_B, load_8255_chip1_B },
	{ load_8255_chip0_C, load_8255_chip1_C },
	{ NULL, NULL },
	{ NULL, NULL },
	{ write_8255_chip0_C, write_8255_chip1_C }
};

/* FDC callbacks */
static void pc8801_fdc_interrupt(int state)
{
	cpunum_set_input_line(1, 0, state ? HOLD_LINE : CLEAR_LINE);
}

static struct nec765_interface pc8801_fdc_interface =
{
	pc8801_fdc_interrupt,
	NULL
};

void pc8801_init_5fd(void)
{
	ppi8255_init(&pc8801_8255_config);
	nec765_init(&pc8801_fdc_interface, NEC765A);
	cpunum_set_input_line_vector(1, 0, 0);
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1);
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 0), 1, 0);
	floppy_drive_set_ready_state(image_from_devtype_and_index(IO_FLOPPY, 1), 1, 0);
}

READ8_HANDLER( pc8801fd_nec765_tc )
{
	nec765_set_tc_state(1);
	nec765_set_tc_state(0);
	return 0xff;
}

WRITE8_HANDLER( pc8801_upd765_mc_w )
{
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 0), (data & 1) ? 0 : 1);
	floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, 1), (data & 2) ? 0 : 1);
}

WRITE8_HANDLER( pc8801_fdc_irq_vector_w )
{
	pc8801_fdc_irq_opcode = data;
}

WRITE8_HANDLER( pc8801_fdc_drive_mode_w )
{
	/* drive mode: 2D, 2DD, 2HD */
}

/* Interrupt handling */
static int pc8801_irq_callback(int irqline)
{
	if (pc8801_sound_irq_latch)
	{
		pc8801_sound_irq_latch = 0;
		return 4 * 2;
	}
	else if (pc8801_vrtc_irq_latch)
	{
		pc8801_vrtc_irq_latch = 0;
		return 1 * 2;
	}
	else if (pc8801_timer_irq_latch)
	{
		pc8801_timer_irq_latch = 0;
		return 2 * 2;
	}

	return 4 * 2;
}

void pc88sr_sound_interrupt(int irq)
{
	if (irq)
	{
		if (pc8801_sound_irq_mask)
		{
			pc8801_sound_irq_latch = 1;
			pc8801_sound_irq_pending = 0;
			cpunum_set_input_line(0, 0, HOLD_LINE);
		}
		else
		{
			pc8801_sound_irq_pending = 1;
		}
	}
}

INTERRUPT_GEN( pc8801_interrupt )
{
	if (pc8801_vrtc_irq_mask && pc8801_i8214_irq_level >= 2)
	{
		pc8801_vrtc_irq_latch = 1;
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
}

static void pc8801_rtc_timer_callback(int param)
{
	if (pc8801_timer_irq_mask && pc8801_i8214_irq_level >= 3)
	{
		pc8801_timer_irq_latch = 1;
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
}

WRITE8_HANDLER( pc8801_irq_level_w )
{
	if (data & 8)
		pc8801_i8214_irq_level = 7;
	else
		pc8801_i8214_irq_level = data & 7;
}

WRITE8_HANDLER( pc8801_irq_mask_w )
{
	pc8801_timer_irq_mask = data & 1;
	pc8801_vrtc_irq_mask = data & 2;

	if (pc8801_timer_irq_mask == 0)
		pc8801_timer_irq_latch = 0;

	if (pc8801_vrtc_irq_mask == 0)
		pc8801_vrtc_irq_latch = 0;

	if (pc8801_timer_irq_latch == 0 && pc8801_vrtc_irq_latch == 0 && pc8801_sound_irq_latch == 0)
		cpunum_set_input_line(0, 0, CLEAR_LINE);
}

/* Memory read / write handlers */
READ8_HANDLER( pc8801_mem_r )
{
	if (offset <= 0x7fff)
	{
		if (pc8801_extram_mode & 1)
		{
			UINT32 ext_offset = offset | (pc8801_extram_bank * 0x8000);
			if (ext_offset < pc8801_extram_size && pc8801_ewram)
				return pc8801_ewram[ext_offset];
			return 0xff;
		}

		if (pc8801_gfx_ctrl & 2)
			return pc8801_wram[offset];

		if (pc8801_gfx_ctrl & 4)
			return pc8801_n80_rom[offset];

		if (offset >= 0x6000 && offset <= 0x7fff && ((pc8801_ext_rom_bank & 1) == 0))
			return pc8801_n88_sub_rom[(offset & 0x1fff) + (0x2000 * (pc8801_misc_ctrl & 3))];

		return pc8801_n88_rom[offset];
	}
	else if (offset >= 0x8000 && offset <= 0x83ff)
	{
		UINT32 window_offset;

		if (pc8801_gfx_ctrl & 6)
			return pc8801_wram[offset];

		window_offset = (offset & 0x3ff) + (pc8801_window_offset_bank << 8);

		if (((window_offset & 0xf000) == 0xf000) && (pc8801_misc_ctrl & 0x10))
			return pc8801_hiwram[window_offset & 0xfff];

		return pc8801_wram[window_offset & 0xffff];
	}
	else if (offset >= 0x8400 && offset <= 0xbfff)
	{
		return pc8801_wram[offset];
	}
	else if (offset >= 0xc000 && offset <= 0xffff)
	{
		if (pc8801_misc_ctrl & 0x40)
		{
			pc8801_vram_sel = 3;
			if (pc8801_alu_ctrl2 & 0x80)
				return pc8801_alu_r(offset & 0x3fff);
		}

		if (pc8801_vram_sel == 3)
		{
			if (offset >= 0xf000 && offset <= 0xffff && (pc8801_misc_ctrl & 0x10))
				return pc8801_hiwram[offset & 0xfff];

			return pc8801_wram[offset];
		}

		return pc8801_gvram[(offset & 0x3fff) + (0x4000 * pc8801_vram_sel)];
	}

	return 0xff;
}

WRITE8_HANDLER( pc8801_mem_w )
{
	if (offset <= 0x7fff)
	{
		if (pc8801_extram_mode & 0x10)
		{
			UINT32 ext_offset = offset | (pc8801_extram_bank * 0x8000);
			if (ext_offset < pc8801_extram_size && pc8801_ewram)
				pc8801_ewram[ext_offset] = data;
		}
		else
		{
			pc8801_wram[offset] = data;
		}
		return;
	}
	else if (offset >= 0x8000 && offset <= 0x83ff)
	{
		if (pc8801_gfx_ctrl & 6)
		{
			pc8801_wram[offset] = data;
		}
		else
		{
			UINT32 window_offset = (offset & 0x3ff) + (pc8801_window_offset_bank << 8);

			if (((window_offset & 0xf000) == 0xf000) && (pc8801_misc_ctrl & 0x10))
				pc8801_hiwram[window_offset & 0xfff] = data;
			else
				pc8801_wram[window_offset & 0xffff] = data;
		}
		return;
	}
	else if (offset >= 0x8400 && offset <= 0xbfff)
	{
		pc8801_wram[offset] = data;
		return;
	}
	else if (offset >= 0xc000 && offset <= 0xffff)
	{
		if (pc8801_misc_ctrl & 0x40)
		{
			pc8801_vram_sel = 3;
			if (pc8801_alu_ctrl2 & 0x80)
			{
				pc8801_alu_w(offset & 0x3fff, data);
				return;
			}
		}

		if (pc8801_vram_sel == 3)
		{
			if (offset >= 0xf000 && offset <= 0xffff && (pc8801_misc_ctrl & 0x10))
			{
				pc8801_hiwram[offset & 0xfff] = data;
				return;
			}

			pc8801_wram[offset] = data;
			return;
		}

		pc8801_gvram[(offset & 0x3fff) + (0x4000 * pc8801_vram_sel)] = data;
		return;
	}
}

/* Control ports */
READ8_HANDLER( pc8801_ctrl_r )
{
	UINT8 res = readinputportbytag_safe("CTRL", 0xff);

	if (!pc8801_is_24KHz)
		res |= 0x02;
	else
		res &= ~0x02;

	res &= ~0x30;
	if (pc8801_rtc_data_out)
		res |= 0x10;
	if (cpu_getvblank())
		res |= 0x20;

	return res;
}

WRITE8_HANDLER( pc8801_ctrl_w )
{
	UINT8 new_stb = (data & 0x02) >> 1;
	UINT8 new_clk = (data & 0x04) >> 2;

	/* uPD1990A STB handling */
	if (new_stb && !pc8801_rtc_clk)
	{
		pc8801_rtc_c = pc8801_rtc_c_unlatched;
		if (pc8801_rtc_c == 3)
		{
			time_t t = time(NULL);
			struct tm *tc = localtime(&t);
			pc8801_rtc_shift_reg[0] = dec_2_bcd(tc->tm_sec);
			pc8801_rtc_shift_reg[1] = dec_2_bcd(tc->tm_min);
			pc8801_rtc_shift_reg[2] = dec_2_bcd(tc->tm_hour);
			pc8801_rtc_shift_reg[3] = dec_2_bcd(tc->tm_mday);
			pc8801_rtc_shift_reg[4] = ((tc->tm_mon + 1) << 4) | (tc->tm_wday & 0x0f);
			pc8801_rtc_data_out = 1;
		}
		else if (pc8801_rtc_c == 1)
		{
			pc8801_rtc_data_out = pc8801_rtc_shift_reg[0] & 1;
		}
		else if (pc8801_rtc_c == 0)
		{
			pc8801_rtc_data_out = 1;
		}
	}
	pc8801_rtc_stb = new_stb;

	/* uPD1990A CLK handling (rising edge) */
	if (!pc8801_rtc_clk && new_clk)
	{
		if (pc8801_rtc_c == 1)
		{
			pc8801_rtc_shift_reg[0] = (pc8801_rtc_shift_reg[0] >> 1) | ((pc8801_rtc_shift_reg[1] & 1) << 7);
			pc8801_rtc_shift_reg[1] = (pc8801_rtc_shift_reg[1] >> 1) | ((pc8801_rtc_shift_reg[2] & 1) << 7);
			pc8801_rtc_shift_reg[2] = (pc8801_rtc_shift_reg[2] >> 1) | ((pc8801_rtc_shift_reg[3] & 1) << 7);
			pc8801_rtc_shift_reg[3] = (pc8801_rtc_shift_reg[3] >> 1) | ((pc8801_rtc_shift_reg[4] & 1) << 7);
			pc8801_rtc_shift_reg[4] = (pc8801_rtc_shift_reg[4] >> 1) | (pc8801_rtc_data_in << 7);
			pc8801_rtc_data_out = pc8801_rtc_shift_reg[0] & 1;
		}
	}
	pc8801_rtc_clk = new_clk;

	/* Beeper handling */
	if (((pc8801_device_ctrl_data & 0x20) == 0x00) && ((data & 0x20) == 0x20))
		beep_set_state(0, 1);

	if (((pc8801_device_ctrl_data & 0x20) == 0x20) && ((data & 0x20) == 0x00))
		beep_set_state(0, 0);

	/* Mouse latch handling */
	if ((pc8801_device_ctrl_data & 0x40) != (data & 0x40))
	{
		double new_time = timer_get_time();

		if (pc8801_mouse.phase == 0)
		{
			pc8801_mouse.x = readinputportbytag_safe("MOUSEX", 0);
			pc8801_mouse.y = readinputportbytag_safe("MOUSEY", 0);
		}

		if ((data & 0x40) && ((new_time - pc8801_mouse.time) > (1.0 / 900.0)))
		{
			pc8801_mouse.phase = 0;
		}
		else
		{
			pc8801_mouse.phase++;
			pc8801_mouse.phase &= 3;
		}

		pc8801_mouse.time = new_time;
	}

	if (pc8801_device_ctrl_data & 0x80)
		beep_set_state(0, 0);

	pc8801_device_ctrl_data = data;
}

WRITE8_HANDLER( pc8801_rtc_w )
{
	pc8801_rtc_c_unlatched = data & 0x07;
	pc8801_rtc_data_in = (data >> 3) & 1;
}

READ8_HANDLER( pc8801_ext_rom_bank_r )
{
	return pc8801_ext_rom_bank;
}

WRITE8_HANDLER( pc8801_ext_rom_bank_w )
{
	pc8801_ext_rom_bank = data;
}

WRITE8_HANDLER( pc8801_txt_cmt_ctrl_w )
{
	pc8801_txt_width = data & 1;
	pc8801_txt_color = data & 2;
}

WRITE8_HANDLER( pc8801_gfx_ctrl_w )
{
	pc8801_gfx_ctrl = data;
}

READ8_HANDLER( pc8801_misc_ctrl_r )
{
	return pc8801_misc_ctrl;
}

WRITE8_HANDLER( pc8801_misc_ctrl_w )
{
	pc8801_misc_ctrl = data;
	pc8801_sound_irq_mask = ((data & 0x80) == 0);

	if (pc8801_sound_irq_mask == 0)
		pc8801_sound_irq_latch = 0;

	if (pc8801_timer_irq_latch == 0 && pc8801_vrtc_irq_latch == 0 && pc8801_sound_irq_latch == 0)
		cpunum_set_input_line(0, 0, CLEAR_LINE);

	if (pc8801_sound_irq_mask && pc8801_sound_irq_pending)
	{
		cpunum_set_input_line(0, 0, HOLD_LINE);
		pc8801_sound_irq_latch = 1;
		pc8801_sound_irq_pending = 0;
	}
}

READ8_HANDLER( pc8801_window_bank_r )
{
	return pc8801_window_offset_bank;
}

WRITE8_HANDLER( pc8801_window_bank_w )
{
	pc8801_window_offset_bank = data;
}

WRITE8_HANDLER( pc8801_window_bank_inc_w )
{
	pc8801_window_offset_bank++;
}

READ8_HANDLER( pc8801_extram_mode_r )
{
	return (pc8801_extram_mode ^ 0x11) | 0xee;
}

WRITE8_HANDLER( pc8801_extram_mode_w )
{
	pc8801_extram_mode = data & 0x11;
}

READ8_HANDLER( pc8801_extram_bank_r )
{
	return pc8801_extram_bank;
}

WRITE8_HANDLER( pc8801_extram_bank_w )
{
	pc8801_extram_bank = data;
}

READ8_HANDLER( pc8801_kanji_r )
{
	if ((offset & 2) == 0)
	{
		UINT32 addr = (pc8801_knj_addr[0] * 2 + ((offset & 1) ^ 1)) & 0x1ffff;
		return pc8801_kanji_rom[addr];
	}
	return 0xff;
}

WRITE8_HANDLER( pc8801_kanji_w )
{
	if ((offset & 2) == 0)
	{
		if ((offset & 1) == 0)
			pc8801_knj_addr[0] = (pc8801_knj_addr[0] & 0xff00) | (data & 0xff);
		else
			pc8801_knj_addr[0] = (pc8801_knj_addr[0] & 0x00ff) | (data << 8);
	}
}

READ8_HANDLER( pc8801_kanji_lv2_r )
{
	if ((offset & 2) == 0)
	{
		UINT32 addr = 0x20000 + ((pc8801_knj_addr[1] * 2 + ((offset & 1) ^ 1)) & 0x1ffff);
		return pc8801_kanji_rom[addr];
	}
	return 0xff;
}

WRITE8_HANDLER( pc8801_kanji_lv2_w )
{
	if ((offset & 2) == 0)
	{
		if ((offset & 1) == 0)
			pc8801_knj_addr[1] = (pc8801_knj_addr[1] & 0xff00) | (data & 0xff);
		else
			pc8801_knj_addr[1] = (pc8801_knj_addr[1] & 0x00ff) | (data << 8);
	}
}

READ8_HANDLER( pc8801_opn_porta_r )
{
	if (readinputportbytag_safe("BOARD_CONFIG", 0) & 2)
	{
		UINT8 shift = (pc8801_mouse.phase & 1) ? 0 : 4;
		UINT8 res = (pc8801_mouse.phase & 2) ? pc8801_mouse.y : pc8801_mouse.x;
		return ((res >> shift) & 0x0f) | 0xf0;
	}
	return readinputportbytag_safe("OPN_PA", 0xff);
}

READ8_HANDLER( pc8801_opn_portb_r )
{
	return readinputportbytag_safe("OPN_PB", 0xff);
}

DRIVER_INIT( pc8801 )
{
	pc8801_wram = auto_malloc(0x10000);
	pc8801_hiwram = auto_malloc(0x1000);
	pc8801_gvram = auto_malloc(0xc000);
	pc8801_ewram = auto_malloc(0x8000 * 0x100);

	memset(pc8801_wram, 0, 0x10000);
	memset(pc8801_hiwram, 0, 0x1000);
	memset(pc8801_gvram, 0, 0xc000);
	memset(pc8801_ewram, 0, 0x8000 * 0x100);

	pc8801_n80_rom = memory_region(REGION_CPU1);
	pc8801_n88_rom = memory_region(REGION_CPU1) + 0x8000;
	pc8801_n88_sub_rom = memory_region(REGION_CPU1) + 0x10000;
	pc8801_kanji_rom = memory_region(REGION_GFX1);
	pc8801_gfx_rom = memory_region(REGION_GFX1) + 0x1000;

	pc8801_rtc_timer = timer_alloc(pc8801_rtc_timer_callback);
}

MACHINE_RESET( pc88srl )
{
	int i;

	pc8801_is_24KHz = 0;

	pc8801_ext_rom_bank = 0xff;
	pc8801_gfx_ctrl = 0x31;
	pc8801_window_offset_bank = 0x80;
	pc8801_misc_ctrl = 0x80;
	pc8801_layer_mask = 0x00;
	pc8801_vram_sel = 3;

	pc8801_fdc_irq_opcode = 0;
	pc8801_mouse.phase = 0;
	pc8801_mouse.time = 0.0;

	cpunum_set_input_line_vector(1, 0, 0);

	pc8801_txt_width = 0;
	pc8801_txt_color = 2;

	for (i = 0; i < 3; i++)
		pc8801_alu_reg[i] = 0x00;

	pc8801_crtc.param_count = 0;
	pc8801_crtc.cmd = 0;
	pc8801_crtc.status = 0;
	pc8801_crtc.cursor_on = 0;
	pc8801_crtc.irq_mask = 0;
	pc8801_crtc.inverse = 0;
	memset(pc8801_crtc.param, 0, sizeof(pc8801_crtc.param));

	beep_set_frequency(0, 2400);
	beep_set_state(0, 0);

	pc8801_vrtc_irq_mask = 0;
	pc8801_vrtc_irq_latch = 0;
	pc8801_timer_irq_mask = 0;
	pc8801_timer_irq_latch = 0;
	pc8801_sound_irq_mask = 0;
	pc8801_sound_irq_latch = 0;
	pc8801_i8214_irq_level = 0;
	pc8801_sound_irq_pending = 0;

	pc8801_dma_address[2] = 0xf300;
	pc8801_dmac_mode = 0;
	pc8801_dmac_ff = 0;

	pc8801_extram_bank = 0;
	pc8801_extram_mode = 0;
	pc8801_extram_size = 0;

	cpunum_set_irq_callback(0, pc8801_irq_callback);

	pc8801_init_5fd();

	if (pc8801_rtc_timer)
		timer_adjust(pc8801_rtc_timer, TIME_IN_HZ(600), 0, TIME_IN_HZ(600));
}

MACHINE_RESET( pc88srh )
{
	machine_reset_pc88srl();
	pc8801_is_24KHz = 1;
}
