############################################################################
# Common version

ifeq ($(CONF_EMU),mess)
EMUVERSION = 0.56.x
else
ifeq ($(CONF_EMU),pac)
EMUVERSION = 0.58.x
else
EMUVERSION = 0.61.2
endif
endif
MENUVERSION = 2.1.0
CABVERSION = 0.11.3

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

ifeq ($(CONF_EMU),mess)
# from MESS 0.56
INSTALL_IMAGEDIRS += \
	a2600 a5200 a7800 lynx lynxa lynx2 nes nespal famicom gameboy snes gamegear \
	sms genesis saturn astrocde studio2 channelf coleco colecoa pce arcadia vcg \
	vectrex raaspec intv intvsrs advision sfzch svision atom atomeb bbca bbcb \
	bbcb1770 bbcbp bbcbp128 z88 cpc464 cpc664 cpc6128 kccomp pcw8256 pcw8512 \
	pcw9256 pcw9512 pcw10 pcw16 nc100 nc100a nc200 apple1 apple2c apple2c0 \
	apple2cp apple2e apple2ee apple2ep lisa2 lisa210 macxl mac512ke macplus \
	a400 a400pal a800 a800pal a800xl kim1 pet cbm30 cbm30b cbm40 cbm40pal cbm40b \
	cbm80 cbm80pal cbm80ger cbm80swe superpet vic20 vic1001 vc20 vic20swe vic20i \
	max c64 c64pal vic64s cbm4064 c64gs cbm500 cbm610 cbm620 cbm620hu cbm710 \
	cbm720 cbm720se c16 c16hun c16c plus4 plus4c c364 c128 c128ger c128fra \
	c128ita c128swe c65 c65e c65d c65c c65ger c65a ibmpc ibmpca pcmda pc europc \
	t1000hx ibmxt pc200 pc20 pc1512 pc1640 xtvga at zx80 zx81 ts1000 aszmic pc8300 \
	pow3000 spectrum specpls4 specbusy specpsch specgrot specimc speclec inves \
	tk90x tk95 tc2048 ts2068 uk2086 spec128 spec128s specpls2 specpl2a specpls3 \
	specp2fr specp2sp specp3sp specpl3e pc1251 pc1401 pc1402 pc1350 pc1403 \
	pc1403h mz700 mz700j ti99_4 ti99_4e ti99_4a ti99_4ae avigo ti81 ti85 ti85v40 \
	ti85v50 ti85v60 ti85v80 ti85v90 ti85v100 ti86 ti86v13 ti86v14 ti86v16 ti86grom \
	pc88srl pc88srh jupiter sordm5 apfm1000 apfimag einstein ep128 ep128a kaypro \
	mbee mbeepc mbee56k trs80 trs80l2 trs80l2a sys80 lnw80 coco cocoe coco2 \
	coco2b coco3 coco3p coco3h dragon32 cp400 mc10 cgenie laser110 laser200 \
	laser210 laser310 vz200 vz300 fellow tx8000 laser350 laser500 laser700 \
	microtan oric1 orica prav8d prav8dd prav8dda telstrat p2000t p2000m uk101 \
	superbrd msx msxj msxkr msxuk hotbit11 hotbit12 expert10 expert11 msx2 msx2a \
	msx2j nascom1 nascom1a nascom1b nascom2 nascom2a coupe coupe512 pdp1 mtx512 \
	intvkbd aquarius exidy galaxy svi318 svi328 svi328a apexc mk1 mk2
endif

