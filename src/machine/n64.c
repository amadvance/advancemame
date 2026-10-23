#include "driver.h"
#include "cpu/mips/mips3.h"
#include "cpu/rsp/rsp.h"
#include "includes/n64.h"
#include "sound/dmadac.h"

#ifdef LSB_FIRST
	#define BYTE_ADDR_XOR		3
	#define WORD_ADDR_XOR		1
#else
	#define BYTE_ADDR_XOR		0
	#define WORD_ADDR_XOR		0
#endif

UINT32 *rdram;
UINT32 *rsp_imem;
UINT32 *rsp_dmem;

static UINT32 mi_version;
static UINT32 mi_interrupt = 0;
static UINT32 mi_intr_mask = 0;

extern int fb_width;

static void n64_check_interrupts(void)
{
	cpunum_set_input_line(0, INPUT_LINE_IRQ0,
		(mi_interrupt & mi_intr_mask) ? ASSERT_LINE : CLEAR_LINE);
}

void signal_rcp_interrupt(int interrupt)
{
	mi_interrupt |= interrupt;
	n64_check_interrupts();
}

void clear_rcp_interrupt(int interrupt)
{
	mi_interrupt &= ~interrupt;
	n64_check_interrupts();
}

READ32_HANDLER( n64_mi_reg_r )
{
	switch (offset)
	{
		case 0x04/4:
			return mi_version;

		case 0x08/4:
			return mi_interrupt;

		case 0x0c/4:
			return mi_intr_mask;

		default:
			logerror("mi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_mi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:
			if (data & 0x0800)
			{
				clear_rcp_interrupt(DP_INTERRUPT);
			}
			break;

		case 0x04/4:
			mi_version = data;
			break;

		case 0x0c/4:
		{
			if (data & 0x0001) mi_intr_mask &= ~0x1;
			if (data & 0x0002) mi_intr_mask |= 0x1;
			if (data & 0x0004) mi_intr_mask &= ~0x2;
			if (data & 0x0008) mi_intr_mask |= 0x2;
			if (data & 0x0010) mi_intr_mask &= ~0x4;
			if (data & 0x0020) mi_intr_mask |= 0x4;
			if (data & 0x0040) mi_intr_mask &= ~0x8;
			if (data & 0x0080) mi_intr_mask |= 0x8;
			if (data & 0x0100) mi_intr_mask &= ~0x10;
			if (data & 0x0200) mi_intr_mask |= 0x10;
			if (data & 0x0400) mi_intr_mask &= ~0x20;
			if (data & 0x0800) mi_intr_mask |= 0x20;
			n64_check_interrupts();
			break;
		}

		default:
			logerror("mi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

static UINT32 sp_mem_addr;
static UINT32 sp_dram_addr;
static int sp_dma_length;
static int sp_dma_count;
static int sp_dma_skip;

static UINT32 sp_semaphore;

static void sp_dma(int direction)
{
	UINT8 *src, *dst;
	int i;

	if (sp_dma_length == 0)
	{
		return;
	}

	sp_dma_length++;

	if ((sp_dma_length & 3) != 0)
	{

		sp_dma_length = sp_dma_length & ~3;

	}

	if (sp_mem_addr & 0x3)
	{

        sp_mem_addr = sp_mem_addr & ~3;

	}
	if (sp_dram_addr & 0x3)
	{

        sp_dram_addr = sp_dram_addr & ~3;

	}

	if (sp_dma_count > 0)
	{
		fatalerror("sp_dma: dma_count = %d\n", sp_dma_count);
	}
	if (sp_dma_skip > 0)
	{
		fatalerror("sp_dma: dma_skip = %d\n", sp_dma_skip);
	}

	if ((sp_mem_addr & 0xfff) + (sp_dma_length) > 0x1000)
	{
		fatalerror("sp_dma: dma out of memory area: %08X, %08X\n", sp_mem_addr, sp_dma_length);
	}

	if (direction == 0)
	{
		src = (UINT8*)&rdram[sp_dram_addr / 4];
		dst = (sp_mem_addr & 0x1000) ? (UINT8*)&rsp_imem[(sp_mem_addr & 0xfff) / 4] : (UINT8*)&rsp_dmem[(sp_mem_addr & 0xfff) / 4];

		for (i=0; i < sp_dma_length; i++)
		{
			dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(i)];
		}

        sp_mem_addr += sp_dma_length;
        sp_dram_addr += sp_dma_length;
	}
	else
	{
		src = (sp_mem_addr & 0x1000) ? (UINT8*)&rsp_imem[(sp_mem_addr & 0xfff) / 4] : (UINT8*)&rsp_dmem[(sp_mem_addr & 0xfff) / 4];
		dst = (UINT8*)&rdram[sp_dram_addr / 4];

		for (i=0; i < sp_dma_length; i++)
		{
			dst[BYTE4_XOR_BE(i)] = src[BYTE4_XOR_BE(i)];
		}

        sp_mem_addr += sp_dma_length;
        sp_dram_addr += sp_dma_length;
	}
}

void sp_set_status(UINT32 status)
{
	if (status & 0x1)
	{

		cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
        cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_HALT);

	}
	if (status & 0x2)
	{

        cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_BROKE);

        if (cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & RSP_STATUS_INTR_BREAK)
		{
			signal_rcp_interrupt(SP_INTERRUPT);
		}
	}
}

READ32_HANDLER( n64_sp_reg_r )
{
	switch (offset)
	{
		case 0x00/4:
			return sp_mem_addr;

		case 0x04/4:
			return sp_dram_addr;

		case 0x08/4:
			return (sp_dma_skip << 20) | (sp_dma_count << 12) | sp_dma_length;

		case 0x10/4:
            return cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR);

		case 0x14/4:
			return 0;

		case 0x18/4:
			return 0;

		case 0x1c/4:
			return sp_semaphore;

        case 0x20/4:
        case 0x24/4:
        case 0x28/4:
        case 0x2c/4:
        case 0x30/4:
        case 0x34/4:
        case 0x38/4:
        case 0x3c/4:
            return 0;

        case 0x40000/4:
            return cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_PC);

        default:
            logerror("sp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
            break;
	}

	return 0;
}

