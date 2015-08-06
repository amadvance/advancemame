###########################################################################
#
#   cpu.mak
#
#   Rules for building CPU cores
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# Acorn ARM series
#-------------------------------------------------

CPUDEFS += -DHAS_ARM=$(if $(filter ARM,$(CPUS)),1,0)
CPUDEFS += -DHAS_ARM7=$(if $(filter ARM7,$(CPUS)),1,0)

ifneq ($(filter ARM,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/arm
CPUOBJS += $(OBJ)/cpu/arm/arm.o
DBGOBJS += $(OBJ)/cpu/arm/armdasm.o
$(OBJ)/cpu/arm/arm.o: arm.c arm.h
endif

ifneq ($(filter ARM7,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/arm7
CPUOBJS += $(OBJ)/cpu/arm7/arm7.o
DBGOBJS += $(OBJ)/cpu/arm7/arm7dasm.o
$(OBJ)/cpu/arm7/arm7.o: arm7.c arm7.h arm7exec.c
endif



#-------------------------------------------------
# Advanced Digital Chips SE3208
#-------------------------------------------------

CPUDEFS += -DHAS_SE3208=$(if $(filter SE3208,$(CPUS)),1,0)

ifneq ($(filter SE3208,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/se3208
CPUOBJS += $(OBJ)/cpu/se3208/se3208.o
DBGOBJS += $(OBJ)/cpu/se3208/se3208dis.o
$(OBJ)/cpu/se3208/se3208.o: se3208.c se3208.h se3208dis.c
endif



#-------------------------------------------------
# Alpha 8201
#-------------------------------------------------

CPUDEFS += -DHAS_ALPHA8201=$(if $(filter ALPHA8201,$(CPUS)),1,0)
CPUDEFS += -DHAS_ALPHA8301=$(if $(filter ALPHA8301,$(CPUS)),1,0)

ifneq ($(filter ALPHA8201 ALPHA8301,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/alph8201
CPUOBJS += $(OBJ)/cpu/alph8201/alph8201.o
DBGOBJS += $(OBJ)/cpu/alph8201/8201dasm.o
$(OBJ)/cpu/alph8201/alph8201.o: alph8201.c alph8201.h
endif



#-------------------------------------------------
# Analog Devices ADSP21xx series
#-------------------------------------------------

CPUDEFS += -DHAS_ADSP2100=$(if $(filter ADSP2100,$(CPUS)),1,0)
CPUDEFS += -DHAS_ADSP2101=$(if $(filter ADSP2101,$(CPUS)),1,0)
CPUDEFS += -DHAS_ADSP2104=$(if $(filter ADSP2104,$(CPUS)),1,0)
CPUDEFS += -DHAS_ADSP2105=$(if $(filter ADSP2105,$(CPUS)),1,0)
CPUDEFS += -DHAS_ADSP2115=$(if $(filter ADSP2115,$(CPUS)),1,0)
CPUDEFS += -DHAS_ADSP2181=$(if $(filter ADSP2181,$(CPUS)),1,0)

ifneq ($(filter ADSP2100 ADSP2101 ADSP2104 ADSP2105 ADSP2115 ADSP2181,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/adsp2100
CPUOBJS += $(OBJ)/cpu/adsp2100/adsp2100.o
DBGOBJS += $(OBJ)/cpu/adsp2100/2100dasm.o
$(OBJ)/cpu/adsp2100/adsp2100.o: adsp2100.c adsp2100.h 2100ops.c
endif



#-------------------------------------------------
# Analog Devices "Sharc" ADSP21062
#-------------------------------------------------

CPUDEFS += -DHAS_ADSP21062=$(if $(filter ADSP21062,$(CPUS)),1,0)

ifneq ($(filter ADSP21062,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/sharc
CPUOBJS += $(OBJ)/cpu/sharc/sharc.o
DBGOBJS += $(OBJ)/cpu/sharc/sharcdsm.o
$(OBJ)/cpu/sharc/sharc.o: sharc.c sharc.h sharcops.c sharcops.h sharcdsm.c sharcdsm.h compute.c
endif



#-------------------------------------------------
# AT&T DSP32C
#-------------------------------------------------

CPUDEFS += -DHAS_DSP32C=$(if $(filter DSP32C,$(CPUS)),1,0)

ifneq ($(filter DSP32C,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/dsp32
CPUOBJS += $(OBJ)/cpu/dsp32/dsp32.o
DBGOBJS += $(OBJ)/cpu/dsp32/dsp32dis.o
$(OBJ)/cpu/dsp32/dsp32.o: dsp32.c dsp32.h
endif



#-------------------------------------------------
# Atari custom RISC processor
#-------------------------------------------------

CPUDEFS += -DHAS_ASAP=$(if $(filter ASAP,$(CPUS)),1,0)

ifneq ($(filter ASAP,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/asap
CPUOBJS += $(OBJ)/cpu/asap/asap.o
DBGOBJS += $(OBJ)/cpu/asap/asapdasm.o
$(OBJ)/cpu/asap/asap.o: asap.c asap.h
endif



#-------------------------------------------------
# Atari Jaguar custom DSPs
#-------------------------------------------------

CPUDEFS += -DHAS_JAGUAR=$(if $(filter JAGUAR,$(CPUS)),1,0)

ifneq ($(filter JAGUAR,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/jaguar
CPUOBJS += $(OBJ)/cpu/jaguar/jaguar.o
DBGOBJS += $(OBJ)/cpu/jaguar/jagdasm.o
$(OBJ)/cpu/jaguar/jaguar.o: jaguar.c jaguar.h
endif



#-------------------------------------------------
# Cinematronics vector "CPU"
#-------------------------------------------------

CPUDEFS += -DHAS_CCPU=$(if $(filter CCPU,$(CPUS)),1,0)

ifneq ($(filter CCPU,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/ccpu
CPUOBJS += $(OBJ)/cpu/ccpu/ccpu.o
DBGOBJS += $(OBJ)/cpu/ccpu/ccpudasm.o
$(OBJ)/cpu/ccpu/ccpu.o: ccpu.c ccpu.h
endif



#-------------------------------------------------
# DEC T-11
#-------------------------------------------------

CPUDEFS += -DHAS_T11=$(if $(filter T11,$(CPUS)),1,0)

ifneq ($(filter T11,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/t11
CPUOBJS += $(OBJ)/cpu/t11/t11.o
DBGOBJS += $(OBJ)/cpu/t11/t11dasm.o
$(OBJ)/cpu/t11/t11.o: t11.c t11.h t11ops.c t11table.c
endif



#-------------------------------------------------
# G65816
#-------------------------------------------------

CPUDEFS += -DHAS_G65816=$(if $(filter G65816,$(CPUS)),1,0)

ifneq ($(filter G65816,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/g65816
CPUOBJS += $(OBJ)/cpu/g65816/g65816.o
CPUOBJS += $(OBJ)/cpu/g65816/g65816o0.o
CPUOBJS += $(OBJ)/cpu/g65816/g65816o1.o
CPUOBJS += $(OBJ)/cpu/g65816/g65816o2.o
CPUOBJS += $(OBJ)/cpu/g65816/g65816o3.o
CPUOBJS += $(OBJ)/cpu/g65816/g65816o4.o
DBGOBJS += $(OBJ)/cpu/g65816/g65816ds.o
$(OBJ)/cpu/g65816/g65816.o: g65816.c g65816.h g65816cm.h g65816op.h
$(OBJ)/cpu/g65816/g65816o0.o: g65816o0.c g65816.h g65816cm.h g65816op.h
$(OBJ)/cpu/g65816/g65816o1.o: g65816o1.c g65816.h g65816cm.h g65816op.h
$(OBJ)/cpu/g65816/g65816o2.o: g65816o2.c g65816.h g65816cm.h g65816op.h
$(OBJ)/cpu/g65816/g65816o3.o: g65816o3.c g65816.h g65816cm.h g65816op.h
$(OBJ)/cpu/g65816/g65816o4.o: g65816o4.c g65816.h g65816cm.h g65816op.h
endif



#-------------------------------------------------
# Hitachi 6309
#-------------------------------------------------

CPUDEFS += -DHAS_HD6309=$(if $(filter HD6309,$(CPUS)),1,0)

ifneq ($(filter HD6309,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/hd6309
CPUOBJS += $(OBJ)/cpu/hd6309/hd6309.o
DBGOBJS += $(OBJ)/cpu/hd6309/6309dasm.o
$(OBJ)/cpu/hd6309/hd6309.o: hd6309.c hd6309.h 6309ops.c 6309tbl.c
endif



#-------------------------------------------------
# Hitachi H8/3002
#-------------------------------------------------

CPUDEFS += -DHAS_H83002=$(if $(filter H83002,$(CPUS)),1,0)

ifneq ($(filter H83002,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/h83002
CPUOBJS += $(OBJ)/cpu/h83002/h83002.o $(OBJ)/cpu/h83002/h8periph.o
DBGOBJS += $(OBJ)/cpu/h83002/h8disasm.o
$(OBJ)/cpu/h83002/h83002.o: h83002.c h83002.h h8priv.h
$(OBJ)/cpu/h83002/h8disasm.o: h8disasm.c
$(OBJ)/cpu/h83002/h8periph.o: h8periph.c h8priv.h
endif



#-------------------------------------------------
# Hitachi SH2
#-------------------------------------------------

CPUDEFS += -DHAS_SH2=$(if $(filter SH2,$(CPUS)),1,0)

ifneq ($(filter SH2,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/sh2
CPUOBJS += $(OBJ)/cpu/sh2/sh2.o
DBGOBJS += $(OBJ)/cpu/sh2/sh2dasm.o
$(OBJ)/cpu/sh2/sh2.o: sh2.c sh2.h
endif



#-------------------------------------------------
# Hudsonsoft 6280
#-------------------------------------------------

CPUDEFS += -DHAS_H6280=$(if $(filter H6280,$(CPUS)),1,0)

ifneq ($(filter H6280,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/h6280
CPUOBJS += $(OBJ)/cpu/h6280/h6280.o
DBGOBJS += $(OBJ)/cpu/h6280/6280dasm.o
$(OBJ)/cpu/h6280/h6280.o: h6280.c h6280.h h6280ops.h tblh6280.c
endif



#-------------------------------------------------
# Hyperstone E1 series
#-------------------------------------------------

CPUDEFS += -DHAS_E116T=$(if $(filter E116T,$(CPUS)),1,0)
CPUDEFS += -DHAS_E116XT=$(if $(filter E116XT,$(CPUS)),1,0)
CPUDEFS += -DHAS_E116XS=$(if $(filter E116XS,$(CPUS)),1,0)
CPUDEFS += -DHAS_E116XSR=$(if $(filter E116XSR,$(CPUS)),1,0)
CPUDEFS += -DHAS_E132N=$(if $(filter E132N,$(CPUS)),1,0)
CPUDEFS += -DHAS_E132T=$(if $(filter E132T,$(CPUS)),1,0)
CPUDEFS += -DHAS_E132XN=$(if $(filter E132XN,$(CPUS)),1,0)
CPUDEFS += -DHAS_E132XT=$(if $(filter E132XT,$(CPUS)),1,0)
CPUDEFS += -DHAS_E132XS=$(if $(filter E132XS,$(CPUS)),1,0)
CPUDEFS += -DHAS_E132XSR=$(if $(filter E132XSR,$(CPUS)),1,0)
CPUDEFS += -DHAS_GMS30C2116=$(if $(filter GMS30C2116,$(CPUS)),1,0)
CPUDEFS += -DHAS_GMS30C2132=$(if $(filter GMS30C2132,$(CPUS)),1,0)
CPUDEFS += -DHAS_GMS30C2216=$(if $(filter GMS30C2216,$(CPUS)),1,0)
CPUDEFS += -DHAS_GMS30C2232=$(if $(filter GMS30C2232,$(CPUS)),1,0)

ifneq ($(filter E116T E116XT E116XS E116XSR E132N E132T E132XN E132XT E132XS E132XSR GMS30C2116 GMS30C2132 GMS30C2216 GMS30C2232,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/e132xs
CPUOBJS += $(OBJ)/cpu/e132xs/e132xs.o
DBGOBJS += $(OBJ)/cpu/e132xs/32xsdasm.o
$(OBJ)/cpu/e132xs/e132xs.o: e132xs.c e132xs.h
endif



#-------------------------------------------------
# Intel 8080/8085A
#-------------------------------------------------

CPUDEFS += -DHAS_8080=$(if $(filter 8080,$(CPUS)),1,0)
CPUDEFS += -DHAS_8085A=$(if $(filter 8085A,$(CPUS)),1,0)

ifneq ($(filter 8080 8085A,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i8085
CPUOBJS += $(OBJ)/cpu/i8085/i8085.o
DBGOBJS += $(OBJ)/cpu/i8085/8085dasm.o
$(OBJ)/cpu/i8085/i8085.o: i8085.c i8085.h i8085cpu.h i8085daa.h
endif



#-------------------------------------------------
# Intel 8039 and derivatives
#-------------------------------------------------

CPUDEFS += -DHAS_I8035=$(if $(filter I8035,$(CPUS)),1,0)
CPUDEFS += -DHAS_I8039=$(if $(filter I8039,$(CPUS)),1,0)
CPUDEFS += -DHAS_I8048=$(if $(filter I8048,$(CPUS)),1,0)
CPUDEFS += -DHAS_N7751=$(if $(filter N7751,$(CPUS)),1,0)

ifneq ($(filter I8035 I8039 I8048 N7751,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i8039
CPUOBJS += $(OBJ)/cpu/i8039/i8039.o
DBGOBJS += $(OBJ)/cpu/i8039/8039dasm.o
$(OBJ)/cpu/i8039/i8039.o: i8039.c i8039.h
endif



#-------------------------------------------------
# Intel 8x41
#-------------------------------------------------

CPUDEFS += -DHAS_I8X41=$(if $(filter I8X41,$(CPUS)),1,0)

ifneq ($(filter I8X41,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i8x41
CPUOBJS += $(OBJ)/cpu/i8x41/i8x41.o
DBGOBJS += $(OBJ)/cpu/i8x41/8x41dasm.o
$(OBJ)/cpu/i8x41/i8x41.o: i8x41.c i8x41.h
endif



#-------------------------------------------------
# Intel 8051 and derivatives
#-------------------------------------------------

CPUDEFS += -DHAS_I8051=$(if $(filter I8051,$(CPUS)),1,0)
CPUDEFS += -DHAS_I8052=$(if $(filter I8052,$(CPUS)),1,0)
CPUDEFS += -DHAS_I8751=$(if $(filter I8751,$(CPUS)),1,0)
CPUDEFS += -DHAS_I8752=$(if $(filter I8752,$(CPUS)),1,0)

ifneq ($(filter I8051 I8052 I8751 I8752,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i8051
CPUOBJS += $(OBJ)/cpu/i8051/i8051.o
DBGOBJS += $(OBJ)/cpu/i8051/8051dasm.o
$(OBJ)/cpu/i8051/i8051.o: i8051.c i8051.h i8051ops.c
endif



#-------------------------------------------------
# Intel 8086 series
#-------------------------------------------------

CPUDEFS += -DHAS_I86=$(if $(filter I86,$(CPUS)),1,0)
CPUDEFS += -DHAS_I88=$(if $(filter I88,$(CPUS)),1,0)
CPUDEFS += -DHAS_I186=$(if $(filter I186,$(CPUS)),1,0)
CPUDEFS += -DHAS_I188=$(if $(filter I188,$(CPUS)),1,0)
CPUDEFS += -DHAS_I286=$(if $(filter I286,$(CPUS)),1,0)
CPUDEFS += -DHAS_I386=$(if $(filter I386,$(CPUS)),1,0)
CPUDEFS += -DHAS_I486=$(if $(filter I486,$(CPUS)),1,0)
CPUDEFS += -DHAS_PENTIUM=$(if $(filter PENTIUM,$(CPUS)),1,0)
CPUDEFS += -DHAS_MEDIAGX=$(if $(filter MEDIAGX,$(CPUS)),1,0)

ifneq ($(filter I86 I88 I186 I188,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i86
CPUOBJS += $(OBJ)/cpu/i86/i86.o
DBGOBJS += $(OBJ)/cpu/i386/i386dasm.o
$(OBJ)/cpu/i86/i86.o: i86.c instr86.c instr186.c i86.h i86intf.h i186intf.h ea.h host.h modrm.h
endif

ifneq ($(filter I286,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i86
CPUOBJS += $(OBJ)/cpu/i86/i286.o
DBGOBJS += $(OBJ)/cpu/i386/i386dasm.o
$(OBJ)/cpu/i86/i86.o: i86.c instr286.c i86.h i286intf.h ea.h host.h modrm.h
endif

ifneq ($(filter I386 I486 PENTIUM MEDIAGX,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i386
CPUOBJS += $(OBJ)/cpu/i386/i386.o
DBGOBJS += $(OBJ)/cpu/i386/i386dasm.o
$(OBJ)/cpu/i386/i386.o: i386.c i386.h i386intf.h i386op16.c i386op32.c i386ops.c i486ops.c pentops.c x87ops.c i386ops.h cycles.h
endif



#-------------------------------------------------
# Intel i960
#-------------------------------------------------

CPUDEFS += -DHAS_I960=$(if $(filter I960,$(CPUS)),1,0)

ifneq ($(filter I960,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/i960
CPUOBJS += $(OBJ)/cpu/i960/i960.o
DBGOBJS += $(OBJ)/cpu/i960/i960dis.o
$(OBJ)/cpu/i960/i960.o: i960.c i960.h
endif



#-------------------------------------------------
# Konami custom CPU (6809-based)
#-------------------------------------------------

CPUDEFS += -DHAS_KONAMI=$(if $(filter KONAMI,$(CPUS)),1,0)

ifneq ($(filter KONAMI,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/konami
CPUOBJS += $(OBJ)/cpu/konami/konami.o
DBGOBJS += $(OBJ)/cpu/konami/knmidasm.o
$(OBJ)/cpu/konami/konami.o: konami.c konami.h konamops.c konamtbl.c
endif



#-------------------------------------------------
# Microchip PIC16C5x
#-------------------------------------------------

CPUDEFS += -DHAS_PIC16C54=$(if $(filter PIC16C54,$(CPUS)),1,0)
CPUDEFS += -DHAS_PIC16C55=$(if $(filter PIC16C55,$(CPUS)),1,0)
CPUDEFS += -DHAS_PIC16C56=$(if $(filter PIC16C56,$(CPUS)),1,0)
CPUDEFS += -DHAS_PIC16C57=$(if $(filter PIC16C57,$(CPUS)),1,0)
CPUDEFS += -DHAS_PIC16C58=$(if $(filter PIC16C58,$(CPUS)),1,0)

ifneq ($(filter PIC16C54 PIC16C55 PIC16C56 PIC16C57 PIC16C58,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/pic16c5x
CPUOBJS += $(OBJ)/cpu/pic16c5x/pic16c5x.o
DBGOBJS += $(OBJ)/cpu/pic16c5x/16c5xdsm.o
$(OBJ)/cpu/pic16c5x/pic16c5x.o: pic16c5x.c pic16c5x.h
endif



#-------------------------------------------------
# MIPS R3000 (MIPS I/II) series
#-------------------------------------------------

CPUDEFS += -DHAS_R3000=$(if $(filter R3000,$(CPUS)),1,0)

ifneq ($(filter R3000,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/r3000
CPUOBJS += $(OBJ)/cpu/mips/r3000.o
DBGOBJS += $(OBJ)/cpu/mips/r3kdasm.o
$(OBJ)/cpu/mips/r3000.o: r3000.c r3000.h
endif



#-------------------------------------------------
# MIPS R4000 (MIPS III/IV) series
#-------------------------------------------------

CPUDEFS += -DHAS_R4600=$(if $(filter R4600,$(CPUS)),1,0)
CPUDEFS += -DHAS_R4650=$(if $(filter R4650,$(CPUS)),1,0)
CPUDEFS += -DHAS_R4700=$(if $(filter R4700,$(CPUS)),1,0)
CPUDEFS += -DHAS_R5000=$(if $(filter R5000,$(CPUS)),1,0)
CPUDEFS += -DHAS_QED5271=$(if $(filter QED5271,$(CPUS)),1,0)
CPUDEFS += -DHAS_RM7000=$(if $(filter RM7000,$(CPUS)),1,0)

ifneq ($(filter R4600 R4650 R4700 R5000 QED5271 RM7000,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/mips
ifdef X86_MIPS3_DRC
CPUOBJS += $(OBJ)/cpu/mips/mips3drc.o
$(OBJ)/cpu/mips/mips3drc.o: mips3drc.c mdrcold.c mips3.h
else
CPUOBJS += $(OBJ)/cpu/mips/mips3.o
$(OBJ)/cpu/mips/mips3.o: mips3.c mips3.h
endif
DBGOBJS += $(OBJ)/cpu/mips/mips3dsm.o
endif



#-------------------------------------------------
# Mitsubishi M37702 and M37710 (based on 65C816)
#-------------------------------------------------

CPUDEFS += -DHAS_M37702=$(if $(filter M37702,$(CPUS)),1,0)
CPUDEFS += -DHAS_M37710=$(if $(filter M37710,$(CPUS)),1,0)

ifneq ($(filter M37702 M37710,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m37710
CPUOBJS += $(OBJ)/cpu/m37710/m37710.o
CPUOBJS += $(OBJ)/cpu/m37710/m37710o0.o
CPUOBJS += $(OBJ)/cpu/m37710/m37710o1.o
CPUOBJS += $(OBJ)/cpu/m37710/m37710o2.o
CPUOBJS += $(OBJ)/cpu/m37710/m37710o3.o
CPUOBJS += $(OBJ)/cpu/m37710/m7700ds.o
$(OBJ)/cpu/m37710/m37710.o: m37710.c m37710.h m37710o0.c m37710o1.c m37710o2.c m37710o3.c m37710op.h m7700ds.h
$(OBJ)/cpu/m37710/m37710o0.o: m37710.h m37710o0.c m37710op.h m7700ds.h
$(OBJ)/cpu/m37710/m37710o1.o: m37710.h m37710o1.c m37710op.h m7700ds.h
$(OBJ)/cpu/m37710/m37710o2.o: m37710.h m37710o2.c m37710op.h m7700ds.h
$(OBJ)/cpu/m37710/m37710o3.o: m37710.h m37710o3.c m37710op.h m7700ds.h
$(OBJ)/cpu/m37710/m7700ds.o: m7700ds.c m7700ds.h
endif



#-------------------------------------------------
# Mostek 6502 and its many derivatives
#-------------------------------------------------

CPUDEFS += -DHAS_M6502=$(if $(filter M6502,$(CPUS)),1,0)
CPUDEFS += -DHAS_M65C02=$(if $(filter M65C02,$(CPUS)),1,0)
CPUDEFS += -DHAS_M65SC02=$(if $(filter M65SC02,$(CPUS)),1,0)
CPUDEFS += -DHAS_M65CE02=$(if $(filter M65CE02,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6509=$(if $(filter M6509,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6510=$(if $(filter M6510 M6510T M7501 M8502,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6510T=$(if $(filter M6510T,$(CPUS)),1,0)
CPUDEFS += -DHAS_M7501=$(if $(filter M7501,$(CPUS)),1,0)
CPUDEFS += -DHAS_M8502=$(if $(filter M8502,$(CPUS)),1,0)
CPUDEFS += -DHAS_N2A03=$(if $(filter N2A03,$(CPUS)),1,0)
CPUDEFS += -DHAS_DECO16=$(if $(filter DECO16,$(CPUS)),1,0)
CPUDEFS += -DHAS_M4510=$(if $(filter M4510,$(CPUS)),1,0)

ifneq ($(filter M6502 M65C02 M65SC02 M6510 M6510T M7501 M8502 N2A03 DECO16,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6502
CPUOBJS += $(OBJ)/cpu/m6502/m6502.o
DBGOBJS += $(OBJ)/cpu/m6502/6502dasm.o
$(OBJ)/cpu/m6502/m6502.o: m6502.c m6502.h ops02.h t6502.c t65c02.c t65sc02.c t6510.c tdeco16.c
endif

ifneq ($(filter M65CE02,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6502
CPUOBJS += $(OBJ)/cpu/m6502/m65ce02.o
DBGOBJS += $(OBJ)/cpu/m6502/6502dasm.o
$(OBJ)/cpu/m6502/m65ce02.o: m65ce02.c m65ce02.h opsce02.h t65ce02.c
endif

ifneq ($(filter M6509,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6502
CPUOBJS += $(OBJ)/cpu/m6502/m6509.o
DBGOBJS += $(OBJ)/cpu/m6502/6502dasm.o
$(OBJ)/cpu/m6502/m6509.o: m6509.c m6509.h ops09.h t6509.c
endif

ifneq ($(filter M4510,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6502
CPUOBJS += $(OBJ)/cpu/m6502/m4510.o
DBGOBJS += $(OBJ)/cpu/m6502/6502dasm.o
endif



#-------------------------------------------------
# Motorola 680x
#-------------------------------------------------

CPUDEFS += -DHAS_M6800=$(if $(filter M6800,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6801=$(if $(filter M6801,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6802=$(if $(filter M6802,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6803=$(if $(filter M6803,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6808=$(if $(filter M6808,$(CPUS)),1,0)
CPUDEFS += -DHAS_HD63701=$(if $(filter HD63701,$(CPUS)),1,0)
CPUDEFS += -DHAS_NSC8105=$(if $(filter NSC8105,$(CPUS)),1,0)

ifneq ($(filter M6800 M6801 M6802 M6803 M6808 HD63701 NSC8105,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6800
CPUOBJS += $(OBJ)/cpu/m6800/m6800.o
DBGOBJS += $(OBJ)/cpu/m6800/6800dasm.o
$(OBJ)/cpu/m6800/m6800.o: m6800.c m6800.h 6800ops.c 6800tbl.c
endif



#-------------------------------------------------
# Motorola 6805
#-------------------------------------------------

CPUDEFS += -DHAS_M6805=$(if $(filter M6805,$(CPUS)),1,0)
CPUDEFS += -DHAS_M68705=$(if $(filter M68705,$(CPUS)),1,0)
CPUDEFS += -DHAS_HD63705=$(if $(filter HD63705,$(CPUS)),1,0)

ifneq ($(filter M6805 M68705 HD63705,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6805
CPUOBJS += $(OBJ)/cpu/m6805/m6805.o
DBGOBJS += $(OBJ)/cpu/m6805/6805dasm.o
$(OBJ)/cpu/m6805/m6805.o: m6805.c m6805.h 6805ops.c
endif



#-------------------------------------------------
# Motorola 6809
#-------------------------------------------------

CPUDEFS += -DHAS_M6809=$(if $(filter M6809,$(CPUS)),1,0)
CPUDEFS += -DHAS_M6809E=$(if $(filter M6809E,$(CPUS)),1,0)

ifneq ($(filter M6809 M6809E,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/m6809
CPUOBJS += $(OBJ)/cpu/m6809/m6809.o
DBGOBJS += $(OBJ)/cpu/m6809/6809dasm.o
$(OBJ)/cpu/m6809/m6809.o: m6809.c m6809.h 6809ops.c 6809tbl.c
endif



#-------------------------------------------------
# Motorola 68HC11
#-------------------------------------------------

CPUDEFS += -DHAS_MC68HC11=$(if $(filter MC68HC11,$(CPUS)),1,0)

ifneq ($(filter MC68HC11,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/mc68hc11
CPUOBJS += $(OBJ)/cpu/mc68hc11/mc68hc11.o
DBGOBJS += $(OBJ)/cpu/mc68hc11/hc11dasm.o
$(OBJ)/cpu/mc68hc11/mc68hc11.o: mc68hc11.c hc11dasm.c
endif



#-------------------------------------------------
# Motorola 68000 series
#-------------------------------------------------

CPUDEFS += -DHAS_M68000=$(if $(filter M68000,$(CPUS)),1,0)
CPUDEFS += -DHAS_M68008=$(if $(filter M68008,$(CPUS)),1,0)
CPUDEFS += -DHAS_M68010=$(if $(filter M68010,$(CPUS)),1,0)
CPUDEFS += -DHAS_M68EC020=$(if $(filter M68EC020,$(CPUS)),1,0)
CPUDEFS += -DHAS_M68020=$(if $(filter M68020,$(CPUS)),1,0)
CPUDEFS += -DHAS_M68040=$(if $(filter M68040,$(CPUS)),1,0)

ifneq ($(filter M68000 M68008 M68010 M68EC020 M68020 M68040,$(CPUS)),)

# these files are generated by the core builder
M68000_GENERATED_FILES = \
	$(OBJ)/cpu/m68000/m68kops.c \
	$(OBJ)/cpu/m68000/m68kopac.c \
	$(OBJ)/cpu/m68000/m68kopdm.c \
	$(OBJ)/cpu/m68000/m68kopnz.c

M68000_GENERATED_HEADERS = \
	$(OBJ)/cpu/m68000/m68kops.h

OBJDIRS += $(OBJ)/cpu/m68000
CPUOBJS += $(OBJ)/cpu/m68000/m68kcpu.o $(OBJ)/cpu/m68000/m68kmame.o $(subst .c,.o,$(M68000_GENERATED_FILES))
DBGOBJS += $(OBJ)/cpu/m68000/m68kdasm.o

# when we compile source files we need to include generated files from the OBJ directory
$(OBJ)/cpu/m68000/%.o: src/cpu/m68000/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -I$(OBJ)/cpu/m68000 -c $< -o $@

# when we compile generated files we need to include stuff from the src directory
$(OBJ)/cpu/m68000/%.o: $(OBJ)/cpu/m68000/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -Isrc/cpu/m68000 -c $< -o $@

# rule to generate the C files
$(M68000_GENERATED_FILES) $(M68000_GENERATED_HEADERS): $(OBJ)/cpu/m68000/m68kmake$(EXE) m68k_in.c
	@echo Generating M68K source files...
	$(OBJ)/cpu/m68000/m68kmake$(EXE) $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# rule to build the generator
$(OBJ)/cpu/m68000/m68kmake$(EXE): $(OBJ)/cpu/m68000/m68kmake.o $(OSDBGOBJS)

# rule to ensure we build the header before building the core CPU file
$(OBJ)/cpu/m68000/m68kcpu.o: $(M68000_GENERATED_HEADERS)

endif



#-------------------------------------------------
# Motorola/Freescale dsp56k
#-------------------------------------------------

CPUDEFS += -DHAS_DSP56156=$(if $(filter DSP56156,$(CPUS)),1,0)

ifneq ($(filter DSP56156,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/dsp56k
CPUOBJS += $(OBJ)/cpu/dsp56k/dsp56k.o
DBGOBJS += $(OBJ)/cpu/dsp56k/dsp56dsm.o
$(OBJ)/cpu/dsp56k/dsp56k.o: dsp56k.c dsp56ops.c dsp56k.h
endif



#-------------------------------------------------
# Motorola PowerPC series
#-------------------------------------------------

CPUDEFS += -DHAS_PPC403=$(if $(filter PPC403,$(CPUS)),1,0)
CPUDEFS += -DHAS_PPC602=$(if $(filter PPC602,$(CPUS)),1,0)
CPUDEFS += -DHAS_PPC603=$(if $(filter PPC603,$(CPUS)),1,0)

ifneq ($(filter PPC403 PPC602 PPC603,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/powerpc
ifdef X86_PPC_DRC
CPUOBJS += $(OBJ)/cpu/powerpc/ppcdrc.o
$(OBJ)/cpu/powerpc/ppcdrc.o: ppcdrc.c ppc.h drc_ops.c drc_ops.h ppc_ops.c ppc_mem.c ppc403.c ppc602.c ppc603.c
else
CPUOBJS += $(OBJ)/cpu/powerpc/ppc.o
$(OBJ)/cpu/powerpc/ppc.o: ppc.c ppc.h ppc_ops.c ppc_ops.c ppc_mem.c ppc403.c ppc602.c ppc603.c
endif
DBGOBJS += $(OBJ)/cpu/powerpc/ppc_dasm.o
endif



#-------------------------------------------------
# NEC V-series Intel-compatible
#-------------------------------------------------

CPUDEFS += -DHAS_V20=$(if $(filter V20,$(CPUS)),1,0)
CPUDEFS += -DHAS_V30=$(if $(filter V30,$(CPUS)),1,0)
CPUDEFS += -DHAS_V33=$(if $(filter V33,$(CPUS)),1,0)

ifneq ($(filter V20 V30 V33,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/nec
CPUOBJS += $(OBJ)/cpu/nec/nec.o
DBGOBJS += $(OBJ)/cpu/i386/i386dasm.o
$(OBJ)/cpu/nec/nec.o: nec.c nec.h necintrf.h necea.h nechost.h necinstr.h necmodrm.h
endif



#-------------------------------------------------
# NEC V60/V70
#-------------------------------------------------

CPUDEFS += -DHAS_V60=$(if $(filter V60,$(CPUS)),1,0)
CPUDEFS += -DHAS_V70=$(if $(filter V70,$(CPUS)),1,0)

ifneq ($(filter V60 V70,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/v60
CPUOBJS += $(OBJ)/cpu/v60/v60.o
DBGOBJS += $(OBJ)/cpu/v60/v60d.o
$(OBJ)/cpu/v60/v60.o: am.c am1.c am2.c am3.c op12.c op2.c op3.c op4.c op5.c op6.c op7a.c optable.c v60.c v60.h v60d.c
endif



#-------------------------------------------------
# NEC V810 (uPD70732)
#-------------------------------------------------

CPUDEFS += -DHAS_V810=$(if $(filter V810,$(CPUS)),1,0)

ifneq ($(filter V810,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/v810
CPUOBJS += $(OBJ)/cpu/v810/v810.o
DBGOBJS += $(OBJ)/cpu/v810/v810dasm.o
$(OBJ)/cpu/v810/v810.o: v810.c v810.h
endif



#-------------------------------------------------
# NEC uPD7810 series
#-------------------------------------------------

CPUDEFS += -DHAS_UPD7810=$(if $(filter UPD7810,$(CPUS)),1,0)
CPUDEFS += -DHAS_UPD7807=$(if $(filter UPD7807,$(CPUS)),1,0)

ifneq ($(filter UPD7810 UPD7807,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/upd7810
CPUOBJS += $(OBJ)/cpu/upd7810/upd7810.o
DBGOBJS += $(OBJ)/cpu/upd7810/7810dasm.o
$(OBJ)/cpu/upd7810/upd7810.o: upd7810.c 7810tbl.c 7810ops.c upd7810.h
endif



#-------------------------------------------------
# Nintendo/SGI RSP (R3000-based + vector processing)
#-------------------------------------------------

CPUDEFS += -DHAS_RSP=$(if $(filter RSP,$(CPUS)),1,0)

ifneq ($(filter RSP,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/rsp
CPUOBJS += $(OBJ)/cpu/rsp/rsp.o
DBGOBJS += $(OBJ)/cpu/rsp/rsp_dasm.o
$(OBJ)/cpu/rsp/rsp.o: rsp.c rsp.h
endif



#-------------------------------------------------
# Signetics 2650
#-------------------------------------------------

CPUDEFS += -DHAS_S2650=$(if $(filter S2650,$(CPUS)),1,0)

ifneq ($(filter S2650,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/s2650
CPUOBJS += $(OBJ)/cpu/s2650/s2650.o
DBGOBJS += $(OBJ)/cpu/s2650/2650dasm.o
$(OBJ)/cpu/s2650/s2650.o: s2650.c s2650.h s2650cpu.h
endif



#-------------------------------------------------
# Sony/Nintendo SPC700
#-------------------------------------------------

CPUDEFS += -DHAS_SPC700=$(if $(filter SPC700,$(CPUS)),1,0)

ifneq ($(filter SPC700,$(CPUS)),)
SPCD = cpu/spc700
OBJDIRS += $(OBJ)/cpu/spc700
CPUOBJS += $(OBJ)/cpu/spc700/spc700.o
DBGOBJS += $(OBJ)/cpu/spc700/spc700ds.o
$(OBJ)/cpu/spc700/spc700.o: spc700.c spc700.h
endif



#-------------------------------------------------
# Sony PlayStation CPU (R3000-based + GTE)
#-------------------------------------------------

CPUDEFS += -DHAS_PSXCPU=$(if $(filter PSXCPU,$(CPUS)),1,0)

ifneq ($(filter PSXCPU,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/mips
CPUOBJS += $(OBJ)/cpu/mips/psx.o
DBGOBJS += $(OBJ)/cpu/mips/mipsdasm.o
$(OBJ)/cpu/mips/psx.o: psx.c psx.h
endif



#-------------------------------------------------
# Texas Instruments TMS99xx series
#-------------------------------------------------

CPUDEFS += -DHAS_TMS9900=$(if $(filter TMS9900,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS9940=$(if $(filter TMS9940,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS9980=$(if $(filter TMS9980,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS9985=$(if $(filter TMS9985,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS9989=$(if $(filter TMS9989,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS9995=$(if $(filter TMS9995,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS99105A=$(if $(filter TMS99105A,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS99110A=$(if $(filter TMS99110A,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS99000=$(if $(filter TMS99000,$(CPUS)),1,0)
CPUDEFS += -DHAS_TI990_10=$(if $(filter TMS99010,$(CPUS)),1,0)

ifneq ($(filter TMS9900 TMS9940 TMS99000,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms9900
CPUOBJS += $(OBJ)/cpu/tms9900/tms9900.o
DBGOBJS += $(OBJ)/cpu/tms9900/9900dasm.o
$(OBJ)/cpu/tms9900/tms9900.o: tms9900.c tms9900.h 99xxcore.h 9900stat.h
endif

ifneq ($(filter TMS9980 TMS9985 TMS9989,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms9900
CPUOBJS += $(OBJ)/cpu/tms9900/tms9980a.o
DBGOBJS += $(OBJ)/cpu/tms9900/9900dasm.o
$(OBJ)/cpu/tms9900/tms9980a.o: tms9980a.c tms9900.h 99xxcore.h 99xxstat.h
endif

ifneq ($(filter TMS9995 TMS99105A TMS99110A,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms9900
CPUOBJS += $(OBJ)/cpu/tms9900/tms9995.o
DBGOBJS += $(OBJ)/cpu/tms9900/9900dasm.o
$(OBJ)/cpu/tms9900/tms9995.o: tms9995.c tms9900.h 99xxcore.h 99xxstat.h
endif

ifneq ($(filter TMS99010,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms9900
CPUOBJS += $(OBJ)/cpu/tms9900/ti990_10.o
DBGOBJS += $(OBJ)/cpu/tms9900/9900dasm.o
$(OBJ)/cpu/tms9900/ti990_10.o: ti990_10.c tms9900.h 99xxcore.h 99xxstat.h
endif



#-------------------------------------------------
# Texas Instruments TMS340x0 graphics controllers
#-------------------------------------------------

CPUDEFS += -DHAS_TMS34010=$(if $(filter TMS34010,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS34020=$(if $(filter TMS34020,$(CPUS)),1,0)

ifneq ($(filter TMS34010 TMS34020,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms34010
CPUOBJS += $(OBJ)/cpu/tms34010/tms34010.o $(OBJ)/cpu/tms34010/34010fld.o
DBGOBJS += $(OBJ)/cpu/tms34010/34010dsm.o
$(OBJ)/cpu/tms34010/tms34010.o: tms34010.c tms34010.h 34010ops.c 34010gfx.c 34010tbl.c
endif



#-------------------------------------------------
# Texas Instruments TMS3201x DSP
#-------------------------------------------------

CPUDEFS += -DHAS_TMS32010=$(if $(filter TMS32010,$(CPUS)),1,0)

ifneq ($(filter TMS32010,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms32010
CPUOBJS += $(OBJ)/cpu/tms32010/tms32010.o
DBGOBJS += $(OBJ)/cpu/tms32010/32010dsm.o
$(OBJ)/cpu/tms32010/tms32010.o: tms32010.c tms32010.h
endif



#-------------------------------------------------
# Texas Instruments TMS3202x DSP
#-------------------------------------------------

CPUDEFS += -DHAS_TMS32025=$(if $(filter TMS32025,$(CPUS)),1,0)
CPUDEFS += -DHAS_TMS32026=$(if $(filter TMS32026,$(CPUS)),1,0)

ifneq ($(filter TMS32025 TMS32026,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms32025
CPUOBJS += $(OBJ)/cpu/tms32025/tms32025.o
DBGOBJS += $(OBJ)/cpu/tms32025/32025dsm.o
$(OBJ)/cpu/tms32025/tms32025.o: tms32025.c tms32025.h
endif



#-------------------------------------------------
# Texas Instruments TMS3203x DSP
#-------------------------------------------------

CPUDEFS += -DHAS_TMS32031=$(if $(filter TMS32031,$(CPUS)),1,0)

ifneq ($(filter TMS32031,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms32031
CPUOBJS += $(OBJ)/cpu/tms32031/tms32031.o
DBGOBJS += $(OBJ)/cpu/tms32031/dis32031.o
$(OBJ)/cpu/tms32031/tms32031.o: tms32031.c tms32031.h
endif



#-------------------------------------------------
# Texas Instruments TMS3205x DSP
#-------------------------------------------------

CPUDEFS += -DHAS_TMS32051=$(if $(filter TMS32051,$(CPUS)),1,0)

ifneq ($(filter TMS32051,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/tms32051
CPUOBJS += $(OBJ)/cpu/tms32051/tms32051.o
DBGOBJS += $(OBJ)/cpu/tms32051/dis32051.o
$(OBJ)/cpu/tms32051/tms32051.o: tms32051.c tms32051.h
endif



#-------------------------------------------------
# Zilog Z80
#-------------------------------------------------

CPUDEFS += -DHAS_Z80=$(if $(filter Z80,$(CPUS)),1,0)

ifneq ($(filter Z80,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/z80
CPUOBJS += $(OBJ)/cpu/z80/z80.o $(OBJ)/cpu/z80/z80daisy.o
DBGOBJS += $(OBJ)/cpu/z80/z80dasm.o
$(OBJ)/cpu/z80/z80.o: z80.c z80.h
endif



#-------------------------------------------------
# Zilog Z180
#-------------------------------------------------

CPUDEFS += -DHAS_Z180=$(if $(filter Z180,$(CPUS)),1,0)

ifneq ($(filter Z180,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/z180
CPUOBJS += $(OBJ)/cpu/z180/z180.o $(OBJ)/cpu/z80/z80daisy.o
DBGOBJS += $(OBJ)/cpu/z180/z180dasm.o
$(OBJ)/cpu/z180/z180.o: z180.c z180.h z180daa.h z180op.c z180ops.h z180tbl.h z180cb.c z180dd.c z180ed.c z180fd.c z180xy.c
endif



#-------------------------------------------------
# Zilog Z8000
#-------------------------------------------------

CPUDEFS += -DHAS_Z8000=$(if $(filter Z8000,$(CPUS)),1,0)
ifneq ($(filter Z8000,$(CPUS)),)
OBJDIRS += $(OBJ)/cpu/z8000
CPUOBJS += $(OBJ)/cpu/z8000/z8000.o
DBGOBJS += $(OBJ)/cpu/z8000/8000dasm.o
$(OBJ)/cpu/z8000/z8000.o: z8000.c z8000.h z8000cpu.h z8000dab.h z8000ops.c z8000tbl.c
endif
