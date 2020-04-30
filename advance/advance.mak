############################################################################
# Common dir

ifeq ($(CONF_TINY),yes)
OBJ = obj/mame-tiny/$(BINARYDIR)
MESSOBJ = obj/mess-tiny/$(BINARYDIR)
else
OBJ = obj/mame/$(BINARYDIR)
MESSOBJ = obj/mess/$(BINARYDIR)
endif
MENUOBJ = obj/menu/$(BINARYDIR)
MOBJ = obj/m/$(BINARYDIR)
JOBJ = obj/j/$(BINARYDIR)
KOBJ = obj/k/$(BINARYDIR)
IOBJ = obj/i/$(BINARYDIR)
VOBJ = obj/v/$(BINARYDIR)
SOBJ = obj/s/$(BINARYDIR)
BLUEOBJ = obj/blue/$(BINARYDIR)
CFGOBJ = obj/cfg/$(BINARYDIR)
LINEOBJ = obj/line/$(BINARYBUILDDIR)
D2OBJ = obj/d2/$(BINARYBUILDDIR)
DOCOBJ = $(srcdir)/doc

############################################################################
# Binaries

MAME_INSTALL_BINFILES = $(OBJ)/advmame$(EXE)
MAME_INSTALL_MANFILES = $(DOCOBJ)/advmame.1 $(DOCOBJ)/advdev.1
MAME_INSTALL_DATAFILES = $(srcdir)/support/event.dat \
	$(srcdir)/support/history.dat \
	$(srcdir)/support/hiscore.dat \
	$(srcdir)/support/cheat.dat \
	$(srcdir)/support/category.ini
# Freely available roms: http://www.mame.net/roms/
MAME_INSTALL_ROMFILES = $(srcdir)/support/free/rom/gridlee.zip \
	$(srcdir)/support/free/rom/polyplay.zip \
	$(srcdir)/support/free/rom/robby.zip
MAME_INSTALL_SAMPLEFILES = $(srcdir)/support/free/rom/gridlee.zip
MAME_INSTALL_SNAPFILES = $(srcdir)/support/free/snap/gridlee.zip \
	$(srcdir)/support/free/snap/polyplay.zip \
	$(srcdir)/support/free/snap/robby.zip
MESS_INSTALL_BINFILES = $(MESSOBJ)/advmess$(EXE)
MESS_INSTALL_MANFILES = $(srcdir)/support/advmess.1
MESS_INSTALL_DATAFILES = $(srcdir)/support/sysinfo.dat
# Freely available systems: http://www.mess.org/freely_available_systems#texas_instruments_ti-994a_and_ti-998
MESS_INSTALL_ROMFILES = $(srcdir)/support/free/rom/ti99_4a.zip
MESS_INSTALL_IMAGEFILES_TI99_4A = \
	$(srcdir)/support/free/image/ti99_4a/alpiner.zip \
	$(srcdir)/support/free/image/ti99_4a/attack.zip \
	$(srcdir)/support/free/image/ti99_4a/carwars.zip \
	$(srcdir)/support/free/image/ti99_4a/munchmn.zip \
	$(srcdir)/support/free/image/ti99_4a/parsec.zip \
	$(srcdir)/support/free/image/ti99_4a/ti-inva.zip \
	$(srcdir)/support/free/image/ti99_4a/tombcit.zip \
	$(srcdir)/support/free/image/ti99_4a/v-chess.zip \
	$(srcdir)/support/free/image/ti99_4a/vidgam1.zip \
	$(srcdir)/support/free/image/ti99_4a/vidgam2.zip
MESS_INSTALL_SNAPFILES = $(srcdir)/support/free/snap/ti99_4a.png
MESS_INSTALL_SNAPFILES_TI99_4A = \
	$(srcdir)/support/free/snap/ti99_4a/alpiner.zip \
	$(srcdir)/support/free/snap/ti99_4a/attack.zip \
	$(srcdir)/support/free/snap/ti99_4a/carwars.zip \
	$(srcdir)/support/free/snap/ti99_4a/munchmn.zip \
	$(srcdir)/support/free/snap/ti99_4a/parsec.zip \
	$(srcdir)/support/free/snap/ti99_4a/ti-inva.zip \
	$(srcdir)/support/free/snap/ti99_4a/tombcit.zip \
	$(srcdir)/support/free/snap/ti99_4a/v-chess.zip \
	$(srcdir)/support/free/snap/ti99_4a/vidgam1.zip \
	$(srcdir)/support/free/snap/ti99_4a/vidgam2.zip
