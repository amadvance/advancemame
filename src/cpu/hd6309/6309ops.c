// license:BSD-3-Clause
// copyright-holders:Nathan Woods

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))
#pragma GCC diagnostic ignored "-Wunused-label"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static void hd6309_execute_one(void)
{
	switch (pop_state())
	{
		case 0:	goto MAIN;
		case 1:	goto state_1;
		case 2:	goto NEG8;
		case 3:	goto state_3;
		case 4:	goto OIM;
		case 5:	goto state_5;
		case 6:	goto AIM;
		case 7:	goto COM8;
		case 8:	goto LSR8;
		case 9:	goto state_9;
		case 10:	goto EIM;
		case 11:	goto ROR8;
		case 12:	goto ASR8;
		case 13:	goto ASL8;
		case 14:	goto ROL8;
		case 15:	goto DEC8;
		case 16:	goto state_16;
		case 17:	goto TIM;
		case 18:	goto INC8;
		case 19:	goto TST8;
		case 20:	goto JMP;
		case 21:	goto CLR8;
		case 22:	goto LEA_xy;
		case 23:	goto LEA_us;
		case 24:	goto state_24;
		case 25:	goto state_25;
		case 26:	goto state_26;
		case 27:	goto state_27;
		case 28:	goto state_28;
		case 29:	goto state_29;
		case 30:	goto state_30;
		case 31:	goto state_31;
		case 32:	goto SUB8;
		case 33:	goto CMP8;
		case 34:	goto SBC8;
		case 35:	goto SUB16;
		case 36:	goto AND8;
		case 37:	goto BIT8;
		case 38:	goto LD8;
		case 39:	goto ST8;
		case 40:	goto EOR8;
		case 41:	goto ADC8;
		case 42:	goto OR8;
		case 43:	goto ADD8;
		case 44:	goto CMP16;
		case 45:	goto JSR;
		case 46:	goto LD16;
		case 47:	goto ST16;
		case 48:	goto JSR_ind;
		case 49:	goto ADD16;
		case 50:	goto state_50;
		case 51:	goto SBC16;
		case 52:	goto AND16;
		case 53:	goto BIT16;
		case 54:	goto EOR16;
		case 55:	goto ADC16;
		case 56:	goto OR16;
		case 57:	goto LDQ;
		case 58:	goto STQ;
		case 59:	goto state_59;
		case 60:	goto DIVD;
		case 61:	goto DIVQ;
		case 62:	goto MULD;
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
		case 194:	goto state_194;
		case 195:	goto state_195;
		case 196:	goto state_196;
		case 197:	goto state_197;
		case 198:	goto state_198;
		case 199:	goto state_199;
		case 200:	goto state_200;
		case 201:	goto state_201;
		case 202:	goto state_202;
		case 203:	goto state_203;
		case 204:	goto state_204;
		case 205:	goto state_205;
		case 206:	goto state_206;
		case 207:	goto state_207;
		case 208:	goto state_208;
		case 209:	goto state_209;
		case 210:	goto state_210;
		case 211:	goto state_211;
		case 212:	goto state_212;
		case 213:	goto state_213;
		case 214:	goto state_214;
		case 215:	goto state_215;
		case 216:	goto state_216;
		case 217:	goto state_217;
		case 218:	goto state_218;
		case 219:	goto state_219;
		case 220:	goto state_220;
		case 221:	goto state_221;
		case 222:	goto state_222;
		case 223:	goto state_223;
		case 224:	goto state_224;
		case 225:	goto state_225;
		case 226:	goto state_226;
		case 227:	goto state_227;
		case 228:	goto state_228;
		case 229:	goto state_229;
		case 230:	goto state_230;
		case 231:	goto state_231;
		case 232:	goto state_232;
		case 233:	goto state_233;
		case 234:	goto state_234;
		case 235:	goto state_235;
		case 236:	goto state_236;
		case 237:	goto state_237;
		case 238:	goto state_238;
		case 239:	goto state_239;
		case 240:	goto state_240;
		case 241:	goto state_241;
		case 242:	goto state_242;
		case 243:	goto state_243;
		case 244:	goto state_244;
		case 245:	goto state_245;
		case 246:	goto state_246;
		case 247:	goto state_247;
		case 248:	goto state_248;
		case 249:	goto state_249;
		case 250:	goto state_250;
		case 251:	goto state_251;
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
		case 0x00:
			push_state(2);	// NEG8
			goto DIRECT;
		case 0x01:
			push_state(3);
			goto IMM_IM;
state_3:
				push_state(4);	// OIM
				goto DIRECT;
		case 0x02:
			push_state(5);
			goto IMM_IM;
state_5:
				push_state(6);	// AIM
				goto DIRECT;
		case 0x03:
			push_state(7);	// COM8
			goto DIRECT;
		case 0x04:
			push_state(8);	// LSR8
			goto DIRECT;
		case 0x05:
			push_state(9);
			goto IMM_IM;
state_9:
				push_state(10);	// EIM
				goto DIRECT;
		case 0x06:
			push_state(11);	// ROR8
			goto DIRECT;
		case 0x07:
			push_state(12);	// ASR8
			goto DIRECT;
		case 0x08:
			push_state(13);	// ASL8
			goto DIRECT;
		case 0x09:
			push_state(14);	// ROL8
			goto DIRECT;
		case 0x0A:
			push_state(15);	// DEC8
			goto DIRECT;
		case 0x0B:
			push_state(16);
			goto IMM_IM;
state_16:
				push_state(17);	// TIM
				goto DIRECT;
		case 0x0C:
			push_state(18);	// INC8
			goto DIRECT;
		case 0x0D:
			push_state(19);	// TST8
			goto DIRECT;
		case 0x0E:
			push_state(20);	// JMP
			goto DIRECT;
		case 0x0F:
			push_state(21);	// CLR8
			goto DIRECT;

		case 0x10:
			goto DISPATCH10;
		case 0x11:
			goto DISPATCH11;
		case 0x12:
			goto NOP;
		case 0x13:
			goto SYNC;
		case 0x14:
			goto SEXW;
		case 0x16:				set_cond(true);	goto LBRANCH;
		case 0x17:
			goto LBSR;
		case 0x19:
			goto DAA;
		case 0x1A:				set_imm();		goto ORCC;
		case 0x1C:				set_imm();		goto ANDCC;
		case 0x1D:
			goto SEX;
		case 0x1E:
			goto EXG;
		case 0x1F:
			goto TFR;

		case 0x20:				set_cond(true);			goto BRANCH;
		case 0x21:				set_cond(false);		goto BRANCH;
		case 0x22:				set_cond(cond_hi());	goto BRANCH;
		case 0x23:				set_cond(!cond_hi());	goto BRANCH;
		case 0x24:				set_cond(cond_cc());	goto BRANCH;
		case 0x25:				set_cond(!cond_cc());	goto BRANCH;
		case 0x26:				set_cond(cond_ne());	goto BRANCH;
		case 0x27:				set_cond(!cond_ne());	goto BRANCH;
		case 0x28:				set_cond(cond_vc());	goto BRANCH;
		case 0x29:				set_cond(!cond_vc());	goto BRANCH;
		case 0x2A:				set_cond(cond_pl());	goto BRANCH;
		case 0x2B:				set_cond(!cond_pl());	goto BRANCH;
		case 0x2C:				set_cond(cond_ge());	goto BRANCH;
		case 0x2D:				set_cond(!cond_ge());	goto BRANCH;
		case 0x2E:				set_cond(cond_gt());	goto BRANCH;
		case 0x2F:				set_cond(!cond_gt());	goto BRANCH;

		case 0x30:				set_regop16(m_x);		push_state(22);	// LEA_xy
		goto INDEXED;
		case 0x31:				set_regop16(m_y);		push_state(22);	// LEA_xy
		goto INDEXED;
		case 0x32:				set_regop16(m_s);		push_state(23);	// LEA_us
		goto INDEXED;
		case 0x33:				set_regop16(m_u);		push_state(23);	// LEA_us
		goto INDEXED;
		case 0x34:
			goto PSHS;
		case 0x35:
			goto PULS;
		case 0x36:
			goto PSHU;
		case 0x37:
			goto PULU;
		case 0x39:
			goto RTS;
		case 0x3A:
			goto ABX;
		case 0x3B:
			goto RTI;
		case 0x3C:
			goto CWAI;
		case 0x3D:
			goto MUL;
		case 0x3F:
			goto SWI;

		case 0x40:										set_a();	goto NEG8;
		case 0x43:										set_a();	goto COM8;
		case 0x44:										set_a();	goto LSR8;
		case 0x46:										set_a();	goto ROR8;
		case 0x47:										set_a();	goto ASR8;
		case 0x48:										set_a();	goto ASL8;
		case 0x49:										set_a();	goto ROL8;
		case 0x4A:										set_a();	goto DEC8;
		case 0x4C:										set_a();	goto INC8;
		case 0x4D:										set_a();	goto TST8;
		case 0x4F:										set_a();	goto CLR8;

		case 0x50:										set_b();	goto NEG8;
		case 0x53:										set_b();	goto COM8;
		case 0x54:										set_b();	goto LSR8;
		case 0x56:										set_b();	goto ROR8;
		case 0x57:										set_b();	goto ASR8;
		case 0x58:										set_b();	goto ASL8;
		case 0x59:										set_b();	goto ROL8;
		case 0x5A:										set_b();	goto DEC8;
		case 0x5C:										set_b();	goto INC8;
		case 0x5D:										set_b();	goto TST8;
		case 0x5F:										set_b();	goto CLR8;

		case 0x60:
			push_state(2);	// NEG8
			goto INDEXED;
		case 0x61:
			push_state(24);
			goto IMM_IM;
state_24:
				push_state(4);	// OIM
				goto INDEXED;
		case 0x62:
			push_state(25);
			goto IMM_IM;
state_25:
				push_state(6);	// AIM
				goto INDEXED;
		case 0x63:
			push_state(7);	// COM8
			goto INDEXED;
		case 0x64:
			push_state(8);	// LSR8
			goto INDEXED;
		case 0x65:
			push_state(26);
			goto IMM_IM;
state_26:
				push_state(10);	// EIM
				goto INDEXED;
		case 0x66:
			push_state(11);	// ROR8
			goto INDEXED;
		case 0x67:
			push_state(12);	// ASR8
			goto INDEXED;
		case 0x68:
			push_state(13);	// ASL8
			goto INDEXED;
		case 0x69:
			push_state(14);	// ROL8
			goto INDEXED;
		case 0x6A:
			push_state(15);	// DEC8
			goto INDEXED;
		case 0x6B:
			push_state(27);
			goto IMM_IM;
state_27:
				push_state(17);	// TIM
				goto INDEXED;
		case 0x6C:
			push_state(18);	// INC8
			goto INDEXED;
		case 0x6D:
			push_state(19);	// TST8
			goto INDEXED;
		case 0x6E:
			push_state(20);	// JMP
			goto INDEXED;
		case 0x6F:
			push_state(21);	// CLR8
			goto INDEXED;

		case 0x70:
			push_state(2);	// NEG8
			goto EXTENDED;
		case 0x71:
			push_state(28);
			goto IMM_IM;
state_28:
				push_state(4);	// OIM
				goto EXTENDED;
		case 0x72:
			push_state(29);
			goto IMM_IM;
state_29:
				push_state(6);	// AIM
				goto EXTENDED;
		case 0x73:
			push_state(7);	// COM8
			goto EXTENDED;
		case 0x74:
			push_state(8);	// LSR8
			goto EXTENDED;
		case 0x75:
			push_state(30);
			goto IMM_IM;
state_30:
				push_state(10);	// EIM
				goto EXTENDED;
		case 0x76:
			push_state(11);	// ROR8
			goto EXTENDED;
		case 0x77:
			push_state(12);	// ASR8
			goto EXTENDED;
		case 0x78:
			push_state(13);	// ASL8
			goto EXTENDED;
		case 0x79:
			push_state(14);	// ROL8
			goto EXTENDED;
		case 0x7A:
			push_state(15);	// DEC8
			goto EXTENDED;
		case 0x7B:
			push_state(31);
			goto IMM_IM;
state_31:
				push_state(17);	// TIM
				goto EXTENDED;
		case 0x7C:
			push_state(18);	// INC8
			goto EXTENDED;
		case 0x7D:
			push_state(19);	// TST8
			goto EXTENDED;
		case 0x7E:
			push_state(20);	// JMP
			goto EXTENDED;
		case 0x7F:
			push_state(21);	// CLR8
			goto EXTENDED;

		case 0x80:				set_regop8(m_d.b.h);	set_imm();	goto SUB8;
		case 0x81:				set_regop8(m_d.b.h);	set_imm();	goto CMP8;
		case 0x82:				set_regop8(m_d.b.h);	set_imm();	goto SBC8;
		case 0x83:				set_regop16(m_d);		set_imm();	goto SUB16;
		case 0x84:				set_regop8(m_d.b.h);	set_imm();	goto AND8;
		case 0x85:				set_regop8(m_d.b.h);	set_imm();	goto BIT8;
		case 0x86:				set_regop8(m_d.b.h);	set_imm();	goto LD8;
		case 0x88:				set_regop8(m_d.b.h);	set_imm();	goto EOR8;
		case 0x89:				set_regop8(m_d.b.h);	set_imm();	goto ADC8;
		case 0x8A:				set_regop8(m_d.b.h);	set_imm();	goto OR8;
		case 0x8B:				set_regop8(m_d.b.h);	set_imm();	goto ADD8;
		case 0x8C:				set_regop16(m_x);		set_imm();	goto CMP16;
		case 0x8D:
			goto BSR;
		case 0x8E:				set_regop16(m_x);		set_imm();	goto LD16;

		case 0x90:				set_regop8(m_d.b.h);	push_state(32);	// SUB8
	goto DIRECT;
		case 0x91:				set_regop8(m_d.b.h);	push_state(33);	// CMP8
	goto DIRECT;
		case 0x92:				set_regop8(m_d.b.h);	push_state(34);	// SBC8
	goto DIRECT;
		case 0x93:				set_regop16(m_d);		push_state(35);	// SUB16
		goto DIRECT;
		case 0x94:				set_regop8(m_d.b.h);	push_state(36);	// AND8
	goto DIRECT;
		case 0x95:				set_regop8(m_d.b.h);	push_state(37);	// BIT8
	goto DIRECT;
		case 0x96:				set_regop8(m_d.b.h);	push_state(38);	// LD8
	goto DIRECT;
		case 0x97:				set_regop8(m_d.b.h);	push_state(39);	// ST8
	goto DIRECT;
		case 0x98:				set_regop8(m_d.b.h);	push_state(40);	// EOR8
	goto DIRECT;
		case 0x99:				set_regop8(m_d.b.h);	push_state(41);	// ADC8
	goto DIRECT;
		case 0x9A:				set_regop8(m_d.b.h);	push_state(42);	// OR8
	goto DIRECT;
		case 0x9B:				set_regop8(m_d.b.h);	push_state(43);	// ADD8
	goto DIRECT;
		case 0x9C:				set_regop16(m_x);		push_state(44);	// CMP16
		goto DIRECT;
		case 0x9D:
			push_state(45);	// JSR
			goto DIRECT;
		case 0x9E:				set_regop16(m_x);		push_state(46);	// LD16
		goto DIRECT;
		case 0x9F:				set_regop16(m_x);		push_state(47);	// ST16
		goto DIRECT;

		case 0xA0:				set_regop8(m_d.b.h);	push_state(32);	// SUB8
	goto INDEXED;
		case 0xA1:				set_regop8(m_d.b.h);	push_state(33);	// CMP8
	goto INDEXED;
		case 0xA2:				set_regop8(m_d.b.h);	push_state(34);	// SBC8
	goto INDEXED;
		case 0xA3:				set_regop16(m_d);		push_state(35);	// SUB16
		goto INDEXED;
		case 0xA4:				set_regop8(m_d.b.h);	push_state(36);	// AND8
	goto INDEXED;
		case 0xA5:				set_regop8(m_d.b.h);	push_state(37);	// BIT8
	goto INDEXED;
		case 0xA6:				set_regop8(m_d.b.h);	push_state(38);	// LD8
	goto INDEXED;
		case 0xA7:				set_regop8(m_d.b.h);	push_state(39);	// ST8
	goto INDEXED;
		case 0xA8:				set_regop8(m_d.b.h);	push_state(40);	// EOR8
	goto INDEXED;
		case 0xA9:				set_regop8(m_d.b.h);	push_state(41);	// ADC8
	goto INDEXED;
		case 0xAA:				set_regop8(m_d.b.h);	push_state(42);	// OR8
	goto INDEXED;
		case 0xAB:				set_regop8(m_d.b.h);	push_state(43);	// ADD8
	goto INDEXED;
		case 0xAC:				set_regop16(m_x);		push_state(44);	// CMP16
		goto INDEXED;
		case 0xAD:
			push_state(48);	// JSR_ind
			goto INDEXED;
		case 0xAE:				set_regop16(m_x);		push_state(46);	// LD16
		goto INDEXED;
		case 0xAF:				set_regop16(m_x);		push_state(47);	// ST16
		goto INDEXED;

		case 0xB0:				set_regop8(m_d.b.h);	push_state(32);	// SUB8
	goto EXTENDED;
		case 0xB1:				set_regop8(m_d.b.h);	push_state(33);	// CMP8
	goto EXTENDED;
		case 0xB2:				set_regop8(m_d.b.h);	push_state(34);	// SBC8
	goto EXTENDED;
		case 0xB3:				set_regop16(m_d);		push_state(35);	// SUB16
		goto EXTENDED;
		case 0xB4:				set_regop8(m_d.b.h);	push_state(36);	// AND8
	goto EXTENDED;
		case 0xB5:				set_regop8(m_d.b.h);	push_state(37);	// BIT8
	goto EXTENDED;
		case 0xB6:				set_regop8(m_d.b.h);	push_state(38);	// LD8
	goto EXTENDED;
		case 0xB7:				set_regop8(m_d.b.h);	push_state(39);	// ST8
	goto EXTENDED;
		case 0xB8:				set_regop8(m_d.b.h);	push_state(40);	// EOR8
	goto EXTENDED;
		case 0xB9:				set_regop8(m_d.b.h);	push_state(41);	// ADC8
	goto EXTENDED;
		case 0xBA:				set_regop8(m_d.b.h);	push_state(42);	// OR8
	goto EXTENDED;
		case 0xBB:				set_regop8(m_d.b.h);	push_state(43);	// ADD8
	goto EXTENDED;
		case 0xBC:				set_regop16(m_x);		push_state(44);	// CMP16
		goto EXTENDED;
		case 0xBD:
			push_state(45);	// JSR
			goto EXTENDED;
		case 0xBE:				set_regop16(m_x);		push_state(46);	// LD16
		goto EXTENDED;
		case 0xBF:				set_regop16(m_x);		push_state(47);	// ST16
		goto EXTENDED;

		case 0xC0:				set_regop8(m_d.b.l);	set_imm();	goto SUB8;
		case 0xC1:				set_regop8(m_d.b.l);	set_imm();	goto CMP8;
		case 0xC2:				set_regop8(m_d.b.l);	set_imm();	goto SBC8;
		case 0xC3:				set_regop16(m_d);		set_imm();	goto ADD16;
		case 0xC4:				set_regop8(m_d.b.l);	set_imm();	goto AND8;
		case 0xC5:				set_regop8(m_d.b.l);	set_imm();	goto BIT8;
		case 0xC6:				set_regop8(m_d.b.l);	set_imm();	goto LD8;
		case 0xC8:				set_regop8(m_d.b.l);	set_imm();	goto EOR8;
		case 0xC9:				set_regop8(m_d.b.l);	set_imm();	goto ADC8;
		case 0xCA:				set_regop8(m_d.b.l);	set_imm();	goto OR8;
		case 0xCB:				set_regop8(m_d.b.l);	set_imm();	goto ADD8;
		case 0xCC:				set_regop16(m_d);		set_imm();	goto LD16;
		case 0xCD:										set_imm();	goto LDQ;
		case 0xCE:				set_regop16(m_u);		set_imm();	goto LD16;

		case 0xD0:				set_regop8(m_d.b.l);	push_state(32);	// SUB8
	goto DIRECT;
		case 0xD1:				set_regop8(m_d.b.l);	push_state(33);	// CMP8
	goto DIRECT;
		case 0xD2:				set_regop8(m_d.b.l);	push_state(34);	// SBC8
	goto DIRECT;
		case 0xD3:				set_regop16(m_d);		push_state(49);	// ADD16
		goto DIRECT;
		case 0xD4:				set_regop8(m_d.b.l);	push_state(36);	// AND8
	goto DIRECT;
		case 0xD5:				set_regop8(m_d.b.l);	push_state(37);	// BIT8
	goto DIRECT;
		case 0xD6:				set_regop8(m_d.b.l);	push_state(38);	// LD8
	goto DIRECT;
		case 0xD7:				set_regop8(m_d.b.l);	push_state(39);	// ST8
	goto DIRECT;
		case 0xD8:				set_regop8(m_d.b.l);	push_state(40);	// EOR8
	goto DIRECT;
		case 0xD9:				set_regop8(m_d.b.l);	push_state(41);	// ADC8
	goto DIRECT;
		case 0xDA:				set_regop8(m_d.b.l);	push_state(42);	// OR8
	goto DIRECT;
		case 0xDB:				set_regop8(m_d.b.l);	push_state(43);	// ADD8
	goto DIRECT;
		case 0xDC:				set_regop16(m_d);		push_state(46);	// LD16
		goto DIRECT;
		case 0xDD:				set_regop16(m_d);		push_state(47);	// ST16
		goto DIRECT;
		case 0xDE:				set_regop16(m_u);		push_state(46);	// LD16
		goto DIRECT;
		case 0xDF:				set_regop16(m_u);		push_state(47);	// ST16
		goto DIRECT;

		case 0xE0:				set_regop8(m_d.b.l);	push_state(32);	// SUB8
	goto INDEXED;
		case 0xE1:				set_regop8(m_d.b.l);	push_state(33);	// CMP8
	goto INDEXED;
		case 0xE2:				set_regop8(m_d.b.l);	push_state(34);	// SBC8
	goto INDEXED;
		case 0xE3:				set_regop16(m_d);		push_state(49);	// ADD16
		goto INDEXED;
		case 0xE4:				set_regop8(m_d.b.l);	push_state(36);	// AND8
	goto INDEXED;
		case 0xE5:				set_regop8(m_d.b.l);	push_state(37);	// BIT8
	goto INDEXED;
		case 0xE6:				set_regop8(m_d.b.l);	push_state(38);	// LD8
	goto INDEXED;
		case 0xE7:				set_regop8(m_d.b.l);	push_state(39);	// ST8
	goto INDEXED;
		case 0xE8:				set_regop8(m_d.b.l);	push_state(40);	// EOR8
	goto INDEXED;
		case 0xE9:				set_regop8(m_d.b.l);	push_state(41);	// ADC8
	goto INDEXED;
		case 0xEA:				set_regop8(m_d.b.l);	push_state(42);	// OR8
	goto INDEXED;
		case 0xEB:				set_regop8(m_d.b.l);	push_state(43);	// ADD8
	goto INDEXED;
		case 0xEC:				set_regop16(m_d);		push_state(46);	// LD16
		goto INDEXED;
		case 0xED:				set_regop16(m_d);		push_state(47);	// ST16
		goto INDEXED;
		case 0xEE:				set_regop16(m_u);		push_state(46);	// LD16
		goto INDEXED;
		case 0xEF:				set_regop16(m_u);		push_state(47);	// ST16
		goto INDEXED;

		case 0xF0:				set_regop8(m_d.b.l);	push_state(32);	// SUB8
	goto EXTENDED;
		case 0xF1:				set_regop8(m_d.b.l);	push_state(33);	// CMP8
	goto EXTENDED;
		case 0xF2:				set_regop8(m_d.b.l);	push_state(34);	// SBC8
	goto EXTENDED;
		case 0xF3:				set_regop16(m_d);		push_state(49);	// ADD16
		goto EXTENDED;
		case 0xF4:				set_regop8(m_d.b.l);	push_state(36);	// AND8
	goto EXTENDED;
		case 0xF5:				set_regop8(m_d.b.l);	push_state(37);	// BIT8
	goto EXTENDED;
		case 0xF6:				set_regop8(m_d.b.l);	push_state(38);	// LD8
	goto EXTENDED;
		case 0xF7:				set_regop8(m_d.b.l);	push_state(39);	// ST8
	goto EXTENDED;
		case 0xF8:				set_regop8(m_d.b.l);	push_state(40);	// EOR8
	goto EXTENDED;
		case 0xF9:				set_regop8(m_d.b.l);	push_state(41);	// ADC8
	goto EXTENDED;
		case 0xFA:				set_regop8(m_d.b.l);	push_state(42);	// OR8
	goto EXTENDED;
		case 0xFB:				set_regop8(m_d.b.l);	push_state(43);	// ADD8
	goto EXTENDED;
		case 0xFC:				set_regop16(m_d);		push_state(46);	// LD16
		goto EXTENDED;
		case 0xFD:				set_regop16(m_d);		push_state(47);	// ST16
		goto EXTENDED;
		case 0xFE:				set_regop16(m_u);		push_state(46);	// LD16
		goto EXTENDED;
		case 0xFF:				set_regop16(m_u);		push_state(47);	// ST16
		goto EXTENDED;
		default:
			goto ILLEGAL;
			
	}
	return;

DISPATCH10:
	m_opcode = read_opcode_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(50); return; }
