#############################################################################
# Manual user configuration
#
# If you use the ./configure script you don't need to edit this file.
#

# Installation prefix (only for Unix):
prefix=/usr/local

# Select the emu executable (mame by default)
# The available choices are: mame, mess
#CONF_EMU=mame

# Uncomment the host operating system.
# The available choices are linux, dos, windows.
#
# The unix host uses:
#   SVGALIB, Frame Buffer, SDL for the video
#   tty, SDL for the text
#   ALSA, OSS, SDL for the sound
#   SVGALIB, KEVENT, KRAW, SDL for the keyboard
#   SVGALIB, JEVENT, JRAW, SDL for the joystick
#   SVGALIB, MEVENT, MRAW, SDL for the mouse
#   pthread for the Multiprocessor support
#
# The dos host uses:
#   SVGALINE, VBELINE, VGALINE, VBE for the video
#   VGALINE for the text
#   SEAL, Allegro and VSYNC for the sound
#   Allegro for the keyboard
#   Allegro for the joystick
#   Allegro for the mouse
#   none for the Multiprocessor support (no Multiprocessor support)
#
# The windows host uses:
#   SVGAWIN, SDL for the video
#   SDL for the text
#   SDL for the sound
#   SDL for the keyboard
#   JLGRAWINPUT, SDL for the joystick
#   MRAWINPUT, MCPN, SDL for the mouse
#   none for the Multiprocessor support (no Multiprocessor support)
#
#CONF_HOST=linux
#CONF_HOST=dos
#CONF_HOST=windows

# Uncomment and set to "no" to disable some libraries:
#CONF_LIB_DIRECT=yes
#CONF_LIB_SVGALIB=yes
#CONF_LIB_SVGAWIN=yes
#CONF_LIB_SDL=yes
#CONF_LIB_FREETYPE=yes
#CONF_LIB_FB=yes
#CONF_LIB_ALSA=yes
#CONF_LIB_OSS=yes
#CONF_LIB_PTHREAD=yes
#CONF_LIB_SLANG=yes
#CONF_LIB_NCURSES=yes
#CONF_LIB_KRAW=yes
#CONF_LIB_JRAW=yes
#CONF_LIB_MRAW=yes
#CONF_LIB_KEVENT=yes
#CONF_LIB_JEVENT=yes
#CONF_LIB_MEVENT=yes
#CONF_LIB_MRAWINPUT=yes
#CONF_LIB_JLGRAWINPUT=yes
#CONF_LIB_MCPN=yes

# Select the optimization flags: (optimized build for Pentium by default)
# Optimized build for Pentium IV
#CONF_CFLAGS_OPT=-O2 -fomit-frame-pointer -march=pentium4 -Wall -Wno-sign-compare -Wno-unused
# Optimized build for K6, K6-II, K6-III
#CONF_CFLAGS_OPT=-O2 -fomit-frame-pointer -march=k6 -Wall -Wno-sign-compare -Wno-unused
# Optimized build for Duron, Athlon, Athlon XP, Athlon MP
#CONF_CFLAGS_OPT=-O2 -fomit-frame-pointer -march=athlon -Wall -Wno-sign-compare -Wno-unused

# Select the optimization linker flags: (optimized link by default)
# Optimized build
#CONF_LDFLAGS=-s

# Uncomment and set to "yes" to compile the MAME debugger (default no):
#CONF_DEBUGGER=no

# Uncomment and set to "yes" to enable the debug code (default no):
#CONF_DEBUG=no

#############################################################################
# Completing manual configuration with defaults
#

# Automatic EMU detection by source
ifeq ($(CONF_EMU),)
ifneq ($(wildcard src),)
CONF_EMU = mame
endif
endif
ifeq ($(CONF_EMU),)
ifneq ($(wildcard srcmess),)
CONF_EMU = mess
endif
endif

# Automatic BUILD environment detection
ifndef CONF_BUILD
UNAME := $(shell uname)
ifneq (,$(findstring Linux,$(UNAME)))
CONF_BUILD=linux
else
ifneq (,$(findstring DOS,$(UNAME)))
CONF_BUILD=dos
else
CONF_BUILD=windows
endif
endif
endif

# Automatic HOST environment detection
ifndef CONF_HOST
CONF_HOST=$(CONF_BUILD)
endif

# Automatic libraries and other options setting
ifndef CONF_LIB_ZLIB
ifeq ($(CONF_HOST),dos)
CONF_LIB_ZLIB=no
else
CONF_LIB_ZLIB=yes
endif
endif

