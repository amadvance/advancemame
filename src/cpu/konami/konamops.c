// license:BSD-3-Clause
// copyright-holders:Nathan Woods

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static void konami_execute_one(void)
{
	switch (pop_state())
	{
		case 0:	goto MAIN;
		case 1:	goto state_1;
		case 2:	goto LEA_xy;
		case 3:	goto LEA_us;
		case 4:	goto LD8;
		case 5:	goto ADD8;
		case 6:	goto ADC8;
		case 7:	goto SUB8;
		case 8:	goto SBC8;
		case 9:	goto AND8;
		case 10:	goto BIT8;
		case 11:	goto EOR8;
		case 12:	goto OR8;
		case 13:	goto CMP8;
		case 14:	goto SETLINE;
		case 15:	goto ST8;
		case 16:	goto LD16;
		case 17:	goto CMP16;
		case 18:	goto ADD16;
		case 19:	goto SUB16;
		case 20:	goto ST16;
		case 21:	goto CLR8;
		case 22:	goto COM8;
		case 23:	goto NEG8;
		case 24:	goto INC8;
		case 25:	goto DEC8;
		case 26:	goto TST8;
		case 27:	goto LSR8;
		case 28:	goto ROR8;
		case 29:	goto ASR8;
		case 30:	goto ASL8;
		case 31:	goto ROL8;
		case 32:	goto LSR16;
		case 33:	goto ROR16;
		case 34:	goto ASR16;
		case 35:	goto ASL16;
		case 36:	goto ROL16;
		case 37:	goto JMP;
		case 38:	goto JSR;
		case 39:	goto LSRD;
		case 40:	goto RORD;
		case 41:	goto ASRD;
		case 42:	goto ASLD;
		case 43:	goto ROLD;
		case 44:	goto CLR16;
		case 45:	goto NEG16;
		case 46:	goto INC16;
		case 47:	goto DEC16;
		case 48:	goto TST16;
		case 49:	goto state_49;
		case 50:	goto state_50;
		case 51:	goto state_51;
		case 52:	goto state_52;
		case 53:	goto state_53;
		case 54:	goto state_54;
		case 55:	goto state_55;
		case 56:	goto state_56;
		case 57:	goto state_57;
		case 58:	goto state_58;
		case 59:	goto state_59;
		case 60:	goto state_60;
		case 61:	goto state_61;
		case 62:	goto state_62;
		case 63:	goto state_63;
		case 64:	goto state_64;
		case 65:	goto state_65;
		case 66:	goto state_66;
		case 67:	goto state_67;
		case 68:	goto state_68;
		case 69:	goto state_69;
		case 70:	goto state_70;
		case 71:	goto state_71;
		case 72:	goto state_72;
		case 73:	goto state_73;
		case 74:	goto state_74;
		case 75:	goto state_75;
		case 76:	goto state_76;
		case 77:	goto state_77;
		case 78:	goto state_78;
		case 79:	goto state_79;
		case 80:	goto state_80;
		case 81:	goto state_81;
		case 82:	goto state_82;
		case 83:	goto state_83;
		case 84:	goto state_84;
		case 85:	goto state_85;
		case 86:	goto state_86;
		case 87:	goto state_87;
		case 88:	goto state_88;
		case 89:	goto state_89;
		case 90:	goto state_90;
		case 91:	goto state_91;
		case 92:	goto state_92;
		case 93:	goto state_93;
		case 94:	goto state_94;
		case 95:	goto state_95;
		case 96:	goto state_96;
		case 97:	goto state_97;
		case 98:	goto state_98;
		case 99:	goto state_99;
		case 100:	goto state_100;
		case 101:	goto state_101;
		case 102:	goto state_102;
		case 103:	goto state_103;
		case 104:	goto state_104;
		case 105:	goto state_105;
		case 106:	goto state_106;
		case 107:	goto state_107;
		case 108:	goto state_108;
		case 109:	goto state_109;
		case 110:	goto state_110;
		case 111:	goto state_111;
		case 112:	goto state_112;
		case 113:	goto state_113;
		case 114:	goto state_114;
		case 115:	goto state_115;
		case 116:	goto state_116;
		case 117:	goto state_117;
		case 118:	goto state_118;
		case 119:	goto state_119;
		case 120:	goto state_120;
		case 121:	goto state_121;
		case 122:	goto state_122;
		case 123:	goto state_123;
		case 124:	goto state_124;
		case 125:	goto state_125;
		case 126:	goto state_126;
		case 127:	goto state_127;
		case 128:	goto state_128;
		case 129:	goto state_129;
		case 130:	goto state_130;
		case 131:	goto state_131;
		case 132:	goto state_132;
		case 133:	goto state_133;
		case 134:	goto state_134;
		case 135:	goto state_135;
		case 136:	goto state_136;
		case 137:	goto state_137;
		case 138:	goto state_138;
		case 139:	goto state_139;
		case 140:	goto state_140;
		case 141:	goto state_141;
		case 142:	goto state_142;
		case 143:	goto state_143;
		case 144:	goto state_144;
		case 145:	goto state_145;
		case 146:	goto state_146;
		case 147:	goto state_147;
		case 148:	goto state_148;
		case 149:	goto state_149;
		case 150:	goto state_150;
		case 151:	goto state_151;
		case 152:	goto state_152;
		case 153:	goto state_153;
		case 154:	goto state_154;
		case 155:	goto state_155;
		case 156:	goto state_156;
		case 157:	goto state_157;
		case 158:	goto state_158;
		case 159:	goto state_159;
		case 160:	goto state_160;
		case 161:	goto state_161;
		case 162:	goto state_162;
		case 163:	goto state_163;
		case 164:	goto state_164;
		case 165:	goto state_165;
		case 166:	goto state_166;
		case 167:	goto state_167;
		case 168:	goto state_168;
		case 169:	goto state_169;
		case 170:	goto state_170;
		case 171:	goto state_171;
		case 172:	goto state_172;
		case 173:	goto state_173;
		case 174:	goto state_174;
		case 175:	goto state_175;
		case 176:	goto state_176;
		case 177:	goto state_177;
		case 178:	goto state_178;
		case 179:	goto state_179;
		case 180:	goto state_180;
		case 181:	goto state_181;
		case 182:	goto state_182;
		case 183:	goto state_183;
		case 184:	goto state_184;
		case 185:	goto state_185;
		case 186:	goto state_186;
		case 187:	goto state_187;
		case 188:	goto state_188;
		case 189:	goto state_189;
		case 190:	goto state_190;
		case 191:	goto state_191;
		case 192:	goto state_192;
		case 193:	goto state_193;
		default:
			fatalerror("Unexpected state");
			break;

// license:BSD-3-Clause
// copyright-holders:Nathan Woods
MAIN:
	// check interrupt lines
	switch(get_pending_interrupt())
	{
		case VECTOR_NMI:	goto NMI;
		case VECTOR_FIRQ:	goto FIRQ;
		case VECTOR_IRQ:	goto IRQ;
	}

	// debugger hook
	m_ppc = m_pc;
	change_pc(m_pc.w);
	CALL_MAME_DEBUG;

	// opcode fetch
	m_opcode = read_opcode_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(1); return; }
state_1:

	// dispatch opcode
	switch(m_opcode)
	{
		case 0x08:				set_regop16(m_x);		push_state(2);	// LEA_xy
		goto INDEXED;
		case 0x09:				set_regop16(m_y);		push_state(2);	// LEA_xy
		goto INDEXED;
		case 0x0A:				set_regop16(m_u);		push_state(3);	// LEA_us
		goto INDEXED;
		case 0x0B:				set_regop16(m_s);		push_state(3);	// LEA_us
		goto INDEXED;
		case 0x0C:
			goto PSHS;
		case 0x0D:
			goto PSHU;
		case 0x0E:
			goto PULS;
		case 0x0F:
			goto PULU;

		case 0x10:				set_regop8(m_d.b.h);	set_imm();	goto LD8;
		case 0x11:				set_regop8(m_d.b.l);	set_imm();	goto LD8;
		case 0x12:				set_regop8(m_d.b.h);	push_state(4);	// LD8
	goto INDEXED;
		case 0x13:				set_regop8(m_d.b.l);	push_state(4);	// LD8
	goto INDEXED;
		case 0x14:				set_regop8(m_d.b.h);	set_imm();	goto ADD8;
		case 0x15:				set_regop8(m_d.b.l);	set_imm();	goto ADD8;
		case 0x16:				set_regop8(m_d.b.h);	push_state(5);	// ADD8
	goto INDEXED;
		case 0x17:				set_regop8(m_d.b.l);	push_state(5);	// ADD8
	goto INDEXED;
		case 0x18:				set_regop8(m_d.b.h);	set_imm();	goto ADC8;
		case 0x19:				set_regop8(m_d.b.l);	set_imm();	goto ADC8;
		case 0x1A:				set_regop8(m_d.b.h);	push_state(6);	// ADC8
	goto INDEXED;
		case 0x1B:				set_regop8(m_d.b.l);	push_state(6);	// ADC8
	goto INDEXED;
		case 0x1C:				set_regop8(m_d.b.h);	set_imm();	goto SUB8;
		case 0x1D:				set_regop8(m_d.b.l);	set_imm();	goto SUB8;
		case 0x1E:				set_regop8(m_d.b.h);	push_state(7);	// SUB8
	goto INDEXED;
		case 0x1F:				set_regop8(m_d.b.l);	push_state(7);	// SUB8
	goto INDEXED;

		case 0x20:				set_regop8(m_d.b.h);	set_imm();	goto SBC8;
		case 0x21:				set_regop8(m_d.b.l);	set_imm();	goto SBC8;
		case 0x22:				set_regop8(m_d.b.h);	push_state(8);	// SBC8
	goto INDEXED;
		case 0x23:				set_regop8(m_d.b.l);	push_state(8);	// SBC8
	goto INDEXED;
		case 0x24:				set_regop8(m_d.b.h);	set_imm();	goto AND8;
		case 0x25:				set_regop8(m_d.b.l);	set_imm();	goto AND8;
		case 0x26:				set_regop8(m_d.b.h);	push_state(9);	// AND8
	goto INDEXED;
		case 0x27:				set_regop8(m_d.b.l);	push_state(9);	// AND8
	goto INDEXED;
		case 0x28:				set_regop8(m_d.b.h);	set_imm();	goto BIT8;
		case 0x29:				set_regop8(m_d.b.l);	set_imm();	goto BIT8;
		case 0x2A:				set_regop8(m_d.b.h);	push_state(10);	// BIT8
	goto INDEXED;
		case 0x2B:				set_regop8(m_d.b.l);	push_state(10);	// BIT8
	goto INDEXED;
		case 0x2C:				set_regop8(m_d.b.h);	set_imm();	goto EOR8;
		case 0x2D:				set_regop8(m_d.b.l);	set_imm();	goto EOR8;
		case 0x2E:				set_regop8(m_d.b.h);	push_state(11);	// EOR8
	goto INDEXED;
		case 0x2F:				set_regop8(m_d.b.l);	push_state(11);	// EOR8
	goto INDEXED;

		case 0x30:				set_regop8(m_d.b.h);	set_imm();	goto OR8;
		case 0x31:				set_regop8(m_d.b.l);	set_imm();	goto OR8;
		case 0x32:				set_regop8(m_d.b.h);	push_state(12);	// OR8
	goto INDEXED;
		case 0x33:				set_regop8(m_d.b.l);	push_state(12);	// OR8
	goto INDEXED;
		case 0x34:				set_regop8(m_d.b.h);	set_imm();	goto CMP8;
		case 0x35:				set_regop8(m_d.b.l);	set_imm();	goto CMP8;
		case 0x36:				set_regop8(m_d.b.h);	push_state(13);	// CMP8
	goto INDEXED;
		case 0x37:				set_regop8(m_d.b.l);	push_state(13);	// CMP8
	goto INDEXED;
		case 0x38:										set_imm();	goto SETLINE;
		case 0x39:
			push_state(14);	// SETLINE
			goto INDEXED;
		case 0x3A:				set_regop8(m_d.b.h);	push_state(15);	// ST8
	goto INDEXED;
		case 0x3B:				set_regop8(m_d.b.l);	push_state(15);	// ST8
	goto INDEXED;
		case 0x3C:				set_imm();							goto ANDCC;
		case 0x3D:				set_imm();							goto ORCC;
		case 0x3E:
			goto EXG;
		case 0x3F:
			goto TFR;

		case 0x40:				set_regop16(m_d);		set_imm();	goto LD16;
		case 0x41:				set_regop16(m_d);		push_state(16);	// LD16
		goto INDEXED;
		case 0x42:				set_regop16(m_x);		set_imm();	goto LD16;
		case 0x43:				set_regop16(m_x);		push_state(16);	// LD16
		goto INDEXED;
		case 0x44:				set_regop16(m_y);		set_imm();	goto LD16;
		case 0x45:				set_regop16(m_y);		push_state(16);	// LD16
		goto INDEXED;
		case 0x46:				set_regop16(m_u);		set_imm();	goto LD16;
		case 0x47:				set_regop16(m_u);		push_state(16);	// LD16
		goto INDEXED;
		case 0x48:				set_regop16(m_s);		set_imm();	goto LD16;
		case 0x49:				set_regop16(m_s);		push_state(16);	// LD16
		goto INDEXED;
		case 0x4A:				set_regop16(m_d);		set_imm();	goto CMP16;
		case 0x4B:				set_regop16(m_d);		push_state(17);	// CMP16
		goto INDEXED;
		case 0x4C:				set_regop16(m_x);		set_imm();	goto CMP16;
		case 0x4D:				set_regop16(m_x);		push_state(17);	// CMP16
		goto INDEXED;
		case 0x4E:				set_regop16(m_y);		set_imm();	goto CMP16;
		case 0x4F:				set_regop16(m_y);		push_state(17);	// CMP16
		goto INDEXED;

		case 0x50:				set_regop16(m_u);		set_imm();	goto CMP16;
		case 0x51:				set_regop16(m_u);		push_state(17);	// CMP16
		goto INDEXED;
		case 0x52:				set_regop16(m_s);		set_imm();	goto CMP16;
		case 0x53:				set_regop16(m_s);		push_state(17);	// CMP16
		goto INDEXED;
		case 0x54:				set_regop16(m_d);		set_imm();	goto ADD16;
		case 0x55:				set_regop16(m_d);		push_state(18);	// ADD16
		goto INDEXED;
		case 0x56:				set_regop16(m_d);		set_imm();	goto SUB16;
		case 0x57:				set_regop16(m_d);		push_state(19);	// SUB16
		goto INDEXED;
		case 0x58:				set_regop16(m_d);		push_state(20);	// ST16
		goto INDEXED;
		case 0x59:				set_regop16(m_x);		push_state(20);	// ST16
		goto INDEXED;
		case 0x5A:				set_regop16(m_y);		push_state(20);	// ST16
		goto INDEXED;
		case 0x5B:				set_regop16(m_u);		push_state(20);	// ST16
		goto INDEXED;
		case 0x5C:				set_regop16(m_s);		push_state(20);	// ST16
		goto INDEXED;

		case 0x60:				set_cond(true);						goto BRANCH;
		case 0x61:				set_cond(cond_hi());				goto BRANCH;
		case 0x62:				set_cond(cond_cc());				goto BRANCH;
		case 0x63:				set_cond(cond_ne());				goto BRANCH;
		case 0x64:				set_cond(cond_vc());				goto BRANCH;
		case 0x65:				set_cond(cond_pl());				goto BRANCH;
		case 0x66:				set_cond(cond_ge());				goto BRANCH;
		case 0x67:				set_cond(cond_gt());				goto BRANCH;
		case 0x68:				set_cond(true);						goto LBRANCH;
		case 0x69:				set_cond(cond_hi());				goto LBRANCH;
		case 0x6A:				set_cond(cond_cc());				goto LBRANCH;
		case 0x6B:				set_cond(cond_ne());				goto LBRANCH;
		case 0x6C:				set_cond(cond_vc());				goto LBRANCH;
		case 0x6D:				set_cond(cond_pl());				goto LBRANCH;
		case 0x6E:				set_cond(cond_ge());				goto LBRANCH;
		case 0x6F:				set_cond(cond_gt());				goto LBRANCH;

		case 0x70:				set_cond(false);					goto BRANCH;
		case 0x71:				set_cond(!cond_hi());				goto BRANCH;
		case 0x72:				set_cond(!cond_cc());				goto BRANCH;
		case 0x73:				set_cond(!cond_ne());				goto BRANCH;
		case 0x74:				set_cond(!cond_vc());				goto BRANCH;
		case 0x75:				set_cond(!cond_pl());				goto BRANCH;
		case 0x76:				set_cond(!cond_ge());				goto BRANCH;
		case 0x77:				set_cond(!cond_gt());				goto BRANCH;
		case 0x78:				set_cond(false);					goto LBRANCH;
		case 0x79:				set_cond(!cond_hi());				goto LBRANCH;
		case 0x7A:				set_cond(!cond_cc());				goto LBRANCH;
		case 0x7B:				set_cond(!cond_ne());				goto LBRANCH;
		case 0x7C:				set_cond(!cond_vc());				goto LBRANCH;
		case 0x7D:				set_cond(!cond_pl());				goto LBRANCH;
		case 0x7E:				set_cond(!cond_ge());				goto LBRANCH;
		case 0x7F:				set_cond(!cond_gt());				goto LBRANCH;

		case 0x80:										set_a();	goto CLR8;
		case 0x81:										set_b();	goto CLR8;
		case 0x82:
			push_state(21);	// CLR8
			goto INDEXED;
		case 0x83:										set_a();	goto COM8;
		case 0x84:										set_b();	goto COM8;
		case 0x85:
			push_state(22);	// COM8
			goto INDEXED;
		case 0x86:										set_a();	goto NEG8;
		case 0x87:										set_b();	goto NEG8;
		case 0x88:
			push_state(23);	// NEG8
			goto INDEXED;
		case 0x89:										set_a();	goto INC8;
		case 0x8A:										set_b();	goto INC8;
		case 0x8B:
			push_state(24);	// INC8
			goto INDEXED;
		case 0x8C:										set_a();	goto DEC8;
		case 0x8D:										set_b();	goto DEC8;
		case 0x8E:
			push_state(25);	// DEC8
			goto INDEXED;
		case 0x8F:
			goto RTS;

		case 0x90:										set_a();	goto TST8;
		case 0x91:										set_b();	goto TST8;
		case 0x92:
			push_state(26);	// TST8
			goto INDEXED;
		case 0x93:										set_a();	goto LSR8;
		case 0x94:										set_b();	goto LSR8;
		case 0x95:
			push_state(27);	// LSR8
			goto INDEXED;
		case 0x96:										set_a();	goto ROR8;
		case 0x97:										set_b();	goto ROR8;
		case 0x98:
			push_state(28);	// ROR8
			goto INDEXED;
		case 0x99:										set_a();	goto ASR8;
		case 0x9A:										set_b();	goto ASR8;
		case 0x9B:
			push_state(29);	// ASR8
			goto INDEXED;
		case 0x9C:										set_a();	goto ASL8;
		case 0x9D:										set_b();	goto ASL8;
		case 0x9E:
			push_state(30);	// ASL8
			goto INDEXED;
		case 0x9F:
			goto RTI;

		case 0xA0:										set_a();	goto ROL8;
		case 0xA1:										set_b();	goto ROL8;
		case 0xA2:
			push_state(31);	// ROL8
			goto INDEXED;
		case 0xA3:
			push_state(32);	// LSR16
			goto INDEXED;
		case 0xA4:
			push_state(33);	// ROR16
			goto INDEXED;
		case 0xA5:
			push_state(34);	// ASR16
			goto INDEXED;
		case 0xA6:
			push_state(35);	// ASL16
			goto INDEXED;
		case 0xA7:
			push_state(36);	// ROL16
			goto INDEXED;
		case 0xA8:
			push_state(37);	// JMP
			goto INDEXED;
		case 0xA9:
			push_state(38);	// JSR
			goto INDEXED;
		case 0xAA:
			goto BSR;
		case 0xAB:
			goto LBSR;
		case 0xAC:
			goto DECBJNZ;
		case 0xAD:
			goto DECXJNZ;
		case 0xAE:
			goto NOP;

		case 0xB0:
			goto ABX;
		case 0xB1:
			goto DAA;
		case 0xB2:
			goto SEX;
		case 0xB3:
			goto MUL;
		case 0xB4:
			goto LMUL;
		case 0xB5:
			goto DIVX;
		case 0xB6:
			goto BMOVE;
		case 0xB7:
			goto MOVE;
		case 0xB8:										set_imm();	goto LSRD;
		case 0xB9:
			push_state(39);	// LSRD
			goto INDEXED;
		case 0xBA:										set_imm();	goto RORD;
		case 0xBB:
			push_state(40);	// RORD
			goto INDEXED;
		case 0xBC:										set_imm();	goto ASRD;
		case 0xBD:
			push_state(41);	// ASRD
			goto INDEXED;
		case 0xBE:										set_imm();	goto ASLD;
		case 0xBF:
			push_state(42);	// ASLD
			goto INDEXED;

		case 0xC0:										set_imm();	goto ROLD;
		case 0xC1:
			push_state(43);	// ROLD
			goto INDEXED;
		case 0xC2:										set_d();	goto CLR16;
		case 0xC3:
			push_state(44);	// CLR16
			goto INDEXED;
		case 0xC4:										set_d();	goto NEG16;
		case 0xC5:
			push_state(45);	// NEG16
			goto INDEXED;
		case 0xC6:										set_d();	goto INC16;
		case 0xC7:
			push_state(46);	// INC16
			goto INDEXED;
		case 0xC8:										set_d();	goto DEC16;
		case 0xC9:
			push_state(47);	// DEC16
			goto INDEXED;
		case 0xCA:										set_d();	goto TST16;
		case 0xCB:
			push_state(48);	// TST16
			goto INDEXED;
		case 0xCC:										set_a();	goto ABS8;
		case 0xCD:										set_b();	goto ABS8;
		case 0xCE:										set_d();	goto ABS16;
		case 0xCF:
			goto BSET;

		case 0xD0:
			goto BSET2;
		default:
			goto ILLEGAL;
	}
	return;

// license:BSD-3-Clause
// copyright-holders:Nathan Woods
NMI:
	m_nmi_asserted = false;
	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(49);
	goto PUSH_REGISTERS;
state_49:
	m_cc |= CC_I | CC_F;
	set_ea(VECTOR_NMI);
	eat(1);
	standard_irq_callback(INPUT_LINE_NMI);
	goto INTERRUPT_VECTOR;

FIRQ:
	if (firq_saves_entire_state())
	{
		m_cc |= CC_E;
		m_temp.w = entire_state_registers();
	}
	else
	{
		m_cc &= ~CC_E;
		m_temp.w = partial_state_registers();
	}
	set_regop16(m_s);
	push_state(50);
	goto PUSH_REGISTERS;
state_50:
	m_cc |= CC_I | CC_F;
	set_ea(VECTOR_FIRQ);
	eat(1);
	standard_irq_callback(M6809_FIRQ_LINE);
	goto INTERRUPT_VECTOR;

IRQ:
	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(51);
	goto PUSH_REGISTERS;
state_51:
	m_cc |= CC_I;
	set_ea(VECTOR_IRQ);
	eat(1);
	standard_irq_callback(M6809_IRQ_LINE);
	goto INTERRUPT_VECTOR;

INTERRUPT_VECTOR:
	eat(4);
	if (UNEXPECTED(m_icount <= 0)) { push_state(52); return; }
state_52:
	m_pc.b.h = read_operand_ord(0);	if (UNEXPECTED(m_icount <= 0)) { push_state(53); return; }
state_53:
	// Not sure if this is cycle exact
	m_pc.b.l = read_operand_ord(1);	if (UNEXPECTED(m_icount <= 0)) { push_state(54); return; }
state_54:
	// Not sure if this is cycle exact
	change_pc(m_pc.w);
	return;

NEG8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(55); return; }
state_55:
	m_temp.b.l = set_flags(CC_NZVC, (UINT8)0, m_temp.b.l, -m_temp.b.l);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(56); return; }
