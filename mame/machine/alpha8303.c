// license:BSD-3-Clause
// copyright-holders:hap
/***************************************************************************

    Alpha Denshi ALPHA-8303 protection controller.
    Hitachi HD44801 with external bytecode program RAM.

***************************************************************************/

#include "driver.h"
#include "includes/alpha8303.h"

enum { HMCS42_FAMILY, HMCS43_FAMILY, HMCS44_FAMILY, HMCS45_FAMILY, HMCS46_FAMILY, HMCS47_FAMILY };

static struct
{
	UINT16 stack[4], op, prev_op, pc, prev_pc, d;
	UINT8 i, pc_upper, a, b, x, spx, y, spy, s, c;
	UINT8 prescaler, block_int, tc, cf, ie, iri, irt, iflag[2], tf, int_line[2], r[8];
	int icount, eint_line, halt;
} mcu;
static UINT8 internal_ram[0x100], external_ram[0x400];
static UINT8 *firmware;
static int bus;
static mame_timer *execution_timer;

/* The upper RAM files mirror their sixteen physical nibbles. */
static UINT8 data_read(int address)
{
	address &= 0xff;
	if (address & 0x80) address &= ~0x30;
	return internal_ram[address];
}

static void data_write(int address, UINT8 data)
{
	address &= 0xff;
	if (address & 0x80) address &= ~0x30;
	internal_ram[address] = data & 15;
}

static UINT16 rom_read(int address)
{
	address = (address & 0xfff) * 2;
	return firmware[address] | (firmware[address + 1] << 8);
}

static void update_external_ram(void)
{
	/* WR is a level, not a strobe.  Address and data changes can each
	   complete a write while the controller owns the shared bus. */
	if (bus && (mcu.d & 0x0c) == 0x0c)
	{
		int address = ((mcu.d << 8) & 0x300) | (mcu.r[2] << 4) | mcu.r[3];
		external_ram[address] = (mcu.r[0] << 4) | mcu.r[1];
	}
}

static UINT8 read_r(UINT8 index)
{
	UINT8 input = 15;
	index &= 7;
	if (index < 2)
	{
		int address = ((mcu.d << 8) & 0x300) | (mcu.r[2] << 4) | mcu.r[3];
		input = bus && !(mcu.d & 4) ? external_ram[address] : 0;
		if (!index) input >>= 4;
	}
	return input & mcu.r[index] & 15;
}

static void write_r(UINT8 index, UINT8 data)
{
	index &= 7;
	if (index < 6)
	{
		mcu.r[index] = data & 15;
		update_external_ram();
	}
}

static int read_d(UINT8 index) { return (mcu.d >> (index & 15)) & 1; }
static void write_d(UINT8 index, int state)
{
	UINT16 mask = 1 << (index & 15);
	mcu.d = (mcu.d & ~mask) | (state ? mask : 0);
	update_external_ram();
}

static UINT8 ram_r(void);
static void ram_w(UINT8 data);
static void pop_stack(void);
static void push_stack(void);
static void op_illegal(void);
static void op_lab(void);
static void op_lba(void);
static void op_lay(void);
static void op_laspx(void);
static void op_laspy(void);
static void op_xamr(void);
static void op_lxa(void);
static void op_lya(void);
static void op_lxi(void);
static void op_lyi(void);
static void op_iy(void);
static void op_dy(void);
static void op_ayy(void);
static void op_syy(void);
static void op_xsp(void);
static void op_lam(void);
static void op_lbm(void);
static void op_xma(void);
static void op_xmb(void);
static void op_lmaiy(void);
static void op_lmady(void);
static void op_lmiiy(void);
static void op_lai(void);
static void op_lbi(void);
static void op_ai(void);
static void op_ib(void);
static void op_db(void);
static void op_amc(void);
static void op_smc(void);
static void op_am(void);
static void op_daa(void);
static void op_das(void);
static void op_nega(void);
static void op_comb(void);
static void op_sec(void);
static void op_rec(void);
static void op_tc(void);
static void op_rotl(void);
static void op_rotr(void);
static void op_or(void);
static void op_mnei(void);
static void op_ynei(void);
static void op_anem(void);
static void op_bnem(void);
static void op_alei(void);
static void op_alem(void);
static void op_blem(void);
static void op_sem(void);
static void op_rem(void);
static void op_tm(void);
static void op_br(void);
static void op_cal(void);
static void op_lpu(void);
static void op_tbr(void);
static void op_rtn(void);
static void op_seie(void);
static void op_seif0(void);
static void op_seif1(void);
static void op_setf(void);
static void op_secf(void);
static void op_reie(void);
static void op_reif0(void);
static void op_reif1(void);
static void op_retf(void);
static void op_recf(void);
static void op_ti0(void);
static void op_ti1(void);
static void op_tif0(void);
static void op_tif1(void);
static void op_ttf(void);
static void op_lti(void);
static void op_lta(void);
static void op_lat(void);
static void op_rtni(void);
static void op_sed(void);
static void op_red(void);
static void op_td(void);
static void op_sedd(void);
static void op_redd(void);
static void op_lar(void);
static void op_lbr(void);
static void op_lra(void);
static void op_lrb(void);
static void op_p(void);
static void clock_timer(void);
static void clock_prescaler(void);
static void increment_pc(void);
static void cycle(void);
static void take_interrupt(void);
static void execute_set_input(int line, int state);
static void execute_run(void);