state_50:
	switch(m_opcode)
	{
		case 0x20:				set_cond(true);						goto LBRANCH;
		case 0x21:				set_cond(false);					goto LBRANCH;
		case 0x22:				set_cond(cond_hi());				goto LBRANCH;
		case 0x23:				set_cond(!cond_hi());				goto LBRANCH;
		case 0x24:				set_cond(cond_cc());				goto LBRANCH;
		case 0x25:				set_cond(!cond_cc());				goto LBRANCH;
		case 0x26:				set_cond(cond_ne());				goto LBRANCH;
		case 0x27:				set_cond(!cond_ne());				goto LBRANCH;
		case 0x28:				set_cond(cond_vc());				goto LBRANCH;
		case 0x29:				set_cond(!cond_vc());				goto LBRANCH;
		case 0x2A:				set_cond(cond_pl());				goto LBRANCH;
		case 0x2B:				set_cond(!cond_pl());				goto LBRANCH;
		case 0x2C:				set_cond(cond_ge());				goto LBRANCH;
		case 0x2D:				set_cond(!cond_ge());				goto LBRANCH;
		case 0x2E:				set_cond(cond_gt());				goto LBRANCH;
		case 0x2F:				set_cond(!cond_gt());				goto LBRANCH;

		case 0x30:				register_register_op();				if (is_register_register_op_16_bit()) goto ADD16; else goto ADD8;
		case 0x31:				register_register_op();				if (is_register_register_op_16_bit()) goto ADC16; else goto ADC8;
		case 0x32:				register_register_op();				if (is_register_register_op_16_bit()) goto SUB16; else goto SUB8;
		case 0x33:				register_register_op();				if (is_register_register_op_16_bit()) goto SBC16; else goto SBC8;
		case 0x34:				register_register_op();				if (is_register_register_op_16_bit()) goto AND16; else goto AND8;
		case 0x35:				register_register_op();				if (is_register_register_op_16_bit()) goto OR16; else goto OR8;
		case 0x36:				register_register_op();				if (is_register_register_op_16_bit()) goto EOR16; else goto EOR8;
		case 0x37:				register_register_op();				if (is_register_register_op_16_bit()) goto CMP16; else goto CMP8;
		case 0x38:
			goto PSHSW;
		case 0x39:
			goto PULSW;
		case 0x3A:
			goto PSHUW;
		case 0x3B:
			goto PULUW;
		case 0x3F:
			goto SWI2;

		case 0x40:									set_d();		goto NEG16;
		case 0x43:									set_d();		goto COM16;
		case 0x44:									set_d();		goto LSR16;
		case 0x46:									set_d();		goto ROR16;
		case 0x47:									set_d();		goto ASR16;
		case 0x48:									set_d();		goto ASL16;
		case 0x49:									set_d();		goto ROL16;
		case 0x4A:									set_d();		goto DEC16;
		case 0x4C:									set_d();		goto INC16;
		case 0x4D:									set_d();		goto TST16;
		case 0x4F:									set_d();		goto CLR16;

		case 0x50:									set_w();		goto NEG16;
		case 0x53:									set_w();		goto COM16;
		case 0x54:									set_w();		goto LSR16;
		case 0x56:									set_w();		goto ROR16;
		case 0x57:									set_w();		goto ASR16;
		case 0x58:									set_w();		goto ASL16;
		case 0x59:									set_w();		goto ROL16;
		case 0x5A:									set_w();		goto DEC16;
		case 0x5C:									set_w();		goto INC16;
		case 0x5D:									set_w();		goto TST16;
		case 0x5F:									set_w();		goto CLR16;

		case 0x80:				set_regop16(m_w);	set_imm();		goto SUB16;
		case 0x81:				set_regop16(m_w);	set_imm();		goto CMP16;
		case 0x82:				set_regop16(m_d);	set_imm();		goto SBC16;
		case 0x83:				set_regop16(m_d);	set_imm();		goto CMP16;
		case 0x84:				set_regop16(m_d);	set_imm();		goto AND16;
		case 0x85:				set_regop16(m_d);	set_imm();		goto BIT16;
		case 0x86:				set_regop16(m_w);	set_imm();		goto LD16;
		case 0x88:				set_regop16(m_d);	set_imm();		goto EOR16;
		case 0x89:				set_regop16(m_d);	set_imm();		goto ADC16;
		case 0x8A:				set_regop16(m_d);	set_imm();		goto OR16;
		case 0x8B:				set_regop16(m_w);	set_imm();		goto ADD16;
		case 0x8C:				set_regop16(m_y);	set_imm();		goto CMP16;
		case 0x8E:				set_regop16(m_y);	set_imm();		goto LD16;

		case 0x90:				set_regop16(m_w);	push_state(35);	// SUB16
	goto DIRECT;
		case 0x91:				set_regop16(m_w);	push_state(44);	// CMP16
	goto DIRECT;
		case 0x92:				set_regop16(m_d);	push_state(51);	// SBC16
	goto DIRECT;
		case 0x93:				set_regop16(m_d);	push_state(44);	// CMP16
	goto DIRECT;
		case 0x94:				set_regop16(m_d);	push_state(52);	// AND16
	goto DIRECT;
		case 0x95:				set_regop16(m_d);	push_state(53);	// BIT16
	goto DIRECT;
		case 0x96:				set_regop16(m_w);	push_state(46);	// LD16
	goto DIRECT;
		case 0x97:				set_regop16(m_w);	push_state(47);	// ST16
	goto DIRECT;
		case 0x98:				set_regop16(m_d);	push_state(54);	// EOR16
	goto DIRECT;
		case 0x99:				set_regop16(m_d);	push_state(55);	// ADC16
	goto DIRECT;
		case 0x9A:				set_regop16(m_d);	push_state(56);	// OR16
	goto DIRECT;
		case 0x9B:				set_regop16(m_w);	push_state(49);	// ADD16
	goto DIRECT;
		case 0x9C:				set_regop16(m_y);	push_state(44);	// CMP16
	goto DIRECT;
		case 0x9E:				set_regop16(m_y);	push_state(46);	// LD16
	goto DIRECT;
		case 0x9F:				set_regop16(m_y);	push_state(47);	// ST16
	goto DIRECT;

		case 0xA0:				set_regop16(m_w);	push_state(35);	// SUB16
	goto INDEXED;
		case 0xA1:				set_regop16(m_w);	push_state(44);	// CMP16
	goto INDEXED;
		case 0xA2:				set_regop16(m_d);	push_state(51);	// SBC16
	goto INDEXED;
		case 0xA3:				set_regop16(m_d);	push_state(44);	// CMP16
	goto INDEXED;
		case 0xA4:				set_regop16(m_d);	push_state(52);	// AND16
	goto INDEXED;
		case 0xA5:				set_regop16(m_d);	push_state(53);	// BIT16
	goto INDEXED;
		case 0xA6:				set_regop16(m_w);	push_state(46);	// LD16
	goto INDEXED;
		case 0xA7:				set_regop16(m_w);	push_state(47);	// ST16
	goto INDEXED;
		case 0xA8:				set_regop16(m_d);	push_state(54);	// EOR16
	goto INDEXED;
		case 0xA9:				set_regop16(m_d);	push_state(55);	// ADC16
	goto INDEXED;
		case 0xAA:				set_regop16(m_d);	push_state(56);	// OR16
	goto INDEXED;
		case 0xAB:				set_regop16(m_w);	push_state(49);	// ADD16
	goto INDEXED;
		case 0xAC:				set_regop16(m_y);	push_state(44);	// CMP16
	goto INDEXED;
		case 0xAE:				set_regop16(m_y);	push_state(46);	// LD16
	goto INDEXED;
		case 0xAF:				set_regop16(m_y);	push_state(47);	// ST16
	goto INDEXED;

		case 0xB0:				set_regop16(m_w);	push_state(35);	// SUB16
	goto EXTENDED;
		case 0xB1:				set_regop16(m_w);	push_state(44);	// CMP16
	goto EXTENDED;
		case 0xB2:				set_regop16(m_d);	push_state(51);	// SBC16
	goto EXTENDED;
		case 0xB3:				set_regop16(m_d);	push_state(44);	// CMP16
	goto EXTENDED;
		case 0xB4:				set_regop16(m_d);	push_state(52);	// AND16
	goto EXTENDED;
		case 0xB5:				set_regop16(m_d);	push_state(53);	// BIT16
	goto EXTENDED;
		case 0xB6:				set_regop16(m_w);	push_state(46);	// LD16
	goto EXTENDED;
		case 0xB7:				set_regop16(m_w);	push_state(47);	// ST16
	goto EXTENDED;
		case 0xB8:				set_regop16(m_d);	push_state(54);	// EOR16
	goto EXTENDED;
		case 0xB9:				set_regop16(m_d);	push_state(55);	// ADC16
	goto EXTENDED;
		case 0xBA:				set_regop16(m_d);	push_state(56);	// OR16
	goto EXTENDED;
		case 0xBB:				set_regop16(m_w);	push_state(49);	// ADD16
	goto EXTENDED;
		case 0xBC:				set_regop16(m_y);	push_state(44);	// CMP16
	goto EXTENDED;
		case 0xBE:				set_regop16(m_y);	push_state(46);	// LD16
	goto EXTENDED;
		case 0xBF:				set_regop16(m_y);	push_state(47);	// ST16
	goto EXTENDED;

		case 0xCE:				set_regop16(m_s);	set_imm();		goto LD16;
		case 0xDC:
			push_state(57);	// LDQ
			goto DIRECT;
		case 0xDD:
			push_state(58);	// STQ
			goto DIRECT;
		case 0xDE:				set_regop16(m_s);	push_state(46);	// LD16
	goto DIRECT;
		case 0xDF:				set_regop16(m_s);	push_state(47);	// ST16
	goto DIRECT;
		case 0xEC:
			push_state(57);	// LDQ
			goto INDEXED;
		case 0xED:
			push_state(58);	// STQ
			goto INDEXED;
		case 0xEE:				set_regop16(m_s);	push_state(46);	// LD16
	goto INDEXED;
		case 0xEF:				set_regop16(m_s);	push_state(47);	// ST16
	goto INDEXED;
		case 0xFC:
			push_state(57);	// LDQ
			goto EXTENDED;
		case 0xFD:
			push_state(58);	// STQ
			goto EXTENDED;
		case 0xFE:				set_regop16(m_s);	push_state(46);	// LD16
	goto EXTENDED;
		case 0xFF:				set_regop16(m_s);	push_state(47);	// ST16
	goto EXTENDED;

		default:
			goto ILLEGAL;
			
	}
	return;