state_56:
	write_operand(m_temp.b.l);
	return;

COM8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(57); return; }
state_57:
	m_cc &= ~CC_V;
	m_cc |= CC_C;
	m_temp.b.l = set_flags(CC_NZ, (UINT8) ~m_temp.b.l);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(58); return; }
state_58:
	write_operand(m_temp.b.l);
	return;

LSR8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(59); return; }
state_59:
	m_cc &= ~CC_C;
	m_cc |= (m_temp.b.l & 1) ? CC_C : 0;
	m_temp.b.l = set_flags8(CC_NZ, m_temp.b.l >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(60); return; }
state_60:
	write_operand(m_temp.b.l);
	return;

ROR8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(61); return; }
state_61:
	m_temp.b.l = set_flags8(CC_NZ, rotate_right(m_temp.b.l));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(62); return; }
state_62:
	write_operand(m_temp.b.l);
	return;

ASR8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(63); return; }
state_63:
	m_cc &= ~CC_NZC;
	m_cc |= (m_temp.b.l & 1) ? CC_C : 0;
	m_temp.b.l = set_flags8(CC_NZ, ((INT8) m_temp.b.l) >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(64); return; }
state_64:
	write_operand(m_temp.b.l);
	return;

ASL8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(65); return; }
state_65:
	m_temp.b.l = set_flags8(CC_NZVC, m_temp.b.l, m_temp.b.l, m_temp.b.l << 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(66); return; }
