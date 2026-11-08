// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/*********************************************************************

    6309tbl.c

    Portable 6309 emulator - Inlines and tables

*********************************************************************/

#define m_pc (hd6309.pc)
#define m_ppc (hd6309.ppc)
#define m_d (hd6309.d)
#define m_u (hd6309.u)
#define m_s (hd6309.s)
#define m_x (hd6309.x)
#define m_y (hd6309.y)
#define m_dp (hd6309.dp)
#define m_cc (hd6309.cc)
#define m_temp (hd6309.temp)
#define m_opcode (hd6309.opcode)
#define m_ea (hd6309.ea)
#define m_addressing_mode (hd6309.addressing_mode)
#define m_reg (hd6309.reg)
#define m_cond (hd6309.cond)
#define m_state (hd6309.state)
#define m_nmi_asserted (hd6309.nmi_asserted)
#define m_nmi_line (hd6309.nmi_line)
#define m_firq_line (hd6309.firq_line)
#define m_irq_line (hd6309.irq_line)
#define m_lds_encountered (hd6309.lds_encountered)
#define m_w (hd6309.w)
#define m_v (hd6309.v)
#define m_md (hd6309.md)
#define m_temp_im (hd6309.temp_im)
#define m_reg8 (hd6309.reg8)
#define m_reg16 (hd6309.reg16)
#define m_icount (hd6309_ICount)

#define M6809_FIRQ_LINE HD6309_FIRQ_LINE
#define M6809_IRQ_LINE  HD6309_IRQ_LINE

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
	ADDRESSING_MODE_REGISTER_D,
	ADDRESSING_MODE_REGISTER_X,
	ADDRESSING_MODE_REGISTER_Y,
	ADDRESSING_MODE_REGISTER_U,
	ADDRESSING_MODE_REGISTER_S,
	ADDRESSING_MODE_REGISTER_PC,
	ADDRESSING_MODE_REGISTER_W,
	ADDRESSING_MODE_REGISTER_V,
	ADDRESSING_MODE_REGISTER_CC,
	ADDRESSING_MODE_REGISTER_DP,
	ADDRESSING_MODE_REGISTER_E,
	ADDRESSING_MODE_REGISTER_F,
	ADDRESSING_MODE_ZERO
};

enum
{
	VECTOR_TRAP          = 0xFFF0,
	VECTOR_ILLEGAL       = 0xFFF0,
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
		case ADDRESSING_MODE_REGISTER_E:    return m_w.b.h;
		case ADDRESSING_MODE_REGISTER_F:    return m_w.b.l;
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
		case ADDRESSING_MODE_REGISTER_W:    return (ordinal & 1) ? m_w.b.l : m_w.b.h;
		case ADDRESSING_MODE_REGISTER_X:    return (ordinal & 1) ? m_x.b.l : m_x.b.h;
		case ADDRESSING_MODE_REGISTER_Y:    return (ordinal & 1) ? m_y.b.l : m_y.b.h;
		case ADDRESSING_MODE_REGISTER_U:    return (ordinal & 1) ? m_u.b.l : m_u.b.h;
		case ADDRESSING_MODE_REGISTER_S:    return (ordinal & 1) ? m_s.b.l : m_s.b.h;
		case ADDRESSING_MODE_REGISTER_V:    return (ordinal & 1) ? m_v.b.l : m_v.b.h;
		case ADDRESSING_MODE_REGISTER_PC:   return (ordinal & 1) ? m_pc.b.l : m_pc.b.h;
		case ADDRESSING_MODE_ZERO:          return 0x00;
		default:                            fatalerror("Unexpected addressing mode"); return 0x00;
	}
}

static inline void write_operand(UINT8 data)
{
	switch (m_addressing_mode)
	{
		case ADDRESSING_MODE_EA:            write_memory(m_ea.w, data); break;
		case ADDRESSING_MODE_REGISTER_A:    m_d.b.h = data; break;
		case ADDRESSING_MODE_REGISTER_B:    m_d.b.l = data; break;
		case ADDRESSING_MODE_REGISTER_E:    m_w.b.h = data; break;
		case ADDRESSING_MODE_REGISTER_F:    m_w.b.l = data; break;
		case ADDRESSING_MODE_ZERO:          break;
		default:                            fatalerror("Unexpected addressing mode"); break;
	}
}