WRITE32_HANDLER( n64_sp_reg_w )
{
	if ((offset & 0x10000) == 0)
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:
				sp_mem_addr = data;
				break;

			case 0x04/4:
				sp_dram_addr = data & 0xffffff;
				break;

			case 0x08/4:
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(0);
				break;

			case 0x0c/4:
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(1);
				break;

            case 0x10/4:
            {

                if (data & 0x00000001)
                {

                        cpunum_set_input_line(1, INPUT_LINE_HALT, CLEAR_LINE);
                        cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_HALT );

                }
                if (data & 0x00000002)
                {

                    cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_HALT );

                }
                if (data & 0x00000004)
                {

                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_BROKE );

                }
                if (data & 0x00000008)
                {
                    clear_rcp_interrupt(SP_INTERRUPT);
                }
                if (data & 0x00000010)
                {
                    signal_rcp_interrupt(SP_INTERRUPT);
                }
                if (data & 0x00000020)
                {

                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SSTEP );

                }
                if (data & 0x00000040)
                {

                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SSTEP );

                }
                if (data & 0x00000080)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_INTR_BREAK );

                }
                if (data & 0x00000100)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_INTR_BREAK );

                }
                if (data & 0x00000200)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL0 );

                }
                if (data & 0x00000400)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL0 );

                }
                if (data & 0x00000800)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL1 );

                }
                if (data & 0x00001000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL1 );

                }
                if (data & 0x00002000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL2 );

                }
                if (data & 0x00004000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL2 );

                }
                if (data & 0x00008000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL3 );

                }
                if (data & 0x00010000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL3 );

                }
                if (data & 0x00020000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL4 );

                }
                if (data & 0x00040000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL4 );

                }
                if (data & 0x00080000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL5 );

                }
                if (data & 0x00100000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL5 );

                }
                if (data & 0x00200000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL6 );

                }
                if (data & 0x00400000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL6 );

                }
                if (data & 0x00800000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) & ~RSP_STATUS_SIGNAL7 );

                }
                if (data & 0x01000000)
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_SR, cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_SR) | RSP_STATUS_SIGNAL7 );

                }
                break;
            }

			case 0x1c/4:
				sp_semaphore = data;

				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
		}
	}
	else
	{
        switch (offset & 0xffff)
        {
            case 0x00/4:

                if( cpunum_get_info_int(1, CPUINFO_INT_REGISTER + RSP_NEXTPC) != 0xffffffff )
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_NEXTPC, 0x04001000 | (data & 0xfff));
                }
                else
                {
                    cpunum_set_info_int(1, CPUINFO_INT_REGISTER + RSP_PC, 0x04001000 | (data & 0xfff));
                }
                break;

            default:
                logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
                break;
		}
	}
}

