// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    konamtbl.c

    Portable Konami CPU emulator - Inlines and tables

*********************************************************************/

#define m_pc (konami.pc)
#define m_ppc (konami.ppc)
#define m_d (konami.d)
#define m_u (konami.u)
#define m_s (konami.s)
#define m_x (konami.x)
#define m_y (konami.y)
#define m_dp (konami.dp)
#define m_cc (konami.cc)
#define m_temp (konami.temp)
#define m_opcode (konami.opcode)
#define m_ea (konami.ea)
#define m_addressing_mode (konami.addressing_mode)
#define m_reg (konami.reg)
#define m_cond (konami.cond)
#define m_state (konami.state)
#define m_nmi_asserted (konami.nmi_asserted)
#define m_nmi_line (konami.nmi_line)
#define m_firq_line (konami.firq_line)
#define m_irq_line (konami.irq_line)
#define m_lds_encountered (konami.lds_encountered)
#define m_reg8 (konami.reg8)
#define m_reg16 (konami.reg16)
#define m_icount (konami_ICount)

#define M6809_FIRQ_LINE KONAMI_FIRQ_LINE
#define M6809_IRQ_LINE  KONAMI_IRQ_LINE

enum
{
	CC_C = 0x01,
	CC_V = 0x02,
	CC_Z = 0x04,
	CC_N = 0x08,
	CC_I = 0x10,
	CC_H = 0x20,
	CC_F = 0x40,
	CC_E = 0x80,
	CC_ZC = CC_Z | CC_C,
	CC_VC = CC_V | CC_C,
	CC_NZ = CC_N | CC_Z,
	CC_NZV = CC_N | CC_Z | CC_V,
	CC_NZC = CC_N | CC_Z | CC_C,
	CC_NZVC = CC_N | CC_Z | CC_V | CC_C,
	CC_HNZVC = CC_H | CC_N | CC_Z | CC_V | CC_C
};

enum
{
	ADDRESSING_MODE_IMMEDIATE = 0,
	ADDRESSING_MODE_EA,
	ADDRESSING_MODE_REGISTER_A,
	ADDRESSING_MODE_REGISTER_B,
	ADDRESSING_MODE_REGISTER_D
};

enum
{
	VECTOR_NEG1          = -1,
	VECTOR_SWI3          = 0xFFF2,
	VECTOR_SWI2          = 0xFFF4,
	VECTOR_FIRQ          = 0xFFF6,
	VECTOR_IRQ           = 0xFFF8,
	VECTOR_SWI           = 0xFFFA,
	VECTOR_NMI           = 0xFFFC,
	VECTOR_RESET_FFFE   = 0xFFFE
};

#define push_state(st) do { m_state = (m_state << 8) | (UINT8)(st); } while (0)
static inline UINT8 pop_state(void)
{
	UINT8 result = (UINT8)m_state;
	m_state >>= 8;
	return result;
}
#define reset_state() do { m_state = 0; } while (0)

#define eat(cycles) do { m_icount -= (cycles); } while (0)

static inline void eat_remaining(void)
{
	UINT16 real_pc = m_pc.w;
	eat(m_icount);
	m_pc.w = m_ppc.w;
	CALL_MAME_DEBUG;
	m_pc.w = real_pc;
}

static inline UINT8 read_memory(UINT16 address)
{
	eat(1);
	return program_read_byte_8(address);
}

static inline void write_memory(UINT16 address, UINT8 data)
{
	eat(1);
	program_write_byte_8(address, data);
}

static inline UINT8 read_opcode_next(void)
{
	eat(1);
	return cpu_readop(m_pc.w++);
}

static inline UINT8 read_opcode_arg_next(void)
{
	eat(1);
	return cpu_readop_arg(m_pc.w++);
}

static inline UINT8 read_operand(void)
{
	switch (m_addressing_mode)
	{
		case ADDRESSING_MODE_EA:            return read_memory(m_ea.w);
		case ADDRESSING_MODE_IMMEDIATE:     return read_opcode_arg_next();
		case ADDRESSING_MODE_REGISTER_A:    return m_d.b.h;
		case ADDRESSING_MODE_REGISTER_B:    return m_d.b.l;
		default:                            fatalerror("Unexpected addressing mode"); return 0x00;
	}
}

static inline UINT8 read_operand_ord(int ordinal)
{
	switch (m_addressing_mode)
	{
		case ADDRESSING_MODE_EA:            return read_memory(m_ea.w + ordinal);
		case ADDRESSING_MODE_IMMEDIATE:     return read_opcode_arg_next();
		case ADDRESSING_MODE_REGISTER_D:    return (ordinal & 1) ? m_d.b.l : m_d.b.h;
		default:                            fatalerror("Unexpected addressing mode"); return 0x00;
	}
}