DISPATCH11:
	m_opcode = read_opcode_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(59); return; }
state_59:
	switch(m_opcode)
	{
		case 0x30:
			goto BAND;
		case 0x31:
			goto BIAND;
		case 0x32:
			goto BOR;
		case 0x33:
			goto BIOR;
		case 0x34:
			goto BEOR;
		case 0x35:
			goto BIEOR;
		case 0x36:
			goto LDBT;
		case 0x37:
			goto STBT;
		case 0x38:
			goto TFM;
		case 0x39:
			goto TFM;
		case 0x3A:
			goto TFM;
		case 0x3B:
			goto TFM;
		case 0x3C:				set_regop8(m_md);	set_imm();		goto BIT8;
		case 0x3D:				set_regop8(m_md);	set_imm();		goto LD8;
		case 0x3F:
			goto SWI3;

		case 0x43:									set_e();		goto COM8;
		case 0x4A:									set_e();		goto DEC8;
		case 0x4C:									set_e();		goto INC8;
		case 0x4D:									set_e();		goto TST8;
		case 0x4F:									set_e();		goto CLR8;

		case 0x53:									set_f();		goto COM8;
		case 0x5A:									set_f();		goto DEC8;
		case 0x5C:									set_f();		goto INC8;
		case 0x5D:									set_f();		goto TST8;
		case 0x5F:									set_f();		goto CLR8;

		case 0x80:				set_regop8(m_w.b.h);	set_imm();		goto SUB8;
		case 0x81:				set_regop8(m_w.b.h);	set_imm();		goto CMP8;
		case 0x83:				set_regop16(m_u);		set_imm();		goto CMP16;
		case 0x86:				set_regop8(m_w.b.h);	set_imm();		goto LD8;
		case 0x8B:				set_regop8(m_w.b.h);	set_imm();		goto ADD8;
		case 0x8C:				set_regop16(m_s);		set_imm();		goto CMP16;
		case 0x8D:										set_imm();		goto DIVD;
		case 0x8E:										set_imm();		goto DIVQ;
		case 0x8F:										set_imm();		goto MULD;

		case 0x90:				set_regop8(m_w.b.h);	push_state(32);	// SUB8
	goto DIRECT;
		case 0x91:				set_regop8(m_w.b.h);	push_state(33);	// CMP8
	goto DIRECT;
		case 0x93:				set_regop16(m_u);		push_state(44);	// CMP16
		goto DIRECT;
		case 0x96:				set_regop8(m_w.b.h);	push_state(38);	// LD8
	goto DIRECT;
		case 0x97:				set_regop8(m_w.b.h);	push_state(39);	// ST8
	goto DIRECT;
		case 0x9B:				set_regop8(m_w.b.h);	push_state(43);	// ADD8
	goto DIRECT;
		case 0x9C:				set_regop16(m_s);		push_state(44);	// CMP16
		goto DIRECT;
		case 0x9D:
			push_state(60);	// DIVD
			goto DIRECT;
		case 0x9E:
			push_state(61);	// DIVQ
			goto DIRECT;
		case 0x9F:
			push_state(62);	// MULD
			goto DIRECT;

		case 0xA0:				set_regop8(m_w.b.h);	push_state(32);	// SUB8
	goto INDEXED;
		case 0xA1:				set_regop8(m_w.b.h);	push_state(33);	// CMP8
	goto INDEXED;
		case 0xA3:				set_regop16(m_u);		push_state(44);	// CMP16
		goto INDEXED;
		case 0xA6:				set_regop8(m_w.b.h);	push_state(38);	// LD8
	goto INDEXED;
		case 0xA7:				set_regop8(m_w.b.h);	push_state(39);	// ST8
	goto INDEXED;
		case 0xAB:				set_regop8(m_w.b.h);	push_state(43);	// ADD8
	goto INDEXED;
		case 0xAC:				set_regop16(m_s);		push_state(44);	// CMP16
		goto INDEXED;
		case 0xAD:
			push_state(60);	// DIVD
			goto INDEXED;
		case 0xAE:
			push_state(61);	// DIVQ
			goto INDEXED;
		case 0xAF:
			push_state(62);	// MULD
			goto INDEXED;

		case 0xB0:				set_regop8(m_w.b.h);	push_state(32);	// SUB8
	goto EXTENDED;
		case 0xB1:				set_regop8(m_w.b.h);	push_state(33);	// CMP8
	goto EXTENDED;
		case 0xB3:				set_regop16(m_u);		push_state(44);	// CMP16
		goto EXTENDED;
		case 0xB6:				set_regop8(m_w.b.h);	push_state(38);	// LD8
	goto EXTENDED;
		case 0xB7:				set_regop8(m_w.b.h);	push_state(39);	// ST8
	goto EXTENDED;
		case 0xBB:				set_regop8(m_w.b.h);	push_state(43);	// ADD8
	goto EXTENDED;
		case 0xBC:				set_regop16(m_s);		push_state(44);	// CMP16
		goto EXTENDED;
		case 0xBD:
			push_state(60);	// DIVD
			goto EXTENDED;
		case 0xBE:
			push_state(61);	// DIVQ
			goto EXTENDED;
		case 0xBF:
			push_state(62);	// MULD
			goto EXTENDED;

		case 0xC0:				set_regop8(m_w.b.l);	set_imm();		goto SUB8;
		case 0xC1:				set_regop8(m_w.b.l);	set_imm();		goto CMP8;
		case 0xC6:				set_regop8(m_w.b.l);	set_imm();		goto LD8;
		case 0xCB:				set_regop8(m_w.b.l);	set_imm();		goto ADD8;

		case 0xD0:				set_regop8(m_w.b.l);	push_state(32);	// SUB8
	goto DIRECT;
		case 0xD1:				set_regop8(m_w.b.l);	push_state(33);	// CMP8
	goto DIRECT;
		case 0xD6:				set_regop8(m_w.b.l);	push_state(38);	// LD8
	goto DIRECT;
		case 0xD7:				set_regop8(m_w.b.l);	push_state(39);	// ST8
	goto DIRECT;
		case 0xDB:				set_regop8(m_w.b.l);	push_state(43);	// ADD8
	goto DIRECT;

		case 0xE0:				set_regop8(m_w.b.l);	push_state(32);	// SUB8
	goto INDEXED;
		case 0xE1:				set_regop8(m_w.b.l);	push_state(33);	// CMP8
	goto INDEXED;
		case 0xE6:				set_regop8(m_w.b.l);	push_state(38);	// LD8
	goto INDEXED;
		case 0xE7:				set_regop8(m_w.b.l);	push_state(39);	// ST8
	goto INDEXED;
		case 0xEB:				set_regop8(m_w.b.l);	push_state(43);	// ADD8
	goto INDEXED;

		case 0xF0:				set_regop8(m_w.b.l);	push_state(32);	// SUB8
	goto EXTENDED;
		case 0xF1:				set_regop8(m_w.b.l);	push_state(33);	// CMP8
	goto EXTENDED;
		case 0xF6:				set_regop8(m_w.b.l);	push_state(38);	// LD8
	goto EXTENDED;
		case 0xF7:				set_regop8(m_w.b.l);	push_state(39);	// ST8
	goto EXTENDED;
		case 0xFB:				set_regop8(m_w.b.l);	push_state(43);	// ADD8
	goto EXTENDED;

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
	push_state(63);
	goto PUSH_REGISTERS;
state_63:
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
	push_state(64);
	goto PUSH_REGISTERS;
state_64:
	m_cc |= CC_I | CC_F;
	set_ea(VECTOR_FIRQ);
	eat(1);
	standard_irq_callback(M6809_FIRQ_LINE);
	goto INTERRUPT_VECTOR;

IRQ:
	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(65);
	goto PUSH_REGISTERS;
state_65:
	m_cc |= CC_I;
	set_ea(VECTOR_IRQ);
	eat(1);
	standard_irq_callback(M6809_IRQ_LINE);
	goto INTERRUPT_VECTOR;

INTERRUPT_VECTOR:
	eat(4);
	if (UNEXPECTED(m_icount <= 0)) { push_state(66); return; }
state_66:
	m_pc.b.h = read_operand_ord(0);	if (UNEXPECTED(m_icount <= 0)) { push_state(67); return; }
state_67:
	// Not sure if this is cycle exact
	m_pc.b.l = read_operand_ord(1);	if (UNEXPECTED(m_icount <= 0)) { push_state(68); return; }
state_68:
	// Not sure if this is cycle exact
	change_pc(m_pc.w);
	return;

NEG8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(69); return; }
state_69:
	m_temp.b.l = set_flags(CC_NZVC, (UINT8)0, m_temp.b.l, -m_temp.b.l);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(70); return; }
