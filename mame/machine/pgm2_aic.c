// license:BSD-3-Clause
// copyright-holders:David Haywood, MetalliC
/* Atmel ARM AIC, converted from MAME 0.287 to C. */
#include "driver.h"
#include "pgm2_aic.h"
static UINT32 enabled, pending, vector, status, core_status, spurious, debug, fast;
static UINT32 smr[32], svr[32];
static INT32 levels[9];
static int level_index;
static void set_lines(void)
{
	cpunum_set_input_line(0, 0, (core_status & ~debug & 2) ? ASSERT_LINE : CLEAR_LINE);
}
static void check_irqs(void)
{
	UINT32 mask = enabled & pending & ~fast;
	int i;
	core_status = 0;
	for (i=0; i<32; i++)
		if ((mask & (1U << i)) && (int)(smr[i] & 7) > levels[level_index]) {
			core_status |= 2; break;
		}
	if (enabled & pending & fast) core_status |= 1;
	set_lines();
}
void pgm2_aic_reset(void)
{
	enabled = pending = vector = status = core_status = spurious = debug = 0;
	fast = 1;
	memset(smr, 0, sizeof(smr)); memset(svr, 0, sizeof(svr));
	memset(levels, 0, sizeof(levels)); level_index = 0; levels[0] = -1;
	set_lines();
}
void pgm2_aic_init(void)
{
	state_save_register_global(enabled); state_save_register_global(pending);
	state_save_register_global(vector); state_save_register_global(status);
	state_save_register_global(core_status); state_save_register_global(spurious);
	state_save_register_global(debug); state_save_register_global(fast);
	state_save_register_global(level_index); state_save_register_global_array(levels);
	state_save_register_global_array(smr); state_save_register_global_array(svr);
	state_save_register_func_postload(check_irqs);
}
void pgm2_aic_irq(int line, int state)
{
	if (state) pending |= 1U << line;
	else if (smr[line] & 0x40) pending &= ~(1U << line);
	check_irqs();
}
READ32_HANDLER(pgm2_aic_r)
{
	if (offset < 0x80/4) return smr[offset];
	if (offset < 0x100/4) return svr[offset - 0x80/4];
	switch (offset*4) {
	case 0x100: {
		UINT32 mask = enabled & pending & ~fast;
		int i, selected = -1, pri = levels[level_index];
		vector = spurious;
		for (i=31; i>=0; i--) if ((mask & (1U << i)) && (int)(smr[i] & 7) >= pri) {
			selected = i; pri = smr[i] & 7;
		}
		if (selected > 0) {
			status = selected; vector = svr[selected];
			if (level_index < 8) levels[++level_index] = pri;
			if (smr[selected] & 0x20) pending &= ~(1U << selected);
		}
		core_status &= ~2; set_lines(); return vector;
	}
	case 0x104: return (enabled & pending & fast) ? svr[0] : spurious;
	case 0x108: return status;
	case 0x10c: return pending;
	case 0x110: return enabled;
	case 0x114: return core_status;
	case 0x148: return fast;
	}
	return 0;
}
WRITE32_HANDLER(pgm2_aic_w)
{
	UINT32 bits = data & ~mem_mask;
	if (offset < 0x80/4) { COMBINE_DATA(&smr[offset]); return; }
	if (offset < 0x100/4) { COMBINE_DATA(&svr[offset - 0x80/4]); return; }
	switch (offset*4) {
	case 0x120: enabled |= bits; break;
	case 0x124: enabled &= ~bits; break;
	case 0x128: pending &= ~bits; break;
	case 0x12c: pending |= bits; break;
	case 0x130: status = 0; if (level_index) --level_index; break;
	case 0x134: COMBINE_DATA(&spurious); break;
	case 0x138: COMBINE_DATA(&debug); break;
	case 0x140: fast |= bits; break;
	case 0x144: fast &= ~bits | 1; break;
	}
	check_irqs();
}