state_66:
	write_operand(m_temp.b.l);
	return;

ROL8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(67); return; }
state_67:
	m_temp.b.l = set_flags8(CC_NZV, m_temp.b.l, m_temp.b.l, rotate_left(m_temp.b.l));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(68); return; }
state_68:
	write_operand(m_temp.b.l);
	return;

DEC8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(69); return; }
state_69:
	m_temp.b.l = set_flags8(CC_NZV, m_temp.b.l, 1, m_temp.b.l - 1);
	eat(hd6309_native_mode() && is_register_addressing_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(70); return; }
state_70:
	write_operand(m_temp.b.l);
	return;

INC8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(71); return; }
state_71:
	m_temp.b.l = set_flags8(CC_NZV, m_temp.b.l, 1, m_temp.b.l + 1);
	eat(hd6309_native_mode() && is_register_addressing_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(72); return; }
state_72:
	write_operand(m_temp.b.l);
	return;

TST8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(73); return; }
state_73:
	set_flags(CC_NZV, m_temp.b.l);
	eat(hd6309_native_mode() ? 0 : 1);
	eat(is_register_addressing_mode() ? 0 : 1);
	return;

JMP:
	m_pc.w = m_ea.w;
	change_pc(m_pc.w);
	return;

CLR8:
	read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(74); return; }