state_70:
	write_operand(m_temp.b.l);
	return;

COM8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(71); return; }
state_71:
	m_cc &= ~CC_V;
	m_cc |= CC_C;
	m_temp.b.l = set_flags(CC_NZ, (UINT8) ~m_temp.b.l);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(72); return; }
state_72:
	write_operand(m_temp.b.l);
	return;

LSR8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(73); return; }
state_73:
	m_cc &= ~CC_C;
	m_cc |= (m_temp.b.l & 1) ? CC_C : 0;
	m_temp.b.l = set_flags8(CC_NZ, m_temp.b.l >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(74); return; }
state_74:
	write_operand(m_temp.b.l);
	return;

ROR8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(75); return; }
state_75:
	m_temp.b.l = set_flags8(CC_NZ, rotate_right(m_temp.b.l));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(76); return; }
state_76:
	write_operand(m_temp.b.l);
	return;

ASR8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(77); return; }
state_77:
	m_cc &= ~CC_NZC;
	m_cc |= (m_temp.b.l & 1) ? CC_C : 0;
	m_temp.b.l = set_flags8(CC_NZ, ((INT8) m_temp.b.l) >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(78); return; }
state_78:
	write_operand(m_temp.b.l);
	return;

ASL8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(79); return; }
state_79:
	m_temp.b.l = set_flags8(CC_NZVC, m_temp.b.l, m_temp.b.l, m_temp.b.l << 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(80); return; }
