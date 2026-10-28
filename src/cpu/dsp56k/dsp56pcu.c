// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz

UINT8 LF_bit(const dsp56156_core* cpustate) { return (SR & 0x8000) >> 15; }
UINT8 FV_bit(const dsp56156_core* cpustate) { return (SR & 0x4000) >> 14; }

UINT8 I_bits(const dsp56156_core* cpustate) { return (SR & 0x0300) >> 8;  }
UINT8 S_bit (const dsp56156_core* cpustate) { return (SR & 0x0080) >> 7;  }
UINT8 L_bit (const dsp56156_core* cpustate) { return (SR & 0x0040) >> 6;  }
UINT8 E_bit (const dsp56156_core* cpustate) { return (SR & 0x0020) >> 5;  }
UINT8 U_bit (const dsp56156_core* cpustate) { return (SR & 0x0010) >> 4;  }
UINT8 N_bit (const dsp56156_core* cpustate) { return (SR & 0x0008) >> 3;  }
UINT8 Z_bit (const dsp56156_core* cpustate) { return (SR & 0x0004) >> 2;  }
UINT8 V_bit (const dsp56156_core* cpustate) { return (SR & 0x0002) >> 1;  }
UINT8 C_bit (const dsp56156_core* cpustate) { return (SR & 0x0001) >> 0;  }

void LF_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x8000); else (SR &= (~0x8000)); }
void FV_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x4000); else (SR &= (~0x4000)); }
void S_bits_set(dsp56156_core* cpustate, UINT8 value) { value = value & 0x03;  SR &= ~(0x0c00);  SR |= (value << 10); }
void I_bits_set(dsp56156_core* cpustate, UINT8 value) { value = value & 0x03;  SR &= ~(0x0300);  SR |= (value <<  8); }

void S_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0080); else (SR &= (~0x0080)); }
void L_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0040); else (SR &= (~0x0040)); }
void E_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0020); else (SR &= (~0x0020)); }
void U_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0010); else (SR &= (~0x0010)); }
void N_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0008); else (SR &= (~0x0008)); }
void Z_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0004); else (SR &= (~0x0004)); }
void V_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0002); else (SR &= (~0x0002)); }
void C_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (SR |= 0x0001); else (SR &= (~0x0001)); }

UINT8  R_bit(const dsp56156_core* cpustate) { return ((OMR & 0x0020) != 0); }

UINT8 MB_bit(const dsp56156_core* cpustate) { return ((OMR & 0x0002) != 0); }
UINT8 MA_bit(const dsp56156_core* cpustate) { return ((OMR & 0x0001) != 0); }

void CD_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0080); else (OMR &= (~0x0080)); }
void SD_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0040); else (OMR &= (~0x0040)); }
void  R_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0020); else (OMR &= (~0x0020)); }
void SA_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0010); else (OMR &= (~0x0010)); }
void MC_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0004); else (OMR &= (~0x0004)); }
void MB_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0002); else (OMR &= (~0x0002)); }
void MA_bit_set(dsp56156_core* cpustate, UINT8 value) { if (value) (OMR |= 0x0001); else (OMR &= (~0x0001)); }

UINT8 dsp56156_operating_mode(const dsp56156_core* cpustate)
{
	return ((MB_bit(cpustate) << 1) | MA_bit(cpustate));
}

UINT8 UF_bit(const dsp56156_core* cpustate) { return ((SP & 0x0020) != 0); }
UINT8 SE_bit(const dsp56156_core* cpustate) { return ((SP & 0x0010) != 0); }

void pcu_init(dsp56156_core* cpustate)
{

	dsp56156_irq_table_init();
}

