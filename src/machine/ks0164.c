// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, David Carne

// Samsung Semiconductor KS0164 Wavetable Synthesizer
// KS0164 core

#include "driver.h"
#include "streams.h"
#include "sound/custom.h"
#include "ks0164.h"
#include <stdbool.h>
typedef UINT8 u8;
typedef INT8 s8;
typedef UINT16 u16;
typedef INT16 s16;
typedef UINT32 u32;
typedef INT32 s32;
typedef UINT64 u64;
typedef INT64 s64;
enum { R_SP=4,R_PSW=5,R_PC=6,R_ZERO=7,F_MASK=0xf0,F_Z=0x10,F_C=0x20,F_N=0x40,F_V=0x80,F_I=0x8000 };
enum { MPUS_RX_FULL=1, MPUS_TX_FULL=2, MPUS_RX_CTRL=4, MPUS_TX_INT=0x40, MPUS_RX_INT=0x80 };
static u16 m_r[8],m_sregs[32][32];
static u32 m_irq;
static int m_icount;
static u8 ram[0x2000];
static u16 banks[2];
static u8 m_mpu_in,m_mpu_out,m_mpu_status,m_unk60,m_voice_select,m_irqen_76,m_irqen_77;
static int timer_interrupt;
static sound_stream *m_stream;
static mame_timer *cpu_timer;
static unsigned tick_count, clock_remainder;
static u8 rom_rb(u32 a) {

	return memory_region(REGION_CPU2)[a & (memory_region_length(REGION_CPU2)-1)];
}
static u16 rom_rw(u32 a) { return (rom_rb(a)<<8)|rom_rb(a+1); }
static void set_irq(int n,int state) { if(state) m_irq|=1<<n; else m_irq&=~(1<<n); }
static u8 cpu_rb(u16 a) {
	if(a>=0xe000) return ram[a-0xe000];
	if(a>=0x4000 && a<0xc000) return rom_rb(((u32)banks[a>=0x8000]<<14)|(a&0x3fff));
	if((a>=0x80 && a<0x4000) || a<0x20) return rom_rb(a);
	if(a<0x60) { u16 v=m_sregs[m_voice_select&31][(a-0x20)/2]; return (a&1)?v:v>>8; }
	switch(a) {
	case 0x60:return m_unk60;
	case 0x61:return m_voice_select;
	case 0x62:case 0x63:case 0x64:case 0x65:return (a&1)?banks[(a-0x62)/2]:banks[(a-0x62)/2]>>8;
	case 0x68:m_mpu_status&=~MPUS_RX_FULL;set_irq(11,0);return m_mpu_in;
	case 0x69:return m_mpu_status;
	case 0x6c:case 0x6d:return 0;
	case 0x76:return m_irqen_76;
	case 0x77:return m_irqen_77;
	}
	return 0;
}
static u16 cpu_rw(u16 a) { return (cpu_rb(a)<<8)|cpu_rb(a+1); }
static void cpu_wb(u16 a,u8 d) {
	if(a>=0xe000) { ram[a-0xe000]=d;return; }
	if(a>=0x4000 && a<0xc000) {

		return;
	}
	if(a>=0x20 && a<0x60) {
		u16 *v=&m_sregs[m_voice_select&31][(a-0x20)/2];
		stream_update(m_stream,0);
		*v=(a&1)?((*v&0xff00)|d):((*v&0xff)|(d<<8));return;
	}
	switch(a) {
	case 0x60:m_unk60=d;break;
	case 0x61:m_voice_select=d;break;
	case 0x62:case 0x63:case 0x64:case 0x65:{ u16 *v=&banks[(a-0x62)/2];*v=(a&1)?((*v&0xff00)|d):((*v&255)|(d<<8));break; }
	case 0x68:m_mpu_out=d;m_mpu_status|=MPUS_TX_FULL;break;
	case 0x69:m_mpu_status=(m_mpu_status&~(MPUS_RX_INT|MPUS_TX_INT))|(d&(MPUS_RX_INT|MPUS_TX_INT));set_irq(11,(m_mpu_status&(MPUS_RX_INT|MPUS_RX_FULL))==(MPUS_RX_INT|MPUS_RX_FULL));break;

	case 0x76:m_irqen_76=d;if(d&0x40)set_irq(14,timer_interrupt);else {timer_interrupt=0;set_irq(14,0);}break;
	case 0x77:m_irqen_77=d;break;
	}
}
static void cpu_ww(u16 a,u16 d) {cpu_wb(a,d>>8);cpu_wb(a+1,d);}