ifndef CONF_LIB_EXPAT
ifeq ($(CONF_HOST),dos)
CONF_LIB_EXPAT=no
else
CONF_LIB_EXPAT=yes
endif
endif

ifndef CONF_LIB_DIRECT
CONF_LIB_DIRECT=yes
endif

ifndef CONF_LIB_SVGALIB
CONF_LIB_SVGALIB=no
endif
     
ifndef CONF_LIB_FB
CONF_LIB_FB=yes
endif

ifndef CONF_LIB_ALSA
CONF_LIB_ALSA=yes
endif

ifndef CONF_LIB_OSS
CONF_LIB_OSS=no
endif
     
ifndef CONF_LIB_SDL
CONF_LIB_SDL=yes
endif

ifndef CONF_LIB_FREETYPE
CONF_LIB_FREETYPE=yes
endif

ifndef CONF_LIB_SVGAWIN
CONF_LIB_SVGAWIN=yes
endif

ifndef CONF_LIB_PTHREAD
CONF_LIB_PTHREAD=yes
endif
     
ifndef CONF_LIB_SLANG
CONF_LIB_SLANG=yes
endif

ifndef CONF_LIB_NCURSES
CONF_LIB_NCURSES=yes
endif

ifndef CONF_LIB_KRAW
CONF_LIB_KRAW=yes
endif

ifndef CONF_LIB_JRAW
CONF_LIB_JRAW=yes
endif

ifndef CONF_LIB_MRAW
CONF_LIB_MRAW=yes
endif

ifndef CONF_LIB_KEVENT
CONF_LIB_KEVENT=yes
endif

ifndef CONF_LIB_JEVENT
CONF_LIB_JEVENT=yes
endif

ifndef CONF_LIB_MEVENT
CONF_LIB_MEVENT=yes
endif

ifndef CONF_LIB_MRAWINPUT
CONF_LIB_MRAWINPUT=yes
endif

ifndef CONF_LIB_JLGRAWINPUT
CONF_LIB_JLGRAWINPUT=yes
endif

ifndef CONF_LIB_MCPN
CONF_LIB_MCPN=yes
endif

ifndef CONF_CFLAGS_ARCH
CONF_CFLAGS_ARCH = -DUSE_LSB -DUSE_ASM_INLINE -DUSE_ASM_EMUMIPS3
endif

ifndef CONF_CFLAGS_OPT
CONF_CFLAGS_OPT = -march=i686 -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-merge-constants -Wall -Wno-sign-compare -Wno-unused
endif

ifndef CONF_LDFLAGS
CONF_LDFLAGS=-s
endif

ifndef CONF_DEBUGGER
CONF_DEBUGGER=no
endif

ifndef CONF_DEBUG
CONF_DEBUG=no
endif

ifndef CONF_DEFS
CONF_DEFS=
endif

srcdir=.
datadir=${prefix}/share
sysconfdir=${prefix}/etc
bindir=${prefix}/bin
mandir=${prefix}/man
docdir=${prefix}/share/doc

#############################################################################
# Extra configuration common for ./configure and manual
#

# Enable the creation of the map files
ifndef CONF_MAP
CONF_MAP=no
endif

# Name of the architecture. Used in the distribution file names.
ifndef CONF_ARCH
CONF_ARCH=blend
endif

# Pack also the emulator source in the dist package
ifndef CONF_DIFFSRC
CONF_DIFFSRC=no
endif

#############################################################################
# Tools configuration for manual
#

MD = -@mkdir -p
RM = @rm -f
ECHO = @echo

# No installation with the manual configuration
INSTALL = false

ifeq ($(CONF_HOST),linux)
LN_S = @ln -s
EXE =
ASMFLAGS = -f elf
CFLAGS_FOR_BUILD += -O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_ELF -DUSE_OS_UNIX
endif
ifeq ($(CONF_HOST),dos)
LN_S = @cp -p
EXE = .exe
ASMFLAGS = -f coff
CFLAGS_FOR_BUILD += -O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_DOS
endif
ifeq ($(CONF_HOST),windows)
LN_S = @cp -p
EXE = .exe
ASMFLAGS = -f coff
CFLAGS_FOR_BUILD += -O0 -DUSE_COMPILER_GNUC -DUSE_OBJ_COFF -DUSE_OS_WINDOWS
endif

