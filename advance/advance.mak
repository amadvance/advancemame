############################################################################
# Common version

ifeq ($(CONF_EMU),mess)
EMUVERSION = 0.65.0.0
else
ifeq ($(CONF_EMU),pac)
EMUVERSION = 0.58.x
else
EMUVERSION = 0.66.0
endif
endif
MENUVERSION = 2.2.5
CABVERSION = 1.1.4

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
LINEOBJ = obj/line/$(BINARYDIR_BUILD)
D2OBJ = obj/d2/$(BINARYDIR_BUILD)
DOCOBJ = obj/doc

############################################################################
# Common targets

ifdef ALL
all_override: $(ALL)
endif

ifneq ($(wildcard $(EMUSRC)),)
INSTALL_BINFILES += $(OBJ)/$(EMUNAME)$(EXE)
INSTALL_DATAFILES += $(srcdir)/support/safequit.dat
INSTALL_MANFILES += $(DOCOBJ)/advmame.1
ifeq ($(CONF_EMU),mess)
INSTALL_MANFILES += $(srcdir)/support/advmess.1
endif
ifeq ($(CONF_EMU),pac)
INSTALL_MANFILES += $(srcdir)/support/advpac.1
endif
endif
ifneq ($(wildcard $(srcdir)/advance/menu.mak),)
INSTALL_BINFILES += $(MENUOBJ)/advmenu$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advmenu.1
endif
ifneq ($(wildcard $(srcdir)/advance/v.mak),)
INSTALL_BINFILES += $(VOBJ)/advv$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advv.1
endif
ifneq ($(wildcard $(srcdir)/advance/cfg.mak),)
INSTALL_BINFILES += $(CFGOBJ)/advcfg$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advcfg.1
endif
ifneq ($(CONF_HOST),windows)
ifneq ($(wildcard $(srcdir)/advance/s.mak),)
INSTALL_BINFILES += $(SOBJ)/advs$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advs.1
endif
ifneq ($(wildcard $(srcdir)/advance/k.mak),)
INSTALL_BINFILES += $(KOBJ)/advk$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advk.1
endif
ifneq ($(wildcard $(srcdir)/advance/j.mak),)
INSTALL_BINFILES += $(JOBJ)/advj$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advj.1
endif
ifneq ($(wildcard $(srcdir)/advance/m.mak),)
INSTALL_BINFILES += $(MOBJ)/advm$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advm.1
endif
endif

