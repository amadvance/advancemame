// license:BSD-3-Clause
// copyright-holders:Ville Linde
/* Konami PowerPC-based 3D games common functions */

#include "driver.h"
#include "cpu/sharc/sharc.h"
#include "konppc.h"
#include "vidhrdw/voodoo.h"

#define MAX_CG_BOARDS	2

static UINT32 dsp_comm_ppc[MAX_CG_BOARDS][2];
static UINT32 dsp_comm_sharc[MAX_CG_BOARDS][2];
static int dsp_shared_ram_bank[MAX_CG_BOARDS];
static UINT32 dsp_state[MAX_CG_BOARDS];
static int enable_3d[MAX_CG_BOARDS];

static int cgboard_id;
static int cgboard_texture_bank = -1;
static int cgboard_type;
static int num_cgboards;

static UINT32 *dsp_shared_ram[MAX_CG_BOARDS];

/* NWK-TR and Hang Pilot CG boards */
static UINT32 nwk_device_sel[MAX_CG_BOARDS];
static int nwk_fifo_read_ptr[MAX_CG_BOARDS];
static int nwk_fifo_write_ptr[MAX_CG_BOARDS];
static UINT32 *nwk_fifo[MAX_CG_BOARDS];
static UINT32 *nwk_ram[MAX_CG_BOARDS];
static int nwk_fifo_half_full_r;
static int nwk_fifo_half_full_w;
static int nwk_fifo_full;
static int nwk_fifo_mask;

/* K033906 PCI bridge */
static int k033906_reg_set[MAX_CG_BOARDS];
static UINT32 *k033906_reg[MAX_CG_BOARDS];
static UINT32 *k033906_ram[MAX_CG_BOARDS];

/*****************************************************************************/

void init_konami_cgboard(int board_id, int type)
{
	dsp_comm_ppc[board_id][0] = 0x00;
	dsp_shared_ram[board_id] = auto_malloc(0x20000);
	memset(dsp_shared_ram[board_id], 0, 0x20000);
	dsp_shared_ram_bank[board_id] = 0;
	dsp_state[board_id] = 0x80;
	enable_3d[board_id] = 0;
	cgboard_type = type;
	if (board_id >= num_cgboards)
		num_cgboards = board_id + 1;

	if (type == CGBOARD_TYPE_HANGPLT)
	{
		nwk_device_sel[board_id] = 0;
		nwk_fifo_read_ptr[board_id] = 0;
		nwk_fifo_write_ptr[board_id] = 0;
		nwk_fifo[board_id] = auto_malloc(0x800 * sizeof(UINT32));
		nwk_ram[board_id] = auto_malloc(0x2000 * sizeof(UINT32));
		memset(nwk_fifo[board_id], 0, 0x800 * sizeof(UINT32));
		memset(nwk_ram[board_id], 0, 0x2000 * sizeof(UINT32));

		nwk_fifo_half_full_r = 0x3ff;
		nwk_fifo_half_full_w = 0x400;
		nwk_fifo_full = 0x7ff;
		nwk_fifo_mask = 0x7ff;

		k033906_reg_set[board_id] = 0;
		k033906_reg[board_id] = auto_malloc(0x100 * sizeof(UINT32));
		k033906_ram[board_id] = auto_malloc(0x8000 * sizeof(UINT32));
		memset(k033906_reg[board_id], 0, 0x100 * sizeof(UINT32));
		memset(k033906_ram[board_id], 0, 0x8000 * sizeof(UINT32));
	}
}

int cgboard_output_3d_enabled(void)
{
	if (cgboard_id >= num_cgboards)
		return 0;
	return enable_3d[cgboard_id];
}

void set_cgboard_id(int board_id)
{
	/* the PowerPC can select a board that isn't installed, the accesses to it are ignored */
	cgboard_id = board_id;
}

int get_cgboard_id(void)
{
	return (cgboard_id >= num_cgboards) ? 0 : cgboard_id;
}

void set_cgboard_texture_bank(int bank)
{
	cgboard_texture_bank = bank;
}

/*****************************************************************************/

/* CG Board DSP interface for PowerPC */

