############################################################################
# Menu build

MENUCFLAGS += \
	-Iadvance/$(HOST_SYSTEM) \
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
	$(MENUOBJ)/$(HOST_SYSTEM)
MENUOBJS += \
	$(MENUOBJ)/menu/category.o \
	$(MENUOBJ)/menu/choice.o \
	$(MENUOBJ)/menu/common.o \
	$(MENUOBJ)/menu/crc.o \
	$(MENUOBJ)/menu/emulator.o \
	$(MENUOBJ)/menu/game.o \
	$(MENUOBJ)/menu/mconfig.o \
	$(MENUOBJ)/menu/menu.o \
	$(MENUOBJ)/menu/submenu.o \
	$(MENUOBJ)/menu/mm.o \
	$(MENUOBJ)/menu/play.o \
	$(MENUOBJ)/menu/playsnd.o \
	$(MENUOBJ)/menu/resource.o \
	$(MENUOBJ)/menu/text.o \
	$(MENUOBJ)/lib/log.o \
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
	$(MENUOBJ)/lib/key.o \
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
MENULIBS += $(ZLIBS)

ifeq ($(HOST_SYSTEM),linux)
MENUCFLAGS += -DPREFIX=\"$(PREFIX)\"
MENUCFLAGS += \
	-DUSE_VIDEO_SVGALIB -DUSE_VIDEO_FB -DUSE_VIDEO_NONE \
	-DUSE_SOUND_OSS -DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_SVGALIB -DUSE_MOUSE_SVGALIB -DUSE_JOYSTICK_SVGALIB
MENULIBS += -lvga
MENUOBJS += \
	$(MENUOBJ)/lib/filenix.o \
	$(MENUOBJ)/lib/targnix.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/os.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vsvgab.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vfb.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/soss.o
endif

ifeq ($(HOST_SYSTEM),dos)
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
	$(MENUOBJ)/lib/filedos.o \
	$(MENUOBJ)/lib/targdos.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/os.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vvgal.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vvbe.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vvbel.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vsvgal.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/scrvbe.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/scrvga.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/salleg.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/sseal.o \
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

ifeq ($(HOST_SYSTEM),sdl)
MENUCFLAGS += \
	$(SDLCFLAGS) \
	-DPREFIX=\"$(PREFIX)\" \
	-DUSE_VIDEO_SDL -DUSE_VIDEO_NONE \
	-DUSE_SOUND_SDL -DUSE_SOUND_NONE \
	-DUSE_KEYBOARD_SDL -DUSE_MOUSE_SDL -DUSE_JOYSTICK_SDL
MENULIBS += $(SDLLIBS)
MENUOBJS += \
	$(MENUOBJ)/$(HOST_SYSTEM)/os.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/vsdl.o \
	$(MENUOBJ)/$(HOST_SYSTEM)/ssdl.o
ifeq ($(HOST_TARGET),linux)
MENUOBJS += \
	$(MENUOBJ)/lib/filenix.o \
	$(MENUOBJ)/lib/targnix.o
endif
ifeq ($(HOST_TARGET),windows)
MENUOBJS += \
	$(MENUOBJ)/lib/filedos.o \
	$(MENUOBJ)/lib/targwin.o \
	$(MENUOBJ)/lib/icondef.o
# Customize the SDL_main function
MENUCFLAGS += -DNO_STDIO_REDIRECT
MENUOBJS += $(MENUOBJ)/sdl/sdlmwin.o
endif
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

$(MENUOBJ)/%.o: advance/%.rc
	$(ECHO) $@ $(MSG)
	$(RC) $(RCFLAGS) $< -o $@

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
# MENU dist

MENU_ROOT_SRC = \
	makefile

