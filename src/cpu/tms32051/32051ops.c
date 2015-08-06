INLINE void PUSH_PC(UINT16 pc)
{
	if (tms.pcstack_ptr >= 8)
	{
		fatalerror("32051: PC stack overflow!\n");
	}

	tms.pcstack[tms.pcstack_ptr] = pc;
	tms.pcstack_ptr++;
}

INLINE UINT16 POP_PC(void)
{
	UINT16 pc;
	tms.pcstack_ptr--;
	if (tms.pcstack_ptr < 0)
	{
		fatalerror("32051: PC stack underflow!\n");
	}

	pc = tms.pcstack[tms.pcstack_ptr];
	return pc;
}

INLINE INT32 SUB(INT32 a, INT32 b)
{
	INT64 res = a - b;
	if (tms.st0.ovm)	// overflow saturation mode
	{
		if ((res & 0x80000000) && ((UINT32)(res >> 32) & 0xffffffff) != 0xffffffff)
		{
			res = 0x80000000;
		}
		else if (((res & 0x80000000) == 0) && ((UINT32)(res >> 32) & 0xffffffff) != 0)
		{
			res = 0x7fffffff;
		}
	}
	else				// normal mode, result is not modified
	{
		// set OV flag if overflow occured, this is a sticky flag
		if (((a) ^ (b)) & ((a) ^ ((INT32)res)) & 0x80000000)
		{
			tms.st0.ov = 1;
		}
	}

	// set carry
	// TODO: shift with 16-bits works differently!
	if (res & U64(0x100000000))
	{
		tms.st1.c = 0;
	}
	else
	{
		tms.st1.c = 1;
	}

	return (INT32)(res);
}

INLINE INT32 ADD(INT32 a, INT32 b)
{
	INT64 res = a + b;
	if (tms.st0.ovm)	// overflow saturation mode
	{
		if ((res & 0x80000000) && ((UINT32)(res >> 32) & 0xffffffff) != 0xffffffff)
		{
			res = 0x80000000;
		}
		else if (((res & 0x80000000) == 0) && ((UINT32)(res >> 32) & 0xffffffff) != 0)
		{
			res = 0x7fffffff;
		}
	}
	else				// normal mode, result is not modified
	{
		// set OV flag if overflow occured, this is a sticky flag
		if (((res) ^ (b)) & ((res) ^ (a)) & 0x80000000)
		{
			tms.st0.ov = 1;
		}
	}

	// set carry
	// TODO: shift with 16-bits works differently!
	if (res & U64(0x100000000))
	{
		tms.st1.c = 0;
	}
	else
	{
		tms.st1.c = 1;
	}

	return (INT32)(res);
}


static UINT16 GET_ADDRESS(void)
{
	if (tms.op & 0x80)		// Indirect Addressing
	{
		UINT16 ea;
		int arp = tms.st0.arp;
		int nar = tms.op & 0x7;

		switch ((tms.op >> 3) & 0xf)
		{
			case 0x0:	// *            (no operation)
			{
				ea = tms.ar[arp];
				break;
			}
			case 0x1:	// *, ARn       (NAR -> ARP)
			{
				ea = tms.ar[arp];
				tms.st1.arb = tms.st0.arp;
				tms.st0.arp = nar;
				break;
			}
			case 0x2:	// *-           ((CurrentAR)-1 -> CurrentAR)
			{
				ea = tms.ar[arp];
				tms.ar[arp]--;
				break;
			}
			case 0x3:	// *-, ARn      ((CurrentAR)-1 -> CurrentAR, NAR -> ARP)
			{
				ea = tms.ar[arp];
				tms.ar[arp]--;
				tms.st1.arb = tms.st0.arp;
				tms.st0.arp = nar;
				break;
			}
			case 0x4:	// *+           ((CurrentAR)+1 -> CurrentAR)
			{
				ea = tms.ar[arp];
				tms.ar[arp]++;
				break;
			}
			case 0x5:	// *+, ARn      ((CurrentAR)+1 -> CurrentAR, NAR -> ARP)
			{
				ea = tms.ar[arp];
				tms.ar[arp]++;
				tms.st1.arb = tms.st0.arp;
				tms.st0.arp = nar;
				break;
			}

			default:	fatalerror("32051: GET_ADDRESS: unimplemented indirect addressing mode %d\n", (tms.op >> 3) & 0xf);
		}

		return ea;
	}
	else					// Direct Addressing
	{
		return tms.st0.dp | (tms.op & 0x7f);
	}
}