READ32_HANDLER( cgboard_dsp_comm_r_ppc )
{
//  printf("dsp_cmd_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
	if (cgboard_id >= num_cgboards)
		return 0;
	return dsp_comm_sharc[cgboard_id][offset] | (dsp_state[cgboard_id] << 16);
}

WRITE32_HANDLER( cgboard_dsp_comm_w_ppc )
{
//  printf("dsp_cmd_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());

	if (cgboard_id >= num_cgboards)
		return;

	if (cgboard_id == 0)
	{
		if (offset == 0)
		{
			if (!(mem_mask & 0xff000000))
			{
				/* the bank must be selected before the DSP is released, as it boots from the shared RAM */
				dsp_shared_ram_bank[cgboard_id] = (data >> 24) & 0x1;

				if (data & 0x80000000)
					dsp_state[cgboard_id] |= 0x10;

				k033906_reg_set[cgboard_id] = (data >> 29) & 1;

				if (data & 0x10000000)
				{
					cpunum_set_input_line(2, INPUT_LINE_RESET, CLEAR_LINE);
					cpu_spinuntil_time(TIME_IN_USEC(1000));		// Give the SHARC enough time to boot itself
				}
				else
				{
					cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
				}

				if (data & 0x02000000)
				{
					cpunum_set_input_line(2, INPUT_LINE_IRQ0, ASSERT_LINE);
					cpu_spinuntil_time(TIME_IN_USEC(1000));		// Give the SHARC enough time to respond
				}
				if (data & 0x04000000)
				{
					cpunum_set_input_line(2, INPUT_LINE_IRQ1, ASSERT_LINE);
					cpu_spinuntil_time(TIME_IN_USEC(1000));		// Give the SHARC enough time to respond
				}

			}

			if (!(mem_mask & 0x00ff0000))
			{
				enable_3d[cgboard_id] = (data >> 22) & 1;
			}

			if (!(mem_mask & 0x000000ff))
			{
				dsp_comm_ppc[cgboard_id][offset] = data & 0xff;
				cpu_spinuntil_time(TIME_IN_USEC(500));			// Give the SHARC enough time to respond
			}
		}
		else
		{
			dsp_comm_ppc[cgboard_id][offset] = data;
		}
	}
	else if (cgboard_id == 1)
	{
		if (offset == 0)
		{
			if (!(mem_mask & 0xff000000))
			{
				dsp_shared_ram_bank[cgboard_id] = (data >> 24) & 0x1;

				if (data & 0x80000000)
					dsp_state[cgboard_id] |= 0x10;

				k033906_reg_set[cgboard_id] = (data >> 29) & 1;

				if (data & 0x10000000)
				{
					cpunum_set_input_line(3, INPUT_LINE_RESET, CLEAR_LINE);
					cpu_spinuntil_time(TIME_IN_USEC(1000));		// Give the SHARC enough time to boot itself
				}
				else
				{
					cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);
				}

				if (data & 0x02000000)
				{
					cpunum_set_input_line(3, INPUT_LINE_IRQ0, ASSERT_LINE);
					cpu_spinuntil_time(TIME_IN_USEC(1000));		// Give the SHARC enough time to respond
				}
				if (data & 0x04000000)
				{
					cpunum_set_input_line(3, INPUT_LINE_IRQ1, ASSERT_LINE);
					cpu_spinuntil_time(TIME_IN_USEC(1000));		// Give the SHARC enough time to respond
				}

			}

			if (!(mem_mask & 0x00ff0000))
			{
				enable_3d[cgboard_id] = (data >> 22) & 1;
			}

			if (!(mem_mask & 0x000000ff))
			{
				dsp_comm_ppc[cgboard_id][offset] = data & 0xff;
				cpu_spinuntil_time(TIME_IN_USEC(500));			// Give the SHARC enough time to respond
			}
		}
		else
		{
			dsp_comm_ppc[cgboard_id][offset] = data;
		}
	}
}



READ32_HANDLER( cgboard_dsp_shared_r_ppc )
{
	if (cgboard_id >= num_cgboards)
		return 0;
	return dsp_shared_ram[cgboard_id][offset + (dsp_shared_ram_bank[cgboard_id] * 0x4000)];
}

