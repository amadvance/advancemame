#
# Advance
#

############################################################################
# Version

ifeq ($(TARGET),mess)
TARGETVERSION = 0.56.x
else
ifeq ($(TARGET),pac)
TARGETVERSION = 0.58.x
else
TARGETVERSION = 0.61.0
endif
endif
MENUVERSION = 1.19.1
CABVERSION = 0.11.2

############################################################################
# Utilities dir

MOBJ = obj/m/$(HOST_TARGET)/$(ARCH)
JOBJ = obj/j/$(HOST_TARGET)/$(ARCH)
KOBJ = obj/k/$(HOST_TARGET)/$(ARCH)
VOBJ = obj/v/$(HOST_TARGET)/$(ARCH)
SOBJ = obj/s/$(HOST_TARGET)/$(ARCH)
CFGOBJ = obj/cfg/$(HOST_TARGET)/$(ARCH)
LINEOBJ = obj/line/$(HOST_TARGET)/$(ARCH)
MENUOBJ = obj/menu/$(HOST_TARGET)/$(ARCH)

############################################################################
# Common

ifeq ($(HOST_TARGET),linux)
ifdef PREFIX
OSCFLAGS += -DPREFIX=\"$(PREFIX)\"
VCFLAGS += -DPREFIX=\"$(PREFIX)\"
SCFLAGS += -DPREFIX=\"$(PREFIX)\"
CFGCFLAGS += -DPREFIX=\"$(PREFIX)\"
JCFLAGS += -DPREFIX=\"$(PREFIX)\"
KCFLAGS += -DPREFIX=\"$(PREFIX)\"
MCFLAGS += -DPREFIX=\"$(PREFIX)\"
MENUCFLAGS += -DPREFIX=\"$(PREFIX)\"
endif
endif

############################################################################
# Main target

ifeq ($(TARGET),mess)
OSCFLAGS += -DMESS
endif

ifeq ($(TARGET),pac)
OSCFLAGS += -DPAC
endif

ifeq ($(HOST_TARGET),linux)
TARGETCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_NONE \
	-DUSE_SOUND_OSS -DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_SVGALIB -DUSE_MOUSE_SVGALIB -DUSE_JOYSTICK_SVGALIB
ifdef USE_SMP
TARGETCFLAGS += -DUSE_SMP
TARGETLIBS += -lpthread
endif
endif

ifeq ($(HOST_TARGET),dos)
TARGETCFLAGS += \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE \
	-DUSE_SOUND_ALLEGRO -DUSE_SOUND_SEAL -DUSE_SOUND_NONE
TARGETLDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
TARGETLIBS += -laudio
endif

OSCFLAGS += \
	-Iadvance/$(HOST_TARGET)

ifeq ($(HOST_TARGET),linux)
TARGETCFLAGS += \
	-DPI=M_PI \
	-Dstricmp=strcasecmp \
	-Dstrnicmp=strncasecmp
endif

ifeq ($(HOST_TARGET),dos)
OSCFLAGS += \
	-DUSE_CONFIG_ALLEGRO_WRAPPER \
	-Iadvance/card \
	-Iadvance/svgalib \
	-Iadvance/svgalib/clockchi \
	-Iadvance/svgalib/ramdac \
	-Iadvance/svgalib/drivers
TARGETLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
endif

OSCFLAGS += \
	-Iadvance/osd \
	-Iadvance/lib \
	-Iadvance/common \
	-Iadvance/blit \

TARGETCFLAGS += -Iadvance/osd
M68000FLAGS += -Iadvance/osd

ifeq ($(HOST_TARGET),linux)
TARGETLIBS += -lvga
endif

ifeq ($(HOST_TARGET),dos)
TARGETLIBS += -lalleg
endif

OBJDIRS += \
	$(OBJ) \
	$(OBJ)/advance \
	$(OBJ)/advance/lib \
	$(OBJ)/advance/osd \
	$(OBJ)/advance/blit \
	$(OBJ)/advance/$(HOST_TARGET)

ifeq ($(HOST_TARGET),dos)
OBJDIRS += \
	$(OBJ)/advance/card \
	$(OBJ)/advance/svgalib \
	$(OBJ)/advance/svgalib/ramdac \
	$(OBJ)/advance/svgalib/clockchi \
	$(OBJ)/advance/svgalib/drivers
endif

TARGETOSOBJS += \
	$(OBJ)/advance/osd/advance.o \
	$(OBJ)/advance/osd/glue.o \
	$(OBJ)/advance/osd/videoma.o \
	$(OBJ)/advance/osd/videocf.o \
	$(OBJ)/advance/osd/videomn.o \
	$(OBJ)/advance/osd/estimate.o \
	$(OBJ)/advance/osd/record.o \
	$(OBJ)/advance/osd/sound.o \
	$(OBJ)/advance/osd/input.o \
	$(OBJ)/advance/osd/lexyy.o \
	$(OBJ)/advance/osd/y_tab.o \
	$(OBJ)/advance/osd/script.o \
	$(OBJ)/advance/osd/hscript.o \
	$(OBJ)/advance/osd/safequit.o \
	$(OBJ)/advance/osd/fileio.o \
	$(OBJ)/advance/osd/fuzzy.o \
	$(OBJ)/advance/blit/blit.o \
	$(OBJ)/advance/blit/clear.o \
	$(OBJ)/advance/lib/video.o \
	$(OBJ)/advance/lib/conf.o \
	$(OBJ)/advance/lib/incstr.o \
	$(OBJ)/advance/lib/fz.o \
	$(OBJ)/advance/lib/videoio.o \
	$(OBJ)/advance/lib/update.o \
	$(OBJ)/advance/lib/generate.o \
	$(OBJ)/advance/lib/crtc.o \
	$(OBJ)/advance/lib/crtcbag.o \
	$(OBJ)/advance/lib/monitor.o \
	$(OBJ)/advance/lib/sounddrv.o \
	$(OBJ)/advance/lib/snone.o \
	$(OBJ)/advance/lib/vnone.o \
	$(OBJ)/advance/lib/device.o \
	$(OBJ)/advance/lib/videoall.o \
	$(OBJ)/advance/lib/soundall.o

ifeq ($(HOST_TARGET),linux)
TARGETOSOBJS += \
	$(OBJ)/advance/$(HOST_TARGET)/os.o \
	$(OBJ)/advance/$(HOST_TARGET)/soss.o \
	$(OBJ)/advance/$(HOST_TARGET)/vsvgab.o \
	$(OBJ)/advance/$(HOST_TARGET)/vfb.o
endif

ifeq ($(HOST_TARGET),dos)
TARGETOSOBJS += \
	$(OBJ)/advance/$(HOST_TARGET)/os.o \
	$(OBJ)/advance/$(HOST_TARGET)/sseal.o \
	$(OBJ)/advance/$(HOST_TARGET)/salleg.o \
	$(OBJ)/advance/$(HOST_TARGET)/vvgal.o \
	$(OBJ)/advance/$(HOST_TARGET)/vvbel.o \
	$(OBJ)/advance/$(HOST_TARGET)/vsvgal.o \
	$(OBJ)/advance/$(HOST_TARGET)/scrvbe.o \
	$(OBJ)/advance/$(HOST_TARGET)/scrvga.o \
	$(OBJ)/advance/$(HOST_TARGET)/snprintf.o \
	$(OBJ)/advance/card/card.o \
	$(OBJ)/advance/card/pci.o \
	$(OBJ)/advance/card/map.o \
	$(OBJ)/advance/card/board.o \
	$(OBJ)/advance/svgalib/libdos.o \
	$(OBJ)/advance/svgalib/accel.o \
	$(OBJ)/advance/svgalib/vgaio.o \
	$(OBJ)/advance/svgalib/vgammvga.o \
	$(OBJ)/advance/svgalib/vgaregs.o \
	$(OBJ)/advance/svgalib/vgarelvg.o \
	$(OBJ)/advance/svgalib/drivers/apm.o \
	$(OBJ)/advance/svgalib/drivers/ark.o \
	$(OBJ)/advance/svgalib/drivers/banshee.o \
	$(OBJ)/advance/svgalib/drivers/et6000.o \
	$(OBJ)/advance/svgalib/drivers/g400.o \
	$(OBJ)/advance/svgalib/drivers/pm2.o \
	$(OBJ)/advance/svgalib/drivers/i740.o \
	$(OBJ)/advance/svgalib/drivers/i810.o \
	$(OBJ)/advance/svgalib/drivers/laguna.o \
	$(OBJ)/advance/svgalib/drivers/millenni.o \
	$(OBJ)/advance/svgalib/drivers/mx.o \
	$(OBJ)/advance/svgalib/drivers/nv3.o \
	$(OBJ)/advance/svgalib/drivers/r128.o \
	$(OBJ)/advance/svgalib/drivers/rage.o \
	$(OBJ)/advance/svgalib/drivers/s3.o \
	$(OBJ)/advance/svgalib/drivers/savage.o \
	$(OBJ)/advance/svgalib/drivers/sis.o \
	$(OBJ)/advance/svgalib/drivers/trident.o \
	$(OBJ)/advance/svgalib/drivers/renditio.o \
	$(OBJ)/advance/svgalib/ramdac/ibmrgb52.o \
	$(OBJ)/advance/svgalib/ramdac/attdacs.o \
	$(OBJ)/advance/svgalib/ramdac/icw.o \
	$(OBJ)/advance/svgalib/ramdac/normal.o \
	$(OBJ)/advance/svgalib/ramdac/ramdac.o \
	$(OBJ)/advance/svgalib/ramdac/s3dacs.o \
	$(OBJ)/advance/svgalib/ramdac/sierra.o \
	$(OBJ)/advance/svgalib/ramdac/btdacs.o \
	$(OBJ)/advance/svgalib/ramdac/ics_gend.o \
	$(OBJ)/advance/svgalib/clockchi/icd2061a.o
