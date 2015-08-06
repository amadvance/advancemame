/* compute operations */

#include <math.h>

#define CLEAR_ALU_FLAGS()		(sharc.astat &= ~(AZ|AN|AV|AC|AS|AI))

#define SET_FLAG_AZ(r)			{ sharc.astat |= (((r) == 0) ? AZ : 0); }
#define SET_FLAG_AN(r)			{ sharc.astat |= (((r) & 0x80000000) ? AN : 0); }
#define SET_FLAG_AC_ADD(r,a,b)	{ sharc.astat |= (((UINT32)r < (UINT32)a) ? AC : 0); }
#define SET_FLAG_AV_ADD(r,a,b)	{ sharc.astat |= (((~((a) ^ (b)) & ((a) ^ (r))) & 0x80000000) ? AV : 0); }
#define SET_FLAG_AC_SUB(r,a,b)	{ sharc.astat |= ((!((UINT32)a < (UINT32)b)) ? AC : 0); }
#define SET_FLAG_AV_SUB(r,a,b)	{ sharc.astat |= ((( ((a) ^ (b)) & ((a) ^ (r))) & 0x80000000) ? AV : 0); }

#define IS_FLOAT_ZERO(r)		((((r) & 0x7fffffff) == 0))
#define IS_FLOAT_DENORMAL(r)	((((r) & 0x7f800000) == 0) && (((r) & 0x7fffff) != 0))
#define IS_FLOAT_NAN(r)			((((r) & 0x7f800000) == 0x7f800000) && (((r) & 0x7fffff) != 0))
#define IS_FLOAT_INFINITY(r)	(((r) & 0x7fffffff) == 0x7f800000)

#define CLEAR_MULTIPLIER_FLAGS()	(sharc.astat &= ~(MN|MV|MU|MI))

#define SET_FLAG_MN(r)			{ sharc.astat |= (((r) & 0x80000000) ? MN : 0); }
#define SET_FLAG_MV(r)			{ sharc.astat |= ((((UINT32)((r) >> 32) != 0) && ((UINT32)((r) >> 32) != 0xffffffff)) ? MV : 0); }

/* TODO: MU needs 80-bit result */
#define SET_FLAG_MU(r)			{ sharc.astat |= ((((UINT32)((r) >> 32) == 0) && ((UINT32)(r)) != 0) ? MU : 0); }


/*****************************************************************************/
/* Integer ALU operations */

/* Rn = Rx + Ry */
INLINE void compute_add(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) + REG(ry);

	if(sharc.mode1 & ALUSAT)
		fatalerror("SHARC: compute_add: ALU saturation not implemented !");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, REG(rx), REG(ry));
	SET_FLAG_AC_ADD(r, REG(rx), REG(ry));
	REG(rn) = r;
}

/* Rn = Rx - Ry */
INLINE void compute_sub(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) - REG(ry);

	if(sharc.mode1 & ALUSAT)
		fatalerror("SHARC: compute_sub: ALU saturation not implemented !");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, REG(rx), REG(ry));
	SET_FLAG_AC_SUB(r, REG(rx), REG(ry));
	REG(rn) = r;
}

/* Rn = Rx + Ry + CI */
INLINE void compute_add_ci(int rn, int rx, int ry)
{
	int c = (sharc.astat & AC) ? 1 : 0;
	UINT32 r = REG(rx) + REG(ry) + c;

	if(sharc.mode1 & ALUSAT)
		fatalerror("SHARC: compute_add_ci: ALU saturation not implemented !");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, REG(rx), REG(ry)+c);
	SET_FLAG_AC_ADD(r, REG(rx), REG(ry)+c);
	REG(rn) = r;
}

/* Rn = Rx - Ry + CI - 1 */
INLINE void compute_sub_ci(int rn, int rx, int ry)
{
	int c = (sharc.astat & AC) ? 1 : 0;
	UINT32 r = REG(rx) - REG(ry) + c - 1;

	if(sharc.mode1 & ALUSAT)
		fatalerror("SHARC: compute_sub_ci: ALU saturation not implemented !");

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, REG(rx), REG(ry)+c-1);
	SET_FLAG_AC_SUB(r, REG(rx), REG(ry)+c-1);
	REG(rn) = r;
}

