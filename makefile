#
# AdvanceMAME
#

#############################################################################
# Basic configuration
#

# Installation prefix (only for Linux):
PREFIX=/usr/local

# Select the emu executable (mame by default)
# The available choices are: mame, neomame, cpmame, mess, pac
#EMU = mame

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

# Uncomment the main system library to use.
# The available choices for Linux are :
#   linux, sdl (default is linux)
# The available choices for DOS are :
#   dos (default is dos)
#
# The dos system uses:
#   SVGALIB, VESAtweak and VGAtweak for the graphics output
#   VGAtweak for the text output
#   SEAL and Allegro for the sound output
#   Allegro for the input
# This system is able to directly access and completly control the graphics
# output of your video board.
#
# The linux system uses:
#   SVGALIB for the graphics output
#   SLang for the text output
#   OSS for the sound output
#   SVGALIB for the input
# This system is able to directly access and completly control the graphics
# output of your video board.
#
# The sdl system uses:
#   SDL for the graphics output
#   none for the text output (no text mode programs available)
#   SDL for the sound output
#   SDL for the input
# This system can be used to show the programs in a Window Manager, but
# it's unable to completly control the graphics output. It isn't a good
# choice for a fullscreen use of the programs.
#
#HOST_SYSTEM=linux
#HOST_SYSTEM=dos
#HOST_SYSTEM=sdl

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
#

# Build environment
HOST := $(shell uname)
ifeq ($(HOST),Linux)
HOST_BUILD=linux
else
HOST_BUILD=dos
endif

# Target environment
ifndef HOST_TARGET
HOST_TARGET=$(HOST_BUILD)
endif

# Core environment
ifndef HOST_SYSTEM
HOST_SYSTEM=$(HOST_TARGET)
endif

# Binary description
BINARYTAG = $(HOST_TARGET)-$(HOST_SYSTEM)-$(ARCH)
BINARYDIR = $(HOST_TARGET)/$(HOST_SYSTEM)/$(ARCH)

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

OBJ = obj/$(EMU)/$(BINARYDIR)

ifeq ($(HOST_TARGET),linux)
LN = @ln -s
EXE =
ASMFLAGS = -f elf
CFLAGS-HOST = -O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_ELF
endif
ifeq ($(HOST_TARGET),dos)
LN = @cp
EXE = .exe
ASMFLAGS = -f coff
CFLAGS-HOST = -O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_COFF
endif

ifeq ($(HOST_BUILD),linux)
EXE-HOST =
CC-HOST = @gcc
TOUCH = @touch
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
endif
endif

ifeq ($(HOST_BUILD),dos)
EXE-HOST = .exe
CC-HOST = @gcc
AR = @ar
CC = @gcc
CXX = @gxx
LD = @gcc
LDXX = @gxx
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
# EMU options

# Automatic EMU detection by source
ifeq ($(EMU),)
ifneq ($(wildcard src),)
EMU = mame
endif
endif
ifeq ($(EMU),)
ifneq ($(wildcard srcmess),)
EMU = mess
endif
endif
ifeq ($(EMU),)
ifneq ($(wildcard srcpac),)
EMU = pac
endif
endif
ifeq ($(EMU),)
EMU = unknow
endif

# Target file name
EMUNAME = adv$(EMU)

# Target source directory
ifeq ($(EMU),mess)
EMUSRC=srcmess
endif
ifeq ($(EMU),pac)
EMUSRC=srcpac
endif
ifeq ($(EMUSRC),)
EMUSRC=src
endif

# Advance makefile
include advance/advance.mak

# Only if the EMU source are present
ifneq ($(wildcard $(EMUSRC)),)

# Target CFLAGS
ifneq (,$(findstring USE_ASM_i586,$(CFLAGS)))
EMUCFLAGS += -DX86_ASM
X86_ASM_68000=1
#X86_ASM_68020=1
endif

ifneq (,$(findstring USE_LSB,$(CFLAGS)))
EMUCFLAGS += -DLSB_FIRST
M68000FLAGS += -DLSB_FIRST
endif

ifneq (,$(findstring USE_MSB,$(CFLAGS)))
EMUCFLAGS += -DMSB_FIRST
M68000FLAGS += -DMSB_FIRST
endif

EMUCFLAGS += \
	-I. \
	-I$(EMUSRC)
ifeq ($(EMU),mess)
EMUCFLAGS += \
	-Imess \
	-DUNIX
# -DUNIX is required by the MESS source
endif

EMUCFLAGS += \
	-I$(EMUSRC)/includes \
	-I$(OBJ)/cpu/m68000 \
	-I$(EMUSRC)/cpu/m68000 \
	-DINLINE="static __inline__" \
	-Dasm=__asm__

# Map
ifdef MAP
TARGETLDFLAGS += -Xlinker -Map -Xlinker $(OBJ)/$(EMUNAME).map
endif

ifeq ($(EMU),mess)
include $(EMUSRC)/core.mak
include mess/$(EMU).mak
include $(EMUSRC)/rules.mak
include mess/rules_ms.mak
else
include $(EMUSRC)/core.mak
include $(EMUSRC)/$(EMU).mak
include $(EMUSRC)/rules.mak
endif

