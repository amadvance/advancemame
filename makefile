#
# AdvanceMAME
#

#############################################################################
# Basic configuration (change options only in this section)
#

# Installation prefix (only for Linux):
PREFIX=/usr/local

# Select the target executable (mame by default)
# The available choices are: mame, neomame, cpmame, mess, pac
#TARGET = mame

# Select the target architecture CFLAGS (i686 by default)
# The available choices for the -march= option with gcc-2.95 are:
#   i386, i486, i586, i686
# The available choices for the -march= option with gcc-3.0 are:
#   i386, i486, i586, pentium, pentium-mmx, i686, pentiumpro, k6, athlon
# The available choices for the -march= option with gcc-3.1 are:
#   i386, i486, i586, pentium, pentium-mmx, i686, pentiumpro, pentium2,
#   pentium3, pentium4, k6, k6-2, k6-3, athlon, athlon-tbird, athlon-4,
#   athlon-xp, athlon-mp
# for i586
#CFLAGS_ARCH = -march=i586 -DUSE_LSB -DUSE_ASM_i586
# for i686
CFLAGS_ARCH = -march=i686 -DUSE_LSB -DUSE_ASM_i586 -DUSE_ASM_MMX
# for k6
#CFLAGS_ARCH = -march=k6 -DUSE_LSB -DUSE_ASM_i586 -DUSE_ASM_MMX
# for generic LittleEndian
#CFLAGS_ARCH = -DUSE_LSB
# for generic BigEndian
#CFLAGS_ARCH = -DUSE_LSB

#############################################################################
# Advanced configuration
#

# Uncomment to enable the SMP code:
USE_SMP=1

# Uncommment the target operating system or the current one is used:
#HOST_TARGET=linux
#HOST_TARGET=dos

# Compilation option for the optimized build
CFLAGS_OPTIMIZE = -O3 -fomit-frame-pointer -fstrict-aliasing

# Compilation option for the debug build
CFLAGS_DEBUG = -O0

# Uncomment to compile the MAME debugger:
#DEBUG=1

# Uncomment to generate the profiling information:
#PROFILE=1

# Uncomment to generate the debug symbols:
#SYMBOLS=1

# Uncomment to generate the map file:
#MAP=1

# Uncomment to compress the executable:
COMPRESS=1

#############################################################################
# System options

# Build environment
HOST := $(shell uname)

# Target environment
ifndef HOST_TARGET
ifeq ($(HOST),Linux)
HOST_TARGET=linux
else
HOST_TARGET=dos
endif
endif

# Build environment
ifeq ($(HOST),Linux)
HOST_BUILD=linux
else
HOST_BUILD=dos
endif

############################################################################
# Tool options

MAKE = @make -j 2
ASM = @nasm
MD = -@mkdir -p
RM = @rm -f
UPX = @upx -q -q -q
ECHO = @echo
# Don't add the prefix @. This command must be used also in a sheel
INSTALL = install
INSTALL_PROGRAM_DIR = $(INSTALL) -d -o root -g bin -m 755
INSTALL_MAN_DIR = $(INSTALL) -d -o root -g 0 -m 755
INSTALL_DATA_DIR = $(INSTALL) -d -o root -g bin -m 755
INSTALL_PROGRAM = $(INSTALL) -c -o root -g bin -m 555
INSTALL_PROGRAM_STRIP = $(INSTALL) -c -s -o root -g bin -m 555
INSTALL_MAN = $(INSTALL) -c -o root -g bin -m 444
INSTALL_DATA = $(INSTALL) -c -o root -g bin -m 644

OBJ = obj/$(TARGET)/$(HOST_TARGET)/$(ARCH)

ifeq ($(HOST_TARGET),linux)
LN = @ln -s
EXE =
ASMPREPROCESSOR = @advance/strip_
ASMFLAGS = -f elf
endif
ifeq ($(HOST_TARGET),dos)
LN = @cp
EXE = .exe
ASMFLAGS = -f coff
endif