/* Rn = Rx AND Ry */
INLINE void compute_and(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) & REG(ry);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	REG(rn) = r;
}

/* COMP(Rx, Ry) */
INLINE void compute_comp(int rx, int ry)
{
	UINT32 comp_accum;

	CLEAR_ALU_FLAGS();
	if( REG(rx) == REG(ry) )
		sharc.astat |= AZ;
	if( (INT32)REG(rx) < (INT32)REG(ry) )
		sharc.astat |= AN;

	// Update ASTAT compare accumulation register
	comp_accum = (sharc.astat >> 24) & 0xff;
	comp_accum >>= 1;
	if ( (INT32)REG(rx) > (INT32)REG(ry) )
	{
		comp_accum |= 0x80;
	}
	sharc.astat &= 0xffffff;
	sharc.astat |= comp_accum << 24;
}

/* Rn = PASS Rx */
INLINE void compute_pass(int rn, int rx)
{
	CLEAR_ALU_FLAGS();
	/* TODO: floating-point extension field is set to 0 */

	REG(rn) = REG(rx);
	if (REG(rn) == 0)
		sharc.astat |= AZ;
	if (REG(rn) & 0x80000000)
		sharc.astat |= AN;
}

/* Rn = Rx XOR Ry */
INLINE void compute_xor(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) ^ REG(ry);
	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	REG(rn) = r;
}

/* Rn = Rx OR Ry */
INLINE void compute_or(int rn, int rx, int ry)
{
	UINT32 r = REG(rx) | REG(ry);
	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	REG(rn) = r;
}

/* Rn = Rx + 1 */
INLINE void compute_inc(int rn, int rx)
{
	UINT32 r = REG(rx) + 1;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_ADD(r, REG(rx), 1);
	SET_FLAG_AC_ADD(r, REG(rx), 1);

	REG(rn) = r;
}

/* Rn = Rx - 1 */
INLINE void compute_dec(int rn, int rx)
{
	UINT32 r = REG(rx) - 1;

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);
	SET_FLAG_AV_SUB(r, REG(rx), 1);
	SET_FLAG_AC_SUB(r, REG(rx), 1);

	REG(rn) = r;
}

/* Rn = MAX(Rx, Ry) */
INLINE void compute_max(int rn, int rx, int ry)
{
	UINT32 r = MAX((INT32)REG(rx), (INT32)REG(ry));

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r);
	SET_FLAG_AZ(r);

	REG(rn) = r;
}

/* Rn = -Rx */
INLINE void compute_neg(int rn, int rx)
{
	UINT32 r = -REG(rx);

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(REG(rn));
	SET_FLAG_AZ(REG(rn));
	SET_FLAG_AV_SUB(r, 0, REG(rx));
	SET_FLAG_AC_SUB(r, 0, REG(rx));

	REG(rn) = r;
}

/*****************************************************************************/
/* Floating-point ALU operations */

/* Fn = FLOAT Rx */
INLINE void compute_float(int rn, int rx)
{
	FREG(rn) = (float)(INT32)REG(rx);

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(REG(rn));
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(REG(rn)) || IS_FLOAT_ZERO(REG(rn))) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(REG(rn))) ? AUS : 0;
	/* TODO: AV flag */
}

/* Rn = FIX Fx */
INLINE void compute_fix(int rn, int rx)
{
	INT32 alu_i;
	SHARC_REG r_alu;

	r_alu.f = FREG(rx);
	if (sharc.mode1 & TRUNCATE)
	{
		alu_i = (INT32)(r_alu.f);
	}
	else
	{
		alu_i = (INT32)(r_alu.f < 0 ? (r_alu.f - 0.5f) : (r_alu.f + 0.5f));
	}

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(alu_i);
	// AZ
	SET_FLAG_AZ(alu_i);
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;
	/* TODO: AV flag */

	REG(rn) = alu_i;
}

