############################################################################
# COMMON version

ifeq ($(CONF_EMU),mess)
EMUVERSION = 0.56.x
else
ifeq ($(CONF_EMU),pac)
EMUVERSION = 0.58.x
else
EMUVERSION = 0.61.2
endif
endif
MENUVERSION = 2.0.1
CABVERSION = 0.11.3

############################################################################
# COMMON dir

OBJ = obj/$(CONF_EMU)/$(BINARYDIR)
MENUOBJ = obj/menu/$(BINARYDIR)
MOBJ = obj/m/$(BINARYDIR)
JOBJ = obj/j/$(BINARYDIR)
KOBJ = obj/k/$(BINARYDIR)
VOBJ = obj/v/$(BINARYDIR)
SOBJ = obj/s/$(BINARYDIR)
CFGOBJ = obj/cfg/$(BINARYDIR)
LINEOBJ = obj/line/$(BINARYDIR_BUILD)
D2OBJ = obj/d2/$(BINARYDIR_BUILD)

############################################################################
# COMMON targets

ifdef ALL
all_override: $(ALL)
endif

# From MESS 0.56
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

ifneq ($(wildcard $(EMUSRC)),)
INSTALL_BINFILES += $(OBJ)/$(EMUNAME)$(EXE)
INSTALL_DATAFILES += support/safequit.dat
INSTALL_MANFILES += $(D2OBJ)/advmame.1
endif
ifneq ($(wildcard advance/menu),)
INSTALL_BINFILES += $(MENUOBJ)/advmenu$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advmenu.1
endif
ifneq ($(CONF_SYSTEM),sdl)
ifneq ($(wildcard advance/v),)
INSTALL_BINFILES += $(VOBJ)/advv$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advv.1
endif
ifneq ($(wildcard advance/cfg),)
INSTALL_BINFILES += $(CFGOBJ)/advcfg$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advcfg.1
endif
ifneq ($(wildcard advance/s),)
INSTALL_BINFILES += $(SOBJ)/advs$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advs.1
endif
ifneq ($(wildcard advance/k),)
INSTALL_BINFILES += $(KOBJ)/advk$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advk.1
endif
ifneq ($(wildcard advance/j),)
INSTALL_BINFILES += $(JOBJ)/advj$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advj.1
endif
ifneq ($(wildcard advance/m),)
INSTALL_BINFILES += $(MOBJ)/advm$(EXE)
INSTALL_MANFILES += $(D2OBJ)/advm.1
endif
endif

