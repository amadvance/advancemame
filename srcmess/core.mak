###########################################################################
#
#   core.mak
#
#   MAME core makefile
#
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


#-------------------------------------------------
# the core object files (without target specific
# objects; those are added in the target.mak
# files)
#-------------------------------------------------

COREOBJS = \
	$(OBJ)/artwork.o \
	$(OBJ)/audit.o \
	$(OBJ)/cdrom.o \
	$(OBJ)/chd.o \
	$(OBJ)/cheat.o \
	$(OBJ)/config.o \
	$(OBJ)/cpuexec.o \
	$(OBJ)/cpuint.o \
	$(OBJ)/cpuintrf.o \
	$(OBJ)/drawgfx.o \
	$(OBJ)/driver.o \
	$(OBJ)/fileio.o \
	$(OBJ)/harddisk.o \
	$(OBJ)/hash.o \
	$(OBJ)/hiscore.o \
	$(OBJ)/info.o \
	$(OBJ)/input.o \
	$(OBJ)/inptport.o \
	$(OBJ)/jedparse.o \
	$(OBJ)/mame.o \
	$(OBJ)/mamecore.o \
	$(OBJ)/md5.o \
	$(OBJ)/memory.o \
	$(OBJ)/palette.o \
	$(OBJ)/png.o \
	$(OBJ)/romload.o \
	$(OBJ)/sha1.o \
	$(OBJ)/sound.o \
	$(OBJ)/sndintrf.o \
	$(OBJ)/state.o \
	$(OBJ)/streams.o \
	$(OBJ)/tilemap.o \
	$(OBJ)/timer.o \
	$(OBJ)/ui_text.o \
	$(OBJ)/unzip.o \
	$(OBJ)/usrintrf.o \
	$(OBJ)/validity.o \
	$(OBJ)/version.o \
	$(OBJ)/video.o \
	$(OBJ)/xmlfile.o \
	$(OBJ)/sound/filter.o \
	$(OBJ)/sound/flt_vol.o \
	$(OBJ)/sound/flt_rc.o \
	$(OBJ)/sound/wavwrite.o \
	$(OBJ)/machine/eeprom.o \
	$(OBJ)/machine/generic.o \
	$(OBJ)/sndhrdw/generic.o \
	$(OBJ)/vidhrdw/generic.o \
	$(OBJ)/vidhrdw/vector.o \

ifdef X86_MIPS3_DRC
COREOBJS += $(OBJ)/x86drc.o
else
ifdef X86_PPC_DRC
COREOBJS += $(OBJ)/x86drc.o
endif
endif


#-------------------------------------------------
# additional core files needed for the debugger
#-------------------------------------------------

ifdef DEBUG
COREOBJS += \
	$(OBJ)/profiler.o

ifdef NEW_DEBUGGER
COREOBJS += \
	$(OBJ)/debug/debugcmd.o \
	$(OBJ)/debug/debugcmt.o \
	$(OBJ)/debug/debugcon.o \
	$(OBJ)/debug/debugcpu.o \
	$(OBJ)/debug/debughlp.o \
	$(OBJ)/debug/debugvw.o \
	$(OBJ)/debug/express.o \
	$(OBJ)/debug/textbuf.o
else
COREOBJS += \
	$(OBJ)/debug/mamedbg.o \
	$(OBJ)/debug/window.o
endif
endif



#-------------------------------------------------
# set of tool targets
#-------------------------------------------------

TOOLS = romcmp$(EXE) chdman$(EXE) xml2info$(EXE) jedutil$(EXE)