/* Rn = FIX Fx BY Ry */
INLINE void compute_fix_scaled(int rn, int rx, int ry)
{
	INT32 alu_i;
	SHARC_REG r_alu;

	r_alu.f = (FREG(rx) * (float)pow(2.0, (INT32)REG(ry)));
	if (sharc.mode1 & TRUNCATE)
	{
		alu_i = (INT32)(r_alu.f);
	}
	else
	{
		alu_i = (INT32)(r_alu.f < 0 ? (r_alu.f - 0.5f) : (r_alu.f + 0.5f));
	}

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(alu_i);
	// AZ
	SET_FLAG_AZ(alu_i);
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;
	/* TODO: AV flag */

	REG(rn) = alu_i;
}

/* Fn = FLOAT Rx BY Ry */
INLINE void compute_float_scaled(int rn, int rx, int ry)
{
	float r = (float)(INT32)REG(rx);

	r *= (float)pow(2.0, (INT32)REG(ry));

	FREG(rn) = r;

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(REG(rn));
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(REG(rn)) || IS_FLOAT_ZERO(REG(rn))) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(REG(rn))) ? AUS : 0;
	/* TODO: set AV if overflowed */
}

/* Rn = LOGB Fx */
INLINE void compute_logb(int rn, int rx)
{
	UINT32 r = REG(rx);

	int exponent = (r >> 23) & 0xff;
	exponent -= 127;

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(exponent);
	// AZ
	SET_FLAG_AZ(exponent);
	// AV
	sharc.astat |= (IS_FLOAT_INFINITY(REG(rx)) || IS_FLOAT_ZERO(REG(rx))) ? AV : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	REG(rn) = exponent;
}

/* Fn = SCALB Fx BY Fy */
INLINE void compute_scalb(int rn, int rx, int ry)
{
	SHARC_REG r;
	r.f = FREG(rx);
	r.f *= (float)pow(2.0, (INT32)REG(ry));

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(r.r);
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r.r) || IS_FLOAT_ZERO(r.r)) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(r.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}

/* Fn = Fx + Fy */
INLINE void compute_fadd(int rn, int rx, int ry)
{
	SHARC_REG r;
	r.f = FREG(rx) + FREG(ry);

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(r.r);
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r.r) || IS_FLOAT_ZERO(r.r)) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(r.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}

/* Fn = Fx - Fy */
INLINE void compute_fsub(int rn, int rx, int ry)
{
	SHARC_REG r;
	r.f = FREG(rx) - FREG(ry);

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(r.r);
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r.r) || IS_FLOAT_ZERO(r.r)) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(r.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}

/* Fn = -Fx */
INLINE void compute_fneg(int rn, int rx)
{
	SHARC_REG r;
	r.f = -FREG(rx);

	CLEAR_ALU_FLAGS();
	// AZ
	SET_FLAG_AZ(r.r);
	// AN
	SET_FLAG_AN(r.r);
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}

/* COMP(Fx, Fy) */
INLINE void compute_fcomp(int rx, int ry)
{
	UINT32 comp_accum;

	CLEAR_ALU_FLAGS();
	// AZ
	if( FREG(rx) == FREG(ry) )
		sharc.astat |= AZ;
	// AN
	if( FREG(rx) < FREG(ry) )
		sharc.astat |= AN;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	// Update ASTAT compare accumulation register
	comp_accum = (sharc.astat >> 24) & 0xff;
	comp_accum >>= 1;
	if ( FREG(rx) > FREG(ry) )
	{
		comp_accum |= 0x80;
	}
	sharc.astat &= 0xffffff;
	sharc.astat |= comp_accum << 24;
}

/* Fn = ABS(Fx + Fy) */
INLINE void compute_fabs_plus(int rn, int rx, int ry)
{
	SHARC_REG r;
	r.f = fabs(FREG(rx) + FREG(ry));

	CLEAR_ALU_FLAGS();
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r.r) || IS_FLOAT_ZERO(r.r)) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(r.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}

