// license:BSD-3-Clause
// copyright-holders:Aaron Giles

struct ppc602_direct_region
{
	UINT32 start, size;
	const UINT8 *base;
	int readonly;
};

static struct ppc602_direct_map
{
	struct ppc602_direct_region regions[3];
	unsigned count;
} ppc602_direct_maps[MAX_CPU];

void ppc602_add_direct_memory(int cpu, UINT32 start, UINT32 size, const void *base, int readonly)
{
	struct ppc602_direct_map *map = &ppc602_direct_maps[cpu];
	struct ppc602_direct_region *region;

	assert_always(map->count < ARRAY_LENGTH(map->regions), "PPC602 direct map is full");
	assert_always(base && !(start & 7) && size && !(size & 7), "Invalid PPC602 direct region");
	region = &map->regions[map->count++];
	region->start = start;
	region->size = size;
	region->base = base;
	region->readonly = readonly;
}

INLINE const struct ppc602_direct_map *ppc602_direct_map_for_cpu(int cpu)
{
#if defined(MESS) || defined(MAME_DEBUG)
	return NULL;
#else
	const struct ppc602_direct_map *map = &ppc602_direct_maps[cpu];
	return map->count ? map : NULL;
#endif
}

INLINE const struct ppc602_direct_region *ppc602_opcode_region(const struct ppc602_direct_map *map, UINT32 pc)
{
	unsigned i;

#ifndef LSB_FIRST
	/* With relocation off, retain the core's 64-bit opcode accessor on big-endian hosts. */
	if (!(MSR & MSR_IR))
		return NULL;
#endif
	if (!map || (pc & 3))
		return NULL;
	for (i = 0; i < map->count; ++i)
		if (pc - map->regions[i].start < map->regions[i].size)
			return &map->regions[i];
	return NULL;
}

INLINE UINT32 ppc602_region_opcode(const struct ppc602_direct_region *region, UINT32 pc)
{
	UINT32 opcode;

	memcpy(&opcode, region->base + DWORD_XOR_BE(pc - region->start), sizeof(opcode));
	return opcode;
}

INLINE UINT32 ppc602_fetch(const struct ppc602_direct_map *map, UINT32 pc)
{
	const struct ppc602_direct_region *region = ppc602_opcode_region(map, pc);

	if (region)
		return ppc602_region_opcode(region, pc);
	if (MSR & MSR_IR)
		return ppc_readop_translated(pc);
	return ROPCODE64(pc);
}

void ppc602_exception(int exception)
{
	UINT32 msr = ppc_get_msr();
	UINT32 saved_pc = ppc.npc;
	UINT32 reason = 0;
	UINT32 vector;

	switch (exception)
	{
		case EXCEPTION_IRQ:
			vector = 0x500;
			break;
		case EXCEPTION_DECREMENTER:
			vector = 0x900;
			ppc.interrupt_pending &= ~2;
			break;
		case EXCEPTION_TRAP:
			vector = 0x700;
			saved_pc = ppc.pc;
			reason = 0x20000;
			break;
		case EXCEPTION_SYSTEM_CALL:
			vector = 0xc00;
			break;
		default:
			fatalerror("ppc602: Unhandled exception %d", exception);
			return;
	}

	SRR0 = saved_pc;
	SRR1 = (msr & 0x87c0ffff) | reason;
	msr &= ~(MSR_POW | MSR_EE | MSR_PR | MSR_FP | MSR_FE0 | MSR_SE |
		MSR_BE | MSR_FE1 | MSR_IR | MSR_DR | MSR_RI | MSR_LE);
	if (msr & MSR_ILE)
		msr |= MSR_LE;
	ppc_set_msr(msr);

	ppc.npc = ((msr & MSR_IP) ? 0xfff00000 : ppc.ibr) | vector;
	if (exception == EXCEPTION_IRQ && ppc.irq_callback)
		ppc.irq_callback(INPUT_LINE_IRQ0);
	change_pc(ppc.npc);
}

