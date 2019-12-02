#include "driver.h"
#include "sound/es5506.h"
#include "includes/taito_f3.h"

static int counter,vector_reg,imr_status;
static UINT16 es5510_dsp_ram[0x200];
static UINT32	es5510_gpr[0xc0];
static UINT32   es5510_dram[1<<16];
static UINT32   es5510_dol_latch;
static UINT32   es5510_dil_latch;
static UINT32   es5510_dadr_latch;
static UINT32	es5510_gpr_latch;
static UINT8    es5510_ram_sel;
static void *timer_68681=NULL;
static int timer_mode,m68681_imr;

static int es_tmp=1;

#define M68000_CLOCK	16000000
#define M68681_CLOCK	2000000 /* Actually X1, not the main clock */

enum { TIMER_SINGLESHOT, TIMER_PULSE };

READ16_HANDLER(f3_68000_share_r)
{
	if ((offset&3)==0) return (f3_shared_ram[offset/4]&0xff000000)>>16;
	if ((offset&3)==1) return (f3_shared_ram[offset/4]&0x00ff0000)>>8;
	if ((offset&3)==2) return (f3_shared_ram[offset/4]&0x0000ff00)>>0;
	return (f3_shared_ram[offset/4]&0x000000ff)<<8;
}

WRITE16_HANDLER(f3_68000_share_w)
{
	if ((offset&3)==0) f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0x00ffffff)|((data&0xff00)<<16);
	else if ((offset&3)==1) f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0xff00ffff)|((data&0xff00)<<8);
	else if ((offset&3)==2) f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0xffff00ff)|((data&0xff00)<<0);
	else f3_shared_ram[offset/4]=(f3_shared_ram[offset/4]&0xffffff00)|((data&0xff00)>>8);
}

WRITE16_HANDLER( f3_es5505_bank_w )
{
	unsigned int max_banks_this_game=(memory_region_length(REGION_SOUND1)/0x200000)-1;

#if 0
{
	static char count[10];
	count[data&7]++;
	ui_popup("%x %x %x %x %x %x %x %x (%d)",count[0]&0xf,count[1]&0xf,count[2]&0xf,count[3]&0xf,count[4]&0xf,count[5]&0xf,count[6]&0xf,count[7]&0xf, max_banks_this_game);
}
#endif

	/* mask out unused bits */
	data &= max_banks_this_game;
	ES5505_voice_bank_0_w(offset,data<<20);
}

WRITE16_HANDLER( f3_volume_w )
{
	static UINT16 channel[8],last_l,last_r;
	static int latch;

	if (offset==0) latch=(data>>8)&0x7;
	if (offset==1) channel[latch]=data>>8;

//      if (channel[7]!=last_l) mixer_set_volume(0, (int)((float)channel[7]*1.58)); /* Left master volume */
//      if (channel[6]!=last_r) mixer_set_volume(1, (int)((float)channel[6]*1.58)); /* Right master volume */
	last_l=channel[7];
	last_r=channel[6];

	/* Channel 5 - Left Aux?  Always set to volume, but never used for panning */
	/* Channel 4 - Right Aux?  Always set to volume, but never used for panning */
	/* Channels 0, 1, 2, 3 - Unused */
}

static void timer_callback(int param)
{
	/* Only cause IRQ if the mask is set to allow it */
	if (m68681_imr&8) {
		cpunum_set_input_line_vector(1, 6, vector_reg);
		cpunum_set_input_line(1, 6, ASSERT_LINE);
		imr_status|=0x8;
	}
}

void f3_68681_reset(void)
{
	timer_68681 = timer_alloc(timer_callback);
}

READ16_HANDLER(f3_68681_r)
{
	if (offset==0x5) {
		int ret=imr_status;
		imr_status=0;
		return ret;
	}

	if (offset==0xe)
		return 1;

	/* IRQ ack */
	if (offset==0xf) {
		cpunum_set_input_line(1, 6, CLEAR_LINE);
		return 0;
	}

	return 0xff;
}

WRITE16_HANDLER(f3_68681_w)
{
	switch (offset) {
		case 0x04: /* ACR */
			switch ((data>>4)&7) {
				case 0:
					logerror("Counter:  Unimplemented external IP2\n");
					break;
				case 1:
					logerror("Counter:  Unimplemented TxCA - 1X clock of channel A\n");
					break;
				case 2:
					logerror("Counter:  Unimplemented TxCB - 1X clock of channel B\n");
					break;
				case 3:
					logerror("Counter:  X1/Clk - divided by 16, counter is %04x, so interrupt every %d cycles\n",counter,(M68000_CLOCK/M68681_CLOCK)*counter*16);
					timer_mode=TIMER_SINGLESHOT;
					timer_adjust(timer_68681, TIME_IN_CYCLES((M68000_CLOCK/M68681_CLOCK)*counter*16,1), 0, 0);
					break;
				case 4:
					logerror("Timer:  Unimplemented external IP2\n");
					break;
				case 5:
					logerror("Timer:  Unimplemented external IP2/16\n");
					break;
				case 6:
					logerror("Timer:  X1/Clk, counter is %04x, so interrupt every %d cycles\n",counter,(M68000_CLOCK/M68681_CLOCK)*counter);
					timer_mode=TIMER_PULSE;
					timer_adjust(timer_68681, TIME_IN_CYCLES((M68000_CLOCK/M68681_CLOCK)*counter,1), 0, TIME_IN_CYCLES((M68000_CLOCK/M68681_CLOCK)*counter,1));
					break;
				case 7:
					logerror("Timer:  Unimplemented X1/Clk - divided by 16\n");
					break;
			}
			break;

		case 0x05: /* IMR */
			logerror("68681:  %02x %02x\n",offset,data&0xff);
			m68681_imr=data&0xff;
			break;

		case 0x06: /* CTUR */
			counter=((data&0xff)<<8)|(counter&0xff);
			break;
		case 0x07: /* CTLR */
			counter=(counter&0xff00)|(data&0xff);
			break;
		case 0x08: break; /* MR1B (Mode register B) */
		case 0x09: break; /* CSRB (Clock select register B) */
		case 0x0a: break; /* CRB (Command register B) */
		case 0x0b: break; /* TBB (Transmit buffer B) */
		case 0x0c: /* IVR (Interrupt vector) */
			vector_reg=data&0xff;
			break;
		default:
			logerror("68681:  %02x %02x\n",offset,data&0xff);
			break;
	}
}