static int GET_ZLCV_CONDITION(int zlcv, int zlcv_mask)
{
	int condition = 0;

	if (zlcv_mask & 0x8)		// Z-bit
	{
		if ((zlcv & 0x8) && (INT32)(tms.acc) == 0)				// EQ
		{
			condition = 1;
		}
		else if ((zlcv & 0x8) == 0 && (INT32)(tms.acc) != 0)	// NEQ
		{
			condition = 1;
		}
	}
	if (zlcv_mask & 0x4)		// L-bit
	{
		if ((zlcv & 0x4) && (INT32)(tms.acc) < 0)				// LT
		{
			condition = 1;
		}
		else if ((zlcv & 0x4) == 0 && (INT32)(tms.acc) > 0)		// GT
		{
			condition = 1;
		}
	}
	if (zlcv_mask & 0x2)		// C-bit
	{
		if ((zlcv & 0x2) && tms.st1.c)							// C
		{
			condition = 1;
		}
		else if ((zlcv & 0x2) == 0 && tms.st1.c == 0)			// NC
		{
			condition = 1;
		}
	}
	if (zlcv_mask & 0x1)		// OV-bit
	{
		if ((zlcv & 0x1) && tms.st0.ov)							// OV
		{
			condition = 1;
		}
		else if ((zlcv & 0x1) == 0 && tms.st0.ov == 0)			// NOV
		{
			condition = 1;
		}
	}
	return condition;
}

static int GET_TP_CONDITION(int tp)
{
	switch (tp)
	{
		case 0:		// BIO pin low
		{
			// TODO
			return 1;
		}
		case 1:		// TC = 1
		{
			return (tms.st1.tc == 1) ? 1 : 0;
		}
		case 2:		// TC = 0
		{
			return (tms.st1.tc == 0) ? 1 : 0;
		}
		case 3:		// always true
		{
			return 1;
		}
	}
	return 0;
}



static void op_invalid(void)
{
	fatalerror("32051: invalid op at %08X", tms.pc-1);
}

static void op_group_be(void);
static void op_group_bf(void);

/*****************************************************************************/

static void op_abs(void)
{
	fatalerror("32051: unimplemented op abs at %08X", tms.pc-1);
}

static void op_adcb(void)
{
	fatalerror("32051: unimplemented op adcb at %08X", tms.pc-1);
}

static void op_add_mem(void)
{
	INT32 d;
	UINT16 ea = GET_ADDRESS();
	UINT16 data = DM_READ16(ea);
	int shift = (tms.op >> 8) & 0xf;

	if (tms.st1.sxm)
	{
		d = (INT32)(INT16)(data) << shift;
	}
	else
	{
		d = (UINT32)(UINT16)(data) << shift;
	}

	tms.acc = ADD(tms.acc, d);

	CYCLES(1);
}

static void op_add_simm(void)
{
	UINT16 imm = tms.op & 0xff;

	tms.acc = ADD(tms.acc, imm);

	CYCLES(1);
}

static void op_add_limm(void)
{
	INT32 d;
	UINT16 imm = ROPCODE();
	int shift = tms.op & 0xf;

	if (tms.st1.sxm)
	{
		d = (INT32)(INT16)(imm) << shift;
	}
	else
	{
		d = (UINT32)(UINT16)(imm) << shift;
	}

	tms.acc = ADD(tms.acc, d);

	CYCLES(2);
}

static void op_add_s16_mem(void)
{
	fatalerror("32051: unimplemented op add s16 mem at %08X", tms.pc-1);
}

static void op_addb(void)
{
	fatalerror("32051: unimplemented op addb at %08X", tms.pc-1);
}

static void op_addc(void)
{
	fatalerror("32051: unimplemented op addc at %08X", tms.pc-1);
}

static void op_adds(void)
{
	fatalerror("32051: unimplemented op adds at %08X", tms.pc-1);
}

static void op_addt(void)
{
	fatalerror("32051: unimplemented op addt at %08X", tms.pc-1);
}

