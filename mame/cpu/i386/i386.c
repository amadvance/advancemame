// license:BSD-3-Clause
// copyright-holders:Ville Linde, Barry Rodewald
#include "driver.h"
#include "debugger.h"
#include "usrintrf.h"
#include "i386.h"




#undef i386

int i386_parity_table[256];
MODRM_TABLE i386_MODRM_table[256];

static void i386_trap_with_error(int irq, int irq_gate, int trap_level, UINT32 err);
static void i286_task_switch(UINT16 selector, UINT8 nested);
static void i386_task_switch(UINT16 selector, UINT8 nested);

#define FAULT(fault,error) {i386_trap_with_error(fault,0,0,error); return;}
#define FAULT_EXP(fault,error) {i386_trap_with_error(fault,0,trap_level+1,error); return;}


#define INT_DEBUG	1

static void i386_load_protected_mode_segment(I386_SREG *seg )
{
	UINT32 v1,v2;
	UINT32 base, limit;
	int entry;

	if ( seg->selector & 0x4 )
	{
		base = I.ldtr.base;
		limit = I.ldtr.limit;
	} else {
		base = I.gdtr.base;
		limit = I.gdtr.limit;
	}

	if (limit == 0 || seg->selector + 7 > limit)
		return;
	entry = seg->selector & ~0x7;

	v1 = READ32(base + entry );
	v2 = READ32(base + entry + 4 );

	seg->flags = (v2 >> 8) & 0xf0ff;
	seg->base = (v2 & 0xff000000) | ((v2 & 0xff) << 16) | ((v1 >> 16) & 0xffff);
	seg->limit = (v2 & 0xf0000) | (v1 & 0xffff);
	if (seg->flags & 0x8000)
		seg->limit = (seg->limit << 12) | 0xfff;
	seg->d = (seg->flags & 0x4000) ? 1 : 0;
}

static void i386_load_call_gate(I386_CALL_GATE *gate)
{
	UINT32 v1,v2;
	UINT32 base,limit;
	int entry;

	if ( gate->segment & 0x4 )
	{
		base = I.ldtr.base;
		limit = I.ldtr.limit;
	} else {
		base = I.gdtr.base;
		limit = I.gdtr.limit;
	}

	if (limit == 0 || gate->segment + 7 > limit)
		return;

	entry = gate->segment & ~0x7;

	v1 = READ32(base + entry );
	v2 = READ32(base + entry + 4 );

	gate->selector = (v1 >> 16) & 0xffff;
	gate->offset = (v1 & 0x0000ffff) | (v2 & 0xffff0000);
	gate->ar = (v2 >> 8) & 0xff;
	gate->dword_count = v2 & 0x001f;
	gate->present = (gate->ar >> 7) & 0x01;
	gate->dpl = (gate->ar >> 5) & 0x03;
}

static void i386_load_segment_descriptor(int segment )
{
	if (PROTECTED_MODE)
	{
		if (!V8086_MODE)
			i386_load_protected_mode_segment(&I.sreg[segment] );
		else
		{
			I.sreg[segment].base = I.sreg[segment].selector << 4;
			I.sreg[segment].limit = 0xffff;
			I.sreg[segment].flags = (segment == CS) ? 0x009a : 0x0092;
		}
	}
	else
	{
		I.sreg[segment].base = I.sreg[segment].selector << 4;

		if( segment == CS && !I.performed_intersegment_jump )
			I.sreg[segment].base |= 0xfff00000;
	}
}

static UINT32 i386_get_stack_segment(UINT8 privilege)
{
	if(privilege >= 3)
		return 0;

	return READ32((I.task.base+8) + (8*privilege));
}

static UINT16 i286_get_stack_segment(UINT8 privilege)
{
	if(privilege >= 3)
		return 0;

	return READ16((I.task.base+4) + (4*privilege));
}

static UINT32 i386_get_stack_ptr(UINT8 privilege)
{
	if(privilege >= 3)
		return 0;

	return READ32((I.task.base+4) + (8*privilege));
}

static UINT16 i286_get_stack_ptr(UINT8 privilege)
{
	if(privilege >= 3)
		return 0;

	return READ16((I.task.base+2) + (4*privilege));
}

static UINT32 get_flags(void)
{
	UINT32 f = 0x2;
	f |= I.CF;
	f |= I.PF << 2;
	f |= I.AF << 4;
	f |= I.ZF << 6;
	f |= I.SF << 7;
	f |= I.TF << 8;
	f |= I.IF << 9;
	f |= I.DF << 10;
	f |= I.OF << 11;
	f |= I.IOP1 << 12;
	f |= I.IOP2 << 13;
	f |= I.NT << 14;
	return (I.eflags & I.eflags_mask) | (f & 0xffff);
}

static void set_flags(UINT32 f )
{
	I.CF = (f & 0x1) ? 1 : 0;
	I.PF = (f & 0x4) ? 1 : 0;
	I.AF = (f & 0x10) ? 1 : 0;
	I.ZF = (f & 0x40) ? 1 : 0;
	I.SF = (f & 0x80) ? 1 : 0;
	I.TF = (f & 0x100) ? 1 : 0;
	I.IF = (f & 0x200) ? 1 : 0;
	I.DF = (f & 0x400) ? 1 : 0;
	I.OF = (f & 0x800) ? 1 : 0;
	I.IOP1 = (f & 0x1000) ? 1 : 0;
	I.IOP2 = (f & 0x2000) ? 1 : 0;
	I.NT = (f & 0x4000) ? 1 : 0;
	I.eflags = f & I.eflags_mask;
}

static void sib_byte(UINT8 mod, UINT32* out_ea, UINT8* out_segment)
{
	UINT32 ea = 0;
	UINT8 segment = 0;
	UINT8 scale, i, base;
	UINT8 sib = FETCH();
	scale = (sib >> 6) & 0x3;
	i = (sib >> 3) & 0x7;
	base = sib & 0x7;

	switch( base )
	{
		case 0: ea = REG32(EAX); segment = DS; break;
		case 1: ea = REG32(ECX); segment = DS; break;
		case 2: ea = REG32(EDX); segment = DS; break;
		case 3: ea = REG32(EBX); segment = DS; break;
		case 4: ea = REG32(ESP); segment = SS; break;
		case 5:
			if( mod == 0 ) {
				ea = FETCH32();
				segment = DS;
			} else if( mod == 1 ) {
				ea = REG32(EBP);
				segment = SS;
			} else if( mod == 2 ) {
				ea = REG32(EBP);
				segment = SS;
			}
			break;
		case 6: ea = REG32(ESI); segment = DS; break;
		case 7: ea = REG32(EDI); segment = DS; break;
	}
	switch( i )
	{
		case 0: ea += REG32(EAX) * (1 << scale); break;
		case 1: ea += REG32(ECX) * (1 << scale); break;
		case 2: ea += REG32(EDX) * (1 << scale); break;
		case 3: ea += REG32(EBX) * (1 << scale); break;
		case 4: break;
		case 5: ea += REG32(EBP) * (1 << scale); break;
		case 6: ea += REG32(ESI) * (1 << scale); break;
		case 7: ea += REG32(EDI) * (1 << scale); break;
	}
	*out_ea = ea;
	*out_segment = segment;
}

static void modrm_to_EA(UINT8 mod_rm, UINT32* out_ea, UINT8* out_segment)
{
	INT8 disp8;
	INT16 disp16;
	INT32 disp32;
	UINT8 mod = (mod_rm >> 6) & 0x3;
	UINT8 rm = mod_rm & 0x7;
	UINT32 ea;
	UINT8 segment;

	if( mod_rm >= 0xc0 )
		fatalerror("i386: Called modrm_to_EA with modrm value %02X !",mod_rm);

	if( I.address_size ) {
		switch( rm )
		{
			default:
			case 0: ea = REG32(EAX); segment = DS; break;
			case 1: ea = REG32(ECX); segment = DS; break;
			case 2: ea = REG32(EDX); segment = DS; break;
			case 3: ea = REG32(EBX); segment = DS; break;
			case 4: sib_byte(mod, &ea, &segment ); break;
			case 5:
				if( mod == 0 ) {
					ea = FETCH32(); segment = DS;
				} else {
					ea = REG32(EBP); segment = SS;
				}
				break;
			case 6: ea = REG32(ESI); segment = DS; break;
			case 7: ea = REG32(EDI); segment = DS; break;
		}
		if( mod == 1 ) {
			disp8 = FETCH();
			ea += (INT32)disp8;
		} else if( mod == 2 ) {
			disp32 = FETCH32();
			ea += disp32;
		}

		if( I.segment_prefix )
			segment = I.segment_override;

		*out_ea = ea;
		*out_segment = segment;

	} else {
		switch( rm )
		{
			default:
			case 0: ea = REG16(BX) + REG16(SI); segment = DS; break;
			case 1: ea = REG16(BX) + REG16(DI); segment = DS; break;
			case 2: ea = REG16(BP) + REG16(SI); segment = SS; break;
			case 3: ea = REG16(BP) + REG16(DI); segment = SS; break;
			case 4: ea = REG16(SI); segment = DS; break;
			case 5: ea = REG16(DI); segment = DS; break;
			case 6:
				if( mod == 0 ) {
					ea = FETCH16(); segment = DS;
				} else {
					ea = REG16(BP); segment = SS;
				}
				break;
			case 7: ea = REG16(BX); segment = DS; break;
		}
		if( mod == 1 ) {
			disp8 = FETCH();
			ea += (INT32)disp8;
		} else if( mod == 2 ) {
			disp16 = FETCH16();
			ea += (INT32)disp16;
		}

		if( I.segment_prefix )
			segment = I.segment_override;

		*out_ea = ea & 0xffff;
		*out_segment = segment;
	}
}

static UINT32 GetNonTranslatedEA(UINT8 modrm)
{
	UINT8 segment;
	UINT32 ea;
	modrm_to_EA(modrm, &ea, &segment );
	return ea;
}

static UINT32 GetEA(UINT8 modrm)
{
	UINT8 segment;
	UINT32 ea;
	modrm_to_EA(modrm, &ea, &segment );
	return i386_translate(segment, ea );
}

static void i386_check_sreg_validity(int reg)
{
	UINT16 selector = I.sreg[reg].selector;
	UINT8 CPL = I.CPL;
	UINT8 DPL,RPL;
	I386_SREG desc;
	int invalid = 0;

	memset(&desc, 0, sizeof(desc));
	desc.selector = selector;
	i386_load_protected_mode_segment(&desc);
	DPL = (desc.flags >> 5) & 0x03;
	RPL = selector & 0x03;

	if(selector & 0x04)
	{
		if((selector & ~0x07) > I.ldtr.limit)
			invalid = 1;
	}
	else
	{
		if((selector & ~0x07) > I.gdtr.limit)
			invalid = 1;
	}

	if(((desc.flags & 0x0018) == 0x0018 && (desc.flags & 0x0002)) || (desc.flags & 0x0018) == 0x0010)
		invalid = 0;
	else
		invalid = 1;

	if(((desc.flags & 0x0018) == 0x0018 && (desc.flags & 0x0004) == 0) || (desc.flags & 0x0018) == 0x0010)
	{
		if((DPL < CPL) || (DPL < RPL))
			invalid = 1;
	}

	if(invalid != 0)
	{
		I.sreg[reg].selector = 0;
		i386_load_segment_descriptor(reg);
	}
}