static UINT8 ram_r(void)
{
	UINT8 address = (mcu.x << 4 | mcu.y) & 0xff;
	return data_read(address) & 0xf;
}

static void ram_w(UINT8 data)
{
	UINT8 address = (mcu.x << 4 | mcu.y) & 0xff;
	data_write(address, data & 0xf);
}

static void pop_stack(void)
{
	mcu.pc = mcu.stack[0] & 0x7ff;
	for (int i = 0; i < 4-1; i++)
		mcu.stack[i] = mcu.stack[i+1];
}

static void push_stack(void)
{
	for (int i = 4-1; i >= 1; i--)
		mcu.stack[i] = mcu.stack[i-1];
	mcu.stack[0] = mcu.pc;
}

static void op_illegal(void)
{
	logerror("unknown opcode $%03X @ $%04X\n", mcu.op, mcu.prev_pc);
}

static void op_lab(void)
{

	mcu.a = mcu.b;
}

static void op_lba(void)
{

	mcu.b = mcu.a;
}

static void op_lay(void)
{

	mcu.a = mcu.y;
}

static void op_laspx(void)
{

	mcu.a = mcu.spx;
}

static void op_laspy(void)
{

	mcu.a = mcu.spy;
}

static void op_xamr(void)
{

	UINT8 address = mcu.op & 0xf;

	if (HMCS44_FAMILY == HMCS42_FAMILY || HMCS44_FAMILY == HMCS43_FAMILY)
		address |= (address < 4) ? (address << 4) : 0x40;

	else
		address |= 0xf0;

	address &= 0xff;
	UINT8 old_a = mcu.a;
	mcu.a = data_read(address) & 0xf;
	data_write(address, old_a & 0xf);
}

static void op_lxa(void)
{

	mcu.x = mcu.a;
}

static void op_lya(void)
{

	mcu.y = mcu.a;
}

static void op_lxi(void)
{

	mcu.x = mcu.i;
}

static void op_lyi(void)
{

	mcu.y = mcu.i;
}

static void op_iy(void)
{

	mcu.y = (mcu.y + 1) & 0xf;
	mcu.s = (mcu.y != 0);
}

static void op_dy(void)
{

	mcu.y = (mcu.y - 1) & 0xf;
	mcu.s = (mcu.y != 0xf);
}

static void op_ayy(void)
{

	mcu.y += mcu.a;
	mcu.s = BIT(mcu.y, 4);
	mcu.y &= 0xf;
}

static void op_syy(void)
{

	mcu.y -= mcu.a;
	mcu.s = BIT(~mcu.y, 4);
	mcu.y &= 0xf;
}

static void op_xsp(void)
{

	if (mcu.op & 1)
	{
		UINT8 old_x = mcu.x;
		mcu.x = mcu.spx;
		mcu.spx = old_x;
	}
	if (mcu.op & 2)
	{
		UINT8 old_y = mcu.y;
		mcu.y = mcu.spy;
		mcu.spy = old_y;
	}
}