INSTALL_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.txt,$(wildcard $(srcdir)/doc/*.d)))
INSTALL_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.html,$(wildcard $(srcdir)/doc/*.d)))
WEB_DOCFILES += $(subst $(srcdir)/doc/,$(DOCOBJ)/,$(subst .d,.hh,$(wildcard $(srcdir)/doc/*.d)))

all: $(INSTALL_BINFILES) $(INSTALL_DOCFILES) $(INSTALL_MANFILES) $(INSTALL_DATAFILES)
emu: $(OBJ)/$(EMUNAME)$(EXE)
menu: $(MENUOBJ)/advmenu$(EXE)
cfg: $(CFGOBJ)/advcfg$(EXE)
v: $(VOBJ)/advv$(EXE)
s: $(SOBJ)/advs$(EXE)
k: $(KOBJ)/advk$(EXE)
i: $(IOBJ)/advi$(EXE)
j: $(JOBJ)/advj$(EXE)
m: $(MOBJ)/advm$(EXE)
line: $(LINEOBJ)/advline$(EXE_BUILD)
d2: $(D2OBJ)/advd2$(EXE_BUILD)
web: $(WEB_DOCFILES)

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
	$(wildcard $(srcdir)/advance/lib/*.ico) \
	$(wildcard $(srcdir)/advance/lib/*.rc) \
	$(wildcard $(srcdir)/advance/lib/*.dat) \
	$(wildcard $(srcdir)/advance/lib/*.cfg) \
	$(wildcard $(srcdir)/advance/lib/*.html)

BLIT_SRC = \
	$(wildcard $(srcdir)/advance/blit/*.c) \
	$(wildcard $(srcdir)/advance/blit/*.h)

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
	$(wildcard $(srcdir)/advance/osd/*.l)

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

CONF_SRC = \
	$(srcdir)/Makefile.in \
	$(srcdir)/config.guess \
	$(srcdir)/config.sub \
	$(srcdir)/configure \
	$(srcdir)/configure.ac \
	$(srcdir)/configure.msdos \
	$(srcdir)/configure.windows \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/install-sh \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/missing

CONF_BIN = \
	$(srcdir)/support/confbin/INSTALL \
	$(srcdir)/support/confbin/Makefile.am \
	$(srcdir)/support/confbin/Makefile.in \
	$(srcdir)/support/confbin/aclocal.m4 \
	$(srcdir)/support/confbin/autogen.sh \
	$(srcdir)/support/confbin/configure \
	$(srcdir)/support/confbin/configure.ac \
	$(srcdir)/support/confbin/install-sh \
	$(srcdir)/support/confbin/missing \
	$(srcdir)/support/confbin/mkinstalldirs

############################################################################
# Common install

installdirs:
	-$(INSTALL_DATA_DIR) $(DATADIR)
	-$(INSTALL_DATA_DIR) $(DATADIR)/doc
	-$(INSTALL_MAN_DIR) $(PREFIX)/man/man1
ifneq ($(wildcard $(EMUSRC)),)
	-$(INSTALL_DATA_DIR) $(DATADIR)/rom
	-$(INSTALL_DATA_DIR) $(DATADIR)/sample
	-$(INSTALL_DATA_DIR) $(DATADIR)/artwork
	-$(INSTALL_DATA_DIR) $(DATADIR)/image
endif

install-data: $(INSTALL_DATAFILES)
ifdef INSTALL_DATAFILES
	@for i in $(INSTALL_DATAFILES); do \
		$(INSTALL_DATA) $$i $(DATADIR); \
	done
endif

uninstall-data:
ifdef INSTALL_DATAFILES
	@for i in $(INSTALL_DATAFILES); do \
		rm -f $(DATADIR)/$$i; \
	done
endif

install-bin: $(INSTALL_BINFILES)
	@for i in $(INSTALL_BINFILES); do \
		$(INSTALL_PROGRAM) $$i $(PREFIX)/bin; \
	done

uninstall-bin:
	@for i in $(INSTALL_BINFILES); do \
		rm -f $(PREFIX)/bin/$$i; \
	done

install-doc: $(INSTALL_DOCFILES)
ifdef INSTALL_DOCFILES
	@for i in $(INSTALL_DOCFILES); do \
		$(INSTALL_DATA) $$i $(DATADIR)/doc; \
	done
endif

uninstall-doc:
ifdef INSTALL_DOCFILES
	@for i in $(INSTALL_DOCFILES); do \
		rm -f $(DATADIR)/doc/$$i; \
	done
endif

install-man: $(INSTALL_MANFILES)
ifdef INSTALL_MANFILES
	@for i in $(INSTALL_MANFILES); do \
		$(INSTALL_DATA) $$i $(PREFIX)/man/man1; \
	done
endif

uninstall-man:
ifdef INSTALL_MANFILES
	@for i in $(INSTALL_MANFILES); do \
		rm -f $(PREFIX)/man/man1/$$i; \
	done
endif

install: installdirs install-bin install-data install-doc install-man

install-strip: install

uninstall: uninstall-bin uninstall-data uninstall-doc uninstall-man

############################################################################
# Common build

# Resource include dir
RCFLAGS += --include-dir advance/lib

############################################################################
# Special Rules

# Debug
#WHOLE_CFLAGS_OPT = -O3 -Wall -Wno-sign-compare -Wno-unused
#WHOLE_CFLAGS_EMU = -fomit-frame-pointer
#WHOLE_LDFLAGS = -rdynamic

# Optimized
WHOLE_CFLAGS_OPT = -O3 -Wall -Wno-sign-compare -Wno-unused -fomit-frame-pointer
WHOLE_CFLAGS_EMU =
WHOLE_LDFLAGS = -s

ARCH_I386 = CONF_ARCH=i386 CONF_CFLAGS_OPT="-march=i386 $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)"
ARCH_PENTIUM = CONF_ARCH=pentium CONF_CFLAGS_OPT="-march=pentium $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)"
ARCH_PENTIUM_BLEND = CONF_ARCH=pentium CONF_CFLAGS_OPT="-march=pentium -mcpu=pentium2 $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)"
ARCH_PENTIUM_BLEND_GCCOLD = CONF_ARCH=pentium CONF_CFLAGS_OPT="-march=i586 -mcpu=i686 $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)"

mame:
	$(MAKE) CONF=no CONF_EMU=mame emu

neomame:
	$(MAKE) CONF=no CONF_EMU=neomame emu

cpmame:
	$(MAKE) CONF=no CONF_EMU=cpmame emu

messmame:
	$(MAKE) CONF=no CONF_EMU=mess emu

pacmame:
	$(MAKE) CONF=no CONF_EMU=pac emu

wholemame:
	$(MAKE) CONF=no dist
	$(MAKE) $(ARCH_PENTIUM_BLEND_GCCOLD) CONF=no CONF_HOST=windows distbin
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=unix distbin
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=dos distbin

WHOLECD_FLAGS = \
		CONF_ARCH=cd CONF_CFLAGS_OPT="-march=pentium -mcpu=pentium2 $(WHOLE_CFLAGS_OPT)" CONF_CFLAGS_EMU="$(WHOLE_CFLAGS_EMU)" CONF_LDFLAGS="$(WHOLE_LDFLAGS)" \
		CONF=no CONF_HOST=unix \
		CONF_LIB_SVGALIB=yes CONF_LIB_ALSA=yes CONF_LIB_KRAW=yes \
		CONF_LIB_FB=yes CONF_LIB_OSS=no CONF_LIB_PTHREAD=yes \
		CONF_LIB_SDL=no \
		DATADIR=/advance

wholecd:
	$(MAKE) $(WHOLECD_FLAGS) distbin
	$(MAKE) $(WHOLECD_FLAGS) distmenubin
	$(MAKE) $(WHOLECD_FLAGS) CONF_EMU=mess distbin

wholemess:
	$(MAKE) CONF=no CONF_EMU=mess dist
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=unix CONF_EMU=mess distbin
	$(MAKE) $(ARCH_PENTIUM_BLEND_GCCOLD) CONF=no CONF_HOST=windows CONF_EMU=mess distbin
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=dos CONF_EMU=mess distbin

wholepac:
	$(MAKE) CONF=no CONF_EMU=pac dist
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=dos CONF_EMU=pac distbin

wholemenu:
	$(MAKE) CONF=no distmenu
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=unix distmenubin
	$(MAKE) $(ARCH_PENTIUM_BLEND_GCCOLD) CONF=no CONF_HOST=windows distmenubin
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=dos distmenubin

wholecab:
	$(MAKE) CONF=no distcab
	$(MAKE) $(ARCH_I386) CONF=no CONF_HOST=dos distcabbin
	$(MAKE) $(ARCH_I386) CONF=no CONF_HOST=windows distcabbin

wholedist:
	$(MAKE) CONF=no dist
	$(MAKE) CONF=no CONF_EMU=mess dist
	$(MAKE) CONF=no distmenu

wholewin:
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=windows distbin
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=windows CONF_EMU=mess distbin
	$(MAKE) $(ARCH_PENTIUM_BLEND) CONF=no CONF_HOST=windows distmenubin

distmess:
	$(MAKE) CONF=no CONF_EMU=mess dist

distmessbin:
	$(MAKE) CONF=no CONF_EMU=mess distbin

distpac:
	$(MAKE) CONF=no CONF_EMU=pac dist

distpacbin:
	$(MAKE) CONF=no CONF_EMU=pac distbin

distmame:
	$(MAKE) CONF=no CONF_EMU=mame dist

distmamebin:
	$(MAKE) CONF=no CONF_EMU=mame distbin