static void i386_trap(int irq, int irq_gate, int trap_level)
{

	UINT32 v1, v2;
	UINT32 offset;
	UINT16 segment;
	int entry = irq * (PROTECTED_MODE ? 8 : 4);
	int SetRPL = 0;

	if( !(PROTECTED_MODE) )
	{

		PUSH16(get_flags() & 0xffff );
		PUSH16(I.sreg[CS].selector );
		if(irq == 3 || irq == 4 || irq == 9 || irq_gate == 1)
			PUSH16(I.eip );
		else
			PUSH16(I.prev_eip );

		I.sreg[CS].selector = READ16(I.idtr.base + entry + 2 );
		I.eip = READ16(I.idtr.base + entry );

		I.TF = 0;
		I.IF = 0;
	}
	else
	{
		int type;
		UINT16 flags;
		I386_SREG desc;
		UINT8 CPL = 0, DPL = 0;
		I386_CALL_GATE gate;

		v1 = READ32(I.idtr.base + entry );
		v2 = READ32(I.idtr.base + entry + 4 );
		offset = (v2 & 0xffff0000) | (v1 & 0xffff);
		segment = (v1 >> 16) & 0xffff;
		type = (v2>>8) & 0x1F;
		flags = (v2>>8) & 0xf0ff;

		if(trap_level == 2)
		{
			logerror("IRQ: Double fault.\n");
			FAULT_EXP(FAULT_DF,0);
		}
		if(trap_level >= 3)
		{
			logerror("IRQ: Triple fault. CPU reset.\n");
			cpunum_set_input_line(cpu_getactivecpu(), INPUT_LINE_RESET, PULSE_LINE);
			return;
		}


		if(entry > I.idtr.limit)
		{
			logerror("IRQ: Vector is past IDT limit.\n");
			FAULT_EXP(FAULT_GP,entry+2)
		}

		if(type != 0x05 && type != 0x06 && type != 0x07 && type != 0x0e && type != 0x0f)
		{
			logerror("IRQ: Vector segment is not an interrupt, trap or task gate.\n");
			FAULT_EXP(FAULT_GP,entry+2)
		}


		if((flags & 0x0080) == 0)
		{
			logerror("IRQ: Vector segment is not present.\n");
			FAULT_EXP(FAULT_NP,entry+2)
		}

		if(type == 0x05)
		{

			memset(&gate, 0, sizeof(gate));
			gate.segment = segment;
			i386_load_call_gate(&gate);
			memset(&desc, 0, sizeof(desc));
			desc.selector = gate.selector;
			i386_load_protected_mode_segment(&desc);
			if(segment & 0x04)
			{
				logerror("IRQ: Task gate: TSS is not in the GDT.\n");
				FAULT_EXP(FAULT_TS,segment & ~0x07);
			}
			else
			{
				if(segment > I.gdtr.limit)
				{
					logerror("IRQ: Task gate: TSS is past GDT limit.\n");
					FAULT_EXP(FAULT_TS,segment & ~0x07);
				}
			}
			if((desc.flags & 0x000f) != 0x09 && (desc.flags & 0x000f) != 0x01)
			{
				logerror("IRQ: Task gate: TSS is not an available TSS.\n");
				FAULT_EXP(FAULT_TS,segment & ~0x07);
			}
			if((desc.flags & 0x0080) == 0)
			{
				logerror("IRQ: Task gate: TSS is not present.\n");
				FAULT_EXP(FAULT_NP,segment & ~0x07);
			}
			if(desc.flags & 0x08)
				i386_task_switch(desc.selector,0);
			else
				i286_task_switch(desc.selector,0);
			return;
		}
		else
		{

			memset(&desc, 0, sizeof(desc));
			desc.selector = segment;
			i386_load_protected_mode_segment(&desc);
			CPL = I.CPL;
			DPL = (desc.flags >> 5) & 0x03;

			if((segment & ~0x07) == 0)
			{
				logerror("IRQ: Gate segment is null.\n");
				FAULT_EXP(FAULT_GP,0)
			}
			if(segment & 0x04)
			{
				if((segment & ~0x07) > I.ldtr.limit)
				{
					logerror("IRQ: Gate segment is past LDT limit.\n");
					FAULT_EXP(FAULT_GP,segment)
				}
			}
			else
			{
				if((segment & ~0x07) > I.gdtr.limit)
				{
					logerror("IRQ: Gate segment is past GDT limit.\n");
					FAULT_EXP(FAULT_GP,segment)
				}
			}
			if((desc.flags & 0x0018) != 0x18)
			{
				logerror("IRQ: Gate descriptor is not a code segment.\n");
				FAULT_EXP(FAULT_GP,segment)
			}
			if((desc.flags & 0x0080) == 0)
			{
				logerror("IRQ: Gate segment is not present.\n");
				FAULT_EXP(FAULT_NP,segment)
			}
			if((desc.flags & 0x0004) == 0 && (DPL < CPL))
			{

				I386_SREG stack;
				UINT32 newESP,oldSS,oldESP;

				memset(&stack, 0, sizeof(stack));
				if(flags & 0x0008)
					stack.selector = i386_get_stack_segment(DPL);
				else
					stack.selector = i286_get_stack_segment(DPL);
				i386_load_protected_mode_segment(&stack);
				oldSS = I.sreg[SS].selector;
				if(flags & 0x0008)
					oldESP = REG32(ESP);
				else
					oldESP = REG16(SP);
				if((stack.selector & ~0x07) == 0)
				{
					logerror("IRQ: New stack selector is null.\n");
					FAULT_EXP(FAULT_GP,0)
				}
				if(stack.selector & 0x04)
				{
					if((stack.selector & ~0x07) > I.ldtr.base)
					{
						logerror("IRQ: New stack selector is past LDT limit.\n");
						FAULT_EXP(FAULT_TS,stack.selector & ~0x07)
					}
				}
				else
				{
					if((stack.selector & ~0x07) > I.gdtr.base)
					{
						logerror("IRQ: New stack selector is past GDT limit.\n");
						FAULT_EXP(FAULT_TS,stack.selector & ~0x07)
					}
				}
				if((stack.selector & 0x03) != DPL)
				{
					logerror("IRQ: New stack selector RPL is not equal to code segment DPL.\n");
					FAULT_EXP(FAULT_TS,stack.selector & ~0x07)
				}
				if(((stack.flags >> 5) & 0x03) != DPL)
				{
					logerror("IRQ: New stack segment DPL is not equal to code segment DPL.\n");
					FAULT_EXP(FAULT_TS,stack.selector & ~0x07)
				}
				if(((stack.flags & 0x0018) != 0x10) && (stack.flags & 0x0002) != 0)
				{
					logerror("IRQ: New stack segment is not a writable data segment.\n");
					FAULT_EXP(FAULT_TS,stack.selector & ~0x07)
				}
				if((stack.flags & 0x0080) == 0)
				{
					logerror("IRQ: New stack segment is not present.\n");
					FAULT_EXP(FAULT_SS,stack.selector & ~0x07)
				}
				if(type & 0x08)
				{
					newESP = i386_get_stack_ptr(DPL);
					if(newESP < 20)
					{
						logerror("IRQ: New stack has no space for return addresses.\n");
						FAULT_EXP(FAULT_SS,0)
					}
				}
				else
				{
					newESP = i286_get_stack_ptr(DPL);
					if(newESP < 10)
					{
						logerror("IRQ: New stack has no space for return addresses.\n");
						FAULT_EXP(FAULT_SS,0)
					}
				}
				if(offset > desc.limit)
				{
					logerror("IRQ: New EIP is past code segment limit.\n");
					FAULT_EXP(FAULT_GP,0)
				}

				I.sreg[SS].selector = stack.selector;
				i386_load_segment_descriptor(SS);
				if(flags & 0x0008)
					REG32(ESP) = i386_get_stack_ptr(DPL);
				else
					REG16(SP) = i286_get_stack_ptr(DPL);
				if(type & 0x08)
				{

					PUSH32(oldSS);
					PUSH32(oldESP);
				}
				else
				{

					PUSH16(oldSS);
					PUSH16(oldESP);
				}
				I.CPL = DPL;
				SetRPL = 1;
			}
			else
			{
				int stack_limit;
				if((desc.flags & 0x0004) || (DPL == CPL))
				{

					if(type == 0x0e || type == 0x0f)
						stack_limit = 10;
					else
						stack_limit = 6;

					if(REG32(ESP) < stack_limit)
					{
						logerror("IRQ: Stack has no space left (needs %i bytes).\n",stack_limit);
						FAULT_EXP(FAULT_SS,0)
					}
					if(offset > desc.limit)
					{
						logerror("IRQ: Gate segment offset is past segment limit.\n");
						FAULT_EXP(FAULT_GP,0)
					}
					SetRPL = 1;
				}
				else
				{
					logerror("IRQ: Gate descriptor is non-conforming, and DPL does not equal CPL.\n");
					FAULT_EXP(FAULT_GP,segment)
				}
			}
		}

		if(type != 0x0e && type != 0x0f)
		{
			PUSH16(get_flags() & 0xffff );
			PUSH16(I.sreg[CS].selector );
			if(irq == 3 || irq == 4 || irq == 9 || irq_gate == 1)
				PUSH16(I.eip );
			else
				PUSH16(I.prev_eip );
		}
		else
		{
			PUSH32(get_flags() & 0x00fcffff );
			PUSH32(I.sreg[CS].selector );
			if(irq == 3 || irq == 4 || irq == 9 || irq_gate == 1)
				PUSH32(I.eip );
			else
				PUSH32(I.prev_eip );
		}
		if(SetRPL != 0)
			segment = (segment & ~0x03) | I.CPL;
		I.sreg[CS].selector = segment;
		I.eip = offset;

		if(type == 0x0e || type == 0x06)
			I.IF = 0;
		I.TF = 0;
		I.NT = 0;
	}

	i386_load_segment_descriptor(CS);
	CHANGE_PC(I.eip);

}

static void i386_trap_with_error(int irq, int irq_gate, int trap_level, UINT32 error)
{
	i386_trap(irq,irq_gate,trap_level);
	if(irq == 8 || irq == 10 || irq == 11 || irq == 12 || irq == 13 || irq == 14)
	{

		PUSH32(error);
	}
}

static void i286_task_switch(UINT16 selector, UINT8 nested)
{
	UINT32 tss;
	I386_SREG seg;
	UINT16 old_task;
	UINT8 ar_byte;



	if(nested == 0)
	{
		if(I.task.segment & 0x0004)
		{
			ar_byte = READ8(I.ldtr.base + (I.task.segment & ~0x0007) + 5);
			WRITE8(I.ldtr.base + (I.task.segment & ~0x0007) + 5,ar_byte & ~0x02);
		}
		else
		{
			ar_byte = READ8(I.gdtr.base + (I.task.segment & ~0x0007) + 5);
			WRITE8(I.gdtr.base + (I.task.segment & ~0x0007) + 5,ar_byte & ~0x02);
		}
	}

	tss = I.task.base;
	WRITE16(tss+0x0e,I.eip & 0x0000ffff);
	WRITE16(tss+0x10,get_flags() & 0x0000ffff);
	WRITE16(tss+0x12,REG16(AX));
	WRITE16(tss+0x14,REG16(CX));
	WRITE16(tss+0x16,REG16(DX));
	WRITE16(tss+0x18,REG16(BX));
	WRITE16(tss+0x1a,REG16(SP));
	WRITE16(tss+0x1c,REG16(BP));
	WRITE16(tss+0x1e,REG16(SI));
	WRITE16(tss+0x20,REG16(DI));
	WRITE16(tss+0x22,I.sreg[ES].selector);
	WRITE16(tss+0x24,I.sreg[CS].selector);
	WRITE16(tss+0x26,I.sreg[SS].selector);
	WRITE16(tss+0x28,I.sreg[DS].selector);

	old_task = I.task.segment;

	I.task.segment = selector;
	memset(&seg, 0, sizeof(seg));
	seg.selector = I.task.segment;
	i386_load_protected_mode_segment(&seg);
	I.task.limit = seg.limit;
	I.task.base = seg.base;
	I.task.flags = seg.flags;

	I.cr[0] |= 0x08;

	tss = I.task.base;
	I.ldtr.segment = READ16(tss+0x2a) & 0xffff;
	seg.selector = I.ldtr.segment;
	i386_load_protected_mode_segment(&seg);
	I.ldtr.limit = seg.limit;
	I.ldtr.base = seg.base;
	I.ldtr.flags = seg.flags;
	I.eip = READ16(tss+0x0e);
	set_flags(READ16(tss+0x10));
	REG16(AX) = READ16(tss+0x12);
	REG16(CX) = READ16(tss+0x14);
	REG16(DX) = READ16(tss+0x16);
	REG16(BX) = READ16(tss+0x18);
	REG16(SP) = READ16(tss+0x1a);
	REG16(BP) = READ16(tss+0x1c);
	REG16(SI) = READ16(tss+0x1e);
	REG16(DI) = READ16(tss+0x20);
	I.sreg[ES].selector = READ16(tss+0x22) & 0xffff;
	i386_load_segment_descriptor(ES);
	I.sreg[CS].selector = READ16(tss+0x24) & 0xffff;
	i386_load_segment_descriptor(CS);
	I.sreg[SS].selector = READ16(tss+0x26) & 0xffff;
	i386_load_segment_descriptor(SS);
	I.sreg[DS].selector = READ16(tss+0x28) & 0xffff;
	i386_load_segment_descriptor(DS);

	if(selector & 0x0004)
	{
		ar_byte = READ8(I.ldtr.base + (selector & ~0x0007) + 5);
		WRITE8(I.ldtr.base + (selector & ~0x0007) + 5,ar_byte | 0x02);
	}
	else
	{
		ar_byte = READ8(I.gdtr.base + (selector & ~0x0007) + 5);
		WRITE8(I.gdtr.base + (selector & ~0x0007) + 5,ar_byte | 0x02);
	}

	if(nested != 0)
	{
		WRITE16(tss+0,old_task);
		I.NT = 1;
	}
	CHANGE_PC(I.eip);

	I.CPL = I.sreg[CS].selector & 0x03;
	printf("286 Task Switch from selector %04x to %04x\n",old_task,selector);
}

static void i386_task_switch(UINT16 selector, UINT8 nested)
{
	UINT32 tss;
	I386_SREG seg;
	UINT16 old_task;
	UINT8 ar_byte;



	if(nested == 0)
	{
		if(I.task.segment & 0x0004)
		{
			ar_byte = READ8(I.ldtr.base + (I.task.segment & ~0x0007) + 5);
			WRITE8(I.ldtr.base + (I.task.segment & ~0x0007) + 5,ar_byte & ~0x02);
		}
		else
		{
			ar_byte = READ8(I.gdtr.base + (I.task.segment & ~0x0007) + 5);
			WRITE8(I.gdtr.base + (I.task.segment & ~0x0007) + 5,ar_byte & ~0x02);
		}
	}

	tss = I.task.base;
	WRITE32(tss+0x1c,I.cr[3]);
	WRITE32(tss+0x20,I.eip);
	WRITE32(tss+0x24,get_flags());
	WRITE32(tss+0x28,REG32(EAX));
	WRITE32(tss+0x2c,REG32(ECX));
	WRITE32(tss+0x30,REG32(EDX));
	WRITE32(tss+0x34,REG32(EBX));
	WRITE32(tss+0x38,REG32(ESP));
	WRITE32(tss+0x3c,REG32(EBP));
	WRITE32(tss+0x40,REG32(ESI));
	WRITE32(tss+0x44,REG32(EDI));
	WRITE32(tss+0x48,I.sreg[ES].selector);
	WRITE32(tss+0x4c,I.sreg[CS].selector);
	WRITE32(tss+0x50,I.sreg[SS].selector);
	WRITE32(tss+0x54,I.sreg[DS].selector);
	WRITE32(tss+0x58,I.sreg[FS].selector);
	WRITE32(tss+0x5c,I.sreg[GS].selector);

	old_task = I.task.segment;

	I.task.segment = selector;
	memset(&seg, 0, sizeof(seg));
	seg.selector = I.task.segment;
	i386_load_protected_mode_segment(&seg);
	I.task.limit = seg.limit;
	I.task.base = seg.base;
	I.task.flags = seg.flags;

	I.cr[0] |= 0x08;

	tss = I.task.base;
	I.ldtr.segment = READ32(tss+0x60) & 0xffff;
	seg.selector = I.ldtr.segment;
	i386_load_protected_mode_segment(&seg);
	I.ldtr.limit = seg.limit;
	I.ldtr.base = seg.base;
	I.ldtr.flags = seg.flags;
	I.cr[3] = READ32(tss+0x1c);
	I.eip = READ32(tss+0x20);
	set_flags(READ32(tss+0x24));
	REG32(EAX) = READ32(tss+0x28);
	REG32(ECX) = READ32(tss+0x2c);
	REG32(EDX) = READ32(tss+0x30);
	REG32(EBX) = READ32(tss+0x34);
	REG32(ESP) = READ32(tss+0x38);
	REG32(EBP) = READ32(tss+0x3c);
	REG32(ESI) = READ32(tss+0x40);
	REG32(EDI) = READ32(tss+0x44);
	I.sreg[ES].selector = READ32(tss+0x48) & 0xffff;
	i386_load_segment_descriptor(ES);
	I.sreg[CS].selector = READ32(tss+0x4c) & 0xffff;
	i386_load_segment_descriptor(CS);
	I.sreg[SS].selector = READ32(tss+0x50) & 0xffff;
	i386_load_segment_descriptor(SS);
	I.sreg[DS].selector = READ32(tss+0x54) & 0xffff;
	i386_load_segment_descriptor(DS);
	I.sreg[FS].selector = READ32(tss+0x58) & 0xffff;
	i386_load_segment_descriptor(FS);
	I.sreg[GS].selector = READ32(tss+0x5c) & 0xffff;
	i386_load_segment_descriptor(GS);

	if(selector & 0x0004)
	{
		ar_byte = READ8(I.ldtr.base + (selector & ~0x0007) + 5);
		WRITE8(I.ldtr.base + (selector & ~0x0007) + 5,ar_byte | 0x02);
	}
	else
	{
		ar_byte = READ8(I.gdtr.base + (selector & ~0x0007) + 5);
		WRITE8(I.gdtr.base + (selector & ~0x0007) + 5,ar_byte | 0x02);
	}

	if(nested != 0)
	{
		WRITE32(tss+0,old_task);
		I.NT = 1;
	}
	CHANGE_PC(I.eip);

	I.CPL = I.sreg[CS].selector & 0x03;
	printf("386 Task Switch from selector %04x to %04x\n",old_task,selector);
}