state_74:
	m_cc &= ~CC_NZVC;
	m_cc |= CC_Z;
	eat(hd6309_native_mode() && is_register_addressing_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(75); return; }
state_75:
	write_operand(0);
	return;

NEG16:
	m_temp.b.h = read_operand_ord(0);
	m_temp.b.l = read_operand_ord(1);
	m_temp.w = set_flags(CC_NZVC, (UINT16)0, m_temp.w, -m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	write_operand_ord(0, m_temp.b.h);
	write_operand_ord(1, m_temp.b.l);
	return;

LSR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(76); return; }
state_76:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(77); return; }
state_77:
	m_cc &= ~CC_C;
	m_cc |= (m_temp.w & 1) ? CC_C : 0;
	m_temp.w = set_flags16(CC_NZ, m_temp.w >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(78); return; }
state_78:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(79); return; }
state_79:
	write_operand_ord(1, m_temp.b.l);
	return;

ROR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(80); return; }
state_80:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(81); return; }
state_81:
	m_temp.w = set_flags16(CC_NZ, rotate_right(m_temp.w));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(82); return; }
state_82:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(83); return; }
state_83:
	write_operand_ord(1, m_temp.b.l);
	return;

ASR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(84); return; }
state_84:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(85); return; }
state_85:
	m_cc &= ~CC_NZC;
	m_cc |= (m_temp.w & 1) ? CC_C : 0;
	m_temp.w = set_flags16(CC_NZ, ((INT16) m_temp.w) >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(86); return; }