ifeq ($(CONF_BUILD),linux)
ASM = @nasm
EXE_FOR_BUILD =
CC_FOR_BUILD = @gcc
LD_FOR_BUILD = @gcc
CXX_FOR_BUILD = @g++
LDXX_FOR_BUILD = @g++
ifeq ($(CONF_HOST),linux)
AR = @ar
CC = @gcc
CXX = @g++
LD = @gcc
LDXX = @g++
SDLCFLAGS = $(shell sdl-config --cflags)
SDLLIBS = $(shell sdl-config --libs)
FREETYPECFLAGS = $(shell freetype-config --cflags)
FREETYPELIBS = $(shell freetype-config --libs)
endif
ifeq ($(CONF_HOST),dos)
# Probably you need to changes these to cross compile:
CROSSTARGET = i586-pc-msdosdjgpp
CROSSDIR = /usr/local/gcc-3.2.3-i586-pc-msdosdjgpp
AR = @$(CROSSDIR)/bin/$(CROSSTARGET)-ar
CC = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
CXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
LD = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
LDXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
FREETYPECFLAGS = -I$(CROSSDIR)/include/freetype2 -I$(CROSSDIR)/include
FREETYPELIBS = -L$(CROSSDIR)/lib -lfreetype -lz
endif
ifeq ($(CONF_HOST),windows)
# Probably you need to changes these to cross compile:
CROSSTARGET = i686-pc-mingw32
CROSSDIR = /usr/local/mingw-cross-env-2.19
AR = @$(CROSSDIR)/bin/$(CROSSTARGET)-ar
CC = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
CXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
LD = @$(CROSSDIR)/bin/$(CROSSTARGET)-gcc
LDXX = @$(CROSSDIR)/bin/$(CROSSTARGET)-g++
RC = @$(CROSSDIR)/bin/$(CROSSTARGET)-windres
SDLCFLAGS = -I$(CROSSDIR)/$(CROSSTARGET)/include/SDL -I$(CROSSDIR)/$(CROSSTARGET)/include
SDLLIBS = -L$(CROSSDIR)/$(CROSSTARGET)/lib -lmingw32 -lSDL -lwinmm -mwindows -liconv -lm -luser32 -lgdi32 -lwinmm -ldxguid
FREETYPECFLAGS = -I$(CROSSDIR)/$(CROSSTARGET)/include/freetype2 -I$(CROSSDIR)/$(CROSSTARGET)/include
FREETYPELIBS = -L$(CROSSDIR)/$(CROSSTARGET)/lib -lfreetype -lz
endif
endif

ifeq ($(CONF_BUILD),dos)
ASM = @nasm
EXE_FOR_BUILD = .exe
CC_FOR_BUILD = @gcc
LD_FOR_BUILD = @gcc
CXX_FOR_BUILD = @gxx
LDXX_FOR_BUILD = @gxx
AR = @ar
CC = @gcc
CXX = @gxx
LD = @gcc
LDXX = @gxx
FREETYPECFLAGS = "-IC:\DJGPP\INCLUDE" "-IC:\DJGPP\INCLUDE\FREETYPE2"
FREETYPELIBS = "-LC:\DJGPP\LIB" -lfreetype -lz
endif

ifeq ($(CONF_BUILD),windows)
ASM = @nasmw
EXE_FOR_BUILD = .exe
CC_FOR_BUILD = @gcc
LD_FOR_BUILD = @gcc "-LC:\MINGW\LIB" 
CXX_FOR_BUILD = @g++
LDXX_FOR_BUILD = @g++ "-LC:\MINGW\LIB" 
AR = @ar
CC = @gcc
CXX = @g++
LD = @gcc
LDXX = @g++
RC = @windres
# The "" are required, otherwise the backslash are removed
SDLCFLAGS = "-IC:\MINGW\INCLUDE" "-IC:\MINGW\INCLUDE\SDL"
SDLLIBS = "-LC:\MINGW\LIB" -lmingw32 -lSDL -mwindows
FREETYPECFLAGS = "-IC:\MINGW\INCLUDE" "-IC:\MINGW\INCLUDE\FREETYPE2"
FREETYPELIBS = -lfreetype -lz
endif

# No installation with the manual configuration
INSTALL_PROGRAM_DIR = false
INSTALL_MAN_DIR = false
INSTALL_DATA_DIR = false
INSTALL_PROGRAM = false
INSTALL_MAN = false
INSTALL_DATA = false

#############################################################################
# Root makefile

include root.mak