INSTALL_DOCFILES += $(subst doc/,$(D2OBJ)/,$(subst .d,.txt,$(wildcard doc/*.d)))
INSTALL_DOCFILES += $(subst doc/,$(D2OBJ)/,$(subst .d,.html,$(wildcard doc/*.d)))

all: $(INSTALL_BINFILES) $(INSTALL_DOCFILES) $(INSTALL_MANFILES) $(INSTALL_DATAFILES)
emu: $(OBJ)/$(EMUNAME)$(EXE)
menu: $(MENUOBJ)/advmenu$(EXE)
cfg: $(CFGOBJ)/advcfg$(EXE)
v: $(VOBJ)/advv$(EXE)
s: $(SOBJ)/advs$(EXE)
k: $(KOBJ)/advk$(EXE)
j: $(JOBJ)/advj$(EXE)
m: $(MOBJ)/advm$(EXE)
line: $(LINEOBJ)/advline$(EXE_BUILD)
d2: $(D2OBJ)/advd2$(EXE_BUILD)

############################################################################
# COMMON SRC

RCSRC = support/pcvga.rc support/pcsvga60.rc \
	support/standard.rc support/medium.rc \
	support/extended.rc support/pal.rc \
	support/ntsc.rc

MPGLIB_SRC = \
	$(wildcard advance/mpglib/*.c) \
	$(wildcard advance/mpglib/*.h) \
	$(wildcard advance/mpglib/*.txt)

LIB_SRC = \
	$(wildcard advance/lib/*.c) \
	$(wildcard advance/lib/*.h) \
	$(wildcard advance/lib/*.ico) \
	$(wildcard advance/lib/*.rc)

BLIT_SRC = \
	$(wildcard advance/blit/*.c) \
	$(wildcard advance/blit/*.h)

CARD_SRC = \
	$(wildcard advance/card/*.c) \
	$(wildcard advance/card/*.h)
	
SVGALIB_SRC = \
	$(wildcard advance/svgalib/*.c) \
	$(wildcard advance/svgalib/*.h) \
	$(wildcard advance/svgalib/*.dif) \
	$(wildcard advance/svgalib/*.txt) \
	$(wildcard advance/svgalib/*.bat)

SVGALIBDRIVERS_SRC = \
	$(wildcard advance/svgalib/drivers/*.c) \
        $(wildcard advance/svgalib/drivers/*.h)

SVGALIBCLOCKCHI_SRC = \
	$(wildcard advance/svgalib/clockchi/*.c) \
        $(wildcard advance/svgalib/clockchi/*.h)

SVGALIBRAMDAC_SRC = \
	$(wildcard advance/svgalib/ramdac/*.c) \
	$(wildcard advance/svgalib/ramdac/*.h)

COMMON_SRC = \
	$(wildcard advance/common/*.c) \
	$(wildcard advance/common/*.h)

V_SRC = \
	$(wildcard advance/v/*.c) \
	$(wildcard advance/v/*.h)

S_SRC = \
	$(wildcard advance/s/*.c) \
	$(wildcard advance/s/*.h)

K_SRC = \
	$(wildcard advance/k/*.c) \
	$(wildcard advance/k/*.h)

J_SRC = \
	$(wildcard advance/j/*.c) \
	$(wildcard advance/j/*.h)

M_SRC = \
	$(wildcard advance/m/*.c) \
	$(wildcard advance/m/*.h)

CFG_SRC = \
	$(wildcard advance/cfg/*.c)

LINE_SRC = \
	$(wildcard advance/line/*.cc)

SRCOSD = \
	$(wildcard advance/osd/*.c) \
	$(wildcard advance/osd/*.h) \
	$(wildcard advance/osd/*.y) \
	$(wildcard advance/osd/*.l)

LINUX_SRC = \
	$(wildcard advance/linux/*.c) \
	$(wildcard advance/linux/*.h)

DOS_SRC = \
	$(wildcard advance/dos/*.c) \
	$(wildcard advance/dos/*.h)

SDL_SRC = \
	$(wildcard advance/sdl/*.c) \
	$(wildcard advance/sdl/*.h)

D2_SRC = \
	$(wildcard advance/d2/*.cc)

############################################################################
# COMMON install

install_data: $(INSTALL_DATAFILES)
ifdef INSTALL_DATAFILES
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/rom
ifeq ($(CONF_EMU),mess)
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/image
	@for i in $(INSTALL_IMAGEDIRS); do \
		$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/image/$$i; \
	done
endif
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/sample
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/artwork
	@for i in $(INSTALL_DATAFILES); do \
		$(INSTALL_DATA) $$i $(PREFIX)/share/advance; \
	done
endif

install_bin: $(INSTALL_BINFILES)
	@for i in $(INSTALL_BINFILES); do \
		$(INSTALL_PROGRAM) $$i $(PREFIX)/bin; \
	done

install_doc: $(INSTALL_DOCFILES)
ifdef INSTALL_DOCFILES
	-$(INSTALL_DATA_DIR) $(PREFIX)/doc/advance
	@for i in $(INSTALL_DOCFILES); do \
		$(INSTALL_DATA) $$i $(PREFIX)/doc/advance; \
	done
endif

install_man: $(INSTALL_MANFILES)
ifdef INSTALL_MANFILES
	-$(INSTALL_MAN_DIR) $(PREFIX)/man/man1
	@for i in $(INSTALL_MANFILES); do \
		$(INSTALL_DATA) $$i $(PREFIX)/man/man1; \
	done
endif

install: install_bin install_data install_doc install_man

############################################################################
# COMMON build

# Resource include dir
RCFLAGS += --include-dir advance/lib

# Specials rule for forcing -O1 in the video drivers:
# The gcc 3.0.3 (-O3 -fomit-frame-pointer -fstrict-aliasing) miscompile the
# svgalib/r128.c file.

MSG_FIX = $(MSG) "(with low opt)"
CFLAGS_FIX_GCC30 = $(subst -O3,-O1,$(CFLAGS))

$(OBJ)/advance/card/%.o: advance/card/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(TARGETCFLAGS) $(SYSTEMCFLAGS) -c $< -o $@

$(OBJ)/advance/svgalib/%.o: advance/svgalib/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(TARGETCFLAGS) $(SYSTEMCFLAGS) -c $< -o $@

$(MENUOBJ)/card/%.o: advance/card/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(MENUCFLAGS) -c $< -o $@

$(MENUOBJ)/svgalib/%.o: advance/svgalib/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(MENUCFLAGS) -c $< -o $@

$(VOBJ)/card/%.o: advance/card/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(VCFLAGS) -c $< -o $@

$(VOBJ)/svgalib/%.o: advance/svgalib/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(VCFLAGS) -c $< -o $@

$(CFGOBJ)/card/%.o: advance/card/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(CFGCFLAGS) -c $< -o $@

$(CFGOBJ)/svgalib/%.o: advance/svgalib/%.c
	$(ECHO) $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(CFGCFLAGS) -c $< -o $@

############################################################################
# Special Rules

ARCH_ALL = ARCH=i386 CONF_CFLAGS_ARCH="-march=i386 -DUSE_LSB"
ARCH_PENTIUM = ARCH=i586 CONF_CFLAGS_ARCH="-march=i586 -DUSE_LSB -DUSE_ASM_i586"
ARCH_PENTIUM2 = ARCH=i686 CONF_CFLAGS_ARCH="-march=i686 -DUSE_LSB -DUSE_ASM_i586"
ARCH_K6 = ARCH=k6 CONF_CFLAGS_ARCH="-march=k6 -DUSE_LSB -DUSE_ASM_i586"

dosmame:
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos CONF_EMU=mame emu

dosmess:
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos CONF_EMU=mess emu

dospac:
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos CONF_EMU=pac emu

dosmenu:
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos CONF_EMU=mame menu

dosdistbin: dosdistbinpentium2

dosdistbinpentium:
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos distbin

dosdistbinpentium2:
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos distbin

dosdistbink6:
	$(MAKE) $(ARCH_K6) CONF_HOST=dos distbin

dosdistcabbin:
	$(MAKE) $(ARCH_ALL) CONF_HOST=dos distcabbin

dosdistmenubin:
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos CONF_SYSTEM=dos distmenubin

linuxdistmenubin:
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=linux CONF_SYSTEM=linux distmenubin

windowsdistmenubin:
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=windows CONF_SYSTEM=sdl distmenubin

linuxsdldistmenubin:
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=linux CONF_SYSTEM=sdl distmenubin

mame:
	$(MAKE) CONF_EMU=mame emu

mame586:
	$(MAKE) $(ARCH_PENTIUM) CONF_EMU=mame emu

neomame:
	$(MAKE) CONF_EMU=neomame emu

cpmame:
	$(MAKE) CONF_EMU=cpmame emu

messmame:
	$(MAKE) CONF_EMU=mess emu

pacmame:
	$(MAKE) CONF_EMU=pac emu

menu586:
	$(MAKE) $(ARCH_PENTIUM) menu

wholemame:
	$(MAKE) dist
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos CONF_MAP=yes distbin
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos CONF_MAP=yes distbin
	$(MAKE) $(ARCH_K6) CONF_HOST=dos CONF_MAP=yes distbin

wholemess:
	$(MAKE) CONF_EMU=mess dist
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos CONF_EMU=mess CONF_MAP=yes distbin
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos CONF_EMU=mess CONF_MAP=yes distbin
	$(MAKE) $(ARCH_K6) CONF_HOST=dos CONF_EMU=mess CONF_MAP=yes distbin

wholepac:
	$(MAKE) CONF_EMU=pac dist
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos CONF_EMU=pac CONF_MAP=yes distbin
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=dos CONF_EMU=pac CONF_MAP=yes distbin
	$(MAKE) $(ARCH_K6) CONF_HOST=dos CONF_EMU=pac CONF_MAP=yes distbin

wholemenu:
	$(MAKE) distmenu
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=dos CONF_SYSTEM=dos CONF_MAP=yes distmenubin
	$(MAKE) $(ARCH_PENTIUM) CONF_HOST=windows CONF_SYSTEM=sdl CONF_MAP=yes distmenubin
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=linux CONF_SYSTEM=sdl CONF_MAP=yes distmenubin
	$(MAKE) $(ARCH_PENTIUM2) CONF_HOST=linux CONF_SYSTEM=linux CONF_MAP=yes distmenubin

wholecab:
	$(MAKE) CONF_HOST=dos distcab
	$(MAKE) $(ARCH_ALL) CONF_HOST=dos CONF_MAP=yes distcabbin

distmess:
	$(MAKE) CONF_EMU=mess dist

distmessbin:
	$(MAKE) CONF_EMU=mess distbin

distpac:
	$(MAKE) CONF_EMU=pac dist

distpacbin:
	$(MAKE) CONF_EMU=pac distbin

distmame:
	$(MAKE) CONF_EMU=mame dist

distmamebin:
	$(MAKE) CONF_EMU=mame distbin

