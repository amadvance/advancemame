/*
   Analog Devices ADSP-2106x SHARC Disassembler

   Written by Ville Linde for use in MAME
*/

#include "cpuintrf.h"
#include "sharcdsm.h"
#include <stdarg.h>

#define GET_UREG(x)		(ureg_names[x])
#define GET_SREG(x)		(GET_UREG(0x70 | (x & 0xf)))
#define GET_DREG(x)		(GET_UREG(0x00 | (x & 0xf)))
#define GET_DAG1_I(x)	(GET_UREG(0x10 | (x & 0x7)))
#define GET_DAG1_M(x)	(GET_UREG(0x20 | (x & 0x7)))
#define GET_DAG1_L(x)	(GET_UREG(0x30 | (x & 0x7)))
#define GET_DAG1_B(x)	(GET_UREG(0x40 | (x & 0x7)))
#define GET_DAG2_I(x)	(GET_UREG(0x10 | (8 + (x & 0x7))))
#define GET_DAG2_M(x)	(GET_UREG(0x20 | (8 + (x & 0x7))))
#define GET_DAG2_L(x)	(GET_UREG(0x30 | (8 + (x & 0x7))))
#define GET_DAG2_B(x)	(GET_UREG(0x40 | (8 + (x & 0x7))))

#define SIGN_EXTEND6(x)		((x & 0x20) ? (0xffffffc0 | x) : x)
#define SIGN_EXTEND24(x)	((x & 0x800000) ? (0xff000000 | x) : x)


static char *output;
static void print(const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	output += vsprintf(output, fmt, vl);
	va_end(vl);
}



