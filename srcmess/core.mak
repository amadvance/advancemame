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

MESSCOREOBJS = \
	$(MESSOBJ)/artwork.o \
	$(MESSOBJ)/audit.o \
	$(MESSOBJ)/cdrom.o \
	$(MESSOBJ)/chd.o \
	$(MESSOBJ)/cheat.o \
	$(MESSOBJ)/config.o \
	$(MESSOBJ)/cpuexec.o \
	$(MESSOBJ)/cpuint.o \
	$(MESSOBJ)/cpuintrf.o \
	$(MESSOBJ)/drawgfx.o \
	$(MESSOBJ)/driver.o \
	$(MESSOBJ)/fileio.o \
	$(MESSOBJ)/harddisk.o \
	$(MESSOBJ)/hash.o \
	$(MESSOBJ)/hiscore.o \
	$(MESSOBJ)/info.o \
	$(MESSOBJ)/input.o \
	$(MESSOBJ)/inptport.o \
	$(MESSOBJ)/jedparse.o \
	$(MESSOBJ)/mame.o \
	$(MESSOBJ)/mamecore.o \
	$(MESSOBJ)/md5.o \
	$(MESSOBJ)/memory.o \
	$(MESSOBJ)/palette.o \
	$(MESSOBJ)/png.o \
	$(MESSOBJ)/romload.o \
	$(MESSOBJ)/sha1.o \
	$(MESSOBJ)/sound.o \
	$(MESSOBJ)/sndintrf.o \
	$(MESSOBJ)/state.o \
	$(MESSOBJ)/streams.o \
	$(MESSOBJ)/tilemap.o \
	$(MESSOBJ)/timer.o \
	$(MESSOBJ)/ui_text.o \
	$(MESSOBJ)/unzip.o \
	$(MESSOBJ)/usrintrf.o \
	$(MESSOBJ)/validity.o \
	$(MESSOBJ)/version.o \
	$(MESSOBJ)/video.o \
	$(MESSOBJ)/xmlfile.o \
	$(MESSOBJ)/sound/filter.o \
	$(MESSOBJ)/sound/flt_vol.o \
	$(MESSOBJ)/sound/flt_rc.o \
	$(MESSOBJ)/sound/wavwrite.o \
	$(MESSOBJ)/machine/eeprom.o \
	$(MESSOBJ)/machine/generic.o \
	$(MESSOBJ)/sndhrdw/generic.o \
	$(MESSOBJ)/vidhrdw/generic.o \
	$(MESSOBJ)/vidhrdw/vector.o \

ifdef X86_MIPS3_DRC
MESSCOREOBJS += $(MESSOBJ)/x86drc.o
else
ifdef X86_PPC_DRC
MESSCOREOBJS += $(MESSOBJ)/x86drc.o
endif
endif


#-------------------------------------------------
# additional core files needed for the debugger
#-------------------------------------------------

ifdef DEBUG
MESSCOREOBJS += \
	$(MESSOBJ)/profiler.o

ifdef NEW_DEBUGGER
MESSCOREOBJS += \
	$(MESSOBJ)/debug/debugcmd.o \
	$(MESSOBJ)/debug/debugcmt.o \
	$(MESSOBJ)/debug/debugcon.o \
	$(MESSOBJ)/debug/debugcpu.o \
	$(MESSOBJ)/debug/debughlp.o \
	$(MESSOBJ)/debug/debugvw.o \
	$(MESSOBJ)/debug/express.o \
	$(MESSOBJ)/debug/textbuf.o
else
MESSCOREOBJS += \
	$(MESSOBJ)/debug/mamedbg.o \
	$(MESSOBJ)/debug/window.o
endif
endif



#-------------------------------------------------
# set of tool targets
#-------------------------------------------------

TOOLS = romcmp$(EXE) chdman$(EXE) xml2info$(EXE) jedutil$(EXE)