MENU_INSTALL_BINFILES = $(MENUOBJ)/advmenu$(EXE)
MENU_INSTALL_MANFILES = $(DOCOBJ)/advmenu.1
CFG_INSTALL_BINFILES = $(CFGOBJ)/advcfg$(EXE)
CFG_INSTALL_MANFILES = $(DOCOBJ)/advcfg.1
V_INSTALL_BINFILES = $(VOBJ)/advv$(EXE)
V_INSTALL_MANFILES = $(DOCOBJ)/advv.1
S_INSTALL_BINFILES = $(SOBJ)/advs$(EXE)
S_INSTALL_MANFILES = $(DOCOBJ)/advs.1
K_INSTALL_BINFILES = $(KOBJ)/advk$(EXE)
K_INSTALL_MANFILES = $(DOCOBJ)/advk.1
J_INSTALL_BINFILES = $(JOBJ)/advj$(EXE)
J_INSTALL_MANFILES = $(DOCOBJ)/advj.1
M_INSTALL_BINFILES = $(MOBJ)/advm$(EXE)
M_INSTALL_MANFILES = $(DOCOBJ)/advm.1
BLUE_INSTALL_BINFILES = $(BLUEOBJ)/advblue$(EXE)
BLUE_INSTALL_MANFILES = $(DOCOBJ)/advblue.1

############################################################################
# Install

ifneq ($(wildcard $(srcdir)/advance/emu.mak),)
OBJ_DIRS += $(OBJ)
INSTALL_BINFILES += $(MAME_INSTALL_BINFILES)
INSTALL_DATAFILES += $(MAME_INSTALL_DATAFILES)
INSTALL_MANFILES += $(MAME_INSTALL_MANFILES)
INSTALL_ROMFILES += $(MAME_INSTALL_ROMFILES)
INSTALL_SAMPLEFILES += $(MAME_INSTALL_SAMPLEFILES)
INSTALL_SNAPFILES += $(MAME_INSTALL_SNAPFILES)
OBJ_DIRS += $(MESSOBJ)
INSTALL_BINFILES += $(MESS_INSTALL_BINFILES)
INSTALL_DATAFILES += $(MESS_INSTALL_DATAFILES)
INSTALL_MANFILES += $(MESS_INSTALL_MANFILES)
INSTALL_ROMFILES += $(MESS_INSTALL_ROMFILES)
INSTALL_IMAGEFILES_TI99_4A += $(MESS_INSTALL_IMAGEFILES_TI99_4A)
INSTALL_SNAPFILES += $(MESS_INSTALL_SNAPFILES)
INSTALL_SNAPFILES_TI99_4A += $(MESS_INSTALL_SNAPFILES_TI99_4A)
endif
ifneq ($(wildcard $(srcdir)/advance/menu.mak),)
OBJ_DIRS += $(MENUOBJ)
INSTALL_BINFILES += $(MENU_INSTALL_BINFILES)
INSTALL_MANFILES += $(MENU_INSTALL_MANFILES)
endif
ifeq ($(CONF_LIB_DIRECT),yes)
ifneq ($(wildcard $(srcdir)/advance/cfg.mak),)
OBJ_DIRS += $(CFGOBJ)
INSTALL_BINFILES += $(CFG_INSTALL_BINFILES)
INSTALL_MANFILES += $(CFG_INSTALL_MANFILES)
endif
ifneq ($(wildcard $(srcdir)/advance/v.mak),)
OBJ_DIRS += $(VOBJ)
INSTALL_BINFILES += $(V_INSTALL_BINFILES)
INSTALL_MANFILES += $(V_INSTALL_MANFILES)
endif
endif
ifneq ($(CONF_SYSTEM),windows)
ifneq ($(wildcard $(srcdir)/advance/s.mak),)
OBJ_DIRS += $(SOBJ)
INSTALL_BINFILES += $(S_INSTALL_BINFILES)
INSTALL_MANFILES += $(S_INSTALL_MANFILES)
endif
ifneq ($(wildcard $(srcdir)/advance/k.mak),)
OBJ_DIRS += $(KOBJ)
INSTALL_BINFILES += $(K_INSTALL_BINFILES)
INSTALL_MANFILES += $(K_INSTALL_MANFILES)
endif
ifneq ($(wildcard $(srcdir)/advance/j.mak),)
OBJ_DIRS += $(JOBJ)
INSTALL_BINFILES += $(J_INSTALL_BINFILES)
INSTALL_MANFILES += $(J_INSTALL_MANFILES)
endif
ifneq ($(wildcard $(srcdir)/advance/m.mak),)
OBJ_DIRS += $(MOBJ)
INSTALL_BINFILES += $(M_INSTALL_BINFILES)
INSTALL_MANFILES += $(M_INSTALL_MANFILES)
endif
endif
ifeq ($(CONF_SYSTEM),unix)
ifneq ($(wildcard $(srcdir)/advance/blue.mak),)
OBJ_DIRS += $(BLUEOBJ)
INSTALL_BINFILES += $(BLUE_INSTALL_BINFILES)
INSTALL_MANFILES += $(BLUE_INSTALL_MANFILES)
endif
endif

