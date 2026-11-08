# license:BSD-3-Clause
# copyright-holders:Aaron Giles, Nicola Salmoria
# Shared rules: call with variable prefix, object directory and target name.
# Escape ordinary make references so each instantiation retains its own variables.
define EMU_CORE
###########################################################################
#
#   core.mak
#
#   MAME core makefile
#
###########################################################################


#-------------------------------------------------
# the core object files (without target specific
# objects; those are added in the target.mak
# files)
#-------------------------------------------------

$(1)COREOBJS = \
	$(2)/artwork.o \
	$(2)/artwork_lay.o \
	$(2)/artwork_package.o \
	$(2)/cdrom.o \
	$(2)/chdwrite.o \
	$(2)/cheat.o \
	$(2)/config.o \
	$(2)/cpuexec.o \
	$(2)/cpuint.o \
	$(2)/cpuintrf.o \
	$(2)/drawgfx.o \
	$(2)/driver.o \
	$(2)/fileio.o \
	$(2)/harddisk.o \
	$(2)/hash.o \
	$(2)/hiscore.o \
	$(2)/info.o \
	$(2)/input.o \
	$(2)/inptport.o \
	$(2)/mame.o \
	$(2)/mamecore.o \
	$(2)/md5.o \
	$(2)/memory.o \
	$(2)/palette.o \
	$(2)/png.o \
	$(2)/romload.o \
	$(2)/sha1.o \
	$(2)/sound.o \
	$(2)/sndintrf.o \
	$(2)/state.o \
	$(2)/streams.o \
	$(2)/tilemap.o \
	$(2)/timer.o \
	$(2)/ui_text.o \
	$(2)/unzip.o \
	$(2)/usrintrf.o \
	$(2)/validity.o \
	$(2)/version.o \
	$(2)/video.o \
	$(2)/xmlfile.o \
	$(2)/sound/filter.o \
	$(2)/sound/flt_vol.o \
	$(2)/sound/flt_rc.o \
	$(2)/sound/wavwrite.o \
	$(2)/machine/eeprom.o \
	$(2)/machine/generic.o \
	$(2)/sndhrdw/generic.o \
	$(2)/vidhrdw/generic.o \
	$(2)/vidhrdw/vector.o \


#-------------------------------------------------
# additional core files needed for the debugger
#-------------------------------------------------

ifeq ($$(CONF_DEBUGGER),yes)
$(1)COREOBJS += \
	$(2)/profiler.o

ifdef NEW_DEBUGGER
$(1)COREOBJS += \
	$(2)/debug/debugcmd.o \
	$(2)/debug/debugcmt.o \
	$(2)/debug/debugcon.o \
	$(2)/debug/debugcpu.o \
	$(2)/debug/debughlp.o \
	$(2)/debug/debugvw.o \
	$(2)/debug/express.o \
	$(2)/debug/textbuf.o
else
$(1)COREOBJS += \
	$(2)/debug/mamedbg.o \
	$(2)/debug/window.o
endif
endif


endef