/* Fn = MAX(Fx, Fy) */
INLINE void compute_fmax(int rn, int rx, int ry)
{
	SHARC_REG r_alu;

	r_alu.f = MAX(FREG(rx), FREG(ry));

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_alu.r);
	// AZ
	sharc.astat |= (IS_FLOAT_ZERO(r_alu.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;
	/* TODO: AV flag */

	FREG(rn) = r_alu.f;
}

/* Fn = MIN(Fx, Fy) */
INLINE void compute_fmin(int rn, int rx, int ry)
{
	SHARC_REG r_alu;

	r_alu.f = MIN(FREG(rx), FREG(ry));

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_alu.r);
	// AZ
	sharc.astat |= (IS_FLOAT_ZERO(r_alu.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;
	/* TODO: AV flag */

	FREG(rn) = r_alu.f;
}

/* Fn = RECIPS Fx */
INLINE void compute_recips(int rn, int rx)
{
	SHARC_REG r;
	/* TODO: calculate reciprocal, this is too accurate! */
	r.f = 1.0f / FREG(rx);

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(r.r);
	// AZ & AV
	sharc.astat |= (IS_FLOAT_ZERO(r.r)) ? (AZ | AV) : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}

/* Fn = RSQRTS Fx */
INLINE void compute_rsqrts(int rn, int rx)
{
	SHARC_REG r;
	/* TODO: calculate reciprocal, this is too accurate! */
	r.f = 1.0f / sqrt(FREG(rx));

	CLEAR_ALU_FLAGS();
	// AN
	sharc.astat |= (r.r == 0x80000000) ? AN : 0;
	// AZ & AV
	sharc.astat |= (IS_FLOAT_ZERO(r.r)) ? (AZ | AV) : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || (REG(rx) & 0x80000000)) ? AI : 0;

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(rn) = r.f;
}


/* Fn = PASS Fx */
INLINE void compute_fpass(int rn, int rx)
{
	SHARC_REG r;
	r.f = FREG(rx);

	CLEAR_ALU_FLAGS();
	// AN
	SET_FLAG_AN(r.r);
	// AZ
	sharc.astat |= (IS_FLOAT_ZERO(r.r)) ? AZ : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx))) ? AI : 0;

	FREG(rn) = r.f;
}

/*****************************************************************************/
/* Multiplier opcodes */

/* Rn = (unsigned)Rx * (unsigned)Ry, integer, no rounding */
INLINE void compute_mul_uuin(int rn, int rx, int ry)
{
	UINT64 r = (UINT64)(UINT32)REG(rx) * (UINT64)(UINT32)REG(ry);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN((UINT32)r);
	SET_FLAG_MV(r);
	SET_FLAG_MU(r);

	REG(rn) = (UINT32)(r);
}

/* Rn = (signed)Rx * (signed)Ry, integer, no rounding */
INLINE void compute_mul_ssin(int rn, int rx, int ry)
{
	UINT64 r = (INT64)(INT32)REG(rx) * (INT64)(INT32)REG(ry);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN((UINT32)r);
	SET_FLAG_MV(r);
	SET_FLAG_MU(r);

	REG(rn) = (UINT32)(r);
}

/* MRF + (signed)Rx * (signed)Ry, integer, no rounding */
INLINE UINT32 compute_mrf_plus_mul_ssin(int rx, int ry)
{
	UINT64 r = sharc.mrf + ((INT64)(INT32)REG(rx) * (INT64)(INT32)REG(ry));

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN((UINT32)r);
	SET_FLAG_MV(r);
	SET_FLAG_MU(r);

	return (UINT32)(r);
}

/* MRB + (signed)Rx * (signed)Ry, integer, no rounding */
INLINE UINT32 compute_mrb_plus_mul_ssin(int rx, int ry)
{
	INT64 r = sharc.mrb + ((INT64)(INT32)REG(rx) * (INT64)(INT32)REG(ry));

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN((UINT32)r);
	SET_FLAG_MV(r);
	SET_FLAG_MU(r);

	return (UINT32)(r);
}

