############################################################################
# Common dir

OBJ = obj/$(CONF_EMU)/$(BINARYDIR)
MENUOBJ = obj/menu/$(BINARYDIR)
MOBJ = obj/m/$(BINARYDIR)
JOBJ = obj/j/$(BINARYDIR)
KOBJ = obj/k/$(BINARYDIR)
IOBJ = obj/i/$(BINARYDIR)
VOBJ = obj/v/$(BINARYDIR)
SOBJ = obj/s/$(BINARYDIR)
CFGOBJ = obj/cfg/$(BINARYDIR)
LINEOBJ = obj/line/$(BINARYBUILDDIR)
D2OBJ = obj/d2/$(BINARYBUILDDIR)
DOCOBJ = $(srcdir)/doc

############################################################################
# Common targets

ifdef ADV_ALL
all_override: $(ADV_ALL)
endif

ifneq ($(wildcard $(EMUSRC)),)
INSTALL_DIRS += $(OBJ)
INSTALL_BINFILES += $(OBJ)/$(EMUNAME)$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advmame.1
INSTALL_MANFILES += $(DOCOBJ)/advdev.1
ifeq ($(CONF_EMU),mame)
INSTALL_DATAFILES += $(srcdir)/support/event.dat
INSTALL_DATAFILES += $(srcdir)/support/history.dat
INSTALL_DATAFILES += $(srcdir)/support/hiscore.dat
INSTALL_ROMFILES += $(srcdir)/contrib/mame/free/rom/gridlee.zip
INSTALL_ROMFILES += $(srcdir)/contrib/mame/free/rom/polyplay.zip
INSTALL_ROMFILES += $(srcdir)/contrib/mame/free/rom/robby.zip
INSTALL_SAMPLEFILES += $(srcdir)/contrib/mame/free/rom/gridlee.zip
INSTALL_SNAPFILES += $(srcdir)/contrib/mame/free/snap/gridlee.zip
INSTALL_SNAPFILES += $(srcdir)/contrib/mame/free/snap/polyplay.zip
INSTALL_SNAPFILES += $(srcdir)/contrib/mame/free/snap/robby.zip
endif
ifeq ($(CONF_EMU),mess)
INSTALL_DATAFILES += $(srcdir)/support/sysinfo.dat
INSTALL_MANFILES += $(srcdir)/support/advmess.1
endif
endif
ifneq ($(wildcard $(srcdir)/advance/menu.mak),)
INSTALL_DIRS += $(MENUOBJ)
INSTALL_BINFILES += $(MENUOBJ)/advmenu$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advmenu.1
endif
ifeq ($(CONF_LIB_DIRECT),yes)
ifneq ($(wildcard $(srcdir)/advance/cfg.mak),)
INSTALL_DIRS += $(CFGOBJ)
INSTALL_BINFILES += $(CFGOBJ)/advcfg$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advcfg.1
endif
ifneq ($(wildcard $(srcdir)/advance/v.mak),)
INSTALL_DIRS += $(VOBJ)
INSTALL_BINFILES += $(VOBJ)/advv$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advv.1
endif
endif
ifneq ($(CONF_SYSTEM),windows)
ifneq ($(wildcard $(srcdir)/advance/s.mak),)
INSTALL_DIRS += $(SOBJ)
INSTALL_BINFILES += $(SOBJ)/advs$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advs.1
endif
ifneq ($(wildcard $(srcdir)/advance/k.mak),)
INSTALL_DIRS += $(KOBJ)
INSTALL_BINFILES += $(KOBJ)/advk$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advk.1
endif
ifneq ($(wildcard $(srcdir)/advance/j.mak),)
INSTALL_DIRS += $(JOBJ)
INSTALL_BINFILES += $(JOBJ)/advj$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advj.1
endif
ifneq ($(wildcard $(srcdir)/advance/m.mak),)
INSTALL_DIRS += $(MOBJ)
INSTALL_BINFILES += $(MOBJ)/advm$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advm.1
endif
endif