endif

ifdef WRAPALLOC
TARGETOSOBJS += $(OBJ)/advance/osd/allocw.o
TARGETLDFLAGS += -Xlinker --wrap -Xlinker malloc -Xlinker --wrap -Xlinker free -Xlinker --wrap -Xlinker realloc -Xlinker --wrap -Xlinker strdup
else
TARGETOSOBJS += $(OBJ)/advance/osd/allocz.o
TARGETLDFLAGS += -Xlinker --wrap -Xlinker malloc
endif

############################################################################
# Rules

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

ifneq ($(wildcard advance/osd),)
INSTALL_BINFILES += $(OBJ)/$(TARGETNAME)$(EXE)
INSTALL_DATAFILES += support/safequit.dat
endif
ifneq ($(wildcard advance/v),)
INSTALL_BINFILES += $(VOBJ)/advv$(EXE)
endif
ifneq ($(wildcard advance/cfg),)
INSTALL_BINFILES += $(CFGOBJ)/advcfg$(EXE)
endif
ifneq ($(wildcard advance/s),)
INSTALL_BINFILES += $(SOBJ)/advs$(EXE)
endif
ifneq ($(wildcard advance/k),)
INSTALL_BINFILES += $(KOBJ)/advk$(EXE)
endif
ifneq ($(wildcard advance/j),)
INSTALL_BINFILES += $(JOBJ)/advj$(EXE)
endif
ifneq ($(wildcard advance/m),)
INSTALL_BINFILES += $(MOBJ)/advm$(EXE)
endif
ifneq ($(wildcard advance/line),)
INSTALL_BINFILES += $(LINEOBJ)/advline$(EXE)
endif
ifneq ($(wildcard advance/menu),)
INSTALL_BINFILES += $(MENUOBJ)/advmenu$(EXE)
endif