state_86:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(87); return; }
state_87:
	write_operand_ord(1, m_temp.b.l);
	return;

ASL16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(88); return; }
state_88:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(89); return; }
state_89:
	m_temp.w = set_flags16(CC_NZVC, m_temp.w, m_temp.w, m_temp.w << 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(90); return; }
state_90:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(91); return; }
state_91:
	write_operand_ord(1, m_temp.b.l);
	return;

ROL16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(92); return; }
state_92:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(93); return; }
state_93:
	m_temp.w = set_flags16(CC_NZV, rotate_left(m_temp.w));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(94); return; }
state_94:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(95); return; }
state_95:
	write_operand_ord(1, m_temp.b.l);
	return;

DEC16:
	m_temp.b.h = read_operand_ord(0);
	m_temp.b.l = read_operand_ord(1);
	m_temp.w = set_flags16(CC_NZVC, m_temp.w, 1, m_temp.w - 1);
	eat(hd6309_native_mode() ? 0 : 1);
	write_operand_ord(0, m_temp.b.h);
	write_operand_ord(1, m_temp.b.l);
	return;

INC16:
	m_temp.b.h = read_operand_ord(0);
	m_temp.b.l = read_operand_ord(1);
	m_temp.w = set_flags16(CC_NZVC, m_temp.w, 1, m_temp.w + 1);
	eat(hd6309_native_mode() ? 0 : 1);
	write_operand_ord(0, m_temp.b.h);
	write_operand_ord(1, m_temp.b.l);
	return;

TST16:
	m_temp.b.h = read_operand_ord(0);
	m_temp.b.l = read_operand_ord(1);
	set_flags(CC_NZV, m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	eat(is_register_addressing_mode() ? 0 : 1);
	return;

CLR16:
	eat(hd6309_native_mode() ? 0 : 1);
	m_cc &= ~CC_NZVC;
	m_cc |= CC_Z;
	write_operand_ord(0, 0x00);
	write_operand_ord(1, 0x00);
	return;

SUB8:
	m_temp.b.l = read_operand();
	regop8() = set_flags(CC_NZVC, regop8(), m_temp.b.l, regop8() - m_temp.b.l);
	return;

CMP8:
	m_temp.b.l = read_operand();
	set_flags(CC_NZVC, regop8(), m_temp.b.l, regop8() - m_temp.b.l);
	return;

SBC8:
	m_temp.w = (UINT16)read_operand() + (m_cc & CC_C ? 1 : 0);
	regop8() = set_flags(CC_NZVC, regop8(), m_temp.b.l, regop8() - m_temp.w);
	return;

AND8:
	m_cc &= ~CC_V;
	regop8() = set_flags(CC_NZ, (UINT8)0, regop8(), regop8() & read_operand());
	return;

BIT8:
	m_cc &= ~CC_V;
	set_flags(CC_NZ, (UINT8)0, regop8(), regop8() & read_operand());
	return;

EOR8:
	m_cc &= ~CC_V;
	regop8() = set_flags(CC_NZ, (UINT8)0, regop8(), regop8() ^ read_operand());
	return;

ADC8:
	m_temp.w = (UINT16)read_operand() + (m_cc & CC_C ? 1 : 0);
	regop8() = set_flags(add8_sets_h() ? CC_HNZVC : CC_NZVC, regop8(), m_temp.b.l, regop8() + m_temp.w);
	return;

OR8:
	m_cc &= ~CC_V;
	regop8() = set_flags(CC_NZ, (UINT8)0, regop8(), regop8() | read_operand());
	return;

ADD8:
	m_temp.b.l = read_operand();
	regop8() = set_flags(add8_sets_h() ? CC_HNZVC : CC_NZVC, regop8(), m_temp.b.l, regop8() + m_temp.b.l);
	return;

ADD16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(96); return; }
state_96:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(97); return; }
state_97:
	regop16().w = set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w + m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

SUB16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(98); return; }
state_98:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(99); return; }
state_99:
	regop16().w = set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w - m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

CMP16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(100); return; }
state_100:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(101); return; }
state_101:
	set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w - m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

LD8:
	regop8() = read_operand();
	set_flags(CC_NZV, regop8());
	return;

LD16:
	regop16().b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(102); return; }
state_102:
	regop16().b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(103); return; }