WRITE32_HANDLER( cgboard_dsp_shared_w_ppc )
{
	if (cgboard_id >= num_cgboards)
		return;
	cpu_trigger(10000);		// Remove the timeout (a part of the GTI Club FIFO test workaround)
	COMBINE_DATA(dsp_shared_ram[cgboard_id] + (offset + (dsp_shared_ram_bank[cgboard_id] * 0x4000)));
}

/*****************************************************************************/

/* CG Board DSP interface for SHARC */

READ32_HANDLER( cgboard_dsp_comm_r_sharc )
{
	return dsp_comm_ppc[cgboard_id][offset];
}

WRITE32_HANDLER( cgboard_dsp_comm_w_sharc )
{
	if (offset >= 2)
	{
		fatalerror("dsp_comm_w: %08X, %08X", data, offset);
	}

	if (offset == 1)
	{
		if (cgboard_texture_bank != -1)
		{
			if (data & 0x08)
			{
				memory_set_bankptr(cgboard_texture_bank, memory_region(REGION_USER5) + 0x800000);
			}
			else
			{
				memory_set_bankptr(cgboard_texture_bank, memory_region(REGION_USER5));
			}
		}
	}

	if (cgboard_type == CGBOARD_TYPE_GTICLUB || cgboard_type == CGBOARD_TYPE_ZR107)
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG0, ASSERT_LINE);

		if (offset == 1 && (data & 0x03))
		{
			cpunum_set_input_line(2, INPUT_LINE_IRQ2, ASSERT_LINE);
			activecpu_abort_timeslice();
		}
	}

//  printf("cgboard_dsp_comm_w_sharc: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
	dsp_comm_sharc[cgboard_id][offset] = data;
}



READ32_HANDLER( cgboard_dsp_shared_r_sharc )
{
//  printf("dsp_shared_r: %08X, (%08X, %08X)\n", offset, dsp_shared_ram[(offset >> 1)], dsp_shared_ram[offset]);

	if (offset & 0x1)
	{
		return (dsp_shared_ram[cgboard_id][(offset >> 1) + ((dsp_shared_ram_bank[cgboard_id] ^ 1) * 0x4000)] >> 0) & 0xffff;
	}
	else
	{
		return (dsp_shared_ram[cgboard_id][(offset >> 1) + ((dsp_shared_ram_bank[cgboard_id] ^ 1) * 0x4000)] >> 16) & 0xffff;
	}
}

WRITE32_HANDLER( cgboard_dsp_shared_w_sharc )
{
//  printf("dsp_shared_w: %08X, %08X\n", offset, data);
	if (offset & 0x1)
	{
		dsp_shared_ram[cgboard_id][(offset >> 1) + ((dsp_shared_ram_bank[cgboard_id] ^ 1) * 0x4000)] &= 0xffff0000;
		dsp_shared_ram[cgboard_id][(offset >> 1) + ((dsp_shared_ram_bank[cgboard_id] ^ 1) * 0x4000)] |= (data & 0xffff);
	}
	else
	{
		dsp_shared_ram[cgboard_id][(offset >> 1) + ((dsp_shared_ram_bank[cgboard_id] ^ 1) * 0x4000)] &= 0x0000ffff;
		dsp_shared_ram[cgboard_id][(offset >> 1) + ((dsp_shared_ram_bank[cgboard_id] ^ 1) * 0x4000)] |= ((data & 0xffff) << 16);
	}
}

/*****************************************************************************/

/* CG Board DSP interface for the SHARC of a specific board */

static UINT32 board_comm_sharc_r(int board, offs_t offset)
{
	return dsp_comm_ppc[board][offset];
}

static void board_comm_sharc_w(int board, offs_t offset, UINT32 data)
{
	if (offset >= 2)
	{
		fatalerror("dsp_comm_w: %08X, %08X", data, offset);
	}

	if (offset == 1)
	{
		nwk_device_sel[board] = data;

		if ((data & 0x01) || (data & 0x10))
			cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG1, ASSERT_LINE);

		if (cgboard_texture_bank != -1)
			memory_set_bankptr(cgboard_texture_bank + board, memory_region(REGION_USER5) + ((data & 0x08) ? 0x800000 : 0));
	}

	dsp_comm_sharc[board][offset] = data;
}