static const u16 sample_dec[0x100]=
{
	0x8000, 0x8400, 0x8800, 0x8c00, 0x9000, 0x9400, 0x9800, 0x9c00,
	0xa000, 0xa400, 0xa800, 0xac00, 0xb000, 0xb400, 0xb800, 0xbc00,
	0x4000, 0x4400, 0x4800, 0x4c00, 0x5000, 0x5400, 0x5800, 0x5c00,
	0x6000, 0x6400, 0x6800, 0x6c00, 0x7000, 0x7400, 0x7800, 0x7c00,
	0xc000, 0xc200, 0xc400, 0xc600, 0xc800, 0xca00, 0xcc00, 0xce00,
	0xd000, 0xd200, 0xd400, 0xd600, 0xd800, 0xda00, 0xdc00, 0xde00,
	0x2000, 0x2200, 0x2400, 0x2600, 0x2800, 0x2a00, 0x2c00, 0x2e00,
	0x3000, 0x3200, 0x3400, 0x3600, 0x3800, 0x3a00, 0x3c00, 0x3e00,
	0xe000, 0xe100, 0xe200, 0xe300, 0xe400, 0xe500, 0xe600, 0xe700,
	0xe800, 0xe900, 0xea00, 0xeb00, 0xec00, 0xed00, 0xee00, 0xef00,
	0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700,
	0x1800, 0x1900, 0x1a00, 0x1b00, 0x1c00, 0x1d00, 0x1e00, 0x1f00,
	0xf000, 0xf080, 0xf100, 0xf180, 0xf200, 0xf280, 0xf300, 0xf380,
	0xf400, 0xf480, 0xf500, 0xf580, 0xf600, 0xf680, 0xf700, 0xf780,
	0x0800, 0x0880, 0x0900, 0x0980, 0x0a00, 0x0a80, 0x0b00, 0x0b80,
	0x0c00, 0x0c80, 0x0d00, 0x0d80, 0x0e00, 0x0e80, 0x0f00, 0x0f80,
	0xf800, 0xf840, 0xf880, 0xf8c0, 0xf900, 0xf940, 0xf980, 0xf9c0,
	0xfa00, 0xfa40, 0xfa80, 0xfac0, 0xfb00, 0xfb40, 0xfb80, 0xfbc0,
	0x0400, 0x0440, 0x0480, 0x04c0, 0x0500, 0x0540, 0x0580, 0x05c0,
	0x0600, 0x0640, 0x0680, 0x06c0, 0x0700, 0x0740, 0x0780, 0x07c0,
	0xfc00, 0xfc20, 0xfc40, 0xfc60, 0xfc80, 0xfca0, 0xfcc0, 0xfce0,
	0xfd00, 0xfd20, 0xfd40, 0xfd60, 0xfd80, 0xfda0, 0xfdc0, 0xfde0,
	0x0200, 0x0220, 0x0240, 0x0260, 0x0280, 0x02a0, 0x02c0, 0x02e0,
	0x0300, 0x0320, 0x0340, 0x0360, 0x0380, 0x03a0, 0x03c0, 0x03e0,
	0xfe00, 0xfe10, 0xfe20, 0xfe30, 0xfe40, 0xfe50, 0xfe60, 0xfe70,
	0xfe80, 0xfe90, 0xfea0, 0xfeb0, 0xfec0, 0xfed0, 0xfee0, 0xfef0,
	0x0100, 0x0110, 0x0120, 0x0130, 0x0140, 0x0150, 0x0160, 0x0170,
	0x0180, 0x0190, 0x01a0, 0x01b0, 0x01c0, 0x01d0, 0x01e0, 0x01f0,
	0x0000, 0x0008, 0x0010, 0x0018, 0x0020, 0x0028, 0x0030, 0x0038,
	0x0040, 0x0048, 0x0050, 0x0058, 0x0060, 0x0068, 0x0070, 0x0078,
	0xff80, 0xff88, 0xff90, 0xff98, 0xffa0, 0xffa8, 0xffb0, 0xffb8,
	0xffc0, 0xffc8, 0xffd0, 0xffd8, 0xffe0, 0xffe8, 0xfff0, 0xfff8,
};