state_103:
	set_flags(CC_NZV, regop16().w);
	if (&regop16() == &m_s)
		m_lds_encountered = true;
	return;

ST8:
	write_ea(set_flags(CC_NZV, regop8()));
	return;

ST16:
	write_operand_ord(0, regop16().b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(104); return; }
state_104:
	write_operand_ord(1, regop16().b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(105); return; }
state_105:
	set_flags(CC_NZV, regop16().w);
	return;

NOP:
	eat(hd6309_native_mode() ? 0 : 1);
	return;

SYNC:
	// SYNC stops processing instructions until an interrupt request happens.
	// This doesn't require the corresponding interrupt to be enabled: if it
	// is disabled, execution continues with the next instruction.
	eat(3);

	while(!m_nmi_asserted && !m_firq_line && !m_irq_line)
	{
		// massaging the PC this way makes the debugger's behavior more
		// intuitive
		m_pc.w--;
		
		eat_remaining();
		if (UNEXPECTED(m_icount <= 0)) { push_state(106); return; }
state_106:
		
		// unmassage...
		m_pc.w++;
	}
	return;

DAA:
	daa();
	eat(hd6309_native_mode() ? 0 : 1);
	return;

ORCC:
	m_cc |= read_operand();
	eat(hd6309_native_mode() ? 0 : 1);
	return;

ANDCC:
	m_cc &= read_operand();
	eat(1);
	return;

SEX:
	m_d.w = set_flags16(CC_NZ, (INT8) m_d.b.l);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

BRANCH:
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(107); return; }
state_107:
	eat(1);
	if (branch_taken())
	{
		m_pc.w += (INT8) m_temp.b.l;
		change_pc(m_pc.w);
	}
	return;

LBRANCH:
	m_temp.b.h = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(108); return; }
state_108:
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(109); return; }
state_109:
	eat(1);
	if (branch_taken())
	{
		m_pc.w += m_temp.w;
		change_pc(m_pc.w);
		eat(hd6309_native_mode() ? 0 : 1);
	}
	return;

BSR:
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(110); return; }
state_110:
	m_ea.w = m_pc.w + (INT8) m_temp.b.l;
	eat(hd6309_native_mode() ? 2 : 3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(111); return; }
state_111:
	goto GOTO_SUBROUTINE;

LBSR:
	m_temp.b.h = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(112); return; }
state_112:
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(113); return; }
state_113:
	m_ea.w = m_pc.w + (INT16) m_temp.w;
	eat(hd6309_native_mode() ? 2 : 4);
	if (UNEXPECTED(m_icount <= 0)) { push_state(114); return; }
state_114:
	goto GOTO_SUBROUTINE;

JSR:
	eat(2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(115); return; }
state_115:
	goto GOTO_SUBROUTINE;
	
GOTO_SUBROUTINE:
	write_memory(--m_s.w, m_pc.b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(116); return; }
state_116:
	write_memory(--m_s.w, m_pc.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(117); return; }
state_117:
	m_pc = m_ea;
	change_pc(m_pc.w);
	return;

RTS:
	m_temp.w = 0x80;	// RTS is equivalent to "PULS PC"
	eat(hd6309_native_mode() ? 0 : 1);
	set_regop16(m_s);
	goto PULL_REGISTERS;

ABX:
	m_x.w += m_d.b.l;
	eat(hd6309_native_mode() ? 0 : 2);
	return;

MUL:
	mul();
	eat(hd6309_native_mode() ? 9 : 10);
	return;

RTI:
	set_regop16(m_s);
	m_temp.w = 0x01;	// PULS CC
	push_state(118);
	goto PULL_REGISTERS;
state_118:
	m_temp.w = ((m_cc & CC_E) ? entire_state_registers() : partial_state_registers()) & ~0x01;
	goto PULL_REGISTERS;

CWAI:
	m_cc &= read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(119); return; }
state_119:
	eat(2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(120); return; }
state_120:

	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(121);
	goto PUSH_REGISTERS;
state_121:

	while((m_ea.w = get_pending_interrupt()) == 0)
	{
		// massaging the PC this way makes the debugger's behavior more
		// intuitive
		m_pc.w -= 2;

		eat_remaining();
		if (UNEXPECTED(m_icount <= 0)) { push_state(122); return; }
state_122:
		
		// unmassage...
		m_pc.w += 2;
	}

	if (m_nmi_asserted)
		m_nmi_asserted = false;

	set_ea(m_ea.w);	// need to do this to set the addressing mode
	m_cc |= CC_I | (m_ea.w != VECTOR_IRQ ? CC_F : 0);

	// invoke standard interrupt callback for MAME core
	switch (m_ea.w)
	{
		case VECTOR_NMI:	standard_irq_callback(INPUT_LINE_NMI); break;
		case VECTOR_FIRQ:	standard_irq_callback(M6809_FIRQ_LINE); break;
		case VECTOR_IRQ:	standard_irq_callback(M6809_IRQ_LINE); break;
		default:			break;
	}

	goto INTERRUPT_VECTOR;

LEA_xy:
	regop16().w = set_flags(CC_Z, m_ea.w);
	eat(1);
	return;

LEA_us:
	if (&regop16() == &m_s)
		m_lds_encountered = true;
	regop16().w = m_ea.w;
	eat(1);
	return;

PSHS:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(123); return; }
state_123:
	eat(hd6309_native_mode() ? 2 : 3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(124); return; }
state_124:
	set_regop16(m_s);
	goto PUSH_REGISTERS;

PULS:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(125); return; }
state_125:
	eat(hd6309_native_mode() ? 1 : 2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(126); return; }
state_126:
	set_regop16(m_s);
	goto PULL_REGISTERS;

PSHU:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(127); return; }
state_127:
	eat(hd6309_native_mode() ? 2 : 3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(128); return; }
state_128:
	set_regop16(m_u);
	goto PUSH_REGISTERS;

PULU:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(129); return; }
state_129:
	eat(hd6309_native_mode() ? 1 : 2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(130); return; }
state_130:
	set_regop16(m_u);
	goto PULL_REGISTERS;

SWI:
	// doesn't use SOFTWARE_INTERRUPT label because SWI will
	// inhibit IRQ/FIRQ
	set_ea(VECTOR_SWI);
	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(131);
	goto PUSH_REGISTERS;
state_131:
	m_cc |= CC_I | CC_F;
	goto INTERRUPT_VECTOR;

SWI2:
	set_ea(VECTOR_SWI2);
	goto SOFTWARE_INTERRUPT;

SWI3:
	set_ea(VECTOR_SWI3);
	goto SOFTWARE_INTERRUPT;

SOFTWARE_INTERRUPT:
	// used for SWI2/SWI3 and illegal/div0 on 6309
	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(132);
	goto PUSH_REGISTERS;