INSTALL_DOCFILES += $(wildcard doc/*.txt)

all: $(INSTALL_BINFILES)
emu: $(OBJ)/$(TARGETNAME)$(EXE)
menu: $(MENUOBJ)/advmenu$(EXE)
line: $(LINEOBJ)/advline$(EXE)
cfg: $(CFGOBJ)/advcfg$(EXE)
v: $(VOBJ)/advv$(EXE)
s: $(SOBJ)/advs$(EXE)
k: $(KOBJ)/advk$(EXE)
j: $(JOBJ)/advj$(EXE)
m: $(MOBJ)/advm$(EXE)

############################################################################
# Special compile rules

MSG_FIX = $(MSG) "(with low opt)"

# Specials rule for forcing -O1 in the video drivers:
# The gcc 3.0.3 (-O3 -fomit-frame-pointer -fstrict-aliasing) miscompile the
# svgalib/r128.c file.

CFLAGS_FIX_GCC30 = $(subst -O3,-O1,$(CFLAGS))

$(OBJ)/advance/card/%.o: advance/card/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(OSCFLAGS) $(TARGETCFLAGS) -c $< -o $@

$(OBJ)/advance/svgalib/%.o: advance/svgalib/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(OSCFLAGS) $(TARGETCFLAGS) -c $< -o $@

$(MENUOBJ)/card/%.o: advance/card/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(MENUCFLAGS) -c $< -o $@

$(MENUOBJ)/svgalib/%.o: advance/svgalib/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(MENUCFLAGS) -c $< -o $@

$(VOBJ)/card/%.o: advance/card/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(VCFLAGS) -c $< -o $@

$(VOBJ)/svgalib/%.o: advance/svgalib/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(VCFLAGS) -c $< -o $@

$(CFGOBJ)/card/%.o: advance/card/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(CFGCFLAGS) -c $< -o $@

$(CFGOBJ)/svgalib/%.o: advance/svgalib/%.c
	@echo $@ $(MSG_FIX)
	$(CC) $(CFLAGS_FIX_GCC30) $(CFGCFLAGS) -c $< -o $@

############################################################################
# Main compile rules

$(OBJ)/advance/%.o: advance/%.c
	@echo $@ $(MSG)
	$(CC) $(CFLAGS) $(OSCFLAGS) $(TARGETCFLAGS) -c $< -o $@

############################################################################
# Install

install_data:
ifdef INSTALL_DATAFILES
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance
	-$(INSTALL_DATA_DIR) $(PREFIX)/share/advance/rom
ifeq ($(TARGET),mess)
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

install_bin:
	@for i in $(INSTALL_BINFILES); do \
		$(INSTALL_PROGRAM) $$i $(PREFIX)/bin; \
	done

install_doc:
ifdef INSTALL_DOCFILES
	-$(INSTALL_DATA_DIR) $(PREFIX)/doc/advance
	@for i in $(INSTALL_DOCFILES); do \
		$(INSTALL_DATA) $$i $(PREFIX)/doc/advance; \
	done
endif

install: install_bin install_data install_doc

############################################################################
# V

VCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/blit \
	-Iadvance/common
VOBJS = \
	$(VOBJ)/lib/video.o \
	$(VOBJ)/lib/conf.o \
	$(VOBJ)/lib/incstr.o \
	$(VOBJ)/lib/videoio.o \
	$(VOBJ)/lib/update.o \
	$(VOBJ)/lib/generate.o \
	$(VOBJ)/lib/crtc.o \
	$(VOBJ)/lib/crtcbag.o \
	$(VOBJ)/lib/monitor.o \
	$(VOBJ)/lib/device.o \
	$(VOBJ)/lib/gtf.o \
	$(VOBJ)/lib/videoall.o \
	$(VOBJ)/blit/blit.o \
	$(VOBJ)/blit/clear.o \
	$(VOBJ)/v/v.o \
	$(VOBJ)/v/draw.o
VOBJDIRS = \
	$(VOBJ) \
	$(VOBJ)/v \
	$(VOBJ)/lib \
	$(VOBJ)/blit \
	$(VOBJ)/$(HOST_TARGET)

ifeq ($(HOST_TARGET),linux)
VCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_SLANG \
	-DUSE_INPUT_SVGALIB
VLIBS = -lslang -lvga
VOBJS += \
	$(VOBJ)/$(HOST_TARGET)/os.o \
	$(VOBJ)/$(HOST_TARGET)/vsvgab.o \
	$(VOBJ)/$(HOST_TARGET)/vfb.o \
	$(VOBJ)/$(HOST_TARGET)/vslang.o
endif

ifeq ($(HOST_TARGET),dos)
VCFLAGS += \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE -DUSE_VIDEO_VBE \
	-Iadvance/card \
	-Iadvance/svgalib \
	-Iadvance/svgalib/clockchi \
	-Iadvance/svgalib/ramdac \
	-Iadvance/svgalib/drivers
VLIBS = -lalleg
VOBJS += \
	$(VOBJ)/$(HOST_TARGET)/os.o \
	$(VOBJ)/$(HOST_TARGET)/vvgal.o \
	$(VOBJ)/$(HOST_TARGET)/vvbe.o \
	$(VOBJ)/$(HOST_TARGET)/vvbel.o \
	$(VOBJ)/$(HOST_TARGET)/vsvgal.o \
	$(VOBJ)/$(HOST_TARGET)/scrvbe.o \
	$(VOBJ)/$(HOST_TARGET)/scrvga.o \
	$(VOBJ)/card/card.o \
	$(VOBJ)/card/pci.o \
	$(VOBJ)/card/map.o \
	$(VOBJ)/card/board.o \
	$(VOBJ)/svgalib/libdos.o \
	$(VOBJ)/svgalib/accel.o \
	$(VOBJ)/svgalib/vgaio.o \
	$(VOBJ)/svgalib/vgammvga.o \
	$(VOBJ)/svgalib/vgaregs.o \
	$(VOBJ)/svgalib/vgarelvg.o \
	$(VOBJ)/svgalib/drivers/apm.o \
	$(VOBJ)/svgalib/drivers/ark.o \
	$(VOBJ)/svgalib/drivers/banshee.o \
	$(VOBJ)/svgalib/drivers/et6000.o \
	$(VOBJ)/svgalib/drivers/g400.o \
	$(VOBJ)/svgalib/drivers/pm2.o \
	$(VOBJ)/svgalib/drivers/i740.o \
	$(VOBJ)/svgalib/drivers/i810.o \
	$(VOBJ)/svgalib/drivers/laguna.o \
	$(VOBJ)/svgalib/drivers/millenni.o \
	$(VOBJ)/svgalib/drivers/mx.o \
	$(VOBJ)/svgalib/drivers/nv3.o \
	$(VOBJ)/svgalib/drivers/r128.o \
	$(VOBJ)/svgalib/drivers/rage.o \
	$(VOBJ)/svgalib/drivers/s3.o \
	$(VOBJ)/svgalib/drivers/savage.o \
	$(VOBJ)/svgalib/drivers/sis.o \
	$(VOBJ)/svgalib/drivers/trident.o \
	$(VOBJ)/svgalib/drivers/renditio.o \
	$(VOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(VOBJ)/svgalib/ramdac/attdacs.o \
	$(VOBJ)/svgalib/ramdac/icw.o \
	$(VOBJ)/svgalib/ramdac/normal.o \
	$(VOBJ)/svgalib/ramdac/ramdac.o \
	$(VOBJ)/svgalib/ramdac/s3dacs.o \
	$(VOBJ)/svgalib/ramdac/sierra.o \
	$(VOBJ)/svgalib/ramdac/btdacs.o \
	$(VOBJ)/svgalib/ramdac/ics_gend.o \
	$(VOBJ)/svgalib/clockchi/icd2061a.o
VOBJDIRS += \
	$(VOBJ)/card \
	$(VOBJ)/svgalib \
	$(VOBJ)/svgalib/ramdac \
	$(VOBJ)/svgalib/clockchi \
	$(VOBJ)/svgalib/drivers
endif

$(VOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(VCFLAGS) -c $< -o $@

$(sort $(VOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(VOBJ)/advv$(EXE) : $(sort $(VOBJDIRS)) $(VOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(VLDFLAGS) $(VOBJS) $(VLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advv$(EXE)
	$(LN) $@ advv$(EXE)

############################################################################
# P

SCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/mpglib \
	-Iadvance/common
SOBJS = \
	$(SOBJ)/lib/conf.o \
	$(SOBJ)/lib/incstr.o \
	$(SOBJ)/lib/sounddrv.o \
	$(SOBJ)/lib/device.o \
	$(SOBJ)/lib/mixer.o \
	$(SOBJ)/lib/wave.o \
	$(SOBJ)/lib/fz.o \
	$(SOBJ)/lib/soundall.o \
	$(SOBJ)/mpglib/dct64.o \
	$(SOBJ)/mpglib/decode.o \
	$(SOBJ)/mpglib/interfac.o \
	$(SOBJ)/mpglib/internal.o \
	$(SOBJ)/mpglib/layer3.o \
	$(SOBJ)/mpglib/tabinit.o \
	$(SOBJ)/s/s.o
SOBJDIRS = \
	$(SOBJ) \
	$(SOBJ)/s \
	$(SOBJ)/lib \
	$(SOBJ)/mpglib \
	$(SOBJ)/$(HOST_TARGET)

ifeq ($(HOST_TARGET),linux)
SCFLAGS += \
	-DUSE_SOUND_OSS
SLIBS = -lz -lm
SOBJS += \
	$(SOBJ)/$(HOST_TARGET)/os.o \
	$(SOBJ)/$(HOST_TARGET)/soss.o
endif

ifeq ($(HOST_TARGET),dos)
SCFLAGS += \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO
SLIBS = -laudio -lalleg -lz -lm
SLDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
SOBJS += \
	$(SOBJ)/$(HOST_TARGET)/os.o \
	$(SOBJ)/$(HOST_TARGET)/sseal.o \
	$(SOBJ)/$(HOST_TARGET)/salleg.o
endif

$(SOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(SCFLAGS) -c $< -o $@

$(sort $(SOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(SOBJ)/advs$(EXE) : $(sort $(SOBJDIRS)) $(SOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(SLDFLAGS) $(SOBJS) $(SLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advs$(EXE)
	$(LN) $@ advs$(EXE)

############################################################################
# CFG

CFGCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/blit \
	-Iadvance/v \
	-Iadvance/common
CFGOBJDIRS = \
	$(CFGOBJ) \
	$(CFGOBJ)/cfg \
	$(CFGOBJ)/lib \
	$(CFGOBJ)/blit \
	$(CFGOBJ)/v \
	$(CFGOBJ)/$(HOST_TARGET)
CFGOBJS = \
	$(CFGOBJ)/lib/video.o \
	$(CFGOBJ)/lib/conf.o \
	$(CFGOBJ)/lib/incstr.o \
	$(CFGOBJ)/lib/videoio.o \
	$(CFGOBJ)/lib/update.o \
	$(CFGOBJ)/lib/generate.o \
	$(CFGOBJ)/lib/crtc.o \
	$(CFGOBJ)/lib/crtcbag.o \
	$(CFGOBJ)/lib/monitor.o \
	$(CFGOBJ)/lib/gtf.o \
	$(CFGOBJ)/lib/device.o \
	$(CFGOBJ)/lib/videoall.o \
	$(CFGOBJ)/blit/blit.o \
	$(CFGOBJ)/blit/clear.o \
	$(CFGOBJ)/cfg/cfg.o \
	$(CFGOBJ)/cfg/list.o \
	$(CFGOBJ)/v/draw.o

ifeq ($(HOST_TARGET),linux)
CFGCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_SLANG \
	-DUSE_INPUT_SVGALIB \
	-Iadvance/$(HOST_TARGET)
CFGLIBS = -lslang -lvga
CFGOBJS += \
	$(CFGOBJ)/$(HOST_TARGET)/os.o \
	$(CFGOBJ)/$(HOST_TARGET)/vsvgab.o \
	$(CFGOBJ)/$(HOST_TARGET)/vfb.o \
	$(CFGOBJ)/$(HOST_TARGET)/vslang.o
endif

ifeq ($(HOST_TARGET),dos)
CFGCFLAGS += \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/card \
	-Iadvance/svgalib \
	-Iadvance/svgalib/clockchi \
	-Iadvance/svgalib/ramdac \
	-Iadvance/svgalib/drivers
CFGLIBS = -lalleg
CFGOBJS += \
	$(CFGOBJ)/$(HOST_TARGET)/os.o \
	$(CFGOBJ)/$(HOST_TARGET)/vvgal.o \
	$(CFGOBJ)/$(HOST_TARGET)/vvbel.o \
	$(CFGOBJ)/$(HOST_TARGET)/vsvgal.o \
	$(CFGOBJ)/$(HOST_TARGET)/scrvbe.o \
	$(CFGOBJ)/$(HOST_TARGET)/scrvga.o \
	$(CFGOBJ)/card/card.o \
	$(CFGOBJ)/card/pci.o \
	$(CFGOBJ)/card/map.o \
	$(CFGOBJ)/card/board.o \
	$(CFGOBJ)/svgalib/libdos.o \
	$(CFGOBJ)/svgalib/accel.o \
	$(CFGOBJ)/svgalib/vgaio.o \
	$(CFGOBJ)/svgalib/vgammvga.o \
	$(CFGOBJ)/svgalib/vgaregs.o \
	$(CFGOBJ)/svgalib/vgarelvg.o \
	$(CFGOBJ)/svgalib/drivers/apm.o \
	$(CFGOBJ)/svgalib/drivers/ark.o \
	$(CFGOBJ)/svgalib/drivers/banshee.o \
	$(CFGOBJ)/svgalib/drivers/et6000.o \
	$(CFGOBJ)/svgalib/drivers/g400.o \
	$(CFGOBJ)/svgalib/drivers/pm2.o \
	$(CFGOBJ)/svgalib/drivers/i740.o \
	$(CFGOBJ)/svgalib/drivers/i810.o \
	$(CFGOBJ)/svgalib/drivers/laguna.o \
	$(CFGOBJ)/svgalib/drivers/millenni.o \
	$(CFGOBJ)/svgalib/drivers/mx.o \
	$(CFGOBJ)/svgalib/drivers/nv3.o \
	$(CFGOBJ)/svgalib/drivers/r128.o \
	$(CFGOBJ)/svgalib/drivers/rage.o \
	$(CFGOBJ)/svgalib/drivers/s3.o \
	$(CFGOBJ)/svgalib/drivers/savage.o \
	$(CFGOBJ)/svgalib/drivers/sis.o \
	$(CFGOBJ)/svgalib/drivers/trident.o \
	$(CFGOBJ)/svgalib/drivers/renditio.o \
	$(CFGOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(CFGOBJ)/svgalib/ramdac/attdacs.o \
	$(CFGOBJ)/svgalib/ramdac/icw.o \
	$(CFGOBJ)/svgalib/ramdac/normal.o \
	$(CFGOBJ)/svgalib/ramdac/ramdac.o \
	$(CFGOBJ)/svgalib/ramdac/s3dacs.o \
	$(CFGOBJ)/svgalib/ramdac/sierra.o \
	$(CFGOBJ)/svgalib/ramdac/btdacs.o \
	$(CFGOBJ)/svgalib/ramdac/ics_gend.o \
	$(CFGOBJ)/svgalib/clockchi/icd2061a.o
CFGOBJDIRS += \
	$(CFGOBJ)/card \
	$(CFGOBJ)/svgalib \
	$(CFGOBJ)/svgalib/ramdac \
	$(CFGOBJ)/svgalib/clockchi \
	$(CFGOBJ)/svgalib/drivers
endif

$(CFGOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(CFGCFLAGS) -c $< -o $@

$(sort $(CFGOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(CFGOBJ)/advcfg$(EXE) : $(sort $(CFGOBJDIRS)) $(CFGOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(CFGLDFLAGS) $(CFGOBJS) $(CFGLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advcfg$(EXE)
	$(LN) $@ advcfg$(EXE)

############################################################################
# K

KCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/common
KOBJDIRS = \
	$(KOBJ) \
	$(KOBJ)/k \
	$(KOBJ)/lib \
	$(KOBJ)/$(HOST_TARGET)
KOBJS = \
	$(KOBJ)/k/k.o \
	$(KOBJ)/lib/conf.o \
	$(KOBJ)/lib/incstr.o

ifeq ($(HOST_TARGET),linux)
KLIBS = -lvga
KOBJS += $(KOBJ)/$(HOST_TARGET)/os.o
KCFLAGS += \
	-DUSE_KEYBOARD_SVGALIB
endif

ifeq ($(HOST_TARGET),dos)
KLIBS = -lalleg
KOBJS += $(KOBJ)/$(HOST_TARGET)/os.o
endif

$(KOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(KCFLAGS) -c $< -o $@

$(sort $(KOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(KOBJ)/advk$(EXE) : $(sort $(KOBJDIRS)) $(KOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(KLDFLAGS) $(KOBJS) $(KLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advk$(EXE)
	$(LN) $@ advk$(EXE)

############################################################################
# J

JCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/common
JOBJDIRS = \
	$(JOBJ) \
	$(JOBJ)/j \
	$(JOBJ)/lib \
	$(JOBJ)/$(HOST_TARGET)
JOBJS = \
	$(JOBJ)/j/j.o \
	$(JOBJ)/lib/conf.o \
	$(JOBJ)/lib/incstr.o

ifeq ($(HOST_TARGET),linux)
JCFLAGS += \
	-DUSE_JOYSTICK_SVGALIB
JLIBS = -lvga
JOBJS += $(JOBJ)/$(HOST_TARGET)/os.o
endif

ifeq ($(HOST_TARGET),dos)
JCFLAGS += -DUSE_CONFIG_ALLEGRO_WRAPPER
JLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
JLIBS = -lalleg
JOBJS += $(JOBJ)/$(HOST_TARGET)/os.o
endif

$(JOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(JCFLAGS) -c $< -o $@

$(sort $(JOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(JOBJ)/advj$(EXE) : $(sort $(JOBJDIRS)) $(JOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(JLDFLAGS) $(JOBJS) $(JLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advj$(EXE)
	$(LN) $@ advj$(EXE)

############################################################################
# M

MCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/common
MOBJDIRS = \
	$(MOBJ) \
	$(MOBJ)/m \
	$(MOBJ)/lib \
	$(MOBJ)/$(HOST_TARGET)
MOBJS = \
	$(MOBJ)/m/m.o \
	$(MOBJ)/lib/conf.o \
	$(MOBJ)/lib/incstr.o

ifeq ($(HOST_TARGET),linux)
MCFLAGS += \
	-DUSE_MOUSE_SVGALIB
MLIBS = -lvga
MOBJS += $(MOBJ)/$(HOST_TARGET)/os.o
endif

ifeq ($(HOST_TARGET),dos)
MCFLAGS += -DUSE_CONFIG_ALLEGRO_WRAPPER
MLDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
MLIBS = -lalleg
MOBJS += $(MOBJ)/$(HOST_TARGET)/os.o
endif

$(MOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MCFLAGS) -c $< -o $@

$(sort $(MOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(MOBJ)/advm$(EXE) : $(sort $(MOBJDIRS)) $(MOBJS)
	$(ECHO) $@ $(MSG)
	$(LD) $(LDFLAGS) $(MLDFLAGS) $(MOBJS) $(MLIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advm$(EXE)
	$(LN) $@ advm$(EXE)

############################################################################
# Line

LINECFLAGS += \
	-Iadvance/common
LINEOBJDIRS = \
	$(LINEOBJ) \
	$(LINEOBJ)/line
LINEOBJS = \
	$(LINEOBJ)/line/line.o
LINELIBS = -lm

$(LINEOBJ)/%.o: advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX) $(CFLAGS) $(LINECFLAGS) -c $< -o $@

$(sort $(LINEOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(LINEOBJ)/advline$(EXE) : $(sort $(LINEOBJDIRS)) $(LINEOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX) $(LDFLAGS) $(LINELDFLAGS) $(LINEOBJS) $(LINELIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advline$(EXE)
	$(LN) $@ advline$(EXE)

############################################################################
# Menu

MENUCFLAGS += \
	-Iadvance/$(HOST_TARGET) \
	-Iadvance/lib \
	-Iadvance/blit \
	-Iadvance/mpglib \
	-Iadvance/common
MENUOBJDIRS += \
	$(MENUOBJ) \
	$(MENUOBJ)/menu \
	$(MENUOBJ)/lib \
	$(MENUOBJ)/blit \
	$(MENUOBJ)/mpglib \
	$(MENUOBJ)/$(HOST_TARGET)
MENUOBJS += \
	$(MENUOBJ)/menu/category.o \
	$(MENUOBJ)/menu/choice.o \
	$(MENUOBJ)/menu/common.o \
	$(MENUOBJ)/menu/crc.o \
	$(MENUOBJ)/menu/emulator.o \
	$(MENUOBJ)/menu/game.o \
	$(MENUOBJ)/menu/mconfig.o \
	$(MENUOBJ)/menu/menu.o \
	$(MENUOBJ)/menu/mm.o \
	$(MENUOBJ)/menu/play.o \
	$(MENUOBJ)/menu/playsnd.o \
	$(MENUOBJ)/menu/resource.o \
	$(MENUOBJ)/menu/text.o \
	$(MENUOBJ)/lib/mng.o \
	$(MENUOBJ)/lib/bitmap.o \
	$(MENUOBJ)/lib/fz.o \
	$(MENUOBJ)/lib/unzip.o \
	$(MENUOBJ)/lib/mixer.o \
	$(MENUOBJ)/lib/wave.o \
	$(MENUOBJ)/lib/png.o \
	$(MENUOBJ)/lib/pcx.o \
	$(MENUOBJ)/lib/icon.o \
	$(MENUOBJ)/lib/fontdef.o \
	$(MENUOBJ)/lib/font.o \
	$(MENUOBJ)/lib/video.o \
	$(MENUOBJ)/lib/conf.o \
	$(MENUOBJ)/lib/incstr.o \
	$(MENUOBJ)/lib/videoio.o \
	$(MENUOBJ)/lib/update.o \
	$(MENUOBJ)/lib/generate.o \
	$(MENUOBJ)/lib/crtc.o \
	$(MENUOBJ)/lib/crtcbag.o \
	$(MENUOBJ)/lib/monitor.o \
	$(MENUOBJ)/lib/device.o \
	$(MENUOBJ)/lib/sounddrv.o \
	$(MENUOBJ)/lib/snone.o \
	$(MENUOBJ)/lib/readinfo.o \
	$(MENUOBJ)/lib/soundall.o \
	$(MENUOBJ)/lib/videoall.o \
	$(MENUOBJ)/lib/vnone.o \
	$(MENUOBJ)/blit/clear.o \
	$(MENUOBJ)/blit/blit.o \
	$(MENUOBJ)/mpglib/interfac.o \
	$(MENUOBJ)/mpglib/internal.o \
	$(MENUOBJ)/mpglib/decode.o \
	$(MENUOBJ)/mpglib/dct64.o \
	$(MENUOBJ)/mpglib/layer3.o \
	$(MENUOBJ)/mpglib/tabinit.o
MENULIBS += -lz

ifeq ($(HOST_TARGET),linux)
MENUCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_NONE \
	-DUSE_SOUND_OSS -DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_SVGALIB -DUSE_MOUSE_SVGALIB -DUSE_JOYSTICK_SVGALIB
MENULIBS += -lvga
MENUOBJS += \
	$(MENUOBJ)/$(HOST_TARGET)/os.o \
	$(MENUOBJ)/$(HOST_TARGET)/vsvgab.o \
	$(MENUOBJ)/$(HOST_TARGET)/vfb.o \
	$(MENUOBJ)/$(HOST_TARGET)/soss.o
endif

ifeq ($(HOST_TARGET),dos)
MENUCFLAGS += \
	-Iadvance/card \
	-Iadvance/svgalib \
	-Iadvance/svgalib/clockchi \
	-Iadvance/svgalib/ramdac \
	-Iadvance/svgalib/drivers \
	-DUSE_VIDEO_SVGALINE -DUSE_VIDEO_VBELINE -DUSE_VIDEO_VGALINE -DUSE_VIDEO_VBE -DUSE_VIDEO_NONE \
	-DUSE_SOUND_SEAL -DUSE_SOUND_ALLEGRO -DUSE_SOUND_NONE -DUSE_SOUND_INT
MENULDFLAGS += -Xlinker --wrap -Xlinker _mixer_init
MENUCFLAGS += -DUSE_CONFIG_ALLEGRO_WRAPPER
MENULDFLAGS += \
	-Xlinker --wrap -Xlinker get_config_string \
	-Xlinker --wrap -Xlinker get_config_int \
	-Xlinker --wrap -Xlinker set_config_string \
	-Xlinker --wrap -Xlinker set_config_int \
	-Xlinker --wrap -Xlinker get_config_id \
	-Xlinker --wrap -Xlinker set_config_id
MENULIBS += -lalleg -laudio
MENUOBJS += \
	$(MENUOBJ)/$(HOST_TARGET)/os.o \
	$(MENUOBJ)/$(HOST_TARGET)/vvgal.o \
	$(MENUOBJ)/$(HOST_TARGET)/vvbe.o \
	$(MENUOBJ)/$(HOST_TARGET)/vvbel.o \
	$(MENUOBJ)/$(HOST_TARGET)/vsvgal.o \
	$(MENUOBJ)/$(HOST_TARGET)/scrvbe.o \
	$(MENUOBJ)/$(HOST_TARGET)/scrvga.o \
	$(MENUOBJ)/$(HOST_TARGET)/salleg.o \
	$(MENUOBJ)/$(HOST_TARGET)/sseal.o \
	$(MENUOBJ)/card/card.o \
	$(MENUOBJ)/card/pci.o \
	$(MENUOBJ)/card/map.o \
	$(MENUOBJ)/card/board.o \
	$(MENUOBJ)/svgalib/libdos.o \
	$(MENUOBJ)/svgalib/accel.o \
	$(MENUOBJ)/svgalib/vgaio.o \
	$(MENUOBJ)/svgalib/vgammvga.o \
	$(MENUOBJ)/svgalib/vgaregs.o \
	$(MENUOBJ)/svgalib/vgarelvg.o \
	$(MENUOBJ)/svgalib/drivers/apm.o \
	$(MENUOBJ)/svgalib/drivers/ark.o \
	$(MENUOBJ)/svgalib/drivers/banshee.o \
	$(MENUOBJ)/svgalib/drivers/et6000.o \
	$(MENUOBJ)/svgalib/drivers/g400.o \
	$(MENUOBJ)/svgalib/drivers/pm2.o \
	$(MENUOBJ)/svgalib/drivers/i740.o \
	$(MENUOBJ)/svgalib/drivers/i810.o \
	$(MENUOBJ)/svgalib/drivers/laguna.o \
	$(MENUOBJ)/svgalib/drivers/millenni.o \
	$(MENUOBJ)/svgalib/drivers/mx.o \
	$(MENUOBJ)/svgalib/drivers/nv3.o \
	$(MENUOBJ)/svgalib/drivers/r128.o \
	$(MENUOBJ)/svgalib/drivers/rage.o \
	$(MENUOBJ)/svgalib/drivers/s3.o \
	$(MENUOBJ)/svgalib/drivers/savage.o \
	$(MENUOBJ)/svgalib/drivers/sis.o \
	$(MENUOBJ)/svgalib/drivers/trident.o \
	$(MENUOBJ)/svgalib/drivers/renditio.o \
	$(MENUOBJ)/svgalib/ramdac/ibmrgb52.o \
	$(MENUOBJ)/svgalib/ramdac/attdacs.o \
	$(MENUOBJ)/svgalib/ramdac/icw.o \
	$(MENUOBJ)/svgalib/ramdac/normal.o \
	$(MENUOBJ)/svgalib/ramdac/ramdac.o \
	$(MENUOBJ)/svgalib/ramdac/s3dacs.o \
	$(MENUOBJ)/svgalib/ramdac/sierra.o \
	$(MENUOBJ)/svgalib/ramdac/btdacs.o \
	$(MENUOBJ)/svgalib/ramdac/ics_gend.o \
	$(MENUOBJ)/svgalib/clockchi/icd2061a.o
MENUOBJDIRS += \
	$(MENUOBJ)/card \
	$(MENUOBJ)/svgalib \
	$(MENUOBJ)/svgalib/ramdac \
	$(MENUOBJ)/svgalib/clockchi \
	$(MENUOBJ)/svgalib/drivers
endif

ifdef MAP
MENULDFLAGS += -Xlinker -Map -Xlinker $(MENUOBJ)/advmenu.map
endif

$(MENUOBJ)/%.o: advance/%.cc
	$(ECHO) $@ $(MSG)
	$(CXX) $(CFLAGS) $(MENUCFLAGS) -c $< -o $@

$(MENUOBJ)/%.o: advance/%.c
	$(ECHO) $@ $(MSG)
	$(CC) $(CFLAGS) $(MENUCFLAGS) -c $< -o $@

$(sort $(MENUOBJDIRS)):
	$(ECHO) $@
	$(MD) $@

$(MENUOBJ)/advmenu$(EXE) : $(sort $(MENUOBJDIRS)) $(MENUOBJS)
	$(ECHO) $@ $(MSG)
	$(LDXX) $(LDFLAGS) $(MENULDFLAGS) $(MENUOBJS) $(MENULIBS) -o $@
ifeq ($(COMPRESS),1)
	$(UPX) $@
	$(TOUCH) $@
endif
	$(RM) advmenu$(EXE)
	$(LN) $@ advmenu$(EXE)

############################################################################
# RC

RCSRC = support/pcvga.rc support/pcsvga60.rc \
	support/standard.rc support/medium.rc \
	support/extended.rc support/pal.rc \
	support/ntsc.rc

############################################################################
# Diff

advance/advmame.dif: src src.ori
	find src \( -name "*.orig" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive -x "msdos" -x "unix" -x "windows" -x "windowsui" -x "--linux-.---" src.ori src > advance/advmame.dif
	ls -l advance/advmame.dif

advance/advpac.dif: srcpac srcpac.ori
	find srcpac \( -name "*.orig" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive -x "msdos" -x "unix" -x "windows" -x "windowsui" -x "--linux-.---" srcpac.ori srcpac > advance/advpac.dif
	ls -l advance/advpac.dif

advance/advmess.dif: srcmess srcmess.ori
	find srcmess \( -name "*.orig" -o -name "*.rej" -o -name "*~" -o -name "*.bak" \)
	-diff -U 5 --new-file --recursive -x "msdos" -x "unix" -x "windows" -x "windowsui" -x "--linux-.---" srcmess.ori srcmess > advance/advmess.dif
	ls -l advance/advmess.dif

############################################################################
# Test for a clean makefile

testmake:
	grep "^#HOST_TARGET" makefile > /dev/null
	grep "^#DEBUG=1" makefile > /dev/null
	grep "^#PROFILE=1" makefile > /dev/null
	grep "^#SYMBOLS" makefile > /dev/null
	grep "^#MAP=1" makefile > /dev/null
	grep "^COMPRESS" makefile > /dev/null

############################################################################
# Dist MAME

TARGET_ROOT_SRC = \
	makefile

TARGET_ADVANCE_SRC = \
	advance/advance.mak

ifeq ($(TARGET),mess)
TARGET_ADVANCE_SRC += advance/advmess.dif
else
ifeq ($(TARGET),pac)
TARGET_ADVANCE_SRC += advance/advpac.dif
else
TARGET_ADVANCE_SRC += advance/advmame.dif
endif
endif

MPGLIB_SRC = \
	$(wildcard advance/mpglib/*.c) \
	$(wildcard advance/mpglib/*.h) \
	$(wildcard advance/mpglib/*.txt)

LIB_SRC = \
	$(wildcard advance/lib/*.c) \
	$(wildcard advance/lib/*.h)

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

TARGET_CONTRIB_SRC = \
	$(wildcard contrib/mame/*)

TARGET_SUPPORT_SRC = \
	$(RCSRC) \
	support/safequit.dat

TARGET_DOC_COMMON = \
	doc/advmame.txt \
	doc/advv.txt \
	doc/advcfg.txt \
	doc/advk.txt \
	doc/advs.txt \
	doc/advj.txt \
	doc/advm.txt \
	doc/advline.txt \
	doc/card.txt \
	doc/readme.txt \
	doc/release.txt \
	doc/install.txt \
	doc/license.txt \
	doc/authors.txt \
	doc/script.txt \
	doc/history.txt \
	doc/copying \
	doc/faq.txt \
	doc/tips.txt

TARGET_DOC_SRC = \
	$(TARGET_DOC_COMMON) \
	doc/build.txt

TARGET_ROOT_BIN = \
	$(TARGET_DOC_COMMON) \
	$(TARGET_SUPPORT_SRC) \
	$(OBJ)/$(TARGETNAME)$(EXE) \
	$(KOBJ)/advk$(EXE) \
	$(SOBJ)/advs$(EXE) \
	$(JOBJ)/advj$(EXE) \
	$(MOBJ)/advm$(EXE) \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE) \
	$(LINEOBJ)/advline$(EXE)

ifeq ($(TARGET),mess)
TARGET_ROOT_BIN += support/advmessv.bat support/advmessc.bat
TARGET_DOC_SRC += support/advmessv.bat support/advmessc.bat
endif
ifeq ($(TARGET),pac)
TARGET_ROOT_BIN += support/advpacv.bat support/advpacc.bat
TARGET_DOC_SRC += support/advpacv.bat support/advpacc.bat
endif

TARGET_DISTFILE_SRC = advance$(TARGET)-$(TARGETVERSION)
TARGET_DISTFILE_BIN = advance$(TARGET)-$(TARGETVERSION)-$(HOST_TARGET)-$(ARCH)

ifeq ($(HOST_TARGET),dos)
TARGET_DISTDIR_SRC = tmp
TARGET_DISTDIR_BIN = tmpbin
else
TARGET_DISTDIR_SRC = $(TARGET_DISTFILE_SRC)
TARGET_DISTDIR_BIN = $(TARGET_DISTFILE_BIN)
endif

dist: testmake $(RCSRC) $(TARGET_ADVANCE_SRC)
	mkdir $(TARGET_DISTDIR_SRC)
	cp $(TARGET_ROOT_SRC) $(TARGET_DISTDIR_SRC)
	mkdir $(TARGET_DISTDIR_SRC)/doc
	cp $(TARGET_DOC_SRC) $(TARGET_DISTDIR_SRC)/doc
	mkdir $(TARGET_DISTDIR_SRC)/advance
	cp $(TARGET_ADVANCE_SRC) $(TARGET_DISTDIR_SRC)/advance
	mkdir $(TARGET_DISTDIR_SRC)/support
	cp $(TARGET_SUPPORT_SRC) $(TARGET_DISTDIR_SRC)/support
	mkdir $(TARGET_DISTDIR_SRC)/advance/linux
	cp $(LINUX_SRC) $(TARGET_DISTDIR_SRC)/advance/linux
	mkdir $(TARGET_DISTDIR_SRC)/advance/dos
	cp $(DOS_SRC) $(TARGET_DISTDIR_SRC)/advance/dos
	mkdir $(TARGET_DISTDIR_SRC)/advance/osd
	cp $(SRCOSD) $(TARGET_DISTDIR_SRC)/advance/osd
	mkdir $(TARGET_DISTDIR_SRC)/advance/common
	cp $(COMMON_SRC) $(TARGET_DISTDIR_SRC)/advance/common
	mkdir $(TARGET_DISTDIR_SRC)/advance/lib
	cp $(LIB_SRC) $(TARGET_DISTDIR_SRC)/advance/lib
	mkdir $(TARGET_DISTDIR_SRC)/advance/blit
	cp $(BLIT_SRC) $(TARGET_DISTDIR_SRC)/advance/blit
	mkdir $(TARGET_DISTDIR_SRC)/advance/card
	cp $(CARD_SRC) $(TARGET_DISTDIR_SRC)/advance/card
	mkdir $(TARGET_DISTDIR_SRC)/advance/svgalib
	cp $(SVGALIB_SRC) $(TARGET_DISTDIR_SRC)/advance/svgalib
	mkdir $(TARGET_DISTDIR_SRC)/advance/svgalib/clockchi
	cp $(SVGALIBCLOCKCHI_SRC) $(TARGET_DISTDIR_SRC)/advance/svgalib/clockchi
	mkdir $(TARGET_DISTDIR_SRC)/advance/svgalib/ramdac
	cp $(SVGALIBRAMDAC_SRC) $(TARGET_DISTDIR_SRC)/advance/svgalib/ramdac
	mkdir $(TARGET_DISTDIR_SRC)/advance/svgalib/drivers
	cp $(SVGALIBDRIVERS_SRC) $(TARGET_DISTDIR_SRC)/advance/svgalib/drivers
	mkdir $(TARGET_DISTDIR_SRC)/advance/mpglib
	cp $(MPGLIB_SRC) $(TARGET_DISTDIR_SRC)/advance/mpglib
	mkdir $(TARGET_DISTDIR_SRC)/advance/v
	cp $(V_SRC) $(TARGET_DISTDIR_SRC)/advance/v
	mkdir $(TARGET_DISTDIR_SRC)/advance/k
	cp $(K_SRC) $(TARGET_DISTDIR_SRC)/advance/k
	mkdir $(TARGET_DISTDIR_SRC)/advance/j
	cp $(J_SRC) $(TARGET_DISTDIR_SRC)/advance/j
	mkdir $(TARGET_DISTDIR_SRC)/advance/m
	cp $(M_SRC) $(TARGET_DISTDIR_SRC)/advance/m
	mkdir $(TARGET_DISTDIR_SRC)/advance/s
	cp $(S_SRC) $(TARGET_DISTDIR_SRC)/advance/s
	mkdir $(TARGET_DISTDIR_SRC)/advance/cfg
	cp $(CFG_SRC) $(TARGET_DISTDIR_SRC)/advance/cfg
	mkdir $(TARGET_DISTDIR_SRC)/advance/line
	cp $(LINE_SRC) $(TARGET_DISTDIR_SRC)/advance/line
	mkdir $(TARGET_DISTDIR_SRC)/contrib
	mkdir $(TARGET_DISTDIR_SRC)/contrib/mame
	cp -R $(TARGET_CONTRIB_SRC) $(TARGET_DISTDIR_SRC)/contrib/mame
	rm -f $(TARGET_DISTFILE_SRC).zip
	cd $(TARGET_DISTDIR_SRC) && zip -r ../$(TARGET_DISTFILE_SRC).zip *
	rm -f $(TARGET_DISTFILE_SRC).tar.gz
	tar cfzo $(TARGET_DISTFILE_SRC).tar.gz $(TARGET_DISTDIR_SRC)
	rm -r $(TARGET_DISTDIR_SRC)

distbin: $(TARGET_ROOT_BIN)
	mkdir $(TARGET_DISTDIR_BIN)
	cp $(TARGET_ROOT_BIN) $(TARGET_DISTDIR_BIN)
	mkdir $(TARGET_DISTDIR_BIN)/contrib
	cp -R $(TARGET_CONTRIB_SRC) $(TARGET_DISTDIR_BIN)/contrib
ifeq ($(HOST_TARGET),dos)
	rm -f $(TARGET_DISTFILE_BIN).zip
	find $(TARGET_DISTDIR_BIN) \( -name "*.txt" \) -type f -exec utod {} \;
	cd $(TARGET_DISTDIR_BIN) && zip -r ../$(TARGET_DISTFILE_BIN).zip *
else
	rm -f $(TARGET_DISTFILE_BIN).tar.gz
	tar cfzo $(TARGET_DISTFILE_BIN).tar.gz $(TARGET_DISTDIR_BIN)
endif
	rm -r $(TARGET_DISTDIR_BIN)

distmess:
	$(MAKE) TARGET=mess dist

distmessbin:
	$(MAKE) TARGET=mess distbin

distpac:
	$(MAKE) TARGET=pac dist

distpacbin:
	$(MAKE) TARGET=pac distbin

distmame:
	$(MAKE) TARGET=mame dist

distmamebin:
	$(MAKE) TARGET=mame distbin

############################################################################
# Dist MENU

MENU_ROOT_SRC = \
	makefile

MENU_SRC = \
	$(wildcard advance/menu/*.c) \
	$(wildcard advance/menu/*.cc) \
	$(wildcard advance/menu/*.h)

MENU_ADVANCE_SRC = \
	advance/advance.mak

MENU_CONTRIB_SRC = \
	$(wildcard contrib/menu/*)

MENU_DOC_COMMON = \
	doc/advmenu.txt \
	doc/advv.txt \
	doc/advcfg.txt \
	doc/install.txt \
	doc/card.txt \
	doc/license.txt \
	doc/authors.txt \
	doc/copying

MENU_DOC_SRC = \
	$(MENU_DOC_COMMON) \
	doc/build.txt \
	doc/histmenu.txt \
	doc/readmenu.txt \
	doc/relemenu.txt

MENU_SUPPORT_SRC = \
	$(RCSRC) \
	support/advmenuv.bat \
	support/advmenuc.bat

MENU_ROOT_BIN = \
	$(MENU_DOC_COMMON) \
	$(MENU_SUPPORT_SRC) \
	$(MENUOBJ)/advmenu$(EXE) \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE)

MENU_DIST_FILE_SRC = advancemenu-$(MENUVERSION)
MENU_DIST_FILE_BIN = advancemenu-$(MENUVERSION)-$(HOST_TARGET)-$(ARCH)

ifeq ($(HOST_TARGET),dos)
MENU_DIST_DIR_SRC = tmp
MENU_DIST_DIR_BIN = tmpbin
else
MENU_DIST_DIR_SRC = $(MENU_DIST_FILE_SRC)
MENU_DIST_DIR_BIN = $(MENU_DIST_FILE_BIN)
endif

distmenu: testmake $(RCSRC)
	mkdir $(MENU_DIST_DIR_SRC)
	cp $(MENU_ROOT_SRC) $(MENU_DIST_DIR_SRC)
	mkdir $(MENU_DIST_DIR_SRC)/doc
	cp $(MENU_DOC_SRC) $(MENU_DIST_DIR_SRC)/doc
	mkdir $(MENU_DIST_DIR_SRC)/support
	cp $(MENU_SUPPORT_SRC) $(MENU_DIST_DIR_SRC)/support
	mkdir $(MENU_DIST_DIR_SRC)/advance
	cp $(MENU_ADVANCE_SRC) $(MENU_DIST_DIR_SRC)/advance
	mkdir $(MENU_DIST_DIR_SRC)/advance/linux
	cp $(LINUX_SRC) $(MENU_DIST_DIR_SRC)/advance/linux
	mkdir $(MENU_DIST_DIR_SRC)/advance/dos
	cp $(DOS_SRC) $(MENU_DIST_DIR_SRC)/advance/dos
	mkdir $(MENU_DIST_DIR_SRC)/advance/menu
	cp $(MENU_SRC) $(MENU_DIST_DIR_SRC)/advance/menu
	mkdir $(MENU_DIST_DIR_SRC)/advance/common
	cp $(COMMON_SRC) $(MENU_DIST_DIR_SRC)/advance/common
	mkdir $(MENU_DIST_DIR_SRC)/advance/lib
	cp $(LIB_SRC) $(MENU_DIST_DIR_SRC)/advance/lib
	mkdir $(MENU_DIST_DIR_SRC)/advance/blit
	cp $(BLIT_SRC) $(MENU_DIST_DIR_SRC)/advance/blit
	mkdir $(MENU_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(MENU_DIST_DIR_SRC)/advance/card
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib
	cp $(SVGALIB_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/clockchi
	cp $(SVGALIBCLOCKCHI_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/clockchi
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/ramdac
	cp $(SVGALIBRAMDAC_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/ramdac
	mkdir $(MENU_DIST_DIR_SRC)/advance/svgalib/drivers
	cp $(SVGALIBDRIVERS_SRC) $(MENU_DIST_DIR_SRC)/advance/svgalib/drivers
	mkdir $(MENU_DIST_DIR_SRC)/advance/mpglib
	cp $(MPGLIB_SRC) $(MENU_DIST_DIR_SRC)/advance/mpglib
	mkdir $(MENU_DIST_DIR_SRC)/advance/v
	cp $(V_SRC) $(MENU_DIST_DIR_SRC)/advance/v
	mkdir $(MENU_DIST_DIR_SRC)/contrib
	mkdir $(MENU_DIST_DIR_SRC)/contrib/menu
	cp -R $(MENU_CONTRIB_SRC) $(MENU_DIST_DIR_SRC)/contrib/menu
	rm -f $(MENU_DIST_FILE_SRC).zip
	cd $(MENU_DIST_DIR_SRC) && zip -r ../$(MENU_DIST_FILE_SRC).zip *
	rm -f $(MENU_DIST_FILE_SRC).tar.gz
	tar cfzo $(MENU_DIST_FILE_SRC).tar.gz $(MENU_DIST_DIR_SRC)
	rm -r $(MENU_DIST_DIR_SRC)

distmenubin: $(MENU_ROOT_BIN)
	mkdir $(MENU_DIST_DIR_BIN)
	cp doc/readmenu.txt $(MENU_DIST_DIR_BIN)/readme.txt
	cp doc/relemenu.txt $(MENU_DIST_DIR_BIN)/release.txt
	cp doc/histmenu.txt $(MENU_DIST_DIR_BIN)/history.txt
	cp $(MENU_ROOT_BIN) $(MENU_DIST_DIR_BIN)
	mkdir $(MENU_DIST_DIR_BIN)/contrib
	cp -R $(MENU_CONTRIB_SRC) $(MENU_DIST_DIR_BIN)/contrib
ifeq ($(HOST_TARGET),dos)
	rm -f $(MENU_DIST_FILE_BIN).zip
	find $(MENU_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec utod {} \;
	cd $(MENU_DIST_DIR_BIN) && zip -r ../$(MENU_DIST_FILE_BIN).zip *
else
	rm -f $(MENU_DIST_FILE_BIN).tar.gz
	tar cfzo $(MENU_DIST_FILE_BIN).tar.gz $(MENU_DIST_DIR_BIN)
endif
	rm -r $(MENU_DIST_DIR_BIN)


############################################################################
# Dist CAB

TSR_SRC = \
	$(wildcard advance/tsr/*.asm) \
	$(wildcard advance/tsr/*.c) \
	$(wildcard advance/tsr/*.h)

VBE_SRC = \
	advance/vbe/makefile \
	$(wildcard advance/vbe/*.asm) \
	$(wildcard advance/vbe/*.c) \
	$(wildcard advance/vbe/*.h)

VGA_SRC = \
	advance/vga/makefile \
	$(wildcard advance/vga/*.asm) \
	$(wildcard advance/vga/*.c) \
	$(wildcard advance/vga/*.h)

OFF_SRC = \
	advance/off/makefile \
	$(wildcard advance/off/*.c) \
	$(wildcard advance/off/*.h)

PORTIO_SRC = \
	advance/portio/makefile \
	$(wildcard advance/portio/*.c) \
	$(wildcard advance/portio/*.h)

VIDEO_SRC = \
	advance/video/makefile \
	$(wildcard advance/video/*.c) \
	$(wildcard advance/video/*.h)

CAB_DOC_COMMON = \
	doc/vbe.txt \
	doc/vga.txt \
	doc/video.txt \
	doc/off.txt \
	doc/portio.txt \
	doc/copying

CAB_DOC_SRC = \
	$(CAB_DOC_COMMON) \
	doc/histcab.txt \
	doc/readcab.txt

CAB_SUPPORT_SRC = \
	$(RCSRC) \
	support/video.pcx \
	support/videobis.pcx \
	support/vbev.bat \
	support/vgav.bat

CAB_CONTRIB_SRC = \
	$(wildcard contrib/cab/*)

CAB_ROOT_BIN = \
        $(CAB_DOC_COMMON) \
	$(CAB_SUPPORT_SRC) \
	doc/advline.txt \
	doc/advv.txt \
	advance/vbe/vbe.com \
	advance/vga/vga.exe \
	advance/video/video.exe \
	advance/off/off.com \
	advance/portio/portio.exe \
	$(VOBJ)/advv$(EXE) \
	$(LINEOBJ)/advline$(EXE)

CAB_DIST_DIR_SRC = tmpcab
CAB_DIST_DIR_BIN = tmpcabbin

distcab: $(RCSRC)
	mkdir $(CAB_DIST_DIR_SRC)
	mkdir $(CAB_DIST_DIR_SRC)/doc
	cp $(CAB_DOC_SRC) $(CAB_DIST_DIR_SRC)/doc
	mkdir $(CAB_DIST_DIR_SRC)/support
	cp $(CAB_SUPPORT_SRC) $(CAB_DIST_DIR_SRC)/support
	mkdir $(CAB_DIST_DIR_SRC)/advance
	mkdir $(CAB_DIST_DIR_SRC)/advance/common
	cp $(COMMON_SRC) $(CAB_DIST_DIR_SRC)/advance/common
	mkdir $(CAB_DIST_DIR_SRC)/advance/card
	cp $(CARD_SRC) $(CAB_DIST_DIR_SRC)/advance/card
	mkdir $(CAB_DIST_DIR_SRC)/advance/tsr
	cp $(TSR_SRC) $(CAB_DIST_DIR_SRC)/advance/tsr
	mkdir $(CAB_DIST_DIR_SRC)/advance/vbe
	cp $(VBE_SRC) $(CAB_DIST_DIR_SRC)/advance/vbe
	mkdir $(CAB_DIST_DIR_SRC)/advance/vga
	cp $(VGA_SRC) $(CAB_DIST_DIR_SRC)/advance/vga
	mkdir $(CAB_DIST_DIR_SRC)/advance/video
	cp $(VIDEO_SRC) $(CAB_DIST_DIR_SRC)/advance/video
	mkdir $(CAB_DIST_DIR_SRC)/advance/off
	cp $(OFF_SRC) $(CAB_DIST_DIR_SRC)/advance/off
	mkdir $(CAB_DIST_DIR_SRC)/advance/portio
	cp $(PORTIO_SRC) $(CAB_DIST_DIR_SRC)/advance/portio
	mkdir $(CAB_DIST_DIR_SRC)/contrib
	mkdir $(CAB_DIST_DIR_SRC)/contrib/cab
	cp -R $(CAB_CONTRIB_SRC) $(CAB_DIST_DIR_SRC)/contrib/cab
	rm -f advancecab-$(CABVERSION).zip
	cd $(CAB_DIST_DIR_SRC) && zip -r ../advancecab-$(CABVERSION).zip *
	rm -r $(CAB_DIST_DIR_SRC)

distcabbin: $(CAB_ROOT_BIN)
	mkdir $(CAB_DIST_DIR_BIN)
	cp doc/readcab.txt $(CAB_DIST_DIR_BIN)/readme.txt
	cp doc/histcab.txt $(CAB_DIST_DIR_BIN)/history.txt
	cp $(CAB_ROOT_BIN) $(CAB_DIST_DIR_BIN)
	mkdir $(CAB_DIST_DIR_BIN)/contrib
	cp -r $(CAB_CONTRIB_SRC) $(CAB_DIST_DIR_BIN)/contrib
	rm -f advancecab-$(CABVERSION)-$(HOST_TARGET)-$(ARCH).zip
	cd $(CAB_DIST_DIR_BIN) && zip -r ../advancecab-$(CABVERSION)-$(HOST_TARGET)-$(ARCH).zip *
	rm -r $(CAB_DIST_DIR_BIN)

############################################################################
# ...

ARCH_ALL = ARCH=i386 CFLAGS_ARCH_OVERRIDE="-march=i386 -DUSE_LSB"
ARCH_PENTIUM = ARCH=i586 CFLAGS_ARCH_OVERRIDE="-march=i586 -DUSE_LSB -DUSE_ASM_i586"
ARCH_PENTIUM2 = ARCH=i686 CFLAGS_ARCH_OVERRIDE="-march=i686 -DUSE_LSB -DUSE_ASM_i586 -DUSE_ASM_MMX"
ARCH_K6 = ARCH=k6 CFLAGS_ARCH_OVERRIDE="-march=k6 -DUSE_LSB -DUSE_ASM_i586 -DUSE_ASM_MMX"

dosmame:
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos TARGET=mame emu

dosmess:
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos TARGET=mess emu

dospac:
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos TARGET=pac emu

dosmenu:
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos TARGET=mame menu

dosdistbin: dosdistbinpentium2

dosdistbinpentium:
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos distbin

dosdistbinpentium2:
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos distbin

dosdistbink6:
	$(MAKE) $(ARCH_K6) HOST_TARGET=dos distbin

dosdistcabbin:
	$(MAKE) $(ARCH_ALL) HOST_TARGET=dos distcabbin

dosdistmenubin: dosdistmenubinpentium

dosdistmenubinpentium:
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos distmenubin

dosdistmenubinpentium2:
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos distmenubin

dosdistmenubink6:
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos distmenubin

mame:
	$(MAKE) TARGET=mame emu

neomame:
	$(MAKE) TARGET=neomame emu

cpmame:
	$(MAKE) TARGET=cpmame emu

messmame:
	$(MAKE) TARGET=mess emu

pacmame:
	$(MAKE) TARGET=pac emu

wholemame:
	$(MAKE) dist
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos MAP=1 distbin
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos MAP=1 distbin
	$(MAKE) $(ARCH_K6) HOST_TARGET=dos MAP=1 distbin

wholemess:
	$(MAKE) TARGET=mess dist
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos TARGET=mess MAP=1 distbin
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos TARGET=mess MAP=1 distbin
	$(MAKE) $(ARCH_K6) HOST_TARGET=dos TARGET=mess MAP=1 distbin

wholepac:
	$(MAKE) TARGET=pac dist
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos TARGET=pac MAP=1 distbin
	$(MAKE) $(ARCH_PENTIUM2) HOST_TARGET=dos TARGET=pac MAP=1 distbin
	$(MAKE) $(ARCH_K6) HOST_TARGET=dos TARGET=pac MAP=1 distbin

wholemenu:
	$(MAKE) distmenu
	$(MAKE) $(ARCH_PENTIUM) HOST_TARGET=dos MAP=1 distmenubin

wholecab:
	$(MAKE) HOST_TARGET=dos distcab
	$(MAKE) $(ARCH_ALL) HOST_TARGET=dos MAP=1 distcabbin
