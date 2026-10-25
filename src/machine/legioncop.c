// license:LGPL-2.1+
// copyright-holders:Olivier Galibert, Angelo Salese, David Haywood, Tomasz Slanina
/*
 * Seibu COP emulation; Olivier Galibert, Angelo Salese,
 * David Haywood and Tomasz Slanina.
 */
#include "driver.h"
#include "machine/legioncop.h"
#include <math.h>

extern WRITE16_HANDLER( legionna_dma_video_w );
extern WRITE16_HANDLER( legionna_dma_palette_w );

/* The 68000 bus exposes word accesses, so long words require two transfers. */
static UINT32 legioncop_read_dword(offs_t address)
{
	return ((UINT32)program_read_word(address) << 16) | program_read_word(address + 2);
}

static void legioncop_write_dword(offs_t address, UINT32 data)
{
	program_write_word(address, data >> 16);
	program_write_word(address + 2, data & 0xffff);
}

static struct {
	UINT16 cop_func_trigger[0x100/8];
	UINT16 cop_func_value[0x100/8];
	UINT16 cop_func_mask[0x100/8];
	UINT16 cop_program[0x100];
	UINT16 cop_latch_addr, cop_latch_trigger, cop_latch_value, cop_latch_mask;
	UINT16 cop_dma_v1, cop_dma_v2, cop_dma_mode;
	UINT16 cop_dma_src[0x200], cop_dma_dst[0x200], cop_dma_size[0x200];
	UINT16 cop_dma_adr_rel;
	UINT16 pal_brightness_val;
	UINT16 pal_brightness_mode;
	UINT32 cop_itoa;
	UINT16 cop_itoa_mode;
	UINT8 cop_itoa_digits[10];
	UINT32 cop_regs[8];
	UINT16 cop_status, cop_scale, cop_angle, cop_dist;
	UINT16 cop_angle_target;
	UINT16 cop_angle_step;
	UINT16 cop_hit_status, cop_hit_baseadr;
	INT16 cop_hit_val[3];
	UINT16 cop_hit_val_stat;
	UINT32 cop_sort_ram_addr, cop_sort_lookup;
	UINT16 cop_sort_param;
	UINT16 m_cop_rng_max_value;
	UINT32 m_cop_sprite_dma_param;
	UINT32 m_cop_sprite_dma_src;
	int m_cop_sprite_dma_size;
	UINT16 m_cop_unk_param_a,m_cop_unk_param_b;
	UINT16 m_cop_rom_addr_lo,m_cop_rom_addr_hi,m_cop_precmd;
	int m_cop_sprite_dma_abs_x,m_cop_sprite_dma_abs_y;
	int m_sprite_dma_rel_x, m_sprite_dma_rel_y;
	int m_sprite_dma_x_clip;
	int m_LEGACY_r0, m_LEGACY_r1;
	struct colinfo {
	 INT16 pos[3], min[3], max[3];
	 INT8 dx[3];
	 UINT8 size[3];
	 int allow_swap;
	 UINT16 flags_swap;
	 UINT32 spradr;
	} cop_collision_info[2];

} cop;

static void LEGACY_cop_cmd_w(offs_t offset, UINT16 data);
static void LEGACY_execute_130e_cupsoc(int offset, UINT16 data);
static void LEGACY_execute_6200(int offset, UINT16 data);
static void LEGACY_execute_6980(int offset, UINT16 data);
static void LEGACY_execute_c480(int offset, UINT16 data);
static void LEGACY_execute_d104(int offset, UINT16 data);
static void LEGACY_execute_dde5(int offset, UINT16 data);
static void LEGACY_execute_e30e(int offset, UINT16 data);
static void bcd_update(void);
static int check_command_matches(int command, UINT16 seq0, UINT16 seq1, UINT16 seq2, UINT16 seq3, UINT16 seq4, UINT16 seq5, UINT16 seq6, UINT16 seq7, UINT16 _funcval_, UINT16 _funcmask_);
static UINT16 cop_angle_r(void);
static void cop_angle_step_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_angle_target_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_collision_read_pos(int slot, UINT32 spradr, int allow_swap);
static UINT16 cop_collision_status_r(void);
static UINT16 cop_collision_status_stat_r(void);
static UINT16 cop_collision_status_val_r(offs_t offset);
static void cop_collision_update_hitbox(UINT16 data, int slot, UINT32 hitadr);
static UINT16 cop_dist_r(void);
static void cop_dma_adr_rel_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_dma_dst_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static UINT16 cop_dma_mode_r(void);
static void cop_dma_mode_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_dma_size_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_dma_src_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_dma_trigger_w(UINT16 data);
static void cop_dma_v1_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_dma_v2_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_hitbox_baseadr_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static UINT16 cop_itoa_digits_r(offs_t offset);
static void cop_itoa_high_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_itoa_low_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_itoa_mode_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pal_brightness_mode_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pal_brightness_val_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pgm_addr_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pgm_data_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pgm_mask_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pgm_trigger_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_pgm_value_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_precmd_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static UINT16 cop_prng_maxvalue_r(void);
static void cop_prng_maxvalue_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static UINT16 cop_prng_r(void);
static UINT8 cop_read_byte(int address);
static UINT16 cop_read_word(int address);
static UINT16 cop_reg_high_r(offs_t offset);
static void cop_reg_high_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static UINT16 cop_reg_low_r(offs_t offset);
static void cop_reg_low_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_rom_addr_hi_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_rom_addr_lo_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_scale_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_sort_dma_trig_w(UINT16 data);
static void cop_sort_lookup_hi_w(UINT16 data);
static void cop_sort_lookup_lo_w(UINT16 data);
static void cop_sort_param_w(UINT16 data);
static void cop_sort_ram_addr_hi_w(UINT16 data);
static void cop_sort_ram_addr_lo_w(UINT16 data);
static void cop_sprite_dma_abs_x_w(UINT16 data);
static void cop_sprite_dma_abs_y_w(UINT16 data);
static void cop_sprite_dma_inc_w(UINT16 data);
static void cop_sprite_dma_param_hi_w(UINT16 data);
static void cop_sprite_dma_param_lo_w(UINT16 data);
static void cop_sprite_dma_size_w(UINT16 data);
static void cop_sprite_dma_src_hi_w(UINT16 data);
static void cop_sprite_dma_src_lo_w(UINT16 data);
static UINT16 cop_status_r(void);
static void cop_unk_param_a_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_unk_param_b_w(offs_t offset, UINT16 data, UINT16 mem_mask);
static void cop_write_byte(int address, UINT8 data);
static void cop_write_word(int address, UINT16 data);
static void dma_fill(void);
static void dma_palette_brightness(void);
static void dma_palette_buffer(void);
static void dma_tilemap_buffer(void);
static void dma_zsorting(UINT16 data);
static void execute_0205(int offset, UINT16 data);
static void execute_0904(int offset, UINT16 data);
static void execute_130e(int offset, UINT16 data, int is_yflip);
static void execute_338e(int offset, UINT16 data, int is_yflip);
static void execute_3b30(int offset, UINT16 data);
static void execute_42c2(int offset, UINT16 data);
static void execute_6200(int offset, UINT16 data);
static void execute_8100(int offset, UINT16 data);
static void execute_8900(int offset, UINT16 data);
static void execute_a100(int offset, UINT16 data);
static void execute_a900(int offset, UINT16 data);
static void execute_b100(int offset, UINT16 data);
static void execute_b900(int offset, UINT16 data);
static void execute_f105(int offset, UINT16 data);
static UINT8 fade_table(int v);
static int find_trigger_match(UINT16 triggerval, UINT16 mask);
static UINT16 get_func_mask(int command);
static UINT16 get_func_value(int command);