static inline void write_operand(UINT8 data)
{
	switch (m_addressing_mode)
	{
		case ADDRESSING_MODE_IMMEDIATE:     /* do nothing */ break;
		case ADDRESSING_MODE_EA:            write_memory(m_ea.w, data); break;
		case ADDRESSING_MODE_REGISTER_A:    m_d.b.h = data; break;
		case ADDRESSING_MODE_REGISTER_B:    m_d.b.l = data; break;
		default:                            fatalerror("Unexpected addressing mode"); break;
	}
}

static inline void write_operand_ord(int ordinal, UINT8 data)
{
	switch (m_addressing_mode)
	{
		case ADDRESSING_MODE_IMMEDIATE:     /* do nothing */ break;
		case ADDRESSING_MODE_EA:            write_memory(m_ea.w + ordinal, data); break;
		case ADDRESSING_MODE_REGISTER_D:    *((ordinal & 1) ? &m_d.b.l : &m_d.b.h) = data; break;
		default:                            fatalerror("Unexpected addressing mode"); break;
	}
}

#define set_a()   do { m_addressing_mode = ADDRESSING_MODE_REGISTER_A; } while (0)
#define set_b()   do { m_addressing_mode = ADDRESSING_MODE_REGISTER_B; } while (0)
#define set_d()   do { m_addressing_mode = ADDRESSING_MODE_REGISTER_D; } while (0)
#define set_imm() do { m_addressing_mode = ADDRESSING_MODE_IMMEDIATE; } while (0)

#define read_ea()            read_memory(m_ea.w)
#define write_ea(data)       write_memory(m_ea.w, data)
#define set_ea(ea)           do { m_ea.w = (ea); m_addressing_mode = ADDRESSING_MODE_EA; } while (0)
#define set_ea_h(ea_h)       do { m_ea.b.h = (ea_h); } while (0)
#define set_ea_l(ea_l)       do { m_ea.b.l = (ea_l); m_addressing_mode = ADDRESSING_MODE_EA; } while (0)

#define set_regop8(r)   do { m_reg8 = &(r); m_reg16 = NULL; } while (0)
#define set_regop16(r)  do { m_reg16 = &(r); m_reg8 = NULL; } while (0)
#define regop8()        (*m_reg8)
#define regop16()       (*m_reg16)

static inline UINT16 *konami_get_ireg(void)
{
	switch (m_opcode & 0x70)
	{
		case 0x20: return &m_x.w;
		case 0x30: return &m_y.w;
		case 0x50: return &m_u.w;
		case 0x60: return &m_s.w;
		case 0x70: return &m_pc.w;
		default:   fatalerror("Unexpected konami ireg"); return &m_x.w;
	}
}
#define ireg() (*konami_get_ireg())

static inline UINT8 set_flags8_full(UINT8 mask, UINT8 a, UINT8 b, UINT32 r)
{
	m_cc &= ~mask;
	if (mask & CC_H)
		m_cc |= ((a ^ b ^ r) & 0x10) ? CC_H : 0;
	if (mask & CC_N)
		m_cc |= (r & 0x80) ? CC_N : 0;
	if (mask & CC_Z)
		m_cc |= (((UINT8)r) == 0) ? CC_Z : 0;
	if (mask & CC_V)
		m_cc |= ((a ^ b ^ r ^ (r >> 1)) & 0x80) ? CC_V : 0;
	if (mask & CC_C)
		m_cc |= (r & 0x100) ? CC_C : 0;
	return (UINT8)r;
}

static inline UINT16 set_flags16_full(UINT8 mask, UINT16 a, UINT16 b, UINT32 r)
{
	m_cc &= ~mask;
	if (mask & CC_H)
		m_cc |= ((a ^ b ^ r) & 0x10) ? CC_H : 0;
	if (mask & CC_N)
		m_cc |= (r & 0x8000) ? CC_N : 0;
	if (mask & CC_Z)
		m_cc |= (((UINT16)r) == 0) ? CC_Z : 0;
	if (mask & CC_V)
		m_cc |= ((a ^ b ^ r ^ (r >> 1)) & 0x8000) ? CC_V : 0;
	if (mask & CC_C)
		m_cc |= (r & 0x10000) ? CC_C : 0;
	return (UINT16)r;
}

static inline UINT32 set_flags32_full(UINT8 mask, UINT32 a, UINT32 b, UINT64 r)
{
	m_cc &= ~mask;
	if (mask & CC_H)
		m_cc |= ((a ^ b ^ r) & 0x10) ? CC_H : 0;
	if (mask & CC_N)
		m_cc |= (r & 0x80000000U) ? CC_N : 0;
	if (mask & CC_Z)
		m_cc |= (((UINT32)r) == 0) ? CC_Z : 0;
	if (mask & CC_V)
		m_cc |= ((a ^ b ^ r ^ (r >> 1)) & 0x80000000U) ? CC_V : 0;
	if (mask & CC_C)
		m_cc |= (r & 0x100000000ULL) ? CC_C : 0;
	return (UINT32)r;
}