INSTALL_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.txt,$(wildcard $(srcdir)/doc/*.d)))
INSTALL_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.html,$(wildcard $(srcdir)/doc/*.d)))
WEB_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.hh,$(wildcard $(srcdir)/doc/*.d)))

############################################################################
# Build

ifdef ADV_ALL
all_override: $(ADV_ALL)
endif

all: $(OBJ_DIRS) $(INSTALL_BINFILES) $(INSTALL_DOCFILES) $(INSTALL_MANFILES)
mame: $(OBJ) $(OBJ)/advmame$(EXE)
mess: $(MESSOBJ) $(MESSOBJ)/advmess$(EXE)
emu: mame mess
menu: $(MENUOBJ) $(MENUOBJ)/advmenu$(EXE)
cfg: $(CFGOBJ) $(CFGOBJ)/advcfg$(EXE)
v: $(VOBJ) $(VOBJ)/advv$(EXE)
s: $(SOBJ) $(SOBJ)/advs$(EXE)
k: $(KOBJ) $(KOBJ)/advk$(EXE)
i: $(IOBJ) $(IOBJ)/advi$(EXE)
j: $(JOBJ) $(JOBJ)/advj$(EXE)
m: $(MOBJ) $(MOBJ)/advm$(EXE)
blue: $(BLUEOBJ) $(BLUEOBJ)/advblue$(EXE)
line: $(LINEOBJ) $(LINEOBJ)/advline$(EXE_FOR_BUILD)
d2: $(D2OBJ) $(D2OBJ)/advd2$(EXE_FOR_BUILD)
web: $(WEB_DOCFILES)

# Ensure that the doc target is always created also if a doc directory exists
.PHONY: doc

doc: $(INSTALL_DOCFILES)

############################################################################
# Source

RCSRC = $(srcdir)/support/pcvga.rc \
	$(srcdir)/support/pcsvga60.rc \
	$(srcdir)/support/standard.rc \
	$(srcdir)/support/medium.rc \
	$(srcdir)/support/extended.rc \
	$(srcdir)/support/pal.rc \
	$(srcdir)/support/ntsc.rc

MPGLIB_SRC = \
	$(wildcard $(srcdir)/advance/mpglib/*.c) \
	$(wildcard $(srcdir)/advance/mpglib/*.h) \
	$(wildcard $(srcdir)/advance/mpglib/*.txt)

LIB_SRC = \
	$(wildcard $(srcdir)/advance/lib/*.c) \
	$(wildcard $(srcdir)/advance/lib/*.h) \
	$(wildcard $(srcdir)/advance/lib/*.hin) \
	$(wildcard $(srcdir)/advance/lib/*.ico) \
	$(wildcard $(srcdir)/advance/lib/*.rc) \
	$(wildcard $(srcdir)/advance/lib/*.dat) \
	$(wildcard $(srcdir)/advance/lib/*.cfg) \
	$(wildcard $(srcdir)/advance/lib/*.html) \
	$(wildcard $(srcdir)/advance/lib/*.bmp)

BLIT_SRC = \
	$(wildcard $(srcdir)/advance/blit/*.c) \
	$(wildcard $(srcdir)/advance/blit/*.h) \
	$(wildcard $(srcdir)/advance/blit/*.dat)

CARD_SRC = \
	$(wildcard $(srcdir)/advance/card/*.c) \
	$(wildcard $(srcdir)/advance/card/*.h)
	
SVGALIB_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/*.h) \
	$(wildcard $(srcdir)/advance/svgalib/*.dif) \
	$(wildcard $(srcdir)/advance/svgalib/*.txt) \
	$(wildcard $(srcdir)/advance/svgalib/*.bat) \
	$(wildcard $(srcdir)/advance/svgalib/*.sh) \
	$(wildcard $(srcdir)/advance/svgalib/*.cfg) \
	$(wildcard $(srcdir)/advance/svgalib/*.html)

SVGALIBDRIVERS_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/drivers/*.c) \
        $(wildcard $(srcdir)/advance/svgalib/drivers/*.h)

SVGALIBCLOCKCHI_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/clockchi/*.c) \
        $(wildcard $(srcdir)/advance/svgalib/clockchi/*.h)

SVGALIBRAMDAC_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/ramdac/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/ramdac/*.h)

SVGALIBSVGADOS_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgados/*.c)

SVGALIBSVGAWIN_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/*.h)

SVGALIBSVGAWINSYS_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/sys/*.h)

SVGALIBSVGAWININSTALL_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/install/makefile) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/install/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/install/*.h)

SVGALIBSVGAWINDRIVER_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/driver/makefile) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/driver/sources) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/driver/*.rc) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/driver/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/svgawin/driver/*.h)

SVGALIBSVGAVDD_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/*.c)

SVGALIBSVGAVDDVDD_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/vdd/makefile) \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/vdd/sources) \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/vdd/*.rc) \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/vdd/*.def) \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/vdd/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/svgavdd/vdd/*.h)

V_SRC = \
	$(wildcard $(srcdir)/advance/v/*.c) \
	$(wildcard $(srcdir)/advance/v/*.h)

S_SRC = \
	$(wildcard $(srcdir)/advance/s/*.c) \
	$(wildcard $(srcdir)/advance/s/*.h)

I_SRC = \
	$(wildcard $(srcdir)/advance/i/*.c) \
	$(wildcard $(srcdir)/advance/i/*.h)

K_SRC = \
	$(wildcard $(srcdir)/advance/k/*.c) \
	$(wildcard $(srcdir)/advance/k/*.h)

J_SRC = \
	$(wildcard $(srcdir)/advance/j/*.c) \
	$(wildcard $(srcdir)/advance/j/*.h)

M_SRC = \
	$(wildcard $(srcdir)/advance/m/*.c) \
	$(wildcard $(srcdir)/advance/m/*.h)

CFG_SRC = \
	$(wildcard $(srcdir)/advance/cfg/*.c)

MENU_SRC = \
	$(wildcard $(srcdir)/advance/menu/*.c) \
	$(wildcard $(srcdir)/advance/menu/*.cc) \
	$(wildcard $(srcdir)/advance/menu/*.h) \
	$(wildcard $(srcdir)/advance/menu/*.dat)

LINE_SRC = \
	$(wildcard $(srcdir)/advance/line/*.cc)

BLUE_SRC = \
	$(wildcard $(srcdir)/advance/blue/*.c)

SRCOSD = \
	$(wildcard $(srcdir)/advance/osd/*.c) \
	$(wildcard $(srcdir)/advance/osd/*.h) \
	$(wildcard $(srcdir)/advance/osd/*.y) \
	$(wildcard $(srcdir)/advance/osd/*.l) \
	$(wildcard $(srcdir)/advance/osd/*.dat)

LINUX_SRC = \
	$(wildcard $(srcdir)/advance/linux/*.c) \
	$(wildcard $(srcdir)/advance/linux/*.h) \
	$(wildcard $(srcdir)/advance/linux/*.dat) \
	$(wildcard $(srcdir)/advance/linux/*.py)

DOS_SRC = \
	$(wildcard $(srcdir)/advance/dos/*.c) \
	$(wildcard $(srcdir)/advance/dos/*.h) \
	$(wildcard $(srcdir)/advance/dos/*.dat)

WINDOWS_SRC = \
	$(wildcard $(srcdir)/advance/windows/*.c) \
	$(wildcard $(srcdir)/advance/windows/*.h)

SDL_SRC = \
	$(wildcard $(srcdir)/advance/sdl/*.c) \
	$(wildcard $(srcdir)/advance/sdl/*.h) \
	$(wildcard $(srcdir)/advance/sdl/*.dat)

D2_SRC = \
	$(wildcard $(srcdir)/advance/d2/*.cc)

EXPAT_SRC = \
	$(wildcard $(srcdir)/advance/expat/*.c) \
	$(wildcard $(srcdir)/advance/expat/*.h) \
	$(wildcard $(srcdir)/advance/expat/COPYING) \
	$(wildcard $(srcdir)/advance/expat/README) \
	$(wildcard $(srcdir)/advance/expat/*.diff)

ZLIB_SRC = \
	$(wildcard $(srcdir)/advance/zlib/*.c) \
	$(wildcard $(srcdir)/advance/zlib/*.h) \
	$(wildcard $(srcdir)/advance/zlib/COPYING) \
	$(wildcard $(srcdir)/advance/zlib/README) \
	$(wildcard $(srcdir)/advance/zlib/*.diff)

CONF_SRC = \
	$(srcdir)/Makefile.in \
	$(srcdir)/Makefile.usr \
	$(srcdir)/root.mak \
	$(srcdir)/configure.ac \
	$(srcdir)/configure \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/config.guess \
	$(srcdir)/config.sub \
	$(srcdir)/install-sh

############################################################################
# Install

pkgdir = $(datadir)/advance
pkgdocdir = $(docdir)/advance

install-dirs:
	-$(INSTALL_PROGRAM_DIR) $(DESTDIR)$(bindir)
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdocdir)
	-$(INSTALL_MAN_DIR) $(DESTDIR)$(mandir)/man1
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/rom
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/sample
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/artwork
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/image
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/image/ti99_4a
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/crc
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/snap
	-$(INSTALL_DATA_DIR) $(DESTDIR)$(pkgdir)/snap/ti99_4a

install-data: $(INSTALL_DATAFILES) $(INSTALL_ROMFILES) $(INSTALL_IMAGEFILES_TI99_4A) $(INSTALL_SAMPLEFILES) $(INSTALL_SNAPFILES) $(INSTALL_SNAPFILES_TI99_4A)
ifdef INSTALL_DATAFILES
	@for i in $(INSTALL_DATAFILES); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir); \
	done
endif
ifdef INSTALL_ROMFILES
	@for i in $(INSTALL_ROMFILES); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/rom"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/rom; \
	done
endif
ifdef INSTALL_IMAGEFILES_TI99_4A
	@for i in $(INSTALL_IMAGEFILES_TI99_4A); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/image/ti99_4a"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/image/ti99_4a; \
	done
endif
ifdef INSTALL_SAMPLEFILES
	@for i in $(INSTALL_SAMPLEFILES); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/sample"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/sample; \
	done
endif
ifdef INSTALL_SNAPFILES
	@for i in $(INSTALL_SNAPFILES); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/snap"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/snap; \
	done
endif
ifdef INSTALL_SNAPFILES_TI99_4A
	@for i in $(INSTALL_SNAPFILES_TI99_4A); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/snap/ti99_4a"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdir)/snap/ti99_4a; \
	done
endif

uninstall-data:
ifdef INSTALL_DATAFILES
	@for i in $(notdir $(INSTALL_DATAFILES)); do \
		rm -f $(DESTDIR)$(pkgdir)/$$i; \
	done
endif
ifdef INSTALL_ROMFILES
	@for i in $(notdir $(INSTALL_ROMFILES)); do \
		rm -f $(DESTDIR)$(pkgdir)/rom/$$i; \
	done
endif
ifdef INSTALL_SAMPLEFILES
	@for i in $(notdir $(INSTALL_SAMPLEFILES)); do \
		rm -f $(DESTDIR)$(pkgdir)/sample/$$i; \
	done
endif
ifdef INSTALL_SNAPFILES
	@for i in $(notdir $(INSTALL_SNAPFILES)); do \
		rm -f $(DESTDIR)$(pkgdir)/snap/$$i; \
	done
endif

install-bin: $(INSTALL_BINFILES)
	@for i in $(INSTALL_BINFILES); do \
		echo "$(INSTALL_PROGRAM) $$i $(DESTDIR)$(bindir)"; \
		$(INSTALL_PROGRAM) $$i $(DESTDIR)$(bindir); \
	done

uninstall-bin:
	@for i in $(notdir $(INSTALL_BINFILES)); do \
		rm -f $(DESTDIR)$(bindir)/$$i; \
	done

install-doc: $(INSTALL_DOCFILES)
ifdef INSTALL_DOCFILES
	@for i in $(INSTALL_DOCFILES); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdocdir)"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(pkgdocdir); \
	done
endif

uninstall-doc:
ifdef INSTALL_DOCFILES
	@for i in $(notdir $(INSTALL_DOCFILES)); do \
		rm -f $(DESTDIR)$(pkgdocdir)/$$i; \
	done
endif

install-man: $(INSTALL_MANFILES)
ifdef INSTALL_MANFILES
	@for i in $(INSTALL_MANFILES); do \
		echo "$(INSTALL_DATA) $$i $(DESTDIR)$(mandir)/man1"; \
		$(INSTALL_DATA) $$i $(DESTDIR)$(mandir)/man1; \
	done
endif

uninstall-man:
ifdef INSTALL_MANFILES
	@for i in $(notdir $(INSTALL_MANFILES)); do \
		rm -f $(DESTDIR)$(mandir)/man1/$$i; \
	done
endif

uninstall-dirs:
	-rmdir $(DESTDIR)$(pkgdir)/rom
	-rmdir $(DESTDIR)$(pkgdir)/sample
	-rmdir $(DESTDIR)$(pkgdir)/artwork
	-rmdir $(DESTDIR)$(pkgdir)/image/ti99_4a
	-rmdir $(DESTDIR)$(pkgdir)/image
	-rmdir $(DESTDIR)$(pkgdir)/crc
	-rmdir $(DESTDIR)$(pkgdir)/snap/ti99_4a
	-rmdir $(DESTDIR)$(pkgdir)/snap
	-rmdir $(DESTDIR)$(pkgdir)
	-rmdir $(DESTDIR)$(pkgdocdir)

install: install-dirs install-bin install-data install-doc install-man

uninstall: uninstall-bin uninstall-data uninstall-doc uninstall-man uninstall-dirs

############################################################################
# Common build

# Resource include dir
RCFLAGS += --include-dir advance/lib

############################################################################
# Whole targets

# To generate this list of defines, comment the AC_CONFIG_HEADERS command
# in configure.ac, run autoconf and ./configure and grab it from the Makefile
DEFS_LINUX = -DSTDC_HEADERS=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_DIRENT_H=1 -DTIME_WITH_SYS_TIME=1 -DGWINSZ_IN_SYS_IOCTL=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SCHED_H=1 -DHAVE_NETDB_H=1 -DHAVE_TERMIOS_H=1 -DHAVE_EXECINFO_H=1 -DHAVE_SYS_UTSNAME_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_SYS_MMAN_H=1 -DHAVE_SYS_IO_H=1 -DHAVE_SYS_KD_H=1 -DHAVE_SYS_VT_H=1 -DHAVE_NETINET_IN_H=1 -DHAVE_UCONTEXT_H=1 -Drestrict=__restrict -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_MMAP=1 -DHAVE_SYS_SELECT_H=1 -DHAVE_SYS_SOCKET_H=1 -DSELECT_TYPE_ARG1=int -DSELECT_TYPE_ARG234=\(fd_set\ \*\) -DSELECT_TYPE_ARG5=\(struct\ timeval\ \*\) -DHAVE_VPRINTF=1 -DHAVE_STRCASECMP=1 -DHAVE_STRERROR=1 -DHAVE_UNAME=1 -DHAVE_SYSCONF=1 -DHAVE_BACKTRACE=1 -DHAVE_BACKTRACE_SYMBOLS=1 -DHAVE_FLOCKFILE=1 -DHAVE_FUNLOCKFILE=1 -DHAVE_FREAD_UNLOCKED=1 -DHAVE_FWRITE_UNLOCKED=1 -DHAVE_FGETC_UNLOCKED=1 -DHAVE_FEOF_UNLOCKED=1 -DHAVE_IOPL=1 -DHAVE_MPROTECT=1 -DHAVE_INOUT=1 -DHAVE_SLANG_SLANG_H=1

# Optimized
WHOLE_CFLAGS_OPT = -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-strict-overflow -fno-merge-constants -Wall -Wno-sign-compare -Wno-unused
WHOLE_CFLAGS_EMU =
WHOLE_LDFLAGS = -s

ARCH_X86 = CONF_MAP=yes CONF_ARCH=x86 CONF_CFLAGS_OPT="-march=i686 $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)"

MANUAL=-f Makefile.usr

whole:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos

wholedist:
	$(MAKE) $(MANUAL) dist
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distbin

wholemenu:
	$(MAKE) $(MANUAL) distmenu
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distmenubin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distmenubin

wholelinux:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=linux CONF_DEFS="$(DEFS_LINUX)" distbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=linux CONF_DEFS="$(DEFS_LINUX)" distmenubin

wholecab:
	$(MAKE) $(MANUAL) distcab
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distcabbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distcabbin

############################################################################
# deb

DEB_REVISION = 1

DEB_BINFILES = \
	$(MAME_INSTALL_BINFILES) \
	$(MESS_INSTALL_BINFILES) \
	$(MENU_INSTALL_BINFILES) \
	$(CFG_INSTALL_BINFILES) \
	$(V_INSTALL_BINFILES) \
	$(S_INSTALL_BINFILES) \
	$(K_INSTALL_BINFILES) \
	$(J_INSTALL_BINFILES) \
	$(M_INSTALL_BINFILES) \
	$(BLUE_INSTALL_BINFILES)
DEB_MANFILES = \
	$(MAME_INSTALL_MANFILES) \
	$(MESS_INSTALL_MANFILES) \
	$(MENU_INSTALL_MANFILES) \
	$(CFG_INSTALL_MANFILES) \
	$(V_INSTALL_MANFILES) \
	$(S_INSTALL_MANFILES) \
	$(K_INSTALL_MANFILES) \
	$(J_INSTALL_MANFILES) \
	$(M_INSTALL_MANFILES) \
	$(BLUE_INSTALL_MANFILES)
DEB_DATAFILES = \
	$(MAME_INSTALL_DATAFILES) \
	$(MESS_INSTALL_DATAFILES)
DEB_ROMFILES = $(MAME_INSTALL_ROMFILES) $(MESS_INSTALL_ROMFILES)
DEB_IMAGEFILES_TI99_4A = $(MESS_INSTALL_IMAGEFILES_TI99_4A)
DEB_SAMPLEFILES = $(MAME_INSTALL_SAMPLEFILES)
DEB_SNAPFILES = $(MAME_INSTALL_SNAPFILES) $(MESS_INSTALL_SNAPFILES)
DEB_SNAPFILES_TI99_4A = $(MESS_INSTALL_SNAPFILES_TI99_4A)
DEB_DOCFILES = $(INSTALL_DOCFILES)
DEB_MACHINE = $(subst armv7l,armhf,$(subst i686,i386,$(subst x86_64,amd64,$(shell uname -m))))
DEB_DIST_FILE_BIN = advancemame_$(VERSION)-$(DEB_REVISION)_$(DEB_MACHINE)
DEB_DIST_DIR_BIN = $(DEB_DIST_FILE_BIN)

deb:
	$(MAKE)
	rm -rf $(DEB_DIST_DIR_BIN)
	mkdir $(DEB_DIST_DIR_BIN)
	mkdir $(DEB_DIST_DIR_BIN)/DEBIAN
	sed -e s/VERSION/$(VERSION)/ -e s/MACHINE/$(DEB_MACHINE)/ -e s/REVISION/$(DEB_REVISION)/ $(srcdir)/support/debian.$(DEB_MACHINE) > $(DEB_DIST_DIR_BIN)/DEBIAN/control
	mkdir $(DEB_DIST_DIR_BIN)/usr
	mkdir $(DEB_DIST_DIR_BIN)/usr/local
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/bin
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/doc
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/doc/advance
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance/image
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance/image/ti99_4a
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance/rom
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance/sample
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance/snap
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/share/advance/snap/ti99_4a
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/man
	mkdir $(DEB_DIST_DIR_BIN)/usr/local/man/man1
	cp $(DEB_BINFILES) $(DEB_DIST_DIR_BIN)/usr/local/bin
	cp $(DEB_DOCFILES) $(DEB_DIST_DIR_BIN)/usr/local/share/doc/advance
	cp $(DEB_DATAFILES) $(DEB_DIST_DIR_BIN)/usr/local/share/advance
	cp $(DEB_ROMFILES) $(DEB_DIST_DIR_BIN)/usr/local/share/advance/rom
	cp $(DEB_IMAGEFILES_TI99_4A) $(DEB_DIST_DIR_BIN)/usr/local/share/advance/image/ti99_4a
	cp $(DEB_SAMPLEFILES) $(DEB_DIST_DIR_BIN)/usr/local/share/advance/sample
	cp $(DEB_SNAPFILES) $(DEB_DIST_DIR_BIN)/usr/local/share/advance/snap
	cp $(DEB_SNAPFILES_TI99_4A) $(DEB_DIST_DIR_BIN)/usr/local/share/advance/snap/ti99_4a
	cp $(DEB_MANFILES) $(DEB_DIST_DIR_BIN)/usr/local/man/man1
	find $(DEB_DIST_DIR_BIN)
	dpkg-deb --build $(DEB_DIST_DIR_BIN)
	rm -rf $(DEB_DIST_DIR_BIN)

#############################################################################
# Development targets

devflags: obj
	$(ECHO) CC=$(CC)
	$(ECHO) CFLAGS=$(CFLAGS)
	$(ECHO) CXX=$(CXX)
	$(ECHO) CXXFLAGS=$(CXXFLAGS)
	$(ECHO) LD=$(LD)
	$(ECHO) LDFLAGS=$(LDFLAGS)
	$(ECHO) LIBS=$(LIBS)
	$(ECHO) CC_FOR_BUILD=$(CC_FOR_BUILD)
	$(ECHO) CFLAGS_FOR_BUILD=$(CFLAGS_FOR_BUILD)
	$(ECHO) LD_FOR_BUILD=$(LD_FOR_BUILD)
	$(ECHO) SDLCFLAGS=$(SDLCFLAGS)
	$(ECHO) SDLLIBS=$(SDLLIBS)
	$(ECHO) FREETYPECFLAGS=$(FREETYPECFLAGS)
	$(ECHO) FREETYPELIBS=$(FREETYPELIBS)
	$(ECHO) EMUCFLAGS=$(EMUCFLAGS)
	$(ECHO) EMULDFLAGS=$(EMULDFLAGS)
	$(ECHO) ADVANCECFLAGS=$(ADVANCECFLAGS)
	$(ECHO) ADVANCELDFLAGS=$(ADVANCELDFLAGS)
	$(ECHO) ADVANCELIBS=$(ADVANCELIBS)
	$(ECHO) "int test(void) { return 0; }" > obj/flags.c
	$(CC) $(CFLAGS) obj/flags.c -S -fverbose-asm -o obj/flags.S

devosdep:
	rgrep -r MSDOS $(srcdir)/advance
	rgrep -r WIN32 $(srcdir)/advance

devdef:
	rgrep -r "^#if" $(srcdir)/advance | grep -v -E "_H$$|USE|__cplusplus|expat|zlib|svgalib|windows|NDEBUG|MESS|linux/.*event|advmame.dif|advmess.dif|/y_tab|/lexyy|/tsr|/card|/dos"

devtags:
	cd advance && ctags -R