static void LEGACY_cop_cmd_w(offs_t offset, UINT16 data)
{
	int command;

	command = find_trigger_match(data, 0xf800);

	if (command == -1)
	{
		return;
	}
	UINT16 funcval, funcmask;

	funcval = get_func_value(command);
	funcmask = get_func_mask(command);

	if (check_command_matches(command, 0x188, 0x282, 0x082, 0xb8e, 0x98e, 0x000, 0x000, 0x000, 6, 0xffeb))
	{
		execute_0205(offset, data);
		return;
	}

	if (check_command_matches(command, 0x194, 0x288, 0x088, 0x000, 0x000, 0x000, 0x000, 0x000, 6, 0xfbfb))
	{
		execute_0904(offset, data);
		return;
	}

	if (check_command_matches(command, 0xb9a, 0xb88, 0x888, 0x000, 0x000, 0x000, 0x000, 0x000, 7, 0xfdfb))
	{
		execute_8100(offset, data);
		return;
	}

	if (check_command_matches(command, 0xb9a, 0xb8a, 0x88a, 0x000, 0x000, 0x000, 0x000, 0x000, 7, 0xfdfb))
	{
		execute_8900(offset, data);
		return;
	}

	if (check_command_matches(command, 0x984, 0xaa4, 0xd82, 0xaa2, 0x39b, 0xb9a, 0xb9a, 0xa9a, 5, 0xbf7f))
	{

		LEGACY_execute_130e_cupsoc(offset, data);
		return;
	}

	if (check_command_matches(command, 0x984, 0xaa4, 0xd82, 0xaa2, 0x39b, 0xb9a, 0xb9a, 0xb9a, 5, 0xbf7f))
	{
		execute_130e(offset, data, 1);
		return;
	}

	if (check_command_matches(command, 0xf9c, 0xb9c, 0xb9c, 0xb9c, 0xb9c, 0xb9c, 0xb9c, 0x99c, 4, 0x007f))
	{
		execute_3b30(offset, data);
		return;
	}

	if (check_command_matches(command, 0xf9a, 0xb9a, 0xb9c, 0xb9c, 0xb9c, 0x29c, 0x000, 0x000, 5, 0xfcdd))
	{
		execute_42c2(offset, data);
		return;
	}

	if (check_command_matches(command, 0xb80, 0xb82, 0xb84, 0xb86, 0x000, 0x000, 0x000, 0x000, funcval, funcmask))
	{
		execute_a100(offset, data);
		return;
	}

	if (check_command_matches(command, 0xb40, 0xbc0, 0xbc2, 0x000, 0x000, 0x000, 0x000, 0x000, funcval, funcmask))
	{
		execute_b100(offset, data);
		return;
	}

	if (check_command_matches(command, 0xba0, 0xba2, 0xba4, 0xba6, 0x000, 0x000, 0x000, 0x000, funcval, funcmask))
	{
		execute_a900(offset, data);
		return;
	}

	if (check_command_matches(command, 0xb60, 0xbe0, 0xbe2, 0x000, 0x000, 0x000, 0x000, 0x000, funcval, funcmask))
	{
		execute_b900(offset, data);
		return;
	}

	if (check_command_matches(command, 0xb80, 0xba0, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 10, 0xfff3))
	{
		LEGACY_execute_6980(offset, data);
		return;
	}

	if (check_command_matches(command, 0x080, 0x882, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 10, 0xff00))
	{
		LEGACY_execute_c480(offset, data);
		return;
	}

	if (check_command_matches(command, 0xf80, 0xaa2, 0x984, 0x0c2, 0x000, 0x000, 0x000, 0x000, 5, 0x7ff7))
	{
		LEGACY_execute_dde5(offset, data);
		return;
	}

	if (check_command_matches(command, 0x3a0, 0x3a6, 0x380, 0xaa0, 0x2a6, 0x000, 0x000, 0x000, 8, 0xf3e7))
	{
		LEGACY_execute_6200(offset, data);
		return;
	}

	if (check_command_matches(command, 0x380, 0x39a, 0x380, 0xa80, 0x29a, 0x000, 0x000, 0x000, 8, 0xf3e7))
	{
		execute_6200(offset, data);
		return;
	}

	if (check_command_matches(command, 0x984, 0xac4, 0xd82, 0xac2, 0x39b, 0xb9a, 0xb9a, 0xa9a, 5, 0xb07f))
	{
		LEGACY_execute_e30e(offset, data);
		return;
	}

	if (check_command_matches(command, 0xac2, 0x9e0, 0x0a2, 0x000, 0x000, 0x000, 0x000, 0x000, 5, 0xfffb))
	{
		LEGACY_execute_d104(offset, data);
		return;
	}

	if (check_command_matches(command, 0xa80, 0x984, 0x082, 0x000, 0x000, 0x000, 0x000, 0x000, 5, 0xfefb))
	{
		return;
	}

	if (check_command_matches(command, 0x9c8, 0xa84, 0x0a2, 0x000, 0x000, 0x000, 0x000, 0x000, 5, 0xfffb))
	{
		return;
	}

	if (check_command_matches(command, 0xa88, 0x994, 0x088, 0x000, 0x000, 0x000, 0x000, 0x000, 5, 0xfefb))
	{
		execute_f105(offset,data);
		return;
	}

}