static void op_and_mem(void)
{
	fatalerror("32051: unimplemented op and mem at %08X", tms.pc-1);
}

static void op_and_limm(void)
{
	UINT32 imm = ROPCODE();
	int shift = tms.op & 0xf;

	tms.acc &= imm << shift;

	CYCLES(2);
}

static void op_and_s16_limm(void)
{
	fatalerror("32051: unimplemented op and s16 limm at %08X", tms.pc-1);
}

static void op_andb(void)
{
	fatalerror("32051: unimplemented op andb at %08X", tms.pc-1);
}

static void op_bsar(void)
{
	int shift = (tms.op & 0xf) + 1;

	if (tms.st1.sxm)
	{
		tms.acc = (INT32)(tms.acc) >> shift;
	}
	else
	{
		tms.acc = (UINT32)(tms.acc) >> shift;
	}

	CYCLES(1);
}

static void op_cmpl(void)
{
	tms.acc = ~tms.acc;

	CYCLES(1);
}

static void op_crgt(void)
{
	fatalerror("32051: unimplemented op crgt at %08X", tms.pc-1);
}

static void op_crlt(void)
{
	fatalerror("32051: unimplemented op crlt at %08X", tms.pc-1);
}

static void op_exar(void)
{
	INT32 tmp = tms.acc;
	tms.acc = tms.accb;
	tms.accb = tmp;

	CYCLES(1);
}

static void op_lacb(void)
{
	tms.acc = tms.accb;

	CYCLES(1);
}

static void op_lacc_mem(void)
{
	int shift = (tms.op >> 8) & 0xf;
	UINT16 ea = GET_ADDRESS();
	UINT16 data = DM_READ16(ea);

	if (tms.st1.sxm)
	{
		tms.acc = (INT32)(INT16)(data) << shift;
	}
	else
	{
		tms.acc = (UINT32)(UINT16)(data) << shift;
	}

	CYCLES(1);
}

static void op_lacc_limm(void)
{
	UINT16 imm = ROPCODE();
	int shift = tms.op & 0xf;

	if (tms.st1.sxm)
	{
		tms.acc = (INT32)(INT16)(imm) << shift;
	}
	else
	{
		tms.acc = (UINT32)(UINT16)(imm) << shift;
	}

	CYCLES(1);
}

static void op_lacc_s16_mem(void)
{
	fatalerror("32051: unimplemented op lacc s16 mem at %08X", tms.pc-1);
}

static void op_lacl_simm(void)
{
	tms.acc = tms.op & 0xff;

	CYCLES(1);
}

static void op_lacl_mem(void)
{
	UINT16 ea = GET_ADDRESS();
	tms.acc = DM_READ16(ea) & 0xffff;

	CYCLES(1);
}

static void op_lact(void)
{
	fatalerror("32051: unimplemented op lact at %08X", tms.pc-1);
}

static void op_lamm(void)
{
	UINT16 ea = GET_ADDRESS();
	ea &= 0x7f;

	tms.acc = DM_READ16(ea) & 0xffff;
	CYCLES(1);
}

static void op_neg(void)
{
	fatalerror("32051: unimplemented op neg at %08X", tms.pc-1);
}

static void op_norm(void)
{
	fatalerror("32051: unimplemented op norm at %08X", tms.pc-1);
}

static void op_or_mem(void)
{
	fatalerror("32051: unimplemented op or mem at %08X", tms.pc-1);
}

static void op_or_limm(void)
{
	fatalerror("32051: unimplemented op or limm at %08X", tms.pc-1);
}

static void op_or_s16_limm(void)
{
	fatalerror("32051: unimplemented op or s16 limm at %08X", tms.pc-1);
}

static void op_orb(void)
{
	tms.acc |= tms.accb;

	CYCLES(1);
}

static void op_rol(void)
{
	fatalerror("32051: unimplemented op rol at %08X", tms.pc-1);
}

static void op_rolb(void)
{
	fatalerror("32051: unimplemented op rolb at %08X", tms.pc-1);
}

static void op_ror(void)
{
	fatalerror("32051: unimplemented op ror at %08X", tms.pc-1);
}