static inline UINT8 set_flags8_2(UINT8 mask, UINT8 r)
{
	return set_flags8_full(mask, 0, r, r);
}

static inline UINT16 set_flags16_2(UINT8 mask, UINT16 r)
{
	return set_flags16_full(mask, 0, r, r);
}

static inline UINT32 set_flags32_2(UINT8 mask, UINT32 r)
{
	return set_flags32_full(mask, 0, r, r);
}

#define GET_SET_FLAGS_MACRO(_1, _2, _3, _4, NAME, ...) NAME
#define set_flags8(...)  GET_SET_FLAGS_MACRO(__VA_ARGS__, set_flags8_full,  unused, set_flags8_2)(__VA_ARGS__)
#define set_flags16(...) GET_SET_FLAGS_MACRO(__VA_ARGS__, set_flags16_full, unused, set_flags16_2)(__VA_ARGS__)
#define set_flags32(...) GET_SET_FLAGS_MACRO(__VA_ARGS__, set_flags32_full, unused, set_flags32_2)(__VA_ARGS__)

#define set_flags_4(mask, a, b, r) 	(sizeof(a) == 1 ? set_flags8_full((mask), (a), (b), (r)) : 	 sizeof(a) == 2 ? set_flags16_full((mask), (a), (b), (r)) : 	 set_flags32_full((mask), (a), (b), (r)))

#define set_flags_2(mask, r) 	(sizeof(r) == 1 ? set_flags8_2((mask), (r)) : 	 sizeof(r) == 2 ? set_flags16_2((mask), (r)) : 	 set_flags32_2((mask), (r)))

#define set_flags(...) GET_SET_FLAGS_MACRO(__VA_ARGS__, set_flags_4, unused, set_flags_2)(__VA_ARGS__)

static inline UINT8 rotate_right8(UINT8 value)
{
	UINT8 new_carry = (value & 1);
	value = value >> 1;
	if (m_cc & CC_C) value |= 0x80;
	else value &= ~0x80;
	if (new_carry) m_cc |= CC_C;
	else m_cc &= ~CC_C;
	return value;
}

static inline UINT16 rotate_right16(UINT16 value)
{
	UINT8 new_carry = (value & 1);
	value = value >> 1;
	if (m_cc & CC_C) value |= 0x8000;
	else value &= ~0x8000;
	if (new_carry) m_cc |= CC_C;
	else m_cc &= ~CC_C;
	return value;
}

static inline UINT32 rotate_left8(UINT8 value)
{
	UINT8 new_carry = (value & 0x80);
	UINT32 new_value = (UINT32)value << 1;
	if (m_cc & CC_C) new_value |= 1;
	else new_value &= ~1;
	if (new_carry) m_cc |= CC_C;
	else m_cc &= ~CC_C;
	return new_value;
}

static inline UINT32 rotate_left16(UINT16 value)
{
	UINT8 new_carry = (value & 0x8000) ? 1 : 0;
	UINT32 new_value = (UINT32)value << 1;
	if (m_cc & CC_C) new_value |= 1;
	else new_value &= ~1;
	if (new_carry) m_cc |= CC_C;
	else m_cc &= ~CC_C;
	return new_value;
}

#define rotate_right(val) (sizeof(val) == 1 ? rotate_right8(val) : rotate_right16(val))
#define rotate_left(val)  (sizeof(val) == 1 ? rotate_left8(val) : rotate_left16(val))

static inline void daa(void)
{
	UINT16 t, cf = 0;
	UINT8 msn = m_d.b.h & 0xF0;
	UINT8 lsn = m_d.b.h & 0x0F;

	if (lsn > 0x09 || (m_cc & CC_H)) cf |= 0x06;
	if (msn > 0x80 && lsn > 0x09)    cf |= 0x60;
	if (msn > 0x90 || (m_cc & CC_C)) cf |= 0x60;

	t = m_d.b.h + cf;

	m_cc &= ~CC_V;
	if (t & 0x0100)
		m_cc |= CC_C;

	m_d.b.h = set_flags8(CC_NZ, (UINT8)t);
}

static inline void mul(void)
{
	UINT16 result = ((UINT16)m_d.b.h) * ((UINT16)m_d.b.l);
	m_d.w = set_flags16(CC_Z, result);
	if (m_d.w & 0x0080)
		m_cc |= CC_C;
	else
		m_cc &= ~CC_C;
}