static void LEGACY_execute_130e_cupsoc(int offset, UINT16 data)
{
	int dy = legioncop_read_dword(cop.cop_regs[1] + 4) - legioncop_read_dword(cop.cop_regs[0] + 4);
	int dx = legioncop_read_dword(cop.cop_regs[1] + 8) - legioncop_read_dword(cop.cop_regs[0] + 8);

	cop.cop_status = 7;

	if (!dx) {
		cop.cop_status |= 0x8000;
		cop.cop_angle = 0;
	}
	else
	{
		cop.cop_angle = (int)(atan(((double)(dy)) / ((double)(dx))) * 128.0 / M_PI);
		if (dx < 0)
			cop.cop_angle += 0x80;

		cop.cop_angle &= 0xff;
	}

	cop.m_LEGACY_r0 = dy;
	cop.m_LEGACY_r1 = dx;

	if (data & 0x80)
		cop_write_byte(cop.cop_regs[0] + (0x34), cop.cop_angle);
}

static void LEGACY_execute_6200(int offset, UINT16 data)
{
	int primary_reg = 1;
	int primary_offset = 0xc;

	UINT8 angle = cop_read_byte(cop.cop_regs[primary_reg] + primary_offset);
	UINT16 flags = cop_read_word(cop.cop_regs[primary_reg]);
	cop.cop_angle_target &= 0xff;
	cop.cop_angle_step &= 0xff;
	flags &= ~0x0004;
	int delta = angle - cop.cop_angle_target;
	if (delta >= 128)
		delta -= 256;
	else if (delta < -128)
		delta += 256;
	if (delta < 0) {
		if (delta >= -cop.cop_angle_step) {
			angle = cop.cop_angle_target;
			flags |= 0x0004;
		}
		else
			angle += cop.cop_angle_step;
	}
	else {
		if (delta <= cop.cop_angle_step) {
			angle = cop.cop_angle_target;
			flags |= 0x0004;
		}
		else
			angle -= cop.cop_angle_step;
	}

	cop_write_word(cop.cop_regs[primary_reg], flags);

	if (!1)
		cop_write_byte(cop.cop_regs[primary_reg] + primary_offset, angle);
	else
		cop_write_word(cop.cop_regs[primary_reg] + primary_offset, angle);
}

static void LEGACY_execute_6980(int offset, UINT16 data)
{
	UINT8 offs;

	int rel_xy;

	offs = (offset & 3) * 4;

	rel_xy = program_read_word(cop.m_cop_sprite_dma_src + 4 + offs);

	cop.m_sprite_dma_rel_x = (rel_xy & 0xff);
	cop.m_sprite_dma_rel_y = ((rel_xy & 0xff00) >> 8);

}

static void LEGACY_execute_c480(int offset, UINT16 data)
{
	UINT8 offs;
	UINT16 sprite_info;
	UINT16 sprite_x, sprite_y;
	int abs_x, abs_y;

	offs = (offset & 3) * 4;

	sprite_info = program_read_word(cop.m_cop_sprite_dma_src + offs) + (cop.m_cop_sprite_dma_param & 0x3f);
	program_write_word(cop.cop_regs[4] + offs + 0, sprite_info);

	abs_x = program_read_word(cop.cop_regs[0] + 8) - cop.m_cop_sprite_dma_abs_x;
	abs_y = program_read_word(cop.cop_regs[0] + 4) - cop.m_cop_sprite_dma_abs_y;

	const UINT8 fx     =  (sprite_info & 0x4000) >> 14;
	const UINT8 fy     =  (sprite_info & 0x2000) >> 13;
	const UINT8 dx     = ((sprite_info & 0x1c00) >> 10) + 1;

	if (fx)
	{
		sprite_x = (0x100 - (dx*16));
		sprite_x += (abs_x)-(cop.m_sprite_dma_rel_x & 0x78);
		if (cop.m_sprite_dma_rel_x & 0x80)
			sprite_x -= 0x80;
		else
			sprite_x -= 0x100;
	}
	else
		sprite_x = (cop.m_sprite_dma_rel_x & 0x78) + (abs_x)-((cop.m_sprite_dma_rel_x & 0x80) ? 0x80 : 0);

	if (fy)
		sprite_y = (abs_y)+((cop.m_sprite_dma_rel_y & 0x80) ? 0x80 : 0)-(cop.m_sprite_dma_rel_y & 0x78);
	else
		sprite_y = (cop.m_sprite_dma_rel_y & 0x78) + (abs_y)-((cop.m_sprite_dma_rel_y & 0x80) ? 0x80 : 0);

	if (cop.m_cop_sprite_dma_param & 0x00020000)
	{
		UINT16 sprite_pri = program_read_word(cop.cop_regs[4] + offs + 2);
		sprite_pri |= 0x8000;
		program_write_word(cop.cop_regs[4] + offs + 2, sprite_pri);
	}

	cop.m_sprite_dma_x_clip = sprite_x;
	program_write_word(cop.cop_regs[4] + offs + 4, cop.m_sprite_dma_x_clip);
	program_write_word(cop.cop_regs[4] + offs + 6, sprite_y);
}

static void LEGACY_execute_d104(int offset, UINT16 data)
{
	UINT16 *ROM = (UINT16 *)memory_region(REGION_CPU1);
	UINT32 rom_addr = (cop.m_cop_rom_addr_hi << 16 | cop.m_cop_rom_addr_lo);
	UINT16 rom_data = ROM[rom_addr / 2];
}

static void LEGACY_execute_dde5(int offset, UINT16 data)
{
	UINT8 offs;
	int div;
	INT16 dir_offset;

	offs = (offset & 3) * 4;

	div = program_read_word(cop.cop_regs[4] + offs);
	dir_offset = program_read_word(cop.cop_regs[4] + offs + 8);

	if (div == 0) { div = 1; }

	program_write_word((cop.cop_regs[6] + offs + 4), ((program_read_word(cop.cop_regs[5] + offs + 4) + dir_offset) / div));
}

static void LEGACY_execute_e30e(int offset, UINT16 data)
{
	int dy = legioncop_read_dword(cop.cop_regs[2] + 4) - legioncop_read_dword(cop.cop_regs[0] + 4);
	int dx = legioncop_read_dword(cop.cop_regs[2] + 8) - legioncop_read_dword(cop.cop_regs[0] + 8);

	cop.cop_status = 7;
	if (!dx)
	{
		cop.cop_status |= 0x8000;
		cop.cop_angle = 0;
	}
	else {
		cop.cop_angle = (int)(atan(((double)(dy)) / ((double)(dx))) * 128.0 / M_PI);
		if (dx < 0)
			cop.cop_angle += 0x80;

		cop.cop_angle &= 0xff;
	}
	if (data & 0x0080)
		cop_write_byte(cop.cop_regs[0] + 0x34, cop.cop_angle);
}