static void op_lam(void)
{

	mcu.a = ram_r();
	op_xsp();
}

static void op_lbm(void)
{

	mcu.b = ram_r();
	op_xsp();
}

static void op_xma(void)
{

	UINT8 old_a = mcu.a;
	mcu.a = ram_r();
	ram_w(old_a);
	op_xsp();
}

static void op_xmb(void)
{

	UINT8 old_b = mcu.b;
	mcu.b = ram_r();
	ram_w(old_b);
	op_xsp();
}

static void op_lmaiy(void)
{

	ram_w(mcu.a);
	op_iy();
	op_xsp();
}

static void op_lmady(void)
{

	ram_w(mcu.a);
	op_dy();
	op_xsp();
}

static void op_lmiiy(void)
{

	ram_w(mcu.i);
	op_iy();
}

static void op_lai(void)
{

	mcu.a = mcu.i;
}

static void op_lbi(void)
{

	mcu.b = mcu.i;
}

static void op_ai(void)
{

	mcu.a += mcu.i;
	mcu.s = BIT(mcu.a, 4);
	mcu.a &= 0xf;
}

static void op_ib(void)
{

	mcu.b = (mcu.b + 1) & 0xf;
	mcu.s = (mcu.b != 0);
}

static void op_db(void)
{

	mcu.b = (mcu.b - 1) & 0xf;
	mcu.s = (mcu.b != 0xf);
}

static void op_amc(void)
{

	mcu.a += ram_r() + mcu.c;
	mcu.c = BIT(mcu.a, 4);
	mcu.s = mcu.c;
	mcu.a &= 0xf;
}

static void op_smc(void)
{

	mcu.a = ram_r() - mcu.a - (mcu.c ^ 1);
	mcu.c = BIT(~mcu.a, 4);
	mcu.s = mcu.c;
	mcu.a &= 0xf;
}

static void op_am(void)
{

	mcu.a += ram_r();
	mcu.s = BIT(mcu.a, 4);
	mcu.a &= 0xf;
}

static void op_daa(void)
{

	if (mcu.c || mcu.a > 9)
	{
		mcu.a = (mcu.a + 6) & 0xf;
		mcu.c = 1;
	}
}

static void op_das(void)
{

	if (!mcu.c || mcu.a > 9)
	{
		mcu.a = (mcu.a + 10) & 0xf;
		mcu.c = 0;
	}
}

static void op_nega(void)
{

	mcu.a = (0 - mcu.a) & 0xf;
}

static void op_comb(void)
{

	mcu.b ^= 0xf;
}

static void op_sec(void)
{

	mcu.c = 1;
}

static void op_rec(void)
{

	mcu.c = 0;
}

static void op_tc(void)
{

	mcu.s = mcu.c;
}

static void op_rotl(void)
{

	mcu.a = mcu.a << 1 | mcu.c;
	mcu.c = BIT(mcu.a, 4);
	mcu.a &= 0xf;
}

static void op_rotr(void)
{

	UINT8 c = mcu.a & 1;
	mcu.a = mcu.a >> 1 | mcu.c << 3;
	mcu.c = c;
}

static void op_or(void)
{

	mcu.a |= mcu.b;
}

static void op_mnei(void)
{

	mcu.s = (mcu.i != ram_r());
}

static void op_ynei(void)
{

	mcu.s = (mcu.y != mcu.i);
}

static void op_anem(void)
{

	mcu.s = (mcu.a != ram_r());
}

static void op_bnem(void)
{

	mcu.s = (mcu.b != ram_r());
}

static void op_alei(void)
{

	mcu.s = (mcu.a <= mcu.i);
}

static void op_alem(void)
{

	mcu.s = (mcu.a <= ram_r());
}

static void op_blem(void)
{

	mcu.s = (mcu.b <= ram_r());
}

static void op_sem(void)
{

	ram_w(ram_r() | (1 << (mcu.op & 3)));
}

static void op_rem(void)
{

	ram_w(ram_r() & ~(1 << (mcu.op & 3)));
}

static void op_tm(void)
{

	mcu.s = BIT(ram_r(), mcu.op & 3);
}