# Special search paths required by the CPU core rules
VPATH=$(wildcard $(EMUSRC)/cpu/*)

OBJDIRS += \
	$(OBJ)/cpu \
	$(OBJ)/sound \
	$(OBJ)/drivers \
	$(OBJ)/machine \
	$(OBJ)/vidhrdw \
	$(OBJ)/sndhrdw
ifeq ($(EMU),mess)
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
EMUDEFS += -DMAME_DEBUG
else
# Required because DBGOBJS is always included
DBGOBJS =
endif

EMUDEFS += $(COREDEFS) $(CPUDEFS) $(SOUNDDEFS) $(ASMDEFS)

M68000FLAGS += \
	$(CFLAGS-HOST) \
	$(EMUDEFS) \
	-DINLINE="static __inline__" \
	-I$(OBJ)/cpu/m68000 \
	-I$(EMUSRC)/cpu/m68000 \
	-I$(EMUSRC)
ifeq ($(EMU),mess)
M68000FLAGS += \
	-Imess
endif

$(OBJ)/$(EMUNAME)$(EXE): $(sort $(OBJDIRS)) $(TARGETOSOBJS) $(SYSTEMOBJS) $(EMUOBJS) $(COREOBJS) $(DRVLIBS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(TARGETLDFLAGS) $(SYSTEMLDFLAGS) $(TARGETOSOBJS) $(SYSTEMOBJS) $(EMUOBJS) $(COREOBJS) $(TARGETLIBS) $(SYSTEMLIBS) $(EMULIBS) $(DRVLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) $(EMUNAME)$(EXE)
	$(LN) $(OBJ)/$(EMUNAME)$(EXE) $(EMUNAME)$(EXE)

$(OBJ)/%.o: $(EMUSRC)/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(TARGETCFLAGS) $(SYSTEMCFLAGS) $(EMUCFLAGS) $(EMUDEFS) -c $< -o $@

$(OBJ)/mess/%.o: mess/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(TARGETCFLAGS) $(SYSTEMCFLAGS) $(EMUCFLAGS) $(EMUDEFS) -c $< -o $@

# Generate C source files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST)
	$(ECHO) $@
	$(CC) $(M68000FLAGS) -c $*.c -o $@

# Additional rule, because m68kcpu.c includes the generated m68kops.h
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST)

$(OBJ)/cpu/m68000/m68kmake$(EXE-HOST): $(EMUSRC)/cpu/m68000/m68kmake.c
	$(ECHO) $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST)
	$(CC-HOST) $(M68000FLAGS) -o $(OBJ)/cpu/m68000/m68kmake$(EXE-HOST) $<
	@$(OBJ)/cpu/m68000/m68kmake$(EXE-HOST) $(OBJ)/cpu/m68000 $(EMUSRC)/cpu/m68000/m68k_in.c > /dev/null

# Generate asm source files for the 68000/68020 emulators
$(OBJ)/cpu/m68000/make68k$(EXE-HOST): $(EMUSRC)/cpu/m68000/make68k.c
	$(ECHO) $@
	$(CC-HOST) $(M68000FLAGS) -o $(OBJ)/cpu/m68000/make68k$(EXE-HOST) $<

$(OBJ)/cpu/m68000/68000.asm: $(OBJ)/cpu/m68000/make68k$(EXE-HOST)
	$(ECHO) $@
	@$(OBJ)/cpu/m68000/make68k$(EXE-HOST) $@ $(OBJ)/cpu/m68000/68000tab.asm 00 > /dev/null

$(OBJ)/cpu/m68000/68020.asm: $(OBJ)/cpu/m68000/make68k$(EXE-HOST)
	$(ECHO) $@
	@$(OBJ)/cpu/m68000/make68k$(EXE-HOST) $@ $(OBJ)/cpu/m68000/68020tab.asm 20 > /dev/null

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

# EMU switch
endif

#############################################################################
# Special targets

clean:
	$(ECHO) obj
	$(RM) -r obj

flags: obj
	$(ECHO) CFLAGS=$(CFLAGS)
	$(ECHO) LDFLAGS=$(LDFLAGS)
	$(ECHO) CFLAGS-HOST=$(CFLAGS-HOST)
	$(ECHO) EMUCFLAGS=$(EMUCFLAGS)
	$(ECHO) EMULDFLAGS=$(EMULDFLAGS)
	$(ECHO) EMULIBS=$(EMULIBS)
	$(ECHO) TARGETCFLAGS=$(TARGETCFLAGS)
	$(ECHO) TARGETLDFLAGS=$(TARGETLDFLAGS)
	$(ECHO) TARGETLIBS=$(TARGETLIBS)
	$(ECHO) SYSTEMCFLAGS=$(SYSTEMCFLAGS)
	$(ECHO) SYSTEMLDFLAGS=$(SYSTEMLDFLAGS)
	$(ECHO) SYSTEMLIBS=$(SYSTEMLIBS)
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