static void bcd_update(void)
{
 int i;

	UINT32 val = cop.cop_itoa;

		int digits = 9;

	for (i = 0; i < digits; i++)
	{
		if (!val && i)
		{

			cop.cop_itoa_digits[i] = (cop.cop_itoa_mode == 3) ? 0x30 : 0x20;
		}
		else
		{
			cop.cop_itoa_digits[i] = 0x30 | (val % 10);
			val = val / 10;
		}
	}

	cop.cop_itoa_digits[9] = 0;
}

static int check_command_matches(int command, UINT16 seq0, UINT16 seq1, UINT16 seq2, UINT16 seq3, UINT16 seq4, UINT16 seq5, UINT16 seq6, UINT16 seq7, UINT16 _funcval_, UINT16 _funcmask_)
{
	command *= 8;

	if (cop.cop_program[command+0] == seq0 && cop.cop_program[command+1] == seq1 && cop.cop_program[command+2] == seq2 && cop.cop_program[command+3] == seq3 &&
		cop.cop_program[command+4] == seq4 && cop.cop_program[command+5] == seq5 && cop.cop_program[command+6] == seq6 && cop.cop_program[command+7] == seq7 &&
		cop.cop_func_value[command/8] == _funcval_ &&
		cop.cop_func_mask[command/8] == _funcmask_)
		return 1;
	else
		return 0;
}

static UINT16 cop_angle_r(void)
{
	return cop.cop_angle;
}

static void cop_angle_step_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_angle_step);
}

static void cop_angle_target_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_angle_target);
}

static void cop_collision_read_pos(int slot, UINT32 spradr, int allow_swap)
{
 int i;

	cop.cop_collision_info[slot].allow_swap = allow_swap;
	cop.cop_collision_info[slot].flags_swap = cop_read_word(spradr+2);
	cop.cop_collision_info[slot].spradr = spradr;
	for (i =0; i<3; i++)
		cop.cop_collision_info[slot].pos[i] = cop_read_word(spradr+6+4*i);
}

static UINT16 cop_collision_status_r(void)
{
	return cop.cop_hit_status;
}

static UINT16 cop_collision_status_stat_r(void)
{
	return cop.cop_hit_val_stat;
}

static UINT16 cop_collision_status_val_r(offs_t offset)
{
	return cop.cop_hit_val[offset];
}

static void cop_collision_update_hitbox(UINT16 data, int slot, UINT32 hitadr)
{
	UINT32 hitadr2 = program_read_word(hitadr) | (cop.cop_hit_baseadr << 16);
	int num_axis = 2;
	int extraxor = 0;
	extraxor = 1;

	if (data & 0x0100) num_axis = 3;

	int i;

	for(i=0; i<3; i++) {
		cop.cop_collision_info[slot].dx[i] = 0;
		cop.cop_collision_info[slot].size[i] = 0;
	}

	for(i=0; i<num_axis; i++) {
		cop.cop_collision_info[slot].dx[i] = program_read_byte(extraxor^ (hitadr2++));
		cop.cop_collision_info[slot].size[i] = program_read_byte(extraxor^ (hitadr2++));
	}

	INT16 dx[3],size[3];

	for (i = 0; i < num_axis; i++)
	{
		size[i] = ((UINT8)(cop.cop_collision_info[slot].size[i]));
		dx[i] = ((INT8)(cop.cop_collision_info[slot].dx[i]));
	}

	int j = slot;

	UINT8 res;

	if (num_axis==3) res = 7;
	else res = 3;

	for (i = 0; i < num_axis;i++)
	{
		if (cop.cop_collision_info[j].allow_swap && (cop.cop_collision_info[j].flags_swap & (1 << i)))
		{
			cop.cop_collision_info[j].max[i] = (cop.cop_collision_info[j].pos[i]) - dx[i];
			cop.cop_collision_info[j].min[i] = cop.cop_collision_info[j].max[i] - size[i];
		}
		else
		{
			cop.cop_collision_info[j].min[i] = (cop.cop_collision_info[j].pos[i]) + dx[i];
			cop.cop_collision_info[j].max[i] = cop.cop_collision_info[j].min[i] + size[i];
		}

		if(cop.cop_collision_info[0].max[i] > cop.cop_collision_info[1].min[i] && cop.cop_collision_info[0].min[i] < cop.cop_collision_info[1].max[i])
			res &= ~(1 << i);

		if(cop.cop_collision_info[1].max[i] > cop.cop_collision_info[0].min[i] && cop.cop_collision_info[1].min[i] < cop.cop_collision_info[0].max[i])
			res &= ~(1 << i);

		cop.cop_hit_val[i] = (cop.cop_collision_info[0].pos[i] - cop.cop_collision_info[1].pos[i]);
	}

	cop.cop_hit_val_stat = res;
	cop.cop_hit_status = res;
}

static UINT16 cop_dist_r(void)
{
	return cop.cop_dist;
}

static void cop_dma_adr_rel_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_adr_rel);
}

static void cop_dma_dst_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_dst[cop.cop_dma_mode]);
}

static UINT16 cop_dma_mode_r(void)
{
	return cop.cop_dma_mode;
}

static void cop_dma_mode_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_mode);
	cop.cop_dma_mode &= 0x1ff;
}

static void cop_dma_size_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_size[cop.cop_dma_mode]);
}

static void cop_dma_src_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_src[cop.cop_dma_mode]);
}

static void cop_dma_trigger_w(UINT16 data)
{

	switch (cop.cop_dma_mode)
	{
		case 0x14:
		{
			dma_tilemap_buffer();
			break;
		}

		case 0x15:
		{
			dma_palette_buffer();
			break;
		}

		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		{
			dma_palette_brightness();
			break;
		}

	case 0x09:
	{
		UINT32 src, dst, size;
		int i;

		src = (cop.cop_dma_src[cop.cop_dma_mode] << 6);
		dst = (cop.cop_dma_dst[cop.cop_dma_mode] << 6);
		size = ((cop.cop_dma_size[cop.cop_dma_mode] << 5) - (cop.cop_dma_dst[cop.cop_dma_mode] << 6) + 0x20) / 2;

		for (i = 0; i < size; i++)
		{
			program_write_word(dst, program_read_word(src));
			src += 2;
			dst += 2;
		}

		break;
	}

	case 0x0e:
	{
		UINT32 src, dst, size, i;

		src = (cop.cop_dma_src[cop.cop_dma_mode] << 6);
		dst = (cop.cop_dma_dst[cop.cop_dma_mode] << 6);
		size = ((cop.cop_dma_size[cop.cop_dma_mode] << 5) - (cop.cop_dma_dst[cop.cop_dma_mode] << 6) + 0x20) / 2;

		for (i = 0; i < size; i++)
		{
			program_write_word(dst, program_read_word(src));
			src += 2;
			dst += 2;
		}

		break;
	}

	case 0x116:
	{
		UINT32 length, address;
		int i;

		address = (cop.cop_dma_src[cop.cop_dma_mode] << 6);
		length = ((cop.cop_dma_size[cop.cop_dma_mode] + 1) << 4);

		for (i = address; i < address + length; i += 4)
		{
			legioncop_write_dword(i, (cop.cop_dma_v1) | (cop.cop_dma_v2 << 16));
		}
		break;
	}

		case 0x118:
		case 0x119:
		case 0x11a:
		case 0x11b:
		case 0x11c:
		case 0x11d:
		case 0x11e:
		case 0x11f:
		{
			dma_fill();
			break;
		}
	}

}