MENU_SRC = \
	$(wildcard advance/menu/*.c) \
	$(wildcard advance/menu/*.cc) \
	$(wildcard advance/menu/*.h)

MENU_ADVANCE_SRC = \
	advance/advance.mak \
	advance/v.mak \
	advance/cfg.mak \
	advance/d2.mak

MENU_CONTRIB_SRC = \
	$(wildcard contrib/menu/*)

MENU_DOC_SRC = \
	doc/copying \
	doc/license.d \
	doc/advmenu.d \
	doc/authors.d \
	doc/faq.d \
	doc/histmenu.d \
	doc/readmenu.txt \
	doc/relemenu.txt \
	doc/advv.d \
	doc/advcfg.d \
	doc/install.d \
	doc/card.d \
	doc/build.d

MENU_SUPPORT_SRC = \
	$(RCSRC) \
	support/advmenuv.bat \
	support/advmenuc.bat

MENU_DOC_BIN = \
	doc/copying \
	$(D2OBJ)/license.txt \
	$(D2OBJ)/advmenu.txt \
	$(D2OBJ)/authors.txt \
	$(D2OBJ)/faq.txt \
	$(D2OBJ)/histmenu.txt \
	$(D2OBJ)/license.html \
	$(D2OBJ)/advmenu.html \
	$(D2OBJ)/authors.html \
	$(D2OBJ)/faq.html \
	$(D2OBJ)/histmenu.html
ifneq ($(HOST_TARGET),sdl)
MENU_DOC_BIN += \
	$(D2OBJ)/advv.txt \
	$(D2OBJ)/advcfg.txt \
	$(D2OBJ)/install.txt \
	$(D2OBJ)/card.txt \
	$(D2OBJ)/advv.html \
	$(D2OBJ)/advcfg.html \
	$(D2OBJ)/install.html \
	$(D2OBJ)/card.html \
	$(RCSRC)
endif

MENU_ROOT_BIN = \
	$(MENUOBJ)/advmenu$(EXE)
ifeq ($(HOST_SYSTEM),linux)
MENU_ROOT_BIN += \
	$(D2OBJ)/advmenu.1
endif
ifeq ($(HOST_TARGET),dos)
MENU_ROOT_BIN += \
	support/advmenuv.bat \
	support/advmenuc.bat
endif
ifeq ($(HOST_TARGET),windows)
MENU_ROOT_BIN += \
	support/sdl.dll
endif
ifneq ($(HOST_SYSTEM),sdl)
MENU_ROOT_BIN += \
	$(VOBJ)/advv$(EXE) \
	$(CFGOBJ)/advcfg$(EXE)
ifeq ($(HOST_SYSTEM),linux)
MENU_ROOT_BIN += \
	$(D2OBJ)/advv.1 \
	$(D2OBJ)/advcfg.1
endif
endif

MENU_DIST_FILE_SRC = advancemenu-$(MENUVERSION)
MENU_DIST_FILE_BIN = advancemenu-$(MENUVERSION)-$(BINARYTAG)

ifeq ($(HOST_TARGET),dos)
MENU_DIST_DIR_SRC = tmp
MENU_DIST_DIR_BIN = tmpbin
else
MENU_DIST_DIR_SRC = $(MENU_DIST_FILE_SRC)
MENU_DIST_DIR_BIN = $(MENU_DIST_FILE_BIN)
endif

distmenu: $(RCSRC)
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
	mkdir $(MENU_DIST_DIR_SRC)/advance/sdl
	cp $(SDL_SRC) $(MENU_DIST_DIR_SRC)/advance/sdl
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
	mkdir $(MENU_DIST_DIR_SRC)/advance/d2
	cp $(D2_SRC) $(MENU_DIST_DIR_SRC)/advance/d2
	mkdir $(MENU_DIST_DIR_SRC)/contrib
	mkdir $(MENU_DIST_DIR_SRC)/contrib/menu
	cp -R $(MENU_CONTRIB_SRC) $(MENU_DIST_DIR_SRC)/contrib/menu
	rm -f $(MENU_DIST_FILE_SRC).tar.gz
	tar cfzo $(MENU_DIST_FILE_SRC).tar.gz $(MENU_DIST_DIR_SRC)
	rm -r $(MENU_DIST_DIR_SRC)

distmenubin: $(MENU_ROOT_BIN) $(MENU_DOC_BIN)
	mkdir $(MENU_DIST_DIR_BIN)
ifeq ($(HOST_TARGET),linux)
	cp doc/readmenu.txt $(MENU_DIST_DIR_BIN)/README
	cp doc/relemenu.txt $(MENU_DIST_DIR_BIN)/RELEASE
else
	cp doc/readmenu.txt $(MENU_DIST_DIR_BIN)/readme.txt
	cp doc/relemenu.txt $(MENU_DIST_DIR_BIN)/release.txt
endif
	cp $(MENU_ROOT_BIN) $(MENU_DIST_DIR_BIN)
	mkdir $(MENU_DIST_DIR_BIN)/doc
	cp $(MENU_DOC_BIN) $(MENU_DIST_DIR_BIN)/doc
	mkdir $(MENU_DIST_DIR_BIN)/contrib
	cp -R $(MENU_CONTRIB_SRC) $(MENU_DIST_DIR_BIN)/contrib
ifneq ($(HOST_TARGET),linux)
	rm -f $(MENU_DIST_FILE_BIN).zip
	find $(MENU_DIST_DIR_BIN) \( -name "*.txt" \) -type f -exec utod {} \;
	cd $(MENU_DIST_DIR_BIN) && zip -r ../$(MENU_DIST_FILE_BIN).zip *
else
	rm -f $(MENU_DIST_FILE_BIN).tar.gz
	tar cfzo $(MENU_DIST_FILE_BIN).tar.gz $(MENU_DIST_DIR_BIN)
endif
	rm -r $(MENU_DIST_DIR_BIN)
