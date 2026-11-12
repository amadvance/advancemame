// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/*
  Driver based on the initial reverse-engineering of Elsemi, extended,
  generalized and made to look more like a cpu since then thanks in
  part to a "manual" that barely deserves the name.

  The 86232 has 512 32-bits dwords of triple-port memory (1 write, 2
  read).  The 86233/86234 have instead two normal (1 read, 1 write,
  non-simultaneous) independent ram banks, one of 256 dwords and one
  of 512.

  The ram banks are mapped at 0x000-0x0ff and 0x200-0x3ff (proven by
  geometrizer code that clears the ram at startup).  Move and load
  instructions kind of target a specific ram, but do it by adding
  0x200 to the address on one side of the other, which can then end up
  anywhere.  In particular model1 coprocessor has the output fifo at
  0x400, which is sometimes hit by having x1 at 0x200 and using the
  automatic 0x200 adder.  Theorically external accesses to 100-1ff and
  400+ seem to be routed externally, since they're used for the fifo
  in model 1.

  The cpu can theorically work in either floating point (32-bits ieee
  flots) or fixed point (32/36/48 bits registers) modes.  All sega
  programs start by activating floating point and staying there, so
  fixed point is not implemented.

  An interrupt is used to update the rf0 (status? leds?) registers in
  the coprocessor programs.  It's on bit 1 of the mask (irq3?) and
  vector 0x004.  It's probably periodic, maybe on vblank.  Note that
  the copro programs never initialize the stack pointer.  Interrupts
  are not implemented at this point.

  The 86233 and 86234 dies are slightly different in the die shots,
  but there's no known programming-level difference at this point.
  It's unclear whether some register-file linked functionality is
  internal or external though (fifos, banking in model2/86234), so
  there may lie the actual differences.

  Note: in the event of two writes occurring at the same register in a
  single instruction, priority appears to vary depending on the ALU
  operation. Transfers appear to take precedence over integer ops, while
  floating point ops take precedence over transfers. This may be a result
  of the ALU unit taking more than one cycle to execute floating point ops
  (86232 datasheet), hence the register is updated after the transfer.
*/

/*
    Differences from MAME:

    Written in C. Floating point mode only, as in MAME. The program, data,
    io and register file address spaces are the callbacks of struct
    mb86233_config (given with MDRV_CPU_CONFIG) instead of memory maps. Only
    one TGP per machine is supported (the state is static).
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "driver.h"
#include "cpuintrf.h"
#include "debugger.h"
#include "state.h"
#include "mb86233.h"

#define SEXT( v, bits )		( (INT32)( (UINT32)( v ) << ( 32 - ( bits ) ) ) >> ( 32 - ( bits ) ) )

enum {
	F_ZRC  = 0x00000001, F_ZRD  = 0x00000002, F_SGC  = 0x00000004, F_SGD  = 0x00000008,
	F_CPC  = 0x00000010, F_CPD  = 0x00000020, F_OVC  = 0x00000040, F_OVD  = 0x00000080,
	F_UNC  = 0x00000100, F_UND  = 0x00000200, F_DVZC = 0x00000400, F_DVZD = 0x00000800,
	F_CA   = 0x00001000, F_CPP  = 0x00002000, F_OVM  = 0x00004000, F_UNM  = 0x00008000,
	F_SIF0 = 0x00010000, F_SIF1 = 0x00020000, F_SOF0 = 0x00040000, F_PIF  = 0x00100000,
	F_POF  = 0x00200000, F_PAIF = 0x00400000, F_PAOF = 0x00800000, F_F0S  = 0x01000000,
	F_F1S  = 0x02000000, F_IT   = 0x04000000, F_ZX0  = 0x08000000, F_ZX1  = 0x10000000,
	F_ZX2  = 0x20000000, F_ZC0  = 0x40000000
};
#define F_ZC1 0x80000000

typedef struct
{
	UINT32 st, a, b, d, p;
	UINT32 alu_stmask, alu_stset, alu_r1, alu_r2;
	UINT16 ppc, pc, sp, b0, b1, x0, x1, i0, i1, vsmr, pcs[ 4 ], mask, m;
	UINT8 r, rpc, c0, c1, sft, vsm;
	int gpio[ 4 ];
	int stall;
	struct mb86233_config *cfg;
} mb86233_regs;

static mb86233_regs mb;
int mb86233_icount;

#define m_st			mb.st
#define m_a				mb.a
#define m_b				mb.b
#define m_d				mb.d
#define m_p				mb.p
#define m_alu_stmask	mb.alu_stmask
#define m_alu_stset		mb.alu_stset
#define m_alu_r1		mb.alu_r1
#define m_alu_r2		mb.alu_r2
#define m_ppc			mb.ppc
#define m_pc			mb.pc
#define m_sp			mb.sp
#define m_b0			mb.b0
#define m_b1			mb.b1
#define m_x0			mb.x0
#define m_x1			mb.x1
#define m_i0			mb.i0
#define m_i1			mb.i1
#define m_vsmr			mb.vsmr
#define m_pcs			mb.pcs
#define m_mask			mb.mask
#define m_m				mb.m
#define m_r				mb.r
#define m_rpc			mb.rpc
#define m_c0			mb.c0
#define m_c1			mb.c1
#define m_sft			mb.sft
#define m_vsm			mb.vsm
#define m_gpio0			mb.gpio[ 0 ]
#define m_gpio1			mb.gpio[ 1 ]
#define m_gpio2			mb.gpio[ 2 ]
#define m_gpio3			mb.gpio[ 3 ]
#define m_stall			mb.stall
#define m_icount		mb86233_icount
#define mb_cfg			mb.cfg

/* roundf() in C89: half away from zero, exact for float inputs */
static double mb86233_round( double x ) { double a = fabs( x ), f = floor( a ); if( a - f >= 0.5 ) f += 1.0; return x < 0.0 ? -f : f; }

