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

# Uncommment the target operating system or the current one is used:
#HOST_TARGET=linux
#HOST_TARGET=dos
#HOST_TARGET=windows

# Uncomment the main system library to use.
# The available choices for Linux are :
#   linux, sdl (default is linux)
# The available choices for DOS are :
#   dos (default is dos)
# The available choices for Windows are :
#   sdl (default is sdl)
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
#   SVGALIB and Frame Buffer Consolle for the graphics output
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

# Uncomment to enable the SMP (Multiprocessor) code:
USE_SMP=1

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
#CFLAGS_ARCH = -DUSE_MSB

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
ifneq (,$(findstring Linux,$(HOST)))
HOST_BUILD=linux
else
ifneq (,$(findstring DOS,$(HOST)))
HOST_BUILD=dos
else
HOST_BUILD=windows
endif
endif

# Target environment
ifndef HOST_TARGET
HOST_TARGET=$(HOST_BUILD)
endif

# System environment
ifndef HOST_SYSTEM
ifeq ($(HOST_TARGET),windows)
HOST_SYSTEM=sdl
else
HOST_SYSTEM=$(HOST_TARGET)
endif
endif

# Binary description
ifeq ($(HOST_TARGET),$(HOST_SYSTEM))
BINARYTAG = $(HOST_TARGET)-native-$(ARCH)
else
BINARYTAG = $(HOST_TARGET)-$(HOST_SYSTEM)-$(ARCH)
endif
BINARYDIR = $(HOST_TARGET)/$(HOST_SYSTEM)/$(ARCH)

# Check
ifeq (,$(findstring $(HOST_TARGET),/linux/dos/windows/))
$(error Invalid HOST_TARGET=$(HOST_TARGET))
endif
ifeq (,$(findstring $(HOST_SYSTEM),/linux/dos/sdl/))
$(error Invalid HOST_SYSTEM=$(HOST_SYSTEM))
endif
ifeq ($(HOST_TARGET),linux)
ifeq (,$(findstring $(HOST_SYSTEM),/linux/sdl/))
$(error Unsupported HOST_SYSTEM=$(HOST_SYSTEM) for HOST_TARGET=$(HOST_TARGET))
endif
endif
ifeq ($(HOST_TARGET),dos)
ifeq (,$(findstring $(HOST_SYSTEM),/dos/))
$(error Unsupported HOST_SYSTEM=$(HOST_SYSTEM) for HOST_TARGET=$(HOST_TARGET))
endif
endif
ifeq ($(HOST_TARGET),windows)
ifeq (,$(findstring $(HOST_SYSTEM),/sdl/))
$(error Unsupported HOST_SYSTEM=$(HOST_SYSTEM) for HOST_TARGET=$(HOST_TARGET))
endif
endif

############################################################################
# Tool options

MAKE = @make -j 2
MD = -@mkdir -p
RM = @rm -f
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
# Don't compress the linux executables. I like to see the ldd dependencies.
UPX = @true
LN = @ln -s
EXE =
ASMFLAGS = -f elf
CFLAGS-HOST = -O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_ELF
ZLIBS = -lz
endif
ifeq ($(HOST_TARGET),dos)
UPX = @upx -q -q -q
LN = @cp
EXE = .exe
ASMFLAGS = -f coff
CFLAGS-HOST = -O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_COFF
ZLIBS = -lz
endif
ifeq ($(HOST_TARGET),windows)
UPX = @upx -q -q -q
LN = @cp
EXE = .exe
ASMFLAGS = -f coff
CFLAGS-HOST = -O0 -DCOMPILER_TARGET_GNUC -DOBJ_TARGET_COFF
ZLIBS = -static -lz
endif

ifeq ($(HOST_BUILD),linux)
ASM = @nasm
EXE-HOST =
CC-HOST = @gcc
TOUCH = @touch
ifeq ($(HOST_TARGET),linux)
AR = @ar
CC = @gcc
CXX = @g++
LD = @gcc
LDXX = @g++
ifeq ($(SYMBOLS),1)
# For the stack backtrace
#LDFLAGS += -rdynamic
endif
SDLCFLAGS = $(shell sdl-config --cflags)
SDLLIBS = $(shell sdl-config --libs)
endif
ifeq ($(HOST_TARGET),dos)
# Probably you need to changes these to cross compile:
DJDIR = /mnt/dos1/djgpp
LDFLAGS += -L $(DJDIR)/lib
CROSSTARGET = i586-pc-msdosdjgpp
CROSSDIR = /usr
AR = @$(CROSSDIR)/bin/$(CROSSTARGET)-ar
CC = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
CXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
LD = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc -B$(CROSSDIR)/bin/
LDXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++ -B$(CROSSDIR)/bin/
endif
ifeq ($(HOST_TARGET),windows)
# Probably you need to changes these to cross compile:
CROSSTARGET = i386-mingw32msvc
CROSSDIR = /usr/local/cross-tools
AR = @$(CROSSDIR)/bin/$(CROSSTARGET)-ar
CC = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
CXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
LD = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc -B$(CROSSDIR)/bin/
LDXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++ -B$(CROSSDIR)/bin/
RC = @$(CROSSDIR)/bin/$(CROSSTARGET)-windres
SDLCFLAGS = -I$(CROSSDIR)/$(CROSSTARGET)/include/SDL
SDLLIBS = -lSDL -mwindows
endif
endif

ifeq ($(HOST_BUILD),dos)
ASM = @nasm
EXE-HOST = .exe
CC-HOST = @gcc
AR = @ar
CC = @gcc
CXX = @gxx
LD = @gcc
LDXX = @gxx
TOUCH = @rem
endif

ifeq ($(HOST_BUILD),windows)
ASM = @nasmw
EXE-HOST = .exe
CC-HOST = @gcc
AR = @ar
CC = @gcc
CXX = @g++
LD = @gcc
LDXX = @g++
TOUCH = @true
RC = @windres
# The "" are required, otherwise the backslash are removed
SDLCFLAGS = "-IC:\MINGW\INCLUDE\SDL"
SDLLIBS = -lSDL -mwindows
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
	$(CC) $(CFLAGS) $(EMUCFLAGS) $(EMUDEFS) -c $< -o $@

$(OBJ)/mess/%.o: mess/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(EMUCFLAGS) $(EMUDEFS) -c $< -o $@

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
	$(ECHO) SDLCFLAGS=$(SDLCFLAGS)
	$(ECHO) SDLLIBS=$(SDLLIBS)
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

os:
	rgrep -r MSDOS advance
	rgrep -r WIN32 advance

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