state_132:
	goto INTERRUPT_VECTOR;
	
DIRECT:
	set_ea(((UINT16)m_dp << 8) | read_opcode_arg_next());
	if (UNEXPECTED(m_icount <= 0)) { push_state(133); return; }
state_133:
	eat(hd6309_native_mode() ? 0 : 1);
	return;

EXTENDED:
	set_ea_h(read_opcode_arg_next());
	if (UNEXPECTED(m_icount <= 0)) { push_state(134); return; }
state_134:
	set_ea_l(read_opcode_arg_next());
	if (UNEXPECTED(m_icount <= 0)) { push_state(135); return; }
state_135:
	eat(hd6309_native_mode() ? 0 : 1);
	return;

PUSH_REGISTERS:
	if (m_temp.w & 0x80)
	{
		write_memory(--regop16().w, m_pc.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(136); return; }
state_136:
		write_memory(--regop16().w, m_pc.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(137); return; }
state_137:
		nop();
	}
	if (m_temp.w & 0x40)
	{
		write_memory(--regop16().w, (&regop16() == &m_s) ? m_u.b.l : m_s.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(138); return; }
state_138:
		write_memory(--regop16().w, (&regop16() == &m_s) ? m_u.b.h : m_s.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(139); return; }
state_139:
		nop();
	}
	if (m_temp.w & 0x20)
	{
		write_memory(--regop16().w, m_y.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(140); return; }
state_140:
		write_memory(--regop16().w, m_y.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(141); return; }
state_141:
		nop();
	}
	if (m_temp.w & 0x10)
	{
		write_memory(--regop16().w, m_x.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(142); return; }
state_142:
		write_memory(--regop16().w, m_x.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(143); return; }
state_143:
		nop();
	}
	if (m_temp.w & 0x08)
	{
		write_memory(--regop16().w, m_dp);
		if (UNEXPECTED(m_icount <= 0)) { push_state(144); return; }
state_144:
		nop();
	}
	if (m_temp.w & 0x04)
	{
		write_memory(--regop16().w, m_d.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(145); return; }
state_145:
		nop();
	}
	if (m_temp.w & 0x02)
	{
		write_memory(--regop16().w, m_d.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(146); return; }
state_146:
		nop();
	}
	if (m_temp.w & 0x01)
	{
		write_memory(--regop16().w, m_cc);
		if (UNEXPECTED(m_icount <= 0)) { push_state(147); return; }
state_147:
		nop();
	}
	return;

PULL_REGISTERS:
	if (m_temp.w & 0x01)
	{
		m_cc = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(148); return; }
state_148:
		nop();
	}
	if (m_temp.w & 0x02)
	{
		m_d.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(149); return; }
state_149:
		nop();
	}
	if (m_temp.w & 0x04)
	{
		m_d.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(150); return; }
state_150:
		nop();
	}
	if (m_temp.w & 0x08)
	{
		m_dp = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(151); return; }
state_151:
		nop();
	}
	if (m_temp.w & 0x10)
	{
		m_x.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(152); return; }
state_152:
		m_x.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(153); return; }
state_153:
		nop();
	}
	if (m_temp.w & 0x20)
	{
		m_y.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(154); return; }
state_154:
		m_y.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(155); return; }
state_155:
		nop();
	}
	if (m_temp.w & 0x40)
	{
		(&regop16() == &m_s ? &m_u : &m_s)->b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(156); return; }
state_156:
		(&regop16() == &m_s ? &m_u : &m_s)->b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(157); return; }
state_157:
		nop();
	}
	if (m_temp.w & 0x80)
	{
		m_pc.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(158); return; }
state_158:
		m_pc.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(159); return; }
state_159:
		nop();
		change_pc(m_pc.w);
	}
	eat(1);
	return;

INDEXED:
	m_opcode = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(160); return; }
state_160:
	switch(m_opcode & 0xF7)
	{
		case 0x07:
			// extended addressing mode
			m_temp.b.h = read_opcode_arg_next();
			if (UNEXPECTED(m_icount <= 0)) { push_state(161); return; }
state_161:
			m_temp.b.l = read_opcode_arg_next();
			if (UNEXPECTED(m_icount <= 0)) { push_state(162); return; }
state_162:
			break;

		case 0x20: case 0x30: case 0x50: case 0x60: case 0x70:
			// auto increment
			m_temp.w = ireg();
			ireg()++;
			eat(2);
			break;

		case 0x21: case 0x31: case 0x51: case 0x61: case 0x71:
			// double auto increment
			m_temp.w = ireg();
			ireg() += 2;
			eat(3);
			break;

		case 0x22: case 0x32: case 0x52: case 0x62: case 0x72:
			// auto decrement
			ireg()--;
			m_temp.w = ireg();
			eat(2);
			break;

		case 0x23: case 0x33: case 0x53: case 0x63: case 0x73:
			// double auto decrement
			ireg() -= 2;
			m_temp.w = ireg();
			eat(3);
			break;

		case 0x24: case 0x34: case 0x54: case 0x64: case 0x74:
			// postbyte offset
			m_ea.w = ireg();	// need to do this now because ireg() might be PC
			m_temp.b.l = read_opcode_arg_next();
			if (UNEXPECTED(m_icount <= 0)) { push_state(163); return; }
state_163:
			m_temp.w = m_ea.w + (INT8) m_temp.b.l;
			eat(1);
			break;

		case 0x25: case 0x35: case 0x55: case 0x65: case 0x75:
			// postword offset
			m_ea.w = ireg();	// need to do this now because ireg() might be PC
			m_temp.b.h = read_opcode_arg_next();
			if (UNEXPECTED(m_icount <= 0)) { push_state(164); return; }
state_164:
			m_temp.b.l = read_opcode_arg_next();
			if (UNEXPECTED(m_icount <= 0)) { push_state(165); return; }
state_165:
			m_temp.w = m_ea.w + (INT16) m_temp.w;
			eat(2);
			break;

		case 0x26: case 0x36: case 0x56: case 0x66: case 0x76:
			m_temp.w = ireg();
			break;

		case 0xC4:
			// direct addressing mode
			m_temp.b.h = m_dp;
			m_temp.b.l = read_opcode_arg_next();
			if (UNEXPECTED(m_icount <= 0)) { push_state(166); return; }
state_166:
			break;

		case 0xA0: case 0xB0: case 0xD0: case 0xE0: case 0xF0:
			// relative to register A
			m_temp.w = ireg() + (INT8) m_d.b.h;
			eat(1);
			if (UNEXPECTED(m_icount <= 0)) { push_state(167); return; }
state_167:
			break;

		case 0xA1: case 0xB1: case 0xD1: case 0xE1: case 0xF1:
			// relative to register B
			m_temp.w = ireg() + (INT8) m_d.b.l;
			eat(1);
			if (UNEXPECTED(m_icount <= 0)) { push_state(168); return; }
state_168:
			break;

		case 0xA7: case 0xB7: case 0xD7: case 0xE7: case 0xF7:
			// relative to register D
			m_temp.w = ireg() + (INT16) m_d.w;
			eat(4);
			if (UNEXPECTED(m_icount <= 0)) { push_state(169); return; }
state_169:
			break;

		default:
			logerror("KONAMI: Unknown/Invalid postbyte at PC = %04x\n", m_pc.w - 1);
			m_temp.w = 0;
			break;
	}

	if (m_opcode & 0x08)
	{
		// indirect mode
		set_ea(m_temp.w);
		m_temp.b.h = read_operand_ord(0);
		if (UNEXPECTED(m_icount <= 0)) { push_state(170); return; }
state_170:
		m_temp.b.l = read_operand_ord(1);
		if (UNEXPECTED(m_icount <= 0)) { push_state(171); return; }
state_171:
		nop();
	}

	set_ea(m_temp.w);
	return;