static void i386_check_irq_line(void)
{

	if ( (I.irq_state) && I.IF )
	{
		I.cycles -= 2;
		i386_trap(I.irq_callback(0), 1, 0);
	}
}

static void i386_protected_mode_jump(UINT16 seg, UINT32 off, int indirect, int operand32)
{
	I386_SREG desc;
	I386_CALL_GATE call_gate;
	UINT8 CPL,DPL,RPL;
	UINT8 SetRPL = 0;
	UINT16 segment = seg;
	UINT32 offset = off;

	if((segment & ~0x07) == 0)
	{
		logerror("JMP: Segment is null.\n");
		FAULT(FAULT_GP,0)
	}

	if((segment & 0x04) == 0)
	{

		if((segment & ~0x07) > (I.gdtr.limit))
		{
			logerror("JMP: Segment is past GDT limit.\n");
			FAULT(FAULT_GP,segment & 0xfffc)
		}
	}
	else
	{

		if((segment & ~0x07) > (I.ldtr.limit))
		{
			logerror("JMP: Segment is past LDT limit.\n");
			FAULT(FAULT_GP,segment & 0xfffc)
		}
	}

	memset(&desc, 0, sizeof(desc));
	desc.selector = segment;
	i386_load_protected_mode_segment(&desc);
	CPL = I.CPL;
	DPL = (desc.flags >> 5) & 0x03;
	RPL = segment & 0x03;
	if((desc.flags & 0x0018) == 0x0018)
	{

		if((desc.flags & 0x0004) == 0)
		{

			SetRPL = 1;
			if(RPL > CPL)
			{
				logerror("JMP: RPL %i is less than CPL %i\n",RPL,CPL);
				FAULT(FAULT_GP,segment & 0xfffc)
			}
			if(DPL != CPL)
			{
				logerror("JMP: DPL %i is not equal CPL %i\n",DPL,CPL);
				FAULT(FAULT_GP,segment & 0xfffc)
			}
		}
		else
		{

			if(DPL > CPL)
			{
				logerror("JMP: DPL %i is less than CPL %i\n",DPL,CPL);
				FAULT(FAULT_GP,segment & 0xfffc)
			}
		}
		if((desc.flags & 0x0080) == 0)
		{
			logerror("JMP: Segment is not present\n");
			FAULT(FAULT_NP,segment & 0xfffc)
		}
		if(offset > desc.limit)
		{
			logerror("JMP: Offset is past segment limit\n");
			FAULT(FAULT_GP,0)
		}
	}
	else
	{
		if((desc.flags & 0x0010) != 0)
		{
			logerror("JMP: Segment is a data segment\n");
			FAULT(FAULT_GP,segment & 0xfffc)
		}
		else
		{
			switch(desc.flags & 0x000f)
			{
			case 0x01:
			case 0x09:
				logerror("JMP: Available 386 TSS at %08x\n",I.pc);
				memset(&desc, 0, sizeof(desc));
				desc.selector = segment;
				i386_load_protected_mode_segment(&desc);
				DPL = (desc.flags >> 5) & 0x03;
				if(DPL < CPL)
				{
					logerror("JMP: TSS: DPL %i is less than CPL %i\n",DPL,CPL);
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if(DPL < RPL)
				{
					logerror("JMP: TSS: DPL %i is less than TSS RPL %i\n",DPL,RPL);
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if((desc.flags & 0x0080) == 0)
				{
					logerror("JMP: TSS: Segment is not present\n");
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if(desc.flags & 0x0008)
					i386_task_switch(desc.selector,0);
				else
					i286_task_switch(desc.selector,0);
				return;
				break;
			case 0x04:
			case 0x0c:
				logerror("JMP: Call gate at %08x\n",I.pc);
				SetRPL = 1;
				memset(&call_gate, 0, sizeof(call_gate));
				call_gate.segment = segment;
				i386_load_call_gate(&call_gate);
				DPL = call_gate.dpl;
				if(DPL < CPL)
				{
					logerror("JMP: Call Gate: DPL %i is less than CPL %i\n",DPL,CPL);
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if(DPL < RPL)
				{
					logerror("JMP: Call Gate: DPL %i is less than RPL %i\n",DPL,RPL);
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if((desc.flags & 0x0080) == 0)
				{
					logerror("JMP: Call Gate: Segment is not present\n");
					FAULT(FAULT_NP,segment & 0xfffc)
				}

				if(call_gate.selector == 0)
				{
					logerror("JMP: Call Gate: Gate selector is null\n");
					FAULT(FAULT_GP,0)
				}
				if(call_gate.selector & 0x04)
				{

					if((call_gate.selector & ~0x07) > I.gdtr.limit)
					{
						logerror("JMP: Call Gate: Gate Selector is past GDT segment limit\n");
						FAULT(FAULT_GP,call_gate.selector & 0xfffc)
					}
				}
				else
				{

					if((call_gate.selector & ~0x07) > I.ldtr.limit)
					{
						logerror("JMP: Call Gate: Gate Selector is past LDT segment limit\n");
						FAULT(FAULT_GP,call_gate.selector & 0xfffc)
					}
				}
				desc.selector = call_gate.selector;
				i386_load_protected_mode_segment(&desc);
				DPL = (desc.flags >> 5) & 0x03;
				if((desc.flags & 0x0018) != 0x18)
				{
					logerror("JMP: Call Gate: Gate does not point to a code segment\n");
					FAULT(FAULT_GP,call_gate.selector & 0xfffc)
				}
				if((desc.flags & 0x0004) == 0)
				{
					if(DPL != CPL)
					{
						logerror("JMP: Call Gate: Gate DPL does not equal CPL\n");
						FAULT(FAULT_GP,call_gate.selector & 0xfffc)
					}
				}
				else
				{
					if(DPL > CPL)
					{
						logerror("JMP: Call Gate: Gate DPL is greater than CPL\n");
						FAULT(FAULT_GP,call_gate.selector & 0xfffc)
					}
				}
				if((desc.flags & 0x0080) == 0)
				{
					logerror("JMP: Call Gate: Gate Segment is not present\n");
					FAULT(FAULT_NP,call_gate.selector & 0xfffc)
				}
				if(call_gate.offset > desc.limit)
				{
					logerror("JMP: Call Gate: Gate offset is past Gate segment limit\n");
					FAULT(FAULT_GP,call_gate.selector & 0xfffc)
				}
				segment = call_gate.selector;
				offset = call_gate.offset;
				break;
			case 0x05:
				logerror("JMP: Task gate at %08x\n",I.pc);
				memset(&call_gate, 0, sizeof(call_gate));
				call_gate.segment = segment;
				i386_load_call_gate(&call_gate);
				DPL = call_gate.dpl;
				if(DPL < CPL)
				{
					logerror("JMP: Task Gate: Gate DPL %i is less than CPL %i\n",DPL,CPL);
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if(DPL < RPL)
				{
					logerror("JMP: Task Gate: Gate DPL %i is less than CPL %i\n",DPL,CPL);
					FAULT(FAULT_GP,segment & 0xfffc)
				}
				if(call_gate.present == 0)
				{
					logerror("JMP: Task Gate: Gate is not present.\n");
					FAULT(FAULT_GP,segment & 0xfffc)
				}

				desc.selector = call_gate.selector;
				i386_load_protected_mode_segment(&desc);
				DPL = (desc.flags >> 5) & 0x03;
				RPL = call_gate.selector & 0x03;
				if(call_gate.selector & 0x04)
				{
					logerror("JMP: Task Gate TSS: TSS must be global.\n");
					FAULT(FAULT_GP,call_gate.selector & 0xfffc)
				}
				else
				{
					if((call_gate.selector & ~0x07) > I.gdtr.limit)
					{
						logerror("JMP: Task Gate TSS: TSS is past GDT limit.\n");
						FAULT(FAULT_GP,call_gate.selector & 0xfffc)
					}
				}
				if((call_gate.ar & 0x000f) == 0x0009 || (call_gate.ar & 0x000f) == 0x0001)
				{
					logerror("JMP: Task Gate TSS: Segment is not an available TSS.\n");
					FAULT(FAULT_GP,call_gate.selector & 0xfffc)
				}
				if(call_gate.present == 0)
				{
					logerror("JMP: Task Gate TSS: TSS is not present.\n");
					FAULT(FAULT_NP,call_gate.selector & 0xfffc)
				}
				if(call_gate.ar & 0x08)
					i386_task_switch(call_gate.selector,0);
				else
					i286_task_switch(call_gate.selector,0);
				return;
				break;
			default:
				logerror("JMP: Invalid segment type (%i) to jump to.\n",desc.flags & 0x000f);
				FAULT(FAULT_GP,segment & 0xfffc)
			}
		}
	}

	if(SetRPL != 0)
		segment = (segment & ~0x03) | I.CPL;
	if(operand32 == 0)
		I.eip = offset & 0x0000ffff;
	else
		I.eip = offset;
	I.sreg[CS].selector = segment;
	I.performed_intersegment_jump = 1;
	i386_load_segment_descriptor(CS);
	CHANGE_PC(I.eip);
}

static void i386_protected_mode_call(UINT16 seg, UINT32 off, int indirect, int operand32)
{
	I386_SREG desc;
	I386_CALL_GATE gate;
	UINT8 SetRPL = 0;
	UINT8 CPL, DPL, RPL;
	UINT16 selector = seg;
	UINT32 offset = off;
	int x;

	if((selector & ~0x07) == 0)
	{
		logerror("CALL: Selector is null.\n");
		FAULT(FAULT_GP,0)
	}
	if(selector & 0x04)
	{
		if((selector & ~0x07) > I.ldtr.limit)
		{
			logerror("CALL: Selector is past LDT limit.\n");
			FAULT(FAULT_GP,selector & ~0x07)
		}
	}
	else
	{
		if((selector & ~0x07) > I.gdtr.limit)
		{
			logerror("CALL: Selector is past GDT limit.\n");
			FAULT(FAULT_GP,selector & ~0x07)
		}
	}

	memset(&desc, 0, sizeof(desc));
	desc.selector = selector;
	i386_load_protected_mode_segment(&desc);
	CPL = I.CPL;
	DPL = (desc.flags >> 5) & 0x03;
	RPL = selector & 0x03;
	if((desc.flags & 0x0018) == 0x18)
	{
		if(desc.flags & 0x0004)
		{

			if(DPL > CPL)
			{
				logerror("CALL: Code segment DPL %i is greater than CPL %i\n",DPL,CPL);
				FAULT(FAULT_GP,selector & ~0x07)
			}
		}
		else
		{

			if(RPL > CPL)
			{
				logerror("CALL: RPL %i is greater than CPL %i\n",RPL,CPL);
				FAULT(FAULT_GP,selector & ~0x07)
			}
			if(DPL != CPL)
			{
				logerror("CALL: Code segment DPL %i is not equal to CPL %i\n",DPL,CPL);
				FAULT(FAULT_GP,selector & ~0x07)
			}
			SetRPL = 1;
		}
		if((desc.flags & 0x0080) == 0)
		{
			logerror("CALL: Code segment is not present.\n");
			FAULT(FAULT_NP,selector & ~0x07)
		}
		if (operand32 != 0)
		{
			if(REG32(ESP) < 8)
			{
				logerror("CALL: Stack has no room for return address.\n");
				FAULT(FAULT_SS,0)
			}
		}
		else
		{
			if(REG16(SP) < 4)
			{
				logerror("CALL: Stack has no room for return address.\n");
				FAULT(FAULT_SS,0)
			}
		}
		if(offset > desc.limit)
		{
			logerror("CALL: EIP is past segment limit.\n");
			FAULT(FAULT_GP,0)
		}
	}
	else
	{

		if(desc.flags & 0x0010)
		{
			logerror("CALL: Segment is a data segment.\n");
			FAULT(FAULT_GP,desc.selector & ~0x07)
		}
		else
		{
			switch(desc.flags & 0x000f)
			{
			case 0x01:
			case 0x09:
				logerror("CALL: Available TSS at %08x\n",I.pc);
				if(DPL < CPL)
				{
					logerror("CALL: TSS: DPL is less than CPL.\n");
					FAULT(FAULT_TS,selector & ~0x07)
				}
				if(DPL < RPL)
				{
					logerror("CALL: TSS: DPL is less than RPL.\n");
					FAULT(FAULT_TS,selector & ~0x07)
				}
				if(desc.flags & 0x0002)
				{
					logerror("CALL: TSS: TSS is busy.\n");
					FAULT(FAULT_TS,selector & ~0x07)
				}
				if(desc.flags & 0x0080)
				{
					logerror("CALL: TSS: Segment is not present.\n");
					FAULT(FAULT_NP,selector & ~0x07)
				}
				if(desc.flags & 0x08)
					i386_task_switch(desc.selector,1);
				else
					i286_task_switch(desc.selector,1);
				return;
				break;
			case 0x04:
			case 0x0c:
				if((desc.flags & 0x000f) == 0x04)
					operand32 = 0;
				else
					operand32 = 1;
				memset(&gate, 0, sizeof(gate));
				gate.segment = selector;
				i386_load_call_gate(&gate);
				DPL = gate.dpl;
				logerror("CALL: Call gate at %08x (%i parameters)\n",I.pc,gate.dword_count);
				if(DPL < CPL)
				{
					logerror("CALL: Call gate DPL %i is less than CPL %i.\n",DPL,CPL);
					FAULT(FAULT_GP,desc.selector & ~0x07)
				}
				if(DPL < RPL)
				{
					logerror("CALL: Call gate DPL %i is less than RPL %i.\n",DPL,RPL);
					FAULT(FAULT_GP,desc.selector & ~0x07)
				}
				if(gate.present == 0)
				{
					logerror("CALL: Call gate is not present.\n");
					FAULT(FAULT_NP,desc.selector & ~0x07)
				}
				desc.selector = gate.selector;
				if((gate.selector & ~0x07) == 0)
				{
					logerror("CALL: Call gate: Segment is null.\n");
					FAULT(FAULT_GP,0)
				}
				if(desc.selector & 0x04)
				{
					if((desc.selector & ~0x07) > I.ldtr.limit)
					{
						logerror("CALL: Call gate: Segment is past LDT limit\n");
						FAULT(FAULT_GP,desc.selector & ~0x07)
					}
				}
				else
				{
					if((desc.selector & ~0x07) > I.gdtr.limit)
					{
						logerror("CALL: Call gate: Segment is past GDT limit\n");
						FAULT(FAULT_GP,desc.selector & ~0x07)
					}
				}
				i386_load_protected_mode_segment(&desc);
				if((desc.flags & 0x0018) != 0x18)
				{
					logerror("CALL: Call gate: Segment is not a code segment.\n");
					FAULT(FAULT_GP,desc.selector & ~0x07)
				}
				DPL = ((desc.flags >> 5) & 0x03);
				if(DPL > CPL)
				{
					logerror("CALL: Call gate: Segment DPL %i is greater than CPL %i.\n",DPL,CPL);
					FAULT(FAULT_GP,desc.selector & ~0x07)
				}
				if(DPL < CPL && (desc.flags & 0x0004) == 0)
				{
					I386_SREG stack;
					I386_SREG temp;
					UINT32 oldSS,oldESP;

					memset(&stack, 0, sizeof(stack));
					if(operand32 != 0)
						stack.selector = i386_get_stack_segment(DPL);
					else
						stack.selector = i286_get_stack_segment(DPL);
					i386_load_protected_mode_segment(&stack);
					if((stack.selector & ~0x07) == 0)
					{
						logerror("CALL: Call gate: TSS selector is null\n");
						FAULT(FAULT_TS,0)
					}
					if(stack.selector & 0x04)
					{
						if((stack.selector & ~0x07) > I.ldtr.limit)
						{
							logerror("CALL: Call gate: TSS selector is past LDT limit\n");
							FAULT(FAULT_TS,stack.selector)
						}
					}
					else
					{
						if((stack.selector & ~0x07) > I.gdtr.limit)
						{
							logerror("CALL: Call gate: TSS selector is past GDT limit\n");
							FAULT(FAULT_TS,stack.selector)
						}
					}
					if((stack.selector & 0x03) != DPL)
					{
						logerror("CALL: Call gate: Stack selector RPL does not equal code segment DPL %i\n",DPL);
						FAULT(FAULT_TS,stack.selector)
					}
					if(((stack.flags >> 5) & 0x03) != DPL)
					{
						logerror("CALL: Call gate: Stack DPL does not equal code segment DPL %i\n",DPL);
						FAULT(FAULT_TS,stack.selector)
					}
					if((stack.flags & 0x0018) != 0x10 && (stack.flags & 0x0002))
					{
						logerror("CALL: Call gate: Stack segment is not a writable data segment\n");
						FAULT(FAULT_TS,stack.selector)
					}
					if((stack.flags & 0x0080) == 0)
					{
						logerror("CALL: Call gate: Stack segment is not a writable data segment\n");
						FAULT(FAULT_SS,stack.selector)
					}
					if(operand32 != 0)
					{
						UINT32 newESP = i386_get_stack_ptr(DPL);
						if(newESP < ((gate.dword_count & 0x1f) + 16))
						{
							logerror("CALL: Call gate: New stack has no room for 32-bit return address and parameters.\n");
							FAULT(FAULT_SS,0)
						}
						if(gate.offset > desc.limit)
						{
							logerror("CALL: Call gate: EIP is past segment limit.\n");
							FAULT(FAULT_GP,0)
						}
					}
					else
					{
						UINT32 newESP = i286_get_stack_ptr(DPL) & 0x0000ffff;
						if(newESP < ((gate.dword_count & 0x1f) + 8))
						{
							logerror("CALL: Call gate: New stack has no room for 16-bit return address and parameters.\n");
							FAULT(FAULT_SS,0)
						}
						if((gate.offset & 0xffff) > desc.limit)
						{
							logerror("CALL: Call gate: IP is past segment limit.\n");
							FAULT(FAULT_GP,0)
						}
					}
					selector = gate.selector;
					offset = gate.offset;

					oldSS = I.sreg[SS].selector;
					if(operand32 != 0)
					{
						oldESP = REG32(ESP);
						I.sreg[SS].selector = i386_get_stack_segment(gate.selector & 0x03);
					}
					else
					{
						oldESP = REG16(SP);
						I.sreg[SS].selector = i286_get_stack_segment(gate.selector & 0x03);
					}
					i386_load_segment_descriptor(SS );
					if(operand32 != 0)
						REG32(ESP) = i386_get_stack_ptr(gate.selector & 0x03);
					else
						REG16(SP) = i286_get_stack_ptr(gate.selector & 0x03) & 0x0000ffff;

					if(operand32 != 0)
					{
						PUSH32(oldSS);
						PUSH32(oldESP);
					}
					else
					{
						PUSH16(oldSS);
						PUSH16(oldESP & 0xffff);
					}

					memset(&temp, 0, sizeof(temp));
					temp.selector = oldSS;
					i386_load_protected_mode_segment(&temp);

					for(x=(gate.dword_count & 0x1f)-1;x>=0;x--)
					{
						UINT32 addr = temp.base + oldESP + (x*2);
						PUSH16(READ16(addr));
					}
					I.CPL = (stack.flags >> 5) & 0x03;
					SetRPL = 1;
				}
				else
				{

					if (operand32 != 0)
					{
						if(REG32(ESP) < 8)
						{
							logerror("CALL: Stack has no room for return address.\n");
							FAULT(FAULT_SS,0)
						}
						selector = gate.selector;
						offset = gate.offset;
					}
					else
					{
						if(REG16(SP) < 4)
						{
							logerror("CALL: Stack has no room for return address.\n");
							FAULT(FAULT_SS,0)
						}
						selector = gate.selector;
						offset = gate.offset & 0xffff;
					}
					if(offset > desc.limit)
					{
						logerror("CALL: EIP is past segment limit.\n");
						FAULT(FAULT_GP,0)
					}
				}
				break;
			case 0x05:
				logerror("CALL: Task gate at %08x\n",I.pc);
				memset(&gate, 0, sizeof(gate));
				gate.segment = selector;
				i386_load_call_gate(&gate);
				DPL = gate.dpl;
				if(DPL < CPL)
				{
					logerror("CALL: Task Gate: Gate DPL is less than CPL.\n");
					FAULT(FAULT_TS,selector & ~0x07)
				}
				if(DPL < RPL)
				{
					logerror("CALL: Task Gate: Gate DPL is less than RPL.\n");
					FAULT(FAULT_TS,selector & ~0x07)
				}
				if(gate.ar & 0x0080)
				{
					logerror("CALL: Task Gate: Gate is not present.\n");
					FAULT(FAULT_NP,selector & ~0x07)
				}

				desc.selector = gate.selector;
				i386_load_protected_mode_segment(&desc);
				if(gate.selector & 0x04)
				{
					logerror("CALL: Task Gate: TSS is not global.\n");
					FAULT(FAULT_TS,gate.selector & ~0x07)
				}
				else
				{
					if((gate.selector & ~0x07) > I.gdtr.limit)
					{
						logerror("CALL: Task Gate: TSS is past GDT limit.\n");
						FAULT(FAULT_TS,gate.selector & ~0x07)
					}
				}
				if(desc.flags & 0x0002)
				{
					logerror("CALL: Task Gate: TSS is busy.\n");
					FAULT(FAULT_TS,gate.selector & ~0x07)
				}
				if(desc.flags & 0x0080)
				{
					logerror("CALL: Task Gate: TSS is not present.\n");
					FAULT(FAULT_NP,gate.selector & ~0x07)
				}
				if(desc.flags & 0x08)
					i386_task_switch(desc.selector,1);
				else
					i286_task_switch(desc.selector,1);
				return;
				break;
			default:
				logerror("CALL: Invalid special segment type (%i) to jump to.\n",desc.flags & 0x000f);
				FAULT(FAULT_GP,selector & ~0x07)
			}
		}
	}

	if(SetRPL != 0)
		selector = (selector & ~0x03) | I.CPL;
	if(operand32 == 0)
	{

		PUSH16(I.sreg[CS].selector );
		PUSH16(I.eip & 0x0000ffff );
		I.sreg[CS].selector = selector;
		I.performed_intersegment_jump = 1;
		I.eip = offset;
		i386_load_segment_descriptor(CS);
	}
	else
	{

		PUSH32(I.sreg[CS].selector );
		PUSH32(I.eip );
		I.sreg[CS].selector = selector;
		I.performed_intersegment_jump = 1;
		I.eip = offset;
		i386_load_segment_descriptor(CS );
	}
	CHANGE_PC(I.eip);
}

static void i386_protected_mode_retf(UINT8 count, UINT8 operand32)
{
	UINT32 newCS, newEIP;
	UINT32 newSS, newESP;
	I386_SREG desc;
	UINT8 CPL, RPL, DPL;

	if(operand32 == 0)
	{
		newEIP = POP16() & 0xffff;
		newCS = POP16() & 0xffff;
		REG16(SP) += count;
	}
	else
	{
		newEIP = POP32();
		newCS = POP32() & 0xffff;
		REG32(ESP) += count;
	}

	memset(&desc, 0, sizeof(desc));
	desc.selector = newCS;
	i386_load_protected_mode_segment(&desc);
	CPL = I.CPL;
	DPL = (desc.flags >> 5) & 0x03;
	RPL = newCS & 0x03;

	if(RPL < CPL)
	{
		logerror("RETF: Return segment RPL is less than CPL.\n");
		FAULT(FAULT_GP,newCS & ~0x07)
	}

	if(RPL == CPL)
	{

		if((newCS & ~0x07) == 0)
		{
			logerror("RETF: Return segment RPL is less than CPL.\n");
			FAULT(FAULT_GP,0)
		}
		if(newCS & 0x04)
		{
			if((newCS & ~0x07) > I.ldtr.limit)
			{
				logerror("RETF: Return segment is past LDT limit.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		else
		{
			if((newCS & ~0x07) > I.gdtr.limit)
			{
				logerror("RETF: Return segment is past GDT limit.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		if((desc.flags & 0x0018) != 0x0018)
		{
			logerror("RETF: Return segment is not a code segment.\n");
			FAULT(FAULT_GP,newCS & ~0x07)
		}
		if(desc.flags & 0x0004)
		{
			if(DPL > CPL)
			{
				logerror("RETF: Conforming code segment DPL is greater than CPL.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		else
		{
			if(DPL != CPL)
			{
				logerror("RETF: Non-conforming code segment DPL does not equal CPL.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		if((desc.flags & 0x0080) == 0)
		{
			logerror("RETF: Code segment is not present.\n");
			FAULT(FAULT_NP,newCS & ~0x07)
		}
		if(newEIP > desc.limit)
		{
			logerror("RETF: EIP is past code segment limit.\n");
			FAULT(FAULT_GP,0)
		}
		if(operand32 == 0)
		{
			if(REG16(SP) > (I.sreg[SS].limit & 0xffff))
			{
				logerror("RETF: SP is past stack segment limit.\n");
				FAULT(FAULT_SS,0)
			}
		}
		else
		{
			if(REG32(ESP) > I.sreg[SS].limit)
			{
				logerror("RETF: ESP is past stack segment limit.\n");
				FAULT(FAULT_SS,0)
			}
		}
	}
	else if(RPL > CPL)
	{

		if(operand32 == 0)
		{
			newESP = POP16() & 0xffff;
			newSS = POP16();
			if(newESP+8+count > I.sreg[SS].limit)
			{
				logerror("RETF: SP is past stack segment limit.\n");
				FAULT(FAULT_SS,0)
			}
		}
		else
		{
			newESP = POP32();
			newSS = POP32();
			if(newESP+16+count > I.sreg[SS].limit)
			{
				logerror("RETF: ESP is past stack segment limit.\n");
				FAULT(FAULT_SS,0)
			}
		}

		if((newCS & ~0x07) == 0)
		{
			logerror("RETF: CS segment is null.\n");
			FAULT(FAULT_GP,0)
		}
		if(newCS & 0x04)
		{
			if((newCS & ~0x07) > I.ldtr.limit)
			{
				logerror("RETF: CS segment selector is past LDT limit.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		else
		{
			if((newCS & ~0x07) > I.gdtr.limit)
			{
				logerror("RETF: CS segment selector is past GDT limit.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		if((desc.flags & 0x0018) != 0x0018)
		{
			logerror("RETF: CS segment is not a code segment.\n");
			FAULT(FAULT_GP,newCS & ~0x07)
		}
		if(desc.flags & 0x0004)
		{
			if(DPL > RPL)
			{
				logerror("RETF: Conforming CS segment DPL is greater than return selector RPL.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		else
		{
			if(DPL != RPL)
			{
				logerror("RETF: Non-conforming CS segment DPL is not equal to return selector RPL.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
		}
		if((desc.flags & 0x0080) == 0)
		{
			logerror("RETF: CS segment is not present.\n");
			FAULT(FAULT_NP,newCS & ~0x07)
		}
		if(newEIP > desc.limit)
		{
			logerror("RETF: EIP is past return CS segment limit.\n");
			FAULT(FAULT_GP,0)
		}

		desc.selector = newSS;
		i386_load_protected_mode_segment(&desc);
		DPL = (desc.flags >> 5) & 0x03;
		if((newSS & ~0x07) == 0)
		{
			logerror("RETF: SS segment is null.\n");
			FAULT(FAULT_GP,0)
		}
		if(newSS & 0x04)
		{
			if((newSS & ~0x07) > I.ldtr.limit)
			{
				logerror("RETF: SS segment selector is past LDT limit.\n");
				FAULT(FAULT_GP,newSS & ~0x07)
			}
		}
		else
		{
			if((newSS & ~0x07) > I.gdtr.limit)
			{
				logerror("RETF: SS segment selector is past GDT limit.\n");
				FAULT(FAULT_GP,newSS & ~0x07)
			}
		}
		if((newSS & 0x03) != RPL)
		{
			logerror("RETF: SS segment RPL is not equal to CS segment RPL.\n");
			FAULT(FAULT_GP,newSS & ~0x07)
		}
		if((desc.flags & 0x0018) != 0x0010 || (desc.flags & 0x0002) == 0)
		{
			logerror("RETF: SS segment is not a writable data segment.\n");
			FAULT(FAULT_GP,newSS & ~0x07)
		}
		if(((desc.flags >> 5) & 0x03) != RPL)
		{
			logerror("RETF: SS DPL is not equal to CS segment RPL.\n");
			FAULT(FAULT_GP,newSS & ~0x07)
		}
		if((desc.flags & 0x0080) == 0)
		{
			logerror("RETF: SS segment is not present.\n");
			FAULT(FAULT_GP,newSS & ~0x07)
		}
		I.CPL = newCS & 0x03;

		if(operand32 == 0)
			REG16(SP) = newESP & 0xffff;
		else
			REG32(ESP) = newESP;
		I.sreg[SS].selector = newSS;
		i386_load_segment_descriptor(SS );

		if(operand32 == 0)
			REG16(SP) += count;
		else
			REG32(ESP) += count;

		i386_check_sreg_validity(DS);
		i386_check_sreg_validity(ES);
		i386_check_sreg_validity(FS);
		i386_check_sreg_validity(GS);
	}

	if(operand32 == 0)
		I.eip = newEIP & 0xffff;
	else
		I.eip = newEIP;
	I.sreg[CS].selector = newCS;
	i386_load_segment_descriptor(CS );
	CHANGE_PC(I.eip);
}

static void i386_protected_mode_iret(int operand32)
{
	UINT32 newCS, newEIP;
	UINT32 newSS, newESP;
	I386_SREG desc;
	UINT8 CPL, RPL, DPL;
	UINT32 newflags;

	CPL = I.CPL;
	if(operand32 == 0)
	{
		newEIP = POP16();
		newCS = POP16();
		newflags = POP16();
	}
	else
	{
		newEIP = POP32();
		newCS = POP32() & 0xffff;
		newflags = POP32();
	}

	if(V8086_MODE)
	{
		ui_popup("IRET: Is in Virtual 8086 mode.\n");
		FAULT(FAULT_GP,0)
	}
	else if(NESTED_TASK)
	{
		UINT32 task = READ32(I.task.base);

		ui_popup("IRET: Nested task return.");

		if(task & 0x04)
		{
			ui_popup("IRET: Task return: Back-linked TSS is not in GDT.\n");
			FAULT(FAULT_TS,task & ~0x07)
		}
		if((task & ~0x07) > I.gdtr.limit)
		{
			ui_popup("IRET: Task return: Back-linked TSS is not in GDT.\n");
			FAULT(FAULT_TS,task & ~0x07)
		}
		memset(&desc, 0, sizeof(desc));
		desc.selector = task;
		i386_load_protected_mode_segment(&desc);
		if((desc.flags & 0x001f) != 0x000b)
		{
			ui_popup("IRET: Task return: Back-linked TSS is not a busy TSS.\n");
			FAULT(FAULT_TS,task & ~0x07)
		}
		if((desc.flags & 0x0080) == 0)
		{
			ui_popup("IRET: Task return: Back-linked TSS is not present.\n");
			FAULT(FAULT_NP,task & ~0x07)
		}
		if(desc.flags & 0x08)
			i386_task_switch(desc.selector,0);
		else
			i286_task_switch(desc.selector,0);
		return;
	}
	else
	{
		if(newflags & 0x00020000)
		{

			ui_popup("IRET: Unimplemented return to Virtual 8086 mode.");
		}
		else
		{
			if(operand32 == 0)
			{
				if(REG16(SP)+4 > I.sreg[SS].limit)
				{
					ui_popup("IRET: Data on stack is past SS limit.\n");
					FAULT(FAULT_SS,0)
				}
			}
			else
			{
				if(REG32(ESP)+6 > I.sreg[SS].limit)
				{
					ui_popup("IRET: Data on stack is past SS limit.\n");
					FAULT(FAULT_SS,0)
				}
			}
			RPL = newCS & 0x03;
			if(RPL < CPL)
			{
				ui_popup("IRET: Return CS RPL is less than CPL.\n");
				FAULT(FAULT_GP,newCS & ~0x07)
			}
			if(RPL == CPL)
			{

				if(operand32 == 0)
				{
					if(REG16(SP)+6 > I.sreg[SS].limit)
					{
						ui_popup("IRET: Data on stack is past SS limit.\n");
						FAULT(FAULT_SS,0)
					}
				}
				else
				{
					if(REG32(ESP)+12 > I.sreg[SS].limit)
					{
						ui_popup("IRET: Data on stack is past SS limit.\n");
						FAULT(FAULT_SS,0)
					}
				}
				if((newCS & ~0x07) == 0)
				{
					ui_popup("IRET: Return CS selector is null.\n");
					FAULT(FAULT_GP,0)
				}
				if(newCS & 0x04)
				{
					if((newCS & ~0x07) > I.ldtr.limit)
					{
						ui_popup("IRET: Return CS selector (%04x) is past LDT limit.\n",newCS);
						FAULT(FAULT_GP,newCS & ~0x07)
					}
				}
				else
				{
					if((newCS & ~0x07) > I.gdtr.limit)
					{
						ui_popup("IRET: Return CS selector is past GDT limit.\n");
						FAULT(FAULT_GP,newCS & ~0x07)
					}
				}
				memset(&desc, 0, sizeof(desc));
				desc.selector = newCS;
				i386_load_protected_mode_segment(&desc);
				DPL = (desc.flags >> 5) & 0x03;
				RPL = newCS & 0x03;
				if((desc.flags & 0x0018) != 0x0018)
				{
					ui_popup("IRET: Return CS segment is not a code segment.\n");
					FAULT(FAULT_GP,newCS & ~0x07)
				}
				if(desc.flags & 0x0004)
				{
					if(DPL > CPL)
					{
						ui_popup("IRET: Conforming return CS DPL is greater than CPL.\n");
						FAULT(FAULT_GP,newCS & ~0x07)
					}
				}
				else
				{
					if(DPL != CPL)
					{
						ui_popup("IRET: Non-conforming return CS DPL is not equal to CPL.\n");
						FAULT(FAULT_GP,newCS & ~0x07)
					}
				}
				if((desc.flags & 0x0080) == 0)
				{
					ui_popup("IRET: Return CS segment is not present.\n");
					FAULT(FAULT_NP,newCS & ~0x07)
				}
				if(newEIP > desc.limit)
				{
					ui_popup("IRET: Return EIP is past return CS limit.\n");
					FAULT(FAULT_GP,0)				}

				if(operand32 == 0)
				{
					I.eip = newEIP;
					I.sreg[CS].selector = newCS;
					set_flags(newflags);
				}
				else
				{
					I.eip = newEIP;
					I.sreg[CS].selector = newCS & 0xffff;
					set_flags(newflags);
				}
			}
			else if(RPL > CPL)
			{
				I386_SREG stack;

				if(operand32 == 0)
				{
					newESP = POP16();
					newSS = POP16();
					if(REG16(SP) > I.sreg[SS].limit)
					{
						ui_popup("IRET: ESP is past SS limit.\n");
						FAULT(FAULT_SS,0)					}
				}
				else
				{
					newESP = POP32();
					newSS = POP32() & 0xffff;
					if(REG32(ESP) > I.sreg[SS].limit)
					{
						ui_popup("IRET: ESP is past SS limit.\n");
						FAULT(FAULT_SS,0)
					}
				}
				memset(&desc, 0, sizeof(desc));
				desc.selector = newCS;
				i386_load_protected_mode_segment(&desc);
				DPL = (desc.flags >> 5) & 0x03;
				RPL = newCS & 0x03;
				memset(&stack, 0, sizeof(stack));
				stack.selector = newSS;
				i386_load_protected_mode_segment(&stack);

				if((newCS & ~0x07) == 0)
				{
					ui_popup("IRET: Return CS selector is null.\n");
					FAULT(FAULT_GP,0)
				}
				if(newCS & 0x04)
				{
					if((newCS & ~0x07) > I.ldtr.limit)
					{
						ui_popup("IRET: Return CS selector is past LDT limit.\n");
						FAULT(FAULT_GP,newCS & ~0x07);
					}
				}
				else
				{
					if((newCS & ~0x07) > I.gdtr.limit)
					{
						ui_popup("IRET: Return CS selector is past GDT limit.\n");
						FAULT(FAULT_GP,newCS & ~0x07);
					}
				}
				if((desc.flags & 0x0018) != 0x0018)
				{
					ui_popup("IRET: Return CS segment is not a code segment.\n");
					FAULT(FAULT_GP,newCS & ~0x07)
				}
				if(desc.flags & 0x0004)
				{
					if(DPL <= CPL)
					{
						ui_popup("IRET: Conforming return CS DPL is not greater than CPL.\n");
						FAULT(FAULT_GP,newCS & ~0x07)
					}
				}
				else
				{
					if(DPL != RPL)
					{
						ui_popup("IRET: Non-conforming return CS DPL does not equal CS RPL.\n");
						FAULT(FAULT_GP,newCS & ~0x07)
					}
				}
				if((desc.flags & 0x0080) == 0)
				{
					ui_popup("IRET: Return CS segment is not present.\n");
					FAULT(FAULT_NP,newCS & ~0x07)
				}

				DPL = (stack.flags >> 5) & 0x03;
				if((newSS & ~0x07) == 0)
				{
					ui_popup("IRET: Return SS selector is null.\n");
					FAULT(FAULT_GP,0)
				}
				if(newSS & 0x04)
				{
					if((newSS & ~0x07) > I.ldtr.limit)
					{
						ui_popup("IRET: Return SS selector is past LDT limit.\n");
						FAULT(FAULT_GP,newSS & ~0x07);
					}
				}
				else
				{
					if((newSS & ~0x07) > I.gdtr.limit)
					{
						ui_popup("IRET: Return SS selector is past GDT limit.\n");
						FAULT(FAULT_GP,newSS & ~0x07);
					}
				}
				if((newSS & 0x03) != RPL)
				{
					ui_popup("IRET: Return SS RPL is not equal to return CS RPL.\n");
					FAULT(FAULT_GP,newSS & ~0x07)
				}
				if((stack.flags & 0x0018) != 0x0010)
				{
					ui_popup("IRET: Return SS segment is not a data segment.\n");
					FAULT(FAULT_GP,newSS & ~0x07)
				}
				if((stack.flags & 0x0002) == 0)
				{
					ui_popup("IRET: Return SS segment is not writable.\n");
					FAULT(FAULT_GP,newSS & ~0x07)
				}
				if(DPL != RPL)
				{
					ui_popup("IRET: Return SS DPL does not equal SS RPL.\n");
					FAULT(FAULT_GP,newSS & ~0x07)
				}
				if((stack.flags & 0x0080) == 0)
				{
					ui_popup("IRET: Return SS segment is not present.\n");
					FAULT(FAULT_NP,newSS & ~0x07)
				}
				if(newEIP > desc.limit)
				{
					ui_popup("IRET: EIP is past return CS limit.\n");
					FAULT(FAULT_GP,0)
				}

				if(operand32 == 0)
				{
					I.eip = newEIP & 0xffff;
					I.sreg[CS].selector = newCS;
					set_flags(newflags);
					REG16(SP) = newESP & 0xffff;
					I.sreg[SS].selector = newSS;
				}
				else
				{
					I.eip = newEIP;
					I.sreg[CS].selector = newCS & 0xffff;
					set_flags(newflags);
					REG32(ESP) = newESP;
					I.sreg[SS].selector = newSS & 0xffff;
				}
				I.CPL = newCS & 0x03;
				i386_load_segment_descriptor(SS);

				i386_check_sreg_validity(DS);
				i386_check_sreg_validity(ES);
				i386_check_sreg_validity(FS);
				i386_check_sreg_validity(GS);
			}
		}
	}

	i386_load_segment_descriptor(CS);
	CHANGE_PC(I.eip);
}

#include "cycles.h"

static UINT8 *cycle_table_rm[X86_NUM_CPUS];
static UINT8 *cycle_table_pm[X86_NUM_CPUS];

#define CYCLES_NUM(x)	(I.cycles -= (x))

INLINE void CYCLES(int x)
{
	if (PROTECTED_MODE)
	{
		I.cycles -= I.cycle_table_pm[x];
	}
	else
	{
		I.cycles -= I.cycle_table_rm[x];
	}
}

INLINE void CYCLES_RM(int modrm, int r, int m)
{
	if (modrm >= 0xc0)
	{
		if (PROTECTED_MODE)
		{
			I.cycles -= I.cycle_table_pm[r];
		}
		else
		{
			I.cycles -= I.cycle_table_rm[r];
		}
	}
	else
	{
		if (PROTECTED_MODE)
		{
			I.cycles -= I.cycle_table_pm[m];
		}
		else
		{
			I.cycles -= I.cycle_table_rm[m];
		}
	}
}

static void build_cycle_table(void)
{
	int i, j;
	for (j=0; j < X86_NUM_CPUS; j++)
	{
		cycle_table_rm[j] = auto_malloc(CYCLES_NUM_OPCODES);
		cycle_table_pm[j] = auto_malloc(CYCLES_NUM_OPCODES);

		for (i=0; i < sizeof(x86_cycle_table)/sizeof(X86_CYCLE_TABLE); i++)
		{
			int opcode = x86_cycle_table[i].op;
			cycle_table_rm[j][opcode] = x86_cycle_table[i].cpu_cycles[j][0];
			cycle_table_pm[j][opcode] = x86_cycle_table[i].cpu_cycles[j][1];
		}
	}
}

static void I386OP(decode_opcode)(void);
static void I386OP(decode_two_byte)(void);


#include "i386ops.c"
#include "i386op16.c"
#include "i386op32.c"
#include "i486ops.c"
#include "pentops.c"
#include "x87ops.c"
#include "i386ops.h"

static void I386OP(decode_opcode)(void)
{
	I.opcode = FETCH();
	if( I.operand_size )
		I.opcode_table1_32[I.opcode]();
	else
		I.opcode_table1_16[I.opcode]();
}

static void I386OP(decode_two_byte)(void)
{
	I.opcode = FETCH();
	if( I.operand_size )
		I.opcode_table2_32[I.opcode]();
	else
		I.opcode_table2_16[I.opcode]();
}


static void i386_postload(void)
{
	/* Hidden segment caches can differ from descriptors and must survive a restore. */
	CHANGE_PC(I.eip);
}

void i386_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	int i, j;
	static const int regs8[8] = {AL,CL,DL,BL,AH,CH,DH,BH};
	static const int regs16[8] = {AX,CX,DX,BX,SP,BP,SI,DI};
	static const int regs32[8] = {EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI};
	static const char state_type[] = "I386";

	build_cycle_table();

	for( i=0; i < 256; i++ ) {
		int c=0;
		for( j=0; j < 8; j++ ) {
			if( i & (1 << j) )
				c++;
		}
		i386_parity_table[i] = ~(c & 0x1) & 0x1;
	}

	for( i=0; i < 256; i++ ) {
		i386_MODRM_table[i].reg.b = regs8[(i >> 3) & 0x7];
		i386_MODRM_table[i].reg.w = regs16[(i >> 3) & 0x7];
		i386_MODRM_table[i].reg.d = regs32[(i >> 3) & 0x7];

		i386_MODRM_table[i].rm.b = regs8[i & 0x7];
		i386_MODRM_table[i].rm.w = regs16[i & 0x7];
		i386_MODRM_table[i].rm.d = regs32[i & 0x7];
	}

	I.irq_callback = irqcallback;

	state_save_register_item_array(state_type, index, I.reg.d);
	state_save_register_item_array(state_type, index, I.cr);
	state_save_register_item_array(state_type, index, I.dr);
	state_save_register_item_array(state_type, index, I.tr);
	state_save_register_item(state_type, index, I.eip);
	state_save_register_item(state_type, index, I.prev_eip);
	state_save_register_item(state_type, index, I.eflags);
	state_save_register_item(state_type, index, I.CF);
	state_save_register_item(state_type, index, I.DF);
	state_save_register_item(state_type, index, I.SF);
	state_save_register_item(state_type, index, I.OF);
	state_save_register_item(state_type, index, I.ZF);
	state_save_register_item(state_type, index, I.PF);
	state_save_register_item(state_type, index, I.AF);
	state_save_register_item(state_type, index, I.IF);
	state_save_register_item(state_type, index, I.TF);
	state_save_register_item(state_type, index, I.IOP1);
	state_save_register_item(state_type, index, I.IOP2);
	state_save_register_item(state_type, index, I.NT);
	state_save_register_item(state_type, index, I.CPL);
	state_save_register_item(state_type, index, I.irq_state);
	state_save_register_item(state_type, index, I.performed_intersegment_jump);
	state_save_register_item(state_type, index, I.delayed_interrupt_enable);
	state_save_register_item(state_type, index, I.halted);
	state_save_register_item(state_type, index, I.a20_mask);
	state_save_register_item(state_type, index, I.tsc);
	state_save_register_item(state_type, index, I.idtr.base);
	state_save_register_item(state_type, index, I.idtr.limit);
	state_save_register_item(state_type, index, I.gdtr.base);
	state_save_register_item(state_type, index, I.gdtr.limit);
	state_save_register_item(state_type, index, I.task.base);
	state_save_register_item(state_type, index, I.task.segment);
	state_save_register_item(state_type, index, I.task.limit);
	state_save_register_item(state_type, index, I.task.flags);
	state_save_register_item(state_type, index, I.ldtr.base);
	state_save_register_item(state_type, index, I.ldtr.segment);
	state_save_register_item(state_type, index, I.ldtr.limit);
	state_save_register_item(state_type, index, I.ldtr.flags);
	state_save_register_item(state_type, index, I.fpu_control_word);
	state_save_register_item(state_type, index, I.fpu_status_word);
	state_save_register_item(state_type, index, I.fpu_tag_word);
	state_save_register_item(state_type, index, I.fpu_data_ptr);
	state_save_register_item(state_type, index, I.fpu_inst_ptr);
	state_save_register_item(state_type, index, I.fpu_opcode);
	state_save_register_item(state_type, index, I.fpu_top);
	state_save_register_item(state_type, index, I.sreg[ES].selector);
	state_save_register_item(state_type, index, I.sreg[ES].base);
	state_save_register_item(state_type, index, I.sreg[ES].limit);
	state_save_register_item(state_type, index, I.sreg[ES].flags);
	state_save_register_item(state_type, index, I.sreg[ES].d);
	state_save_register_item(state_type, index, I.sreg[CS].selector);
	state_save_register_item(state_type, index, I.sreg[CS].base);
	state_save_register_item(state_type, index, I.sreg[CS].limit);
	state_save_register_item(state_type, index, I.sreg[CS].flags);
	state_save_register_item(state_type, index, I.sreg[CS].d);
	state_save_register_item(state_type, index, I.sreg[SS].selector);
	state_save_register_item(state_type, index, I.sreg[SS].base);
	state_save_register_item(state_type, index, I.sreg[SS].limit);
	state_save_register_item(state_type, index, I.sreg[SS].flags);
	state_save_register_item(state_type, index, I.sreg[SS].d);
	state_save_register_item(state_type, index, I.sreg[DS].selector);
	state_save_register_item(state_type, index, I.sreg[DS].base);
	state_save_register_item(state_type, index, I.sreg[DS].limit);
	state_save_register_item(state_type, index, I.sreg[DS].flags);
	state_save_register_item(state_type, index, I.sreg[DS].d);
	state_save_register_item(state_type, index, I.sreg[FS].selector);
	state_save_register_item(state_type, index, I.sreg[FS].base);
	state_save_register_item(state_type, index, I.sreg[FS].limit);
	state_save_register_item(state_type, index, I.sreg[FS].flags);
	state_save_register_item(state_type, index, I.sreg[FS].d);
	state_save_register_item(state_type, index, I.sreg[GS].selector);
	state_save_register_item(state_type, index, I.sreg[GS].base);
	state_save_register_item(state_type, index, I.sreg[GS].limit);
	state_save_register_item(state_type, index, I.sreg[GS].flags);
	state_save_register_item(state_type, index, I.sreg[GS].d);
	state_save_register_item(state_type, index, I.fpu_reg[0].i);
	state_save_register_item(state_type, index, I.fpu_reg[1].i);
	state_save_register_item(state_type, index, I.fpu_reg[2].i);
	state_save_register_item(state_type, index, I.fpu_reg[3].i);
	state_save_register_item(state_type, index, I.fpu_reg[4].i);
	state_save_register_item(state_type, index, I.fpu_reg[5].i);
	state_save_register_item(state_type, index, I.fpu_reg[6].i);
	state_save_register_item(state_type, index, I.fpu_reg[7].i);
	state_save_register_func_postload(i386_postload);
}

static void build_opcode_table(UINT32 features)
{
	int i;
	for (i=0; i < 256; i++)
	{
		I.opcode_table1_16[i] = I386OP(invalid);
		I.opcode_table1_32[i] = I386OP(invalid);
		I.opcode_table2_16[i] = I386OP(invalid);
		I.opcode_table2_32[i] = I386OP(invalid);
	}

	for (i=0; i < sizeof(x86_opcode_table)/sizeof(X86_OPCODE); i++)
	{
		const X86_OPCODE *op = &x86_opcode_table[i];

		if ((op->flags & features))
		{
			if (op->flags & OP_2BYTE)
			{
				I.opcode_table2_32[op->opcode] = op->handler32;
				I.opcode_table2_16[op->opcode] = op->handler16;
			}
			else
			{
				I.opcode_table1_32[op->opcode] = op->handler32;
				I.opcode_table1_16[op->opcode] = op->handler16;
			}
		}
	}
}

static void i386_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset(&I, 0, sizeof(I));
	I.irq_callback = save_irqcallback;

	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;

	I.sreg[DS].base = I.sreg[ES].base = I.sreg[FS].base = I.sreg[GS].base = I.sreg[SS].base = 0x00000000;
	I.sreg[DS].limit = I.sreg[ES].limit = I.sreg[FS].limit = I.sreg[GS].limit = I.sreg[SS].limit = 0xffff;
	I.sreg[DS].flags = I.sreg[ES].flags = I.sreg[FS].flags = I.sreg[GS].flags = I.sreg[SS].flags = 0x0092;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0x7fffffe0;
	I.eflags = 0;
	I.eflags_mask = 0x00030000;
	I.eip = 0xfff0;



	REG32(EAX) = 0;
	REG32(EDX) = (3 << 8) | (0 << 4) | (8);

	I.CPL = 0;

	build_opcode_table(OP_I386);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_I386];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_I386];

	CHANGE_PC(I.eip);
}

static void i386_get_context(void *dst)
{
	if(dst) {
		*(I386_REGS *)dst = I;
	}
}

static void i386_set_context(void *src)
{
	if(src) {
		I = *(I386_REGS *)src;
	}

	CHANGE_PC(I.eip);
}

static void i386_set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE && I.halted)
	{
		I.halted = 0;
	}

	if ( irqline == INPUT_LINE_NMI )
	{

		if ( state )
			i386_trap(2, 1, 0);
	}
	else
	{
		I.irq_state = state;
	}
}

static void i386_set_a20_line(int state)
{
	if (state)
	{
		I.a20_mask = ~0;
	}
	else
	{
		I.a20_mask = ~(1 << 20);
	}
}

int i386_execute(int num_cycles)
{

	int cycles = num_cycles;
	I.cycles = num_cycles;
	I.base_cycles = num_cycles;
	CHANGE_PC(I.eip);

	if (I.halted)
	{
		I.tsc += cycles;
		I.cycles = 0;
		return num_cycles;
	}

	while( I.cycles > 0 )
	{
		I.operand_size = I.sreg[CS].d;
		I.address_size = I.sreg[CS].d;

		I.segment_prefix = 0;
		I.prev_eip = I.eip;

		CALL_MAME_DEBUG;

		i386_check_irq_line();
		if(I.delayed_interrupt_enable != 0)
		{
			I.IF = 1;
			I.delayed_interrupt_enable = 0;
		}
		I386OP(decode_opcode)();
	}
	I.tsc += (cycles - I.cycles);
	return num_cycles - I.cycles;
}

/*************************************************************************/

static UINT8 i386_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		0
};

static UINT8 i386_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/*************************************************************************/

static int translate_address_cb(int space, offs_t *addr)
{
	int result = 1;
	if (space == ADDRESS_SPACE_PROGRAM)
	{
		if (I.cr[0] & 0x80000000)
			result = translate_address(addr);
		*addr &= I.a20_mask;
	}
	return result;
}

static offs_t i386_dasm(char *buffer, offs_t pc, UINT8 *oprom, UINT8 *opram, int bytes)
{
#ifdef MAME_DEBUG
	return i386_dasm_one(buffer, pc, oprom, I.sreg[CS].d, I.sreg[CS].d);
#else
	sprintf( buffer, "$%02X", *oprom );
	return 1;
#endif
}

static void i386_set_info(UINT32 state, union cpuinfo *info)
{
	if (state == CPUINFO_INT_INPUT_STATE+INPUT_LINE_A20)
	{
		i386_set_a20_line(info->i);
		return;
	}
	if (state >= CPUINFO_INT_INPUT_STATE && state <= CPUINFO_INT_INPUT_STATE + 1)
	{
		i386_set_irq_line(state-CPUINFO_INT_INPUT_STATE, info->i);
		return;
	}

	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I386_PC:			I.pc = info->i;break;
		case CPUINFO_INT_REGISTER + I386_EIP:			I.eip = info->i; CHANGE_PC(I.eip); break;
		case CPUINFO_INT_REGISTER + I386_AL:			REG8(AL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_AH:			REG8(AH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BL:			REG8(BL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BH:			REG8(BH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CL:			REG8(CL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CH:			REG8(CH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DL:			REG8(DL) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DH:			REG8(DH) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_AX:			REG16(AX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BX:			REG16(BX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CX:			REG16(CX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DX:			REG16(DX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_SI:			REG16(SI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DI:			REG16(DI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_BP:			REG16(BP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_SP:			REG16(SP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_IP:			I.eip = (I.eip & ~0xFFFF) | (info->i & 0xFFFF); CHANGE_PC(I.eip); break;
		case CPUINFO_INT_REGISTER + I386_EAX:			REG32(EAX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EBX:			REG32(EBX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_ECX:			REG32(ECX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EDX:			REG32(EDX) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EBP:			REG32(EBP) = info->i; break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + I386_ESP:			REG32(ESP) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_ESI:			REG32(ESI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EDI:			REG32(EDI) = info->i; break;
		case CPUINFO_INT_REGISTER + I386_EFLAGS:		set_flags(info->i); break;
		case CPUINFO_INT_REGISTER + I386_CS:			I.sreg[CS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_SS:			I.sreg[SS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_DS:			I.sreg[DS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_ES:			I.sreg[ES].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_FS:			I.sreg[FS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_GS:			I.sreg[GS].selector = info->i & 0xffff; break;
		case CPUINFO_INT_REGISTER + I386_CR0:			I.cr[0] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR1:			I.cr[1] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR2:			I.cr[2] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_CR3:			I.cr[3] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR0:			I.dr[0] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR1:			I.dr[1] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR2:			I.dr[2] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR3:			I.dr[3] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR4:			I.dr[4] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR5:			I.dr[5] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR6:			I.dr[6] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_DR7:			I.dr[7] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_TR6:			I.tr[6] = info->i; break;
		case CPUINFO_INT_REGISTER + I386_TR7:			I.tr[7] = info->i; break;
	}
}

void i386_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:				info->i = sizeof(I386_REGS);			break;
		case CPUINFO_INT_INPUT_LINES:				info->i = 32;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:		info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:				info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:				info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:		info->i = 1;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:		info->i = 8;							break;
		case CPUINFO_INT_MIN_CYCLES:				info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:				info->i = 40;							break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_LOGADDR_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_PAGE_SHIFT + ADDRESS_SPACE_PROGRAM: 	info->i = 12;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE:			info->i = CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:				/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I386_PC:			info->i = I.pc; break;
		case CPUINFO_INT_REGISTER + I386_EIP:			info->i = I.eip; break;
		case CPUINFO_INT_REGISTER + I386_AL:			info->i = REG8(AL); break;
		case CPUINFO_INT_REGISTER + I386_AH:			info->i = REG8(AH); break;
		case CPUINFO_INT_REGISTER + I386_BL:			info->i = REG8(BL); break;
		case CPUINFO_INT_REGISTER + I386_BH:			info->i = REG8(BH); break;
		case CPUINFO_INT_REGISTER + I386_CL:			info->i = REG8(CL); break;
		case CPUINFO_INT_REGISTER + I386_CH:			info->i = REG8(CH); break;
		case CPUINFO_INT_REGISTER + I386_DL:			info->i = REG8(DL); break;
		case CPUINFO_INT_REGISTER + I386_DH:			info->i = REG8(DH); break;
		case CPUINFO_INT_REGISTER + I386_AX:			info->i = REG16(AX); break;
		case CPUINFO_INT_REGISTER + I386_BX:			info->i = REG16(BX); break;
		case CPUINFO_INT_REGISTER + I386_CX:			info->i = REG16(CX); break;
		case CPUINFO_INT_REGISTER + I386_DX:			info->i = REG16(DX); break;
		case CPUINFO_INT_REGISTER + I386_SI:			info->i = REG16(SI); break;
		case CPUINFO_INT_REGISTER + I386_DI:			info->i = REG16(DI); break;
		case CPUINFO_INT_REGISTER + I386_BP:			info->i = REG16(BP); break;
		case CPUINFO_INT_REGISTER + I386_SP:			info->i = REG16(SP); break;
		case CPUINFO_INT_REGISTER + I386_IP:			info->i = I.eip & 0xFFFF; break;
		case CPUINFO_INT_REGISTER + I386_EAX:			info->i = REG32(EAX); break;
		case CPUINFO_INT_REGISTER + I386_EBX:			info->i = REG32(EBX); break;
		case CPUINFO_INT_REGISTER + I386_ECX:			info->i = REG32(ECX); break;
		case CPUINFO_INT_REGISTER + I386_EDX:			info->i = REG32(EDX); break;
		case CPUINFO_INT_REGISTER + I386_EBP:			info->i = REG32(EBP); break;
		case CPUINFO_INT_REGISTER + I386_ESP:			info->i = REG32(ESP); break;
		case CPUINFO_INT_REGISTER + I386_ESI:			info->i = REG32(ESI); break;
		case CPUINFO_INT_REGISTER + I386_EDI:			info->i = REG32(EDI); break;
		case CPUINFO_INT_REGISTER + I386_EFLAGS:		info->i = get_flags(); break;
		case CPUINFO_INT_REGISTER + I386_CS:			info->i = I.sreg[CS].selector; break;
		case CPUINFO_INT_REGISTER + I386_SS:			info->i = I.sreg[SS].selector; break;
		case CPUINFO_INT_REGISTER + I386_DS:			info->i = I.sreg[DS].selector; break;
		case CPUINFO_INT_REGISTER + I386_ES:			info->i = I.sreg[ES].selector; break;
		case CPUINFO_INT_REGISTER + I386_FS:			info->i = I.sreg[FS].selector; break;
		case CPUINFO_INT_REGISTER + I386_GS:			info->i = I.sreg[GS].selector; break;
		case CPUINFO_INT_REGISTER + I386_CR0:			info->i = I.cr[0]; break;
		case CPUINFO_INT_REGISTER + I386_CR1:			info->i = I.cr[1]; break;
		case CPUINFO_INT_REGISTER + I386_CR2:			info->i = I.cr[2]; break;
		case CPUINFO_INT_REGISTER + I386_CR3:			info->i = I.cr[3]; break;
		case CPUINFO_INT_REGISTER + I386_DR0:			info->i = I.dr[0]; break;
		case CPUINFO_INT_REGISTER + I386_DR1:			info->i = I.dr[1]; break;
		case CPUINFO_INT_REGISTER + I386_DR2:			info->i = I.dr[2]; break;
		case CPUINFO_INT_REGISTER + I386_DR3:			info->i = I.dr[3]; break;
		case CPUINFO_INT_REGISTER + I386_DR4:			info->i = I.dr[4]; break;
		case CPUINFO_INT_REGISTER + I386_DR5:			info->i = I.dr[5]; break;
		case CPUINFO_INT_REGISTER + I386_DR6:			info->i = I.dr[6]; break;
		case CPUINFO_INT_REGISTER + I386_DR7:			info->i = I.dr[7]; break;
		case CPUINFO_INT_REGISTER + I386_TR6:			info->i = I.tr[6]; break;
		case CPUINFO_INT_REGISTER + I386_TR7:			info->i = I.tr[7]; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = i386_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = i386_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = i386_set_context;	break;
		case CPUINFO_PTR_INIT:		      				info->init = i386_init;			break;
		case CPUINFO_PTR_RESET:		      				info->reset = i386_reset;		break;
		case CPUINFO_PTR_EXECUTE:	      				info->execute = i386_execute;		break;
		case CPUINFO_PTR_BURN:		      				info->burn = NULL;			break;
		case CPUINFO_PTR_DISASSEMBLE_NEW:				info->disassemble_new = i386_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER: 			info->icount = &I.cycles;		break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i386_reg_layout;		break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i386_win_layout;		break;
		case CPUINFO_PTR_TRANSLATE:						info->translate = translate_address_cb;	break;
#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
		case CPUINFO_PTR_DEBUG_SETUP_COMMANDS:			info->setup_commands = i386_debug_setup; break;
#endif

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "I386"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 386"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) 2003-2004 Ville Linde"); break;

		case CPUINFO_STR_FLAGS:	   				sprintf(info->s = cpuintrf_temp_str(), "%08X", get_flags()); break;

		case CPUINFO_STR_REGISTER + I386_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC: %08X", I.pc); break;
		case CPUINFO_STR_REGISTER + I386_EIP:			sprintf(info->s = cpuintrf_temp_str(), "EIP: %08X", I.eip); break;
		case CPUINFO_STR_REGISTER + I386_AL:			sprintf(info->s = cpuintrf_temp_str(), "~AL: %02X", REG8(AL)); break;
		case CPUINFO_STR_REGISTER + I386_AH:			sprintf(info->s = cpuintrf_temp_str(), "~AH: %02X", REG8(AH)); break;
		case CPUINFO_STR_REGISTER + I386_BL:			sprintf(info->s = cpuintrf_temp_str(), "~BL: %02X", REG8(BL)); break;
		case CPUINFO_STR_REGISTER + I386_BH:			sprintf(info->s = cpuintrf_temp_str(), "~BH: %02X", REG8(BH)); break;
		case CPUINFO_STR_REGISTER + I386_CL:			sprintf(info->s = cpuintrf_temp_str(), "~CL: %02X", REG8(CL)); break;
		case CPUINFO_STR_REGISTER + I386_CH:			sprintf(info->s = cpuintrf_temp_str(), "~CH: %02X", REG8(CH)); break;
		case CPUINFO_STR_REGISTER + I386_DL:			sprintf(info->s = cpuintrf_temp_str(), "~DL: %02X", REG8(DL)); break;
		case CPUINFO_STR_REGISTER + I386_DH:			sprintf(info->s = cpuintrf_temp_str(), "~DH: %02X", REG8(DH)); break;
		case CPUINFO_STR_REGISTER + I386_AX:			sprintf(info->s = cpuintrf_temp_str(), "~AX: %04X", REG16(AX)); break;
		case CPUINFO_STR_REGISTER + I386_BX:			sprintf(info->s = cpuintrf_temp_str(), "~BX: %04X", REG16(BX)); break;
		case CPUINFO_STR_REGISTER + I386_CX:			sprintf(info->s = cpuintrf_temp_str(), "~CX: %04X", REG16(CX)); break;
		case CPUINFO_STR_REGISTER + I386_DX:			sprintf(info->s = cpuintrf_temp_str(), "~DX: %04X", REG16(DX)); break;
		case CPUINFO_STR_REGISTER + I386_SI:			sprintf(info->s = cpuintrf_temp_str(), "~SI: %04X", REG16(SI)); break;
		case CPUINFO_STR_REGISTER + I386_DI:			sprintf(info->s = cpuintrf_temp_str(), "~DI: %04X", REG16(DI)); break;
		case CPUINFO_STR_REGISTER + I386_BP:			sprintf(info->s = cpuintrf_temp_str(), "~BP: %04X", REG16(BP)); break;
		case CPUINFO_STR_REGISTER + I386_SP:			sprintf(info->s = cpuintrf_temp_str(), "~SP: %04X", REG16(SP)); break;
		case CPUINFO_STR_REGISTER + I386_IP:			sprintf(info->s = cpuintrf_temp_str(), "~IP: %04X", I.eip & 0xFFFF); break;
		case CPUINFO_STR_REGISTER + I386_EAX:			sprintf(info->s = cpuintrf_temp_str(), "EAX: %08X", I.reg.d[EAX]); break;
		case CPUINFO_STR_REGISTER + I386_EBX:			sprintf(info->s = cpuintrf_temp_str(), "EBX: %08X", I.reg.d[EBX]); break;
		case CPUINFO_STR_REGISTER + I386_ECX:			sprintf(info->s = cpuintrf_temp_str(), "ECX: %08X", I.reg.d[ECX]); break;
		case CPUINFO_STR_REGISTER + I386_EDX:			sprintf(info->s = cpuintrf_temp_str(), "EDX: %08X", I.reg.d[EDX]); break;
		case CPUINFO_STR_REGISTER + I386_EBP:			sprintf(info->s = cpuintrf_temp_str(), "EBP: %08X", I.reg.d[EBP]); break;
		case CPUINFO_STR_REGISTER + I386_ESP:			sprintf(info->s = cpuintrf_temp_str(), "ESP: %08X", I.reg.d[ESP]); break;
		case CPUINFO_STR_REGISTER + I386_ESI:			sprintf(info->s = cpuintrf_temp_str(), "ESI: %08X", I.reg.d[ESI]); break;
		case CPUINFO_STR_REGISTER + I386_EDI:			sprintf(info->s = cpuintrf_temp_str(), "EDI: %08X", I.reg.d[EDI]); break;
		case CPUINFO_STR_REGISTER + I386_EFLAGS:		sprintf(info->s = cpuintrf_temp_str(), "EFLAGS: %08X", I.eflags); break;
		case CPUINFO_STR_REGISTER + I386_CS:			sprintf(info->s = cpuintrf_temp_str(), "CS: %04X:%08X", I.sreg[CS].selector, I.sreg[CS].base); break;
		case CPUINFO_STR_REGISTER + I386_SS:			sprintf(info->s = cpuintrf_temp_str(), "SS: %04X:%08X", I.sreg[SS].selector, I.sreg[SS].base); break;
		case CPUINFO_STR_REGISTER + I386_DS:			sprintf(info->s = cpuintrf_temp_str(), "DS: %04X:%08X", I.sreg[DS].selector, I.sreg[DS].base); break;
		case CPUINFO_STR_REGISTER + I386_ES:			sprintf(info->s = cpuintrf_temp_str(), "ES: %04X:%08X", I.sreg[ES].selector, I.sreg[ES].base); break;
		case CPUINFO_STR_REGISTER + I386_FS:			sprintf(info->s = cpuintrf_temp_str(), "FS: %04X:%08X", I.sreg[FS].selector, I.sreg[FS].base); break;
		case CPUINFO_STR_REGISTER + I386_GS:			sprintf(info->s = cpuintrf_temp_str(), "GS: %04X:%08X", I.sreg[GS].selector, I.sreg[GS].base); break;
		case CPUINFO_STR_REGISTER + I386_CR0:			sprintf(info->s = cpuintrf_temp_str(), "CR0: %08X", I.cr[0]); break;
		case CPUINFO_STR_REGISTER + I386_CR1:			sprintf(info->s = cpuintrf_temp_str(), "CR1: %08X", I.cr[1]); break;
		case CPUINFO_STR_REGISTER + I386_CR2:			sprintf(info->s = cpuintrf_temp_str(), "CR2: %08X", I.cr[2]); break;
		case CPUINFO_STR_REGISTER + I386_CR3:			sprintf(info->s = cpuintrf_temp_str(), "CR3: %08X", I.cr[3]); break;
		case CPUINFO_STR_REGISTER + I386_DR0:			sprintf(info->s = cpuintrf_temp_str(), "DR0: %08X", I.dr[0]); break;
		case CPUINFO_STR_REGISTER + I386_DR1:			sprintf(info->s = cpuintrf_temp_str(), "DR1: %08X", I.dr[1]); break;
		case CPUINFO_STR_REGISTER + I386_DR2:			sprintf(info->s = cpuintrf_temp_str(), "DR2: %08X", I.dr[2]); break;
		case CPUINFO_STR_REGISTER + I386_DR3:			sprintf(info->s = cpuintrf_temp_str(), "DR3: %08X", I.dr[3]); break;
		case CPUINFO_STR_REGISTER + I386_DR4:			sprintf(info->s = cpuintrf_temp_str(), "DR4: %08X", I.dr[4]); break;
		case CPUINFO_STR_REGISTER + I386_DR5:			sprintf(info->s = cpuintrf_temp_str(), "DR5: %08X", I.dr[5]); break;
		case CPUINFO_STR_REGISTER + I386_DR6:			sprintf(info->s = cpuintrf_temp_str(), "DR6: %08X", I.dr[6]); break;
		case CPUINFO_STR_REGISTER + I386_DR7:			sprintf(info->s = cpuintrf_temp_str(), "DR7: %08X", I.dr[7]); break;
		case CPUINFO_STR_REGISTER + I386_TR6:			sprintf(info->s = cpuintrf_temp_str(), "TR6: %08X", I.tr[6]); break;
		case CPUINFO_STR_REGISTER + I386_TR7:			sprintf(info->s = cpuintrf_temp_str(), "TR7: %08X", I.tr[7]); break;
	}
}

/*****************************************************************************/
/* Intel 486 */

#if (HAS_I486)

static UINT8 i486_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		-1,
	X87_CTRL,		X87_STATUS,		-1,
	X87_ST0,		X87_ST1,		-1,
	X87_ST2,		X87_ST3,		-1,
	X87_ST4,		X87_ST5,		-1,
	X87_ST6,		X87_ST7,		0
};

static UINT8 i486_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void i486_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	i386_init(index, clock, config, irqcallback);
}

static void i486_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset(&I, 0, sizeof(I));
	I.irq_callback = save_irqcallback;

	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;
	I.sreg[CS].flags    = 0x009b;

	I.sreg[DS].base = I.sreg[ES].base = I.sreg[FS].base = I.sreg[GS].base = I.sreg[SS].base = 0x00000000;
	I.sreg[DS].limit = I.sreg[ES].limit = I.sreg[FS].limit = I.sreg[GS].limit = I.sreg[SS].limit = 0xffff;
	I.sreg[DS].flags = I.sreg[ES].flags = I.sreg[FS].flags = I.sreg[GS].flags = I.sreg[SS].flags = 0x0092;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0x00000010;
	I.eflags = 0;
	I.eflags_mask = 0x00070000;
	I.eip = 0xfff0;



	REG32(EAX) = 0;
	REG32(EDX) = (4 << 8) | (0 << 4) | (3);

	build_opcode_table(OP_I386 | OP_FPU | OP_I486);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_I486];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_I486];

	CHANGE_PC(I.eip);
}

static void i486_exit(void)
{

}

static void i486_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_REGISTER + X87_CTRL:			I.fpu_control_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			I.fpu_status_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			ST(0).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			ST(1).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			ST(2).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			ST(3).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			ST(4).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			ST(5).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			ST(6).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			ST(7).f = info->i; break;

		default: i386_set_info(state, info); break;
	}
}

void i486_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = i486_set_info;		break;
		case CPUINFO_PTR_INIT:		      				info->init = i486_init;				break;
		case CPUINFO_PTR_RESET:		      				info->reset = i486_reset;			break;
		case CPUINFO_PTR_EXIT:		      				info->exit = i486_exit;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = i486_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = i486_win_layout;			break;

		case CPUINFO_INT_REGISTER + X87_CTRL:			info->i = I.fpu_control_word; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			info->i = I.fpu_status_word; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			info->i = ST(0).f; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			info->i = ST(1).f; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			info->i = ST(2).f; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			info->i = ST(3).f; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			info->i = ST(4).f; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			info->i = ST(5).f; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			info->i = ST(6).f; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			info->i = ST(7).f; break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "I486"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel 486"); break;
		case CPUINFO_STR_REGISTER + X87_CTRL:			sprintf(info->s = cpuintrf_temp_str(), "FPU_CW: %04X", I.fpu_control_word); break;
		case CPUINFO_STR_REGISTER + X87_STATUS:			sprintf(info->s = cpuintrf_temp_str(), "FPU_SW: %04X", I.fpu_status_word); break;
		case CPUINFO_STR_REGISTER + X87_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0: %f", ST(0).f); break;
		case CPUINFO_STR_REGISTER + X87_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1: %f", ST(1).f); break;
		case CPUINFO_STR_REGISTER + X87_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2: %f", ST(2).f); break;
		case CPUINFO_STR_REGISTER + X87_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3: %f", ST(3).f); break;
		case CPUINFO_STR_REGISTER + X87_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4: %f", ST(4).f); break;
		case CPUINFO_STR_REGISTER + X87_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5: %f", ST(5).f); break;
		case CPUINFO_STR_REGISTER + X87_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6: %f", ST(6).f); break;
		case CPUINFO_STR_REGISTER + X87_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7: %f", ST(7).f); break;

		default:	i386_get_info(state, info); break;
	}
}
#endif

/*****************************************************************************/
/* Pentium */

#if (HAS_PENTIUM)

static UINT8 pentium_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		-1,
	X87_CTRL,		X87_STATUS,		-1,
	X87_ST0,		X87_ST1,		-1,
	X87_ST2,		X87_ST3,		-1,
	X87_ST4,		X87_ST5,		-1,
	X87_ST6,		X87_ST7,		0
};

static UINT8 pentium_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void pentium_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	i386_init(index, clock, config, irqcallback);
}

static void pentium_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset(&I, 0, sizeof(I));
	I.irq_callback = save_irqcallback;

	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;
	I.sreg[CS].flags    = 0x009b;

	I.sreg[DS].base = I.sreg[ES].base = I.sreg[FS].base = I.sreg[GS].base = I.sreg[SS].base = 0x00000000;
	I.sreg[DS].limit = I.sreg[ES].limit = I.sreg[FS].limit = I.sreg[GS].limit = I.sreg[SS].limit = 0xffff;
	I.sreg[DS].flags = I.sreg[ES].flags = I.sreg[FS].flags = I.sreg[GS].flags = I.sreg[SS].flags = 0x0092;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0x00000010;
	I.eflags = 0;
	I.eflags_mask = 0x003b0000;
	I.eip = 0xfff0;



	REG32(EAX) = 0;
	REG32(EDX) = (5 << 8) | (2 << 4) | (5);

	build_opcode_table(OP_I386 | OP_FPU | OP_I486 | OP_PENTIUM);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_PENTIUM];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_PENTIUM];

	I.cpuid_id0 = 0x756e6547;
	I.cpuid_id1 = 0x49656e69;
	I.cpuid_id2 = 0x6c65746e;

	I.cpuid_max_input_value_eax = 0x01;
	I.cpu_version = REG32(EDX);





	I.feature_flags = 0x000001bf;

	CHANGE_PC(I.eip);
}

static void pentium_exit(void)
{

}

static void pentium_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_REGISTER + X87_CTRL:			I.fpu_control_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			I.fpu_status_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			ST(0).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			ST(1).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			ST(2).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			ST(3).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			ST(4).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			ST(5).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			ST(6).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			ST(7).f = info->i; break;

		default: i386_set_info(state, info); break;
	}
}

void pentium_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = pentium_set_info;		break;
		case CPUINFO_PTR_INIT:		      				info->init = pentium_init;				break;
		case CPUINFO_PTR_RESET:		      				info->reset = pentium_reset;			break;
		case CPUINFO_PTR_EXIT:		      				info->exit = pentium_exit;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = pentium_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = pentium_win_layout;			break;

		case CPUINFO_INT_REGISTER + X87_CTRL:			info->i = I.fpu_control_word; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			info->i = I.fpu_status_word; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			info->i = ST(0).f; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			info->i = ST(1).f; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			info->i = ST(2).f; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			info->i = ST(3).f; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			info->i = ST(4).f; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			info->i = ST(5).f; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			info->i = ST(6).f; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			info->i = ST(7).f; break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "PENTIUM"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Intel Pentium"); break;
		case CPUINFO_STR_REGISTER + X87_CTRL:			sprintf(info->s = cpuintrf_temp_str(), "FPU_CW: %04X", I.fpu_control_word); break;
		case CPUINFO_STR_REGISTER + X87_STATUS:			sprintf(info->s = cpuintrf_temp_str(), "FPU_SW: %04X", I.fpu_status_word); break;
		case CPUINFO_STR_REGISTER + X87_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0: %f", ST(0).f); break;
		case CPUINFO_STR_REGISTER + X87_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1: %f", ST(1).f); break;
		case CPUINFO_STR_REGISTER + X87_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2: %f", ST(2).f); break;
		case CPUINFO_STR_REGISTER + X87_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3: %f", ST(3).f); break;
		case CPUINFO_STR_REGISTER + X87_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4: %f", ST(4).f); break;
		case CPUINFO_STR_REGISTER + X87_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5: %f", ST(5).f); break;
		case CPUINFO_STR_REGISTER + X87_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6: %f", ST(6).f); break;
		case CPUINFO_STR_REGISTER + X87_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7: %f", ST(7).f); break;

		default:	i386_get_info(state, info); break;
	}
}
#endif

/*****************************************************************************/
/* Cyrix MediaGX */

#if (HAS_MEDIAGX)

static UINT8 mediagx_reg_layout[] =
{
	I386_EIP,		I386_ESP,		-1,
	I386_EAX,		I386_EBP,		-1,
	I386_EBX,		I386_ESI,		-1,
	I386_ECX,		I386_EDI,		-1,
	I386_EDX,		-1,
	I386_CS,		I386_CR0,		-1,
	I386_SS,		I386_CR1,		-1,
	I386_DS,		I386_CR2,		-1,
	I386_ES,		I386_CR3,		-1,
	I386_FS,		I386_TR6,		-1,
	I386_GS,		I386_TR7,		-1,
	I386_DR0,		I386_DR1,		-1,
	I386_DR2,		I386_DR3,		-1,
	I386_DR4,		I386_DR5,		-1,
	I386_DR6,		I386_DR7,		-1,
	X87_CTRL,		X87_STATUS,		-1,
	X87_ST0,		X87_ST1,		-1,
	X87_ST2,		X87_ST3,		-1,
	X87_ST4,		X87_ST5,		-1,
	X87_ST6,		X87_ST7,		0
};

static UINT8 mediagx_win_layout[] =
{
	 0, 0,32,12,	/* register window (top rows) */
	33, 0,46,15,	/* disassembler window (left colums) */
	33,10,46,12,	/* memory #2 window (right, lower middle) */
	 0,19,32, 3,	/* memory #1 window (right, upper middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void mediagx_init(int index, int clock, const void *config, int (*irqcallback)(int))
{
	i386_init(index, clock, config, irqcallback);
}

static void mediagx_reset(void)
{
	int (*save_irqcallback)(int);

	save_irqcallback = I.irq_callback;
	memset(&I, 0, sizeof(I));
	I.irq_callback = save_irqcallback;

	I.sreg[CS].selector = 0xf000;
	I.sreg[CS].base		= 0xffff0000;
	I.sreg[CS].limit	= 0xffff;
	I.sreg[CS].flags    = 0x009b;

	I.sreg[DS].base = I.sreg[ES].base = I.sreg[FS].base = I.sreg[GS].base = I.sreg[SS].base = 0x00000000;
	I.sreg[DS].limit = I.sreg[ES].limit = I.sreg[FS].limit = I.sreg[GS].limit = I.sreg[SS].limit = 0xffff;
	I.sreg[DS].flags = I.sreg[ES].flags = I.sreg[FS].flags = I.sreg[GS].flags = I.sreg[SS].flags = 0x0092;

	I.idtr.base = 0;
	I.idtr.limit = 0x3ff;

	I.a20_mask = ~0;

	I.cr[0] = 0x00000010;
	I.eflags = 0;
	I.eflags_mask = 0x00270000;
	I.eip = 0xfff0;



	REG32(EAX) = 0;
	REG32(EDX) = (4 << 8) | (4 << 4) | (1);

	build_opcode_table(OP_I386 | OP_FPU | OP_I486 | OP_PENTIUM | OP_CYRIX);
	I.cycle_table_rm = cycle_table_rm[CPU_CYCLES_MEDIAGX];
	I.cycle_table_pm = cycle_table_pm[CPU_CYCLES_MEDIAGX];

	I.cpuid_id0 = 0x69727943;
	I.cpuid_id1 = 0x736e4978;
	I.cpuid_id2 = 0x6d616574;

	I.cpuid_max_input_value_eax = 0x01;
	I.cpu_version = REG32(EDX);

	I.feature_flags = 0x00000001;

	CHANGE_PC(I.eip);
}

static void mediagx_exit(void)
{

}

static void mediagx_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_INT_REGISTER + X87_CTRL:			I.fpu_control_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			I.fpu_status_word = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			ST(0).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			ST(1).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			ST(2).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			ST(3).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			ST(4).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			ST(5).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			ST(6).f = info->i; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			ST(7).f = info->i; break;

		default: i386_set_info(state, info); break;
	}
}

void mediagx_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		case CPUINFO_PTR_SET_INFO:	      				info->setinfo = mediagx_set_info;		break;
		case CPUINFO_PTR_INIT:		      				info->init = mediagx_init;				break;
		case CPUINFO_PTR_RESET:		      				info->reset = mediagx_reset;			break;
		case CPUINFO_PTR_EXIT:		      				info->exit = mediagx_exit;				break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = mediagx_reg_layout;			break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = mediagx_win_layout;			break;

		case CPUINFO_INT_REGISTER + X87_CTRL:			info->i = I.fpu_control_word; break;
		case CPUINFO_INT_REGISTER + X87_STATUS:			info->i = I.fpu_status_word; break;
		case CPUINFO_INT_REGISTER + X87_ST0:			info->i = ST(0).f; break;
		case CPUINFO_INT_REGISTER + X87_ST1:			info->i = ST(1).f; break;
		case CPUINFO_INT_REGISTER + X87_ST2:			info->i = ST(2).f; break;
		case CPUINFO_INT_REGISTER + X87_ST3:			info->i = ST(3).f; break;
		case CPUINFO_INT_REGISTER + X87_ST4:			info->i = ST(4).f; break;
		case CPUINFO_INT_REGISTER + X87_ST5:			info->i = ST(5).f; break;
		case CPUINFO_INT_REGISTER + X87_ST6:			info->i = ST(6).f; break;
		case CPUINFO_INT_REGISTER + X87_ST7:			info->i = ST(7).f; break;

		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "MEDIAGX"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Cyrix MediaGX"); break;
		case CPUINFO_STR_REGISTER + X87_CTRL:			sprintf(info->s = cpuintrf_temp_str(), "FPU_CW: %04X", I.fpu_control_word); break;
		case CPUINFO_STR_REGISTER + X87_STATUS:			sprintf(info->s = cpuintrf_temp_str(), "FPU_SW: %04X", I.fpu_status_word); break;
		case CPUINFO_STR_REGISTER + X87_ST0:			sprintf(info->s = cpuintrf_temp_str(), "ST0: %f", ST(0).f); break;
		case CPUINFO_STR_REGISTER + X87_ST1:			sprintf(info->s = cpuintrf_temp_str(), "ST1: %f", ST(1).f); break;
		case CPUINFO_STR_REGISTER + X87_ST2:			sprintf(info->s = cpuintrf_temp_str(), "ST2: %f", ST(2).f); break;
		case CPUINFO_STR_REGISTER + X87_ST3:			sprintf(info->s = cpuintrf_temp_str(), "ST3: %f", ST(3).f); break;
		case CPUINFO_STR_REGISTER + X87_ST4:			sprintf(info->s = cpuintrf_temp_str(), "ST4: %f", ST(4).f); break;
		case CPUINFO_STR_REGISTER + X87_ST5:			sprintf(info->s = cpuintrf_temp_str(), "ST5: %f", ST(5).f); break;
		case CPUINFO_STR_REGISTER + X87_ST6:			sprintf(info->s = cpuintrf_temp_str(), "ST6: %f", ST(6).f); break;
		case CPUINFO_STR_REGISTER + X87_ST7:			sprintf(info->s = cpuintrf_temp_str(), "ST7: %f", ST(7).f); break;

		default:	i386_get_info(state, info); break;
	}
}
#endif