static void op_rorb(void)
{
	fatalerror("32051: unimplemented op rorb at %08X", tms.pc-1);
}

static void op_sacb(void)
{
	fatalerror("32051: unimplemented op sacb at %08X", tms.pc-1);
}

static void op_sach(void)
{
	UINT16 ea = GET_ADDRESS();
	int shift = (tms.op >> 8) & 0x7;

	DM_WRITE16(ea, (UINT16)((tms.acc << shift) >> 16));
	CYCLES(1);
}

static void op_sacl(void)
{
	UINT16 ea = GET_ADDRESS();
	int shift = (tms.op >> 8) & 0x7;

	DM_WRITE16(ea, (UINT16)(tms.acc << shift));
	CYCLES(1);
}

static void op_samm(void)
{
	UINT16 ea = GET_ADDRESS();
	ea &= 0x7f;

	DM_WRITE16(ea, (UINT16)(tms.acc));
	CYCLES(1);
}

static void op_sath(void)
{
	fatalerror("32051: unimplemented op sath at %08X", tms.pc-1);
}

static void op_satl(void)
{
	fatalerror("32051: unimplemented op satl at %08X", tms.pc-1);
}

static void op_sbb(void)
{
	fatalerror("32051: unimplemented op sbb at %08X", tms.pc-1);
}

static void op_sbbb(void)
{
	fatalerror("32051: unimplemented op sbbb at %08X", tms.pc-1);
}

static void op_sfl(void)
{
	fatalerror("32051: unimplemented op sfl at %08X", tms.pc-1);
}

static void op_sflb(void)
{
	fatalerror("32051: unimplemented op sflb at %08X", tms.pc-1);
}

static void op_sfr(void)
{
	fatalerror("32051: unimplemented op sfr at %08X", tms.pc-1);
}

static void op_sfrb(void)
{
	fatalerror("32051: unimplemented op sfrb at %08X", tms.pc-1);
}

static void op_sub_mem(void)
{
	fatalerror("32051: unimplemented op sub mem at %08X", tms.pc-1);
}

static void op_sub_s16_mem(void)
{
	fatalerror("32051: unimplemented op sub s16 mem at %08X", tms.pc-1);
}

static void op_sub_simm(void)
{
	UINT16 imm = tms.op & 0xff;

	tms.acc = SUB(tms.acc, imm);

	CYCLES(1);
}

static void op_sub_limm(void)
{
	INT32 d;
	UINT16 imm = ROPCODE();
	int shift = tms.op & 0xf;

	if (tms.st1.sxm)
	{
		d = (INT32)(INT16)(imm) << shift;
	}
	else
	{
		d = (UINT32)(UINT16)(imm) << shift;
	}

	tms.acc = SUB(tms.acc, d);

	CYCLES(2);
}

static void op_subb(void)
{
	fatalerror("32051: unimplemented op subb at %08X", tms.pc-1);
}

static void op_subc(void)
{
	fatalerror("32051: unimplemented op subc at %08X", tms.pc-1);
}

static void op_subs(void)
{
	fatalerror("32051: unimplemented op subs at %08X", tms.pc-1);
}

static void op_subt(void)
{
	fatalerror("32051: unimplemented op subt at %08X", tms.pc-1);
}

static void op_xor_mem(void)
{
	fatalerror("32051: unimplemented op xor mem at %08X", tms.pc-1);
}

static void op_xor_limm(void)
{
	fatalerror("32051: unimplemented op xor limm at %08X", tms.pc-1);
}

static void op_xor_s16_limm(void)
{
	fatalerror("32051: unimplemented op xor s16 limm at %08X", tms.pc-1);
}

static void op_xorb(void)
{
	fatalerror("32051: unimplemented op xorb at %08X", tms.pc-1);
}

static void op_zalr(void)
{
	fatalerror("32051: unimplemented op zalr at %08X", tms.pc-1);
}

static void op_zap(void)
{
	tms.acc = 0;
	tms.preg = 0;

	CYCLES(1);
}

/*****************************************************************************/

static void op_adrk(void)
{
	UINT16 imm = tms.op & 0xff;
	tms.ar[tms.st0.arp] += imm;

	CYCLES(1);
}

static void op_cmpr(void)
{
	fatalerror("32051: unimplemented op cmpr at %08X", tms.pc-1);
}