INSTALL_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.txt,$(wildcard $(srcdir)/doc/*.d)))
INSTALL_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.html,$(wildcard $(srcdir)/doc/*.d)))
WEB_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.hh,$(wildcard $(srcdir)/doc/*.d)))

all: $(INSTALL_DIRS) $(INSTALL_BINFILES) $(INSTALL_DOCFILES) $(INSTALL_MANFILES) $(INSTALL_DATAFILES) $(INSTALL_ROMFILES) $(INSTALL_SAMPLEFILES) $(INSTALL_SNAPFILES)
emu: $(OBJ) $(OBJ)/$(EMUNAME)$(EXE)
menu: $(MENUOBJ) $(MENUOBJ)/advmenu$(EXE)
cfg: $(CFGOBJ) $(CFGOBJ)/advcfg$(EXE)
v: $(VOBJ) $(VOBJ)/advv$(EXE)
s: $(SOBJ) $(SOBJ)/advs$(EXE)
k: $(KOBJ) $(KOBJ)/advk$(EXE)
i: $(IOBJ) $(IOBJ)/advi$(EXE)
j: $(JOBJ) $(JOBJ)/advj$(EXE)
m: $(MOBJ) $(MOBJ)/advm$(EXE)
line: $(LINEOBJ) $(LINEOBJ)/advline$(EXE_FOR_BUILD)
d2: $(D2OBJ) $(D2OBJ)/advd2$(EXE_FOR_BUILD)
web: $(WEB_DOCFILES)

# Ensure that the doc target is always created also if a doc directory exists
.PHONY: doc

doc: $(INSTALL_DOCFILES)

############################################################################
# Common SRC

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

LINE_SRC = \
	$(wildcard $(srcdir)/advance/line/*.cc)

SRCOSD = \
	$(wildcard $(srcdir)/advance/osd/*.c) \
	$(wildcard $(srcdir)/advance/osd/*.h) \
	$(wildcard $(srcdir)/advance/osd/*.y) \
	$(wildcard $(srcdir)/advance/osd/*.l) \
	$(wildcard $(srcdir)/advance/osd/*.dat)

LINUX_SRC = \
	$(wildcard $(srcdir)/advance/linux/*.c) \
	$(wildcard $(srcdir)/advance/linux/*.h)

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
# Common install

pkgdir = $(datadir)/advance
pkgdocdir = $(docdir)/advance

install-dirs:
	-$(INSTALL_PROGRAM_DIR) $(bindir)
	-$(INSTALL_DATA_DIR) $(pkgdir)
	-$(INSTALL_DATA_DIR) $(pkgdocdir)
	-$(INSTALL_MAN_DIR) $(mandir)/man1
ifneq ($(wildcard $(EMUSRC)),)
	-$(INSTALL_DATA_DIR) $(pkgdir)/rom
	-$(INSTALL_DATA_DIR) $(pkgdir)/sample
	-$(INSTALL_DATA_DIR) $(pkgdir)/artwork
	-$(INSTALL_DATA_DIR) $(pkgdir)/image
	-$(INSTALL_DATA_DIR) $(pkgdir)/crc
	-$(INSTALL_DATA_DIR) $(pkgdir)/snap
endif

install-data: $(INSTALL_DATAFILES) $(INSTALL_ROMFILES) $(INSTALL_SAMPLEFILES) $(INSTALL_SNAPFILES)
ifdef INSTALL_DATAFILES
	@for i in $(INSTALL_DATAFILES); do \
		echo "$(INSTALL_DATA) $$i $(pkgdir)"; \
		$(INSTALL_DATA) $$i $(pkgdir); \
	done
endif
ifdef INSTALL_ROMFILES
	@for i in $(INSTALL_ROMFILES); do \
		echo "$(INSTALL_DATA) $$i $(pkgdir)/rom"; \
		$(INSTALL_DATA) $$i $(pkgdir)/rom; \
	done
endif
ifdef INSTALL_SAMPLEFILES
	@for i in $(INSTALL_SAMPLEFILES); do \
		echo "$(INSTALL_DATA) $$i $(pkgdir)/sample"; \
		$(INSTALL_DATA) $$i $(pkgdir)/sample; \
	done
endif
ifdef INSTALL_SNAPFILES
	@for i in $(INSTALL_SNAPFILES); do \
		echo "$(INSTALL_DATA) $$i $(pkgdir)/snap"; \
		$(INSTALL_DATA) $$i $(pkgdir)/snap; \
	done
endif

uninstall-data:
ifdef INSTALL_DATAFILES
	@for i in $(notdir $(INSTALL_DATAFILES)); do \
		rm -f $(pkgdir)/$$i; \
	done
endif
ifdef INSTALL_ROMFILES
	@for i in $(notdir $(INSTALL_ROMFILES)); do \
		rm -f $(pkgdir)/rom/$$i; \
	done
endif
ifdef INSTALL_SAMPLEFILES
	@for i in $(notdir $(INSTALL_SAMPLEFILES)); do \
		rm -f $(pkgdir)/sample/$$i; \
	done
endif
ifdef INSTALL_SNAPFILES
	@for i in $(notdir $(INSTALL_SNAPFILES)); do \
		rm -f $(pkgdir)/snap/$$i; \
	done
endif

install-bin: $(INSTALL_BINFILES)
	@for i in $(INSTALL_BINFILES); do \
		echo "$(INSTALL_PROGRAM) $$i $(bindir)"; \
		$(INSTALL_PROGRAM) $$i $(bindir); \
	done

uninstall-bin:
	@for i in $(notdir $(INSTALL_BINFILES)); do \
		rm -f $(bindir)/$$i; \
	done

install-doc: $(INSTALL_DOCFILES)
ifdef INSTALL_DOCFILES
	@for i in $(INSTALL_DOCFILES); do \
		echo "$(INSTALL_DATA) $$i $(pkgdocdir)"; \
		$(INSTALL_DATA) $$i $(pkgdocdir); \
	done
endif

uninstall-doc:
ifdef INSTALL_DOCFILES
	@for i in $(notdir $(INSTALL_DOCFILES)); do \
		rm -f $(pkgdocdir)/$$i; \
	done
endif

install-man: $(INSTALL_MANFILES)
ifdef INSTALL_MANFILES
	@for i in $(INSTALL_MANFILES); do \
		echo "$(INSTALL_DATA) $$i $(mandir)/man1"; \
		$(INSTALL_DATA) $$i $(mandir)/man1; \
	done
endif

uninstall-man:
ifdef INSTALL_MANFILES
	@for i in $(notdir $(INSTALL_MANFILES)); do \
		rm -f $(mandir)/man1/$$i; \
	done
endif

uninstall-dirs:
ifneq ($(wildcard $(EMUSRC)),)
	-rmdir $(pkgdir)/rom
	-rmdir $(pkgdir)/sample
	-rmdir $(pkgdir)/artwork
	-rmdir $(pkgdir)/image
	-rmdir $(pkgdir)/crc
	-rmdir $(pkgdir)/snap
	-rmdir $(pkgdir)
	-rmdir $(pkgdocdir)
endif

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
WHOLE_CFLAGS_OPT = -O2 -fomit-frame-pointer -fno-strength-reduce -fno-strict-aliasing -fno-merge-constants -Wall -Wno-sign-compare -Wno-unused
WHOLE_CFLAGS_EMU =
WHOLE_LDFLAGS = -s

ARCH_X86 = CONF_MAP=yes CONF_ARCH=x86 CONF_CFLAGS_OPT="-march=i686 $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)"

MANUAL=-f Makefile.usr

WHOLECD_FLAGS = \
	ADV_DATADIR="/root" ADV_SYSCONFDIR="/etc" \
	CONF_ARCH=cd CONF_CFLAGS_OPT="-march=i686 $(WHOLE_CFLAGS_OPT) -fno-merge-constants" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)" \
	CONF_DEFS="$(DEFS_LINUX)" \
	CONF_HOST=linux \
	CONF_LIB_KEVENT=yes CONF_LIB_JEVENT=yes CONF_LIB_MEVENT=yes \
	CONF_LIB_KRAW=yes CONF_LIB_JRAW=yes CONF_LIB_MRAW=yes \
	CONF_LIB_SVGALIB=no CONF_LIB_ALSA=yes CONF_LIB_FB=yes \
	CONF_LIB_SLANG=yes CONF_LIB_NCURSES=no \
	CONF_LIB_OSS=no CONF_LIB_PTHREAD=no CONF_LIB_SDL=no

wholemame: mamedif
	$(MAKE) $(MANUAL) dist
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distbin

dosmame:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distbin

winmame:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distbin

wholecd:
	$(MAKE) $(MANUAL) $(WHOLECD_FLAGS) distbin
	$(MAKE) $(MANUAL) $(WHOLECD_FLAGS) distmenubin
	$(MAKE) $(MANUAL) $(WHOLECD_FLAGS) CONF_EMU=mess distbin

wholemess: messdif
	$(MAKE) $(MANUAL) CONF_EMU=mess dist
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows CONF_EMU=mess distbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos CONF_EMU=mess distbin

dosmess:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos CONF_EMU=mess distbin

winmess:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows CONF_EMU=mess distbin

wholemenu:
	$(MAKE) $(MANUAL) distmenu
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distmenubin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distmenubin

dosmenu:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distmenubin

winmenu:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distmenubin

wholelinux:
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=linux CONF_DEFS="$(DEFS_LINUX)" distbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=linux CONF_EMU=mess CONF_DEFS="$(DEFS_LINUX)" distbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=linux CONF_DEFS="$(DEFS_LINUX)" distmenubin

wholecab:
	$(MAKE) $(MANUAL) distcab
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=dos distcabbin
	$(MAKE) $(MANUAL) $(ARCH_X86) CONF_HOST=windows distcabbin

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