#define mb_log logerror

static UINT32 set_exp(UINT32 val, UINT32 exp)
{
	return (val & 0x807fffff) | ((exp & 0xff) << 23);
}

static UINT32 set_mant(UINT32 val, UINT32 mant)
{
	return (val & 0x07f800000) | ((mant & 0x00800000) << 8) | (mant & 0x007fffff);
}

static UINT32 get_exp(UINT32 val)
{
	return (val >> 23) & 0xff;
}

static UINT32 get_mant(UINT32 val)
{
	return val & 0x80000000 ? val | 0x7f800000 : val & 0x807fffff;
}

static void pcs_push(void)
{
	unsigned int i;
	for(i=3; i; i--)
		m_pcs[i] = m_pcs[i-1];
	m_pcs[0] = m_pc;
}

static void pcs_pop(void)
{
	unsigned int i;
	m_pc = m_pcs[0];
	for(i=0; i != 3; i++)
		m_pcs[i] = m_pcs[i+1];
}

static void stset_set_sz_int(UINT32 val)
{
	m_alu_stset = val ? (val & 0x80000000 ? F_SGD : 0) : F_ZRD;
}

static void stset_set_sz_fp(UINT32 val)
{
	m_alu_stset = (val & 0x7fffffff) ? (val & 0x80000000 ? F_SGD : 0) : F_ZRD;
}