state_80:
	write_operand(m_temp.b.l);
	return;

ROL8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(81); return; }
state_81:
	m_temp.b.l = set_flags8(CC_NZV, m_temp.b.l, m_temp.b.l, rotate_left(m_temp.b.l));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(82); return; }
state_82:
	write_operand(m_temp.b.l);
	return;

DEC8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(83); return; }
state_83:
	m_temp.b.l = set_flags8(CC_NZV, m_temp.b.l, 1, m_temp.b.l - 1);
	eat(hd6309_native_mode() && is_register_addressing_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(84); return; }
state_84:
	write_operand(m_temp.b.l);
	return;

INC8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(85); return; }
state_85:
	m_temp.b.l = set_flags8(CC_NZV, m_temp.b.l, 1, m_temp.b.l + 1);
	eat(hd6309_native_mode() && is_register_addressing_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(86); return; }
state_86:
	write_operand(m_temp.b.l);
	return;

TST8:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(87); return; }
state_87:
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
	if (UNEXPECTED(m_icount <= 0)) { push_state(88); return; }
state_88:
	m_cc &= ~CC_NZVC;
	m_cc |= CC_Z;
	eat(hd6309_native_mode() && is_register_addressing_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(89); return; }
state_89:
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
	if (UNEXPECTED(m_icount <= 0)) { push_state(90); return; }
