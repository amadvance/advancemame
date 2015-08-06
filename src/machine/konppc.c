/* Konami PowerPC-based 3D games common functions */

#include "driver.h"
#include "cpu/sharc/sharc.h"
#include "konppc.h"

#define MAX_CG_BOARDS	2

static UINT32 dsp_comm_ppc[MAX_CG_BOARDS][2];
static UINT32 dsp_comm_sharc[MAX_CG_BOARDS][2];
static int dsp_shared_ram_bank[MAX_CG_BOARDS];

static int cgboard_id;
static int cgboard_texture_bank = -1;
static int cgboard_type;

static UINT32 *dsp_shared_ram[MAX_CG_BOARDS];

/*****************************************************************************/

void init_konami_cgboard(int board_id, int type)
{
	dsp_comm_ppc[board_id][0] = 0x00;
	dsp_shared_ram[board_id] = auto_malloc(0x20000);
	dsp_shared_ram_bank[board_id] = 0;
	cgboard_type = type;
}

void set_cgboard_id(int board_id)
{
	cgboard_id = board_id;
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
	return dsp_comm_sharc[cgboard_id][offset];
}

WRITE32_HANDLER( cgboard_dsp_comm_w_ppc )
{
//  printf("dsp_cmd_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());

	if (cgboard_id == 0)
	{
		if (offset == 0)
		{
			if (!(mem_mask & 0xff000000))
			{
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

				dsp_shared_ram_bank[cgboard_id] = (data >> 24) & 0x1;
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

				dsp_shared_ram_bank[cgboard_id] = (data >> 24) & 0x1;
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
	return dsp_shared_ram[cgboard_id][offset + (dsp_shared_ram_bank[cgboard_id] * 0x4000)];
}

WRITE32_HANDLER( cgboard_dsp_shared_w_ppc )
{
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

	if (cgboard_type == CGBOARD_TYPE_GTICLUB)
	{
		cpunum_set_input_line(2, SHARC_INPUT_FLAG0, ASSERT_LINE);
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
