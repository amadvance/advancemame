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

MESSCPUDEFS += -DHAS_ARM=$(if $(filter ARM,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ARM7=$(if $(filter ARM7,$(MESSCPUS)),1,0)

ifneq ($(filter ARM,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/arm
MESSCPUOBJS += $(MESSOBJ)/cpu/arm/arm.o
MESSDBGOBJS += $(MESSOBJ)/cpu/arm/armdasm.o
$(MESSOBJ)/cpu/arm/arm.o: arm.c arm.h
endif

ifneq ($(filter ARM7,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/arm7
MESSCPUOBJS += $(MESSOBJ)/cpu/arm7/arm7.o
MESSDBGOBJS += $(MESSOBJ)/cpu/arm7/arm7dasm.o
$(MESSOBJ)/cpu/arm7/arm7.o: arm7.c arm7.h arm7exec.c
endif



#-------------------------------------------------
# Advanced Digital Chips SE3208
#-------------------------------------------------

MESSCPUDEFS += -DHAS_SE3208=$(if $(filter SE3208,$(MESSCPUS)),1,0)

ifneq ($(filter SE3208,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/se3208
MESSCPUOBJS += $(MESSOBJ)/cpu/se3208/se3208.o
MESSDBGOBJS += $(MESSOBJ)/cpu/se3208/se3208dis.o
$(MESSOBJ)/cpu/se3208/se3208.o: se3208.c se3208.h se3208dis.c
endif



#-------------------------------------------------
# Alpha 8201
#-------------------------------------------------

MESSCPUDEFS += -DHAS_ALPHA8201=$(if $(filter ALPHA8201,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ALPHA8301=$(if $(filter ALPHA8301,$(MESSCPUS)),1,0)

ifneq ($(filter ALPHA8201 ALPHA8301,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/alph8201
MESSCPUOBJS += $(MESSOBJ)/cpu/alph8201/alph8201.o
MESSDBGOBJS += $(MESSOBJ)/cpu/alph8201/8201dasm.o
$(MESSOBJ)/cpu/alph8201/alph8201.o: alph8201.c alph8201.h
endif



#-------------------------------------------------
# Analog Devices ADSP21xx series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_ADSP2100=$(if $(filter ADSP2100,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ADSP2101=$(if $(filter ADSP2101,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ADSP2104=$(if $(filter ADSP2104,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ADSP2105=$(if $(filter ADSP2105,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ADSP2115=$(if $(filter ADSP2115,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_ADSP2181=$(if $(filter ADSP2181,$(MESSCPUS)),1,0)

ifneq ($(filter ADSP2100 ADSP2101 ADSP2104 ADSP2105 ADSP2115 ADSP2181,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/adsp2100
MESSCPUOBJS += $(MESSOBJ)/cpu/adsp2100/adsp2100.o
MESSDBGOBJS += $(MESSOBJ)/cpu/adsp2100/2100dasm.o
$(MESSOBJ)/cpu/adsp2100/adsp2100.o: adsp2100.c adsp2100.h 2100ops.c
endif



#-------------------------------------------------
# Analog Devices "Sharc" ADSP21062
#-------------------------------------------------

MESSCPUDEFS += -DHAS_ADSP21062=$(if $(filter ADSP21062,$(MESSCPUS)),1,0)

ifneq ($(filter ADSP21062,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/sharc
MESSCPUOBJS += $(MESSOBJ)/cpu/sharc/sharc.o
MESSDBGOBJS += $(MESSOBJ)/cpu/sharc/sharcdsm.o
$(MESSOBJ)/cpu/sharc/sharc.o: sharc.c sharc.h sharcops.c sharcops.h sharcdsm.c sharcdsm.h compute.c
endif



#-------------------------------------------------
# AT&T DSP32C
#-------------------------------------------------

MESSCPUDEFS += -DHAS_DSP32C=$(if $(filter DSP32C,$(MESSCPUS)),1,0)

ifneq ($(filter DSP32C,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/dsp32
MESSCPUOBJS += $(MESSOBJ)/cpu/dsp32/dsp32.o
MESSDBGOBJS += $(MESSOBJ)/cpu/dsp32/dsp32dis.o
$(MESSOBJ)/cpu/dsp32/dsp32.o: dsp32.c dsp32.h
endif



#-------------------------------------------------
# Atari custom RISC processor
#-------------------------------------------------

MESSCPUDEFS += -DHAS_ASAP=$(if $(filter ASAP,$(MESSCPUS)),1,0)

ifneq ($(filter ASAP,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/asap
MESSCPUOBJS += $(MESSOBJ)/cpu/asap/asap.o
MESSDBGOBJS += $(MESSOBJ)/cpu/asap/asapdasm.o
$(MESSOBJ)/cpu/asap/asap.o: asap.c asap.h
endif



#-------------------------------------------------
# Atari Jaguar custom DSPs
#-------------------------------------------------

MESSCPUDEFS += -DHAS_JAGUAR=$(if $(filter JAGUAR,$(MESSCPUS)),1,0)

ifneq ($(filter JAGUAR,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/jaguar
MESSCPUOBJS += $(MESSOBJ)/cpu/jaguar/jaguar.o
MESSDBGOBJS += $(MESSOBJ)/cpu/jaguar/jagdasm.o
$(MESSOBJ)/cpu/jaguar/jaguar.o: jaguar.c jaguar.h
endif



#-------------------------------------------------
# Cinematronics vector "CPU"
#-------------------------------------------------

MESSCPUDEFS += -DHAS_CCPU=$(if $(filter CCPU,$(MESSCPUS)),1,0)

ifneq ($(filter CCPU,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/ccpu
MESSCPUOBJS += $(MESSOBJ)/cpu/ccpu/ccpu.o
MESSDBGOBJS += $(MESSOBJ)/cpu/ccpu/ccpudasm.o
$(MESSOBJ)/cpu/ccpu/ccpu.o: ccpu.c ccpu.h
endif



#-------------------------------------------------
# DEC T-11
#-------------------------------------------------

MESSCPUDEFS += -DHAS_T11=$(if $(filter T11,$(MESSCPUS)),1,0)

ifneq ($(filter T11,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/t11
MESSCPUOBJS += $(MESSOBJ)/cpu/t11/t11.o
MESSDBGOBJS += $(MESSOBJ)/cpu/t11/t11dasm.o
$(MESSOBJ)/cpu/t11/t11.o: t11.c t11.h t11ops.c t11table.c
endif



#-------------------------------------------------
# G65816
#-------------------------------------------------

MESSCPUDEFS += -DHAS_G65816=$(if $(filter G65816,$(MESSCPUS)),1,0)

ifneq ($(filter G65816,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/g65816
MESSCPUOBJS += $(MESSOBJ)/cpu/g65816/g65816.o
MESSCPUOBJS += $(MESSOBJ)/cpu/g65816/g65816o0.o
MESSCPUOBJS += $(MESSOBJ)/cpu/g65816/g65816o1.o
MESSCPUOBJS += $(MESSOBJ)/cpu/g65816/g65816o2.o
MESSCPUOBJS += $(MESSOBJ)/cpu/g65816/g65816o3.o
MESSCPUOBJS += $(MESSOBJ)/cpu/g65816/g65816o4.o
MESSDBGOBJS += $(MESSOBJ)/cpu/g65816/g65816ds.o
$(MESSOBJ)/cpu/g65816/g65816.o: g65816.c g65816.h g65816cm.h g65816op.h
$(MESSOBJ)/cpu/g65816/g65816o0.o: g65816o0.c g65816.h g65816cm.h g65816op.h
$(MESSOBJ)/cpu/g65816/g65816o1.o: g65816o1.c g65816.h g65816cm.h g65816op.h
$(MESSOBJ)/cpu/g65816/g65816o2.o: g65816o2.c g65816.h g65816cm.h g65816op.h
$(MESSOBJ)/cpu/g65816/g65816o3.o: g65816o3.c g65816.h g65816cm.h g65816op.h
$(MESSOBJ)/cpu/g65816/g65816o4.o: g65816o4.c g65816.h g65816cm.h g65816op.h
endif



#-------------------------------------------------
# Hitachi 6309
#-------------------------------------------------

MESSCPUDEFS += -DHAS_HD6309=$(if $(filter HD6309,$(MESSCPUS)),1,0)

ifneq ($(filter HD6309,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/hd6309
MESSCPUOBJS += $(MESSOBJ)/cpu/hd6309/hd6309.o
MESSDBGOBJS += $(MESSOBJ)/cpu/hd6309/6309dasm.o
$(MESSOBJ)/cpu/hd6309/hd6309.o: hd6309.c hd6309.h 6309ops.c 6309tbl.c
endif



#-------------------------------------------------
# Hitachi H8/3002
#-------------------------------------------------

MESSCPUDEFS += -DHAS_H83002=$(if $(filter H83002,$(MESSCPUS)),1,0)

ifneq ($(filter H83002,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/h83002
MESSCPUOBJS += $(MESSOBJ)/cpu/h83002/h83002.o $(MESSOBJ)/cpu/h83002/h8periph.o
MESSDBGOBJS += $(MESSOBJ)/cpu/h83002/h8disasm.o
$(MESSOBJ)/cpu/h83002/h83002.o: h83002.c h83002.h h8priv.h
$(MESSOBJ)/cpu/h83002/h8disasm.o: h8disasm.c
$(MESSOBJ)/cpu/h83002/h8periph.o: h8periph.c h8priv.h
endif



#-------------------------------------------------
# Hitachi SH2
#-------------------------------------------------

MESSCPUDEFS += -DHAS_SH2=$(if $(filter SH2,$(MESSCPUS)),1,0)

ifneq ($(filter SH2,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/sh2
MESSCPUOBJS += $(MESSOBJ)/cpu/sh2/sh2.o
MESSDBGOBJS += $(MESSOBJ)/cpu/sh2/sh2dasm.o
$(MESSOBJ)/cpu/sh2/sh2.o: sh2.c sh2.h
endif



#-------------------------------------------------
# Hudsonsoft 6280
#-------------------------------------------------

MESSCPUDEFS += -DHAS_H6280=$(if $(filter H6280,$(MESSCPUS)),1,0)

ifneq ($(filter H6280,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/h6280
MESSCPUOBJS += $(MESSOBJ)/cpu/h6280/h6280.o
MESSDBGOBJS += $(MESSOBJ)/cpu/h6280/6280dasm.o
$(MESSOBJ)/cpu/h6280/h6280.o: h6280.c h6280.h h6280ops.h tblh6280.c
endif



#-------------------------------------------------
# Hyperstone E1 series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_E116T=$(if $(filter E116T,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E116XT=$(if $(filter E116XT,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E116XS=$(if $(filter E116XS,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E116XSR=$(if $(filter E116XSR,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E132N=$(if $(filter E132N,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E132T=$(if $(filter E132T,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E132XN=$(if $(filter E132XN,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E132XT=$(if $(filter E132XT,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E132XS=$(if $(filter E132XS,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_E132XSR=$(if $(filter E132XSR,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_GMS30C2116=$(if $(filter GMS30C2116,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_GMS30C2132=$(if $(filter GMS30C2132,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_GMS30C2216=$(if $(filter GMS30C2216,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_GMS30C2232=$(if $(filter GMS30C2232,$(MESSCPUS)),1,0)

ifneq ($(filter E116T E116XT E116XS E116XSR E132N E132T E132XN E132XT E132XS E132XSR GMS30C2116 GMS30C2132 GMS30C2216 GMS30C2232,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/e132xs
MESSCPUOBJS += $(MESSOBJ)/cpu/e132xs/e132xs.o
MESSDBGOBJS += $(MESSOBJ)/cpu/e132xs/32xsdasm.o
$(MESSOBJ)/cpu/e132xs/e132xs.o: e132xs.c e132xs.h
endif



#-------------------------------------------------
# Intel 8080/8085A
#-------------------------------------------------

MESSCPUDEFS += -DHAS_8080=$(if $(filter 8080,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_8085A=$(if $(filter 8085A,$(MESSCPUS)),1,0)

ifneq ($(filter 8080 8085A,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i8085
MESSCPUOBJS += $(MESSOBJ)/cpu/i8085/i8085.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i8085/8085dasm.o
$(MESSOBJ)/cpu/i8085/i8085.o: i8085.c i8085.h i8085cpu.h i8085daa.h
endif



#-------------------------------------------------
# Intel 8039 and derivatives
#-------------------------------------------------

MESSCPUDEFS += -DHAS_I8035=$(if $(filter I8035,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I8039=$(if $(filter I8039,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I8048=$(if $(filter I8048,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_N7751=$(if $(filter N7751,$(MESSCPUS)),1,0)

ifneq ($(filter I8035 I8039 I8048 N7751,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i8039
MESSCPUOBJS += $(MESSOBJ)/cpu/i8039/i8039.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i8039/8039dasm.o
$(MESSOBJ)/cpu/i8039/i8039.o: i8039.c i8039.h
endif



#-------------------------------------------------
# Intel 8x41
#-------------------------------------------------

MESSCPUDEFS += -DHAS_I8X41=$(if $(filter I8X41,$(MESSCPUS)),1,0)

ifneq ($(filter I8X41,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i8x41
MESSCPUOBJS += $(MESSOBJ)/cpu/i8x41/i8x41.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i8x41/8x41dasm.o
$(MESSOBJ)/cpu/i8x41/i8x41.o: i8x41.c i8x41.h
endif



#-------------------------------------------------
# Intel 8051 and derivatives
#-------------------------------------------------

MESSCPUDEFS += -DHAS_I8051=$(if $(filter I8051,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I8052=$(if $(filter I8052,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I8751=$(if $(filter I8751,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I8752=$(if $(filter I8752,$(MESSCPUS)),1,0)

ifneq ($(filter I8051 I8052 I8751 I8752,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i8051
MESSCPUOBJS += $(MESSOBJ)/cpu/i8051/i8051.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i8051/8051dasm.o
$(MESSOBJ)/cpu/i8051/i8051.o: i8051.c i8051.h i8051ops.c
endif



#-------------------------------------------------
# Intel 8086 series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_I86=$(if $(filter I86,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I88=$(if $(filter I88,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I186=$(if $(filter I186,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I188=$(if $(filter I188,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I286=$(if $(filter I286,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I386=$(if $(filter I386,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_I486=$(if $(filter I486,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PENTIUM=$(if $(filter PENTIUM,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_MEDIAGX=$(if $(filter MEDIAGX,$(MESSCPUS)),1,0)

ifneq ($(filter I86 I88 I186 I188,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i86
MESSCPUOBJS += $(MESSOBJ)/cpu/i86/i86.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i386/i386dasm.o
$(MESSOBJ)/cpu/i86/i86.o: i86.c instr86.c instr186.c i86.h i86intf.h i186intf.h ea.h host.h modrm.h
endif

ifneq ($(filter I286,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i86
MESSCPUOBJS += $(MESSOBJ)/cpu/i86/i286.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i386/i386dasm.o
$(MESSOBJ)/cpu/i86/i86.o: i86.c instr286.c i86.h i286intf.h ea.h host.h modrm.h
endif

ifneq ($(filter I386 I486 PENTIUM MEDIAGX,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i386
MESSCPUOBJS += $(MESSOBJ)/cpu/i386/i386.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i386/i386dasm.o
$(MESSOBJ)/cpu/i386/i386.o: i386.c i386.h i386intf.h i386op16.c i386op32.c i386ops.c i486ops.c pentops.c x87ops.c i386ops.h cycles.h
endif



#-------------------------------------------------
# Intel i960
#-------------------------------------------------

MESSCPUDEFS += -DHAS_I960=$(if $(filter I960,$(MESSCPUS)),1,0)

ifneq ($(filter I960,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/i960
MESSCPUOBJS += $(MESSOBJ)/cpu/i960/i960.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i960/i960dis.o
$(MESSOBJ)/cpu/i960/i960.o: i960.c i960.h
endif



#-------------------------------------------------
# Konami custom CPU (6809-based)
#-------------------------------------------------

MESSCPUDEFS += -DHAS_KONAMI=$(if $(filter KONAMI,$(MESSCPUS)),1,0)

ifneq ($(filter KONAMI,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/konami
MESSCPUOBJS += $(MESSOBJ)/cpu/konami/konami.o
MESSDBGOBJS += $(MESSOBJ)/cpu/konami/knmidasm.o
$(MESSOBJ)/cpu/konami/konami.o: konami.c konami.h konamops.c konamtbl.c
endif



#-------------------------------------------------
# Microchip PIC16C5x
#-------------------------------------------------

MESSCPUDEFS += -DHAS_PIC16C54=$(if $(filter PIC16C54,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PIC16C55=$(if $(filter PIC16C55,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PIC16C56=$(if $(filter PIC16C56,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PIC16C57=$(if $(filter PIC16C57,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PIC16C58=$(if $(filter PIC16C58,$(MESSCPUS)),1,0)

ifneq ($(filter PIC16C54 PIC16C55 PIC16C56 PIC16C57 PIC16C58,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/pic16c5x
MESSCPUOBJS += $(MESSOBJ)/cpu/pic16c5x/pic16c5x.o
MESSDBGOBJS += $(MESSOBJ)/cpu/pic16c5x/16c5xdsm.o
$(MESSOBJ)/cpu/pic16c5x/pic16c5x.o: pic16c5x.c pic16c5x.h
endif



#-------------------------------------------------
# MIPS R3000 (MIPS I/II) series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_R3000=$(if $(filter R3000,$(MESSCPUS)),1,0)

ifneq ($(filter R3000,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/r3000
MESSCPUOBJS += $(MESSOBJ)/cpu/mips/r3000.o
MESSDBGOBJS += $(MESSOBJ)/cpu/mips/r3kdasm.o
$(MESSOBJ)/cpu/mips/r3000.o: r3000.c r3000.h
endif



#-------------------------------------------------
# MIPS R4000 (MIPS III/IV) series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_R4600=$(if $(filter R4600,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_R4650=$(if $(filter R4650,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_R4700=$(if $(filter R4700,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_R5000=$(if $(filter R5000,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_QED5271=$(if $(filter QED5271,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_RM7000=$(if $(filter RM7000,$(MESSCPUS)),1,0)

ifneq ($(filter R4600 R4650 R4700 R5000 QED5271 RM7000,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/mips
ifdef X86_MIPS3_DRC
MESSCPUOBJS += $(MESSOBJ)/cpu/mips/mips3drc.o
$(MESSOBJ)/cpu/mips/mips3drc.o: mips3drc.c mdrcold.c mips3.h
else
MESSCPUOBJS += $(MESSOBJ)/cpu/mips/mips3.o
$(MESSOBJ)/cpu/mips/mips3.o: mips3.c mips3.h
endif
MESSDBGOBJS += $(MESSOBJ)/cpu/mips/mips3dsm.o
endif



#-------------------------------------------------
# Mitsubishi M37702 and M37710 (based on 65C816)
#-------------------------------------------------

MESSCPUDEFS += -DHAS_M37702=$(if $(filter M37702,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M37710=$(if $(filter M37710,$(MESSCPUS)),1,0)

ifneq ($(filter M37702 M37710,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m37710
MESSCPUOBJS += $(MESSOBJ)/cpu/m37710/m37710.o
MESSCPUOBJS += $(MESSOBJ)/cpu/m37710/m37710o0.o
MESSCPUOBJS += $(MESSOBJ)/cpu/m37710/m37710o1.o
MESSCPUOBJS += $(MESSOBJ)/cpu/m37710/m37710o2.o
MESSCPUOBJS += $(MESSOBJ)/cpu/m37710/m37710o3.o
MESSCPUOBJS += $(MESSOBJ)/cpu/m37710/m7700ds.o
$(MESSOBJ)/cpu/m37710/m37710.o: m37710.c m37710.h m37710o0.c m37710o1.c m37710o2.c m37710o3.c m37710op.h m7700ds.h
$(MESSOBJ)/cpu/m37710/m37710o0.o: m37710.h m37710o0.c m37710op.h m7700ds.h
$(MESSOBJ)/cpu/m37710/m37710o1.o: m37710.h m37710o1.c m37710op.h m7700ds.h
$(MESSOBJ)/cpu/m37710/m37710o2.o: m37710.h m37710o2.c m37710op.h m7700ds.h
$(MESSOBJ)/cpu/m37710/m37710o3.o: m37710.h m37710o3.c m37710op.h m7700ds.h
$(MESSOBJ)/cpu/m37710/m7700ds.o: m7700ds.c m7700ds.h
endif



#-------------------------------------------------
# Mostek 6502 and its many derivatives
#-------------------------------------------------

MESSCPUDEFS += -DHAS_M6502=$(if $(filter M6502,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M65C02=$(if $(filter M65C02,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M65SC02=$(if $(filter M65SC02,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M65CE02=$(if $(filter M65CE02,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6509=$(if $(filter M6509,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6510=$(if $(filter M6510 M6510T M7501 M8502,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6510T=$(if $(filter M6510T,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M7501=$(if $(filter M7501,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M8502=$(if $(filter M8502,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_N2A03=$(if $(filter N2A03,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_DECO16=$(if $(filter DECO16,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M4510=$(if $(filter M4510,$(MESSCPUS)),1,0)

ifneq ($(filter M6502 M65C02 M65SC02 M6510 M6510T M7501 M8502 N2A03 DECO16,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6502
MESSCPUOBJS += $(MESSOBJ)/cpu/m6502/m6502.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6502/6502dasm.o
$(MESSOBJ)/cpu/m6502/m6502.o: m6502.c m6502.h ops02.h t6502.c t65c02.c t65sc02.c t6510.c tdeco16.c
endif

ifneq ($(filter M65CE02,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6502
MESSCPUOBJS += $(MESSOBJ)/cpu/m6502/m65ce02.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6502/6502dasm.o
$(MESSOBJ)/cpu/m6502/m65ce02.o: m65ce02.c m65ce02.h opsce02.h t65ce02.c
endif

ifneq ($(filter M6509,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6502
MESSCPUOBJS += $(MESSOBJ)/cpu/m6502/m6509.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6502/6502dasm.o
$(MESSOBJ)/cpu/m6502/m6509.o: m6509.c m6509.h ops09.h t6509.c
endif

ifneq ($(filter M4510,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6502
MESSCPUOBJS += $(MESSOBJ)/cpu/m6502/m4510.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6502/6502dasm.o
endif



#-------------------------------------------------
# Motorola 680x
#-------------------------------------------------

MESSCPUDEFS += -DHAS_M6800=$(if $(filter M6800,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6801=$(if $(filter M6801,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6802=$(if $(filter M6802,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6803=$(if $(filter M6803,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6808=$(if $(filter M6808,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_HD63701=$(if $(filter HD63701,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_NSC8105=$(if $(filter NSC8105,$(MESSCPUS)),1,0)

ifneq ($(filter M6800 M6801 M6802 M6803 M6808 HD63701 NSC8105,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6800
MESSCPUOBJS += $(MESSOBJ)/cpu/m6800/m6800.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6800/6800dasm.o
$(MESSOBJ)/cpu/m6800/m6800.o: m6800.c m6800.h 6800ops.c 6800tbl.c
endif



#-------------------------------------------------
# Motorola 6805
#-------------------------------------------------

MESSCPUDEFS += -DHAS_M6805=$(if $(filter M6805,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M68705=$(if $(filter M68705,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_HD63705=$(if $(filter HD63705,$(MESSCPUS)),1,0)

ifneq ($(filter M6805 M68705 HD63705,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6805
MESSCPUOBJS += $(MESSOBJ)/cpu/m6805/m6805.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6805/6805dasm.o
$(MESSOBJ)/cpu/m6805/m6805.o: m6805.c m6805.h 6805ops.c
endif



#-------------------------------------------------
# Motorola 6809
#-------------------------------------------------

MESSCPUDEFS += -DHAS_M6809=$(if $(filter M6809,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M6809E=$(if $(filter M6809E,$(MESSCPUS)),1,0)

ifneq ($(filter M6809 M6809E,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/m6809
MESSCPUOBJS += $(MESSOBJ)/cpu/m6809/m6809.o
MESSDBGOBJS += $(MESSOBJ)/cpu/m6809/6809dasm.o
$(MESSOBJ)/cpu/m6809/m6809.o: m6809.c m6809.h 6809ops.c 6809tbl.c
endif



#-------------------------------------------------
# Motorola 68HC11
#-------------------------------------------------

MESSCPUDEFS += -DHAS_MC68HC11=$(if $(filter MC68HC11,$(MESSCPUS)),1,0)

ifneq ($(filter MC68HC11,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/mc68hc11
MESSCPUOBJS += $(MESSOBJ)/cpu/mc68hc11/mc68hc11.o
MESSDBGOBJS += $(MESSOBJ)/cpu/mc68hc11/hc11dasm.o
$(MESSOBJ)/cpu/mc68hc11/mc68hc11.o: mc68hc11.c hc11dasm.c
endif



#-------------------------------------------------
# Motorola 68000 series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_M68000=$(if $(filter M68000,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M68008=$(if $(filter M68008,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M68010=$(if $(filter M68010,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M68EC020=$(if $(filter M68EC020,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M68020=$(if $(filter M68020,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_M68040=$(if $(filter M68040,$(MESSCPUS)),1,0)

ifneq ($(filter M68000 M68008 M68010 M68EC020 M68020 M68040,$(MESSCPUS)),)

# these files are generated by the core builder
MESSM68000_GENERATED_FILES = \
	$(MESSOBJ)/cpu/m68000/m68kops.c \
	$(MESSOBJ)/cpu/m68000/m68kopac.c \
	$(MESSOBJ)/cpu/m68000/m68kopdm.c \
	$(MESSOBJ)/cpu/m68000/m68kopnz.c

MESSM68000_GENERATED_HEADERS = \
	$(MESSOBJ)/cpu/m68000/m68kops.h

MESSOBJDIRS += $(MESSOBJ)/cpu/m68000
MESSCPUOBJS += $(MESSOBJ)/cpu/m68000/m68kcpu.o $(MESSOBJ)/cpu/m68000/m68kmame.o $(subst .c,.o,$(MESSM68000_GENERATED_FILES))
MESSDBGOBJS += $(MESSOBJ)/cpu/m68000/m68kdasm.o

# AdvanceMAME has its own rules for m68k, and then we comment the following ones
#
# when we compile source files we need to include generated files from the MESSOBJ directory
#$(MESSOBJ)/cpu/m68000/%.o: src/cpu/m68000/%.c
#	@echo Compiling $<...
#	$(CC) $(CDEFS) $(CFLAGS) -I$(MESSOBJ)/cpu/m68000 -c $< -o $@
#
# when we compile generated files we need to include stuff from the src directory
#$(MESSOBJ)/cpu/m68000/%.o: $(MESSOBJ)/cpu/m68000/%.c
#	@echo Compiling $<...
#	$(CC) $(CDEFS) $(CFLAGS) -Isrc/cpu/m68000 -c $< -o $@
#
# rule to generate the C files
#$(M68000_GENERATED_FILES) $(M68000_GENERATED_HEADERS): $(MESSOBJ)/cpu/m68000/m68kmake$(EXE) m68k_in.c
#	@echo Generating M68K source files...
#	$(MESSOBJ)/cpu/m68000/m68kmake$(EXE) $(MESSOBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c
#
# rule to build the generator
#$(MESSOBJ)/cpu/m68000/m68kmake$(EXE): $(MESSOBJ)/cpu/m68000/m68kmake.o $(OSDBGOBJS)
#
# rule to ensure we build the header before building the core CPU file
#$(MESSOBJ)/cpu/m68000/m68kcpu.o: $(M68000_GENERATED_HEADERS)

endif



#-------------------------------------------------
# Motorola/Freescale dsp56k
#-------------------------------------------------

MESSCPUDEFS += -DHAS_DSP56156=$(if $(filter DSP56156,$(MESSCPUS)),1,0)

ifneq ($(filter DSP56156,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/dsp56k
MESSCPUOBJS += $(MESSOBJ)/cpu/dsp56k/dsp56k.o
MESSDBGOBJS += $(MESSOBJ)/cpu/dsp56k/dsp56dsm.o
$(MESSOBJ)/cpu/dsp56k/dsp56k.o: dsp56k.c dsp56ops.c dsp56k.h
endif



#-------------------------------------------------
# Motorola PowerPC series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_PPC403=$(if $(filter PPC403,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PPC602=$(if $(filter PPC602,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_PPC603=$(if $(filter PPC603,$(MESSCPUS)),1,0)

ifneq ($(filter PPC403 PPC602 PPC603,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/powerpc
ifdef X86_PPC_DRC
MESSCPUOBJS += $(MESSOBJ)/cpu/powerpc/ppcdrc.o
$(MESSOBJ)/cpu/powerpc/ppcdrc.o: ppcdrc.c ppc.h drc_ops.c drc_ops.h ppc_ops.c ppc_mem.c ppc403.c ppc602.c ppc603.c
else
MESSCPUOBJS += $(MESSOBJ)/cpu/powerpc/ppc.o
$(MESSOBJ)/cpu/powerpc/ppc.o: ppc.c ppc.h ppc_ops.c ppc_ops.c ppc_mem.c ppc403.c ppc602.c ppc603.c
endif
MESSDBGOBJS += $(MESSOBJ)/cpu/powerpc/ppc_dasm.o
endif



#-------------------------------------------------
# NEC V-series Intel-compatible
#-------------------------------------------------

MESSCPUDEFS += -DHAS_V20=$(if $(filter V20,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_V30=$(if $(filter V30,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_V33=$(if $(filter V33,$(MESSCPUS)),1,0)

ifneq ($(filter V20 V30 V33,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/nec
MESSCPUOBJS += $(MESSOBJ)/cpu/nec/nec.o
MESSDBGOBJS += $(MESSOBJ)/cpu/i386/i386dasm.o
$(MESSOBJ)/cpu/nec/nec.o: nec.c nec.h necintrf.h necea.h nechost.h necinstr.h necmodrm.h
endif



#-------------------------------------------------
# NEC V60/V70
#-------------------------------------------------

MESSCPUDEFS += -DHAS_V60=$(if $(filter V60,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_V70=$(if $(filter V70,$(MESSCPUS)),1,0)

ifneq ($(filter V60 V70,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/v60
MESSCPUOBJS += $(MESSOBJ)/cpu/v60/v60.o
MESSDBGOBJS += $(MESSOBJ)/cpu/v60/v60d.o
$(MESSOBJ)/cpu/v60/v60.o: am.c am1.c am2.c am3.c op12.c op2.c op3.c op4.c op5.c op6.c op7a.c optable.c v60.c v60.h v60d.c
endif



#-------------------------------------------------
# NEC V810 (uPD70732)
#-------------------------------------------------

MESSCPUDEFS += -DHAS_V810=$(if $(filter V810,$(MESSCPUS)),1,0)

ifneq ($(filter V810,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/v810
MESSCPUOBJS += $(MESSOBJ)/cpu/v810/v810.o
MESSDBGOBJS += $(MESSOBJ)/cpu/v810/v810dasm.o
$(MESSOBJ)/cpu/v810/v810.o: v810.c v810.h
endif



#-------------------------------------------------
# NEC uPD7810 series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_UPD7810=$(if $(filter UPD7810,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_UPD7807=$(if $(filter UPD7807,$(MESSCPUS)),1,0)

ifneq ($(filter UPD7810 UPD7807,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/upd7810
MESSCPUOBJS += $(MESSOBJ)/cpu/upd7810/upd7810.o
MESSDBGOBJS += $(MESSOBJ)/cpu/upd7810/7810dasm.o
$(MESSOBJ)/cpu/upd7810/upd7810.o: upd7810.c 7810tbl.c 7810ops.c upd7810.h
endif



#-------------------------------------------------
# Nintendo/SGI RSP (R3000-based + vector processing)
#-------------------------------------------------

MESSCPUDEFS += -DHAS_RSP=$(if $(filter RSP,$(MESSCPUS)),1,0)

ifneq ($(filter RSP,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/rsp
MESSCPUOBJS += $(MESSOBJ)/cpu/rsp/rsp.o
MESSDBGOBJS += $(MESSOBJ)/cpu/rsp/rsp_dasm.o
$(MESSOBJ)/cpu/rsp/rsp.o: rsp.c rsp.h
endif



#-------------------------------------------------
# Signetics 2650
#-------------------------------------------------

MESSCPUDEFS += -DHAS_S2650=$(if $(filter S2650,$(MESSCPUS)),1,0)

ifneq ($(filter S2650,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/s2650
MESSCPUOBJS += $(MESSOBJ)/cpu/s2650/s2650.o
MESSDBGOBJS += $(MESSOBJ)/cpu/s2650/2650dasm.o
$(MESSOBJ)/cpu/s2650/s2650.o: s2650.c s2650.h s2650cpu.h
endif



#-------------------------------------------------
# Sony/Nintendo SPC700
#-------------------------------------------------

MESSCPUDEFS += -DHAS_SPC700=$(if $(filter SPC700,$(MESSCPUS)),1,0)

ifneq ($(filter SPC700,$(MESSCPUS)),)
SPCD = cpu/spc700
MESSOBJDIRS += $(MESSOBJ)/cpu/spc700
MESSCPUOBJS += $(MESSOBJ)/cpu/spc700/spc700.o
MESSDBGOBJS += $(MESSOBJ)/cpu/spc700/spc700ds.o
$(MESSOBJ)/cpu/spc700/spc700.o: spc700.c spc700.h
endif



#-------------------------------------------------
# Sony PlayStation CPU (R3000-based + GTE)
#-------------------------------------------------

MESSCPUDEFS += -DHAS_PSXCPU=$(if $(filter PSXCPU,$(MESSCPUS)),1,0)

ifneq ($(filter PSXCPU,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/mips
MESSCPUOBJS += $(MESSOBJ)/cpu/mips/psx.o
MESSDBGOBJS += $(MESSOBJ)/cpu/mips/mipsdasm.o
$(MESSOBJ)/cpu/mips/psx.o: psx.c psx.h
endif



#-------------------------------------------------
# Texas Instruments TMS99xx series
#-------------------------------------------------

MESSCPUDEFS += -DHAS_TMS9900=$(if $(filter TMS9900,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS9940=$(if $(filter TMS9940,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS9980=$(if $(filter TMS9980,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS9985=$(if $(filter TMS9985,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS9989=$(if $(filter TMS9989,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS9995=$(if $(filter TMS9995,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS99105A=$(if $(filter TMS99105A,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS99110A=$(if $(filter TMS99110A,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS99000=$(if $(filter TMS99000,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TI990_10=$(if $(filter TMS99010,$(MESSCPUS)),1,0)

ifneq ($(filter TMS9900 TMS9940 TMS99000,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms9900
MESSCPUOBJS += $(MESSOBJ)/cpu/tms9900/tms9900.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms9900/9900dasm.o
$(MESSOBJ)/cpu/tms9900/tms9900.o: tms9900.c tms9900.h 99xxcore.h 9900stat.h
endif

ifneq ($(filter TMS9980 TMS9985 TMS9989,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms9900
MESSCPUOBJS += $(MESSOBJ)/cpu/tms9900/tms9980a.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms9900/9900dasm.o
$(MESSOBJ)/cpu/tms9900/tms9980a.o: tms9980a.c tms9900.h 99xxcore.h 99xxstat.h
endif

ifneq ($(filter TMS9995 TMS99105A TMS99110A,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms9900
MESSCPUOBJS += $(MESSOBJ)/cpu/tms9900/tms9995.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms9900/9900dasm.o
$(MESSOBJ)/cpu/tms9900/tms9995.o: tms9995.c tms9900.h 99xxcore.h 99xxstat.h
endif

ifneq ($(filter TMS99010,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms9900
MESSCPUOBJS += $(MESSOBJ)/cpu/tms9900/ti990_10.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms9900/9900dasm.o
$(MESSOBJ)/cpu/tms9900/ti990_10.o: ti990_10.c tms9900.h 99xxcore.h 99xxstat.h
endif



#-------------------------------------------------
# Texas Instruments TMS340x0 graphics controllers
#-------------------------------------------------

MESSCPUDEFS += -DHAS_TMS34010=$(if $(filter TMS34010,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS34020=$(if $(filter TMS34020,$(MESSCPUS)),1,0)

ifneq ($(filter TMS34010 TMS34020,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms34010
MESSCPUOBJS += $(MESSOBJ)/cpu/tms34010/tms34010.o $(MESSOBJ)/cpu/tms34010/34010fld.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms34010/34010dsm.o
$(MESSOBJ)/cpu/tms34010/tms34010.o: tms34010.c tms34010.h 34010ops.c 34010gfx.c 34010tbl.c
endif



#-------------------------------------------------
# Texas Instruments TMS3201x DSP
#-------------------------------------------------

MESSCPUDEFS += -DHAS_TMS32010=$(if $(filter TMS32010,$(MESSCPUS)),1,0)

ifneq ($(filter TMS32010,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms32010
MESSCPUOBJS += $(MESSOBJ)/cpu/tms32010/tms32010.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms32010/32010dsm.o
$(MESSOBJ)/cpu/tms32010/tms32010.o: tms32010.c tms32010.h
endif



#-------------------------------------------------
# Texas Instruments TMS3202x DSP
#-------------------------------------------------

MESSCPUDEFS += -DHAS_TMS32025=$(if $(filter TMS32025,$(MESSCPUS)),1,0)
MESSCPUDEFS += -DHAS_TMS32026=$(if $(filter TMS32026,$(MESSCPUS)),1,0)

ifneq ($(filter TMS32025 TMS32026,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms32025
MESSCPUOBJS += $(MESSOBJ)/cpu/tms32025/tms32025.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms32025/32025dsm.o
$(MESSOBJ)/cpu/tms32025/tms32025.o: tms32025.c tms32025.h
endif



#-------------------------------------------------
# Texas Instruments TMS3203x DSP
#-------------------------------------------------

MESSCPUDEFS += -DHAS_TMS32031=$(if $(filter TMS32031,$(MESSCPUS)),1,0)

ifneq ($(filter TMS32031,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms32031
MESSCPUOBJS += $(MESSOBJ)/cpu/tms32031/tms32031.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms32031/dis32031.o
$(MESSOBJ)/cpu/tms32031/tms32031.o: tms32031.c tms32031.h
endif



#-------------------------------------------------
# Texas Instruments TMS3205x DSP
#-------------------------------------------------

MESSCPUDEFS += -DHAS_TMS32051=$(if $(filter TMS32051,$(MESSCPUS)),1,0)

ifneq ($(filter TMS32051,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/tms32051
MESSCPUOBJS += $(MESSOBJ)/cpu/tms32051/tms32051.o
MESSDBGOBJS += $(MESSOBJ)/cpu/tms32051/dis32051.o
$(MESSOBJ)/cpu/tms32051/tms32051.o: tms32051.c tms32051.h
endif



#-------------------------------------------------
# Zilog Z80
#-------------------------------------------------

MESSCPUDEFS += -DHAS_Z80=$(if $(filter Z80,$(MESSCPUS)),1,0)

ifneq ($(filter Z80,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/z80
MESSCPUOBJS += $(MESSOBJ)/cpu/z80/z80.o $(MESSOBJ)/cpu/z80/z80daisy.o
MESSDBGOBJS += $(MESSOBJ)/cpu/z80/z80dasm.o
$(MESSOBJ)/cpu/z80/z80.o: z80.c z80.h
endif



#-------------------------------------------------
# Zilog Z180
#-------------------------------------------------

MESSCPUDEFS += -DHAS_Z180=$(if $(filter Z180,$(MESSCPUS)),1,0)

ifneq ($(filter Z180,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/z180
MESSCPUOBJS += $(MESSOBJ)/cpu/z180/z180.o $(MESSOBJ)/cpu/z80/z80daisy.o
MESSDBGOBJS += $(MESSOBJ)/cpu/z180/z180dasm.o
$(MESSOBJ)/cpu/z180/z180.o: z180.c z180.h z180daa.h z180op.c z180ops.h z180tbl.h z180cb.c z180dd.c z180ed.c z180fd.c z180xy.c
endif



#-------------------------------------------------
# Zilog Z8000
#-------------------------------------------------

MESSCPUDEFS += -DHAS_Z8000=$(if $(filter Z8000,$(MESSCPUS)),1,0)
ifneq ($(filter Z8000,$(MESSCPUS)),)
MESSOBJDIRS += $(MESSOBJ)/cpu/z8000
MESSCPUOBJS += $(MESSOBJ)/cpu/z8000/z8000.o
MESSDBGOBJS += $(MESSOBJ)/cpu/z8000/8000dasm.o
$(MESSOBJ)/cpu/z8000/z8000.o: z8000.c z8000.h z8000cpu.h z8000dab.h z8000ops.c z8000tbl.c
endif