void pcu_reset(dsp56156_core* cpustate)
{
	int i;

	MA_bit_set(cpustate, cpustate->modA_state);
	MB_bit_set(cpustate, cpustate->modB_state);
	MC_bit_set(cpustate, cpustate->modC_state);

	switch(dsp56156_operating_mode(cpustate))
	{
		case 0x00:
			logerror("Dsp56156 in Special Bootstrap Mode 1\n");

			cpustate->bootstrap_mode = BOOTSTRAP_OFF;

			for (i = 0; i < 0x800; i++)
			{
				UINT32 mem_offset = (0xc000<<1) + (i<<1);

				UINT8 mem_value_low  = program_read_byte_16le(mem_offset);
				UINT8 mem_value_high = program_read_byte_16le(mem_offset);
				cpustate->program_ram[i] = (mem_value_high << 8) | mem_value_low;
			}

			PC = 0x0000;

			MB_bit_set(cpustate, 1);
			MA_bit_set(cpustate, 0);
			cpustate->PCU.reset_vector = 0xe000;
			break;

		case 0x01:
			logerror("Dsp56156 in Special Bootstrap Mode 2\n");

			if (dsp_program_read(0xc000) & 0x8000)
			{
				cpustate->bootstrap_mode = BOOTSTRAP_SSIX;
				logerror("DSP56156 : Currently in (hacked) bootstrap mode - reading from SSIx.\n");
			}
			else
			{
				cpustate->bootstrap_mode = BOOTSTRAP_HI;
				logerror("DSP56156 : Currently in (hacked) bootstrap mode - reading from Host Interface.\n");
			}

			PC = 0x0000;

			MB_bit_set(cpustate, 1);
			MA_bit_set(cpustate, 0);
			cpustate->PCU.reset_vector = 0xe000;
			break;

		case 0x02:
			logerror("Dsp56156 in Normal Expanded Mode\n");
			PC = 0xe000;
			cpustate->PCU.reset_vector = 0xe000;
			break;

		case 0x03:
			logerror("Dsp56156 in Development Expanded Mode\n");

			PC = 0x0000;
			cpustate->PCU.reset_vector = 0x0000;
			break;
	}

	IPR = 0x0000;

	I_bits_set(cpustate, 0x03);
	S_bits_set(cpustate, 0);
	L_bit_set(cpustate, 0);
	S_bit_set(cpustate, 0);
	FV_bit_set(cpustate, 0);

	E_bit_set(cpustate, 0);
	U_bit_set(cpustate, 0);
	N_bit_set(cpustate, 0);
	V_bit_set(cpustate, 0);
	Z_bit_set(cpustate, 0);

	C_bit_set(cpustate, 0);
	LF_bit_set(cpustate, 0);
	SP = 0x0000;

	SA_bit_set(cpustate, 0);
	R_bit_set(cpustate, 0);
	SD_bit_set(cpustate, 0);
	CD_bit_set(cpustate, 0);

	dsp56156_clear_pending_interrupts(cpustate);
}

struct dsp56156_irq_data
{
	UINT16 irq_vector;
	char   irq_source[128];
};

struct dsp56156_irq_data dsp56156_interrupt_sources[32];

void pcu_service_interrupts(dsp56156_core* cpustate)
{
	int i;

	int num_servicable = dsp56156_count_pending_interrupts(cpustate);

	if (num_servicable == 0)
		return;

	dsp56156_sort_pending_interrupts(cpustate, num_servicable);

	for (i = 0; i < num_servicable; i++)
	{
		const int interrupt_index = cpustate->PCU.pending_interrupts[i];

		INT8 priority = dsp56156_get_irq_priority(cpustate, interrupt_index);

		if (priority >= I_bits(cpustate))
		{
			IPC = PC;

			if (interrupt_index != 22)
			{

				PC = dsp56156_interrupt_sources[interrupt_index].irq_vector;
			}
			else
			{

				const UINT16 irq_vector = HV_bits(cpustate) << 1;

				PC = irq_vector;

				HC_bit_set(cpustate, 0);
			}
		}
	}

	dsp56156_clear_pending_interrupts(cpustate);
}

void dsp56156_add_pending_interrupt(dsp56156_core* cpustate, const char* name)
{
	int i;
	int irq_index = dsp56156_get_irq_index_by_tag(name);

	for (i = 0; i < 32; i++)
	{
		if (cpustate->PCU.pending_interrupts[i] == -1)
		{
			cpustate->PCU.pending_interrupts[i] = irq_index;
			break;
		}
	}
}

void dsp56156_set_irq_source(UINT8 irq_num, UINT16 iv, const char* source)
{
	dsp56156_interrupt_sources[irq_num].irq_vector = iv;
	strcpy(dsp56156_interrupt_sources[irq_num].irq_source, source);
}