UINT32 sp_read_reg(UINT32 reg)
{
	switch (reg)
	{

		default:	return n64_sp_reg_r(reg, 0x00000000);
	}
}

void sp_write_reg(UINT32 reg, UINT32 data)
{
	switch (reg)
	{
		default:	n64_sp_reg_w(reg, data, 0x00000000); break;
	}
}

UINT32 dp_start;
UINT32 dp_end;
UINT32 dp_current;
UINT32 dp_status = 0;

void dp_full_sync(void)
{
	signal_rcp_interrupt(DP_INTERRUPT);
}

READ32_HANDLER( n64_dp_reg_r )
{
	switch (offset)
	{
		case 0x00/4:
			return dp_start;

		case 0x04/4:
			return dp_end;

		case 0x08/4:
			return dp_current;

		case 0x0c/4:
			return dp_status;

		default:
			logerror("dp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_dp_reg_w )
{
	switch (offset)
	{
		case 0x00/4:
			dp_start = data;
			dp_current = dp_start;
			break;

		case 0x04/4:
			dp_end = data;
			rdp_process_list();
			break;

		case 0x0c/4:
			if (data & 0x00000001)	dp_status &= ~DP_STATUS_XBUS_DMA;
			if (data & 0x00000002)	dp_status |= DP_STATUS_XBUS_DMA;
			if (data & 0x00000004)	dp_status &= ~DP_STATUS_FREEZE;
			if (data & 0x00000008)	dp_status |= DP_STATUS_FREEZE;
			if (data & 0x00000010)	dp_status &= ~DP_STATUS_FLUSH;
			if (data & 0x00000020)	dp_status |= DP_STATUS_FLUSH;
			break;

		default:
			logerror("dp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

UINT32 vi_width;
UINT32 vi_origin;
UINT32 vi_control;
UINT32 vi_burst, vi_vsync, vi_hsync, vi_leap, vi_hstart, vi_vstart;
UINT32 vi_intr, vi_vburst, vi_xscale, vi_yscale;

READ32_HANDLER( n64_vi_reg_r )
{
	switch (offset)
	{
		case 0x04/4:
			return vi_origin;

		case 0x08/4:
			return vi_width;

		case 0x0c/4:
			return vi_intr;

		case 0x10/4:
			return cpu_getscanline();

		case 0x14/4:
			return vi_burst;

		case 0x18/4:
			return vi_vsync;

		case 0x1c/4:
			return vi_hsync;

		case 0x20/4:
			return vi_leap;

		case 0x24/4:
			return vi_hstart;

		case 0x28/4:
			return vi_vstart;

		case 0x2c/4:
			return vi_vburst;

		case 0x30/4:
			return vi_xscale;

		case 0x34/4:
			return vi_yscale;

		default:
			logerror("vi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}
	return 0;
}

WRITE32_HANDLER( n64_vi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:
			if ((vi_control & 0x40) != (data & 0x40))
			{
				rectangle visarea = Machine->visible_area;
				visarea.max_y = (data & 0x40) ? 479 : 239;
				set_visible_area(visarea.min_x, visarea.max_x, visarea.min_y, visarea.max_y);
			}
			vi_control = data;
			break;

		case 0x04/4:
			vi_origin = data & 0xffffff;
			break;

		case 0x08/4:
			if (vi_width != data && data > 0)
			{
				rectangle visarea = Machine->visible_area;
				visarea.max_x = data-1;
				set_visible_area(visarea.min_x, visarea.max_x, visarea.min_y, visarea.max_y);
			}
			vi_width = data;
		        fb_width = data;
			break;

		case 0x0c/4:
			vi_intr = data;
			break;

		case 0x10/4:
			clear_rcp_interrupt(VI_INTERRUPT);
			break;

		case 0x14/4:
			vi_burst = data;
			break;

		case 0x18/4:
			vi_vsync = data;
			break;

		case 0x1c/4:
			vi_hsync = data;
			break;

		case 0x20/4:
			vi_leap = data;
			break;

		case 0x24/4:
			vi_hstart = data;
			break;

		case 0x28/4:
			vi_vstart = data;
			break;

		case 0x2c/4:
			vi_vburst = data;
			break;

		case 0x30/4:
			vi_xscale = data;
			break;

		case 0x34/4:
			vi_yscale = data;
			break;

		default:
			logerror("vi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

static UINT32 ai_dram_addr;
static UINT32 ai_len;
static UINT32 ai_control = 0;
static int ai_dacrate;
static int ai_bitrate;
static UINT32 ai_status = 0;

static mame_timer *audio_timer;

#define AUDIO_DMA_DEPTH		2

typedef struct
{
	UINT32 address;
	UINT32 length;
} AUDIO_DMA;

static AUDIO_DMA audio_fifo[AUDIO_DMA_DEPTH];
static int audio_fifo_wpos = 0;
static int audio_fifo_rpos = 0;
static int audio_fifo_num = 0;

static void start_audio_dma(void);

static void audio_fifo_push(UINT32 address, UINT32 length)
{
	if (audio_fifo_num == AUDIO_DMA_DEPTH || length == 0)
		return;
	audio_fifo[audio_fifo_wpos].address = address;
	audio_fifo[audio_fifo_wpos].length = length;
	audio_fifo_wpos = (audio_fifo_wpos + 1) % AUDIO_DMA_DEPTH;
	++audio_fifo_num;
	if (audio_fifo_num == AUDIO_DMA_DEPTH)
		ai_status |= 0x80000001;
	if (!(ai_status & 0x40000000))
		start_audio_dma();
}

static void audio_fifo_pop(void)
{
	audio_fifo_rpos++;
	audio_fifo_num--;

	if (audio_fifo_num < 0)
	{
		fatalerror("audio_fifo_pop: FIFO underflow!\n");
	}

	if (audio_fifo_rpos >= AUDIO_DMA_DEPTH)
	{
		audio_fifo_rpos = 0;
	}

	if (audio_fifo_num < AUDIO_DMA_DEPTH)
	{
		ai_status &= ~0x80000001;
	}

	ai_len = 0;
}

static AUDIO_DMA *audio_fifo_get_top(void)
{
	if (audio_fifo_num > 0)
	{
		return &audio_fifo[audio_fifo_rpos];
	}
	else
	{
		return NULL;
	}
}

static void start_audio_dma(void)
{
	INT16 *ram = (INT16*)rdram;
	AUDIO_DMA *current = audio_fifo_get_top();

	ram = &ram[current->address/2];

	dmadac_transfer(0, 2, 1, 2, current->length/4, ram);
	ai_status |= 0x40000000;
	timer_adjust(audio_timer, TIME_IN_HZ(DACRATE_NTSC) * (ai_dacrate + 1) * (current->length / 4), 0, 0);
}

static void audio_timer_callback(int param)
{
	audio_fifo_pop();
	signal_rcp_interrupt(AI_INTERRUPT);
	if (audio_fifo_get_top() != NULL)
		start_audio_dma();
	else
		ai_status &= ~0x40000000;
}

void n64_machine_start(void)
{
	audio_timer = timer_alloc(audio_timer_callback);
}

READ32_HANDLER( n64_ai_reg_r )
{
	switch (offset)
	{
		case 0x04/4:
		{
			return (ai_status & 0x40000000) ?
				(UINT32)(timer_timeleft(audio_timer) * DACRATE_NTSC / (ai_dacrate + 1)) * 4 : 0;
		}

		case 0x0c/4:
			return ai_status;

		default:
			logerror("ai_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_ai_reg_w )
{

	switch (offset)
	{
		case 0x00/4:

			ai_dram_addr = data & 0xffffff;
			break;

		case 0x04/4:

			ai_len = data & 0x3ffff;
			audio_fifo_push(ai_dram_addr, ai_len);
			break;

		case 0x08/4:

			ai_control = data;
			break;

		case 0x0c/4:
			clear_rcp_interrupt(AI_INTERRUPT);
			break;

		case 0x10/4:
			ai_dacrate = data & 0x3fff;
			dmadac_set_frequency(0, 2, (double)DACRATE_NTSC / (double)(ai_dacrate+1));
			dmadac_enable(0, 2, 1);
			break;

		case 0x14/4:

			ai_bitrate = data & 0xf;
			break;

		default:
			logerror("ai_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

static UINT32 pi_dram_addr, pi_cart_addr;
static UINT32 pi_first_dma = 1;

READ32_HANDLER( n64_pi_reg_r )
{
	switch (offset)
	{
		case 0x00/4:
			return pi_dram_addr;

		case 0x04/4:
			return pi_cart_addr;

		case 0x10/4:
			return 0;

		default:
			logerror("pi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}
	return 0;
}

WRITE32_HANDLER( n64_pi_reg_w )
{
	switch (offset)
	{
		case 0x00/4:
		{
			pi_dram_addr = data;
			break;
		}

		case 0x04/4:
		{
			pi_cart_addr = data;
			break;
		}

		case 0x08/4:
		{
			int i;
			UINT32 dma_length = (data + 1);

			if (pi_dram_addr != 0xffffffff)
			{
				for (i=0; i < dma_length; i++)
				{
					UINT8 b = program_read_byte_32be(pi_dram_addr);
					program_write_byte_32be(pi_cart_addr, b);
					pi_cart_addr += 1;
					pi_dram_addr += 1;
				}
			}

			signal_rcp_interrupt(PI_INTERRUPT);
			break;
		}

		case 0x0c/4:
		{
			int i;
			UINT32 dma_length = (data + 1);

			if (pi_dram_addr != 0xffffffff)
			{
				for (i=0; i < dma_length; i++)
				{

					UINT8 b = program_read_byte_32be(pi_cart_addr);
					program_write_byte_32be(pi_dram_addr, b);
					pi_cart_addr += 1;
					pi_dram_addr += 1;
				}
			}
			signal_rcp_interrupt(PI_INTERRUPT);

			if (pi_first_dma)
			{

				program_write_dword_32be(0x00000318, 0x800000);
				program_write_dword_32be(0x000003f0, 0x800000);
				pi_first_dma = 0;
			}

			break;
		}

		case 0x10/4:
		{
			if (data & 0x2)
			{
				clear_rcp_interrupt(PI_INTERRUPT);
			}
			break;
		}

		default:
			logerror("pi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

READ32_HANDLER( n64_ri_reg_r )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());
			break;
	}

	return 0;
}

WRITE32_HANDLER( n64_ri_reg_w )
{
	switch (offset)
	{
		default:
			logerror("ri_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
			break;
	}
}

UINT8 pif_ram[0x40];
UINT8 pif_cmd[0x40];
UINT32 si_dram_addr = 0;
UINT32 si_pif_addr = 0;
UINT32 si_status = 0;

static UINT8 eeprom[512];
static UINT8 mempack[0x8000];

static UINT8 calc_mempack_crc(UINT8 *buffer, int length)
{
	int i, j;
	UINT32 crc = 0;
	UINT32 temp2 = 0;

	for (i=0; i <= length; i++)
	{
		for (j=7; j >= 0; j--)
		{
			if ((crc & 0x80) != 0)
			{
				temp2 = 0x85;
			}
			else
			{
				temp2 = 0;
			}

			crc <<= 1;

			if (i == length)
			{
				crc &= 0xff;
			}
			else
			{
				if ((buffer[i] & (1 << j)) != 0)
				{
					crc |= 0x1;
				}
			}

			crc ^= temp2;
		}
	}

	return crc;
}

static int pif_channel_handle_command(int channel, int slength, UINT8 *sdata, int rlength, UINT8 *rdata)
{
	int i;
	UINT8 command = sdata[0];
	if (channel < 2 && readinputportbytag("input") == 0)
		return 1;

	switch (command)
	{
		case 0x00:
		{
			if (slength != 1 || rlength != 3)
			{

			}

			switch (channel)
			{
				case 0:
				case 1:
				{
					rdata[0] = readinputportbytag("input");
					rdata[1] = 0x00;
					rdata[2] = 0x02;
					return 0;
				}
				case 2:
				case 3:
				{

					return 1;
				}
				case 4:
				{
					rdata[0] = 0x00;
					rdata[1] = 0x80;
					rdata[2] = 0x00;

					return 1;
				}
				case 5:
				{
					logerror("EEPROM2? read status\n");
					return 1;
				}
			}

			break;
		}

		case 0x01:
		{
			UINT16 buttons = 0;
			INT8 x = 0, y = 0;
			if (slength != 1 || rlength != 4)
			{
				fatalerror("handle_pif: read button values (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			switch (channel)
			{
				case 0:
				case 1:
				{
					buttons = readinputportbytag(channel ? "P2" : "P1");
					x = readinputportbytag(channel ? "P2_ANALOG_X" : "P1_ANALOG_X") - 128;
					y = readinputportbytag(channel ? "P2_ANALOG_Y" : "P1_ANALOG_Y") - 128;

					rdata[0] = (buttons >> 8) & 0xff;
					rdata[1] = (buttons >> 0) & 0xff;
					rdata[2] = (UINT8)(x);
					rdata[3] = (UINT8)(y);
					return 0;
				}
				case 2:
				case 3:
				{

					return 1;
				}
			}

			break;
		}

		case 0x02:
		{
			UINT32 address, checksum;

			address = (sdata[1] << 8) | (sdata[2]);
			checksum = address & 0x1f;
			address &= ~0x1f;

			if (address == 0x400)
			{
				for (i=0; i < rlength-1; i++)
				{
					rdata[i] = 0x00;
				}

				rdata[rlength-1] = calc_mempack_crc(rdata, rlength-1);

			}
			else if (address < 0x7fe0)
			{
				for (i=0; i < rlength-1; i++)
				{
					rdata[i] = mempack[address+i];
				}

				rdata[rlength-1] = calc_mempack_crc(rdata, rlength-1);

			}
			return 1;
		}
		case 0x03:
		{
			UINT32 address, checksum;
			int i;

			address = (sdata[1] << 8) | (sdata[2]);
			checksum = address & 0x1f;
			address &= ~0x1f;

			if (address == 0x8000)
			{

			}
			else
			{
				for (i=3; i < slength; i++)
				{
					mempack[address++] = sdata[i];
				}
			}

			rdata[0] = calc_mempack_crc(&sdata[3], slength-3);

			return 1;
		}

		case 0x04:
		{
			UINT8 block_offset;

			if (channel != 4)
			{

				return 1;
			}

			if (slength != 2 || rlength != 8)
			{
				fatalerror("handle_pif: write EEPROM (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			block_offset = sdata[1] * 8;

			for (i=0; i < 8; i++)
			{
				rdata[i] = eeprom[block_offset+i];
			}

			return 1;
		}

		case 0x05:
		{
			UINT8 block_offset;

			if (channel != 4)
			{

				return 1;
			}

			if (slength != 10 || rlength != 1)
			{
				fatalerror("handle_pif: write EEPROM (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			block_offset = sdata[1] * 8;

			for (i=0; i < 8; i++)
			{

				eeprom[block_offset+i] = sdata[2+i];
			}

			rdata[0] = 0;

			return 1;
		}

		case 0xff:
		{
			rdata[0] = 0xff;
			rdata[1] = 0xff;
			rdata[2] = 0xff;
			return 0;
		}

		default:
		{
			logerror("handle_pif: unknown/unimplemented command %02X\n", command);
			return 1;
		}
	}

	return 0;
}

static void handle_pif(void)
{
	int j;

	if (pif_cmd[0x3f] == 0x1)
	{
		int channel = 0;
		int end = 0;
		int cmd_ptr = 0;

		while (cmd_ptr < 0x3f && !end)
		{
			UINT8 bytes_to_send;
			INT8 bytes_to_recv;

			bytes_to_send = pif_cmd[cmd_ptr++];

			if (bytes_to_send == 0xfe)
			{
				end = 1;
			}
			else if (bytes_to_send == 0xff)
			{

			}
			else
			{
				if (bytes_to_send > 0 && (bytes_to_send & 0xc0) == 0)
				{
					int res;
					UINT8 recv_buffer[0x40];
					UINT8 send_buffer[0x40];

					bytes_to_recv = pif_cmd[cmd_ptr++];

					for (j=0; j < bytes_to_send; j++)
					{
						send_buffer[j] = pif_cmd[cmd_ptr++];
					}

					res = pif_channel_handle_command(channel, bytes_to_send, send_buffer, bytes_to_recv, recv_buffer);

					if (res == 0)
					{
						if (cmd_ptr + bytes_to_recv > 0x3f)
						{
							fatalerror("cmd_ptr overflow\n");
						}
						for (j=0; j < bytes_to_recv; j++)
						{
							pif_ram[cmd_ptr++] = recv_buffer[j];
						}
					}
					else if (res == 1)
					{
						int offset = 0;
						pif_ram[cmd_ptr-offset-2] |= 0x80;
					}
				}

				channel++;
			}
		}

		pif_ram[0x3f] = 0;
	}

}

static void pif_dma(int direction)
{
	int i;
	UINT32 *src, *dst;

	if (si_dram_addr & 0x3)
	{
		fatalerror("pif_dma: si_dram_addr unaligned: %08X\n", si_dram_addr);
	}

	if (direction)
	{
		src = (UINT32*)&rdram[(si_dram_addr & 0x1fffffff) / 4];

		for (i=0; i < 64; i+=4)
		{
			UINT32 d = *src++;
			pif_ram[i+0] = (d >> 24) & 0xff;
			pif_ram[i+1] = (d >> 16) & 0xff;
			pif_ram[i+2] = (d >>  8) & 0xff;
			pif_ram[i+3] = (d >>  0) & 0xff;
		}

		memcpy(pif_cmd, pif_ram, 0x40);
	}
	else
	{
		handle_pif();

		dst = (UINT32*)&rdram[(si_dram_addr & 0x1fffffff) / 4];

		for (i=0; i < 64; i+=4)
		{
			UINT32 d = 0;
			d |= pif_ram[i+0] << 24;
			d |= pif_ram[i+1] << 16;
			d |= pif_ram[i+2] <<  8;
			d |= pif_ram[i+3] <<  0;

			*dst++ = d;
		}
	}

	si_status |= 0x1000;
	signal_rcp_interrupt(SI_INTERRUPT);
}

READ32_HANDLER( n64_si_reg_r )
{
	switch (offset)
	{
		case 0x00/4:
			return si_dram_addr;

		case 0x18/4:
			return si_status;
	}
	return 0;
}

WRITE32_HANDLER( n64_si_reg_w )
{
	switch (offset)
	{
		case 0x00/4:
			si_dram_addr = data;

			break;

		case 0x04/4:

			si_pif_addr = data;
			pif_dma(0);
			break;

		case 0x10/4:

			si_pif_addr = data;
			pif_dma(1);
			break;

		case 0x18/4:
			si_status &= ~0x1000;
			clear_rcp_interrupt(SI_INTERRUPT);
			break;

		default:
			logerror("si_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
			break;
	}
}

UINT32 cic_status = 0x00000000;

READ32_HANDLER( n64_pif_ram_r )
{

    if( offset == ( 0x24 / 4 ) )
    {
        cic_status = 0x00000080;
    }
    if( offset == ( 0x3C / 4 ) )
    {
        return cic_status;
    }
    return ( ( pif_ram[offset*4+0] << 24 ) | ( pif_ram[offset*4+1] << 16 ) | ( pif_ram[offset*4+2] <<  8 ) | ( pif_ram[offset*4+3] <<  0 ) ) & ~mem_mask;
}

WRITE32_HANDLER( n64_pif_ram_w )
{

    signal_rcp_interrupt(SI_INTERRUPT);
}

void n64_machine_reset(void)
{
	int i;

	UINT32 *cart = (UINT32*)memory_region(REGION_USER2);
	UINT64 boot_checksum;

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_DRC_OPTIONS, MIPS3DRC_FASTEST_OPTIONS + MIPS3DRC_STRICT_VERIFY);

	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_SELECT, 0);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_START, 0x00000000);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_END, 0x007fffff);
	cpunum_set_info_ptr(0, CPUINFO_PTR_MIPS3_FASTRAM_BASE, rdram);
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_FASTRAM_READONLY, 0);

	mi_version = 0x01010101;
	mi_interrupt = mi_intr_mask = 0;
	pi_first_dma = 1;
	cic_status = 0;
	n64_check_interrupts();

	timer_adjust(audio_timer, TIME_NEVER, 0, 0);
	audio_fifo_wpos = audio_fifo_rpos = audio_fifo_num = 0;
	ai_status = ai_control = ai_len = ai_dram_addr = 0;
	ai_dacrate = ai_bitrate = 0;
	dmadac_enable(0, 2, 0);
	sp_mem_addr = sp_dram_addr = sp_semaphore = 0;
	sp_dma_length = sp_dma_count = sp_dma_skip = 0;
	dp_start = dp_end = dp_current = dp_status = 0;
	vi_control = vi_origin = vi_width = 0;
	si_status = si_dram_addr = si_pif_addr = 0;
	memset(pif_ram, 0, sizeof(pif_ram));
	memset(pif_cmd, 0, sizeof(pif_cmd));

	cpunum_set_input_line(1, INPUT_LINE_HALT, ASSERT_LINE);

    boot_checksum = 0;
    for (i=0x40; i < 0x1000; i+=4)
    {
        boot_checksum += cart[i/4]+i;
    }

    pif_ram[0x24] = 0;
    pif_ram[0x25] = 0;
    pif_ram[0x26] = 0x3f;
    pif_ram[0x27] = 0x3f;
    rdram[0x318 / 4] = 0x800000;

    if (boot_checksum == U64(0x000000d057e84864))
    {

        logerror("CIC-NUS-6101 detected\n");

        pif_ram[0x24] = 0x00;
        pif_ram[0x25] = 0x04;
        pif_ram[0x26] = 0x3f;
        pif_ram[0x27] = 0x3f;
    }
    else if (boot_checksum == U64(0x000000d6499e376b))
    {

        logerror("CIC-NUS-6103 detected\n");

        pif_ram[0x24] = 0x00;
        pif_ram[0x25] = 0x04;
        pif_ram[0x26] = 0x78;
        pif_ram[0x27] = 0x78;
    }
    else if (boot_checksum == U64(0x0000011a4a1604b6))
    {

        logerror("CIC-NUS-6105 detected\n");

        pif_ram[0x24] = 0x00;
        pif_ram[0x25] = 0x04;
        pif_ram[0x26] = 0x91;
        pif_ram[0x27] = 0x91;
    }
    else if (boot_checksum == U64(0x000000d6d5de4ba0))
    {

        logerror("CIC-NUS-6106 detected\n");

        pif_ram[0x24] = 0x00;
        pif_ram[0x25] = 0x04;
        pif_ram[0x26] = 0x85;
        pif_ram[0x27] = 0x85;
    }
    else
    {
        logerror("Unknown BootCode Checksum %08X%08X\n", (UINT32)(boot_checksum>>32),(UINT32)(boot_checksum));
    }

}