static void op_lar_mem(void)
{
	int arx = (tms.op >> 8) & 0x7;
	UINT16 ea = GET_ADDRESS();
	UINT16 data = DM_READ16(ea);

	tms.ar[arx] = data;

	CYCLES(2);
}

static void op_lar_simm(void)
{
	int arx = (tms.op >> 8) & 0x7;
	tms.ar[arx] = tms.op & 0xff;

	CYCLES(2);
}

static void op_lar_limm(void)
{
	int arx = tms.op & 0x7;
	UINT16 imm = ROPCODE();
	tms.ar[arx] = imm;

	CYCLES(2);
}

static void op_ldp_mem(void)
{
	fatalerror("32051: unimplemented op ldp mem at %08X", tms.pc-1);
}

static void op_ldp_imm(void)
{
	tms.st0.dp = (tms.op & 0x1ff) << 7;
	CYCLES(2);
}

static void op_mar(void)
{
	// direct addressing is NOP
	if (tms.op & 0x80)
	{
		GET_ADDRESS();
	}
	CYCLES(1);
}

static void op_sar(void)
{
	int arx = (tms.op >> 8) & 0x7;
	UINT16 ea = GET_ADDRESS();
	DM_WRITE16(ea, tms.ar[arx]);

	CYCLES(1);
}

static void op_sbrk(void)
{
	fatalerror("32051: unimplemented op sbrk at %08X", tms.pc-1);
}

/*****************************************************************************/

static void op_b(void)
{
	UINT16 pma = ROPCODE();
	GET_ADDRESS();		// update AR/ARP

	CHANGE_PC(pma);
	CYCLES(4);
}

static void op_bacc(void)
{
	CHANGE_PC((UINT16)(tms.acc));

	CYCLES(4);
}

static void op_baccd(void)
{
	fatalerror("32051: unimplemented op baccd at %08X", tms.pc-1);
}

static void op_banz(void)
{
	UINT16 pma = ROPCODE();

	if (tms.ar[tms.st0.arp] != 0)
	{
		CHANGE_PC(pma);
		CYCLES(4);
	}
	else
	{
		CYCLES(2);
	}

	GET_ADDRESS();		// modify AR/ARP
}

static void op_banzd(void)
{
	fatalerror("32051: unimplemented op banzd at %08X", tms.pc-1);
}

static void op_bcnd(void)
{
	UINT16 pma = ROPCODE();

	int zlcv_condition = GET_ZLCV_CONDITION((tms.op >> 4) & 0xf, tms.op & 0xf);
	int tp_condition = GET_TP_CONDITION((tms.op >> 8) & 0x3);

	if (zlcv_condition && tp_condition)
	{
		CHANGE_PC(pma);
		CYCLES(4);
	}
	else
	{
		CYCLES(2);
	}
}

static void op_bcndd(void)
{
	fatalerror("32051: unimplemented op bcndd at %08X", tms.pc-1);
}

static void op_bd(void)
{
	fatalerror("32051: unimplemented op bd at %08X", tms.pc-1);
}

static void op_cala(void)
{
	PUSH_PC(tms.pc);

	CHANGE_PC(tms.acc);

	CYCLES(4);
}

static void op_calad(void)
{
	fatalerror("32051: unimplemented op calad at %08X", tms.pc-1);
}

static void op_call(void)
{
	UINT16 pma = ROPCODE();
	GET_ADDRESS();		// update AR/ARP
	PUSH_PC(tms.pc);

	CHANGE_PC(pma);

	CYCLES(4);
}

static void op_calld(void)
{
	fatalerror("32051: unimplemented op calld at %08X", tms.pc-1);
}

static void op_cc(void)
{
	fatalerror("32051: unimplemented op cc at %08X", tms.pc-1);
}

static void op_ccd(void)
{
	fatalerror("32051: unimplemented op ccd at %08X", tms.pc-1);
}

static void op_intr(void)
{
	fatalerror("32051: unimplemented op intr at %08X", tms.pc-1);
}

static void op_nmi(void)
{
	fatalerror("32051: unimplemented op nmi at %08X", tms.pc-1);
}