static UINT32 board_shared_sharc_r(int board, offs_t offset)
{
	UINT32 value = dsp_shared_ram[board][(offset >> 1) + ((dsp_shared_ram_bank[board] ^ 1) * 0x4000)];

	return (offset & 1) ? (value & 0xffff) : (value >> 16);
}

static void board_shared_sharc_w(int board, offs_t offset, UINT32 data)
{
	UINT32 *ram = &dsp_shared_ram[board][(offset >> 1) + ((dsp_shared_ram_bank[board] ^ 1) * 0x4000)];

	if (offset & 1)
		*ram = (*ram & 0xffff0000) | (data & 0xffff);
	else
		*ram = (*ram & 0x0000ffff) | ((data & 0xffff) << 16);
}

static UINT32 nwk_fifo_r(int board)
{
	UINT32 data;

	if (nwk_fifo_read_ptr[board] < nwk_fifo_half_full_r)
		cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG1, CLEAR_LINE);
	else
		cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG1, ASSERT_LINE);

	if (nwk_fifo_read_ptr[board] < nwk_fifo_full)
		cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG2, ASSERT_LINE);
	else
		cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG2, CLEAR_LINE);

	data = nwk_fifo[board][nwk_fifo_read_ptr[board]];
	nwk_fifo_read_ptr[board] = (nwk_fifo_read_ptr[board] + 1) & nwk_fifo_mask;

	return data;
}

static void nwk_fifo_w(int board, UINT32 data)
{
	if (nwk_fifo_write_ptr[board] < nwk_fifo_half_full_w)
		cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG1, ASSERT_LINE);
	else
		cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG1, CLEAR_LINE);

	cpunum_set_input_line(2 + board, SHARC_INPUT_FLAG2, ASSERT_LINE);

	nwk_fifo[board][nwk_fifo_write_ptr[board]] = data;
	nwk_fifo_write_ptr[board] = (nwk_fifo_write_ptr[board] + 1) & nwk_fifo_mask;
}

static UINT32 k033906_reg_r(int board, int reg)
{
	switch (reg)
	{
		case 0x00:	return 0x0001121a;			/* PCI Vendor ID (0x121a = 3dfx), Device ID (0x0001 = Voodoo 1) */
		case 0x02:	return 0x04000000;			/* Revision ID */
		case 0x04:	return k033906_reg[board][0x04];	/* memBaseAddr */
		case 0x0f:	return k033906_reg[board][0x0f];	/* interrupt_line, interrupt_pin, min_gnt, max_lat */
		case 0x14:	return k033906_reg[board][0x14];
	}

	fatalerror("k033906_reg_r: %08X", reg);
	return 0;
}

static void k033906_reg_w(int board, int reg, UINT32 data)
{
	switch (reg)
	{
		case 0x00:
		case 0x01:	/* command register */
		case 0x11:	/* busSnoop0 */
		case 0x12:	/* busSnoop1 */
		case 0x38:
			break;

		case 0x04:	/* memBaseAddr */
			k033906_reg[board][0x04] = (data == 0xffffffff) ? 0xff000000 : (data & 0xff000000);
			break;

		case 0x0f:	/* interrupt_line, interrupt_pin, min_gnt, max_lat */
		case 0x14:
			k033906_reg[board][reg] = data;
			break;

		case 0x10:	/* initEnable */
			voodoo_set_init_enable(board, data);
			break;

		default:
			fatalerror("k033906_reg_w: %08X, %08X", data, reg);
	}
}

static UINT32 board_k033906_r(int board, offs_t offset)
{
	if (nwk_device_sel[board] & 0x01)
		return nwk_fifo_r(board);

	if (k033906_reg_set[board])
		return k033906_reg_r(board, offset);

	return k033906_ram[board][offset & 0x7fff];
}

static void board_k033906_w(int board, offs_t offset, UINT32 data)
{
	if (k033906_reg_set[board])
		k033906_reg_w(board, offset, data);
	else
		k033906_ram[board][offset & 0x7fff] = data;
}