static inline void write_operand_ord(int ordinal, UINT8 data)
{
	switch (m_addressing_mode)
	{
		case ADDRESSING_MODE_EA:            write_memory(m_ea.w + ordinal, data); break;
		case ADDRESSING_MODE_REGISTER_D:    *((ordinal & 1) ? &m_d.b.l : &m_d.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_W:    *((ordinal & 1) ? &m_w.b.l : &m_w.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_X:    *((ordinal & 1) ? &m_x.b.l : &m_x.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_Y:    *((ordinal & 1) ? &m_y.b.l : &m_y.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_U:    *((ordinal & 1) ? &m_u.b.l : &m_u.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_S:    *((ordinal & 1) ? &m_s.b.l : &m_s.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_V:    *((ordinal & 1) ? &m_v.b.l : &m_v.b.h) = data; break;
		case ADDRESSING_MODE_REGISTER_PC:   *((ordinal & 1) ? &m_pc.b.l : &m_pc.b.h) = data; change_pc(m_pc.w); break;
		case ADDRESSING_MODE_ZERO:          break;
		default:                            fatalerror("Unexpected addressing mode"); break;
	}
}

#define set_a()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_A; } while (0)
#define set_b()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_B; } while (0)
#define set_d()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_D; } while (0)
#define set_e()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_E; } while (0)
#define set_f()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_F; } while (0)
#define set_w()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_W; } while (0)
#define set_v()    do { m_addressing_mode = ADDRESSING_MODE_REGISTER_V; } while (0)
#define set_cc()   do { m_addressing_mode = ADDRESSING_MODE_REGISTER_CC; } while (0)
#define set_dp()   do { m_addressing_mode = ADDRESSING_MODE_REGISTER_DP; } while (0)
#define set_zero() do { m_addressing_mode = ADDRESSING_MODE_ZERO; } while (0)
#define set_imm()  do { m_addressing_mode = ADDRESSING_MODE_IMMEDIATE; } while (0)

#define read_ea()            read_memory(m_ea.w)
#define write_ea(data)       write_memory(m_ea.w, data)
#define set_ea(ea)           do { m_ea.w = (ea); m_addressing_mode = ADDRESSING_MODE_EA; } while (0)
#define set_ea_h(ea_h)       do { m_ea.b.h = (ea_h); } while (0)
#define set_ea_l(ea_l)       do { m_ea.b.l = (ea_l); m_addressing_mode = ADDRESSING_MODE_EA; } while (0)

#define set_regop8(r)   do { m_reg8 = &(r); m_reg16 = NULL; } while (0)
#define set_regop16(r)  do { m_reg16 = &(r); m_reg8 = NULL; } while (0)
#define regop8()        (*m_reg8)
#define regop16()       (*m_reg16)

static inline UINT16 *hd6309_get_ireg(void)
{
	switch (m_opcode & 0x60)
	{
		case 0x00: return &m_x.w;
		case 0x20: return &m_y.w;
		case 0x40: return &m_u.w;
		case 0x60: return &m_s.w;
		default:   fatalerror("Unexpected ireg"); return &m_x.w;
	}
}
#define ireg() (*hd6309_get_ireg())

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
#define firq_saves_entire_state() ((m_md & 1) != 0)
#define entire_state_registers() 0xFF
#define partial_state_registers() 0x81
#define add8_sets_h() 1
#define hd6309_native_mode() ((m_md & 1) != 0)
#define is_register_register_op_16_bit() (m_reg16 != NULL)
#define log_illegal() logerror("illegal opcode at %04x\n", (unsigned)m_pc.w)
#ifndef UNEXPECTED
#define UNEXPECTED(x) __builtin_expect((x), 0)
#endif

#define standard_irq_callback(line) do { if (hd6309.irq_callback) (*hd6309.irq_callback)(line); } while (0)

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

static inline hd6309_exgtfr_register read_exgtfr_register(UINT8 reg)
{
	UINT16 value;

	switch (reg & 0x0F)
	{
		case  0: value = m_d.w;                             break;
		case  1: value = m_x.w;                             break;
		case  2: value = m_y.w;                             break;
		case  3: value = m_u.w;                             break;
		case  4: value = m_s.w;                             break;
		case  5: value = m_pc.w;                            break;
		case  6: value = m_w.w;                             break;
		case  7: value = m_v.w;                             break;
		case  8: value = ((UINT16) m_d.b.h) << 8 | m_d.b.h; break;
		case  9: value = ((UINT16) m_d.b.l) << 8 | m_d.b.l; break;
		case 10: value = ((UINT16) m_cc) << 8 | m_cc;       break;
		case 11: value = ((UINT16) m_dp) << 8 | m_dp;       break;
		case 12: value = 0;                                 break;
		case 13: value = 0;                                 break;
		case 14: value = ((UINT16) m_w.b.h) << 8 | m_w.b.h; break;
		case 15: value = ((UINT16) m_w.b.l) << 8 | m_w.b.l; break;
		default: fatalerror("Should not reach here"); value = 0; break;
	}

	hd6309_exgtfr_register result;
	result.byte_value = (UINT8)value;
	result.word_value = value;
	return result;
}

static inline void write_exgtfr_register(UINT8 reg, hd6309_exgtfr_register value)
{
	switch (reg & 0x0F)
	{
		case  0: m_d.w   = value.word_value;                break;
		case  1: m_x.w   = value.word_value;                break;
		case  2: m_y.w   = value.word_value;                break;
		case  3: m_u.w   = value.word_value;                break;
		case  4: m_s.w   = value.word_value;                break;
		case  5: m_pc.w  = value.word_value; change_pc(m_pc.w); break;
		case  6: m_w.w   = value.word_value;                break;
		case  7: m_v.w   = value.word_value;                break;
		case  8: m_d.b.h = (UINT8) (value.word_value >> 8); break;
		case  9: m_d.b.l = (UINT8) (value.word_value >> 0); break;
		case 10: m_cc    = (UINT8) (value.word_value >> 0); break;
		case 11: m_dp    = (UINT8) (value.word_value >> 8); break;
		case 12:                                            break;
		case 13:                                            break;
		case 14: m_w.b.h = (UINT8) (value.word_value >> 8); break;
		case 15: m_w.b.l = (UINT8) (value.word_value >> 0); break;
		default: fatalerror("Should not reach here");       break;
	}
}

static inline UINT8 *bittest_register(void)
{
	switch (m_temp_im & 0xC0)
	{
		case 0x00: return &m_cc;
		case 0x40: return &m_d.b.h;
		case 0x80: return &m_d.b.l;
		default:   return &m_temp.b.l;
	}
}

static inline bool bittest_source(void)
{
	return (m_temp.b.l & (1 << ((m_temp_im >> 3) & 0x07))) ? true : false;
}

static inline bool bittest_dest(void)
{
	return (*bittest_register() & (1 << ((m_temp_im >> 0) & 0x07))) ? true : false;
}

static inline void bittest_set(bool result)
{
	if (result)
		*bittest_register() |= (1 << ((m_temp_im >> 0) & 0x07));
	else
		*bittest_register() &= ~(1 << ((m_temp_im >> 0) & 0x07));
	eat(4);
}

static inline bool hd6309_tfr_read(UINT8 opcode, UINT8 arg, UINT8 *data)
{
	PAIR16 *reg;
	switch (arg & 0xF0)
	{
		case 0x00: reg = &m_d; break;
		case 0x10: reg = &m_x; break;
		case 0x20: reg = &m_y; break;
		case 0x30: reg = &m_u; break;
		case 0x40: reg = &m_s; break;
		default:   return false;
	}

	*data = read_memory(reg->w);

	switch (opcode & 0x03)
	{
		case 0x00: reg->w++; break;
		case 0x01: reg->w--; break;
		case 0x02: reg->w++; break;
		case 0x03: break;
	}
	return true;
}
#define tfr_read(op, arg, data) hd6309_tfr_read((op), (arg), &(data))

static inline bool hd6309_tfr_write(UINT8 opcode, UINT8 arg, UINT8 data)
{
	PAIR16 *reg;
	switch (arg & 0x0F)
	{
		case 0x00: reg = &m_d; break;
		case 0x01: reg = &m_x; break;
		case 0x02: reg = &m_y; break;
		case 0x03: reg = &m_u; break;
		case 0x04: reg = &m_s; break;
		default:   return false;
	}

	write_memory(reg->w, data);

	switch (opcode & 0x03)
	{
		case 0x00: reg->w++; break;
		case 0x01: reg->w--; break;
		case 0x02: break;
		case 0x03: reg->w++; break;
	}
	return true;
}
#define tfr_write(op, arg, data) hd6309_tfr_write((op), (arg), (data))

static inline void register_register_op(void)
{
	UINT8 operand = read_opcode_arg_next();
	bool promote = ((operand & 0x80) ? true : false) != ((operand & 0x08) ? true : false);

	m_temp.w = 0;

	switch ((operand >> 0) & 0x0F)
	{
		case  0: set_regop16(m_d);                                            break;
		case  1: set_regop16(m_x);                                            break;
		case  2: set_regop16(m_y);                                            break;
		case  3: set_regop16(m_u);                                            break;
		case  4: set_regop16(m_s);                                            break;
		case  5: set_regop16(m_pc);                                           break;
		case  6: set_regop16(m_w);                                            break;
		case  7: set_regop16(m_v);                                            break;
		case  8: if (promote) set_regop16(m_d);    else set_regop8(m_d.b.h);  break;
		case  9: if (promote) set_regop16(m_d);    else set_regop8(m_d.b.l);  break;
		case 10: if (promote) set_regop16(m_temp); else set_regop8(m_cc);     break;
		case 11: if (promote) set_regop16(m_temp); else set_regop8(m_dp);     break;
		case 12: if (promote) set_regop16(m_temp); else set_regop8(m_temp.b.l); break;
		case 13: if (promote) set_regop16(m_temp); else set_regop8(m_temp.b.l); break;
		case 14: if (promote) set_regop16(m_w);    else set_regop8(m_w.b.h);  break;
		case 15: if (promote) set_regop16(m_w);    else set_regop8(m_w.b.l);  break;
		default: fatalerror("Should not reach here"); break;
	}

	switch ((operand >> 4) & 0x0F)
	{
		case  0: m_addressing_mode = ADDRESSING_MODE_REGISTER_D;                                        break;
		case  1: m_addressing_mode = ADDRESSING_MODE_REGISTER_X;                                        break;
		case  2: m_addressing_mode = ADDRESSING_MODE_REGISTER_Y;                                        break;
		case  3: m_addressing_mode = ADDRESSING_MODE_REGISTER_U;                                        break;
		case  4: m_addressing_mode = ADDRESSING_MODE_REGISTER_S;                                        break;
		case  5: m_addressing_mode = ADDRESSING_MODE_REGISTER_PC;                                       break;
		case  6: m_addressing_mode = ADDRESSING_MODE_REGISTER_W;                                        break;
		case  7: m_addressing_mode = ADDRESSING_MODE_REGISTER_V;                                        break;
		case  8: m_addressing_mode = promote ? ADDRESSING_MODE_REGISTER_D : ADDRESSING_MODE_REGISTER_A; break;
		case  9: m_addressing_mode = promote ? ADDRESSING_MODE_REGISTER_D : ADDRESSING_MODE_REGISTER_B; break;
		case 10: m_addressing_mode = promote ? ADDRESSING_MODE_ZERO : ADDRESSING_MODE_REGISTER_CC;      break;
		case 11: m_addressing_mode = promote ? ADDRESSING_MODE_ZERO : ADDRESSING_MODE_REGISTER_DP;      break;
		case 12: m_addressing_mode = ADDRESSING_MODE_ZERO;                                              break;
		case 13: m_addressing_mode = ADDRESSING_MODE_ZERO;                                              break;
		case 14: m_addressing_mode = promote ? ADDRESSING_MODE_REGISTER_W : ADDRESSING_MODE_REGISTER_E; break;
		case 15: m_addressing_mode = promote ? ADDRESSING_MODE_REGISTER_W : ADDRESSING_MODE_REGISTER_F; break;
		default: fatalerror("Should not reach here");                                                   break;
	}

	eat(1);
}

static inline UINT32 get_q(void)
{
	PAIR result;
	result.w.h = m_d.w;
	result.w.l = m_w.w;
	return result.d;
}

static inline void put_q(UINT32 value)
{
	PAIR pair;
	pair.d = value;
	m_d.w = pair.w.h;
	m_w.w = pair.w.l;
}

static inline void muld(void)
{
	INT32 result = ((INT16) m_d.w) * ((INT16) m_temp.w);
	put_q(set_flags32(CC_NZ, result));
	m_cc &= ~CC_VC;
}

static inline bool divq(void)
{
	INT32 result;
	if (m_temp.w == 0)
		return false;

	INT32 q = get_q();
	INT32 old_q = q;

	result = q / (INT16) m_temp.w;
	m_d.w = q % (INT16) m_temp.w;

	m_w.w = set_flags16(CC_NZ, result);

	if (m_w.w & 0x0001)
		m_cc |= CC_C;
	else
		m_cc &= ~CC_C;

	if ((result > 32768) || (result < -32767))
	{
		m_cc |= CC_V;
		if ((result > 65536) || (result < -65535))
		{
			if (old_q < 0)
				m_cc |= CC_N;
			else if (old_q == 0)
				m_cc |= CC_Z;

			put_q(old_q);
		}
	}
	else
	{
		m_cc &= ~CC_V;
	}

	return true;
}

static inline bool divd(void)
{
	if (m_temp.b.l == 0)
		return false;

	INT16 old_d = m_d.w;
	INT16 result;

	result = ((INT16) m_d.w) / (INT8) m_temp.b.l;
	m_d.b.h = ((INT16) m_d.w) % (INT8) m_temp.b.l;

	m_d.b.l = set_flags8(CC_NZ, result);

	if (m_d.b.l & 0x01)
		m_cc |= CC_C;
	else
		m_cc &= ~CC_C;

	if ((result > 128) || (result < -127))
	{
		m_cc |= CC_V;
		if ((result > 256) || (result < -255))
		{
			set_flags16(CC_NZ, old_d);
			m_d.w = abs(old_d);
		}
	}
	else
	{
		m_cc &= ~CC_V;
	}

	return true;
}
