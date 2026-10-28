// license:BSD-3-Clause
// copyright-holders:Andrew Gardner, Ryan Holtz

#ifndef MAME_CPU_DSP56156_DSP56DEF_H
#define MAME_CPU_DSP56156_DSP56DEF_H

#define X       cpustate->ALU.x.d
#define X1      cpustate->ALU.x.w.h
#define X0      cpustate->ALU.x.w.l
#define Y       cpustate->ALU.y.d
#define Y1      cpustate->ALU.y.w.h
#define Y0      cpustate->ALU.y.w.l

#define A       cpustate->ALU.a.lw
#define A2      cpustate->ALU.a.b.h4
#define A1      cpustate->ALU.a.w.h
#define A0      cpustate->ALU.a.w.l
#define B       cpustate->ALU.b.lw
#define B2      cpustate->ALU.b.b.h4
#define B1      cpustate->ALU.b.w.h
#define B0      cpustate->ALU.b.w.l

#define R0      cpustate->AGU.r0
#define R1      cpustate->AGU.r1
#define R2      cpustate->AGU.r2
#define R3      cpustate->AGU.r3

#define N0      cpustate->AGU.n0
#define N1      cpustate->AGU.n1
#define N2      cpustate->AGU.n2
#define N3      cpustate->AGU.n3

#define M0      cpustate->AGU.m0
#define M1      cpustate->AGU.m1
#define M2      cpustate->AGU.m2
#define M3      cpustate->AGU.m3

#define TEMP    cpustate->AGU.temp

#endif