void dsp56156_irq_table_init()
{

	dsp56156_set_irq_source(0,  0x0000, "Hardware RESET");
	dsp56156_set_irq_source(1,  0x0002, "Illegal Instruction");
	dsp56156_set_irq_source(2,  0x0004, "Stack Error");
	dsp56156_set_irq_source(3,  0x0006, "Reserved");
	dsp56156_set_irq_source(4,  0x0008, "SWI");
	dsp56156_set_irq_source(5,  0x000a, "IRQA");
	dsp56156_set_irq_source(6,  0x000c, "IRQB");
	dsp56156_set_irq_source(7,  0x000e, "Reserved");
	dsp56156_set_irq_source(8,  0x0010, "SSI0 Receive Data with Exception");
	dsp56156_set_irq_source(9,  0x0012, "SSI0 Receive Data");
	dsp56156_set_irq_source(10, 0x0014, "SSI0 Transmit Data with Exception");
	dsp56156_set_irq_source(11, 0x0016, "SSI0 Transmit Data");
	dsp56156_set_irq_source(12, 0x0018, "SSI1 Receive Data with Exception");
	dsp56156_set_irq_source(13, 0x001a, "SSI1 Receive Data");
	dsp56156_set_irq_source(14, 0x001c, "SSI1 Transmit Data with Exception");
	dsp56156_set_irq_source(15, 0x001e, "SSI1 Transmit Data");
	dsp56156_set_irq_source(16, 0x0020, "Timer Overflow");
	dsp56156_set_irq_source(17, 0x0022, "Timer Compare");
	dsp56156_set_irq_source(18, 0x0024, "Host DMA Receive Data");
	dsp56156_set_irq_source(19, 0x0026, "Host DMA Transmit Data");
	dsp56156_set_irq_source(20, 0x0028, "Host Receive Data");
	dsp56156_set_irq_source(21, 0x002a, "Host Transmit Data");
	dsp56156_set_irq_source(22, 0x002c, "Host Command");
	dsp56156_set_irq_source(23, 0x002e, "Codec Receive/Transmit");
	dsp56156_set_irq_source(24, 0x0030, "Host Command 1");
	dsp56156_set_irq_source(25, 0x0032, "Host Command 2");
	dsp56156_set_irq_source(26, 0x0034, "Host Command 3");
	dsp56156_set_irq_source(27, 0x0036, "Host Command 4");
	dsp56156_set_irq_source(28, 0x0038, "Host Command 5");
	dsp56156_set_irq_source(29, 0x003a, "Host Command 6");
	dsp56156_set_irq_source(30, 0x003c, "Host Command 7");
	dsp56156_set_irq_source(31, 0x003e, "Host Command 8");
}

void dsp56156_clear_pending_interrupts(dsp56156_core* cpustate)
{
	int i;
	for (i = 0; i < 32; i++)
	{
		cpustate->PCU.pending_interrupts[i] = -1;
	}
}

int dsp56156_count_pending_interrupts(dsp56156_core* cpustate)
{
	int numI = 0;
	while (cpustate->PCU.pending_interrupts[numI] != -1)
	{
		numI++;
	}

	return numI;
}

void dsp56156_sort_pending_interrupts(dsp56156_core* cpustate, int num)
{
	int i, j;

	int priority_list[32];
	for (i = 0; i < num; i++)
	{
		priority_list[i] = dsp56156_get_irq_priority(cpustate, cpustate->PCU.pending_interrupts[i]);
	}

	for (i = 0; i < num; i++)
	{
		for(j = 0; j < num-1; j++)
		{
			if (priority_list[j] > priority_list[j+1])
			{
				int holder;

				holder = priority_list[j+1];
				priority_list[j+1] = priority_list[j];
				priority_list[j] = holder;

				holder = cpustate->PCU.pending_interrupts[j+1];
				cpustate->PCU.pending_interrupts[j+1] = cpustate->PCU.pending_interrupts[j];
				cpustate->PCU.pending_interrupts[j] = holder;
			}
		}
	}

}

INT8 dsp56156_get_irq_priority(dsp56156_core* cpustate, int index)
{

	switch (index)
	{

		case 0:  return 3;
		case 1:  return 3;
		case 2:  return 3;
		case 3:  return 3;
		case 4:  return 3;

		case 5:  return irqa_ipl(cpustate);
		case 6:  return irqb_ipl(cpustate);
		case 7:  return -1;
		case 8:  return ssi0_ipl(cpustate);
		case 9:  return ssi0_ipl(cpustate);
		case 10: return ssi0_ipl(cpustate);
		case 11: return ssi0_ipl(cpustate);
		case 12: return ssi1_ipl(cpustate);
		case 13: return ssi1_ipl(cpustate);
		case 14: return ssi1_ipl(cpustate);
		case 15: return ssi1_ipl(cpustate);
		case 16: return tm_ipl(cpustate);
		case 17: return tm_ipl(cpustate);
		case 18: return host_ipl(cpustate);
		case 19: return host_ipl(cpustate);
		case 20: return host_ipl(cpustate);
		case 21: return host_ipl(cpustate);
		case 22: return host_ipl(cpustate);
		case 23: return codec_ipl(cpustate);
		case 24: return host_ipl(cpustate);
		case 25: return host_ipl(cpustate);
		case 26: return host_ipl(cpustate);
		case 27: return host_ipl(cpustate);
		case 28: return host_ipl(cpustate);
		case 29: return host_ipl(cpustate);
		case 30: return host_ipl(cpustate);
		case 31: return host_ipl(cpustate);

		default: break;
	}

	return -1;
}

int dsp56156_get_irq_index_by_tag(const char* tag)
{
	int i;
	for (i = 0; i < 32; i++)
	{
		if (strcmp(tag, dsp56156_interrupt_sources[i].irq_source) == 0)
		{
			return i;
		}
	}

	fatalerror("DSP56156 ERROR : IRQ TAG specified incorrectly (get_vector_by_tag) : %s.\n", tag);

}