static void cop_dma_v1_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_v1);
}

static void cop_dma_v2_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_dma_v2);
}

static void cop_hitbox_baseadr_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_hit_baseadr);
}

static UINT16 cop_itoa_digits_r(offs_t offset)
{
	return cop.cop_itoa_digits[offset*2] | (cop.cop_itoa_digits[offset*2+1] << 8);
}

static void cop_itoa_high_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	cop.cop_itoa = (cop.cop_itoa & ~(((UINT16)~mem_mask) << 16)) | ((data & ((UINT16)~mem_mask)) << 16);

	bcd_update();
}

static void cop_itoa_low_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	cop.cop_itoa = (cop.cop_itoa & ~((UINT32)(((UINT16)~mem_mask)))) | (data & ((UINT16)~mem_mask));

	bcd_update();
}

static void cop_itoa_mode_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{

	COMBINE_DATA(&cop.cop_itoa_mode);
}

static void cop_pal_brightness_mode_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.pal_brightness_mode);
}

static void cop_pal_brightness_val_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.pal_brightness_val);
}

static void cop_pgm_addr_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{

	assert(data < 0x100);
	cop.cop_latch_addr = data;
}

static void cop_pgm_data_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
 int idx = (cop.cop_latch_addr & 0xff) >> 3;
 cop.cop_program[cop.cop_latch_addr & 0xff] = data;
 cop.cop_func_trigger[idx] = cop.cop_latch_trigger;
 cop.cop_func_value[idx] = cop.cop_latch_value;
 cop.cop_func_mask[idx] = cop.cop_latch_mask;
}

static void cop_pgm_mask_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{

	cop.cop_latch_mask = data;
}

static void cop_pgm_trigger_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{

	cop.cop_latch_trigger = data;
}

static void cop_pgm_value_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{

	cop.cop_latch_value = data;
}

static void cop_precmd_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.m_cop_precmd);
}

static UINT16 cop_prng_maxvalue_r(void)
{
	return cop.m_cop_rng_max_value;
}

static void cop_prng_maxvalue_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.m_cop_rng_max_value);
}

static UINT16 cop_prng_r(void)
{
	return cpunum_gettotalcycles(0) % (cop.m_cop_rng_max_value + 1);
}

static UINT8 cop_read_byte(int address)
{
	return program_read_byte(address ^ 3);
}

static UINT16 cop_read_word(int address)
{
	return program_read_word(address ^ 2);
}

static UINT16 cop_reg_high_r(offs_t offset)
{
	return cop.cop_regs[offset] >> 16;
}

static void cop_reg_high_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	cop.cop_regs[offset] = (cop.cop_regs[offset] & ~(((UINT16)~mem_mask) << 16)) | ((data & ((UINT16)~mem_mask)) << 16);
}

static UINT16 cop_reg_low_r(offs_t offset)
{
	return cop.cop_regs[offset];
}

static void cop_reg_low_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	cop.cop_regs[offset] = (cop.cop_regs[offset] & ~((UINT32)(((UINT16)~mem_mask)))) | (data & ((UINT16)~mem_mask));
}

static void cop_rom_addr_hi_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.m_cop_rom_addr_hi);
}

static void cop_rom_addr_lo_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.m_cop_rom_addr_lo);
}

static void cop_scale_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.cop_scale);
	cop.cop_scale &= 3;
}

static void cop_sort_dma_trig_w(UINT16 data)
{
	dma_zsorting(data);
}

static void cop_sort_lookup_hi_w(UINT16 data)
{
	cop.cop_sort_lookup = (cop.cop_sort_lookup&0x0000ffff)|(data<<16);
}

static void cop_sort_lookup_lo_w(UINT16 data)
{
	cop.cop_sort_lookup = (cop.cop_sort_lookup&0xffff0000)|(data&0xffff);
}

static void cop_sort_param_w(UINT16 data)
{
	cop.cop_sort_param = data;
}

static void cop_sort_ram_addr_hi_w(UINT16 data)
{
	cop.cop_sort_ram_addr = (cop.cop_sort_ram_addr&0x0000ffff)|(data<<16);
}

static void cop_sort_ram_addr_lo_w(UINT16 data)
{
	cop.cop_sort_ram_addr = (cop.cop_sort_ram_addr&0xffff0000)|(data&0xffff);
}

static void cop_sprite_dma_abs_x_w(UINT16 data)
{
	cop.m_cop_sprite_dma_abs_x = data;
}

static void cop_sprite_dma_abs_y_w(UINT16 data)
{
	cop.m_cop_sprite_dma_abs_y = data;
}

static void cop_sprite_dma_inc_w(UINT16 data)
{
	if (!data)
	{

		if (cop.m_sprite_dma_x_clip >= -160 && cop.m_sprite_dma_x_clip < 320)
			cop.cop_regs[4] += 8;
		cop.m_cop_sprite_dma_src += 6;

		cop.m_cop_sprite_dma_size--;

		if (cop.m_cop_sprite_dma_size > 0)
			cop.cop_status &= ~2;
		else
			cop.cop_status |= 2;
	}
}

static void cop_sprite_dma_param_hi_w(UINT16 data)
{
	cop.m_cop_sprite_dma_param = (cop.m_cop_sprite_dma_param&0x0000ffff)|(data<<16);
}

static void cop_sprite_dma_param_lo_w(UINT16 data)
{
	cop.m_cop_sprite_dma_param = (cop.m_cop_sprite_dma_param&0xffff0000)|(data&0xffff);
}

static void cop_sprite_dma_size_w(UINT16 data)
{
	cop.m_cop_sprite_dma_size = data;
}

static void cop_sprite_dma_src_hi_w(UINT16 data)
{
	cop.m_cop_sprite_dma_src = (cop.m_cop_sprite_dma_src&0x0000ffff)|(data<<16);
}

static void cop_sprite_dma_src_lo_w(UINT16 data)
{
	cop.m_cop_sprite_dma_src = (cop.m_cop_sprite_dma_src&0xffff0000)|(data&0xffff);
}