EXG:
	{
		// konami's EXG instruction differs enough from 6809 to fork the code
		UINT8 param = read_opcode_arg_next();
		exgtfr_register reg1 = read_exgtfr_register(param >> 0);
		exgtfr_register reg2 = read_exgtfr_register(param >> 4);
		write_exgtfr_register(param >> 0, reg2);
		write_exgtfr_register(param >> 4, reg1);
	}
	eat(hd6309_native_mode() ? 3 : 6);
	return;

TFR:
	{
		// konami's TFR instruction differs enough from 6809 to fork the code
		UINT8 param = read_opcode_arg_next();
		exgtfr_register reg = read_exgtfr_register(param >> 0);
		write_exgtfr_register(param >> 4, reg);
	}
	eat(hd6309_native_mode() ? 2 : 4);
	return;

SETLINE:
	set_lines(read_operand());
	return;

MOVE:
	m_temp.b.l = read_memory(m_y.w++);
	if (UNEXPECTED(m_icount <= 0)) { push_state(172); return; }
state_172:
	write_memory(m_x.w++, m_temp.b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(173); return; }
state_173:
	m_u.w--;
	return;

BMOVE:
	// BMOVE does not appear to be interruptable, at least judging from the
	// old implementation
	while(m_u.w != 0)
	{
		m_temp.b.l = read_memory(m_y.w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(174); return; }
state_174:
		write_memory(m_x.w++, m_temp.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(175); return; }
state_175:
		m_u.w--;
	}
	return;

BSET:
	// BSET does not appear to be interruptable, at least judging from the
	// old implementation
	while(m_u.w != 0)
	{
		eat(1);
		if (UNEXPECTED(m_icount <= 0)) { push_state(176); return; }
state_176:
		write_memory(m_x.w++, m_d.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(177); return; }
state_177:
		m_u.w--;
	}
	return;

BSET2:
	// BSET2 does not appear to be interruptable, at least judging from the
	// old implementation
	while(m_u.w != 0)
	{
		eat(1);
		if (UNEXPECTED(m_icount <= 0)) { push_state(178); return; }
state_178:
		write_memory(m_x.w++, m_d.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(179); return; }
state_179:
		write_memory(m_x.w++, m_d.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(180); return; }
state_180:
		m_u.w--;
	}
	return;

DECXJNZ:
	// not sure if this affects V?
	m_x.w = set_flags16(CC_NZV, m_x.w, 1, m_x.w - 1);
	eat(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(181); return; }
state_181:
	set_cond(cond_ne());
	goto BRANCH;

DECBJNZ:
	// not sure if this affects V?
	m_d.b.l = set_flags8(CC_NZV, m_d.b.l, 1, m_d.b.l - 1);
	eat(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(182); return; }
state_182:
	set_cond(cond_ne());
	goto BRANCH;

LSRD:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(183); return; }
state_183:
	if (m_temp.b.l != 0x00)
	{
		// set C condition code
		if (m_d.w & safe_shift_left(1, m_temp.b.l))
			m_cc |= CC_C;
		else
			m_cc &= ~CC_C;

		m_d.w = set_flags16(CC_NZ, safe_shift_right_unsigned16(m_d.w, m_temp.b.l));
	}
	eat(1);
	return;

ASLD:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(184); return; }
state_184:
	if (m_temp.b.l != 0x00)
	{
		// set C condition code
		if (m_d.w & safe_shift_right(0x10000, m_temp.b.l))
			m_cc |= CC_C;
		else
			m_cc &= ~CC_C;

		m_d.w = set_flags16(CC_NZV, safe_shift_left16(m_d.w, m_temp.b.l));
	}
	eat(1);
	return;

ASRD:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(185); return; }
state_185:
	if (m_temp.b.l != 0x00)
	{
		// set C condition code
		if (m_d.w & safe_shift_left(1, m_temp.b.l))
			m_cc |= CC_C;
		else
			m_cc &= ~CC_C;

		m_d.w = set_flags16(CC_NZ, safe_shift_right16(m_d.w, m_temp.b.l));
	}
	eat(1);
	return;

ROLD:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(186); return; }
state_186:

	// doing this as a loop is lame
	while(m_temp.b.l--)
		m_temp.w = set_flags16(CC_NZ, rotate_left(m_temp.w));

	eat(1);
	return;

RORD:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(187); return; }
state_187:

	// doing this as a loop is lame
	while(m_temp.b.l--)
		m_temp.w = set_flags16(CC_NZ, rotate_right(m_temp.w));

	eat(1);
	return;

ABS8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(188); return; }
state_188:
	m_temp.b.l = set_flags8(CC_NZVC, 0, m_temp.b.l, ((INT8) m_temp.b.l) >= 0 ? m_temp.b.l : -m_temp.b.l);
	eat(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(189); return; }
state_189:
	write_operand(m_temp.b.l);
	return;

ABS16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(190); return; }
state_190:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(191); return; }
state_191:
	m_temp.w = set_flags16(CC_NZVC, 0, m_temp.w, ((INT16) m_temp.w) >= 0 ? m_temp.w : -m_temp.w);
	eat(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(192); return; }
state_192:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(193); return; }
state_193:
	write_operand_ord(1, m_temp.b.l);
	return;

LMUL:
	lmul();
	eat(21);
	return;

DIVX:
	divx();
	eat(10);
	return;

ILLEGAL:
	log_illegal();
	return;

	}
}