/* Fn = Fx * Fy */
INLINE void compute_fmul(int rn, int rx, int ry)
{
	FREG(rn) = FREG(rx) * FREG(ry);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(REG(rn));
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */
}

/*****************************************************************************/

/* multi function opcodes */

/* integer*/
INLINE void compute_multi_mr_to_reg(int ai, int rk)
{
	switch(ai)
	{
		case 0:		SET_UREG(rk, (UINT32)(sharc.mrf)); break;
		case 1:		SET_UREG(rk, (UINT32)(sharc.mrf >> 32)); break;
		case 2:		fatalerror("SHARC: tried to load MR2F"); break;
		case 4:		SET_UREG(rk, (UINT32)(sharc.mrb)); break;
		case 5:		SET_UREG(rk, (UINT32)(sharc.mrb >> 32)); break;
		case 6:		fatalerror("SHARC: tried to load MR2B"); break;
		default:	fatalerror("SHARC: unknown ai %d in mr_to_reg", ai);
	}

	CLEAR_MULTIPLIER_FLAGS();
}

INLINE void compute_multi_reg_to_mr(int ai, int rk)
{
	switch(ai)
	{
		case 0:		sharc.mrf &= ~0xffffffff; sharc.mrf |= GET_UREG(rk); break;
		case 1:		sharc.mrf &= 0xffffffff; sharc.mrf |= (UINT64)(GET_UREG(rk)) << 32; break;
		case 2:		fatalerror("SHARC: tried to write MR2F"); break;
		case 4:		sharc.mrb &= ~0xffffffff; sharc.mrb |= GET_UREG(rk); break;
		case 5:		sharc.mrb &= 0xffffffff; sharc.mrb |= (UINT64)(GET_UREG(rk)) << 32; break;
		case 6:		fatalerror("SHARC: tried to write MR2B"); break;
		default:	fatalerror("SHARC: unknown ai %d in reg_to_mr", ai);
	}

	CLEAR_MULTIPLIER_FLAGS();
}

/* Ra = Rx + Ry,   Rs = Rx - Ry */
INLINE void compute_dual_add_sub(int ra, int rs, int rx, int ry)
{
	UINT32 r_add = REG(rx) + REG(ry);
	UINT32 r_sub = REG(rx) - REG(ry);

	REG(ra) = r_add;
	REG(rs) = r_sub;

	CLEAR_ALU_FLAGS();
	if (r_add == 0 || r_sub == 0)
	{
		sharc.astat |= AZ;
	}
	if (r_add & 0x80000000 || r_sub & 0x80000000)
	{
		sharc.astat |= AN;
	}
	if (((~(REG(rx) ^ REG(ry)) & (REG(rx) ^ r_add)) & 0x80000000) ||
		(( (REG(rx) ^ REG(ry)) & (REG(rx) ^ r_sub)) & 0x80000000))
	{
		sharc.astat |= AV;
	}
	if (((UINT32)r_add < (UINT32)REG(rx)) ||
		(!(UINT32)r_sub < (UINT32)REG(rx)))
	{
		sharc.astat |= AC;
	}
}

/* Rm = (signed)Rxm * (signed)Rym, fractional, rounding,   Ra = Rxa + Rya */
INLINE void compute_mul_ssfr_add(int rm, int rxm, int rym, int ra, int rxa, int rya)
{
	UINT32 r_mul = (UINT32)(((INT64)(REG(rxm)) * (INT64)(REG(rym))) >> 31);
	UINT32 r_add = REG(rxa) + REG(rya);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_add);
	SET_FLAG_AZ(r_add);
	SET_FLAG_AV_ADD(r_add, REG(rxa), REG(rya));
	SET_FLAG_AC_ADD(r_add, REG(rxa), REG(rya));


	REG(rm) = r_mul;
	REG(ra) = r_add;
}