state_90:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(91); return; }
state_91:
	m_cc &= ~CC_C;
	m_cc |= (m_temp.w & 1) ? CC_C : 0;
	m_temp.w = set_flags16(CC_NZ, m_temp.w >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(92); return; }
state_92:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(93); return; }
state_93:
	write_operand_ord(1, m_temp.b.l);
	return;

ROR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(94); return; }
state_94:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(95); return; }
state_95:
	m_temp.w = set_flags16(CC_NZ, rotate_right(m_temp.w));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(96); return; }
state_96:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(97); return; }
state_97:
	write_operand_ord(1, m_temp.b.l);
	return;

ASR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(98); return; }
state_98:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(99); return; }
state_99:
	m_cc &= ~CC_NZC;
	m_cc |= (m_temp.w & 1) ? CC_C : 0;
	m_temp.w = set_flags16(CC_NZ, ((INT16) m_temp.w) >> 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(100); return; }
state_100:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(101); return; }
state_101:
	write_operand_ord(1, m_temp.b.l);
	return;

ASL16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(102); return; }
state_102:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(103); return; }
state_103:
	m_temp.w = set_flags16(CC_NZVC, m_temp.w, m_temp.w, m_temp.w << 1);
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(104); return; }
state_104:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(105); return; }
state_105:
	write_operand_ord(1, m_temp.b.l);
	return;

ROL16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(106); return; }
state_106:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(107); return; }
state_107:
	m_temp.w = set_flags16(CC_NZV, rotate_left(m_temp.w));
	eat(hd6309_native_mode() ? 0 : 1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(108); return; }
state_108:
	write_operand_ord(0, m_temp.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(109); return; }
state_109:
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
	if (UNEXPECTED(m_icount <= 0)) { push_state(110); return; }
state_110:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(111); return; }
state_111:
	regop16().w = set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w + m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

SUB16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(112); return; }
state_112:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(113); return; }
state_113:
	regop16().w = set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w - m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

CMP16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(114); return; }
state_114:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(115); return; }
state_115:
	set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w - m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	return;

LD8:
	regop8() = read_operand();
	set_flags(CC_NZV, regop8());
	return;

LD16:
	regop16().b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(116); return; }
state_116:
	regop16().b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(117); return; }
state_117:
	set_flags(CC_NZV, regop16().w);
	if (&regop16() == &m_s)
		m_lds_encountered = true;
	return;

ST8:
	write_ea(set_flags(CC_NZV, regop8()));
	return;

ST16:
	write_operand_ord(0, regop16().b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(118); return; }
state_118:
	write_operand_ord(1, regop16().b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(119); return; }
state_119:
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
		if (UNEXPECTED(m_icount <= 0)) { push_state(120); return; }
state_120:
		
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
	if (UNEXPECTED(m_icount <= 0)) { push_state(121); return; }
state_121:
	eat(1);
	if (branch_taken())
	{
		m_pc.w += (INT8) m_temp.b.l;
		change_pc(m_pc.w);
	}
	return;

LBRANCH:
	m_temp.b.h = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(122); return; }
state_122:
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(123); return; }
state_123:
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
	if (UNEXPECTED(m_icount <= 0)) { push_state(124); return; }
state_124:
	m_ea.w = m_pc.w + (INT8) m_temp.b.l;
	eat(hd6309_native_mode() ? 2 : 3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(125); return; }
state_125:
	goto GOTO_SUBROUTINE;

LBSR:
	m_temp.b.h = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(126); return; }
state_126:
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(127); return; }
state_127:
	m_ea.w = m_pc.w + (INT16) m_temp.w;
	eat(hd6309_native_mode() ? 2 : 4);
	if (UNEXPECTED(m_icount <= 0)) { push_state(128); return; }
state_128:
	goto GOTO_SUBROUTINE;

JSR:
	eat(2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(129); return; }
state_129:
	goto GOTO_SUBROUTINE;
	
GOTO_SUBROUTINE:
	write_memory(--m_s.w, m_pc.b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(130); return; }
state_130:
	write_memory(--m_s.w, m_pc.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(131); return; }
state_131:
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
	push_state(132);
	goto PULL_REGISTERS;
state_132:
	m_temp.w = ((m_cc & CC_E) ? entire_state_registers() : partial_state_registers()) & ~0x01;
	goto PULL_REGISTERS;

CWAI:
	m_cc &= read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(133); return; }
state_133:
	eat(2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(134); return; }
state_134:

	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(135);
	goto PUSH_REGISTERS;
state_135:

	while((m_ea.w = get_pending_interrupt()) == 0)
	{
		// massaging the PC this way makes the debugger's behavior more
		// intuitive
		m_pc.w -= 2;

		eat_remaining();
		if (UNEXPECTED(m_icount <= 0)) { push_state(136); return; }
state_136:
		
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
	if (UNEXPECTED(m_icount <= 0)) { push_state(137); return; }
state_137:
	eat(hd6309_native_mode() ? 2 : 3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(138); return; }
state_138:
	set_regop16(m_s);
	goto PUSH_REGISTERS;

PULS:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(139); return; }
state_139:
	eat(hd6309_native_mode() ? 1 : 2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(140); return; }
state_140:
	set_regop16(m_s);
	goto PULL_REGISTERS;

PSHU:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(141); return; }
state_141:
	eat(hd6309_native_mode() ? 2 : 3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(142); return; }
state_142:
	set_regop16(m_u);
	goto PUSH_REGISTERS;

PULU:
	m_temp.w = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(143); return; }
state_143:
	eat(hd6309_native_mode() ? 1 : 2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(144); return; }
state_144:
	set_regop16(m_u);
	goto PULL_REGISTERS;

SWI:
	// doesn't use SOFTWARE_INTERRUPT label because SWI will
	// inhibit IRQ/FIRQ
	set_ea(VECTOR_SWI);
	m_cc |= CC_E;
	set_regop16(m_s);
	m_temp.w = entire_state_registers();
	push_state(145);
	goto PUSH_REGISTERS;
state_145:
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
	push_state(146);
	goto PUSH_REGISTERS;
state_146:
	goto INTERRUPT_VECTOR;
	
DIRECT:
	set_ea(((UINT16)m_dp << 8) | read_opcode_arg_next());
	if (UNEXPECTED(m_icount <= 0)) { push_state(147); return; }
state_147:
	eat(hd6309_native_mode() ? 0 : 1);
	return;

EXTENDED:
	set_ea_h(read_opcode_arg_next());
	if (UNEXPECTED(m_icount <= 0)) { push_state(148); return; }
state_148:
	set_ea_l(read_opcode_arg_next());
	if (UNEXPECTED(m_icount <= 0)) { push_state(149); return; }
state_149:
	eat(hd6309_native_mode() ? 0 : 1);
	return;

PUSH_REGISTERS:
	if (m_temp.w & 0x80)
	{
		write_memory(--regop16().w, m_pc.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(150); return; }
state_150:
		write_memory(--regop16().w, m_pc.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(151); return; }
state_151:
		nop();
	}
	if (m_temp.w & 0x40)
	{
		write_memory(--regop16().w, (&regop16() == &m_s) ? m_u.b.l : m_s.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(152); return; }
state_152:
		write_memory(--regop16().w, (&regop16() == &m_s) ? m_u.b.h : m_s.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(153); return; }
state_153:
		nop();
	}
	if (m_temp.w & 0x20)
	{
		write_memory(--regop16().w, m_y.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(154); return; }
state_154:
		write_memory(--regop16().w, m_y.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(155); return; }
state_155:
		nop();
	}
	if (m_temp.w & 0x10)
	{
		write_memory(--regop16().w, m_x.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(156); return; }
state_156:
		write_memory(--regop16().w, m_x.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(157); return; }
state_157:
		nop();
	}
	if (m_temp.w & 0x08)
	{
		write_memory(--regop16().w, m_dp);
		if (UNEXPECTED(m_icount <= 0)) { push_state(158); return; }
state_158:
		nop();
	}
	if (m_temp.w & 0x200)
	{
		write_memory(--regop16().w, m_w.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(159); return; }
state_159:
		nop();
	}
	if (m_temp.w & 0x100)
	{
		write_memory(--regop16().w, m_w.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(160); return; }
state_160:
		nop();
	}
	if (m_temp.w & 0x04)
	{
		write_memory(--regop16().w, m_d.b.l);
		if (UNEXPECTED(m_icount <= 0)) { push_state(161); return; }
state_161:
		nop();
	}
	if (m_temp.w & 0x02)
	{
		write_memory(--regop16().w, m_d.b.h);
		if (UNEXPECTED(m_icount <= 0)) { push_state(162); return; }
state_162:
		nop();
	}
	if (m_temp.w & 0x01)
	{
		write_memory(--regop16().w, m_cc);
		if (UNEXPECTED(m_icount <= 0)) { push_state(163); return; }
state_163:
		nop();
	}
	return;

