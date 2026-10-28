// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz
#ifndef DSP56CORE_H
#define DSP56CORE_H

typedef struct dsp56156_host_interface
{

	UINT16 *hcr;

	UINT16 *hsr;

	UINT16 *htrx;

	UINT8 icr;

	UINT8 cvr;

	UINT8 isr;

	UINT8 ivr;

	UINT8 trxh;
	UINT8 trxl;

	UINT16 bootstrap_offset;
} dsp56156_host_interface;

typedef struct dsp56156_data_alu
{

	PAIR x;
	PAIR y;

	PAIR64 a;
	PAIR64 b;

} dsp56156_data_alu;

typedef struct dsp56156_agu
{

	UINT16 r0;
	UINT16 r1;
	UINT16 r2;
	UINT16 r3;

	UINT16 n0;
	UINT16 n1;
	UINT16 n2;
	UINT16 n3;

	UINT16 m0;
	UINT16 m1;
	UINT16 m2;
	UINT16 m3;

	UINT16 temp;

} dsp56156_agu;

typedef struct dsp56156_pcu
{

	UINT16 pc;
	UINT16 la;
	UINT16 lc;
	UINT16 sr;
	UINT16 omr;
	UINT16 sp;
	PAIR     ss[16];

	void (*service_interrupts)(void);

	INT8 pending_interrupts[32];

	UINT16 reset_vector;
	UINT16 ipc;
} dsp56156_pcu;

typedef struct dsp56156_core
{

	dsp56156_pcu PCU;

	dsp56156_agu AGU;

	dsp56156_data_alu ALU;

	dsp56156_host_interface HI;

	int modA_state;
	int modB_state;
	int modC_state;
	int reset_state;

	UINT8 bootstrap_mode;

	UINT8   repFlag;
	UINT32  repAddr;

	int icount;

	UINT32          ppc;
	UINT32          op;
	int               interrupt_cycles;
	void              (*output_pins_changed)(UINT32 pins);
	void (*portc)(UINT16);

	UINT16 peripheral_ram[0x40];
	UINT16 *program_ram;
} dsp56156_core;

#endif