/* Rm = (signed)Rxm * (signed)Rym, fractional, rounding,   Ra = Rxa - Rya */
INLINE void compute_mul_ssfr_sub(int rm, int rxm, int rym, int ra, int rxa, int rya)
{
	UINT32 r_mul = (UINT32)(((INT64)(REG(rxm)) * (INT64)(REG(rym))) >> 31);
	UINT32 r_sub = REG(rxa) - REG(rya);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_sub);
	SET_FLAG_AZ(r_sub);
	SET_FLAG_AV_SUB(r_sub, REG(rxa), REG(rya));
	SET_FLAG_AC_SUB(r_sub, REG(rxa), REG(rya));


	REG(rm) = r_mul;
	REG(ra) = r_sub;
}


/* floating-point */

/* Fa = Fx + Fy,   Fs = Fx - Fy */
INLINE void compute_dual_fadd_fsub(int ra, int rs, int rx, int ry)
{
	SHARC_REG r_add, r_sub;
	r_add.f = FREG(rx) + FREG(ry);
	r_sub.f = FREG(rx) - FREG(ry);

	CLEAR_ALU_FLAGS();
	// AN
	sharc.astat |= ((r_add.r & 0x80000000) || (r_sub.r & 0x80000000)) ? AN : 0;
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r_add.r) || IS_FLOAT_ZERO(r_add.r) ||
					IS_FLOAT_DENORMAL(r_sub.r) || IS_FLOAT_ZERO(r_sub.r)) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(r_add.r) || IS_FLOAT_DENORMAL(r_sub.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(rx)) || IS_FLOAT_NAN(REG(ry))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(ra) = r_add.f;
	FREG(rs) = r_sub.f;
}

/* Fm = Fxm * Fym,   Fa = Fxa + Fya */
INLINE void compute_fmul_fadd(int fm, int fxm, int fym, int fa, int fxa, int fya)
{
	SHARC_REG r_mul, r_add;
	r_mul.f = FREG(fxm) * FREG(fym);
	r_add.f = FREG(fxa) + FREG(fya);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_add.r);
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r_add.r) || IS_FLOAT_ZERO(r_add.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_add.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(fxa)) || IS_FLOAT_NAN(REG(fya))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(fm) = r_mul.f;
	FREG(fa) = r_add.f;
}

/* Fm = Fxm * Fym,   Fa = Fxa - Fya */
INLINE void compute_fmul_fsub(int fm, int fxm, int fym, int fa, int fxa, int fya)
{
	SHARC_REG r_mul, r_sub;
	r_mul.f = FREG(fxm) * FREG(fym);
	r_sub.f = FREG(fxa) - FREG(fya);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_sub.r);
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r_sub.r) || IS_FLOAT_ZERO(r_sub.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_sub.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(fxa)) || IS_FLOAT_NAN(REG(fya))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(fm) = r_mul.f;
	FREG(fa) = r_sub.f;
}

/* Fm = Fxm * Fym,   Fa = FLOAT Fxa BY Fya */
INLINE void compute_fmul_float_scaled(int fm, int fxm, int fym, int fa, int fxa, int fya)
{
	SHARC_REG r_mul, r_alu;
	r_mul.f = FREG(fxm) * FREG(fym);

	r_alu.f = (float)(INT32)REG(fxa);
	r_alu.f *= (float)pow(2.0, (INT32)REG(fya));

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_alu.r);
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r_alu.r) || IS_FLOAT_ZERO(r_alu.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	/* TODO: set AV if overflowed */

	FREG(fm) = r_mul.f;
	FREG(fa) = r_alu.f;
}