PULL_REGISTERS:
	if (m_temp.w & 0x01)
	{
		m_cc = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(164); return; }
state_164:
		nop();
	}
	if (m_temp.w & 0x02)
	{
		m_d.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(165); return; }
state_165:
		nop();
	}
	if (m_temp.w & 0x04)
	{
		m_d.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(166); return; }
state_166:
		nop();
	}
	if (m_temp.w & 0x100)
	{
		m_w.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(167); return; }
state_167:
		nop();
	}
	if (m_temp.w & 0x200)
	{
		m_w.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(168); return; }
state_168:
		nop();
	}
	if (m_temp.w & 0x08)
	{
		m_dp = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(169); return; }
state_169:
		nop();
	}
	if (m_temp.w & 0x10)
	{
		m_x.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(170); return; }
state_170:
		m_x.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(171); return; }
state_171:
		nop();
	}
	if (m_temp.w & 0x20)
	{
		m_y.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(172); return; }
state_172:
		m_y.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(173); return; }
state_173:
		nop();
	}
	if (m_temp.w & 0x40)
	{
		(&regop16() == &m_s ? &m_u : &m_s)->b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(174); return; }
state_174:
		(&regop16() == &m_s ? &m_u : &m_s)->b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(175); return; }
state_175:
		nop();
	}
	if (m_temp.w & 0x80)
	{
		m_pc.b.h = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(176); return; }
state_176:
		m_pc.b.l = read_memory(regop16().w++);
		if (UNEXPECTED(m_icount <= 0)) { push_state(177); return; }
state_177:
		nop();
		change_pc(m_pc.w);
	}
	eat(1);
	return;

INDEXED:
	m_opcode = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(178); return; }
state_178:
	if (m_opcode & 0x80)
	{
		switch(m_opcode & 0x7F)
		{
			case 0x00: case 0x20: case 0x40: case 0x60:
				m_temp.w = ireg();
				ireg()++;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 2 : 3);
				break;

			case 0x01: case 0x21: case 0x41: case 0x61:
			case 0x11: case 0x31: case 0x51: case 0x71:
				m_temp.w = ireg();
				ireg() += 2;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 3 : 4);
				break;

			case 0x02: case 0x22: case 0x42: case 0x62:
				ireg()--;
				m_temp.w = ireg();
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 2 : 3);
				break;

			case 0x03: case 0x23: case 0x43: case 0x63:
			case 0x13: case 0x33: case 0x53: case 0x73:
				ireg() -= 2;
				m_temp.w = ireg();
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 3 : 4);
				break;

			case 0x04: case 0x24: case 0x44: case 0x64:
			case 0x14: case 0x34: case 0x54: case 0x74:
				m_temp.w = ireg();
				eat(1);
				break;

			case 0x05: case 0x25: case 0x45: case 0x65:
			case 0x15: case 0x35: case 0x55: case 0x75:
				m_temp.w = ireg() + (INT8) m_d.b.l;
				eat(2);
				break;

			case 0x06: case 0x26: case 0x46: case 0x66:
			case 0x16: case 0x36: case 0x56: case 0x76:
				m_temp.w = ireg() + (INT8) m_d.b.h;
				eat(2);
				break;

			case 0x08: case 0x28: case 0x48: case 0x68:
			case 0x18: case 0x38: case 0x58: case 0x78:
				m_temp.w = ireg() + (INT8) read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(179); return; }
state_179:
				eat(1);
				break;

			case 0x09: case 0x29: case 0x49: case 0x69:
			case 0x19: case 0x39: case 0x59: case 0x79:
				m_temp.b.h = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(180); return; }
state_180:
				m_temp.b.l = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(181); return; }
state_181:
				m_temp.w = ireg() + m_temp.w;
				eat(hd6309_native_mode() ? 2 : 3);
				break;

			case 0x0B: case 0x2B: case 0x4B: case 0x6B:
			case 0x1B: case 0x3B: case 0x5B: case 0x7B:
				m_temp.w = ireg() + m_d.w;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 3 : 5);
				break;

			case 0x0C: case 0x2C: case 0x4C: case 0x6C:
			case 0x1C: case 0x3C: case 0x5C: case 0x7C:
				m_temp.b.l = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(182); return; }
state_182:
				m_temp.w = m_pc.w + (INT8) m_temp.b.l;
				eat(1);
				break;

			case 0x0D: case 0x2D: case 0x4D: case 0x6D:
			case 0x1D: case 0x3D: case 0x5D: case 0x7D:
				m_temp.b.h = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(183); return; }
state_183:
				m_temp.b.l = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(184); return; }
state_184:
				m_temp.w = m_pc.w + (INT16) m_temp.w;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 2 : 4);
				break;

			case 0x1F: case 0x3F: case 0x5F: case 0x7F:
				m_temp.b.h = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(185); return; }
state_185:
				m_temp.b.l = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(186); return; }
state_186:
				eat(1);
				break;

			case 0x07: case 0x27: case 0x47: case 0x67:
			case 0x17: case 0x37: case 0x57: case 0x77:
				// 6309 specific mode
				m_temp.w = ireg() + (INT8) m_w.b.h;
				eat(2);
				break;

			case 0x0A: case 0x2A: case 0x4A: case 0x6A:
			case 0x1A: case 0x3A: case 0x5A: case 0x7A:
				// 6309 specific mode
				m_temp.w = ireg() + (INT8) m_w.b.l;
				eat(2);
				break;

			case 0x0E: case 0x2E: case 0x4E: case 0x6E:
			case 0x1E: case 0x3E: case 0x5E: case 0x7E:
				// 6309 specific mode
				m_temp.w = ireg() + m_w.w;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 2 : 5);
				break;

			case 0x0F:
				// 6309 specific mode
				m_temp.w = m_w.w;
				eat(1);
				break;

			case 0x2F:
				// 6309 specific mode
				m_temp.b.h = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(187); return; }
state_187:
				m_temp.b.l = read_opcode_arg_next();
				if (UNEXPECTED(m_icount <= 0)) { push_state(188); return; }
state_188:
				m_temp.w = m_w.w + m_temp.w;
				eat(hd6309_native_mode() ? 1 : 5);
				break;

			case 0x4F:
				// 6309 specific mode
				m_temp.w = m_w.w;
				m_w.w += 2;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 2 : 4);
				break;

			case 0x6F:
				// 6309 specific mode
				m_w.w -= 2;
				m_temp.w = m_w.w;
				eat((hd6309_native_mode() && !(m_opcode & 0x10)) ? 2 : 4);
				break;

			default:
				goto ILLEGAL;
		}

		// indirect mode
		if (m_opcode & 0x10)
		{
			set_ea(m_temp.w);
			m_temp.b.h = read_operand_ord(0);
			if (UNEXPECTED(m_icount <= 0)) { push_state(189); return; }
state_189:
			m_temp.b.l = read_operand_ord(1);
			if (UNEXPECTED(m_icount <= 0)) { push_state(190); return; }
state_190:
			eat(1);
		}
	}
	else
	{
		// 5-bit offset
		m_temp.w = ireg() + (INT8) ((m_opcode & 0x0F) | (m_opcode & 0x10 ? 0xF0 : 0x00));
		eat(2);
	}
	set_ea(m_temp.w);
	return;

EXG:
	{
		UINT8 param = read_opcode_arg_next();
		exgtfr_register reg1 = read_exgtfr_register(param >> 4);
		exgtfr_register reg2 = read_exgtfr_register(param >> 0);
		write_exgtfr_register(param >> 4, reg2);
		write_exgtfr_register(param >> 0, reg1);
	}
	eat(hd6309_native_mode() ? 3 : 6);
	return;

TFR:
	{
		UINT8 param = read_opcode_arg_next();
		exgtfr_register reg = read_exgtfr_register(param >> 4);
		write_exgtfr_register(param >> 0, reg);
	}
	eat(hd6309_native_mode() ? 2 : 4);
	return;

ILLEGAL:
	m_md |= 0x40;		// illegal op flag
	set_ea(VECTOR_ILLEGAL);
	goto SOFTWARE_INTERRUPT;

DIVIDE_BY_ZERO:
	m_md |= 0x80;		// divide by zero flag
	set_ea(VECTOR_ILLEGAL);
	goto SOFTWARE_INTERRUPT;

IMM_IM:
	m_temp_im = read_opcode_arg_next();
	return;

JSR_ind:
	// this is dubious, but the old core did this
	eat(hd6309_native_mode() ? -1 : 0);
	goto JSR;

LDQ:
	m_d.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(191); return; }
state_191:
	m_d.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(192); return; }
state_192:
	m_w.b.h = read_operand_ord(2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(193); return; }
state_193:
	m_w.b.l = read_operand_ord(3);
	if (UNEXPECTED(m_icount <= 0)) { push_state(194); return; }