static UINT32 board_nwk_voodoo_r(int board, offs_t offset, UINT32 mem_mask)
{
	if (nwk_device_sel[board] == 0x04 && offset >= 0x100000 && offset < 0x200000)
		return nwk_ram[board][offset & 0x1fff];

	return board ? voodoo_1_r(offset, mem_mask) : voodoo_0_r(offset, mem_mask);
}

static void board_nwk_voodoo_fifo_w(int board, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	if (nwk_device_sel[board] & 0x01)
	{
		nwk_fifo_w(board, data);
	}
	else if (nwk_device_sel[board] & 0x02)
	{
		nwk_ram[board][(((offset >> 8) << 9) | (offset & 0xff)) & 0x1fff] = data;
	}
	else if (board)
	{
		voodoo_1_w(offset ^ 0x80000, data, mem_mask);
	}
	else
	{
		voodoo_0_w(offset ^ 0x80000, data, mem_mask);
	}
}

READ32_HANDLER( cgboard_0_comm_sharc_r ) { return board_comm_sharc_r(0, offset); }
WRITE32_HANDLER( cgboard_0_comm_sharc_w ) { board_comm_sharc_w(0, offset, data); }
READ32_HANDLER( cgboard_0_shared_sharc_r ) { return board_shared_sharc_r(0, offset); }
WRITE32_HANDLER( cgboard_0_shared_sharc_w ) { board_shared_sharc_w(0, offset, data); }
READ32_HANDLER( cgboard_0_k033906_r ) { return board_k033906_r(0, offset); }
WRITE32_HANDLER( cgboard_0_k033906_w ) { board_k033906_w(0, offset, data); }
READ32_HANDLER( nwk_voodoo_0_r ) { return board_nwk_voodoo_r(0, offset, mem_mask); }
WRITE32_HANDLER( nwk_voodoo_fifo_0_w ) { board_nwk_voodoo_fifo_w(0, offset, data, mem_mask); }

READ32_HANDLER( cgboard_1_comm_sharc_r ) { return board_comm_sharc_r(1, offset); }
WRITE32_HANDLER( cgboard_1_comm_sharc_w ) { board_comm_sharc_w(1, offset, data); }
READ32_HANDLER( cgboard_1_shared_sharc_r ) { return board_shared_sharc_r(1, offset); }
WRITE32_HANDLER( cgboard_1_shared_sharc_w ) { board_shared_sharc_w(1, offset, data); }
READ32_HANDLER( cgboard_1_k033906_r ) { return board_k033906_r(1, offset); }
WRITE32_HANDLER( cgboard_1_k033906_w ) { board_k033906_w(1, offset, data); }
READ32_HANDLER( nwk_voodoo_1_r ) { return board_nwk_voodoo_r(1, offset, mem_mask); }
WRITE32_HANDLER( nwk_voodoo_fifo_1_w ) { board_nwk_voodoo_fifo_w(1, offset, data, mem_mask); }

/*****************************************************************************/

#define LED_ON		0xff00ff00

void draw_7segment_led(mame_bitmap *bitmap, int x, int y, UINT8 value)
{
	if ((value & 0x7f) == 0x7f)
	{
		return;
	}

	plot_box(bitmap, x-1, y-1, 7, 11, 0x00000000);

	/* Top */
	if( (value & 0x40) == 0 ) {
		plot_box(bitmap, x+1, y+0, 3, 1, LED_ON);
	}
	/* Middle */
	if( (value & 0x01) == 0 ) {
		plot_box(bitmap, x+1, y+4, 3, 1, LED_ON);
	}
	/* Bottom */
	if( (value & 0x08) == 0 ) {
		plot_box(bitmap, x+1, y+8, 3, 1, LED_ON);
	}
	/* Top Left */
	if( (value & 0x02) == 0 ) {
		plot_box(bitmap, x+0, y+1, 1, 3, LED_ON);
	}
	/* Top Right */
	if( (value & 0x20) == 0 ) {
		plot_box(bitmap, x+4, y+1, 1, 3, LED_ON);
	}
	/* Bottom Left */
	if( (value & 0x04) == 0 ) {
		plot_box(bitmap, x+0, y+5, 1, 3, LED_ON);
	}
	/* Bottom Right */
	if( (value & 0x10) == 0 ) {
		plot_box(bitmap, x+4, y+5, 1, 3, LED_ON);
	}
}