static UINT16 cop_status_r(void)
{
	return cop.cop_status;
}

static void cop_unk_param_a_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.m_cop_unk_param_a);
}

static void cop_unk_param_b_w(offs_t offset, UINT16 data, UINT16 mem_mask)
{
	COMBINE_DATA(&cop.m_cop_unk_param_b);
}

static void cop_write_byte(int address, UINT8 data)
{
	program_write_byte(address ^ 3, data);
}

static void cop_write_word(int address, UINT16 data)
{
	program_write_word(address ^ 2, data);
}

static void dma_fill(void)
{
	UINT32 length, address;
	int i;
	if (cop.cop_dma_dst[cop.cop_dma_mode] != 0x0000)
		return;

	address = (cop.cop_dma_src[cop.cop_dma_mode] << 6);
	length = (cop.cop_dma_size[cop.cop_dma_mode] + 1) << 5;

	for (i = address; i < address + length; i += 4)
		legioncop_write_dword(i, (cop.cop_dma_v1) | (cop.cop_dma_v2 << 16));

}

static void dma_palette_brightness(void)
{
	UINT32 src, dst, size, i;

	src = (cop.cop_dma_src[cop.cop_dma_mode] << 6);
	dst = (cop.cop_dma_dst[cop.cop_dma_mode] << 6);
	size = ((cop.cop_dma_size[cop.cop_dma_mode] << 5) - (cop.cop_dma_dst[cop.cop_dma_mode] << 6) + 0x20) / 2;

	for (i = 0; i < size; i++)
	{
		UINT16 pal_val;
		int r, g, b;
		int rt, gt, bt;

		if (cop.pal_brightness_mode == 5)
		{
			UINT16 paldata = program_read_word(src);
			if (BIT(paldata, 15))
				pal_val = paldata;
			else
			{
				UINT16 targetpaldata = program_read_word(src + (cop.cop_dma_adr_rel * 0x400));
				bt = (targetpaldata & 0x7c00) >> 5;
				bt = fade_table(bt | (cop.pal_brightness_val ^ 0));
				b = (paldata & 0x7c00) >> 5;
				b = fade_table(b | (cop.pal_brightness_val ^ 0x1f));
				pal_val = ((b + bt) & 0x1f) << 10;
				gt = (targetpaldata & 0x03e0);
				gt = fade_table(gt | (cop.pal_brightness_val ^ 0));
				g = (paldata & 0x03e0);
				g = fade_table(g | (cop.pal_brightness_val ^ 0x1f));
				pal_val |= ((g + gt) & 0x1f) << 5;
				rt = (targetpaldata & 0x001f) << 5;
				rt = fade_table(rt | (cop.pal_brightness_val ^ 0));
				r = (paldata & 0x001f) << 5;
				r = fade_table(r | (cop.pal_brightness_val ^ 0x1f));
				pal_val |= ((r + rt) & 0x1f);
			}
		}
		else if (cop.pal_brightness_mode == 4)
		{

			UINT16 targetpaldata = program_read_word((src + (cop.cop_dma_adr_rel * 0x400)));
			UINT16 paldata = program_read_word(src);

			bt = (targetpaldata & 0x7c00) >> 10;
			b = (paldata & 0x7c00) >> 10;
			gt = (targetpaldata & 0x03e0) >> 5;
			g = (paldata & 0x03e0) >> 5;
			rt = (targetpaldata & 0x001f) >> 0;
			r = (paldata & 0x001f) >> 0;

			if (cop.pal_brightness_val == 0x10)
				pal_val = bt << 10 | gt << 5 | rt << 0;
			else if (cop.pal_brightness_val == 0xffff)
				pal_val = bt << 10 | gt << 5 | rt << 0;
			else
			{
				bt = fade_table(bt << 5 | ((cop.pal_brightness_val * 2) ^ 0));
				b = fade_table(b << 5 | ((cop.pal_brightness_val * 2) ^ 0x1f));
				pal_val = ((b + bt) & 0x1f) << 10;
				gt = fade_table(gt << 5 | ((cop.pal_brightness_val * 2) ^ 0));
				g = fade_table(g << 5 | ((cop.pal_brightness_val * 2) ^ 0x1f));
				pal_val |= ((g + gt) & 0x1f) << 5;
				rt = fade_table(rt << 5 | ((cop.pal_brightness_val * 2) ^ 0));
				r = fade_table(r << 5 | ((cop.pal_brightness_val * 2) ^ 0x1f));
				pal_val |= ((r + rt) & 0x1f);
			}
		}
		else
		{
			pal_val = program_read_word(src);
		}

		program_write_word(dst, pal_val);
		src += 2;
		dst += 2;
	}
}

static void dma_palette_buffer(void)
{
 int i;

	int src = cop.cop_dma_src[cop.cop_dma_mode] << 6;

	for (i = 0; i < 0x1000 / 2; i++)
	{
		UINT16 palval = program_read_word(src);
		src += 2;
		legionna_dma_palette_w(i, palval, 0);

	}
}

static void dma_tilemap_buffer(void)
{
 int i;

	int src = cop.cop_dma_src[cop.cop_dma_mode] << 6;
	if (src == 0xcfc0) src = 0xd000;

	for (i = 0; i < 0x2800 / 2; i++)
	{
		UINT16 tileval = program_read_word(src);
		src += 2;
		legionna_dma_video_w(i, tileval, 0);
	}
}

static void dma_zsorting(UINT16 data)
{
 int i, j;
 for (i = 1; i < data; i++) {
  UINT16 val = program_read_word(cop.cop_sort_lookup + i * 2);
  INT32 key = legioncop_read_dword(cop.cop_sort_ram_addr + val);
  for (j = i; j > 0; j--) {
   UINT16 prev = program_read_word(cop.cop_sort_lookup + (j - 1) * 2);
   INT32 pk = legioncop_read_dword(cop.cop_sort_ram_addr + prev);
   if (cop.cop_sort_param == 1 ? pk <= key : cop.cop_sort_param == 2 ? pk >= key : 1) break;
   program_write_word(cop.cop_sort_lookup + j * 2, prev);
  }
  program_write_word(cop.cop_sort_lookup + j * 2, val);
 }
}

static void execute_0205(int offset, UINT16 data)
{
	int ppos =        legioncop_read_dword(cop.cop_regs[0] + 0x04 + offset * 4);
	int npos = ppos + legioncop_read_dword(cop.cop_regs[0] + 0x10 + offset * 4);
	int delta = (npos >> 16) - (ppos >> 16);
	legioncop_write_dword(cop.cop_regs[0] + 4 + offset * 4, npos);

	cop_write_word(cop.cop_regs[0] + 0x1e + offset * 4, cop_read_word(cop.cop_regs[0] + 0x1e + offset * 4) + delta);
}