READ16_HANDLER(es5510_dsp_r)
{
	switch(offset)
	{
		case 0x09: return (es5510_dil_latch >> 16) & 0xff;
		case 0x0a: return (es5510_dil_latch >> 8) & 0xff;
		case 0x0b: return (es5510_dil_latch >> 0) & 0xff; //TODO: docs says that this always returns 0
	}

	if (offset==0x12) return 0;

//	if (offset>4)
	if (offset==0x16) return 0x27;

	return es5510_dsp_ram[offset];
}

WRITE16_HANDLER(es5510_dsp_w)
{
	UINT8 *snd_mem = (UINT8 *)memory_region(REGION_SOUND1);

//	if (offset>4 && offset!=0x80  && offset!=0xa0  && offset!=0xc0  && offset!=0xe0)
//		logerror("%06x: DSP write offset %04x %04x\n",activecpu_get_pc(),offset,data);

	COMBINE_DATA(&es5510_dsp_ram[offset]);

	switch (offset) {
		case 0x00: es5510_gpr_latch=(es5510_gpr_latch&0x00ffff)|((data&0xff)<<16); break;
		case 0x01: es5510_gpr_latch=(es5510_gpr_latch&0xff00ff)|((data&0xff)<< 8); break;
		case 0x02: es5510_gpr_latch=(es5510_gpr_latch&0xffff00)|((data&0xff)<< 0); break;

		/* 0x03 to 0x08 INSTR Register */
		/* 0x09 to 0x0b DIL Register (r/o) */

		case 0x0c: es5510_dol_latch=(es5510_dol_latch&0x00ffff)|((data&0xff)<<16); break;
		case 0x0d: es5510_dol_latch=(es5510_dol_latch&0xff00ff)|((data&0xff)<< 8); break;
		case 0x0e: es5510_dol_latch=(es5510_dol_latch&0xffff00)|((data&0xff)<< 0); break; //TODO: docs says that this always returns 0xff

		case 0x0f:
			es5510_dadr_latch=(es5510_dadr_latch&0x00ffff)|((data&0xff)<<16);

            if(es5510_ram_sel)
                es5510_dil_latch = es5510_dram[es5510_dadr_latch&(1<<16)-2];
            else
                es5510_dram[es5510_dadr_latch&(1<<16)-2] = es5510_dol_latch;
            break;

		case 0x10: es5510_dadr_latch=(es5510_dadr_latch&0xff00ff)|((data&0xff)<< 8); break;
		case 0x11: es5510_dadr_latch=(es5510_dadr_latch&0xffff00)|((data&0xff)<< 0); break;

		/* 0x12 Host Control */

		case 0x14: es5510_ram_sel = data & 0x80; /* bit 6 is i/o select, everything else is undefined */break;

		/* 0x16 Program Counter (test purpose, r/o?) */
		/* 0x17 Internal Refresh counter (test purpose) */
		/* 0x18 Host Serial Control */
		/* 0x1f Halt enable (w) / Frame Counter (r) */

		case 0x80: /* Read select - GPR + INSTR */
	//		logerror("ES5510:  Read GPR/INSTR %06x (%06x)\n",data,es5510_gpr[data]);

			/* Check if a GPR is selected */
			if (data<0xc0) {
				es_tmp=0;
				es5510_gpr_latch=es5510_gpr[data];
			} else es_tmp=1;
			break;

		case 0xa0: /* Write select - GPR */
	//		logerror("ES5510:  Write GPR %06x %06x (0x%04x:=0x%06x\n",data,es5510_gpr_latch,data,snd_mem[es5510_gpr_latch>>8]);
			if (data<0xc0)
				es5510_gpr[data]=snd_mem[es5510_gpr_latch>>8];
			break;

		case 0xc0: /* Write select - INSTR */
	//		logerror("ES5510:  Write INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;

		case 0xe0: /* Write select - GPR + INSTR */
	//		logerror("ES5510:  Write GPR/INSTR %06x %06x\n",data,es5510_gpr_latch);
			break;
	}
}