ifneq ($(wildcard $(EMUSRC)),)
INSTALL_BINFILES += $(OBJ)/$(EMUNAME)$(EXE)
INSTALL_DATAFILES += $(srcdir)/support/safequit.dat
INSTALL_MANFILES += $(DOCOBJ)/advmame.1
endif
ifneq ($(wildcard $(srcdir)/advance/menu),)
INSTALL_BINFILES += $(MENUOBJ)/advmenu$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advmenu.1
endif
ifneq ($(CONF_SYSTEM),sdl)
ifneq ($(wildcard $(srcdir)/advance/v),)
INSTALL_BINFILES += $(VOBJ)/advv$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advv.1
endif
ifneq ($(wildcard $(srcdir)/advance/cfg),)
INSTALL_BINFILES += $(CFGOBJ)/advcfg$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advcfg.1
endif
ifneq ($(wildcard $(srcdir)/advance/s),)
INSTALL_BINFILES += $(SOBJ)/advs$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advs.1
endif
ifneq ($(wildcard $(srcdir)/advance/k),)
INSTALL_BINFILES += $(KOBJ)/advk$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advk.1
endif
ifneq ($(wildcard $(srcdir)/advance/j),)
INSTALL_BINFILES += $(JOBJ)/advj$(EXE)
INSTALL_MANFILES += $(DOCOBJ)/advj.1
endif
ifneq ($(wildcard $(srcdir)/advance/m),)
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
	$(wildcard $(srcdir)/advance/lib/*.dat)

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
	$(wildcard $(srcdir)/advance/svgalib/*.bat)

SVGALIBDRIVERS_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/drivers/*.c) \
        $(wildcard $(srcdir)/advance/svgalib/drivers/*.h)

SVGALIBCLOCKCHI_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/clockchi/*.c) \
        $(wildcard $(srcdir)/advance/svgalib/clockchi/*.h)

SVGALIBRAMDAC_SRC = \
	$(wildcard $(srcdir)/advance/svgalib/ramdac/*.c) \
	$(wildcard $(srcdir)/advance/svgalib/ramdac/*.h)

COMMON_SRC = \
	$(wildcard $(srcdir)/advance/common/*.c) \
	$(wildcard $(srcdir)/advance/common/*.h)

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

SDL_SRC = \
	$(wildcard $(srcdir)/advance/sdl/*.c) \
	$(wildcard $(srcdir)/advance/sdl/*.h)

D2_SRC = \
	$(wildcard $(srcdir)/advance/d2/*.cc)

CONF_SRC = \
	$(srcdir)/Makefile.in \
	$(srcdir)/config.guess \
	$(srcdir)/config.status \
	$(srcdir)/config.sub \
	$(srcdir)/configure \
	$(srcdir)/configure.ac \
	$(srcdir)/configure.msdos \
	$(srcdir)/configure.windows \
	$(srcdir)/install-sh \
	$(srcdir)/mkinstalldirs

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
	-$(INSTALL_DATA_DIR) $(PREFIX)/doc/advance
	-$(INSTALL_MAN_DIR) $(PREFIX)/man/man1
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance
ifdef INSTALL_DATAFILES
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/rom
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/sample
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/artwork
endif
ifdef INSTALL_IMAGEDIRS
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/image
	@for i in $(INSTALL_IMAGEDIRS); do \
		-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/image/$$i; \
	done
endif

install-data: $(INSTALL_DATAFILES)
ifdef INSTALL_DATAFILES
	@for i in $(INSTALL_DATAFILES); do \
		$(INSTALL_DATA) $$i $(PREFIX)/share/advance; \
	done
endif

uninstall-data:
ifdef INSTALL_DATAFILES
	@for i in $(INSTALL_DATAFILES); do \
		rm -f $(PREFIX)/share/advance/$$i; \
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
		$(INSTALL_DATA) $$i $(PREFIX)/doc/advance; \
	done
endif

uninstall-doc:
ifdef INSTALL_DOCFILES
	@for i in $(INSTALL_DOCFILES); do \
		rm -f $(PREFIX)/doc/advance/$$i; \
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
# Special build

# It seems to be required with gcc 3.1 for DOS. No problem in Linux.
#
#MSG_FIX = $(MSG) "(with low opt)"
#
#CFLAGS_FIX = $(subst -O3,-O2,$(CFLAGS))
#
#$(OBJ)/memory.o: $(EMUSRC)/memory.c
#	$(ECHO) $@ $(MSG_FIX)
#	$(CC) $(CFLAGS_FIX) $(EMUCFLAGS) $(EMUDEFS) -c $< -o $@

############################################################################
# Special Rules

ARCH_COMMON = -O3 -fomit-frame-pointer -Wall -Wno-sign-compare -Wno-unused
ARCH_ALL = CONF_ARCH=i386 CONF_CFLAGS_OPT="-march=i386 $(ARCH_COMMON)"
ARCH_PENTIUM = CONF_ARCH=i586 CONF_CFLAGS_OPT="-march=i586 $(ARCH_COMMON)"
ARCH_PENTIUM2 = CONF_ARCH=i686 CONF_CFLAGS_OPT="-march=i686 $(ARCH_COMMON)"
ARCH_K6 = CONF_ARCH=k6 CONF_CFLAGS_OPT="-march=k6 $(ARCH_COMMON)"

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
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=dos CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_PENTIUM2) CONF=no CONF_HOST=dos CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_K6) CONF=no CONF_HOST=dos CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=windows CONF_MAP=yes CONF_COMPRESS=yes distbin

wholemess:
	$(MAKE) CONF=no CONF_EMU=mess dist
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=dos CONF_EMU=mess CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_PENTIUM2) CONF=no CONF_HOST=dos CONF_EMU=mess CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_K6) CONF=no CONF_HOST=dos CONF_EMU=mess CONF_MAP=yes CONF_COMPRESS=yes distbin

wholepac:
	$(MAKE) CONF=no CONF_EMU=pac dist
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=dos CONF_EMU=pac CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_PENTIUM2) CONF=no CONF_HOST=dos CONF_EMU=pac CONF_MAP=yes CONF_COMPRESS=yes distbin
	$(MAKE) $(ARCH_K6) CONF=no CONF_HOST=dos CONF_EMU=pac CONF_MAP=yes CONF_COMPRESS=yes distbin

wholemenu:
	$(MAKE) CONF=no distmenu
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=dos CONF_SYSTEM=dos CONF_MAP=yes CONF_COMPRESS=yes distmenubin
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=windows CONF_SYSTEM=sdl CONF_MAP=yes CONF_COMPRESS=yes distmenubin
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=unix CONF_SYSTEM=sdl CONF_MAP=yes CONF_COMPRESS=no distmenubin
	$(MAKE) $(ARCH_PENTIUM) CONF=no CONF_HOST=unix CONF_SYSTEM=linux CONF_MAP=yes CONF_COMPRESS=no distmenubin

wholecab:
	$(MAKE) CONF=no CONF_HOST=dos distcab
	$(MAKE) $(ARCH_ALL) CONF=no CONF_HOST=dos CONF_MAP=yes distcabbin

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