static void handle_irq()
{
	u16 mask = m_irq & ((1U << ((m_r[R_PSW] & 15) + 1)) - 1);
	if(mask) {
		int index;
		for(index = 0; !(mask & (1 << index)); index ++);
		if(index) {

			if(m_r[R_PSW] & F_I)
				return;

			cpu_ww(m_r[R_SP] - 2, m_r[R_PC]);
			cpu_ww(m_r[R_SP] - 4, m_r[R_PSW]);
			m_r[R_SP] -= 4;
			m_icount -= 2;
		} else
			m_irq &= 0xfffe;

		m_r[R_PSW] = (m_r[R_PSW] & 0xfff0) | (index ? index - 1 : 0);
		m_r[R_PC] = cpu_rw(index*2);
		m_icount --;
	}
}

static u16 snz(u16 r)
{
	u16 f = 0;
	if(!r)
		f |= F_Z;
	if(r & 0x8000)
		f |= F_N;
	m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
	return r;
}

static void do_alu(u16 opcode, u16 v2)
{
	int r = (opcode >> 8) & 7;
	switch((opcode >> 11) & 7) {
	case 0: {
		u16 v1 = m_r[r];
		u16 res = v1 + v2;
		u16 f = 0;
		if(!res)
			f |= F_Z;
		if(res & 0x8000)
			f |= F_N;
		if(((v1 & v2) | ((~res) & (v1 | v2))) & 0x8000)
			f |= F_C;
		if(((v1 ^ res) & (v2 ^ res)) & 0x8000)
			f |= F_V;
		m_r[r] = res;
		m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
		break;
	}
	case 1: {
		u16 v1 = m_r[r];
		u16 res = v1 - v2;
		u16 f = 0;
		if(!res)
			f |= F_Z;
		if(res & 0x8000)
			f |= F_N;
		if(((v2 & res) | ((~v1) & (v2 | res))) & 0x8000)
			f |= F_C;
		if(((v1 ^ v2) & (v1 ^ res)) & 0x8000)
			f |= F_V;
		m_r[r] = res;
		m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
		break;
	}
	case 2: {
		u16 v1 = m_r[r];
		u16 res = v1 - v2;
		u16 f = 0;
		if(!res)
			f |= F_Z;
		if(res & 0x8000)
			f |= F_N;
		if(((v2 & res) | ((~v1) & (v2 | res))) & 0x8000)
			f |= F_C;
		if(((v1 ^ v2) & (v1 ^ res)) & 0x8000)
			f |= F_V;
		m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
		break;
	}
	case 3:
		m_r[r] = snz(m_r[r] & v2);
		break;
	case 4:
		m_r[r] = snz(m_r[r] | v2);
		break;
	case 5:
		m_r[r] = snz(m_r[r] ^ v2);
		break;
	case 6:
		m_r[r] = snz(v2);
		break;
	case 7: {
		u32 res;
		if(opcode & 0x0080)
			res = ((s16)(m_r[r])) * ((s16)(v2));
		else
			res = ((u16)(m_r[r])) * ((u16)(v2));

		u16 f = 0;
		if(!res)
			f |= F_Z;
		if(res & 0x8000)
			f |= F_N;
		if(res & 0xffff0000)
			f |= F_C;
		if(res >= 0x00008000 && res < 0xffff8000)
			f |= F_V;
		m_r[r] = res;
		m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
		break;
	}
	}
}