/* Fm = Fxm * Fym,   Fa = FIX Fxa BY Fya */
INLINE void compute_fmul_fix_scaled(int fm, int fxm, int fym, int fa, int fxa, int fya)
{
	INT32 alu_i;
	SHARC_REG r_mul, r_alu;
	r_mul.f = FREG(fxm) * FREG(fym);

	r_alu.f = (FREG(fxa) * (float)pow(2.0, (INT32)REG(fya)));
	if (sharc.mode1 & TRUNCATE)
	{
		alu_i = (INT32)(r_alu.f);
	}
	else
	{
		alu_i = (INT32)(r_alu.f < 0 ? (r_alu.f - 0.5f) : (r_alu.f + 0.5f));
	}

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(alu_i);
	// AZ
	SET_FLAG_AZ(alu_i);
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(fxa))) ? AI : 0;
	/* TODO: AV flag */

	FREG(fm) = r_mul.f;
	REG(fa) = alu_i;
}


/* Fm = Fxm * Fym,   Fa = MAX(Fxa, Fya) */
INLINE void compute_fmul_fmax(int fm, int fxm, int fym, int fa, int fxa, int fya)
{
	SHARC_REG r_mul, r_alu;
	r_mul.f = FREG(fxm) * FREG(fym);

	r_alu.f = MAX(FREG(fxa), FREG(fya));

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_alu.r);
	// AZ
	sharc.astat |= (IS_FLOAT_ZERO(r_alu.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(fxa)) || IS_FLOAT_NAN(REG(fya))) ? AI : 0;
	/* TODO: AV flag */

	FREG(fm) = r_mul.f;
	FREG(fa) = r_alu.f;
}


/* Fm = Fxm * Fym,   Fa = MIN(Fxa, Fya) */
INLINE void compute_fmul_fmin(int fm, int fxm, int fym, int fa, int fxa, int fya)
{
	SHARC_REG r_mul, r_alu;
	r_mul.f = FREG(fxm) * FREG(fym);

	r_alu.f = MIN(FREG(fxa), FREG(fya));

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	SET_FLAG_AN(r_alu.r);
	// AZ
	sharc.astat |= (IS_FLOAT_ZERO(r_alu.r)) ? AZ : 0;
	// AU
	sharc.stky |= (IS_FLOAT_DENORMAL(r_alu.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(fxa)) || IS_FLOAT_NAN(REG(fya))) ? AI : 0;
	/* TODO: AV flag */

	FREG(fm) = r_mul.f;
	FREG(fa) = r_alu.f;
}



/* Fm = Fxm * Fym,   Fa = Fxa + Fya,   Fs = Fxa - Fya */
INLINE void compute_fmul_dual_fadd_fsub(int fm, int fxm, int fym, int fa, int fs, int fxa, int fya)
{
	SHARC_REG r_mul, r_add, r_sub;
	r_mul.f = FREG(fxm) * FREG(fym);
	r_add.f = FREG(fxa) + FREG(fya);
	r_sub.f = FREG(fxa) - FREG(fya);

	CLEAR_MULTIPLIER_FLAGS();
	SET_FLAG_MN(r_mul.r);
	/* TODO: MV flag */
	/* TODO: MU flag */
	/* TODO: MI flag */

	CLEAR_ALU_FLAGS();
	// AN
	sharc.astat |= ((r_add.r & 0x80000000) || (r_sub.r & 0x80000000)) ? AN : 0;
	// AZ
	sharc.astat |= (IS_FLOAT_DENORMAL(r_add.r) || IS_FLOAT_ZERO(r_add.r) ||
					IS_FLOAT_DENORMAL(r_sub.r) || IS_FLOAT_ZERO(r_sub.r)) ? AZ : 0;
	// AUS
	sharc.stky |= (IS_FLOAT_DENORMAL(r_add.r) || IS_FLOAT_DENORMAL(r_sub.r)) ? AUS : 0;
	// AI
	sharc.astat |= (IS_FLOAT_NAN(REG(fxa)) || IS_FLOAT_NAN(REG(fya))) ? AI : 0;
	/* TODO: AV flag */

	// AIS
	if (sharc.astat & AI)	sharc.stky |= AIS;

	FREG(fm) = r_mul.f;
	FREG(fa) = r_add.f;
	FREG(fs) = r_sub.f;
}