ifeq ($(HOST_BUILD),linux)
EXE-HOST =
CC-HOST = @gcc
TOUCH = @touch
CFLAGS-HOST = -O0
ifeq ($(HOST_TARGET),linux)
AR = @ar
CC = @gcc
CXX = @g++
LD = @gcc
LDXX = @g++
# For the stack backtrace
LDFLAGS += -rdynamic
endif
ifeq ($(HOST_TARGET),dos)
# Probably you need to changes these to cross compile:
DJDIR = /mnt/dos1/djgpp
#CROSSTARGET = i386-pc-msdosdjgpp
CROSSTARGET = i586-pc-msdosdjgpp
#CROSSDIR = /usr/local/gcc-2.95.2-i386-pc-msdosdjgpp
CROSSDIR = /usr/local/gcc-3.0.4-i586-pc-msdosdjgpp
#CROSSDIR = /usr/local/gcc-3.1-i586-pc-msdosdjgpp
AR = @$(CROSSDIR)/bin/$(CROSSTARGET)-ar
CC = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
CXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
LD = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc -B$(CROSSDIR)/bin/
LDXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++ -B$(CROSSDIR)/bin/
LDFLAGS += -L $(DJDIR)/lib
ASMPREPROCESSOR = @true
endif
endif

ifeq ($(HOST_BUILD),dos)
EXE-HOST = .exe
CC-HOST = @gcc
CFLAGS-HOST = -O0
AR = @ar
CC = @gcc
CXX = @gxx
LD = @gcc
LDXX = @gxx
ASMPREPROCESSOR = @rem
TOUCH = @rem
endif

#############################################################################
# Architecture options

# Default architecture name
ifeq ($(ARCH),)
ARCH = def
endif

# Override of the default option
ifneq ($(CFLAGS_ARCH_OVERRIDE),)
CFLAGS += $(CFLAGS_ARCH_OVERRIDE)
else
CFLAGS += $(CFLAGS_ARCH)
endif

#############################################################################
# Symbol options

ifdef SYMBOLS
MSG = "(symbols)"
CFLAGS += $(CFLAGS_DEBUG) -g -Wall -Wno-sign-compare -Wno-unused
else
ifdef PROFILE
MSG = "(profile)"
CFLAGS += $(CFLAGS_OPTIMIZE) -DNDEBUG -pg -Wall -Wno-sign-compare -Wno-unused
else
MSG =
CFLAGS += $(CFLAGS_OPTIMIZE) -DNDEBUG -Wall -Wno-sign-compare -Wno-unused
LDFLAGS += -s
endif
endif

#############################################################################
# TARGET options

# Automatic TARGET detection by source
ifeq ($(TARGET),)
ifneq ($(wildcard src),)
TARGET = mame
endif
endif
ifeq ($(TARGET),)
ifneq ($(wildcard srcmess),)
TARGET = mess
endif
endif
ifeq ($(TARGET),)
ifneq ($(wildcard srcpac),)
TARGET = pac
endif
endif
ifeq ($(TARGET),)
TARGET = unknow
endif

# Target file name
TARGETNAME = adv$(TARGET)

# Target source directory
ifeq ($(TARGET),mess)
TARGETSRC=srcmess
endif
ifeq ($(TARGET),pac)
TARGETSRC=srcpac
endif
ifeq ($(TARGETSRC),)
TARGETSRC=src
endif

# Advance makefile
include advance/advance.mak

# Only if the TARGET source are present
ifneq ($(wildcard $(TARGETSRC)),)

# Target CFLAGS
ifneq (,$(findstring USE_ASM_i586,$(CFLAGS)))
TARGETCFLAGS += -DX86_ASM
X86_ASM_68000=1
#X86_ASM_68020=1
endif

ifneq (,$(findstring USE_LSB,$(CFLAGS)))
TARGETCFLAGS += -DLSB_FIRST
M68000FLAGS += -DLSB_FIRST
endif