static inline void lmul(void)
{
	PAIR result;
	result.d = (UINT32)m_x.w * m_y.w;
	m_x.w = result.w.h;
	m_y.w = result.w.l;
	set_flags32(CC_Z, result.d);
	if (result.d & 0x8000)
		m_cc |= CC_C;
	else
		m_cc &= ~CC_C;
}

static inline void divx(void)
{
	UINT16 result;
	UINT8 remainder;
	if (m_d.b.l != 0)
	{
		result = m_x.w / m_d.b.l;
		remainder = m_x.w % m_d.b.l;
	}
	else
	{
		result = 0;
		remainder = 0;
	}
	m_x.w = set_flags16(CC_Z, result);
	m_d.b.l = remainder;
	if (result & 0x0080)
		m_cc |= CC_C;
	else
		m_cc &= ~CC_C;
}

static inline UINT16 safe_shift_right_unsigned16(UINT16 value, UINT32 shift)
{
	return (shift < 16) ? (value >> shift) : 0;
}

static inline INT16 safe_shift_right16(INT16 value, UINT32 shift)
{
	if (shift < 16)
		return value >> shift;
	else if (value < 0)
		return (INT16)-1;
	else
		return 0;
}

static inline INT16 safe_shift_left16(INT16 value, UINT32 shift)
{
	return (shift < 16) ? (value << shift) : 0;
}

static inline UINT32 safe_shift_left32(UINT32 value, UINT32 shift)
{
	return (shift < 32) ? (value << shift) : 0;
}

static inline UINT32 safe_shift_right32(UINT32 value, UINT32 shift)
{
	return (shift < 32) ? (value >> shift) : 0;
}

#define safe_shift_left(val, shift)  safe_shift_left32((val), (shift))
#define safe_shift_right(val, shift) safe_shift_right32((val), (shift))


static inline void set_lines(UINT8 data)
{
	if (konami.setlines_callback)
	{
		(*konami.setlines_callback)(data);
		change_pc(m_pc.w);
	}
}

#define cond_hi()   (!((m_cc & CC_ZC)))
#define cond_cc()   (!(m_cc & CC_C))
#define cond_ne()   (!(m_cc & CC_Z))
#define cond_vc()   (!(m_cc & CC_V))
#define cond_pl()   (!(m_cc & CC_N))
#define cond_ge()   (((m_cc & CC_N) ? 1 : 0) == ((m_cc & CC_V) ? 1 : 0))
#define cond_gt()   (cond_ge() && !(m_cc & CC_Z))
#define set_cond(c) do { m_cond = (c); } while (0)
#define branch_taken() (m_cond)

#define is_register_addressing_mode() ((m_addressing_mode != ADDRESSING_MODE_IMMEDIATE) && (m_addressing_mode != ADDRESSING_MODE_EA))
#define nop() do { } while (0)
#define m_lic_func(x) do { } while (0)
#define firq_saves_entire_state() 0
#define entire_state_registers() 0xFF
#define partial_state_registers() 0x81
#define add8_sets_h() 1
#define hd6309_native_mode() 0
#define is_register_register_op_16_bit() 0
#define log_illegal() logerror("illegal opcode at %04x\n", (unsigned)m_pc.w)
#ifndef UNEXPECTED
#define UNEXPECTED(x) __builtin_expect((x), 0)
#endif

#define standard_irq_callback(line) do { if (konami.irq_callback) (*konami.irq_callback)(line); } while (0)

static inline UINT16 get_pending_interrupt(void)
{
	if (m_nmi_asserted)
		return VECTOR_NMI;
	else if (!(m_cc & CC_F) && m_firq_line)
		return VECTOR_FIRQ;
	else if (!(m_cc & CC_I) && m_irq_line)
		return VECTOR_IRQ;
	else
		return 0;
}

static inline konami_exgtfr_register read_exgtfr_register(UINT8 reg)
{
	konami_exgtfr_register result;
	result.word_value = 0x00FF;

	switch (reg & 0x07)
	{
		case  0: result.word_value = m_d.b.h;   break;
		case  1: result.word_value = m_d.b.l;   break;
		case  2: result.word_value = m_x.w;     break;
		case  3: result.word_value = m_y.w;     break;
		case  4: result.word_value = m_s.w;     break;
		case  5: result.word_value = m_u.w;     break;
	}
	result.byte_value = (UINT8) result.word_value;
	return result;
}

static inline void write_exgtfr_register(UINT8 reg, konami_exgtfr_register value)
{
	switch (reg & 0x07)
	{
		case  0: m_d.b.h = value.byte_value;    break;
		case  1: m_d.b.l = value.byte_value;    break;
		case  2: m_x.w   = value.word_value;    break;
		case  3: m_y.w   = value.word_value;    break;
		case  4: m_s.w   = value.word_value;    break;
		case  5: m_u.w   = value.word_value;    break;
	}
}