static void execute_0904(int offset, UINT16 data)
{
	if (data&0x0001)
		legioncop_write_dword(cop.cop_regs[0] + 16 + offset * 4, legioncop_read_dword(cop.cop_regs[0] + 16 + offset * 4) + legioncop_read_dword(cop.cop_regs[0] + 0x28 + offset * 4));
	else
		legioncop_write_dword(cop.cop_regs[0] + 16 + offset * 4, legioncop_read_dword(cop.cop_regs[0] + 16 + offset * 4) - legioncop_read_dword(cop.cop_regs[0] + 0x28 + offset * 4));
}

static void execute_130e(int offset, UINT16 data, int is_yflip)
{

	execute_338e(offset, data, is_yflip);
}

static void execute_338e(int offset, UINT16 data, int is_yflip)
{
	int dx = legioncop_read_dword(cop.cop_regs[1] + 4) - legioncop_read_dword(cop.cop_regs[0] + 4);
	int dy = legioncop_read_dword(cop.cop_regs[1] + 8) - legioncop_read_dword(cop.cop_regs[0] + 8);

	cop.cop_status = 7;

	if (!dy) {
		cop.cop_status |= 0x8000;
		cop.cop_angle = 0;
	}
	else {
		cop.cop_angle = (int)(atan(((double)(dx)) / ((double)(dy))) * 128 / M_PI);
		if (dy < 0)
			cop.cop_angle += 0x80;

		cop.cop_angle &= 0xff;
	}
	if (data & 0x0080) {

		if(is_yflip == 1)
			cop_write_byte(cop.cop_regs[0] + 0x34, cop.cop_angle ^ 0x80);
		else
			cop_write_byte(cop.cop_regs[0] + 0x34, cop.cop_angle);
	}
}

static void execute_3b30(int offset, UINT16 data)
{

	int dx, dy;

	dx = legioncop_read_dword(cop.cop_regs[1] + 4) - legioncop_read_dword(cop.cop_regs[0] + 4);
	dy = legioncop_read_dword(cop.cop_regs[1] + 8) - legioncop_read_dword(cop.cop_regs[0] + 8);

	dx = dx >> 16;
	dy = dy >> 16;
	cop.cop_dist = sqrt((double)(dx*dx + dy*dy));
	if (data & 0x0080)
		cop_write_word(cop.cop_regs[0] + (data & 0x200 ? 0x3a : 0x38), cop.cop_dist);
}

static void execute_42c2(int offset, UINT16 data)
{
	int div = cop_read_word(cop.cop_regs[0] + (0x36));
	if (!div)
	{
		cop.cop_status |= 0x8000;
		cop_write_word(cop.cop_regs[0] + (0x38), 0);
		return;
	}

	cop.cop_status = 7;
	cop_write_word(cop.cop_regs[0] + (0x38), (cop.cop_dist << (5 - cop.cop_scale)) / div);
}

static void execute_6200(int offset, UINT16 data)
{
	int primary_reg = 0;
	int primary_offset = 0x34;

	UINT8 angle = cop_read_byte(cop.cop_regs[primary_reg] + primary_offset);
	UINT16 flags = cop_read_word(cop.cop_regs[primary_reg]);
	cop.cop_angle_target &= 0xff;
	cop.cop_angle_step &= 0xff;
	flags &= ~0x0004;
	int delta = angle - cop.cop_angle_target;
	if (delta >= 128)
		delta -= 256;
	else if (delta < -128)
		delta += 256;
	if (delta < 0) {
		if (delta >= -cop.cop_angle_step) {
			angle = cop.cop_angle_target;
			flags |= 0x0004;
		}
		else
			angle += cop.cop_angle_step;
	}
	else {
		if (delta <= cop.cop_angle_step) {
			angle = cop.cop_angle_target;
			flags |= 0x0004;
		}
		else
			angle -= cop.cop_angle_step;
	}

	cop_write_word(cop.cop_regs[primary_reg], flags);

	if (!1)
		cop_write_byte(cop.cop_regs[primary_reg] + primary_offset, angle);
	else
		cop_write_word(cop.cop_regs[primary_reg] + primary_offset, angle);

}

static void execute_8100(int offset, UINT16 data)
{
	int raw_angle = (cop_read_word(cop.cop_regs[0] + (0x34)) & 0xff);
	double angle = raw_angle * M_PI / 128;
	double amp = (65536 >> 5)*(cop_read_word(cop.cop_regs[0] + (0x36)) & 0xff);
	int res;

	if (raw_angle == 0xc0)
		amp *= 2;
	res = (int)(amp*sin(angle)) * (1 << cop.cop_scale);
	legioncop_write_dword(cop.cop_regs[0] + 16, res);
}

static void execute_8900(int offset, UINT16 data)
{
	int raw_angle = (cop_read_word(cop.cop_regs[0] + (0x34)) & 0xff);
	double angle = raw_angle * M_PI / 128;
	double amp = (65536 >> 5)*(cop_read_word(cop.cop_regs[0] + (0x36)) & 0xff);
	int res;

	if (raw_angle == 0x80)
		amp *= 2;
	res = (int)(amp*cos(angle)) * (1 << cop.cop_scale);
	legioncop_write_dword(cop.cop_regs[0] + 20, res);
}

static void execute_a100(int offset, UINT16 data)
{
	cop_collision_read_pos(0, cop.cop_regs[0], data & 0x0080);
}

static void execute_a900(int offset, UINT16 data)
{
	cop_collision_read_pos(1, cop.cop_regs[1], data & 0x0080);
}

static void execute_b100(int offset, UINT16 data)
{
	cop_collision_update_hitbox(data, 0, cop.cop_regs[2]);
}

static void execute_b900(int offset, UINT16 data)
{
	cop_collision_update_hitbox(data, 1, cop.cop_regs[3]);
}

static void execute_f105(int offset, UINT16 data)
{

}

static UINT8 fade_table(int v)
{
	int low  = v & 0x001f;
	int high = v & 0x03e0;

	return (low * (high | (high >> 5)) + 0x210) >> 10;
}

static int find_trigger_match(UINT16 triggerval, UINT16 mask)
{
 int i;
 for (i = 0; i < 32; i++)
  if (cop.cop_func_trigger[i] && (triggerval & mask) == (cop.cop_func_trigger[i] & mask)) return i;
 return -1;
}

static UINT16 get_func_mask(int command)
{return cop.cop_func_mask[command];}