ifneq (,$(findstring USE_MSB,$(CFLAGS)))
TARGETCFLAGS += -DMSB_FIRST
M68000FLAGS += -DMSB_FIRST
endif

TARGETCFLAGS += \
	-I. \
	-I$(TARGETSRC)
ifeq ($(TARGET),mess)
TARGETCFLAGS += \
	-Imess \
	-DUNIX
# -DUNIX is required by the MESS source
endif

TARGETCFLAGS += \
	-I$(TARGETSRC)/includes \
	-I$(OBJ)/cpu/m68000 \
	-I$(TARGETSRC)/cpu/m68000 \
	-DINLINE="static __inline__" \
	-Dasm=__asm__

# Target LDFLAGS
ifdef MAP
TARGETLDFLAGS += -Xlinker -Map -Xlinker $(OBJ)/$(TARGETNAME).map
endif

# Target LIBS
TARGETLIBS += -lz

ifeq ($(TARGET),mess)
include $(TARGETSRC)/core.mak
include mess/$(TARGET).mak
include $(TARGETSRC)/rules.mak
include mess/rules_ms.mak
else
include $(TARGETSRC)/core.mak
include $(TARGETSRC)/$(TARGET).mak
include $(TARGETSRC)/rules.mak
endif

# Special search paths required by the CPU core rules
VPATH=$(wildcard $(TARGETSRC)/cpu/*)

OBJDIRS += \
	$(OBJ)/cpu \
	$(OBJ)/sound \
	$(OBJ)/drivers \
	$(OBJ)/machine \
	$(OBJ)/vidhrdw \
	$(OBJ)/sndhrdw
ifeq ($(TARGET),mess)
OBJDIRS += \
	$(OBJ)/mess \
	$(OBJ)/mess/systems \
	$(OBJ)/mess/machine \
	$(OBJ)/mess/vidhrdw \
	$(OBJ)/mess/sndhrdw \
	$(OBJ)/mess/formats \
	$(OBJ)/mess/tools \
	$(OBJ)/mess/tools/dat2html \
	$(OBJ)/mess/tools/mkhdimg \
	$(OBJ)/mess/tools/messroms \
	$(OBJ)/mess/tools/imgtool \
	$(OBJ)/mess/tools/mkimage \
	$(OBJ)/mess/sound \
	$(OBJ)/mess/cpu
endif

ifdef DEBUG
TARGETDEFS += -DMAME_DEBUG
else
# Required because DBGOBJS is always included
DBGOBJS =
endif

TARGETDEFS += $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS)

M68000FLAGS += \
	$(CFLAGS-HOST) \
	$(TARGETDEFS) \
	-DINLINE="static __inline__" \
	-I$(OBJ)/cpu/m68000 \
	-I$(TARGETSRC)/cpu/m68000 \
	-I$(TARGETSRC)
ifeq ($(TARGET),mess)
M68000FLAGS += \
	-Imess
endif

$(OBJ)/$(TARGETNAME)$(EXE): $(sort $(OBJDIRS)) $(TARGETOSOBJS) $(OBJS) $(COREOBJS) $(DRVLIBS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(TARGETLDFLAGS) $(OBJS) $(COREOBJS) $(TARGETOSOBJS) $(TARGETLIBS) $(DRVLIBS) -o $@
ifdef COMPRESS
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) $(TARGETNAME)$(EXE)
	$(LN) $(OBJ)/$(TARGETNAME)$(EXE) $(TARGETNAME)$(EXE)

$(OBJ)/%.o: $(TARGETSRC)/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(TARGETCFLAGS) $(TARGETDEFS) -c $< -o $@

$(OBJ)/mess/%.o: mess/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(TARGETCFLAGS) $(TARGETDEFS) -c $< -o $@

# Generate C source files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST)
	$(ECHO) $@
	$(CC) $(M68000FLAGS) -c $*.c -o $@

# Additional rule, because m68kcpu.c includes the generated m68kops.h
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST)

$(OBJ)/cpu/m68000/m68kmake$(EXE-HOST): $(TARGETSRC)/cpu/m68000/m68kmake.c
	$(ECHO) $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST)
	$(CC-HOST) $(M68000FLAGS) -DDOS -o $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST) $<
	@$(OBJ)/cpu/m68000/m68kmake$(EXE-HOST) $(OBJ)/cpu/m68000 $(TARGETSRC)/cpu/m68000/m68k_in.c > /dev/null

# Generate asm source files for the 68000/68020 emulators
$(OBJ)/cpu/m68000/make68k$(EXE-HOST): $(TARGETSRC)/cpu/m68000/make68k.c
	$(ECHO) $@
	$(CC-HOST) $(M68000FLAGS) -DDOS -o $(OBJ)/cpu/m68000/make68k$(EXE-HOST) $<

$(OBJ)/cpu/m68000/68000.asm: $(OBJ)/cpu/m68000/make68k$(EXE-HOST)
	$(ECHO) $@
	@$(OBJ)/cpu/m68000/make68k$(EXE-HOST) $@ $(OBJ)/cpu/m68000/68000tab.asm 00 > /dev/null
	$(ASMPREPROCESSOR) $@

$(OBJ)/cpu/m68000/68020.asm: $(OBJ)/cpu/m68000/make68k$(EXE-HOST)
	$(ECHO) $@
	@$(OBJ)/cpu/m68000/make68k$(EXE-HOST) $@ $(OBJ)/cpu/m68000/68020tab.asm 20 > /dev/null
	$(ASMPREPROCESSOR) $@

$(OBJ)/cpu/m68000/68000.o: $(OBJ)/cpu/m68000/68000.asm
	$(ECHO) $@
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/cpu/m68000/68020.o: $(OBJ)/cpu/m68000/68020.asm
	$(ECHO) $@
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/%.a:
	$(ECHO) $@
	$(RM) $@
	$(AR) cr $@ $^

$(sort $(OBJDIRS)):
	$(ECHO) $@
	$(MD) $@

# TARGET switch
endif

#############################################################################
# Special targets

clean:
	$(ECHO) obj
	$(RM) -r obj

flags: obj
	$(ECHO) CFLAGS=$(CFLAGS)
	$(ECHO) LDFLAGS=$(LDFLAGS)
	$(ECHO) TARGETCFLAGS=$(TARGETCFLAGS)
	$(ECHO) OSCFLAGS=$(OSCFLAGS)
	$(ECHO) TARGETLDFLAGS=$(TARGETLDFLAGS)
	$(ECHO) TARGETLIBS=$(TARGETLIBS)
	$(ECHO) VCFLAGS=$(VCFLAGS)
	$(ECHO) VLDFLAGS=$(VLDFLAGS)
	$(ECHO) VLIBS=$(VLIBS)
	$(ECHO) CC=$(CC)
	$(ECHO) CC-HOST=$(CC-HOST)
	$(ECHO) "int test(void) { return 0; }" > obj/flags.c
	$(CC) $(CFLAGS) obj/flags.c -S -fverbose-asm -o obj/flags.S

#############################################################################
# SLOCCount

COSTTMP=/tmp/cost

costadvance:
	-mkdir cost
	mkdir $(COSTTMP)
	unzip -o -d $(COSTTMP) advancemame-$(MAMEVERSION).zip
	unzip -o -d $(COSTTMP) advancemenu-$(MENUVERSION).zip
	unzip -o -d $(COSTTMP) advancecab-$(CABVERSION).zip
	rm -r $(COSTTMP)/advance/svgalib
	rm -r $(COSTTMP)/advance/mpglib
	rm $(COSTTMP)/advance/menu/playsnd.cc
	rm $(COSTTMP)/advance/cfg/list.c
	rm $(COSTTMP)/advance/lib/fontdef.c
	rm -r $(COSTTMP)/contrib
	sloccount $(COSTTMP) > cost/advance.txt
	rm -r $(COSTTMP)

costmame:
	sloccount src > cost/mame.txt

costmess:
	sloccount mess > cost/mess.txt

