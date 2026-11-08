# license:BSD-3-Clause
# copyright-holders:Aaron Giles, Nicola Salmoria
# Shared rules: call with variable prefix, object directory and target name.
# Escape ordinary make references so each instantiation retains its own variables.
define EMU_CPU
###########################################################################
#
#   cpu.mak
#
#   Rules for building CPU cores
#
###########################################################################


#-------------------------------------------------
# Acorn ARM series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_ARM=$$(if $$(filter ARM,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ARM7=$$(if $$(filter ARM7,$$($(1)CPUS)),1,0)

ifneq ($$(filter ARM,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/arm
$(1)CPUOBJS += $(2)/cpu/arm/arm.o
$(1)DBGOBJS += $(2)/cpu/arm/armdasm.o
$(2)/cpu/arm/arm.o: arm.c arm.h
endif

ifneq ($$(filter ARM7,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/arm7
$(1)CPUOBJS += $(2)/cpu/arm7/arm7.o
$(1)DBGOBJS += $(2)/cpu/arm7/arm7dasm.o
$(2)/cpu/arm7/arm7.o: arm7.c arm7.h arm7core.c arm7core.h arm7exec.c
endif


# IGS036 (ARM946E-S), C port of MAME 0.287
$(1)CPUDEFS += -DHAS_IGS036=$$(if $$(filter IGS036,$$($(1)CPUS)),1,0)
ifneq ($$(filter IGS036,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/igs036
$(1)CPUOBJS += $(2)/cpu/igs036/igs036.o
ifeq ($$(filter ARM7,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/arm7
$(1)DBGOBJS += $(2)/cpu/arm7/arm7dasm.o
endif
$(2)/cpu/igs036/igs036.o: igs036.c igs036.h igs036core.h igs036help.h igs036ops.c igs036thumb.c igs036irq.c
endif



#-------------------------------------------------
# Advanced Digital Chips SE3208
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_SE3208=$$(if $$(filter SE3208,$$($(1)CPUS)),1,0)

ifneq ($$(filter SE3208,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/se3208
$(1)CPUOBJS += $(2)/cpu/se3208/se3208.o
$(1)DBGOBJS += $(2)/cpu/se3208/se3208dis.o
$(2)/cpu/se3208/se3208.o: se3208.c se3208.h se3208dis.c
endif



#-------------------------------------------------
# Alpha 8201
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_ALPHA8201=$$(if $$(filter ALPHA8201,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ALPHA8301=$$(if $$(filter ALPHA8301,$$($(1)CPUS)),1,0)

ifneq ($$(filter ALPHA8201 ALPHA8301,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/alph8201
$(1)CPUOBJS += $(2)/cpu/alph8201/alph8201.o
$(1)DBGOBJS += $(2)/cpu/alph8201/8201dasm.o
$(2)/cpu/alph8201/alph8201.o: alph8201.c alph8201.h
endif



#-------------------------------------------------
# Analog Devices ADSP21xx series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_ADSP2100=$$(if $$(filter ADSP2100,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ADSP2101=$$(if $$(filter ADSP2101,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ADSP2104=$$(if $$(filter ADSP2104,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ADSP2105=$$(if $$(filter ADSP2105,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ADSP2115=$$(if $$(filter ADSP2115,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_ADSP2181=$$(if $$(filter ADSP2181,$$($(1)CPUS)),1,0)

ifneq ($$(filter ADSP2100 ADSP2101 ADSP2104 ADSP2105 ADSP2115 ADSP2181,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/adsp2100
$(1)CPUOBJS += $(2)/cpu/adsp2100/adsp2100.o
$(1)DBGOBJS += $(2)/cpu/adsp2100/2100dasm.o
$(2)/cpu/adsp2100/adsp2100.o: adsp2100.c adsp2100.h 2100ops.c adsp2100jit.c
endif



#-------------------------------------------------
# Analog Devices "Sharc" ADSP21062
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_ADSP21062=$$(if $$(filter ADSP21062,$$($(1)CPUS)),1,0)

ifneq ($$(filter ADSP21062,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/sharc
$(1)CPUOBJS += $(2)/cpu/sharc/sharc.o
$(1)DBGOBJS += $(2)/cpu/sharc/sharcdsm.o
$(2)/cpu/sharc/sharc.o: sharc.c sharc.h sharcops.c sharcops.h sharcdsm.c sharcdsm.h compute.c
endif



#-------------------------------------------------
# AT&T DSP32C
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_DSP32C=$$(if $$(filter DSP32C,$$($(1)CPUS)),1,0)

ifneq ($$(filter DSP32C,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/dsp32
$(1)CPUOBJS += $(2)/cpu/dsp32/dsp32.o
$(1)DBGOBJS += $(2)/cpu/dsp32/dsp32dis.o
$(2)/cpu/dsp32/dsp32.o: dsp32.c dsp32.h
endif



#-------------------------------------------------
# Atari custom RISC processor
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_ASAP=$$(if $$(filter ASAP,$$($(1)CPUS)),1,0)

ifneq ($$(filter ASAP,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/asap
$(1)CPUOBJS += $(2)/cpu/asap/asap.o
$(1)DBGOBJS += $(2)/cpu/asap/asapdasm.o
$(2)/cpu/asap/asap.o: asap.c asap.h
endif



#-------------------------------------------------
# Atari Jaguar custom DSPs
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_JAGUAR=$$(if $$(filter JAGUAR,$$($(1)CPUS)),1,0)

ifneq ($$(filter JAGUAR,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/jaguar
$(1)CPUOBJS += $(2)/cpu/jaguar/jaguar.o
$(1)DBGOBJS += $(2)/cpu/jaguar/jagdasm.o
$(2)/cpu/jaguar/jaguar.o: jaguar.c jaguar.h
endif



#-------------------------------------------------
# Cinematronics vector "CPU"
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_CCPU=$$(if $$(filter CCPU,$$($(1)CPUS)),1,0)

ifneq ($$(filter CCPU,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/ccpu
$(1)CPUOBJS += $(2)/cpu/ccpu/ccpu.o
$(1)DBGOBJS += $(2)/cpu/ccpu/ccpudasm.o
$(2)/cpu/ccpu/ccpu.o: ccpu.c ccpu.h
endif



#-------------------------------------------------
# DEC T-11
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_T11=$$(if $$(filter T11,$$($(1)CPUS)),1,0)

ifneq ($$(filter T11,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/t11
$(1)CPUOBJS += $(2)/cpu/t11/t11.o
$(1)DBGOBJS += $(2)/cpu/t11/t11dasm.o
$(2)/cpu/t11/t11.o: t11.c t11.h t11ops.c t11table.c
endif



#-------------------------------------------------
# G65816
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_G65816=$$(if $$(filter G65816,$$($(1)CPUS)),1,0)

ifneq ($$(filter G65816,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/g65816
$(1)CPUOBJS += $(2)/cpu/g65816/g65816.o
$(1)CPUOBJS += $(2)/cpu/g65816/g65816o0.o
$(1)CPUOBJS += $(2)/cpu/g65816/g65816o1.o
$(1)CPUOBJS += $(2)/cpu/g65816/g65816o2.o
$(1)CPUOBJS += $(2)/cpu/g65816/g65816o3.o
$(1)CPUOBJS += $(2)/cpu/g65816/g65816o4.o
$(1)DBGOBJS += $(2)/cpu/g65816/g65816ds.o
$(2)/cpu/g65816/g65816.o: g65816.c g65816.h g65816cm.h g65816op.h
$(2)/cpu/g65816/g65816o0.o: g65816o0.c g65816.h g65816cm.h g65816op.h
$(2)/cpu/g65816/g65816o1.o: g65816o1.c g65816.h g65816cm.h g65816op.h
$(2)/cpu/g65816/g65816o2.o: g65816o2.c g65816.h g65816cm.h g65816op.h
$(2)/cpu/g65816/g65816o3.o: g65816o3.c g65816.h g65816cm.h g65816op.h
$(2)/cpu/g65816/g65816o4.o: g65816o4.c g65816.h g65816cm.h g65816op.h
endif



#-------------------------------------------------
# Hitachi 6309
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_HD6309=$$(if $$(filter HD6309,$$($(1)CPUS)),1,0)

ifneq ($$(filter HD6309,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/hd6309
$(1)CPUOBJS += $(2)/cpu/hd6309/hd6309.o
$(1)DBGOBJS += $(2)/cpu/hd6309/6309dasm.o
$(2)/cpu/hd6309/hd6309.o: hd6309.c hd6309.h 6309ops.c 6309tbl.c
endif



#-------------------------------------------------
# Hitachi H8/3002
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_H83002=$$(if $$(filter H83002,$$($(1)CPUS)),1,0)

ifneq ($$(filter H83002,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/h83002
$(1)CPUOBJS += $(2)/cpu/h83002/h83002.o $(2)/cpu/h83002/h8periph.o
$(1)DBGOBJS += $(2)/cpu/h83002/h8disasm.o
$(2)/cpu/h83002/h83002.o: h83002.c h83002.h h8priv.h
$(2)/cpu/h83002/h8disasm.o: h8disasm.c
$(2)/cpu/h83002/h8periph.o: h8periph.c h8priv.h
endif



#-------------------------------------------------
# Hitachi SH2
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_SH2=$$(if $$(filter SH2,$$($(1)CPUS)),1,0)

ifneq ($$(filter SH2,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/sh2
$(1)CPUOBJS += $(2)/cpu/sh2/sh2.o
$(1)DBGOBJS += $(2)/cpu/sh2/sh2dasm.o
$(2)/cpu/sh2/sh2.o: sh2.c sh2.h sh2jit.c $$(srcdir)/mame/cpu/jitcache.h
endif



#-------------------------------------------------
# Hudsonsoft 6280
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_H6280=$$(if $$(filter H6280,$$($(1)CPUS)),1,0)

ifneq ($$(filter H6280,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/h6280
$(1)CPUOBJS += $(2)/cpu/h6280/h6280.o
$(1)DBGOBJS += $(2)/cpu/h6280/6280dasm.o
$(2)/cpu/h6280/h6280.o: h6280.c h6280.h h6280ops.h tblh6280.c
endif



#-------------------------------------------------
# Hyperstone E1 series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_E116T=$$(if $$(filter E116T,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E116XT=$$(if $$(filter E116XT,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E116XS=$$(if $$(filter E116XS,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E116XSR=$$(if $$(filter E116XSR,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E132N=$$(if $$(filter E132N,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E132T=$$(if $$(filter E132T,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E132XN=$$(if $$(filter E132XN,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E132XT=$$(if $$(filter E132XT,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E132XS=$$(if $$(filter E132XS,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_E132XSR=$$(if $$(filter E132XSR,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_GMS30C2116=$$(if $$(filter GMS30C2116,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_GMS30C2132=$$(if $$(filter GMS30C2132,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_GMS30C2216=$$(if $$(filter GMS30C2216,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_GMS30C2232=$$(if $$(filter GMS30C2232,$$($(1)CPUS)),1,0)

ifneq ($$(filter E116T E116XT E116XS E116XSR E132N E132T E132XN E132XT E132XS E132XSR GMS30C2116 GMS30C2132 GMS30C2216 GMS30C2232,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/e132xs
$(1)CPUOBJS += $(2)/cpu/e132xs/e132xs.o
$(1)DBGOBJS += $(2)/cpu/e132xs/32xsdasm.o
$(2)/cpu/e132xs/e132xs.o: e132xs.c e132xs.h
endif



#-------------------------------------------------
# Intel 8080/8085A
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_8080=$$(if $$(filter 8080,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_8085A=$$(if $$(filter 8085A,$$($(1)CPUS)),1,0)

ifneq ($$(filter 8080 8085A,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i8085
$(1)CPUOBJS += $(2)/cpu/i8085/i8085.o
$(1)DBGOBJS += $(2)/cpu/i8085/8085dasm.o
$(2)/cpu/i8085/i8085.o: i8085.c i8085.h i8085cpu.h i8085daa.h
endif



#-------------------------------------------------
# Intel 8039 and derivatives
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_I8035=$$(if $$(filter I8035,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I8039=$$(if $$(filter I8039,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I8048=$$(if $$(filter I8048,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_N7751=$$(if $$(filter N7751,$$($(1)CPUS)),1,0)

ifneq ($$(filter I8035 I8039 I8048 N7751,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i8039
$(1)CPUOBJS += $(2)/cpu/i8039/i8039.o
$(1)DBGOBJS += $(2)/cpu/i8039/8039dasm.o
$(2)/cpu/i8039/i8039.o: i8039.c i8039.h
endif



#-------------------------------------------------
# Intel 8x41
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_I8X41=$$(if $$(filter I8X41,$$($(1)CPUS)),1,0)

ifneq ($$(filter I8X41,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i8x41
$(1)CPUOBJS += $(2)/cpu/i8x41/i8x41.o
$(1)DBGOBJS += $(2)/cpu/i8x41/8x41dasm.o
$(2)/cpu/i8x41/i8x41.o: i8x41.c i8x41.h
endif



#-------------------------------------------------
# Intel 8051 and derivatives
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_I8051=$$(if $$(filter I8051,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I8052=$$(if $$(filter I8052,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I8751=$$(if $$(filter I8751,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I8752=$$(if $$(filter I8752,$$($(1)CPUS)),1,0)

ifneq ($$(filter I8051 I8052 I8751 I8752,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i8051
$(1)CPUOBJS += $(2)/cpu/i8051/i8051.o
$(1)DBGOBJS += $(2)/cpu/i8051/8051dasm.o
$(2)/cpu/i8051/i8051.o: i8051.c i8051.h i8051ops.c
endif



#-------------------------------------------------
# Dallas DS5002FP
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_DS5002FP=$$(if $$(filter DS5002FP,$$($(1)CPUS)),1,0)

ifneq ($$(filter DS5002FP,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/ds5002fp
$(1)CPUOBJS += $(2)/cpu/ds5002fp/ds5002fp.o
$(1)DBGOBJS += $(2)/cpu/ds5002fp/ds5002fpdasm.o
$(2)/cpu/ds5002fp/ds5002fp.o: ds5002fp.c ds5002fp.h ds5002fpops.c
endif



#-------------------------------------------------
# Intel 8086 series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_I86=$$(if $$(filter I86,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I88=$$(if $$(filter I88,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I186=$$(if $$(filter I186,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I188=$$(if $$(filter I188,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I286=$$(if $$(filter I286,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I386=$$(if $$(filter I386,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_I486=$$(if $$(filter I486,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PENTIUM=$$(if $$(filter PENTIUM,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_MEDIAGX=$$(if $$(filter MEDIAGX,$$($(1)CPUS)),1,0)

ifneq ($$(filter I86 I88 I186 I188,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i86
$(1)CPUOBJS += $(2)/cpu/i86/i86.o
$(1)DBGOBJS += $(2)/cpu/i386/i386dasm.o
$(2)/cpu/i86/i86.o: i86.c instr86.c instr186.c i86.h i86intf.h i186intf.h ea.h host.h modrm.h
endif

ifneq ($$(filter I286,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i86
$(1)CPUOBJS += $(2)/cpu/i86/i286.o
$(1)DBGOBJS += $(2)/cpu/i386/i386dasm.o
$(2)/cpu/i86/i86.o: i86.c instr286.c i86.h i286intf.h ea.h host.h modrm.h
endif

ifneq ($$(filter I386 I486 PENTIUM MEDIAGX,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i386
$(1)CPUOBJS += $(2)/cpu/i386/i386.o
$(1)DBGOBJS += $(2)/cpu/i386/i386dasm.o
$(2)/cpu/i386/i386.o: i386.c i386.h i386intf.h i386op16.c i386op32.c i386ops.c i486ops.c pentops.c x87ops.c i386ops.h cycles.h
endif



#-------------------------------------------------
# Intel i960
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_I960=$$(if $$(filter I960,$$($(1)CPUS)),1,0)

ifneq ($$(filter I960,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/i960
$(1)CPUOBJS += $(2)/cpu/i960/i960.o
$(1)DBGOBJS += $(2)/cpu/i960/i960dis.o
$(2)/cpu/i960/i960.o: i960.c i960.h
endif



#-------------------------------------------------
# Konami custom CPU (6809-based)
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_KONAMI=$$(if $$(filter KONAMI,$$($(1)CPUS)),1,0)

ifneq ($$(filter KONAMI,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/konami
$(1)CPUOBJS += $(2)/cpu/konami/konami.o
$(1)DBGOBJS += $(2)/cpu/konami/knmidasm.o
$(2)/cpu/konami/konami.o: konami.c konami.h konamops.c konamtbl.c
endif

#-------------------------------------------------
# Fujitsu MB86233
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_MB86233=$$(if $$(filter MB86233,$$($(1)CPUS)),1,0)

ifneq ($$(filter MB86233,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/mb86233
$(1)CPUOBJS += $(2)/cpu/mb86233/mb86233.o
$(1)DBGOBJS += $(2)/cpu/mb86233/mb86233d.o
$(2)/cpu/mb86233/mb86233.o: mb86233.c mb86233.h
endif

#-------------------------------------------------
# Microchip PIC16C5x
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_PIC16C54=$$(if $$(filter PIC16C54,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PIC16C55=$$(if $$(filter PIC16C55,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PIC16C56=$$(if $$(filter PIC16C56,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PIC16C57=$$(if $$(filter PIC16C57,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PIC16C58=$$(if $$(filter PIC16C58,$$($(1)CPUS)),1,0)

ifneq ($$(filter PIC16C54 PIC16C55 PIC16C56 PIC16C57 PIC16C58,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/pic16c5x
$(1)CPUOBJS += $(2)/cpu/pic16c5x/pic16c5x.o
$(1)DBGOBJS += $(2)/cpu/pic16c5x/16c5xdsm.o
$(2)/cpu/pic16c5x/pic16c5x.o: pic16c5x.c pic16c5x.h
endif



#-------------------------------------------------
# MIPS R3000 (MIPS I/II) series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_R3000=$$(if $$(filter R3000,$$($(1)CPUS)),1,0)

ifneq ($$(filter R3000,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/r3000
$(1)CPUOBJS += $(2)/cpu/mips/r3000.o
$(1)DBGOBJS += $(2)/cpu/mips/r3kdasm.o
$(2)/cpu/mips/r3000.o: r3000.c r3000.h
endif



#-------------------------------------------------
# MIPS R4000 (MIPS III/IV) series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_R4600=$$(if $$(filter R4600,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_R4650=$$(if $$(filter R4650,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_R4700=$$(if $$(filter R4700,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_R5000=$$(if $$(filter R5000,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_QED5271=$$(if $$(filter QED5271,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_RM7000=$$(if $$(filter RM7000,$$($(1)CPUS)),1,0)

ifneq ($$(filter R4600 R4650 R4700 R5000 QED5271 RM7000,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/mips
$(1)CPUOBJS += $(2)/cpu/mips/mips3.o
$(2)/cpu/mips/mips3.o: mips3.c mips3.h mips3jit.c mips3jit.h $$(srcdir)/mame/cpu/jitcache.h
$(1)DBGOBJS += $(2)/cpu/mips/mips3dsm.o
endif



#-------------------------------------------------
# Mitsubishi M37702 and M37710 (based on 65C816)
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_M37702=$$(if $$(filter M37702,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M37710=$$(if $$(filter M37710,$$($(1)CPUS)),1,0)

ifneq ($$(filter M37702 M37710,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m37710
$(1)CPUOBJS += $(2)/cpu/m37710/m37710.o
$(1)CPUOBJS += $(2)/cpu/m37710/m37710o0.o
$(1)CPUOBJS += $(2)/cpu/m37710/m37710o1.o
$(1)CPUOBJS += $(2)/cpu/m37710/m37710o2.o
$(1)CPUOBJS += $(2)/cpu/m37710/m37710o3.o
$(1)CPUOBJS += $(2)/cpu/m37710/m7700ds.o
$(2)/cpu/m37710/m37710.o: m37710.c m37710.h m37710o0.c m37710o1.c m37710o2.c m37710o3.c m37710op.h m7700ds.h
$(2)/cpu/m37710/m37710o0.o: m37710.h m37710o0.c m37710op.h m7700ds.h
$(2)/cpu/m37710/m37710o1.o: m37710.h m37710o1.c m37710op.h m7700ds.h
$(2)/cpu/m37710/m37710o2.o: m37710.h m37710o2.c m37710op.h m7700ds.h
$(2)/cpu/m37710/m37710o3.o: m37710.h m37710o3.c m37710op.h m7700ds.h
$(2)/cpu/m37710/m7700ds.o: m7700ds.c m7700ds.h
endif



#-------------------------------------------------
# Mostek 6502 and its many derivatives
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_M6502=$$(if $$(filter M6502,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M65C02=$$(if $$(filter M65C02,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M65SC02=$$(if $$(filter M65SC02,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M65CE02=$$(if $$(filter M65CE02,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6509=$$(if $$(filter M6509,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6510=$$(if $$(filter M6510 M6510T M7501 M8502,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6510T=$$(if $$(filter M6510T,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M7501=$$(if $$(filter M7501,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M8502=$$(if $$(filter M8502,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_N2A03=$$(if $$(filter N2A03,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_DECO16=$$(if $$(filter DECO16,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M4510=$$(if $$(filter M4510,$$($(1)CPUS)),1,0)

ifneq ($$(filter M6502 M65C02 M65SC02 M6510 M6510T M7501 M8502 N2A03 DECO16,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6502
$(1)CPUOBJS += $(2)/cpu/m6502/m6502.o
$(1)DBGOBJS += $(2)/cpu/m6502/6502dasm.o
$(2)/cpu/m6502/m6502.o: m6502.c m6502.h ops02.h t6502.c t65c02.c t65sc02.c t6510.c tdeco16.c
endif

ifneq ($$(filter M65CE02,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6502
$(1)CPUOBJS += $(2)/cpu/m6502/m65ce02.o
$(1)DBGOBJS += $(2)/cpu/m6502/6502dasm.o
$(2)/cpu/m6502/m65ce02.o: m65ce02.c m65ce02.h opsce02.h t65ce02.c
endif

ifneq ($$(filter M6509,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6502
$(1)CPUOBJS += $(2)/cpu/m6502/m6509.o
$(1)DBGOBJS += $(2)/cpu/m6502/6502dasm.o
$(2)/cpu/m6502/m6509.o: m6509.c m6509.h ops09.h t6509.c
endif

ifneq ($$(filter M4510,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6502
$(1)CPUOBJS += $(2)/cpu/m6502/m4510.o
$(1)DBGOBJS += $(2)/cpu/m6502/6502dasm.o
endif



#-------------------------------------------------
# Motorola 680x
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_M6800=$$(if $$(filter M6800,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6801=$$(if $$(filter M6801,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6802=$$(if $$(filter M6802,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6803=$$(if $$(filter M6803,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6808=$$(if $$(filter M6808,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_HD63701=$$(if $$(filter HD63701,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_NSC8105=$$(if $$(filter NSC8105,$$($(1)CPUS)),1,0)

ifneq ($$(filter M6800 M6801 M6802 M6803 M6808 HD63701 NSC8105,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6800
$(1)CPUOBJS += $(2)/cpu/m6800/m6800.o
$(1)DBGOBJS += $(2)/cpu/m6800/6800dasm.o
$(2)/cpu/m6800/m6800.o: m6800.c m6800.h 6800ops.c 6800tbl.c
endif



#-------------------------------------------------
# Motorola 6805
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_M6805=$$(if $$(filter M6805,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M68705=$$(if $$(filter M68705,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_HD63705=$$(if $$(filter HD63705,$$($(1)CPUS)),1,0)

ifneq ($$(filter M6805 M68705 HD63705,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6805
$(1)CPUOBJS += $(2)/cpu/m6805/m6805.o
$(1)DBGOBJS += $(2)/cpu/m6805/6805dasm.o
$(2)/cpu/m6805/m6805.o: m6805.c m6805.h 6805ops.c
endif



#-------------------------------------------------
# Motorola 6809
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_M6809=$$(if $$(filter M6809,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M6809E=$$(if $$(filter M6809E,$$($(1)CPUS)),1,0)

ifneq ($$(filter M6809 M6809E,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/m6809
$(1)CPUOBJS += $(2)/cpu/m6809/m6809.o
$(1)DBGOBJS += $(2)/cpu/m6809/6809dasm.o
$(2)/cpu/m6809/m6809.o: m6809.c m6809.h 6809ops.c 6809tbl.c
endif



#-------------------------------------------------
# Motorola 68HC11
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_MC68HC11=$$(if $$(filter MC68HC11,$$($(1)CPUS)),1,0)

ifneq ($$(filter MC68HC11,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/mc68hc11
$(1)CPUOBJS += $(2)/cpu/mc68hc11/mc68hc11.o
$(1)DBGOBJS += $(2)/cpu/mc68hc11/hc11dasm.o
$(2)/cpu/mc68hc11/mc68hc11.o: mc68hc11.c hc11dasm.c
endif



#-------------------------------------------------
# Motorola 68000 series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_M68000=$$(if $$(filter M68000,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M68008=$$(if $$(filter M68008,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M68010=$$(if $$(filter M68010,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M68EC020=$$(if $$(filter M68EC020,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M68020=$$(if $$(filter M68020,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_M68040=$$(if $$(filter M68040,$$($(1)CPUS)),1,0)

ifneq ($$(filter M68000 M68008 M68010 M68EC020 M68020 M68040,$$($(1)CPUS)),)

# these files are generated by the core builder
$(1)M68000_GENERATED_FILES = \
	$(2)/cpu/m68000/m68kops.c \
	$(2)/cpu/m68000/m68kopac.c \
	$(2)/cpu/m68000/m68kopdm.c \
	$(2)/cpu/m68000/m68kopnz.c

$(1)M68000_GENERATED_HEADERS = \
	$(2)/cpu/m68000/m68kops.h

$(1)OBJDIRS += $(2)/cpu/m68000
$(1)CPUOBJS += $(2)/cpu/m68000/m68kcpu.o $(2)/cpu/m68000/m68kmame.o $$(subst .c,.o,$$($(1)M68000_GENERATED_FILES))
$(1)DBGOBJS += $(2)/cpu/m68000/m68kdasm.o

# AdvanceMAME has its own rules for m68k, and then we comment the following ones
#
# when we compile source files we need to include generated files from the OBJ directory
#$(2)/cpu/m68000/%.o: mame/cpu/m68000/%.c
#	@echo Compiling $$<...
#	$$(CC) $$(CDEFS) $$(CFLAGS) -I$(2)/cpu/m68000 -c $$< -o $$@
#
# when we compile generated files we need to include stuff from the src directory
#$(2)/cpu/m68000/%.o: $(2)/cpu/m68000/%.c
#	@echo Compiling $$<...
#	$$(CC) $$(CDEFS) $$(CFLAGS) -Imame/cpu/m68000 -c $$< -o $$@
#
# rule to generate the C files
#$$($(1)M68000_GENERATED_FILES) $$($(1)M68000_GENERATED_HEADERS): $(2)/cpu/m68000/m68kmake$$(EXE) m68k_in.c
#	@echo Generating M68K source files...
#	$(2)/cpu/m68000/m68kmake$$(EXE) $(2)/cpu/m68000 mame/cpu/m68000/m68k_in.c
#
# rule to build the generator
#$(2)/cpu/m68000/m68kmake$$(EXE): $(2)/cpu/m68000/m68kmake.o $$(OSDBGOBJS)
#
# rule to ensure we build the header before building the core CPU file
#$(2)/cpu/m68000/m68kcpu.o: $$($(1)M68000_GENERATED_HEADERS)

endif



#-------------------------------------------------
# Motorola/Freescale dsp56k
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_DSP56156=$$(if $$(filter DSP56156,$$($(1)CPUS)),1,0)

ifneq ($$(filter DSP56156,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/dsp56k
$(1)CPUOBJS += $(2)/cpu/dsp56k/dsp56k.o
$(1)DBGOBJS += $(2)/cpu/dsp56k/dsp56dsm.o
# Preserve the arcade core's existing target-specific aliasing option.
ifeq ($(3),mame)
$(2)/cpu/dsp56k/dsp56k.o: CFLAGS += -fno-strict-aliasing
endif
$(2)/cpu/dsp56k/dsp56k.o: dsp56k.c dsp56ops.c dsp56k.h dsp56core.h dsp56def.h dsp56pcu.c dsp56pcu.h dsp56mem.c dsp56mem.h
endif



#-------------------------------------------------
# Motorola PowerPC series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_PPC403=$$(if $$(filter PPC403,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PPC602=$$(if $$(filter PPC602,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_PPC603=$$(if $$(filter PPC603,$$($(1)CPUS)),1,0)

ifneq ($$(filter PPC403 PPC602 PPC603,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/powerpc
$(1)CPUOBJS += $(2)/cpu/powerpc/ppc.o
$(2)/cpu/powerpc/ppc.o: ppc.c ppc.h ppc_ops.c ppc_ops.c ppc_mem.c ppc403.c ppc602.c ppc603.c
$(1)DBGOBJS += $(2)/cpu/powerpc/ppc_dasm.o
endif



#-------------------------------------------------
# NEC V-series Intel-compatible
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_V20=$$(if $$(filter V20,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_V30=$$(if $$(filter V30,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_V33=$$(if $$(filter V33,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_V25=$$(if $$(filter V25,$$($(1)CPUS)),1,0)

ifneq ($$(filter V20 V30 V33,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/nec
$(1)CPUOBJS += $(2)/cpu/nec/nec.o
$(1)DBGOBJS += $(2)/cpu/i386/i386dasm.o
$(2)/cpu/nec/nec.o: nec.c nec.h necintrf.h necea.h nechost.h necinstr.h necmodrm.h
endif

ifneq ($$(filter V25,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/nec
$(1)CPUOBJS += $(2)/cpu/nec/v25.o
$(2)/cpu/nec/v25.o: v25.c v25.h v25ops.h
endif



#-------------------------------------------------
# NEC V60/V70
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_V60=$$(if $$(filter V60,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_V70=$$(if $$(filter V70,$$($(1)CPUS)),1,0)

ifneq ($$(filter V60 V70,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/v60
$(1)CPUOBJS += $(2)/cpu/v60/v60.o
$(1)DBGOBJS += $(2)/cpu/v60/v60d.o
$(2)/cpu/v60/v60.o: am.c am1.c am2.c am3.c op12.c op2.c op3.c op4.c op5.c op6.c op7a.c optable.c v60.c v60.h v60d.c
endif



#-------------------------------------------------
# NEC V810 (uPD70732)
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_V810=$$(if $$(filter V810,$$($(1)CPUS)),1,0)

ifneq ($$(filter V810,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/v810
$(1)CPUOBJS += $(2)/cpu/v810/v810.o
$(1)DBGOBJS += $(2)/cpu/v810/v810dasm.o
$(2)/cpu/v810/v810.o: v810.c v810.h
endif



#-------------------------------------------------
# NEC uPD7810 series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_UPD7810=$$(if $$(filter UPD7810,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_UPD7807=$$(if $$(filter UPD7807,$$($(1)CPUS)),1,0)

ifneq ($$(filter UPD7810 UPD7807,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/upd7810
$(1)CPUOBJS += $(2)/cpu/upd7810/upd7810.o
$(1)DBGOBJS += $(2)/cpu/upd7810/7810dasm.o
$(2)/cpu/upd7810/upd7810.o: upd7810.c 7810tbl.c 7810ops.c upd7810.h
endif



#-------------------------------------------------
# Nintendo/SGI RSP (R3000-based + vector processing)
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_RSP=$$(if $$(filter RSP,$$($(1)CPUS)),1,0)

ifneq ($$(filter RSP,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/rsp
$(1)CPUOBJS += $(2)/cpu/rsp/rsp.o
$(1)DBGOBJS += $(2)/cpu/rsp/rsp_dasm.o
$(2)/cpu/rsp/rsp.o: rsp.c rsp.h
endif



#-------------------------------------------------
# Signetics 2650
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_S2650=$$(if $$(filter S2650,$$($(1)CPUS)),1,0)

ifneq ($$(filter S2650,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/s2650
$(1)CPUOBJS += $(2)/cpu/s2650/s2650.o
$(1)DBGOBJS += $(2)/cpu/s2650/2650dasm.o
$(2)/cpu/s2650/s2650.o: s2650.c s2650.h s2650cpu.h
endif



#-------------------------------------------------
# Sony/Nintendo SPC700
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_SPC700=$$(if $$(filter SPC700,$$($(1)CPUS)),1,0)

ifneq ($$(filter SPC700,$$($(1)CPUS)),)
SPCD = cpu/spc700
$(1)OBJDIRS += $(2)/cpu/spc700
$(1)CPUOBJS += $(2)/cpu/spc700/spc700.o
$(1)DBGOBJS += $(2)/cpu/spc700/spc700ds.o
$(2)/cpu/spc700/spc700.o: spc700.c spc700.h
endif



#-------------------------------------------------
# Sony PlayStation CPU (R3000-based + GTE)
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_PSXCPU=$$(if $$(filter PSXCPU,$$($(1)CPUS)),1,0)

ifneq ($$(filter PSXCPU,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/mips
$(1)CPUOBJS += $(2)/cpu/mips/psx.o
$(1)DBGOBJS += $(2)/cpu/mips/mipsdasm.o
$(2)/cpu/mips/psx.o: psx.c psx.h psxjit.c psxloop.c $$(srcdir)/mame/cpu/jitcache.h
endif



#-------------------------------------------------
# Texas Instruments TMS99xx series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TMS9900=$$(if $$(filter TMS9900,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS9940=$$(if $$(filter TMS9940,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS9980=$$(if $$(filter TMS9980,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS9985=$$(if $$(filter TMS9985,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS9989=$$(if $$(filter TMS9989,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS9995=$$(if $$(filter TMS9995,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS99105A=$$(if $$(filter TMS99105A,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS99110A=$$(if $$(filter TMS99110A,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS99000=$$(if $$(filter TMS99000,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TI990_10=$$(if $$(filter TMS99010,$$($(1)CPUS)),1,0)

ifneq ($$(filter TMS9900 TMS9940 TMS99000,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms9900
$(1)CPUOBJS += $(2)/cpu/tms9900/tms9900.o
$(1)DBGOBJS += $(2)/cpu/tms9900/9900dasm.o
$(2)/cpu/tms9900/tms9900.o: tms9900.c tms9900.h 99xxcore.h 9900stat.h
endif

ifneq ($$(filter TMS9980 TMS9985 TMS9989,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms9900
$(1)CPUOBJS += $(2)/cpu/tms9900/tms9980a.o
$(1)DBGOBJS += $(2)/cpu/tms9900/9900dasm.o
$(2)/cpu/tms9900/tms9980a.o: tms9980a.c tms9900.h 99xxcore.h 99xxstat.h
endif

ifneq ($$(filter TMS9995 TMS99105A TMS99110A,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms9900
$(1)CPUOBJS += $(2)/cpu/tms9900/tms9995.o
$(1)DBGOBJS += $(2)/cpu/tms9900/9900dasm.o
$(2)/cpu/tms9900/tms9995.o: tms9995.c tms9900.h 99xxcore.h 99xxstat.h
endif

ifneq ($$(filter TMS99010,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms9900
$(1)CPUOBJS += $(2)/cpu/tms9900/ti990_10.o
$(1)DBGOBJS += $(2)/cpu/tms9900/9900dasm.o
$(2)/cpu/tms9900/ti990_10.o: ti990_10.c tms9900.h 99xxcore.h 99xxstat.h
endif



#-------------------------------------------------
# Texas Instruments TMS340x0 graphics controllers
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TMS34010=$$(if $$(filter TMS34010,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS34020=$$(if $$(filter TMS34020,$$($(1)CPUS)),1,0)

ifneq ($$(filter TMS34010 TMS34020,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms34010
$(1)CPUOBJS += $(2)/cpu/tms34010/tms34010.o $(2)/cpu/tms34010/34010fld.o
$(1)DBGOBJS += $(2)/cpu/tms34010/34010dsm.o
$(2)/cpu/tms34010/tms34010.o: tms34010.c tms34010.h 34010ops.c 34010gfx.c 34010tbl.c
endif



#-------------------------------------------------
# Texas Instruments TMS3201x DSP
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TMS32010=$$(if $$(filter TMS32010,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS32015=$$(if $$(filter TMS32015,$$($(1)CPUS)),1,0)

ifneq ($$(filter TMS32010 TMS32015,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms32010
$(1)CPUOBJS += $(2)/cpu/tms32010/tms32010.o
$(1)DBGOBJS += $(2)/cpu/tms32010/32010dsm.o
$(2)/cpu/tms32010/tms32010.o: tms32010.c tms32010.h
endif



#-------------------------------------------------
# Texas Instruments TMS3202x DSP
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TMS32025=$$(if $$(filter TMS32025,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_TMS32026=$$(if $$(filter TMS32026,$$($(1)CPUS)),1,0)

ifneq ($$(filter TMS32025 TMS32026,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms32025
$(1)CPUOBJS += $(2)/cpu/tms32025/tms32025.o
$(1)DBGOBJS += $(2)/cpu/tms32025/32025dsm.o
$(2)/cpu/tms32025/tms32025.o: tms32025.c tms32025.h
endif



#-------------------------------------------------
# Texas Instruments TMS3203x DSP
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TMS32031=$$(if $$(filter TMS32031,$$($(1)CPUS)),1,0)

ifneq ($$(filter TMS32031,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms32031
$(1)CPUOBJS += $(2)/cpu/tms32031/tms32031.o
$(1)DBGOBJS += $(2)/cpu/tms32031/dis32031.o
$(2)/cpu/tms32031/tms32031.o: tms32031.c tms32031.h
endif



#-------------------------------------------------
# Texas Instruments TMS3205x DSP
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TMS32051=$$(if $$(filter TMS32051,$$($(1)CPUS)),1,0)

ifneq ($$(filter TMS32051,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tms32051
$(1)CPUOBJS += $(2)/cpu/tms32051/tms32051.o
$(1)DBGOBJS += $(2)/cpu/tms32051/dis32051.o
$(2)/cpu/tms32051/tms32051.o: tms32051.c tms32051.h
endif



#-------------------------------------------------
# Toshiba TLCS-90 Series
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_TLCS90=$$(if $$(filter TLCS90,$$($(1)CPUS)),1,0)
ifneq ($$(filter TLCS90,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/tlcs90
$(1)CPUOBJS += $(2)/cpu/tlcs90/tlcs90.o
#DBGOBJS += $(2)/cpu/tlcs90/tlcs90.o
$(2)/cpu/tlcs90/tlcs90.o: tlcs90.c tlcs90.h
endif



#-------------------------------------------------
# Zilog Z80
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_Z80=$$(if $$(filter Z80,$$($(1)CPUS)),1,0)

ifneq ($$(filter Z80,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/z80
$(1)CPUOBJS += $(2)/cpu/z80/z80.o $(2)/cpu/z80/z80daisy.o
$(1)DBGOBJS += $(2)/cpu/z80/z80dasm.o
$(2)/cpu/z80/z80.o: z80.c z80.h
endif



#-------------------------------------------------
# Zilog Z180
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_Z180=$$(if $$(filter Z180,$$($(1)CPUS)),1,0)

ifneq ($$(filter Z180,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/z180
$(1)CPUOBJS += $(2)/cpu/z180/z180.o $(2)/cpu/z80/z80daisy.o
$(1)DBGOBJS += $(2)/cpu/z180/z180dasm.o
$(2)/cpu/z180/z180.o: z180.c z180.h z180daa.h z180op.c z180ops.h z180tbl.h z180cb.c z180dd.c z180ed.c z180fd.c z180xy.c
endif



#-------------------------------------------------
# Zilog Z8000
#-------------------------------------------------

$(1)CPUDEFS += -DHAS_Z8000=$$(if $$(filter Z8000,$$($(1)CPUS)),1,0)
ifneq ($$(filter Z8000,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/z8000
$(1)CPUOBJS += $(2)/cpu/z8000/z8000.o
$(1)DBGOBJS += $(2)/cpu/z8000/8000dasm.o
$(2)/cpu/z8000/z8000.o: z8000.c z8000.h z8000cpu.h z8000dab.h z8000ops.c z8000tbl.c
endif

$(1)CPUDEFS += -DHAS_SH3BE=$$(if $$(filter SH3BE,$$($(1)CPUS)),1,0)
ifneq ($$(filter SH3BE,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/sh3
$(1)CPUOBJS += $(2)/cpu/sh3/sh3.o
$(1)DBGOBJS += $(2)/cpu/sh3/sh3dasm.o
$(2)/cpu/sh3/sh3.o: CFLAGS += -fno-strict-aliasing
$(2)/cpu/sh3/sh3.o: sh3.c sh3.h sh3jit.c $$(srcdir)/mame/cpu/jitcache.h
endif

# MESS supplies these CPUs in mess/cpu/cpu.mak; never link both implementations.
ifeq ($(3),mame)
$(1)CPUDEFS += -DHAS_CDP1802=$$(if $$(filter CDP1802,$$($(1)CPUS)),1,0)
$(1)CPUDEFS += -DHAS_COP420=$$(if $$(filter COP420,$$($(1)CPUS)),1,0)
ifneq ($$(filter CDP1802,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/cdp1802
$(1)CPUOBJS += $(2)/cpu/cdp1802/cdp1802.o
$(1)DBGOBJS += $(2)/cpu/cdp1802/1802dasm.o
$(2)/cpu/cdp1802/cdp1802.o $(2)/cpu/cdp1802/1802dasm.o: mame/cpu/cdp1802/cdp1802.h
endif
ifneq ($$(filter COP420,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/cop400
$(1)CPUOBJS += $(2)/cpu/cop400/cop420.o
$(1)DBGOBJS += $(2)/cpu/cop400/cop420ds.o
$(2)/cpu/cop400/cop420.o: mame/cpu/cop400/410ops.c mame/cpu/cop400/420ops.c mame/cpu/cop400/cop400.h
endif

endif

$(1)CPUDEFS += -DHAS_ARM920T=$$(if $$(filter ARM920T,$$($(1)CPUS)),1,0)
ifneq ($$(filter ARM920T,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/arm920t
$(1)CPUOBJS += $(2)/cpu/arm920t/arm920t.o
ifeq ($$(filter ARM7 IGS036,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/arm7
$(1)DBGOBJS += $(2)/cpu/arm7/arm7dasm.o
endif
$(2)/cpu/arm920t/arm920t.o: arm920t.c arm920t.h ../igs036/igs036core.h ../igs036/igs036help.h ../igs036/igs036ops.c ../igs036/igs036thumb.c ../igs036/igs036irq.c
endif

$(1)CPUDEFS += -DHAS_XA=$$(if $$(filter XA,$$($(1)CPUS)),1,0)
ifneq ($$(filter XA,$$($(1)CPUS)),)
$(1)OBJDIRS += $(2)/cpu/xa
$(1)CPUOBJS += $(2)/cpu/xa/xa.o
$(2)/cpu/xa/xa.o: xa.c xa.h
endif
endef