state_194:
	set_flags32(CC_NZV, get_q());
	return;

STQ:
	write_operand_ord(0, m_d.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(195); return; }
state_195:
	write_operand_ord(1, m_d.b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(196); return; }
state_196:
	write_operand_ord(2, m_w.b.h);
	if (UNEXPECTED(m_icount <= 0)) { push_state(197); return; }
state_197:
	write_operand_ord(3, m_w.b.l);
	if (UNEXPECTED(m_icount <= 0)) { push_state(198); return; }
state_198:
	set_flags32(CC_NZV, get_q());
	return;

OIM:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(199); return; }
state_199:
	m_cc &= ~CC_V;
	m_temp.b.l = set_flags(CC_NZ, (UINT8)0, m_temp.b.l, m_temp.b.l | m_temp_im);
	eat(1);	if (UNEXPECTED(m_icount <= 0)) { push_state(200); return; }
state_200:
	// this is just a guess
	write_operand(m_temp.b.l);
	return;

AIM:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(201); return; }
state_201:
	m_cc &= ~CC_V;
	m_temp.b.l = set_flags(CC_NZ, (UINT8)0, m_temp.b.l, m_temp.b.l & m_temp_im);
	eat(1);	if (UNEXPECTED(m_icount <= 0)) { push_state(202); return; }
state_202:
	// this is just a guess
	write_operand(m_temp.b.l);
	return;

EIM:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(203); return; }
state_203:
	m_cc &= ~CC_V;
	m_temp.b.l = set_flags(CC_NZ, (UINT8)0, m_temp.b.l, m_temp.b.l ^ m_temp_im);
	eat(1);	if (UNEXPECTED(m_icount <= 0)) { push_state(204); return; }
state_204:
	// this is just a guess
	write_operand(m_temp.b.l);
	return;

TIM:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(205); return; }
state_205:
	m_cc &= ~CC_V;
	m_temp.b.l = set_flags(CC_NZ, (UINT8)0, m_temp.b.l, m_temp.b.l & m_temp_im);
	eat(2);	if (UNEXPECTED(m_icount <= 0)) { push_state(206); return; }
state_206:
	// this is just a guess
	return;

TFM:
	// The TFM instruction takes up 6 CPU cycles, plus 3 CPU cycles
	// for each byte transferred.  At least three of these are spent
	// reading the opcodes (00010001 001110xx xxxxxxxx).
	//
	// The old (pre-0.149) core allowed interrupts to interrupt
	// execution of the transfer by implementing a transfer as an
	// operation that executed one transfer, and bumped the program
	// counter back.  However, some documentation suggests that TFM
	// was abortable, so we now have a get_pending_interrupt() call
	// here. 
	//
	// Lastly, I have no information on the precise sub-instruction timing
	// here; the timings of the reads and writes are really just a guess.
	m_temp.b.l = read_opcode_arg_next();
	if (UNEXPECTED(m_icount <= 0)) { push_state(207); return; }
state_207:

	while (m_w.w != 0x0000)
	{
		// TFM is abortable - we need to check for a pending interrupt
		if (get_pending_interrupt() != 0)
		{
			m_pc.w -= 3;
			change_pc(m_pc.w);
			return;
		}

		if (!tfr_read(m_opcode, m_temp.b.l, m_temp.b.h))
			goto ILLEGAL;
		eat(0);
		if (UNEXPECTED(m_icount <= 0)) { push_state(208); return; }
state_208:

		if (!tfr_write(m_opcode, m_temp.b.l, m_temp.b.h))
			goto ILLEGAL;
		eat(1);
		if (UNEXPECTED(m_icount <= 0)) { push_state(209); return; }
state_209:

		m_w.w--;
	}

	// Not sure if this sub instruction timing is accurate either
	eat(3);
	return;

COM16:
	m_temp.b.h = read_operand_ord(0);
	m_temp.b.l = read_operand_ord(1);
	m_cc &= ~CC_V;
	m_cc |= CC_C;
	m_temp.w = set_flags(CC_NZ, (UINT16) ~m_temp.w);
	eat(hd6309_native_mode() ? 0 : 1);
	write_operand_ord(0, m_temp.b.h);
	write_operand_ord(1, m_temp.b.l);
	return;

ADC16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(210); return; }
state_210:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(211); return; }
state_211:
	regop16().w = set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w + m_temp.w + (m_cc & CC_C ? 1 : 0));
	eat(hd6309_native_mode() ? 0 : 1);
	return;

SBC16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(212); return; }
state_212:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(213); return; }
state_213:
	regop16().w = set_flags(CC_NZVC, regop16().w, m_temp.w, regop16().w - m_temp.w - (m_cc & CC_C ? 1 : 0));
	eat(hd6309_native_mode() ? 0 : 1);
	return;

AND16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(214); return; }
state_214:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(215); return; }
state_215:
	m_cc &= ~CC_V;
	regop16().w = set_flags(CC_NZ, (UINT16)0, regop16().w, regop16().w & m_temp.w);
	return;

BIT16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(216); return; }
state_216:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(217); return; }
state_217:
	m_cc &= ~CC_V;
	set_flags(CC_NZ, (UINT16)0, regop16().w, regop16().w & m_temp.w);
	return;

OR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(218); return; }
state_218:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(219); return; }
state_219:
	m_cc &= ~CC_V;
	regop16().w = set_flags(CC_NZ, (UINT16)0, regop16().w, regop16().w | m_temp.w);
	return;

EOR16:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(220); return; }
state_220:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(221); return; }
state_221:
	m_cc &= ~CC_V;
	regop16().w = set_flags(CC_NZ, (UINT16)0, regop16().w, regop16().w ^ m_temp.w);
	return;

PSHSW:
	m_temp.w = 0x300;	// PSHS W
	eat(2);
	set_regop16(m_s);
	goto PUSH_REGISTERS;

PULSW:
	m_temp.w = 0x300;	// PULS W
	eat(2);
	set_regop16(m_s);
	goto PULL_REGISTERS;

PSHUW:
	m_temp.w = 0x300;	// PSHU W
	eat(2);
	set_regop16(m_u);
	goto PUSH_REGISTERS;

PULUW:
	m_temp.w = 0x300;	// PULU W
	eat(2);
	set_regop16(m_u);
	goto PULL_REGISTERS;

BAND:
	push_state(222);
	goto IMM_IM;
state_222:
	push_state(223);
	goto DIRECT;
state_223:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(224); return; }
state_224:
	bittest_set(bittest_dest() && bittest_source());
	return;

BIAND:
	push_state(225);
	goto IMM_IM;
state_225:
	push_state(226);
	goto DIRECT;
state_226:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(227); return; }
state_227:
	bittest_set(bittest_dest() && !bittest_source());
	return;

BOR:
	push_state(228);
	goto IMM_IM;
state_228:
	push_state(229);
	goto DIRECT;
state_229:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(230); return; }
state_230:
	bittest_set(bittest_dest() || bittest_source());
	return;

BIOR:
	push_state(231);
	goto IMM_IM;
state_231:
	push_state(232);
	goto DIRECT;
state_232:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(233); return; }
state_233:
	bittest_set(bittest_dest() || !bittest_source());
	return;

BEOR:
	push_state(234);
	goto IMM_IM;
state_234:
	push_state(235);
	goto DIRECT;
state_235:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(236); return; }
state_236:
	bittest_set(bittest_dest() != bittest_source());
	return;

BIEOR:
	push_state(237);
	goto IMM_IM;
state_237:
	push_state(238);
	goto DIRECT;
state_238:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(239); return; }
state_239:
	bittest_set(bittest_dest() != !bittest_source());
	return;

LDBT:
	push_state(240);
	goto IMM_IM;
state_240:
	push_state(241);
	goto DIRECT;
state_241:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(242); return; }
state_242:
	bittest_set(bittest_source());
	return;

STBT:
	push_state(243);
	goto IMM_IM;
state_243:
	push_state(244);
	goto DIRECT;
state_244:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(245); return; }
state_245:
	if (bittest_source())
		m_temp.b.l |= (1 << ((m_temp_im >> 0) & 0x07));
	else
		m_temp.b.l &= ~(1 << ((m_temp_im >> 0) & 0x07));
	eat(2);
	if (UNEXPECTED(m_icount <= 0)) { push_state(246); return; }
state_246:
	write_operand(m_temp.b.l);
	return;

MULD:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(247); return; }
state_247:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(248); return; }
state_248:
	muld();
	return;

DIVQ:
	m_temp.b.h = read_operand_ord(0);
	if (UNEXPECTED(m_icount <= 0)) { push_state(249); return; }
state_249:
	m_temp.b.l = read_operand_ord(1);
	if (UNEXPECTED(m_icount <= 0)) { push_state(250); return; }
state_250:
	if (!divq())
		goto DIVIDE_BY_ZERO;
	return;

DIVD:
	m_temp.b.l = read_operand();
	if (UNEXPECTED(m_icount <= 0)) { push_state(251); return; }
state_251:
	if (!divd())
		goto DIVIDE_BY_ZERO;
	return;

SEXW:
	m_d.w = set_flags16(CC_N, (m_w.w & 0x8000) ? 0xFFFF : 0x0000);
	if ((m_d.w == 0x0000) && (m_w.w == 0x0000))
		m_cc |= CC_Z;
	else
		m_cc &= ~CC_Z;
	return;

	}
}