/* ESA and DSA save and restore the machine state surrounding supervisor access. */
static void ppc602_esa(UINT32 op)
{
	if (MSR & MSR602_SA)
		longjmp(ppc.exception_jmpbuf, EXCEPTION_TRAP);

	ppc.esasrr = ((MSR & MSR_EE) >> 15) | ((MSR & (MSR602_SA | MSR602_AP)) >> 21) |
		((MSR & MSR_PR) >> 11);
	ppc_set_msr((MSR & ~(MSR602_AP | MSR_EE | MSR_PR)) | MSR602_SA);
}

static void ppc602_dsa(UINT32 op)
{
	if (!(MSR & MSR602_SA))
		longjmp(ppc.exception_jmpbuf, EXCEPTION_TRAP);

	ppc_set_msr((MSR & ~(MSR602_SA | MSR602_AP | MSR_EE | MSR_PR)) |
		((ppc.esasrr & 1) << 15) | ((ppc.esasrr & 6) << 21) | ((ppc.esasrr & 8) << 11));
}

static void ppc602_set_irq_line(int irqline, int state)
{
	if (state)
		ppc.interrupt_pending |= 0x1;
	else
		ppc.interrupt_pending &= ~0x1;
}

INLINE void ppc602_check_interrupts(void)
{
	if (MSR & MSR_EE)
	{
		if (ppc.interrupt_pending != 0)
		{
			if (ppc.interrupt_pending & 0x1)
			{
				ppc602_exception(EXCEPTION_IRQ);
			}
			else if (ppc.interrupt_pending & 0x2)
			{
				ppc602_exception(EXCEPTION_DECREMENTER);
			}
		}
	}
}

static void ppc602_reset(void)
{
	ppc.pc = ppc.npc = 0xfff00100;

	ppc_set_msr(0x40);
	change_pc(ppc.pc);

	ppc.hid0 = 1;
	ppc.esasrr = ppc.sebr = ppc.ser = 0;
	ppc.tb = ppc.tb_frac602 = ppc.dec_frac = 0;
	DEC = 0xffffffff;

	ppc.interrupt_pending = 0;
}

static int ppc602_execute(int cycles)
{
	const struct ppc602_direct_map *map = ppc602_direct_map_for_cpu(cpu_getactivecpu());
	int exception_type;
	UINT32 opcode;
	ppc_icount = cycles;
	ppc_tb_base_icount = cycles;
	ppc_dec_base_icount = cycles + ppc.dec_frac;
	ppc602_schedule_decrementer();

	change_pc(ppc.npc);

	exception_type = setjmp(ppc.exception_jmpbuf);
	if (exception_type)
	{
		ppc.npc = ppc.pc;
		ppc602_exception(exception_type);
	}

	while( ppc_icount > 0 )
	{
		ppc.pc = ppc.npc;
		CALL_MAME_DEBUG;

		opcode = ppc602_fetch(map, ppc.pc);

		ppc.npc = ppc.pc + 4;
		switch(opcode >> 26)
		{
			case 19:	optable19[(opcode >> 1) & 0x3ff](opcode); break;
			case 31:	optable31[(opcode >> 1) & 0x3ff](opcode); break;
			case 59:	optable59[(opcode >> 1) & 0x3ff](opcode); break;
			case 63:	optable63[(opcode >> 1) & 0x3ff](opcode); break;
			default:	optable[opcode >> 26](opcode); break;
		}

		ppc_icount--;

		if(ppc_icount == ppc_dec_trigger_cycle) {
			ppc.interrupt_pending |= 0x2;
		}

		ppc602_check_interrupts();
	}

	ppc.tb_frac602 += ppc_tb_base_icount - ppc_icount;
	ppc.tb += ppc.tb_frac602 / (bus_freq_multiplier * 2);
	ppc.tb_frac602 %= bus_freq_multiplier * 2;

	DEC -= (ppc_dec_base_icount - ppc_icount) / (bus_freq_multiplier * 2);
	ppc.dec_frac = (ppc_dec_base_icount - ppc_icount) % (bus_freq_multiplier * 2);

	return cycles - ppc_icount;
}