static void unk(u16 opcode)
{
	logerror("Unknown opcode %04x at address %04x\n", opcode, m_r[R_PC]-2);
}

static void execute_run()
{
	while(m_icount > 0) {
		if(m_irq)
			handle_irq();
		m_icount--;
		u16 opcode = cpu_rw(m_r[R_PC]);
		m_r[R_PC] += 2;

		switch(((opcode >> 11) & 0x18) | (opcode & 0x7)) {

		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: {

			bool cond;
			switch((opcode >> 10) & 0xf) {
			case 0x0: cond = !(m_r[R_PSW] & F_Z); break;
			case 0x1: cond =   m_r[R_PSW] & F_Z ; break;
			case 0x2: cond = !(m_r[R_PSW] & F_C); break;
			case 0x3: cond =   m_r[R_PSW] & F_C ; break;
			case 0x4: cond = !(m_r[R_PSW] & F_N); break;
			case 0x5: cond =   m_r[R_PSW] & F_N ; break;
			case 0x6: cond = !(m_r[R_PSW] & F_V); break;
			case 0x7: cond =   m_r[R_PSW] & F_V ; break;
			case 0x8: cond = false;               break;
			case 0x9: cond = !(m_r[R_PSW] & F_Z) && !(m_r[R_PSW] & F_C); break;
			case 0xa: cond = (m_r[R_PSW] & F_Z) || (m_r[R_PSW] & F_C); break;
			case 0xb: cond = (!(m_r[R_PSW] & F_Z) && (m_r[R_PSW] & F_N) && (m_r[R_PSW] & F_V)) || (!(m_r[R_PSW] & F_Z) && !(m_r[R_PSW] & F_V) && !(m_r[R_PSW] & F_N)); break;
			case 0xc: cond = ((m_r[R_PSW] & F_N) && (m_r[R_PSW] & F_V)) || (!(m_r[R_PSW] & F_V) && !(m_r[R_PSW] & F_N)); break;
			case 0xd: cond = (m_r[R_PSW] & F_Z) || ((m_r[R_PSW] & F_N) && !(m_r[R_PSW] & F_V)) || ((m_r[R_PSW] & F_V) && !(m_r[R_PSW] & F_N)); break;
			case 0xe: cond = ((m_r[R_PSW] & F_N) && !(m_r[R_PSW] & F_V)) || ((m_r[R_PSW] & F_V) && !(m_r[R_PSW] & F_N)); break;
			case 0xf: default: cond = true; break;
			}
			if(cond) {
				m_r[R_PC] += (((int)(opcode & 0x3ff) ^ 0x200) - 0x200);
			}
			break;
		}

		case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: {

			do_alu(opcode, ((s8)(opcode)));
			break;
		}

		case 0x14: {

			u16 rv = m_r[(opcode >> 4) & 7];
			u16 v = opcode & 0x0008 ? rv : opcode & 0x0080 ? ((s8)(rv)) : ((u8)(rv));

			do_alu(opcode, v);
			break;
		}

		case 0x15: {

			u16 rv = cpu_rw(m_r[R_PC]);
			m_r[R_PC] += 2;
			u16 v = opcode & 0x0008 ? rv : opcode & 0x0080 ? ((s8)(rv)) : ((u8)(rv));

			do_alu(opcode, v);
			break;
		}

		case 0x16: {

			u16 a = m_r[(opcode >> 4) & 7];
			u16 v = opcode & 0x0008 ? cpu_rw(a) : opcode & 0x0080 ? ((s8)cpu_rb(a)) : ((u8)cpu_rb(a));
			m_icount -= 2;

			do_alu(opcode, v);
			break;
		}

		case 0x17: {

			u16 a = m_r[(opcode >> 4) & 7] + cpu_rw(m_r[R_PC]);
			m_r[R_PC] += 2;
			u16 v = opcode & 0x0008 ? cpu_rw(a) : opcode & 0x0080 ? ((s8)cpu_rb(a)) : ((u8)cpu_rb(a));
			m_icount -= 2;

			do_alu(opcode, v);
			break;
		}

		case 0x18: {
			switch((opcode >> 12) & 3) {
			case 0: {

				int r1 = (opcode >> 8) & 7;
				int r2 = (opcode >> 4) & 7;
				switch((((opcode>>9)&4)|((opcode>>6)&2)|((opcode>>3)&1))) {
				case 1: cpu_ww(m_r[r1], m_r[r2]); m_r[r1] += 2; break;
				case 4: m_r[r1] -= 2; cpu_ww(m_r[r1], m_r[r2]); break;
				default: unk(opcode); break;
				}
				m_icount --;
				break;
			}

			case 1: {

				u16 v1 = m_r[(opcode >> 8) & 7];
				u16 v2 = cpu_rw(m_r[R_PC]);
				m_r[R_PC] += 2;
				u16 res;
				switch((((opcode>>10)&2)|((opcode>>7)&1))) {
				case 0: res = ((u16)v1 > (u16)v2 ? v1 : v2); break;
				case 1: res = ((s16)v1 > (s16)v2 ? v1 : v2); break;
				case 2: res = ((u16)v1 < (u16)v2 ? v1 : v2); break;
				case 3: default: res = ((s16)v1 < (s16)v2 ? v1 : v2); break;
				}
				m_r[(opcode >> 8) & 7] = res;
				break;
			}

			case 2: {

				if(m_r[(opcode >> 8) & 7] & (1 << ((opcode >> 4) & 0xf)))
					m_r[R_PSW] &= ~F_Z;
				else
					m_r[R_PSW] |=  F_Z;
				break;
			}

			case 3: {
				unk(opcode);
				break;
			}
			}
			break;
		}

		case 0x19: {
			switch((opcode >> 12) & 3) {
			case 0: {

				cpu_ww(m_r[R_SP] - 2, m_r[0]);
				cpu_ww(m_r[R_SP] - 4, m_r[1]);
				cpu_ww(m_r[R_SP] - 6, m_r[2]);
				cpu_ww(m_r[R_SP] - 8, m_r[3]);
				m_r[R_SP] -= 8;
				break;
			}

			case 1: {

				m_r[R_PC] = cpu_rw(m_r[R_PC]);
				break;
			}

			case 2: {

				if(m_r[(opcode >> 8) & 7] & (1 << ((opcode >> 4) & 0xf)))
					m_r[R_PSW] &= ~F_Z;
				else {
					m_r[R_PSW] |=  F_Z;
					m_r[(opcode >> 8) & 7] |= 1 << ((opcode >> 4) & 0xf);
				}
				break;
			}

			case 3: {
				unk(opcode);
				break;
			}
			}
			break;
		}

		case 0x1a: {
			switch((opcode >> 12) & 3) {
			case 0: {

				int r1 = (opcode >> 8) & 7;
				int r2 = (opcode >> 4) & 7;
				switch((((opcode>>9)&4)|((opcode>>6)&2)|((opcode>>3)&1))) {
				case 1: m_r[r1] = cpu_rw(m_r[r2]); m_r[r2] += 2; break;
				default: unk(opcode); break;
				}
				m_icount --;
				break;
			}

			case 1: {

				u16 a = cpu_rw(m_r[R_PC]);
				cpu_ww(m_r[R_SP] - 2, m_r[R_PC] + 2);
				m_r[R_SP] -= 2;
				m_r[R_PC] = a;
				m_icount -= 2;
				break;
			}

			case 2: {

				u16 a = m_r[(opcode >> 8) & 7];
				u16 v = opcode & 0x0008 ? cpu_rw(a) : cpu_rb(a);
				if(v & (1 << ((opcode >> 4) & 0xf)))
					m_r[R_PSW] &= ~F_Z;
				else
					m_r[R_PSW] |=  F_Z;
				m_icount -= 2;
				break;
			}

			case 3: {
				unk(opcode);
				break;
			}
			}
			break;
		}

		case 0x1b: {
			switch((opcode >> 12) & 3) {
			case 0: {

				m_r[0] = cpu_rw(m_r[R_SP] + 6);
				m_r[1] = cpu_rw(m_r[R_SP] + 4);
				m_r[2] = cpu_rw(m_r[R_SP] + 2);
				m_r[3] = cpu_rw(m_r[R_SP] + 0);
				m_r[R_SP] += 8;
				break;
			}

			case 1: {

				m_r[R_PC] = cpu_rw(m_r[R_SP]);
				m_r[R_SP] += 2;
				m_icount --;
				break;
			}

			case 2: {

				u16 a = m_r[(opcode >> 8) & 7] + cpu_rw(m_r[R_PC]);
				m_r[R_PC] += 2;
				u16 v = opcode & 0x0008 ? cpu_rw(a) : cpu_rb(a);
				if(v & (1 << ((opcode >> 4) & 0xf)))
					m_r[R_PSW] &= ~F_Z;
				else
					m_r[R_PSW] |=  F_Z;
				m_icount -= 2;
				break;
			}

			case 3: {
				unk(opcode);
				break;
			}
			}
			break;
		}

		case 0x1c: {
			switch((opcode >> 12) & 3) {
			case 0: {

				int r = (opcode >> 8) & 7;
				int shift = (opcode >> 4) & 0xf;
				u16 v1 = m_r[r];
				u16 res;

				if(!shift) {
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | (v1 ? v1 & 0x8000 ? F_N : 0 : F_Z);
					res = v1;
				} else if(opcode & 0x0800) {
					res = v1 >> shift;
					u16 f = res ? 0 : F_Z;
					if(v1 & (1 << (shift - 1)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				} else {
					res = v1 << shift;
					u16 f = res ? res & 0x8000 ? F_N : 0 : F_Z;
					if(v1 & (1 << (16-shift)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				}
				m_r[r] = res;
				break;
			}

			case 1: {

				m_r[R_PSW] = cpu_rw(m_r[R_SP] + 0);
				m_r[R_PC]  = cpu_rw(m_r[R_SP] + 2);
				m_r[R_SP] += 4;
				m_icount -= 2;
				break;
			}

			case 2: {

				if(m_r[(opcode >> 8) & 7] & (1 << ((opcode >> 4) & 0xf))) {
					m_r[(opcode >> 8) & 7] &= ~(1 << ((opcode >> 4) & 0xf));
					m_r[R_PSW] &= ~F_Z;
				} else
					m_r[R_PSW] |=  F_Z;
				break;
			}

			case 3: {

				int r = (opcode >> 8) & 7;
				u16 a = cpu_rw(m_r[R_PC]);
				m_r[R_PC] += 2;

				m_r[r] --;
				if(m_r[r] != 0)
					m_r[R_PC] = a;
				break;
			}
			}
			break;
		}

		case 0x1d: {
			switch((opcode >> 12) & 3) {
			case 0: {

				int r = (opcode >> 8) & 7;
				int shift = (opcode >> 4) & 0xf;
				u16 v1 = m_r[r];
				u16 res;

				if(!shift) {
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | (v1 ? v1 & 0x8000 ? F_N : 0 : F_Z);
					res = v1;
				} else if(opcode & 0x0800) {
					res = ((s16)(v1)) >> shift;
					u16 f = res ? res & 0x8000 ? F_N : 0 : F_Z;
					if(v1 & (1 << (shift - 1)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				} else {
					res = v1 << shift;
					u16 f = res ? res & 0x8000 ? F_N : 0 : F_Z;
					if(v1 & (1 << (16-shift)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				}
				m_r[r] = res;
				break;
			}

			case 1: {

				u16 v = m_r[(opcode >> 8) & 7];
				if(opcode & 0x0080)
					v = -((s16)(v));
				else
					v = ~v;
				m_r[(opcode >> 8) & 7] = v;
				m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | (v ? v & 0x8000 ? F_N : 0 : F_Z);
				break;
			}

			case 2: {
				unk(opcode);
				break;
			}

			case 3: {

				u16 v = cpu_rw(m_r[R_PC]);
				u16 a = cpu_rw(m_r[R_PC] + 2);
				m_r[R_PC] += 4;

				do_alu((opcode & 0x07ff) | 0x1000, v);
				if(m_r[R_PSW] & F_Z)
					m_r[R_PC] = a;
				break;
			}
			}
			break;
		}

		case 0x1e: {
			switch((opcode >> 12) & 3) {
			case 0: {

				int r = (opcode >> 8) & 7;
				int shift = (opcode >> 4) & 0xf;
				u16 v1 = m_r[r];
				u16 res;

				if(!shift) {
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | (v1 ? v1 & 0x8000 ? F_N : 0 : F_Z);
					res = v1;
				} else if(opcode & 0x0800) {
					res = (v1 >> shift) | (v1 << (16 - shift));
					u16 f = res ? 0 : F_Z;
					if(v1 & (1 << (shift - 1)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				} else {
					res = (v1 << shift) | (v1 >> (16 - shift));
					u16 f = res ? res & 0x8000 ? F_N : 0 : F_Z;
					if(v1 & (1 << (16-shift)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				}
				m_r[r] = res;
				break;
			}

			case 1: {
				unk(opcode);
				break;
			}

			case 2: {

				u16 a = m_r[(opcode >> 8) & 7];
				if(opcode & 0x0008)
					cpu_ww(a, m_r[(opcode >> 4) & 7]);
				else
					cpu_wb(a, m_r[(opcode >> 4) & 7]);
				break;
			}

			case 3: {
				unk(opcode);
				break;
			}
			}
			break;
		}

		case 0x1f: {
			switch((opcode >> 12) & 3) {
			case 0: {

				int r = (opcode >> 8) & 7;
				int shift = (opcode >> 4) & 0xf;
				u32 v1 = m_r[r] | (m_r[R_PSW] & F_C ? 0x10000 : 0);
				u16 res;

				if(!shift) {
					m_r[R_PSW] = (m_r[R_PSW] & ~(F_N|F_Z|F_V)) | (v1 & 0xffff ? v1 & 0x8000 ? F_N : 0 : F_Z);
					res = v1;
				} else if(opcode & 0x0800) {
					res = (v1 >> shift) | (v1 << (17-shift));
					u16 f = res ? res & 0x8000 ? F_N : 0 : F_Z;
					if(v1 & (1 << (shift - 1)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				} else {
					res = (v1 << shift) | (v1 >> (17-shift));
					u16 f = res ? res & 0x8000 ? F_N : 0 : F_Z;
					if(v1 & (1 << (16-shift)))
						f |= F_C;
					m_r[R_PSW] = (m_r[R_PSW] & ~F_MASK) | f;
				}
				m_r[r] = res;
				break;
			}

			case 1: {
				unk(opcode);
				break;
			}

			case 2: {

				u16 a = m_r[(opcode >> 8) & 7] + cpu_rw(m_r[R_PC]);
				m_r[R_PC] += 2;
				if(opcode & 0x0008)
					cpu_ww(a, m_r[(opcode >> 4) & 7]);
				else
					cpu_wb(a, m_r[(opcode >> 4) & 7]);
				break;
			}

			case 3: {
				unk(opcode);
				break;
			}
			}
			break;
		}

		default:
			unk(opcode);
			break;
		}

		m_r[R_ZERO] = 0;
	}
}

static void audio_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	for(int sample = 0; sample != samples; sample++) {
		s32 suml = 0, sumr = 0;
		for(int voice = 0; voice < 0x20; voice++) {
			u16 *regs = m_sregs[voice];
			if(regs[0] & 0x0001) {
				u64 current = (((u64)(regs[1])) << 32) | (((u64)(regs[2])) << 16) | regs[3];
				u32 adr = current >> 16;
				s16 samp0, samp1;
				switch(regs[0] & 0x8400) {
				case 0x0000:
					samp0 = rom_rw(2*adr);
					samp1 = rom_rw(2*adr+2);
					break;

				case 0x8400:
					samp0 = sample_dec[rom_rb(adr)];
					samp1 = sample_dec[rom_rb(adr+1)];
					break;

				default:

					samp0 = samp1 = 0;
					break;
				}

				s16 samp = samp0 + (((samp1 - samp0) * (current & 0xffff)) >> 16);
				u32 step = 0x10000 | (regs[8] & ~0xf);
				u32 shift = regs[8] & 0xf;
				if(shift >= 0x8)
					step >>= 0x10 - shift;
				else if(shift)
					step <<= shift;
				current += step;
				u64 end = (((u64)(regs[0xd])) << 32) | (((u64)(regs[0xe])) << 16) | regs[0xf];
				if(current >= end) {
					if (regs[0] & 8) {
						u64 loop = (((u64)(regs[9])) << 32) | (((u64)(regs[0xa])) << 16) | regs[0xb];
						while(current >= end && end > loop)
							current = current - end + loop;
					} else {
						regs[0] &= ~1;
						regs[0xc] = 0;
						regs[0x10] = regs[0x12] = regs[0x14] = regs[0x16] = 0;
					}
				}
				regs[1] = current >> 32;
				regs[2] = current >> 16;
				regs[3] = current;

				suml += (((s64)(samp)) * regs[0x12] * regs[0x16]) >> 32;
				sumr += (((s64)(samp)) * regs[0x10] * regs[0x14]) >> 32;

				if(regs[0xc]) {
					regs[0x10] += regs[0x11];
					regs[0x12] += regs[0x13];
					regs[0x14] += regs[0x15];
					regs[0x16] += regs[0x17];
					regs[0xc] --;
				}
			}
		}
		outputs[0][sample] = suml / 32;
		outputs[1][sample] = sumr / 32;
	}
}

static void cpu_tick(int param)
{
	if (++tick_count % 10 == 0) { timer_interrupt=1; if(m_irqen_76&0x40)set_irq(14,1); }
	clock_remainder += 16934400;
	m_icount += clock_remainder / 60000;
	clock_remainder %= 60000;
	execute_run();
}
static void ks_reset(void *token)
{
	memset(m_r,0,sizeof(m_r)); memset(m_sregs,0,sizeof(m_sregs));
	memset(ram,0,sizeof(ram));
	banks[0]=banks[1]=0;m_irq=1;m_r[R_PSW]=F_I;m_icount=0;
	m_mpu_in=m_mpu_out=m_mpu_status=m_unk60=m_voice_select=m_irqen_76=m_irqen_77=0;
	timer_interrupt=0;tick_count=clock_remainder=0;
	timer_adjust(cpu_timer,TIME_IN_HZ(10000),0,TIME_IN_HZ(10000));
}
static void *ks_start(int clock,const struct CustomSound_interface *config)
{
	m_stream=stream_create(0,2,16934400/384,0,audio_update);
	cpu_timer=timer_alloc(cpu_tick);
	ks_reset(0);
	return m_stream;
}
const struct CustomSound_interface ks0164_interface={ks_start,0,ks_reset,0};
READ32_HANDLER(ks0164_data_r) { m_mpu_status&=~MPUS_TX_FULL;return m_mpu_out; }
READ32_HANDLER(ks0164_status_r) {return 0x3f|(!(m_mpu_status&MPUS_TX_FULL)?0x80:0)|((m_mpu_status&MPUS_RX_FULL)?0x40:0);}
static void host_w(u8 d,int ctrl) {
	m_mpu_in=d;m_mpu_status=(m_mpu_status&~MPUS_RX_CTRL)|(ctrl?MPUS_RX_CTRL:0)|MPUS_RX_FULL;
	if(m_mpu_status&MPUS_RX_INT)set_irq(11,1);
	cpu_boost_interleave(0,TIME_IN_USEC(100));
}
WRITE32_HANDLER(ks0164_data_w) {if(ACCESSING_LSB32)host_w(data,0);}
WRITE32_HANDLER(ks0164_ctrl_w) {if(ACCESSING_LSB32)host_w(data,1);}