static void compute(UINT32 opcode)
{
	int op = (opcode >> 12) & 0xff;
	int cu = (opcode >> 20) & 0x3;
	int rn = (opcode >> 8) & 0xf;
	int rx = (opcode >> 4) & 0xf;
	int ry = (opcode >> 0) & 0xf;
	int rs = (opcode >> 12) & 0xf;
	int ra = rn;
	int rm = rs;

	if( opcode & 0x400000 ) {	/* Multi-function opcode */
		int multiop = (opcode >> 16) & 0x3f;
		int rxm = (opcode >> 6) & 0x3;
		int rym = (opcode >> 4) & 0x3;
		int rxa = (opcode >> 2) & 0x3;
		int rya = (opcode >> 0) & 0x3;

		switch(multiop)
		{
			case 0x04:		print("R%d = R%d * R%d (SSFR),  R%d = R%d + R%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x05:		print("R%d = R%d * R%d (SSFR),  R%d = R%d - R%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x06:		print("R%d = R%d * R%d (SSFR),  R%d = (R%d + R%d)/2", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x08:		print("MRF = MRF + R%d * R%d (SSF),  R%d = R%d + R%d", rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x09:		print("MRF = MRF + R%d * R%d (SSF),  R%d = R%d - R%d", rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x0a:		print("MRF = MRF + R%d * R%d (SSF),  R%d = (R%d + R%d)/2", rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x0c:		print("R%d = MRF + R%d * R%d (SSFR),  R%d = R%d + R%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x0d:		print("R%d = MRF + R%d * R%d (SSFR),  R%d = R%d - R%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x0e:		print("R%d = MRF + R%d * R%d (SSFR),  R%d = (R%d + R%d)/2", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x10:		print("MRF = MRF - R%d * R%d (SSF),  R%d = R%d + R%d", rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x11:		print("MRF = MRF - R%d * R%d (SSF),  R%d = R%d - R%d", rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x12:		print("MRF = MRF - R%d * R%d (SSF),  R%d = (R%d + R%d)/2", rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x14:		print("R%d = MRF - R%d * R%d (SSFR),  R%d = R%d + R%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x15:		print("R%d = MRF - R%d * R%d (SSFR),  R%d = R%d - R%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x16:		print("R%d = MRF - R%d * R%d (SSFR),  R%d = (R%d + R%d)/2", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x18:		print("F%d = F%d * F%d,  F%d = F%d + F%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x19:		print("F%d = F%d * F%d,  F%d = F%d - F%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x1a:		print("F%d = F%d * F%d,  F%d = FLOAT F%d BY F%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x1b:		print("F%d = F%d * F%d,  F%d = FIX F%d BY F%d", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x1c:		print("F%d = F%d * F%d,  F%d = (F%d + F%d)/2", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x1d:		print("F%d = F%d * F%d,  F%d = ABS F%d", rm, rxm, rym+4, ra, rxa+8); break;
			case 0x1e:		print("F%d = F%d * F%d,  F%d = MAX(F%d, F%d)", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			case 0x1f:		print("F%d = F%d * F%d,  F%d = MIN(F%d, F%d)", rm, rxm, rym+4, ra, rxa+8, rya+12); break;
			default:
				if( ((multiop >> 4) & 0x3) == 0x2 ) {
					print("R%d = R%d * R%d (SSFR), R%d = R%d + R%d, R%d = R%d - R%d", rm, rxm, rym+4, ra, rxa+8, rya+12, rs, rxa+8, rya+12);
				}
				else if( ((multiop >> 4) & 0x3) == 0x3 ) {
					print("F%d = F%d * F%d, F%d = F%d + F%d, F%d = F%d - F%d", rm, rxm, rym+4, ra, rxa+8, rya+12, rs, rxa+8, rya+12);
				}
				else if( ((multiop >> 1) & 0x1f) == 0 ) {		/* MR=Rn / Rn=MR */
					int t = opcode & 0x10000;
					int rk = (opcode >> 8) & 0xf;
					int ai = (opcode >> 12) & 0xf;
					if( t ) {
						print("%s = R%d", mr_regnames[ai], rk);
					} else {
						print("R%d = %s", rk, mr_regnames[ai]);
					}
				}
				else {
					print("??? (COMPUTE, MULTIOP)");
				}
				break;
		}

	} else {					/* Single-function */
		switch(cu)
		{
			/******************/
			/* ALU operations */
			/******************/
			case 0:
			switch(op)
			{
				/* Fixed-point */
				case 0x01:	print("R%d = R%d + R%d", rn, rx, ry); break;
				case 0x02:	print("R%d = R%d - R%d", rn, rx, ry); break;
				case 0x05:	print("R%d = R%d + R%d + CI", rn, rx, ry); break;
				case 0x06:	print("R%d = R%d - R%d + CI - 1", rn, rx, ry); break;
				case 0x09:	print("R%d = (R%d + R%d)/2", rn, rx, ry); break;
				case 0x0a:	print("COMP(R%d, R%d)", rx, ry); break;
				case 0x25:	print("R%d = R%d + CI", rn, rx); break;
				case 0x26:	print("R%d = R%d + CI - 1", rn, rx); break;
				case 0x29:	print("R%d = R%d + 1", rn, rx); break;
				case 0x2a:	print("R%d = R%d - 1", rn, rx); break;
				case 0x22:	print("R%d = -R%d", rn, rx); break;
				case 0x30:	print("R%d = ABS R%d", rn, rx); break;
				case 0x21:	print("R%d = PASS R%d", rn, rx); break;
				case 0x40:	print("R%d = R%d AND R%d", rn, rx, ry); break;
				case 0x41:	print("R%d = R%d OR R%d", rn, rx, ry); break;
				case 0x42:	print("R%d = R%d XOR R%d", rn, rx, ry); break;
				case 0x43:	print("R%d = NOT R%d", rn, rx); break;
				case 0x61:	print("R%d = MIN(R%d, R%d)", rn, rx, ry); break;
				case 0x62:	print("R%d = MAX(R%d, R%d)", rn, rx, ry); break;
				case 0x63:	print("R%d = CLIP R%d BY R%d", rn, rx, ry); break;
				/* Floating-point */
				case 0x81:	print("F%d = F%d + F%d", rn, rx, ry); break;
				case 0x82:	print("F%d = F%d - F%d", rn, rx, ry); break;
				case 0x91:	print("F%d = ABS(F%d + F%d)", rn, rx, ry); break;
				case 0x92:	print("F%d = ABS(F%d - F%d)", rn, rx, ry); break;
				case 0x89:	print("F%d = (F%d + F%d)/2", rn, rx, ry); break;
				case 0x8a:	print("COMP(F%d, F%d)", rx, ry); break;
				case 0xa2:	print("F%d = -F%d", rn, rx); break;
				case 0xb0:	print("F%d = ABS F%d", rn, rx); break;
				case 0xa1:	print("F%d = PASS F%d", rn, rx); break;
				case 0xa5:	print("F%d = RND R%d", rn, rx); break;
				case 0xbd:	print("F%d = SCALB F%d BY R%d", rn, rx, ry); break;
				case 0xad:	print("R%d = MANT F%d", rn, rx); break;
				case 0xc1:	print("R%d = LOGB F%d", rn, rx); break;
				case 0xd9:	print("R%d = FIX F%d BY R%d", rn, rx, ry); break;
				case 0xc9:	print("R%d = FIX F%d", rn, rx); break;
				case 0xdd:	print("R%d = TRUNC F%d BY R%d", rn, rx, ry); break;
				case 0xcd:	print("R%d = TRUNC F%d", rn, rx); break;
				case 0xda:	print("F%d = FLOAT R%d BY R%d", rn, rx, ry); break;
				case 0xca:	print("F%d = FLOAT R%d", rn, rx); break;
				case 0xc4:	print("F%d = RECIPS F%d", rn, rx); break;
				case 0xc5:	print("F%d = RSQRTS F%d", rn, rx); break;
				case 0xe0:	print("F%d = F%d COPYSIGN F%d", rn, rx, ry); break;
				case 0xe1:	print("F%d = MIN(F%d, F%d)", rn, rx, ry); break;
				case 0xe2:	print("F%d = MAX(F%d, F%d)", rn, rx, ry); break;
				case 0xe3:	print("F%d = CLIP F%d BY F%d", rn, rx, ry); break;
				default:
					if( ((op >> 4) & 0xf) == 0x7 ) {
						print("R%d = R%d + R%d,  R%d = R%d - R%d", ra, rx, ry, rs, rx, ry);
					} else if( ((op >> 4) & 0xf) == 0xf ) {
						print("F%d = F%d + F%d,  F%d = F%d - F%d", ra, rx, ry, rs, rx, ry);
					} else {
						print("??? (COMPUTE, ALU)");
					}
					break;
			}
			break;

			/*************************/
			/* Multiplier operations */
			/*************************/
			case 1:
				if( op == 0x30 ) {
					print("F%d = F%d * F%d", rn, rx, ry);
					return;
				}

				switch((op >> 1) & 0x3)
				{
					case 0:
					case 1:		print("R%d = ", rn); break;
					case 2:		print("MRF = "); break;
					case 3:		print("MRB = "); break;
				}
				switch((op >> 6) & 0x3)
				{
					case 0:
						switch((op >> 4) & 0x3)
						{
							case 0:		print("SAT %s", (op & 0x2) ? "MRB" : "MRF"); break;
							case 1:
								if( op & 0x8 ) {
									print("RND %s", (op & 0x2) ? "MRB" : "MRF");
								} else {
									print("0");
								}
								break;
						}
						break;

					case 1:
						print("R%d * R%d", rx, ry); break;

					case 2:
						print("%s +(R%d * R%d)", (op & 0x2) ? "MRB" : "MRF", rx, ry); break;

					case 3:
						print("%s -(R%d * R%d)", (op & 0x2) ? "MRB" : "MRF", rx, ry); break;
				}
			break;

			/**********************/
			/* Shifter operations */
			/**********************/
			case 2:
				switch(op)
				{
					case 0x00:		print("R%d = LSHIFT R%d BY R%d", rn, rx, ry); break;
					case 0x20:		print("R%d = R%d OR LSHIFT R%d BY R%d", rn, rn, rx, ry); break;
					case 0x04:		print("R%d = ASHIFT R%d BY R%d", rn, rx, ry); break;
					case 0x24:		print("R%d = R%d OR ASHIFT R%d BY R%d", rn, rn, rx, ry); break;
					case 0x08:		print("R%d = ROT R%d BY R%d", rn, rx, ry); break;
					case 0xc4:		print("R%d = BCLR R%d BY R%d", rn, rx, ry); break;
					case 0xc0:		print("R%d = BSET R%d BY R%d", rn, rx, ry); break;
					case 0xc8:		print("R%d = BTGL R%d BY R%d", rn, rx, ry); break;
					case 0xcc:		print("BTST R%d BY R%d", rx, ry); break;
					case 0x44:		print("R%d = FDEP R%d BY R%d", rn, rx, ry); break;
					case 0x64:		print("R%d = R%d OR FDEP R%d BY R%d", rn, rn, rx, ry); break;
					case 0x4c:		print("R%d = FDEP R%d BY R%d (SE)", rn, rx, ry); break;
					case 0x6c:		print("R%d = R%d OR FDEP R%d BY R%d (SE)", rn, rn, rx, ry); break;
					case 0x40:		print("R%d = FEXT R%d BY R%d", rn, rx, ry); break;
					case 0x48:		print("R%d = FEXT R%d BY R%d (SE)", rn, rx, ry); break;
					case 0x80:		print("R%d = EXP R%d", rn, rx); break;
					case 0x84:		print("R%d = EXP R%d (EX)", rn, rx); break;
					case 0x88:		print("R%d = LEFTZ R%d", rn, rx); break;
					case 0x8c:		print("R%d = LEFTO R%d", rn, rx); break;
					case 0x90:		print("R%d = FPACK F%d", rn, rx); break;
					case 0x94:		print("F%d = FUNPACK R%d", rn, rx); break;
					default:		print("??? (COMPUTE, SHIFT)"); break;
				}
			break;

			default:
				print("??? (COMPUTE)");
				break;
		}
	}
}

static void get_if_condition(int cond)
{
	if( cond != 31 ) {
		print("IF %s, ", condition_codes_if[cond]);
	}
}

void pm_dm_ureg(int g, int d, int i, int m, int ureg, int update)
{
	if (update)		// post-modify
	{
		if( d ) {
			if( g )
				print("PM(%s, %s) = %s", GET_DAG2_I(i), GET_DAG2_M(m), GET_UREG(ureg));
			else
				print("DM(%s, %s) = %s", GET_DAG1_I(i), GET_DAG1_M(m), GET_UREG(ureg));
		} else {
			if( g )
				print("%s = PM(%s, %s)", GET_UREG(ureg), GET_DAG2_I(i), GET_DAG2_M(m));
			else
				print("%s = DM(%s, %s)", GET_UREG(ureg), GET_DAG1_I(i), GET_DAG1_M(m));
		}

	}
	else			// pre-modify
	{
		if( d ) {
			if( g )
				print("PM(%s, %s) = %s", GET_DAG2_M(m), GET_DAG2_I(i), GET_UREG(ureg));
			else
				print("DM(%s, %s) = %s", GET_DAG1_M(m), GET_DAG1_I(i), GET_UREG(ureg));
		} else {
			if( g )
				print("%s = PM(%s, %s)", GET_UREG(ureg), GET_DAG2_M(m), GET_DAG2_I(i));
			else
				print("%s = DM(%s, %s)", GET_UREG(ureg), GET_DAG1_M(m), GET_DAG1_I(i));
		}
	}
}

void pm_dm_imm_dreg(int g, int d, int i, int data, int dreg, int update)
{
	if (update)		// post-modify
	{
		if( d ) {
			if( g )
				print("PM(%s, 0x%02X) = %s", GET_DAG2_I(i), data, GET_DREG(dreg));
			else
				print("DM(%s, 0x%02X) = %s", GET_DAG1_I(i), data, GET_DREG(dreg));
		} else {
			if( g )
				print("%s = PM(%s, 0x%02X)", GET_DREG(dreg), GET_DAG2_I(i), data);
			else
				print("%s = DM(%s, 0x%02X)", GET_DREG(dreg), GET_DAG1_I(i), data);
		}

	}
	else			// pre-modify
	{
		if( d ) {
			if( g )
				print("PM(0x%02X, %s) = %s", data, GET_DAG2_I(i), GET_DREG(dreg));
			else
				print("DM(0x%02X, %s) = %s", data, GET_DAG1_I(i), GET_DREG(dreg));
		} else {
			if( g )
				print("%s = PM(0x%02X, %s)", GET_DREG(dreg), data, GET_DAG2_I(i));
			else
				print("%s = DM(0x%02X, %s)", GET_DREG(dreg), data, GET_DAG1_I(i));
		}
	}
}

void pm_dm_dreg(int g, int d, int i, int m, int dreg)
{
	if( d ) {
		if( g )
			print("PM(%s, %s) = %s", GET_DAG2_I(i), GET_DAG2_M(m), GET_DREG(dreg));
		else
			print("DM(%s, %s) = %s", GET_DAG1_I(i), GET_DAG1_M(m), GET_DREG(dreg));
	} else {
		if( g )
			print("%s = PM(%s, %s)", GET_DREG(dreg), GET_DAG2_I(i), GET_DAG2_M(m));
		else
			print("%s = DM(%s, %s)", GET_DREG(dreg), GET_DAG1_I(i), GET_DAG1_M(m));
	}
}

void shiftop(int shift, int data, int rn, int rx)
{
	INT8 data8 = data & 0xff;
	int bit6 = data & 0x3f;
	int len = (data >> 6) & 0x3f;

	switch(shift)
	{
		case 0x00:		print("R%d = LSHIFT R%d BY %d", rn, rx, data8); break;
		case 0x08:		print("R%d = R%d OR LSHIFT R%d BY %d", rn, rn, rx, data8); break;
		case 0x01:		print("R%d = ASHIFT R%d BY %d", rn, rx, data8); break;
		case 0x09:		print("R%d = R%d OR ASHIFT R%d BY %d", rn, rn, rx, data8); break;
		case 0x02:		print("R%d = ROT R%d BY %d", rn, rx, data8); break;
		case 0x31:		print("R%d = BCLR R%d BY %d", rn, rx, data8); break;
		case 0x30:		print("R%d = BSET R%d BY %d", rn, rx, data8); break;
		case 0x32:		print("R%d = BTGL R%d BY %d", rn, rx, data8); break;
		case 0x33:		print("BTST R%d BY %d", rx, data8); break;
		case 0x11:		print("R%d = FDEP R%d BY %d:%d", rn, rx, bit6, len); break;
		case 0x19:		print("R%d = R%d OR FDEP R%d BY %d:%d", rn, rn, rx, bit6, len); break;
		case 0x13:		print("R%d = FDEP R%d BY %d:%d (SE)", rn, rx, bit6, len); break;
		case 0x1b:		print("R%d = R%d OR FDEP R%d BY %d:%d (SE)", rn, rn, rx, bit6, len); break;
		case 0x10:		print("R%d = FEXT R%d BY %d:%d", rn, rx, bit6, len); break;
		case 0x12:		print("R%d = FEXT R%d BY %d:%d (SE)", rn, rx, bit6, len); break;
		case 0x20:		print("R%d = EXP R%d", rn, rx); break;
		case 0x21:		print("R%d = EXP R%d (EX)", rn, rx); break;
		case 0x22:		print("R%d = LEFTZ R%d", rn, rx); break;
		case 0x23:		print("R%d = LEFTO R%d", rn, rx); break;
		case 0x24:		print("R%d = FPACK F%d", rn, rx); break;
		case 0x25:		print("F%d = FUNPACK R%d", rn, rx); break;
		default:		print("??? (SHIFTOP)"); break;
	}
}





static void dasm_compute_dreg_dmpm(UINT32 pc, UINT64 opcode)
{
	int dmi = (opcode >> 41) & 0x7;
	int dmm = (opcode >> 38) & 0x7;
	int pmi = (opcode >> 30) & 0x7;
	int pmm = (opcode >> 27) & 0x7;
	int dmdreg = (opcode >> 33) & 0xf;
	int pmdreg = (opcode >> 23) & 0xf;
	int comp = opcode & 0x7fffff;

	if( comp ) {
		compute(comp);
		print(",  ");
	}
	print("DM(%s, %s) = R%d, ", GET_DAG1_I(dmi), GET_DAG1_M(dmm), dmdreg);
	print("PM(%s, %s) = R%d", GET_DAG2_I(pmi), GET_DAG2_M(pmm), pmdreg);
}

static void dasm_compute(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 33) & 0x1f;
	int comp = opcode & 0x7fffff;

	if( comp ) {
		get_if_condition(cond);
		compute(comp);
	}
}

static void dasm_compute_uregdmpm_regmod(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 33) & 0x1f;
	int g = (opcode >> 32) & 0x1;
	int d = (opcode >> 31) & 0x1;
	int i = (opcode >> 41) & 0x7;
	int m = (opcode >> 38) & 0x7;
	int u = (opcode >> 44) & 0x1;
	int ureg = (opcode >> 23) & 0xff;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);
	if( comp ) {
		compute(comp);
		print(",  ");
	}
	pm_dm_ureg(g,d,i,m, ureg, u);
}

static void dasm_compute_dregdmpm_immmod(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 33) & 0x1f;
	int g = (opcode >> 40) & 0x1;
	int d = (opcode >> 39) & 0x1;
	int i = (opcode >> 41) & 0x7;
	int u = (opcode >> 38) & 0x1;
	int dreg = (opcode >> 23) & 0xf;
	int data = (opcode >> 27) & 0x3f;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);
	if( comp ) {
		compute(comp);
		print(",  ");
	}
	pm_dm_imm_dreg(g,d,i, data, dreg, u);
}

static void dasm_compute_ureg_ureg(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 31) & 0x1f;
	int uregs = (opcode >> 36) & 0xff;
	int uregd = (opcode >> 23) & 0xff;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);
	if( comp ) {
		compute(comp);
		print(",  ");
	}
	print("%s = %s", GET_UREG(uregd), GET_UREG(uregs));
}

static void dasm_immshift_dregdmpm(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 33) & 0x1f;
	int g = (opcode >> 32) & 0x1;
	int d = (opcode >> 31) & 0x1;
	int i = (opcode >> 41) & 0x7;
	int m = (opcode >> 38) & 0x7;
	int rn = (opcode >> 4) & 0xf;
	int rx = (opcode >> 0) & 0xf;
	int shift = (opcode >> 16) & 0x3f;
	int dreg = (opcode >> 23) & 0xf;
	int data = (((opcode >> 27) & 0xf) << 8) | ((opcode >> 8) & 0xff);

	get_if_condition(cond);
	shiftop(shift, data, rn, rx);
	print(",  ");
	pm_dm_dreg(g,d,i,m, dreg);
}

static void dasm_immshift_dregdmpm_nodata(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 33) & 0x1f;
	int rn = (opcode >> 4) & 0xf;
	int rx = (opcode >> 0) & 0xf;
	int shift = (opcode >> 16) & 0x3f;
	int data = (((opcode >> 27) & 0xf) << 8) | ((opcode >> 8) & 0xff);

	get_if_condition(cond);
	shiftop(shift, data, rn, rx);
}

static void dasm_compute_modify(UINT32 pc, UINT64 opcode)
{
	int cond = (opcode >> 33) & 0x1f;
	int g = (opcode >> 38) & 0x7;
	int i = (opcode >> 30) & 0x7;
	int m = (opcode >> 27) & 0x7;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);
	if( comp ) {
		compute(comp);
		print(",  ");
	}
	print("MODIFY(I%d, M%d)", (g ? 8+i : i), (g ? 8+m : m));
}

static void dasm_direct_jump(UINT32 pc, UINT64 opcode)
{
	int j = (opcode >> 26) & 0x1;
	int cond = (opcode >> 33) & 0x1f;
	int ci = (opcode >> 24) & 0x1;
	UINT32 addr = opcode & 0xffffff;

	get_if_condition(cond);
	if( opcode & U64(0x8000000000) )
		print("CALL");
	else
		print("JUMP");

	if( opcode & U64(0x10000000000) ) {		/* PC-relative branch */
		print(" (0x%08X)", pc + SIGN_EXTEND24(addr));
	} else {							/* Indirect branch */
		print(" (0x%08X)", addr);
	}
	if( j ) {
		print(" (DB)");
	}
	if (ci) {
		print(" (CI)");
	}

}

static void dasm_indirect_jump_compute(UINT32 pc, UINT64 opcode)
{
	int b = (opcode >> 39) & 0x1;
	int j = (opcode >> 26) & 0x1;
	int e = (opcode >> 25) & 0x1;
	int ci = (opcode >> 24) & 0x1;
	int cond = (opcode >> 33) & 0x1f;
	int pmi = (opcode >> 30) & 0x7;
	int pmm = (opcode >> 27) & 0x7;
	int reladdr = (opcode >> 27) & 0x3f;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);
	if( b )
		print("CALL");
	else
		print("JUMP");

	if( opcode & U64(0x10000000000) ) {	/* PC-relative branch */
		print(" (0x%08X)", pc + SIGN_EXTEND6(reladdr));
	} else {						/* Indirect branch */
		print(" (%s, %s)", GET_DAG2_M(pmm), GET_DAG2_I(pmi));
	}
	if( j ) {
		print(" (DB)");
	}
	if (ci) {
		print(" (CI)");
	}

	if( comp ) {
		print(", ");
		if( e )
			print("ELSE ");

		compute(comp);
	}
}

static void dasm_indirect_jump_compute_dregdm(UINT32 pc, UINT64 opcode)
{
	int d = (opcode >> 44) & 0x1;
	int cond = (opcode >> 33) & 0x1f;
	int pmi = (opcode >> 30) & 0x7;
	int pmm = (opcode >> 27) & 0x7;
	int dmi = (opcode >> 41) & 0x7;
	int dmm = (opcode >> 38) & 0x7;
	int reladdr = (opcode >> 27) & 0x3f;
	int dreg = (opcode >> 23) & 0xf;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);
	print("JUMP");

	if( opcode & U64(0x200000000000) ) {	/* PC-relative branch */
		print(" (0x%08X)", pc + SIGN_EXTEND6(reladdr));
	} else {						/* Indirect branch */
		print(" (%s, %s)", GET_DAG2_M(pmm), GET_DAG2_I(pmi));
	}
	print(", ELSE ");

	if( comp ) {
		compute(comp);
		print(",  ");
	}
	if( d )
		print("%s = DM(%s, %s)", GET_DREG(dreg), GET_DAG1_I(dmi), GET_DAG1_M(dmm));
	else
		print("DM(%s, %s) = %s", GET_DAG1_I(dmi), GET_DAG1_M(dmm), GET_DREG(dreg));
}

static void dasm_rts_compute(UINT32 pc, UINT64 opcode)
{
	int j = (opcode >> 26) & 0x1;
	int e = (opcode >> 25) & 0x1;
	int lr = (opcode >> 24) & 0x1;
	int cond = (opcode >> 33) & 0x1f;
	int comp = opcode & 0x7fffff;

	get_if_condition(cond);

	if( opcode & U64(0x10000000000) )
		print("RTI");
	else
		print("RTS");

	if( j )
		print(" (DB)");
	if( lr )
		print(" (LR)");

	if( comp ) {
		print(", ");
		if( e )
			print("ELSE ");

		compute(comp);
	}
}

static void dasm_do_until_counter(UINT32 pc, UINT64 opcode)
{
	int data = (opcode >> 24) & 0xffff;
	int ureg = (opcode >> 32) & 0xff;
	UINT32 addr = opcode & 0xffffff;

	if( opcode & U64(0x10000000000) ) {		/* Loop counter from universal register */
		print("LCNTR = %s, ", GET_UREG(ureg));
		print("DO (0x%08X)", pc + SIGN_EXTEND24(addr));
	} else {							/* Loop counter from immediate */
		print("LCNTR = 0x%04X, ", data);
		print("DO (0x%08X) UNTIL LCE", pc + SIGN_EXTEND24(addr));
	}
}

static void dasm_do_until(UINT32 pc, UINT64 opcode)
{
	int term = (opcode >> 33) & 0x1f;
	UINT32 addr = opcode & 0xffffff;

	print("DO (0x%08X) UNTIL %s", pc + SIGN_EXTEND24(addr), condition_codes_do[term]);
}

static void dasm_immmove_uregdmpm(UINT32 pc, UINT64 opcode)
{
	int d = (opcode >> 40) & 0x1;
	int g = (opcode >> 41) & 0x1;
	int ureg = (opcode >> 32) & 0xff;
	UINT32 addr = opcode & 0xffffffff;

	if( g ) {
		if( d )
			print("PM(0x%08X) = %s", addr, GET_UREG(ureg));
		else
			print("%s = PM(0x%08X)", GET_UREG(ureg), addr);

	} else {
		if( d )
			print("DM(0x%08X) = %s", addr, GET_UREG(ureg));
		else
			print("%s = DM(0x%08X)", GET_UREG(ureg), addr);
	}
}

static void dasm_immmove_uregdmpm_indirect(UINT32 pc, UINT64 opcode)
{
	int d = (opcode >> 40) & 0x1;
	int g = (opcode >> 44) & 0x1;
	int i = (opcode >> 41) & 0x7;
	int ureg = (opcode >> 32) & 0xff;
	UINT32 addr = opcode & 0xffffffff;

	if( g ) {
		if( d )
			print("PM(0x%08X, %s) = %s", addr, GET_DAG2_I(i), GET_UREG(ureg));
		else
			print("%s = PM(0x%08X, %s)", GET_UREG(ureg), addr, GET_DAG2_I(i));
	} else {
		if( d )
			print("DM(0x%08X, %s) = %s", addr, GET_DAG1_I(i), GET_UREG(ureg));
		else
			print("%s = DM(0x%08X, %s)", GET_UREG(ureg), addr, GET_DAG1_I(i));
	}
}

static void dasm_immmove_immdata_dmpm(UINT32 pc, UINT64 opcode)
{
	int g = (opcode >> 37) & 0x1;
	int i = (opcode >> 41) & 0x7;
	int m = (opcode >> 38) & 0x7;
	UINT32 data = opcode & 0xffffffff;

	if( g ) {
		print("PM(%s, %s) = 0x%08X", GET_DAG2_I(i), GET_DAG2_M(m), data);
	} else {
		print("DM(%s, %s) = 0x%08X", GET_DAG1_I(i), GET_DAG1_M(m), data);
	}
}

static void dasm_immmove_immdata_ureg(UINT32 pc, UINT64 opcode)
{
	int ureg = (opcode >> 32) & 0xff;
	UINT32 data = opcode & 0xffffffff;

	print("%s = %08X", GET_UREG(ureg), data);
}

static void dasm_sysreg_bitop(UINT32 pc, UINT64 opcode)
{
	int bop = (opcode >> 37) & 0x7;
	int sreg = (opcode >> 32) & 0xf;
	UINT32 data = opcode & 0xffffffff;

	print("BIT ");
	print("%s ", bopnames[bop]);
	print("%s ", GET_SREG(sreg));
	print("0x%08X", data);
}

static void dasm_ireg_modify(UINT32 pc, UINT64 opcode)
{
	int g = (opcode >> 38) & 0x1;
	int i = (opcode >> 32) & 0x7;
	UINT32 data = opcode & 0xffffffff;

	if( opcode & U64(0x8000000000) ) {	/* with bit-reverse */
		if( g )
			print("BITREV (%s, %08X)", GET_DAG2_I(i), data);
		else
			print("BITREV (%s, %08X)", GET_DAG1_I(i), data);
	} else {						/* without bit-reverse */
		if( g )
			print("MODIFY (%s, %08X)", GET_DAG2_I(i), data);
		else
			print("MODIFY (%s, %08X)", GET_DAG1_I(i), data);
	}
}

static void dasm_misc(UINT32 pc, UINT64 opcode)
{
	int bits = (opcode >> 33) & 0x7f;
	int lpu = (opcode >> 39) & 0x1;
	int lpo = (opcode >> 38) & 0x1;
	int spu = (opcode >> 37) & 0x1;
	int spo = (opcode >> 36) & 0x1;
	int ppu = (opcode >> 35) & 0x1;
	int ppo = (opcode >> 34) & 0x1;
	int fc = (opcode >> 33) & 0x1;

	if( lpu ) {
		print("PUSH LOOP");
		if( bits & 0x3f )
			print(", ");
	}
	if( lpo ) {
		print("POP LOOP");
		if( bits & 0x1f )
			print(", ");
	}
	if( spu ) {
		print("PUSH STS");
		if( bits & 0xf )
			print(", ");
	}
	if( spo ) {
		print("POP STS");
		if( bits & 0x7 )
			print(", ");
	}
	if( ppu ) {
		print("PUSH PCSTK");
		if( bits & 0x3 )
			print(", ");
	}
	if( ppo ) {
		print("POP PCSTK");
		if( bits & 0x1 )
			print(", ");
	}
	if( fc ) {
		print("FLUSH CACHE");
	}
}

static void dasm_idlenop(UINT32 pc, UINT64 opcode)
{
	if( opcode & U64(0x8000000000) )
		print("IDLE");
	else
		print("NOP");
}

static void dasm_cjump_rframe(UINT32 pc, UINT64 opcode)
{
	/* TODO */
	if( opcode & U64(0x10000000000) ) {	/* RFRAME */
		print("TODO: RFRAME");
	} else {
		print("TODO: CJUMP");
	}
}

static void dasm_invalid(UINT32 pc, UINT64 opcode)
{
	print("?");
}

#include "sharcdtb.c"


void sharc_dasm_one(char *buffer, offs_t pc, UINT64 opcode)
{
	int op = (opcode >> 40) & 0xff;

	/* set buffer for print */
	output = buffer;

	sharcdasm_table[op](pc, opcode);
}