static void op_ret(void)
{
	UINT16 pc = POP_PC();
	CHANGE_PC(pc);

	CYCLES(4);
}

static void op_retc(void)
{
	fatalerror("32051: unimplemented op retc at %08X", tms.pc-1);
}

static void op_retcd(void)
{
	fatalerror("32051: unimplemented op retcd at %08X", tms.pc-1);
}

static void op_retd(void)
{
	fatalerror("32051: unimplemented op retd at %08X", tms.pc-1);
}

static void op_rete(void)
{
	fatalerror("32051: unimplemented op rete at %08X", tms.pc-1);
}

static void op_reti(void)
{
	fatalerror("32051: unimplemented op reti at %08X", tms.pc-1);
}

static void op_trap(void)
{
	fatalerror("32051: unimplemented op trap at %08X", tms.pc-1);
}

static void op_xc(void)
{
	fatalerror("32051: unimplemented op xc at %08X", tms.pc-1);
}

/*****************************************************************************/

static void op_bldd_slimm(void)
{
	UINT16 pfc = ROPCODE();

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = DM_READ16(pfc);
		DM_WRITE16(ea, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

static void op_bldd_dlimm(void)
{
	UINT16 pfc = ROPCODE();

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = DM_READ16(ea);
		DM_WRITE16(pfc, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

static void op_bldd_sbmar(void)
{
	fatalerror("32051: unimplemented op bldd sbmar at %08X", tms.pc-1);
}

static void op_bldd_dbmar(void)
{
	UINT16 pfc = tms.bmar;

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = DM_READ16(ea);
		DM_WRITE16(pfc, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

static void op_bldp(void)
{
	UINT16 pfc = tms.bmar;

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = DM_READ16(ea);
		PM_WRITE16(pfc, data);
		pfc++;
		CYCLES(1);

		tms.rptc--;
	};
}

static void op_blpd_bmar(void)
{
	fatalerror("32051: unimplemented op bpld bmar at %08X", tms.pc-1);
}

static void op_blpd_imm(void)
{
	UINT16 pfc = ROPCODE();

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = PM_READ16(pfc);
		DM_WRITE16(ea, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

/*****************************************************************************/

static void op_dmov(void)
{
	fatalerror("32051: unimplemented op dmov at %08X", tms.pc-1);
}

static void op_in(void)
{
	fatalerror("32051: unimplemented op in at %08X", tms.pc-1);
}

static void op_lmmr(void)
{
	UINT16 pfc = ROPCODE();

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = DM_READ16(pfc);
		DM_WRITE16(ea & 0x7f, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

static void op_out(void)
{
	fatalerror("32051: unimplemented op out at %08X", tms.pc-1);
}

static void op_smmr(void)
{
	UINT16 pfc = ROPCODE();

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = DM_READ16(ea & 0x7f);
		DM_WRITE16(pfc, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

static void op_tblr(void)
{
	UINT16 pfc = (UINT16)(tms.acc);

	while (tms.rptc > -1)
	{
		UINT16 ea = GET_ADDRESS();
		UINT16 data = PM_READ16(pfc);
		DM_WRITE16(ea, data);
		pfc++;
		CYCLES(2);

		tms.rptc--;
	};
}

static void op_tblw(void)
{
	fatalerror("32051: unimplemented op tblw at %08X", tms.pc-1);
}

/*****************************************************************************/

static void op_apl_dbmr(void)
{
	fatalerror("32051: unimplemented op apl dbmr at %08X", tms.pc-1);
}

static void op_apl_imm(void)
{
	fatalerror("32051: unimplemented op apl imm at %08X", tms.pc-1);
}

static void op_cpl_dbmr(void)
{
	fatalerror("32051: unimplemented op cpl dbmr at %08X", tms.pc-1);
}

static void op_cpl_imm(void)
{
	UINT16 imm = ROPCODE();
	UINT16 ea = GET_ADDRESS();
	UINT16 data = DM_READ16(ea);

	if (data == imm)
	{
		tms.st1.tc = 1;
	}
	else
	{
		tms.st1.tc = 0;
	}

	CYCLES(1);
}

static void op_opl_dbmr(void)
{
	fatalerror("32051: unimplemented op opl dbmr at %08X", tms.pc-1);
}

static void op_opl_imm(void)
{
	UINT16 ea = GET_ADDRESS();
	UINT16 imm = ROPCODE();
	UINT16 data = DM_READ16(ea);
	data |= imm;
	DM_WRITE16(ea, data);
	CYCLES(1);
}

static void op_splk(void)
{
	UINT16 ea = GET_ADDRESS();
	UINT16 imm = ROPCODE();

	DM_WRITE16(ea, imm);

	CYCLES(2);
}

static void op_xpl_dbmr(void)
{
	fatalerror("32051: unimplemented op xpl dbmr at %08X", tms.pc-1);
}

static void op_xpl_imm(void)
{
	fatalerror("32051: unimplemented op xpl imm at %08X", tms.pc-1);
}

static void op_apac(void)
{
	fatalerror("32051: unimplemented op apac at %08X", tms.pc-1);
}

static void op_lph(void)
{
	fatalerror("32051: unimplemented op lph at %08X", tms.pc-1);
}

static void op_lt(void)
{
	fatalerror("32051: unimplemented op lt at %08X", tms.pc-1);
}

static void op_lta(void)
{
	fatalerror("32051: unimplemented op lta at %08X", tms.pc-1);
}

static void op_ltd(void)
{
	fatalerror("32051: unimplemented op ltd at %08X", tms.pc-1);
}

static void op_ltp(void)
{
	fatalerror("32051: unimplemented op ltp at %08X", tms.pc-1);
}

static void op_lts(void)
{
	fatalerror("32051: unimplemented op lts at %08X", tms.pc-1);
}

static void op_mac(void)
{
	fatalerror("32051: unimplemented op mac at %08X", tms.pc-1);
}

static void op_macd(void)
{
	fatalerror("32051: unimplemented op macd at %08X", tms.pc-1);
}

static void op_madd(void)
{
	fatalerror("32051: unimplemented op madd at %08X", tms.pc-1);
}

static void op_mads(void)
{
	fatalerror("32051: unimplemented op mads at %08X", tms.pc-1);
}

static void op_mpy_mem(void)
{
	fatalerror("32051: unimplemented op mpy mem at %08X", tms.pc-1);
}

static void op_mpy_simm(void)
{
	fatalerror("32051: unimplemented op mpy simm at %08X", tms.pc-1);
}

static void op_mpy_limm(void)
{
	fatalerror("32051: unimplemented op mpy limm at %08X", tms.pc-1);
}

static void op_mpya(void)
{
	fatalerror("32051: unimplemented op mpya at %08X", tms.pc-1);
}

static void op_mpys(void)
{
	fatalerror("32051: unimplemented op mpys at %08X", tms.pc-1);
}

static void op_mpyu(void)
{
	fatalerror("32051: unimplemented op mpyu at %08X", tms.pc-1);
}

static void op_pac(void)
{
	fatalerror("32051: unimplemented op pac at %08X", tms.pc-1);
}

static void op_spac(void)
{
	fatalerror("32051: unimplemented op spac at %08X", tms.pc-1);
}

static void op_sph(void)
{
	fatalerror("32051: unimplemented op sph at %08X", tms.pc-1);
}

static void op_spl(void)
{
	fatalerror("32051: unimplemented op spl at %08X", tms.pc-1);
}

static void op_spm(void)
{
	fatalerror("32051: unimplemented op spm at %08X", tms.pc-1);
}

static void op_sqra(void)
{
	fatalerror("32051: unimplemented op sqra at %08X", tms.pc-1);
}

static void op_sqrs(void)
{
	fatalerror("32051: unimplemented op sqrs at %08X", tms.pc-1);
}

static void op_zpr(void)
{
	fatalerror("32051: unimplemented op zpr at %08X", tms.pc-1);
}

static void op_bit(void)
{
	UINT16 ea = GET_ADDRESS();
	UINT16 data = DM_READ16(ea);
	int bit = 15 - ((tms.op >> 8) & 0xf);

	if (data & (1 << bit))
	{
		tms.st1.tc = 1;
	}
	else
	{
		tms.st1.tc = 0;
	}

	CYCLES(1);
}

static void op_bitt(void)
{
	fatalerror("32051: unimplemented op bitt at %08X", tms.pc-1);
}

static void op_clrc_ov(void)
{
	tms.st0.ovm = 0;

	CYCLES(1);
}

static void op_clrc_ext(void)
{
	tms.st1.sxm = 0;

	CYCLES(1);
}

static void op_clrc_hold(void)
{
	fatalerror("32051: unimplemented op clrc hold at %08X", tms.pc-1);
}

static void op_clrc_tc(void)
{
	fatalerror("32051: unimplemented op clrc tc at %08X", tms.pc-1);
}

static void op_clrc_carry(void)
{
	fatalerror("32051: unimplemented op clrc carry at %08X", tms.pc-1);
}

static void op_clrc_cnf(void)
{
	tms.st1.cnf = 0;

	CYCLES(1);
}

static void op_clrc_intm(void)
{
	tms.st0.intm = 0;

	CYCLES(1);
}

static void op_clrc_xf(void)
{
	fatalerror("32051: unimplemented op clrc xf at %08X", tms.pc-1);
}

static void op_idle(void)
{
	fatalerror("32051: unimplemented op idle at %08X", tms.pc-1);
}

static void op_idle2(void)
{
	fatalerror("32051: unimplemented op idle2 at %08X", tms.pc-1);
}

static void op_lst_st0(void)
{
	fatalerror("32051: unimplemented op lst st0 at %08X", tms.pc-1);
}

static void op_lst_st1(void)
{
	fatalerror("32051: unimplemented op lst st1 at %08X", tms.pc-1);
}

static void op_pop(void)
{
	fatalerror("32051: unimplemented op pop at %08X", tms.pc-1);
}

static void op_popd(void)
{
	fatalerror("32051: unimplemented op popd at %08X", tms.pc-1);
}

static void op_pshd(void)
{
	fatalerror("32051: unimplemented op pshd at %08X", tms.pc-1);
}

static void op_push(void)
{
	fatalerror("32051: unimplemented op push at %08X", tms.pc-1);
}

static void op_rpt_mem(void)
{
	UINT16 ea = GET_ADDRESS();
	UINT16 data = DM_READ16(ea);
	tms.rptc = data;
	tms.rpt_start = tms.pc;
	tms.rpt_end = tms.pc+1;

	CYCLES(1);
}

static void op_rpt_limm(void)
{
	tms.rptc = (UINT16)ROPCODE();
	tms.rpt_start = tms.pc;
	tms.rpt_end = tms.pc+1;

	CYCLES(2);
}

static void op_rpt_simm(void)
{
	tms.rptc = (tms.op & 0xff);
	tms.rpt_start = tms.pc;
	tms.rpt_end = tms.pc+1;

	CYCLES(1);
}

static void op_rptb(void)
{
	UINT16 pma = ROPCODE();
	tms.pmst.braf = 1;
	tms.pasr = tms.pc;
	tms.paer = pma + 1;

	CYCLES(2);
}

static void op_rptz(void)
{
	fatalerror("32051: unimplemented op rptz at %08X", tms.pc-1);
}

static void op_setc_ov(void)
{
	tms.st0.ovm = 1;

	CYCLES(1);
}

static void op_setc_ext(void)
{
	tms.st1.sxm = 1;

	CYCLES(1);
}

static void op_setc_hold(void)
{
	fatalerror("32051: unimplemented op setc hold at %08X", tms.pc-1);
}

static void op_setc_tc(void)
{
	fatalerror("32051: unimplemented op setc tc at %08X", tms.pc-1);
}

static void op_setc_carry(void)
{
	fatalerror("32051: unimplemented op setc carry at %08X", tms.pc-1);
}

static void op_setc_xf(void)
{
	fatalerror("32051: unimplemented op setc xf at %08X", tms.pc-1);
}

static void op_setc_cnf(void)
{
	fatalerror("32051: unimplemented op setc cnf at %08X", tms.pc-1);
}

static void op_setc_intm(void)
{
	tms.st0.intm = 1;

	CYCLES(1);
}

static void op_sst_st0(void)
{
	fatalerror("32051: unimplemented op sst st0 at %08X", tms.pc-1);
}

static void op_sst_st1(void)
{
	fatalerror("32051: unimplemented op sst st1 at %08X", tms.pc-1);
}