static UINT16 get_func_value(int command)
{return cop.cop_func_value[command];}
READ16_HANDLER( legioncop_r )
{
 offset = offset * 2 + 0x400;
 switch (offset) {
 case 0x42c: return cop_prng_maxvalue_r();
 case 0x47e: return cop_dma_mode_r();
 case 0x4a0: return cop_reg_high_r(0);
 case 0x4a2: return cop_reg_high_r(1);
 case 0x4a4: return cop_reg_high_r(2);
 case 0x4a6: return cop_reg_high_r(3);
 case 0x4a8: return cop_reg_high_r(4);
 case 0x4aa: return cop_reg_high_r(5);
 case 0x4ac: return cop_reg_high_r(6);
 case 0x4c0: return cop_reg_low_r(0);
 case 0x4c2: return cop_reg_low_r(1);
 case 0x4c4: return cop_reg_low_r(2);
 case 0x4c6: return cop_reg_low_r(3);
 case 0x4c8: return cop_reg_low_r(4);
 case 0x4ca: return cop_reg_low_r(5);
 case 0x4cc: return cop_reg_low_r(6);
 case 0x580: return cop_collision_status_r();
 case 0x582: return cop_collision_status_val_r(0);
 case 0x584: return cop_collision_status_val_r(1);
 case 0x586: return cop_collision_status_val_r(2);
 case 0x588: return cop_collision_status_stat_r();
 case 0x590: return cop_itoa_digits_r(0);
 case 0x592: return cop_itoa_digits_r(1);
 case 0x594: return cop_itoa_digits_r(2);
 case 0x596: return cop_itoa_digits_r(3);
 case 0x598: return cop_itoa_digits_r(4);
 case 0x5a0: return cop_prng_r();
 case 0x5a2: return cop_prng_r();
 case 0x5a4: return cop_prng_r();
 case 0x5a6: return cop_prng_r();
 case 0x5b0: return cop_status_r();
 case 0x5b2: return cop_dist_r();
 case 0x5b4: return cop_angle_r();
 }
 return 0xffff;
}

WRITE16_HANDLER( legioncop_w )
{
 offset = offset * 2 + 0x400;
 switch (offset) {
 case 0x400: cop_sprite_dma_param_lo_w(data); break;
 case 0x402: cop_sprite_dma_param_hi_w(data); break;
 case 0x40c: cop_sprite_dma_size_w(data); break;
 case 0x410: cop_sprite_dma_inc_w(data); break;
 case 0x412: cop_sprite_dma_src_hi_w(data); break;
 case 0x414: cop_sprite_dma_src_lo_w(data); break;
 case 0x41c: cop_angle_target_w(0, data, mem_mask); break;
 case 0x41e: cop_angle_step_w(0, data, mem_mask); break;
 case 0x420: cop_itoa_low_w(0, data, mem_mask); break;
 case 0x422: cop_itoa_high_w(0, data, mem_mask); break;
 case 0x424: cop_itoa_mode_w(0, data, mem_mask); break;
 case 0x428: cop_dma_v1_w(0, data, mem_mask); break;
 case 0x42a: cop_dma_v2_w(0, data, mem_mask); break;
 case 0x42c: cop_prng_maxvalue_w(0, data, mem_mask); break;
 case 0x432: cop_pgm_data_w(0, data, mem_mask); break;
 case 0x434: cop_pgm_addr_w(0, data, mem_mask); break;
 case 0x436: cop_hitbox_baseadr_w(0, data, mem_mask); break;
 case 0x438: cop_pgm_value_w(0, data, mem_mask); break;
 case 0x43a: cop_pgm_mask_w(0, data, mem_mask); break;
 case 0x43c: cop_pgm_trigger_w(0, data, mem_mask); break;
 case 0x440: cop_unk_param_a_w(0, data, mem_mask); break;
 case 0x442: cop_unk_param_b_w(0, data, mem_mask); break;
 case 0x444: cop_scale_w(0, data, mem_mask); break;
 case 0x446: cop_rom_addr_hi_w(0, data, mem_mask); break;
 case 0x448: cop_rom_addr_lo_w(0, data, mem_mask); break;
 case 0x44a: cop_precmd_w(0, data, mem_mask); break;
 case 0x450: cop_sort_ram_addr_hi_w(data); break;
 case 0x452: cop_sort_ram_addr_lo_w(data); break;
 case 0x454: cop_sort_lookup_hi_w(data); break;
 case 0x456: cop_sort_lookup_lo_w(data); break;
 case 0x458: cop_sort_param_w(data); break;
 case 0x45a: cop_pal_brightness_val_w(0, data, mem_mask); break;
 case 0x45c: cop_pal_brightness_mode_w(0, data, mem_mask); break;
 case 0x476: cop_dma_adr_rel_w(0, data, mem_mask); break;
 case 0x478: cop_dma_src_w(0, data, mem_mask); break;
 case 0x47a: cop_dma_size_w(0, data, mem_mask); break;
 case 0x47c: cop_dma_dst_w(0, data, mem_mask); break;
 case 0x47e: cop_dma_mode_w(0, data, mem_mask); break;
 case 0x48c: cop_sprite_dma_abs_y_w(data); break;
 case 0x48e: cop_sprite_dma_abs_x_w(data); break;
 case 0x4a0: cop_reg_high_w(0, data, mem_mask); break;
 case 0x4a2: cop_reg_high_w(1, data, mem_mask); break;
 case 0x4a4: cop_reg_high_w(2, data, mem_mask); break;
 case 0x4a6: cop_reg_high_w(3, data, mem_mask); break;
 case 0x4a8: cop_reg_high_w(4, data, mem_mask); break;
 case 0x4aa: cop_reg_high_w(5, data, mem_mask); break;
 case 0x4ac: cop_reg_high_w(6, data, mem_mask); break;
 case 0x4c0: cop_reg_low_w(0, data, mem_mask); break;
 case 0x4c2: cop_reg_low_w(1, data, mem_mask); break;
 case 0x4c4: cop_reg_low_w(2, data, mem_mask); break;
 case 0x4c6: cop_reg_low_w(3, data, mem_mask); break;
 case 0x4c8: cop_reg_low_w(4, data, mem_mask); break;
 case 0x4ca: cop_reg_low_w(5, data, mem_mask); break;
 case 0x4cc: cop_reg_low_w(6, data, mem_mask); break;
 case 0x500: LEGACY_cop_cmd_w(0, data); break;
 case 0x502: LEGACY_cop_cmd_w(1, data); break;
 case 0x504: LEGACY_cop_cmd_w(2, data); break;
 case 0x6fc: cop_dma_trigger_w(data); break;
 case 0x6fe: cop_sort_dma_trig_w(data); break;
 }
}

void legioncop_reset(void)
{
 memset(&cop, 0, sizeof(cop));
 cop.cop_itoa_mode = 3;
}