static void op_br(void)
{

	if (mcu.s)
		mcu.pc = (mcu.pc & ~0x3f) | (mcu.op & 0x3f);
	else
		mcu.s = 1;
}

static void op_cal(void)
{

	if (mcu.s)
	{
		mcu.block_int = 1;
		push_stack();
		mcu.pc = mcu.op & 0x3f;
	}
	else
		mcu.s = 1;
}

static void op_lpu(void)
{

	if (mcu.s)
	{
		mcu.block_int = 1;
		mcu.pc_upper = mcu.op & 0x1f;

		if (HMCS44_FAMILY == HMCS46_FAMILY || HMCS44_FAMILY == HMCS47_FAMILY)
			mcu.pc_upper |= ~mcu.r[7] << 5 & 0x20;
	}
	else
		mcu.op |= 0x400;
}

static void op_tbr(void)
{

	UINT16 address = mcu.a | mcu.b << 4 | mcu.c << 8 | (mcu.op & 7) << 9 | (mcu.pc & ~0x3f);
	mcu.pc = address & 0x7ff;
}

static void op_rtn(void)
{

	pop_stack();
}

static void op_seie(void)
{

	mcu.ie = 1;
}

static void op_seif0(void)
{

	mcu.iflag[0] = 1;
}

static void op_seif1(void)
{

	mcu.iflag[1] = 1;
}

static void op_setf(void)
{

	mcu.tf = 1;
}

static void op_secf(void)
{

	mcu.cf = 1;
}

static void op_reie(void)
{

	mcu.ie = 0;
}

static void op_reif0(void)
{

	mcu.iflag[0] = 0;
}

static void op_reif1(void)
{

	mcu.iflag[1] = 0;
}

static void op_retf(void)
{

	mcu.tf = 0;
}

static void op_recf(void)
{

	mcu.cf = 0;
}

static void op_ti0(void)
{

	mcu.s = mcu.int_line[0];
}

static void op_ti1(void)
{

	mcu.s = mcu.int_line[1];
}

static void op_tif0(void)
{

	mcu.s = mcu.iflag[0];
}

static void op_tif1(void)
{

	mcu.s = mcu.iflag[1];
}

static void op_ttf(void)
{

	mcu.s = mcu.tf;
}

static void op_lti(void)
{

	mcu.tc = mcu.i;
	mcu.prescaler = 0;
}

static void op_lta(void)
{

	mcu.tc = mcu.a;
	mcu.prescaler = 0;
}

static void op_lat(void)
{

	mcu.a = mcu.tc;
}

static void op_rtni(void)
{

	op_seie();
	op_rtn();
}

static void op_sed(void)
{

	write_d(mcu.y, 1);
}

static void op_red(void)
{

	write_d(mcu.y, 0);
}

static void op_td(void)
{

	mcu.s = read_d(mcu.y);
}

static void op_sedd(void)
{

	write_d(mcu.op & 3, 1);
}

static void op_redd(void)
{

	write_d(mcu.op & 3, 0);
}

static void op_lar(void)
{

	mcu.a = read_r(mcu.op & 7);
}

static void op_lbr(void)
{

	mcu.b = read_r(mcu.op & 7);
}

static void op_lra(void)
{

	write_r(mcu.op & 7, mcu.a);
}

static void op_lrb(void)
{

	write_r(mcu.op & 7, mcu.b);
}