static void alu_pre(UINT32 alu)
{
	UINT32 r;
	switch(alu) {
	case 0x00: break; /* no alu */

	case 0x01: {
		/* andd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d & m_a;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x02: {
		/* orad */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d | m_a;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x03: {
		/* eord */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d ^ m_a;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x04: {
		/* notd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = ~m_d;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x05: {
		/* fcpd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		r = f2u(u2f(m_d) - u2f(m_a));
		stset_set_sz_fp(r);
		break;
	}

	case 0x06: {
		/* fadd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_d) + u2f(m_a));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x07: {
		/* fsbd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_d) - u2f(m_a));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x08: {
		/* fml */
		m_alu_stmask = 0;
		m_alu_r1 = f2u(u2f(m_a) * u2f(m_b));
		m_alu_stset = 0;
		break;
	}

	case 0x09: {
		/* fmsd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_d) + u2f(m_p));
		m_alu_r2 = f2u(u2f(m_a) * u2f(m_b));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x0a: {
		/* fmrd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_d) - u2f(m_p));
		m_alu_r2 = f2u(u2f(m_a) * u2f(m_b));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x0b: {
		/* fabd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d & 0x7fffffff;
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x0c: {
		/* fsmd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_d) + u2f(m_p));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x0d: {
		/* fspd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_p;
		m_alu_r2 = f2u(u2f(m_a) * u2f(m_b));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x0e: {
		/* cxfd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u((INT32)(m_d));
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x0f: {
		/* cfxd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		switch((m_m >> 1) & 3) {
		/* C89 double math: exact for float inputs, same results as roundf/ceilf/floorf */
		case 0: m_alu_r1 = (INT32)(mb86233_round(u2f(m_d))); break;
		case 1: m_alu_r1 = (INT32)(ceil(u2f(m_d))); break;
		case 2: m_alu_r1 = (INT32)(floor(u2f(m_d))); break;
		case 3: m_alu_r1 = (INT32)(u2f(m_d)); break;
		}
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x10: {
		/* fdvd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_d) / u2f(m_a));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x11: {
		/* fned */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d ? m_d ^ 0x80000000 : 0;
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x13: {
		/* d = b + a */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_b) + u2f(m_a));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x14: {
		/* d = b - a */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = f2u(u2f(m_b) - u2f(m_a));
		stset_set_sz_fp(m_alu_r1);
		break;
	}

	case 0x16: {
		/* lsrd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d >> m_sft;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x17: {
		/* lsld */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d << m_sft;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x18: {
		/* asrd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = (INT32)(m_d) >> m_sft;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x19: {
		/* asld */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = (INT32)(m_d) << m_sft;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x1a: {
		/* addd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d + m_a;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	case 0x1b: {
		/* subd */
		m_alu_stmask = F_ZRD|F_SGD|F_CPD|F_OVD|F_DVZD;
		m_alu_r1 = m_d - m_a;
		stset_set_sz_int(m_alu_r1);
		break;
	}

	default:
		mb_log("unhandled alu pre %02x\n", alu);
		break;
	}
}

static void alu_update_st(void)
{
	m_st = (m_st & ~m_alu_stmask) | m_alu_stset;
}

static void alu_post_1(UINT32 alu)
{
	/* integer alu post ops */
	switch(alu) {
	case 0x01: case 0x02: case 0x03: case 0x04:
	case 0x0e: case 0x0f: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1a: case 0x1b:
		/* d update */
		m_d = m_alu_r1;
		alu_update_st();
		break;

	default:
		break;
	}
}

static void alu_post_2(UINT32 alu)
{
	/* floating point alu post ops */
	/* assume each one takes 2 cycles */
	switch (alu) {
	case 0x05:
		/* flags only */
		alu_update_st();
		m_icount--;
		break;

	case 0x06: case 0x07: case 0x0b: case 0x0c:
	case 0x10: case 0x11: case 0x13: case 0x14:
		/* d update */
		m_d = m_alu_r1;
		alu_update_st();
		m_icount--;
		break;

	case 0x08:
		/* p update */
		m_p = m_alu_r1;
		m_icount--;
		break;

	case 0x09: case 0x0a: case 0x0d:
		/* d, p update */
		m_d = m_alu_r1;
		m_p = m_alu_r2;
		alu_update_st();
		m_icount--;
		break;

	default:
		break;
	}
}

static UINT16 ea_pre_0(UINT32 r)
{
	switch(r & 0x180) {
	case 0x000: return r & 0x7f;
	case 0x080: case 0x100: return (r & 0x7f) + m_b0 + m_x0;
	case 0x180: {
		switch(r & 0x60) {
		case 0x00: return m_b0 + m_x0;
		case 0x20: return m_x0;
		case 0x40: return m_b0 + (m_x0 & m_vsmr);
		case 0x60: return m_x0 & m_vsmr;
		}
	}
	}
	return 0;
}

static void ea_post_0(UINT32 r)
{
	if(!(r & 0x100))
		return;
	if(!(r & 0x080))
		m_x0 += m_i0;
	else
		m_x0 += SEXT(r, 5);
}

static UINT16 ea_pre_1(UINT32 r)
{
	switch(r & 0x180) {
	case 0x000: return r & 0x7f;
	case 0x080: case 0x100: return (r & 0x7f) + m_b1 + m_x1;
	case 0x180: {
		switch(r & 0x60) {
		case 0x00: return m_b1 + m_x1;
		case 0x20: return m_x1;
		case 0x40: return m_b1 + (m_x1 & m_vsmr);
		case 0x60: return m_x1 & m_vsmr;
		}
	}
	}
	return 0;
}

static void ea_post_1(UINT32 r)
{
	if(!(r & 0x100))
		return;
	if(!(r & 0x080))
		m_x1 += m_i1;
	else
		m_x1 += SEXT(r, 5);
}

static UINT32 read_reg(UINT32 r)
{
	r &= 0x3f;
	if(r >= 0x20 && r < 0x30)
		return mb_cfg->rf_r(r & 0x1f);
	switch(r) {
	case 0x00: return m_b0;
	case 0x01: return m_b1;
	case 0x02: return m_x0;
	case 0x03: return m_x1;

	case 0x0c: return m_c0;
	case 0x0d: return m_c1;

	case 0x10: return m_a;
	case 0x11: return get_exp(m_a);
	case 0x12: return get_mant(m_a);
	case 0x13: return m_b;
	case 0x14: return get_exp(m_b);
	case 0x15: return get_mant(m_b);
	case 0x19: return m_d;
		/* c */
	case 0x1a: return get_exp(m_d);
	case 0x1b: return get_mant(m_d);
	case 0x1c: return m_p;
	case 0x1d: return get_exp(m_p);
	case 0x1e: return get_mant(m_p);
	case 0x1f: return m_sft;

	case 0x34: return m_rpc;

	default:
		mb_log("unimplemented read_reg(%02x) (%x)\n", r, m_ppc);
		return 0;
	}
}

static void write_reg(UINT32 r, UINT32 v)
{
	r &= 0x3f;
	if(r >= 0x20 && r < 0x30) {
		mb_cfg->rf_w(r & 0x1f, v);
		return;
	}
	switch(r) {
	case 0x00: m_b0 = v; break;
	case 0x01: m_b1 = v; break;
	case 0x02: m_x0 = v; break;
	case 0x03: m_x1 = v; break;

	case 0x05: m_i0 = v; break;
	case 0x06: m_i1 = v; break;

	case 0x08: m_sp = v; break;

	case 0x0a: m_vsm = v & 7; m_vsmr = (8 << m_vsm) - 1; break;

	case 0x0c:
		m_c0 = v;
		if(m_c0 == 1)
			m_st |= F_ZC0;
		else
			m_st &= ~F_ZC0;
		break;

	case 0x0d:
		m_c1 = v;
		if(m_c1 == 1)
			m_st |= F_ZC1;
		else
			m_st &= ~F_ZC1;
		break;

	case 0x0f: break;

	case 0x10: m_a = v; break;
	case 0x11: m_a = set_exp(m_a, v); break;
	case 0x12: m_a = set_mant(m_a, v); break;
	case 0x13: m_b = v; break;
	case 0x14: m_b = set_exp(m_b, v); break;
	case 0x15: m_b = set_mant(m_b, v); break;
		/* c */
	case 0x19: m_d = v; break;
	case 0x1a: m_d = set_exp(m_d, v); break;
	case 0x1b: m_d = set_mant(m_d, v); break;
	case 0x1c: m_p = v; break;
	case 0x1d: m_p = set_exp(m_p, v); break;
	case 0x1e: m_p = set_mant(m_p, v); break;
	case 0x1f: m_sft = v; break;

	case 0x34: m_rpc = v; break;
	case 0x3c: m_mask = v; break;

	default:
		mb_log("unimplemented write_reg(%02x, %08x) (%x)\n", r, v, m_ppc);
		break;
	}
}

static void write_mem_internal_1(UINT32 r, UINT32 v, int bank)
{
	UINT16 ea = ea_pre_1(r);
	if(bank)
		ea += 0x200;
	mb_cfg->data_w(ea, v);
	ea_post_1(r);
}

static void write_mem_io_1(UINT32 r, UINT32 v)
{
	UINT16 ea = ea_pre_1(r);
	mb_cfg->io_w(ea, v);
	ea_post_1(r);
}

static int mb86233_core_execute(int cycles)
{
	UINT32 opcode, r1, r2, alu, op, ea, ea1, ea2, v, v1, v2, sub2, cond, subtype, data;
	int invert, cond_passed;
	UINT8 rr;

	m_icount = cycles;
	while(m_icount > 0) {
		m_ppc = m_pc;
		CALL_MAME_DEBUG;
		opcode = mb_cfg->prog_r(m_pc++);

		switch((opcode >> 26) & 0x3f) {
		case 0x00: {
			/* lab */
			r1 = opcode & 0x1ff;
			r2 = (opcode >> 9) & 0x1ff;
			alu = (opcode >> 21) & 0x1f;
			op = (opcode >> 18) & 0x7;

			alu_pre(alu);

			switch(op) {
			case 0: case 1: {
				/* lab mem, mem (e) */

				ea1 = ea_pre_0(r1);
				v1 = mb_cfg->data_r(ea1);
				if(m_stall) goto do_stall;

				ea2 = ea_pre_1(r2);
				v2 = mb_cfg->io_r(ea2);
				if(m_stall) goto do_stall;

				ea_post_0(r1);
				ea_post_1(r2);

				m_a = v1;
				m_b = v2;
				break;
			}

			case 3: {
				/* lab mem, mem + 0x200 */

				ea1 = ea_pre_0(r1);
				v1 = mb_cfg->data_r(ea1);
				if(m_stall) goto do_stall;

				ea2 = ea_pre_1(r2) + 0x200;
				v2 = mb_cfg->data_r(ea2);
				if(m_stall) goto do_stall;

				ea_post_0(r1);
				ea_post_1(r2);

				m_a = v1;
				m_b = v2;
				break;
			}

			case 4: {
				/* lab mem + 0x200, mem */

				ea1 = ea_pre_0(r1) + 0x200;
				v1 = mb_cfg->data_r(ea1);
				if(m_stall) goto do_stall;

				ea2 = ea_pre_1(r2);
				v2 = mb_cfg->data_r(ea2);
				if(m_stall) goto do_stall;

				ea_post_0(r1);
				ea_post_1(r2);

				m_a = v1;
				m_b = v2;
				break;
			}

			default:
				mb_log("unhandled lab subop %x\n", op);
				mb_log("%x\n", m_ppc);
				break;

			}

			alu_post_1(alu);
			alu_post_2(alu);
			break;
		}


		case 0x07: {
			/* ld / mov */
			r1 = opcode & 0x1ff;
			r2 = (opcode >> 9) & 0x1ff;
			alu = (opcode >> 21) & 0x1f;
			op = (opcode >> 18) & 0x7;

			alu_pre(alu);

			switch(op) {
			case 0: {
				/* mov mem, mem (e) */
				ea = ea_pre_0(r1);
				v = mb_cfg->data_r(ea);
				if(m_stall) goto do_stall;
				ea_post_0(r1);
				alu_post_1(alu);
				write_mem_io_1(r2, v);
				break;
			}

			case 1: {
				/* mov mem, mem (e) */
				ea = ea_pre_0(r1);
				v = mb_cfg->data_r(ea);
				if(m_stall) goto do_stall;
				ea_post_0(r1);
				alu_post_1(alu);
				write_mem_io_1(r2, v);
				break;
			}

			case 2: {
				/* mov mem (e), mem */
				ea = ea_pre_0(r1);
				v = mb_cfg->io_r(ea);
				if(m_stall) goto do_stall;
				ea_post_0(r1);
				alu_post_1(alu);
				write_mem_internal_1(r2, v, 0);
				break;
			}

			case 3: {
				/* mov mem, mem + 0x200 */
				ea = ea_pre_0(r1);
				v = mb_cfg->data_r(ea);
				if(m_stall) goto do_stall;
				ea_post_0(r1);
				alu_post_1(alu);
				write_mem_internal_1(r2, v, 1);
				break;
			}

			case 4: {
				/* mov mem + 0x200, mem */
				ea = ea_pre_0(r1) + 0x200;
				v = mb_cfg->data_r(ea);
				if(m_stall) goto do_stall;
				ea_post_0(r1);
				alu_post_1(alu);
				write_mem_internal_1(r2, v, 0);
				break;
			}

			case 5: {
				/* mov mem (o), mem */
				ea = ea_pre_0(r1);
				v = mb_cfg->prog_r(ea);
				if(m_stall) goto do_stall;
				ea_post_0(r1);
				alu_post_1(alu);
				write_mem_internal_1(r2, v, 0);
				break;
			}

			case 7: {
				switch(r2 >> 6) {
				case 0: {
					/* mov reg, mem */
					v = read_reg(r2);
					if(m_stall) goto do_stall;
					alu_post_1(alu);
					write_mem_internal_1(r1, v, 0);
					break;
				}

				case 1: {
					/* mov reg, mem (e) */
					v = read_reg(r2);
					if(m_stall) goto do_stall;
					alu_post_1(alu);
					write_mem_io_1(r1, v);
					break;
				}

				case 2: {
					/* mov mem + 0x200, reg */
					ea = ea_pre_1(r1) + 0x200;
					v = mb_cfg->data_r(ea);
					if(m_stall) goto do_stall;
					ea_post_1(r1);
					alu_post_1(alu);
					write_reg(r2, v);
					break;
				}

				case 3: {
					/* mov mem, reg */
					ea = ea_pre_1(r1);
					v = mb_cfg->data_r(ea);
					if(m_stall) goto do_stall;
					ea_post_1(r1);
					alu_post_1(alu);
					write_reg(r2, v);
					break;
				}

				case 4: {
					/* mov mem (e), reg */
					ea = ea_pre_1(r1);
					v = mb_cfg->io_r(ea);
					if(m_stall) goto do_stall;
					ea_post_1(r1);
					alu_post_1(alu);
					write_reg(r2, v);
					break;
				}

				case 5: {
					/* mov mem (o), reg */
					ea = ea_pre_0(r1);
					v = mb_cfg->prog_r(ea);
					if(m_stall) goto do_stall;
					ea_post_0(r1);
					alu_post_1(alu);
					write_reg(r2, v);
					break;
				}

				case 6: {
					/* mov reg, reg */
					v = read_reg(r1);
					if(m_stall) goto do_stall;
					alu_post_1(alu);
					write_reg(r2, v);
					break;
				}

				default:
					alu_post_1(alu);
					mb_log("unhandled ld/mov subop 7/%x (%x)\n", r2 >> 6, m_ppc);
					break;
				}
				break;
			}

			default:
				alu_post_1(alu);
				mb_log("unhandled ld/mov subop %x (%x)\n", op, m_ppc);
				break;
			}

			/* For floating point ops, registers are updated after transfer */
			alu_post_2(alu);
			break;
		}

		case 0x0d: {
			/* stm/clm */
			sub2 = (opcode >> 17) & 7;

			/* Theorically has restricted alu too */

			switch(sub2) {
			case 5:
				/* stmh */
				/* bit 0 = floating point */
				/* bit 1-2 = rounding mode */
				m_m = opcode;
				break;

			default:
				mb_log("unimplemented opcode 0d/%x (%x)\n", sub2, m_ppc);
				break;
			}
			break;
		}

		case 0x0e: {
			/* lipl / lia / lib / lid */
			switch((opcode >> 24) & 0x3) {
			case 0:
				m_p = (m_p & 0xff000000) | (opcode & 0xffffff);
				break;
			case 1:
				m_a = SEXT(opcode, 24);
				break;
			case 2:
				m_b = SEXT(opcode, 24);
				break;
			case 3:
				m_d = SEXT(opcode, 24);
				break;
			}
			break;
		}

		case 0x0f: {
			/* rep/clr0/clr1/set */
			alu = (opcode >> 20) & 0x1f;
			sub2 = (opcode >> 17) & 7;

			alu_pre(alu);

			switch(sub2) {
			case 0:
				/* clr0 */
				if(opcode & 0x0004) m_a = 0;
				if(opcode & 0x0008) m_b = 0;
				if(opcode & 0x0010) m_d = 0;
				break;

			case 1:
				/* clr1 - flags mapping unknown */
				break;

			case 2: {
				/* rep */
				rr = opcode & 0x8000 ? read_reg(opcode) : opcode;
				if(m_stall) goto do_stall;
				m_r = rr;
				goto rep_start;
			}

			case 3:
				/* set - flags mapping unknown */
				/* 0800 = enable interrupt flag */
				break;

			default:
				mb_log("unimplemented opcode 0f/%x (%x)\n", sub2, m_ppc);
				break;
			}

			alu_post_1(alu);
			break;
		}

		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: {
			/* ldi */
			write_reg(opcode >> 24, SEXT(opcode, 24));
			break;
		}

		case 0x2f: case 0x3f: {
			/* Conditional branch of every kind */
			cond = ( opcode >> 20 ) & 0x1f;
			subtype = ( opcode >> 17 ) & 7;
			data = opcode & 0xffff;
			invert = opcode & 0x40000000;

			cond_passed = 0;

			switch(cond) {
			case 0x00: /* zrd - d zero */
				cond_passed = m_st & F_ZRD;
				break;

			case 0x01: /* ged - d >= 0 */
				cond_passed = !(m_st & F_SGD);
				break;

			case 0x02: /* led - d <= 0 */
				cond_passed = m_st & (F_ZRD | F_SGD);
				break;

			case 0x0a: /* gpio0 */
				cond_passed = m_gpio0;
				break;

			case 0x0b: /* gpio1 */
				cond_passed = m_gpio1;
				break;

			case 0x0c: /* gpio2 */
				cond_passed = m_gpio2;
				break;

			case 0x10: /* zc0 - c0 == 1 */
				cond_passed = !(m_st & F_ZC0);
				break;

			case 0x11: /* zc1 - c1 == 1 */
				cond_passed = !(m_st & F_ZC1);
				break;

			case 0x12: /* gpio3 */
				cond_passed = m_gpio3;
				break;

			case 0x16: /* alw - always */
				cond_passed = 1;
				break;

			default:
				mb_log("unimplemented condition %x (%x)\n", cond, m_ppc);
				break;
			}
			if(invert)
				cond_passed = !cond_passed;

			if(cond_passed) {
				switch(subtype) {
				case 0: /* brif #adr */
					m_pc = data;
					break;

				case 1: /* brul */
					if(opcode & 0x4000) {
						/* brul reg */
						v = read_reg(opcode);
						if(m_stall) goto do_stall;
						m_pc = v;
					} else {
						/* brul adr */
						ea = ea_pre_0(opcode);
						v = mb_cfg->data_r(ea);
						if(m_stall) goto do_stall;
						ea_post_0(opcode);
						m_pc = v;
					}
					break;

				case 2: /* bsif #adr */
					pcs_push();
					m_pc = data;
					break;

				case 3: /* bsul */
					if(opcode & 0x4000) {
						/* bsul reg */
						v = read_reg(opcode);
						if(m_stall) goto do_stall;
						pcs_push();
						m_pc = v;
					} else {
						/* bsul adr */
						ea = ea_pre_0(opcode);
						v = mb_cfg->data_r(ea);
						if(m_stall) goto do_stall;
						ea_post_0(opcode);
						pcs_push();
						m_pc = v;
					}
					break;

				case 5: /* rtif #adr */
					pcs_pop();
					break;

				case 6: { /* ldif adr, rn */
					ea = ea_pre_0(opcode);
					v = mb_cfg->data_r(ea);
					if(m_stall) goto do_stall;
					ea_post_0(opcode);
					write_reg(opcode >> 9, v);
					break;
				}

				default:
					mb_log("unimplemented branch subtype %x (%x)\n", subtype, m_ppc);
					break;
				}
			}

			if(subtype < 2)
				switch(cond) {
				case 0x10:
					if(m_c0 != 1) {
						m_c0 --;
						if(m_c0 == 1)
							m_st |= F_ZC0;
					}
					break;

				case 0x11:
					if(m_c1 != 1) {
						m_c1 --;
						if(m_c1 == 1)
							m_st |= F_ZC1;
					}
				break;
				}

			break;
		}

		default:
			mb_log("unimplemented opcode type %02x (%x)\n", (opcode >> 26) & 0x3f, m_ppc);
			break;
		}

		if(m_r != 1) {
			m_pc = m_ppc;
			m_r --;
		}

	rep_start:
		if(0) {
		do_stall:
			m_pc = m_ppc;
			m_stall = 0;
			/* waits for its FIFO: the driver halts it, give up the timeslice */
			m_icount = 0;
		}
		m_icount--;
	}
	return cycles - m_icount;
}

/***************************************************************************
    CPU interface
***************************************************************************/

void mb86233_stall( void )
{
	m_stall = 1;
}

void mb86233_gpio_w( int pin, int state )
{
	mb.gpio[ pin & 3 ] = state;
}

static void mb86233_core_init( void )
{
	int cpu = cpu_getactivecpu();

	memset( &mb, 0, sizeof( mb ) );
	state_save_register_memory("mb86233", cpu, "st", &mb.st, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "a", &mb.a, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "b", &mb.b, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "d", &mb.d, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "p", &mb.p, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "alu_stmask", &mb.alu_stmask, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "alu_stset", &mb.alu_stset, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "alu_r1", &mb.alu_r1, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "alu_r2", &mb.alu_r2, sizeof(UINT32), 1);
	state_save_register_memory("mb86233", cpu, "ppc", &mb.ppc, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "pc", &mb.pc, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "sp", &mb.sp, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "b0", &mb.b0, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "b1", &mb.b1, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "x0", &mb.x0, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "x1", &mb.x1, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "i0", &mb.i0, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "i1", &mb.i1, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "vsmr", &mb.vsmr, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "pcs", mb.pcs, sizeof(UINT16), 4);
	state_save_register_memory("mb86233", cpu, "mask", &mb.mask, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "m", &mb.m, sizeof(UINT16), 1);
	state_save_register_memory("mb86233", cpu, "r", &mb.r, sizeof(UINT8), 1);
	state_save_register_memory("mb86233", cpu, "rpc", &mb.rpc, sizeof(UINT8), 1);
	state_save_register_memory("mb86233", cpu, "c0", &mb.c0, sizeof(UINT8), 1);
	state_save_register_memory("mb86233", cpu, "c1", &mb.c1, sizeof(UINT8), 1);
	state_save_register_memory("mb86233", cpu, "sft", &mb.sft, sizeof(UINT8), 1);
	state_save_register_memory("mb86233", cpu, "vsm", &mb.vsm, sizeof(UINT8), 1);
	state_save_register_memory("mb86233", cpu, "gpio0", &mb.gpio[ 0 ], sizeof(int), 1);
	state_save_register_memory("mb86233", cpu, "gpio1", &mb.gpio[ 1 ], sizeof(int), 1);
	state_save_register_memory("mb86233", cpu, "gpio2", &mb.gpio[ 2 ], sizeof(int), 1);
	state_save_register_memory("mb86233", cpu, "gpio3", &mb.gpio[ 3 ], sizeof(int), 1);
	/* a stalled FIFO access is retried when the CPU runs again */
	state_save_register_memory("mb86233", cpu, "stall", &mb.stall, sizeof(int), 1);
}

static void mb86233_core_reset( void *param )
{
	struct mb86233_config *cfg = param ? (struct mb86233_config *)param : mb.cfg;
	int gpio[ 4 ];
	memcpy( gpio, mb.gpio, sizeof( gpio ) );
	memset( &mb, 0, sizeof( mb ) );
	memcpy( mb.gpio, gpio, sizeof( gpio ) );
	mb.cfg = cfg;

	m_st = F_ZRC|F_ZRD|F_ZX0|F_ZX1|F_ZX2|F_ZC0|F_ZC1;
	m_r = 1;
	m_rpc = 1;
	m_c0 = 1;
	m_c1 = 1;
	m_vsmr = 7;
	m_m = 1;
}

static unsigned mb86233_core_get_context( void *dst )
{
	if( dst )
		*(mb86233_regs *)dst = mb;
	return sizeof( mb86233_regs );
}

/* single instance: the static state is always the live one (the host
   changes it while another CPU runs, e.g. the FIFO pins). set_context never
   copies back, so a stale context buffer cannot clobber those changes */
static void mb86233_core_set_context( void *src )
{
	(void)src;
}

static unsigned mb86233_core_get_reg( int regnum )
{
	switch( regnum )
	{
	case REG_PC:			return m_pc;
	case REG_PREVIOUSPC:	return m_ppc;
	case REG_SP:			return m_sp;
	}
	return 0;
}

static void mb86233_core_set_reg( int regnum, unsigned val )
{
	switch( regnum )
	{
	case REG_PC:	m_pc = val; break;
	case REG_SP:	m_sp = val; break;
	}
}


#ifdef MAME_DEBUG
static offs_t mb86233_core_dasm_info( char *buffer, offs_t pc )
{
	sprintf( buffer, "$%08x", mb.cfg ? mb.cfg->prog_r( pc ) : 0 );
	return 1;
}
#endif

/***************************************************************************
    get_info interface (the core above keeps the state in a static context,
    only one TGP per machine is supported)
***************************************************************************/

static struct mb86233_config *mb86233_config_ptr;

static void mb86233_get_context( void *dst )
{
	mb86233_core_get_context( dst );
}

static void mb86233_set_context( void *src )
{
	mb86233_core_set_context( src );
}

static void mb86233_init( int index, int clock, const void *config, int (*irqcallback)(int) )
{
	mb86233_core_init();
	mb86233_config_ptr = (struct mb86233_config *)config;
	mb.cfg = mb86233_config_ptr;
}

static void mb86233_reset( void )
{
	mb86233_core_reset( mb86233_config_ptr );
}

static void mb86233_set_info( UINT32 state, union cpuinfo *info )
{
	switch( state )
	{
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + MB86233_PC:		mb86233_core_set_reg( REG_PC, info->i );	break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + MB86233_SP:		mb86233_core_set_reg( REG_SP, info->i );	break;
	}
}

void mb86233_get_info( UINT32 state, union cpuinfo *info )
{
	switch( state )
	{
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof( mb86233_regs );		break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 0;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 2;							break;

		/* the address spaces are driver callbacks, the core uses no memory map */
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM:	info->i = -2;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO:		info->i = 0;					break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = mb86233_core_get_reg( REG_PREVIOUSPC );	break;
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + MB86233_PC:			info->i = mb86233_core_get_reg( REG_PC );	break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + MB86233_SP:			info->i = mb86233_core_get_reg( REG_SP );	break;

		case CPUINFO_PTR_SET_INFO:						info->setinfo = mb86233_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = mb86233_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = mb86233_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = mb86233_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = mb86233_reset;			break;
		case CPUINFO_PTR_EXIT:							info->exit = 0;							break;
		case CPUINFO_PTR_EXECUTE:						info->execute = mb86233_core_execute;	break;
		case CPUINFO_PTR_BURN:							info->burn = 0;							break;
#ifdef MAME_DEBUG
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = mb86233_core_dasm_info;	break;
#endif
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &mb86233_icount;			break;

		case CPUINFO_STR_NAME:							strcpy( info->s = cpuintrf_temp_str(), "MB86233" );				break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy( info->s = cpuintrf_temp_str(), "Fujitsu TGP" );			break;
		case CPUINFO_STR_CORE_VERSION:					strcpy( info->s = cpuintrf_temp_str(), "1.0" );					break;
		case CPUINFO_STR_CORE_FILE:						strcpy( info->s = cpuintrf_temp_str(), __FILE__ );				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy( info->s = cpuintrf_temp_str(), "Olivier Galibert" );		break;
		case CPUINFO_STR_FLAGS:							sprintf( info->s = cpuintrf_temp_str(), "%08X", mb.st );		break;
		case CPUINFO_STR_REGISTER + MB86233_PC:			sprintf( info->s = cpuintrf_temp_str(), "PC:%04X", mb.pc );		break;
	}
}