static void op_p(void)
{

	cycle();

	UINT16 address = mcu.a | mcu.b << 4 | mcu.c << 8 | (mcu.op & 7) << 9 | (mcu.pc & ~0x3f);
	UINT16 data = rom_read(address & 0xfff);

	if (data & 0x100)
	{

		mcu.a = BITSWAP8(data,7,6,5,4,0,1,2,3) & 0xf;
		mcu.b = data >> 4 & 0xf;
	}
	if (data & 0x200)
	{

		data = BITSWAP8(data,0,1,2,3,4,5,6,7);
		write_r(2, data & 0xf);
		write_r(3, data >> 4 & 0xf);
	}
}
static void clock_timer(void)
{

	mcu.tc = (mcu.tc + 1) & 0xf;

	if (mcu.tc == 0 && !mcu.tf)
	{
		mcu.irt = 1;
		mcu.tf = 1;
	}
}
static void clock_prescaler(void)
{
	mcu.prescaler = (mcu.prescaler + 1) & 0x3f;

	if (mcu.prescaler == 0 && !mcu.cf)
		clock_timer();
}
static void increment_pc(void)
{

	UINT8 mask = 0x3f;
	UINT8 low = mcu.pc & mask;
	int fb = (low << 1 & 0x20) == (low & 0x20);

	if (low == (mask >> 1))
		fb = 1;
	else if (low == mask)
		fb = 0;

	mcu.pc = (mcu.pc & ~mask) | ((mcu.pc << 1 | fb) & mask);
}
static void cycle(void)
{
	mcu.icount--;
	clock_prescaler();
}
static void take_interrupt(void)
{
	push_stack();
	mcu.ie = 0;



	mcu.pc = 0x3f | (mcu.iri ? 0x40 : 0);
	if (mcu.iri)
		mcu.iri = 0;
	else
		mcu.irt = 0;

	mcu.prev_pc = mcu.pc;

	cycle();
}
static void execute_set_input(int line, int state)
{
	state = state ? 1 : 0;

	if (line == 2 && state != mcu.halt)
	{
		mcu.halt = state;
		return;
	}

	if (line != 0 && line != 1)
		return;

	if (state && !mcu.int_line[line])
	{
		if (!mcu.iflag[line])
		{
			mcu.eint_line = line;
			mcu.iri = 1;
			mcu.iflag[line] = 1;
		}

		if (mcu.cf && line == 1)
			clock_timer();
	}

	mcu.int_line[line] = state;
}
static void execute_run(void)
{

	if (mcu.halt)
	{

		mcu.icount = 0;
		return;
	}

	while (mcu.icount > 0)
	{

		if ((mcu.prev_op & 0x7e0) == 0x340)
			mcu.pc = ((mcu.pc_upper << 6) | (mcu.pc & 0x3f)) & 0x7ff;

		mcu.prev_op = mcu.op;
		mcu.prev_pc = mcu.pc;

		if (mcu.ie && (mcu.iri || mcu.irt) && !mcu.block_int)
			take_interrupt();
		mcu.block_int = 0;


		mcu.op = rom_read(mcu.pc) & 0x3ff;
		mcu.i = BITSWAP8(mcu.op,7,6,5,4,0,1,2,3) & 0xf;
		increment_pc();
		cycle();

		switch (mcu.op & 0x3f0)
		{
			case 0x1c0: case 0x1d0: case 0x1e0: case 0x1f0: op_br(); break;
			case 0x3c0: case 0x3d0: case 0x3e0: case 0x3f0: op_cal(); break;
			case 0x340: case 0x350: op_lpu(); break;

			case 0x010: op_lmiiy(); break;
			case 0x070: op_lai(); break;
			case 0x080: op_ai(); break;
			case 0x0f0: op_xamr(); break;
			case 0x140: op_lxi(); break;
			case 0x150: op_lyi(); break;
			case 0x160: op_lbi(); break;
			case 0x170: op_lti(); break;
			case 0x210: op_mnei(); break;
			case 0x270: op_alei(); break;
			case 0x280: op_ynei(); break;

			default:
				switch (mcu.op & 0x3fc)
				{
			case 0x0c0: case 0x0c4: op_lar(); break;
			case 0x0e0: case 0x0e4: op_lbr(); break;
			case 0x2c0: case 0x2c4: op_lra(); break;
			case 0x2e0: case 0x2e4: op_lrb(); break;
			case 0x360: case 0x364: op_tbr(); break;
			case 0x368: case 0x36c: op_p(); break;

			case 0x000: op_xsp(); break;
			case 0x004: op_sem(); break;
			case 0x008: op_lam(); break;
			case 0x020: op_lbm(); break;
			case 0x0d0: op_sedd(); break;
			case 0x200: op_tm(); break;
			case 0x204: op_rem(); break;
			case 0x208: op_xma(); break;
			case 0x220: op_xmb(); break;
			case 0x2d0: op_redd(); break;

			default:
				switch (mcu.op)
				{
			case 0x024: op_blem(); break;
			case 0x030: op_amc(); break;
			case 0x034: op_am(); break;
			case 0x03c: op_lta(); break;
			case 0x040: op_lxa(); break;
			case 0x045: op_das(); break;
			case 0x046: op_daa(); break;
			case 0x04c: op_rec(); break;
			case 0x04f: op_sec(); break;
			case 0x050: op_lya(); break;
			case 0x054: op_iy(); break;
			case 0x058: op_ayy(); break;
			case 0x060: op_lba(); break;
			case 0x064: op_ib(); break;
			case 0x090: op_sed(); break;
			case 0x094: op_td(); break;
			case 0x0a0: op_seif1(); break;
			case 0x0a1: op_secf(); break;
			case 0x0a2: op_seif0(); break;
			case 0x0a4: op_seie(); break;
			case 0x0a5: op_setf(); break;

			case 0x110: case 0x111: op_lmaiy(); break;
			case 0x114: case 0x115: op_lmady(); break;
			case 0x118: op_lay(); break;
			case 0x120: op_or(); break;
			case 0x124: op_anem(); break;
			case 0x1a0: op_tif1(); break;
			case 0x1a1: op_ti1(); break;
			case 0x1a2: op_tif0(); break;
			case 0x1a3: op_ti0(); break;
			case 0x1a5: op_ttf(); break;

			case 0x224: op_rotr(); break;
			case 0x225: op_rotl(); break;
			case 0x230: op_smc(); break;
			case 0x234: op_alem(); break;
			case 0x23c: op_lat(); break;
			case 0x240: op_laspx(); break;
			case 0x244: op_nega(); break;
			case 0x24f: op_tc(); break;
			case 0x250: op_laspy(); break;
			case 0x254: op_dy(); break;
			case 0x258: op_syy(); break;
			case 0x260: op_lab(); break;
			case 0x267: op_db(); break;
			case 0x290: op_red(); break;
			case 0x2a0: op_reif1(); break;
			case 0x2a1: op_recf(); break;
			case 0x2a2: op_reif0(); break;
			case 0x2a4: op_reie(); break;
			case 0x2a5: op_retf(); break;

			case 0x320: op_comb(); break;
			case 0x324: op_bnem(); break;
			case 0x3a4: op_rtni(); break;
			case 0x3a7: op_rtn(); break;

			default: op_illegal(); break;
				}
				break;

				}
				break;

		}
	}
}

static void execute_tick(int param)
{
	/* Eight machine cycles per 64 us at 500 kHz, four clocks per cycle.
	   Preserve cycle debt so multi-cycle instructions never accelerate time. */
	mcu.icount += 8;
	execute_run();
}

void alpha8303_init(int region)
{
	firmware = memory_region(region);
	memset(&mcu, 0, sizeof(mcu));
	memset(internal_ram, 0, sizeof(internal_ram));
	memset(external_ram, 0, sizeof(external_ram));
	mcu.s = 1;
	execution_timer = timer_alloc(execute_tick);
}

void alpha8303_reset(void)
{
	int i;
	mcu.pc = 0x7ff;
	mcu.op = mcu.prev_op = 0;
	mcu.cf = mcu.ie = mcu.iri = mcu.irt = 0;
	mcu.iflag[0] = mcu.iflag[1] = mcu.tf = 1;
	mcu.int_line[0] = mcu.int_line[1] = 0;
	mcu.icount = 0;
	bus = 0;
	mcu.d = 0xffff;
	for (i = 0; i < 8; ++i) mcu.r[i] = 15;
	timer_adjust(execution_timer, TIME_IN_USEC(64), 0, TIME_IN_USEC(64));
}

void alpha8303_start(int state) { execute_set_input(0, state); }
void alpha8303_bus_dir(int state) { bus = state != 0; update_external_ram(); }
READ16_HANDLER( alpha8303_ram_r ) { return 0xff00 | external_ram[offset & 0x3ff]; }
WRITE16_HANDLER( alpha8303_ram_w ) { if (ACCESSING_LSB) external_ram[offset & 0x3ff] = data; }
